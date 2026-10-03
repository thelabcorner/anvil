# Track 16 — Segmentation / Routing Theory (Space Bunny Free)

**Status:** RECONCILED after adversarial review (`16-segmentation-routing-fledge.md`).
**Date:** 2026-10-02
**Track:** 16 — region discovery, candidate pruning, regret bounds, fallback guarantees
**Verdict change:** **CDR is WITHDRAWN (KILL-as-specified).** No threshold was moved to
rescue it. A replacement pilot is staged as an **addendum arm to Track 02**, not a new lane.
**Production source changed:** no. **Files modified:** this report only.
**Prior artifacts of this lane:** this file, both versions. No other file touched; no
commit, push, reset, clean, stash, restore, or rebase; no local benchmark, sweep, or fuzz.

---

## PART A — Point-by-point response to the adversarial review

The reviewer raised four structural defects and five provenance defects. **All nine
stand.** I verified each against primary artifacts before responding. Summary:

| # | objection | verdict | evidence |
|---|---|---|---|
| D-1 | estimability: `O(1)` probes vs `Θ(S)` fit | **STANDS** | arithmetic, below |
| D-2 | Gate A bar looser than already-failed `S2` | **STANDS** | ledger PART XV §G4 |
| D-3 | wrong denominator (payload vs causal gain) | **STANDS, worse than stated** | `I10-GROTLI-G1-RESULTS.md` §6–7 |
| D-4 | region granularity inexpressible in frozen G3 carrier | **STANDS, fatal** | `tools/grotli_g3.cpp:328-358`, `:1103` |
| D-5 | decoder-RSS claim self-contradictory | **STANDS** | my §5.1 vs §5.2 |
| D-6 | Gate A's interaction/local split is circular | **STANDS** | below |
| D-7 | Gate D fails ARM-1 on the lane's own arithmetic | **STANDS** (structural, not predictive) | below |
| D-8 | G5B-ORDINAL is evidence *against* cheap positional prediction | **STANDS; I read it backwards** | ledger PART XV §G5B |
| D-9 | prior-art risk HIGH (BLOT / Brotli meta-block / zstd opt≥3) | **STANDS; I understated it** | below |
| — | E4 do-not-reburn overlap | **partially contestable, now moot** | see §A.9 |

---

### A.1 D-1 — Estimability. Conceded, and it is worse than I wrote.

I claimed 5 whole-carrier probes identify a displacement table, then specified fitting
it at `S ∈ {16, 64, 256, 1024}`. Those cannot both hold.

Each probe returns **one scalar**: the delta from placing leaf `ℓ` at its ordinal
positions with everything else at default. That delta is the **sum** of every
displacement contribution inside the probe, aliased with every byte that changed.
`|L| = 5` probes over one common baseline yield at most **4 independent contrasts**.
At `S = 16` that is 4× short; at `S = 1024` it is **256× short**.

