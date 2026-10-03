# 01-g5d-dictionary — SPACE BUNNY FREE (constructive inventor)

**Track:** G5D paged base+overlay exact dictionary — cost model, dictionary drift, escape economics, decode locality
**Date:** 2026-10-02
**Worktree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty
**Companion review (read in full, adjudicated in §11):** `docs/swarm-2026-10-02/01-g5d-dictionary-fledge.md`

**Verdict (headline):**

> ## HOLD (family) · KILL (the current G5D discovery design as preregistered) · PILOT (one census-first, attribution-fixed, remote-only ablation)

This **concedes** the critic's three load-bearing objections — overlay inertness
below one page, D3-concentration of the gate, and the four-way confound in the
discovery arms — and **downgrades** my own earlier leaning accordingly. It
**refutes** two of the critic's supporting arguments with named source and
arithmetic, and **adds** a correctness defect in the frozen implementation that
is stronger than the critic's proxy-fidelity objection.

**Zero G5D corpus bytes exist.** No ratio number in this report is measured.

---

## 0. Structure of this report

1. Evidence map — measured facts only, each with a source path.
2. Precise mechanism as implemented (not as intended).
3. Structural fact that dissolves the mechanism: root and overlay are one code space.
4. Full byte / cycle / RSS cost model, including the exact admission rule.
5. Reconciliation with the critic's seven obligations (§11) — the mandated core.
6. Family engineering value vs discovery-design validity (§10, mandated separation).
7. Novelty and prior-art boundary.
8. Asymptotic and performance analysis.
9. Redesigned decisive remote-only experiment with pre-registered thresholds.
10. Adversarial failure cases.
11. Final recommendation.

---

## 1. Evidence map

### 1.1 MEASURED — frozen result documents

| Fact | Value | Source |
|---|---|---|
| G2 `EXACT_DICT` win, D2 CDISC | −16.4849% vs raw Brotli; P-DICT 20,903 B vs G1R 25,916 B (−19.3433%) | `docs/I10-GROTLI-G2-RESULTS.md` §3, §5 |
| G2 `EXACT_DICT` win, D4 CROVIA | −22.5029%; P-DICT 724 dict leaves, P-MIXED 631 | `docs/I10-GROTLI-G2-RESULTS.md` §6 |
| G2 `EXACT_DICT` win, D1 Amazon | −2.0610% (39,299 B); only 2 dict substitutions | `docs/I10-GROTLI-G2-RESULTS.md` §7 |
| G2 ruling | `NO-GO-G2-DISCOVERY`, routed aggregate −2.0077% (1,437,320 B) | `docs/I10-GROTLI-G2-RESULTS.md` §0, §3 |
| G2 planner cost | marginal search 6.80 s / 75.10 s / 121.28 s on D1/D2/D4; anatomy 0.74/3.97/8.03 ms | `docs/I10-GROTLI-G2-RESULTS.md` §9 |
| G3 discovery | `PASS-G3-REGION`, routed 1,384,654 B vs raw 1,466,769 B (−5.5984%) | `docs/I10-GROTLI-G3-RESULTS.md` §0, §1 |
| G3 D3 causal result | 11,228 structured frames + 1 raw residual; 1,292,757 → 1,240,155 B (−4.0690%) | `docs/I10-GROTLI-G3-RESULTS.md` §2 |
| G3 held-out V1 | `PASS-G3-NARROW`; 2,179,615 B vs raw 2,112,235 B = **+3.1900% worse**; 11,444 frames, 100% structured, 3 exact shapes, 19/48 DICT leaves | `docs/I10-GROTLI-G3-RESULTS.md` §3 |
| G3 `G3_REGION_DICT` per file | D1 39,277 · D2 20,861 · D3 1,240,155 · D4 84,431 (Σ 1,384,724) | `docs/I10-GROTLI-G3-RESULTS.md` §1 |
| G5A ordering | A0/A1/A2/A3 = 2,054,532 / 1,470,205 / 1,452,383 / 1,380,245 B; A3 −89,960 B vs A1, smaller 4/4; `ORDER-MATERIAL`, `COLUMN-DOMINANT`; V1 `V1-COLUMN-ADVERSE` | `RESEARCH_LEDGER.md` L4749–4767 |
| G5B-ORDINAL | `ORDINAL-ADVERSE`; B2 = 1,403,029 B, +1.6507% vs B1, wins 1/4 | `RESEARCH_LEDGER.md` L4769–4792 |
| G4 planner fidelity | `NO-GO-G4`; S0/S1/S2 aggregate regret −2.0353% / −1.1453% / −0.2120%, worst-file 7.0802% / 7.0802% / 1.5388%; speed column `INVALID_SPEED_ACCOUNTING` | `docs/I10-GROTLI-G4-RESULTS.md` §2; `RESEARCH_LEDGER.md` L4723–4735 |
| G5D prototype state | `clang-cl /std:c++20 /O2 /DNDEBUG /W4 /WX` warning-clean; both selftests PASS; smoke emits real `decode_peak_rss_kib: 5416`; invariants now computed, not hard-coded | `RESEARCH_LEDGER.md` L4875–4884, L4912–4924 |
| G5D dispatch blocker | uncommitted source + prereg + workflow; user-authorization dependency, not a measurement defect | `RESEARCH_LEDGER.md` L4953–4964 |
| Portfolio decision | "**PROMOTE to remote pilot:** one paged base-plus-overlay exact dictionary with mandatory escape, isolated from FSST/defaults/hierarchy" | `RESEARCH_LEDGER.md` L4814–4817 |

### 1.2 MEASURED — from the frozen G5D source

| Fact | Location in `tools/grotli_g5_paged_dictionary.cpp` |
|---|---|
| `kPagePolicies = {4096, 16384, 65536}` | L154 |
| `kCapCandidates = {0,16,64,256,1024,4096,16384,65536}` | L155 |
| `bit_width_u64(x)` = bits to represent `x`, **0 for x = 0** | inherited from `tools/grotli_g3.cpp` L468–472 |
| `code_width = bit_width_u64(root.size() + page.overlay.size())` | L481 |
| root table = first `root_cap` entries of the **descending-frequency** ranking | L360–377, L438–445 |
| overlay table = page-local ranking minus root members, first `overlay_cap` | L452–477 |
| leaf payload = `dict_id uvar · page_policy_id u8 · root_count uvar · root entries · per page (overlay_count uvar · overlay entries · code_width u8)` | L535–549 |
| **packed IDs and escapes are serialized in the coordinate order, not the leaf-payload order**; escapes go to a **separate trailing region** | L862–895 |
| `component_sum_bytes == body.size()` enforced for both arms | L803, L900 |
| MDL selector objective = *uncompressed* carrier bytes (`local_cost`, `page_cost`, `total_cost`) | L451, L504–508, L553 |
| leaf = one (shape, slot) column; one dictionary namespace per leaf; **no cross-leaf shared root** | L313–321; PREREG §7.1 |

### 1.3 MEASURED — backend provenance (new, verified this session)

