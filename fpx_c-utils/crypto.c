////////////////////////////////////////////////////////////////
//  "crypto.c"                                                //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "crypto.h"
#include "endian.h"
#include "../fpx_mem/mem.h"

#include <stdio.h>
#include <stdlib.h>

void fpx_sha1_init(SHA1_Context* ctx_ptr) {
  ctx_ptr->state[0] = 0x67452301;
  ctx_ptr->state[1] = 0xEFCDAB89;
  ctx_ptr->state[2] = 0x98BADCFE;
  ctx_ptr->state[3] = 0x10325476;
  ctx_ptr->state[4] = 0xC3D2E1F0;

  ctx_ptr->count = 0;
}

void __fpx_sha1_0to19(uint32_t* words, uint32_t* vals, int8_t debug) {
  uint32_t temp;
  uint32_t* a = &vals[0];
  uint32_t* b = &vals[1];
  uint32_t* c = &vals[2];
  uint32_t* d = &vals[3];
  uint32_t* e = &vals[4];
  
  for (int i=0; i<20; i++) {
    if (debug) printf("%08x%08x%08x%08x%08x\n", *a, *b, *c, *d, *e);
    if (debug) printf("%08x | %08x\n", ((*b & *c) | ((~*b) & *d)), 0x5A827999);
    if (debug) printf("W[%d]%08x\n",i , words[i]);
    temp = ROL(*a, 5) + *e + words[i] + ((*b & *c) | ((~*b) & *d)) + 0x5A827999;
    *e = *d;
    *d = *c;
    *c = ROL(*b, 30);
    *b = *a;
    *a = temp;
  }
}

void __fpx_sha1_20to39(uint32_t* words, uint32_t* vals, int8_t debug) {
  uint32_t temp;
  uint32_t* a = &vals[0];
  uint32_t* b = &vals[1];
  uint32_t* c = &vals[2];
  uint32_t* d = &vals[3];
  uint32_t* e = &vals[4];
  
  for (int i=20; i<40; i++) {
    if (debug) printf("%08x%08x%08x%08x%08x\n", *a, *b, *c, *d, *e);
    if (debug) printf("%08x | %08x\n", (*b ^ *c ^ *d), 0x6ED9EBA1);
    if (debug) printf("W[%d]%08x\n",i , words[i]);
    temp = ROL(*a, 5) + *e + words[i] + (*b ^ *c ^ *d) + 0x6ED9EBA1;
    *e = *d;
    *d = *c;
    *c = ROL(*b, 30);
    *b = *a;
    *a = temp;
  }
}

void __fpx_sha1_40to59(uint32_t* words, uint32_t* vals, int8_t debug) {
  uint32_t temp;
  uint32_t* a = &vals[0];
  uint32_t* b = &vals[1];
  uint32_t* c = &vals[2];
  uint32_t* d = &vals[3];
  uint32_t* e = &vals[4];
  
  for (int i=40; i<60; i++) {
    if (debug) printf("%08x%08x%08x%08x%08x\n", *a, *b, *c, *d, *e);
    if (debug) printf("%08x | %08x\n", ((*b & *c) | (*b & *d) | (*c & *d)), 0x8F1BBCDC);
    if (debug) printf("W[%d]%08x\n",i , words[i]);
    temp = ROL(*a, 5) + *e + words[i] + ((*b & *c) | (*b & *d) | (*c & *d)) + 0x8F1BBCDC;
    *e = *d;
    *d = *c;
    *c = ROL(*b, 30);
    *b = *a;
    *a = temp;
  }
}

void __fpx_sha1_60to79(uint32_t* words, uint32_t* vals, int8_t debug) {
  uint32_t temp;
  uint32_t* a = &vals[0];
  uint32_t* b = &vals[1];
  uint32_t* c = &vals[2];
  uint32_t* d = &vals[3];
  uint32_t* e = &vals[4];
  
  for (int i=60; i<80; i++) {
    if (debug) printf("%08x%08x%08x%08x%08x\n", *a, *b, *c, *d, *e);
    if (debug) printf("%08x | %08x\n", (*b ^ *c ^ *d), 0xCA62C1D6);
    if (debug) printf("W[%d]%08x\n",i , words[i]);
    temp = ROL(*a, 5) + *e + words[i] + (*b ^ *c ^ *d) + 0xCA62C1D6;
    *e = *d;
    *d = *c;
    *c = ROL(*b, 30);
    *b = *a;
    *a = temp;
  }
}

