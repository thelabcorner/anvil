# I8 PRIOR-ART AUDIT — the four ANVIL novelty claims against the frontier

**Author:** `research-gate`. **Date:** 2026-09-04.
**Method:** this document exists because the project already caught itself once
(context clustering = Brotli's RFC 7932 context map, demoted to enabling
infrastructure). The audit hunts for that same failure in every remaining claim.
**Sources:** five existing passes in `docs/priorart-tcopy-external.md`;
fetched this session — Wikipedia *Fractal compression* (Barnsley/Jacquin/PIFS
lineage), *Delta encoding* (xz `--delta`, VCDIFF/bsdiff, rsync), *Asymmetric
numeral systems* (Duda; FSE/Zstd/LZFSE/Draco/CRAM/DivANS/BCPack/JPEG XL).
arXiv API was rate-limited (429) and timed out; cs.IT recent listing was
retrieved and contained **no** relevant general-purpose codec work (all MIMO /
channel coding / LDPC). **This is a coverage gap, recorded honestly below.**

---

## 0. The genus that can never be claimed

Every mechanism below descends from one public principle:

> **Encode a value relative to already-reconstructed context.**

LZ77, VCDIFF, DPCM, FPC, Gorilla, delta patching, TCOPY, ARI-REF and
ARI-STRIDE are all instances of it. **This principle is never claimable and must
never appear in a novelty claim.** What ANVIL can possibly own is always a
*species*-level property: where the transform lives, where its parameter comes
from, and how exactness is restored.

---

## 1. ARI-REF / model-based phrases (synth-arith.bin) — **NARROWED, and the narrow core is narrower than recorded**

### Lineage checked
| Art | What it does | Read |
|---|---|---|
| Finite-context / CTW predictors; PAQ-family context mixing | Predict next symbol from context, code the residual | **Different axis** — they model symbol distributions; they do not emit *references*. No phrase/transform parameter exists. Not anticipatory. |
| `delta coding + entropy coding` in columnar stores | Parquet/ORC RLE + bit-packing; Facebook Gorilla XOR for timeseries; xz `--delta[=dist=1..256]` | **This is the real collision.** See below. |
| Linear prediction / DPCM | Predict from a linear combination of prior samples, code residual | Lineage for the *prediction* idea. DPCM has no match/reference primitive and no transform parameter. |

### The Gorilla/Parquet question, answered plainly
**Is "carry a generative model (start, stride, length) + sparse residuals"
genuinely new, or is it Gorilla/Parquet with a different jacket?**

**Answer: on the form ANVIL actually validated — it is Gorilla/Parquet with a
different jacket. Say so plainly.**

- Experiment Z's own ablation says it: **implicit Δ=σ·(d/4) FAILED (+38.35% vs
  transmitted); the gain is ENTIRELY transmitted-Δ.**
- A **transmitted** per-word constant Δ over a word-aligned span, plus sparse
  byte residuals, is precisely the shape of (a) xz's `--delta` filter with a
  distance, (b) Gorilla's delta-of-delta on a fixed column, (c) Parquet's
  RLE/bit-packed deltas. The differences are: ANVIL discovers the reference by
  search instead of fixing it to "the previous value in this column", and ANVIL
  transmits one Δ per phrase rather than per sample. **Those are engineering
  differences within an established species, not a new species.**
- The mode-16 pre-registration itself concedes the narrow claim is
  "integration novelty … deliberately NARROW", and excludes implicit-Δ. **That
  is the correct call and it should be stated even more bluntly: mode-16 as
  frozen carries NO mechanism-level novelty. It is an integration of
  prior-art-shaped machinery, gated on the Pareto arbiter.**

**Narrow defensible core, restated:** only the **zero-bit, reference-derived**
parameter form (Δ derived from decoder-visible reference state) could be novel
— and **it failed its ablation on this very file**. Which means: on
synth-arith, ANVIL currently has **no defensible novelty position at all**. It
has a defensible *engineering* position (a per-reference delta at LZ-class
decode cost, which no shipped codec does in that combination) and that is what
mode-16 is. Record it that way.

### New fact that sharpens this (measured this session)
brotli-q11 on the **delta+zigzag-varint-transformed** bytes of synth-arith.bin
= **16,313 B** vs 87,013 B raw — **5.33×**. So the "frontier" ANVIL is chasing
on this cell is an artifact of brotli declining a filter it ships elsewhere.
Full correction in ledger PART XIII §5b and `docs/gate-verdict-i8-ari-stride.md`
§4. Implication for the audit: the cell's value as a *novelty* vehicle is
further reduced, because the dominant effect on it is a textbook global
transform, not a reference mechanism.

---

## 2. Transformation-invariant temporal anchoring (PNRA) — **claim stands, fractal collision addressed and dismissed**

### The collision risk, addressed head on as instructed
pnra and strategy both flagged **fractal compression / PIFS**: partitioned IFS
matches a range block against a *transformed copy of another region* with a
discovered parameter. On its face that is transformation-invariant indexing.

