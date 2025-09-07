#ifndef FPX_CLIENT_TCP_H
#define FPX_CLIENT_TCP_H

//
//  "tcpclient.h"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "../../fpx_types.h"
#include "../netutils.h"

#include <pthread.h>

#define TCP_BUF_SIZE 1024
#define TCP_DEFAULTPORT 8080

namespace fpx {

namespace ClientProperties {

typedef void (*fn_ptr)(uint8_t *);

/**
 * The background thread that will read incoming TCP messages
 */
void *TcpReaderLoop(void *);

/**
 * The background thread that will send written TCP messages,
 * if background mode is enabled.
 */
void *TcpWriterLoop(void *);

} // namespace ClientProperties

class TcpClient {
public:
  /**
   * Interactive spawns a TCP shell, built to work with fpx::TcpServer.
   * Background allows a callback to send TCP messages.
   */
  enum class Mode { Interactive, Background };

public:
  /**
   * Takes an IP and a PORT to connect to.
   */
  TcpClient(const char *ip, short port = TCP_DEFAULTPORT);

  /**
   * Connect to the fpx::TcpServer instance
   * Takes:
   * an fpx::TcpClient::Mode,
   * a callback method for passing strings incoming over the socket,
   * a username for connecting to an fpx::TcpServer instance.
   * ---
   * Modes:
   * - Interactive - Opens a terminal prompt allowing the user to read and write
   * messages
   * - Background - Messages must be manually sent using SendRaw() and
   * SendMessage()
   * ---
   * The callback is ignored and should be set to NULL when the Mode
   * is interactive.
   */
  void Connect(Mode mode, void (*readerCallback)(uint8_t *),
               const char *name = nullptr);

  /**
   * Gracefully close the socket. Takes no arguments.
   */
  bool Disconnect();

  /**
   * Send a raw string over the socket.
   */
  void SendRaw(const char *);

  /**
   * Invokes 'SendRaw' to send a plain message to
   * an instance of fpx::TcpServer
   */
  void SendMessage(const char *);

  /**
   * A struct containing data about all the current running threads.
   */
  typedef struct {
    TcpClient *Caller;
    ClientProperties::fn_ptr fn;
    pthread_t ReaderThread, WriterThread;

    int Socket;

    char ReadBuffer[TCP_BUF_SIZE];

    char WriterName[17];
    char WriteBuffer[TCP_BUF_SIZE];
    char Input[TCP_BUF_SIZE - 16];
  } threaddata_t;

private:
  threaddata_t m_ThreadData;

  const char *m_SrvIp;
  short m_SrvPort;

  struct sockaddr_in m_SrvAddress;
};

} // namespace fpx

#endif // FPX_CLIENT_TCP_H
