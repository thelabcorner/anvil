# Track 12 — Encoder-only search over reversible representations

**Agent:** Space Bunny Free (constructive inventor)
**Swarm:** `docs/swarm-2026-10-02`, track `12-encoder-search`
**Date:** 2026-10-02
**Status:** FINAL. **The mechanism's core exactness claim is WITHDRAWN** (see §1). What survives is a structural negative plus one provable byte bound.
**Paths:** this report; isolated sketch at `prototypes/swarm-2026-02/…` — see §12 for the exact path and status.

---

## 0. Correction notice (read before anything else)

An earlier draft of this report claimed that the encoder could prune representation candidates using an **admissible order-0 entropy bound** and thereby compute the **exact** argmin cheaply:

```
LB(f,s) := framing_min(f,s) + 1 + ceil( H0(payload_f(s)) )      ← WITHDRAWN
cost_Brotli(f,s) ≥ LB(f,s)                                      ← FALSE
```

**This is false for the frozen backend.** The frozen backend is `Brotli_q11_lgwin30` (`docs/I10-G5A-LOCALITY-CONTROLS-PREREG.md` §4.1), whose format contains copy commands over previously reconstructed output and a static dictionary (RFC 7932). Neither is memoryless order-0. A backend that exploits sequence correlation can and does emit fewer bits than the order-0 entropy of the same payload.

With that inequality void, the prune can discard the actual winner, **so PRA-1 is not exact**, and the honest verdict changes from PILOT to **HOLD** (§11). The two survivors — a structural theorem about the frozen backend, and a fully-charged selector-framing byte bound — are stated in §2 and §6 respectively.

Nothing else in this report depends on the withdrawn claim.

---

## 1. The disproof, stated as a counterexample

**Counterexample family.** Let `q` be `m` pseudo-random bytes drawn uniformly from a 256-symbol alphabet, and let `p = q ‖ q` (length `2m`).

| quantity | value | basis |
|---|---|---|
| `H0(p)` (ideal memoryless order-0 length) | `2m · log2 256 = 16m` bits `= 2m` bytes | exact arithmetic — every byte value appears exactly twice, so each symbol costs exactly 8 bits |
| `Brotli_q11(p)` | `≈ 8m` bits `+ O(log m)` = `≈ m` bytes `+ O(1)` | **projection**: literals for `q` at `≈ 8 bits` each, then **one copy command** (length `m`, distance `m`) costing `O(log m)` bits |

So `Brotli_q11(p) ≈ m` bytes while `H0(p) = 2m` bytes: **the claimed bound is violated by a factor of ≈ 2**, for every `m`.

**Why the exact Brotli figure is a projection but the violation is not.** I did not run Brotli and claim no measurement. But the *existence* of the violation follows from the format specification alone: RFC 7932 provides copy commands over the reconstructed history and a static dictionary, so `Brotli(p) ≤ m` bytes `+ O(1)` for this family by construction, while `H0(p) = 2m` bytes by arithmetic. The bound is therefore inadmissible **independently of any timing or byte measurement**, and no measurement can rescue it.

**Consequence, stated without hedging.**

1. `ENC_2` prune soundness is void ⇒ the emitted argmin is **not** the exact argmin.
2. PRA-1 must not be described as exact, optimal, or oracle-equivalent anywhere.
3. `A_greedy_h0`, the "bound-only, zero backend calls" control arm from the earlier draft, is withdrawn: it optimizes a non-admissible surrogate.

---

## 2. What survives: a structural theorem about the frozen backend

> **[SUPERSEDED IN PART BY ADDENDUM B, 2026-10-02.]** The "Theorem N" stated in this section was an **over-claim**: it asserted an impossibility result, but the proof is a counterexample against one bound family (unconditional payload frequencies). Addendum B replaces it with **Proposition P1** (admissibility failure, proved) and **Finding P2** (status of exact methods, descriptive), and lists what is **not** ruled out — backend-aware bounds, shared/incremental evaluation, exact dominance certificates, candidate-family structural bounds, and exactly-sound memoization of byte-identical candidates. Read this section as the historical claim, not as a standing result.

> **Theorem N (no encoder-computable nontrivial admissible bound).**
> Let `B` be a lossless backend whose decoder maintains (i) backward references over previously reconstructed output, (ii) a static dictionary, and (iii) adaptive context modelling. Let `LB` be any function computed by the encoder from the payload alone. If `LB` is *not* derived from `B`'s own model class, then `LB` is **not** a valid lower bound on `B(x)` for all payloads `x`: there exists a payload family on which `LB` exceeds `B(x)` by an unbounded factor. Consequently the only universally valid one-sided bounds on `B(x)` computable from `x` alone are those obtained by assuming a model class strictly narrower than `B`'s — and any such bound excludes, by construction, the possibility that the true minimum lies outside that class.

**Proof sketch.**
(a) *Non-admissibility.* §1 exhibits the family `p = q‖q` for which `H0(p) = 2|B(q)|`-scale while `B(p) ≈ |B(q)|`. The same construction applies to any `LB` that only counts symbol frequencies: repeating any payload strictly decreases the backend's cost while leaving the frequency-derived quantity at full scale. Multiply the repeat count `k` and the gap grows like `k`.
(b) *Dictionary.* Any `LB` ignoring the static dictionary is exceeded by any payload containing a dictionary word sequence.
(c) *Only tight bound.* The information-theoretically tight lower bound is `H(payload | decoder state)`, where `decoder state` includes the entire reconstructed history and the chosen context models. Computing that quantity requires the model the backend itself builds — i.e. it *is* the objective. There is no cheaper route.
(d) *The trichotomy.* Any method is one of: **exact**, which requires evaluating `B` on every candidate (full enumeration); **cheap**, which requires a bound that Theorem N says cannot exist; or **surrogate**, which approximates. There is no fourth option that is both exact and cheap. ∎

### 2.1 What this settles about the project's own record (all MEASURED, cited)

- **G4's `NO-GO` is now explained, not just observed.** `RESEARCH_LEDGER.md` PART XV §G4 records worst-file regret `6.6056% / 7.0802% / 1.5388%`. The recorded cause is proxy infidelity. Theorem N adds the structural reason: G4 was trying to make a cheap ranking surface *exact enough*, and Theorem N says no such surface exists for this backend. G4's failure was not a tuning failure.
- **G4 §1's doctrine is upgraded from convention to theorem.** `docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md` §1 states: *"The production planner must not call q11 to rank candidates. > q11 is a verifier, not a ranking primitive."* Theorem N proves that a production planner on this carrier **must** be a bounded finalist ladder — a bounded set of exact evaluations plus a rule for choosing among them — because exactness requires enumeration and enumeration at full width is `O(slots × candidates)`. This retroactively justifies the already-recorded portfolio decision to promote the G5 bounded finalist planner (`RESEARCH_LEDGER.md` PART XV, "Portfolio decisions") and to close G4.
- **Track 16's `KILL-INTERACTION` arm is the only remaining lever, and it is already preregistered.** O11 "scores candidates **in isolation**, so it ignores cross-slot interactions by construction" (`docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md` §1.1). The residual PRA-1 hoped to capture — joint assignment optimisation beyond isolated scoring — is exactly the `Δ_revert` interaction term, already registered by `16-segmentation-routing-fledge.md` §7.1 arm R2 with a `0.25%` kill bar. **PRA-1 must not duplicate it; it should defer to it.**

