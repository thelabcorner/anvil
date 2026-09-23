# THEORY — Transformation-Invariant Families: the algebra, the invariants,
# and the economics of indexing over each.

Status: **derivation**. Every invariant below is proved algebraically and
falsified numerically on `tests/corpus/` where the file exists. Nothing here
claims a compression win.

---

## 0. The general principle

Let `x` be a datum at file position `p`, and let `T(x, θ; p, p')` be a family
of transforms parameterised by `θ` that maps a source datum at `p` to a
destination datum at `p'`. PNRA's move is:

> Find `I(x, p)` — computable in O(1) per datum — such that
> `I(T(x, θ; p, p'), p') = I(x, p)` for all admissible `(θ, p, p')`.
>
> Then index the history over `I`. Lookup becomes EXACT HASH EQUALITY instead
> of approximate-nearest-neighbour, and the parameter `θ` never has to be
> guessed, estimated, or transmitted.

Ordinary LZ is the identity case: `T = id`, `I(x,p) = x`.

### 0.1 What makes an invariant useful (the three conditions)

An invariant is worth indexing iff ALL THREE hold:

1. **INVARIANCE** — `I(T(x,θ;p,p'), p') = I(x,p)` exactly (not approximately).
   Exactness is what converts search into hashing.
2. **SELECTIVITY** — the collision rate of `I` over the file is low enough
   that a probe returns a manageable candidate set. Quantified below as the
   *selectivity ratio* `σ = P(I match | not a true transformed match)`.
3. **ECONOMY** — `cost(hash I + verify | hit) << cost(approximate match)`.
   The invariant must be no more expensive to compute than the raw compare it
   replaces, and the event rate must be well below the byte rate.

Condition 3 is where the strongest families die, and it is the one prior work
never checks. I derive it for each family.

### 0.2 The structural theorem (when a cheap invariant CANNOT exist)

This is the honest negative half, and it belongs beside the positive results.

**Theorem (no-go for position-blind invariants).** Let the transform family
be `T(x,θ) = x + θ` (additive, position-independent) with `θ` ranging over a
set `Θ` of size `|Θ| > 1`. Then there is NO function `I` of `(x,p)` such that
`I(x+θ, p') = I(x,p)` for all `θ ∈ Θ` and all `p,p'`, other than a constant.

*Proof.* Fix `p, p'`. Invariance requires `I(x+θ, p') = I(x,p)` for every
`θ ∈ Θ`. Taking `θ₁ ≠ θ₂`, we get `I(x+θ₁, p') = I(x+θ₂, p')` for all `x`, so
`I(·, p')` is constant on `x + Θ`, hence (varying `x`) constant on the whole
domain. With `Θ` an additive subgroup this is immediate; for finite `Θ` of
size ≥ 2 iterate. ∎

**Consequence — this is the reason the transmitted-Δ submode was REJECTED.**
A *position-independent* additive delta admits only the constant invariant,
which is maximally non-selective. The PNRA relocation case escapes ONLY
because the transform parameter is *tied to position*: `θ = −(p−q) = −d`.
**The invariant must consume the SAME degrees of freedom the transform
consumes.** That is the design law, and it generalises:

> **Design law.** A cheap invariant exists iff the transform parameter `θ` is
> a *function of the positions* (or of other decoder-visible state), not a
> free variable. If `θ` is free in the transform family, the only invariant is
> constant, and no hash index can help.

This is exactly why `Δ = −d` (implicit, zero bits) is the surviving submode
and transmitted-Δ is dead: it is not an empirical accident, it is a theorem.

---

## 1. Family A — Additive delta on aligned 32-bit fields

**Transform.** A 32-bit field at byte offset `p` holds `v`. Destination at
offset `p' = p + d` holds `v' = v + Δ`.

