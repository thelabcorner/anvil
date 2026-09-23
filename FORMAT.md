# ANVIL v0.1 Experimental Format

This is an unstable research format. It is intentionally versioned to allow the bitstream to change whenever measurements justify it.

## File header

- 4 bytes: ASCII `ANV0`
- 1 byte: format revision (`1` or `2`)
- uvarint: nominal block size
- uvarint: original uncompressed size

Revision compatibility is intentionally narrow:

- **revision 1**: legacy research codec modes `0`, `1`-`5`, and `10`-`16`;
  nominal block size is bounded to `1..64 MiB`.
- **revision 2**: ratio/container framework modes `0` and `17` only; nominal
  block size is bounded to `1..128 MiB`. A revision-2 decoder MUST reject any
  nonzero block mode other than `17` before interpreting its payload.

The encoder currently writes revision 2 only for `--parse=ratio`. Pre-registry
development artifacts that used a one-byte mode-17 prefix before this revision-2
contract was written were experimental scratch output, not a supported wire
format; the current mode-17 payload below is authoritative.

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
- `16`: ARI-REF backend (additive arithmetic reference; experiment I8/I9 — **experimental**, encoder gate `--ariref=on`, default OFF; wire provisional/uncommitted at the I9 registry pass)
- `17`: revision-2 ratio envelope (reversible transform + explicit backend ID)

Unknown modes are rejected.

## Revision 2 ratio envelope (block mode 17)

ANVIL's revision-2 architecture is a **codec/container framework**: reversible
representations and byte-compression backends are independent, explicitly
identified wire choices. ANVIL is therefore not defined as a Brotli
preprocessor. Brotli is backend ID 1 in a registry that can contain other
backends without changing transform semantics.

Mode-17 payload:

```
byte transform_id
byte backend_id
uvarint transformed_size
backend_payload[remaining block payload bytes]
```

The two identifier bytes are mandatory even when the direct transform and the
only currently shipped backend are selected. Unknown IDs are rejected **before**
backend decode. Backend ID `0` is reserved/invalid.

### Transform registry

| ID | name | transformed representation |
|---:|---|---|
| 0 | direct | original block bytes unchanged |
| 1 | ctx1-256 | 256 previous-byte-routed streams plus their lengths |
| 2 | line-columns | record lengths followed by column-major record bytes |
| 3 | lzp-residue | LZP side table (literals, then matches); the backend payload is the BWT of the concatenated `literals || matches` residue. **DISABLED** (do-not-reburn 06 §G2); the encoder refuses `--bwt-lzp=on` and the decoder rejects transform 3 — see below |

Transform 1 (`ctx1-256`) stores `256 x uvarint stream_len`, then streams 0..255.
For each original byte, the route is the previously decoded byte (initial
previous byte `0`); route IDs/positions are therefore derived rather than
stored. The decoder requires the 256 lengths to sum exactly to the block output
length, requires transformed bytes after the length table to equal that output
length, and requires every routed stream to be consumed exactly.

Transform 2 (`line-columns`) stores `uvarint record_count`, then one
`uvarint record_len` per record, then the bytes column-major. Newline is included
in a record; a final non-newline tail is a record. The encoder caps record length
at 4096 bytes and requires at least 8 records. The decoder enforces those length
bounds, exact sum to the output length, exact transformed-size accounting, and
full input consumption.

Status note (I9 registry pass, resolved): transform 2 round-trips with backend 1
(Brotli) and with backend 2 (BWT). The forced combination transform 2 + backend 2
(`--parse=ratio --ratio-lines=on --ratio-backend=bwt`) failed decode
(`ratio line length sum mismatch`) earlier in the I9 pass; root cause was the
`libsais` primary contract (the encoder remapped `primary == n` to 1, and
`unbwt(..., 1)` reconstructs the wrong string), fixed at source (encoder stores
the returned primary as-is, `1..n`; decoder validates `1..n`). Verified byte-exact
across 64–12,000 record inputs on the I9 registry build sha
`8EAE1FB327AD759B1CF80BE765B59FA7EFB8AB843F908341AC2283652BFDD105`, and
re-asserted by the full fuzz gate on the current canonical `build\anvil.exe` sha
`72D65150CBD65182C69BA85DAD3A8961C25DE48E794FD9823BCAB4ED20A36505`
(`src/anvil.cpp` sha
`BDC9047434D19106D1D2316DC094FD615F8B71E3EB0FCA98CAFCD076B53B8CE0`), asserted by
`tests/fuzz.py::registry_coverage_roundtrip`.

