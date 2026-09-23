# 08 — Next Experiments: Exact Queue and Stop Conditions

This file turns the portfolio into executable work. Order matters.

## Status board (2026-09-07, swarm ANVIL-BLOCKSPLIT-EXP)

| exp | owner | status | measured result so far |
|---|---|---|---|
| E0 | format-bwt | **DONE (v4)** | FORMAT.md backend-2 section (postcoders 0-3, bounds, bit-count/padding, memory bound); 24-case BWT adversarial fuzz matrix (all reject); goldens in `tests/bwt-golden/` (tiny/repetitive/random/text, all roundtrip + hash-verified); full fuzz green incl. `forced_postcoders=20` (IDs 1/2/3 byte-exact, IDs 0/4 reject up front). FORMAT.md postcoder table marks 0/4 DISABLED, 1/2/3 supported. Degenerate 1-byte/all-equal round-trip bug fixed (arch-bwt libsais). One real decoder bug (F1) found and fixed. Backend 2 **validated**. |
| E1 | bench-normalize | **DONE** | pinned clang-cl 22.1.8 build; BWT + references share one compiler/flags (confound from 01 §4 removed). Provenance in build/build-info.txt, git fc23d9a. |
| E2 | bench-normalize | **DONE** | `--ratio-backend=auto`, transforms OFF. Silesia = 46,446,995 B (beats xz 48,456,100 by 2,009,105 B; +159 B over oracle = framing). enwik8 = 23,534,368 B (beats xz by 1.30 MB). All roundtrip OK. Verdict: FRONT-GAP (bytes beat bar, decode + peakmem fail). See 12 §2.5. |
| E3 | bench-normalize / arch-bwt | **blocked on E2** | transform × backend matrix not started (E2 now measured; unblocked when convenient). |
| E4 | router-regret | **DONE — FAILED its go-bar (NEGATIVE)** | block-local oracle measured at 4 MiB and 16 MiB on mozilla/samba/ooffice/sao/webster. Block routing is **worse than whole-file on every configuration**: aggregate **+1,653,661 B** (4 MiB) and **+272,636 B** (16 MiB) over 5 files. Go-bar (>250 KiB gain / >529,846 B = 0.25% Silesia) **not met — zero gain**. Cause: per-block **model warmup**, not framing. See §E4 measured result below. |
| E5 | router-regret | **PARKED — CLOSED BY E4 NEGATIVE** | E5 was pre-gated on E4 passing. E4 failed with zero gain at every scale (block oracle strictly worse than whole-file), so no block router is to be built. PARKED, NOT completed; reopen only on a near-zero-warmup backend or cross-block model carryover. |
| E6 | arch-bwt / bwt-theory | **DONE — NO-GO on both (NEGATIVE)** | bwt-theory standalone harness, 13 files: QLFC **+2,437,384 B (+3.23%)**, beats baseline **0/13**; LZP **+279,419 B (+0.37%)**, beats baseline 4/13 but **loses on every headline BWT winner** (webster +102 KB, x-ray +46 KB, nci +255 KB). Independently corroborated in-codec: arch-bwt shipped QLFC as postcoder 4 and bwt-direct output is **byte-identical to the pre-QLFC baseline on all 12 files** — QLFC was never selected. Two independent implementations agree. See §E6 measured result below. |
| E7 | arch-bwt / bench-normalize | **DONE** | enwik8 BWT + auto = 23,534,368 B (audit gap closed). |

**Rule restated:** no experiment below is "complete" until its measured number exists and is judged against the xz bar on bytes *and* the decode/peakmem margin (11 §3). Oracle/landscape numbers are targets, never results.

### Which reference is binding — per corpus (claim-hygiene note)

The "best reference" is **not the same codec on every corpus**, and mixing them
up produces wrong deltas. Always state which reference binds:

| corpus | Brotli q11/lw30 | xz -9e | **binding bar** | ANVIL measured | Δ vs binding bar | Δ quoted (conservative) |
|---|---:|---:|---|---:|---:|---:|
| Silesia | 49,383,136 | **48,456,100** | **xz** | **46,446,995** | −2,009,105 | −2,009,105 (xz) |
| enwik8 | **24,810,180** | 24,831,656 | **Brotli** | **23,534,368** | −1,275,812 | −1,297,288 (xz) |

