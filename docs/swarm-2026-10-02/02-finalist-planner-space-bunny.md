# Track 02 — G5A-Informed Bounded Finalist Planner (Space Bunny Free)

**Status:** FINAL for this lane. Supersedes the interim checkpoint of the same path.
**Date:** 2026-10-02
**Track:** `02-finalist-planner` · **Role:** constructive inventor
**Production source modified:** no. **Existing files modified:** none. **Commits/pushes:** none.
**Local benchmarks / sweeps / fuzzing:** none run, none proposed.
**Prototype:** none built. Rationale in §10.

---

## 0. Verdict

> # **HOLD** the frozen `I10-G5-PLANNER-PIVOT` contract. Do not dispatch it.
> # Adopt instead **R1**, a 4-arm, **≤ 20 whole-carrier encodes** pre-flight derived in
> # this report, *before* any new pilot result exists.

I do **not** reach this verdict by accepting the adversarial audit's reasoning, and I
do **not** defend the frozen gate by rhetoric. Two of the audit's four grounds I
**refute from same-window evidence** (§3). Two I **concede** (§4). And I contribute a
**third, independent, and more serious** ground that neither the audit nor my own
interim checkpoint found, which invalidates the *premise both of us were arguing
about*: **the frozen G3 carrier already emits in G5A's column-major order, so the
89,960 B ordering prize does not exist on the carrier the planner would operate
on** (§2). That single fact removes the 19× prize inversion *and* my own
"relocate planning to the layout axis" thesis in one stroke.

Recommendation in one line: the planner question has not been falsified, but it has
been **mis-asked**; R1 costs ~4% of one CI job and answers the real question.

---

## 1. Evidence map — measured facts, with exact provenance

Window tags are mandatory in this report and are never mixed into one aggregate.

| tag | window |
|---|---|
| **[GHA-24]** | G0–G5B GitHub Actions runs, 2026-09-24/25: G3 `35947013432`, G4 `35952830106`, G5A `35985412906`, G5B-corrected `36011333908` |
| **[SRC]** | frozen source read this session, no execution: `tools/grotli_g3.cpp`, `tools/grotli_g5_planner_pivot.cpp`, `tools/grotli_g4_planner.cpp` |
| **[LOCAL-I9]** | canonical I9 binaries/grid; ranking-grade only |
| **[P]** | projection, arithmetic shown, premise named |

| ID | Fact | Value | Source |
|---|---|---|---|
| M1 | G3 D1–D4 routed portfolio | `1,384,654 B` vs `1,466,769 B` raw = **−5.598359%** | `docs/I10-GROTLI-G3-RESULTS.md:52-58` **[GHA-24]** |
| M2 | G3 D3 causal cell | `1,240,155 B` vs `1,292,757 B` = **−4.0690%**; 11,228 structured frames + 1 raw residual frame | same `:50,63-79` **[GHA-24]** |
| M3 | G3 held-out V1 | `2,179,615 B` vs `2,112,235 B` raw = **+3.1900% worse**; 100% structured, 0 residual, **3** shapes, 0 int leaves | same `:98-117` **[GHA-24]** |
| M4 | G4 identity | O11 vs independently built frozen-G3 carrier **16/16 exact** | `docs/I10-GROTLI-G4-RESULTS.md:38-52` **[GHA-24]** |
| M5 | G4 proxy regret (ledger-corrected) | aggregate S0 `−2.0353%` S1 `−1.1453%` S2 `−0.2120%`; **worst-file S0 `6.6056%` S1 `7.0802%` S2 `1.5388%`** | `RESEARCH_LEDGER.md:4728-4730` **[GHA-24]** |
| M6 | G4 ruling | **`NO-GO-G4`**; `DSTAR=None`, `ISTAR=None`, Q1 FAIL | `docs/I10-GROTLI-G4-RESULTS.md:32,84-90` **[GHA-24]** |
| M7 | G4 speed column **VOIDED** | `1.0218x / 0.4614x / 0.0591x` withdrawn as `INVALID_SPEED_ACCOUNTING`; O11 denominator read **cached** ranking labels while q11 label build was timed separately | `RESEARCH_LEDGER.md:4731-4737`; corroboration `tools/grotli_g4_planner.cpp:921-957,1411-1423`, `.github/workflows/anvil-i10-grotli-g4.yml:671-680` **[GHA-24]/[SRC]** |
| M8 | G4 Q2 | 24 pairs, `max_pair_gain = 0.277632%`, `agg_pair_gain = 0.057144%`, **`SEPARABLE-ENOUGH`** | `docs/I10-GROTLI-G4-RESULTS.md:96-104` **[GHA-24]** |
| M9 | G5A charged same-multiset totals | A0 `2,054,532` / A1 `1,470,205` / A2 `1,452,383` / A3 `1,380,245 B` | `RESEARCH_LEDGER.md:4756-4757` **[GHA-24]** |
| M10 | G5A classification | `ORDER-MATERIAL`, `COLUMN-DOMINANT`; A3 saves **89,960 B** vs A1; smaller on **4/4** | `RESEARCH_LEDGER.md:4757-4759` **[GHA-24]** |
| M11 | G5A decomposition | shape `17,822 B`; column `72,138 B` = **80.1889728768%** | `docs/I10-G5A-LOCALITY-CONTROLS-PREREG.md:20-21` **[GHA-24]** |
| M12 | G5A V1 **adverse** | A3 − A1 = **+1,721 B**; A1 already `70,030 B` worse than raw | same `:25` **[GHA-24]** |
| M13 | G5B-ORDINAL | B0/B1/B2 = `2,054,910 / 1,380,245 / 1,403,029`; B2 **+1.6507%**, wins 1/4; `ORDINAL-ADVERSE` | `RESEARCH_LEDGER.md:4786-4789` **[GHA-24]** |
| M14 | Pivot contract frozen, never executed | "Workflow status: not created"; "reports missing links and has no frozen workflow" | `docs/I10-G5-PLANNER-PIVOT-PREREG.md:13`; `RESEARCH_LEDGER.md:4840-4842` |
| M15 | Pivot source exists, unwired | `tools/grotli_g5_planner_pivot.cpp`, 1921 lines, includes G3 via `G5P_FROZEN_G3_HEADER` | `tools/grotli_g5_planner_pivot.cpp:1-6` **[SRC]** |
| M16 | Pivot 8 features | `x0=log2 N, x1=log2 m, x2=D/m, x3=log2 P−log2 R, x4=H0/N, x5=H1/H0, x6=match_coverage, x7=coded_bits/occ` — **all functions of the isolated object `o(c)` alone** | `docs/I10-G5-PLANNER-PIVOT-PREREG.md:279-297` |
| M17 | Pivot label | `y = log2(max(1, o11_isolated_q11_bytes))` — explicitly *"not a complete-carrier byte value"* | same `:301-311` |
| M18 | Pivot gates | agg `≤0.0025`, per-file `≤0.0050`, ladder `≤0.0010/0.0025`, speed `≥5.0` mand / `10.0` target, RSS `≤` O11, model `≤65536 B`, decoder delta **exactly 0**, `q11_rank=0` | same `:976-1043` |
| M19 | Pivot sample cap | isolated q4 sample input = `o(c)` truncated to first **8192 B** | same `:416-421` |
| M20 | Pivot R1 call count bound | "at most eight q1 calls are paid" | same `:606-616` |
| M21 | G3 binary digest is **single-sourced** | `ac980f35…` appears in exactly one document; not independently recomputed anywhere | `docs/I10-GROTLI-G3-RESULTS.md:6` vs `RESEARCH_LEDGER.md:4741` (G4 digest appears in both) |
| M22 | Class A encode rates | Brotli q1 `432.106 MB/s`, q11 `0.681 MB/s` (harmonic) | `RESEARCH_LEDGER.md:4862-4863` **[LOCAL-I9]** |
| M23 | No unopened held-out structured corpus | "No new held-out structured corpus is currently available" | `RESEARCH_LEDGER.md:4831-4833` **[GHA-24]** |
| M24 | Corpus | D1 `277,673` · D2 `615,350` · D3 `10,485,760` · D4 `3,585,053 B` | `docs/I10-G5-PLANNER-PIVOT-PREREG.md:177-180` |
| M25 | Frozen frontier | `33 non-dominated | 5 FRONT-GAP | 0 FRONT-CROSSING | 28 DEGENERATE | 435/468 dominated` | `RESEARCH_LEDGER.md:4711-4714` |

