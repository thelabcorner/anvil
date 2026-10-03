# Track 02 — G5A-Informed Bounded Finalist Planner: Independent Adversarial Audit

**Agent:** Fledge Alpha Free (independent adversarial reviewer / validator role)
**Date:** 2026-10-02
**Status:** INTERIM — issued before `02-finalist-planner-space-bunny.md` exists. Reconciliation section (§11) is reserved and will be filled when the constructive lane lands.
**Track:** 02-finalist-planner
**Audited artifact:** `docs/I10-G5-PLANNER-PIVOT-PREREG.md` (frozen r1, 1534 lines) as the only formal specification of the "bounded finalist planner"; its parent evidence chain G3 → G4 → G5A → G5B.
**Evidence discipline:** every claim below is tagged `[M]` measured (with source artifact), `[A]` assumption/unverified, or `[P]` projection (arithmetic shown). Arithmetic in §8 is reproducible via the isolated check in `prototypes/swarm-2026-10-02/02-finalist-planner/fledge/prize_gate_audit.py`.

---

## 0. Verdict up front

**HOLD** — do not dispatch the pivot as written. Convert to a cheap, decisive, remote-only pre-flight (R0, §7) before any pilot source is published.

The single dominant finding is not a subtle defect. It is a **prize/gate calibration inversion**:

> The pivot's fidelity gates are sized against a prize that measurement shows is **0.343 % of the aggregate carrier (≈ 4,748 B on the 1,384,654 B D1–D4 routed total)**, while the **already-measured, already-authorized representation prize in the same carrier is 89,960 B (6.1189 %)** — **19× larger**. The aggregate fidelity gate (0.25 %) is **72.9 % of the entire prize ceiling**, and the per-file gate (0.50 %) is **1.80× the largest single-pair gain ever measured on any file**.

A selector that is *uninformative but not anti-correlated* can pass §16.1. A *perfect* selector is not demonstrably able to pass §16.1 per-file. The gates cannot distinguish a good planner from a coin flip, and the speed gate compares against a baseline its own preregistration calls "not a viable production planner".

---

## 1. Fact base — what actually exists

### 1.1 There is **zero** measured evidence for this track

`[M]` `I10-G5-PLANNER-PIVOT-PREREG.md:13` — *"Workflow status: not created; blocked on a conforming pilot source and immutable implementation identity."*
`[M]` `RESEARCH_LEDGER.md:4840-4841` — *"The planner pivot likewise reports missing links and has no frozen workflow."*
`[M]` `RESEARCH_LEDGER.md:4953` — *"Named publication blocker. No commit or push was performed in this cycle."*

**Consequence for the swarm:** any track-02 artifact — mine, the constructive lane's, or the coordinator's synthesis — that presents P1 regret, `speedup_plan`, `speedup_total_encode`, or ladder-recovery values as *results* is presenting projections as measurements. There is no pilot source, no frozen implementation identity, no run. `[M]`

### 1.2 Measured parent evidence (the only numbers that exist)

| Fact | Value | Source |
|---|---|---|
| G3 D1–D4 routed portfolio | 1,384,654 B vs 1,466,769 B raw Brotli = **−5.5984 %** | `I10-FRONTIER-RECON-2026-09-24.md:180` `[M]` |
| G3 V1 held-out | 2,179,615 B vs 2,112,235 B raw = **+3.1900 % worse** | same, line 181 `[M]` |
| G4 decision | **NO-GO-G4**; Q2 **SEPARABLE-ENOUGH** | `I10-GROTLI-G4-RESULTS.md:10,32,103` `[M]` |
| G4 proxy worst-file regret | S0 7.0802 %, S1 7.0802 %, S2 1.5388 % | same, line 70 `[M]` |
| G4 proxy aggregate regret | S0 −2.0353 %, S1 −1.1453 %, S2 −0.2120 % | same, line 69 `[M]` |
| G4 Q2 max pair gain | **0.277632 %** (threshold 0.50 %) | same, line 101 `[M]` |
| G4 Q2 aggregate pair gain | **0.057144 %** (threshold 0.25 %) | same, line 102 `[M]` |
| G4 speed ratios | **WITHDRAWN** as `INVALID_SPEED_ACCOUNTING` | `I10-FRONTIER-RECON-2026-09-24.md:210-215` `[M]` |
| G4 speed failure mechanism | O11 denominator read **cached** ranking labels; actual q11 label build timed separately (D1: 667.948690 ms vs 0.004758 ms) | same, lines 211-214 `[M]` |
| G5A A0/A1/A2/A3 | 2,054,532 / 1,470,205 / 1,452,383 / **1,380,245** B | `RESEARCH_LEDGER.md:4756` `[M]` |
| G5A ordering classification | `ORDER-MATERIAL` / **`COLUMN-DOMINANT`**; A3 saves **89,960 B** vs A1, smaller on 4/4 | `I10-GROTLI-G4-RESULTS.md` framing + `RESEARCH_LEDGER.md:4757` `[M]` |
| G5A ordering saving concentration | D3 63,483 B, D4 22,823 B | `I10-FRONTIER-RECON-2026-09-24.md:188-189` `[M]` |
| G5A V1 | A1 2,182,265 B, A3 2,183,986 B → **adverse**; A1 already 70,030 B worse than raw | `I10-G5A-LOCALITY-CONTROLS-PREREG.md:25`; recon line 185 `[M]` |
| G5A A0 null | one random draw, 674,287 B above A3 — **a null point, not a distribution** | recon line 194 `[M]` |
| G5B corrected | B2 **+1.6507 %** vs B1, wins 1/4 → `ORDINAL-ADVERSE` | `RESEARCH_LEDGER.md:4786-4788` `[M]` |
| Class A encode rates | Brotli q1 **432.106 MB/s**, q11 **0.681 MB/s** (harmonic, `tools/pareto_front.py`) | `RESEARCH_LEDGER.md:4862` `[M]` |
| Corpus sizes | D1 277,673 · D2 615,350 · D3 10,485,760 · D4 3,585,053 B | `I10-G5-PLANNER-PIVOT-PREREG.md:177-180` `[M]` |

### 1.3 What the pivot contract actually specifies (audited)

`[M]` `I10-G5-PLANNER-PIVOT-PREREG.md`:
- §1.1 (l.53-66): analytical features from exact candidate objects; fixed 8-feature ridge fit against **frozen O11 labels** on D1–D4; ≤80 q4 evaluations; bounded whole-carrier finalist ladder.
- §5.1 (l.301-311): the label is `y = log2(max(1, o11_isolated_q11_bytes))` — **Brotli q11 on the isolated object `o(c)`**, explicitly *"not a complete-carrier byte value."*
- §4.7 (l.280-291): features are `x0=log2(N), x1=log2(m), x2=D/m, x3=log2(P)−log2(R), x4=H0/N, x5=H1/H0, x6=match_coverage, x7=coded_bits_per_occurrence` — **all eight are intrinsic to `o(c)`; none references position, neighbourhood, column, or carrier context.**
- §7.2 (l.416-421): sample input is `o(c)` capped at **first 8192 bytes**.
- §10 (l.606-683): ladder q1(≤8) → q4(≤4) → q6(≤2) → q11(≤2); then raw q11 + G2 q11.
- §13.2 (l.794-821): `speedup_plan = T_plan(O11 ranking+selection) / T_plan(P1)`, where *"O11 ranking time includes isolated q11 scoring and selection."*
- §14.1 (l.885-903): production-shaped maximum = 80 q4 + 8 q1 + 4 q4 + 2 q6 + 2 q11 + 1 raw q11 + 1 G2 q11, and `q11_rank_calls(P1) = 0`.
- §16.1-16.5 (l.976-1038): gates `P1_agg ≤ 0.0025`, `max_f ≤ 0.0050`, ladder `≤ 0.0010 / 0.0025`, `speedup ≥ 5.0` (10.0 target), `peak_rss(P1) ≤ peak_rss(O11)`, ≤ 65,536 B compiled encoder model, **decoder model delta exactly zero**.
- §3 (l.170-190): D1–D4 only; V1 excluded as known stress; *"No other corpus may be measured"*; *"D1-D4 cannot support a generalization claim."*

**What is genuinely well built (credit where due):** §18 leakage/tuning rules; §5.4 LOFO with the all-four-file model explicitly barred from D1–D4; §7.4 outcome-blind SHA-256 order key; §9 exact-byte dedup with reused-result counting; §12's bar on using `C_prod`/raw/G2 savings to satisfy fidelity gates; §17's decision matrix that **separates `NO-GO-PILOT-LADDER` from `NO-GO-PILOT-FIDELITY`**; §16.5's zero decoder-delta requirement. This is a better-specified pilot than G4. My objections below are to *calibration and target alignment*, not to discipline.

---

## 2. Oracle-leakage audit

### 2.1 What is clean

`[M]` LOFO (§5.4) prevents file-level target leakage. §7.4's `order_key` is provably outcome-blind (SHA-256 over file/shape/slot/leaf identity only, and §7.4 explicitly bars scores, O11 labels, and byte results). §7.7's priority uses only P0 predictions and q4 residuals.

