#ifndef FPX_HTTP_H
#define FPX_HTTP_H

////////////////////////////////////////////////////////////////
//  "http.h"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../../fpx_types.h"

typedef struct _fpx_httprequest   fpx_httprequest_t;
typedef struct _fpx_httpresponse  fpx_httpresponse_t;

typedef enum {
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
  ERROR = 0x8000,
} fpx_httpmethod_t;

/**
 * Initialize a httprequest object to defaults (zeroes)
 *
 * Input:
 * - Pointer to request object;
 *
 * Returns:
 * -  0 on success
 *   -1 if the provided pointer is NULL
 */
int fpx_httprequest_init(fpx_httprequest_t*);

/**
 * Set the HTTP method of an fpx_httprequest_t object
 *
 * Input:
 * - Pointer to request object
 * - HTTP method (from enum)
 *
 * returns:
 * -  0 on success
 * - -1 is any passed pointer is unexpectedly NULL
 */
int fpx_httprequest_set_method(fpx_httprequest_t*, fpx_httpmethod_t);

/**
 * Get the HTTP method of an fpx_httprequest_t object
 *
 * Input:
 * - Pointer to request object
 *
 * returns:
 * - The associated HTTP method on success
 * - ERROR on error
 */
fpx_httpmethod_t fpx_httprequest_get_method(fpx_httprequest_t*);

/**
 * Set the URI of an fpx_httprequest_t object
 *
 * Input:
 * - Pointer to request object
 * - String containing NULL-terminated URI
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - any positive value to represent the maximum allowed URI size
 * if the one provided was too large
 *
 * Notes:
 * - The operation will NOT start if it is determined that the given URI is too long
 */
int fpx_httprequest_set_uri(fpx_httprequest_t*, const char*);

/**
 * Get the URI of an fpx_httprequest_t object
 *
 * Input:
 * - Pointer to request object
 * - Pointer to a buffer of characters to store the NULL-terminated value in
 * - Size of output buffer in bytes
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - any positive value to represent the minimum required buffer size
 * if the one provided was too small
 */
int fpx_httprequest_get_uri(fpx_httprequest_t*, char*, size_t);

/**
 * Set the HTTP version of the request
 *
 * Input:
 * - Pointer to the request object
 * - NULL-terminated string that looks like the following: "1.1" or "2" or "3"
 * in short: it must represent the version like what comes after HTTP/
 * when normally defining the version
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - any positive value to represent the maximum length of the input
 * if the input is too large
 *
 * Notes:
 * - The operation will NOT start if it is determined that the input is too long
 */
int fpx_httprequest_set_version(fpx_httprequest_t*, const char*);

/**
 * Get the HTTP version of the request
 *
 * Input:
 * - Pointer to the request object
 * - Pointer to a character buffer to store the result in
 * - Length of the output buffer
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if no version information was found
 * - any positive value to represent the minimum length of the output buffer
 * if it is deemed too small
 *
 * Notes:
 * - The operation will PARTIALLY complete if the output buffer is deemed too small
 */
int fpx_httprequest_get_version(fpx_httprequest_t*, char*, size_t);

/**
 * Add a header to an fpx_httprequest_t object
 *
 * Input:
 * - Pointer to request object
 * - String containing NULL-terminated Header name
 * - String containing NULL-terminated Header value
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if any memory allocation fails for any reason
 */
int fpx_httprequest_add_header(fpx_httprequest_t*, const char*, const char*);

/**
 * Get the Header value attached to the first occurence of a key
 * in an fpx_httprequest_t object, and store it in 
 *
 * Input:
 * - Pointer to request object
 * - Pointer to a Header key
 * - Pointer to a buffer of characters to store the value in
 * - Size of output buffer in bytes
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if the Header key was not found
 * - -3 if an unknown error occured
 * - any positive value to represent the minimum required buffer size
 * if the one provided was too small
 */
int fpx_httprequest_get_header(fpx_httprequest_t*, const char*, char*, size_t);

/**
 * Append to the body of an fpx_httprequest_t object
 *
 * Input:
 * - Pointer to request object
 * - String containing Body data to append
 * - Size of the content body to append
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if any memory allocation fails for any reason
 */
int fpx_httprequest_append_body(fpx_httprequest_t*, const char*, size_t);

/**
 * Get the request body of an fpx_httprequest_t object
 *
 * Input:
 * - Pointer to request object
 * - Pointer to a buffer of characters to store the value in
 * - Size of output buffer in bytes
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if no Body was found
 * - any positive value to represent the minimum required buffer size
 * if the one provided was too small
 *
 * Notes:
 * - The operation will PARTIALLY complete if the output buffer is deemed too small
 * - The body stored in the output_buffer is always NULL-terminated
 */
