////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "endian.h"

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
