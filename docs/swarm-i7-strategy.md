# ANVIL — Iteration-7 Strategy Synthesis (the cost-model-fidelity theme, measured) + the Iteration-8 plan

Author: `strategy` (strategy synthesist, swarm anvil-i7-cost). Status: **analysis
only** — no production code, no benchmark run by this lane, no claim. This document
turns Iteration-7's measured results into the ranked I8 plan. Every number is
sourced from a peer's reported measurement (blackboard deliverable keys cited
inline), from `RESEARCH_LEDGER.md`, or from the frozen baseline snapshot
`tests/benchmark-suite.pre-s61.csv`; projections are explicitly labeled as
arithmetic on landed numbers, never as measurements. Where a result was pending
at freeze time it is marked PENDING, not guessed.

**Freeze state of this doc:** written 2026-08-21 after u1/u2/u3/u4/u5/u6 landed.
The decode swarm's S6-1 t7 median-3 verdict had NOT landed; §2.1 incorporates its
frozen gate, its landed interim measurements (t3 floor profile, Experiment-Y
triage, correctness gate), and the pre-committed interpretation branches. Patch
§2.1 verbatim when bench reports.

---

## 1. Executive summary (the thesis)

The streak — **0 EXTENDS_FRONT across six iterations (~396 verdict rows through
I5 per `docs/swarm-i6-strategy.md` §1, plus EXP. X's regenerated suite, all
DOMINATED)** — survived I7's primary attempt *by measured construction*: the
decode swarm's own floor profile (`deliverable/t3-decode-floor-profile`)
decomposed the mode-15 decode time as **crc32 44% + macro-stream materialization
26% + token loop 11% (opcode entropy pulls only 0.3%) + block concat/alloc ~15%**,
which caps stream-budget-only gains at **≤2–5% end-to-end under gate bar B**
(zero byte growth) and makes bar A (≥547.1 MB/s = 2×) unreachable without the
wire-invisible CRC fix. The Linux raw-shape-delta lever does not transfer: the
Windows analog stream (macro-dvar) is **already stored raw** (~1.2 cyc/B).

But the same body of measurements relocates the streak-breaker. Recomputing the
per-file frontier arithmetic on the frozen baseline (`tests/benchmark-suite.pre-s61.csv`,
exact bytes):

- **synth-arith.bin is the closest cell in project history.** Every anvil row
  sits at 256,022 B (ratio 1.000); the reference front's ratio extreme is
  brotli-q11 at **87,013 B (0.340)**. Experiment Z's validated harness wire for
  the same file is **89,363 B (0.349)** — **+2.7% from the front**, already below
  zstd-19 (106,614), q9 (114,417), and zstd-1 (201,318). Whether the entropy-coded
  end-to-end container lands above or below 87,013 B is exactly what the frozen
  mode-16 integration gate will measure.
- **generated.json needs one mechanism, precisely sized:** best anvil
  (mdl-rans 89,589 B @ 150 MB/s dec) is dominated by exactly ONE reference row —
  brotli-q11 (79,172 B @ 652 MB/s). Escape = **−11.6% bytes** (or decode >652,
  unreachable). jsonl needs −18.0%; every other corpus cell is ≥15.7% out.

So the ranked answer to "what breaks the streak next" changes shape: **not a
decode-only lever** (S6-1b moves the decode plane materially but byte-identically
— it cannot extend a front while every anvil row is ratio-dominated), and **not
the record-file q9 bar** (I6's rank-1 framing, now measured-out at −29% to −53%
bytes depending on the decode band). It is **mode-16 ARI-REF landing on
synth-arith.bin within ~3% of its harness wire** — the first cell where a
measured anvil-side artifact already touches the reference front — with
**generated.json −11.6%** named as the precise open problem for record-file
ratio work. Everything else this iteration produced enablement: a frozen
acceptance formula whose honest prediction is a clean negative, a measured
refutation of post-parse rollback, a legal gate discharged, an anti-overfit
substrate, and a decode-floor lift pre-registration that converts FLAG-A from
"3–4× behind" toward parity.

---

## 2. What Iteration 7 measured (input inventory)

### 2.1 S6-1 whole-codec stream budget (concurrent swarm anvil-i7-decode) — verdict PENDING; interim measurements decisive

**Frozen gate** (`deliverable/pre-reg-s6-1` v1.2, frozen 2026-08-21 04:41 −05:00,
HEAD 392e937 + `tests/benchmark-suite.pre-s61.csv`): on generated.log, median-3,
all three legs — (A) decode ≥ **547.1 MB/s** (= 2× frozen mode-15 baseline
273.534); (B) bytes ≤ **175,550 B** (ratio ≤ 0.090); (C) beat q9 both planes:
< **124,669 B** AND > **619.508 MB/s** → EXTENDS_FRONT via `tools/pareto_front.py`
= first in project history. Constants binding: additive J = L + λ·C_decode,
λ = 0.01; size-proportional C_decode calibrated from the t3 table (Amendment 3);
threshold-fit voids the run. Pre-registered partial-credit tier
FORMULATION-REPRODUCED: A ∧ ≤183,233 B (+4.377% Linux trade factor, floor-rounded)
∧ controls. Anchor correction of record (Amendment 2): the Linux 452 KB crossing
was NOT generated.log — Linux started ~12% ahead of q9 on its file; Windows starts
~41% behind on this one.

**Landed interim measurements (citable now):**

