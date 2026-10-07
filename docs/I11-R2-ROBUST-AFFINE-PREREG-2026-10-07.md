# ANVIL I11-R2 — Robust Affine Recovery and Validated Decoder Timing
**Preregistered on:** 2026-10-07, before any I11-R2 codec build or corpus run.  
**Experiment class:** discovery only / no heldout promotion / no Class-A crossing / no novelty.  
**R1 reference:** [GitHub Actions run 37695845314](https://github.com/thelabcorner/anvil/actions/runs/37695845314) (fully green execution, negative mechanism selection).  
**R2 I11 source:** SHA-256 `e07ab08a1aeab7b6cfd79d1ea00a19af45992bc76d4f700d8799fedfc64e2e71`, Git blob `079916dbf799904ceb2a5c44337e64f35fe2bdcc`. Both identities enforced by GitHub Actions, immutable across this R2 run.

## Frozen R1 result and why it is inadequate

On seven tracked discovery files the R1 AVI1 encoder selected **zero** AFF and zero PATCH blocks (1,357 RAW blocks), proving that initializing the affine slope solely from the first two values is inadequate for inputs with noisy beginnings or field misalignment. Exact R1 whole-wire compressed sizes vs Brotli q11:
- synth-arith.bin 256,196 B vs 87,013 B;
- synth-timeseries.bin 280,214 B vs 134,718 B;
- synth-jitter.bin 975,643 B vs 62,717 B;
- random.bin 262,343 B vs 262,149 B;
- generated.repeat.jsonl 936,694 B vs 160 B;
- generated.jsonl 2,805,330 B vs 150,415 B;
- src.cpp 32,543 B vs 7,991 B.

Thus R1 **failed** H1, H2's precondition (no reconstructed blocks), and demonstrated the expected negative control; it is not a frontier contribution. R1 code's timed decoder lambda consumed only the output *length*. Numbers above 60,000 MB/s and up to 113,575 MB/s are **invalid** because the compiler can elide unused reconstruction. Report R1 timing as VOID, not fast decoding.

## R2 exact changes

1. Replace initial-two-word slope by **modal modular adjacent finite difference** for each word width 1/2/4/8:
   `Δ* = mode((x[i]-x[i-1]) mod 2^(8w))`. Enumerate whole-block differences and select the exact modal value deterministically with sorting (max block 4KiB, O(n log n) encoder work).
2. If fewer than half the adjacent differences match the candidate slope, prune the width. This is a conservative hypothesis based on the <=25% exception ceiling (each bad word may disturb two adjacent differences).
3. Recover base by **modal projected intercept** `a* = mode((x[i]-iΔ*) mod 2^(8w))`. Prune if too many words differ from the modal intercept. This allows sparse errors in early words without corrupting the inferred sequence. Same exact 4096 B block, wire format, exception budget, and full serialized cost selection as R1.
4. Enforce a positive sparse-exception selftest: the R1 sparse fixture must select at least one PATCH block and exactly roundtrip.
5. Replace decode length-only sinks with an every-byte digest computed over **all** reconstructed bytes, for AVI1 and both Brotli decoders equally. Timed rates are decode + digest, not pure decoder wall time. The digest work is shared and R2's results are not directly comparable to R1 timing. R1's implausible results must never be used.
6. Preserve same seven tracked discovery files, pinned Brotli commit (quality 5 and 11, lgwin22), C++ optimizer and 4KiB independent blocks. No new corpus controls, no newly enabled workflow triggers or changed input role.

## Preregistered qualitative stopping rules

- If `synth-arith.bin` still selects **no AFF/PATCH**, abandon this global affine-block candidate as insufficient, and shift to field framing/structural inference before further oracle tuning.
- If any true lossless mismatch occurs, cancel benchmarking conclusions and repair correctness only.
- If AFF/PATCH selects on synthetic data but full bytes are worse than Brotli q11, do not claim ratio win; measure causal deficit, consider predictor as a future *residual transform*, not standalone compression.
- If encode time collapses from modal sorting, use bounded samples or SIMD histograms only after this fixed-pilot result; do not retroactively change R2 fitting.
- A speed advantage on tiny raw-dominated inputs is not evidence of a compression Pareto crossing, even with corrected timing.
- No corpus-generalization, held-out, mechanism novelty or production adoption statements follow from this discovery-only run.

## Strict evidence requirements

The workflow must output raw bytes and full AVI1 bytes, per-mode counts, exception counts, Brotli q5/q11 complete bytes, **corrected** digest-inclusive paired medians, source/compiler/corpus hashes, real roundtrips, and retrievable GitHub artifact. The numerical comparison is admissible only for discovery, with exact runner/provenance identity. A future study needs whole-system RSS, binary footprint, fixed-window controls, independent real families, and Gate B to claim Pareto membership.

## Mathematically distinct next branches

**R2 winning affine** → investigate strided field discovery with hashed alignment evidence, SIMD parallel modulo recurrences, and residual entropy coding with full headers.  
**R2 loses pure affine** → inspect synthetic generator framing and reset boundaries; do not invent a larger arithmetic search budget without a high-support evidence trace.  
**R2 wins decode but loses bytes** → compare integrated transform+LZ/ANS to baseline, while charging the entire transform control-plane overhead.  
**R2 fails on interleaved/misaligned** → use a separate field-framing experiment, not after-the-fact tuning of R2.  
**R2 no selective adoption** → stop I11 and prioritize fair Q2 geometry, source-independent Gate B corpus validation, and the already proven ratio-focused BWT path.

## Source-placement syntax correction, before R2 execution

The first R2 GitHub run [37696228453](https://github.com/thelabcorner/anvil/actions/runs/37696228453) **failed at C++ compilation**: the new `observedDigest` function was inserted into the range-for header of `emitBlock`, instead of at file scope. No I11-R2 code was executed, no decoder/encoder self-test ran, and no R2 performance metrics were generated. The repair exclusively relocates the exact digest function to file scope before `medianMicros`, restoring the unchanged range-for header. The modal fitting, codec wire format, inputs, Brotli pin, thresholds, and digest algorithm are byte-identical to the R2 intended implementation. Corrected source SHA-256: `c8d9da7e4bc1eb154d5e9ba572052c29fb35898751f24bfb6ae3fb91f92cf1e9`; corrected Git blob `bf2aa9560b01baedd504cbee47e8631b151c367c`. The Actions identity assertion is updated accordingly. The first failed run is never retroactively counted as a completed experiment.

## Compile-only correction 2: complete digest brace (run 37696509451)

[GitHub Actions 37696509451](https://github.com/thelabcorner/anvil/actions/runs/37696509451) cleared source/input checks and built pinned Brotli, but failed compiling I11-R2 **before any codec operation**: a closing `}` was absent after `observedDigest`. The surgical, syntax-only one-line fix adds that brace at file scope. Fitting, wire format, digest, baseline, population and thresholds remain unchanged. Rebound Git blob `e365fe7e4afb0c35b5907b727ef8ac3f37709dc5` and SHA-256 `7857dc487b6656ed1a3280c1c00cfd0beb4d56e5f79c1164cbe981e5be517c44`; only a future fully green remote run can supply scientific evidence. No performance result was generated by run 37696509451.
