////////////////////////////////////////////////////////////////
//  "httpserver.h"                                            //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "httpserver_c.h"
#include "../../fpx_types.h"
#include "../netutils.h"
#include "../../fpx_debug.h"

#include <arpa/inet.h>
#include <bits/pthreadtypes.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>


// START OF FPXLIBC LINK-TIME DEPENDENCIES
#include "../../fpx_mem/mem.h"
#include "../../fpx_string/string.h"
#include "../../fpx_c-utils/format.h"
#include "http.h"
// END OF FPXLIBC LINK-TIME DEPENDENCIES

/* this is the default thread count for the HTTP or WS threads */
#define THREADS_DEFAULT 4

/* this is the default maximum amount of endpoints to be handled */
#define ENDPOINTS_DEFAULT 16

/* this is the default maximum amount of clients to handle per HTTP thread */
#define CLIENTS_DEFAULT 16

/* this is the default buffer size for HTTP reading and writing */
#define BUFFER_DEFAULT 16384

#define MAX_HEADERS 4096
#define MAX_BODY 8192

/* this is the HTTP version that will be used in responses */
#define HTTP_VERSION 1.1

/* this is the keepalive time to enforce on the server */
#define KEEPALIVE_SECONDS 7

#define SERVER_HEADER "fpx_http"

// returns out of the function this macro is placed in if
// the server pointer is not valid or the server is not initialized
#define SRV_ASSERT(ptr)              \
  {                                  \
    int r = _server_init_check(ptr); \
    if (r < 0)                       \
      return r;                      \
  }

// struct forward declarations
struct _http_client_metadata;
struct _http_thread;

// TODO: struct _websocket_client_metadata;
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

static int _fpx_http_add_client(struct _http_thread*, int client_fd);
static int _fpx_http_handle_client(struct _http_thread* thread, int idx);
static int _fpx_http_disconnect_client(struct _http_thread* thread, int idx);

static int _fpx_http_parse_request_line(const char*, fpx_httprequest_t* output);

// returns -2 on invalid request (400)
// returns -3 if no content-length was passed, and thus no body was read
static int _fpx_http_parse_request(char* readbuf, int readbuf_len, fpx_httprequest_t* output);

static int _fpx_apply_default_headers(fpx_httpserver_t* srvptr, fpx_httpresponse_t* resptr);

static int _fpx_send_response(struct _http_thread* threadptr, fpx_httpresponse_t* resptr, int client_id);


#define SET_HTTPRESPONSE_PRESET(srvptr, res, code, reason_text, body, body_len)       \
  (                                                                                   \
    0 == fpx_httpresponse_set_version(&res, STR(HTTP_VERSION)) &&                     \
    (res.status = code) &&                                                            \
    fpx_strcpy(res.reason, reason_text) &&                                            \
    0 == fpx_httpresponse_append_body(&res, body, body_len)                           \
  )

#define SET_HTTP_200(srvptr, res, body, body_len) \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 200, "OK", body, body_len)

#define SET_HTTP_400(srvptr, res, body, body_len) \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 400, "Bad Request", body, body_len)

#define SET_HTTP_404(srvptr, res, body, body_len) \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 404, "Not Found", body, body_len)

#define SET_HTTP_411(srvptr, res, body, body_len) \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 411, "Length Required", body, body_len)

#define SET_HTTP_500(srvptr, res, body, body_len) \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 500, "Internal Server Error", body, body_len)

#define SET_HTTP_503(srvptr, res, body, body_len) \
  SET_HTTPRESPONSE_PRESET(srvptr, res, 503, "Service Unavailable", body, body_len)


// start struct definitions
struct _fpx_httpendpoint {
    uint8_t active;

    char uri[256];
    uint16_t allowed_methods;

    fpx_httpcallback_t callback;
};

struct _http_client_metadata {
  time_t last_time;
};

struct _http_thread {
    fpx_httpserver_t* server;
    pthread_t thread;

    pthread_mutex_t loop_mutex;
    pthread_cond_t loop_condition;

    int connection_count;
    struct pollfd* pfds;
    struct _http_client_metadata* client_data;
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

  fpx_httpserver_set_default_headers(srvptr, "server: " SERVER_HEADER "\r\n");

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
  meta->active_endpoints++;

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

    {
      int one = 1;
      setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &one, sizeof(one));
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