D-4 (the reviewer's §1.3 D-4) compounds this and is also mine: I wrote
`b = (b_1 … b_{|shapes|×|buckets|})` — a **full 2-D table** — and called it "rank-1".
Those are opposites, and the contradiction was hiding the real problem: if `b` is the
full table, identification is `Θ(S)` q11 encodes per file. If `b` is genuinely
separable (`b = u[shape] ⊗ v[bucket]`), then the object is "position in ordinal space
matters", which is exactly what **G5B-ORDINAL measured as adverse (+1.6507%)**.

I cannot refute either branch. **The `O(1)`-probe production claim is withdrawn.**

### A.2 D-2 — Gate A was not discriminating. Conceded without qualification.

Ledger PART XV §G4 (corrected set): `S2` worst-file regret **1.5388%**. My Gate A bar
was **0.0200**. A mechanism reproducing `S2`'s measured failure **passes my gate**.

My written justification — *"deliberately not setting it below `S2`, because that would
make it uninformative"* — inverts the logic. A gate that a known failure passes is not
"uninformative"; it is **non-discriminating**. That sentence was self-serving and I
withdraw it.

The second half is worse. `0.0200 × 1,240,155 = 24,803 B`, and D3's measured causal
gain is `1,292,757 − 1,240,155 = 52,602 B`. So **Gate A permitted giving back 47.2% of
G3's total measured win on the file carrying ~88% of the aggregate.** My gate was
expressed as an absolute regret floor inherited from a *fidelity* experiment (G4's
0.25%/0.50% were fidelity-to-`O11` thresholds); CDR's value proposition is *economic*.
The scale was wrong by construction.

**Gate A is withdrawn, not re-tuned.** Re-deriving a bar after seeing the audit is the
exact move the brief's doctrine §5 forbids.

### A.3 D-3 — Wrong denominator, and the anti-correlation is structural. Conceded.

I charged `W_CDR` against the payload. The reviewer is right that the correct
denominator is the **measured causal gain the regionized architecture already earned**,
because that is the pool a routing mechanism spends from. Recomputed, and using the
**measured** frame counts from `docs/I10-GROTLI-G1-RESULTS.md`:

| cell | G1 records | exact shapes | G3 selected | raw | **causal gain** | `W_CDR` at R=records | **share of gain** |
|---|---:|---:|---:|---:|---:|---:|---:|
| D1 Amazon | 793 (§6) | **1** | 39,277 | 40,126 | **849 B** | 3,150 B | **371.1%** |
| D2 CDISC | 1,192 (§7) | **2** | 20,861 | 25,029 | **4,168 B** | 3,699 B | **88.7%** |
| D3 GH Archive | 11,228 (G3 §2) | *unrecorded* | 1,240,155 | 1,292,757 | **52,602 B** | 17,498 B | **33.3%** |
| D4 CROVIA | 3,718 (§5) | **42** | 84,361 | 108,857 | **24,496 B** | 7,172 B | **29.3%** |

**The mechanism's own shape counts kill it.** D1 has **one** exact shape across 793
records and D2 has **two** across 1,192: per-slot leaf choice has essentially **no
freedom** on those cells, yet they carry the largest wire charge per unit of gain
(371.1% and 88.7%). D4 has 42 shapes — real routing freedom — and the *lowest* charge
(29.3%). **Wire cost is anti-correlated with routing freedom**, because wire cost
scales with record count while freedom scales with shape diversity. That is structural,
not a tuning artifact.

Two further facts I had not surfaced:
- the 2,048 B table term alone is **49.1%** of D2's entire measured gain;
- **D4's selected G3 arm was `G2_WHOLE`, not regionized at all** (G3 §1) — so on D4
  CDR's regions are not even the arm the portfolio chose.

And my §4.4 merge objective cannot rescue it: merging adjacent regions *whose leaf
assignments differ* discards the routing decision, so the merge can only collapse
regions that already agreed — the ones that needed no route.

**The economics are withdrawn.**

### A.4 D-4 — The frozen G3 carrier cannot express a region. Conceded; this is the fatal one.

Verified directly in the frozen source:

- `tools/grotli_g3.cpp:333` — `struct ProdSlot { std::vector<ProdCandidate> candidates; ... }`
- `tools/grotli_g3.cpp:349` — `struct ProdShape { std::vector<ProdSlot> slots; }`
- `tools/grotli_g4_planner.cpp:320-327` — *"mirror the frozen G3 SlotPlan/ShapePlan
  structure"*; emission order is per slot, `RAW_LEX` first.
- `tools/grotli_g3.cpp:1102-1103` — the frozen carrier wire: *"for each slot:
  `leaf_id u8`, `payload_len`, `payload`"*.

So the leaf is selected and transmitted **per (shape, slot)**. G2 measured the
resulting counts: D2 = 296 `RAW_LEX` + **43 `EXACT_DICT`** columns
(`I10-GROTLI-G2-RESULTS.md` §D2); D4 = 724 `EXACT_DICT` leaves of 1,246 slots.

My "region" was defined (§4.1) as *a maximal run of consecutive G3 frames sharing one
shape and one leaf assignment* — i.e. agreement **across all slots simultaneously**.
That is a different granularity, and exactly two things can happen:

1. **Redundant.** Per-slot leaf IDs already on the wire fully determine the carrier. The
   region stream is then 100% waste and changes no decision.
2. **Grammar-breaking.** CDR overrides per-slot leaves. The carrier grammar, the
   decoder, and the wire change — and my §6.1 claim *"E7 reuses G3's builder verbatim,
   which is what keeps attribution clean"* is **false in this branch, always**.

There is no third option. This is a correctness blocker on the *experimental design*:
it voids the E5/G4 `16/16` carrier-identity comparator, which was the sole thing making
the G4 fidelity numbers trustworthy. **CDR cannot be measured against frozen G3 at all.**

### A.5 D-5 — Decoder state: my report contradicted itself. Conceded.

§5.1 claimed "decoder-visible tables **0 bytes**" while §5.2 charged `W_CDR` of *new
decoder-visible bytes*. Both cannot be reported as written. The reviewer also correctly
notes the "0 bytes" claim was contingent on an unstated assumption — region dispatch
needs the region extent before leaf decode begins, and G3's leaf payloads sit inside
length-prefixed group descriptors, so either the region stream is redundant (an
undeclared refund) or a pre-pass is required. I also under-stated that the 16-bit
on-wire table and the 8-byte encoder working table are different objects, only one of
which was in `W_CDR`.

### A.6 D-6 — Gate A's interaction/local split was circular. Conceded.

Decomposing the `O11`-vs-proxy residual into a "local" and an "interaction" part
requires a definition of `ĉ` **independent of the model under test**. Any residual can
be pushed into either bucket by moving the line. My second gate clause —
`interaction_share ≥ 0.30` — therefore described the answer rather than testing it. (The
clause was also written dimensionally wrong: `|b|/|a| + |b|` is not a share.)

The only external candidate for a frozen `ĉ` is S6-3, which I simultaneously called
calibrated **and** flagged as `[HARN-LX]`, harness-relative, single parse family, no
container router, transfer "never made and explicitly not assumed". Then Gate A
depended on it. **Unrunnable as written.**

### A.7 D-7 — Gate D fails ARM-1 on my own arithmetic. Conceded as structural.

Using my own §0-tagged windows and flagging that they are spliced (per D-10 below):
`O11` candidate scoring on D3 ≈ 50k isolated q11 evals ≈ 32 s
(`I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md` §9.1); `brotli-q11-lw30` ≈ 0.615 MB/s
(`tests/auto-routing.v2.csv`, single-run, ranking-grade only); CDR ARM-1 = 5 whole-carrier
q11 encodes ≈ 10.1 s ⇒ implied speedup ≈ **3.2×** against a **≥5.0×** bar.

I do **not** offer this as a prediction of ARM-1's outcome — the windows are
incompatible and the q11 figure is single-run. I offer it as what it is: **my Gate D
was designed to be failed by the arm it was meant to certify**, and it had to be run
*before* building, not after.

ARM-0's escape route was a globally-fitted position table on D1–D4 — which is D-1's
branch (a), i.e. the G5B object, and with `n = 4` files and no unopened held-out family
(ledger PART XV: *"No new held-out structured corpus is currently available"*), a
leave-one-file-out protocol over four points is **not a validation protocol**.

### A.8 D-8 — I read G5B backwards. Conceded.

I cited G5B-ORDINAL (`B1` exact 1,380,245 B; `B2` cheap-predicted 1,403,029 B,
**+1.6507%**) as evidence *for* the ordinal basis. The evidence is for the opposite
proposition: **placement is valuable when exact and harmful when cheaply predicted.**
CDR *is* cheap positional prediction. My §1.5 heading said `COLUMN-DOMINANT` and the
sentence under it said the basis is reusable; those did not support each other and I did
not notice.

### A.9 D-9 — Prior art: I understated the risk. Conceded, and I add to it.

I self-rated MEDIUM-HIGH and deferred the search. On the reviewer's map the honest
rating is **HIGH, likely adopt-class**. The closest line I missed is the **blocally-
compressible / BLOT** formulation (Pireddu & Langdon), which explicitly poses
*choosing the representation per block* as an MDL problem with the block-local model
itself selectable. Also damaging: Brotli's per-meta-block static/dynamic context and
literal selection (RFC 7932) and zstd's per-block coder trials at opt ≥ 3 — i.e. CDR's
method with a different fidelity ladder. I add 7-Zip's coder graph and bsc/lrzip
per-block mode choice. FastLanes/BtrBlocks damage the reviewer's own EXP-C equally and
should be recorded against both.

On **E4 overlap**: partially contestable, and I record the contest. E4 closed
*block-local backend routing* (measured `+643,453` B at 16 MiB to `+5,424,162` B at
256 KiB; `docs/audit-2026-09-07/13-e4-block-routing-oracle.md` — the reviewer quotes a
narrower `+1.65 MB`/4 MiB figure from `anvil-i9-findings.md`; **both are correct, at
different scales**, and I had quoted only one). Its reopen condition is "a backend with
near-zero warmup, or cross-block model carryover". CDR is a *leaf*-choice mechanism
inside a single backend pass, so the warmup mechanism does not literally transfer — a
real distinction. **But the reviewer's functional point stands**: a routing layer whose
value has never been measured is not entitled to a presumption, and E4's *lesson* —
locality is not automatically valuable; measure it before building for it — is exactly
what CDR failed to do. Moot now that CDR is withdrawn.

### A.10 Provenance defects I concede

- **D-1 (transcription).** The correct frozen G3 digest is
  `ac980f35feeefde0007c8f3aa1b5fd0c2262866ff66526b8c7d100c5117ab537` (`aa1b5f`, not
  `aa0b5f`). My first version mis-copied it *and* raised an integrity concern about it.
  The concern was legitimate; the transcription was not. Corrected above.
- **D-2 (unfrozen prereg as comparison base).** I compared `K_probe = 5` against the G5
  pilot's frozen 80-q4 cap. G5 is *authorized* but ledger PART XV records it as
  *"reports missing links and has no frozen workflow"*. A call-count comparison against
  an unexecuted prereg is against a plan. Conceded — though noting the prereg document
  itself is frozen as a *design target*, just not as measured evidence.
- **D-3 (`|shapes|` guessed).** I instantiated 64×16 = 2,048 B while conceding D3's
  shape count is unrecorded. G1 measured **D1 = 1, D2 = 2, D4 = 42**; D3 remains
  unrecorded and is the file carrying ~88% of the aggregate. The guess was unbounded
  where it mattered most.
- **D-10 (window splicing I named as failure mode).** §5.3's decode-cycle table used
  "assumed 1 GB/s / 3.0 GHz" beside measured bytes without the `[window]` tags my own
  §0 mandates. Naming failure mode #10 while committing it is exactly the swarm-wide
  integrity hazard.

---

## PART B — What survives the audit

Not the mechanism. These are provenance-clean, independently re-derived by the
reviewer, and load-bearing for the replacement pilot.

| fact | value | provenance |
|---|---|---|
| regionization is causal | D3 1,240,155 vs 1,292,757 B = **−4.0690%**, gain **52,602 B**, 11,228 structured + 1 residual frames | `I10-GROTLI-G3-RESULTS.md` §1–2, run `35947013432` |
| granularity is not universally good | V1 **+3.1900% worse**, 11,444 frames, 100% structured, **3 shapes, 0 integer leaves** | same §3 |
| routing freedom is tiny where frames are many | D1 **1 shape**/793 records; D2 **2 shapes**/1,192; D4 **42 shapes**/3,718 | `I10-GROTLI-G1-RESULTS.md` §5, §6, §7 |
| the local cost term is calibrated | **473,985** candidates, **zero sign flips**, median ≤16%, pessimistic | ledger §S6-3, `[HARN-LX]`, harness-relative |
| the interaction is the open question | Q2 max pair gain **0.277632%**, aggregate **0.057144%**, `SEPARABLE-ENOUGH`; vs S6-3's large trajectory effect | `I10-GROTLI-G4-RESULTS.md` §3, run `35952830106` |
| ordering is material when exact, adverse when predicted | G5A A3−A1 = **89,960 B**; G5B `B2 +1.6507%` | ledger PART XV, runs `35985412906` / `36011333908` |
| per-region **backend** invocation is math-closed | regret **+643,453 B** (16 MiB) … **+5,424,162 B** (256 KiB); warmup tax monotone | `docs/audit-2026-09-07/13-e4-block-routing-oracle.md` |
| fallback is free | `RAW_BROTLI` always eligible, raw wins ties; S6-3 measured-payload arbitration **Δ = +0.0000% exact, CV = 0** over 22 file-runs | G3 §11; ledger §S6-3 |
| **no unopened held-out structured family exists** | "No new held-out structured corpus is currently available"; V1 consumed by G3 and `KNOWN-STRESS` | ledger PART XV |
| G4 speed numbers are void | `INVALID_SPEED_ACCOUNTING` | ledger PART XV §G4 |
| only shipping routing path in ANVIL is per-file | `--ratio-backend=auto`, +159 B over oracle | `src/anvil.cpp:4400-4428`, `4614-4639` |

**The single question track 16 actually needs answered has never been measured:**
*is there any economic per-slot routing headroom on this carrier at all?* G4 measured
**fidelity to `O11`**. Nobody measured **how much better than `O11` the best
achievable per-slot selection could be**. That gap is the whole lane.

---

## PART C — Replacement pilot, budgeted against Track 02

**Track 02 is the G5A-informed bounded finalist planner** — `docs/I10-G5-PLANNER-PIVOT-PREREG.md`
(80 isolated q4/file; ladder 8×q1 / 4×q4 / 2×q6 / 2×q11 per file; raw q11 ×1; G2 q11
×1; `q11_rank_calls = 0`; research controls `O11` isolated q11, `O11` final q11 4/file,
`K-main` q11 ≤8 distinct/file, ≤6 additional).

### C.1 Budget comparison — the deciding factor

| | critic's EXP-R as a **separate** pilot | **EXP-R' as a Track 02 addendum arm** |
|---|---|---|
| rebuild frozen G3 + G4 | pays full fixed cost | **shares the job Track 02 already builds** |
| corpus fetch + identity verification | pays full fixed cost | **shared** |
| `16/16` carrier-identity gate | pays full fixed cost | **shared** |
| headroom/substitution encodes | ≤132 q11 | ≤120 q11 (R2+R3), **marginal CI cost = the encodes only** |
| result usable by Track 02 | no — two reports to reconcile | **yes — becomes Track 02's economic-value gate** |
| risk of double-spending planner budget | **high** | **none — it is the same job** |

Dispatching EXP-R standalone would re-pay Track 02's entire fixed overhead to produce a
number Track 02 needs in order to interpret its own verdict. **Run it as an arm.**

### C.2 Why the headroom arm is *upstream* of Track 02's verdict, not beside it

Track 02's gates are **fidelity** gates: `P1_aggregate_regret ≤ 0.0025`,
`max_f P1_O11_regret ≤ 0.0050`, `ladder_K_aggregate_regret ≤ 0.0010`. Those ask *"how
close is P1 to `O11`?"* They cannot ask *"was `O11` worth approximating?"* because
`O11` is defined as the reference (`C_ref(f) = min_F C(f,O11,F)`, prereg §11.1).

So Track 02 can PASS its fidelity gates on a carrier where **zero** per-slot headroom
exists — in which case its entire q4 ladder buys fidelity to a reference that was
already optimal, and the pilot has **no economic value**. The headroom arm converts
Track 02's verdict from "faithful" to "worthwhile", and it is cheap enough to be worth
running unconditionally.

### C.3 EXP-R' — preregistered before dispatch

Frozen population D1–D4 (`I10-GROTLI-G2-CORPUS-FREEZE.md`, re-verified in-job).
**V1 not fetched; no fetch token in the job.** Backend `Brotli_q11/lgwin30`, same build
identity every arm, `BrotliEncoderVersion()` per row.

- **Arm H0 — identity.** Reuse Track 02's existing gate: `16/16` exact `O11` carrier
  hashes. Mismatch ⇒ `INVALID-EXP-R'`, no interpretation.
- **Arm H1 — headroom (the real kill test).** Outcome-blind slot sample, G4 §10.2
  discipline: `N = min(6, eligible_column_count(f))`, ordered by
  `SHA-256(canonical_bytes(file_identity) ‖ 0x1F ‖ shape_id_le_u64 ‖ 0x1F ‖ slot_id_le_u64)`,
  ties by lower `(shape_id, slot_id)`. For each sampled slot, emit one complete carrier
  per alternative eligible leaf under the frozen `REGION_MIXED` mask, excluding the
  `O11` choice:

  ```
  Δ_sub(f,i)  = C(carrier with slot i → ℓ)  −  C(O11 carrier f)
  headroom(f) = Σ_i  min over ℓ of Δ_sub(f,i)          # ≤ 0 is an improvement
  ```

  Cost ≤ 6×4 = 24 per file, **≤ 96 total**, plus the `O11` floor already paid by
  Track 02.
- **Arm H2 — coherence (diagnostic only, no kill authority).** Same 6 slots, each
  reverted to `RAW_LEX`: `Δ_revert(f,i) = C(slot i → RAW_LEX) − C(O11,f)`. ≤ 24 encodes.
  **I do not give this arm kill authority.** The reviewer's
  `Σ_i max(0, Δ_revert)` measures coherence *against raw only*; it says nothing about
  coherence among the non-raw leaves. Its verdict is a characterization, not a gate.
- **Correctness, mandatory before any byte number:** exact roundtrip on every emitted
  carrier; all 19 G3 §15 adversarial decoder tests pass **unchanged**; call counters
  deterministic across repeated identical runs.
- **No timing claim.** Timing is Track 02's existing separate arm under G5 §13.4.

**Frozen thresholds.** All fractions of `C(O11 REGION_MIXED, f)`.

```
KILL-ROUTING (economic headroom exhausted):
    headroom(f) ≥ −0.0025  on ≥ 3 of 4 files
    ⇒ the best achievable per-slot substitution beats O11 by < 0.25%
    ⇒ NO-GO-EXP-R'-HEADROOM.  KILL every per-slot routing mechanism on this
      carrier.  Do not add a basis.  Report to Track 02 that its fidelity
      gates are economically vacuous.

KILL-WIRE (region-metadata economics; registered independently, runs first):
    median over D1..D4 of  W_CDR(f) / (raw_brotli(f) − G3_selected(f))  ≥ 0.25
    ⇒ any region-segmentation layer spends ≥ 25% of the measured causal gain
      on region metadata.  On the A.3 arithmetic this is the EXPECTED
      outcome; it is registered first so it cannot be excused later.
    ⇒ NO-GO-EXP-R'-WIRE.  KILL all region-segmentation mechanisms
      (i.e. CDR and every successor of its shape) regardless of H1.

REPORT-ONLY (coherence characterization, H2):
    reported per file; no promotion or kill authority.

ECONOMIC-VALUE (what Track 02 is actually asking):
    headroom aggregate ≤ −0.0100   (≥ 1.0% aggregate available)
    AND worst-file headroom ≤ −0.0025 on ≥ 2 of 4 files
    ⇒ interaction-or-fidelity aside, there is ≥ 1% of *carrier* bytes to win
      by routing alone, and Track 02's q4 ladder is aimed at a real target.
```

The `ECONOMIC-VALUE` bar is **4× the fidelity bar** (1.0% vs 0.25%) because this is an
economic question and 0.25% is Track 02's *fidelity* threshold. That scaling is now
stated **before** any measurement, in contrast to my withdrawn Gate A.

### C.4 EXP-C — kept, reframed, and explicitly subordinated

The reviewer's cardinality gate (`EXACT_DICT` iff distinct/occurrence ratio exceeds a
frozen threshold, `R = 0`, no new wire field, reuse G3's leaf-ID byte, charge 3 B as an
algorithm constant per the A5 disclosure precedent) is the cheapest possible probe of
per-slot headroom. I accept it and add one correction and one demotion.