| Fact | Source |
|---|---|
| Backend call is `BrotliEncoderCompress(11, 30, BROTLI_MODE_GENERIC, ...)` | `tools/grotli_g3.cpp` L179; `tools/grotli_g5_paged_dictionary.cpp` (inherits G3) |
| `lgwin=30 > BROTLI_MAX_WINDOW_BITS(24)` sets `BROTLI_PARAM_LARGE_WINDOW = TRUE` | `third_party/brotli/c/enc/encode.c` L1332–1334 |
| large-window mode clamps to `BROTLI_LARGE_MAX_WINDOW_BITS = 30`, i.e. the window is **2^30 bits**, a non-standard Brotli extension | `third_party/brotli/c/enc/quality.h` L59–70; `third_party/brotli/c/include/brotli/encode.h` L30, L35 |
| G3's packed-ID primitive is bit-identical in kind to G5D's (`pack_fixed`) | `tools/grotli_g3.cpp` L483–511 vs G5D L608–689 |

Consequence (§4.4): all frozen corpora are ≤ 15,374,047 B < 2^24, so the
large-window setting is **byte-neutral here** — it does not confound the ratio
comparisons. It does, however, dominate decode RSS and therefore weakens the
prereg's 110% RSS gate.

### 1.4 NOT MEASURED — the decisive evidence gap

| Unknown | Status |
|---|---|
| Per-leaf occurrence count `L`, distinct count `D`, and page-local distinct count `D_p` for D1–D4 | **Absent from every results document.** This is the shared biggest gap of both lanes. |
| Any G5D corpus byte count | **Does not exist.** |
| Whether dictionary drift (locally-stable vocabulary inside a globally-large one) is present at all | **Unmeasured.** |
| Whether frequency ordering alone beats first-occurrence ordering for `EXACT_DICT` | **Unmeasured.** Never ablated. |
| The §4.2 exact-admission algebra and the one-sidedness of the frozen-ladder regret | **MEASURED this session**, synthetic correctness fixture, 2 seeds, 155,287 pages, exit 0 — `prototypes/swarm-2026-10-02/01-g5d-dictionary/space-bunny/exact_admission_verify.cpp`; see §4.9. **No corpus was touched.** |
| The *corpus* magnitude of the ladder regret | **UNKNOWN.** Governed by singleton-tail mass (§4.3), which is unmeasured for D1–D4. The synthetic 6.12% is an upper bound, not an estimate. |

---

## 2. Precise mechanism as implemented

For each exact shape-slot leaf `ℓ` with `n_ℓ` occurrences:

```
root  R  = top root_cap entries of ℓ's descending-frequency distinct ranking
page  p  = occurrences [p·P, min(n_ℓ,(p+1)·P)),   P ∈ {4096, 16384, 65536}
overlay O_p = top overlay_cap of p's page-local descending-frequency ranking,
              restricted to tokens not in R
active a_p = |R| + |O_p|
w_p       = bit_width(a_p)            (0 iff a_p = 0)
ids       = per-occurrence code in [0, a_p);  code == a_p means ESCAPE
```

Wire order in the body: **`prefix` → leaf payload table (all leaves) → one packed-ID
region in coordinate order → one escape region in coordinate order**
(L776–895). The escape region is separate so escapes never disturb ID bit
alignment — a real implementation strength the critic credits (§6).

Complete bytes: `C = outer_envelope + Brotli_q11_lw30(body)`.

---

## 3. Structural fact that dissolves the "paged overlay" framing

**Root and overlay occupy one contiguous code space.** At L480–493 an entry
resolved in the root gets `id = index in R`; an entry resolved in the overlay
gets `id = |R| + index in O_p`; an unresolved entry gets `id = a_p`. The decoder
(L1010 onward) rebuilds one `(offset,length)` table over `[0, a_p)`. **Nothing on
the wire records which table an ID came from.**

Therefore:

> **For a leaf with `n_ℓ ≤ P` (one page), G5D is wire-identical to a flat,
> frequency-ordered, capped dictionary with a mandatory escape — plus one
> `page_policy_id` byte and one page descriptor.** The root/overlay split is not
> merely suboptimal there; it is *not a distinguishable mechanism*.