---

## 3. Corpus classification — correction applied (Track 03 protocol)

Both Track 03 reports were read. Adopted without exception:

| corpus | **role (corrected)** | may support | may **not** support |
|---|---|---|---|
| `V1` | **`KNOWN-STRESS`** — consumed by G3; its `+3.1900%` is the decisive *refutation* of "more regions is better" | regression detection, adversarial control, mechanism diagnosis | **any** generalization, validation, or promotion claim |
| `D1–D4` | `DISCOVERY` | anatomy, headroom census, mechanism diagnosis, threshold-free exploration | held-out claims; absolute ratio claims vs the frontier |
| `generated.json`, `generated.jsonl`, `generated.sqlite`, `generated.log`, `synth-*` | **`SYNTHETIC CONTROL`** — one generator lineage, `make_synth_corpus.py` | prove a mechanism *can* fire; prove it does not regress; measure detection sensitivity | aggregate ratio, Pareto points, or any generalization claim |
| `tests/corpus/pe-*.exe`, `anvil*.exe`, `src.cpp` | `DISCOVERY` / `INSTRUMENT` — `anvil_bench.exe` is the measurement instrument and the corpus copy is a **measured, different build** (`03-heldout-corpus-fledge.md` §6) | executable-family mechanism work only | source-code or instrument claims |

**Track 03's §13 scoreboard, adopted verbatim** (`03-heldout-corpus-fledge.md` §4): 3 independently published structured JSON/NDJSON families → **have 0, FAIL**; 2 mixed-validity families → **have 0, FAIL**; 3 executables spanning ≥2 unrelated producers → 6 files but **1 distinct producer, FAIL on producer isolation**; 2 real non-synthetic numeric streams → **have 0, FAIL**; AITDCC lock → **absent, FAIL**.

**Explicit withdrawal.** The earlier draft's sub-claim *"H-12b: remove the measured G3 held-out regression (`+3.1900%`)"* is **withdrawn entirely as a hypothesis**. `+3.1900%` is a **known-stress replay of already-consumed data**, not held-out evidence. Every reference to it in this report is labelled stress. **No promotion, generalization, or held-out claim in this report depends on V1 or on any synthetic file.**

### 3.1 Exactly what Track 03 must provide before any promotion claim in this family is admissible

A locked, never-fetched, audited family set satisfying `docs/I10-CORPUS-LOCK-PROTOCOL.md` §13 and Fledge's `G1 ADMISSIBILITY` gate:

1. `corpus-lock.json` + `corpus-lock.sha256`, companion-verified, fail-closed (§6); SHA-256 only, never MD5 (`03-heldout-corpus-fledge.md` §15).
2. **`≥ 2` structured JSON/NDJSON families from unrelated publishers** (currently 0). Independence units, not file counts.
3. **`≥ 1` non-synthetic numeric/telemetry or scientific stream** (currently 0 — all 4 numeric files are `make_synth_corpus.py` output).
4. **`≥ 2` mixed-validity / malformed-source families with substantial residual material** (currently 0 — none exists in `tests/corpus`).
5. **`≥ 2` executable producers** with an evidenced toolchain separation, and `src.cpp` / `anvil*.exe` reclassified as instrument, not corpus.
6. `≥ 5` independence units total, `0` ungrouped §5.1 conflicts, `generated.sqlite` resolved against `generated.json`.
7. License clearance for redistribution, or outside-repository fetch (§4.8).
8. A frozen contamination ledger (§7) and an AITDCC lock with A–H as development and **I–P as a frozen external test class**.
9. `generated.*` regrouped as **one** independence unit; `synth-*` as **one**; `{pe-where, pe-winver}` as one (measured 4 KiB collision).
10. A preregistered **V2 threshold set**, frozen before fetch, adjustable only by creating V3.

**Until all ten exist, every number this track produces is labelled `{DISCOVERY}` or `{SYNTHETIC-CONTROL}` and no promotion language is admissible.**

---

## 4. The two branches, kept strictly separate

The coordinator requires that *"repair selector framing"* and *"learn/choose representation"* not be conflated. They are separated here because their evidence status differs completely.

| | **Branch F — selector framing repair** | **Branch R — representation choice** |
|---|---|---|
| what it changes | how many bytes the **already-decoded** choice sequence occupies | which representation is chosen |
| search involved | **none** | enumeration or surrogate |
| exactness | n/a — pure accounting | **withdrawn** (§1) |
| byte bound | **provable**, escape-inclusive (§6) | none available (Theorem N) |
| decoder cost | **provably non-zero** (Incompatibility Lemma, §6.1) | zero **only** if no field changes |
| prior status | unmeasured pool; 1 selector byte/record is the entire pool | G4 `NO-GO`; G5 pivot **PROMOTED**; interaction residual owned by Track 16 |
| verdict this track | **the only thing worth measuring**, and it is a census | **no residual content — defer** |

---

## 5. Branch R — representation choice: closed, with the reason

**R1. Full enumeration is the frozen reference and is already implemented.** G3's `O11` *is* exact per-candidate isolated evaluation. "Compute the exact argmin over the isolated candidate set" reproduces O11 and adds nothing.

**R2. Exact enumeration at full width is not a production planner.** `docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md` §1: O11 performs on the order of one expensive backend call per (column × eligible leaf), `O(slots × candidates)`, *"and therefore not scalable."*

**R3. Cheap-and-exact does not exist.** Theorem N.

**R4. What remains is joint/interaction optimisation**, which O11 excludes by construction (§1.1) and which Track 16's `EXP-R` arm R2 already measures with a pre-registered `KILL-INTERACTION` bar of `0.25%` (`16-segmentation-routing-fledge.md` §7.1). **Track 12 defers to Track 16 and will not run a competing interaction experiment.**

**R5. Consequence: this track holds no promotable mechanism.** The G5 bounded finalist planner is, per Theorem N, not a compromise between exactness and cost — it is the *only sound design class* for this carrier. It is already promoted. Nothing in Branch R is left for track 12.

---

## 6. Branch F — the selector-framing byte bound, fully charged

This is the deliverable that survives scrutiny. It is stated with headers, escapes, and decoder cost, because the earlier draft's version omitted the decoder cost and was therefore wrong.

### 6.1 Incompatibility Lemma — why framing repair cannot also be decoder-free

> **Lemma.** On the frozen carrier the decoder's selector reader is a flat one-byte-per-record table lookup (§4.1: *"Every G5A-LC record has exactly one outer selector byte"*; 32-value frozen table, §8). Any reduction in selector bytes requires the decoder to consume **fewer** bytes, or bytes with **different** meaning. Either is a change to the decoder's parse step, hence `Δ decoder text ≠ 0`.
>
> Therefore **`Δ selector bytes < 0` and `Δ decoder cost = 0` cannot both hold.** The earlier draft's Theorem Z (wire-identical) and a selector-framing repair are **mutually exclusive**.

