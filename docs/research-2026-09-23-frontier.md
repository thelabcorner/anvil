# ANVIL Frontier Research Memo — 2026-09-23

Status: precursor research/architecture memo only. No local benchmark was run for this memo. Its live recommendations and recovered literature were folded into `docs/FRONTIER-RESET-2026-09-23.md`; use that document for the current synthesized agenda, while this memo remains as detailed provenance.

## 1. Where ANVIL actually is

The current live branch is `blocksplit-exp` at the I9 canonical checkpoint. The important conclusion from I9 is not merely that ANVIL compresses well: the ratio-first whole-file portfolio already beats the recorded Brotli q11 and xz -9e aggregate byte totals on Silesia and enwik8, but it has **zero verified Pareto crossings** because the byte-winning BWT route decodes far too slowly.

Therefore the next breakthrough cannot be defined as “make the existing ratio result a little smaller.” The project needs either:

1. a new fast-decode representation that captures redundancy the current fast backend misses, or
2. a representation-level change that creates enough rate surplus to deliberately spend bytes on decode acceleration.

The I9 decode program also closed several cheap implementation-only escape hatches. CRC/PCLMUL, direct-output allocation and postcoder improvements are real engineering wins, but they do not close the structural BWT decode gap.

Current do-not-reburn constraints remain binding:
- current PNRA candidate injection finds real candidates but does not amortize its real wire cost;
- current (mask,slot) modal-residual coding is closed until a new representation makes the residual topology stationary enough to clear a precomputed break-even point;
- current structural-channel reinforcement does not alter the useful parse;
- RLZ/RePair stream coding is not a default-path candidate in its current complexity class;
- the old small-lambda whole-codec stream budget did not move selection boundaries;
- reciprocal-rANS division replacement lost on the target host;
- global lane transposition destroys useful phrase locality;
- block-local BWT/Brotli routing loses to whole-file routing because of warmup tax.

## 2. The information-theoretic interpretation that should guide ANVIL

Shannon entropy is not a prescription for one compressor architecture. It is a lower bound relative to a source model. The practically important quantity for ANVIL is **conditional information left after all decoder-visible state has been applied**.

If a value X is deterministically recoverable from decoder-visible state S, then:

```
H(X | S) = 0
```

and X should require no residual payload bits. The cost moves into the description of S, the mechanism that derives X from S, and the computation/memory required to execute that mechanism.

That makes the useful engineering objective closer to Minimum Description Length than to “minimize entropy-stream bytes”:

```
J =
    L(mechanism/model)
  + L(residual | decoder-visible state)
  + lambda_d * C_decode
  + lambda_e * C_encode
  + lambda_m * M_working_set
  + lambda_i * I_decoder/code-size
```

The lambdas are Pareto prices, not arbitrary tuning constants. They should be derived from the current reference frontier and byte surplus. A mechanism is valuable only if its complete description plus residual reduces J.

This formalizes the project intuition: **the best stored byte is one the decoder can derive exactly and therefore never receives.**

TCOPY's implicit displacement relation is a clean example. A transformed field whose delta is derivable from the reference distance has zero transform-parameter cost. The core lesson is broader: search for exact, decoder-visible invariants and generative relations first; use entropy coding on the innovation that remains.

## 3. Research that changes the search space

### 3.1 Morphism-based repetitiveness is genuinely different from copy/paste repetitiveness

Navarro and Urbina, *Repetitiveness Measures Based on String Morphisms* (Theoretical Computer Science, 2025), show that deterministic morphism/L-system descriptions can capture regularity that conventional copy-paste repetitiveness measures do not. Their measure can be asymptotically much smaller than the substring-complexity lower bound used for many ordinary dictionary representations. They then combine morphism substitution with bidirectional macro schemes into NU-systems, which can be strictly smaller than either mechanism alone on the same family.

