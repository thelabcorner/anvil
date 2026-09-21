# ledger-i7-cost — STAGING FILE for RESEARCH_LEDGER.md PART XII

**Scribe:** `ledger` (anvil-i7-cost). **Status: LANDED** — the PART XII section
below was appended to RESEARCH_LEDGER.md (line 3007) on 2026-08-21; this file
is retained as the staging/process record. Successor ledger rules for the
Gorilla/vXOR lane: `docs/ledger-tagging-rules.md`.

Coordination rule: this swarm's entries are staged here complete, then appended
to `RESEARCH_LEDGER.md` under `# PART XII — Iteration-7 cost-fidelity + Orbit
lane` at final consolidation ONLY, after re-reading the file tail (the
concurrent swarm writes `# PART XI`; ours goes cleanly AFTER theirs; never
rewrite or reorder existing content).

Slots marked `[PENDING — <peer>]` await that peer's measured numbers. No entry
is final until every number in it comes from a peer's actual measurement.

---

## Entry plan (6 entries)

1. **Experiment Z completion** — ARI-REF harness results (ariref). Completes
   the pre-registration at RESEARCH_LEDGER.md line 2818 (PART X). Targets
   a/b/c/d each pass/fail with exact numbers; honest framing per the
   pre-registered 'token-economics harness' limitation.
2. **S6-3 oracle prototype results** (oracle) — calibration distribution,
   ratio/encode/decode deltas, integration recommendation. Standalone harness;
   integration deferred to arch.
3. **S6-2 threshold design record** (pnra-cost) — formula, derivation, flip
   predictions, frozen held-out verdict protocol. Marked NOT YET MEASURED
   end-to-end.
4. **Corpus expansion record** (corpus-expand) — files added, provenance,
   checksums, orientation ratios.
5. **Intel patent-gate Pass 5 verdict** (priorart) — retrieved claim language;
   impact on the TCOPY narrowed claim (implicit Δ=−d, executable code only).
6. **Verification matrix summary** (verify) — independent reproduction of peer
   results.

---

# PART XII — Iteration-7 cost-fidelity + Orbit lane

*Consolidated by `ledger` (anvil-i7-cost) from `docs/ledger-i7-cost.md`;
appended after an immediate tail re-read (the concurrent swarm writes
`# PART XI`; this part lands cleanly AFTER whatever existed at write time;
existing content is never rewritten or reordered).*

**Quality seal:** every entry in this part is covered by the independent
verification matrix (`deliverable/u6` v4, `docs/verify-notes/i7-verification.md`):
all six lanes' numbers reproduce exactly or within pre-registered bands from
verify's own rebuilds, plus ariref's complementary independent pass (u6 v3
addendum + v4 resolution addendum); zero ledger contradictions; "no claim
exceeds its evidence anywhere." Four review findings were raised across the
double pass and ALL resolved before consolidation (anvil_bench.exe manifest
re-pin; ariref source-hash re-pin; oracle range-gloss rewording; S6-2
flip-count counting-basis reconciliation + one staging-file hash-suffix typo,
fixed).

The Experiment Z results entry was landed by its author (ariref) directly
below its PART X pre-registration and is cross-referenced here rather than
duplicated.

**Closing cross-reference — I7 strategy synthesis / I8 ranking (strategy,
analysis-only; NOT a claim):** `docs/swarm-i7-strategy.md` consumes all six
deliverables above plus the decode-swarm's S6-1 frozen gate and interim
measurements. Headline re-rank (all numbers from landed measurements): the
streak-breaker relocates to mode-16 ARI-REF on synth-arith.bin (EXP. Z harness
wire 89,363 B vs brotli-q11 87,013 B = +2.7%, already below zstd-19/q9 — a
falsifiable first-EXTENDS_FRONT prediction if the end-to-end container lands
≤87,013 B); the record-file frontier is measured RATIO-blocked first;
generated.json's precise open problem is −11.6% vs exactly one dominating ref
row; five dead ends are closed with measured reasons (post-parse rollback,
stream-budget-only bar A, implicit Δ default, RLZ/RePair default,
word-aligned constant-Δ off-layout); I8 DAG sequenced A–G. Standing
disclaimer: pre-registrable falsifiable tests, not claims.

## Experiment Z — ARI-REF results — ALREADY LANDED IN LEDGER (cross-reference; do not duplicate)

**Disposition:** ariref appended the complete Experiment Z RESULTS entry
directly to RESEARCH_LEDGER.md immediately after the PART X pre-registration
it completes (current file lines ~2910–3003), matching the T/U/W/X one-section
pattern (pre-reg + results together). Per the no-rewrite/no-reorder rule this
entry STAYS where it landed; PART XII carries this pointer instead of a
duplicate. (Process deviation noted: entries were supposed to stage here
first; the result is nonetheless correct, complete, and verified.)

**Verdict summary (full numbers in the ledger entry):**
- **(a) Representation/density: PASS on synth-arith.bin** — raw(exact-LZ)
  257,124 → ari-tx 89,363 = −65.25% (bar ≥10%, passed 6.5×); **FAIL on
  synth-timeseries.bin** (+0.03%; 14 B stride not ×4-alignable; per-field
  deltas outside the pre-registered word-aligned constant-Δ family).
- **(b) Implicit-Δ ablation: FAIL as pre-registered on corpus; sharp boundary**
  — implicit Δ=σ·(d/4) costs +38.35% vs transmitted on the same 2,160 tokens;
  exact progression → implicit BEATS transmitted (−15.38%); ±1 jitter →
  +40.55%. Zero-bit claim narrows to exact progressions; transmitted Δ is the
  working form and carries the (a) gain.
- **(c) No-regression/attribution: PASS** — real-corpus tx-mode ≈zero (0–72
  tokens text/structured, deltas ≤0.25%; binaries 0.02–0.35% of tokens;
  anvil_bench.exe −1.42% outlier noted NOT claimed).
