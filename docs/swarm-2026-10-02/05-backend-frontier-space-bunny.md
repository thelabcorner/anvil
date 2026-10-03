# Track 05 — Backend Frontier Beyond Brotli — INTERIM CHECKPOINT (Space Bunny Free)

> **ADDENDUM §11 (shared-measurement alignment, 2026-10-02) supersedes the protocol
> enumeration in §8 wherever the two differ, and adds counter set B4 + a sharpened
> attribution gate.** One protocol, one driver, one job. See §11.
>
> **ADDENDUM §12 (instrumentation-perturbation separation, 2026-10-02) supersedes §11
> wherever counters and timing are mixed.** Timing comes only from uninstrumented builds;
> counters are byte-only and never enter a gate. See §12.

**Author:** Space Bunny Free (constructive inventor), track `05-backend-frontier`
**Date:** 2026-10-02
**Worktree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty. No existing file modified.
**Status:** INTERIM. Every number below carries its source artifact and timing-series label.
Measured facts are marked **[M]**, arithmetic on measured inputs **[M+arith]**, and
projections/beliefs **[P]** / **[B-belief]**. No local benchmark was run; no corpus
compression/performance measurement was executed locally.

---

## 0. Executive verdict (read this first)

1. **ANVIL's ratio lead is the BWT backend, not the semantic frontend.** `[M+arith]`
   The frozen canonical portfolio is Brotli-size only because `kRatioBackendBwt`
   is selected on 7/12 Silesia files and on enwik8 (`docs/I10-AUX-UNBWT-RESULTS.md` §8).
   Every G-stage (G0–G3) pinned **Brotli q11/lgwin30 as the backend**
   (`docs/I10-BREAKTHROUGH-PROGRAM.md` §10 I10-1C), and the mode-17 cross-product
   `transform ∈ {0,1,2} × backend ∈ {brotli,bwt}` already exists in production code
   (`src/anvil.cpp:4614-4639`) yet produced **no** ratio-transform win on the frozen
   corpora (`docs/I10-REMOTE-BASELINE-CLOSURE.md` lines 32-37: "No new ratio-transform
   win appeared in the baseline itself"). Therefore: for this project the *backend layer
   is the mechanism layer*. Novelty effort spent on frontend×Brotli cells is largely
   redundant with a backend the project already ships.

2. **The binding Pareto deficits are decode latency and peak RSS, and both are
   measured, localised, and — for decode — largely unexploited.** `[M]` Silesia: ANVIL aux
   46,466,339 B at 4.2015 s whole-decode (≈50.4 MB/s derived) vs Brotli q11/lw30
   49,383,136 B at ≈166.9 MB/s vs xz-9e 48,456,004 B at ≈82.5 MB/s; ruling `FRONT-GAP_COST`
   (`RESEARCH_LEDGER.md` PART XV; `docs/I10-AUX-UNBWT-RESULTS.md` §11).

3. **Decode is a serial DRAM-latency chain at ≈1 dependent round trip per output byte,
   and the decoder already pays for ~600-800 independent chain segments that it walks
   one after another.** `[M+arith]` Measured BWT-direct decode is 84.8-88.8 ns/byte on
   10-41 MB inputs (`tests/bwt-backend-standard.csv`), and measured decode peak RSS is
   5.99-6.35× source — i.e. the 6n working set `T(n) + L(n) + ISA(4n)`. The auxiliary
   index already transmits the segment entry points at **4 bytes per ≈2^16 output bytes
   (2.5-3.1 KB measured)**, and my cost model reproduces all three measured wire deltas
   to ±2 B (§4.3). Those segments are consumed **sequentially**, so none of the
   memory-level parallelism is used.

4. **Strongest mechanism: Walk-Segmented Interleaved inverse-BWT (WSI-BWT) — decode the
   `≈1024` already-transmitted LF-walk segments `k` lanes at a time so the `k` dependency
   chains overlap.** It needs **zero new wire bytes** (`k` is a decoder-side constant),
   ~8·k bytes of decoder state, and ~30-60 lines of decoder code. It is **ADOPT-CLASS /
   NO NOVELTY CLAIM** — and that is the correct classification, not a hedge.

5. **The decisive arithmetic: 3.33× is exactly the factor that puts ANVIL's byte count at
   Brotli's decode rate.** `[M+arith]` Control = 16 MiB-tiled aux Silesia, 4.2232 s
   (`docs/I10-BREAKTHROUGH-PROGRAM.md` §10 I10-1A.2). Required to reach 166.9 MB/s:
   211,938,580 / 166.9e6 = 1.2700 s ⇒ candidate/control ratio ≤ **0.3007**. Bytes at that
   point 46,938,837 B = **−4.94% vs Brotli**; RSS 125.7 MiB = 1.01× Brotli, 2.31× xz.

6. **Two candidate "novel" backend mechanisms are analysed and retired with reasons**
   (§7): walk-order (fused, reverse) postcoding is *dominated* on the decode axis by
   interleaving (mutual exclusivity of serial-fusion and parallel-latency-hiding), and
   any attempt to break the 6n working set without tiling is a **math-class dead end**,
   because inverse BWT needs Ω(n) *randomly addressed* state and marginal information
   given `L` is negligible.

7. **Current leaning: PROMOTE-TO-REMOTE** for WSI-BWT as a pre-registered, remote-only,
   paired A/B pilot (`KILL / HOLD / PILOT / PROMOTE-TO-REMOTE` → **PROMOTE-TO-REMOTE**),
   with `HOLD` on the two novel candidates pending the control result, and `KILL` on
   zstd-as-ratio-backend and on CM/PPM/neural-as-backend. No prototype code is proposed
   (rationale in §8) — the change is a scheduling rewrite of an existing decoder call,
   and the only valid evidence is a paired remote benchmark.

---

## 1. Evidence map (measured facts, exact provenance)

Timing-series discipline: numbers are **never spliced** across series. Series labels used
below: **[REM10]** = Linux GitHub-Actions I10 remote series (`docs/I10-REMOTE-BASELINE-CLOSURE.md`,
`docs/I10-AUX-UNBWT-RESULTS.md`); **[WIN-I9]** = Windows I9 committed grid
(`tests/pareto-verdict.csv`, `tests/xz-reference-i9.csv`, `tests/auto-routing.v2.csv`);
**[LOCAL-ABL]** = local ablation CSVs (bytes authoritative, timings ranking-grade only).
`tests/auto-routing.v2.csv` rows carry `timing_status=VALID_100MS_NONRECURSIVE` and are
**explicitly excluded from all throughput claims** (100 ms timer granularity).

| # | Fact | Value | Source |
|---|---|---|---|
| E1 | Frozen ANVIL auto-direct, Silesia / enwik8 | 46,446,995 B / 23,534,368 B | `docs/I10-REMOTE-BASELINE-CLOSURE.md` §Canonical runs **[REM10]** |
| E2 | Frozen references, Silesia | Brotli q11/lw30 **49,383,136**; xz-9e **48,456,004**; **zstd ultra-22 long27 52,364,240** | same **[REM10]** |
| E3 | Frozen references, enwik8 | Brotli 24,810,180; xz 24,831,648; zstd ultra-22 long27 25,272,471 | same **[REM10]** |
| E4 | ⇒ ANVIL byte lead | Silesia **−5.94%** vs Brotli, **−4.14%** vs xz, **−11.30%** vs zstd-ultra-22-long27 | derived from E1/E2 **[M+arith]** |
| E5 | Canonical frontier state | `33 non-dominated / 5 FRONT-GAP / 0 FRONT-CROSSING / 28 DEGENERATE`; Silesia `FRONT-GAP_COST`, enwik8 `TIMING_BLOCKED` | `RESEARCH_LEDGER.md` PART XV §Established facts **[REM10]** |
| E6 | Aux-index paired whole-decode speedups / charged wire | dickens 1.364× (+2,494 B), webster 1.795× (+2,535 B), enwik8 2.339× (+3,054 B); +8,083 B total | `docs/I10-AUX-UNBWT-RESULTS.md` §6 **[REM10]** |
| E7 | Whole-codec decode & RSS vs external refs (Silesia) | ANVIL 46,466,339 B ≈47.9-50.4 MB/s, **248.4 MiB**; xz 48,456,004 B ≈82.5 MB/s, 54.3 MiB; Brotli 49,383,136 B ≈166.9 MB/s, 124.5 MiB; ratios ANVIL/xz **1.7226**, ANVIL/Brotli **3.4474**, RSS **4.572× xz**, **1.995× Brotli** | `docs/I10-AUX-UNBWT-RESULTS.md` §11 **[REM10]** |
| E8 | enwik8 decode/RSS | ANVIL 23,537,422 B ≈26.5 MB/s, **598.9 MiB (9.043× xz, 2.378× Brotli)**; ANVIL/xz **3.9065**, ANVIL/Brotli **5.6642** | same **[REM10]** |
| E9 | BWT-routed files | 7/12 Silesia files route to BWT; 0 route changes under aux | `docs/I10-AUX-UNBWT-RESULTS.md` §8 **[REM10]** |
| E10 | BWT-direct per-file decode latency | dickens 0.905580 s / 10,192,446 B = **88.8 ns/B**, 64.734 MiB (**6.35×**); webster 3.517194 s / 41,458,703 B = **84.8 ns/B**, 248.203 MiB (**5.99×**); mozilla 4.517467 s = 88.2 ns/B, 314.152 MiB (6.13×); x-ray 83.1 ns/B | `tests/bwt-backend-standard.csv` **[LOCAL-ABL, ranking-grade]** |
| E11 | BWT subblock rate/decode/RSS curve (aux, Silesia) | 128 MiB: 46,466,339 B / 4.2015 s / 248.4 MiB · 32 MiB: 46,648,642 B (+0.3923%) / 4.2104 s / 203.4 MiB · **16 MiB: 46,938,837 B (+1.0169%) / 4.2232 s / 125.7 MiB** · 8 MiB: 47,365,274 B (+1.9346%) / 3.7022 s / 124.4 MiB · 64 MiB: byte-identical, internally dominated | `docs/I10-BREAKTHROUGH-PROGRAM.md` §10 I10-1A.2 **[REM10]** |
| E12 | ⇒ 16 MiB tiling is nearly free | +1.0169% bytes, **+0.52% decode time**, **−49.4% peak RSS** | derived from E11 **[M+arith]** |
| E13 | Backend registry in production | `kRatioBackendBrotli=1`, `kRatioBackendBwt=2` (`src/anvil.cpp:3856-3857`); `ratio_backend_ids` (`:4400-4427`); `encode_ratio_block` considers transform 0/1/2 × every enabled backend (`:4614-4639`) | source read |
| E14 | Ratio block size | `opt.block_size = 128 MiB` when `--parse=ratio` and not explicit (`src/anvil.cpp:4999-5000`) ⇒ **one BWT per Silesia/enwik8 file** ⇒ cross-block context is **not** an available lever on the canonical corpora | source read **[M]** |
| E15 | Transform 3 (LZP residue) | hard-disabled, do-not-reburn (`src/anvil.cpp:4630-4634`, `docs/audit-2026-09-07/06-do-not-reburn.md` §G2) | source read |
| E16 | Aux index wire policy | `kBwtAuxTargetWalks = 1024` (`src/anvil.cpp:3913`); `r = 2^ceil(log2(n/1024))`, `icount = 1+(n-1)/r` (`:4247-4274`); wire = tag + uvar(r) + uvar(icount) + icount×u32le (`:4312-4320`); `kBwtAuxMaxIndexes = 1<<20` | source read |
| E17 | G-lane backend pinning | G0-G3 all use "the same Brotli q11/lgwin30"; G1 four-family routed aggregate **−1.6564%** (NO-GO gate −3%); G2 **−2.007746%** (NO-GO) with D2 CDISC **−16.4849%**, D4 CROVIA **−22.5029%** vs *raw Brotli* | `docs/I10-BREAKTHROUGH-PROGRAM.md` §10 I10-1C **[REM10]** |
| E18 | R-front rate gap | PAQ-class public Silesia totals ≈28 MB vs ANVIL I9 ≈46.4 MB | `docs/I10-BREAKTHROUGH-PROGRAM.md` §1 |
| E19 | Executable-family gap | `tests/pareto-verdict.csv`: on `pe-git.exe` ANVIL best ratio 0.408 vs brotli-q11 0.329 (**+24.01%**) — ANVIL is *far behind* on binaries **[WIN-I9]** | `tests/pareto-verdict.csv` |
| E20 | Dense-frontier vehicle (frozen, undispatched) | reference class = Brotli q1/q4/q6/q9/q11 lg30 + zstd 1..22 + zstd ultra-22/long27 + xz-9e; panel = dickens, mozilla, nci, webster, xml; reps ≥7, warmups 2, seed 41246, eps 2%, arm CV ≤15%, ambient CV ≤10%, affinity CPU0, A/A null must span 1.0 | `docs/I10-DENSE-FRONTIER-PREREG.md` §3-§8 |
| E21 | Gate vocabulary | dual-bar (raw + transform-enabled) binding on synthetic cells; `GRID-THIN`/`NON-DEFAULT` labels mandatory; byte-only win is `BYTE-WIN`, never `P-CROSSING` | `docs/gate-ruling-i9-recon-crossing.md` R-2/R-5; `docs/I10-DENSE-FRONTIER-PREREG.md` §1 |
| E22 | Precedent for the measurement failure mode to avoid | G4's published speed ratios withdrawn as `INVALID_SPEED_ACCOUNTING` because the q11 label denominator read cached labels while label construction was timed separately | `RESEARCH_LEDGER.md` PART XV §G4 |

---

## 2. Where the ceiling actually is (the load-bearing reframing)

ANVIL has three structural backends' worth of behaviour and one shipped backend that wins
bytes. Decomposing the canonical Silesia result:

* ANVIL total 46,466,995 B **[M, E1]**.
* The BWT backend wins on 7/12 files **[M, E9]**; on the 5 non-BWT files ANVIL is at or
  near Brotli (e.g. mozilla ANVIL 13,806,173 vs Brotli 13,806,141 — 32 B apart **[M, E7
  table / dense-prereg §6]**).
* So the entire −5.94% byte lead is produced by **one backend on 7 files**, and it is
  purchased with 3.45× Brotli decode time and 1.995× Brotli peak RSS **[M, E7]**.
* The transforms that win big against *Brotli* (E17: −16% to −22% on two of four G
  families) **did not** win against the BWT backend, because mode 17 already offered
  exactly that cross-product on the canonical corpora and it produced nothing **[M, E13,
  E1-doc §32-37]**.

**Interpretation [P]:** the G-lane's measured "typed structure" gains and the BWT backend's
gains are largely the *same* redundancy seen twice. BWT is close to invariant to column
permutation within a block (repeated rows stay repeated; rotation-sorting reorganises but
does not destroy repetition), which predicts that `transform × BWT` stays null — a
prediction the existing cross-product already supports. **Consequence for track 05: a
novel *frontend* cell aimed at the BWT backend has a low prior probability of adding
bytes. A novel *backend-schedule* cell does not, because nothing in the corpus constrains
decode scheduling at all.**

---

## 3. Backend landscape beyond Brotli (adopt / kill / oracle-only), with the required novelty line

Notation: **bytes/decode** quoted only where measured in a citable series.

| Candidate backend | Measured standing | Class | What must be novel / what must not be re-claimed |
|---|---|---|---|
| **BWT family** (libsais + 5 postcoders, `--bwt-aux`) | best measured bytes (E1-E4); 50.4 MB/s, 6n RSS (E7,E10) | **ADOPT — the incumbent rate engine** | Nothing about BWT+MTF+RLE+arith is novel (bzip2/divsufsort/xz). Novelty must live in schedule, tiling policy, or postcoder *interaction* |
| **zstd ultra-22/long27 as ANVIL ratio backend** | **52,364,240 B** = 11.30% *worse* than ANVIL's own portfolio (E2,E4) | **KILL as ratio backend** | — Wrapping zstd adds a reference codec as an internal arm while losing bytes on the axis the project is trying to win. (DEFLATE *replay*, by contrast, stays adopt-class: bounded 2,289-stream/2.86 MB ZIP population per `docs/I10-BREAKTHROUGH-PROGRAM.md` §10 I10-1B.) |
| **xz / LZMA / LZMA2 replay** | in reference class (E2,E3) | **KILL as new arm** | Already the bar; replaying it inside ANVIL adds decoder code size for a known point |
| **BWT + context-mixing postcoder** (libbsc/PAQ-BWT class) | not measured here | **KILL for novelty; optional adopt for rate** | `[B-belief]` BWT→CM is an established research family; novelty here would be unavailable. If adopted it must be charged with *lost MTF/RLE structure*, and it makes decode slower — adverse on the binding axis |
| **PAQ / CM / CTW / neural (CMIX-class)** | ≈28 MB Silesia (E18) vs 46.4 MB — a ~40% rate gap | **ORACLE-ONLY (kill as backend)** | Rate gap is real and large, but closing it needs per-byte model updates (decode 1-20 MB/s). Falsifiability rule 11 in `docs/I10-BREAKTHROUGH-PROGRAM.md` §11 forbids it |
| **Grammar / SLP / RLZ / bidirectional-copy backends** | project-measured adverse (`anvil-hotop-rlzp` 0.048 MB/s encode, retired-as-default; `docs/gate-ruling-i9-recon-crossing.md` R-3) | **KILL** | Dependency depth + encode collapse; re-opened only by tracks 11/15 as representation work, not backend |
| **Static/transmitted dictionary (G5D paged base+overlay)** | promoted to remote pilot already (`RESEARCH_LEDGER.md` PART XV §Portfolio) | **ADOPT (track 01 owns)** | Dictionary *transport* is prior art (FSST, zstd CDict, Apple's typed dicts); not a backend novelty |
| **Typed-structure graph codecs** (OpenZL/DataCortex/Corra/FastLanes/ALP/BtrBlocks) | prior art per `docs/I10-BREAKTHROUGH-PROGRAM.md` §10 I10-1C closing note | **ADOPT as architecture; NO novelty at component level** | Novelty must live at a higher explanation/compiler level (already the binding ruling for PORDER) |
| **Walk-schedule engineering of the incumbent BWT** (WSI-BWT, §4-§6) | not measured; headroom derived from E10/E16 | **PILOT — ADOPT-CLASS, NO NOVELTY CLAIM** | Nothing to claim; the value is a Pareto crossing |
| **Walk-order (reverse, fused) postcoder** | — | **RETIRED for decode; retain as memory-only variant** (§7.1) | Dominated on decode by WSI-BWT; would need a novelty separator that does not currently exist |
| **Sub-Ω(n)-space / externally-partitioned inverse BWT** | — | **MATH-CLASS DEAD END as a rate-preserving replacement** (§7.2) | Any rate-preserving alternative must still carry Ω(n) randomly addressed state |
| **Explicit-transmission of the LF permutation instead of primary index** | — | **KILL (information-neutral)** | `(L, σ)` and `(L, primary)` determine the same text; the extra permutation is not cheaper to model than what MTF/RLE/arith already exploits |

---

## 4. PRIMARY MECHANISM — Walk-Segmented Interleaved inverse-BWT (WSI-BWT)

### 4.1 Statement of the mechanism (exact)

**One-line:** the auxiliary LF-walk index already on the wire splits the inverse-BWT walk
into `S = 1 + ⌊(n-1)/r⌉` independent segments at ~4 bytes each; WSI-BWT advances **`k` of
those segments simultaneously in one pass of the decode loop**, so `k` DRAM dependency
chains are in flight instead of one.

**Why it is a mechanism and not a micro-optimisation [M+arith + P]:** the measured decode
rate is 84.8-88.8 ns per output byte (E10). A dependent DRAM load on the measured host
class costs ≈80-95 ns. The loop's recurrence is

```
next(p) = ISA[p] − 1          (libsais backward convention; ISA is 4 B/entry)
```

so per output byte the loop performs *one dependent* random load from a table of size
`4n`, plus a random byte read from `L` and a random byte write into `T`. The recurrence
makes the loop **latency-bound, not bandwidth-bound**: total time ≈ `n · L_DRAM`, and no
amount of added bandwidth helps. Segmenting the walk removes the recurrence *across*
segments while preserving it *within* a segment — which is precisely the standard
memory-level-parallelism argument, and precisely why the segment entry points are worth
transmitting at all.

### 4.2 Precise encoder / decoder state machine

**Encoder (unchanged except one parameter).** For a BWT block of decoded length `n`:

```
S0: build BWT of the block; postcode with the existing postcoder registry
    (ids 0 static-MTF, 1 arith-o0, 2 arith-o1, 3 raw-BWT, 4 QLFC)  [src/anvil.cpp:4222-4310]
S1: if n == 1 -> legacy v1 payload (already special-cased)
S2: r = 2^ceil(log2(max(1, n/1024)))          # existing policy, src/anvil.cpp:4247-4256
S3: indexes = libsais_bwt_aux(...)            # indexes[0] == primary (1-based, <= n)
S4: emit v2 payload: 0xFE, uvar(r), uvar(S), uvar32le(indexes[j]) for j in 0..S-1
```
WSI-BWT adds **no new field**. `k` is a decoder constant. Optionally the encoder may
choose a *smaller* `r` (denser segments) as a Pareto-budgeted trade (§6.4), never a larger
one; that is the only format-visible knob and it is off by default.

**Decoder state machine (the whole mechanism).**

```
decode_aux_v2(bwt_payload, n, out):
  0xFE tag; r = uvar; S = uvar; I[0..S-1] = uvar32le array
  hard bounds (already present, src/anvil.cpp:4349-4366):
      1 <= I[j] <= n ; S == 1 + (n-1)/r ; S <= expected ; 4*S <= remaining bytes
  ISA = inverse-BWT working structure of size n (4n B)     # unchanged
  K = min(k_max, S)                                       # k_max is a build constant
  t[j] = segment j's output cursor;  p[j] = I[j]           # j = 0..K-1
  while any segment active:
      for j in 0..K-1 while t[j] < seg_end(j):
          out[t[j]] = L[p[j]]
          p[j] = ISA[p[j]] - 1        # successor; convention pinned by libsais_unbwt
          t[j] += 1
      retire lane j when t[j] == seg_end(j); pull lane K-1 forward from I[next_segment]
  assert sum(seg_len(j)) == n and no byte of `out` written twice
```

State/registers per lane: `(p, t)` = 8 bytes ⇒ **`8k` bytes of decoder state**, k ≤ 32 ⇒
≤ 256 B. Segment ends are derivable: `seg_end(j) = I[j+1]` (last segment ends at `n`), so
**no additional index bytes**.

Correctness invariants (must be preregistered as remote assertions):
1. `⋃_j [seg_start(j), seg_end(j)) = [0, n)` and segments are disjoint ⇒ every output
   byte is written exactly once. Because segments are consecutive *walk* intervals, this
   holds by construction of the libsais auxiliary index, but the pilot must assert it
   (e.g. write-then-verify on a canary pattern) rather than assume it.
2. Lane reordering must not change output bytes: `out` is a pure permutation of `{L[]}`
   assignments, so interleaving cannot alter content — **but a lane whose segment
   overlaps another would silently corrupt**; hence invariant 1 is the load-bearing gate.
3. Malformed input: a forged `I[]` that makes segments not tile `[0,n)` must be rejected
   **before** allocation, exactly as the current sub-block framing does
   (`src/anvil.cpp:4479-4492` precedent).

### 4.3 Byte cost model (full, with the transmitted part)

`b_aux(n) = 1 + uvar(r) + uvar(S) + 4S`, `S = 1 + ⌊(n-1)/r⌉`, `r = 2^ceil(log2(n/1024))`.

| n (decoded bytes) | r (predicted) | S (predicted) | 4S (predicted) | measured delta | source |
|---:|---:|---:|---:|---:|---|
| 10,192,446 (dickens) | 16,384 | 623 | 2,492 B | **+2,494 B** | E6 |
| 41,458,703 (webster) | 65,536 | 634 | 2,536 B | **+2,535 B** | E6 |
| 100,000,000 (enwik8) | 131,072 | 764 | 3,056 B | **+3,054 B** | E6 |

Model reproduces all three measured points within **±2 B** (the residual is the 3-4 framing
bytes). **[M+arith]** ⇒ the wire cost of segmentation is *independent of n* and bounded by
`4·1024 = 4 KiB` plus ≤ 3 framing bytes for any `n ≤ 2^31`.

**WSI-BWT's own byte cost: 0.** `k` is a decoder constant; no wire change, no format
revision, no new backend ID, no new transform ID. Total measured-basis charge for the whole
pilot point (16 MiB tiling + aux + interleaving) = **+1.0169%** vs the frozen legacy
default, of which 100% is the *tiling* trade (E11/E12) and 0.0072% is the aux index
(E6). This is the cost model the prereg must charge — **the mechanism itself is free and the
budget it rides on is a measured trade, not a novelty purchase.**

### 4.4 Cycle / throughput cost model

Per output byte the loop issues: 1 dependent 4-B random load (`ISA`), 1 random 1-B load
(`L`), 1 random 1-B store (`T`), plus loop/compare overhead. Model:

```
t_byte(k) = max( L_DRAM / k , BW_demand / B_eff , t_fixed )  +  t_lane_overhead
BW_demand = 6 bytes/output (4 B ISA + 1 B L + 1 B T, ignoring ISA write-back)
t_lane_overhead = c · k  (register pressure, bounds checks, loop bookkeeping)
```

Anchors: `t_byte(1) = 84.8-88.8 ns` measured (E10) ⇒ `L_DRAM ≈ 80-88 ns` on the runner
class. **[P]** With `k = 8`, latency term ≈ 10-11 ns; the binding term becomes DRAM
*bandwidth* for random access (prior-knowledge single-core random-access bandwidth on
EPYC-class parts ≈1.5-3 GB/s ⇒ 250-500 MB/s ceiling for 6 B/output), plus TLB pressure from
8 concurrent random streams. **[P]** Therefore the realistic k=8 target band is
**150-300 MB/s**, and the *decisive* threshold is the 3.33× line in §6, not the ceiling.

Explicitly **not** claimed: that interleaving reduces memory traffic (it does not — footprint
stays `T(n)+L(n)+ISA(4n)`), nor that it helps encode at all (encode untouched), nor that it
helps blocks whose segments are degenerate (§8).

### 4.5 Memory / RSS cost model

`RSS(n) ≈ 6n + O(1)` for whole-block BWT — verified against E10 (5.99-6.35×). WSI-BWT adds
`8k` bytes and **zero** new allocation; it must not add a second `L` copy (that would be
+1n = +17%). With 16 MiB tiling the measured RSS is 125.7 MiB vs 248.4 MiB whole-block
(E11), i.e. **−49.4% for +1.0169% bytes and +0.52% decode time** **[M+arith]**.

**Projection [P], stated as a projection:** the RSS axis cannot be brought inside the
project's 2× margin against xz (Silesia: 125.7/54.3 = 2.31×; enwik8 whole-block 598.9/66.2 =
9.04×) by any whole-block exact-BWT arrangement, because inverse BWT needs Ω(n) randomly
addressed state (§7.2). The honest expectation is therefore a ruling of
`FRONT-GAP_RSS` versus xz with a genuine `EXTENDS_FRONT`/dominance versus the *Brotli-complete*
class. **The pilot must be preregistered to be able to return that mixed verdict** — it must
not be preregistered as "all-or-nothing crossing", or a real win will be recorded as a failure.

### 4.6 Decoder code / binary-size cost

**Projection [P]:** 30-60 lines of C++ in the existing `ratio_backend_decode`/`bwt` path
(lane array, inner 2-loop, retire/pull) ⇒ **+1.0 to +2.5 KiB `.text`**, i.e. the same order
as the measured aux index (`+2,976 B .text`, `+8,192 B` stripped ELF, E6 source
`docs/I10-AUX-UNBWT-RESULTS.md` §9). Combined CLI stays ≈461 KiB against AITDCC's 1 MiB
decoder-size cap. Charge: **≤ +4 KiB `.text` is a GO condition**, not a hope.

---

## 5. Novelty / prior-art risk (WSI-BWT and the two retired candidates)

**WSI-BWT: novelty = NONE, and that is the correct answer.** `[B-belief]` Any parallel BWT
decoder (OpenMP/threaded bzip2-style decoders, GPU BWT decoders, chunked/segmented inverse
BWT) obtains latency hiding the same way; `libsais_unbwt_aux` itself already creates the
segments. What is *novel to ANVIL* is only the observation that the segments are being
consumed sequentially and that ~600-800 of them are already paid for. **Do not let any
report describe this as a mechanism-novelty contribution** — its value is entirely in the
Pareto column, and the coordinator should fund it as engineering with a hard measurement
gate.

**Retired candidate 1 — walk-order (reverse/fused) postcoder.** `[P]` Coding the BWT string
in LF-walk order would (a) eliminate the `L` buffer (−1n = −17% RSS) and (b) allow the
entropy decoder to overlap the pointer chase, and (c) expose *right* context to the model.
It is **retired for the decode axis** because the two designs are mutually exclusive: an
arithmetic/rANS decoder is inherently serial, so fusing it into the walk forbids
interleaving, while interleaving is worth up to 8× and fusing is worth at most the
postcoder's share of the critical path (≈5-10% [P], from `bwt_postcoder_payload` being a
sequential stage over `n` bytes). Retained only as a **memory-only** variant to be
reconsidered if RSS becomes the sole gate. Its rate risk is also structural: MTF/RLE
structure is defined in *string* order and is destroyed in walk order.

