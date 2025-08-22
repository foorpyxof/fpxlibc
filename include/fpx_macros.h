#ifndef FPX_MACROS_H
#define FPX_MACROS_H

#include "fpx_debug.h"

#define UNUSED(var)                                          \
  {                                                          \
    char _li[sizeof(__FILE__) + 16] = { 0 };                 \
    FPX_LINE_INFO(_li);                                      \
    FPX_DEBUG("Variable '%s' is unused (at %s)", #var, _li); \
    if (var) { }                                             \
  }

#endif  // FPX_MACROS_H