This is strategically important for ANVIL. It says there are strings for which “find another occurrence and copy it” is fundamentally the wrong explanatory language. A stronger compressor must sometimes encode **a rule that generates structure**, not a pointer to structure that already exists.

Reference:
- Gonzalo Navarro, Cristian Urbina, “Repetitiveness Measures Based on String Morphisms”, TCS 1043:115259, 2025. DOI 10.1016/j.tcs.2025.115259.

### 3.2 Bidirectional references expose an offline advantage

LZRR and bidirectional macro-scheme research show that a phrase may safely reference a substring to its left or right as long as the dependency system is acyclic. LZRR guarantees no more phrases than ordinary LZ77 and reported roughly 6% fewer phrases on benchmarks. Computing the smallest bidirectional macro scheme is NP-hard, but KR 2026 work demonstrates practical exact BMS/SLP solving on bounded inputs using ASP modulo acyclicity.

This suggests two uses for ANVIL:

- **oracle use:** exact/near-exact BMS/SLP solving on small windows tells us whether causal-past-only matching is leaving material representational headroom;
- **architecture use:** if the oracle gap is real, investigate a deliberately shallow, levelized reference DAG rather than unconstrained forward pointers.

A production representation should cap dependency depth and schedule operations by level so decoding stays predictable and parallelizable.

References:
- Nishimoto & Tabei, “LZRR: LZ77 parsing with right reference”, Information and Computation 285, 2022.
- “Optimal Dictionary-Based Compression with Answer Set Programming: Encodings and Empirical Analysis”, KR 2026.

### 3.3 Online string-attractor theory tells us where LZ is intrinsically constrained

The 2024 Online String Attractors result connects LZ factorization to the best online strategy for the online attractor problem, while showing an O(log n) separation can remain relative to offline structure on families generated by morphisms.

This is a useful warning: repeatedly improving ANVIL's online-ish temporal matcher can hit a conceptual ceiling. If a corpus contains offline-generative structure, the missing gain may be inaccessible to any search formulation that insists “the explanation must already exist to my left.”

Reference:
- Philip Whittington, “Online String Attractors”, arXiv:2407.15599, 2024.

### 3.4 Large predictive models are compression oracles, not ANVIL's implementation model

“Language Modeling Is Compression” (ICLR 2024) and the 2025 LMCompress line reinforce the equivalence between prediction quality/log-loss and lossless coding efficiency. The important lesson for ANVIL is not to ship an LLM in the decoder. It is that unexplained conditional structure is real compressor headroom.

Use expensive predictors as **diagnostic oracles** on bounded samples:
- estimate how much conditional information remains after the current ANVIL representation;
- localize bytes whose probability becomes sharp under richer context;
- ask whether that predictability can be replaced by a tiny deterministic mechanism.

If a huge model predicts a region almost perfectly, the research question becomes: what small exact structure is the model exploiting?

References:
- Delétang et al., “Language Modeling Is Compression”, ICLR 2024 / arXiv:2309.10668.
- Li et al., “Lossless data compression by large models”, 2024–2025.

### 3.5 OpenZL closes the generic "compression graph" novelty lane

Meta's OpenZL formalizes compression as a directed acyclic graph of modular codecs/transforms carried in a self-describing wire format and executed by a universal decoder. Its architecture explicitly makes data structure visible, composes transforms/codecs, and optimizes specialized plans while keeping one decoder.

This is directly relevant and changes ANVIL's novelty boundary. **A DAG of compression operations is not a novel ANVIL contribution.** ANVIL should study OpenZL as both prior art and a high-performance reference architecture.

The remaining research question is narrower: can ANVIL discover **intra-file generative relations between output spans** — particularly exact relations exposed through transformation-invariant structural events — that ordinary modular codec composition, LZ copy/paste, and known grammar/macro representations do not capture economically?

References:
- Collet et al., “OpenZL: Using Graphs to Compress Smaller and Faster”, arXiv:2605.09928, 2026.
- facebook/OpenZL, current open-source implementation.

