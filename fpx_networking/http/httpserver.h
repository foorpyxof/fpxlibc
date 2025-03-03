#ifndef FPX_SERVER_HTTP_H
#define FPX_SERVER_HTTP_H

////////////////////////////////////////////////////////////////
//  "httpserver.h"                                            //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../../fpx_types.h"
#include "../tcp/tcpserver.h"

#define FPX_HTTP_DEFAULTPORT 8080

#define FPX_HTTP_MAX_CLIENTS 32
#define FPX_WS_MAX_CLIENTS 8

#define FPX_HTTP_THREADS_DEFAULT 16
#define FPX_WS_THREADS_DEFAULT 2

namespace fpx {

class HttpServer : public TcpServer {
  public:
    /**
     * The constructor takes two optional arguments.
     * - The amount of HTTP_threads
     * - The amount of WebSocket_threads
     */
    HttpServer(uint8_t = FPX_HTTP_THREADS_DEFAULT, uint8_t = FPX_WS_THREADS_DEFAULT);

    ~HttpServer();

  public:
    enum HttpMethod {
      NONE = 0x000,
      GET = 0x001,
      HEAD = 0x002,
      POST = 0x004,
      PUT = 0x008,
      DELETE = 0x010,
      CONNECT = 0x020,
      OPTIONS = 0x040,
      TRACE = 0x080,
      PATCH = 0x100,
    };

    /**
     * Set options for the HttpServer:
     *
     * - ManualWebSocket: Instead of the main thread accepting the WebSocket request,
     * the programmer can do this themselves. After setting proper headers, the request
     * will automatically be upgraded to WebSocket after the response is sent.
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
         * First bool asks if you want it returned in lowercase. This is nice for case-insensitive
         * headers, but not for headers such as Sec-WebSocket-Key (for which you will want to pass
         * 'false'). Second bool asks for whether to store the value in the char*, of which the
         * address is passed using the second argument.
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
    typedef struct {
        char URI[256];
        http_callback_t Callback;
        short AllowedMethods;
    } http_endpoint_t;

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
        void HandleDisconnect(int clientIndex, bool closeSocket = true);
        struct sockaddr ClientDetails;
        http_endpoint_t* Endpoints;
    } http_threadpackage_t;

    /**
     * Contains WebSocket client data, for the WebSocket request processing threads.
     */
    typedef struct {
      public:
        /**
         * Send a WebSocket PING frame
         */
        void Ping();

      public:
        // 0x1: Sent "close"
        // 0x2: Received "close"
        uint8_t Flags;
        bool Fragmented, PendingClose;
        int BytesRead;
        uint8_t ControlReadBuffer[128];
        size_t ReadBufSize;
        uint8_t* ReadBufferPTR = nullptr;
        time_t LastActiveSeconds;
        int FileDescriptor;
    } websocket_client_t;

    /**
     * A WebSocket callback function that can be assigned to a websocket_threadpackage_t,
     * for processing an incoming websocket frame.
     *
     * NOTE: The masking key on the incoming frame is PREAPPLIED!
     */
    typedef void (*ws_callback_t)(websocket_client_t*, uint16_t frame_metadata,
      uint64_t data_length, uint32_t masking_key, uint8_t* data_pointer);

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
        bool SetPayload(const char* payload, uint16_t len = 0);

        /**
         * Send the WS frame to the given socket file descriptor
         */
        void Send(int fd);

      private:
        uint8_t m_MetaByte = 0;
        uint64_t m_PayloadLen = 0;
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
        void HandleDisconnect(int clientIndex, bool closeSocket = true);

        /**
         * Sends a WebSocket frame to the client whose index was passed.
         */
        void SendFrame(int index, websocket_frame_t*);

        /**
         * Specifically sends a WS closing frame to the client whose index was passed.
         */
        void SendClose(int index, uint16_t status, bool bigEndian, uint8_t* message = nullptr);
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

    uint8_t HttpThreads, WsThreads;

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
     * Gets the currently set maximum body length for this server.
     *
     * Default is 4096 bytes;
     *
     * Also see fpx::HttpServer::SetMaxBodySize()
     */
    uint16_t GetMaxBodySize();

    /**
     * Sets the maximum body length for this server.
     *
     * Also see fpx::HttpServer::GetMaxBodySize()
     */
    void SetMaxBodySize(uint16_t);

    /**
     * Creates a valid HTTP endpoint with the given URI, allowed methods, and endpoint-callback.
     */
    void CreateEndpoint(const char* uri, short methods, http_callback_t endpointCallback);

    /**
     * Starts listening. Takes fpx::HttpServer::ServerType and a fpx::HttpServer::ws_callback_t to
     * set as callback for incoming WebSocket messages;
     */
    void Listen(const char*, unsigned short = FPX_HTTP_DEFAULTPORT, ws_callback_t = nullptr);

    /**
     * Listen(), but TLS.
     * Not implemented yet.
     */
    void ListenSecure(
      const char*, const char*, ServerType = ServerType::Both, ws_callback_t = nullptr);

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
     * Set the HttpServer type.
     * Options:
     * - HttpServer::ServerType::HttpOnly
     * - HttpServer::ServerType::WebSockets
     * - HttpServer::ServerType::Both
     */
    void SetServerType(ServerType);

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
    uint16_t m_MaxBodyLen;

    ServerType m_ServerType;
    uint8_t m_Options;

    uint16_t m_WebSocketTimeout;

    short m_EndpointCount;
    http_endpoint_t* m_Endpoints;
};

}  // namespace fpx


#endif  // FPX_SERVER_HTTP_H
