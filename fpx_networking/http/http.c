////////////////////////////////////////////////////////////////
//  "http.c"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "http.h"
#include "../netutils.h"
#include "websockets.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>  // malloc()
#include <sys/socket.h>

// START OF FPXLIBC LINK-TIME DEPENDENCIES
#include "../../fpx_mem/mem.h"
#include "../../fpx_string/string.h"
// END OF FPXLIBC LINK-TIME DEPENDENCIES

#define URI_MAXLENGTH 255
#define HTTP_DATA_ALLOC_BLOCK_SIZE 512

#define BUFFER_DEFAULT 16384

static int _set_http_version(struct _fpx_http_content* _cnt, const char* _version);
static int _get_http_version(struct _fpx_http_content* _cnt, char* _output, size_t _maxlen);

static int _add_http_header(struct _fpx_http_content* _cnt, const char* _key, const char* _value);
static int _get_http_header(
  struct _fpx_http_content* _cnt, const char* _key, char* _output, size_t _maxlen);

static int _append_http_body(struct _fpx_http_content* _cnt, const char* _chunk, size_t _len);
static int _get_http_body(struct _fpx_http_content* _cnt, char* _output, size_t _maxlen);

static int _copy_http_content(struct _fpx_http_content* _dst, const struct _fpx_http_content* _src);

static int _destroy_http_content(struct _fpx_http_content* cntptr);

int fpx_httprequest_init(fpx_httprequest_t* reqptr) {
  if (NULL == reqptr)
    return -1;

  fpx_memset(reqptr, 0, sizeof(fpx_httprequest_t));

  return 0;
}


int fpx_httprequest_set_method(fpx_httprequest_t* reqptr, fpx_httpmethod_t method) {
  if (NULL == reqptr)
    return -1;

  reqptr->method = method;

  return 0;
}


fpx_httpmethod_t fpx_httprequest_get_method(fpx_httprequest_t* reqptr) {
  if (NULL == reqptr)
    return ERROR;

  return reqptr->method;
}


int fpx_httprequest_set_uri(fpx_httprequest_t* reqptr, const char* newuri) {
  if (NULL == reqptr || NULL == newuri)
    return -1;

  int urilen;
  if ((urilen = fpx_getstringlength(newuri)) > URI_MAXLENGTH)
    return URI_MAXLENGTH;

  // copy URI into request object after determining proper length
  fpx_memcpy(reqptr->uri, newuri, urilen);

  return 0;
}


int fpx_httprequest_get_uri(fpx_httprequest_t* reqptr, char* store_buf, size_t maxlen) {
  if (NULL == reqptr || NULL == store_buf)
    return -1;

  int urilen;
  if ((urilen = fpx_getstringlength(reqptr->uri)) > (maxlen - 1))
    return urilen + 1;

  // we copy after we confirm the sizes check out
  fpx_memcpy(store_buf, reqptr->uri, urilen);
  store_buf[urilen] = 0;  // and we NULL-terminate

  return 0;
}


int fpx_httprequest_set_version(fpx_httprequest_t* reqptr, const char* input) {
  if (NULL == reqptr)
    return -1;

  return _set_http_version(&reqptr->content, input);
}


int fpx_httprequest_get_version(fpx_httprequest_t* reqptr, char* outbuffer, size_t outbuf_len) {
  if (NULL == reqptr)
    return -1;

  return _get_http_version(&reqptr->content, outbuffer, outbuf_len);
}


int fpx_httprequest_add_header(fpx_httprequest_t* reqptr, const char* key, const char* value) {
  if (NULL == reqptr)
    return -1;

  return _add_http_header(&reqptr->content, key, value);
}


int fpx_httprequest_get_header(
  fpx_httprequest_t* reqptr, const char* key, char* output, size_t output_len) {
  if (NULL == reqptr)
    return -1;

  return _get_http_header(&reqptr->content, key, output, output_len);
}


int fpx_httprequest_append_body(fpx_httprequest_t* reqptr, const char* new_chunk, size_t body_len) {
  if (NULL == reqptr)
    return -1;

  return _append_http_body(&reqptr->content, new_chunk, body_len);
}