---

## 2. NEW FINDING — the G3 carrier is *already* column-major (§2 invalidates both prior framings)

Read from source, not inferred. This is the load-bearing result of the report.

### 2.1 What G3 actually emits

`tools/grotli_g3.cpp:1203-1226`, inside `make_region_carrier`:

```
for (sid = 0; sid < a.shapes.size(); ++sid)        // shape, first-appearance order
    for (slot = 0; slot < slots; ++slot) {          // slot ascending
        out.push_back(leaf_id); put_uvar(payload.size()); out.insert(payload);
    }
```

and `tools/grotli_g3.cpp:591-599`, `make_raw_payload`:

```
for (const Bytes& tok : tokens) { put_uvar(tok.size()); out.insert(out.end(), tok.begin(), tok.end()); }
```

`slot.tokens` is filled in source-frame order (`tools/grotli_g3.cpp:1039-1053` region;
`SlotPlan` at `:892-906`). Therefore the emitted structured stream is exactly:

> **shape (first-appearance) → slot (ascending) → occurrence (source order) → token**

**[SRC]**

### 2.2 That is G5A's A3, definition for definition

`docs/I10-GROTLI-G5A-ORDERING-ATTRIBUTION-PREREG.md:205-216`:

```
A3 — SHAPE_COLUMN
  for shape in first-appearance order:
      for slot ascending:
          for occurrence in source order:
              emit chunk
```

Identical. **[SRC]**

### 2.3 Three consequences

**(a) My own interim checkpoint was wrong and I retract it.** That checkpoint
proposed a 32-selector *layout ladder* as the oracle-free decision axis, on the theory
that "the layout axis is cheap to plan because a layout is a permutation of a fixed
multiset over a bounded alphabet." True as a statement about G5A's carrier. **But the
frozen G3 carrier has no layout degree of freedom to plan** — the layout is fixed at
construction to the measured-optimal value. A ladder over G5A's 32 selectors measures
a decision variable that does not exist in `make_region_carrier`. Retracted.

**(b) The 89,960 B ordering prize is not on this carrier.** G5A's prize is the A1→A3
delta *within G5A's own envelope*, whose structured stream is per-scalar chunks each
prefixed `token_len` (`docs/I10-GROTLI-G5A-ORDERING-ATTRIBUTION-PREREG.md:131-135`).
G3's envelope groups per `(shape, slot)` with one leaf descriptor and one payload
length (`tools/grotli_g3.cpp:1212-1217`), and applies a typed leaf transform. Two
different carriers, two different framing granularities, two different complete-byte
accounting conventions (G5A charges a 1-byte out-of-band mode; G3 charges none, its
layout being fixed). **[SRC]**

> **Therefore the comparison "0.343% selector prize vs 6.12% ordering prize, so spend
> on the ordering" is a cross-carrier splice.** G5A's `89,960 B` and G3's `1,384,654 B`
> are not commensurable as *headroom*. The 89,960 B is an explanation of *why G3 wins*,
> not an unclaimed prize available to a planner. The adversarial audit's §5.3 kill
> argument rests on this splice, and so did my own checkpoint's §2.

**(c) The frozen planner's only decision surface is the per-`(shape, slot)` leaf ID.**
This is now a source-level fact, not an inference. It is the same surface Track 16's
critic identified (zero-new-wire cardinality gating reuses exactly that field), and it
is a **much smaller** surface than either of us was reasoning about.

