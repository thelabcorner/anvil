# GATE RULING I9-1 — PR-2 (DEGENERATE guard) and PR-3 (FRONT-GAP / FRONT-CROSSING tokens)

**Author:** `research-gate` (novelty gate arbiter / ledger historian).
**Date:** 2026-09-12. **Swarm:** `anvil-i9-pareto`. **Task:** `gate-i9`.
**Status: RULED — binding on ledger, brief, synthesis, and every claim.**
**Supersedes:** nothing semantic. The I8 ruling
(`docs/gate-ruling-i8-pareto-win.md` R-2) is adopted and completed: the v4
row-level amendment to PR-2 is now the formal text, the token definitions are
restated for I9, the decode-route vocabulary collision is closed, and the
tuple's provenance is corrected to a committed artifact.

**Triggers:** strategy PR-2/PR-3 (I8 §6), arch's §2.7a correction (row- vs
file-level degeneracy), coordinator directives `msg_0e4a75b76a4443939cfd3a1ccd7eb7c1`
(vocabulary), `msg_d30840a8b2c9484ca5e7440bf06333af` (tuple provenance),
bench `decisions/anti-tie-convention-v1` v2 (decode-route tokens).

---

## R-1. PR-2 — the DEGENERATE guard (row-level)

> **RULING.** A **non-dominated** row whose **OWN ratio >= 0.95** is
> **DEGENERATE**. It is reported as DEGENERATE, **never as a crossing, never as
> a compression result**. The test uses the row's own `ratio` field for its
> `(file, codec)` — not the file's current best row, not another row on the
> same file, not the aggregate, not a projection.

**Why (math-class retirement, unchanged from I8):** at ratio ≈ 1.0 the
comparison is *store vs fast-store*, not compression. The predicate is satisfied
by a non-compression property, so **no decoder work and no implementation
improvement converts a DEGENERATE row into a result**. DNB-M3 stands.

