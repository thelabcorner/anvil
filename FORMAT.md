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
- `12`: SHAPE backend (per-shape displacement prediction; arch, iteration 2)
- `13`: TOPOLOGY backend (per-slot modal residual + exception mask; arch, iteration 2 — MEASURED NOT-ADOPTED, retained as a research mode)
- `14`: TCOPY backend (implicit Delta=-d transformed copy; arch, iteration 3 — prototype)
- `15`: HOTOP backend (compiled hot-op instruction book; arch, iteration 4 — prototype)

Unknown modes are rejected.

## HOTOP token backend (mode 15)

Mode 15 (landed by `arch` during iteration 4) is the mode-12 token model with a
compiled decoder instruction book (Linux modes 26-28 pattern, corrected per the
Linux results: a fully concrete absolute-distance book is rejected — hot
commands COEXIST with the per-shape displacement state). The encoder compiles
the most frequent `(kind, len, shape)` tuples that REUSE the shape's last
displacement into a book; the hot stream is a single opcode-index stream (one
entropy field per hot token — no per-field pulls), and the shape-state index is
compiled INTO each entry (constant-time opcode -> semantics -> state read ->
copy). Rare tokens escape to macro-ops, which use the full mode-12 shape coding
(absolute / reuse / delta + sparse patches) and update the SAME shape state.

Payload: `num_states` byte (1 or 28), `uvarint K` (book size <= 254), K entries
of `(kind byte, len uvar, shape byte)` — kind 0 = literal run (len; no state),
kind 1 = exact-match reuse (`dist = last[shape]`) — then 9 streams (per-stream
suite): opcodes (0..K, K = escape) / macro types / macro ll / macro ml / macro
dflags / macro dvar / literals / macro masks / macro residuals.

Decoder (fused single path): the opcode and literal streams are pulled on
demand; a hot op executes with one pull + bulk copy; macro streams are eager
(rare). Strictness: hot kind-1 requires `last[shape]` set and valid
(`dist in [1, pos]`), book bounds, macro invariants as mode 12, full substream
consumption, CRC.

**Measured status (arch, Windows): prototype. Round-trip verified on all corpus
files + PEs; fuzzed. Decode (median-5) vs the fused mode-12 baseline:
generated.log 274 vs 234 MB/s (1.17x), generated.json 194 vs 157 (1.23x),
generated.jsonl 284 vs 226 (1.25x), generated.sqlite 157 vs 118 (1.34x) — a
real hot-path win, but the pre-registered >=2x I4-1 target is NOT met; the
remaining floor is the opcode-stream entropy decode + copy throughput (the full
22-stream/precision-adaptive architecture is the Linux 0.87-0.99 GB/s lever).
Ratio: near-neutral on record files (log -0.1% vs sparse; +0.3-0.7% elsewhere;
small files pay the book header).

## TCOPY token backend (mode 14)

Mode 14 (landed by `arch` during iteration 3) is the SPARSE-REF token model plus
token type 3: a transformed copy. Token type 3 = copy a prior phrase (overlap
allowed; the copy is periodic as in modes 11-13) and, at 4-aligned windows where
the 32-bit little-endian target equals the source value minus the match distance
(the executable-relative relocation algebra, implicit `Delta = -dist`, zero
bits), rewrite the field; all other corrections are residuals as in mode 11.
Transform fields are constrained to the non-overlapping region of the reference
(`field_end <= dist`), so they are excluded first for overlapping refs.

Payload = 8 separated streams (each serialized with the per-stream suite):
token types (0-3) / lit-run len / match len / match dist / literals /
residual mask (flat 32-bit words per 32 B, as mode 11) / residual values /
transform mask (one 32-bit word per 32 four-byte windows; bit j of word w marks
window 32w+j as a transform field).