- On **Silesia** xz beats Brotli by 927,036 B, so xz is the bar.
- On **enwik8** Brotli beats xz by 21,476 B, so **Brotli** is the bar. The correct
  enwik8 ranking is **BWT < Brotli < xz** (not BWT < xz < Brotli).
- Both published claims quote the **xz** delta, which on enwik8 is the *weaker*
  reference and therefore the **conservative** choice. The claims are safe as
  written, but the ordering must not be "corrected" the wrong way later.
- Combined Silesia + enwik8 auto = **69,981,363 B**.

## E0 — Close backend 2 as a valid experimental format

### Work

- update `FORMAT.md` backend registry with ID 2;
- specify postcoder payloads and bounds;
- add BWT adversarial cases;
- add golden direct-BWT files for tiny/repetitive/random/text inputs;
- run full fuzz.

### Pass

- all goldens roundtrip;
- all malformed cases reject cleanly;
- full existing fuzz passes;
- no rev1 behavior change.

### Stop

Any unexplained accepted corruption or attacker-sized allocation blocks all further backend-2 claims.

## E1 — Normalize build/timing

### Work

Create one pinned build script. Rebuild references and BWT with the same compiler/flags.

### Measure

- executable hash;
- compiler/version;
- compressed bytes;
- median-3 encode/decode;
- peak RSS;
- decoder-only binary size if available.

### Pass

Byte counts reproduce. Timing CV sufficiently small for claimed deltas.

## E2 — Whole-file backend auto, transforms OFF

Command concept:

```text
--parse=ratio
--ratio-backend=auto
--ratio-context=off
--ratio-lines=off
```

Run canonical Silesia + enwik8.

### Prediction

Silesia should approach the ~46.45 MB file-oracle derived from existing direct measurements, plus exact current-wire framing.

### Report

- per-file backend chosen;
- exact bytes;
- delta vs naked Brotli, xz, zstd;
- delta vs direct-BWT;
- aggregate;
- roundtrip.

### Stop / investigate

If measured bytes differ materially from the derived per-file minima, inspect candidate framing or selection before any optimization.

## E3 — Transform × backend matrix

For each Silesia file and enwik8, measure:

```text
direct × Brotli
ctx1   × Brotli
lines  × Brotli where eligible
direct × BWT
ctx1   × BWT
lines  × BWT where eligible
```

This answers whether transforms are globally weak or merely Brotli-specific.

### Key falsifier

If ctx1/lines remain rejected by BWT too, demote top-level transform work substantially. If they help BWT on new files, route transforms **conditioned on backend** rather than globally.

## E4 — Block-routing oracle

### Files first

`mozilla`, `samba`, `ooffice`, `sao`, plus one BWT-favorable control (`webster`).

### Block sizes

256 KiB, 1 MiB, 4 MiB, 16 MiB, whole file.

For every block, encode direct Brotli and direct BWT; compute exact oracle total including hypothetical backend/framing bytes.

### Report

- oracle bytes by block size;
- number/fraction of blocks won by each backend;
- median and p95 winning margin;
- total routing regret of whole-file selection;
- header/model warmup tax.

### Go condition

Build a production router only if an intermediate block scale buys a material aggregate amount after framing—suggested bar: >0.25% on Silesia or >250 KiB concentrated in mixed files.

### E4 MEASURED RESULT — FAILED (2026-09-07)

Source: `scratch/block-oracle.csv` + `scratch/block-oracle-wholes.csv`. Blocks
encoded independently with Brotli q11/lw30 and direct BWT; oracle = per-block min.

| file | block | whole-file min | block oracle | warmup tax | BWT block wins |
|---|---|---:|---:|---:|---|
| mozilla | 4 MiB | 13,806,141 | 14,341,000 | **+534,859** | 0 / 13 |
| mozilla | 16 MiB | 13,806,141 | 14,011,311 | **+205,170** | 0 / 4 |
| samba | 4 MiB | 3,761,899 | 3,946,257 | **+184,358** | 0 / 6 |
| samba | 16 MiB | 3,761,899 | 3,829,365 | **+67,466** | 0 / 2 |
| ooffice | 4 MiB | 2,478,857 | 2,504,298 | **+25,441** | 0 / 2 |
| ooffice | 16 MiB | 2,478,857 | 2,478,857 | 0 | 0 / 1 |
| sao | 4 MiB | 4,586,094 | 4,600,782 | **+14,688** | 0 / 2 |
| sao | 16 MiB | 4,586,094 | 4,586,094 | 0 | 0 / 1 |
| webster | 4 MiB | 7,317,329 | 8,211,644 | **+894,315** | 10 / 10 |

