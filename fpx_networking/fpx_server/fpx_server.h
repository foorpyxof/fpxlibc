#ifndef FPX_SERVER_H
#define FPX_SERVER_H

#include "../../fpx_cpp-utils/fpx_cpp-utils.h"
extern "C"{
#include "../../fpx_string/fpx_string.h"
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

#define MAX_CONNECTIONS 24
#define POLLTIMEOUT 1000
#define BUF_SIZE 1024

#define FPX_INCOMING "MSG:"
#define FPX_DISCONNECT "DISCONNECT"
#define FPX_INIT "NAME:"

namespace fpx {

class TcpServer {
  public:
    static bool Setup(const char*, unsigned short = 8000);
    static void Listen();
    static void ListenSecure(const char*, const char*);
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