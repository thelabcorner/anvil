# S6-2 — Stated-Formula Acceptance Threshold for Type-3 (PNRA/TCOPY) Candidates

Author: `pnra-cost` (acceptance-threshold theorist). Status: **calibration
complete on the pinned PEs; NOT a claim.** The verdict is reserved for the
held-out PE set per the frozen protocol in §8. This document is the
integration contract for `arch` and the pre-registration text for the ledger.

---

## 1. Provenance (measured, exact)

Calibration files (pinned PEs, and ONLY these were used for any constant):

| file | size | sha-256 (prefix) | EXP. X chain reproduced by prototype |
|---|---:|---|---|
| `tests/corpus/anvil.exe` | 268,800 | `09b9b0cc…` | idxhit 713 / verify 615 / commit 429, out=108,514 — exact; gate 1329 vs ledger 1333 (4 gate fires produced no index hit; candidate pipeline identical) |
| `tests/corpus/anvil_bench.exe` | 1,929,216 | `fcd30da5…` | gate 6919 / idxhit 2145 / verify 1958 / commit 1599, out=846,050 — **all exact** |

The `anvil_bench.exe` manifest-drift question (corpus-expand, 2026-08-21) is
thereby settled **empirically**: Experiment X's counters were measured on the
current on-disk bytes (`fcd30da5…`); the drift predates EXP. X. Calibration
here and ledger numbers there are on identical data.

Instrumentation: `prototypes/pnra_cost/anvil_pnra_proto.cpp` — a patched COPY
of `src/anvil.cpp` (working tree @ 2026-08-21; the +230-line RLZ-RePair delta
touches only hotop stream codecs and diagnostics, NOT the PNRA acceptance
path — verified by diff). `src/anvil.cpp` itself was never modified. With no
env vars set the copy reproduces EXP. X bit-for-bit (sizes above). Analysis
scripts: `tools/pnra_cost_analysis/{analyze.py,flips.py,sweep.ps1}`.

## 2. What is being fixed (EXP. X root cause, refined by measurement)

EXP. X verdict: PNRA's invariant search fires and verifies real candidates
the byte-hash chain cannot reach, but the reused general-purpose cost formula
commits candidates whose real entropy-coded cost exceeds the estimate —
wash-to-regression (−0.0037% / +0.0443%). Two measured components:

1. **Framing underpriced** (ledger root cause): the per-token fixed wire cost
   (type symbol + ml varint + ds varint + residual-mask words +
   transform-mask words) is only partially represented in
   `1.5 + varint_cost(L−4) + varint_cost(D−1) + L/8 + 0.18·log2(D+1)` —
   e.g. the tmask stream is not priced at all.
2. **Alternative overpriced** (NEW, this calibration): the formula's `alt_c`
   prices the whole candidate span as literals when no exact match exists at
   the anchor. But a verified PNRA candidate's non-field bytes byte-match at
   distance D *by construction* — the re-search recovers them with an exact
   match anchored just past the field (same distance). Counterfactual
   measurement: Σ(real marginal cost vs naive literal-splice) over commits =
   +1,486 B (anvil.exe) / +2,195 B (bench), while the true end-to-end effect
   is −4 B / +375 B — the parser re-discovers most of the span, so the
   formula's implied saving `S = alt_c − pnra_c` is systematically inflated.

Consequence: a **minimum-gain threshold on S** is the right lever (it cannot
repair S's scale, but it can require S to dominate the fixed framing, which
is where the modal mispriced shape lives), and the threshold MUST scale with
shape — flat margins fail (§4).

## 3. The frozen formula (integration contract)

For a verified PNRA type-3 candidate c with span L, distance D, F transform
fields, R residual corrections, let

```
S(c)          = alt_c − pnra_c                      [bits; both already computed
                                                     by the existing code path]
vb(x)         = varint byte count of x
FramingRaw(L,D,F,R) = 8·1 + 8·vb(L−4) + 8·vb(D−1)
                    + 32·⌈L/32⌉ + 32·⌈⌈L/4⌉/32⌉ + 8·R
                    [raw fixed wire bits: type byte + ml varint + ds varint
                     + residual-mask words + transform-mask words + residuals]
```

**Accept iff  S(c) ≥ γ · FramingRaw(L,D,F,R),  with γ = 0.5 (frozen).**

- γ is the ONLY constant. It is not per-file, per-shape, or per-block tuned.
- The rule is monotone: any candidate accepted under γ=0.5 would also have
  been accepted under the legacy rule with S>0 (since FramingRaw ≥ 0), so
  flips are one-way (commit → reject) and the E8/E9-gated no-op guarantee on
  non-PE files is structurally preserved (verified byte-identical, §5).
