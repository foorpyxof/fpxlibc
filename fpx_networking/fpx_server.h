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

#include <signal.h>

#define MAX_CONNECTIONS 32

namespace fpx {

#define TCP_BUF_SIZE 1024
#define TCP_DEFAULTPORT 8080

#define POLLTIMEOUT 1000

#define FPX_ECHO "ECHO:"
#define FPX_PRIVATE "PRIVATE_FOR_"
#define FPX_INCOMING "MSG:"
#define FPX_DISCONNECT "DISCONNECT"
#define FPX_INIT "NAME:"

class TcpServer {
  public:
    /**
     * Takes an IP and a PORT to set up for listening.
     */
    static bool Setup(const char*, unsigned short = TCP_DEFAULTPORT);

    /**
     * Starts listening on the set IP and PORT.
     * This method functions as the main loop for handling messages from clients.
     * 
     * Also creates a thread that continuously listens for NEW connections
     * and adds those to an array.
     */
    static void Listen();

    /**
     * Listen(), but using TLS.
     * 
     * Not finished yet, thus throws fpx::NotImplementedException.
     */
    static void ListenSecure(const char*, const char*);

    /**
     * Close the listening socket.
     */
    static void Close();

  private:
    struct client { 
      char Name[16];
    };

    static unsigned short m_Port;

    static int m_Socket4;
    // int m_Socket6;
    
    static struct pollfd m_Sockets[MAX_CONNECTIONS + 1];
    static struct client m_Clients[MAX_CONNECTIONS];
    
    static struct sockaddr_in m_SocketAddress4;
    // struct sockaddr_in6 m_SocketAddress6;

    static struct sockaddr m_ClientAddress;
    static socklen_t m_ClientAddressSize;

    static short m_ConnectedClients;

    static bool m_IsListening;

    static pthread_t m_AcceptThread;
  private:
    static void* pvt_AcceptLoop(void*);
    static bool pvt_DropConnection();
    static void pvt_HandleDisconnect(pollfd&, short&);

};

}

#endif // FPX_SERVER_H