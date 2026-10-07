# I19 Phase 1 — Encoder-Work-Constrained Reconstruction: Mathematical Draft

**Date:** October 7, 2026. **State:** ARCHITECTURAL DESIGN ONLY, NOT AN EFFICACY PREREGISTRATION, NO BENCHMARK AUTHORIZATION. **Frozen control:** I18 code at 7ec50e8; do not mutate its measured bytes, threshold, candidate set, or reported timings.

## Two decoupled objectives

1. Real-structure advantage: Does AVI6 offer a complete-wire advantage on independent typed integer data relative to a dense matched set of established codecs? Only origin-stratified results with matched memory and decoder timing could qualify.
2. Encoder-work reduction: Can a bounded source-sampling filter prevent expensive AVI4/AVI6 invocations when their *complete serialized output* could not beat the current q5/raw incumbent? False-negative opportunity regret counts against the selector; skipping a losing candidate is not itself compression novelty.

A selector that integrates existing encoders without a distinct reconstruction mechanism may occupy an operational Pareto point, but is NOT a new codec mechanism. The scientific object under test is decoder-cheap residual reconstruction rather than a brand-new conditional.

## Why prefix-only sampling is unsound

The I18 numerical probe (12 strides, at most four lanes, up to 32 adjacent step observations) examines only an early prefix, and its true/false decision is statistically vulnerable to a regular-prefix/noise-tail input. More generally, any bounded deterministic probe omitting input positions can be defeated by choosing identical sampled values and modifying unsampled bytes. Therefore **no distribution-free worst-case bound on compression regret exists** from that probe alone, regardless of the precision of its historical accuracy. Any oracle threshold is a calibrated empirical heuristic, not a correctness theorem.

## Adaptive mathematical reconstruction mechanism candidates

On each signed or unsigned word lane in the modular ring Z/(2^w), for w in {16,32} and field stride s in a preregistered small set:

- Constant model: p_i = c.
- Local first difference: p_i = x_(i-s) + d_lane (mod 2^w).
- Second difference: p_i = 2*x_(i-s) - x_(i-2s) (mod 2^w).
- Affine index: p_i = a_lane + i * d_lane (mod 2^w).
- Field-to-field shared slope: p_(i,lane) = x_(i-s,lane) + d_shared + epsilon_lane, with common d for a bounded group of lanes.

Residual: e_i = x_i - p_i mod 2^w; unsigned residual zigzag_w(e_i) only where semantically justified. Candidate coding: fixed-width packed residual blocks plus a bounded exceptions bitmap and exact unmodeled literals. Full decoder operation counts, dependency depth, and scratch memory must be estimated using the actual wire format and independently verified against instrumentation; no floating-point fitting or unbounded search may occur in decoding.

The implementation must not interpret NOAA missing sentinel -9999 as ordinary temperature. Missing masks and calendar slot gaps are part of the candidate's **full wire cost**, and observations must preserve exact signed values and record ordering. Typed projection input is distinct from end-to-end source-file bytes.

## Multistage work oracle (prospective, not yet frozen)

Stage A: q5 and raw incumbent complete-wire bytes. Preserve I18 conservative <=12.5%-of-input q5 rejection policy as a control, not an unquestioned optimal threshold.

Stage B: deterministic multi-region sample positions defined by source length and a fixed seed, covering early/middle/late regions rather than only the first sample window. Candidate floor: count of residuals that are provably expensive for *a specified residual coder*, not an unsupported universal lower bound. Track sampled distinctness, zero residual density, bit-width histogram, exception counts, lane regularity, and slice-to-slice variance. Do not fit thresholds or investigate missed candidates using validation or heldout origins.

Stage C: choose action among [reject both, test AVI6 only, test AVI4+AVI6], under a fixed encoder-work budget per profile. In all cases the output must be byte-for-byte decoded before admitting a size result. Instrument actual sample reads and candidate invocations; maintain a diagnostic full-evaluation arm with actual cumulative work charged.

