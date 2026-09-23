# SPARSE-REF Mask Stream — Research Advisory (draft, NOT the wire spec)

Status: advisory from `research` for `arch` (t-sparse) and `format` (t-format).
`format` owns the final wire spec in FORMAT.md. This documents the candidate
representations and cost math so the prototype starts from a measured
decision, not a guess.

## Scope

A sparse-corrected phrase edge = `(distance, copy_len, mask, residuals)`.
`mask` marks the byte offsets (relative to the copy start) where the decoded
byte differs from the copied phrase byte. `residuals` is the concatenation of
the replacing bytes, in mask order. Decoder: memcpy phrase → for each set
mask bit, store residual byte. This draft is only about **mask** + **residual**
encoding (edge framing / token interleaving is the block-mode spec's job).

## Candidate mask representations (measure, don't assume)

| Rep | Encoding | Cost on 32B window | Decoder cost |
|---|---|---|---|
| **A. Flat bitmask** | 1 bit/byte (4 B per 32 B window) | 4 B/window fixed + residual | mask lookup + sparse store; SIMD-friendly |
| **B. Run-length (uncorrected spans)** | count of clean bytes, then mask byte | depends on runs; ≥0 bits when fully clean | slightly more state |
| **C. Topology model (order-1 over masks)** | entropy-code mask words given previous record's mask | entropy of mask ≤ flat always (by construction) | model lookup per word; rANS on mask words |
| **D. Per-record mask delta** | XOR of mask with previous record's mask | tiny when topology recurs (JSON field-offset recurrence) | XOR + store |

Recommendation from the agenda (R1 first milestone): **start with A**, ship
the edge type end-to-end, measure. Then add C/D as R2 (the novelty claim that
mask topology recurs is *hypothesis until the ablation shows it* — flat A is
the honest baseline for that ablation).

## Residual stream considerations

- Residual bytes are the "value field" content in record-structured data —
  they are exactly where a small context model pays (R7). But for milestone 1,
  residual = plain rANS byte stream (existing static rANS backend, order-0).
  Do NOT add a context model until R7 ablation is scoped.
- Mask-to-residual consistency: `len(residuals) == popcount(mask)` MUST be an
  invariant the decoder enforces strictly (format's lane; strictness is
  non-negotiable per CONTEXT.md).

## Cost math for the parser (feeds t-parser MDL)

For a candidate edge with `n` correction bytes in a copy of length `L`:

```
cost(edge) ≈ log2(dist) + log2(L) + cost(mask) + cost(residuals)
cost(mask) ≈ L/8 bits  (flat, rep A)
            or H(mask model) bits  (rep C, after R2)
cost(residuals) ≈ n · H(residual_model)
```

Rule of thumb for greedy-first prototyping: prefer a sparse edge over exact-LZ
only if `cost(mask) + n·H(residual) < n·H(literal)` (what the n bytes would
cost as literals) AND the distance itself is cheaper than the alternative
exact edge. If the mask is denser than ~1 in 4 bytes, exact-LZ + literals
usually wins — the parser can prune candidate edges by this ratio before
running the full model.

## Open questions for format

1. New block mode number (do not collide with 0–10); versioning strategy.
2. Edge framing: sparse-corrected edges as a new token type in the existing
   separated-stream layout, or a distinct mode? (My lean: new token type in
   the rANS backend keeps one decoder path; format decides.)
3. Distance validity for phrase edges: must point into already-decoded output
   (self-referential), same rule as exact matches; record-period sanity check
   (dist in a plausible range) is optional, reject-invalid mandatory.

## Falsifiable expectation (for the ablation record)

On record-structured corpora, flat-A masks should already beat "exact-LZ +
literals" on files with long near-identical records; the R2 topology models
are expected to add a few % on top by compressing the mask itself. If A does
not beat the baseline on generated.json/sqlite/log, the whole mechanism is
suspect — that is the ablation that decides, per the agenda §1.4.
