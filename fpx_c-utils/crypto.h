#ifndef FPX_CRYPTO_H
#define FPX_CRYPTO_H

////////////////////////////////////////////////////////////////
//  "crypto.h"                                                //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../fpx_types.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef STR_HELPER
#define STR_HELPER(x) #x
#endif
#ifndef STR
#define STR(x) STR_HELPER(x)
#endif

#define ROL(v, n) ((v << n) | (v >> (32 - n)))

typedef struct {
    uint32_t state[5];
    uint32_t count;
    uint8_t buffer[64];
} SHA1_Context;

void __fpx_sha1_0to19(uint32_t*, uint32_t*, int8_t);
void __fpx_sha1_20to39(uint32_t*, uint32_t*, int8_t);
void __fpx_sha1_40to59(uint32_t*, uint32_t*, int8_t);
void __fpx_sha1_60to79(uint32_t*, uint32_t*, int8_t);

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
 * SHA-1 digest function. Takes a [const char*] and length.
 * Outputs result inside of third argument (char*).
 * Last argument decides on whether or not it's printable (hex string representation)
 * or not (pure bytes).
 */
void fpx_sha1_digest(const char* input, size_t lengthBytes, char* output, uint8_t printable);

/**
 * Returns a (!HEAP ALLOCATED!) base64 string based on the input.
 */
char* fpx_base64_encode(const char* input, int lengthBytes);

#endif  // FPX_CRYPTO_H
