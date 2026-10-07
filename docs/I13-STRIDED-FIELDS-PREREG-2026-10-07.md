# ANVIL I13 — Strided Field Reconstruction Pre-Registration

**Date:** 2026-10-07, locked before any I13 C++ build, codec invocation, or benchmark.  
**Execution:** manual `workflow_dispatch` GitHub Actions ONLY for CPU-heavy work. No delegates.  
**Source:** `prototypes/i13-strided/i13_strided.cpp` SHA-256 `274be4c8a5de222b4bbb16c586fc04310dd1deadd455dacc7741110b8dfcb7c0`, Git blob `ca3f9e684917b276f6a4274de62c3b20ca73fa95`.  
**Experiment class:** Discovery and mechanism falsification only. **NO** production format, novelty claim, heldout promotion, or full Pareto reclassification.

## Frozen causal basis

The [I12 measured result](I12-BOUNDED-INNOVATION-RESULTS-2026-10-07.md) passes all synthetic-arithmetic preregistration gates: 256,000 input bytes become **62,458 AVI2 bytes** versus **87,013 Brotli q11 bytes**; same-run digest-inclusive decode is **358.872 MB/s** versus **141.060 MB/s** with **all 63 blocks** represented by dense mathematical residuals. But `synth-timeseries.bin` remains entirely RAW under I12: 280,214 B versus Brotli q11 **134,718 B**. The timeseries is an already-consumed 280,000-byte binary containing repeated fields, not a fresh heldout.

I13 tests a new **representation arrangement**: field-local modular predictors and independent raw columns, with absolute source byte reconstruction, rather than letting one unpredictable lane force the entire mixed block to RAW. Interleaving, transpose/bitshuffle, FOR/delta, ZigZag and column encoders are occupied prior art. **No novelty.**

## Frozen mechanism and complete serialized charge

Independent blocks <=4096 bytes; outer `AVI3 || canonical_uvarint(uncompressed_size)`. Retains AVI2 RAW and width-1/2/4/8 full-block affine+dense innovation as optional leaf. Adds tag 4:

`COL = u8(4) | canonical_uvarint(block_bytes) | u8(record_width_words S) | per-column descriptors for lanes 0..S-1 | exact raw tail`.

- Fixed 32-bit little-endian words; search only `S ∈ {2,...,12}`. For a given block `n`, full records `R=floor(n/(4S))`; require `R>=3`. A block with partial trailing record appends exactly `n-4SR` unchanged bytes, charged fully.
- Each column is evaluated as either `0 | R*4 raw column bytes`, or `1 | base_LE32 | step_LE32 | u8(k) | ceil(R*k/8) packed ZigZag innovations`. Column slope is median of modular adjacent differences, base is median projected intercept; the generator is the prior I12 modular sequence model, now field-local. Require `k<32`. Choose modeled column only when its **entire** tag/header/payload is smaller than RAW; ties choose RAW. Zero padding bits, varint canonicality, bounds and word endianness enforced.
- `COL` is selected only when its full serialized cost strictly beats the best RAW or AVI2 full-block candidate. Otherwise preserve prior semantics. For every possible stride, unchosen bytes/headers are not elided from costing. No decoder-side sorting, learned model, variable search, external schema or reference history.
- Bounded decoder: sequential column payload parse then within-block fixed-stride scatter to exactly reconstruct each field at its original offsets, followed by literal tail. Decoding complexity `O(input_words)`, memory bounded by block and source/output cap. No forward pointer chasing.
- This precise fixed stride catalog is **not a claim of optimized or exhaustive field discovery**; model search encoder overhead is measured.

## Frozen hypotheses and gates

1. **H1 (structure discovery):** At least one `COL` block containing >=1 modeled lane is selected on the same seven-source discovery cohort, specifically on `synth-timeseries.bin`. Zero selected → kill I13 under frozen stride geometry, do not after-the-fact expand catalog.
2. **H2 (complete bytes):** Complete AVI3 wire bytes < Brotli q11 complete bytes on `synth-timeseries.bin` (Brotli 1.1.0 fixed `lgwin22`, quality11), on the same runner. Block geometry is non-equivalent, so positive result is **specialized discovery only**.
3. **H3 (decoded work):** On the *same* model-bearing `synth-timeseries.bin`, AVI3 digest-inclusive paired decode MB/s > Brotli q11. A RAW-only speedup is invalid evidence.
4. **H4 (compatibility):** Existing `synth-arith.bin` AVI3 full-wire bytes <= previously measured 62,458 B, since an exact I12 candidate remains available. It is a sanity gate, not independent byte evidence.
5. **H5 (safety):** Mixed-column synthetic selftest with three opaque noise lanes, low-entropy lanes and a trailing partial record exercises tag 4; bit-exact roundtrip and truncation/malformed stream rejection. All fixed seven tracked corpus inputs must round-trip exactly.

Inputs exactly: `synth-arith.bin`, `synth-timeseries.bin`, `synth-jitter.bin`, `random.bin`, `generated.repeat.jsonl`, `generated.jsonl`, `src.cpp` under `tests/corpus/`, no newly added heldout. Brotli exact static source commit `ed738e842d2fbdf2d6459e39267a633c4a9b2f5d`. Checkout/head, tool source SHA+Git blob, per-input Git blob and SHA256, both reference producer and selftests must pass before authoritative discovery measurements. Same GitHub runner; byte-exact full stream sizes, per-mode column counts/field counts, encode and digest-inclusive decode medians, error/tail behaviors, compilation, binary hash/size, runner metadata and evidence artifact retained.

**Stop rules:** source drift, compiler/codec/roundtrip issue ⇒ `INVALID_INFRA`, repair only and rebind source/runner identity before any scientific measurement. H1 false ⇒ `KILL-STRIDE-DISCOVERY`. H1 true/H2 false ⇒ `STRUCTURE-BUT-NO-BYTE-WIN`. H1,H2 true/H3 false ⇒ `RATIO-ONLY-DISCOVERY`. All three true ⇒ `STRIDED-SPECIALIZED-DISCOVERY`. H4 false voids any compatibility claim; retain transparent negative evidence, fix if a correctness defect. **No R2 Class-A reclassification under any I13 scenario.**

## Genuine promotion remains unfunded

Need legal provenance and independent real telemetry families, full corpus Gate B, fair block/window geometry, rigorous whole-system encode/decode/RSS/binary memory accounting, compare Gorilla/Chimp/ALP/FastLanes/Sprintz, and reproducible physical infrastructure. Improving a synthetic source over one Brotli setting is not establishing global ANVIL Pareto membership.
