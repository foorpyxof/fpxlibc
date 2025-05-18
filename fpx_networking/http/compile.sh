#!/bin/bash

CFLAGS="-g -O3"

if [[ "$1" == "debug" ]]; then
  CFLAGS+=" -DFPX_DEBUG_ENABLE"
fi

gcc \
  test.c \
  http.c \
  httpserver.c \
  ../../build/unlinked/mem* \
  ../../build/unlinked/string* \
  ../../build/unlinked/format* \
  ../../build/unlinked/math* \
  ../../build/unlinked/endian* \
  ../../build/unlinked/crypto.o \
  $CFLAGS \
  -o c.out
