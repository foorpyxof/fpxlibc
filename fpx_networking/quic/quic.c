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

static uint8_t _quic_get_varlen(fpx_quic_varlen_t variable) {
  // to make it a bit easier to handle, we're gonna only grab the
  // first byte from the variable-length integer, since we only need
  // the two most significant bits anyway

  uint8_t first_byte = variable.BYTE;

  if (first_byte & 0x80)
    if (first_byte & 0x40) return 8;  // 0b1100 0000
    else return 4;                    // 0b1000 0000
  else
    if (first_byte & 0x40) return 2;  // 0b0100 0000
    else return 1;                    // 0b0000 0000
}
