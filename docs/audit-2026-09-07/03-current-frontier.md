# 03 — Current Ratio Frontier and Headroom

## 1. Canonical baseline

All numbers in this section are exact compressed byte counts. Timing is discussed separately because of the compiler confound.

### Silesia

Canonical total: **211,938,580 bytes**.

| file | input | Brotli q11/lw30 | xz -9e | zstd u22/l27 | old ANVIL ratio |
|---|---:|---:|---:|---:|---:|
| dickens | 10,192,446 | 2,827,779 | 2,831,220 | 2,849,381 | 2,827,810 |
| mozilla | 51,220,480 | 13,806,141 | 13,376,248 | 14,967,572 | 13,806,172 |
| mr | 9,970,564 | 2,823,137 | **2,751,900** | 3,105,643 | 2,768,634 |
| nci | 33,553,445 | 1,497,411 | **1,449,280** | 1,610,427 | 1,497,441 |
| ooffice | 6,152,192 | 2,478,857 | **2,427,232** | 2,598,777 | 2,478,888 |
| osdb | 10,085,684 | **2,816,279** | 2,844,564 | 3,098,444 | 2,816,310 |
| reymont | 6,627,202 | 1,332,159 | **1,315,600** | 1,347,556 | 1,332,189 |
| samba | 21,606,400 | 3,761,899 | **3,739,532** | 3,876,634 | 3,761,930 |
| sao | 7,251,944 | 4,586,094 | **4,425,672** | 5,000,515 | 4,586,125 |
| webster | 41,458,703 | **8,340,058** | 8,368,680 | 8,458,469 | 8,340,089 |
| x-ray | 8,474,240 | 4,682,754 | **4,491,272** | 5,155,752 | 4,682,785 |
| xml | 5,345,280 | **430,568** | 434,900 | 453,173 | 430,598 |
| **total** | **211,938,580** | **49,383,136** | **48,456,100** | **52,522,343** | **49,328,971** |

Interpretation:

- xz is the strongest of the three local reference commands on Silesia aggregate;
- ANVIL’s pre-BWT ratio path beats naked Brotli by only 54,165 bytes;
- that aggregate gain is almost entirely `mr`;
- the only third-party transform win still does not beat xz on its winning file.

### enwik8

| codec | bytes |
|---|---:|
| Brotli q11/lw30 | **24,810,180** |
| ANVIL ratio | 24,810,211 |
| xz -9e | 24,831,656 |
| zstd ultra22 long27 | 25,333,695 |

ANVIL transforms do not help enwik8.

## 2. Why the fresh Brotli baseline matters

Published historical Brotli tables often use smaller windows / older versions. ANVIL’s local q11/lw30 baseline is materially stronger than those historical rows. Historical tables are useful landscape references, not current head-to-head baselines.

This should remain a general rule: **published corpus numbers orient the frontier; local fresh builds decide direct claims.**

## 3. `mr`: what the only old transform win actually is

The preserved pre-registry archive was inspected. `mr` selected:

- transform ID 1: `ctx1-256`;
- not the line-column transform.

The result is interesting but should not be over-read. A first-order entropy probe showed `x-ray`, `sao`, and `osdb` have larger raw next-byte conditional-entropy reductions than `mr`, yet Brotli rejected ctx1 on those files.

Therefore the useful feature is not simply “high first-order predictability.” The transform likely changes **where similar conditional symbols live relative to the backend’s window/model**, i.e. it changes locality and model accessibility.

Research implication: transform selection should be trained/evaluated against **backend-relative realized gain**, not a single entropy statistic.

## 4. Direct BWT results

Current backend 2 uses `libsais` for the BWT primitive and ANVIL-owned postcoding. Transforms were disabled for this table.

