# ANVIL I15 — Field-Local Modular Innovations: Frozen Discovery Closeout

**2026-10-07 • Source-attested matched remote discovery • SPECIALIZED-STRIDED-DISCOVERY-ADVANCE**

**Authoritative [GitHub Actions run 37700673925](https://github.com/thelabcorner/anvil/actions/runs/37700673925)**: workflow_dispatch, all steps successful, source SHA e5c50db8334c3bb347d8c9c362ba4dedd9ba80da. All compilation, selftests, 14 file roundtrips and all paired benchmarks ran exclusively on GitHub Actions. No delegates, swarms or local CPU-intensive experiments.

**Exact source pin:** I15 SHA-256 ce9ff217ca32cae761362e11261ee9fdacaca7b27dcb86d9b2d41fb6a73ff612, Git blob fa60ec8fdab9931b5befae6e6575de8eb5ff0321. Control I13 SHA-256 274be4c8a5de222b4bbb16c586fc04310dd1deadd455dacc7741110b8dfcb7c0, blob ca3f9e684917b276f6a4274de62c3b20ca73fa95. Brotli static upstream commit ed738e842d2fbdf2d6459e39267a633c4a9b2f5d, q5 and q11 with lgwin22. Ubuntu 24.04, GCC 13.3.0 and CMake 3.31.6.

**Evidence:** CI artifact includes provenance.txt, I13/I15 executable SHA+sizes, selftests, fourteen byte-identical file roundtrip confirmations, benchmark-13.tsv, benchmark-15.tsv, decision.json. All input Git blobs and SHA-256 were asserted. Timings include the same every-output-byte digest; these are not pure decode rates. Shared GitHub VM figures are exploratory, not statistically controlled confidence intervals.

## 1. Research progression and causal isolation

I11 sparse affine/replacement failed to select on the intended arithmetic source. I12 dense innovation bitpacking recovered all 63 arithmetic blocks: 62,458 B vs Brotli q11 87,013 B. I13's strided 32-bit fields identified 407 modeled columns in 69/69 time-series blocks, but complete AVI3 bytes were 139,387 versus Brotli q11 134,718 — 4,669 B worse. I14 introduced local modular differences and improved known arithmetic to 33,196 B, 46.85% smaller than I12.

I15 combined *I13 field isolation* with *I14 local predictor*. It preserves every I13 RAW/AFF/COL model as an exact serialized candidate and adds whole-block DIFF plus a field-local 32-bit DIFF submode. Critically, I15 did not expand the stride search, alter the seven external corpus files or add a new entropy backend: the observed time-series improvement therefore isolates the effect of the residual-representation change. On the previously consumed time-series source, it selected **336 local-DIFF lanes among 407 modeled columns in 69 COL blocks**.

## 2. Complete measurements, including adverse controls

| Discovery file | Raw B | I13 full AVI3 B | I15 full AVI5 B | Brotli q5 B | Brotli q11 B | I15 DIFF adoption | I15 decode+digest MB/s | Brotli q11 decode+digest MB/s |
|---|---:|---:|---:|---:|---:|---|---:|---:|
| synth-arith.bin | 256,000 | 62,256 | **33,196** | 115,107 | 87,013 | 63 global DIFF; 0 COL | 369.801 | 136.862 |
| synth-timeseries.bin | 280,000 | 139,387 | **128,560** | **120,594** | 134,718 | 336 field DIFF, 407 modeled lanes in 69 COL blocks | **390.091** | 131.986 |
| synth-jitter.bin | 974,920 | 975,641 | 975,641 | 86,663 | **62,717** | 0 DIFF | 614.050 | 368.131 |
| random.bin | 262,144 | 262,343 | 262,343 | 262,149 | 262,149 | 0 DIFF | 627.002 | 590.001 |
| generated.repeat.jsonl | 936,000 | 916,835 | 916,835 | 178 | **160** | 0 DIFF | 366.796 | 390.140 |
| generated.jsonl | 2,803,267 | 2,797,603 | 2,797,603 | 207,699 | **150,415** | 0 DIFF | 477.476 | 394.748 |
| src.cpp | 32,512 | 32,510 | 32,510 | 8,626 | **7,991** | 0 DIFF | 566.145 | 239.651 |

**Do not ignore negative controls.** I15 selects no new DIFF modes in six of seven files beyond arithmetic and timeseries. Its JSONL archives are orders of magnitude worse than Brotli, because AVI5 contains no true general-purpose LZ compression or cross-block repetition exploitation. A raw-dominated file's fast decoding is irrelevant to its very poor ratio.

## 3. Frozen gate rulings

- H1 field-local modeling **PASS**: 336 DIFF-mode columns actually selected on the time-series, not RAW-copy artifacts.
- H2a full bytes vs same-run I13 **PASS**: 128,560 < 139,387 B. Improvement 10,827 B or **7.77%** relative to I13.
- H2b full bytes vs same-run Brotli q11 **PASS**: 128,560 < 134,718 B. Improvement 6,158 B or **4.57%** relative to q11.
- H3 paired model-bearing decode **PASS**: 390.091 MB/s versus q11 131.986 MB/s, **2.96×** digest-inclusive rate.
- H4 seven-file nonregression **PASS**: AVI5 ≤ AVI3 on all seven because old candidates are retained, bytecharged and ties preserve old choices. Arithmetic AVI5 = I14 historic 33,196 B.
- H5 all negative controls **PASS**: no adverse family omitted.
- H6 safety, evidence, roundtrip **PASS**: 20-case I15 selftest including two whole-block DIFF and one column DIFF test; source/input hashes, static Brotli, 14 file-exact paired roundtrips and complete artifact.

**Frozen verdict: SPECIALIZED-STRIDED-DISCOVERY-ADVANCE. This is not a general-purpose ANVIL Pareto crossing, independent held-out generalization, a novel mathematical primitive or a production format promotion.**

## 4. The crucial Brotli q5 counterexample

On the known time-series, Brotli q5 produces **120,594 B**, smaller than I15 128,560 B by 7,966 B, while also encoding faster (29.385 versus I15 5.187 MB/s). Conversely I15's digest-inclusive decoding **390.091 MB/s** beats q5's 169.741 MB/s, a roughly **2.30×** advantage. Thus I15 does not dominate the Brotli family across compressed bytes, encode speed and decode speed. It is a tradeoff point that needs equal geometry and real-data confirmation.

On time-series I15 encode is **5.187 MB/s** versus I13 **6.862 MB/s** (~24.4% lower) because testing more predictor candidates costs encoder work. I15 versus I13 decode + digest **390.091 versus 374.042 MB/s** differs by ~4.3%, not a robust statistically significant finding in one virtualized session. On arithmetic I15 encode **6.628 MB/s**, I13 **9.502 MB/s**, while I15 size is much smaller. Any real engineering Pareto adjudication must include this encode penalty explicitly.

## 5. Mathematics and reproducibility

Operate in the ring Z/(2^b) for each integer field. Estimate median modular adjacent step d on encoder only. Global affine coding packs ZigZag(x_i − a − i·d); for noisy steps residual magnitude can accumulate with index. Local coding packs ZigZag(x_i − x_(i−1) − d) for i≥1. The decoder reconstructs each field sequentially by x_i = x_(i−1) + d + unzig(residual_i) modulo 2^b and scatters the exact original 32-bit field to its source offset. Each block still stores literal unmodeled lanes and trailing bytes, and complete tag/metadata/bit-width/padding costs are charged.

The mathematics has extensive prior art (delta-of-delta, FOR/PFOR, Sprintz, FastLanes, ZigZag, bitpacking). The demonstrated engineering composition does not substantiate a mechanism-level novelty or patent claim. Relevant research: [Sprintz](https://arxiv.org/abs/1808.02515), [FastLanes](https://github.com/spiraldb/fastlanes), [middle-out](https://github.com/schizofreny/middle-out).

## 6. Premeasurement failure lineage

Initial I15 [GitHub Actions 37700369378](https://github.com/thelabcorner/anvil/actions/runs/37700369378) passed source attestation, Brotli build, C++ compilation and all I13 controls; stopped before external I15 tests/benchmark because an artificially periodic *selftest* was represented more cheaply by the preexisting strided mode than whole-block DIFF. Its efficacy evidence is VOID. The two misleading test sequences were replaced with deterministic bounded nonperiodic noise; exact codec math, source format, external files, references and gates unchanged. Updated source hash/blob above. The corrected run 37700673925 passed all steps.

## 7. Priority to reach genuine Pareto membership

1. **P0: freeze evidence** and preserve the existing R2 gates and current dirty canonical checkout. Historic Class-A remains **435 dominated, 28 degenerate, 5 unresolved, zero verified general-purpose crossings**.
2. **P1: byte-accurate field framing**. The synthetic time-series stores 14-byte records of 64-bit timestamp, 32-bit float, 16-bit channel ID; I13/I15 still use 32-bit-word stride 2..12. A new preregistered I16 should test *bounded byte-stride and field-offset discovery*, include all opaque bytes and descriptor cost, and compare to byte-shuffle+Brotli. This must not be hardcoded to the known generator and relabeled general inference.
3. **P2: encoder cost**. Use a bounded cheap regularity sketch to prune improbable field strides/models before median fitting, and quantify exact size loss versus exhaustive candidates and total CPU saved.
4. **P3: patched exceptions / SIMD**. A single bitwidth outlier currently charges every sample. Study exact exception-location costs and FastLanes-like lane-transposed/checkpoint reconstruction, measuring wire bytes, speed, branch/cache/RSS and binary size separately.
5. **P4: general-purpose fallback**. AVI5 cannot ship as a universal compressor. Add a strong whole-file or block-level LZ/Zstd/Brotli fallback, paying for outer mode framing and encoding costs, to prevent the JSONL catastrophes. Match windows fairly.
6. **P5: independent real-world data and advanced typed controls**. Legally sourced sensor, telemetry, floating-point and machine log series with origin-level independence and discovery/heldout separation. Compare Gorilla/Chimp, Sprintz, FastLanes, ALP, byte/bitshuffle and contemporary Zstd/Brotli references. Run repeated hardware-controlled encode/decode tests with full wire, memory/RSS, binary size and confidence intervals.

No delegate/swarm involvement. No claim of general Pareto frontier advancement until these independently verified gates have passed.

## 8. Source navigation

- Frozen preregistration: docs/I15-FIELD-LOCAL-DIFF-PREREG-2026-10-07.md
- Experiment code: prototypes/i15-field-diff/i15_field_diff.cpp
- Reproducible source construction: tools/i15_assemble.py
- Preregistered evaluator: tools/i15_decide.py
- GitHub Action: .github/workflows/anvil-i15-field-diff.yml
- I12 closeout: docs/I12-BOUNDED-INNOVATION-RESULTS-2026-10-07.md
- I13 closeout: docs/I13-STRIDED-FIELDS-RESULTS-2026-10-07.md
- I14 closeout: docs/I14-LOCAL-DIFFERENTIAL-RESULTS-2026-10-07.md
- Complete next-experiment research map: docs/ANVIL-FRONTIER-COMPOSITION-RESEARCH-MAP-2026-10-07.md
