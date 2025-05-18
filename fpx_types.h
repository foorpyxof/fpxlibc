#ifndef FPX_TYPES_H
#define FPX_TYPES_H

typedef unsigned long size_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

typedef float fl32_t;
typedef double fl64_t;

typedef unsigned long size_t;
typedef signed long ssize_t;

#ifndef NULL
  #define NULL ((void*)0)
#endif // NULL

#define TRUE 1
#define FALSE 0

#endif  // FPX_TYPES_H
