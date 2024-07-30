////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "fpx_c-utils.h"
#include "../fpx_string/fpx_string.h"
#include <string.h>

void fpx_endian_swap (void* input, int bytes) {  
  uint8_t* tempBuf = (uint8_t*)malloc(bytes);
  void* originalInput = input;
  for (int i=bytes-1; i>-1; i--) {
    memcpy(tempBuf+i, input, 1);
    input++;
  }
  input = originalInput;
  memcpy(input, tempBuf, bytes);
  free(tempBuf);
}

uint32_t __fpx_sha1_varfunc1(int i, uint32_t B, uint32_t C, uint32_t D) {
  if (i<20) {
    return ((B & C) | ((~B) & D));
  } else if (i<40) {
    return (B ^ C ^ D);
  } else if (i<60) {
    return ((B & C) | (B & D) | (C & D));
  } else {
    return (B ^ C ^ D);
  }
}

uint32_t __fpx_sha1_varfunc2(int i) {
  return (i<20) ? 0x5A827999 : (i<40) ? 0x6ED9EBA1 : (i<60) ? 0x8F1BBCDC : 0xCA62C1D6;
}

void fpx_sha1_init(SHA1_Context* ctx_ptr) {
  ctx_ptr->state[0] = 0x67452301;
  ctx_ptr->state[1] = 0xEFCDAB89;
  ctx_ptr->state[2] = 0x98BADCFE;
  ctx_ptr->state[3] = 0x10325476;
  ctx_ptr->state[4] = 0xC3D2E1F0;

  ctx_ptr->count = 0;
}

void fpx_sha1_0to19(uint32_t* words, uint32_t* vals, int8_t debug) {
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

void fpx_sha1_20to39(uint32_t* words, uint32_t* vals, int8_t debug) {
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

void fpx_sha1_40to59(uint32_t* words, uint32_t* vals, int8_t debug) {
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

void fpx_sha1_60to79(uint32_t* words, uint32_t* vals, int8_t debug) {
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

void fpx_sha1_transform(SHA1_Context* ctx_ptr, const uint8_t buffer[64]) {
  uint32_t vals[5];
  uint32_t temp;
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

  fpx_sha1_0to19(W, vals, 0);
  fpx_sha1_20to39(W, vals, 0);
  fpx_sha1_40to59(W, vals, 0);
  fpx_sha1_60to79(W, vals, 0);

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
    memcpy(&ctx_ptr->buffer[j], data, (i = 64 - j));
    fpx_sha1_transform(ctx_ptr, ctx_ptr->buffer);
    for (; i + 63 < len; i += 64) {
      fpx_sha1_transform(ctx_ptr, &data[i]);
    }
    j = 0;
  } else {
    i = 0;
  }

  memcpy(&ctx_ptr->buffer[j], &data[i], len - i);
}

void fpx_sha1_final(SHA1_Context* ctx_ptr, uint8_t digest[20]) {
  uint64_t finalcount = (uint64_t)ctx_ptr->count;
  uint8_t c;

  short bigEndianChecker = 1;
  char bigEndian = (!*(char*)&bigEndianChecker) ? 1 : 0;

  if (!bigEndian) fpx_endian_swap(&finalcount, 8);

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

void fpx_sha1_digest(const char* input, size_t lengthBytes, char output[40]) {
  SHA1_Context ctx;
  uint8_t digest[20];

  fpx_sha1_init(&ctx);
  fpx_sha1_update(&ctx, (const uint8_t*)input, lengthBytes);
  fpx_sha1_final(&ctx, digest);

  for (int i = 0; i < 20; i++) {
    sprintf(&output[i * 2], "%02x", digest[i]);
  }
/*
  short endianChecker = 1;
  char bigEndian = (!(*(char*)&endianChecker)) ? 1 : 0;

  int inputLenBits = lengthBytes*8;
  long long bigEndianLength = (long long)inputLenBits;

  if (!bigEndian) {
    fpx_endian_swap(&bigEndianLength, 8);
  }
  
  char* paddedInput;
  int paddedInputLenBits = (inputLenBits%512 || !inputLenBits) ? ((inputLenBits/512)+1)*512 : inputLenBits;
  paddedInput = (char*)malloc(paddedInputLenBits/8);
  memcpy(paddedInput, input, inputLenBits/8);
  paddedInput[inputLenBits/8] = 0x80;
  inputLenBits += 8;

  memset(&paddedInput[inputLenBits/8], 0, ((paddedInputLenBits-64)/8)-(inputLenBits/8));
  inputLenBits += ((paddedInputLenBits-64)-inputLenBits);
  memcpy(&paddedInput[inputLenBits/8], &bigEndianLength, 8);

  int chunks = paddedInputLenBits/512;
  long* paddedInputButLong = (long*)paddedInput;
  */
  /* 
  using this for loop to swap the endianness of the 32bit words
  causes "malloc(): corrupted top size"
  at the final print statement in the program...
  */
  // if (!bigEndian) {
  //   for(long i = 0; i<chunks*16; i++) {
  //     fpx_endian_swap(&(paddedInputButLong[i]), 4);
  //   }
  // }
/*
  for (int i=0; i<chunks; i++) {
    fpx_sha1_transform(ctx_ptr, &paddedInput[i]);
  }
  // printf("%x\n", (0x00000001 << 1) | (v >> )));
  free(paddedInput);
  paddedInput = NULL;
  paddedInputButLong = NULL;
  printf("%08x%08x%08x%08x%08x\n", ctx_ptr->state[0], ctx_ptr->state[1], ctx_ptr->state[2], ctx_ptr->state[3], ctx_ptr->state[4]);
  */
  return;
}