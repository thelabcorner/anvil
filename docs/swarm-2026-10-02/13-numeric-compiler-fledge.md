# Track 13 — Numeric / Floating Representation Compiler — Fledge Alpha Free

**Role:** independent adversarial reviewer (`Fledge Alpha Free`). Constructive lane
for this track was **not present in the workspace at the time of writing**; see
§0.2. I reconstructed the evidence independently from repository artifacts and
did **not** read any other track-13 report.

**Status: RECONCILED.** §1–11 were frozen as INTERIM and are **unchanged**.
`13-numeric-compiler-space-bunny.md` (50,778 B) has since been read in full and
reconciled in **§12**: a 13-row disagreement table, the three mandated questions
answered, the mandatory corpus-lock specification, eight required control
upgrades, an explicit record of where the constructive lane is *better* than my
interim, and the post-reconciliation verdict. No §1–11 text was edited to fit the
reconciliation. Final verdict: **HOLD** (§12.8, §13.1).

**Authoring constraints honoured:** no existing file modified; no commit/push;
no reset/clean/stash/restore/rebase; no local corpus or performance benchmark;
no heavy fuzzing. Exactly one deliverable file created. One optional isolated
arithmetic proof lives at
`prototypes/swarm-2026-10-02/13-numeric-compiler/fledge/lane_overhead.py` — it is
pure closed-form integer arithmetic on constants read out of the repository. It
compresses nothing and benchmarks nothing.

**Compute policy:** all proposed measurement is GitHub-Actions-only.

---

## 0. Mandate, method, and what this document is not

### 0.1 What I was asked to attack

Red-team a "reversible typed lanes" numeric/floating representation compiler
against Gorilla, FPC, ALP, Parquet, ORC, FastLanes, BtrBlocks, LeCo, OpenZL,
white-box compression and neighbours; find what (if anything) survives; define
dual-bar controls; close with a frozen verdict.

### 0.2 Evidence actually used (all read by me, this session)

| # | Artifact | Load-bearing role here |
|---|---|---|
| E1 | `docs/swarm-2026-10-02/MASTER-BRIEF.md` | binding doctrine; doctrine items 3, 5, 7, 10, 11, 13 |
| E2 | `RESEARCH_LEDGER.md:4430-4459` | xz `--delta` covers record periods; auto-detect = infrastructure |
| E3 | `RESEARCH_LEDGER.md:4598-4652` | datastruct-ts **NOVELTY: NO**; dual-bar transform-enabled references; P=28→14 alias correction |
| E4 | `RESEARCH_LEDGER.md:4810-4833` | **PORDER killed as standalone novelty**; held-out numeric family unavailable; CI-only compute |
| E5 | `docs/I10-GROTLI-G2-TYPED-PREREG.md` | frozen typed-leaf experiment; float/Gorilla/ALP **explicitly excluded** (§4); 20%-class gates |
| E6 | `docs/I10-GROTLI-G2-RESULTS.md` | measured NO-GO-G2; integer FOR secondary; DELTA/DoD selected 0× |
| E7 | `docs/verify-notes/gorilla-verification-plan.md` | F-G1 generator-dependence; DoD tier table; B2 bloat claim |
| E8 | `prototypes/cost_oracle/GORILLA_COST_PREREG.md` | cost model J, guards, ε gates, header ≤0.4% cap |
| E9 | `prototypes/profile_tmp/REPORT.md:100-124` | measured ns/value and b/value for Gorilla/vXOR/DoD; implementation traps |
| E10 | `docs/audit-2026-09-07/10-specialist-disagreement.md:49-77,128-141` | sao headroom 160,422 B = 24.2% of gap; M2 = "MEDIUM, small… stride/delta probe, not a new backend" |
| E11 | `docs/audit-2026-09-07/06-do-not-reburn.md` | A2, E1, E3, E5, F1 fund/stop bar, G1 block routing negative |
| E12 | `docs/gate-ruling-i9-datastruct-ts.md:137-194` | novelty NO; prior-art lineage list; dual-bar qualifier |
| E13 | `docs/gate-verdict-i8-ari-stride.md:125-165` | G3 transmitted-parameter NOT DEFENSIBLE; G2c Gorilla/FPC no conflict |
| E14 | `docs/FRONTIER-RESET-2026-09-23.md:498-544` | ALP/FastLanes/Parquet-ALP adoption; "hardware-friendly representation may reveal a better model" |
| E15 | `tests/corpus/README.md:110-175` | synthetic telemetry/NDJSON specs; `synth-telemetry-f64.bin` 0.8880 ratio |
| E16 | `tests/benchmark-recon-i9.md:58-70` | FRONT-GAP dual-bar rows; two corpus cells never recon'd |
| E17 | filesystem survey: `tests/corpus/*`, `tools/*`, `.github/workflows/*` | **no numeric lane exists in ANVIL; no numeric workflow exists** |

**Not used as evidence:** any other track-13 output; any Space Bunny report for
this track (none existed); any model recollection of Gorilla/ALP numbers not
pinned to a repository artifact. Where I use an external system's published
*existence* (ALP, FastLanes, LeCo, OpenZL, Parquet-ALP), that is a **prior-art
citation**, tagged as such, never as a measurement.

### 0.3 Derivation classes used throughout

`measured-exact` (read from a repository artifact that records the measurement),
`measured-approx` (measured on a non-canonical window/host, carried with its
qualifier), `modelled` (formula/estimate, no measurement behind it),
`derived` (my own closed-form arithmetic over the above, reproducible via the
proof script), `hypothesis` (untested assertion).

---

## 1. Evidence and provenance audit

### 1.1 The numbers a track-13 proposal will lean on, with their true class

| Claim you will see in circulation | Class | Provenance | Adversarial note |
|---|---|---|---|
| `synth-telemetry-f64.bin` compresses to 0.8880 with ANVIL defaults | measured-exact (orientation only) | E15:171 | Explicitly labelled "round-trip orientation only (NOT a benchmark row)". A pilot that quotes it as a ratio row violates E16's rule. |
| Gorilla Case-A-dominant: 2.88 b/val, 2.91 ns/val (2.75 GB/s) | measured-exact | E9:103 | Favourable regime is *synthetic constant-cadence*. |
| Gorilla smooth-drift (B1-heavy): 54.15 b/val, 7.68 ns/val | measured-exact | E9:104 | **54 b/val is 6.75× worse per value than Case A.** "Gorilla is fast" and "Gorilla is small" are different regimes and are not jointly available on the same data. |
| Random control: 37.79 b/val, **plus bloat** | measured-exact | E9:105 | Gorilla *inflates* incompressible input. Any lane without an ε gate is an amplification bug, not a codec. |
| vXOR 0.146 ns/B (6.86 GB/s), zero-density 59.3% | measured-exact | E9:107-109 | Byte-wise loop was **5× slower**; 8-byte chunking is mandatory to vectorise. |
| DoD 8.41–8.44 b/val, 4.14–5.90 ns/val | measured-exact | E9:111 | Branchy tier select. |
| Router-J per output byte: Gorilla A-dom ~0.36; mixed 0.85–0.96; Pv ~0.15; DoD 0.52–0.74 | measured-approx | E9:123-124 | `mixed` costs **2.4–2.7×** the Case-A constant. Any "one J constant for Gorilla" claim is false. |
| B2 bloat on mb≈60: "(13+60)/64 − 1 = **+14.06%**" | **provenance defect — see §1.2.1** | E7:18 | Do not reuse the percentage. Use the exact ±11-bit statement. |
| DoD zero-rate 99.8% / 1092 bits / "98.3% reduction" | **modelled-capacity, generator-dependent** | E7:21-39 (Finding F-G1) | Independent simulation: quantised 1 s jitter σ=0.3 → 75.9% zeros / 3004 bits / 95.3%; unquantised f64 → **0.0% zeros** / 9060 bits / 85.8%; constant cadence → ~100% / ~1077 bits / ~98.3%. **The headline is a property of a fixture generator, not of Gorilla.** |
| DoD tiers 0→1b, [−63,64]→9b, [−255,256]→12b, [−2047,2048]→16b, else 36b; Δ₁ 14-bit | measured-exact (verified against `grotli.ts:572-579`) | E7:14 | Pinned to an **external JS project**, not to a standard. Treat as one published variant. |
| G2 selected typed leaves: EXACT_DICT dominant; INT_FOR secondary; **DELTA/DoD selected 0× on D2 and 0× on D4** | measured-exact | E6:194-212, 249-266 | The single most decision-relevant fact for this track (§4.4). |
| G2 integer-only vs G1R on D2: **+0.2701% worse** | measured-exact | E6:190 | Numeric coordinate transforms *lost to an all-raw column carrier* on real data. |
| G2 carrier tax on D2: G1R 25,916 vs historical G1 25,654 | measured-exact | E6:180 | **+1.02% carrier overhead** just for adding per-leaf IDs and payload lengths, before any leaf is chosen. |
| G2 local Brotli candidate scoring: D1 510 ms, D2 784 ms, D4 2.68 s; marginal search 6.80 s / 75.10 s / 121.28 s | measured-approx (CI) | E6:354-372 | The only measured typed-leaf planner cost we have. |
| P-MARGINAL lost to dense local composition: D2 24,865 vs P-DICT 20,903; D4 86,505 vs P-DICT 84,375 | measured-exact | E6:302-322 | Planner-class precedent, adverse. |
| `ts.ref_field` 89,877 B is **above** `xz -9e --delta=dist=14` 89,564 B and `brotli q11 on delta14` 80,650 B → FRONT-GAP (dual-bar) | measured-exact | E3:4621-4628, E12:186-194 | **The project's canonical demonstration that beating your own raw control is worth nothing.** |
| sao headroom = 160,422 B = **24.2%** of the 664,307-B gap; mozilla = 429,893 B = 64.7% | measured-exact | E10:73-77 | |
| Float/unsortable numeric classified **"MEDIUM, small"**, prescribed response = "a stride/delta probe (P4.2) rather than a new backend" | measured-exact (own audit) | E10:135-140 | The project's own prior verdict on this exact family, already on file. |
| xz `--delta[=dist=d]` documented range **1..256**; periods 14 and 23 both inside it | measured-exact | E2:4436-4443, E3:4632-4642 | Auto-detection of P = **infrastructure**, same class as the Gorilla-Pv detector stack. |
| Record periods are **P=14 and P=23**, not 28 and 184; constant offsets 28.6% / **21.7%** (not 30.4%) | measured-exact, supersedes prior | E3:4632-4642 | An **8× decimation alias** inflated the earlier "56/184 = 30.4%" figure. |
| PORDER killed as standalone graph/layout novelty; FastLanes, white-box, OpenZL, Corra, LeCo, BtrBlocks, ALP already cover the broad interaction | measured-exact (ledger ruling) | E4:4824-4829 | Directly disposes of "typed lanes = relation graph + width-aware schedule". |
| "No new held-out structured corpus is currently available… Promotion requires new locked independent … **numeric** families." | measured-exact | E4:4831-4833 | **Track 13 has no admissible corpus today.** |
| G5D prototype "still emits a zero decode-RSS placeholder and hard-codes several invariant flags" | measured-exact | E4:4837-4840 | The project's own decode-RSS instrumentation is **not closed**. Any track-13 RSS claim inherits an unclosed instrument. |
| No float/Gorilla/ALP code in `tools/`; no numeric workflow in `.github/workflows/` | measured-exact (filesystem survey) | E17 | Track 13 is greenfield. That is a cost, not a moat. |

