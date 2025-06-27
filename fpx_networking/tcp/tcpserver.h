#ifndef FPX_SERVER_TCP_H
#define FPX_SERVER_TCP_H

//
//  "tcpserver.h"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>

#define FPX_MAX_CONNECTIONS 32
#define FPX_TCP_DEFAULTPORT 9090

namespace fpx {

class TcpServer {
  public:
    TcpServer();

  public:
    /**
     * Starts listening on the IP and PORT, given as parameters.
     * Default port is defined in FPX_TCP_DEFAULTPORT.
     * This method functions as the main loop for
     * handling messages from clients.
     *
     * Also creates a thread that continuously listens
     * for NEW connections and adds those to an array.
     */
    virtual void Listen(const char*, unsigned short = FPX_TCP_DEFAULTPORT);

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
    struct ClientData {
        char Name[16];
    };

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

}  // namespace ServerProperties

}  // namespace fpx

#endif  // FPX_SERVER_TCP_H