### 2.2 LEAK-1 — design-time feature selection on the very files LOFO claims to protect

`[M]` The 8 features were chosen by an author who had already read G4's D1–D4 outcomes (S0 −2.0353 % / S1 −1.1453 % / S2 −0.2120 % aggregate, 7.0802 % / 1.5388 % worst-file — `I10-GROTLI-G4-RESULTS.md:69-70`). §18.2 (l.1131-1145) freezes features against *post-hoc* change but says nothing about *post-hoc selection*: the feature set was selected on D1–D4 and LOFO does not undo selection bias.

**Consequence:** the LOFO estimate of P1 fidelity is optimistically biased by an unquantified amount. Thresholds (`[M]` §16.1) *are* clean — they descend from the G4 prereg frozen before G4's D1–D4 — so the bias is confined to the feature basis and to any constant informed by G4's numbers (`[A]` §7.8's shrinkage prior of 8, §7.7's UCB multiplier 1.64, and §7.3's four size bins have no recorded provenance in the doc; `[A]`).

**Required:** disclose feature-set provenance, or add a nested-selection control (feature-set chosen on D1–D2 only, evaluated on D3–D4).

### 2.3 LEAK-2 — the label is not the objective (target misalignment, inherited unfixed from G4)

`[M]` The label is Brotli q11 on `o(c)` **in isolation** (§5.1). The decision objective is **whole-carrier complete bytes** (§12). These differ by construction:

- an isolated object carries its own Brotli meta-block headers and its own Huffman tree construction, sized for ~256 candidates' worth of statistics;
- in the carrier, every slot shares one stream, one context history, and one set of model updates.

`[A]` This is the most plausible mechanical explanation of G4's 1.5388 % worst-file failure: a proxy can be near-perfect on the isolated label and still mis-rank at the carrier level because the isolated metric contains a constant-per-object overhead that carries no information about carrier-level interaction. `[M]` G4's own Q2 result is the counter-evidence to this story — Q2 found the *carrier-level* pair interactions small (0.277632 % max). Both can be true: interactions are small **and** the isolated target is misaligned. Neither is repaired by the pivot, which keeps §5.1's label verbatim.

`[A]` **This is the pivot's deepest structural weakness.** The pivot changes the *function class* (ridge + q4 correction) but not the *target definition*. G4's measured failure was fidelity; G4 never established that fidelity failure was a function-class failure — its Q2 probe explicitly argued against the "needs joint combinatorics" story and its speed column was withdrawn. So the pivot's central bet — *a better surrogate for the same misaligned target fixes it* — is **unevidenced**.

### 2.4 LEAK-3 — the 8,192-byte sample cap biases exactly the strata where bytes live

`[M]` §7.6 computes `residual = log2(q4_sample_input_bytes / exp2(p0_predicted_object_bytes))`. For any candidate with `len(o(c)) > 8192`, the numerator is a **truncated** measurement while the denominator is a **full-object** prediction, and the resulting `stratum_correction` (§7.8) is applied to full-object scores.

`[A]` Strata S2 (4,097–65,536 B) and S3 (>65,536 B) therefore receive a **systematically biased** correction, biased toward whichever leaf compresses well on its own *prefix*. Those are precisely the strata carrying the largest per-candidate byte counts. There is no bias-correction term, no prefix-consistent feature variant, and no diagnostic for it in §5.5.

**Required:** either compute §4.3-§4.7 features on the same 8,192-byte prefix used for the q4 call (making the residual coherent), or add a frozen, preregistered size-bias term, or restrict sampling to S0/S1. As written, P1's advantage over P0 in large strata is an artifact risk, not a measurement.

---

## 3. Cached-label and speed-accounting audit

### 3.1 The G4 failure mode is guarded only in prose

`[M]` G4's speed column was withdrawn because the O11 denominator read cached ranking labels while actual q11 label construction was timed separately — D1 **667.948690 ms vs 0.004758 ms**, a 140,000× discrepancy (`I10-FRONTIER-RECON-2026-09-24.md:211-214`).

`[M]` The pivot's §13.2 (l.820) states O11 ranking time "includes isolated q11 scoring and selection" — correct intent. But:

- there is **no machine gate** requiring the O11 label build to be re-timed in the same job;
- §11.3 (l.717-720) forbids *substituting for same-job timing* only when importing G4 S2 — a textual guard on a different artifact;
- §14.3's counter list (l.921-944) contains `o11_q11_candidate_calls` but **no `o11_label_build_ms`** and no assertion that it equals `candidates_enumerated`.

`[A]` A pilot that reads a precomputed O11 label table (e.g. rebuilt from the G4 artifact, which §11.1 explicitly contemplates: *"must match any reused G4 carrier identity records where available"*) would satisfy every listed gate and produce a fictitious `speedup_plan`. **This is the single most likely way the pivot produces a false positive.**

### 3.2 No anti-cheat counter on "q11_rank_calls(P1) = 0"

`[M]` §14.1 asserts `q11_rank_calls(P1) = 0` and §16.6 makes any q11 candidate-ranking call "an unconditional failure" — but the *only* counters are `p1_whole_q11_calls`, `final_raw_q11_calls`, `final_g2_q11_calls`. A stray q11 inside the feature/scoring path, counted as a ladder call, satisfies every counter.

**Required:** (a) a dedicated `p1_q11_calls_total` that must equal `p1_whole_q11_calls + final_raw_q11_calls + final_g2_q11_calls` exactly; (b) a single unconditional Brotli-call wrapper incrementing a process-global counter, asserted against a per-file expected total; (c) `o11_q11_candidate_calls == candidates_enumerated` and a same-job `o11_label_build_ms`. Machine gates, not prose.

### 3.3 The speed baseline is invalid *by the prereg's own argument*

`[M]` G4 §1 (l.55-67): O11 *"is a **research oracle**. It produces faithful labels for candidate ranking, but it is not a viable production planner."* `[M]` Pivot §11.1 (l.689-697): *"O11 remains a reference, not an optimum."*

`[P]` Therefore `speedup_plan = T_plan(O11)/T_plan(P1)` compares a cheap planner against a **baseline the project has already declared unusable**. The ratio is unbounded above by construction: make O11's oracle work arbitrarily expensive and the gate passes without the planner improving anything. `[A]` The honest production baselines are the *cheapest correct alternatives*, both of which pay **zero candidate-ranking calls**: (i) a fixed per-leaf heuristic (no per-slot ranking), (ii) raw Brotli q11 on the source. Against those, the pivot's speedup is **not a ratio at all** — it is an absolute overhead, and the correct gate is an overhead budget.

---

## 4. Hidden-cost audit and a stricter selector-cost accounting

### 4.1 The unbounded quantity the prereg never bounds or counts

`[M]` §4.3/§4.4/§4.5 each cost Θ(N) per candidate: 256-bin histogram, 257×256 order-1 contingency, and *"hash every four-byte sequence … inspect at most the eight most recent prior positions … `O(8N)`"* with a *"single 32-bit hash table"* **whose size is unspecified** (§4.5, l.263).

`[M]` The feature volume is therefore `F = Σ_c len(o(c))`, and **`F` is bounded nowhere, counted nowhere** (§14.3 has no `sum_object_bytes`), and gated nowhere. It is almost certainly **≫ N_src**, because `o(c) = leaf_id ‖ occurrence_count ‖ payload_length ‖ payload` (§4.1) and payload grows with occurrence count.

`[A]` If the mean object is 5 KB and D3 has 20,000 eligible candidates, `F ≈ 100 MB ≈ 9.5× N_src`. This is a projection with an unmeasured premise — **which is the point**: the number decides the speed gate and is not instrumented.

### 4.2 Stricter accounting (my replacement for §13.2)

Define per file: `N_src` source bytes; `C` eligible `(slot,leaf)` candidate rows; `F = Σ_c len(o(c))`; `B` carrier bytes; `κ` byte-touches per byte of `o(c)` across the three feature passes (`[A]` κ ≈ 10–30); `ω` effective feature-pass byte rate (`[A]` ≈ 1–3 GB/s for scattered histogram + hash-chain work); `T_q` Brotli encode rate at quality q (`[M]` T₁ = 432.106 MB/s, T₁₁ = 0.681 MB/s from the Class A grid; `[A]` T₄, T₆ between).

```
T_features(P0) = T_features(P1) ≈ κ·F / ω
T_sample_q4(P1) ≤ 80 × 8192 / T4                        = 655,360 / T4
T_ladder(P1)  ≤ 7·B/T1 + 4·B/T4 + 2·B/T6 + 2·B/T11      (7, not 8: §15 gate 4
                                                          forces REGION_RAW identical
                                                          across P0/P1/O11)
T_portfolio_extra(P1) = 1·N_src/T11 (raw) + 1·N_src/T11 (G2)
T_encode(O11) ≈ N_src·C/T11 + 4·B/T11
```

**Three corrections to the prereg's arithmetic that I require:**

1. **The R1 call count is 7, not 8.** `[M]` §15 gate 4: *"`REGION_RAW` is byte-identical across P0, P1, and O11."* P0-RAW and P1-RAW are one carrier. §14.1's "at most eight q1 calls" is an upper bound that is never reachable. Minor, but it means the prereg's table is not tight and no counter validates it.
2. **`T_encode` must exclude nothing and include F.** §13.2's `T_plan` does include `mdl_feature_ms` — correct — but with no `F` counter, no gate, and no pinned hash-table size, a pass is unfalsifiable. `[M]` §4.5 "a single 32-bit hash table" at 2²² entries is 16 MB of RSS that no gate sees.
3. **The O11 denominator must be the same-job re-timed label build**, per §3.1 above.

### 4.3 The headline cost, quantified on D3

`[P]` Using `[M]` N_src = 10,485,760 B, `[M]` T₁₁ = 0.681 MB/s, `[M]` T₁ = 432.106 MB/s, and setting the **unmeasured** T₄/T₆ terms to zero (an optimistic bound):

```
raw Brotli q11 encode, D3            = 10,485,760 / 0.681e6   ≈ 15,395 ms
R1: 7 carriers at q1                 = 7 × 10,485,760 / 432.106e6 ≈    170 ms
R4: 2 finalists at q11               = 2 × 15,395                ≈ 30,790 ms
C_prod raw q11 baseline              ≈                            15,395 ms
C_prod G2 q11 baseline              ≈                            15,395 ms
incremental over "just ship raw q11" ≈ 30,790 + 15,395 = 46,185 ms ≈ 3.0× raw q11
                                                             (+ unmeasured q4/q6 terms)
```

`[P]` **The pilot's production-shaped portfolio costs ≈ 3× a plain raw Brotli q11 encode on a 10 MB file, before any q4/q6 term, before the 80 samples, and before the feature sweep.** Against a prize ceiling of 0.343 % (§5). That is the trade, stated honestly. It is *not* what "speedup_total_encode ≥ 5×" (§16.3) invites a reader to believe.

`[A]` **Cheap fix requiring re-freeze, not patching:** authorize early abandonment — drop any finalist worse than `1.005 ×` the best q1 after R1, and skip the G2 baseline entirely if it lost at q1. That cuts the q11 count from 3 to ~1 and the incremental cost from ~3× to ~1×. It also changes the frozen contract, so it must be a **new revision frozen before dispatch** (MASTER-BRIEF doctrine 5).

### 4.4 Converter: charge the planner as overhead against a tier the user chose

Replace §16.3's ratio gates with:

```
planner_overhead(P1) = T_features + T_sample_q4 + T_ladder + T_portfolio_extra − T_raw_q11
gate: planner_overhead(P1) ≤ 0.50 × T_encode(raw Brotli q11)      # ladder is q11-tier work
gate: planner_overhead(P0) ≤ 0.05 × T_encode(raw Brotli q11)
gate: T_features ≤ 0.10 × T_encode(raw Brotli q11)               # the F-volume gate
plus: absolute ms reported per stage; sum_object_bytes(f) / N_src reported and ≤ 64
```

`[A]` Rationale: q11 is a tier the user *asked for*, so a q11-class finalist ladder is a legitimate cost of that tier; what must be bounded is (a) the feature sweep, which is pure overhead, and (b) the *number* of q11-class encodes, which the pivot inflates from 1 to 3. `[A]` Under this accounting the pivot's worst term is not the ladder — it is `κ·F/ω`, which is unbounded today.

---

## 5. Gate calibration — the central falsification

### 5.1 Arithmetic (reproducible: `prize_gate_audit.py`)

`[M]` Q2's denominator is confirmed to be **carrier complete bytes**: `pair_gain_pct(i,j) = (C_indep_MIXED − C_pair(i,j)) / C_indep_MIXED × 100` (`I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md:1156-1162`), with `max_pairs = C(4,2) = 6 per file`, `max_pairs_total = 24` (l.1106-1107), and `agg_pair_gain` reusing **one** `C_indep_MIXED(f)` across that file's 6 pairs (l.1168-1170).

```
[ M ]  agg_pair_gain = 0.057144 %   over  Σ_pairs C_indep_MIXED  =  6 × Σ_f C_ref(f)
[ P ]  ⇒ total pair prize over 24 sampled (slot,slot) pairs
          = 0.00057144 × 6 × 1,384,654  ≈  4,748 B
[ P ]  ⇒ as a fraction of the aggregate carrier:  0.3429 %
```

This is an **upper bound** on what any per-slot independent selector can win (pair-exact optimization ⊇ per-slot independent optimization), and it is the only evidence in the project about the size of the selector's job.

```
[ P ]  aggregate gate   0.25 %  ÷  0.3429 %  =  0.729   → gate is 72.9 % of the entire prize
[ P ]  per-file gate    0.50 %  ÷  0.277632 % = 1.801  → gate is 1.80× the largest pair gain
                              ever measured on any file
[ P ]  G5A representation prize 89,960 B  ÷  4,748 B   = 18.95×  → 19× larger prize,
                              already measured, already authorized, on the same carrier
```

### 5.2 What this means

1. **§16.1's aggregate gate is nearly vacuous.** A selector whose predictions are independent of the truth but not *anti*-correlated is expected to lose roughly half the prize (2,374 B ≈ 0.17 %) — **under the 0.25 % gate**. `[A]` G4's S0 and S1 posted *negative* aggregate regret (−2.0353 %, −1.1453 %) while missing per-file by 14×. A pass on §16.1's aggregate leg is therefore **not evidence of a working selector**.
2. **§16.1's per-file leg is not attainable in principle.** The gate is 1.80× the largest single-file pair gain ever measured. `[A]` Whether unmeasured slots can supply the remainder is unknown — but the prereg never asserts they can, and §16.1 is written as if a perfect selector is expected to clear it.
3. **The quantization floor may sit above the aggregate gate.** `[P]` Objects in stratum S0 (1–256 B) round to integer q11 bytes; per-object decision noise is ~0.5–1 B. The aggregate gate is `0.0025 × 1,384,654 = 3,462 B`. So even a *zero-information* selector is safe only if fewer than ~3,462–6,924 eligible objects fall in S0. **The prereg reports no size distribution of eligible candidate objects** (§14.3 has no `n_candidates_by_stratum`). `[A]` This single unmeasured count can decide feasibility on its own, and it costs nothing to measure.
4. **[P] Therefore the correct gate is prize-relative, not absolute:**

```
regret_gate(f) = max(0.0025, 0.25 × Ceil_isolated(f))      aggregate
               = max(0.0050, 0.50 × Ceil_isolated(f))      per file
Ceil_isolated(f) = ( Σ_slots max_L o11_bytes(slot,L) − Σ_slots min_L o11_bytes(slot,L) ) / C_ref(f)
```

`Ceil_isolated` requires **zero additional backend calls** — it is computed from the O11 label table the pilot already builds. It is the exact ceiling of the selector's job under §5.1's own label definition.

### 5.3 The strong kill argument, stated at full strength

> **The pivot spends a remote publication cycle, a ≥1,500-line frozen source, 80 sampled calls and up to 13 whole-carrier encodes per file to chase a prize that measurement caps at 0.343 % of the carrier, using fidelity gates that a coin flip can pass and that a perfect selector is not shown able to clear per-file, with a speed gate measured against a baseline the project has already declared unusable, and a feature basis that is provably blind to the only mechanism measured to matter in this system (§6.1). A 19×-larger prize in the same carrier is already measured (G5A, 89,960 B, `COLUMN-DOMINANT`, 4/4 files) and already authorized for arbitration. The rational allocation of the next remote run is to the representation, not the selector.**

### 5.4 The strongest surviving case (stated at full strength)

> The pivot is **correctly specified engineering hygiene, not a discovery**. It has zero decoder delta by construction (§16.5, genuinely rare), charges no new selector bytes (per-slot leaf identity is already in the frozen G3 carrier — which is why G4 achieved 16/16 carrier-identity equalities, `I10-GROTLI-G4-RESULTS.md:48`), and its decision matrix (§17) is disciplined enough to survive its own failure: a `NO-GO-PILOT-LADDER` outcome does not condemn the selector. If O11's ranking cost is genuinely prohibitive in production — and G4's withdrawn 667.9 ms-vs-0.0048 ms figure *suggests* it is, without proving it — then a bounded selector is worth building **as a cost component of a representation project**, exactly the role MASTER-BRIEF item 15 assigns to DEFLATE reconstruction ("adopt-class prior art with real engineering value"). On that reading the correct gates are cost gates, not fidelity gates, and §5.2's prize-relative formulation is the right contract. **The surviving case is real; it is not the case the pivot as written argues.**

---

## 6. Feature-basis and mechanism-coherence audit

### 6.1 The 8 features cannot in principle see the measured mechanism

`[M]` G5A's measured effect is **ordering/locality of the identical charged token multiset**: same bytes, same count, same multiset SHA-256, same carrier body size — only the permutation changed; result 89,960 B, `COLUMN-DOMINANT`, concentrated in D3 (63,483 B) and D4 (22,823 B) (`I10-GROTLI-G5A-ORDERING-ATTRIBUTION-PREREG.md:34-61`; `I10-FRONTIER-RECON-2026-09-24.md:184,188-189`).

`[M]` All eight features (§4.7) are functions of the isolated object `o(c)` alone. An ordering effect is by construction a property of the **arrangement of the multiset** and is *invariant* under any per-candidate relabeling. Therefore:

> **A per-candidate intrinsic-entropy model is provably incapable of representing a column-ordering effect.** `x0..x7` contain no positional, neighbourhood, column-identity, or carrier-context term.

`[A]` This is not a defect the pilot can route around by tuning; it is a statement about what the feature space contains. The pivot is not *trying* to capture G5A's effect (its objective is to approximate O11), so this is not fatal to the fidelity gate — but it is fatal to any framing in which the planner is described as "G5A-informed" in the sense of exploiting the ordering result. **"G5A-informed" is, on inspection, a lineage label (the ladder and the arbitration structure come from G5A's carrier), not a mechanism claim.** `[A]` If the constructive lane's framing leans on "G5A-informed" as a mechanism justification, that is exactly the "novelty-by-difference" failure MASTER-BRIEF doctrine 1 prohibits, and I will flag it in §11.

