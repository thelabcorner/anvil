# ANVIL Decoder Malformed-Input Audit — mode 11 (SPARSE-REF) + shared paths

Owner: `format` (t-format). Scope: strict decoder safety of block mode 11
(SPARSE-REF, landed in `src/anvil.cpp` by `arch`) and the shared
substream/header machinery it reuses (modes 10, 1-5, 0). Findings are labeled
`[FIXED]` (already enforced in the landed code), `[OPEN]` (gap confirmed
empirically), or `[CLOSED]` (`[OPEN]` gap remediated and landed by `arch`
during this task).

## Summary

The mode-11 decoder's per-token checks are correct and strict. The two
confirmed robustness gaps lived in the **shared** machinery and predated mode
11, but mode 11 made them reachable with a 7-substream payload. Both were
allocation/amplification guards, not memory-safety bugs (no OOB observed; the
decoder rejects cleanly in every probe). **Both were remediated and landed by
`arch` during this task and re-verified here: the 1 GiB substream probe now
fails in 11 ms with "stream too large" (was ~1 GiB alloc / 188 ms), and a
declared `total = 2^40` is rejected with "declared size exceeds amplification
bound".**

## Audit method

- Full source read of every decode path (`decompress`, `decode_tokens`,
  `decode_tokens_rans`, `decode_tokens_sparse`, `decode_stream`, `rans_decode`,
  varint/CRC helpers).
- Empirical probes with crafted files: 1 GiB substream declaration (65 KB
  file), 64 MiB single-block declaration (35-byte file), huge declared `total`.
  All probes measured with the Release build (clang-cl, Windows).
- Fuzz: `tests/fuzz.py` extended with a **mutation phase** (bit flips, byte
  overwrites, byte drops on valid files) asserting the strict property
  "reject OR reconstruct identical bytes — never accept different output".
  Runs: seed 0xA11E (650 roundtrip variants + 5200 mutations) and seed 0xBEEF
  (1050 + 10500), all PASS.

## Per-path findings

### File header / block loop (`decompress`)
- Magic/revision: `[FIXED]` — rejects bad magic, bad revision, empty file.
- `block_size`: `[FIXED]` — `[1, 64 MiB]` enforced.
- `blen`: `[FIXED]` — `[1, block_size]`, `blen <= total - out.size()`.
- `total`: `[CLOSED]` — was only `<= size_t max` checked. A crafted file can
  declare `total = 2^62` with tiny valid blocks; the loop keeps decoding until
  input runs out (truncated-header reject), but output can grow far beyond
  input size. **Remediation landed by arch:** reject
  `total > (in.size()/7 + 2) * 2^26`. Justification: min block header is 7
  bytes (length varint + mode + payload-length varint + 4-byte CRC), and no
  `blen` may exceed the 64 MiB block cap, so no well-formed encoder output
  exceeds this bound. Verified: 22-byte file declaring `total = 2^40` is
  rejected in ~ms with "declared size exceeds amplification bound".

### Substream machinery (`decode_stream`, shared by modes 10 and 11)
- Per-stream codec mode byte / rANS model: `[FIXED]` — unknown codec mode,
  bad model total (`sum != 4096`), zero/oversized frequencies, duplicate
  symbols, truncated rANS state all rejected.
- Decoded length bound: `[CLOSED]` — was capped at 4 GiB
  (`raw_n > (1ull<<32)`), but modes 10 and 11 decode **all** substreams
  eagerly into full vectors before any token processing. A 65 KB crafted file
  declaring `raw_n = 1 GiB` on six substreams forced a ~1 GiB allocation
  (measured 188 ms) before failing with "truncated rANS renorm".
  **Remediation landed by arch:** `decode_stream(q, qe, max_n)`; each
  substream's decoded length is bounded relative to the block's `out_len` —
  reject `raw_n > 16 * out_len + 64`. Per-substream
  legitimate maxima: literal bytes `<= out_len`; token types `<= out_len`;
  varint streams `<= 10 * out_len` (each varint <= 10 bytes, at most one per
  output byte); masks `<= out_len/8 + 4`; residuals `<= out_len`. `16*out_len`
  covers the varint worst case with margin and never rejects valid output.
  Verified: 65 KB file declaring 1 GiB substreams is rejected in 11 ms with
  "stream too large" (was ~1 GiB alloc / 188 ms).

