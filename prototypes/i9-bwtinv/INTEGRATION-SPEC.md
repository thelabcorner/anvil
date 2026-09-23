# i9-bwtinv — integration spec: indexed inverse BWT (libsais aux) for ANVIL backend 2

Status: PROTOTYPE MEASURED, NOT INTEGRATED. Owner: `bwtinv`. Date: 2026-09-12.
Evidence: `prototypes/i9-bwtinv/` (harness + raw CSV + attestation window). `src/anvil.cpp` untouched.
This file is the handoff to `arch`; `format` must register the wire change.

## 0. Bottom line

Replace the shipped `libsais_unbwt` call (`src/anvil.cpp:4144`) with
`libsais_unbwt_aux` + an encoder-supplied sample index `I`:

- **1-thread speedup 3.3–3.5x** (same-core-count, citation axis): dickens 20.64 → 70.18 MB/s,
  webster 18.36 → 64.15, enwik8 17.00 → 60.33 (reps=7, degraded load window; CV 4.8–13.3%).
- **16-thread speedup 27–29x** (context-only; `THREAD-IMBALANCE: prototype 16t vs 1t baseline`):
  dickens 604.2, webster 502.7, enwik8 448.7 MB/s.
- Wire cost: `I` = `1+(n-1)/r` int32 = **40 B…8 KB per whole-file block** depending on `r`
  (at the measured knee ≈2–5 KB; ≈0.02–0.05% of a 10 MB block; **exact 19,328 B total at the
  ~1024-block policy across the 7-file Silesia portfolio** — see §4.2).
- Output byte-identical to the shipped path (BWT bytes identical; decode == raw input, all variants).
- **End-to-end projection (DERIVED; decode-perf split + my k)**: 1t **1.56–1.91x**, 16t **1.95–2.69x**
  BWT decode. Postcoder becomes the floor (see §5) — unbwt alone cannot reach 40+ MB/s on
  dickens/webster.

## 1. Current shipped callsite (observed on worktree source 62BC6631, HEAD fc23d9a)

> Provenance note: the worktree source has since moved to **38409E26** (ALLOC-only
de> materialization, wire-identical). Callsites below should be re-located **by symbol** —
> encode/decode call shape and wire layout are unchanged; only decode allocation/copy code
> moved. Re-verify line numbers before editing.

- Encode (`bwt_backend_encode`): `libsais_bwt(in,bwt,tmp,n,0,nullptr)` then postcoder selection;
  payload = `[post u8][primary uvar][postcoder bytes]`.
- Decode (`bwt_backend_decode`): allocates `out(expected)`, `tmp(expected+1)` and calls
  `libsais_unbwt(bwt,out,tmp,(int32_t)expected,nullptr,primary)`.
- libsais 2.10.4, upstream unmodified. `arch`'s P0 fix: primary is now validated
  `1<=primary<=n` and passed as-is (primary==n is legitimate; `unbwt(...,n)` is exact,
  `unbwt(...,1)` reconstructs the wrong string). With aux, `I[0] == primary` (verified).

## 2. Mechanism

`libsais_unbwt` = `libsais_unbwt_aux(r=n, I={primary})`: ONE LF-walk of `n/2` serial
dependent steps (`decode_1`, `libsais.c:7714`). With `r<n`, the output splits into
`blocks = 1+(n-1)/r` independent walks seeded by `I[t] = ISA[t*r]+1`; the decoder runs
8 walks interleaved per batch (`decode_8`, `libsais.c:7882-7941`), hiding DRAM latency;
`_omp` spreads block batches over threads (`libsais.c:7943`).
`I` is not derivable at decode without rebuilding a suffix array, so it is transmitted
(encoder: `libsais_bwt_aux`, `libsais.c:7123-7146`).

Falsified: `_omp`/`libsais_unbwt_omp` with **`r=n`** parallelises init only — measured
17.4–23.2 MB/s at t=1/4/8/16 (no walk gain). Do not ship that form as a decode fix.

## 3. Measured (degraded window `w-bwtinv-20260912T064334Z`, reps=7, raw `measure/w-bwtinv-*`)

Baseline rows are the exact ANVIL call shape (fresh `out`+`tmp` per call, 1 thread), shipped
non-OMP libsais build. Exe sha256: ref `E47D0A51…`, omp `FC345062…`. Host load during window:
median 36.3% (max 90.9%) — `measured (parallel; attestation attached)`, ranking-only.

| file (n) | plain_fresh (1t) | aux best (1t) | k(1t) | aux_omp best (16t) | k(16t) |
|---|---:|---:|---:|---:|---:|
| dickens 10,192,446 | 20.64 MB/s | 70.18 (r=262144) | 3.40x | 604.18 (r=32768) | 29.3x |
| webster 41,458,703 | 18.36 MB/s | 64.15 (r=32768) | 3.49x | 502.69 (r=131072) | 27.4x |
| enwik8 100,000,000 | 17.00 MB/s | 60.33 (r=524288) | 3.55x | 448.67 (r=1048576) | 28.9x |

