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
#include <sched.h>
#include <signal.h>

#define TCP_BUF_SIZE 1024
#define TCP_DEFAULTPORT 8080

#define FPX_ECHO "ECHO:"
#define FPX_PRIVATE "PRIVATE_FOR_"
#define FPX_INCOMING "MSG:"
#define FPX_DISCONNECT "DISCONNECT"
#define FPX_INIT "NAME:"

namespace fpx {


class TcpClient {
  typedef void (*fn_ptr)(const char*);

  public:
    enum class Mode {
      Interactive,
      Background
    };
  
  public:
    /**
     * Takes an IP and a PORT to set, prior to connecting.
     */
    static bool Setup(const char* ip, short port = TCP_DEFAULTPORT);

    /**
     * Connect to the fpx::TcpServer instance
     * Takes an fpx::TcpClient::Mode, 
     * a callback method for passing strings incoming over the socket, 
     * a username for connecting to an fpx::TcpServer instance.
     * ---
     * Modes:
     * - Interactive - Opens a terminal prompt allowing the user to read and write messages
     * - Background - Messages must be manually sent using SendRaw() and SendMessage()
     * ---
     * The callback is ignored and can be set to NULL when the Mode
     * is interactive.
     */
    static void Connect(Mode mode, void (*readerCallback)(const char*), const char* name = "");
    
    /**
     * Gracefully close the socket. Takes no arguments.
     */
    static bool Disconnect();

    /**
     * Send a raw string over the socket.
     */
    static void SendRaw(const char*);

    /**
     * Invokes 'SendRaw' to send a plain message to
     * an instance of fpx::TcpServer
     */
    static void SendMessage(const char*);
  
  private:
    struct threaddata {
      fn_ptr fn;
    };

    static struct threaddata m_ThreadData;
    static pthread_t m_ReaderThread, m_WriterThread;

    static const char* m_SrvIp;
    static short m_SrvPort;

    static struct sockaddr_in m_SrvAddress;

    static int m_Socket;

    static char m_ReadBuffer[];
    static char m_WriteBuffer[];
    static char m_Input[];
    
  private:
    static void* pvt_ReaderLoop(void*);
    static void* pvt_WriterLoop(void*);


};

}

#endif // FPX_CLIENT_H