- **t3 floor profile** (`deliverable/t3-decode-floor-profile`, median-7
  interleaved, HEAD-faithful payloads, gen_log.anv = 175,550 B exactly): floor
  log 235.8 MB/s (CV 10%), json 195.7, jsonl 293.5, sqlite 168.6 (harness runs
  ~5–15% below bench protocol; relative deltas valid). Time shares (log):
  crc32 **44%** (1.9 ns/B bytewise), masks+resid eager materialization **26%**
  (setup IS masks+resid: 2.07 of 2.12 ms), token loop 11% (**opcode entropy
  pulls 0.3%**), block concat/alloc ~15%. Per-stream: only real raw-storage
  gains are macro-masks (+21–27% decode for **+166,823 B**) and macro-resid
  (+18–21% for **+84,956 B**) — ratio-infeasible under bar B; opcodes/types/
  dflags are 0.2–0.3% of decode (inside the ±3–8% noise band measured with
  zero-byte-change controls); **macro-dvar (the Linux shape-delta analog) is
  ALREADY RAW**. Strategic consequence (their §5): stream budget alone caps at
  **≤2–5% end-to-end** under bar B; "CRC alone caps end-to-end improvement at
  ~1/(1−0.44) even if streams were FREE." C_decode table (ns/B): raw 0.1 bulk /
  2.6 byte, defexc 3.2, huffman 4.3 (data-dependent 4–9), rANS ≈6.0 flat,
  ctx-rANS 7.5 — current constants have the right order, wrong gaps.
- **Experiment-Y triage** (`deliverable/exp-y-verdict`, arch, commit da01c54):
  RLZ/RePair book-stream codecs (modes 7/8, `--hotop-rlzp`) landed flag-gated
  default-off after fixing five decoder findings (F1–F5, all fixed-with-test).
  T1 book bytes PASS: log 175,550→157,446 (**−10.31%**), jsonl −10.76%, json
  −4.15%, sqlite −0.64%; synth-drift-stride −28.6%, counters −19.1%,
  columnar-align −14.2%, jitter −5.2%; PEs −0.5..−2.9%; controls flat. T2 decode
  FAIL: log −8.1%, jsonl −15.9%, json −11.0% (sqlite +4.6%). Encode prohibitive:
  pe-git 141.5 s vs 2.2 s (~64×). **Verdict: NOT ADOPTED AS DEFAULT** (ratio-only
  trade-off point, kept as the measured record).
- **Correctness gate** (`deliverable/t6-correctness-gate-baseline` v3,
  fuzz-verify): 242/242 round-trips byte-exact over 22 files × 11 combos;
  flag-off byte-identity vs an independently built freeze-HEAD binary 242/242;
  fuzz seed 0xA11E PASS (910 variants / 5,460 mutations) + extended rlzp fuzz
  (650 / 3,900). Format determined S6-1 **wire-invisible** (selection behind
  existing per-substream mode bytes; zero new bits/modes).

**Pre-committed interpretation branches (from the frozen text, quoted not
invented):** A-pass/C-fail = "decode lever reproduced, frontier not crossed";
B-fail above the tier bound = trade overpaid; clean fail = implementation-era
attribution per §7, with the t3 profile as the pre-staged measured explanation.
Given the profile arithmetic (budget-only cap ≤2–5% on a 273.5 baseline ⇒
≤~287 MB/s vs bar A 547.1), the measured expectation is leg-A FAIL; this doc
records that expectation and defers the verdict itself to bench.

### 2.2 Experiment Z — ARI-REF additive-family reference (ariref, `deliverable/u1`) — representation VALIDATED via transmitted Δ

Token-economics harness (counted/varint wire, no rANS/container; absolutes not
comparable to anvil/brotli). Targets:

- **(a) Density: PASS on synth-arith.bin** — exact-LZ wire 257,124 → tx **89,363 B
  = −65.25%** (bar ≥10%, passed 6.5×); implicit 123,638 (−51.92%). FAIL on
  synth-timeseries.bin (+0.03%, 33 tokens — 14 B stride not ×4-alignable;
  per-field deltas outside the pre-registered word-aligned constant-Δ family).
- **(b) Implicit-Δ ablation: FAIL as pre-registered** — implicit Δ=σ·(d/4) costs
  **+38.35%** vs transmitted on the same 2,160 tokens; boundary isolated: exact
  progression → implicit BEATS transmitted (−15.38%); ±1 jitter → +40.55%
  (one-word-pair σ estimation amplifies jitter into ±(d/4) Δ error paid as
  residuals). Zero-bit claim narrows to exact progressions; **transmitted Δ is
  the working form and carries the (a) gain**.
- **(c) Attribution control: PASS** — real-corpus contribution ≈zero in tx mode
  (0–72 tokens on text/structured, wire deltas ≤0.25%; binaries 0.02–0.35%;
  anvil_bench.exe −1.42% outlier noted, not claimed).
- **(d) Correctness: PASS** — byte-exact round-trip 22 files × 4 modes; fuzz
  280/280 (seed 0xC0FFEE); 199 single-bit mutations: 79 rejected / 1 equivalent /
  119 detected-by-self-check / 0 crashes / 0 silent accepts.
- priorart independently rebuilt (source SHA CFE97A36…) and reproduced every
  number exactly. verify re-ran line-identical 71/71 (`deliverable/u6`).

