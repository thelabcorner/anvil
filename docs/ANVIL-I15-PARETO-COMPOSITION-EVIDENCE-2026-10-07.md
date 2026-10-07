# ANVIL — Source-Pinned I12–I15 Findings and Conditional I16 Research Decision

**Date:** 2026-10-07. **Role:** research closeout/forward-seed, not class-A promotion or a replacement for frozen original experiment documents.  
**Compute policy:** No agents, delegates or swarms. All CPU-intensive compilation, tests, synthetic generation, benchmarking, and corpus scans on GitHub Actions.  
**Checkout protection:** The original `ANVIL/` tree remains intentionally dirty and must not be reset/stashed. All current changes are isolated by research branch.

## 1. Evidential status and non-negotiable classification

The frozen historical frontier audit remains **435 dominated + 28 degenerate + 5 unresolved, zero verified general-purpose Pareto crossings**. I12–I15 use seven repeatedly consumed discovery sources, unmatched 4096-byte vs Brotli 2^22 window geometry, and scout-level within-run timing, so they do **not** update the class-A frontier grid.

- I11 R1/R2: exact affine/sparse repairs selected no useful arithmetic model on consumed discovery fixture; reject the representation rather than moving a threshold.
- I12 [run 37697658306](https://github.com/thelabcorner/anvil/actions/runs/37697658306): 256,000-byte arithmetic => AVI2 **62,458 B** vs Brotli q11 **87,013 B**, 63/63 modeled; 358.872 vs 141.060 MB/s digest-inclusive decode on its paired runner.
- I13 [run 37698492483](https://github.com/thelabcorner/anvil/actions/runs/37698492483): 280,000-byte 14-byte-record synthetic time series => AVI3 **139,387 B** vs Brotli q11 **134,718 B**. All 69 blocks chose COL; 407 modeled columns. Representation recovered structure but did not cross q11.
- I14 [run 37699578503](https://github.com/thelabcorner/anvil/actions/runs/37699578503): arithmetic => AVI4 **33,196 B** vs I12 **62,458 B** vs Brotli q11 **87,013 B**. 63 local-DIFF blocks, 361.970 vs q11 141.741 MB/s decode+digest. Decode near-tied with I12 within this single run; avoid claiming throughput dominance.
- I15 field-local [run 37700673925](https://github.com/thelabcorner/anvil/actions/runs/37700673925), HEAD `e5c50db8334c3bb347d8c9c362ba4dedd9ba80da`: paired runner selftests, source checks, seven exact roundtrips and benchmark steps passed. On time series I15 AVI5 **128,560 B**; I13 **139,387 B**; Brotli q11 **134,718 B**; Brotli q5 **120,594 B**. Selected **336 column-DIFF modes** in 69 COL blocks, all 407 modeled columns retained. I15 decode+digest 390.091 MB/s vs Brotli q11 131.986 and q5 169.741; encode 5.187 MB/s vs q11 0.364 and q5 29.385. Classify **SPECIALIZED-STRIDED-DISCOVERY-ADVANCE** (CI judge), **NOT class-A crossing**.
- I15-T [run 37700852636](https://github.com/thelabcorner/anvil/actions/runs/37700852636), commit `dee232445fe303dcf555a37dd769cf9fca616a2c`, [artifact 11517468246](https://github.com/thelabcorner/anvil/actions/runs/37700852636/artifacts/11517468246): separately pre-registered sixteen-value adaptive **affine** residual tiles. Source SHA-256 `2b2a891af077d51f53d26b22c05098e59a7ea2530015de75fcd9fb16d4afb4c7`. All source checks, I13/I15-T selftests, fourteen seven-file roundtrips, paired benchmarks and artifact upload passed. On the synthetic time series: **131,384 B** (8,003 below I13), **299 tiled lanes**, **421.491 MB/s** digest-observed decode vs q5 **176.382 MB/s** and q11 **136.134 MB/s**. Encode **6.881 MB/s** vs q5 **26.043 MB/s** and q11 **0.367 MB/s**. Brotli q11 **134,718 B**, q5 **120,594 B**; frozen result `SPECIALIZED-Q11-TRADEOFF-DISCOVERY`. The independently measured I15 field-local **128,560 B** remains smaller than I15-T by **2,824 B**, but its speeds are from a separate runner and must not be ranked directly. Savings from different predictors are **not additive** unless shown by a new actual combined wire.

### Falsifiable q5 byte gap

I15 field-local has to save **another 7,966 complete bytes** on the 280,000-byte time-series file to beat Brotli q5 (about **0.228 bits per original byte**). That is the *size* constraint alone. q5 encodes approximately 5.66x faster than I15 in that paired run; any full encode/decode Pareto claim also has to address encoder work or explicitly occupy a faster-decode/slower-encode regime. Against q11 alone I15 wins ratio, encode and decode on this consumed fixture, but all-purpose ANVIL remains dramatically worse on the text/JSONL negatives.

### Binding negative controls (I15)

| Discovery input | AVI5 complete bytes | Brotli q5 bytes | Brotli q11 bytes | Result |
|---|---:|---:|---:|---|
| synth-arith.bin | 33,196 | 115,107 | 87,013 | specialized arithmetic success |
| synth-timeseries.bin | 128,560 | 120,594 | 134,718 | q11 ratio win; q5 size/encode deficit |
| synth-jitter.bin | 975,641 | 86,663 | 62,717 | catastrophic relative to strong LZ |
| random.bin | 262,343 | 262,149 | 262,149 | framing disadvantage |
| generated.repeat.jsonl | 916,835 | 178 | 160 | numeric-only approach fundamentally insufficient |
| generated.jsonl | 2,797,603 | 207,699 | 150,415 | strong repetition carrier mandatory |
| src.cpp | 32,510 | 8,626 | 7,991 | generic source-code compression not competitive |

Source role is consumed discovery. No held-out, independently sourced numeric qualification exists for this line.

## 2. Mechanism boundaries

For fixed-width integers in `Z/(2^w)`, the full-block/global affine representation is
`x_i = (a + i*d + e_i) mod 2^w`. Its absolute residuals accumulate the error of a random walk. I14/I15 instead use local modular differences `e_i = (x_i - x_(i-1) - d) mod 2^w`, with ZigZag/bitpacked signed representatives and exact sequential prefix reconstruction. I13's byte-exact field scatter and retained raw lanes permit such reconstruction within interleaved records. All these are old predictive/bitpacking families (e.g., FOR/PFOR, Sprintz, FastLanes); no novel mechanism claimed.

I15-T analyzes a **different** axis: with fixed 16-value tiles, use tile-specific bit width `k_t = bitLength(max ZigZag residual in tile)`, each width costs one full byte and each tile is byte-padded. Its gain depends on spatial concentration of outliers. Flat-vs-tiled-vs-raw choice is by *actually serialized* byte count; the decoder uses bounded indexed tiles. It must be judged against its own frozen I13 control, never substituted into I15 fields without new preregistration and code.

## 3. Next branch: independent I16 combination (conditional, not pre-approved)

**Hypothesis:** independently selecting a per-field local-DIFF-flat lane and a per-field local-DIFF-tiled lane, while preserving existing I15 and I15-T candidates, can lower full-wire bytes beyond either effect alone. This must not be represented as simple summation of savings because high-width bursts and monotonic drift are correlated.

Freeze: canonical source, mixed mode grammar and tags, exact tile width catalog (start fixed 16), tie-breaking and descriptor bytes; source/fixture SHA and all baselines, field-selected counts, output padding/remainder handling, and complete-file fallback policy **before any run**. In an isolated new worktree, implement both competing predictors over the same reconstructed field values, select shortest actual bytes, enforce exact inverse, and include malformed tile and wraparound adversarial tests. All CPU-intensive trials on Actions. H1: combination mode actually selected on numeric sample. H2: complete time-series bytes below same-run I15 field-local. H3: strictly below same-run q5, separately q11. H4: modeled decode > q5 and q11 on same run; encoder work reported and never ignored. H5: all seven role-locked discovery negatives reported. H6: no regressions/roundtrip failures. Any q5 size win is still **specialized discovery**, not general frontier.

### Competing branch A — record-native byte stride

If I15-T or combination stalls, test explicit bounded **14-byte/23-byte** record geometries with width-1/2/4 fields and exact uncovered raw bytes. Use an encoder-only bounded stride/offset search; descriptors include field position/width/record count, no overlapping fields, no endian ambiguity, no hidden per-file hints. Compare against stride7 32-bit-word aliasing and byte-shuffle + identical carrier, otherwise gains may merely expose known transposition.

### Competing branch B — entropy of unmodeled columns

I13's frozen wire audit totals 44,008 B in literal raw columns, 90,390 B in packed residuals, with 4,989 B of other headers/tails/frame. Determine on Actions whether raw lanes are near incompressible (do not optimize them blindly) and whether residual bitwidth outliers cause persistent padding waste. **Exact serialized attribution** must precede adding PFOR exceptions, shared strides, or transforms. Record 99th-percentile bit widths and exception metadata, not only mean entropy.

### Competing branch C — true general-purpose fallback

A production candidate must carry a strong whole-file Brotli/Zstd/LZ codec alongside specialized paths; the existing repeated JSONL result is orders of magnitude behind. A per-file fallback incurs header and encode/decode costs and cannot, by itself, have smaller bytes than the selected unchanged fallback. To beat the reference, it needs an independently profitable transform or competitive native backend, with complete framing, memory and work accounting. Strong baselines include q5/q11, representative zstd/LZ4HC, and typed Sprintz/FastLanes/ALP on schema-matched numeric cohorts.

### Competing branch D — decoder critical path

Serial local recurrence creates a dependency chain. Test **fixed-size prefix checkpoints** or interleaved lane scans only when data already selects the corresponding predictor. Charge every restart base and actual scratch/RSS cost. A theoretical SIMD win on RAW-only files is a falsification, not a speedup.

## 4. Promotion firewall / next checkpoint

1. Preserve both I15 experiments as separate reproducible references. Record all invalid/premeasurement run histories.
2. I15-T actually saves **8,003 B** from I13, surpassing its prior **7,704 B** theoretical estimate by **299 B** because the earlier wire-anatomy projection intentionally excluded one saved global-width byte per winning tiled lane. Its complete q11 size crossing and q5 failure are confirmed. Preserve source-pinned run and artifact unchanged.
3. Select exactly one new independent I16 pilot with predeclared go/no-go byte/throughput/encoder gates and a falsification fixture.
4. For class-A promotion, freeze **independent real-source** numeric, JSONL, executable and heterogeneous families *by origin*, alongside legal provenance and dedup. Enforce Q2 matched windows/blocks, multiple paired timing repeats, peak RSS, binary footprint, encode budget and statistical intervals. Existing 468-cell frontier cannot be overwritten by synthetic wins.
5. Stop novelty claims unless a mechanism-specific prior-art adjudication survives Sprintz, FastLanes, ALP, PFOR, Gorilla/Chimp, middle-out, OpenZL transform graphs and bounded reconstruction work.

**Conclusion:** The high-probability near-term ratio target is not another general arithmetic model; it is fixing the observed **7,966-byte q5 gap** on a type-aware representation while preserving fast reconstructability. The high-value long-term target is a constrained, heterogeneous engine that never substitutes numeric models for a proper LZ fallback and earns a *verified multi-objective* frontier point on independent data.
