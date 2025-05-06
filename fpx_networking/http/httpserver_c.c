////////////////////////////////////////////////////////////////
//  "httpserver.h"                                            //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "httpserver_c.h"
#include "../../fpx_types.h"

#include <arpa/inet.h>
#include <errno.h>
#include <http.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>


// START OF FPXLIBC LINK-TIME DEPENDENCIES
#include "../../fpx_mem/mem.h"
#include "../../fpx_string/string.h"
// END OF FPXLIBC LINK-TIME DEPENDENCIES


/* this is the default thread count for the HTTP or WS threads */
#define THREADS_DEFAULT 4

/* this is the default maximum amount of endpoints to be handled */
#define ENDPOINTS_DEFAULT 16

// returns out of the function this macro is placed in if
// the server pointer is not valid or the server is not initialized
#define SRV_ASSERT(ptr)              \
  {                                  \
    int r = _server_init_check(ptr); \
    if (r < 0)                       \
      return r;                      \
  }

// struct forward declarations
struct _http_thread;
// TODO: struct _websocket_thread;

// TODO: struct _websocket_frame;

struct _fpx_httpendpoint;
struct _fpx_httpserver_metadata;
// end of struct forward declarations

static int _fpx_header_sanitize(const char* in, char* out, size_t out_len, size_t* copied_count);
static int _server_init_check(fpx_httpserver_t*);

static void* _fpx_http_loop(void* thread_package);
// TODO: static void* _fpx_websocket_loop(void* thread_package);
static void* _fpx_http_killer(void* http_server_metadata);


// start struct definitions
struct _fpx_httpendpoint {
    uint8_t active;

    char uri[256];
    uint16_t allowed_methods;

    fpx_httpcallback_t callback;
};

struct _http_thread {
    fpx_httpserver_t* server;
    pthread_t thread;

    int connected_clients;
};

struct _fpx_httpserver_metadata {
    int socket_4;
    // int socket_6;

    struct sockaddr_in addr_4;
    // struct sockaddr_in6 addr_6;

    uint8_t http_thread_count;
    struct _http_thread* http_threads;
    uint8_t ws_thread_count;
    struct _ws_thread* ws_threads;

    struct _fpx_httpendpoint* endpoints;
    uint8_t active_endpoints;
    uint8_t max_endpoints;

    char* default_headers;
    size_t default_headers_len;

    uint8_t is_listening;

    int16_t max_body_size;
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
    meta->http_thread_count = THREADS_DEFAULT;
  else
    meta->http_thread_count = http_threads;

  if (0 == ws_threads)
    meta->ws_thread_count = THREADS_DEFAULT;
  else
    meta->ws_thread_count = ws_threads;

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
  SRV_ASSERT(srvptr);
  if (NULL == headers)
    return -1;

  int headers_len = fpx_getstringlength(headers);

  // first we check if we're maybe unsetting all the headers
  if (headers_len < 1) {
    // if so, we ball

    if (NULL != srvptr->_metadata->default_headers)
      free(srvptr->_metadata->default_headers);

    srvptr->_metadata->default_headers_len = 0;

    return 0;
  }


  size_t new_headers_len;
  char* temp_output = (char*)malloc(headers_len + 1);

  int retval = _fpx_header_sanitize(headers, temp_output, headers_len + 1, &new_headers_len);


  if (retval < 0)
    return retval;
  // else means success


  {
    if (NULL == srvptr->_metadata->default_headers)
      srvptr->_metadata->default_headers = (char*)malloc(new_headers_len);
    else
      srvptr->_metadata->default_headers =
        realloc(srvptr->_metadata->default_headers, new_headers_len);
  }


  // copy contents of temp buffer into default_headers buffer of the server
  fpx_memcpy(srvptr->_metadata->default_headers, temp_output, new_headers_len);
  srvptr->_metadata->default_headers_len = new_headers_len;

  free(temp_output);

  return retval;
}


int fpx_httpserver_get_default_headers(fpx_httpserver_t* srvptr, char* outbuf, size_t outbuf_size) {
  SRV_ASSERT(srvptr);
  if (NULL == outbuf)
    return -1;

  {
    struct _fpx_httpserver_metadata* meta = srvptr->_metadata;
    if (NULL == meta->default_headers || meta->default_headers_len == 0)
      return -3;

    if (outbuf_size < 2)
      return meta->default_headers_len + 1;
  }


  // check how much to copy over
  // (we do partial if buffer not long enough)
  int amount;
  int retval;
  if (outbuf_size > srvptr->_metadata->default_headers_len) {
    amount = srvptr->_metadata->default_headers_len;
    retval = 0;
  } else {
    amount = outbuf_size - 1;
    retval = srvptr->_metadata->default_headers_len + 1;
  }


  fpx_memcpy(outbuf, srvptr->_metadata->default_headers, amount);
  outbuf[amount] = 0;  // NULL-terminate the output


  return retval;
}


int fpx_httpserver_set_max_body(fpx_httpserver_t* srvptr, int16_t maxsize) {
  SRV_ASSERT(srvptr);

  srvptr->_metadata->max_body_size = maxsize;

  return 0;
}


int16_t fpx_httpserver_get_max_body(fpx_httpserver_t* srvptr) {
  SRV_ASSERT(srvptr);

  return srvptr->_metadata->max_body_size;
}


