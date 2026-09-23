# ANVIL I10 — Explanation-Gap Oracle Architecture

**Date:** 2026-09-23
**Status:** research design; no production codec change
**Execution policy:** CPU-heavy oracle runs execute on GitHub Actions, not the workstation or homelab

---

## 0. Purpose

ANVIL currently has many plausible mechanism families and a growing do-not-reburn
register. The next breakthrough search should not choose another mechanism by
intuition alone.

This oracle answers a more useful question:

> **For the bytes ANVIL still pays to store, what kind of explanation is actually
> missing?**

For each sampled source region, the oracle compares concrete, lossless
representations from several explanation families. It emits an
**explanation-gap map** showing which family, if any, removes the most complete
serialized bits beyond the current ANVIL representation.

The goal is not to build a new production compressor inside this tool. The goal
is to identify where research effort has positive expected value.

---

## 1. Core principle: measure explanatory family gap, not only compression ratio

Suppose a window X currently costs:

    L_anvil(X)

For an oracle family F, define the best concrete candidate found under the
oracle budget as:

    L_F(X)

where L_F includes all bytes/bits required by that oracle representation:

- operator/rule IDs;
- lengths and boundaries;
- references;
- model/table descriptions;
- parameters;
- residuals;
- literals;
- framing;
- any per-window initialization state.

Then:

    gap_F(X) = L_anvil(X) - L_F(X)

Positive gap means family F exposes structure that the current representation
does not exploit.

This is intentionally different from:

- zero-order entropy;
- model cross-entropy without model cost;
- phrase count;
- grammar rule count;
- number of BWT runs;
- an idealized Shannon lower bound;
- an oracle that assumes free side information.

Those quantities are diagnostics. They are not archive cost.

---

## 2. Oracle families

The first version should contain a deliberately small number of distinct
families. The point is to classify missing structure, not exhaust every codec.

### O0 — Current ANVIL control

Purpose:

- establish the exact complete cost being explained;
- record the current route/backend;
- retain source-to-output provenance.

Measurements:

- exact payload bytes;
- route/mode;
- current block/window framing share;
- residual/literal share where introspection is available.

This is the denominator for every gap.

### O1 — Statistical predictability control

Question:

> Is there still substantial sequential predictive information after the current
> structural representation?

Candidate controls:

- CTW-class bounded context model;
- PPM-class model;
- optional large predictive/neural oracle on a **small sampled subset only**.

Requirements:

- report model/table cost separately;
- report pure predictive cross-entropy separately from realizable coded size;
- never call model weights "free" unless the decoder profile explicitly assumes
  them as pre-installed shared state.

Interpretation:

- large O1 gap, small structural gaps -> statistical model is weak;
- large neural-only gap -> information exists but may be too expensive to deploy;
- small O1 gap -> stop inventing order-N entropy coders.

### O2 — Strong causal-reference oracle

Question:

> Is current ANVIL losing primarily because its left-reference search/parser is
> weak?

Use a deliberately expensive causal LZ parser/search:

- large match candidate budget;
- near-optimal/DP parse under exact token costs;
- current and alternate distance/length symbolizations.

The important comparison is not against zstd bytes. It is against ANVIL's own
explanatory vocabulary with substantially relaxed encoder search.

Interpretation:

- large gap -> improve search/parser before inventing a new decoder mechanism;
- tiny gap -> the explanatory family itself is the limitation.

### O3 — Noncausal / bidirectional reference oracle

Question:

> What is the price of requiring every reference source to precede the phrase?

On bounded windows, use:

- LZRR-like safe right references;
- exact/near-exact bidirectional macro schemes where tractable;
- dependency-DAG depth and edge statistics.

Record:

- complete phrase/reference cost;
- dependency depth;
- topological layer count;
- width/parallelism by layer.

A large byte gap with a shallow DAG is especially interesting: it suggests that
a bounded levelized COPY_DAG may be practical.

A large gap that requires deep serial dependencies may be mathematically real
but systemically useless.

### O4 — Grammar / generative oracle

Question:

> Is the region better described by a small generator than by copies?

Initial controls:

- SLP/RePair-style grammar;
- Repeat / periodic generator;
- small affine/integer sequence rules;
- bounded morphic substitutions on small windows;
- restricted iterated rules;
- **PROGRAM_COPY oracle**: reference a contiguous sequence of prior explanation
  operations whose boundaries are already decoder-visible, then transmit only
  changed parameters/residuals. This is motivated by LZBE's factor-aligned
  reference structure, but the object being reused here is an ANVIL explanation
  sequence rather than merely the raw prior bytes.

