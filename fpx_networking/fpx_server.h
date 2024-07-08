#ifndef FPX_SERVER_H
#define FPX_SERVER_H

////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../fpx_cpp-utils/fpx_cpp-utils.h"
extern "C"{
  #include "../fpx_string/fpx_string.h"
}

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <signal.h>

#define MAX_CONNECTIONS 32

namespace fpx {

#define BUF_SIZE 1024
#define FPX_TCP_DEFAULTPORT 9090

#define FPX_HTTP_DEFAULTPORT 8080

#define POLLTIMEOUT 1000

#define FPX_ECHO "ECHO:"
#define FPX_PRIVATE "PRIVATE_FOR_"
#define FPX_INCOMING "MSG:"
#define FPX_DISCONNECT "DISCONNECT"
#define FPX_INIT "NAME:"

// Takes a pointer to an fpx::acceptargs_t object.
void* AcceptLoop(void* arguments);

class TcpServer {
  public:
    /**
     * Takes an IP and a PORT to set up for listening.
     * Default port is defined in FPXTCP_DEFAULTPORT
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
    void Close();
  public:
    struct ClientData { 
      char Name[16];
    };

  protected:
    unsigned short m_Port;
    int m_Socket4;
    // int m_Socket6;
    
    struct pollfd m_Sockets[MAX_CONNECTIONS + 1];
    struct ClientData m_Clients[MAX_CONNECTIONS];
    
    struct sockaddr_in m_SocketAddress4;
    // struct sockaddr_in6 m_SocketAddress6;

    struct sockaddr m_ClientAddress;
    socklen_t m_ClientAddressSize;

    short m_ConnectedClients;

    bool m_IsListening;

    pthread_t m_AcceptThread;
  protected:
    virtual void pvt_HandleDisconnect(pollfd&, short&);

};

class HttpServer : public TcpServer {
  public:
    HttpServer(const char* ip, unsigned short port = FPX_HTTP_DEFAULTPORT);
  public:
    enum class ServerType {
      Http,
      WebSockets,
      Both
    };
  public:
    void Listen();
    void ListenSecure(const char*, const char*);
    void Close();
  
  protected:
    char* m_ResThreadsBusyPtr;
    pthread_t m_ResThreads[4];
  protected:
    void pvt_HandleDisconnect(pollfd&, short&);
};

typedef struct AcceptLoopArgs {
  short* ClientCountPtr;
  int* ListenSocketPtr;
  struct sockaddr* ClientAddressBlockPtr;
  socklen_t* ClientAddressSizePtr;
  pollfd* ConnectedSockets;
  TcpServer::ClientData* ConnectedClients;
} acceptargs_t;

}

#endif // FPX_SERVER_H