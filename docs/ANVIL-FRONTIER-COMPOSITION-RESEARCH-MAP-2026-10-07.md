# ANVIL — Frontier Composition Research Map (I12 → I15+)
**2026-10-07.** Engineering decision document; new mechanism implementations must have separate frozen preregistrations and remote CI. No delegates. All CPU-heavy compile, fuzz, benchmarks, corpus processing, and competitive comparisons run in GitHub Actions.

## Executive distinction: discovery vs class-A frontier

ANVIL's historical R2 adjudication: **435 dominated, 28 degenerate, 5 unresolved, zero verified full crossings**. A synthetic-file win cannot alter this. A credible **general-purpose** frontier point must outperform or remain nondominated against matched current codecs on an independent, provenance-locked corpus with complete wires, encode speed, decode speed, peak RSS, binary size and memory-window budget, with statistical uncertainty.

**I11**: sparse exact affine or sparse replacement failed on all seven discovery fixtures. **I12**: mathematically dense, block-local modular residual bitpacking made the pre-consumed arithmetic 256,000B fixture **62,458B**, against Brotli-q11 **87,013B**, with measured digest-inclusive decode **358.872 vs 141.060 MB/s**; 63/63 modeled blocks. Six negative controls lost on size. **I13**: strided 32-bit columns recovered structure (407 modeled columns on 69/69 time-series blocks), but **139,387B** vs Brotli **134,718B** on that 280,000B fixture. It slightly improved arithmetic (62,256B), while general text/JSONL remained dramatically worse. **I14**: tests local modular difference residuals as a candidate alongside I12, same-run paired; its scientific verdict must be read from its remote artifact, never predicted.

## 1. Mathematical signal taxonomy

Let block samples x_0,...,x_(n-1) belong to R = Z/(2^b). Preserve exact modular semantics across wraps; no undefined signed overflow or floating-point approximation.

**A — absolute / FOR:** encode z_i = ZigZag(x_i - a mod 2^b). Allows wide jumps and quick random access. Strong when values clustered; decoding embarrassingly parallel. Already standard (FOR/PFOR).

**B — global affine:** encode z_i = ZigZag(x_i - a - i d mod 2^b). Fast closed-form reconstruction; cumulative random-walk drift expands residual width. I12 used a variant of this with robust median estimation.

**C — local first-difference innovation:** z_i = ZigZag(x_i - x_(i-1) - d mod 2^b). Strong when per-step deviation is narrow even as the absolute path drifts. Decoder recurrence is serial absent checkpoints, lane transposition, or chunked prefix sums. I14 isolates this.

**D — bounded recurrence:** x_i = Σ_k a_k x_(i-k) + e_i (mod 2^b), small k chosen under *complete serialization and decoder-work budget*. This may discover exact deterministic relations without float rounding; explore only if B/C fail and fit has enough evidence. Reversible order-k predictors have extensive prior art.

**E — low-rank shared innovations:** interleaved fields x_(i,j) with correlated discrete innovations e_(i,j) = c_j u_i + r_(i,j), mod 2^b. Charge the shared stream u, coefficients, sparse residuals, field map, and inverse work. Do not claim novelty without close algorithmic prior-art comparison (predictive vector codecs, multivariate time-series compression).

**F — near-repeated record skeletons:** treat record bytes as LZ phrases plus low-entropy innovation spans; test whether encoder-only field detection leads to cheaper independent decode than sparse field patching. Compare to ordinary LZ and BCJ-like transforms, do not equate representation change with mechanism novelty.

