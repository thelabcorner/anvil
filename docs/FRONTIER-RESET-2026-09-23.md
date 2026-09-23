# Project ANVIL — Frontier Research Reset

**Date:** 2026-09-23  
**Purpose:** Reconstruct the actual post-I9 state, re-ground the project in compression theory and current research, and define the next mechanism-level research program before more codec implementation.

> This document is a research/architecture reset, not a benchmark result. No new ANVIL compression benchmarks were run locally while preparing it. Heavy measurements are to run on GitHub Actions.

---

## 1. Executive synthesis

ANVIL's current problem is no longer "find a better entropy coder."

Iteration 9 established three facts:

1. **ANVIL can already win raw bytes.** The ratio portfolio beats the binding legacy references on the canonical Silesia/enwik8 measurements recorded in `docs/anvil-i9-findings.md`.
2. **ANVIL has not crossed the complete Pareto frontier.** The BWT-heavy ratio path pays a severe decode penalty; the frozen I9 report records zero true frontier crossings.
3. **Several plausible-looking local optimizations have already been falsified.** Current PNRA injection, old mask/modal coding, old structural-channel reinforcement, RLZ/RePair-as-default, and a small whole-codec stream-budget lambda are all in the do-not-reburn register.

The research direction should therefore move one abstraction layer upward:

> **Do not primarily optimize the code that stores unpredictable bytes. Expand the class of bytes the decoder can reconstruct without storing them.**

The key object is not an entropy coder. It is a **decoder-visible explanation**.

If a value is deterministically recoverable from already-decoded state, position, a compact rule, a previous phrase, or a validated reconstruction model, its conditional entropy under that decoder state is zero. The payload should not carry that value again. The only cost is the description of the explanation itself.

That reframes the project as:

`compressed_size ≈ L(explanation_program) + L(irreducible_residual | explanation_program, decoder_state)`

subject to hard constraints on:

- encode work,
- decode work,
- memory traffic,
- working-set size,
- decoder binary size,
- model warmup,
- random-access/streaming requirements,
- robustness and bounded execution.

This is MDL-like reasoning, but ANVIL's objective cannot be bits-only. The *program* must be cheaper to execute than the bytes it eliminates.

---

## 2. Where ANVIL actually stands

Source of record:

- `docs/anvil-i9-findings.md`
- `RESEARCH_LEDGER.md`, PART XIV / A1–A21
- `docs/audit-2026-09-07/06-do-not-reburn.md`
- canonical source checkpoint `9caeee9` / I9 frozen report

### 2.1 Current byte frontier

The I9 frozen report records:

- Silesia input: 211,938,580 B
- ANVIL auto portfolio: 46,446,995 B
- xz -9e: 48,456,100 B
- Brotli q11/lw30: 49,383,136 B

and:

- enwik8 input: 100,000,000 B
- ANVIL auto: 23,534,368 B
- Brotli: 24,810,180 B
- xz: 24,831,656 B

These are real byte wins, not a full Pareto crossing.

### 2.2 Binding failure

The same portfolio is far behind on decode:

- Silesia auto: ~14.6x slower than Brotli q11, ~7.9x slower than xz
- enwik8 auto: ~23.9x slower than Brotli, ~14.1x slower than xz

I9 also closed many cheap decoder-only routes by measurement. This matters: repeating micro-optimizations on the old representation is unlikely to unlock the missing factor.

### 2.3 Kept engineering wins

The most relevant kept wins are architectural clues:

- PCLMUL CRC: ~4.5x encode / ~3.4x decode on the store path, wire-identical.
- `libsais_unbwt_aux` prototype: ~3.4–3.55x inverse-BWT speedup at small wire cost; integration remains pending.
- ALLOC decode path: real but insufficient.
- two-level arithmetic tables + buffered renorm: real but insufficient.

The CRC result is important beyond CRC: **hardware-native dataflow can move an axis by multiples without changing the representation.**

### 2.4 High-value pending work

I9 leaves two unusually clean integration candidates:

1. **Bit-exact DEFLATE reconstruction (P4.1)** — ~1.36 MB recovered on `mozilla` in the prototype, targeting files already routed to fast legacy backends, so it can improve bytes without worsening the BWT decode problem.
2. **Auxiliary-index inverse BWT** — a measured multi-x improvement in the BWT inversion stage, with a small rate charge.

These are worth landing, but they are not the long-term breakthrough mechanism.

---

## 3. Information theory: what Shannon does and does not say

### 3.1 Entropy is conditional on the source/model

For a source (X), Shannon's lossless coding theorem gives entropy as the lower bound on average code length under the source distribution.

That does **not** mean "the byte entropy of this file is the final limit."

For dependent data:

`H(X₁, …, Xₙ) ≤ Σᵢ H(Xᵢ)`

and conditioning can reduce uncertainty:

