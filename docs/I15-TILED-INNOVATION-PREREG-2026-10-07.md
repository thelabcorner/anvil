# I15 — Fixed 16-Value Adaptive Packed Innovations (AVI4) Preregistration

**Date:** 2026-10-07; frozen before initial AVI4 compile/codec run.
**Pinned pilot identity:** `prototypes/i15-tiled/i15_tiled.cpp`, raw SHA-256 `2b2a891af077d51f53d26b22c05098e59a7ea2530015de75fcd9fb16d4afb4c7`, Git blob `b63136b8d4cc38bd5deca0bbdca1e5c396d36e15`; matched control I13 raw SHA-256 `274be4c8a5de222b4bbb16c586fc04310dd1deadd455dacc7741110b8dfcb7c0`, blob `ca3f9e684917b276f6a4274de62c3b20ca73fa95`. Pilot workflow `.github/workflows/anvil-i15-tiled.yml` and decision script `tools/i15_tiled_decide.py` are source-locked to this identity. No thresholds or discovery members may change after initial dispatch. This I15-T experiment is distinct from the separately frozen I15 field-local difference branch.
**Role:** isolated discovery proof-of-implementation, not production ANVIL format or novel mechanism/promotion/whole Pareto crossing.
**Prerequisite evidence:** I13 [run 37698492483](https://github.com/thelabcorner/anvil/actions/runs/37698492483); I14 [run 37699298906](https://github.com/thelabcorner/anvil/actions/runs/37699298906). Frozen I14 positive-only 16-value microtile capacity 7,704 B after width metadata/padding on consumed synthetic time-series.
**Compute:** all C++ compilation, selftest, corpus encode/decode, timed benchmarks and sweeps on GitHub Actions only. No delegates/swarm agents.

## Exact mechanism / no post-hoc retuning

Copy frozen AVI3 research pilot into a new AVI4 executable, preserve RAW tag 0, full-block affine tag 3, and COL group tag 4 with all existing semantics except COL lane tag **2** is now permitted in addition to 0 RAW and 1 flat affine.
- COL lane 2 = 1-byte lane tag, 4-byte base LE, 4-byte step LE, then for each successive fixed **16** input records (last may contain 1..15 values): 1 byte bitwidth k (0..31), followed by ceil(group_count*k/8) payload bytes containing low-bit-first ZigZag signed modular innovations relative to base + i*step mod 2^32.
- Group size **16**, globally fixed in the decoder, not transmitted and not selected after results. Use exactly the already selected median slope/intercept predictor; do not change field inference to help the first result.
- Per input column, compute RAW, flat (lane 1), and tiled16 (lane 2) as actual encoded Byte vectors, choose *strictly shortest* complete serialized candidate; ties favor older modes. Candidate stride 2..12 and other I13 whole-block choices retain same shortest serialized-byte selection. Malformed groups/widths, noncanonical varints, nonzero packed padding, trailing bytes and output amplification must fail closed.
- No ANN, learning, external dictionary, warmed state, parser search budget changes, new corpus data, or reference-dependent data. No direct dependency on other decoded blocks. Decode bound is O(output values) and scratch bounded to one output plus small accumulators (no per-tile vector allocation in the tiled lane decoder).
- Archive header magic AVI4 distinguishes bitstream; output size cap 256 MiB; exact wire charged.

## Populations and references

Same seven tracked, previously consumed discovery fixtures as I12/I13:
synth-arith.bin, synth-timeseries.bin, synth-jitter.bin, random.bin, generated.repeat.jsonl, generated.jsonl, src.cpp.

Pinned official Brotli v1.1.0 source commit ed738e842d2fbdf2d6459e39267a633c4a9b2f5d, statically linked, q5/q11 with lgwin22, on the same GitHub runner. Workflow must attest exact dispatch SHA, clean checkout, C++ SHA-256+Git blob, per-input Git blobs and SHA-256, executable/binary hashes and reference commit. Manual workflow_dispatch only; no secrets and read-only contents permission; always upload artifact. C++20 O3. Measure complete bytes, selected column-blocks, tiled16 lanes, flat lanes, raw lanes, encode MB/s, digest-observed decode MB/s and binary footprint. Bit-exact source roundtrip/cmp on every source. Safety fixtures include 0/1/15/16/17/31/32/33-value tiles, zero and max width, wrapping signed deltas, random data, malformed tag, truncated bit payload and padding.

## Frozen decision predicates

1. **Correctness/source failure** => INVALID-INFRA; no byte/performance claim, repair only with new source SHA and clearly retained failed run.
2. **H1 real selection:** >=1 tiled16 lane on synthetic time-series; otherwise KILL-TILED-MODE, no retrospective width-choice search.
3. **H2 q11 byte threshold:** complete AVI4 timeseries bytes < 134,718 B in this run. If not, NO-Q11-BYTE-WIN.
4. **H2b stronger q5 threshold:** complete AVI4 timeseries bytes < 120,594 B; report independently. A q11 byte win does NOT constitute q5 ratio victory.
5. **H3 throughput:** timed actual modeled AVI4 decode > within-run Brotli q5 AND q11, using identical per-byte digest consumption; separate from H2; raw-only speed ignored. No cross-run AVI3 speed claim.
6. **H4 size nonregression:** complete AVI4 arithmetic <=62,256 B and repeated JSONL <=916,835 B, since the prior exact candidate remains available. Record failures transparently, they are not automatically valid old-codec dominance comparisons (different magic/version).
7. **H5 execution:** record complete wire, actual selected tags, full-file roundtrips, speed and artifact on *all seven* files. Codec engineering and discovery only; Gate B still closed.
8. If H1+H2+H3 hold, label **SPECIALIZED-Q11-TRADEOFF-DISCOVERY**, NOT FRONT-CROSSING. If H2b also holds, **SPECIALIZED-Q5-BYTE-DISCOVERY**, still not full Class-A. No mechanism novelty.

## Future context

ALP/FastLanes, TurboPFor/PForDelta, SIMD-bitpacking, Gorilla, Chimp, middle-out and established adaptive group-width coders are close prior art. A broader ANVIL contribution demands independently held-out real sources, fair window geometry, matched modern typed and generic codec baselines, complete speed/RSS/binary/encode objective, and a genuinely distinct semantic separator if novelty is sought.
