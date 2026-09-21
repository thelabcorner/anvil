# ANVIL — Iteration-6 Strategy Synthesis (Orbit-Program Compression × the cost-model-fidelity theme)

Author: `orch-strategy` (strategy researcher). Status: **analysis only** — no
production code, no benchmark, no claim. This document turns the Orbit-Program
Compression direction (`docs/ORBIT_PROGRAM_COMPRESSION.md`) and the measured
0-EXTENDS_FRONT streak (Iterations 1–5, `RESEARCH_LEDGER.md`) into concrete
pre-registrable next-leads. Every number is sourced from the ledger / agenda /
CONTEXT; where a mechanism has no Windows A/B evidence it is explicitly
flagged and NOT claimed.

---

## 1. Executive summary (the thesis)

The streak is **0 EXTENDS_FRONT in ~396 verdict rows across five iterations**
(EXP. F.1 → 34 RATIO-BEATS-SOME/14 NO-BEAT; I2 → 76/20; I3 → 127/23; I4 →
126/24; EXP. X → DOMINATED on both planes). The Pareto verdict is decided on
**both** the ratio-vs-decode and ratio-vs-encode planes per file and
aggregate. The binding constraint, stated repeatedly and never disproven, is
**decode throughput (FLAG-A)**: several anvil configs already beat brotli q1/q4
on *ratio* (and mdL beat q9's ratio on generated.json), but **no anvil config
has ever reached brotli's decode on any file** (`anvil-hotop-rans` is the best
anvil decode at ~195 MB/s vs brotli q9 ~548–819 MB/s, ~3–4x behind).

Two independent facts select the highest-probability streak-breaker:

1. **The only config in project history measured to beat brotli q9 on BOTH
   ratio AND decode on a single file is the Linux hot-op hybrid on
   generated.log** — `≈452 KB @ 0.87–0.99 GB/s decode` vs `q9 513 KB @
   0.84 GB/s`, encode `33 vs 19.6 MB/s` "winning all 61 paired encode trials"
   (CONTEXT §"HOT-OP HYBRID + STREAM BUDGET"). That is a frontier* point
   (it dominates q9 on that file). No other anvil row ever did this.
2. That crossing is **exactly the faithful whole-codec stream-budget cost
   model** (J-selection + raw-stream choice), which is the ONE cost model in
   the project measured 100% faithful (EXP. L, λ=0.01). On Windows it is the
   un-wired remainder: mode 15 (EXP. R) reached 194 MB/s; the recorded lever
   is "I4-3's economics applied to the hot-opcode stream itself."

By contrast, the **encoder-side** cost model that underprices candidate shapes
is the diagnosis for why the RATIO-side structural mechanisms keep washing out
(EXP. X: a length/distance-blind local heuristic commits short single-field
far-distance candidates whose real entropy-coded cost exceeds their estimate).
This is a genuine, actionable root cause — but it gates the *binary/structural*
lanes, whose realistic ceiling is small and unproven. It is the SWARM's active
fix (`pnra-cost`), and it deserves pre-registration — but it is not the most
likely streak-breaker; the decode leg is.

**Primary recommendation:** close the decode leg by reproducing the Linux
hot-op frontier crossing on Windows mode 15 via whole-codec J-selection +
raw-stream budgeting (S6-1, §5). **Secondary:** convert the shared cost-model
root cause into a general measured-cost candidate-acceptance oracle (S6-3) and
into a held-out PE-validated acceptance threshold for PNRA/TCOPY that does not
overfit the two pinned PEs (S6-2).

---

## 2. Current-state read: the leads under test vs the Orbit direction

