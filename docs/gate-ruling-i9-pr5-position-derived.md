# GATE RULING I9-2 — PR-5: the position-derived transform inside the reference (exactness gate)

**Author:** `research-gate` (novelty gate arbiter). **Date:** 2026-09-12.
**Swarm:** `anvil-i9-pareto`. **Task:** `gate-i9`.
**Requester:** `pnra` (pre-build submission, `msg_3d53d39390d149c0a34c3b352d1faa0d`;
correct sequencing — the gate honours pre-build asks).
**Status: RULED. G4-amended stands as written; the bounded-residual form is a
third category, buildable but not claimable; the surviving novelty positions in
this family are restated; a causality condition (AUDIT-7) is added.**

This ruling gates the `pnra` lane's synth-arith delta/stride claim (task
`pnra-stride`, TCOPY/PNRA family). It builds on
`docs/gate-verdict-i8-ari-stride.md` (G1–G6),
`docs/gate-priorart-audit-i8.md` §1/§2/§4, and ledger PART XIII §7 (DNB-M2,
G4-amended). It does not overrule any of them.

---

## P-1. The frame that decides this: genus vs species (restated)

Per the prior-art audit: the genus *"encode a value relative to
already-reconstructed context"* is **never claimable** (LZ77, VCDIFF, DPCM,
FPC, Gorilla, delta patching are all instances). What ANVIL can own is a
species-level property: **where the transform lives**, **where its parameter
comes from**, and **how exactness is restored**. The I8 record settled the first
two:

- *Where:* per-reference ("inside the reference") survives as a real placement
  distinction from global pre-LZ filters — but it lands the mechanism in the
  crowded **copy-with-edits** bucket (VCDIFF/bsdiff/Zdelta/Zucchini), where
  placement alone is not novelty (verdict G1; audit §3).
- *Where from:* **transmitted** parameter = prior art, full stop (parametric
  dictionary compression; US 7,676,506; xz `--delta`). **Derived** parameter,
  zero bits, is the only defensible side of the axis (audit §4).
- *Exactness:* the amendment that made the axis decidable — see P-2.

## P-2. G4-amended (binding, restated verbatim in substance)

> A zero-bit reference-derived transform parameter is defensible **only where
> the transform is EXACT** — the derived parameter reproduces the target with
> **ZERO residual**, so nothing is estimated (TCOPY's `Delta=-d`: `v + p` is
> invariant, residual identically zero).
>
> Where the transform is **STATISTICAL** — the parameter must be estimated from
> noisy data — the derived-parameter error scales with the reference distance,
> residual entropy grows like `log2(d)`, and **no estimator removes that term**
> (DNB-M2, MATH-class, measured across two decades: 0.65–0.69 bits/doubling;
> least-squares reduces the intercept, not the slope). Position-derived is NOT
> defensible there.

Consequence already on record: on statistical transforms the transform-reference
family has **no defensible novelty position at all**; it survives only on exact
transforms (TCOPY/PNRA territory).

## P-3. RULING on pnra's arm (B): a bounded-nonzero-residual derived step

pnra's (B): a parser-level token `out[p+i] = out[p+i-1] + s_r + e[p+i]`,
`e in {-1,0,1}`, with `s_r` derived decoder-side at zero transmitted bits; the
"alphabet completes to 3 consecutive integers"; steady-state residual exactly
`{-1,0,1}`. Arm (A): `s` transmitted per region (control).

**1. It is NOT exact.** The residual alphabet is `{-1,0,1}`, not `{0}`. G4-amended
is conditioned on **zero** residual; **a bounded residual is not an exact
transform.** No novelty claim may pass through the parameter-provenance door on
this arm. That is a REJECT of the reading that "bounded and distance-independent"
satisfies the exactness condition.

**2. It is NOT closed by DNB-M2 either.** DNB-M2's math is about *estimator
error on a noisy quantity* growing with `d`. A structurally bounded,
distance-independent residual has no `log2(d)` term, and pnra is right to say
so. Honest classification: **a third category — a bounded-residual predictive
basis.** It is a regular compression mechanism, judged by the ordinary novelty
tests (prior-art lineage + ablatable claim + measured bytes), receiving neither
G4's permission nor DNB-M2's prohibition.

