#ifndef FPX_DEBUG_H
#define FPX_DEBUG_H

#include <stdio.h>
#include <time.h>

#ifdef FPX_DEBUG_ENABLE

#define FPX_DEBUG(fmt, ...) \
  fprintf(stderr, "\e[0;32mFPXLIBC DEBUG:\e[0m " fmt "\e[0m\n" __VA_OPT__(, ) __VA_ARGS__)
#define FPX_WARN(fmt, ...) \
  fprintf(stderr, "\e[0;93mFPXLIBC WARN: \e[0m " fmt "\e[0m\n" __VA_OPT__(, ) __VA_ARGS__)
#define FPX_ERROR(fmt, ...) \
  fprintf(stderr, "\e[0;91mFPXLIBC ERROR:\e[0m " fmt "\e[0m\n" __VA_OPT__(, ) __VA_ARGS__)

#else

#define FPX_DEBUG(fmt, ...)
#define FPX_WARN(fmt, ...)
#define FPX_ERROR(fmt, ...)

#endif  // FPX_DEBG_ENABLE


#endif  // FPX_DEBUG_H
