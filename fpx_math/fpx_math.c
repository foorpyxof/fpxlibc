#include "fpx_math.h"

int fpx_math_ceiling(float target) {
  
  int result;

  unsigned long* x32 = &target;
  unsigned char* x8 = (unsigned char*)x32;

  unsigned char isSigned = *x32 & 0x80000000;

  *x32 << 1;
  
  signed char exponent = *x8 - 127;

  *x32 << 8;

  result = *x32;
  result >> (22-exponent);

  result = (isSigned) ? -result + 1 : result + 1;

  return result;

}