| file | BWT direct | Brotli | BWT − Brotli | winner |
|---|---:|---:|---:|---|
| dickens | **2,571,873** | 2,827,779 | **−255,906** | BWT |
| mozilla | 17,823,651 | **13,806,141** | +4,017,510 | Brotli |
| mr | **2,382,322** | 2,823,137 | **−440,815** | BWT |
| nci | **1,365,712** | 1,497,411 | **−131,699** | BWT |
| ooffice | 2,868,832 | **2,478,857** | +389,975 | Brotli |
| osdb | **2,584,657** | 2,816,279 | **−231,622** | BWT |
| reymont | **1,144,032** | 1,332,159 | **−188,127** | BWT |
| samba | 4,398,658 | **3,761,899** | +636,759 | Brotli |
| sao | 5,117,977 | **4,586,094** | +531,883 | Brotli |
| webster | **7,317,361** | 8,340,058 | **−1,022,697** | BWT |
| x-ray | **4,017,320** | 4,682,754 | **−665,434** | BWT |
| xml | 437,790 | **430,568** | +7,222 | Brotli |
| **total if forced BWT** | **52,030,185** | **49,383,136** | **+2,647,049** | Brotli |

BWT wins 7/12 but forced whole-corpus BWT loses because the five hostile files are expensive, especially `mozilla`.

### 4.1 BWT also beats the *xz* bar on every winning file (verified, landscape-positive)

Independent recomputation (`findings/oracle-verified-3way`, lead) shows BWT direct
is smaller than **xz -9e** on all 7 of its wins, not merely smaller than a
handicapped Brotli. Per-file BWT vs xz -9e (both measured, exact):

```
webster  −1,051,319   x-ray  −473,952   mr     −369,578
osdb     −259,907     dickens −259,347  reymont −171,568
nci      −83,568
```

On the 5 BWT-hostile files, xz beats Brotli on 4 of 5 (mozilla, ooffice, samba,
sao); Brotli wins only xml (430,568 vs 434,900). So the BWT advantage is a *real
compressor property*, and the residual headroom to the landscape bar is
concentrated in the archive-heavy files — see `10-specialist-disagreement.md` §3.

### 4.2 Decode-cost caveat (this is a cost axis, not a ratio axis)

BWT direct decodes far slower than Brotli/xz. Straight per-cell decode sums from
the CSVs over the **7 BWT-routed files** (dickens, mr, nci, osdb, reymont, webster,
x-ray; 120,362,284 B input — mixed build, see 01 §4 confound, directional only):

| codec | decode sum | MB/s |
|---|---:|---:|
| ANVIL BWT | **9.248 s** | **13.0** |
| xz -9e | **1.529 s** | **78.7** |
| Brotli q11/lw30 | **0.823 s** | **146.3** |

BWT decode is **11.2× slower than Brotli and 6.0× slower than xz**. (Earlier drafts
of this section quoted ~17× and ~9.35 / 2.95 / 1.55 s; those were arithmetic errors
— corrected 2026-09-07 by recomputing directly from the CSVs. The 6× vs xz gap is
too large to be compiler noise, so the conclusion is unchanged.) A routed portfolio
that wins bytes is therefore **decode-dominated**; see
`11-front-crossing-criteria.md` §2–§4 for the multi-axis classification (any
bytes-only win is FRONT-GAP on decode until normalized).

**The regression is structural, not routing-amortizable** (`findings/decode-amortization-deadend`):
the BWT-favorable files are **56.79%** of Silesia, so region-scoping BWT cannot shrink
the slow-decode fraction enough — matching xz decode would require f ≤ 8.38% BWT-routed
and forfeit ~6.8× of BWT bytes (≈ the whole 2,009,264 B win). Byte-win and decode-win are
in **~7× direct conflict**. The real fixes are (b) a faster BWT *inverse* (engineering) or
(c) byte wins from a fast-decode backend — which is why DEFLATE reconstruction (P4.1)
now outranks CM (P3.1) in the portfolio (07).

## 5. The routing opportunity — labelled oracle vs landscape

Three totals below are all **derived** from measured cells; none is a measured
ANVIL result.