Full aggregate sweep (5 files: mozilla, samba, ooffice, sao, webster), block
regret vs whole-file routing:

| block size | aggregate regret | BWT-winning blocks on mozilla/samba/ooffice/sao |
|---|---:|---|
| 256 KiB | **+5,424,162 B** | 0 |
| 1 MiB | **+3,244,121 B** | 0 |
| 4 MiB | **+1,654,047 B** | 0 |
| 16 MiB | **+643,453 B** | 0 |

Go-bar was a *gain* of >529,846 B (0.25% of Silesia). Result: **zero gain —
regret ranges from +0.64 MB to +5.42 MB, monotonically worsening as blocks
shrink.**

**Structure finding.** Every file is won by **one backend across all its blocks**
(webster 100% BWT; the other four 100% Brotli at every scale). So a perfect
block router would gain nothing even before warmup is counted.

**Mechanism.** Per-block **model warmup** dominates; framing is negligible
(~100 B total at 4 MiB, derived from `src/anvil.cpp` block framing). The clean
proof is webster: BWT wins **all 10 of 10** of its 4 MiB blocks, yet
sum-of-blocks BWT = 8,211,644 vs whole-file 7,317,329 = **+894,315 B (+12.22%)**.
Brotli shows the same effect (+773,686 B, +9.28%). A *perfect* per-block router
would still lose.

**Falsification delivered.** mozilla, samba, ooffice and sao have **zero
BWT-winning blocks** at 4 MiB. This **falsifies the "mixed files contain
BWT-friendly text regions" hypothesis** (03 §6, I5, P1.2). Those files are
BWT-hostile *throughout*, not merely in aggregate. Consequence: they cannot be
recovered by carving out regions — which **strengthens P4.1 (DEFLATE
reconstruction)** as the only route to that headroom.

**Do not pursue finer granularity.** Warmup tax grows as blocks shrink, so
256 KiB / 1 MiB would be monotonically worse (16 MiB is already ~0 tax on
ooffice/sao).

## E5 — Cheap router from oracle labels

**PARKED (not runnable) — closed by E4 NEGATIVE.** E5 was pre-gated on E4 passing
its go-bar; E4 failed at every scale (block oracle strictly worse than whole-file),
so the router has no labels to learn from and is not runnable. This is the useful
outcome — the negative E4 *supersedes* E5, there is nothing to build.

**Reopen condition:** a near-zero-warmup backend (so per-block model cost vanishes)
or cross-block model carryover enters the picture. Until then, do not build. (Aligns
with `deliverable/E5` status = PARKED.)

### Dataset

Features from each oracle block, winner, byte margin.

### Model

Start with a deterministic shallow decision tree / integer rule list.

### Objective

Minimize **byte-weighted regret**, not classification error.

### Bars

- recover ≥90% of oracle byte savings;
- feature extraction <5% of cheaper backend encode time;
- deterministic;
- no decoder complexity increase beyond existing backend ID.

If a two-feature rule achieves the bar, do not build a fancier model.

## E6 — BWT postcoder ablation

### Controls

- current postcoder 0/1/2/3 individually forced;
- QLFC-like local-frequency coder;
- optional LZP on/off.

### Corpus

All Silesia files + enwik8, but prioritize the seven BWT winners first.

### Bars

- beat current BWT on ≥5/7 BWT-winning Silesia files without major decode regression;
- move direct-BWT aggregate enough to matter after router selection;
- report memory and decoder-size delta.

### Stop

If QLFC/LZP closes <~0.5% on BWT-winning files or makes decode/program size disproportionate, move to CM rather than endlessly polishing MTF.

### E6 MEASURED RESULT — NO-GO on both (2026-09-07)

Byte bar was: beat current BWT on ≥5/7 BWT-winning Silesia files. **Both
candidates beat it on 0 files.** Measured by bwt-theory in the standalone
harness `prototypes/bwt-lab/bwt_lab.cpp` over 13 files (Silesia 12 + enwik8),
baseline = BWT + adaptive order-1 arithmetic postcoder (75,564,140 B).

