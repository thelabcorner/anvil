# ANVIL: Decoder-Work-Constrained Lossless Reconstruction — Research Synthesis and Open Questions

**Research report / evidence-graded working paper · 7 October 2026**
**Project:** [thelabcorner/anvil](https://github.com/thelabcorner/anvil)
**Document state:** living scientific synthesis, **not** a peer-reviewed article, benchmark submission, implementation specification, certified novelty claim, or replacement for frozen preregistrations.
**Source of record:** [ANVIL research ledger](../RESEARCH_LEDGER.md), [I17–I18 adjudication](I17-I18-SOURCE-ATTESTED-EXPERIMENT-ADJUDICATION-2026-10-07.md), [I18 results](I18-BUDGET-GATED-FUSION-RESULTS-2026-10-07.md), [I19 source attestation](I19-PHASE0-SOURCE-ATTESTATION-CLOSEOUT-2026-10-07.md), and the named immutable GitHub Actions runs. **Do not use this synthesis in place of raw archived outputs.**

## Abstract

ANVIL is an experimental lossless-compression research program investigating whether compact *explanations* of a byte stream can reduce serialized information while keeping reconstruction deterministic, resource-bounded, and inexpensive. Historical LZ, arithmetic/rANS, sparse corrected copies, BWT routing, lexical reordering, and modular numerical predictors show isolated byte or decode-speed improvements, but no verified general-purpose multi-axis Pareto crossing. On an already-consumed 256,000-byte synthetic arithmetic sequence, the I18 budgeted AVI6/Brotli-q5 portfolio produced a complete 19,445-byte archive against 33,204 bytes for the matched I17 fast selector, with 381.422 MB/s same-run observed-output decoding, compared with 161.660 MB/s for native Brotli q5. The same I18 implementation encoded the arithmetic sequence at 4.089 MB/s, below I17 fast's 11.473 MB/s; a time-series fixture fell back to q5 after two unproductive numerical candidate evaluations. The I19 acquisition phase subsequently hash-locked four independent-publisher response bodies and verified four **derived** non-held-out numerical streams totaling 112,620 bytes. I19 has not performed a compression efficacy comparison. Its provisional UCI reserve is exposed in an artifact and is not a sealed holdout. The scientific contribution to date is a carefully instrumented *research trajectory*, not a novel or generally superior codec. This report identifies exact hypotheses, cost models, controls, evidence gaps, and the falsification conditions for future experiments.

**Keywords:** lossless compression; reconstruction; modular innovations; integer bitpacking; Pareto optimality; decoding complexity; provenance; multivariate time series; experimental reproducibility.

## 1. Research questions and scope

**RQ1 — compression:** Can exact decoder-known structure reduce *complete serialized bytes*, including all framing, model metadata, residuals, exceptions, checksum, and source reconstruction, on independent real input domains?

**RQ2 — execution:** Can reconstruction retain enough decode throughput, bounded latency, reference locality, memory, and implementation size to constitute an operationally useful point, not merely a ratio improvement?

**RQ3 — selection:** Can encoder-side inspection reduce the cost of testing expensive representations while bounding *empirical* byte regret against an exhaustive selector, without leakage from validation or holdout sets?

**RQ4 — novelty:** Is an observed gain attributable to a distinguishable mechanism absent from appropriate prior art, rather than use of established delta/FOR/PFOR, tile packing, entropy coding, backend selection, or domain-specific transforms?

The primary target is complete end-to-end lossless compression of arbitrary input *bytes*. A **typed-numeric projection** is a legitimate, narrower competition only if its input representation, data type, missingness, endianness, and exclusions are explicit; exact restoration of typed bytes does **not** imply restoration of their parent text/JSON/DLY/ZIP.

## 2. Theory and operational model

### 2.1 Exact reconstruction

Let x be an n-byte source and (d,r) an encoder-selected decoder-visible description and residual. Losslessness requires D(d,r)=x exactly. For a typed w-bit lane, arithmetic occurs in the ring Z/(2^w), not unbounded signed arithmetic; signed interpretation and zigzag map must be specified separately. Decode must define byte order, overflow, restart seeds, malformed-input response, reference reachability, and maximum output/work. Do not use floating-point approximate predictors as a supposed bit-exact decoder unless raw bit residuals close every difference.

A useful encoder-side explanation cost is

```text
L_complete = L_envelope + L_metadata + L_predictor + L_restart
           + L_packed_residual + L_exception_positions
           + L_exception_values + L_literal + L_integrity
           + L_source_reconstruction
```

**Every term is counted only once, from actual serialized output**; estimated marginal costs may screen candidates but may not replace observed complete-archive bytes. The explanation cannot be counted as “free” if it is inferred at expensive decoder time. Exact reversible transformation cannot lower the Shannon entropy of the entire joint object; finite coder models and computational budgets allow its *representation* to be more practical.

### 2.2 Reconstruction-work constrained frontier

For codec/profile c and source x, define the observed vector

```text
F(c,x) = (
  complete_archive_bytes,
  encode_wall_ns,
  decode_wall_ns,
  peak_encode_RSS_bytes,
  peak_decode_RSS_bytes,
  decoder_binary_bytes,
  max_decoder_window_bytes,
  worst_case_decode_work_bound
).
```

Additional dimensions (startup, random access, energy) may be tracked when prerequisites are equal. Componentwise dominance applies only on a **matched reference class**. If a dimension is missing, its status is **unresolved**, not zero or ignored. Separately report operational epsilon-constraint profiles (e.g., minimize bytes subject to decode/RSS/encoder-work budgets); do not conceal tradeoffs behind a post-hoc weighted scalar. The scientific object is a set of nondominated *measured operating points*, and a library of conditional routing choices is not automatically a new compression mechanism.

### 2.3 Mathematical candidate families

For a lane sequence x_i mod 2^w and fixed known lane stride s, compare predictors under identical residual coding:
- absolute/frame of reference: p_i=a;
- global affine: p_i=a+i d;
- local first difference: p_i=x_(i-s)+d;
- local second difference: p_i=2x_(i-s)-x_(i-2s);
- shared per-record innovation across correlated lanes: p_(i,j)=x_(i-s,j)+d_shared+delta_j;
- XOR/predictor plus exact bitplane or residual literals;
- exception-aware packing and fully accounted index/patch maps.

All of these families overlap heavily with established prior art. A useful new finding must identify a *specific causal algorithmic innovation* or a credible new tradeoff under controls. Encoder-side discovery can be sophisticated; decoder-side execution should favor fixed-length batches, direct memory access, short dependency chains, restart points, SIMD-friendly unpacking and safe bounded execution. Novelty by rearrangement or renaming is disallowed.

### 2.4 Limits of bounded selectors

No bounded probe that ignores part of an arbitrary source can guarantee distribution-free zero regret: two inputs identical on inspected locations can differ arbitrarily elsewhere. The I18 early-prefix stride probe is therefore a heuristic, not a theorem. A future multi-region oracle should use deterministic positions fixed before evaluation, log **all** probe work, and compare against a full-evaluation diagnostic oracle:

```text
absolute_regret(x) = max(0, S_selected(x) - S_exhaustive(x))
relative_regret(x) = absolute_regret(x) / S_exhaustive(x)
aggregate_regret = sum_x absolute_regret(x) / sum_x S_exhaustive(x)
```

In these expressions S must be complete wire bytes under exactly specified candidate envelopes. The exhaustive comparator consumes resources and its cost must be reported even if it is absent from the deployment profile. Detect and report cases where a cheap probe would miss a large win.

## 3. Historical findings by phase — no chronology conflation

| Phase | Evidence-supported result | Boundary / negative result |
|---|---|---|
| Early LZ/rANS/TCOPY | Learned practical parser and decoder bottlenecks; sparse correction useful on selected structured data | No verified whole-front crossing; TCOPY distinctness and novelty unproven |
| I9 canonical portfolio | Silesia 46,446,995 B vs Brotli q11 49,383,136 B; enwik8 23,534,368 vs 24,810,180 B at cited geometry | BWT decode/RSS deficits prevented full dominance |
| I10 auxiliary inverse BWT | Same-run ANVIL internal decode improved 1.364–2.339x for charged byte increases | Still FRONT-GAP_COST versus external reference codecs |
| I10 Grotli G1/G3/G4/G5 | Same-backend lexical column reorder -21.77% on a specific receipt dataset; G5A ordering causal on discovery | G1 broad NO-GO, G3 independent validation +3.19% adverse, G4 proxy NO-GO, G5B ordinal adverse |
| I11 R1/R2 | Affine discovery variants and decoded-output observation hardening | Predeclared selection gate failed |
| I12 | Arithmetic synthetic, complete 62,458 B vs Brotli q11 87,013 B | Synthetic-specialized; six negative controls; no generality |
| I13 | All 69 time-series blocks structurally modeled | Archive 139,387 B vs q11 134,718 B: descriptor/packing costs dominate |
| I14/I15 | Local differential and field-local improvements on consumed fixtures; see their exact source reports | No independent real-origin generalization established |
| I16 fusion | Time-series AVI6 120,884 B vs I15 128,560 B and q11 134,718 B | Brotli q5 **120,594 B**, still 290 B smaller; not dominance |
| I17 | q11-free fast selector greatly reduced encode work | Regret on q11-friendly jitter and JSONL; selection is not novelty |
| I18 | AVH2 numerical/fallback portfolio, successful corrected run; all six limited discovery gates passed | Expensive numeric trials on time series, full-selector byte regret; 7 previously consumed inputs only |
| I19 Phase 0/0b | Source bodies hash captured; 4 typed derived streams semantically/cryptographically checked | **No codec efficacy**, no sealed holdout, no durable immutable archive |

The previous frozen Class-A adjudication accounts for **435 DOMINATED + 28 DEGENERATE + 5 FRONT-GAP = 468** entries and **zero FRONT-CROSSING**. The five gaps are unresolved, not wins. Subsequent I16–I19 progress has not been certified into a new Class-A result.

## 4. I16–I18 controlled quantitative evidence

**I16 source-of-record:** [Actions 37701888318](https://github.com/thelabcorner/anvil/actions/runs/37701888318) commit `9ca860f45d7dad1b617267372e378f18a93f2721`. On consumed time-series (280,000 B), AVI6 120,884 B / 6.556 MB/s encode / 507.937 MB/s decode versus q5 120,594 B / 37.024 encode / 228.779 decode. The absolute 290-byte loss against q5 matters; a q11-only win must not be marketed as domination of the baseline grid.

**I17 source-of-record:** [Actions 37701773624](https://github.com/thelabcorner/anvil/actions/runs/37701773624) commit `a26d770c400cd05b26195180ee4da157a8ee9c73`. AVH1 fast tests AVI4 plus q5; full additionally evaluates q11. On arithmetic the fast/full archives both measured 33,204 B, with 10.751/0.516 MB/s encoding in the I17 run. Dropping q11 increased selected bytes on jitter 86,671 vs 62,725 B and generated JSONL 207,708 vs 150,424 B. This validates a configurable work/ratio policy, not a mechanism improvement.

**I18 source-of-record:** [Actions 37703141058](https://github.com/thelabcorner/anvil/actions/runs/37703141058) commit `7ec50e8dadd1b3cc12d815c80007a9fcc2bfd490`. AVH2 uses q5, RAW fallback, bounded numeric probe, and AVI4/AVI6 under the probe. All 21 profile/file roundtrips and six preregistered discovery gates passed. The unsuccessful first integration run [37702906749](https://github.com/thelabcorner/anvil/actions/runs/37702906749) failed compilation before efficacy and is excluded.

| Consumed input | Bytes input | I18 complete B | I17 fast B | I17 full B | Native q5 B | Native q11 B | I18 mode |
|---|---:|---:|---:|---:|---:|---:|---|
| Arithmetic | 256,000 | **19,445** | 33,204 | 33,204 | 115,107 | 87,013 | AVI6 |
| Time series | 280,000 | 120,602 | 120,602 | 120,602 | 120,594 | 134,718 | q5 |
| Jitter | 974,920 | 86,671 | 86,671 | 62,725 | 86,663 | 62,717 | q5 |
| Random | 262,144 | 262,152 | 262,157 | 262,157 | 262,149 | 262,149 | RAW |
| Repeated JSONL | 936,000 | 186 | 186 | 168 | 178 | 160 | q5 |
| Generated JSONL | 2,803,267 | 207,708 | 207,708 | 150,424 | 207,699 | 150,415 | q5 |
| C++ source | 32,512 | 8,634 | 8,634 | 7,999 | 8,626 | 7,991 | q5 |
| **Sum of archives** | **5,544,843** | **705,398** | **719,162** | **637,279** | **801,016** | **705,163** | — |

Sums describe this selected seven-file mix, *not* a population metric, single concatenated archive, or overall Pareto proof. Bare native Brotli wires lack the AVH2 envelope; q11 is not invoked inside the I18 timed selector. On arithmetic I18 encoded at 4.089 MB/s vs I17 fast 11.473 and I17 full 0.482 in the same I18 job; decoded at 381.422 vs q5 161.660 MB/s. On time-series I18 encoded at 3.647 MB/s vs I17 fast 11.246, yet selected the same 120,602 B, making the two unsuccessful numerical candidate calls the strongest demonstrated optimization target. I18 measured one-shot RSS values (arithmetic encode 10,964 KiB, decode 3,800 KiB), **not** matched multi-codec RSS evidence. The 1,062,832-byte multi-arm linked binary is not a comparable decoder-only footprint. Shared GitHub runner timings are directional scout evidence and do not clear a confidence-bounded generalization claim.

## 5. I19 source/measurement admission: what is and is not established

**Acquisition:** [Actions 37704571734](https://github.com/thelabcorner/anvil/actions/runs/37704571734), source commit `c7e423e24ab94e1156f1245c1a4a06397ef15a70`; four raw HTTP *entity bodies*, not verified upstream database releases. **Typed semantics:** [Actions 37704897319](https://github.com/thelabcorner/anvil/actions/runs/37704897319), source `c09375511cc56f582c5147204bdaa2a2450e3e4f`. Independent Phase-0b evidence audit passed. These were acquisition and parsing workflows; neither benchmarked codecs.

| Role | Origin and projection | Type | Values | Bytes | SHA-256 |
|---|---|---|---:|---:|---|
| Discovery | NOAA station USW00003947 TMAX | signed 16-bit LE | 20,119 | 40,238 | `5fa00acbcc1d7e6f7341158e778dc1859cfb9aa838b7ab5617d214b37e968674` |
| Discovery | NOAA station USW00003947 TMIN | signed 16-bit LE | 20,119 | 40,238 | `74186bb2e58580be7b011a00e6bde9694a215d8e31d6d9ea679804137dcadf1f` |
| Validation | USGS site 01646500 discharge x1000 | signed 32-bit LE | 4,018 | 16,072 | `43216110c8a695493117902e066b8c42ce05c1b252d8e304fcc22ef875217590` |
| Validation | NASA POWER T2M x1000 | signed 32-bit LE | 4,018 | 16,072 | `83d9d384e4d2063724769b078cbac51b23400a0a8ed10c40d3e3a23870684b49` |
| **Total** | 4 typed projections | — | **48,274** | **112,620** | Hash **per stream**, no aggregate SHA stated |

The two NOAA streams share a source/station **and count as one discovery origin**. Each retains 393 missing-day slots coded as `-9999`; treating missing sentinel transitions as physical temperature differences is a scientific confound. NASA is modeled meteorological data; distinct publishers are not equivalent to statistically independent generating processes. Original DLY/JSON, station flags, timestamps, formatting, and omitted fields are **not reconstructible from typed streams alone**. Fixed-point projections used exact decimal-to-integer conversion with representability checks.

The UCI household-electricity ZIP (original 20,640,916 B, SHA-256 `9f84b46ade8a2d8e1286ec4b2b6c2987a45a755c59f263be3b3b3d10dfbda3ff`) was included in a downloadable Actions artifact. It was not parsed in Phase-0b, but it was publicly/privately observable through the artifact and **cannot be represented as blinded sealed heldout**. Acquisition/semantic artifact retention ends **2027-01-05**. Copies outside Git worktrees are not durable, tamper-evident scientific deposits merely by existing. Origin licenses/attribution, release/version identity, durable immutable originals, source-to-typed transformation mapping, additional discovery diversity, distinct sealed reserve, fully frozen comparator implementation, and final statistical criteria remain **admission blockers**.

**Efficacy verdict: BLOCKED.** Four typed streams have been *prepared*, zero numerical compression experiments have been run on them under I19. The I19 manifest remains unfrozen. The structural and efficacy-admission validators reject known bad metadata; passing a validator cannot establish legal permissions or cryptographic custody.

## 6. Threats to validity

1. **Selection bias and repeated consumption.** I12–I18 repeatedly reuse synthetic arithmetic/time-series data. The I18 archive win is discovery-specific, not a fresh independent confirmation. Repeatedly examining validation reduces its inferential value; treat it as exploratory and replace the validation origin when contaminated.
2. **Strong controls.** Brotli q11 is not the binding competitor on every file; q5 defeated I16 time-series bytes, and typed integer codecs may be much stronger than either. Compare pinned releases of Zstd, LZ4/LZ4HC, Sprintz, FastLanes, FOR/PFOR, OpenZL numerical graphs, shuffled/bitshuffled carriers, and appropriate floating-point codecs *on semantically identical data*.
3. **Source-vs-projection problem.** A 40 KiB temperature vector excludes station/month/date/quality markers and is not a 3.8 MB DLY file encoded losslessly. Publish typed results in a separate table from full-source results; full source reconstruction requires the sideband and its bytes.
4. **Hardware, clocking and censoring.** CI VM variation, thermal/frequency drift, cache effects, small inputs, and compiler differences affect MB/s. Require paired randomized/interleaved order, raw timing repetitions and uncertainty estimates; do not compare raw MB/s across jobs as a performance result.
5. **Decoder observation.** Force reconstructed outputs into a nondiscardable checksum or cryptographic digest and separately verify equality to original bytes. A benchmark whose reconstructed output is optimized away is invalid.
6. **Unfair container cost.** Attribute the outer AVH1/AVH2 header, checksum, dictionary, predictor descriptors, restarts, required padding and metadata. Disclose whenever reference native frames and ANVIL envelopes differ.
7. **Memory and executable cost.** Peak process RSS and decoder-only binary footprint must be paired and scoped. Missing measurements cannot disappear from the objective vector. Controlling maximum theoretical working set and malformed-input worst-case expansion is a separate safety gate.
8. **Prior art and causal explanation.** A winner that reuses delta, predictor, FOR/PFOR, tiled packing, zigzag or codec portfolio is not inherently novel. Isolate candidate generation, residual representation, locality order, decoder schedule, and entropy backend through matched ablations.
9. **Dependence among origins.** Two climate publishers may measure overlapping geography/weather dynamics. Independent publisher labels do not guarantee independent statistical samples; report generating processes and group by true lineage.
10. **Researcher degrees of freedom.** Preregister hypotheses, candidate grids, limits, exclusions, order, statistical method, thresholds, and stopping rules before efficacy. Record failed CI before measurement separately from adverse efficacy.
11. **Archive life cycle.** A transient source URL, 90-day Actions artifact, mutable release tag, or unreviewed data right fails long-term reproducibility even when its captured SHA is correct.
12. **General-purpose vs domain specialization.** Size improvements on selected numerical types can support a useful specialized compressor but not ANVIL's general-purpose goal without a much broader heterogeneous independent corpus.

## 7. Reproducibility / claim ladder

| Tier | Evidence needed | Claim permitted |
|---|---|---|
| 0 idea/prior-art hypothesis | Written algorithm, competing explanation, explicit falsifier | Proposed mechanism only |
| 1 protocol/correctness | Pinned source+toolchain, recorded source bytes, exact roundtrips and error tests | This frozen implementation reconstructs tested inputs |
| 2 synthetic discovery | Predeclared paired experiment, complete wire, logged output-observed times and controls | Fixture-specific improvement/tradeoff |
| 3 independent real-source discovery | Origin-locked typed/full-source semantics, independently verified data rights, matched comparators, strong negatives | Narrow real-domain result on examined origins |
| 4 validation | Untuned genuinely independent origins, source-pinned artifacts, robust paired CI, matched memory/size | Confidence-qualified limited external validity |
| 5 blinded heldout | Pre-frozen selectors, independently controlled reserve unsealed once, intact provenance and full comparisons | Stronger external-validity claim on specified population |
| 6 general-purpose frontier | Heterogeneous canonical independent corpus, dense modern reference profile grid, all measured axes and uncertainty | A qualified general-purpose Pareto claim |
| 7 mechanism novelty | Published closest-prior-art comparison plus causal factorial ablations and independent replication | Evidence of distinguishable mechanism (still not a legal patent conclusion) |

A successful workflow is only evidence of the steps it actually executed; passing six synthetic gates is not equivalent to Tier 6. The canonical historical Class-A state remains **0 full crossings**.

## 8. Preregistered next-step template (proposed; NOT frozen or executable)

**Hypothesis H-A:** A low-work AVI6-only filter retains arithmetic-structure gains on *new discovery-origin* typed sequences while reducing I18's wasted AVI4/AVI6 calls and preserving a fixed maximum tolerable byte-regret distribution. Competing hypothesis: the synthetic advantage is a fixture artifact and modern integer codecs dominate.

**Hypothesis H-B:** A multi-region residual-summary oracle separates true globally regular sequences from prefix-regular/noisy-tail adversaries, under a strict sampled-byte/CPU budget. Competing hypothesis: selector cost plus false-negative regret dominates its savings.

**Hypothesis H-C:** Field-local tiled innovations improve complete bytes against byte/bitshuffle+Zstd, Sprintz, FastLanes, and OpenZL tuned and untuned numeric graphs, with no decode/RSS regression. Competing hypothesis: conventional typed transforms and better bitpacking explain all gains.

For each *new* experiment:
1. Freeze distinct discovery/validation/heldout lineages and original source SHA, rights/attribution, version, dtype, endian, missingness, exact full-source restoration policy; do not promote provisional exposed UCI reserve to heldout.
2. Precommit tool and binary pins, CI image, ISA, repetition count, randomized paired timing order, CPU affinity, output checksum, RSS sampler, full wire-count script, per-origin aggregation, expected magnitude, acceptance and kill rules.
3. Include untouched I17/I18 source controls plus strong matched native and typed codecs. Distinguish separate native frame comparisons from fair common-envelope ablation.
4. Compare at least reject-both, AVI6-only, AVI4+AVI6, calibrated multi-region filter, always-full oracle; measure candidate invocations, filter overhead, total encode wall time, missed best-mode bytes, selected archive size, decoded CPU work, RSS, format and standalone decoder size.
5. Use negative controls (random, same-prefix noisy tail, missing bursts, pathological stride, wraparound, mixed-width, small/empty, truncation). Stop at a correctness or bounds failure *before* efficacy.
6. Use [normative paired benchmark protocol](github-actions-benchmark-protocol.md) for repeated raw samples and confidence intervals; report inconclusive contrasts honestly. Do not let a new post-hoc axis or exchange rate replace preregistered gates.
7. Keep I19 proposed validation threshold separate from the mathematical concept of nondominance: >=3% complete-byte improvement on at least two genuinely independent validation origins vs matched competitor, decode throughput >=90% and peak RSS <=110% of that same comparator, plus negative-control fast-path constraints (see [I19 design](I19-REAL-NUMERIC-WORK-BUDGET-EXPERIMENT-DESIGN-2026-10-07.md)). These values are **design criteria**, not frozen efficacy authorization.
8. A full test of general-purpose competitiveness requires a *separate* heterogeneous Class-A design. Do not reinterpret numeric-specific success as that result.

## 9. Prior art: explicit non-novel baselines and research context

- Blalock, Madden & Guttag, [*Sprintz: Time Series Compression for the Internet of Things*](https://arxiv.org/abs/1808.02515) (2018): short predictive integer time-series and fast, low-memory decoding; close baseline for ANVIL's lane predictors.
- Collet et al., [*OpenZL: A Graph-Based Model for Compression*](https://arxiv.org/abs/2510.03203) (2025): composable codec DAGs, domain-specific transforms, universal decoding; a serious comparator for any ANVIL transform graph or routing claim. The [OpenZL numeric Compression Transformer](https://openzl.org/blog/2026-09-24-compression-transformer/) (2026) learns to select graphs and raises the comparator standard further; its author-reported results are **not ANVIL reproduced**.
- [FastLanes](https://github.com/cwida/fastlanes): SIMD-friendly column transform and compressed integer processing; use a pinned runnable revision and report width/padding.
- [simdcomp](https://github.com/fast-pack/simdcomp): SIMD integer packing baseline; appropriate for the bitplane/exception kernel attribution.
- [middle-out](https://github.com/schizofreny/middle-out): contextual seed for XOR/numerical representation investigation, not evidence that ANVIL invented XOR structure.
- [Brotli](https://github.com/google/brotli), [Zstandard](https://github.com/facebook/zstd), [LZ4](https://github.com/lz4/lz4): baseline algorithms/configurations with disclosed whole-frame bytes.
- [I9–I10 frontier strategy](ANVIL-PARETO-FRONTIER-RESEARCH-STRATEGY-2026-10-07.md), [historical prior art](priorart-tcopy-external.md), [research do-not-reburn ledger](audit-2026-09-07/06-do-not-reburn.md).

**Explicit non-claim:** None of these comparisons is itself an exhaustive literature review, and citation to prior art does not mean a matched ANVIL head-to-head was run. Published results from third parties must not be inserted into an ANVIL measured Pareto table.

## 10. Current conclusion and research decision

The strongest supported result is **specialized, source-attested synthetic discovery of decoder-cheap numerical reconstruction**, tempered by meaningful encoder-work cost and unresolved real-data transfer. I19 has moved the campaign from synthetic-only inputs toward verifiable independently sourced typed test material, but **has not yet cleared corpus admission or measured codec efficacy**. The rational next funding decision is to close corpus custody, data rights, source reconstruction, and modern typed-codec competitor pins; only then execute a frozen encoder-work oracle on genuine discovery sources. If typed competitors explain the purported advantage, retain AVI6 as a useful controlled prototype but stop claiming a frontier discovery. If an independent narrow advantage survives, promote the *narrow* claim first and require a separate heterogeneous general-purpose certification.

**Research integrity invariant:** zero verified general-purpose full crossings until a separate Class-A adjudication changes it; no original frozen result or failed hypothesis is silently overwritten by this synthesis.
