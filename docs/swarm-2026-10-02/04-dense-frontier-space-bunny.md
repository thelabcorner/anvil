# Track 04 — Dense Pareto Measurement · Space Bunny Free

**Status:** INTERIM CHECKPOINT + §9 measured results (offline recomputation on frozen artifacts; U4 closed, U5/U6 open)
**Date:** 2026-10-02
**Track mandate:** audit the dense-frontier prereg/workflow for reference density, paired timing, robust CV/A/A controls, RSS, binary size, thread/affinity parity, FRONT-GAP/CROSSING semantics, and statistical power; propose a minimal valid remote matrix.
**Scope note:** this track produces **no codec mechanism**. Its "mechanism" is the *measuring instrument*. The deliverable is an audit plus a falsifiable proposal.
**Current leaning:** **KILL the classification-rewrite mechanism (refuted, §9.1). HOLD the dense-frontier dispatch as frozen. Downgrade the timing plane from "crossing hunt" to "margin-reduction measurement".**
**Substrate correction (§11):** the Class A grid compares ANVIL at a **256 KiB** match window against references at **4 MiB–64 MiB**. Source-confirmed. This conditions my own §9 result. **Do not declare any block-reset-sensitive mechanism dominated from Class A bytes until PWC-1 (§11.4) runs.**

---

## 0. Bottom line

I proposed a new arbiter primitive (ENVELOPE-dominance) and pre-registered its refutation. **It was refuted: 0 of 5 non-dominated rows reclassify, at every resolution tested.** The density-non-monotonicity argument I used to motivate it was also refuted by direct measurement — the opposite is true.

Two findings survive and are stronger than the proposal was:

1. **The dense-frontier timing plane cannot produce a `FRONT-CROSSING` on this corpus at any reference density.** `mean_crossing = 0.000` across random 9-of-10 and 10-of-10 reference subsets; the full grid leaves 0 crossings out of ~370 eligible cells. The vehicle's stated purpose — removing `GRID-THIN` — is *already satisfied by the existing grid*. Further densification buys **margin reduction**, not a crossing. (§9.2)
2. **All five canonical `FRONT-GAP` rows lie within one reference-grid spacing of the reference Pareto envelope.** That is a sharper, scale-free, citable statement than "they are bracketed", and it gives the dense run a *falsifiable success criterion* that is not a class token. (§9.1)

---

## 1. Proposed mechanism and its refutation

> **READ §9 FIRST.** Both claims in this section were tested offline against the frozen canonical grid and **both were refuted**. The analysis is retained because the refutation is the load-bearing result.

### 1.1 ENVELOPE-dominance

The dense vehicle's classification primitive is a **pairwise dominance test over a point cloud** (`dominates()`, `.github/workflows/anvil-i10-dense-frontier.yml:885-904`), whose output vocabulary is `DEGENERATE_RATIO | DESCRIPTIVE_NON_DOMINATED | DOMINATED` (`:913`). The project's own gate ruling defines four classes with a *bracket* test: `DOMINATED → DEGENERATE → FRONT-GAP → FRONT-CROSSING` (`RESEARCH_LEDGER.md:4508`).

**ENVELOPE** would replace the bracket test with a test against the *computed lower envelope* of the reference class at a declared resolution `ε_env`:

> An ANVIL row `p` is `ENVELOPE-GAP` (not `FRONT-CROSSING`) if it is non-dominated but its non-dominance margin on the binding axis is smaller than `ε_env` times the local reference-grid spacing at `p`.

**The justification I originally advanced is refuted.** I argued the bracket test is *not* monotone in grid density: `FRONT-GAP` requires ∃ two mutually non-dominating reference rows `q_lo` (smaller AND slower) and `q_hi` (larger AND faster) with `p` between them on both axes (`RESEARCH_LEDGER.md:4500-4503`); I claimed the number of such brackets equals the number of *curve intersections*, so adding reference curves would create brackets and hence create `FRONT-GAP` labels.

Measurement (`§9.2`) shows `mean_bracket_gap` decreasing **strictly and monotonically** in reference-set size: `28.98 (k=2) → 21.51 (4) → 11.04 (6) → 6.04 (9) → 5.00 (10)`, with `mean_crossing` falling `55.81 → 0.00`.

My reasoning error: bracket *count* can grow while bracket *occupancy* shrinks, and occupancy determines labels. Adding on-front points subdivides each open rectangle into smaller ones, so a candidate must land in a *smaller* rectangle — strictly harder to satisfy. The operative quantity is the **extent of the union of open rectangles**, and that shrinks monotonically.

**Corrected statement, now load-bearing:** the bracket rule *is* monotone in density, so the dense-frontier vehicle's *premise* is sound — densifying the class legitimately shrinks false gaps. But it also means densification cannot manufacture a crossing, and §9.2 shows it cannot approach one here.

