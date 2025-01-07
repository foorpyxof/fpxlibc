#ifndef FPX_SERVER_TCP_H
#define FPX_SERVER_TCP_H

////////////////////////////////////////////////////////////////
//  "tcpserver.h"                                             //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../../fpx_cpp-utils/exceptions.h"
extern "C"{
  #include "../../fpx_string/string.h"
}

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <signal.h>
#include <time.h>
#include <limits.h>

#define FPX_BUF_SIZE 1024

#define FPX_MAX_CONNECTIONS 32

namespace fpx {

#define FPX_TCP_DEFAULTPORT 9090

#define FPX_POLLTIMEOUT 1000

#define FPX_ECHO "ECHO:"
#define FPX_PRIVATE "PRIVATE_FOR_"
#define FPX_INCOMING "MSG:"
#define FPX_DISCONNECT "DISCONNECT"
#define FPX_INIT "NAME:"

class TcpServer {
  public:
    /**
     * Takes an IP and a PORT to set up for listening.
     * Default port is defined in FPX_TCP_DEFAULTPORT.
     */
    TcpServer(const char*, unsigned short = FPX_TCP_DEFAULTPORT);
  public:
    /**
     * Starts listening on the set IP and PORT.
     * This method functions as the main loop for
     * handling messages from clients.
     * 
     * Also creates a thread that continuously listens
     * for NEW connections and adds those to an array.
     */
    virtual void Listen();

    /**
     * Listen(), but using TLS.
     * 
     * Not finished yet, thus throws fpx::NotImplementedException.
     */
    virtual void ListenSecure(const char*, const char*);

    /**
     * Close the listening socket.
     */
    virtual void Close();
  public:
    /**
     * Struct containing ClientData.
     * 
     * Attributes:
     * - Name (char[16]) - Holds the username of the connected TcpClient
     */
    struct ClientData { char Name[16]; };

  protected:
    unsigned short m_Port;
    int m_Socket4;
    // int m_Socket6;
    
    struct pollfd m_Sockets[FPX_MAX_CONNECTIONS + 1];
    struct ClientData m_Clients[FPX_MAX_CONNECTIONS];
    
    struct sockaddr_in m_SocketAddress4;
    // struct sockaddr_in6 m_SocketAddress6;

    struct sockaddr m_ClientAddress;
    socklen_t m_ClientAddressSize;

    short m_ConnectedClients;

    bool m_IsListening;

    pthread_t m_AcceptThread;
  protected:
    void pvt_HandleDisconnect(pollfd&, short&);

};

namespace ServerProperties {

/**
 * Struct containing information for general threads used
 * as a abse for thread-data structs in:
 * fpx::HttpServer and its WebSocket capability
 */
typedef struct {
  pthread_t Thread;
  pthread_mutex_t TalkingStick;
  pthread_cond_t Condition;
} threadpackage_t;

/**
 * Struct containing information for the thread that accepts
 * new TCP connections for fpx::TcpServer.
 */
typedef struct {
  short* ClientCountPtr;
  int* ListenSocketPtr;
  struct sockaddr* ClientAddressBlockPtr;
  socklen_t* ClientAddressSizePtr;
  pollfd* ConnectedSockets;
  TcpServer::ClientData* ConnectedClients;
} tcp_acceptargs_t;

}

}

#endif // FPX_SERVER_TCP_H