This is a clean structural result and it explains why the frozen design charges exactly one selector byte and treats it as irreducible. Any proposal to reduce it must pay decoder code.

### 6.2 The escape-inclusive charge bound (provable)

Code space, all inside the existing one-byte-per-record frame:

```
code 0..L-1   PER_RECORD   : this record uses family `code`
code L        RUN_FAM      : uvar_len(n) bytes, then 1 family byte  -> n records share family
code L+1      RUN_LIT      : uvar_len(n) bytes, then n family bytes -> explicit literal run
code L+2      RESERVED     : decoder rejects (malformed-input hard fail)
```

`L` is the number of live families the decoder already implements; codes `≥ L+2` are rejected, so malformed input fails closed.

Per run `r` of length `R_r`, the encoder emits the cheaper of the two encodings:

```
cost_r = min( R_r ,                                    # PER_RECORD for every record
              1 + uvar_len(R_r) + 1 )                  # RUN_FAM
```

`RUN_LIT` is the escape for the case where a future family pushes `L` up, or as an explicit long-run alternative; because `cost_r` already takes a `min` against `R_r`, **`RUN_LIT` can never be the cheaper branch and is retained only for forward-compatibility and fail-closed decoding.**

> **Bound F (provable, escape-inclusive).**
> ```
> selector_bytes(F) = Σ_runs cost_r  ≤  Σ_runs R_r  =  R
> saving(F)         = Σ_runs max(0, R_r − 2 − uvar_len(R_r))  ≥  0
> ```
> Equality holds **iff** every run has `R_r ≤ 3`. Worst case is exactly the incumbent's `R` bytes — never worse.

Worked values (`uvar_len(n) = 1` for `n < 128`, `2` for `n < 16384`):

| run length | incumbent | F2 (bytes) | saving |
|---:|---:|---:|---:|
| 1 | 1 | 1 | 0 |
| 3 | 3 | 3 | 0 |
| 4 | 4 | 3 | **1** |
| 16 | 16 | 3 | **13** |
| 127 | 127 | 3 | **124** |
| 1,000 | 1,000 | 4 | **996** |
| 100,000 | 100,000 | 5 | **99,995** |

### 6.3 The complete charge — every term, with status

| term | value | status |
|---|---|---|
| selector bytes | `−saving(F)` from Bound F | **MEASURED-BY-PROOF** |
| decoder text | RLE reader replacing a single byte load | **PROJECTION ~40–120 B x86-64; must be measured with `nm --size-sort`** |
| decoder rodata | **0 B** — the family table is unchanged | proven |
| decoder per-file state | **+2 registers** (run remaining, pointer) | proven |
| decoder cycles | `− (mispredicts eliminated) + (one extra branch per record)` | **MEASUREMENT REQUIRED — sign unknown, may be a loss** |
| escapes | bounded by `min`, so `≤ 0` extra bytes | proven |
| framing / envelope | **0 B** — no field added or widened | proven |
| encode time | `O(R)` scan to compute runs | trivial |
| AITDCC 1 MiB decompressor cap | must be charged | measurement required |

### 6.4 The decisive fact about Branch F

**The total byte pool is 1 byte per record, and `R` has never been reported.** The available saving is at most `R` bytes. On a carrier whose complete bytes are ~10^6, a pool of `10^4` records is 1%; a pool of `10^3` is 0.1%. **Branch F's entire value is bounded above by a number nobody has measured, and the measurement is free** — it is a census over diagnostics already frozen in `docs/I10-G5A-LOCALITY-CONTROLS-PREREG.md` §7, which "add no new candidate search and no new Brotli call."

---

## 7. Cost model (Branch F only; Branch R has no mechanism)

| quantity | cost |
|---|---|
| bytes | `−Σ_runs max(0, R_r − 2 − uvar_len(R_r))`, bounded by `−R` |
| decode cycles | unmeasured; upper bound `+1` branch per record, offset by eliminated mispredicts |
| decode text | ~40–120 B projection |
| decode rodata | 0 |
| decode state | `O(1)`, 2 registers |
| peak RSS | 0 delta |
| encode | `O(R)` |

---

## 8. MEASURED evidence vs PROJECTION

### 8.1 MEASURED (transcribed with exact sources)

| # | Fact | Value | Source |
|---|---|---|---|
| M1 | Frozen grid | `33 non-dominated \| 5 FRONT-GAP \| 0 FRONT-CROSSING \| 28 DEGENERATE \| 435/468 dominated` | `RESEARCH_LEDGER.md` PART XV §facts 1 |
| M2 | G4 = `NO-GO`; cause is proxy infidelity | regret `6.6056% / 7.0802% / 1.5388%`; speed withdrawn `INVALID_SPEED_ACCOUNTING` (667.948690 ms vs 0.004758 ms on D1) | PART XV §G4; recon §2 |
| M3 | G4's fidelity bars | aggregate `≤ 0.0025`; per-file `≤ 0.0050` | `docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md` §G8.2/§G8.3 |
| M4 | **"q11 is a verifier, not a ranking primitive"**; O11 is `O(slots × candidates)`, **not scalable**; O11 scores candidates **in isolation** and ignores cross-slot interactions by construction | verbatim | same §1, §1.1 |
| M5 | Selector is one byte per record, charged in every arm; 32-value frozen table | `complete_bytes = 1 + Brotli_q11_lgwin30(body).size()` | `docs/I10-G5A-LOCALITY-CONTROLS-PREREG.md` §4.1, §8 |
| M6 | Selector/run diagnostics already frozen and **cost zero Brotli calls** | `shape_transition_count`, `slot_transition_count`, `same_shape_adjacent_pair_fraction`, `adjacent_equal_token_length_fraction`, `permutation_liveness_class` | same §7 |
| M7 | Assignment is first-order on a fixed charged multiset | A0/A1/A2/A3 = `2,054,532 / 1,470,205 / 1,452,383 / 1,380,245`; A3−A1 = **89,960 B**; 4/4; `COLUMN-DOMINANT` | PART XV §G5A |
| M8 | Same effect reverses on stress | V1 `V1-COLUMN-ADVERSE` | same |
| M9 | Region metadata can consume 29–371% of the causal gain | D1 `W/gain` = **371%**, D2 **88.7%**, D3 **33.3%**, D4 **29.3%**; `KILL-WIRE` bar = median ≥ 0.25 | `16-segmentation-routing-fledge.md` §3.1, §7.1 |
| M10 | Interaction term is already preregistered elsewhere | `Δ_revert` arm R2; `KILL-INTERACTION` at `0.0025 · C(O11,f)` on ≥3/4 files | same §7.1 |
| M11 | No unopened held-out structured family; V1 is `KNOWN-STRESS`; LOFO over 4 spent files is not a validation protocol | — | PART XV portfolio bullet; `16-segmentation-routing-fledge.md` §3.4 E18, §5 |
| M12 | Corpus §13 fails 3 of 5 counts; structured + numeric families are 100% synthetic, single-lineage | see §3 table | `03-heldout-corpus-fledge.md` §4, §5 |
| M13 | `tests/corpus/anvil_bench.exe` is a **different build** than `build/anvil_bench.exe`; a manifest-drift incident already occurred and recurred | sha256 `fcd30da5…` vs `379341d9…` | same §6 |
| M14 | G3 discovery −5.5984%; G3 stress **+3.1900%** | 1,384,654 vs 1,466,769 / 2,179,615 vs 2,112,235 | PART XV; recon §1 Class C |
| M15 | Decode-only crossings closed, 12 of 13 files; byte-identical decode ceiling ~1.79x | MATH-class | PART XIII §3 |
| M16 | Local Windows C++ build is BLOCKED (no RC compiler) | — | PART XV verification addendum |

