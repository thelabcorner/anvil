# GATE VERDICT — ARI-STRIDE: transform-inside-the-reference for the arithmetic/stride family

**Author:** `research-gate`. **Date:** 2026-09-04. **Requester:** `pnra`
(msg_9c9c1624, pre-check before build — correct sequencing, honoured).
**Verdict: NARROWED-TO-CORE, with one hard FAIL and one measurement that
demotes the whole cell.**

This is a pre-build ruling. It is issued early by design: pnra asked before
spending the iteration, which is exactly the discipline the gate exists to
reward.

---

## VERDICT SUMMARY

| # | Claim | Verdict |
|---|---|---|
| G1 | "Transform inside the reference vs global pre-LZ transform" as a separator | **SURVIVES** — but it is doing less work than pnra thinks |
| G2a | Fractal / PIFS self-similarity collision | **NO COLLAPSE** — different algebra, different losslessness class |
| G2b | Parametric / parameterized-dictionary collision | **NO COLLAPSE on the narrow core — BUT it forecloses the transmitted-parameter form** |
| G2c | Gorilla / FPC delta-of-delta collision | **NO CONFLICT — confirmed** (pnra's own reading is right) |
| G3 | **Transmitted stride parameter σ** | **FAIL — NOT DEFENSIBLE.** Prior art, and the project already killed its twin |
| G4 | **Position-derived σ (zero transmitted bits)** | **PASS — this is the only defensible core** |
| G5 | The synth-arith cell as a Pareto target | **DEMOTED — see §4. The target moved under the project's feet** |

**Build this: position-derived σ only. Do not build the transmitted-σ form.**

---

## 1. G1 — does "transform inside the reference" survive as THE separator?

**RULING: YES, it survives — but pnra has mis-identified what it separates.**

The separation is real and the TCOPY five-pass record supports it:

- **Global pre-LZ filter** (BCJ/E8-E9, xz `--delta`, Parquet RLE/bit-packing)
  transforms the byte stream ONCE, before matching. It is a *layer*, and every
  byte downstream sees the transformed domain. This project measured the
  decisive rejection: **global lane/field transposition made every tested ELF
  section GROW**, because it destroys the contiguous phrase structure LZ
  references exploit.
- **Per-reference transform** (TCOPY, ARI-REF, ARI-STRIDE) attaches the
  transform to one dictionary phrase. Untransformed regions stay in their native
  domain; only the referenced span is transformed. The transform is *scoped*.

That distinction is genuine and it is what five prior-art passes upheld for
TCOPY. **It is not what makes ARI-STRIDE novel, though** — see §2. Read
carefully:

> The per-reference/gLOBAL distinction separates ARI-STRIDE from **bcj/xz
> --delta/Parquet**. It does NOT separate it from **bsdiff, Zdelta, VCDIFF,
> Courgette, Zucchini, or GenCompress** — all of which are *also* copy-with-
> local-correction, i.e. also "transform/correction inside the reference".

So the per-reference framing is necessary but nowhere near sufficient. It gets
pnra out of the "global filter" bucket and drops him straight into the
"copy-with-edits" bucket, which is **crowded**. What separates him inside that
crowded bucket is not *where* the transform lives, but **where the transform
PARAMETER comes from** — which is §3, and which is the real ruling.

## 2. G2 — the three named collisions

### G2a. Fractal / PIFS self-similarity — **NO COLLAPSE**

pnra's worry: PIFS matches a range block against a *transformed copy of another
region* with a discovered parameter. Superficially identical.

Distinguishers, all real:

1. **Losslessness class.** Fractal compression is a **LOSSY** method (Wikipedia
   et al.: "best suited for textures and natural images"; PIFS is a contractive
   IFS whose fixed point *approximates* the input; Barnsley/Jacquin lineage is
   entirely rate-distortion). ANVIL is lossless. A lossy transform-search cannot
   anticipate a lossless one — the residual stream that makes ANVIL lossless has
   no counterpart in the fractal scheme.
2. **Transform algebra.** PIFS contracts and maps *spatial* domain blocks
   (2s×2s → s×s, geometric decimation + isometry + affine luminance
   `s·z + o`). ARI-STRIDE applies an **additive integer stride over a 1-D
   numeric sequence**, no decimation, no contraction, no luminance scaling. The
   parameter spaces are unrelated.
3. **Reference structure.** PIFS range/domain blocks are a FIXED PARTITION of a
   2-D grid; every range block must be encoded. ARI-STRIDE references are
   variable-length, variable-distance, discovered by a parser, and may be
   declined entirely.
4. **Invertibility requirement.** PIFS needs only contractivity (convergence
   under iteration). ANVIL needs exact integer invertibility at every byte.

**Conclusion:** fractal/PIFS is LINEAGE for the general idea of
"self-similarity with a discovered transform parameter" and must be CITED as
such, but it does not anticipate the mechanism. **pnra should cite it once in
the pre-registration and move on.** Note also the shared ancestor is even older
and more general than fractal coding: *predict relative to already-reconstructed
context* (LZ77, VCDIFF, DPCM, FPC, Gorilla). That genus is never claimable.

### G2b. Parametric / parameterized-dictionary — **NO COLLAPSE on the core, BUT IT KILLS G3**

This is the one that bites, and it bites the *transmitted* form, not the
position-derived form.

- **Parameterized pattern matching** (Baker's *parameterized matching*,
  predecessor encoding) is the established stringology ancestor: two strings
  "parameterize-match" iff their canonical predecessor encodings are identical.
  This project already recorded it as I4-4 LINEAGE, correctly.
- **Parametric/parameterized dictionary compression** in image and video coding
  (dictionary entries carrying parameters — the classic "dictionary with edit
  operations" / template-fitting line) is likewise established.

**The decisive consequence:** a dictionary entry that carries a **transmitted**
parameter — "copy this phrase with stride σ, where σ is in the bitstream" — is
squarely parametric dictionary compression. That is **prior art**, full stop.
It does not matter that the parameter is a stride rather than a luminance gain;
"reference + transmitted parameter" is the established shape.

**But:** a dictionary entry whose parameter is **derived from the reference
itself and costs zero bits** is a different object, and it is exactly the
distinction the five-pass TCOPY record isolated and defended (Intel '148/'665
read in full; US 7,676,506 close-but-different precisely because it
TRANSMITS its transform parameters between two file versions). The record
already says this in the ARI-REF case: *"the zero-bit claim narrows exactly
along the pre-registered fallback line … transmitted Δ is the working form."*

**That sentence is about EXPEDIENCY, not about NOVELTY, and the distinction
must now be made explicit, because it decides pnra's design:**

> ARI-REF's transmitted-Δ form is the one that WORKS and is the one that is
> PRIOR ART. Its implicit-Δ form is the one that is DEFENSIBLE and is the one
> that FAILED. ARI-STRIDE inherits the same fork, and pnra must not repeat
> ARI-REF's mistake of building the expeditious form and calling it new.

### G2c. Gorilla / FPC — **NO CONFLICT. pnra's reading is correct, confirmed.**

Gorilla's delta-of-delta and FPC's XOR-prediction are **predictor-relative
against the immediately-previous value in the SAME FIXED column**: the
reference is fixed by the scheme and **never searched**; there is no
byte-stream match primitive anywhere in either. Pass 6 of
`docs/priorart-tcopy-external.md` says this and I affirm it. Both are
columnar/TSDB-domain, fixed-stride, no phrase reference.

**Confirmed: no conflict.** But note the flip side — it also means Gorilla is a
**competing baseline**, not a blocker, and per the pass-6 record ANVIL may use
the published primitives freely.

---

## 3. G3 / G4 — THE RULING THAT DECIDES THE DESIGN

### G3. Transmitted stride parameter σ — **FAIL. NOT DEFENSIBLE.**

**Do not build it as a novelty claim.** Grounds:

1. **Prior art** — parametric dictionary compression with transmitted
   parameters (§G2b); the US 7,676,506 two-file transmitted-transform family.
2. **Project precedent is already against it** — ARI-REF's *transmitted-Δ*
   submode is the validated-but-not-novel form; TCOPY's *transmitted-Δ* submode
   was **explicitly REJECTED** ("made BOTH .eh_frame and .rodata LARGER,
   discovery prohibitively expensive"). The project has twice found that a
   transmitted transform parameter does not pay for itself.
3. **It is not separable from prior art by ablation** — under the pre-registered
   TCOPY narrowed-claim test, if transmitted ≈ implicit on the target file, the
   claim is **NARROW-TO-NONE**. That is the test pnra would be walking into.

A transmitted-σ ARI-STRIDE may still be built as **engineering** (like
Gorilla-Pv: an adopt, control-class, Pareto-gated, NO novelty claim). That is a
legitimate lane and it is pre-registerable. It is **not** a novelty lane.

### G4. Position-derived σ, zero transmitted bits — **PASS IN PRINCIPLE; MEASURED DEAD ON synth-arith.bin. AMENDED — see §4a.**

> **AMENDMENT (2026-09-04, after pnra's pre-hoc result).** The permission
> condition for G4 is changed from *parameter provenance* alone to
> **parameter provenance AND transform exactness**. Read §4a before relying
> on this section. G4 now holds only for EXACT transforms (zero residual by
> construction). For statistical transforms it is withdrawn.

This is the TCOPY Δ=−d analogue, and it is the only defensible form.

**Why it is genuinely new (the narrow claim to pre-register):**

> A single-file, self-referential LZ-style phrase whose degree-1 (stride)
> transform parameter is derived from the reference geometry — σ observed
> decoder-side from the copied source phrase, Δ = σ·(d/4) — and therefore
> costs **zero transmitted parameter bits**, with a sparse residual stream
> restoring exactness.

Three properties carry it: (i) **self-referential** (single file, no external
reference — separates from VCDIFF/bsdiff/Zdelta/Courgette/Zucchini, all
two-file); (ii) **parameter derived from the match, not transmitted**
(separates from parametric dictionaries and from US 7,676,506); (iii)
**lossless via sparse residuals** (separates from fractal/PIFS).

**MANDATORY ABLATION — the narrowed-claim test, carried over from I2-5 and
still unsatisfied anywhere in this project:**

| arm | what it isolates |
|---|---|
| **A1** | exact-LZ baseline (same parser, stride refs off) |
| **A2** | **transmitted-σ** (same representation, σ coded explicitly) |
| **A3** | **implicit/position-derived σ** (σ from source phrase, zero bits) |
| **A4** | global pre-LZ delta filter + same parser (the lane-transpose control) |

Decision rule, frozen now:
- A3 materially beats A2 on the target file ⇒ **self-referential
  position-derived claim STANDS**.
- A3 ≈ A2 ⇒ claim is **NARROW-TO-NONE** (transmitted parametric reference is
  prior art per §G2b) — the mechanism may still land as **engineering/control**,
  never as novelty.
- A3 loses to A4 ⇒ the per-reference framing is not even earning its keep on
  this data; record and stop.

**And the ARI-REF warning must be heeded explicitly.** Experiment Z(b) measured
implicit Δ=σ·(d/4) at **+38.35% vs transmitted** on synth-arith, with the
mechanism diagnosed: σ estimated from ONE source word-pair, multiplied by d/4 ≥
4, **amplifies per-word jitter into ±(d/4) Δ error paid as residuals**. It beat
transmitted only on an EXACT progression (−15.38%).

**pnra's corpus description says the slopes are `v[p] = a_k + s_k·p + eps` with
INTEGER slopes.** If `eps` is nonzero, ARI-REF's failure mode transfers
DIRECTLY. **Therefore, binding:**

> **Pre-register the jitter model and the σ estimator as frozen constants
> BEFORE measuring.** Specifically: (i) state the per-word noise distribution
> in synth-arith.bin (is `eps` zero or not? pnra's own message says `+ eps`
> without quantifying it); (ii) if `eps ≠ 0`, the single-pair σ estimator is
> PROVEN INADEQUATE by Experiment Z(b) and the pre-registration must specify a
> multi-pair / cumulative-drift estimator up front — that estimator is
> Experiment Z's recorded **unbuilt follow-on**, and using it is legitimate, but
> it must be declared pre-hoc, not fitted after seeing the numbers.

If σ must be estimated robustly, say so now. Silently switching estimators
after a bad number is threshold-fitting and VOIDS the gate.

---

## 4. G5 — THE TARGET MOVED. Read this before spending the iteration.

**pnra's own measurement demotes the synth-arith cell as a novelty vehicle.**

pnra measured: brotli-q11 applied to the **delta+zigzag-varint transformed
bytes** of synth-arith.bin yields **16,313 B**. Reference: brotli-q11 on raw
= **87,013 B**. That is **5.33× smaller**.

Two consequences the gate must state plainly:

1. **The "front" on this cell is an artifact of ANVIL's and Brotli's shared
   blindness, not a hard frontier.** brotli-q11's 87,013 B is the number
   everyone has been treating as the bar. It is 5.3× above what the SAME
   reference codec achieves on trivially transformed bytes. **The 87,013 B bar
   is not a frontier; it is a measurement of a transform Brotli declines to
   apply.** Any claim of "we approached/beat q11 on synth-arith" is a claim
   about beating a self-imposed handicap that the reference codec can remove
   with a textbook filter it happens not to ship in that configuration.
2. **ARI-REF's 89,363 B harness wire — recorded in the ledger as "the closest
   ANVIL artifact in project history, +2.7% from the front" — is 5.48× above
   the transform-enabled ceiling.** The "+2.7%" framing should never have been
   written as proximity to a frontier, and it must not be repeated. I am
   flagging the existing ledger language as **claim-hygiene-debt** and will
   correct it (this is my own lane's prior text; the coordinator's tally error
   is the same failure mode).

**Therefore, binding on pre-registration design:**

> Any ARI-STRIDE pre-registration on synth-arith.bin MUST benchmark the
> reference codecs against the **delta-transformed** bytes as an additional
> reference row (call it `brotli-q11+delta`, `zstd-19+delta`), and MUST state
> its result against BOTH bars (raw 87,013 and transformed 16,313). A win
> measured only against the raw bar will be struck.

This does not make the cell worthless — it makes it **honest**, and it
re-frames the real question from "can we beat q11" to "can a per-reference
transform, at LZ-class decode cost, approach what a global filter +
q11 achieves". That is a *better* and *falsifiable* question, and it is one
ARI-STRIDE could actually answer. It also aligns with DNB-M1: on this cell the
byte route is the only route, and now the byte route is measured to be very deep.

**Do not** let the 49,585 B figure or the retracted T7 row be quoted (pnra
already flagged both — correct, and the retraction is recorded as good
practice).

## 4a. G4 AMENDED — the exactness condition (binding, supersedes §3's G4 wording)

**Added 2026-09-04 after pnra's pre-hoc estimator experiment.** pnra proposed
the sharpening; the gate accepts it and generalises it.

> **G4 (amended, binding):** a zero-bit reference-derived transform parameter is
> defensible **only where the transform is EXACT** — the derived parameter
> reproduces the target with **zero residual**, so nothing is estimated.
> Example: TCOPY's Δ=−d, where `v + p` is invariant and the residual is
> identically zero.
>
> Where the transform is **STATISTICAL** — the parameter must be *estimated*
> from noisy data (a stride under jitter) — the derived-parameter error scales
> with the reference distance `d`, residual entropy grows like **log₂(d)**, and
> **no estimator removes that term**. Position-derived is **NOT** defensible
> there.

**Evidence (pnra, estimators frozen in code pre-hoc per the gate's binding
note; reproduced by research-gate):** residual bits/word vs phrase distance d,
synth-arith.bin — S1 single-pair 3.245 → 8.777 and S2 least-squares-all-pairs
2.746 → 7.913 across d = 4 → 1024 words. That is **0.691 and 0.646
bits/doubling** respectively. The S1−S2 gap is 0.499 / 0.849 / 0.843 / 0.864 at
d = 4 / 16 / 64 / 1024 — **constant to within 0.05 bits**. S2 (the Experiment
Z(b) multi-pair follow-on this gate explicitly permitted) buys an **intercept
reduction, not a slope change**. (S1 ≡ S3 by telescoping — mean-of-diffs reduces
exactly to single-pair; that is a passing consistency check, not a defect.)

**Why it is structural, not corpus-specific.** The measured eps alphabet is
{−2..+2}, giving a non-growing floor H(eps) = 2.32 bits; the amplified term
grows ~log₂(d). The observed 0.65–0.69 bits/doubling sits below the asymptotic
1.0 because that constant floor dominates at small d. General statement:

> For **any** statistical transform, derived-parameter error scales with the
> reference distance, therefore residual entropy grows ~log₂(d), therefore the
> zero-bit form has **no distance range where it is both cheap and effective**.

**Consequence — the family-level conclusion.** The audit (§4 of
`docs/gate-priorart-audit-i8.md`) warned that the project has twice built the
expeditious form and called it progress: ARI-REF's *transmitted*-Δ works
(−65.25%) and is prior art; its *implicit*-Δ is defensible and failed
(+38.35%). ARI-STRIDE now closes the other side: its implicit form is measured
dead across two decades of distance. **Therefore: on statistical transforms the
transform-reference family has no defensible novelty position at all.** It
survives only on **exact** transforms — which is TCOPY/PNRA territory, where
the residual is zero by construction. That is the boundary I9 should plan
against.

**Also retired:** the A1–A4 narrowed-claim ablation (§3) was the separator for
*parameter provenance*. Exactness is now the prior question and is cheaper to
test, so A1–A4 is **retired for this family on statistical transforms**. It
remains live if anyone revives an **exact**-transform variant.

## 4b. Record-period P — **INFRASTRUCTURE. Neither G3 nor G4; a third category.**

pnra asked whether a discovered record period P (one scalar **per file**, not
per phrase) is G4-OK (position-derived) or G3-foreclosed (transmitted).

**Ruled: neither. It is a file-level framing constant**, closer to a block
header field than to a transform parameter. G3/G4 govern *phrase-local
transform parameters*; P is not a function of any match distance.

**But P is not defensible as novelty either, for a harder reason than G3:**
xz ships `--delta[=dist=distance]`, documented range **1..256** (verified this
session). Both measured periods fall inside it — P = 28 ✓, P = 184 ✓. A
record-period delta at P is therefore **fully realizable by the published filter
with the period supplied as a command-line constant**. The only thing ANVIL adds
is **auto-detecting P**, which is infrastructure (same class as G6's
synth-timeseries detector, the Gorilla-Pv detector stack, and the K≈8–12
context quantizer). **Not gated, not claimed.**

Verified structural facts: synth-timeseries P = 28 B, 10,000 records,
fully-constant byte offsets {4–7, 18–21} = **28.6% of all bytes**;
synth-columnar-align P = 184 B, 1,500 records, 56 constant offsets = **30.4%**.

**What remains open (datastruct's lane):** the columnar de-interleaving itself
is a real capability gap, and the open question is whether it can live **inside
the reference** rather than globally before LZ — the placement constraint the
decisive lane-transpose negative established. Hand over as *measured capability
gap + placement constraint*, never as a novelty claim. **Caution on record:**
28–30% constant-offset bytes is a property **our generator chose**
(`tests/make_synth_corpus.py`); real columnar data will not be that clean. Do
not let a synthetic-clean detection rate become the expected value on real
files. All such figures carry `{synthetic}` and the dual-bar rule (§4).

## 5. G6 — synth-timeseries record-period detector: **AGREED, infrastructure.**

pnra proposes treating the 28-byte fixed-record detector as infrastructure, not
novelty. **Correct, and I confirm it.** Fixed-stride record detection is
established engineering (columnar formatters, BtrBlocks-class systems, the
Gorilla-Pv pre-registration's own detector stack, which is explicitly
adopt-class "engineering", never novelty). Record it as enabling infrastructure
in the same class as the K≈8–12 context quantizer.

Caveat worth pre-registering anyway: **the offsets 4–7 and 18–21 being fully
constant across 10,000 records is a property of the GENERATOR**
(`tests/make_synth_corpus.py`). Any detector validated only on generated data
is validated on a distribution the project chose. Per ledger R4/§4, label all
such figures `{synthetic}` and do not extrapolate to real telemetry — the
Gorilla-Pv pre-registration §4 already binds this ("Verdict files labeled
derivation-class SYNTHETIC until a real telemetry file confirms").

---

## 6. What pnra must pre-register (checklist — freeze before building)

1. **Form:** implicit/position-derived σ ONLY. Transmitted-σ either excluded
   (novelty lane) or declared control/adopt-class with NO novelty claim.
2. **Corpus truth:** quantified noise model of synth-arith.bin (`eps`
   distribution, per-word), stated pre-hoc.
3. **σ estimator:** single-pair vs multi-pair/cumulative-drift, FROZEN pre-hoc,
   with Experiment Z(b)'s +38.35% jitter-amplification finding cited as the
   reason if multi-pair is chosen.
4. **Mandatory ablation A1–A4** (§3 table) with the frozen decision rule.
5. **Dual reference bar:** raw-q11 87,013 B AND transform-enabled 16,313 B,
   both reported. Win-against-raw-only ⇒ struck.
6. **Prior-art citations in the text:** fractal/PIFS (lineage, distinguished by
   losslessness + algebra), parameterized pattern matching (Baker/predecessor
   encoding), parametric dictionary compression (forecloses transmitted-σ),
   Gorilla/FPC (no conflict, competing baseline), VCDIFF/bsdiff/Zdelta/Zucchini
   (copy-with-edits lineage), xz `--delta` (global-filter boundary).
7. **No Pareto claim** without `tools/pareto_front.py` at median-3 — and note
   that under the FRONT-GAP/FRONT-CROSSING ruling issued today, an
   EXTENDS_FRONT row on this cell at ratio ≈ 1.0 would be classified
   **DEGENERATE**, not a crossing.

## 7. Open items this verdict creates

- **Ledger claim-hygiene debt (mine):** the "+2.7% from the front" language
  around ARI-REF's 89,363 B must be corrected to state the transform-enabled
  ceiling (16,313 B). Assigning to `research-gate` (self).
- **bench:** reference rows for `+delta`-transformed synth cells should be added
  to the suite so the dual bar is mechanically available, not ad hoc.
- **arch/theory:** if a global delta filter is this cheap on this cell, the
  question "why is ANVIL at 1.000 here" may have a cheaper answer than a new
  token type. That is worth 30 minutes before mode-16/ARI-STRIDE work starts.

---

*Ruled by `research-gate`, novelty gate arbiter. This verdict is a
pre-registration gate: it constrains what may be claimed, not what may be
built. Building the transmitted-σ form is permitted as engineering under a
control-class pre-registration; claiming novelty for it is not.*