What survives is the **margin statistic** `binding_margin / local_spacing`: scale-free, monotone, and strictly more informative than a class token — it says *how far* a row is from the envelope, not merely which side of a line it falls on.

### 1.2 Pre-registered hypothesis (REFUTED)

> **H-ENVELOPE.** Replacing pairwise-bracket `FRONT-GAP` with envelope-spacing `ENVELOPE-GAP` at `ε_env` declared *before* measurement changes the classification of at least one ANVIL row in the retained artifact set relative to the bracket rule, toward fewer false gaps — or the hypothesis is refuted, meaning the dense grid was already locally dense enough that the two rules agree everywhere and the arbiter rewrite is unjustified.

**Outcome: REFUTED — 0 / 5 reclassifications** at `ε_env ∈ {0.05, 0.10, 0.25, 0.50, 1.00}` (§9.1). The pre-registered refutation branch fired exactly as written.

**Disposition: KILL the classification rewrite.** The vehicle's three-class vocabulary *is* the wrong shape relative to four-class doctrine — but the correct fix is to **implement the adopted bracket rule**, not to invent a fifth rule.

### 1.3 The second defect class: the rate axis is a contaminated measurement

The dominant rate axis is `plane_mbps = source_bytes / candidate_median_s` (`:851`, `:861`), emitted into `frontier-inputs.csv:922-925` and `pareto.csv:926-930`. Its denominator is **whole-process wall time** measured by `time.perf_counter()` around `subprocess.run` (`tools/paired_bench.py:112-121`), and **every timed invocation is routed through a generated bash wrapper** that `rm -f`s the output then `exec`s (`:467-499`, invoked `:502-508`).

```
T_measured = T_exec_bash + T_dynlink(arm) + T_codec(input)
```

`T_exec_bash` is common to all arms; `T_dynlink` is not. A fixed additive term inflates the slower arm's time proportionally more, compressing all measured ratios toward 1. Consequence: the vehicle is **least sensitive exactly where the frontier is densest** (fast/small decode cells such as `xml`, 5.3 MB) and essentially unaffected on enwik8 q11 encode.

Additional asymmetry: `xz-9e` is routed through `bash xz-encode.sh` / `bash xz-decode.sh` (`:530-536`) — **two** bash startups, one inside a single-process decode's timed interval. The RSS census (`:558-559`) uses the *unwrapped* command while timing uses the *wrapped* command, so RSS and timing columns come from different command constructions.

> **H-CAL (open).** Measuring the additive process cost `c = median(T(observe.sh /dev/null true))` and `τ_arm` in-job, and reporting `plane_mbps_corrected = bytes / (median_s − c − τ_arm)`, changes at least one Pareto dominance verdict. Direction (hypothesis): de-trending raises the faster arm's rate proportionally more, so it should **sharpen** real differences and can create flips among fast cells; it cannot manufacture a win for the slower arm.
>
> **Unmeasured (U1).** I did not measure `c` or `τ_arm`. This cannot be measured locally for an Ubuntu runner.

---

## 2. Evidence map

### 2.1 Measured facts (verified in this session, with source)