| Lead (peer) | Mechanism | Where it fits Orbit | Status anchor |
|---|---|---|---|
| PNRA cost-fidelity (`pnra-cost`) | length/distance-aware candidate acceptance | the *implicit/zero-bit transform-parameter* + *invariant-indexed search* families — the enablement that, if fixed, turns EXP. X's wash-to-regression into a real EMU candidate | EXP. X root cause; EXP. V first positive Windows signal (−0.23..−1.49% raw) |
| hot-op RLZ/RePair (`hotop-rlz`) | systematic grammar for the compiled hot-op book | the *conditional-program* / small-program decomposition (Brevis-lineage DSL synthesis, but cheap/deterministic) | EXP. R PARTIAL PASS — a ratio/regularity refinement of mode 15; complements (not replaces) S6-1's decode budget |
| corpus broadening (`corpus-expand`) | purpose-built periodic/arithmetic/structured files | the *latent-schema discovery* + *structural-distance* evaluation substrate | EXPs T/U/W: corpus measurably lacks real periodic/arithmetic structure (0.14–1.1% finite-diff hit = hash noise; SRR span-like 7–272/1000s) — a prerequisite for honest structural-mechanism valuation |
| tANS verify (`tans-verify`) | discrepancy-minimizing tANS table build (arXiv 2504.18541) | entropy-backend economics (enabling infra, not Orbit-unique) | **Ledger Note already concluded NOT APPLICABLE as proposed**: ANVIL is direct/byte-oriented rANS, no tANS state-machine coder exists, so the paper's table-placement freedom has no counterpart. Expected outcome: a verified negative, consistent with the ledger. |
| DP-parser latency/cost (`dp-parser`) | estimated-bit DP tuning vs greedy | the MDL-parser economics (R3) — measured-cost parsing was the strongest ratio mechanism but DP-class encode | EXP. F (mdl −8.6..−11% record files, encode 0.8–1.9 MB/s); STOP-PARALLEL: don't re-derive measured-cost from scratch |
| robustness | decode hardening / truncation safety | orthogonal infra; precondition for any ratio claim | decoder-audit + fuzz gates already hold |
| ledger-verify | independent verification + I6 synthesis | — | distinct from this strategy doc (verification, not forward-plan) |

Big picture: the active leads sit on **three different levers** — encoder
candidate-acceptance (pnra-cost), hot-op-book regularity (hotop-rlz), and
corpus/evaluation (corpus-expand). None of them *by themselves* crosses the
decode plane. The strategy below adds the missing **whole-codec decode cost
model** as the primary lever.

**Orbit mapping (why this direction is right but is being tested at the wrong
layer).** The Orbit document is a *representation* program (generalized
`REF(d,L,P,θ,R)` with low-cost programs). The project's measured failures have
all been **cost-model / discovery-cost** failures, not representation failures:
- TCOPY (θ zero-bit) validated — the representation is fine; EXP. O/X wash out at the *parse/acceptance* layer (greedy parse, local cost formula).
- PNRA (invariant-indexed search) validated — EXP. X failed on *per-token framing cost*, not on the invariant.
- SRR/topology (structural-distance) — the *coding layer* (k,slot modality) can't exploit the discovered structure; and the finite-difference *corpus* has no exploitable signal. Representation-style, not a math failure.
- The one place the Orbit direction *is* the bottleneck is the **decode leg**: Orbit's core discipline — "decoder stays tiny and fast" — is not yet *operative* on Windows; the 22-stream/materialization overhead is why anvil can't reach LZ4-class decode.

So: the Orbit direction is credible but it is being bottlenecked on the
**decoder-economics** layer, which is precisely the layer where the Linux
hot-op hybrid already produced frontier-level evidence.

---

## 3. The shared diagnosis: cost models under-pricing candidate shapes

The assignment's theme is confirmed by every ratio-side failure where a real
root cause was measured:

- **EXP. X (PNRA wired into mode 14, `--pnra=on`), the cleanest instance.**
  Invariant search fires for real (anvil.exe gate 1333 / idxhit 713 / verify
  615 / commit 429; anvil_bench gate 6919 / 2145 / 1958 / 1599). Yet the real
  entropy-coded result is a wash-to-regression (−0.0037% anvil.exe, +0.044%
  anvil_bench). Ledger root cause (verbatim): *"the actual binding cost is the
  per-TOKEN fixed framing overhead (type byte + length varint + distance
  varint + transform-mask bit), which the shared local cost-model formula
  underestimates specifically for PNRA's modal candidate shape: a single
  isolated 4-byte transform field ... a single 4-byte field saves at most 4
  raw bytes minus one mask bit, but pays the SAME fixed per-token wire
  overhead an ordinary multi-byte match pays."* The local formula
  `1.5 + varint(len-4) + varint(dist-1) + len/8 + 0.18·log2(dist+1)` judged
  1,599 candidates "cheaper" on one binary when the real aggregate was worse.
- **EXP. F (mdl) is the positive control that proves the theme is fixable.**
  When parser edge costs are *measured* from the real rANS streams (not a
  heuristic), every record file improved −8.6..−11%. The cost model works when
  calibrated to real coded cost; it fails when a fixed-shape heuristic is
  reused across candidate classes.
- **EXP. L (J-selection) is the second positive control.** At the
  pre-registered λ=0.01 the cost model is 100% faithful and gives real decode
  wins at zero ratio cost. Cost-model fidelity is achievable and measurable.
