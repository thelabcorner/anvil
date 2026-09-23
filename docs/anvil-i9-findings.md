# ANVIL — Iteration-9 Findings (Comprehensive Integration Report)

> **Status:** frozen iteration snapshot, compiled at I9 close (2026-09-12) by the
> coordinator of swarm `anvil-i9-pareto` (`swarm_660ef29af883450dbb409d8a3f084d2e`).
> **Nature:** integration report over *verified* artifacts only. It adds no new
> measurements and makes no new claims. Where a number lives in a lane artifact,
> the artifact is named so it can be recomputed.
>
> **Truth priority when documents conflict** (audit README): canonical CSV /
> exact executable output > current source + `FORMAT.md` > gate derivations >
> `RESEARCH_LEDGER.md` > old `docs/CONTEXT.md` / briefs.
>
> **Standing claim rules:** every number recomputable from a committed artifact
> with the command shown; measured vs derived vs projection labelled; frontier
> citations carry reference class + grid + `GRID-THIN`; decode-route arithmetic
> tokens (`DECODE-GO` / `DECODE-TIE` / `DECODE-SHORT`) are never frontier
> verdicts; `FRONT-GAP` / `FRONT-CROSSING` / `DEGENERATE` are research-gate's.

---

## 1. Executive verdict

**ANVIL beats both binding references on bytes on both canonical corpora, and
has produced zero frontier crossings.** That is the whole story, stated
honestly:

- **Bytes — WINS.** `--ratio-backend=auto` (BWT + Brotli portfolio) produces
  **46,446,995 B** on Silesia (xz -9e: 48,456,100; Brotli q11/lw30:
  49,383,136) and **23,534,368 B** on enwik8 (xz: 24,831,656; Brotli:
  24,810,180). Reproduced byte-exactly on the frozen canonical binary.
- **Decode — FRONT-GAP.** The same rows decode **14.597x slower than
  brotli-q11** (7.885x slower than xz) on Silesia, and **23.946x / 14.062x** on
  enwik8. The bytes win cannot be called a Pareto crossing.
- **Frontier — ZERO CROSSINGS.** Frozen-grid tuple
  `33 non-dominated | 5 FRONT-GAP | 0 FRONT-CROSSING | 28 DEGENERATE |
  435/468 dominated` (471/504 with AGGREGATE); xz-inclusive committed-HEAD
  tuple `4 | 4 | 0 | 412/416`. No row is a FRONT-CROSSING on any grid or
  reference class.
- **What changed in I9:** the decode axis was *closed by measurement* (five
  routes tried, four falsified, one retained but insufficient); the byte axis
  gained a verified DEFLATE-reconstruction result (1.36 MB on mozilla, pending
  integration); two strongest mechanisms were proven self-discovering but
  `{synthetic}`-bounded; the reference class was hardened with xz and
  transform controls; and four significant recorded-state errors were caught
  and superseded.

---

## 2. Scope and method

**Swarm:** 10 autonomous workers + coordinator, all
`opencode-go/deepseek-v4.1-flash@zen-889db3308123` (variant `max`), working the
same repository under strict lane ownership:

| lane | owns |
|---|---|
| arch | `src/anvil.cpp` (sole writer) |
| bench | `tests/*.csv`, `tools/pareto_front.py`, measurement windows, arbiter sign-off |
| format | `FORMAT.md`, `tests/fuzz.py`, registry coverage |
| research-gate | novelty gate, rulings, `RESEARCH_LEDGER.md`, claim verification |
| strategy | `docs/swarm-i9-strategy.md` (synthesis, no code) |
| decode-perf / bwtinv / deflate / pnra / datastruct | their prototype lanes |

**Measurement discipline (contract v1.2):** announced windows are exclusive
(no builds/gates/fuzz during a window); `RANKING-GRADE` = effect >= 5x arm CV
and external load <= 20 pp over ambient; `CITATION-GRADE` adds affinity, a
pinned-core pre-window mean < 5%, same-window paired arms, reps>=7, and load
<= 5 pp over ambient. Every decode number states its thread count; multi-thread
rows are never silently compared against single-thread references.

