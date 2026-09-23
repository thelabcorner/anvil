# ANVIL Frontier Research Addendum — Program Synthesis, Information Theory, and CPU-Native Decode

**Date:** 2026-09-23
**Status:** research-only reset addendum; no codec source change authorized by this document
**Scope:** post-I10-1A direction finding before I10-1B or any new production mode
**Measurement policy:** CPU-heavy compression/decompression benchmarks run on public GitHub Actions only

---

## 0. Executive conclusion

The most important conclusion from this research pass is that ANVIL should stop
treating "find the next compression mode" as the primary breakthrough-search
strategy.

The stronger direction is:

> **Search for the smallest fast decoder-visible explanation program that
> reconstructs a region exactly.**

This is not just philosophical. Several independent research lines now point in
the same direction:

1. **Brevis (2026)** demonstrates practical bit-exact compression as bounded
   program synthesis over a typed reversible DSL, with expensive search at
   compression time and direct program execution at decode time.
2. **OpenZL** demonstrates that a resolved graph of reversible operators can
   specialize compression while retaining a universal decoder, and that fusing
   stages matters materially for decode throughput.
3. **NU-systems / generalized straight-line programs** show theoretically that
   copy/paste repetitiveness is not the only useful explanation class; compact
   generative programs can capture structure that ordinary dictionary methods
   fundamentally miss.
4. **Exact BMS/SLP solvers** give us an offline oracle for the price ANVIL pays
   for causal/left-reference constraints.
5. **Shannonic** shows that even inside entropy coding, changing the
   representation of a value into a small categorical decision plus a local
   offset can move closer to the Shannon bound with extremely small state.
6. **Modern CPU-oriented compressors** repeatedly achieve speed by changing
   representation and control flow so hardware sees regular work: separated
   controls, independent entropy states, wide compares, fused decode, dense
   bitpacking, specialized copies, and runtime ISA dispatch.

The synthesis is stronger than any one mechanism:

> **Compression quality is determined by the explanatory vocabulary exposed to
> the decoder; decode speed is determined by how that vocabulary maps onto the
> machine. ANVIL should co-design both.**

I10-1B (bit-exact DEFLATE replay) remains a high-EV engineering experiment and
an important instance of the REPLAY explanation class. It should remain
isolated and proceed only after this research reset is fully understood. It
should not become the conceptual center of Iteration 10.

### 0.1 Same-runner external evidence changes the systems priority

After I10-1A closed, GitHub Actions run `35927623136` measured the frozen
auxiliary-BWT candidate against xz -9e and Brotli q11/lw30 in the **same jobs**,
with paired/interleaved decode timing, an A/A null control, exact bytes, and
per-process peak RSS.

The result is decisively **FRONT-GAP_COST**, not an external crossing.

Silesia:

- ANVIL aux: **46,466,339 B**, ~47.9 MB/s decode, **248.4 MiB** peak RSS;
- xz -9e: 48,456,004 B, ~82.5 MB/s, 54.3 MiB;
- Brotli q11/lw30: 49,383,136 B, ~166.9 MB/s, 124.5 MiB;
- paired ANVIL/xz decode-time ratio: **1.7226**, 95% CI [1.6640, 1.8400];
- paired ANVIL/Brotli ratio: **3.4474**, CI [3.2680, 3.5059];
- ANVIL/xz RSS ratio: **4.572x**.

enwik8:

- ANVIL aux: **23,537,422 B**, ~26.5 MB/s decode, **598.9 MiB** peak RSS;
- xz -9e: 24,831,648 B, ~103.6 MB/s, 66.2 MiB;
- Brotli q11/lw30: 24,810,180 B, ~150.2 MB/s, 251.9 MiB;
- paired ANVIL/xz decode-time ratio: **3.9065**, 95% CI [3.9003, 4.0088];
- paired ANVIL/Brotli ratio: **5.6642**, CI [5.6528, 6.7596];
- ANVIL/xz RSS ratio: **9.043x**.

The A/A null intervals span 1.0 in both jobs.

This changes the immediate engineering interpretation:

> **For large BWT-routed inputs, working-set size is now a measured first-class
> frontier problem, not a secondary implementation detail.**

The existing `--bwt-subblock` representation therefore deserves a proper
three-axis **bytes / decode / peak-RSS sweep** before inventing a new BWT
mechanism. It already bounds the size of any one inverse-BWT problem, and the
historical audit proves that the knob is genuinely wired. The cost is known to
be nonzero: smaller independent blocks can lose BWT context and add framing.

That is exactly a Pareto experiment: spend some rate to reduce working set and
possibly expose more independent decode work. Max-ratio keeps the 128 MiB
effectively-off point; a memory/balanced profile is allowed to retain a
different non-dominated cap.

---

## 1. Re-state the actual optimization problem

ANVIL is not optimizing compressed bytes alone.

For source block X, decoder-visible state S, and explanation program P, a useful
decomposition is:

    L_archive(X) ~= L(P) + L(R | P,S)

where:

- P is the decoder-visible explanation;
- R is the irreducible residual after applying that explanation;
- every byte needed to reconstruct X must be included in P, R, or pre-agreed
  decoder state.

The systems objective is a vector, not a single scalar:

    C(P) = (
      bytes,
      encode_time,
      decode_time,
      peak_memory,
      working_set,
      decoder_binary_size,
      dependency_depth,
      robustness
    )

The target is the Pareto frontier of this vector over realistic heterogeneous
data.

This immediately explains why several earlier ANVIL mechanisms were interesting
but not winners:

- a mechanism may reduce residual entropy but spend too many instruction bytes;
- it may save bytes while creating a serial decoder dependency chain;
- it may create a tiny side stream that destroys locality;
- it may improve a stage while making the whole codec slower;
- it may discover structure that is real but too expensive to describe;
- it may require an encoder search so large that the practical point is
  dominated.

I10-1A was a clean example of a legitimate new Pareto point: it spends a tiny
amount of additional wire to materially reduce inverse-BWT dependency depth.
Neither point dominates the other.

---

## 2. Shannon: what the bound does and does not say for ANVIL

### 2.1 The central mistake to avoid

Shannon entropy is not a universal fixed "compressibility number" attached to a
particular byte string independent of representation and side information.

For a stochastic source X under a specified probability model, entropy describes
the expected irreducible information of that source. For a decoder that already
knows state S, the relevant quantity is conditional entropy:

    H(X | S)

If X is a deterministic function of decoder-visible state,

    X = f(S),

then:

    H(X | S) = 0.

That is the rigorous form of ANVIL's thesis that the cheapest byte is a byte the
decoder can reconstruct rather than store.

The catch is equally important: if the decoder does not already know the
explanation, then the explanation itself has information content and must be
transmitted or fixed in the decoder. There is no free information.

### 2.2 Model cost is part of the compressed file

A more useful ANVIL mental model is Minimum Description Length:

    L(X) = L(M) + L(X | M)

where M can be a statistical model, grammar, transform, reconstruction program,
dictionary, field layout, or replay recipe.

A highly expressive model can drive L(X | M) toward zero while losing overall
because L(M) explodes.

ANVIL should therefore score the **complete serialized representation**,
including:

- operator IDs;
- parameters;
- shapes/lengths;
- dictionaries/rules;
- transform masks;
- tables;
- entropy models;
- literal/residual bytes;
- framing.

Proxy token costs are useful only for pruning. Final decisions must use exact
wire cost.

### 2.3 Algorithmic information is a north star, not an executable target

Kolmogorov complexity asks for the shortest program that outputs an individual
sequence and halts. It is uncomputable in general, but the concept is extremely
useful:

> A compact algorithmic explanation can beat every representation that is
> restricted to copies, local contexts, or fixed transforms.

The 2025 KoLMogorov Test makes this concrete by asking code-generating models to
produce short programs for data. Its negative result is also useful: current
large code models do poorly and synthetic improvements generalize poorly to
natural data. ANVIL should therefore not outsource the problem to an LLM. It
should construct a deliberately tiny, reversible, compression-specific search
space.

Reference:
- https://proceedings.iclr.cc/paper_files/paper/2025/hash/db1d69ee01c2b42554f8e14e6f8ca8b6-Abstract-Conference.html

### 2.3.1 Algorithmic sufficient statistics are almost exactly ANVIL's thesis

Classical Kolmogorov complexity asks for the shortest complete description of
one object. **Algorithmic statistics** asks a more useful two-part question:

> How much of that description is reusable **model/structure**, and how much is
> irreducible **data-to-model residual**?

Gács, Tromp, and Vitányi formalize an algorithmic sufficient statistic as a
model that captures essentially all meaningful regularity in an individual
object while leaving the object typical inside that model. Vereshchagin and
Vitányi's Kolmogorov structure function then studies the trade-off between a
model-complexity budget and the shortest residual/index needed to identify the
particular object inside the model.

References:
- https://doi.org/10.1109/18.945257
- https://homepages.cwi.nl/~paulv/papers/algorithmicstatistics.pdf
- https://doi.org/10.1109/TIT.2004.838346
- https://homepages.cwi.nl/~paulv/papers/structure.pdf

That is unusually close to ANVIL's design objective:

    source
      = compact decoder-visible explanation
      + residual information not explained by that explanation

The theoretical quantities are uncomputable in general, which is precisely why
ANVIL should **not** attempt arbitrary program search.

Instead define a computable, restricted analogue over an explicit operator DSL.

