# ANVIL Iteration 10 — Breakthrough Research Program

**Date:** 2026-09-23
**Status:** research plan; mechanism implementation intentionally deferred
**Primary synthesis:** `docs/FRONTIER-RESET-2026-09-23.md`

## 0. Objective

Iteration 10 is not a tuning iteration.

ANVIL has already demonstrated that a portfolio of known and experimental representations can beat the project's measured Brotli/xz byte bar while still losing the complete Pareto contest because its strongest ratio route is too expensive to decode.

The I10 objective is therefore:

> **Find a representation that converts a materially larger fraction of real input bytes into cheap deterministic reconstruction, while keeping the decoder close to memory-copy / dense-integer-codec economics.**

Novelty is a consequence of finding a new useful mechanism. It is not the optimization target.

No candidate enters the production codec because it is elegant, new, or statistically interesting. It enters only if the complete description and execution economics show a plausible frontier movement.

---

## 1. Two frontiers must be tracked separately

ANVIL has historically focused on Brotli/Zstd/xz-class references. That remains the right **production frontier**.

It is not the full information landscape.

The public Silesia compression leaderboard contains modelling-heavy PAQ-class systems with totals around 28 MB, versus ANVIL I9's approximately 46.4 MB ratio portfolio. These are not fair throughput peers and must never be treated as direct product competitors, but their rate establishes an important scientific fact:

> **There is much more predictable structure available in the corpus than ANVIL currently explains.**

I10 therefore tracks two fronts:

### P-front — production Pareto front

Axes:

- compressed bytes;
- encode time;
- decode time;
- peak memory;
- decoder binary size;
- optionally random-access/streaming properties.

Reference families:

- Brotli across meaningful quality/window tiers;
- Zstd regular + high-compression tiers;
- xz/LZMA tiers;
- fast LZ/Deflate anchors where informative;
- current ANVIL checkpoints.

Goal: **cross this front.**

### R-front — research/rate landscape

Examples:

- PAQ/PAQ8PX-class context mixing;
- CMIX-class systems;
- modern neural predictive controls;
- grammar/BMS oracle lower descriptions on bounded windows.

Goal: **measure unexplained information, not compete on runtime.**

A mechanism is especially valuable if it closes a meaningful fraction of the P-to-R byte gap with P-front-class decode work.

---

## 2. I10's central abstraction: explanation yield

For a candidate explanation `e`, define:

- `O_e`: output bytes reconstructed;
- `D_e`: descriptor bytes;
- `R_e`: irreducible residual bytes after the explanation;
- `C_e`: decoder cycles or calibrated work;
- `W_e`: working-set / source-read cost;
- `K_e`: universal decoder-code cost attributable to supporting the mechanism.

A useful primitive needs high **explanation yield**:

`Y_e = (O_e − D_e − R_e) / C_e`

This is not a universal scalar objective; cycles and bytes are different resources. It is a diagnostic for finding obviously poor mechanisms.

The real decision remains Pareto-based.

For each candidate, record:

`ΔB = B_reference − B_candidate`

and ask how much decode/encode/memory/code-size cost the available `ΔB` can justify before the candidate becomes dominated.

---

## 3. The I10 oracle stack

Before defining another wire mode, ANVIL should build a hierarchy of **explanation oracles**. The point is to identify what kind of structure remains, not to ship the oracle.

The oracles should operate on bounded windows or sampled regions when their exact optimization problem is expensive.

### O0 — current ANVIL causal-reference oracle

Measure the best explanation obtainable from the current family:

- exact LZ;
- current sparse-corrected copy;
- TCOPY relation;
- current literal/statistical backend.

Output:

- complete bytes;
- phrase count;
- reference distance distribution;
- residual bytes;
- correction density;
- decoder-op proxy.

This is the baseline explanatory vocabulary.

### O1 — statistical oracle

Use a strong bounded-memory statistical model such as CTW/PPM-class modelling.

Question:

> How much of the residual is still predictable from local history?

Interpretation:

- large gain: statistical context remains under-modelled;
- small gain: stop inventing more ordinary byte contexts.

No production claim follows automatically.

### O2 — bidirectional-copy oracle

On bounded windows:

- LZ77;
- LZRR;
- minimum/near-minimum BMS where tractable.

Question:

> What rate is lost solely because references are required to point backward?

