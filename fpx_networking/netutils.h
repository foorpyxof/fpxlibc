#ifndef FPX_NETUTILS_H
#define FPX_NETUTILS_H

////////////////////////////////////////////////////////////////
//  "netutils.h"                                              //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../fpx_types.h"

#include "../fpx_c-utils/endian.h"

static void fpx_network_order(void* address, size_t bytes) {
  uint16_t endian_checker = 0xff00;
  if (*((uint8_t*)&endian_checker) == 0x00) // the system is little_endian;
    fpx_endian_swap(address, bytes);
}

static void fpx_host_order(void* address, size_t bytes) {
  uint16_t endian_checker = 0xff00;
  if (*((uint8_t*)&endian_checker) == 0xff) // the system is big_endian;
    fpx_endian_swap(address, bytes);
}

#endif // FPX_NETUTILS
