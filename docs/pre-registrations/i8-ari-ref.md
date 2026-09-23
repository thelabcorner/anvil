# I8 / MODE-16 PRE-REGISTRATION — ARI-REF integration — v2 (research-gate)

> Provenance: authored live in swarm anvil-i7-decode blackboard
> `deliverable/pre-reg-i8-ari-ref` (key v2 = text v2). Persisted to repo after
> swarm-store outage. Binding frozen text for the I8/mode-16 experiment.

v1→v2 AMENDMENT (pre-implementation; Leg 1 PASS BAR UNTOUCHED): STRETCH LEG
added per coordinator request, sourcing strategy's landed u7 re-rank
(docs/swarm-i7-strategy.md). STRETCH LEG (pre-hoc stated ambition, reported in
addition to Leg 1): mode-16 end-to-end bytes ≤ 87,013 B on synth-arith.bin
(= brotli-q11's size, the front extreme; harness wire 89,363 B is +2.7% from
it) ⇒ tools/pareto_front.py would emit the project's FIRST EXTENDS_FRONT.
HONEST FRAMING (binding in any verdict narrative): synth-arith is a
PURPOSE-BUILT arithmetic-structure corpus cell — an EXTENDS_FRONT there is
CLASS-SPECIFIC (the corpus's structure-carrying files exist precisely to make
such cells measurable), NOT a general-purpose victory claim; any headline must
carry that qualifier. The stretch leg does not alter pass/fail: Leg 1
(≤0.80× current-best-anvil) remains the ONLY pass bar; the stretch leg is the
recorded ambition with its own pre-committed outcome classes: (a) ≤87,013 B +
EXTENDS_FRONT ⇒ "first frontier row — class-specific (arithmetic cell)";
(b) >87,013 B but Leg 1 pass ⇒ "mechanism converted end-to-end; front not
reached — gap to q11 = <x%>"; (c) Leg 1 fail ⇒ existing §7 honest-failure
framing. Also recorded: pnra-cost already closed the flip-count item strategy
flagged (exact partition, reconcile.py assertion-passing). Sequencing
unchanged: mode-16 implementation stays behind S6-1b in arch's serial lane.

Frozen: 2026-08-21 (pre-implementation). Gate class:
mechanism-integration (Experiment-Z-validated representation carried
end-to-end). Sequencing gate: implementation queues BEHIND S6-1b; the S6-1/S6-1b
verdict builds must not contain mode-16 code.

## §1 LINEAGE (ledger PART X Experiment Z, priorart-corroborated)
- Representation validated: ARI-REF(d,L,Δ,R) — copy L=4W aligned u32 words from
  distance d (non-overlap d ≥ L), add constant per-word transmitted Δ, sparse
  single-byte residual mask R (mode-11 style). Harness (counted/varint wire):
  synth-arith.bin −65.25% vs exact-LZ same-parser baseline (bar ≥10%, passed
  6.5x); priorart reproduced exactly.
- Attribution control PASSED: real-corpus contribution ≈zero in tx mode.
- NARROWED (binding): implicit Δ=σ·(d/4) FAILED its ablation (+38.35% vs tx;
  exact-progressions-only; jitter amplification diagnosed). Gain is ENTIRELY
  transmitted-Δ.
- Family boundary (honest misses, NOT mode-16 scope): synth-timeseries +0.03%
  (per-FIELD delta, non-aligned stride); synth-columnar-align 0 tokens;
  synth-counters.log +0.37% in naive-wave harness.

## §2 WHAT IS NEW (mechanism-level)
First ANVIL token type encoding the additive (degree-1 finite-difference)
family: "delta from an arithmetic relation" — exact LZ cannot express it.
Integration novelty claim deliberately NARROW: validated representation carried
end-to-end with router-gated adoption. NOT claimed: implicit-Δ (failed),
per-field/unaligned deltas (outside family), search-formulation advance.