### 3.6 STC is a fresh example of structure exposure beating a fixed backend

The 2026 STC work on BWT-family text compression separates digit runs from their surrounding text, replaces them with unambiguous placeholders, and sends the digits through length/context-conditioned side streams. In its same-coder enwik9 ablation, this reversible decomposition removes about 2.63 MB relative to leaving the digits in the main stream.

The mechanism is domain-specific, but the lesson is general and highly relevant to ANVIL: **heterogeneous variation can poison the model of otherwise regular structure; separating the innovation channel can improve both the main representation and the residual representation.** This is conceptually adjacent to sparse correction and typed structural-event extraction, but implemented as a deterministic decomposition rather than approximate-reference search.

Reference:
- Du, Shen, Xiang, “STC: Reversible Digit-Context Decomposition for BWT-Family Text Compression”, arXiv:2606.03570, 2026.

### 3.7 CTW/MDL remain the right statistical controls

Context Tree Weighting efficiently mixes an entire bounded-memory context-tree model class with finite-sequence redundancy guarantees and linear-time/storage complexity. It is a better statistical-control oracle than hand-picking one order-N context.

ANVIL should use CTW/PPM-class controls to answer “is this region mainly statistical redundancy or structural/generative redundancy?” without assuming the production codec should adopt CTW.

References:
- Willems, Shtarkov, Tjalkens, “The Context-Tree Weighting Method: Basic Properties”, IEEE TIT 41(3), 1995.
- Barron, Rissanen, Yu, “The Minimum Description Length Principle in Coding and Modeling”, IEEE TIT 44(6), 1998.
- Shannon, “A Mathematical Theory of Communication”, Bell System Technical Journal, 1948.

## 4. Hardware architecture implications

### 4.1 SIMD has to influence the wire format

Stream VByte is the canonical lesson: separating a compact control stream from bulk data enables SIMD decoding that a conventional byte-interleaved format cannot recover later through compiler tricks. Modern TurboPFor extends the same philosophy across FOR, delta, delta-of-delta, zigzag and hybrid integer packing.

ANVIL metadata streams — lengths, distance classes, patch positions, masks, offsets, rule IDs — should not automatically go through general entropy coding. Candidate representations should include:
- raw;
- bitpack/FOR;
- zigzag + bitpack;
- delta/delta-of-delta + bitpack;
- control/data-split group varint;
- rANS/Huffman only where their density repays their serial/table cost.

This is adopt-class engineering, but it may be enabling technology for a novel representation by making richer metadata cheap enough to exist.

References:
- Lemire & Boytsov et al., “Stream VByte: Faster Byte-Oriented Integer Compression”.
- TurboPFor project, current 2026 implementations.

### 4.2 Multi-lane rANS should be treated as an implementation primitive, not novelty

ANVIL's ordinary rANS loops remain fundamentally scalar-state loops. Current rANS implementations demonstrate that independent states can be interleaved and decoded with SIMD. CRAM 3.1 uses up to 32 states; modern AVX2 rANS implementations similarly exploit many independent lanes.

For ordinary ANVIL metadata streams, a future wire version should evaluate 4/8/16/32-way state interleave. This is especially appropriate for non-contextual streams.

The context-switched literal rANS path is different: previous-symbol context creates a true serial dependency. It cannot simply be “AVX2'd” without changing the model. Possible research variants must explicitly trade context semantics for lane independence rather than pretending the dependency is gone.

### 4.3 Cache footprint belongs in the entropy decision

Current K=12 context-rANS with 4096 slots per context creates a decode table footprint around the L1-data-cache scale before other hot state is counted. Smaller 256/512-state tables can turn tens of KiB into a few KiB and may win despite slightly worse quantization.

Therefore entropy precision must be selected by complete cost:
`bits + table build + decode cycles + working-set pressure`.

