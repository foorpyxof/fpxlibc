#ifndef FPX_HTTPSERVER_H
#define FPX_HTTPSERVER_H

#include "http.h"
#include "websockets.h"

//
//  "httpserver.h"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

typedef struct _fpx_httpserver fpx_httpserver_t;
struct _fpx_httpserver_metadata;
typedef void (*fpx_httpcallback_t)(const fpx_httprequest_t *,
                                   fpx_httpresponse_t *);

typedef struct _fpx_websocketclient fpx_websocketclient_t;
typedef void (*fpx_websocketcallback_t)(const fpx_websocketframe_t *_incoming,
                                        int file_descriptor,
                                        const struct sockaddr *);

enum fpx_httpserver_type {
  Http = 0,       // this means HTTP ONLY
  WebSockets = 1, // this means HTTP + WebSockets
};

enum fpx_httpserver_opts {
  ManualWsUpgrade = 0x01, // this allows for upgrading the request within the
                          // HTTP endpoint callback instead of automatically
};

/**
 * Initializes an object of type fpx_httpserver_t to default values
 *
 * Input:
 * - Pointer to the server to initialize
 * - The amount of HTTP processing threads to use (> 0)
 * - The amount of WebSocket processing threads to use (> 0)
 * - The maximum amount of HTTP endpoints to handle
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * -  the value of `errno` if other failure occurs
 *
 * Notes:
 * - Passing 0 for the processing threads or the endpoint count
 * will set it to the default instead. The default is specified
 * in the implementation file
 */
int fpx_httpserver_init(fpx_httpserver_t *, const uint8_t http_threads,
                        const uint8_t ws_threads, uint8_t max_endpoints);

/**
 * Set the default headers to be applied to every outgoing response\
 *
 * Input:
 * - Pointer to the server to apply the default headers to
 * - NULL-terminated string that contains the default headers in the format:
 * "[key]: [value]\r\n[key]:[value]\r\n" etc.
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if the server object is uninitialized
 * - -3 if the format of the passed headers is incorrect
 *
 * Notes:
 * - The header key can only contain human readable ASCII characters,
 * no control characters
 * - Leading and trailing whitespace on a header key is removed
 * - Leading whitespace on a header value is removed
 */
int fpx_httpserver_set_default_headers(fpx_httpserver_t *, const char *);

/**
 * Get the default headers of the server, and output them in the provided output
 * buffer
 *
 * Input:
 * - Pointer to the server to read the default headers from
 * - Output buffer to store NULL-terminated output in
 * - Maximum length of the output
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if the server object is uninitialized
 * - -3 if there are no default headers to read
 * - any positive number to indicate the maximum buffer length
 * if the current one is deemed too small
 *
 * Notes:
 * - The operation will PARTIALLY complete if the output buffer is deemed too
 * small
 * - A null-terminator will ALWAYS be appended. Even if the buffer is too short.
 */
int fpx_httpserver_get_default_headers(fpx_httpserver_t *, char *, size_t);

/**
 * Set the maximum body length that the server will accept from requests
 * or send in responses
 *
 * Input:
 * - Pointer to the server to set the max body size of
 * - The maximum body length to set (>= 0)
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if the server object is uninitialized
 */
// int fpx_httpserver_set_max_body(fpx_httpserver_t*, int16_t);

/**
 * Get the current max body length enforced on the HTTP server
 *
 * Input:
 * - Pointer to the server to get the max body size of
 *
 * Returns:
 * -  any number >= 0 to indicate the maximum body length of the server
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if the server object is uninitialized
 */
// int16_t fpx_httpserver_get_max_body(fpx_httpserver_t*);

/**
 * Set the callback function for the default, catch-all endpoint.
 * If this function is never called, the endpoint will not be used.
 *
 * Input:
 * - Pointer to the server to set the default callback for
 * - Bitwise OR of the HTTP methods that are allowed on this endpoint
 * (e.g. GET | POST | PUT allows GET, POST and PUT)
 * - A callback function to run every time the endpoint is successfully
 * contacted
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if the server object is uninitialized
 */
int fpx_httpserver_set_default_http_endpoint(fpx_httpserver_t *,
                                             const uint16_t methods,
                                             fpx_httpcallback_t);

/**
 * Append an HTTP endpoint to the current list of endpoints
 *
 * Input:
 * - Pointer to the server to add an endpoint to
 * - Null-terminated string to represent the URI
 * - Bitwise OR of the HTTP methods that are allowed on this endpoint
 * (e.g. GET | POST | PUT allows GET, POST and PUT)
 * - A callback function to run every time the endpoint is successfully
 * contacted
 *
 * Returns:
 * - The index of the endpoint in the array on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if the server object is uninitialized
 * - -3 if the limit of endpoints is reached
 * - -4 if the given URI is too long to fit (maximum 255)
 *
 * Notes:
 * - If the URI is missing a leading '/', this will be prepended.
 * - The URI will be processed first and any '.'s or '..'s will be removed.
 */
int fpx_httpserver_create_endpoint(fpx_httpserver_t *, const char *uri,
                                   const uint16_t methods,
                                   fpx_httpcallback_t callback);

/**
 * Append a WebSockets endpoint to the current list of endpoints
 *
 * Input:
 * - Pointer to the server to add an endpoint to
 * - Null-terminated string to represent the URI
 * - A callback function to run every time a client connected to the endpoint
 * sends a message
 *
 * Returns:
 * - The index of the endpoint in the array on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if the server object is uninitialized
 * - -3 if the limit of endpoints is reached
 * - -4 if the given URI is too long to fit (maximum 255)
 *
 * Notes:
 * - If the URI is missing a leading '/', this will be prepended.
 * - The URI will be processed first and any '.'s or '..'s will be removed.
 */
int fpx_httpserver_create_ws_endpoint(fpx_httpserver_t *, const char *uri,
                                      fpx_websocketcallback_t callback);

/**
 * Start listening for HTTP requests on [ip]:[port]
 *
 * Input:
 * - Pointer to the server object to listen with
 * - A string containing an IPv4 address in dot-decimal notation
 * (e.g. 12.34.56.78 or 127.0.0.1)
 * - A TCP port to listen on (e.g. 8080 or 443)
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if the server object is uninitialized
 * - -3 is the passed IP address is invalid
 * - -4 if the passed port is invalid ( == 0 )
 * -  the value of `errno` on other failure
 */
int fpx_httpserver_listen(fpx_httpserver_t *, const char *ip,
                          const uint16_t port);

/**
 * Stops listening on the HTTP server
 *
 * Input:
 * - Pointer to the server to stop listening on
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if the server object is uninitialized
 * - -3 if the server object is not currently listening
 * -  the value of `errno` on other failure
 */
int fpx_httpserver_close(fpx_httpserver_t *);

struct _fpx_httpserver {
  enum fpx_httpserver_type server_type;
  uint8_t options;

  uint8_t max_endpoints;

  uint8_t http_thread_count;
  uint8_t ws_thread_count;

  fpx_websocketcallback_t ws_callback;

  uint32_t keepalive_timeout;
  uint32_t websockets_timeout;

  struct _fpx_httpserver_metadata *_internal;
};

#endif // FPX_HTTPSERVER_H
