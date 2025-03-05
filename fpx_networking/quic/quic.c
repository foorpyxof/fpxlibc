////////////////////////////////////////////////////////////////
//  "quic.c"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "quic.h"

static int _quic_can_send(fpx_quicstream_t* stream) {
  // code that checks whether or not the stream is able to have data sent over it
  // (a.k.a. is flow control credit reserved?)
  return 0;
}
