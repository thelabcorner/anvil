# Project ANVIL — Pareto-Frontier Research Strategy, Mathematical Mechanisms and Experimental Decision Tree

> **Chronology/authority update (2026-10-07):** This is a preserved ideation snapshot. The later source-of-record Q1a-XRUN [37694740386](https://github.com/thelabcorner/anvil/actions/runs/37694740386) established **R2 Gate A PASS**, superseding §A.8's earlier unpublished state; **Gate B remains BLOCKED**. I16–I18 subsequently produced synthetic discovery, and I19 verified real input identities but ran **no codec efficacy**. See [research synthesis](ANVIL-RESEARCH-SYNTHESIS-AND-OPEN-QUESTIONS-2026-10-07.md) and [evidence index](RESEARCH-EVIDENCE-INDEX-2026-10-07.md). The dated assertions below remain part of the historical record, not present experiment authorization.

**Date:** 2026-10-07 (America/Chicago)  
**Status:** RESEARCH / IDEATION ONLY; not a codec implementation, experiment authorization, revised freeze, novelty claim, or performance result.  
**Authority:** `docs/swarm-2026-10-02/FROZEN-CLOSEOUT-MATRIX-R2.md` remains binding for execution.  
**Research baseline:** `docs/ANVIL-STATE-RESEARCH-HANDOFF-2026-10-07.md`; `docs/ORBIT_PROGRAM_COMPRESSION.md`; `docs/I10-BREAKTHROUGH-PROGRAM.md`; `docs/I10-EXPLANATION-GAP-ORACLE-DESIGN.md`; `docs/RESEARCH-SEED-MIDDLE-OUT-2026-10-07.md`.  
**Operating constraints:** no delegated agents, no swarms awakened/spawned, CPU-intensive work on **GitHub Actions exclusively**; local activity limited to reading, documentation, bounded static checks. No GitHub Actions were launched while producing this document.

> **Recommendation:** Stop treating high compressed-byte yield and fast decoding as separable engineering problems. Optimize for **reconstructible information per complete decoded unit of work** under exact source identity, memory, and output-size constraints. Favor inexpensive reversible programs with high explanatory power, while using a serious high-throughput LZ/entropy baseline as the numerical performance floor. However, new semantics/novelty are not established by a portfolio of already known transforms, programs, or selectors.

---

## A. What is actually measured, and why it matters

1. Frozen Class-A: **435 DOMINATED, 28 DEGENERATE, 5 unresolved FRONT-GAP, 0 FRONT-CROSSING**, 468/468 accounted. The five gaps are not wins.
2. Frozen ANVIL ratio-auto (historical): Silesia **46,446,995 B** vs xz **48,456,100 B** vs Brotli q11/lgwin30 **49,383,136 B**; enwik8 **23,534,368 B** vs xz **24,831,656 B**, Brotli **24,810,180 B**. These are byte-only wins at their recorded geometry, not complete front crossings.
3. I10 integrated auxiliary BWT: Silesia **46,466,339 B**, **47.9 MB/s** decode, **248.4 MiB** decode RSS; Brotli **49,383,136 B**, **166.9 MB/s**, **124.5 MiB**. Enwik8 ANVIL **23,537,422 B**, **26.5 MB/s**, **598.9 MiB** vs Brotli **24,810,180 B**, **150.2 MB/s**, **251.9 MiB**. Same-job reference decode-time disadvantages **3.447×** and **5.664×** respectively. Auxiliary indexing improved within-ANVIL integrated decode **1.3645×–2.3392×** for +2,494–3,054 B on named targets; this does not close the external gap.
4. **Byte headroom**, purely arithmetic and without a speed claim: relative to the named Brotli archive, the auxiliary route could grow by up to **2,916,797 B** on Silesia and **1,272,758 B** on enwik8 before surrendering its absolute byte lead. This is not a license to spend those bytes without clearing decode/RSS/encode gates; the relevant competing frontier includes other Brotli/Zstd/xz settings.
5. Historical Class-A fast/token modes have not crossed the reference envelope. The canonical winning `--parse=ratio` route **bypasses** LZ matcher/DP/MDL token parsing. Therefore optimizing those components alone cannot affect that route.
6. G3 strict-lexical regionized structured representation gained **5.5984%** over same-backend raw Brotli in D1–D4 discovery, but lost **3.1900%** on consumed V1. G5A shape-column ordering saved **6.1189%** relative to retained shape-token ordering on D1–D4, but V1 again went adverse. G4 planner proxy was **NO-GO**; G5B ordinal coarsening **+1.6507%** worse on D1–D4. These negatives are binding for the tested formulations.
7. Block/window geometry and incompressibility gate are confounded; Q2 Rev8's precise 2×2 is ready only subject to Gate A/publication. Previous block-warmup penalty evidence is **~5–17%** in a separate historical context, not a prospective Q2 result.
8. Evidence infrastructure: R2 Gate A is **unpublished/unattested**: narrow clean source publication and Q1a-XRUN are prerequisites. Gate B remains blocked by unverified c9 containment and absence of new independent locked held-out structured/numeric/executable families.
9. R2 novelty audit leaves **zero surviving funded mechanism novelty claims**. TCOPY, localized normalization, ordinary routing, parameter seeding, generic grammar/iterative program operators, and numeric field transforms are not newly novel merely from syntax or placement.
10. **No new ANVIL benchmark was executed in this ideation.** Every design, algorithm, threshold beyond a frozen threshold, and rank below is a hypothesis or decision recommendation, not an empirical gain.

## B. The actual optimization problem: full multi-resource Pareto, not one “best ratio”

For mode `m` on immutable source `x`, define the **complete** objective vector:

`V(m,x) = (B_complete, T_encode, T_decode, RSS_decode_peak, RSS_encode_peak, decoder_binary_bytes, startup, optional random_access_penalty)`.

Dominance is evaluated only on matched, declared dimensions. Missing RSS is **unknown**, never zero. File and aggregate weighting, reference geometry, decoder-only executable scope, window, CPU/ISA, repetitions and statistical decision rule must be frozen before measurement.

For analytical mechanism triage (not a promotion metric), let:

- `L(e)`: exact serialized explanation metadata/program;
- `L(r|e)`: exact residual stream cost under explicit backend and decoder-visible state;
- `W(e)`: measured or faithfully costed reconstruction cycles, memory traffic and scratch;
- `L_raw`: same-backend raw reference cost at identical geometry.

Then `ΔB(e) = L_raw - [L(e)+L(r|e)+L(container)]`. Candidate becomes a serious front prospect **only** when `ΔB>0` for relevant role-locked inputs and the actual decoder and memory costs fit a desired reference-front cell.

**Fundamental bound:** a lossless bijection does not reduce Shannon entropy of the *full joint object*. It changes which dependencies are exposed to finite-context coders and how fast they are decoded. Any apparently “free” parameter is allowed only if it is unambiguously derivable from decoder-known state and does not increase work unpredictably.

**Search without fake exchange rates:** use epsilon-constraint search: first impose maximum decode cycles/source-byte, RSS, binary size, and minimum decode throughput; among feasible candidates minimize complete bytes. Repeat for several explicit constraint vectors to generate an approximate frontier. A weighted scalar can rank local candidates in a fixed laboratory setting but may not retroactively overturn the R2 Q3 physical-exchange-rate closure.

### Why the decoder is the right unit of scientific reasoning

A descriptor saving 5,000 bytes is useless if it adds millions of irregular cache misses to reconstruct a large window. Prioritize operations with **O(n)** sequential scans/copies, vectorized fixed-width arithmetic, small bounded state and predictable contiguous accesses. Measure bytes read/written and dependent loads alongside cycles. Defend the model against malformed inputs and work amplification with explicit output/work ceilings.

---

## C. Independent strategic branches (convergent, competing and falsifiable)

### Branch 1 — FASTER EXISTING OPERATING POINT (highest execution plausibility, low fundamental novelty)

**Thesis:** It may be easier to create a legitimate fast/intermediate non-dominated ANVIL point than to make the BWT-heavy maximum-ratio point faster than Brotli q11 across every axis.

Build a clean comparison-grade **ANVIL-Fast reference path** from the strongest real token engine, actual optimized LZ match copy, efficient offset/length metadata and one or more strong entropy backends. Compare:
- existing rANS and direct/raw token stream;
- **PivCo-Huffman/PHA** as an occupied but relevant high-decode-throughput entropy approach;
- bounded table-based ANS/FSE if applicable, carefully counting table rebuild;
- **OpenZL-style bucketed offsets** with V-optimal histogram/bitpack before adding another entropy stage;
- appropriately configured Brotli q1/q4/q6/q9/q11, Zstd, xz, LZ4 and OpenZL itself.

**Why it might move a front:** decoder-bound workloads reward predictable memory-copy kernels, byte-oriented controls, short tables and SIMD/lane-local decoding. OpenZL reports >2× decode advantage over Zstd for its own native LZ engine at comparable ratios (external authors' reported context, *not* ANVIL measured). PivCo uses wavelet-tree-like partition bitmaps, some ANS on skewed branches; it is not a new ANVIL invention.

**Kill:** if ANVIL-Fast cannot beat a relevant existing point on a complete, matched multi-axis cell after block parity and RSS are charged, retain the implementation as a measurement baseline/architecture substrate, not a breakthrough. Never assume a standalone PivCo microbenchmark reproduces in an LZ decoder.

**Novelty:** adopt-class engineering only; explicitly cite OpenZL (September 2026), PivCo-Huffman (June 2026), Zstd/FSE/rANS lineage.

### Branch 2 — RECONSTRUCTION-WORK-CONSTRAINED SPAN IR (highest long-run research value)

**Thesis:** Optimize a bounded decoder-side algebra of cheap explanations, not an unconstrained bytewise predictor or general program interpreter.

The smallest candidate IR is:

```
RAW(n, bytes)
COPY(n, backward_distance)
FILL(n, byte)
AFFINE_RUN(n, width, base, step)
COPY_PLUS_EXCEPTIONS(n, distance, k, exact_patch_positions, exact_patch_bytes)
TILED_XOR(n, layout, seed_state, side_streams)
BOUND_LOOP(n, body_id, count, exact_correction_stream)
```

All integer widths, overflows, endian semantics, source reachability, overlap, expansion, dependency order and literal restoration must be in the wire contract. A given operator should be emitted only when its **decoder operation is cheaper than a strong matched baseline**. A control implementation must express the *same semantic relation* with existing LZ/BCJ/grammar/typed codecs; otherwise calling it new would be novelty-by-difference.

**Key research opportunity:** joint optimization over description length **and dynamic reconstruction schedule**, with explicit cache/latency/dependency penalties. Existing Orbit-LZ/IGS-IR and Brevis already occupy much of the conceptual program-synthesis space. A plausible narrower research gap is an experimentally superior *hardware-shaped, bounded-dependency explanation schedule* across heterogeneous record types. That is a **hypothesis**, not a novel mechanism claim.

**Source discovery:** SIMD byte-class/event scanner -> compact event posting lists -> transformation-invariant candidate keys -> exact verification -> Pareto candidate set -> bounded grammar/region selector -> compiled homogeneous decode batches. Avoid O(n²) transformed comparisons by requiring finite candidate caps and measuring useful verified candidates per generated event.

**Critical incompatibility:** an arbitrary span graph with forward references, aliasing or recursive production undermines streaming, safety and worst-case work. Default grammar must be causal and bounded. Explicitly separate any optional noncausal offline profile.

**Causal controls:** existing ANVIL exact LZ/SHAPE/TCOPY, same-transform BWT+Brotli, OpenZL graph, Brevis-like fixed DSL, simple grammar/repetition. **Kill** if description gain falls below *preregistered* materiality, metadata dominates, decode budget is exceeded, or benefits evaporate against a semantically equivalent established primitive.

### Branch 3 — RECONSTRUCTIBLE INNOVATION / CONDITIONAL-DEPENDENCY FACTORIZATION (high discovery merit, uncertain transfer)

A structured byte stream can be viewed as stable **carrier** + irregular **innovation** + exact inverse-placement schedule. Rather than large padding-heavy columns, encode only the sparse **events that break a predictor**.

Represent:
`X = Reconstruct(carrier, innovation, position_map)`.
A candidate is worthwhile when:
`L(carrier) + L(innovation | structure) + L(position_map) + L(reconstruction program) < L_raw`
and reconstructing does not require expensive random scatter.

Two distinct hypotheses:
1. **Exception-location factoring:** bucket abnormal widths/lexemes while grouping the high-commonality lexical carrier; use packed positions or gap codes with bounded density.
2. **Cross-field predictor factoring:** for fixed-width inferred fields, predict a value from known previous field and previous row via one tiny transform (XOR, additive, finite-difference) and encode exact mismatch stream. Discover only with verified byte alignment, otherwise a false schema can increase overhead dramatically.

**G5A lesson:** *ordering* was most of the local gain; do not credit alphabet changes, entropy backend, hierarchy or range coding unless isolated. **G3/V1 lesson:** real structured-looking data can already be near-optimal with raw Brotli. Test coherent-permutation nulls, shape-only, field-column, causal column contexts and original order with exact identical carrier bytes. Test whole-file and subregion overhead; a G4-style cheap proxy must not be resurrected as if its failure did not happen.

**Key design point:** lexical JSON must roundtrip verbatim (whitespace, byte case, escape sequences, exact numeric spelling, trailing incomplete frames, NaN text). Never reinterpret raw decimal strings as IEEE binary without exact lexical restoration.

**Novelty:** data transposition, typed predictor, PATCH/FOR, and innovation factoring are crowded prior art (FastLanes, ALP, Pcodec, BtrBlocks, OpenZL); only genuinely new decoder semantics plus independent evidence could survive novelty review.

### Branch 4 — TILE-ADAPTIVE MIDDLE-OUT / SIMD NUMERIC RECONSTRUCTION (comparative seed, not instant promotion)

The 2017 middle-out source uses eight independent XOR chains for 64-bit time-series, one block unchanged mask, byte-rounded trailing offsets and **one shared maximum significant length** per eight-lane frame. It exploits SIMD concurrency, but one outlier increases the cost of all changed lanes in the frame. Its original AVX-512 author's rates cannot be compared to ANVIL's measurements without matched hardware, codepaths and source roles.

Possible adopt-class redesign:
- short **tiled interleaving** that balances contiguous output stores, source-lane locality and independent recurrences;
- AVX2 baseline (important for Zen 3), optional runtime AVX-512 on actual supported CI hardware, scalar fallback;
- two-tier shared-width payload with escape bitset for outlier lanes rather than forcing eight lanes to the widest XOR;
- unchanged-mask + local exact bit/byte width + patched FOR versus Gorilla/Chimp, ALP, FastLanes and middle-out;
- 1/2/4/8/16-lane, contiguous/strided/interleaved/tiled ablations, including seeds/tails/headers.

Let per-frame shared width be `w=max_i w_i` for changed lanes. The padded-bit overhead is:
`P = Σ_{changed i} (w - w_i)`.
An exception-coded width model helps only if the saved `P` exceeds exact mask, class, footer and payload overhead. A robust model can choose `w_base` plus exceptions; select by exact encoded cost, not “median” intuition. Evaluate heavy-tailed outliers specifically.

This cannot be promoted from known synthetic telemetry. Acquire new, genuinely independent, frozen, **bit-exact numeric** families first. Do not claim novel lane parallelism, shared frame width or XOR coding.

### Branch 5 — GENERATED STRUCTURE WITH BOUNDED RECURRENCES (high-upside, high false-positive risk)

Prototype *only* a tiny deterministic grammar:
- `RUN(base, delta, n)` over exact fixed-width modular arithmetic;
- `RUN2(base, delta, second_delta, n)` where source actually obeys it;
- `STRIDED_COPY(stride, period, repetitions)` when semantics are exact;
- `LOOP(body, n, sparse_exception_map)`, bounded body and total expansion;
- `FINITE_STATE(seed, transition_id, n)` only for a small audited published operator set.

For `x_i = a + i d mod 2^w`, descriptor size is (operator+width+count+two words); output decode is one vectorizable recurrence or closed-form lane offset. But obvious numeric runs are occupied by delta/DoD/FOR/bitpacking and compressor dictionaries. Program synthesis (Brevis), grammars and minimum BMS already provide strong lineage.

**Do not prioritize PRNG seed recovery** for broad general-purpose Pareto: rare exact generator matches can look spectacular on curated fixtures but require expensive search and readily overfit. Generated/random controls must be prohibited from held-out promotion. Any novel claim needs a semantics-level separator beyond “we added generators to the DSL.”

### Branch 6 — BIT-EXACT NESTED-COMPRESSION RECONSTRUCTION (best retained byte signal; adopt-class)

ANVIL's historical DEFLATE prototype saved approximately **1.36 MB on mozilla** in its reported lane. The relevant modern strong control is **Microsoft preflate-rs** (pre-existing related preflate and precomp). Its algorithm parses a DEFLATE stream, recovers plaintext and compact corrections, predicts original compressor decisions and reconstructs the exact original compressed stream. Thus the category, including re-compression and bit-exact reassembly, is **occupied** prior art.

Investigate **Q7 only under R2 conditions**:
1. exact original producer identity and bit-exact replay—not just an embedded zlib signature;
2. whole-file controls **precomp→xz -9e** and **precomp→Brotli q11/lgwin30**, with frozen reference producer builds;
3. overhead of reconstruction parameters, patch stream and scanning; additional decompression/recompression cycles and peak memory;
4. malformed stream, zip-bomb/resource ceilings and decompressor independence;
5. potential direct reconstruction of codec bitstream decisions without reinflating large plaintext, when semantics permit.

Could create a legitimate domain-specific Pareto point **if** it decreases bytes enough with acceptable decode cost. Do not present as a new universal compressor or new mechanism.

### Branch 7 — REPLACE OR SEVERELY CONSTRAIN THE BWT HOT PATH (high potential, costly development)

The retained auxiliary inverse-BWT optimization is substantial but even after it, external decode is multiple times faster and RSS lower. A hypothetical infinite LF-walk speedup is bounded by other stages (Amdahl). No fresh evidence establishes current stage shares.

Decision branches:
- **B7a** keep BWT **only** in the high-ratio archival profile, where decode price is explicit;
- **B7b** apply typed/field-local transformation then use faster LZ backend, compare directly to transformed-BWT on exact same region;
- **B7c** consider bounded independent inverse regions only when Q2 geometry/extra reset cost is understood and a stage-attribution experiment would change that decision;
- **B7d** cut transient memory by stream-fusing stages and reusing scratch if a source-level liveness analysis proves safety; these are engineering, not fundamental novelty.

Do **not** fund another generic inverse-BWT scheduler replaying Q4/aux. The major question is whether a cheap front-end can move enough bytes into a backend that does not require inverse BWT at all.

### Branch 8 — NEW ENTROPY-CODING INTERFACE, NOT “NEW ENTROPY THEORY” (adopt-class)

The metadata streams and residual symbols need not all be entropy-decoded uniformly. Consider:
- cheap fixed-width packs for near-uniform metadata;
- bucketed offsets with independent low-bit packing (OpenZL's 2026 V-optimal scheme);
- PivCo/PHA on symbol streams that justify its block/table costs;
- interleaved rANS where distribution skew dominates and stream length is sufficient;
- variable bitpacking and exception patches;
- raw literals when both headers and entropy CPU dominate savings.

Investigate a **cost-tuple per leaf**, not unvalidated global switching. Save complete bytes, encoder overhead, decoder cycles, code size and memory for every leaf. An adaptive portfolio is already well occupied by OpenZL and BtrBlocks. The interesting question is whether these choices let a *different reconstructive representation* cross the frontier.

### Branch 9 — DISTRIBUTION-SHIFT-AWARE SELECTOR (important reliability, not novelty)

OpenZL's **September 24, 2026 Compression Transformer** automatically builds compression graphs with a small encoder-side neural scorer, deterministic validity guards and the same universal decoder. Its reported 868 numeric families / 34,737 files are a serious contemporary benchmark for per-stream adaptation. Thus “neural or automatic codec DAG selection” is **not** a defensible ANVIL novelty separator.

ANVIL should prefer a deterministic, bounded candidate filter first:
- exact capability/type signatures;
- sampling that preserves tail/outliers;
- cheap admissibility validation;
- *actual compressed payload* and full envelope for finalists;
- raw fallback;
- strong out-of-distribution abstention, especially after G3/V1 and G5A/V1 negatives.

A future encoder-only learned ranker could improve selection cost, with deterministic proof/measurement of the final candidate and no model in the decoder. **Do not repeat G4**'s already failed exact-cost proxy under a new name: any replacement must have a specified distinguishing premise and a new preregistration.

### Branch 10 — EXPLANATION-GAP ATTRIBUTION AS THE ALLOCATION ENGINE (highest information value)

The earlier `I10-EXPLANATION-GAP-ORACLE-DESIGN.md` already prescribes a hierarchy of bounded oracles. Implement only after source Gate A, and prevent it from becoming an unbounded benchmark factory.

For each immutable region, obtain:
- baseline route, actual bits and token/inverse-transform fraction;
- O1 statistical model gap (predictive cross-entropy distinguished from emitted bits);
- O2 exact/near-exact causal LZ gap;
- O3 bounded bidirectional/BMS/grammar relation;
- O4 typed/affine/innovation gap;
- O5 optional offline neural surprisal gap;
- **new decoder-cost attribution**: output cycles, bytes of dependent random reads, scratch and critical path.

The allocation decision becomes: *find family with a sufficiently large **complete-bit** prize AND inexpensive feasible decoder*. Allocate CI only after its potential worst-case advantage could escape an actual front cell. Never convert an oracle bound into a product result.

---

## D. Candidate mathematical ideas to investigate, with explicit limitations

### D1. Decoder-work-aware minimum description length (W-MDL)
For an exact candidate program `p`, let `ℓ(p,x)` be **actual serialized bits** and `c(p,x)` be measured/frozen-calibrated decoder work. Seek the nondominated set
`P(x) = nondominated{ (ℓ, c, peak_scratch, binary_support_cost) }`.
No globally fixed bits-per-cycle exchange rate. Use dominated-state pruning only when state equivalence and cost additivity are **proven**. Adaptive entropy tables and shared dictionary costs violate naive per-segment additivity. A search approach could maintain an epsilon-bounded frontier of a small *verified* candidate set; it is not a universal safe bound or a guarantee of global optimum.

### D2. Transformation-invariant indexing
For a transformation family `G`, develop a canonical signature `I(x)` satisfying `I(g(x))=I(x)` and ideally rejecting unrelated candidates at high probability. PNRA already instantiated translation-invariant event anchoring, and prior-art work covers parameterized/affine matching. Research value lies in useful **new relation classes**, not just a faster hash map.

Proposed verification: candidates per MB, bytes explained per verified event, false-positive rate, total encoder work, exact residual+descriptor bytes, decoder locality. Avoid an all-pairs polynomial regression engine.

### D3. Robust sparse-innovation coding
For changed lane widths `w_i`, compare shared-width packing with a base-width+exceptions policy. Each policy has a **computable complete-bit cost**, and SIMD decode does not need unpredictable per-symbol branches if masks are extracted in blocks. Risk: escape maps consume more bits than they save; high-entropy outlier positions destroy benefit.

### D4. Affine and low-rank cross-column exact predictors
Candidate reversible modular predictor for row `i`, field `j`:
`\hat x_{i,j} = (a x_{i-1,j} + b x_{i,j-1} + c) mod 2^w`,
where a,b,c belong to a *small* decoder-supported set. Store exact modular residual and exact schema boundaries. This is generalized delta/cross-field coding, likely occupied prior art. Seek evidence where it beats delta/DoD/Gorilla/ALP and raw LZ; reject if metadata, field detection or scatter dominate.

### D5. Shallow generative span graphs
Each phrase depends on at most `d` prior phrases and each decoder operation has bounded expansion cost. Track critical-path depth, source distance and strided reads. The decoder schedule may group same-op spans into SIMD batches. This is an optimization research question about **dependency geometry**, not a license to assert novelty over BMS/grammar/SLPs/OpenZL/Brevis.

### D6. Cost-normalized explanation yield
`Y = ΔB / (decoder_cycles + κ·cache_line_misses)` can help sort ideas **within one explicitly calibrated machine/workload**, but κ is a calibration coefficient, not a universal law. The actual acceptance test is nondominance, not maximum Y. Also track total bytes moved because a single LF/cache miss penalty can exceed many arithmetic instructions.

### D7. Canonical exact reconstruction at byte level
For arbitrary untyped sources, treat positions as byte offsets and widths as explicit wire facts. Named structured modes must restore original exact ordering and tokenization. No lossy numeric normalization, no normalized JSON serialization, no “almost exact” semantic equivalence.

---

## E. Triage matrix — what deserves research, what deserves no CPU budget

| Candidate | Evidence-strength today | Potential role | Main risk | Best next decision |
|---|---|---|---|---|
| R2 Gate A / XRUN | Prepared but unverified | Mandatory trustworthy substrate | Unpublished source/identity | Narrow approved clean publication then zero-codec XRUN |
| Q2 2×2 geometry | Prepared, no remote ruling | Remove unfair window confound | Baseline drift | Exact frozen G0 byte preflight before factorial |
| Fast LZ + efficient metadata / PivCo / OpenZL | External strong prior art; ANVIL untested integration | Practical fast Pareto point | Ratio and decoder code overhead | Future paired established-backend comparator |
| Shallow Work-MDL span IR | Earlier ANVIL designs, no validated full gain | High-value fundamental direction | Novelty overlap and code overhead | First bounded explanation-gap attribution |
| Innovation factorization | G3/G5A discovery benefits + negative V1 | Niche structured improvement | Poor generalization/reordering | Source-independent held-out and coherent-permutation controls |
| Middle-out-inspired tiled leaf | Upstream author-only throughput; no ANVIL result | Ultra-fast typed numeric leaf | Heavy outliers/ISA/seed cost | New numeric held-out then exact ALP/Gorilla/Chimp comparison |
| Generic polynomial/PRNG generation | Hypothesis; prior art | Rare special data | Overfitting/unbounded encoder search | Only if independent oracle identifies large natural frequency |
| Q7 preflate-style exact compressed replay | Historical ~1.36 MB mozilla signal | Domain-specific byte gain | Predictor/producer CPU + prior art | Producer-replay plus xz/Brotli dual-bar prerequisites |
| Further inverse-BWT tricks | Aux already measured; stage shares unknown | Incremental archival engineering | Amdahl & RSS | Conditional decisional stage attribution only |
| Raw context/entropy family swap | Old negative/weak route | Leaf infrastructure | No major ratio mechanism | No standalone novelty campaign |
| G4 proxy/G5B ordinal/G5D old page policy | Frozen negative/closed | Historical lessons | Retest-by-renaming | ZERO additional CI as constituted |

**Qualitative ranking:** (i) publication and Q2 as knowledge-gates, (ii) fast reference engine as practical Pareto vehicle, (iii) bounded reconstruction IR plus explanation gap as strategic differentiator, (iv) Q7 when source controls exist, (v) numeric/structured lanes only with independent evidence, (vi) BWT microoptimization only when stage data decisional. Rankings reflect expected research value **not measured improvement probabilities**.

---

## F. Recommended target architecture, without replacing frozen production formats

```
Exact source bytes
  -> provenance/role guard
  -> cheap event statistics and type hypotheses
  -> candidate families (RAW, conventional LZ, bounded typed-IR, exact compression replay)
  -> semantic proof / exact inverse availability
  -> real backend size for finalists (header/residual/schedule included)
  -> decoder-work + memory feasibility checks
  -> preserve nondominated candidate modes
  -> emit a single bounded versioned archive
       -> simple opcode/stream dispatcher
       -> parallelizable homogeneous reconstruction kernels
       -> exact source byte output
```

**Important:** this architecture is an ideation target, not a new uniqueness claim. OpenZL already supplies graph compression, a universal decoder and encoder-only graph selection; Brevis already supplies bounded program synthesis and fast reconstruction. ANVIL needs a demonstrably valuable narrower semantic contribution and competitive measured economics.

Suggested isolated code boundaries (future only; NOT implementation now): `CandidateSource`, `ExactInverseContract`, `BoundedReconstructionProgram`, `DecoderCostTrace`, `WireAccountant`, `ReferenceComparator`, `FrontierCellEvaluator`. Each must be unit-testable using tiny non-CPU-heavy fixtures before remote benchmarking; no unbounded runtime VM.

## G. Pre-registration discipline for future single-agent GitHub Actions research

**Do not dispatch yet.** R2 Gate A publication, exact source attestation and specific experiment authorization remain binding.

Every future serious experiment should preregister:
1. One falsifiable hypothesis and target Pareto reference cell, not a laundry list.
2. Exact immutable source SHA, workflow SHA, producer SHA, manifest and corpus role graph.
3. Independent held-out family **not** previously consumed; never self-synthetic or known-stress as promotion.
4. Same source bytes/serialization, explicit windows/reset/block geometry, identical backend for causal transform tests.
5. Exact complete output bytes, header/table/framing/masks/index/side streams, startup time, encode/decode MB/s, cycles/source byte where supported, peak RSS, code size.
6. CPU model, ISA dispatch, OS, affinity, runner image, compiler flags, repetition and interleaving/order policy, dispersion and confidence criteria appropriate to that claim.
7. Bitwise roundtrip for all modes plus malicious/malformed lengths, output/work cap, recursion, integer boundary/overflow, NaN and endian cases.
8. Named established controls: no new LZ mode without Brotli/Zstd/OpenZL; no new numeric mode without middle-out/Gorilla/Chimp/ALP/FastLanes; no nested compressed stream mode without preflate/precomp; no program DSL without Brevis/grammar.
9. Exact go/no-go policy and evidence roles fixed before consumption, *no moving negative gates*.
10. Failure-path artifact uploads, source identity in every CSV/JSON, and objective statement of what the run **cannot** establish.

**Experiment design:** first tiny deterministic producer-vs-reference byte attribution, then a correctly paired real decode/RSS study only when byte ceiling and model predict decision relevance, then new independent held-out replication. Existence of a positive discovery result does not automatically grant follow-on dispatch.

### Experiments in dependency order

- **E0 / R2 infrastructure:** approved publication -> Q1a-XRUN. Its expected `CORPUS_BLOCKED` is a valid source-reproducibility outcome, not held-out approval.
- **E1 / geometry:** Q2 strict G0 equality, then G0/G1/G2/G3 exact factorial; pure signed bytes, no crossing tokens.
- **E2 / safety:** Q8 synthetic output/work amplification with explicit caller ceilings after Gate A.
- **E3 / independent coverage:** c9 byte-verified containment + fresh independent held-out family collection, pursued separately from Gate A.
- **E4 / practical decoder path:** controlled baseline fast token+metadata backend on matched windows and complete axes, after specific new authorization; no automatic assumption it can beat q11 bytes.
- **E5 / explanation map:** candidate bit/source-region attribution, including decoder-work ceiling; use bounded samples, no promotion from oracle.
- **E6 / conditional adopt-class:** Q7 dual-control producer identity; Q4 stage breakdown only if decision-dependent.
- **E7 / ambitious semantics:** only the strongest surviving bounded reconstruction candidate, with same-transform controls and locked held-out families.
- **E8 / numeric:** optional middle-out tiles when a genuine independent numeric family and CPU-ISA runner are available.

## H. Distinct research-outcome branches

**Outcome A: Q2 geometry accounts for much of the small-block compression deficit.** Pursue fair matched geometry on conventional fast backend, then paired decode. Do not use Q2 itself to reclassify historical gaps.

**Outcome B: strong exact LZ/entropy controls explain most of residual.** Stop funding generative DSL novelty. Invest in fast decode and backend correctness.

**Outcome C: typed structural oracle explains significant independent bits and decoder work is cheap.** Prototype the smallest possible distinct typed operation plus a same-semantics baseline. Prove the new explanation survives the strongest nearby prior-art control.

**Outcome D: only statistical/neural oracle closes the gap.** This favors a high-ratio archival research tier; a product Pareto crossing requires a way to distill the predictability into a cheap finite-state, byte-coded decoder. Do not ship an expensive ML predictor by default.

**Outcome E: BWT retains superior bytes but irreducibly expensive decode/RSS.** Preserve archival high-ratio profile, stop chasing impossible high-throughput equivalence, and seek other frontier cells with a simpler decoder.

**Outcome F: no typed model transfers to held-out sources.** Restrict specialized profiles with honest applicability; do not market broad general-purpose superiority.

## I. Research claims ANVIL must NOT make

- “Faster than Brotli” from an isolated decoder subkernel or from differently configured references.
- “Beats Brotli” as a complete frontier claim because q11 compressed byte totals are lower.
- “Entropy was reduced by a reversible transform” without specifying conditional model or finite-code redundancy.
- “Middle-out invented new SIMD mathematics”; it is an older published XOR/time-series design.
- “ANVIL invented graph-based compression/program synthesis”; OpenZL and Brevis are direct occupied comparators.
- “ANVIL invented lossless DEFLATE recompression”; preflate/precomp and Microsoft preflate-rs are occupied.
- “Novel transformed self-copy” merely by moving BCJ/parameterization/patch semantics between a global and local decoder.
- “Pareto crossing” from 5 unresolved historical gaps, synthetic samples, consumed V1, non-independent corpora, an incomplete RSS axis, or a failed preregistration reinterpreted post hoc.
- Any output from a not-yet-published/identity-attested Actions workflow as an authoritative promotion result.

## J. External source seeds verified/inspected for this ideation (2026-10-07)

1. **OpenZL / native LZ (2026-09-29):** https://openzl.org/blog/2026-09-29-lz-in-openzl/ — very fast LZ decoder, V-optimal bucket offsets, PivCo; external reported throughput only. Strong must-beat fast-engine comparator.
2. **OpenZL Compression Transformer (2026-09-24):** https://openzl.org/blog/2026-09-24-compression-transformer/ — per-input encoder-only small MLP graph selection, validity/depth guards, unchanged universal decoder. Occupies adaptive DAG route.
3. **OpenZL graph paper:** https://arxiv.org/abs/2605.09928 — graph-based modular codec architecture.
4. **PivCo-Huffman (2026):** https://arxiv.org/abs/2606.05765 ; https://github.com/MarcinZukowski/pivco-huffman — SIMD-friendly wavelet-partition Huffman, optional node ANS; adopt-class entropy comparator, upstream calls implementation WIP.
5. **Brevis / Lossless Tensor Compression as Program Synthesis (2026-08):** https://arxiv.org/abs/2608.02162 ; https://github.com/jiekeshi/Brevis — typed DSL, bounded A* guided by encoder-side prior, bit-exact direct program reconstruction. Occupies broad program-synthesis narrative; reported throughput under their own configuration, not ANVIL-comparable.
6. **middle-out:** https://github.com/schizofreny/middle-out — eight-lane XOR/shared-width numeric scheme, author scalar/AVX-512 results; new source seed is separate.
7. **Microsoft preflate-rs:** https://github.com/microsoft/preflate-rs ; https://github.com/deus-libri/preflate — byte-exact embedded DEFLATE reconstruction, with correction stream; strong Q7 prior art.
8. **libsais:** https://github.com/IlyaGrebnov/libsais — existing inverse-BWT/bigram/aux-index substrate, performance/memory tradeoff.
9. **2026 Algorithmic Information Theory Compression Challenge:** https://arxiv.org/abs/2606.17712 — hidden-test, memory/code-size/frontier requirements; relevant benchmark methodology.
10. **ALP/FastLanes:** https://github.com/cwida/ALP ; https://github.com/cwida/fastlanes — typed SIMD-friendly numerical representation controls.
11. **Random access in grammar-compressed strings (ICALP 2026):** https://doi.org/10.4230/LIPIcs.ICALP.2026.86 — grammar space/access trade-offs reinforce need to charge extra structures.
12. **Historical ANVIL documentation:** the `docs/` items named in the header and `docs/swarm-2026-10-02/FROZEN-CLOSEOUT-MATRIX-R2.md` are authoritative for internal measurements and execution policy.

## K. Final verdict and next actions

**Fund experimentally interpretable research, not another broadly unproven compression mechanism.**

**First build *knowledge*:** exact source/CI attestation and geometry fairness. **Then build *a fast decode floor*:** a credible fast LZ/metadata/entropy stack tested against OpenZL and Brotli/Zstd. **In parallel on paper, design the constrained reconstruction IR**, prioritizing invariant candidate discovery, small exact operator set, sparse innovation accounting, cache-aware bounded schedules and full-wire cost. Only grant heavy compute to the one typed explanation that has measurable byte prize and a plausible exit from an actual Pareto cell.

The strongest scientifically interesting target remains: **a new, nontrivially distinct relation class that explains real bytes with a tiny descriptor and predictable near-copy-speed reconstruction, surviving comparison to equivalent established transforms and independent held-out data**. Nothing inspected yet proves such a relation exists at the required scale. That uncertainty is the research problem, not a result to conceal.

**No files except this new Markdown research document should be changed. No experiments, CI dispatches, commits, pushes or delegated workers are implied or authorized by this document.**
