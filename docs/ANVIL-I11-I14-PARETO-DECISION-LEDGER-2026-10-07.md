# ANVIL — I11–I14 Pareto Research Decision Ledger

**Reconciled:** 2026-10-07. **Authority:** docs/swarm-2026-10-02/FROZEN-CLOSEOUT-MATRIX-R2.md.
**Execution:** manual GitHub Actions for CPU-intensive tasks, no delegates, no swarms.
**Evidence:** discovery only; no held-out promotion, general-purpose crossing, or mechanism novelty.

## Measurement ledger

The global certified ANVIL Class-A state remains **435 DOMINATED / 28 DEGENERATE / 5 FRONT-GAP / 0 FRONT-CROSSING (468 total)**. New AVI1–AVI3 research does not change that grid.

| Experiment / file | AVI complete B | Brotli q11 B | Within-run decode MB/s (AVI vs Brotli) | Mode selection / outcome |
|---|---:|---:|---|---|
| I11 R1 arithmetic | 256,196 | 87,013 | VOID — eliminated output | 0 AFF, 0 PATCH; model rejected |
| I11 R2 arithmetic | 256,196 | 87,013 | 3,353.6 vs 172.8, RAW-only | 0 AFF, 0 PATCH; model rejected |
| I12 arithmetic | **62,458** | 87,013 | **358.872 vs 141.060** | 63/63 dense innovation; specialized discovery |
| I13 arithmetic | **62,256** | 87,013 | **540.728 vs 223.739** | 58 whole-model + 5 COL, 25 modeled lanes |
| I13 time-series | 139,387 | **134,718** | **598.592 vs 189.402** | 69 COL, 407 modeled lanes; 4,669-byte deficit |
| I13 repeat JSONL | 916,835 | **160** | 511.432 vs 633.218 | 229 COL, 916 modeled lanes; fails against LZ |

[Confirmed I11 R2](https://github.com/thelabcorner/anvil/actions/runs/37696816081), [I12](https://github.com/thelabcorner/anvil/actions/runs/37697658306), [I13](https://github.com/thelabcorner/anvil/actions/runs/37698492483); detailed retained results at docs/I12-BOUNDED-INNOVATION-RESULTS-2026-10-07.md and docs/I13-STRIDED-FIELDS-RESULTS-2026-10-07.md.

## Causal discoveries

**I11 failed for a reason:** Exact affine-plus-sparse-replacement pays an entire word and index for exceptions and rejects widely jittered sequences. R2's modal slope/intercept did not enable actual arithmetic-model selection on the reference discovery cohort; fast RAW-only output is not an optimization.

**I12 succeeded on a narrow, genuine mechanism:** Model x_i = (a+i*d+r_i) mod 2^w, use ZigZag signed innovations and fixed-bit packing. 63 modeled blocks encoded 256,000 existing synthetic source bytes into 62,458 complete wire bytes, versus 87,013 B at fixed Brotli q11. Paired digest-observed decode was about 2.54x in that one run. The mechanism is occupied FOR/delta/ZigZag prior art. It is specialized and its encode speed did not clear a broad objective.

**I13 showed field locality matters:** AVI3 tests 32-bit little-endian interleaved record strides 2–12 with independent raw or predicted lanes. All 69 existing synthetic time-series blocks selected COL; 407 modeled lanes demonstrate recoverable structure. Bytes were 139,387 versus q11 134,718, a 4,669-B or 3.466%-of-reference deficit. Decode was about 3.16x faster in the matched I13 runner, with actual modeled output. Its 69 block results cannot be compared directly to a fictional q11 per-block number because Brotli uses whole-stream dependencies.

### Critical 4,669-byte budget

Each modeled lane currently costs a 1-byte tag + 4-byte base + 4-byte step + 1-byte width **before residuals**. Thus 407 x 10 = 4,070 B of modeled-lane headers. Even hypothetically deleting **all 9 non-tag bytes** per modeled lane saves at most 407 x 9 = **3,663 B**, still **1,006 B short** of the observed Brotli comparison. This hypothetical removal is generally invalid: necessary decoder-visible state has to come from somewhere. Other charged costs include group headers, raw lane bodies, bit padding and partial-record tails. Therefore header packing alone cannot establish a crossing.

## Next research, ranked by falsifiability and probable value

1. **I14 exact wire anatomy (now preregistered):** independently parse actual AVI3 bytes on GitHub Actions; sum every serialized part; determine whether per-column residual width microtiles have a positive exact byte budget. No change to compression format or performance claim.
2. **I15 microtile bit widths if I14 capacity passes:** widths of 16/32/64/128 values per modeled lane, full 1-byte tile width, alignment padding and decoder checks. Only implement after exact cost evidence suggests a real prize.
3. **State reuse with stable record coordinates:** amortize repeated column step and intercept only when decoder can provably derive them from earlier decoded state, and charge cache, reset and window dependence. Current 4096-byte blocks may rotate field alignments across records; preserve exact source order.
4. **Fallback LZ for general-purpose compression:** repeated JSONL's 916,835 vs 160 B shows numeric-only transformations will never dominate a strong LZ reference on that family. Fast LZ + tables/entropy and low-overhead transforms is mandatory for ANVIL's overarching frontier.
5. **Independent real-data qualification:** secure genuinely new legally/provenance-locked structured, numeric and executable held-out families; complete c9 Graph Gate B; fair Q2 block/window geometry; matched typed baselines and multi-run encode/decode/RSS/binary footprint.
6. **Novel mechanism program:** novelty requires a genuinely new structural separator against ALP, PFor, Gorilla/Chimp, FastLanes, middle-out, existing transform graphs and bounded program-synthesis techniques. Neither striding nor parameter optimization is novel.

## Mathematical gate

For an exact candidate E and residual R, require L_complete(E,R) < L_same-geometry-baseline before analyzing speed, with decoder cycles/output byte, peak RSS, encoder cost, binary size and startup as independent objectives. A Shannon entropy bound is not an exact output-size comparator for Brotli/LZ, and no purported "free" predictor parameters may be omitted unless decoder knowledge makes them reproducible. Complete wire cost and role-locked corpus evaluation are mandatory.

## Supporting reference controls

- https://github.com/cwida/ALP (ALP/FastLanes typed compression, SIGMOD 2024)
- https://github.com/brettwooldridge/TurboPFor (PForDelta, fixed-bit microtiles, FOR/delta, SIMD)
- https://github.com/schizofreny/middle-out (SIMD parallel XOR reconstruction, not our measured baseline)
- https://github.com/google/brotli (actual q5/q11 matched-control source commit ed738e842d2fbdf2d6459e39267a633c4a9b2f5d)

**Evidence firewall:** All AVI-family trials are discovery only on previously consumed files, and their windows differ from Brotli lgwin22. Existing ANVIL Class-A grid remains unaltered. No user workstation CPU-heavy work.