    meta->is_listening = TRUE;
  }
  // end of socket things


  // start threading
  {
    meta->http_threads =
      (struct _http_thread*)calloc(meta->http_thread_count, sizeof(struct _http_thread));

    for (int i = 0; i < meta->http_thread_count; ++i) {
      struct _http_thread* t = &meta->http_threads[i];

      t->server = srvptr;

      if (0 != pthread_create(&t->thread, NULL, _fpx_http_loop, t)) {
        int err = errno;
        perror("pthread_create()");
        return err;
      }
    }
  }
  // end of threading things

  {
    char debug_line[64];
    int index = fpx_getstringlength(debug_line);

    struct sockaddr_in* addr = &srvptr->_metadata->addr_4;
    snprintf(debug_line, sizeof(debug_line), "fpx_http is listening on %s:%hu", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));

    FPX_DEBUG(debug_line);
  }

  while (TRUE) {
    int client = accept(meta->socket_4, NULL, NULL);

    int current_lowest = CLIENTS_DEFAULT;
    int lowest_thread = 0;

    for (int i = 0; i < meta->http_thread_count; ++i) {
      struct _http_thread* t = &meta->http_threads[i];
      pthread_mutex_lock(&t->loop_mutex);

      if (t->connection_count < current_lowest)
        current_lowest = t->connection_count;

      pthread_mutex_unlock(&t->loop_mutex);
    }

    if (current_lowest >= CLIENTS_DEFAULT) {
      // server full; respond with 503 Service Unavailable
      fpx_httpresponse_t res;
      fpx_httpresponse_init(&res);
      _fpx_apply_default_headers(srvptr, &res);
      SET_HTTP_503(srvptr, res, "", 0);
      fpx_httpresponse_add_header(&res, "retry-after", "60");
    }

    pthread_mutex_lock(&meta->http_threads[current_lowest].loop_mutex);

    if (0 == _fpx_http_add_client(&srvptr->_metadata->http_threads[current_lowest], client))
      pthread_cond_signal(&srvptr->_metadata->http_threads[current_lowest].loop_condition);

    pthread_mutex_unlock(&meta->http_threads[current_lowest].loop_mutex);
  }

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


static void* _fpx_http_loop(void* tp) {
  struct _http_thread* t = (struct _http_thread*)tp;

  {
    int bytes = CLIENTS_DEFAULT * sizeof(struct pollfd);
    t->pfds = (struct pollfd*)malloc(bytes);
    fpx_memset(t->pfds, -1, bytes);
  }

  {
    int bytes = CLIENTS_DEFAULT * sizeof(struct _http_client_metadata);
    t->client_data = (struct _http_client_metadata*)malloc(bytes);
    fpx_memset(t->client_data, -1, bytes);
  }

  while (t->server->_metadata->is_listening) {
    // loop logic
    pthread_mutex_lock(&t->loop_mutex);


    if (t->connection_count < 1)
      pthread_cond_wait(&t->loop_condition, &t->loop_mutex);


    // do things
    int available_clients;
    available_clients = poll(t->pfds, t->connection_count, 1000);


    for (int i = 0, j = 0; i < t->connection_count && j < available_clients; ++i) {
      if (0 == t->pfds[i].revents)
        continue;

      if (t->pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        // client disconnect
        _fpx_http_disconnect_client(t, i);
      } else if (t->pfds[i].revents & POLLIN) {
        // client has written
        _fpx_http_handle_client(t, i);
      }

      ++j;
    }


    pthread_mutex_unlock(&t->loop_mutex);
  }

  free(t->pfds);

  return NULL;
}


static void* _fpx_websocket_loop(void* tp) {
  return NULL;
}


static void* _http_killer(void* meta) {
  return NULL;
}


static int _fpx_http_add_client(struct _http_thread* t, int client_fd) {
  if (NULL == t)
    return -1;

  if (0 > client_fd)
    return -2;

  if (CLIENTS_DEFAULT <= t->connection_count)
    return -3;

  struct pollfd* pfd = &t->pfds[t->connection_count];
  struct _http_client_metadata* client_meta = &t->client_data[t->connection_count];

  fpx_memset(pfd, 0, sizeof(struct pollfd));
  pfd->fd = client_fd;
  pfd->events = POLLIN;

  time(&client_meta->last_time);

  t->connection_count++;

  return 0;
}


