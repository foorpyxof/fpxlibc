#!/bin/bash

CFLAGS="-g -O0"

if [[ "$1" == "debug" ]]; then
  CFLAGS+=" -DFPX_DEBUG_ENABLE"
fi

clang \
  test.c \
  http.c \
  httpserver_c.c \
  ../../build/unlinked/mem* \
  ../../build/unlinked/string* \
  ../../build/unlinked/format* \
  ../../build/unlinked/math* \
  ../../build/unlinked/endian* \
  ../../build/unlinked/crypto.o \
  $CFLAGS