| # | Fact | Source |
|---|---|---|
| M1 | The dense-frontier workflow and prereg are **untracked**; `git ls-files --error-unmatch` fails for both paths. | `.github/workflows/anvil-i10-dense-frontier.yml`, `docs/I10-DENSE-FRONTIER-PREREG.md` |
| M2 | **Zero dense-frontier runs have ever executed.** "Dense-frontier YAML parses, but cannot be dispatched until an explicitly authorized commit/push publishes the workflow." | `RESEARCH_LEDGER.md:4835-4837`, `:4953-4964` |
| M3 | The class is **20 arms** (2 anvil + 5 brotli + 11 zstd + `zstd-22-long27` + `xz-9e`). | `.github/workflows/anvil-i10-dense-frontier.yml:412-415` |
| M4 | Timing panel is **5 Silesia files**; bytes/RSS census is full-corpus (12 / 1). | `:403-409`, `:679`, prereg §4 |
| M5 | The arbiter emits only `DEGENERATE_RATIO` / `DESCRIPTIVE_NON_DOMINATED` / `DOMINATED`; `no_automatic_crossing_claim: true`. | `:913`, `:944` |
| M6 | The arbiter loops dominance **only over the timing panel**; census bytes exist for the full corpus. `pareto.csv` therefore holds **5 of 12 Silesia files** and **no aggregate row**. | `:907-914` vs `:544`, `:621-625` |
| M7 | `tools/pareto_front.py` — the superseded tool — **does** compute an AGGREGATE pseudo-file. Arbiter capability regressed. | `tools/pareto_front.py:78-91` |
| M8 | `dominated_full_cost` compares `stripped_binary_bytes` across **mixed scopes**: `decoder-linker-gc-harness` (anvil, brotli) vs `system-cli` (zstd, xz). | `:873-883`, `:912` |
| M9 | Prereg §9 states binary size "is not treated as a directly comparable crossing axis until a separate decoder-only build policy is frozen" — while `summary.json` sets `binary_size_comparability: "descriptive-only"`. **Direct prereg↔code contradiction.** | prereg `:228` vs `:912`, `:945` |
| M10 | `dominated_full_cost` mixes a **plane-local** rate with **cross-plane** memory; there is **no joint (ratio, encode, decode) dominance anywhere in the vehicle.** | `:885-893`, `:907-914` |
| M11 | Affinity is passed as `--cpu 0`, but `set_affinity` only *records* the result and returns `None` if unavailable; nothing enforces `== {0}`. Prereg §3 says unpinned timing is invalid — **not enforced**. | `paired_bench.py:83-89`, `:309`, `:477`; `:749`; prereg `:65` |
| M12 | The RSS census runs **unpinned** while timing runs pinned. Asymmetric, undeclared. | `:444-460` vs `:749` |
| M13 | `--reps` default **7** (7/9/13 allowed); `--epsilon 0.02`, `--max-cv 0.15`, ambient 5×250 000 @ CV 10%, bootstrap 20 000. | `:14-22`, `:742-752` |
| M14 | The **statistical core is byte-identical** between the pinned driver blob `f847c50e…` and the hardened local blob `bb2b2a7…` (working-tree `git hash-object`). The hardening adds only observation discipline; `robust_cv`, `bootstrap_paired_ratio`, `ambient_probe`, `clears_speed_gate`, `lo_i`/`hi_i`, `1.4826` are unchanged. **Adopting the hardened driver fixes none of §4.** | `git diff --no-index f847c50e… tools/paired_bench.py` |
| M15 | **The per-arm CV gate has already destroyed a real measurement.** enwik8 ruled `TIMING_BLOCKED` "because the Brotli control robust CV was 0.1756" (13 reps), in the run that measured enwik8 decode ratios **3.5162x / 10.1499x**. | `RESEARCH_LEDGER.md:4801-4802`, `:4941-4942` |
| M16 | **Peak RSS is a measured, dominant deficit.** Silesia ANVIL 248.4 MiB vs Brotli q11 114.4 MiB; enwik8 ANVIL 598.9 MiB vs q11 209.0 MiB; paired peak-RSS ratios 4.5717x/2.0273x (Silesia), 9.0594x/2.3785x (enwik8). | `RESEARCH_LEDGER.md:4798-4800`, `:4930-4934` |
| M17 | Decode measured slower than Brotli: Silesia paired ratio 1.7995x–3.3312x (95% CI `[3.2473, 3.3828]`), enwik8 3.5162x–10.1499x. Ruling `FRONT-GAP_COST`. | `RESEARCH_LEDGER.md:4796-4800`, `:4938-4942` |
| M18 | ANVIL is **smaller** than Brotli on both corpora (46,466,339 vs 49,383,136; 23,537,422 vs 24,810,180). The byte axis already wins; the deficit is cost-axis-only. | `RESEARCH_LEDGER.md:4796-4800` |
| M19 | **Speed accounting has already been ruled invalid once**: G4 closed as `NO-GO-G4` with the speed column *withdrawn* as `INVALID_SPEED_ACCOUNTING`. | `RESEARCH_LEDGER.md:4943-4944` |
| M20 | Canonical frozen tuple: `33 non-dominated | 5 FRONT-GAP | 0 FRONT-CROSSING | 28 DEGENERATE | 435/468 dominated`. | `RESEARCH_LEDGER.md:4927-4928`, `:4712-4713` |
| M21 | Bracket test is adopted doctrine; classification order is fixed. | `RESEARCH_LEDGER.md:4156-4176`, `:4500-4508` |
| M22 | Frozen substrate provenance: window `w-bench-frozen-suite-20260912T120527Z` + pass2, `src/anvil.cpp` BDC90474, pinned cores, **throughput ranking-grade, bytes citation-grade**, 13 files × 28 codecs = 364 rows. | `tests/suite-frozen-bdc90474.md`, `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` |

### 2.2 Verified by direct computation (cheap arithmetic, no corpus benchmark)