**Correction:** it is **not** wholly novel ground, and it must not be presented as
either a mechanism or a discovery. G2 already measured the *decisions* such a rule must
reproduce — D2 `P-DICT` selected **43 `EXACT_DICT` of 339** columns, D4 selected **724
of 1,246** (`I10-GROTLI-G2-RESULTS.md` §D2/§D4) — and G2's own attribution states D2
*"was not missing a more complicated entropy backend. It was missing an exact
low-cardinality token representation before Brotli."* So the hypothesis is pre-answered;
what is unmeasured is whether a **one-parameter** rule reproduces `O11`'s choices. That
is a diagnostic question about **Track 02's `P0` model**, not a candidate mechanism.

**Demotion — and this is the Track 02 interaction the reviewer's plan does not handle.**
G5 prereg §1.2 forbids using *"a learned model whose features, solver, weights, or
constants differ from this contract"*, and §18.1 fixes the q4 strata, the UCB
multiplier, and the tie rule before measurement. Bolting EXP-C into the Track 02 job as
a *routing arm* would violate that contract. It may therefore be run **only** as a
**parallel, non-interfering reported row** — same job, same frozen build, same corpus,
reporting its own carrier bytes — with **zero influence** on `P0`, `P1`, the ladder, the
shortlist, the call budget, or any gate. If it cannot be run that way, it must wait for
a separate authorization; it may not be smuggled into Track 02's decision path.