Decoder strictness: type-3 requires `dist in [1, out.size()]`, `len <= 65536`,
`field_end <= dist` per transform window (rejected otherwise), transform-mask
bits beyond the window count rejected, residual invariants as mode 11, full
substream consumption, CRC. Transform fields and residuals live in separate
streams (isolated statistical domains).

**Measured status (arch, Windows): prototype. Round-trip verified on the corpus
and on real PE executables; TCOPY beats plain sparse (mode 11, same greedy
parse) by ~0.1-0.6% on executables (transform fields fire: 519 fields in
`anvil.exe` block 0) and is neutral on non-binary data. Exact-LZ MDL (mode 10)
still wins on the executables — the greedy approximate parse is the limiting
factor, not the transform. The implicit-parameter claim (Delta derived from the
distance, zero bits) is the implemented mechanism; the explicit-Delta control is
a follow-up per the t3-patent narrowed-claim contract.

**`--pnra=on` (Experiment X, mode 14 only, off by default):** an additional,
invariant-anchored candidate SOURCE for the SAME token type 3 — no wire-format
change. Indexes x86 `E8`/`E9` (near call/jmp) relocation fields by their
translation-invariant absolute target (`field_pos+4+rel32`, constant across the
`Delta=-dist` transform), gated on the opcode byte only (~0.1-0.4% density on
real PE `.text`, keeping the index O(1) amortized per opcode occurrence — see
Experiment U's density-mismatch finding). This lets the parser find and cost-
compare transform candidates whose raw bytes never byte-match anywhere (so the
ordinary byte-hash chain search structurally cannot reach them), competing
against the existing exact/literal alternatives via the same bits-based cost
model already used for every other candidate. **Measured (Experiment X): a
wash to a small regression once wired through the real cost-model parser and
entropy coder** — see RESEARCH_LEDGER.md for the honest numbers; NOT ADOPTED,
kept off by default.

## TOPOLOGY token backend (mode 13)

Mode 13 (landed by `arch` during iteration 2) is mode 12's token/dist coding with
residuals coded against a per-slot modal value. Payload = 1 byte `num_states`
(1 or 28), a modal table, then 9 substreams:

1. token types (0 literal run, 1 exact match, 2 sparse-corrected match)
2. literal-run length varints
3. match length varints
4. distance flags (0 first-absolute, 1 reuse-last, 2 signed delta)
5. distance varints (absolute or zigzag delta)
6. literal bytes
7. correction masks (flat 32-bit words, as mode 11)
8. exception masks: per type-2 token, ceil(k/8) bytes; bit j set = correction j is
   an exception (its value is in stream 9)
9. exception residual values

Modal table: `uvarint` count then per entry `(k, slot, value)` where k =
correction count and slot = correction index within the token's mask; only
contexts with >= 2 observations get an entry. Decode: non-exception corrections
use `modal[k][slot]` (rejected if absent), exceptions read stream 9. All other
strictness mirrors modes 11/12 (dist bounds, mask-bits-beyond-len, full
substream consumption, CRC).

**Measured status (arch ablation, single-rep Windows): NOT ADOPTED.** Modal
accuracy of the (k, slot) context on this corpus/parser is 17-23% (generated.log
23.5%, json 17.0%, jsonl 22.5%), so exception coding costs more than flat
residuals; and correction masks are ~90% unique (top-32 masks cover 7-12% of
type-2 tokens), so there is no recurring topology to exploit. The Linux 86.5%
modal accuracy / 128 recurring masks came from a structural-channel parser that
aligns corrections to a record frame; ANVIL's greedy sparse parser lets
correction positions drift with varying field lengths. Topology coding is
retained as a research mode (`--parse=topology`) and as the measured flat-A
baseline for the future structural-distance (R4) work that would create the
recurring topology. The flat-A repeat-control headroom is recovered by the block
router, not by topology coding.

## SHAPE token backend (mode 12)

Mode 12 (landed by `arch` during iteration 2) is the SPARSE-REF token model with
a shape-conditional distance coder. Payload = 1 header byte (`num_states`, must
be `1` or `28`) followed by 8 substreams, each serialized with the per-stream
codec (raw or static order-0 rANS):

1. token types (0 literal run, 1 exact match, 2 sparse-corrected match)
2. literal-run length, uvarint(len-1)
3. match length, uvarint(len-4) [types 1,2]
4. distance flags: 0 = first-absolute, 1 = reuse-last, 2 = signed delta [types 1,2]
5. distance varints: absolute `uvarint(dist-1)` or zigzag `uvarint(delta)` [types 1,2]
6. literal bytes
7. correction masks (flat 32-bit words, 4 B per 32 B of phrase) [type 2]
8. residual bytes, popcount(mask) per type-2 token [type 2]

Distance prediction: each match's shape = `(type-1)*14 + len_class(len)` when
`num_states == 28` (len_class buckets lengths 4..65535 into 14 doubling
classes), or shape 0 for all matches when `num_states == 1` (the generic
single-state control). Each shape keeps a last-displacement state (init 0 =
unset). Flag 0 codes an absolute distance (first occurrence of the shape) and
sets the state; flag 1 reuses the state exactly; flag 2 codes a signed delta
(zigzag: `dlt>=0 ? 2*dlt : -2*dlt-1`) from the state and updates it.

Decoder strictness: `num_states` in {1,28}; delta before first absolute or
before reuse rejected; zigzag decode bounds-checked; `dist in [1, out.size()]`;
type-2 invariants identical to mode 11 (mask bits beyond len rejected,
`len(residuals) == popcount(mask)`, full substream consumption, CRC). Decode
cost is one state-table lookup + add per match — LZ-class.

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

Each stream independently chooses a codec from the **stream suite** (arch, iteration 2; `--stream-suite` and `ANVIL_STREAM_LAMBDA` control the encoder selection):

| mode | codec | notes |
|---|---|---|
| 0 | raw | unchanged |
| 1 | static order-0 rANS, 12-bit (4096 states), `RANS_L = 1<<23` | the original backend (unchanged wire) |
| 2 | static order-0 rANS, 9-bit (512 states), `RANS_L = 1<<17` | smaller symtab (cache-resident) |
| 3 | static order-0 rANS, 8-bit (256 states), `RANS_L = 1<<16` | smallest symtab |
| 4 | canonical Huffman | 256 code-length bytes header + bitstream; canonical codes by (len, sym) |
| 5 | default-with-exceptions | 1 default byte + `uvarint nexc` + `ceil(n/8)` mask bytes + `nexc` exception bytes |
| 6 | context-switched rANS (arch, iteration 4) | ONE physical rANS state whose frequency table is selected per symbol by a sparse-support quantizer: the previous decoded symbol maps through a learned K-context map (256 -> K, K <= 12, Lloyd-clustered on per-context symbol distributions) to one of K tables. Header: K byte + 256-byte context map + K rANS models + `uvarint dn` + state+renorm bytes. NOT multi-stream fan-out — a single interleaved rANS stream |

Frequency tables for modes 1-3 are serialized with only nonzero symbols. All
modes carry the decoded length first (`uvarint`). The decoder dispatches on the
mode byte; unknown modes are rejected.

Encoder selection (per stream): each codec is built and scored by
`J = L + λ·C_decode·L`, where `L` = encoded byte length, `C_decode` = per-byte
decode cost units (raw 1, rANS-4096 4, rANS-512 3.5, rANS-256 3, Huffman 2.2,
default-exc 2.0), and `λ` defaults to 0.04 (`ANVIL_STREAM_LAMBDA` overrides;
`0` = pure length). The lowest-J codec is chosen.

This representation is deliberately asymmetric: the encoder performs parsing,
stream construction, and codec selection, while the decoder mostly performs
codec table lookups, varint reads, literal copies, and overlapping match copies.

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
