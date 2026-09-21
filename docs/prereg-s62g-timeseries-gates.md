# PRE-REGISTRATION — S6-2G: Transform-Acceptance Gates for Timeseries Shapes (Gorilla XOR / vXOR) in ANVIL

Author: `pnra-cost`. Status: **PRE-REGISTRATION ONLY — NO CODE, NO CLAIMS.**
Build is gated behind (a) coordinator freeze order ("build only after freeze")
and (b) S6-1b landing (decode-swarm queue). This document transfers the
measured S6-2 methodology (docs/s62-threshold-formula.md) to the
float/timeseries domain BEFORE any implementation exists, so that when the
lane opens it opens with falsifiable gates, not tuning.

Sources read and cited: `grotli-codec/refs/FINAL.md` (reconciled v2),
`grotli-codec/src/lib/grotli.ts` (detector + Gorilla core), coordinator
briefing 2026-08-21, Pelkonen et al. VLDB 2015 (Gorilla). Every figure below
is tagged with its derivation class per FINAL §3.1: **[measured-exact]**,
**[measured-approx]**, **[modelled-capacity]**, or **[design]** (this doc's
own proposals — all of §3–§5 are [design] until measured).

---

## 1. Lineage — what transfers and what does not

Measured results being transferred **[measured-exact]**:

- S6-2 (u5): a shape-scaled minimum-gain margin (`γ·FramingRaw`, γ=0.5)
  fixed EXP. X's over-commit at parse time; flat and distance-aware margins
  are measured dead ends; the regression was trajectory-borne, so the gate
  must live at the decision site, not post-parse.
- u2/F1: committed tokens can be individually sound while the aggregate
  regresses — displacement of downstream matches is invisible per-token.
- EXP. L: stream-level codec choice by measured size is 100% faithful.
- Grotli FINAL §2.3: vXOR wins ONLY where alignment + low residual entropy
  exist (json-lines −36.3% [measured-approx]); csv/log/text/prose correctly
  NEUTRAL; random mandatory 0% with CI tripwire.

What does NOT transfer: the specific γ=0.5 constant. It was derived from
mode-14 type-3 coded/raw framing ratios on PE binaries. The timeseries domain
has different framing economics (per-value control bits vs per-token varints)
→ the FORM transfers, the CONSTANT must be re-derived on timeseries
calibration data (§4). Pre-announcing γ_G=0.5 here would be fiat — the exact
thing FINAL.md purges.

## 2. Two integration shapes — only one gets an acceptance threshold

**Shape A — post-parse stream transform (vXOR/Gorilla as a literal-stream
codec alternative, chosen by measured size).** ANVIL's existing
`consider_parse`/`best.payload.size()` machinery already implements the
`min()` no-worse guard structurally. No amortization threshold is needed —
this is EXP. L's J-selection territory, measured 100% faithful. Trajectory-
safe by construction: the LZ parse is unchanged, so no displacement cascade
exists. Gates: guards only (§5).

**Shape B — in-parse transformed-run token (parser emits an xor-run span).**
This has the FULL EXP. X displacement hazard: committing the span changes
hash-chain state and downstream candidates. Any Shape-B proposal REQUIRES the
stated-formula gate below plus the S6-2 held-out contract. Recommendation
[design]: prefer Shape A first (cheaper, safer, precedent-backed); Shape B
only if Shape A leaves measured headroom that in-parse competition could
capture.

## 3. S6-2G stated formula (Shape B only) — form frozen NOW, constant later

For a candidate xor-run token covering N values of width w ∈ {32,64}:

```
saving_est(run)  = alt_c − token_c          [bits; shared-formula comparison
                                             against the same span coded as
                                             literal run / ordinary match]
FramingRaw_G(N, w, frames) =
    8                                          // type/mode symbol
  + 8·vb(N−1)                                  // run-length varint
  + w                                          // base value (first, raw)
  + Σ_i frame_bits(v_i)                        // Gorilla frames:
                                               //   A: 1; B1: 2+mb_i;
                                               //   B2: 13+mb_i  [grotli.ts:459-481]
  + block params                                // lead/trail window state or
                                               //   per-block table, per design

ACCEPT iff  saving_est ≥ γ_G · FramingRaw_G(...)
```