**Anti-tie convention v2 (bench):** `R' = 1.02 x R` from exact rows;
single-component cap `C = 1/(1-s)`; no compounding legs; a wall-clock override
requires clearing by `>= max(2%, window CV)`. Tokens `DECODE-GO` / `DECODE-TIE`
/ `DECODE-SHORT` apply to decode-route arithmetic only.

**Dual bar (research-gate):** on synthetic cells a raw-bytes win must also beat
a documented **same-transform control** (e.g. `xz -9e --delta=dist=P`);
otherwise it is `FRONT-GAP`. `PR-2`: a row whose own ratio is >= 0.95 is
`DEGENERATE`. `PR-3`: `FRONT-CROSSING` iff non-dominated AND outside every
axis-rectangle gap of the reference front.

**Registry-coverage rule (in code):** every decoder-visible registry ID has a
direct forced encode->decode test, or a clean up-front rejection; selection-
based coverage is insufficient.

---

## 3. Canonical artifacts and provenance

**Final canonical binary (frozen):**

| artifact | sha256 |
|---|---|
| `src/anvil.cpp` | `BDC90474` (ALLOC + decode leg 4 retained; MAT reverted) |
| `build\anvil.exe` | `72D65150CBD65182C69BA85DAD3A8961C25DE48E794FD9823BCAB4ED20A36505` |
| `build\anvil_bench.exe` | `379341D93B6504EBDADF8BDB7FE69B2756142FD9823E5398A31E9A5C2176851E` |

Superseded shas (their byte results still carry by wire-identity gates, and
must be labelled when cited): `E8AA2E48`/src `38409E26` (ALLOC-only),
`0D1E130B`/src `62BC6631` (decode fix + S6-1b CRC), `de9b4caf` (pre-fix
canonical for the original ratio-first numbers), lane `FE0CF4F1` (leg-4 exe).

**Verification state on the frozen build:**

- `python tests\fuzz.py --exe build\anvil.exe --cases 50` -> **PASS, zero skips**:
  `roundtrip_variants=480 mutations=2880 deterministic_rev2=9 deterministic_bwt=24 golden_bwt=4 forced_postcoders=20 registry_block_modes=14 registry_transforms=5 suite_modes=[0,1,2,3,5,6,8] suite_unforceable=[4,7]`.
- Registry: block modes `0,1,2,3,4,5,10,11,12,13,14,15,16,17` forced-roundtrip;
  transforms `0,1,2` roundtrip (incl. transform-2 + backend-2), transform `3`
  clean up-front rejection (LZP, NO-GO per 06 G2); backends `1,2`; postcoders
  `0-4` (1/2/3 roundtrip, 0/4 reject); malformed IDs reject.
- Roundtrip: **442/442** (24 files x 18 specs, serial and
  `--decode-threads=4`). Encode wire identity: **494/494 same / 0 diff** vs
  HEAD `fc23d9a`. Suite byte identity: **351/351 rows, 0 diff** across the
  frozen grid refresh.

**Key datasets:**

| artifact | contents |
|---|---|
| `tests/ratio-first-standard.csv`, `tests/bwt-backend-standard.csv` | Silesia 4-way + BWT-direct cells (canonical byte wins) |
| `tests/ratio-reverify-i9.csv` (sha `53880D50`) | canonical re-verify on fixed binary |
| `tests/benchmark-suite.frozen-bdc90474.csv` (sha `fc422712`) | 351-row frozen 13-file grid |
| `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` (sha `af44d9d3`) | xz-inclusive grid |
| `tests/suite-frozen-bdc90474-reps.csv` | 1,269 per-rep rows |
| `tests/suite-frozen-bdc90474.md` | frozen-grid narrative + signed store-path section |
| `tests/xz-reference-i9.csv`, `tests/xz-transform-controls-i9.csv` | reference-class v2 |
| `tests/pareto-baseline.head-fc23d9a.csv`, `pareto-verdict.head-fc23d9a.csv` | committed-HEAD tuples |
| `tests/pr-4-measurement-window-protocol.md` (v1.2) | measurement contract |

**Synthesis and ledger:** `docs/swarm-i9-strategy.md` (frozen section
`7-FINAL`, sha256 `6EACCA45CAC2C450C691CBB9A6D13D4F634CCC8FD951F3CE23D2522078A8049E`);
`RESEARCH_LEDGER.md` PART XIV, addenda **A1–A21** (authoritative index);
rulings `docs/gate-ruling-i9-*.md`; harnesses `docs/gate-verify-i9*.py`.

