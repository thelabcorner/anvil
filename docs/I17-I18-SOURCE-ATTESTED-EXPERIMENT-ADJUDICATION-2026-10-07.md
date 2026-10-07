# ANVIL I17–I18 Research Evidence and Frontier Adjudication — October 7, 2026

**Evidence status:** SOURCE-ATTESTED GITHUB ACTIONS DISCOVERY on an already-consumed seven-file corpus. **No new independent held-out general-purpose frontier result.** No agents/delegates or local heavy experiments. All rates and bytes below from named remote artifact files, not inferred from workflow success alone.

## R2 control-plane gate correction — Gate A already PASS, Gate B still BLOCKED

The earlier October 7 audit's `PUBLICATION REQUIRED` status is superseded by subsequently verified source-of-record evidence: [Q1a-XRUN run **37694740386**](https://github.com/thelabcorner/anvil/actions/runs/37694740386), head commit `c93ce7b5ebff801bd0088b10a13a2cb962f7ebc3`, successfully completed **before** the I11–I18 run sequence analyzed here. Its GitHub Actions artifact `anvil-q1a-xrun-c93ce7b5ebff801bd0088b10a13a2cb962f7ebc3-run1`, ID **11514736936**, GitHub-reported SHA256 ZIP digest `46b56d44de2b50aad73beb6bb0fb8cc8bd14fdad8f6ec3439a54bebb779acd86`, contains `xrun-verdict.txt`, immutable source checks, and deterministic fixture outputs.

**Gate A = PASS**: artifact explicitly states `XRUN=PASS`, `source_dirty=false`, Q1a tool blob `0d2467c719daef37d301fa221fb8b4b3d069934b` (SHA256 `c0c027e15f2a572d2816396129a1d0ed950bdd86dbf84d3c3d1e6310ccb399a4`), emitter blob `07720d4fbba9e4f77143e873fb70d8a05154da43` (SHA256 `255ec7d56633bc4fa970024d446586cdd18afc3d8d7d5930257c85dc953dc3fb`), **135/135** Q1a tests, fixture manifest hash `feaa7848967d8245dc16f5d531fcc1e5c9f2898e941b6f5a424182301c4fec5c` and summary hash `112add224d0017e6bbb7fad083424d90c3818526416de546f3000e00216dcdaa`, both matching the frozen R2 publication manifest. Q1a emitted twice and produced the expected same result.

**Gate B = BLOCKED**: `CORPUS_BLOCKED`, `graph_verification=attestation_backed`, `independence_units_authoritative_for_promotion=false`, 7 synthetic fixture units, 30 conflicts, 113 not-evaluated relationships. Gate A authorizes separate preregistered source-attested **discovery/engineering** jobs; it does not authorize a Class-A frontier or generalization promotion. This resolves the earlier chronology error without altering a frozen performance threshold.

## Immutable source-of-record and failures