### 2.4 What survives from G5A — stated precisely, so it is not overstated

G5A still earns two things, and neither is planner headroom:

1. **Causal attribution:** within a fixed carrier and a fixed multiset, column-major
   emission beats source-order emission by `89,960 B`, `COLUMN-DOMINANT` (M10, M11).
   This *explains* the G3 carrier design. It authorizes no new mechanism.
2. **A measured negative that constrains any planning claim:** the same effect
   **reversed** on held-out V1 (+1,721 B, M12) and G5B shows that *predicting* ordinal
   placement cheaply costs `+1.6507%` (M13). Layout is valuable when exact and harmful
   when cheaply predicted. A planner must not try to learn the layout — and per §2.3(c)
   it cannot, there being nothing to learn.

**"G5A-informed" is a lineage label, not a mechanism justification.** I state this
because my interim checkpoint leaned on it as one, and the doctrine-1 failure is real.

---

## 3. Where the adversarial audit is wrong (two refutations, same-window)

I audit `docs/swarm-2026-10-02/02-finalist-planner-fledge.md` on the same terms it
audited me. Two of its four grounds fail.

### 3.1 REFUTATION 1 — the "prize" is a sampled-pair statistic, not the selector's prize

The audit computes: `0.057144% × 6 × 1,384,654 ≈ 4,748 B`, calls it "the prize",
then derives "aggregate gate 0.25% ÷ 0.3429% = 0.729 → the gate is 72.9% of the
entire prize," and concludes the gate "cannot distinguish a good planner from a coin
flip" (audit §5.1-§5.2).

**The arithmetic is correct. The identification is not.** Q2's sampling design is frozen
at `docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md:1106-1107`: `max_pairs = C(4,2) = 6`
per file, `max_pairs_total = 24`. Each pair optimizes **two** slots while all other
slots stay at `O11` (`:1127-1134`). So the `4,748 B` is the total gain from optimizing
**24 pairs ≈ 48 slot-decisions**, out of a slot population that is unrecorded but, on
D3 with 11,228 structured frames (M2), certainly orders of magnitude larger.

That is a **sampled-pair headroom density**. Extrapolating it to the whole population
requires an assumption the audit never makes and the data cannot support: that
per-slot headroom is uniform across slots. If it is, the total population prize is
`4,748 × (n_slots / 48)`, i.e. **larger**, not equal to 4,748 B. If headroom is
concentrated in the 48 sampled slots, it is ~4,748 B. **The two readings differ by
orders of magnitude and the measurement does not discriminate between them.**

The audit's own proposed fix concedes this: it defines
`Ceil_isolated(f) = (Σ_slots max_L o11 − Σ_slots min_L o11) / C_ref(f)` and states it
"requires zero additional backend calls" (audit §5.2 item 4, §11 R0.1). That is the
correct quantity. **The audit then labels §5.1's `4,748 B` as though it were the same
quantity, and builds the "gate is 72.9% of the prize" verdict on the substitution.**

> **Verdict on the audit's headline:** *"the aggregate gate is 72.9% of the entire
> prize"* is **not established**. It rests on reading a 24-pair sample as the
> population. The `0.25%` gate may well be mis-scaled — that is precisely what R1
> measures at zero backend cost — but the audit has not shown it.

### 3.2 REFUTATION 2 — the per-file gate *is* attainable in principle, trivially

The audit asserts: *"§16.1's per-file leg is not attainable in principle. The gate is
1.80× the largest single-file gain ever measured"* (audit §5.2 item 2). Comparing a
**per-file aggregate** gate to a **single pair's** maximum is a category error.

`docs/I10-G5-PLANNER-PIVOT-PREREG.md:739-741` defines:

```
C_ref(f)  = min_F C(f, O11, F)
P0_O11_regret(f) = (C_P0(f) - C_ref(f)) / C_ref(f)
```

The gate is **regret against the O11 reference**. A selector that exactly reproduces
O11's leaf choices has regret **identically zero** on every file, aggregate and
per-file. So `0.50%` is not merely attainable — it is attainable by the oracle itself,
trivially. The question the gate actually poses is whether a *bounded-budget* selector
can *match* an oracle within 0.50%, not whether 0.50% exceeds anything.

The audit's real concern is valid but differently stated, and it is the concern R1
answers:

> The gate does not ask "can you match O11?" It asks "can you match O11 **without
> paying O11's cost**?" A gate that is trivially satisfiable by the oracle is a gate
> whose difficulty is entirely in its *cost* side, and the cost side is exactly where
> the pilot's accounting is broken (M7).

### 3.3 What survives of the audit

The audit's §5.2 item 1 — that an uninformative-but-uncorrelated selector is expected
to lose roughly half the prize and could land under `0.25%` — **does** survive, and it
is the strongest surviving form of the mis-scaling charge. It is an expectation about a
random predictor, not a measurement, and R1's B2/B3 arms (§7) turn it into a
measurement. Recorded as **partially conceded**.

---

## 4. Where the adversarial audit is right (two concessions, both fatal to the frozen contract)

### 4.1 CONCEDED 1 — the speed baseline is invalid, and this is dispositive

Three independent confirmations, all mine and the audit's:

- M7: G4's speed column was **withdrawn** because the O11 denominator read cached
  ranking labels while q11 label construction was timed separately.
- `docs/I10-G5-PLANNER-PIVOT-PREREG.md:820-821` puts O11's isolated q11 scoring
  **inside** `T_plan(O11)` — the correct intent.
- But the prereg has **no machine gate** forcing that label build to be re-timed
  in-job, and `§14.3`'s counter list (`:921-944`) has `o11_q11_candidate_calls` but no
  `o11_label_build_ms`. G4's defect is guarded in prose only.