---

## 4. Program B — canonical ratio-first corpora

Deployment: whole-file `--ratio-backend=auto` routes per file (BWT 7/12 on
Silesia, Brotli 5/12); transforms OFF; sha256 roundtrips enforced.

### 4.1 Bytes (measured, reproduced on the fixed binary)

| corpus | ANVIL auto | vs Brotli q11/lw30 | vs xz -9e | vs zstd u22/l27 |
|---|---:|---:|---:|---:|
| Silesia (211,938,580 B) | **46,446,995** | **-2,936,141** | **-2,009,105** | **-6,075,348** |
| enwik8 (100,000,000 B) | **23,534,368** | **-1,275,812** | **-1,297,288** | — |

- BWT-direct alone: 52,030,185 B (wins 7/12 files; `mozilla` is catastrophic
  at +4,017,510 B vs Brotli, which is exactly why routing, not a single
  backend, wins the aggregate).
- Oracle vs direct-BWT/Brotli (`46,446,836`) is **validated, not beaten**:
  measured is +159 B (framing only). The `min(Brotli,BWT,xz)` figure
  `45,782,529` remains a *landscape target*, not an ANVIL result.

### 4.2 Decode cost (gate A1; definitive citation of record)

| row | decode | vs brotli-q11 (137.722 MB/s) | vs xz-9e (74.397 MB/s) |
|---|---:|---:|---:|
| Silesia 12-file auto portfolio | 9.435 MB/s | **14.597x** | **7.885x** |
| enwik8 auto | 5.890 MB/s | **23.946x** | **14.062x** |
| enwik8 same-bytes BWT-direct variant | 6.730 MB/s | 20.960x | 12.308x |
| forced-BWT 7-file subset | 13.015 MB/s | **11.243x** | 6.0x |

The audit's historical "11.2x / 13.0 MB/s" survives **only** as the forced-BWT
subset; it is not the portfolio figure. Decode multiples are single-run unless
a PR-4 window is named; the frozen-suite window remains ranking-grade.

### 4.3 Cost axes

- **Peak memory (auto):** ~10.3 GiB on every file, ~ flat in input size — this
  is Brotli q11 `lgwin=30`'s own encoder window (the reference helper pays
  10,273 MiB vs ANVIL's 10,278 MiB); BWT-only paths are 69-718 MiB. Not an
  ANVIL defect, but it still fails the peak-memory axis vs xz (100-570 MiB).
- **Encode:** the auto router pays a Brotli candidate encode it may discard
  (on one BWT-won file: auto 30.42 s / 10,318 MiB vs bwt 9.22 s / 126 MiB).
  This is a cost-axis fix candidate, not a byte issue.

---

## 5. Program A — 13-file Pareto arbiter (frozen grid)

Arbiter predicate (`tools/pareto_front.py`): `q` dominates `p` iff
`q.ratio <= p.ratio AND q.mbps >= p.mbps`, one strict. Reference-class v2 adds
`xz -9e` rows and side-channel transform controls.

### 5.1 Frozen tuple (canonical `BDC90474`; v1 and v2 identical)

> **33 non-dominated | 5 FRONT-GAP | 0 FRONT-CROSSING | 28 DEGENERATE |
> 435/468 dominated (471/504 incl AGGREGATE)**

- The **28 DEGENERATE** are `random.bin` + `synth-arith.bin` store-class cells
  (14 pairs x 2 planes, ratio 1.000) — crossing-inert by PR-2.
- The **5 FRONT-GAP** are `generated.json` encode-plane rows (mdl-rans,
  mdl-rans-l0, mdl-rans-l001, shape-rans, shape-rans-l0), bracketed by
  brotli-q11 <-> zstd-19; dominated on decode.
- `v1 = v2` on this grid is **speed-dependent**: frozen `generated.json` rows
  encode at 1.396-1.596 MB/s, above xz's 1.325 MB/s, so xz adds no dominance
  here (on HEAD/worktree it did).

### 5.2 Co-listed tuples (cite with grid + reference class + `GRID-THIN`)