If the gap is tiny, forward-reference complexity is not worth funding.

If large, inspect dependency depth and locality rather than immediately shipping BMS.

### O3 — grammar/generative oracle

Use SLP/RLSLP/ISLP-inspired analysis to find:

- repeated derivation structures;
- iterated bodies;
- affine iteration counts;
- recursively reusable fragments.

Question:

> Is the missing structure copy/paste repetition, or rule-generated self-similarity?

### O4 — transformation oracle

For typed relations:

- relocation-derived executable fields;
- affine integer fields;
- position-derived values;
- XOR/delta bases;
- record-relative fields.

Question:

> Can a cheap transform make two spans identical or nearly identical, and can the transform parameter be derived or cheaply encoded?

### O5 — neural predictive oracle

Optional, offline-only.

Use a strong predictor to map where classical models still assign poor probabilities.

The output of this oracle is **not compressed data**. It is an attribution map:

- positions with large classical-vs-neural log-loss gap;
- context features correlated with that gap;
- candidate rule families explaining it.

A neural gain that cannot be distilled into cheap decoder state is not an ANVIL mechanism.

---

## 4. Priority mechanism hypotheses

### H1 — Invariant Generative Span IR (IGS-IR)

**Priority:** highest research value
**Status:** hypothesis; novelty not established

Pipeline:

1. SIMD Stage-1 scanner emits sparse structural events.
2. Events generate transformation-invariant signatures.
3. Exact signature lookup proposes typed span relationships.
4. Verification computes complete real-wire benefit.
5. Selected relations form a shallow dependency graph.
6. Encoder serializes a bounded resolved schedule.
7. Decoder executes homogeneous kernels over that schedule.

Typed relation families should be deliberately small at first:

- exact copy;
- position-normalized relocation copy;
- fixed-width affine-field copy;
- repeated body + iteration state;
- basis + sparse innovation;
- replay relation for embedded deterministic codec streams.

**Critical separator from generic graph compression:** the hypothesis is not "represent compression as a graph." That is established. The hypothesis is the interaction of event-driven invariant discovery, typed span-generation relationships, and a hardware-oriented bounded-depth execution schedule.

#### H1 kill conditions

Stop if any of the following hold:

- complete description improves <1% on real-corpus target regions versus a strong same-family baseline;
- dependency depth/source locality predicts decode materially worse than the rate surplus can buy back;
- most gains disappear against BMS/grammar/same-transform controls;
- candidate generation requires dense approximate search;
- descriptor entropy dominates the innovation removed.

### H2 — decoder-derived relocation transform copy

**Priority:** highest concrete primitive
**Status:** strong candidate; dedicated prior-art audit required

Start with x86 `E8/E9 rel32` only.

The copied source bytes themselves reveal where the relocation operand lives. Therefore, instead of transmitting a TCOPY transform mask, the decoder can identify transform sites from the source span and apply the exact position-derived adjustment.

Core hypothesis:

> eliminating explicit transform topology makes multi-field transformed phrases economically viable.

This directly attacks the recorded PNRA/TCOPY failure mode without merely making the old search faster.

#### Prior-art boundary

The audit must explicitly include more than BCJ/E8E9.

Google Courgette is close prior art on the **semantic normalization** side: it uses a primitive disassembler to identify internal `REL32`/`ABS32` references, separates pointer targets from ordinary bytes, replaces pointers with symbolic/indexed references, and reconstructs the exact executable through an assembly-like instruction stream before/after binary differencing.

Therefore ANVIL must **not** claim novelty for:

- discovering relocation-bearing instructions;
- converting addresses into a normalized/symbolic representation;
- reconstructing exact machine-code bytes from normalized pointer metadata;
- using normalized executable structure to improve downstream matching/delta coding.

The narrower open question is whether a single-file compressor can economically perform **reference-local transformed self-copy** in which:

- no external basis/previous executable is required;
- no whole-file disassembly/symbol table is required;
- transform sites are deterministically recoverable from the referenced phrase;
- the arithmetic adjustment is derivable from source/destination geometry;
- multiple transformed fields amortize one copy descriptor;
- the output remains stream-compatible with ordinary LZ-style reconstruction.

Those separators define the search target; they do not establish novelty.

