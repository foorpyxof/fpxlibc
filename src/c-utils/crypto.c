//
//  "crypto.c"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "c-utils/crypto.h"
#include "c-utils/endian.h"
#include "c-utils/format.h"
#include "math/math.h"
#include "mem/mem.h"
#include "networking/netutils.h"

#include <stdio.h>
#include <stdlib.h>

#define SHL(value, bits) (value << bits)
#define SHR(value, bits) (value >> bits)

#define ROL(value, bits, size) ((value << bits) | (value >> (size - bits)))
#define ROR(value, bits, size) ((value >> bits) | (value << (size - bits)))

#define CH(x, y, z) ((x & y) ^ ((~x) & z))
#define MAJ(x, y, z) ((x & y) ^ (x & z) ^ (y & z))

#define SHA256_BSIG0(x, size)                                                  \
  (ROR(x, 2, size) ^ ROR(x, 13, size) ^ ROR(x, 22, size))
#define SHA256_BSIG1(x, size)                                                  \
  (ROR(x, 6, size) ^ ROR(x, 11, size) ^ ROR(x, 25, size))

#define SHA256_SSIG0(x, size) (ROR(x, 7, size) ^ ROR(x, 18, size) ^ SHR(x, 3))
#define SHA256_SSIG1(x, size) (ROR(x, 17, size) ^ ROR(x, 19, size) ^ SHR(x, 10))

void fpx_sha1_init(SHA1_Context *ctx_ptr) {
  ctx_ptr->state[0] = 0x67452301;
  ctx_ptr->state[1] = 0xEFCDAB89;
  ctx_ptr->state[2] = 0x98BADCFE;
  ctx_ptr->state[3] = 0x10325476;
  ctx_ptr->state[4] = 0xC3D2E1F0;

  ctx_ptr->count = 0;
}

static void _sha1_0to19(uint32_t *words, uint32_t *vals, int8_t debug) {
  uint32_t temp;
  uint32_t *a = &vals[0];
  uint32_t *b = &vals[1];
  uint32_t *c = &vals[2];
  uint32_t *d = &vals[3];
  uint32_t *e = &vals[4];

  for (int i = 0; i < 20; i++) {
    if (debug)
      printf("%08x%08x%08x%08x%08x\n", *a, *b, *c, *d, *e);
    if (debug)
      printf("%08x | %08x\n", ((*b & *c) | ((~*b) & *d)), 0x5A827999);
    if (debug)
      printf("W[%d]%08x\n", i, words[i]);
    temp = ROL(*a, 5, 32) + *e + words[i] + ((*b & *c) | ((~*b) & *d)) +
           0x5A827999;
    *e = *d;
    *d = *c;
    *c = ROL(*b, 30, 32);
    *b = *a;
    *a = temp;
  }
}

static void _sha1_20to39(uint32_t *words, uint32_t *vals, int8_t debug) {
  uint32_t temp;
  uint32_t *a = &vals[0];
  uint32_t *b = &vals[1];
  uint32_t *c = &vals[2];
  uint32_t *d = &vals[3];
  uint32_t *e = &vals[4];

  for (int i = 20; i < 40; i++) {
    if (debug)
      printf("%08x%08x%08x%08x%08x\n", *a, *b, *c, *d, *e);
    if (debug)
      printf("%08x | %08x\n", (*b ^ *c ^ *d), 0x6ED9EBA1);
    if (debug)
      printf("W[%d]%08x\n", i, words[i]);
    temp = ROL(*a, 5, 32) + *e + words[i] + (*b ^ *c ^ *d) + 0x6ED9EBA1;
    *e = *d;
    *d = *c;
    *c = ROL(*b, 30, 32);
    *b = *a;
    *a = temp;
  }
}