For target region X and explanation-program budget b:

    h_ANVIL(X, b)
      = minimum exact residual bits found
        using a valid ANVIL explanation whose serialized program <= b bits

Then the byte-optimal point for that vocabulary is approximately the lower
envelope of:

    b + h_ANVIL(X, b)

Unlike the classical structure function, ANVIL also cares about execution. A
systems version should therefore retain at least:

    (program bits, residual bits, decode work, working set)

as a Pareto surface rather than collapsing them immediately.

This gives the Explanation Machine a much sharper scientific role:

> **It is a computable restricted structure-function estimator for the
> explanation vocabulary ANVIL actually knows how to decode cheaply.**

That framing also gives negative results value. If the curve stops improving
after COPY/PATCH/SCAN/REPLAY operators are available, then adding more search
effort inside the same vocabulary is unlikely to be the breakthrough. The next
research question becomes: **which new primitive moves the restricted structure
function downward without exploding decoder cost?**

This is a more rigorous form of "make the bytes reconstructable rather than
store them."

### 2.4 Prediction and compression are dual diagnostic views

For a sequential probability model:

    ideal local bits ~= -log2 p(x_i | x_<i)

Predictive surprise and coding cost are two views of the same information.

Therefore a strong predictor that beats ANVIL on a region is evidence that
usable information remains. It does **not** imply that the predictor belongs in
the production decoder.

A large model can instead be an oracle:

1. identify where ANVIL leaves predictive information;
2. inspect what features explain the gap;
3. distill those features into deterministic cheap operators;
4. reject the idea if it cannot be distilled without a large decoder/model.

Recurring principle:

> **Use expensive intelligence to discover structure; require cheap deterministic
> machinery to reproduce it.**

### 2.5 Mutual information prices the value of decoder-visible state

The amount of uncertainty removed by side information S is:

    I(X;S) = H(X) - H(X|S)

This gives ANVIL a useful theoretical interpretation for any proposed
decoder-visible state:

> the maximum asymptotic information benefit available from S is the mutual
> information it carries about the target, before charging the cost of making S
> available and the finite-block/code overhead.

This is not directly an implementable score on one finite file, but it prevents
a conceptual error: a complex side structure cannot save more information than
it actually explains.

Slepian-Wolf theory goes even further. For correlated sources, asymptotic
lossless compression can approach H(X|Y) even when correlated side information
Y is available only at the decoder. ANVIL does not currently need a
Slepian-Wolf code, but the theorem reinforces the central point that
**decoder-visible correlation is a first-class compression resource**. The
resource is only "free" when Y is genuinely already available to the decoder.

Reference:
- https://ocw.mit.edu/courses/6-441-information-theory-spring-2016/pages/lecture-notes/

### 2.6 KL divergence separates irreducible entropy from model failure

Suppose the real source law is P but an entropy coder assigns probabilities Q.
Ideal expected code length is the cross-entropy:

    H(P,Q) = H(P) + D_KL(P || Q)

up to finite-coder overhead.

That decomposition is strategically important for ANVIL:

- H(P) is irreducible under the specified source/modeling abstraction;
- D_KL(P||Q) is **avoidable model mismatch**;
- coder mechanics contribute another, usually smaller, finite implementation
  overhead.

Therefore an apparent entropy-coder problem may actually be a representation
problem. A reversible transform cannot destroy information in the complete
sequence, but it can move the data into coordinates where ANVIL's simple model
Q has dramatically less mismatch.

This is exactly what BWT, delta coding, field splitting, range/offset coding,
and future explanation operators are trying to do.

The target is not "lower entropy by reversible magic." It is:

> **expose dependencies so a small, fast decoder model stops paying KL mismatch
> for structure that was present all along.**

Reference:
- https://www.gatsby.ucl.ac.uk/teaching/courses/ml1-2012/lect1-handout.pdf

### 2.7 Entropy rate, not byte histogram entropy, is the relevant source quantity

A byte histogram estimates only a zero-order marginal H(X_t). For a source with
memory, the asymptotically relevant quantity is the entropy rate:

    h = lim H(X_n | X_1,...,X_{n-1})

when the limit exists.

Consequently:

- low zero-order entropy can be easy but is not required for compressibility;
- high byte entropy does not imply incompressibility when long-range structure
  exists;
- a transform that leaves the global information unchanged can greatly improve
  a bounded-memory model by making the dependency local;
- grammar/reference/generative mechanisms attack dependencies that a local
  symbol model may never expose efficiently.

This is another reason ANVIL's anatomy tools should measure explanation gaps,
not just entropy histograms.

---

## 3. The strongest new systems result: Brevis

### 3.1 Why it matters to ANVIL

Shi et al., *Lossless Tensor Compression as Program Synthesis* (August 2026),
introduce Brevis. Although tensor-specific, its architecture is unusually close
to ANVIL's central thesis.

Brevis represents each tensor by a typed self-contained program whose execution
reconstructs the original physical words bit-exactly. Compression performs a
bounded A* search, guided by an input-specific production prior. Decompression
does not search and does not need that prior; it validates and executes the
serialized program.

The paper evaluates the **complete serialized program size**, including
instructions, parameters, literal coding tables, lengths, and payloads.

Paper:
- https://arxiv.org/abs/2608.02162

Implementation:
- https://github.com/jiekeshi/Brevis

### 3.2 Operators that matter conceptually

Brevis uses a compact typed vocabulary:

- Lit
- Const
- Concat
- Repeat
- Map
- Scan
- Merge

Map includes reversible relations such as XOR and modular addition. Scan captures
adjacent update structure. Merge exposes fields/planes separately.

The lesson is not to reuse these seven operators verbatim. The lesson is that a
small number of **composable reversible primitives** can cover many structures
that would otherwise become separate codec modes.

### 3.3 Target-directed synthesis is the key idea to steal

Naive program synthesis does:

    program -> execute -> compare against target

for enormous numbers of candidates.

Brevis searches backward from the exact target. Applying a reversible operator
**decomposes the target into the exact child streams that would be required**.
Most impossible programs are never instantiated or executed.

For ANVIL this is a major conceptual upgrade.

A candidate operator should preferably define both:

    G_theta(C1,...,Ck) = X

and a target decomposition:

    D_theta(X) -> (C1,...,Ck)

such that applying G to the resulting children reconstructs X exactly.

That changes search from "guess a program and test it" to:

> **Ask which bounded explanations are algebraically compatible with the target.**

This is especially promising for:

- field/plane splits;
- delta/XOR scans;
- repeat/periodic structure;
- deterministic transforms;
- structured copies;
- replay recipes;
- record/column decompositions;
- base + deviation representations.

### 3.4 Brevis validates the architecture, not general-purpose transfer

Brevis's published results are on model tensors, not arbitrary binaries. Its
production prior is learned from representative tensors, and the DSL exploits
tensor metadata.

ANVIL therefore must not use Brevis's ratio or GB/s results as evidence that the
same numbers transfer to general-purpose data.

What transfers is the architecture:

- finite typed DSL;
- universal literal fallback;
- expensive bounded search;
- exact complete-byte objective;
- target-directed decomposition;
- validated direct execution;
- no search model required by decoder.

This is probably the single highest-value architectural result found in this
research pass.

---

## 4. Compression synthesis did not begin with Brevis

Brevis is part of a longer line that makes this direction less speculative.

### 4.1 Real-time synthesis and SPDP

Burtscher et al. explored automatic composition of scientific-data transforms
in 2016. SPDP (2018) was selected from **9,400,320** four-stage algorithms
constructed from 48 components.

References:
- https://doi.org/10.1109/SC.2016.22
- https://doi.org/10.1109/DCC.2018.00042
- https://userweb.cs.txstate.edu/~mb92/papers/dcc18.pdf

The important lesson is that transform composition can expose combinations a
human would not necessarily select manually.

### 4.2 Adaptive per-file search

AdaptiveFC (2024) chains transformations and uses a genetic algorithm to select
a compressor per file. Its authors emphasize that the best pipeline differs
across files.

Reference:
- https://userweb.cs.txstate.edu/~burtscher/papers/essa24.pdf

### 4.3 Transformation importance is stage-dependent

Azami & Burtscher (ISPASS 2025) evaluate 74 transformation components across
195 scientific datasets. Their key systems lesson is highly relevant to ANVIL:

- transformation utility differs substantially between datasets;
- even datasets from the same simulation may prefer different pipelines;
- transformations useful in one pipeline stage may be unimportant in another;
- pruning low-value transformations can make deeper search feasible.

Reference:
- https://userweb.cs.txstate.edu/~burtscher/papers/ispass25.pdf

For ANVIL this argues against one global ranking of operators. Search priors
should be conditioned on **region anatomy and stage**, while final admission
remains exact-cost based.

---

## 5. OpenZL: resolved graphs and decode fusion

OpenZL supplies a complementary systems lesson.

Its graph architecture separates configurable encoder-side composition from a
universal decoder. The May 2026 v0.2 release introduced a native LZ graph and
explicitly emphasizes mixing entropy stages and **fusing multiple graph stages
into one operation**.

OpenZL's own Silesia benchmark reports its 64 KiB-window LZ graph at the same
2.74 ratio as the matched-window zstd level-1 comparison while reporting 466
MB/s compression and 2288 MB/s decompression versus 419 MB/s and 1254 MB/s for
that zstd comparison. These are OpenZL project benchmarks, not universal
cross-platform results.