int fpx_httpserver_create_endpoint(
  fpx_httpserver_t* srvptr, const char* uri, const uint16_t methods, fpx_httpcallback_t callback) {
  SRV_ASSERT(srvptr);
  if (NULL == uri || NULL == callback)
    return -1;


  struct _fpx_httpserver_metadata* meta = srvptr->_metadata;

  int uri_len = fpx_getstringlength(uri);

  if (uri_len > sizeof(meta->endpoints->uri) - 1)
    return -4;

  int available_endpoint = 0;
  {
    int i;

    i = 0;
    for (int j = 0; i < meta->max_endpoints && j < meta->active_endpoints; ++i) {
      if (TRUE == meta->endpoints[i].active)
        ++j;
    }

    available_endpoint = i;
  }


  if (available_endpoint >= meta->max_endpoints)
    return -3;


  struct _fpx_httpendpoint* ptr = &meta->endpoints[available_endpoint];
  ptr->active = TRUE;
  ptr->allowed_methods = methods;
  ptr->callback = callback;

  fpx_strcpy(ptr->uri, uri);


  return 0;
}


int fpx_httpserver_listen(fpx_httpserver_t* srvptr, const char* ip, const uint16_t port) {
  SRV_ASSERT(srvptr);
  if (NULL == ip)
    return -1;

  if (0 == port)
    return -4;

  struct _fpx_httpserver_metadata* meta = srvptr->_metadata;

  // socket things
  {
    int listen_socket;
    struct sockaddr_in listen_addr;

    listen_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_socket == -1) {
      int err = errno;
      perror("socket()");
      return err;
    }

    // set IP address for socket; return error if invalid
    if (0 == inet_aton(ip, &listen_addr.sin_addr))
      return -3;

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);

    if (-1 == bind(listen_socket, (struct sockaddr*)&listen_addr, sizeof(listen_addr))) {
      int err = errno;
      perror("bind()");
      return err;
    }

    if (-1 == listen(listen_socket, 256)) {
      int err = errno;
      perror("listen()");
      return err;
    }

    meta->socket_4 = listen_socket;
    meta->addr_4 = listen_addr;
  }
  // end of socket things


  // start threading
  {
    meta->http_threads =
      (struct _http_thread*)calloc(meta->http_thread_count, sizeof(struct _http_thread));

    for (int i = 0; i < meta->http_thread_count; ++i) {
      struct _http_thread* t = &meta->http_threads[i];

      if (0 != pthread_create(&t->thread, NULL, _fpx_http_loop, t)) {
        int err = errno;
        perror("pthread_create()");
        return err;
      }

      t->server = srvptr;
    }
  }
  // end of threading things


  return 0;
}


int fpx_httpserver_close(fpx_httpserver_t* srvptr) {
  SRV_ASSERT(srvptr);
  if (FALSE == srvptr->_metadata->is_listening)
    return -3;

  srvptr->_metadata->is_listening = FALSE;

  return 0;
}


// ------------------------------------------------------- //
// ------------------------------------------------------- //
// ---------- STATIC FUNCTION DEFINITIONS BELOW ---------- //
// ------------------------------------------------------- //
// ------------------------------------------------------- //


/**
 * - return  0 on success
 * - return -1 if any passed pointers are NULL
 * - return -3 if the general header formatting is incorrect
 */
static int _fpx_header_sanitize(const char* in, char* out, size_t out_len, size_t* copied_count) {
  if (NULL == in || NULL == out || NULL == copied_count)
    return -1;

  char* temp_buffer;
  char* temp_copy;

  int headers_len = fpx_getstringlength(in);

  int key_index;
  int value_index;
  int crlf_index;
  int headers_index = 0;


  temp_buffer = (char*)malloc(headers_len);
  temp_copy = temp_buffer;


  uint8_t going = TRUE;
  while (TRUE) {
    int addition;


    // stage 1: find first readable character
    while (in[headers_index] <= ' ' || in[headers_index] > '~') {
      if (in[headers_index] == '\0') {
        going = FALSE;
        break;
      }

      headers_index++;
    }


    if (!going)
      break;

    key_index = headers_index;


    // stage 2: find colon
    while (in[headers_index] > ' ' && in[headers_index] <= '~' && in[headers_index] != ':')
      headers_index++;

    if (in[headers_index] != ':') {
      free(temp_buffer);
      return -3;
    }


    addition = headers_index - key_index;
    fpx_memcpy(temp_copy, in + key_index, addition);
    temp_copy[addition] = 0;
    // set the header key to lowercase to match the rest of
    // fpxlibc's HTTP implementation
    fpx_string_to_lower(temp_copy, FALSE);
    temp_copy += addition;

    fpx_memcpy(temp_copy, ": ", 2);
    temp_copy += 2;

    headers_index++;  // skip past the colon


    // stage 3: remove leading whitespace from value
    while (in[headers_index] <= ' ' || in[headers_index] > '~') {
      if (in[headers_index] == '\0') {
        going = FALSE;
        break;
      }

      headers_index++;
    }

    if (!going)
      break;

    value_index = headers_index;

    // stage 4: find where the value ends, and copy to temo buffer
    while (in[headers_index] >= ' ' && in[headers_index] < '~')
      headers_index++;

    if (in[headers_index++] != '\r') {
      free(temp_buffer);
      return -3;
    }
    if (in[headers_index++] != '\n') {
      free(temp_buffer);
      return -3;
    }


    addition = headers_index - value_index;
    fpx_memcpy(temp_copy, in + value_index, addition);
    temp_copy += addition;
  }


  int new_headers_len;
  new_headers_len = fpx_getstringlength(temp_buffer);

  *copied_count = new_headers_len;


  // copy contents of temp buffer into output buffer
  fpx_memcpy(out, temp_buffer, new_headers_len);

  free(temp_buffer);

  return 0;
}

/**
 * - returns  0 on success
 * - returns -1 if the server pointer is NULL
 * - returns -2 if the server is uninitialized
 */
static int _server_init_check(fpx_httpserver_t* srvptr) {
  if (NULL == srvptr)
    return -1;

  if (NULL == srvptr->_metadata)
    return -2;

  return 0;
}