**Verdict: NO COLLAPSE. Four independent distinguishers.**

1. **Losslessness class.** Fractal compression is **lossy** (PIFS = contractive
   IFS whose fixed point *approximates* the input; Barnsley/Jacquin/DARPA
   lineage is entirely rate-distortion). There is no residual stream because
   there is no exactness requirement. ANVIL is lossless and its sparse residual
   stream is load-bearing. A lossy transform-search cannot anticipate a lossless
   one.
2. **Transform algebra.** PIFS: spatial contractive affine — 2s×2s domain block
   → s×s range block (geometric decimation) + isometry + luminance `s·z + o`.
   PNRA: `I(v,p) = v + p`, an additive translation invariant on a 1-D byte
   stream. No decimation, no contraction, no scaling. Unrelated parameter
   spaces.
3. **Reference structure.** PIFS range/domain blocks are a **fixed partition**
   of a 2-D grid; every range block must be encoded. ANVIL references are
   variable-length, variable-distance, parser-discovered, and may be declined.
4. **Mathematical requirement.** PIFS needs **contractivity** (convergence under
   iteration). ANVIL needs **exact integer invertibility** at every byte.

**Disposition:** fractal/PIFS is **LINEAGE** for "self-similarity with a
discovered transform parameter" and must be cited once in any PNRA
pre-registration. It does not anticipate the mechanism.

### Other PNRA lineage, confirmed non-anticipatory
- **Parameterized dictionary / dictionary-with-edit-operations:** established —
  but this is exactly why the **transmitted**-parameter form is not defensible
  (see §4). PNRA's parameter is *derived*, zero bits.
- **rsync/bsdiff-style delta:** two-file, and bsdiff's correction mechanism is
  a **flat add-array**, not an entropy-coded sparse mask; discovery is
  suffix-sort, not invariant hashing.
- **Grammar-based compression with parameters (RePair with arguments):** the
  rules carry arguments, but there is no transformation-invariant *index* —
  discovery is still byte-identity digram ranking.
- **Baker's parameterized matching / predecessor encoding** (and Lewenstein &
  Porat set-parameterized matching, already in I4-4 lineage): the invariant
  *trick per class* is **established**. This is the honest limit on PNRA: a
  single invariant is not new; what has never been shipped is a practical
  LZ-family compressor unifying multiple equivalence relations under one MDL
  parser. That remains the only defensible PNRA-level claim, and it is a
  *systems/unification* claim, not an invariant claim.

**Net: PNRA's search-formulation claim survives. Its per-invariant claims do
not, and were never claimed.**

---

## 3. SPARSE-REF (sparse-corrected phrase copy) — **NARROWED. The mask alone is prior art; the combination is the claim**

Requirement was to verify the specific formulation is not anticipated. It is
partly anticipated, and the agenda overstates it.

| Element in §1.2 | Anticipated? | By what |
|---|---|---|
| 1. Sparse correction mask as first-class entropy-coded stream | **PARTLY — the "mask-only" filings exist** | US 12,373,439 / US 2024/0211132 A1 (approximate-match MASKING, no additive transform). Pass 4's own record says: "the mask-without-transform formulation exists". |
| 1'. Self-reference + exact COPY + corrections | **YES — fully anticipated** | **VCDIFF / RFC 3284 (2002)**, standardised: source window may be already-decoded target data, overlapping target copies explicitly supported, ADD/RUN encode unmatched material. Pass 4 already ruled C1's separator cannot be self-reference per se. |
| 2. Entropy coding of correction TOPOLOGY | **NOT FOUND** | No general-purpose codec models the distribution of correction positions. Slot-default analysis (128 masks, 598 (mask,slot) contexts, 86.5% modal accuracy) is ANVIL's own evidence. **This is the strongest surviving element.** |
| 3. Structural-distance propagation across non-identical records | **NOT FOUND as a mechanism; but the evidence for it is weak** | Experiment N: reinforcement-of-taken channels **cannot discover** the record period (69% of greedy sparse matches are far-distance). Experiment T: SRR probe finds more span-like structure but it does not propagate. **Claim 3 is currently UNPROVEN, not merely unclaimed.** |
| 4. Joint MDL parse over both edge types with measured downstream cost | **NOT FOUND; also not yet built** | RCM is greedy single-pass, not a joint MDL parse over both edge types. Agenda itself marks "full measured-cost MDL (R3) still to be proven". |

**Verdict: NARROWED-TO-COMBINATION.** The defensible claim is *not* "sparse
corrected copy" (VCDIFF + the masking filings) and *not* "self-reference"
(VCDIFF). It is the **combination**: sparse-correction mask as a first-class
entropy-coded stream **+** entropy-coded correction *topology* **+** an
implicit/derived transform, **inside a single-file self-referential LZ parser**.
Element 2 (topology coding) is the load-bearing novel part; element 3 is
currently unproven and should be recorded as such rather than as "validated
directionally".

