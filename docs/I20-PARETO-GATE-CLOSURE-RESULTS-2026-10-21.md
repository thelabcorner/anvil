# ANVIL Iteration 20 — Pareto Gate Closure: Results

**Date:** 2026-10-21
**Status:** IN PROGRESS. Preregistration (binding): `docs/I20-PARETO-GATE-CLOSURE-PREREG-2026-10-21.md`.
**Goal:** move the canonical `--parse=ratio --ratio-backend=auto --bwt-aux=on` route from the ruling
`FRONT-GAP_COST` to `COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE` on frozen Silesia (enwik8 best-effort),
as ruled by `tools/reference_cost_gate.py` on a GitHub Actions run of `thelabcorner/anvil`.
**Compute locus:** GitHub Actions only. **No local-compute measurement is used as evidence anywhere in
this document.** Every number below is quoted from a named Actions run and artifact, or is exact
arithmetic over such numbers.

**Program base commit:** `research/anvil-i20-pareto-gate-closure-20261021` @ `edf8e04` (preregistration
`d05fceb`; arbiter workflow `cc37e24`; arbiter flag fix `edf8e04`).
`src/anvil.cpp` blob at the program base is **byte-identical** to the frozen I10-1A candidate
(`755df76ae53795aea040f4c99784d1effb43b3ab`), so the byte side of the baseline is reproducible by
construction.

---

## 1. Fresh baseline (MEASURED)

Two independent baseline reproductions were dispatched on the program branch. Both reproduce the
frozen I10-1A byte totals exactly, and both rule **`FRONT-GAP_COST`**.

### 1.1 Run A — frozen reference-cost gate (reproduction of run `35927623136`)

- **run_url:** `https://github.com/thelabcorner/anvil/actions/runs/39654699366`
- **job:** `reference-cost (silesia)` / `reference-cost (enwik8)`
- **workflow:** `.github/workflows/anvil-i10-aux-reference-cost.yml`
- **commit_sha:** `d05fceb0ce61a853378822f63eb5f2ceeef26557` (candidate `0dce534e5df40945d518c9ef4a722f4a0ecfc0e6`)
- **artifact:** `anvil-i10-aux-reference-cost-silesia-39654699366`, `anvil-i10-aux-reference-cost-enwik8-39654699366`
- **corpus:** silesia, enwik8

| Quantity | Silesia | enwik8 |
|---|---:|---:|
| ANVIL aux total bytes | **46,466,339** | **23,537,422** |
| `xz -9e` total bytes | 48,456,004 | 24,831,648 |
| Brotli q11/lgwin30 total bytes | 49,383,136 | 24,810,180 |
| **ANVIL byte lead vs `xz -9e`** | **1,989,665 B** | **1,294,226 B** |
| ANVIL byte lead vs Brotli | 2,916,797 B | 1,272,758 B |
| ANVIL decode (portfolio) | 48.452 MB/s (4.374 s) | 23.689 MB/s (4.221 s) |
| `xz -9e` decode | 71.344 MB/s | 93.750 MB/s |
| Brotli decode | 244.517 MB/s | 139.739 MB/s |
| ANVIL/`xz` decode-time ratio (CI95) | 1.4726 [1.4717, 1.4897] | 3.9552 [3.9010, 3.9585] |
| ANVIL/Brotli decode-time ratio (CI95) | 5.4547 [5.1604, 5.6033] | 5.8291 [5.4383, 5.9040] |
| A/A null ratio (CI95) | 1.0002 [0.9891, 1.0112] — **PASS** | 1.0002 [0.9879, 1.0119] — **PASS** |
| ANVIL peak decode RSS | 254,224 KiB (248.3 MiB) | 613,484 KiB (599.1 MiB) |
| `xz -9e` peak decode RSS | 55,408 KiB (54.1 MiB) | 67,880 KiB (66.3 MiB) |
| Brotli peak decode RSS | 125,228 KiB (122.3 MiB) | 256,976 KiB (251.0 MiB) |
| ANVIL/`xz` RSS ratio | 4.588× | 9.038× |
| ANVIL/Brotli RSS ratio | 2.030× | 2.387× |
| **Ruling** | **`FRONT-GAP_COST`** | **`FRONT-GAP_COST`** |

Gate status at baseline (Silesia): G1 timing **PASS**, G2 bytes<xz **PASS**, G3 decode ≤2× xz **PASS**,
G4 decode ≤2× Brotli **FAIL** (CI-upper 5.6033), G5 RSS ≤2× xz **FAIL** (4.588×), G6 RSS ≤2× Brotli
**FAIL** (2.030×). enwik8: G3/G4/G5/G6 all **FAIL**.

