#ifndef FPX_STRING_H
#define FPX_STRING_H

////////////////////////////////////////////////////////////////
//  "string.h"                                                //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

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

/**
 * Returns the length of a null terminated string (null-byte not included).
 */
extern int fpx_getstringlength(const char*);

/**
 * Copies from source to dest, based on null-terminated string length
 */
extern char* fpx_strcpy(char* dst, const char* src);

/**
 * Returns the index of a given substring within a string.
 */
int fpx_substringindex(const char* haystack, const char* needle);

/**
 * Replaces a substring within a string with another string.
 */
char* fpx_substr_replace(const char* haystack, const char* needle, const char* replacement);

/**
 * Converts the passed string to uppercase.
 * Second argument is:
 * - true to return a pointer to a malloc'd string
 * - false to modify the input string
 */
char* fpx_string_to_upper(const char* input, int doReturn);

/**
 * Converts the passed string to lowercase.
 * Second argument is:
 * - true to return a pointer to a malloc'd string
 * - false to modify the input string
 */
char* fpx_string_to_lower(const char* input, int doReturn);

#endif /* FPX_STRING_H */
