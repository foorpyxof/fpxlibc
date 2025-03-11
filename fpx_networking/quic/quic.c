////////////////////////////////////////////////////////////////
//  "quic.c"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../../fpx_types.h"

#include "quic.h"
#include "quic_types.h"

// FPXLIBC LINK-TIME DEPENDENCIES
#include "../../fpx_mem/mem.h"
// END OF FPXLIBC LINK-TIME DEPENDENCIES

#include "../netutils.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <pthread.h>
#include <signal.h>

// length of connection ID in byte IP_DONTFRAGs
#define QUIC_CONNECTION_ID_LENGTH 16

static void* _background_listener(void* arg) {
  fpx_quic_socket_t* quic_sock = (fpx_quic_socket_t*)arg;

  while (1) {
    pthread_mutex_lock(&quic_sock->ListenerMutex);

    // handle new connections
    {

      recv(quic_sock->FileDescriptor, < buffer >, < length >, MSG_PEEK);
    }

    pthread_mutex_unlock(&quic_sock->ListenerMutex);
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

  switch (variable.BYTE >> 6) {
    case 0: // 0b0000 0000
      return 1;

    case 1: // 0b0100 0000
      return 2;

    case 2: // 0b1000 0000
      return 4;

    case 3: // 0b1100 0000
      return 8;

    default: // literally not possible wtf?
      return 0;
  }

  //if (variable.BYTE & 0x80)
    //if (variable.BYTE & 0x40) return 8; // 0b1100 0000
    //else return 4;                      // 0b1000 0000
  //else
    //if (variable.BYTE & 0x40) return 2; // 0b0100 0000
    //else return 1;                      // 0b0000 0000
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

int fpx_quic_socket_init(fpx_quic_socket_t* quic_sock, const char* ip, uint16_t port, uint8_t ip_version) {
  int listen_fd = -1;

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
        perror("fpx_quic_init() -> setsockopt() (Reuse Address and Port)");
        return -1;
      }

      setsockopt_result = setsockopt(
        listen_fd,
        IPPROTO_IP,
        IP_PMTUDISC_DO,
        &optval,
        sizeof(optval)
      );

      if (setsockopt_result == -1) {
        perror("fpx_quic_init() -> setsockopt() (Don't Fragment)");
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

  quic_sock->FileDescriptor = listen_fd;
  quic_sock->IpVersion = ip_version;

  return 0;
}

int fpx_quic_listen(fpx_quic_socket_t* quic_sock, int filedescriptor, uint16_t max_active, uint16_t backlog) {
  // TODO: finish proper allocator implementation so it's usable
  // in fpx_quic
  if (quic_sock->Backlog != NULL)
    free(quic_sock->Backlog);

  quic_sock->Backlog = (fpx_quic_connection_t*)calloc(
    backlog,
    sizeof(fpx_quic_connection_t)
  );

  if (quic_sock->Backlog == NULL) {
    perror("fpx_quic_listen() -> calloc() (backlog)");
    return -1;
  }

  quic_sock->Connections = (fpx_quic_connection_t*)calloc(
    max_active,
    sizeof(fpx_quic_connection_t)
  );

  if (quic_sock->Connections == NULL) {
    perror("fpx_quic_listen() -> calloc() (connections)");
    return -1;
  }

  quic_sock->BacklogLength = backlog;
  quic_sock->MaxConnections = max_active;
  quic_sock->ActiveConnections = 0;

  quic_sock->Thread = pthread_create(&quic_sock->Thread, NULL, _background_listener, quic_sock);
  pthread_mutex_init(&quic_sock->ListenerMutex, NULL);

  return 0;
}

int fpx_quic_stoplisten(fpx_quic_socket_t* quic_sock) {
  // XXX: this could be bad (infinite sleep), try to find a better way
  pthread_mutex_lock(&quic_sock->ListenerMutex);

  pthread_kill(quic_sock->Thread, SIGKILL);
  pthread_join(quic_sock->Thread, NULL);

  quic_sock->Thread = 0;
  pthread_mutex_destroy(&quic_sock->ListenerMutex);

  // it SHOULD never be NULL here
  // but you never know :sob:
  if (quic_sock->Backlog != NULL)
    free(quic_sock->Backlog);

  if (quic_sock->Connections != NULL)
    free(quic_sock->Connections);

  quic_sock->Backlog = NULL;
  quic_sock->Backlog = 0;

  quic_sock->Connections = NULL;
  quic_sock->MaxConnections = 0;
  quic_sock->ActiveConnections = 0;

  if (quic_sock->FileDescriptor) {
    close(quic_sock->FileDescriptor);
    quic_sock->FileDescriptor = -1;
  }

  return 0;
}

fpx_quic_connection_t fpx_quic_accept(fpx_quic_socket_t* quic_sock) {
  pthread_mutex_lock(&quic_sock->ListenerMutex);
  if (quic_sock->Backlog[0].FileDescriptor == 0) {
    // there is no-one in the backlog.
    // let's wait for a connection to appear there
    pthread_cond_wait(&quic_sock->BacklogCondition, &quic_sock->ListenerMutex);
  }

  // there is guaranteed to be a connection now!
  // (assuming that the condition was signaled properly)
  fpx_quic_connection_t to_return;
  fpx_memcpy(&to_return, &quic_sock->Backlog[0], sizeof(fpx_quic_connection_t));

  {
    int i = 0;

    do {
      fpx_memcpy(
        &quic_sock->Backlog[i],
        &quic_sock->Backlog[i + 1],
        sizeof(fpx_quic_connection_t)
      );
    } while (quic_sock->Backlog[++i].FileDescriptor != 0 && i < quic_sock->BacklogLength);
  }

  pthread_mutex_unlock(&quic_sock->ListenerMutex);
  return to_return;
}
