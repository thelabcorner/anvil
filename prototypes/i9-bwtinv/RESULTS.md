# i9-bwtinv — RESULTS (prototype measurements + falsifications)

Owner: `bwtinv`, task `bwt-inverse`. Lane: `prototypes/i9-bwtinv/`, build `build-i9-bwtinv/`.
`src/anvil.cpp` untouched; integration handoff in `INTEGRATION-SPEC.md`.

## Headline (measured)

| | dickens 10.19 MB | webster 41.46 MB | enwik8 100 MB |
|---|---:|---:|---:|
| shipped `libsais_unbwt` call shape, 1t (`plain_fresh`) | 20.64 MB/s | 18.36 MB/s | 17.00 MB/s |
| prototype `libsais_unbwt_aux` (I-indexed), 1t | **70.18** (r=262144) | **64.15** (r=32768) | **60.33** (r=524288) |
| same-core-count speedup | **3.40x** | **3.49x** | **3.55x** |
| prototype `libsais_unbwt_aux_omp`, 16t | **604.18** (r=32768) | **502.69** (r=131072) | **448.67** (r=1048576) |
| 16t speedup (context-only, thread-imbalanced) | 29.3x | 27.4x | 28.9x |

Window `w-bwtinv-20260912T064334Z`, reps=7, interleaved, median; raw CSV
`measure/w-bwtinv-20260912T064334Z-degraded.csv`, attestation + input hashes
`measure/w-bwtinv-…-attestation.md`. Label **`measured (parallel; attestation attached)`** —
host load median 36.3% / max 90.9% (mostly non-swarm: WSL/webstorm), CV 4.8–13.3% on
baseline rows; the <10% quiet gate was unreachable, so per bench this is ranking-only until a
quiet rerun. Exe sha256 ref `E47D0A51…`, omp `FC345062…`.

`I` size (encoder-supplied index, int32 × `1+(n-1)/r`): 156 B (dickens r=262144), 624 B
(r=65536), 4980 B (r=8192); webster 1268 B (r=131072), 5064 B (r=32768); enwik8 384 B
(r=1048576), 1528 B (r=262144). At a ~1024-block single-thread policy this is ~2–5 KB per
whole-file block (~0.02–0.05% of a 10 MB block).
Exact at `r` = smallest pow2 with `r*1024 >= n` (1t policy): dickens 2492, mr 2436, nci 4096,
osdb 2464, reymont 3236, webster 2532, x-ray 2072 B → Silesia 7-file total **19,328 B**
(0.0083% of 211,938,580; ≈0.96% of the 2,009,105-B xz win margin); enwik8 3052 B.

## End-to-end projection (DERIVED, not measured end-to-end)

decode-perf stage splits on worktree source 62BC6631 (canonical containers): v1 — dickens
10.0 MB/s (postcoder 40.1%, unbwt 63.3%); webster 12.5 (postcoder 35.2%, unbwt 69.4%);
v2 re-validated — dickens 11.2 MB/s (postcoder 43.9%, unbwt 50.2%, residual 5.0%); webster
12.5 (postcoder 34.2%, unbwt 61.0%, residual 3.8%); cross-run range postcoder 34–44%,
unbwt 50–69%. Applying my measured k: dickens 1t 1.56–1.71x (→17.1–17.4 MB/s), 16t 1.95–2.37x
(→21.9–23.7); webster 1t 1.82–1.91x (→22.7–23.9), 16t 2.65–2.69x (→33.1–33.7); enwik8
~1.9x / ~2.8x (s_u≈0.66 assumed). Portfolio (old 7-file BWT 13.0 MB/s, 5 Brotli files 146.3):
BWT-only ≈21–25 (1t) / 28–36 (16t); whole-portfolio ≈34–38 / 43–54 vs old 21.4; xz 78.7.
**Postcoder floor:** even free unbwt caps dickens BWT decode at 22.6–24.9 MB/s (webster
~35.5) — the 40+ MB/s reopen bar needs a postcoder leg as well (coordinator assigned
decode-perf the postcoder profile/spec). decode-perf independently measured postcoder-only
speeds 25.22 / 35.87 / 28.41 MB/s (dickens/webster/enwik8, median-7, canonical containers)
= the BWT-only ceilings with a free unbwt; required postcoder speedup 1.66x/1.13x/1.46x
(~1.38x portfolio), cheap edges insufficient (C=1.02 buffered renorm).
With arch's leg-4 postcoder A/B (1.179x aggregate, whole-decode) the two measured legs combine
to ~1.77–1.95x end-to-end (conservative) / ~2.15–2.38x if the A/B is read as a stage speedup;
with the real 3.4x aux unbwt the postcoder must reach ~3.8–7.5x (not 1.38x) to clear the
40 MB/s BWT-only bar — see INTEGRATION-SPEC §5.1.

