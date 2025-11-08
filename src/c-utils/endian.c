//
//  "endian.c"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "c-utils/endian.h"
#include "mem/mem.h"

#include <stdlib.h>

#ifndef __FPXLIBC_ASM
void fpx_endian_swap(void *input, uint8_t bytes) {
  if (NULL == input)
    return;

  uint8_t *tempBuf = (uint8_t *)malloc(bytes);
  void *originalInput = input;
  for (int16_t i = bytes - 1; i > -1; i--) {
    tempBuf[i] = *((uint8_t *)input);
    input = (uint8_t *)input + 1;
  }
  input = originalInput;
  fpx_memcpy(input, tempBuf, bytes);
  free(tempBuf);
}
#endif // __FPXLIBC_ASM

void fpx_endian_swap_if_little(void *address, size_t bytes) {
  uint16_t endian_checker = 0xff00;
  if (*((uint8_t *)&endian_checker) == 0x00) // the system is little_endian;
    fpx_endian_swap(address, bytes);
}

void fpx_endian_swap_if_big(void *address, size_t bytes) {
  uint16_t endian_checker = 0xff00;
  if (*((uint8_t *)&endian_checker) == 0xff) // the system is big_endian;
    fpx_endian_swap(address, bytes);
}