| # | Result |
|---|---|
| V1 | **MAD degeneracy is reachable at n=7.** `robust_cv = 1.4826·MAD/median` (`paired_bench.py:57-58`). `[1,1,1,1,1.4,1.9,3.0]` → MAD **0.0** → robust_cv **0.0000** → gate `<= 0.15` **PASSES with a 3× outlier present**. `[1.0,1.8,1.0,1.0,1.0,1.9,1.0]` (90% bimodal) → MAD **0.0** → **PASSES**. Cause: MAD is the 4th order statistic of 7 deviations; ≥4 coincident values ⇒ MAD=0. |
| V2 | **Bootstrap quantile convention is CORRECT.** `lo_i=500` → 2.5001%, `hi_i=19500` → 97.5049% of a 20 000-sample sorted list. I asserted an off-by-one, checked it, and **withdrew the finding**. |
| V3 | **Power.** MDE ≈ 1.96·σ_log-ratio/√n. To clear ε=2% the per-pair log-ratio SD must be ≤ **2.70%** at n=7, 3.06% at n=9, 3.68% at n=13. At ε=5%: ≤6.75%/7.65%/9.20%. |
| V4 | **Multiplicity.** Silesia: 20 arms × 5 files × 2 planes = 200 cells, 10 A/A, **190 non-null** tests. enwik8: 40 cells, 2 A/A, **38 non-null**. Expected false `PASS_SPEED_GATE` under the global null: **5.7–9.5** (Silesia), **1.1–1.9** (enwik8). |
| V5 | **I/O volume** for one Silesia job at reps=7: panel = 141.8 MB; 20 arms × (2 warmup + 14 timed) = 320 runs per (plane,file) ⇒ ≈ **90.7 GB** timing + ≈ **5.7 GB** census ≈ **96 GB read per job**. |
| V6 | Artifact files: 4 per cell × 200 cells + ~25 top-level ≈ **825 files**, each SHA-256'd at `:966-970`. |

---

## 3. Precise mechanism, state machine, and full cost model

### 3.1 The instrument as a two-pass transform

"Pass 1" writes evidence; "Pass 2" reads it and emits a verdict. State is bounded and explicit.

**PASS 1 — CENSUS (`:544-619`).** One encode + one decode per (file, arm). Emits `census.csv`, `bytes.csv`, `payload-manifest.json`, `corpus-manifest.json`, `arm-contract.json`, `prepare-summary.json`. Each cell records `{source_bytes, source_sha256, compressed_bytes, compressed_sha256, encode_s, decode_s, encode_peak_kib, decode_peak_kib}`. Verifies ANVIL byte identity against the frozen table and reference totals; cross-checks dense Brotli q11 against frozen `brotli_lw` (`:599-610`).

**PASS 2 — PAIRED TIMING (`:707-771`).** Per (plane, file): shuffle arm order by `Random(seed_base + file_index + plane_offset)`; per arm, control = `anvil-legacy`, candidate = arm; invoke the pinned driver.

Driver state machine (`tools/paired_bench.py:309-431`), n = reps:
```
SET_AFFINITY(cpu)         -> record only, never enforced           [F9]
AMBIENT_PROBE(5 x 250k)   -> blocked := robust_cv > 0.10   ONCE, not per-pair
for w in 0..1:                       # symmetric warmup, alternating order
    for arm in order(w): UNLINK(observe[arm]); EXEC; VERIFY size+sha
for i in 0..n-1:            order_i := seeded coin flip
    for arm in order_i:     UNLINK(observe[arm]); EXEC; VERIFY size+sha ; t[arm][i] := perf_counter delta
log_ratios[i] := ln(t[cand][i] / t[ctrl][i])
ratio, ci := BOOTSTRAP_MEDIAN(log_ratios, seed^0xA5A5A5A5, 20000)
timing_valid := !blocked and rCV(ctrl) <= 0.15 and rCV(cand) <= 0.15     [F7]
clears_speed_gate := timing_valid and ci_hi < 0.98                       [F6]
```

**PASS 3 — CLASSIFICATION (`:801-930`).** Per cell: load summary, re-verify raw observations → `paired.csv`; map size key → scope + stripped bytes; compute `dominated_no_binary` / `dominated_full_cost`; assign `descriptive_class`; emit `frontier-inputs.csv`, `pareto.csv`, `summary.json`. Then `manifest.json` SHA-256s every file (`:966-970`).

**Decoder-visible state of the instrument:** arm table, corpus table, toolchain pin table (prereg §2), threshold table (prereg §8), classification order (`RESEARCH_LEDGER.md:4508`). All frozen, none inferred at decision time. **This part of the design is correct and should be preserved.**

### 3.2 Byte / cycle / memory cost of the instrument

| Axis | Current | Minimal valid matrix (§5) |
|---|---|---|
| Artifact files | **≈ 825** per Silesia job (V6) | ≈ 273 |
| Timed codec launches | 20 arms × 16 runs × 5 files × 2 planes ≈ **3 200** | 14 × 12 × 4 × 2 ≈ **1 344** |
| Disk read volume | **≈ 96 GB** (V5) | ≈ 30 GB |
| Wall time | Silesia q11/lw30 encode of `webster` alone is minutes; 3 200 launches plausibly approach the 360-min cap (`:34`) — **unverified (U3)** | ≈ 45–60 min × 3 jobs |
| Instrument RSS | Python driver holds O(reps) rows/cell ⇒ negligible. The binding RSS numbers are the *codec* numbers (M16). | unchanged |

### 3.3 Cost of the ANVIL arms themselves — unchanged by any arbiter work

