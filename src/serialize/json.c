#include "serialize/json.h"
#include "alloc/arena.h"
#include "c-utils/format.h"
#include "string/string.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define FREE_SAFE(_ptr) \
  if (NULL != _ptr) {   \
    free(_ptr);         \
  }                     \
  _ptr = NULL;

#define IS_WHITESPACE(character) \
  (character == 0x20 || character == 0x0A || character == 0x0D || character == 0x09)

#define TRIM_WHITESPACE(_dataptr, _limitptr)                               \
  {                                                                        \
    for (; (_dataptr < _limitptr) && IS_WHITESPACE(*_dataptr); ++_dataptr) \
      ;                                                                    \
  }

#define SYNTAX_EXPECT(ptr, expect)

#if !defined(NDEBUG) || defined(DEBUG)
#undef SYNTAX_EXPECT
#define SYNTAX_EXPECT(ptr, expect)                                    \
  fprintf(stderr,                                                     \
    "%s:%d - EXPECTED '%c' BUT GOT '%c'. CONTEXT: \"...%.40s...\"\n", \
    __FILE__,                                                         \
    __LINE__,                                                         \
    expect,                                                           \
    *ptr,                                                             \
    ptr);
#endif

// expects first character to be '{'
// returns FPX_JSON_SYNTAX_ERROR otherwise
static Fpx_Json_E_Result _json_object_parse(
  const char** data, const char* limit, fpx_arena* arena, Fpx_Json_Object* output);

static Fpx_Json_E_Result _json_member_parse(
  const char** data, const char* limit, fpx_arena* arena, Fpx_Json_Member* output);

static Fpx_Json_E_Result _json_value_parse(
  const char** data, const char* limit, fpx_arena* arena, Fpx_Json_Value* output);

// expects first character to be '"'
// returns FPX_JSON_SYNTAX_ERROR otherwise
static Fpx_Json_E_Result _json_string_parse(
  const char** string, const char* limit, fpx_arena* arena, Fpx_Json_String* output);

// expects first character to be either '-' or a number [0-9]
// returns FPX_JSON_SYNTAX_ERROR otherwise
static Fpx_Json_E_Result _json_number_parse(const char** string, const char* limit, double* output);

// expects first chatacter to be either 't' or 'f'
// returns FPX_JSON_SYNTAX_ERROR otherwise
static Fpx_Json_E_Result _json_bool_parse(const char** data, const char* limit, bool* output);

// expects first chatacter to be 'n'
// returns FPX_JSON_SYNTAX_ERROR otherwise
static Fpx_Json_E_Result _json_null_validate(const char** data, const char* limit);

// expects first character to be '['
// returns FPX_JSON_SYNTAX_ERROR otherwise
static Fpx_Json_E_Result _json_array_parse(
  const char** string, const char* limit, fpx_arena* arena, Fpx_Json_Array* output);

static void _json_object_print(Fpx_Json_Object*);
static void _json_array_print(Fpx_Json_Array*);
static void _json_value_print(Fpx_Json_Value*);

Fpx_Json_Entity fpx_json_read(const char* json_data, size_t len) {
  Fpx_Json_Entity retval = { 0 };

  if (NULL == json_data || 0 == len)
    return retval;

  const char* current_char = json_data;
  const char* limit = json_data + len;

  int page_size = 0;

#if defined(_WIN32) || defined(_WIN64)
  page_size = 4096;
#else
  page_size = getpagesize();
#endif

  // REALLY nasty! maybe see if there's a better way
  len *= 2;

  size_t page_diff = 0;

  if (len % page_size != 0) {
    page_diff = (page_size - (len % page_size));
    len += page_diff;
  }

  // allocate an extra page, just in case

  retval.arena = fpx_arena_create(len);

  // trim leading whitespace
  TRIM_WHITESPACE(current_char, limit);

  Fpx_Json_E_Result root_val = _json_value_parse(&current_char, limit, retval.arena, &retval.root);

  retval.isValid = (FPX_JSON_RESULT_SUCCESS == root_val);

  if (false == retval.isValid) {
    fpx_arena_destroy(retval.arena);
    memset(&retval, 0, sizeof(retval));
  }

  return retval;
}

Fpx_Json_E_Result fpx_json_destroy(Fpx_Json_Entity* entity) {
  if (NULL == entity)
    return FPX_JSON_RESULT_ARGUMENT_ERROR;

  if (entity->arena) {
    fpx_arena_destroy(entity->arena);
  }

  memset(entity, 0, sizeof *(entity));
  return FPX_JSON_RESULT_SUCCESS;
}

