#ifndef FPX_QUIC_TYPES_H
#define FPX_QUIC_TYPES_H

//
//  "quic_types.h"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "../../fpx_types.h"
#include "../netutils.h"
#include "quic_macros.h"
#include <pthread.h>

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
    } Value;
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
    uint8_t IpVersion;
    int FileDescriptor;
    struct sockaddr PeerAddress;

    // QUIC v1 only supports a maximum of 20 bytes for an ID;
    // For future expandability, RFC 9000 requires servers
    // to allow reading of up to 255 bytes.
    uint8_t RemoteConnectionIDs[ACTIVE_CONNECTION_ID_LIMIT][256];
    uint8_t RemoteConnectionIdLengths[ACTIVE_CONNECTION_ID_LIMIT];

    uint8_t LocalConnectionIDs[ACTIVE_CONNECTION_ID_LIMIT][QUIC_CONNECTION_ID_LENGTH];
    // we're using a fixed-length Local Connection ID,
    // because we need to know how many bytes to read
    // from a short-header packet.
    // all in all, this means we're not using a variable for this value.
    // instead, it is defined in the macro "QUIC_CONNECTION_ID_LENGTH"
    // within the macros file (quic_macros.h)

    uint32_t ProtocolVersion;
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
    union _packetNumber {
        uint8_t BYTE;
        uint16_t WORD;
        uint32_t DWORD;
    } PacketNumber;

    uint8_t FirstByte;  // MSB is HeaderForm; 0 for short, 1 for long
    union _data {
        union _shortHeader {
            struct _oneRTT {
                // in this packet type, the first byte contains: (MSB to LSB)
                //  HeaderForm bit,
                //  FixedBit,
                //  SpinBit,
                //  ReservedBits (2),
                //  KeyPhase bit,
                //  PacketNumberLength (2); ENCODED!
                //  (https://datatracker.ietf.org/doc/html/rfc9000#name-packet-number-encoding-and-)

                union _destConnID {
                    uint8_t BYTES[16];
                    uint16_t WORDS[8];
                    uint32_t DWORDS[4];
                    uint64_t QWORDS[2];
                } DestinationConnectionId;

                uint32_t DestConnIdOverflow;

                union _packetNumber PacketNumber;

                fpx_quic_frame_t* Payload;
                size_t FrameCount;
            } OneRTT;
        } Short;

        struct _longHeader {
            uint32_t Version;
            uint8_t DestinationConnectionIdLength;
            // ----------------------------------------
            // TODO: the plan is to read up to here automatically
            // as in: by memcpy()'ing it into the quic_packet_t struct
            // afterwards we check the packet form, and if it's a long-header,
            // we check the long-header-type
            // ----------------------------------------
            uint8_t DestinationConnectionId[20];

            uint8_t SourceConnectionIdLength;
            uint8_t SourceConnectionId;
            union _payload {
                struct _version {
                    // only sent by server!
                    // in this packet type, the first byte contains: (MSB to LSB)
                    //  HeaderForm bit,
                    //  Unused (7); but server SHOULD set MSB to 1 if QUIC is being multiplexed (see
                    //  [RFC7983])

                    uint32_t* SupportedVersions;
                } Version;

                struct _initial {
                    fpx_quic_varlen_t TokenLength;  // depends on HMAC type (SHA1, etc.)
                    uint8_t* Token;                 // HMAC output
                    fpx_quic_varlen_t Length;
                    union _packetNumber PacketNumber;
                    uint8_t* PacketPayload;
                } Initial;

                struct _zeroRTT {
                    // https://datatracker.ietf.org/doc/html/rfc9000#name-0-rtt
                    fpx_quic_varlen_t Length;
                    union _packetNumber PacketNumber;
                    uint8_t* PacketPayload;
                } ZeroRTT;

                struct _handshake {
                    fpx_quic_varlen_t Length;
                    union _packetNumber PacketNumber;
                    uint8_t* PacketPayload;
                } Handshake;

                // struct _retry {
                //
                // } Retry;
            } Payload;
        } Long;
    } Data;

    // TODO: implement ;-;
} fpx_quic_packet_t;

typedef struct {
    pthread_t Thread;
    pthread_mutex_t ListenerMutex;

    uint8_t IpVersion;
    int FileDescriptor;

    fpx_quic_connection_t* Connections;
    size_t MaxConnections;
    size_t ActiveConnections;

    fpx_quic_connection_t* Backlog;
    size_t BacklogLength;
    pthread_cond_t BacklogCondition;
} fpx_quic_socket_t;

#endif  // FPX_QUIC_TYPES_H
