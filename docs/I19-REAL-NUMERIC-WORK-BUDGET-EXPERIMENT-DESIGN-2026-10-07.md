# I19 — Real-Source Reconstruction and Encoder-Work Oracle (Design, NOT a measured preregistration)

**Date:** 2026-10-07. **State:** DESIGN ONLY. Frozen I18 result: [I18 closeout](I18-BUDGET-GATED-FUSION-RESULTS-2026-10-07.md). **DO NOT run efficacy comparisons or label data heldout until a separate, immutable source-origin manifest and final I19 thresholds are committed and attested.**

## Scientific question

Can ANVIL's bounded 32-bit mathematical innovations genuinely compress independent real numerical streams while reducing wasted expensive candidate evaluations, without shifting decode cost to inference? The measured I18 example is instructive: on synth-timeseries, the encoder invoked AVI4 and AVI6, but chose q5; its 3.647 MB/s encoding was 3.08x slower than I17 fast's 11.246 MB/s for identical complete bytes. Conversely, synth-arith gains 13,759 complete bytes from AVI6 while retaining 381.422 MB/s measured decode. Both are previously consumed synthetic fixtures, not validation.

## Two distinct hypotheses; neither implies novelty

**I19-A / real source:** On independent real streams, an AVI6-like arithmetic reconstruction model wins complete serialized bytes and remains non-dominated in decode time and peak memory against matched high-performance numerical and general-purpose codecs. If no real-origin family benefits, terminate mathematical-prominence claims even if synthetic gains remain.

**I19-B / work oracle:** A bounded encoder-only sample of local modular innovations predicts the *opportunity* for a candidate before paying its full expensive encoding cost. A short-circuit filter can avoid negative evaluations, but must count CPU work and false negatives. If the upper confidence bound of lost compression is larger than saved work under the frozen objectives, do not adopt the filter.

## Mathematical mechanism sketch (not a novelty assertion)

For typed `w`-bit streams in the ring `Z/2^w Z`, with inferred stride `s` and lane `j`, test predictor family
`p_(i,j) = x_(i-1,j) + d_j (mod 2^w)`,
innovation `e_(i,j) = zigzag_w(x_(i,j) - p_(i,j))`.
For each bounded candidate, compute its *serialized* residual budget:
`L = L_header + L_strides + L_seeds + L_predictors + L_packed + L_exceptions + L_unmodeled + L_checksums`.
Decoder reconstruction must be bounded and exact by fixed-width modular arithmetic, no floating point fitting in the decode loop. Separate every **measured** archive total from any **estimated** cost used for scouting.

The prospective routing decision is not `best predicted compressed bytes`. The relevant encoder action value is
`V(m) = E[(S_best - S_m)_+] - lambda * E[T_encode(m)] - mu * E[T_decode(m)] - nu * E[M_peak(m)]`,
with explicitly registered nonnegative weights for a named profile and a hard work/memory cap; each expectation must be estimated on discovery origins only. The actual Pareto report retains all axes and all nondominated profiles, rather than reducing science to one scalar. For the size-first profile, allow higher encoder work but report its cost; for the throughput-first profile, bound the number of expensive mathematical candidates. No unregistered weights or threshold searches after reading validation.

A useful *low-cost rejection* needs a conservative bound on residual coding benefit, not merely a high fraction of small first differences in the first 32 samples: I18's probe can be fooled by a regular prefix before noise. Probe multiple disjoint, content-independent sample positions, keep time complexity and byte budget explicit, and include prefix-regular/tail-random adversaries. A statistical estimate is not a proof; quantify false-negative rate against the always-evaluate full selector.

## Phase 0: origin-locked real corpus (required before Phase 1)

1. Select at least **three independent upstream origins**, each with stable public artifacts and explicit access/license permissions, and several file shapes (integer samples, interleaved records, counters/timestamps; optional bitwise-exact IEEE-754 after an independently justified representation). Candidate families include NOAA observations, USGS sensor histories, and open scientific instrument telemetry; candidates are NOT frozen selections.
2. Commit one manifest with `origin_id`, named source URL and publisher, source release/version/date, cryptographic SHA-256 of original downloaded bytes, acquisition recipe, media/field metadata, data rights, raw file byte count, and accepted decoded types. A single authority, dataset and derivative cannot cross origin splits.
3. Implement deterministic lossless conversion to explicitly defined numeric byte arrays with source byte-to-record mapping. If source is CSV/parquet, separate **typed-numeric payload** competitions from **original-file byte** competitions; record schema, missing values, offsets, timestamps, nulls, row order and all original metadata. Typed-only measurements cannot be marketed as end-to-end lossless compression of the original file.
4. Partition by origin before any codec tuning: discovery origin(s), validation origin(s), sealed heldout origin(s). If fewer than three sources survive, declare `BLOCKED_CORPUS`; do not reuse consumed I12–I18 fixtures as new heldout.
5. Freeze negative controls (unstructured text, executable bytes, incompressible random, periodic and adversarial prefix) with the same SHA/source rules.

