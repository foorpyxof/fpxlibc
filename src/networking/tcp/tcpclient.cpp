//
//  "tcpclient.cpp"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "networking/tcp/tcpclient.hpp"
#include "cpp-utils/exceptions.hpp"
extern "C" {
#include "mem/mem.h"
#include "string/string.h"
}

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FPX_ECHO "ECHO:"
#define FPX_PRIVATE "PRIVATE_FOR_"
#define FPX_INCOMING "MSG:"
#define FPX_DISCONNECT "DISCONNECT"
#define FPX_INIT "NAME:"

namespace fpx::ClientProperties {

void *TcpReaderLoop(void *pack) {
  if (!pack)
    return nullptr;
  TcpClient::threaddata_t *package = (TcpClient::threaddata_t *)pack;

  fpx_memset(&package->ReadBuffer, 0, TCP_BUF_SIZE);
  if (package->fn) {
    while (1) {
      if (read(package->Socket, package->ReadBuffer, TCP_BUF_SIZE) == -1) {
        // handle errors
      }
      if (!(short)package->ReadBuffer[0]) {
        // code 1: server closed connection
        package->fn((uint8_t *)"CONN_CLOSE");
        package->Caller->Disconnect();
        pthread_exit(NULL);
      }
      // m_ReadBuffer[strcspn(m_ReadBuffer, "\r\n")] = 0;
      package->fn((uint8_t *)(package->ReadBuffer));
      fpx_memset(package->ReadBuffer, 0, TCP_BUF_SIZE);
    }
  } else {
    while (1) {
      if (read(package->Socket, package->ReadBuffer, TCP_BUF_SIZE) == -1) {
        // handle read error
      }
      if (!(short)package->ReadBuffer[0]) {
        printf("\nServer closed connection.\n");
        pthread_kill(package->WriterThread, SIGINT);
        pthread_exit(NULL);
      }
      if (package->ReadBuffer[fpx_getstringlength(package->ReadBuffer) - 1] !=
          '\n')
        printf("\r%s\n>> ", package->ReadBuffer);
      else
        printf("\r%s>> ", package->ReadBuffer);
      fflush(stdout);
      fpx_memset(package->ReadBuffer, 0, TCP_BUF_SIZE);
    }
  }
}

void *TcpWriterLoop(void *pack) {
  if (!pack)
    return nullptr;
  TcpClient::threaddata_t *package = (TcpClient::threaddata_t *)pack;

  // const char* name = package->WriterName;
  bool preventPrompt = 0;

  fpx_memset(&package->WriteBuffer, 0, TCP_BUF_SIZE);

  while (1) {
    fpx_memset(package->Input, 0, sizeof(package->Input));
    fpx_memset(package->WriteBuffer, 0, sizeof(package->WriteBuffer));
    if (!preventPrompt)
      printf(">> ");
    fflush(stdout);
    preventPrompt = 0;
    if ((fgets(package->Input, sizeof(package->Input), stdin) != NULL &&
         strncmp(package->Input, "quit", 4))) {
      package->Input[strcspn(package->Input, "\r\n")] = 0;
      if (!(*package->Input)) {
        continue;
      }
      if (*package->Input == '!' && *(package->Input + 1) != 0)
        preventPrompt = 1;
      package->Caller->SendMessage(package->Input);
    } else {
      package->Caller->Disconnect();
      printf("Quitting...\n");
      pthread_exit(NULL);
    }
  }
}

} // namespace fpx::ClientProperties

namespace fpx {

TcpClient::TcpClient(const char *ip, short port)
    : m_SrvIp(ip), m_SrvPort(port),
      m_SrvAddress{AF_INET, htons(m_SrvPort), {}, {}} {
  fpx_memset(&m_ThreadData, 0, sizeof(threaddata_t));
  m_ThreadData.Caller = this;
  m_ThreadData.Socket = -1;
  inet_pton(AF_INET, m_SrvIp, &m_SrvAddress.sin_addr);
}

void TcpClient::Connect(Mode mode, void (*readerCallback)(uint8_t *),
                        const char *name) {
  if (mode == Mode::Background && readerCallback == nullptr)
    throw fpx::ArgumentException("No callback function was supplied.");
  if (mode == Mode::Background)
    m_ThreadData.fn = readerCallback;

  m_ThreadData.Socket = socket(AF_INET, SOCK_STREAM, 0);
  if (connect(m_ThreadData.Socket, (struct sockaddr *)&m_SrvAddress,
              sizeof(m_SrvAddress)) == -1) {
    printf("\nInvalid address or address not supported\n");
    return;
  }
  if (!name)
    name = "";
  if (mode == Mode::Interactive) {
    sprintf(m_ThreadData.WriteBuffer, "%s", FPX_INIT);
    strncat(m_ThreadData.WriteBuffer, name, 17);
    write(m_ThreadData.Socket, m_ThreadData.WriteBuffer,
          fpx_getstringlength(m_ThreadData.WriteBuffer));
  }

  strncpy(m_ThreadData.WriterName, name, 16);

  pthread_create(&m_ThreadData.ReaderThread, NULL,
                 ClientProperties::TcpReaderLoop, &m_ThreadData);
  if (mode == Mode::Interactive) {
    pthread_create(&m_ThreadData.WriterThread, NULL,
                   ClientProperties::TcpWriterLoop, &m_ThreadData);
    pthread_join(m_ThreadData.WriterThread, NULL);
    pthread_kill(m_ThreadData.ReaderThread, SIGINT);
  }

  return;
}

bool TcpClient::Disconnect() {
  write(m_ThreadData.Socket, FPX_DISCONNECT,
        fpx_getstringlength(FPX_DISCONNECT));
  return !(close(m_ThreadData.Socket));
}

void TcpClient::SendRaw(const char *msg) {
  const short msgLen = fpx_getstringlength(msg);
  // bool crlfEnding = (fpx_substringindex(msg, "\r\n") == msgLen - 2);
  // if (msg[msgLen-1] == '\n') {
  //   memset((char*)msg+msgLen-(1+cr), 0, 1+cr);
  // }
  snprintf(m_ThreadData.WriteBuffer, TCP_BUF_SIZE, "%s", msg);
  write(m_ThreadData.Socket, m_ThreadData.WriteBuffer, msgLen);
  fpx_memset(m_ThreadData.WriteBuffer, 0, TCP_BUF_SIZE);
}

void TcpClient::SendMessage(const char *msg) {
  char buf[TCP_BUF_SIZE - 16];

  snprintf(buf, sizeof(buf), "%s%s", FPX_INCOMING, msg);
  SendRaw(buf);
}

} // namespace fpx