`H(X | S) ≤ H(X)`

where (S) is decoder-visible state.

If:

`X = f(S)`

deterministically, then:

`H(X | S) = 0`.

This is the formal kernel of ANVIL's "do not store what is reconstructable" idea.

The compression problem becomes the construction of a cheap, shared state (S) and a cheap decoder function (f).

### 3.2 Prediction and compression are the same log-loss problem

Arithmetic/ANS coding turns a probability assignment (Q) into a code length approximately:

`L(x) ≈ −log₂ Q(x)`.

Sequential factorization gives:

`Q(xⁿ) = ∏ₜ Q(xₜ | x^{t−1})`.

So universal lossless compression is equivalent to sequential prediction under log loss.

This is why powerful neural predictors can reach astonishing ratios. It is also why they are not automatically useful to ANVIL: a predictor that saves 10 MB but needs hundreds of MB of model state and millisecond-scale sequential inference can be far behind the Pareto frontier.

**Use expensive predictors as signal oracles, not necessarily as production decoders.**

### 3.3 MDL: model cost is real

A strong model is not free. A compressor must pay for some combination of:

- model description,
- parameters,
- branch/transform choices,
- dictionaries/rules,
- residuals.

The correct comparison is always total payload.

This exactly explains several ANVIL failures: a correlation existed, but its metadata could not amortize.

### 3.4 Entropy measures are not interchangeable with repetitiveness measures

LZ-style phrase copying, BWT runs, grammar size, substring complexity, and morphism-based repetitiveness measure different structure.

This matters because optimizing one representation can hit a structural blind spot even when the string is "obviously" repetitive.

The 2025 Navarro/Urbina work is especially important here: string morphisms capture families of regularity that copy/paste-based measures can miss, and combining morphism substitution with bidirectional copying (NU-systems) can be asymptotically smaller than either mechanism alone.

That is direct mathematical support for expanding ANVIL beyond "copy an old substring plus corrections."

---

## 4. Recent research that should change ANVIL's thinking

### 4.1 String morphisms + bidirectional copying: NU-systems

**Gonzalo Navarro & Cristian Urbina, _Repetitiveness Measures Based on String Morphisms_, Theoretical Computer Science, 2025.**

Key result: morphism-generated descriptions capture a form of self-similarity largely orthogonal to ordinary copy/paste repetitiveness. Their NU-system combines L-system-style substitution with bidirectional macro schemes and is asymptotically strictly smaller than either on some string families.

ANVIL implication:

> The decoder instruction vocabulary may need **generation rules**, not only references.

Do **not** implement a general L-system compressor. The practical translation is to ask which *restricted, bounded, fast-to-execute generative rules* occur in real data often enough to amortize their description.

Source:
https://doi.org/10.1016/j.tcs.2025.115259

### 4.2 Generalized straight-line programs

**Navarro, Olivares, Urbina, _Generalized Straight-Line Programs_, Acta Informatica, 2025.**

GSLPs permit a rule body to be represented by a compact program; specialized iterated SLPs can break traditional substring-complexity bounds on some text families.

The general construction is much too powerful for a fast commodity codec, but the abstraction is valuable:

> A phrase can be represented by a **small program that generates many output bytes**.

For ANVIL, the production-safe version must be:

- non-Turing-complete,
- bounded,
- fixed instruction count,
- allocation-free in the hot loop,
- ideally vectorizable,
- easy to validate before execution.

Source:
https://arxiv.org/abs/2404.07057

### 4.3 Generalized deduplication

Generalized deduplication represents similar chunks as a common base plus deviations. The theory shows that, under its source model, this can approach entropy while converging much faster than exact deduplication.

This is philosophically close to SPARSE-REF, but it also provides an important warning: **the definition of the base/deviation mapping determines everything**.

ANVIL should treat "find a reconstructable representative" as a mapping problem, not just approximate nearest-neighbor byte matching.

Source:
https://arxiv.org/abs/1901.02720

### 4.4 OpenZL: compression as an executable graph

Meta's OpenZL is the strongest recent systems-level confirmation of the architectural direction.

OpenZL uses:

- a configurable graph of reversible operations,
- offline/training-time plan search,
- a resolved graph embedded in the compressed frame,
- one universal decoder executing that recipe.

That separates **compressor intelligence** from **decoder machinery**.

OpenZL v0.2 additionally reports a new graph-native LZ path and aggressive operator fusion. Regardless of benchmark comparability, the architecture is relevant to ANVIL.

ANVIL should borrow the principle, not copy the product:

> **Search can be expensive and adaptive at encode time; decode should execute a tiny resolved program.**

Sources:
- https://arxiv.org/abs/2605.09928
- https://github.com/facebook/openzl
- https://engineering.fb.com/2025/10/06/developer-tools/openzl-open-source-format-aware-compression-framework/

### 4.5 Neural lossless compression as an oracle