This **concedes the critic's D2 conclusion and strengthens it**: the overlay
cannot be the active ingredient on any single-page leaf, not because it costs
more than escaping (the critic's stated reason, which I refute in §4.5) but
because it does not exist as a separate wire entity.

It also sharpens the critic's §9 degeneracy point. All three page policies emit
the **same carrier** for every single-page leaf except the one-byte
`page_policy_id`. So:

> Unless at least one leaf exceeds 4,096 occurrences, the prereg's six
> dictionary arms (§11.3) contain **at most one distinct configuration**, and
> `G5D_P4K`, `G5D_P16K`, `G5D_P64K` differ only in a label byte.

---

## 4. Complete cost model

### 4.1 Pre-backend leaf cost (exact, as charged)

With `ν(x)` = canonical-uvar length, matching L451–553:

```
L_leaf(ℓ) = ν(leaf_id) + 1                       # page_policy_id
          + ν(r) + Σ_{e∈R} [ν(|e|) + |e|]        # root table
          + Σ_{p<K} [ ν(o_p)                     # overlay_count uvar
                     + Σ_{e∈O_p}[ν(|e|)+|e|]     # overlay table
                     + 1                          # code_width u8
                     + ⌈n_p·w_p/8⌉                # packed IDs
                     + Σ_{esc∈p}[ν(|e|)+|e|] ]    # escape bytes
          + ν(payload_len)
```

`C_G5D = outer_envelope + Brotli_q11_lw30(body)`, and
`component_sum_bytes == body.size()` is asserted (L803, L900). The accounting is
honest and complete at the wire level. **The critic is right that it omits
decoder memory (§4.6).**

### 4.2 Exact per-page cost, and the exact admission rule

For one page, rank non-root tokens by descending page-local count
`(c_1,|e_1|), …, (c_m,|e_m|)`. The exact charged cost of admitting the first `c`
of them is

```
cost(c) = ν(c) + Σ_{i≤c}[ν(|e_i|)+|e_i|]  +  ⌈n_p·bit_width(c)/8⌉  +  Σ_{i>c} c_i[ν(|e_i|)+|e_i|]
```

**Exact algebraic reduction (VERIFIED, see §4.9).** The table bytes and the
escape bytes cancel exactly, because the table cost of entry `i` equals the
escape bytes its *first* occurrence would have cost:

```
Σ_{i≤c}[ν(|e_i|)+|e_i|]  −  Σ_{i≤c} c_i[ν(|e_i|)+|e_i|]  =  −Σ_{i≤c}[ν(|e_i|)+|e_i|](c_i − 1)

cost(c) = Escapes_total  −  Σ_{i≤c}[ν(|e_i|)+|e_i|](c_i − 1)  +  ν(c)  +  ⌈n_p·bit_width(c)/8⌉
```

Hence the **exact marginal value of admitting entry `i`** is

```
value(i) = [ν(|e_i|) + |e_i|] · (c_i − 1)     −     n_p·Δw_i/8     −     Δν(c)
Δw_i = bit_width(i) − bit_width(i−1) ∈ {0,1},  =1 only at powers of two
```

Three consequences, all verified:

1. **Table admission is exactly a "count ≥ 2" trade.** A token occurring once
   has `value(i) = 0` minus the width/uvar penalties — i.e. strictly **negative**.
   Every token occurring `k ≥ 2` times has positive value `(k−1)(1+|e_i|)` before
   the width step. This is the escape-economics law, and it is now exact rather
   than asymptotic.
2. **The marginal is not monotone**, because `[ν(|e_i|)+|e_i|]` varies with token
   length while `c_i` is non-increasing. `value(i)` therefore has positive spikes
   at powers of two (where `n_p·Δw/8` is charged) that fall back afterwards.
   **Consequence: cost(c) is not unimodal, so no early-exit scan and no band-top
   rule is correct.** I tested both and **brute force refuted them** (§4.9).
3. **The exact minimiser is nonetheless computable in `O(D)`** by evaluating
   `cost(c)` for all `c ∈ [0, D]` from two running sums — no cap enumeration, no
   per-occurrence re-resolution, no backend call:

```
T = Σ_all c_i(1+ℓ_i);  H = 0
for c = 0..D:  cost(c) = T − H + ν(c) + ⌈n_p·bit_width(c)/8⌉ ;  then H += (1+ℓ_c)(c_c − 1)
c* = argmin cost(c)   (ties → smallest c, matching the frozen tie rule)
```

This is the *exact* minimiser of the objective PREREG §7.5 names, over all
integer caps, at `O(D)` — versus the frozen implementation's
`8 × 8 × pages` re-ranking with a binary search **per occurrence**
(L452–528), i.e. `O(64·n·log D)` per leaf. The exact rule is both strictly
cheaper and strictly better.

### 4.3 The frozen ladder is not the argmin of its own stated objective — VERIFIED

PREREG §7.5 states the selector chooses "the overlay cap with the smallest
**exact serialized G5D leaf cost**." The implementation restricts the search to
`kCapCandidates` (L155) and returns the argmin over that set (L509–514,
L554–559). `kCapCandidates = {0, 2^4, 2^6, 2^8, 2^10, 2^12, 2^14, 2^16}` — the
**band bottoms**, not the minimisers.

**Why the ladder loses, in closed form.** Two failure modes, both from §4.2:

- **Under-admission inside a band.** Within a width band the `⌈n·bit_width/8⌉`
  term is constant, so admitting further entries is governed by
  `−(1+ℓ_i)(c_i−1)`, which is ≤ 0. Admitting the whole band is never worse than
  admitting only its bottom. The ladder offers the bottom (`16`, `64`, `256`,
  …) while the band tops are `15`, `63`, `255`, `511`, `1023`, …
- **Over-admission past the last repeated token.** Because a count-1 token has
  `value = 0` minus the width penalty, admitting a long singleton tail is
  **strictly harmful**: pure table bytes for zero escape saving. `min(65536, D)`
  (L464, L426) puts full admission on the ladder for every `D ≤ 65536`, so
  whenever the leaf has a long singleton tail the ladder's best choice is
  over-admission.

So the ladder regret is one-sided: `cost(ĉ) ≥ cost(c*)` always, with equality
only when `ĉ = c*`.

**MEASURED (§4.9, synthetic pages — see the caveat in §4.9):** over 119,249
randomised pages the frozen ladder is strictly worse than the exact optimum in
**97.31%** of them, regret exceeds 1% of the optimum in **92.44%**, exceeds 5% in
**52.94%**, total regret is **6.12%** of total optimal pre-backend cost, and the
worst single page loses **7,998 B**. A second seed reproduces
(97.36% / 92.32% / 52.76% / 6.13% / 7,962 B).

**This is a correctness defect against the frozen specification, not merely a
proxy-fidelity risk** — strictly stronger than the critic's K4, which concerns
pre-backend→q11 fidelity. Here the implementation simply does not compute the
objective PREREG §7.5 says it computes.

**Blast radius, stated honestly.** The defect's magnitude depends on a
distribution statistic **nobody has measured**: the singleton-tail mass
`Σ_{i>c₂}[1+ℓ_i]`. If D1–D4 columns are low-cardinality with no singleton tail,
the ladder regret is ≈0 and the defect is cosmetic. If they are
high-cardinality-with-local-repeats, the regret is ~6% pre-backend. **The
critic's D4 argument predicts the corpus may be in the benign regime.** This is
exactly why the census must report singleton-tail mass (§9.1) and why arm 5 must
exist (§9.2) — it is the only way to find out, and it costs one arm.

**Encode cost, quantified.** Current: `8 root caps × 8 overlay caps × pages`, each
page candidate re-ranking the page and re-resolving every occurrence by binary
search (L452–528) — `O(64·n_ℓ·log D_ℓ)` per leaf. Exact rule: `O(D_ℓ)`, computed
from the same frequency pass the ranking already needs. The exact rule is
**strictly cheaper and strictly better**; the ladder is dominated on both axes.

### 4.4 Cycle model — decoder

Backend is large-window q11 (L1332–1334 above). Per occurrence in a
dictionary-hit leaf, excluding the output copy:

| Step | Frozen SHAPE_COLUMN (leaf-major by construction, L342–355) | Frozen SOURCE_ORDER (interleaved, L862–882) |
|---|---|---|
| bit read `w_p` bits (128-bit accumulator, L650–689) | 4–6 cyc | 4–6 cyc |
| leaf resolve | 0 (cursor in registers) | 1 load + leaf-index calc |
| page resolve (`occ / policy`, L868) | strength-reduce to counter compare | integer division by runtime constant |
| `(offset,len)` lookup | 2 loads ≈ 4 cyc | 2 loads ≈ 4 cyc |
| escape branch (predictable, L877) | 1 cmp + 1 branch | same |
| **total non-copy overhead** | **~10–14 cyc** | **~18–26 cyc** |

Projections, not measurements. Two structural facts are cheap and real:

- **G5A and decode locality are the same decision.** `shape_column_coordinates`
  is built shape→slot→occurrence (L342–355), i.e. **exactly leaf-major**. So the
  production route G5A favours is the one where the frozen wire is already
  leaf-major and per-token indirection is removable **with zero wire change**,
  provable by body SHA-256 equality. G5A's ordering result (A3 −89,960 B vs A1,
  4/4) and the prereg's §10 sequential-page requirement therefore *agree* rather
  than conflict. This is a genuine free win — and, per
  `RESEARCH_LEDGER.md` L4820–4821 and MASTER-BRIEF item 12, I label it
  **wire-invisible / DECODE-SHORT**, not a crossing route.
- **Implicit entry length.** Entries are concatenated in ID order, so entry `i`
  spans `[off_i, off_{i+1})`. A single `u32` offset array replaces the `(offset,
  len)` pair: 4 B/entry instead of 8 B/entry, one load instead of two, and
  **zero wire bytes** (it is a decoder-internal structure derived from the
  charged table bytes). This halves the dictionary index footprint of the
  critic's §5.1 working set.

### 4.5 RSS model — and why the prereg's RSS gate is near-vacuous

The critic's §5.1 correctly identifies uncharged decoder memory. Two additions:

1. **Only the current page's overlay needs to be resident**, by construction —
   that is what paging buys. Root blob + current overlay per live leaf.