int fpx_httprequest_get_body(fpx_httprequest_t* reqptr, char* outbuffer, size_t max_len) {
  if (NULL == reqptr)
    return -1;

  return _get_http_body(&reqptr->content, outbuffer, max_len);
}


int fpx_httprequest_get_body_length(fpx_httprequest_t* reqptr) {
  if (NULL == reqptr)
    return -1;

  return reqptr->content.body_len;
}


int fpx_httprequest_copy(fpx_httprequest_t* _dst, const fpx_httprequest_t* _src) {
  if (NULL == _dst || NULL == _src)
    return -1;

  fpx_strcpy(_dst->uri, _src->uri);

  _dst->method = _src->method;

  return _copy_http_content(&_dst->content, &_src->content);
}


int fpx_httprequest_destroy(fpx_httprequest_t* reqptr) {
  if (NULL == reqptr)
    return -1;

  return _destroy_http_content(&reqptr->content);
}


int fpx_httpresponse_init(fpx_httpresponse_t* resptr) {
  if (NULL == resptr)
    return -1;

  fpx_memset(resptr, 0, sizeof(fpx_httpresponse_t));

  return 0;
}


int fpx_httpresponse_set_version(fpx_httpresponse_t* resptr, const char* input) {
  if (NULL == resptr)
    return -1;

  return _set_http_version(&resptr->content, input);
}


int fpx_httpresponse_get_version(fpx_httpresponse_t* resptr, char* outbuffer, size_t outbuf_len) {
  if (NULL == resptr)
    return -1;

  return _get_http_version(&resptr->content, outbuffer, outbuf_len);
}


int fpx_httpresponse_add_header(fpx_httpresponse_t* resptr, const char* key, const char* value) {
  if (NULL == resptr)
    return -1;

  return _add_http_header(&resptr->content, key, value);
}


int fpx_httpresponse_get_header(
  fpx_httpresponse_t* resptr, const char* key, char* output, size_t output_len) {
  if (NULL == resptr)
    return -1;

  return _get_http_header(&resptr->content, key, output, output_len);
}


int fpx_httpresponse_append_body(
  fpx_httpresponse_t* resptr, const char* new_chunk, size_t body_len) {
  if (NULL == resptr)
    return -1;

  return _append_http_body(&resptr->content, new_chunk, body_len);
}


int fpx_httpresponse_get_body(fpx_httpresponse_t* resptr, char* outbuffer, size_t max_len) {
  if (NULL == resptr)
    return -1;

  return _get_http_body(&resptr->content, outbuffer, max_len);
}


int fpx_httpresponse_get_body_length(fpx_httpresponse_t* resptr) {
  if (NULL == resptr)
    return -1;

  return resptr->content.body_len;
}


int fpx_httpresponse_copy(fpx_httpresponse_t* _dst, const fpx_httpresponse_t* _src) {
  if (NULL == _dst || NULL == _src)
    return -1;

  _dst->status = _src->status;
  fpx_strcpy(_dst->reason, _src->reason);

  return _copy_http_content(&_dst->content, &_src->content);
}


int fpx_httpresponse_destroy(fpx_httpresponse_t* resptr) {
  if (NULL == resptr)
    return -1;

  return _destroy_http_content(&resptr->content);
}


int fpx_websocket_send_close(
  int filedescriptor, int16_t code, const uint8_t* reason, uint8_t reason_length, uint8_t masked) {

  fpx_websocketframe_t close_frame = { .final = TRUE,
    .mask_set = ((FALSE == masked) ? 0 : 1),
    .opcode = WEBSOCKET_CLOSE,
    .reserved1 = FALSE,
    .reserved2 = FALSE,
    .reserved3 = FALSE,
    .payload = NULL,
    .payload_length = 0,
    .payload_allocated = 0 };

  if (0 > code) {
    reason_length = 0;
  } else {
    fpx_endian_swap_if_host(&code, sizeof(code));
    fpx_websocketframe_append_payload(&close_frame, (uint8_t*)&code, sizeof(code));
  }

  if (reason_length > 123)
    reason_length = 123;

  if (NULL != reason && reason_length > 0)
    fpx_websocketframe_append_payload(&close_frame, reason, reason_length);

  int retval = fpx_websocketframe_send(&close_frame, filedescriptor);

  fpx_websocketframe_destroy(&close_frame);

  return retval;
}