For PROGRAM_COPY record separately:

- instruction bytes avoided;
- inherited versus overridden parameters;
- underlying output bytes covered;
- explanation dependency depth;
- whether the same gain remains after ordinary entropy-coding of the instruction
  stream.

If instruction-stream entropy coding erases most of the opportunity, do not add
a new decoder reference mechanism.

Every rule must have:

- exact serialized definition;
- exact expansion bound;
- exact output length;
- decoder-work estimate.

Do not use "grammar size" alone as the cost.

### O5 — Typed reversible-transform synthesis

Question:

> Is the apparent entropy mostly a bad coordinate system?

Initial reversible vocabulary:

- XOR/add constant maps;
- XOR/add adjacent scans;
- delta and delta-of-delta;
- zigzag;
- fixed 2/4/8-byte field splitting;
- byte/bit plane splitting;
- range/class + offset;
- frame of reference;
- default + exceptions;
- base + sparse deviation.

This is the natural first implementation target for ANVIL's target-directed
synthesis research because most operators have cheap exact inverse
decompositions.

### O6 — Replay / serialization oracle

Question:

> Are these bytes themselves an encoded artifact that can be reconstructed from
> a simpler semantic object?

Initial high-confidence classes:

- DEFLATE/zlib/gzip using the I10-1B/preflate lineage;
- future format-specific replay only after exact reconstruction semantics are
  established.

Report separately:

- normalized semantic payload;
- predictor/replay metadata;
- correction stream;
- original container/framing reconstruction cost.

A replay win is a valid general-purpose codec portfolio win even if the replay
operator itself is format-specific.

---

## 3. Window selection

Oracle work can become arbitrarily expensive. Window selection must therefore be
designed, not incidental.

### 3.1 Two sampling lanes

Use both:

**Uniform lane**
- fixed-seed windows sampled uniformly from every canonical file;
- prevents the detector from only studying easy/interesting regions.

**Hard-region lane**
- windows where current ANVIL spends unusually many residual/literal bits;
- route boundaries;
- large correction density;
- high model surprise;
- known BWT/Brotli route disagreements.

The uniform lane estimates prevalence. The hard lane maximizes discovery yield.

### 3.2 Window sizes

Start with a geometric set:

- 4 KiB;
- 16 KiB;
- 64 KiB;
- 256 KiB;
- 1 MiB where oracle complexity permits.

Why multiple sizes:

- tiny windows expose local typed structure;
- medium windows amortize rules/dictionaries;
- large windows expose long-range reference/generative structure;
- finite-block sample complexity means conclusions at one scale do not
  automatically transfer to another.

### 3.3 Avoid corpus overfitting

Discovery corpus:

- canonical Silesia;
- enwik8;
- public AITDCC A-H only for mechanisms intended to claim broad generality.

Validation corpus:

- preserve AITDCC I-P as pseudo-hidden during candidate development despite
  their public availability;
- additional external corpora not used to choose the mechanism.

A mechanism that only wins on the discovery windows is evidence for a
format-specific operator, not a general breakthrough.

---

## 4. Exact-cost contract

Every oracle candidate returns a common record:

    {
      family,
      source_offset,
      source_length,
      serialized_bits,
      model_bits,
      instruction_bits,
      residual_bits,
      literal_bits,
      reference_bits,
      framing_bits,
      decoder_work_class,
      dependency_depth,
      working_set_bytes,
      exact_roundtrip
    }

Only serialized_bits participates in the primary byte-gap ranking.

The decomposition fields explain *why* it wins.

### 4.1 No free model state

A candidate must declare one of:

- SELF_CONTAINED — every decoder-required bit is charged;
- PROFILE_SHARED — state is explicitly part of a named deployment profile;
- ORACLE_ONLY — rate is diagnostic and cannot be promoted to a codec claim.

Neural weights, trained dictionaries, and external databases default to
ORACLE_ONLY unless the archive carries them.

### 4.2 No free parse

Encoder compute may be huge for an oracle, but the emitted representation still
must be decoder-complete.

A SAT/ILP solver finding a grammar does not mean the decoder needs SAT/ILP.
That is allowed.

A candidate whose decoder also needs to repeat the expensive search is rejected.

### 4.3 No ideal entropy as final cost

A value like:

    sum -log2 p(x_i)

is a predictive diagnostic.

For promotion, also measure or upper-bound:

- probability-table/model serialization;
- finite arithmetic/ANS precision;
- flush/state bytes;
- block restarts.

---

