# Track 12 — Fledge Alpha Free: Independent Adversarial Audit
# Encoder-only search/learning — zero-decoder-ML ratio gains

**Agent:** Fledge Alpha Free (independent adversarial reviewer / validator)
**Swarm:** `docs/swarm-2026-10-02`, track `12-encoder-search`
**Date:** 2026-10-02
**Live worktree:** `i10-aux-unbwt` at `b8eae11`, intentionally dirty. Untouched.
**Artifacts written:** this file only. No prototype was needed (see §12 — every
load-bearing test is a byte decomposition against already-frozen artifacts, which
by doctrine cannot run locally).

**Paired constructive report:** `docs/swarm-2026-10-02/12-encoder-search-space-bunny.md`
("PRA-1", Admissible-Bound Exact Arbitration). **Read in full before writing this
audit.** Reconciliation is §10.

**Standing of this document:** INTERIM-COMPLETE. The constructive lane's
`SELREG-1` has not run. Sections marked `[OPEN]` are the only places where a later
pass could change the verdict, and each names the artifact that would settle it.

---

## 0. Evidence labelling and the one provenance rule I impose

Every number below carries one of:

- **`[M]` MEASURED** — recomputable from a named committed artifact + command.
- **`[D]` DERIVED** — arithmetic on `[M]` values, same run, no new measurement.
- **`[P]` PROJECTED** — hypothesis. Never promotable.
- **`[Q]` QUESTIONED** — a value asserted in a source I could not trace to a
  specific arm/carrier/backend/window. Excluded from every load-bearing argument.

**Binding provenance rule for this track (per coordinator):** the V1 figures in
circulation are *different quantities* until an artifact proves equivalence. I
therefore trace each to its arm, and I use **only** the regression whose byte
components I can causally attribute. I do not reconcile them by intuition.

---

## 1. The mechanism under audit, in one sentence

PRA-1: the encoder evaluates the **exact** complete charged byte cost of every
admissible representation assignment over a **frozen** option set, prunes with a
**one-sided admissible bound** that costs zero backend calls, and emits the
argmin — producing a wire that is **byte-identical in grammar, field set, field
count, width classes, and decoder code path** to the incumbent's, so the only
bytes that move are already-charged selector bytes
(`12-encoder-search-space-bunny.md` §1, §2).

The strongest claim in the constructive report, stated at its best, is this:

> **The project's frozen G5A carrier already charges exactly one selector byte per
> record from a 32-value alphabet. That cost is already paid. PRA-1's claim is
> that the existing charge currently buys a *heuristic guess* and can be made to
> buy the *exact answer* for free at the decoder — and that a one-sided
> (admissible) bound makes the exact search affordable, unlike the failed G4 lane
> whose error was two-sided.** `[M]` for the charge (G5A-LC prereg §4.1, §8);
> `[P]` for the recoverable prize.

I take that claim seriously enough to spend this audit on it rather than on a
strawman. But I do **not** accept its load-bearing premise, and §6 is why.

---

## 2. What the mandate asked, answered

| Mandate item | Answer | Where |
|---|---|---|
| Hidden decoder model transfer | **None found**, and this is PRA-1's genuine strength — *conditional on Theorem Z holding*, which is itself unverified (§5.3) | §5 |
| Search overfit | **Present and unaddressed.** The regressor is the *incumbent heuristic's* errors, and D1–D4 is the population the incumbent was tuned on | §6.1 |
| Objective leakage | **Real and specific.** The exact objective is defined in terms of Brotli q11/lgwin30 *complete bytes on the extracted carrier*. That is a leakage-exposed objective, not the deployment objective | §6.2 |
| Enormous encode cost | **Overcharged by the constructive report.** Against its own cited M17, `beta = 0.02–0.05` is ~**48x–120x** over the measured headroom | §7.1 |
| Candidate explosion | **Bounded by construction, and that is a genuine win** — `E ≤ floor(beta·n)` beats G4's `candidates_enumerated` (M10) | §7.2 |
| Corpus memorization | **Two distinct vectors**: (a) population-level memorization, §6.1; (b) *selector-level* memorization, which is the sharpest finding in this audit — §8.3 | §8.3 |
| Evidence needed for a generalizable zero-decoder-ML win | Pre-registered here: §11 | §11 |

---

## 3. Current evidence base (only what this track may cite)

| # | Fact | Value | Tag | Source |
|---|---|---|---|---|
| E1 | Canonical frontier | `33 non-dominated \| 5 FRONT-GAP \| **0 FRONT-CROSSING** \| 28 DEGENERATE \| 435/468 dominated`; `GRID-THIN` binding | `[M]` | `RESEARCH_LEDGER.md` PART XV §"Established current facts" 1 |
| E2 | G4 planner-fidelity outcome | `NO-GO-G4`. Overall worst-file regret **6.6056% / 7.0802% / 1.5388%** (S0/S1/S2) | `[M]` | PART XV §G4; `docs/I10-GROTLI-G4-RESULTS.md` §2 |
| E3 | G4 speed column **withdrawn** | `INVALID_SPEED_ACCOUNTING` — the O11 denominator read cached ranking labels while q11 label construction was timed separately (**667.948690 ms vs 0.004758 ms** on D1) | `[M]` | PART XV §G4 |
| E4 | G4 frozen fidelity bars | `aggregate_regret ≤ 0.0025` (0.25%); `per_file_regret ≤ 0.0050` (0.50%) | `[M]` | `I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md` §G8.2/§G8.3 |
| E5 | G4 call budget | `q11_rank(O11) = candidates_enumerated` — unbounded in candidates | `[M]` | same §G8.5 |
| E6 | G5A assignment is first-order *on one fixed charged multiset, one backend* | A0/A1/A2/A3 = **2,054,532 / 1,470,205 / 1,452,383 / 1,380,245 B**; A3−A1 = **89,960 B**; A3 smaller on **4/4**; `COLUMN-DOMINANT` | `[M]` | PART XV §G5A |
| E7 | Selector is already charged | one outer selector byte per record, 32 frozen selector values, charged in every arm; `complete_bytes = 1 + Brotli_q11_lgwin30(body).size()` | `[M]` | `I10-G5A-LOCALITY-CONTROLS-PREREG.md` §4.1, §8 |
| E8 | Same-multiset is a hard invariant | `I5` byte-level chunk multiset identity; `I14` complete accounting; fail-closed `I2`–`I15` | `[M]` | same §5 |
| E9 | G5A V1 reversal | A3 exceeded A1 by **1,721 B**; V1 is `V1-COLUMN-ADVERSE` | `[M]` | `I10-G5A-LOCALITY-CONTROLS-PREREG.md` §0 |
| E10 | G3 V1 regionized regression | 2,179,615 B vs 2,112,235 B raw = **+3.1900% worse**; 11,444 frames, **100% structured, 0 residual frames, 3 exact shapes** | `[M]` | `docs/I10-GROTLI-G3-RESULTS.md` §3; `docs/I10-FRONTIER-RECON-2026-09-24.md` §1 |
| E11 | The G5A A0 permutation is not a significance distribution | explicitly recorded as "a null draw, not a significance distribution" | `[M]` | `docs/I10-FRONTIER-RECON-2026-09-24.md` §1 Class C |
| E12 | **Live rate surplus vs the same-backend reference** | Silesia ANVIL **46,466,339 B** vs Brotli **49,383,136 B**, xz **48,456,004 B** | `[M]` diagnostic | PART XV (run `36061511123`) |
| E13 | E12's status | `Silesia FRONT-GAP_COST`; enwik8 `TIMING_BLOCKED` (control CV 0.1756); **diagnostic only**, no per-run output hashes → not a new Pareto claim | `[M]` | same |
| E14 | Extract-carrier penalty vs *raw* Brotli (Class A, 13-file) | ANVIL extracted **1,470,205 B** vs aggregate raw Brotli **1,466,769 B** ⇒ **+3,436 B** | `[M]` + `[D]` | PART XV §G5A arithmetic corrections |
| E15 | G3 V1 had **no raw residual frames** | "V1 contained no raw residual frames, so this is [not] a coverage failure" | `[M]` | `I10-GROTLI-G3-RESULTS.md` §3 |
| E16 | BWT warmup tax closes block-local routing | webster wins 10/10 blocks; sum-of-blocks 8,211,644 vs whole-file 7,317,329 = **+894,315 B (+12.22%)** | `[M]` | `06-do-not-reburn.md` §G1 |
| E17 | QLFC / LZP postcoders | +3.23% / +0.37%; 0/13 and 4/13 file wins | `[M]` | same §G2 |
| E18 | Statistical zero-bit derived parameters | `DNB-M2`, **MATH-class**, closed | `[M]` | PART XIII §7 |
| E19 | G5B ordinal placement | `ORDINAL-ADVERSE`: B2 **+1.6507%** vs B1, wins 1/4 | `[M]` | PART XV §G5B-ORDINAL |
| E20 | G4 separability is *not* the blocker | `SEPARABLE-ENOUGH`; max pair gain 0.277632%, aggregate 0.057144% | `[M]` | `I10-GROTLI-G4-RESULTS.md` §3 |
| E21 | No new held-out structured corpus exists | "Promotion requires new locked independent structured, mixed-validity, executable, and numeric families" | `[M]` | PART XV §Portfolio, final bullet |
| E22 | Prior-art audit incomplete | arXiv 429 ×3; **ACM/IEEE/USPTO never queried**; Espacenet/lens.org never queried; no post-2024 survey | `[M]` | `docs/gate-priorart-audit-i8.md` §5 |
| E23 | G5A-LC vehicle unfrozen locally | not committed; `tools/grotli_g3.cpp` local copy is **not an admissible substitute** | `[M]` | `I10-G5A-LOCALITY-CONTROLS-PREREG.md` §1.2 |
| E24 | Decode-only crossings closed | 12/13 files; byte-identical decode ceiling ~1.79x | `[M]` | PART XIII §3 (DNB-M1, MATH-class) |

