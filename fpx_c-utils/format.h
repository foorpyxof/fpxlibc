#ifndef FPX_FORMAT_H
#define FPX_FORMAT_H

////////////////////////////////////////////////////////////////
//  "format.h"                                                //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../fpx_types.h"

/**
 *  Converts a given input string to an integer
 *
 *  Returns the integer
 */
int fpx_strint(const char* input);

/**
 *  Formats a given input integer to a character buffer
 *
 *  Returns the integer that was formatted into the string
 */
int fpx_intstr(int input, char* output);

/**
 *  Formats the value of a passed pointer into a hex-string
 *  Fails when:
 *    - Output buffer not long enough
 *
 *  Null-terminates the string if possible, otherwise leaves it as is
 *
 *  Returns the output-buffer pointer on success, NULL on failure
 */
void* fpx_hexstr(void* input, size_t inputsize, char* output, size_t buflen);

#endif  // FPX_FORMAT_H