static void _sha1_40to59(uint32_t *words, uint32_t *vals, int8_t debug) {
  uint32_t temp;
  uint32_t *a = &vals[0];
  uint32_t *b = &vals[1];
  uint32_t *c = &vals[2];
  uint32_t *d = &vals[3];
  uint32_t *e = &vals[4];

  for (int i = 40; i < 60; i++) {
    if (debug)
      printf("%08x%08x%08x%08x%08x\n", *a, *b, *c, *d, *e);
    if (debug)
      printf("%08x | %08x\n", ((*b & *c) | (*b & *d) | (*c & *d)), 0x8F1BBCDC);
    if (debug)
      printf("W[%d]%08x\n", i, words[i]);
    temp = ROL(*a, 5, 32) + *e + words[i] +
           ((*b & *c) | (*b & *d) | (*c & *d)) + 0x8F1BBCDC;
    *e = *d;
    *d = *c;
    *c = ROL(*b, 30, 32);
    *b = *a;
    *a = temp;
  }
}

static void _sha1_60to79(uint32_t *words, uint32_t *vals, int8_t debug) {
  uint32_t temp;
  uint32_t *a = &vals[0];
  uint32_t *b = &vals[1];
  uint32_t *c = &vals[2];
  uint32_t *d = &vals[3];
  uint32_t *e = &vals[4];

  for (int i = 60; i < 80; i++) {
    if (debug)
      printf("%08x%08x%08x%08x%08x\n", *a, *b, *c, *d, *e);
    if (debug)
      printf("%08x | %08x\n", (*b ^ *c ^ *d), 0xCA62C1D6);
    if (debug)
      printf("W[%d]%08x\n", i, words[i]);
    temp = ROL(*a, 5, 32) + *e + words[i] + (*b ^ *c ^ *d) + 0xCA62C1D6;
    *e = *d;
    *d = *c;
    *c = ROL(*b, 30, 32);
    *b = *a;
    *a = temp;
  }
}

void fpx_sha1_transform(SHA1_Context *ctx_ptr, const uint8_t *buffer) {
  uint32_t vals[5];
  uint32_t W[80];

  for (int i = 0; i < 16; i++) {
    W[i] = (buffer[i * 4] << 24) | (buffer[i * 4 + 1] << 16) |
           (buffer[i * 4 + 2] << 8) | (buffer[i * 4 + 3]);
    // fpx_endian_swap(&W[i], 4);
  }

  for (int i = 16; i < 80; i++) {
    W[i] = ROL((W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16]), 1, 32);
  }
  // for (int i = 0; i<80; i++) {
  //   printf("W[%d]: %08x\n", i, W[i]);
  // }

  vals[0] = ctx_ptr->state[0];
  vals[1] = ctx_ptr->state[1];
  vals[2] = ctx_ptr->state[2];
  vals[3] = ctx_ptr->state[3];
  vals[4] = ctx_ptr->state[4];

  _sha1_0to19(W, vals, 0);
  _sha1_20to39(W, vals, 0);
  _sha1_40to59(W, vals, 0);
  _sha1_60to79(W, vals, 0);

  ctx_ptr->state[0] += vals[0];
  ctx_ptr->state[1] += vals[1];
  ctx_ptr->state[2] += vals[2];
  ctx_ptr->state[3] += vals[3];
  ctx_ptr->state[4] += vals[4];
}

void fpx_sha1_update(SHA1_Context *ctx_ptr, const uint8_t *data, size_t len) {
  size_t i, j;

  j = (ctx_ptr->count >> 3) & 63;
  ctx_ptr->count += (uint64_t)len << 3;

  if ((j + len) > 63) {
    fpx_memcpy(&ctx_ptr->buffer[j], data, (i = 64 - j));
    fpx_sha1_transform(ctx_ptr, ctx_ptr->buffer);
    for (; i + 63 < len; i += 64) {
      fpx_sha1_transform(ctx_ptr, &data[i]);
    }
    j = 0;
  } else {
    i = 0;
  }

  fpx_memcpy(&ctx_ptr->buffer[j], &data[i], len - i);
}