## Findings

1. **`libsais_unbwt_aux` with `r<n` + encoder index is 3.3–3.5x faster at 1 thread**
   (same-core-count, citation axis, pending quiet rerun). Mechanism: 8 interleaved LF walks
   (`libsais.c:7882-7941`) hide DRAM latency vs the single serial walk in `decode_1`.
2. **`r=n` + threads is a FALSIFIED decode lever** (`libsais_unbwt_omp` parallelises init
   only): dickens 23.15 MB/s at t=16 vs 20.64 baseline; webster 21.33 vs
   18.36; enwik8 17.70 vs 17.00. No walk speedup.
3. **`_omp` with `r<n` scales ~linearly to 12c/24t**: 27–29x vs shipped 1t. Context-only
   (`THREAD-IMBALANCE: prototype 16t vs 1t baseline`).
4. **Wire cost small but real**: ~40 B–8 KB per block; **exact 19,328 B across the 7-file
   Silesia portfolio at the ~1024-block policy** (≈0.96% of the 2,009,105-B win margin;
   enwik8 3,052 B). Must be charged by bench and registered by format.
5. **BWT bytes identical**: `libsais_bwt_aux` output == `libsais_bwt` and `I[0] == primary`
   at n = 64, 32, 5,345,280, 10,192,446, 41,458,703, 100,000,000 (`probe_aux.cpp`,
   `probe_file.cpp`); decode byte-identical to raw for every variant (harness gate).
6. Cache effect explains the legacy 13–16 MB/s: plain walk ~31 MB/s when P (4n) fits L3
   (xml 5.3 MB) vs ~18 MB/s at 10–41 MB; aux is ~60–70 MB/s regardless.
7. `freq` precompute and buffer reuse: within noise (not levers at these sizes).

## Instruments / reproduce

```powershell
. .\env.ps1
cmake -S prototypes/i9-bwtinv -B build-i9-bwtinv -G Ninja -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_C_COMPILER=clang-cl -DCMAKE_SUPPRESS_REGENERATION=ON
ninja -C build-i9-bwtinv
build-i9-bwtinv\unbwt_bench_ref.exe --reps=7 --warmup=1 --csv=prototypes\i9-bwtinv\measure\scan.csv `
  scratch\ratio-first\corpora\silesia\dickens scratch\ratio-first\corpora\silesia\webster scratch\ratio-first\corpora\enwik8dir\enwik8
build-i9-bwtinv\unbwt_bench_omp.exe --reps=7 --warmup=1 --csv=prototypes\i9-bwtinv\measure\scan.csv `
  scratch\ratio-first\corpora\silesia\dickens scratch\ratio-first\corpora\silesia\webster scratch\ratio-first\corpora\enwik8dir\enwik8
# gated PR-4 window (gate: CPU median <10%, no anvil*/ninja/clang*/cmake*); -Force = labelled degraded
pwsh -File prototypes\i9-bwtinv\run_gated.ps1 [-Force]
```

Upstream `third_party/libsais/src/libsais.c` is compiled unmodified by `sais_ref` (shipped
config) and `sais_omp` (same file + documented `LIBSAIS_OPENMP`, clang-cl `/openmp`, lld
`libomp.lib`). Harness variants: `plain_fresh` (ANVIL call shape), `plain_reuse`, `plain_freq`,
`aux_r<r>` (1t), `omp_rn_t<t>` (init-only control), `omp_r<r>_t<t>`. Every variant is
byte-identity verified before timing; `reps_ms` column carries raw per-rep values.