| candidate | aggregate bytes | vs baseline | files beaten |
|---|---:|---:|---:|
| baseline | 75,564,140 | — | — |
| QLFC (ID 4, per-context static rANS on MTF tokens) | 78,001,524 | **+2,437,384 (+3.23%)** | **0 / 13** |
| LZP (ID 5, order-4/32 prepass) | 75,843,559 | **+279,419 (+0.37%)** | 4 / 13 |

- **QLFC**: static tables cannot track the fast-changing MTF-rank distribution,
  and the per-file table costs 6–266 KB. Loses on every single file.
- **LZP**: wins only on dickens (−6.4 KB), mozilla (−122 KB), ooffice (−19.5 KB),
  samba (−55 KB) — **none of which are BWT-routed**. It **loses on every headline
  BWT winner**: webster +102 KB, x-ray +46 KB, nci +255 KB, osdb +25 KB, mr +16 KB,
  reymont +0.7 KB. The net is noise concentrated on files Brotli already wins.

**Independent corroboration (two implementations, same conclusion).** arch-bwt
shipped QLFC into `src/anvil.cpp` as postcoder 4 and the encoder selects the
smallest candidate; bwt-direct output is **byte-identical to the pre-QLFC
baseline on all 12 Silesia files** (`tests/bwt-backend-standard.csv` vs
`tests/auto-routing.csv`), i.e. QLFC was never selected in-codec either.

**Consistency with E2.** This explains why the measured auto total sits only
+159 B above the byte oracle: there was no postcoder headroom left to capture.
bwt-theory's isolated accounting agrees — the current adaptive O1 MTF/RLE
postcoder is essentially **saturated against the H1 entropy floor of its own
representation** (≈0 headroom on 9/13 files).

**Decision.** Do not implement QLFC/LZP as ratio features. Retain only as
ablation/scientific controls. Redirect the budget to **BWT decode throughput**
and **P4.1 DEFLATE reconstruction**. Note the in-codec QLFC encoder also contains
an unoptimized `O(N·256)` hot loop (`qlfc_build_rank` called per byte) — left
unfixed deliberately, since the representation itself is a measured loss.

## E7 — enwik8 BWT and auto

This should happen before claiming bsc-class standing. Current BWT work has no enwik8 result.

Measure:

- BWT direct;
- auto direct;
- postcoder breakdown;
- compiler-normalized speed/memory.

Published bsc ~20.8 MB provides landscape orientation, not a substitute for a local bsc build if making a direct speed claim.

## E8 — Fixed-point CM prototype

Start only after E0–E7 make the BWT/routing frontier clear.

### Freeze before code

- memory budget (e.g. 512 MiB / 1 GiB / chosen point);
- target encode/decode throughput class;
- exact context set;
- fixed-point probability/logit precision;
- SSE/APM table sizes;
- backend ID and wire constants.

### First bar

Do not target cmix. Target lpaq1-class transfer:

- beat Brotli on a majority of Silesia files;
- beat current auto-Brotli/BWT aggregate by a material amount;
- exact deterministic roundtrip;
- bounded memory.

## E9 — Deflate reconstruction probe before full implementation

### Phase A: census

On `mozilla` and `samba`, identify candidate zlib/gzip/ZIP/PNG/raw-DEFLATE streams and total compressed/plain bytes.

### Phase B: reference oracle

Use scratch preflate/preflate-rs references to estimate reconstruction-info economics. Do not integrate code yet.

### Phase C: clean-room/attributed implementation decision

Only build ANVIL integration if the census+oracle shows enough recoverable bytes under our actual backends.

### Mandatory rule

Encode must validate bit-exact reconstruction before accepting a stream. Failure = opaque fallback, never lossy reconstruction.

## E10 — Binary stride matrix

Targets:

- `mr`;
- `x-ray`;
- `sao`;
- `osdb`.

Test candidate strides/deltas under Brotli **and BWT**. Later add CM.

Do not route based on entropy proxy alone; the `mr` H1 diagnostic already disproved that simplification.

## E11 — External falsification expansion

Add fetch scripts + hashes for Pizza&Chili Real/Logs and small-block slices.

No new general-purpose claim after Silesia/enwik8 should be considered mature until it survives at least one additional third-party regime.

## Session planning rule

For future sessions, ship complete gates rather than nine partial threads:

1. correctness/spec;
2. measurement;
3. verdict;
4. only then next mechanism.

The project’s history shows that partially integrated mechanisms accumulate narrative debt faster than research value.

