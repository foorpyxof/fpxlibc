#ifndef FPX_ENDIAN_H
#define FPX_ENDIAN_H

////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Swaps the endianness (or byte order) of the given pointer, based on how
 * many bytes the object occupies in memory (value supplied by the programmer)
 */
extern void fpx_endian_swap(void* input, uint8_t bytes);

#endif // FPX_ENDIAN_H