Relevant prior art:
- Chromium Courgette design: https://www.chromium.org/developers/design-documents/software-updates-courgette/
- xz/7-Zip-style BCJ filters;
- ZPAQ E8/E9 normalization;
- Microsoft delta-compression/update patents already tracked by ANVIL, including US 7,509,636.

#### Required controls

- exact LZ;
- current TCOPY;
- global BCJ + same backend;
- disassembly/relocation-normalized control where practical;
- reference-local derived-mask transform;
- payload with all descriptors charged.

Do not extend to general x86 decoding until E8/E9 clears the economic gate and the Courgette/BCJ separation is documented mechanism-by-mechanism.

### H3 — restricted iterated span generation

**Priority:** high
**Status:** anatomy first

A deliberately tiny rule family:

`ITER(n, body, state₀, update)`

Possible exact updates:

- `xᵢ = x₀ + iΔ`;
- position-derived field;
- repeated body with sparse exception vector;
- fixed permutation/lane update;
- previous-state recurrence with bounded-width state.

Decoder:

- counted loop;
- fixed operator;
- no allocation;
- no arbitrary branch/jump;
- bounded expansion;
- vector-friendly field update.

This is the practical test of the morphism/ISLP lesson without building a general grammar VM.

### H4 — innovation-channel factoring

**Priority:** medium-high
**Status:** architecture hypothesis

STC and ALP illustrate the same systems principle in different domains:

> isolate the small irregular channel so it stops poisoning the large regular channel.

ANVIL should test a generic detector for cases where a small typed field class has high conditional surprise while the surrounding carrier becomes much more regular once the field is factored out.

Candidate types:

- digit/numeric runs;
- fixed-width counters;
- timestamps;
- floating-point decimal-like fields;
- relocation operands;
- length/value pairs.

The gate is **not** whether a field detector works. It is whether:

`L(main_channel) + L(innovation_channel) + L(metadata) < L(original)`

under the same downstream backend and with decode work charged.

### H5 — representation-specialized integer metadata

**Priority:** enabling infrastructure
**Status:** no novelty claim

ANVIL currently converts many integers to varint-byte streams and then applies byte entropy coding.

Replay current token streams through:

- FOR + bitpack;
- delta + bitpack;
- patched FOR;
- Stream-VByte-style control/data split;
- sparse exceptions;
- raw fixed width when rate surplus permits.

No parse change.

This isolates whether some of ANVIL's decode tax is self-inflicted by using a general entropy mechanism for dense numeric metadata.

---

## 5. SIMD / microarchitecture program

Hardware work is split into **representation-enabling kernels** and **post-hoc optimization**.

Only the former receives I10 research priority.

### K1 — Stage-1 structural scanner

AVX2 baseline on x86, scalar fallback.

Per 32/64-byte chunk, compute masks for selected byte classes and structural motifs.

Candidate masks:

- newline / CR / separators;
- quote / escape;
- digits / sign / decimal punctuation;
- zero bytes;
- E8/E9 opcodes;
- high-bit / ASCII classification;
- sampled equality against predicted lane patterns.

Then use bit operations to enumerate only events.

Required metrics:

- bytes scanned / cycle;
- events emitted / MB;
- true useful explanation candidates / event;
- expensive verifications / MB;
- cache footprint.

The final two matter more than the first.

### K2 — vectorized candidate verification

For non-overlap spans:

- 32-byte AVX2 XOR/compare;
- equality/mismatch bitmask;
- TZCNT first mismatch;
- per-4-byte-lane mismatch classification;
- optional batch residual-position extraction.

For overlap/periodic copies, keep a separate semantics-correct kernel; do not force vectorization through modulo-heavy source access unless measurements justify it.

### K3 — integer side-stream kernels

Decode 16/32 values per group:

- width control separate from payload;
- bit-unpack into registers;
- vector prefix/delta reconstruction where dependency permits;
- direct use by semantic kernel where possible instead of materializing a temporary vector.

### K4 — independent-state entropy lanes

Where semantic contexts are independent:

- 4/8-way rANS state interleave;
- table layouts engineered for L1;
- batched renormalization;
- decode into the consumer kernel when the fusion lowers total work.

Do **not** force context-dependent predecessor-byte streams into fake SIMD if it costs more model state/rate than it saves.

### K5 — small-copy specialization

Profile match-length histogram only after the new representation exists.

Potential kernels:

