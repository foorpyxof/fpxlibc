#ifndef FPX_HTTPSERVER_H
#define FPX_HTTPSERVER_H

////////////////////////////////////////////////////////////////
//  "httpserver.h"                                            //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "http.h"

typedef struct _fpx_httpserver        fpx_httpserver_t;
struct _fpx_httpserver_metadata;
typedef void (*fpx_httpcallback_t)(fpx_httprequest_t*, fpx_httpresponse_t*);

// TODO: typedef struct _fpx_websocketclient   fpx_websocketclient_t;
// TODO: typedef struct _fpx_websocketframe    fpx_websocketframe_t;
// TODO: typedef void (*fpx_websocketcallback_t)(fpx_websocketclient_t*, const fpx_websocketframe_t);


enum fpx_httpserver_type {
  Http = 0,       // this means HTTP ONLY
  // TODO: WebSockets = 1, // this means HTTP + WebSockets
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
int fpx_httpserver_init(fpx_httpserver_t*, const uint8_t http_threads, const uint8_t ws_threads, uint8_t max_endpoints);

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
 * - -2 if the format of the passed headers is incorrect
 *
 * Notes:
 * - The header key can only contain human readable ASCII characters,
 * no control characters
 * - Leading and trailing whitespace on a header key is removed
 * - Leading whitespace on a header value is removed
 */
int fpx_httpserver_set_default_headers(fpx_httpserver_t*, const char*);

/**
 * Get the default headers of the server, and output them in the provided output buffer
 *
 * Input:
 * - Pointer to the server to read the default headers from
 * - Output buffer to store output in
 * - Maximum length of the output
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - any positive number to indicate the maximum buffer length
 * if the current one is deemed too small
 */
int fpx_httpserver_get_default_headers(fpx_httpserver_t*, char*, size_t);

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
 */
int fpx_httpserver_set_max_body(fpx_httpserver_t*, int16_t);

/**
 * Get the current max body length enforced on the HTTP server
 *
 * Input:
 * - Pointer to the server to get the max body size of
 *
 * Returns:
 * -  any number >= 0 to indicate the maximum body length of the server
 * - -1 if any passed pointer is unexpectedly NULL
 */
int16_t fpx_httpserver_get_max_body(fpx_httpserver_t*);

/**
 * Append an HTTP endpoint to the current list of endpoints
 *
 * Input:
 * - Pointer to the server to add an endpoint to
 * - String to represent the URI
 * - Bitwise OR of the HTTP methods that are allowed on this endpoint
 * (e.g. GET | POST | PUT allows GET, POST and PUT)
 * - A callback function to run every time the endpoint is successfully contacted
 *
 * Returns:
 * - The index of the endpoint in the array on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if an unknown error occurs
 *
 * Notes:
 * - If the URI is missing a leading '/', this will be prepended.
 * - The URI will be processed first and any '.'s or '..'s will be removed.
 */
int fpx_httpserver_create_endpoint(fpx_httpserver_t*, const char* uri, const uint16_t methods, fpx_httpcallback_t callback);

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
 * -  the value of `errno` on other failure
 */
int fpx_httpserver_listen(fpx_httpserver_t*, const char* ip, const uint16_t port);

/**
 * Stops listening on the HTTP server
 *
 * Input:
 * - Pointer to the server to stop listening on
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * -  the value of `errno` on other failure
 */
int fpx_httpserver_close(fpx_httpserver_t*);

struct _fpx_httpserver {
  struct _fpx_httpserver_metadata* _metadata;
};

#endif // FPX_HTTPSERVER_H
