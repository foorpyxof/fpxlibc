- QUIC connection can have multiple streams

```
Example Structure {
  One-bit Field (1),
  7-bit Field with Fixed Value (7) = 61,
  Field with Variable-Length Integer (i),
  Arbitrary-Length Field (..),
  Variable-Length Field (8..24),
  Field With Minimum Length (16..),
  Field With Maximum Length (..128),
  [Optional Field (64)],
  Repeated Field (8) ...,
}
```

0x01 is 0 for client-initiated stream; 1 for server-initiated;
0x02 is 0 for bidirectional streams and 1 for unidirectional;

out of order data okay? read later

stream offset means place of data in stream; probably 0 for first frame
vector as stream? look into it

look more into streams in general

[https://datatracker.ietf.org/doc/html/rfc9000#name-operations-on-streams]
[https://datatracker.ietf.org/doc/html/rfc9000#name-sending-stream-states]

# important notes:
- coalesced packets ([https://datatracker.ietf.org/doc/html/rfc9000#name-coalescing-packets])
  - "Receivers MAY route based on the information in the first packet contained in a UDP datagram."
  - "Senders MUST NOT coalesce QUIC packets with different connection IDs into a single UDP datagram."
  - "Receivers SHOULD ignore any subsequent packets with a different Destination Connection ID than the first packet in the datagram."
  - "The receiver of coalesced QUIC packets MUST individually process each QUIC packet and separately acknowledge them, as if they were received as the payload of different UDP datagrams."
  - "\[upon failure to process\] the receiver MAY either discard or buffer the packet for later processing and MUST attempt to process the remaining packets."
  - "\[Retry, Version Negotiation and short-header packets\] do not contain a Length field and so cannot be followed by other packets in the same UDP datagram."
---
- packet numbers ([https://datatracker.ietf.org/doc/html/rfc9000#name-packet-numbers])
  - integer in range 0 to (2^62)-1
    - except when in long- or short-header; here they are 1 to 4 bytes, encoded using Variable-Length integer encoding.
  - used for cryptographic nonce
  - "\[Version Negotiation and Retry packets\] do not include a packet number."
  - Three spaces:
    - Initial space           (for all "initial packets")
    - Handshake space         (for all "handshake packets")
    - Application data space  (for all 0-RTT and 1-RTT packets)
  - "Conceptually, a packet number space is the context in which a packet can be processed and acknowledged."
  - "Initial packets can only be sent with Initial packet protection keys and acknowledged in packets that are also Initial packets."
  - "Handshake packets are sent at the Handshake encryption level and can only be acknowledged in Handshake packets."
  - " Packet numbers in each space start at packet number 0. Subsequent packets sent in the same packet number space MUST increase the packet number by at least one."
  - "A QUIC endpoint MUST NOT reuse a packet number within the same packet number space in one connection."
  - "If \[the maximum packet number is reached\] the sender MUST close the connection without sending a CONNECTION_CLOSE frame or any further packets;"
  - "A receiver MUST discard a newly unprotected packet unless it is certain that it has not processed another packet with the same packet number from the same packet number space." Please check [https://datatracker.ietf.org/doc/html/rfc9000#name-packet-numbers] for more important information regarding this.
  - "Packet number encoding at a sender and decoding at a receiver are described in Section 17.1."
---
- Section 17.1: [https://datatracker.ietf.org/doc/html/rfc9000#name-packet-number-encoding-and-](Packet Number Encoding and Decoding)
  - see the following psuedocode for the encoding of a packet-number:
```python
EncodePacketNumber(full_pn, largest_acked):

  # The number of bits must be at least one more
  # than the base-2 logarithm of the number of contiguous
  # unacknowledged packet numbers, including the new packet.
  if largest_acked is None:
    num_unacked = full_pn + 1
  else:
    num_unacked = full_pn - largest_acked

  min_bits = log(num_unacked, 2) + 1
  num_bytes = ceil(min_bits / 8)

  # Encode the integer value and truncate to
  # the num_bytes least significant bytes.
  return encode(full_pn, num_bytes)
```
For example, if an endpoint has received an acknowledgment for packet 0xabe8b3 and is sending a packet with a number of 0xac5c02, there are 29,519 (0x734f) outstanding packet numbers. In order to represent at least twice this range (59,038 packets, or 0xe69e), 16 bits are required.

In the same state, sending a packet with a number of 0xace8fe uses the 24-bit encoding, because at least 18 bits are required to represent twice the range (131,222 packets, or 0x020096).

