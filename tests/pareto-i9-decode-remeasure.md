# I9 canonical decode re-measure — generated.json mode-10 mdl + timeseries hotop-rlzp

Owner: `bench`. Window: `w-bench-remeasure-20260912T072549Z`.
Purpose (coordinator request, msg_0becb8c0): decide whether ANY decode route survives
by re-measuring the two binding rows on a roundtripping binary, plus the binding
reference rows, in one PR-4 window.

## Provenance / attestation

| field | value |
|---|---|
| binary | `build-i9-arch\anvil_bench.exe` sha256 `D6A70ACD9F2AAFF401F5B5ED34AFB009F4CC6681F163B85CD3052976208CC901` |
| source | `src/anvil.cpp` sha256 `62BC663122D8240C1A79A29D315CCA44AFFFCBC4DE59F3B555B350922FFEE74A` (HEAD `fc23d9a`); same source as canonical `build\anvil.exe` `0D1E130B` |
| codegen | build-i9-arch = clang-cl /O2 (matches decode-perf t3 protocol). NOTE: shared `build\` is `-O3 -DNDEBUG -std:c++20` — do not compare timings across codegens |
| command | `anvil_bench.exe <file> 1` x 7 process invocations per file (in-process timing, 1 rep each) |
| window | start 2026-09-12T07:25:52Z, end 2026-09-12T07:30:17Z |
| reps | 7 per codec per file; all roundtrip=OK |
| pinning | core 18 (mask 262144), priority High on every invocation |
| threads | 1 (harness sets no thread count; anvil::Options.decode_threads default 1) |
| load | 1 Hz sampler `load-sampler.csv`: total CPU median 37.0% / max 72.0%; own-process share <= 16.5 CPU-s over the window |
| core gate | pre-window pinned-core mean 8.27% -> FAILS the strict <5% core gate; window is **ranking-grade**, not citation-grade (see PR-4 section 3b) |
| raw data | `tests/decode-remeasure-i9.csv` (all 25 codecs x 7 reps x 2 files) |

## Measured medians (MB/s, 7 reps) — target rows and binding references

| file | codec | bytes | ratio | enc MB/s | dec MB/s | dec CV% |
|---|---|---|---|---|---|---|
| generated.json | **anvil-mdl-rans** (mode-10 mdl) | 89,589 | 0.10824 | 1.564 | **189.146** | 15.95 |
| generated.json | anvil-mdl-rans-l0 | 89,589 | 0.10824 | 1.582 | 188.564 | 2.76 |
| generated.json | anvil-mdl-rans-l001 | 89,589 | 0.10824 | 1.576 | 190.944 | 2.44 |
| generated.json | brotli-q11 (binding ref) | 79,172 | 0.09566 | 0.770 | **515.390** | 5.51 |
| generated.json | zstd-19 | 93,363 | 0.11280 | 2.575 | 1213.762 | 24.10 |
| synth-timeseries | **anvil-hotop-rlzp-rans** | 140,898 | 0.50321 | 0.047 | **177.891** | 9.21 |
| synth-timeseries | brotli-q6 (binding ref) | 120,669 | 0.43096 | 33.054 | **293.132** | 16.81 |
| synth-timeseries | brotli-q9 | 120,669 | 0.43096 | 8.472 | 279.916 | 8.79 |
| synth-timeseries | brotli-q11 | 134,718 | 0.48114 | 0.438 | 217.933 | 2.47 |

Binding rule: the row must exceed the decode of every reference whose ratio is
<= the row's ratio; R = max(dec of those refs) / target dec.
- generated.json: only brotli-q11 qualifies (0.09566 <= 0.10824) -> R = 515.390 / 189.146.
- synth-timeseries: q6/q9/q11 qualify -> binding is q6 at 293.132 -> R = 293.132 / 177.891.

## Anti-tie arithmetic (DECODE-GO / DECODE-TIE / DECODE-SHORT)

R' = 1.02 x R; single-component cap C = 1/(1-s); no compounding legs.

### generated.json, mode-10 mdl
- R = 2.7246; **R' = 2.7791**
- mat share s=0.514 (decode-perf profile, their build) -> C_mat = **2.058**
- alloc share s=0.215 -> C_alloc = 1.274
- **DECODE-SHORT**: best admissible single-component cap (2.058) < R' (2.779), by 26%.
- Combined mat+alloc (=3.69) is a **PROJECTION** and is NOT an admissible cap under v2;
  it can only count if the combined leg is measured end-to-end (then it is a
  measured result, not a cap). If built and measured, it would clear 2.779.
- Cross-check with the frozen suite row (112.8 MB/s): R'=4.66, still SHORT.

### synth-timeseries, hotop-rlzp
- R = 1.6479; **R' = 1.6809**
- mat share s=0.313 -> C_mat = **1.456**
- **DECODE-TIE (unresolved), not SHORT/GO**: the cap (1.456) sits below this window's R'
  (1.681) but above decode-perf's window R' (1.327 from their 216.9 MB/s target vs
  282.173 q6). The two windows straddle the threshold; input CVs (target 9.2%,
  q6 ref 16.8%) put R' at ~1.68 +/- 0.32, i.e. the cap is inside 1 sigma.
  Per PR-4 section 3b the wall-clock override also demands the margin exceed
  max(2%, CV_target, CV_ref) = 16.8%; no measured margin can be claimed at this precision.

## Frontier stability (does the decode lift flip the arbiter?)

No. For all 5 FRONT-GAP rows (generated.json encode plane, ratio 0.10824/0.11152),
`brotli-q11` has ratio 0.09566 <= row ratio AND fresh decode 515.390 >= row decode
(189.1 for mdl; ~220-223 for the shape rows). Every GAP row remains decode-dominated
by q11 at equal-or-better ratio, so the 5|5|0 tuple is unchanged by the decode lift.

## Verdict

- **generated.json mode-10 mdl: DECODE-SHORT** on admissible single-component arithmetic.
  The only remaining opening is a *measured* combined mat+alloc leg (projection 3.69x > R' 2.779).
- **synth-timeseries hotop-rlzp mat leg: DECODE-TIE** — this window says SHORT, decode-perf's
  window says GO; the decision needs a paired, core-gated re-measure of target and binding
  ref on ONE frozen binary. Until then neither GO nor SHORT is citable.
- This window is ranking-grade (pinned-core gate failed at 8.27%); raise to citation-grade
  with a quiet core and a paired design before any of these numbers supports a bar.