void fpx_json_print(Fpx_Json_Entity* json) {
  if (NULL == json || false == json->isValid || NULL == json->arena)
    return;

  _json_value_print(&json->root);

  printf("\n");

  return;
}

// STATIC FUNCTIONS BELOW -------------------------

static Fpx_Json_E_Result _json_object_parse(
  const char** string, const char* limit, fpx_arena* arena, Fpx_Json_Object* output) {
  if (NULL == string || NULL == *string || NULL == limit || NULL == arena || NULL == output)
    return FPX_JSON_RESULT_ARGUMENT_ERROR;

  const char* data = *string;

#define RETURN(_return_value)                    \
  {                                              \
    for (; *data != '}' && data < limit; ++data) \
      ;                                          \
    if (data != limit)                           \
      ++data;                                    \
    *string = data;                              \
    return _return_value;                        \
  }


  if (limit - data < 2) {
    // no room for "{}"
    return FPX_JSON_RESULT_OUT_OF_BOUNDS_ERROR;
  }

  if (*data != '{') {
    SYNTAX_EXPECT(data, '{');
    return FPX_JSON_RESULT_SYNTAX_ERROR;
  }

  ++data;

  TRIM_WHITESPACE(data, limit);

  if (*data == '}') {
    memset(output, 0, sizeof(*output));
    RETURN(FPX_JSON_RESULT_SUCCESS);
  }

  if (data >= limit)
    return FPX_JSON_RESULT_OUT_OF_BOUNDS_ERROR;

  Fpx_Json_Object new_obj = { 0 };

  uint8_t member_increment = 8;
  uint16_t member_capacity = 0;

  do {
    if (member_capacity == new_obj.memberCount) {
      new_obj.members = (Fpx_Json_Member*)realloc(
        new_obj.members, sizeof(Fpx_Json_Member) * (new_obj.memberCount + member_increment));
      member_capacity += member_increment;
    }

    Fpx_Json_E_Result member_result =
      _json_member_parse(&data, limit, arena, &new_obj.members[new_obj.memberCount]);

    if (FPX_JSON_RESULT_SUCCESS > member_result) {
      free(new_obj.members);
      return member_result;
    }

    new_obj.memberCount++;

    TRIM_WHITESPACE(data, limit);
    if (*data != ',' && *data != '}') {
      free(new_obj.members);
      SYNTAX_EXPECT(data, ',');
      SYNTAX_EXPECT(data, '}');
      return FPX_JSON_RESULT_SYNTAX_ERROR;
    }
    if (*data == ',')
      ++data;

    TRIM_WHITESPACE(data, limit);
  } while (data < limit && *data != '}');

  *output = new_obj;
  RETURN(FPX_JSON_RESULT_SUCCESS);

#undef RETURN
}

static Fpx_Json_E_Result _json_member_parse(
  const char** dataptr, const char* limit, fpx_arena* alloc_arena, Fpx_Json_Member* output) {
  if (NULL == dataptr || NULL == *dataptr || NULL == limit || NULL == alloc_arena || NULL == output)
    return FPX_JSON_RESULT_ARGUMENT_ERROR;

#define RETURN(_return_value) \
  {                           \
    *dataptr = data;          \
    return _return_value;     \
  }

  const char* data = *dataptr;

  Fpx_Json_Member new_member = { 0 };

  new_member.value = fpx_arena_alloc(alloc_arena, sizeof(*new_member.value));

  if (NULL == new_member.value) {
    return FPX_JSON_RESULT_MEMORY_ERROR;
  }

  Fpx_Json_E_Result key_result = _json_string_parse(&data, limit, alloc_arena, &new_member.key);

  if (FPX_JSON_RESULT_SUCCESS > key_result)
    return key_result;

  TRIM_WHITESPACE(data, limit);
  if (*data != ':') {
    SYNTAX_EXPECT(data, ':');
    return FPX_JSON_RESULT_SYNTAX_ERROR;
  }

  ++data;
  TRIM_WHITESPACE(data, limit);

  // parse value
  Fpx_Json_E_Result val_res = _json_value_parse(&data, limit, alloc_arena, new_member.value);

  if (FPX_JSON_RESULT_SUCCESS > val_res)
    return val_res;

  *output = new_member;

  RETURN(FPX_JSON_RESULT_SUCCESS);

#undef RETURN
}