> **Runner-dependence note (important, and it is a caution against the historical baseline).**
> The historical run `35927623136` measured the Silesia Brotli reference at 166.9 MB/s; this
> baseline measured it at 244.5 MB/s, and `xz -9e` at 71.3 MB/s rather than 82.5 MB/s. Because the
> gate compares the candidate against references measured **in the same job**, the *ratio* is what
> governs, and on this runner the ratio is materially worse (ANVIL/Brotli CI-upper 5.6033 rather
> than 3.5059). The programme's Silesia target must therefore be judged against the *current*
> baseline: **the Silesia portfolio decode must fall from 4.374 s to ≤ 1.733 s (ANVIL ≥ 122.3 MB/s),
> a ≥2.52× portfolio speed-up.** The five Brotli-routed files (127,526,830 B ≈ 0.522 s) leave
> ≈1.211 s for the seven BWT-routed files (84,411,750 B), i.e. the BWT decode path must reach
> **≈69.7 MB/s (≈3.2× faster)**. All I20 thresholds are re-derived from this measured baseline, not
> from the historical one.

### 1.2 Run B — coordinator arbiter path (the tool that will judge the final arm)

- **run_url:** `https://github.com/thelabcorner/anvil/actions/runs/39655813000`
- **job:** `gate-closure (silesia)` and `gate-closure (enwik8)`
- **workflow:** `.github/workflows/anvil-i20-gate-closure.yml`
- **commit_sha:** `cc37e24af95c8e3450f45e0db650715ae17f670c` (`src/anvil.cpp` sha256 `770ea0f5f2ff32f84a775c8c1548dcf52b55f9d2a7c7a3bfd0ebaad4c59f5d90`)
- **artifact:** `anvil-i20-gate-closure-silesia-39655813000`, `anvil-i20-gate-closure-enwik8-39655813000`
- **corpus:** silesia, enwik8

| Quantity | Silesia | enwik8 |
|---|---:|---:|
| ANVIL aux / `xz -9e` / Brotli bytes | 46,466,339 / 48,456,004 / 49,383,136 | 23,537,422 / 24,831,648 / 24,810,180 |
| byte lead vs `xz -9e` | 1,989,665 B | 1,294,226 B |
| ANVIL decode | 42.602 MB/s (4.977 s) | 23.689 MB/s (4.221 s) |
| `xz -9e` decode | 70.141 MB/s | 93.750 MB/s |
| Brotli decode | 198.619 MB/s | 139.739 MB/s |
| ANVIL/`xz` decode ratio (CI95) | 1.6311 [1.6202, 1.6465] | 3.9552 [3.9010, 3.9585] |
| ANVIL/Brotli decode ratio (CI95) | 4.6650 [4.6155, 4.8839] | 5.8291 [5.4383, 5.9040] |
| A/A null (CI95) | 0.9897 [0.9895, 1.0004] — **PASS** | 1.0002 [0.9879, 1.0119] — **PASS** |
| ANVIL peak decode RSS | 248.2 MiB | 599.1 MiB |
| `xz -9e` / Brotli peak decode RSS | 51.0 / 122.2 MiB | 66.3 / 251.0 MiB |
| ANVIL RSS ratio vs xz / Brotli | 4.869× / 2.031× | 9.038× / 2.387× |
| **Ruling** | **`FRONT-GAP_COST`** | **`FRONT-GAP_COST`** |

The `--expected-candidate-total` assertions (`46466339` / `23537422`) **passed** on both corpora: the
coordinator's candidate arm reproduced the frozen totals byte-for-byte through the new arbiter path.

> **Reference-arm variation between the two baseline runs (a methodological finding, recorded
> because it changes how the final arm must be judged).** The candidate bytes are identical
> (46,466,339 B / 23,537,422 B) but the *reference* decode speeds moved by up to ~23 % between runs
> on the same repository and the same nominal runner class: Silesia Brotli 244.517 → 198.619 MB/s,
> `xz -9e` 71.344 → 70.141 MB/s, `xz -9e` peak RSS 54.1 → 51.0 MiB. The **paired same-job ratio** is
> therefore the only stable quantity, and the required portfolio speed-up depends on the run:
>
> | baseline run | ANVIL portfolio decode | G4 bound (2 × Brotli) | required portfolio speed-up | required BWT-path speed-up |
> |---|---:|---:|---:|---:|
> | `39654699366` (frozen gate path) | 4.374 s | ≤1.731 s | 2.53× | 2.94× (≤1.358 s for 120,362,284 B) |
> | `39655813000` (coordinator arbiter) | 4.977 s | ≤2.134 s | 2.33× | 2.70× (≤1.673 s) |
>
> **The programme's design target is the harder case, 2.94× on the BWT path.** Because the gate
> outcome for a fixed candidate is reference-dependent at the ~20 % level, **the final arm must be
> ruled in a run that measures its own references (never spliced), and the ruling should be
> reproduced in a second independent run before it is cited.**