2025–2026 neural compressors continue to demonstrate that much lower predictive cross-entropy is available than classical byte models expose.

This is useful to ANVIL even if a transformer will never be in the decoder.

Use neural models to answer:

- Which regions still contain predictable bits after ANVIL?
- Which context features explain the gain?
- Can that signal be distilled into a cheap deterministic rule?
- Is the signal local, structural, lexical, arithmetic, or semantic?

A model that predicts well but cannot be distilled is evidence of **available information**, not yet a production mechanism.

### 4.6 Bidirectional macro schemes: measure the price of causality

Ordinary LZ77 requires a phrase's source to already exist to its left. That causality constraint is excellent for streaming decode, but it is also a representational restriction.

LZRR (Nishimoto & Tabei) permits safe right references while preserving an acyclic dependency structure. It guarantees a phrase count no worse than LZ77, and its published experiments report roughly 6% fewer phrases on benchmark strings. Finding the globally smallest bidirectional macro scheme is NP-hard, but 2026 work demonstrates exact BMS/SLP solving on bounded instances using Answer Set Programming modulo acyclicity.

ANVIL implication:

> Use bidirectional schemes primarily as an **offline oracle** before paying the streaming/dependency cost in the production format.

On small real-data windows, compare current causal references against LZ77/LZRR/near-optimal BMS. If the gap is negligible, forward references are not worth a decoder complication. If the gap is material and concentrated in a few shallow dependency patterns, test a **bounded-depth, levelized reference DAG** whose dependency cost is explicit in the Pareto objective.

Sources:
- https://doi.org/10.1016/j.ic.2021.104859
- https://doi.org/10.24963/kr.2026/66

### 4.7 Online string attractors sharpen the same warning

Whittington's 2024 online-string-attractor result connects LZ factorization to the best online strategy for the online attractor problem while exhibiting an O(log n) online/offline separation on some morphic families.

That result does not say practical LZ codecs are doomed. It says something narrower and useful:

> More engineering on a causal temporal matcher cannot recover structure that fundamentally belongs to an offline/generative explanation class.

This strengthens the case for **oracle separation**: first determine whether a bad ANVIL region is limited by search quality inside the LZ family or by the explanatory family itself.

Source:
- https://arxiv.org/abs/2407.15599

### 4.8 CTW/PPM-class models should be statistical controls

Context Tree Weighting efficiently mixes a bounded-memory context-tree model class and has finite-sequence redundancy guarantees with linear computational/storage complexity in the sequence length.

ANVIL should not adopt CTW merely because it is theoretically elegant. It should use CTW/PPM-class models as **diagnostic controls**:

- If CTW sharply beats ANVIL's residual/literal coding, statistical predictability remains.
- If CTW buys little but a BMS/grammar oracle buys a lot, the missing structure is generative/referential.
- If both buy little, the region may already be close to its useful innovation floor.

This gives the project a cleaner decomposition than repeatedly inventing ad hoc order-N models.

Source:
- https://doi.org/10.1109/18.382012

### 4.9 Structure separation remains a live systems pattern

The 2026 STC work is a useful BWT-specific example: it removes digit runs from surrounding text, replaces them with an unambiguous placeholder, and codes the removed data in length/context-conditioned side streams. Its reported same-coder enwik9 ablation attributes about 2.63 MB of improvement to the decomposition.

The important lesson is broader than digits:

> A small heterogeneous innovation channel can poison the representation/model of a much larger regular channel.

That is exactly the kind of condition ANVIL should detect during anatomy. Sparse correction does this locally around a reference; deterministic decomposition does it globally or regionally. The correct mechanism depends on whether the changing fields are best explained by a prior span, by typed field semantics, or merely by isolation from the main channel.

Source:
- https://arxiv.org/abs/2606.03570

### 4.10 AITDCC 2026 should become an external generalization gate

The 2026 Algorithmic Information Theory Data Compression Challenge is unusually aligned with ANVIL's research discipline:

- 16 heterogeneous files;
- an original public-training / hidden-testing split;
- 117 valid compressors in the published study;
- compression/decompression time and Pareto analysis;
- an 8 GiB peak-memory limit;
- a <=1 MiB decompressor constraint;
- the complete A–P dataset is now public with canonical SHA-256 values.

ANVIL should use AITDCC as an **external validation class**, not another corpus to tune against. Preserve the original A–H versus I–P distinction in reports even though all files are now public.

This also adds two axes the current inner loop underweights: **decoder binary size** and **generalization to formerly hidden data**.

Sources:
- https://arxiv.org/abs/2606.17712
- https://aitdcc.github.io/dataset.html

---

## 5. Hardware reality: design the format for the CPU

The hot decoder should be treated like a tiny virtual machine whose ISA is designed for modern superscalar CPUs.

The target is not "use SIMD." The target is:

> maximize useful reconstructed bytes per retired instruction and per cache miss.

### 5.1 SIMD is strongest on dense, regular work

Good SIMD targets in ANVIL:

- equality/mismatch detection,
- byte classification,
- small integer transforms,
- bitpacking/unpacking,
- checksum folding,
- table initialization,
- stream transposition,
- multi-lane entropy states,
- bulk literal movement.

Bad SIMD targets:

- pointer-chasing match history,
- highly divergent token dispatch,
- dependent variable-length state machines with gathers everywhere,
- tiny irregular sparse patches.

### 5.2 Control/data separation is a first-class design technique

Stream VByte's key insight is not "varints but SIMD." It is:

> **separate control bytes from payload bytes so decoding can see widths ahead of time.**

ANVIL already splits semantic streams. Extend this thinking to numeric metadata:

- lengths,
- distances,
- signed displacement deltas,
- correction positions,
- run counts,
- dictionary/rule IDs.

For small integers, a raw/bitpacked/PFor/Stream-VByte-like stream can be faster than entropy-coding every byte even when it costs a few more bits.

If ANVIL has rate surplus against the target reference, spending some of it to remove dependency/branching is often correct.

Source:
https://arxiv.org/abs/1709.08990

### 5.3 rANS parallelism comes from independent states

A scalar rANS state is inherently serial:

`xₜ₊₁ = F(xₜ, sₜ)`.

Throughput comes from interleaving independent states to expose ILP/SIMD.

Known implementations use:

- 4/8/16+ interleaved states,
- SSE/AVX lanes,
- precomputed decode tables,
- branchless/masked renormalization.

ANVIL already explored 4-way interleaving earlier. The key caveat now is the context-switched literal coder: previous-symbol context creates a semantic dependency between adjacent symbols, so simply widening the rANS arithmetic does not remove the context dependency.

Practical implication:

- vectorize **independent streams** and independent blocks where possible;
- keep serial context streams only where their rate gain pays for the dependency;
- consider context reset/interleave layouts only if their metadata/rate penalty clears a measured budget.

Reference implementation:
https://github.com/rygorous/ryg_rans

A more aggressive reference implementation with AVX2 multi-state decoders:
https://github.com/jkbonfield/rans_static

### 5.4 Integer metadata deserves a different codec family

ANVIL currently serializes many quantities into varint-byte streams and then entropy-codes the resulting bytes.

For highly structured integer streams, test a separate family:

- frame-of-reference + bitpacking,
- patched frame-of-reference,
- delta / delta-of-delta,
- zigzag + fixed bit width,
- Stream VByte,
- sparse exception vectors.

TurboPFor is useful as a systems reference because it shows how far dense integer decode can be pushed when representation and SIMD are co-designed.

Reference:
https://github.com/powturbo/TurboPFor

### 5.5 Match verification: vectorize comparison, not search state

A strong low-level kernel for any future generalized-reference family:

1. load 32/64 bytes from source and target,
2. XOR or compare,
3. produce an equality/mismatch mask,
4. find the first mismatch with mask + TZCNT,
5. optionally classify mismatches by 4-byte/8-byte lanes.

AVX2:
- `VPCMPEQB`
- `VPMOVMSKB`
- `TZCNT`

AVX-512:
- compare directly into k-masks,
- potentially `VPCOMPRESSB` / compress-store for encoder-only sparse residual gathering where profitable.

This is useful only when candidate generation is already selective. SIMD cannot rescue a search that generates millions of useless candidates.

### 5.6 Small-copy execution matters more than heroic memcpy replacement

For large non-overlapping copies, libc/compiler `memcpy` is already heavily optimized.

ANVIL's opportunity is the distribution of **small exact/match copies**:

- 4–7 B sidecar tokens were previously a major token-count problem.
- fixed 8/16/32-byte copies can avoid generic-call/setup overhead.
- overlap semantics must remain explicit.
- group adjacent compatible ops before considering vector copies.

Do not replace bulk `memcpy` blindly.

### 5.7 CRC lesson: use dedicated polynomial hardware where it wins

ANVIL's PCLMUL CRC work is already a successful example of format-preserving hardware acceleration.

Keep the pattern:

- runtime dispatch,
- bit-exact output,
- scalar fallback,
- architecture-specific kernels isolated,
- benchmark same wire.

### 5.8 BWT is currently memory/data-dependency limited

Current `libsais` notes that inverse BWT uses a bi-gram LF mapping to decode two symbols per step and provides auxiliary-index APIs.

ANVIL's own I9 prototype already measured `libsais_unbwt_aux` at roughly 3.4–3.55x the ordinary inverse path. That is the first BWT engineering task to integrate before inventing another inverse.

Current upstream:
https://github.com/IlyaGrebnov/libsais

### 5.9 ALP/FastLanes reinforce "design representation around vector execution"