void fpx_sha1_transform(SHA1_Context* ctx_ptr, const uint8_t* buffer) {
  uint32_t vals[5];
  uint32_t W[80];

  for (int i = 0; i < 16; i++) {
    W[i] = (buffer[i * 4] << 24) | (buffer[i * 4 + 1] << 16) | (buffer[i * 4 + 2] << 8) | (buffer[i * 4 + 3]);
    // fpx_endian_swap(&W[i], 4);
  }

  for (int i = 16; i < 80; i++) {
    W[i] = ROL((W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16]), 1);
  }
  // for (int i = 0; i<80; i++) {
  //   printf("W[%d]: %08x\n", i, W[i]);
  // }

  vals[0] = ctx_ptr->state[0];
  vals[1] = ctx_ptr->state[1];
  vals[2] = ctx_ptr->state[2];
  vals[3] = ctx_ptr->state[3];
  vals[4] = ctx_ptr->state[4];

  __fpx_sha1_0to19(W, vals, 0);
  __fpx_sha1_20to39(W, vals, 0);
  __fpx_sha1_40to59(W, vals, 0);
  __fpx_sha1_60to79(W, vals, 0);

  ctx_ptr->state[0] += vals[0];
  ctx_ptr->state[1] += vals[1];
  ctx_ptr->state[2] += vals[2];
  ctx_ptr->state[3] += vals[3];
  ctx_ptr->state[4] += vals[4];
}

void fpx_sha1_update(SHA1_Context* ctx_ptr, const uint8_t* data, size_t len) {
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

void fpx_sha1_final(SHA1_Context* ctx_ptr, uint8_t digest[20]) {
  uint64_t finalcount = (uint64_t)ctx_ptr->count;
  uint8_t c;

  short bigEndianChecker = 1;
  char bigEndian = (!*(char*)&bigEndianChecker) ? 1 : 0;

  if (!bigEndian) fpx_endian_swap(&finalcount, sizeof(finalcount));

  c = 0x80;
  fpx_sha1_update(ctx_ptr, &c, 1);
  while ((ctx_ptr->count % 512) != 448) {
    c = 0x00;
    fpx_sha1_update(ctx_ptr, &c, 1);
  }

  fpx_sha1_update(ctx_ptr, (uint8_t*)&finalcount, 8);

  for (int i = 0; i < 20; i++) {
    digest[i] = (uint8_t)((ctx_ptr->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
  }
}

void fpx_sha1_digest(const char* input, size_t lengthBytes, char* output, uint8_t printable) {
  SHA1_Context ctx;
  uint8_t digest[20];

  fpx_sha1_init(&ctx);
  fpx_sha1_update(&ctx, (const uint8_t*)input, lengthBytes);
  fpx_sha1_final(&ctx, digest);

  if (printable) {
    for (int i = 0; i < 20; i++) {
      sprintf(&output[i * 2], "%02x", digest[i]);
    }
  } else {
    fpx_memcpy(output, digest, 20);
  }
  return;
}

char* fpx_base64_encode(const char* input, int lengthBytes) {
  int bits = lengthBytes*8;
  int outputLen = ((bits/6)%4) ? (bits/6)+(4-((bits/6)%4)) : (bits/6);
  uint8_t* uInput = (uint8_t*)input;

  char* output = (char*)malloc(outputLen+1);
  if (!output) return NULL;

  char alphabet[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
  };

  output[outputLen] = 0;

  int i=0, j=0, iter=0;
  while (lengthBytes > 0) {
    int octet_a = i < lengthBytes+(iter*3) ? uInput[i++] : 0;
    int octet_b = i < lengthBytes+(iter*3) ? uInput[i++] : 0;
    int octet_c = i < lengthBytes+(iter*3) ? uInput[i++] : 0;

    int triple = (octet_a << 16) + (octet_b << 8) + octet_c;
    output[j++] = alphabet[(triple >> 18) & 0b00111111];
    output[j++] = alphabet[(triple >> 12) & 0b00111111];
    output[j++] = alphabet[(triple >> 6) & 0b00111111];
    output[j++] = alphabet[triple & 0b00111111];

    lengthBytes -= 3;
    iter++;
  }
  for (int k = 0; k < (3 - lengthBytes % 3) % 3; k++) {
    output[--j] = '=';
  }

  return output;
}
