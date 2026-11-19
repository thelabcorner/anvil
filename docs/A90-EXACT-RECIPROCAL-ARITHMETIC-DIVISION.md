# A90 — Exact reciprocal division for ANVIL's arithmetic coder

**Research state:** VERIFY mathematical equivalence; **execution:** TEST a bounded same-wire speed ablation only after profiling justifies it. **Not executed as an ANVIL codec change.** The human's objective is full multidimensional Pareto dominance against common codecs, not Hutter bpb.

## Current source and cost

At frozen source `2c5eeb9e570b1e1162cbff98d29e0a978edeca92`, `src/anvil.cpp`'s `ArithmeticEncoder::encode()` and `ArithmeticDecoder::consume()` each perform **two divisions by the same `total`** to calculate exact arithmetic interval boundaries:

```text
hi_delta = floor(range * cum_hi / total) - 1
lo_delta = floor(range * cum_lo / total)
```

The decoder's `scaled()` separately performs a division by `range` and is NOT included in this hypothesis. The BWT arithmetic postcoder also uses symbol-selection and renormalization loops. An older Fenwick-specific profile is outdated because `AdaptiveDecModel` already has a block-sum decoder. Therefore CPU share and actual machine instructions remain unmeasured for the current branch. Do not credit all arithmetic time to divides.

**Mechanism hypothesis H1:** compute ONE unsigned reciprocal for each per-symbol denominator, then derive BOTH exact quotients from multiply-high and one-step correction. This could remove one hardware integer division per symbol from the encoder and decoder, while preserving every exact interval boundary, the wire format, decoder states, source bytes, RSS and model assets. **This is generic exact integer-division strength reduction, prior art in computer arithmetic, NOT a novel compression algorithm.**

## Proof, not an approximation

Let `0 <= x < 2^64` and integer `2 <= d < 2^64`. Define

```text
U = 2^64 - 1
m = floor(2^64 / d)
  = floor(U / d) + [ U mod d == d - 1 ]
q0 = floor(x*m / 2^64)             // exact 128-bit product high half
q  = q0 + [ x - q0*d >= d ]
```

This gives **exactly** `floor(x/d)`. Since `0 <= (2^64/d) - m < 1`, `x/d - x*m/2^64` lies in `[0, x/2^64)`, strictly below 1. Thus the floor `q0` is either the true quotient or one less; the remainder check corrects it exactly. For `d==1`, directly return `x` to avoid representing `2^64` in a 64-bit reciprocal. A valid implementation must confirm `q0*d <= x` without overflow (holds because `q0 <= floor(x/d)`), use full `unsigned __int128` product on Clang/GCC and preserve a portable MSVC fallback.

Both numerators `x_hi = range*cum_hi` and `x_lo = range*cum_lo` share `d=total`, so derive `m` once and use the exact quotient function twice. Do **not** apply approximate reciprocal floats, rounded quotients or reciprocal values cached across adaptive-model updates. Neither encoder nor decoder may alter its arithmetic interval even by one unit.

## Source-grounded falsifiers

1. **G0 arithmetic identity:** test `x=0,1,d-1,d,d+1,2^64-1`, all small denominators, rescale boundaries (`32767,32768,65535,65536`), and fixed-seed random x/d; compare to original integer division. Also test realistic `range <= 2^32`, `cum_lo < cum_hi <= total`, `range*cum <=2^64-1`. One counterexample = STOP.
2. **G1 C++ semantics:** under the **same** Clang18, ABI, CPU and flags, compile a tiny scalar checker (hosted GHA only when permitted) and compare old `/total` with reciprocal helper on all pre-registered cases, including unsigned overflow and `d=1`. Inspect generated assembly: confirm it actually reduces hardware `div`/equivalent operations. A compiler may already optimize common-denominator division better; then PARK.
3. **G2 causality:** make an *opt-in source-only macro/branch* replacing ONLY the two `/total` operations in both encoder and decoder, with `/range` unchanged. For same frozen raw bytes and all forced postcoder IDs, demand exact original compressed-wire SHA, exact decoder output SHA, malformed/truncation/fuzz safety and deterministic state hashes. Separately assess encoder-only vs decoder-only vs both to isolate benefit.
4. **G3 performance:** on one manual-only GHA Ubuntu 24.04 runner, bracket with A/A null and pair raw encode/decode across identical old/new binaries, identical input/wire, relevant BWT-routed Silesia files plus Brotli-routed negative control and unrelated heldouts. Report total wall, CPU, program size, peak RSS and complete wire bytes. Avoid averaging together different codec routing.
5. **G4 Amdahl:** the observed A44 ANVIL/Brotli decode-time ratio `2.3618` requires >=15.3% whole-ANVIL time decrease even to meet the *legacy* `2x` cost tolerance. Actual **strict Pareto decode dominance** requires >=57.66% total decrease under the same reference (`1 - 1/2.3618`), even before encode/RSS/binary gates. If measured arithmetic-division share p is smaller than the desired end-to-end improvement, no conceivable speedup in this arithmetic substage can meet that target—PARK or combine orthogonal mechanisms only with independent ablations.

## Observation and disposition

A private **scalar-only** Python identity check using the exact formula above passed **67,848 x/d cases** spanning `d=1..2048`, CDF and rescale boundaries, `2^32`, `2^64-1`, and 64-bit extremes with seeded random trials. This is *not* a C++ compilation, a codec roundtrip or a speed measurement.

This idea has higher multi-axis potential than importing a huge Hutter text predictor because it might improve **both encode and decode time with identical bytes**, but only if current real CPU profiles show division is a large fraction of the actual workload and the reciprocal construction is cheap. **No performance gain is claimed.** Keep draft PR #24 libsais version-only benchmark as the nearest existing decoder test; A90 is a separate, bounded research hypothesis, not a priority override.