ALP is domain-specific floating-point compression, but its architecture is highly relevant. Its authors explicitly designed the encoding to fit vectorized execution and found that the vector-oriented representation also exposed **better compression opportunities**, not merely faster loops. FastLanes extends that approach with lane-oriented layouts and composable expression encodings.

The newest systems signal is adoption: Apache Parquet added ALP encoding in September 2026, describing SIMD/GPU-friendly decoding with compression ratios in the zstd range on suitable decimal-like floating-point data.

ANVIL lesson:

> Hardware-friendly representation is not necessarily a tax paid after choosing the mathematical model; the hardware layout can reveal a *better model*.

This argues for designing candidate ANVIL metadata/reconstruction operators in vector-sized groups from the beginning, then measuring whether the grouping itself exposes regularity.

Sources:
- https://doi.org/10.1145/3626717
- https://github.com/cwida/FastLanes
- https://parquet.apache.org/blog/2026/09/22/alp-adaptive-lossless-floating-point-encoding-in-apache-parquet/

### 5.10 Build a SIMD Stage-1 structural-event scanner

PNRA's deepest systems idea was event-driven inversion: spend expensive work only where the input exposes a structural event.

A generalized Stage 1 should borrow the *hardware pattern* of parsers such as simdjson without copying their semantics:

1. wide byte classification/comparison;
2. emit compact masks/event records;
3. use POPCNT/TZCNT and table lookup to enumerate only meaningful positions;
4. feed typed indexes/oracles from that sparse event stream.

Candidate event classes:

- line/record separators and quote transitions;
- digit/sign runs;
- zero/nonzero masks;
- aligned 16/32/64-bit change masks;
- likely fixed-width numeric lanes;
- x86 E8/E9 relocation events;
- recurring separator motifs;
- high-confidence token boundaries.

The research metric is not scanner GB/s alone. It is:

useful candidate explanations found / bytes scanned / verification work

A scanner that is extremely fast but emits an enormous false-positive stream simply moves the bottleneck.

The workstation reference CPU is Zen 3, so the baseline production design should be **AVX2-class**, with scalar fallback and optional AVX-512/NEON/SVE paths. Do not make AVX-512 a format assumption.

---

## 6. The proposed breakthrough research program

### 6.1 Working abstraction: the ANVIL Explanation Machine

This is a **research abstraction**, not a novelty claim.

Represent a block as a sequence of bounded decoder instructions. Each instruction explains output bytes from decoder-visible state.

Candidate semantic classes:

1. **LITERAL**
   - emit explicit residual bytes.

2. **COPY**
   - ordinary LZ exact reference.

3. **COPY_PATCH**
   - prior phrase + sparse correction.

4. **TRANSFORM_COPY**
   - prior phrase under a compact deterministic transform.

5. **GENERATE**
   - output generated from a small bounded rule.

6. **REPLAY**
   - reconstruct an embedded representation from a normalized form + compact correction/replay metadata.

The key difference from the current mode explosion is conceptual:

> Every mechanism competes in one common currency: **how many output bytes become deterministic per byte of instruction/residual and per decode cycle**.

### 6.2 Do not build a general VM

A general decoder VM is a trap:

- huge attack surface,
- unpredictable runtime,
- branch-heavy,
- hard to vectorize,
- hard to prove bounds,
- decoder-size tax.

Instead, use a closed registry of **microprogram templates**.

Each template must have:

- bounded output expansion,
- bounded memory reads,
- no arbitrary jumps,
- no dynamic allocation in the hot path,
- a fixed decoder kernel,
- a precise wire ID,
- a static cost model.

Think "macro-instruction library", not bytecode interpreter.

### 6.3 Transformation-Invariant Temporal Anchoring should become a framework

PNRA's important idea survives even though the current candidate injection failed economically.

General formulation:

Find an invariant (I(x,p)) for transform family (T):

`I(T(x, θ), p′) = I(x, p)`

for decoder-relevant transformed equivalents.

Then index history by `I`, so candidate discovery becomes:

`structural event → exact invariant hash lookup → cheap verifier`

instead of:

`raw approximate search → many candidates → infer transform → verify`.

The latter burns encoder cycles discovering a parameter after the expensive part.

The **next** version must satisfy PNRA's recorded reopen condition: a candidate needs either real-wire cost prediction that rejects short/far losers or a token that amortizes multiple invariant fields.

### 6.4 Strong new candidate: decoder-derived transform masks

TCOPY currently pays to identify transformed fields.

For executable code, some field positions can be derived from the copied bytes themselves:

- x86 `E8`/`E9` relative branch/call operands,
- selected RIP-relative addressing forms if a minimal instruction scanner can identify them safely,
- analogous fixed-layout branch immediates on ARM/AArch64.

Hypothesis:

> A transformed copy can avoid sending a per-phrase transform mask when the decoder can deterministically derive transform sites from the source phrase's syntax.

