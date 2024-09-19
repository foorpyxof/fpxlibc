#ifndef FPX_CLIENT_H
#define FPX_CLIENT_H

////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../fpx_cpp-utils/fpx_cpp-utils.h"
extern "C"{
#include "../fpx_string/fpx_string.h"
}

#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define TCP_BUF_SIZE 1024
#define TCP_DEFAULTPORT 8080

#define FPX_ECHO "ECHO:"
#define FPX_PRIVATE "PRIVATE_FOR_"
#define FPX_INCOMING "MSG:"
#define FPX_DISCONNECT "DISCONNECT"
#define FPX_INIT "NAME:"

namespace fpx::ClientProperties {

typedef void (*fn_ptr)(uint8_t*);

void* TcpReaderLoop(void*);
void* TcpWriterLoop(void*);

}

namespace fpx {

class TcpClient {
  public:
    enum class Mode {
      Interactive,
      Background
    };
  
  public:
    /**
     * Takes an IP and a PORT to set.
     */
    TcpClient(const char* ip, short port = TCP_DEFAULTPORT);

    /**
     * Connect to the fpx::TcpServer instance
     * Takes: 
     * an fpx::TcpClient::Mode, 
     * a callback method for passing strings incoming over the socket, 
     * a username for connecting to an fpx::TcpServer instance.
     * ---
     * Modes:
     * - Interactive - Opens a terminal prompt allowing the user to read and write messages
     * - Background - Messages must be manually sent using SendRaw() and SendMessage()
     * ---
     * The callback is ignored and should be set to NULL when the Mode
     * is interactive.
     */
    void Connect(Mode mode, void (*readerCallback)(uint8_t*), const char* name = nullptr);
    
    /**
     * Gracefully close the socket. Takes no arguments.
     */
    bool Disconnect();

    /**
     * Send a raw string over the socket.
     */
    void SendRaw(const char*);

    /**
     * Invokes 'SendRaw' to send a plain message to
     * an instance of fpx::TcpServer
     */
    void SendMessage(const char*);

    typedef struct {
      TcpClient* Caller;
      ClientProperties::fn_ptr fn;
      pthread_t ReaderThread, WriterThread;    

      int Socket;

      char ReadBuffer[TCP_BUF_SIZE];

      char WriterName[17];
      char WriteBuffer[TCP_BUF_SIZE];
      char Input[TCP_BUF_SIZE-16];
    } threaddata_t;
  
  private:
    threaddata_t m_ThreadData;

    const char* m_SrvIp;
    short m_SrvPort;

    struct sockaddr_in m_SrvAddress;
};

}

#endif // FPX_CLIENT_H