## 5. Decoder-work classes

The oracle should not pretend to predict exact CPU cycles for mechanisms that do
not yet have kernels. It should classify decoder structure using calibrated
work classes.

Initial classes:

### D0 — Bulk memory

Examples:
- literal memcpy;
- memset/default fill;
- fixed-width copy.

Characteristics:
- regular;
- high bytes/instruction;
- usually memory-bound at scale.

### D1 — Dense SIMD arithmetic

Examples:
- XOR/add maps;
- field merges;
- bitpacking;
- fixed-lane transforms.

Characteristics:
- vectorizable;
- predictable control.

### D2 — Sparse event patch

Examples:
- copy + correction mask;
- exception vectors.

Characteristics:
- dense base operation plus POPCNT/TZCNT-like event enumeration.

### D3 — Independent entropy lanes

Examples:
- interleaved rANS;
- multi-stream Huffman.

Characteristics:
- stateful but exposes ILP.

### D4 — Serial entropy/predictive state

Examples:
- one-state rANS;
- previous-symbol context chain;
- arithmetic coder with serial model update.

### D5 — Pointer/reference dependency

Examples:
- ordinary LZ overlap;
- BWT LF mapping;
- bounded dependency DAG.

Record dependency depth separately.

### D6 — Rule expansion

Examples:
- grammar;
- repeat;
- bounded generator.

Require static output/work bounds.

These are initially ordinal/structural labels. Remote microbenchmarks may later
calibrate per-byte/per-op costs.

---

## 6. Explanation-gap classification

For each window compute the non-dominated oracle candidates and assign one or
more labels.

### STATISTICAL

O1 dominates current representation; structural families do not.

Meaning:
- information remains in probability/context rather than deterministic
  structure.

### SEARCH-LIMITED

O2 strongly improves over current causal representation.

Meaning:
- current match finder/parser is leaving ordinary LZ value on the table.

### CAUSALITY-TAX

O3 materially beats O2.

Meaning:
- right/noncausal references explain bytes that left-reference LZ cannot.

### GENERATIVE

O4 materially beats both causal and noncausal copy families.

Meaning:
- copy/paste is not the right explanatory language.

### COORDINATE

O5 materially improves the residual.

Meaning:
- dependencies become cheap after reversible reparameterization.

### SERIALIZATION

O6 wins.

Meaning:
- the source is largely the output of another deterministic encoder/container
  and should be replayed.

### NEAR-FLOOR

No family materially improves after complete cost.

Meaning:
- do not spend mechanism R&D here without new evidence.

A window may have multiple labels when families are complementary.

---

## 7. Promotion statistics

A spectacular single-window win is not a breakthrough.

For each family/mechanism aggregate:

- fraction of sampled source bytes where it wins;
- total recoverable bits;
- median and p90 gap bits/input-byte;
- maximum complete metadata share;
- number of corpus/files represented;
- stability across window scales;
- discovery-vs-validation retention;
- predicted decoder work distribution.

High-value candidate pattern:

- moderate/large aggregate recovered bytes;
- appears across several independent files/classes;
- explanation metadata is a small share of the gain;
- decode class D0-D3 or shallow D5/D6;
- validation retains the effect.

Low-value pattern:

- giant gain on one signature;
- tiny prevalence;
- deep serial dependency;
- model state larger than saved payload;
- validation collapse.

---

## 8. Interaction between families

The most important future mechanisms may be compositions.

Therefore after the first single-family scan, measure selected two-stage
compositions where the first stage creates a simpler second-stage residual.

High-value combinations:

- COPY + sparse deviation;
- field split + delta/XOR scan;
- range/class + entropy coding;
- grammar/generator + residual literals;
- replay + correction stream;
- transform + causal reference;
- default + exception positions + value-class coding.

Do **not** immediately search arbitrary deep pipelines.

Use the first-pass gap map to authorize combinations.

This is where target-directed synthesis becomes valuable: it can search only
operator compositions whose inverse decomposition is exact for the target.

---

## 9. Search-budget discipline

Every oracle receives a hard budget and must emit:

- candidates considered;
- candidates pruned;
- wall CPU time;
- peak memory;
- search depth;
- best-cost trace over time.

This reveals diminishing returns.

If a family gains only 0.01% more bytes after 100x more search, production
encoder work should not chase that tail unless the archive use case explicitly
permits it.

The breakthrough target is **explanatory family power**, not an unbounded
offline search contest.

---

## 10. Hardware feedback loop

The oracle result should feed the hardware lane.

Examples:

If COORDINATE gaps are commonly:
- fixed-width XOR/add scans -> prioritize prefix/vector kernels.

If SEARCH-LIMITED gaps dominate:
- optimize candidate generation/parser before decoder ISA expansion.

If CAUSALITY-TAX gaps require:
- depth <= 2 and wide levels -> test bounded levelized reference DAG;
- depth hundreds -> likely reject for production despite rate.

If GENERATIVE gaps use:
- Repeat/affine rules -> create dedicated macro kernels;
- irregular deep grammar -> likely decoder-cost problem.

If metadata is dominated by:
- small integers -> prioritize control/data separated integer coding.

The hardware project therefore reacts to measured explanation prevalence rather
than guessing which loop looks fun to vectorize.

---

## 11. Remote execution architecture

All heavy oracle evaluation runs on GitHub Actions.

Recommended workflow:

1. checkout frozen ANVIL candidate and oracle tooling;
2. fetch/hash canonical corpus;
3. deterministic fixed-seed window manifest;
4. run O0 plus cheap anatomy;
5. dispatch family oracles as matrix jobs;
6. roundtrip every concrete candidate;
7. merge records by stable window ID;
8. calculate gap/frontier classifications;
9. upload compact JSON/Parquet/CSV summaries and selected candidate traces;
10. retain raw giant intermediates only when needed for a promoted mechanism.

Do not make one monolithic job. Oracle families have very different runtime and
failure modes.

### 11.1 Deterministic window IDs

Use:

    corpus / file SHA256 / offset / length

This permits independent oracle jobs to merge without ambiguity.

### 11.2 Reproducibility

Every record includes:

- ANVIL source SHA;
- oracle tool SHA;
- corpus hash;
- compiler/runtime version;
- solver/library versions;
- random seed;
- host fingerprint.

---

## 12. Initial stopping rules

The first explanation-gap campaign is a **scout**, not a publication benchmark.

Stop a family early when:

- first N diverse windows show effectively zero complete gap;
- metadata consumes most gross gain;
- decoder dependency class is clearly disqualifying;
- solver search cannot reach useful window sizes;
- result duplicates an already-falsified ANVIL mechanism.

Promote when:

- aggregate complete-byte opportunity is large enough to move the corpus;
- effect survives multiple files/scales;
- a bounded decoder kernel is obvious;
- validation data retains the signal.

---

## 13. Relationship to Explanation Synthesis

The oracle and synthesizer are separate.

Oracle:

    "Which family explains this region?"

Synthesizer:

    "Within authorized cheap families, what exact bounded program best explains
     this target?"

This separation prevents the synthesizer's DSL from growing into a museum of
every idea ever tested.

A family first earns DSL surface area by showing a real explanation gap.

Then the target-directed synthesizer can search compositions inside that
evidence-backed vocabulary.

After synthesis, equality-preserving lowering/e-graphs can choose the best
machine implementation without changing the explanation semantics.

---

## 14. First experiment order

Do not launch all expensive oracles at once.

### Phase A — cheap anatomy

- current ANVIL exact cost/route;
- zero/order-1 entropy diagnostics;
- run/periodicity;
- word-lane change masks;
- simple delta/XOR/field splits;
- strong causal LZ.

This should classify a large fraction cheaply.

### Phase B — expensive structural controls

Only on ambiguous/high-gap windows:

- BMS/LZRR;
- grammar/SLP;
- restricted generator synthesis;
- stronger statistical model.

### Phase C — learned/neural oracle

Only on a small stratified subset.

Purpose:
- estimate information still invisible after classical/structural families;
- identify distillation targets.

Not a production candidate.

### Phase D — candidate-specific validation

Once one operator family repeatedly wins, build a focused prototype with:

- exact wire;
- bounded decoder;
- remote paired timing/memory;
- external validation.

---

## 15. Success criterion

The explanation-gap project succeeds even if it discovers that no exotic
mechanism is currently worth implementing.

A useful result can be:

- "70% of the remaining gap is ordinary causal search quality";
- "noncausal references buy almost nothing";
- "numeric field transforms dominate database windows";
- "grammar oracle wins only on one synthetic-looking file";
- "neural predictor sees 15% more information but no cheap feature explains it";
- "DEFLATE replay accounts for most recoverable bytes in one portfolio class."

That evidence prevents months of mechanism roulette.

The ideal result is stronger:

> a small, repeated explanation family produces a material complete-byte gap,
> maps to a bounded hardware-cheap decoder kernel, and retains the gain on
> validation data.

That is the point where ANVIL should implement a new codec mechanism.
