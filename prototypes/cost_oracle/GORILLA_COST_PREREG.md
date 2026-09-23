# GORILLA/vXOR COST-SCORER PRE-REGISTRATION (oracle lane, cost side)

Author: `oracle` · swarm anvil-i7-cost · analysis-only, NO CODE (coordinator
gate: "Build only after freeze"). Grounded in: grotli-codec
`src/lib/grotli.ts` (gorillaCompressF64 Cases A/B1/B2, classifyDoD 5-tier,
verticalXORTransform), `refs/paradigm/COST.md` v3 (J-axis, lambda tiers,
epsilon_strict, header<=0.4% gate, derivation classes), and this lane's
measured deliverable (`deliverable/u2`, verify-reproduced).

## 0. What this lane contributes to the freeze

The u2 per-symbol measured-cost attribution (rANS renorm bytes / Huffman bit
lengths / defexc mask bits charged to exact symbols; fixed costs tracked
separately; identity sum(per_sym)+fixed==size asserted) is the missing
measured-exact substrate for COST.md's two asserted layers:

1. `bytes_touched(stage)` in D(block) = D_fixed + sum C_decode*bytes_touched
   — today taken from stream lengths by intent; attribution makes it
   measured-exact per stage, per symbol.
2. Model/table headers in B(shape) — today asserted <=0.4%; attribution
   measures every header byte exactly (fixed-cost accounting), so the gate
   becomes machine-checkable.

Everything below is stated-formula, calibrated on nothing yet (smooth f64
telemetry does not exist in tests/corpus — corpus-expand action), and every
constant carries a derivation class per COST.md section 5.

## 1. The scorer contract (what gets built after freeze)

For each candidate wire shape s in {Px-Gorilla, Pv-vertical-XOR} x
{Huffman-2.2, rANS-256-3.0} x {tier choices}:

    J(s) = B_meas(s) + lambda_tier * D_meas(s)

    B_meas(s)  = actual serialized bytes incl. all headers/tables (attribution
                 fixed costs; measured-exact)
    D_meas(s)  = sum_stages C_unit(stage) * attributed_bytes(stage)
                 (C_unit from COST.md 1.1 table; attributed_bytes measured-exact;
                  D_fixed counted once, amortized reported separately)
    lambda_tier in {0.02, 0.04, 0.06, 0.10} per COST.md 2 tier table (frozen)

Selection rule (adoption hysteresis, mirrors epsilon_strict):

    adopt s over fallback iff J(s) + eps < J(fallback),
    eps = max(1 B, 0.005 * |fallback B|)     [COST.md 2.1]

Fallback is ALWAYS reachable: raw/literal path stays a candidate (the
incompressible clean reject; same by-construction guarantee as u2's
variant-0-keeps-all arbitration).

## 2. Decisions that become measured J comparisons

(a) **Entropy backend per stream** (Huffman 2.2 vs rANS-256 3.0): apply
COST.md 2.1 cross-over rule with MEASURED delta_L from attribution, not the
assumed ratio: choose rANS-256 iff B_huff - B_rans256 > lambda*(3.0-2.2)*B.
Pre-registered expectation: descriptor streams (case letters, DoD tiers) are
tiny and Huffman-biased; mantissa bitstreams are raw by construction (no
backend choice). Falsifiable: report the measured cross-over point per stream.

(b) **B1-reuse window as re-blocking arbitration**: grotli.ts fixes B1-vs-B2
by a compatibility rule. Pre-register the ANVIL-side upgrade: encoder may emit
B2 (re-block) where B1 is legal, decided by measured-payload arbitration over
{keep-all-B1, re-block-K-worst} with K chosen by stated formula (frames whose
meaningfulBits exceed the running block mean by >2x). By construction
delta-bytes <= 0 (u2 pattern); falsifiable target: >=0 gain allowed, encode
cost <= 1.25x plain Gorilla pass (transform pass is O(n), arbitration adds
K-1 serializations of a tiny descriptor stream).

(c) **DoD tier boundaries stay canonical** (0/9/12/16/36b): NOT a J decision
— they are decoder-visible wire format; changing them is a format-lane matter.
Explicitly out of scope to avoid a mode-15-style book bloat repeat.

## 3. Guards (mandatory, no-worse by construction)

| guard | form | class |
|---|---|---|
| ASCII | highByteFraction < 0.02 -> never Px/Pv | measured-exact (radar) |
| smooth-gate | route Px only if sampled CaseA+B1 fraction >= 0.90 AND S >= 0.60 on a 16 KiB sample | threshold PROPOSED-FREEZE (modelled until smooth corpus exists) |
| vXOR-gate | route Pv only if stride CV < 0.20 AND H0(xor-stream) <= 0.80 * H0(raw bytes) measured on first block | H0 term measured-exact; CV radar |
| random tripwire | reductionPct > 0 on random.bin fails CI | mandatory negative control |
| header cap | any learned table > 0.4% of block bytes -> barred from J trade | measured-exact via attribution |
| neutral fallback | all guards fail -> existing pipeline untouched, byte-identical | by construction |

The smooth-gate thresholds (0.90 / 0.60) are the ONLY numbers in this document
not grounded in a measurement that exists today; they are flagged
PROPOSED-FREEZE and must be validated on corpus-expand's smooth-telemetry
files before the gate ships. This is the honest statement of what we do not
yet know.

## 4. Falsifiable targets (pre-registered, post-freeze build)

- T1 J-faithfulness: on smooth f64 corpus, the J-selected shape's B is within
  1% of the min-B shape across the candidate set (EXP. L precedent: J was
  100%-faithful at lambda=0.01; report per-tier agreement rate).
- T2 re-blocking arbitration: aggregate delta-bytes <= 0 on ALL corpus files
  (by construction) and <= -1% on smooth f64 (prediction; clean negative OK);
  encode <= 1.25x plain Gorilla pass.
- T3 neutrality: all 22 existing corpus files route to fallback byte-identical
  (guards fire); zero regression anywhere.
- T4 calibration: predicted-vs-measured per-frame bit cost (Case A 1b,
  B1 2+mb, B2 13+mb, DoD tiers) median |err| <= 5% — trivially exact for
  fixed-width codes; the informative row is the DESCRIPTOR-stream entropy
  estimate vs attributed bytes (report distribution; EXP. X lesson: judge
  committed candidates only).

## 5. Non-goals / honesty

- No DOMINANT claim without smooth-telemetry measured H0 (COST.md 5.1);
  today's 1152 B / -90% J figures are modelled-capacity on synthetic profiles.
- No decoder-visible format change proposed; Px/Pv wire stays as grotli.ts
  defines it. Tier boundaries frozen (section 2c).
- JS-runtime decode numbers (10-30 MB/s) are NOT the claim; native fused
  estimates are labelled architectural until measured (COST.md 4.5).
- This document pre-registers; it claims nothing. Build starts only after the
  coordinator freezes gates (a)-(c).