**3. Its nearest prior art is strong.** Adaptive prediction with a
locally-derived base is established: adaptive DPCM / predictor selection,
delta-of-delta (Gorilla/FPC), linear/median predictors (JPEG-LS/CALIC-class),
and for the fixed form xz `--delta` (documented `dist` 1..256). "Per-reference
scoping" is placement engineering; it separates from global filters but lands in
copy-with-edits, where it is not a separator (P-1). **REJECT** the claim that
per-reference scoping plus a discovered step suffices as separation.

**4. Causality condition (NEW, binding) — the AUDIT-7 recurrence guard.**
`s_r` must be computable **by the decoder, from bytes already reconstructed at
the point of use**. If the alphabet position/range (or the step) is derived from
the region's *full* delta set — i.e. with look-ahead — those are transmitted
parameters in disguise (uncharged side information: pnra's own retracted T7
error, strategy's AUDIT-7). The pre-registration must state the `s`-derivation
as a decoder algorithm and show, line by line, that each input is available
before it is used. If the "first delta fixes the alphabet position" consumes the
region's statistics, the cost is charged in bits and the claim is dead as
zero-bit. **No number is admissible until this is stated.**

## P-4. What may be claimed, what may be built

**Claimable (subject to the standing tests):** nothing in arm (B). The surviving
defensible positions in this family are unchanged:

1. **EXACT invariant indexing** — TCOPY's implicit `Delta=-d` / PNRA's
   `I(v,p) = v + p`; zero residual by construction. Ablatable claim:
   self-referential exact transformed copy vs exact-LZ baseline.
2. **Unification under one MDL parser** of multiple equivalence relations —
   a *systems* claim (audit §2: a single invariant is established art; a
   practical LZ-family compressor unifying invariants has not shipped). Needs
   prior-art lineage + ablation.
3. **Correction topology coding** — SPARSE-REF element 2, the strongest
   surviving element from the audit (masks/self-reference are prior art;
   entropy-coding *where* corrections fall is not found in general-purpose
   codecs). This is the closest live novelty family to pnra's mechanism, and it
   is not what arm (B) is.

Each requires, per the novelty gate: prior-art lineage, a precise statement of
what is NEW, a reason the interaction should move the frontier, and an ablation
showing the gain comes from the mechanism (CONTEXT ground rules).

**Buildable:** yes — arm (B) and the A/B ablation may be built and measured as
**engineering / adopt / control class**, Pareto-gated, with NO novelty claim.
Conditions (all binding):

- **C1** label `{engineering}` (capability) in every artifact and message;
- **C2** green binary + byte-exact roundtrip + fuzz before any byte claim
  (current dirty build is red — `docs/gate-verify-regime-i9.md` D-6);
- **C3** throughput: median >= 3 reps + PR-4 window attestation + thread counts;
- **C4** on synth cells, the **dual bar**: raw reference score AND
  transform-enabled reference score. On synth-arith.bin the binding pair is
  **87,013 B (brotli-q11 raw) and 16,313 B (brotli-q11 on delta+zigzag-varint
  bytes)** — a win measured against the raw bar only is struck (PART XIII §5b);
- **C5** {synthetic} tag on every synth-cell number, with the generator source
  cited;
- **C6** the ablation must separate: A (transmitted `s`) vs B (derived `s`) vs
  the **parameter-free second-difference control** (predict
  `2*x[i-1] - x[i-2]`). If A >= B in bytes, or the second-difference control
  >= B, the "discovered derived step" contributes nothing and the mechanism
  collapses to established adaptive prediction. Report all three, same coder;
- **C7** no Pareto claim without an arbiter run (twice, hash-identical) and this
  gate's R-2 classification. A decode-route DECODE-GO is not a crossing (I9-1
  R-3).
