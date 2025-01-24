#ifndef FPX_MATH_H
#define FPX_MATH_H

////////////////////////////////////////////////////////////////
//  "math.h"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

/**
 *  Raises [base] to the power of [power]
 */
int fpx_pow(int base, int power);

/**
 *  Returns the absolute value of the input
 *  (basically the number without the minus sign)
 */
unsigned int fpx_abs(int input);

#endif
