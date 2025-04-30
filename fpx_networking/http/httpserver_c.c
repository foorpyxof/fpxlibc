////////////////////////////////////////////////////////////////
//  "httpserver.h"                                            //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "httpserver_c.h"

#include <errno.h>
#include <fpxlibc/fpx_types.h>
#include <netinet/in.h>
#include <stdlib.h>


// START OF FPXLIBC LINK-TIME DEPENDENCIES
#include "../../fpx_mem/mem.h"
#include "../../fpx_string/string.h"
// END OF FPXLIBC LINK-TIME DEPENDENCIES


/* this is the default thread count for the HTTP or WS threads */
#define THREADS_DEFAULT 4

/* this is the default maximum amount of endpoints to be handled */
#define ENDPOINTS_DEFAULT 16


// struct forward declarations
struct _http_package;
// TODO: struct _websocket_package;

// TODO: struct _websocket_frame;

struct _fpx_httpendpoint;
struct _fpx_httpserver_metadata;
// end of struct forward declarations


static void* _fpx_http_loop(void* thread_package);
// TODO: static void* _fpx_websocket_loop(void* thread_package);
static void* _fpx_http_killer(void* http_server);


// start struct definitions
struct _fpx_httpendpoint {
    uint8_t active;

    char uri[256];
    uint16_t allowed_methods;

    fpx_httpcallback_t callback;
};

struct _fpx_httpserver_metadata {
    int socket_4;
    // int socket_6;

    struct sockaddr_in addr_4;
    // struct sockaddr_in6 addr_6;

    uint8_t http_threads;
    uint8_t ws_threads;

    struct _fpx_httpendpoint* endpoints;
    uint8_t active_endpoints;
    uint8_t max_endpoints;

    char* default_headers;
    size_t default_headers_len;
};
// end struct definitions


int fpx_httpserver_init(fpx_httpserver_t* srvptr, const uint8_t http_threads,
  const uint8_t ws_threads, uint8_t max_endpoints) {
  if (NULL == srvptr)
    return -1;

  fpx_memset(srvptr, 0, sizeof(*srvptr));

  srvptr->_metadata =
    (struct _fpx_httpserver_metadata*)calloc(1, sizeof(struct _fpx_httpserver_metadata));
  if (NULL == srvptr->_metadata)
    return errno;

  struct _fpx_httpserver_metadata* meta = srvptr->_metadata;

  if (0 == http_threads)
    meta->http_threads = THREADS_DEFAULT;
  else
    meta->http_threads = http_threads;

  if (0 == ws_threads)
    meta->ws_threads = THREADS_DEFAULT;
  else
    meta->ws_threads = ws_threads;

  if (0 == max_endpoints)
    meta->max_endpoints = ENDPOINTS_DEFAULT;
  else
    meta->max_endpoints = max_endpoints;

  meta->endpoints =
    (struct _fpx_httpendpoint*)calloc(meta->max_endpoints, sizeof(*(meta->endpoints)));

  if (NULL == meta->endpoints) {
    free(meta);
    return errno;
  }

  return 0;
}


int fpx_httpserver_set_default_headers(fpx_httpserver_t* srvptr, const char* headers) {
  if (NULL == srvptr || NULL == headers)
    return -1;

  char* temp_buffer;
  char* temp_copy;

  int headers_len = fpx_getstringlength(headers);

  temp_buffer = (char*)malloc(headers_len);
  temp_copy = temp_buffer;

  int key_index;
  int value_index;
  int crlf_index;
  int headers_index = 0;


  uint8_t going = TRUE;
  while (TRUE) {
    int addition;


    // stage 1: find first readable character
    while (headers[headers_index] < ' ' || headers[headers_index] > '~') {
      if (headers[headers_index] == '\0') {
        going = FALSE;
        break;
      }

      headers_index++;
    }


    if (!going)
      break;

    key_index = headers_index;


    // stage 2: find colon
    while (headers[headers_index] > ' ' && headers[headers_index] <= '~' &&
      headers[headers_index] != ':')
      headers_index++;

    if (headers[headers_index] != ':')
      return -2;


    addition = headers_index - key_index;
    fpx_memcpy(temp_copy, headers + key_index, addition);
    temp_copy += addition;
    fpx_memcpy(temp_copy, ": ", 2);
    temp_copy += 2;

    headers_index++;  // skip past the colon


    // stage 3: remove leading whitespace from value
    while (headers[headers_index] < ' ' || headers[headers_index] > '~') {
      if (headers[headers_index] == '\0') {
        going = FALSE;
        break;
      }

      headers_index++;
    }

    if (!going)
      break;

    value_index = headers_index;

    // stage 4: find where the value ends, and copy to temo buffer
    while (headers[headers_index] >= ' ' && headers[headers_index] < '~')
      headers_index++;

    if (headers[headers_index++] != '\r')
      return -2;
    if (headers[headers_index++] != '\n')
      return -2;


    addition = headers_index - value_index;
    fpx_memcpy(temp_copy, headers + value_index, addition);
    temp_copy += addition;
  }


  int new_headers_len;
  new_headers_len = fpx_getstringlength(temp_buffer);


  {
    if (NULL == srvptr->_metadata->default_headers)
      srvptr->_metadata->default_headers = (char*)malloc(new_headers_len);
    else
      srvptr->_metadata->default_headers =
        realloc(srvptr->_metadata->default_headers, new_headers_len);
  }


  // copy contents of temp buffer into default_headers buffer of the server
  fpx_memcpy(srvptr->_metadata->default_headers, temp_buffer, new_headers_len);
  srvptr->_metadata->default_headers_len = new_headers_len;

  free(temp_buffer);

  return 0;
}