Downstream: **mode-16 pre-registration FROZEN** in the decode swarm
(`deliverable/pre-reg-i8-ari-ref`): transmitted-Δ only (implicit excluded);
primary target synth-arith.bin, bar Leg 1 ≤ **0.80× current-best-anvil
end-to-end** (= ≤≈204,818 B against the measured 256,022 B row — retains ≥30%
of the harness gain); Leg 2 decode co-arbiter paired same-build; Leg 3 full-corpus
no-regression with byte-identity on zero-fired files; columnar-align (0 ARI
tokens in harness) and counters.log (+0.37%) demoted to reported-not-required;
sequencing behind S6-1; **binding non-goal: no Pareto claim without
EXTENDS_FRONT from the tools**.

### 2.3 S6-3 measured-cost rejection oracle (oracle, `deliverable/u2`) — calibration PASS; EXP. X root cause REFINED; rollback ruled out

- (a) Calibration: 473,985 committed candidates sampled, **ZERO sign flips**,
  median |pred−measured| ≤16% everywhere (P90 +0.5..+83%; synth-timeseries P90
  +2219% tail = len/8 mask term OVER-pricing long matches — pessimistic
  direction). (b) Record-file aggregate delta **+0.0000% exact, CV=0** by
  construction (measured-payload arbitration over {keep-all, drop-R1,
  drop-R1∪R2}). (c) Encode 1.10–1.40× greedy-class (PASS ≤2×; repeat.jsonl 2.36×
  = degenerate control, absolute 30.8 ms). (d) Decode −4.6..+7.6% (PASS ≤10%).
- **Finding F1 (headline): EXP. X's +0.0443% regression is TRAJECTORY-borne, not
  token-borne.** Every committed type-3 token is individually sound (keep
  2.4–4.8 B vs 32–39 B literal alternative); dropping the len≤8 class — or ALL
  2,377 type-3 tokens — enlarges EVERY block; yet the pnra=off parse is 375 B
  smaller. PNRA commits displace better downstream matches via hash-chain
  insertion; **no post-parse rollback can recover that**. Corroborates u5 from
  the other side (parse-TIME threshold works because it changes the trajectory:
  845,657 < 845,675).
- **Finding F2:** the local heuristic is well-calibrated ON WHAT IT COMMITS —
  the ledger's "underpricing" refines to "**global displacement cost exceeds
  local gain**, invisible per-token."
- Integration recommendations: do NOT integrate post-parse rollback for EXP. X
  recovery; DO adopt the per-symbol measured-cost attribution (~200 lines,
  encoder-only) as shared calibration infra; measured-payload arbitration is the
  safe shape for any future refinement pass (≤0 by construction); a
  displacement-targeting rejection pass would be DP-class (EXP. F territory),
  out of greedy-class scope. Byte-faithfulness anchor: baselines reproduce EXP. X
  sizes exactly (108,514/108,518; 846,050/845,675).

### 2.4 S6-2 acceptance threshold (pnra-cost, `deliverable/u5`) — formula FROZEN; honest prediction recorded: held-out FAIL expected

- **Frozen stated formula:** accept iff (alt_c − pnra_c) ≥ **0.5 · FramingRaw(L,D,F,R)**,
  FramingRaw = 8 + 8·vb(L−4) + 8·vb(D−1) + 32·⌈L/32⌉ + 32·⌈⌈L/4⌉/32⌉ + 8·R (raw
  fixed wire bits). γ=0.5 is the ONLY constant = measured coded/raw framing-ratio
  band midpoint (masks .435, tmask .153, types .169, ml .59, ds .93). Flat and
  distance-aware families rejected by sweep data; one-way flips only; non-PE
  no-op preserved (byte-identical).
