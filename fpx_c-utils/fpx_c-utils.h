#ifndef FPX_CUTILS_H
#define FPX_CUTILS_H

////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#ifndef STR_HELPER
  #define STR_HELPER(x) #x
#endif
#ifndef STR
  #define STR(x) STR_HELPER(x)
#endif

#define ROL(v, n) ((v << n) | (v >> (32 - n)))

#include "../fpx_string/fpx_string.h"
#include <sys/types.h>

typedef struct {
  uint32_t state[5];
  uint32_t count;
  uint8_t buffer[64];
} SHA1_Context;

void fpx_endian_swap(void* input, int bytes);

void __fpx_sha1_0to19(uint32_t*, uint32_t*, int8_t);
void __fpx_sha1_20to39(uint32_t*, uint32_t*, int8_t);
void __fpx_sha1_40to59(uint32_t*, uint32_t*, int8_t);
void __fpx_sha1_60to79(uint32_t*, uint32_t*, int8_t);

void fpx_sha1_init(SHA1_Context*);
void fpx_sha1_transform(SHA1_Context*, const uint8_t[64]);
void fpx_sha1_update(SHA1_Context*, const uint8_t*, size_t);
void fpx_sha1_final(SHA1_Context*, uint8_t[20]);
void fpx_sha1_digest(const char* input, size_t lengthBytes, char* output, uint8_t printable);

char* fpx_base64_encode(const char* input, int lengthBytes);

#endif // FPX_CUTILS_H