This is not the same as a global BCJ prefilter:

- BCJ rewrites the entire stream before LZ.
- This proposed primitive performs **reference-local semantic normalization during copy reconstruction**, preserving original byte layout outside the selected reference.

However, prior art includes BCJ/E8E9 filters and relocation-aware binary delta systems. Therefore **no novelty claim is allowed without a dedicated patent/paper audit.**

Why it is worth an oracle prototype:

- removes exactly the mask/framing cost that weakened TCOPY,
- makes one phrase amortize multiple relocation fields,
- decoder can be simple for a narrow opcode subset,
- verification can be event-driven,
- semantics are exact.

First scope should be only E8/E9 rel32. Do not start with a full x86 decoder.

### 6.5 Strong general candidate: restricted iterated phrases

Inspired by ISLP/NU systems, test a tiny generative phrase family with no general grammar machinery.

Example conceptual form:

`ITER(n, body, state₀, Δ)`

where each iteration emits a fixed-shape body whose selected fields are updated by a simple operation.

Possible operations:

- `xᵢ = x₀ + iΔ`
- `xᵢ = xᵢ₋₁ + Δᵢ` with sparse exceptions
- repeated body/rule IDs
- position-derived values
- field permutation + simple delta.

The decoder should execute this as a counted loop, ideally with SIMD across fields.

This is a much stronger test of the "program beats copy" thesis than another approximate-match heuristic.

**Gate before implementation:** an offline anatomy pass must show enough real-corpus bytes covered to pay for rule metadata.

### 6.6 REPLAY should remain a first-class family

P4.1 is conceptually important because it converts already-compressed data from "entropy-looking bytes" back into a structured explanation.

For embedded DEFLATE:

`original_stream = Replay(plaintext, predictor_parameters, corrections)`

If replay metadata is smaller than the opacity tax, the compressor wins.

This is the purest instance of "make data reconstructable instead of storing it."

Long term this family can include other deterministic encoders only when corpus frequency and decoder-size tax justify them.

### 6.7 Narrow mechanism hypothesis: Invariant Generative Span IR (IGS-IR)

The generic idea "compression is a graph/program of transforms" is already established by OpenZL, grammar compressors, bidirectional macro schemes, generalized deduplication, and reconstruction systems. ANVIL should not claim novelty for a graph or IR.

A narrower mechanism hypothesis survives:

> discover sparse structural events cheaply; derive exact transformation-invariant signatures from those events; use exact signature matches to discover **typed generative relationships among output spans**; encode only the innovation the relation cannot reconstruct; compile selected relations into a bounded-depth, hardware-oriented decode schedule.

Working name: **Invariant Generative Span IR (IGS-IR).**

The potential differentiator is the interaction of four properties:

1. **event-driven discovery** rather than all-byte approximate search;
2. **transform-invariant indexing** so the relation is known before verification rather than inferred afterward;
3. **typed exact span relations** such as displacement-derived copy, recurrence, basis+innovation, or bounded generation;
4. **compiled shallow decode schedule** optimized for homogeneous kernels and explicit dependency depth.

This still requires a dedicated prior-art audit. The above separators define what to search for; they do not prove novelty.

The first oracle test should be deliberately hostile to the hypothesis:

- compare causal LZ;
- compare LZRR/BMS/SLP controls;
- compare same-transform controls;
- charge every relation descriptor;
- charge dependency schedule bytes;
- estimate source-read locality and cycles/emitted byte.

If IGS-IR's best offline explanation barely improves complete description length, stop. If it produces a large rate surplus but a terrible dependency graph, treat it as a theory result, not a production mechanism. It earns implementation only if the representation headroom and decode economics appear simultaneously.

---

## 7. A new cost function: Pareto budget, not arbitrary lambda

The previous stream-budget experiment used a fixed small λ and did not move choices.

Replace arbitrary weights with a **frontier-derived budget**.

For a candidate (c), compare against a target reference point (r).

Let:

- `B_c`: compressed bytes
- `T_c`: decode time
- `M_c`: peak memory
- `D_c`: decoder binary bytes

Then define explicit admissible budgets from the reference point:

`ΔB = B_r − B_c`

is the candidate's byte surplus.

The question is not "what lambda feels right?"

It is:

> **How many cycles, cache misses, model bytes, or decoder-code bytes are we allowed to spend before the candidate becomes dominated?**

This can be computed per corpus and per target tier.

Examples:

- If ANVIL is 60 KB smaller than a reference but 2 ms slower, that 60 KB is the exact budget available for a faster representation.
- If emitting a raw control stream costs 20 KB but removes 3 ms decode, the trade can be evaluated directly.
- If an opcode book saves 80 KB but adds 100 KB to the universal decoder binary, a deployment objective may reject it even when file bytes win.

