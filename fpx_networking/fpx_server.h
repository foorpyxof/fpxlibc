#ifndef FPX_SERVER_H
#define FPX_SERVER_H

////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../fpx_cpp-utils/fpx_cpp-utils.h"
extern "C"{
  #include "../fpx_string/fpx_string.h"
  #include "../fpx_c-utils/fpx_c-utils.h"
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

namespace fpx::ServerProperties {

#define FPX_HTTP_GET      0x1
#define FPX_HTTP_HEAD     0x2
#define FPX_HTTP_POST     0x4
#define FPX_HTTP_PUT      0x8
#define FPX_HTTP_DELETE   0x10
#define FPX_HTTP_CONNECT  0x20
#define FPX_HTTP_OPTIONS  0x40
#define FPX_HTTP_TRACE    0x80
#define FPX_HTTP_PATCH    0x100

#define FPX_BUF_SIZE 1024

typedef struct {
  pthread_t Thread;
  pthread_mutex_t TalkingStick;
  pthread_cond_t Condition;
} threadpackage_t;

// Takes a pointer to an fpx::acceptargs_t object.
void* TcpAcceptLoop(void* arguments);

// Takes a pointer to an fpx::threadpackage_t object.
void* HttpProcessingThread(void* threadpack);

// Takes a pointer to an fpx::websocket_threadpackage_t object.
void* WebSocketThread(void* threadpack);

}

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

typedef struct {
  short* ClientCountPtr;
  int* ListenSocketPtr;
  struct sockaddr* ClientAddressBlockPtr;
  socklen_t* ClientAddressSizePtr;
  pollfd* ConnectedSockets;
  TcpServer::ClientData* ConnectedClients;
} tcp_acceptargs_t;

#define FPX_HTTP_ENDPOINTS 64

#define FPX_HTTP_READ_BUF 4096
#define FPX_HTTP_WRITE_BUF 8192

#define FPX_HTTP_DEFAULTPORT 8080

#define FPX_HTTP_THREADS 2
#define FPX_WS_THREADS 2

#define FPX_HTTPSERVER_VERSION "alpha:aug-2024"

class HttpServer : public TcpServer {
  public:
    HttpServer(const char* ip, unsigned short port = FPX_HTTP_DEFAULTPORT);
    ~HttpServer();
  public:

    enum HttpMethod {
      NONE =    0,
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

    enum HttpServerOption { ManualWebSocket = 0x1 };

    typedef struct {
      HttpMethod Method;
      char URI[256], Version[16], Headers[1024], Body[2800];

      /**
       * Returns the header value matching the given name.
       * 
       * First bool asks if you want it returned in lowercase. This is nice for case-insensitive headers,
       * but not for headers such as Sec-WebSocket-Key (for which you will want to pass 'false').
       * Second bool asks for whether to store the value in the char*, of which the address is passed using the second argument.
       * 
       * Returns:
       *  true ON SUCCESS
       *  false ON FAILURE
       * 
       * ! Also on failure: the char* from the second argument is set to 'nullptr' !
       */
      bool GetHeaderValue(const char*, char**, bool lowercase = true, bool storeValue = true);
    } http_request_t;

    typedef struct Http_Response {
      public:
        char Version[16], Code[4], Status[32];
        char* Headers = nullptr;
        char* Body = nullptr;

        void CopyFrom(struct Http_Response*);

        bool SetCode(const char*);
        bool SetStatus(const char*);
        bool SetHeaders(const char*);
        bool SetBody(const char*);

        int GetHeaderLength();
        int GetBodyLength();

        int AddHeader(const char*, bool = false);

      private:
        int m_HeaderLen = 0, m_BodyLen = 0;
    } http_response_t;

    typedef bool (*http_handler_method)(int, http_request_t*);
    typedef void (*http_callback_t)(http_request_t*, http_response_t*);

    typedef struct { HttpMethod Name; http_handler_method Handler; } method_mapper_t;

    enum class ServerType { HttpOnly, WebSockets, Both };

    typedef struct { const char* Name; const char* Value; } http_header_t;

    typedef struct { char URI[256]; http_callback_t Callback; short AllowedMethods; } http_endpoint_t;

    typedef struct : ServerProperties::threadpackage_t {
      int ClientFD;
      struct sockaddr ClientDetails;
      HttpServer* Caller;
      http_endpoint_t* Endpoints;
    } http_threadpackage_t;

    #define FPX_WS_MAX_CLIENTS 8
    #define FPX_WS_BUFFER 0xffff

    #define WS_FIN    0b10000000
    #define WS_RSV1   0b01000000
    #define WS_RSV2   0b00100000
    #define WS_RSV3   0b00010000

    #define FPX_WS_SEND_CLOSE 0x1
    #define FPX_WS_RECV_CLOSE 0x2

    typedef struct {
      // 0x1: Sent "close"
      // 0x2: Received "close"
      uint8_t Flags;
      bool Fragmented, PendingClose;
      int BytesRead;
      uint8_t ControlReadBuffer[128];
      size_t ReadBufSize;
      uint8_t* ReadBufferPTR;
      time_t LastActiveSeconds;
    } websocket_client_t;
    
    typedef void (*ws_callback_t)(websocket_client_t*, uint16_t, uint64_t, uint32_t, uint8_t*);

    typedef struct {
      public:
        void SetBit(uint8_t bit, bool value = true);
        void SetOpcode(uint8_t opcode);
        bool SetPayload(char* payload, uint16_t len);
      private:
        uint8_t m_MetaByte = 0;
        uint16_t m_PayloadLen = 0;
        char* m_Payload = nullptr;
    } websocket_frame_t;

    typedef struct : ServerProperties::threadpackage_t {
      HttpServer* Caller;
      pollfd PollFDs[FPX_WS_MAX_CLIENTS];
      websocket_client_t Clients[FPX_WS_MAX_CLIENTS];
      ws_callback_t Callback;
      short ClientCount = 0;
      void HandleDisconnect(int clientIndex);
      void SendFrame(int index, websocket_frame_t*);
      void SendClose(int index, uint16_t status, uint8_t* message = nullptr);
    } websocket_threadpackage_t;
    
    http_threadpackage_t* RequestHandlers;
    websocket_threadpackage_t* WebsocketThreads;

    http_response_t Response101;
    http_response_t Response400;
    http_response_t Response404;
    http_response_t Response405;
    http_response_t Response413;
    http_response_t Response426;
    http_response_t Response501;
    http_response_t Response505;

    ServerType Mode;
  public:
    const char* GetDefaultHeaders();
    void SetDefaultHeaders(const char*);
    void CreateEndpoint(const char* uri, short methods, http_callback_t endpointCallback);
    void Listen(ServerType, ws_callback_t = nullptr);
    void ListenSecure(ServerType, const char*, const char*, ws_callback_t = nullptr);
    void Close();

    void SetWebSocketTimeout(uint16_t minutes);
    uint16_t GetWebSocketTimeout();
    
    void SetOption(HttpServerOption, bool = true);
    uint8_t GetOptions();

    static HttpServer::HttpMethod ParseMethod(const char* method);
    
  private:
    char* m_DefaultHeaders;

    uint16_t m_WebSocketTimeout;
    short m_EndpointCount;
    uint8_t m_Options;
    http_endpoint_t* m_Endpoints;
};

}

#endif // FPX_SERVER_H
