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

/**
 * Takes a pointer to an fpx::acceptargs_t object.
 */
void* TcpAcceptLoop(void* arguments);

/**
 * Takes a pointer to an fpx::threadpackage_t object.
 */
void* HttpProcessingThread(void* threadpack);

/**
 * Takes a pointer to an fpx::websocket_threadpackage_t object.
 */
void* WebSocketThread(void* threadpack);

/**
 * Takes an instance of fpx::HttpServer as an argument
 * Kills idle keepalive connections after the timeout has passed
 */ 
void* HttpKillerThread(void* hs);

}

#define FPX_HTTP_ENDPOINTS 64

#define FPX_HTTP_READ_BUF 4096
#define FPX_HTTP_WRITE_BUF 8192

#define FPX_HTTP_DEFAULTPORT 8080

#define FPX_HTTP_KEEPALIVE 7

#define FPX_HTTP_MAX_CLIENTS 16
#define FPX_HTTP_THREADS 8
#define FPX_WS_THREADS 2

#define FPX_HTTPSERVER_VERSION "alpha:sep-2024"

class HttpServer : public TcpServer {
  public:
    /**
     * The constructor takes an IP-address to listen on in the format of
     * x.x.x.x, where x is a number from 0 to 255
     * 
     * It also takes a port to listen on that is a number from 0 to 65535
     */
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

    /**
     * This enum currently goes unused.
     */
    enum HttpServerOption { ManualWebSocket = 0x1 };