Frozen NOW [design]: the form (shape-scaled margin over raw fixed framing,
including per-value control bits — the direct analog of mask+tmask words).
Frozen LATER on calibration data: γ_G, and whether B2-heavy runs need a
tier-aware surcharge (B2 carries 11 extra control bits/value — if calibration
shows B2-dominant runs misprice, the surcharge enters the FORM now as an
optional term `+γ_2·13·count(B2)` decided before any held-out datum, not
after). Calibration set: the sweet-spot corpus (§6) minus held-out; verdict
contract identical to S6-2 §8 (held-out only, median-3, no re-tuning, clean
negative acceptable).

## 4. Detector (auto-detect) — adopted constants, never re-tuned here

Adopted VERBATIM from `grotli.ts:787-872` **[measured in grotli's codebase;
adopted-not-rederived — re-tuning on ANVIL corpus would need its own held-out
contract and is explicitly OUT OF SCOPE]**:

- `H_THRESH = 7.5`, `S_THRESH = 0.60`, `R_THRESH = 0.15`
- strong-float override: `S > 0.85 ∧ H < 7.6 ∧ G_s > 0.6`
- meaningful-Gorilla signal: `G_s > 0.55 ∧ exponentVar < mantissaVar`
- ASCII hard block: `highByteFraction < 0.02` → never Gorilla/vXOR

Route rule: detector fires → Shape-A codec becomes *available* in the
block-level min(); it never forces selection. Non-firing → transform
unavailable (zero encode cost on non-timeseries blocks — the S6-2 no-op
property, structurally).

## 5. Mandatory no-worse guards (all frozen; each is load-bearing)

1. **ASCII guard** (§4) — prose/JSON never routes. [grotli-measured]
2. **Residual-entropy engage gate**: transform engages only if
   `H(residual_after_transform) ≤ H_byte_raw − 2%`, measured on the live
   buffer. [grotli AUTOSQUEEZE §1.4]
3. **ε_strict reversibility/net-win gate**: adopt only if net win >
   `max(2, ⌈N·0.005⌉)` bytes, monotone adjacency-closed. Descriptor/
   record-boundary bytes count INSIDE the cost (no free riders — FINAL §3.5).
4. **Block-level min()**: final payload = min(transformed, untransformed) —
   ANVIL's existing best-payload selection provides this structurally.
5. **Record-boundary serialization requirement** (from grotli's two vXOR
   losslessness blockers, FINAL §4.3): record lengths/stride MUST be
   serialized in the descriptor and stride ≥ max record length. Bit-exact
   round-trip is a GATE, not a hope. [design-for-ANVIL; grotli's bugs are
   measured cautionary evidence]
6. **Random negative control**: uniform-random input MUST produce
   byte-identical output to the baseline path (ANVIL equivalent of grotli's
   CI-enforced `reductionPct=0` tripwire). Any positive reduction on random
   = implementation bug, auto-FAIL.
7. **Non-timeseries zero-regression**: on every corpus file where the
   detector does not fire, output byte-identical to baseline (S6-2's no-op
   guarantee, preserved structurally).

## 6. Corpus prerequisite (BLOCKING for any measurement)

corpus-expand to add, BEFORE any gate measurement: (a) smooth-f64 telemetry
generator (~0.001°/sample drift — grotli's measured Gorilla-unfriendly
generator at 0.25°/step is the documented anti-example [FINAL §2.3]);
(b) real NDJSON/columnar sets that trigger vXOR (H0 ≈ 0.64 target class);
(c) their non-smooth and random counterparts as controls. Without these the
only honest outcome is NEUTRAL everywhere — which is exactly what the guards
should then produce.

## 7. Falsifiable decision rules (pre-committed)

- **Shape A adoption**: detector-firing held-out files aggregate strictly
  smaller at median-3 AND guards hold everywhere else (byte-identical);
  magnitude reported whatever it is; derivation class of the headline set by
  the actual measurement (measured-exact requires real round-trip, not
  entropy model).
- **Shape B adoption**: additionally the S6-2G threshold frozen before
  held-out, decode ≤10% regression, flip analysis in the dual-basis
  (record-intersection + counter) form from day one — reconcile.py method,
  learned the hard way in u5.
- **Failure of either** = clean negative recorded; the guards' NEUTRAL
  behavior on non-sweet-spot data is itself the expected outcome on most of
  the corpus and is not a failure.

## 8. Explicit non-goals

No code in this pre-registration. No src/anvil.cpp edits. No detector
re-tuning. No corpus generation (corpus-expand owns §6). No claims — every
number here is [design]/[adopted] until measured under §7. Sequencing:
behind S6-1b per coordinator; nothing here displaces the I8 DAG.