---

## 4. The provenance conflict I refuse to reconcile by intuition

The coordinator is right that the V1/G5A figures in circulation are not one
quantity. Traced:

| Figure | Arm | Carrier | Backend | Comparison | Tag |
|---|---|---|---|---|---|
| **+3.1900%** (E10) | G3 `REGION_DICT`/`REGION_MIXED` best | G3 regionized | Brotli q11/lgwin30 **inside the extracted carrier** + rev-2 framing | vs **raw whole-file Brotli** 2,112,235 B | `[M]` |
| **+1,721 B** (E9) | G5A **A3 vs A1** | G5A ordering, D-carrier semantics | same backend, same carrier | vs **another ordering of the same charged multiset** | `[M]` |
| **+3,436 B** (E14) | G5A **A1 vs aggregate raw Brotli** | extracted-chunk source order | same | vs **raw** Brotli, Class A | `[M]`+`[D]` |
| **+70,030 B** `[Q]` | "A1 already 70,030 B worse than raw complete Brotli" | — | — | — | **UNTRACED** |

**Finding P-1 (load-bearing).** `+3.1900%` (E10) and `+3,436 B` (E14) are
**different quantities**: E10 is a G3 *representation-regime* result on a
100%-structured V1 file; E14 is a G5A *ordering* result on the D-family. They are
near-identical in magnitude (67,380 B vs 3,436 B differ by 20x) which is exactly
why conflating them is tempting and exactly why it is a provenance error. E10 is
the **only one** that is a pure representation result on a fully-structured file.

**Finding P-2 (load-bearing).** E10 and E15 together are decisive for the
coordinator's test (1). V1 had **no raw residual frames**. If no leaf was ever
*declined*, then **no selector ever selected RAW**, so no "raw wins exact ties"
branch could have fired, so the selector stream's value distribution was
degenerate on V1. Consequently:

> **A regression observed on V1 is a regression of the *representation regime
> itself*, not of the *selector choice*, and not of selector/framing overhead.**
> The framing/selector term on V1 was `R` records × 1 byte and nothing selected
> RAW; it cannot be both negligible and decisive.

**Finding P-3 (load-bearing).** Therefore **H-12b is not a repair claim and must
never be written as one.** There is **no measured byte decomposition** anywhere
in this project that splits a V1-class regression into (i) selector bytes,
(ii) envelope/frame-to-group-map/group-descriptor/shape-ID overhead, (iii) the
Brotil-compressed body, and (iv) loss of whole-file Brotli prefix locality. I
searched the G5A-LC prereg, G3/G5A results, and PART XV; the G5A-LC design
*forbids* charging-free components (§4.4, §4.5) and *demands* `I14` complete
accounting, but no `I14` accounting artifact has been produced.
**`[OPEN]`** — the artifact that settles it is a per-component byte breakdown of
one G3 V1 carrier, which is cheap and remote-only (§11 `KB-0`).

---

## 5. Adversarial audit of the mechanism itself

### 5.1 Claim (a): "the objective is evaluated, never predicted" — **holds, and is the real contribution**

Cost is `complete_charged_bytes(B, G, f, s)` with `B` the actual backend. No
surrogate, no learned score, no fitted coefficient. This is the true and
meaningful difference from G4 (E2/E3) and from the promoted G5 planner, whose
objective `y = log2(max(1, o11_isolated_q11_bytes))` is **external** and observed
only by running the backend.

**Audit finding A-1 (favourable).** PRA-1's failure mode is *structurally
unavailable* relative to G4. G4 died because a two-sided regression could prune a
winner. A one-sided admissible bound cannot do that, provided the bound is
admissible. PRA-1 is the correct repair to the specific defect that killed G4.
I grant this without reservation.

**Audit finding A-2 (against, technical).** `LB(f,s) = framing_min(f,s) + 1 +
max(0, ceil(H0(payload_f(s))))` (§1(b)). The `+1` is the selector. But
`H0` is the order-0 **ideal** code length of the payload *alone*. The real cost is
`Brotli_q11(body)`, whose body contains literals, distance streams, and command
streams **whose cost depends on the ordering of the payload**, not on the payload's
own order-0 histogram. So:

- ~~the bound is **admissible** (never over-estimates cost) — yes, sound;~~
  **WITHDRAWN 2026-10-02 - SEE APPENDIX 1.** `H0` is **not** a lower bound on
  complete Brotli q11 output. The prune is unsound; PRA-1's exactness claim is
  void. Appendix 1 is the formal correctness kill and it supersedes this bullet.
- but it is **near-useless whenever `framing_min + 1` is large relative to
  `U(s)`**, because then every `LB ≥ U` and *every* leaf is pruned and the arm
  degenerates to the incumbent. Conversely when framing is cheap the prune does
  almost no work, because all leaves on a record-structured chunk tend to have
  similar `H0`. **This "near-useless" observation was correct but understated:
  the real problem is worse — the bound is not merely weak, it is wrong (§17).**

**Consequence:** the bound's informative window is narrow, and `A_greedy_h0`
(the constructive report's own control arm, §7) is the arm that tests it. PRA-1's
own §7.1 correctly identifies this. **I endorse the control arm; I do not endorse
the prior probability that it will be informative.**

### 5.2 Claim (b): budget — **the one genuinely novel bit, but see §7.1 for the dose**

`E ≤ floor(beta·n)` denominator in **input bytes** is a real structural
improvement over E5's `candidates_enumerated`. Denominating in `n` makes cost a
fixed fraction of one pass, independent of `R`, `S`, `K`, and content. I accept
this as sound and as the most defensible engineering idea in the report.

### 5.3 Claim (c): Theorem Z, zero decoder delta — **structurally sound, but stated too strongly**

Theorem Z's five clauses follow from grammar freeze + alphabet freeze, and I
verified the argument: the decoder reads the same fields at the same widths and
dispatches through a table that already exists (E7).

**Audit finding A-3 (against, overstatement).** Clause 5 says decoder cycles are
"bit-identical." That is **not** established and is not derivable from field
identity. Dispatch cost of a 32-entry table indexed by a *changed* value is not
identical to dispatch of a *different* value — branch predictor training, I-cache
and BTB pressure all differ with the value mix, and the selector *distribution*
changes when the incumbent heuristic is replaced by an exact argmin.

> **Corrected claim: `Δ decoder text/rodata = 0 B`, `Δ decoder per-record
> algorithmic work = 0`, and `Δ decoder cycles = bounded by second-order
> microarchitectural effects, not proven zero`.**

This is not pedantry. `RESEARCH_LEDGER.md` PART XIII §6 claim-hygiene and the
project's own `RANKING-GRADE`/`CITATION-GRADE` regime (E3 is the precedent: a
*timing* claim was withdrawn for exactly this class of reason) require the weaker
statement. **The constructive report should strike "bit-identical cycles" from
§2.1 clause 5 and Theorem Z's Δcycle line.**

### 5.4 The excluded components — **I disagree with two of the three exclusions**

| Component | Constructive verdict | My verdict |
|---|---|---|
| Run-length coding of the selector stream | excluded: needs new opcodes ⇒ decoder cost | **Agree on mechanism, disagree on framing** — see §8.4, this is the *highest-value* follow-up and it is the one that makes selector bytes matter |
| Static decode-cost tie-break | excluded: needs transmitted tag | **Agree, and I add the decisive reason**: see §12 `KB-3` — it is *already* falsified by E16/E17 |
| Adding a representation family | excluded: decoder text/rodata | **Agree** |