Its diagnostic value, stated honestly: **if a one-parameter cardinality threshold
reproduces `O11`'s leaf choices at ≥95% agreement, Track 02's eight-feature ridge is
buying nothing and the pilot should be simplified to the threshold.** That is a
legitimate and cheap result either way, and it is adopt-class
(FastLanes/Parquet/ORC/BtrBlocks), so **no novelty is claimed and none may be recorded.**

---

## PART D — Revised recommendation

> ## **KILL** CDR-as-specified. **PILOT** one zero-extra-overhead addendum arm to
> Track 02: EXP-R′ (headroom `H1` + wire `KILL-WIRE` + coherence `H2`) plus the
> parallel reported-only EXP-C row.
>
> **Do not build:** the rank-1/full-table displacement model, the `O(1)`-probe
> production claim, any region-segmentation layer above frame groups, ARM-0's
> globally-fitted position table, ARM-1's q11-in-the-decision ranking.
>
> **Do not claim:** novelty for cardinality routing; any inherited speedup (G4's
> column is void); any held-out generalization (no unopened family exists);
> any region-level mechanism's viability (`KILL-WIRE` is the expected outcome).
>
> **Do not touch:** `src/anvil.cpp`, `FORMAT.md`, `RESEARCH_LEDGER.md`, `tools/*`, or
> any existing `docs/*` file other than this report.

