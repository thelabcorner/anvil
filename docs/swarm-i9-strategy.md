# ANVIL — Iteration-9 Strategy Synthesis (patched from I8) + the Iteration-9 plan and endgame accounting

Author: `strategy` (strategy synthesist, swarm `anvil-i9-pareto`). Status: **analysis
only** — no production code, no benchmark run by this lane, no arbiter run by this
lane, no novelty verdict. Every figure below is either (a) recomputed by me from a
repo artifact at write time with the command shown (§8 and inline), or (b) arithmetic
on a landed measurement, explicitly labeled `[PROJECTION]`, or (c) a peer-landed
measurement carried with its provenance and labeled `[LANDED-PEER]`. Anything not
recomputable is labeled **PENDING**. I do not run `tools/pareto_front.py`; the
predicate is re-implemented from source and labeled "re-derived, not run" wherever
used.

Hard constraints honored: I do not edit `src/anvil.cpp` (arch), `FORMAT.md` /
`tests/fuzz.py` (format), `tests/*.csv` / `tools/pareto_front.py` (bench),
`docs/gate-*.md` / `RESEARCH_LEDGER.md` (research-gate). This document is my lane's
only output.

**Patch discipline (inherited from I7/I8):** this document is patched from
`docs/swarm-i8-strategy.md`, not rewritten. I8 text carried forward was re-verified;
where it failed recomputation the correction is recorded in public (§0 C-1..C-5,
§3 AUDIT-7..AUDIT-10), never silently edited. §7 (Endgame accounting) is rewritten at
iteration end.

**Artifact-state note (new in I9).** The worktree is dirty and is never reset. Every
figure names its artifact AND its state. The 13-file Pareto grid lives in a
worktree-modified `tests/benchmark-suite.csv` (SHA-256 prefix `C70179EA20B63BF5`,
351 data rows = 13 files x 27 codecs = 18 anvil + 9 reference). The version at
`git show HEAD:tests/benchmark-suite.csv` is an older 25-codec grid (325 data rows;
16 anvil codecs; missing `anvil-hotop-budget-rans`, `anvil-hotop-rlzp-rans`) and
still contains the same five non-dominated rows (416 per-file cells there). The
tuple is reported for **both** grids — committed HEAD primary, uncommitted
worktree `c70179ea` labelled — per coordinator `msg_30351ec7`. Ratio-first
artifacts (`tests/ratio-first-standard.csv`, `tests/bwt-backend-standard.csv`,
`tests/auto-routing.csv`, `tests/enwik8-bwt.csv`) are **untracked** worktree
artifacts; flagged to coordinator/bench so they can be committed.

---

## 0. HEADLINE — recompute result, five corrections, and the two programs

**Status at write time:** I9 is in flight; the task board showed 0/10 complete when
this document was first drafted, and **bench's P0 arbiter refresh has since landed**
on both grids (worktree: `tests/pareto-baseline.csv` sha `f432c34445d7de4a...` +
`tests/pareto-verdict.csv` sha `382b262c81c71e80...` from `tests/benchmark-suite.csv`
sha `c70179ea20b63bf5...`; committed HEAD: `tests/pareto-baseline.head-fc23d9a.csv`
sha `66f5e5cd87e9fd85...` + `tests/pareto-verdict.head-fc23d9a.csv` sha
`b9680ac93fe5fb15...`; double-run hash-identical on both grids; detail
`tests/pareto-recompute-i9.md`, blackboard `deliverable/p0-arbiter`). My re-derivation
of the arbiter predicate (tool NOT run) reproduces bench's arbiter output row-for-row:
0 status mismatches across all 468 anvil cells of the worktree grid and all 448 anvil
rows (416 per-file + 32 AGGREGATE) of the committed grid (commands §8). **Citation
rule (coordinator `msg_30351ec7`, research-gate verified; ledger PART XIV): at
committed HEAD `fc23d9a` the grid is 416 per-file cells, 411 dominated, 443/448 with
AGGREGATE; the 468/463 grid exists only in the uncommitted worktree suite `c70179ea`
and must carry that label. Both grids carry the same `5 | 5 | 0`.** This document
cites the committed-HEAD tuple as primary and labels every worktree-grid figure. I do
not run the tool.

**Build-integrity gate (format `msg_774c45e3` -> RESOLVED by format
`msg_d82b6d39`).** The dirty-tree `decode_one_block` refactor broke all decode
paths; **arch's fix is confirmed** and the fuzz gate is GREEN, zero skips, on
`build-i9-format\anvil.exe` sha256 `8EAE1FB3...` (src `62BC6631...`, HEAD fc23d9a):
PASS roundtrip_variants=480, mutations=2880, deterministic_rev2=9,
deterministic_bwt=24, golden_bwt=4, forced_postcoders=20, registry_block_modes=14,
registry_transforms=5, suite_modes=[0,1,2,3,5,6,8], suite_unforceable=[4,7].
**PENDING-0 is therefore CLEARED on that binary** (coordinator-verified: byte-exact
roundtrip on greedy and ratio/auto); arch is rebuilding the shared `./build` from src
`62BC6631...` and has published the canonical binary sha `0D1E130B...` (`build\anvil.exe`,
src `62BC6631...`, HEAD fc23d9a; coordinator `msg_77517809`) — measurement lanes must use
that binary, PR-4-attested (quiet window, median >= 3, thread counts stated). Any
`src/anvil.cpp` change requires re-verification (the PASS is pinned to those shas).
**UPDATE (arch `msg_a1bf3122`):** after the ALLOC-only landing (src `38409E26`,
gates green 442/442 + fuzz 50 PASS + encode identity 494/494), the canonical shas are
`build\anvil.exe` **E8AA2E48** / `build\anvil_bench.exe` **56092B9B** — superseding
`0D1E130B`; all prior canonical-sha citations are invalidated per the naming rule.
**CANONICAL v3 (`msg_2dcf62b6`):** frozen src **BDC90474** (ALLOC + leg 4 retained)
-> `build\anvil.exe` **72D65150...** / `anvil_bench` **379341D9...**; `E8AA2E48` /
src 38409E26 superseded for new runs (byte results carry under the 494/494
encode-wire-identity gate, originating sha labelled). `FE0CF4F1` is the leg-4 LANE
exe sha; `BDC90474` is the src sha. leg-4 gates green; format is independently
re-verifying `72D65150`. Static-table postcoder remains unauthorized / not built.
No commit. All byte figures cited here remain CSV-artifact
recomputes or labels as stated. Recorded as AUDIT-11 (resolved).

**PRIMARY — FROZEN CANONICAL GRID** (`tests/benchmark-suite.frozen-bdc90474.csv`,
351 rows; build `72D65150` / src `BDC90474`; byte identity 351/351 vs prior grid;
`tests/suite-frozen-bdc90474.md`, `deliverable/frozen-grid`; bytes citable,
ranking-grade window). **Tuple — v1 and v2 IDENTICAL:**
`33 | 5 | 0 | 28 | 435/468` (471/504). The 28 DEGENERATE are ratio=1.000
store-path cells (R-2); the 5 FRONT-GAP are the familiar generated.json
encode-plane rows. Co-listed labels: worktree `c70179ea` v1 `5|5|0|0|463/468` /
v2 `1|1|0|0|467/468`; committed HEAD `fc23d9a` v1 brotli+zstd reference class
(grid v1) `5|5|0|0|411/416` / v2 +xz-9e `4|4|0|0|412/416`. **ZERO FRONT-CROSSING
everywhere; GRID-THIN always.** **A19 (gate `msg_9caeb2b2`): the frozen `DECODE-TIE`
calls are VOID** (pre-A9 mat caps); correct status = **DECODE-SHORT** via the
alloc/concat-only caps (generated.json ≈1.394/1.352; synth hotop-rlzp ≈1.054/1.074
vs R' 2.0699/1.5248); decode axis stays closed. **A21 (gate `msg_47c26072`): the store-path anomaly is CLOSED, bench-signed —**
the CRC fast path (S6-1b leg 1 + PCLMUL), a KEPT wire-invisible encode+decode win
(**4.5x enc / 3.4x dec** vs bytewise on the store path; bytes bit-exact, 351/351 +
494/494). The 28 DEGENERATE cells STAND as real store-path speed (DEGENERATE-class,
no frontier claim); store throughput is citable with window
`w-arch-storepath-20260912T1300Z` + arm shas; **A18's caveat is closed.** Do not
attribute the older-grid reference-codec movement (zstd-9 random dec 1990 -> 25206)
to this change. **Do not cite
store-path throughput until the anomaly is root-caused.**
**ANOMALY flagged:** frozen store path ~5-6x faster (325-380 -> 1950-2184 MB/s
enc) with identical bytes — owning change to be named by arch/gate. **Caveat:
bench-issued tuple (arbiter owner); strategy recompute from the new CSV is PENDING
(item 15).**

**The frontier tuple — reference-class v2 (SUPERSEDED by the frozen grid above;
retained for lineage; bench `msg_3cd77a24`, all
double-run hash-identical; `tests/pareto-reference-class-i9.md`,
`deliverable/reference-class-v2`). Five-figure form (DEGENERATE = 0 in all cells):**

> **Committed HEAD `fc23d9a` — v1 brotli+zstd reference class (grid v1) (cite ONLY with this label;
> GRID-THIN: no zstd 4-22, no brotli lw30):** `5 | 5 | 0 | 0 | 411/416` (443/448).
> **— v2 +xz-9e (xz-inclusive citation of record):** `4 | 4 | 0 | 0 | 412/416`
> (444/448).
>
> **Uncommitted worktree `c70179ea` — v1 (label required):** `5 | 5 | 0 | 0 |
> 463/468` (499/504). **— v2 +xz-9e (label required):** `1 | 1 | 0 | 0 | 467/468`
> (503/504).

**ZERO FRONT-CROSSING on every grid and reference class.** The familiar five is a
property of the brotli/zstd-only class: xz-9e on generated.json (ratio 0.092975,
encode 1.32 MB/s) dominates every encode-plane row below that speed — HEAD loses
shape-rans-l0 (-> 4); worktree loses 4 of 5 (only mdl-rans-l001, enc 1.329, survives).

No new EXTENDS_FRONT row exists in I9: bench's P0 refresh landed on the uncommitted
worktree grid and confirmed the same five. No frontier-moving **ANVIL-row**
measurement has landed; three prototype landings have (datastruct 89,877 B on
synth-timeseries — FRONT-GAP dual-bar per PR-3-Q1; pnra-stride 12,936 B on
synth-arith — {engineering}, PR-5 ruled; deflate P4.1 GO, mozilla -1,362,177 B
encode-only) and none alters the tuple (landed rows only; no crossing claim). The
five remain single-plane (encode only), all on `generated.json`, all FRONT-GAP, all
decode-dominated by `brotli-q11`.

### The two programs — always co-labelled

**Program A — the 13-file arbiter grid** (generated/synth corpus,
`tests/benchmark-suite.csv` + `tests/pareto-baseline.csv`). This is the tuple above.
All five non-dominated rows are on `generated.json`'s **encode** plane; the bracket
is `brotli-q11` (0.096, 0.674) to `zstd-19` (0.113, 1.963); all five are DOMINATED
on the decode plane by `brotli-q11` (507.4 MB/s). Program A says: **no crossing on
the 13-file grid.**

**Program B — the ratio-first canonical corpora** (Silesia / enwik8,
`tests/ratio-first-standard.csv`, `tests/bwt-backend-standard.csv`,
`tests/auto-routing.csv`, `tests/enwik8-bwt.csv`). Silesia `--ratio-backend=auto`
= **46,446,995 B** vs xz -9e **48,456,100 B** (**−2,009,105 B**); enwik8 auto =
**23,534,368 B** vs xz **24,831,656 B** (**−1,297,288 B**) and vs Brotli q11/lw30
**24,810,180 B** (**−1,275,812 B**). Program B's byte figures are **as-recorded on
`de9b4caf` and RE-VERIFIED on the canonical `0D1E130B`** (13/13 roundtrip sha256 OK;
`tests/ratio-reverify-i9.csv` sha `53880D50`). Program B
says: **a bytes win on both canonical
corpora that is FRONT-GAP on the decode axis** (Silesia 12-file auto 14.60x / 7.89x;
enwik8 auto 23.95x / 14.06x; the bwt-direct 20.96x / 12.31x and 7-routed 11.24x are
labelled variants; all PR-4-pending — see C-4) and FRONT-GAP on
peak memory vs xz (10.3 GiB, root-caused to Brotli q11 `lgwin=30`, paid identically
by the pure reference helper). **The two programs are never conflated and never
summed.**

### Corrections produced by this recompute pass (recorded in public)

- **C-1 — the "needed %" convention erratum (corrects the I8 erratum, AUDIT-4).**
  I8 §1.1 stated the brief's json/jsonl figures were "computed against rounded
  ratios" and that "exact-byte arithmetic gives 11.33% and 17.22%". Recomputed:
  exact-byte arithmetic over the row's own bytes gives **11.63%** (generated.json,
  89,589 vs 79,172) and **17.92%** (generated.jsonl, 183,506 vs 150,619). I8's
  11.3%/17.2% are reachable only by mixing an exact anvil ratio with a **rounded
  reference ratio** (`1 − 0.096/0.108243 = 11.31%`; `1 − 0.054/0.065182 = 17.16%`).
  The same mix shifted I8's generated.log (37.1 vs recomputed 36.84),
  synth-jitter (28.6 vs 28.25) and doc.md (27.6 vs 27.49). **The brief's original
  −11.6%/−18.0% were Convention A (exact-byte) and were right.** This document
  freezes **Convention A**: `needed = (anvil_bytes − ref_bytes) / anvil_bytes`, both
  terms exact integers from the same row set. All §1.2 figures are Convention A.
  The pattern is AUDIT-4's own failure mode one level down: a recompute that
  re-introduced the error it was correcting.
- **C-2 — one corridor spot-check in I8 §2.7a is flipped.** I8 wrote
  "87,013 B @187.0 -> EXTENDS_FRONT". Recomputed with arch's own convention
  (3-decimal quantised ratio, `prototypes/i8-arch/crossing_check.py`; re-implemented,
  not run): **87,013 B @187.0 is DOMINATED by brotli-q11**; it crosses at 188.0.
  The three corridor values themselves reproduce exactly (86,911 / 106,368 /
  201,088; §2.4). One further I8 count drifts: `anvil.exe` encode front is **8**
  points, not 6 (§1.4).