| grid | reference class | tuple |
|---|---|---|
| frozen `BDC90474` | v1 = v2 | `33 | 5 | 0 | 28 | 435/468` |
| worktree `C70179EA` | v1 brotli+zstd | `5 | 5 | 0 | 463/468` |
| worktree `C70179EA` | v2 +xz-9e | `1 | 1 | 0 | 467/468` |
| committed HEAD `fc23d9a` | v1 brotli+zstd | `5 | 5 | 0 | 411/416` |
| committed HEAD `fc23d9a` | v2 +xz-9e | `4 | 4 | 0 | 412/416` |
| recon-i9 (9 newer cells) | v2 +controls | `3 | 3 | 0 | 321/324` (2 mechanically non-dominated candidates + 1 bracket GAP; all dual-bar rejected) |

`GRID-THIN` = missing zstd 4-22 / brotli lw30 tiers; denser tiers can only make
crossings harder. **Zero FRONT-CROSSING on every grid and reference class.**

### 5.3 The one "frontier event" and its rejection

`synth-columnar-align.bin` `anvil-hotop-rlzp-rans` reached ratio **0.3680**,
below brotli-q11 (0.3840) and xz -9e (0.4028) — the first row ahead of all
references. It was **rejected** by the dual bar: the documented same-transform
control `xz -9e --delta=dist=23` = **21,332-21,340 B (ratio 0.0773)** is
**4.76x smaller**. Classification: `FRONT-GAP`; the row additionally carries
`NON-DEFAULT/RESEARCH-CONFIG` (hotop-rlzp is retired as a default, 0.048 MB/s
encode) and `{synthetic}`. "First crossing" language was struck.

---

## 6. The decode axis — closed by measurement

### 6.1 Route-by-route

| route | result | verdict | artifact |
|---|---|---|---|
| CRC-only fast path | synth-timeseries CRC share 26.3/28.7/27.0% vs 43.82% floor; C=1.36-1.40x vs 1.78x needed | **DECODE-SHORT** (dead) | `deliverable/pr1-profile` |
| Materialization (lazy StreamPull) | generated.json mdl 4.1545 -> 4.2117 ms (+1.4%); hotop-rlzp 1.4495 -> 1.8499 ms (+27.6%, CV 9.5%) | **falsified / RETIRED** (implementation-era) | `prototypes/i9-arch/MAT-ALLOC-leg.md` |
| ALLOC (decode into output, no block vector/concat, bounded reserve) | mdl 4.1604 -> 3.0766 ms (**1.352x**); hotop-rlzp 1.4829 -> 1.3802 ms (**1.074x**) | **retained** (insufficient) | same |
| Postcoder cheap edges (buffered renorm) | arith+Fenwick = 91-93% of postcoder; C=1.021/1.014/1.013 vs R'=1.617/1.137/1.436 | **DECODE-SHORT** | `prototypes/i9-decode-perf/POSTCODER-SPEC.md` |
| Decode leg 4 (two-level cumulative tables + buffered renorm) | dickens 89.121 -> 73.353 ns/outB (1.215x); webster 74.980 -> 64.135 (1.169x); aggregate **1.179x** vs ~1.4x target | **retained; target falsified** | `prototypes/i9-arch/LEG4-postcoder.md` |
| aux-unbwt (`libsais_unbwt_aux` + I-array) | **3.40-3.55x** same-core (dickens 20.64->70.18, webster 18.36->64.15, enwik8 17.00->60.33 MB/s); wire charge 19,328 B / enwik8 3,052 B | ranking-grade; integration pending | `prototypes/i9-bwtinv/` |
| threaded unbwt | `libsais_unbwt_omp` parallelizes init only; no walk gain | **falsified** | same |

### 6.2 Corrected requirement

The published postcoder requirement (1.66x/1.13x/1.46x, ~1.38x portfolio)
assumed a **free** unbwt. Corrected with the real aux factor
`k_p = s_p/(R_base/40 - s_u/k_u - s_other)`:

> **required 6.05x dickens / 3.86x webster / 6.60x enwik8** (pinned record);
> leg-4 stage achieves ~1.67-1.73x; aux+leg4 combined 2.13-2.38x ->
> BWT-only ~23.8-29.7 MB/s; portfolio ~**25.7 MB/s vs the 40 MB/s bar**.

### 6.3 Final decode decisions (frozen)