static int _fpx_http_handle_client(struct _http_thread* thread, int idx) {
  // we enter this function assuming the client-socket is ready to be read from

  time(&thread->client_data[idx].last_time);

  char read_buffer[BUFFER_DEFAULT] = { 0 };

  // read from socket
  struct pollfd* pfd = &thread->pfds[idx];
  int amount_read = recv(pfd->fd, read_buffer, sizeof(read_buffer), 0);
  if (1 > amount_read) {
    pthread_mutex_unlock(&thread->loop_mutex);
    _fpx_http_disconnect_client(thread, idx);
    pthread_mutex_lock(&thread->loop_mutex);
    return 0;
  }

  fpx_httprequest_t incoming_request = { 0 };
  fpx_httpresponse_t outgoing_response = { 0 };

  fpx_httprequest_init(&incoming_request);

  fpx_httpresponse_init(&outgoing_response);
  _fpx_apply_default_headers(thread->server, &outgoing_response);
  fpx_httpresponse_set_version(&outgoing_response, STR(HTTP_VERSION));

  int parse_result = _fpx_http_parse_request(read_buffer, amount_read, &incoming_request);

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
          case POST:
          case PUT:
          case PATCH:
            // HTTP 411: Length Required
            SET_HTTP_411(thread->server, outgoing_response, "", 0);
            break;

          default:
            break;
        }
      }
      break;
  }

  if (outgoing_response.status > 0) {
    // that means we already set a response!
    // now we send it
    _fpx_send_response(thread, &outgoing_response, idx);

    return 0;
  }


  {
    int endpoint_found = -1;

    for (int i = 0; i < thread->server->_metadata->active_endpoints && endpoint_found < 0; ++i) {
      struct _fpx_httpendpoint* endp = &thread->server->_metadata->endpoints[i];

      if (0 == strncmp(endp->uri, incoming_request.uri, sizeof(endp->uri))) {
        endpoint_found = i;
        break;
      }
    }

    if (endpoint_found < 0)
      SET_HTTP_404(thread->server, outgoing_response, "", 0);
    else {
      SET_HTTP_200(thread->server, outgoing_response, "", 0);
      thread->server->_metadata->endpoints[endpoint_found].callback(&incoming_request, &outgoing_response);
    }
  }

  _fpx_send_response(thread, &outgoing_response, idx);


  return 0;
}


static int _fpx_http_disconnect_client(struct _http_thread* t, int idx) {
  for (int i = idx; i < (t->connection_count - 1); ++i) {
    t->pfds[i] = t->pfds[i + 1];
    t->client_data[i] = t->client_data[i + 1];
  }

  fpx_memset(&t->pfds[t->connection_count], -1, sizeof(t->pfds[0]));
  fpx_memset(&t->client_data[t->connection_count], -1, sizeof(t->client_data[0]));

  t->connection_count--;

  return 0;
}


static int _fpx_http_parse_request_line(const char* reql, fpx_httprequest_t* reqptr) {
  int reqline_len;
  const char* reql_start = reql;

  reqline_len = fpx_substringindex(reql, "\r\n") + 2 /*2 = CRLF*/;


  fpx_httpmethod_t method;
  if (0 == strncmp(reql, "GET ", 4)) {
    method = GET;
    reql += 4;
  } else if (0 == strncmp(reql, "POST ", 5)) {
    method = POST;
    reql += 5;
  } else if (0 == strncmp(reql, "OPTIONS ", 8)) {
    method = OPTIONS;
    reql += 8;
  } else if (0 == strncmp(reql, "HEAD ", 5)) {
    method = HEAD;
    reql += 5;
  } else if (0 == strncmp(reql, "DELETE ", 7)) {
    method = DELETE;
    reql += 7;
  } else if (0 == strncmp(reql, "PUT ", 4)) {
    method = PUT;
    reql += 4;
  } else if (0 == strncmp(reql, "TRACE ", 6)) {
    method = TRACE;
    reql += 6;
  } else if (0 == strncmp(reql, "CONNECT ", 8)) {
    method = CONNECT;
    reql += 8;
  } else
    method = ERROR;

  fpx_httprequest_set_method(reqptr, method);


  if (method == ERROR)
    return -2;  // bad METHOD

  if (*reql != '/')
    return -3;  // bad URI

  {
    char* temp_reql = (char*)reql;

    int uri_len = fpx_substringindex(reql, " ");
    temp_reql[uri_len] = 0;
    if (0 < fpx_httprequest_set_uri(reqptr, reql))
      return -4;  // URI too long

    temp_reql[uri_len] = ' ';

    reql += uri_len + 1;
  }

  if (0 != strncmp(reql, "HTTP/", 5))
    return -5;  // bad VERSION

  reql += 5;

  int ver_len = fpx_substringindex(reql, "\r\n");
  if (0 > ver_len)
    return -5;

  char ver[8] = { 0 };
  fpx_memset(ver, 0, sizeof(ver));
  fpx_memcpy(ver, reql, ver_len);

  if (0 < fpx_httprequest_set_version(reqptr, ver))
    return -6;  // VERSION too long

  reql += ver_len + 2;

  if (reql - reql_start != reqline_len)
    return -7;  // something went horribly wrong


  return reqline_len;
}