### 1.2 Provenance defects a constructive report is most likely to inherit

#### 1.2.1 The "+14.06% B2 bloat" figure has an unexplained denominator

`docs/verify-notes/gorilla-verification-plan.md:18` records
`(13+60)/64 − 1 = +14.06%` as "✅ EXACT". The wire shape is exact
(`gorilla.ts`: Case B1 = `2+mb` bits, Case B2 = `13+mb` bits, per E7:15). But the
denominator `64` is neither the B1 cost (`2+mb`) nor the payload width (`mb`):

- vs the **B1 wire cost** the relative overhead is `11/(2+mb)` = **+17.74%** at
  mb=60, and **+78.6%** at mb=12;
- vs a **64-bit container** (the plausible intended reading, if a block is
  materialised in one 64-bit word) it is `(13+mb)/64 − 1` = +14.06% at mb=60, and
  **+13/64 = +20.3%** at mb=51, exceeding the container at mb>51.

Both readings are legitimate; they are not the same claim. **Rule for track 13:
quote the exact invariant ("B2 costs exactly 11 more bits than B1 for identical
payload") and never the percentage.** If a proposal needs the container reading,
it must first show that its container is 64 bits and state what happens for
`mb > 51`.

#### 1.2.2 "Gorilla wins on floats" is a fixture artefact, not a property

Finding F-G1 (E7:21-39) is the most dangerous number in this neighbourhood. The
same σ=0.3 s jitter assumption yields **75.9% DoD zeros when timestamps are
quantised to 1 s and 0.0% when they are not**. Real telemetry timestamp
resolution is the entire mechanism. Any track-13 report quoting a DoD
reduction percentage without pinning (distribution, quantisation, seed) is
reporting a generator, not a codec, and is `modelled-capacity` under project
rules. Reproduce F-G1's table verbatim or do not quote the claim.

#### 1.2.3 Synthetic-cleanliness must not survive into expected value

`RESEARCH_LEDGER.md:4455-4459` (E2) warns that 28–30% constant-offset bytes is a
property our generator chose, and mandates `{synthetic}` + the dual-bar rule. The
subsequent alias correction (E3:4636-4642) shows how badly synthetic structure
can be over-read: the "30.4%" figure was an **8× decimation artifact**. The two
Gorilla fixtures have the same shape of risk: they were generated specifically to
be the sweet spot ("Added per the coordinator's … briefing so the proposed
auto-detect mechanisms have a fair substrate", E15:115-120).

#### 1.2.4 "96.3% column-equal density" is not a general expectation

E15:144-146 reports 96.3% column-equal density for `synth-ndjson-columnar.ndjson`
and notes it sits above grotli's own "93.2% zero-density honest-win zone". This is
a *deliberately favourable* fixture for vertical-XOR. It cannot be used as a
prevalence prior for real NDJSON.

---

## 2. Prior-art map — component-by-component occupancy

Every constituent of a "reversible typed lane compiler" is occupied. The right
question is not "is each piece novel" (none is) but "is the *composition* novel"
(also no, at the level where it would matter).

| Mechanism element | Occupant | Consequence for track 13 |
|---|---|---|
| XOR of consecutive f64 with leading/trailing-zero block reuse | **Gorilla** (Pelkonen et al., VLDB 2015) | Not claimable. Project already concedes this (`prototypes/i8-theory/INVARIANTS.md:236-238`: "Anything in this family on float columns is Gorilla, not ANVIL novelty"). |
| Delta-of-delta timestamps, 5-tier ladder | **Gorilla** | Frozen tier boundaries are decoder-visible wire; changing them is a format-lane matter (E8 §2c). |
| Bit-packed XOR prediction of doubles | **FPC** (Abridio et al.) | Not claimable. |
| Exact low-cardinality token dictionary | **G2 L1 EXACT_DICT** — and it is the **measured winner** (E6:194-212) | A numeric proposal that omits a dictionary arm is omitting the arm that won. |
| Frame-of-reference, bitpacking | **G2 L2/L3/L4**, Parquet, PFor | Measured secondary at best (E6:249-266). |
| Per-column int delta / delta-of-delta | **xz `--delta`**, Gorilla, FPC | Transmitted-stride form ruled **NOT DEFENSIBLE** (E13 G3). |
| Decimal factor ladder + exponent/mantissa split + patched outliers | **ALP** (crotra/Azadkia et al.), adopted by **Apache Parquet Sept 2026** (E14) | This is *the* prior art for "typed numeric lane that is fast *and* small". A lane compiler that discovers a decimal factor, splits exponent/mantissa, and patches outliers is ALP. |
| Composable expression encodings over lanes | **FastLanes** | Kills "expression DAG over lanes". |
| Lane-oriented layouts + expression encodings for compression | **FastLanes**, **LeCo**, **BtrBlocks**, **DataCortex** | Occupies the composition, not just the parts. |
| Bit-plane / row-wise LUT transform families | **white-box compression** line (JPEG-XS / CALLetc) | Occupied (E4:4826). |
| Structured-format framework with pluggable columnar engines | **OpenZL** | Occupied. |
| Composable relation-graph + width-aware schedule | **PORDER** — **already KILLED** (E4:4824-4829) | **This is the fatal overlap.** A "typed-lane compiler" that picks, per lane, a type tag and a width/expression schedule *is* PORDER's interaction. The ledger retained only a frozen 2×2 probe (relation-graph × width-aware schedule) requiring ≥1% complete-byte beat over *both* single-factor arms **and** metadata/codebook ≤20% of gross savings. Track 13 must meet that bar or it is a relabelled closed lane. |
| Context-mapped literal models | **Brotli RFC 7932 §7** | Already conceded twice in-project (E3, E12). |
| Row-copy + per-column residual contexts | **datastruct-ts** — **NOVELTY: NO** (E3:4609, E12:139) | A numeric lane emitted *inside* a reference lands in copy-with-edits. |
| Global lane transposition | **measured decisive negative**; killed (doctrine 13) | A lane-compiler that de-interleaves globally inherits this. |
| Transmitted transform parameter | **ARI-REF transmitted-Δ = validated but prior art; TCOPY transmitted-Δ = REJECTED** (E13:151-157) | Two prior project negatives in the same genus. |

### 2.1 Ruling on novelty

> **Every component is published prior art. The composition that matters is also
> published (ALP + FastLanes + BtrBlocks/LeCo + Parquet adoption + white-box
> bit-plane families). The specific interaction "relation graph over typed lanes
> with a width-aware schedule" was already adjudicated and KILLED as PORDER
> (`RESEARCH_LEDGER.md:4824-4829`).**

Therefore: **no mechanism-level novelty claim is available at the typed-lane
compiler level.** Any report that offers one is either relabelling PORDER or
relabelling ALP. That is a prior-art ruling, not a judgement about execution
quality.

---

## 3. The falsifiable hypothesis I would require before any build

Restated so it can actually fail:

> **H13.** On a *real* (non-synthetic) numeric binary population, there exists a
> reversible typed-lane representation whose **complete** bytes — including all
> decoder-visible lane descriptors, type tags, width schedules, exception models
> and routing decisions — beat the **best transform-enabled published reference**
> (`min(xz -9e --delta`, Gorilla-equivalent, ALP-equivalent) by ≥3%, at
> ≥1.0 GB/s decode and ≤1.25× the fastest reference's decode peak RSS.

Three properties of H13 matter:

1. **"real"** — the whole hypothesis is currently untestable (§4.2), and the
   project's own ledger forbids promotion without a new locked numeric family.
2. **"complete bytes … beat the best transform-enabled reference"** — not raw
   Brotli. This is the dual-bar (E3:4621-4628). Beating raw Brotli on float is
   the `ts.ref_field` failure mode: 89,877 B lost to 89,564 B and 80,650 B.
3. **"≥3%"** — derived, not arbitrary. See §10.3.

---

## 4. Strongest falsification case (the kill argument)

Six independent legs. Any one of legs 1–4 is sufficient to KILL the mechanism as
a *general-purpose* numeric compiler; legs 5–6 kill specific framings.

### 4.1 Leg 1 — Novelty: the interaction is already adjudicated closed

PORDER was killed as standalone novelty with the express finding that
"FastLanes, white-box compression, OpenZL, Corra, LeCo, BtrBlocks, and ALP already
cover the broad interaction" (E4:4825-4826). A typed-lane compiler *is* that
interaction. Doctrine item 13 lists PORDER as killed. Building it again with
numeric types substituted for width types is a relabel, and relabelling is
precisely what doctrine item 1 forbids ("Mechanism novelty, not
novelty-by-difference").

### 4.2 Leg 2 — Prevalence: the addressable headroom is the smallest identified cell, and the corpus does not exist

- The 3rd-backend gap is 664,307 B. Split: mozilla 429,893 (64.7%), **sao
  160,422 (24.2%)**, ooffice 51,625 (7.8%), samba 22,367 (3.4%), all others 0
  (E10:73-77).
- Floating-point numeric coverage can address **at most the sao cell**.
- The project's own audit already ruled on this family: **M2 "Float / unsortable
  numeric (MEDIUM, small) … Small headroom (~0.16 MB to landscape) but a
  distinct family worth a stride/delta probe (P4.2) rather than a new backend"**
  (E10:135-140). That is a standing verdict that the right response is an
  *adopt-class probe*, not a compiler.
- F1's fund/stop bar (E11) requires a mechanism to plausibly recover a
  **comparable fraction of its file's share**. Applied to sao that means
  recovering on the order of **160 KB** from one real file — i.e. essentially the
  whole cell — or the work does not move the frontier.
- Meanwhile `RESEARCH_LEDGER.md:4831-4833` states plainly that no new held-out
  numeric family exists and that promotion *requires* one. `tests/corpus/` contains
  **no real float binary**; the only f64 file is `synth-telemetry-f64.bin`, an
  explicitly favourable synthetic fixture (E15:115-133), and even that is
  "not yet recon'd" into the benchmark grids (E16:69-70).

**Consequence: track 13 is currently un-runnable as a novelty experiment, and
that is a frozen pre-committed outcome, not a reason to substitute synthetic
files.**

### 4.3 Leg 3 — Dual-bar: the project's own best numeric-representation result is a FRONT-GAP

The one mechanism in-project that got closest — record-period reference copy with
per-column residual contexts, including numeric columns — landed at 89,877 B on
`synth-timeseries.bin` while `xz -9e --delta=dist=14` reached 89,564 B and
`brotli q11` on the delta-transformed stream reached **80,650 B** (E3:4621-4628).
That is **+1.1% and +11.5% worse than references it was never compared against
during construction.**

Read that as a rule: *a numeric representation that beats ANVIL's own raw control
is worth approximately nothing until it also beats `xz --delta` and
Brotli-on-the-same-transform.* This is the single most transferable adversarial
lesson in the repository for this track, and it is measured.

### 4.4 Leg 4 — The measured in-project evidence says numeric transforms lose to dictionaries

From the one real, frozen, same-backend typed-leaf experiment we have (G2, remote
run `35943876855`, frozen binary SHA-256 `ea0531f…`):

| Finding | Number | Source |
|---|---|---|
| EXACT_DICT is the dominant typed contribution | D2 P-DICT 20,903 B; D4 P-DICT 84,375 B | E6:114-116 |
| Integer FOR alone is **worse than the all-raw carrier** | D2 P-INT 25,986 vs G1R 25,916 → **+0.27%** | E6:180, 190 |
| DELTA / DoD leaves **never selected** | D2: 11 INT_FOR, 2 INT_DELTA_FOR, **0 INT_DOD_FOR**; D4 P-INT: 99 INT_FOR, 1,147 RAW_LEX, no DELTA/DoD | E6:196-212, 253-258 |
| Adding P-MIXED over P-DICT bought almost nothing | D4: 84,361 vs 84,375 = **14 B** | E6:267-272 |
| G2's own ruling | "Dictionary representation is again the dominant typed contribution. Integer FOR is useful but secondary" | E6:271-272 |
| G2 excluded float/Gorilla/ALP by design | prereg §4 | E5:206 |

Two readings, and both cut against the track:

1. **Generous reading:** the numeric-coordinate-transform axis was tested on real
   structured data with a real backend and came *third* behind a plain token
   dictionary. Numeric modelling is not where typed representation value lives.
2. **Ungenerous but fair reading:** G2's leaves were *integer-lexical* only, so it
   does not directly test binary f64. True — but that is exactly why the track
   needs a *binary* numeric corpus, which §4.2 says does not exist.

Either way the burden of proof is entirely on the constructive lane, and nothing
in the current evidence base discharges it.

### 4.5 Leg 5 — Transmitted-parameter economics: the same genus has failed twice here already

A typed-lane compiler's decoder-visible payload is, by construction, a
**transmitted per-lane parameterisation**: lane count, lane offsets, per-lane
width schedule, per-lane type tag, per-lane operator/expression selection,
exception-model choice. That is the genus of:

- **G3 (transmitted stride σ): "FAIL. NOT DEFENSIBLE"** (E13:147-164);
- **ARI-REF transmitted-Δ**: the form that *works* is prior art; the form that was
  *defensible* failed (E13:125-128);
- **TCOPY transmitted-Δ**: "made BOTH .eh_frame and .rodata LARGER, and discovery
  became extremely expensive" (E13:154-156).

The project's synthesised rule: **a transmitted transform parameter does not pay
for itself.** Track 13 proposes a mechanism that is *mostly* transmitted
parameters. It must beat that prior, not assume it away.

### 4.6 Leg 6 — Placement: lanes must live inside the reference, and that is already closed ground

- Global lane/field transposition is a **measured decisive negative**: every
  tested ELF section grew (doctrine 13; E3:4613-4619; E12:157-164).
- If the typed lane is emitted **before** the matcher as a global de-interleave,
  it inherits that negative.
- If it is emitted **inside** the reference (per-column residual contexts), it is
  **datastruct-ts**, whose novelty ruling is **NO** and whose unsolved gap
  ("auto-discovery of the period and the field partition", ca true 24,055 vs naive
  44,837, +86%) was explicitly classified as an **engineering gap, not a novelty
  position** (E3:4616-4619; E12:171-176).

So both placements are closed: outside-the-reference is measured-worse; inside is
non-novel.

### 4.7 The single strongest kill sentence

> **Track 13 proposes, in a family the project's own audit already classified
> "MEDIUM, small… worth a stride/delta probe rather than a new backend", a
> compiler for an interaction (relation graph over typed lanes with a width-aware
> schedule) the ledger already killed as PORDER, whose components are all
> occupied by ALP/FastLanes/BtrBlocks/LeCo/white-box, whose two nearest in-project
> precedents lost on complete bytes to `xz --delta` and Brotli-on-transform, whose
> required corpus does not exist, and whose numeric-transform component was
> measured *third* behind a plain dictionary on the only real typed-leaf data we
> have — while its one favourable fixture is a generator artefact whose headline
> DoD claim changes from 99.8% to 0.0% zeros depending on timestamp
> quantisation.**

---

## 5. Hidden byte cost — where the mechanism actually dies

This is the leg I can settle analytically without any benchmark, and it is
independent of the prior-art argument.

### 5.1 The metadata cap conflict

`RESEARCH_LEDGER.md:4828-4829` freezes the surviving PORDER-probe condition as
"metadata/codebook cost at most **20% of gross savings**". Apply that to lanes.

Minimum viable per-lane descriptor (all decoder-visible, none optional):

| Field | Bits | Note |
|---|---|---|
| end-offset delta (uvar) | 8 (typ.) | 1 byte per lane is the common case |
| width code | 4–5 | needed for unpacking |
| operator/expression selector | 3–4 | ALP-style factor choice, Gorilla vs raw, … |
| exception/patch model | 0–2 | often derivable from the operator |
| **total** | **≈15–19 bits ≈ 1.9–2.4 B/lane** | round to **2–3 B/lane** |

Let `S` = fraction of lane bytes saved and `M` = metadata bytes per lane over
`n` lanes, lane input width `w` bytes. Then
`M/S·input = (m/w)/S` where `m ≈ 2–3`. The 20%-of-gross-savings cap requires

> **(m / w) / S ≤ 0.20  ⟹  S ≥ 5·m/w.**

At `w = 16` (interleaved mixed binary, one numeric field per record):
`m=2 → S ≥ 62.5%`, `m=3 → S ≥ 93.8%`.
At `w = 8`: `m=2 → S ≥ 125%` — **impossible**.
At `w = 1024` (contiguous column, HDF5/Parquet interior): `m=2 → S ≥ 0.98%` —
trivial.

**Derived conclusion (§5.1, `derived`; reproduced by
`prototypes/swarm-2026-10-02/13-numeric-compiler/fledge/lane_overhead.py`):** a
typed-lane compiler can only satisfy the project's own metadata cap on
**long contiguous column runs** — i.e. exactly the Parquet/ORC/HDF5/netCDF
interiors where ALP, FastLanes, BtrBlocks and LeCo already operate and where the
format already carries a schema.

On interleaved mixed binary — the population that would make track 13 a
*general-purpose* compressor rather than a format adapter — the cap is
**structurally unreachable** at `w ≤ 16`. This is a derived structural kill, not
a tuning problem, and it does not depend on any prior-art judgement.

### 5.2 The other hidden bytes

1. **Block re-header cost.** Gorilla B2 costs exactly **11 more bits** than B1 for
   identical payload (§1.2.1). Every re-block decision must therefore be
   arbitrated on measured whole-stream bytes — exactly the K-reblocking
   arbitration pre-registered in E8 §2b — never on a proxy.
2. **Two independent value domains.** The track brief itself asks for "reversible
   typed lanes … mixed data". Type *tagging* is the expensive part; ANVIL's own
   precedent is that transform fields and residual bytes must land in **separate
   statistical domains** so gains are attributable (TCOPY ablation, doctrine
   footnote in `MASTER-BRIEF` lineage item 15). A lane compiler that mixes them
   cannot attribute its own win.
3. **Carrier tax, measured.** G2's per-leaf IDs and payload lengths alone cost
   **+1.02%** on D2 (25,916 vs 25,654) before a single leaf was chosen (E6:180).
   Any lane descriptor table inherits an equivalent tax.
4. **Routing decision is decoder-visible.** Per doctrine item 2, every selector
   and framing field is charged. For a whole-file routed lane that is small
   (one flag); for **regionally** routed lanes — which §4.2's prevalence problem
   essentially forces — the region map is a real cost, and E4:4819-4824 warns that
   any result "that depends on omitted framing/model/dictionary/planner cost" is
   killed.

---

## 6. Hidden cycle cost — and where I *refuse* to attack

Being adversarial does not mean attacking an axis that is strong. Two axes are
strong for the numeric-lane direction and I state that plainly, because
overclaiming a kill on a weak axis would itself be a falsification failure.

### 6.1 Decode throughput is NOT a problem — measured

| Mechanism | ns/value | GB/s out |
|---|---|---|
| Gorilla Case-A dominant | 2.91 | **2.75** |
| Gorilla smooth-drift (B1-heavy) | 7.68 | 1.04 |
| vXOR (vertical XOR) | 0.146 ns/**B** | **6.86** |
| DoD (60 s cadence, ±3 s jitter) | 4.14–5.90 | — |

(E9:100-112.) Against a project whose best measured Brotli-class decode is
~0.8–1.2 GB/s (doctrine/ledger), **a correctly implemented numeric lane is
decode-competitive or better**. Any argument that "a numeric lane is too slow to
decode" is falsified by the project's own measurements and should be struck from
the proposal.

The real risk is the opposite one, and it is a *regime* risk: Gorilla's cheap
2.91 ns/val regime co-occurs with 2.88 b/val, while its 7.68 ns/val regime costs
54.15 b/val (E9:103-104). **Fastness and smallness are anti-correlated across
Gorilla's own regimes** (6.75× wire difference for 2.6× time). A lane
implementation that reports only the Case-A row is reporting the favourable end of
an anti-correlation.

### 6.2 Encode/planner cost is the real cycle problem, and its precedent is adverse

The only measured typed-leaf planner costs we have (G2, CI, E6:354-372):

| File | anatomy | local candidate scoring | marginal search |
|---|---:|---:|---:|
| D1 | 0.74 ms | 510 ms | 6.80 s |
| D2 | 3.97 ms | 784 ms | 75.10 s |
| D4 | 8.03 ms | 2.68 s | 121.28 s |

The structural parser is cheap; **repeated backend evaluation dominates**. A
numeric-lane compiler adds an orthogonal search axis on top: type × operator ×
lane partition × expression factor. That is a ≥3–4× multiplication of the
candidate set the G2 planner already could not navigate — and G2's own planner
**lost to dense local composition on both real families** (P-MARGINAL 24,865 vs
P-DICT 20,903 on D2; 86,505 vs 84,375 on D4, E6:302-322).

The constructively useful conclusion: if a numeric lane ever ships, its planner
must be **dense/batched and cheap-signal**, not another per-candidate whole-backend
search. I record that as a constraint on any successor, not as an endorsement.

### 6.3 Memory

Gorilla-style decode state is tiny (two u32 lanes + a normalised bit accumulator).
ALP can be fused single-pass to avoid materialising the exponent array. So
**decode RSS is a non-issue for the mechanism itself** — the axis is weak, and I
will not manufacture a kill there. Two honest caveats:

1. The project's decode-RSS instrument is **not closed**: the G5D prototype "still
   emits a zero decode-RSS placeholder" (E4:4837-4840), and G5D is not
   dispatchable. A track-13 RSS claim made with today's tooling would be measured
   with a known-bad instrument.
2. The G2 carrier demonstrated the wrong pattern for metadata memory: explicit
   per-leaf lengths are charged *even when the grammar could parse without them*
   (E5:258-259). A lane map has the same property and should be charged the same
   way.

---

## 7. Decoder and resource risks (must be pre-registered, not discovered)

These are implementation-class but several are **already-measured traps** in this
project's own profiling work (E9:114-121), which is why they are not hypothetical:

1. **Bit-by-bit reader.** Measured **24.2 ns/val**, 3–8× slower. A normalised u64
   accumulator is mandatory. (Same defect class as grotli's D1–D6, E8 §1.)
2. **Shift-by-64 UB.** `shift-by-64` on full-word fields silently corrupts after
   the first value because clang masks the shift. `bits == 64` must be
   special-cased. This is a **correctness** bug that round-trips-looking smoke
   tests can miss.
3. **Vectorisation requires 8-byte chunking**; a byte-wise XOR loop measured 5×
   slower. Design the lane kernel 64-bit-first.
4. **Container overflow for B2.** `13 + mb` exceeds a 64-bit container for
   `mb > 51`; the frozen record's own percentage (§1.2.1) silently assumes a
   container that is not stated in the prereg.
5. **Bit-pattern exactness, not float equality.** `G-RT` in E7:43-49 requires
   strict uint64 comparison including −0.0/+0.0, sign flips, denormals, max-u64
   payloads; NaN claims require payload-level comparison. `float==` will accept
   incorrect decoders here.
6. **Amplification / unbounded expansion.** A typed lane declares both a lane
   count and per-lane value counts. Product of declared counts is the attack
   surface. Pre-register: reject if `Σ declared lane values > 2^20`, if any
   declared offset exceeds the source bound, if reconstructed length ≠ declared
   output length, or on any trailing byte. This is the same hardening discipline
   as G2's decoder contract (E5:455-479) and the format-security lane's.
7. **Incompressible-input inflation.** Measured: Gorilla on random control
   **bloats** at 37.79 b/val (E9:105). An ε gate is not optional; the frozen
   `ε_strict = max(2, 0.005·|block|)` and the "random → reductionPct exactly 0"
   tripwire (E7:69-72, E8:80) must be enforced and the tripwire must **fail the
   run** if reduction > 0.

---

## 8. The strongest surviving case (stated at full strength)

If I only attacked, this review would be worthless. Here is the case I could *not*
kill:

1. **The coverage gap is measured, not imagined.** `synth-telemetry-f64.bin` sits
   at **0.8880** with ANVIL defaults (E15:171) while its Gorilla-equivalent wire
   on the same generator is measured at **2.88 b/val** (E9:103) — a ~4× gap on
   320,000 B. Even discounting the fixture as favourable (§1.2.3), *no ANVIL
   mechanism currently touches binary float records* (E10:137, E17). That is a
   real capability gap in a general-purpose codec, not a synthetic artefact.
2. **The cost profile is unusually favourable.** Decode 2.75 GB/s / vXOR 6.86
   GB/s (§6.1), low RSS, tiny decoder state, ~2–3 B/lane metadata on contiguous
   columns (§5.1). This is a better decode-side profile than almost anything else
   in the project's portfolio, and ALP's adoption by Parquet (E14) is independent
   market confirmation that the family is worth having.
3. **The honest, defensible framing has real value even at zero novelty:** an
   adopted, dual-bar'd numeric lane converts a dead direction into a **live
   measurement instrument**. Without it, every future numeric claim in ANVIL will
   be compared against raw Brotli and will produce FRONT-GAP illusions of exactly
   the `ts.ref_field` kind. The reference grid *needs* this row.

That third point is the crux of my verdict. The mechanism's value is
**instrumental and adopt-class**, and the project has an established class for
exactly that: the Gorilla-Pv pre-registration — "ENGINEERING ADOPT of published
prior art … Pareto-gated, NO mechanism-level claim" (E1/E5 lineage, `docs/pre-
registrations/gorilla-pv.md:10-17`), with block-routing-only placement (no
pipeline stage feeding the matcher) and frozen, blind-adopted detector constants.

---

## 9. Alternative mechanism, if one is justified

**Recommendation: do not build a second numeric mechanism. Build the reference row.**

- **Alt-A (recommended, adopt-class):** a faithful native implementation of one
  published numeric lane (Gorilla A/B1/B2 + DoD is the cheapest faithful choice;
  ALP-equivalent decimal-factor + exponent-delta + exception mask is the stronger
  *ratio* reference and is what the dual bar really needs) added to the benchmark
  harness **as a reference codec only** — never as an ANVIL mode, never routed,
  never wired into `src/`. Purpose: it converts the numeric axis from an unmeasured
  hope into a measured reference row on the dense grid, and it discharges the
  dual-bar's "the reference codec is given the same representational opportunity"
  requirement (doctrine item 3, `06-do-not-reburn.md` E1/E5).
- **Alt-B (only if a novelty lane is still wanted after Alt-A lands):** put the
  numeric content **inside the reference**, not in a lane compiler. The one family
  in the project with a live novelty claim is transformed self-reference
  (TCOPY/PNRA), and `06-do-not-reburn.md` A2 explicitly reopens the
  derived-parameter lane **only if the transform is exact, not estimated**. A
  numeric residual class inside a transformed reference — e.g. exact additive
  deltas on aligned 32/64-bit fields, where the algebra gives `v + position`
  invariance (PNRA) — satisfies: (i) exactness, (ii) inside-the-reference
  placement, (iii) zero transmitted parameter bits, (iv) a defensible separation
  from ARI-REF's transmitted-Δ form. **Honest labelling: numeric content is the
  application domain, not the novelty.** And note it still needs its own
  pre-registration; it does not inherit track 9's.
- **Alt-C (explicitly rejected):** a self-scheduling typed-lane compiler with
  transmitted per-lane parameters. This is PORDER-with-types (§4.1) *and* the
  transmitted-parameter genus that already failed twice (§4.5). I recommend the
  ledger record it as a closed framing so a later session does not rebuild it.

---

## 10. Decisive GitHub-Actions-only experiment

### 10.1 E13-NUMDUAL — purpose

One experiment, run remotely, that can kill the track. It is designed so that its
most likely outcome is a clean negative, and so that a positive is
unambiguously attributable.

### 10.2 Corpus gate (runnability precondition)

- **Calibration/control (may be reused):** Silesia `sao` — the only real float
  file already in the landscape grid. It is a *calibration* file, **not** held-out.
- **Discovery (must be newly locked, real, with SHA-256 manifest):** ≥2 files that
  are genuinely numeric binaries, at least one of which is *not* decimal-like
  (so ALP's advantage does not decide the run), e.g. a real netCDF/HDF5 ocean
  profile, a real Parquet float column set, a real scientific-sim binary dump.
- **Held-out (opened exactly once, only after discovery passes):** 1 further real
  numeric binary.
- **Diagnostics only, never gate inputs:** `synth-telemetry-f64.bin`,
  `synth-ndjson-columnar.ndjson`. Tagged `{synthetic}` per E2:4455-4459 and E15.
- **If no real file can be locked: E13-NUMDUAL IS NOT RUNNABLE, and track 13
  converts from HOLD to KILL.** This is pre-committed here so it cannot later be
  rationalised into "run it on synth instead".

### 10.3 Arms (all native, same compiler/flags/machine, complete bytes)

| Arm | Content | Role |
|---|---|---|
| A0 | raw bytes | reference floor |
| A1 | Brotli q11 and q9 on raw | the *weak* bar; reporting only |
| A2 | `xz -9e --delta=dist=P` for detected P, **both** P≤256 and P>256 reported | **dual-bar transform-enabled control** (E2:4436-4443) |
| A3 | ANVIL current best config (same build) | own-pipeline control |
| A4 | Gorilla A/B1/B2 + DoD, native, two-lane u32, normalised u64 accumulator | adopt-class reference; correctness per `G-RT`/`G-DOD`/`G-VXOR` (E7) |
| A5 | ALP-equivalent: decimal factor ladder + exponent delta + mantissa residual + exception mask | adopt-class reference; the strong ratio bar |
| A6 | **the track's mechanism** (typed lanes), if it exists | candidate |
| A7 | A4 and A5 **routed at block granularity as alternatives**, never as pipeline stages (gorilla-pv §PHASE B) | the only sanctioned integration shape |

Every arm reports: complete bytes (all headers/tables/framing charged), encode
MB/s, decode MB/s, decode peak RSS (`getrusage` maxrss), and tool binary-size
delta. Median ≥3 paired reps, same-run, same-build, per the project's paired
protocol.

### 10.4 Dual-bar rule (binding)

> **A6 (and A7) are judged against `min(A2, A4, A5)` complete bytes — never
> against A1.** Both deltas are reported. Any claim expressed against A1 is
> void. The transform-enabled bar is mandatory because the project's own best
> numeric-representation result was a FRONT-GAP against exactly this bar
> (E3:4621-4628).

### 10.5 Frozen thresholds (set **now**, before any run; movement after results = VOID)

**Correctness gate (any failure ⇒ immediate KILL, no interpretation):**

- C1 byte-exact roundtrip on every arm, every file;
- C2 `G-RT` bit-pattern equality (NaN payload, ±0, denormals, max-u64) for A4/A5;
- C3 `G-DOD` boundary vectors: 0→1b; ±63 **and** ±64→9b; ±255 **and** ±256→12b;
  ±2047 **and** ±2048→16b; ±2049→36b; Δ₁ 14-bit;
- C4 malformed/truncated/trailing/oversized-declared-count rejection suite green;
- C5 random-control tripwire: reduction on incompressible input **exactly 0**, else
  the run FAILS.

**Prevalence gate (cheap; runs first):**

- P1 at least one of A4/A5 beats A1 by **≥10% complete bytes on ≥2/3 real
  discovery files**. If published prior art cannot clear the gap on real data,
  ANVIL's lane cannot clear it by a defensible margin ⇒ **KILL**.

**Promotion gate (A6/A7 must satisfy ALL):**

- N1 `bytes(A6) ≤ 0.97 × min(bytes(A2), bytes(A4), bytes(A5))` on **≥2/3** real
  discovery files (≥3% better than the best published transform-enabled
  reference). *Origin of 3%:* the frozen PORDER probe retained a 1%-over-single-
  factor bar (E4:4828); 3% is that bar raised to clear the measured carrier and
  routing taxes we have actually observed (G2 carrier +1.02% on D2, E6:180) with
  margin.
- N2 decoder-visible metadata + codebook bytes **≤ 20% of gross savings** (the
  frozen E4:4828 condition, applied literally). Per §5.1 this additionally
  *requires* contiguous column layout; report the lane geometry.
- N3 decode ≥ **1.0 GB/s** **and** ≥ **0.90 ×** the fastest reference arm's decode.
- N4 decode peak RSS ≤ **1.25 ×** max(fastest reference RSS, A0-control RSS).
- N5 encode time ≤ **20 ×** A1 encode time (a ratio gate, not an MB/s gate —
  absolute MB/s is not portable across runners).
- N6 tool binary-size delta ≤ **40 KiB** per lane.
- N7 held-out: N1–N6 re-applied once to the single locked held-out file; any
  failure ⇒ KILL.

**Interpretation rule:** KILL requires no post-hoc rescue. No arm may be added,
removed, or re-tuned after the run. A partial pass is reported as a partial pass.

### 10.6 Why this experiment is decisive

It is the only design that can simultaneously (a) prove the gap is real on real
data, (b) hold prior art to the same representational opportunity, (c) charge
every decoder-visible byte, and (d) separate the *novelty* question (already
settled: none) from the *value* question (open, and worth one honest measurement).
Any weaker design reports raw-Brotli wins — i.e. FRONT-GAP illusions.

---

## 11. Failure modes (enumerated, so they can be recognised rather than argued)

1. **Fixture substitution.** Synthetic telemetry is used to satisfy the corpus
   gate because real data is inconvenient. ⇒ Void; converts HOLD to KILL.
2. **Regime cherry-pick.** Only Gorilla's Case-A row (2.88 b/val) is quoted and the
   54.15 b/val B1-heavy row is omitted (§6.1 anti-correlation).
3. **Carrier omission.** Per-lane descriptors, type tags, or the routing map are
   excluded from "complete bytes" because a grammar could infer them. ⇒ Void
   (E5:258-259 charges lengths even when parseable).
4. **Raw-bar comparison.** A win against A1 is reported as a win. ⇒ Void (§10.4).
5. **Detector tuning.** Auto-detection constants are fitted on ANVIL corpus files
   after seeing outcomes. Explicitly forbidden by the Gorilla-Pv pre-registration
   §3 ("adjustment = new pre-reg"), and it is the same error class as F-G1.
6. **Planner-reported-as-operator.** A win is attributed to lane typing when it
   came from the search (G2's operator/planner confound, E6:409-416).
7. **ALP-composition relabel.** Decimal factor + exponent delta + exception patching
   presented as a new interaction. ⇒ ALP (E14).
8. **PORDER relabel.** Lane relation graph + width-aware schedule. ⇒ KILLED
   (E4:4824-4829).
9. **Transmitted-parameter hope.** Assuming a per-lane schedule will amortise. ⇒
   two prior project negatives (E4.5).
10. **RSS measured with the closed-bad instrument** (E4:4837-4840) and reported as
    a passing memory gate.
11. **Round-trip smoke instead of `G-RT` bit-pattern equality**, letting a NaN or
    ±0 defect through (§7.5).
12. **Incompressible inflation shipped** because the ε gate was "tuned away".
    C5 exists precisely to make this a run failure.
13. **Held-out opened early or twice**, converting discovery into validation.

---

## 12. RECONCILIATION — `13-numeric-compiler-space-bunny.md` (50,778 B, read in full)

### 12.0 Status change and good-faith note

This document is no longer INTERIM. §1–11 above is **unchanged**; the
constructive lane and I agree on more than we differ, and §12.7 records where the
constructive report is *better* than my interim. Where we differ, the
disagreement is recorded with quoted text, evidence location, and my ruling —
never by editing my earlier text.

**Constructive mechanism, restated fairly.** CANL — *Copy-Anchored Numeric Lane*:
a typed numeric lane whose predictive anchor is **defined as a read-back of the
already-reconstructed output** at offset `(q − L) + off` rather than as a carried
register; three exact per-record-unit modes (`M_RAW` / `M_XOR` / `M_VALDELTA`),
emitted inside the existing op/macro-op stream; per-lane mode selection against
actual downstream rANS cost. Space Bunny states up front that the *codes* are
prior art and that the **rebase rule is the entire novelty surface**
(`13-numeric-compiler-space-bunny.md:19, 282`). That framing is honest and it is
the right frame to argue about. I therefore argue about exactly that.

### 12.1 Disagreement table

| # | Constructive claim (quoted, with line) | My ruling | Decisive evidence | Action |
|---|---|---|---|---|
| D1 | "**the rebase rule itself** … the entire novelty surface … **RESIDUAL — the entire novelty surface**" (`:19, :282`) | **Rejected.** It is placement engineering, and the project's own ruling on the inside-the-reference placement already exists and is adverse. | `RESEARCH_LEDGER.md:4614` — placement inside the reference is "a satisfied placement constraint, **not a novel interaction**"; `:4609` datastruct-ts **NOVELTY: NO**; and that mechanism **measured 89,877 B vs `xz --delta=14` 89,564 B vs `brotli q11`-on-delta14 80,650 B** (`:4621-4628`). | Novelty claim struck. Adopt-class retained. |
| D2 | "CANL is Shape-C … **That distinction is the whole novelty surface**, and it is exactly where the ledger's most damning precedent sits" (`:110`) | **Conceded as self-identification, and it is fatal.** The report names its own killer in the same sentence and then proceeds past it. | Same as D1. | Accept the concession; it converts PILOT-into-novelty into HOLD. |
| D3 | "synth-arith … `[projection]` ≈ **12.9 KB … ratio ≈ 0.051**, i.e. ~5.9x below the `xz-9e` bar" (`:51, :224`) | **Rejected as a headline number.** Unmeasured; omits the per-unit mode symbol its own wire emits; its 1.6 bits/value payload entropy has no measurement behind it; and the project's binding rule for this file forbids using the 87,013 B / 79,372 B bar. | See §12.3 in full — including the **measured 16,313 B** transform-enabled row on this exact file, which `gate-verdict-i8-ari-stride.md:261-265` makes **binding**: "A win measured only against the raw bar will be struck." | Struck as a headline. Must be re-expressed as a range with both unmeasured terms named, and judged against 16,313 B. |
| D4 | "**8 interleaved `u32` AP columns** … `v += stride + randint(-1,+1)`" → δ ∈ {s−1,s,s+1} ⇒ 1.6 bits/value (`:51`) | **The 1.6-bit term is unmeasured, and the nearest measured quantity on the same file is 0.74 bits higher.** | `gate-verdict-i8-ari-stride.md:306-307`: "**The measured eps alphabet is {−2..+2}, giving a non-growing floor H(eps) = 2.32 bits**." H = 1.585 bits is only correct for an exactly-uniform 3-symbol noise alphabet. | Treat the payload term as **1.6–2.4 bits/value (unmeasured range)**, not 1.6. |
| D5 | "§5.1 … **total ≈ 5.2 B/record ⇒ ratio ≈ 0.371** … vs … **dual bar `xz --delta=14` 0.3198**" (`:222`), then "**§5.1 projection: 0.503 → 0.371 on `synth-timeseries`**" used as PILOT rationale (`:420`) | **Rejected as a win.** The projection **loses the transform-enabled bar by 16.1%** (0.371 vs 0.3198; 104,000 B vs 89,564 B), and the row printing both numbers does not say so. 4.00 of 5.17 B/record is **raw f32** — 77% of the projected output is un-coded float, which a plain stride-delta on raw bytes beats. | `:222` itself prints `0.3198`; `RESEARCH_LEDGER.md:4623-4625` gives 89,564 B for `xz -9e --delta=dist=14` on the same file. | The `0.503 → 0.371` claim is struck. On the project's closest numeric binary the mechanism's own projection is FRONT-GAP on the dual bar. |
| D6 | `w ∈ {4,8}` in the lane definition (`:118`) vs the projection table listing an "`id` (u16, period 1000)" lane at offset 12 (`:219`) | **Internal inconsistency.** A `u16` lane is not representable under `w ∈ {4,8}`. | `:118` vs `:219`. | Fix the definition (`w ∈ {2,4,8}`) or drop the row. Blocks the cost table from being a faithful charge. |
| D7 | Wire emits `step u8` (`:146`); `step ∈ [0,7]` in the definition (`:118`); the decoder validation list (`:172`) validates `lane_count`, `off+w`, `w`, `stride_mode` — **not `step`** | **Decoder-safety gap.** `step` participates in lane addressing and is unvalidated on a hostile wire. Exactly track-19's class. | `:146` vs `:172`, against `10.4`'s own stated intent. | Add `step` validation, or remove `step` from the wire. Blocks containment. |
| D8 | Re-entry path: "emit raw anchor (w literal bytes); **then as above**" (`:182-183`) | **Ambiguous/undefined.** If the mode is `M_RAW` the else-branch writes `w` bytes and the loop writes `w` more into a `w`-byte slot. And the re-entry anchor is *self*-referential — it reads `out[p+off]`, which is unwritten. Correct semantics: **re-entry ≡ `M_RAW`**, not "raw anchor + payload". | `:176-183`. Not pedantry: re-entry is the price the §3.1 "rebase is free" argument implicitly assumes away. | Restate as: re-entry costs `8w` bits and is exactly `M_RAW`; make re-entry count a first-class charged diagnostic. |
| D9 | `A4` dual bar uses `xz -9e --delta=dist=P` with "P from the frozen detector" (`:324, :347`) | **The bar would be computed at a mis-detected distance on the files that matter.** The same report records that this detector class scores **2.57%** on `synth-arith` and **0.01%** on `synth-timeseries` (`:107`, from `RESEARCH_LEDGER.md:2332-2334`). | `:107` is self-defeating for `:347`. | **Control U1, mandatory:** make the dual bar an **oracle** sweep — `min` over all `d ∈ 1..256` — plus the `brotli-q11+delta` row. Removes the detector from the bar. |
| D10 | No metadata cap appears in `PROMOTE-CANL` (`:355-363`) | **Rejected: incompatible with the frozen cap at this descriptor size.** With CANL's own `5 B/lane + 4 B` header, the frozen `≤20% of gross savings` condition (`RESEARCH_LEDGER.md:4828`) requires `S ≥ 25/w`, so **no record span narrower than ~64 B is feasible** at a realistic ≤50% saving rate. | Arithmetic in §12.4. Empirical anchor: G2's measured **+1.02% carrier tax** on D2 (25,916 vs 25,654, `I10-GROTLI-G2-RESULTS.md:180`) ≈ 0.8 B/column, so 5 B/lane is if anything conservative — the descriptor is honest; the *geometry* is the problem. | Add the cap as a gate **and** report lane geometry per file. |
| D11 | §12 leans PILOT on the byte opportunity (`:417-420`) while §6.2 rates risk **HIGH**, bounds the mechanism-specific benefit at **≈0.004 B/value**, and says "**cannot be the source of a frontier crossing**" (`:290`); §11.6 says the rebase advantage "is a problem **ANVIL creates for itself**" (`:406`) | **The self-refutation wins.** §6.2/§11.2/§11.6 are the load-bearing analysis and they are correct. The byte opportunity is real but **adopt-class and, on both cells projected, already beaten by a measured generic transform.** A PILOT of the *mechanism* is not justified by it. | Internal: `:290`, `:402-407` vs `:417-420`. External: §12.3, §12.4. | Re-scope the pilot to the **reference row + corpus lock**, not the mechanism. |
| D12 | "G2's int leaves … ≈ neutral … **Caution: this is on JSON lexical columns, not binary columns** … must not be conflated" (`:102`) | **Conceded — and it is correct.** My interim §4.4 leg 4 over-weighted G2 for a binary-float question. This is a real limit on my own argument. | `:102` vs my §4.4. | I withdraw G2 as a *primary* argument; it survives only as secondary evidence that a fifth typed operator has small measured marginal value in this project. |
| D13 | "the two files designed to make a float lane win have **zero measured rows on any grid**" (`:81`) | **Confirmed and important.** I verified the corpus survey independently; the claim stands, and it is the strongest *pro-availability* fact in either report. | `tests/benchmark-recon-i9.md:69-70` ("Two corpus cells outside the I7 '9 new' … not yet recon'd"). | Makes "measure the substrate first" the cheapest high-value action in the track. |

### 12.2 MANDATED QUESTION 1 — does CANL's copy-anchored rebase escape the Gorilla / ALP / FastLanes / xz-`--delta` lineage, or is it placement engineering?

**Ruling: placement engineering. The lineage is not escaped.**

**(a) The novelty surface reduces, by the report's own definition, to "derive the predictor distance instead of transmitting it" — and that already has a verdict.**
CANL's anchor is `out[q − L + off]`: a read-back of output at a position computed from the current position, with `L` derived, not transmitted (`:120-123`). Strip the framing and that is `xz --delta[=dist=d]` with `d` auto-detected instead of supplied — read-back from output at `cur − d`, with `d` derived. Both halves are individually occupied:
- read-back-from-output **is** the defining property of the LZ family and of every published filter in this class;
- derived-vs-transmitted stride is **already adjudicated**: `RESEARCH_LEDGER.md:4436-4443` rules the record period `P` to be "**a framing constant**" and that "**the only thing ANVIL adds is auto-detecting P = infrastructure** … Not gated, not claimed."

CANL's residual novelty surface is therefore *exactly* the residue the ledger classified as infrastructure.

**(b) The project implemented this form and measured it.**
`docs/gate-verdict-i8-ari-stride.md:166` — "**G4. Position-derived σ, zero transmitted bits — PASS IN PRINCIPLE; MEASURED DEAD ON synth-arith.bin.**" That is the position-derived-parameter form, on the same file CANL projects on. To be fair: CANL's transform is *exact* (a delta against the immediately preceding lane value has zero residual), so the §4a exactness amendment does **not** bite — I do not claim it does, and the constructive report is right to rely on exactness. But that is precisely the point: the exactness condition is satisfied, so the remaining novelty **must** come from placement — and placement is where D1 lands.

**(c) The bytes attributable to the rebase are ~0.004 B/value — by the report's own arithmetic (§6.2, `:290`).**
Gorilla's window state is 2–4 B per block; ALP's predictor choice is 2–4 B per 128 values; the maximum state-maintenance saving is 0.02–0.03 bits/value. A mechanism whose mechanism-specific byte effect is ~0.004 B/value cannot carry a crossing — and, more importantly here, **cannot be what separates CANL from A1 in a dual-bar comparison**, because the dual bar (`xz --delta`, Gorilla, ALP) has already amortised the same state away. I accept this number and adopt it as the basis for my ruling. It is the strongest thing in the constructive report.

**(d) The "composes with the copy layer" claim (`:287`) is not established, and is probably a conflation.**
The claim is that Gorilla/ALP/Parquet columnar state cannot compose with an LZ copy layer. In Parquet the columnar encoding is applied **before** the general codec, so LZ copies inside e.g. zstd/brotli land on the *encoded* column bytes — the copy layer never sees the value structure, and the state is never invalidated. CANL puts the predictor **inside** the op stream, so copies land on decoded values and the anchor is re-established for free. **Both compose with a copy layer; they differ in ordering.** Nothing measured favours one ordering — and the one ordering this project measured against itself, the value-domain predictor placed inside the reference, **lost**: `ts.ref_field` 89,877 B vs `xz --delta=14` 89,564 B vs `brotli`-on-delta 80,650 B (`RESEARCH_LEDGER.md:4621-4628`).

**Conclusion.** The rebase is a real engineering property with a ~0.004 B/value cost benefit and an unmeasured cycle benefit on mixed data. It is **not** mechanism-level novelty. Adopt-class is the correct label — which is what §6.2 of the constructive report says, and §6.2 is inconsistent with its own PILOT rationale in §12.

### 12.3 MANDATED QUESTION 2 — is the projected ~0.051 on `synth-arith.bin` a synthetic dual-bar trap?

**Ruling: yes, on four independent counts. The number is not usable as a headline in any form.**

**Count 1 — the bar the report uses is one the project's own gate verdict voided.**
§2.1 sets the bar at `xz-9e` 79,372 B and `brotli-q11` 87,013 B — both **raw**-input bars. `gate-verdict-i8-ari-stride.md:237-239` records the transform-enabled measurement for this exact file: **`brotli-q11` applied to the delta+zigzag-varint transformed bytes of `synth-arith.bin` yields 16,313 B** — 5.33× below the 87,013 B row — and `:243-250` rules:

> "**The 87,013 B bar is not a frontier; it is a measurement of a transform Brotli declines to apply.** Any claim of 'we approached/beat q11 on synth-arith' is a claim about beating a self-imposed handicap…"

and `:261-265` makes it **binding on pre-registration design**:

> "Any ARI-STRIDE pre-registration on synth-arith.bin MUST benchmark the reference codecs against the **delta-transformed** bytes as an additional reference row … and MUST state its result against BOTH bars (raw 87,013 and transformed 16,313). **A win measured only against the raw bar will be struck.**"

CANL's `A4` arm does include `brotli-q11 on stride-delta(P)`, so the *intent* is right — but §2.1/§5.1 never cite 16,313 B, never name it as the bar, and state the headline as "5.9x below the `xz-9e` bar". That is a struck comparison.

*(Verification debt I must disclose: 16,313 B is a `[measured]` figure quoted in a gate verdict from a **different window** than the frozen BDC90474 grid, and I did not reproduce it. It is citable-but-unreproduced and must be re-measured in the frozen grid before anything is built on it — control U2 in §12.6.)*

**Count 2 — the projection omits a cost its own wire specification incurs.**
§3.3 emits `mode_symbol` "per (lane, unit) that is not M_RAW-by-copy". On `synth-arith` (8 lanes, unit = 32 B) that is **one mode symbol per value**: 64,000 symbols. §5.1's cost model contains `+ n_modes · h_mode` — and then charges it in neither projection. Adding it:

| assumption | bits/value | bytes | ratio |
|---|---:|---:|---:|
| mode hoist (per-lane constant, 1 bit/lane) — **not what §3.3 specifies** | 1.60 | 12,801 | **0.050** |
| per-unit mode symbol, mode dist ≈ 95/4/1 (`h ≈ 0.32`) | 1.92 | 15,360 | 0.060 |
| per-unit mode symbol, mode dist ≈ 85/10/5 (`h ≈ 0.75`) | 2.35 | 18,784 | 0.073 |
| **measured transform-enabled row (`brotli-q11+delta`)** | **2.04** | **16,313** | **0.0637** |

The headline 0.051 is reachable **only** under the hoist assumption, which contradicts the wire in §3.3. Under the spec as written the projection **straddles** the already-measured 16,313 B row rather than clearing it. That is the finding: the novelty-bearing margin over a measured generic transform on this file is, on the report's own specification, **between roughly −15% and +15% — indistinguishable from zero.**

**Count 3 — the payload entropy term has no measurement behind it.**
1.6 bits/value requires `δ` to be an exactly-uniform 3-symbol alphabet. The nearest measured quantity on this exact file is **2.32 bits**: `gate-verdict-i8-ari-stride.md:306-307` — "**The measured eps alphabet is {−2..+2}, giving a non-growing floor H(eps) = 2.32 bits**." I flag honestly that this figure comes from an estimator experiment and may partly reflect estimator error rather than generator noise; either way it is a *measurement on this file of a comparable quantity*, and it is 0.74 bits above the projection's assumption. Substituting it moves the payload term to 2.32 bits/value ⇒ 18,560 B before mode symbols, i.e. **0.073–0.095** — worse than the measured 16,313 B bar on the report's own numbers.

**Count 4 — the baseline it improves on is a known anomaly, and the corpus has a demonstrated history of over-reading itself.**
§12.1 leans on "1.000 vs 0.310". But 1.000 is a **store-class** row, and the report itself excludes store-class throughput as not citable (`:35`, "open store-path anomaly"). A store row is a detector/store artefact until proven otherwise, so the *magnitude* of the opportunity is unverified even though the *existence* of the gap (no AP token type; `RESEARCH_LEDGER.md:2352-2358`) is solid. And this corpus has a recorded history of its own cleanliness figures collapsing: the same ledger records this family's recorded period being **aliased 8×** (28 → 14) and a 30.4% constant-offset figure being "**an 8x-decimation artifact**", corrected to 21.7% (`:4636-4642`), plus the standing `{synthetic}` warning that such figures "are a property **our generator chose**" (`:4455-4459`).

**Net:** `synth-arith` can still demonstrate *"ANVIL has no arithmetic-relation token type"* — a genuine, Experiment-W-backed capability finding. It **cannot** carry a magnitude claim, and **0.051 is struck as a headline**.

### 12.4 MANDATED QUESTION 3 — is any proposed metadata cap compatible with the measured +1.02% G2 carrier tax?

**Ruling: the descriptor itself is honest; the geometry it implies is fatal, and no cap compatible with it was proposed.**

1. **Credit where due.** CANL's `5 B/lane + 4 B` header (`:212`) is *more* conservative than my interim §5.1 estimate of 2–3 B (I had assumed bit-packing; CANL emits five unpacked uvar/u8 fields, so 5 B is correct for its wire). It is also consistent with the only measured carrier tax we have: G2's per-leaf IDs + payload lengths cost **+262 B on 25,916 B (+1.02%)** across ~326 columns ≈ 0.8 B/column on a *compressed* carrier — several bytes per column uncompressed. **CANL's descriptor accounting is not the problem, and I withdraw any implication that it is.**
2. **The problem is the frozen cap versus lane width.** `RESEARCH_LEDGER.md:4828` retains the ≤20%-of-gross-savings condition. With `m = 5` B/lane, saving fraction `S`, record span `w`:

   `metadata/savings = (m/w)/S ≤ 0.20` ⟹ **`S ≥ 25/w`**

   | record span `w` | required `S` | feasible? |
   |---:|---:|---|
   | 8 B | 312% | **impossible** |
   | 16 B | 156% | **impossible** |
   | 32 B | 78% | only at implausible savings |
   | 64 B | 39% | marginal |
   | 256 B | 9.8% | comfortable |
   | 1024 B | 2.4% | free |

   **No record span narrower than ~64 B can satisfy the cap at a realistic ≤50% saving rate.** That excludes exactly the interleaved mixed-binary population that would make track 13 a *general-purpose* compressor, and admits exactly the wide-record / contiguous-column regime.
3. **On the two cells actually projected the cap is never reached — because there are no savings.** On `synth-arith` the corrected projection (17–24 KB) is at or above the 16,313 B bar; on `synth-timeseries` the projection (104,000 B) is **16.1% above** the 89,564 B bar. Metadata/savings is undefined or negative, so a cap cannot be evaluated. **The cap only bites on real files — where it excludes everything narrower than ~64 B.**
4. **This closes a loop with the prior-art map.** The only admissible geometry — long contiguous runs — is the geometry Parquet/ORC/HDF5 carry a **schema** for, and those formats amortise descriptors **per file, O(R)**. CANL's own cost table concedes this (`:303`). So the metadata cap and the prior-art map independently select the *same* narrow regime: schema-carrying columnar formats. There, ALP / FastLanes / BtrBlocks / LeCo are established and the schema already exists — so the mechanism's marginal value over simply picking a codec per column is a **planner** question, which G2 already measured as its cost sink (0.51–2.68 s local scoring; 6.8–121 s marginal search; planner losing to dense composition on both real families).

**Required action:** add `metadata + codebook bytes ≤ 20% of gross savings` as a `PROMOTE-CANL` gate, **and** report per-file lane geometry, **and** record the ≥64 B scope restriction as binding.

### 12.5 MANDATED REQUIREMENT — binding specification for the new locked numeric family

I agree with the constructive lane's blocking-dependency statement (`:336`) and strengthen it into a hard precondition. **No promotion claim of any kind, and no PILOT of any mechanism, until all of the following hold.**

1. **≥3 real, non-synthetic, independently sourced numeric binary files**, SHA-256-pinned in a frozen manifest, locked under `docs/I10-CORPUS-LOCK-PROTOCOL.md`, **frozen before any prototype byte is written** (G2's firewall precedent: V1 stayed unopened, `I10-GROTLI-G2-RESULTS.md:100-107`).
2. **Designed to answer the deciding question, not merely "three numeric files".** The verdict turns on `A3 (columnar) ≤ A2 (in-op-stream)` (constructive `:359`, `:437`). So the family must contain by construction:
   - ≥1 **contiguous columnar** file (schema-present; the only geometry the metadata cap admits, §12.4); **and**
   - ≥1 **row-interleaved / mixed-width** file (the regime Brotli and ANVIL actually serve); **and**
   - ≥1 **drifting-stride** file (SB's own §13.3 flags this as unknown, and the project's detector is measured at 0.01% hit rate on a 14 B mixed-width file — `RESEARCH_LEDGER.md:2333-2334`).
3. **≥1 file where ANVIL's current pipeline is *not* store-class**, so the delta cannot be inflated by the store anomaly (§12.3 count 4).
4. **≥1 file whose float column is a smooth trend**, so the float half of the mechanism is actually exercised — `synth-telemetry-f64.bin` was built for this and has **never been measured** (`tests/benchmark-recon-i9.md:69-70`; constructive `:81`, `:438`).
5. **`sao` is calibration, not held-out.** It is already in the landscape grid and must not be burned as the discovery set.
6. **The decimal-text regime must be explicitly declared in or out.** The constructive report flags it as "the higher-EV numeric sub-question" (`:440`) but excludes it from scope. Either it becomes a separate, separately pre-registered track, or the scope statement says numeric-binary-only. Ambiguity here is how a track silently changes population between runs.
7. **The 16,313 B `brotli-q11+delta` row must be reproduced in the frozen grid** before it is used as a bar (§12.6 U2); until reproduced it is treated as *unverified*.

### 12.6 Control upgrades I require over `E13-1` (adopt-or-reject, decided now)

| ID | Required control | Why |
|---|---|---|
| **U1** | **Oracle dual bar**: `min` over all `d ∈ 1..256` of `xz -9e --delta=dist=d`, **plus** `brotli-q11` on the delta+zigzag stream, **plus** the 8-way byte-plane split — per file. The detector must **not** be inside the bar. | The project's detector is measured at 2.57% / 0.01% hit rate on the very cells the bar must be computed for (`:107`). A bar at a mis-detected `d` is not a bar. |
| **U2** | **Reproduce 16,313 B** in the frozen grid and report it as a first-class reference row. | It is the binding bar per `gate-verdict-i8-ari-stride.md:261-265`, and it is currently citable-but-unreproduced. |
| **U3** | **Report projections as ranges with both unmeasured terms named** (payload entropy 1.6–2.4 bits/value; mode cost 0–0.75 bits/value depending on the hoist decision), never as point estimates. | §12.3 counts 2 and 3. A single `0.051` conceals two unmeasured quantities. |
| **U4** | Add `metadata + codebook ≤ 20% of gross savings` as a gate; report lane geometry per file; record the ≥64 B scope restriction. | §12.4. |
| **U5** | Fix `w ∈ {2,4,8}` or drop the u16 row; validate `step` on the wire; restate re-entry as `M_RAW` with an explicit charge. | D6, D7, D8. |
| **U6** | Add a `brotli-q11` **+ ALP-equivalent** arm. | Without ALP the dual bar is `xz --delta` vs Gorilla, and ALP is the occupant that has already amortised the framing the report itself credits (`:235`). |
| **U7** | Stage-1 must **measure** `synth-telemetry-f64.bin` and `synth-ndjson-columnar.ndjson` before any held-out byte is touched; those rows are diagnostics only. | D13 — the cheapest high-value measurement in the track. |
| **U8** | Freeze an explicit **KILL on the novelty claim independent of results**: if `A2 ≥ A3` on any 2 of 3 held-out files, CANL is adopt-class engineering with **no** further novelty spend, regardless of bytes. | §6.2 already bounds the novelty at ~0.004 B/value; U8 makes that bound enforceable rather than advisory. |

I adopt the `PROMOTE-CANL` / `KILL-CANL` / `HOLD-CANL` structure as written (`:355-372`) — it is well-formed, it separates operator value from adoption, and its clause 8 ("at least one held-out family carries the claim") is the right firewall. With U1–U8 folded in, the gates are sound. **The gates are not the problem. The recommendation attached to them is.**

### 12.7 Where the constructive lane is better than my interim (recorded, no qualifier)

1. **It names its own novelty surface precisely** (`:19, :282`) and then bounds it at ~0.004 B/value (`:290`). My interim reached "novelty occupied" by prior-art mapping; it did not produce the *quantitative* bound on the residual component. That is strictly better, and I adopt it as the basis of D1/§12.2(c).
2. **The ALP framing-amortisation corollary (§5.2, `:228-235`)** — ALP's per-value framing term is ~0.02–0.03 bits/value, so "our lane wastes bits on per-value control" is false by arithmetic — is a result I did not have. It kills an entire class of would-be novelty claims before they are written, and it *strengthens* my §4.1 ruling.
3. **The `M_XOR`-loses-on-a-random-walk-f32 prediction** (`:220`, `:370`) is a sharp falsifiable self-test: if the selector picks `M_XOR` for that lane, the cost model is wrong and every downstream number is untrustworthy. Good experimental hygiene, and it is a real discriminator.
4. **The float exactness restriction** (v1: `M_VALDELTA` on `U*`/`I*` only; `F*` gets `M_RAW`/`M_XOR`) (`:139`, `:380`) is the correct call and pre-empts the NaN / ±0 / Inf / overflow involution failure class.
5. **The corpus-lock dependency, stated as blocking rather than assumed** (`:336`), and the refusal to substitute `synth-*` for a held-out family, is exactly the discipline I demanded — met before I asked for it.
6. **§11.6** — "the rebase advantage is a problem **ANVIL creates for itself**" — is the sharpest self-disconfirmation in the set, and it is the sentence that decides this reconciliation.

### 12.8 Post-reconciliation verdict

Direction unchanged from my interim, now **better supported on the mechanism side and better bounded on the corpus side**:

- **CANL as a novelty mechanism: KILL.** Placement engineering (D1, D2, §12.2); every component occupied (my §2 — which the constructive §6.1 independently agrees with, marking nine rows "FATAL — closed"); residual component bounded at ~0.004 B/value by its own analysis; the one cell it projects on already carries a binding measured transform-enabled bar it does not clear (§12.3).
- **CANL as adopt-class engineering: HOLD.** Not KILL — the capability gap is real (Experiment W: no AP token type; `synth-arith` ANVIL 1.000 vs `xz-9e` 0.310) and the decode-side profile is favourable (measured 2.75 GB/s; 0.146 ns/B vXOR). Not PILOT — because §12.3 and §12.5 show the deciding evidence cannot be obtained without a locked real corpus that does not exist, and because the pilot's own byte rationale is struck.
- **PROMOTE-TO-REMOTE — the only thing actually dispatchable now:** the **reference-only** workflow: (i) the locked numeric family (§12.5); (ii) the **oracle** dual-bar sweep + `brotli-q11+delta` reproduction (U1, U2); (iii) Stage-1 measurement of the two never-measured sweet-spot files (U7). **No CANL prototype, no `src/anvil.cpp` mode, no `FORMAT.md` change, no detector constant touched.** Remote-only, no ANVIL source change, and the only action that can move this track on evidence rather than projection.

**Blocking dependency, unchanged and now doubly sourced:** a new locked real numeric family (`RESEARCH_LEDGER.md:4831-4833`; constructive `:336`). Until it exists, every quantitative statement about this track — mine included — is a projection.

---

## 13. Recommendation

### 13.1 Verdict

> **FINAL, post-reconciliation (§12.8). Unchanged in direction from INTERIM.**
>
> ## **HOLD** — split ruling:
> **(a) KILL** the typed-lane compiler *as a novelty mechanism* (novelty is
> occupied; the interaction is already adjudicated closed as PORDER).
> **(b) KILL** the global de-interleave placement (measured decisive negative) and
> the inside-the-reference placement (datastruct-ts: NOVELTY NO).
> **(c) HOLD** an **adopt-class, dual-bar'd, reference-only numeric lane** —
> Alt-A — as a measurement instrument, gated entirely on E13-NUMDUAL, with a
> pre-committed conversion to KILL if no real numeric corpus can be locked.

**Not PILOT:** nothing has passed a gate, and the corpus precondition
(`RESEARCH_LEDGER.md:4831-4833`) is unmet, so a pilot would have to run on
synthetic data — which is precisely the failure mode this document exists to
prevent.

**Not PROMOTE-TO-REMOTE:** there is no mechanism to promote. What *is* remote-ready
is the **experiment**, not the codec: E13-NUMDUAL (§10) is fully specified,
frozen, GitHub-Actions-only, and needs no ANVIL source change. If the coordinator
wants a dispatchable artifact now, authorise a workflow that runs **A1/A2/A4/A5
only** — the reference grid row — and defer A6 entirely.

**Not full KILL:** the coverage gap is measured (`sao` = 24.2% of the identified
gap; no ANVIL mechanism targets binary float records), the decode-side profile is
unusually good (2.75–6.86 GB/s), the market has independently adopted the family
(Parquet/ALP), and the dual-bar reference row is genuinely missing from the
project's measurement apparatus. Killing outright would leave a known hole in the
grid and would leave future numeric claims free to produce FRONT-GAP illusions.

### 13.2 Quantified bottom line

- **Bytes:** the mechanism's entire survival budget is
  `bytes(A6) ≤ 0.97 × min(xz --delta, Gorilla, ALP)` on ≥2/3 real files, with
  metadata ≤20% of gross savings — and §5.1 shows the metadata condition is
  **unreachable for interleaved mixed binary at lane width ≤16 B** (needs ≥62.5%
  savings at `m=2`) and **impossible at `w=8`**. Contiguous columns only; i.e.
  schema-carrying formats only.
- **Cycles:** decode is a non-issue (2.91–7.68 ns/val; 0.146 ns/B vXOR). Encode
  planner is the risk, with a measured adverse precedent (G2: 0.5–2.7 s local
  scoring, 6.8–121 s marginal search, planner losing to dense composition on both
  real families).
- **Memory:** decoder state is small; RSS is a non-issue for the mechanism, but the
  project's RSS instrument is itself unclosed (G5D zero placeholder) and must be
  fixed before any memory gate is credible.
- **Prevalence:** ≤24.2% of the identified 3rd-backend gap, in a single file, in a
  family the project's own audit already graded "MEDIUM, small" and assigned to a
  stride/delta probe.

### 13.3 Explicit falsification criteria (this review is falsified if…)

1. A **real**, locked, SHA-256-pinned numeric binary corpus with ≥3 files is
   produced ⇒ §4.2's prevalence kill weakens and the verdict moves to PILOT.
2. A lane representation is shown to beat `min(xz --delta, Gorilla, ALP)` by
   **≥3% complete bytes on ≥2/3 real files** at ≥1.0 GB/s decode, RSS ≤1.25×, and
   metadata ≤20% of gross savings ⇒ §5.1's structural conflict is wrong about the
   population, and the mechanism deserves a promotion lane.
3. A **pre-existing prior-art ruling** is shown not to cover the lane/width/expression
   interaction — i.e. PORDER's kill is narrower than `RESEARCH_LEDGER.md:4824-4829`
   states ⇒ §4.1 weakens.
4. The project's own G2 integer-leaf result is shown not to generalise to binary
   f64 **and** a binary-float corpus is locked ⇒ §4.4 weakens.

None of these are currently satisfied. Statement (1) is the cheapest to satisfy
and the most likely to change this verdict — it is the single thing the
coordinator should decide next.

**Reconciliation effect on these criteria (§12.2–§12.4):**

- Criterion (2) is *strengthened*: the required margin is now stated against a
  **measured** transform-enabled bar (16,313 B on `synth-arith`; 89,564 B for
  `xz -9e --delta=dist=14` on `synth-timeseries`), not against raw Brotli — and
  the constructive mechanism's own projections fail both.
- Criterion (3) is *withdrawn as stated*: I no longer contest that the
  lane/expression interaction is uncovered; I contest that covering it is
  **claimable**, because the project's own placement rulings
  (`RESEARCH_LEDGER.md:4609`, `:4614`) classify inside-the-reference placement
  as satisfied engineering rather than a novel interaction.
- Criterion (4) is *demoted*: I withdraw G2 as a primary argument, per D12 — it
  is secondary evidence only.

---

*Prepared by Fledge Alpha Free, adversarial reviewer, track 13. RECONCILED against
`13-numeric-compiler-space-bunny.md` under §12 (13 disagreements; §1–11 unchanged
from the INTERIM freeze). No existing
file was modified; no commit, push, reset, clean, stash, restore or rebase was
performed; no local corpus or performance benchmark was executed.*