Measured, not modelled (M16/M17/M18): ANVIL wins bytes and loses decode by 1.80x–10.15x and peak RSS by 2.03x–9.06x. **No arbiter change can alter this.** The arbiter's only job is to state it without letting measurement artifacts manufacture or destroy crossings.

---

## 4. Failure modes (adversarial), ranked by ability to change a verdict

**F1 — `dominated_full_cost` is dominated by I/O architecture, not mechanism (CRITICAL).**
Peak RSS is a *buffering* choice, not a compression cost. ANVIL's wrappers `read_file()` the whole input (`:284`, `:514`); zstd and xz stream. Measured consequence (M16): ANVIL 248.4–598.9 MiB vs Brotli q11 114.4–209.0 MiB. With both RSS axes inside the dominance predicate (`:891-892`), **every ANVIL row is dominated on memory by every zstd row regardless of mechanism quality** on the three largest panel files.
*Fix:* replace raw peak RSS with `resident_working_set_charge = peak_rss − source_bytes` in the predicate; report raw RSS descriptively; never let RSS enter a cross-family dominance test while families differ in buffering discipline.

**F2 — mixed binary-size scopes in one dominance predicate (CRITICAL).**
`dominated_full_cost` compares anvil/brotli `decoder-linker-gc-harness` stripped sizes against zstd/xz `system-cli` stripped sizes (M8). The prereg explicitly forbids treating this as comparable (M9). The column is named `full_cost`, invites citation, and is arithmetically meaningless.
*Fix:* drop `dominated_full_cost`; or restrict binary-size dominance to the anvil↔brotli pair (the only matched-scope pair) and label it `decoder_size_paired`.

**F3 — no joint (ratio, encode, decode) dominance (CRITICAL) — CONFIRMED ON REAL DATA, §9.3.**
Per-plane dominance with shared memory axes (M10), while doctrine requires a row to be non-dominated on both planes before it counts (`RESEARCH_LEDGER.md:4156`). §9.3 measures **0 rows non-dominated on both planes; all 5 are single-plane.** So the vehicle will print `DESCRIPTIVE_NON_DOMINATED` for rows that doctrine classifies as *not* a Pareto win.
*Fix:* one row per (file, arm) with `dominates_joint` over `(compressed_bytes, encode_mbps, decode_mbps, resident_charge)`; per-plane booleans become diagnostics.

**F4 — additive process cost contaminates the rate axis (HIGH).**
`plane_mbps` includes bash-wrapper + exec + dynlink. Structure of the bias is unambiguous; **magnitude is unmeasured (U1)**. The vehicle is least sensitive where the frontier is densest.
*Fix:* in-job calibration (§1.3); report `plane_mbps_raw` **and** `plane_mbps_corrected`; require the corrected axis for any dominance verdict.

**F5 — the "no binary" variant still carries the streaming artifact (HIGH).**
`descriptive_class` (`:913`) keys off `dominated_no_binary` — no binary, **but still both RSS axes**. So even the most optimistic label is structurally unreachable for a whole-file codec.

**F6 — 190 uncorrected one-sided tests (HIGH).**
`PASS_SPEED_GATE` is per-cell with no family-wise control (V4): expected 5.7–9.5 false positives per Silesia job. `paired.csv` ships 190 rows with an uncorrected `status` column and no multiplicity column.
*Fix:* Holm–Bonferroni across the arm family within each (plane, file); ship `p_holm` and `status_holm`; label the raw column uncorrected.

**F7 — the per-arm CV gate screens the wrong quantity (MEDIUM-HIGH).**
`robust_cv` conflates (i) *within-pair* differential noise, which does contaminate the paired ratio, and (ii) *across-pair* common-mode drift, which does not contaminate the ratio but does inflate per-arm MAD. The gate screens on the sum, so (ii) blocks valid cells. **Measured proof it binds and has cost real evidence: M15** (enwik8 blocked at CV 0.1756, discarding a 10.15x decode-ratio measurement). Compounded by **V1**: MAD=0 passes a 3× outlier. The gate is simultaneously too strict on the wrong statistic and too lax on gross outliers.
*Fix:* screen on the dispersion of `log_ratios` plus a MAD-floor so V1 cannot pass; keep per-arm CV as a recorded diagnostic.

**F8 — A/A null is degenerate in the observation dimension (MEDIUM).**
For `arm == "anvil-legacy"` control and candidate are the *same dict entry* (`:712-714`), so `--control-observe == --candidate-observe` — one path, one expected hash (`:729-732`). The A/A cell validates the **noise floor** but exercises **no** two-distinct-path freshness handling. A dual-path defect would pass A/A.
*Fix (cheap, no new arm):* add an `A/A′` cell with two distinct-but-equivalent commands — `brotli-dense 11` vs frozen `brotli_lw`, which the vehicle already proves byte-identical (`:599-610`). Different argv, different binary, identical output ⇒ a true two-path A/A′.

