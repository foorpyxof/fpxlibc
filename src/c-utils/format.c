//
//  "format.c"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "c-utils/format.h"
#include "c-utils/endian.h"
#include "math/math.h"

#ifndef __FPXLIBC_ASM
int fpx_strint(const char *input) {
  // state checkers
  unsigned char first = 1;
  unsigned char positive = 1;

  int retval = 0;

  while ((*input >= '0' && *input <= '9') || (first && (*input == '-'))) {
    retval *= 10;
    first = 0;

    if (*input == '-') {
      positive = 0;
      ++input;
      continue;
    }

    retval += (*input - '0');
    ++input;
  }

  if (!positive)
    retval *= -1;

  return retval;
}
#endif // __FPXLIBC_ASM

int fpx_intstr(int input, char *output) {
  uint8_t digits = 0;
  int input_clone = input;

  // get amount of digits
  for (; input_clone || !digits; input_clone /= 10) {
    ++digits;
  }

  if (input < 0) {
    *(output++) = '-';
    input = (int)fpx_abs(input);
  }

  for (; digits > 0; --digits) {
    *(output++) = ((input / (fpx_pow(10, digits - 1))) % 10) + '0';
  }

  return input;
}

void *fpx_hexstr(void *input, size_t bytes, char *output, size_t buflen) {
  // output buffer too small;
  // we return NULL to indicate error
  if ((buflen < (bytes * 2)) || buflen == 0)
    return NULL;

  uint8_t terminate = 0;
  if (buflen > (bytes * 2))
    terminate = 1;

  char hex_alphabet[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

  size_t outputindex = 0;

  // for(int i = 0; i < bytes; ++i) {
  //   uint8_t byte = ((uint8_t*)input)[i];

  //  output[outputindex++] = hex_alphabet[byte / 16];
  //  output[outputindex++] = hex_alphabet[byte % 16];
  //}
  // TODO: something something endianness (check if big endian needs to be
  // different)

  for (int i = bytes - 1; i > -1; --i) {
    uint8_t byte = ((uint8_t *)input)[i];

    // if (!started) {
    // if (byte / 16) { output[outputindex++] = hex_alphabet[byte / 16]; started
    // = 1; } if (byte % 16) { output[outputindex++] = hex_alphabet[byte % 16];
    // started = 1; }
    //} else {
    output[outputindex++] = hex_alphabet[byte / 16];
    output[outputindex++] = hex_alphabet[byte % 16];
    //}
  }

  // this should literally never happen
  if (!output[0]) {
    output[0] = '0';
    output[1] = '0';

    outputindex += 2;
  }

  if (terminate)
    output[outputindex] = 0;

  return output;
}