- **(d) Correctness: PASS** — byte-exact round-trip 22 files × 4 modes; fuzz
  280/280 seed 0xC0FFEE; 199 single-bit mutations → 79 rejected / 1 equivalent
  / 119 detected-by-self-check / 0 crashes / 0 silent wrong-accepts.
  Coordinator interpretation note recorded (no integrity field at this layer
  by design; hardening defers to ANVIL per-block CRC).

**Process honesty:** run1 failures were two real bugs (unverified hash-bucket
hits; mode-0 demotion inflating raw baselines), fixed before any recorded
number. priorart independently rebuilt source SHA256 CFE97A36…1CFF73F and
reproduced every number exactly (corroboration record preserved verbatim in
blackboard deliverable/u1 v2/v3). Provenance re-pin (deliverable v3): FINAL
source = `ariref.cpp` SHA256 `86BA68D0…4F63A7`, `results_run3.txt` SHA256
`36548062…97E9E` — the file changed AFTER priorart's corroboration snapshot
(final mutation-accounting wording / exit-code semantics + debug-scaffolding
cleanup) with NO numeric behavioral delta, established by priorart's
CFE97A36-rebuild matching every number AND verify's fresh 86BA68D0-rebuild
producing output line-identical to results_run3.txt (71/71 lines); both hashes
valid for their stated snapshots. Recommendation per pre-reg: additive-family
representation validated via TRANSMITTED Δ; carry into a real ANVIL token type
via arch/format lanes; stronger decoder-visible σ estimator = unbuilt
follow-on. Discharges the experimental side of ledger C12 condition (1).
NO Pareto claim (token-economics harness).

## S6-3 — measured-cost rejection-oracle PROTOTYPE — built, calibrated, measured; integration recommendation recorded (oracle)

**Lineage:** EXP. F proved measured-cost parsing is the strongest ratio
mechanism but its iterative DP re-parse is DP-class encode (0.8–1.9 MB/s);
EXP. L proved measured per-stream cost can be 100% faithful (J-selection,
λ=0.01); EXP. X washed out because a local fixed-shape heuristic committed
candidates whose real entropy-coded cost exceeded their estimate. S6-3 asks
the cheapest unifying question: can ONE measured-rANS-cost rejection pass
after a greedy parse capture the fidelity at greedy-class encode cost?
Strategy-doc falsifiable targets: (a) calibration — match true entropy-coded
token cost within X% on a candidate sample; (b) end-to-end record-file
aggregate ratio Δ ≤ 0 (exact), encode ≤ 2× greedy-class, decode penalty ≤ 10%.

**Scope:** PROTOTYPE standalone harness (`prototypes/cost_oracle/`:
`cost_oracle.cpp`+exe, README with full tables, `results_m11.csv`,
`results_m14_pe.csv`, `cal_jsonl.csv`, `cal_bench.csv`). Verbatim copies from
`src/anvil.cpp` only — **no src edits** (integration deferred to arch).
Build per the Experiment V precedent (clang-cl /std:c++20 /MD /O2 /EHsc
/DNDEBUG). Correctness anchor: harness baselines reproduce the ledger's EXP. X
wire sizes BYTE-EXACTLY (anvil.exe 108,514 `pnra=on` / 108,518 off;
anvil_bench.exe 846,050 / 845,675). Round-trips verified decode==input on
every rep of every run. All numbers harness-relative (single parse family, no
container router), labeled as such.

**Results — (a) calibration (parse-time heuristic vs measured attributed
bytes, consistent units):** 473,985 sampled committed candidates across 22
file-runs, **ZERO sign flips**; median |err| ≤16% on every file; P90
+0.5%..+83% on text/binaries; one large tail (synth-timeseries P90 +2219%)
where the flat `len/8` mask term OVER-prices long clean matches — pessimistic
direction, never over-commits. **PASS.**

**Results — (b) end-to-end record-file aggregate delta:** **+0.0000% exact,
CV=0 — delta exactly 0 on all 22 file-runs. PASS (target ≤0).** The guarantee
is by construction: the decision is measured-payload arbitration over {keep-
all, drop-R1 (keep>drop), drop-R1∪R2 (type==3 && len≤8)} with variant 0 =
baseline payload — the smallest ACTUAL encoding wins, so the oracle cannot
make output larger.

**Results — (c) encode vs greedy-class:** 1.10×–1.40× on all >100 ms files
(jsonl 1.32×, log 1.38×, sqlite 1.15×, bench 1.03×, pe-git 1.10×) — **PASS
≤2×.** Outlier repeat.jsonl 2.36× is the degenerate control (940 KB → 1,182 B:
parse is ~free, so the fixed O(n) attribution pass dominates; absolute cost
30.8 ms). Honesty note: the harness baseline jsonl encode is 13.4 MB/s, so the
pre-registration's "~20 MB/s" illustration does not hold for this harness
either — the binding form of the target is the RATIO, which passes.

**Results — (d) decode penalty:** on ≥3 ms files −4.6%..+7.6% (jsonl +1.3%,
log +7.6%, bench −4.6%, pe-git +0.25%) — **PASS ≤10%.** Sub-ms files swing
±15–136% = timer noise; reported, not claimed.

**Finding F1 (measured): EXP. X's +0.0443% regression is TRAJECTORY-borne,
NOT token-borne.** On anvil_bench `pnra=on`: every committed type-3 token is
individually sound (kept tokens cost far less than their order-0-literal
alternative — type-3 median 4.45 B kept vs 43.0 B alternative, R2-class median
3.66 vs 35.75 B, per verify's exact-subset recheck of the deliverable's
rounded "2.4–4.8 vs 32–39" gloss); dropping the R2 class (1,470 tokens)
enlarges EVERY block; forcing ALL 2,377 type-3 tokens in or out still leaves
every block larger than the `pnra=off` parse (845,675). PNRA commits displace
better exact/sparse matches downstream via hash-chain insertion —
un-recoverable by ANY post-parse rollback. Independently corroborated by
pnra-cost's u5: a parse-TIME threshold (which changes the trajectory) lands
845,657 < 845,675 on the same bytes.

