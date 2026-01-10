/**
 * Performance benchmarks for core algorithms
 * Uses Google Benchmark framework
 */

#include <benchmark/benchmark.h>
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>

// Include headers for functions to benchmark
#include "picosha2.h"
#include "Mode.hpp"
#include "precomp.hpp"

/**
 * @brief RAII file guard for automatic cleanup
 */
class FileGuard {
public:
    explicit FileGuard(const char* path) : m_path(path) {}
    ~FileGuard() {
        if (m_path) {
            std::remove(m_path);
        }
    }
    // Prevent copying
    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;
private:
    const char* m_path;
};

/**
 * Benchmark 1: SHA256 hashing performance
 */
static void BM_SHA256_Hash(benchmark::State& state) {
    std::string input = "This is a test string for SHA256 hashing performance benchmark";

    for (auto _ : state) {
        std::string hash = picosha2::hash256_hex_string(input);
        benchmark::DoNotOptimize(hash);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SHA256_Hash);

/**
 * Benchmark 2: SHA256 with varying input sizes
 */
static void BM_SHA256_VaryingSize(benchmark::State& state) {
    size_t size = state.range(0);
    std::string input(size, 'x');

    for (auto _ : state) {
        std::string hash = picosha2::hash256_hex_string(input);
        benchmark::DoNotOptimize(hash);
    }

    state.SetBytesProcessed(state.iterations() * size);
}
BENCHMARK(BM_SHA256_VaryingSize)->Range(8, 8<<10);

/**
 * Benchmark 3: Precomputed point access
 */
static void BM_PrecompPointAccess(benchmark::State& state) {
    int index = 0;

    for (auto _ : state) {
        point p = g_precomp[index];
        benchmark::DoNotOptimize(p);
        index = (index + 1) % 8160;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PrecompPointAccess);

/**
 * Benchmark 4: Sequential precomp access
 */
static void BM_PrecompSequentialAccess(benchmark::State& state) {
    size_t count = state.range(0);

    for (auto _ : state) {
        for (size_t i = 0; i < count && i < 8160; i++) {
            point p = g_precomp[i];
            benchmark::DoNotOptimize(p);
        }
    }

    state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_PrecompSequentialAccess)->Range(8, 1024);

/**
 * Benchmark 5: Base58 encoding (helper function)
 *
 * Note: This helper function duplicates base58 encoding logic that exists in the main codebase.
 * The duplication is intentional for benchmark isolation - we want to measure the encoding
 * performance without dependencies on the Dispatcher or other components that might introduce
 * variability in the benchmark results.
 */
static std::string base58Encode(const std::vector<uint8_t> &data)
{
    static const uint8_t base58Alphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    std::vector<uint8_t> digits((data.size() * 138 / 100) + 1);
    size_t digitslen = 1;
    for (size_t i = 0; i < data.size(); i++)
    {
        uint32_t carry = static_cast<uint32_t>(data[i]);
        for (size_t j = 0; j < digitslen; j++)
        {
            carry = carry + static_cast<uint32_t>(digits[j] << 8);
            digits[j] = static_cast<uint8_t>(carry % 58);
            carry /= 58;
        }
        for (; carry; carry /= 58)
        {
            digits[digitslen++] = static_cast<uint8_t>(carry % 58);
        }
    }
    std::string result;
    for (size_t i = 0; i < (data.size() - 1) && !data[i]; i++)
    {
        result.push_back(base58Alphabet[0]);
    }
    for (size_t i = 0; i < digitslen; i++)
    {
        result.push_back(base58Alphabet[digits[digitslen - 1 - i]]);
    }
    return result;
}

static void BM_Base58Encode(benchmark::State& state) {
    size_t size = state.range(0);
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; i++) {
        data[i] = static_cast<uint8_t>(i % 256);
    }

    for (auto _ : state) {
        std::string encoded = base58Encode(data);
        benchmark::DoNotOptimize(encoded);
    }

    state.SetBytesProcessed(state.iterations() * size);
}
BENCHMARK(BM_Base58Encode)->Range(20, 128);

/**
 * Benchmark 6: Vector allocation and initialization
 */
static void BM_VectorAllocation(benchmark::State& state) {
    size_t size = state.range(0);

    for (auto _ : state) {
        std::vector<uint8_t> vec(size);
        benchmark::DoNotOptimize(vec.data());
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VectorAllocation)->Range(8, 8<<10);

/**
 * Benchmark 7: String concatenation
 */
static void BM_StringConcatenation(benchmark::State& state) {
    std::string base = "0123456789abcdef";

    for (auto _ : state) {
        std::string result;
        for (int i = 0; i < state.range(0); i++) {
            result += base;
        }
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StringConcatenation)->Range(1, 128);

/**
 * Benchmark 8: Hex to byte conversion
 */
static void BM_HexToByte(benchmark::State& state) {
    std::string hex = "0123456789abcdef0123456789abcdef";

    for (auto _ : state) {
        std::string result;
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byte = hex.substr(i, 2);
            long val = strtol(byte.c_str(), nullptr, 16);
            result.push_back(static_cast<char>(val));
        }
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HexToByte);

/**
 * Benchmark 9: Random number generation simulation
 */
static void BM_SeedUpdate(benchmark::State& state) {
    typedef unsigned long long cl_ulong;
    struct cl_ulong4 { cl_ulong s[4]; };

    cl_ulong4 seed;
    seed.s[0] = 0x1234567890abcdef;
    seed.s[1] = 0xfedcba0987654321;
    seed.s[2] = 0x1111111111111111;
    seed.s[3] = 0x2222222222222222;

    cl_ulong round = 0;

    for (auto _ : state) {
        // Simulate seed update logic
        cl_ulong carry = 0;
        cl_ulong new_s0 = seed.s[0] + round;
        carry = (new_s0 < seed.s[0]) ? 1 : 0;

        cl_ulong temp1 = seed.s[1] + carry;
        carry = (temp1 < seed.s[1]) ? 1 : 0;
        cl_ulong new_s1 = temp1 + (round >> 32);

        benchmark::DoNotOptimize(new_s0);
        benchmark::DoNotOptimize(new_s1);

        round++;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SeedUpdate);

/**
 * Benchmark 10: Memory copy operations
 */
static void BM_MemoryCopy(benchmark::State& state) {
    size_t size = state.range(0);
    std::vector<uint8_t> src(size, 0x42);
    std::vector<uint8_t> dst(size);

    for (auto _ : state) {
        std::copy(src.begin(), src.end(), dst.begin());
        benchmark::DoNotOptimize(dst.data());
    }

    state.SetBytesProcessed(state.iterations() * size);
}
BENCHMARK(BM_MemoryCopy)->Range(8, 8<<10);

/**
 * Benchmark 11: Mode file parsing (with temporary file)
 */
static void BM_ModeFileParsing(benchmark::State& state) {
    // Create a temporary test file
    // Note: This uses /tmp which is Unix-specific. For Windows compatibility,
    // consider using a platform-independent temporary directory solution.
    const char* testFile = "/tmp/bench_test_addresses.txt";

    // RAII guard ensures file cleanup even if exception is thrown
    FileGuard guard(testFile);

    std::ofstream file(testFile);
    for (int i = 0; i < state.range(0); i++) {
        file << "TJRabPrwbZy45sbavfcjinPJC18kjpRTv8\n";
    }
    file.close();

    for (auto _ : state) {
        try {
            Mode mode = Mode::matching(testFile);
            benchmark::DoNotOptimize(mode.matchingCount);
        } catch (...) {
            // Ignore errors in benchmark
        }
    }

    state.SetItemsProcessed(state.iterations());
    // File cleanup happens automatically via FileGuard destructor
}
BENCHMARK(BM_ModeFileParsing)->Range(1, 100);

/**
 * Benchmark 12: Byte array comparison
 */
static void BM_ByteArrayComparison(benchmark::State& state) {
    size_t size = state.range(0);
    std::vector<uint8_t> arr1(size, 0x42);
    std::vector<uint8_t> arr2(size, 0x42);

    for (auto _ : state) {
        bool equal = (arr1 == arr2);
        benchmark::DoNotOptimize(equal);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ByteArrayComparison)->Range(8, 1024);

// Run the benchmarks
BENCHMARK_MAIN();