**Retired candidate 2 — sub-Ω(n)-space inverse BWT.** `[P, math-class]` Exact whole-block
inverse BWT is a random walk over row space; any decoder must therefore hold Ω(n) randomly
addressable state. The *marginal* information of the ISA given `L` is negligible (the text
determines everything), so the ISA cannot be replaced by anything smaller that is still
randomly addressable; the only smaller structures are (i) lossy in time (partitioned /
multi-pass external-memory BWT — time cost ≫ rate cost) or (ii) lossy in rate (tiling —
which ANVIL already has as a measured curve, E11). **Conclusion: the memory axis is a trade,
not a cleverness target.** Anyone proposing "clever sublinear inverse BWT" in a later swarm
should be sent to this section.

**Residual prior-art exposure for WSI-BWT:** none material. The one thing a reviewer can
attack is *"libsais_unbwt_aux already claims to do this"* — it does not: it walks segments
one at a time (the segment index exists to allow *restart*, not concurrency). **The pilot
must therefore include a control arm that proves sequentiality**, e.g. report both
whole-Silesia decode time and a *segment-restart-only* arm from the same binary, so the
claimed factor cannot be attributed to the index alone.

---

## 6. Asymptotics, projections, and the decisive arithmetic

**Asymptotics [P].** Decode cost stays `Θ(n)` time but moves from `Θ(n·L_DRAM)` (latency-
bound) to `Θ(n·max(L_DRAM/k, BW⁻¹))` (bandwidth-bound), i.e. the achievable constant
improves by up to `min(k, BW·L_DRAM/B)` where `B = 6 B/output`. Nothing changes in the
asymptotic memory complexity (`6n`) — consistent with §4.5. For tiled blocks of size `T`,
decode cost is `Σ_tiles T·max(L_DRAM/k, BW⁻¹)` and rate cost is the measured tiling curve
(E11), which is *sublinear in n* (constant per block) — so the trade is byte-neutral in the
limit and only bites on the corpus as configured.