### Mode 11 per-token decode (`decode_tokens_sparse`) — the new code
- `[FIXED]` unknown token type > 2 rejected.
- `[FIXED]` literal run: `len <= out_len - out.size()` and `len <= literals
  remaining`.
- `[FIXED]` exact match: `dist in [1, out.size()]`, `len <= out_len - out.size()`.
- `[FIXED]` sparse match: `dist in [1, out.size()]`, `4 <= len <= 65536`,
  `len <= out_len - out.size()`, `nwords*4 <= masks remaining`.
- `[FIXED]` mask bits beyond `len` in the final word rejected.
- `[FIXED]` `popcount(mask) <= residuals remaining` (with final full-consumption
  check this enforces `len(residuals) == popcount(mask)` exactly).
- `[FIXED]` periodic overlapping copy (memcpy-style RLE) — `out[out.size()-dist]`
  indexing is always in-range because `dist <= out.size()` is checked first.
- `[FIXED]` corrections applied after the full copy against the periodic copy;
  `out[start + 32*w + b]` is in-range because bits beyond `len` were rejected.
- `[FIXED]` all 7 substreams must be fully consumed; payload trailing bytes
  rejected; block CRC-32 verified.

### Arithmetic backend (modes 1-5) — unchanged
- `[FIXED]` varint overflow (10-byte cap), literal run exceeds block, invalid
  match distance, match exceeds block — all rejected. Decoder pads with zero
  bits past stream end; garbage that *changes decoded bits* is caught by
  CRC-32.
- `[OPEN]` **F1 — in-payload trailing garbage is accepted.** The arithmetic
  coder is self-terminating (decoder stops at the declared block length) and
  never verifies full payload consumption; the block CRC covers only the
  *reconstructed* bytes. Trailing full bytes spliced inside a mode-1..5
  payload are therefore silently accepted (decoder exit 0, output correct).
  Repro: compress `tests/corpus/doc.md` with `--parse=greedy
  --literal=o0 --entropy=arith`, splice 14 bytes of garbage into the mode-1
  payload (extending `plen`, CRC unchanged) -> decoder exits 0. Same splice on
  modes 10/11 -> exit 1 ("payload trailing bytes"); the gap is arithmetic-only
  because rANS/sparse substreams are length-prefixed with full-consumption
  checks. Severity: LOW — no memory unsafety, no incorrect output (CRC still
  binds the reconstructed bytes), but it violates the documented "strict
  decoder" contract and can mask encoder bugs that write extra bytes.
  Suggested remediation (arch lane; no wire-format change, valid files
  unaffected): track the arithmetic `BitReader` byte position after
  `decode_tokens` returns and reject when a *full* trailing byte was never
  consumed (allow only the final partial byte's zero padding). Status:
  reported to arch; not yet remediated.

## Follow-up status

Both `[OPEN]` findings from this audit were remediated by `arch` in
`src/anvil.cpp` (confirmed in tree, re-verified with fuzz):

1. `decompress`: `total` bounded by `(in.size()/7 + 2) * 2^26` —
   "declared size exceeds amplification bound".
2. `decode_stream(q, qe, max_n)` with `max_sub = 16*out_len + 64` from both
   `decode_tokens_rans` and `decode_tokens_sparse` — "stream too large".

Both keep the existing clean-reject behavior and are documented in FORMAT.md
("Integrity and malformed input" section) as required decoder invariants.

## Fuzz harness changes (this task)

`tests/fuzz.py`:
- Added `('sparse','rans')` to the round-trip/truncation combo matrix
  (mode 11 coverage).
- Added `--mutations` phase: per valid encoded file, N random mutations
  (1-3 of bit-flip / byte-overwrite / byte-drop), asserting the decoder either
  exits nonzero or reconstructs byte-identical output. Accepting a mutation
  with *different* output is reported as a bug.