`I` bytes at best-1t r: dickens 156 (r=262144) / 624 (r=65536) / 4980 (r=8192);
webster 1268 (r=131072) / 5064 (r=32768); enwik8 1528 (r=262144) / 384 (r=1048576).
`r=n, t=16` (init-only): 23.15 / 21.33 / 17.70 MB/s — no gain.

Same-core-count compare (1t vs 1t): label `measured`, pending a quiet-window rerun for
citation-grade sign-off. All `_omp` rows: **THREAD-IMBALANCE: prototype 16t vs 1t baseline**,
context-only per PR-4 §2.

## 4. Proposed integration

### 4.1 Encode

Replace `libsais_bwt` with `libsais_bwt_aux` (power-of-two `r>=2`), keep `primary = I[0]`
and the existing degenerate handling (n==1 shortcut; no primary remap):

```cpp
int32_t blocks_target = (T > 1) ? (int32_t)(16 * T) : 1024;   // measured knee
int32_t r = 2; while ((int64_t)r * blocks_target < (int64_t)n) r <<= 1;   // r = pow2 >= n/blocks_target
size_t  icount = 1 + (size_t)(n - 1) / r;                     // == blocks
std::vector<int32_t> aux(icount);
int32_t rc = libsais_bwt_aux(in.data(), bwt.data(), tmp.data(), n, 0, nullptr, r, aux.data());
if (rc != 0) throw std::runtime_error("libsais BWT-aux failed");   // rc==0 on success
int32_t primary = aux[0];                                           // I[0] is the primary
```

All postcoder candidates operate on the same BWT bytes; `bwt_mtf_rle`/selection unchanged.

### 4.2 Wire (format decides; two options)

Current payload: `[post u8][primary uvar][postcoder bytes]`.

- **Option A (recommended): new BWT backend payload version.**
  `[post u8][aux_version u8=2][primary uvar][r uvar][icount uvar][I: icount x uint32 LE][postcoder bytes]`.
  Validate on decode: `r` power of two or `r==n`; `icount == 1+(n-1)/r`; every `I[t]` in
  `1..n`; total length check before allocating.
- **Option B: new backend id** (e.g. `3 = BWT-aux`) with the same payload layout; router must
  treat it as byte-identical-except-index.
- `I` is raw int32 (4 B/entry). Varint deltas would save ~1–1.5 B/entry; not worth the code.
  At the ~1024-block policy: ~4 KB per 10 MB block. `bench` must charge these bytes.
  **Exact at `r` = smallest pow2 with `r*1024 >= n`:** dickens 2492 B, mr 2436, nci 4096,
  osdb 2464, reymont 3236, webster 2532, x-ray 2072 B → **Silesia 7-file total 19,328 B**
  (0.0083% of 211,938,580; ≈0.96% of the 2,009,105-B xz win margin); **enwik8 3052 B**.

### 4.3 Decode

```cpp
if (aux_present) {
  if (T > 1) libsais_unbwt_aux_omp(bwt, out, tmp, expected, nullptr, r, I, T);
  else       libsais_unbwt_aux    (bwt, out, tmp, expected, nullptr, r, I);
} else {
  libsais_unbwt(bwt, out, tmp, expected, nullptr, primary);   // v1 path unchanged
}
```

`tmp` is already `expected+1` int32 (same requirement as `libsais.h:308`). No other change.

### 4.4 Build / threads

`libsais_unbwt_aux_omp` needs `-DLIBSAIS_OPENMP` + `libomp` (clang-cl: `/openmp`,
link `C:/Program Files/LLVM/lib/libomp.lib`). Keep the non-OMP object for the v1 path if
preferred. Threads: explicit `T` (`0` = library default). Memory at T threads:
libsais allocates `T*(256+65536)*4 B` ≈ **4.2 MB at T=16**, plus `P` (4n), `bucket2`
(256 KB), `fastbits` (~2n/shift B). Per PR-4 §2 every published decode row carries its
thread count; MT rows are context-only vs 1t references.
Note: arch's `--decode-threads=N` is **block-level** (independent blocks) — orthogonal and
composable; it is not this.

## 5. Expected end-to-end decode (DERIVED; decode-perf splits + §3 k)

`decode-perf` stage splits on worktree source 62BC6631 (containers byte-identical to canonical;
`deliverable/pr1-profile`):

- v1 window: dickens **10.0 MB/s** — postcoder 40.1%, unbwt 63.3%, alloc 0.6%, crc 0.2%;
  webster **12.5 MB/s** — postcoder 35.2%, unbwt 69.4%.
