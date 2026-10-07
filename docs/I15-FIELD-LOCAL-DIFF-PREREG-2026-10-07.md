# ANVIL I15 — Field-Local Modular Difference Innovations (Frozen Discovery)

**Date:** 2026-10-07. Locked before any I15 compilation, codec execution or benchmark.

**Source:** prototypes/i15-field-diff/i15_field_diff.cpp — SHA-256 ce9ff217ca32cae761362e11261ee9fdacaca7b27dcb86d9b2d41fb6a73ff612; Git blob fa60ec8fdab9931b5befae6e6575de8eb5ff0321.

**Control:** frozen I13 source prototypes/i13-strided/i13_strided.cpp — SHA-256 274be4c8a5de222b4bbb16c586fc04310dd1deadd455dacc7741110b8dfcb7c0; Git blob ca3f9e684917b276f6a4274de62c3b20ca73fa95.

**Reference:** Brotli upstream commit ed738e842d2fbdf2d6459e39267a633c4a9b2f5d, q5/q11, lgwin22.

**Execution:** all CPU-heavy compilation, fuzzing and benchmarks via manually dispatched GitHub Actions only, no delegates or swarms. Discovery-only: no held-out promotion, no general-purpose frontier claim.

## 1. Causal motivation

I13 successfully recovered 407 modeled 32-bit fields in all 69 previously consumed time-series blocks, but its complete archive was 139,387 B versus Brotli q11 134,718 B: a 4,669-B deficit. I14 then demonstrated local modular step differences on known arithmetic data: 33,196 B versus I12 62,458 B and Brotli q11 87,013 B, with 63 DIFF-bearing blocks and paired digest-inclusive decode 361.970 versus 141.741 MB/s. These facts suggest testing local innovations *within* I13's field lanes. This experiment intentionally retains I13's imperfect 4-byte word stride catalog to isolate the residual representation from field-schema discovery.

## 2. Exact representation and complete-byte selection

Independent blocks <=4096 B; AVI5 wire with canonical original length, exact trailing consumption, zero padding, output ceiling 256 MiB. Retain every old I13 candidate: RAW outer tag0, AFF outer tag3, COL outer tag4, including RAW=0/AFF=1 column submodes. A new whole-block DIFF outer tag5 writes width, count, first word, median step, bitwidth and packed n-1 local ZigZag residuals. A new COL submode2 writes first 32-bit word, median 32-bit step, packed n-1 local residuals; the decoder uses sequential modular recurrence and scatter.

For b-bit values in Z/(2^b), each innovation is e_i = (x_i - x_(i-1) - step) mod 2^b. Reconstruct x_i = (x_(i-1) + step + unzig(e_i)) mod 2^b. The encoder estimates the step by median signed modular adjacent differences. Cost includes all tags, first/step, bit width, packed payload, raw columns and literal tail. Only a strictly smaller **actually serialized** candidate replaces prior best; ties preserve previous. Fixed column stride catalog 2..12 32-bit words, >=3 full records, all input bytes reconstructed.

The candidate does not infer a new semantic field layout. No mechanism novelty: delta, delta-of-delta and bitpacking have extensive Sprintz/FastLanes/PFOR prior art.

## 3. Frozen cohort

Precisely seven previously consumed tracked inputs, fixed order: tests/corpus/synth-arith.bin; synth-timeseries.bin; synth-jitter.bin; random.bin; generated.repeat.jsonl; generated.jsonl; src.cpp. Never relabel this cohort held out.

## 4. Hypotheses

- **H1 selection:** at least one *column DIFF* selected on synth-timeseries.bin, not merely RAW/global mode.
- **H2 complete bytes:** I15 AVI5 full wire **strictly below** same-run I13 AVI3 AND same-run Brotli q11 on synth-timeseries.bin; distinguish incremental improvement from true reference crossing.
- **H3 decoded work:** paired byte-digest-inclusive I15 decode rate > same-run Brotli q11 on actually column-DIFF-bearing time-series. Measure same-run I13 for context.
- **H4 retained-choice invariant:** for all seven original inputs, I15 bytes <= I13 bytes, as all original candidates persist with identical boundaries. Arithmetic I15 bytes <= historical I14 33,196 B (deterministic byte comparison only; never splice different-run throughput).
- **H5 honest negatives:** show every file, size, model/column counts, encode/decode MB/s; repeated JSONL and C++ are binding.
- **H6 correctness:** source and corpus Git blob/SHA checks, frozen Brotli, complete I13 and I15 byte-exact file roundtrips, exhaustive in-source test for DIFF/column DIFF and malformed/short/padding/tail/wrap behavior, every-byte digest, retrievable artifact.

## 5. Predeclared adjudication

Source drift, build failure, malformed acceptance, mismatch, invalid complete-byte accounting or incomplete artifact => INVALID-INFRA, no measurement verdict. H1 false => KILL-STRIDED-DIFF. H1 true but I15 >= I13 => SELECTED-NO-INCREMENT. I15 improves I13 but remains >= Brotli q11 => STRUCTURE-ADVANCE-NO-BROTLI-CROSSING. Both size gates pass but decode loses => RATIO-ONLY-STRIDED-DISCOVERY. H1/H2/H3 all pass => SPECIALIZED-STRIDED-DISCOVERY-ADVANCE. None is a general-purpose frontier crossing.

## 6. Competing follow-up branches, not funded by this pilot

- If 14-byte field framing matters more than residuals, preregister I16 byte-stride/offset discovery with exact gap reconstruction; do not silently change I15 stride catalog.
- If outlier bitwidth dominates, separately test PFOR-style sparse exceptions and charge location/payload headers fully.
- If decode is serially limited, separately compare prefix-scan tile restarts, lane interleaving and SIMD transposition with full-wire overhead.
- If text/JSONL still lose badly, require an actual complete-file LZ/Brotli/Zstd fallback, including mode header and encoder cost, before any release integration.
- Holdout sources must be independent real numeric families with legal provenance, compared with Sprintz, FastLanes, Gorilla/Chimp, ALP and a range of general codecs.

**Historical Class-A ANVIL status remains 435 dominated, 28 degenerate, 5 unresolved, zero verified general-purpose crossings.** Original dirty checkout is protected.

## 7. Derivation and provenance

tools/i15_assemble.py mechanically generates I15 by exact-match replacements against the frozen I13 source. The assembly script is file manipulation only, never a CPU benchmark. The preregistered immutable output source blob and SHA above bind the workflow. All subsequent corrections must be logged as invalid-premeasurement infra if discovered before a valid remote result.

## Premeasurement fixture correction — run 37700369378

First I15 Actions run 37700369378 passed immutable source provenance, the pinned Brotli static build, compilation, and all seven frozen I13 controls. It stopped **before I15 corpus measurements** when an artificially periodic walking selftest was encoded more efficiently by the retained strided candidate than by a whole-block DIFF; it did not test the intended local innovation branch. Correction changes ONLY two in-source selftest fixtures to deterministic, bounded nonperiodic modular step noise; source format, actual codec encoder/decoder, frozen seven inputs, reference, thresholds, and adjudication are unchanged. No efficacy evidence from run 37700369378. New source SHA-256 ce9ff217ca32cae761362e11261ee9fdacaca7b27dcb86d9b2d41fb6a73ff612; blob fa60ec8fdab9931b5befae6e6575de8eb5ff0321. The tracked reproducible assembler produces exactly these source bytes. A new source-attested remote run is mandatory.