**Finding F2 (measured): the local heuristic is well-calibrated ON WHAT IT
COMMITS** (median ±16%, zero sign flips in 473,985 samples). EXP. X's
"underpricing" root cause is thereby REFINED: per-candidate numbers are
adequate; the failure mode is accepting candidates whose GLOBAL opportunity
cost (displacement of downstream matches) exceeds their local gain — invisible
to any per-token model, measurable only end-to-end. This sharpens the
iteration theme: cost-model fidelity has a per-token layer (calibrated, works)
and a trajectory layer (where EXP. X actually failed).

**Integration recommendation for arch (recorded, not executed):**
1. Do **NOT** integrate post-parse rollback for EXP. X recovery — measured
   impossible (F1). S6-2's parse-time threshold (pnra-cost's frozen formula)
   is the correct lever.
2. **DO** integrate the attribution machinery (~200 lines, encoder-side only,
   no decoder change) as calibration/diagnosis infrastructure — it powers
   S6-1's stream budget and S6-2's FramingRaw with measured numbers.
3. Measured-payload arbitration (try K stated-formula variants, keep the
   smallest actual encoding) is the safe integration shape for any future
   refinement pass: Δ≤0 by construction, K−1 extra O(n) encodes, no re-search,
   cannot mis-price interactions.
4. A displacement-targeting rejection pass would be DP-class (EXP. F
   territory) — out of S6-3's greedy-class scope by pre-registration.

**Verdict vs pre-registered targets: (a)(b)(c)(d) all PASS** — the oracle is
faithful where per-token models can be faithful, provably harmless end-to-end,
and cheap enough to keep; the measured negative (post-parse rollback cannot
recover EXP. X) redirects integration to the parse-time lever before arch
spends effort on the wrong shape.

## S6-2 — stated-formula acceptance threshold for PNRA/TCOPY type-3 candidates — DESIGN RECORD, calibrated, NOT YET MEASURED end-to-end (pnra-cost)

**Status (read this first):** the threshold formula is FROZEN and calibrated
on the pinned PEs ONLY. **The end-to-end verdict is NOT YET MEASURED** — it
belongs to arch's integrated build running the frozen protocol below on
corpus-expand's held-out PE set. Nothing here is a victory claim; the
designer's own pre-measurement prediction is that the held-out verdict will
FAIL the pre-registered bar (see Flip predictions).

**Lineage:** EXP. X's recorded remainder verbatim — "a length- or
distance-aware minimum-gain threshold specific to single-window transform
candidates, rather than reusing the general-purpose sparse-candidate cost
formula verbatim" — plus the explicit warning against overfitting hand-tuned
constants to two pinned PEs. Full contract: `docs/s62-threshold-formula.md`
(integration contract for arch + pre-registration text).

**Provenance (measured, exact):** calibration used ONLY `anvil.exe`
(`09b9b0cc…`) + `anvil_bench.exe` (`fcd30da5…`). The prototype
(`prototypes/pnra_cost/anvil_pnra_proto.cpp`) is a patched COPY of
`src/anvil.cpp` — `src/anvil.cpp` itself never modified; with no env vars set
the copy reproduces EXP. X bit-for-bit, and the working tree's +230-line
RLZ-RePair delta was verified by diff to not touch the PNRA acceptance path.
EXP. X's counter chain reproduces EXACTLY on current on-disk bytes (anvil.exe
idxhit 713 / verify 615 / commit 429, out=108,514 — gate 1329 vs ledger 1333,
4 gate fires producing no index hit; bench gate 6919 / idxhit 2145 / verify
1958 / commit 1599, out=846,050 — all exact). **This settles the
`anvil_bench.exe` manifest-drift question empirically: EXP. X was measured on
the current bytes; the drift predates EXP. X**, so calibration here and ledger
numbers there are on identical data.

**What is being fixed — EXP. X's root cause, refined by new measurement into
TWO components:**