The existing ANVIL precision/work-adaptive idea is correct; the next version should make cache footprint a measured first-class term instead of an annotation.

### 4.4 The event scanner should be SIMD-first

PNRA's deepest idea was not its particular x86 relocation token. It was the inversion of search:

```
structural event
  -> invariant signature
  -> historical equivalence-class lookup
  -> candidate
```

instead of:

```
every byte
  -> generic candidates
  -> approximate verification
  -> infer transform
```

simdjson's two-stage parser is an existence proof for the hardware pattern: use wide comparisons/masks to cheaply locate structurally meaningful bytes, then perform more expensive work only at those positions.

A generalized ANVIL Stage 1 should scan with AVX2 on Zen 3 and equivalent SIMD elsewhere for events such as:
- ASCII structural delimiters / quote transitions;
- digit and sign classes;
- aligned word-change masks;
- likely fixed-width numeric lanes;
- x86 E8/E9 and other decoder-safe instruction-field signatures;
- repeated separator motifs / line and record boundaries;
- zero/nonzero and byte-class masks.

The scanner emits sparse event records. Typed candidate indexes consume those records. Search work should scale with information-bearing structural events, not blindly with every byte.

### 4.5 Zen 3 target reality

The workstation reference CPU is Zen 3. The architecture target should therefore assume AVX2-class SIMD, not AVX-512. AVX-512 can be an additional path for newer CPUs but must not be the only high-performance design.

Useful x86 primitives include:
- AVX2 compare/shuffle/movemask for event detection and control decoding;
- BMI/BMI2 bit extraction/deposit where profitable;
- POPCNT/TZCNT for sparse masks;
- PCLMULQDQ for CRC/polynomial work (already validated by ANVIL);
- wide unaligned loads with careful overread bounds;
- software pipelining/interleaved independent states to occupy multiple execution units.

The project already has direct evidence that hardware specialization matters: the PCLMUL CRC path produced a multi-fold store-path improvement without changing one compressed byte.

## 5. Candidate breakthrough program: Invariant Generative Span IR

Working hypothesis, **not a novelty claim**.

The generic graph idea is closed by OpenZL, while bidirectional macro schemes, SLPs and NU-systems already establish graph-like/generative dependencies inside a string. Generalized deduplication already establishes basis + deviation coding. Preflate-style systems already establish “predict/reconstruct a representation and store corrections.”

So the candidate ANVIL contribution must be the **interaction**, not any one noun:

> discover sparse structural events with SIMD; derive exact transformation-invariant signatures from those events; use those signatures to find typed generative relationships between spans; encode only the innovation that the relation cannot reconstruct; then compile the selected relations into a bounded-depth, hardware-oriented decode schedule.

Working name: **Invariant Generative Span IR (IGS-IR)**.

The IR would contain a deliberately small operator vocabulary:

1. literal / entropy-coded anchor span;
2. exact COPY;
3. exact transformed COPY whose parameter is decoder-derived where possible (TCOPY family);
4. basis + sparse innovation/correction;
5. exact fixed-width recurrence such as delta, delta-of-delta or affine step;
6. bounded grammar/morphism expansion where it wins complete cost;
7. specialist reversible reconstruction operators such as bit-exact DEFLATE reconstruction.

The important separation from OpenZL is that OpenZL's graph primarily composes codecs/transforms over streams, whereas this hypothesis concerns **relationships among output regions of one object**. The important separation from BMS/SLP/NU is that references are **typed by exact decoder-cheap transforms** and are intended to be discovered through transformation-invariant structural events rather than raw substring search alone. The important separation from generalized deduplication is temporal/intra-object generative referencing plus multiple exact relation families rather than one basis/deviation mapping.

None of those separators proves novelty. They define the prior-art audit ANVIL must now perform.

### 5.1 Hardware constraint is part of the representation

