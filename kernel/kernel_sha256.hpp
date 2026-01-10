#ifndef HPP_KERNEL_SHA256
#define HPP_KERNEL_SHA256

#include <string>

const std::string kernel_sha256 = R"(
#define H0 0x6a09e667
#define H1 0xbb67ae85
#define H2 0x3c6ef372
#define H3 0xa54ff53a
#define H4 0x510e527f
#define H5 0x9b05688c
#define H6 0x1f83d9ab
#define H7 0x5be0cd19

inline uint rotr(uint x, int n) {
  return (x >> n) | (x << (32 - n));
}

inline uint ch(uint x, uint y, uint z) {
  return (x & y) ^ (~x & z);
}

inline uint maj(uint x, uint y, uint z) {
  return (x & y) ^ (x & z) ^ (y & z);
}

inline uint sig0(uint x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
inline uint sig1(uint x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }
inline uint csig0(uint x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
inline uint csig1(uint x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }

__constant uint K[64] =
      {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
       0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
       0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
       0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
       0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
       0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
       0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
       0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
       0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
       0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
       0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

void sha256_21bytes(const uchar *key, uchar *output) {
  uint A, B, C, D, E, F, G, H;
  uint T1, T2;
  uint W[64];

  W[0] = ((uint)key[0] << 24) | ((uint)key[1] << 16) | ((uint)key[2] << 8) | (uint)key[3];
  W[1] = ((uint)key[4] << 24) | ((uint)key[5] << 16) | ((uint)key[6] << 8) | (uint)key[7];
  W[2] = ((uint)key[8] << 24) | ((uint)key[9] << 16) | ((uint)key[10] << 8) | (uint)key[11];
  W[3] = ((uint)key[12] << 24) | ((uint)key[13] << 16) | ((uint)key[14] << 8) | (uint)key[15];
  W[4] = ((uint)key[16] << 24) | ((uint)key[17] << 16) | ((uint)key[18] << 8) | (uint)key[19];
  W[5] = ((uint)key[20] << 24) | 0x800000;
  W[6] = 0; W[7] = 0; W[8] = 0; W[9] = 0;
  W[10] = 0; W[11] = 0; W[12] = 0; W[13] = 0; W[14] = 0;
  W[15] = 168;

  #pragma unroll
  for (int i = 16; i < 64; i++) {
    W[i] = sig1(W[i - 2]) + W[i - 7] + sig0(W[i - 15]) + W[i - 16];
  }

  A = H0; B = H1; C = H2; D = H3; E = H4; F = H5; G = H6; H = H7;

  #pragma unroll
  for (int i = 0; i < 64; i++) {
    T1 = H + csig1(E) + ch(E, F, G) + K[i] + W[i];
    T2 = csig0(A) + maj(A, B, C);
    H = G; G = F; F = E; E = D + T1; D = C; C = B; B = A; A = T1 + T2;
  }

  uint state[8] = {A+H0, B+H1, C+H2, D+H3, E+H4, F+H5, G+H6, H+H7};
  #pragma unroll
  for (int i = 0; i < 8; i++) {
    output[i * 4 + 0] = (state[i] >> 24) & 0xFF;
    output[i * 4 + 1] = (state[i] >> 16) & 0xFF;
    output[i * 4 + 2] = (state[i] >> 8) & 0xFF;
    output[i * 4 + 3] = state[i] & 0xFF;
  }
}

void sha256_32bytes(const uchar *key, uchar *output) {
  uint A, B, C, D, E, F, G, H;
  uint T1, T2;
  uint W[64];

  W[0] = ((uint)key[0] << 24) | ((uint)key[1] << 16) | ((uint)key[2] << 8) | (uint)key[3];
  W[1] = ((uint)key[4] << 24) | ((uint)key[5] << 16) | ((uint)key[6] << 8) | (uint)key[7];
  W[2] = ((uint)key[8] << 24) | ((uint)key[9] << 16) | ((uint)key[10] << 8) | (uint)key[11];
  W[3] = ((uint)key[12] << 24) | ((uint)key[13] << 16) | ((uint)key[14] << 8) | (uint)key[15];
  W[4] = ((uint)key[16] << 24) | ((uint)key[17] << 16) | ((uint)key[18] << 8) | (uint)key[19];
  W[5] = ((uint)key[20] << 24) | ((uint)key[21] << 16) | ((uint)key[22] << 8) | (uint)key[23];
  W[6] = ((uint)key[24] << 24) | ((uint)key[25] << 16) | ((uint)key[26] << 8) | (uint)key[27];
  W[7] = ((uint)key[28] << 24) | ((uint)key[29] << 16) | ((uint)key[30] << 8) | (uint)key[31];
  W[8] = 0x80000000;
  W[9] = 0; W[10] = 0; W[11] = 0; W[12] = 0; W[13] = 0; W[14] = 0;
  W[15] = 256;

  #pragma unroll
  for (int i = 16; i < 64; i++) {
    W[i] = sig1(W[i - 2]) + W[i - 7] + sig0(W[i - 15]) + W[i - 16];
  }

  A = H0; B = H1; C = H2; D = H3; E = H4; F = H5; G = H6; H = H7;

  #pragma unroll
  for (int i = 0; i < 64; i++) {
    T1 = H + csig1(E) + ch(E, F, G) + K[i] + W[i];
    T2 = csig0(A) + maj(A, B, C);
    H = G; G = F; F = E; E = D + T1; D = C; C = B; B = A; A = T1 + T2;
  }

  uint state[8] = {A+H0, B+H1, C+H2, D+H3, E+H4, F+H5, G+H6, H+H7};
  #pragma unroll
  for (int i = 0; i < 8; i++) {
    output[i * 4 + 0] = (state[i] >> 24) & 0xFF;
    output[i * 4 + 1] = (state[i] >> 16) & 0xFF;
    output[i * 4 + 2] = (state[i] >> 8) & 0xFF;
    output[i * 4 + 3] = state[i] & 0xFF;
  }
}

void ethhash_to_tronhash(const uchar *ethhash, uchar *tronhash) {
  uchar hash0[21];
  uchar hash1[32];
  uchar hash2[32];

  hash0[0] = 0x41;
  #pragma unroll
  for (uint i = 0; i < 20; i++) {
    hash0[i + 1] = ethhash[i];
  }

  sha256_21bytes(hash0, hash1);
  sha256_32bytes(hash1, hash2);

  #pragma unroll
  for (uint i = 0; i < 21; i++) {
    tronhash[i] = hash0[i];
  }
  tronhash[21] = hash2[0];
  tronhash[22] = hash2[1];
  tronhash[23] = hash2[2];
  tronhash[24] = hash2[3];
}

__constant char alphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

void base58_encode(const uchar *input, char *output, const int input_len) {
  uint digits[40] = {0};
  int digit_count = 1;

  #pragma unroll
  for (int i = 0; i < 25; i++) {
    uint carry = input[i];
    for (int j = 0; j < digit_count; j++) {
      carry += digits[j] << 8;
      digits[j] = carry % 58;
      carry /= 58;
    }
    while (carry) {
      digits[digit_count++] = carry % 58;
      carry /= 58;
    }
  }

  int output_idx = 0;
  for (int i = digit_count - 1; i >= 0; i--) {
      output[output_idx++] = alphabet[digits[i]];
  }
  output[output_idx] = 0;
}
)";

#endif /* HPP_KERNEL_SHA256 */