### 8.2 PROJECTION / ARITHMETIC (labelled; not citable as measurement)

| # | Statement | Status |
|---|---|---|
| P1 | `Brotli_q11(q‖q) ≈ m` bytes vs `H0(q‖q) = 2m` bytes | arithmetic for `H0`; **projection** for the Brotli figure; the **violation** follows from RFC 7932's copy/dictionary commands and is independent of any measurement |
| P2 | Decoder text for the RLE reader ≈ 40–120 B | projection; must be measured |
| P3 | Net decode-cycle sign for Branch F | **unknown**; may be a loss |
| P4 | `R` (record count) and hence Branch F's byte pool | **unmeasured**; the entire value of the branch |
| P5 | Prior-art: BLOT / blocally-compressible MDL block-representation selection, zstd opt-level-3 per-block coder trials, 7-Zip coder graph, racing/multi-fidelity | reported by `16-segmentation-routing-fledge.md` §4; **not independently verified by this agent** |

---

## 9. Novelty and prior-art risk

- **Theorem N is not a compression novelty claim.** It is the standard "no distribution-free lower bound tighter than trivial" statement applied to an LZ+context backend. It is reported because it discharges a project convention by proof, not because it is new.
- **Branch F is adopt-class engineering** at best: run-length coding of a side stream is textbook, and the selector stream is exactly a side stream. No novelty claim is made or possible.
- **Risk on any Branch-R successor is HIGH**, per `16-segmentation-routing-fledge.md` §4, which is a stronger assessment than this lane originally recorded. Track 20 owns the kill; this report supplies it the Theorem N separator, which should *kill* most Branch-R novelty attempts on its own (a mechanism cannot claim to be exact-and-cheap on this carrier).

---

## 10. Adversarial failure cases

| # | Failure | Consequence | Gate |
|---|---|---|---|
| F1 | **Admissible bound assumed without proof** (the failure this report just made) | silent loss of the true winner; mechanism mis-described as exact | withdrawn; Theorem N binds any successor |
| F2 | **`R` too small** → pool `≈ 0` bytes | Branch F worthless | the census, §11.1 |
| F3 | Long runs absent → saving `= 0` | Branch F worthless | same census reports the run histogram |
| F4 | RLE reader costs more cycles than it saves mispredicts | Branch F is a **loss** on the decode axis | cycles measured remotely, paired, same run; sign gate |
| F5 | Decoder text growth vs AITDCC 1 MiB cap | unusable under deployment constraint | `size` charged every run |
| F6 | Malformed selector stream (`L+2`, truncated uvar, over/under run) | decoder vulnerability | hard fail-closed; codes `≥ L+2` rejected; remote fuzz only |
| F7 | Branch R resurrected on a proxy | repeats G4 at 1.5–7% regret | Theorem N + `KILL-INTERACTION` bar |
| F8 | V1 or a synthetic file used as generalization evidence | protocol breach | §3 role table is binding |
| F9 | Splice provenance across runs | fabricated result | census is byte-only, so structurally impossible |
| F10 | Local execution | protocol breach | everything remote; local build BLOCKED anyway (M16) |

---

## 11. Recommendation: **HOLD**

### 11.1 What this track contributes, in final form

1. **A withdrawn claim, with its disproof** (§0, §1) — recorded rather than quietly deleted, because a wrong admissibility argument is exactly the kind of error that would otherwise be re-derived by a successor lane.
2. **Theorem N** (§2) — a structural result that (a) explains G4's `NO-GO` mechanically, (b) upgrades G4 §1's doctrine from convention to theorem, (c) justifies the already-promoted G5 bounded finalist ladder as the *only* sound design class for this carrier, and (d) hands Track 20 a decisive separator for Branch-R novelty attempts.

   > **CORRECTED BY ADDENDUM B.** Items (b), (c) and (d) over-claimed. The supported statements are narrower: **(P1)** unconditional-frequency bounds are not admissible for Brotli (proved); **(P2)** absent a *proven* admissible bound or an *exact* reuse scheme, per-candidate evaluation is the exact method this project currently has — a statement about the present, not an impossibility. The screening rule issued to Tracks 16/20 is retracted (Addendum B §B.4).
3. **The Incompatibility Lemma** (§6.1) — selector bytes cannot be bought for free on this carrier; this explains the frozen design's irreducibility.
4. **Bound F** (§6.2) — a complete, escape-inclusive, provable byte bound with every cost term charged, including the decoder cost that the earlier draft omitted.

### 11.2 Why HOLD rather than PILOT

The earlier draft recommended PILOT on the strength of an exactness claim that is now withdrawn. With it withdrawn:

- **Branch R has no residual content.** Exact enumeration is O11 (already built); affordable-exact is impossible (Theorem N); the interaction residual is owned by Track 16 (`EXP-R` R2). Nothing is left to pilot.

  > **CORRECTED BY ADDENDUM B.** The middle clause "affordable-exact is impossible (Theorem N)" is **withdrawn as an over-claim** — exact-and-cheap search on a *restricted* candidate family is **not** ruled out (§B.2 items 1–5, including exactly-sound memoization). Branch R still closes for the two *correct* reasons: the isolated-object exact method already exists as `O11`, and the complete-carrier objective is a different, harder quantity that no lane has computed.
- **Branch F's value is bounded by an unmeasured pool** (`R`, one byte per record). Piloting the *repair* before measuring the *pool* is backwards: the repair's total possible benefit is `≤ R` bytes, and `R` is free to measure.

### 11.3 The single remote measurement still worth running (pre-registered, byte-only, zero new Brotli calls)

**`POOL-1` — selector pool census.** Byte-only GitHub Actions run over `D1–D4` (`{DISCOVERY}`) using the diagnostics already frozen in `docs/I10-G5A-LOCALITY-CONTROLS-PREREG.md` §7. Reports, per file: `R` (record count); `complete_bytes`; the per-record family run-length histogram; `mean/median run length`; `saving(F)` from Bound F; and the frozen `I10` G5A same-run floor replay.

Pre-registered, before any run:

```
KILL-POOL-F   if  saving(F) aggregate / Σ complete_bytes  <  0.0025   (0.25%)
KILL-POOL-R   if  R = 0, or the carrier does not use per-record selectors
SUPPORT-POOL  if  saving(F) aggregate / Σ complete_bytes >= 0.0100 (1.0%)
                AND saving(F) > 0 on >= 3 of 4 files
                AND the same-multiset invariant I5 holds exactly on every arm
```

`0.25%` and `1.0%` are deliberately the same bars as G4's frozen fidelity tolerance (M3) and Track 16's `SUPPORT` bar respectively, so the three lanes are commensurable and a coordinator can read them together.

**`SUPPORT-POOL` licenses exactly one successor: a remote, paired measurement of the RLE reader's decode-cycle delta and decoder-text delta on the frozen carrier.** It licenses **no** promotion, because §3's ten-item corpus requirement is unmet.

### 11.4 Explicit HOLD triggers already recorded (any one ⇒ stay HOLD)

1. `POOL-1` returns `KILL-POOL-F` or `KILL-POOL-R`;
2. Track 16's `KILL-INTERACTION` fires (Branch R closed independently);
3. Track 03 does not deliver the §3.1 ten-item locked family set;
4. any successor proposes a cheap-and-exact search on this carrier (Theorem N forbids it);
5. the G3/G5A frozen sources cannot be published and dispatched — a user-authorization dependency (`RESEARCH_LEDGER.md` PART XV "Pending handoff"), not a technical one.

### 11.5 Claims that must never be made from this track

- **exactness, optimality, or oracle-equivalence** for any encoder-side search on this carrier;
- **any bound on `Brotli(x)` derived from order-0 or any non-Brotli model** — Theorem N;
- **mechanism-level novelty** — Branch F is adopt-class; Theorem N is standard theory;
- **a FRONT-CROSSING** (M15, DNB-M1 MATH-class);
- **any generalization, validation, or promotion claim resting on V1 or a synthetic file** (§3);
- **any timing figure assembled across runs** (M9-class splicing; structurally moot for a byte-only census).

---

## 12. Isolated artifact

`prototypes/swarm-2026-10-02/12-encoder-search/space-bunny/pra1_bound_check.cpp` — specification-grade single file containing the **H0-admissibility falsification harness** for §1 (the counterexample family and a Brute-force check over small payloads), Bound F's encoder/decoder arithmetic with the `min`-escape, and the `POOL-1` counters.

**Status: NOT COMPILED, NOT EXECUTED, NOT MEASURED.** No corpus runner, no timing harness, no benchmark path; it cannot produce a number. Windows C++ build verification is BLOCKED (M16), so no local compile was attempted and none is claimed. Nothing under `src/`, `FORMAT.md`, or `tools/` is touched; no production format ID is requested. No commit, push, reset, clean, stash, restore, or rebase was performed.

---

*Space Bunny Free, track `12-encoder-search`. Final verdict: **HOLD**. Deliverables: a withdrawn claim with its disproof, Theorem N, the Incompatibility Lemma, Bound F, and the `POOL-1` census preregistration.*
---
---

# ADDENDUM A — FORMAL CLOSEOUT OF THE COORDINATOR MATH CHALLENGE

**Dated:** 2026-10-02
**Raised by:** coordinator (cross-track math challenge), after the Track 03 / Track 16 corpus and metadata-economics reviews
**Addressed to:** the pre-correction claim set of this report
**Status:** **binding closeout.** Nothing in this addendum may be silently folded back into the body without a new dated revision.

---

## A.0 Process disclosure — history was overwritten and is now restored in place

**Disclosure, recorded rather than concealed.** When the challenge arrived, the pre-correction body of this file was **overwritten in place**, not appended to. The original 40,107-byte version was therefore not preserved in the file at that moment. That is exactly the failure mode the coordinator's instruction guards against.

**Remediation, effective now.** This addendum restores the withdrawn claims **verbatim** so the record is auditable inside the file. A reader must be able to see (i) what was claimed, (ii) what refutes it, and (iii) what survives, without consulting any external channel.

Version history of this file:

| version | date | content | disposition |
|---|---|---|---|
| `v0` | 2026-10-02 | Pre-correction report: PRA-1 "Admissible-Bound Exact Arbitration", Theorem Z, H0-based prune, `SELREG-1` | **WITHDRAWN IN PART** — claims quoted verbatim in §A.1; the invalid ones struck in §A.2 |
| `v1` | 2026-10-02 | Post-correction body: correction notice, counterexample, Theorem N, Incompatibility Lemma, Bound F, `POOL-1`, verdict HOLD | current body |
| `v2` | 2026-10-02 | **This addendum** | binding closeout |

---

## A.1 The original claims, verbatim (from `v0`)

Quoted unchanged so the refutation has a fixed target.

**Original mechanism statement (`v0` §1):**

> ### **PRA-1: Admissible-Bound Exact Arbitration inside the already-charged selector alphabet**
>
> The encoder evaluates the *exact* complete charged byte cost of every admissible representation assignment over a frozen option set, prunes candidates with a **one-sided admissible bound** that never requires a backend call, and emits the argmin. The emitted frame is **byte-identical in grammar, field set, field count, field widths, and decoder code path** to the frame the incumbent encoder already emits. Only the *values* of already-charged selector bytes change.

**Original admissibility claim (`v0` §1(b)):**

> **(b) The search prunes with a one-sided bound.** Define the admissible lower bound
>
> ```
> LB(f,s)  =  framing_min(f,s)  +  1  +  max(0, ceil(H0(payload_f(s))))
> ```
>
> where `H0` is the order-0 ideal-code length over the 256-bin histogram of the family's serialized payload on segment `s`, and `framing_min` is the exact fixed width of the fields the family requires. Since no prefix/entropy coder without an explicit model can emit fewer bytes than the ideal code length of the string it codes, and the framing width is exactly known, **`cost(f,s) ≥ LB(f,s)` always**. Therefore if `LB(f,s) ≥ U`, where `U` is the incumbent's complete bytes, `f` cannot be the argmin and is discarded **with zero backend calls**.
>
> This is the entire technical difference from the failed lanes, and it is worth stating precisely:
>
> > **A regression is a two-sided estimate; an admissible bound is one-sided. A two-sided estimate cannot be used to prune without risking loss; a one-sided bound can.**

**Original exactness claim (`v0` §4):**

> **Soundness of the prune.** … Therefore no discarded candidate can improve `U`, and **the emitted argmin equals the exact argmin over the admitted set** whenever `E ≤ floor(beta·n)` is not binding.

**Original Theorem Z (`v0` §2.1), retained for audit:**

> **Theorem Z (zero decoder delta).** Let `W_inc` be the wire produced by the incumbent encoder under the frozen G5A-LC carrier grammar and `W_pra` the wire produced by PRA-1 over the same frozen grammar, same frozen option set, same frozen backend. Then: … (4) the only fields whose *values* differ are the already-charged selector bytes; (5) the decoder's parse path, dispatch table, code, and per-record work are bit-identical …

**Original control arm (`v0` §7), now invalid:**

> | `A_greedy_h0` | argmin of `LB` only — i.e. the bound *without* any backend call. **This is the important control**: it separates "the admissible bound is informative" from "the exhaustive evaluation is informative" |

