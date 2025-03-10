////////////////////////////////////////////////////////////////
//  "quic.c"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../../fpx_types.h"
#include "quic.h"
#include "quic_types.h"

#include <stdio.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <pthread.h>
#include <signal.h>

static void* _background_listener(void* arg) {
  fpx_quic_socket_t* quicsock = (fpx_quic_socket_t*)arg;

  while (1) {
    pthread_mutex_lock(&quicsock->ThreadMutex);

    // handle new connections

    pthread_mutex_unlock(&quicsock->ThreadMutex);
  }

  return NULL;
}

// static int _quic_can_send(fpx_quic_stream_t* stream) {
//   // code that checks whether or not the stream is able to have data sent over it
//   // (a.k.a. is flow control credit reserved?)
// }

static uint8_t _quic_get_varlen(fpx_quic_varlen_t variable) {
  // to make it a bit easier to handle, we're gonna only grab the
  // first byte from the variable-length integer, since we only need
  // the two most significant bits anyway

  if (variable.BYTE & 0x80)
    if (variable.BYTE & 0x40) return 8; // 0b1100 0000
    else return 4;                      // 0b1000 0000
  else
    if (variable.BYTE & 0x40) return 2; // 0b0100 0000
    else return 1;                      // 0b0000 0000
}

static int _quic_set_varlen(fpx_quic_varlen_t* variablePTR, uint64_t value) {
  if (value < 0x3F)
    variablePTR->BYTE = value;

  else if (value < 0x3FFF)
    variablePTR->WORD = (0x4000 + value);

  else if (value < 0x3FFFFFFF)
    variablePTR->DWORD = (0x80000000 + value);

  else if (value < 0x3FFFFFFFFFFFFFFF)
    variablePTR->QWORD = (0xC000000000000000 + value);

  else return -1; // number larger than ((2^62) - 1),
                  // and thus invalid for varlen integer encoding

  return 0;
}

int fpx_quic_socket_init(const char* ip, uint16_t port, uint8_t ip_version) {
  int listen_fd;

  if (ip_version == 4) {

    listen_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_fd == -1) {
      perror("fpx_quic_init() -> socket()");
      return -1;
    }

    {
      int optval = 1;
      int setsockopt_result = setsockopt(
        listen_fd,
        SOL_SOCKET,
        SO_REUSEADDR | SO_REUSEPORT,
        (void*)&optval,
        sizeof(optval)
      );

      if (setsockopt_result == -1) {
        perror("fpx_quic_init() -> setsockopt()");
        return -1;
      }
    }

    struct sockaddr_in address = { 0 };
    socklen_t sock_length;

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_aton(ip, &address.sin_addr);

    int bind_result = bind(listen_fd, (struct sockaddr*)&address, sizeof(address));
    if (bind_result == -1) {
      perror("fpx_quic_init() -> bind()");
      return -1;
    }

  } else {
    printf("IP version number \"%hhu\" not supported.", ip_version);
    return -1;
  }

  return listen_fd;
}

int fpx_quic_listen(fpx_quic_socket_t* quic_sock, int filedescriptor, uint16_t backlog) {
  // TODO: finish proper allocator implementation so it's usable
  // in fpx_quic
  if (quic_sock->Backlog != NULL)
    free(quic_sock->Backlog);

  quic_sock->Backlog = (fpx_quic_connection_t*)calloc(
    backlog,
    sizeof(fpx_quic_connection_t)
  );

  if (quic_sock->Backlog == NULL) {
    perror("fpx_quic_listen() -> calloc()");
    return -1;
  }

  quic_sock->BacklogLength = backlog;

  quic_sock->Thread = pthread_create(&quic_sock->Thread, NULL, _background_listener, quic_sock);
  pthread_mutex_init(&quic_sock->ThreadMutex, NULL);

  return 0;
}

int fpx_quic_stoplisten(fpx_quic_socket_t* quic_sock) {
  // XXX: this could be bad, try to find a better way
  pthread_mutex_lock(&quic_sock->ThreadMutex);

  pthread_kill(quic_sock->Thread, SIGKILL);
  pthread_join(quic_sock->Thread, NULL);

  quic_sock->Thread = 0;
  pthread_mutex_destroy(&quic_sock->ThreadMutex);

  // it SHOULD never be NULL here
  // but you never know :sob:
  if (quic_sock->Backlog != NULL)
    free(quic_sock->Backlog);

  quic_sock->Backlog = NULL;
  quic_sock->BacklogLength = 0;

  return 0;
}

fpx_quic_connection_t fpx_quic_accept(fpx_quic_socket_t* quic_sock) {
  // TODO: implement :3
}