**Independent reinforcement from source:** `tools/grotli_g4_planner.cpp:486-492`
(`count_oracle_leaf_calls`) counts the frozen G3 per-candidate q11 calls, and
`:634` shows the production path zeroing `isolated_brotli_bytes` with the comment
*"score-free by construction"* — i.e. the frozen source already distinguishes the
oracle label from the production score. **The correct instrumentation exists in the
frozen source and the prereg simply does not require it.** That makes the fix cheap and
the omission clearly a contract defect rather than an impossibility.

**Consequence.** The pilot's `speedup_plan ≥ 5.0` compares against a baseline the
project has itself declared *"a research oracle … not a viable production planner"*
(`docs/I10-GROTLI-G4-RESULTS.md:55-56`, echoed at pivot prereg `:689-697`). A ratio
against a deliberately unbounded oracle is unbounded above by construction. This alone
justifies HOLD; the other concessions are independent.

### 4.2 CONCEDED 2 — the feature basis and the label are both misaligned, and only one is repairable

**(a) Arrangement-invariance.** All eight features (M16) are functions of `o(c)` alone.
By §2.1, `o(c)` is a per-slot column. So the features are *already* column-conditioned
and **invariant to any reordering of the carrier**. Conceded fully — but note the
stronger consequence I did not previously state: per §2.3(c) there is no ordering
variable in this carrier, so this is not a defect to fix. It is a **description of the
representation's actual decision surface**, and it is correct.

**(b) Target misalignment — the deeper defect.** The label is isolated q11 on `o(c)`
(M17); the objective is complete-carrier bytes (`:739-748`). Two distinct causes, both
source-verifiable:

- `o(c) = leaf_id ‖ occurrences ‖ payload_len ‖ payload` (`tools/grotli_g3.cpp:743-750`)
  is a *standalone* Brotli stream. It pays its own meta-block header and builds its
  own model over ~256 candidates' statistics. In the carrier, all slots share one
  stream and one model-update history.
- Every D1–D4 file is `≤ 10,485,760 B < 2³⁰`, so `lgwin 30` is **non-binding** for all
  four. The isolated-vs-carrier difference is therefore **purely shared-model-state**,
  which is precisely what an isolated-object label cannot represent. **[SRC]**

> **The pivot changes the estimator's function class and keeps the misaligned target
> verbatim.** G4's measured failure was fidelity; G4 never established that fidelity
> failure was a *function-class* failure — its Q2 probe argued *against* the
> "needs joint combinatorics" story (M8), and its speed column was withdrawn (M7). So
> the pivot's central bet is unevidenced. Conceded.

**(c) The 8,192 B prefix bias (M19).** For any `len(o(c)) > 8192`, the residual in
prereg `:476-486` is `log2(q4_sample_bytes) − log2(exp2(p0_predicted_object_bytes))`:
a **truncated** numerator against a **full-object** denominator, then applied to
full-object scores via the frozen shrinkage (`:513-527`). Strata S2/S3 (4,097 B and
65,536 B boundaries, `:437-444`) — the strata carrying the largest byte counts —
receive a systematically biased correction, biased toward whichever leaf compresses
well on its own prefix. This is my own finding, independently reached, and it is a
correctness-of-measurement defect, not a tuning concern. Conceded.

---

## 5. Mechanism — what R1 actually decides

The frozen planner tries to *replace* an oracle's ranking. R1 instead asks the
question that determines whether replacement is worth anything at all:

> **How far is the cheapest possible zero-ranking-call selector from the O11 reference,
> and how far is O11 from raw Brotli?**

If the cheapest zero-oracle baseline is already within the fidelity gate of O11, then
the entire planner is unnecessary — ship the baseline. If it is far, a bounded selector
has real work. **Neither question requires a fitted model, an 8,192 B sampling scheme,
a UCB allocator, or a ladder.** It requires ~4 whole-carrier encodes per file.

### 5.1 The zero-oracle baseline family (exact, `O(n)`, zero backend calls)

All four are pure functions of quantities frozen G3 **already computes and already
transmits** (`tools/grotli_g3.cpp:892-906`; the `dictionary_cardinality_ratio`
diagnostic is emitted at `:1862-1870`):

