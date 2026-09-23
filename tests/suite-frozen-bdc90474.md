# Frozen-src canonical suite refresh — BDC90474 (bench, consolidated window)

Owner: `bench`. Window `w-bench-frozen-suite-20260912T120527Z` (main, 12:05:30-12:30:15Z)
+ `w-bench-frozen-pass2-20260912T123023Z` (tagged pass for the four `generated.*` files,
12:30:26-12:40:32Z). Frozen canonical: `src/anvil.cpp` BDC90474; `build\anvil.exe`
`72D65150CBD65182C69BA85DAD3A8961C25DE48E794FD9823BCAB4ED20A36505`; `build\anvil_bench.exe`
`379341D93B6504EBDADF8BDB7FE69B2756142FD9823E5398A31E9A5C2176851E`.

Method: per-rep process invocations of `anvil_bench.exe <file> 1` (all 25 codecs),
pinned (core 19 main / core 18 pass2) HIGH priority, 1 Hz total+own sampler.
reps = 3 for 11 files, 7 for generated.json + synth-timeseries.bin. All rc=0, all
roundtrip OK. **Grade: RANKING-GRADE, not citation-grade** — pinned-core pre-window
util 26.9% (main) / 9.2% (pass2), both above the <5% gate. `compressed_bytes` are
deterministic and citable; throughput columns are ranking-grade.
Runner note: `Path.GetFileNameWithoutExtension` collapsed generated.json/.jsonl/.log/
.sqlite to one tag, clobbering pass-1 outputs for those four; fixed with a tagged
pass-2 (`generated.json` etc.). Aggregation prefers pass-2 for those four.

Artifacts:
- `tests/benchmark-suite.frozen-bdc90474.csv` — 351 median rows (sha `fc422712...`)
- `tests/suite-frozen-bdc90474-reps.csv` — 1,269 per-rep rows
- `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` — + xz-9e refs (sha `af44d9d3...`)

## Byte identity vs the previous grid

351/351 rows byte-identical to `tests/benchmark-suite.csv` (worktree C70179EA):
**0 diff, 0 missing** — the 494/494 encode-wire identity gate independently confirmed
across the whole 13-file suite on the frozen binary.

## Decision rows on the frozen build (median, reps=7)

| file | codec | bytes | ratio | enc MB/s | dec MB/s (min-max) | dec CV% |
|---|---|---|---:|---:|---|---:|
| generated.json | anvil-mdl-rans (mode-10) | 89,589 | 0.108 | 1.596 | **274.707** (260.4-279.8) | 2.61 |
| generated.json | brotli-q11 (binding) | 79,172 | 0.09566 | 0.780 | **557.462** | 7.75 |
| generated.json | xz-9e | 76,952 | 0.092975 | 1.32 | 178.7 | — |
| synth-timeseries | anvil-hotop-rlzp-rans | 140,898 | 0.5032 | 0.047 | **197.141** (110.8-202.9) | 16.59 |
| synth-timeseries | brotli-q6 (binding) | 120,669 | 0.43096 | 34.129 | **294.706** | 10.80 |
| synth-timeseries | brotli-q9 | 120,669 | 0.43096 | 10.138 | 289.017 | 17.78 |
| synth-timeseries | brotli-q11 | 134,718 | 0.48114 | 0.445 | 190.697 | 6.09 |

Anti-tie decision, RESTATED on MEASURED routes (coordinator v3 + decode-perf
cap-basis correction; supersedes the earlier DECODE-TIE):

- The pre-A9 materialization caps (2.0580 / 1.4556) are **NOT admissible** — arch's
  MAT A/B falsified the materialization leg as removable (mdl +1.4% / hotop-rlzp
  +27.6% slower; see the MAT/ALLOC handoffs). No mat cap exists post-A9.
- MEASURED removable-set caps: generated.json mode-10 ALLOC-only **1.352x** (arch
  paired window, 4.1604 -> 3.0766 ms); synth-timeseries hotop-rlzp ALLOC-only
  **1.074x** (1.4829 -> 1.3802 ms). leg-4 (1.179x aggregate) is a BWT-postcoder
  route and does not apply to these two cells; it also cleared neither its own
  ~1.4x target nor these R' values.
