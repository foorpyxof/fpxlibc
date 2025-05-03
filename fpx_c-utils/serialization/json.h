#ifndef FPX_JSON_H
#define FPX_JSON_H

////////////////////////////////////////////////////////////////
//  "json.h"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////


#include "../../fpx_types.h"


typedef struct _fpx_json_object fpx_json_object_t;
typedef struct _fpx_json_kv fpx_json_kv_t;
typedef struct _fpx_json_value fpx_json_value_t;

typedef enum {
  STRING,
  OBJECT
} fpx_json_value_type_t;

struct _fpx_json_object {
  fpx_json_kv_t* kv_pairs;
  size_t pair_count;
};

struct _fpx_json_value {
  void* data;  // this could be a string value or another object
  fpx_json_value_type_t value_type;

  size_t value_length; // if a string
};

// TODO: improve comments

/**
 * takes json kv ptr, output buffer to store NULL-terminated key data in, max output len
 * return -1 if nullptrs passed, any positive number to resemble minimum buffer length if too short, 0 on success
 */
int fpx_json_kv_get_key(fpx_json_kv_t*, char*, size_t);

/**
 * takes json kv ptr, output value ptr
 * return -1 if nullptrs passed, 0 on success
 */
int fpx_json_kv_get_value(fpx_json_kv_t*, fpx_json_value_t*);

/**
 *  takes json object ptr, key, value (either NULL-terminated string or object ptr), value type (string or object)
 *  return -1 if nullptrs passed, any positive number to resemble maximum key length if too long, errno on other error, 0 on success
 */
int fpx_json_object_add_kv(fpx_json_object_t*, const char*, const void*, fpx_json_value_type_t);

/**
 * takes json object ptr, key, value struct ptr for output
 * returns -1 if nullptrs passed, -2 if not found, 0 on success
 */
int fpx_json_object_get_value(fpx_json_object_t*, const char*, fpx_json_value_t*);


#endif // FPX_JSON_H