    /**
     * A struct outlining the general structure of an HTTP request with all of its fields.
     * - URI      (char[256])
     * - Version  (char[16])
     * - Headers  (char[1024])
     * - Body     (char[2800])
     * 
     * Also contains methods used to manipulate or read certain parts of the request.
     */
    typedef struct {
      HttpMethod Method;
      char URI[256], Version[16], Headers[1024];
      char* Body;

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

    /**
     * A struct outlining the general structure of an HTTP response with all of its fields.
     * - Version  (char[16])
     * - Code     (char[4])
     * - Status   (char[32])
     * - Headers  (char*)
     * - Body     (char*)
     * 
     * Also contains methods used to manipulate or read certain parts of the response.
     */
    typedef struct Http_Response {
      public:
        char Version[16], Code[4], Status[32];
        char* Headers = nullptr;
        char* Body = nullptr;

        /**
         * Copies a specified http_response_t opbject into the calling object.
         * Useful for using response presets.
         */
        void CopyFrom(struct Http_Response*);


        /**
         * Sets the status code of the HTTP response.
         * e.g. "200" or "404"
         * ! WARNING: The code must be specified using a string !
         */
        bool SetCode(const char*);

        /**
         * Sets the status text of the HTTP response.
         * e.g. "OK" or "Not Found"
         */
        bool SetStatus(const char*);

        /**
         * Sets the headers for the current response to the given string.
         * Headers are separated by "\r\n". A "\r\n" must also be used at the end
         * of the headers.
         * 
         * e.g. "Host: goodgirl.dev\r\nServer: fpxHTTP\r\nConnection: close\r\n"
         * 
         * ! WARNING: This method will OVERWRITE any currently assigned headers !
         */
        bool SetHeaders(const char*);

        /**
         * Sets the body for the current response to the given string.
         * 
         * ! WARNING: This method will OVERWRITE the current body !
         */
        bool SetBody(const char*);

        /**
         * Returns the current length of all of the entire HTTP header.
         */
        int GetHeaderLength();

        /**
         * Returns the current length of the response body.
         */
        int GetBodyLength();

        /**
         * Adds the specified string to the HTTP header
         * 
         * e.g. "Connection: close"
         */
        int AddHeader(const char*, bool = false);

      public:
        bool Finalized = false;

      private:
        int m_HeaderLen = 0, m_BodyLen = 0;
    } http_response_t;

    /**
     * The HTTP endpoint callback format: function that returns void;
     * takes:
     * - A pointer to an http_request_t object
     * - A pointer to an http_response_t object
     */
    typedef void (*http_callback_t)(http_request_t*, http_response_t*);

    /**
     * HTTP server type:
     * - HTTP only
     * - WebSocket only (not implemented)
     * - Both
     */
    enum class ServerType { HttpOnly, WebSockets, Both };

    /**
     * Internal type for an HTTP endpoint declaration.
     * Contains the respective URI, the http_callback_t function and a
     * short binary switch containing all of the allowed HTTP methods
     * for that endpoint
     */
    typedef struct { char URI[256]; http_callback_t Callback; short AllowedMethods; } http_endpoint_t;

    /**
     * Contains HTTP client data, for the HTTP request processing threads.
     */
    typedef struct {
      bool Keepalive = false, WsUpgrade = false, WsFail = false;
      http_request_t Request;
      http_response_t Response;
      size_t ReadBufSize;
      char* ReadBufferPTR = nullptr;
      time_t LastActiveSeconds;
    } http_client_t;

    /**
     * Contains info about an HTTP processing thread.
     */
    typedef struct : ServerProperties::threadpackage_t {
      HttpServer* Caller;
      pollfd PollFDs[FPX_HTTP_MAX_CLIENTS];
      http_client_t Clients[FPX_HTTP_MAX_CLIENTS];
      short ClientCount;

      /**
       * Disconnects the client with the given index from the HTTP server.
       */
      void HandleDisconnect(int clientIndex);
      struct sockaddr ClientDetails;
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

    /**
     * Contains WebSocket client data, for the WebSocket request processing threads.
     */
    typedef struct {
      // 0x1: Sent "close"
      // 0x2: Received "close"
      uint8_t Flags;
      bool Fragmented, PendingClose;
      int BytesRead;
      uint8_t ControlReadBuffer[128];
      size_t ReadBufSize;
      uint8_t* ReadBufferPTR = nullptr;
      time_t LastActiveSeconds;
    } websocket_client_t;
    
    /**
     * A WebSocket callback function that can be assigned to a websocket_threadpackage_t,
     * for processing an incoming websocket frame.
     */
    typedef void (*ws_callback_t)(websocket_client_t*, uint16_t, uint64_t, uint32_t, uint8_t*);

    /**
     * Offers a context for manipulating a WebSocket frame for sending
     */
    typedef struct {
      public:
        /**
         * Sets or resets a bit in the WS frame metadata (the first 8 bits)
         */
        void SetBit(uint8_t bit, bool value = true);

        /**
         * Sets the opcode for the WS frame
         */
        void SetOpcode(uint8_t opcode);

        /**
         * Sets the WS frame's payload
         */
        bool SetPayload(char* payload, uint16_t len);
      private:
        uint8_t m_MetaByte = 0;
        uint16_t m_PayloadLen = 0;
        char* m_Payload = nullptr;
    } websocket_frame_t;

    /**
     * Contains info about a WebSocket processing thread.
     */
    typedef struct : ServerProperties::threadpackage_t {
      HttpServer* Caller;
      pollfd PollFDs[FPX_WS_MAX_CLIENTS];
      websocket_client_t Clients[FPX_WS_MAX_CLIENTS];
      ws_callback_t Callback;
      short ClientCount = 0;
      
      /**
       * Disconnects the client with the given index from the WebSocket server.
       */
      void HandleDisconnect(int clientIndex);
      
      /**
       * Sends a WebSocket frame to the client whose index was passed.
       */
      void SendFrame(int index, websocket_frame_t*);

      /**
       * Specifically sends a WS closing frame to the client whose index was passed.
       */
      void SendClose(int index, uint16_t status, uint8_t* message = nullptr);
    } websocket_threadpackage_t;
    
    /**
     * The thread that kills idle HTTP connections after FPX_HTTP_KEEPALIVE has passed.
     */
    pthread_t HttpKillerThread;

    /**
     * The threads that handle HTTP requests.
     */
    http_threadpackage_t* RequestHandlers;

    /**
     * The threads that handle WebSocket connections.
     */
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
    /**
     * Gets the currently set default server headers.
     * These headers will always be sent along with any outgoing
     * HTTP request.
     * 
     * Also see: fpx::HttpServer::SetDefaultHeaders()
     */
    const char* GetDefaultHeaders();

    /**
     * Sets the default server headers for this server.
     * 
     * e.g. "Server: fpxHTTP\r\nHost: goodgirl.dev\r\n"
     * 
     * Also see fpx::HttpServer::GetDefaultHeaders()
     */
    void SetDefaultHeaders(const char*);

    /**
     * Creates a valid HTTP endpoint with the given URI, allowed methods, and endpoint-callback.
     */
    void CreateEndpoint(const char* uri, short methods, http_callback_t endpointCallback);

    /**
     * Starts listening. Takes fpx::HttpServer::ServerType and fpx::HttpServer::ws_callback_t;
     */
    void Listen(ServerType = ServerType::Both, ws_callback_t = nullptr);

    /**
     * Listen(), but TLS.
     * Not implemented yet.
     */
    void ListenSecure(const char*, const char*, ServerType = ServerType::Both, ws_callback_t = nullptr);

    /**
     * Stops listening.
     */
    void Close();

    /**
     * Sets maximum inactive time for WS clients.
     * Will forcefully disconnect them afterwards.
     * 
     * Also see: fpx::HttpServer::GetWebSocketTimeout();
     */
    void SetWebSocketTimeout(uint16_t minutes);

    /**
     * Gets maximum inactive time for WS clients.
     * 
     * Also see: fpx::HttpServer::SetWebSocketTimeout();
     */
    uint16_t GetWebSocketTimeout();
    
    /**
     * Set HTTP server options.
     * These go unused as of yet.
     * 
     * Also see: fpx::HttpServer::GetOption();
     */
    void SetOption(HttpServerOption, bool = true);

    /**
     * Get HTTP server options.
     * 
     * Also see: fpx::HttpServer::SetOption();
     */
    uint8_t GetOptions();

    /**
     * Will read an HTTP method string and return an enum value representing it.
     * 
     * e.g. "GET" -> fpx::HttpServer::HttpMethod::GET
     */
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