**The decisive line [M+arith].** Control = the frozen aux 16 MiB Silesia point
(46,938,837 B, 4.2232 s decode, 125.7 MiB — E11), source 211,938,580 B
(`docs/I10-DENSE-FRONTIER-PREREG.md` §4).

| quantity | value |
|---|---|
| required decode to match Brotli q11/lw30 | 211,938,580 / 166.9e6 = **1.2700 s** |
| required candidate/control ratio | 1.2700 / 4.2232 = **≤ 0.3007 (3.33×)** |
| complete bytes at that point | 46,938,837 = **−4.94% vs Brotli**, −3.13% vs xz |
| peak decode RSS at that point | 125.7 MiB = **1.01× Brotli**, 2.31× xz |
| enwik8 analogue (whole-block aux, 3.538773 s, 598.9 MiB) | to match Brotli's 150.2 MB/s needs **0.6658 s ⇒ ratio ≤ 0.1881 (5.32×)** — *not reachable; enwik8 is expected to stay `FRONT-GAP_RSS` even at GO* |

**Why enwik8 is the honest stress case [M+arith]:** enwik8 needs 5.32×, Silesia needs 3.33×,
and enwik8's RSS is 9.04× xz. A preregistration that promises both corpora would be
promising something the mechanism cannot deliver. **Split gate: Silesia is the promotion
corpus; enwik8 is a reported non-blocker.**

