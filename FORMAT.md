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
- `11`: SPARSE-REF backend (approximate self-reference with sparse correction)

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

## SPARSE-REF token backend (mode 11)

Mode 11 extends the separated-stream layout of mode 10 with a third token type:
sparse-corrected matches. Instead of requiring a match to be byte-identical,
the encoder copies a prior phrase and entropy-codes a sparse correction mask
plus the replacing bytes (residuals). The decoder stays copy + sparse stores.

A mode 11 payload is **7 separated streams**, each serialized with the same
per-stream codec as mode 10 (mode byte + uvarint length; raw or static order-0
rANS, chosen per stream):

1. `S0` token types: `0` literal run, `1` exact match, `2` sparse-corrected match
2. `S1` literal-run length varint bytes, `uvarint(len-1)`
3. `S2` match length varint bytes, `uvarint(len-4)` (token types 1 and 2)
4. `S3` match distance varint bytes, `uvarint(dist-1)` (token types 1 and 2)
5. `S4` literal bytes
6. `S5` correction masks: per type-2 token, `ceil(len/32)` little-endian 32-bit words; bit `j` of word `w` marks byte at phrase offset `32*w+j`
7. `S6` residual bytes: `popcount(mask)` bytes per type-2 token, in mask order

Decode of a sparse-corrected match (type 2):

- `dist` must satisfy `1 <= dist <= out.size()`; `len` must satisfy
  `4 <= len <= 65536` and `len <= out_len - out.size()`.
- `nwords = ceil(len/32)`. The last word's bits beyond `len` (i.e. set bits at
  phrase offsets `>= len`) are invalid — the decoder MUST reject them.
- Copy `len` bytes from `out[out.size()-dist]` with the standard overlapping
  copy (periodic when `len > dist`), recording the copy destination start.
- Then apply residuals: for each set bit `b` in word `w`, in mask order,
  `out[start + 32*w + b] = next residual byte`.
- Invariant: `len(residuals) == popcount(mask)` per token, enforced strictly.
- The copy is the *periodic* copy of the phrase as reconstructed so far;
  corrections are computed against that periodic copy on the encoder side and
  applied after the full copy on the decoder side.

After all tokens: every substream must be fully consumed (no trailing bytes in
any of S0-S6), the reconstructed size must equal the declared block length, and
the block CRC-32 must match.

## Parser

Three parser families are currently implemented:

- `greedy`: longest useful hash-chain match
- `dp`: forward dynamic programming over literal edges and sampled match-length edges, minimizing an estimated coded-bit objective
- `sparse`: single-pass greedy parse over literal / exact-match / sparse-corrected edges (feeds mode 11). A sparse edge is accepted only when its estimated cost (token + len/dist varints + `len/8` flat mask + residual literal costs) beats the best exact-edge-plus-literals alternative covering the same span.

`--parse=auto` encodes the available candidates and selects the smaller result per block (raw fallback included). This is intentionally expensive research-search behavior, not yet the intended production mode.

## Integrity and malformed input

The decoder rejects invalid magic/revision, invalid block sizes, malformed varints, unknown modes, truncated payloads/substreams, invalid match distances/lengths, stream-consumption mismatches, trailing bytes, and CRC mismatches.

Strict bounds (see the decoder audit, `deliverable/t-format`):

- `block_size` is bounded to `[1, 64 MiB]`; every block length must satisfy
  `1 <= blen <= block_size` and `blen <= total - out.size()`.
- Declared `total` must be plausible for the input: `total <= (in.size()/7 + 2) * 2^26`.
  The minimum block header is 7 bytes (length varint + mode + payload-length
  varint + 4-byte CRC), and `blen` cannot exceed the 64 MiB block cap, so no
  well-formed encoder output can exceed this bound; it is a guard against
  output amplification from attacker-declared sizes.
- Every per-stream decoded length (mode 10 and mode 11 substreams) must satisfy
  `raw_n <= 16 * out_len + 64`. Literal bytes, token types, varint bytes, masks,
  and residuals of a block of `out_len` bytes can never legitimately exceed this;
  it bounds eager substream allocation against tiny crafted payloads.
- Mode 11 additionally enforces: unknown token types rejected, mask bits beyond
  `len` rejected, `popcount(mask) == residuals consumed` per token, match
  distance in `[1, out.size()]`, match length in `[4, 65536]`, and full
  consumption of all seven substreams.

All failures surface as a clean nonzero exit with a diagnostic; the decoder
never overruns a buffer or accepts a block whose CRC does not match.
