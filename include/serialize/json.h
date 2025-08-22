#ifndef FPX_JSON_H
#define FPX_JSON_H

#include "alloc/arena.h"

#include "fpx_types.h"

typedef struct _fpx_json_entity Fpx_Json_Entity;
typedef struct _fpx_json_member Fpx_Json_Member;
typedef struct _fpx_json_value Fpx_Json_Value;
typedef struct _fpx_json_string Fpx_Json_String;
typedef struct _fpx_json_object Fpx_Json_Object;
typedef struct _fpx_json_array Fpx_Json_Array;

typedef enum _fpx_json_result {
  FPX_JSON_RESULT_SUCCESS = 0,
  FPX_JSON_RESULT_ARGUMENT_ERROR = -1,
  FPX_JSON_RESULT_SYNTAX_ERROR = -2,
  FPX_JSON_RESULT_MEMORY_ERROR = -3,
  FPX_JSON_RESULT_OUT_OF_BOUNDS_ERROR = -4,
} Fpx_Json_E_Result;

typedef enum {
  FPX_JSON_VALUE_INVALID = 0,
  FPX_JSON_VALUE_OBJECT = 1,
  FPX_JSON_VALUE_STRING = 2,
  FPX_JSON_VALUE_BOOL = 3,
  FPX_JSON_VALUE_NUMBER = 4,
  FPX_JSON_VALUE_ARRAY = 5,
  FPX_JSON_VALUE_NULL = 6,
} Fpx_Json_E_ValueType;

struct _fpx_json_string {
    char* data;
    size_t size;
};

struct _fpx_json_member {
    bool isEmpty;

    Fpx_Json_String key;

    Fpx_Json_Value* value;
};

struct _fpx_json_object {
    Fpx_Json_Member* members;
    size_t memberCount;
};

struct _fpx_json_array {
    Fpx_Json_Value* values;
    size_t count;
};

struct _fpx_json_value {
    Fpx_Json_E_ValueType valueType;

    union {
        Fpx_Json_Object object;
        Fpx_Json_String string;
        bool boolean;
        double number;
        Fpx_Json_Array array;
    };
};

struct _fpx_json_entity {
    fpx_arena* arena;

    Fpx_Json_Object rootObject;

    bool isValid;
};

Fpx_Json_Entity fpx_json_read(const char* json_data, size_t data_len);

Fpx_Json_E_Result fpx_json_destroy(Fpx_Json_Entity*);

#endif  // FPX_JSON_H