## §3 BINDING CONSTANTS (from the validated harness — no new tuning)
- Token form: ARI-REF(d, L=4W, Δ_transmitted, R) ONLY. Implicit-Δ EXCLUDED.
- d ≥ L; L = 4W word-aligned; Δ = one transmitted per-word constant (svar);
  R = flat per-byte mask (mode-11 style).
- Token acceptance wire-cost-gated AND router-gated at config level. Existing
  J-selection (λ=0.01 additive) UNCHANGED.
- THRESHOLD-FIT INVALIDATES THE GATE.

## §4 FALSIFIABLE TARGET (median-3, bench arbiter, paired same-build A/B vs
same build ARI-off)
PRIMARY FILE: synth-arith.bin (the only file with validated
representation-level evidence).
- LEG 1 (pass bar): end-to-end bytes ≤ 0.80 × current-best-anvil-config bytes
  (pre-mode-16 suite snapshot; currently ~1.0 ratio / 256,0xx B). Derivation:
  retains ≥30% of the harness's 65.25% relative gain.
- LEG 2 (decode co-arbiter): decode(ARI-on) not slower than decode(ARI-off)
  beyond bench timing noise, paired median-3.
- LEG 3 (no-regression, full corpus): router-gated ARI-on no file worse than
  same-build ARI-off; includes held-out PEs + all manifest files. Zero-fired
  files byte-identical ARI-on vs ARI-off (strong attribution control).
- STRETCH LEG (v2, ambition not bar): ≤ 87,013 B ⇒ first EXTENDS_FRONT — see
  header framing.
REPORTED, NOT REQUIRED: synth-counters.log + synth-columnar-align.bin;
synth-timeseries.bin (family boundary, out of scope); real-corpus token
counts/deltas (watch item incl. the −1.42% harness outlier — report, never
claim without per-file pareto verdict); position vs brotli q11 / zstd-1.
PARETO NON-GOAL (binding): NO Pareto / EXTENDS_FRONT claim unless
tools/pareto_front.py says EXTENDS_FRONT at median-3.

## §5 CONTROLS (ALL mandatory before any claim)
1. FLAG-OFF BYTE-IDENTITY: ARI disabled ⇒ output byte-exactly equals the
   pre-mode-16 build on ALL corpus files.
2. Round-trip all corpus files + fuzz BEFORE any number counts; decoder
   strictness per format rules (bounds d/L/W/Δ/mask; truncated-stream
   rejection; per-block CRC).
3. PROTOCOL: median ≥3 reps; ratio exact; throughput only ≥ ~100 KB files;
   verdict ONLY from tools/pareto_front.py + bench CSVs.
4. FORMAT.md mode-16 spec BEFORE verdict run; old files decode (additive mode
   number).
5. Attribution ledger: per-file ARI token counts + wire deltas in the
   experiment row.

## §6 SCOPE BOUNDARY
IN: mode-16 token type (transmitted-Δ form); step-hash candidate probe in
existing parser; router/cost gating; FORMAT.md spec; §5 controls.
OUT (smuggling VOIDS the run): implicit-Δ in any form; per-field or
non-word-aligned delta families; new entropy coders; J-selection constant
changes; search-formulation rewrites beyond the step-hash probe; TCOPY/PNRA
changes. Discoveries → NEW pre-registration.

## §7 HONEST-FAILURE FRAMING (pre-committed)
If Leg 1 fails end-to-end where the harness passed, separate: (a) search never
surfaces candidates in the real parser (implementation-era), (b) entropy coding
re-inflates Δ/residual streams (representation-vs-backend interaction), (c)
container/router overhead eats the gain (implementation-era).
Math-vs-implementation-era recorded per working agreement; do-not-re-burn
treatment like EXP. J if (a)+(c) exhaust with (b) clean. Leg-3 regression on
any file = hard fail regardless of Leg 1. Stretch-leg miss records the gap to
q11 without spinning.
Verdict authority: bench median-3 + tools/pareto_front.py. Verdict drafting:
research-gate against THIS frozen text.
Version history: v1 frozen → v2 (stretch leg added pre-implementation per
coordinator/strategy-u7; Leg 1 untouched; class-specificity framing binding;
pnra-cost flip-count closure noted).
