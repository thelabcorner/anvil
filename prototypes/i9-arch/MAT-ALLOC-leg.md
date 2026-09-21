# arch I9 — Decode leg 2: MATERIALIZATION (staged) + ALLOC (staged)

Status: MAT built + byte-identity gated; ALLOC built, gates in progress; timing
pending in a bench-coordinated window. No commit (coordinator owns commits).

## Source/binary pins

| artifact | sha256 | notes |
|---|---|---|
| src/anvil.cpp pre-MAT (frozen 62BC6631) | 62BC663122D8240C1A79A29D315CCA44AFFFCBC4DE59F3B555B350922FFEE74A | baseline for MAT |
| src/anvil.cpp MAT-only | FECCF10E2ADC083F062ECECACE1D6A7149BBD2EB41129C8E3C4E038F7152043B | MAT hunks only |
| src/anvil.cpp MAT+ALLOC | EAD0B91A61434B0C71CBDDF21A030748E9E16732A0757DC9A1AF5F5F0D5F432B | current worktree |
| build-i9-arch/anvil.exe MAT-only | 5642272AF7A84BBA0D36B00278288D51277C8373AC609C7D9746C35C84CFFC50 | gated |
| paired bench arm BEFORE (pre-MAT) | prototypes/i9-arch/anvil_bench_premat.exe 7BE7CB34A25EEAD736265C25085C9AFE0FB51BE3597A9BCC287F65786A92855F | manual clang-cl /O2 |
| paired bench arm MAT | prototypes/i9-arch/anvil_bench_mat.exe 34735B07F696F7AE37C2949C2758AA77A9D9304503631C465E315FF750229828 | manual clang-cl /O2 |
| paired bench arm MAT+ALLOC | prototypes/i9-arch/anvil_bench_matalloc.exe 6E817033FEA26427A67EE1A5FB82729C94720C8F5816014CED02DA8D393EFDAE | manual clang-cl /O2, gates pending |

The three paired bench arms are built from `tools/bench_native.cpp` with the
include retargeted (`bench_premat.cpp` -> `prototypes/i9-decode-perf/anvil_wt_raw.cpp`
= pre-MAT source; `bench_mat.cpp` -> `../../src/anvil.cpp` at the pinned state),
same flags: `/nologo /O2 /std:c++20 /EHsc /DNDEBUG /DANVIL_HAVE_BROTLI=1 /DANVIL_HAVE_LIBSAIS=1`.

## What changed

### MAT leg (decoder-only, wire untouched)

- `decode_tokens_rans` (block mode 10, the generated.json mdl cell): five
  substreams were decoded eagerly into `std::array<std::vector<uint8_t>,5>` via
  `decode_stream`, then consumed by the token loop. They are consumed strictly in
  order, so the vectors + full decode passes were pure overhead. Now
  `StreamPull`-based on-demand pulls (same `StreamPull` machinery already used by
  the mode-15 hot path): `next_byte` for the type stream, `read_varint_pull` for
  len/dist, `pull_bytes` for literal runs. End-of-stream strictness preserved via
  `StreamPull::at_end()` on all five substreams.
- `decode_tokens_hotop_fused` (block mode 15, synth-timeseries hotop-rlzp cell):
  the 7 macro streams (types/ll/ml/dflags/dvar/masks/resid) were eagerly
  materialized; now all are pulled on demand in the same order the token loop
  consumes them. Opcodes + literals were already pulls.
- Rejected/rejection paths unchanged; malformed input still throws (different
  message for "fewer types than output" is the only user-visible text change).

### ALLOC leg (decoder-only, wire untouched; staged on top of MAT)

- `decode_tokens_rans_into` / `decode_tokens_hotop_fused_into` write straight
  into a caller-provided buffer; the vector-returning functions remain as thin
  wrappers (used by the generic dispatch).