Transform 3 (`lzp-residue`) is **DISABLED**. LZP prepass was measured NO-GO in
Iteration 6 (RESEARCH_LEDGER.md / 06 §G2: +0.37% bytes, wins 4/13, none of the
headline BWT winners), so it carries **no byte claim**. Its I9 wire was
underdefined — the `[backend payload][LZP side table]` framing had no explicit
backend-payload length, so the decoder could not locate the side table — and it
was retired rather than fixed. The encoder rejects `--bwt-lzp=on` up front
(`"--bwt-lzp=on is not supported: ratio transform 3 (LZP residue) is disabled"`)
and the decoder rejects any `transform == 3` payload with
`"ratio transform 3 (LZP residue) is not supported"`. No archive may contain
transform 3; the `bwt_lzp_preprocess` / `bwt_lzp_reconstruct` helpers are kept
`[[maybe_unused]]` as an ablation reference only. `tests/fuzz.py` asserts this
path as a clean up-front rejection.

The encoder always builds transform 0 under the selected backend and accepts a
nonzero transform only when the **complete mode-17 payload** is smaller. Thus a
reported transform win cannot be a backend substitution disguised as a
preprocessing win.

### Backend registry

| ID | backend | decoder semantics | current encoder policy |
|---:|---|---|---|
| 1 | Brotli | one complete Brotli bitstream; large-window decoding enabled | quality 11, `lgwin=30`, generic mode |
| 2 | BWT + ANVIL postcoder portfolio | explicit postcoder reconstructs BWT bytes, then validated `libsais` inverse BWT (legacy primary or auxiliary indexes) | build postcoder candidates 1/2/3 (smallest wins); IDs 0 and 4 are defined but **disabled** (known encoder/decoder mismatch) — see postcoder table |

Quality/window/mode are encoder policy, not backend identity: a decoder for
backend 1 decodes the Brotli bitstream it receives and does not need to know
which quality setting produced it. A future backend gets a new ID rather than
silently changing ID 1 semantics. Backend 2 shares the same `transform_id` /
`backend_id` / `transformed_size` envelope and the same `2*block_output+4096`
transformed-size bound as backend 1 (see below for postcoder-specific rules).

### Backend 2 — BWT + postcoder portfolio (validated)

Backend 2 is the only transform-independent backend that ships an ANVIL-owned
entropy representation. The forward BWT uses pinned Apache-2.0 `libsais`
(`BWT`/`unbwt` only); everything after the sort is ANVIL wire.

The `transformed_size` declared in the mode-17 envelope is the **exact output
byte count of the inverse BWT**, i.e. the length of the reconstructed block
before any inverse transform. This is fixed externally and never inferred from
the postcoder payload, so attacking payload lengths cannot change the output
size the decoder must produce.

Backend 2 has two additive inner-payload forms. The first byte is an
unambiguous discriminator because postcoder IDs occupy only `0..4`.

Legacy BWT payload (v1):

```
byte    postcoder_id        (0..4 defined; 0 and 4 are encoder-disabled — see table)
uvarint primary_index       (libsais primary index; 1 <= primary_index <= transformed_size)
postcoder-specific payload
```

Auxiliary-index BWT payload (I10-1A / v2):

```
byte    0xFE                 (auxiliary BWT payload tag)
byte    postcoder_id         (same registry as v1)
uvarint sampling_rate_r      (power of two; 2 <= r <= transformed_size)
uvarint auxiliary_count      (must equal 1 + floor((transformed_size-1)/r))
u32le   I[auxiliary_count]    (each 1..transformed_size; I[0] is the primary index)
postcoder-specific payload
```

The v2 representation is **additive**. A decoder that supports I10-1A continues
to read every valid legacy-v1 payload; `--bwt-aux=off` remains the byte-identical
legacy encoder path. The encoder emits v2 only when explicitly selected by its
current policy/CLI.

`0xFF` is reserved one layer above these inner payloads for BWT subblock
framing. Therefore a bare backend-2 payload begins with either a postcoder ID
`0..4` (v1) or `0xFE` (v2), never `0xFF`.

Postcoder ID status (decoder-visible registry):

| ID | representation | status |
|---:|---|---|
| 0 | MTF + zero-run RLE, separated stream-suite streams | **disabled** — known encoder/decoder mismatch (`invalid huffman code`); rejected up front by the encoder |
| 1 | MTF/RLE + adaptive order-0 arithmetic | supported (selected on 2/12 Silesia files) |
| 2 | MTF/RLE + adaptive order-1 arithmetic | supported (canonical winner; smallest on most BWT-friendly files) |
| 3 | raw BWT bytes (stream-suite control/ablation) | supported |
| 4 | QLFC-like local frequency | **disabled** — known encoder/decoder arithmetic-model desync on rank-0 runs; rejected up front by the encoder |

IDs 0 and 4 are part of the registry so a *decoder* will still parse a hand-built
payload, but the encoder refuses to emit them (`--bwt-post=0|4` errors with
"not supported yet"). Auto backend-2 selection only ever considers 1/2/3, so the
canonical numbers (Silesia 46,446,995 B; enwik8 23,534,368 B) are unaffected.
Every postcoder ID 0..4 has a direct forced encode→decode test in `tests/fuzz.py`
(`forced_postcoder_roundtrip`) — selection-based coverage alone would have left
0 and 4 unexercised.

