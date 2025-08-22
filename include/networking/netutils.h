#ifndef FPX_NETUTILS_H
#define FPX_NETUTILS_H

//
//  "netutils.h"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "c-utils/endian.h"

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>

#include <psdk_inc/_socket_types.h>
#include <winsock.h>
#include <ws2tcpip.h>

#define SOCKET_TYPE SOCKET
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>

#define SOCKET_TYPE int
#define INVALID_SOCKET -1
#endif

#ifndef STR_HELPER
#define STR_HELPER(x) #x
#endif
#ifndef STR
#define STR(x) STR_HELPER(x)
#endif

enum byte_order { HOST = 0x00, NETWORK = 0x01 };

void fpx_endian_swap_if_host(void* address, size_t bytes) {
  uint16_t endian_checker = 0xff00;
  if (*((uint8_t*)&endian_checker) == 0x00)  // the system is little_endian;
    fpx_endian_swap(address, bytes);
}

void fpx_endian_swap_if_network(void* address, size_t bytes) {
  uint16_t endian_checker = 0xff00;
  if (*((uint8_t*)&endian_checker) == 0xff)  // the system is big_endian;
    fpx_endian_swap(address, bytes);
}

#endif  // FPX_NETUTILS
