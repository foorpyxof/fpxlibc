////////////////////////////////////////////////////////////////
//  "json.c"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "json.h"

#include "../../fpx_mem/mem.h"
#include "../../fpx_string/string.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef JSON_MAX_KEY_LENGTH
#define MAX_KEY_LENGTH JSON_MAX_KEY_LENGTH
#else
#define MAX_KEY_LENGTH 127
#endif


struct _fpx_json_kv {
    char key[MAX_KEY_LENGTH + 1];
    fpx_json_value_t value;
};


int fpx_json_kv_get_key(fpx_json_kv_t* kvptr, char* outbuf, size_t outbuf_len) {
  if (NULL == kvptr || NULL == outbuf)
    return -1;

  int keylen = fpx_getstringlength(kvptr->key);

  if (outbuf_len < 2)
    return keylen + 1;

  if (outbuf_len < (keylen + 1)) {
    fpx_memcpy(outbuf, kvptr->key, outbuf_len - 1);
    outbuf[outbuf_len] = 0;
    return keylen + 1;
  }

  fpx_strcpy(outbuf, kvptr->key);
  return 0;
}


int fpx_json_kv_get_value(fpx_json_kv_t* kvptr, fpx_json_value_t* output) {
  if (NULL == kvptr || NULL == output)
    return -1;

  fpx_memcpy(output, &kvptr->value, sizeof(*output));

  return 0;
}


int fpx_json_object_add_kv(
  fpx_json_object_t* objptr, const char* key, const void* value, fpx_json_value_type_t val_type) {
  if (NULL == objptr || NULL == key || NULL == value)
    return -1;

  objptr->kv_pairs =
    (fpx_json_kv_t*)reallocarray(objptr->kv_pairs, objptr->pair_count, sizeof(fpx_json_kv_t));

  if (NULL == objptr->kv_pairs) {
    int err = errno;
    perror("reallocarray()");
    return err;
  }

  int given_key_length = fpx_getstringlength(key);
  if (given_key_length > MAX_KEY_LENGTH)
    return MAX_KEY_LENGTH;

  fpx_json_kv_t* pair = &objptr->kv_pairs[objptr->pair_count++];
  fpx_memset(pair, 0, sizeof(*pair));

  fpx_strcpy(pair->key, key);

  switch (val_type) {
    case STRING:
      pair->value.data = malloc(fpx_getstringlength(value) + 1);

      if (NULL == pair->value.data) {
        int err = errno;
        perror("malloc()");
        return err;
      }

      fpx_strcpy(pair->value.data, value);
      break;
    case OBJECT:
      pair->value.data = malloc(sizeof(fpx_json_object_t));

      if (NULL == pair->value.data) {
        int err = errno;
        perror("malloc()");
        return err;
      }

      fpx_memcpy(pair->value.data, value, sizeof(fpx_json_object_t));
      break;
  }

  return 0;
}


int fpx_json_object_get_value(
  fpx_json_object_t* objptr, const char* key, fpx_json_value_t* output) {
  if (NULL == objptr || NULL == key || NULL == output)
    return -1;

  for (int i = 0; i < objptr->pair_count; ++i) {
    fpx_json_kv_t* pair = &objptr->kv_pairs[i];

    if (0 == strncmp(pair->key, key, MAX_KEY_LENGTH)) {
      *output = pair->value;
      return 0;
    }
  }

  return -2;
}