For legacy v1, `primary_index` is the **1-based** index returned by
`libsais_bwt`. The valid range is:

`1 <= primary_index <= transformed_size`

The endpoint `primary_index == transformed_size` is valid. Zero is not. This
matches the vendored `libsais_unbwt` contract actually used by the decoder.

For auxiliary v2, validation occurs before inverse-BWT allocation/work:

- `transformed_size > 1`;
- `sampling_rate_r` is a power of two and
  `2 <= sampling_rate_r <= transformed_size`;
- `auxiliary_count == 1 + floor((transformed_size - 1) / sampling_rate_r)`;
- `auxiliary_count <= min(transformed_size, 1<<20)`;
- exactly `4 * auxiliary_count` index bytes must remain before postcoder data;
- every auxiliary index is in `[1, transformed_size]`;
- `I[0]` is the primary index used by postcoder semantics that require it.

The encoder's current sampling policy targets approximately 1024 independent
LF walks by choosing the smallest power-of-two `r >= ceil(n/1024)`, with a
minimum of 2. That choice is encoder policy, not a redefinition of the v2 wire
contract.

Decode memory bound: the decoder allocates at most `transformed_size` output
bytes plus an `int32` work array of size `transformed_size + 1`, the bounded
auxiliary-index array for v2, plus the postcoder's own intermediate buffers
(each individually bounded by `transformed_size` or by a smaller declared
length). No wire length may authorize an output larger than the enclosing
`transformed_size`.

#### Optional outer BWT subblock framing

Backend 2 may split one transformed stream into independent BWT subblocks before
entering either inner payload form. This framing sits **outside** the v1/v2 BWT
payload:

```
byte    0xFF
uvarint subblock_count
repeat subblock_count times:
    uvarint decoded_len
    uvarint payload_len
    byte[payload_len] inner_bwt_payload
```

Each `inner_bwt_payload` is independently either legacy v1 (first byte `0..4`)
or auxiliary v2 (first byte `0xFE`).

Decoder bounds are strict:

- `subblock_count > 0`;
- the count cannot exceed the enclosing `transformed_size` and cannot exceed
  what the remaining payload could encode even at the two-varint-per-subblock
  minimum;
- each `decoded_len > 0` and cannot exceed the still-unreconstructed portion of
  `transformed_size`;
- each `payload_len` must fit entirely inside the remaining backend payload;
- every inner payload must reconstruct exactly its declared `decoded_len`;
- the concatenated decoded lengths must equal `transformed_size`;
- the `0xFF` frame must consume the backend payload **exactly**; trailing bytes
  are malformed.

The encoder emits this frame only when its configured BWT subblock cap requires
more than one piece. A single piece uses the bare inner payload so legacy
byte identity is preserved.

#### Postcoder 0 — MTF + zero-run RLE, separated stream-suite streams

MTF is seeded with identity mapping (`sym[i]=i`, `pos[i]=i`). Each BWT byte is
mapped to its MTF rank (0..255). A run of `rank 0` symbols is coalesced into a
single token `0` whose length is stored in the **runs** stream as a uvarint
encoding `run_len - 1` (so a run of length 1 consumes one token of type 0 and a
uvarint value `0`). Non-zero ranks are emitted as literal token bytes `1..255`.
Decoding of a `0` token appends `run_len` copies of the current rank-0 symbol
(`sym[0]`); the MTF list is unchanged by a zero run.

Postcoder payload (after the legacy-v1 or auxiliary-v2 BWT header):

```
uvarint token_stream_len
token_stream   -> decode_stream() (exact smallest stream-suite codec)
uvarint run_stream_len
run_stream     -> decode_stream() (exact smallest stream-suite codec)
```

- `token_stream_len` and `run_stream_len` cannot exceed the remaining backend
  payload; otherwise `truncated BWT token/run stream`.
- The two substreams and the whole backend payload must be **consumed exactly**;
  trailing bytes after the run stream, or inside either substream
  (`... trailing bytes`), are rejected.
- The token stream is expanded against `transformed_size`. The decoder throws
  `BWT MTF token overflow` if a token would push output past `transformed_size`,
  and `BWT zero run exceeds output` if a run length would overflow it.
- Exact token/run counts must reconstruct exactly `transformed_size` bytes; a
  mismatch is `BWT MTF stream consumption mismatch`.
- The nested stream codec inside either substream is the standard stream-suite
  codec (`decode_stream`, modes 0–8). Malformed nested codecs (bad rANS model,
  Kraft-inequality failure, grammar/RLZ amplification, truncated inner stream,
  or a disallowed nesting depth) are rejected by `decode_stream` before they can
  affect the BWT.