For a selected mode m, compare complete bytes S_m and a same-input full selector gold S_*; define regret r = max(0,S_m-S_*) / S_*. Per-origin aggregate regret = sum(max(0,S_m-S_*)) / sum(S_*). Report per-input regret as well; a low aggregate cannot mask a catastrophic minority. Also retain selector overhead in nanoseconds, candidate branch distribution, decode work, RSS, and hard input caps.

Exact encoded bytes include outer envelope header, varints, mode tag, model/stride metadata, bitpack exception masks, residual payload, checksum, and any source-reconstruction metadata. Any bound which omits a required component is only a heuristic.

## Preliminary candidate treatment for sampled real input

The 2026-10-07 source preparation produced two small NOAA temperature sequences (20,119 signed16 samples each) as the *only discovery origin*. USGS discharge and NASA POWER temperature (4,018 signed32 samples each) are frozen as validation origins and must remain unused for tuning. This is **not enough independent discovery-origin diversity** to calibrate robust statistical threshold families. More genuine, lineage-distinct discovery origins and an unexposed heldout must be admitted before experiment preregistration.

## Exact comparator requirements

Brotli q5/q11, Zstandard fast/medium/high, LZ4/LZ4HC, original frozen I17 fast/full, original frozen I18, matched bitshuffle/byteshuffle + the same entropy backend, and established typed integer methods **Sprintz, FastLanes/FOR/PFOR** where dtype/stride semantics and framing are genuinely comparable. Record the overhead for padding 16-bit streams to 32-bit for codecs that require that width; never omit the padding or declare a synthetic format advantage. OpenZL is relevant as a *format-aware transform* comparator, but evaluate on a pinned release and identical representation. Existing prior-art sources:

- Sprintz: https://github.com/dblalock/sprintz — integer multivariate time-series, prediction and bit-packing.
- FastLanes: https://github.com/cwida/fastlanes — expression-composed column encodings, SIMD-friendly.
- OpenZL: https://github.com/facebook/openzl — format-specialized graph transforms and universal decoding.
- SIMD bitpacking: https://github.com/fast-pack/simdcomp — distinct baseline for residual pack-only costs.

These have established mathematical overlap with much of ANVIL's family. A classical predictor or packing combination alone is **not** sufficient novelty evidence.

## Next CI acceptance protocol (must be frozen in a later prereg before execution)

- Freeze immutable source SHA, integer endianness and missingness, source-lineage splits, archive storage, rights/attribution, complete baseline versions, comparator compiler configuration, exact profile weights/budgets, sampler selection, residual bit packing and full-wire accounting.
- Discovery-only oracle calibration, with adversarial regular-prefix/random-tail, missing-mask bursts, high-bit jitter, random, periodic, constant, wraparound, pathological stride, and tiny input controls. Expose corpus hash, work counters and SHA of every compressed output.
- Run >=7 independent paired randomized encode repetitions, >=9 decode repetitions, output-observed consumption, same-runner controls, bootstrap confidence bounds, root-mean-square deviations and RSS in separate isolated processes. No medians-only frontier promotion.
- Two independent validation origins must show predeclared non-dominated complete-byte benefit against matched typed/general codecs with decode >=90% and peak RSS <=110% of compared codec, and >=3% meaningful byte advantage, **after** fully frozen thresholds. These are proposed thresholds from the I19 design document and may not be retroactively relaxed.
- Profile high-ratio and fast-encode separately; full multidimensional Pareto on archive bytes, encode ns, decode ns, peak RSS, decode binary bytes; unresolved axes cannot silently be dropped.
- Final heldout requires origin and raw bytes not externally exposed to optimizing agents, with independent custody attestation and one-time unseal only after all discovery+validation decisions are irrevocably frozen.

## Blocking status

The I19 original UCI ZIP appears in a downloadable GH Actions artifact, so it is **not genuinely sealed heldout**. The current source manifest remains unfrozen; rights decisions are pending; sources exist in expiring artifacts. New tools/i19_validate_efficacy_admission.py specifically rejects the known leaked UCI digest, expiring Actions archives, pending rights, absent comparator registrations and unsealed holdout, but no metadata validator can independently prove legal rights or private custody. Passing its schema would merely qualify *for external attestation*, not authorize efficacy.

**No I19 real compression performance results are reported or implied in this document.**