- `generated.json` and `synth-timeseries.bin`: **DECODE-SHORT, closed by
  measurement** on the measured ALLOC-only caps (1.352x / 1.074x) against
  frozen requirements R'=2.0699 / 1.5248. The earlier `DECODE-TIE` calls used
  the pre-A9 projected mat caps and are **VOID**.
- **Static-table postcoder (format change): NOT AUTHORIZED.** ID3 raw is
  5-6.6x faster on the postcoder but +52-101% bytes; even a perfect postcoder
  leaves libsais_unbwt at 63-69% of decode; the 11-24x real-corpus gap is not
  closable this way. Any revisit needs a pre-registered byte budget.
- **Classification: the decode axis is a STRUCTURAL/MATH gap on the real
  corpora, not an unexplored implementation.**

### 6.4 The one kept decode win: store-path CRC

Root-caused and signed (`w-arch-storepath-20260912T1300Z`, 3 arms interleaved,
reps=5, core 18, 1t, `wire_fnv` identical across arms):

| arm | random.bin enc / dec | synth-arith enc / dec |
|---|---:|---:|
| bytewise (`51C6A939`) | 412 / 362 MB/s | 418 / 364 MB/s |
| slicing-by-8 (`B743061C`) | 1156 / 888 MB/s | 1109 / 813 MB/s |
| PCLMUL (`4F00627B`) | **1872 / 1216** MB/s | **2012 / 1147** MB/s |

The bytewise arm reproduces the prior grid's band under the same protocol, so
the jump is the CRC implementation, not the measurement method. CRC values are
bit-exact (16,529 unit + 5,784 corpus checks, 0 fail), so bytes are identical.
**Kept win: 4.5x encode / 3.4x decode (PCLMUL vs bytewise) on the store path;
wire-invisible; citable.** The 28 DEGENERATE cells' throughput axis is
explained by this change.

---

## 7. Byte-axis mechanisms

### 7.1 Backend heterogeneity and routing (the win)

- Direct BWT beats Brotli on **7/12** Silesia files and beats xz on **all 7**
  of its wins (webster -1,051,319; x-ray -473,952; mr -369,578; osdb -259,907;
  dickens -259,347; reymont -171,568; nci -83,568 vs xz -9e).
- The measured auto portfolio is therefore a **router result**, not a single-
  algorithm result: per-file `min`-like selection with framing overhead of
  only +159 B over the oracle.
- Block-local routing was separately **closed by negative** (E4): per-block
  warmup tax makes every block scale worse than whole-file (+1.65 MB at
  4 MiB); reopen only on a near-zero-warmup backend or cross-block carryover.

### 7.2 P4.1 — bit-exact DEFLATE reconstruction (GO direction)

- **mozilla:** 13,806,173 -> **12,443,996 B** = **1,362,177 B recovered**
  (3.17x the go/stop bar), beating mozilla xz -9e (13,376,248) by 932,252 B.
- Census: 2,564 DEFLATE stream records (3,177,007 B compressed / 9,991,436 B
  uncompressed); replay valid on 2,331 of 2,354 attempts = **91.2% of bytes**;
  local ceiling 1,459,509 B; diff-mode upside +37-72 KB `[PROJECTION]`.
- Scope: `sao` + `ooffice` contain **zero** DEFLATE (212,047 B of the
  664,307-B landscape gap is untouchable); `samba` 411,393 B deflate /
  193,463 B ceiling; addressable ~**1,652,972 B**.
- Status: both legs byte-verified (the two `.anv` files decode sha256-exact;
  reconstruction leg sha-verified by the prototype inverse+replay), but the
  recovery figure is **encode-only on a build whose decode path was broken at
  measurement time** -> **GO direction, compression claim PENDING native
  integration + roundtrip + fuzz**. No mechanism novelty (precomp/preflate
  prior art) — adopt-class infrastructure.

### 7.3 pnra — native step reference (adopt-class, self-discovering)

- Prototype wire **12,936 B** (ratio 0.0505) on `synth-arith.bin` at 462.3 MB/s
  prototype-parallel decode; AUDIT-7 exact charge
  `44+32+40+13+16+12,791 = 12,936`; entropy floor 12,670.4 B; 160/160 fuzz.
- **Transmitted step (12,921 B) beats derived step (12,936 B)** — the zero-bit
  derived form is unnecessary as well as non-exact. PR-5: **no novelty**,
  adopt-class `{engineering}`.