#### Postcoder 1 — MTF/RLE + adaptive order-0 arithmetic

Same MTF byte stream as postcoder 0, but the token/run sequence is coded by a
single adaptive arithmetic coder:

```
uvarint bit_count      (number of meaningful payload bits)
arithmetic payload    ((bit_count+7)/8 bytes; bit_count fixes exact extent)
```

Arithmetic rules (decoder, `bwt_arith_decode`):

- `bit_count == 0` or `bit_count > payload_bytes * 8` → `bad BWT arithmetic bit count`.
- The payload must be exactly `(bit_count+7)/8` bytes; a different count is
  `BWT arithmetic byte count mismatch` (covers both "more bytes than bits need"
  and "fewer bytes than bits require").
- If `bit_count % 8 != 0`, the final payload byte's unused low bits are padding
  and **must be zero**; nonzero padding is `nonzero BWT arithmetic padding`.
- The arithmetic decoder reads exactly `bit_count` bits (BitReader pads with
  trailing zeros, so bit_count is the sole extent authority — there can be no
  hidden trailing bytes).
- The token/run expansion against `transformed_size` uses the same MTF/RLE rules
  as postcoder 0; `BWT arithmetic zero run exceeds output` and token overflow are
  rejected identically.

#### Postcoder 2 — MTF/RLE + adaptive order-1 arithmetic

Identical wire and rules to postcoder 1, except the token arithmetic model is
conditioned on the previous emitted token (`order-1`). The same `bit_count`,
padding, byte-count, and `transformed_size` bounds apply; the only difference is
model context. The same rejections (`bad BWT arithmetic bit count`, byte-count
mismatch, nonzero padding, run/token overflow) apply.

#### Postcoder 3 — raw BWT bytes (control / ablation)

The BWT output (before MTF) is stored verbatim through the smallest stream-suite
codec:

```
byte (stream codec mode)
uvarint raw stream length
raw stream bytes (decode_stream)
```

The decoded raw stream length **must equal** `transformed_size`; otherwise
`BWT raw stream size mismatch`. Trailing bytes after the raw stream
(`BWT raw stream trailing bytes`) are rejected.

#### BWT inverse and final checks

After a postcoder reconstructs the BWT-transformed bytes:

- legacy v1 calls `libsais_unbwt` with the validated 1-based `primary_index`;
- auxiliary v2 calls `libsais_unbwt_aux` with the validated sampling rate and
  complete auxiliary-index array.

Both paths must produce exactly `transformed_size` output bytes. The temporary
array passed to libsais is `transformed_size + 1` `int32` entries as required by
the vendored inverse-BWT API.

The reconstructed block then passes through the normal mode-17 + block CRC-32
verification over the **original** block bytes, so corruption that somehow
survived structural/postcoder validation still fails the outer integrity check.

#### Degenerate but valid cases

- **1-byte input**: `transformed_size == 1`, legacy `primary_index == 1`, and
  the BWT output is the one source byte. The encoder intentionally keeps this as
  legacy v1 even when `--bwt-aux=on`; auxiliary framing has no useful work to
  parallelize at length one.
- **auxiliary v2 with `transformed_size <= 1`** is malformed and rejected.
- **all-equal input** (e.g. `AAAA…`) is a valid BWT input and must reconstruct
  byte-exactly.
- `primary_index == transformed_size` is valid; `primary_index == 0` is not.

These cases are asserted by the direct BWT/golden/auxiliary tests in
`tests/fuzz.py`. Historical notes that described the libsais primary index as
zero-based are superseded by this contract and by the current decoder's
validated `[1,n]` behavior.

#### Measured validation (backend 2 as a valid experimental format)

With the wire format above, the **whole-file `--ratio-backend=auto`** measurement
(pinned clang-cl build, transforms OFF; canonical figures from `bench-normalize`,
blackboard `deliverable/auto-routing` + `deliverable/enwik8-bwt`) shows backend 2
is a competitive, self-consistent format:

- Silesia whole-file `auto` = **46,446,995 B** → beats xz -9e (48,456,100 B) by
  **2,009,105 B**; backend 2 (BWT) chosen on 7/12 files, Brotli on 5/12. All
  round-trips verify (sha256-enforced).
- enwik8 `auto` = **23,534,368 B** (router chose BWT) → beats xz by 1,297,288 B.
- The byte win is real but a **front-gap on decode cost**: BWT decode is ~11.2x
  slower than Brotli (enwik8 ≈ 6.7 MB/s). Reported honestly per the falsification
  mandate, not as a Pareto crossing.
- The +159 B gap to the 46,446,836 B oracle is rev-2 **framing only** (~240 B
  total), confirming backend-2 overhead is negligible.

These are the authoritative measured figures; they are backend 2's validation
evidence now that E0 (this spec + 24-case BWT fuzz + golden files) closes.