int fpx_websocketframe_init(fpx_websocketframe_t* frameptr) {
  if (NULL == frameptr)
    return -1;

  fpx_memset(frameptr, 0, sizeof(*frameptr));

  return 0;
}


#define FPX_ALLOC_CALC(output, old_len, new_len)                                            \
  output = HTTP_DATA_ALLOC_BLOCK_SIZE * ((old_len + new_len) / HTTP_DATA_ALLOC_BLOCK_SIZE); \
  if ((old_len + new_len) % HTTP_DATA_ALLOC_BLOCK_SIZE != 0)                                \
  output += HTTP_DATA_ALLOC_BLOCK_SIZE

int fpx_websocketframe_append_payload(
  fpx_websocketframe_t* frameptr, const uint8_t* payload, size_t payload_len) {
  if (NULL == frameptr)
    return -1;

  if (payload_len < 1)
    return 0;

  size_t to_allocate;

  FPX_ALLOC_CALC(to_allocate, frameptr->payload_length, payload_len);

  if (NULL == frameptr->payload) {
    frameptr->payload_length = 0;
    frameptr->payload_allocated = 0;
  }

  if (frameptr->payload_allocated == 0) {
    frameptr->payload = (uint8_t*)calloc(to_allocate, 1);
  } else if (frameptr->payload_allocated < to_allocate) {
    frameptr->payload = (uint8_t*)realloc(frameptr->payload, to_allocate);
  }

  if (NULL == frameptr->payload) {
    // bad alloc
    return -2;
  }

  fpx_memcpy(&frameptr->payload[frameptr->payload_length], payload, payload_len);

  frameptr->payload_length += payload_len;
  frameptr->payload_allocated = to_allocate;

  return 0;
}


int fpx_websocketframe_send(const fpx_websocketframe_t* frameptr, int fd) {
  uint8_t write_buf[BUFFER_DEFAULT] = { 0 };

  // to ignore SIGPIPE when writing to a disconnected client
  int send_flags = MSG_NOSIGNAL;

  {
    uint8_t meta_byte = 0x0;
    meta_byte |= ((frameptr->final != 0) << 7);  // fin bit is MSB of byte
    meta_byte |= (frameptr->opcode & 0x0f);      // set op-code

    if (0 > send(fd, &meta_byte, 1, MSG_MORE | send_flags))  // send meta_byte
      return errno;
  }


  uint8_t len_len = 0;

  {
    uint8_t payload_len_first_byte = 0;

    if (frameptr->payload_length < 126)
      payload_len_first_byte = frameptr->payload_length;
    else if (frameptr->payload_length <= USHRT_MAX) {
      payload_len_first_byte = 126;
      len_len = 2;
    } else {
      payload_len_first_byte = 127;
      len_len = 8;
    }

    if (frameptr->mask_set)
      payload_len_first_byte |= 0x80;

    if (0 > send(fd, &payload_len_first_byte, 1, MSG_MORE | send_flags))
      return errno;
  }


  {
    uint16_t short_len;
    uint64_t longlong_len;

    void* the_length;

    if (2 == len_len) {
      short_len = frameptr->payload_length;
      the_length = &short_len;
    } else if (8 == len_len) {
      longlong_len = frameptr->payload_length;
      the_length = &longlong_len;
    }

    fpx_endian_swap_if_host(the_length, len_len);

    int flag = (0 < frameptr->payload_length) ? MSG_MORE : 0;
    if (0 > send(fd, the_length, len_len, flag | send_flags))
      return errno;
  }

  uint64_t payload_written = 0;
  while (payload_written < frameptr->payload_length) {
    int to_write = (frameptr->payload_length - payload_written);
    int remaining_space = sizeof(write_buf);

    if (to_write > remaining_space)
      to_write = remaining_space;

    fpx_memcpy(write_buf, frameptr->payload + payload_written, to_write);

    // mask the payload
    if (frameptr->mask_set) {
      for (int i = 0; i < to_write; ++i) {
        write_buf[i] ^= frameptr->masking_key[i % 4];
      }
    }

    payload_written += to_write;

    if (0 > send(fd, write_buf, to_write, 0 | send_flags))
      return errno;
  }

  return 0;
}


