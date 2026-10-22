# I11 / D2 — the postcoder's share of BWT-path decode (same-job differential)

**Candidate:** D2 — `docs/I11-PARETO-DOMINANCE-PREREG-2026-10-22.md` §5.
**Owner:** worker D2. **Branch:** `research/anvil-i11-d2-postcoder-share`.
**Compute locus:** GitHub Actions only (`.github/workflows/anvil-i11-postcoder-share.yml`).
**Zero local-compute evidence.**

## Why this exists

The I20 C1 attempt attributed the BWT-path decode cost with
`valgrind --tool=callgrind` and **failed four times on the hosted runner**
(runs `39659153216`, `39659202582`, `39659854677`, `39660334510`). This candidate
measures the same quantity by a **differential, same-job** method that needs no
profiler.

## What is measured, inside ONE job

| # | quantity | how |
|---|---|---|
| 1 | the **postcoder stage alone** (MTF + zero-run RLE + adaptive arithmetic → the BWT string); the inverse BWT is deliberately **not** run | `stage_probe bwt`, the two `[PROBE]` clock reads around the postcoder branch of the copied `bwt_backend_decode` |
| 2 | the **inverse BWT alone** over that same BWT string | the second `[PROBE]` clock pair around the `libsais_unbwt_aux` / `libsais_unbwt` call |
| 3 | the **end-to-end ANVIL decode** of the same payload | the shipped `anvil d`, timed by `tools/paired_bench.py` with an A/A null |
| 4 | the reference **`brotli_lw` one-shot decode** of the same bytes | the `brotli_lw` CMake target, plus the probe's in-process one-shot (`brotli_lw_decode_exact` copy) |
| 5 | ANVIL's **streaming** Brotli decode (`brotli_lw_decode_stream`, 256 KiB output window) against (4) | the probe's copied `brotli_lw_decode_stream` with a discarding sink, plus the process-level paired series |

Item 5 settles whether the 256 KiB streaming window is a material cost against
the reference's one-shot full-buffer decode — a candidate-side defect in its own
right, independent of the postcoder.

## The fixed decision rule (quoted verbatim from the preregistration)

> **If the postcoder is ≥ 35 % of BWT-path decode, D4 (the entropy-coder swap) is FUNDED and
> must be executed. If the postcoder is < 20 %, D4 is `NO-GO` on Amdahl grounds and the
> programme's speed effort pivots entirely to D3. If the 256 KiB streaming window costs
> ≥ 10 % against the one-shot reference decode, enlarging it is a mandatory part of any
> later arm.**

Aggregation convention (declared before the numbers are read; it does **not**
re-weight the 35 % / 20 % thresholds): the rule is evaluated against **every**
denominator the decomposition provides — the stage sum (`postcoder + inverse BWT`),
the probe's own decode total, and the shipped end-to-end decode. **FUNDED** iff the
share is ≥ 35 % under every denominator; **NO-GO** iff it is < 20 % under every
denominator; otherwise **INCONCLUSIVE** with the measured range reported.

## PROVENANCE — what is copied and what is not

`src/anvil.cpp` is **not modified** (D3 owns mechanism changes).

`stage_probe.cpp` carries **verbatim copies** of the shipped code it measures, taken
from `src/anvil.cpp` at commit `89e4b32` (and every ancestor):

| lines | what |
|---|---|
| 45–48 | `kHalf` / `kFirstQtr` / `kThirdQtr` / `kModelRescale` |
| 69–86 | `struct BitReader` |
| 127–160 | `class ArithmeticDecoder` |
| 214–246 | `class AdaptiveDecModel` |
| 280–289 | `decode_uvar_m` |
| 294–298 | `get_uvar` |
| 481–482 | `get_u32le` |
| 4022 | `kBwtAuxMaxIndexes` |
| 4055–4061 | `bwt_mtf_emit_rank` |
| 4098–4122 | `bwt_arith_decode` |
| 3945–3960 | `brotli_lw_decode_exact` |
| 3972–3995 | `brotli_lw_decode_stream` |
| 4474–4560 | `bwt_backend_decode` |
| 4636–4683 | the BWT branch of `ratio_backend_decode` (sub-block framing) |

The **only** edits to those copies are:

1. **[PROBE]** two `std::chrono::steady_clock` reads inside `bwt_backend_decode` —
   one pair around the postcoder branch, one pair around the libsais inverse-BWT call;
2. **[PROBE]** the three postcoder branches this probe never selects (id 0
   static-MTF, id 3 raw-stream, id 4 QLFC) throw instead of calling
   `decode_stream` / `bwt_mtf_expand` / `bwt_qlfc_decode`. The shipped encoder
   never emits those ids on the canonical route (id 2, arithmetic order-1, is the
   canonical winner; ids 0 and 4 are rejected up front in `bwt_backend_encode`).
   The probe **asserts** the postcoder id it sees and fails loudly otherwise;
3. the probe's `DecodeSink` substitute folds an FNV-1a instead of ANVIL's CRC-32
   (a correctness check, not a timing cost). The probe's reconstructed output is
   verified by **SHA-256** in the driver tool, which is strictly stronger.

The vendored `third_party/libsais/src/libsais.c` is **not copied**: the probe links
the same `anvil_libsais` static library target (same `-O3 -DNDEBUG` flags) that the
`anvil` binary links, so the inverse-BWT code executed here is byte-identical to the
shipped one.

## Fidelity controls

The probe's copy must not be slower or faster than the shipped decoder, or the
share would be an artefact of the copy. Three controls are measured in the same job:

1. **byte-exactness** — the probe's reconstructed output must have the source's
   SHA-256 (the tool fails the job otherwise);
2. **probe-vs-shipped ratio** — the probe's own decode total against the shipped
   `anvil d` wall time, reported per measurement row;
3. **an in-process A/A null** — every probe round runs the identical decode twice;
   the ratio of the two halves' total decode times must have a CI spanning 1.0, or
   the series is `TIMING_BLOCKED` and counts for nothing.

## Files

| file | role |
|---|---|
| `stage_probe.cpp` | the probe (stage clocks, A/A null, Brotli one-shot vs streaming) |
| `README.md` | this file |
| `../../tools/i11_postcoder_share.py` | the driver (`prepare` / `measure` / `summarize`) |
| `../../.github/workflows/anvil-i11-postcoder-share.yml` | the CI job |
| `../../docs/I11-RESULTS-D2-postcoder-share.md` | the results write-up |