- `decode_one_block_into(mode,p,plen,dst,blen)` returns true for modes 10/15.
- `decompress` serial path: for revision-1 modes 10/15 it grows `out` once and
  decodes the block directly into `out.data()+base` (no per-block vector, no
  concat copy); CRC is computed over the destination span. All other modes keep
  the exact generic path.
- `out.reserve` hint is now the declared total, bounded by
  `max(1 MiB, 256 x container size)` so a crafted tiny header cannot force an
  unbounded reserve (the previous `min(total,max_block)` reserved up to 128 MiB
  for rev-2 regardless of container size).
- Parallel path (`--decode-threads>1`) unchanged.

## Hunk boundaries (current file line anchors)

- MAT mode-10: lines 2107-2129 (`decode_tokens_rans_into` + wrapper).
- MAT mode-15: line 2934 function head, 2955-2973 parse section, macro loop
  (~2994-3048); vector wrapper 3052-3056.
- ALLOC: mode-10 `_into` 2111; mode-15 `_into` 2934; `decode_one_block_into`
  4686-4693; `decompress` reserve 4706-4710; serial direct-decode 4725-4736.
- (P0/CRC/BWT/transform-3 hunks from the previous legs are unchanged and were
  reported separately.)

## Gate evidence

Run on the MAT-only binary `build-i9-arch/anvil.exe` 5642272A unless noted.

- Full-corpus roundtrip 24 files x 18 specs (auto/greedy/dp/sparse/mdl/shape/
  topology/tcopy/hotop/ariref/ratio/bwt/auto + LZP clean-reject), decode serial
  AND `--decode-threads=4`: **442/442 PASS** — `logs/gate_roundtrip_mat.log`.
- `python tests\fuzz.py --exe build-i9-arch\anvil.exe --cases 50`: **PASS**
  seed=41246 registry_block_modes=14 registry_transforms=5 — `logs/fuzz50_mat.log`.
- Encode wire identity MAT vs pre-MAT across 24 files x 19 bench specs:
  **494/494 SAME, 0 DIFF** — `logs/gate_encode_identity_mat.log`.
- Targeted byte identity (generated.json mdl, timeseries hotop-rlzp, generated.log
  hotop, doc.md auto): containers byte-identical, same sizes
  (89,589 / 140,898 / 175,550 / 1,444 B).
- ALLOC arm (EAD0B91A): single-rep row check passed roundtrip=OK on both target
  rows with identical wire sizes; full gates to be re-run on the ALLOC build
  before any ALLOC number is cited (same 442/442 + fuzz + identity).

## Measurement — MAT step (window w-arch-mat-20260912T0905Z)

Interleaved paired arms on core 18, reps=7, threads=1, wire identical both arms.
Log: `logs/mat_ab_w-arch-mat.log`; command in the script header.

| row | BEFORE (premat 7BE7CB34, src 62BC6631) | MAT (34735B07, src FECCF10E) | delta |
|---|---|---|---|
| generated.json `anvil-mdl-rans` | 4.1545 ms (CV 2.64%) | 4.2117 ms (CV 0.73%) | **+1.4% slower** |
| synth-timeseries.bin `anvil-hotop-rlzp-rans` | 1.4495 ms (CV 2.77%) | 1.8499 ms (CV 9.48%) | **+27.6% slower** |

Wire anchors: 89,589 B / 140,898 B (identical). Load field per rep in the log
(9-48%).

**VERDICT: the materialization leg is FALSIFIED as implemented.** Replacing
eager stream materialization with on-demand `StreamPull` does not reduce decode
time; it regresses the mode-15 macro path by 27.6%. The t3 "26-31%
materialization" residual is not removable liveness overhead: the entropy-decode
work is unchanged and per-symbol pull dispatch is more expensive than bulk decode
plus vector indexing. MAT alone cannot reach R'=2.35x (mdl) or the 1.30x staging
bar. ALLOC step measurement pending on the `mat -> matalloc` delta.

## Measurement — ALLOC step (window w-arch-alloc-20260912T0945Z)

