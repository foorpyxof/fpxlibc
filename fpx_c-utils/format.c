////////////////////////////////////////////////////////////////
//  "format.c"                                                //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "format.h"

#ifndef __FPXLIBC_ASM
int fpx_strint(char* input) {
  // state checkers
  unsigned char first = 1;
  unsigned char positive = 1;

  int retval = 0;

  while (*input >= '0' && *input <= '9' || (first && (*input == '-'))) {
    retval *= 10;
    first = 0;

    if (*input == '-') {
      positive = 0;
      ++input;
      continue;
    }

    if (positive)
      retval += (*input - '0');
    else
      retval -= (*input - '0');
    ++input;
  }

  return retval;
}
#endif // __FPXLIBC_ASM
