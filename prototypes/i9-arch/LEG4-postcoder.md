# arch I9 — Decode leg 4: postcoder token decode (buffered renorm + two-level cumulative tables)

Status: implemented, correctness gates in progress, measurement pending (window
requested). Wire-identical by construction (decoder-only; arithmetic intervals and
model frequencies unchanged).

## What changed (decoder-only)

Per `prototypes/i9-decode-perf/POSTCODER-SPEC.md`:

1. `BitReader` (src ~67): the arithmetic decoder's bit reader now buffers one byte
   into an accumulator (`acc`/`have`) instead of recomputing `p[byte] >>
   (7-bitpos)` per bit. Zero-padding past the payload and `consumed_bytes()`
   semantics (partial byte counts as touched) are preserved exactly.
2. `AdaptiveDecModel` (new, after `AdaptiveModel`): decode-only adaptive model
   with the SAME frequencies, rescale rule (`total_ >= kModelRescale`, halve with
   floor >= 1) and totals as `AdaptiveModel`. The Fenwick prefix search is
   replaced by a two-level walk: 16-symbol block sums (`blk_`) then within-block
   frequencies. The cumulative interval handed to `ArithmeticDecoder::consume` is
   exactly the prefix sum, so decoded symbols, intervals and bits are identical.
   No `add_tree` maintenance on the decode path.
3. `decode_uvar_m` (template) mirrors `decode_uvar` for the decode-only model.
4. `bwt_arith_decode` uses `AdaptiveDecModel` (tok0 / runm / per-context tok1)
   and `decode_uvar_m`.

Not changed: formulas (`scaled`/`consume` divisions kept exact), model alphabet
sizes, rescale threshold, encoder (`bwt_arith_encode`) untouched. The three 64-bit
divisions per symbol are retained deliberately — replacing them with reciprocal
multiplies would risk wire drift and historically measured slower (do-not-reburn
D3).

## Pins

| artifact | sha256 |
|---|---|
| src/anvil.cpp (ALLOC + leg 4, RETAINED) | BDC9047434D19106D1D2316DC094FD615F8B71E3EB0FCA98CAFCD076B53B8CE0 |
| canonical shared build/anvil.exe | 72D65150CBD65182C69BA85DAD3A8961C25DE48E794FD9823BCAB4ED20A36505 |
| canonical shared build/anvil_bench.exe | 379341D93B6504EBDADF8BDB7FE69B2756142FD9823E5398A31E9A5C2176851E |
| lane build-i9-arch/anvil.exe (leg 4) | FE0CF4F12DE47E0352CFBD562299144352E18102A66169B9A1F8C14D4A86CEF7 |
| src pre-leg4 (variant used for A/B) | variants/anvil_preleg4.cpp (reverse-edit of the 5 leg-4 hunks) |
| A/B arms | prototypes/i9-arch/dt_leg4.exe (21517573...) vs dt_preleg4.exe (1B34D319...), manual clang-cl /O2, BROTLI+LIBSAIS |

## Correctness gates (pending completion)

- full-corpus roundtrip 24 files x 18 specs: logs/gate_roundtrip_leg4.log
- fuzz `--cases 50`: logs/fuzz50_leg4.log
- encode identity 494-case rerun (decoder-only, expected 494/494)
- BWT postcoders 1/2/3 forced roundtrip on generated.log (smoke PASS before build)

## Attested window w-arch-leg4-20260912T1150Z (interleaved, reps=7, core18, 1t)

| file | preleg4 ns/outB (CV) | leg4 ns/outB (CV) | speedup |
|---|---|---|---|
| dickens.bwt.anv (10.19 MB out) | 89.121 (11.89%) | 73.353 (9.50%) | 1.215x |
| webster.bwt.anv (41.46 MB out) | 74.980 (3.41%) | 64.135 (17.50%) | 1.169x |
| bytes-weighted aggregate | — | — | **1.179x** |

Arms dt_preleg4 `1B34D319...` / dt_leg4 `21517573...`; containers unchanged
(dickens `1F8FE57D...`, webster `0DBECD01...`). Log
`logs/dt_ab_w-arch-leg4.log`.

## Verdict

**FALSIFIED as implemented**: 1.179x aggregate < the ~1.4x target. The two named
levers (buffered renorm + block-sum decode) are real and wire-identical, but
bounded; the fallback is a new postcoder ID carrying transmitted static tables,
which is a format change requiring format + research-gate sign-off before any
build. Retained only as a partial wire-identical decode win if the coordinator
chooses to keep it; otherwise the leg-4 hunks are revertible.