| Arm | Rule | New decoder-visible bytes |
|---|---|---|
| **B0** | every eligible slot → `RAW_LEX` | **0** |
| **B1** | slot → `EXACT_DICT` iff `distinct_tokens / occurrences ≤ τ` | **0** (reuses G3's existing per-slot leaf-ID byte) |
| **B2** | B1 **and** integer leaves enabled iff `canonical_int` / `delta_eligible` / `dod_eligible` (all already frozen fields) | **0** |
| **B3** | per-`REGION_MIXED` **full-O11 mask replayed verbatim** | **0** — this is G3's own carrier, and the identity gate |

`τ ∈ {0.05, 0.10, 0.20, 0.35, 0.50, 0.75, 1.00}` frozen before dispatch (7 constants).
Note B3 is the identity comparator: B3's bytes must equal frozen G3 exactly, which is a
free correctness gate. B1/B2 are adopt-class (§8) and are claimed as such.

`B0..B2` cost **zero** backend calls to *construct*. Measuring their complete bytes
costs one q11 encode each. `Ceil_isolated` costs **zero** — it is read off the O11
label table G3 already builds (`tools/grotli_g3.cpp:1044-1049`).

### 5.2 Encoder + decoder state machine

```
R1 encoder (per file)
  A0  materialise frozen G3 analysis (parse, shapes, slots, candidates)   [frozen]
  A1  read o11_isolated label per (slot,leaf)                            [already built]
  A2  compute Ceil_isolated(f) = (Σ_slots max_L o11 − Σ_slots min_L o11) / C_ref(f)
                                                → 0 additional backend calls
  A3  build carriers B0, B1(τ)…B1(7τ), B2 under REGION_MIXED            → 0 calls
  A4  build the O11 REGION_MIXED carrier B3                              → 0 calls
  A5  encode {B0..B2(τ), B3, RAW_BROTLI} at q11/lgwin30, once each
  A6  emit complete bytes, counters, and the gates

R1 decoder
  UNCHANGED. B0..B2 emit exactly the frozen G3 carrier with different per-slot
  leaf IDs already present on the wire (tools/grotli_g3.cpp:1213). The frozen
  decode_region_carrier (:1260-1325) is used verbatim, unmodified, for every arm.
```

**Decoder delta: exactly 0 bytes, exactly 0 new state, exactly 0 new code.** Not an
estimate — R1 adds no format at all. Every arm is a *different selection* over an
existing wire field. This is inherited, verified, and it is the pilot's one genuinely
rare property (pivot prereg `:1034`).

---

## 6. Complete cost model

### 6.1 Bytes — charged, complete, no allowances

| Component | R1 | Pivot pilot |
|---|---|---|
| selector / mode byte | **0** — every arm is the frozen G3 carrier | 0 (§14.1 counts no new byte) |
| per-slot leaf ID | **0 new** — `tools/grotli_g3.cpp:1213` already writes it | 0 |
| `τ` threshold | **0 B** — a frozen algorithm constant, `G5P`-style compile-time | 0 |
| backend output | all q11 bytes, same build identity per arm | same |
| **new archive bytes from the planner** | **0** | **0** |

The one place the pilot spends archive bytes that R1 does not is **§7.2's `8192 B`
prefix-capped sampling**, which is not a wire cost but a *measurement-validity* cost
(§4.2c). R1 eliminates it by never sampling.

### 6.2 Cycles / time — projection, premises named, same window only

Using **only** [LOCAL-I9] M22 (`q1 = 432.106 MB/s`, `q11 = 0.681 MB/s`), never
splicing windows:

| Arm | encodes/file | D3 (10,485,760 B) at `q11` [P] |
|---|---:|---:|
| B0, B1×7τ, B2, B3 | 10 | `10 × 10,485,760 / 0.681e6` ≈ **154,000 ms** |
| `RAW_BROTLI` | 1 | ≈ **15,400 ms** |
| O11 reference (M4) | 0 (frozen identity) | — |
| `Ceil_isolated` | **0** | **0 ms** |

R1 total ≈ **169 s on D3**, ≈ **20 s** across D1–D4. The pivot's frozen budget is
80 isolated q4 + up to 8 q1 + 4 q4 + 2 q6 + 2 q11 + 1 raw q11 + 1 G2 q11 per file;
the same [P] arithmetic puts its q11-class portion alone at `4 × 15,400 ≈ 61.6 s`
**per file**, i.e. ≈ **250 s across D1–D4 before any q4/q6 term**. **[P]**

> **[P] Caveat, stated because the project has been burned by exactly this:** M22 is a
> *Class A aggregate* rate on a different corpus mix, not a G3-carrier rate. The
> *ratio* R1:pilot is the reportable quantity; the absolute milliseconds are not. If the
> control CV exceeds the frozen limit the verdict is `TIMING_BLOCKED`, not FAIL — the
> bytes verdict stands independently (`RESEARCH_LEDGER.md:4800-4803` precedent).

**Decoder cycles: 0, exactly.** R1 introduces no decoder change to time.

### 6.3 Memory / RSS

R1's arms are ordinary G3 carriers, so peak RSS is whatever frozen G3's carrier
materialization costs — identical across arms, and **matched to the O11 baseline by
construction** because B3 *is* the O11 carrier. This structurally discharges the
audit's §8.2 objection (unmatched RSS denominator) that applies to the pivot, whose
`peak_rss(P1) ≤ peak_rss(O11)` gate compares a feature-sweep process against a
baseline whose object-residency policy is unpinned (prereg `:1019-1024`).

### 6.4 The one quantity the pivot leaves unbounded — and R1 does not need

The pivot's features are `Θ(N)` per candidate over `o(c)` (§4.3/§4.4/§4.5), so the
swept volume is `F = Σ_c len(o(c))`. Source inspection: `o(c)` is a whole slot column
(`tools/grotli_g3.cpp:743-750` + `:591-599`), so `F ≈ Σ_slots column_bytes + headers`
— i.e. `F` is **comparable to `N_src`**, not `≫ N_src` as the audit's `[A]` guessed
from a 5 KB-mean-object assumption. The audit's `9.5× N_src` projection is therefore
**not supported**; the real risk is different and smaller: `order1_bits` allocates a
`std::map<pair<uint16_t,uint8_t>,uint64_t>` **per candidate** and
`bounded_match_coverage` a `std::unordered_map<uint32_t, vector<size_t>>` holding
**every** 4-byte-hash position (`tools/grotli_g5_planner_pivot.cpp:467-504`) — an
allocation pattern that is `Θ(F log F)` with `vector` churn, and whose RSS is gated by
nothing and pinned by nothing (prereg §4.5 says only *"a single 32-bit hash table"*,
size unspecified).

Recorded as a **real but bounded** defect, not the audit's overstated one. R1 has no
feature sweep, so it is moot.

---

## 7. R1 — the decisive remote-only pre-flight

Derived in this report, **before** any new pilot result exists. Nothing below is
reactive to an outcome.

### 7.1 Contract

- **Remote only** (GitHub Actions, `ubuntu-24.04`, Clang 18, `workflow_dispatch`).
- Compile against the **materialized** frozen G3 blob `1a3d18fed76adb6fb33264e1994f9c357306b3fa`; verify blob + SHA-256 in-job.
- Backend: `BrotliEncoderVersion()` recorded per row; require `16781312` / `1.1.0` and the pinned library SHA-256 set from `docs/I10-G5A-LOCALITY-CONTROLS-PREREG.md:120-133`. Mismatch ⇒ `INVALID-R1`.
- **D1–D4 only.** V1 **not fetched**; no fetch token in the discovery job. M23: no held-out corpus exists, so V1 is `KNOWN-STRESS` anatomy only.
- **≤ 20 whole-carrier q11 encodes total** (10 on D3, 3 each on D1/D2/D4), **0 isolated calls**.
- No production edit. No timing claim in the primary arm.
- Every carrier round-trips byte-for-byte **before** any byte is reported; all frozen
  G3 adversarial decoder self-tests pass **unchanged**; `B3` reproduces the recorded
  frozen G3 carrier SHA-256 (identity gate, 4/4).

### 7.2 Pre-registered measurements and thresholds

Let `C_ref(f) = C(f, O11, REGION_MIXED)` (the B3 identity arm), and
`Ceil_iso(f) = (Σ_slots max_L o11 − Σ_slots min_L o11) / C_ref(f)` **[0 calls]**.

```
G0  IDENTITY
    B3 carrier SHA-256 == frozen G3 recorded hash, 4/4 files
      else → INVALID-R1. No byte is interpretable.

G1  HEADROOM-SCALE                       [0 calls]
    PASS  if  median_f Ceil_iso(f) >= 0.0025   (the frozen aggregate gate)
    FAIL  if  median_f Ceil_iso(f) <  0.0010   → NO-GO-R1-GATE-UNSCALED
    OPEN  otherwise  (0.0010 .. 0.0025)  → gates are unresolvable at this
            population size; escalate to R2 (see §7.4). Do NOT run the pivot.

G2  ZERO-ORACLE-GAP                      [≤ 18 encodes]
    gap(f) = C(f, best of B0..B2, τ*) / C_ref(f)  −  1        τ* = argmin over τ
    PASS  if  median_f gap(f) <= 0.0025   AND  worst_f gap(f) <= 0.0050
          ⇒ a ZERO-RANKING-CALL selector already meets the pivot's own fidelity
            gates. The planner is NOT needed. Ship B2. → NO-GO-PILOT-BY-DOMINANCE
    FAIL  if  median_f gap(f) >= 0.0100
          ⇒ ≥1% aggregate is provably recoverable with zero ranking calls, and
            G2's measured 1.5388% worst-file proxy error (M5) is the entire gap.
            This is real, cheap, planner-shaped headroom. → R2 AUTHORIZED

G3  FAMILY-SIGN                          [0 calls]
    For each family f, report sign(gap(f)).
    BLOCKING RULE, frozen: if any single family has gap(f) > 0 while the median
    is <= 0.0025, the zero-oracle baseline is population-dependent (the R-B regime).
    → classify POPULATION-DEPENDENT; adoption requires a held-out family that does
      not exist (M23), so the outcome is HOLD-ENGINEERING, never PROMOTE.

G4  COST SANITY                          [diagnostic, no gate]
    Report sum_object_bytes(f), n_candidates_by_stratum(f), n_slots(f),
    distinct-leaf-choice count per file. These are the counts the pivot prereg
    never reports (:921-944) and they are what G1/G2 are interpreted against.
```

### 7.3 Reading table

| G1 | G2 | G3 | Ruling | Disposition |
|---|---|---|---|---|
| FAIL | — | — | `NO-GO-R1-GATE-UNSCALED` | Pivot contract is unsound at this population. Re-freeze gates prize-relatively **or** abandon. Does **not** kill the idea. |
| OPEN | — | — | `R1-GATE-UNRESOLVABLE` | Escalate to R2 before spending on the pivot. |
| PASS | PASS | clean | `NO-GO-PILOT-BY-DOMINANCE` | **Zero-oracle selector meets the pivot's gates.** Ship B2 as adopt-class engineering. The pivot is unnecessary. |
| PASS | PASS | POP-DEP | `HOLD-ENGINEERING` | B2 works on 3/4 and fails 1/4. No held-out set exists. Document, do not adopt, do not promote. |
| PASS | FAIL | any | `R2-AUTHORIZED` | Headroom is real and cheap-adjacent. A bounded planner is justified — but only *after* R2. |
| — | INVALID | — | `INVALID-R1` | Correctness/identity failure. Fix, re-freeze, rerun. No tuning on D1–D4. |

### 7.4 R2, named now so the escalation is not a post-hoc escape

If G1 is OPEN, the frozen question is *"is the population's selector prize resolvable
at all?"* R2 is: extend R1's zero-call measurement to a **newly frozen** structured
family (none exists today, M23 — so R2 is currently **unrunnable**, which is itself the
answer). Recording this prevents "escalate" from becoming a loophole.

### 7.5 What R1 does **not** claim

- No speedup. No timing claim in the primary arm.
- No novelty. B0–B2 are adopt-class (§8).
- No held-out generalization. D1–D4 are spent (`:170-190`).
- No Pareto claim. M25: the frontier is `0 FRONT-CROSSING`; nothing here touches it.

---

## 8. Novelty and prior-art risk

### 8.1 Adopt-class — stated, not hidden

| Element | Prior art | Verdict |
|---|---|---|
| per-slot cardinality-gated dictionary selection (B1/B2) | **FastLanes** (Parquet/ORC/ClickHouse), **BtrBlocks**, **ALP**, Parquet dictionary encoding | **Adopt-class. Not novel.** |
| analytic cost model to rank parse options | **zstd `optPrice`/`Z_entropyCost` optimal parser**, **LZMA SDK `Price` tables**, 7-Zip optimal-parser price model | **Adopt-class.** The pivot's 8-feature ridge is a *learned* price table. |
| q1→q4→q6→q11 ladder, keep-4/keep-2/keep-2 | **Successive halving** (Karnin et al. 2016), **Hyperband** (Li et al. 2016) | **Adopt-class.** |
| bounded multi-setting portfolio + ship smallest + one selector byte | paq8/cmix method tournament, 7-Zip `-m0..-m9`, xz `-9e`, Brotli's own q0–q11 | **Adopt-class.** |
| racing allocation of a bounded eval budget | Hyperband / BOHB racing; codec parameter search | **Adopt-class.** |
| min-portfolio with raw always eligible | G3 §11, measured on V1 (M3) | **Adopt-class, inherited.** |
| everything else in the pivot | — | **nothing identified** |

**Therefore: the bounded finalist planner is adopt-class end to end and cannot carry a
mechanism-novelty claim or be the vehicle for a Pareto crossing.** This is the audit's
§7 ruling and I concur with it without reservation. MASTER-BRIEF item 15 supplies the
only legitimate framing: *"adopt-class prior art with real engineering value."* That
framing yields **cost gates**, not fidelity gates — which is exactly the contract
defect of §4.1.

### 8.2 What, if anything, is claimable

Nothing in this report claims novelty. If Track 20 wants a candidate, the only locus
with any defensible content is a *negative* result with standing: **"per-slot
transform selection on the G3 carrier is bounded by a zero-call statistic, and the
measured ordering prize that motivates 'G5A-informed' planning is already realized by
the carrier's fixed layout (§2)."** That is a **do-not-reburn entry**, not a novelty
claim, and it is worth more to the project than a mechanism would have been.

---

## 9. Adversarial failure cases

1. **§2.3(b) is itself wrong** — G3's emission order is not A3-equivalent because a
   shape's `parts`/template bytes are interleaved differently, or because
   `make_dict_payload` reorders values. **This is the single failure that would flip the
   verdict.** Verified at `tools/grotli_g3.cpp:601-...` (`make_dict_payload` builds
   `dict` then IDs — but the *emitted* order is still the slot's occurrence order) and
   `:1203-1226`. R1's G0 identity gate would surface any residual divergence.
   **Falsification test:** emit a G3 `REGION_MIXED` carrier and a G5A A3 carrier over the
   same file and compare the structured-stream bytes. If they differ, §2.3 collapses and
   the ordering prize returns to the table. This check costs **one encode**.
2. **`Ceil_iso` is small because O11 is already near-optimal.** Then G2 `FAIL`s and R2 is
   authorized — correctly. The pivot is not the answer; §7.4 says what is.
3. **`Ceil_iso` is large but `gap` is also large.** Correct outcome: real headroom,
   cheap oracle. G2 `FAIL`, R2 authorized.
4. **`gap` passes on 3/4 and fails on 1/4** (G3). This is M3's shape recurring. Adoption
   is blocked by M23 regardless.
5. **`τ` was chosen on D1–D4.** It is one scalar fit on spent data, same status as the
   pivot's ridge coefficients. R1 reports **all 7 τ values**, not just the argmin, so the
   curve is visible and no τ-selection is hidden. Still disclosed as spend.
6. **Identity gate fails** because M21's single-sourced G3 digest is stale. Then R1 is
   `INVALID-R1`, and the *correct* first action is to recompute the G3 binary digest
   from the pinned blob — M21 is a known, cheap, unresolved provenance defect that
   should be closed **before** R1 regardless of outcome.
7. **Brotli/backend mismatch** ⇒ `INVALID-R1-BACKEND`, never reinterpreted.
8. **D3 dominates the aggregate** (M2: ~88%). A D3-only result must be labelled as such
   and must not be reported as breadth.
9. **Control CV too high** ⇒ `TIMING_BLOCKED` on any timing field, bytes stand alone.
10. **Someone runs the pivot anyway.** The `≤20 encodes` R1 is ~1/12 the pivot's q11-class
    cost **[P]**; running the pilot first spends the larger budget to answer a question
    R1 answers for less.

---

## 10. Minimum prototype

**None built, deliberately.** R1 requires **zero new codec code**: it is a driver over
`tools/grotli_g3.cpp` (already existing, frozen) reusing `analyze_regions`,
`build_structured_candidates`, `make_region_carrier`, `decode_region_carrier`, and the
`distinct_tokens` / `canonical_int` / `delta_eligible` / `dod_eligible` fields already
computed at `tools/grotli_g3.cpp:1039-1053`. The only new code is the B0/B1/B2
selection mask, the τ sweep, `Ceil_iso`, the JSON emitter, and `schema-check`.

If built, it goes **only** under
`prototypes/swarm-2026-10-02/02-finalist-planner/space-bunny/`, is not wired to
production, and is not committed by this lane.

This is also the disposition I recommend over the pivot's own minimum prototype
requirement (prereg §21's four-command CLI over `tools/grotli_g5_planner_pivot.cpp`):
the existing pilot source is 1921 unwired lines with no frozen workflow (M14, M15) and
its decisive experiment has not been run. **Build the falsifier before the artifact.**

---

## 11. Reconciliation with Track 16 (CDR / EXP-C / EXP-R)

Answering the coordinator's question directly: **Track 16's EXP-C is a prerequisite
diagnostic for my track, and it is partially redundant with R1. It is not orthogonal.**

**Convergence.** Both lanes independently arrived at the same structural fact from
different directions: Track 16's critic found that the frozen G3 carrier **already
transmits a leaf ID per `(shape, slot)`**, so any per-slot selection mechanism costs
**zero new decoder-visible bytes**; I found the same field is the carrier's **only**
decision surface (§2.3c). Track 16's `EXP-C` (cardinality-gated `EXACT_DICT`) is
**exactly my arm B1**. That is independent convergence on the decisive fact, and it is
the strongest reason to believe the §2 reading is correct.

**Relationship.**

- **EXP-C ≡ R1's B1/B2.** Same mechanism, same zero-wire property, same
  `distinct_tokens/occurrences` statistic (already emitted by frozen G3 at
  `tools/grotli_g3.cpp:1862-1870`). **Redundant in mechanism.**
- **EXP-C is a prerequisite in ordering.** Track 16's framing is correct: *do not spend
  CI budget estimating a decision surface if an exact `O(n)` slot statistic can first
  prove there is no headroom.* EXP-C's construction is `O(n)` and zero-call; its
  *measurement* is 1 encode per τ. R1 is EXP-C's measurement plus the two decisive
  additions: **`Ceil_iso`** (zero calls, resolves the audit-vs-me gate-mis-scaling
  dispute in §3.1) and the **zero-oracle gap** `gap(f)` (resolves whether the pivot is
  needed *at all*, which EXP-C never asks).
- **CDR as specified: I concur it should not be built**, on Track 16's identifiability
  argument (a probe returns one scalar; `Θ(S)` parameters are not identifiable from
  `O(1)` probes) and on the G5B precedent (exact placement wins, cheap ordinal
  prediction loses `+1.6507%`, M13). I add one independent reason: per §2.3(c) there is
  no placement variable in this carrier for a position table to model.
- **EXP-R's `KILL-ROUTING` is a weaker version of R1's G2.** `KILL-ROUTING` fires when
  best-achievable per-slot substitution beats O11 by `< 0.25%` on 3/4 files. R1's G2
  measures the same surface without needing per-slot substitution probes, because it
  measures **cheap policies' complete bytes directly** rather than searching for the
  optimum. Same falsifier, ~1/6 the encodes.

**Staged ordering and shared controls — no double-charged decision surface.**

```
STAGE 1  (Track 16 EXP-C / R1 arms B0,B1,B2)     ≤ 18 encodes   ~0 min extra
           Ceil_iso (0 calls) + gap(f) + τ curve
           → answers: does a zero-oracle selector meet the pivot's gates?
STAGE 2  (only if G2 FAIL)  a bounded per-slot planner
           → justified by a MEASURED gap, not by a modelled one
STAGE 3  (only if STAGE 2 passes on a NEWLY frozen family)
           → held-out validation. UNREACHABLE today: no held-out corpus exists (M23).
```

Shared controls, stated once and used by both lanes: the materialized frozen-G3 blob
identity, the frozen D1–D4 corpus identities, the pinned Brotli library hashes, the
`B3`-equals-frozen-G3 identity gate, and the frozen counter set. **Track 16's EXP-R
`KILL-ROUTING` should be dropped in favour of R1's G2** — it is strictly more expensive
for the same falsifier, and running both would spend the same decision surface twice,
which the coordinator's budget constraint correctly forbids.

---

## 12. Final recommendation

> # **HOLD** — the frozen `I10-G5-PLANNER-PIVOT` contract, on four independent grounds:
> its speed baseline is a research oracle the project has itself declared unusable, with
> the exact defect that voided G4's speed column guarded only in prose (§4.1); its
> feature sweep has an unbounded, ungated volume with an unpinned allocation pattern
> (§6.4); its 8,192 B sampling biases precisely the strata that carry the bytes (§4.2c);
> and its stated mechanism justification does not exist on this carrier (§2).
>
> **Adopt R1** (§7): a ≤20-encode, zero-isolated-call, byte-only, GitHub-Actions
> pre-flight that resolves the gate-mis-scaling dispute from **measurement** rather than
> from a 24-pair extrapolation, and that answers the question the frozen contract never
> asked — *is a planner needed at all, given a zero-ranking-call selector?*
>
> **Unify with Track 16:** EXP-C ≡ R1's B1/B2; run EXP-R's `KILL-ROUTING` **not** in
> parallel. Stage 1 is shared; Stage 2 is authorized only by a measured gap; Stage 3 is
> unreachable until a held-out family exists.
>
> **Close M21** (the single-sourced G3 binary digest) before dispatching anything.

**Why HOLD and not the alternatives.**

- **Not KILL.** The engineering value is real; the decoder delta is genuinely zero; and
  §3 shows the audit's own headline arithmetic does not establish that the gate is
  unscalable. What is dead is the *contract*, not the *idea*. KILL would also forfeit
  the possibility that G2 returns `PASS` and the whole planner becomes unnecessary —
  which would be the best possible outcome for the project.
- **Not PILOT.** The frozen pilot has never been run (M14), its source is unwired
  (M15), and the four defects above are contract-level: they require a **new frozen
  revision**, not a dispatch. Dispatching r1 as-is would spend the larger budget on the
  less informative question (§9.10).
- **Not PROMOTE-TO-REMOTE.** M23: no held-out structured corpus exists. M25: the
  frontier is `0 FRONT-CROSSING` and nothing in this lane touches it. §8: adopt-class
  end to end, so it can never be the vehicle for a crossing. And no novelty claim
  survives §2.3(c).

**What converts HOLD → PILOT:** R1 returns `G2 FAIL` (real, cheap, planner-shaped
headroom, measured not modelled) **and** `G1 PASS` (the gate is scalable at this
population). **What converts HOLD → KILL:** R1 returns `G2 PASS` with `G3` clean —
`NO-GO-PILOT-BY-DOMINANCE`, a zero-ranking-call selector meeting the pivot's own gates,
which retires the planner as unnecessary rather than merely failed.

**The most valuable outcome of this lane is a negative one**, and I say so plainly: if
the ordering prize motivating "G5A-informed planning" is already realized by the
carrier's fixed layout (§2), then the correct project action is to stop planning leaves
and spend the budget on representation — which is exactly what the do-not-reburn
register, the G5B verdict, and Track 16 all independently point to.

---

## 13. Open uncertainties that could change this verdict

1. **§2.3 is the whole report.** One encode comparing a G3 `REGION_MIXED` structured
   stream against a G5A A3 stream on the same file either confirms or refutes it. If it
   refutes, the 6.12% ordering prize returns to the table, the audit's §5.3 argument is
   rehabilitated, and the right move becomes representation work — still not the pivot.
2. **Is `Ceil_iso` ≥ the gate?** Unmeasured, **zero** backend calls to find out. This is
   the cleanest open question in the track.
3. **M21's G3 digest.** Single-sourced, never independently recomputed. Cheap to close,
   should be closed first.
4. **Slot population per file.** Never recorded (G4 reports slot counts for some arms;
   §3.1 needs `n_slots`). Determines whether §3.1's sampling objection is large or
   small — and it is free.
5. **Is `gap(f)`'s τ curve flat or sharp?** A sharp optimum would mean the statistic
   carries real signal (interesting); a flat curve means cardinality is nearly
   irrelevant and the whole routing direction is dead. R1 reports all 7 τ, so this is
   answered by construction.