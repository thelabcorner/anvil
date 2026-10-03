# Track 16 — Segmentation / Routing Theory (Fledge Alpha Free, independent adversarial review)

**Status:** INTERIM-FINAL for this lane; written after reading `docs/swarm-2026-10-02/16-segmentation-routing-space-bunny.md` in full.
**Date:** 2026-10-02
**Track:** 16 — region discovery, candidate pruning, regret bounds, fallback guarantees
**Role:** independent adversarial reviewer. I did not use the Space Bunny report to derive any number below; I re-derived every figure from `RESEARCH_LEDGER.md`, the G0–G5B results documents, the G3/G4/G5 preregistrations, and `src/anvil.cpp`. Where I quote Space Bunny, I am auditing it.
**Production source changed:** no. **Files modified by this lane:** this report only. No commit, push, reset, clean, stash, restore, or rebase. No local corpus benchmark, sweep, or fuzz campaign.
**Ruling:** **PILOT** — but of a *different* mechanism than the constructive lane proposes. **KILL the constructive formulation (CDR) as specified.**

---

## 0. Audit posture

The strongest thing the constructive lane did was *not* build anything, which is correct. But it also asserted an **estimability claim that its own experimental design cannot satisfy**, chose a **gate that an already-measured failure would pass**, sized the mechanism against the **wrong denominator**, and proposed a **carrier granularity that the frozen G3 carrier cannot express**. Those are four independent defects. Any one of them is disqualifying; together they are a kill.

I am not attacking the *idea* that representation choice is interaction-dominated. That may well be true and it is the most interesting open question in this lane. I am attacking the specific mechanism, the specific accounting, and the specific gate.

---

## 1. Evidence and provenance audit

### 1.1 Measured facts I independently re-derived from primary artifacts

| # | fact | value | provenance |
|---|---|---:|---|
| E1 | G3 D3 regionized best vs raw Brotli | 1,240,155 vs 1,292,757 B = **−4.0690%**, saving **52,602 B** | `docs/I10-GROTLI-G3-RESULTS.md` §1–2; run `35947013432` |
| E2 | G3 four-file routed portfolio | 1,466,769 → 1,384,654 B = **−5.598359%**, saving 82,115 B | same |
| E3 | G3 D3 regionization anatomy | **11,228 structured frames + 1 raw residual frame** | same §2 |
| E4 | G3 held-out V1 outcome | 2,179,615 vs 2,112,235 B = **+3.1900% worse**; 11,444 frames, **100% structured, 0 residual, 3 exact shapes, 0 integer leaves** | same §3 |
| E5 | G4 identity gate | O11 vs independently built frozen-G3 carrier **16/16 exact** | `docs/I10-GROTLI-G4-RESULTS.md` §1 |
| E6 | G4 proxy worst-file regret (**corrected**) | S0 **6.6056%**, S1 **7.0802%**, S2 **1.5388%** | `RESEARCH_LEDGER.md` PART XV §G4 |
| E7 | G4 speed column **withdrawn** | 1.0218× / 0.4614× / 0.0591× = `INVALID_SPEED_ACCOUNTING` (O11 denominator read cached labels) | same |
| E8 | G4 Q2 pairwise separability | 24 pairs, max **0.277632%**, aggregate **0.057144%**, `SEPARABLE-ENOUGH` | `docs/I10-GROTLI-G4-RESULTS.md` §3 |
| E9 | G5A same-multiset totals | A0 2,054,532 / A1 1,470,205 / A2 1,452,383 / A3 1,380,245; A3−A1 = **89,960 B**; `ORDER-MATERIAL` / `COLUMN-DOMINANT` | ledger PART XV §G5A; run `35985412906` |
| E10 | G5A arithmetic check | 1,470,205 − 1,380,245 = 89,960 ✓; 674,287/2,054,532 = **32.8195%** ✓; 674,287/1,470,205 = **45.8635%** ✓ | my recomputation |
| E11 | G5A critical caveat | **A1 is extracted-chunk source order, already 3,436 B above aggregate raw Brotli**; valid claim is ordering *within a fixed carrier* only | ledger PART XV §G5A |
| E12 | G5B-ORDINAL corrected | B0/B1/B2 = 2,054,910 / **1,380,245** / 1,403,029; B2 **+1.6507%** vs B1, wins 1/4; ruling `ORDINAL-ADVERSE` | ledger PART XV; run `36011333908` |
| E13 | E4 block-local backend routing | **DECISIVE NEGATIVE**, +0.64 MB…+5.42 MB regret; warmup tax grows monotonically as blocks shrink; classified **math** | `docs/audit-2026-09-07/13-e4-block-routing-oracle.md` |
| E14 | S6-3 local-cost calibration | **473,985 candidates, ZERO sign flips**, median \|err\| ≤16%, pessimistic direction; harness-relative, single parse family, no container router | ledger §S6-3 |
| E15 | S6-3 finding F1 | EXP. X's +0.0443% regression is **trajectory-borne, not token-borne**; post-parse rollback measured **unrecoverable** | ledger §S6-3 |
| E16 | AUTOSEG / A5 | adopt-class, byte-identical to forced-truth oracle on 10/10 anatomies, **166/166 boundaries**, +48 B failure-floor; `decode_auto` **does not search** | ledger PART XIV addendum §A5 |
| E17 | datastruct A6 | adopt-class; jsonl AUTO 1,199,898 B vs brotli-q6 206,840; **235/235 phases occupied**, phase entropy **7.76 vs 7.88 bits max** → fixed-period segmentation cannot lock phase on variable-length records | ledger PART XIV addendum §A6 |
| E18 | **No unopened held-out structured corpus exists** | "No new held-out structured corpus is currently available." V1 was consumed by G3 | ledger PART XV, portfolio decisions |
| E19 | G5 planner-pivot call budget | isolated sampled q4 ≤ **80**, whole-carrier ladder 8/4/2/2, **q11 rank = 0**; pilot is a feasibility experiment | `docs/I10-G5-PLANNER-PIVOT-PREREG.md` §1, §14.1 |
| E20 | ANVIL's only shipping routing path | `--ratio-backend=auto` = **per-file** candidate selection, +159 B framing over oracle; framing negligible; sub-block framing is BWT-only (`src/anvil.cpp:4400-4428`, `4614-4639`) | `src/anvil.cpp` |

I re-derived E1, E2, E9, E10, E12 arithmetic from the recorded byte values. All check out exactly. The ledger's G4 correction (E6/E7) and the G5A caveat (E11) are present and correct in the ledger and are *not* present in the G4/G5A results documents — the ledger is the authority.

### 1.2 Where the constructive lane is correct and I concur

- **Regionization is causal and worth 52,602 B on D3 with an unchanged leaf vocabulary and backend** (E1/E3). Agreed.
- **Granularity is not universally good**: V1 was 100% structured, 0 residual, 3 shapes, and the regionized representation was **3.19% worse** (E4). Any "more regions is better" premise is dead. Agreed.
- **Per-region *backend* invocation is closed by math** (E13). Any routing must occur inside one carrier under one backend pass. Agreed and reinforced.
- **G4 speed numbers are void** (E7) and cannot be inherited by any successor. Agreed.
- **Wire cost scaling linearly in R is the dominant cost risk.** Agreed — and I make it worse below (§3.2).

### 1.3 Provenance defects found in the constructive report

**D-1 (confirmed mis-transcription).** Space Bunny §1.1 quotes the frozen G3 binary SHA-256 as
`ac980f35feeefde0007c8f3aa0b5fd0c2262866ff66526b8c7d100c5117ab537`.
`docs/I10-GROTLI-G3-RESULTS.md` line 6 actually reads
`ac980f35feeefde0007c8f3aa1b5fd0c2262866ff66526b8c7d100c5117ab537`.
The subsequence is `aa1b5f`, not `aa0b5f`. Space Bunny then raises an *integrity concern* about this very digest (§1.8, §15.5) while having mis-copied it. The concern is legitimate; the transcription is not. Any downstream identity gate that copies Space Bunny's string will fail closed against the real artifact.

**D-2 (unverified citation presented as evidence).** Space Bunny cites the G5 pilot's 80 q4 cap (E19) as the frozen accounting against which CDR's `K_probe = 5` is "6.25%". But the G5 planner pivot is **not frozen as executed evidence** — ledger PART XV records it as *"reports missing links and has no frozen workflow"*. A call-count comparison against an unfrozen preregistration is not a like-for-like comparison; it is a comparison against a plan.