- **C-3 — record periods were aliased (coordinator erratum, verified against the
  generator source).** `synth-timeseries.bin` is **20,000 x 14-byte** records
  (`struct.pack('<QfH', ts, val, rec_id)`, `tests/make_synth_corpus.py:44-60`), true
  **P = 14**. `synth-columnar-align.bin` is **12,000 x 23-byte** rows
  (`tests/make_synth_corpus.py:112-147`), true **P = 23** (184 = 8 x 23). The I8
  §2.7c "28-byte / 10,000 records / P=28 / P=184" facts came from stride aliasing at
  multiples of the true period. Patched in §2.5; corrected values are
  **PENDING-LEDGER** until research-gate records them in PART XIV. The closest-cell
  arithmetic is **grid-dependent (see C-6)**: worktree −14.4% bytes or 1.78x decode;
  committed HEAD −15.7% bytes or 2.16x decode; the mechanism target simply moves
  to P=14.
- **C-4 — RESOLVED by research-gate (`msg_3145a5c9`, PART XIV S6.2; `docs/gate-verify-regime-i9.md` §9). Two of my claims were wrong and are corrected here in public; one was right.** (a) The Silesia `11.243x / 13.015 MB/s` pair **does recompute** from `tests/bwt-backend-standard.csv` (7 BWT-routed files, dec_s sum 9.248178 s; brotli-q11-lw30 146.329 MB/s; xz-9e 78.698) — my earlier "not recomputable" was wrong and is withdrawn. (b) My 20.33x Silesia replacement came from `tests/auto-routing.csv`'s `anvil-bwt-direct` rows — a different window (~1.81x slower; webster 6.769 s vs 3.516 s) — and is **REJECTED as the Silesia figure pending PR-4**. Carry (gate `msg_0e3fdb5c`, `docs/gate-ruling-i9-decode-multiples.md`): **Silesia = 12-file auto 9.435 MB/s = 14.60x / 7.89x** (adopted); **enwik8 = auto row 5.890 MB/s = 23.95x / 14.06x**; the bwt-direct row (6.730 / 20.960x / 12.308x) is the SAME bytes in a second window - cite as the variant, not the headline; the 7-routed bwt-backend window (11.24x) is a further variant; both portfolio rows are PR-4-pending. (c) My enwik8 correction is **VERIFIED**, and the audit's real error was applying the Silesia multiple to enwik8. All four ratio-first CSVs are untracked, hence **not admissible as citation of record** (gate regime §1); `de9b4caf` is a binary SHA-256 prefix, not a git revision. Exact multipliers remain PR-4-pending (no thread counts); the decode FRONT-GAP class is unaffected. No byte figure is affected.
- **C-5 — grid provenance (UPGRADED by coordinator `msg_30351ec7`, research-gate
  verified).** Two grids exist and must never be conflated: committed HEAD `fc23d9a`
  (artifacts `tests/pareto-baseline.head-fc23d9a.csv` sha `66f5e5cd...`,
  `tests/pareto-verdict.head-fc23d9a.csv` sha `b9680ac9...`)
  = 16 anvil codecs, **416 per-file cells, 411 dominated, 443/448 with AGGREGATE**;
  uncommitted worktree suite `c70179ea` = 18 anvil codecs, **468 per-file cells,
  463 dominated, 499/504**. Both carry the same five non-dominated FRONT-GAP rows.
  **Citation rule:** the committed-HEAD tuple `5 | 5 | 0 | 411/416` (443/448) is the
  citation of record; any 468/463 figure must be labelled "uncommitted worktree grid
  c70179ea". Uncommitted-worktree figures in this document carry that label.
- **C-6 — best-ANVIL grid provenance and grid-dependent decode arithmetic
  (coordinator `msg_70cba686`).** The `140,898 B hotop-rlzp` row on
  synth-timeseries.bin exists ONLY in the uncommitted worktree suite `c70179ea`.
  Committed HEAD `fc23d9a` best is **143,132 B** (three-way tie: sparse-rans,
  sparse-rans-l0, sparse-channels-rans; sparse-rans fastest at 132.6 MB/s); needed
  −15.7% (worktree −14.4%). Every 140,898/158.4 citation is grid-labelled. More
  generally: the two grids are different timing sessions (reference decode speeds
  differ on all nine reference rows), so decode bars/lifts are computed per grid and
  every decode figure in this document names its grid; HEAD-grid lifts:
  synth-timeseries 2.16x (2.20x slacked), generated.json 4.34x (4.43x slacked). All
  decode numbers remain PR-4-unattested (thread counts absent).

---

## 1. The frontier arithmetic, recomputed (not inherited)

Arbiter rule, re-implemented from `tools/pareto_front.py` lines 28-41 (the tool was
NOT run): `q dominates p iff q.ratio <= p.ratio AND q.mbps >= p.mbps, one strict`.
Source: `tests/benchmark-suite.csv` (worktree, SHA-256 prefix `C70179EA20B63BF5`),
13 files x 27 codecs (18 anvil + 9 reference), median-3.

### 1.1 Inventory and the tuple — both grids

- **Uncommitted worktree suite `c70179ea`** (the bench P0 refresh source): 13 files;
  18 anvil codecs; 9 reference codecs (brotli q1/q4/q6/q9/q11, zstd 1/3/9/19); 351
  data rows; **468 per-file anvil row-plane cells** (13 x 18 x 2) plus **36
  AGGREGATE cells**. Re-derived: **5 non-dominated, 463 dominated** per-file;
  AGGREGATE **36/36 dominated**; combined **499/504**.
- **Committed HEAD `fc23d9a`**: same 13 files; 16 anvil codecs; 9 reference codecs;
  325 data rows; **416 per-file cells** plus **32 AGGREGATE cells**. Re-derived:
  **5 non-dominated, 411 dominated** per-file; AGGREGATE **32/32 dominated**;
  combined **443/448**.
- All five non-dominated rows (both grids) are `generated.json` / `encode_MBps`:
  `anvil-mdl-rans` 0.108/1.060, `-l0` 0.108/1.300, `-l001` 0.108/1.329,
  `anvil-shape-rans` 0.112/1.004, `-l0` 0.112/0.979 (bytes: 89,589 / 89,589 / 89,589
  / 92,300 / 92,300).
- Bracket test (gate R-2 mechanical test): all five satisfy
  `q_lo.ratio <= p.ratio <= q_hi.ratio` and `q_lo.mbps <= p.mbps <= q_hi.mbps` with
  `q_lo = brotli-q11 (0.096, 0.674)`, `q_hi = zstd-19 (0.113, 1.963)`. **5 GAPFILL.
  0 CROSSING.**
- Join check: the re-derived worktree statuses match the landed P0
  `tests/pareto-baseline.csv` (SHA-256 prefix `F432C34445D7DE4A`) on **468/468**
  anvil cells, **0 mismatches**.

**Mandatory co-listed tuple.** Primary (committed HEAD `fc23d9a`): v1 brotli+zstd
class (label required; GRID-THIN) `5 | 5 | 0 | 0 | 411/416` (443/448); **v2 +xz-9e
`4 | 4 | 0 | 0 | 412/416` (444/448)**. Uncommitted worktree `c70179ea`: v1
`5 | 5 | 0 | 0 | 463/468` (499/504); v2 (label required) `1 | 1 | 0 | 0 | 467/468`
(503/504). Any one figure quoted alone is misleading; v1 must carry the brotli+zstd
class label, worktree figures need the grid label.

### 1.2 Frozen per-file gap — RECOMPUTED, Convention A (exact bytes)

"needed" = shortfall of the best-ANVIL row against the best-reference row:
`(anvil_B − ref_B) / anvil_B`, both from `tests/benchmark-suite.csv`.

| file | input | best ANVIL (bytes, codec) | best REF | needed |
|---|---|---|---|---|
| generated.json | 827,664 | 89,589 mdl-rans | 79,172 q11 | **−11.6%** |
| src.cpp | 32,512 | 9,483 dp-arith | 7,991 q11 | −15.7% |
| synth-timeseries.bin | 280,000 | **143,132 sparse-rans** (committed HEAD; worktree `c70179ea`: 140,898 hotop-rlzp) | 120,669 q6/q9 | **−15.7%** (worktree: −14.4%) |
| generated.jsonl | 2,815,267 | 183,506 mdl-rans | 150,619 q11 | −17.9% |
| anvil.exe | 268,800 | 107,468 mdl-rans | 88,322 q11 | −17.8% |
| anvil_bench.exe | 1,929,216 | 830,331 mdl-rans | 646,675 q11 | −22.1% |
| synth-arith.bin | 256,000 | 256,022 dp-arith **r=1.000** | 87,013 q11 | −66.0% |
| doc.md | 2,724 | 1,444 dp-arith | 1,047 q11 | −27.5% |
| synth-jitter.bin | 974,920 | 87,412 mdl-rans | 62,717 q11 | −28.3% |
| generated.log | 1,942,280 | 129,739 shape-rans | 81,946 q11 | −36.8% |
| generated.sqlite | 1,740,800 | 323,014 mdl-rans | 185,702 q11 | −42.5% |
| generated.repeat.jsonl | 940,000 | 873 dp-arith | 162 q11 | −81.4% |
| random.bin | 262,144 | 262,166 dp-arith **r=1.000** | 262,149 q1 | 17 B (control) |

**Erratum C-1 applies to this table's history:** I8 showed −11.3% / −17.2% /
−37.1% / −28.6% / −27.6% for json / jsonl / log / jitter / doc.md by mixing exact
anvil bytes with rounded reference ratios. The table above is Convention A and
supersedes those five cells. All other cells reconcile with I8 and with the brief
within rounding.

**Column semantics (retained from I8 §1.1).** This column is the gap to the **best
reference row**, not the gap to the crossing budget at ANVIL's current decode speed.
The two differ where the binding row at current speed is not the smallest row
(e.g. src.cpp: −15.7% vs q11, but −12.2% against the crossing budget). Both are
correct; they answer different questions. §2.2 is the authority for the second
quantity.

### 1.3 Strong-crossing test — the honest headline

Test: does any anvil row dominate any reference row on **both** throughput planes
(ratio, encode, decode)?

> **NONE. Zero anvil rows dominate any brotli/zstd row on both planes.**

On (ratio + decode) only, exactly **three** rows beat anything, and what they beat is
`brotli-q1` on synth-timeseries: `anvil-hotop-rans` 0.512/150.6,
`anvil-hotop-budget-rans` 0.512/163.6, `anvil-hotop-rlzp-rans` 0.503/158.4 vs
`brotli-q1` 0.579/139.7 (all three are uncommitted-worktree `c70179ea` rows).
Beating **q1** is audit-trap class (d) — not a citable result. So the truthful state of the project on Program A is: 5 EXTENDS_FRONT rows
exist and are real per the sole arbiter; **0 are strong crossings; 463 of 468
per-file cells are dominated** (499/504 including AGGREGATE).

### 1.4 Front thinness — why the five rows exist (recomputed)

Non-dominated reference rows per file and plane (full table; corrected C-2):

| file | encFront | decFront | decFront members |
|---|---|---|---|
| anvil.exe | **8** | 3 | q11 0.329/245.3, zstd-9 0.385/1035.0, zstd-19 0.347/447.6 |
| anvil_bench.exe | 7 | 5 | q11 0.335/205.1, zstd-1/3/9/19 (0.401–0.466 / 631.9–893.5) |
| doc.md | 5 | 5 | q6, q11, zstd-1, zstd-9, zstd-19 |
| generated.json | 6 | **2** | **q11 0.096/507.4, zstd-19 0.113/1506.5** |
| generated.jsonl | 6 | 3 | q11 0.054/867.2, zstd-3 0.087/2003.3, zstd-19 0.059/1984.5 |
| generated.log | 6 | **2** | q11 0.042/947.5, zstd-19 0.055/2092.5 |
| generated.repeat.jsonl | 1 | 1 | zstd-19 0.0/24479.2 |
| generated.sqlite | 7 | **2** | q11 0.107/445.2, zstd-19 0.124/1253.5 |
| random.bin | 1 | 1 | zstd-3 1.0/2699.7 |
| src.cpp | 6 | 3 | q11 0.246/252.8, zstd-9 0.268/1083.7, zstd-19 0.256/953.4 |
| synth-arith.bin | 6 | 3 | q11 0.340/187.5, zstd-3 0.786/885.8, zstd-19 0.416/325.5 |
| synth-jitter.bin | 5 | 3 | q11 0.064/844.3, zstd-1 0.093/3989.0, zstd-19 0.077/2187.4 |
| synth-timeseries.bin | 3 | 4 | q6 0.431/282.2, zstd-1 0.537/945.0, zstd-9 0.535/584.8, zstd-19 0.526/341.1 |

The encode front on generated.json is 6 points but the **decode front is 2**
(q11 and zstd-19). A 2-point front means any point with ratio in (0.096, 0.113) and
decode in (507.4, 1506.5) is non-dominated automatically. The five rows sit in the
encode analogue of that rectangle. This is a property of the shipped reference
configuration space, not of ANVIL. Corrected count: anvil.exe encode front is 8, not
6 (C-2).

---

## 2. The decode-floor arithmetic (the lane's central question)

### 2.1 Reachable bands — the t3 profile and its I9 status

**PR-1 PENDING.** The I8 time-share decomposition is a peer-landed measurement
(`deliverable/t3-decode-floor-profile`, decode-perf; measured on generated.log
mode-15 payload, median-7 interleaved): crc32 **44%**, masks+resid eager
materialization **26%**, block concat/alloc **~15%**, token loop **11%**. This lane
**cannot recompute those shares from a committed CSV** — they are carried as
`[LANDED-PEER, I8]` only, and PR-1 (in flight, task `pr1-profile`) is the measurement
that replaces them for the target cells. The band arithmetic below is pure division
on that input and is labeled `[PROJECTION-UPPER-BOUND]` per theory's review.

| fix set | removed share | lift `1/(1−s)` |
|---|---|---|
| CRC only | 0.44 | **1.786x** |
| CRC + materialization | 0.70 | 3.333x |
| CRC + mat + alloc @75% | 0.8125 | 5.333x |
| CRC + mat + alloc @100% | 0.85 | 6.667x |

