# 09 — Inside the Box / Outside the Box Opportunity Map

This document intentionally permits more speculation than the rest of the audit. Each idea is still tied to measured ANVIL behavior and is labeled so speculation cannot leak into the result record.

## Part I — Inside the box

“Inside the box” means: keep the current architecture and exploit obvious measured gaps more intelligently.

## I1. Finish the backend portfolio we already accidentally proved useful

The direct BWT pass is strong enough that the first priority should be exploiting it cleanly rather than immediately inventing backend 3.

Sequence:

1. measure file-level Brotli/BWT auto;
2. measure block-level oracle;
3. improve BWT postcoding;
4. add a cheap router;
5. only then decide whether CM’s implementation cost is justified.

This is conservative because all required format primitives already exist.

## I2. QLFC/local-frequency postcoding instead of endlessly tuning MTF

Current BWT uses MTF/RLE variants. Prior art gives a precise next hypothesis: MTF rank discards the identity of the symbol whose local frequency is being represented; QLFC-style coding retains symbol-associated local-frequency state.

This is a better experiment than adding more generic order contexts to the MTF token stream because it changes the representation of the BWT output for a known reason.

## I3. LZP before BWT

Direct BWT’s worst files include mixed/archive-heavy inputs, but even favorable files may contain long exact repetition that BWT postcoding pays to rediscover statistically.

An LZP prepass can remove predictable long phrases before sorting the residue.

The useful experiment is not “clone libbsc.” It is:

```text
same block
same BWT
same postcoder
LZP OFF vs LZP ON
```

Then inspect where the gain comes from.

## I4. Backend-conditioned transforms

Mode 17 currently thinks of transform candidates globally. But a transform can be bad for Brotli and good for BWT, or vice versa.

The actual selection unit should be the pair:

```text
(transform, backend)
```

not independently “best transform” then “best backend.”

This is already what an encode-all oracle can evaluate. The research question is how cheaply production selection can approximate it.

## I5. Smaller BWT blocks inside large rev-2 blocks

Whole-file BWT maximizes context but also maximizes memory and mixed-regime contamination.

Separate:

- outer container block: large, good for transforms and framing;
- BWT sub-block: selected for memory/local stationarity.

This can improve both memory and mixed-file ratio without changing the outer format model.

## I6. Decoder-only engineering

The current portfolio is being judged partly through a full encoder executable. A separate decoder target can make the architecture look much more favorable under decompressor-size-accounted benchmarks.

This is low-risk, unglamorous, and potentially important enough to change which backend combinations are rational to ship.

## I7. Container reconstruction where BWT is weak

`mozilla` is the largest BWT loss and contains archive/compressed material. Published DEFLATE reconstruction results concentrate gains on exactly this class.

That makes DEFLATE reconstruction **complementary** to BWT rather than merely another transform candidate.

Inside-box strategy:

```text
recognize embedded DEFLATE
  -> reconstructable plaintext representation
  -> route plaintext to BWT/Brotli/CM
  -> preserve correction/reconstruction side data
```

## I8. Better cost oracles before better parsers

PNRA’s end-to-end failure showed a real candidate can be chosen by a heuristic and still worsen actual entropy-coded bytes.

Before designing more candidate sources, build a **cheap local delta-cost oracle calibrated against actual encoded stream deltas**.

Potential method:

- log candidate features and exact post-block byte impact offline;
- fit integer cost corrections by token shape/distance/residual count;
- require prospective candidates to clear a safety margin.

This improves every exotic reference mechanism without changing their wire forms.

---

# Part II — Outside the box

“Outside the box” means: change the level at which the problem is formulated.

## O1. Stop asking for a universal compressor; build an expert system of compressors

The most important reframing from current Silesia data is that **specialists are complementary**.

Rather than backend 1 competing philosophically with backend 2, treat them as experts in a mixture:

```text
representation experts
    ×
backend experts
    ×
scale/segment experts
```

The encoder performs model selection; the decoder only executes the chosen explicit IDs.

This is not new compression theory by itself, but it may be the highest-value product architecture for ANVIL.

## O2. Optimize the portfolio itself, including decoder code size

Once backends are specialists, selecting which backends to **ship** becomes an optimization problem.

For backend set `S`, score something like:

```text
corpus_bytes(S)
+ alpha * zipped_decoder_bytes(S)
+ beta  * encode_cost(S)
+ gamma * decode_cost(S)
+ delta * peak_memory(S)
```

Then ask whether adding backend 3 earns its binary/memory complexity across the target distribution.

This prevents “more backends must be better” from becoming the next unexamined assumption.

## O3. Use backend disagreement as a scientific instrument

The seven BWT wins and five Brotli wins are not just routing labels. They are a new dataset about **what redundancy each family can see**.

For each block, collect:

- Brotli bytes;
- BWT bytes;
- future CM bytes;
- structural diagnostics.

Then study the *difference vector* rather than only the winner.

Examples:

- BWT ≪ Brotli: likely long homogeneous contexts / sortable symbol neighborhoods;
- Brotli ≪ BWT: likely mixed/already-compressed/LZ-friendly regions;
- CM ≪ both: statistical context not captured by either;
- all ≈ raw: incompressible or encrypted/compressed.

