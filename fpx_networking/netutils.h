#ifndef FPX_NETUTILS_H
#define FPX_NETUTILS_H

//
//  "netutils.h"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "../fpx_c-utils/endian.h"
#include "../fpx_types.h"

#ifndef STR_HELPER
#define STR_HELPER(x) #x
#endif
#ifndef STR
#define STR(x) STR_HELPER(x)
#endif

enum byte_order { HOST = 0x00, NETWORK = 0x01 };

static void fpx_endian_swap_if_host(void* address, size_t bytes) {
  uint16_t endian_checker = 0xff00;
  if (*((uint8_t*)&endian_checker) == 0x00)  // the system is little_endian;
    fpx_endian_swap(address, bytes);
}

static void fpx_endian_swap_if_network(void* address, size_t bytes) {
  uint16_t endian_checker = 0xff00;
  if (*((uint8_t*)&endian_checker) == 0xff)  // the system is big_endian;
    fpx_endian_swap(address, bytes);
}

#endif  // FPX_NETUTILS