- exact 4/8/16/32-byte non-overlap copy;
- batched adjacent copy ops;
- direct literal insertion;
- fused copy + typed patch.

Large copies remain `memcpy` unless evidence says otherwise.

---

## 6. Dependency depth is a first-class compression cost

A bidirectional/generative representation can reduce phrase count while creating a bad execution graph.

For each oracle relation graph, report:

- maximum dependency depth;
- mean depth;
- fraction of output by depth;
- source distance distribution;
- source cache-line reuse;
- fan-out;
- number of independent nodes available per level.

A representation with modest descriptor growth but many independent nodes at each level may be **more** SIMD/parallel friendly than causal LZ.

A representation with excellent bytes but a long pointer-chasing chain is likely a non-starter.

This is an important new axis:

> **Compress the dependency graph, not only the byte stream.**

---

## 7. Information-attribution experiment

The most informative I10 experiment may not compress anything.

For sampled blocks, compute a per-region attribution table:

| Region | Current ANVIL bits | CTW/PPM bits | BMS/grammar estimate | Transform oracle | Neural log-loss oracle | Interpretation |
|---|---:|---:|---:|---:|---:|---|
| A | | | | | | statistical |
| B | | | | | | copy/repetition |
| C | | | | | | generative |
| D | | | | | | typed transform |
| E | | | | | | near innovation floor |

The purpose is to classify the **kind of unexplained information** before building a mechanism.

This prevents the recurring failure mode:

> observe "compressible" bytes → invent a mechanism from intuition → discover downstream that the wrong structure was modeled.

---

## 8. Corpora and anti-overfitting policy

### Inner loop

Use the existing deterministic ANVIL corpora only for:

- mechanism anatomy;
- regression;
- adversarial controls;
- quick remote scouts.

### Standard real corpora

Use:

- Silesia;
- enwik8;
- real source/binary/log/database files already represented in the project.

### External generalization

Use AITDCC 2026:

- retain A–H as training/development class;
- retain I–P as a **frozen external test class**;
- do not inspect I–P results while tuning a candidate that is meant to claim generalization;
- report decoder binary size because the benchmark explicitly caps it at 1 MiB;
- report peak memory against its 8 GiB constraint.

Even though I–P is now public, ANVIL can voluntarily preserve the original hidden-test discipline.

---

## 9. Remote measurement ladder

All expensive work runs on public GitHub Actions.

### R0 — build/correctness

- clean Linux Release build;
- registry coverage;
- fuzz;
- deterministic hashes.

Already operational.

### R1 — byte-only oracle

Timing ignored.

Use for:

- BMS/grammar optimizers;
- CTW controls;
- transform anatomy;
- relation census;
- theoretical lower-description experiments.

This is the cheapest place to kill bad ideas.

### R2 — scout throughput

- same GitHub runner;
- candidate/control same build;
- CPU affinity;
- interleaved A/B;
- raw repetitions;
- ambient-load gate;
- robust CI.

### R3 — promotion evidence

- repeated clean runs;
- deterministic byte reproduction;
- complete reference front;
- same-transform controls;
- memory/code-size;
- no hidden compiler/reference change;
- claim ruled by Pareto tooling, not prose.

---

## 10. Immediate queue

### I10-0 — remote reference validation — **CLOSED / PASS**

Closure record: `docs/I10-REMOTE-BASELINE-CLOSURE.md`.

1. Canonical Silesia remote run completed successfully.
2. Canonical enwik8 remote run completed successfully.
3. Corpus hashes, roundtrips and deterministic ANVIL bytes validated; `anvil-auto-direct` is byte-identical to frozen I9 per file.
4. Linux zstd/xz differences are recorded as a **new benchmark series**, not a continuation of Windows reference bytes or throughput numbers.

### I10-1 — land high-EV existing work after remote baselines

These remain engineering/adopt-class improvements, not the I10 novelty program.
They must remain causally isolated from one another.

#### I10-1A — `libsais_unbwt_aux` integration — **CLOSED / ADOPT**

- implementation contract: `docs/I10-AUX-UNBWT-INTEGRATION-PLAN.md`;
- final evidence/ruling: `docs/I10-AUX-UNBWT-RESULTS.md`;
- default-off legacy BWT remains byte-identical to frozen I9 per file;
- additive inner-BWT v2 wire fully charges the auxiliary indexes;
- paired whole-decode speedups:
  - dickens **1.364×**;
  - webster **1.795×**;
  - enwik8 **2.339×**;
