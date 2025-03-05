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