static int _fpx_http_parse_request(
  char* read_buffer, int read_buffer_length, fpx_httprequest_t* reqptr) {
  char* req_line = NULL;
  char* headers = NULL;
  char* body = NULL;

  req_line = read_buffer;
  for (int i = 0; i < read_buffer_length && req_line[i] == ' '; ++i, ++req_line)
    ;

  if (!(*req_line == 'O' ||  // OPTIONS
        *req_line == 'G' ||  // GET
        *req_line == 'H' ||  // HEAD
        *req_line == 'P' ||  // POST, PUT
        *req_line == 'D' ||  // DELETE
        *req_line == 'T' ||  // TRACE
        *req_line == 'C'     // CONNECT
        ))
    return -2;

  // parse request line into request object
  {
    int req_line_len = _fpx_http_parse_request_line(req_line, reqptr);

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
    char* current_header = headers;

    while (TRUE) {
      int colon_index = fpx_substringindex(current_header, ":");
      int crlf_index = fpx_substringindex(current_header, "\r\n");
      if (colon_index > crlf_index || colon_index == -1)
        break;  // no more headers left

      current_header[colon_index] = 0;
      current_header[crlf_index] = 0;

      if (0 > fpx_httprequest_add_header(reqptr, current_header, current_header + colon_index + 1))
        return -2;

      current_header[colon_index] = ':';  // set back to colon
      current_header[crlf_index] = '\r';  // set back to CR

      current_header += crlf_index + 2;
    }

    body = current_header + 2;
  }

  char content_length_value[32];
  if (-2 == fpx_httprequest_get_header(reqptr, "content-length", content_length_value, sizeof(content_length_value))) {
    // no content-length header in POST request body
    // so we just return now
    return -3;
  }

  // now copy body over
  fpx_httprequest_append_body(reqptr, body, fpx_strint(content_length_value));

  return 0;
}


static int _fpx_apply_default_headers(fpx_httpserver_t* srvptr, fpx_httpresponse_t* resptr) {
  char* def_headers = srvptr->_metadata->default_headers;

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


static int _fpx_send_response(struct _http_thread* threadptr, fpx_httpresponse_t* resptr, int client_id) {
  if (NULL == resptr || NULL == threadptr)
    return -1;

  if (client_id < 0)
    return -2;

  char write_buffer[BUFFER_DEFAULT] = { 0 };
  char* write_copy = write_buffer;
  fpx_memset(write_buffer, 0, sizeof(write_buffer));

  // some prerequisites (content-length header etc.)
  {
    char content_length_header[32] = { 0 };
    if (-2 == fpx_httpresponse_get_header(resptr, "content-length", content_length_header, sizeof(content_length_header))) {
      // header not yet set, so we set it
      fpx_intstr(resptr->content.body_len, content_length_header);
      fpx_httpresponse_add_header(resptr, "content-length", content_length_header);
    }
  }

  // response status line
  {
    char version[16] = { 0 };
    fpx_httpresponse_get_version(resptr, version, sizeof(version));

    write_copy += snprintf(write_copy, 1024, "HTTP/%s %hu %s\r\n", version, resptr->status, resptr->reason);
  }

  // headers
  {
    fpx_memcpy(write_copy, resptr->content.headers, resptr->content.headers_len);
    write_copy += resptr->content.headers_len;

    fpx_memcpy(write_copy, "\r\n", 2);
    write_copy += 2;
  }

  // body
  {
    int content_length;
    char content_length_stringvalue[32] = { 0 };

    fpx_httpresponse_get_header(resptr, "content-length", content_length_stringvalue, sizeof(content_length_stringvalue));
    content_length = fpx_strint(content_length_stringvalue);

    fpx_memcpy(write_copy, resptr->content.body, content_length);
    write_copy += content_length;
  }

  // debug print for the response
  // FPX_DEBUG(write_buffer);

  send(threadptr->pfds[client_id].fd, write_buffer, write_copy - write_buffer, 0);

  return 0;
}