### 6.2 Feature quality risks independent of the above

- `[M]` `x5 = H1_bits/H0_bits` (l.245) is a ratio of two no-smoothing entropy estimates and is `0` when `H0_bits == 0` — i.e. **for any object whose bytes are a single repeated value, x5 collapses to a constant that carries no information**, while the object's actual compressibility is maximal. §4.4's zero-smoothing choice means small contexts (`n_x` = 1) contribute exactly 0 bits, systematically *understating* entropy for sparse objects — the regime where leaf choice actually matters.
- `[M]` `x3 = log2(P) − log2(R), or 0 if R == 0` (l.287) and §4.2's *"If `R == 0`, ratios that require `R` use the explicit value zero"* (l.223-224) inject a **structural discontinuity** into the feature at exactly the boundary where a candidate carries no lexical tokens — a boundary that plausibly separates leaves. `[A]` A linear model with a hard zero at a decision-relevant boundary will be poorly fit there, and the ridge has no interaction or nonlinear term (§5.3 forbids them).
- `[M]` `x6 = match_coverage` uses 4-byte hashes with a ≤258-byte cap and ≤8 chain entries (l.250-264) — a *proxy for LZ matchability that is itself a compressor*. `[A]` It will correlate strongly with `x4`/`x5`, and with only 8 coefficients and λ = 1.0 on standardized features, collinearity means the effective rank of the model is lower than 8 and the per-feature diagnostics in §5.5 will be uninterpretable.