static Fpx_Json_E_Result _json_value_parse(
  const char** dataptr, const char* limit, fpx_arena* arena, Fpx_Json_Value* output) {
  if (NULL == dataptr || NULL == *dataptr || NULL == arena || NULL == output)
    return FPX_JSON_RESULT_ARGUMENT_ERROR;
  const char* data = *dataptr;
#define RETURN(_return_value) \
  {                           \
    *dataptr = data;          \
    return _return_value;     \
  }

  Fpx_Json_Value new_val = { 0 };
  Fpx_Json_E_ValueType type = FPX_JSON_VALUE_INVALID;
  switch (*data) {
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'i':  // infinity
    case 'I':
      type = FPX_JSON_VALUE_NUMBER;
      Fpx_Json_E_Result num_res = _json_number_parse(&data, limit, &new_val.number);
      if (FPX_JSON_RESULT_SUCCESS > num_res)
        return num_res;
      break;

    case 't':
    case 'f':
      type = FPX_JSON_VALUE_BOOL;
      Fpx_Json_E_Result bool_res = _json_bool_parse(&data, limit, &new_val.boolean);
      if (FPX_JSON_RESULT_SUCCESS > bool_res)
        return bool_res;
      break;

    case 'n':
      type = FPX_JSON_VALUE_NULL;
      Fpx_Json_E_Result null_res = _json_null_validate(&data, limit);
      if (FPX_JSON_RESULT_SUCCESS > null_res)
        return null_res;
      break;

    case '{':
      type = FPX_JSON_VALUE_OBJECT;
      Fpx_Json_E_Result mem_res = _json_object_parse(&data, limit, arena, &new_val.object);
      if (FPX_JSON_RESULT_SUCCESS > mem_res)
        return mem_res;
      break;

    case '[':
      type = FPX_JSON_VALUE_ARRAY;
      Fpx_Json_E_Result arr_res = _json_array_parse(&data, limit, arena, &new_val.array);
      if (FPX_JSON_RESULT_SUCCESS > arr_res)
        return arr_res;
      break;

    case '"':
      type = FPX_JSON_VALUE_STRING;
      Fpx_Json_E_Result str_res = _json_string_parse(&data, limit, arena, &new_val.string);
      if (FPX_JSON_RESULT_SUCCESS > str_res)
        return str_res;
      break;

    default:
      fprintf(stderr, "General syntax error while checking for value\n");
      return FPX_JSON_RESULT_SYNTAX_ERROR;
  }

  new_val.valueType = type;

  *output = new_val;

  RETURN(FPX_JSON_RESULT_SUCCESS);

#undef RETURN
}

static Fpx_Json_E_Result _json_string_parse(
  const char** in_string, const char* limit, fpx_arena* arena, Fpx_Json_String* output) {
  Fpx_Json_String retval = { 0 };

  const char* data = *in_string;

#define RETURN(_return_value) \
  {                           \
    if (data != limit)        \
      ++data;                 \
    *in_string = data;        \
    return _return_value;     \
  }

  if (limit - data < 2) {
    // no room for ""
    return FPX_JSON_RESULT_OUT_OF_BOUNDS_ERROR;
  }

  if (*data != '\"') {
    SYNTAX_EXPECT(data, '"');
    return FPX_JSON_RESULT_SYNTAX_ERROR;
  }

  ++data;

  // empty string
  if (*data == '\"') {
    output->size = 0;
    output->data = NULL;

    RETURN(FPX_JSON_RESULT_SUCCESS);
  }

  const char* next_dbl_quote = data;

  bool escaped = false;
  for (; next_dbl_quote <= limit; ++next_dbl_quote) {
    if (!escaped) {
      if (*next_dbl_quote == '\\') {
        escaped = true;
        continue;
      } else if (*next_dbl_quote == '"')
        break;
    } else {
      escaped = false;
      continue;
    }
  }

  if (*next_dbl_quote != '\"')
    return FPX_JSON_RESULT_OUT_OF_BOUNDS_ERROR;

  size_t clone_idx = 0;
  retval.data = (char*)calloc(next_dbl_quote - data, 1);

  for (; data < next_dbl_quote; ++data) {
    if (*data == '\\') {
      ++data;
      uint8_t to_clone = ' ';  // filler in case of weird logic error

      if (*data == '"' || *data == '\\' || *data == '/')
        to_clone = *data;
      else if (*data == 'r')
        to_clone = '\r';
      else if (*data == 'n')
        to_clone = '\n';
      else if (*data == 't')
        to_clone = '\t';
      else if (*data == 'b')
        to_clone = '\b';
      else if (*data == 'f')
        to_clone = '\f';

      if (*data != 'u')
        retval.data[clone_idx++] = to_clone;
      else {
        // ESCAPE UNICODE 2 BYTES

        // if unicode escape sequence reaches past the next double quote:
        if (data + 5 > next_dbl_quote) {
          free(retval.data);
          fprintf(stderr, "NO DBL QUOTE IN RANGE\n");
          return FPX_JSON_RESULT_SYNTAX_ERROR;
        }

        // should be good for both endiannesses
        for (size_t _iter = 0; _iter < 2; ++_iter) {
          uint8_t byteval = 0;

          for (size_t _iter2 = 0; _iter2 < 2; ++_iter2) {
            byteval <<= 4;
            ++data;
            uint8_t nibble = 0;
            if (*data >= 'A' && *data <= 'F')
              nibble = *data - 55;
            else if (*data >= 'a' && *data <= 'f')
              nibble = *data - 87;

            byteval |= nibble & 0x0f;
          }

          retval.data[clone_idx++] = byteval;
        }
      }
    } else if (*data >= ' ' && *data <= '~') {
      retval.data[clone_idx++] = *data;
    }
  }

  void* temp = fpx_arena_alloc(arena, clone_idx + 1);
  if (NULL == temp) {
    free(retval.data);
    return FPX_JSON_RESULT_MEMORY_ERROR;
  }

  memcpy(temp, retval.data, clone_idx);
  free(retval.data);
  retval.size = clone_idx;
  retval.data = temp;

  retval.data[clone_idx] = 0;

  *output = retval;

  RETURN(FPX_JSON_RESULT_SUCCESS);

#undef RETURN
}