## Phase 1: work-budget and cost-oracle A/B (discovery origins only)

- A0: frozen I18 AVH2 budget. A1: frozen I17 fast AVH1. A2: frozen I17 full AVH1. A3: Brotli q5; A4: Brotli q11. A5: exact AVI6 leaf independently. A6: new work-oracle candidate, with all deltas logged.
- Candidate oracle must log `sample_bytes`, `sample_positions`, `probe_ns`, `q5_encode_ns`, `avi4_encode_ns`, `avi6_encode_ns`, candidate counts, discarded potential byte wins, chosen complete wire bytes, decode ns and scratch peaks. Sampling selection must be deterministic, bounded and attested; candidate encoders must not execute in bypassed modes.
- A held-out *oracle loss* comparator always encodes all candidates, at the cost of timing, to measure false-negative opportunity regret. Such exhaustive arm is a comparator only and cannot be silently excluded from release work accounting.
- Run immutable, same-runner (Ubuntu 24.04, pinned sources) paired randomized-order trials, warm/cold regimes reported separately, at least seven independent encode and nine decode repeats, median plus bootstrap confidence intervals, output-observed CRC/SHA consumption, memory RSS isolated, plus cache/frequency/noise settings. Keep each file's whole compressed wire and mode labels.
- Rigorously adversarial checks: tiny/empty/truncated/wraparound/stride misalignment, bad varints, zero exception density, random high bits, alternating step modes, pathological regular first kilobyte then noise, noncanonical wire, input and output caps. Run sanitizer, roundtrip/fuzz, and measured tests **only on GitHub Actions**.
- No numerical result from these seven existing synthetic inputs counts as independent validation; old synthetic results are a diagnosis and reproducibility control only.

## Phase 2: controls and unbiased validation

- Include identical input representation for all codecs: Brotli q5/q11, Zstd fast/medium/high, LZ4/LZ4HC, and a pinned integer-specific comparator such as FastLanes/Sprintz when widths and semantics match. Floating-point candidate family requires Gorilla/Chimp/ALP or clearly recorded unavailable comparator. Compare standalone bitshuffle and byte-shuffle + identical Brotli/Zstd backends to isolate standard byte-locality benefits.
- Charge complete container bytes, frame overhead, predictor metadata and all source-reconstruction metadata. Report decode/encode throughput, peak RSS, max working window, program binary bytes and compile-time dependencies. Never claim ANVIL wins against typed codecs when only Brotli was actually tested.
- Define nondominance across the vector `(archive_bytes, encode_ns, decode_ns, peak_rss_bytes, code_bytes)`, all minimized; file-group aggregates use summed time and byte totals, **not averaged per-file MB/s**. An uncertainty-qualified crossing requires paired confidence bounds under frozen budgets, never a single medians-only lucky run.
- Preaccept Phase 2 only if at least two validation-origin real datasets exhibit non-dominated gain with >=3% complete-byte improvement vs a matched typed/general competitor, while decode speed >=90% and RSS <=110% of that same comparator; separately retain any demonstrably useful high-throughput profile. This is an intentionally difficult product-value threshold, not a mathematical definition of all nondominated points.
- Negative controls must not incur >5% median encoding slowdown vs frozen I18 baseline when mathematical candidates are rejected. False-negative byte regret and rate must be published even when the benchmark gate passes. Gate failures are retained, not tuned away.

## Phase 3: sealed heldout

Only after Phase 1/2 freeze and deterministic source attestation may one Actions workflow unseal heldout-origin artifacts. No additional threshold tuning following heldout results. Output a per-origin CSV and complete archive hashes, standard multi-objective Pareto analysis, paired CI, negative controls, memory and binary costs. One novel architecture is never certified because an encoder conditional chooses among known codecs.

## Execution constraints / explicit stop conditions

- One ChatGPT agent acting directly via OpenFork OXP. **No delegates, subagents, swarms or local CPU-intensive work**.
- All source fetch/corpus processing, compiler builds, sanitizers, fuzz and timing run on GitHub Actions. Local file reads, document editing, SHA metadata, and inexpensive metadata checks are permitted.
- Keep I18 commit `7ec50e8` immutable as measured control, and preserve other working trees without resetting.
- **STOP/NO-GO:** unverifiable provenance; origin leakage; lossy conversion; incomplete full-wire accounting; correctness or sanitizer failure; absent typed controls; unwarranted memory growth; absent or inconclusive real-data benefit; failed source-pinned independent reproduce.
- Under all outcomes retain complete negative evidence and update Research Ledger. The historical general-purpose frontier remains **zero verified complete crossings** until the Class-A gate is separately passed.
