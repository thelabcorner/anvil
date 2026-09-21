# P0 arbiter recompute — Iteration 9 (`anvil-i9-pareto`)

Task: `p0-arbiter`. Owner: `bench`. Date: 2026-09-07.
Status: **COMPLETE** (CSV-only; no benchmark run, no codec binary invoked).

## Provenance — two grids exist, both carry 5 non-dominated rows

| grid | source | suite sha256 | per-file cells | dominated | agg cells | combined |
|---|---|---|---|---|---|---|
| **committed HEAD `fc23d9a`** | `git cat-file blob HEAD:tests/benchmark-suite.csv` (git blob `4c986eb6`) | `7e7f530efea54a7694ba267d5356904e6efb37060c34f65edf12a5f0d64bce4c` | **416** | **411** | 32/32 | **443/448** |
| **uncommitted worktree** | `tests\benchmark-suite.csv` (sha256 C70179EA) | `c70179ea20b63bf582df186b399c05926259e658824ca59b238871d476661896` | **468** | **463** | 36/36 | **499/504** |

The difference is codec set only: HEAD has 25 codecs/file (16 ANVIL + 5 brotli
+ 4 zstd); the worktree adds `anvil-hotop-budget-rans` + `anvil-hotop-rlzp-rans`
(still 13 files) → 16→18 ANVIL × 13 × 2 = 416→468 cells. Neither adds a new
non-dominated row.

**Citation rule (coordinator ruling, research-gate verified; ledger PART XIV):**
cite the committed-HEAD tuple `5 | 5 | 0 | 411/416` (443/448 incl AGGREGATE).
If the worktree fraction is cited it MUST be labelled
`"uncommitted worktree grid C70179EA"`. The tuple is always quoted whole.

## Exact recompute commands (`pwsh`, repo root)

Committed HEAD grid:

```powershell
git cat-file blob HEAD:tests/benchmark-suite.csv > $env:TEMP\benchmark-suite.head-fc23d9a.csv
python tools\pareto_front.py $env:TEMP\benchmark-suite.head-fc23d9a.csv --out tests\pareto-baseline.head-fc23d9a.csv
python tools\beats_brotli.py  $env:TEMP\benchmark-suite.head-fc23d9a.csv --out tests\pareto-verdict.head-fc23d9a.csv
```

Worktree grid (current bytes):

```powershell
python tools\pareto_front.py tests\benchmark-suite.csv --out tests\pareto-baseline.csv
python tools\beats_brotli.py  tests\benchmark-suite.csv --out tests\pareto-verdict.csv
```

Fresh artifacts in tree:

| file | sha256 |
|---|---|
| `tests/pareto-baseline.head-fc23d9a.csv` | `66f5e5cd87e9fd85e4bbcb761019f5b6e1ab3ccc24d57dcadfade790412dd027` |
| `tests/pareto-verdict.head-fc23d9a.csv` | `b9680ac93fe5fb158a728abf0cccfd7aa50ba7ad0512fa168a7285ea592465a7` |
| `tests/pareto-baseline.csv` (worktree grid) | `f432c34445d7de4a70d1eb509e168330fd7c60aab0ae356ff7f4a9770e9a2f7a` |
| `tests/pareto-verdict.csv` (worktree grid) | `382b262c81c71e803eb6a3778ea01d49aac6ab977f1efba8e58dfeb221974949` |

Double-run check (protocol §4): `pareto_front.py` re-run in a second process on
each suite produced byte-identical outputs (worktree `f432c344...`,
HEAD `66f5e5cd...`) — the recompute is deterministic.

## Result — 5 non-dominated rows (CONFIRMED on both grids)

All 5 are `tests\corpus\generated.json`, **encode plane only**; ratios identical
in both grids (0.108/0.112); speeds differ because the suite runs differ:

| codec | ratio | worktree enc MB/s | HEAD enc MB/s | dec MB/s (worktree) | gate class |
|---|---|---|---|---|---|
| anvil-mdl-rans | 0.108 | 1.060 | 1.499 | 112.815 | FRONT-GAP |
| anvil-mdl-rans-l0 | 0.108 | 1.300 | 1.491 | 154.874 | FRONT-GAP |
| anvil-mdl-rans-l001 | 0.108 | 1.329 | 1.507 | 155.235 | FRONT-GAP |
| anvil-shape-rans | 0.112 | 1.004 | 1.326 | 156.166 | FRONT-GAP |
| anvil-shape-rans-l0 | 0.112 | 0.979 | 1.198 | 180.362 | FRONT-GAP |

