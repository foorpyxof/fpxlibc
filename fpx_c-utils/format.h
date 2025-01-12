#ifndef FPX_FORMAT_H
#define FPX_FORMAT_H

////////////////////////////////////////////////////////////////
//  "format.h"                                                //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../fpx_int.h"
#include "../fpx_math/arithmetic.h"

/**
 *  Converts a given input string to an integer
 *  - Returns the integer
 */
int fpx_strint(char* input);

/**
 *  Formats a given input integer to a character buffer
 *  - Returns the integer that was formatted into the string
 */
int fpx_intstr(int input, char* output);

#endif  // FPX_FORMAT_H