---

## 7. Why not a novel backend mechanism *now* (the honest negative space)

1. **Entropy-backend novelty is track 06's territory and is saturated** (context clustering
   is already ruled non-novel in this project's own ledger: `RESEARCH_LEDGER.md` Experiment
   J, PART II §3; Brotli's own context map is the cited prior art).
2. **Transform/backend novelty is occupied** (E17 + PORDER killed in
   `RESEARCH_LEDGER.md` PART XV §Portfolio).
3. **Rate-vs-decode is not a Pareto gap but a modelling law for this project:** the
   R-front is ≈40% better in bytes and 1-20 MB/s in decode (E18); whole-block exact BWT is
   already at the *local optimum* where "unbounded previous-occurrence context" costs one
   dependent random access per byte. **Any representation that must maintain long-range
   state online (a decoder-built hash chain, a CM model, a dictionary index) costs the same
   order or more per byte while modelling strictly less** ⇒ it is dominated before
   measurement. That argument is why I do not propose "BCT without BWT".
4. What remains genuinely open in the backend layer is exactly the two things this report
   quantifies: **decode schedule** (§4) and **the rate/RSS trade curve** (E11). Both are
   engineering. **The correct coordination decision is to fund the engineering with a hard
   gate and move mechanism-novelty budget to tracks 09/10/11/14, where ANVIL's unique
   assets (position-derived transforms, invariant anchoring, typed lanes) actually are.**

---

## 8. Minimum prototype, adversarial cases, and the one decisive remote experiment

**Minimum prototype.** Deliberately **none in this repo.** WSI-BWT is a ~40-line
rewrite of one decode loop over an index the codec already emits; a local prototype could
only measure it *locally*, which is forbidden and would be worthless as evidence (all
load-bearing timing is GitHub-Actions-only per MASTER-BRIEF doctrine 7). The correct
minimum artefact is therefore a **workflow-only vehicle** modelled on
`.github/workflows/anvil-i10-dense-frontier.yml` + `docs/I10-DENSE-FRONTIER-PREREG.md`,
plus one new decoder build flag (`--bwt-lanes=K`) in a *frozen candidate branch*. Nothing
in `src/anvil.cpp` is edited for this report.

**Adversarial / failure cases that must be in the prereg:**

| # | Adversarial case | Why it bites | Required handling |
|---|---|---|---|
| A1 | Single-cycle LF walk (e.g. long constant runs, `sao`-like files) | segments become degenerate ⇒ `k_eff = 1` ⇒ zero gain, possibly regression | instrument `k_eff = distinct(I[j])`-derived effective lanes; block-level NO-GO → serial fallback; report `k_eff` per file |
| A2 | Small blocks (`n` ≪ 1 MiB, e.g. `--block=256K` rev-1) | 4 KiB index on a 256 KiB block = 1.6% wire | aux already competes per-payload; assert the "smallest payload" rule still selects correctly |
| A3 | `k` too large (16-32) | 8-32 concurrent random streams ⇒ TLB thrash ⇒ **slower than control** | full k-sweep {2,4,8,16,32}; any regression is a reported NO-GO row, not a deleted outlier (E20 §8) |
| A4 | Memory-constrained runner | interleaving must not add a second `L` copy | RSS is a preregistered gate (±2%) and `.text` is gated at +4 KiB |
| A5 | Forged/hostile `I[]` | non-tiling segments ⇒ silent double-write into `out` | reuse existing bounds discipline + assert segment tiling before the first store; fuzz on remote only |
| A6 | **Decompression-bomb amplification** | BWT decode costs `6n` RSS / ~90 ns/B regardless of compressed size; a 1 MiB→4 KiB payload still costs 6 MiB and 90 ms | cap/measure; cross-reference track 19. This is an *adversarial property of the incumbent backend*, not of WSI-BWT |
| A7 | Splicing | E10 `[LOCAL-ABL]` timings must not be compared against E7 `[REM10]` ratios | all pilot comparisons are same-job paired only (E20) |
| A8 | The G4 accounting trap (E22) | reading cached labels while timing their construction | the pilot times **exactly** the same code path on both sides; the segment-restart-only control arm (§5) is timed, not assumed |

**The one decisive REMOTE-ONLY experiment (preregistered).**

*Arms (single job, single toolchain, one thread, `taskset -c 0`, E20 §3):*
`A0` = frozen aux, 16 MiB cap, `--bwt-lanes=1` (serial control) ·
`A1..A5` = same binary, `--bwt-lanes ∈ {2,4,8,16,32}` ·
`C0` = frozen **legacy default-off** byte-identity control (must reproduce
46,466,995 B / 23,534,368 B exactly) ·
`C1` = segment-restart-only arm (proves the factor is concurrency, not the index).

*Corpus:* Silesia timing panel {dickens, mozilla, nci, webster, xml} (E20 §4); full-corpus
byte/RSS census; enwik8 reported as a non-blocker (§6).

*Protocol:* ≥7 paired reps, 2 warmups, seeded interleaved AB/BA seed 41246 + per-file offset,
20,000 bootstrap samples, 2% practical epsilon, arm robust CV ≤15%, ambient robust CV ≤10%,
A/A null CI must span 1.0, per-rep output SHA-256 verified outside the timed interval.

*Pre-registered thresholds (frozen before any run):*

* **PROMOTE (GO):** Silesia paired decode ratio `A_best/A0` point ≤ **0.28** with bootstrap
  95% CI upper ≤ **0.30** (= 3.33×, the Brotli-parity line of §6), **and** complete bytes
  increase ≤ **0.02%** vs `A0`, **and** peak decode RSS increase ≤ **2%**, **and** `.text`
  increase ≤ **4 KiB**, **and** `C0` byte-identity PASS, **and** A/A null spans 1.0, **and**
  no k-row regresses below 0.95×.
* **HOLD:** best ratio in (0.30, 0.50] with all other gates green ⇒ one preregistered
  follow-up allowed only for TLB/hugepage mitigation; **thresholds may not move**.
* **KILL (NO-GO):** best ratio > **0.50** after the full k-sweep; **or** any byte increase
  > 0.05%; **or** RSS increase > 5%; **or** `.text` > +4 KiB; **or** `k_eff < 2` on more
  than 20% of panel blocks; **or** any correctness/fuzz/malformed-input failure; **or**
  `C1` (restart-only) reproducing the full factor (⇒ the mechanism is mis-attributed).
* **Classification on GO:** expected `EXTENDS_FRONT` vs the Brotli-complete class with
  `FRONT-GAP_RSS` vs xz. **A GO must not be recorded as a failure for the RSS margin**, and
  the run must never be reported as `P-CROSSING` without the dual-bar and label checklist
  of E21.

---

## 9. Strongest disconfirming evidence against my own leaning

1. **The serial-chain diagnosis is an inference, not a measurement.** E10 gives ~85 ns/B;
  I attribute it to one DRAM round trip. If the real limiter is instead TLB misses, page-table
  walks, or libsais's internal loop structure, interleaving yields far less. **Mitigation
  built into the prereg:** `C1` control plus the full k-sweep plus RSS/TLB reporting. This
  is the single most likely way my recommendation is wrong.
2. **`libsais_unbwt_aux` may already interleave internally.** If so the gain is partly or
   wholly pre-existed and my "sequential consumption" claim is wrong. **Mitigation:** the
   pilot measures `A0` against `C1` explicitly; if they are equal, the mechanism is dead and
   the report's central claim must be struck.
3. **The 3.33× target may be unreachable at any k** because the loop becomes TLB/bandwidth-
   bound near 100-150 MB/s. Then Silesia lands at ratio ≈0.35-0.45 ⇒ `HOLD`, and the
   project should spend the next budget on tiling-rate recovery instead (§7.2 note) or on
   tracks 09/10.
4. **"BWT is transform-invariant-ish" is my hypothesis, not a measurement.** If some typed
   transform *does* win against BWT on the large G families, then §2's conclusion inverts
   and frontend×backend work becomes the higher-EV direction. **Cheap decisive test that
   should be run by tracks 01/02, not by me:** run the frozen G2 typed basis through
   `kRatioBackendBwt` on the sealed G families at bytes-only cost (R1 oracle tier,
   `docs/I10-BREAKTHROUGH-PROGRAM.md` §9) — no timing, no new workflow, and it either
   confirms or kills §2.
5. **Peak RSS could be the only gate that matters** to the coordinator, in which case a
   decode-only win is worthless. Counter-evidence: the project's own ruling names decode
   *and* RSS as the binding deficits (E7/E8) and `FRONT-GAP_COST` was the Silesia ruling;
   the enwik8-only view would make even the measured aux adoption (E6) valueless, which the
   project already accepted as a Pareto point. Medium risk, accepted.

---

## 10. Disposition table and final recommendation

| Item | Disposition |
|---|---|
| WSI-BWT (walk-segmented interleaved inverse BWT, 16 MiB tile) | **PROMOTE-TO-REMOTE** — frozen remote paired pilot, thresholds §8 |
| Denser segment index (`r/2`, `r/4`) as a Pareto-budgeted knob | **HOLD** — only after the k-sweep shows latency still binding at k=32 |
| Walk-order/fused postcoder | **KILL for decode; retain as memory-only variant** (§5) |
| Sub-Ω(n)-space inverse BWT as a rate-preserving replacement | **KILL (math-class)** (§7.2) |
| zstd ultra-22/long27 as ANVIL ratio backend | **KILL** (11.30% worse bytes than the incumbent portfolio) |
| CM/PPM/neural backend | **KILL as backend; RETAIN as oracle** (track 06/I10-2) |
| Grammar/RLZ/BMS backend | **KILL** (encode collapse + depth, already measured) |
| DEFLATE replay / G5D paged dictionary | **ADOPT via existing promotions** (tracks 01, 08) — not re-opened here |
| Any "clever sublinear BWT memory" proposal in a later swarm | **ROUTE to §7.2** before implementation |
| G-lane typed basis × **BWT** backend | **HOLD, cheap bytes-only test** (§9 item 4) — high information, near-zero cost |