- **`oracle_min(Brotli,BWT)` = 46,446,836 B** — the **achievable ANVIL target**
  once `--ratio-backend=auto` is measured, before adding current rev-2 framing.
  = −2,009,264 B vs xz -9e (exact: 48,456,100 − 46,446,836 = 2,009,264 = 1.916 MiB).
- **`oracle_min(Brotli,BWT,xz)` = 45,782,529 B** — **LANDSCAPE ONLY** (xz is not an
  ANVIL backend). = −2,673,571 B vs xz. Not a target, not a result.
- **Gap = 664,307 B**, entirely inside mozilla/samba/ooffice/sao — the quantitative
  case for DEFLATE reconstruction (P4.1) and CM (P3.1).

Reference points:
- ~2.94 MB below naked Brotli (49,383,136);
- 1.916 MiB below xz -9e (48,456,100);
- ~0.28 MB below the published bsc 46,723,436-byte Silesia total (landscape only).

The 46,446,836 figure is a research target, not a result. The actual
`--ratio-backend=auto` run (bench-normalize, E2) must prove selection + framing +
roundtrip end-to-end and be judged against the xz bar on **bytes** *and* within
decode/peakmem margin (11 §4.2). Until then the portfolio is a hypothesis.

## 6. Why mixed-file/block routing may be larger than file routing

Whole-file BWT is a blunt experiment. `mozilla`, `samba`, and `ooffice` are mixed containers/archives with heterogeneous regions. It is entirely plausible that a file contains:

- plain text regions favorable to BWT;
- already-compressed ZIP/JAR/DEFLATE regions favorable to opaque storage or reconstruction;
- executable/binary regions favorable to Brotli/xz-like LZ;
- repetitive structural regions favorable to another backend.

The correct question is therefore not only “which backend wins this file?” but:

> **At what segmentation scale does backend identity become stable enough to exploit without paying excessive framing/warmup cost?**

Candidate scales: 256 KiB, 1 MiB, 4 MiB, 16 MiB, whole file.

Measure the oracle first. Do not build a router until oracle gain exceeds framing/model cost by a comfortable margin.

## 7. BWT postcoder evidence

Spot checks show the internal BWT postcoder is content-dependent:

- `mr`: adaptive order-0 MTF/RLE path;
- `reymont`: adaptive order-1 path;
- `xml`: adaptive order-1 path;
- small `doc.md`: order-0, but still worse than Brotli due warmup/header economics.

This supports internal postcoder routing rather than one fixed model.

The obvious next control is QLFC/local-frequency coding. libbsc itself exposes QLFC static/adaptive/fast modes and defaults to BWT + QLFC static, with optional LZP. ANVIL should treat these as prior-art targets and ablation references, not copy them blindly.

## 8. External landscape / headroom

Published figures show substantial remaining headroom beyond BWT/Brotli:

- bsc class: around mid-46 MB Silesia, ~20.8 MB enwik8;
- lpaq1 class: ~43.0 MB Silesia;
- stronger context mixers and PAQ descendants go much lower at much higher time/RAM cost;
- deflate-reconstruction + strong backend materially improves `mozilla`/`samba` in published precomp/cmix results.

This means ANVIL currently has at least three distinct headroom pools:

1. **backend routing / BWT postcoding**: low-to-medium implementation cost;
2. **container reconstruction**: concentrated gains on archive-heavy files;
3. **context mixing**: large general ratio headroom, high implementation/runtime cost.

The project should attack them in that order unless new measurements overturn the expected-value calculation.

## 9. Program-size tax

Current MSVC `build/anvil.exe` with Brotli + libsais is:

- 1,325,568 bytes on disk;
- ~608,635 bytes as a ZIP-deflated executable in the audit measurement.

This is not yet a fair “decompressor size” because the executable also contains encoder paths. But it is a warning: a multi-backend container can quietly win data bytes while losing benchmark-accounted program bytes.

Required follow-up:

- build a decoder-only target;
- dead-strip forward BWT / encoder / benchmark code;
- report compressed data **and** zipped decoder size for LTCB-style comparisons.

