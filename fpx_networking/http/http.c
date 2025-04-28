////////////////////////////////////////////////////////////////
//  "http.c"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "http.h"

#include <stdlib.h>

// START OF FPXLIBC LINK-TIME DEPENDENCIES
#include "../../fpx_mem/mem.h"
#include "../../fpx_string/string.h"
// END OF FPXLIBC LINK-TIME DEPENDENCIES

#define URI_MAXLENGTH 255

struct _fpx_http_content {
    char version[16];

    char* headers;  // HEAP
    char* body;     // HEAP

    size_t headers_len;  // no null terminator included
    size_t body_len;     // no null terminator included

    size_t headers_allocated;
    size_t body_allocated;
};

#define HTTP_DATA_ALLOC_BLOCK_SIZE 512
struct _fpx_httprequest {
    fpx_httpmethod_t _method;
    char _uri[256];

    struct _fpx_http_content _content;
};

struct _fpx_httpresponse {
    uint8_t _status;
    char _reason[32];

    struct _fpx_http_content _content;
};

static int _set_http_version(struct _fpx_http_content*, const char*);
static int _get_http_version(struct _fpx_http_content*, char*, size_t);

static int _add_http_header(struct _fpx_http_content*, const char*, const char*);
static int _get_http_header(struct _fpx_http_content*, const char*, char*, size_t);

static int _append_http_body(struct _fpx_http_content*, const char*, size_t);
static int _get_http_body(struct _fpx_http_content*, char*, size_t);


int fpx_httprequest_init(fpx_httprequest_t* reqptr) {
  if (NULL == reqptr)
    return -1;

  fpx_memset(reqptr, 0, sizeof(fpx_httprequest_t));

  return 0;
}


int fpx_httprequest_set_method(fpx_httprequest_t* reqptr, fpx_httpmethod_t method) {
  if (NULL == reqptr)
    return -1;

  reqptr->_method = method;

  return 0;
}


fpx_httpmethod_t fpx_httprequest_get_method(fpx_httprequest_t* reqptr) {
  if (NULL == reqptr)
    return -1;

  return reqptr->_method;
}


int fpx_httprequest_set_uri(fpx_httprequest_t* reqptr, const char* newuri) {
  if (NULL == reqptr || NULL == newuri)
    return -1;

  int urilen;
  if ((urilen = fpx_getstringlength(newuri)) > URI_MAXLENGTH)
    return URI_MAXLENGTH;

  // copy URI into request object after determining proper length
  fpx_memcpy(reqptr->_uri, newuri, urilen);

  return 0;
}


int fpx_httprequest_get_uri(fpx_httprequest_t* reqptr, char* store_buf, size_t maxlen) {
  if (NULL == reqptr || NULL == store_buf)
    return -1;

  int urilen;
  if ((urilen = fpx_getstringlength(reqptr->_uri)) > (maxlen - 1))
    return urilen + 1;

  // we copy after we confirm the sizes check out
  fpx_memcpy(store_buf, reqptr->_uri, urilen);
  store_buf[urilen] = 0;  // and we NULL-terminate

  return 0;
}


int fpx_httprequest_set_version(fpx_httprequest_t* reqptr, const char* input) {
  if (NULL == reqptr)
    return -1;

  return _set_http_version(&reqptr->_content, input);
}


int fpx_httprequest_get_version(fpx_httprequest_t* reqptr, char* outbuffer, size_t outbuf_len) {
  if (NULL == reqptr)
    return -1;

  return _get_http_version(&reqptr->_content, outbuffer, outbuf_len);
}


int fpx_httprequest_add_header(fpx_httprequest_t* reqptr, const char* key, const char* value) {
  if (NULL == reqptr)
    return -1;

  return _add_http_header(&reqptr->_content, key, value);
}


int fpx_httprequest_get_header(
  fpx_httprequest_t* reqptr, const char* key, char* output, size_t output_len) {
  if (NULL == reqptr)
    return -1;

  return _get_http_header(&reqptr->_content, key, output, output_len);
}


int fpx_httprequest_append_body(fpx_httprequest_t* reqptr, const char* new_chunk, size_t body_len) {
  if (NULL == reqptr)
    return -1;

  return _append_http_body(&reqptr->_content, new_chunk, body_len);
}


int fpx_httprequest_get_body(fpx_httprequest_t* reqptr, char* outbuffer, size_t max_len) {
  if (NULL == reqptr)
    return -1;

  return _get_http_body(&reqptr->_content, outbuffer, max_len);
}


int fpx_httpresponse_init(fpx_httpresponse_t* resptr) {
  if (NULL == resptr)
    return -1;

  fpx_memset(resptr, 0, sizeof(fpx_httpresponse_t));

  return 0;
}


int fpx_httpresponse_add_header(fpx_httpresponse_t* resptr, const char* key, const char* value) {
  if (NULL == resptr)
    return -1;

  return _add_http_header(&resptr->_content, key, value);
}


int fpx_httpresponse_get_header(
  fpx_httpresponse_t* resptr, const char* key, char* output, size_t output_len) {
  if (NULL == resptr)
    return -1;

  return _get_http_header(&resptr->_content, key, output, output_len);
}


int fpx_httpresponse_append_body(
  fpx_httpresponse_t* resptr, const char* new_chunk, size_t body_len) {
  if (NULL == resptr)
    return -1;

  return _append_http_body(&resptr->_content, new_chunk, body_len);
}


int fpx_httpresponse_get_body(fpx_httpresponse_t* resptr, char* outbuffer, size_t max_len) {
  if (NULL == resptr)
    return -1;

  return _get_http_body(&resptr->_content, outbuffer, max_len);
}


// ------------------------------------------------------- //
// ------------------------------------------------------- //
// ---------- STATIC FUNCTION DEFINITIONS BELOW ---------- //
// ------------------------------------------------------- //
// ------------------------------------------------------- //


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

  to_allocate = (cntptr->headers_len + new_header_len) / HTTP_DATA_ALLOC_BLOCK_SIZE;
  if (new_header_len % HTTP_DATA_ALLOC_BLOCK_SIZE == 0)
    to_allocate += HTTP_DATA_ALLOC_BLOCK_SIZE;

  if (NULL == cntptr->headers) {
    cntptr->headers_len = 0;
    cntptr->headers_allocated = 0;

    cntptr->headers = (char*)malloc(to_allocate);
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

  size_t to_allocate;

  to_allocate = (cntptr->body_len + body_len) / HTTP_DATA_ALLOC_BLOCK_SIZE;
  if (body_len % HTTP_DATA_ALLOC_BLOCK_SIZE == 0)
    to_allocate += HTTP_DATA_ALLOC_BLOCK_SIZE;

  if (NULL == cntptr->body) {
    cntptr->body_len = 0;
    cntptr->body_allocated = 0;

    cntptr->body = (char*)malloc(to_allocate);
  } else if (cntptr->body_allocated < to_allocate) {
    cntptr->body = (char*)realloc(cntptr->body, to_allocate);
  }

  if (NULL == cntptr->body) {
    // bad alloc
    return -2;
  }

  fpx_memcpy(&cntptr->body[cntptr->body_len], new_chunk, body_len);

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