- combined Silesia + enwik8 auto-portfolio cost: **+22,398 B**
  (**+0.00718026% of source**), with **zero routing changes**;
- final hardened remote fuzz: **480 roundtrip variants / 2,880 mutations /
  24 direct auxiliary assertions**;
- final same-toolchain code delta: **+2,976 B `.text`** and **+8,192 B**
  stripped ELF size.

Ruling: retain both representations. Legacy/default-off is the byte-smaller
size-first point; auxiliary v2 is the faster-decode point. Do not silently
change `--parse=ratio` into a scalarized speed/size objective.

The external same-job reference-cost gate is now closed by GitHub Actions run
`35927623136`: **FRONT-GAP_COST** on both Silesia and enwik8.

- Silesia: candidate/xz decode-time ratio **1.7226** (95% CI
  **[1.6640, 1.8400]**), but peak RSS is **4.572x xz**; candidate/Brotli decode
  ratio remains **3.4474**.
- enwik8: candidate/xz decode-time ratio **3.9065** (95% CI
  **[3.9003, 4.0088]**), candidate/Brotli **5.6642**, and peak RSS is
  **598.9 MiB / 9.043x xz**.

The byte advantage survives, but there is no complete external crossing.
Working-set size is now a measured binding axis.

#### I10-1A.2 — BWT subblock bytes/decode/RSS sweep — **CLOSED / NO DEFAULT CHANGE**

GitHub Actions run `35930672607` completed both frozen corpora successfully.
The sweep confirms that subblock size is a real rate/working-set lever, but it
does **not** justify changing the 128 MiB max-ratio/default point.

Silesia, relative to the frozen 128 MiB result (46,466,339 B; 4.2015 s decode;
248.4 MiB peak decode RSS):

- 8 MiB: 47,365,274 B (**+1.9346%**), 3.7022 s, 124.4 MiB;
- 16 MiB: 46,938,837 B (**+1.0169%**), 4.2232 s, 125.7 MiB;
- 32 MiB: 46,648,642 B (**+0.3923%**), 4.2104 s, 203.4 MiB;
- 64 MiB: byte-identical to 128 MiB, 4.2193 s, 248.4 MiB, internally dominated.

On enwik8, 8/16/32 MiB route away from BWT entirely and cost **+5.4075%**
bytes versus the 128 MiB point; 64 MiB retains the BWT route but costs
**+3.2259%** bytes while reducing peak decode RSS from 598.9 to 411.4 MiB.
The enwik8 timing/Pareto classification is blocked by the sweep's own timing
validity rule, so no speed claim is promoted from those rows.

Ruling:

- preserve 128 MiB for max-ratio/default;
- retain the measured smaller-cap points as explicit rate/memory evidence;
- do not reopen BWT representation research merely to chase working set;
- active representation research moves to the Grotli-derived SRS lane.

#### I10-1B — P4.1 bit-exact DEFLATE reconstruction — **NEXT ENGINEERING / ISOLATED**

- implementation contract: `docs/I10-DEFLATE-REPLAY-INTEGRATION-PLAN.md`;
- first production scope: ZIP method-8 raw DEFLATE only;
- the frozen mozilla census puts 3,132,001 of 3,177,007 DEFLATE compressed bytes in ZIP (~98.6%), while PNG contributes only 45,006 B;
- valid ZIP replay population is 2,289 streams / 2,856,886 compressed bytes;
- use a new ratio transform ID; do not reuse tombstoned transform 3;
- reconstruction semantics must be pinned by the format. Do **not** make exact decode depend on whichever host zlib happens to be installed;
- a pinned replay engine is the shortest adopt-class path for the first causal experiment; decoder code-size cost must be charged explicitly.

I10-1B starts from a new branch/checkpoint only after the I10-1A hardened result
is tagged. No DEFLATE-replay source may be mixed into the I10-1A branch.

#### I10-1C — Grotli-derived representation compiler — **G1 CLOSED / SPECIALIST SIGNAL; G2 NEXT**

This lane tests whether ANVIL can beat a mature backend by changing the
representation presented to that same backend.

G0:

