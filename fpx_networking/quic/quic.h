#ifndef FPX_QUIC_H
#define FPX_QUIC_H

#include "../../fpx_types.h"

////////////////////////////////////////////////////////////////
//  "quic.h"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

// implements [https://datatracker.ietf.org/doc/html/rfc9000]

typedef union {
  uint8_t   BYTE;
  uint16_t  WORD;
  uint32_t DWORD;
  uint64_t QWORD;
} fpx_quic_varlen_t;

struct AckRange {
  union Value {
    fpx_quic_varlen_t Gap;
    fpx_quic_varlen_t AckRangeLength;
  };
};

struct EcnCounts {
  fpx_quic_varlen_t ECT0;
  fpx_quic_varlen_t ECT1;
  fpx_quic_varlen_t ECN_CE;
};

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
  // Types:
  //
  // 0x00: PADDING      | 0x01: PING          | 0x02: ACK NO ECN  | 0x03: ACK YES ECN
  //
  // 0x04: RESET_STREAM | 0x05: STOP_SENDING  | 0x06: CRYPTO      | 0x07: NEW_TOKEN
  //
  // 0x08: STREAM       | 0x09: STREAM        | 0x0a: STREAM      | 0x0b: STREAM
  // 0x0c: STREAM       | 0x0d: STREAM        | 0x0e: STREAM      | 0x0f: STREAM
  //
  // 0x10: MAX_DATA     |

  union FrameData {

    struct Ack {
      // https://datatracker.ietf.org/doc/html/rfc9000#name-ack-frames
      fpx_quic_varlen_t LargestAcknowledged; // highest packet number received when acknowledging
      fpx_quic_varlen_t AckDelay; // (microseconds after receiving frame) / (ack_delay_exponent transport parameter)
      fpx_quic_varlen_t AckRangeCount;
      fpx_quic_varlen_t FirstAckOffset; // amount of contiguous packets that came before the LargestAcknowledged
      struct AckRange* AckRanges;
      struct EcnCounts* EcnCounts;
    };

    struct ResetStream {
      // https://datatracker.ietf.org/doc/html/rfc9000#name-reset_stream-frames
      fpx_quic_varlen_t ApplicationError;
      fpx_quic_varlen_t FinalStreamSize;
    };

    struct StopSending {
      // https://datatracker.ietf.org/doc/html/rfc9000#name-stop_sending-frames
      fpx_quic_varlen_t ApplicationError;
    };

    struct Crypto {
      // https://datatracker.ietf.org/doc/html/rfc9000#name-crypto-frames
      fpx_quic_varlen_t Offset; // offset of CryptoData within the whole stream
      fpx_quic_varlen_t CryptoLength;
      uint8_t* CryptoData;
    };

    struct NewToken {
      // https://datatracker.ietf.org/doc/html/rfc9000#name-new_token-frames
      // new token for the receiver to use when initializing a new connection to the peer
      fpx_quic_varlen_t TokenLength;
      uint8_t* TokenBlob;
    };

    struct Stream {
      // https://datatracker.ietf.org/doc/html/rfc9000#name-stream-frames
      // if fin bit set (Type & 0x01), this is the last frame of the stream
      // a.k.a.: 
      fpx_quic_varlen_t Offset; // only if offset bit set (Type & 0x04);
      // ^^^ this bit is typically set, unless it's either
      // the first frame of the stream, or the last one that contains no data
      fpx_quic_varlen_t Length; // only if fin bit set (Type & 0b02)
      uint8_t* Data;
    };

    struct MaxData {
      // https://datatracker.ietf.org/doc/html/rfc9000#name-max_data-frames
      // sent by the server to indicate flow control credit
      fpx_quic_varlen_t MaximumData;
    };

    struct MaxStreamData {

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