- v2 re-validated: dickens **11.2 MB/s** — postcoder 43.9%, unbwt 50.2%, alloc 0.6%, residual 5.0%;
  webster **12.5 MB/s** — postcoder 34.2%, unbwt 61.0%, alloc 0.7%, residual 3.8%.
- cross-run range: postcoder 34–44%, unbwt 50–69%.

`T_new = T_old * ((1-s_u) + s_u/k)`, k from §3:

| file | 1t aux | 16t aux | postcoder-only floor |
|---|---:|---:|---:|
| dickens (v1 / v2) | 1.71x → 17.1 / **1.56x → 17.4 MB/s** | 2.37x → 23.7 / **1.95x → 21.9** | 24.9 / **22.6 MB/s** |
| webster (v1 / v2) | 1.82x → 22.7 / **1.91x → 23.9 MB/s** | 2.65x → 33.1 / **2.69x → 33.7** | 35.5 / **35.8 MB/s** |
| enwik8 (s_u≈0.66 assumed) | ~1.9x | ~2.8x | — |

Portfolio (DERIVED; old 7-file BWT decode 13.0 MB/s, 5 Brotli-routed files 146.3 MB/s):
BWT-only ≈ **21–25 MB/s (1t) / 28–36 (16t)**; whole-portfolio ≈ **34–38 (1t) / 43–54 (16t)**
vs the old 21.4; xz = 78.7.

**Consequence for the 06 §F0.1 reopen bar (BWT decode ≳40 MB/s):** not reachable by unbwt
alone — the postcoder floor is ~22.6–24.9 MB/s (dickens) / ~35.5 (webster), so dickens caps
below 25 MB/s even with free unbwt. **Independent confirmation** (decode-perf, median-7,
canonical containers, 1t): postcoder-only speeds dickens **25.22**, webster **35.87**, enwik8
**28.41 MB/s** — exactly the BWT-only ceilings with a free/aux-hidden unbwt. Required postcoder
speedup to clear 40: **1.66x / 1.13x / 1.46x** (~1.38x portfolio); the cheap edges (MTF,
alloc, buffered renorm C=1.02) measure insufficient (POSTCODER-SPEC.md). The postcoder leg must
move too (decode-perf/arch); decode-perf offers forced-postcoder timings (`--bwt-post=1/3`).
The 1t aux leg is the cheapest honest step: **+56–91% end-to-end**, no threads, ~2–5 KB wire.

### 5.1 Both decode legs are now measured (addendum 2026-09-12)

arch's leg 4 (postcoder buffered renorm + two-level cumulative tables, wire-identical;
lane exe FE0CF4F1, src BDC90474; canonical build 72D65150) measured whole-decode A/B **1.215x dickens / 1.169x webster / 1.179x bytes-weighted
aggregate** (attested; no crossing claim). Those are whole-decode factors; converted to the
**postcoder-stage** factor with decode-perf's v2 shares they are **1.674x / 1.733x / ~1.671x**
(decode-perf pinned this). Combined with the 1t aux unbwt:

- dickens: **2.134x** end-to-end (11.2 → 23.80 MB/s BWT-only)
- webster: **2.379x** (12.5 → 29.65 MB/s)
- enwik8: **2.286x** (→24.55 MB/s, s_u assumed)
- BWT-only portfolio ≈ **25.7 MB/s** (bytes-weighted) vs 13.0 old.

**Requirement to clear the 40 MB/s BWT-only bar** with the aux unbwt in place:
`k_p = s_p / (R_base/40 − s_u/k_u − s_other)` = **6.05x dickens / 3.86x webster / 6.60x enwik8**
(decode-perf, stage-correct; enwik8 s_u assumed). Measured leg-4 stage k_p ≈1.67–1.73x, so the
arithmetic+Fenwick symbol decode still needs **~4–7x** more, not the 1.38x that a free/hidden
unbwt implied. A transmitted-static-table postcoder ID (format change, not yet authorized) is
the apparent route. decode-perf offers a direct postcoder-stage A/B on FE0CF4F1 to replace the
whole-vs-stage conversion; not decision-critical (the >4x conclusion holds either way).

## 6. Obligations / risks

1. Wire bytes are real; charge them (`bench`). ~4 KB per 10 MB block at the knee.
2. Byte-identity: `bwt_aux` BWT == `bwt` and `I[0] == primary`, verified n=64 … 100,000,000
   (probes `probe_aux.cpp` / `probe_file.cpp`); decode == raw for every timed variant.
3. New id/version needs a direct forced encode→decode fuzz test (format's registry rule).
4. Reject malformed `r`/`icount`/`I` before the library call (the library also validates).
5. `r` policy: 1t knee ~512–1300 blocks; MT knee ~16–32 blocks/thread. Never `r=n` with threads.
6. `n==1` shortcut and `primary==n` semantics stay exactly as arch's P0 fix defines them.
7. Provenance: upstream libsais 2.10.4 APIs; no new codec logic, no novelty claim.
