# ANVIL I10 — Adaptive Representation Compiler Frontier Architecture

**State:** living architecture / post-G3 research map  
**Date:** 2026-09-24  
**Production integration authorized:** no  
**Heavy benchmarking:** GitHub Actions only

---

## 0. Mission

ANVIL should not be a fixed transform followed by an entropy backend.

The target architecture is an **adaptive reversible representation compiler**:

```
bytes
  -> discover reversible coordinate systems
  -> generate a small set of representation candidates
  -> estimate complete downstream cost cheaply
  -> verify a bounded finalist set
  -> emit the winning reversible carrier
  -> compress that carrier with a backend
```

Raw input is always a candidate.

The fundamental causal comparison remains:

```
backend(raw bytes)
vs
backend(complete reversible carrier)
```

A transform is valuable only if the complete compressed carrier is smaller,
faster, or otherwise Pareto-superior after charging every decoder-visible byte.

The long-term objective is not merely "beat Brotli on structured JSON."
The objective is to build a representation compiler that can expose redundancy
which a generic backend cannot cheaply discover in its native byte ordering,
while falling back exactly when no useful representation exists.

---

## 1. Evidence accumulated through G0-G3

### G0 — representation can destroy backend locality

A transform may look lower-entropy and still make Brotli worse.

Therefore ANVIL must never promote a transform from:

- transformed byte count;
- zero density;
- entropy alone;
- semantic elegance;
- local leaf size alone.

Only complete-carrier backend measurements count.

### G1 — coordinate-system changes can make the same backend beat itself

Shape/column reordering produced a large same-Brotli win on CROVIA.

This established the central SRS premise:

> choosing a better coordinate system can be more valuable than inventing a
> new entropy coder.

### G2 — exact dictionaries are a real typed mechanism

G2 broadened the representation basis while holding the backend fixed.

The strongest new result was D2 CDISC:

- G1 structured representation lost to raw Brotli;
- G2 EXACT_DICT converted it into a large win;
- integer FOR/delta/DoD did not explain the crossing.

The discovery portfolio still missed its broad aggregate gate.

### G3 — eligibility granularity matters

G3 made structure eligibility regional rather than whole-file.

Discovery run `35947013432` passed its frozen discovery gate:

- D3: 11,228 structured frames + 1 raw residual frame;
- D3 regionized best: 1,240,155 B vs raw Brotli 1,292,757 B;
- four-file routed aggregate: **-5.5984%** vs raw Brotli;
- >=5% structured wins: 2/3.

Held-out V1 was therefore legitimately opened with the frozen G3
implementation. At the time this architecture document was written, validation
was still running.

---

## 2. The architecture now has two distinct layers

### 2.1 Representation compiler

Responsibilities:

1. identify candidate region boundaries;
2. discover structural/semantic coordinate systems;
3. form reversible groups/columns/lane groups;
4. generate eligible leaf representations;
5. preserve exact residual bytes;
6. expose all decoder-visible metadata explicitly.

This layer answers:

> **What representations are legal and potentially useful?**

### 2.2 Planner/runtime

Responsibilities:

1. gather cheap statistics;
2. prune representation families;
3. rank candidate parameters;
4. compose dense representation choices;
5. verify only a bounded set of finalists;
6. account for size, encode cost, decode cost, memory, and metadata;
7. retain raw fallback.

This layer answers:

> **Which legal representation should actually be emitted?**

The research oracle and the production planner are deliberately different.

The research oracle may spend expensive backend calls to establish ground truth.

The production planner must not.

---

## 3. Non-negotiable invariants

1. **Raw fallback is permanent.**
2. **Exact roundtrip is mandatory.**
3. **No decoder-side semantic rediscovery.**
4. **Every representation decision needed for reconstruction is charged.**
5. **Backend(raw) vs backend(complete carrier) remains the causal ratio test.**
6. **A failed parser/typed expert cannot invalidate unrelated bytes.**
7. **A candidate family may be unavailable without making the file unavailable.**
8. **No new mechanism is promoted because it looks theoretically attractive.**
9. **Heavy performance/compression runs stay on GitHub Actions.**
10. **Discovery and held-out gates are frozen before measurement.**