- **Auto-discovery:** run boundaries + per-run step/base are discoverable from
  data alone; compact auto wire **12,920 B = 0.9988x oracle** (24 B discovery
  table); 8/8 regions, 7/7 boundaries exact; 10/10 randomized anatomies,
  166/166 boundaries exact. Failure bounds: drift-stride / log / counters /
  random have no >=6-word step regions (literal fallback +48 B, no gain);
  +/-2-jitter fragments degrade (915 regions, 0.807); escape-derived
  boundaries cost 3,197.7 B vs 24 B.
- Standing: `{synthetic}`-class, mechanism probe; not an ANVIL row.

### 7.4 datastruct — record-reference columnar residuals (adopt-class)

- Supplied structure: **89,877 B** (ratio 0.3210) vs brotli-q6 120,669
  (-25.5%) and ANVIL best 143,132 (-36.2%); ladder `raw_o0 207,197 /
  raw_o1 187,436 / ref_flat 107,564 / ref_col 97,057 / ref_field 89,877`;
  ablation: mechanism **97,559 B** vs coder-only 19,761 B.
- **Auto-discovery:** `ts` AUTO 89,916 B (+39 B), `ca` AUTO 24,026 B (P=23;
  the 184 alias rejected; 29 B better than the hand layout);
  `generated.repeat.jsonl` P=235 all-const. **Honest non-transfer bound:**
  `generated.jsonl` AUTO 1,199,898 B vs brotli-q6 206,840 (5.8x);
  `generated.sqlite` 1,359,687 vs 252,591 (5.4x) — phase instability (line
  lengths 229-250, uniform line-start phase, best match 0.57); requires
  per-record alignment, outside the fixed-period model.
- Dual bar: the transform controls (xz+delta 89,564 B; brotli+delta 80,650 B)
  are both smaller than the row -> **FRONT-GAP, novelty NO**.

---

## 8. Kept engineering wins (wire-invisible)

| win | magnitude | status |
|---|---|---|
| Store-path CRC (slicing-by-8 + PCLMUL dispatcher) | **4.5x enc / 3.4x dec** on store path | signed, citable |
| aux-unbwt I-array | **3.40-3.55x** same-core on unbwt | ranking-grade; integration pending |
| ALLOC decode | **1.352x** (generated.json mdl) / **1.074x** (hotop-rlzp) | retained |
| Decode leg 4 (two-level tables + buffered renorm) | **1.179x** aggregate (dickens 1.215x / webster 1.169x) | retained |
| PCLMUL dispatch (part of CRC bundle) | included above | retained |

Wire charges: aux I-array = 19,328 B across the 7-file Silesia BWT set
(enwik8 3,052 B), ~0.96% of the 2,009,105-B xz margin. ALLOC/leg-4/CRC carry
zero wire cost.

---

## 9. Falsifications and retirements (do not re-burn)

| mechanism | status | class |
|---|---|---|
| CRC-only decode route | DECODE-SHORT, dead | math (share too small) |
| Materialization / lazy StreamPull | falsified, retired | implementation-era (dispatch > bulk+vector) |
| Postcoder cheap wire-invisible edges (MTF/alloc/renorm) | exhausted | implementation-era |
| Static-table postcoder (transmitted tables) | not authorized | byte cost vs 11-24x gap |
| Threaded libsais unbwt | falsified | implementation-era |
| Block-local routing (E4) | closed by negative | math (warmup tax) |
| QLFC / LZP postcoders (E6) | NO-GO | measured ratio loss |
| Encode-plane knee rows as objective | retired (I8) | math |
| Decode-plane rows at ratio ~1.000 | DEGENERATE (PR-2) | math |
| Global pre-LZ lane/field transposition | retired (I8) | math |
| Pathological "first crossing" on synth cells | dual-bar rejected | contract |
| Zero-bit derived parameters on statistical transforms | closed (DNB-M2) | math |

---

## 10. Corrections and errata caught in I9

1. **Dirty-tree decode break** — the worktree `decode_one_block` refactor
   passed the mode byte as the payload pointer; every file failed to decode.
   Fixed by arch (contract fix + BWT primary index 1-based inclusive + n==1
   edge), verified independently by the coordinator and format.