**Original decisive experiment and its gates (`v0` §7):**

> `SELREG-1` … `sel_regret = complete_bytes(incumbent) − complete_bytes(exact argmin)` …
> ```
> KILL   if  sel_regret_agg  <= 0.0025   (0.25%, = G4's frozen aggregate fidelity bar)
>   or   sel_regret(f)     <= 0.0050    for every file (0.50%, = G4's frozen per-file bar)
> ```
> `GO-1  sel_regret_agg >= 1.00%   and  sel_regret(f) >= 1.00% on >= 3 of 4 discovery files`

**Original verdict (`v0` §12):**

> ### **PILOT** — one byte-only remote run, `SELREG-1`, gated on a kill threshold borrowed from the project's own preregistration.

---

## A.2 Formal withdrawals

The following are **withdrawn without qualification**. Each is struck here, dated, and must not be restated in any artifact, message, or successor lane.

### **W-1 — The `H0` admissibility theorem is WITHDRAWN.**

> Withdrawn: *"`cost_Brotli(f,s) ≥ LB(f,s)` always … since no prefix/entropy coder without an explicit model can emit fewer bytes than the ideal code length of the string it codes."*

**Refutation (arithmetic; no measurement required).** For `p = q ‖ q` with `q` uniform over 256 symbols and `|q| = m`:
- `H0(p) = 16m` bits `= 2m` bytes (exact — every value occurs exactly twice, so each symbol costs exactly 8 bits);
- `Brotli_q11(p) ≤ m` bytes `+ O(log m)` **by construction**, because RFC 7932 provides copy commands over reconstructed output and a static dictionary, so the second `q` costs a length/distance pair rather than `m` literals.

The claimed inequality is therefore violated by a factor of `≈ 2` for every `m`. **The premise of the quoted sentence is false**: the frozen backend is *not* "a prefix/entropy coder without an explicit model." It has LZ77-style backward references over the reconstructed history and a static dictionary, both of which are strictly stronger than order-0 symbol coding. The exact Brotli figure in `v1` §1 is labelled a **projection**; the **violation** follows from the format specification and is independent of any timing or byte measurement.

**Scope of the withdrawal.** `H0` remains a perfectly good **estimator** and remains useful as a *ranking* heuristic or as an oracle-quality diagnostic. It is withdrawn **only** as a **lower bound**, i.e. only as a pruning certificate.

### **W-2 — The "exact safe prune" claim is WITHDRAWN.**

> Withdrawn: *"if `LB(f,s) ≥ U` … `f` cannot be the argmin and is discarded"*; and consequently *"the emitted argmin equals the exact argmin over the admitted set."*

**Refutation.** Since `LB` is not a lower bound on the backend's output, the elimination test `LB(f,s) ≥ U` can hold while `cost(f,s) < U`. The discarded candidate **can** be the argmin. The prune is therefore neither exact nor safe; it is a heuristic that happens to be phrased as a proof.

**Withdrawn consequences, explicitly:**
- the `v0` §4 soundness argument ("no discarded candidate can improve `U`");
- the characterisation of PRA-1 as **exact**, **optimal**, or **oracle-equivalent**;
- the `A_greedy_h0` control arm as specified in `v0` §7 — it optimizes a non-admissible surrogate and cannot separate "admissible" from "exhaustive," because it is neither;
- the `SELREG-1` decision gates (`KILL` at `0.25%`/`0.50%`, `GO-1` at `1.0%`), which were keyed to `sel_regret` measured against an "exact argmin" that no longer exists. `SELREG-1` is **superseded and withdrawn as a decision instrument** by `POOL-1` (`v1` §11.3).

### **W-3 — Framing-only pruning is universally safe but almost certainly non-informative.**

Retaining only the framing term:

```
LB_frame(f,s) := framing_min(f,s)
```

**Universally safe.** This *is* a valid lower bound: the backend's output must at minimum carry the fields the family requires, and those widths are exactly known from the frozen grammar. It is therefore legitimate to prune on it.

**Non-informative, and here is the bound.** Its total pruning power over a whole file is at most

```
max_f framing_min(f,s) − min_f framing_min(f,s)     summed over slots s
```

On the frozen G5A-LC carrier the per-slot fields are a leaf ID and a payload length, both already present and fixed-width except for varint width differences. The spread is therefore on the order of **1–2 bytes per slot**. Against complete-carrier sizes of `10^5`–`10^6` bytes that is `≤ 0.2%` of the file, and the test `LB_frame(f,s) ≥ U` fires only in the knife-edge case where the candidate is already within 1–2 bytes of the incumbent — i.e. on ties.

**Formal statement.** `LB_frame` is a valid and free lower bound whose **discriminating power is bounded above by the framing-width spread, a few bytes per slot.** It is retained as a correctness sanity check, not as a search accelerator. It does not license any claim of exactness and it does not change any verdict.

---

## A.3 What SURVIVES, stated as the preserved negative theorem

> **[SUPERSEDED BY ADDENDUM B, 2026-10-02 — READ B FIRST.]**
> The statement below was written as an **impossibility theorem**. It is not one. A counterexample to one payload-only marginal-entropy bound does **not** establish an impossibility result over the algorithm class. Addendum B replaces "Theorem N + Corollary" with the narrower, proved **Proposition P1** (admissibility failure) and the descriptive **Finding P2** (status of exact methods), and **retracts** the screening rule Addendum A issued to Tracks 16 and 20. The text of A.3 is left in place, unedited, as the historical record of what was claimed.

Theorem Z (`A.1`) **survives unchanged**, because it is a statement about *field structure* under a change of selector *values*, not about optimality: if the grammar, field set, field count, field widths, selector alphabet cardinality, and per-record selector width are all held frozen, then changing only the selector values produces a frame the existing decoder parses by the identical path.

**Theorem Z does not survive in one respect**, and the correction is stated here so it is not lost: in `v0` Theorem Z was used to argue that the mechanism was free *and* exact. It establishes **free**; it never established **exact**. That conflation was the first error, and `H0` inadmissibility was the second.

### **THE PRESERVED NEGATIVE THEOREM (final form, binding)**

> **Theorem N (no encoder-computable nontrivial admissible bound).** Let `B` be a lossless backend whose decoder maintains backward references over previously reconstructed output, a static dictionary, and adaptive context modelling — for the frozen carrier, `Brotli_q11_lgwin30` (RFC 7932). Let `LB` be any function the encoder computes from the payload alone. If `LB` is not derived from `B`'s own model class, `LB` is **not** a valid lower bound on `B(x)` for all payloads `x`; there exists a payload family on which `LB` exceeds `B(x)` by an unbounded factor.
>
> **Corollary (the operative consequence, and the reason PRA-1 is closed).** In the **absence of a backend-valid lower bound**, an **exact** `Brotli` argmin over a candidate set **requires evaluating the backend on every candidate**. That is oracle-class cost: `O(slots × candidates)` expensive evaluations, which `docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md` §1 already records as *"on the order of one expensive backend call per (column × eligible leaf) … and therefore not scalable"* and closes with the doctrine *"q11 is a verifier, not a ranking primitive."*
>
> Therefore **there is no method on this carrier that is simultaneously exact and cheap.** The design space is exhausted by three options and there is no fourth:
>
> 1. **Exact** ⇒ full enumeration ⇒ oracle-class cost ⇒ not a production planner.
> 2. **Cheap** ⇒ requires an admissible lower bound ⇒ forbidden by Theorem N ⇒ collapses into option 3.
> 3. **Surrogate** ⇒ approximate ⇒ the only affordable class, and the class in which G4 failed at `1.5388%–7.0802%` worst-file regret and the G5 bounded finalist ladder operates.