Bracket test (gate R-2): `brotli-q11 (r=0.096, 0.674 MB/s)` ↔
`zstd-19 (r=0.113, 1.963 MB/s)`; all 5 lie inside the rectangle → FRONT-GAP.
Decode plane: all 5 dominated by brotli-q11. Ratios < 0.95 → 0 DEGENERATE.

## Mandated co-listed tuple — CONFIRMED, both grids

```
committed HEAD fc23d9a:   5 non-dominated | 5 FRONT-GAP | 0 FRONT-CROSSING | 411/416 dominated  (443/448 incl AGGREGATE)
uncommitted worktree C70179EA: 5 | 5 | 0 | 463/468  (499/504)  [must be labelled]
```

Independent checks: distinct `(file, codec, plane)` anvil sets = 416 and 468
respectively (zero missing); AGGREGATE all dominated in both; bracket test
5 GAPFILL / 0 CROSSING / 0 DEGENERATE on both.
`beats_brotli.py` tally (HEAD): RATIO-BEATS-SOME 161 | NO-BEAT 42 |
EXTENDS-ONE-PLANE 5 = 208 anvil rows.

## Status update (2026-09-12): PENDING-0 CLEARED

The decode fix landed and was independently verified (format gate + coordinator:
`build-i9-format\anvil.exe` 8EAE1FB3, `src/anvil.cpp` 62BC6631, HEAD fc23d9a;
canonical shared `build\anvil.exe` published as 0D1E130B). Byte/roundtrip work is
unblocked; measurements use the canonical shared build or same-source bench exe
(D6A70ACD, /O2) per the coordinator's same-source ruling.

1. Canonical re-verification Silesia `46,446,995 B` / enwik8 `23,534,368 B` —
   **IN PROGRESS** on `build\anvil.exe` 0D1E130B (pinned core 17, BelowNormal,
   byte-only); results land in `tests/ratio-reverify-i9.csv`. Historical backing
   rows remain `tests/auto-routing.csv` / `tests/enwik8-bwt.csv`; canonical
   decode multiples live in `decisions/bwt-decode-multiple-canonical` v2 and
   `docs/gate-ruling-i9-decode-multiples.md` (Silesia auto portfolio 14.597x /
   7.885x; enwik8 auto 23.946x / 14.062x; 11.243x only for the forced-BWT
   7-file subset).
2. Fresh reference-row reconnaissance on the 9 newer corpus cells — **DONE**
   (2026-09-12, window `w-bench-recon-20260912T103740Z`, build `56092B9B`/src
   `38409E26`, `--reps 1`, ranking-grade). xz -9e added by hand (xz 5.6.4) and is
   the best reference on 7/9 cells. One result is a **FRONT-CROSSING candidate**
   (`synth-columnar-align.bin`, anvil-hotop-rlzp-rans ratio 0.3680 vs brotli-q11
   0.3840 / xz 0.4028), escalated to research-gate; not declared. See
   `tests/benchmark-recon-i9.md` + blackboard `deliverable/i9-recon`.
3. Decode re-measure (coordinator request) — **DONE**:
   `tests/pareto-i9-decode-remeasure.md` (generated.json mode-10 mdl 189.146 MB/s
   -> R'=2.779 vs mat cap 2.058 = DECODE-SHORT; synth-timeseries hotop-rlzp ->
   DECODE-TIE across windows).
4. Any future `EXTENDS_FRONT` claim: two separate arbiter process invocations
   hash-identical before citation (protocol §4), plus `bench` sign-off.

## CORRECTION 2026-09-12 (I9-6, reference-class v2) — tuple superseded as default

The tuple above is the **brotli+zstd reference-class (grid v1)** result and must be
labelled as such. After adding `xz-9e` to the arbiter reference class (coordinator
I9-6 / research-gate A13), the double-run tuples are:

```
committed HEAD fc23d9a:         4 non-dominated | 4 FRONT-GAP | 0 FRONT-CROSSING | 412/416 (444/448 incl AGGREGATE)
uncommitted worktree C70179EA:  1 | 1 | 0 | 467/468 (503/504)   [must be labelled]
```

Cause: `xz-9e` on generated.json is ratio 0.092975 at encode 1.32 MB/s and dominates
every encode-plane row below that encode speed (HEAD loses shape-rans-l0; worktree
loses 4 of 5 — only mdl-rans-l001 at enc 1.329 survives). Zero FRONT-CROSSING either
way. Full detail: `tests/pareto-reference-class-i9.md`; blackboard
`deliverable/reference-class-v2`.
