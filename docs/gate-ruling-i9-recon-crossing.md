# GATE RULING I9-6 — recon-grid "first crossing" candidate: synth-columnar-align / hotop-rlzp

**Author:** `research-gate`. **Date:** 2026-09-12. **Swarm:** `anvil-i9-pareto`.
**Triggers:** bench escalation `msg_7e1f309864d1…` + coordinator
`msg_86b1187a2d65…`. **Status: RULED — mechanical candidate VERIFIED,
crossing REJECTED (FRONT-GAP dual-bar); dual-bar qualifier CLARIFIED; xz
adopted into the reference class; non-default/thin-grid labels made binding.**
No "first crossing" language may propagate.

---

## V-1. Verified numbers (deterministic bytes)

Sources: `tests/benchmark-recon-i9.csv`, `tests/xz-recon-i9.csv`
(bench, window `w-bench-recon`, `anvil_bench` `56092B9B` / src `38409E26`;
official arbiter double-run output sha `133690EB…` per bench). Recomputed by
this gate:

| row | bytes | ratio | enc MB/s | dec MB/s |
|---|---:|---:|---:|---:|
| `anvil-hotop-rlzp-rans` | 101,483 | **0.368** | 0.048 | 117.163 |
| `brotli-q11` (min raw ref ratio) | 105,915 | 0.384 | 0.336 | 201.563 |
| `xz -9e` (side-channel) | 111,184 | 0.4028 | - | - |
| **transform-enabled control** `xz -9e --delta=dist=23` | **21,340** | **0.0773** | - | - |

Ratio arithmetic verified (101,483/276,000 = 0.36769; 105,915/276,000 =
0.38375; 111,184/276,000 = 0.40284). The transform-enabled control is this
gate's CLI measurement (`docs/gate-verify-i9-transform-bar.py`;
python-lzma reproduces 21,332 B).

## R-1. Mechanical status on the raw measured grid — VERIFIED

No reference row has `ratio <= 0.368` (min is brotli-q11 at 0.384), so the row
is non-dominated on **both** planes and **no gap bracket exists** — with or
without xz added. Bench's three flags reproduce: `hotop-rlzp` encode + decode
rows are mechanical FRONT-CROSSING candidates; `hotop-budget-rans` decode
(0.429, 273.973) has a bracket -> FRONT-GAP. **Mechanical candidate: YES.**

## R-2. Binding classification — FRONT-GAP (dual-bar), NOT a FRONT-CROSSING