**Consequence for the coordinator.** Any successor proposal to this project that claims an *exact* representation search achievable at *sublinear* or *budgeted* cost on the Brotli carrier should be rejected on Theorem N **before** any implementation or preregistration. Theorem N is standard information-theory content — the absence of a distribution-free lower bound tighter than trivial — reported here because it discharges a project convention by proof and because it must be cited by Tracks 16 and 20 when they screen Branch-R candidates.

---

## A.4 Revised final verdict

| | `v0` (pre-challenge) | `v2` (this closeout) |
|---|---|---|
| mechanism | PRA-1, exact admissible-bound arbitration | **no mechanism**; Branch R closed, Branch F reduced to a census |
| exactness claim | asserted | **WITHDRAWN (W-2)** |
| `H0` admissibility | asserted | **WITHDRAWN (W-1)** |
| Theorem Z | asserted as "free **and** exact" | **SURVIVES** as "free" only |
| `SELREG-1` | decisive experiment | **WITHDRAWN** as a decision instrument; superseded by `POOL-1` |
| surviving prune | `H0`-based | `framing_min` only — safe, `≤ ~1–2 B/slot`, non-informative (§A.2 W-3) |
| verdict | **PILOT** | **HOLD** |

### Final verdict: **HOLD**

**Contributions, final and binding:**

1. **W-1, W-2, W-3** — three dated withdrawals with their refutations, retained in the file so no successor re-derives them.
2. **Theorem N + Corollary** (§A.3) — the preserved negative theorem: absent a backend-valid lower bound, exact Brotli argmin requires evaluating every candidate, i.e. oracle-class cost; hence no exact-and-cheap method exists on this carrier.
3. **Theorem Z, narrowed** — wire identity holds for a change of selector *values*; it never conferred exactness.
4. **Incompatibility Lemma** (`v1` §6.1) — selector bytes cannot be reduced at zero decoder cost; this explains the frozen design's irreducibility.
5. **Bound F** (`v1` §6.2) — the escape-inclusive, fully charged, provable selector-framing byte bound, with the decoder cost the earlier draft omitted.
6. **`POOL-1`** (`v1` §11.3) — the only remote measurement still worth running: byte-only, zero new Brotli calls, `{DISCOVERY}`-labelled, gated at the project's own `0.25%`/`1.0%` bars. It measures a *pool*, not a mechanism, and licenses no promotion while the §3.1 ten-item corpus requirement is unmet.

**HOLD triggers, unchanged and binding:** `POOL-1` returns `KILL-POOL-F`/`KILL-POOL-R`; Track 16's `KILL-INTERACTION` fires; Track 03 does not deliver the ten-item locked family set; any successor proposes a cheap-and-exact search on this carrier; the G3/G5A frozen sources cannot be published and dispatched.

---

*Addendum A, 2026-10-02, binding. Coordinator challenge accepted in full. The pre-correction claim set is preserved verbatim in §A.1; the invalid claims are struck in §A.2; the preserved negative theorem is stated in §A.3; the verdict is revised to **HOLD** in §A.4. This addendum closes track `12-encoder-search`.*
---
---

# ADDENDUM B — NARROWING OF THE NEGATIVE RESULT

**Dated:** 2026-10-02
**Raised by:** coordinator (second math correction)
**Supersedes:** Addendum A §A.3 in its entirety, and Addendum A §A.4 item 2
**Does not change:** withdrawals `W-1`, `W-2`, `W-3`; Theorem Z (narrowed); the Incompatibility Lemma; Bound F; `POOL-1`; the verdict

---

## B.0 Why this correction is being made

Addendum A named its central result **"Theorem N"** and stated a **Corollary** that exact search "requires evaluating the backend on every candidate", concluding that *"no method on this carrier is simultaneously exact and cheap"* and instructing Tracks 16 and 20 to **reject** any successor claiming exact-and-cheap search.

**That was a category error.** The proof I supplied is a single counterexample against **one** bound family — bounds computed from unconditional payload symbol frequencies. Refuting one member of a bound family does not produce a lower bound over the *algorithm class* of all search procedures. Naming it a theorem, and turning it into a screening rule, overstated what was shown. The coordinator is correct and the claim is withdrawn as stated.

**History is preserved, not rewritten.** Addendum A §A.3 remains in the file, unedited, with a supersession banner. This addendum replaces it.

---

## B.1 What the proof actually supports (narrow, and proved)

### **Proposition P1 — admissibility failure (PROVED)**

> For the frozen backend `B = Brotli_q11_lgwin30`, **no function of the payload's unconditional symbol frequencies alone is a valid lower bound on `B(x)`** for all payloads `x`. In particular `ceil(H0(x))` is not admissible, and neither is any monotone transform of it, nor any bound of the form `g(H0(x))`.

**Proof.** Let `q` be `m` bytes uniform over 256 symbols and let `x = q ‖ q`.
- `H0(x) = 16m` bits `= 2m` bytes — exact, since every value occurs exactly twice and therefore costs exactly 8 bits.
- RFC 7932 provides copy commands over reconstructed output and a static dictionary, so `B(x) ≤ B(q) + O(log m)` bits. Since `B(q) ≤ 8m` bits for any order-0-coded `q`, we get `B(x) ≤ m` bytes `+ O(log m)`.
- Hence `H0(x)/B(x) ≥ ≈ 2`, unbounded in `m`, for a bound that ignores repetition and dictionary membership. ∎

**What P1 does *not* say.** P1 says nothing about bounds that are **not** functions of unconditional frequencies — in particular nothing about bounds that use Brotli's own model, the static dictionary, copy-command reachability, or candidate-family structure. §B.2 enumerates these as open.

### **Finding P2 — status of exact methods (DESCRIPTIVE, not a theorem)**

> **Absent (i) some *proven* admissible lower bound for `B`, or (ii) an *exact* evaluation scheme that shares, reuses, or incrementally extends compression state across candidates, the only exact evaluation method available in this project today is per-candidate evaluation.**

This is a statement about **what exists in this project as of 2026-10-02**, not a statement about what is possible.

Two distinct objectives must not be conflated, and Addendum A conflated them:

| objective | exact method today | cost |
|---|---|---|
| **isolated-object** cost `B(o(c))` for candidate `c` | **already implemented** — G3's `O11` | `O(slots × candidates)` *isolated leaf-object* evaluations, plus exactly `final_q11 = 4` whole-carrier encodes per proxy per file (`docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md` §G8.5) |
| **complete-carrier** cost `C(assignment)` | **nobody has computed it** | combinatorial in `K^slots`; no bounded method has been proposed or measured anywhere in this project |

Addendum A's "exact" referred to the first row and therefore **described something that already exists** (O11). That is an independent reason the original PRA-1 had no residual content, separate from anything about bounds.

---

## B.2 Explicitly NOT ruled out (the holes in my own argument)

Each of the following is a live research direction that Addendum A's "no exact-and-cheap method" phrasing wrongly appeared to foreclose. For each I state the specific unresolved obstacle, so a successor knows what must actually be proven.

| # | not ruled out | what remains to be shown |
|---|---|---|
| 1 | **Backend-aware lower bounds** derived from Brotli's actual model — context-map class, static-dictionary membership, copy-command reachability, window availability | a bound provably `≤ B(x)` for all `x`, exploiting structure that unconditional frequencies discard. Plausible for *restricted candidate families* (e.g. all candidates that are dictionary-encoded are bounded below by the exact code length). Not attempted here. |
| 2 | **Shared / incremental compression-state evaluation** | whether two candidates sharing a prefix can be evaluated with one partial Brotli pass, exactly. Plausible in principle for candidates that differ only in a suffix; would likely require a **format change**, hence non-zero decoder cost and a new preregistration. |
| 3 | **Exact dominance certificates** between candidates | a provable partial order (e.g. `X` dominates `Y` iff some structural containment forces `B(X) ≤ B(Y)`) independent of running `B`. Unproved here and **not refuted**. |
| 4 | **Candidate-family structural bounds** | a *closed* family whose members admit a shared structural lower bound, so that only family representatives need evaluation. This is the most promising direction and directly addresses Addendum A §A.3's "no residual content" claim, which this addendum therefore also weakens. |
| 5 | **Memoization of duplicate candidates** | **this one is exactly valid and I failed to state it.** `B` is a deterministic function of (payload bytes, parameters), so two candidates with byte-identical payloads and identical parameters have *identical* `B` output. Deduplicating identical candidate payloads before evaluation is an **exactly sound** reduction of the call count — not a heuristic. Addendum A implicitly denied this. |
| 6 | **Branch-and-bound seeded by information derived from Brotli's own model** | as (1); the bound quality is the whole question |

**Consequence.** Any of 1–6 could, if it worked, make exact search affordable on a restricted candidate family. **PRA-1 as specified had none of them**, which is why *PRA-1 specifically* is dead — the withdrawal in `W-2` is unaffected. But the *lane* is not closed on impossibility grounds.

---

## B.3 Corrected cost statement for the existing exact method

Addendum A wrote `O(slots × candidates)` "expensive evaluations" without qualification. The precise, source-backed statement:

```
O11 cost  =  (number of eligible candidates) isolated leaf-object Brotli_q11 encodes
             +  exactly 4 whole-carrier final encodes per proxy per file
```

Source: `docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md` §1 ("on the order of one expensive backend call per (column × eligible leaf), which is `O(slots × candidates)` and therefore not scalable") and §G8.5 (`final_q11(P) = 4 per proxy per file … no deduplication of byte-identical portfolios is permitted in G4`).

The last clause is notable and I should have cited it: **G4 explicitly forbade deduplicating byte-identical portfolios.** That is a *protocol* choice for measurement hygiene, not a statement that deduplication is unsound — consistent with B.2 item 5, which is therefore an available and exactly-sound saving that G4 declined to take.

---

## B.4 Retraction of the screening rule issued to Tracks 16 and 20

Addendum A §A.3 stated:

> *"Any successor proposal to this project that claims an exact representation search achievable at sublinear or budgeted cost on the Brotli carrier should be rejected on Theorem N before any implementation or preregistration."*

**That screening rule is RETRACTED.** It is not coordinator-approved and it is not supported by the proof.

**Replacement rule, for Tracks 16 and 20 to apply:**

> Reject a successor's *exactness* claim only if the successor **relies on a bound derived from unconditional payload statistics** (Proposition P1 applies), or **asserts exactness without stating which of B.2 items 1–6 it is using**. Do **not** reject a successor merely for claiming bounded exact evaluation cost; that claim is testable and must be tested by measurement, not refused by appeal to a theorem.
>
> When screening an exactness claim, require the successor to name: (a) the admissible bound and its proof, or (b) the exact reuse scheme and its proof; and (c) the number of distinct `B` invocations that survive. Absent (a) or (b), the claim is a surrogate and must be labelled one.

---

## B.5 Revised contributions and verdict

**Superseded:** Addendum A §A.3's "Theorem N + Corollary" and §A.4 item 2. **Replaced by:** Proposition P1 (proved) and Finding P2 (descriptive), plus the explicit non-impossibility clause B.2.

**Unchanged and still binding:**

- `W-1` — `H0` admissibility withdrawn (P1 is its proof).
- `W-2` — the exact-safe-prune claim withdrawn. PRA-1's exactness dies with `W-2` regardless of B.2, because **PRA-1 used none of items 1–6**: its only certificate was unconditional-frequency-based, and it had no reuse scheme.
- `W-3` — framing-only is safe and non-informative.
- Theorem Z, narrowed to "free", never "exact".
- Incompatibility Lemma — selector bytes cannot be bought at zero decoder cost.
- Bound F — escape-inclusive, fully charged, provable.
- `POOL-1` — byte-only census, zero new Brotli calls, `{DISCOVERY}`, gated at `0.25%`/`1.0%`.

### Verdict: **HOLD** (unchanged — but the reasoning is corrected)

The verdict does not move, and it **no longer rests on any impossibility claim**. Branch R closes for these reasons only:

1. The **isolated-object** exact method already exists (`O11`); restating it is not new content.
2. The **complete-carrier** exact objective is a *different and harder* quantity that no lane has computed, and no bounded method for it has been proposed — so PRA-1 was not addressing it.
3. The **interaction residual** is already preregistered by Track 16 as `EXP-R` arm R2 with a `0.25%` kill bar; duplicating it would waste a budget.
4. `POOL-1`'s byte pool is unmeasured, and the corpus requirement of §3.1 is unmet.

**And the honest new statement of what track 12 leaves open:** items **1–4 in §B.2** — backend-aware bounds, shared/incremental evaluation, exact dominance certificates, and candidate-family structural bounds — are **unexplored in this project** and are the only constructions that could make exact search affordable on a restricted family. Track 12 does not attempt them. They are handed to Tracks 16 and 20 as *named open directions with stated obstacles*, not as closed questions.

---

*Addendum B, 2026-10-02, binding. The coordinator's second correction is accepted in full: "Theorem N" was an over-claim and is withdrawn as stated; Addendum A §A.3 is retained unedited with a supersession banner; Proposition P1 and Finding P2 replace it; the screening rule issued to Tracks 16 and 20 is retracted and replaced by §B.4. Verdict remains **HOLD** for corrected reasons. Track `12-encoder-search` closes.*