**FINAL RECOMMENDATION: PROMOTE-TO-REMOTE.**
Ship no new backend *identity*; promote a bounded, free, adopt-class decode-schedule change
in the incumbent backend to a **pre-registered remote-only paired pilot** whose promotion
threshold is the exact 3.33× arithmetic that would place ANVIL's byte count at Brotli's
decode rate, with RSS reported honestly as the likely remaining `FRONT-GAP` axis. Hold both
novel-backend candidates; kill the zstd/CM/grammar/sublinear-memory branches with the
reasons recorded above so the swarm does not re-derive them.

**Uncertainty that could still change this verdict (and only these):** (a) `libsais_unbwt_aux`
concurrency behaviour, (b) TLB/bandwidth ceiling for k ≥ 8, (c) whether the coordinator
weights RSS above decode. Items (a) and (b) are answered by one remote job.
---

## 11. ADDENDUM — Shared-measurement alignment with track 11 §9 P1 (one pass, one protocol)

**Coordinator instruction honoured:** no second timing protocol is invented here. Track 11's
§9 P1 (`docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md:450-458`) is adopted as the
instrumentation vehicle; this section only (a) fixes the single protocol chain, (b) defines
how one pass serves both decode paths, (c) lists the counters I need **beyond** the common
core, and (d) tightens my thresholds using the backend-attribution data the pass will emit.

### 11.1 Single protocol chain (no second protocol)

| layer | artifact | role |
|---|---|---|
| normative statistics | `docs/github-actions-benchmark-protocol.md` §5-§6 | ≥7 paired reps, raw reps retained, MAD robust CV, paired `log(t_cand/t_ctrl)`, **seeded bootstrap 95% CI**, no dropped outliers, arm CV ≤ 15%, pre-registered epsilon (2% default), ambient gate |
| CI vehicle | `docs/GITHUB-ACTIONS-BENCHMARKING.md` §3 (tiers A/B/C), §5 `taskset` pin, §8.1 `manifest.json` + `bytes.csv` | arm layout, affinity, artifact contract |
| frozen instance | `docs/I10-DENSE-FRONTIER-PREREG.md` §3-§8 | corpus identities/MD5s, Silesia panel {dickens, mozilla, nci, webster, xml}, reference class, seed 41246, reps 7/9/13, warmups 2, eps 2%, CV 15%/10% |
| driver | `tools/paired_bench.py` (pinned blob per dense prereg §2), self-test via `--self-test-observation` | the **only** promotion-path driver |

