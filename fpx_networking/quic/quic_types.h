#ifndef FPX_QUIC_TYPES_H
#define FPX_QUIC_TYPES_H

////////////////////////////////////////////////////////////////
//  "quic_types.h"                                            //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "../../fpx_types.h"

#include <netinet/in.h>

// variable length integer
// https://datatracker.ietf.org/doc/html/rfc9000#name-variable-length-integer-enc
// this union allows easy checking of the first two MSB to identify the amount of bytes to read
typedef union {
  uint8_t BYTE;
  uint16_t WORD;
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

struct Ack {
  // https://datatracker.ietf.org/doc/html/rfc9000#name-ack-frames
  fpx_quic_varlen_t LargestAcknowledged;  // highest packet number received when acknowledging
  fpx_quic_varlen_t
    AckDelay;  // (microseconds after receiving frame) / (ack_delay_exponent transport parameter)
  fpx_quic_varlen_t AckRangeCount;
  fpx_quic_varlen_t
    FirstAckOffset;  // amount of contiguous packets that came before the LargestAcknowledged
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
  fpx_quic_varlen_t Offset;  // offset of CryptoData within the whole stream
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
  fpx_quic_varlen_t Offset;  // only if offset bit set (Type & 0x04);
  // ^^^ this bit is typically set, unless it's either
  // the first frame of the stream, or the last one that contains no data
  fpx_quic_varlen_t Length;  // only if fin bit set (Type & 0b02)
  uint8_t* Data;
};

struct MaxData {
  // https://datatracker.ietf.org/doc/html/rfc9000#name-max_data-frames
  // sent by the server to indicate flow control credit
  fpx_quic_varlen_t MaximumData;
};

struct MaxStreamData {
  // https://datatracker.ietf.org/doc/html/rfc9000#name-max_stream_data-frames
  fpx_quic_varlen_t MaximumStreamData;
};

struct MaxStreams {
  // https://datatracker.ietf.org/doc/html/rfc9000#name-max_streams-frames
  fpx_quic_varlen_t MaximumStreams;
};

struct DataBlocked {
  // https://datatracker.ietf.org/doc/html/rfc9000#name-data_blocked-frames
  fpx_quic_varlen_t CurrentLimit;
};

struct StreamDataBlocked {
  // https://datatracker.ietf.org/doc/html/rfc9000#name-stream_data_blocked-frames
  fpx_quic_varlen_t CurrentStreamLimit;
};

struct StreamsBlocked {
  // https://datatracker.ietf.org/doc/html/rfc9000#name-streams_blocked-frames
  fpx_quic_varlen_t CurrentLimit;
};

struct NewConnectionId {
  // https://datatracker.ietf.org/doc/html/rfc9000#name-new_connection_id-frames
  fpx_quic_varlen_t SequenceNumber;
  fpx_quic_varlen_t RetirePriorTo;
  uint8_t Length;
  uint8_t* ConnectionID;
  uint16_t StatelessResetToken;
};

struct RetireConnectionId {
  // https://datatracker.ietf.org/doc/html/rfc9000#name-retire_connection_id-frames
  fpx_quic_varlen_t SequenceNumber;
};

struct PathChallenge {
  // https://datatracker.ietf.org/doc/html/rfc9000#name-path_challenge-frames
  uint64_t Data;
};

struct PathResponse {
  // https://datatracker.ietf.org/doc/html/rfc9000#name-path_response-frames
  uint64_t Data;
};

struct ConnectionClose {
  // https://datatracker.ietf.org/doc/html/rfc9000#name-connection_close-frames
  fpx_quic_varlen_t ErrorCode;
  fpx_quic_varlen_t FrameType;
  fpx_quic_varlen_t ReasonLength;
  uint8_t* Reason;
};

union FrameData {
  struct Ack Ack;
  struct ResetStream ResetStream;
  struct StopSending StopSending;
  struct Crypto Crypto;
  struct NewToken NewToken;
  struct Stream Stream;
  struct MaxData MaxData;
  struct MaxStreamData MaxStreamData;
  struct MaxStreams MaxStreams;
  struct DataBlocked DataBlocked;
  struct StreamDataBlocked StreamDataBlocked;
  struct StreamsBlocked StreamsBlocked;
  struct NewConnectionId NewConnectionId;
  struct RetireConnectionId RetireConnectionId;
  struct PathChallenge PathChallenge;
  struct PathResponse PathResponse;
  struct ConnectionClose ConnectionClose;
};

typedef struct {
  int FileDescriptor;
  struct sockaddr_in PeerAddress;
} fpx_quic_connection_t;

typedef struct {
  fpx_quic_connection_t* Connection;
  uint64_t StreamID;

  // 0b0000 00xy where:
  //  x is 0 (bi-) or 1 (uni-directional)
  //  y is 0 (client-) or 1 (server-initiated)
  uint8_t Type;

  uint8_t WriteBuffer[1024];  // contains frames(?)
  uint8_t ReadBuffer[1024];
} fpx_quic_stream_t;

typedef struct {
  uint64_t StreamID;
  uint16_t Datalen;
  uint8_t Type;
  // Types:
  //
  // 0x00: PADDING            | 0x01: PING
  // 0x02: ACK NO ECN         | 0x03: ACK YES ECN
  // 0x04: RESET_STREAM       | 0x05: STOP_SENDING
  // 0x06: CRYPTO             | 0x07: NEW_TOKEN
  //
  // 0x08: STREAM             | 0x09: STREAM
  // 0x0a: STREAM             | 0x0b: STREAM
  // 0x0c: STREAM             | 0x0d: STREAM
  // 0x0e: STREAM             | 0x0f: STREAM
  //
  // 0x10: MAX_DATA           | 0x11: MAX_STREAM_DATA
  // 0x12: MAX_STREAMS        | 0x13: MAX_STREAMS
  // 0x14: DATA_BLOCKED       | 0x15: STREAM_DATA_BLOCKED
  // 0x16: STREAMS_BLOCKED    | 0x17: STREAMS_BLOCKED
  //
  // 0x18: NEW_CONNECTION_ID  | 0x19: RETIRE_CONNECTION_ID
  // 0x1a: PATH_CHALLENGE     | 0x1b: PATH_RESPONSE
  // 0x1c: CONNECTION_CLOSE   | 0x1d: CONNECTION_CLOSE
  // 0x1e: HANDSHAKE_DONE

  union FrameData FrameData;
} fpx_quic_frame_t;

typedef struct {
  pthread_t Thread;
  pthread_mutex_t ThreadMutex;

  fpx_quic_connection_t* Backlog;
  size_t BacklogLength;
} fpx_quic_socket_t;

#endif  // FPX_QUIC_TYPES_H
