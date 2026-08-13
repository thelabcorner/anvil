# ANVIL v0.1 Experimental Format

This is an unstable research format. It is intentionally versioned to allow the bitstream to change whenever measurements justify it.

## File header

- 4 bytes: ASCII `ANV0`
- 1 byte: format revision (`1`)
- uvarint: nominal block size
- uvarint: original uncompressed size

## Block header

Repeated until the declared original size has been reconstructed:

- uvarint: uncompressed block length
- byte: block mode
- uvarint: payload length
- uint32 little-endian: CRC-32 (IEEE) of the reconstructed block
- payload bytes

Block modes:

- `0`: raw block
- `1`: arithmetic backend, adaptive order-0 literal model
- `2`: arithmetic backend, adaptive order-1 literal model
- `3`: arithmetic backend, order-1 gated after 4 observations
- `4`: arithmetic backend, order-1 gated after 8 observations
- `5`: arithmetic backend, order-1 gated after 16 observations
- `10`: separated-stream static rANS backend

Unknown modes are rejected.

## Arithmetic token backend

A block is reconstructed from literal-run and match tokens. Token type, literal-run length, match length, and match distance use independent adaptive probability models. Literal bytes can use order-0, order-1, or confidence-gated order-1 modeling. Integer fields are LEB128 byte sequences passed through their own adaptive byte models.

The decoder stops exactly when the declared block length is reached, then validates CRC-32.

## rANS token backend

Tokens are de-interleaved into five logical streams:

1. token types
2. literal-run length varint bytes
3. match-length varint bytes
4. match-distance varint bytes
5. literal bytes

Each stream independently chooses raw storage or a static order-0 byte rANS representation. rANS uses a 12-bit normalized frequency space (4096 states) and byte renormalization with `RANS_L = 1 << 23`. Frequency tables are serialized with only nonzero symbols.

This representation is deliberately asymmetric: the encoder performs parsing and stream construction, while the decoder mostly performs rANS table lookups, varint reads, literal copies, and overlapping match copies.

## Parser

Two parser families are currently implemented:

- `greedy`: longest useful hash-chain match
- `dp`: forward dynamic programming over literal edges and sampled match-length edges, minimizing an estimated coded-bit objective

`--parse=auto` encodes both and selects the smaller result. This is intentionally expensive research-search behavior, not yet the intended production mode.

## Integrity and malformed input

The decoder rejects invalid magic/revision, invalid block sizes, malformed varints, unknown modes, truncated payloads/substreams, invalid match distances/lengths, stream-consumption mismatches, trailing bytes, and CRC mismatches.
