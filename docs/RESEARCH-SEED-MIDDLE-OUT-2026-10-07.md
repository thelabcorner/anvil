# ANVIL Research Context Seed — Middle-out Time-Series Compression
**Added:** 2026-10-07  
**Status:** UNTESTED REFERENCE SEED / PRIOR-ART AND DESIGN CONTROL. No ANVIL source integration, benchmark, format ID, or research authorization is implied.  
**Source:** https://github.com/schizofreny/middle-out  
**Review basis:** upstream README.md (Git blob e856af895789df562529909f20a0eed81bcf7a54), scalar.cpp (blob 023e93b76570e38203055e6430a671758792371c), avx512.cpp (blob 3dd12d2f77f09dc91741af0eaa0db0ebabcdbe00). Public code originally authored in 2017. Upstream README identifies WTFPL version 2; independently verify actual license file and attribution obligations if reusing code.

## Why ANVIL should remember this reference

This repository illustrates a relatively inexpensive decoder-visible explanation for a restricted domain: the bit patterns of consecutive 64-bit numerical samples can often be reconstructed from previous samples plus small XOR residuals. Its unusual *performance architecture* divides an input vector into eight independent contiguous segments, rather than preserving one global dependency chain. One SIMD step advances eight separate predecessor states. It amortizes per-frame payload-width metadata across the eight changed values.

Potential relevance: ANVIL seeks improved **explanation yield per decode operation and per memory byte**, especially where a structured/numeric leaf can replace expensive generic entropy decoding. Middle-out is a candidate control for that question, not evidence of a new general-purpose universal compressor.

## Mechanism reconstructed from source

1. For N 64-bit values, partition the sequence into eight contiguous segments, each of floor(N/8) values; save the first value of each segment verbatim. The short remainder at the end is emitted verbatim. The segmentation is called “middle-out” by its authors but does not entail bidirectional parsing, graph-minimum description, or mathematical middle-out prediction.
2. At each iteration, independently XOR each lane's current 64-bit value with its previous 64-bit value. One bit in the common 8-bit equality mask indicates whether each lane is unchanged. When all eight match, the frame needs no further residual data.
3. For each changed value, encode a byte-granular right/trailing-zero offset using three bits. Leading and trailing zero runs determine the significant nonzero XOR portion.
4. Encode **one maximum nonzero-byte length for the entire eight-lane frame** (one three-bit value, lengths 1 through 8); every changed lane is serialized at that shared length. Pack offset and length metadata, add alignment padding where required, and emit its shifted XOR bytes.
5. The scalar implementation loops over the lanes, whereas the AVX-512 implementation uses eight 64-bit lanes, mask operations, gathers/compresses/scatters, vectorized zero counts, shifts, and a reduction for the shared maximum.
6. The released implementation targets x86/little-endian and template-instantiates double, signed 64-bit, and unsigned 64-bit types. Bit-exact reconstruction of floating-point representations is the relevant losslessness contract; numeric equality alone is insufficient.

**Source-specific caveat:** the scalar implementation relies on reference reinterpretation and typed writes into a byte vector. Any ANVIL-inspired rewrite should use defined bit casts or memcpy-safe loads/stores, explicit length checks, valid alignment rules, and thoroughly specified wire endianness. Do not copy low-level C++ assumptions uncritically.

## Historical author-reported results — NOT ANVIL measurements

The upstream README reports, on a single 2.0 GHz Skylake-X Xeon core:

| Implementation | Compress throughput | Decompress throughput |
|---|---:|---:|
| Scalar | 0.7–1.7 GB/s | 2.3–2.9 GB/s |
| AVX-512 | 2.3–2.5 GB/s | 3.4–4.8 GB/s |

It reports example compression factors of 1.3–3.3. These are **historical author claims**, not current, independently reproduced numbers, and not matched-end-to-end comparisons against ANVIL, Brotli, Zstd, xz, Gorilla, Chimp, or ALP. The dataset identities, byte-accounting scope, calibration, RSS, binary cost, and confidence intervals have not been reconciled to ANVIL's measurement contract.

## Novelty and related-work limits

- XOR delta on IEEE-754 bit patterns, unchanged-value flags, leading/trailing zero suppression, block frame coding, and SIMD lane-parallel decoding are established families. Middle-out does not earn ANVIL a novelty claim by being named, adopted, or trivially reparameterized.
- Mandatory comparators include Gorilla-style float XOR coding; Chimp/Chimp128's recent-leading-zero variants; ALP and ALP+adapt for floating-point columns; straightforward finite-difference/DoD FOR on integer series; Zstd and Brotli on original bytes; and optionally byte-shuffle + identical backend.
- Novelty, if any future hypothesis merits a gate, would require a genuinely new, precise semantic model or low-cost coding interaction that survives these controls and achieves a **complete frontier movement**. A fast AVX-512 path by itself is adopt-class optimization.

