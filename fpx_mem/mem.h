#ifndef FPX_MEM_H
#define FPX_MEM_H

////////////////////////////////////////////////////////////////
//  "mem.h"                                                   //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../fpx_types.h"

extern void* fpx_memcpy(void* dst, const void* src, size_t count);
extern void* fpx_memset(void* target, uint8_t value, size_t count);

#endif // FPX_MEM_H
