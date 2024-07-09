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
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <signal.h>

namespace fpx::Server {

#define FPX_HTTP_GET      0b000000001
#define FPX_HTTP_HEAD     0b000000010
#define FPX_HTTP_POST     0b000000100
#define FPX_HTTP_PUT      0b000001000
#define FPX_HTTP_DELETE   0b000010000
#define FPX_HTTP_CONNECT  0b000100000
#define FPX_HTTP_OPTIONS  0b001000000
#define FPX_HTTP_TRACE    0b010000000
#define FPX_HTTP_PATCH    0b100000000

enum HttpMethod {
  NONE =    0b000000000,
  GET =     0b000000001,
  HEAD =    0b000000010,
  POST =    0b000000100,
  PUT =     0b000001000,
  DELETE =  0b000010000,
  CONNECT = 0b000100000,
  OPTIONS = 0b001000000,
  TRACE =   0b010000000,
  PATCH =   0b100000000,
};

typedef struct {
  int ClientFD;
  struct sockaddr ClientDetails;
  pthread_t Thread;
  pthread_mutex_t TalkingStick;
  pthread_cond_t Condition;
} threadpackage_t;

typedef struct {
  HttpMethod Method;
  char URI[256];
  char Version[16];
  short BodySize;
} http_request_t;

typedef struct {
  char Version[16], Code[4];
  char* Status;
  char* Headers;
  char* Body;
} http_response_t;

typedef bool (*http_handler_method)(int, http_request_t*);
typedef Server::http_response_t (*http_callback_t)(Server::http_request_t*);

typedef struct {
  char URI[256];
  http_callback_t Callback;
  short AllowedMethods;
} http_endpoint_t;

typedef struct : threadpackage_t {
  http_endpoint_t* Endpoints;
} http_threadpackage_t;

// Takes a pointer to an fpx::acceptargs_t object.
void* TcpAcceptLoop(void* arguments);

// Takes a pointer to an fpx::threadpackage_t object.
void* HttpProcessingThread(void* threadpack);


typedef struct {
  HttpMethod Name;
  http_handler_method Handler;
} method_mapper_t;

HttpMethod ParseMethod(const char* method);

bool HandleGet(int socket, http_request_t* request);
bool HandleHead(int socket, http_request_t* request);
bool HandlePost(int socket, http_request_t* request);
bool HandlePut(int socket, http_request_t* request);
bool HandleDelete(int socket, http_request_t* request);
bool HandleConnect(int socket, http_request_t* request);
bool HandleOptions(int socket, http_request_t* request);
bool HandleTrace(int socket, http_request_t* request);
bool HandlePatch(int socket, http_request_t* request);
bool HandleUnknown(int socket, http_request_t* request);

}

#define FPX_MAX_CONNECTIONS 32

namespace fpx {


#define FPX_BUF_SIZE 1024

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
    virtual void Close();
  public:
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

typedef struct {
  short* ClientCountPtr;
  int* ListenSocketPtr;
  struct sockaddr* ClientAddressBlockPtr;
  socklen_t* ClientAddressSizePtr;
  pollfd* ConnectedSockets;
  TcpServer::ClientData* ConnectedClients;
} tcp_acceptargs_t;

#define FPX_HTTP_ENDPOINTS 64

#define FPX_HTTP_READ_BUF 2048
#define FPX_HTTP_WRITE_BUF 4096

#define FPX_HTTP_DEFAULTPORT 8080

#define FPX_HTTP_THREADS 2
#define FPX_WEBSOCKETS_THREADS 2

class HttpServer : public TcpServer {
  public:
    HttpServer(const char* ip, unsigned short port = FPX_HTTP_DEFAULTPORT);
  public:
    enum class ServerType {
      Http,
      WebSockets,
      Both
    };
    typedef struct {
      const char* Name;
      const char* Value;
    } http_header_t;
  public:
    void CreateEndpoint(const char* uri, short methods, Server::http_callback_t endpointCallback);
    void Listen(ServerType);
    void ListenSecure(ServerType, const char*, const char*);
    void Close();
  
  private:
    short m_EndpointCount;
    Server::http_endpoint_t* m_Endpoints;

    Server::http_threadpackage_t* m_RequestHandlers;
    // Server::threadpackage_t* m_WebsocketThreads;
};


}

#endif // FPX_SERVER_H