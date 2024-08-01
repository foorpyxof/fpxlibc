#ifndef FPX_STRING_H
#define FPX_STRING_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

        // _Generics don't seem to work :/
/* 
#define fpx_string_to_upper(input) _Generic(    \
  input,                                      \
  char*: fpx_string_to_upper_STACK,             \
  const char*: fpx_string_to_upper_HEAP         \
)(input)
*/

/*
#define fpx_string_to_lower(input) _Generic(    \
  input,                                      \
  char*: fpx_string_to_lower_STACK,             \
  const char*: fpx_string_to_lower_HEAP         \
)(input)
*/

int fpx_getstringlength(const char*);
int fpx_substringindex(const char* haystack, const char* needle);
char* fpx_substr_replace(const char* haystack, const char* needle, const char* replacement);
char* fpx_string_to_upper(const char* input, int doReturn);
char* fpx_string_to_lower(const char* input, int doReturn);

#endif /* FPX_STRING_H */