**Why KILL for the mechanism and PILOT for the question.** All four structural defects
stand, and one of them (D-4) means CDR cannot be measured against frozen G3 at all —
it would void the `16/16` identity comparator that makes every G4 fidelity number
trustworthy. There is no version of CDR that survives the audit. Withdrawing is the
correct disposition, and **no gate was moved to avoid that**: Gate A, Gate C, and the
`S ≤ 1024` cap are withdrawn rather than re-tuned.

But the *lane* is not dead and neither is the question. The decisive number — per-slot
**economic** headroom on this carrier — has never been measured by anyone, and it costs
≤120 encodes *inside a job Track 02 is already running*. Killing track 16 outright would
spend a promoted pilot's fidelity budget on a reference whose economic worth is
unknown.

**What moves this to PROMOTE-TO-REMOTE:** `ECONOMIC-VALUE` met, `KILL-WIRE` not firing,
a Track 20 novelty ruling separating marginal-displacement pricing from Brotli
per-meta-block selection / zstd per-block coder trials / BLOT, **and** a newly frozen
independent structured family — which does not currently exist and must be created
before any promotion is legitimate.

**What kills track 16 outright:** `KILL-ROUTING`, or `KILL-WIRE`.

**Explicit falsification criteria for my own revised verdict.** The reviewer is wrong
— and I am wrong to have withdrawn CDR — if the measured `|shapes|` on D3 is small
enough (≤16) that `Θ(S)` identification is genuinely bounded and production-viable,
which would resurrect the `O(1)`-probe claim in separable form. D3's shape count is
unrecorded; D1/D2/D4 measured 1 / 2 / 42. EXP-R′ `H1` reports it as a diagnostic.
Likewise, if `KILL-ROUTING` does **not** fire and `ECONOMIC-VALUE` does, the reviewer's
"bounded per-slot router that fits no global position table" is the right successor
over anything I proposed, and I will say so.

