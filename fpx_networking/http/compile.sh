#!/bin/bash

gcc -g \
  -DFPX_DEBUG_ENABLE \
  test.c \
  http.c \
  httpserver_c.c \
  ../../build/unlinked/mem* \
  ../../build/unlinked/string* \
  ../../build/unlinked/format* \
  ../../build/unlinked/math* \
  ../../build/unlinked/endian*