- **EXP. I/N/T/W (topology / SRR probe).** The (k,slot) modal-residual model
  was calibrated against Linux's 86.5% modal accuracy but measures 17–23%
  here — a cost-model mismatch, plus a genuinely weak-corpus finding. The
  probe *surfaces* 2–7x more span-like structure but the *coding* layer still
  can't make it pay. Two failure faces of the same theme: (a) the parser's
  acceptance heuristic underprices shapes; (b) the coding-layer cost model
  (modal residual) underprices the exception stream.

**Net:** the ratio-side of the project has repeatedly been limited by
*miscalibrated local cost estimates*, and the two times a *measured* cost was
used (EXP. F ratio, EXP. L decode) it was either the strongest result of its
iteration or 100% faithful. **This is the strongest evidence-backed framing for
the next-leads: spend the next cycle on cost-model fidelity at the two layers
where it gates everything — encoder candidate acceptance (§5 S6-2/S6-3) and
the whole-codec stream budget (§5 S6-1).**

---

## 4. Which horizon breaks the streak — and WHY (the ranked answer)

**Rank 1 — the DECODE leg via whole-codec stream-budget economics on the
hot-op/fused representation (the S6-1 horizon).**

Evidence:
1. **It is the nearest measured frontier point in project history.** The Linux
   hot-op hybrid on generated.log beat q9 on ratio (452 KB < 513 KB), decode
   (0.87–0.99 GB/s > 0.84), and encode (33 > 19.6 MB/s, 61/61 paired trials).
   Every other anvil best (shape-predict 0.1046 @ 957, generated.json) is
   dominated by q6/q9.
2. **It targets the binding plane directly.** FLAG-A: no anvil row ever reached
   brotli decode. Fusion only took decode ~1.2x (EXP. M) and hot-op only
   ~1.17–1.34x (EXP. R) because per-field entropy pulls are the floor — i.e.
   the *multi-stream materialization* is the remaining cost. Storing the
   dominant shape-distance-delta stream raw (the Linux stream-budget sweep:
   ~19.8 KB, decode → ~0.99 GB/s) is a measured, contained knob.
3. **Every component is already validated on Windows.** J-selection 100%
   faithful (EXP. L), mode 15 already the best anvil decode (~195 MB/s,
   EXP. R/I4-bench), raw-stream is a flag. The remaining work is a
   whole-codec application of an already-working cost model — low
   mechanism-risk, high payoff.