This can guide **new transform discovery from real failure modes**, instead of inventing transforms first and searching for a corpus later.

## O4. Train a router on byte regret, not codec labels

A normal classifier minimizes “picked wrong backend.” That is the wrong loss.

If two codecs differ by 12 bytes, wrong selection barely matters. If they differ by 4 MB, it matters enormously.

Train/fit the router on:

```text
loss = chosen_bytes - oracle_bytes
```

Use a tiny deterministic model after offline training. This is a powerful way to spend complex analysis **once during research** and ship only a cheap rule.

## O5. Multi-stage early exits: a compression cascade

Instead of running all codecs or one classifier, build a decision cascade:

```text
cheap incompressibility probe
    -> already-compressed/container probe
    -> cheap repetition/context probe
    -> likely backend
    -> uncertain? run a small sampled trial
    -> still uncertain? encode two candidates
```

The amount of encoder effort becomes proportional to decision uncertainty and potential byte regret.

This directly generalizes ANVIL’s successful negative-gate philosophy.

## O6. Sampled “micro-compression” as a router feature

Rather than infer backend suitability only from hand-engineered entropy statistics, actually compress a few deterministic slices with tiny/fast versions of each backend.

Example:

- sample 4 × 32 KiB regions;
- low-cost Brotli mode / small BWT / simple context estimator;
- extrapolate winner confidence;
- only full-encode uncertain cases.

This may predict real backend economics better than H0/H1, as the `mr` conditional-entropy counterexample already warns.

The router remains encoder-only and deterministic.

## O7. Representation ladder instead of a flat transform menu

Current mode 17 treats transforms as siblings. Some transformations are naturally hierarchical:

```text
raw container
  -> container reconstruction/extraction
  -> semantic/stride transform
  -> generic statistical rearrangement
  -> backend
```

Example: ZIP/JAR DEFLATE recovery should happen before a text/BWT decision on the inflated content.

A **representation DAG** may be more appropriate than a flat transform ID registry long term. The wire can still serialize a bounded sequence of registered reversible stages.

Do not implement this until at least two genuinely composable stages prove additive value; otherwise it is architecture astronautics.

## O8. Search for transformations by maximizing cross-backend improvement

Instead of designing a transform from domain intuition, search a constrained transform family and score:

```text
gain(T) = min_backend size(backend(T(x))) - min_backend size(backend(x))
```

This automatically rejects transforms that merely make one weak backend look better while the portfolio’s existing best backend was already smaller.

It is the correct objective once ANVIL is a portfolio codec.

For strides, byte permutations, deltas, and lane grouping, this could be explored cheaply on samples.

## O9. “Generative explanation” tokens, but only for exact relations

The TCOPY/ARI work points toward a broader abstraction: a match token is one **generative explanation** of target bytes from history.

Possible exact explanation families:

- exact copy;
- relocation-normalized copy;
- known-format reconstruction;
- dictionary/template expansion;
- exact numeric affine relation where parameter is explicitly coded.

The key audit constraint is strict: noisy statistical derived parameters were closed. A future explanation family must either be exact or pay its parameter/residual costs honestly.

This is a conceptual path for future novelty without repeating the statistical-zero-bit mistake.

## O10. Optimize for the workload distribution, not only benchmark totals

Silesia/enwik8 are falsification tools, not necessarily the eventual product workload.

Long-term ANVIL should distinguish:

- scientific benchmark objective;
- archival/storage objective;
- database/log objective;
- software/package objective.

A portfolio may rationally choose different backend sets for each profile while sharing one format registry.

The danger is benchmark overfitting; the remedy is explicit workload profiles and held-out corpora, not pretending one scalar score answers every use case.

---

# Part III — Ideas that sound exciting but should wait

## Neural compression

Deferred. Model-size accounting, deterministic deployment, and decompressor distribution make this a different project. Do not mix it into the current evidence program.

## GPU-only BWT / CUDA routing

Potentially useful for archival encode throughput, but it changes hardware assumptions and does not solve ratio. First establish CPU backend economics and routing.

## Arbitrary transform-search language

Too unconstrained. It risks enormous encoder search, format complexity, and novelty confusion. Restrict transforms to small, falsifiable families derived from measured backend failures.

## Fully learned end-to-end router

Unnecessary until a shallow deterministic router fails to recover enough oracle savings. The decoder does not benefit from router complexity, so the encoder should earn every extra microsecond.

---

# The strongest speculative hypothesis from this audit

The highest-upside reframing is:

> **ANVIL’s path to a strong general-purpose result may be to become a rigorously routed portfolio of complementary reversible representations and compression backends, with cheap uncertainty-aware gating, rather than to discover one monolithic novel compressor that dominates every file.**

This is a hypothesis, not a result. It has one unusually strong advantage: it is directly falsifiable with the data and code already in the repository.

Measure the hybrid oracle and routing regret. If the headroom disappears after honest framing/block costs, kill the idea cheaply. If it survives, the project has a much clearer architecture than it had before this audit.