**Audit finding A-4 (favourable, and under-credited by the constructive report).**
The decision to exclude all three is what keeps PRA-1 an *encoder-only* mechanism
in the strict sense. Track 18's own instrumentation work (`18-future-decoder-architecture-space-bunny.md`)
reports **2.3–8.1%** perturbation from added instrumentation, and the project's
`source` vs `FORMAT.md` constant-drift hazard is a live, repeated failure class.
A mechanism whose entire value proposition is "we changed *nothing* the decoder
sees" cannot afford a 2.3% perturbation on its own verification axis. **The
boring option set is not timidity; it is load-bearing.** I credit this.

---

## 6. The two mechanisms by which PRA-1 can fail to generalize

### 6.1 Population-level memorization — **the structural objection**

**The regressor is the incumbent heuristic's own errors, and D1–D4 is the
population on which that heuristic was chosen.**

- E6 shows the assignment decision is first-order **on D1–D4, one backend, one
  fixed charged multiset**. That is a within-population ordering result.
- E11 records that G5A's A0 arm was "a null draw, not a significance
  distribution" — i.e. the project itself already declined to treat the D-family
  ordering spread as a sampling result.
- E9 records that the *same frozen machinery* reversed sign on V1 (A3 > A1).
- E10 records that a *different* frozen regime regressed **+3.1900%** on the same
  V1 file.

So the pattern across three independent arms is: **sign flips between D-family and
V1, twice.** PRA-1's prize is precisely the size of the gap between an incumbent
heuristic and the exact argmin **on D1–D4**. If that gap is population-specific —
and three recorded sign flips say it plausibly is — then `sel_regret(D1–D4)` will
overstate held-out recovery, and the promotion thresholds GO-1/GO-2 (§7.2 of the
constructive report), which are **all defined on the discovery population**, are
**not** generalization evidence.

> **Audit finding A-5 (against, decisive for the gate design).** GO-1/GO-2 as
> written measure `sel_regret` on D1–D4. That is a **discovery** measurement.
> The constructive report's own §12 correctly refuses promotion, but §7.2's
> promotion thresholds are stated in discovery-population terms, which invites a
> later reader to mistake a discovery pass for generalization. **The kill test may
> run on D1–D4; the promotion test may not.**

### 6.2 Objective leakage — **the sharper and under-appreciated objection**

The exact objective is
`complete_charged_bytes(Brotli q11/lgwin30, G, f, s)` — **Brotli q11 bytes of the
extracted carrier**.

This is not a neutral "true cost." It is a **specific leakage-exposed cost**:

1. **It is measured on the extracted carrier, not on the deliverable.** E14 shows
   extraction already costs +3,436 B against raw Brotli on the D-family; E10 shows
   +3.1900% on V1. If the routing decision is optimized against post-extraction
   q11 bytes, the argmin is *by construction* indifferent to the extraction tax
   that dominates the real-world outcome. **PRA-1 optimizes a term that is
   provably smaller than the term it is being blamed for.**
2. **It inherits G3's own blind spot.** `raw wins exact ties` is *not* the
   reference's tie-break (E16: block-local BWT wins 10/10 blocks yet whole-file is
   894,315 B better; E17: QLFC wins 0/13 and LZP 4/13). A per-record q11-bytes
   argmin inherits the per-record-is-a-good-unit assumption that the project's own
   negatives falsify.
3. **It spends bytes that are worth different amounts.** One selector byte saved on
   a record whose body is 200 B is worth ~0.4%; the same byte on a record whose
   body is 2,000 B is worth ~0.05%. A uniform `argmin` optimizes the numerator and
   ignores the marginal value of the denominator. This is not fixed by `I5`
   (multiset identity is about *content*, not about *marginal rate*).

> **Audit finding A-6 (against).** The exact argmin is exact **with respect to a
> proxy objective that the project has independent measured reason to distrust.**
> PRA-1's claim (a) — "evaluated, never predicted" — is true and is *not* the
> same claim as "fidelity to the deployment objective." G4 failed on the second;
> PRA-1 has only fixed the first.

### 6.3 Where PRA-1 genuinely beats every alternative — **stated in its favour**

For completeness, and because a critic who only attacks is not a validator: given
a **frozen carrier**, a **frozen option set**, and a **frozen backend**, PRA-1's
argmin is exactly optimal for its stated objective, with no fitted parameters and
no corpus leakage, and its cost is bounded by a hard ceiling. **No** planner,
proxy, learner, or router in this project's history has that property. G4 needed
a corpus-fitted ridge and still missed fidelity by 13–27x. That is a real,
structural difference and I do not want it discounted.

---

## 7. Hidden-cost audit

### 7.1 Encode cost — **the constructive report's budget is 48x–120x over the measured headroom**

This is my most quantitative finding.

**Measured reference points on the binding bar (E12/E13), same run:**
Silesia ANVIL (BWT+aux-unbwt) **46,466,339 B** vs Brotli q11 **49,383,136 B**.
The whole *byte* surplus vs the reference is **2,916,797 B = 5.91% of Brotli**.

**Encode-speed reference points** (same class of evidence, directional):
- Project's Class A grid `[D]` from `RESEARCH_LEDGER.md` PART XV verification
  addendum: brotli q11 aggregate encode **0.681 MB/s**, xz-9e **1.310 MB/s**,
  brotli q1 **432.106 MB/s**.
- E16: BWT-direct whole-file webster **9.22 s** for 7,317,329 B ⇒ **0.794 MB/s**.
- E13: on one BWT-won file, auto 30.42 s vs bwt 9.22 s — a **3.30x** encode
  multiplier *for one extra whole-backend candidate evaluation*, and the file
  measured **126 MiB (BWT) → 10,318 MiB (auto)** peak RSS.

**Arithmetic.**

```
byte surplus available to any mechanism        = 2,916,797 B  (5.91% of Brotli)
per whole-backend-pass multiplier (measured)    = 3.30x          (E13)
=> max defensible beta for a q11-pass-on-extracted-chunks search:

   beta_max = 0.0591 / 3.30  ~=  0.018
```

The constructive report's P3 targets **`beta ≈ 0.02–0.05`** — i.e. **1.1x to 2.8x
over the entire measured headroom**, and the upper end is a **51–83% encode-time
regression against the reference it must not slow down**.

```[D]``` arithmetic: `(49,383,136 - 46,466,339) / 49,383,136 = 5.9073%`.
Subtracting: `5.9073 / 3.30 = 1.7901%` = `beta_max ≈ 0.0179`.

> **Audit finding A-7 (against, quantitative).** `beta = 0.02–0.05` is not
> "medium confidence"; it is **affordably impossible** against E12/E13. The
> correct pre-registered ceiling is **`beta ≤ 0.015`**, or the mechanism must be
> re-scoped to *not* pay a full q11 pass per candidate.

**Caveat, stated honestly:** E12/E13 are `TIMING_BLOCKED`-class diagnostic
evidence, so the 3.30x multiplier is **directional, not citation-grade**. That
weakens the *precision* of `beta_max ≈ 0.0179`; it does not weaken the *sign*,
because the order of magnitude (single-digit percent of a 3.3x-cost pass) is far
below `0.02–0.05`. **I flag this as the one place where my own arithmetic should
be re-derived on citation-grade timing before it is used to set a frozen gate.**

### 7.2 Candidate explosion — **genuinely bounded (accept)**

`E ≤ floor(beta·n)` beats E5's `candidates_enumerated`. The `Theta(n·K)` histogram
term is real but cheap: one 256-bin pass per option per segment. **Memory
discipline is correct and explicitly enforced**: `O(256)` scratch, `S×K` score
table prohibited (at `S = 10^6` that is ~48 MB). I endorse this as specified.

### 7.3 Cost items the constructive report does **not** charge

| Uncharged item | Why it matters | Tag |
|---|---|---|
| Payload materialization at `O(len)` per (leaf, segment) | §3.1 `ENC_2` says "with `payload_p` already materialized" — that is **`Theta(n·K)` bytes of serialization work**, i.e. the search is `2·Theta(n·K)`, not `Theta(n·K)`. Understates the cheap term by ~2x | `[D]` from §3.1/§4 |
| The frozen 32-entry selector table's **value distribution shift** | changes BTB/branch behaviour; see A-3 | `[P]` |
| Encoder RSS growth from `O(S)` `u64` incumbents | at `S = 10^6`, 8 MB; unmeasured | `[P]` |
| **The dual-bar transform control** | E12's 2.9 MB surplus is measured against *Brotli*, not against a *transform-enabled* reference. E14/E10 show extraction itself is negative. **A search that maximizes q11-on-extracted-chunks will never select the answer that a transform-enabled reference would prefer** | `[D]` |
| V1-family non-transfer | E9/E10 | `[M]` |

---

## 8. Corpus memorization — three vectors, the third is new