These are three layers of one protocol, already mutually consistent (the dense prereg's
numbers are the protocol's defaults). **My earlier §8 enumeration is withdrawn as a
protocol statement and re-expressed as arm selection + thresholds only.**

### 11.2 One job, two paths, one manifest

The two tracks' mechanisms live on **disjoint decode paths**, which is a fact of the
production code, not a preference: mode 15 is a rev-1 semantic path
(`src/anvil.cpp:4944` flag surface), while WSI-BWT acts on rev-2 mode 17 backend 2
(`src/anvil.cpp:4400-4427`, `:4634-4657`, default 128 MiB ratio block, `:4999-5000`).
So the shared pass is **one workflow job containing two counter arms**, not one arm:

| arm | flags (differing part only) | counters owned by |
|---|---|---|
| **A0** control | frozen legacy default-off `--parse=ratio` | both (byte-identity + routing census) |
| **B0** control | `--parse=ratio --bwt-aux=on --bwt-subblock=16MiB --bwt-lanes=1` | track 05 |
| **B1..B5** | B0 with `--bwt-lanes ∈ {2,4,8,16,32}` | track 05 |
| **B6** attribution control | B0 with segment-restart-only (no lane concurrency) | track 05 (§5) |
| **S0/S1** | track 11's frozen mode-15 arms | track 11 |
| references | Brotli q11/lw30, zstd ultra-22/long27, xz-9e, dense tiers | shared |

One `manifest.json`, one `bytes.csv`, one `census.csv`, one `paired.csv`, one corpus
manifest, one runner fingerprint, one `taskset` result. Arm selection is a flag; **no
harness is duplicated** (track 11 §11 A14 explicitly asks for this).

### 11.3 Common core — one schema, two path-scoped definitions

Track 11's P1 core is `H_op`, per-token category counts, pulls/output byte, varint count,
copy bytes, macro entries, and paired same-job decode cost. I fill the **same slots** for
the rev-2 BWT path so a single CSV schema serves both; a `path_kind` column
(`semantic15` | `ratio_bwt`) distinguishes them.

| slot | `semantic15` (track 11 P1a-P1c) | `ratio_bwt` (mine) |
|---|---|---|
| `symbol_entropy_bits_per_op` | `H_op` of the frozen opcode stream | postcoder coding efficiency: `8 · post_payload_bytes / n` bits per output byte |
| `symbol_entropy_dist` | full opcode histogram | postcoder-id histogram with per-id payload bytes (§B5) |
| `pulls_per_output_byte` | entropy-pull count ÷ output bytes | postcoder symbol pulls ÷ n; **plus** `isa_successor_loads_per_output_byte`, which is structurally `1.0` and is **asserted**, not measured |
| `varint_reads_per_output_byte` | varint reads ÷ output bytes | payload-framing uvar reads ÷ n; **plus** `lf_index_entries_per_output_byte` = `1/r`, structurally ≤ `1/1024` and asserted |
| `copy_bytes` | match-copy bytes | LF-walk byte assignments (= `n`, structural) |
| `macro_entries` | macro-path entries | **lane retirements + lane pulls** (= `S` per block, structural) — the BWT analogue of a macro entry |
| `paired_decode` | paired log-ratio + bootstrap CI, same driver | identical fields, same driver |
| `complete_payload_bytes` | full mode-15 payload | full mode-17 payload **including** aux wire, postcoder, transform id, backend id, tile framing |

### 11.4 Extra backend-specific counters I need (B1-B9) — beyond the common core

These are the only additions I ask the shared pass to make. B1-B4 are byte-exact R1-tier
(citation-grade, no timing risk); B5-B7 are counters; B8 is diagnostic-only; B9 is a
fingerprint item.

* **B1 — segment geometry.** `segments_S`, `aux_rate_r`, `seg_len_min`, `seg_len_median`,
  `seg_len_max`, `seg_len_cv`, `distinct_I` (must equal `S`). Detects adversarial case A1
  (single-cycle / degenerate walks) *before* any decode timing is interpreted, and is the
  evidence for the `k_eff ≥ 2 on ≥80% of panel blocks` gate.
* **B2 — observed concurrency.** `k_eff_observed = min(K, S)` per block,
  `lanes_retired`, `lane_pull_events`, plus `k_eff` histogram per corpus file. Distinguishes
  "interleaving did nothing" from "interleaving was not reached".
* **B3 — pointer-chase geometry (the diagnosis test).** Decile histogram of
  `|ISA[p] − p|` over the walk, and `isa_pages_touched` / `l_pages_touched` /
  `t_pages_touched` (distinct 4 KiB pages per decode pass, per domain). This is the direct
  test of §4.4's "one dependent DRAM round trip per byte" claim and of the TLB alternative
  in §9 item 1 — **byte-exact, cheap, and it converts my central inference into a
  measurement.** If `|ISA[p]−p|` is small and page counts are low, the walk is already
  cache-friendly and WSI-BWT is dead (predicted NO-GO, honestly recorded).
* **B4 — footprint assertion.** `n`, `n` (L/tmp), `4n` (ISA) asserted allocated, plus
  measured peak RSS and `bytes_allocated_high_water`. Guards the "no second `L` copy"
  requirement (§4.5) and charges RSS as a gate rather than a footnote.
* **B5 — postcoder census.** `postcoder_id ∈ {0..4}` histogram with per-id payload bytes and
  per-id `n`. Required to charge postcoder model cost and to keep §7.1's retire-argument
  falsifiable (if the postcoder is a large share of the critical path, the walk-order
  variant must be re-opened rather than retired).
* **B6 — backend attribution per file.** Which backend won per file (and per block), with
  **per-file decode seconds split by winning backend in the same job**. This is the input to
  the attribution gate in §11.5 and it is the only legitimate way to get `t_nonBWT`
  (§11.6 provenance note).
* **B7 — tile census.** `n_tiles`, per-tile decoded length, per-tile payload bytes, per-tile
  aux wire bytes. Makes the 16 MiB tiling arm fully charged rather than assumed.
* **B8 — stage-isolation probe (diagnostic-only, never a codec arm).**
  `--bwt-postcoder-only` decodes the postcoder stream and discards the walk. Its output is
  **not** a valid reconstruction and must be flagged `DIAGNOSTIC_NOT_A_CODEC_ARM` in the
  CSV; its only use is the stage split `walk ≈ total − postcoder`. Without it the
  mechanism's cost model stays a projection.
* **B9 — fingerprint additions.** `libsais` tag/commit and the exact unbwt entry point used
  (`libsais_unbwt` vs `libsais_unbwt_aux`), `ANVIL_HAVE_LIBSAIS`, THP/`AnonHugePages`
  status, and `lscpu` cache sizes. Uncertainty §9 item 2 (does `libsais_unbwt_aux` already
  interleave?) is *only* answerable if the exact library identity is pinned in the artifact.

### 11.5 Sharpened thresholds (frozen here, before any run)

Track 11's G1 asks "is entropy+dispatch ≥ 25% of measured per-iteration decode cost?"
The backend-path analogue is the B6 attribution split. Combining it with §6 gives a
*two-level* gate that cannot be satisfied by an unrelated arm moving:

| gate | GO | NO-GO / VOID |
|---|---|---|
| **G-B1 (whole-corpus, denominator-complete)** | Silesia paired decode ratio `B_best/B0` point ≤ **0.28**, bootstrap 95% CI upper ≤ **0.30** (= the 3.33× / Brotli-parity line, §6) | point > 0.28 or CI upper > 0.30 ⇒ no promotion |
| **G-B2 (attribution)** | BWT-path ratio ≤ **0.20**, i.e. `Σ_{BWT-routed files} decode(candidate) / Σ decode(control) ≤ 0.20`, computed from B6 in the same job | whole-corpus ≤ 0.3007 **and** BWT-path > 0.20 ⇒ **VOID** (the non-BWT arm moved; that is a control failure, not a win) |
| **G-B3 (mechanism, not the index)** | `B6_restart_only` does **not** reproduce the factor (its ratio > 0.80) | restart-only reproduces the factor ⇒ mis-attributed ⇒ KILL |
| **G-B4 (diagnostic, not blocking)** | B3 shows `|ISA[p]−p|` distribution consistent with a latency-bound walk | B3 shows a cache-friendly walk ⇒ record the predicted NO-GO honestly and stop the k-sweep |
| **G-B5 (cost)** | complete bytes ≤ control + **0.02%**; peak RSS ≤ control × **1.02**; `.text` ≤ **+4 KiB**; B1 `distinct_I == S` on every block; `isa_successor_loads_per_output_byte == 1.0` | bytes > +0.05%, RSS > ×1.05, `.text` > +4 KiB, or any structural assertion violated ⇒ KILL |
| **G-B6 (enwik8, reported non-blocker)** | ratio ≤ 0.188 point (= 5.32×, §6) | reported, does not block promotion |
| **G-B7 (measurement validity)** | A/A null CI spans 1.0; ambient robust CV ≤ 10%; arm robust CV ≤ 15%; every repetition's output SHA-256 identical; `A0` byte-identity vs frozen 46,446,995 B / 23,534,368 B PASS | any failure ⇒ `TIMING_BLOCKED`, no claim |

Threshold-movement prohibition (master-brief item 5) applies to all of G-B1..G-B7.

### 11.6 Threshold derivation and provenance (no splicing)

* 3.33× line **[M+arith]**: control 4.2015 s (aux, 128 MiB) or 4.2232 s (aux, 16 MiB) from
  `docs/I10-BREAKTHROUGH-PROGRAM.md` §10 I10-1A.2 **[REM10]** over 211,938,580 B
  (`docs/I10-DENSE-FRONTIER-PREREG.md` §4) at Brotli's 166.9 MB/s
  (`docs/I10-AUX-UNBWT-RESULTS.md` §11 **[REM10]**) ⇒ target 1.2700 s ⇒ ratio 0.3007.
* **Dilution check [M+arith]:** BWT-routed files carry
  10,192,446+9,970,564+33,553,445+10,085,684+6,627,202+41,458,703+8,474,240 =
  **120,362,284 B = 56.79%** of Silesia (routing from `docs/I10-AUX-UNBWT-RESULTS.md` §8
  **[REM10]**). Even an *instant* BWT decode leaves
  `91,576,296 / 166.9e6 / 4.2015` = **0.1306** as the whole-corpus floor — far below the
  0.3007 target, so dilution is **not** the binding risk.
* **BWT-path factor derivation [M+arith, PROVISIONAL]:** taking `t_nonBWT` = non-BWT bytes at
  the Silesia Brotli aggregate rate gives `t_nonBWT ≈ 0.549 s`, implied control BWT-path time
  `≈ 3.653 s` (≈32.9 MB/s), and a required BWT-path factor of **5.06×**. This number is
  **provisional and derived from two aggregates**; B6 replaces it with same-job measurement
  and G-B2 is expressed as a ratio so it survives that substitution.
* **Prohibition honoured:** the per-file BWT-direct decode seconds in
  `tests/bwt-backend-standard.csv` (≈85 ns/B, 6n RSS) are **[LOCAL-ABL, ranking-grade]** and
  are **never summed with, or divided into, the [REM10] aggregates above**. They are used
  only (a) as the qualitative 85 ns/B anchor for §4.4 and (b) as the hypothesis that B3
  will convert into an [M] measurement. The 6n footprint constant (5.99-6.35× across four
  files) is host-independent enough to survive as a structural constant.

### 11.7 Dependency I am taking on track 11 (not re-deriving)

Track 11's **P1d** publishes one reconciled product of `λ`, the cost unit `c`, and the
Exp-AA ns/B table. My §4.4 cycle model needs exactly that calibration plus one new anchor
(the BWT walk latency constant). **I adopt track 11's P1d output as the shared cost unit and
add only `L_DRAM_eff` from B3/B8** — I do not open a second calibration. If P1d slips, my
cost model stays a projection and the k-sweep is run on measured wall-clock only; that is
acceptable because every GO/NO-GO threshold in §11.5 is a wall-clock ratio, not a modelled
cycle count.

---

## 12. ADDENDUM — Instrumentation must never touch a timing number (track 18 finding)

**Finding adopted from track 18:** instrumentation itself perturbs decode by **2.3-8.1%**.
Consequence, binding on track 05: **a timing delta produced by a build that contains
counters is not evidence.** This section rewrites §11.3/§11.4/§11.5 accordingly and
supersedes them wherever they mixed the two.

### 12.1 Structural separation: one source, two binaries, two passes

Single source tree, two CMake targets from the **same blob** (both sha256 recorded in
`manifest.json`):

| binary | build | permitted use |
|---|---|---|
| `anvil` | production flags; counters `#ifdef`-compiled **out** | **all timing** (every §11.5 gate), byte census, correctness, fuzz |
| `anvil-telemetry` | `-DANVIL_TELEMETRY=1`, counters **observation-only** | **byte-only** counter emission (B1-B5, B7); **zero timing claims** |

Hard rules:

1. **No timing number from `anvil-telemetry` may enter any gate, any CI, any table, or any
   sentence containing a ratio.** Not "may be corroborated by", not "sane because" — it does
   not exist as evidence.
2. **Counters are pure observation**: no counter may influence control flow, stream
   selection, J-gating, the postcoder choice, the aux/`r` choice, or lane scheduling.
   Enforced by a **byte-identity assertion**: for every corpus file,
   `anvil-telemetry` compressed output and decoded output must be byte-identical to `anvil`
   in the same job. Any divergence is a hard failure (a counter has leaked into the
   representation, which invalidates every counter it accompanied).
3. **Determinism assertion for counters**: B1-B5, B7 must be *bit-identical across all
   repetitions*, not just present. A counter that varies between reps is measuring
   perturbation, not the codec.

### 12.2 Two passes in one job (same manifest, no new protocol)

| pass | tier | binary | content | claims allowed |
|---|---|---|---|---|
| **Pass 1** | Tier A / R1 (byte-only) | `anvil-telemetry` + `anvil` | counters B1-B5, B7; byte identity; per-block routing; corpus/fingerprint | **bytes and counts only** |
| **Pass 2** | Tier C (promotion timing) | `anvil` only, uninstrumented | paired A/B arms A0, B0-B6, S0/S1, references; per-file decode; peak RSS; `.text` | all G-B1..G-B7 gates |

Same driver (`tools/paired_bench.py`), same pin, same corpus manifest, same ambient gate,
same reps — only the binary and the phase differ. Pass 1 cannot influence Pass 2 except
through byte counts, which are deterministic.

### 12.3 Consequence for each counter I requested (§11.4)

| counter | classification | handling |
|---|---|---|
| B1 segment geometry, B2 observed concurrency, B3 pointer-chase geometry, B4 footprint assertion, B5 postcoder census, B7 tile census | **byte-exact counters** | Pass 1 / telemetry only. B4's *measured peak RSS* moves to Pass 2 (`/usr/bin/time -f %M`, uninstrumented); only the allocation assertions stay in telemetry |
| **B6 backend attribution per file** | **timing** — reclassified | Obtained **without telemetry**: with `--parse=ratio` the block size is 128 MiB (`src/anvil.cpp:4999-5000`), so every Silesia/enwik8 file is a **single block** and per-file wall-clock decode time is *exactly* per-backend decode time; routing per file is byte-deterministic and comes from Pass 1. G-B2 is therefore computed from uninstrumented per-file seconds. If a future corpus produces **multi-block files with mixed backends**, per-block attribution would need telemetry ⇒ in that case attribution is reported `DIAGNOSTIC` and G-B2 is evaluated on the aggregate ratio alone |
| **B8 postcoder-only probe** | **timing** — reclassified | Must be an **uninstrumented** build variant of `anvil` with a debug decode path (no counters compiled in), compared same-job against the uninstrumented full path. Label `DIAGNOSTIC_NOT_A_CODEC_ARM`; feeds no gate |
| B9 fingerprint | neither | manifest only; both binaries' identities recorded |

### 12.4 A/A nulls — now two of them

* **A/A-1 (protocol null):** `anvil` vs `anvil`, same flags, same pass. CI must span 1.0
  (`docs/github-actions-benchmark-protocol.md` §5). Unchanged requirement.
* **A/A-2 (perturbation null, new):** `anvil-telemetry` vs `anvil` on the *same* arm and
  the *same* Pass-2 protocol, reported as a paired ratio. This **converts track 18's
  2.3-8.1% range into a per-run, per-arm artifact** instead of an assumed constant.
  - Recorded, **never gated** as a win/loss; it is an instrument-calibration row.
  - **Validity rule:** if the A/A-2 point ratio falls outside **[0.92, 1.081]**, the
    telemetry build is out of contract ⇒ **the counter set for that run is VOID** (Pass 1
    results quarantined and re-run). The **timing** arms are unaffected either way — they
    contain no counters — so a perturbation failure can never void or rescue a frontier
    claim.

### 12.5 Effect on track 05's own gates

* **G-B1 (whole-corpus ratio ≤ 0.28 point / ≤ 0.30 CI-upper vs a 1.0 null) is ~3.5x outside
  the perturbation band** and is read from Pass 2 only. **Unchanged.** The headline
  threshold is robust to this finding by construction.
* **G-B2 (BWT-path ratio ≤ 0.20) unchanged in value, re-sourced** to uninstrumented per-file
  seconds (§12.3).
* **G-B4 (pointer-chase geometry) is explicitly `diagnostic, not blocking`** — it is a
  counter, so it may inform the *interpretation* of a Pass-2 result but may never be the
  reason a Pass-2 number is accepted or rejected.
* **The HOLD band is where this bites:** §8's HOLD band (0.30, 0.50] and any sub-8%
  sub-finding inside a k-sweep cell must be reported as *"below the instrumentation
  resolution of this pass"* and **may not be used to justify a mechanism change, a default
  change, or a claim.** If a sub-8% timing difference is decision-relevant, the remedy is
  more repetitions and an uninstrumented build — never a smaller epsilon after the fact
  (master-brief item 5).

### 12.6 Cross-track flags for the coordinator (not my lane to set)

1. **Track 11 G6 (`decode ≥ 1.08x`, paired CI excluding 1.00) lies inside the measured
   2.3-8.1% perturbation band.** As written, an instrumented 1.08x could be an artifact.
   Recommend it be read from uninstrumented timing, or its epsilon raised before any data
   exists.
2. **Any gate whose effect size is <8% across tracks 05/06/11/15/18 inherits this rule**,
   not just track 05.
3. **Pass 1 telemetry is cheap and reusable:** one counter binary serves all tracks'
   byte-only decomposition needs in one job. Recommend the coordinator designate
   `anvil-telemetry` as a shared, version-pinned artifact rather than per-track builds.
4. **No change to `tools/paired_bench.py`** and no second driver — the perturbation problem
   is fixed by build separation, not by statistics.
---

## 13. ADDENDUM — Reuse the recorded decode floor; shrink the request; audit CAM

**Coordinator correction adopted.** The `generated.log` decode-floor decomposition
(`RESEARCH_LEDGER.md:3911-3931`; decode-perf t3, payloads from git `392e937`) is **existing
evidence**. It is not re-measured, no remote budget goes to it, and §13.1 derives from it what
it can supply. Recorded caveats carried forward: `median-of-7 INTERLEAVED`, `warmup 2`,
pinned last logical processor, QPC+invariant-TSC, instrumented decoder verified
byte-identical to `decode_tokens_hotop_fused` on every block, all four containers byte-exact,
within-run CV 0.6-10%, run-to-run drift ±2-4%, **zero-byte-change control variants measured
+2.3-8.1%**, and the absolute level **~5-15% below** the bench-suite protocol so only the
*relative* deltas are load-bearing.

### 13.1 What the recorded profile supplies, and the derived consequence

Shares (`RESEARCH_LEDGER.md:3924-3930`) converted against that profile's own measured floor
of **235.8 MB/s = 4.24 ns/B** on `generated.log` **[M+arith]**:

| component | recorded share | derived absolute cost |
|---|---:|---:|
| `crc32` (per-byte; cross-file 31.4-56.1%) | 44% | **1.86-1.91 ns/B** |
| eager materialization of the 7 macro streams (setup = masks+resid) | 26% | ≈1.10 ns/B |
| token loop beyond setup | 11% | ≈0.47 ns/B |
| **opcode entropy pulls** | **0.3%** | ≈0.013 ns/B |
| concat / alloc / headers | 15% | ≈0.64 ns/B |

**Load-bearing derived result [M+arith]:** the BWT backend decodes at **84.8-88.8 ns per
output byte** (`tests/bwt-backend-standard.csv`, `[LOCAL-ABL]`, ranking-grade, hypothesis-grade
until Pass 2). Every component above is *per-byte and path-independent* — the ledger says so
explicitly for `crc32` ("strictly per-byte and stable across files"). Scaled to the BWT path:
`crc32` **2.1-2.2%**, materialization **1.2-1.3%**, concat/alloc **0.7%**, token loop **0.5%**,
opcode pulls **0.015%** ⇒ **all *known* non-walk work on the BWT backend is bounded by ≈4.7%
of decode, and by ≈2.5% after the retained CRC PCLMUL fix** (master-brief item 12;
`RESEARCH_LEDGER.md` A21).

Three consequences, each of which *removes* work:

1. **CRC is not a track-05 contribution and is not measured on my path** — already adopted,
   wire-invisible, and at 1.9 ns/B it cannot matter against an 85 ns/B walk.
2. **Nothing in the semantic-path cost surface explains or rescues the BWT path.** The
   residual ≥95% is the LF walk, which the profile never touched.
3. **Track 05's decode thesis therefore rests on exactly one unmeasured quantity — the walk —
   and the legitimate remote request collapses to §13.3.**

### 13.2 Why the profile cannot simply be extended to this path

Structural, not budgetary:

| profile property | BWT-backend reality | consequence |
|---|---|---|
| mode 15 fused-token path, `generated.log` container | rev-2 mode 17, backend 2 (`src/anvil.cpp:4400-4427`) | different decode program |
| then-current default **256 KiB** blocks (`src/anvil.cpp:3769`) | ratio mode forces **128 MiB** blocks (`src/anvil.cpp:4999-5000`) ⇒ **one block per Silesia/enwik8 file** | there is *no* per-block model-restart component to decompose on the canonical corpora |
| 7 macro streams (masks/resid/dvar byte shares given) | postcoder registry ids 0-4 = static-MTF / arith-o0 / arith-o1 / raw-BWT / QLFC (`src/anvil.cpp:5006`) | the recorded ns/B fits name **different codec objects** |
| no permutation; the token loop *is* the work | one dependent random access into a **4n** table per output byte | the 4.24 ns/B floor is ~20x below the BWT path's ns/B; nothing scales |
| relative deltas only; level 5-15% low; noise band 2.3-8.1% | — | my requests must be **byte-deterministic** or **uninstrumented and >8%** (§12) |

### 13.3 Reduced request list — with a derivation for every withdrawal

**Withdrawn (derivable or inapplicable):** *copy-length distribution* (degenerate here — the
LF walk assigns every output byte exactly once, so copy bytes = `n` and the distribution is a
point mass); *opcode entropy / opcode stream* (the rev-2 path has none); *token-loop and
state-update counters* (no token loop); *per-stream CRC / materialization / concat counters*
(already bounded at ≤4.7% by §13.1, and CRC is already fixed by an adopted change);
*re-measuring the `generated.log` floor* (prohibited, and wrong program).

**Still requested (M1-M6), each with why it is not derivable:**

| id | quantity | tier | why it is genuinely missing |
|---|---|---|---|
| **M1** | per-postcoder-id payload bytes, per-id `n`, block count | Pass 1, byte-only | the recorded ns/B fits name the rev-1 stream-suite codecs; the BWT postcoders are different objects. With M1 the calibrated table applies to *measured byte counts*, making the postcoder's ns share a **derivation** rather than a measurement |
| **M2** | per-file decode seconds, uninstrumented | Pass 2 | backend attribution needs no counters (§12.3); G-B2 input |
| **M3** | minor/major page-fault counts, peak RSS, `.text` (`/usr/bin/time -v`, `size`) | Pass 2, **zero instrumentation** | the 6n working set's **first-touch** cost has no analogue in the profile (whose alloc/concat share is 0.64 ns/B on a path with no 4n table). OS counters are immune to §12's perturbation rule |
| **M4** | LF segment geometry (`S`, `r`, seg-len min/median/max/cv, `distinct_I`) and the `|ISA[p]-p|` decile histogram | Pass 1, byte-only | **absent from the entire ledger**; it is the mechanism's own precondition. If `|ISA[p]-p|` is small and page counts low, WSI-BWT is dead — recorded as a predicted NO-GO, not deleted |
| **M5** | tiling-penalty attribution ablation (§13.5.3) | Pass 1, byte-only | decides whether CAM has any target on the backend path; not derivable from 256 KiB semantic numbers |
| **M6** | one **uninstrumented** stage-split build (postcoder-only) vs full, same job | Pass 2 | the one new ns number that cannot be derived. Effect ≈95%/5%, far outside the 2.3-8.1% band, and neither binary contains counters |

### 13.4 J-dependence: does the ranking change under calibrated size-proportional cost?

Source facts: the default selector charges **length-independent per-stream constants**
(`src/anvil.cpp:1553`; incumbent `10/20/22/30/35/40/45` — `RESEARCH_LEDGER.md:3902-3903`:
"right ORDER, wrong GAPS"), while a **size-proportional ns/B** path exists behind
`--hotop-budget` with pre-registered fits `kBudgetNsPerByte[7] = {0.1, 6.0, 6.0, 6.0, 4.3, 3.2,
7.5}` (`src/anvil.cpp:1618`, `:1635-1636`, `:3786`, `:4979`).

| track-05 candidate | bytes | selector interaction | ranking under calibrated size-proportional `C_decode` |
|---|---|---|---|
| **WSI-BWT** (any k) | **unchanged** (0 wire bytes) | none — adds no candidate to any J-minimisation set | **INVARIANT.** Same `L`, strictly lower measured `C_decode` ⇒ J-value strictly improves under *both* models; the k-sweep ranking is pure decode wall-clock |
| **aux index on/off** | +0.0072% | none — `bwt_postcoder_payload` picks the smallest **payload** (`src/anvil.cpp:4304-4309`) | **INVARIANT** |
| **16 MiB tile arm** | **+1.0169%** | none directly, but a λ>0 J-selector sees its +0.52% decode | **NOT INVARIANT — it degrades.** A length-independent constant under-charges per-byte decode work; size-proportional charging penalises the tile *more*, while its benefit (−49.4% RSS) is not in J at all ⇒ **no λ>0 J-selector can prefer 16 MiB over 128 MiB.** The tile must be an **explicitly named profile**, never a J-selected arm — matching the standing I10-1A ruling that a frontier router must use an explicit profile/Pareto budget, not a hidden scalar λ |
| **WFR / walk-order postcoder** (§7.1) | unknown | would interact with any stream-suite J | **SENSITIVE — the one J-exposed claim.** Its retirement cited "postcoder ≈5-10% of the critical path"; that number must come from **M1 + M6** (§13.1's ≤4.7% bound strengthens the retirement but is a cross-path transfer, not a substitute) |

**Cross-track input (tracks 02/16):** any J-based planner scoring block/tile choice is
**systematically biased toward the largest block**, silently discarding the RSS axis. RSS must
enter the planner's objective explicitly or not at all.

### 13.5 Audit of Track 15's CAM (compact per-block adaptive-model prior)

Track 15's framing is correct and I concur: CAM targets a **byte** tax, not a decode tax. The
measured fragmentation penalty is **13.74% at 256 KiB, 8.60% at 1 MiB, 4.65% at 4 MiB**,
attributed to entropy/model warmup at block restarts
(`docs/audit-2026-09-07/13-e4-block-routing-oracle.md:42-46`, cited in track 15 §2.1 M4).
Three backend-lane observations:

**13.5.1 — On the ratio path CAM's target is structurally zero.** Ratio mode forces a 128 MiB
block (`src/anvil.cpp:4999-5000`), so every Silesia and enwik8 file is **one block** and there
is **no model restart to amortise**. CAM-0 (self-prime from the block's own first K bytes) is
there *a hand-rolled re-implementation of the free null* (one big block) plus a K-byte priming
latency at the head of every block.

**13.5.2 — Against the free null, CAM loses on the axis that matters here.** The free null is
"larger blocks / model carryover" and its curve is already measured: 13.74% → 8.60% → 4.65%
as blocks grow. Track 15's own §4.3 arithmetic has `P = 64 B` admissible only at ≤256 KiB and
failing at 4 MiB unless ≥38% of the tax is model-attributable. The ratio path's block is
already **512x** larger than 256 KiB, so both the tax and the admissibility window are gone.
**Verdict: CAM is correctly scoped to the rev-1 semantic path; on the backend path it is KILL
as a primary target, not HOLD.**

**13.5.3 — The one backend analogue, and the byte-only experiment that decides it (M5).** My
tiling arms *create* a restart tax: +1.0169% bytes at 16 MiB, +1.9346% at 8 MiB
(`docs/I10-BREAKTHROUGH-PROGRAM.md` §10 I10-1A.2). **But the attribution is unmeasured and
is not obviously "model warmup":** in a BWT backend the dominant cross-tile loss is *lost
cross-tile BWT context* (substrings spanning the boundary cease to be runs in `L`), with
postcoder/MTF-state restart second-order. Prior expectation **[P]**: mostly context loss ⇒
CAM-family recovery is a small fraction of ≤1.93%.

| arm | BWT scope | postcoder model | isolates |
|---|---|---|---|
| **T0** | 16 MiB tiles (current) | restart per tile | the full measured penalty, +1.0169% |
| **T1** | 16 MiB tiles | **carried across tiles** (0 transmitted bytes — "do not reset") | the **model-warmup component** |
| **T2** | whole block (128 MiB) | n/a | the ceiling: full recovery |

If `T1 ≈ T0` ⇒ the penalty is cross-tile **BWT context**, CAM has no target here, and track 05
keeps the tile cost as a measured trade. If `T1` recovers a material share ⇒ CAM gains a
*backend-lane* application track 15 did not have, and the correct instrument is **carryover
(0 bytes)**, not a transmitted prior — with the charge track 15 already identified for chained
state: tile *k* stops being independently decodable, so **independent-tile random access is
lost at tile granularity**, a format-visible independence change (charge to tracks 15/19, not
to bytes).

**13.5.4 — Interaction with my primary candidate: none, and that is the point.** WSI-BWT costs
0 bytes and changes no selector input; CAM/carryover costs bytes *or* independence. They are
composable but **sequenced**, and M5 is byte-only and cheap. **CAM/carryover is not a
prerequisite for track 05 and must not delay it** — the RSS axis it attacks is the axis WSI-BWT
cannot move (§4.5, §7.2), so they are independent bets on two different deficits.

---

## 14. ADDENDUM — `f_walk` becomes the decisive scalar: freeze stage decomposition BEFORE any WSI build

**Critic's scalar adopted.** `f_walk` = (LF-walk time) / (total BWT-backend decode time). No
lane-scheduler prototype, no `--bwt-lanes` build, and no k-sweep may be written before one
remote stage-decomposition job has measured it with perturbation accounting.

### 14.1 The predeclared arithmetic kill (frozen here, before any data)

Let `T0` = control BWT-path decode, `g = 1 − f_walk` = non-walk share, `r` = residual walk
fraction after interleaving (so the mechanism multiplies the walk term by `r`). Then

```
BWT-path ratio  =  f_walk·r + g
whole-corpus    =  (1−α) + α·(f_walk·r + g),   α = T0/T_total
```

`α = 3.653/4.2015 = 0.8695` **[M+arith, provisional]** — BWT files carry 56.79% of Silesia
bytes (§11.6) but ≈87% of its decode time; the whole-corpus floor is 0.1306.

**Arithmetic kill, absolute form (perfect walk, `r = 0`):**

* BWT-path ratio ≤ 0.20 requires **`f_walk ≥ 0.80`**.
* Whole-corpus ratio ≤ 0.3007 requires **`f_walk ≥ 0.804`**.
* ⇒ **`f_walk < 0.80` ⇒ KILL WSI-BWT with no implementation, no prototype, no k-sweep.**

**Full requirement table (`f_walk ≥ X` needed for each residual-walk target):**

| residual `r` (walk speedup) | to hit BWT-path 0.20 | to hit whole-corpus 0.3007 | verdict at `f_walk = 0.95` |
|---|---|---|---|
| `r = 0.25` (4×) | ≥ 1.067 — **impossible** | ≥ 1.072 | unreachable at any `f_walk` |
| `r = 0.20` (5×) | ≥ 1.000 — **impossible** | ≥ 1.005 | unreachable |
| `r = 0.125` (8×) | ≥ 0.914 | ≥ 0.919 | reachable (0.875 needed) |
| `r = 0.10` (10×) | ≥ 0.889 | ≥ 0.894 | comfortable |
| `r = 0.05` (20×) | ≥ 0.842 | ≤ 0.847 | very comfortable |

Two hard, predeclared readings:

1. **Even an infinitely fast walk needs `f_walk ≥ 0.80`.**
2. **Any residual `r ≥ 0.20` (≤5× walk speedup) is unreachable at any `f_walk`.** So G-B2
   (BWT-path ≤ 0.20) is only satisfiable by a *≥8×* walk speedup; a 4-5× outcome lands in the
   G-B1 band (≤0.30) but fails G-B2 and must be recorded `EXTENDS_FRONT / FRONT-GAP_RSS`,
   never upgraded post hoc.

**CI discipline (predeclared, because stage subtraction compounds error):** the kill test uses
the **pessimistic end** of the interval. **`f_walk`'s bootstrap 95% CI lower bound < 0.80 ⇒
KILL.** A point estimate above 0.80 with a CI lower bound below it is a **HOLD**, not a GO.
Any individual stage share below **8%** is reported as *"below this pass's resolution"* and
never used as a number (ledger's zero-byte-change band, `RESEARCH_LEDGER.md:3919`).

### 14.2 My prior on `f_walk` — and why it must not be trusted

**[P] `f_walk ≥ 0.95`**, from §13.1: known non-walk per-byte work (crc32 1.9 + materialization
1.10 + concat/alloc 0.64 + token loop 0.47 ≈ 4.1 ns/B) against 84.8-88.8 ns/B. **This
projection is unsafe in four specific ways**, each of which is exactly what the freeze job
measures:

1. **ISA construction is unmeasured.** libsais must build or read a 4n inverse structure; if
   that is 15-20% of decode, `f_walk` falls to ≈0.76-0.80 and WSI-BWT is **dead**.
2. **The postcoder ids 0-4 have no ns/B** (the recorded fits name different codecs) — M1/M6.
3. **First-touch of 6n is unmeasured** (M3); the profile's 0.64 ns/B alloc share comes from a
   path with no 4n table.
4. **The transfer is cross-path and cross-host**: absolute level 5-15% low, drift ±2-4%.

⇒ The prior is directionally useful for *sequencing* and must not enter any gate.

### 14.3 The frozen stage-decomposition job (perturbation-safe by construction)

**Principle: no timers inside a binary.** Four **uninstrumented** variants of the same
source, differing only by an early `return` after a stage boundary; each timed end-to-end
with the §11.1 protocol. Differences, not internal timers, give the decomposition.

| arm | stops after | yields |
|---|---|---|
| **F0** | postcoded stream decoded to `L` (aux framing parsed; `L` allocated) | `T_postcoder = T(F0)` |
| **F1** | + inverse structure built (ISA or equivalent), no walk | `T_isa = T(F1) − T(F0)` |
| **F2** | + full walk = production decode | `T_walk = T(F2) − T(F1)` |
| **F3** | + decode-side CRC verification + final framing/alloc (only if the main decode path performs it — **established by code read inside the freeze job, not assumed**) | `T_frame = T(F3) − T(F2)` |

`f_walk = T_walk / T(F2)`; report each stage with its own CI; report any negative or
implausible difference as a measurement failure rather than clipping it.

**Perturbation accounting (mandatory, extends §12):**

* **A/A-1** identical uninstrumented arm twice — CI must span 1.0.
* **A/A-2** telemetry vs uninstrumented, same arm — per-run perturbation artifact; outside
  `[0.92, 1.081]` ⇒ counters VOID (timing arms unaffected).
* **A/A-3 (new, mandatory here)** `F0` and `F1` are *not* byte-valid decodes ⇒ they are
  labelled `STAGE_PROBE_NOT_A_CODEC_ARM`, excluded from every round-trip/hash gate, and
  **must each be run against the same payload and the same corpus as F2**. Because they
  differ from F2 by a large structural amount, their effect sizes must clear 8%; if a stage
  difference does not, it is unresolvable in this pass and is reported as such.
* Binary-size delta of each probe arm is charged (each probe must be ≤ +4 KiB `.text`, and
  the deltas are **not** additive into the production arm's code budget).

### 14.4 Sequencing — G-lane×BWT bytes-only outranks all timing work

The coordinator's condition is met and I adopt the ordering:

| rank | job | tier | can it invalidate the reframing? |
|---|---|---|---|
| **0** | **G-lane typed basis × BWT cross-product, bytes only** | R1, byte-only | **YES — it can invalidate §2 entirely.** If a frozen G2 typed basis beats BWT-raw by ≥3% complete bytes on any discovery family, the claim "the backend is the mechanism; frontend×BWT is redundant" is false, tracks 01/02/09 outrank track 05, and all timing work below is deprioritised. Prediction **[P]**: null, because BWT is near-invariant to column permutation (repeated rows stay repeated) — consistent with `ctx1`/`line-columns` × BWT having measured nothing (`src/anvil.cpp:4614-4639`; `docs/I10-REMOTE-BASELINE-CLOSURE.md` lines 32-37). Dual-bar reporting mandatory (`docs/gate-ruling-i9-recon-crossing.md` R-2); discovery families only — held-out promotion still requires newly locked families per coordinator D1/D16 |
| **1** | **stage decomposition F0-F3 + M1-M5** (§14.3, §13.3) | Pass 1 bytes + Pass 2 timing | **YES — it can kill WSI-BWT arithmetically** (§14.1) |
| **2** | M6 stage-split confirmation + M3 fault/RSS accounting | Pass 2 | no |
| **3** | **only if rank 1 returns `f_walk` CI-lower ≥ 0.80:** the `--bwt-lanes` build and k-sweep, arms B0-B6, gates G-B1..G-B7 | Pass 2 | — |

### 14.5 Revised disposition (supersedes §10's PROMOTE-TO-REMOTE in scope only)

**PROMOTE-TO-REMOTE (measurement):** ranks 0-2 above, one job, frozen thresholds.
**HOLD (implementation):** the `--bwt-lanes` lane scheduler and the k-sweep, pending
`f_walk` CI-lower-bound ≥ 0.80. **KILL if** CI-lower-bound < 0.80 — no prototype, no code, no
follow-up run, recorded with the stage decomposition as the reason.
This narrows what track 05 asks for; it does not move any threshold. Everything else in §10
(adopt landscape, retired candidates, CAM verdict) stands.