References:
- https://github.com/facebook/openzl/releases/tag/v0.2.0
- https://github.com/facebook/openzl

The architectural lesson is more important than the number:

> A mathematically good graph can still lose if every node becomes a separate
> pass, allocation, buffer, or dispatch.

ANVIL's future synthesized explanation should therefore lower into **fused
decoder kernels**, not literally interpret a high-level tree node-by-node.

---

## 6. Grammar, morphisms, and the cost of staying inside LZ

### 6.1 Morphic structure is genuinely different from substring copies

Navarro & Urbina (2025) formalize repetitiveness based on string morphisms. They
show their L-system-based measure can be asymptotically much smaller than the
substring-complexity measure on some families, and combine morphisms with
bidirectional macro schemes into NU-systems that can be strictly smaller than
either family alone.

References:
- https://doi.org/10.1016/j.tcs.2025.115259
- https://users.dcc.uchile.cl/~gnavarro/abstracts/tcs25.html

This supports a strong ANVIL statement:

> **More effort spent improving LZ search cannot recover regularity that does
> not have a compact copy-paste explanation.**

### 6.2 Generalized SLPs strengthen the same point

Generalized Straight-Line Programs permit a rule body to be a compact program
that generates a potentially much larger sequence. The practical ANVIL
translation is not "implement a Turing-complete grammar." It is:

> Permit a bounded macro-instruction to generate many bytes when its semantics
> are cheap, reversible, and statically bounded.

Reference:
- https://arxiv.org/abs/2404.07057

### 6.3 Bidirectional/reference oracles should quantify causality tax

Computing the optimal bidirectional macro scheme or SLP is NP-hard. A 2026 KR
paper demonstrates exact solving on bounded instances using Answer Set
Programming modulo acyclicity.

Reference:
- https://doi.org/10.24963/kr.2026/66

This makes exact small-window oracles realistic as a **diagnostic**.

For sampled windows:

1. compute current ANVIL/LZ explanation cost;
2. compute a near/exact BMS or SLP lower oracle;
3. measure the gap;
4. inspect dependency depth and pattern type.

If the gap is tiny, forward references and grammar machinery are a distraction.
If it is large and concentrated in shallow DAGs or simple generative rules,
there is a concrete mechanism target.

### 6.4 Online string attractors name the same limitation

Whittington's online-string-attractor result links LZ factorization to the best
online strategy for an online attractor formulation and proves an asymptotic
online/offline separation on some morphic families.

Reference:
- https://arxiv.org/abs/2407.15599

For ANVIL, that is not an argument to abandon streaming. It is an argument to
**measure the opportunity cost of streaming before optimizing within its
constraints**.

### 6.5 2026 BBWT work is a useful BWT-side oracle, not an automatic replacement

Badkobeh, Bannai, I, and Köppl's 2026 work on the **bijective
Burrows-Wheeler transform (BBWT)** is directly relevant because ANVIL currently
uses ordinary BWT as a major ratio backend.

BBWT differs structurally from ordinary BWT: it is a bijection on strings rather
than requiring the ordinary transform's primary-index convention. The 2026
analysis proves that a bidirectional macro scheme of size O(r_B) can be induced
from the number r_B of BBWT runs, and relates r_B to LZ77 factor count. It also
shows an important warning: BBWT can be asymptotically worse than ordinary BWT
on some strings, while the **best cyclic rotation** has BBWT run count no larger
than the ordinary BWT run count; efficiently finding that optimal rotation
remains nontrivial.

Reference:
- https://doi.org/10.1007/s00224-025-10235-w

ANVIL implication:

> Treat BBWT/rotation choice as a representation oracle asking whether the
> current BWT coordinate system is leaving run structure on the table.

Do **not** replace libsais/BWT based on theory alone. A useful first experiment
would be offline and byte-focused:

- compare ordinary-BWT run statistics with BBWT and selected rotations on
  BWT-routed windows;
- count the primary-index/framing savings honestly;
- estimate inverse complexity and auxiliary-index compatibility;
- stop immediately if gains are tiny or decode becomes less regular.

The deeper point is that even a mature transform has a **representation search
space** above it.

---

## 7. Shannonic: rethink entropy symbols, not only entropy coders

Shannonic (MLSys 2026) is domain-specific to ML tensors, but its representational
idea is portable.

It partitions a value space into optimized subranges, then represents a value
as:

    (range index, offset within range)

Only the range index goes through ANS; the local offset is represented
separately. The paper reports a 530-byte codec state and compression within 1%
of the Shannon limit on its 8-bit quantized-model workload.

Reference:
- https://proceedings.mlsys.org/paper_files/paper/2026/hash/96f39c8de84678cb2a908cd52bfd7819-Abstract-Conference.html

The ANVIL lesson is broad:

> **The correct alphabet is itself a compression decision.**

Current ANVIL often turns semantic integers into varint bytes and then asks an
entropy coder to rescue the resulting byte distribution. That can hide
structure.

For integer/residual streams, candidate representations should include:

- range/class ID + low bits;
- frame-of-reference + local offset;
- sign/magnitude class + residual;
- quotient/remainder partitions;
- dense bit width + sparse exceptions;
- delta-of-delta classes;
- high-bits categorical stream + low-bits raw stream.

The entropy coder should be the final stage after symbolization has exposed the
simplest distribution.

### 7.4 Mature compositional codecs are prior art for operators, not for the synthesis architecture

Two existing general-purpose projects are useful reality checks.

**bzip3** uses a fixed per-block pipeline of RLE -> LZP -> BWT -> arithmetic
coding. Its source explicitly documents that the LZP dictionary/minimum-match
settings were chosen to collapse long redundant regions without interfering too
much with the later BWT/postcoder, and notes that the prepass can improve both
speed and compression on some inputs.

References:
- https://github.com/iczelia/bzip3
- https://github.com/iczelia/bzip3/blob/master/bzip3.1.in

This means that ANVIL combinations such as "long-match LZP before BWT" are
engineering/prior-art territory unless the mechanism materially differs.

**Kanzi** exposes an even broader manually composable vocabulary:

- BWT and bijective BWTS;
- LZ/LZX/LZP/ROLZ/ROLZX;
- RLT/ZRLT;
- MTF/rank/sorted-rank;
- executable, text, UTF, multimedia, packing, and DNA transforms;
- Huffman, ANS, range, FPAQ, context mixing, and neural-ish TPAQ backends.

References:
- https://github.com/flanglet/kanzi-cpp
- https://github.com/flanglet/kanzi-go/wiki/Main-page

The lesson is deliberately restrictive:

> **"Many transforms" and "transform pipelines" are not the ANVIL breakthrough.**

The potential architectural step beyond these systems is target-directed,
typed, exact-cost **explanation synthesis**, where operators are selected and
composed because their inverse semantics algebraically explain the particular
target region, and where the result is lowered to a bounded fused decoder
schedule.

---

## 7.5 REPLAY is already a real mechanism class: preflate-rs

Microsoft's current `preflate-rs` is direct, modern prior art for the exact
DEFLATE-reconstruction direction planned for I10-1B.

It does not merely decompress and hope a generic encoder recreates the same
bytes. Its architecture is explicitly predictive:

1. parse the original DEFLATE bitstream into literals and length/distance
   decisions;
2. fingerprint compressor behavior and parameters;
3. replay compression with that predicted configuration;
4. encode only the decision differences as corrections;
5. reconstruct the original DEFLATE bitstream bit-exactly from plaintext,
   parameters, and corrections.

The project states that unrecognized compressors still round-trip exactly; they
simply require more correction information. The corrections stream is CABAC
coded, and processing is chunked to bound memory.

Reference:
- https://github.com/microsoft/preflate-rs

This has three consequences for ANVIL.

### 7.5.1 I10-1B is adopt-class engineering, not a novelty candidate

The novelty target must not be "we can recreate DEFLATE from plaintext plus
side information." That mechanism class is established.

The useful I10-1B question is narrower and measurable:

> **Does a REPLAY explanation produce a non-dominated complete-cost point inside
> ANVIL's heterogeneous portfolio once plaintext compression, reconstruction
> metadata, correction bytes, decoder code size, memory, and replay time are all
> charged?**

This is still strategically valuable because it tests a fundamentally
different explanation from COPY or statistical coding.

### 7.5.2 preflate-rs should be an oracle/reference before it is a dependency

Before writing native ANVIL replay code, use the existing implementation to
answer anatomy questions on the frozen DEFLATE population:

- correction bytes per stream and compressor family;
- fraction explained by inferred global parameters versus local corrections;
- correction density and locality;
- whether the same corrections exhibit reusable structures;
- replay encode/decode cost;
- memory and code-size implications;
- behavior on recognized versus unknown compressor fingerprints.

This work belongs in GitHub Actions, not on the workstation.

The purpose is to determine whether ANVIL should:

- integrate/pin an existing replay engine;
- implement a deliberately narrower replay subset;
- or treat preflate only as an oracle while searching for a more general
  serialization-reconstruction abstraction.

### 7.5.3 The transferable breakthrough idea is predictor + sparse innovation

The deepest reusable pattern is:

    original serialization
      ~= deterministic predictor(semantic payload, global parameters)
         + sparse decision corrections

That is much broader than DEFLATE.

