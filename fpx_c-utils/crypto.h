#ifndef FPX_CRYPTO_H
#define FPX_CRYPTO_H

////////////////////////////////////////////////////////////////
//  "crypto.h"                                                //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../fpx_types.h"

typedef struct {
  uint32_t state[5];
  uint32_t count;
  uint8_t buffer[64];
} SHA1_Context;

typedef struct {
  uint32_t state[8];
  uint32_t index;
  uint8_t* buffer;
} SHA256_Context;

/**
 * Creates a SHA-1 context to work with.
 */
void fpx_sha1_init(SHA1_Context*);

/**
 * SHA-1 transform function.
 */
void fpx_sha1_transform(SHA1_Context*, const uint8_t*);

/**
 * SHA-1 update function.
 */
void fpx_sha1_update(SHA1_Context*, const uint8_t*, size_t);

/**
 * SHA-1 finalize function.
 */
void fpx_sha1_final(SHA1_Context*, uint8_t[20]);

/**
 * SHA-1 digest function. Takes a [const uint8_t*] and length.
 * Outputs result inside of third argument (uint8_t*).
 * Last argument decides on whether or not it's printable (hex string representation)
 * or not (pure bytes).
 */
void fpx_sha1_digest(const uint8_t* input, size_t lengthBytes, uint8_t* output, uint8_t printable);

/**
 * Creates a SHA-256 context to work with.
 */
void fpx_sha256_init(SHA256_Context*);

/**
 * Fills Context->buffer with the input array
 */
void fpx_sha256_input(SHA256_Context*, uint8_t*, size_t);

/**
 * SHA-256 padding function.
 */
void fpx_sha256_pad(SHA256_Context*);

/**
 * SHA-256 transform function.
 */
void fpx_sha256_transform(SHA256_Context*, size_t);

/**
 * SHA-256 finalize function.
 */
void fpx_sha256_final(SHA256_Context*);

void fpx_sha256_digest(const uint8_t* input, size_t lengthBytes, uint8_t* output, uint8_t printable);

/**
 * Returns a (!HEAP ALLOCATED!) base64 string based on the input.
 * TODO:
 * make it instead output in an output buffer;
 * return 0 on success, !0 on failure (out_buffer too small, etc.)
 */
char* fpx_base64_encode(const uint8_t* input, int lengthBytes);

enum HashAlgorithms {
  SHA1 = 0,
  SHA256 = 1,
};

void fpx_hmac(const uint8_t* key, size_t keyLength, const uint8_t* data, size_t dataLength, uint8_t* output,
  enum HashAlgorithms algo);

void fpx_hkdf_extract(const uint8_t* salt, size_t salt_len, const uint8_t* ikm, size_t ikm_len, uint8_t output[], enum HashAlgorithms algo);

int fpx_hkdf_expand(const uint8_t prk[], const uint8_t* info, size_t info_len, uint8_t* output, size_t output_len, enum HashAlgorithms algo);

#endif  // FPX_CRYPTO_H