---

## 7. Prior-art map

`[A]` unless marked; this is my assessment, not a measured result, and it is the basis for the novelty ruling.

| # | Mechanism the pivot uses | Prior art | Novelty verdict |
|---|---|---|---|
| 1 | q1→q4→q6→q11 ladder, keep-4 / keep-2 / keep-2 | **Successive halving** (Karnin et al. 2016); **Hyperband** (Li et al. 2016). The ladder is a direct instance with r = 4. | **Adopt-class. Not novel.** |
| 2 | "Compress with several settings, ship the smallest, transmit a selector byte" | paq8/cmix method tournament; 7-Zip `-m0..-m9`; xz `-9e` preset ladder; Brotli's own q0–q11 as a user-facing ladder; `advdef`/`advzip` wrappers. | **Adopt-class. Not novel.** |
| 3 | **Analytic cost model to rank parse options instead of trial compression** | **zstd's optimal parser** (`Z_compressBlock_optimal`, `optPrice` / `Z_entropyCost`) — a hand-tuned additive bit-price model over literals, matches, repcodes; **LZMA SDK `Price` tables**; 7-Zip's optimal-parser price model; zlib's `deflate_slow` static-vs-dynamic tree cost estimates. | **Adopt-class. Not novel.** The pivot's 8-feature ridge is a *learned* price table. |
| 4 | Fitting a linear surrogate to a trusted compressor's output size | hyperparameter-cost surrogates (Hyperopt, RMBO/BOHB); `paq` model selection by total bytes. | **Adopt-class.** The only residual novelty locus, and it is a *parameter-estimation choice inside an adopt-class mechanism*, not a mechanism. |
| 5 | Contextual-bandit allocation of a bounded evaluation budget over candidates | Hyperband/BOHB racing; codec parameter search (`brotli -q` sweeps, `zstd --auto-level`). The pivot's §7.7 priority rule is a deterministic UCB-racing allocation. | **Adopt-class. Not novel.** |
| 6 | Bounded whole-carrier arbitration over a frozen representation set | Brotli/Zstd preset search; multi-method compressors. | **Adopt-class. Not novel.** |
| 7 | Anything in the pivot not in rows 1–6 | — | **Nothing identified.** |

**Ruling `[A]`:** the finalist planner is **adopt-class end to end**. Per MASTER-BRIEF doctrine 1 ("mechanism novelty, not novelty-by-difference") and item 15 (DEFLATE reconstruction is "adopt-class prior art with real engineering value"), the planner **cannot carry a novelty claim and cannot be the vehicle for a frontier crossing**. It may legitimately be justified — and only — by measured engineering value as a cost-reduction component of a representation project. This caps its ambition and dictates that its gates must be **cost gates**, which is exactly what §16.3 fails to be (§3.3, §4.4).

**Prior-art risk to the *representation* tracks `[A]`:** row 3 is a live threat to any ANVIL lane that proposes "an analytical MDL selector over parse candidates" as novelty. Track 17 (parser/candidate-generation algorithms) should be told explicitly that zstd's optimal-parser price model is the separator.

---

## 8. Decoder, resource, and benchmark-window risks

### 8.1 Decoder risk: genuinely zero — credit

`[M]` §16.5 (l.1034) requires the decoder model delta to be **exactly zero**, and §9 shows the planner only changes *which* leaf occupies a slot in a carrier that already transmits per-slot leaf identity. Decode bytes and decode RSS are therefore **identical** to whatever carrier is selected. There is **no decode-side risk in this pilot** — no new state, no new stream, no new table, no fuzz surface, no adversarial-format surface. `[A]` This is the pilot's most underrated strength and it should be stated in any promotion case.

### 8.2 Encoder RSS gate is a free pass

`[M]` §16.4 (l.1019-1024): `peak_rss(P1) <= peak_rss(O11)`, measured "for a fresh P1 process that performs the complete bounded portfolio." **Nothing pins how frozen G3 manages candidate-object residency during its isolated q11 ranking.** `[A]` If G3 streams, O11's RSS is small and P1's `F`-sweep plus order-1 tables can exceed it; if G3 materializes all objects, the gate is free. This is **structurally the same defect as G4's `INVALID_SPEED_ACCOUNTING`** — an unmatched denominator — and the ledger's own instruction applies verbatim (`RESEARCH_LEDGER.md:4743-4747`): *"Any future planner pilot must time equivalent work on both sides of the ratio."*

**Required:** memory-match the O11 baseline (same candidate-object residency, same allocation pattern) or the RSS gate is void.

### 8.3 Unpinned constants that hide memory

