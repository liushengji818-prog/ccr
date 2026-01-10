#include "Mode.hpp"
#include "picosha2.h"

#include <array>
#include <cctype>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

Mode::Mode() : score(0), prefixCount(0), suffixCount(0), matchingCount(0) {}

// Base58 alphabet (Bitcoin/Tron)
static const char kBase58Alphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// Minimal Base58 decode (no BigInt lib). Returns full decoded bytes.
static bool Base58Decode(const std::string &input, std::vector<uint8_t> &out) {
    // Count leading '1' (zero bytes)
    size_t zeros = 0;
    while (zeros < input.size() && input[zeros] == '1') ++zeros;

    // Approximate size for base256 buffer
    std::vector<uint8_t> b256((input.size() - zeros) * 733 / 1000 + 1);

    for (size_t i = zeros; i < input.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(input[i]);
        const char *p = std::find(std::begin(kBase58Alphabet), std::end(kBase58Alphabet) - 1, ch);
        if (p == std::end(kBase58Alphabet) - 1) return false; // invalid char
        int carry = static_cast<int>(p - kBase58Alphabet);

        // b256 = b256 * 58 + carry
        for (auto it = b256.rbegin(); it != b256.rend(); ++it) {
            carry += 58 * (*it);
            *it = carry & 0xFF;
            carry >>= 8;
        }
        if (carry != 0) return false;
    }

    // Skip leading zeroes in b256
    auto it = std::find_if(b256.begin(), b256.end(), [](uint8_t v) { return v != 0; });

    std::vector<uint8_t> decoded;
    decoded.reserve(zeros + (b256.end() - it));
    decoded.insert(decoded.end(), zeros, 0x00);
    decoded.insert(decoded.end(), it, b256.end());

    out.swap(decoded);
    return true;
}

// Tron Base58Check address validator:
// - length 34
// - Base58Check decode to 25 bytes
// - payload[0] == 0x41
// - checksum = first4bytes(SHA256(SHA256(payload)))
static bool IsValidTronBase58Address(const std::string &addr) {
    if (addr.size() != 34) return false;

    std::vector<uint8_t> full;
    if (!Base58Decode(addr, full)) return false;
    if (full.size() != 25) return false; // 1-byte prefix + 20-byte hash + 4-byte checksum
    if (full[0] != 0x41) return false;   // prefix check

    const uint8_t *payload = full.data();           // 21 bytes
    const uint8_t *checksum = full.data() + 21;     // last 4 bytes

    std::array<uint8_t, 32> h1{}, h2{};
    picosha2::hash256(payload, payload + 21, h1.begin(), h1.end());
    picosha2::hash256(h1.begin(), h1.end(), h2.begin(), h2.end());
    return std::equal(h2.begin(), h2.begin() + 4, checksum);
}

Mode Mode::matching(const std::string & matchingInput) {
    Mode r;
    const size_t addressLength = 34;
    static const bool kBase58Table[128] = {
        /*   0- 15 */ false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
        /*  16- 31 */ false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
        /*  32- 47 */ false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
        /*  48- 63 */ false,true ,true ,true ,true ,true ,true ,true ,true ,true ,false,false,false,false,false,false,
        /*  64- 79 */ false,true ,true ,true ,true ,true ,true ,true ,true ,false,true ,true ,true ,true ,true ,false,
        /*  80- 95 */ true ,true ,true ,true ,true ,true ,true ,true ,true ,true ,true ,false,false,false,false,false,
        /*  96-111 */ false,true ,true ,true ,true ,true ,true ,true ,true ,true ,true ,true ,false,true ,true ,true ,
        /* 112-127 */ true ,true ,true ,true ,true ,true ,true ,true ,true ,true ,true ,false,false,false,false,false,
    };
    std::vector<std::string> matchingList;

    auto trim = [](std::string &value) {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
            value.erase(value.begin());
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
            value.pop_back();
        }
    };

    // Fast character filter + strict Base58Check + prefix checksum validation
    auto isBase58Address = [&](const std::string &value) {
        if (value.size() != addressLength) return false;
        for (const char c : value) {
            if (static_cast<unsigned char>(c) >= 128 || !kBase58Table[static_cast<unsigned char>(c)]) {
                return false;
            }
        }
        return IsValidTronBase58Address(value);
    };

    auto pushMatching = [&](std::string value) {
        trim(value);
        if (value.empty()) return;
        if (isBase58Address(value)) {
            matchingList.push_back(value);
        } else {
            throw std::runtime_error("invalid address: " + value);
        }
    };

    // Create a local copy to trim
    std::string input = matchingInput;
    trim(input);
    if (input.size() == addressLength && input[0] == 'T') {
        pushMatching(input);
    } else {
        std::ifstream file(input);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                pushMatching(line);
            }
        } else {
            std::cout << "error: Failed to open matching file. :<" << std::endl;
        }
    }

    if (matchingList.size() > 0) {
        r.matchingCount = matchingList.size();
        for (size_t j = 0; j < matchingList.size(); j += 1) {
            const std::string &matchingItem = matchingList[j];
            for (size_t i = 0; i < addressLength; ++i) {
                const unsigned char value = static_cast<unsigned char>(matchingItem[i]);
                const unsigned long valHi = (value >> 4) & 0x0F;
                const unsigned long valLo = value & 0x0F;
                const int maskHi = 0xF << 4;
                const int maskLo = 0xF;
                r.data1.push_back(maskHi | maskLo);
                r.data2.push_back((valHi << 4) | valLo);
            }
        }
    }

    return r;
}
