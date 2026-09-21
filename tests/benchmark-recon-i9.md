# I9 reference-front reconnaissance — 9 newer corpus cells (bench)

Task: carried I8 rank-6 / coordinator step (3). Owner: `bench`.
Window: `w-bench-recon-20260912T103740Z`, 2026-09-12T10:37:40Z-10:46:17Z.
Binary: `build\anvil_bench.exe` sha256 `56092B9BAB31D49D06996F78532DBCB1F1D853F7E9F0FE35B772C896939D789B`
(src `38409E26`, ALLOC-only; the shared build moved mid-announcement from 62BC6631 —
the script hashed the current build at start, so all rows are 56092B9B/38409E26).
Command: `python tools\bench_suite.py <9 files> --bench build\anvil_bench.exe --reps 1 --out ...`
reps=1, UNPINNED, load sampler median 36% / max 100% -> **ranking-grade**, throughput columns
not citation-grade; `compressed_bytes` are deterministic and citable.
Raw: `tests/benchmark-recon-i9.csv` (sha256 7214E864...), summary `tests/benchmark-recon-i9-summary.csv`.

## STATUS: RULED — mechanical candidate, crossing REJECTED (FRONT-GAP, dual-bar)

research-gate ruling `docs/gate-ruling-i9-recon-crossing.md` + ledger PART XIV A13:
the three arbiter flags reproduce and the mechanical R-2 candidate is real, but the
**same-transform control destroys it**:

> `xz -9e --delta=dist=23` on `synth-columnar-align.bin` = **21,332 B (ratio 0.07729)**
> — reproduced here with XZ Utils 5.6.4 (`--filters='delta:dist=23 lzma2:preset=9e'`;
> the gate records 21,340 B, +8 B CLI framing) — vs the row's 0.368 = **4.76x better**.
> Classification: **FRONT-GAP**, not a crossing. Recon tally **3 non-dominated |
> 3 FRONT-GAP | 0 FRONT-CROSSING | 0 DEGENERATE**. The committed tuple is unchanged.
> Do not propagate "first crossing".

Binding labels from the ruling: `NON-DEFAULT/RESEARCH-CONFIG` (hotop-rlzp is retired
as a default; encode 0.048 MB/s) and `GRID-THIN` (missing zstd 4-22 / brotli lw30
tiers). Recon throughput columns stay ranking-only.

## Cells and gap-to-front (raw-byte references + xz -9e)

xz measured with XZ Utils 5.6.4 `xz -9e -T1` (byte-deterministic). xz decode context
on the key cell is process-level and is NOT used for any claim.

| file | anvil best | codec | best raw ref | ref | gap (raw refs) | same-transform control |
|---|---|---|---:|---|---:|---|
| synth-columnar-align.bin | 0.3680 | anvil-hotop-rlzp-rans | 0.3840 | brotli-q11 | **−4.17%** | **xz -9e --delta=dist=23 = 0.0773 → dominates** |
| pe-python.exe | 0.5270 | anvil-greedy-arith | 0.4855 | xz-9e | +8.54% | n/a |
| pe-notepad.exe | 0.5770 | anvil-mdl-rans | 0.5098 | xz-9e | +13.18% | n/a |
| pe-where.exe | 0.3710 | anvil-mdl-rans | 0.3206 | xz-9e | +15.73% | n/a |
| synth-drift-stride.bin | 0.1760 | anvil-hotop-rlzp-rans | 0.1518 | xz-9e | +15.94% | (delta sweep not run) |
| pe-winver.exe | 0.1960 | anvil-dp-arith | 0.1690 | brotli-q11 | +15.98% | n/a |
| pe-ninja.exe | 0.4980 | anvil-mdl-rans | 0.4097 | xz-9e | +21.55% | n/a |
| pe-git.exe | 0.4960 | anvil-mdl-rans | 0.4057 | xz-9e | +22.26% | n/a |
| synth-counters.log | 0.1050 | anvil-hotop-rlzp-rans | 0.0848 | xz-9e | +23.78% | (delta/other not run) |

Ranking change vs the brotli/zstd-only view: xz -9e is the best raw reference on 7 of
9 cells (drift-stride +7.32% → +15.94%; counters.log +15.38% → +23.78%). Only
`pe-python.exe` (+8.54%) is in the ≤12% band on raw refs. No cell is below the
dual bar; the `synth-columnar-align.bin` "anvil leads" statement is true ONLY against
raw-byte references and is struck by the transform control above.

## Arbiter result (official tool, double-run)

`python tools\pareto_front.py tests\benchmark-recon-i9.csv --out tests\pareto-baseline-recon-i9.csv`
run twice -> byte-identical (`sha256 133690EB3FA414B28F7497B2DA8036598A925875F5C53EF0FE024524E9674703`).

EXTENDS_FRONT rows (3), all `synth-columnar-align.bin`:
1. encode plane: `anvil-hotop-rlzp-rans` ratio 0.368, 0.048 MB/s — FRONT-GAP (dual bar).
2. decode plane: `anvil-hotop-rlzp-rans` ratio 0.368, 117.163 MB/s — FRONT-GAP (dual bar).
3. decode plane: `anvil-hotop-budget-rans` ratio 0.429, 273.973 MB/s — FRONT-GAP
   (bracket q11/q6 low, zstd-9 high).

## Follow-ups

- Reference-class adoption (coordinator I9-6): add `xz -9e` (5.6.4) as an `xz-` ref
  to the arbiter + reference CSVs, same-transform controls side-channel, versioned,
  both suite grids co-listed, `GRID-THIN` flagged. In progress in the bench lane.
- Two corpus cells outside the I7 "9 new": `synth-telemetry-f64.bin`,
  `synth-ndjson-columnar.ndjson` — not yet recon'd.