Potential future domains include any deterministic or nearly deterministic
serialization pipeline where the decoder can cheaply re-run a canonical model:

- image/container encodings;
- compiler/linker relocation choices;
- structured binary encoders;
- protocol/message serialization;
- database/page layouts;
- archive metadata.

The general research question becomes:

> **Can ANVIL identify the latent generating process of a byte region, store the
> semantic object, and encode only the innovation required to replay the exact
> original serialization?**

That is a legitimate path toward "do not store the bytes at all" without
pretending the reconstruction program is free.

### 7.6 Neural compressors quantify a large residual information gap — but do not solve ANVIL's systems problem

The 2025 peer-reviewed LMCompress result and several 2026 preprints reinforce a
useful fact: modern predictive models can expose source dependencies far beyond
those represented by ordinary byte-local or dictionary models.

LMCompress uses large generative models to supply probability distributions to
a lossless entropy coder and reports very large ratio gains across text, image,
audio, and video workloads. Nacrith combines a 135M-parameter language model
with online corrections and high-precision arithmetic coding; its preprint
reports 0.9389 bpb on enwik8. These systems are valuable evidence that substantial
predictive information remains invisible to classical local models.

References:
- https://doi.org/10.1038/s42256-025-01033-7
- https://arxiv.org/abs/2602.19626

For ANVIL, however, the model is part of the deployment state. A 500 MB external
weight file is not a free decoder prior merely because the compressed payload
does not contain it.

The correct accounting distinction is:

    archive bits
      + required shared model state
      + inference working set
      + decoder compute
      + model-distribution/version cost

unless a target environment independently guarantees that exact model as
pre-existing side information.

Therefore neural compression should primarily become an **information oracle**:

- identify windows where ANVIL's current explanation leaves unusually large
  predictive surprise;
- compare the gain against CTW/PPM and structural oracles;
- inspect which local/semantic features account for that gain;
- attempt to distill those features into a much smaller deterministic operator
  or bounded predictor.

This turns "the LLM compresses better" into a useful research question:

> **What compact state is the large predictor exploiting that ANVIL has not yet
> made decoder-visible?**

#### 7.6.1 StateSMix is a useful self-contained counterexample

StateSMix (2026) is especially informative because it removes the external-model
objection. It trains a small Mamba-style state-space model online, mixes it with
sparse n-gram contexts, uses arithmetic coding, and is implemented in C with
AVX2.

Reference:
- https://arxiv.org/abs/2605.02904

The reported scale behavior is the important part, not the headline win:

- it beats xz -9e on the authors' 1 MB, 3 MB, and 10 MB enwik8 prefixes;
- the public project reports **2.130 bpb on full 100 MB enwik8 versus about
  1.989 bpb for xz**, i.e. the advantage reverses at full scale;
- the implementation uses roughly 6 GB of RAM, dominated by sparse n-gram
  tables, and processes only around 2,000 tokens/s in the cited configuration.

This is a nearly ideal warning for ANVIL:

> A sophisticated predictor can expose real information and still move the
> wrong system-level Pareto axis.

Use StateSMix-like models as anatomy controls or distillation teachers unless a
much smaller/faster state representation emerges.

### 7.7 Frequency-ordered tokenization reinforces "choose the alphabet first"

Kalcher's 2026 frequency-ordered tokenization work provides a much cheaper
example of the same representational principle. It tokenizes text, assigns
smaller integer IDs to more frequent tokens, emits variable-length integers, and
then hands the result to an ordinary compressor. The paper reports improvements
even after vocabulary overhead and reports that preprocessing can reduce total
wall time for expensive backends because the transformed input becomes smaller.

Reference:
- https://arxiv.org/abs/2602.22958

The ANVIL lesson is not to add BPE as a universal transform. It is:

> **Symbol identity and numeric labeling are part of the model.**

If a semantic stream has a Zipf-like or clustered distribution, arbitrary raw
IDs can create needless high bits and poor varint behavior. Candidate ANVIL
streams should test:

- frequency-ranked local IDs;
- move-to-front / recency-ranked IDs;
- per-block dictionary IDs;
- class ID + local offset;
- stable global ID only when avoiding remap metadata is cheaper.

Again, total dictionary/remap cost must be charged. The important change is to
stop treating the numeric label of a symbol as semantically inevitable.

### 7.8 Bits-back coding: explanation choice need not always be paid naively

Bits-Back ANS (Townsend, Bird, Barber, 2019) shows a subtle but important limit
of the simple "latent/model bits + residual bits" accounting picture.

With a latent-variable generative model, a naive code appears to pay for both:

    latent z + data x given z

Yet if the encoder has an approximate posterior q(z|x), ANS's stack semantics
allow it to decode posterior-distributed latent choices from existing message
bits, use z to encode x under the generative model, encode z under its prior,
and eventually recover much of the information used to select z. Over chained
items, the effective rate approaches the variational coding objective rather
than simply paying a full independent latent description each time.

References:
- https://arxiv.org/abs/1901.04866
- https://proceedings.mlr.press/v97/kingma19a.html

This does **not** make explanation information free, and the initial seed/model
state, approximation gap, inference compute, and model distribution are all
real costs.

The ANVIL implication is nevertheless interesting:

> If many decoder-valid explanations describe the same target, transmitting the
> identity of one explanation as an ordinary independent symbol may be an
> information-theoretically wasteful formulation.

A distant future oracle could therefore compare:

- explicit explanation-ID cost;
- entropy-coded explanation families conditioned on anatomy;
- deterministic canonical explanation selection;
- latent/explanation-choice coding where ambiguous choice information is
  recoverable or recyclable.

The production bar should be extremely high. ANVIL should **not** adopt a
bits-back neural stack merely because the theory is elegant. The near-term use
is conceptual: do not assume that every latent structural decision must be paid
as an unrelated side-channel bit string.

---

## 8. Information-anatomy oracles: determine why a region is expensive

Before adding another codec mode, ANVIL should classify unexplained regions.

For sampled windows, compare several expensive offline oracles.

### A. Statistical oracle

Use CTW/PPM-class or a strong predictive model.

Question:

> Are there still predictable next-symbol bits?

If yes, the current statistical model is weak.

### B. Causal-reference oracle

Use a strong LZ parser/search.

Question:

> Is the problem merely candidate search/parse quality inside left-reference LZ?

### C. Noncausal/reference-DAG oracle

Use bounded BMS/LZRR/exact small-window solving.

Question:

> Is causality itself the problem?

### D. Grammar/generative oracle

Use SLP/Repeat/iterative-rule searches.

Question:

> Is the data better explained by generation than copying?

### E. Typed transform oracle

Try field splits, maps, scans, range/offset, base/deviation.

Question:

> Is the apparent entropy a coordinate-system problem?

### F. Replay oracle

Detect embedded representations with deterministic re-encoders.

Question:

> Is the original byte stream a serialization artifact that can be regenerated
> from a simpler semantic object?

The result should be an **explanation-gap map**, not another benchmark table.

For each region record:

- current ANVIL bits;
- oracle bits by explanation family;
- estimated decode work;
- metadata needed;
- family that closes the largest gap.

This turns research from mechanism roulette into information attribution.

---

## 9. Proposed research abstraction: ANVIL Explanation Synthesis

This section is deliberately an architecture proposal, not an implementation
authorization.

### 9.1 Typed region contract

Let a hole carry:

    Region<word_width, count>

meaning exactly count words of a fixed width, plus the target words that must be
reconstructed.

Every operator must specify:

- input/child types;
- exact output type;
- reversible semantics;
- target-directed decomposition when possible;
- encoded parameter cost;
- static output bound;
- static memory bound;
- decoder-kernel class.

### 9.2 Candidate operator families

The initial **research** vocabulary should unify current ANVIL ideas rather than
create parallel mode silos:

**Terminals**
- LITERAL
- CONST

**Composition**
- CONCAT
- REPEAT

**Referential**
- COPY
- COPY_PATCH
- COPY_DAG as oracle-only initially

**Reversible transforms**
- MAP_XOR_CONST
- MAP_ADD_CONST
- MAP_ZIGZAG
- SCAN_XOR
- SCAN_ADD
- SPLIT_FIELDS
- SPLIT_PLANES
- RANGE_OFFSET

**Structural generation**
- restricted periodic/affine generators
- bounded run/sequence generators
- small dictionary/rule expansion

**Serialization reconstruction**
- REPLAY_DEFLATE
- future replay classes only when reconstruction semantics are pinned

No arbitrary jumps, recursion without a static expansion bound, dynamic code
loading, or Turing-complete programs.

### 9.3 Search direction

The synthesizer should be target-directed whenever an operator has an inverse
decomposition.

Instead of testing arbitrary:

    SCAN_ADD(child)

against a target, derive the child immediately as adjacent differences.

Instead of testing:

    SPLIT_FIELDS(children)

derive the exact field streams.

Instead of testing:

    MAP_XOR_CONST(child)

derive child = target XOR constant for a bounded set of constants selected by
anatomy.

Instead of searching arbitrary repeat programs, detect candidate periods first,
then construct the exact required child.

This changes the scaling law of the search.

### 9.4 Search prior versus correctness

A learned or empirical prior may order expansion, but it must never be required
to decode and must never determine correctness.

Possible search-prior features:

- byte histogram;
- entropy by lane;
- zero masks;
- periodicity spectrum;
- field-change masks;
- record separators;
- estimated numeric width;
- repetition fingerprints;
- executable/code signatures;
- current mode routing.