4. **It reuses the faithful cost model** (the theme's positive control), so it
   is not asking for a new discovery — it is *wiring the validated economy*.

Counter-argument handled: *"the Linux result was directional / 'not called
yet'."* True — but it is the *only* positive both-planes data point, it is
reproducible in principle on the same file, and the alternative horizons have
only *smaller* ceilings. A pre-registered Windows reproduction is cheap enough
to settle it.

**Rank 2 — the RATIO-side structural lanes via cost-model fidelity
(PNRA/TCOPY acceptance, S6-2; measured-cost oracle, S6-3).** This is the
shared theme in its clearest form, but it is NOT the primary streak-breaker:
the affected lanes (binary PE, structural records) have the smallest measured
signals (EXP. V raw+pnra −0.23..−1.49% in an uncoded harness; EXP. O tcopy
−0.6% on anvil.exe), and the binary lane has never produced a Pareto candidate.
Worth doing as enablement (and it is where two peers are already working), but
it is unlikely on its own to extend the front — it is what *enables the next*
binary/structural frontier attempt.

**Conclusion for (b):** the horizon most likely to break the streak, given the
cost-model-fidelity diagnosis, is **S6-1 — faithful whole-codec
decode-economics on the hot-op/fused representation** — because it is the
only horizon with a prior measured both-planes-ratio-AND-decode-AND-encode
crossing over brotli q9, it attacks the binding (decode) plane, and it is
built entirely from components already validated on this tree.

---

## 5. Pre-registerable, falsifiable experiments (ranked by EV / effort)

> Every experiment below is a *pre-registration sketch*. No mechanism is
> claimed. Each requires the standing protocol (§4 of research-agenda.md):
> round-trip + fuzz before claims, median ≥3 reps, ratio CV = 0.000% so ratio
> deltas are exact, timing only on ≥ ~100 KB files, per-file AND aggregate
> verdict via `tools/pareto_front.py`. Decode-plane co-arbiter mandatory
> (FLAG-A).

### S6-1 (PRIMARY — decode leg, best EV/effort) — Reproduce the Linux hot-op frontier crossing on Windows mode 15 via whole-codec J-selection + raw-stream budget

- **Lineage / what-is-new (admin):** Reuses the validated J-selection (EXP. L,
  agenda I2-2, 100% faithful at λ=0.01) and the validated mode-15 hot-op decode
  accelerator (EXP. R). The NEW interaction is *whole-codec* application: the
  per-stream codec choice (incl. `raw` for the shape-distance-delta stream) is
  decided once at block level with a decode-cost-weighted objective, per the
  Linux stream-budget sweep (CONTEXT §"STREAM-BUDGET SWEEP"). This is the
  recorded remainder of EXP. R, not a new mechanism.
- **Pre-registered constants (binding):** the same additive
  `J = L + λ·C_decode` as EXP. L with **λ = 0.01 bytes/μs** (the pre-registered
  value), plus the `raw`-stream candidate enabled for the shape-distance-delta
  stream specifically. No new hand-tuned per-stream weights.
- **Falsifiable target (the frontier bar):** on **generated.log** (and
  secondarily generated.json/jsonl), mode 15 + whole-codec budget must reach
  decode ≥ **2× the current mode-15 baseline** (≥ ~390–500 MB/s) at ratio **≤
  current mode-15 ratio**, AND beat **brotli q9 on BOTH ratio and decode** on
  generated.log (reproducing the Linux `452 KB @ 0.87–0.99 GB/s` vs `513 KB @
  0.84 GB/s`). Passing the q9-both-planes bar on any single corpus file at
  median-3 = **first EXTENDS_FRONT row in project history** (that exact claim).
- **Explicit non-goals:** no new wire format, no decoder-visible change beyond
  stream codec selection; round-trip + fuzz on all 13 corpus files; random/
  repeat controls no-regression (EXP. L precedent: suite=off byte-exactly =
  baseline).
- **Effort:** low-medium. **EV: highest** (only path with a prior both-planes
  crossing; all parts validated). **Primary owner: `hotop-rlz`/`arch` in
  coordination with `ledger-verify`**; `corpus-expand` confirms generated.log
  is a fair corpus cell.
- **Risk to record honestly:** EXP. R noted decode after stream
  materialization depends on allocator/setup; if Windows decode economics
  differ (host per host-spec), the verdict fails cleanly and is recorded (math
  vs implementation-era per working agreement).

### S6-2 (SECONDARY — enablement, the shared root cause) — Length/distance/shape-aware candidate acceptance for PNRA/TCOPY, validated on a HELD-OUT PE set (no overfit)

- **Background / what-is-new:** DIRECT follow-up to EXP. X's explicit
  remainder: *"a length- or distance-aware minimum-gain threshold specific to
  single-window transform candidates, rather than reusing the
  general-purpose sparse-candidate cost formula verbatim"* + the warning
  against overfitting to two pinned PEs. This is the `pnra-cost` lane; the
  pre-registration names the guard so the verdict is honest.
- **Mechanism sketch:** replace the single reused cost formula's fixed
  `len≥8`/`1.5+…` acceptance for type-3 transform candidates with a threshold
  that is a *stated function* of candidate shape (single-field vs multi-field,
  length, distance), so a 4-byte far single-field does not commit token-framing
  it can't amortize.
- **Pre-registered anti-overfit contract (binding):** calibration/tuning done
  ONLY on `anvil.exe` + `anvil_bench.exe` (the pinned PEs). **Verdict measured
  ONLY on a held-out PE set (≥4–6 additional binaries that `corpus-expand`
  adds to `tests/corpus/`, distinct from the calibration pair).** Falsifiable:
  `--pnra=on` beats `--pnra=off` by **≥ 0.5%** on the held-out PE aggregate at
  median-3, no worse than 10% decode regression, and **zero regression
  (byte-identical) on all non-PE corpus files** (EXP. X's gating already
  guarantees the E8/E9-absent case is a no-op). Threshold must be a stated
  formula, not a case-tuned magic number.
- **Explicit non-goals / honesty:** EXP. V's signal is in an uncoded harness
  and EXP. X's is a wash — so the pre-registration sets the bar HIGHER than
  EXP. X's +0.2% (which failed); the held-out requirement is what prevents
  fitting to 2 files. If the held-out verdict fails, that is a clean negative
  (the root-cause hypothesis is wrong OR the ceiling is simply too small —
  recorded, not hidden).
- **Effort:** medium (needs corpus PE expansion). **EV:** medium (enablement
  for the binary lane, which has never had a Pareto candidate; small ceiling).
  **Owner: `pnra-cost` + `corpus-expand`.**

### S6-3 (TERTIARY — cheapest, unifying) — A measured-cost candidate-rejection oracle: one greedy pass + one measured-rANS-cost refinement pass (NOT a full DP re-parse)

- **Background / what-is-new:** EXP. F proved measured-cost parsing is the best
  ratio mechanism but its 2-pass iterative DP is DP-class encode (0.8–1.9 MB/s)
  — the encode-speed claim was explicitly withdrawn. EXP. L proved the
  *measured* per-stream cost is 100% faithful. S6-3 captures the *same* fidelity
  at **greedy-class cost**: after a greedy/PNRA/sparse parse, run ONE measured
  rANS-cost pass and reject/roll-back only the worst over-committed candidates
  (or re-choose the knife-edge token-type/distance trades), using the real
  coded stream sizes — the direct, cheap fix for "the local heuristic
  underpriced this candidate" (EXP. X class).
- **Explicitly distinct from R3/mdl:** S6-3 is a single rejection pass, not an
  iterative DP re-parse; its target is greedy-class encode (not DP-class), so
  it does not re-derive EXP. F's slow path. Note for `dp-parser`: coordinate so
  the measured-cost oracle is shared, not built twice.
- **Falsifiable target:** on the record files (generated.log/json/jsonl), the
  single refinement pass must (a) match the true entropy-coded token cost to
  within X% on a candidate sample (calibration), and (b) yield an end-to-end
  aggregate ratio delta ≤ 0 on record files (exact, ratio CV=0) with encode
  cost bounded at **≤ 2× greedy-class** (e.g. ≥ ~20 MB/s on generated.jsonl)
  and **≤ 10% decode penalty**.
- **Effort:** low. **EV:** medium-high (unifies the theme across PNRA, sparse,
  and topology over-commit; cheap; produces a reusable measured-cost oracle
  that benefits S6-1's and S6-2's calibration). **Owner: `dp-parser` in
  coordination with `pnra-cost`.**

### Clear sequencing

1. S6-1 (decode/frontier test) — start first; it's the ranked-1 streak-breaker
   and reuses validated parts.
2. S6-2 (held-out PNRA cost-acceptance) — prep `corpus-expand` PE set in
   parallel; verdict after calibration/holdout split is ready.
3. S6-3 (measured-cost oracle) — cheap unifying enabler; feeds S6-1/S6-2
   calibration.
`corpus-expand`'s synthetic-structure work continues independently and is a
PREREQUISITE for honestly re-attempting any structural/invariant mechanism
(T/U/W all found the current corpus's periodic/arithmetic signal is weak).

---

## 6. Which mechanisms genuinely remove wire-framing overhead vs which are only tuning

Criterion: *removes per-token/stream wire-framing cost (type/class/distance/
parameter bits or decode passes) at the decoder* = genuine. *Adjusts an
acceptance budget / cost weight / knob without removing any wire cost* = tuning.

**Genuinely removes wire-framing / decode overhead (a Pareto claim can rest on
these):**
- **Compiled hot-op instruction book (I4-1/mode 15)** — fuses semantic ops into
  a small opcode index, removing per-field entropy pulls (validated: 1.17–1.34x
  decode, EXP. R). The strongest decode-overhead remover on Windows today.
- **Macro-ops (Linux mode 23)** — 16-bit fused semantic op; decoder skips
  separate type/class/distance passes. Validated on Linux; not yet on Windows.
- **Fused single-path decode (I3-1/EXP. M)** — removes stream-vector assembly
  (validated ~1.2x). Enabling, sub-frontier alone.
- **Zero-bit implicit transform parameter (TCOPY Δ=−d / PNRA θ)** — removes the
  transform-parameter bits entirely (validated: EXP. O, and EXP. X confirmed θ
  needed zero new bits). This is Orbit's zero-bit-params discipline, realized.
- **Single physical context-switched literal stream (I4-3/mode 6)** — removes
  model-multiplicity wiring without fan-out; RATIO PASS (−8.3..−16.2%, the
  strongest single ratio mechanism) — but decode ~3–10% slower, so it is
  wire-waste removal on the RATIO side, not a decode remover.

**Only tuning / fidelity (enablers — never a Pareto claim alone):**
- **Length/distance/shape-aware acceptance thresholds (S6-2, pnra-cost)** —
  calibrates the candidate gate to remove EXP. X's over-commit; enables a real
  mechanism, removes no wire itself.
- **Measured-cost candidate oracle / DP cost tuning (S6-3, dp-parser)** —
  parser-economics fidelity; the real mechanism is the parse, the oracle is a
  calibration layer (though a faithful oracle is how EXP. F got its gains).
- **J-selection λ/μ/ν weights + raw-vs-entropy stream budget (EXP. L)** — a
  cost-model layer that *decides* wire-vs-raw; when it picks `raw` it removes
  decode work, but the mechanism is the hot-op/fused representation, and the J
  is the tuning that budgets it. (In S6-1 the raw-stream budget is the lever,
  but it is tuning *on top of* the hot-op remover.)
- **Surprise / mismatch budget sweep (G1)** — entropy-control knob (adopted).
- **P(d|s) per-shape displacement (I2-1/R4, narrow, FLAG-D PASS)** — reduces
  distance-bit cost; coding-efficiency, small. Not framing removal.
- **Slot-default / topology coding (R2, mode 13, REJECTED)** — residual-stream
  coding-efficiency; overprices the exception stream relative to its value here.
- **Negative gate / boundary-aligned candidates (G3/G2)** — encoder-side search
  tuning; no wire.

---

## 7. Coordination notes for peers

- **`pnra-cost`:** the S6-2 anti-overfit contract (held-out PE verdict,
  stated-formula threshold) is the honest way to close EXP. X — lean on
  `corpus-expand` for the held-out binaries. Do not ship a magic number fit to
  anvil.exe/anvil_bench.exe; the ledger already names that trap.
- **`hotop-rlz`:** your RLZ/RePair book-grammar is a *regularity* refinement of
  mode 15 (ratio/smaller-book); the *decode* crossing (S6-1) is the 
  whole-codec stream budget. Verify the pair: a smaller/more-regular book and
  a raw shape-delta stream are compatible, not competing.
- **`dp-parser`:** S6-3 is deliberately NOT the full iterative DP re-parse you'd
  be inclined to build — one rejection pass at greedy-class cost. Reuse the
  measured-cost machinery rather than re-derive.
- **`corpus-expand`:** the single highest-value additions are (a) 4–6 additional
  real PEs for S6-2's held-out verdict, and (b) structure-carrying binary/
  columnar files (real periodic/arithmetic/record-layout) so a future
  structural/invariant mechanism can be valued — T/U/W twice showed the current
  corpus can't.
- **`tans-verify`:** expect a clean negative consistent with the ledger Note —
  ANVIL has no tANS state-machine coder; record it, don't build one to host the
  technique unless an independent reason for a tANS family emerges.
- **`ledger-verify`:** this doc is forward-plan; your I6 synthesis should treat
  S6-1's pre-registered q9-both-planes bar as the concrete "what would count as
  breaking the streak" acceptance criterion.

---

## 8. Sources grounded

- `docs/ORBIT_PROGRAM_COMPRESSION.md` — Orbit direction (generalized REF,
  Orbit-LZ, zero-bit params, latent-schema, generator synthesis, encoder-only AI
  search); "The mathematical problem"; "Disciplines retained."
- `RESEARCH_LEDGER.md` — EXP. A–X: F.1, G, H, I, L, M, N, O, P, Q, R, S, T, U,
  V, W, X; PART II–VIII consolidations (all verdict tallies, FLAG-A, costs,
  root causes cited above).
- `docs/research-agenda.md` — §1.3 FLAG-A/B/C/D, §4 abstraction protocol, PART
  IV (I4-1..I4-4 pre-registration), I2-2 J-contract.
- `docs/CONTEXT.md` — Linux v2 mechanism history; HOT-OP HYBRID + STREAM BUDGET;
  shape-frontier table; ELF/TCOPY/PNRA evolution.
- `docs/priorart-tcopy-external.md` + ledger EXP. K/P patent gate — TCOPY/PNRA
  prior-art boundary (US7676506B2 closest art; self-referential implicit-Δ
  unclaimed across 4 passes; Intel US 7,111,148/7,010,665 binding closeout).

**Standing disclaimer (repeated per the project's honesty rule):** none of
S6-1/S6-2/S6-3 is a claim. Each is a pre-registered, falsifiable test. The
frontier bar in S6-1 is explicit and exact; if it fails it fails cleanly and is
recorded like EXP. J's "don't re-burn," not papered over.