| Experiment | GitHub Actions run | Pinned SHA | Adjudication |
|---|---|---|---|
| I15-T tiled innovation control | [37700852636](https://github.com/thelabcorner/anvil/actions/runs/37700852636) | `dee232445fe303dcf555a37dd769cf9fca616a2c` | CI success; retrospective control only |
| I16 byte-aligned frames | [37701687296](https://github.com/thelabcorner/anvil/actions/runs/37701687296) | `8d020e4895c6d58646c925f3ae14e046758a207c` | **INVALID-PREMEASUREMENT:** `I16_FAIL bounded innovation was not selected`; CI stopped at candidate selftest before efficacy or timing |
| I16 residual predictor fusion | [37701888318](https://github.com/thelabcorner/anvil/actions/runs/37701888318) | `9ca860f45d7dad1b617267372e378f18a93f2721` | **SPECIALIZED-Q11-ONLY**: selected tiled DIFF and size smaller than both I15 controls and q11, but time-series size is 290 B **larger than q5** |
| I17 fast/full portfolio | [37701773624](https://github.com/thelabcorner/anvil/actions/runs/37701773624) | `a26d770c400cd05b26195180ee4da157a8ee9c73` | **FAST-ENCODE-TRADEOFF-DISCOVERY**: all six limited gates passed; 14 exact fast/full roundtrips |
| I18 initial integration | [37702906749](https://github.com/thelabcorner/anvil/actions/runs/37702906749) | `40cdb8412dc4ff86c6171583a9d26f7c97c96196` | **INVALID-PREMEASUREMENT:** source and Brotli attestations passed; including nested I17 `main` caused duplicate C++ entrypoints. Preserved failure; no codec measurements |
| I18 corrected fusion/fast integration | [37703141058](https://github.com/thelabcorner/anvil/actions/runs/37703141058) | `7ec50e8dadd1b3cc12d815c80007a9fcc2bfd490` | **BUDGETED-SPECIALIZED-DISCOVERY:** workflow, source/corpus hashes, compile, selftests, 21/21 exact roundtrips and all six preregistered quantitative discovery gates passed |

I18 source sha256 `e5fd1a2c44d697f7cfd49427e1ca3654b192cfff07168e67409007b6f97d65e2`. Its AVI6 source is an exact byte copy sha256 `b214dcff0355293dd588bd762c7328dfce6e114749669f2b7a32d8c052f51dc3`. The I17 included pilot is exact-source mirrored with a single entrypoint substitution sha256 `dbd0b11cac966a7c5aedf603c3ba7d30b8a4f63b039deb48cebbadbc37d02615`. Pinned Brotli source `ed738e842d2fbdf2d6459e39267a633c4a9b2f5d` (q5 and q11, lgwin22). Same-run I18 reference controls compiled under Ubuntu 24.04, GCC 13.3.0, C++20 `-O3 -DNDEBUG`.

**I18 artifact archive:** `anvil-i18-7ec50e8dadd1b3cc12d815c80007a9fcc2bfd490-run1`, artifact ID **11518358032**, reported ZIP digest **sha256:a800b6719f9da3c94bff5419d2adfe4bea0ff7ca3a398e25570718b16262a806**, on [run 37703141058](https://github.com/thelabcorner/anvil/actions/runs/37703141058). Immutable source-of-record files `benchmark.tsv`, `decision.json`, `provenance.txt`, `roundtrips.txt`, `selftest.txt`, `binary-sizes.txt`, `rss-*.txt`. A local download of this artifact resides at `i18-evidence-20261007/ci-r2/` (intentionally untracked to avoid committing generated binary payloads). GitHub job is source-of-record; the digest is **GitHub's reported metadata**, not a separate end-to-end local cryptographic verification. Do not use local download as substitute for attestation.

## I16 mathematical fusion: what actually improved

The I16 AVI6 reconstruction grammar combined original field-local prediction with fixed 16-value bit-width tiles. On the **280,000-byte consumed synthetic time-series** fixture:

| Codec (paired I16 run) | Archive bytes | Encode MB/s | Decode MB/s |
|---|---:|---:|---:|
| I15 flat field-local | 128,560 | 6.752 | 496.649 |
| I15-T affine tiled | 131,384 | 8.554 | 519.345 |
| **I16 residual fusion** | **120,884** | **6.556** | **507.937** |
| Brotli q5 | **120,594** | 37.024 | 228.779 |
| Brotli q11 | 134,718 | 0.428 | 161.170 |

I16 selected **117 DIFF-tiled lanes** and one affine-tiled lane (407 modeled lanes). It saved **7,676 bytes** vs I15 field-local; **10,500 bytes** vs I15-T; **13,834 bytes** vs Brotli q11, but still lost q5 by **290 bytes**. I16 arithmetic 256,000-byte source used 14 DIFF-tiled lanes, compressed to **19,437 bytes** vs 33,196 I15 and 87,013 q11. These are *specialized mechanism and selection evidence on consumed synthetic data*, not independent generality.

## I17 fast/full selection: real cost and regret

I17 fast selects the minimum of AVI4 and Brotli q5; full also evaluates Brotli q11. Both modes use AVH1 framing. On arithmetic, fast and full = **33,204 bytes**, encode **10.751** vs **0.516 MB/s**; on time-series, both **120,602 bytes**, encode **10.552** vs **0.379 MB/s**. Byte regret appears when q11 is smaller: jitter fast/full **86,671/62,725 B**, generated JSONL **207,708/150,424 B**, source C++ **8,634/7,999 B**. The fast profile's faster candidate work **does not** imply better ratio or a general Pareto crossing.

## I18: full paired results, not cross-run speeds

I18 uses **AVH2**; it is deliberately distinct from AVH1. Header is 4-byte magic, canonical uncompressed length and mode (total 8 B for these inputs). It uses q5 as baseline, RAW for incompressibility, skips numeric candidates when q5 compresses to <=12.5% of raw or bounded strided probe cannot find regular numeric differences; otherwise evaluates both AVI4 and AVI6 and selects the shortest complete bytes. q11 is **not** evaluated in I18 timed encoding (but q11 is measured separately as reference).

| Input | I18 budget B | I17 fast B | I17 full B | q5 B | q11 B | I18 encode MB/s | I18 decode MB/s |
|---|---:|---:|---:|---:|---:|---:|---:|
| Arithmetic | **19,445** | 33,204 | 33,204 | 115,107 | 87,013 | 4.089 | 381.422 |
| Time series | 120,602 | 120,602 | 120,602 | 120,594 | 134,718 | 3.647 | 175.439 |
| Jitter | 86,671 | 86,671 | 62,725 | 86,663 | 62,717 | 111.414 | 414.223 |
| Random | **262,152** | 262,157 | 262,157 | 262,149 | 262,149 | 245.025 | 634.056 |
| Repeated JSONL | 186 | 186 | 168 | 178 | 160 | 1,024.369 | 419.467 |
| Generated JSONL | 207,708 | 207,708 | 150,424 | 207,699 | 150,415 | 91.566 | 427.072 |
| Source code | 8,634 | 8,634 | 7,999 | 8,626 | 7,991 | 50.167 | 279.777 |
| **Aggregate sum of independent archives** | **705,398** | **719,162** | **637,279** | **801,016** | **705,163** | **NOT summed** | **NOT summed** |

Encoding on arithmetic is **4.089 MB/s** for I18 vs **11.473 I17 fast**, **0.482 I17 full**, **21.580 q5** in the *same I18 job*. Decoding arithmetic **381.422 MB/s** vs q5 **161.660** and q11 **141.568**. So the arithmetic compressed bytes improve by **13,759 B (41.44%)** versus I17 fast but encode throughput is about **2.81× slower** than that fast baseline (and **~8.48× faster** than I17 full). Full wire already counted.

Time-series I18 was **3.647 MB/s** encode vs I17 fast **11.246**, I17 full **0.353**, q5 **29.291**; extra numeric evaluations were wasted because q5 still won. This is the **primary quantified next encoder-optimization target**: skip uncompetitive numeric candidates sooner without losing arithmetic modeling.

Across all five negative controls (jitter, random, repeated JSONL, generated JSONL, source) **zero** mathematical codecs were invoked, as counted in the CI TSV. RAW fallback saves 5 bytes on random relative to the nested Brotli selection, but the 8-byte frame still costs 3 bytes vs bare q5. No comparisons between this sum and Brotli q11 are evidence of general dominance, because per-file decoding, encode cost, memory, binary and dataset admission differ.

### Memory and decoder-binary measurements (I18 only)

The statically linked multi-arm I18 binary was **1,062,832 B**, not a standalone decoder-footprint comparison. Isolated single-shot `/usr/bin/time -v` measurements (max RSS in KiB) on I18 showed:

| Input | Encode max RSS KiB | Decode max RSS KiB |
|---|---:|---:|
| Arithmetic | 10,964 | 3,800 |
| Time series | 10,784 | 4,348 |
| Generated JSONL | 16,932 | 9,856 |

These process-level RSS values are not paired q5/q11 RSS, not peak allocations inside the timed benchmark, and cannot clear a multi-codec memory front on their own. CI `selftest.txt` contains all four independent self-test pass markers, and `roundtrips.txt` has **21/21** cases across budget, I17 fast and I17 full.

## Scientific assessment and research priorities

1. **Correctness:** I18 is valid as a new experimentally encoded/decoded AVH2 format on the frozen set. Malformed-wire selftests reject selected corrupt framing, but broad mutation fuzzing, sanitizer stress, and decoded-resource adversarial testing remain incomplete; do not label production hardened.
2. **Mechanism:** I16 shows localized sparse innovative bitwidth tiles are useful on selected structured data, and I18 establishes they can be deployed inside a fast fallback portfolio. It is not new prior-art novelty: differential coding, bitpacking, FOR/PFOR and portfolio selection are established design spaces.
3. **Encoder priority:** I18 numeric candidate **dual evaluation** consumes significant CPU and misses the available fast-profile encoding rate. Next compare *AVI6-only*, *AVI4-only*, and a cheap-bound prefilter with fixed maximum regret, measuring chosen bytes and candidate calls, and require exact backstop for correctness. Do not infer AVI6 dominance from these seven already-consumed files. For time-series find an inexpensive certifiable lower bound or early reject before full AVI4/AVI6 scans, without specializing on fixture filenames.
4. **Residual priority:** The **290-byte** time-series deficit vs q5 is an engineering target, not a threshold to overfit. Examine per-lane width descriptors, tile bit padding and outlier placement; evaluate how much each component accounts for before introducing new modes. A smaller aggregate on a synthetic fixture is not a general win.
5. **Generality priority:** R2 Gate A/Q1a-XRUN is now independently verified **PASS**; concentrate on distinct **Gate B**: independent real-origin typed telemetry, time-series, structured JSONL, compiled binaries, natural language, media and incompressible controls; modern typed-codec competitors Sprintz, FastLanes/ALP, Gorilla/Chimp, PFOR and middle-out where applicable; match window and memory. Keep hidden heldout truly inaccessible to tuning.
6. **Frontier priority:** Use epsilon-constrained vector (complete bytes, encode work, decode work, encoder/decoder RSS, binary bytes), per-file and aggregate resource constraints with error bars; non-dominated against matched q5/q11, zstd, xz, modern typed baselines. Expose all regressions. **Maintain frozen verdict: 435 DOMINATED / 28 DEGENERATE / 5 unresolved FRONT-GAP / 0 verified FRONT-CROSSING.**

### Experiment constraints

No additional worker, delegate, swarm or local CPU-heavy benchmark was run in this handoff. I18 CI is **consumed discovery**, NOT independent frontier certification. The green run is credible correctness/source evidence *and* has machine-readable observed pilot efficacy; **Gate A already passed earlier via Q1a, but Gate B's stricter Class-A admission freeze remains**. No promotion to the production compression format is authorized. Pending independent heldout corpus, preserve this branch and all uncommitted earlier R2 worktrees.