**D-3 (unrecorded quantity used as a fixed assumption).** Space Bunny §5.2 instantiates the displacement table at "conservative `|shapes| = 64, |buckets| = 16` ⇒ 2,048 B" and Gate C caps `S ≤ 1024`. **D3's exact shape count is not recorded in any results document** (Space Bunny concedes this in §15.4). V1's was 3; D4's was 42. D3 is a heterogeneous GH Archive stream — the one file carrying ~88% of the aggregate. The table term is therefore a guess precisely where it matters most, and it is not bounded by any measurement.

**D-4 (terminology error with mathematical consequence).** Space Bunny calls its model "rank-1 in the (shape × column-ordinal-bucket) basis" while writing `b = (b_1 … b_{|shapes|×|buckets|})` — a **full 2-D table**, which is the opposite of rank-1. This is not pedantry; it is the entire mechanism. See §2.

**D-5 (window splicing in the decode table).** Space Bunny §5.3 computes decode cycles from "an assumed 1 GB/s decode and 3.0 GHz" on the D3 payload. E7 voids the project's timing accounting; there is no citation-grade 1 GB/s decode figure for a G3 D3 carrier in any frozen window. The ratio column is presented as the reportable quantity, which is a partial mitigation, but the absolute cycle counts are presented in a table alongside measured bytes without the window tag discipline the report itself mandates in §0. **In a 40-agent swarm this is the single most likely integrity failure and the report names it as failure mode #10 while committing it.**

### 1.4 Do-not-reburn overlap check — the constructive lane is re-opening a closed route in part

- **E4 (§G1 of the do-not-reburn register)** closes "block-local / sub-file backend routing" and names **P1.2 (block-local routing oracle) and E5 (cheap router from oracle labels)** as **PARKED / not runnable**. CDR is a coarse, backend-free variant of the same "route at finer-than-file granularity" idea. The reopen condition is explicit: *"a backend with near-zero model warmup, or cross-block model carryover."* CDR satisfies neither — it does not add carryover, it adds a *new* predictor. **On the register's own terms, CDR has not met the reopen condition.** It is a different object (leaf choice, not backend choice), but the register's reasoning — warmup/warm-start state resets destroy the marginal value of locality — transfers directly to region-level *leaf* switching, and Space Bunny does not address it.
- **F0.1** closes BWT-routing arithmetic: routing cannot fix the decode axis.
- **A6/E17** measures that fixed-period segmentation fails on variable-length records. CDR's region *boundaries* come from frame-group runs, which is content-addressed and therefore safe from E17 — Space Bunny is right about that — but it inherits E17's *general* lesson that region geometry is unstable under input perturbation, which bears on the R-census risk in §3.

---

## 2. Strongest falsification case against CDR

### 2.1 The kill shot: `O(1)` probes and `Θ(S)` identification cannot both be true

Space Bunny's central production claim (§4.3, §7) is that the displacement table is priceable from **`|L| = 5` whole-carrier decompositions, independent of `R`**. Its own experimental design (§9 step 3) then asks to **fit the interaction term against the shape × bucket basis at `S ∈ {16, 64, 256, 1024}`**.

These are incompatible.

Each whole-carrier probe returns **one scalar**. A single probe places leaf ℓ at its ordinal positions with all other positions at the default, and the measured delta is the **sum** of every displacement contribution inside that probe, confounded with every byte that changed. With 5 probes you obtain at most **4 independent contrasts**. At `S = 16` you are 4× short; at `S = 1024` you are **256× short**. There is no differencing scheme that fixes this — the confound is not noise, it is *aliasing of unmeasured parameters into each measurement*.

Therefore:

> **Either (a)** `b` is genuinely low-rank (`b = u[shape] ⊗ v[bucket]`), in which case the mechanism is *not* "rank-1 in a 2-D table" but a separable model, identification needs `Θ(|shapes| + |buckets|)` probes, and — critically — the model is *position-in-ordinal-space is what matters*, which is exactly the object G5B-ORDINAL measured as `ORDINAL-ADVERSE` (+1.6507%, E12); **or (b)** `b` is the full table Space Bunny writes down, in which case identification is `Θ(S)` whole-carrier q11 encodes per file (at D3's measured q11 throughput, ~2 s per 1.24 MB encode ⇒ **~34 minutes for `S = 1024` on one file**), which is a perfectly acceptable *research* cost but must never be described as an `O(1)` *production* cost.

**And the branch that makes it production-viable — a globally fitted table of position constants — is precisely the object G5B measured as adverse.** CDR's value proposition is "cheaply predict that placement matters." G5B is the project's direct, measured, prior experiment on "cheaply predict that placement matters." It won by being exact and lost by being cheap: **B1 (exact placement) 1,380,245 B; B2 (cheap ordinal prediction) 1,403,029 B, +1.6507%.** Space Bunny cites G5B in §1.5 as evidence *for* the basis being valuable. The evidence is for the opposite proposition: **placement is valuable when exact and harmful when cheaply predicted.** CDR is cheap prediction.

### 2.2 Gate A does not discriminate — an already-measured failure passes it

Gate A (Space Bunny §10) sets the byte bar at:

```
PASS-A: leave-one-file-out predictive regret vs O11 ≤ 0.0200 worst-file
```

and justifies it as *"above S2's measured 1.5388% … so CDR must be a real improvement over the best existing surface."*

That justification is arithmetically inverted. **S2's measured worst-file regret is 1.5388% (E6), which is *below* the 0.0200 bar. A mechanism that reproduced S2's failure exactly would satisfy Gate A's byte criterion.** Gate A therefore cannot distinguish CDR from a re-run of the surface G4 already closed. It is not a discriminating gate.

Worse, the bar is disconnected from the economics. 2.0% of D3's C(O11) ≈ 1,240,155 B is **24,803 B**. G3's entire causal D3 gain is **52,602 B** (E1). So:

> **A mechanism that PASSES Gate A as written may give back 47% of G3's total measured win on the file that carries ~88% of the aggregate.**

G4's 0.25%/0.50% fidelity gates were about *fidelity to O11*, an internal reference where being close is the point. CDR's value is *economic*, so its gate must be expressed as a **fraction of available headroom**, not an absolute regret floor inherited from a fidelity experiment. Space Bunny never measures headroom.

### 2.3 The interaction/local decomposition in Gate A is not identifiable

Gate A also requires `interaction_share = |b|/(|a| + |b|) ≥ 0.30`. (As literally written, `|b|/|a| + |b|` is dimensionally incoherent — presumably `|b|/(|a|+|b|)` was meant. A frozen gate must be dimensionally well-formed.)

More substantively: decomposing an O11-vs-proxy residual into a "local" part and an "interaction" part **requires a definition of `ĉ` that is independent of the model being tested**. Any residual can be pushed into either bucket by choosing where to draw the local/interaction line. Unless `ĉ` is frozen to an external, pre-existing estimator — and the only candidate is S6-3 (E14), which Space Bunny simultaneously (a) calls calibrated and (b) flags as a **harness-relative Linux single-parse-family measurement with no container router** (E14, Space Bunny §15.3) — Gate A's second clause is a description of the answer rather than a test of it. Space Bunny is explicit that the `[HARN-LX] → [GHA-24]` transfer "has not been made and is explicitly not allowed to assume." Then Gate A depends on it.

### 2.4 Mechanism-correctness blocker: CDR's region granularity cannot be expressed in the frozen G3 carrier

Space Bunny §4.1 defines a region as *"a maximal run of consecutive G3 frames sharing one shape and one leaf assignment"* and §6.1 E7 asserts *"E7 reuses G3's builder verbatim, which is what keeps attribution clean: the only causal change vs the frozen G3 arm is the `λ`/`Π` selection."*

This is false against the frozen carrier. In G3/G4 the leaf is selected **per (shape, slot)** — a slot is a shape's placeholder *column*. G4 §4.0's family masks restrict leaves per slot; G2's D2 result reports per-slot leaf counts (296 RAW_LEX + **43 EXACT_DICT** columns; D4: 1,246 slots, **724** EXACT_DICT). A *region* is a run of frames sharing a leaf assignment **across all slots simultaneously**. Those are different granularities, and only two things can happen:

1. **CDR's region λ is redundant.** The per-slot leaves already transmitted by G3 fully determine the wire. Then the region stream is pure added cost, the routing decision changes nothing, and `W_CDR` is 100% waste.
2. **CDR overrides the per-slot leaves.** Then the carrier grammar changes, the decoder changes, and the *"only causal change is λ/Π"* claim — and with it the entire attribution against frozen G3 (E5's 16/16 identity comparator) — is void.

There is no third option. This is a correctness blocker on the experimental design, not a tuning concern.

### 2.5 Gate D is arithmetically likely to fail ARM-1, using the lane's own numbers

Using Space Bunny's own measured windows (and flagging that they are spliced, per §1.3 D-5):

- O11 candidate scoring on D3: "on the order of 50k-ish isolated Brotli q11 candidate evaluations … ~32 s" (`I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md` §9.1).
- Whole-file Brotli q11/lgwin30 throughput: `brotli-q11-lw30` compresses dickens (10,192,446 B) in 16.56 s ⇒ **0.615 MB/s** (`tests/auto-routing.v2.csv`, `[LOCAL-I9]`, single-run, ranking-grade only).
- CDR ARM-1 cost on D3: 5 whole-carrier q11 encodes × 1.24 MB / 0.615 MB/s ≈ **10.1 s**.
- Implied symmetric speedup: 32 / 10.1 ≈ **3.2×**.

Gate D requires **≥ 5.0×**. ARM-1 lands below it on the lane's own arithmetic. ARM-0 escapes only by having `b` come from "G5A-derived analytic constants" fitted on D1–D4 — i.e. a globally fitted table, i.e. branch (a) of §2.1, i.e. G5B's `ORDINAL-ADVERSE` object.

*(These windows are spliced and the q11 throughput is single-run; that is exactly why this is a structural objection to Gate D's design, not a prediction of its outcome. A properly windowed symmetric measurement must still be run — but it must be run before the mechanism is built, not after.)*

---

## 3. Hidden costs, quantified

### 3.1 Byte cost — Space Bunny's arithmetic is right; the **denominator** is wrong

I recompute Space Bunny's `W_CDR = 1.375·R + 2·|shapes||buckets| + 12` and confirm it at `R = 3,011` (6,201 B, 0.500% of payload) and `R = 11,229` (17,500 B, 1.411%). The formula is correct.

The error is comparing `W_CDR` to the **payload**. The relevant denominator is the **measured causal gain the regionized architecture already earned**, because that is the pool the new mechanism is spending from:

| cell | G3 selected | raw | **causal gain** | `W_CDR` at `R = frame count` | **`W_CDR` / gain** |
|---|---:|---:|---:|---:|---:|
| D1 (793 frames, 1 shape) | 39,277 | 40,126 | **849 B** | 1.375×793 + 2,048 + 12 = **3,151 B** | **371%** |
| D2 (1,192 frames, 2 shapes) | 20,861 | 25,029 | **4,168 B** | 1,375×1,192 + 2,048 + 12 = **3,699 B** | **88.7%** |
| D3 (11,228 frames) | 1,240,155 | 1,292,757 | **52,602 B** | 17,500 B | **33.3%** |
| D4 (3,718 frames, 42 shapes) | 84,361 | 108,857 | **24,496 B** | 7,172 B | **29.3%** |

> **This is the structural kill on the economics. The region wire cost is *anti-correlated* with the value of routing.** D2 has 2 shapes and 1,192 frames: routing freedom is nearly nil (two shapes ⇒ the per-slot leaf set is almost constant) while the wire charge consumes **88.7% of the entire measured win**. D3 has the most frames and therefore the *largest* absolute wire charge, on the file that decides the aggregate. D4's selected arm under G3 was `G2_WHOLE`, not regionized at all (E2) — so on D4 CDR's regions are not even the arm the portfolio selected.

Note also that at small `R` the **2,048 B table term alone is 49% of D2's entire causal gain** (2,048 / 4,168). The table does not amortize on precisely the cells where the measured wins are concentrated.

### 3.2 Granularity explosion — the pessimistic branch is the *likely* branch

Under CDR's own definition (maximal runs of frames sharing shape **and** full per-slot leaf assignment), `R` equals the frame count whenever leaf assignment or shape varies frame-to-frame. Measured frame counts: D1 793 (E: D1 anatomy), D2 1,192 records (G1 §7), D3 **11,228** (E3), D4 3,718 (G1 §5). D3 is a heterogeneous GH Archive event stream — shape interleaving is the expected case, so `R ≈ 11,228` is the *likely* branch for the cell that carries ~88% of the aggregate. Space Bunny's `R = 3,011` break-even is a **hope**, not a measurement, and it is the optimistic branch.

The merge objective (§4.4) is proposed as the defense. But merging adjacent regions *whose leaf assignments differ* is exactly discarding the routing decision. The merge can only collapse regions that already agree, which are the ones that needed no route. **The merge objective cannot reduce `R` on the cases that motivate CDR.**

### 3.3 Fragmented metadata — G5 already absorbed this cost

G3 already transmits, per slot, leaf IDs and leaf payload lengths (G3 §13 complete-byte accounting). G5A already transmits frame→group reconstruction order and group descriptors. CDR adds a **third** orthogonal description of the same structure: region length deltas + region leaf IDs. That is not new information — it is a **coarser restatement** of bytes already on the wire. Charging it is correct; the fact that it is redundant is the problem. The A5/A6 precedent (E16, E17) shows ANVIL's boundary tables are genuinely cheap *because they replace* a search, not because they are added to one.

### 3.4 q-call cost — oracle dependence is not discharged

- **ARM-0** = 0 backend calls, but requires a globally fitted position table fitted on **spent** data (D1–D4; V1 consumed by G3 per E18). With `n = 4` and no unopened held-out family (E18), the leave-one-file-out protocol in §9 step 3 is not a validation protocol — with four points, LOFO has no usable variance estimate and every one of the four folds is a different corpus.
- **ARM-1** = 5 whole-carrier **q11** encodes used *inside the routing decision*. G4 §1 freezes the doctrine: **"q11 is a verifier, not a ranking primitive."** Using q11 to *rank* region representations is exactly the banned use.
- **Neither arm escapes G4's withdrawn-speed regime** (E7). Neither may quote an inherited speedup.

### 3.5 Decoder cycles — self-refuting at the design point

Space Bunny's own projection (§5.3): `R = 11,229` ⇒ **6.04% of decode cycles**, at an assumed 1 GB/s. MASTER-BRIEF §12 records that **I9 wire-invisible decode optimization is exhausted as a crossing route** and that remaining decode legs are `DECODE-SHORT`. So at the likely `R`, CDR places itself — by its own arithmetic — inside the territory the project has already closed, while spending 33% of G3's causal gain on metadata. CDR is therefore viable only inside a narrow, **unmeasured** `R` band. A mechanism whose viability window is unmeasured is a HOLD, not a PILOT.

### 3.6 Memory / RSS

- Decoder RSS delta 0: plausible *if* the region stream is streamed. But region dispatch requires the region extent before the leaf decode begins, and G3's leaf payloads are length-prefixed within group descriptors — so either the region length stream is redundant (a refund nobody claimed) or a pre-pass is required. **The "0 bytes" claim is contingent on an unstated structural assumption and cannot be both true and charged as §5.2 requires.**
- §5.1 lists "decoder-visible tables **0 bytes**" while §5.2 charges `W_CDR` of new decoder-visible bytes. These cannot both be reported as written.
- Encoder RSS delta: the table is `|shapes|×|buckets|×8 B`; at Space Bunny's 64×16 that's 8 KB, but at `S = 1024` (§7 cap) with 8 B entries it's 8 KB *of table* against a claimed 2 KB *on the wire* — the on-wire charge is a 16-bit base table while the encoder holds 8-byte floats. **The wire charge and the working charge are different objects and only one of them is in `W_CDR`.** That is a legitimate choice but must be stated, not buried.

---

## 4. Prior-art map (adversarial, and it is worse than the constructive lane rates)

Space Bunny self-rates the novelty MEDIUM-HIGH and explicitly defers the search to Track 20. As the adversarial reviewer I must state the risk as I see it: **HIGH — likely adopt-class.**

| line of art | what it already does | damage to CDR |
|---|---|---|
| **Brotli meta-block + static/dynamic block-type selection** (RFC 7932; `BrotliBuildMetaBlock` chooses static vs dynamic context/literal per meta-block; block splitting in `BrotliEncoderCompressStream` q0–q1) | per-region *representation* selection **inside one backend pass**, priced against a literal/entropy cost model | CDR's structural shape is a generalization of a shipping, decades-hardened mechanism. The distinguishing claim narrows to "price *marginal displacement on the remainder*" — a quantitative refinement, not a new interaction. |
| **zstd `ZSTD_optimal` / `ZSTD_splitBlock` / `BlockSplitter` / opt-level-3 per-block coder trials** | splits blocks on entropy/literal heuristics; at opt ≥3 trials `fast`/`greedy`/`btlazy2` per block | this is *multi-fidelity per-region representation selection*, i.e. CDR's method with a different fidelity ladder |
| **zstd `ZSTD_c_targetCBlockSize`, superblocks** | learned/heuristic sub-block sizing for compression-ratio targets | region granularity as a tunable is established |
| **LZMA `SetCutValue` / `LzmaEnc_EncodeBlocks`, LZMA2 chunking, xz block index** | cut-penalty segmentation with a charged per-block restart cost | the `ρ·R` merge objective (§4.4) is a cut penalty |
| **7-Zip coder graph / bsc / lrzip / paq8 multi-coder** | evaluate several coders per block by marginal benefit, keep the winner | per-region codec selection by measurement is standard |
| **BLOT / blocally-compressible data model (Pireddu, Langdon; ℓ-bracket, "blocally compressible")** | explicitly formulates *choosing the representation per block* as an MDL problem, with the block-local model itself as a selectable option | **direct hit on CDR's central formulation.** This is the closest prior art and Space Bunny does not mention it. |
| **FastLanes (Parquet/ORC/ClickHouse), BtrBlocks, ALP** | **cardinality/width-driven automatic per-column choice** among raw / RLE / dictionary / delta / FOR | damages CDR *and* my own alternative below (§6). Must be stated. |
| **racing / multi-fidelity / learning-curve (Li & dy-sk; Fusi et al.; Gormley et al.)** | cheap low-fidelity probes → promote the promising few → measure exactly | CDR's ARM-1 is a racing scheme with 5 arms and no promotion phase |
| **PELT / Bayesian online changepoint detection** | MDL segmentation with a penalty | change-point segmentation is statistical methodology |

**Consequence:** the only defensible novelty residue I can find in track 16 is a *quantitative separator* — "marginal displacement on the carrier remainder is low-rank in a shape × ordinal basis and is therefore predictable from a bounded number of exact probes." That is a real, testable, quantitative claim. It is **not** a new interaction, and it must clear: (i) G5B's `ORDINAL-ADVERSE` prior on cheap positional prediction, and (ii) the identification-count arithmetic in §2.1, before it can be called a novelty. Until then it is a **measurement claim**, not a mechanism claim.

---

## 5. Distribution shift and fallback guarantees

- **Fallback is genuinely inherited and free.** `RAW_BROTLI` always eligible, raw wins exact ties (G3 §11); S6-3's measured-payload arbitration measured **Δ = +0.0000% exact, CV = 0** across 22 file-runs (E14). I concur that CDR cannot produce a selected-byte regression. Space Bunny is right that the downside is "paying `W` for nothing," not a regression — and §3.1 shows "paying `W`" is between 29% and 371% of the gain.
- **There is no held-out set to shift to.** E18: no unopened held-out structured corpus; V1 was consumed by G3 and is `KNOWN-STRESS`. Any CDR pilot closes on **spent** data with a four-file leave-one-out. Gate E is therefore not a validation gate; at best it is a stress replay. Space Bunny presents Gate E as if it were held-out. It is not, and its own §11.7 acknowledges the collapse risk while §10 still calls it `HELD-OUT V1`.
- **Region-boundary instability.** E17 measured that fixed-period segmentation cannot lock phase on variable-length records (235/235 phases occupied, 7.76 vs 7.88 bits). CDR's frame-group-derived boundaries avoid that specific failure, but the general result — region geometry is a high-entropy function of input alignment — means `R` is **not stable under any perturbation of the input that changes record lengths**. A `R` census on four fixed corpora does not bound `R` on a fifth. Gate C (`R ≤ 3,011` on D3) is a measurement of four specific files, not a bound.

---

## 6. Alternative mechanism (justified by the project's own measured evidence)

The decisive observation from §2.4 and §3.1: **the frozen G3 carrier already transmits a leaf ID per (shape, slot).** Therefore a mechanism that reuses that existing field costs **zero new decoder-visible bytes**, and a mechanism that adds a *region* layer costs `Θ(R)` new bytes it does not need.

> **EXP-C — cardinality-gated leaf routing (zero-new-wire, decoder-free, no segmentation decision).**
>
> Select `EXACT_DICT` for a slot **iff** an exact encoder-side cardinality statistic exceeds a frozen break-even; otherwise `RAW_LEX`. No region segmentation at all (`R = 0`). No new wire field. No backend call in the decision. The threshold is a single frozen constant, calibrated once, charged as an algorithm constant (the A5 disclosure precedent, E16: "a stricter protocol charges 2–3 B" — so charge 3 B).
>
> The predictor variable is **distinct-token count / occurrence count per slot**, an `O(n)` encoder-side scan. This is not an invention: **G2's D2 result already identified it.** `docs/I10-GROTLI-G2-RESULTS.md` §5, verbatim: *"D2 was not missing a more complicated entropy backend. It was missing an exact low-cardinality token representation before Brotli."* D2 P-DICT selected **43 of 339** slots as `EXACT_DICT`; D4 selected **724 of 1,246**. Those are exactly the decisions a cardinality threshold must reproduce.
>
> **Honest novelty label: adopt-class / infrastructure** (FastLanes, Parquet, ORC, BtrBlocks all do cardinality-gated per-column selection — see §4). Its value here is not novelty. Its value is that it is the **cheapest test of the one question track 16 actually needs answered**: *is there any per-slot routing headroom at all on this carrier?*

**Why this dominates CDR on every axis:**

| axis | CDR | EXP-C |
|---|---|---|
| new decoder-visible bytes | `1.375·R + 2S + 12` = 3,151–17,500 B (**29–371% of the causal gain**) | **0** (reuses G3's leaf-ID field) |
| backend calls in the decision | 0 (ARM-0) or 5 q11 (ARM-1, violates G4 §1 doctrine) | **0** |
| identifies its own parameters | `Θ(S)` or `Θ(|shapes|+|buckets|)` probes (§2.1) | **1 frozen scalar**, fitted on spent data |
| prior measured precedent on the *same* move | G5B `ORDINAL-ADVERSE` (+1.6507%) | G2 D2 attribution, −19.34% vs G1R |
| segmentation risk (§5, E17) | `R` unmeasured, unstable | **none — `R = 0`** |
| held-out evidence available | none (E18) | none (E18) — identical, so neither can be promoted |

EXP-C does **not** claim novelty and I will not let it be presented as such. It claims only: *cheapest available falsifier of the routing-headroom hypothesis.* That is exactly what a lane whose distinguishing measurement has never been taken should be doing.

---

## 7. Decisive remote-only falsification experiment

**Nothing below runs on this workstation.** No local corpus benchmark, sweep, or fuzz campaign. No `anvil_bench`, no `paired_bench.py`, no `tools/block_oracle.py`. Everything is a bounded, byte-only, preregistered GitHub Actions job on the pinned Ubuntu runners, using the same frozen-materialization discipline as G3/G4/G5A (materialize `tools/grotli_g3.cpp` at `1a3d18fe…` and `tools/grotli_g4_planner.cpp` at the frozen G4 blob; verify blob + SHA-256 in-job; any mismatch ⇒ `INVALID-EXP-R`, no interpretation).

### EXP-R — routing headroom, region census, and interaction discriminator

**Frozen population:** D1–D4 identities from `docs/I10-GROTLI-G2-CORPUS-FREEZE.md` (re-verified). **V1 is NOT fetched.** No fetch token in the discovery job. Backend: `Brotli_q11/lgwin30`, same build identity for every arm; `BrotliEncoderVersion()` recorded per row.

**Arm R0 — floor (frozen identity gate).** Rebuild frozen G3 and frozen G4; require **16/16** exact O11 carrier hash equality against the recorded G3 identities. Any mismatch ⇒ `INVALID-EXP-R`.

**Arm R1 — region census (byte-only, zero new codec code).** For each file, under CDR's *own* coarsening definition (maximal run of consecutive G3 frames sharing one shape **and** one full per-slot leaf assignment under frozen `O11 REGION_MIXED`), report:

- `R` (region count);
- `frames_that_are_their_own_region` (fraction, the `R = frame_count` indicator);
- `W_CDR = 1.375·R + 2·|shapes|·|buckets| + 12` at the frozen `|buckets| = 16` and the **measured** `|shapes|` per file (closing defect D-3);
- `W_CDR / (raw_brotli − G3_selected)` — the causal-gain denominator of §3.1.

**Arm R2 — single-slot revert (the interaction discriminator).** Outcome-blind slot sample: exactly the G4 §10.2 discipline, `N = min(6, eligible_column_count(f))`, ordered by `SHA-256(canonical_bytes(file_identity) || 0x1F || shape_id_le_u64 || 0x1F || slot_id_le_u64)` ascending, ties by lower `(shape_id, slot_id)`. For each sampled slot, emit **one** complete carrier with **only that slot** reverted from its `O11` choice to `RAW_LEX` (always eligible), all other slots held at `O11`:

```
Δ_revert(f,i) = C(carrier with slot i → RAW_LEX)  −  C(O11 carrier f)
```

Cost: 6 encodes/file + 1 `O11` floor = **7 per file, 28 total.** Deterministic.

**Arm R3 — single-slot substitution (the headroom probe, the real kill test).** Same 6 sampled slots. For each, emit one complete carrier per **alternative eligible leaf** (up to 4, restricted by the frozen `REGION_MIXED` mask, excluding the `O11` choice):

```
Δ_sub(f,i)   = C(carrier with slot i → alternative leaf ℓ)  −  C(O11 carrier f)
headroom(f)  = Σ_i min over ℓ of Δ_sub(f,i)        # most negative = best achievable
```

Cost: ≤ 6 × 4 = 24 per file, **≤ 96 total**, plus the R2 set, plus 1 floor per file: **≤ 132 whole-carrier q11 encodes across all four files**, bounded and preregistered before dispatch.

**Mandatory correctness for every emitted carrier in R1–R3:** exact byte-for-byte roundtrip **before** any byte number is reported; all 19 G3 §15 adversarial decoder self-tests pass **unchanged**; deterministic call counters identical across any repeated identical run (a changed counter ⇒ `INVALID-EXP-R`).

**No decode or timing claim in EXP-R.** Timing is a separate later arm and must not be inferred.

### 7.1 Frozen thresholds (registered before dispatch)

All regret figures are fractions of `C(O11 REGION_MIXED, f)` for that file.

```
PRECONDITION (identity):   16/16 exact O11 carrier hashes                → else INVALID-EXP-R

KILL-ROUTING (headroom exhausted):
    headroom(f) ≥ −0.0025 for ≥ 3 of 4 files
    ⇒ the best achievable per-slot substitution beats O11 by < 0.25%
    ⇒ NO-GO-EXP-R-HEADROOM. KILL every per-slot routing mechanism on this
      carrier, including CDR, EXP-C, and any successor. Do not add a basis.

KILL-INTERACTION (diagnosis refuted):
    Σ_i max(0, Δ_revert(f,i)) ≤ 0.0025 · C(O11,f)  for ≥ 3 of 4 files
    ⇒ O11's per-slot choices are carrier-coherent; isolated scoring is not the
      problem; the G4 residual is proxy estimation error, not interaction.
    ⇒ NO-GO-EXP-R-INTERACTION. KILL the displacement/placement thesis outright.
      This refutes CDR's premise AND G5B's motivation AND Space Bunny §8.3.

KILL-WIRE (metadata economics, pre-registered and independent):
    median over D1..D4 of  W_CDR(f) / (raw_brotli(f) − G3_selected(f))  ≥ 0.25
    ⇒ any region-level mechanism spends ≥ 25% of the measured causal gain on
      region metadata.
    ⇒ NO-GO-EXP-R-WIRE. KILL all region-segmentation mechanisms regardless of
      the headroom or interaction results. (On §3.1 arithmetic this is the
      expected outcome; it is registered first so it cannot be excused later.)

SUPPORT (does NOT promote — see §8):
    ALL of:
      headroom aggregate ≤ −0.0100 (≥ 1.0% aggregate improvement available)
      AND worst-file headroom ≤ −0.0025 on ≥ 2 of 4 files
      AND Σ_i max(0, Δ_revert(f,i)) ≥ 0.0100 · C(O11,f) on ≥ 2 files
    ⇒ interaction is material AND there is headroom to capture.
       This licenses exactly one successor: a *bounded* per-slot router.
       It does NOT license CDR (identification-count arithmetic, §2.1),
       does NOT license a segmentation layer (KILL-WIRE), and does NOT
       license any promotion, because E18 leaves no held-out set.
```

**Reading the rules for the coordinator.** `KILL-ROUTING` and `KILL-INTERACTION` are **independent** of `KILL-WIRE`. Any one of the three ends track 16's routing direction. All three must fail for a successor to exist. `SUPPORT` is deliberately set at 4× the fidelity bar (1.0% vs 0.25%) because, per §2.2, this is an economic question and 0.25% is a *fidelity* threshold.

### 7.2 Why EXP-R rather than Space Bunny's EXP-1

EXP-1 asks whether a *particular* model (rank-1, shape × bucket) explains a residual. EXP-R asks whether **any** headroom exists and whether **any** interaction exists, before committing to a basis. If `KILL-ROUTING` fires, EXP-1 is unrunnable and its Gate A never mattered. EXP-R costs ≤ 132 encodes instead of a basis-sweep and cannot be invalidated by the identifiability problem in §2.1, because it never fits a model. It is strictly cheaper and strictly more decisive.

---

## 8. Explicit reconciliation with the constructive lane

> **Coordinator reconciliation addendum: §11–§14 below supersede parts of this section.** §11 independently verifies the Q2 interaction numbers from source and corrects their interpretation in *both* lanes. §12 states a proof that CDR-ARM-1 cannot identify its own model. §13 compares CDR's ≤5-probe information gain against Track 02's 80-q4 ladder. §14 is the post-reconciliation ruling. The headline ruling in §9 is **unchanged**.

**Agreements (verified independently):** regionization causality and its 52,602 B D3 value; V1's +3.1900% refutation of "more regions is better"; E4's math-closure of per-region backend invocation; G4 speed-column withdrawal; the `Θ(R)` wire-cost risk; the fallback guarantee as free and correctly characterized as "paying `W` for nothing"; the rank-1 terminology error D-4 is mine, not theirs — I found it while auditing; PILOT-over-KILL as a general posture.

**Disagreements, each with the decisive number:**

1. **Estimability.** Their §4.3/§7 claim `O(1)` probes; their §9 asks for `S` up to 1024. 5 probes give ≤4 contrasts. **Either the mechanism is separable (⇒ G5B's `ORDINAL-ADVERSE` object) or identification is `Θ(S)` (⇒ not an `O(1)` production cost).** Not reconcilable.
2. **Gate A is not discriminating.** Their 0.0200 worst-file bar is **looser than S2's measured 1.5388% failure (E6)**. A re-run of a closed surface passes it. And 2.0% of D3 = 24,803 B = **47% of G3's 52,602 B causal gain**. The gate is on the wrong scale.
3. **Wrong denominator in §5.2.** `W_CDR` must be charged against the **causal gain**, not the payload. On that basis the charge is **88.7% of D2's entire win** and **33.3% of D3's**, not "0.5% of payload."
4. **Carrier-expressibility blocker (§2.4).** G3/G4 leaves are per (shape, slot); CDR's regions are per run-of-frames. Either redundant or grammar-breaking. "E7 reuses G3's builder verbatim" is false in one of the two cases, always.
5. **G5B is evidence against them, not for them.** Exact placement won (B1 1,380,245); cheap ordinal prediction lost (B2 1,403,029, +1.6507%). CDR *is* cheap ordinal prediction. Their §1.5 reads it backwards.
6. **Gate D likely fails ARM-1 on their own arithmetic** (§2.5: ~3.2× vs a 5.0× bar), and ARM-0's escape hatch is the G5B object.
7. **No held-out set exists (E18).** Their Gate E is a stress replay, not validation. Their §10 header says `HELD-OUT V1`; the ledger says V1 is `KNOWN-STRESS` and no unopened structured family remains.
8. **Prior-art risk is HIGH, not MEDIUM-HIGH.** Brotli per-meta-block static/dynamic selection, zstd opt-level-3 per-block coder trials, 7-Zip's coder graph, racing/multi-fidelity, and especially **BLOT / blocally-compressible MDL block-representation selection** are prior art they did not search. They defer the search to Track 20; the honest verdict is that §8.2 is a measurement claim, not a novelty claim.
9. **Provenance defects D-1…D-5** (§1.3): a mis-transcribed G3 digest, an unfrozen-prereg call-count comparison, an unrecorded `|shapes|` used as a fixed assumption, the rank-1/full-table contradiction, and window splicing in the decode table while naming window splicing as failure mode #10.
10. **Disposition.** They propose PILOT of CDR. I propose **KILL CDR-as-specified** and **PILOT EXP-R + EXP-C**. The expensive, cleverer mechanism is the wrong first experiment; the cheap headroom probe decides whether *any* mechanism in this track is worth building.

**Where they are right and I was going to be harsher:** the fallback layer is genuinely free and their Gate E "alternative branch" (`PASS-E-FALLBACK`) is a legitimate, non-consolation outcome — reproducing G3's V1 shape while the selector correctly picks raw is real evidence the fallback works. And their insistence on not splicing windows is correct even where they then broke it.

---

## 9. Final recommendation

> # **PILOT** — of EXP-R (headroom/interaction/wire probe) and EXP-C (cardinality-gated leaf routing). **KILL the constructive CDR formulation as specified.**
>
> **Build:** EXP-R exactly as preregistered in §7, GitHub-Actions-only, byte-only, ≤132 whole-carrier q11 encodes across D1–D4, V1 not fetched, all three kill gates registered before dispatch. In the same CI job, add **EXP-C's zero-new-wire cardinality threshold** as a fifth reported row (it costs 3 B and reuses G3's existing leaf-ID field).
>
> **Do not build:** CDR's rank-1 displacement table, its `O(1)`-probe production claim, any region-segmentation layer above frame groups, and any timing arm.
>
> **Do not claim:** any novelty for cardinality routing (FastLanes/Parquet/ORC/BtrBlocks), any speedup (E7 void), or any held-out generalization (E18 — no unopened corpus exists).
>
> **Do not touch:** `src/anvil.cpp`, `FORMAT.md`, `RESEARCH_LEDGER.md`, `tools/*`, or any existing `docs/*` file other than this report.

**Why PILOT and not the alternatives:**

- **Not KILL of the whole lane.** The single most important quantity in track 16 — *is there any per-slot routing headroom on this carrier at all?* — has **never been measured**. G4 measured fidelity to O11; nobody measured economic headroom. A bounded 132-encode experiment that answers it is the cheapest high-information action available in this swarm, and the project's own encode-speed evidence (q11 at ~0.6 MB/s ⇒ ~1.6 s/MB) puts its cost at minutes of CI time.
- **Not KILL of CDR specifically.** If `KILL-ROUTING` and `KILL-INTERACTION` both fail to fire, then interaction *is* material and headroom *is* real — and the correct successor is a **bounded per-slot router that fits no global position table**, precisely because §2.1 shows a fitted position table cannot be identified from bounded probes and G5B shows cheap ordinal prediction is adverse. CDR's diagnosis may survive; its estimator does not.
- **Not HOLD.** HOLD would mean "no experiment until the direction is clear." The direction's decisive number is one CI run away, the harness is byte-only, the cost is minutes, and the fallback layer (E14, Δ = +0.0000% exact, CV = 0) makes the downside a null result rather than a regression.
- **Not PROMOTE-TO-REMOTE.** Nothing in track 16 has a novelty separator (§4), nothing has an unopened held-out family (E18), the only shipping routing path in ANVIL is per-file (`src/anvil.cpp:4400-4428`), and per-region granularity is math-closed for backends (E13) and probably closed for leaves (KILL-WIRE). Promoting to a remote pilot would spend planner budget that the already-promoted G5 pilot (E19) is consuming, on a mechanism whose central estimability claim is false.

**What would move track 16 to PROMOTE-TO-REMOTE:** `SUPPORT` (§7.1) with `KILL-WIRE` not firing, **plus** a Track 20 novelty ruling that the marginal-displacement-on-the-remainder claim is separable from Brotli per-meta-block selection, zstd per-block coder trials, and BLOT, **plus** a newly frozen independent structured family (which does not currently exist and must be created before any promotion).

**What kills track 16 outright:** any one of `KILL-ROUTING`, `KILL-INTERACTION`, `KILL-WIRE`.

**Explicit falsification criteria for my own recommendation.** I am wrong if:
- `SUPPORT` fires with `KILL-WIRE` not firing **and** a Track 20 ruling finds a sharp separator — then CDR's diagnosis was right and only its estimator was wrong, and the correct successor is a bounded per-slot router I have argued against on identifiability grounds alone;
- or if the measured `|shapes|` on D3 is small enough (≤16) that `Θ(S)` identification *is* bounded and production-viable, which would resurrect the `O(1)`-probe claim in a form I have declared impossible. **D3's shape count is unrecorded; EXP-R Arm R1 measures it, and a small value would be a genuine refutation of my §2.1 argument.** I have registered that measurement first for exactly this reason.

---

## 10. Open uncertainties that could still change this verdict

1. **D3's exact shape count is unrecorded.** This single number determines whether §2.1's identifiability objection is fatal (large `S`) or survivable (small `S`). EXP-R R1 measures it.
2. **Is Q2's small pairwise interaction the same phenomenon as S6-3's large trajectory interaction?** E8 vs E15. They are different objects — Q2 held all non-sampled slots at `O11` and enumerated pairs; S6-3 measured parse-trajectory displacement on a different corpus and a different backend path. EXP-R R2/R3 discriminate. **This is the single most valuable open question in track 16.**
3. **Does the S6-3 calibration transfer to carrier granularity?** `[HARN-LX]` → `[GHA-24]`, single parse family, no container router, never transferred, and Gate A would depend on it. EXP-R does not depend on it — that is part of why EXP-R is the right first experiment.
4. **The `1 GB/s / 3.0 GHz` decode inputs in the constructive lane's §5.3 have no citation-grade source.** If real G3-carrier decode is materially slower, the DECODE-SHORT objection in §3.5 strengthens; if faster, CDR's region count has more room. Unmeasured in every window.
5. **The G5 planner pivot is unfrozen** (E19, ledger PART XV pending handoff). If it is dispatched and passes, the planner budget question changes and CDR-vs-G5 attribution contention in §2.6 becomes live.

---

## 11. Independent verification of the Q2 interaction numbers (0.277632% / 0.057144%)

The coordinator directed me to verify these from primary source rather than from either lane's prose. I read the probe implementation at `tools/grotli_g4_planner.cpp:1700-1900` and the ruling logic at `.github/workflows/anvil-i10-grotli-g4.yml:504-540`.

### 11.1 What the code actually computes

```cpp
// tools/grotli_g4_planner.cpp:1754-1755
const size_t n = std::min<size_t>(4, eligible.size());
std::vector<ProbeColumn> sampled(eligible.begin(), eligible.begin() + n);
```

```cpp
// tools/grotli_g4_planner.cpp:1799-1814
for (const ProdCandidate* li : ci.ordered) {
  for (const ProdCandidate* lj : cj.ordered) {
    LeafSelection sel = o11_mixed;                 // non-sampled stay at O11 MIXED
    sel[sampled[pi].sid][sampled[pi].slot] = li->id;
    sel[sampled[pj].sid][sampled[pj].slot] = lj->id;
    const Bytes carrier = build_carrier_only(src, carrier_a, sel);
    const Bytes br = brotli_encode(carrier);       // exactly one q11 call
    ...
    if (comp < pr.c_pair) { pr.c_pair = comp; ... }
  }
}
```

```cpp
// tools/grotli_g4_planner.cpp:1822-1833
double max_pair_gain = 0.0;
double sum_indep = 0.0, sum_pair = 0.0;
for (const PairResult& pr : pairs) {
  const double gain = c_indep ? (double(c_indep) - double(pr.c_pair)) / double(c_indep) * 100.0 : 0.0;
  if (gain > max_pair_gain) max_pair_gain = gain;
  sum_indep += double(c_indep); sum_pair += double(pr.c_pair);
}
const double agg_pair_gain = sum_indep > 0.0 ? (sum_indep - sum_pair) / sum_indep * 100.0 : 0.0;
const bool nonseparable = (max_pair_gain > 0.50) || (agg_pair_gain > 0.25);
```

**The transcription and the arithmetic are correct.** `docs/I10-GROTLI-G4-RESULTS.md:101-102` reports 0.277632% / 0.057144% and thresholds 0.50% / 0.25%; the workflow thresholds at `anvil-i10-grotli-g4.yml:504-505` are `MAX_PAIR_GAIN = 0.50`, `AGG_PAIR_GAIN = 0.25`, matching prereg r5 §10.6. `SEPARABLE-ENOUGH` follows mechanically. **I confirm the numbers and the verdict.**

### 11.2 Four structural properties that neither lane reports — and that change the interpretation

**(P1) `pair_gain_pct` is one-sided. It has a floor of exactly zero by construction.**
`c_pair` is the minimum over the full Cartesian product of eligible leaves, and `sel` starts as `o11_mixed`, so the O11 assignment is always in the enumeration. Therefore `c_pair ≤ c_indep` always, and `pair_gain_pct ≥ 0` always. The metric is a **bounded headroom** ("how much can 2 slots jointly improve on O11"), **not** a signed interaction. It cannot observe negative interaction at all.

**(P2) `pair_gain_pct` confounds three distinct quantities and cannot separate them.**
`C_pair − C_indep` for a 2-slot joint re-optimization mixes:
  (a) genuine **cross-slot interaction** (the only thing CDR prices);
  (b) **univariate carrier-level headroom** — O11 picks each slot by *isolated* q11, so re-optimizing one slot against the whole carrier can gain even with zero interaction;
  (c) **rounding/quantization** of ~1.24 MB payloads — `complete_bytes` is integer, so gains of tens of bytes register as fractions of a percent.

Only (a) is CDR's target. The measurement returns (a)+(b)+(c). **0.277632% is therefore an upper bound on (a), not an estimate of it, and it is not evidence that (a) is the dominant term.** The constructive lane's §12.1 reads it as a direct measurement of (a).

**(P3) The probe touches ≤4 columns of `N` per file.**
`n = min(4, eligible.size())`; pairs = `C(4,2) = 6`; across four files that is exactly the reported 24 pairs — so **every file had `eligible ≥ 4`**. D2 P-DICT alone had 339 slots and D4 1,246 (E: `I10-GROTLI-G2-RESULTS.md` §5, §6). So the probe samples ≤4 of ≥339 (**≤1.2%**) on D2 and ≤4 of 1,246 (**≤0.32%**) on D4. **Q2 never measured interactions across the bulk of the slot population.** Sampling is outcome-blind (SHA-256 order key, prereg §10.2) which protects against cherry-picking, but it does not make a ≤1.2% sample representative of a many-body interaction sum.

**(P4) The aggregate denominator is inflated 6×.**
`c_indep` is computed once per file (`:1759-1762`) and **reused for every pair** (`:1828` adds it once per pair). With 6 pairs per file the denominator is `6 × Σ_f C_indep(f)`, not `Σ_f C_indep(f)`. The 0.057144% figure is therefore *not* an aggregate file-level improvement; it is a mean over 6 duplicated references. Track 02's critic report derived this independently (`02-finalist-planner-fledge.md:207`) and I confirm it. This does not invalidate the number — it is the prereg's frozen formula (§10.6) — but it means "aggregate pair gain" must never be quoted as an aggregate corpus improvement.

### 11.3 What the numbers do and do not license

| claim | licensed? | why |
|---|---|---|
| `SEPARABLE-ENOUGH` under the frozen gate | **YES** | arithmetic confirmed |
| max 2-slot joint improvement over O11 ≤ 0.277632% on the sampled columns | **YES** | that is what is computed |
| "carrier interaction is tiny, so the 6.6056% regret is local error" | **NO** | P1 (one-sided), P2 (confounded with univariate headroom + quantization), P3 (≤1.2% sample). This is the constructive lane's §12.1 and it is an over-read. |
| "interaction cannot explain a 6.6% worst-file regret" | **NOT ESTABLISHED** | the probe bounds 2-body interaction on ≤1.2% of slots. A many-body or long-range interaction that no 2-slot probe on 4 columns could see is **not excluded**. |
| per-file fidelity gate 0.50% is unattainable in principle | **NOT ESTABLISHED** | Track 02's critic asserts this from the same number; P1–P3 defeat it. The gate is hard but not proven impossible. |

**Correction to my own §1.1 and §2.5.** I initially wrote E8 as "Q2 measured the carrier interaction as tiny," following the results document's framing. Having read the source, that is too strong. The correct statement is: **Q2 measured that joint re-optimization of ≤2 of ≤4 sampled columns improves complete-carrier bytes by ≤0.277632% (max) / ≤0.057144% (mean over a 6×-duplicated denominator), one-sided, with interaction confounded against univariate headroom and integer quantization.** That is a *much weaker* constraint than "interaction is tiny," and it materially weakens — without reversing — my §2.1 and §7's `KILL-INTERACTION` clause. **I am downgrading my own confidence in `KILL-INTERACTION` from "expected" to "genuinely uncertain," and I say so rather than quietly keeping the stronger claim.**

**The honest position after verification:** the constructive lane's strongest self-kill argument (§12.1) is **weaker than it believes**, and so is my own symmetric use of it. The Q2 evidence no longer decides the fork. That is exactly why EXP-R Arm R2 (single-slot revert) and Arm R3 (single-slot substitution) must run **before** any basis is chosen — they measure the decomposed quantities the Q2 probe confounds, and they do so on the full slot population rather than 4 columns.

---

## 12. The rank-1 displacement assumption: a proof, not an objection

Space Bunny's §4.2–§4.3 claim is that `D(Π,λ) ≈ Σ_i b[shape(r_i), bucket(i)]·Δ(λ_i, default)` and that the table `b` is recoverable from `|L| = 5` whole-carrier probes.

### 12.1 Probe-insufficiency theorem

**Setup.** Each whole-carrier probe returns exactly one scalar: the complete compressed byte count. Let `b ∈ ℝ^S` with `S = |shapes| × |buckets|` be the displacement table, and let `Δ_ℓ` be the scalar sensitivity of leaf `ℓ` (`ℓ ∈ {1..|L|}`, `|L| = 5`).

**Construction.** Probe `ℓ` replaces the default leaf with leaf `ℓ` at *all* positions where `ℓ` is eligible, leaving all other positions at default. The measured response is

```
y_ℓ = Σ_{k=1}^{S} b_k · Δ_ℓ · 1[probe ℓ touches bucket k]  +  ε_ℓ
```

Each `y_ℓ` is a **scalar linear functional of the S-vector `b`**. With `|L| = 5` probes the observation matrix has rank **≤ 5**. Standard result: `b` is identifiable only on the 5-dimensional row space of the observation matrix; the remaining `S − 5` dimensions are **unidentified** — every value of `b` along those directions produces byte-identical probe results.

**Corollary.** Since EXP-1 §9 step 3 evaluates `S ∈ {16, 64, 256, 1024}`, CDR-ARM-1 under-determines its own model by **at least 11×** (at S=16: 5 of 16) and up to **205×** (at S=1024). No differencing, bootstrapping, or regularization recovers an unidentified direction; the data contains no information about it.

**This holds whether or not `b` is separable.** If `b = u[shape] ⊗ v[bucket]` with `|u| = |shapes|`, `|v| = |buckets|`, the parameter count is `|shapes| + |buckets|` — at the frozen 64×16 that is **80 parameters against 5 measurements**, still 16× short. So the dichotomy in §2.1 is complete:

> **Every version of CDR requires more free parameters than its 5 probes can identify.** The resolution is therefore forced to be *a globally fitted table* — i.e. the object G5B-ORDINAL measured at **+1.6507% worse than exact** (E12). There is no third branch in which 5 probes suffice.

### 12.2 The rank-1 label itself

Space Bunny calls a `|shapes| × |buckets|` table "rank-1" (defect D-4, §1.3). Note the actual prior-art collision this creates: a genuinely rank-1 (separable) `b` in a **shape × column-ordinal** basis is a statement that *position in ordinal space is predictable from a per-shape profile*. That is the same hypothesis G5B-ORDINAL tested when it tried to place shapes by ordinal cheaply and lost by **+1.6507% on 1/4 files**. CDR's ARM-0 is a re-parameterization of the loser. Its ARM-1 is an unidentifiable version of the same model.

### 12.3 The confound CDR cannot see even with perfect estimation

G5B measured that exact ordinal placement (B1, 1,380,245 B) beats cheap ordinal placement (B2, 1,403,029 B). CDR prices *displacement given* a fixed layout. If the layout itself is wrong, every `b[shape,bucket]` entry is conditioned on a layout that will not be used. Neither `Δ(·)` nor `b` is invariant to the layout choice, and CDR's cost model has no term for changing it — because layout selection is deliberately excluded as a "later, separately preregistered" question. **So CDR prices the marginal effect of a decision inside a layout whose selection is the larger, already-measured win (E9: 89,960 B, `COLUMN-DOMINANT`, 4/4) and is silently held fixed.** §3.1 then charges CDR 29–371% of that very win in region metadata. The economic incoherence is direct.

---

## 13. CDR's ≤5 probes versus Track 02's 80-q4 finalist ladder

The coordinator asked for a direct information-gain comparison. Both budgets are frozen in one place each: CDR's `K_probe ∈ {0,5}` (constructive §5.5) and Track 02's ≤80 isolated q4 + ladder (E19, `I10-G5-PLANNER-PIVOT-PREREG.md` §14.1).

### 13.1 Raw budget comparison

| dimension | **CDR ARM-1** | **Track 02 P1** |
|---|---|---|
| measurements in the decision | **5** | **80** (isolated q4, ≤8,192 B each) + ≤16 whole-carrier ladder |
| free parameters to identify | `S ∈ [16, 1024]` (≥80 even if separable) | **8** ridge features |
| **identifiable?** | **NO — 5 < 16 at best** | **YES — 80 ≫ 8, over-determined ~10×** |
| bytes moved through the codec | `5 × 1,240,155 ≈ 6.20 MB` at **q11** | `80 × 8,192 = 655,360 B` at **q4** + ladder |
| **bytes per measurement** | **1,240,155 B** | **8,192 B** |
| **information efficiency** | — | **151.4× cheaper per measurement** |
| uses q11 *inside* the routing decision | **YES** (5 q11 encodes) | **NO** — `q11_rank_calls(P1) = 0` is a frozen gate |

**Byte-per-measurement: CDR 1,240,155 B vs Track 02 8,192 B → 151.4×.** And Track 02 buys 16× more measurements against a parameter count 2–128× smaller. On every axis of an information/measurement exchange, CDR is the worse instrument.

### 13.2 Wall-time consequence

Using the only q11 throughput figure in the repo — `brotli-q11-lw30` on dickens, 10,192,446 B in 16.56 s ⇒ **0.615 MB/s** (`tests/auto-routing.v2.csv`, `[LOCAL-I9]`, single-run; ledger A7 explicitly downgrades this CSV's timing columns to non-citation-grade):

- CDR ARM-1 on D3: `5 × 1.24 MB / 0.615 MB/s ≈ 10.1 s`.
- Track 02's sampled q4: `655,360 B` of q4 input. q4 is a materially cheaper tier; even at a conservative 3× q11 it is `0.655 MB / 1.85 MB/s ≈ 0.35 s`.

So CDR spends **~29× more wall time to obtain 1/16 the measurements, against a model it cannot identify.** Its only advantage over Track 02 is *exactness of the measurement point* (whole-carrier q11 vs isolated q4) — but §2.5 already showed CDR-ARM-1's symmetric speedup lands at **~3.2× against a 5.0× gate** using the lane's own O11 figures, and Track 02 does not need q11 at all.

### 13.3 The decisive asymmetry

Track 02's 80 probes buy **calibration of a decision it is entitled to make**: choose among leaves *within* a carrier whose layout is fixed. CDR's 5 probes are asked to buy **identification of a global position-response surface** — a strictly harder target — and are structurally incapable of it (§12.1).

> **CDR is not a cheaper way to do Track 02's job. It is a more expensive way to do a harder job, and it fails at the harder one.** If per-slot routing work is funded at all, Track 02's budget shape (many cheap samples, few parameters, no q11 in the decision) strictly dominates CDR's. CDR must justify itself on the *segmentation* axis alone — and §3.1 shows that axis costs 29–371% of the measured causal gain.

### 13.4 A finding that cuts against both lanes

Track 02's critic (`02-finalist-planner-fledge.md`) computes the **prize ceiling for any per-slot routing mechanism at 0.343% of the carrier (≈4,748 B on the 1,384,654 B D1-D4 routed total)**, and notes the already-authorized G5A representation prize in the same carrier is **89,960 B (6.1189%) — 19× larger**. I independently confirm the ratio from E9 and E2: `89,960 / 1,384,654 = 6.4955%` and `82,115 / 1,384,654 = 5.9313%`; against the critic's 6.1189%/0.343% pair the order of magnitude — **~19×** — holds.

**I report this as corroboration, not as an attack on Track 02**, and I add my own caveat to it: the 0.343% ceiling is itself derived by differencing measured quantities whose provenance I have not re-verified line-by-line, and per §11 it may share the Q2 confound (if the ceiling was inferred from pair gains, it inherits P1–P3). **The one thing all three lanes can assert without inheritance is E9: the *ordering* prize in this carrier is 89,960 B, measured, 4/4 files, `COLUMN-DOMINANT`, and it is 19× any number currently attached to per-slot leaf routing.** That is the allocation fact the coordinator should act on, and it is orthogonal to whether CDR, Track 02, or neither survives.

---

## 14. Post-reconciliation ruling (supersedes §9's supporting detail; headline unchanged)

### 14.1 Net effect of the reconciliation on my position

| my earlier claim | after verification | change |
|---|---|---|
| §1.1 E8 "Q2 measured the carrier interaction as tiny" | corrected in §11.2 — one-sided, confounded, ≤1.2% sample | **weakened by me** |
| §2.5 Gate D likely fails ARM-1 (~3.2× vs 5.0×) | unchanged; strengthened by §13.2 (~29× more wall time than Track 02 for 1/16 the measurements) | unchanged |
| §2.1 estimability is incoherent | **upgraded from objection to proof** (§12.1): 5 measurements vs ≥16 parameters, both branches of the dichotomy under-determined | **strengthened** |
| §3.1 `W_CDR` = 29–371% of the causal gain | unchanged; reinforced by §12.3 (the layout whose 89,960 B prize it prices is held fixed) | unchanged |
| §2.2 Gate A is not discriminating (2.0% > S2's measured 1.5388%) | unchanged — this is arithmetic, not interpretation | unchanged |
| §7 `KILL-INTERACTION` expected to fire | **downgraded to genuinely uncertain** after §11.2 | **weakened by me** |

### 14.2 Corrected strongest falsification case against CDR

The constructive lane's §12.1 ("Q2 says interaction is tiny ⇒ CDR is the wrong cure") **does not survive source verification** (§11.2). My strongest surviving case is therefore not the Q2 fork. It is:

> **CDR-ARM-1 is mathematically incapable of identifying the model it prices** — 5 scalar observations against ≥16 free parameters, under-determined in every branch of the rank-1/full-table dichotomy (§12.1) — and the only resolution (a globally fitted position table) **is the object G5B-ORDINAL measured at +1.6507% worse than exact placement**. Independently, its arm structure cannot be expressed in the frozen G3 carrier without either being redundant or breaking the 16/16 attribution identity (§2.4), its metadata charge consumes 29–371% of the measured causal gain on a denominator it chose wrongly (§3.1), its primary gate passes a surface G4 already closed (§2.2), its Gate D is likely to fail on the lane's own arithmetic (§2.5), it is 151.4× less information-efficient per measurement than Track 02's 80-q4 budget (§13.1), and it prices the marginal effect of a decision inside a layout it holds fixed while excluding selection of that layout (§12.3).

That is six independent defects, four of them arithmetic or structural and unaffected by the Q2 re-verification.

### 14.3 Corrected strongest surviving case for anything in track 16

Unchanged in substance, but now stated without leaning on Q2:

1. **The one unmeasured quantity that gates the whole lane is routing headroom.** G4 measured fidelity to O11; nobody measured whether any per-slot choice beats O11 at carrier level. That is a ≤132-encode question (§7).
2. **Interaction has not been falsified.** §11.2 shows the Q2 probe cannot separate interaction from univariate headroom, covers ≤1.2% of slots, and is one-sided. The fork the coordinator named is **still open** — but it is open *because the measurement was badly designed*, not because the evidence favours either side. EXP-R R2/R3 are the correctly designed version.
3. **Ordering, not routing, is where the 19× prize is** (E9, §13.4) — measured, 4/4 files, already authorized. Track 02's own critic reaches this conclusion independently. Two adversarial lanes converging on it is the strongest signal in this swarm.

### 14.4 Final recommendation (unchanged headline, sharpened basis)

> # **PILOT** — of EXP-R (headroom / interaction-decomposition / region-census probe) plus EXP-C (zero-new-wire cardinality gating). **KILL the constructive CDR formulation as specified.**

Changed from §9:

- **EXP-R Arm R2 is now the highest-value single measurement in track 16**, not Arm R3. §11.2 shows the existing interaction evidence is unusable; a correctly-specified single-slot revert on the full slot population is what the lane has never had.
- **EXP-R must also report the Q2 probe's own confound components** — per sampled slot, the univariate carrier-level gain of flipping that slot alone *against the whole carrier* — so the decomposition (a) interaction / (b) univariate headroom / (c) quantization is measured rather than assumed. This is one extra column in Arm R2 and costs nothing.
- **§7's `KILL-INTERACTION` clause is retained but re-labelled `NO-GO-EXP-R-INTERACTION-UNFOUND`**, and its stated basis changes from "Q2 suggests interactions are tiny" to "no interaction was found by a correctly-specified probe." That is a weaker and more honest kill, and it is the one I am willing to have fire.
- **Track 02's budget shape is endorsed over CDR's** on information-efficiency grounds alone (§13.3). This is not a Track 02 endorsement on its merits — its per-file gate, prize ceiling, and feature-blindness objections stand in `02-finalist-planner-fledge.md` and I do not disturb them. It is only a statement that **if** per-slot routing is funded, CDR's probe budget is the wrong instrument.

**Unchanged falsification criteria for my own recommendation** (§9): `SUPPORT` with `KILL-WIRE` not firing plus a Track 20 novelty separator plus a newly frozen independent structured family would move track 16 to PROMOTE-TO-REMOTE. Any one of `KILL-ROUTING`, `KILL-INTERACTION`, `KILL-WIRE` ends the direction. And the refutation I am most exposed to remains **D3's unrecorded `|shapes|` count** — if it is small (≤16), §12.1's under-determination shrinks toward the separable case, the `O(1)`-probe claim partially revives, and my objection weakens. EXP-R Arm R1 measures it first, deliberately.
