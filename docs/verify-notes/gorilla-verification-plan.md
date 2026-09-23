# Gorilla/vXOR verification plan — FROZEN before any ANVIL-side code lands

(`verify`, post-u6 extension per coordinator briefing 2026-08-21.
Pre-registered per the project rule: build only after freeze — this file is
the verification-side freeze.)

## Spec anchors independently pinned TODAY (against grotli-codec source)

External reference: `C:\Users\<user>\WebstormProjects\grotli-codec`
(read-only; readable from this host, no permission stall).

| anchor | source location | verified |
|---|---|---|
| DoD tiers zero→1b, [−63,64]→9b, [−255,256]→12b, [−2047,2048]→16b, else 36b | `src/lib/grotli.ts` L572–579 (`classifyDoD`) | ✅ matches briefing |
| Case B1 = 2+mb bits; Case B2 = 2+5+6+mb = 13+mb bits | grotli.ts L473/L479 | ✅ |
| `serializeRecordLengths` / `deserializeRecordLengths` pair | grotli.ts L1231/L1258 | ✅ exists (their FINAL.md admits wire "not yet implemented" for the Pareto row — the ANVIL port is a NEW implementation) |
| Round-trip test suite (gorilla/dod/vXOR, "real lossless verification") | `src/lib/unitTests.ts` (85 KB) | ✅ exists |
| B2 bloat on mb≈60: (13+60)/64−1 = **+14.06%** | arithmetic | ✅ EXACT |
| Derivation-class rule: only measured-exact/measured-approx may headline | refs/FINAL.md §C2 | ✅ |

## Finding F-G1 (IMPORTANT — affects ledger tagging NOW)

The briefing's DoD headline ("1000-point 60s cadence σ=0.3s → 99.8% zeros,
1092 bits, 98.3% reduction") is **generator-parameterization-dependent and NOT
reproducible from the stated parameters alone**. My independent simulation
(seed 41246, `scratch/verify/gorilla_dod_check.py`):

| assumption | DoD==0 | total bits | reduction |
|---|---:|---:|---:|
| jitter N(0,0.3s), timestamps quantized to 1 s | 75.9% | 3,004 | 95.3% |
| jitter N(0,0.3s), unquantized f64 seconds | 0.0% | 9,060 | 85.8% |
| constant cadence (no jitter) — Pelkonen's regime | ~100% | ~1,077 | ~98.3% |

The claimed 99.8%/1092 b sits between these and is a property of THEIR
fixture generator, not of the stated σ. Consequence for any ANVIL port: the
DoD zero-rate claim MUST pin the exact generator (distribution + quantization
+ seed) or be tagged `modelled-capacity`; a "98.3% timestamp reduction"
headline without that pin is not `measured-exact`. (Constant-cadence fixtures
reproduce the spirit of the claim; jittered ones do not.)

## Frozen verification protocol (executes when arch lands the port)

**G-RT — Gorilla XOR round-trip bit-exact.** Rebuild the landed harness from
source myself (standard method). Decode∘encode == input as uint64 bit patterns
(strict bitwise, incl. -0.0/+0.0 distinction, sign flips, denormals if
claimed, max-u64 payloads) on: smooth-f64 telemetry fixture,
synth-timeseries value column, random.bin reinterpreted as f64, and the
project's own edge-case vectors. Any NaN-payload claim requires payload-level
comparison, not float==.

**G-DOD — DoD tier correctness + accounting.**
1. Boundary unit vectors: 0→1b; ±63 AND ±64→9b; ±255 AND ±256→12b;
   ±2047 AND ±2048→16b; ±2049→36b (both ends of every range).
2. End-to-end wire bits on a fixed-seed fixture == hand-computed sum over
   classifyDoD (+ header + initial delta).
3. Any zero-rate percentage claim: generator pinned (distribution, quantization,
   seed) or tagged modelled-capacity — per F-G1.

**G-VXOR — serializeRecordLengths bit-exactness + alignment.**
1. serialize→deserialize identity on length vectors {0, 1, 127, 128, 255, 256,
   65535, 65536, 2^31} and mixed realistic record-length sets.
2. Frame-alignment test: record-aligned detection must beat greedy alignment
   on a columnar fixture (the 86%-vs-17–23% modal-accuracy lesson from EXP.
   I/N is the prior here — alignment is the whole game).
3. The −36.3%-vs-brotli-q5 class of claim: reproducible only with the real
   NDJSON corpus present (corpus-expand action) + brotli q5 reference run on
   THIS host; until then it stays their-project measured-approx, not ours.

**G-GUARD — mandatory no-worse guards.** ASCII-guard (highByteFraction<0.02)
never routes text to Gorilla; uniform-random input → store with reductionPct
exactly 0 (the anti-lie row); ε_strict/AUTOSQUEEZE active ⇒ output ≤ raw on
the adversarial fixtures.

Every check runs against my own rebuild from recorded commands; deterministic
quantities must match byte/bit-exactly, throughput within the standing noise
bands. Results will extend `docs/verify-notes/i7-verification.md` and get
their own matrix section.