**F9 — affinity not enforced; RSS unpinned (MEDIUM).** M11, M12.
*Fix:* `set_affinity` raises → `BLOCKED_AFFINITY` when `--cpu` is given and the result ≠ `{cpu}`; add `taskset -c 0` to the census.

**F10 — undeclared threshold (LOW-MEDIUM).** `DEGENERATE_RATIO` at `ratio >= 0.95` is hard-coded at `:913` and appears nowhere in the prereg. Never fires on Silesia/enwik8 (0.219–0.248) — dead code here, but it is a threshold *inside* the arbiter, violating brief §5.

**F11 — `/usr/bin/time %M` semantics (LOW).** `ru_maxrss` from `wait4` is a **max over waited children, not a sum**. All arms are single-process today, so currently correct — but `xz` runs under `bash` (`:530-536`), so the max is over {bash, xz}. Harmless at current margins; fragile if any arm spawns a worker.
*Fix:* assert single-process via a `/proc` child-count probe, or move to cgroup `memory.peak`.

---

## 5. Minimum prototype and the minimal valid remote matrix

### 5.1 Minimum prototype — **written and run** (§9)

`prototypes/swarm-2026-10-02/04-dense-frontier/space-bunny/envelope_arbiter.py` — a pure post-processor over a retained frozen suite. It reads; it never re-measures; it never touches `src/`. It implements the adopted bracket rule, the proposed envelope rule, a margin statistic, and a reference-density monotonicity sweep. **Validation anchor: on the full frozen grid it reproduces the canonical tuple exactly — 5 non-dominated, 5 bracket FRONT-GAP, 0 crossings (`RESEARCH_LEDGER.md:4927-4928`).**

Its result is that the proposed classification rewrite is unjustified (§9.1) and the density premise needs correcting (§9.2). Per the project norm that a dropped mechanism records *why*, the refutation is recorded here rather than discarded.

### 5.2 Minimal valid remote matrix

Design rule: *valid* = every prereg §1 gate condition satisfiable; *minimal* = no cell that cannot change a verdict.