- padded row-major positional vXOR + the same Brotli q11/lgwin30;
- **closed NO-GO** on real NDJSON;
- source of record: `docs/I10-GROTLI-G0-RESULTS.md`.

G1:

- byte-exact lexical shape dictionary;
- SHAPE_ROW versus SHAPE_COLUMN;
- same Brotli q11/lgwin30 for raw and structured candidates;
- no typed value codec;
- frozen public implementation:
  `b82d9c83c9c8528861eb65595fface6605cf0e7a`;
- remote run: `35937406182`.

Measured G1 discovery:

- D1 Amazon: SHAPE_COLUMN **-1.4853%** versus raw Brotli;
- D2 CDISC: raw Brotli wins; best structured arm **+2.4971%**;
- D3 frozen GH Archive excerpt: structured candidate unavailable -> raw fallback;
- D4 CROVIA receipts: SHAPE_COLUMN **-21.7717%** versus raw Brotli;
- four-family routed aggregate: **-1.6564%** versus raw Brotli.

Frozen G1 gate required two >=5% wins and >=3% aggregate improvement, so the
broad untyped base is **NO-GO-G1-DISCOVERY**.

However, D4 is a clean same-backend class-specific crossing. The row/column
ablation shows that homogeneous semantic-position locality, not merely repeated
syntax removal, is the dominant mechanism on that family.

Source of record:

- `docs/I10-GROTLI-G1-RESULTS.md`.

The G1 held-out Sino-US DrugQA corpus was not opened and remains available.

Next:

- preregister **G2 typed column expert**;
- keep raw Brotli permanent fallback;
- retain exact final-Brotli arbitration;
- start with only raw lexical, exact-token dictionary/enum, canonical integer
  FOR/delta/DoD, and narrowly justified default/null coding;
- do not integrate into `src/anvil.cpp` before G2 closes.

This lane is adopt/architecture research. DataCortex, CLP, LogPrism, BtrBlocks,
FastLanes, ALP, Pcodec, FSST and historical columnar encodings are close prior
art for individual mechanisms. Novelty, if any, must live at a higher
explanation/compiler mechanism level and must not be claimed from G1.

### I10-2 — build information-attribution oracles

Order:

1. statistical control (CTW/PPM);
2. LZRR/BMS bounded-window control;
3. iterated/grammar anatomy;
4. typed transform anatomy.

No new production wire format.

### I10-3 — derived-mask executable transform anatomy

E8/E9 only.

Success requires:

- real executable corpora;
- complete payload estimate;
- global BCJ same-transform control;
- multi-field phrase amortization evidence;
- predicted decoder work near copy+few integer ops.

### I10-4 — IGS-IR feasibility

Only if I10-2 shows meaningful structure outside current causal copying.

Build an **offline explanation compiler** first.

It emits:

- relation graph;
- charged descriptor bits;
- residual bits;
- depth/locality metrics;
- no production ANVIL file format.

Only a successful offline compiler earns a decoder opcode experiment.

---

## 11. Falsifiability rules

An I10 mechanism is rejected when:

- its advantage exists only on generated/synthetic corpora;
- a same-transform reference erases the advantage;
- a stronger explanatory oracle shows the mechanism targets the wrong structure;
- complete metadata removes the apparent gain;
- decoder dependency depth destroys its compute budget;
- the candidate needs a huge out-of-band shared model not charged to deployment;
- its best case is already dominated by a simpler known representation;
- the observed gain is below the predeclared threshold and no new causal mechanism is identified.

Rejected mechanisms stay documented.

---

## 12. Definition of an I10 breakthrough

A breakthrough is **not**:

- a new wire ID;
- a new entropy backend;
- a synthetic ratio win;
- a renamed prior-art transform;
- a 2% microbenchmark improvement;
- a bytes-only oracle result.

A mechanism-level breakthrough requires all of:

1. **real-data explanatory gain** that survives complete metadata accounting;
2. **a causal mechanism** explaining why the gain exists;
3. **a decoder implementation path** whose work is compatible with the rate surplus;
4. **generalization** beyond the development corpus;
5. **a real Pareto movement** against an adequately dense reference class;
6. **prior-art separation** precise enough that novelty, if claimed, is defensible.

The target remains ambitious:

> **make more of the file mathematically unnecessary to store, then reconstruct those bytes with hardware-friendly operations whose cost is low enough that the rate gain survives as a true system-level Pareto win.**
