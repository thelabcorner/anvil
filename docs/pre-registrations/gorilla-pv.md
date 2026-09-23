# GORILLA/Pv PRE-REGISTRATION — timeseries auto-detect adopt — v1 (research-gate)

> Provenance: drafted in swarm anvil-i7-decode session; blackboard publish
> failed due to swarm-store outage — this repo file is the FIRST durable copy
> and is the binding frozen text. Constants verified against grotli.ts primary
> source before freezing.

Frozen: 2026-08-21, pre-implementation.

Gate class (BINDING, per linux-ref CORRECTION 3): ENGINEERING ADOPT of
published prior art — Gorilla (Pelkonen et al., VLDB 2015): XOR float
compression + delta-of-deltas timestamps; vertical-XOR record alignment
(grotli). BCJ precedent applies: control/enabling, NOT novelty-gated —
Pareto-gated. NO mechanism-level claim available unless a future ablation
proves a genuinely new interaction (e.g., Gorilla residual streams composing
with shape-conditioned displacement in a way neither achieves alone —
plausible, unproven, would need its OWN pre-registration).

## TWO-PHASE STRUCTURE (linux-ref FLAG 1, binding)
- PHASE A — ISOLATED PROTOTYPE (TCOPY/PNRA pattern): prototypes/gorilla_probe/
  — representation value measured harness-style (EXP Z precedent: counted
  wire, relative deltas only) against ANVIL's existing modes on the SAME
  files. NO src/anvil.cpp touch; does not conflict with the S6-1b sequencing
  gate.
- PHASE B — INTEGRATION AS BLOCK-GRANULAR ROUTING ONLY: when detection fires,
  the whole block routes to the Gorilla/Pv codec INSTEAD of the LZ pipeline
  (difference-cover negative-gate pattern) — NEVER a pipeline stage feeding
  ANVIL's match parser. The briefing's "stack them (<P,R> dependency order)"
  phrasing is REJECTED as built-shape: global pre-parser transforms are the
  record's DECISIVE rejection (lane-transpose: every tested ELF section grew;
  transform must live INSIDE the reference). Queues behind S6-1b AND
  I8/mode-16 in arch's serial lane.

## §1 LINEAGE
- Gorilla VLDB 2015 (published prior art): DoD timestamp tiers; IEEE-754 XOR
  value coding with lead/trail-zero block reuse.
- grotli-codec measurements (EXTERNAL PROJECT, directional — FLAG-B/C analog):
  NDJSON Pv −36.3% vs brotli q5 (their generator, their build); smooth-f64
  dominant win proven only after adding a smooth generator (their original
  sensor gen was Gorilla-hostile); JS BigInt decoder defects cap their decode
  at 10–30 MB/s (D1-D6) — their decode numbers are NOT expectations.
- grotli honesty precedent (verified in source :2017/:2722): modelled-capacity
  claims ("×0.60 fiat", "fabricated constant") RETRACTED in favor of measured
  — derivation-class tags MANDATORY in our ledger rows (measured-exact /
  measured-approx / modelled, per FINAL §3.1).
- ANVIL record fit: EXP Z diagnosed synth-timeseries miss as per-field delta
  over non-aligned stride — Pv with lossless stride + serialized record
  lengths directly addresses that family boundary; EXP W: current corpus lacks
  periodic/arithmetic signal (drives the corpus precondition below).

## §2 WHAT IS NEW (honest: nothing mathematical)
Transplant of published techniques + an auto-detect router branch. The adopt's
value proposition is evaluation coverage (timeseries domain ANVIL lacks) +
measured engineering value, gated by the standing honesty gates.

## §3 BINDING CONSTANTS — frozen from grotli SOURCE, verified by research-gate
BEFORE any ANVIL corpus result exists (anti-circularity guard, linux-ref
FLAG 2):
- Detection (grotli.ts:788-789, 868-874): S_THRESH=0.60; R_THRESH=0.15; route
  iff !isAscii && entropy<7.5 && S>0.60 && G_s>0.55 (with expVar<mantissaVar)
  && (autocorr>0.15 || strongFloat: S>0.85 && entropy<7.6 && G_s>0.6); ASCII
  guard highByteFraction<0.02 ⇒ NEVER route.
- Engagement gates: AUTOSQUEEZE H(residual) ≤ H_raw − 2%; ε_strict =
  max(2, 0.005·|block|).
- DoD tiers (grotli.ts:572-576, 601): zero→1b; [−63,64]→9b; [−255,256]→12b;
  [−2047,2048]→16b; else 36b; Δ₁ 14-bit.
