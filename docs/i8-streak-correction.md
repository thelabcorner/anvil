# ANVIL — Iteration 8: streak-tally correction (coordinator error, on the record)

Author: coordinator. Date: 2026-09-04. Status: verified, closed.

## What happened

The Iteration-8 shared brief (`docs/swarm-i8-brief.md`) opened with:

> "**Mission: produce the first `EXTENDS_FRONT` verdict row in project history**,
> and then make it systematic. Seven iterations and ~396+ verdict rows have
> produced ZERO."

**That is false.** There are **5 EXTENDS_FRONT rows committed at HEAD.**

The error was found by the `strategy` lane (deliverable `strategy-i8-audit`,
findings AUDIT-1/1b) within the first hour of the iteration, and independently
re-verified by the coordinator before acceptance.

## The facts

Verified with:

```powershell
git show HEAD:tests/pareto-baseline.csv | Select-String -Pattern "EXTENDS_FRONT"
```

Five rows, all `tests\corpus\generated.json`, all **encode plane**:

| codec | ratio | encode MB/s | decode MB/s |
|---|---|---|---|
| anvil-mdl-rans | 0.108 | 1.060 | 112.815 |
| anvil-mdl-rans-l0 | 0.108 | 1.300 | 154.874 |
| anvil-mdl-rans-l001 | 0.108 | 1.329 | 155.235 |
| anvil-shape-rans | 0.112 | 1.004 | 156.166 |
| anvil-shape-rans-l0 | 0.112 | 0.979 | 180.362 |

Committed, not worktree drift (`git show HEAD:...` reads the committed object).

### Provenance — when the count changed

```
21669ad -> 0      (I2)
3bf4c32 -> 0      (I2)
d506953 -> 0      (I3)
33499cc -> 0      (t4-srr)
7b999c6 -> 5      (Experiment X)  <-- FIRST APPEARANCE
fc23d9a -> 5      (HEAD at I8 start)
```

Both `docs/swarm-i7-strategy.md` §1 and the I8 brief were written **after**
7b999c6 and still asserted zero.

## Why the five rows are a front-knee artifact, not a crossing

The reference encode front on generated.json is:

```
brotli-q11   r=0.096   0.674 MB/s
zstd-19      r=0.113   1.963 MB/s
brotli-q9    r=0.137  17.170 MB/s
```

q11 is smaller but slower than zstd-19; zstd-19 is faster but larger. Neither
dominates the other. Therefore **any** point with ratio in (0.096, 0.113) **and**
encode speed in (0.674, 1.963) MB/s is non-dominated automatically — it occupies
a rectangle that no shipped reference configuration covers. All five ANVIL rows
sit inside that rectangle. The result is arithmetic, not interpretation.

The decode front on the same file is only two points (q11 0.096/507.4, zstd-19
0.113/1506.5), and all five ANVIL rows are **dominated** there: 112.8–180.4 vs
507.4 MB/s.

**Honest state:** 5 EXTENDS_FRONT rows exist and are real per our sole arbiter.
Zero of them is a decode-plane or both-plane crossing. 463 of 468 anvil
row-plane cells remain dominated (499 of 504 including the AGGREGATE rows).

## The real failure mode

This is not a story about a missing win. It is a story about a number that was
copied. The coordinator wrote the brief by propagating "0 EXTENDS_FRONT" out of
a prior synthesis without recomputing it from `tests/pareto-baseline.csv`. Two
syntheses and a mission brief then carried the same stale number forward.

### Erratum 1 — the dominated-cell count (caught by research-gate, conceded by strategy)

The first version of this document stated "460 of 465 anvil row-plane cells
remain dominated." That figure was supplied by the `strategy` lane and is
**wrong**; `research-gate` caught it. Recomputed from
`tests/benchmark-suite.csv` by the coordinator:

```
PER-FILE cells:  468  non-dominated: 5  dominated: 463
AGGREGATE cells:  36  non-dominated: 0  dominated:  36
TOTAL:           504  non-dominated: 5  dominated: 499
```

Corrected in this document and in `docs/swarm-i8-brief.md`. Recorded as an
erratum rather than silently edited away.

Note the recursion, which is the actual lesson: the coordinator propagated an
unrecomputed tally (0 EXTENDS_FRONT), and then — while fixing that error —
propagated a *second* unrecomputed tally (460/465) from the lane that had just
corrected the first one. The recompute rule binds the coordinator at least as
hard as it binds anyone else, and "my source is the lane that caught the last
error" is not a recomputation.

## Binding rule (applies to the coordinator too)

> Any cumulative, tally, or streak number entering a brief, a synthesis, a
> ledger entry, or a claim **must be recomputed from the source artifact at the
> moment of writing, with the verifying command shown in the text.** A number in
> a document is a claim, not a fact.

## Narrative rule

**Record, do not celebrate.** The ledger may state:

> 5 EXTENDS_FRONT rows exist per `tools/pareto_front.py` on generated.json,
> encode plane, since 7b999c6.

and must state in the same breath:

1. all five are single-plane,
2. encode-plane only,
3. all five are dominated on the decode plane by brotli-q11,
4. they occupy a ratio×speed interval the shipped reference front does not
   cover — a front-knee artifact.

A bare "0 → 5" or a bare "we crossed the frontier" both fail claim hygiene.

## Actions taken

- `docs/swarm-i8-brief.md` — mission statement corrected in place.
- Blackboard `context/i8-streak-correction` — correction available to every lane.
- `bench` — directed to re-run `tools/pareto_front.py` on current bytes and
  confirm or reject the re-derivation (confirmation of a committed artifact, not
  a new verdict). Must happen early; everything downstream keys off the count.
- `research-gate` — directed to rule on whether a single-plane encode knee
  against a two-point front may be called a "Pareto win", and to consider a
  distinct token (e.g. `FRONT-GAP` vs `FRONT-CROSSING`) so this ambiguity cannot
  recur.
- `strategy` — endgame accounting updated; "still zero" is no longer writable.

## Mission: unchanged

Produce a **decode-plane or both-plane** Pareto crossing at competitive ratio.
The five existing rows do not meet that bar, and nothing about the corrected
tally changes the ranked target cells.