- Interpretation: the implied saving must cover ~the ENTROPY-CODED framing
  cost. Measured compressed/raw stream ratios (bench diagnostics):
  masks 0.435, tmask 0.153, types 0.169, ml 0.59, ds 0.93 — blended ≈ 0.3–0.6
  of raw. γ=0.5 is the mid-point of that measured band, i.e. the literal
  amortization condition named in the pre-registration ("estimated true
  saving > per-token framing overhead with margin"), with the margin being
  the raw-vs-coded pricing slack.

Reference C++ (drop-in for the PNRA branch; ~10 lines, no new state):

```cpp
auto proto_varint_bytes = [](uint64_t x){ uint32_t b=1; while(x>=128){x>>=7;++b;} return b; };
double framing = 8.0 + 8.0*proto_varint_bytes(pm.len-4) + 8.0*proto_varint_bytes(pm.dist-1)
               + 32.0*((pm.len+31)/32) + 32.0*(((pm.len/4)+31)/32)
               + 8.0*double(pm.off.size());
bool accept = (alt_c - pnra_c) >= 0.5 * framing;   // gamma = 0.5, frozen
```

Prototype equivalence: `PNRA_MODE=1 PNRA_FORM=1 PNRA_GAMMA=0.5` on
`anvil_pnra_proto.exe` reproduces every number in §5.

## 4. Derivation — why this form and these constants

**(a) The margin must scale with shape (data, not taste).** End-to-end sweep
of two margin families on the pinned PEs (sizes in bytes; `off` = 108,518 /
845,675):

| family | setting | anvil.exe | bench | verdict |
|---|---|---:|---:|---|
| legacy (S>0) | — | 108,514 | 846,050 | bench regresses vs off |
| flat M=G0 | G0=20 | 108,514 | 845,688 | bench still ≥ off |
| flat M=G0 | G0=40 | 108,506 | 845,670 | both < off only at near-total rejection (33–90 commits kept) |
| dist-aware G0+G1·8·vb(D−1) | best | 108,506 | 845,658 | distance term adds ≤2 B over flat at same commit count → dropped (87% of commits at D<4K) |
| **γ·FramingRaw** | **γ=0.5** | **108,506** | **845,657** | **both < off; 29/80 commits kept** |

A flat bar cannot separate the modal mispriced shape (L=4–8, tiny S) from
legitimately long candidates without also rejecting the latter; the
framing-proportional bar does, because FramingRaw grows with L while the
mispriced class does not.

**(b) Why γ=0.5.** It is the measured coded/raw framing ratio band midpoint
(§3), making the rule self-describing rather than a swept optimum. The sweep
bracket [0.4, 0.6] both beat `off` on both PEs (γ=0.4: 108,503/845,674;
γ=0.6: 108,509/845,666); 0.5 was frozen BEFORE any held-out data exists, and
no re-tuning is permitted after (§8).

**(c) Rejected alternatives, with reasons.** Flat margin (fails bench at any
principled constant); distance-aware term (no measured leverage); correcting
`alt_c` itself to price the shifted-anchor exact alternative (mechanistically
cleaner but changes the shared comparison — larger integration surface, and
the counterfactual data show the threshold achieves the same rejection
profile with a one-line rule; recorded as the S6-3 oracle's natural
follow-up, owner `dp-parser`/`oracle` coordination).

## 5. Calibration results at the frozen setting (pinned PEs ONLY)

| metric | anvil.exe | anvil_bench.exe |
|---|---:|---:|
| `--pnra=off` | 108,518 | 845,675 |
| legacy rule (EXP. X) | 108,514 (−0.0037%) | 846,050 (+0.0443%) |
| **frozen γ=0.5** | **108,506 (−0.0110% vs off)** | **845,657 (−0.0021% vs off)** |
| commits legacy → frozen | 429 → 29 | 1,599 → 80 |
| round-trip (decode SHA-256) | PASS | PASS |
| non-PE no-op (generated.json, legacy vs frozen) | byte-identical | — |

First configuration in this lane's history that beats `--pnra=off` on BOTH
pinned PEs simultaneously — by a hair, which is itself the honest headline.

## 6. Flip predictions vs the legacy formula (deliverable 2)

**Counting basis (explicit — reconciled after ariref/ledger review).** Two
bases exist and must not be read as complements of each other:

- *Counter basis* (`pnra_commit=`): committed TOKENS in the full end-to-end
  parse under each rule — 429 → 29 (anvil.exe), 1,599 → 80 (bench).
- *Record basis* (candidate dumps): per-verified-candidate rows. Because
  rejecting a commit changes the downstream parse trajectory, the frozen-rule
  run verifies a slightly DIFFERENT set of candidate sites than the legacy
  run; flip analysis joins only the INTERSECTION of sites (605 of 615/614 on
  anvil.exe; 1,932 of 1,958/1,956 on bench).

The one-way monotonicity claim ("any γ=0.5 accept would have been a legacy
accept") holds at identical local state, i.e. per site — not across diverged
trajectories. Full accounting (asserted programmatically,
`tools/pnra_cost_analysis/reconcile.py`):

| file | legacy commits | = common-site flips + kept | + trajectory-lost | frozen commits | = common kept + trajectory-new | counter delta | = flips + lost − new |
|---|---:|---|---:|---:|---|---:|---|
| anvil.exe | 429 | 391 + 28 | 10 | 29 | 28 + 1 | 400 | 391+10−1 ✓ |
| anvil_bench.exe | 1,599 | 1,502 + 78 | 19 | 80 | 78 + 2 | 1,519 | 1,502+19−2 ✓ |

Flip shape profile (record basis, common sites): anvil.exe — L=4: 186, L=5:
96, L=6: 45, L=7: 27, L=8: 22, L=9–19: 15 → **95.6% of flips have L≤8**;
kept sites median L=18, 22/28 at D<4K. bench — L=4: 909, L=5: 267, L=6: 192,
L=7: 46, L=8: 41, L=9–19: 47 → **96.9% have L≤8**; kept median L=16, 62/78
at D<4K.

This is exactly the EXP. X root-cause class ("dominated by minimal,
one-window matches"): the rule removes the short-single-field far-distance
over-commit mass and retains long, amortized candidates.

**Expected byte effect on held-out PEs (prediction, falsifiable):** the
legacy rule's per-file delta should flip from ≈0/+ toward small negative;
magnitude expectation from calibration is **−0.002%…−0.03% per PE**, i.e.
**two orders of magnitude below the pre-registered ≥0.5% held-out bar**.
Unless held-out PEs carry materially denser relocation structure than the
pinned pair, the honest prediction is **VERDICT: FAIL (clean negative)** —
the threshold fixes the measured mispricing, but the mechanism's ceiling on
real PE data appears far smaller than the bar. This prediction is recorded
NOW, before any held-out measurement, per the project's honesty rule.

## 7. What the calibration does NOT establish

- No held-out file was touched. Nothing here is a victory claim.
- The counterfactual per-candidate costs are vs a naive literal splice, not
  vs the true re-parse (the re-search recovers non-field bytes); they were
  used for structure only. All constants trace to end-to-end sweeps, which
  capture re-search exactly.
- Second-order parse interactions (channel reinforcement, hash-chain state)
  differ between threshold settings; only aggregate sizes are claimed.

### 7a. Cross-corroboration addendum (u2, S6-3 oracle — recorded 2026-08-21)

The S6-3 measured-cost oracle (`deliverable/u2`) independently characterized
the same regression and REFINES this document's §2 story — the three
datasets are mutually consistent:

- Oracle (per-token measured-cost attribution, 473,985 candidates): every
  committed PNRA token is INDIVIDUALLY sound; post-hoc dropping any subset
  from the fixed on-parse trajectory enlarges every block; yet the
  `--pnra=off` parse (a DIFFERENT trajectory) is 375 B smaller on bench.
  Conclusion: the EXP. X regression is TRAJECTORY-borne — PNRA commits
  displace better downstream matches; the loss is invisible per-token.
- This re-explains this document's counterfactual sign pattern (§2 item 2):
  the splice-CF measured fallback cost WITHIN the on-trajectory, which is
  exactly the quantity oracle shows is always unfavorable. Both instruments
  agree the per-span fallback is bad; neither contradicts the e2e sweeps.
- Consequence for the frozen formula's INTERPRETATION (not its validity):
  γ·FramingRaw works because it is a PARSE-TIME decision — rejecting at the
  candidate site lets the ordinary search consume the span, changing the
  trajectory (845,657 < 845,675). The bar should be read as "local implied
  saving must be large enough (scaled by fixed framing) to plausibly dominate
  the candidate's displacement footprint", not as repairing per-token
  mispricing. Post-parse rollback CANNOT recover EXP. X (oracle, measured);
  S6-2's parse-time rule is the only lever, as pre-registered.
- Arch guidance stands (§3, §8); oracle's measured-payload arbitration and
  per-symbol attribution are complementary shared infra (their deliverable).

## 8. FROZEN VERDICT PROTOCOL (for the ledger — verbatim)

> **S6-2 held-out verdict protocol (frozen 2026-08-21, before any held-out
> measurement).**
>
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

## 9. Files

- `prototypes/pnra_cost/anvil_pnra_proto.cpp` + `.exe` — instrumented copy
  (env knobs: `PNRA_DUMP`, `PNRA_CF`, `PNRA_MODE`, `PNRA_FORM`,
  `PNRA_G0/G1/GAMMA`); defaults = exact EXP. X clone.
- `tools/pnra_cost_analysis/analyze.py` — counterfactual validation, shape
  tables, form fits.
- `tools/pnra_cost_analysis/flips.py` — frozen-vs-legacy flip join.
- `tools/pnra_cost_analysis/sweep.ps1` — end-to-end threshold sweep.
- `scratch/pnra-cost/` — candidate CSVs (legacy + frozen dumps), sweep
  results, frozen outputs + logs.
