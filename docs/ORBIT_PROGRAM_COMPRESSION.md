# Project ANVIL — Orbit-Program Compression (strategic direction, v1)

Source: operator research brief (Aug 2026). Status: DIRECTION — not yet a
measured claim. The Windows swarm's measured PNRA result (transformation-
invariant anchoring, prototypes/pnra/) is the first experimental instance of
the core idea; this document generalizes it.

## The central conceptual shift

Brotli asks: "Where have I seen these bytes?" The target architecture asks:
**"Where have I seen something from which these bytes are cheaply
computable?"** — moving compression from repetition discovery toward
conditional program discovery.

Classical LZ asks ∃x in history: y = x? ANVIL should approximate
K(y | x, decoder state) — the shortest cheap program that turns an earlier
phrase x into the current phrase y.

## External signals (2026, as reported by operator)

- **Brevis (Aug 3, 2026)**: lossless compression as program synthesis over a
  typed DSL (repeat/map/scan/merge); learned prior guides bounded A* search;
  decoder executes only the compact program. Reports archives up to 30.87%
  smaller on 2.13 TB of checkpoints at 6.61 GB/s decode. Principle: the
  compressor can be extremely intelligent while the decompressor stays
  stupid and fast — learned machinery is NOT shipped to the decoder.
- **LZ77 k-sensitivity / pre-editing (Feb 2026, arXiv 2602.19649)**: modify a
  small number of symbols so the string becomes more LZ-compressible, store
  the edits to reverse it; favorable cases improve total representation by
  up to ~3x. Philosophically TCOPY: bad exact match → normalize differences →
  excellent match + cheap residual.
- **2026 AIT Compression Challenge (arXiv 2606.17712)**: one entrant detects
  glibc rand()-generated data, brute-forces the seed, replaces the file with
  a 14-byte generator descriptor. High Shannon entropy, tiny algorithmic
  complexity — algorithmic redundancy is a distinct, attackable source.
  Another entrant discovers byte-distance contexts via mutual information.
- **KoLMogorov test (arXiv 2503.13992)**: theoretically optimal compression
  = shortest program producing the data; unconstrained LLM codegen is
  currently poor at it. ANVIL's tractable middle ground: a carefully
  designed algebra of extremely cheap programs.
- **Pattern matching under polynomial transformation (Butman et al.,
  arXiv 1109.1494)**: O(n log m) matching under linear transformations,
  extended to polynomials; bounded-Hamming under additive transforms.
- **Set parameterized matching (Lewenstein & Porat, 2026, arXiv 2605.00566)**:
  generalized set version in randomized linear time via structural
  representations + multilayer hashing. Baker's predecessor-encoding
  canonical invariant is the classic trick: two strings parameterized-match
  iff their invariant representations are identical.
- **Set reconciliation / sparse recovery (arXiv 1410.2645)**: characteristic-
  polynomial methods and BCH-style sketches make communication depend on the
  number of differences, not object size.
- **Pcodec (arXiv 2502.06112)**: decomposes numbers into latent variables +
  delta + distribution coding; 29–94% better ratios on columnar data. Starts
  from known typed structure.
- **OpenZL**: composable reversible graph primitives with a universal
  decompressor; numeric identification + delta/FieldLZ.
- **Diffuse to Compress (Aug 2026, arXiv 2608.11249)**: diffusion LM
  lossless compression, orders of magnitude faster than autoregressive
  neural compressors but still kb/s-scale — NOT the decoder ANVIL wants.

## The generalized reference

Compressed phrase concept:

```
REF(d, L, P, θ, R)   with   y = P(x; θ, known context) ⊕ R
```

Objective is not longest match:

```
min_{x,P,θ,R} [ B(d,L) + B(P) + B(θ) + B(R) ]   subject to exact reconstruction
```

## Ranked directions (operator ranking)

1. **Orbit-LZ / equivalence-class dictionary matching.** For a transformation
   family G acting on phrases, define repetition as y = g_θ(x) — all
   transformed versions of x form an orbit/equivalence class [x]·G. LZ then
   searches history for a phrase in the same ORBIT, not the same byte string.
   Viability: canonical/hashable invariants make transformed matching ≈ exact
   matching cost (parameterized matching precedent; PNRA is the first measured
   instance). Research question: for which useful transformation groups can
   we construct a canonical or hashable invariant?
2. **Conditional-program references** — the general architecture subsuming
   TCOPY; REF(d,L,P,θ,R) with programs from a tiny fixed deterministic VM.
3. **Implicit/zero-bit transform parameters** — θ = F(d, p, x, previous
   output) using decoder-known state. TCOPY Δ=−d is the first instance
   (B(Δ)=0). Objective: maximize structure-represented / new-information-
   required. Candidates: relocation translation, sequence continuation,
   row/record index offsets, pointer rebasing, counters, polynomial
   continuation, advancing IDs, anchor-literal-recoverable coefficients.
4. **Structured pre-editing + residuals** (LZ77 k-sensitivity theory).
5. **Automatic latent-schema discovery** — infer the data's hidden type from
   raw bytes (8/16/32/64-bit lanes, bitplanes, structs, strided arrays,
   records, pointers, counters, floats); compression ratio itself is the
   hypothesis test. No MIME type, no parser, no extension.
6. **Bounded generator/program synthesis** — bank of tiny generative
   programs (LCGs, xorshift, counters, Gray codes, LFSRs, recurrences,
   CAs); exact → (program ID, state, L); approximate → +R.
7. **Encoder-only AI search** — AI proposes schemas/transforms/masks/
   recurrences/generators; ANVIL deterministically verifies; archive holds
   only the winning tiny program. Decoder stays LZ4-class.

## The mathematical problem to concentrate on

> Develop a near-linear-time algorithm for finding the minimum-description
> transformed backreference over a useful algebra of transformations.

Formally: min_{q,L,g,θ,e} B(q,L)+B(g)+B(θ)+B(e) s.t.
y = g_θ(X[q:q+L]) + e. The breakthrough is invariants/sketches that prune
this space without evaluating every (reference × transformation) pair.
Precedent exists per class (linear/polynomial matching, predecessor
canonicalization, multilayer structural hashing, PNRA). What does NOT
appear to exist: a practical LZ-family compressor unifying these equivalence
relations under an MDL parser.

## Convergence with measured ANVIL results

- PNRA (transformation-invariant anchoring) is Orbit-LZ for the translation
  family with invariant I(v,p) = v + p — exact hash lookup, ~474 MB/s
  event-driven candidate generation, density below brotli q4 on .text.
- TCOPY Δ=−d is zero-bit-parameter compression for the executable-relative
  family.
- The 22-stream/precision-adaptive entropy + shape-book + per-shape
  displacement stack is the statistical base these primitives sit on.
- PCODEC-style latent schema discovery would extend the block router: the
  router already picks per-block representations; latent-schema hypotheses
  become more router candidates, tested by actual encoded bits.

## Disciplines retained

- Decoder stays cheap: tiny fixed VM + copy + sparse stores + integer adds.
  No LLM/neural code in the decoder; AI is encoder-side search only.
- Every mechanism passes the gate (lineage, what-is-new, why-Pareto,
  isolated ablation). Brevis/Pcodec/OpenZL/sketch papers are lineage, not
  novelty claims.
- The 2026 citations above are operator-reported; verify each before citing
  in the ledger.
- Round-trip + fuzz before any claim; Pareto on both planes.
