# Reference-class v2 (xz -9e) — arbiter re-run on both grids + recon (bench, I9-6)

Owner: `bench`. Authority: research-gate ruling A13 (xz- prefix + side-channel
transform controls adopted) + coordinator I9-6. Date: 2026-09-12.

## What changed

`tools/pareto_front.py` reference class v2 adds `xz-` to REF_PREFIXES. `xz-9e` rows
for the 13 suite files + 9 recon cells are in `tests/xz-reference-i9.csv`
(XZ Utils 5.6.4 `-9e -T1`; encode = process-level single run; decode = concatenated
stream, one process; **ranking-grade**, label them). Same-transform controls
(`xz --delta=dist=P`, `xz --x86`) are SIDE-CHANNEL in
`tests/xz-transform-controls-i9.csv` — the arbiter does not consume them; the gate
applies them as the dual-bar qualifier.

Combined grids (suite + xz rows), all double-run hash-identical:

| combined CSV | sha256 |
|---|---|
| `tests/benchmark-suite.head-fc23d9a.plus-xz.csv` | `82ec45c87f1a25402d9993a1704c4b7f6227ebf1af8a82575233064df07a8042` |
| `tests/benchmark-suite.c70179ea.plus-xz.csv` | `f54b8fce567eb74331724c6bfa7d50f06cb60ba23b86d5dd85df27a4f44515f4` |
| `tests/benchmark-recon-i9.plus-xz.csv` | `850af4d6d781c344c812a389e5c6bede87c29866929c748522959014071f02fb` |

## Co-listed results (cite with the grid label)

| grid | ref class | per-file cells | dominated | non-dom | FRONT-GAP | FRONT-CROSSING | DEGEN | combined (incl AGG) |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| committed HEAD fc23d9a | v1 brotli/zstd | 416 | 411 | 5 | 5 | 0 | 0 | 443/448 |
| committed HEAD fc23d9a | **v2 +xz-9e** | 416 | **412** | **4** | **4** | **0** | **0** | **444/448** |
| worktree C70179EA | v1 brotli/zstd | 468 | 463 | 5 | 5 | 0 | 0 | 499/504 |
| worktree C70179EA | **v2 +xz-9e** | 468 | **467** | **1** | **1** | **0** | **0** | **503/504** |
| recon-i9 | v1 | 324 | 321 | 3 | 1 mechanical | 2 mechanical (dual-bar rejected) | 0 | 357/360 |
| recon-i9 | v2 +xz-9e | 324 | 321 | 3 | 1 mechanical | 2 mechanical (dual-bar rejected) | 0 | 357/360 |

Binding dual-bar recon tuple (gate A14): **3 non-dominated | 3 FRONT-GAP | 0 FRONT-CROSSING | 321/324** — the dual-bar pass marks ALL THREE non-dominated rows FRONT-GAP (all ratios are above the xz --delta control 0.0773). Keep the mechanical parenthetical adjacent to the binding tuple whenever the recon grid is cited.

**Zero FRONT-CROSSING on every grid and ref class (mechanical or binding).**

## Which rows changed, and why

The four/five formerly non-dominated rows are all `generated.json` **encode plane**.
`xz-9e` on that file is ratio 0.092975 at **encode 1.32 MB/s** (all xz throughput
figures are ranking-grade: process-level encode, concatenated-stream decode; the
ratio/bytes are citable) — better ratio AND
faster encode than most of those ANVIL rows, so it now dominates them:

- HEAD grid v2: `anvil-shape-rans-l0` (enc 1.198) → DOMINATED by `xz-9e`; survivors
  `anvil-mdl-rans` (1.499), `-l0` (1.491), `-l001` (1.507), `anvil-shape-rans` (1.326).
- worktree grid v2: `anvil-mdl-rans` (1.060), `-l0` (1.300), `anvil-shape-rans` (1.004),
  `anvil-shape-rans-l0` (0.979) → all DOMINATED by `xz-9e`; the **only survivor is
  `anvil-mdl-rans-l001` (enc 1.329 MB/s, just over xz's 1.32)**.

So the familiar "5 non-dominated rows" is **a property of the brotli/zstd-only
reference class** (grid v1). Under the mission's binding bar (xz -9e included) it is
4 (HEAD) / 1 (worktree). The decode plane was already dominated by brotli-q11 and did
not change.

## Side-channel transform controls (dual-bar basis)

Best same-transform control per cell (`tests/xz-transform-controls-i9.csv`):
- `synth-columnar-align.bin`: `xz -9e --delta=dist=23` = 21,332 B (0.07729) vs plain
  xz 111,184 (0.4028) — the control that killed the recon crossing candidate (A13).
- PE cells: `xz -9e --x86` beats plain xz on 5/6 (pe-git 1,637,188 vs 1,778,240;
  pe-where 18,660 vs 19,696; pe-winver 5,000 vs 5,008; pe-ninja 230,696 vs 247,312;
  pe-notepad 181,724 vs 183,756; pe-python unchanged).
- `synth-counters.log` / `synth-drift-stride.bin`: no delta control beats plain xz
  (delta is worse) — plain xz-9e is already the binding control there.

## Citation rule for the transition

- v1 tuple `5 | 5 | 0 | 411/416` (443/448) / worktree `463/468` may be cited ONLY as
  `brotli+zstd reference class (grid v1)`.
- xz-inclusive citation is `4 | 4 | 0 | 412/416` (HEAD, 444/448) or
  `1 | 1 | 0 | 467/468` (worktree C70179EA, labelled, 503/504).
- GRID-THIN on every v2 citation: zstd 4-22 and brotli lw30 tiers are not measured.
- No crossing claim may rest on the arbiter alone: same-transform control + gate ruling
  are required (A13).