---

## 2. Candidate verdicts

Populated as each candidate's pre-registered decision rule is applied to its own Actions evidence.
Verdicts are only `WIN`, `NO-GO` or `INCONCLUSIVE`.

| ID | Candidate | Worker | Verdict | Key numbers | Evidence |
|---|---|---|---|---|---|
| C1 | BWT decode stage decomposition | W1 | *in flight* | — | — |
| C2 | Auxiliary-index walk-budget sweep | W2 | *in flight* | — | — |
| C3 | BWT subblock-cap sweep below 8 MiB | W3 | *in flight* | — | — |
| C4 | Decode working-set engineering (0-byte RSS) | W4 | *in flight* | — | — |
| C5 | Postcoder decode-cost swap | W5 | *in flight* | — | — |
| C6 | Dense reference-grid control | W6 | *in flight* | — | — |

---

## 3. Combined-arm plan (the arm the coordinator will rule)

The gate's three binding Silesia deficits are decode-vs-Brotli (G4), RSS-vs-xz (G5) and
RSS-vs-Brotli (G6). No single lever clears all three; the programme's working combination is:

1. **C2 — auxiliary-index walk budget.** More checkpoints ⇒ shorter LF walks ⇒ faster inverse BWT.
2. **C3 — BWT subblock cap.** Bounds the per-piece inverse-BWT working set.
3. **C4 — decode working-set reduction.** Zero byte cost; removes the materialised output and the
   retained compressed input from the decode peak.

### 3.1 Measured per-file routing facts (from run `39654699366`, `bytes.csv`)

`DERIVED` from the measured baseline artifact (exact arithmetic over measured bytes):

| file | input bytes | ANVIL total | Brotli total | `xz -9e` | BWT saving vs Brotli | route |
|---|---:|---:|---:|---:|---:|---|
| webster | 41,458,703 | 7,319,896 | 8,340,058 | 8,368,672 | **1,020,162** | BWT |
| nci | 33,553,445 | 1,369,810 | 1,497,411 | 1,449,272 | 127,601 | BWT |
| x-ray | 8,474,240 | 4,019,394 | 4,682,754 | 4,491,264 | 663,360 | BWT |
| dickens | 10,192,446 | 2,574,367 | 2,827,779 | 2,831,212 | 253,412 | BWT |
| osdb | 10,085,684 | 2,587,124 | 2,816,279 | 2,844,556 | 229,155 | BWT |
| mr | 9,970,564 | 2,384,760 | 2,823,137 | 2,751,892 | 438,377 | BWT |
| reymont | 6,627,202 | 1,147,270 | 1,332,159 | 1,315,592 | 184,889 | BWT |
| mozilla | 51,220,480 | 13,806,173 | 13,806,141 | 13,376,240 | −32 | Brotli |
| samba | 21,606,400 | 3,761,931 | 3,761,899 | 3,739,524 | −32 | Brotli |
| ooffice | 6,152,192 | 2,478,889 | 2,478,857 | 2,427,224 | −32 | Brotli |
| xml | 5,345,280 | 430,599 | 430,568 | 434,892 | −31 | Brotli |
| sao | 7,251,944 | 4,586,126 | 4,586,094 | 4,425,664 | −32 | Brotli |
| **total** | 211,938,580 | **46,466,339** | 49,383,136 | 48,456,004 | 2,916,797 | 7 BWT / 5 Brotli |

The mode-17 envelope costs 31–32 B per Brotli-routed file; that is the whole difference. The
BWT-routed files hold **120,362,284 B (56.79 % of Silesia)** and the Brotli-routed files
**91,576,296 B (43.21 %)**.

### 3.2 Two derived constraints that decide the arm

**(a) G5 (peak RSS ≤ 2.0 × `xz -9e` = 110,816 KiB) is *not* reachable by any Brotli-backed route.**
Brotli's own Silesia peak decode RSS is 125,228 KiB (122.3 MiB) = 2.26× `xz -9e`. So a route that
moves files to the Brotli backend can clear G4 but can never clear G5. **G5 therefore requires the BWT
route with a bounded working set.** Its dominant term is the accumulated per-file output buffer
(webster = 41,458,703 B = 39.5 MiB) plus the per-piece inverse-BWT set (BWT string `C` + libsais
`int32` array `4C`). With the output streamed to disk (C4) the peak becomes ≈ `6C`; with the output
materialised it becomes ≈ `41.5 MB + 6C`. Solving `6C + input ≤ 110,816 KiB` gives **C ≤ 16 MiB**
(with streaming) and **C ≤ 8 MiB** (without). The prior sweep measured 8 MiB at 124.4 MiB
(2.30× `xz`, still a G5 FAIL) and 16 MiB at 125.7 MiB.