The final selected representation is still the smallest/non-dominated exact
candidate actually found.

### 9.5 Objective must be Pareto-aware

Brevis minimizes complete serialized bytes because its target is archival.

ANVIL's goal is broader. A single hidden scalar such as:

    bytes + lambda * decode_time

is dangerous because the answer changes with arbitrary lambda.

The search should maintain a small local Pareto set using explicit profiles:

- **max-ratio:** minimize bytes subject to hard decode/memory ceilings;
- **balanced:** minimize bytes subject to an explicit decode target;
- **speed:** minimize decode time subject to an explicit rate budget.

Within each profile, exact bytes and measured/calibrated decoder classes can
prune candidates.

The archive should not silently change objective.

### 9.6 Current ANVIL modes already want to become a normalized explanation IR

A direct source inventory shows that the existing block-mode portfolio contains
many useful ideas, but several are bundled variants of a smaller set of
underlying explanation primitives.

Current rev-1 families include:

- ordinary LZ parses with several literal/context entropy models;
- rANS token coding;
- SPARSE-REF;
- SHAPE displacement modeling;
- TOPOLOGY residual/exception masks;
- TCOPY arithmetic-adjusted copies;
- ARI-REF additive reference;
- HOTOP compiled instruction books.

The rev-2 ratio path separately owns:

- direct bytes;
- context-1 permutation;
- line/column transforms;
- Brotli/BWT backend selection;
- BWT postcoder selection.

This is evidence for **factorization**, not another mode ID.

A normalized research IR could represent much of the current portfolio using
combinations such as:

    COPY
    COPY + PATCH
    COPY + predicted displacement
    COPY + arithmetic map + PATCH
    SPLIT control/data
    literal/residual leaf codec

The important consequences are:

1. search can compare combinations at the primitive level rather than only
   comparing preassembled modes;
2. lower-level decoder kernels can be reused across explanation families;
3. a primitive that repeatedly loses can be removed globally instead of being
   rediscovered inside several modes;
4. new mechanisms can be identified by whether they add a genuinely new
   explanatory primitive rather than a new packaging of old ones.

The current source also makes several **missing or only partially represented**
primitive families obvious:

- explicit CONST / REPEAT generation;
- general XOR/add MAP and SCAN rather than mode-specific arithmetic relations;
- typed field and bit-plane decomposition beyond the current line transform;
- RANGE_OFFSET / frame-of-reference integer representation;
- deterministic serialization REPLAY;
- bounded noncausal references / grammar generation as oracle-only primitives
  until their value is measured.

This is a major architectural reason to build the synthesis system as an oracle
first. It can tell us whether the current mode zoo actually spans the useful
space before any production format is redesigned.

---

## 10. The CPU is part of the format design

Current ANVIL source has one clearly mature architecture-specific kernel:
PCLMULQDQ CRC-32 with runtime dispatch and a scalar/slicing fallback.

The rest of the hot machinery remains substantially more scalar:

- rANS symbol loop;
- context-switched rANS dependency chain;
- uvar metadata decode;
- sparse/copy verification;
- token/control dispatch;
- many semantic stream operations.

The goal is not "add intrinsics." The goal is:

    useful reconstructed bytes /
    (retired instructions + cache cost + dependency cost)

### 10.1 Zen 3 target model

The current workstation is Zen 3. LLVM's Znver3 scheduling model, which cites
AMD Family 19h documentation, models:

- four general-purpose integer execution pipes;
- three AGUs;
- roughly four-cycle simple integer load-to-use latency;
- a common-case branch-mispredict penalty around 13 cycles;
- native PDEP/PEXT as one micro-op with modeled latency 3, issued through one
  integer pipe;
- 256-bit MOVMSK as a single modeled vector operation.

References:
- https://llvm.googlesource.com/llvm-project/+/refs/tags/llvmorg-17.0.6/llvm/lib/Target/X86/X86ScheduleZnver3.td
- https://docs.amd.com/r/en-US/57368-uProf-user-guide/14.4.-Useful-URLs

These are scheduling-model values, not a substitute for measurement. They are
enough to guide which kernels are worth prototyping.

### 10.2 Highest-EV AVX2 primitive: wide event/mismatch masks

For 32 input bytes:

1. load target/source;
2. compare or XOR;
3. map equality/classification into a 32-bit mask;
4. use TZCNT / POPCNT to enumerate sparse events.

This kernel supports multiple higher-level mechanisms:

- first mismatch / match extension;
- changed-lane detection;
- digit/sign/separator scanner;
- zero/nonzero masks;
- 4-byte/8-byte field-change maps;
- transform-site detection.

The important principle is that the SIMD kernel emits **compact semantic
events**, not another full byte array.

### 10.3 PDEP/PEXT are worth considering, but selectively

Earlier AMD generations made PDEP/PEXT unattractive. The Znver3 scheduling model
treats them as native 3-latency operations, and current uops.info Zen-3
measurements agree: register PEXT/PDEP are one executed uop, three-cycle latency,
with measured one-instruction-per-cycle throughput.

By contrast, uops.info measures AVX2 `VPGATHERDD ymm` on Zen 3 at 39 executed
uops and roughly eight-cycle throughput. That is a useful reality check:
**vector width does not make random-access dependencies cheap.** For LF mapping
or other scattered tables, several interleaved ordinary scalar loads can be a
better machine architecture than one wide gather.

References:
- https://uops.info/html-instr/PDEP_R64_R64_R64.html
- https://uops.info/html-lat/ZEN3/PEXT_R32_R32_R32-Measurements.html
- https://uops.info/html-instr/VPGATHERDD_YMM_VSIB_YMM_YMM.html

Potential ANVIL uses for PEXT/PDEP:

- gather/scatter small control bitfields;
- compact lane-change masks;
- pack selected fixed-width flags.

However they share a specific execution resource in the model. A loop that
depends on one PEXT every iteration may simply create a new serial bottleneck.

Therefore:

> Use PEXT/PDEP for sparse control manipulation only after comparing them with
> shifts/masks/table lookup in a remote microbenchmark.

### 10.4 zlib-ng is the right systems reference for ISA dispatch

zlib-ng currently dispatches architecture-specific kernels for:

- checksums;
- slide hash;
- 256-byte match comparison;
- inflate chunk copying;
- longest match;
- AVX2/AVX-512/NEON and other targets.

References:
- https://github.com/zlib-ng/zlib-ng
- https://github.com/zlib-ng/zlib-ng/blob/develop/functable.c

The lesson for ANVIL is the separation:

- stable semantic operation;
- several ISA kernels;
- runtime feature dispatch;
- identical wire.

Hardware optimization should almost always be **wire-invisible**.

### 10.5 Do not vectorize inherently serial state blindly

A scalar rANS state is a dependency recurrence. A context-switched rANS stream
has an additional dependency because the previous decoded symbol chooses the
next model.

The successful strategy is independent states/streams, not wider arithmetic on
one dependency chain.

rans_static contains manually vectorized 8/16/32-state AVX2 variants and is a
useful reference for what sufficient independent work looks like.

Reference:
- https://github.com/jkbonfield/rans_static

ANVIL should first identify streams/blocks that are truly independent. Any rate
penalty from introducing resets/interleaving must be fully charged.

### 10.6 Metadata should be redesigned for dense decode

Stream VByte's lasting insight is control/data separation. Width controls can be
read ahead while packed values flow through a regular decoder.

Reference:
- https://arxiv.org/abs/1709.08990

TurboPFor is a useful implementation catalog for:

- frame-of-reference;
- delta;
- zigzag;
- patched FOR;
- SIMD bitpacking;
- group-varint-style layouts.

Reference:
- https://github.com/powturbo/TurboPFor-Integer-Compression

This is a strong candidate for ANVIL's length/distance/rule-ID/correction-index
streams because these are semantic integers, not naturally byte symbols.

### 10.7 Spend a few bytes to manufacture instruction-level parallelism when it pays

Zstandard's format contains an unusually clean precedent: Huffman literals may
be split into four independent bitstreams. The format documentation explicitly
states that the four-stream form is faster because it exposes instruction-level
parallelism, while costing about 7.3 additional bytes on average.

Reference:
- https://github.com/facebook/zstd/blob/dev/doc/zstd_compression_format.md

That is almost exactly the I10-1A lesson in another subsystem:

> **a small rate charge can be the correct Pareto move if it breaks a long
> dependency chain into independent lanes.**

ANVIL should treat dependency-breaking metadata as a legitimate representation
primitive. Candidate uses include:

- interleaved entropy states;
- independently restartable residual lanes;
- bounded BWT walks;
- independent record/field streams;
- parallel patch lanes.

The charge must always be explicit, and max-ratio must retain the smaller point.

### 10.8 False dependencies are a real decompressor bottleneck class

An August 31, 2026 change in klauspost/compress's Huff0/Zstd assembly is a
useful current systems lesson. Four otherwise-independent Huffman streams were
partly serialized by byte writes into partial x86 registers. Replacing the first
partial-register write with a zero-extending full-register definition broke the
false loop/stream dependency; the same source pattern also lowered more cleanly
to ARM64. A related fix removed false dependencies from an overlapping-copy byte
loop.

Reference:
- https://github.com/klauspost/compress/commit/629c2ea148ae6ddbd3b70bc1e35076aaf94b7143