**The v4 amendment (arch's §2.7a correction, now binding).** The original PR-2
draft tested the **file**: "synth-arith is at ratio 1.000, therefore degenerate."
That would have vetoed a legitimate compressed row on the same file. Degeneracy
is a property of the **row**. Recorded lineage: strategy §2.7a Correction 2;
adopted by this gate in PART XIII §2 (5th bullet) and formalised here.

**Boundary cases (all binding):**

1. **Row-level only.** A file whose current best row is at ratio >= 0.95 does
   NOT veto a different row on the same file that actually compressed. The
   mode-16 example: a row at 89,363 B / ratio 0.349 on synth-arith (file best
   1.000) is judged on its own ratio — 0.349 < 0.95, not DEGENERATE.
2. **No reverse rescue.** A legitimate compressed row on the same file does not
   legitimise a degenerate row; each row is classified alone.
3. **Dominated rows are DOMINATED, not DEGENERATE.** The token applies only to
   non-dominated rows. A row at ratio >= 0.95 that is dominated is a normal
   dominated cell; it is still counted in the dominated fraction.
4. **No escape arguments.** "But decode is fast", "but it is the only row on
   the plane", "but the references are near-store too" do not reclassify.
   PART XIII §3's closure of the synth-arith decode route at ratio 1.000 rests
   on this rule.
5. **Inclusive threshold.** `ratio >= 0.95`, evaluated on the artifact's own
   numeric field, no rounding.

**Live cells (DNB-M3, restated at committed HEAD):** `synth-arith.bin` and
`random.bin` carry ANVIL rows at ratio 1.000 (store-class). No non-dominated row
is DEGENERATE as of this writing; the count is 0 (see R-4). The rule is armed,
not triggered.

**What DEGENERATE does NOT mean:** not "worthless". It is a measurement of
store-vs-fast-store behaviour and may be reported *with the label*, exactly as
the 5 FRONT-GAP rows may be reported as fills.

---

## R-2. PR-3 — FRONT-GAP vs FRONT-CROSSING (one token each, mechanical tests)

The ambiguity that let five encode-knee rows be misread is resolved by two
mutually exclusive tokens, both defined by a test a machine can run on the
suite CSV. Same-file, same-plane scoping is mandatory. `p` is always an ANVIL
row; `q` is always a reference row (`brotli-*` / `zstd-*`, the same class
`tools/pareto_front.py` uses).

### Non-dominance (necessary condition, bench's arbiter)
`p` is non-dominated iff no reference `q` satisfies
`q.ratio <= p.ratio AND q.mbps >= p.mbps AND (q.ratio < p.ratio OR q.mbps > p.mbps)`.
This is `EXTENDS_FRONT`, and it remains **necessary but not sufficient**
(DNB-M2). `tools/pareto_front.py` is the sole issuer; this gate is the sole
classifier.

### FRONT-GAP (a FILL, never an advance)
`p` is **non-dominated** AND a **gap bracket exists**: two reference rows
`q_lo`, `q_hi` on the same file and plane with all of:
- `q_lo.ratio < q_hi.ratio` and `q_lo.mbps < q_hi.mbps` (they do not dominate
  each other), and
- `q_lo.ratio <= p.ratio <= q_hi.ratio` and `q_lo.mbps <= p.mbps <= q_hi.mbps`.

Every point inside that open rectangle is non-dominated **by construction**; a
deliberately bad codec landing there scores identically. FRONT-GAP means
"the reference set does not cover this rectangle", never "we advanced".

### FRONT-CROSSING
`p` is **non-dominated** AND **not FRONT-GAP** AND **not DEGENERATE**. Only
this token may be written as a frontier result. A row coordinate-identical to a
reference row on the plane (both axes) is not a crossing — it advances nothing;
report it as a tie with the reference, count it under non-dominated, and do not
call it a crossing. (Count as of this writing: 0.)

### Classification order (binding)
1. DOMINATED — if the arbiter finds a dominator. Stop.
2. DEGENERATE — if the row's own ratio >= 0.95 (R-1). Stop.
3. FRONT-GAP — if a bracket exists. Stop.
4. FRONT-CROSSING — otherwise.

### The five existing rows (committed HEAD fc23d9a, machine-verified R-4)
All five are on `tests\corpus\generated.json`, **encode plane only**; on the
decode plane every one is dominated by `brotli-q11` (651.755 MB/s at HEAD).
Canonical bracket (HEAD values): `brotli-q11` (ratio 0.096, 0.714 MB/s) <->
`zstd-19` (ratio 0.113, 2.511 MB/s).

| codec | bytes | ratio | enc MB/s | dec MB/s | encode plane | decode plane |
|---|---:|---:|---:|---:|---|---|
| anvil-mdl-rans | 89,589 | 0.108 | 1.499 | 150.138 | **FRONT-GAP** | DOMINATED (q11) |
| anvil-mdl-rans-l0 | 89,589 | 0.108 | 1.491 | 157.383 | **FRONT-GAP** | DOMINATED (q11) |
| anvil-mdl-rans-l001 | 89,589 | 0.108 | 1.507 | 145.823 | **FRONT-GAP** | DOMINATED (q11) |
| anvil-shape-rans | 92,300 | 0.112 | 1.326 | 172.678 | **FRONT-GAP** | DOMINATED (q11) |
| anvil-shape-rans-l0 | 92,300 | 0.112 | 1.198 | 171.030 | **FRONT-GAP** | DOMINATED (q11) |

**5 FRONT-GAP. 0 FRONT-CROSSING. 0 DEGENERATE.** The classification is
invariant across the committed and worktree grids (R-4).

### The I9 co-listing tuple (binding)
*(non-dominated | FRONT-GAP | FRONT-CROSSING | DEGENERATE | dominated fraction)*
**with the grid provenance attached**. Anything quoted alone will be struck.

- **Committed HEAD fc23d9a:** `5 | 5 | 0 | 0 | 411/416` — `443/448` including
  AGGREGATE (AGGREGATE alone: `0 | 0 | 0 | 0 | 32/32`).
- **Uncommitted worktree grid `C70179EA…` (label it if cited):**
  `5 | 5 | 0 | 0 | 463/468` — `499/504` including AGGREGATE.
- Both grids carry **the same 5 rows** with the same classification.

### REFERENCE-CLASS v2 UPDATE (I9-6/A13) — xz -9e adopted (binding)
The arbiter is now xz-aware (`tools/pareto_front.py` REF_PREFIXES + `xz-`);
xz rows in `tests/xz-reference-i9.csv`, same-transform controls side-channel in
`tests/xz-transform-controls-i9.csv`. Independently reproduced by this gate
(`python docs\gate-verify-i9.py --suite <combined-CSV>`; `--refs brotli-,zstd-`
reproduces v1):

| grid | ref class | tuple | combined |
|---|---|---|---|
| committed HEAD fc23d9a | v1 brotli+zstd | 5 | 5 | 0 | 0 | 411/416 | 443/448 |
| **committed HEAD fc23d9a** | **v2 +xz-9e** | **4 | 4 | 0 | 0 | 412/416** | 444/448 |
| worktree C70179EA (label) | v1 | 5 | 5 | 0 | 0 | 463/468 | 499/504 |
| worktree C70179EA (label) | **v2 +xz-9e** | **1 | 1 | 0 | 0 | 467/468** | 503/504 |

**ZERO FRONT-CROSSING on every grid and reference class.** v1 may be cited ONLY
as "brotli+zstd reference class (grid v1)"; the v2 tuples are the canonical
current citation. **GRID-THIN** must accompany every v2 citation (no zstd 4-22 /
brotli lw30 tiers). xz throughput is ranking-grade; bytes are deterministic.

### Dual-bar qualifier for {synthetic} transform cells (new, binding here)
Any {synthetic} structured cell for which a **documented, measured reversible
same-transform reference control exists** must have its candidate rows clear
the **transform-enabled reference rows** before any crossing token is issued:
a row that is non-dominated on the raw reference grid but is beaten by the raw
reference codec given the same transform opportunity is a **FRONT-GAP
(dual-bar)**, not a crossing. This generalises PART XIII §5b from synth-arith to
every synth cell, operationalises DNB/E1 ("always run same-transform reference
controls"), and is **cell-based, not conditioned on ANVIL's own mechanism**
(clarified by I9-6 R-2.2). Worked applications: `datastruct` ref_field 89,877
loses to `xz -9e --delta=dist=14` (89,564) and `brotli-q11+delta14` (80,650)
-> FRONT-GAP (dual-bar); `pnra` 12,936 clears both bars -> the first synth
candidate to do so; recon `hotop-rlzp` 0.368 is 4.76x above the transform bar
0.0773 -> FRONT-GAP (dual-bar), crossing rejected (`docs/gate-ruling-i9-recon-crossing.md`).

---

## R-3. Vocabulary separation, and the I9 decode standard (coordinator request)

**RULING: two vocabularies exist and are never interchangeable.**

| vocabulary | tokens | meaning | authority |
|---|---|---|---|
| **frontier-row verdict** | FRONT-GAP / FRONT-CROSSING / DEGENERATE / DOMINATED | what a *landed row* means against the reference front | this gate (R-1/R-2) |
| **decode-route arithmetic** | DECODE-GO / DECODE-TIE / DECODE-SHORT | go/no-go on a *proposed* decode leg (not yet a row) | bench, `decisions/anti-tie-convention-v1` v2 |

> A **DECODE-GO is an arithmetic go-ahead, never a frontier crossing.** A
> decode-route CROSSING (any old use of the word) is retired. A frontier claim
> still requires: a measured end-to-end artifact + PR-4 attested window + two
> hash-identical arbiter runs + bench sign-off + this gate's classification.

**Recorded as the I9 decode standard** (`decisions/anti-tie-convention-v1` v2,
bench, coordinator-ratified): margin `R' = 1.02 x R` computed from exact
reference + target rows (no hardcoded constants, no rounding down); cap model
`C = 1/(1-s)`, single byte-identical component only, **no compounding legs**;
wall-clock override: a claim must clear by `>= max(2%, CV of the target metric
in that window)` else DECODE-TIE; `generated.json` 5.33x stays labelled
PROJECTION at every citation. The gate records this rule; it does not own it.
(For completeness: `tests/pr-4-measurement-window-protocol.md` v1.1 adds the
required window fields and the thread-count disclosure rule.)

The same non-interchange discipline applies to the ratio-first lane's
vocabulary: its `FRONT-GAP (cost)` (decode/peak-mem margin sense,
`docs/audit-2026-09-07/11-front-crossing-criteria.md`) is **not** this lane's
rectangle-gap FRONT-GAP. Cross-lane citations must carry the qualifier.

---

## R-4. Verification — every figure recomputed from the COMMITTED artifact

Committed blobs (git object ids):
`fc23d9a:tests/benchmark-suite.csv` = `4c986eb61c953a24dad298132b5f5f03da6fb410`;
`fc23d9a:tests/pareto-baseline.csv` = `0cbda4f1b73ffcf8a0c1f48c0e472ab545efaad3`.
Worktree artifacts (NOT committed): suite sha256
`C70179EA20B63BF582DF186B399C05926259E658824CA59B238871D476661896`; baseline
sha256 `F432C34445D7DE4A70D1EB509E168330FD7C60AAB0AE356FF7F4A9770E9A2F7A`.

```powershell
# exact recompute of R-1/R-2/R-4 at committed HEAD
git show fc23d9a:tests/benchmark-suite.csv  > $env:TEMP\suite-fc23d9a.csv
git show fc23d9a:tests/pareto-baseline.csv  > $env:TEMP\baseline-fc23d9a.csv
python docs\gate-verify-i9.py --suite $env:TEMP\suite-fc23d9a.csv `
       --baseline $env:TEMP\baseline-fc23d9a.csv
```

Output (transcribed): per-file `5 non-dominated | 5 FRONT-GAP | 0 FRONT-CROSSING
| 0 DEGENERATE | 411/416 dominated`; AGGREGATE `0 | 0 | 0 | 0 | 32/32`;
combined `443/448`; baseline cross-check `448 anvil cells, 0 status
mismatches`. `docs/gate-verify-i9.py` reimplements the `tools/pareto_front.py`
predicate and adds the R-1/R-2 classification; it does not replace the tool.

**Provenance correction of the I8 tally (claim-verification finding).** PART XIII
§2's tuple `463/468` (and its `499/504` combined form) is **not recomputable
from any committed artifact**. No commit ever contained a 234-anvil-pair suite:
the sequence of committed suite blobs is 32/48/96/140/150/208/208 anvil pairs
(HEAD), and the 234-pair (18-codec) grid exists **only** in the uncommitted
worktree CSVs — the same files the I8 verification read at write time. Under
the committed-artifact rule the citation of record is the HEAD tuple
`5 | 5 | 0 | 0 | 411/416` (`443/448`), until bench's refresh is committed.
The five rows and their FRONT-GAP classification are identical in both grids;
only 52 extra dominated cells (two codecs added after the last suite commit)
differ. This is a provenance correction, not a classification change. bench's
`p0-arbiter` landed fresh artifacts
(`tests/pareto-baseline.head-fc23d9a.csv`, hash `66f5e5cd…`) that independently
confirm the HEAD tuple; this gate's harness confirms 0 status mismatches
against the committed baseline.

---

## R-5. What this does NOT say

- It does not say the 5 rows are worthless. They are real fills of a rectangle
  the shipped reference set leaves open. That is worth knowing; it is not a win.
- It does not retire decode work. Decode legs are governed by R-3's arithmetic
  vocabulary; they lift every row on the decode plane. They cannot alone make a
  crossing on the cells DNB-M1 closed.
- It does not pre-judge the datastruct / pnra mechanisms; those are separate
  rulings (`docs/gate-ruling-i9-datastruct-ts.md`,
  `docs/gate-ruling-i9-pr5-position-derived.md`).

---

*Ruled by `research-gate`, novelty gate arbiter. `tools/pareto_front.py` is
unchanged and remains the sole issuer of `EXTENDS_FRONT`; the reading is this
lane's. Complements: `docs/gate-verify-regime-i9.md` (claim-verification
regime), `docs/gate-verify-i9.py` (harness).*