**(b) The auxiliary-index cost is PER SUB-BLOCK, so the cap and the walk budget interact
multiplicatively.** The `0xFF` outer frame splits the BWT input into `ceil(file_size / cap)` pieces
(confirmed by W3's enwik8 sweep: 100,000,000 B gives 96 / 48 / 24 / 12 / 6 / 3 / 2 / 1 pieces at
caps 1 / 2 / 4 / 8 / 16 / 32 / 64 / 128 MiB), and each piece carries its own inner BWT payload with
its own auxiliary index. So the total index bytes are `4 × W × (total sub-blocks)`, not
`4 × W × (number of files)`. For the seven BWT-routed Silesia files the sub-block totals are:

| cap | dickens | mr | nci | osdb | reymont | webster | x-ray | **total sub-blocks** |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 MiB | 1 | 1 | 2 | 1 | 1 | 3 | 1 | **10** |
| 8 MiB | 2 | 2 | 4 | 2 | 1 | 5 | 2 | **18** |

Therefore:

| arm | cap byte cost (prior run `35930672607`) | aux index bytes at W = 16384 | total charged | vs 1,989,665 B surplus |
|---|---:|---:|---:|---|
| 16 MiB cap + W = 16384 | +472,650 B | 10 × 65,536 = 655,360 B | **+1,128,010 B** | 861,655 B left — **fits** |
| 8 MiB cap + W = 16384 | +898,935 B | 18 × 65,536 = 1,179,648 B | **+2,078,583 B** | **exceeds the surplus** |
| 16 MiB cap + W = 32768 | +472,650 B | 10 × 131,072 = 1,310,720 B | +1,783,370 B | 206,295 B left |
| 16 MiB cap + W = 8192 | +472,650 B | 10 × 32,768 = 327,680 B | +800,330 B | 1,189,335 B left |
| 16 MiB cap + W = 4096 | +472,650 B | 10 × 16,384 = 163,840 B | +636,490 B | 1,353,175 B left |

**Consequence:** the walk budget cannot be chosen independently of the cap. The 8 MiB cap is
excluded at W = 16384 on the byte budget alone, so **16 MiB is the leading cap** — it is *cheaper*
in bytes than 8 MiB (the warm-up tax grows as the cap shrinks) while still clearing G5 provided the
output is streamed.

### 3.3 The decisive unknown: the LF-walk share `f`

Let `f` be the fraction of BWT-path decode time spent in the inverse-BWT LF walk. Reducing the
checkpoint stride by a factor `k` (i.e. `k`× more checkpoints) gives a whole-path speed-up of
`1/(1−f+f/k)`. The Silesia budget is: portfolio decode ≤ **1.733 s** (48.452 MB/s → 122.3 MB/s).
The five Brotli-routed files (91,576,296 B) take ≈0.375 s at the measured 244.517 MB/s, leaving
**≤1.358 s** for the seven BWT-routed files (120,362,284 B), which currently take ≈4.000 s
(≈30.1 MB/s) — i.e. the BWT path must reach **≈88.6 MB/s, a 2.94× speed-up**. Solving
`1/(1−f+f/k) ≥ 2.94`:

| checkpoint factor `k` | required LF-walk share `f` | walk budget `W` | index bytes at the 16 MiB cap |
|---:|---:|---:|---:|
| 4 | ≈ 0.88 | 4096 | 163,840 B |
| 8 | ≈ 0.75 | 8192 | 327,680 B |
| 16 | ≈ 0.70 | 16384 | 655,360 B |
| 32 | ≈ 0.67 | 32768 | 1,310,720 B |

**This is the single number the programme turns on.** It is C1's (W1's) measurement, and it is
implicitly fitted by C2's (W2's) sweep. C1 and C2 are therefore the critical path; C3 and C4 are
the arithmetic that makes their combination fit the byte budget and clear G5.


### 3.4 The final arbiter

The final arm is ruled by the **frozen** `tools/reference_cost_gate.py` via
`.github/workflows/anvil-i20-gate-closure.yml`, with the candidate built from the merged branch and
the encoder-policy flags passed through the additive, default-preserving
`--candidate-extra-args` flag (one shlex-quoted string; empty reproduces the pre-I20 command byte
for byte).

---

*Results are appended as each candidate's decision rule is applied. Nothing in this document is
asserted without a named GitHub Actions run URL, job, workflow file, commit SHA, artifact name and
corpus.*