**Case A1 — `Δ = −d` (PC/RIP-relative relocation; PNRA's case).**
`v' = v − d = v − (p' − p)`.
Rearranged: `v' + p' = v + p`.
**Invariant: `I(v, p) = v + p`** (mod 2³²).

*Proof.* `I(v', p') = v' + p' = (v − (p'−p)) + p' = v + p = I(v,p)`. ∎

- Selectivity: `I` is 32 bits from a 32-bit field; collisions are those of a
  random 32-bit hash. For a file with `F` fields, expected false-collision
  count per probe ≈ `F / 2³²` — negligible.
  **σ_A1 ≈ F/2³².**
- **But**: a *single* anchor is empirically under-selective in practice — not
  because of hash collisions but because the same callee is called from many
  sites (the Linux note: "common callees → huge candidate populations"). The
  selectivity that matters is not hash selectivity but *structural*
  selectivity. Fix: use two anchors and include their spacing:
  **`I₂ = (K₁, K₂, Δf)`** with `Kᵢ = vᵢ + pᵢ` and `Δf = p₂ − p₁`. Measured:
  2,862/4,583 transformed phrases contain ≥2 E8/E9 fields → the pair index is
  available on ~62% of the useful mass.
- Economy: O(1) per field, one 32-bit add. Event-driven inversion drops the
  work from ~1.4M parser positions to ~115k relocation-pair events on 3.26 MB
  .text (a 12x reduction). **This family is economical. VERDICT: INDEX IT.**

**Case A2 — `Δ` a free constant shared by several fields in one phrase.**
`v'ᵢ = vᵢ + Δ` for `i = 1..k`, same `Δ`.
By the no-go theorem with `Θ` free, there is no position-blind invariant.
BUT there is a *difference* invariant: for `i ≠ j`,
`v'ᵢ − v'ⱼ = vᵢ − vⱼ`.
**Invariant: `I(vᵢ, vⱼ) = vᵢ − vⱼ`** (pairwise field difference).

- This is free of `Δ` and free of position. It is invariant under ANY common
  additive shift, whether or not the shift is position-derived.
- Selectivity: 32 bits per pair, but the *population* is the set of field
  pairs, which can be large. Requires `k ≥ 2` fields per phrase.
- **This is the family `.eh_frame` / `.rodata` live in** — the anatomy says
  "a single repeated 32-bit additive delta explains ~49–61% of mismatch
  bytes", and transmitted-Δ was rejected. The transmitted-Δ rejection killed
  the *parameter transmission*, not the *invariant*: `vᵢ − vⱼ` finds the
  candidate for free. **The prior rejection does not logically close this.**
  Honest caveat: discovery was "prohibitively expensive" there, and this
  invariant does not by itself make it cheap — see economy below.
- Economy: the pairwise invariant has O(k²) pairs per phrase, and the pair
  population is dominated by *spurious* pairs (any two fields whose
  difference coincidentally recurs). Estimated candidate population ≈
  `F²/2³²` for `F` fields; on 3.26 MB of .text with ~10⁶ candidate field
  positions this is ~10¹²/4·10⁹ ≈ 250 false pairs per probe — borderline.
  **VERDICT: index only with a third conditioning term (e.g. field spacing
  `Δf` and the byte context between the two fields). Marginal alone.**

---

## 2. Family B — Stride / period transforms (timeseries and columnar)

**Transform.** A field sequence `vᵢ` at positions `pᵢ = p₀ + i·s` satisfies
`vᵢ = a + b·i + eᵢ`. Two destination variants:

- **B1 (same slope, different phase/offset):** `v'ᵢ = v_{i+m}`.
- **B2 (slope change):** `v'ᵢ = a' + b'·i + eᵢ`.

**B1 invariant.** Under a pure index shift, differences are preserved:
`v'_{i+1} − v'_i = v_{i+m+1} − v_{i+m}` as *sequences*, so the sequence of
first differences is invariant up to a shift of origin. A position-free
invariant that is O(1) per datum:
**`I(vᵢ, v_{i+1}) = v_{i+1} − vᵢ`** (the local first difference).

- This is the **finite-difference invariant** Experiment U already probes.
  It is correct in family B1.
- Selectivity: for synth-arith it is *maximally* selective in the right way —
  the difference takes exactly 3 values (measured, all 8 columns:
  `{63,64,65}, {82,83,84}, …`), so `H = log2(3) = 1.585 b/value` and the
  residual stream is a 3-symbol alphabet. But note the *inversion of purpose*:
  here the invariant is not used to find a match, it is used to *replace* the
  data. The value is in the RATE, not the search. See
  `deliverable/theory-i8-model-phrase-rate`.

**B2 invariant (the one that matters for synth-timeseries / columnar).**
Under `vᵢ = a + b·i + eᵢ`, the *second* difference kills both parameters:
`Δ²vᵢ = v_{i+2} − 2v_{i+1} + vᵢ = e_{i+2} − 2e_{i+1} + eᵢ`,
which is free of `a` AND `b`.
**Invariant: `I₂(vᵢ, v_{i+1}, v_{i+2}) = v_{i+2} − 2v_{i+1} + vᵢ`.**

- Invariant to any affine reparametrisation `(a,b) → (a',b')`. This is the
  *affine* invariant, and it is the direct generalisation of PNRA's `v + p`
  (which is invariance under the specific affine map `θ = −d`).
- Selectivity: if `e` is iid, `Δ²v` has HIGHER entropy than `e` (it is a
  filtered noise, `H(Δ²e) ≈ H(e) + log2(6)` for iid ternary `e`: the filter
  `1 − 2z + z²` amplifies). So the second difference is a *poor* residual
  coder and a *good* invariant — the two roles are in tension.
  **Use it for SEARCH (find the column/stride), not for CODING.**
- Economy: recomputing `Δ²` at every candidate stride `s` is O(N) per stride.
  The stride search is the cost. Cheap prefilter: the autocorrelation of the
  byte stream at lag `s` (the `synth-timeseries` record stride is 14 B, and
  the ledger shows the SRR probe found 98.7% periodicity there).
  **VERDICT: use `Δ²` for stride/offset DETECTION (cheap, O(N) per candidate
  stride, and the stride set is small), then switch to the correct coder for
  the residual. Do NOT code with `Δ²`.**

---

## 3. Family C — Endianness / width field transforms

**Transform.** `C_w`: reinterpret `w` bytes at position `p` as an integer
under byte-order `ω ∈ {LE, BE}`; or re-width `w → w'` by truncation/extension.

**Invariant.** The multiset of bytes is invariant under endianness:
**`I_C(x) = sort(bytes(x))`** (or any order-independent digest: sum, XOR, or
a product over a commutative hash).

- *Proof.* `C_LE` and `C_BE` are permutations of the same byte multiset. Any
  symmetric function of the bytes is invariant. ∎
- Stronger: **`(Σ bytes, Σ i·bᵢ)`** is invariant under any permutation *up to
  the permutation itself*, i.e. it IDENTIFIES the permutation class. For
  width-4 there are only 24 permutations, so a 12-bit permutation index
  separates them — meaning the pair `(symmetric digest, permutation index)`
  is a *complete* invariant of the endianness/width class.
- Selectivity: **POOR.** A symmetric byte digest destroys all positional
  information, so collisions are frequent: for 4-byte fields drawn from
  text-ish data, the multiset collision rate is ~(number of 4-multisets
  from a 256-alphabet with skew). Measured on `anvil.exe`: the byte-value
  distribution is heavily skewed, so multiset collisions are far above the
  1/2³² of Family A. Estimated σ_C ≈ 10⁻²–10⁻³, i.e. 3–6 orders of magnitude
  worse than A1.
- Economy: endianness variation within a single file is essentially
  nonexistent (a file is LE or BE; mixed-endian fields are a container
  pathology, not a compression-relevant population). The population of
  *useful transforms* is near zero, so an index over `I_C` pays for itself
  almost nowhere.
- **VERDICT: DO NOT INDEX.** Correct invariant, useless population. This is
  an honest negative: the mathematics is valid and the engineering case is
  absent. Recorded so nobody re-derives it hopefully.

---

## 4. Family D — XOR masks

**Transform.** `v' = v ⊕ M` for a mask `M` (constant across a phrase, or
derived from position).

**Case D1 — free mask `M`.** By the no-go theorem applied to the group
`(Z₂^w, ⊕)`: invariance under all `M` forces `I` constant.
*Proof.* For any `x,y` choose `M = x ⊕ y`; then `I(y) = I(x ⊕ M) = I(x)`. ∎

**Case D2 — mask derived from a keyed/position stream, `M = f(p)`.**
`v' = v ⊕ f(p')`, source `v = v₀ ⊕ f(p)`. Then
`v ⊕ f(p) = v' ⊕ f(p')`, so **invariant `I_D(v,p) = v ⊕ f(p)`**.

- This is the exact XOR analogue of PNRA: `v + p` ↔ `v ⊕ f(p)`. Everything
  PNRA does for the additive group transports to the XOR group verbatim,
  because both are abelian groups and the invariant is "subtract the
  position-derived group element".
- Selectivity: identical to A1 (32-bit digest of a 32-bit field).
- Population: XOR-masked fields are common in **obfuscated/packed
  executables and in some container formats**, and in **float data** the
  Gorilla `vXOR` case (`vᵢ ⊕ v_{i−1}`) is exactly this with `f(p) =` the
  previous value. But note: in Gorilla the "mask" is the previous sample,
  which is decoder-visible *data*, not position — so the right invariant is
  `vᵢ ⊕ v_{i−1}` computed on the *reconstructed* stream, and it is already
  what the coder transmits. There is no separate search problem.
  **Prior-art caution: Gorilla (Pelkonen et al., VLDB 2015) owns XOR-of-
  consecutive for timeseries. Anything in this family on float columns is
  Gorilla, not ANVIL novelty.**
- **VERDICT: index D2 only for executable/container XOR-obfuscation, which is
  a population this corpus does not contain. On timeseries it is prior art.**

---

## 5. Summary table — the economics, which is the part that decides

| Family | Transform | Invariant | Free θ? | Selectivity σ | Useful population | Index it? |
|---|---|---|---|---|---|---|
| **A1** | `v' = v − d` (relocation) | `v + p` | **no** (θ=−d) | ~F/2³² | .text, PEs — LARGE | **YES** (proven, PNRA) |
| A2 | `v' = v + Δ`, Δ common | `vᵢ − vⱼ` | yes (but cancels in pairs) | ~F²/2³² | .eh_frame/.rodata — real but discovery-costly | Only with a 3rd term |
| **B1** | index shift | `v_{i+1} − vᵢ` | n/a | alphabet-dependent | columnar — LARGE | **YES** (as a CODER, not a search index) |
| **B2** | affine reparam | `v_{i+2} −2v_{i+1} + vᵢ` | **no** (kills a,b) | poor (noise-amplifying) | stride DETECTION | For detection only |
| C | endianness/width | `sort(bytes)`, or `(Σbᵢ, Σ i·bᵢ)` | n/a | ~10⁻²–10⁻³ | ≈ 0 | **NO** — valid math, no population |
| D1 | `v ⊕ M`, M free | **none but constant** | yes | — | — | **NO** (theorem) |
| D2 | `v ⊕ f(p)` | `v ⊕ f(p)` | **no** | ~F/2³² | packed binaries; timeseries=Gorilla | Only off-Gorilla |

## 6. What is genuinely new, stated narrowly

The mechanism-level claim that survives contact with prior art is:

> **Invariance requires the transform parameter to be position-derived (or
> otherwise decoder-visible); a free parameter admits only the constant
> invariant.** Consequently the *search* problem is only tractable for the
> implicit-parameter sub-family, and given a position-derived `θ` the
> invariant is obtained by subtracting the position-derived group element —
> uniformly across abelian groups (`v + p` for additive, `v ⊕ f(p)` for XOR).

That is a *characterisation theorem* for when transformation-invariant
indexing can exist, plus the construction. PNRA is its additive-group
instance. This is stronger than "PNRA works on .text" and it explains the
transmitted-Δ rejection as a theorem rather than a measurement.

**Falsification.** The no-go theorem is falsified by exhibiting a
position-blind, non-constant `I` invariant under a free additive (or free
XOR) parameter. I do not believe one exists; the proof is two lines. The
*positive* claims (A1, B1, B2) are falsified by measuring the candidate
population and verification cost on real files — a family whose σ or whose
event rate exceeds the byte rate is not economical regardless of its
invariance.