void fpx_sha1_final(SHA1_Context *ctx_ptr, uint8_t digest[20]) {
  uint64_t finalcount = (uint64_t)ctx_ptr->count;
  uint8_t c;

  short bigEndianChecker = 1;
  char bigEndian = (!*(char *)&bigEndianChecker) ? 1 : 0;

  if (!bigEndian)
    fpx_endian_swap(&finalcount, sizeof(finalcount));

  c = 0x80;
  fpx_sha1_update(ctx_ptr, &c, 1);
  while ((ctx_ptr->count % 512) != 448) {
    c = 0x00;
    fpx_sha1_update(ctx_ptr, &c, 1);
  }

  fpx_sha1_update(ctx_ptr, (uint8_t *)&finalcount, 8);

  for (int i = 0; i < 20; i++) {
    digest[i] =
        (uint8_t)((ctx_ptr->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
  }
}

void fpx_sha1_digest(const uint8_t *input, size_t lengthBytes, uint8_t *output,
                     uint8_t printable) {
  SHA1_Context ctx;
  uint8_t digest[20];

  fpx_sha1_init(&ctx);
  fpx_sha1_update(&ctx, input, lengthBytes);
  fpx_sha1_final(&ctx, digest);

  if (printable) {
    for (int i = 0; i < 20; i++) {
      sprintf((char *)&output[i * 2], "%02x", digest[i]);
    }
  } else {
    fpx_memcpy(output, digest, 20);
  }
  return;
}

void fpx_sha256_init(SHA256_Context *ctx_ptr) {
  fpx_memset(ctx_ptr, 0, sizeof(SHA256_Context));

  ctx_ptr->state[0] = 0x6a09e667;
  ctx_ptr->state[1] = 0xbb67ae85;
  ctx_ptr->state[2] = 0x3c6ef372;
  ctx_ptr->state[3] = 0xa54ff53a;
  ctx_ptr->state[4] = 0x510e527f;
  ctx_ptr->state[5] = 0x9b05688c;
  ctx_ptr->state[6] = 0x1f83d9ab;
  ctx_ptr->state[7] = 0x5be0cd19;
}

void fpx_sha256_input(SHA256_Context *ctx_ptr, uint8_t *buffer,
                      size_t length_bytes) {
  if (ctx_ptr->buffer != NULL &&
      (ctx_ptr->index + length_bytes + 9) / 64 > (ctx_ptr->index + 9) / 64)
    ctx_ptr->buffer = (uint8_t *)realloc(
        ctx_ptr->buffer, ((ctx_ptr->index + length_bytes + 9) / 64 + 1) * 64);

  else if (ctx_ptr->buffer == NULL)
    ctx_ptr->buffer = (uint8_t *)malloc(((length_bytes + 9) / 64 + 1) * 64);

  fpx_memset(ctx_ptr->buffer + ctx_ptr->index, 0, length_bytes);
  fpx_memcpy(ctx_ptr->buffer, buffer, length_bytes);

  ctx_ptr->index += length_bytes;
}

void fpx_sha256_pad(SHA256_Context *ctx_ptr) {
  uint32_t buffer_length = ((ctx_ptr->index + 9) / 64 + 1) * 64;
  uint64_t length_bits = ctx_ptr->index * 8;

  // pad with 0's
  fpx_memset(ctx_ptr->buffer + ctx_ptr->index, 0,
             buffer_length - ctx_ptr->index);

  // add '1' to the end of message
  ctx_ptr->buffer[ctx_ptr->index] = 0x80;

  // add size in bits to end
  fpx_endian_swap_if_little(&length_bits, 8);
  fpx_memcpy(&ctx_ptr->buffer[buffer_length - 8], &length_bits, 8);
}

void fpx_sha256_transform(SHA256_Context *ctx_ptr, size_t offset) {
  static uint32_t K[64] = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
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

  uint32_t vals[8];
  uint32_t W[64];

  uint32_t temp1, temp2;

  // fpx_memcpy(W, ctx_ptr->buffer, 16 * sizeof(uint32_t));

  uint8_t *offset_buffer = ctx_ptr->buffer + offset;

  for (int i = 0; i < 16; ++i) {
    W[i] = (((uint32_t)offset_buffer[i * 4]) << 24) |
           (((uint32_t)offset_buffer[i * 4 + 1]) << 16) |
           (((uint32_t)offset_buffer[i * 4 + 2]) << 8) |
           (((uint32_t)offset_buffer[i * 4 + 3]));
  }

  for (int i = 16; i < 64; ++i)
    W[i] = SHA256_SSIG1(W[i - 2], 32) + W[i - 7] + SHA256_SSIG0(W[i - 15], 32) +
           W[i - 16];

  for (int i = 0; i < 8; ++i)
    vals[i] = ctx_ptr->state[i];

  for (int i = 0; i < 64; ++i) {
    temp1 = vals[7] + SHA256_BSIG1(vals[4], 32) +
            CH(vals[4], vals[5], vals[6]) + K[i] + W[i];
    temp2 = SHA256_BSIG0(vals[0], 32) + MAJ(vals[0], vals[1], vals[2]);
    vals[7] = vals[6];
    vals[6] = vals[5];
    vals[5] = vals[4];
    vals[4] = vals[3] + temp1;
    vals[3] = vals[2];
    vals[2] = vals[1];
    vals[1] = vals[0];
    vals[0] = temp1 + temp2;
  }

  for (int i = 0; i < 8; ++i) {
    ctx_ptr->state[i] += vals[i];
  }
}

void fpx_sha256_digest(const uint8_t *input, size_t lengthBytes,
                       uint8_t *output, uint8_t printable) {
  SHA256_Context ctx;

  fpx_sha256_init(&ctx);
  fpx_sha256_input(&ctx, (uint8_t *)input, lengthBytes);
  fpx_sha256_pad(&ctx);

  int iters = (ctx.index + 9) / 64 + 1;
  for (int i = 0; i < iters; ++i) {
    fpx_sha256_transform(&ctx, i * 64);
  }

  fpx_sha256_final(&ctx);

  if (printable == 0) {
    fpx_memcpy(output, ctx.state, 32);
    return;
  }

  for (int i = 0; i < 32; ++i) {
    fpx_hexstr(&((uint8_t *)ctx.state)[i], 1, (char *)output + i * 2, 2);
  }
}

void fpx_sha256_final(SHA256_Context *ctx_ptr) {
  if (ctx_ptr->buffer != NULL) {
    free(ctx_ptr->buffer);
  }

  for (int i = 0; i < 8; ++i) {
    fpx_endian_swap_if_little(&ctx_ptr->state[i], 4);
  }
}

char *fpx_base64_encode(const uint8_t *input, int lengthBytes) {
  int bits = lengthBytes * 8;
  int outputLen =
      ((bits / 6) % 4) ? (bits / 6) + (4 - ((bits / 6) % 4)) : (bits / 6);
  uint8_t *uInput = (uint8_t *)input;

  char *output = (char *)malloc(outputLen + 1);
  if (!output)
    return NULL;

  char alphabet[64] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
                       'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                       'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                       'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                       's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2',
                       '3', '4', '5', '6', '7', '8', '9', '+', '/'};

  output[outputLen] = 0;

  int i = 0, j = 0, iter = 0;
  while (lengthBytes > 0) {
    int octet_a = i < lengthBytes + (iter * 3) ? uInput[i++] : 0;
    int octet_b = i < lengthBytes + (iter * 3) ? uInput[i++] : 0;
    int octet_c = i < lengthBytes + (iter * 3) ? uInput[i++] : 0;

    int triple = (octet_a << 16) + (octet_b << 8) + octet_c;
    // 0x3f == 0b00111111
    output[j++] = alphabet[(triple >> 18) & 0x3f];
    output[j++] = alphabet[(triple >> 12) & 0x3f];
    output[j++] = alphabet[(triple >> 6) & 0x3f];
    output[j++] = alphabet[triple & 0x3f];

    lengthBytes -= 3;
    iter++;
  }
  for (int k = 0; k < (3 - lengthBytes % 3) % 3; k++) {
    output[--j] = '=';
  }

  return output;
}

void fpx_hmac(const uint8_t *key, size_t keyLengthBytes, const uint8_t *data,
              size_t dataLength, uint8_t *output, enum HashAlgorithms algo) {
  size_t block_size = 0;
  size_t digest_size = 0;

  void (*hash_function)(const uint8_t *, size_t, uint8_t *, uint8_t) = NULL;

  switch (algo) {
  case SHA1:
    block_size = 512 / 8;  // = 64 Bytes
    digest_size = 160 / 8; // = 20 Bytes
    hash_function = fpx_sha1_digest;

    break;

  case SHA256:
    block_size = 512 / 8;  // = 64 Bytes
    digest_size = 256 / 8; // = 32 Bytes
    hash_function = fpx_sha256_digest;

    break;
  }

  uint8_t *first_input = (uint8_t *)calloc(block_size + dataLength, 1);
  uint8_t *second_input = (uint8_t *)calloc(block_size + digest_size, 1);

  if (keyLengthBytes > block_size)
    hash_function(key, keyLengthBytes, first_input, 0);
  else if (keyLengthBytes < block_size)
    fpx_memcpy(first_input, key, keyLengthBytes);

  fpx_memcpy(second_input, first_input, block_size);

  // first pass:
  // we XOR the key_block with a string of '0x36'
  // then we hash the result with the actual message appended
  // and append that output to the XOR-ed key in 'second_input'

  // XOR:
  for (size_t i = 0; i < block_size; ++i) {
    first_input[i] ^= 0x36;
    second_input[i] ^= 0x5c;
  }

  // HASH:
  fpx_memcpy(first_input + block_size, data, dataLength);
  hash_function(first_input, block_size + dataLength, second_input + block_size,
                0);

  // second pass:
  // we hash the 'second_input'
  hash_function(second_input, block_size + digest_size, output, 0);

  fpx_memset(first_input, 0, block_size + dataLength);
  fpx_memset(second_input, 0, block_size + digest_size);

  free(first_input);
  free(second_input);
}

void fpx_hkdf_extract(const uint8_t *salt, size_t salt_len, const uint8_t *ikm,
                      size_t ikm_len, uint8_t *output,
                      enum HashAlgorithms algo) {
  size_t hash_output_size = 0;
  switch (algo) {
  case SHA1:
    hash_output_size = 20;
    break;

  case SHA256:
    hash_output_size = 32;
    break;
  }

  uint8_t backup_salt[hash_output_size];
  if (salt == NULL || salt_len == 0) {
    fpx_memset(backup_salt, 0, hash_output_size);
    salt = backup_salt;
    salt_len = hash_output_size;
  }

  fpx_hmac(salt, salt_len, ikm, ikm_len, output, algo);
}

int fpx_hkdf_expand(const uint8_t *prk, const uint8_t *info, size_t info_len,
                    uint8_t *output, size_t output_len,
                    enum HashAlgorithms algo) {
  size_t hash_output_size = 0;
  switch (algo) {
  case SHA1:
    hash_output_size = 20;
    break;

  case SHA256:
    hash_output_size = 32;
    break;
  }

  if (output_len > (255 * hash_output_size))
    return -1;

  int N = fpx_ceil((float)output_len / hash_output_size);
  uint8_t T[N + 1][hash_output_size];

  // T(0)
  T[0][0] = 0;

  // T(1)
  {
    uint8_t temp[info_len + 1];

    fpx_memcpy(temp, info, info_len);
    temp[info_len] = 0x01;

    fpx_hmac(prk, hash_output_size, temp, info_len + 1, T[1], algo);
  }

  // T(2) .. T(N)
  for (int i = 2; i < (N + 1); ++i) {
    uint8_t temp[hash_output_size + info_len + 1];

    fpx_memcpy(temp, T[i - 1], hash_output_size);
    fpx_memcpy(temp + hash_output_size, info, info_len);
    temp[hash_output_size + info_len] = (uint8_t)i;

    fpx_hmac(prk, hash_output_size, temp, hash_output_size + info_len + 1, T[i],
             algo);
  }

  fpx_memcpy(output, &T[1], output_len);

  return 0;
}
