//
//  "httpserver.c"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "fpx_debug.h"
#include "fpx_types.h"
#include "networking/http/httpserver.h"
#include "networking/netutils.h"

#if defined(_WIN32) || defined(_WIN64)
#else
#include <poll.h>
#endif

// START OF FPXLIBC LINK-TIME DEPENDENCIES
#include "c-utils/crypto.h" // requires crypto*.o
#include "c-utils/endian.h" // requires endian*.o
#include "c-utils/format.h" // requires format*.o
#include "mem/mem.h"        // requires mem*.o
#include "string/string.h"  // requires string*.o

#include "networking/http/http.h"       // requires http*.o
#include "networking/http/websockets.h" // requires http*.o
// END OF FPXLIBC LINK-TIME DEPENDENCIES

/* this is the default thread count for the HTTP or WS threads */
#define THREADS_DEFAULT 4

/* this is the default maximum amount of endpoints to be handled */
#define ENDPOINTS_DEFAULT 16

/* this is the default maximum amount of clients to handle per HTTP thread */
#define CLIENTS_DEFAULT 64

/* this is the default buffer size for HTTP reading and writing */
#define BUFFER_DEFAULT 16384

#define MAX_HEADERS 4096
// #define MAX_BODY 8192

/* this is the HTTP version that will be used in responses */
#define HTTP_VERSION 1.1

/* this is the default keepalive time to enforce on the server */
#define KEEPALIVE_SECONDS 7

/* this is the default maximum idle timer for websocket connections */
#define WS_TIMEOUT_SECONDS 60

#define SERVER_HEADER "fpx_http"

// returns out of the function this macro is placed in if
// the server pointer is not valid or the server is not initialized
#define SRV_ASSERT(ptr)                                                        \
  {                                                                            \
    int r = _server_init_check(ptr);                                           \
    if (r < 0)                                                                 \
      return r;                                                                \
  }

// struct forward declarations
struct _client_meta;

struct _thread;

struct _websocket_thread;

struct _websocket_frame;

struct _fpx_endpoint;
struct _fpx_httpserver_metadata;
// end of struct forward declarations

/* start of static function declarations */
static int _server_init_check(fpx_httpserver_t *);

static int _endpoint_get(fpx_httpserver_t *, const char *uri);
static int _available_endpoint_index(fpx_httpserver_t *, const char *uri);
static int _fix_endpoint_uri(const char *in, char *out, size_t maxlen);

static void _thread_init(struct _thread *);
static void *_thread_loop(void *thread_package);

static int _add_client_to_thread(struct _thread *, int client_fd);
static int _disconnect_client(struct _thread *thread, int idx,
                              uint8_t close_socket);

static int _http_handle_client(struct _thread *thread, int idx);
static int _http_parse_request(char *readbuf, int readbuf_len,
                               fpx_httprequest_t *output);
static void _handle_http_endpoint(struct _thread *, fpx_httprequest_t *,
                                  fpx_httpresponse_t *);
static int _send_response(fpx_httprequest_t *, fpx_httpresponse_t *,
                          int client_fd);

static int _ws_handle_client(struct _thread *thread, int idx);
static int _ws_parse_request(int fd, fpx_websocketframe_t *output);

// returns -2 on invalid request (400)
// returns -3 if no content-length was passed, and thus no body was read
static int _apply_default_headers(fpx_httpserver_t *srvptr,
                                  fpx_httpresponse_t *resptr);
static int _http_parse_request_line(const char *, fpx_httprequest_t *output);
static int _sanitize_headers(const char *in, char *out, size_t out_len,
                             size_t *copied_count);
static int _is_upgradable(fpx_httprequest_t *);
static void _set_keepalive(fpx_httprequest_t *, fpx_httpresponse_t *);
static void _upgrade_to_ws(struct _thread *, int idx, fpx_websocketcallback_t);
static void _on_response_ready(struct _thread *, fpx_httprequest_t *,
                               fpx_httpresponse_t *, int idx);

static int _ws_frame_validate(fpx_websocketframe_t *, fpx_websocketclient_t *,
                              int fd);
static void _handle_control_frame(fpx_websocketframe_t *,
                                  fpx_websocketclient_t *, int fd);
static int _generate_ws_accept_header(fpx_httprequest_t *, char[32]);

// static int _check_manual_ws_upgrade(fpx_httprequest_t*, fpx_httpresponse_t*);
/* end of static function declarations */

#define SET_HTTPRESPONSE_PRESET(srvptr, res, code, reason_text, body,          \
                                body_len)                                      \
  {                                                                            \
    if (0 == fpx_httpresponse_set_version(&res, STR(HTTP_VERSION)) &&          \
        (res.status = code) && fpx_strcpy(res.reason, reason_text) &&          \
        0 == fpx_httpresponse_append_body(&res, body, body_len)) {             \
    }                                                                          \
  }

#define SET_HTTP_200(srvptr, res, body, body_len)                              \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 200, "OK", body, body_len)

#define SET_HTTP_400(srvptr, res, body, body_len)                              \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 400, "Bad Request", body, body_len)

#define SET_HTTP_404(srvptr, res, body, body_len)                              \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 404, "Not Found", body, body_len)

#define SET_HTTP_405(srvptr, res, body, body_len)                              \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 405, "Method Not Allowed", body,        \
                          body_len)

#define SET_HTTP_411(srvptr, res, body, body_len)                              \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 411, "Length Required", body, body_len)

#define SET_HTTP_500(srvptr, res, body, body_len)                              \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 500, "Internal Server Error", body,     \
                          body_len)

#define SET_HTTP_503(srvptr, res, body, body_len)                              \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 503, "Service Unavailable", body,       \
                          body_len)

#define SET_HTTP_505(srvptr, res, body, body_len)                              \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 505, "HTTP Version Not Supported",      \
                          body, body_len)

// start struct definitions
enum _ws_client_flags {
  CLOSE_SENT = 0x01,
  CLOSE_RECV = 0x02,
};

struct _fpx_websocketclient {
  uint8_t flags;
  fpx_websocketcallback_t callback;
};

struct _client_meta {
  time_t last_time;
  struct sockaddr in_address;

  fpx_websocketclient_t ws;
};

struct _thread {
  fpx_httpserver_t *server;
  pthread_t thread;

  pthread_mutex_t loop_mutex;
  pthread_cond_t loop_condition;

  int connection_count;
  struct pollfd *pfds;
  struct _client_meta *client_data;

  // function pointer to the client handler; (HTTP or websockets)
  int (*handler)(struct _thread *, int);

  enum { HttpThread, WebSocketsThread } thread_type;
};

struct _fpx_endpoint {
  uint8_t active;

  char uri[256];
  uint16_t allowed_methods;