- XOR cases (grotli.ts:431-class): A=1b (xor==0); B1=2+mb (reuse prev lz/tz
  block); B2=13+mb (new block; lz 5b cap 31; mb 6b).
- Decode: two-lane u32 + normalized bit accumulator (BigInt class FORBIDDEN —
  grotli D1-D6 defect precedent).
- Pv losslessness: stride ≥ maxRecordLen + 1; record lengths varint-serialized
  on wire.
- Threshold sensitivity REPORTED, never tuned. THRESHOLD-FIT INVALIDATES THE
  GATE (standing rule). Detector constants are grotli's, adopted blind to ANVIL
  outcomes — if detection misfires on our cells, that is RECORDED, not fixed by
  adjustment (adjustment = new pre-reg).

## §4 CORPUS PRECONDITION (FLAG 2 guard)
- bench adds smooth-f64 telemetry synth cell(s) (grotli
  generateSmoothSensorFloat64 class); corpus-expand pursues REAL telemetry if
  obtainable.
- Verdict files labeled derivation-class SYNTHETIC until a real telemetry file
  confirms; the DOMINANT claim is scoped "synthetic smooth telemetry" until
  then. An evaluation-scope finding (ANVIL lacks real telemetry domain) is a
  complete honest outcome, not a failure of Gorilla.

## §5 FALSIFIABLE TARGETS
PHASE A (prototype, harness counted-wire, EXP Z pattern):
- (a) On smooth-f64 cell(s): Gorilla-XOR+DoD wire ≥20% smaller than
  same-parser exact-LZ baseline (relative-in-harness bar; absolute bytes NOT
  comparable to anvil.exe/brotli — EXP V/Z caveat stands).
- (b) Attribution/no-regression: on ALL existing corpus files the detector
  either does not fire or engages at neutral-or-better wire cost; random/
  urandom mandatory 0% reduction (CI FAILS if reduction>0); ASCII files never
  route.
- (c) Correctness: self-checked round-trip incl. all DoD tiers (zero→32bit) +
  fuzz over smooth/jittered/random/constant/wraparound families.
PHASE B (integration, block-routing, end-to-end):
- (d) On smooth cell(s): routed-mode end-to-end beats current-best-anvil
  config at equal-or-better decode (paired same-build median-3; two-lane
  decoder; decode co-arbiter per FLAG-A discipline).
- (e) Router-off byte-identity everywhere; full-corpus no-regression;
  independent-block CRC; wire-complete framing (format lane).
- (f) DOMINANT/EXTENDS_FRONT claim ONLY via tools/pareto_front.py, scoped
  class-specific-synthetic per §4.
PARETO NON-GOAL: no frontier claim outside the scoped cell without the pareto
tool's verdict.

## §6 SCOPE BOUNDARY
IN: prototype harness; detector (frozen constants); Gorilla-XOR/DoD/Pv codecs
as BLOCK ALTERNATIVES; FORMAT.md mode spec (Phase B); controls above.
OUT (smuggling VOIDS the run): Pv/Gorilla as pipeline stages feeding the LZ
parser (lane-transpose re-burn); detector tuning on ANVIL data; interaction
claims with sparse-ref/shape-displacement without their own ablation pre-reg;
BigInt-class decoders; changes to existing modes' wire. Discoveries → NEW
pre-reg.

## §7 HONEST-FAILURE FRAMING (pre-committed)
- Prototype fails even on favorable synthetic cells ⇒ falsification: the
  transplanted representation does not convert in a maximally favorable
  harness — do-not-re-burn (EXP J treatment).
- Prototype passes, integration neutral on the cell ⇒ recorded like grotli's
  own text/csv 0% truthfully — adopt value questioned, engineering-era
  analysis.
- Only synthetic cells exist ⇒ evaluation-scope finding recorded; mechanism
  parked validated-on-synthetic.
- Detector misfire on any real-corpus file ⇒ recorded as-is; constants not
  touched (§3).
Verdict authority: bench median-3 + tools/pareto_front.py (Phase B);
harness-relative deltas (Phase A). Verdict drafting: research-gate against
THIS frozen text.
Version history: v1 frozen pre-implementation; constants verified against
grotli.ts primary source (:572-576/:601/:788-789/:868-874); linux-ref
constraints (block-routing-only, anti-circularity, adopt-class) folded as
binding structure; coordinator's measured-H0 requirement honored via §4
derivation-class tagging.