- Calibration (pinned PEs ONLY): first config to beat `--pnra=off` on BOTH —
  anvil.exe 108,506 vs 108,518 (**−0.0110%**, commits 429→29); anvil_bench.exe
  845,657 vs 845,675 (**−0.0021%**, commits 1,599→80). Kept commits median L=16–18.
  Provenance settled: EXP. X counters reproduce EXACTLY on current on-disk bytes
  (drift predates EXP. X; resolves corpus-expand's manifest-drift flag).
- Refined root cause (new measurement): over-commit has TWO components — framing
  underpriced AND alternative overpriced (formula prices the whole span as
  literals; re-search recovers non-field bytes; counterfactual Σ +1,486/+2,195 B
  vs true e2e −4/+375 B).
- **Honest prediction, recorded before any held-out datum:** calibration ceiling
  −0.002..−0.03%/PE, ~100× below the ≥0.5% held-out bar → **expected verdict
  FAIL** (clean negative acceptable). Frozen protocol §8: held-out = the 6
  corpus-expand PEs only; round-trip + fuzz ≥120 cases first; median-3; PASS =
  aggregate ≤0.995·off AND decode ≥0.9·off per PE AND non-PE byte-identical; NO
  re-tuning; retry requires a new pre-registration.
- Reconciliation note (verify/ariref u6): the flip-count sentences (391/429,
  1,502/1,599) don't reconcile with commit-count arithmetic (400/1,519) under
  one-way monotonicity — flagged to pnra-cost/ledger as a counting-basis item to
  resolve before PART XII appends. This doc cites the commit counts.

### 2.5 Corpus expansion (corpus-expand, `deliverable/u3`) — anti-overfit substrate + structure carrier READY

- **6 held-out x64 PEs, 5,540,960 B total, 4 producers / 2 toolchains** (MSVC +
  MinGW/GCC), pinned by SHA-256: pe-winver 28,672 B (orientation ratio 0.1949),
  pe-where 61,440 (0.3640), pe-notepad 360,448 (0.5644), pe-python 103,704
  (0.5228), pe-ninja 603,648 (0.4859), pe-git 4,383,048 (0.4863). Satisfies the
  S6-2 contract (≥4–6, distinct from the pinned pair).
- **3 structure-carrying files:** synth-columnar-align.bin 276,000 B (row-
  interleaved AP columns at prime-23 B misaligned offsets + XOR parity; ratio
  0.4279), synth-drift-stride.bin 615,376 B (sawtooth-drifting 40→48 B stride;
  0.2339), synth-counters.log 1,107,326 B (multi-counter/timestamp text log;
  0.1147). Original 3 synth files regenerate byte-identical; determinism
  double-run verified.
- Integrity: all 12 pre-existing data files byte-identical; manifest 21/21 OK.
  Pre-existing anvil_bench.exe manifest drift documented (and later settled by
  u5's exact counter reproduction). Corpus now 19 data files ~19.8 MB at u3 time
  (22 with the decode-swarm additions).

### 2.6 Prior-art gate (priorart, `deliverable/u4` + `docs/priorart-tcopy-external.md` §Pass 5) — TCOPY CLAIM STANDS

Intel US 7,111,148 B1 (all 39 claims) / US 7,010,665 B1 (all 34) / continuation
US 7,617,382 B1 (32) retrieved verbatim from two independent sources (Google
Patents + FreePatentsOnline, matching). The family is CPU microarchitecture —
compaction of RIP-relative operands of decoded μops in on-die storage (G06F9/26,
711/220): reconstruction uses a STORED per-line head instruction pointer plus a
TRANSMITTED 2-bit correction field ('148 cl.9-10). No file/stream, no LZ parse,
no backreference copy, no match distance anywhere in the claims — so no implicit
Δ=−d, no match-level transform, no pre-LZ filter. Both patents EXPIRED
(fee-related, 2022/2023). Ledger C12 condition (2) DISCHARGED; condition (1)'s
experimental side is reported discharged by Experiment Z(b)'s explicit-Δ control
(ariref's claim — note the forms differ: TCOPY's Δ=−d is exact algebra, ARI's
σ·(d/4) is estimator-based; the ledger should record the distinction when closing
C12). Five passes converge: no accessible prior art teaches single-file
self-referential LZ copy with implicit distance-derived additive transform at
match level.

### 2.7 Independent verification (verify + ariref, `deliverable/u6`) — LEDGER INTEGRITY CONFIRMED

10/10 published numbers reproduced byte-exact (EXP. S/L/X); all four peer
harnesses re-run from independent rebuilds (ariref 71/71 line-identical; oracle
calibration exact incl. zero flips in a 73,586-row dump; u5 γ=0.5 matrix exact;
corpus ratios + checksums cross-checked; '148 quotes verbatim-verified via
independent fetch). Three nits found and resolved (manifest drift re-pinned;
source-hash drift re-pinned; oracle gloss re-worded). One open reconciliation
item (u5 flip-count basis, §2.4). "No claim exceeds its evidence anywhere in the
staged record."

---

## 3. The frontier arithmetic, recomputed on fresh numbers

Per-file escape rule (bench, `context/s61-frontier-bar`, generated.log instance):
an anvil row escapes a reference row iff `ref.ratio > row.ratio OR ref.decode <
row.decode`; it extends the front iff it escapes ALL reference rows. Applied to
exact bytes in `tests/benchmark-suite.pre-s61.csv` (best anvil row per file):

| file | best anvil (bytes, dec MB/s) | binding dominator(s) | gap to escape |
|---|---|---|---|
| synth-arith.bin | 256,022 (1.000) @ ~396 | q11 87,013 @ 210.5; zstd-19 106,614; q9 114,417 | **EXP. Z harness wire = 89,363 (+2.7% vs q11)** |
| generated.json | mdl 89,589 @ 150.1 | q11 79,172 @ 651.8 — the ONLY dominator left | **−11.6% bytes** (or dec >652) |
| synth-timeseries.bin | sparse-ch 143,132 @ 126.5 | q6/q9 120,669 @ 287; q11 134,718 | −15.7% (vs q9) |
| generated.jsonl | mdl 183,506 @ 186.6 | q11 150,619 @ 956; zstd-19 166,693 @ 1869 | **−18.0%** (vs q11) |
| src.cpp | dp-arith 0.292 @ 33 | q11 0.246 | −15.5%-class |
| anvil.exe / anvil_bench.exe | mdl 0.400 / 0.430 | q11 0.329 / 0.335 | −18..−22%-class |
| doc.md | dp-arith 0.530 | q11 0.384 | −28%-class |
| generated.log | shape 129,739 @ 229.3 | q11 81,946 @ 936; zstd-19 106,264 @ 1919; q9 124,669 @ 620 | −36.8% (vs q11) at any decode; −39.4% w/ dec >1919; mode-15 hotop 175,550 (0.090) cannot extend at ANY decode (≥0.064178 band) |
| generated.sqlite | mdl 0.186 | q11 0.107 | −42%-class |

Readings:

1. **The record-file frontier is ratio-blocked first.** On generated.log the
   hotop row's ratio (0.090) exceeds the q9 band edge (0.064178) — no decode
   speed helps. Even the best anvil row (129,739 B) needs −36.8% to pass q11.
   S6-1b's decode lift is necessary infrastructure but extends nothing alone.
2. **generated.json is one mechanism from history.** Exactly one dominator
   remains (q11, −11.6% needed). For scale: EXP. S's ctx literals bought
   −12.4% on this file as a single mechanism — the gap is the size of the
   project's strongest single ratio win, but no current mechanism claims it ON
   TOP of ctx-mdl.
3. **synth-arith is the closest any anvil artifact has ever been to a front.**
   The Experiment-Z harness wire (89,363 B) is +2.7% from q11's 87,013 B and
   already beats every other reference row. This cell did not exist before I5;
   the primitive that reaches it did not exist before I7.

---

## 4. Which horizon most likely breaks the streak next — ranked, with citations

**Rank 1 — mode-16 ARI-REF (transmitted Δ) landing on synth-arith.bin near its
harness wire.** Why, on the numbers: (i) the representation gain is measured and
corroborated twice (−65.25% harness, u1; reproduced exactly by priorart and
verify); (ii) the gap-to-front is measured at **+2.7%** (89,363 vs 87,013) — the
smallest gap to a reference-front extreme ever recorded in this project;
(iii) the missing layer between harness wire and container is entropy coding,
where the project's validated machinery (rANS suite + ctx mode 6, EXP. S:
−8.3..−16.2% end-to-end on record files; J-selection 100% faithful, EXP. L)
historically buys single-digit-to-teens percent — plausibly more than the 2.7%
needed, though container overhead works against it; (iv) decode is structurally
cheap there (copy + word-adds + sparse stores; q11 decodes that file at only
210.5 MB/s); (v) the frozen gate bar (≤0.80× = ≤204,800 B) is deliberately
conservative, so the integration proceeds even if the frontier question answers
"not yet". Honest caveats: the harness wire is uncoded and container-less — the
+2.7% is a projection anchor, not a measurement of the container; family
boundaries are real (timeseries +0.03%, columnar-align 0 tokens, counters
+0.37% in-harness); and the mode-16 pre-reg binds that no Pareto claim may be
made unless `tools/pareto_front.py` says EXTENDS_FRONT. This is a falsifiable
prediction, not a claim: **if the mode-16 verdict build lands synth-arith
end-to-end ≤87,013 B, the tools should emit the project's first EXTENDS_FRONT.**

**Rank 2 — a record-file ratio mechanism sized −11.6% on generated.json.** The
cell analysis (§3) names the exact target for the first time. No current
mechanism claims it; candidate DIRECTIONS (un-measured hypotheses, listed only
to size the problem, each requiring its own novelty gate + pre-registration):
entropy-side work on the hot-op book streams beyond ctx-mode-6; mdl-class
measured-cost parsing fused with the hot-op representation (EXP. F precedent:
−8.6..−11% on record files); Exp-Y's −10.31% book-byte win re-shaped into a
decode-neutral form (its eager materialization is what failed T2, not the
smaller book). None of these is a lead yet — they are the honest search space
that the −11.6% number now defines.

**Rank 3 — S6-1b decode-floor lift (CRC + materialization + alloc).** Not a
streak-breaker by itself (byte-identical output; every row stays
ratio-dominated), but it converts FLAG-A from "~3–4× behind" to "approaching
parity" (projected 450–480 MB/s full-CRC on log, +alloc leg, vs q9's 619.5;
expected band 460–565 two-leg per the freeze) and it lifts EVERY mode's decode
(CRC is format-level), including Rank-1's and Rank-2's future rows. Highest
EV/effort on the decode plane; engineering-adopt class; RUN-VOID if any output
byte differs.

**Not rankable as streak-breakers, recorded for completeness:** S6-2 held-out
execution (predicted clean negative — closure value, §5); reference-row
reconnaissance on the 9 new corpus cells (information that could re-rank I9;
counters.log at 0.1147 default-settings is the most interesting unknown).

---

## 5. Dead ends to close permanently (with the measured reason) vs live threads

**Close permanently:**

1. **Post-parse rollback/rejection as the EXP. X recovery lever** — oracle F1:
   regression is trajectory-borne; every committed token individually sound;
   dropping any class enlarges every block; un-recoverable post-parse. (The
   oracle's arbitration machinery stays as infra; the LEVER is dead.)
2. **Stream-budget-only route to 2× decode / bar A on Windows mode 15** — t3
   profile: CRC 44% + materialization 26% dominate; opcode pulls 0.3%;
   budget-only cap ≤2–5% under bar B; macro-dvar already raw (Linux lever does
   not transfer). Formal recording pending the t7 verdict; the profile is the
   pre-staged explanation either way.
3. **Implicit Δ=σ·(d/4) as the default ARI form** — EXP. Z(b): +38.35% vs
   transmitted on corpus; exact-progressions-only (−15.38% there). Excluded
   from mode 16 by frozen gate.
4. **RLZ/RePair book-stream codecs as default** — Exp-Y: decode −8.1..−15.9% on
   record files, encode 48–64×; ratio-only point. Landed default-off as the
   measured record; do not re-burn as a default-path candidate.
5. **Word-aligned constant-Δ ARI on non-×4-aligned / per-field-delta layouts** —
   EXP. Z(a) timeseries +0.03%; EXP. W alignment-sensitivity; u3's misaligned
   columnar-align drew 0 harness tokens. Family boundary, recorded.
6. **Flat / distance-aware PNRA acceptance-threshold families** — rejected by
   u5 sweep data (shape-scaled γ=0.5 won); folded into the frozen formula.
7. **(carried) tANS table construction** — NOT APPLICABLE, independently
   confirmed twice (ledger notes at lines 2381/2756).

**Live threads:**

- **S6-1b legs 1–3** (frozen pre-reg `deliverable/pre-reg-s6-1b`; leg-3
  alloc/concat amendment requested pre-implementation; expected band updates to
  the three-leg sum at freeze).
- **mode-16 ARI-REF integration** (frozen pre-reg `deliverable/pre-reg-i8-ari-ref`).
- **S6-2 held-out verdict execution** (frozen protocol §8; prediction recorded).
- **Oracle attribution infra adoption** (~200 lines encoder-only; approved
  shared calibration infra; own mini-pre-reg if adopted).
- **Structural-mechanism valuation on the new corpus** — substrate ready
  (columnar-align / drift-stride / counters.log); expectations tempered by the
  measured harness boundaries above; alignment-sensitivity remains the open
  engineering gap (EXP. W finding i).
- **The −11.6% generated.json problem** — newly precise, unsolved, the record
  corpus's representative open problem.

---

## 6. Wire-overhead removal vs tuning (the §6 framework, applied to the new mechanisms)

Criterion (unchanged from `docs/swarm-i6-strategy.md` §6): removes per-token/
stream wire cost or decode passes at the decoder = genuine; adjusts an
acceptance budget / weight / knob without removing wire cost = tuning.

- **ARI-REF transmitted-Δ token (mode-16 form): GENUINE** — replaces a 4W-byte
  literal run with varint(W−1)+varint(d−1)+one svar Δ+mask+residuals; on its
  family it removes whole literal-run wire classes (measured −65.25% vs
  exact-LZ wire). The zero-bit implicit variant would have been stronger still
  and FAILED its ablation — the claim correctly rests on the transmitted form.
- **S6-1b (CRC/materialization/alloc): neither** — zero wire change by design
  (RUN-VOID guard); implementation economics that move the decode plane. Its own
  gate class (engineering adopt) already forbids calling it a Pareto claim.
- **S6-1 whole-codec budget: tuning on top of the hot-op remover** (I6 §6
  classification stands) — and now measured-bound to ≤2–5% under bar B.
- **S6-2 γ-threshold: tuning** — calibrates acceptance, removes no wire;
  consistent with its predicted small ceiling.
- **Measured-payload arbitration + attribution infra: tuning/calibration** —
  safety property (≤0 by construction), never a claim alone.
- **Exp-Y RLZ/RePair: genuine wire removal that fails the economics gate** —
  −10.31% book bytes is real wire reduction, but decode/encode costs reject it
  as default. Reminder the framework's other half binds: wire removal is
  necessary, not sufficient.

---

## 7. The Iteration-8 plan (ranked by EV/effort) and DAG sketch

Ranked leads (all pre-registerable, falsifiable; several gates already frozen):

1. **S6-1b decode-floor lift** — EV highest / effort low-medium. Wire-invisible,
   byte-identical-or-void, frozen bar (log ≥547.1 MB/s), staged legs for clean
   attribution (CRC → materialization → alloc), decode-perf attribution pass
   after each. Unlocks decode-plane credibility for everything else.
2. **mode-16 ARI-REF integration + end-to-end verdict** — EV high / effort
   medium. Frozen gate; transmitted-Δ only; primary synth-arith ≤0.80×
   (≤≈204,818 B); full-suite pareto rows emitted by the arbiter tooling — the
   frontier question (≤87,013 B) answers itself there without any bar change.
3. **S6-2 held-out verdict execution** — EV moderate-as-closure / effort very
   low. ~10-line arch integration of the frozen formula; run protocol §8 on the
   6 PEs; predicted clean negative closes the EXP. X thread permanently with a
   measured ceiling; NO re-tuning regardless of outcome.
4. **Reference-front reconnaissance on the 9 new corpus cells** — EV potentially
   ranking-changing / effort very low. Bench rows (brotli q1/q4/q6/q9/q11 +
   zstd tiers) on columnar-align/drift-stride/counters.log etc.; recompute the
   §3 table; if any new cell shows a ≤~12% gap-to-front, it jumps the I9 queue.
5. **Adopt oracle attribution infra** — EV enabling / effort low; mini-pre-reg;
   powers future calibration (J-work, threshold families, §7-rank-2 sizing).
6. **The −11.6% generated.json mechanism search** — EV frontier-grade / effort
   unbounded until a mechanism clears the novelty gate; the number is now
   precise; treat any proposal against it.

**DAG sketch (respecting frozen sequencing gates):**

```
A [decode-swarm] S6-1 t7 verdict (S6-1b-free build) ──► PART XI
A ► B [arch] S6-1b legs 1→2→3 (staged commits/flags; byte-identity gate;
     decode-perf attribution after each leg)
B ► C [arch] mode-16 ARI-REF integration (transmitted-Δ only) ► bench verdict
     (Leg1 ≤204,800 B synth-arith; Leg2 decode co-arbiter; Leg3 no-regression)
A ► D [pnra-cost+arch] S6-2 ~10-line integration ► frozen §8 held-out verdict
E [bench, parallel anytime] reference rows on 9 new cells + refreshed noise floor
F [verify, continuous] reproduce headline numbers as they land (u1/u2/u5/Exp-Y done)
G [ledger] PART XI (decode swarm) + PART XII (this swarm) staging; resolve the
   u5 flip-count basis item before append
```

Serial-lane note: arch owns src/anvil.cpp solely, so B→C is serial; D's
integrated-build step queues behind B/C in the same lane but its prototype-side
verification is done. E/F/G are independent.

---

## 8. Coordination notes

- **decode-swarm coordinator/arch:** the t7 verdict should land on a build
  snapshot BEFORE mode-16 code enters the tree (their own sequencing gate) —
  this doc's Rank-1 depends on that attribution cleanliness.
- **pnra-cost/ledger:** resolve the flip-count counting basis (391/429 vs 400)
  before PART XII appends; cite commit counts (429→29, 1,599→80) meanwhile.
- **bench:** the §3/§7-rank-4 reconnaissance is one suite run on the existing
  tooling; it could re-rank I9 outright.
- **ariref:** mode-16 consult rights during integration (family boundaries:
  d≥L, L=4W aligned, mask semantics) live in `prototypes/orbit_ariref/`.
- **housekeeping:** `tools/pnra_measure.py` still writes scratch to the Temp dir
  (OUT constant) — vestigial, but update to an in-workspace path if re-run.

---

## 9. Sources grounded

- Blackboard deliverables: `anvil-i7-decode`: pre-reg-s6-1 (v1.2),
  verdict-skeleton-s6-1 (v4), audit-s6-1-linux-ref (v6), t3-decode-floor-profile,
  t3-crc-share-stability, exp-y-verdict, t6-correctness-gate-baseline (v3),
  t-format-stream-selection-wire-spec (v2), pre-reg-s6-1b, pre-reg-i8-ari-ref,
  context/s61-frontier-bar. `anvil-i7-cost`: u1 (Experiment Z), u2 (S6-3
  oracle), u3 (corpus), u4 (priorart Pass 5), u5 (S6-2 threshold), u6
  (verification matrix v3).
- `RESEARCH_LEDGER.md` Parts II–X (EXP. L/S/X anchors; Experiment Y/Z
  pre-registrations; consolidations).
- `tests/benchmark-suite.pre-s61.csv` (frozen per-file baseline; exact bytes
  used throughout §3), `tests/benchmark-summary.pre-s61.csv`.
- `docs/swarm-i6-strategy.md` (I6 synthesis; §6 framework reused), `docs/CONTEXT.md`
  (Linux hot-op/stream-budget history), `docs/priorart-tcopy-external.md` §Pass 5.

**Standing disclaimer:** none of this is a claim. Every ranked item above is a
pre-registrable, falsifiable test with a named arbiter (median-N bench +
`tools/pareto_front.py`), and the two frontier-relevant predictions (mode-16
lands ≤87,013 B on synth-arith; S6-2 held-out fails its ≥0.5% bar) are stated so
they can be right or wrong on the record. If the S6-1 t7 verdict contradicts the
expectation recorded in §2.1, this doc gets patched verbatim — the gate text,
not this document, is the authority.

---

## 10. Addendum (2026-08-21, post-freeze): the Gorilla / vertical-XOR competitive dimension

*Added after the coordinator's GROTLI × GORILLA × ANVIL research briefing
(msg_6ce6bc0a). This section extends §4/§7; it does not amend any frozen gate.
Every grotli/gorilla number below is OPERATOR-REPORTED from an external
codebase (`C:\Users\<user>\WebstormProjects\grotli-codec`, grotli.ts ≈2000
LOC + refs) — none is measured on ANVIL's bench, host, or corpus, and per the
project's standing rule (cf. Experiment Q's "2026 citations are
operator-reported — verify each before citing") none may be cited as evidence
until reproduced here.*

### 10.1 What arrived (grounding status per figure)

- Gorilla DoD timestamp tiers (5-tier zero/9/12/16/36-bit) and value XOR cases
  (A/B1/B2): published algorithm (Pelkonen et al., VLDB 2015) — the ALGORITHM
  is citable prior art; the grotli.ts implementation's measured outcomes
  (e.g. "98.3% timestamp reduction on 1000-pt 60 s σ=0.3 s", "+14.06% bloat on
  uniform random") are operator-reported model demonstrations, not ANVIL
  measurements.
- "vXOR on NDJSON −36.3% vs Brotli q5 (H0=0.637, zero-density 93.2%,
  measured-approx)": operator-reported, external host/file, "approx" by its own
  label. NOT comparable to any number in §3.
- Auto-detect heuristics (FFT autocorrelation stride detection validated
  4.5e-13 vs naive; Shannon window H; exponent stability S; bitwise variance
  G_s; highByteFraction ASCII guard; the routing rule): an implemented,
  validated-on-their-side heuristic stack — a serious model to port, but
  ANVIL-side thresholds must be re-frozen on OUR corpus before any verdict.

### 10.2 Analytic impact on the §3 frontier map: complementary, cell by cell

The briefing is right that Gorilla/vXOR competes on the synth file class — but
the landed I7 measurements say the competition is better read as a PARTITION of
the three synth cells between two mechanism families:

| cell | ARI-REF (mode-16 family) | Gorilla/vXOR family | measured basis |
|---|---|---|---|
| synth-arith.bin | **primary** — harness wire +2.7% from front (u1) | plausible but unproven: block-wise AP columns are also DoD-friendly; q11 already at 0.340 there | u1 (a) PASS −65.25% |
| synth-timeseries.bin | **measured FAIL** (+0.03%, 33 tokens — 14 B stride not ×4-alignable; per-field deltas outside the word-aligned constant-Δ family) | **the natural primary**: u64 ts Δ=1000±50 ms jitter is exactly DoD's design case; f32/f64 value XOR on a small random walk is Case A/B1 territory | u1 (a) FAIL; EXP. W |
| synth-columnar-align.bin | **measured zero** (0 harness tokens — prime-23 B misalignment defeats the word-aligned scanner) | **the natural primary**: vertical/field transposition sidesteps byte-alignment entirely by de-interleaving before coding — the exact capability EXP. W recorded as the alignment-sensitivity gap | u1 via mode-16 freeze notes; EXP. W finding (i) |

So the I8 question on this class becomes a **two-family, three-cell matched
comparison** — which is stronger than either family alone: mode-16 keeps
synth-arith (its measured +2.7%-from-front position is untouched by this
briefing), and the Gorilla family gets two cells where the landed evidence says
ARI-REF cannot reach. The coordinator's requested matched-tier Pareto table
(ARI-REF mode-16 vs Gorilla-Px vs vXOR-Pv on the triple) is adopted as the
VERDICT SHAPE for whichever gates freeze — with the strict condition that no
DOMINANT headline may be claimed without smooth-telemetry measured H0 on our
host (the briefing's own rule, which matches house discipline).

### 10.3 Novelty-gate honesty (before anyone builds)

- **Gorilla itself is prior art** (VLDB 2015, extensively deployed since:
  time-series databases, columnar formats). Delta-of-delta and XOR-frame
  coding cannot be ANVIL's mechanism-level claim, exactly as context
  clustering could not be (Linux verdict 3: RFC 7932 §7).
- **Codec auto-detection per column/block is also established art**
  (operator-cited lineage: BtrBlocks-class systems; zstd/brotli mode routers).
  The heuristic stack is valuable ENGINEERING, not a novelty claim.
- The narrow defensible lane, mirroring the Linux quantizer precedent, is the
  **interaction**: (a) one orbit-invariant framing where integer
  delta-of-delta, float XOR-reuse, and ARI-REF's transmitted-Δ are variants of
  one "model-next-element-from-decoder-visible-state + code the residual"
  family — a research HYPOTHESIS (the algebras differ: XOR vs ADD; streaming
  per-sample vs phrase-copy), not a validated equivalence; (b) measured-cost
  tier selection (oracle's infra scoring J = B + λ·C per tier, the B1-reuse
  decision) inside ANVIL's J-contract; (c) the no-worse guard set
  (ε_strict + AUTOSQUEEZE + ASCII-guard) as router-gated, byte-identical-when-
  not-fired infrastructure. Absent a measured ablation showing the interaction
  beats the parts, this lands as enabling infrastructure — same class as C7.

### 10.4 Revised I8 ranking (delta against §7)

- **Rank 1 unchanged:** mode-16 ARI-REF on synth-arith (frozen gate; measured
  +2.7%-from-front anchor).
- **Rank 3 (S6-1b), Rank 4 (reconnaissance), Rank 5 (attribution infra),
  Rank 6 (json −11.6%) unchanged.**
- **NEW Rank 2b — timeseries-family gate (pre-registration ONLY, no code until
  freeze):** falsifiable definition of the sweet spot (smooth f64/f32 telemetry
  only — the grotli model's own gate says random/uniform stays NEUTRAL, and
  that neutrality is a FEATURE to pre-register, not a weakness), the guard set
  as binding constants, auto-detect thresholds re-frozen on OUR corpus, and
  targets on synth-timeseries + synth-columnar-align + a NEW smooth-telemetry
  corpus file. **Hard dependency:** corpus-expand must add the smooth-sensor
  f64 + real NDJSON-columnar files FIRST — without the sweet-spot substrate the
  verdict is unfalsifiable in the favorable direction (the guards would
  correctly fire neutral everywhere). Sequencing: behind S6-1b on arch's
  serial lane, in competition with mode-16 for the slot AFTER it; the
  matched-tier Pareto table (mode-16 vs Gorilla-Px vs vXOR-Pv on the synth
  triple) is the shared verdict artifact whichever lands first.
- **DAG delta:** insert node `B2 [research-gate + corpus-expand] timeseries
  family pre-reg + sweet-spot corpus` in parallel with B (S6-1b); its
  implementation node queues behind C (mode-16) on arch's lane unless the
  coordinator re-orders. verify's matrix extends to Gorilla round-trip
  bit-exactness + DoD tier boundaries + vXOR frame detection when the time
  comes; ledger tags every grotli-derived figure with derivation class
  (measured-exact / modelled-capacity / operator-reported) per the briefing.

### 10.5 Framework application (§6 rules, new mechanisms)

- **DoD timestamp tiers / XOR value cases: GENUINE wire-overhead removers** on
  their file class — they replace fixed-width fields with variable-length
  codes whose length tracks the actual entropy of the delta sequence (the same
  class as TCOPY's zero-bit θ: wire cost proportional to information, not
  format).
- **Vertical XOR (field transposition + per-field coding): genuine** — it
  removes the cross-field wire noise that row-interleaved layout forces; note
  it is the TRANSFORM the Linux line's global lane-transpose experiment
  REJECTED when applied globally before LZ (CONTEXT: "the transform must live
  INSIDE the reference") — columnar-vXOR applies it per-column-block with a
  router, which is a different, gated thing.
- **Auto-detect heuristics + guards (H/S/G_s/ASCII/ε_strict/AUTOSQUEEZE):
  tuning** — router calibration, no wire of their own; their value is making
  the genuine mechanisms safe to enable by default.
- **B1-reuse tier selection: tuning** — a J-decision inside the block, exactly
  the class the oracle's measured-cost infra scores.

*This addendum changes no frozen gate and claims nothing. The Gorilla family's
ANVIL-side numbers do not exist until a gate freezes and a harness measures.*