2. **The gate cannot bind.** `PREREG` §15 requires peak RSS ≤ 110% of the routed
   control. Both arms decode a large-window stream whose window is fixed at
   `2^30` bits regardless of the dictionary. For D1–D4/V1 (all ≤ 15.4 MB) the
   window dominates RSS by orders of magnitude more than any plausible
   dictionary footprint. A 110% band on top of a fixed multi-hundred-MiB base
   cannot detect a dictionary regression. Measured confirmation of the mechanism:
   the smoke run reports `decode_peak_rss_kib: 5416` on `README.md`
   (`RESEARCH_LEDGER.md` L4879) — a tiny file still pays a fixed cost.

**Required fix, pre-registered:** charge RSS as
`ΔRSS = RSS(candidate) − RSS(frozen control)` at identical window settings, and
add an explicit `L_decoder_memory` line to PREREG §9's `L_total` enumerating
`root_blob + root_index + current_overlay_blob + overlay_index` peak bytes.
Without it, per critic K5, the experiment has no budget to stay inside.

### 4.6 Escape economics — where I refute the critic

The critic (§5.3) asserts escape "only ever fires for tokens that are (i) beyond
a 65,536-distinct leaf, or (ii) not selected by the MDL cap" and is therefore a
"pure loss versus the flat dictionary" in regime (ii).

**Refuted for regime (ii) by §4.2.** The cap is an explicit argmin, and the exact
marginal rule admits while `(1+|e_i|)(c_i−1) > n_p·Δw_i/8`. For a leaf with
`D_ℓ = 2,000`, `n_ℓ = 100,000`, `ℓ̄ = 10`: `n_p/(8(1+ℓ̄)) = 1,250`, so tokens with
in-page count below ~1,250 escape and tokens above are admitted. Escape fires for
the *majority* of occurrences in exactly the mid-cardinality regime that NDJSON
logs occupy. It is not rare-by-construction; it is **adaptive to local
cardinality**, which is the whole point.

**Conceded for the fully-admitted regime.** When `c* = distinct`, no token
escapes, and the escape is pure overhead: `1` byte of `code_width` plus a
trailing empty region. The critic is right that on a low-cardinality leaf the
escape buys bounded decoder memory at a small ratio cost. That is a legitimate
trade, correctly characterised, and it belongs to the bounded-memory property
(§10.1), not to a ratio claim.

### 4.7 Gate arithmetic — I concede the D3 concentration completely

From `docs/I10-GROTLI-G3-RESULTS.md` §1, reproduced:

| | Raw | G2 whole | G3 DICT | G3 routed selected |
|---|---:|---:|---:|---:|
| D1 | 40,126 | 39,299 | 39,277 | 39,277 |
| D2 | 25,029 | 20,903 | 20,861 | 20,861 |
| D3 | 1,292,757 | — | 1,240,155 | 1,240,155 |
| D4 | 108,857 | 84,361 | 84,431 | 84,361 |
| Σ | 1,466,769 | — | 1,384,724 | 1,384,654 |

- PREREG §14.2 requires `Σ min(G3_selected, G5D arms) ≤ 0.995 × 1,384,654`, a
  saving of **≥ 6,924 B**.
- PREREG §14.3 requires `Σ best G5D dict ≤ 0.995 × 1,384,724`, also **≥ 6,924 B**.
- **D4 headroom is ≤ 70 B**: `G2_WHOLE` = 84,361 is a permanent candidate that
  wins ties (PREREG §3.2, §9.3), so G5D can recover at most `84,431 − 84,361 = 70` B
  on D4 even with a perfect dictionary.
- D3 is **89.56%** of the routed aggregate.

> **Conceded without reservation.** ≥ ~6,000 B of the required 6,924 B must come
> from D3 alone — roughly **0.50% of D3's 1,240,155 B** — on the single file where
> the mechanism can even be active, in a corpus where (per §3) the root/overlay
> split is wire-invisible on every single-page leaf. The critic's D1 is correct.

### 4.8 The confound — conceded, and it is worse than stated

Against `G3_REGION_DICT` (PREREG §1.2 semantics vs §7.2), G5D changes:

1. **page/overlay structure**;
2. **mandatory escape**;
3. **root ordering: descending frequency (L360–377, L438–445) instead of G2/G3 first-occurrence order**;
4. **a bounded root cap**.

**All four are real and none is isolated by PREREG §11.1–§11.3.** There is no
`flat + frequency-order + no-escape` arm anywhere in the source (critic §1.2,
confirmed by grep). Confound 3 is the most likely to carry bytes on its own:
first-occurrence table order is essentially arbitrary with respect to the value
distribution, whereas frequency order produces a strongly skewed packed-ID byte
stream that an adaptive backend can model. **A `PASS-G5D-DISCOVERY` under the
current design would be uninterpretable; a `NO-GO` would be ambiguous about which
of four changes failed.**

I checked one further candidate confound and it does **not** apply: G3's packed-ID
primitive (`tools/grotli_g3.cpp` L483–511) is the same bit-packing scheme G5D
inherits, so the ID-stream layout is not a fifth byte-level difference. The width
quantisation, the uvar lengths, and the component accounting are shared. The
confound set is exactly the critic's four.

**Second confound the critic named in §5.2 is the mechanism by which confound 3
pays:** because `w_p` is constant across a power-of-two band, admitting overlay
entries often costs **zero** ID width, so the overlay's real effect is
*renumbering IDs by page-local frequency inside the same width*. That is
adopt-class textbook frequency ordering, credited to the wrong arm.

### 4.9 Verification of §4.2/§4.3 — MEASURED, and it refuted me twice

`prototypes/swarm-2026-10-02/01-g5d-dictionary/space-bunny/exact_admission_verify.cpp`,
built `clang 22.1.8 -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Werror`, **exit 0**.
No corpus, no compression, no timing, no fuzzing campaign — a synthetic
algorithmic correctness fixture only, per PREREG §18.4–§18.5.

| Claim under test | Method | Result |
|---|---|---|
| The §4.2 algebraic reduction `cost(c) = T − Σ_{i<c}(1+ℓ_i)(c_i−1) + ν(c) + ⌈n·bit_width(c)/8⌉` | every `c` compared against a naive-from-scratch recomputation of the original wire formula | **0 mismatches / 119,249 pages** — seed `0x5eed1234`; **0 / 36,048** — seed `0xdeadbeef` |
| Early-exit marginal scan finds the exact optimum | scan vs exhaustive argmin | **FAILS: 92,733 / 200,000 mismatches (46.4%)** — refuted |
| Band-top rule (`c = 2^{k+1}−1`) finds the exact optimum | band-top vs exhaustive argmin | **FAILS: 76,821 / 200,000 (38.4%)** — refuted |
| Frozen ladder ≤ exact optimum (one-sided) | ladder argmin vs exact argmin | **HOLDS: never worse in 155,187 pages; strictly worse in 97.31%** |
| Ladder regret magnitude, seed `0x5eed1234` | over 119,249 pages | ≥1% of optimum in **92.44%**; ≥5% in **52.94%**; total regret **6.1188%** of total optimal; worst single page **7,998 B** (`D=1997`, `n=31,990`, ladder cap `64`) |
| Same, seed `0xdeadbeef` | over 36,048 pages | ≥1% in **92.32%**; ≥5% in **52.76%**; total regret **6.1294%**; worst **7,962 B** |