ANVIL implication:

> When we create parallel explanation lanes, inspect the **generated machine
> dependency graph**, not merely the source-level independence.

For future remote profiling, collect disassembly and look for:

- partial-register merges;
- loop-carried address dependencies;
- unnecessary read-modify-write chains;
- serial bit-reader state;
- repeated bounds branches;
- dependency-forming small-copy loops.

This is a stronger rule than "use SIMD": preserve independence all the way down
to the micro-ops.

### 10.9 libsais itself reinforces the memory-level-parallelism thesis

Current libsais documentation recommends auxiliary indexes for optimal inverse
BWT performance and notes that its implementation is highly sensitive to memory,
software prefetching, branch elimination, loop unrolling, and pass fusion. Its
inverse BWT uses a bi-gram LF mapping so two symbols can be reconstructed per
step, reducing cache misses.

Reference:
- https://github.com/IlyaGrebnov/libsais

This clarifies why I10-1A worked so well: the auxiliary wire did not make the LF
mapping mathematically cheaper; it exposed **more independent starting points**
to a memory/dependency-limited computation.

A future BWT optimization should therefore prioritize:

1. memory-level parallelism among independent walks;
2. prefetch quality and table locality;
3. context/buffer reuse;
4. only then AVX2 gathers or other vector machinery.

A wide gather over dependent/random LF states is not automatically superior to
well-interleaved scalar loads.

### 10.10 Current-source hardware opportunity map

A source audit after I10-1A gives a much more precise priority order than
"SIMD the decoder."

#### Already architecturally strong: CRC

`crc32()` is the pattern to imitate:

- stable semantic operation;
- scalar/slicing fallback;
- PCLMUL-specialized kernel;
- runtime CPUID dispatch;
- identical wire and corruption semantics.

This is mature enough to serve as ANVIL's template for future ISA-specialized
operations.

#### High-EV representation target: semantic varints

`get_uvar()`, `read_varint_pull()`, and `read_varint_bytes()` are all
byte-at-a-time continuation-bit loops. They appear across stream framing,
lengths, distances, grammar symbols, and mode metadata.

This does **not** mean "write a SIMD varint decoder first." It means the wire is
frequently presenting semantic integers in a representation that inherently
creates branch/length dependencies.

Research priority:

- census actual integer distributions and stream lengths;
- compare exact bytes for current uvar, Stream-VByte-like control/data,
  frame-of-reference + bitpack, PFor, and range/offset;
- only build an ISA decoder for a representation that wins complete cost.

Two established references define the useful baseline:

- Masked VByte demonstrates that the existing continuation-byte format itself
  can be SIMD-decoded substantially faster than a branchy scalar loop;
- Stream VByte goes further by separating control and data streams so widths can
  be consumed ahead of packed values, which is a much more regular machine
  representation.

References:
- https://arxiv.org/abs/1503.07387
- https://arxiv.org/abs/1709.08990

#### Serial by construction: one-state rANS

`rans_decode()` is one state recurrence:

    slot -> symbol -> frequency/start -> next state -> renormalize

There is little useful AVX2 width inside one such state. The useful experiment
is **multiple independent states**, explicitly charging their initial states and
partition metadata.

This should be evaluated as another rate/speed Pareto choice, not silently
substituted into max-ratio.

#### Even more serial: context rANS

`ctx_rans_decode()` adds a second dependency:

    previous decoded symbol -> next context/model

on top of the rANS state recurrence.

Trying to vectorize this exact chain is low-EV. Plausible alternatives are:

- independent restart stripes with explicit initial contexts;
- context models on naturally separate semantic streams;
- use the context coder only where its rate gain exceeds the dependency cost.

The anatomy oracle should measure that trade before any new wire exists.

#### Wire-invisible candidate: multi-symbol Huffman tables

The current fast Huffman path uses a 12-bit table but emits one symbol per
lookup. A wider decode-table entry can potentially emit two or more symbols
from one prefix when sufficient bits are known, analogous to multi-symbol
Huffman decoders used by mature codecs.

This is attractive because it may be **wire-invisible**. The costs are larger
decode tables, build time, cache pressure, and fallback complexity. It belongs
in a remote micro/whole-codec experiment only if mode-4 traffic is significant.

A separate four-stream Huffman representation is a different experiment: it
spends wire to create independent states and must be treated as a new Pareto
point.

#### High-EV wire-invisible candidate: default-with-exceptions bulk reconstruction

The fused `StreamPull` mode-5 path currently tests one mask bit per requested
byte. Its semantics are naturally separable:

1. fill a run with the default value;
2. enumerate exception bits in word-sized masks;
3. patch exception values.

That maps directly to bulk memset/vector stores + POPCNT/TZCNT sparse patches,
with no wire change. It is a much cleaner SIMD/word-parallel target than
context-rANS.

#### Copy kernels: specialize overlap classes, not generic memcpy folklore

Several token decoders still copy overlapping matches byte-by-byte. The correct
kernel depends on distance:

- `dist >= len`: ordinary memcpy-like copy;
- medium overlap: repeat a seed pattern with wide stores;
- very small distances: specialized pattern expansion/chunk-set;
- sparse-corrected copy: base copy then sparse patch pass.

zlib-ng's architecture-specific inflate chunk-copy kernels are a useful
reference because they specialize the **semantic operation** rather than
vectorizing a generic token loop.

LZ4 makes the same principle especially explicit: its decoder has wide
wild-copy paths for ordinary offsets and dedicated construction for tiny
offsets such as 1, 2, and 4. At offset 1, for example, the operation is
semantically "repeat one byte", so materializing an 8-byte repeated seed and
issuing wide copies is more appropriate than preserving a bytewise dependency
chain.

References:
- https://github.com/lz4/lz4/blob/dev/lib/lz4.c
- https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md

#### Sparse patch decode is already close to the right abstraction

The hot-op path reads 32-bit masks, uses POPCNT for residual accounting, and
enumerates set bits with countr_zero / clear-lowest-set-bit. That is already a
compact event representation.

The likely SIMD opportunity is therefore more on the **encoder/anatomy side**
(wide compare -> mask) and on the base copy, not replacing the sparse event
loop with vector instructions for their own sake.

#### StreamPull fusion is directionally correct

`StreamPull::pull_bytes()` already avoids per-byte codec dispatch for raw,
rANS, and fast-Huffman bulk reads. That confirms an important rule for the
future Explanation Machine:

> normalize/high-level-dispatch outside the hot region; expose long homogeneous
> kernel runs inside it.

The synthesis architecture should lower toward this shape rather than re-create
a bytecode interpreter in the reconstruction loop.

### 10.11 Whole-buffer information is itself a performance resource

`libdeflate` is a useful counterexample to the idea that a newer format is
required for a large implementation-speed improvement. It stays within DEFLATE
but deliberately targets whole-buffer operation.

Its decompressor documents several reasons for its speed relative to vanilla
zlib:

- word accesses rather than byte accesses when reading input;
- word accesses rather than byte accesses when copying matches;
- faster Huffman decoding;
- a larger bit-buffer variable that requires less frequent refill.

The project also explicitly avoids a streaming API because streaming adds
complexity and slows the fast paths it is designed around.

References:
- https://github.com/ebiggers/libdeflate
- https://github.com/ebiggers/libdeflate/blob/master/lib/deflate_decompress.c

ANVIL implication:

> **Known output extent, block boundaries, and validated framing should be
> treated as optimization inputs, not merely safety metadata.**

If a block already declares exact reconstructed size, the hot decoder should be
free to use:

- bounded wild-copy kernels with a separately safe tail;
- larger bit-buffer refills;
- up-front buffer allocation;
- fewer repeated boundary branches;
- fused operation batches whose output extent is statically known.

This also feeds back into Explanation Synthesis: every synthesized node should
carry exact output extent whenever possible so lowering can choose whole-region
kernels rather than a generic streaming interface.

### 10.12 PivCo-Huffman: redesign the decoding problem for SIMD

Zukowski's 2026 **PivCo-Huffman** is directly relevant to ANVIL's current
canonical-Huffman stream mode. Instead of accelerating the conventional
one-code-at-a-time table walk, it represents Huffman decoding using
wavelet-tree-style per-node bitmaps and processes blocks of symbols with SIMD
partition/merge primitives. A flat-subtree path collapses multiple tree levels
into packed local codes and direct symbol lookup.

The paper reports decode throughput above the tested production Huffman
baselines and also explores selectively ANS-coding skewed partition bitmaps to
move ratio toward ANS while retaining high decode speed. The public project has
scalar, NEON, x86 SSE/AVX2, and AVX-512-oriented implementations.

References:
- https://arxiv.org/abs/2606.05765
- https://github.com/MarcinZukowski/pivco-huffman

The relevant lesson is not "replace ANVIL Huffman with PivCo immediately."
PivCo is currently a research project and its own Zen-3 benchmark notes include
distributions where the new representation loses.

The stronger lesson is architectural:

> **If a scalar decoding recurrence resists SIMD, ask whether an equivalent
> representation can turn it into block operations that SIMD naturally likes.**

For ANVIL this creates a concrete oracle experiment:

1. identify how many bytes/time are actually spent in stream mode 4;
2. on those exact semantic streams, compare current canonical Huffman against a
   PivCo-style block representation;
3. charge its tree/bitmap wire completely;
4. keep both rate and decode throughput;
5. reject it if the useful traffic share is too small or Zen-3 behavior is not
   robust.

