# ANVIL I14 — Local Modular Difference Innovations: Frozen Result

**Date:** 2026-10-07. **Classification:** SPECIALIZED-DISCOVERY-ADVANCE (all H1–H4 passed). **Authoritative run:** [GitHub Actions 37699578503](https://github.com/thelabcorner/anvil/actions/runs/37699578503), all workflow steps successful, artifact [11517396648](https://github.com/thelabcorner/anvil/actions/runs/37699578503/artifacts/11517396648), artifact sha256 a472d7567f420509aafc48654442dd0d3024dc32c38745b2af59fa9e2a6717a2.

## Exact evidence anchors

- Workflow-dispatched SHA: 21bf7746fb10e96639c616008d20c714a7bb89d4.
- I14 code blob 365348ac5b8295f32eed97fb9427bb5516c43b16, raw SHA-256 f4f75b098feee9753084fa5cc132e31064f107fbd8ffa621f5e415b57d825150.
- Frozen I12 code blob 2e6ea6492f970ee56366b5530872da193a815226, SHA-256 3e6c24c90eb4026cca2520e88413caece25f847771f4e0c6513c48964a05992c.
- Pinned static Brotli commit ed738e842d2fbdf2d6459e39267a633c4a9b2f5d, q5/q11, lgwin22; Ubuntu 24.04 x86-64 runner, GCC 13.3.0, CMake 3.31.6.
- Source checks, fixed seven tracked discovery inputs, static reference build, two C++ builds, both selftests, fourteen full file byte-for-byte roundtrips, two same-run benchmark suites and evidence upload all PASS.
- All timed decode numbers include every decoded output byte in the same digest algorithm; shared runner performance remains scouting evidence.

## Preregistered verdict on *consumed* 256000-byte arithmetic fixture

| Metric | I14 AVI4 | I12 AVI2 (same runner) | Brotli q11 (same runner) |
| --- | ---: | ---: | ---: |
| Complete wire bytes | **33,196** | 62,458 | 87,013 |
| Encode MB/s | 24.079 | 29.023 | 0.505 |
| Decode + digest MB/s | **361.970** | 360.158 | 141.741 |
| Selected model blocks | 63 | 63 | n/a |
| Selected local-DIFF blocks | **63** | unavailable | n/a |

- **H1 TRUE:** all 63/63 arithmetic blocks used the new local DIFF mode, not only RAW or retained AFFINE.
- **H2 TRUE:** 33,196 < 62,458, a **29,262-B / 46.85%** reduction from I12.
- **H3 TRUE:** 33,196 < 87,013, a **53,817-B / 61.85%** saving relative to Brotli q11 on this one synthetic fixture.
- **H4 TRUE:** 361.970 > 141.741 MB/s, **2.55x** digest-inclusive paired decoder rate. I14 vs I12 decode was nearly tied, ~0.5% difference, not a reliable throughput superiority claim without repetitions/noise analysis.
- Source sum/roundtrips/structure true; selftest PASS 18 cases and 2 locally constructed DIFF model blocks.
- Encode vs I12 is a tradeoff: **24.079 vs 29.023 MB/s**, about 17% lower; not a dominance claim in the full three-axis space.

**Mathematical interpretation:** A linear predictor with a single base accumulates small signed innovations over a random walk. DIFF instead serializes only e_i = x_i - x_(i-1) - d modulo 2^b. On this source, the local residuals stay narrow while global residuals grow. Decoder work introduces a serial prefix recurrence; the measured decode result shows no major penalty on this corpus/hardware, but a larger, more representative workload is necessary.

## Complete negative-control outcomes (binding)

| File | I14 complete B | Brotli q11 complete B | I14 DIFF blocks |
| --- | ---: | ---: | ---: |
| synth-timeseries.bin | 280,214 | 134,718 | 0 |
| synth-jitter.bin | 975,643 | 62,717 | 0 |
| random.bin | 262,343 | 262,149 | 0 |
| generated.repeat.jsonl | 936,694 | 160 | 0 |
| generated.jsonl | 2,805,330 | 150,415 | 0 |
| src.cpp | 32,543 | 7,991 | 0 |

**These negative results are not noise.** I14 is not a viable all-purpose compressor in isolation. The pseudo-numeric transformations were not even selected on the realistic interleaved time-series source. No *general-purpose* Pareto crossing, production adoption, or prior-art novelty claim follows from the arithmetic result.

## Interrupted-run lineage, not silently suppressed

- [37699173826](https://github.com/thelabcorner/anvil/actions/runs/37699173826): source checks and compilation passed; the synthetic walking selftest selected an equivalent global-affine candidate instead of DIFF. No codec benchmark ran. Classified INVALID-PREMEASUREMENT, then corrected the confounded *fixture only*.
- A branch push with mis-indented YAML produced an invalid workflow. GitHub action dispatch rejected it; repaired two env indentation lines without altering source or hypotheses.
- [37699578503](https://github.com/thelabcorner/anvil/actions/runs/37699578503) is the first all-green, source-pinned admissible I14 measurement.

## Binding continuation

**Retain I14 as a numerically specialized discovery control.** Preserve source/hash and paired evidence. The next step is not to claim novel predictive coding: this is established delta-of-delta / bitpacking territory (Sprintz, FastLanes, PFOR). Instead:
1. Test local DIFF inside I13 strided fields with independent controls and exact complete column cost (I15, preregistration required).
2. Diagnose whether 14-byte record misalignment, weak residual model or raw-lane costs explain I13's **4,669-B deficit** against Brotli q11, separately.
3. Build a full-file fallback before any general-purpose product claim; compare against modern typed alternatives and independent real-world source families.
4. Evaluate decode restart checkpoints and SIMD scans only after profiling true *model-bearing* hardware workload with memory/RSS and matched window constraints.
5. Protect ANVIL historical R2 result: **435 dominated, 28 degenerate, 5 unresolved, zero verified full crossings**.

No delegates or swarm members were started. All CPU-heavy compilation, tests and benchmarking were run by GitHub Actions.
