////////////////////////////////////////////////////////////////
//  "endian.c"                                                //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "endian.h"

#ifndef __FPXLIBC_ASM
void fpx_endian_swap (void* input, uint8_t bytes) {  
  uint8_t* tempBuf = (uint8_t*)malloc(bytes);
  void* originalInput = input;
  for (int16_t i=bytes-1; i>-1; i--) {
    memcpy(tempBuf+i, input, 1);
    input++;
  }
  input = originalInput;
  memcpy(input, tempBuf, bytes);
  free(tempBuf);
}
#endif // __FPXLIBC_ASM