  fpx_httpcallback_t http_callback;
  fpx_websocketcallback_t ws_callback;
};

struct _fpx_httpserver_metadata {
  int socket_4;
  // int socket_6;

  struct sockaddr_in addr_4;
  // struct sockaddr_in6 addr_6;

  struct _thread *http_threads;
  struct _thread *ws_threads;

  struct _fpx_endpoint default_endpoint;

  struct _fpx_endpoint *endpoints;
  uint8_t active_endpoints;

  char *default_headers;
  size_t default_headers_len;

  uint8_t is_listening;

  // int16_t max_body_size;
};
// end struct definitions

int fpx_httpserver_init(fpx_httpserver_t *srvptr, const uint8_t http_threads,
                        const uint8_t ws_threads, uint8_t max_endpoints) {
  if (NULL == srvptr)
    return -1;

  fpx_memset(srvptr, 0, sizeof(*srvptr));

  srvptr->server_type = WebSockets;

  srvptr->_internal = (struct _fpx_httpserver_metadata *)calloc(
      1, sizeof(struct _fpx_httpserver_metadata));

  if (NULL == srvptr->_internal)
    return errno;

  struct _fpx_httpserver_metadata *meta = srvptr->_internal;

  if (0 == http_threads)
    srvptr->http_thread_count = THREADS_DEFAULT;
  else
    srvptr->http_thread_count = http_threads;

  if (0 == ws_threads)
    srvptr->ws_thread_count = THREADS_DEFAULT;
  else
    srvptr->ws_thread_count = ws_threads;

  if (0 == max_endpoints)
    srvptr->max_endpoints = ENDPOINTS_DEFAULT;
  else
    srvptr->max_endpoints = max_endpoints;

  meta->default_endpoint.active = 0;
  meta->endpoints = (struct _fpx_endpoint *)calloc(srvptr->max_endpoints,
                                                   sizeof(*(meta->endpoints)));

  if (NULL == meta->endpoints) {
    free(meta);
    return errno;
  }

  srvptr->keepalive_timeout = KEEPALIVE_SECONDS;
  srvptr->websockets_timeout = WS_TIMEOUT_SECONDS;

  fpx_httpserver_set_default_headers(srvptr, "server: " SERVER_HEADER "\r\n");

  return 0;
}

int fpx_httpserver_set_default_headers(fpx_httpserver_t *srvptr,
                                       const char *headers) {
  SRV_ASSERT(srvptr);
  if (NULL == headers)
    return -1;

  int headers_len = fpx_getstringlength(headers);

  // first we check if we're maybe unsetting all the headers
  if (headers_len < 1) {
    // if so, we ball

    if (NULL != srvptr->_internal->default_headers)
      free(srvptr->_internal->default_headers);

    srvptr->_internal->default_headers_len = 0;

    return 0;
  }

  size_t new_headers_len;
  char *temp_output = (char *)malloc(headers_len + 1);

  int retval = _sanitize_headers(headers, temp_output, headers_len + 1,
                                 &new_headers_len);

  if (retval < 0)
    return retval;
  // else means success

  {
    if (NULL == srvptr->_internal->default_headers)
      srvptr->_internal->default_headers = (char *)malloc(new_headers_len);
    else
      srvptr->_internal->default_headers =
          realloc(srvptr->_internal->default_headers, new_headers_len);
  }

  // copy contents of temp buffer into default_headers buffer of the server
  fpx_memcpy(srvptr->_internal->default_headers, temp_output, new_headers_len);
  srvptr->_internal->default_headers_len = new_headers_len;

  free(temp_output);

  return retval;
}

int fpx_httpserver_get_default_headers(fpx_httpserver_t *srvptr, char *outbuf,
                                       size_t outbuf_size) {
  SRV_ASSERT(srvptr);
  if (NULL == outbuf)
    return -1;

  {
    struct _fpx_httpserver_metadata *meta = srvptr->_internal;
    if (NULL == meta->default_headers || meta->default_headers_len == 0)
      return -3;

    if (outbuf_size < 2)
      return meta->default_headers_len + 1;
  }

  // check how much to copy over
  // (we do partial if buffer not long enough)
  int amount;
  int retval;
  if (outbuf_size > srvptr->_internal->default_headers_len) {
    amount = srvptr->_internal->default_headers_len;
    retval = 0;
  } else {
    amount = outbuf_size - 1;
    retval = srvptr->_internal->default_headers_len + 1;
  }

  fpx_memcpy(outbuf, srvptr->_internal->default_headers, amount);
  outbuf[amount] = 0; // NULL-terminate the output

  return retval;
}

// int fpx_httpserver_set_max_body(fpx_httpserver_t* srvptr, int16_t maxsize) {
//   SRV_ASSERT(srvptr);
//
//   srvptr->_internal->max_body_size = maxsize;
//
//   return 0;
// }
//
//
// int16_t fpx_httpserver_get_max_body(fpx_httpserver_t* srvptr) {
//   SRV_ASSERT(srvptr);
//
//   return srvptr->_internal->max_body_size;
// }

int fpx_httpserver_set_default_http_endpoint(fpx_httpserver_t *srvptr,
                                             const uint16_t methods,
                                             fpx_httpcallback_t callback) {
  SRV_ASSERT(srvptr);

  if (NULL == callback)
    return -1;

  struct _fpx_httpserver_metadata *meta = srvptr->_internal;

  meta->default_endpoint.allowed_methods = methods;
  meta->default_endpoint.http_callback = callback;
  meta->default_endpoint.active = TRUE;

  return 0;
}

int fpx_httpserver_create_endpoint(fpx_httpserver_t *srvptr, const char *uri,
                                   const uint16_t methods,
                                   fpx_httpcallback_t callback) {
  SRV_ASSERT(srvptr);
  if (NULL == uri || NULL == callback)
    return -1;

  int endpoint_index = _endpoint_get(srvptr, uri);

  if (0 > endpoint_index)
    return endpoint_index;

  struct _fpx_endpoint *ptr = &srvptr->_internal->endpoints[endpoint_index];

  ptr->allowed_methods = methods;
  if (methods & HTTP_GET) {
    // if GET is allowed, also allow HEAD
    ptr->allowed_methods |= HTTP_HEAD;
  }

  ptr->http_callback = callback;
  ptr->active = TRUE;

  return 0;
}

int fpx_httpserver_create_ws_endpoint(fpx_httpserver_t *srvptr, const char *uri,
                                      fpx_websocketcallback_t callback) {
  SRV_ASSERT(srvptr);
  if (NULL == uri || NULL == callback)
    return -1;

  int endpoint_index = _endpoint_get(srvptr, uri);

  if (0 > endpoint_index)
    return endpoint_index;

  struct _fpx_endpoint *ptr = &srvptr->_internal->endpoints[endpoint_index];

  ptr->ws_callback = callback;
  ptr->active = TRUE;

  return 0;
}

