#ifndef FPX_QUIC_H
#define FPX_QUIC_H

#include "../../fpx_types.h"

////////////////////////////////////////////////////////////////
//  "quic.h"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

// implements [https://datatracker.ietf.org/doc/html/rfc9000]

typedef struct {
  uint64_t StreamID;

  // 0b0000 00xy where:
  //  x is 0 (bi-) or 1 (uni-directional)
  //  y is 0 (client-) or 1 (server-initiated)
  uint8_t Type;

  uint8_t WriteBuffer[1024]; // contains frames(?)
  uint8_t ReadBuffer[1024];
} fpx_quicstream_t;

typedef struct {
  uint64_t StreamID;
  uint16_t Datalen;
  uint8_t Type;
  union FrameData {
    struct SingleByte {
      uint8_t Type; // 0x00 for PADDING, 0x01 for PING
    };
    struct Ack {
      uint8_t Type;
    };
    // TODO: add more frame types
  };
} fpx_quicframe_t;

int fpx_quicstream_write(fpx_quicstream_t* stream, fpx_quicframe_t* frame); // write to stream->WriteBuffer
int fpx_quicstream_read(fpx_quicstream_t* stream, fpx_quicframe_t* dst_frame); // read frame data into dst_frame

int fpx_quicstream_flush(fpx_quicstream_t* stream); // write current contents of stream buffer into socket, and flush buffer

int fpx_quicstream_fin(fpx_quicstream_t* stream);
int fpx_quicstream_rst(fpx_quicstream_t* stream);

int fpx_quicstream_abortread(fpx_quicstream_t* stream);

#endif // FPX_QUIC_H