**G — XOR/tiled bitplane numeric:** x_i XOR predictor, encode zero-leading/trailing bitspan, reuse middle-out [schizofreny/middle-out](https://github.com/schizofreny/middle-out) as a *research comparator/seed*, not as a novel discovery. Real floating-point cohorts are mandatory; avoid false advantages from numeric integer fixtures.

## 2. Strict explanation cost

For every candidate explanation m in a block, use

    L_total(m) = L_mode + L_schema + L_restart + L_predictor + L_packing
               + L_exception_locations + L_exception_payload + L_raw_residual
               + L_container + L_checksums

Use actual bytes after serialization, NOT Shannon estimates or residual-only bytes. Where an external LZ/entropy carrier is involved, include its framing and reset costs. Avoid applying expensive per-block entropy when the compressed benefit is smaller than headers.

Selection is resource-constrained, not ratio-exclusive. Preserve points nondominated under

    (complete bytes, measured encode seconds, measured decode seconds, peak RSS bytes, binary bytes)

and reject paths whose worst-case decode steps, fanout, pointer jumps or scratch allocations breach explicit bounds. A decoder with cheap sequential memory accesses and straight-line reconstruction is favored over sophisticated irregular metadata, even when the latter reduces a handful of bytes.

**File-level guardrail:** arithmetic-only RAW bypass must NOT leak into general-purpose release. A full-file, content-adaptive strong LZ/Brotli/Zstd carrier acts as permanent fallback, with minimal mode header. Compression decisions must account for *both* candidate encoding costs (a ratio win bought by doubling expensive encoding is not free).

## 3. Next gated research experiments (distinct, not aliases)

### I14: local-step differential source (already pre-registered and executing)

H1: DIFF mode actually selected on the known arithmetic fixture. H2: AVI4 full bytes strictly smaller than I12 from *same* runner; H3: smaller than same-run Brotli q11; H4: faster paired digest-inclusive decode than Brotli q11. Full regression cohort and byte-for-byte decode proof required. A run stopped by compiler/selftest/source mismatch provides **zero** efficacy evidence.

If DIFF saves bytes but costs decoder speed, test *residual representation* against *decode dependency* separately before optimization.

### I15-A: strided byte-record locality versus I13's fixed 32-bit word catalog

Hypothesis: recover true 14-byte / 23-byte record layouts without hardcoding those as secret hand-tuned wins, using a finite, frozen candidate stride catalog and a cheap encoder-only evidence filter. Field descriptors: offset, length, width and mode; ensure no overlap and exact uncoded residual byte coverage. Compare the same 7 consumed inputs and independent new real cohort **in distinct phases**. If byte framing improves time-series size but not speed, label specialized tradeoff.

### I15-B: cumulative innovation versus patched exception coding

Current flat maximum residual width k lets one exceptional word tax every other residual. Introduce fully specified PFOR-style:
- core width k' selected from frozen finite catalog;
- packed low bits for every residual;
- precise sparse location list (canonical increasing deltas) and high payload (or full replacement);
- exact descriptor+patch+restart+raw costs.
Any benefit on old synthetic data is discovery only; strong controls are high-entropy and adversarial outlier distributions. Compare to FastLanes/PForDelta/Sprintz rather than attributing standard mechanisms to ANVIL.

### I15-C: parallel reconstruction

Local recurrence induces a prefix sum: x_i = x_0 + i*d + Σ_(j<=i)e_j in the modular ring. Options are fixed-size tile restart seeds, lane-transposed prefix sums or parallel scan with preserved exact wrap semantics. Charge additional checkpoint bytes and block dependency. Benchmark ALU, unpack, scatter and digest costs individually; compare exact full-wire throughput on equal hardware and choose a tile size before held-out. FastLanes already exploits transposed decode for dependency-heavy delta transforms; this is engineering work, not a novelty claim.

### I15-D: mixed codec full-file selector

Encode strong general-purpose fallback and specialized branch into whole-file envelopes. Strictly select by complete serialized bytes under explicit encoding-work budget. Validate that RAW-only numeric encoders cannot be accidentally selected on JSON/text where LZ wins by orders of magnitude. Persist evidence about which mode was actually used, including whole-file and per-block counts. Both codecs must run and roundtrip before promoting a dispatcher.

### I15-E: real numeric proof and subtype competition

Before any claims, lock multiple legally usable *independent* real-world series, with explicit sensor source, origin, aggregation and dedup groups. Split discovery/validation/held-out by **origin**, not random chunks from the same file. Fix CPU affinity/frequency/noise policy where feasible, sample paired repeats and confidence intervals, and compare at least:
- Brotli q5/q11, Zstd selected levels, LZ4HC and a high-throughput LZ baseline;
- Sprintz, FastLanes, Gorilla/Chimp and ALP on schema-equivalent numeric families;
- standalone byte-shuffle/bitshuffle+same carrier to attribute gains beyond known transforms.

**No general-purpose Pareto promotion** from lower whole-file bytes on one constructed arithmetic test; memory footprint, decode time, encode time, and corpus provenance remain primary gates.

## 4. Branched decisions

- **I14 fails to select DIFF** → stop exactly this geometry; inspect score versus global affine and bytewide maximum. No unregistered threshold expansion.
- **I14 selects but bytes tie I12** → stop DIFF as standalone; test bounded exceptions or strided alignment only as a separately preregistered experiment.
- **I14 saves bytes and slows decode** → isolate prefix-scan recurrence, checkpoint cadence and bit-unpacking cost. A ratio-only result is still useful, not a Pareto win.
- **I14 saves bytes and improves digest-inclusive decode** → retain as numeric-specialized discovery; advance **independent sources and typed-codec comparison** before full integration.
- **I13 remains no-byte-win** → instrument field-level cost, raw gaps, per-record scatter and stride mismatch. Do not inflate success based solely on modeled column count.
- **Combined model beats matched Brotli on numbers but loses text** → invest in explicit full-file fallback, not a promise that numeric structure will generalize.
- **All specialized families fail independent real data** → shift engineering emphasis to Q2 matched block/window and fast LZ/entropy foundation, not unconstrained novelty hunting.

## 5. High-value skepticism

1. Has the proposed encoder discovered *information* not already exploited by standard codecs, or has it only benefited from intentionally generated arithmetic? Cross-family independent tests answer that.
2. Is the new mathematical description shorter **after all metadata** and control-plane costs? If not, kill quickly.
3. Can the decoder reconstruct with contiguous reads, bounded writes, and predictable branches? If not, optimize representation before SIMD micro-tuning.
4. Is the performance gain reproducible on the same workload, same pin, same runner? Digest-inclusive rates are paired scouting, not benchmark confidence.
5. Is the claimed mechanism distinctive after comparison to FOR/PFOR, Sprintz, FastLanes, Gorilla, OpenZL, LZ4, and established transform graphs? New arrangement alone is insufficient.
6. Does ANVIL have a compelling region of the full ratio/speed/memory Pareto surface, including decode/encode budgets and corpus diversity? If no, retain as experimental technology rather than claiming product-level superiority.

## 6. Prior-art and context seeds

- [Sprintz paper](https://arxiv.org/abs/1808.02515) — online forecasting, packed predictive errors, low-memory and high decode rates.
- [FastLanes](https://github.com/spiraldb/fastlanes) — vector-friendly transposition and fused delta/bitpacking; compare specialized decode layout directly.
- [Middle-out](https://github.com/schizofreny/middle-out) — SIMD / XOR structure, relevant to floating-point innovation research.
- [ANVIL I12 measured closeout](I12-BOUNDED-INNOVATION-RESULTS-2026-10-07.md)
- [ANVIL I13 measured closeout](I13-STRIDED-FIELDS-RESULTS-2026-10-07.md)
- [ANVIL I14 frozen pilot](I14-DIFFERENTIAL-INNOVATIONS-PREREG-2026-10-07.md)

**Status at document creation:** I14's remote post-fix run 37699578503 is executing; update evidence and decision in a separate result document, never retroactively alter the frozen I14 thresholds.