This converts the project from "optimize everything" into **resource arbitrage against the frontier**.

---

## 8. Decoder architecture principles for the next format revision

### 8.1 Resolved semantic stream

The decoder should receive a resolved sequence where common operations are nearly direct dispatch:

`opcode → predecoded operands/state → output write`

Avoid reconstructing semantics from many independent entropy streams inside the hottest loop when a small fused representation can encode them.

ANVIL's hot-op work already demonstrated the value of this, but further tuning is closed until a new profile says dispatch dominates again.

### 8.2 Structure of Arrays for side metadata

For cold/rare operands, preserve separate streams:

- operation IDs,
- small lengths,
- distance classes,
- low bits,
- residual positions,
- residual values.

But choose the representation per stream from an explicit cost budget.

### 8.3 Table working-set discipline

A decode table that misses L1/L2 can lose more than it saves in arithmetic.

Track:

- table bytes,
- tables live simultaneously,
- random vs sequential accesses,
- number of dependent gathers,
- initialization cost.

A smaller 256/512-state table may beat a 4096-state table for tiny streams even when the latter saves bits.

### 8.4 Architecture dispatch

Production kernel families should be isolated behind stable interfaces:

- scalar baseline,
- SSE4.1/AVX2 x86,
- AVX-512 where it actually wins,
- NEON,
- later SVE2.

Do not make the wire depend on the ISA.

---

## 9. Research methodology: anatomy before implementation

Before adding a new codec mode, run an **oracle/anatomy pass**.

For a proposed explanation family:

1. identify all exact opportunities in the corpus;
2. compute maximum raw bytes explainable;
3. compute minimum possible instruction metadata;
4. compute residual entropy/proxy;
5. estimate decoder ops / output byte;
6. compare against current best parse;
7. stop if even the oracle cannot cross the economic threshold.

This directly enforces the existing E5 rule: do not build a mechanism before proving there is enough leverage.

### Required outputs

Every anatomy probe should produce:

- opportunities,
- covered bytes,
- average/median phrase length,
- metadata lower bound,
- residual lower bound,
- candidate count,
- verification count,
- predicted token count,
- expected decoder operations,
- corpus/file distribution.

Only after that should a production wire format be designed.

---

## 10. No-reburn list carried forward

Do not spend another iteration on these without satisfying the existing reopen condition:

- current PNRA candidate injection,
- old `(mask,slot)` modal residual coding,
- SRR period probing feeding the old topology coder,
- reinforce-taken structural channels,
- current RLZ/RePair stream implementation as default,
- whole-codec stream budget at (lambda=0.01),
- reciprocal-rANS replacing hardware divide on the old target,
- more hot-op dispatch tuning without a new whole-codec profile,
- more boundary heuristics on the old parser,
- block-local backend routing without cross-block carryover / near-zero warmup,
- QLFC/LZP BWT postcoder retry in the same form,
- global lane transpose before LZ.

---

## 11. Concrete next queue

### Phase A — make measurement remote and reproducible

1. Publish a sanitized public ANVIL snapshot.
2. Add manual GitHub Actions benchmark workflow.
3. Record runner CPU, kernel, compiler, dependency versions and commit SHA.
4. Upload raw CSV/JSON and environment artifacts.
5. Treat shared-runner timing as **scout/ranking-grade**, not absolute cross-run truth.
6. Make claims only from paired same-runner comparisons or repeated evidence.

### Phase B — close the two high-EV pending engineering items

1. Integrate `libsais_unbwt_aux` with explicit wire charge and forced tests.
2. Integrate P4.1 DEFLATE reconstruction natively and fuzz it.

These should be separate commits/experiments.

### Phase C — build anatomy tools, not codec modes

Three first probes:

1. **Derived-mask executable TCOPY oracle**
   - E8/E9 only.
   - Measure bytes coverable with zero explicit transform mask.
   - Compare against global BCJ + same backend as control.

2. **Restricted iterated-phrase oracle**
   - discover fixed-shape repeated regions with affine/position-derived fields.
   - no production decoder yet.
   - report ideal metadata and coverage.

3. **Integer-metadata recoding oracle**
   - replay current token stream through bitpack / FOR / Stream-VByte-like controls.
   - measure bytes + estimated cycles without changing parse.

### Phase D — only then build one mechanism

The first new mechanism should be whichever anatomy probe has the largest **real-corpus Pareto leverage**, not whichever sounds most novel.

---

## 12. GitHub Actions measurement philosophy

Public GitHub-hosted standard runners are free for public repositories, but they are not dedicated benchmark machines.

Therefore:

### Good uses

- expensive ratio sweeps,
- corpus-wide roundtrip/fuzz,
- oracle/anatomy scans,
- same-job A/B throughput scouts,
- compiler/ISA matrices,
- sanitizer builds,
- long-running parameter sweeps.

### Bad uses