This should remain isolated from the Explanation Machine. It is an entropy-leaf
architecture experiment, not a new explanation class.

### 10.13 dtANS reinforces hardware-layout co-design, but is GPU-specific evidence

Schätzle, Pegolotti, and Püschel's 2026 **dtANS** adapts tabled ANS for fast
parallel GPU decoding inside sparse matrix-vector multiplication. Its encoding
interleaves per-thread compressed word streams so a warp's reads are coalesced,
and decoding is fused directly with the consumer computation.

Reference:
- https://arxiv.org/abs/2603.01915

ANVIL should not import a GPU-oriented entropy format into its CPU codec from
this result. The transferable principle is narrower:

> **Compressed layout, independent-state partitioning, memory-access pattern,
> and the consumer kernel should be designed together.**

This is further support for ANVIL's planned fused lowering and multi-state
entropy experiments. A representation that is a few bits worse may still
dominate if it turns serial/random memory traffic into regular independent
lanes. Conversely, a mathematically elegant entropy state that causes cache
misses or a dependency chain may be system-level inferior.

---

## 11. A SIMD Stage-1 anatomy scanner should precede more semantic modes

The project should borrow the execution pattern of high-performance parsers:

    wide classify
      -> bit masks
      -> sparse event records
      -> expensive analysis only at events

Potential masks:

- byte equality against common separators;
- ASCII digit/sign;
- zero byte / all-zero word;
- high-bit classes;
- 4-byte word changes;
- 8-byte word changes;
- branch/call opcode candidates;
- newline/record delimiters;
- quote/backslash states where relevant.

The scanner is useful even if no new wire format results. It can feed:

- anatomy reports;
- transform candidate generation;
- typed region boundaries;
- copy-patch field masks;
- replay detectors.

Research metric:

    useful explanations discovered /
    (bytes scanned + verification work)

Raw scanner GB/s is insufficient. A fast scanner that floods later stages with
false positives is a loss.

---

## 12. Decoder lowering: synthesize high level, execute low level

A high-level explanation tree should not imply an interpreter branch for every
node.

The path should be:

    synthesized explanation
      -> validated normalized IR
      -> small kernel schedule
      -> fused executor

Example:

    SPLIT_FIELDS -> SCAN_XOR -> RANGE_OFFSET -> LITERAL

might lower into one specialized loop that:

- loads a vector;
- reconstructs field deltas;
- reads compact class controls;
- inserts offsets;
- stores full words.

Likewise:

    COPY -> PATCH fixed-4-byte lanes

can lower to a bulk copy plus SIMD/event-mask patch pass, rather than generic VM
dispatch per patch.

This is how ANVIL can gain expressivity without paying a branch-heavy general-VM
tax.

### 12.1 Equality saturation belongs in lowering, not source explanation search

ANVIL's target-directed synthesizer and its decoder-lowering problem are
different search problems and should use different machinery.

The synthesizer asks:

> Which exact explanation can reconstruct these target bytes cheaply?

That search should remain target-directed: invert/restrict an operator against
the known target, derive the exact child streams, and prune impossible
explanations before constructing them.

The lowering stage instead asks:

> Given one exact explanation, which semantically equivalent execution form is
> cheapest on this machine/profile?

This second question is where **e-graphs / equality saturation** are a strong
fit.

Equality saturation stores many equivalent expressions without destructively
choosing a rewrite order. Recent work pushes this idea beyond a one-shot
external optimizer: Merckx et al. (2026) represent the e-graph as a persistent
compiler abstraction so equalities can survive and interact with ordinary
compiler transformations across abstraction levels.

References:
- https://arxiv.org/abs/2602.16707
- https://doi.org/10.1145/3815481

For ANVIL, a lowering e-graph could retain equivalences such as:

- MAP_XOR(0, x) == x;
- nested constant maps folded into one map;
- SCAN_ADD plus known initial state lowered to a specialized prefix kernel;
- compatible field splits fused with the consumer transform;
- COPY + PATCH represented as either copy-then-sparse-patch or a fused
  fixed-lane reconstruction kernel;
- consecutive literals/coalescible outputs combined into one bulk store region;
- independent child regions reordered when the wire/semantic contract permits;
- scalar, AVX2, AVX-512, NEON, or other ISA kernels represented as equivalent
  implementations of the same normalized operation.

The e-graph must **not** become part of the compressed format. It is an
encoder/compiler-side optimization structure. The archive still contains the
small validated explanation/kernel IDs required by the decoder.

### 12.2 Extraction complexity is a design constraint

There is an important theoretical warning. Optimal extraction from a general
e-graph is NP-hard and is even hard to approximate within a constant factor in
the worst case. Goharshady, Lam, and Parreaux (OOPSLA 2024) show, however, that
optimal extraction becomes tractable for sparse e-graphs of bounded
treewidth/pathwidth, and report practical optimal extraction on real compiler
e-graphs with low treewidth.

Reference:
- https://doi.org/10.1145/3689801

This should influence ANVIL's IR design from day one:

> **Keep the lowering equality space deliberately sparse and local.**

Concretely:

- cap rewrite rounds/nodes;
- do not saturate arbitrary arithmetic identities globally;
- rewrite within one bounded explanation region/kernel group;
- prefer typed rewrites that preserve known shape/output extent;
- separate source-synthesis alternatives from lowering equivalences;
- canonicalize aggressively after obviously dominating identities;
- retain only a small set of machine-relevant alternatives.

The explanation DSL is already intended to be shallow and bounded, which makes
this much more plausible than equality-saturating a general program.

### 12.3 Extraction should preserve a Pareto set, not hide it behind one lambda

A standard e-graph extractor usually minimizes one scalar cost. ANVIL has
multiple first-class axes:

- serialized bytes / decoder instruction IDs;
- predicted/measured decode cycles;
- working-set/cache footprint;
- temporary memory;
- decoder code/table footprint;
- required ISA.

Collapsing those into one arbitrary scalar recreates the exact objective problem
ANVIL has spent Iteration 10 avoiding.

For bounded local e-graphs, extraction should instead retain a small
**non-dominated frontier per e-class** under explicit profile constraints.
For example:

- max-ratio extraction can reject any lowering that changes wire size and then
  minimize decode work among byte-identical forms;
- balanced extraction can admit a bounded byte charge and retain several
  byte/cycle/memory alternatives;
- a memory-constrained extraction can eliminate implementations whose working
  set exceeds the profile budget.

If the frontier grows too large, that is a signal that the lowering search space
is not sufficiently bounded—not permission to silently invent a lambda.

### 12.4 The practical architecture becomes synthesis -> equality-preserving lowering

The resulting compiler-like architecture is:

    target bytes
      -> anatomy / oracle signals
      -> target-directed explanation synthesis
      -> exact explanation IR
      -> local equality saturation / canonicalization
      -> Pareto-aware extraction
      -> ISA-aware fused kernel schedule
      -> serialized bounded explanation

This division of labor is important.

- **Synthesis discovers explanatory power.**
- **Equality saturation removes phase ordering from equivalent implementations.**
- **Extraction chooses a machine/profile point.**
- **The decoder executes; it does not search.**

That gives ANVIL a path to a richer explanation vocabulary without forcing
either an exponential runtime decoder or a brittle hand-ordered lowering pass
pipeline.

---

## 13. Proposed research sequence before committing to another breakthrough mode

### R-A — Build the explanation-gap methodology

No format change.

Define sampled windows and record current ANVIL cost against:

- statistical oracle;
- strong causal LZ;
- bounded BMS/SLP;
- restricted transform synthesis;
- repeat/generative synthesis.

Deliverable: explanation-gap report.

### R-B — Build a target-directed synthesis oracle

No production decoder yet.

Start with only:

- Lit;
- Const;
- Concat;
- Repeat;
- XOR/Add Map;
- XOR/Add Scan;
- fixed field/plane split;
- range/offset.

Use exact serialized hypothetical costs and hard depth/node budgets.

The initial question is not "does the prototype beat ANVIL?"

The question is:

> Which operator families repeatedly explain bits that ANVIL currently stores?

### R-C0 — Sweep the existing BWT memory/rate/decode representation

No new codec mechanism.

Use the already-wired `--bwt-subblock` framing with `--bwt-aux=on` and sweep
caps on GitHub Actions, initially:

- 8 MiB;
- 16 MiB;
- 32 MiB;
- 64 MiB;
- 128 MiB / effectively unsplit control.

For every cap record:

- exact compressed bytes;
- exact routing;
- paired/interleaved decode timing where practical;
- encode time;
- peak encode/decode RSS;
- roundtrip/hash;
- subblock count.

The purpose is to discover whether a **memory-bounded BWT Pareto point** already
exists. Do not alter the 128 MiB max-ratio default merely because a smaller cap
uses less memory.

### R-C — Build the Stage-1 AVX2 anatomy kernel

Wire-invisible research tool.

Scalar reference + AVX2 path. Emit identical event masks/records.

Run performance work only in GitHub Actions.

### R-D — Integer metadata anatomy

Measure distributions of:

- lengths;
- distances;
- patch positions;
- rule IDs;
- run counts;
- transform deltas.

Compare hypothetical exact wire for:

- current varint-byte + stream coding;
- Stream-VByte-like control/data;
- FOR + bitpack;
- PFor;
- range/offset;
- sparse exceptions.