---

## PART E — Open uncertainties that could still change this

1. **D3's exact shape count is unrecorded.** D1 = 1, D2 = 2, D4 = 42. This single number
   decides whether the *separable* form of the withdrawn mechanism was ever viable.
   `H1` reports it. Highest-value cheap measurement in the lane.
2. **Is Q2's small pairwise interaction the same phenomenon as S6-3's large trajectory
   effect?** `0.277632%` max pair gain vs `+0.0443%` trajectory-borne regression on a
   different corpus and backend path. Different objects. `H2` characterizes; it does not
   resolve. Still the deepest open question in track 16.
3. **Does the S6-3 calibration transfer to carrier granularity?** `[HARN-LX]` →
   `[GHA-24]`, single parse family, no container router, **never transferred**. My
   withdrawn Gate A depended on it; `EXP-R′` deliberately does not.
4. **E4's regret is scale-dependent** — `+643,453 B` at 16 MiB vs `+5,424,162 B` at
   256 KiB. Any future citation must state the scale; both figures are in
   `docs/audit-2026-09-07/13-e4-block-routing-oracle.md`.
5. **Track 02 is authorized but has no frozen workflow** (ledger PART XV pending
   handoff). `EXP-R′` is an arm *of* that job, so its dispatch is gated on Track 02's
   own workflow being frozen and published — an operator-authorization dependency, not
   a scientific one.
6. **G3 binary digest provenance.** Single-sourced in
   `docs/I10-GROTLI-G3-RESULTS.md` (unlike G4's, which appears in two places, and G5A's,
   which has a three-way identity). Length-valid; not independently recomputed here.
   Track 02's `H0` gate recomputes it. Recorded so it is not discovered late.