### 8.1 Population-level
§6.1. Three recorded sign flips (E9, E10, and G5B's `ORDINAL-ADVERSE` E19) between
D-family and V1/other families.

### 8.2 Decision-level
G4 fit 8 ridge features on D1–D4 and missed per-file fidelity by 13–27x (E2/E4).
PRA-1 fits nothing, so this vector is **closed by construction**. Credit where due.

### 8.3 Selector-level memorization — **[NEW FINDING, sharpest in this audit]**

**The incumbent's heuristic has already *memorized* D1–D4 at the selector level,
by construction.** The incumbent *is* the rule that generated the selector bytes
on the discovery corpus. So on D1–D4:

> **`sel_regret` measures the distance between the frozen G5A selector rule and
> itself, evaluated with the body recomputed exactly. The selector-level part of
> that distance is definitionally zero on the population that defined the rule.**

Everything `sel_regret` can recover on D1–D4 is therefore recovered in the
**body's** compression, not in the selector's choice — i.e. through the
mechanism §6.2 calls leakage.

**This inverts the constructive report's central inference.** §1.2 asserts that
G5A-LC's same-multiset invariants (`I5`, `I14`) make "any byte movement PRA-1
produces *attributable to the assignment decision alone*." That is **half wrong,
in the direction that matters**:

- `I5` (multiset identity) makes the movement attributable to **ordering +
  assignment taken together**;
- it does **not** separate *ordering* (E6, the 89,960 B G5A result) from
  *assignment* (PRA-1's proposed prize).

To separate them you must hold ordering frozen and vary only assignment — or hold
assignment frozen and vary only ordering. **PRA-1 as specified varies assignment
inside a frozen carrier whose ordering is itself a chosen arm; the constructive
report does not state which arm is held.** `[OPEN]`

**Two required experiments, neither currently specified:**
- **M1**: fix ordering at the incumbent arm, vary only assignment → isolates PRA-1.
- **M2**: fix assignment at the incumbent rule, vary only ordering → recovers the
  G5A E6 effect. **If M2 reproduces a large share of the same bytes, the selector
  was never the binding constraint and PRA-1's prize is largely E6 again.**

This is the single most useful thing in my audit. It is also *free* — both arms
reuse frozen artifacts and zero Brotli calls beyond the mandatory floor replay.

### 8.4 Corollary: selector bytes are 0.5% of the budget — which is why RLE was the right thing to exclude *and* the right thing to revisit

```[D]``` using E6/E7: D-family records ≈ 50,000 (constructive report M2 cites ~100 KB
selector bytes at ~2 B/record). Selector bytes ≈ **100 KB of a ~1.38 MB arm =
~7%** on the D-family; but measured *value* of that 7% is bounded by the
achievable reassignment, and the **maximum** conceivable reassignment gain is the
whole selector budget, ~7%, while the **realistic** gain is `sel_regret`.

The coordinator's test (2) — *does RLE of selector IDs stay ≤ the current
one-byte-per-record charge under all inputs, including run breaks and escape
headers?* — is the right question and it has a clean answer:

```[P]``` **No, not in general, and not even close in the adversarial case.**
Break-even for RLE requires run length `L ≥ ~8–10` (escape header + length +
opcodes amortize only then). The corpus contains exactly the families that defeat
it: E16/E17 show the project's *most* profitable routing decisions are
**inconsistent across neighbouring records** (block-local warmup tax +894,315 B;
QLFC 0/13 wins) and E10's V1 is **100% structured with 3 shapes** — which is the
RLE-*friendly* end. So RLE's viability is entirely a function of selector-run
statistics that **have never been measured**. The frozen
`I7`-class diagnostics (`shape_transition_count`, `slot_transition_count`,
`same_shape_adjacent_pair_fraction` — all zero-Brotli-call) would settle it in one
remote run at ~zero cost. `[OPEN]`

**Note the tension worth recording:** the constructive report excluded RLE
*because it needs new decoder opcodes*. Correct. But it is also the **only**
identified lever that could make selector bytes a first-class cost, and therefore
the only lever that could turn this family from a rounding correction into a real
mechanism. Excluding it is right for v1; it must not be excluded from the record.

---

## 9. Prior-art map (Track 20 handoff)

| Layer | Occupant | Status |
|---|---|---|
| **Genus** — "search offline, execute a resolved plan at decode" | OpenZL (offline plan search, resolved graph in frame, one universal decoder); Grotli's four-layer router; Brevis (learned prior + bounded A*, **operator-reported, unverified**) | **OCCUPIED** |
| **Genus** — "encoder picks among equivalent decodable representations by exact bytes" | universal/online portfolio scheduling, double-trigger; dynamic-programming optimal parse (LZMA/`-9e` optimal parser; zstd `btultra`/`--ultra -22`; Brotli's own backward-reference + block-split search) | **OCCUPIED — classical** |
| **Genus** — "one-sided admissible pruning over compression alternatives" | branch-and-bound / A\*; IDA\*; anytime/portfolio anytime algorithms | **OCCUPIED — textbook** |
| **Species** — what is *arguably* left | exact surrogate-free arbitration over a **frozen** reversible-representation set, with a **one-sided payload bound** for zero-backend-call pruning and a **provably wire-identical** decoder, under an input-byte-denominated budget | **NARROW** |

**My novelty verdict — harsher than the constructive report's, and I think
correctly so:**

> **The algorithmic content of PRA-1 is classical in every layer.** Branch-and-bound
> over a candidate set with an admissible lower bound is *the* canonical exact
> search-with-pruning construction. Optimal parsing with a real measured cost
> model is what `xz -9e` and `zstd btultra` already do — and ANVIL's own
> `--parse=mdl` (ledger Experiment F) is that mechanism, already shipped, already
> measured at **−8.6% to −11.0%** ratio on record-structured data, already
> **closed on the throughput leg** at ~1.7–1.9 MB/s.
>
> **So the project already contains the strongest form of this mechanism.** What
> PRA-1 adds over `--parse=mdl` is *which* frozen option set it searches (whole
> representation families, not just tokens within one family). That is a real
> difference, and it is exactly the difference that makes E14/E10's extraction tax
> bite. It is **not** a new algorithmic primitive.

**Decisive novelty separators for Track 20:**
1. **Is the bound admissible over *complete carrier bytes*, or only over payload?**
   PRA-1 is payload-only ⇒ weak (my A-2).
2. **Does the option set cross representation families, or only tokenizations?**
   Only families ⇒ distinct from `--parse=mdl`; tokens ⇒ duplicate.
3. **Is any of it measurable on a held-out family?** No family exists (E21).
4. **Is the budget denominated in input bytes?** Yes ⇒ genuinely distinct from
   G4's unbounded `candidates_enumerated` (E5).

**Prior-art search status: `[OPEN]`.** E22 is binding: ACM/IEEE/USPTO were never
queried and Espacenet/lens.org never queried. No novelty wording may be upgraded
until at least one patent-family search and one post-2024 survey exist. The
constructive report's `{mechanism-candidate, novelty-unaudited}` is the correct
label and must not be softened.

---

## 10. Reconciliation with the constructive report (`PRA-1`)

Read in full. Agreements and disagreements, stated as such.

### 10.1 Agreements — verified independently, not taken on trust

| # | Agreement | My independent check |
|---|---|---|
| G1 | G4's worst-file regrets are 6.6056% / 7.0802% / 1.5388% | re-read PART XV §G4; **matches**, including the correction that 7.0802% was the DICT-specific DSTAR metric, not the overall proxy metric |
| G2 | G4's speed column is `INVALID_SPEED_ACCOUNTING` (667.948690 ms vs 0.004758 ms) | re-read PART XV §G4; **matches**; this is a serious defect in the project's own measurement harness and Track 04 should own it |
| G3 | G4 fidelity bars are 0.25% aggregate / 0.50% per-file | re-read §G8.2/§G8.3 of the prereg; **matches** |
| G4 | Selector is already charged, 1 byte/record, 32 values | re-read G5A-LC §4.1/§8; **matches** — and it is the load-bearing fact for the whole mechanism |
| G5 | G5A A3−A1 = 89,960 B, 4/4, `COLUMN-DOMINANT` | re-read PART XV §G5A; **matches** |
| G6 | G5A V1 reversed; V1 is known-stress, not held-out | re-read PART XV + G5A-LC §0; **matches**, and I strengthen it (E21: **no** held-out family exists at all) |
| G7 | Decode-only routes are closed 12/13, MATH-class | re-read PART XIII §3; **matches** |
| G8 | Prior-art audit is incomplete | re-read `gate-priorart-audit-i8.md` §5 via citation; **matches** |
| G9 | Excluding RLE / static tie-break / new families is what keeps the decoder delta provably zero | re-derived; **correct and load-bearing** (A-4) — and I add that Track 18's 2.3–8.1% instrumentation perturbation makes this non-negotiable |
| G10 | `A_greedy_h0` is the right control arm | **strongly endorsed** — it is the only arm that tests the bound's informativeness, and per my A-2 that is the arm most likely to fail |

### 10.2 Disagreements — each with what would change my mind

| # | Constructive claim | My finding | What would change my mind |
|---|---|---|---|
| **D1** | §2.1 clause 5: decoder cycles are "bit-identical"; `Δ decoder cycles per record = 0` | **Overstated** (A-3). Field/algorithm identity is proven; *cycle* identity is not derivable and is microarchitecture-dependent | A same-run paired decode-timing measurement on ≥7 reps showing the selector-distribution change is inside CV. Even then it would be *no-worse*, not *bit-identical* |
| **D2** | §5.2/§8: `Theta(n·K)` histogram work | **Understated ~2x** — §3.1 `ENC_2` requires payloads already materialized, which is another `Theta(n·K)` | A corrected complexity statement, or dropping the materialization requirement |
| **D3** | §6.2 P3: `beta ≈ 0.02–0.05`, "medium confidence" | **Not affordable** (A-7). `beta_max ≈ 0.018` by E12/E13 arithmetic; the upper end is a 51–83% encode regression | Citation-grade same-run encode timing that shows the per-candidate-pass multiplier is ≪3.30x |
| **D4** | §1.2: `I5` makes PRA-1's bytes "attributable to the assignment decision alone" | **False as stated** (A-5/§8.3). `I5` fixes the *content* multiset; it does not separate *ordering* from *assignment*. Also, selector-level regret is definitionally ~0 on the population that defined the incumbent rule | The M1/M2 decomposition (§8.3) showing assignment-only variance ≫ ordering-only variance |
| **D5** | §7.2: GO-1/GO-2 promotion thresholds on the discovery population | **Cannot serve as promotion evidence** (A-6) | A locked held-out V2 family existing (E21: it does not) |
| **D6** | §6.2 P4: reclaimed bytes may survive the dual bar | **Weakly supported.** E14 (+3,436 B) and E10 (+3.1900%) both show the extraction regime is already *negative* vs raw Brotli. PRA-1 optimizes post-extraction q11 bytes and is therefore structurally indifferent to that tax | A same-transform control showing the extracted-chunk q11 bytes beat the transform-enabled reference by a margin ≥ the extraction tax |
| **D7** | §9: "a reviewer may reasonably classify the algorithmic content as classical" | **Understated.** It is *definitely* classical at every layer. The only defensible species claim is the frozen-carrier/wire-identical/one-sided-bound combination | A prior-art search (E22: never done) finding the species unoccupied |
| **D8** | §7 kill thresholds 0.25%/0.50% | **Keep frozen — I accept and re-affirm them.** They are principled (the project's own definition of proxy fidelity). I only add that they must be applied to **`sel_regret` measured with ordering frozen** | Nothing; these stay |

### 10.3 Coordinator-raised blockers, resolved explicitly

| Blocker | Resolution |
|---|---|
| **(1) Is +3.1900% V1 caused by selector/framing bytes or by representation choice itself?** | **Representation choice itself**, and this is now *derived*, not guessed. E15: V1 had **no raw residual frames** ⇒ no RAW leaf was ever selected ⇒ no "raw wins exact ties" branch fired. **H-12b is UNPROVEN.** Selector/framing share is not measured anywhere in this project. **Not a repair claim — a kill-first byte decomposition (`KB-0`, §11).** |
| **(2) Does RLE of selector IDs stay ≤ the one-byte-per-record charge under all inputs?** | **Not in general** (A-2/§8.4): break-even needs runs of ~8–10; corpus selector-run statistics are **unmeasured**; E16/E17 show neighbouring-record inconsistency is where the project's wins live. Settled only by the zero-Brotli-call `I7`-class diagnostics. |
| **(3) Is static decode-cost tie-breaking valid given 2.3–8.1% instrumentation perturbation and source-vs-FORMAT constant drift?** | **Not valid as v1, and the reason is stronger than the constructive report gives.** It is not merely a cost argument — it is **already falsified by E16/E17**: whole-file/whole-carrier decisions beat per-record decisions by **+894,315 B (12.22%)** on webster, with QLFC winning 0/13. `KB-3` (§12) is a **kill-first** test, and it is the cheapest experiment in this audit. |
| **(4) No new held-out structured set exists, so do not call V1 held-out** | **Accepted and strengthened.** E21: no new locked structured/mixed/executable/numeric family exists. V1 is `KNOWN-STRESS` (`I10-G5A-LOCALITY-CONTROLS-PREREG.md` §6.2: "cannot satisfy any discovery breadth, generalization, promotion, or Pareto gate"). **Nothing in this track may be described as held-out, generalizing, or transfer-validated.** Any future text using "held-out" for V1 is a claim-hygiene violation under PART XIII §6. |

---

## 11. Decisive remote experiment — re-specified, kill-first

The constructive report proposes `SELREG-1` (byte-only search-quality census). **I
accept the experiment and reject its arm set as under-specified**, for three
reasons: (i) `A_greedy_h0` and `A_exact` alone cannot attribute bytes (D4/§8.3);
(ii) it runs on the discovery population only, which cannot support promotion
(D5); (iii) it does not decompose the regression, so a positive result would still
leave H-12b open.

### `KB-0` — the attribution-first byte decomposition (**cheapest; run this first**)

**Remote-only. Byte-only. No timing. No new corpus. No new representation
opportunity. No new Brotli opportunity. Four arms, all from frozen artifacts.**

| arm | definition | what it isolates |
|---|---|---|
| `KB0-a` | **Selector/framing floor.** Emit the incumbent's own carrier with the Brotli body replaced by a **stored** run of equal length. Difference vs the incumbent = selector + envelope + frame-map + descriptors + shape-ID + body-stream overhead, isolated from compression. | **(i) selector bytes, (ii) all framing.** **Settles H-12b / coordinator test (1).** |
| `KB0-b` | **One G3 V1 carrier, per-component byte breakdown**: selector stream / envelope+frame-map+descriptors+shape-IDs / raw-residual section / compressed structured-chunk stream. Computes the project's `I14` complete accounting for the first time. | **Why V1 is +3.1900%.** Coordinator test (1). |
| `KB0-c` | **Ordering-only arm (M2).** Hold assignment at the incumbent rule; vary ordering over the 4 frozen G5A arms. | **How much of the 89,960 B (E6) is ordering** — i.e. whether PRA-1's prize is E6 again. |
| `KB0-d` | **Assignment-only arm (M1).** Hold ordering at the incumbent arm; vary only assignment (incumbent vs exact argmin vs `A_greedy_h0`). | **The only arm that measures PRA-1's actual prize.** |

**Pre-registered kill / promote thresholds (frozen before any run; unchanged
afterwards):**

```text
KILL  KB-0  if  ANY of:
  (K1) KB0-b shows selector + framing bytes account for >= 50% of the V1
       regression magnitude            -> the regression is overhead, not
                                          representation; PRA-1's whole thesis
                                          (improve the assignment) is misaimed.
  (K2) KB0-c reproduces >= 50% of the same bytes that KB0-d moves
       -> the prize is G5A ordering (E6), already measured and already
          attributed COLUMN-DOMINANT; PRA-1 is a duplicate of a closed result.
  (K3) sel_regret_agg <= 0.0025  (0.25%, = G4 frozen bar E4)
       -> below the project's own proxy-fidelity tolerance: no search
          formulation in this family can be a mechanism.
  (K4) sel_regret(f) <= 0.0050 (0.50%) on every file
       -> same conclusion, per-file.
  (K5) KB0-a shows the selector+framing floor exceeds 5% of the complete
       charged bytes on any discovery file
       -> assignment bytes are not the binding constraint.

PROMOTE  KB-0  if ALL of:
  (P1) KB0-d sel_regret_agg >= 0.0100  (1.00%)   AND
  (P2) KB0-d sel_regret(f) >= 1.00% on >= 3 of 4 discovery files
  (P3) KB0-c moves strictly LESS than KB0-d  (assignment > ordering)  [replaces
       constructive GO-2; this is the anti-D4 gate]
  (P4) I5 same-multiset identity + I14 complete accounting hold exactly on every
       arm and file; any failure => INVALID, not adverse
  (P5) E_total, pruned_fraction, sel_regret, and the KB0-b component table are
       reported exactly and reproducibly from the pinned artifacts

THEN, AND ONLY THEN: a citation-grade Stage B on a LOCKED V2 family
  (does not exist -> E21 is a hard external blocker, coordinator-owned)
  measuring paired encode/decode/peak-RSS/decoder-size, dual-bar transform
  control, and beta <= 0.015 (A-7).
```

**Why `KB-0` is the right first experiment, not `SELREG-1`:** `KB0-a` and `KB0-b`
cost **zero Brotli calls** (a stored-run substitution and a re-read of the
incumbent's own bytes). `KB0-c`/`KB0-d` reuse the frozen G5A arms and the
already-frozen zero-Brotli-call `I7` diagnostics. **The entire causal attribution
that the mechanism's viability depends on is obtainable before a single
expensive candidate evaluation is authorized.**

---

## 12. Cheapest decisive next step, ranked

| rank | step | cost | decides |
|---|---|---|---|
| **1** | **`KB-0` arms a/b/c** — byte-only, zero new Brotli calls, frozen artifacts | **lowest** | H-12b (coordinator test 1); whether the prize is ordering or assignment (D4) |
| **2** | **`KB-0` arm d** — byte-only, ≤ `beta·n` candidate evals | low | PRA-1's actual `sel_regret`, with K3/K4 frozen |
| **3** | **`KB3` static decode-cost tie-break** — coordinator test (3) | low, and **likely already answered** | whether per-record decode-cost tie-breaking is admissible at all. **Strong prior it is NOT**, on E16 (+894,315 B / +12.22%) and E17 (QLFC 0/13) grounds — whole-carrier decisions dominate per-record decisions on this corpus. If `KB-0`/`KB3` reproduces even a fraction of that, the tie-break family is dead and should be closed *before* anyone builds it, not after |
| **4** | **selector-run census** — `I7`-class zero-Brotli-call diagnostics (`shape_transition_count`, `slot_transition_count`, `same_shape_adjacent_pair_fraction`) | very low | coordinator test (2): whether RLE of selector IDs could ever break even (§8.4) |
| 5 | citation-grade Stage B on locked V2 | **blocked** | E21: the family does not exist. Coordinator-owned |

---

## 13. Failure modes specific to this audit's objections

| # | Failure | Consequence | Gate |
|---|---|---|---|
| B1 | `beta` binds and `sel_regret > 0` | non-optimal output | must be reported; a run that cannot report `E_total` and `sel_regret` is `INVALID` |
| B2 | The admissible bound is inadmissible at an edge (empty segment, single-byte payload, alphabet < 2) | **silent byte loss** | `max(0, ceil(H0))` + `+1` floor; exhaustive small-input enumeration in remote R0; `I5` catches content drift |
| B3 | Arms silently diverge in representation content, not assignment | byte delta misattributed | `I5` byte-level multiset identity **+** `I14` complete accounting (`KB0-b`) |
| B4 | Dual-bar failure: exact argmin beats raw Brotli but loses to the transform-enabled reference | a win that is a transform the reference declined | mandatory transform-enabled control; **D6** makes this a live risk, not a formality |
| B5 | V1-family non-transfer | generalization failure | locked V2 family — **does not exist** (E21) |
| B6 | Build-dependent argmin (compiler/backend version changes the choice) | irreproducible bytes | freeze option set, carrier grammar, backend, toolchain by SHA-256; **refuse to run on mismatch**. Precedent: E23, local `grotli_g3.cpp` is not an admissible substitute |
| B7 | A random null seed ties or beats the exact arm | not a mechanism | 8 frozen seeds (G5A-LC §9.2); exact arm must win every file |
| B8 | "Improvement" adds an opcode / RLE / a family | voids Theorem Z; becomes a format change; re-opens the 2.3–8.1% instrumentation-perturbation axis | §2.2 forbidden list, enforced as a pre-registered invariant |
| B9 | Provenance splicing across arms/runs/windows | fabricated win | `KB-0` is byte-only and single-run, making this structurally impossible |
| B10 | **Local execution** | protocol breach | everything remote; local Windows C++ build is BLOCKED anyway (no RC compiler) |

---

## 14. Claims that must never be made from this track

- **Mechanism-level novelty.** Every algorithmic layer is classical (§9). The
  defensible species statement is narrow and architectural, and even that is
  `{novelty-unaudited}` until E22 is closed.
- **A FRONT-CROSSING.** E1 (0 crossings), E24 (DNB-M1, MATH-class).
- **Any reversal of the decode-lane closure.**
- **That V1 is held-out, or that anything in this track generalizes** (E21).
- **That the +3.1900% V1 regression is attributable to selector/framing bytes.**
  H-12b is **UNPROVEN** (§4, P-3).
- **That PRA-1's prize is disjoint from G5A's ordering result** until `KB0-c` /
  `KB0-d` are run (§8.3, D4).
- **Any timing figure assembled from more than one run** — E3 is the standing
  precedent.
- **Any number not recomputable from a named artifact with the verifying command**
  (PART XIII §6 standing order 4).

---

## 15. Verdict (SUPERSEDED by Appendix 1 §1.6 — see that ruling)

> **SUPERSESSION NOTICE (2026-10-02).** The ruling below was written while this
> reviewer still accepted PRA-1's prune as *admissible*. Appendix 1 §1.1 proves
> it is not. **Appendix 1 §1.6 replaces this section's KILL/HOLD/PILOT/
> PROMOTE-TO-REMOTE token with `KILL` for PRA-1 as specified.** The three reasons
> below remain valid as *additional* grounds, but the primary basis is now a
> `MATH`-class correctness defect, not performance and attribution. This section
> is retained unedited so the change of mind is auditable.

# **HOLD**

**On the track's mechanism (PRA-1), the paired verdict is HOLD, one step below
the constructive lane's PILOT.** This is a real disagreement and the coordinator
should reconcile it explicitly. Three reasons, in order of force:

1. **The mechanism's own load-bearing premise is unverified and cheaply
   verifiable.** §8.3: `sel_regret` on the discovery population is, at the
   selector level, definitionally near-zero, because the incumbent heuristic
   *defined* the selector bytes on that population. Everything recoverable is
   recovered through the leakage-exposed objective (§6.2). Until `KB0-c`/`KB0-d`
   separate *assignment* variance from *ordering* variance, we cannot tell
   whether PRA-1 is a new mechanism or G5A-E6 re-measured. The constructive
   report's §1.2 assertion that `I5` makes the bytes "attributable to the
   assignment decision alone" is **false as stated** (D4).

2. **The encode budget is not affordable at the proposed size.** `beta_max ≈ 0.018`
   by E12/E13 arithmetic; the report proposes 0.02–0.05, i.e. 1.1x–2.8x over the
   entire measured byte headroom against the reference (A-7). One of my own
   numbers is `TIMING_BLOCKED`-class and I say so — but the order of magnitude is
   not in doubt.

3. **Zero held-out evidence exists at all** (E21), and three independent
   D-family-vs-V1 sign reversals are on record (E9, E10, E19). The correct posture
   is to buy information cheaply *before* committing a run, not after.

**Why not KILL.** PRA-1 fixes a specific, correctly-diagnosed defect (G4's
two-sided error on an external objective), fits nothing to any corpus, provably
adds zero decoder bytes, and has an affine cost ceiling. Its cheapest experiment
(`KB-0`) needs **zero expensive backend calls** and settles the question. Killing
it now would be killing the cheapest remaining information in the track.

**Why not PILOT.** `SELREG-1` as specified would consume a real remote run to
measure a quantity that §8.3 shows is largely confounded with G5A's already-closed
result, and would leave H-12b (coordinator test 1) untouched.

**Why not PROMOTE-TO-REMOTE.** Independently of the scientific verdict: PART XV
"Pending handoff" records the dense-frontier, G5D and planner-pivot vehicles as
**uncommitted local work blocked on explicit commit/push authorization**, and
E23 records that the G5A-LC vehicle is unfrozen with the local `grotli_g3.cpp`
**not an admissible substitute**. Promotion now adds to a publication queue, not
to knowledge. Coordinator-owned blocker.

**HOLD is released to PILOT immediately upon:** the coordinator authorizing
publication of the `KB-0` pilot source, preregistration and workflow. **HOLD
becomes KILL upon:** any of K1–K5 in §11.

### The one sentence to remember

> *PRA-1 is the correct repair to the defect that killed G4, on the one axis where
> ANVIL has provably spare decoder resources — but its prize is measured on the
> population that defined the incumbent's selector values, against an objective the
> project's own negatives say not to trust, at an encode budget 1.1x–2.8x over the
> whole measured headroom; so spend nothing until `KB-0` — four byte-only arms,
> two of which cost zero backend calls — has told us whether the bytes it can
> recover were never the selector's to begin with.*

---

## 16. Post-write reconciliation addendum

*Written after the main body above. Contains no new evidence class.*

- **Space Bunny's PRA-1 report was present** at
  `docs/swarm-2026-10-02/12-encoder-search-space-bunny.md` (40,107 B) and was read
  in full **before** this file was written. §10 is the reconciliation.
- **No disagreement was found in the constructive report's transcribed measured
  facts.** All nine spot-checks (G1–G9) matched their named artifacts exactly,
  including the `INVALID_SPEED_ACCOUNTING` detail. **The disagreement is entirely
  about inference and framing, not about arithmetic.**
- **The single largest divergence remains D4/§8.3** (selector-level memorization ⇒
  the discovery-population prize is confounded with G5A-E6). If Track 02's or
  Track 16's independent analysis independently reaches that the frozen G5A
  carrier's *ordering* arm already accounts for the movement, D4 becomes a
  multi-lane finding rather than a single-reviewer objection, and the HOLD should
  be read as near-certain. I have flagged `KB0-c` vs `KB0-d` as the ordering-only
  precedence gate (P3) precisely so this resolves by measurement, not by vote.
- **Unchanged after reconciliation:** the verdict is **HOLD**; the kill thresholds
  K1–K5 and promote thresholds P1–P5 are frozen; no local measurement was
  performed; no existing file was modified; no commit, push, reset, clean, stash,
  restore or rebase occurred.

---

*Fledge Alpha Free, track `12-encoder-search`. Independent adversarial audit.
Byte-only remote gates only. No commit, no push, no production edit, no local
benchmark, no local build claim, no prototype written (none was needed — every
load-bearing test is a byte decomposition against frozen artifacts, which doctrine
forbids running locally).*
---
---

# APPENDIX 1 — SECOND-PASS MATH AUDIT: `H0` INADMISSIBILITY (correctness kill of PRA-1 as specified)

**Dated:** 2026-10-02. **Author:** Fledge Alpha Free.
**Status:** supersedes §5.1 A-2 only. All other sections of this audit stand.

## 1.1 The withdrawal, restated as a formal correctness kill

I withdraw my own earlier statement (struck in §5.1) that PRA-1's
`LB(f,s) = framing_min(f,s) + 1 + max(0, ceil(H0(payload_f(s))))` is admissible.
**It is not admissible. PRA-1 as specified is a correctness defect, not a
performance concern.**

This is a stronger and different verdict from §15. There I ruled **HOLD** on
*performance and attribution* grounds while accepting the prune's soundness. The
soundness premise was wrong, so §15's HOLD no longer holds and is superseded by
the ruling in §1.6.

**The defect.** An admissible lower bound must satisfy `cost_Brotli(f,s) >= LB(f,s)`
for **every** admissible `(f,s)`. `H0` violates this on a payload family that is
not exotic and is not even unusual for a compressor's input.

**Explicit counterexample (exact arithmetic; no backend needed):**

Let the payload be `p = q||q` where `q` is `m` bytes drawn from a 256-symbol
alphabet. Every byte value then appears exactly twice, so

```
H0(p) = 2m * log2(256) = 2m bytes
```

Brotli emits `q`'s bytes as literals at ~8 bits each, then encodes the second copy
as **one copy command** (length `m`, distance `m`) costing `O(log m)` bits.
RFC 7932 guarantees copy commands over reconstructed history, so

```
Brotli_q11(p) <= m bytes + O(1)
```

`H0(p) = 2m` exceeds that by a factor approaching **2**, for every `m`. A second,
stronger instance from my own check: a perfectly periodic payload `("ABCD")*256`
has a **uniform** 4-symbol marginal, so `H0 = 2048 bits = 256 bytes`, while its LZ
description is *one literal plus one match* — order 10–20 bits. **The bound
over-estimates by more than an order of magnitude.**

**Why no measurement can rescue it.** The violation follows from RFC 7932's
command set plus arithmetic, not from a benchmark. It is therefore a `MATH`-class
defect in the project's own taxonomy — the same class as `DNB-M1` and `DNB-M2`.
**No remote run, and no tuning of `K`, `S`, `beta`, or the corpus, can repair
it.** Only replacing or removing the prune can.

**Third, independent defect — it survives even if the two above were patched.**
`H0` is an **expected** length over a distribution, not a worst-case length of a
specific instance. For `p = (1/2, 1/4, 1/4)`, `H = 1.5 bits/symbol`, but an
all-`A` instance realizes `1.0 bits/symbol`. So `H0` is not a valid lower bound
*per instance* even on a memoryless coder, before context modelling is
considered. Any bound that must hold for **every** payload must be worst-case;
`H0` is average-case.

**Consequences, stated without hedging:**

1. `ENC_2` prune soundness is **void** => the emitted argmin is **not** the exact
   argmin. The soundness induction in the constructive report's §4 ("maintain `U`
   = min over evaluated candidates ... hence no discarded candidate can improve
   `U`") **fails at its base case**, because a discarded candidate's cost may be
   *below* `LB`, not above it.
2. `A_greedy_h0` optimizes a non-admissible surrogate. It cannot separate "the
   bound is informative" from "exhaustive evaluation is informative" — it is
   neither.
3. **`sel_regret` is no longer a regret.** It is the gap between the incumbent and
   an argmin over a *pruned* set. If the prune discarded the true winner, the
   reported figure is **optimistic** — it understates the true prize. Every
   threshold keyed to `sel_regret` (`K3`, `K4`, `GO-1`, `GO-2`) is keyed to a
   quantity that can only err in the favourable direction.
4. Theorem Z is **unaffected** — the decoder genuinely does not change. The
   mechanism is free and *not* exact. Those were conflated and must not be
   conflated again.

**Scale note.** Even a *valid* order-0-style bound would be useless here. The
per-record candidate spread PRA-1 exists to arbitrate is ~1 byte on a 64–235 B
record, while the `H0` overestimate can reach 64–235 B. The signal-to-noise ratio
of the prune is **~64x–235x in the wrong direction** — its error exceeds the
decision it gates by two orders of magnitude. Independent evidence that no
order-0-family bound is the right instrument on this carrier.

## 1.2 Reconciliation with the constructive lane's correction

The constructive lane has, **independently and after the same challenge**, reached
the same disproof and withdrawn the claim (`v1` §0, §1, W-1, W-2; Addendum A
records that its pre-correction body was overwritten in place and is now restored
verbatim). One agreement, three disagreements.

**Agreement (no dispute).** `H0` is inadmissible. The `q||q` family is the right
counterexample. `A_greedy_h0` is withdrawn. `SELREG-1` is withdrawn as a decision
instrument. The mechanism is not exact. This is common ground and must not be
re-litigated by any successor lane.

## 1.3 DISAGREEMENT 1 — `Theorem N` overclaims, and the overclaim is dangerous

> **Theorem N (quoted verbatim).** *"Let `B` be a lossless backend ... Let `LB` be
> any function the encoder computes from the payload alone. If `LB` is not derived
> from `B`'s own model class, then `LB` is not a valid lower bound on `B(x)` for all
> payloads `x` ... Consequently the only universally valid one-sided bounds on `B(x)`
> computable from `x` alone are those obtained by assuming a model class strictly
> narrower than `B`'s — and any such bound excludes, by construction, the
> possibility that the true minimum lies outside that class."*

**What the proof actually establishes** (the report's own sketch (a)):

> *"The same construction applies to any `LB` that only counts symbol frequencies:
> repeating any payload strictly decreases the backend's cost while leaving the
> frequency-derived quantity at full scale."*

**`q||q` refutes frequency-counting bounds. It does not refute all
encoder-computable bounds.** Sketch (a) is scoped by its own first clause to `LB`
that "only counts symbol frequencies." Theorem N's *statement* drops that scope
and generalizes to "any function the encoder computes from the payload alone."
**The generalization is unproved.** This is exactly the failure the coordinator's
second-pass instruction names: one counterexample turned into an impossibility
theorem.

**Four categories must be kept apart; Theorem N collapses them:**

| # | category | status | verdict |
|---|---|---|---|
| 1 | **`H0` / order-0 frequency bounds are invalid** | **PROVEN** by `q||q`, `(ABCD)*k`, and the per-instance defect | **KEEP** — correct and sufficient for the kill |
| 2 | **Framing-only bound** `LB = framing_min(f,s) + 1` | **TRIVIALLY VALID** — the wire has >=1 selector byte and each family's fields are exactly known; no payload content can make it smaller | **KEEP** — admissible, just weak (prunes only when framing alone exceeds `U`) |
| 3 | **Backend-aware / family-specific structural bounds** | **NOT EXCLUDED.** E.g. a family whose serializer emits a fixed-width, byte-identical layout, where a decoder-visible width table gives an exact byte count with zero payload dependence; or a bound derived from the carrier's own admissible set rather than from `B`'s output size | **`[OPEN]`** — no proof either way |
| 4 | **Shared / incremental exact evaluation** | **NOT EXCLUDED.** "Exact => evaluate every candidate" assumes candidates are evaluated *independently*. A bound need not exist if evaluations *share work*: candidates sharing a long common prefix/suffix admit amortized evaluation under a streaming backend. G5A-LC's own `I5` is a multiset identity over charged chunks, so chunk-level sharing is structurally available on this carrier | **`[OPEN]`** — no proof either way |

Theorem N's own statement does not cover (3) or (4), yet its Corollary (d) — the
"trichotomy" — asserts that *exact* requires evaluating `B` on every candidate.
That inference needs two further premises: that candidates are **independent**,
and that evaluation **cannot be amortized**. Neither is stated; neither is proved.

**Category 2 is the practically important one.** Framing-only is *always*
available, always admissible, and free. A **partial** bound of the form

```
LB(f,s) = framing_min(f,s) + 1 + max(0, ceil(H0(payload)))   [used only when the
          family's decoder-visible width table is payload-independent]
```

is admissible **exactly** on the subset `A` of admitted candidates where the
precondition holds, and sound there. Theorem N is silent on restricted-domain
bounds. This gives a **sound and cheap** prune on a subset with exact evaluation
on the rest — strictly stronger than "no prune at all", and nobody has costed it.

**Audit finding A-8 (against, methodological).** `Theorem N` is a **convenient
negative**. Its consequences — "the G5 bounded finalist ladder is the only sound
design class," "Branch R has no residual content," "any successor proposing a
cheap-and-exact search should be **rejected before any implementation or
preregistration**" — all derive from the unproved generalization. An impossibility
claim used to close a lane *before the lane has been searched* is the same error
class as the H0 claim it replaced: an unverified structural claim doing decisive
work. `PART XIII` §6 claim-hygiene and the project's own `DNB-M4` ("propagating a
tally without recomputation") govern.

**What survives, stated at the strength actually proved:**

> **Claim N' (defensible).** For a backend with backward references and context
> modelling (Brotli per RFC 7932), **no frequency-only or order-0 bound computed
> from the payload alone is a valid lower bound on backend output.** A prune keyed
> to `H0`, `H1`, ..., or any symbol-histogram statistic is unsound and must not
> discard candidates.
>
> This discharges exactly one thing: **no surrogate-free prune is available from
> payload statistics alone.** It does **not** establish that no cheap-and-exact
> method exists on this carrier.

**Corollary N' (materially weaker than the reported trichotomy):** If the option
set is `K` families per segment and no admissible bound is available, exact
arbitration requires an exact evaluation per surviving candidate. Whether those
evaluations amortize across candidates sharing charged chunks is **`[OPEN]`**.
Claim N' therefore supports "budget the evaluations and report the shortfall" —
the constructive report's own `POOL-1`/`sel_regret` discipline — and does **not**
support "there is nothing else to try."

## 1.4 DISAGREEMENT 2 — the Incompatibility Lemma is correct but is not a reason to stop

The Lemma (`delta selector bytes < 0` and `delta decoder cost = 0` cannot both
hold) is **sound and I endorse it without reservation**: clean, structural, and it
correctly prices Branch F's decoder cost, which the earlier draft omitted. It does
**not** imply selector bytes are irreducible in *value* — it proves they are
irreducible *for free*. The pool census therefore remains the right next step,
with three amendments (§1.5).

## 1.5 Audit of `POOL-1` and `Bound F` — endorsed with amendments

`Bound F` (`selector_bytes(F) <= R`, saving `sum max(0, R_r - 2 - uvar_len(R_r))`)
is a correct, escape-inclusive, worst-case-never-worse accounting. Endorsed.

**Amendment 1 — `POOL-1` must report the full run-length histogram, not just
mean/median.** The saving depends entirely on run structure, and the project's own
record says that structure is *adverse* where it matters: E16 (block-local
routing, +894,315 B / +12.22% warmup tax) and E17 (QLFC wins 0/13 files) are both
cases where **neighbouring records disagree**. The mean is the wrong statistic for
a `min`-of-two-encodings objective.

**Amendment 2 — the `0.25%` kill bar is not calibrated to Branch F.**
`KILL-POOL-F` borrows G4's *fidelity* tolerance, but G4's bar measures **planner
regret** while `POOL-1` measures a **byte pool** with a hard ceiling of `R`. If
`R/complete_bytes` is itself below 0.25%, `KILL-POOL-F` cannot fire and Branch F
survives on a pool too small to matter — a false negative. Add a structural kill:

```
KILL-POOL-S   if  R / sum complete_bytes  <  0.0025      # pool too small to matter
                 AND saving(F) / sum complete_bytes < 0.0025
```

**Amendment 3 — `POOL-1` must be joined by `KB-0` arms a/b from §11.** The
constructive lane proposes **no** measurement of the component split of the G3 V1
`+3.1900%` regression, and §4/P-3 of this audit established that **H-12b is
unproven and unmeasurable from any existing artifact**. `POOL-1` measures the
*pool*; it does not measure *why the regression happened*. Two of my four `KB-0`
arms (`KB0-a`, `KB0-b`) cost **zero Brotli calls**. Both experiments are remote,
byte-only and independent; there is no reason to run one without the other.

## 1.6 Revised verdict

### **KILL**

**KILL of PRA-1 as specified** — i.e. of admissible-bound exact arbitration with an
`H0` prune. This is a `MATH`-class correctness kill: established by arithmetic plus
RFC 7932's command set, requiring no measurement and admitting no tuning repair.
Recorded as KILL (not HOLD) because no cheap information could change it — a
successor could re-derive `H0`'s inadmissibility forever.

**This KILL does not extend to the track's surviving deliverables**, which are
sound and independently useful:

| deliverable | my ruling |
|---|---|
| the `q||q` disproof + formal withdrawal of the admissibility claim | **ENDORSE** — correct, well-constructed, worth a permanent do-not-re-burn entry |
| `Theorem N` **as stated** | **REJECT as overclaimed** — categories (3) and (4) unexcluded; downgrade to Claim N' (§1.3) |
| `Claim N'` (frequency-only bounds are unsound) | **ENDORSE** — the correct, proved, useful residue |
| the Incompatibility Lemma | **ENDORSE** |
| `Bound F` | **ENDORSE**, with Amendment 1 |
| `POOL-1` | **ENDORSE**, with Amendments 1–3 |
| `SELREG-1` | **ENDORSE the withdrawal** — it was keyed to a regret the defective prune biases optimistically |

**`Theorem N` must not be cited by Tracks 16 or 20 as a novelty separator or an
adoption blocker until restated as Claim N'.** In its current form it would let a
successor reject a sound, unexplored design on an unproved premise. That is the
specific harm flagged here, and the reason this appendix exists.

## 1.7 Cheapest decisive remote next step

Two independent byte-only runs, both zero-new-Brotli-call, neither requiring the
defect to be repaired:

1. **`KB-0` arms a/b/c** (§11) — selector/framing byte decomposition of one G3 V1
   carrier, plus the ordering-vs-assignment variance split. Settles **H-12b**,
   which no existing artifact can settle, and answers whether the frozen G5A
   prize is ordering (already measured, `COLUMN-DOMINANT`) or assignment.
2. **`POOL-1`** with Amendments 1–3 — the selector run-length census plus the
   structural pool kill.

**Neither is a search experiment.** Both are censuses over already-frozen
artifacts, which is why they are cheap and why they precede further mechanism work.

**Frozen thresholds are unchanged.** `K1`–`K5` and `P1`–`P5` from §11 stand as
written. `POOL-1`'s `0.25%` / `1.0%` bars stand; Amendment 2 **adds** a structural
kill and moves nothing.

---

## 1.8 Fact/provenance discipline for this appendix

| statement | tag |
|---|---|
| `H0(q||q) = 2m` bytes; `Brotli_q11(q||q) <= m + O(1)` | `[M]` exact arithmetic + RFC 7932 command set. **No measurement required or claimed.** |
| `("ABCD")*256`: `H0 = 2048 bits`, LZ description ~1 literal + 1 match | `[M]` exact arithmetic; the Brotli figure is `[P]` and irrelevant to the violation |
| `H` is an expectation, not a per-instance lower bound (1.0 vs 1.5 bits/symbol) | `[M]` exact arithmetic |
| prune error (<=235 B) vs decision spread (~1 B) => 64x–235x adverse SNR | `[D]` |
| **PRA-1 unsound => exactness void** | `[M]` — follows from the above |
| **`Theorem N` as stated is unproved beyond frequency-only bounds** | `[M]` — follows from its own proof sketch (a) scope vs its statement |
| **categories (3) family-specific bounds, (4) amortized/shared evaluation unexcluded** | **`[OPEN]`** — no proof either way. **This entry must not be silently upgraded.** |
| E16 warmup tax +894,315 B (+12.22%); E17 QLFC 0/13 | `[M]` PART XV / `06-do-not-reburn.md` §G1–G2 |
| `R` (record count) — the entire Branch F pool | **UNMEASURED** — `POOL-1` exists to measure it |

**No local heavy measurement, benchmark, sweep, or fuzz campaign was performed.**
The only local execution was `bound_check.py` (pure integer/log2 arithmetic on
constructed payloads; no corpus, no codec, no backend). No existing file was
modified. No commit, push, reset, clean, stash, restore, or rebase occurred.