---

## 4. Current active representation basis

The G3 basis is intentionally small:

- `RAW_LEX`
- `EXACT_DICT`
- `INT_FOR`
- `INT_DELTA_FOR`
- `INT_DOD_FOR`
- exact raw residual frames
- whole-file raw Brotli fallback
- frozen G2 whole-file structured control where eligible

This basis should remain frozen for G4 planner research.

No FSST, float, RLE/default, cross-column predictor, shape hierarchy, or second
backend belongs inside G4.

---

## 5. Research oracle versus production planner

The current G3 candidate builder uses isolated Brotli q11 to score candidate
leaf payloads.

That is useful as a ground-truth research oracle, but it is not a viable
production planner.

On D3 the frozen run reports tens of thousands of isolated q11 leaf
evaluations. Candidate construction/scoring alone costs roughly 32 seconds,
while the final local argmin selection itself takes milliseconds.

The architecture must therefore enforce:

> **q11 is a verifier, not a ranking primitive.**

Long-term target:

- O(columns) cheap statistics;
- O(candidate families) analytical/sampled scoring;
- bounded K complete carrier finalists;
- O(1) expensive backend encodes per emitted block/region group.

---

## 6. Planner pipeline

### Stage P0 — structural discovery

Produce:

- exact frame/region boundaries;
- shape/archetype candidates;
- column membership;
- residual regions.

No compression oracle is involved.

### Stage P1 — feature extraction

For each candidate column/group, gather cheap features once:

- occurrence count;
- total lexical bytes;
- min/max/mean token length;
- distinct count and cardinality ratio;
- top-k frequency mass;
- run statistics;
- canonical integer eligibility;
- integer min/max/range;
- monotonicity / delta-width summaries;
- byte histogram / approximate H0;
- sampled prefix/suffix commonality;
- sampled substring repetition;
- optional float lexical/IEEE classification in later lanes.

Features should be vectorizable and cache-friendly.

### Stage P2 — family gating

Use deterministic eligibility/cheap thresholds to prune impossible or
obviously dominated families.

Examples:

- high-cardinality exact tokens -> avoid whole-token dictionary unless sampled
  evidence says otherwise;
- noncanonical integers -> do not build integer leaves;
- zero-run / dominant-default columns -> later RLE/default family;
- float-like columns -> later float family.

Family gating is conservative: uncertain cases remain candidates.

### Stage P3 — family-local parameter search

Only search parameters inside surviving families.

Examples:

- dictionary code width / escape policy;
- FOR group width;
- delta miniblock size;
- FSST symbol table;
- ALP exponent/factor;
- RLE/default exception policy.

This is ALP-style two-stage adaptation:
**choose family first, parameters second.**

### Stage P4 — cheap cost model

Candidate score should approximate complete downstream cost without q11.

Candidate surrogates to evaluate include:

- serialized payload bytes;
- byte-order-0 entropy estimate plus charged metadata;
- sampled lightweight backend score;
- low-quality Brotli score as a bridge oracle;
- calibrated linear/nonlinear cost model over deterministic features.

The score is not trusted until its fidelity is measured against the frozen q11
oracle.

### Stage P5 — dense composition

Do not return to one-substitution-at-a-time greedy search.

G2 showed useful representations can require tens/hundreds of individually
modest substitutions.

The planner should construct dense candidate portfolios:

- RAW-only baseline;
- family-specific dense portfolio;
- best mixed portfolio from cheap local scores;
- optional small number of interaction-aware variants.

### Stage P6 — bounded complete-carrier verification

Serialize only a small finalist set and run the real backend.

Raw is always one finalist.

The production target is a fixed upper bound K independent of the number of
columns.

---

## 7. Decoder architecture

The eventual frontier is not ratio-only.

The representation compiler must be designed for predictable fast decode.

### 7.1 Control/data separation

Keep small control streams contiguous:

- group kinds;
- leaf IDs;
- bit widths;
- exception counts;
- lengths / offsets where required.