The selected explanation is compiled into a shallow dependency schedule. Independent destination spans at the same depth can be grouped by operator and width, allowing tight loops over homogeneous work rather than a branchy token interpreter.

The research version should cap dependency depth aggressively. If forward/bidirectional references buy bytes, the price of dependency depth, random source reads and lost streaming must be explicit in J. A smaller grammar that serializes the decoder is not automatically a better ANVIL point.

### 5.2 The breakthrough test

IGS-IR deserves implementation only if the offline oracles show all three:

- meaningful description-length headroom beyond causal exact-LZ on real data;
- that headroom is attributable to a small family of exact relations discoverable from sparse events/invariants;
- the resulting operations have enough arithmetic intensity/locality to decode near memcpy/bitpack class rather than context-mixing/BWT-walk class.

If any leg fails, stop before integrating a new wire mode.

## 6. The immediate research oracles

Before a production implementation, use expensive offline tools on small samples to answer four questions:

### Oracle A — repetition representation gap
Compare current ANVIL parse cost with:
- LZ77 phrase count/cost;
- LZRR;
- exact/near-exact BMS on bounded windows;
- SLP/RePair;
- small L-system/NU-system approximations where tractable.

Question: is causal copy-paste the limiting representation?

### Oracle B — conditional-entropy gap
Run CTW/PPM-style and optionally model-based predictors on the residual/literal regions left by ANVIL.

Question: are the remaining bytes statistically predictable but structurally unexplained?

### Oracle C — typed relation census
For high-residual regions, search for exact relations:
- constant/affine word deltas;
- repeated transform masks;
- position-derived values;
- periodic record-relative fields;
- morphic substitutions;
- forward-reference opportunities.

Question: can a cheap exact decoder operation explain the model's predictability?

### Oracle D — decode economics
For each proposed operator, estimate before implementation:

```
gross bytes explained
- operator/header cost
- residual cost
- dependency/schedule cost
= net byte value
```

and separately:

```
cycles per emitted byte
working-set footprint
branch count / predictability
parallel width
```

If the economic ceiling cannot move the current reference frontier, stop before building the codec.

## 7. Recommended next research order

1. **Integrate the already-proven aux-unBWT concept only as a control/engineering branch**, because its measured gain is large and cheap, but do not mistake it for the breakthrough.
2. **Land/finish P4.1 DEFLATE reconstruction** as an adopt-class capability. It is one of the rare measured byte wins that does not worsen the BWT decode axis.
3. **Build the offline representation-oracle suite** (LZRR/BMS/SLP/CTW controls) on small real-corpus windows.
4. **Implement a SIMD structural-event census tool**, not a compressor. Its output is a research dataset of event density, invariant collisions, candidate span lengths and exact relation classes.
5. **Prototype bounded-depth Reconstruction Graph encoding** only if the oracle suite shows material headroom beyond causal LZ and the event census can discover it cheaply.
6. **In parallel, create hardware-first metadata codecs** (control/data split, FOR/delta/bitpack, multi-lane rANS) as reusable decoder infrastructure.
7. **Only then** integrate a new representation into ANVIL and submit it to the full Pareto gate.

The target is not “novel compression.” The target is a representation in which more input becomes a deterministic consequence of a small decoder-visible explanation, while the explanation itself is cheaper to store and execute than the bytes it replaces.

## 8. Benchmark execution policy

Effective 2026-09-23 for this project:
- do not run compression benchmarks, corpus sweeps, fuzz campaigns, heavy builds or CPU-heavy experiments on the workstation or homelab unless explicitly requested later;
- local work is for source inspection, lightweight edits and orchestration;
- CPU-heavy ANVIL measurements run in GitHub Actions/public CI;
- shared GitHub-hosted runner timing is **scout/ranking evidence**, not automatically citation-grade microarchitectural evidence;
- every Actions result records runner hardware/toolchain/commit and uploads raw results.

See `docs/github-actions-benchmark-protocol.md`.