Two of my own hypotheses were **refuted by brute force before reaching this
report**, and I record that rather than hiding it:

1. I first claimed an `O(1)`-incremental early-exit scan. It is wrong, because
   the marginal `value(i)` has positive spikes at powers of two where
   `n_p·Δw/8` is charged, and is negative again immediately after — cost is **not
   unimodal**.
2. I then claimed a band-top rule. It is also wrong, because
   `[ν(|e_i|)+|e_i|](c_i−1)` is **not** monotone when token lengths vary, so even
   within one width band the minimiser is not the band top.

The surviving result is the one that matters and is now **verified**: the exact
minimiser is the exhaustive `argmin` over all caps, it is computable in `O(D)`
from the §4.2 running sums, and the frozen 8-point ladder is a one-sided
one-sided-under-estimator of it that loses materially in the
high-cardinality-with-singleton-tail regime.

**Honest caveat, stated as a limitation on the headline number.** The 6.12%
regret is measured on **synthetic pages whose generator I chose** (≈40 heavy
tokens with counts up to 5,000, remainder count-1, lengths 1–60). That generator
is deliberately hostile to the ladder, because it produces exactly the long
singleton tail that makes over-admission wasteful. **These numbers are therefore
an upper bound on the corpus regret, not an estimate of it.** Per §4.3, the
corpus regret is governed by singleton-tail mass, which is unmeasured for
D1–D4, and per the critic's D4 argument the corpus may well have almost none.
Nothing in this section licenses a ratio claim about D1–D4.

---

## 5. Asymptotic analysis

Let leaf `ℓ` have `n_ℓ` occurrences, `D_ℓ` distinct tokens, mean length `ℓ̄`.

| Representation | Pre-backend bytes | Growth |
|---|---|---|
| RAW_LEX | `n_ℓ·(1+ℓ̄)` | `Θ(n_ℓ)` |
| Flat dictionary, full admission | `Θ(D_ℓ·(1+ℓ̄)) + n_ℓ·log₂D_ℓ/8` | `Θ(n_ℓ log D_ℓ)` |
| G5D paged, exact admission | `Θ(D_ℓ·(1+ℓ̄) + Σ_p D_p·(1+ℓ̄) + n_ℓ·log a/8)` | `Θ(n_ℓ log a)` |
| TRGD (critic §11) | `Θ(T + Σ_p |retire ∪ insert|·(1+ℓ̄) + n_ℓ log W/8)` | `Θ(n_ℓ)` at fixed `W` |

The dictionary family is `Θ(n log a)` against `Θ(n)` raw, with the crossover at

```
n*  ≈  T / (1 + ℓ̄ − log₂|R|/8),      T = Σ_{e∈R}(1+|e|)
```

`|R|=64, ℓ̄=8` → `n* ≈ 70`; `|R|=1024, ℓ̄=8` → `n* ≈ 1,189`. This is the
**leaf-level admission threshold** in closed form, computable in `O(1)` with no
backend call — the mechanism by which a q11-free planner could ever be
admissible. It is *not* the same threshold G4 refuted: G4's proxies were
heuristic feature scores for operator *class*, whereas this is derived from the
exact charged wire. Whether it survives q11 is the empirical question of §9.

`log a` is the only super-linear term and it is bounded by 17 bits by the frozen
cap ladder. **Paging cannot reduce `a` below the leaf's own local distinct count
without retiring entries — and G5D's overlay is monotone (entries are never
retired), so under drift `a_p` ratchets up and `w_p` never falls.** The critic's
TRGD §11 is correct on this point and I adopt it as the correct next mechanism
if paging is killed: aging tokens out is the operation G5D structurally cannot
express, and it is the operation that drift economics actually require.

---

## 6. Novelty and prior-art boundary

**Zero primitive novelty is available, and the prereg already concedes it**
(PREREG §19, critic §4). Concordant:

- exact lexical dictionary of distinct column values — Parquet dictionary
  encoding; Brotli static dictionary (RFC 7932); Zstd trained dictionaries;
- per-page / per-block local alphabet — Parquet dictionary pages; Blosc /
  FastLanes block dictionaries; Brotli & Zstd block splits;
- fixed-width bit-packed symbol IDs — Parquet bit-packing hybrid;
- mandatory escape + raw payload — FSST escape; Parquet dictionary fallback;
- frequency-ordered symbol table — universal;
- `ceil(log2(n+1))` width quantisation — textbook;
- MDL cap selection — standard model selection;
- **base + overlay + escape as a combination** — MASTER-BRIEF doctrine 1
  disqualifies "an arbitrary combination of known components".

**The only candidate novelty neighborhood I can nominate**, with a decisive
separator for track 20:

> **Projection-accounted cross-leaf base amortization.** In Zstd/Brotli/LZMA a
> shared dictionary is one fixed table exposed whole to every context; the
> per-context rate charge is not a function of the context's intersection with
> the table. In columnar formats each column chunk owns its dictionary, so no
> cross-column amortization exists at all. A base whose per-leaf rate charge is
> computed from the **projection** `B ∩ D_ℓ` rather than `|B|`, with admission
> governed by `Σ_ℓ c_ℓ(e)`, has no prior-art instance I can identify.

**Decisive separator:** if the Stage-0 census shows mean per-leaf overlap
`|B ∩ D_ℓ|/|D_ℓ| < θ_cross`, the neighborhood is dead on this corpus regardless
of any other property, because the amortization argument has no base to amortize.
PREREG §7.1 explicitly defers cross-leaf roots, so this is a **separate**
hypothesis and must not be folded into the G5D run.

**Honest position: this lane cannot clear the project's mechanism-novelty gate,
and I do not claim it can.** Per MASTER-BRIEF item 1 and the prereg's own §19, its
only defensible output is an exact, isolated, fully charged Pareto demonstration —
which is precisely what the current control set prevents (§4.8).

---

## 7. Family engineering value vs discovery-design validity

The coordinator requires these be separated. They are, and they point in opposite
directions.

### 7.1 Family engineering value — SURVIVES a paging kill

Independent of ratio, paging, and page policy:

1. **Bounded decoder dictionary memory with exact behaviour.** G2/G3
   `EXACT_DICT` admits **every** distinct token per leaf with **no escape**
   (PREREG §1.2) — an unbounded decoder-side table whose size is an attacker- or
   corpus-controlled function of cardinality. `root_count ≤ 65,536` plus a
   mandatory escape converts that into a bounded allocation with a total,
   deterministic decode-time memory ceiling. This is a real property the current
   line does not have, it is independently verifiable, and it serves the
   format/security lane (track 19) directly.
2. **Fail-closed admission instead of silent full admission.** Forged or
   corrupted counts are bounded-checked pre-allocation (L961, L991) and the width
   is re-derived and re-checked on decode (L1010), with canonical-varint and
   zero-alignment assertions (L677). I found **no correctness hole by inspection**,
   matching the critic's §6.
3. **A 42-case malformed-input matrix** (PREREG §13) including truncation,
   overlong counts, noncanonical varints, arithmetic overflow, and adversarial
   seeded fuzzing.
