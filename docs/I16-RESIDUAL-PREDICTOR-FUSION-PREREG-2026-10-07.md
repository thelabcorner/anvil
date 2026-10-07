# ANVIL I16 — Paired Residual-Predictor Fusion Preregistration

**Date:** 2026-10-07. **Status:** frozen before any I16 compilation/selftest/corpus run. **Class:** old seven-file consumed synthetic discovery only; **not** a general-purpose Pareto crossing or mechanism novelty claim.

## Source and comparator attestation

- New `prototypes/i16-fusion/i16_fusion.cpp` SHA-256 `b214dcff0355293dd588bd762c7328dfce6e114749669f2b7a32d8c052f51dc3`, Git blob `9ef1eefee43a46a4fde0588df61e94770c7ad16f`.
- Retained I15 field-local control `prototypes/i15-field-diff/i15_field_diff.cpp` SHA-256 `ce9ff217ca32cae761362e11261ee9fdacaca7b27dcb86d9b2d41fb6a73ff612`, Git blob `fa60ec8fdab9931b5befae6e6575de8eb5ff0321`; anchor commit `e5c50db8334c3bb347d8c9c362ba4dedd9ba80da`.
- Independent I15-T control at fixed commit `dee232445fe303dcf555a37dd769cf9fca616a2c`, `prototypes/i15-tiled/i15_tiled.cpp` SHA-256 `2b2a891af077d51f53d26b22c05098e59a7ea2530015de75fcd9fb16d4afb4c7`, Git blob `b63136b8d4cc38bd5deca0bbdca1e5c396d36e15`. Fetch this exact commit read-only on the Actions runner and verify before compilation; do not graft it into the working tree.
- Brotli q5 and q11, `lgwin22`, Git source `ed738e842d2fbdf2d6459e39267a633c4a9b2f5d`, statically linked. Same Ubuntu 24.04 runner, GCC C++20 `-O3 -DNDEBUG` for all pilots.
- User workstation: file editing, byte hashes and Git operations only. All CPU-intensive compilation, selftests, fuzz, timing, corpus scans and reference comparisons on manually dispatched GitHub Actions. No agents, delegates or swarms.
- All tests and measurements in the seven frozen consumed order: `synth-arith.bin, synth-timeseries.bin, synth-jitter.bin, random.bin, generated.repeat.jsonl, generated.jsonl, src.cpp`.

## Scientific question, competing explanation

I15 field-local DIFF reduced a synthetic 280000-byte time series to **128560 B**, below Brotli q11 **134718 B** but above q5 **120594 B**. It used 336 flat modular difference lanes. Independently, I15-T 16-value affine microtiles produced **131384 B** with 299 tiled affine lanes, still above q5. **The gains cannot be added**. We are testing whether local-difference residual *outliers* benefit from adaptive bit widths while allowing the encoder to choose flat/global/tiled/local models on the exact same field.

I16 copies the frozen I15 AVI5 pilot into isolated **AVI6** grammar and preserves every I15 candidate (block RAW tag0, whole affine tag3, COL tag4, whole local DIFF tag5; COL RAW0, affine1, local DIFF2). Two COL submodes are added:

1. `COL mode3`: affine tiled; 1-byte tag, 4-byte LE base, 4-byte LE step, consecutive fixed 16-value groups (last partial), each 1-byte width `k_t<32` plus `ceil(n_t*k_t/8)` packed ZigZag modular innovations with zero padding. Identical serialized semantics to I15-T lane2 except tag value.
2. `COL mode4`: local modular DIFF tiled; 1-byte tag, 4-byte LE first, 4-byte LE step, consecutive fixed 16-value groups over **n-1** local innovations, each its own 1-byte width and byte-padding. Decode first state, then reconstruct serially `x_i = (x_(i-1)+step+unzig(e_i)) mod 2^32` across tile boundaries. No extra restart state is silently omitted.

The decoder rejects widths ≥32, insufficient bytes, nonzero padding, unknown tags, noncanonical varints, extra trailing bytes, output over cap 256 MiB, and unrepresentable lengths. Only strictly shorter **fully serialized** candidates replace the previous choice; ties preserve old choices. No per-tile residual vector allocations during decode. Block window remains 4096 bytes, stride catalog 2..12 32-bit lanes, raw-gap reconstruction unchanged.

