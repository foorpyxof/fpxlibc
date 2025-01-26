////////////////////////////////////////////////////////////////
//  "math.c"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "math.h"

#ifndef __FPXLIBC_ASM
int fpx_pow(int base, int power) {
  if (!power)
    return 1;

  if (power > 0)
    power -= 1;
  else
    power += 1;

  int baseclone = base;

  while (power) {
    base *= baseclone;
    power += ((power < 0) ? 1 : -1);
  }

  return base;
}
#endif // _FPXLIBC_ASM

unsigned int fpx_abs(int input) {
  return (input < 0) ? (0 - input) : input;
}