static Fpx_Json_E_Result _json_number_parse(
  const char** string, const char* limit, double* output) {
  const char* data = *string;

#define RETURN(_return_value) \
  {                           \
    *string = data;           \
    return _return_value;     \
  }

  uint8_t min_space = 1;
  if (*data == 'i' || *data == 'I') {
    min_space = 8;
    if (0 != strncasecmp("infinity", data, 8)) {
      fprintf(stderr, "BAD NUMBER\n");
      return FPX_JSON_RESULT_SYNTAX_ERROR;
    }
  } else if (*data == '-')
    min_space = 2;
  else if (*data < '0' || *data > '9') {
    fprintf(stderr, "BAD NUMBER\n");
    return FPX_JSON_RESULT_SYNTAX_ERROR;
  }

  if (limit - data < min_space)
    return FPX_JSON_RESULT_OUT_OF_BOUNDS_ERROR;

  const char* number_end = NULL;

  *output = strtod(data, (char**)&number_end);

  if (number_end >= limit)
    return FPX_JSON_RESULT_OUT_OF_BOUNDS_ERROR;

  data = number_end;

  RETURN(FPX_JSON_RESULT_SUCCESS);

#undef RETURN
}

static Fpx_Json_E_Result _json_bool_parse(const char** string, const char* limit, bool* output) {
  if (NULL == string || NULL == *string || NULL == limit || NULL == output)
    return FPX_JSON_RESULT_ARGUMENT_ERROR;
  const char* data = *string;

#define RETURN(_return_value) \
  {                           \
    *string = data;           \
    return _return_value;     \
  }

  uint8_t min_space = 4;

  if (*data == 'f')
    min_space = 5;
  else if (*data != 't') {
    SYNTAX_EXPECT(data, 't');
    SYNTAX_EXPECT(data, 'f');
    return FPX_JSON_RESULT_SYNTAX_ERROR;
  }

  if (limit - data < min_space)
    return FPX_JSON_RESULT_OUT_OF_BOUNDS_ERROR;

  if (*data == 't' && 0 == strncmp("true", data, 4)) {
    data += 4;
    *output = true;
  } else if (*data == 'f' && 0 == strncmp("false", data, 5)) {
    data += 5;
    *output = false;
  } else {
    fprintf(stderr, "BOOL SYNTAX ERROR\n");
    return FPX_JSON_RESULT_SYNTAX_ERROR;
  }

  RETURN(FPX_JSON_RESULT_SUCCESS);

#undef RETURN
}