Arms `mat` (34735B07) vs `matalloc` (6E817033), same conditions; the ALLOC delta
given the MAT state. Log `logs/alloc_ab_w-arch-alloc.log`.

| row | mat (ms) | matalloc (ms) | ALLOC delta |
|---|---|---|---|
| generated.json `anvil-mdl-rans` | 4.2263 (CV 22.5%) | 3.0659 (CV 5.9%) | **1.38x faster** |
| synth-timeseries.bin `anvil-hotop-rlzp-rans` | 1.8312 (CV 14.5%) | 1.7488 (CV 1.9%) | 1.05x faster |

Combined (premat -> matalloc): mdl 4.1545 -> 3.0659 ms = **1.355x**;
timeseries 1.4495 -> 1.7488 ms = **0.83x** (dominated by the MAT regression).
Wire anchors 89,589 / 140,898 B identical in every arm.

Interpretation: ALLOC (direct decode into the output slot + no concat copy +
bounded reserve) IS a real wire-invisible decode win on the mode-10 mdl row
(~1.38x). MAT is a pure regression on mode-15 and neutral on mode-10. Therefore
MAT is reverted and ALLOC is re-based on the eager stream bodies (ALLOC-only).

### ALLOC-only re-base (current source)

- `src/anvil.cpp` MAT+ALLOC: `EAD0B91A...`; **ALLOC-only: `38409E26A439315E642873DE68D133E62C33E813DC5FDD21C866598485ED882D`**
  (MAT lazy bodies removed; eager `decode_stream` restored; `_into` + direct
  serial decode + bounded reserve kept).
- ALLOC-only paired arm: `prototypes/i9-arch/anvil_bench_alloc.exe`
  `13AA6C31FA902BFEB3B4BA0E186576E9A70D2CB984142329C8C2DC05B2FF02D3`
  (same manual recipe as premat/mat/matalloc).
- Shared canonical build from the ALLOC-only src: `build/anvil.exe`
  `E8AA2E4808CCB6AC9F66262F6710DE1300E0E265DC85D1F10616B378FDB223CA`,
  `build/anvil_bench.exe` `56092B9BAB31D49D06996F78532DBCB1F1D853F7E9F0FE35B772C896939D789B`.

### Measurement — ALLOC-only vs pre-MAT (window w-arch-alloconly-20260912T1045Z)

Interleaved paired, reps=7, core 18, threads=1, wire identical. Log
`logs/alloconly_ab_w-arch-alloconly.log`.

| row | BEFORE premat (ms) | ALLOC-only (ms) | speedup |
|---|---|---|---|
| generated.json `anvil-mdl-rans` | 4.1604 (CV 2.10%) | 3.0766 (CV 5.18%) | **1.352x** |
| synth-timeseries.bin `anvil-hotop-rlzp-rans` | 1.4829 (CV 1.63%) | 1.3802 (CV 1.39%) | **1.074x** |

Wire anchors 89,589 / 140,898 B identical. Gates on the ALLOC-only build
(`build-i9-arch/anvil.exe` 7AD8E932): roundtrip 442/442, fuzz 50 PASS, encode
identity 494/494.

### Crossing verdict

Neither MAT nor MAT+ALLOC nor ALLOC-only reaches the required decode multiple:
ALLOC-only is 1.352x vs R'=2.35-2.78x on generated.json mdl, and 1.074x vs the
1.30x staging bar on synth-timeseries. The MATERIALIZATION leg is therefore
**FALSIFIED as a crossing enabler** (the t3 "removable materialization share"
premise is retired, implementation-era). ALLOC-only is offered as a measured,
wire-identical, commit-ready **partial decode win** (mode-10 rows ~1.35x,
mode-15 ~1.07x); it does not flip any frontier cell.

## Labels

Everything above is measured except where marked pending; no crossing claim is
made here. MAT/ALLOC are decoder-only; the encoder wire and the decoded bytes are
gated identical.