**Method validation (retained):** CRC-only gives `1/(1−0.44) = 1.786x`, which
reproduces decode-perf's own stated "~1.79x" cap exactly. **The multi-leg bands are
upper bounds, not estimates** (theory; accepted in I8): lazy materialization *defers*
work rather than deleting it, and concat/alloc is partly a *consequence* of eager
materialization, so the legs are negatively correlated and their combined saving is
**less** than the product. Only CRC-only 1.79x is anchored. This matters because
generated.json's requirement (4.59x with the anti-tie margin, §2.2) is compared
against the 5.33x upper bound; if the true three-leg lift lands below ~4.6x,
generated.json's decode route closes too.

**New in I9 — the anti-tie convention changes the thinness call, and it is now
BINDING (bench `msg_484ead41`, coordinator-ratified).** Decode-route arithmetic uses
three tokens, scoped to decode arithmetic ONLY and never interchangeable with the
frontier verdict vocabulary (FRONT-GAP / FRONT-CROSSING / DEGENERATE remain
research-gate's): **DECODE-GO (C >= R') / DECODE-TIE (R <= C < R') / DECODE-SHORT
(C < R)**, where R' = 1.02 x R from exact rows, C = 1/(1−s) for a single-component
cap, and a wall-clock override applies at >= max(2%, window CV). Under R' the
worktree-grid synth-timeseries requirement is `282.173 x 1.02 / 158.443 = 1.8165x`
(hotop-rlzp 140,898/158.4), so the CRC-only cap 1.7857x is short. **At committed
HEAD the pair is different (C-6): best row 143,132 B at 132.6, bar q9 286.7 ->
`286.65 x 1.02 / 132.645 = 2.204x`; the CRC-only cap is DECODE-SHORT there and the
route rests on the CRC+mat projection (needs s >= 54.6%).** **PR-1a is RATIFIED as
this convention** (thresholds computed on the worktree pair): s >= 44.95% ->
DECODE-GO; 43.82–44.95% -> DECODE-TIE; < 43.82% -> DECODE-SHORT (replaces the old
< 40% wording). The raw strict-crossing requirement is 1.7810x (cap +0.26%), which is
why the TIE band exists. The ratified thresholds were computed on the worktree pair;
PR-1 must state which grid/build its profile measured, and the HEAD pair is already
DECODE-SHORT for a CRC-only cap. Frontier claims still need the PR-4 window + two
hash-identical arbiter runs + bench sign-off.

### 2.2 Crossed against the per-file gaps — the practical target list (recomputed)

Decode bar = strictly exceed the max decode among references whose ratio ≤ the row's
ratio. Byte bar = strictly beat the min ratio among references at least as fast as
the row's current decode. Row = best-ANVIL by min ratio (ties: min bytes), per file.

| file | anvil row (ratio, dec MB/s) | **decode bar** | **lift needed** | band that clears |
|---|---|---|---|---|
| **synth-timeseries.bin** (HEAD `fc23d9a`) | sparse-rans 0.511 / 132.6 | 286.7 (q9) | **2.16x** raw / **2.20x** slacked | none (CRC-only 1.79x short; CRC+mat 3.33x projection) |
| synth-timeseries.bin (worktree `c70179ea`) | hotop-rlzp 0.503 / 158.4 | 282.2 (q6) | **1.78x** raw / **1.82x** slacked | CRC-only (1.79x) - raw only |
| **generated.json** | mdl-rans 0.108 / 112.8 | 507.4 (q11) | **4.50x** raw / **4.59x** slacked | **CRC+mat+alloc@75 (5.33x), PROJECTION upper bound** |
| synth-arith.bin | dp-arith 1.000 / 369.9 | 885.8 (zstd-3) | 2.39x (row-dependent 2.27–2.44x) | CRC+mat (3.33x) — **decode route DEGENERATE, §2.4** |
| generated.jsonl | mdl-rans 0.065 / 173.2 | 1984.5 (zstd-19) | 11.46x | none |
| generated.log | shape-rans 0.067 / 203.0 | 2092.5 (zstd-19) | 10.31x | none |
| generated.sqlite | mdl-rans 0.186 / 122.3 | 1253.5 (zstd-19) | 10.25x | none |
| anvil.exe | mdl-rans 0.400 / 89.1 | 1035.0 (zstd-9) | 11.62x | none |
| anvil_bench.exe | mdl-rans 0.430 / 77.1 | 774.7 (zstd-3) | 10.05x | none |
| synth-jitter.bin | mdl-rans 0.090 / 165.7 | 2187.4 (zstd-19) | 13.20x | none |
| random.bin | dp-arith 1.000 / 319.6 | 2699.7 (zstd-3) | 8.45x | none (DEGENERATE) |
| src.cpp | dp-arith 0.292 / 26.7 | 1083.7 (zstd-9) | 40.55x | none |
| doc.md | dp-arith 0.530 / 19.9 | 469.7 (zstd-1) | 23.62x | none |
| generated.repeat.jsonl | dp-arith 0.001 / 234.4 | 24479.2 (zstd-19) | 104.45x | none |

Row-choice note: I8's §2.2 quoted 2.44x for synth-arith (greedy-arith) and 8.42x for
random.bin (greedy-arith); the same cell gives 2.27–2.44x and 8.42–8.45x depending on
which row is cited. Marginally better than I8 for src.cpp (40.55x vs 40.6) is
rounding.

**Read this correctly — two cells, and only two, are reachable on the decode plane
under the t3 profile at unchanged bytes:**

- **synth-timeseries.bin: grid-dependent (C-6).** Worktree `c70179ea`: 1.78x raw
  (1.82x anti-tie) needed vs the 1.79x CRC cap — razor-thin (+0.26% raw margin).
  Committed HEAD `fc23d9a`: 143,132 B at 132.6 needs **2.16x** (2.20x slacked); the
  CRC-only cap is short and the route rests on the CRC+mat projection (3.33x upper
  bound). Still the smallest lift on the board, but not a 1.79x-cap crossing on the
  committed grid. PR-1 must measure and state its grid.
- **generated.json: 4.50x raw (4.59x anti-tie) needed vs ≤5.33x projected** (worktree
  `c70179ea`; committed HEAD: 4.34x raw / 4.43x slacked — bar q11 651.8, row dec
  150.1). Non-degenerate (ratio 0.108 is real compression), and at
  decode ≥ 517.5 MB/s the binding reference flips to zstd-19 (0.113), which ANVIL
  already beats on ratio at 0.108; the byte budget there is 93,526 B, giving 3,937 B
  of slack over the current 89,589 B row (recomputed; this is theory's step,
  reproduced). Genuine **if** the three-leg lift lands in that band.

Everything else (8.4x–104x) is out of reach of any wire-invisible decode fix. For
those cells the **byte** axis is the only axis, and the bars are §1.2.

### 2.3 Route-B recomputed vs the committed ledger column

Re-implementation of `prototypes/i8-theory/iso_crossing.py` (2% slack; NOT run):
smallest decode speed at which `N x R_ref(t) > best-anvil bytes`.

| file | my recompute (x) | ledger PART XIII §3 (x) |
|---|---|---|
| synth-timeseries.bin | **1.82** | 1.8 |
| synth-arith.bin | 2.46 | 2.5 |
| generated.json | 4.59 | 4.6 |
| random.bin | 8.59 | 8.6 |
| anvil_bench.exe | 10.08 | 10.1 |
| generated.sqlite | 10.46 | 10.5 |
| generated.log | 10.52 | 10.5 |
| generated.jsonl | 11.68 | 11.7 |
| anvil.exe | 11.85 | 11.8 |
| synth-jitter.bin | 13.46 | 13.5 |
| doc.md | 23.69 | 23.7 |
| src.cpp | 40.55 | 40.5 |
| generated.repeat.jsonl | 106.54 | 106.5 |

Every cell agrees within 0.05x; the ledger table is settled. **12 of 13 are closed**
at the ~1.79x wire-invisible ceiling (on the worktree grid; on committed HEAD the
smallest multiplier is 2.20x, so the CRC-only cap closes none); synth-arith's 2.46x
is DEGENERATE (ratio 1.000); **synth-timeseries is the only live, non-degenerate
Route-B target** under the extended CRC+mat projection. The table is computed on the
worktree grid `c70179ea`; committed-HEAD recompute (same convention, different
timing session): anvil.exe 11.97, anvil_bench 11.13, doc.md 29.20, generated.json
**4.43**, jsonl 10.22, log 8.54, repeat 77.26, sqlite 9.93, random 8.27, src.cpp
28.61, synth-arith 2.64, jitter 14.33, synth-timeseries **2.20** (sparse-rans; 2.31 if
anchored to the sparse-channels-rans tie). Every multiplier is grid/session-labelled;
all remain PR-4-unattested.

### 2.4 The synth-arith corridors — RECOMPUTED (one I8 spot-check corrected)

Re-implementation of `prototypes/i8-arch/crossing_check.py` convention (the arbiter
predicate applied to the bench harness's 3-decimal quantised ratio; NOT run):

| decode band | max bytes still non-dominated | binding ref |
|---|---|---|
| ≤ 187.518 MB/s | **86,911 B** | brotli-q11 (0.340) |
| (187.518, 325.534] | **106,368 B** | zstd-19 (0.416) |
| (325.534, 885.813] | **201,088 B** | zstd-1/3/9 (0.786) |
| > 885.813 MB/s | no reference row is fast enough; any row is non-dominated | — |

Corrected spot checks (C-2): 87,013 B @187.0 **DOMINATED (q11)**; 87,013 @188.0
EXTENDS_FRONT; 89,363 @187.0 **DOMINATED (q11)**; 89,363 @188.0 and @200.0
EXTENDS_FRONT; 106,000 @200.0 EXTENDS_FRONT; 107,000 @200.0 DOMINATED (zstd-19);
today's best row 256,022 @389.7 DOMINATED (zstd-1).

**What this does and does not mean.** (i) The corridor above 187.5 MB/s is real and
19.5 KB wide: the mode-16 **harness wire at 89,363 B** would cross at any decode
≥ ~187.5 MB/s, and the current ratio-1.000 rows already decode at 323–390 MB/s.
(ii) **The harness wire is not a codec row** — it is an experimental count, absent
from the suite; nothing crosses until an end-to-end container row exists and the
arbiter sees it. (iii) **Decode-plane crossings on this cell are DEGENERATE** (row
ratio would be ~0.349 only if the container lands at ~89,363 B; at ratio 1.000 any
decode row is store-vs-fast-store — gate R-2/DNB-M3). (iv) The dual-bar rule binds:
the 87,013 B bar is brotli declining a filter it ships elsewhere; pnra's transformed
ladder puts brotli-q11 on delta+zigzag-varint bytes of the same file at 16,313 B
(`[LANDED-PEER, I8, reproduced byte-exactly by I8; not recomputable from a committed
CSV by this lane]`), i.e. the real gap is ~−93.6%, and any synth-cell result must be
reported against **both** bars. A landed prototype now sits deep inside this
corridor: pnra's per-region step wire is 12,936 B at 462.3 MB/s (15.5x inside the
201,088 B bar; `[LANDED-PEER]`, `prototypes/i9-pnra/results-i9-pnra.csv`, binary
and input hashes verified by this lane) — prototype wire, NOT an ANVIL row,
{synthetic}, PR-5 PENDING; see Rank 1.

### 2.5 synth-timeseries structure — CORRECTED (C-3)

Generator reconstruction (`tests/make_synth_corpus.py`; this is the validator):

- `synth-timeseries.bin`: **20,000 records x 14 B = 280,000 B**;
  `struct.pack('<QfH', ts_ms, value_f32, rec_id_u16)`; `struct.calcsize('<QfH')`
  returns **14**. True record period **P = 14**.
- `synth-columnar-align.bin`: **12,000 rows x 23 B**; row layout u16/u32/u64/u32/u16/
  u8/u8/u8 packed as `<HIQIHB` + 2 bytes; true **P = 23**; 184 = 8 x 23.
- The I8 §2.7c "28-byte records / 10,000 / P=28/184" and its distinct-offset
  percentages came from **stride aliasing**: counting distinct values or constant
  offsets at multiples of the true period reproduces the same signature at 2P (28 =
  2 x 14). **Methodological lesson: a distinct-offset / autocorrelation period
  estimate is only an estimate — it must be validated by exact generator
  reconstruction (field layout, `calcsize`, per-record packing), never promoted to
  an anatomy fact on its own.** Coordinator `msg_30351ec7` (verified): P=14
  (20,000 records) and P=23 (12,000 rows); constant-byte fraction 28.6% for
  timeseries and **21.7%** for columnar-align (I8's 30.4% was the alias artifact).
  datastruct independently reconstructed the same values from the generator seed
  (0/20,000 id mismatches under the 14-B parse; at true stride 23 there are 5
  constant offsets = 21.7%; the 184-stride 56-offset reading is aliasing — e.g.
  ts_ns advances +8,000,000 per 8-row group and 8,000,000 = 0 mod 256).
  **PENDING-LEDGER** (research-gate, PART XIV) for the formal record; until then the
  I8 §2.7c shares are superseded by these corrected values.
- **Grid-dependent (C-6):** worktree arithmetic −14.4% bytes or 1.78x decode
  (hotop-rlzp 140,898); committed HEAD −15.7% bytes or 2.16x (143,132 at 132.6 vs
  q9 286.7). The mechanism target (a record-period delta inside the reference) is at
  P=14; the gate ruling stands that record period is infrastructure, not novelty
  (xz --delta covers 1..256, which includes 14 and 23).

### 2.6 The pnra ladder, the gate's exactness boundary, and the asymmetry

**The ladder (carried).** `[LANDED-PEER, I8]` pnra's transform ladder on
synth-arith.bin, scored by brotli-q11, reproduced byte-exactly by I8's lane: raw
87,013; T1 delta u32 17,311; T2 delta+zigzag varint 16,313; T9 delta+lane-split
~15.9–16.2 KB convention-dependent. The coordinator's decisive experiment: ANVIL's
own codec on the **pre-transformed** T1 buffer = 17,301 B vs brotli-q11 17,311 B —
so the synth-arith byte gap is **not an entropy-coding deficit**; it is
parser/transform discovery. `[Mandatory caveat: pre-transformed input, transform
uncharged, NOT an ANVIL codec result.]`

**The gate's exactness boundary (binding on I9).** ARI-STRIDE's pre-hoc estimator
experiment measured residual entropy growing ~0.65–0.69 bits/doubling with reference
distance across two decades; the gate amended G4: **zero-bit reference-derived
transform parameters are defensible only for EXACT transforms** (zero residual by
construction, e.g. TCOPY's Δ=−d). On statistical transforms the
transform-reference family has **no defensible novelty position**; the A1–A4
ablation is retired for that family. Consequence for the I9 `pnra-stride` task:
build the native delta/stride form as engineering under a declared form, or not at
all as novelty. This lane issues no verdict; it records the boundary.

**The asymmetry, stated correctly.** ANVIL loses on bytes by 11.6–81.4% AND on
decode by ~1.7x–104x (grid-session dependent). The decode gap is closable on exactly
**two** cells under the extended (CRC+mat, 3.33x upper-bound) projection —
synth-timeseries (1.78x worktree / 2.16x HEAD) and generated.json (4.50x / 4.34x via
the projected three-leg fix); the CRC-only 1.79x cap alone closes **no** cell on the
committed grid, and 12 of 13 files are decode-closed by arithmetic (DNB-M1). So
decode work is a **two-cell strategy**, and every other cell is byte-blocked until a
ratio mechanism lands.

---

## 3. Audit findings (self-deception, named)

- **AUDIT-1 (CLOSED, I8).** "0 EXTENDS_FRONT in project history" was false; 5 exist.
  Reconfirmed this pass from the current artifact (13 files x 18 codecs grid; 5 ND).
- **AUDIT-1b (CLOSED, I8).** The five rows are a front-knee artifact (encode-plane
  rectangle), not a crossing. Reconfirmed: all five are FRONT-GAP by the mechanical
  bracket test.
- **AUDIT-2 (open -> PENDING).** No I8 end-to-end decode number landed (mode-16 /
  ARI-REF never produced an arbiter row); nothing new to audit. I9 trap watch
  remains active: (b) decode win on a grown wire, (e) Linux number as Windows
  number, (f) EXTENDS_FRONT without the arbiter.
- **AUDIT-3 (standing).** `docs/CONTEXT.md` Linux/EPYC numbers are not citable on
  the Windows line. Struck on sight.
- **AUDIT-4 (CORRECTED by C-1).** The I8 "brief used rounded ratios" erratum was
  itself a convention mix; the brief's exact-byte figures were right. The standing
  rule (recompute at write time) caught it one iteration later, in this lane.
- **AUDIT-5 (RULED, PR-2).** Degenerate escape: any non-dominated row with
  **row ratio ≥ 0.95** is DEGENERATE, never a crossing. Adopted by the gate (R-2);
  arithmetic applies to the ROW, not the file's current best row.
- **AUDIT-6 (open, sharpened).** Measurement isolation. Recomputed from
  `tests/noise-floor.csv` (worktree): reference decode CVs remain large — doc.md
  brotli-q4 **41.8%**, generated.jsonl zstd-9 **38.5%**, doc.md zstd-19 **33.8%**,
  jsonl brotli-q9 26.7%. The synth-timeseries call has a raw margin of **0.26%** and
  an anti-tie shortfall of **1.7%**; it is not decidable without PR-1 (target-cell
  shares) and PR-4 (window attestation).
- **AUDIT-7 (new, C-3).** Record-period aliasing: distinct-offset counting aliases
  at integer multiples of the true period (28 = 2 x 14; 184 = 8 x 23). Validator =
  exact generator reconstruction. Lane: pnra/datastruct (facts), coordinator (caught
  it), research-gate (ledger PART XIV). Corrected values PENDING-LEDGER.
- **AUDIT-8 (new, C-1).** Convention mixing in gap percentages: exact numerator over
  rounded denominator is neither "exact-byte" nor "rounded-ratio" and was written as
  exact. Frozen convention recorded in C-1. Lane: strategy (self).
- **AUDIT-9 (new, C-4).** Ratio-first decode figure provenance: the carried
  "11.2x / 13.0 MB/s" is not recomputable from the current ratio-first CSVs
  (recomputed: 20.96x / 6.73 MB/s enwik8; 20.33x / 7.20 MB/s on the 7 BWT-routed
  Silesia files; 14.60x / 9.43 MB/s whole-Silesia auto). Either the gap is twice as
  deep as carried, or the current timings were taken under a different window —
  PR-4 decides. Lane: bench (flagged), decode-perf (PR-1 covers BWT files).
- **AUDIT-10 (new, C-5).** Grid provenance: worktree 18-codec grid (468 cells) vs
  HEAD 16-codec grid (416 cells), same five ND rows. Any tuple must name its grid.
  Lane: bench (p0 refresh defines the successor), strategy (labelling).
- **AUDIT-11 (new, build integrity).** An uncommitted dirty-tree refactor
  (`decode_one_block`) broke ALL decode paths in the current build while `HEAD`
  remained correct (format, msg_774c45e3; arch's lane). Any new byte/roundtrip
  claim from the dirty build is invalid until a green fix lands. Existing CSV
  figures predate the refactor and stand; future measurement windows must record
  binary identity (PR-4) so this class is detectable. Reason class:
  implementation-era. Lane: arch (fix), bench (windows), format (fuzz gate).
  **RESOLVED 2026-09-12:** arch's fix confirmed by format's green gate on
  `8EAE1FB3...` (zero skips, src `62BC6631...`); re-verify pin applies if
  `src/anvil.cpp` changes.

---

## 4. Ranked lanes by expected Pareto movement per unit of remaining effort

LANDED measurements only; no mechanism ranked above a measurement. Ranks 1–4 cover
the four landed/target routes on the three live cells of Program A (synth-arith
byte, synth-timeseries decode, synth-timeseries byte, generated.json decode); ranks
5–6 are parallel lanes justified by landed problem measurements; rank 7 is hygiene.

**Rank 1 — native per-region step reference on synth-arith.bin (pnra -> arch;
engineering form, PR-5 ruling PENDING) — the strongest landing of I9.** `[GUARD,
carried at every mention: prototype wire, NOT an ANVIL row, {synthetic}, supplied
per-region structure, PR-5 PENDING, no frontier claim; the timer is measured
(parallel window; prototype, not suite), hence ranking-grade only under PR-4 §3.]`
Measured (`prototypes/i9-pnra/results-i9-pnra.csv`; binary `stride_ref.exe` sha256
`bae2ba01...` and the synth-arith input hash verified read-only by this lane):
pack3/derived wire **12,936 B (ratio 0.0505)** at **462.3 MB/s** median decode
(CV 17.3%), window `w-pnra-20260912T062115Z`; all parameters charged (floor
12,667.5 B + 268.5 B). The synth-arith corridor above 325.5 MB/s is 201,088 B, so
the wire is **15.5x inside the binding bar and 8.2x inside the strict 106,368 B
bar**; at those coordinates a landed row would be non-dominated and outside every
R-2 bracket (no reference is smaller than 0.0505) — classification belongs to
research-gate after integration, exactly as with the datastruct contingency
(PR-3-Q1 filed). Controls: literal-only arm 256,049 B @ 587.7 (DOMINATED by zstd-1,
DEGENERATE); no-coder arm 16,134 B (representation alone is inside); coder choice
<=20% bytes and >=2x speed; derived step costs +15 B vs transmitted; selftest,
separate-process roundtrips, and 160/160 fuzz arm-cases PASS. Gating: (i) PR-5 RULED (I9-2): buildable as **{engineering}/adopt, NO novelty
claim**; the P-3.4 causality condition and C1–C8 bind, and the dual bar on this
cell is **87,013 / 16,313 B**, which the gate VERIFIED the prototype **clears**
(12,936 < 16,313) — unlike datastruct, the dual-bar qualifier does NOT strike a
landed row here; transmitted 12,921 beats derived 12,936, so novelty resolves
NEGATIVE and capability POSITIVE; per the ARI-REF pre-registration a crossing here is
**CLASS-SPECIFIC, not a general-purpose victory claim**;
(ii) no ANVIL row exists — integration is arch's (default-off flag
with byte-identity gate), then bench suite row + two hash-identical arbiter runs +
quiet-window PR-4 for citation; (iii) {synthetic} — mechanism probe, not
generalization.
**Auto-discovery LANDED (byte acceptance; `msg_e0169153`,
`prototypes/i9-pnra/AUTOSEG-RESULTS.md` + `autoseg-results.csv`):** run boundaries +
per-run step/base ARE discoverable from data alone with ALL discovery cost charged —
synth-arith auto compact wire **12,920 B = 0.9988x the 12,936 oracle** (discovery
table 24 B; 8/8 regions, 7/7 boundaries exact; byte-identical to the frozen
artifact); 10/10 randomized anatomies: 166/166 boundaries exact, auto == forced-oracle
on every file. Honest bounds: drift-stride / generated.log / synth-counters / random
find zero step regions >=6 words -> literal fallback +48 B, no gain (brotli-q11
6.1x/23.7x/11.0x smaller); +-2 jitter fragments (915 regions, 0.807) with the exact
forced form aborting; escape-derived boundaries cost +3,198 B vs 24 B transmitted.
selftest + 240 fuzz cases + separate-process roundtrips PASS. Still adopt-class
{engineering}, no novelty, no crossing language; **bench co-run timing folded in (ranking-grade, `w-bench-pnra-autoseg-20260912T091754Z`,
1t core19 HIGH): synth-arith 450.5 MB/s CV 11.05%, drift-stride 590.5, log 481.1;
pre-window core util 14.16% fails the <5% gate so absolutes are not citation-grade;
task CLOSED — only arch integration remains if prioritized.** This solves auto-discovery for
the step-region class on this {synthetic} cell (datastruct separately closed its own
field-partition gap with +39 B/+31 B charged overhead; see Rank 3). **Standing change (coordinator
`msg_70f58369`): pnra moves from oracle-assisted to SELF-DISCOVERING in its
generative class; Rank 1 stands and strengthens** (still {engineering}, no novelty,
no crossing language; **research-gate VERIFIED, PART XIV A5 — auto v1 wire sha
`0EAFB385...` byte-identical to the frozen artifact; closes the "PR-5 wire was
oracle-dependent" caveat for the generative class; datastruct's A6 independently
VERIFIED — **the record has no unverified auto-discovery cell**).
**Paired outcome (coordinator `msg_f4285e02`):** BOTH synthesis-cell mechanisms are
now self-discovering — pnra in its generative class, datastruct on fixed-period
synthetic files — but **NEITHER has demonstrated transfer to real structured data**
(datastruct's non-synthetic bound is 5.4–5.8x worse than brotli-q6; pnra's failure
modes find zero step-regions on drift-stride/log/counters/random). **The real-data
and decode gates stand; both are `{synthetic}`-class, adopt-class engineering, no
novelty.**

**Rank 2 — decode MATERIALIZATION leg (PR-1 DONE: CRC-only DECODE-SHORT; the
pre-committed branch fired; generated.json is now the sole decode lead).**
**Bench canonical re-measure verdict (ranking-grade only, `msg_bfc0e8c5`;
`tests/pareto-i9-decode-remeasure.md`): generated.json mode-10 mdl 189.146 MB/s ->
DECODE-SHORT** (mat cap 2.058 vs R' 2.779; only a MEASURED combined mat+alloc leg —
projection 3.69 — would clear); **synth-timeseries hotop-rlzp 177.891 MB/s ->
DECODE-TIE** (cap 1.456 vs R' 1.6809; decode-perf's 216.9 window gives GO; within
1 sigma, needs a paired core-gated re-measure on one frozen binary). Pre-window
pinned-core load 8.27% -> both are RANKING-grade; citation-grade reruns queued. **No
frontier flip: q11 still dominates all 5 GAP rows; tuple unchanged. The decode-only
route is therefore **CLOSED — MAT FALSIFIED as a decode LOSS** (arch paired
interleaved reps=7, core18, 1t, identical wire: generated.json mode-10 mdl
4.1545 -> 4.2117 ms, **+1.4%**; synth-timeseries hotop-rlzp 1.4495 -> 1.8499 ms,
**+27.6%**, CV 9.5%). Cause: the t3 mat 51.4%/31.3% shares are NOT removable
liveness overhead — entropy-decode work is unchanged and per-symbol StreamPull
dispatch costs more than bulk decode + vector indexing. **IMPLEMENTATION-ERA, not
math** (`msg_af51ded9`). ALLOC is built on MAT and awaits a window (ALLOC-only hunks
or whole-leg falsification). **decode-perf ranking update (`msg_0f4f82a6`): both
PR-1 cells close on the decode plane (PART XIV A10); the remaining removable set is alloc/concat
ONLY — synth hotop-rlzp 5.1% -> C=1.054 vs 1.29 needed; synth hotop C=1.121 vs
~1.45; generated.json mode-10 mdl 28.3% -> C=1.394 vs 2.44 (baseline) / 4.59 (frozen
CSV); the ONLY live decode program left in I9 is the BWT postcoder leg-4 (~1.38x
aggregate). Labels: arch A/B measured, caps derived; no crossing claims.**
**ALLOC-only LANDED, wire-identical (arch `msg_a1bf3122`; paired interleaved reps=7
core18 1t): generated.json anvil-mdl-rans 4.1604 -> 3.0766 ms (**1.352x**);
synth-timeseries anvil-hotop-rlzp-rans 1.4829 -> 1.3802 ms (**1.074x**) — still
SHORT (1.352x << R'=2.35–2.78x; 1.074x < 1.30x), so materialization cannot close
the decode route; MAT+ALLOC retired implementation-era. Source is now ALLOC-only
`src 38409E26`, gates green (442/442 roundtrip, fuzz 50 PASS, encode identity
494/494); canonical shas `build\anvil.exe` **E8AA2E48** / `build\anvil_bench.exe`
**56092B9B** (supersede `0D1E130B`). Encoder wire identical (494/494), so
compressed-byte totals are unchanged; `w-bench-recon` on `A0895B1F`/`62BC6631`
stays byte-valid with its sha note. ALLOC numbers are RANKING-grade until bench
attests the window (`msg_37afd9da`). **Gate PART XIV A12:** ALLOC is a retained
wire-identical partial win but DECODE-SHORT; **compressed-byte results carry with
their originating sha labelled, decode/timing stay sha-specific**; net decode status
A3 / A8 / A9 / A10 / A12 with only the BWT postcoder leg-4 live. Next: decode leg 4
(postcoder two-level cumulative tables + buffered renorm).** In the current ordering BWT inverse + postcoder
(Rank 4 below) is the structural path.** Bench queue v2 (`msg_1a57445a`): **arch MAT
before/after A/B is the next timing slot — it decides whether ALLOC is required**;
then pnra autoseg, decode-perf post, format byte-gate, bench 9-cell recon; no window
opens until bench posts the byte-verify stop, and builds/gates/fuzz hold during every
w-* window (contract v1.2).
**PR-1 DONE (decode-perf `msg_e56eb71d`; `deliverable/pr1-profile` +
`prototypes/i9-decode-perf/REPORT.md`).** Measured CRC share on synth-timeseries
(hotop / budget / rlzp, median-7, I8 bytewise base): **26.3 / 28.7 / 27.0%**,
C = 1.36-1.40x vs the 1.78x requirement — 14+ points below the 43.82%
DECODE-SHORT line. Root cause: CRC share scales with decode speed, so I8's 44%
(generated.log) never transferred to the slower timeseries row. Current path
(worktree binary `5DB16A1C`; PCLMUL CRC already landed, 0.067 ns/B, share
1.3-3.4%): the remaining cost is **materialization**. Measured: synth-timeseries
hotop-rlzp 216.9 MB/s (mat 31.3%, need 1.30x); generated.json mode-10 mdl
220.5 MB/s (mat 51.4% + alloc 21.5%, need **2.44x**; supersedes the earlier 2.35 —
gate A10 one-value note). mat-only single-component cap
2.06x on mdl; mat+alloc needed together (cap 3.69x,
`[PROJECTION-UPPER-BOUND]`). Contingency, stated BOTH ways: **fresh measurement (220.5 MB/s) -> 2.44x required
(supersedes 2.35; gate A10), which CLEARS the 3.69x mat+alloc projection** (the mat-only 2.06x cap would not
clear it alone); **frozen arbiter row (112.8 MB/s) -> 4.59x required, which clears
neither cap** — bench's canonical re-measure decides which row is the citation of
record. Handoff to arch: build the MATERIALIZATION leg
first (no more CRC work), cheapest staging on synth-timeseries hotop-rlzp, branch
cell generated.json mode-10 mdl, then the alloc leg. All throughput here is
prototype-measured, PR-4-unattested.

**Rank 3 — native columnar residual contexts on synth-timeseries.bin (datastruct ->
arch), a landed capability target (co-ranked 3–4; Rank 4 leads the pair on landed
evidence).** `[GUARD, carried at every
mention in this document: prototype wire, no ANVIL row exists, {synthetic}, period
+ partition supplied, auto-discovery unsolved.]` The prototype wire is
89,877 B (ratio 0.3210; wire size and SHA-256 recomputed by this lane from
`prototypes/i9-datastruct/out/ts.ref_field` read-only, sha256[0:16]
`56b60452d61b8416`), 25.5% below the q6 byte bar
and 36.2% below the worktree ANVIL row (37.2% below the committed-HEAD best,
143,132 B), with a clean ablation: mechanism gain 97,559 B vs
coder gain 19,761 B, and order-1 alone (187,436 B) is worse than q6. Placement is
inside the reference (per-column, reference-conditioned residual contexts for copy
tokens at the record period) — the placement the retired lane-transpose negative
demands. Gating risks: (i) discovery LANDED with charged overhead (`msg_a11e454e`;
`prototypes/i9-datastruct/RESULTS-discovery.md`): ts AUTO **89,916 B** vs oracle
89,877 (**+39 B**; P=14, partition recovered exactly 0:8:prev,8:4:prev,12:2:posmod);
ca AUTO **24,026 B** (P=23, the 184 alias REJECTED; **29 B better** than the
hand-supplied layout); both roundtrip OK; **gate-verified PART XIV A6** (ts.auto sha
`90a87fe1...`, ca.auto sha `28858ad1...`; discovery state charged in the AC2 header,
no decode-side search); positive control generated.repeat.jsonl -> P=235 all-const;
**honest non-synthetic BOUND, not a success:** generated.jsonl AUTO 1,199,898 B
(5.8x worse than brotli-q6 206,840, 6.5x anvil-mdl 183,506; phase instability
independently reproduced 235/235 phases, H=7.76) and sqlite P=140 **proxy-only**
1,359,687 (5.4x) — measured
cause: phase instability (line lengths 229-250, uniform line-start phase), i.e.
per-record alignment is outside the fixed-period model; (ii) a native integration is a new feature in
arch's serial lane with unmeasured decode cost; (iii) `{synthetic}` generator
caution binds. **PR-3-Q1 ANSWERED (I9-3 §6): mechanically FRONT-CROSSING on the raw
grid, STRUCK by the dual-bar qualifier -> FRONT-GAP (dual-bar) at 89,564 / 80,650 B;
never a crossing.** Novelty ruling (I9-3 §5): **NO mechanism-level novelty** —
engineering/capability + placement; buildable as adopt-class for arch under the I9-2
C-conditions with the native-cost caveat, no novelty claim, no crossing language.
The honest target on this cell is the transform-enabled bar, not the raw q6 bar;
projections/prototype wires never alter the tuple.

**Rank 2b — three-leg decode fix -> generated.json crossing (MERGED into Rank 2 by
PR-1's landed result; same lane).** Projection context retained: 4.50x raw / 4.59x
anti-tie needed vs a ≤5.33x `[PROJECTION]` for CRC+mat+alloc@75% (superseded by the
measured mat 2.06x / mat+alloc 3.69x caps in Rank 2). Non-degenerate; at the required speed the binding reference flips
to zstd-19 (0.113), which ANVIL already beats at 0.108, and the byte budget is
93,526 B (>89,589). Effort: medium-high (three staged legs, each needing
byte-identity + arbiter). Sequencing gate: legs staged CRC -> mat -> alloc so a
partial result is attributable; the upper-bound caveat means a landing below ~4.6x
closes this route too.

**Rank 4 (current ordering: Rank 2) — faster BWT inverse + postcoder — the
structural path; LANDED prototype.**
`libsais_unbwt_aux` + encoder I-array = **3.40–3.55x same-core-count** (dickens
20.64 -> 70.18, webster 18.36 -> 64.15, enwik8 17.00 -> 60.33 MB/s; reps=7, window
`w-bwtinv-20260912T064334Z`, CV 4.8–13.3%) at ~2–5 KB wire per whole-file block;
byte-identity verified to 100 MB (`bwt_aux` BWT == bwt, I[0] == primary, decode ==
raw). `aux_omp` 16t adds to 27–29x but is **THREAD-IMBALANCE context-only**;
`libsais_unbwt_omp` was falsified (init-only parallel, no walk gain).
`[GUARD: measurement contract v1.2 (bench ruling, coordinator `msg_eeb8079d`) — the
3.40-3.55x DIRECTION is ranking-grade-accepted; the absolute MB/s are NOT citable
until a core-gated rerun (citation-grade = all fields + affinity + pinned-core mean
<5% pre-window + same-window paired arms + reps>=7 + effect >= max(2%, CVs) + load
<=5pp over ambient). Windows are EXCLUSIVE: during any w-* window all builds, gate
runs and fuzz must hold. The aux I-array charge is exactly **19,328 B for the 7-file Silesia portfolio**
(dickens 2492 / mr 2436 / nci 4096 / osdb 2464 / reymont 3236 / webster 2532 / x-ray
2072; policy r = smallest power of two with r*1024 >= n) and **3,052 B for enwik8** —
~0.96% of the 2,009,105-B xz win margin — and must be co-listed with any ratio/net
claim (exact values per coordinator `msg_dbb69756`; the ~40 KB worst-case bound is
SUPERSEDED; gate spot-check PENDING). No
ANVIL row until arch wires the 1t aux form behind a new wire version/id; a
consolidated citation-grade window is scheduled by bench once `src/anvil.cpp` is
frozen.]`
DERIVED end-to-end with decode-perf's split: dickens 1t 1.71x / 16t 2.37x; webster
1.82x / 2.65x; BWT-only portfolio ~24.6 (1t) / 36.1 (16t) MB/s. **Critical gate
finding (06 §F0.1 bar): the postcoder floor is 24.9 (dickens) / 35.5 (webster) MB/s
— 40+ MB/s needs a postcoder leg too, not just the inverse transform.**
PR-1 measured the BWT decode split on dickens|webster: **libsais_unbwt 63.3|69.4% +
postcoder 40.1|35.2%**, everything else <1% — BWT decode is an **inverse-transform
problem**, not postcoder tuning. Program B's decode axis is the binding constraint
on a measured byte win: recomputed BWT decode 6.73 MB/s enwik8 / 7.20 MB/s (7
Silesia files) / 9.43 MB/s whole-Silesia auto vs Brotli 137.7–146.3 and xz 74.4–82.8
(C-4 provenance settled: Silesia auto 14.60x / 7-routed bwt-backend 11.24x variants, PR-4-pending).
**Postcoder leg VERDICT (`msg_771c5169`; window 08:40–08:56Z, canonical
containers, lane `post_prof_wt` 8DD0B8B8, median-7, 1t):** adaptive
arithmetic+Fenwick token/run decode is 91–93% of the postcoder (postcoder =
52.1/36.4/37.8% of whole decode on dickens/webster/enwik8; MTF 4–6%, output 3–4%,
init ~0.03%). **Cheap wire-invisible route = DECODE-SHORT** (buffered-renorm
C=1.021/1.014/1.013 vs R'=1.617/1.137/1.436; required token speedup
1.66x/1.13x/1.46x, ~1.38x portfolio); **CORRECTED (decode-perf `msg_129bc263`: those
assumed a FREE unbwt; stage-correct k_p = 6.05x dickens / 3.86x webster / 6.60x
enwik8-est for the 40 MB/s BWT-only bar, while leg-4 delivered a stage factor of
only ~1.67-1.73x — the falsification is STRONGER: arithmetic+Fenwick needs ~4-7x,
and a static-table postcoder ID is the only apparent route)**; ID3 raw is 5–6.6x faster on the postcoder but
+52–101% bytes (selection-only). Spec to arch: two-level cumulative tables +
buffered renorm, wire-identical, must reach ~1.4x aggregate — else the only route is
a new postcoder ID with transmitted static tables (format change). **Synthesis
consequence (coordinator `msg_9109f9b0`): the decode axis is now COMPREHENSIVELY
CLOSED on the cheap wire-invisible routes (CRC DECODE-SHORT (A3); **MAT FALSIFIED as a decode
loss / RETIRED** (implementation-era: on-demand pull dispatch; PART XIV A9);
postcoder cheap route DECODE-SHORT (A8). The only remaining decode options are (a) a wire-identical
two-level cumulative table + buffered renorm inside `bwt_arith_decode` targeting
~1.4x aggregate (leg 4), or (b) a NEW postcoder ID with transmitted static tables — a format
change requiring format + gate. Re-ranked: BWT inverse + postcoder remains the
structural path; materialization is closed on the cheap route; no crossing claims.**
The inverse-transform prototype LANDED (above); next is arch's cheap 1t aux
integration + a postcoder leg. This lane changes no Program A
bytes; it is the only route that preserves Program B's byte win while moving its
decode multiple.

**Rank 5 — P4.1 DEFLATE reconstruction (deflate) — COMPLETE, verdict GO
({engineering}; adopt-class, no novelty).** Gate `docs/gate-ruling-i9-p41-deflate.md`:
**verified GO DIRECTION only; the compression claim is PENDING roundtrip (encode-only
on `DA24665C`); adopt-class prior art, no novelty.** `[GUARD: prototype wire, encode-only on
build `DA24665C` — the decode blocker prevents any roundtrip claim; no ANVIL row
exists.]` Measured: mozilla **13,806,173 -> 12,443,996 B** = **1,362,177 B recovered**
(3.17x the ~430 KB go-bar), beating mozilla xz -9e (13,376,248) by **932,252 B**.
Census: mozilla 2,564 candidates -> 2,354 replay-verified DEFLATE streams
(C=3,177,007 / U=9,991,436; replay
valid 2,331/2,354 = 91.2% of bytes, 2 parameter bytes each). **Scope — one live
value only (deflate v2 self-correction, `msg_7c6a2b66`; coordinator
`msg_6dba8980`):** untouchable = **212,047 B, covering sao + ooffice ONLY**
(160,422 + 51,625; the 234,047 B in deflate's first handoff is SUPERSEDED, and
234,414 B was the sao+ooffice+samba group — samba IS addressable, 411,393 B deflate
/ 193,463 B local ceiling); **addressable total = mozilla 1,459,509 + samba
193,463 = 1,652,972 B**. **EV rationale for Rank 5 (coordinator `msg_6dba8980`):**
P4.1's recovery was measured on the broken-decode binary `DA24665C` (encode-only;
PENDING-0 was in force then and forbids roundtrip claims for that binary), so ranks
1-4 are the actionable-now lanes; on the merits its measured
real-data payoff is the **largest on the board — 1.36 MB on a real corpus file
vs pnra's 12.9 KB on a {synthetic} cell — and it moves to the top of the
actionable list the moment a green binary lands** (coordinator to sequence).
Prior art (precomp/preflate/etc.):
no mechanism novelty claimed. Integration (arch/format/bench) only after the decode
fix, behind a flag with a byte-identity gate.

**Rank 6 — format registry integrity (format) — DONE; gate GREEN (this also
clears PENDING-0).** PASS seed=41246 roundtrip_variants=480 mutations=2880
deterministic_rev2=9 deterministic_bwt=24 golden_bwt=4 forced_postcoders=20
registry_block_modes=14 registry_transforms=5 suite_modes=[0,1,2,3,5,6,8]
suite_unforceable=[4,7] on `build-i9-format\anvil.exe` sha256 `8EAE1FB3...`
(src `62BC6631...`, HEAD fc23d9a), **zero skips**; registry coverage now in code
with emitted-wire-id assertions. Caveat for arch/decode-perf/bench: re-verify if
`src/anvil.cpp` changes before commit — the PASS is pinned to those shas. Hygiene
lane, not a frontier lane.

**Not rankable / should stop (unchanged from I8, reconfirmed):**
- any byte-axis attack on sqlite (−42.5%), log (−36.8%), repeat.jsonl (−81.4%):
  no current mechanism is within a factor of two and the decode axis is closed;
- any further work whose only payoff would be an encode-plane knee row (AUDIT-1b);
- chasing brotli-q1/q4 ratio beats (audit trap d);
- any decode-only pre-registration without a byte leg on the 12 closed files
  (DNB-M1); a decode-only proposal must first show a Route-B multiple below the
  attainable ceiling on its target file.

---

## 5. Mechanism retirement ledger (math vs implementation-era)

Retirement reason is one of the two classes, per the working agreements. Rows marked
`[NEW I9]` are added or corrected this iteration.

| mechanism | status | reason class | evidence |
|---|---|---|---|
| Post-parse rollback as EXP-X recovery | RETIRED | **math** | oracle F1: regression is trajectory-borne; every committed token individually sound |
| Stream-budget-only route to 2x decode | RETIRED | **implementation-era** `[CORRECTED I9]` | S6-1 verdict: formulation reproduces faithfully (J-faithful at lambda=0.01) but lambda is ~4.66 orders below the Linux willingness-to-pay; ZERO raw flips on 22 files. NOT a math failure |
| Implicit Delta=sigma*(d/4) as default ARI form | RETIRED | **math** | EXP-Z(b): +38.35% vs transmitted |
| RLZ/RePair book-stream as default | RETIRED | **implementation-era** | Exp-Y: decode −8.1..−15.9%, encode 48–64x. Ratio-only point |
| Word-aligned constant-Delta ARI on misaligned layouts | RETIRED | **math** | EXP-Z(a) timeseries +0.03%; columnar-align 0 tokens |
| Lane/field transposition applied globally before LZ | RETIRED | **math** | every tested ELF section grew |
| Transmitted-Delta TCOPY submode | RETIRED | **math** | made .eh_frame AND .rodata larger |
| Context clustering as novelty | RETIRED | **prior art** | RFC 7932 §7 context map |
| Encode-plane knee rows as an objective | RETIRED | **math** | AUDIT-1b: 5 exist; all single-plane, all decode-dominated, all front-knee artifacts |
| Decode-plane rows on ratio ~ 1.000 files | RETIRED | **math** | AUDIT-5 / gate R-2: DEGENERATE, store-vs-fast-store |
| **Decode-only Pareto crossings without a byte leg, on 12 of 13 files** | **RETIRED `[NEW I9]`** | **math** | DNB-M1 (gate R-3): required multipliers 4.6x–106.5x vs a 1.79x wire-invisible ceiling; synth-arith's 2.46x is DEGENERATE |
| **Zero-bit derived parameters on STATISTICAL transforms** | **RETIRED `[NEW I9]`** | **math** | gate G4 amendment: residual entropy grows ~log2(d) (0.65–0.69 bits/doubling over two decades); no cheap+effective distance band. Exact transforms only |
| **Record-period P as a novelty claim** | **RETIRED (infrastructure)** | **prior art** | gate G4b: xz `--delta` range 1..256 covers 14 and 23; auto-detection is infrastructure |
| **I8 §2.7c 28-byte record anatomy** | **RETRACTED `[NEW I9]`** | **measurement error** | C-3: aliasing at 2 x true period; corrected P=14/P=23 PENDING-LEDGER |
| **Materialization as a decode win** | **RETIRED `[NEW I9]`** | **implementation-era** | arch paired interleaved reps=7/1t/identical wire: generated.json +1.4%, synth-timeseries +27.6% (CV 9.5%); t3 mat shares are not removable liveness overhead (coordinator `msg_af51ded9`; gate PART XIV A9; **reopen only on a batched/vectorized design with a pre-registered paired win**) |

---

## 6. The Iteration-9 plan (ranked, pre-sized, sequenced)

Cells closest (living artifact; patched as peers land):

1. **synth-timeseries.bin** — grid-dependent (C-6): worktree −14.4% bytes / 1.78x
   decode (1.82x anti-tie); committed HEAD −15.7% bytes / 2.16x decode (sparse-rans;
   q9 bar). Closest on both axes; decode margin inside the noise band on the worktree
   grid, while at HEAD the decode route needs the materialization leg. Byte side now
   has a landed prototype target (89,877 B = 25.5% below the byte bar) awaiting native
   integration; decode side awaits PR-1 (state your grid).
2. **generated.json** — −11.6% bytes OR 4.50x decode (4.59x anti-tie; three-leg fix).
3. **synth-arith.bin** — byte route only; 106,368 B corridor above 187.5 MB/s;
   decode route DEGENERATE.
4. **src.cpp** — −15.7% bytes; decode closed (40.6x).
5. Everything else — ≥ −17.8% bytes, decode closed.

### Pre-registrations PR-1..PR-5 — status at write time

- **PR-1 — synth-timeseries + generated.json decode profiles.**
  **Status: DONE — BRANCH NO-GO / DECODE-SHORT (CRC route); the pre-commit fired.**
  Measured CRC shares 26.3/28.7/27.0% vs the 43.82% DECODE-SHORT line; I8's 44%
  (generated.log) did not transfer. generated.json is now the sole decode lead
  (mode-10 mdl: mat 51.4% + alloc 21.5%, need 2.35x; mat-only cap 2.06x; mat+alloc
  3.69x `[PROJECTION]`), with a canonical re-measure required (frozen CSV 112.8
  vs current 220.5 MB/s). Handoff to arch: materialization leg first. Tokens are
  decode-route arithmetic only; the frontier verdict vocabulary is never
  interchangeable.
- **PR-2 — degenerate-escape guard.**
  **Status: LANDED (ruled).** Gate ruling `docs/gate-ruling-i8-pareto-win.md` R-2
  adopts the row-ratio >= 0.95 DEGENERATE rule. Arithmetically live on synth-arith
  and random.bin; flagged so nobody builds toward it.
- **PR-3 — FRONT-GAP vs FRONT-CROSSING token.**
  **Status: LANDED (ruled).** R-2 mechanical bracket test adopted; this document
  applies it (5 GAPFILL, 0 CROSSING). The mandatory co-listed tuple is binding
  (gate I9-1 five-figure form: `5 | 5 | 0 | 0 | 411/416` at HEAD).
  **PR-3-Q1 — ANSWERED (research-gate, `docs/gate-ruling-i9-datastruct-ts.md` §6):**
  a landed row at ~89,877 B on synth-timeseries is **mechanically FRONT-CROSSING on
  the raw grid but is STRUCK by the dual-bar qualifier** — the transform-enabled
  references reach 89,564 B (xz -9e `--delta=14`) and 80,650 B (brotli-q11 on
  delta14), so it is **FRONT-GAP (dual-bar)**, never a crossing. "Mechanism probe,
  not generalization" changes the narrative/admissibility, not the token;
  projections never alter the tuple. Sequence confirmed: native integration ->
  byte-exact roundtrip -> bench arbiter twice -> PR-4 window -> gate classification
  -> dual bar reported last, not least.
- **PR-4 — measurement-window attestation.**
  **Status: LANDED (v1.1, binding; `tests/pr-4-measurement-window-protocol.md`,
  blackboard `deliverable/pr-4-window-protocol`).** Required fields include binary
  SHA-256, UTC window, raw reps + median, `cv_pct` vs `tests/noise-floor.csv`,
  host-load sampler, affinity, and **thread counts**; the THREAD RULE requires a
  same-core-count comparison or an explicit `THREAD-IMBALANCE: anvil <N>t vs ref 1t`
  disclosure — undisclosed rows are not arbiter-usable. Crossing claims additionally
  require two hash-identical arbiter runs + bench sign-off. Consequence: the
  ratio-first decode multiples remain class-only (no thread counts in those CSVs);
  byte-side work is unaffected (bytes are deterministic).
  **Status: RULED (I9-2, `docs/gate-ruling-i9-pr5-position-derived.md`; requester
  pnra `msg_3d53d393`).** Arm (B) is **not exact** (bounded {-1,0,1} residual) and
  is **not closed by DNB-M2** either: it is a **third category (bounded-residual
  predictive basis)**, **buildable as {engineering}/adopt/control with NO novelty
  claim** under conditions C1–C8. The P-3.4 **causality condition is binding** (`s`
  must be decoder-computable from already-reconstructed bytes at point of use; no
  look-ahead; "no number is admissible until this is stated"). C4's dual bar on
  this cell is **87,013 / 16,313 B**; C6 requires A vs B vs the parameter-free
  second-difference control. Surviving novelty in the family: exact invariant
  indexing, unification under one MDL parser, correction-topology coding — none of
  which is arm (B). research-gate issues verdicts; this lane does not.

### Sequencing (matches the I9 task seed)

```
P0 [bench]    arbiter refresh on CURRENT bytes — COMPLETE (worktree grid c70179ea);
              committed-grid refresh requested by coordinator; recon/re-verify held
              by arch's decode fix (AUDIT-11)
P1 [decode-perf] PR-1: synth-timeseries + generated.json + BWT decode profiles
P2 [arch]     decode leg 1: CRC-only fast path, byte-identical wire
P3 [arch]     legs 2-3: materialization, alloc (staged, byte-identity gates)
P4 [bench]    arbiter per landed leg (two hash-identical runs) + PR-4 window
              attestation + bench sign-off
P5 [bwtinv]   BWT inverse profile + prototype (13-16 -> 50+ MB/s target)
P6 [datastruct] synth-timeseries byte axis inside the reference (P=14)
P7 [pnra]     native delta/stride engineering form under the gate boundary
P8 [deflate]  P4.1 DEFLATE reconstruction feasibility
P9 [format]   registry coverage + FORMAT.md integrity (hygiene)
P10 [gate]    PR-5 ruling, claim verification, ledger PART XIV (record-period)
G  [strategy] patch this document after every landed measurement  ◄──────────────┘
```

Serial-lane note: arch owns `src/anvil.cpp` solely, so P2->P3 are serial. P5–P9 are
parallel (prototypes / other binaries / docs). Build contention: only arch and bench
use `./build`; my lane builds nothing and runs no arbiter.

---

## 7. Endgame accounting (rewritten at iteration end)

**Status at this patch: ITERATION IN PROGRESS — 0/10 tasks complete at write time;
no I9 measurement has landed.** Nothing below is final.

| quantity | value | source |
|---|---|---|
| Arbiter runs this iteration (I9) | **1 (P0, both grids)** | bench `deliverable/p0-arbiter` |
| PR-1 decode profile (I9) | **DONE — DECODE-SHORT (CRC 26.3-28.7% vs 43.82% line); generated.json now sole decode lead** | `deliverable/pr1-profile`; PR-4-unattested |
| Verdict rows produced (I9) | **P0 output** | `tests/pareto-recompute-i9.md` |
| EXTENDS_FRONT rows, all history | **5** | re-derived on both grids |
| EXTENDS_FRONT rows, I9 | **0** | P0 found no new row; worktree grid |
| Strong crossings (both planes) | **0** | §1.3, recomputed |
| Per-file anvil row-plane cells, committed HEAD | **416** | recomputed |
| Dominated (committed HEAD) | **411** | recomputed |
| AGGREGATE cells / dominated (committed HEAD) | **32 / 32** | recomputed |
| Combined dominated (committed HEAD) | **443/448 v1 (brotli+zstd) / 444/448 v2 (+xz-9e)** | recomputed; class label required |
| Per-file cells, uncommitted worktree `c70179ea` | **468** | recomputed; label required |
| Dominated (worktree `c70179ea`) | **463** | recomputed; label required |
| Combined dominated (worktree `c70179ea`) | **499/504 v1 / 503/504 v2** | recomputed; worktree label required |
| Of the 5 non-dominated: FRONT-GAP | **5** | bracket test, §1.1 |
| Of the 5 non-dominated: FRONT-CROSSING | **0** | bracket test, §1.1 |
| Program A: datastruct prototype wire on synth-timeseries (NOT an ANVIL row) | **89,877 B, ratio 0.3210 (25.5% below q6)** | wire size/SHA recomputed; roundtrip peer-asserted; **PR-3-Q1 ANSWERED: FRONT-GAP (dual-bar) 89,564 / 80,650, never a crossing**; does not alter the 0-CROSSING count |
| Program A: pnra-stride prototype wire on synth-arith (NOT an ANVIL row) | **12,936 B, ratio 0.0505 (15.5x inside the 201,088 B corridor)** | `results-i9-pnra.csv` + hashes verified; prototype wire, {synthetic}, PR-5 RULED {engineering} (no novelty); timer parallel (ranking-grade); no verdict |
| Program B: P4.1 DEFLATE reconstruction, mozilla (encode-only prototype) | **1,362,177 B recovered (13,806,173 -> 12,443,996); beats mozilla xz -9e by 932,252 B** | `[LANDED-PEER]` prototypes/i9-deflate/results/; encode-only (decode blocker); no roundtrip claim; **now decode-verified on canonical `E8AA2E48`; byte totals unchanged (encoder identity 494/494)** |
| Program B: Silesia auto bytes | **46,446,995 B** | as-recorded `de9b4caf`; **RE-VERIFIED on canonical `0D1E130B`** (13/13 roundtrip sha256 OK, `tests/ratio-reverify-i9.csv` sha `53880D50`); beats xz by 2,009,105 B |
| Program B: enwik8 auto bytes | **23,534,368 B** | as-recorded `de9b4caf`; **RE-VERIFIED on canonical `0D1E130B`** (13/13 roundtrip sha256 OK); beats xz by 1,297,288 B |
| Program B: decode, enwik8 | **portfolio row: auto 5.890 MB/s = 23.95x Brotli / 14.06x xz; variant: bwt-direct 6.730 = 20.96x / 12.31x** | gate `msg_0e3fdb5c`; PR-4-pending |
| Program B: decode, Silesia | **portfolio row: 12-file auto 9.435 MB/s = 14.60x Brotli / 7.89x xz; variant: 7-routed bwt-backend 13.015 = 11.24x / 6.05x** | gate adopted; PR-4-pending |
| Program B: peak memory | 10.3 GiB, Brotli-lgwin30-attributed | handoff §3; not recomputed here |
| Deviations recorded this pass | **C-1..C-6** | §0 |

**Mandatory co-listed tuple.** Primary (committed HEAD `fc23d9a`): v1 brotli+zstd
class (label required) `5 | 5 | 0 | 0 | 411/416` (443/448); **v2 +xz-9e
`4 | 4 | 0 | 0 | 412/416` (444/448)**. Uncommitted worktree `c70179ea`: v1
`5 | 5 | 0 | 0 | 463/468` (499/504); v2 (label required) `1 | 1 | 0 | 0 | 467/468`
(503/504). Any one figure quoted alone is misleading; the 468/463 pair requires the
worktree label. **The `0 FRONT-CROSSING` counts LANDED rows only; prototype/projected
rows (datastruct contingency) do not alter it.**

**Corrections accepted into this tally:** the coordinator's record-period erratum
(C-3, with the generator as validator) and **my lane's own C-1** (the I8 "erratum"
that reintroduced the convention error it corrected) and **C-2** (a flipped corridor
spot check and one front count). Two iterations, two tally/provenance failures in
this lane's own documents; the recompute rule binds me too.

### PENDING register (explicit)

0. **Build-integrity fix — CLEARED on `8EAE1FB3...`** (format gate GREEN, zero
   skips; src `62BC6631...`). PENDING only the canonical suite-binary sha + a
   re-verify if `src/anvil.cpp` changes before commit. The P0/decode legs are no
   longer blocked on this item; existing CSV figures were never affected.
1. **P0 bench arbiter refresh — COMPLETE** (fresh arbiter output on the uncommitted
   worktree grid `c70179ea`; tuple 5/5/0/463-of-468). Follow-up PENDING: the
   coordinator asked bench to restate suite provenance and refresh on the
   **committed** grid, which heads the citation rule.
2. **PR-1 — DONE** (decode-perf, `deliverable/pr1-profile`): CRC-only
   DECODE-SHORT; PENDING follow-through = arch's materialization leg + a canonical
   re-measure of the generated.json row (frozen CSV 112.8 MB/s vs current 220.5).
   **Bench re-measure DONE (ranking-grade): generated.json 189.146 MB/s ->
   DECODE-SHORT; synth-timeseries 177.891 MB/s -> DECODE-TIE (needs paired
   core-gated re-measure); decode-only route largely closed; BWT inverse+postcoder
   is the structural path.**
3. **PR-4** protocol landed (v1.1); PENDING is the first fully attested window for a
   citation-grade decode multiple (ratio-first rows lack thread counts -> class-only),
   plus the committed-grid suite refresh. AUDIT-9/C-4 resolves with that window.
   **Measurement contract v1.2 (bench ruling, coordinator `msg_eeb8079d`):**
   citation-grade = all fields + affinity + pinned-core mean <5% pre-window +
   same-window paired arms + reps>=7 + effect clears requirement by >= max(2%, CVs)
   + load <=5pp over ambient; **windows are EXCLUSIVE** — hold builds/gates/fuzz;
   bench schedules a consolidated citation-grade window once `src/anvil.cpp` is
   frozen.
4. **Ledger PART XIV** entry for corrected record periods P=14 / P=23 (research-gate).
5. **PR-5** ruling on the native delta/stride form (research-gate).
6. **I9 lanes with no landed measurement yet:** arch's materialization leg (the
   CRC leg is closed), bwtinv prototype. (datastruct — item 10; format harness —
   item 9; pnra — item 11; deflate — item 12.)
7. **The t3 share transfer assumption** to synth-timeseries (part of PR-1).
8. **Ratio-first artifacts untracked** in git (C-5 note): flagged to
   coordinator/bench for commit; hashes recorded in §8 so drift is detectable.
9. **Format harness change landed (tests/fuzz.py, format `msg_7637bdc8`; test-set
   change notice):** `registry_coverage_roundtrip` (forced block modes
   0-5,10-17 + transforms 0-3 + backends 1/2, asserting the emitted wire id),
   emitted-postcoder-id assertions, 2 new adversarial rev2 reject cases, and
   `stream_suite_coverage`; count delta `deterministic_rev2 7 -> 9`; CLI/exit-code
   contract unchanged. Exact new PASS line **PENDING** until the decode fix lands
   (AUDIT-11); no figure in this synthesis depends on the old PASS line.
10. **datastruct prototype -> native integration + arbitration (PR-3-Q1 pending):**
    the 89,877 B wire is not an ANVIL row. Guard at every mention: *prototype wire,
    no ANVIL row exists, {synthetic}, period + partition supplied, auto-discovery
    unsolved*. Only valid sequence: native integration -> byte-exact roundtrip ->
    bench arbiter twice -> PR-4 window -> research-gate ruling; the projected row
    does not alter the `0 FRONT-CROSSING` tuple count (landed rows only). A crossing
    on a supplied-structure synthetic cell is a mechanism probe, not evidence of
    generalization (audit-2026-09-07 README point 1). Auto-discovery landed
    (12,920 B = 0.9988x oracle); **bench co-run timing folded in (ranking-grade,
    `w-bench-pnra-autoseg-20260912T091754Z`, 1t core19 HIGH): synth-arith
    450.5 MB/s CV 11.05%, drift-stride 590.5, log 481.1; pre-window core util
    14.16% fails the <5% gate so absolutes are not citation-grade. Task CLOSED;
    only future work is arch integration (default-off flag + byte-identity gate)
    if prioritized.**
15. **Frozen-grid recompute + anomaly attribution (PENDING):** strategy has not yet
    independently recomputed `tests/benchmark-suite.frozen-bdc90474.csv`; bench's
    frozen tuple (`33|5|0|28|435/468`; 471/504) is cited as arbiter-owner issued.
    The ~5-6x store-path speed anomaly is unattributed (arch/gate).
11. **pnra-stride prototype -> integration + PR-5 + arbitration:** the 12,936 B wire
    at 462.3 MB/s is not an ANVIL row. Sequence: PR-5 ruling (research-gate), arch
    integration behind a default-off flag with a byte-identity gate, bench suite row
    + two hash-identical arbiter runs + quiet-window PR-4, then classification.
    {synthetic}, class-specific cell; no generalization claim.
12. **P4.1 integration (deflate -> arch/format/bench):** the mozilla recovery is
    encode-only on the broken-decode build; after the green binary: staged
    integration behind a flag with a byte-identity gate, roundtrip, then arbiter +
    PR-4. Adopt-class, no novelty; `{encode-only}` label until then.
13. **Recon candidate — RULED FRONT-GAP (dual-bar), not a crossing; tuple stays
    5|5|0|0 (synth-columnar-align.bin):** bench `w-bench-recon` (`msg_e1e17c66`;
    `tests/benchmark-recon-i9.md`, `deliverable/i9-recon`) flags
    anvil-hotop-rlzp-rans ratio **0.3680** vs brotli-q11 0.3840 / xz -9e 0.4028
    (**-4.17%**); official arbiter double-run sha `133690EB` = 3 EXTENDS_FRONT there;
    mechanical R-2 with xz -> hotop-rlzp encode+decode **FRONT-CROSSING CANDIDATES**
    (no bracket), hotop-budget decode = FRONT-GAP. **NOT declared** (coordinator
    `msg_e6adf31b`): synthetic probe cell, hotop-rlzp encode 0.048 MB/s
    (retired-as-default), and xz is NOT in the arbiter reference prefix set
    (`brotli-`/`zstd-` only). **RULED (I9-6 / PART XIV A13): NOT a crossing —
    FRONT-GAP (dual-bar);** the transform-enabled control xz -9e `--delta=dist=23` =
    21,340 B (0.0773) per the gate ruling / **21,332 B (0.07729) per bench's
    reproduction (XZ 5.6.4) — 8 B apart; bench owns the canonical value** — is
    **4.76x smaller** than the 0.368 row. Recon tally — binding form (gate A14, research-gate `msg_da9fdde0`):
    **3 | 3 | 0 | 0 (321/324)**, with the mechanical parenthetical attached: 2
    crossings dual-bar-rejected; committed 13-file tuple v1 unchanged, **v2 +xz-9e
    4 | 4 | 0 | 0 | 412/416 (444/448)**. xz -9e is adopted into the reference class (bench:
    `xz-` prefix, versioned; transform controls side-channel). Binding labels for
    future claims: **NON-DEFAULT/RESEARCH-CONFIG and GRID-THIN**; recon throughput
    ranking-only. Record-worthy (first ANVIL row below every raw reference ratio on a
    cell) but **not frontier-worthy; crossing language is prohibited for it** (the only
    permitted occurrence of that phrase is in ruling R-2.4). xz is best ref on 7/9 recon cells; gaps GREW vs the
    brotli/zstd-only view (drift-stride +7.3% -> +15.9%; counters.log -> +23.8%).
14. **DECODE AXIS CLOSED — endgame classification (coordinator `msg_7770d6e2`):**
    the wire-invisible decode route is FULLY EXHAUSTED — CRC dead; MAT
    falsified/retired (A9); ALLOC retained 1.352x/1.074x but short (A12); postcoder
    edges short (A8); leg-4 (buffered renorm + two-level cumulative tables in
    `bwt_arith_decode`) implemented and wire-identical, but the ~1.4x target is
    **FALSIFIED** as a route while the leg is **RETAINED as a zero-byte-cost decoder
    win** (gate PART XIV A15): dickens 1.215x, webster 1.169x, bytes-weighted
    aggregate **1.179x**
    (attested paired, reps=7, core18, 1t; gates 442/442 + fuzz PASS on leg-4 src
    `FE0CF4F1`). Only remaining route is a format-changing transmitted-static-table
    postcoder — **NOT AUTHORIZED; deferred** with the quantitative reason: ID3 raw is
    5-6.6x faster on the postcoder but +52-101% bytes, and even a perfect postcoder
    leaves libsais_unbwt at 63-69% of decode, so the 11-24x real-corpus decode gap
    is not closable this way. If the fallback is ever revisited: **no novelty**
    (transmitted static tables are prior art), charged rate budget, format
    registration, PR-4 window, dual bar. **Classification: the decode axis is a STRUCTURAL/MATH
    gap on the real corpora, not an unexplored implementation. The Silesia/enwik8
    byte win stands as FRONT-GAP (cost).** Ranked lanes updated: leg 4 FALSIFIED; no
    live wire-invisible decode program remains.
    **A16/A17 (gate `msg_eb6ea423`):** stage-correct k_p verified
    6.05x/3.86x/6.60x; end-to-end aux+leg4 = **2.134x/2.379x** (BWT-only 23.80/29.65
    MB/s, portfolio ~25.7 vs the 40 target; conservative whole-decode read
    1.77–2.09x) — **the 40 MB/s bar is unreachable by any wire-invisible route**;
    the static-table postcoder needs FORMAT.md registration + a direct forced test
    if ever opened. **Format gate PASS on canonical `72D65150` / src `BDC90474`**
    (zero skips; registry identical to both prior freezes -> ALLOC + leg-4
    independently confirmed WIRE-UNTOUCHED; FORMAT.md re-pinned; standing re-verify
    on the next src move).

### Pre-committed endgame rule (unchanged in form, corrected in wording)

If I9 ends with no new EXTENDS_FRONT row, the failure must be classified as one of:

- **math-failure** — the reachable decode band is provably below the required lift
  on every live cell (evidence: PR-1 profile + §2.2 table), or the required byte
  reduction exceeds what the surviving mechanisms can deliver; or
- **implementation-era-failure** — the band is reachable in the profile's arithmetic
  but the legs did not land this iteration (evidence: which leg, which commit, what
  it measured).

I will not characterize a no-new-row iteration as a zero result. The five rows
exist. If I9 adds none, the honest
sentence is: "I9 produced no new EXTENDS_FRONT row; the five that exist remain
single-plane, encode-only, decode-dominated front-knee artifacts; 0 strong crossings
in project history; Program B's bytes win stands and remains FRONT-GAP on decode."

---

## 8. Sources grounded (artifact + state + SHA-256 prefix + command)

Recomputation commands (run from repo root, pwsh; outputs quoted in §0–§2). The
recurring helper is the re-implemented arbiter predicate; the tool itself was NOT
run.

- `tests/benchmark-suite.csv` — worktree, modified (SHA-256 prefix
  `C70179EA20B63BF5`), 13 files x 27 codecs, 351 rows; `git show
  HEAD:tests/benchmark-suite.csv` is the committed 25-codec / 325-row grid.
  Inventory + tuple (both grids):

```powershell
python - <<'PY'
import csv
from collections import defaultdict
rows = list(csv.DictReader(open('tests/benchmark-suite.csv', newline='')))
files = sorted({r['file'] for r in rows})
anvil = [r for r in rows if r['codec'].startswith('anvil-')]
print('files', len(files), 'anvil_codecs', len({r['codec'] for r in anvil}),
      'ref_codecs', len({r['codec'] for r in rows if r['codec'].startswith(('brotli-','zstd-'))}),
      'rows', len(rows))
def dominated(p, refs, key):
    pr, pm = float(p['ratio']), float(p[key])
    for q in refs:
        qr, qm = float(q['ratio']), float(q[key])
        if qr <= pr and qm >= pm and (qr < pr or qm > pm):
            return q
    return None
byf = defaultdict(list)
for r in rows: byf[r['file']].append(r)
nd = []; dom = 0
for f in files:
    A = [r for r in byf[f] if r['codec'].startswith('anvil-')]
    R = [r for r in byf[f] if r['codec'].startswith(('brotli-','zstd-'))]
    for key in ('encode_MBps','decode_MBps'):
        for a in A:
            d = dominated(a, R, key)
            if d is None: nd.append((f, a['codec'], key, a['ratio'], a[key]))
            else: dom += 1
print('per-file cells', 13*18*2, 'dominated', dom, 'non-dominated', len(nd))
for x in nd: print(' ', x)
PY
# -> files 13 anvil_codecs 18 ref_codecs 9 rows 351
# -> per-file cells 468 dominated 463 non-dominated 5
# -> all 5: tests\corpus\generated.json | encode_MBps | ratio 0.108/0.108/0.108/0.112/0.112
```

- Tuple join check vs bench's P0 arbiter outputs: worktree
  `tests/pareto-baseline.csv` (prefix `F432C34445D7DE4A`) matches my re-derivation on
  **468/468** anvil cells; committed grid `tests/pareto-baseline.head-fc23d9a.csv`
  (sha `66f5e5cd87e9fd85...`) matches on **448/448** anvil rows (416 per-file + 32
  AGGREGATE); 0 mismatches on both (same predicate; join on label/plane/codec). If
  a later refresh disagrees, the coordinator's rule applies: stop and surface, do not
  publish a second tuple.
- Per-file gap + decode bars: recomputed with the same predicate by selecting the
  best-ANVIL row per file (min ratio, tie min bytes) and the max-decode reference
  under the row's ratio (§1.2, §2.2 tables). `random.bin`/`synth-arith.bin` bars use
  the row named in the table; the row-dependence is stated there.
- Route-B multiplier table (§2.3): re-implementation of
  `prototypes/i8-theory/iso_crossing.py` (2% threshold slack), NOT run.
- Corridor scan (§2.4): re-implementation of
  `prototypes/i8-arch/crossing_check.py` convention (3-decimal quantised ratio),
  NOT run. Spot checks as quoted.
- `tests/auto-routing.csv` (untracked, prefix `F27C291FDC78B5C2`) +
  `tests/ratio-first-standard.csv` (untracked, prefix `010191A4EC1D046C`) +
  `tests/bwt-backend-standard.csv` (untracked, prefix `462BB4AB04586438`) +
  `tests/enwik8-bwt.csv` (untracked, prefix `3F369C8128462679`). Program B sums:

```powershell
python - <<'PY'
import csv
rs = list(csv.DictReader(open('tests/ratio-first-standard.csv', newline='')))
ar = list(csv.DictReader(open('tests/auto-routing.csv', newline='')))
ew = list(csv.DictReader(open('tests/enwik8-bwt.csv', newline='')))
def s(rows, c, corp=None):
    return [r for r in rows if r['codec'] == c and (corp is None or r['corpus'] == corp)]
auto = s(ar, 'anvil-auto-direct')
print('Silesia auto  =', sum(int(r['compressed_bytes']) for r in auto))
print('Silesia xz    =', sum(int(r['compressed_bytes']) for r in s(rs, 'xz-9e', 'silesia')))
print('Silesia brotli=', sum(int(r['compressed_bytes']) for r in s(rs, 'brotli-q11-lw30', 'silesia')))
brot = {r['file']: int(r['compressed_bytes']) for r in s(rs, 'brotli-q11-lw30', 'silesia')}
bwtd = {r['file']: int(r['compressed_bytes']) for r in s(ar, 'anvil-bwt-direct')}
print('oracle min(Brotli,BWT) =', sum(min(brot[f], bwtd[f]) for f in brot))
print('enwik8 auto   =', s(ew, 'anvil-auto-direct')[0]['compressed_bytes'])
print('enwik8 xz     =', s(rs, 'xz-9e', 'enwik8')[0]['compressed_bytes'])
print('enwik8 brotli =', s(rs, 'brotli-q11-lw30', 'enwik8')[0]['compressed_bytes'])
# 7 BWT-routed Silesia files
bwtr = [f for f in brot if bwtd[f] == {r['file']: int(r['compressed_bytes']) for r in auto}[f]]
inp = sum(int(r['input_bytes']) for r in auto if r['file'] in bwtr)
tb = sum(float(r['decompress_s']) for r in ar if r['codec']=='anvil-bwt-direct' and r['file'] in bwtr)
tbr = sum(float(r['decompress_s']) for r in rs if r['codec']=='brotli-q11-lw30' and r['file'] in bwtr)
tx = sum(float(r['decompress_s']) for r in rs if r['codec']=='xz-9e' and r['file'] in bwtr)
print('7 BWT-routed: n=%d bwt %.3f s (%.2f MB/s) brotli %.3f s (%.2f) xz %.3f s (%.2f)' %
      (len(bwtr), tb, inp/1e6/tb, tbr, inp/1e6/tbr, tx, inp/1e6/tx))
print('  brotli/bwt = %.2fx   xz/bwt = %.2fx' % (tb/tbr, tb/tx))
PY
# -> Silesia auto = 46446995 ; xz = 48456100 ; brotli = 49383136 ; oracle = 46446836
# -> enwik8 auto = 23534368 ; xz = 24831656 ; brotli = 24810180
# -> 7 BWT-routed: n=120362284 bwt 16.725 s (7.20 MB/s) brotli 0.823 s (146.33) xz 1.529 s (78.70)
#    brotli/bwt = 20.33x   xz/bwt = 10.94x
```

- enwik8 decode (single file, `tests/enwik8-bwt.csv` + `tests/ratio-first-standard.csv`):
  bwt-direct 14.859763 s -> 6.73 MB/s; brotli-q11-lw30 0.708968 s -> 141.05 MB/s;
  xz-9e 1.207280 s -> 82.83 MB/s; 20.96x / 12.31x.
- Record periods (C-3): `tests/make_synth_corpus.py:44-60` (`'<QfH'`, 20,000 x 14 B)
  and `:112-147` (23-byte rows, 12,000). `python -c "import struct;
  print(struct.calcsize('<QfH'))"` -> `14`.
- `tests/noise-floor.csv` (worktree, modified) — CV recompute quoted in AUDIT-6.
- Rulings and pre-registrations: `docs/gate-ruling-i8-pareto-win.md` (R-1..R-7),
  `docs/gate-verdict-i8-ari-stride.md` (G4 amended, A1–A4 retired),
  `docs/gate-priorart-audit-i8.md`, `docs/pre-registrations/i8-ari-ref.md`,
  `docs/pre-registrations/s6-1-verdict-skeleton.md` (S6-1 verdict:
  implementation-era, not math), `RESEARCH_LEDGER.md` PART XIII.
- PR-4 + P0 artifacts (bench, all untracked): `tests/pr-4-measurement-window-protocol.md`
  (v1.1, binding), `tests/pareto-recompute-i9.md`,
  `tests/pareto-baseline.head-fc23d9a.csv` (sha `66f5e5cd87e9fd85...`),
  `tests/pareto-verdict.head-fc23d9a.csv` (sha `b9680ac93fe5fb15...`),
  `tests/thread-attestation.csv` when it lands.
- Peer deliverables carried by reference (labeled `[LANDED-PEER]`):
  `deliverable/t3-decode-floor-profile` (decode-perf), pnra's transform ladder and
  estimator experiment (reproduced by I8; values as quoted), theory's iso-crossing
  (`prototypes/i8-theory/iso_crossing.py`), arch's corridor script
  (`prototypes/i8-arch/crossing_check.py`), datastruct's prototype
  (`prototypes/i9-datastruct/RESULTS.md`; wire sizes and SHA-256 prefixes recomputed
  read-only by this lane from `prototypes/i9-datastruct/out/`), and pnra's stride
  prototype (`prototypes/i9-pnra/README.md` + `results-i9-pnra.csv`; `stride_ref.exe`
  sha256 and the synth-arith input hash verified read-only by this lane).
- Gate rulings I9 (research-gate, gate-i9 COMPLETE; `deliverable/gate-i9` v2):
  `docs/gate-ruling-i9-frontier-tokens.md` (I9-1: PR-2/PR-3, row-level DEGENERATE,
  five-figure tuple), `docs/gate-ruling-i9-pr5-position-derived.md` (I9-2: arm (B)
  buildable {engineering}, no novelty; causality + C1–C8),
  `docs/gate-ruling-i9-datastruct-ts.md` (I9-3: PR-3-Q1 answers, dual bar),
  `docs/gate-ruling-i9-decode-multiples.md` (portfolio vs variant decode rows),
  `docs/gate-ruling-i9-p41-deflate.md` (P4.1 GO DIRECTION only, roundtrip
  PENDING), `docs/gate-verify-regime-i9.md`; PART XIV addendum A1–A3.
- deflate artifacts (untracked, `[LANDED-PEER]`, encode-only):
  `prototypes/i9-deflate/{RESULTS.md,results/}` (mozilla/samba census + replay).
- Coordinator directives: `msg_ca2f7e90` (tuple source), `msg_6054a94f`
  (record-period erratum), `msg_30351ec7` (grid-provenance citation rule); bench
  `msg_bb2fda46` (canonical decode multiple, P0 complete), `msg_de30ce7f` (P0
  handoff: head-grid CSVs, PR-4 v1.1, thread rule), `msg_484ead41` (anti-tie
  convention v2: DECODE-GO/DECODE-TIE/DECODE-SHORT, binding), `msg_3a74db41`
  (contingency framing guard for the datastruct coordinate test), `msg_70cba686`
  (best-ANVIL grid provenance: 140,898 is worktree-only); pnra `msg_048a6526`
  (stride prototype landing) and `msg_23c80183` (dual-bar CLEAR, {engineering});
  decode-perf `msg_e56eb71d` (PR-1 DONE, DECODE-SHORT); coordinator `msg_6dba8980`
  (gap reconciliation + P4.1 EV rationale), `msg_d6ba74f9` (PR-1 re-rank); format
  `msg_d82b6d39` (gate GREEN, PENDING-0 cleared on `8EAE1FB3...`); bench/coordinator
  `msg_eeb8079d` (measurement contract v1.2: exclusive windows, ranking vs citation
  ladder, bwtinv direction ranking-grade only, aux wire charge co-listed);
  coordinator `msg_77517809` (canonical binary `0D1E130B...`); pnra `msg_e0169153`
  (auto-discovery landed: 12,920 B = 0.9988x oracle, boundaries exact).

## 7-FINAL. Endgame accounting — FROZEN at iteration end

**Status: FINAL. All 15 tasks complete; all measurement windows closed; no commits by this lane.**

### 7F.1 Frontier tuples (co-listed; GRID-THIN always; every citation names grid + reference class)

| grid / class | non-dom | FRONT-GAP | FRONT-CROSSING | DEGEN | dominated |
|---|---|---|---|---|---|
| FROZEN canonical `BDC90474` (build 72D65150), v1 = v2 | 33 | 5 | 0 | 28 | 435/468 (471/504) |
| worktree `C70179EA` v1 | 5 | 5 | 0 | 0 | 463/468 (499/504) |
| worktree `C70179EA` v2 +xz-9e | 1 | 1 | 0 | 0 | 467/468 (503/504) |
| committed HEAD `fc23d9a` v1 | 5 | 5 | 0 | 0 | 411/416 (443/448) |
| committed HEAD `fc23d9a` v2 +xz-9e | 4 | 4 | 0 | 0 | 412/416 (444/448) |

**ZERO FRONT-CROSSING on every grid and reference class.** The frozen grid's 28 DEGENERATE are the store-path ratio=1.000 cells (random.bin + synth-arith.bin; real store speed, DEGENERATE-class, no frontier claim); the 5 FRONT-GAP are the familiar generated.json encode-plane rows.

### 7F.2 Decode-axis closure (authorized wire-invisible routes exhausted)

| leg | outcome | evidence |
|---|---|---|
| PR-1 CRC | DECODE-SHORT | CRC share 26.3/28.7/27.0% vs 43.82% line (A3) |
| MAT | FALSIFIED / RETIRED (implementation-era) | +1.4% / +27.6% slower (A9) |
| ALLOC | retained, insufficient | 1.352x / 1.074x (A12) |
| postcoder edges | DECODE-SHORT | buffered renorm C=1.021/1.014/1.013 (A8) |
| leg-4 (2-level tables + renorm) | retained, ~1.4x target FALSIFIED | 1.215x / 1.169x, 1.179x aggregate (A15) |
| postcoder requirement, stage-correct | 6.05x / 3.86x / 6.60x; end-to-end 2.134x / 2.379x | A16/A17 |
| frozen decode calls | TIE VOID -> DECODE-SHORT by measurement | A19 |

Static-table postcoder (format change): **NOT AUTHORIZED**; if ever reopened -> FORMAT.md registration + direct forced test + charged rate budget + PR-4 window + dual bar + no novelty. **Store-path side win (kept, wire-invisible):** CRC fast path (S6-1b leg 1 + PCLMUL) = 4.5x enc / 3.4x dec, bytes bit-exact (351/351 + 494/494); citable with window `w-arch-storepath-20260912T1300Z` + arm shas (A21, bench-signed).

### 7F.3 Classification

- **Decode axis: STRUCTURAL/MATH gap on the real corpora** — the 40 MB/s bar is unreachable by any wire-invisible route; this is not an unexplored implementation.
- **Program A (13-file arbiter grid): no crossing; 0 FRONT-CROSSING across all grids/classes.** The familiar five remain single-plane encode FRONT-GAP (brotli+zstd reference class grid v1 only).
- **Program B ratio-first bytes: WIN, reproduced on the fixed binary** — Silesia 46,446,995 B / enwik8 23,534,368 B (as-recorded `de9b4caf`, re-verified on `0D1E130B`); **FRONT-GAP on decode** (Silesia 12-file auto 14.60x / 7.89x; enwik8 auto 23.95x / 14.06x; PR-4-pending).

### 7F.4 Corrections accepted (public)

C-1 needed-% convention (I8 erratum corrected); C-2 corridor spot-check flip; C-3 record periods P=14/P=23; C-4 decode-multiple resolution (my "11.2x not recomputable" withdrawn; 20.33x rejected; enwik8 correction verified); C-5 grid provenance; C-6 best-ANVIL grid provenance (140,898 is worktree-only).

### 7F.5 PENDING list (explicit)

1. Strategy's independent recompute of `tests/benchmark-suite.frozen-bdc90474.csv` (bench's frozen tuple cited as arbiter-owner issued).
2. Bench's corrected decode artifact (post-A9 caps; gate already ruled DECODE-SHORT, A19).
3. Ledger PART XIV formal C-3 entry (record periods; A5/A6/A13 already verified).
4. Ratio-first artifacts untracked in git (commit decision: coordinator).
5. PR-4 citation-grade windows for decode multiples (all class-only/labelled).
6. Committed-grid refresh on the frozen build if the coordinator requires one.

### 7F.6 Next-iteration candidates (from landed facts only)

1. **P4.1 DEFLATE integration** (mozilla-first / samba-second): measured 1,362,177 B recovery (encode-only on `DA24665C`), addressable 1,652,972 B, untouchable 212,047 B; needs green-binary roundtrip + arbiter + PR-4; adopt-class, no novelty.
2. **aux-unbwt integration** (`libsais_unbwt_aux` + I-array): 3.40-3.55x same-core-count ranking-grade; quiet-window rerun + arch wiring behind a new wire id; aux charge 19,328 B Silesia / 3,052 B enwik8 co-listed.
3. **pnra self-discovering step-region integration**: 12,920 B = 0.9988x oracle; adopt-class {engineering}; default-off flag + byte-identity gate.
4. **datastruct columnar-context capability integration**: auto-discovery landed (+39/+31 B); target = transform-enabled bar; adopt-class; dual-bar reporting.

**Mandatory co-listed tuple (FROZEN):** `33 | 5 | 0 | 28 | 435/468` (471/504), with the worktree and HEAD grids co-listed; **ZERO FRONT-CROSSING everywhere; GRID-THIN always.**

---

**Standing disclaimer.** None of this is a claim. Every ranked item is a
pre-registrable, falsifiable test with a named arbiter. Every multi-leg decode band
is `[PROJECTION]` arithmetic on decode-perf's landed t3 shares, not a measurement.
The five non-dominated rows are FRONT-GAP, not crossings. Program B's byte wins are
real and measured; they are FRONT-GAP on decode until the BWT inverse multiple is
settled by PR-4 and moved by bwtinv.
