#ifndef FPX_QUIC_H
#define FPX_QUIC_H

////////////////////////////////////////////////////////////////
//  "quic.h"                                                  //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

// implements [https://datatracker.ietf.org/doc/html/rfc9000]

#include "../../fpx_types.h"
#include "quic_types.h"
#include <pthread.h>

int fpx_quic_socket_init(fpx_quic_socket_t* QUIC_SOCK, const char* IP_ADDRESS, uint16_t PORT, uint8_t IP_VERSION);
int fpx_quic_listen(fpx_quic_socket_t* QUIC_SOCK, int FILEDESCRIPTOR, uint16_t MAX_ACTIVE, uint16_t BACKLOG);
int fpx_quic_stoplisten(fpx_quic_socket_t* QUIC_SOCK);

fpx_quic_connection_t fpx_quic_accept(fpx_quic_socket_t* QUIC_SOCK);

int fpx_quic_stream_write(fpx_quic_stream_t* STREAM, fpx_quic_packet_t* PACKET); // write to stream->WriteBuffer
int fpx_quic_stream_read(fpx_quic_stream_t* STREAM, fpx_quic_packet_t* DST_PACKET); // read packet data into dst_packet

int fpx_quic_stream_flush(fpx_quic_stream_t* STREAM); // write current contents of stream buffer into socket, and flush buffer

int fpx_quic_stream_fin(fpx_quic_stream_t* STREAM);
int fpx_quic_stream_rst(fpx_quic_stream_t* STREAM);

int fpx_quic_stream_abortread(fpx_quic_stream_t* STREAM);

#endif // FPX_QUIC_H