int fpx_httprequest_get_body(fpx_httprequest_t*, char*, size_t);

/**
 * Get the request body length of an fpx_httprequest_t object
 *
 * Input:
 * - Pointer to request object
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 */
int fpx_httprequest_get_body_length(fpx_httprequest_t*);

/**
 * Copy the contents of *_src to *_dst
 *
 * Input:
 * - Pointer to the destination request object (to copy into)
 * - Pointer to the source request object (to copy from)
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if any memory allocation fails for any reason
 * - -3 if an unknown error occurs
 */
int fpx_httprequest_copy(fpx_httprequest_t* _dst, const fpx_httprequest_t* _src);

/**
 * Initialize a httpresponse object to defaults (zeroes)
 *
 * Input:
 * - Pointer to response object;
 *
 * Returns:
 * -  0 on success
 *   -1 if the provided pointer is NULL
 */
int fpx_httpresponse_init(fpx_httpresponse_t*);

/**
 * Set the HTTP version of the response
 *
 * Input:
 * - Pointer to the response object
 * - NULL-terminated string that looks similar to the following: "1.1" or "2.0" or "3"
 * in short: it must represent the version like what comes after HTTP/
 * when normally defining the version
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - any positive value to represent the maximum length of the input
 * if the input is too large
 *
 * Notes:
 * - The operation will NOT start if it is determined that the input is too long
 */
int fpx_httpresponse_set_version(fpx_httpresponse_t*, const char*);

/**
 * Get the HTTP version of the response
 *
 * Input:
 * - Pointer to the response object
 * - Pointer to a character buffer to store the result in
 * - Length of the output buffer
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if no version information was found
 * - any positive value to represent the minimum length of the output buffer
 * if it is deemed too small
 *
 * Notes:
 * - The operation will PARTIALLY complete if the output buffer is deemed too small
 */
int fpx_httpresponse_get_version(fpx_httpresponse_t*, char*, size_t);

/**
 * Add a header to an fpx_httpresponse_t object
 *
 * Input:
 * - Pointer to response object
 * - String containing NULL-terminated Header name
 * - String containing NULL-terminated Header value
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if any memory allocation fails for any reason
 */
int fpx_httpresponse_add_header(fpx_httpresponse_t*, const char*, const char*);

/**
 * Get the Header value attached to the first occurence of a key
 * in an fpx_httpresponse_t object, and store it in 
 *
 * Input:
 * - Pointer to response object
 * - Pointer to a Header key
 * - Pointer to a buffer of characters to store the value in
 * - Size of output buffer in bytes
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if the Header key was not found
 * - -3 if an unknown error occured
 * - any positive value to represent the minimum required buffer size
 * if the one provided was too small
 */
int fpx_httpresponse_get_header(fpx_httpresponse_t*, const char*, char*, size_t);

/**
 * Append to the body of an fpx_httpresponse_t object
 *
 * Input:
 * - Pointer to response object
 * - String containing NULL-terminated Body data to append
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if any memory allocation fails for any reason
 */
int fpx_httpresponse_append_body(fpx_httpresponse_t*, const char*, size_t);

/**
 * Get the response body of an fpx_httpresponse_t object
 *
 * Input:
 * - Pointer to response object
 * - Pointer to a buffer of characters to store the value in
 * - Size of output buffer in bytes
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if no Body was found
 * - any positive value to represent the minimum required buffer size
 * if the one provided was too small
 *
 * Notes:
 * - The operation will PARTIALLY complete if the output buffer is deemed too small
 * - The body stored in the output_buffer is always NULL-terminated
 */
int fpx_httpresponse_get_body(fpx_httpresponse_t*, char*, size_t);

/**
 * Get the response body length of an fpx_httpresponse_t object
 *
 * Input:
 * - Pointer to response object
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 */
int fpx_httpresponse_get_body_length(fpx_httpresponse_t*);

/**
 * Copy the contents of *_src to *_dst
 *
 * Input:
 * - Pointer to the destination response object (to copy into)
 * - Pointer to the source response object (to copy from)
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * - -2 if any memory allocation fails for any reason
 * - -3 if an unknown error occurs
 */
int fpx_httpresponse_copy(fpx_httpresponse_t* _dst, const fpx_httpresponse_t* _src);

struct _fpx_http_content {
    char version[16];

    char* headers;  // HEAP
    char* body;     // HEAP

    size_t headers_len;  // no null terminator included
    size_t body_len;     // no null terminator included

    size_t headers_allocated;
    size_t body_allocated;
};

struct _fpx_httprequest {
    fpx_httpmethod_t method;
    char uri[256];

    struct _fpx_http_content content;
};

struct _fpx_httpresponse {
    uint8_t status;
    char reason[32];

    struct _fpx_http_content content;
};

#endif // FPX_HTTP_H