2. **Record periods** — `synth-timeseries` is 20,000 x 14 B (P=14), not
   10,000 x 28 B; `synth-columnar-align` is 23 B x 12,000 (P=23), not 184
   (=8x23). The old figures were stride aliasing; constant fractions correct
   to 28.6% / 21.7% (I8's 30.4% was an alias artifact).
3. **Grid provenance** — the long-cited `468/463` grid exists only in the
   uncommitted worktree (`C70179EA`); committed HEAD is `416` cells / `411`
   dominated. Citation rule added (v3).
4. **Decode multiples** — the audit's "11.2x / 13.0 MB/s" is the forced-BWT
   7-file subset only; the portfolio is 14.597x (Silesia) and 23.946x
   (enwik8). Strategy's transient 20.33x was an auto-run window artifact.
5. **Reference class** — the familiar `5` non-dominated is a brotli+zstd-class
   property; with xz-9e and transform controls adopted (v2), committed HEAD is
   `4 | 4 | 0`.
6. **Postcoder requirement** — 1.66x/1.13x/1.46x assumed a free unbwt;
   corrected to 6.05x/3.86x/6.60x.
7. **`140,898` hotop-rlzp** is worktree-only; committed-HEAD best on
   synth-timeseries is `143,132` (sparse-rans).
8. **P4.1 scope** — the "234,047 B untouchable" figure was struck; the
   recorded value is 212,047 B (sao + ooffice only).

---

## 11. Contracts and claim rules (in force)

| rule | content |
|---|---|
| **Citation rule v3** | cite tuples with grid + reference class + `GRID-THIN`; frozen `BDC90474` primary; worktree labelled; binary sha named; throughput grade named |
| **Measurement contract v1.2** | exclusive `w-*` windows; ranking vs citation grade; thread counts mandatory; PR-4 fields |
| **Anti-tie v2** | `R'=1.02 x R`; single-component cap; no compounding legs; wall-clock override; decode tokens scoped |
| **Dual bar** | synthetic-cell raw wins must beat the documented same-transform control |
| **PR-2 / PR-3** | DEGENERATE row guard; FRONT-GAP vs FRONT-CROSSING definition |
| **Registry coverage** | every decoder-visible ID force-tested or cleanly rejected |
| **Evidence rule** | every published number recomputable from a committed artifact with the command shown; record, do not celebrate |
| **Recompute rule** | bind all lanes including coordinator/strategy/gate |

---

## 12. Ledger and verification

`RESEARCH_LEDGER.md` **PART XIV** addenda A1-A21 (authoritative), including:

- **A1** definitive decode multiples; **A2** P4.1 verification; **A3** CRC route
  closure; **A5** pnra auto-discovery verified; **A6** datastruct
  auto-discovery verified; **A7** byte-total provenance (canonical re-verify);
  **A8** postcoder cheap-route closure; **A9** MAT retirement; **A10/A12**
  ALLOC retained; **A13** recon crossing rejection; **A14** reference-class v2
  verified; **A15** leg-4 retained + target falsified; **A16** cross-leg `k_p`
  correction; **A17** format re-verify; **A18** frozen grid verified;
  **A19** DECODE-TIE void; **A20/A21** store-path anomaly root-caused and
  closed (signed win).

Verification harnesses: `docs/gate-verify-i9.py` (with `--refs`,
`--transform-controls`), `gate-verify-i9-corpus.py`,
`gate-verify-i9-transform-bar.py`, `gate-verify-i9-decode-multiples.py`.

Rulings: `docs/gate-ruling-i9-frontier-tokens.md`,
`gate-ruling-i9-pr5-position-derived.md`,
`gate-ruling-i9-datastruct-ts.md`, `gate-ruling-i9-decode-multiples.md`,
`gate-ruling-i9-p41-deflate.md`, `gate-ruling-i9-recon-crossing.md`,
`gate-verify-regime-i9.md`.

---

## 13. PENDING (explicit)

1. **P4.1 native integration** — wire-level reconstruction in `src/anvil.cpp`,
   then byte-exact roundtrip + fuzz before the recovery figure is citable.
2. **aux-unbwt integration** — new wire version/id, FORMAT.md registration,
   direct forced test, wire-charge co-listing.
3. **pnra / datastruct integration** — default-off flags + byte-identity gates,
   if prioritized.
4. **Citation-grade reruns** — aux-unbwt magnitude (quiet-window paired);
   store-path is already signed/citable.
5. **Static-table postcoder** — only with a pre-registered byte budget.
6. **Commits** — the entire I9 tree is uncommitted; the coordinator holds
   commit assembly (separated: decode fix / ALLOC / leg-4 / CRC / format /
   docs).

---

## 14. Next-iteration candidates (strategy-ranked)

1. **P4.1 integration** — 1.36 MB on real mozilla, no decode regression
   (target files already Brotli/xz-routed); the largest actionable byte win.
2. **aux-unbwt integration** — 3.40-3.55x real unbwt win at ~19 KB wire cost;
   half of the BWT decode story.
3. **pnra integration** — self-discovering step reference (synthetic-class;
   lower real-data value).
4. **datastruct capability** — fixed-period records only; real-data transfer
   bound is recorded and unsolved.

Everything else on the decode axis is closed or unauthorized; everything else
on the byte axis needs a new mechanism to clear the dual bar on real corpora.

---

## Appendix A — key numbers (quick reference)

- Silesia: ANVIL auto 46,446,995 | Brotli 49,383,136 | xz 48,456,100 | zstd 52,522,343
- enwik8: ANVIL auto 23,534,368 | Brotli 24,810,180 | xz 24,831,656 | zstd 25,333,695
- Decode: Silesia 14.597x vs brotli / 7.885x vs xz; enwik8 23.946x / 14.062x
- Frozen tuple: 33 | 5 | 0 | 28 | 435/468 (471/504); HEAD v2 4 | 4 | 0 | 412/416
- Decode closures: CRC dead; MAT +1.4%/+27.6% (falsified); ALLOC 1.352x/1.074x;
  leg-4 1.179x; postcoder requirement 6.05x/3.86x/6.60x; portfolio ~25.7 vs 40 MB/s
- Store-path CRC: 4.5x enc / 3.4x dec (PCLMUL vs bytewise), signed
- aux-unbwt: 3.40-3.55x, wire 19,328 B / 3,052 B
- P4.1: 1,362,177 B recovered on mozilla (encode-only); ceiling 1,459,509;
  addressable 1,652,972
- pnra: 12,936 B wired / 12,920 B auto (0.9988x oracle); datastruct: 89,877 B
  supplied / 89,916 B auto
- Canonical: `72D65150` / `379341D9` / src `BDC90474`; fuzz PASS zero skips;
  442/442 roundtrip; 494/494 wire; 351/351 suite bytes

## Appendix B — artifact map

- Synthesis: `docs/swarm-i9-strategy.md` (§7-FINAL) | Ledger: `RESEARCH_LEDGER.md` PART XIV
- Rulings/harnesses: `docs/gate-ruling-i9-*.md`, `docs/gate-verify-i9*.py`
- Lanes: `prototypes/i9-arch/`, `i9-decode-perf/`, `i9-bwtinv/`, `i9-deflate/`,
  `i9-pnra/`, `i9-datastruct/`
- Datasets: `tests/benchmark-suite.frozen-bdc90474*.csv`, `tests/suite-frozen-bdc90474.md`,
  `tests/ratio-first-standard.csv`, `tests/bwt-backend-standard.csv`,
  `tests/ratio-reverify-i9.csv`, `tests/xz-reference-i9.csv`, `tests/xz-transform-controls-i9.csv`
- Blackboard: `deliverable/strategy-i9` v33, `deliverable/frozen-grid` v3,
  `deliverable/p0-arbiter`, `deliverable/pr1-profile` v7, `deliverable/bwt-inverse`,
  `deliverable/p41-deflate` v6, `deliverable/pnra-stride` v3,
  `deliverable/datastruct-ts` v2, `deliverable/gate-i9` v2,
  `deliverable/format-i9`, `deliverable/i9-recon` v2, `deliverable/reference-class-v2`,
  `deliverable/i9-decode-remeasure`, `deliverable/pr-4-window-protocol`

---

*Compiled by the coordinator of `anvil-i9-pareto`. This report is a summary of
verified artifacts, not a source of record; where it and a lane artifact
disagree, the lane artifact and the ledger govern.*
