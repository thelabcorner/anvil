# ANVIL I14 — Bounded Modular Difference Innovations: Preregistered Discovery

**Date:** 2026-10-07 (freeze before any I14 build/codec run).  
**Source:** `prototypes/i14-differential/i14_differential.cpp`  
**SHA-256:** `7d2305f857d81887d9cfa155b04763c2c9c620b47a4def916e66e67090158346`  
**Git blob:** `e94f31ea8ce0634b62010e7006c62479b85c03dd`  
**Primary control:** I12 source blob `2e6ea6492f970ee56366b5530872da193a815226`.  
**Brotli control:** source commit `ed738e842d2fbdf2d6459e39267a633c4a9b2f5d` (q5/q11, window 22).  
**Role:** discovery only; zero Class-A general-purpose frontier crossings remain. No held-out evaluation or novelty claims.
**Execution policy:** all C++ compilation, codec roundtrip tests, benchmarking, and fuzzing solely via GitHub Actions. No delegated workers/swarm.

## 1. Motivation and causal branch

I12 passed its frozen hypotheses on the *previously consumed* synthetic arithmetic input in [run 37697658306](https://github.com/thelabcorner/anvil/actions/runs/37697658306): AVI2 62,458 B vs Brotli q11 87,013 B; measured paired decode-plus-every-byte-digest 358.872 vs 141.060 MB/s; 63/63 modeling. Its other six files selected RAW and lost on bytes. The result is promising specialized discovery **not** a complete frontier claim. I13 separately investigates strided record extraction. I14 isolates the *residual representation*: local modular step deviations instead of the cumulative error against a global affine line.

For each independent <=4096 B block, preserve I12's RAW and AFFINE candidates and add DIFF. Test widths w in {1,2,4,8} bytes with exact divisibility. Treat each word as a ring element of Z/(2^(8w)). Estimate the median signed modular adjacent delta `d` (encoder-only). Seed `s=x[0]`; encode exactly `e_i=(x_i - x_(i-1) - d) mod 2^(8w)` for i>=1. ZigZag signed modular innovations, find the full-block maximum width k, and bitpack all n-1 residuals with canonical zero padding. Decode `x_i=(x_(i-1)+d+unzig(e_i)) mod 2^(8w)`. A single different local step cannot perturb all later residuals as it can with the global-affine residual.

**Wire:** AVI4 magic, canonical total length, independent RAW=0, AFFINE=3 (unchanged I12 math), or DIFF=4; DIFF block = tag(1), width(1), canonical count varint, seed[w], step[w], k(1), ceil((count-1)*k/8) packed bytes. Select strictly smaller **complete wire bytes** against current best. Ties retain earlier mode. Reject k >= 8w or k > 56. At decoder: 256MiB total ceiling; count and word width bounds, canonical varints, exact wire consumption and padding checks.

This is delta-of-delta/first-difference prediction plus bitpacking; it has extensive prior art (Sprintz, FastLanes, FOR/PFOR, Gorilla-style encodings). No mechanism-level novelty inference is permitted.

## 2. Frozen population and hypotheses

Exactly these seven prior-consumed discovery inputs in fixed order:

1. `tests/corpus/synth-arith.bin`
2. `tests/corpus/synth-timeseries.bin`
3. `tests/corpus/synth-jitter.bin`
4. `tests/corpus/random.bin`
5. `tests/corpus/generated.repeat.jsonl`
6. `tests/corpus/generated.jsonl`
7. `tests/corpus/src.cpp`

New I14 and frozen I12 must compile and benchmark **on the same GitHub runner** against the same pinned Brotli build. Source blobs, input Git blobs and SHA-256 digests must be asserted before compilation. Full exact roundtrip and malformed-wire selftests precede measurements. Timing must consume every reconstructed byte via identical digest logic. Artifacts uploaded on failure.

- **H1:** at least one I14 DIFF block is selected on `synth-arith.bin` with an exact roundtrip.
- **H2:** the full AVI4 file is **strictly smaller than same-job I12 AVI2** on `synth-arith.bin`. Theoretical upper bound I14 <= I12 full bytes holds for every discovery input (identical block boundary and all I12 options preserved), and is checked.
- **H3:** complete AVI4 bytes < matched Brotli q11 bytes on arithmetic; I12's established size success alone must not be rebranded I14 efficacy.
- **H4:** I14 decode-plus-digest is faster than matched Brotli q11 on arithmetic in the same job, and compare I14 against I12. Same-job performance remains discovery only, not a definitive hardware-adjusted throughput conclusion.
- **H5:** high-entropy/text negative controls remain reported; do not hide cases where non-numeric sources are far worse than Brotli q5/q11.
- **H6:** exact roundtrip across 7 fixtures, synthetic near-constant local steps, modular wrap, malformed and truncated wire; binary identity and reference verification.

## 3. Frozen adjudication

Any source mismatch, workflow failure, codec failure, invalid wire accepted or incomplete evidence => **INVALID** (no performance ruling). H1 false => **KILL-DIFF**. H1 true/H2 false => **NO-INCREMENTAL-BYTE-WIN**, stop this geometry and inspect bitwidth tails, do not silently tune. H1/H2 true but H3 or H4 false => **SPECIALIZED-TRADEOFF**, measure first and do not promote. All H1-H4 true => **SPECIALIZED-DISCOVERY-ADVANCE** only. No held-out opening, no claim about general corpus, no production adoption without fair window/block comparison, source-independent real numeric data, RSS and binary cost, and independent typed codec references.

## 4. Competing next branches, not automatically approved by this result

A. I13 strided/field extraction addresses mixed-structure binary records that whole-block I12/I14 cannot model.

B. Tail-robust local-difference residuals (exception maps, patched frame-of-reference) if I14 1-outlier bitwidth inflation blocks selection. Charge complete descriptor/bitmap/payload costs.

C. Chunked SIMD prefix reconstruction to remove serial decoder dependence, only after a supported decode bottleneck is measured.

D. Explicit per-file/per-block fallback to high-throughput LZ/entropy baseline to avoid the RAW-only failures of arithmetic-only codecs. Full overhead and throughput must be charged.

E. General-purpose BWT decode/memory engineering, Q2 fair geometry attribution and Gate-B independent corpus remain separate research lanes. A specialized numeric advance does **not** change ANVIL's Class-A 435 dominated, 28 degenerate, 5 unresolved, zero verified crossings.

## 5. Work hygiene

I14 exists in a separate Git worktree; the original 427-entry working tree is untouched. Do not merge into canonical ANVIL until independent, meaningful evidence supports integration. Follow documented predeclared decisions even if the result is negative.

## 6. Post-freeze incident and narrowly scoped selftest correction (2026-10-07)

The **original frozen source remains preserved by git blob** `e94f31ea8ce0634b62010e7006c62479b85c03dd` and SHA-256 `7d2305f857d81887d9cfa155b04763c2c9c620b47a4def916e66e67090158346`. Initial [GitHub Actions run 37699173826](https://github.com/thelabcorner/anvil/actions/runs/37699173826) passed source/corpus identity, pinned Brotli build, compilation and all seven *I12* roundtrips. The I14 selftest stopped with `I12_FAIL I14 local difference mode not selected` **before I14 corpus roundtrips and benchmarking**. This is **INVALID-INFRA**, not evidence for/against I14 performance; artifact `11517435519` retains the failed job's provenance.

The synthetic `walking` selftest inadvertently used a short periodic step perturbation. Such periodic increments may leave the error relative to an affine line bounded, making the existing AFFINE candidate just as inexpensive as DIFF. It is not a discriminating test of accumulating innovations. Only the deterministic selftest *input construction* was amended to use an explicitly seeded xorshift pseudo-random sequence of bounded differences in `{-1,0,+1}`. No codec encode/decode logic, objective, frozen population, thresholds, hypotheses, decision rules, negative controls or benchmarking were changed. This is a **source-revision correction** and must never be concealed as a frozen-source reproduction.

Corrected source SHA-256: `f4f75b098feee9753084fa5cc132e31064f107fbd8ffa621f5e415b57d825150`; corrected Git blob: `365348ac5b8295f32eed97fb9427bb5516c43b16`. GitHub Actions must attest this revised identity before testing. If the revised run still fails, report it rather than altering the predeclared efficacy gates.