`transformed_size` is the exact byte count expected after backend decode. It is
bounded by `2 * block_output_length + 4096` and by the implementation's global
ratio-stream cap; a mismatch, truncation, backend trailing input, or transform
inverse-size mismatch is an error. The normal block CRC-32 is then checked over
the final reconstructed **original** block bytes.

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

## ARI-REF token backend (mode 16)

Mode 16 (I8/I9 experiment; **experimental, uncommitted at the time of the I9
registry pass**) is an additive-arithmetic reference model. The encoder gate is
`--ariref=on` (default OFF); with it OFF the default/auto path is byte-identical
to the pre-mode-16 build. It is a revision-1 block mode.

Tokens (type byte in substream 0): `0` literal run, `1` exact match, `4`
ARI-REF transformed block — a 4-aligned, non-overlapping copy of a prior phrase
whose 32-bit words are offset by one transmitted signed delta before sparse
residual bytes are applied. Payload = **9 separated streams**, each
length-prefixed and serialized with the per-stream suite, in order:
`types / lit-run-len / match-len / match-dist / literals / correction-masks /
residuals / ari-word-count / ari-delta`.

- type 0: `uvarint(len-1)` in stream 1; `len` literal bytes from stream 4.
- type 1: `uvarint(len-4)` in stream 2; `uvarint(dist-1)` in stream 3.
- type 4: `uvarint(W-1)` with `W = len/4` in stream 7; `uvarint(dist-1)` in
  stream 3; zigzag `uvarint(delta)` in stream 8; `4*ceil(len/32)` mask bytes in
  stream 5; one residual byte per set mask bit in stream 6. Decode copies the
  non-overlapping phrase, adds `delta` to each 32-bit word, then applies the
  residual bytes at the set mask positions.

Decoder strictness: type 4 requires `dist % 4 == 0`, `dist >= len` (non-overlap),
`dist <= out.size()`, the span must fit the remaining declared output, `W` within
the encoder's word-count bound, mask bits beyond `len` rejected,
`popcount(mask) == residuals consumed`, all nine substreams fully consumed, and
the block CRC-32 over the reconstructed bytes.

**Status:** experimental. No Pareto claim is made for mode 16 in this document.
I8/I9 measurements of this representation live in RESEARCH_LEDGER.md and are only
citable against both the raw and transform-enabled reference bars per the
ledger's dual-bar rule.

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
| 7 | RePair grammar (arch, iteration 6 — Experiment Y, LANDED `da01c54`) | digram-factoring grammar (Larsson & Moffat 1998) over the stream, residual entropy-coded by an inner suite codec. Emission is gated behind `--hotop-rlzp=on` (OFF by default; measured NOT-ADOPTED-as-default: ratio-only win, decode-negative + encode-prohibitive). Full spec below. Old binaries reject mode 7 cleanly ("unknown stream codec"); any file containing mode 7 must conform to the spec below |
| 8 | RLZ self-reference (arch, iteration 6 — Experiment Y, LANDED `da01c54`) | LZ77-style self-referencing matches within the same stream (Kurup/Marin/Ziv 2010 formulation), residual entropy-coded by an inner suite codec. Same gating and status as mode 7. Full spec below |

Frequency tables for modes 1-3 are serialized with only nonzero symbols. All
modes carry the decoded length first (`uvarint`). The decoder dispatches on the
mode byte; unknown modes are rejected.

### Stream-codec selection objective (implemented; corrects earlier text)

Earlier revisions of this document described the selection objective as
multiplicative (`J = L + λ·C_decode·L`, λ = 0.04). The implemented — and, as
of the frozen S6-1 pre-registration (`deliverable/pre-reg-s6-1`, EXP. L
lineage), canonical — objective is **additive**:

```
J = L + λ·c + μ·2 + ν·c        (per candidate codec)
```

where `L` = encoded byte length of the candidate, `c` = per-byte decode-cost
unit from the table below, `λ` defaults to `0.01` (`--stream-lambda=N` or env
`ANVIL_STREAM_LAMBDA` overrides; `0` = pure length), and `μ`, `ν` are reserved
encoder-side hooks (both default `0.0`; no wire effect). Cost units as
implemented (integer tenths of the `C_decode` values used in earlier text:
raw 1, Huffman 2.2, default-exc 2.0, rANS-256 3, rANS-512 3.5, rANS-4096 4,
ctx-rANS 4.5):

| codec | cost unit `c` |
|---|---|
| 0 raw | 10 |
| 4 Huffman | 22 |
| 5 default-exc | 20 |
| 3 rANS-256 | 30 |
| 2 rANS-512 | 35 |
| 1 rANS-4096 | 40 |
| 6 ctx-rANS | 45 |

Raw (mode 0) is **always** a candidate — every stream has a size-and-cost
fallback regardless of flags. The lowest-J candidate is chosen and its mode
byte is what the decoder sees; see "Stream selection and the whole-codec
budget (S6-1)" below for the wire contract this implies.

