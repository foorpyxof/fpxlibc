////////////////////////////////////////////////////////////////
//  "endian.c"                                                //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "endian.h"
#include "../fpx_mem/mem.h"

#include <stdlib.h>

#ifndef __FPXLIBC_ASM
void fpx_endian_swap (void* input, uint8_t bytes) {  
  uint8_t* tempBuf = (uint8_t*)malloc(bytes);
  void* originalInput = input;
  for (int16_t i=bytes-1; i>-1; i--) {
    tempBuf[i] = *((uint8_t*)input);
    input++;
  }
  input = originalInput;
  fpx_memcpy(input, tempBuf, bytes);
  free(tempBuf);
}
#endif // __FPXLIBC_ASM
