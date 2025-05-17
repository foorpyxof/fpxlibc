#ifndef FPX_WEBSOCKETS_H
#define FPX_WEBSOCKETS_H

#include "../../fpx_types.h"

////////////////////////////////////////////////////////////////
//  "websockets.h"                                            //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

enum websocket_opcode {
  WEBSOCKET_CONTINUE = 0x0,
  WEBSOCKET_TEXT = 0x1,
  WEBSOCKET_BINARY = 0x2,

  WEBSOCKET_CLOSE = 0x8,
  WEBSOCKET_PING = 0x9,
  WEBSOCKET_PONG = 0xA,
};

typedef struct _fpx_websocketframe fpx_websocketframe_t;

struct _fpx_websocketframe {
    uint8_t final;
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t reserved3;

    enum websocket_opcode opcode;

    uint8_t mask_set;
    uint8_t masking_key[4];

    uint64_t payload_length;
    uint64_t payload_allocated;
    uint8_t* payload;
};

/**
 * Send a websocket closing frame containing the specified status code and reason
 *
 * Input:
 * - File descriptor of the underlying socket connection to send the frame over
 * - A websocket status code (>= 0)
 * - An optional reason string (NULL if not applicable) (only gets applied if the status code is
 * valid)
 * - The length of the reason string (<= 123)
 * - Boolean whether the frame is masked or not
 *
 * Returns:
 * -  0 on success
 * -  errno if the sending fails
 */
int fpx_websocket_send_close(
  int filedescriptor, int16_t, const uint8_t*, uint8_t payload_length, uint8_t masked);

/**
 * Initializes the websocket frame object to default values
 * WARNING: Will leak memory if allocations have been made on the frame object
 *
 * Input:
 * - Pointer to the websocket frame to initialize
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 */
int fpx_websocketframe_init(fpx_websocketframe_t*);

/**
 * Appends a new chunk onto the frame's payload
 *
 * Input:
 * - Pointer to the websocket frame to append the payload for
 * - Pointer to the byte buffer containing the payload
 * - Length of the payload
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 */
int fpx_websocketframe_append_payload(fpx_websocketframe_t*, const uint8_t*, size_t);

/**
 * Send the websocket frame over the wire
 *
 * Input:
 * - Pointer to the websocket frame to append
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 * -  errno if an error occurs while sending
 */
int fpx_websocketframe_send(const fpx_websocketframe_t*, int filedescriptor);

/**
 * Frees all associated memory allocations to prepare for object destruction
 *
 * Input:
 * - Pointer to the websocket frame to initialize
 *
 * Returns:
 * -  0 on success
 * - -1 if any passed pointer is unexpectedly NULL
 */
int fpx_websocketframe_destroy(fpx_websocketframe_t*);

#endif  // FPX_WEBSOCKETS_H
