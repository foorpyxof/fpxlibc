//
//  "math.c"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

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

unsigned int fpx_abs(int input) { return (input < 0) ? (0 - input) : input; }

float fpx_ceil(float input) {
  int int_version = (int)input;
  if (input == (float)int_version) {
    // number is already a round integer, no need to ceil
    return input;
  }
  return int_version + 1;
}