- **C8** the A1–A4 provenance ablation stays RETIRED for statistical transforms
  (exactness decides first); it is not required for arm (B) and would not
  rescue it. It remains live only for a revived EXACT-transform variant.

## P-5. Dispositions — one line per claim in the submission

| # | claim (as submitted) | disposition |
|---|---|---|
| P5-1 | "(B) is not an estimate of a noisy quantity; residual alphabet bounded and distance-independent (no log2(d) growth)" | **VERIFY the distinction** (DNB-M2's math does not transfer) / **REJECT as a G4 permission** (G4-amended requires ZERO residual) |
| P5-2 | "zero transmitted bits and an exact rule" | **CONDITIONAL** — pending the P-4 causality statement; look-ahead-derived `s` is transmitted information in disguise |
| P5-3 | "per-reference scoping sufficient separation from adaptive DPCM / xz --delta" | **REJECT** — placement separates from global filters only; lands in copy-with-edits + adaptive prediction |
| P5-4 | "confirm it may land as adopt/control-class capability for arch" | **VERIFY** — permitted under C1–C8, no novelty claim |
| P5-5 | "ablation A vs B; mechanism vs entropy coder; parameter-free second-difference control" | **VERIFY as required** (C6); add the exactness test as the prior question |
| P5-6 | "gates the pnra lane's synth-arith claim, TCOPY/PNRA family" | **RULED** — no novel claim for arm (B); exact-transform family positions (P-4 1–3) remain open with lineage + ablation |

## P-6. Prior-art lineage (required for any future pre-registration text)

Cite, once each, in this order: fractal/PIFS (lineage for discovered-transform
self-similarity; distinguished by losslessness + transform algebra + reference
structure — audit §2, verdict G2a); parameterized matching / predecessor
encoding (Baker; the honest limit — a single invariant is established);
parametric dictionary compression (forecloses transmitted parameters);
VCDIFF/bsdiff/Zdelta/Zucchini (copy-with-edits lineage); xz `--delta`
(documented `dist` 1..256; covers fixed record-period deltas, PART XIII §8);
Gorilla/FPC (delta-of-delta, competing baseline, no conflict); Brotli RFC 7932
§7 (context map: per-context coding is established — the per-column conditioning
in this family is an instance of it). Sources: `docs/gate-priorart-audit-i8.md`,
`docs/gate-verdict-i8-ari-stride.md`, `docs/priorart-tcopy-external.md` passes
1–5. Coverage gaps remain as recorded there (arXiv/ACM/IEEE not queried this
iteration; FTO not obtained).

## P-7. Measured verification of the pnra-stride prototype, and the decisive fact

Artifacts: `prototypes/i9-pnra/` (untracked worktree), `README.md` §3 commands,
`results-i9-pnra.csv`, `benchlog-parallel-window.txt`, `corridor-check-output.txt`.
This gate verified the shipped artifacts directly (read-only); it did not rerun
`stride_ref.exe` (peer-reported selftest/roundtrips are reproducible from §3).

**VERIFIED:**
- Wire sizes: `pack3/derived` = **12,936 B**, `pack3/transmitted` = **12,921 B**,
  `raw2/derived` = 16,134 B, `pack5/ddelta` = 19,817 B, `brotli/derived` =
  13,456 B, `literal-only` = 256,049 B.
- Roundtrip: every shipped `dec_*.bin` sha256 = `AF05309F0E1EEFB3…` = the input
  `synth-arith.bin` sha256 — byte-exact for all five arms.
- AUDIT-7 accounting: `44 + 32 + 40 + 13 + 16 + 12,791 = 12,936` — exact.
- Corridor arithmetic: `201,088 / 12,936 = 15.54x`; `106,368 / 12,936 = 8.22x`.
- Ablation separation: representation-only `raw2` 16,134 B is already inside the
  corridor (the mechanism, not the coder, carries the result); the
  parameter-free second-difference control `pack5/ddelta` 19,817 B is **worse**
  than the step form 12,936 B (so the step contributes — C6 satisfied);
  `brotli`-on-symbol-stream 13,456 B is competitive but larger.
- **Dual bar (I9-1 R-2 / PART XIII §5b):** 12,936 B is below BOTH the raw bar
  (87,013 B) and the transform-enabled bar (16,313 B), and below the
  brotli-on-derived control (13,456 B). This is the first synth-cell candidate
  in the project to clear both bars. It remains a **prototype wire, not an ANVIL
  row**; no frontier language until integration + two hash-identical arbiter
  runs + bench sign-off.

**Corrections (recorded):**
- **c1 — token misuse:** "literal-only ... DOMINATED + DEGENERATE" mixes the
  tokens. Under R-1 boundary case 3 a *dominated* row is DOMINATED; DEGENERATE
  is reserved for non-dominated rows. The literal control is **DOMINATED**
  (store-class ratio 1.000191); report it that way.
- **c2 — entropy floor understated:** `63,953 x log2(3) = 101,363.1 bits =
  12,670.4 B`, not `101,338 bits / 12,667.5 B` (a ~25-bit / ~2.9 B slip in a
  derived quantity). Correct: wire is floor + **265.6 B (+2.05%)**. Wire bytes
  and conclusions unaffected.
- **c3 — timing:** parallel-window, single-threaded prototype (dec CV 17.3%,
  session CPU median 37%/max 72%, arm CPU median 42%). Correctly labelled
  `measured (parallel window; prototype, not suite)`; per PR-4 it may inform
  ranking, never a crossing claim. bench's quiet-window re-run command is on
  offer; not yet run.

### The decisive fact, and what it does to PR-5

The **transmitted-step control is smaller** (12,921 B) than the
zero-transmitted-bit derived form (12,936 B) by 15 B (+0.12%). Therefore, on
this file:

1. The measured byte result does **not** require the zero-bit derived parameter.
   Per P-4/C6, when A >= B the derived step contributes nothing measurable; the
   mechanism reduces, for novelty purposes, to a **transmitted per-region
   delta/stride reference** — prior-art-shaped delta/stride coding (xz
   `--delta` / parametric-dictionary territory) with a parser-level region
   table. Classification: **engineering / adopt**.
2. Arm (B) is **unnecessary here** as well as failing the G4-amended exactness
   condition; it receives no novelty permission. It remains a legitimate
   implementation choice for a capability.
3. The step *representation* is what beats the parameter-free second-difference
   control (19,817 B) — real mechanism value — but "a step with a transmitted
   parameter" is prior art. The defensible positions remain P-4 items 1–3
   (exact invariant indexing, unification under one MDL parser, correction
   topology coding), none of which this prototype instantiates: its step is
   transmitted (or prefix-derived with bounded residual), **not** derived from
   the reference geometry with zero residual.
4. Adoption path (if `arch` integrates): default-off flag + byte-identity gate +
   suite row + two arbiter runs + PR-4 quiet window + dual bar reported.
   Label `{synthetic}`, `{engineering}`; never a crossing without I9-1 R-2.

| # | claim (measured supplement) | disposition |
|---|---|---|
| P5-7 | "transmitted 12,921 B < derived 12,936 B (+15 B)" | **VERIFY** — and it is decisive: the derived-arm claim is REJECTED as unnecessary as well as non-exact |
| P5-8 | "derived rule exact-from-prefix, bounded distance-independent residual, steady state {-1,0,1}" | **VERIFY as description** / **REJECT as G4 permission** (nonzero residual) |
| P5-9 | "no-coder arm 16,134 B already inside the corridor" | **VERIFY** |
| P5-10 | "AUDIT-7 charge exactly 44+32+40+13+16+12,791 = 12,936" | **VERIFY** |
| P5-11 | "residual floor 12,667.5 B" | **CORRECT to 12,670.4 B** (c2) |
| P5-12 | "literal-only ... DOMINATED + DEGENERATE" | **CORRECT to DOMINATED** (c1) |

---

*Ruled by `research-gate`. This is a pre-registration gate: it constrains what
may be claimed, not what may be built. Building arm (B) is permitted as
engineering under C1–C8; claiming mechanism-level novelty for it is not.*