- Against the frozen measured requirements: generated.json R = 557.462/274.707 =
  2.0293, **R' = 2.0699** -> best measured route 1.352x is ~35% SHORT;
  synth-timeseries R = 294.706/197.141 = 1.4949, **R' = 1.5248** -> 1.074x is ~30%
  SHORT.
- **BOTH CELLS: DECODE-SHORT, closed by measurement** (not noise-limited). No
  quieter paired window will be run — there is no measured route left to resolve on
  these cells. Store-path throughput stays un-cited until arch names the anomaly's
  cause.

## Co-listed frontier tuples (reference class + grid)

| suite grid | ref class | non-dom | FRONT-GAP | FRONT-CROSSING | DEGENERATE | dominated |
|---|---|---:|---:|---:|---:|---:|
| frozen BDC90474 | v1 brotli/zstd | 33 | 5 | 0 | 28 | 435/468 (471/504) |
| frozen BDC90474 | **v2 +xz-9e** | **33** | **5** | **0** | **28** | **435/468 (471/504)** |
| worktree C70179EA | v1 | 5 | 5 | 0 | 0 | 463/468 (499/504) |
| worktree C70179EA | v2 +xz-9e | 1 | 1 | 0 | 0 | 467/468 (503/504) |
| committed HEAD fc23d9a | v1 | 5 | 5 | 0 | 0 | 411/416 (443/448) |
| committed HEAD fc23d9a | v2 +xz-9e | 4 | 4 | 0 | 0 | 412/416 (444/448) |

All combined CSVs double-run hash-identical. **ZERO FRONT-CROSSING on every grid.**

## Why the frozen grid shows 33 non-dominated

The frozen build's **ratio=1.000 store path is ~5-6x faster** than the previous grid's
rows: synth-arith/random anvil encode 325-380 MB/s -> **1,950-2,184 MB/s**, decode
323-390 -> 1,010-1,484 MB/s. Those 28 cells are the fastest points at ratio 1.0, so no
reference dominates them; under gate R-2 they are **DEGENERATE** (ratio >= 0.95),
never crossings. The remaining 5 are the familiar generated.json encode-plane
FRONT-GAP rows, now fast enough on encode (1.4-1.6 MB/s) to escape xz-9e's 1.32 MB/s.

**Root cause (arch, 2026-09-12):** S6-1b CRC fast path — bytewise CRC was the
per-byte cost on every store/raw block write; slicing-by-8 + PCLMUL removed it
(1.95 -> 0.06-0.08 ns/B), so the store path became memory-bound (~2 GB/s). The
predicted 5-6x matches the observed 5-6x; the IEEE CRC-32 value is bit-exact
(16,529 unit + 5,784 corpus-chunk checks, 0 fail) so container bytes are identical.
The prior grid's 325-380 MB/s is consistent with a bytewise-CRC binary (worktree
C70179EA predates S6-1b/PCLMUL). **SIGNED (w-arch-storepath-20260912T1300Z, interleaved reps=5, core18, 1t, same
recipe, only crc32 differs; wire_fnv identical all arms):** random.bin enc
bytewise 2.4258 ns/B (412 MB/s) / slice8 0.8652 (1156) / pclmul 0.5341 (1872);
dec 2.7611 (362) / 1.1261 (888) / 0.8221 (1216). synth-arith.bin enc 2.3898 (418)
/ 0.9020 (1109) / 0.4969 (2012); dec 2.7508 (364) / 1.2301 (813) / 0.8723 (1147).
The bytewise arm reproduces the prior grid's ~410/370 band **under this runner's
per-rep protocol**, so the method is exonerated: the 28-DEGENERATE count is a real
store-path speed effect (S6-1b CRC fast path), not protocol-specific. Store-path
throughput is now citable with this window + arm shas (bytewise 51C6A939 / slice8
B743061C / pclmul 4F00627B).

## Citation

The frozen BDC90474 grid (v2, +xz-9e) is the current canonical: **33 non-dominated
(5 real FRONT-GAP + 28 DEGENERATE) | 0 FRONT-CROSSING | 435/468 dominated (471/504)**.
Always add GRID-THIN (no zstd 4-22 / brotli lw30). Windows are ranking-grade; byte
counts are citable. Store-path throughput is citable with the signed w-arch-storepath
window + arm shas (wire-invisible CRC fast path).