1. **Framing underpriced** (the ledger's finding): the per-token fixed wire
   cost (type symbol + ml varint + ds varint + residual-mask words +
   transform-mask words) is only partially represented in
   `1.5 + varint_cost(L−4) + varint_cost(D−1) + L/8 + 0.18·log2(D+1)` — e.g.
   the tmask stream is not priced at all.
2. **Alternative overpriced (NEW, this calibration):** the formula's `alt_c`
   prices the whole candidate span as literals when no exact match exists at
   the anchor — but a verified PNRA candidate's non-field bytes byte-match at
   distance D *by construction*, and the re-search recovers them with an exact
   match anchored just past the field (same distance). Counterfactual
   measurement: Σ(real marginal cost vs naive literal-splice) over commits =
   +1,486 B (anvil.exe) / +2,195 B (bench), while the true end-to-end effect
   is −4 B / +375 B — the parser re-discovers most of the span, so the
   formula's implied saving `S = alt_c − pnra_c` is systematically inflated.

Consequence: a minimum-gain threshold on S is the right lever (it cannot
repair S's scale, but it can require S to dominate the fixed framing, which is
where the modal mispriced shape lives), and the threshold MUST scale with
shape — flat margins measurably fail.

**The frozen formula (integration contract for arch):** for a verified PNRA
type-3 candidate c with span L, distance D, F transform fields, R residual
corrections:

```
S(c) = alt_c − pnra_c                                        [bits; both already
                                                              computed by the
                                                              existing code path]
FramingRaw(L,D,F,R) = 8·1 + 8·vb(L−4) + 8·vb(D−1)
                    + 32·⌈L/32⌉ + 32·⌈⌈L/4⌉/32⌉ + 8·R        [raw fixed wire bits:
                                                              type byte + ml varint
                                                              + ds varint + mask
                                                              words + residuals]

Accept iff  S(c) ≥ γ · FramingRaw(L,D,F,R),  γ = 0.5 (FROZEN).
```

- γ is the ONLY constant — not per-file, per-shape, or per-block tuned.
- The rule is monotone: any candidate accepted under γ=0.5 would also have
  been accepted under the legacy S>0 rule, so flips are one-way
  (commit → reject) and the E8/E9-gated no-op guarantee on non-PE files is
  structurally preserved (verified byte-identical).
- Interpretation: the implied saving must cover ~the ENTROPY-CODED framing
  cost. Measured compressed/raw stream ratios (bench diagnostics): masks
  0.435, tmask 0.153, types 0.169, ml 0.59, ds 0.93 — blended ≈0.3–0.6 of raw;
  γ=0.5 is the midpoint of that measured band, i.e. the literal-amortization
  condition named in the pre-registration, with the margin being the
  raw-vs-coded pricing slack.

**Derivation (why this form and these constants):**

- *The margin must scale with shape (data, not taste).* End-to-end sweep on
  the pinned PEs (`off` = 108,518 / 845,675): legacy S>0 → 108,514 / 846,050
  (bench regresses); flat G0=20 → 108,514 / 845,688 (bench still ≥ off); flat
  G0=40 → both < off only at near-total rejection (33–90 commits kept);
  dist-aware G0+G1·8·vb(D−1) → distance term adds ≤2 B over flat at same
  commit count (87% of commits at D<4K) → dropped; **γ·FramingRaw γ=0.5 →
  108,506 / 845,657, both < off at 29/80 commits kept.** A flat bar cannot
  separate the modal mispriced shape (L=4–8, tiny S) from legitimately long
  candidates; the framing-proportional bar does, because FramingRaw grows with
  L while the mispriced class does not.
- *Why γ=0.5:* the measured coded/raw framing-ratio band midpoint, making the
  rule self-describing rather than a swept optimum. The sweep bracket
  [0.4, 0.6] both beat `off` on both PEs (γ=0.4: 108,503/845,674; γ=0.6:
  108,509/845,666); 0.5 was frozen BEFORE any held-out data exists, and no
  re-tuning is permitted after.
- *Rejected alternatives, with reasons:* flat margin (fails bench at any
  principled constant); distance-aware term (no measured leverage); correcting
  `alt_c` itself to price the shifted-anchor exact alternative (mechanistically
  cleaner but changes the shared comparison — larger integration surface; the
  counterfactual data show the threshold achieves the same rejection profile
  with a one-line rule; recorded as the S6-3 oracle's natural follow-up).

**Calibration results at the frozen setting (pinned PEs ONLY):**

| metric | anvil.exe | anvil_bench.exe |
|---|---:|---:|
| `--pnra=off` | 108,518 | 845,675 |
| legacy rule (EXP. X) | 108,514 (−0.0037%) | 846,050 (+0.0443%) |
| **frozen γ=0.5** | **108,506 (−0.0110% vs off)** | **845,657 (−0.0021% vs off)** |
| commits legacy → frozen | 429 → 29 | 1,599 → 80 |
| round-trip (decode SHA-256) | PASS | PASS |
| non-PE no-op (generated.json, legacy vs frozen) | byte-identical | — |

First configuration in this lane's history to beat `--pnra=off` on BOTH pinned
PEs simultaneously — by a hair, which is itself the honest headline.

**Flip predictions vs the legacy formula (measured on calibration PEs;
counting bases explicit — reconciled after ariref/ledger review):** two
counting bases exist and must not be read as complements of each other:

- *Counter basis* (`pnra_commit=`): committed TOKENS in the full end-to-end
  parse under each rule — 429 → 29 (anvil.exe), 1,599 → 80 (bench).
- *Record basis* (candidate dumps): per-verified-candidate rows. Because
  rejecting a commit changes the downstream parse trajectory, the frozen-rule
  run verifies a slightly DIFFERENT set of candidate sites than the legacy
  run; flip analysis joins only the INTERSECTION of sites (605 of 615/614 on
  anvil.exe; 1,932 of 1,958/1,956 on bench).

The one-way monotonicity claim ("any γ=0.5 accept would have been a legacy
accept") holds at identical local state, i.e. PER SITE — not across diverged
trajectories. Full accounting (asserted programmatically,
`tools/pnra_cost_analysis/reconcile.py`):

| file | legacy commits | = flips + kept + traj-lost | frozen commits | = common kept + traj-new | counter Δ = flips + lost − new |
|---|---:|---:|---:|---:|---:|
| anvil.exe | 429 | 391 + 28 + 10 | 29 | 28 + 1 | 400 = 391+10−1 ✓ |
| anvil_bench.exe | 1,599 | 1,502 + 78 + 19 | 80 | 78 + 2 | 1,519 = 1,502+19−2 ✓ |

The commit-counter deltas additionally absorb second-order trajectory shift —
rejected candidates change downstream gate/idxhit/verify outcomes (anvil.exe
gate 1329→1334, idxhit 713→711, verify 615→614; bench 6919→6917, 2145→2140,
1958→1956) — netting −9 / −17 beyond direct flips (trajectory-lost minus
trajectory-new sites: 10−1 / 19−2). One-way monotonicity holds per-site at
identical local state, not across diverged trajectories — consistent with
deliverable/u2 Finding F1 (the regression is trajectory-borne).

Flip shape profile (record basis, common sites): anvil.exe — L=4: 186, L=5:
96, L=6: 45, L=7: 27, L=8: 22, L=9–19: 15 → **95.6% of flips have L≤8**;
kept sites median L=18, 22/28 at D<4K. bench — L=4: 909, L=5: 267, L=6: 192,
L=7: 46, L=8: 41, L=9–19: 47 → **96.9% have L≤8**; kept median L=16, 62/78 at
D<4K. This is exactly EXP. X's "dominated by minimal, one-window matches"
root-cause class: the rule removes the short-single-field far-distance
over-commit mass and retains long, amortized candidates.

**Honest pre-measurement prediction (recorded BEFORE any held-out datum):**
calibration magnitude is −0.002%…−0.03% per PE — **two orders of magnitude
below the pre-registered ≥0.5% held-out bar**. Unless held-out PEs carry
materially denser relocation structure than the pinned pair, the honest
prediction is **VERDICT: FAIL (clean negative)** — the threshold fixes the
measured mispricing, but the mechanism's ceiling on real PE data appears far
smaller than the bar.

**FROZEN VERDICT PROTOCOL (verbatim from `docs/s62-threshold-formula.md` §8;
frozen 2026-08-21 before any held-out measurement):**

> **Mechanism under test.** Mode-14 (`--parse=tcopy`) PNRA candidate source
> (`--pnra=on`) with the stated-formula acceptance threshold replacing the
> reused general-purpose comparison for PNRA candidates ONLY:
> `accept iff (alt_c − pnra_c) ≥ 0.5 · FramingRaw(L,D,F,R)` with FramingRaw
> and γ=0.5 exactly as specified in `docs/s62-threshold-formula.md` §3.
> The formula and γ are FROZEN; no parameter may be changed after any
> held-out datum is observed.
>
> **Calibration/holdout split (binding).** Calibration used ONLY
> `tests/corpus/anvil.exe` (sha `09b9b0cc…`) and `tests/corpus/anvil_bench.exe`
> (sha `fcd30da5…`). The verdict set is EXCLUSIVELY the held-out PE set added
> by corpus-expand (`pe-winver.exe`, `pe-where.exe`, `pe-notepad.exe`,
> `pe-python.exe`, `pe-ninja.exe`, `pe-git.exe` — 6 binaries, 5 distinct
> producers/toolchains, all distinct from the calibration pair).
>
> **Gates before measurement (standing protocol).** Round-trip verify on all
> corpus files at `--parse=tcopy` with `--pnra=on` (integrated build);
> `tests/fuzz.py` including the `tcopy/rans --pnra=on` combo, ≥120 cases;
> only then measure.
>
> **Measurement.** Integrated build (arch). Per held-out PE and aggregate:
> compressed bytes at `--pnra=on` vs `--pnra=off`, median of 3 reps (ratio
> CV = 0.000% — byte counts are exact); decode MB/s via `anvil_bench`
> median-3 on held-out PEs ≥ 100 KB (decode co-arbiter, FLAG-A). Record
> `pnra_{gate,idxhit,verify,commit}` counters per file as diagnostics.
>
> **PASS requires ALL of:**
> 1. held-out PE aggregate: `on` ≤ 0.995 × `off` (≥ 0.5% smaller);
> 2. decode on no held-out PE < 0.9 × `off` (≤ 10% regression);
> 3. every non-PE corpus file byte-identical between `on` and `off`
>    (zero regression, EXP. X gating guarantee);
> 4. round-trip + fuzz gates green.
>
> **Failure handling.** Any unmet condition ⇒ verdict FAIL, recorded as a
> clean negative (root-cause hypothesis wrong on held-out data, or ceiling
> too small — calibration already predicts the latter, §6). NO re-tuning of
> γ or the formula against held-out results; any future attempt requires a
> NEW pre-registration with a different mechanism or a re-derived bar.
> Per-file results are reported regardless of aggregate outcome.

**Cross-corroboration addendum (contract doc §7a, after oracle's u2 landed):**
the S6-3 oracle independently characterized the same regression and the three
datasets are mutually consistent — every committed PNRA token is individually
sound; post-hoc dropping any subset from the fixed on-parse trajectory
enlarges every block; yet the `--pnra=off` parse (a DIFFERENT trajectory) is
375 B smaller on bench. This re-explains u5's counterfactual sign pattern
(the splice-CF measured fallback cost WITHIN the on-trajectory — exactly the
quantity the oracle shows is always unfavorable). **Consequence for the frozen
formula's INTERPRETATION (not its validity):** γ·FramingRaw works because it
is a PARSE-TIME decision — rejecting at the candidate site lets the ordinary
search consume the span, changing the trajectory (845,657 < 845,675). The bar
should be read as "local implied saving must be large enough (scaled by fixed
framing) to plausibly dominate the candidate's displacement footprint", not as
repairing per-token mispricing. Post-parse rollback CANNOT recover EXP. X
(oracle, measured); S6-2's parse-time rule is the only lever, as
pre-registered.

**What the calibration does NOT establish:** no held-out file was touched;
counterfactual per-candidate costs are vs a naive literal splice, not vs the
true re-parse (used for structure only — all constants trace to end-to-end
sweeps, which capture re-search exactly); second-order parse interactions
differ between threshold settings and only aggregate sizes are claimed.

**Artifacts:** `prototypes/pnra_cost/anvil_pnra_proto.{cpp,exe}` (env knobs
`PNRA_MODE/FORM/G0/G1/GAMMA/DUMP/CF`; defaults = exact EXP. X clone;
`PNRA_MODE=1 PNRA_FORM=1 PNRA_GAMMA=0.5` reproduces every number above);
`tools/pnra_cost_analysis/{analyze.py,flips.py,sweep.ps1}`; `scratch/pnra-cost/`
(candidate CSVs, sweeps, frozen outputs + logs).

## Corpus expansion — held-out PE set + structure-carrying files (corpus-expand, I7)

**Purpose (pre-stated by the strategy doc and EXPs T/U/W):** (a) a held-out PE
set so S6-2's acceptance-threshold verdict is measured on binaries DISTINCT
from the calibration pair — the anti-overfit guard Experiment X explicitly
demanded ("without a larger PE corpus to validate against"); (b)
structure-carrying files so future structural/invariant mechanisms can be
valued honestly — T/U/W twice measured the existing corpus's
periodic/arithmetic signal at or below the hash-noise floor (0.14–1.1%
finite-difference hits).

**Added — held-out PE set (6 real x64 PEs, 5,540,960 B total; all MZ/PE
machine=x64 verified; all distinct from the pinned `anvil.exe` /
`anvil_bench.exe` pair):**

| file | size | provenance | SHA-256 (prefix) |
|---|---:|---|---|
| pe-winver.exe | 28,672 | `%SystemRoot%\System32\winver.exe` "Version Reporter Applet", Windows 22H2 system binary (MSVC), v10.0.22621.1 | `d1d050ef…` |
| pe-where.exe | 61,440 | `System32\where.exe`, Windows 22H2 system binary (MSVC), v10.0.22621.1 | `ade557dd…` |
| pe-notepad.exe | 360,448 | `System32\notepad.exe`, Windows 22H2 system binary (MSVC), v10.0.22621.1 | `49f096cb…` |
| pe-python.exe | 103,704 | `C:\Program Files\Python312\python.exe`, official python.org CPython build MSC v.1940 x64, v3.12.4 | `fd5c46d7…` |
| pe-ninja.exe | 603,648 | winget Ninja-build.Ninja release binary (MSVC), `ninja --version` = 1.13.2 | `e52a7ad9…` |
| pe-git.exe | 4,383,048 | `C:\Program Files\Git\mingw64\bin\git.exe`, Git for Windows v2.55.0.windows.3 (MinGW-w64/GCC — the only non-MSVC PE in the corpus) | `1a004355…` |

Full hashes in `tests/corpus/CHECKSUMS.txt`; provenance/version detail in
`tests/corpus/README.md`. Diversity: 4 producers (Microsoft OS, python.org,
ninja-build, Git-for-Windows), 2 toolchain families (MSVC, GCC), sizes 28 KB –
4.3 MB. Binaries are pinned by exact bytes (host-installed binaries are not
bit-reproducible; integrity = SHA-256, not rebuild). The ≥4–6 held-out
contract is satisfied at 6.

**Added — structure-carrying files (`tests/make_synth_corpus.py` extended;
fixed seeds 90004–6; integer-only math ⇒ byte-stable across Python
versions/platforms; determinism verified by double-run hash compare):**

| file | size (seed) | structure | SHA-256 (prefix) |
|---|---|---|---|
| synth-columnar-align.bin | 276,000 (90004) | columnar arithmetic table stored ROW-interleaved, every field misaligned (prime 23 B row stride). Fields: u16 row_id Δ=1, u32 seq Δ=7, u64 ts_ns Δ=1e6, u32 temp_x100 Δ=3±1, u16 volt Δ=2, u8 status mod-8, u8 flags rare-flip, u8 parity = XOR of preceding field bytes (cross-field linear structure). Deliberately harder than `synth-arith.bin`'s block-wise columns: a mechanism must find TRUE field offsets. | `a42ea6b4…` |
| synth-drift-stride.bin | 615,376 (90005) | 40 B structured records (u64 ts Δ=500, u32 counter, u16 channel mod-64, 24 B slow-tick skeleton, u16 CRC16) at SYSTEMATICALLY drifting stride: `pad=(i//96)%9` sawtooth, period ramps 40→48 B, snaps back every 864 records. Deterministic-drift counterpart to `synth-jitter.bin`'s random jitter. | `ea580de5…` |
| synth-counters.log | 1,107,326 (90006) | counter/timestamp-heavy TEXT log, several simultaneous monotonic sequences per line: ISO ts +37 ms, seq +1, tick +1000, hex addr page-step +0x40 across 8 modules, bounded int random-walk latency, cyclic qd, deterministic crc. Strong arithmetic/periodic signal in TEXT form — none existed before. | `df0a9d9b…` |

**Integrity (measured, including one honest adverse finding):**

- All 12 pre-existing data files verified BYTE-IDENTICAL to their original
  2026-08-13 manifest hashes (checked explicitly pre- and post-work). The 3
  original synth files regenerate byte-identical from the extended script.
- New manifest verifies 21/21 data files OK (README verifier snippet, fixed to
  exclude meta files).
- **Pre-existing drift found and documented:** on-disk `anvil_bench.exe`
  (1,929,216 B, sha `fcd30da5…`) already mismatched its 2026-08-13 manifest
  entry (1,866,752 B, sha `e5a0f8a7…`) BEFORE this work — a re-snapshot had
  been taken without updating CHECKSUMS.txt; the file is git-ignored so the
  old bytes are unrecoverable. The file was NOT modified by this task; the
  manifest line was re-pinned to disk truth and a README "Manifest drift note"
  warned S6-2 calibration owners to confirm which `anvil_bench.exe` bytes
  their calibration used. **[Resolved same day:]** pnra-cost's u5 calibration
  reproduced EXP. X's counter chain EXACTLY on the current bytes
  (`fcd30da5…`), proving EXP. X itself was measured on current bytes — the
  drift predates EXP. X, and all recent lane numbers are on identical data
  (see the S6-2 entry below; verify independently corroborated that every
  checked claim reproduces against current bytes).

**Orientation ratios (NOT benchmark rows):** round-trip c→d→byte-compare PASS
on all new files via the current `build\anvil.exe` default settings (codec sha
`d7b02b2f…`, built 2026-08-21 04:44 — newer than the pinned snapshot, an arch
rebuild): pe-winver 0.1949 | pe-where 0.3640 | pe-python 0.5228 | pe-ninja
0.4859 | pe-notepad 0.5644 | pe-git 0.4863 | synth-columnar-align 0.4279 |
synth-drift-stride 0.2339 | synth-counters.log 0.1147. The PE ratio spread
(0.19–0.56 across toolchains/producers) makes the held-out set a genuine
verdict substrate rather than two near-identical pinned anchors.

**Handoff state:** total corpus now 22 data files ≈19.8 MB (19,811,989 B
measured on disk; +7.54 MB added, under the 30 MB budget) — count corrected
from u3's handoff text ("19 data files"), which omitted the 3 new synth files;
verified independently by `verify`. Files touched: 9 new files under
`tests/corpus/`, `CHECKSUMS.txt`, `README.md`, `make_synth_corpus.py` — no
peer-lane files. S6-2's frozen verdict protocol can now run unmodified:
measure `--pnra=on` vs `--pnra=off` aggregate on exactly these 6 `pe-*` files
at median-3, calibration stays on the pinned pair ONLY.

## Intel patent-gate Pass 5 — US 7,111,148 / US 7,010,665 full-text claims review — GATE CLOSED, TCOPY CLAIM STANDS (priorart)

**The binding item.** Pass 4 (13 Aug 2026) left exactly one unresolved
full-text gap: Intel US 7,111,148 B1 "Method and apparatus for compressing
relative addresses" / US 7,010,665 B1 "...decompressing relative addresses"
(prio 27-Jun-2002) — "titles close enough that the family CANNOT be waved
away; full text/claims not retrievable that session. REQUIRED: manual
full-text review before the executable-specific novelty boundary is treated as
closed." This pass retrieves the claims in full and discharges that condition.

**Retrieval record:** all 39 claims of '148 and all 34 claims of '665
retrieved VERBATIM from two independent sources (Google Patents +
FreePatentsOnline; texts match); continuation US 7,617,382 B1 (32 claims)
retrieved via FPO. Legal status (Google Patents): both '148 and '665 EXPIRED —
fee-related, adjusted expiration 2022-07-07 / 2023-03-25. Classifications:
G06F9/26, G06F12/02; USPC 711/220 — processor artifacts, NOT compression
classes.

**What the family actually is (verbatim claim language):** CPU
microarchitecture — bit-width compaction of RIP-relative address operands of
decoded MICRO-OPERATIONS inside on-die micro-operation storage (trace cache /
pipeline FIFO / scheduling queue / reorder buffer), reconstructed at execution
time from a per-storage-line STORED head instruction pointer plus a
TRANSMITTED correction field. '148 claim 1: "decoding a first instruction with
a K-bit displacement data to identify a first micro-operation; adding an
address of a second instruction to the K-bit displacement to generate an N-bit
relative address; compressing the N-bit relative address to generate an M-bit
immediate data; and storing the M-bit immediate data at one or more storage
locations associated with the first micro-operation." '148 cl. 9–10: the
M-bit immediate "comprises a J-bit correction field", J = 2. '665 claim 1:
"reconstruct the N-bit address by combining at least a first portion of an
instruction pointer address for the first location and the M-bit
representation of the N-bit address." '665 cl. 13: storage "to associate with
a second instruction pointer address different from the first instruction
pointer address."

**Four-question analysis (the binding questions):**

1. **Single-file self-referential compression?** NO — no file/stream, no LZ
   parse, no dictionary, no backreference, no copy primitive, no literals.
   The "compression" is runtime bit-width compaction of already-computed
   address operands inside processor storage.
2. **Distance-derived implicit Δ=−d (zero transmitted bits)?** NO — no match
   distance exists anywhere in the 105 claims because there are no matches.
   The reconstruction input is (a) an explicitly STORED per-line head
   instruction pointer and (b) a TRANSMITTED 2-bit correction field — never
   the copy distance. Honest nuance recorded: recovering high-order bits from
   locally-available context is a conceptual echo of "derive part of the
   value from decoder-visible state," but the derivation input is
   stored/transmitted metadata, never d.
3. **Executable-specific vs general-purpose?** NEITHER — CPU-runtime-specific
   (post-decode μop storage width), upstream of any file-format concern.
4. **Pre-LZ global filter vs match-level transformed copy?** NEITHER — no LZ
   layer exists in any claim; the BCJ boundary discussion is not engaged.

**Verdict: TCOPY CLAIM STANDS.** The Intel family does not anticipate, and
does not render obvious, the narrowed TCOPY mechanism — a single-file,
self-referential LZ-style transformed copy whose additive transform parameter
is derived implicitly from the match distance (Δ=−d, zero transmitted
parameter bits) in executable code. The four-pass fear (same words, unknown
art) is resolved: same title words, different art. Ledger C12 condition (2)
DISCHARGED; condition (1) (explicit-Δ control ablation) remains with the
experimental lanes. Formal FTO attorney review remains recommended before any
commercial claim (classification gate record, not legal advice).

**Secondary Pass-4 gaps also closed (with two record corrections):**

- **US 6,466,999 B1 (Microsoft) — full claims retrieved (32), gap CLOSED,
  Pass-1 description CORRECTED.** Actual title: "Preprocessing a reference
  data stream for patch generation and compression" (prio 31-Mar-1999;
  expired-lifetime) — not Pass 1's "iterator + symbol information" gloss
  (symbol tables appear only as one of several cross-referencing sources,
  cl. 16). Claim 1 requires a reference stream "known to exist on the
  destination computer" and claim 2 transmits "preprocessor-driving
  information" alongside the compressed stream: definitively TWO-FILE with
  TRANSMITTED directives. Not single-file, not implicit, not match-level.
- **Qualcomm dedicated query — RUN, decisive, attribution CORRECTED.** Exactly
  one hit: US 7,676,506 B2 "Differential file compression of software image
  versions" (claims 1–19 retrieved). The FPO record shows Assignee = Innopath
  Software, Inc.; QUALCOMM appears only as attorney/agent firm — the swarm
  record's Qualcomm attribution rested on that field. Claims: two-file
  version-pair delta with transformation G(x)=x+f(x) (piece-wise constant f)
  applied as TRANSMITTED CFD hint data. Close-but-different confirmed at
  claims level.
- **IBM dedicated query — RUN, no qualifying art.** 16 hits, none teaching the
  mechanism; closest is the Ajtai-lineage "efficient data searching, storage
  and reduction" family (US 8,275,755 / 8,275,756 / 8,275,782 / 9,378,211 /
  9,400,796 / 9,430,486 / 10,282,257 / 10,649,854): repository
  similarity-search then delta encoding — repository/two-party, not
  self-referential per-match transform.
- **Apple dyld chained-fixups patent number — NOT CONFIRMED (honest
  negative).** FPO full-text: "fixup chains" / "chained fixups" / "fixup
  chain" → 0 hits across US/EP/JP/PCT; Google Patents search rate-limited
  (503) mid-session. Classification unchanged (loader metadata,
  close-but-different). Caveat recorded: phrase absence in FPO full text is
  NOT proof that no Apple patent exists — claims could use different wording.
  Status: UNRESOLVED-NUMBER.

**Residual gaps (honest):** Apple patent number unconfirmed; '382 expiration
not independently verified; Espacenet/lens.org not directly queried (two
verbatim-matched full-text sources used instead); non-patent art unchanged
from passes 1–4 (VCDIFF, zdelta/vdelta, GenCompress, RLZAP, ZPAQ/PCOMP, BCJ,
Courgette/Zucchini).

**Five-pass convergence (final):** passes 1–4 (independent) + pass 5
(full-text claims, binding item) agree — the closest art is (a) global
once-per-file relocation/branch normalization (BCJ/Philips lineage), (b)
two-file copy-with-edits delta (Microsoft '999/'506-lineage, VCDIFF,
Zucchini), (c) hardware runtime address compaction (Intel '148/'665/'382, now
read in full), or (d) loader metadata encoding (Apple chained fixups). **No
accessible prior art teaches the single-file self-referential LZ copy with an
implicit distance-derived additive transform (Δ=−d) at match level.** The
TCOPY novelty claim's patent-classification condition is CLOSED; remaining
gate conditions are the experimental ones recorded in ledger C12. Full record:
`docs/priorart-tcopy-external.md` §"External Research Pass 5".

## Independent verification matrix (verify) — FINAL: LEDGER INTEGRITY CONFIRMED

**Method:** separate `build-verify/` tree (clang-cl 22.1.8 Release, Ninja);
verify's OWN rebuilds of every peer harness from source; re-runs from recorded
commands. Full detail: `docs/verify-notes/i7-verification.md`.

**1. Tree health:** build OK (1 benign getenv warning); `tests/fuzz.py
--cases 50` PASS, seed=41246, 420 roundtrip variants, 2520 mutation checks —
exact precedent match; 12/12 round-trips; cross-build (`build\` vs
`build-verify\`) outputs BYTE-IDENTICAL.

**2. Published numbers — 10/10 BYTE-EXACT:** EXP. S jsonl ctx-on 183,506 /
ctx-off 218,553 (Δ −16.03% recomputed), json ctx-on 89,158; EXP. L jsonl
suite-off 219,042; EXP. X tcopy anvil.exe 108,518→108,514, bench
845,675→846,050 (+0.0443%).

**3. Peer reproductions (all six lanes):**

- **u1 ariref (ARI-REF):** verify's rebuild reproduces `results_run3.txt`
  LINE-IDENTICAL (71/71): synth-arith −65.25% PASS / timeseries +0.03% FAIL;
  impl-vs-tx +38.35% (boundary −15.38%/+40.55%); real-corpus ≈0 tokens; fuzz
  280/280 seed-deterministic (identical mutation signatures), mutations
  79/1/119/0. Provenance nit FOUND+RESOLVED: source hash drifted
  post-corroboration (CFE97A36→86BA68D0, behavior-preserving); ariref re-pinned
  deliverable v3; both hashes verified on disk by verify.
- **u2 oracle (S6-3):** calibration rows EXACT on 4 files (~149k zero-flip
  samples); aggregate 517,885 B reconstructed exactly; F1 trajectory evidence
  exact (845,675 off / +375 B / R2pop 1470 / drop-variants never win); ZERO
  flips in the full 73,586-row dump. One wording nit (the "keep 2.4–4.8 B vs
  32–39 B" central-range gloss was a len-4-subset rounding, not a full-subset
  statistic) — flagged, then RESOLVED in deliverable v2 with exact medians
  (type-3 4.45/43.0; R2 3.66/35.75); soundness claim unaffected and
  independently confirmed.
- **u5 pnra-cost (S6-2):** calibration matrix EXACT from verify's own proto
  rebuild (baseline clone 108,514/846,050 + counters 713/615/429 and
  6919/2145/1958/1599; γ=0.5 → 108,506/845,657, commits 29/80; deltas
  −0.0110%/−0.0021% arithmetic verified); round-trips PASS; non-PE
  byte-identical no-op. First-config-to-beat-off-on-both confirmed. Held-out
  FAIL prediction properly pre-recorded.
- **u3 corpus-expand:** orientation ratios EXACT (pe-winver 0.1949, pe-git
  0.4863, counters.log 0.1147); sizes + SHA prefixes cross-checked;
  `anvil_bench.exe` manifest drift INDEPENDENTLY DETECTED pre-handoff,
  dispositioned by re-pin; 22 data files / 19,811,989 B measured (the staged
  "19 files" count nit → fixed by ledger).
- **u4 priorart:** US 7,111,148 claim-1 + cl.9-10 quotes VERBATIM-VERIFIED via
  independent FPO fetch; metadata matches; no-LZ characterization confirmed
  against all 39 claims; expiration status single-sourced (Google Patents only
  — noted as a caveat, not a defect).

**4. Rigor review of the staged PART XII + the landed Experiment Z entry:**
verdicts match pre-registered bars; FAILs recorded with mechanisms; outliers
explicitly not claimed (anvil_bench −1.42%); controls visibly ran in every
harness; harness-relative labeling present. **No claim exceeds its evidence
anywhere.**

**Verdict: no contradictions with the ledger anywhere; all six lanes' numbers
reproduce exactly or within pre-registered bands. Ledger integrity CONFIRMED.**
Four review findings across the double pass (verify + ariref's complementary
addendum, deliverable/u6 v4), ALL RESOLVED before consolidation:

1. `anvil_bench.exe` manifest drift — independently detected pre-handoff,
   dispositioned by corpus-expand's re-pin + pnra-cost's empirical provenance
   proof (EXP. X counters reproduce on current bytes).
2. ariref source-hash drift post-corroboration — behavior-preserving;
   deliverable re-pinned to v3, both hashes verified on disk.
3. oracle central-range gloss — len-4-subset rounding, not a full-subset
   statistic; reworded with exact medians in deliverable v2 (type-3
   4.45/43.0; R2 3.66/35.75); soundness unaffected.
4. S6-2 flip-count / commit-count mismatch — counting-basis mismatch (record
   vs counter); reconciled in u5 v3 §6 with a programmatically-asserted exact
   partition (`reconcile.py`); plus one staging-file hash-suffix typo,
   real at flag time, fixed in this staging file same day (per the u6 v4
   resolution addendum's timeline evidence).