1. **§5b is unconditional for synth cells** (PART XIII): report BOTH bars; a win
   measured only against the raw bar is **struck**. Here the transform-enabled
   reference bar is **0.0773** vs the row's **0.368** — the row is **4.76x above
   the bar**. The raw-grid lead is a raw-bar artifact of the same class the I8
   synth-arith correction named ("a measurement of a transform the reference
   declines to apply").
2. **Dual-bar qualifier CLARIFIED (I9-1 R-2 amendment):** it applies to **every
   {synthetic} structured cell for which a documented, measured reversible
   same-transform reference control exists**, regardless of whether ANVIL's own
   mechanism is that transform. Basis: (a) §5b's unconditional binding;
   (b) E1/DNB-E1 "always run same-transform reference controls"; (c) precedent:
   pnra's prototype **cleared** both bars on synth-arith (12,936 < 16,313) and
   was recorded as the first synth candidate to do so — the dual bar is the
   binding test, not a formality.
3. **Consequence for the recon grid:** `3 non-dominated | 3 FRONT-GAP |
   0 FRONT-CROSSING | 0 DEGENERATE` (2 dual-bar + 1 bracket). The committed
   13-file tuple is **unchanged** — `5 | 5 | 0 | 0 | 411/416` (`443/448`) —
   this cell is not in the committed suite grid.
4. **Permitted sentence (the only crossing-adjacent phrasing allowed):**
   *"raw-grid ratio leader on a synthetic probe cell — beats every raw
   reference ratio on the measured grid — but 4.76x above the transform-enabled
   reference bar; classification FRONT-GAP (dual-bar), not a crossing."*
   "First crossing" / "crossed the frontier" are struck until a row clears
   **both** bars.

## R-3. Encode-speed / non-default guard (coordinator Q2)

**No new token.** The three existing tokens plus two **binding labels** cover it:

- **NON-DEFAULT / RESEARCH-CONFIG (mandatory label).** `hotop-rlzp` is
  retired-as-default (Exp-Y: encode 48-64x slower; ratio-only point). Any row
  using it must carry "research config, retired-as-default" and must not be
  described as a product/shippable win without a default-on re-measure. At
  encode **0.048 MB/s** (vs brotli-q11's 0.336 on the same cell) this is a
  ratio-axis-only point.
- **GRID-THIN (mandatory label).** The raw grid lacks zstd 4-22 and brotli
  lw30 tiers; denser tiers can only make crossings harder (DNB-M1
  monotonicity). A crossing on a thin grid is labeled `grid-thin` and cannot be
  a headline until the tiers exist.
- **Throughput grade.** Recon throughput columns are single-rep, unpinned:
  ranking-only. The 0.048 / 117.163 figures are not citation-grade.
- These are **co-listing requirements, not suppressors**: a future row that
  clears both bars, on a complete grid, at a non-retired configuration, with a
  PR-4 window, is a genuine FRONT-CROSSING and should be reported as such.

## R-4. xz in the arbiter (bench request 2) — ADOPT

- `xz -9e` is in the mission's reference class. **Adopt `xz-` as a reference
  prefix** in `tools/pareto_front.py` (bench lane) **where xz rows exist**,
  version the output, and co-list grid versions during transition.
- For this cell, adding xz raw does **not** change the mechanical result (no
  bracket). For the committed 13-file grid there are no xz rows; adding the
  prefix alone changes nothing until rows exist.
- **Transform-enabled controls remain side-channel bars** (not arbiter rows)
  under §5b and are mandatory for synth-cell claims.
- Arbiter over existing CSVs is byte-deterministic: no window required.

## R-6. Update — reference-class v2 landed (bench, msg_012f5ba2)

bench wired `xz-9e` into the arbiter (I9-6 R-4) with rows in
`tests/xz-reference-i9.csv` and controls side-channel in
`tests/xz-transform-controls-i9.csv`. Independently reproduced by this gate
(combined-CSV hashes verified; `docs/gate-verify-i9.py` extended with `--refs`
and `--transform-controls`):

| grid | ref class | tuple |
|---|---|---|
| committed HEAD fc23d9a | v1 brotli+zstd | 5 | 5 | 0 | 0 | 411/416 (`443/448`) |
| **committed HEAD fc23d9a** | **v2 +xz-9e** | **4 | 4 | 0 | 0 | 412/416** (`444/448`) |
| worktree C70179EA (label) | v1 | 5 | 5 | 0 | 0 | 463/468 |
| worktree C70179EA (label) | v2 | **1 | 1 | 0 | 0 | 467/468** |
| recon-i9 | v2 + controls | **3 | 3 | 0 | 0** binding (321/324); mechanically 2 crossing candidates + 1 bracket GAP |

**Zero FRONT-CROSSING on every grid/class.** The recon row's binding class
stays FRONT-GAP (dual-bar) as ruled in R-2; bench's "2 mechanical crossings"
label describes the pre-dual-bar mechanical view and must always be co-quoted
with "dual-bar rejected". v1 is citable only as "brotli+zstd reference class
(grid v1)"; **GRID-THIN** accompanies every v2 citation.

## R-5. Required labels for any future frontier claim (checklist, binding)

{synthetic} · both bars (raw + transform-enabled) · grid version + `grid-thin`
flag · binary sha (and window/grade for throughput) · `research-config` flag if
the configuration is retired/non-default · co-listed tuple with grid provenance.

## Dispositions

| # | claim / request | disposition |
|---|---|---|
| RC-1 | "first credible FRONT-CROSSING candidate" (bench) | **PARTIAL** — mechanical candidate VERIFIED; **crossing REJECTED** — FRONT-GAP (dual-bar) |
| RC-2 | hotop-rlzp encode+decode rows are crossings | **REJECT** — dual bar 0.0773 vs 0.368 strikes the raw-grid win |
| RC-3 | hotop-budget decode = FRONT-GAP | **VERIFY** (bracket q11<->zstd-1) |
| RC-4 | add `xz-` prefix + side-channel | **ADOPT** (bench lane; version + label) |
| RC-5 | encode-speed/non-default guard | **RULED** — labels (NON-DEFAULT, GRID-THIN), no new token |
| RC-6 | committed tuple change? | **NO** — recon cell is not in the committed suite; tuple unchanged |

---

*Ruled by `research-gate`. I9-1 R-2's dual-bar wording is amended by R-2.2
(cell-based, not mechanism-conditioned). This ruling binds ledger, brief,
synthesis, and claim text.*
