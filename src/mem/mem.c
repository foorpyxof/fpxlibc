//
//  "mem.c"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "mem/mem.h"

#ifndef __FPXLIBC_ASM
void* fpx_memcpy(void* dst, const void* src, size_t length) {
  if (!(length && dst && src))
    return dst;
  void* temp = dst;
  uint8_t increment;

  while (length > 0) {
    if (length > 7) {
      // qword
      increment = 8;
      *(uint64_t*)dst = *(uint64_t*)src;
    } else if (length > 3) {
      // dword
      increment = 4;
      *(uint32_t*)dst = *(uint64_t*)src;
    } else if (length > 1) {
      // word
      increment = 2;
      *(uint16_t*)dst = *(uint16_t*)src;
    } else if (length == 1) {
      // byte
      increment = 1;
      *(uint8_t*)dst = *(uint8_t*)src;
    }

    dst = (uint8_t*)dst + increment;
    src = (uint8_t*)src + increment;
    length -= increment;
  }

  return temp;
}
#endif  // __FPXLIBC_ASM

#ifndef __FPXLIBC_ASM
void* fpx_memset(void* dst, uint8_t value, size_t length) {
  if (!(dst && length))
    return dst;
  void* temp = dst;
  uint8_t increment;

  uint64_t realval = value | (value << 8);
  realval |= realval << 16;
  realval |= realval << 32;

  while (length > 0) {

    if (length > 7) {
      *(uint64_t*)dst = realval;
      increment = 8;
    } else if (length > 3) {
      uint32_t* new_dst_value_ptr = (uint32_t*)&realval;
      *(uint32_t*)dst = *new_dst_value_ptr;
      increment = 4;
    } else if (length > 1) {
      uint16_t* new_dst_value_ptr = (uint16_t*)&realval;
      *(uint16_t*)dst = *new_dst_value_ptr;
      increment = 2;
    } else if (length == 1) {
      uint8_t* new_dst_value_ptr = (uint8_t*)&realval;
      *(uint8_t*)dst = *new_dst_value_ptr;
      increment = 1;
    }

    dst = (uint8_t*)dst + increment;
    length -= increment;
  }

  return temp;
}
#endif  // __FPXLIBC_ASM