## Frozen predicates

- **H0 correctness/source:** immutable blobs and raw hashes, tracked original corpus and Brotli identity, reproducible static build, all 3 C++ selftests, **21 full file roundtrips**, malformed-wire tests (zero-width/tile bit padding/truncated/word-wrap, selected DIFF-tile inverse), paired bench records, always-upload evidence. Any failure **INVALID-INFRA**, no efficacy conclusion.
- **H1 actual new mechanism selection:** at least one **local-DIFF tiled** lane on existing consumed time-series; separately count tiled AFF lanes. A synthetic constructed selftest alone does not pass H1.
- **H2 incremental exact-size improvement:** I16 time-series wire bytes **strictly less** than same-run I15 flat DIFF **and** same-run I15-T affine tiles. A choice present but not selected is not improvement.
- **H2q11 and H2q5 independently:** I16 complete time-series bytes **strictly below** same-run Brotli q11 and q5 respectively. q5 size gate is the stronger target; do not raise thresholds post hoc. Historical deficits as directional motivation only.
- **H3 real decoding:** I16 actual decoded-byte-digest-throughput > both same-run Brotli q5 and q11 on the **DIFF-tiled-bearing time-series**, report I15 and I15-T separately; no cross-run speed ranking.
- **H4 exact nonregression:** for **every one** of seven original inputs I16 serialized bytes <= both same-run I15 and I15-T. I16 retains their candidate mode semantics; failure indicates construction/cost-selection error and invalidates efficacy gate.
- **H5 work/baselines:** record I16 encode throughput, control encodes, q5/q11 encodes, all output archive sizes, selected lane and block tags, binary sizes and paired digest-inclusive decode rates for the full seven cases. A q5 size/decode crossing accompanied by a 5x worse encoder remains a tradeoff, not all-objective dominance.

Decision precedence after correctness: H1 fails => `NO-DIFF-TILE-SELECTION`; H1 holds but H2 fails => `SELECTED-NO-INCREMENT`; H2 holds but q11 loses => `INCREMENT-NO-Q11-CROSSING`; q11 wins but q5 loses => `SPECIALIZED-Q11-ONLY`; q5 wins but H3 fails => `SPECIALIZED-Q5-RATIO-ONLY`; H1/H2/q5/H3 succeed => `SPECIALIZED-Q5-DECODER-DISCOVERY`. Regardless: consumed discovery, **zero Class-A global-frontier claims**, no prior-art novelty.

## Boundaries and branching afterward

If no DIFF tile is selected, stop this exact configuration; never retune tile sizes after seeing old discovery data without new preregistration. If q11 but not q5 wins, the lower bound to q5 is the actual measured gap (not hypothetical savings from other pilots). For new experiments, separately freeze byte-level record framing (14-byte offset/23-byte), PFOR exceptions, strong LZ carrier, and/or decoder checkpoint/hardware throughput. Any credible general-purpose promotion requires independent real-origin heldout, fair block/window, memory+binary+encode/decoder metrics and an explicit matched typed-codec comparison (Sprintz, FastLanes/ALP, PFOR, Gorilla/Chimp, and middle-out where applicable).

Historical ANVIL frozen state **435 dominated / 28 degenerate / 5 unresolved / 0 verified complete crossings** cannot be overwritten by I16.

## Premeasurement infrastructure correction (2026-10-07)

First CI attempt [37701700846](https://github.com/thelabcorner/anvil/actions/runs/37701700846), source commit `d2fa7e51d7aad06a191a85ca31f26076d30def0e`, passed checked-out source/corpus hashes and verified the independently fetched I15-T source (`sha256sum: OK`), but then exited before building or benchmarking. The second precompile check incorrectly required `git status --porcelain` to be empty **after** the previous step intentionally created untracked `i16-results/provenance.txt`. This was a false positive in the CI hygiene test, not a codec invalidation or effectiveness result. Correct the second check to require unchanged **tracked** working/index trees with `git diff --quiet` and `git diff --cached --quiet`; preserve first source-attestation exact blob and SHA checks, reference pins, all frozen gates, fixture set and source without changes. Keep this failed run in the historical record as `INVALID-INFRA`.