4. **An exact, q11-free, single-pass admission rule** (§4.2). This is the only
   part of the family that is new *mathematics* rather than new plumbing — and it
   is new only in the sense that no prior dictionary format performs the
   marginal-vs-width-step comparison, because prior formats lack a mandatory
   escape at a top-of-alphabet code with explicit width accounting.

Items 1–3 are adopt-class and ratio-agnostic. **They justify keeping the family
alive.** Item 4 is the only candidate for an attributable result.

### 7.2 Discovery-design validity — FAILS

The current design cannot produce an interpretable verdict: four confounded
changes (§4.8), zero isolating arms, a gate concentrated on one file (§4.7), a
policy set that collapses to one configuration without a census (§3), and a
selector that does not compute its own stated objective (§4.3). **This is a
design kill, not a family kill.** The distinction matters: killing the design
costs one prereg revision; killing the family would discard item 7.1(1), the only
property here that nobody else in the project has.

---

## 8. Adversarial failure cases

1. **Census says paging is inert.** `mass_heavy = 0` (no leaf exceeds 4,096
   occurrences) → all three policies emit one configuration and the overlay is
   provably absent from every carrier. *Response: kill the paging axis at Stage 0,
   before any compression.* This is the cheap-kill the design needs.
2. **Census says the leaves are locally all-distinct.** High `L`, high `D_p`, every
   in-page token unique → escapes fire everywhere and `c* = 0` on every page.
   G5D degenerates to *worse than* G3 by exactly `2 B × Σ K_ℓ` of page framing
   (critic §5.4). The exact break-even in §4.2 detects this with counting alone.
3. **Drift is real but monotone.** Locally stable vocabulary that shifts between
   distant windows. G5D's non-retiring overlay pays a full-page table every
   boundary and ratchets `w_p` upward. TRGD wins. *Response: if census shows
   drift, pivot to TRGD rather than re-tuning G5D's caps.*
4. **The win is frequency ordering.** If `arm3 < arm4` accounts for the entire
   gain, the mechanism is adopt-class and belongs to G2. *Response: freeze that
   as the finding and close G5D — do not launder a G2 improvement as G5D.*
5. **Exact admission loses to the ladder after q11.** Possible: `c*` admits more
   entries, producing a packed-ID stream whose *empirical* entropy is worse than
   the more-compressible narrower stream the ladder picks. *Response: this is
   exactly what `R_MDL` must measure. If the exact rule loses complete bytes, the
   ladder is a better *entropy* heuristic and must be kept — with the
   specification corrected to say so, rather than claiming "exact serialized
   leaf cost".*
6. **D3-specific win.** The gate is D3-only (§4.7). A D3-only win on a single
   10 MiB GitHub-events file is a corpus anecdote. *Response: §9 requires ≥2 of 4
   strictly smaller for the paging credit, which D3-only cannot satisfy.*
7. **Decoder RSS.** Multi-leaf live dictionaries during SOURCE_ORDER
   reconstruction (critic §5.1). *Response: mandate `L_decoder_memory`, measure
   ΔRSS, and note that G5A's favoured SHAPE_COLUMN order is already leaf-major,
   which structurally reduces the live set.*
8. **Degenerate policies.** Reported `NULL`, never `PASS` (critic §10.6).
9. **Backend large-window leakage.** All arms share it, so comparisons are
   internally valid; but any *external* Brotli reference at lgwin24 is not a
   valid control for these corpora. *Response: state the window in every
   artifact; never compare G5D bytes to a lgwin24 baseline.*
10. **Second-order confound: cap × order interaction.** The exact rule's `c*`
    depends on counts; frequency order makes the *packed bytes* skewed. A
    factorial that varies order but not cap will still conflate them. *Response:
    `arm4`/`arm3` must be run at **full admission with no cap**, so cap and order
    are separated.*

---

## 9. Redesigned decisive remote experiment (one dispatch, census-gated)

I adopt the critic's two-stage structure and its `arm6 − arm5` credit rule. I
change three things: the census metric (§9.1), the arm set (§9.2), and the gate
thresholds (§9.3).

### 9.1 Stage 0 — counting-only census (no compression, no timing)

Per shape-slot leaf, per file, for D1–D4 (V1 context only, zero weight):
`n_ℓ`, `D_ℓ`, and per page `n_p`, `D_p`, `c₂,p`, plus:

- `K_ℓ(P) = ⌈n_ℓ/P⌉`; `pages_total(P) = Σ_ℓ K_ℓ(P)`.
- `mass_heavy = (occurrences in leaves with n_ℓ > 4096) / total_occurrences`.
- `local_repeat_share = (Σ_p n_p with D_p < n_p) / total_occurrences` — **this is
  the metric the critic's `mass_heavy_distinct` cannot express.** Global
  distinctness cannot distinguish "globally large, locally stable" (overlay
  active) from "globally large, locally all-distinct" (overlay inert, escapes
  everywhere). §4.2's exact quantity does.
- `BE_overlay_exact = Σ_p max(0, cost_p(0) − cost_p(c*_p))` using the `O(D)`
  running sums of §4.2 — no cap enumeration, no compression.
- `R_ladder = Σ_p (cost_p(ĉ_p) − cost_p(c*_p))` — the §4.3 regret, also
  counting-only, and verified one-sided (§4.9).
- `singleton_tail_mass = Σ_p [ Σ_{i>c_{2,p}} (1+ℓ_i) ] / total_escape_bytes` —
  **the statistic that actually governs the ladder regret** (§4.3, §4.9 caveat).
  This is the single most decision-relevant quantity the census can produce for
  the exact-admission question, and it costs no compression.
- `flat_equivalent_floor = 2·pages_total(P)` per policy (critic §5.4).

**Pre-registered Stage-0 gates (frozen before dispatch):**

- `INVALID` on any identity/hash/round-trip failure.
- `KILL-PAGING` if `BE_overlay_exact(D3) < 6,924 B` **or**
  `mass_heavy < 0.02` **or** `local_repeat_share(D3) < 0.10`.
- `KILL-DEGENERATE-DESIGN` if `pages_total(D3, P=4096) < 500`.
- `REPORT-LADDER-DEFECT` (no gate weight; an engineering finding) with
  `R_ladder > 0`.
- `PROCEED` only if none trips.

### 9.2 Stage 1 — factorial ablation (attribution-fixed)

Same frozen G3 carrier, same q11/lw30 backend, both orders, pinned binaries,
alternating arm order, median of ≥9 with warmups. **Cell 3 is the critic's and
it is mandatory**; cells 4–5 are mine.

| # | Arm | Isolates |
|---|---|---|
| 1 | `RAW_BROTLI` | floor |
| 2 | `G3_REGION_DICT` reproduced | control (first-occurrence, full admission, no escape) |
| 3 | **`FLAT_FREQ_FULL`** — single page, all distinct, **frequency order**, no escape, no cap | **frequency ordering alone (critic K1(iii))** |
| 4 | **`FLAT_FREQ_CAP_ESC`** — single page, root ≤ 65,536, frequency order, mandatory escape | **cap + escape alone (K1(ii)+(iv))** |
| 5 | **`FLAT_FREQ_CAP_ESC_EXACT`** — as 4 but cap by the §4.2 exact rule, not the ladder | **ladder discretisation alone (§4.3)** |
| 6 | `G5D_P{4,16,64}K` — pages + overlay | **paging alone (critic K1(i))** |