## Opportunities: falsifiable, not endorsed

**M1 — Group-shared residual-width coding.** Eight-lane maximum-width amortization may lower metadata bytes when each lane has homogeneous small residuals, but a single wide outlier forces all changed lanes to pay the maximal width. Measure actual complete bits of (mask + offsets + width + payload + padding + seeds + tail) against per-lane widths. Assess distributions of max(width) minus individual widths, conditional on changed-lane count.

**M2 — SIMD parallelism without cross-lane dependency.** Eight independent chains reduce serial recurrence and could accelerate decode. However the contiguous 8-segment layout makes an interleaved SIMD gather and causes eight widely separated memory access streams, which can hurt locality and streaming. Compare contiguous-per-segment, interleaved, tiled, and cache-line-friendly layouts with identical residual grammar. Charge dictionary/metadata/segment seeds and working-set costs.

**M3 — Typed numeric leaf within a larger representation compiler.** Exact lexical JSON token preservation is a very different contract from known typed float64 binary fields. Never silently canonicalize decimal lexemes or turn -0 into +0, normalize NaNs, or infer float types without a proof of exact source reconstruction. A leaf may be used only when it carries any lexical restoration metadata and improves complete wire size against raw fallback and type-aware controls.

**M4 — Decoder-operation attribution.** Profile cycles/byte, bytes-read/byte, extra masks, branch mispredictions, front-end pressure, SIMD utilization, 64-bit input loads, and the shared-width encode cost. A large SIMD speedup with weak rate is infrastructure rather than a crossing mechanism.

## Preregistered-style future test sketch — NOT APPROVED FOR CI DISPATCH

Before tests: clear ANVIL Gate A (source/CI identity) and Gate B if any promotion claim is sought. Acquire a new independently locked real numeric/telemetry family; do not represent the existing synthetic telemetry generated by ANVIL as held-out.

- **Correctness:** bitwise 64-bit roundtrip; empty, 1–7, 8, 9, 15–17 values; non-multiples of eight; constant, monotonic, alternating, large outliers, random uniform bits; ±0, ±inf, quiet/signaling NaN payloads; repeated values; adversarial malformed lengths; valid bounds before allocation.
- **Causal ablations:** scalar vs AVX-512; shared max width vs per-lane width; unchanged masks on/off; 1/2/4/8/16 lanes; contiguous, interleaved, and tiled geometry; actual seeds/footer/amplification; same-backend controls.
- **Comparator matrix:** Gorilla, Chimp/Chimp128, ALP, existing ANVIL numeric transforms, Brotli meaningful quality/window tiers, Zstd fast/high-quality, xz if applicable. Freeze exact versions and complete serialized bytes. Avoid comparing a raw numeric column to differently serialized raw files.
- **Performance:** on GitHub Actions only, record CPU model/ISA availability and runtime dispatch, single-thread CPU affinity, warmups, interleaved repetitions, median/dispersion, encode/decode cycles per source byte, output throughput, peak RSS, exact binary/decoder-only size where comparable. Fail closed when AVX-512 unavailable. Do not splice runner windows.
- **Decisions:** KILL if it only wins on synthetic or small stale benchmarks, cannot beat modern typed controls in a meaningful complete-byte/decoder Pareto cell, or has unacceptable outlier/seed/memory costs. RETAIN AS CONTROL if useful for calibrating the numeric-performance floor. Promote to ANVIL engineering only under independently justified preregistered targets and trusted source identity.

## Integration / work queue status (2026-10-07)

**ADDED TO RESEARCH CONTEXT ONLY.** No deployment, implementation, production transform, format change, remote dispatch, GitHub Actions run, new swarm, or heavyweight local benchmark was performed. Numeric promotion is blocked by the lack of new admissible independent numeric families, and R2 Gate A is blocked on approved reproducible publication and Q1a-XRUN.

## Pointers
- Upstream README: https://github.com/schizofreny/middle-out/blob/master/README.md
- Scalar implementation: https://github.com/schizofreny/middle-out/blob/master/scalar.cpp
- AVX-512 implementation: https://github.com/schizofreny/middle-out/blob/master/avx512.cpp
- Existing ANVIL documentation: ../docs/swarm-2026-10-02/FROZEN-CLOSEOUT-MATRIX-R2.md ; ../docs/I10-BREAKTHROUGH-PROGRAM.md ; ../docs/anvil-i9-findings.md ; ../docs/I10-GROTLI-G3-RESULTS.md
- Other named literature must be verified for exact implementations and versions before forming a frozen preregistration.