## Stream-codec suite extensions: RePair (mode 7) and RLZ (mode 8)

Iteration-6 additions to the **stream-codec namespace** (the per-substream
byte inside modes 10-15 payloads — distinct from the block-mode namespace,
which uses different numbers). Both are self-contained: they frame an inner
body, entropy-code the body with any suite codec 0-6, and materialize the
whole substream eagerly at parse time so downstream consumers walk a plain
byte buffer. Neither is in `encode_stream`'s general candidate set (that would
recurse); the hot-op book encoder tries them explicitly against the suite and
picks by size when `--hotop-rlzp=on`.

Wire layout (both):

```
[mode byte (7 or 8)][uvarint N][uvarint ilen][inner x ilen]
```

`N` = declared decoded length (subject to the shared `raw_n <= 16*out_len+64`
substream bound), `ilen` = length of the inner suite-coded body, `inner` = a
complete suite stream (modes 0-6 only — a mode-7/8 inner is invalid; see
recursion cap below).

### Mode 7 — RePair grammar

Inner body (after suite-decoding):

```
[uvarint R][R x (uvarint a, uvarint b)][uvarint M][M x uvarint s]
```

- `R` = rule count, `R <= 768`. Rule `i` (0-based) defines nonterminal
  `256+i` as the concatenation `exp(a_i) ++ exp(b_i)`; terminals are bytes
  `0..255`.
- Every rule symbol `a_i`, `b_i` must satisfy `< 256 + R`; every reduced
  symbol `s_j` likewise. A symbol referencing index `>= ` its own rule index
  (self- or forward-reference) is **rejected** — grammars are acyclic by
  construction and rules expand strictly in order.
- `M` = reduced-sequence length, `M <= N`.
- Decode: expand rules 0..R-1 in order, then concatenate the expansions of
  the reduced sequence. The result MUST be exactly `N` bytes
  (`expansion overflow` / `output-size mismatch` otherwise).
- Amplification invariants (REQUIRED decoder behavior): per-rule materialized
  expansion `|exp[i]| <= N`, and cumulative bytes materialized across all
  rule expansions `<= 2*N + 64 KiB`. Both are provably non-rejecting for the
  shipped encoder — folding conserves each rule's reference count at >= 2 in
  the final structure, so every legit `|exp[i]| <= N/2` — and they bound a
  crafted doubling-chain grammar (~70-byte file) that would otherwise force
  exponential allocations before the final size check fires.

### Mode 8 — RLZ self-reference

Inner body (after suite-decoding):

```
[uvarint nops][nops x op]
op = [0x00][uvarint len][len literal bytes]
   | [0x01][uvarint dminus][uvarint lminus]
```

- Literal op: append `len` bytes; `len` must fit both the remaining body and
  the remaining declared output.
- Match op: stored values are `dminus = dist-1` and `lminus = len-4` (house
  style, as in modes 12/15 macro streams); the decoder reconstructs
  `dist = dminus + 1`, `len = lminus + 4`. The addends are
  untrusted varints: the decoder MUST reject `dminus >= 0xFFFFFFFF` and
  `lminus >= 0xFFFFFFFF` **before** the increment (a `UINT64_MAX + 1` wrap
  yields `dist = 0`, which turns the copy into an out-of-bounds read).
  Validity: `dist <= current output size`, `len <= remaining declared output`;
  copy is the standard overlapping (periodic) copy.
- After all ops the output MUST be exactly `N` bytes.
- Encoder advisory (NOT wire contract): the shipped encoder restricts
  `dist <= 64 KiB` (self-reference window), min match 4, literal runs
  `<= 2048`. The decoder deliberately does not enforce the window — any
  `dist <= output size` decodes — so a future encoder may widen the window
  without a new mode. Only the safety-relevant bounds above are strict.

### Mode 7/8 framing, consumption, and integrity

- Substream framing is unchanged: each substream remains
  `[uvarint zn][mode byte][uvarint N][payload...]` with `zn` covering
  everything after itself; the whole-substream trailing-bytes check applies
  (`q == qe` after decode).
- Consumption accounting: modes 7/8 decode eagerly at parse; the consumer
  walks the materialized buffer. Full consumption requires BOTH the pulled
  count reaching `N` AND the buffer cursor reaching the buffer end — the
  `at_end()` predicate MUST implement the latter explicitly for codecs 7/8
  (falling through to another codec's condition is a defect; see the decoder
  audit, Experiment-Y section).
- Recursion cap: the inner body decodes via the suite path only. A mode-7/8
  stream appearing as the inner body of another mode-7/8 stream is
  **rejected** (legit depth is exactly 1; unbounded nesting is a
  stack-exhaustion vector, ~3 bytes per level).
