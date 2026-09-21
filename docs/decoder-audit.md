# ANVIL Decoder Malformed-Input Audit — mode 11 (SPARSE-REF) + shared paths

Owner: `format` (t-format). Scope: strict decoder safety of block mode 11
(SPARSE-REF, landed in `src/anvil.cpp` by `arch`) and the shared
substream/header machinery it reuses (modes 10, 1-5, 0). Findings are labeled
`[FIXED]` (already enforced in the landed code), `[OPEN]` (gap confirmed
empirically), or `[CLOSED]` (`[OPEN]` gap remediated and landed by `arch`
during this task).

## Summary

The mode-11 decoder's per-token checks are correct and strict. The three
confirmed robustness gaps lived in the **shared** machinery (two) and the
arithmetic backend (one, F1, found by bench); none were memory-safety bugs (no
OOB observed; the decoder rejects cleanly in every probe). **All three were
remediated and landed by `arch` during this task and re-verified here: the
1 GiB substream probe now fails in 11 ms with "stream too large" (was ~1 GiB
alloc / 188 ms), a declared `total = 2^40` is rejected with "declared size
exceeds amplification bound", and arithmetic trailing-byte splices are
rejected with "trailing arithmetic bytes".**

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
- `[CLOSED]` **F1 — in-payload trailing garbage was accepted** (found by
  `bench`, independently confirmed by `format`; remediation landed by `arch`).
  The arithmetic coder is self-terminating (decoder stops at the declared
  block length) and did not verify full payload consumption; the block CRC
  covers only the *reconstructed* bytes. Trailing full bytes spliced inside a
  mode-1..5 payload were silently accepted (decoder exit 0, output correct).
  Repro: compress `tests/corpus/doc.md` with `--parse=greedy
  --literal=o0 --entropy=arith`, splice 14 bytes of garbage into the mode-1
  payload (extending `plen`, CRC unchanged) -> decoder exited 0 with
  byte-identical output. Same splice on modes 10/11 -> exit 1 ("payload
  trailing bytes"); the gap was arithmetic-only because rANS/sparse substreams
  are length-prefixed with full-consumption checks.
  Confirmation detail (`format`, t-format): a first splice harness was buggy
  (duplicated the block header in the rebuild — a no-op control failed), which
  produced false "rejected" results; with a verified harness (no-op rebuild
  control passes) appending `zeros`, `0xAA` and `deadbeef` tails all decoded
  exit 0 with identical SHA-256. Mechanism: `ArithmeticDecoder`'s
  `BitReader::bit()` returns 0 past the stream end without advancing, so once
  the declared block length is reached the decoder never reads (nor validates)
  the remaining payload bytes.
  Remediation landed by `arch` in `decode_tokens`:
  `if (n >= ad.consumed_bytes() + 2) throw "trailing arithmetic bytes"` where
  `consumed_bytes() = br_.byte + (br_.bitpos ? 1 : 0)` — rejects when a *full*
  trailing byte was never consumed, allowing only the final partial byte's
  zero padding. No wire-format change; valid files unaffected. Verified:
  all three splice patterns now exit 1; fuzz re-run clean (800 roundtrip
  variants + 6400 mutations PASS). Severity was LOW (no memory unsafety, no
  incorrect output) but it violated the strict-decoder contract and could mask
  encoder bugs that write extra bytes.

## Follow-up status

All `[OPEN]`/`[CLOSED]` findings from this audit were remediated and landed in
`src/anvil.cpp` (confirmed in tree, re-verified with fuzz):

1. `decompress`: `total` bounded by `(in.size()/7 + 2) * 2^26` —
   "declared size exceeds amplification bound".
2. `decode_stream(q, qe, max_n)` with `max_sub = 16*out_len + 64` from both
   `decode_tokens_rans` and `decode_tokens_sparse` — "stream too large".
3. F1 (arithmetic trailing bytes): `consumed_bytes()` tracked on
   `ArithmeticDecoder`; `decode_tokens` rejects `n >= consumed_bytes() + 2` —
   "trailing arithmetic bytes".

All keep the existing clean-reject behavior and are documented in FORMAT.md
("Integrity and malformed input" section) as required decoder invariants.

## Fuzz harness changes (this task)

`tests/fuzz.py`:
- Added `('sparse','rans')` to the round-trip/truncation combo matrix
  (mode 11 coverage).
- Added `--mutations` phase: per valid encoded file, N random mutations
  (1-3 of bit-flip / byte-overwrite / byte-drop), asserting the decoder either
  exits nonzero or reconstructs byte-identical output. Accepting a mutation
  with *different* output is reported as a bug.

---

# Addendum — iteration 6: stream codecs 7/8 (Experiment Y) + S6-1 selection

Owner: `format`. Scope: strict-decoder review of arch's in-flight
Experiment-Y diff (stream-suite extensions mode 7 RePair / mode 8 RLZ,
uncommitted in the tree at audit time) and the S6-1 whole-codec stream-budget
wire surface. Labels as above; `[OPEN]` findings have remediations specified
and are pending `arch`'s landing.

## Wire-format verdict

Modes 7/8 are an **additive extension of the stream-codec namespace** (0-6 →
0-8). No existing block mode or stream codec changes semantics. The encoder
wiring (`g_rlz_reap`, `--hotop-rlzp`) was not landed at audit time — the flag
is declared in the options struct but never parsed nor read — so no encoder
output can contain modes 7/8 yet; old files are unaffected by construction,
and old binaries reject the new codecs cleanly ("unknown stream codec"),
which is the correct behavior for a newer file. Spec'd normatively in
FORMAT.md ("Stream-codec suite extensions" + "Stream selection and the
whole-codec budget (S6-1)").

**S6-1 determination: wire-invisible.** Whole-codec J-selection (incl. raw
candidates) only changes WHICH already-defined suite codec each substream's
mode byte names; the decoder learns each stream's codec solely from that
byte, validates with the existing per-mode rules, accounts consumption
identically, and has no CRC surface change. No new failure class exists.
Full contract in FORMAT.md §"Stream selection and the whole-codec budget".

## Findings (modes 7/8 decode paths)

Status FINAL: all findings `[CLOSED]` and COMMITTED — F1-F5 landed in
`da01c54` (Experiment Y), verified by `format` probes + `linux-ref`
independent re-read; S6-1 landed in `fc23d9a` (wire-invisible, confirmed:
same candidate encoders, selection-only change, decoder untouched); two
ASan-found latent OOBs fixed in `aea3023` — MatchFinder::find EOF probe
guard (encoder-side, no wire surface) and mode-13 topology modal/has_modal
tables outer dim 64->65 (in-memory bound on BOTH sides; the modal-table wire
format is untouched and no pre-fix file could legally contain k==64 entries
since the encoder wrote out of bounds producing them — "wire unchanged"
claim verified by `format`).

### F1 — `StreamPull::at_end()` mis-evaluates codecs 7/8 `[CLOSED]`
Was: no codec-7/8 branch — fell through to the defexc condition
`bits_done == total && exc == exc_end` = `0 == raw_n`, false for every
non-empty 7/8 substream, so `decode_tokens_hotop_fused`'s consumption gate
rejected every valid file with a mode-7/8 opcode/literal stream.
**Remediation landed by arch:** explicit branch
`if (codec == 7 || codec == 8) return rp_i >= rp_buf.size();`.
(Independently found as D4 by `linux-ref`'s audit — convergence recorded.)

### F2 — mode-8 match addend wrap → out-of-bounds read `[CLOSED]`
Was: `dist = get_uvar(bp,be)+1` and `len = get_uvar(bp,be)+kRlzMinMatch`
incremented untrusted varints; `get_uvar` accepts ten-byte encodings up to
`UINT64_MAX`, so `+1` wraps to 0, the `dist > out.size()` check passes, and
`out[out.size()-dist]` reads out of bounds. **Remediation landed by arch**
exactly as specified: reject `dv >= 0xFFFFFFFF || lv >= 0xFFFFFFFF` BEFORE
increment ("bad rlz match varint", macro-path precedent).
EMPIRICAL VERIFICATION (`format`, post-fix build): crafted probes in
`scratch/format/probes/` all reject cleanly in <10 ms on the fixed decoder —
P_F2_WRAP.anv (59 B) -> "bad rlz match varint"; P_F3_AMP.anv (212 B,
40-rule doubling grammar) -> "reap rule expansion too large" (was exponential
allocation); P_F4_NEST.anv (145,899 B, 30K-deep nesting) -> "nested rlz-reap
stream" (was stack exhaustion). Generator: `scratch/format/make_probes.py`.

### F3 — mode-7 rule-table materialization amplification `[CLOSED]`
Was: `exp[i]` materialized eagerly per rule with no bound tied to `raw_n`; a
crafted doubling chain (`rule_i = (rule_{i-1}, rule_{i-1})`, ~3 bytes/level,
R <= 768) forced exponentially growing allocations before the final size
check. **Remediation landed by arch** exactly as specified: per-rule
`|exp[i]| <= raw_n` and cumulative `<= 2*raw_n + 64 KiB`
("reap cumulative expansion too large"). Non-rejecting proof: folding
conserves every rule's reference count at >= 2 in the final structure, hence
every legit `|exp[i]| <= raw_n/2`. Probe:
`scratch/format/probes/P_F3_AMP.anv`.

### F4 — unbounded `decode_stream` recursion via 7/8 inner streams `[CLOSED]`
Was: crafted nested `[7][uvar][uvar]` headers (~3 bytes/level) recursed
~filesize/3 frames deep — stack exhaustion instead of clean reject. Legit
depth is exactly 1 (inner bodies come from `encode_stream`, whose candidate
set excludes 7/8). **Remediation landed by arch:** `depth` parameter,
`if (mode >= 7 && depth > 0) throw "nested rlz-reap stream"`. Probe:
`scratch/format/probes/P_F4_NEST.anv`.

### F5 — mode-8 length-convention mismatch, encoder side (= linux-ref D3) `[CLOSED]`
Was: encoder stored match length RAW (`val2 = bestlen`) while the decoder
computes `len = varint + kRlzMinMatch` — every legitimate mode-8 stream
containing a match failed round-trip loudly ("bad rlz match" / size
mismatch). Found independently by `linux-ref` (D3) after this audit's initial
pass; accepted into the canonical list. **Remediation landed by arch on the
ENCODER side:** store `(dist-1, len-kRlzMinMatch)` (anvil.cpp RePair/RLZ
encoder, comment-pinned). Wire convention now stated explicitly in FORMAT.md
mode-8 layout: stored values are `dist-1` and `len-4`, matching the house
style of modes 12/15 (`uvarint(len-4)` macro streams).

### Notes (no action required)
- RLZ 64 KiB self-reference window is ENCODER-side only; the decoder accepting
  any `dist <= out.size()` is intentional and documented (window is not part
  of the wire contract, so a future window widening needs no new mode).
- Mode-7 low-value guard (`(X,X)` pair with count 2 skipped) affects encoder
  efficiency only; termination is length-based and unaffected.
- FORMAT.md's previous multiplicative J-formula text (λ=0.04) was stale vs
  the implemented additive objective (λ=0.01); corrected in FORMAT.md this
  task, consistent with the frozen S6-1 pre-registration §3 (corroborated as
  drift by linux-ref's V4).

## Old-file compatibility protocol (S6-1 gate control: COMPATIBILITY, pre-reg §5 item 5)

No pre-change `.anv` artifacts exist in git history, so `format` built a
golden set from the CURRENT binary (cross-verified freeze-faithful — hotop
generated.log = 175,550 B exactly matches the gate snapshot; and an
independent A/B: a scratch-built HEAD-392e937 binary vs the current build
produce BYTE-IDENTICAL output on --parse=auto and --parse=hotop for
tests/corpus/anvil.exe, SHA 4081D221/75A8F313 — the Exp-Y diff is
byte-invisible with flags off): 65 `.anv` artifacts under
`scratch/format/golden/`, covering all 16 corpus sources x {hotop, sparse,
auto} plus representative {arith, shape, tcopy, topology,
hotop --stream-suite=off} rows (set regenerated after bench grew the corpus
by three synth files; manifest with SHA-256 at
`scratch/format/golden-manifest.txt`). All round-trip verified at creation;
all 65 re-decode byte-identical on the Experiment-Y build — no old-file
regression from the stream-codec addition.
**§5.5 COMPATIBILITY SIGN-OFF: GREEN** — final sign-off binary
(build\anvil.exe SHA-256 prefix CE44DE45, HEAD fc23d9a + bench's bench_native
row): 65/65 artifacts decode byte-identical to manifest hashes. Old files
( every block mode and suite state in the set) decode on the S6-1/Exp-Y
build unchanged.

Provenance note: an earlier cross-check by fuzz-verify reported two
auto+PE artifacts as "stale-sized" vs expected 107,178/824,316 B. That
reference does not reproduce from ANY build available (04:44 tree build,
05:50 tree build, scratch HEAD build all yield 104,605/806,497 B,
deterministically across runs); treated as a harness artifact on the
cross-check side and superseded by the A/B hash evidence above.
After S6-1 lands: re-decode all 65 with the new build against recorded
hashes — that is the `format` sign-off for the gate's COMPATIBILITY control
("old files decode"). The same golden set is named by research-gate's
skeleton v3 as the evidence mechanism for the flag-off byte-identity control.