int fpx_httpserver_listen(fpx_httpserver_t *srvptr, const char *ip,
                          const uint16_t port) {
  SRV_ASSERT(srvptr);
  if (NULL == ip)
    return -1;

  if (0 == port)
    return -4;

  struct _fpx_httpserver_metadata *meta = srvptr->_internal;

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

    {
      int one = 1;
      setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (void *)&one,
                 sizeof(one));
    }

// set IP address for socket; return error if invalid
#if defined(_WIN32) || defined(_WIN64)
    if (1 != inet_pton(AF_INET, ip, &listen_addr.sin_addr))
      return -3;
#else
    if (0 == inet_aton(ip, &listen_addr.sin_addr))
      return -3;
#endif

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);

    if (-1 == bind(listen_socket, (struct sockaddr *)&listen_addr,
                   sizeof(listen_addr))) {
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

    meta->is_listening = TRUE;
  }
  // end of socket things

  // start threading
  {
    meta->http_threads = (struct _thread *)calloc(srvptr->http_thread_count,
                                                  sizeof(struct _thread));

    for (int i = 0; i < srvptr->http_thread_count; ++i) {
      struct _thread *t = &meta->http_threads[i];

      t->server = srvptr;
      t->thread_type = HttpThread;
      t->handler = _http_handle_client;

      pthread_mutex_init(&t->loop_mutex, NULL);

      if (0 != pthread_create(&t->thread, NULL, _thread_loop, t)) {
        int err = errno;
        perror("pthread_create()");
        return err;
      }
    }
  }

  {
    meta->ws_threads = (struct _thread *)calloc(srvptr->ws_thread_count,
                                                sizeof(struct _thread));

    for (int i = 0; i < srvptr->ws_thread_count; ++i) {
      struct _thread *t = &meta->ws_threads[i];

      t->server = srvptr;
      t->thread_type = WebSocketsThread;
      t->handler = _ws_handle_client;

      pthread_mutex_init(&t->loop_mutex, NULL);

      if (0 != pthread_create(&t->thread, NULL, _thread_loop, t)) {
        int err = errno;
        perror("pthread_create()");
        return err;
      }
    }
  }
  // end of threading things

  {
    FPX_DEBUG("fpx_http is listening on %s:%hu\n",
              inet_ntoa(srvptr->_internal->addr_4.sin_addr),
              ntohs(srvptr->_internal->addr_4.sin_port));
  }

  while (TRUE) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client = accept(meta->socket_4, (struct sockaddr *)&client_addr,
                        &client_addr_len);

    {
      FPX_DEBUG("connection accepted from %s:%hu\n",
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }

    int current_lowest = CLIENTS_DEFAULT;
    struct _thread *lowest_thread = NULL;

    for (int i = 0; i < srvptr->http_thread_count; ++i) {
      struct _thread *t = &meta->http_threads[i];

      pthread_mutex_lock(&t->loop_mutex);

      if (t->connection_count < current_lowest) {
        current_lowest = t->connection_count;
        lowest_thread = t;
      }

      pthread_mutex_unlock(&t->loop_mutex);
    }

    if (current_lowest >= CLIENTS_DEFAULT) {
      // server full; respond with 503 Service Unavailable
      fpx_httpresponse_t res;
      fpx_httpresponse_init(&res);
      _apply_default_headers(srvptr, &res);
      SET_HTTP_503(srvptr, res, "", 0);
      fpx_httpresponse_add_header(&res, "retry-after", "60");
      if (-10 == _send_response(NULL, &res, client)) {
        // broken pipe
      }

      FPX_WARN("Server full; connection refused\n");
      continue;
    }

    pthread_mutex_lock(&lowest_thread->loop_mutex);

    if (0 == _add_client_to_thread(lowest_thread, client))
      pthread_cond_signal(&lowest_thread->loop_condition);

    pthread_mutex_unlock(&lowest_thread->loop_mutex);
  }

  FPX_ERROR("wtf");

  return 0;
}