- CRC interaction: unchanged. There is no per-substream CRC; integrity =
  structural rejection (unknown mode byte, bound violations, trailing bytes,
  size equality) + full-consumption checks + the block CRC-32 over the
  reconstructed bytes. A mis-signaled codec that still decodes consistently
  is caught by the size-equality and consumption checks, then by CRC.

## Stream selection and the whole-codec budget (S6-1)

**Determination: S6-1 is INVISIBLE TO THE WIRE.** The whole-codec stream
budget (per-stream codec selection including `raw`, chosen by the additive
`J = L + λ·c` objective at the BLOCK level rather than per-stream
smallest-wins) requires **zero new bits, zero new modes, and zero decoder
changes**. This is by construction, and it is the reason the standing rule
("never change an existing mode's wire semantics") is satisfied:

1. **How the decoder learns each stream's codec:** from the existing
   per-substream mode byte — the first byte of every length-prefixed
   substream in modes 10-15 payloads (`[uvarint zn][mode byte][uvarint N]
   [payload...]`). The decoder dispatches solely on that byte (suite table
   above; unknown -> reject). Which encoder-side policy produced the byte is
   not representable on the wire and does not need to be.
2. **Selection is an encoder-side choice among already-defined codings.**
   Whole-codec budgeting changes WHICH suite codec each substream uses (e.g.
   forcing the shape-distance-delta stream to raw where the decode-cost term
   justifies it — the Linux stream-budget sweep precedent); it does not add
   codings. Every selectable state was already a legal file before S6-1.
3. **Strict validation/rejection rules for malformed selections** are exactly
   the existing per-mode rules, unchanged: unknown mode byte rejected; per-mode
   header validation (rANS model totals, Huffman Kraft inequality, ctx-K
   bounds, mode-7 grammar bounds, mode-8 match bounds); declared-length bound
   `N <= 16*out_len + 64`; trailing-byte rejection per substream and per
   payload. A "malformed selection" is therefore indistinguishable from any
   other corrupt substream and is rejected by the same machinery — there is no
   new failure class.
4. **Consumption accounting is unchanged:** every substream must be fully
   consumed (per-stream cursor/count equality after the last token; for modes
   7/8 both the pulled count and the materialized-buffer cursor). Selection
   never changes how much of a substream a token consumes.
5. **CRC interaction is unchanged:** CRC-32 covers only reconstructed block
   bytes. Codec selection has no CRC surface; the CRC backstops any
   consistent-looking but wrong decode.

**Implemented mechanics (landed `fc23d9a`):** the budget selector
(`encode_stream_budget`) runs ONLY in the mode-15 hot-op encoder behind
`--hotop-budget=on` (default off = byte-identical legacy per-stream path).
Same candidate encoders and wire formats as the suite; only the objective
differs: `J = L + λ·C_us` with `C_us = raw_n · ns_per_byte / 1000`, λ = 0.01
(the EXP. L binding constant), and pre-measurement calibrated decode costs
from decode-perf's t3 floor profile — raw 0.1, rANS-4096/512/256 6.0,
Huffman 4.3, default-exc 3.2, ctx-rANS 7.5 ns/B. Raw is always a candidate.
RLZ/RePair (modes 7/8) are NOT budget candidates (gate rule Y-1 orthogonality).
Combo note: with BOTH `--hotop-budget=on` and `--hotop-rlzp=on`, the budget
picks the J-winner among codecs 0-6 first, then the rlzp pass applies
smallest-wins with modes 7/8 ON TOP — a stream coded 7/8 in the combo was
chosen by size, not by the budget objective; do not read the combo as
budget-governed.

**Measured status (S6-1 verdict v3, arch + bench arbiter):** budget-on
output is SIZE-identical to the legacy path on every corpus file but NOT
byte-identical: near-tie streams flip rANS precision (256 -> 4096/512)
because the legacy objective's distinct cost units (40/35/30) break
equal-length ties toward the cheaper codec while the budget's flat
6.0 ns/B for all rANS variants makes them exact J-ties and strict-`<`
iteration keeps the first-added (higher precision). NO raw-flips; the flips
are ratio/decode-neutral precision changes on ~0.2-0.3%-of-decode streams.
Bar outcomes: A FAIL (decode within noise), B met at size-equality, C FAIL,
tier-2 not met — see `deliverable/s6-1-verdict` v3. The wire contract above
is unaffected: the selector may flip selections in future without any
format change, which is precisely the wire-invisibility property this
section specifies.

Compatibility contract (S6-1 gate controls: flag-off byte-identity and
COMPATIBILITY, pre-reg §5): with the budget
disabled, encoder output MUST be byte-identical to the pre-S6-1 build on all
corpus files; files compressed by ANY earlier build MUST decode identically
on the S6-1 build (verified against the format lane's golden set,
`scratch/format/golden/`, 65 artifacts across all 16 corpus sources and suite
states, manifest `scratch/format/golden-manifest.txt`). Because selection
lives entirely behind the existing mode bytes, no version negotiation,
capability flag, or header bit is needed — old decoders that see a codec they
don't know reject cleanly, which is the correct behavior for a newer file;
old FILES contain only codecs they know, so they decode unchanged.