- `[M]` §4.5: *"a single 32-bit hash table"* — **size unspecified**. At 2²² entries this is 16 MB; at 2²⁰ it is 4 MB. No gate, no counter.
- `[M]` §4.4: order-1 contingency is 257 contexts × 256 values = 65,792 counters; **element type unspecified** (u8/u16/u32/u64 ⇒ 64 KB / 131 KB / 263 KB / 526 KB), and whether it is per-candidate or carrier-resident is unspecified.
- `[M]` §13.1 requires `decode_ms_by_finalist`, and §13.3 charges *"all required carrier reconstruction/roundtrip checks"* to `T_encode`. `[A]` Roundtrip-verifying 8 carriers means decoding them. This is a **correctness gate, not production work**; charging it to production encode time is conservative (it inflates P1's cost and thus helps the pilot), but it must be reported in a separate line or readers will compare a verified encoder against an unverified one.

### 8.4 Benchmark-window provenance

`[M]` §2.4 fixes every Brotli call to lgwin 30 and pins quality per use class; §2.4 also requires the linked Brotli library version, package identity, compiler, runner, and OS to be recorded and forbids per-runner/per-corpus variation. **This is clean.** `[M]` And because every D1–D4 file is ≤ 10,485,760 B < 2³⁰, **lgwin 30 is non-binding for all four** — so the isolated-vs-carrier difference is a *context/stream-sharing* difference, not a window difference. `[A]` That sharpens §2.3: the label mismatch is about shared model state, which is precisely what the pivot's isolated-object label cannot see.

---

## 9. Instability and regret regimes

`[A]` Four distinct regimes, each with a required diagnostic:

| Regime | Mechanism | Required diagnostic (currently missing) |
|---|---|---|
| **R-A quantization** | S0 objects (1–256 B) round to integer q11 bytes; ~0.5–1 B decision noise per object. Aggregate gate = 3,462 B; tolerable only if < ~3,462–6,924 S0 objects. | `n_candidates_by_stratum[f]` (§14.3 lacks it) |
| **R-B population dependence** | `[M]` G5A V1 reversed (A3 > A1) and `[M]` G3 V1 was **+3.1900 % worse** than raw. The *same frozen machinery* flips sign on a different population. A D1–D4-fit selector will face at least one V2 family where its coefficients are anti-correlated. | Per-family sign reporting; a `POPULATION-DEPENDENT` rule that blocks adoption on **any** family with positive regret regardless of aggregate |
| **R-C degenerate masks** | `[M]` `REGION_RAW` has one leaf (§2.3). On a file with no structured frames all four masks collapse, P0 = P1 = ladder = K-main, the file carries **zero information**, and it still consumes 80 q4 + up to 13 carrier calls. | `degenerate_files[f]` counter; no rule exists in the prereg |
| **R-D near-tie inflation** | `[M]` §5.5 has a near-tie diagnostic for gaps ≤ 1 B, but the gates in §16.1 are computed over **all** candidates. A file whose slots are all near-ties yields ≈ 0 regret for *any* selector and inflates the apparent pass rate — the opposite failure of R-A, on the same axis. | per-file near-tie mass reported beside per-file regret |

`[A]` R-A and R-D together mean the per-file regret column has **no reliable meaning** until both are reported. Any aggregate built from it inherits the distortion.

---

## 10. Alternatives

### 10.1 Alternative A (recommended if the planner is pursued at all): **Representation-Arbitration Ladder (RAL)**

**Invert the pivot.** Spend the *same already-authorized, already-adopted* bounded ladder on the small number of whole-**representation** choices instead of on per-candidate ranking.

- **Candidates (6–8):** raw Brotli; G3 routed under each of the four frozen masks; G5A A1 (source order of extracted scalar chunks); G5A A3 (shape-column order); optionally G5D paged-dict (track 01) once it clears its own gate.
- **Mechanism:** ladder q1 → q4 → q6 → q11, keep-2/keep-2; ship the smaller; **one selector byte** (already charged by G5A's carrier, `[M]` G5A prereg §1.2 *"charged one-byte order selector"*).
- **What it removes:** the oracle ranking entirely, the 8-feature ridge, the LOFO protocol, the 80-call q4 sampling, the stratum shrinkage constants, the `F` sweep, and the cached-label speed-accounting exposure. **Every mechanism the audit above found defective is deleted, not fixed.**
- **Cost:** ≤ 8 q1 + ≤ 4 q4 + ≤ 2 q6 + ≤ 2 q11 + 1 raw q11 — *identical in kind* to the pivot's §14.1 ladder; RAL is **not cheaper in absolute calls**, and I do not claim it is. Its advantage is that it spends the same budget on a **19× larger measured prize** (§5.1).
- **Falsifiable in one remote run**, with no learned model and no held-out label dependency.
- **Its inherited risk is R-B** (population dependence), which is exactly why raw Brotli must remain an always-paid candidate — which `C_prod` already does (`[M]` §10.5).
- **Governance note `[A]`:** G5B-ORDINAL is `ORDINAL-ADVERSE` (`+1.6507 %`, wins 1/4) and G5A-LC's own §0 warns that *"the transform must not be applied globally."* RAL must arbitrate between *named, individually-built representations*, never apply a global flattening, and must ship the raw-fallback guarantee.

### 10.2 Alternative B (if both fail, and it is nearly free): adopt zstd-style analytic price tables

Drop the planner and the ladder entirely. Insert a small hand-tuned additive price table inside the existing representation — literals, matches, repcodes, per-slot leaf choice — with **zero extra backend calls, zero sampling, zero `F` sweep, zero ladder**. `[M]` §16.5's ≤ 65,536 B compiled-encoder-model budget is satisfied trivially and decoder delta stays zero. `[A]` Prior-art status is unambiguously adopt-class (row 3), which is acceptable for a hygiene change. `[A]` Its ANVIL-side byte value is **unmeasured**; pre-register "≤ +0 complete bytes vs the current heuristic on 4 locked families, else revert" and treat it as cost reduction only.

### 10.3 What I do **not** recommend

Re-freezing the pivot with better features, a larger ridge, more strata, or a different UCB constant. `[A]` That is G4's forbidden move ("do not tune S0/S1/S2 after seeing D1-D4", `I10-GROTLI-G4-RESULTS.md:123`) restated with new numbers, and per §2.3 + §6.1 the deficiency is in the target and the feature space, not in the estimator.

---

## 11. The decisive remote-only falsification experiment (R0)

**Design constraint compliance:** remote-only (GitHub Actions); no new corpus; no new held-out data; no production edit; no publication of a pilot source required; D1–D4 only; read-only w.r.t. frozen G3.

**Question R0 answers:** *Before paying for a pilot, is the pilot's contract even satisfiable, and is its feature basis and its ladder even alive?*

R0 reuses the frozen G3/O11 label table the pilot would build anyway. **Additional backend cost: ≤ 16 whole-carrier calls per file (≤ 64 total), zero isolated calls.**

| ID | Measurement | Cost | Pre-registered rule |
|---|---|---|---|
| **R0.1** | `Ceil_isolated(f) = (Σ_slots max_L o11 − Σ_slots min_L o11)/C_ref(f)`; also `n_candidates_by_stratum[f]` | **0 calls** | emit always |
| **R0.2** | Gate scalability: `Ceil_isolated(f)` vs the frozen 0.25 % / 0.50 % | 0 | **`GATE-UNSCALED(f)` if `Ceil_isolated(f) < gate(f)`** |
| **R0.3** | Kendall τ between complete bytes at q1, q4, q6, q11 across the ≤8 finalists per file; `P(argmin at q11 ∉ survivors after R1–R3)` | ≤ 8 q1 + ≤ 4 q4 + ≤ 2 q6 + ≤ 2 q11 per file | **`LADDER-UNUSABLE` if median τ(q1,q11) < 0.70 or that probability > 0.25** |
| **R0.4** | Per-LOFO-fold, per-feature: Kendall τ(x_j, y) **minus** τ(x_j, y permuted within (leaf, size-bin) stratum); pair-decision accuracy restricted to pairs whose O11 gap ≥ max(1 B, 0.1 % of the larger object) | **0 calls** | **`FEATURE-DEAD` if no feature beats its null by ≥ 0.05 on ≥ 3 of 4 folds** |
| **R0.5** | `sum_object_bytes(f) = Σ_c len(o(c))`; `ratio = sum_object_bytes/N_src`; `mdl_feature_ms(f)`; pinned match-coverage hash-table entry count; pinned order-1 counter width | 0 | emit always; **`FEATURE-VOLUME-FAIL` if `T_features > 0.10 × T_encode(raw Brotli q11)`** |

### Pre-registered kill / promote thresholds

```
KILL-1  R0.2 GATE-UNSCALED on ≥ 2 of 4 files
        → the pivot contract is unsound. Re-freeze gates against Ceil_isolated,
          or abandon. [This KILLs the pilot as written, NOT the engineering idea.]

KILL-2  R0.4 FEATURE-DEAD
        → the 8-feature basis carries no signal beyond its own null; no ridge on
          these features can work. Do not run.

KILL-3  R0.3 LADDER-UNUSABLE
        → NO-GO-PILOT-LADDER is predetermined. Either drop the ladder, or run
          selector-only fidelity with no ladder claim.

KILL-4  R0.5 FEATURE-VOLUME-FAIL
        → no speedup is possible by construction. Abandon.

KILL-5  (post-run) any q11 candidate-ranking call > 0; OR any cached/reused O11
        label in the timing path; OR o11_q11_candidate_calls != candidates_enumerated;
        OR any cross-job timing splice  →  INVALID-PILOT (G4 precedent, verbatim).

PROMOTE (HOLD → PILOT) requires ALL of:
  R0.2 GATE-UNSCALED on ≤ 1 file, with gates re-frozen prize-relatively:
      aggregate gate = max(0.0025, 0.25 × Ceil_isolated)
      per-file  gate = max(0.0050, 0.50 × Ceil_isolated)
  R0.4 not FEATURE-DEAD, AND the same R0.5 counters added to §14.3:
      sum_object_bytes, n_candidates_by_stratum, o11_label_build_ms,
      p1_q11_calls_total, match_coverage_table_entries, order1_counter_width
  R0.3 median τ(q1,q11) ≥ 0.70
  planner_overhead gates of §4.4 replace §16.3's ratio gates

PROMOTE-TO-REMOTE (production planner) requires, in ADDITION:
  a V2 held-out family, frozen by name/bytes/blob/SHA-256 BEFORE fetch, on which
      P1 aggregate regret ≤ max(0.0025, 0.25 × Ceil_isolated)
      AND worst-family regret ≤ 0.50 %
      AND Ceil_isolated ≥ 0.10 % on EACH family
          (below 0.10 % the family cannot discriminate a planner from a coin flip)
      AND no family shows the R-B sign flip
  and §16.5's zero decoder delta and ≤ 65,536 B compiled-model gates re-confirmed.
  No held-out V2 corpus currently exists [M] (RESEARCH_LEDGER.md:4831), so this is
  not reachable in the current cycle.
```

---

## 12. Final recommendation

# HOLD

Not **KILL**: the engineering value is real, the decoder delta is genuinely zero, the spec's discipline is above the project's average, and §17's separation of ladder failure from selector failure means a negative result would be interpretable. A cheap R0 could still vindicate the idea at ~64 whole-carrier calls.

Not **PILOT**: as written the contract fails on four independent, measured grounds — (1) the aggregate gate is 72.9 % of the entire prize ceiling and the per-file gate is 1.80× the largest gain ever measured, so the gates cannot distinguish a selector from a coin flip while not being shown attainable by a perfect one; (2) the speed baseline is a research oracle the project has already declared unusable, and the same `INVALID_SPEED_ACCOUNTING` failure mode has **no machine gate** preventing a repeat; (3) the incremental production cost is `[P]` ≈ 3× a raw Brotli q11 encode on a 10 MB file against a ≤ 0.343 % prize; (4) the feature space is provably incapable of representing the only mechanism measured to matter in this system.

Not **PROMOTE-TO-REMOTE**: no source exists, no workflow exists, dispatch is blocked on publication authorization, `[M]` D1–D4 cannot support a generalization claim and `[M]` no held-out V2 corpus exists, and `[A]` the mechanism is adopt-class end to end with no novelty claim available — it can only ever be an engineering component, never the vehicle for a frontier crossing.

**HOLD converts to PILOT** on: R0.2 GATE-UNSCALED ≤ 1 file, R0.4 not FEATURE-DEAD, R0.3 τ(q1,q11) ≥ 0.70, R0.5 not FEATURE-VOLUME-FAIL, with gates re-frozen prize-relatively and the six new §14.3 counters added — all in one remote run, all preregistered above before dispatch.

**Independent recommendation to the coordinator, orthogonal to the pivot:** prioritize **RAL (§10.1)** over the pivot. It reuses the same authorized ladder, deletes every defect enumerated above rather than repairing it, and targets a measured 19× larger prize in the same carrier. If RAL's V2 arbitration clears raw Brotli on a locked held-out family, that is the finding worth a Pareto claim; the selector is at best a cost footnote inside it.

---

## 13. Reconciliation with the constructive lane (`02-finalist-planner-space-bunny.md`)

**Read before finalizing.** 591 lines. Status: INTERIM CHECKPOINT, constructive inventor. Sections below are my adjudication, not a summary.

### 13.0 The headline: LCPS *is* my Alternative A, arrived at independently — and that is a win for the report, not for the mechanism

`[M]` LCPS §2 (constructive lines 77-110) argues: *"The leaf axis is expensive to plan… The oracle is inside the loop. The layout axis is cheap to plan by construction… Its cost is a **single complete-carrier measurement per candidate layout**… **The oracle is outside the loop.** Therefore the decisive planner move is to relocate the expensive-planning budget from the axis with an inside-the-loop oracle (leaf) to the axis with an outside-the-loop oracle (layout)."*

That is **verbatim my §10.1 Alternative A (RAL)**. The constructive lane reached it independently, with *better* internal evidence than I had — `[M]` F20 establishes the frozen 32-selector alphabet already exists in `I10-G5A-LOCALITY-CONTROLS-PREREG.md:477-516,887-919`, including `SHAPE_COLUMN`, `COLUMN_OCCURRENCE_SHUFFLE_V1`, `COLUMN_BLOCK_SHUFFLE_V1`, plus Stage T post-transform reorders. I did not have that when I wrote §10.1.

**Consequence:** my §12 recommendation to prioritize representation arbitration over the selector is **independently confirmed** by the constructive lane. Two independent derivations, same answer. That materially raises the weight of the staged-order recommendation in §15.

### 13.1 Does the shift from leaf-prediction to layout-planning escape my target-misalignment argument? — **Partially, and only on one of two axes**

This is the question put to me directly. The honest answer is split, and the split is exactly where the risk lives.

**Where it genuinely escapes — the layout axis.** `[M]` My §2.3/§5.0 argument is that a *surrogate* fitted to an *isolated* label cannot predict *carrier* bytes. A layout ladder has **no surrogate**: a layout is scored by a whole-carrier q11 directly, so there is no target to be misaligned with and no feature space in which misalignment can hide. `[A]` **LCPS is correct that the layout axis escapes my objection entirely.** This is a real and material concession and I record it as such.

**Where it does not escape — the leaf axis, which LCPS retains.** `[M]` LCPS §3.3 S4: *"re-run S2 with the layout-conditioned price function; pick argmin leaf per (shape, slot) inside the applicable family mask."* Every defect I raised survives untouched:

| My finding | Status inside LCPS |
|---|---|
| §2.3 label is isolated-q11, objective is carrier bytes | **unchanged** — S4 still scores per-candidate against the same §5.1 label |
| §6.1 features are arrangement-invariant | **unchanged** for the leaf axis — layout is now an *input*, but each candidate's own features remain intrinsic |
| §2.4 8,192-byte prefix bias | **unchanged** — LCPS §8.3 acknowledges it as a "concrete defect risk" and defers the fix ("decided before dispatch") |
| §5 gate mis-scaling | **unchanged** — `[M]` LCPS `LCPS-LEAF-FIDELITY` = aggregate ≤ 0.25 %, per-file ≤ 0.50 %, sourced from its own F11 (G4's frozen gates) |
| §3.3 speed baseline is a declared-unusable oracle | **unchanged** — `LCPS-SPEED-PLAN ≥ 5x` against same-job O11 |

**So: the mechanism moved; the gates did not.** `[A]` That is the precise sense in which the shift is partly a relabeling. LCPS-1 would report `LCPS-LEAF-FIDELITY` as a gate without having repaired the object that gate measures.

### 13.2 The decisive technical objection: LCPS-H1 and LCPS-H2 are on different currencies, and LCPS-1 contains no link measurement

`[M]` LCPS-H1 (§3.1, §3.2) is a claim about **estimator dispersion**: `disp(COLUMN)/disp(SOURCE) ≤ 0.50`.
`[M]` LCPS-H2 is a claim about **carrier bytes**: O11 complete-carrier regret ≤ 0.25 % / 0.50 %.

`[A]` **Dispersion is a property of the estimator's error distribution. The target mismatch is a bias in what the estimator is asked to predict.** Conditioning the layout reduces dispersion under a *fixed* target; it does not remove the bias between "bytes this object would take in isolation" and "bytes this object costs inside the shared carrier stream". A tighter estimate of a biased quantity is still a biased estimate. LCPS's own words concede the gap — §3.1 says *"the leaf axis becomes **plannable**"*, and *"more plannable"* is not *"O11-faithful at 0.25 %"*, which is what H2 demands. **LCPS-1 as designed cannot distinguish "conditioning reduced dispersion" from "conditioning moved carrier bytes", because nothing in it measures the mapping between the two.**

**Required, and cheap:** add a **link arm** reporting, on the *same* coordinates, (a) the dispersion ratio and (b) the *realized* carrier-byte regret delta between arms A and B. Zero extra backend calls — arms A and B already exist in LCPS-1's table. Without it, H1 is decorative.

### 13.3 Un-registered outcome: H1 passes, H2 fails

`[M]` LCPS §7 handles `H1 fail ∧ H2 pass` explicitly (`ENGINEERING-ADOPT-NO-NOVELTY`). It does **not** register the outcome that actually matters:

> **H1 passes (conditioning demonstrably works) ∧ H2 fails (carrier bytes still miss 0.25 %/0.50 %).**

`[A]` That outcome means: *the best-case conditioning available in this carrier does not rescue the leaf axis, and G4's negative reproduces under the most favorable conditions.* It is strictly stronger evidence against the planner than G4's original NO-GO, and it is currently unregistered — so a run could produce it and no frozen ruling would exist. **This must be pre-registered before dispatch.** It is the most consequential omission I found in the constructive lane, because it is the outcome the whole design was built to avoid and has no disposition.

### 13.4 New attacks found only after reading LCPS

1. **LCPS-1's layout ladder substantially duplicates the already-frozen G5A-LC.** `[M]` G5A-LC §0 item 4 asks *"whether the ordering effect survives after dictionary or integer leaves transform the payload"* — which **is** LCPS-1's arm-D lattice question; item 1 asks *"exact-shape column locality vs arbitrary coherent permutation"*; item 5 asks *"whether the result depends on exact A2/A3 permutations being live rather than byte-size ties"* — which **is** LCPS-1's `LCPS-LAYOUT-RECOVERY` against `ORACLE-L`. **G5A-LC is frozen, already authorized, and already contains most of LCPS-1's decisive layout measurements.** Running LCPS-1 first double-pays for the layout axis. See §15 Stage 2.
2. **Internal inconsistency in the frozen call budget.** `[M]` §3.3 S3 caps the ladder at `≤2` q6 and `≤2` q11; §7's `LCPS-CALL-BUDGET` permits `q6 ≤ 4`. A 16→8→4→2 ladder cannot keep 4 positions at q6 and 2 at q11 without an unregistered extra stage. Dead headroom in a contract whose selling point is byte-level accounting discipline.
3. **The 634× anchor is corpus-unstable.** `[M]` LCPS §1.1 derives `q1/q11 = 634.5×` from F23 and correctly flags that it is a whole-input ratio. But `[M]` **no Class A q4 or q6 encode rate exists anywhere in the ledger**, so every q4/q6 cost term in LCPS §6 and in pivot §13 interpolates across a 634× range measured at exactly two endpoints. `[A]` Cross-check against `docs/CONTEXT.md`'s per-file Linux figures for an 827 KB JSON (q4 ≈ 86.6 MB/s, q9 ≈ 22 MB/s) gives a q9/q4 ratio of ~0.25×, not 634×. The anchor is **corpus-dependent by orders of magnitude**. LCPS §6's mitigation ("q1 is ~634x cheaper than q11 on the measured anchor") is therefore not a mitigation. Reproduced as §8 of `prize_gate_audit.py`.
4. **`ORACLE-L` is the cost floor of LCPS-1, not an afterthought.** `[P]` 32 q11 whole-carrier calls/file × 4 files = 128 calls ≈ **703 s of q11-class backend time** on Class-A-anchored rates, *before* rebuilding C0's O11 isolated-q11 population whose cardinality is unmeasured. See §15.

### 13.5 Points where the constructive lane is right and I was too harsh

- **Credit where due, and it is significant:** LCPS §4.2 binds the F12 correction as a *design rule* ("every speed ratio must time label construction on both sides… machine-checked, not promised") — stronger than my §3.1, which asked only for the guard. LCPS §8.9 independently identifies my §8.4 oracle-control cost leak. LCPS §8.3 independently identifies my §2.4 prefix-bias defect. **Three of my findings were independently rediscovered inside the constructive lane**, which raises my confidence in all of them.
- LCPS §5.2 R1/R2 concede the novelty claim is marginal-to-weak and defer to the kill team. More honest than most novelty claims in this swarm; I withdraw any implication that LCPS oversells it.
- LCPS's ladder-streaming commitment (§4.3) resolves my §8.3 RSS concern for the ladder body, though not my unpinned-constant concern.

### 13.6 Points on which we agree, now mutually reinforcing

Both reports independently conclude: (i) layout arbitration is the higher-value axis; (ii) the q11-ranking oracle must stay out of the production loop; (iii) symmetric same-job timing is mandatory after F12; (iv) the leaf estimator's residual is the real blocker; (v) **no held-out V2 corpus exists, so no promotion is reachable this cycle**; (vi) adopt-class must be labelled as such. `[A]` Six of six. The disagreement is entirely about **whether the pivot contract's gates survive the mechanism change** — and they do not.

---

## 14. CROSS-TRACK: are Track 02 (LCPS) and Track 16 (CDR) distinct mechanisms?

**Verdict: no. After the Track 16 critic's EXP-C redundancy finding, both reduce to one object.**

`[M-relayed]` Coordinator-relayed Track 16 critic findings, second-hand: **EXP-C** proposes zero-new-wire cardinality gating, and finds CDR's **region layer likely redundant with existing G3 per-slot leaf IDs**. *I could not verify these against a file: no `16-segmentation-routing-fledge.md` exists in `docs/swarm-2026-10-02/` at the time of writing. I take them as stated and mark them `[M-relayed]`.*

### 14.1 The redundancy, worked through

`[M]` Track 16 §6.1 `E2 REGIONIZE`: *"coarsen frame groups into R regions by **(shape, leaf) run-length**."* The region partition is therefore **a function of the leaf assignment**.
`[M]` `[A]` G3's envelope already transmits **per-(shape, slot) leaf IDs** (pivot §3.2 analogue; the reason G4 achieved 16/16 carrier-identity equalities).

Two exhaustive branches:

- **Branch 1 — regions are coarsenings of per-slot leaf runs.** Then a region's leaf choice *is* the per-slot leaf choice. CDR's region layer adds **no new degree of freedom**; it contributes only a merge objective (§4.4) and a wire charge `W_CDR` (`[M]` Track 16 Gate C requires `R ≤ 3,011` and `W_CDR/payload ≤ 0.0050` — a **cost** with no new freedom). **CDR collapses onto the pivot's S4 / LCPS's S4.**
- **Branch 2 — regions may disagree with per-slot leaf IDs.** Then CDR authorizes a **second, coarser leaf channel** the G3 envelope does not carry. That is (i) a **new decoder-visible field**, contradicting Track 16 §5.1's own claim that *"CDR delta is exactly zero new tables"*; and (ii) functionally a **permutation of the token matrix at region granularity** — which **is** the layout axis, i.e. LCPS's S3.

**There is no third branch.** Either CDR is the pivot's leaf selector, or CDR is LCPS's layout ladder. Both are Track 02.

### 14.2 The deeper unity: same currency, same unknown, same unexamined bet

Strip the granularity and both tracks spend **the same currency** — a bounded number of **whole-carrier backend measurements** — to estimate **the same unknown**, `E[C_carrier | decisions]`:

| | Track 02 / LCPS | Track 16 / CDR |
|---|---|---|
| bounded-measurement axis | layout (permutation of the fixed multiset) | region (leaf run-length) |
| whole-carrier probes | ≤ 16 q1 + 8 q4 + 4 q6 + 2 q11 | ≤ 5 (ARM-1), 0 (ARM-0) |
| what it estimates | carrier bytes given the layout | carrier bytes given the region routing |
| the unexamined bet | that q1/q4/q6 ordering transfers to q11 | that a rank-1 displacement term in the G5A column-ordinal basis captures the transfer |

`[A]` Both are **the same statistical act at two granularities**: sample carrier-level cost at a coarse decision granularity and extrapolate to a fine one. Neither supplies a model of the transfer. The "oracle outside the loop" framing (§13.0) is real for the layout axis, but it is *not* a distinct mechanism — it is the observation that the coarse decision is cheap to measure directly. **There is exactly one family here: bounded whole-carrier probing of a coarse routing decision, with a cheap surrogate supplying the fine decisions.** Funding both as separate mechanisms double-charges the same planner budget — a risk Track 16 §14 itself names (*"Promoting two planner surfaces at once risks double-charging the same planner budget and muddying attribution"*). I concur and sharpen it: the risk is worse than muddying, it is **double-spending on one mechanism**.

### 14.3 What genuinely survives as distinct — and it is not a mechanism

`[A]` The one non-redundant thing Track 16 contributes is **diagnostic, not mechanistic**: its Gate A decomposition of the O11-vs-S2 residual into (a) local and (b) interaction components, with the claim that (b) is large (`[M]` Track 16 §1.4: EXP. X had **zero sign flips over 473,985 samples** and median |err| ≤ 16 %, yet G4's best surface still missed by 1.5388 % worst-file). That is a genuinely sharp anomaly and it is the **right question**. It is also measurable at **zero backend cost** (it reuses the G4 label table) — which is why it belongs in Stage 0, not in a separate CI run.

---

## 15. Staged experiment order — ranked by information gain per backend call, not enthusiasm

**No new thresholds are introduced here.** Every gate below is either frozen elsewhere (F11/G4, pivot §16.1, LCPS §7, CDR §10) or is my own pre-registered R0/KILL rule from §11, written before any of these reports existed. Nothing moves in response to LCPS's or CDR's content.

### Stage 0 — ONE first remote experiment: the merged zero-backend-call audit

**This is the answer to "pick ONE first experiment". It is neither pilot. It is a single remote run spending ZERO new backend calls.**

| Component | Origin | Backend cost |
|---|---|---|
| **R0.1** `Ceil_isolated(f)` + `n_candidates_by_stratum[f]` | my §11 | **0** |
| **R0.2** gate scalability vs 0.25 % / 0.50 % | my §11 | **0** |
| **R0.4** per-feature Kendall τ vs within-stratum null, LOFO | my §11 | **0** |
| **R0.5-lite** `Σ_c len(o(c))`, hash-table entries, order-1 counter width | my §11 | **0** |
| **CDR-Gate-A-lite** local-vs-interaction decomposition of the O11-vs-S2 residual | Track 16 Gate A, byte-only form | **0** |
| **EXP-C** region-layer redundancy vs G3 per-slot leaf IDs (Branch 1/Branch 2) | Track 16 critic, relayed | **0** |
| **UNIFICATION** do LCPS-S4 and CDR-E3/E6 emit the same decision from the same frozen G3 slot enumeration with the bounded correction removed? | this report §14 | **0** |

**Data source:** the frozen G3 O11 isolated-q11 label table, rebuilt in-job — already authorized by pivot §11.1 and already paid for by G4 run `35952830106`. **No corpus is re-measured. No held-out corpus is opened. No new carrier is built.**

**What it kills, at zero cost:**

- **Track 02's contract.** `[P]` If `Ceil_isolated(f) < 0.25 %`/`0.50 %` on ≥ 2 files → `KILL-1` fires. **This survives LCPS's mechanism shift**, because `LCPS-LEAF-FIDELITY` inherits the identical F11 gate. Confirmed by `prize_gate_audit.py` §2: the per-file gate is **1.801×** the largest single-file pair gain ever measured, and an uninformative-but-not-anti-correlated selector is expected at **0.1714 %** — *below* the 0.25 % aggregate gate.
- **Track 02's feature basis.** `[P]` If no feature beats its within-stratum null by ≥ 0.05 on ≥ 3 of 4 folds → `KILL-2`. **This also fires against LCPS's price function**: conditioning changes dispersion, not the presence of signal. A null-informative intrinsic feature does not become informative by being conditioned on.
- **Track 02's feature volume.** `[P]` If `T_features > 0.10 × T_encode(raw q11)` → `KILL-4`, and LCPS's speed argument dies with it, since both plans rest on the same Θ(structured-bytes) pass.
- **Track 16's region layer.** `[P]` If EXP-C finds Branch 1 → `KILL-CDR-v1`: no new degree of freedom, only a wire charge.
- **The two-track framing itself.** `[P]` If UNIFICATION returns "identical decision" → the coordinator is funding one mechanism twice and should merge the tracks before either pilot is published.

**Yield: five live hypotheses resolved at zero backend calls.** That is the maximum information gain per call available anywhere in this swarm.

### Stage 1 — only if Stage 0 survives: ladder rank stability (my R0.3)

`[P]` **≤ 60 whole-carrier calls, ≈ 66 s of q11-class backend time, lower bound** (`prize_gate_audit.py` §7; q1/q4/q6 terms omitted because no Class A rate exists for them — §13.4 item 3).

Emits Kendall τ(q1,q11), τ(q4,q11), τ(q6,q11) over the finalists, and `P(argmin at q11 ∉ survivors after R1–R3)`. `LADDER-UNUSABLE` if median τ(q1,q11) < 0.70 or that probability > 0.25.

**Why second:** it is the cheapest thing that measures the axis *both* tracks bet on, and it is a **predictor** of `NO-GO-PILOT-LADDER` / `NO-GO-LCPS-LADDER` before either is spent.

### Stage 2 — G5A-LC, not LCPS-1, for the layout prize

`[M]` G5A-LC is **already frozen r1**, already gated, and already contains LCPS-1's decisive layout measurements (§13.4 item 1). **Dispatch it before LCPS-1.** It measures the 18.95×-larger prize for the marginal cost of a contract already written, and it does so **without** attaching LCPS's leaf estimator — so a null result cannot be blamed on the estimator, and a positive result is attributable to layout alone. LCPS-1's marginal content over G5A-LC reduces to the dispersion claim and the arm-D lattice, neither of which needs `ORACLE-L`'s 128 calls to be tested.

### Stage 3 — LCPS-1 or CDR EXP-1: **at most one**, and only after Stages 0-2

If both survive Stage 0, fund **LCPS-1**, not CDR EXP-1. Reasons, none of them rhetorical:

1. `[A]` LCPS's decision axis survives §14 Branch 2's elimination (layout is a genuine new degree of freedom; the region layer is not).
2. `[M]` LCPS already has a frozen pilot contract, a frozen alphabet (F20), a zero-decoder-delta property, and a `+1 B` selector already charged by G5A's contract; CDR EXP-1 must first be reconciled with G3's envelope (§14.1 Branch 2 would require a new decoder-visible field, contradicting its own §5.1).
3. `[A]` LCPS's `LCPS-MECH-SUFFICIENCY` gate is a **killable novelty gate**, which is worth more than CDR's Gate A — a gate that can return "mechanism is adopt-class" is more informative per run than one that can only return "bytes improved".

**And if both must run, they must run in the same CI job** so the shared frozen G3 identity, the O11 label table, and same-job symmetric timing serve both — never as two jobs with spliced timings, which is precisely how G4's speed column died.

### Explicitly NOT first, and why

- **LCPS-1 as written.** `[P]` Its `ORACLE-L` control alone is **128 calls ≈ 703 s** of q11-class backend time (`prize_gate_audit.py` §7) — **2.1× the calls and 11× the backend seconds of Stages 0+1 combined** — plus C0's unmeasured O11 isolated-q11 population. It cannot distinguish "no layout prize" from "my ladder missed it" until that control completes. **It cannot be first when it costs 11× more backend time than the audit that determines whether its own gates are satisfiable.**
- **CDR EXP-1.** Deferred pending EXP-C. Track 16 §14 already argues against promoting two planner surfaces at once; §14.1 shows that argument is stronger than it realized.
- **Any q4/q6-rate measurement** tempting as a "cheap add-on": it would be the first new Class A encode-rate interior point, genuinely useful, but it is an *instrumentation* result, not a mechanism result, and must not gate a planner decision.

### 15.1 One-line answer

> **Run Stage 0 — one remote job, zero new backend calls, five hypotheses resolved — and only then G5A-LC for the layout prize. Run LCPS-1 and CDR EXP-1 at most once between them, in the same CI job, and only if Stages 0-2 leave the leaf axis alive. The 18.95× prize is measured on the layout axis; the ≤ 0.343 % prize is on the selector axis; do not spend 703 s of backend time to learn which of two mis-scaled gates is worse.**

---

## 16. Revised final recommendation (supersedes §12)

# HOLD

**Unchanged in verdict, materially sharpened in sequencing.**

- **HOLD** stands. The constructive lane's shift to layout arbitration **independently confirms** my §10.1 (§13.0) and removes my strongest objection to *that axis*. It does not repair the leaf axis (§13.1), the gates (§13.2, §13.3), or the accounting (§13.4).
- **One first remote experiment:** the **Stage 0 merged zero-backend-call audit** (§15). Not LCPS-1, not CDR EXP-1. Justification in calls and bytes, not intuition: **0 new backend calls, 5 hypotheses resolved; versus 128 calls / 703 s for `ORACLE-L` alone before LCPS-1 yields a single verdict.**
- **Track 16's EXP-C is promoted into Stage 0** as a first-class component, on the coordinator's relayed finding, at zero backend cost.
- **Track 16's CDR EXP-1 is deferred**, not killed. If EXP-C returns Branch 2 (regions may disagree with per-slot leaf IDs), CDR becomes a layout-arbitration variant and should be **merged into LCPS's Stage 2 G5A-LC run** rather than run separately.
- **Track 02 LCPS-1 is second in line, not first**, and must pre-register the **H1-pass ∧ H2-fail** ruling (§13.3) and add the **link arm** (§13.2) before dispatch. Both are edits to a preregistration, not to a threshold — no frozen constant moves.
- **KILL conditions from §11 are unchanged and were frozen before either constructive report existed.** `prize_gate_audit.py` reproduces every ratio so a reviewer can confirm them without running anything.

**Still not PROMOTE-TO-REMOTE:** no held-out V2 corpus exists `[M]` (`RESEARCH_LEDGER.md:4831`); the mechanism family is adopt-class `[A]` (§7, plus §13.5 credit where LCPS's novelty gate is honestly self-limiting); no production source is authorized.

---

## 13-legacy: note

The original §13 of this report was an empty reservation, written before the constructive lane existed. It is superseded in full by §13 above, which was produced after reading `02-finalist-planner-space-bunny.md` (591 lines).

---

## Appendix A — Reproducibility of the §5 arithmetic

`prototypes/swarm-2026-10-02/02-finalist-planner/fledge/prize_gate_audit.py` recomputes every ratio in §4.3, §5.1 and §15 from the `[M]` constants cited in §1.2 only, and reproduces the Stage-0/Stage-1/LCPS-1 backend-cost comparison of §15. It reads **no corpus, touches no binary, runs no benchmark, and performs no fuzzing** — it is an arithmetic audit of published numbers. Run it with any Python 3; no dependencies. Verified output on 2026-10-02: prize ceiling **0.3429 %**, aggregate gate **72.9 %** of it, per-file gate **1.801×** max pair gain, representation/selector prize ratio **18.95×**, D3 incremental cost **3.00× raw q11**, `ORACLE-L` **128 calls / 703.1 s** vs Stages 0+1 **60 calls / 65.9 s**.

## Appendix B — Source artifacts cited

- `docs/I10-G5-PLANNER-PIVOT-PREREG.md` (frozen r1) — the audited contract
- `docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md` (§1, §10.5-§10.7) — O11-is-an-oracle; Q2 denominator
- `docs/I10-GROTLI-G4-RESULTS.md` — NO-GO-G4; proxy regrets; SEPARABLE-ENOUGH
- `docs/I10-GROTLI-G5A-ORDERING-ATTRIBUTION-PREREG.md` §1 — the single-mechanism ordering constraint
- `docs/I10-G5A-LOCALITY-CONTROLS-PREREG.md` §0 — G5A per-file/aggregate values
- `docs/I10-FRONTIER-RECON-2026-09-24.md` §1-§3 — reconciled G3/G4/G5A/G5B table; `INVALID_SPEED_ACCOUNTING`; Class A rates
- `RESEARCH_LEDGER.md` PART XIV §G3/G4/G5A/G5B + portfolio decisions + verification addendum — ledger of record
- `docs/swarm-2026-10-02/MASTER-BRIEF.md` — binding doctrine

**No existing file was modified. No commit, push, reset, clean, stash, restore, or rebase was performed. No local corpus or performance benchmark and no fuzzing was run.**