**Frozen credit rule.** `paging` is credited **only** from `arm6 − arm5`. The
frequency-ordering gain (`arm3 − arm2`) is credited to **G2/G3, not G5D**. The
cap/escape gain (`arm4 − arm3`) is credited as the **bounded-memory engineering
property of §7.1**, with no ratio novelty. The ladder gain (`arm5 − arm4`) is
credited to the **exact-admission rule**, and only if it survives q11.

This is the single structural repair the critic's §3 K1 demands: arm 5 is the
*paging-free parent* of arm 6, identical in cap rule, ordering, and escape, so
`arm6 − arm5` is a one-variable contrast.

### 9.3 Pre-registered thresholds

- **`KILL-PAGING`** if `Σ(arm6 − arm5) ≥ 0` (paging is net-negative against its
  own flat parent). Primary kill, cheapest and cleanest.
- **`NO-GO-G5D-PAGING`** if `Σ(arm6 − arm5) > −0.5%·Σ arm2`, or if arm6 < arm5 on
  fewer than 2 of 4 files.
- **`PASS-G5D-PAGING`** requires **all** of:
  1. `Σ(arm6 − arm5) ≤ −0.995·Σ arm2` (paging isolated, ≥0.5%);
  2. arm6 < arm5 strictly on ≥2 of D1–D4;
  3. `arm4 ≤ arm2` — the bounded-escape dictionary is not itself a regression;
  4. `arm3 < arm4` reported, its gain credited to G2/G3 only;
  5. all 42 malformed-input gates pass; all arms round-trip exactly; component
     sums reconcile;
  6. `R_MDL` reported; **no** threshold depends on it;
  7. `L_decoder_memory` charged and `ΔRSS ≤ 110%`;
  8. timing fields quarantined as `diagnostic_only_*`;
  9. V1 reported as `KNOWN-STRESS`, zero gate weight.
- **`PASS-EXACT-ADMISSION`** (separate, does not gate paging) if
  `Σ(arm5 − arm4) ≤ −0.25%` aggregate and ≤ −0.25% per file — the frozen G4
  aggregate fidelity bar, so a pass is directly comparable to G4's failure.
  A pass means the ladder is a real, attributable loss; a fail means the ladder
  is an entropy heuristic that beats its own stated objective, and the
  specification must be corrected rather than the code. **Because the synthetic
  upper bound is 6.12% pre-backend (§4.9) and the q11 stage will attenuate it,
  this arm must be measured, never assumed from §4.9.**
- **`GO-CROSS-LEAF-BASE`** only if Stage 0 shows mean overlap `|B ∩ D_ℓ|/|D_ℓ| ≥
  0.15` on ≥2 of the 3 structured-eligible files. Otherwise killed without
  compression.
- **Inherited unchanged:** PREREG §15 performance gate (≥95% dict-recon decode,
  ≥95% e2e decode, ≤110% RSS, ≤2× encode, or the speed path), §16 held-out gate
  (≥1% aggregate vs raw, ≥2 of 3 objects, V1 excluded).

---

## 10. Reconciliation with the critic — the seven obligations

**1. Do I credit any gain to paging rather than to ordering/escape/cap?**
**No.** §9.2's credit rule admits exactly one arm that can move: `arm6 − arm5`,
where arm 5 is a paging-free parent with identical cap rule, ordering, and
escape. I additionally split frequency ordering (`arm3 − arm2`) and the ladder
discretisation (`arm5 − arm4`) into separately credited, separately reported
contrasts. Under the *current* prereg I would have had to credit all four changes
to one number; that is the defect being fixed.

**2. Do I report the flat-equivalent `+2 B/page` floor?**
**Yes, and I strengthen it.** I report it two ways: as the pre-backend floor
`2·pages_total(P)` (critic §5.4), and — stronger — as the structural result of
§3 that root and overlay share one code space, so for any leaf with `n_ℓ ≤ P` the
overlay is not a *distinct* mechanism at all. I claim no per-page gain anywhere.

**3. Do I acknowledge D3 concentration, and what is my D3 projection?**
**Acknowledged completely** (§4.7). Gate ≥6,924 B; D4 headroom ≤70 B; D3 = 89.56%
of the aggregate; therefore ≥~6,000 B must come from D3, ≈0.50% of D3's
1,240,155 B. **My D3 projection is: I do not have one, and I decline to invent
one.** No per-leaf occurrence or distinctness distribution exists for D3 in any
results document, and any BE_overlay number for D3 would be fabricated. The
Stage-0 census exists precisely to produce it for free before anyone is allowed
to guess.

**4. Do I treat the three page policies as three distinct arms without a census?**
**No — and I claim the opposite.** §3: for every single-page leaf all three
policies emit the same carrier except one label byte, so the six dictionary arms
collapse to at most one configuration unless a leaf exceeds 4,096 occurrences.
The design must not treat them as three arms until the census says otherwise,
and `KILL-DEGENERATE-DESIGN` (`pages_total(D3, P=4096) < 500`) exists for
exactly this.

**5. Are my numbers MEASURED or PROJECTED?**
Every ratio number attributed to G2/G3/G5A/G5B/G4 is MEASURED with a cited
result document. Every G5D number is **PROJECTED or DERIVED and is labelled as
such** (§4.2, §4.3, §4.4, §5). **Zero G5D corpus bytes exist**; the cycle and RSS
figures are architectural projections. I assert no G5D result anywhere.

**6. Do I charge `L_decoder_memory` anywhere?**
**Yes, as a required addition** (§4.5): PREREG §9's `L_total` must gain an
explicit `root_blob + root_index + current_overlay_blob + overlay_index` peak
term, and RSS must be reported as `ΔRSS` against the frozen control at identical
window settings. I add the reason the critic could not have known: the backend is
large-window q11 (`third_party/brotli/c/enc/encode.c` L1332–1334), so a fixed
2^30-bit window dominates RSS on inputs ≤15.4 MB and the existing 110% band
cannot bind. I also supply the free mitigation — a single `u32` offset array
replacing `(offset,len)` pairs (§4.4), halving the index footprint at zero wire
cost.

**7. Disagreement on `arm6 − arm5` as the primary kill?**
**None. I adopt it verbatim** and make it the primary kill.

### Where I concede to the critic

- D1 (D3 concentration) — conceded in full, with reproduced arithmetic.
- D2's *conclusion* — conceded and strengthened (§3).
- D4 (occurrence/distinct anti-correlation) — conceded as the central risk. The
  mechanism needs the quadrant the corpus plausibly does not populate.