| Element | Frozen (as-is) | Minimal valid |
|---|---|---|
| Corpora | Silesia 12 + enwik8 | **both retained** |
| Timing panel | dickens, mozilla, nci, webster, xml | **mozilla, nci, xml** — largest-text, largest-binary, smallest-fast-decode. Drops dickens (near-duplicate of mozilla's text character) and webster (mozilla already spans text). **Panel change is a prereg amendment, frozen before dispatch.** |
| Arms | 20 | **14**: anvil-legacy, anvil-aux, brotli q1/q4/q6/q11-lw30, zstd 1/9/19/22, zstd-22-long27, xz-9e, **+ `A/A′`**. Drops zstd 3/5/7/11/13/15/17 and brotli q9 — interior to their curves, so they cannot move a boundary |
| Planes | encode, decode | unchanged |
| Reps | 7/9/13 | **9 default**, 13 for cells near a gate. Rationale V3: at n=7 ε=2% demands per-pair log-ratio SD ≤ 2.70%, which hosted-VM noise may not meet; n=9 relaxes to 3.06% |
| Epsilon | 0.02 | **0.05 for `dominated`, 0.02 for `crossing`** — frozen pre-dispatch. 2% is below the resolution of a rate axis that still carries additive process cost (F4) |
| ε_env | absent | **declared pre-dispatch**; must be ≤ 1/3 of the smallest inter-point gap on the binding axis, else the run reports `UNDERSAMPLED_REFERENCE_GRID` |
| Multiplicity | none | Holm across arms within (plane, file) |
| Success criterion | (implicit) a crossing | **`binding_margin / local_spacing` falls below a declared value** — *not* "a crossing appears" (§0.2, §9.2) |

---

## 6. Strongest disconfirming evidence

1. **Against §0.2 / §9.2 — this is the sharpest threat to my own recommendation.** The 0-crossing result is computed on the **frozen Windows-host `tests/corpus` suite** (M22, 13 files, ranking-grade throughput). It does not establish that Silesia/enwik8 — the corpora the dense vehicle actually targets — are equally unreachable. **U6 exists precisely for this.** If Silesia behaves differently, the dense plane regains priority.
2. **Against F1/F2 (memory and binary size as real axes):** a reviewer may argue peak RSS genuinely *is* the decoder cost the user pays, so charging it is correct and buffering discipline is part of the shipped product. That is a legitimate position and matches `docs/github-actions-benchmark-protocol.md:116` (AITDCC 8-GB memory constraint, ≤1-MB decompressor). **My claim is narrower:** RSS must not enter a *cross-family* dominance predicate while families differ in buffering discipline. Reporting raw *and* normalized satisfies both positions.
3. **Against F4:** if `c + τ_arm ≲ 1 ms` (U1), F4 collapses to noise. I have not measured it and cannot measure it locally for an Ubuntu runner.
4. **Against the whole track's framing:** M19 (`INVALID_SPEED_ACCOUNTING`) may mean the speed axis is *permanently* unusable on hosted runners, in which case the correct conclusion is that **no dense Pareto timing run on Actions can ever support a crossing claim** and the vehicle's only defensible output is the byte/RSS census plus the margin statistic. This remains a live possibility I cannot refute with available evidence.
5. **Against §9.3/F3 as a *hazard*:** if every consumer of `pareto.csv` already applies doctrine by hand, the 3-class vocabulary is a documentation defect rather than a verdict defect.
6. **Against F6 (multiplicity):** if `PASS_SPEED_GATE` is never cited per-cell and only the arbiter is read, the risk is presentational rather than logical.

---

## 7. Uncertainties

| ID | Uncertainty | How to settle it | Could flip |
|---|---|---|---|
| U1 | Additive process cost `c + τ_arm` on the runner | 1 in-job step: `for i in $(seq 200); do /usr/bin/time -f %e observe.sh /tmp/x -- true; done` plus bare `true`; report medians | F4, ε choice |
| U2 | Whether `dominated_full_cost` verdicts flip under de-trending | Re-run the §5.1 post-processor on retained artifacts | F4 |
| U3 | Whether a reps=7 Silesia job fits the 360-min cap | Derive from census `encode_s` already in `census.csv` (`:590-591`) — no new run | matrix size |
| ~~U4~~ | ~~Whether H-ENVELOPE changes any classification~~ | **CLOSED 2026-10-02 — REFUTED, 0/5 (§9.1)** | mechanism killed |
| U5 | Whether the CV gate's M15 block was justified (was the paired ratio actually unstable?) | `raw.csv` is retained in artifact `10835616518`; recompute `MAD(log_ratios)` | F7 |
| **U6** | **Is 0-crossing a corpus property of `tests/corpus`, or would Silesia/enwik8 behave differently?** | Recompute §9 on a retained Silesia/enwik8 suite if one exists, else on any retained suite with a different file mix | **the recommendation** |

**U5 and U6 are the only two that can still change the verdict.**

---

## 8. Recommendation (REVISED after §9)

**KILL the classification-rewrite mechanism. HOLD the dense-frontier dispatch as frozen. Downgrade the timing plane from "hunt for a crossing" to "measure margin reduction". Reallocate the crossing search to mechanism work on new corpora.**

Rationale, in order of weight:
1. **§9.2 — the timing plane cannot produce a crossing on the measured corpus at any reference density.** `mean_crossing = 0.000` even at k=9 and k=10; the full grid leaves 0 crossings out of ~370 eligible cells. The vehicle's stated purpose (`GRID-THIN` removal, prereg `:242`) is **already achieved by the existing grid**. Densification buys margin reduction, not a crossing. This is stronger than "unproven" — it is a measurement that the target is unreachable.
2. **§9.1 — H-ENVELOPE refuted (0/5).** Killed, with the refutation recorded rather than quietly dropped.
3. **§9.3 — F3 confirmed on real data: 0 rows non-dominated on both planes; all 5 single-plane.** The vehicle prints a positive-sounding per-plane label for rows doctrine classifies as not a Pareto win (`RESEARCH_LEDGER.md:4156`). Implementing the adopted joint-plane rule would flip **every** non-dominated label in the canonical grid. Presentational hazard, not cosmetic.
4. **F1+F2 make `dominated_full_cost` and both RSS axes non-comparable by the prereg's own statement** (M9); M16 shows the RSS deficit is 2.03x–9.06x and partly a buffering artifact. Any cost-axis verdict today is unsafe.
5. **M2/M19** — never executed, untracked, on a timing axis already once ruled `INVALID_SPEED_ACCOUNTING`.
6. **M14** — the hardened driver is available and fixes none of the statistical defects, so "adopt the hardened pin and dispatch" is not a cheaper path.

**KILL — now met — for:** the ENVELOPE classification rewrite (§9.1); the density-non-monotonicity claim (§9.2); any plan whose success criterion is a `FRONT-CROSSING` token from the dense run.

**PROMOTE-TO-REMOTE conditions (revised).** The dense run is worth dispatching **only** as a margin-reduction instrument, and only if **all** hold:
- (a) the adopted 4-class bracket rule replaces the 3-class vocabulary, with the joint-plane rule from §9.3;
- (b) `dominated_full_cost` and raw-RSS axes are removed from cross-family dominance (F1, F2);
- (c) `ε_env` and margin thresholds are frozen pre-dispatch;
- (d) the success criterion is **"candidate rows' `binding_margin / local_spacing` falls below a declared value"** — not "a crossing appears";
- (e) U1 shows `c + τ_arm` is material enough to require rate-axis de-trending, or de-trending is dropped from the prereg;
- (f) the `A/A′` cell (F8) is added.

**If U6 shows the 0-crossing result is a `tests/corpus` artifact rather than a grid property,** the dense plane regains priority and §5.2's matrix should be frozen and dispatched with (a)–(f).

**What I am explicitly not doing:** no codec runs, no local benchmark, no threshold movement, no modification of any existing file, no commit/push. The prototype reads one retained frozen artifact and performs arithmetic only.

---

## 9. Measured results — offline recomputation

**Substrate (single window, no splicing):** `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` — 364 rows, 13 files × 28 codecs, window `w-bench-frozen-suite-20260912T120527Z` + pass2, `src/anvil.cpp` BDC90474. Provenance in `tests/suite-frozen-bdc90474.md`: **`compressed_bytes` citation-grade; throughput ranking-grade** (pinned-core pre-window util 26.9% main / 9.2% pass2, both above the <5% gate). Reference class = 10 rows: `brotli-q1/q4/q6/q9/q11`, `zstd-1/3/9/19`, `xz-9e`. **Throughput columns here are ranking-grade and are NOT spliced with any Actions series** (`RESEARCH_LEDGER.md:4794-4806`).

**Tool:** `prototypes/swarm-2026-10-02/04-dense-frontier/space-bunny/envelope_arbiter.py` (isolated; not wired into production).

### 9.1 U4 — ENVELOPE vs bracket rule: REFUTED

```
non-dominated ANVIL cells (plane-split) : 5
  bracket FRONT-GAP                     : 5
  would be CROSSING under bracket rule  : 0

 eps_env ENVELOPE_GAP  CROSSING disagree_vs_bracket
    0.05            5         0                   0
    0.10            5         0                   0
    0.25            5         0                   0
    0.50            5         0                   0
    1.00            5         0                   0
```

Two conclusions:
- **Zero reclassifications at every resolution.** H-ENVELOPE is refuted. The classification rewrite is unjustified.
- **Positive result:** all 5 rows remain `ENVELOPE_GAP` even at `ε_env = 1.00`, i.e. `binding_margin < 1.00 × local_spacing`. **Every one of the five canonical `FRONT-GAP` rows lies within one reference-grid spacing of the reference Pareto envelope.** This is a sharper statement than "they are bracketed", it independently confirms the ledger's own diagnosis that the non-dominance "is a property of the test matrix, not of ANVIL" (`RESEARCH_LEDGER.md:4191-4193`), and it supplies a scale-free success criterion for any future densification.

### 9.2 Density monotonicity — my §1.1 claim REFUTED

400 random reference subsets per size, seeded 41246:

```
 k_refs  mean_bracket_gap  mean_crossing
      2            28.975         55.807
      3            28.323         20.320
      4            21.508          8.852
      5            15.635          4.725
      6            11.035          2.115
      7             9.908          0.575
      8             7.742          0.100
      9             6.037          0.000
     10             5.000          0.000
```

- **`mean_bracket_gap` is strictly decreasing in k.** The bracket rule *is* monotone in density. My §1.1 claim that densification can increase `FRONT-GAP` labels is **wrong**, and the reasoning error is identified in §1.1 (bracket count vs bracket occupancy).
- **`mean_crossing` reaches 0.000 by k=9** and stays 0. No reference subset of size ≥9 produces a single crossing-eligible row. **A `FRONT-CROSSING` on this corpus is unreachable at any reference density short of changing the corpora.**
- The k=10 row reproduces the canonical tuple (5 / 0) exactly — validation anchor for the implementation.
- **Consequence for the track:** the dense-frontier vehicle cannot change the canonical `0 FRONT-CROSSING`, because the target is already unreachable. Its only honest value is byte/RSS census plus margin reduction.

### 9.3 F3 — joint-plane non-dominance: CONFIRMED

```
non-dominated on BOTH planes : 0
non-dominated on ONE  plane  : 5
```

This *confirms on real data* both F3 and the doctrine at `RESEARCH_LEDGER.md:4156` ("A single-plane encode knee against a 2-point front is NOT a 'Pareto win'"). All 5 canonical rows are single-plane. The vehicle's per-plane `DESCRIPTIVE_NON_DOMINATED` label (`:913`) will therefore be printed for rows that doctrine does not license as frontier results, and it has no mechanism to express the joint verdict. Implementing the adopted joint-plane rule would flip **every** non-dominated label in the canonical grid from positive-sounding to dominated — a material change in what the artifact appears to say.

---

## 10. Claim hygiene

- Every number in §9 derives from **one** frozen window (M22) and is labelled with its grade. No Actions series, no Windows-host series, and no Linux series are compared to each other.
- Throughput-derived columns in §9 are **ranking-grade** and are used only for *relative* dominance, which is what the arbiter uses them for. Absolute MB/s are not claimed.
- The two refutations (§9.1, §9.2) are stated as refutations of **my own** proposals, with the reasoning error named.