int fpx_websocketframe_destroy(fpx_websocketframe_t* frameptr) {
  if (NULL == frameptr)
    return -1;

  if (0 < frameptr->payload_allocated) {
    fpx_memset(frameptr->payload, 0, frameptr->payload_length);
    free(frameptr->payload);
    frameptr->payload_length = 0;
  }

  return 0;
}


// ------------------------------------------------------- //
// ------------------------------------------------------- //
// ---------- STATIC FUNCTION DEFINITIONS BELOW ---------- //
// ------------------------------------------------------- //
// ------------------------------------------------------- //


static int _destroy_http_content(struct _fpx_http_content* cntptr) {
  if (NULL == cntptr)
    return -2;

  if (0 < cntptr->body_allocated) {
    fpx_memset(cntptr->body, 0, cntptr->body_len);
    free(cntptr->body);
    cntptr->body_len = 0;
  }

  if (0 < cntptr->headers_allocated) {
    fpx_memset(cntptr->headers, 0, cntptr->headers_len);
    free(cntptr->headers);
    cntptr->headers_len = 0;
  }

  return 0;
}


int _set_http_version(struct _fpx_http_content* cntptr, const char* input) {
  if (NULL == cntptr || NULL == input)
    return -1;

  int input_len = fpx_getstringlength(input);
  if (input_len + 1 > sizeof(cntptr->version))
    return sizeof(cntptr->version);

  fpx_strcpy(cntptr->version, input);

  return 0;
}


int _get_http_version(struct _fpx_http_content* cntptr, char* outbuffer, size_t outbuf_len) {
  if (NULL == cntptr || NULL == outbuffer)
    return -1;

  int lesser;

  int version_len = fpx_getstringlength(cntptr->version);

  if (version_len + 1 > outbuf_len) {
    lesser = outbuf_len - 1;
  } else
    lesser = version_len;

  fpx_memcpy(outbuffer, cntptr->version, lesser);
  outbuffer[lesser] = 0;

  return 0;
}


static int _add_http_header(struct _fpx_http_content* cntptr, const char* key, const char* value) {
  if (NULL == cntptr || NULL == key || NULL == value)
    return -1;

  size_t keylen, valuelen;
  size_t to_allocate;

  keylen = fpx_getstringlength(key);
  valuelen = fpx_getstringlength(value);

  size_t new_header_len =  // total length of header =
    keylen +               // header key length
    2 +                    // ": "
    valuelen +             // header value length
    2;                     // "\r\n"

  FPX_ALLOC_CALC(to_allocate, cntptr->headers_len, new_header_len);

  if (NULL == cntptr->headers) {
    cntptr->headers_len = 0;
    cntptr->headers_allocated = 0;
  }

  if (cntptr->headers_allocated == 0) {
    cntptr->headers = (char*)calloc(to_allocate, 1);
  } else if (to_allocate > cntptr->headers_allocated) {
    cntptr->headers = (char*)realloc(cntptr->headers, to_allocate);
  }

  if (NULL == cntptr->headers) {
    // bad alloc
    return -2;
  }

  // prepare the key for converting to lowercase;
  // this will help later when finding a header by key
  fpx_memcpy(&cntptr->headers[cntptr->headers_len], key, keylen);
  cntptr->headers[cntptr->headers_len + keylen] = 0;

  // now we convert the key to lowercase
  // this is allowed according to HTTP RFC
  fpx_string_to_lower(&cntptr->headers[cntptr->headers_len], FALSE);

  fpx_memcpy(&cntptr->headers[cntptr->headers_len + keylen], ": ", 2);

  // remove leading whitespace from header value
  for (; *value == ' '; ++value, --valuelen)
    ;

  fpx_memcpy(&cntptr->headers[cntptr->headers_len + keylen + 2], value, valuelen);
  fpx_memcpy(&cntptr->headers[cntptr->headers_len + keylen + 2 + valuelen], "\r\n", 2);

  cntptr->headers_len += keylen + 2 + valuelen + 2;
  cntptr->headers_allocated = to_allocate;

  return 0;
}