- K1 (four-way confound) — conceded in full; §4.8 confirms confound 3 from
  source (L360–377, L438–445) and identifies confound 2's payoff mechanism
  (§4.8, the critic's §5.2).
- K2 (prior-art collapse) — conceded; §6 nominates one narrow neighborhood with
  a decisive separator and claims no novelty.
- K4 (selector risk) — conceded and **escalated**: the selector does not compute
  the objective PREREG §7.5 names (§4.3).
- K5 / §5.1 (decoder memory) — conceded and extended (§4.5).
- §5.5 (encoder cost) — conceded and quantified: `O(64·n·log D)` today versus
  `O(D)` exactly.
- §8 timing-quarantine recommendation — adopted.
- §9 "no G5D ratio number exists" — adopted verbatim.

### Where the critic is wrong, with the refutation

- **§5.3 / D2 reasoning — "the overlay pays `table_bytes + 2` to save at most the
  escape bytes it displaces."** This ignores multiplicity: admitting a token
  displaces `c_i` escape occurrences, not one, and costs one table entry. The
  marginal is `(1+|e_i|)(1 − c_i)` (§4.2), which is **strictly negative** for
  every token with in-page count ≥ 2. So the MDL optimum is *not*
  `overlay_count = 0` in general. Consequence for §5.3: escape fires routinely in
  the mid-cardinality regime (worked example: `D=2,000`, `n=100,000`, `ℓ̄=10` →
  tokens below ~1,250 in-page count escape). I concede the escape is inert only
  in the **fully-admitted** regime (§4.6).
- **§10's census statistic `mass_heavy_distinct` (L>4096 AND D>4096)** is
  insufficient as a decision variable. Global distinctness cannot separate
  "globally large, locally stable" (overlay active, escapes rare) from "globally
  large, locally all-distinct" (overlay inert, escapes everywhere). It will pass
  populations that paging cannot help and fail populations it can.
  `local_repeat_share` + `BE_overlay_exact` (§9.1) are the correct variables.
- **§10's `overlay_break_even_bytes`** is a correct but expensive formulation: it
  enumerates caps. §4.2's marginal identity gives the same quantity in closed
  form at `O(D)` per page, which is what makes a counting-only census cheap
  enough to run over D1–D4 in one remote pass.
- **§11 TRGD is the correct successor *if* drift is real.** I adopt it as the
  designated follow-on. My one addition: TRGD's crossover must be computed
  against **arm 5**, not arm 2, or it inherits the same four-way confound the
  critic is correctly killing.

---

## 11. Minimum prototype — DELIVERED AND VERIFIED

`prototypes/swarm-2026-10-02/01-g5d-dictionary/space-bunny/exact_admission_verify.cpp`.
Bounded, isolated, not wired into any production path, no commit.

**What it does:** (a) verifies the §4.2 algebraic reduction of the charged page
cost against a naive recomputation of the original wire formula; (b) refutes the
two closed-form minimiser hypotheses by exhaustive comparison; (c) measures the
frozen-ladder regret and confirms it is one-sided.

**Result:** exit 0; algebraic reduction exact on 155,287 pages across two seeds;
both closed-form hypotheses refuted; ladder strictly worse than optimal in
**97.31% / 97.36%** of pages with total regret **6.12%** of optimal and worst
single page **7,998 B**. Full table in §4.9.

**Constraints honoured:** no corpus was read, no compression was run, no timing
was taken, no fuzzing campaign was run. Synthetic algorithmic correctness fixture
only, which PREREG §18.4–§18.5 permit locally. This prototype is **not** required
for the remote pilot and does not unblock dispatch.

---

## 12. Final recommendation

> ## HOLD (family) · KILL (the current G5D discovery design as preregistered) · PILOT (one census-first, attribution-fixed, remote-only ablation)

**I converge with the critic's ruling family, and I downgrade my own prior
leaning.** I had been heading toward "PILOT the dispatch as preregistered." The
four-way confound (§4.8), the D3-only gate (§4.7), and the policy degeneracy
(§3) each independently make that dispatch uninterpretable. Spending a remote
cycle to learn "G5D passed, but we cannot say which of four changes did it" is
worse than not spending it.

**HOLD the family**, because of §7.1(1): `root_count ≤ 65,536` plus a mandatory
escape converts G2/G3's unbounded, cardinality-driven decoder-side table into a
bounded allocation with a deterministic decode-time ceiling. That property is
real, is nobody else's in the project, is ratio-agnostic, and serves track 19.
It survives a paging kill intact.

**KILL the discovery design as preregistered** — not the family. The kill is
cheap to reverse: PREREG §7.5 (selector objective), §11 (add arms 3–5), §9 (add
`L_decoder_memory`), and §14 (replace the single threshold with the §9.3 credit
rule) are the four edits required. All four are *narrowings or additions*, none
is a new leaf family, none touches FSST/defaults/hierarchy, and none admits a
threshold chosen after outcomes are visible.

**PILOT exactly one remote dispatch**, in this order:

1. **Stage 0, counting only.** No compression, no timing. Decides
   `KILL-PAGING` / `KILL-DEGENERATE-DESIGN` on the census metrics of §9.1, and
   reports `R_ladder` and `flat_equivalent_floor` as zero-gate-weight
   engineering findings. Counting is an order of magnitude cheaper than the
   compression sweep, and it is the only way to avoid spending the sweep on a
   mechanism that the census would have shown inert.
2. **Stage 1, only if Stage 0 does not kill.** The six-arm factorial of §9.2
   with `arm6 − arm5` as the primary kill, plus the attribution splits that make
   a pass interpretable and a fail unambiguous.

**Not PROMOTE-TO-REMOTE** for this cycle, and **not** a novelty promotion in any
future cycle. Per §6 the family is adopt-class with zero primitive novelty
available; the one candidate neighborhood (projection-accounted cross-leaf base
amortization) is gated behind a census overlap threshold that has never been
measured and that the discovery corpus plausibly fails.

**Sequencing constraint I cannot discharge myself.** `RESEARCH_LEDGER.md`
L4946–4951 orders the dense-frontier authorization *before* G5D publication, and
L4953–4964 names the uncommitted files. I performed no commit, push, or reset,
and I am forbidden from doing so. Adding arms 3–5 additionally requires editing
`tools/grotli_g5_paged_dictionary.cpp`, which I am forbidden to modify; it must
land as a separate authorized commit by the owning lane. **G5D dispatch remains
blocked on user authorization, and Stage 0 does not unblock it.**

**What the verified §4.9 result changes, and what it does not.** It converts one
line of my analysis from *projected* to *measured*: the frozen 8-point cap ladder
is a **one-sided under-estimator** of the exact minimiser of the objective
PREREG §7.5 names, verified over 155,287 synthetic pages across two seeds, losing
in 97.3% of them and by 6.12% of total optimal pre-backend cost in the worst
case my generator could produce. **It does not move the verdict.** The corpus
magnitude is governed by singleton-tail mass, which is unmeasured; if D1–D4 are
low-cardinality the regret is ≈0 and the whole arm-5 question evaporates. That
is precisely why `singleton_tail_mass` is now a Stage-0 census output and arm 5
is in the Stage-1 design: the measurement is free, and the pilot is where the
number has to come from.

**What would change this verdict.** Only two things: a Stage-0 census showing
`local_repeat_share(D3) ≥ 0.10` *and* `BE_overlay_exact(D3) ≥ 6,924 B` (which
would make the paging axis live on the one file that can carry it and would
restore the case for arms 3–5), or a Stage-1 result where `arm3 < arm4` accounts
for the whole win — which would relocate the finding to G2 and close this lane
without further remote spend. I would not change it for any argument, only for
those measurements. In particular, **§4.9 alone does not change it**, and I have
tried it.