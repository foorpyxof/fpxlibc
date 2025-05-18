#ifndef FPX_DEBUG_H
#define FPX_DEBUG_H

////////////////////////////////////////////////////////////////
//  "fpx_debug.h"                                             //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include <stdio.h>


#define FPX_DEBUG(fmt, ...)
#define FPX_WARN(fmt, ...)
#define FPX_ERROR(fmt, ...)

#define FPX_LINE_INFO(output)


#if (defined __FILE__ && defined __LINE__)

#undef FPX_LINE_INFO
#define FPX_LINE_INFO(output) sprintf(output, "%s:%d", __FILE__, __LINE__)

#endif  // __FILE__ && __LINE__


#ifdef FPX_DEBUG_ENABLE

#undef FPX_DEBUG
#define FPX_DEBUG(fmt, ...) \
  fprintf(stderr, "\e[0;32mFPXLIBC DEBUG:\e[0m " fmt "\e[0m" __VA_OPT__(, ) __VA_ARGS__)

#undef FPX_WARN
#define FPX_WARN(fmt, ...) \
  fprintf(stderr, "\e[0;93mFPXLIBC WARN: \e[0m " fmt "\e[0m" __VA_OPT__(, ) __VA_ARGS__)

#endif  // FPX_DEBUG_ENABLE


#ifndef FPX_SILENT_ERROR
#undef FPX_ERROR
#define FPX_ERROR(fmt, ...)                                                            \
  {                                                                                    \
    char _fpx_lineinfo_output_buffer[sizeof(__FILE__) + 8];                            \
    FPX_LINE_INFO(_fpx_lineinfo_output_buffer);                                        \
    fprintf(stderr,                                                                    \
      "\e[0;91mFPXLIBC ERROR:\e[0m " fmt "\e[0m (at %s)\n" __VA_OPT__(, ) __VA_ARGS__, \
      _fpx_lineinfo_output_buffer);                                                    \
  }
#endif  // FPX_SILENT_ERROR


#endif  // FPX_DEBUG_H