int fpx_httpserver_close(fpx_httpserver_t *srvptr) {
  SRV_ASSERT(srvptr);
  if (FALSE == srvptr->_internal->is_listening)
    return -3;

  srvptr->_internal->is_listening = FALSE;

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
 * - return -4 if the given `out_len` is too little. `copied_count` will then be
 * set to the required length
 */
static int _sanitize_headers(const char *in, char *out, size_t out_len,
                             size_t *copied_count) {
  if (NULL == in || NULL == out || NULL == copied_count)
    return -1;

  char *temp_buffer;
  char *temp_copy;

  int headers_len = fpx_getstringlength(in);

  int key_index;
  int value_index;
  int headers_index = 0;

  temp_buffer = (char *)malloc(headers_len);
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
    while (in[headers_index] > ' ' && in[headers_index] <= '~' &&
           in[headers_index] != ':')
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

    headers_index++; // skip past the colon

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

  if ((size_t)new_headers_len > out_len) {
    free(temp_buffer);
    return -4;
  }

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
static int _server_init_check(fpx_httpserver_t *srvptr) {
  if (NULL == srvptr)
    return -1;

  if (NULL == srvptr->_internal)
    return -2;

  return 0;
}

static int _available_endpoint_index(fpx_httpserver_t *srvptr,
                                     const char *uri) {
  SRV_ASSERT(srvptr);

  struct _fpx_httpserver_metadata *meta = srvptr->_internal;

  int i = 0;

  for (; i < srvptr->max_endpoints; ++i) {
    struct _fpx_endpoint *endp = &meta->endpoints[i];
    if (FALSE == endp->active || (0 == strcmp(uri, endp->uri)))
      break;
  }

  return i;
}

static int _fix_endpoint_uri(const char *input, char *output, size_t maxlen) {

  int uri_len = fpx_getstringlength(input);

  char *output_clone = output;

  output[0] = '/';
  if (*input != '/') {
    ++output_clone;
    uri_len += 1;
  }

  if ((size_t)uri_len > maxlen)
    return -4;

  fpx_memcpy(output_clone, input, uri_len - (output_clone - output));

  return 0;
}

static int _endpoint_get(fpx_httpserver_t *srvptr, const char *uri) {
  SRV_ASSERT(srvptr);

  if (NULL == uri)
    return -1;

  struct _fpx_httpserver_metadata *meta = srvptr->_internal;

  char the_uri[sizeof(meta->endpoints->uri)] = {0};

  {
    int fix_status = _fix_endpoint_uri(uri, the_uri, sizeof(the_uri) - 1);
    if (0 > fix_status)
      return fix_status;
  }

  int available_endpoint = _available_endpoint_index(srvptr, the_uri);

  if (available_endpoint >= srvptr->max_endpoints)
    return -3;

  struct _fpx_endpoint *ptr = &meta->endpoints[available_endpoint];
  if (FALSE == ptr->active) {
    fpx_memset(ptr, 0, sizeof(*ptr));
    fpx_strcpy(ptr->uri, the_uri);
    meta->active_endpoints++;
  }

  return available_endpoint;
}

static void _thread_init(struct _thread *t) {
  {
    int bytes = CLIENTS_DEFAULT * sizeof(struct pollfd);
    t->pfds = (struct pollfd *)malloc(bytes);
    fpx_memset(t->pfds, -1, bytes);
  }

  {
    int bytes = CLIENTS_DEFAULT * sizeof(struct _client_meta);
    t->client_data = (struct _client_meta *)malloc(bytes);
    fpx_memset(t->client_data, -1, bytes);
  }

  return;
}

static void *_thread_loop(void *tp) {
  struct _thread *t = (struct _thread *)tp;

  _thread_init(t);

  while (t->server->_internal->is_listening) {
    usleep(50); /* NOTE: i hate this <3 */         \
                /* but it needs to be done */      \
                /* because otherwise it regrabs */ \
                /* the mutex too quickly */        \
                                                   \
    /* loop logic start */
    pthread_mutex_lock(&t->loop_mutex);

    if (t->connection_count < 1)
      pthread_cond_wait(&t->loop_condition, &t->loop_mutex);

    {
      // if http_thread, check for keepalive
      if (t->thread_type == HttpThread) {
        for (int i = 0; i < t->connection_count; ++i) {
          if ((time(NULL) - t->client_data[i].last_time) >
              t->server->keepalive_timeout) {
            FPX_DEBUG("Keepalive timeout reached\n");
            _disconnect_client(t, i, TRUE);
          }
        }
      }

      if (t->thread_type == WebSocketsThread) {
        for (int i = 0; i < t->connection_count; ++i) {
          if ((time(NULL) - t->client_data[i].last_time) >
              t->server->websockets_timeout) {
            FPX_DEBUG("WebSocket timeout reached\n");
            _disconnect_client(t, i, TRUE);
          }
        }
      }

      // do things
      int available_clients;
#if defined(_WIN32) || defined(_WIN64)
      available_clients = WSAPoll(t->pfds, t->connection_count, 1000);
#else
      available_clients = poll(t->pfds, t->connection_count, 1000);
#endif

      for (int i = 0, j = 0; i < t->connection_count && j < available_clients;
           ++i) {
        if (0 == t->pfds[i].revents)
          continue;

        if (t->pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
          // client disconnect
          _disconnect_client(t, i, TRUE);
        } else if (t->pfds[i].revents & POLLIN) {
          // client has written
          t->handler(t, i); // handler is either for HTTP or WebSockets
        }

        ++j;
      }
    }

    pthread_mutex_unlock(&t->loop_mutex);
  }

  free(t->pfds);
  free(t->client_data);

  return NULL;
}

static int _add_client_to_thread(struct _thread *t, int client_fd) {
  if (NULL == t)
    return -1;

  if (0 > client_fd)
    return -2;

  if (CLIENTS_DEFAULT <= t->connection_count)
    return -3;

  struct pollfd *pfd = &t->pfds[t->connection_count];
  struct _client_meta *client_meta = &t->client_data[t->connection_count];

  fpx_memset(pfd, 0, sizeof(struct pollfd));
  pfd->fd = client_fd;
  pfd->events = POLLIN;

  fpx_memset(client_meta, 0, sizeof(*client_meta));

  time(&client_meta->last_time);

  return t->connection_count++;
}

static void _on_response_ready(struct _thread *thread,
                               fpx_httprequest_t *reqptr,
                               fpx_httpresponse_t *resptr, int idx) {
  char method[16] = {0};

  switch (reqptr->method) {
  case HTTP_NONE:
    fpx_strcpy(method, "NONE");
    break;
  case HTTP_OPTIONS:
    fpx_strcpy(method, "OPTIONS");
    break;
  case HTTP_GET:
    fpx_strcpy(method, "GET");
    break;
  case HTTP_HEAD:
    fpx_strcpy(method, "HEAD");
    break;
  case HTTP_POST:
    fpx_strcpy(method, "POST");
    break;
  case HTTP_PUT:
    fpx_strcpy(method, "PUT");
    break;
  case HTTP_PATCH:
    fpx_strcpy(method, "PATCH");
    break;
  case HTTP_DELETE:
    fpx_strcpy(method, "DELETE");
    break;
  case HTTP_CONNECT:
    fpx_strcpy(method, "CONNECT");
    break;
  case HTTP_TRACE:
    fpx_strcpy(method, "TRACE");
    break;

  case HTTP_ERROR:
    fpx_strcpy(method, "ERROR");
    break;
  }

  /* debug print */
  FPX_DEBUG("%hu | %s | %s | HTTP/%s\n", resptr->status, method, reqptr->uri,
            reqptr->content.version);

  uint16_t s = resptr->status;
  uint8_t add_close_header = (s == 429);

  uint8_t closing = add_close_header;

  {
    char header_val[32] = {0};
    if (-2 == fpx_httpresponse_get_header(resptr, "connection", header_val,
                                          sizeof(header_val) - 1)) {
      // there is no connection header set in the response yet, so we do so now
      if (add_close_header)
        fpx_httpresponse_add_header(resptr, "connection", "close");
      else {
        fpx_httpresponse_add_header(resptr, "connection", "keep-alive");
        fpx_httpresponse_add_header(resptr, "keep-alive",
                                    STR(KEEPALIVE_SECONDS));
      }
    } else {
      closing = (0 == strcmp(header_val, "close"));
    }
  }

  if (-10 == _send_response(reqptr, resptr, thread->pfds[idx].fd)) {
    // broken pipe
    closing = TRUE;
  }

  time(&thread->client_data[idx].last_time);

  if (closing) {
    FPX_DEBUG("Disconnecting client %d in thread %lu\n", idx, thread->thread);
    _disconnect_client(thread, idx, TRUE);
  }

  fpx_httprequest_destroy(reqptr);
  fpx_httpresponse_destroy(resptr);

  return;
}

static void _handle_http_endpoint(struct _thread *thread,
                                  fpx_httprequest_t *reqptr,
                                  fpx_httpresponse_t *resptr) {
  // check for endpoints
  struct _fpx_endpoint *endpoint = NULL;

  for (int i = 0;
       i < thread->server->_internal->active_endpoints && NULL == endpoint;
       ++i) {
    struct _fpx_endpoint *endp = &thread->server->_internal->endpoints[i];

    if (0 == strncmp(endp->uri, reqptr->uri, sizeof(endp->uri))) {
      // endpoint found
      endpoint = endp;
      break;
    }
  }

  // check wether to use the default endpoint
  if (FALSE != thread->server->_internal->default_endpoint.active &&
      NULL == endpoint) {
    endpoint = &thread->server->_internal->default_endpoint;
  }

  if (NULL == endpoint || NULL == endpoint->http_callback) {
    // 404
    SET_HTTP_404(thread->server, (*resptr), "", 0);
  } else if (!(endpoint->allowed_methods & reqptr->method)) {
    // 405 | method not allowed
    // TODO: add allowed methods into the response body
    SET_HTTP_405(thread->server, (*resptr), "", 0);
  } else {
    // :thumbsup:
    SET_HTTP_200(thread->server, (*resptr), "", 0);
    endpoint->http_callback(reqptr, resptr);
  }

  return;
}

static void _set_keepalive(fpx_httprequest_t *reqptr,
                           fpx_httpresponse_t *resptr) {
  uint8_t keepalive = TRUE;
  int return_value;

  // this is never read afterwards,
  // so we'll reuse it later
  char header_val[32] = {0};

  {
    char version[8] = {0};
    fpx_httprequest_get_version(reqptr, version, sizeof(version) - 1);
    if (0 == strcmp(version, "1.0"))
      keepalive = FALSE;
  }

  return_value = fpx_httprequest_get_header(reqptr, "connection", header_val,
                                            sizeof(header_val) - 1);
  fpx_string_to_lower(header_val, FALSE);

  if (0 == return_value) {
    if (0 == strncmp(header_val, "close", fpx_getstringlength(header_val))) {
      // connection close
      keepalive = FALSE;
    } else if (0 == strncmp(header_val, "keep-alive",
                            fpx_getstringlength(header_val))) {
      // we keep the connection open ^^
      keepalive = TRUE;
    }
  }

  return_value = fpx_httpresponse_get_header(resptr, "connection", header_val,
                                             sizeof(header_val) - 1);

  if (-2 == return_value)
    fpx_httpresponse_add_header(
        resptr, "connection", (TRUE == keepalive) ? ("keep-alive") : ("close"));

  return_value = fpx_httpresponse_get_header(resptr, "keep-alive", header_val,
                                             sizeof(header_val) - 1);

  if (-2 == return_value && TRUE == keepalive)
    fpx_httpresponse_add_header(resptr, "keep-alive", STR(KEEPALIVE_SECONDS));

  return;
}

static int _is_upgradable(fpx_httprequest_t *reqptr) {

  // must be GET method
  if (HTTP_GET != reqptr->method)
    return FALSE;

  // must be HTTP/1.1
  if (0 != memcmp(reqptr->content.version, "1.1", 3))
    return FALSE;

  // check for headers
  {
    // way oversized buffer just in case
    char header_val[256] = {0};

    fpx_httprequest_get_header(reqptr, "connection", header_val,
                               sizeof(header_val) - 1);
    fpx_string_to_lower(header_val, FALSE);
    if (-1 == fpx_substringindex(header_val, "upgrade"))
      return FALSE;

    fpx_memset(header_val, 0, sizeof(header_val));

    fpx_httprequest_get_header(reqptr, "upgrade", header_val,
                               sizeof(header_val) - 1);
    fpx_string_to_lower(header_val, FALSE);
    if (-1 == fpx_substringindex(header_val, "websocket"))
      return FALSE;

    fpx_memset(header_val, 0, sizeof(header_val));

    fpx_httprequest_get_header(reqptr, "sec-websocket-version", header_val,
                               sizeof(header_val) - 1);
    if (13 != fpx_strint(header_val))
      return FALSE;
  }

  return TRUE;
}

static void _upgrade_to_ws(struct _thread *thread, int idx,
                           fpx_websocketcallback_t ws_cb) {
  fpx_httpserver_t *srv = thread->server;
  struct _thread *ws_thread = srv->_internal->ws_threads;

  int lowest_count = CLIENTS_DEFAULT;
  struct _thread *lowest_thread = NULL;

  for (int i = 0; i < srv->ws_thread_count; ++i, ++ws_thread) {
    pthread_mutex_lock(&ws_thread->loop_mutex);

    if (ws_thread->connection_count < lowest_count) {
      lowest_count = ws_thread->connection_count;
      lowest_thread = ws_thread;
    }

    pthread_mutex_unlock(&ws_thread->loop_mutex);
  }

  pthread_mutex_lock(&lowest_thread->loop_mutex);

  int new_index = _add_client_to_thread(lowest_thread, thread->pfds[idx].fd);
  if (0 == new_index)
    pthread_cond_signal(&lowest_thread->loop_condition);

  lowest_thread->client_data[new_index] = thread->client_data[idx];
  lowest_thread->client_data[new_index].ws.callback = ws_cb;

  pthread_mutex_unlock(&lowest_thread->loop_mutex);

  _disconnect_client(thread, idx, FALSE);

  return;
}

static int _generate_ws_accept_header(fpx_httprequest_t *reqptr,
                                      char outbuf[32]) {
  char key_header[128] = {0};
  int key_len = 24;

  uint8_t sha_intermediate[20];

  char *accept_header;

  if (0 > fpx_httprequest_get_header(reqptr, "sec-websocket-key", key_header,
                                     key_len))
    return -1;

  fpx_strcpy(&key_header[key_len], "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
  fpx_sha1_digest((uint8_t *)key_header, fpx_getstringlength(key_header),
                  sha_intermediate, FALSE);
  accept_header = fpx_base64_encode(sha_intermediate, 20);

  if (NULL == accept_header) {
    // mem allocation failed
    return -10;
  }

  fpx_memcpy(outbuf, accept_header, 28);
  outbuf[28] = 0;

  free(accept_header);

  return 0;
}

// static int _check_manual_ws_upgrade(fpx_httprequest_t* reqptr,
// fpx_httpresponse_t* resptr) {
//   UNUSED(reqptr);
//   {
//     if (101 != resptr->status)
//       return 0;
//   }
//
//   return 1;
// }

static int _http_handle_client(struct _thread *thread, int idx) {
  // we enter this function assuming the client-socket is ready to be read from

  char read_buffer[BUFFER_DEFAULT] = {0};

  // read from socket
  int fd = thread->pfds[idx].fd;
  int amount_read = recv(fd, read_buffer, sizeof(read_buffer), 0);
  if (1 > amount_read) {
    _disconnect_client(thread, idx, TRUE);
    return 0;
  }

  fpx_httprequest_t incoming_request = {0};
  fpx_httpresponse_t outgoing_response = {0};

  fpx_httprequest_init(&incoming_request);

  fpx_httpresponse_init(&outgoing_response);
  _apply_default_headers(thread->server, &outgoing_response);
  fpx_httpresponse_set_version(&outgoing_response, STR(HTTP_VERSION));

  int parse_result =
      _http_parse_request(read_buffer, amount_read, &incoming_request);

  if (incoming_request.content.version[0] != '1') {
    const char msg[] = "Only HTTP/1.X is supported";
    SET_HTTP_505(thread->server, outgoing_response, msg, sizeof(msg) - 1);

    _on_response_ready(thread, &incoming_request, &outgoing_response, idx);

    return 0;
  }

  switch (parse_result) {
  case -1:
    // nullptrs passed
    break;
  case -2:
    // bad
    SET_HTTP_400(thread->server, outgoing_response, "", 0);
    break;

  case -3:
    // no content-length header was passed
    {
      switch (incoming_request.method) {
      case HTTP_POST:
      case HTTP_PUT:
      case HTTP_PATCH:
        // HTTP 411: Length Required
        SET_HTTP_411(thread->server, outgoing_response, "", 0);
        break;

      default:
        break;
      }
    }
    break;
  }

  uint8_t ws_upgrade = FALSE;
  fpx_websocketcallback_t ws_callback = NULL;

  if (_is_upgradable(&incoming_request)) {
    // TODO: Fix this; it is very smelly code
    // to the point where it feels like a hack
    // even though it isn't

    int endpoint_index = thread->server->max_endpoints;
    for (int i = 0; i < thread->server->max_endpoints; ++i) {
      struct _fpx_endpoint *endp = &thread->server->_internal->endpoints[i];

      if (!strcmp(incoming_request.uri, endp->uri)) {
        endpoint_index = i;
        break;
      }

      if (FALSE == endp->active)
        break;
    }

    if (endpoint_index < thread->server->max_endpoints) {
      // WS endpoint available, so we can upgrade
      char accept_header[32] = {0};

      ws_callback =
          thread->server->_internal->endpoints[endpoint_index].ws_callback;

      if (0 > _generate_ws_accept_header(&incoming_request, accept_header))
        return -10;

      outgoing_response.status = 101;
      fpx_strcpy(outgoing_response.reason, "Switching Protocols");

      fpx_httpresponse_add_header(&outgoing_response, "sec-websocket-accept",
                                  accept_header);
      fpx_httpresponse_add_header(&outgoing_response, "upgrade", "websocket");
      fpx_httpresponse_add_header(&outgoing_response, "connection", "Upgrade");

      ws_upgrade = TRUE;
    } else {
      SET_HTTP_404(thread->server, outgoing_response, "", 0);

      ws_upgrade = FALSE;
    }
  }

  if (outgoing_response.status > 0) {
    // that means we already set a response!
    // specifically for a bad request or similar

    // now we send it!

    _on_response_ready(thread, &incoming_request, &outgoing_response, idx);

    if (TRUE == ws_upgrade) {
      _upgrade_to_ws(thread, idx, ws_callback);
    }

    return 0;
  }

  // do endpoint things
  // (this includes running the programmer's callback)
  _handle_http_endpoint(thread, &incoming_request, &outgoing_response);

  // keepalive
  _set_keepalive(&incoming_request, &outgoing_response);

  _on_response_ready(thread, &incoming_request, &outgoing_response, idx);

  return 0;
}

static int _disconnect_client(struct _thread *t, int idx,
                              uint8_t close_socket) {
  if (TRUE == close_socket) {
    FPX_DEBUG("Closing fd: %d\n", t->pfds[idx].fd);
    close(t->pfds[idx].fd);
  }

  for (int i = idx; i < (t->connection_count - 1); ++i) {
    t->pfds[i] = t->pfds[i + 1];
    t->client_data[i] = t->client_data[i + 1];
  }

  fpx_memset(&t->pfds[t->connection_count - 1], -1, sizeof(t->pfds[0]));
  fpx_memset(&t->client_data[t->connection_count - 1], -1,
             sizeof(t->client_data[0]));

  t->connection_count--;

  return 0;
}

static int _http_parse_request_line(const char *reql,
                                    fpx_httprequest_t *reqptr) {
  int reqline_len;
  const char *reql_start = reql;

  reqline_len = fpx_substringindex(reql, "\r\n") + 2 /*2 = CRLF*/;

  fpx_httpmethod_t method;
  if (0 == strncmp(reql, "GET ", 4)) {
    method = HTTP_GET;
    reql += 4;
  } else if (0 == strncmp(reql, "POST ", 5)) {
    method = HTTP_POST;
    reql += 5;
  } else if (0 == strncmp(reql, "OPTIONS ", 8)) {
    method = HTTP_OPTIONS;
    reql += 8;
  } else if (0 == strncmp(reql, "HEAD ", 5)) {
    method = HTTP_HEAD;
    reql += 5;
  } else if (0 == strncmp(reql, "DELETE ", 7)) {
    method = HTTP_DELETE;
    reql += 7;
  } else if (0 == strncmp(reql, "PUT ", 4)) {
    method = HTTP_PUT;
    reql += 4;
  } else if (0 == strncmp(reql, "TRACE ", 6)) {
    method = HTTP_TRACE;
    reql += 6;
  } else if (0 == strncmp(reql, "CONNECT ", 8)) {
    method = HTTP_CONNECT;
    reql += 8;
  } else
    method = HTTP_ERROR;

  fpx_httprequest_set_method(reqptr, method);

  if (method == HTTP_ERROR)
    return -2; // bad METHOD

  if (*reql != '/')
    return -3; // bad URI

  {
    char *temp_reql = (char *)reql;

    int uri_len = fpx_substringindex(reql, " ");
    temp_reql[uri_len] = 0;
    if (0 < fpx_httprequest_set_uri(reqptr, reql))
      return -4; // URI too long

    temp_reql[uri_len] = ' ';

    reql += uri_len + 1;
  }

  if (0 != strncmp(reql, "HTTP/", 5))
    return -5; // bad VERSION

  reql += 5;

  int ver_len = fpx_substringindex(reql, "\r\n");
  if (0 > ver_len)
    return -5;

  char ver[8] = {0};
  fpx_memset(ver, 0, sizeof(ver));
  fpx_memcpy(ver, reql, ver_len);

  if (0 < fpx_httprequest_set_version(reqptr, ver))
    return -6; // VERSION too long

  reql += ver_len + 2;

  if (reql - reql_start != reqline_len)
    return -7; // something went horribly wrong

  return reqline_len;
}

static int _http_parse_request(char *read_buffer, int read_buffer_length,
                               fpx_httprequest_t *reqptr) {
  char *req_line = NULL;
  char *headers = NULL;
  char *body = NULL;

  req_line = read_buffer;
  for (int i = 0; i < read_buffer_length && req_line[i] == ' '; ++i, ++req_line)
    ;

  if (!(*req_line == 'O' || // OPTIONS
        *req_line == 'G' || // GET
        *req_line == 'H' || // HEAD
        *req_line == 'P' || // POST, PUT
        *req_line == 'D' || // DELETE
        *req_line == 'T' || // TRACE
        *req_line == 'C'    // CONNECT
        ))
    return -2;

  // parse request line into request object
  {
    int req_line_len = _http_parse_request_line(req_line, reqptr);

    if (req_line_len < 0)
      return -2;

    headers = &read_buffer[req_line_len];
  }

  int headers_total_len = fpx_substringindex(headers, "\r\n\r\n");

  if (headers_total_len > MAX_HEADERS - 1) {
    // HTTP 431 | request header fields too large
  }

  // separate key from value
  // and store inside of request object
  {
    char *current_header = headers;

    while (TRUE) {
      int colon_index = fpx_substringindex(current_header, ":");
      int crlf_index = fpx_substringindex(current_header, "\r\n");
      if (colon_index > crlf_index || colon_index == -1)
        break; // no more headers left

      current_header[colon_index] = 0;
      current_header[crlf_index] = 0;

      if (0 > fpx_httprequest_add_header(reqptr, current_header,
                                         current_header + colon_index + 1))
        return -2;

      current_header[colon_index] = ':'; // set back to colon
      current_header[crlf_index] = '\r'; // set back to CR

      current_header += crlf_index + 2;
    }

    body = current_header + 2;
  }

  char content_length_value[32];
  if (-2 == fpx_httprequest_get_header(reqptr, "content-length",
                                       content_length_value,
                                       sizeof(content_length_value))) {
    // no content-length header in POST request body
    // so we just return now
    return -3;
  }

  // now copy body over
  fpx_httprequest_append_body(reqptr, body, fpx_strint(content_length_value));

  return 0;
}

static int _apply_default_headers(fpx_httpserver_t *srvptr,
                                  fpx_httpresponse_t *resptr) {
  char *def_headers = srvptr->_internal->default_headers;

  char key[32];
  char value[512];

  while (*def_headers != 0) {
    int colon = fpx_substringindex(def_headers, ":");
    int crlf = fpx_substringindex(def_headers, "\r\n");

    if (colon > crlf || -1 == colon)
      break;

    fpx_memset(key, 0, sizeof(key));
    fpx_memset(value, 0, sizeof(value));

    fpx_memcpy(key, def_headers, colon);
    fpx_memcpy(value, def_headers + colon + 1, crlf - (colon + 1));

    int result = fpx_httpresponse_add_header(resptr, key, value);
    if (result != 0)
      return result;

    def_headers += crlf + 2;
  }

  return 0;
}

static int _send_response(fpx_httprequest_t *reqptr, fpx_httpresponse_t *resptr,
                          int client_fd) {
  if (NULL == resptr)
    return -1;

  if (client_fd < 0)
    return -2;

  char write_buffer[BUFFER_DEFAULT] = {0};
  char *write_copy = write_buffer;
  fpx_memset(write_buffer, 0, sizeof(write_buffer));

// to ignore SIGPIPE when writing to a disconnected client
#if defined(_WIN32) || defined(_WIN64)
  int send_flags = 0;
#else
  int send_flags = MSG_NOSIGNAL;
#endif

  // some prerequisites (content-length header etc.)
  {
    char content_length_header[32] = {0};
    if (-2 == fpx_httpresponse_get_header(resptr, "content-length",
                                          content_length_header,
                                          sizeof(content_length_header))) {
      // header not yet set, so we set it
      fpx_intstr(resptr->content.body_len, content_length_header);
      fpx_httpresponse_add_header(resptr, "content-length",
                                  content_length_header);
    }
  }

  // response status line
  {
    char version[16] = {0};
    fpx_httpresponse_get_version(resptr, version, sizeof(version));

    write_copy += snprintf(write_copy, 1024, "HTTP/%s %hu %s\r\n", version,
                           resptr->status, resptr->reason);
  }

  // headers
  {
    fpx_memcpy(write_copy, resptr->content.headers,
               resptr->content.headers_len);
    write_copy += resptr->content.headers_len;

    fpx_memcpy(write_copy, "\r\n", 2);
    write_copy += 2;
  }

  int sent_success = 0;

  // body
  if (resptr->content.body_len > 0 &&
      (NULL == reqptr || FALSE == (reqptr->method & HTTP_HEAD))) {
    int content_written = 0;

    int content_length;
    char content_length_stringvalue[32] = {0};

    fpx_httpresponse_get_header(resptr, "content-length",
                                content_length_stringvalue,
                                sizeof(content_length_stringvalue));
    content_length = fpx_strint(content_length_stringvalue);

    while (content_written < content_length) {

      int to_write = (content_length - content_written);
      int remaining_space =
          (sizeof(write_buffer) - (write_copy - write_buffer));
      if (to_write > remaining_space)
        to_write = remaining_space;

      fpx_memcpy(write_copy, resptr->content.body + content_written, to_write);
      write_copy += to_write;
      content_written += to_write;

      sent_success =
          send(client_fd, write_buffer, write_copy - write_buffer, send_flags);

      write_copy = write_buffer;
    }
  } else {
    sent_success =
        send(client_fd, write_buffer, write_copy - write_buffer, send_flags);
  }

  if (-1 == sent_success) {
    if (errno == EPIPE)
      return -10;
  }

  return 0;
}

static int _ws_frame_validate(fpx_websocketframe_t *incoming_frame,
                              fpx_websocketclient_t *cliptr, int fd) {
  if (FALSE == incoming_frame->mask_set) {
    const uint8_t msg[] = "no mask set";
    int sent = fpx_websocket_send_close(fd, 1002, msg, sizeof(msg) - 1, FALSE);

    if (0 != sent) {
      FPX_ERROR("Received an unmasked websocket frame from a client, but could "
                "not send a closing frame.\tErrno: %d",
                sent);
    } else {
      cliptr->flags |= CLOSE_SENT;
    }

    return FALSE;
  }

  if (incoming_frame->reserved1 || incoming_frame->reserved2 ||
      incoming_frame->reserved3) {
    const uint8_t msg[] = "no extensions negotiated";
    int sent = fpx_websocket_send_close(fd, 1002, msg, sizeof(msg) - 1, FALSE);

    if (0 != sent) {
      FPX_ERROR("Received a websocket frame with one or more reserved bits set "
                "while but no extensions were negotiated, and failed to send a "
                "closing frame.\tErrno: %d",
                sent);
    } else {
      cliptr->flags |= CLOSE_SENT;
    }

    return FALSE;
  }

  return TRUE;
}

static void _handle_control_frame(fpx_websocketframe_t *incoming_frame,
                                  fpx_websocketclient_t *cliptr, int fd) {
  switch (incoming_frame->opcode) {
  case WEBSOCKET_CLOSE:
    // close

    cliptr->flags |= CLOSE_RECV;

    if (FALSE == (cliptr->flags & CLOSE_SENT)) {
      fpx_websocketframe_t close_frame;

      fpx_websocketframe_init(&close_frame);

      close_frame.final = TRUE;
      close_frame.opcode = WEBSOCKET_CLOSE;

      fpx_websocketframe_append_payload(&close_frame, incoming_frame->payload,
                                        incoming_frame->payload_length);

      int sent_status = fpx_websocketframe_send(&close_frame, fd);
      if (sent_status > 0) {
        // error happened
        if (sent_status == EPIPE)
          FPX_WARN("Broken client pipe\n");
        else
          perror("fpx_websocketframe_send()");
      }

      fpx_websocketframe_destroy(&close_frame);
    }
    break;

  case WEBSOCKET_PING:
    // ping

    {
      fpx_websocketframe_t pong_frame;
      fpx_websocketframe_init(&pong_frame);

      pong_frame.final = TRUE;
      pong_frame.opcode = WEBSOCKET_PONG;

      fpx_websocketframe_append_payload(&pong_frame, incoming_frame->payload,
                                        incoming_frame->payload_length);

      int sent_status = fpx_websocketframe_send(&pong_frame, fd);
      if (sent_status > 0) {
        // error happened
        if (sent_status == EPIPE)
          FPX_WARN("Broken client pipe");
        else
          perror("fpx_websocketframe_send()");
      }

      fpx_websocketframe_destroy(&pong_frame);
    }
    break;

  case WEBSOCKET_PONG:
    // pong
    break;

  default:
    // ???? what
    break;
  }

  return;
}

static int _ws_handle_client(struct _thread *thread, int idx) {
  // we enter this function assuming the client socket is ready to be read from

  struct _client_meta *cliptr = &(thread->client_data[idx]);

  // read from socket
  int fd = thread->pfds[idx].fd;

  // READ READ READ
  // https://datatracker.ietf.org/doc/html/rfc6455#section-5.2

  fpx_websocketframe_t incoming_frame;
  fpx_websocketframe_init(&incoming_frame);

  int parse_result = _ws_parse_request(fd, &incoming_frame);
  if (parse_result == -1) {
    _disconnect_client(thread, idx, TRUE);
    return -1;
  }

  if (0 > parse_result) {
    const uint8_t *msg = NULL;
    switch (parse_result) {
    case -1:
      // too short
      msg = (uint8_t *)"frame too short";
      break;

    case -2:
      // unmasked
      msg = (uint8_t *)"client frame unmasked";
      break;

    case -3:
      msg = (uint8_t *)"incomplete frame received";
      break;
    }

    fpx_websocket_send_close(fd, 1002, msg, fpx_getstringlength((char *)msg),
                             FALSE);
  }

  if (FALSE == (cliptr->ws.flags & CLOSE_SENT)) {
    if (FALSE == _ws_frame_validate(&incoming_frame, &cliptr->ws, fd)) {
      fpx_websocketframe_destroy(&incoming_frame);
      return -1;
    }
  }

  if (0 > parse_result)
    return -1;

  time(&cliptr->last_time);

  for (uint64_t i = 0; i < incoming_frame.payload_length; ++i)
    incoming_frame.payload[i] ^= incoming_frame.masking_key[i % 4];

  if (FALSE == (incoming_frame.opcode & 0x80) &&
      FALSE == (cliptr->ws.flags & CLOSE_SENT)) {
    // NOT a control frame
    cliptr->ws.callback(&incoming_frame, fd, &(cliptr->in_address));
  } else if (125 >= incoming_frame.payload_length) {
    // 125 is max payload length for control frames
    _handle_control_frame(&incoming_frame, &cliptr->ws, fd);
  }

  if ((cliptr->ws.flags & (CLOSE_SENT | CLOSE_RECV)) ==
      (CLOSE_SENT | CLOSE_RECV))
    // we close
    _disconnect_client(thread, idx, TRUE);

  fpx_websocketframe_destroy(&incoming_frame);

  return 0;
}

// return 0 on success; -1 on passed nullptrs; -2 if the data is bad; -3 if data
// too long
static int _ws_parse_request(int fd, fpx_websocketframe_t *output) {
  uint8_t readbuf[BUFFER_DEFAULT] = {0};

  int minimum_length = 1 + // meta-byte
                       1 + // length-byte
                       4;  // masking key

  int amount_read = recv(fd, (char *)readbuf, sizeof(readbuf), 0);

  if (minimum_length > amount_read) {
    return -1;
  }

  output->final = (readbuf[0] >> 7) & 0x01; // MSB; 1 or 0
  output->reserved1 = (readbuf[0] >> 6) & 0x01;
  output->reserved2 = (readbuf[0] >> 5) & 0x01;
  output->reserved3 = (readbuf[0] >> 4) & 0x01;

  output->opcode = (readbuf[0] & 0xf);

  output->mask_set = (readbuf[1] >> 7) & 0x01;

  uint8_t length_first = readbuf[1] & 0x7f;

  if (FALSE == output->mask_set)
    return -2;

  uint8_t *readbuf_copy = readbuf + 2;
  uint64_t p_length;

  {
    // check payload length

    if (126 > length_first) {
      p_length = length_first;

    } else if (126 == length_first) {
      if (amount_read < minimum_length + 2)
        return -1;

      uint16_t extended = *((uint16_t *)readbuf_copy);
      fpx_endian_swap_if_little(&extended, sizeof(extended));
      p_length = extended;

      readbuf_copy += sizeof(extended);
    } else {
      if (amount_read < minimum_length + 8)
        return -1;

      uint64_t extended = *((uint64_t *)readbuf_copy);
      fpx_endian_swap_if_little(&extended, sizeof(extended));
      p_length = extended;

      readbuf_copy += sizeof(extended);
    }
  }

  fpx_memcpy(output->masking_key, readbuf_copy, sizeof(output->masking_key));
  readbuf_copy += 4;

  int length_so_far = readbuf_copy - readbuf;
  size_t left_to_read = p_length;

  int recv_size;
  do {
    fpx_websocketframe_append_payload(output, readbuf + length_so_far,
                                      amount_read - length_so_far);
    left_to_read -= (amount_read - length_so_far);

    length_so_far = 0;

    if (sizeof(readbuf) > left_to_read)
      recv_size = left_to_read;
    else
      recv_size = sizeof(readbuf);
  } while (0 < left_to_read &&
           -1 != (amount_read = recv(fd, (char *)readbuf, recv_size, 0)));

  if (0 < left_to_read)
    return -3;

  return 0;
}