**Correction to `research-agenda.md` §1.2:** that section's claim 1 says bsdiff's
array is "a flat diff, not an entropy-coded sparse mask; no topology modeling" —
true of bsdiff, **but the narrow "mask" concept is in the patent record**
(US 2024/0211132 A1). Claim 1 must be rewritten to concede masking-only art and
rest the novelty on topology coding + the transform. Filed as an action.

### Frontier scan (what exists post-2024)
- **No post-2024 general-purpose codec claiming to beat Brotli was found.**
  arXiv cs.IT recent listing (106 entries, 2026-09-04) contained **zero**
  general-purpose lossless compressor papers — the field's IT venue output is
  MIMO/channel coding/LDPC. This is a **real coverage gap**, not a clean
  negative.
- **ANS/entropy-coder advances:** ANS is now thoroughly commodity — Zstd (RFC
  8478), LZFSE, Draco, CRAM, nvCOMP, DivANS, BCPack, JPEG XL, JPEG AI. **No
  headroom here for ANVIL; this is settled infrastructure.** The one lead the
  project chased (discrepancy-minimizing tANS table construction,
  arXiv 2504.18541) was independently re-verified **twice** as NOT APPLICABLE.
- **Learned / neural compression and LLM-based arithmetic coding:** the
  operator-reported citations (Brevis program synthesis, AIT Challenge
  entrants, KoLMogorov test, Pcodec, OpenZL, Diffuse-to-Compress) are recorded
  in `docs/ORBIT_PROGRAM_COMPRESSION.md` as **lineage, not claims**, with the
  standing caveat that the 2026 figures are operator-reported and unverified.
  **None was re-verified this session** — they remain uncitable as evidence.
  Notable: Brevis reports 30.87% smaller on checkpoints at 6.61 GB/s decode,
  which if true is directly competitive on ANVIL's target plane — **worth one
  verification pass in a future iteration**, not this one.

---

## 4. The ruling that unifies §1 and §2: **where the parameter comes from**

This audit's main structural finding. Across TCOPY, ARI-REF, ARI-STRIDE and
parametric-dictionary art, one axis decides everything:

| Form | Status | Why |
|---|---|---|
| **Parameter TRANSMITTED** | **PRIOR ART** | Parametric dictionary compression; US 7,676,506 (transmitted transform between two versions); and our own project killed it twice (TCOPY transmitted-Δ: made .eh_frame AND .rodata LARGER). |
| **Parameter DERIVED from the reference, zero bits** | **DEFENSIBLE** | This is what five patent passes isolated as NOT-FOUND, and what Intel '148/'665 (read in full, expired) does not teach. |

**And the trap:** ARI-REF's *transmitted*-Δ is the form that **works**
(−65.25%) and is **prior art**; its *implicit*-Δ is the form that is
**defensible** and **failed** (+38.35%). **The project has twice built the
expeditious form and called it progress.** Any future mechanism on this axis
must pre-register which form it is claiming, and the A1–A4 ablation
(exact-LZ | transmitted | implicit | global-filter) is the **only** way to
separate them. That ablation has been an outstanding I2-5 gate condition and
**has never once been executed in this project.**

---

## 5. Coverage gaps (honest)

- **arXiv API rate-limited (429) and timed out** on three targeted queries
  (LZ-with-mismatches, approximate repeat, fractal self-similarity). Only the
  cs.IT *recent listing* was retrieved, and it is not a search.
- **ACM / IEEE / USPTO not queried this session.** Patent coverage relies on
  the five prior passes in `docs/priorart-tcopy-external.md` (Google Patents +
  FreePatentsOnline; Espacenet and lens.org never queried).
- **No post-2024 general-purpose lossless codec survey was completed.** The
  "none found" statement in §3 is a **gap**, not a negative.
- **Operator-reported 2026 citations remain unverified** (Brevis, Pcodec,
  OpenZL, AIT entrants). Do not cite as evidence.
- **FTO attorney review** remains recommended before any commercial claim.

## 6. Actions filed

1. Rewrite `research-agenda.md` §1.2 claim 1 to concede mask-only prior art
   (US 2024/0211132 A1) and rest SPARSE-REF novelty on **topology coding**.
2. Record SPARSE-REF claim 3 (structural-distance propagation) as **UNPROVEN**,
   not "validated directionally" — Experiments N and T both fail to support it.
3. Record mode-16 ARI-REF as **engineering/integration, no mechanism-level
   novelty** (transmitted-Δ is prior-art-shaped; implicit-Δ failed).
4. Correct the "+2.7% from the front" language wherever it appears (ledger
   PART XIII §5b done; `docs/swarm-i8-brief.md` §2 done).
5. **Execute the A1–A4 narrowed-claim ablation** — outstanding since I2-5, never
   run, and it is the sole separator for the entire transform-reference family.
6. One verification pass on Brevis (learned compression, competitive decode)
   next iteration — not this one.

---

*Audited by `research-gate`. This document is a prior-art classification
record, not legal advice. Lineage citations are for pre-registration text; they
are not themselves claims.*
