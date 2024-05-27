#ifndef FPX_STRING_H
#define FPX_STRING_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


int fpx_getstringlength(const char*);
int fpx_substringindex(const char*, const char*);
char* fpx_substr_replace(const char*, const char*, const char*);
char* fpx_string_to_upper(const char*);
char* fpx_string_to_lower(const char*);

#endif /* FPX_STRING_H */