static int _get_http_header(
  struct _fpx_http_content* cntptr, const char* key, char* output, size_t output_len) {
  if (NULL == cntptr || NULL == key || NULL == output)
    return -1;

  if (NULL == cntptr->headers ||
    cntptr->headers_len < 6 /*minimum length of valid header: "x:y\r\n"*/)
    return -2;

  int found_index;

  {
    int keylen;
    char* key_but_lowercase = NULL;

    keylen = fpx_getstringlength(key);
    key_but_lowercase = (char*)malloc(keylen + 3);
    if (NULL == key_but_lowercase)
      return -3;
    fpx_memcpy(key_but_lowercase, key, keylen);
    fpx_memcpy(&key_but_lowercase[keylen], ": ", 2);
    key_but_lowercase[keylen + 2] = 0;  // NULL-terminator

    fpx_string_to_lower(key_but_lowercase, FALSE);

    found_index = fpx_substringindex(cntptr->headers, key_but_lowercase);

    free(key_but_lowercase);
    key_but_lowercase = NULL;
  }

  if (found_index < 0)
    return -2;

  if (found_index > 0 && cntptr->headers[found_index - 1] != '\n') {
    // the key is not immediately after the CRLF of another header
    // this should never happen
    return -3;
  }

  {
    int value_start, crlf_index;

    value_start = fpx_substringindex(&cntptr->headers[found_index], ":");

    if (value_start < 0) {
      // a colon was not found after the key
      // this should never happen
      return -3;
    }

    value_start++;

    // now we ignore leading whitespace
    for (; cntptr->headers[found_index + value_start] == ' '; ++value_start)
      ;

    crlf_index = fpx_substringindex(&cntptr->headers[found_index + value_start], "\r\n");

    if (crlf_index > output_len)
      return crlf_index + 1;

    fpx_memcpy(output, &cntptr->headers[found_index + value_start], crlf_index);
  }

  return 0;
}


static int _append_http_body(
  struct _fpx_http_content* cntptr, const char* new_chunk, size_t body_len) {
  if (NULL == cntptr || NULL == new_chunk)
    return -1;

  if (body_len < 1)
    return 0;

  size_t to_allocate;

  FPX_ALLOC_CALC(to_allocate, cntptr->body_len, body_len);

  if (NULL == cntptr->body) {
    cntptr->body_len = 0;
    cntptr->body_allocated = 0;
  }

  if (cntptr->body_allocated == 0) {
    cntptr->body = (char*)calloc(to_allocate, 1);
  } else if (cntptr->body_allocated < to_allocate) {
    cntptr->body = (char*)realloc(cntptr->body, to_allocate);
  }

  if (NULL == cntptr->body) {
    // bad alloc
    return -2;
  }

  fpx_memcpy(&cntptr->body[cntptr->body_len], new_chunk, body_len);

  cntptr->body_len += body_len;
  cntptr->body_allocated = to_allocate;

  return 0;
}


static int _get_http_body(struct _fpx_http_content* cntptr, char* outbuffer, size_t max_len) {
  if (NULL == cntptr || NULL == outbuffer)
    return -1;

  if (NULL == cntptr->body || cntptr->body_len == 0 || cntptr->body_allocated == 0)
    return -2;

  int lesser;
  int retval = 0;

  if (max_len < cntptr->body_len + 1) {
    lesser = max_len - 1;
    retval = cntptr->body_len;
  } else
    lesser = cntptr->body_len;

  fpx_memcpy(outbuffer, cntptr->body, lesser);
  outbuffer[lesser] = 0;

  return retval;
}


static int _copy_http_content(
  struct _fpx_http_content* _dst, const struct _fpx_http_content* _src) {
  if (NULL == _dst || NULL == _src)
    return -3;

  fpx_memcpy(_dst, _src, sizeof(*_dst));

  if (_dst->headers_allocated > 0) {
    _dst->headers = (char*)malloc(_dst->headers_allocated);
    if (NULL == _dst->headers) {
      // bad alloc
      return -2;
    }

    fpx_memcpy(_dst->headers, _src->headers, _dst->headers_len);
  }

  if (_dst->body_allocated > 0) {
    _dst->body = (char*)malloc(_dst->body_allocated);
    if (NULL == _dst->body) {
      // bad alloc
      return -2;
    }

    fpx_memcpy(_dst->body, _src->body, _dst->body_len);
  }

  return 0;
}