Encoder selection (per stream) is the **additive** objective specified in
"Stream-codec selection objective" above: `J = L + λ·c` per candidate codec, with
`λ` defaulting to `0.01` and the integer cost units in that section's table
(`--stream-lambda` / `ANVIL_STREAM_LAMBDA` override; `0` = pure length). Any
earlier description of a multiplicative `J = L + λ·C_decode·L` (λ = 0.04) is
superseded — the implemented objective is additive.

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

## Registry coverage (decoder-visible IDs)

Project rule: **every decoder-visible registry ID must have a direct forced
encode→decode test.** Selection-based tests are insufficient — a broken candidate
rots silently when the selector never chooses it (this is how BWT postcoders 0/4
stayed broken). `tests/fuzz.py` enforces the registry as follows.

Verification pin (2026-09-12): canonical `build\anvil.exe` sha256
`72D65150CBD65182C69BA85DAD3A8961C25DE48E794FD9823BCAB4ED20A36505`, `src/anvil.cpp`
sha256 `BDC9047434D19106D1D2316DC094FD615F8B71E3EB0FCA98CAFCD076B53B8CE0`, PASS
with zero skips. Re-pin whenever `src/anvil.cpp` moves.

| family | ids | direct forced test | status |
|---|---|---|---|
| block mode | 0,1,2,3,4,5,10,11,12,13,14,15,16,17 | `registry_coverage_roundtrip` (per-id forced flags; asserts emitted mode + exact round-trip) | supported (16 experimental) |
| rev-2 transform | 0,1,2 | `registry_coverage_roundtrip` (asserts emitted transform/backend + exact round-trip) | supported |
| rev-2 transform | 3 | `registry_coverage_roundtrip` (must round-trip OR be cleanly rejected) | **disabled** (clean up-front rejection) |
| rev-2 backend | 1,2 | `registry_coverage_roundtrip` (asserted inside the transform cases) | supported |
| BWT postcoder | 0..4 | `forced_postcoder_roundtrip` (asserts emitted postcoder id; 1/2/3 round-trip, 0/4 must reject up front with `not supported yet`) | 0/4 disabled, 1/2/3 supported |
| stream-suite codec | 0..6 | `stream_suite_coverage` (deterministic matrix; asserts the emitted codec set) | selection-only (no per-codec force flag) |
| stream-suite codec | 7,8 | `stream_suite_coverage` with `--hotop-rlzp=on` | selection-only; 8 observed, 7 not selected on the matrix |
| malformed ids | transform 4/0xff, backend 0/0xff, postcoder 0xff, rev-2 non-{0,17} block mode | `adversarial_rev2` / `adversarial_bwt` | rejected |

Encoder-side policy that keeps this enforceable: broken forced paths are gated
**up front** (clean nonzero exit, never corrupt/undecodable output), exactly as
postcoders 0 and 4 are. The stream-suite namespace has no per-codec force flag, so
its coverage is asserted by observation rather than forcing; ids 4 and 7 are not
selected by the deterministic matrix and remain selection-only.

## Integrity and malformed input

The decoder rejects invalid magic/revision, invalid block sizes, malformed varints, unknown modes, truncated payloads/substreams, invalid match distances/lengths, stream-consumption mismatches, trailing bytes, and CRC mismatches.

Strict bounds (see the decoder audit, `deliverable/t-format`):

- revision-1 `block_size` is bounded to `[1, 64 MiB]`; revision-2 `block_size`
  is bounded to `[1, 128 MiB]`. Every block length must satisfy
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
- Stream codecs 7/8 (when present) additionally enforce: grammar rule count
  `<= 768`, acyclic in-order rule references (self/forward rejected), reduced
  length `<= N`, exact output-size equality (`N`), per-rule expansion
  `<= N` with cumulative expansion `<= 2*N + 64 KiB`, RLZ match addends
  rejected at `>= 0xFFFFFFFF` before increment (wrap guard), match distance
  `<= current output size`, no mode-7/8 nesting inside mode-7/8 (recursion
  depth cap), and full materialized-buffer consumption.
- Revision 2 additionally permits only block modes `0` and `17`. Mode 17
  rejects unknown transform/backend IDs before backend decode; bounds
  `transformed_size <= 2*blen + 4096`; requires the backend to consume its
  complete payload and reproduce exactly `transformed_size` bytes; then applies
  transform-specific exact-size/full-consumption checks before the block CRC.

All failures surface as a clean nonzero exit with a diagnostic; the decoder
never overruns a buffer or accepts a block whose CRC does not match.
