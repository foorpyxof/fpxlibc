#ifndef FPX_NETUTILS_H
#define FPX_NETUTILS_H

//
//  "netutils.h"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "../c-utils/endian.h"

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

#endif // FPX_NETUTILS