No decoder-format change until a family wins broadly.

### R-E — Proceed with I10-1B as an isolated REPLAY experiment

DEFLATE reconstruction remains valuable because it tests a qualitatively
different explanation:

> do not preserve the compressed byte representation; preserve the semantic
> payload plus enough deterministic reconstruction information to recreate that
> representation exactly.

It should remain causally isolated from explanation-synthesis work.

### R-F — External-front validation

Any "breakthrough" claim still requires:

- same-runner reference codecs;
- exact bytes;
- encode/decode;
- memory;
- decoder binary size;
- AITDCC-style held-out/generalization discipline.

---

## 14. Gates for a true ANVIL breakthrough

A mechanism should not be called a breakthrough because it is novel.

It should satisfy most of the following:

1. **New explanatory power**
   - makes meaningful source bytes decoder-deterministic that previous ANVIL
     mechanisms had to store.

2. **Rate significance**
   - gain survives complete metadata/model/instruction accounting.

3. **Whole-codec impact**
   - improvement is visible after routing, framing, CRC, allocations, and real
     execution costs.

4. **Pareto movement**
   - creates a point not dominated by the current ANVIL/reference front.

5. **Hardware plausibility**
   - hot decode consists predominantly of regular loads/stores, dense arithmetic,
     predictable control, or parallel independent states.

6. **Generalization**
   - mechanism is not a one-file signature unless intentionally a
     format-specific replay operator.

7. **Bounded semantics**
   - decoder can validate length, memory, and work before executing.

8. **Small decoder tax**
   - code size and tables are charged.

9. **Mechanism-level novelty only after evidence**
   - prior-art analysis follows a demonstrated mechanism, not the reverse.

---

## 15. What should not happen next

Do not:

- invent another entropy coder merely to shave a fraction of a bit from a fixed
  representation;
- add a Turing-complete decoder VM;
- make an LLM part of the required decoder;
- add forward references before measuring the causality gap;
- add AVX intrinsics without first defining the semantic kernel they accelerate;
- entropy-code every integer stream just because entropy coding exists;
- benchmark CPU-heavy changes on the workstation or homelab;
- tune to formerly hidden AITDCC files and then call the result generalization;
- mix I10-1B source changes into a synthesis experiment;
- accept a stage benchmark as a codec result.

---

## 16. Research hypotheses now worth falsifying

### H1 — Target-directed explanation synthesis exposes new compressibility

A restricted reversible DSL can find smaller complete explanations than
ANVIL's current hand-selected mode portfolio on a meaningful fraction of
heterogeneous windows.

**Falsifier:** after exact metadata charging, the synthesized winner almost
always lowers to an existing ANVIL explanation.

### H2 — Most remaining gains are representational, not entropy-coder gains

Once residuals are expressed in better coordinates/classes, ordinary
rANS/Huffman/raw/bitpack leaves are sufficient.

**Falsifier:** strong statistical oracles consistently show large residual gaps
after the best structural explanation, and those gaps cannot be captured by
cheap deterministic models.

### H3 — Causality is expensive only in identifiable pockets

BMS/SLP oracles materially beat causal LZ only in a minority of windows with
recognizable shallow dependency/generative structure.

**Falsifier:** the gap is broad and large across ordinary data, implying the
production representation needs a more general noncausal mechanism.

### H4 — Semantic integer streams should leave byte-oriented entropy coding

At least some metadata streams achieve a better size/decode frontier with
control/data separation or dense integer coding.

**Falsifier:** complete header/table/control costs eliminate the gain on
canonical corpora.

### H5 — A vector event scanner can lower encoder search cost without changing wire

AVX2 Stage-1 masks can identify useful candidate sites with enough precision to
reduce downstream candidate work.

**Falsifier:** false-positive verification dominates, or scalar specialized
scanners already saturate memory bandwidth.

### H6 — Fused explanation execution is necessary for expressive programs

A direct node interpreter loses materially to a fused normalized kernel schedule
for otherwise identical explanation semantics.

**Falsifier:** instruction dispatch overhead is negligible compared with the
underlying memory/entropy work.

---

## 17. Research references

### Program synthesis / executable explanations

- Shi et al., **Lossless Tensor Compression as Program Synthesis** (Brevis),
  2026: https://arxiv.org/abs/2608.02162
- Brevis implementation: https://github.com/jiekeshi/Brevis
- Yoran et al., **The KoLMogorov Test: Compression by Code Generation**,
  ICLR 2025:
  https://proceedings.iclr.cc/paper_files/paper/2025/hash/db1d69ee01c2b42554f8e14e6f8ca8b6-Abstract-Conference.html
- Burtscher et al., **Real-Time Synthesis of Compression Algorithms for
  Scientific Data**, SC 2016: https://doi.org/10.1109/SC.2016.22
- Claggett et al., **SPDP**, DCC 2018:
  https://doi.org/10.1109/DCC.2018.00042
- Rodriguez et al., **Adaptive Per-File Lossless Compression of Floating-Point
  Data**, 2024: https://userweb.cs.txstate.edu/~burtscher/papers/essa24.pdf
- Azami & Burtscher, **Identifying Important Data Transformations for
  Synthesizing Effective Lossless Compressors**, ISPASS 2025:
  https://userweb.cs.txstate.edu/~burtscher/papers/ispass25.pdf

### Graph / specialization / lowering architecture

- OpenZL: https://github.com/facebook/openzl
- OpenZL v0.2.0:
  https://github.com/facebook/openzl/releases/tag/v0.2.0
- Merckx et al., **E-Graphs as a Persistent Compiler Abstraction**, 2026:
  https://arxiv.org/abs/2602.16707
- Willsey et al., **egg: Fast and Extensible Equality Saturation**:
  https://doi.org/10.1145/3815481
- Goharshady, Lam, Parreaux, **Fast and Optimal Extraction for Sparse Equality
  Graphs**, OOPSLA 2024:
  https://doi.org/10.1145/3689801
- Arbore, Cheung, Willsey, **Optimism in Equality Saturation**, PLDI 2026:
  https://doi.org/10.1145/3808302

### Grammar / repetitiveness / noncausal oracles

- Navarro & Urbina, **Repetitiveness Measures Based on String Morphisms**,
  TCS 2025: https://doi.org/10.1016/j.tcs.2025.115259
- Navarro, Olivares, Urbina, **Generalized Straight-Line Programs**:
  https://arxiv.org/abs/2404.07057
- Banbara et al., **Optimal Dictionary-Based Compression with Answer Set
  Programming**, KR 2026: https://doi.org/10.24963/kr.2026/66
- Whittington, **Online String Attractors**:
  https://arxiv.org/abs/2407.15599

### Entropy representation

- Ibrahim et al., **Shannonic: Efficient Entropy-Optimal Compression for ML
  Workloads**, MLSys 2026:
  https://proceedings.mlsys.org/paper_files/paper/2026/hash/96f39c8de84678cb2a908cd52bfd7819-Abstract-Conference.html
- Townsend, Bird, Barber, **Practical Lossless Compression with Latent Variables
  using Bits Back Coding**, ICLR 2019:
  https://arxiv.org/abs/1901.04866
- Kingma, Abbeel, Ho, **Bit-Swap: Recursive Bits-Back Coding for Lossless
  Compression with Hierarchical Latent Variables**, ICML 2019:
  https://proceedings.mlr.press/v97/kingma19a.html
- Lemire et al., **Stream VByte**:
  https://arxiv.org/abs/1709.08990
- TurboPFor:
  https://github.com/powturbo/TurboPFor-Integer-Compression

### Hardware / decoder implementation references

- zlib-ng: https://github.com/zlib-ng/zlib-ng
- zlib-ng runtime dispatch:
  https://github.com/zlib-ng/zlib-ng/blob/develop/functable.c
- rANS SIMD/interleaving reference:
  https://github.com/jkbonfield/rans_static
- LLVM Znver3 scheduling model:
  https://llvm.googlesource.com/llvm-project/+/refs/tags/llvmorg-17.0.6/llvm/lib/Target/X86/X86ScheduleZnver3.td
- AMD uProf / Family 19h optimization-document index:
  https://docs.amd.com/r/en-US/57368-uProf-user-guide/14.4.-Useful-URLs

### External validation

- **2026 Algorithmic Information Theory Data Compression Challenge**:
  https://arxiv.org/abs/2606.17712
- Dataset/challenge resources:
  https://aitdcc.github.io/

---

## 18. Current direction after this addendum

The project now has three deliberately separate lanes.

### Engineering lane

I10-1B: bit-exact DEFLATE replay.

Purpose: test and potentially adopt a strong REPLAY explanation for embedded
serialization redundancy.

### Breakthrough-discovery lane

ANVIL Explanation Synthesis + information-anatomy oracles.

Purpose: discover explanation classes that make more source bytes
decoder-deterministic, without first committing them to the production format.

### Machine lane

AVX2 Stage-1 anatomy, integer-metadata representation, fused decode, independent
entropy-state execution.

Purpose: make the eventual explanation vocabulary map cleanly onto commodity
hardware.

The most important discipline is that these lanes exchange **evidence**, not
uncontrolled source changes.

---

## 19. One-sentence research thesis

> **ANVIL should search for compact, bounded, hardware-cheap programs that make
> the input inevitable at the decoder, then entropy-code only what no such
> explanation can reconstruct.**