static Fpx_Json_E_Result _json_null_validate(const char** string, const char* limit) {
  if (NULL == string || NULL == *string || NULL == limit)
    return FPX_JSON_RESULT_ARGUMENT_ERROR;
  const char* data = *string;

#define RETURN(_return_value) \
  {                           \
    *string = data;           \
    return _return_value;     \
  }

  if (*data != 'n') {
    fprintf(stderr, "NULL NOT NULL\n");
    return FPX_JSON_RESULT_SYNTAX_ERROR;
  }

  if (0 != strncmp("null", data, 4)) {
    fprintf(stderr, "NULL NOT NULL\n");
    return FPX_JSON_RESULT_SYNTAX_ERROR;
  }

  data += 4;

  RETURN(FPX_JSON_RESULT_SUCCESS);

#undef RETURN
}

static Fpx_Json_E_Result _json_array_parse(
  const char** string, const char* limit, fpx_arena* arena, Fpx_Json_Array* output) {
  if (NULL == string || NULL == *string || NULL == limit || NULL == arena || NULL == output)
    return FPX_JSON_RESULT_ARGUMENT_ERROR;
  const char* data = *string;

#define RETURN(_return_value)                    \
  {                                              \
    for (; *data != ']' && data < limit; ++data) \
      ;                                          \
    if (data != limit)                           \
      ++data;                                    \
    *string = data;                              \
    return _return_value;                        \
  }

  if (*data != '[') {
    SYNTAX_EXPECT(data, '[');
    return FPX_JSON_RESULT_SYNTAX_ERROR;
  }

  ++data;

  TRIM_WHITESPACE(data, limit);

  if (*data == ']') {
    memset(output, 0, sizeof(*output));
    RETURN(FPX_JSON_RESULT_SUCCESS);
  }

  uint8_t value_increment = 8;
  uint16_t value_capacity = 0;

  Fpx_Json_Array new_arr = { 0 };

  do {
    if (value_capacity == new_arr.count) {
      new_arr.values = (Fpx_Json_Value*)realloc(
        new_arr.values, sizeof(Fpx_Json_Value) * (new_arr.count + value_increment));
      value_capacity += value_increment;
    }

    Fpx_Json_E_Result val_res =
      _json_value_parse(&data, limit, arena, &new_arr.values[new_arr.count]);

    if (FPX_JSON_RESULT_SUCCESS > val_res) {
      free(new_arr.values);
      return val_res;
    }

    TRIM_WHITESPACE(data, limit);
    if (*data != ',' && *data != ']') {
      free(new_arr.values);
      SYNTAX_EXPECT(data, ']');
      SYNTAX_EXPECT(data, ',');
      return FPX_JSON_RESULT_SYNTAX_ERROR;
    }
    if (*data == ',')
      ++data;

    TRIM_WHITESPACE(data, limit);

    new_arr.count++;
  } while (data < limit && *data != ']');


  output->values = fpx_arena_alloc(arena, sizeof(Fpx_Json_Value) * new_arr.count);

  if (NULL == output->values) {
    return FPX_JSON_RESULT_MEMORY_ERROR;
  }

  memcpy(output->values, new_arr.values, new_arr.count * sizeof(Fpx_Json_Value));
  output->count = new_arr.count;


  RETURN(FPX_JSON_RESULT_SUCCESS);
}

static void _json_object_print(Fpx_Json_Object* obj) {
  if (NULL == obj)
    return;

  printf("{ ");
  for (size_t i = 0; i < obj->memberCount; ++i) {
    Fpx_Json_Member* m = &obj->members[i];

    printf("\"%s\" : ", m->key.data);

    _json_value_print(m->value);

    if (i < (obj->memberCount - 1))
      printf(", ");
  }
  printf(" }");

  return;
}

static void _json_array_print(Fpx_Json_Array* arr) {
  if (NULL == arr)
    return;

  printf("[ ");
  for (size_t i = 0; i < arr->count; ++i) {
    _json_value_print(&arr->values[i]);

    if (i < (arr->count - 1))
      printf(", ");
  }
  printf(" ]");
}

static void _json_value_print(Fpx_Json_Value* val) {
  if (NULL == val)
    return;

  switch (val->valueType) {
    case FPX_JSON_VALUE_BOOL:
      printf("%s", (val->boolean) ? "true" : "false");
      break;

    case FPX_JSON_VALUE_NULL:
      printf("null");
      break;

    case FPX_JSON_VALUE_NUMBER:
      printf("%lf", val->number);
      break;

    case FPX_JSON_VALUE_STRING:
      printf("\"%s\"", val->string.data);
      break;

    case FPX_JSON_VALUE_OBJECT:
      _json_object_print(&val->object);
      break;

    case FPX_JSON_VALUE_ARRAY:
      _json_array_print(&val->array);
      break;

    default:
      break;
  }

  return;
}