Keep bulk payloads contiguous separately.

This lets decode read control ahead of data and reduces branchy pointer chasing.

### 7.2 Vector-sized lane groups

Future structured carriers should test lane groups around SIMD-friendly sizes
rather than assuming one whole-shape global group.

Benefits to test:

- local dictionaries;
- bounded metadata;
- predictable working set;
- independent decode loops;
- natural SIMD bitpacking;
- better adaptation under distribution drift.

The exact lane size is an experimental parameter, not an architectural constant.

### 7.3 Fused homogeneous decode

Adjacent columns using the same leaf family should be eligible for fused loops.

Examples:

- multiple FOR columns;
- multiple dictionary-ID streams;
- repeated default/exception streams.

Avoid a generic per-value virtual dispatch path.

### 7.4 Branchless/simple dictionary decode

Dictionary representation is currently the strongest typed mechanism.

Production dictionary decode should favor:

- fixed-width packed IDs where useful;
- direct indexed decode tables;
- reserved escape values;
- branchless hot paths;
- cold exception side paths.

---

## 8. Representation families after planner fidelity is solved

These are future **separate causal lanes**, not one giant G4.

### 8.1 Exact dictionary + default/exception

Already supported: exact dictionary.

Next natural extension:

```
dominant value
+ bitmap / run map
+ exception values
```

Useful for:

- booleans;
- null-heavy columns;
- status flags;
- sparse categorical exceptions;
- repeated empty/default tokens.

Compare against Brotli of the complete carrier, not against raw payload bytes.

### 8.2 RLE / RLE-bitpack hybrid

Candidates:

- run-length values;
- run-length dictionary IDs;
- RLE/bitpacked definition-style streams.

This mirrors the reason Parquet uses RLE/bitpacking for low-range streams:
representation reduces the alphabet before the general backend sees it.

### 8.3 Integer family

Keep:

- FOR;
- delta FOR;
- DoD.

Add later, separately:

- miniblock-local FOR;
- patched FOR / sparse exceptions;
- Stream-VByte-style control/data separation.

Current NDJSON evidence says integer transforms are not the primary ratio
mechanism. Their main future value may be speed and non-JSON numeric data.

### 8.4 String family

Whole-token EXACT_DICT is proven useful.

High-EV next mechanisms:

1. delta-length byte arrays;
2. prefix/front coding;
3. FSST-like 1-8 byte symbol table;
4. dictionary + FSST residual;
5. common-prefix + exception suffix.

FSST is especially attractive for high-cardinality text where whole-token
dictionary coding cannot help. It should be tested as a separate same-backend
leaf family after G4.

### 8.5 Float family

Do not add float transforms merely because the architecture supports them.

On an appropriate frozen float corpus, test separately:

- byte-stream split;
- Gorilla XOR;
- ALP decimal-to-integer + exceptions;
- ALP-RD/front-bit dictionary style split;
- raw lexical fallback.

ALP's important architectural lesson is broader than its exact encoding:
sample first, choose a family, then select vector-local parameters.

### 8.6 Shape hierarchy

Current exact shape identity is intentionally conservative but can fragment
heterogeneous data.

Future hierarchy:

```
archetype
  -> key/type/arity signature
      -> exact structural variant
          -> exact lexical shape
```

Possible benefits:

- fewer singleton shapes;
- greater cross-record column populations;
- lower planner work;
- larger dictionary support.

This must retain exact reconstruction metadata and be tested separately from
new leaf families.

### 8.7 Cross-column predictors

Only after single-column mechanisms are understood.

Candidate pattern:

```
column B = predictor(column A, ...)
+ sparse residual exceptions
```

Examples:

- timestamps + duration;
- monotonic IDs;
- related coordinates;
- repeated object IDs / parent IDs.

A correlation coefficient is not enough. Promotion requires complete-carrier
byte wins.

---

## 9. Backend policy

### E1 — one backend over one complete carrier

This remains the cleanest causal configuration and should stay the default
research surface.

Benefits:

- Brotli can discover cross-stream correlations;
- simple accounting;
- direct raw-vs-carrier causal comparison.

### E2 — independently compressed semantic streams

Potential later lane:

- structure stream;
- dictionary IDs;
- numeric streams;
- string residuals;
- raw spans.

Do not assume E2 is better.

Splitting streams can destroy useful cross-stream context just as G0 destroyed
backend locality.

E2 requires its own frozen experiment.

---

## 10. Pareto objective

ANVIL must report a frontier, not only smallest bytes.

Primary axes:

- compressed bytes;
- encode throughput;
- decode throughput;
- peak memory;
- decoder/code footprint where relevant.

A planner may expose policies such as:

```
size
balanced
fast
```

but every policy must be derived from measurable candidates, not hand-tuned
labels.

Conceptual planner objective:

```
J = compressed_bytes
  + lambda_decode * decode_cost
  + lambda_encode * encode_cost
  + lambda_memory * memory_cost
```

For scientific experiments, coefficients must be frozen before measurement or
the full Pareto set should be reported instead.

---

## 11. Research sequence

### G4 — planner fidelity

No new representation mechanism.

Question:

> Can cheap scoring reproduce the frozen q11 leaf oracle closely enough while
> eliminating almost all q11 ranking calls?

This is the immediate next lane.

### G5A — ordering attribution

Isolate how much of the gain comes from coordinate/order changes alone.

Hold leaf vocabulary fixed.

### G5B — shape hierarchy

Test exact flat shapes versus explicitly preregistered structural hierarchy.

Hold leaf vocabulary fixed.

### G5C — genuine mixed-validity regional test

G3 D3 contained only one residual frame.

A later corpus must deliberately contain substantial valid + unsupported/malformed
mixture if we want a strong general claim about regional routing.

### G6 — string symbolization

FSST-like/string-prefix mechanisms, one family at a time.

### G7 — default/RLE and specialized numeric/float lanes

Only after planner cost is controlled.

### G8 — interaction/cross-column lane

Predictor + sparse innovation.

### G9 — production planner + decode optimization

SIMD, vector groups, bounded finalists, branchless hot paths.

---

## 12. Promotion discipline

Every new mechanism follows:

1. write causal question;
2. freeze corpus identity;
3. freeze exact reversible carrier semantics;
4. freeze arms;
5. freeze gates;
6. implement standalone research prototype;
7. local compile/selftest only;
8. run heavy discovery on GitHub Actions;
9. freeze implementation SHA;
10. open held-out only if discovery passes;
11. write source-of-record results;
12. only then consider integration.

No production transform ID is allocated from a promising discovery result alone.

---

## 13. External design principles adopted

The architecture deliberately borrows *principles*, not claims:

- **Brotli:** strong generic LZ/context backend is valuable when representation
  preserves useful locality.
- **Parquet:** dictionary, RLE/bitpack, delta binary packing, delta strings, and
  byte-stream split show that representation can make downstream compression
  substantially easier.
- **FSST:** a compact learned symbol alphabet can help high-cardinality strings
  where whole-token dictionary coding is weak.
- **ALP/FastLanes:** sample first, choose an encoding family, then use
  vector-sized local parameters and sparse exceptions.
- **LogPrism:** structure and value redundancy should be considered jointly,
  rather than treating parsing as an independent semantic goal.
- **Grotli/DataCortex-style systems:** structure discovery + columnization +
  typed representation + generic backend arbitration is a productive design
  pattern, but ANVIL requires stricter causal accounting and raw fallback.

---

## 14. What "best" means for ANVIL

There is no useful single claim called "best compression algorithm."

The engineering target is stronger and measurable:

> produce new Pareto points across heterogeneous data classes that existing
> general compressors do not dominate.

That means ANVIL succeeds when, on rigorously frozen corpora, it can repeatedly
offer combinations of:

- smaller output at comparable speed;
- materially faster decode at comparable ratio;
- or a new size/speed/memory tradeoff not dominated by Brotli/zstd/xz and the
  relevant specialized baselines.

The representation compiler is the path toward that target.