- comparing absolute MB/s from unrelated workflow runs,
- declaring a 2–3% speed win from one VM,
- mixing compiler versions and calling it a codec gain,
- aggregating x64 and arm64 timings into one frontier.

### Required provenance artifact

Every benchmark job should capture:

- commit SHA,
- dirty state should be impossible in CI,
- `lscpu`,
- `uname -a`,
- compiler version,
- CMake version,
- dependency versions,
- CPU flags,
- runner image,
- job/run ID,
- corpus hashes,
- exact command line,
- per-repetition raw timings.

---

## 13. Sources worth keeping in ANVIL's permanent research set

### Information theory / universal coding

- Stanford — Robert Gray, *Fundamentals of Information Theory and Coding*  
  https://www-ee.stanford.edu/~gray/fundcom.pdf
- MIT information theory text — universal compression / arithmetic coding / prediction equivalence  
  https://people.lids.mit.edu/yp/homepage/data/itbook-export.pdf
- Grünwald & Roos, *Minimum Description Length Revisited*  
  https://doi.org/10.1142/S2661335219300018
- Willems, Shtarkov, Tjalkens, *The Context-Tree Weighting Method: Basic Properties*  
  https://doi.org/10.1109/18.382012

### Repetitiveness / generative representations

- Navarro & Urbina, *Repetitiveness Measures Based on String Morphisms* (2025)  
  https://doi.org/10.1016/j.tcs.2025.115259
- Navarro, Olivares, Urbina, *Generalized Straight-Line Programs* (2025)  
  https://arxiv.org/abs/2404.07057
- Kempa & Langmead, *Fast and Space-Efficient Construction of AVL Grammars from the LZ77 Parsing*  
  https://arxiv.org/abs/2105.11052
- Badkobeh, Bannai, Köppl, *Bijective BWT based compression schemes*  
  https://arxiv.org/abs/2406.16475
- Nishimoto & Tabei, *LZRR: LZ77 parsing with right reference*  
  https://doi.org/10.1016/j.ic.2021.104859
- Whittington, *Online String Attractors*  
  https://arxiv.org/abs/2407.15599
- *Optimal Dictionary-Based Compression with Answer Set Programming* (KR 2026)  
  https://doi.org/10.24963/kr.2026/66

### Similarity / deduplication

- Vestergaard, Zhang, Lucani, *Generalized Deduplication: Bounds, Convergence, and Asymptotic Properties*  
  https://arxiv.org/abs/1901.02720
- Aoshima, Kurihara, Tanaka, *Aggregable Generalized Deduplication* (2026)  
  https://doi.org/10.23919/transcom.2025EBP3064
- Wang et al., *ZipLLM: Efficient LLM Storage via Model-Aware Synergistic Data Deduplication and Compression* (NSDI 2026)  
  https://www.usenix.org/conference/nsdi26/presentation/wang-zirui

### Systems architecture

- Meta OpenZL paper  
  https://arxiv.org/abs/2605.09928
- OpenZL implementation  
  https://github.com/facebook/openzl
- Intel ISA-L  
  https://github.com/intel/isa-l
- libdeflate  
  https://github.com/ebiggers/libdeflate
- libsais  
  https://github.com/IlyaGrebnov/libsais

### SIMD / integer / entropy coding references

- Stream VByte  
  https://arxiv.org/abs/1709.08990
- ryg_rans  
  https://github.com/rygorous/ryg_rans
- rans_static  
  https://github.com/jkbonfield/rans_static
- TurboPFor  
  https://github.com/powturbo/TurboPFor
- ALP — Adaptive Lossless Floating-Point Compression  
  https://doi.org/10.1145/3626717
- FastLanes  
  https://github.com/cwida/FastLanes
- STC — reversible digit-context decomposition for BWT-family text  
  https://arxiv.org/abs/2606.03570

### Executable normalization controls / prior art

- xz BCJ filters  
  https://tukaani.org/xz/
- ZPAQ E8/E9 transform documentation  
  https://manpages.debian.org/trixie/zpaq/zpaq.1.en.html

---

## 14. Bottom line

The strongest research hypothesis for the next ANVIL era is:

> **Compression improves when the decoder's vocabulary of cheap explanations becomes richer faster than the description cost and execution cost of those explanations grow.**

LZ says:

> "these bytes already occurred."

TCOPY says:

> "these bytes almost occurred, under a cheap transform."

DEFLATE reconstruction says:

> "these bytes are the deterministic output of an encoder we can replay."

Morphism/grammar work says:

> "these bytes can be generated by a compact rule."

The breakthrough target is a production codec that discovers such explanations cheaply, emits only a bounded resolved program plus irreducible residuals, and executes the program at memory-copy-class speed.

That is a deeper target than a better entropy coder, and it is aligned with both ANVIL's strongest empirical results and the most relevant modern theory.
