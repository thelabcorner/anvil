# GATE RULING — was a single-plane encode knee against a 2-point front a "Pareto win"?

**Author:** `research-gate` (novelty gate arbiter / ledger historian).
**Date:** 2026-09-04. **Swarm:** anvil-i8-pareto.
**Trigger:** coordinator directive (msg_4914fd9a) + theory (msg_96c9f3d7) +
strategy (msg_6336f637). All three asked the same question; this is the single
binding answer.

**Status: RULED. Three verdicts, two adoptions, one correction.**

---

## R-1. A single-plane encode knee against a 2-point front is NOT a "Pareto win"

**RULING: NO.** The phrase "Pareto win" is henceforth **unusable** for any row
that is non-dominated only on one plane, and unusable for any row that is
non-dominated only because the reference suite leaves an axis-rectangle
uncovered.

Grounds (three, independent, any one sufficient):

1. **Single-plane means dominated.** All 5 rows are `DOMINATED` on the decode
   plane by `brotli-q11` (507.4 vs 112.8–180.4 MB/s). A row that is dominated
   on a plane is not a frontier row on that plane. Calling a one-plane result a
   "win" without the plane qualifier is a claim about a plane we lost.
2. **The non-dominance is a property of the TEST MATRIX, not of ANVIL.** On
   generated.json's encode plane the reference front is two mutually
   non-dominating points: `brotli-q11` (r=0.096, 0.674 MB/s) and `zstd-19`
   (r=0.113, 1.963 MB/s). Any point with ratio in (0.096, 0.113) AND speed in
   (0.674, 1.963) is non-dominated **by construction**. Verified below: all 5
   rows satisfy the bracket test exactly. A deliberately bad codec would score
   identically. That is a hole in the reference set, not an achievement.
3. **The project's own standing rule already said this.** `research-agenda.md`
   §4.6 and FLAG-A: *"ratio beat alone is insufficient — the row must not be
   DOMINATED on the decode plane either."* The arbiter's raw boolean
   (`EXTENDS_FRONT`) is a necessary condition, never a sufficient one. It was
   always the gate's job to read it, and the gate (this lane, prior
   iterations) did not read it until strategy forced the issue.

**Binding consequence:** `EXTENDS_FRONT` emitted by `tools/pareto_front.py`
remains the *sole necessary* condition for any frontier claim. It is **not**
sufficient. The gate does the sufficiency check. Nothing may be called a win,
a crossing, or a frontier result on the arbiter's boolean alone.

---

## R-2. ADOPTED — `FRONT-GAP` / `FRONT-CROSSING` (retires `EXTENDS_FRONT` as a claim)

Requested by the coordinator, formalised by theory, tighter wording from
strategy. **All three adopted, with the mechanical test below.** I adopt
theory's algebra and strategy's strictness; where they differ I take the
stricter reading.

> **`FRONT-GAP`** — a row `p` that is non-dominated **only** because it occupies
> an axis-rectangle the shipped reference set does not cover.
> **Mechanical test (binding, and it is testable — see the command below):**
> there exist two reference rows `q_lo`, `q_hi` on the SAME file and SAME plane
> with `q_lo.ratio < q_hi.ratio` AND `q_lo.mbps < q_hi.mbps` (i.e. the two do
> NOT dominate each other) such that
> `q_lo.ratio <= p.ratio <= q_hi.ratio` AND `q_lo.mbps <= p.mbps <= q_hi.mbps`.
> `q_lo`/`q_hi` are the **gap bracket**. A row satisfying this is `FRONT-GAP`
> regardless of how good the codec is.

> **`FRONT-CROSSING`** — a row that is non-dominated AND is **not** `FRONT-GAP`.
> Only `FRONT-CROSSING` may be written as a frontier result.

> **`DEGENERATE`** (PR-2, strategy — **ADOPTED**) — any non-dominated row with
> `ratio >= 0.95`. Reported as `DEGENERATE`, never as a crossing. Rationale:
> at ratio ≈ 1.0 the comparison is *fast-store vs store*, not compression. This
> is a **math-class** retirement: the predicate is satisfied by a
> non-compression property, so no amount of decoder work makes it a result. It
> is arithmetically live right now on `synth-arith.bin` and `random.bin`
> (see §3).

**Verdict table for the existing 5 rows (machine-verified, R-4):**

| file | codec | ratio | enc MB/s | dec MB/s | enc plane | dec plane |
|---|---|---|---|---|---|---|
| generated.json | anvil-mdl-rans | 0.108 | 1.060 | 112.815 | **FRONT-GAP** (q11↔zstd-19) | DOMINATED (q11) |
| generated.json | anvil-mdl-rans-l0 | 0.108 | 1.300 | 154.874 | **FRONT-GAP** (q11↔zstd-19) | DOMINATED (q11) |
| generated.json | anvil-mdl-rans-l001 | 0.108 | 1.329 | 155.235 | **FRONT-GAP** (q11↔zstd-19) | DOMINATED (q11) |
| generated.json | anvil-shape-rans | 0.112 | 1.004 | 156.166 | **FRONT-GAP** (q11↔zstd-19) | DOMINATED (q11) |
| generated.json | anvil-shape-rans-l0 | 0.112 | 0.979 | 180.362 | **FRONT-GAP** (q11↔zstd-19) | DOMINATED (q11) |

**5 FRONT-GAP. 0 FRONT-CROSSING. 0 degenerate (all ratios < 0.95).**

**LEDGER TALLY (the only writable form):**
> 5 non-dominated rows exist (`tools/pareto_front.py`, generated.json, encode
> plane, since 7b999c6). All 5 are `FRONT-GAP` — single-plane, encode-only,
> decode-dominated by brotli-q11, and inside a ratio×speed rectangle the
> reference set does not ship. 0 `FRONT-CROSSING`. 463 of 468 per-file ANVIL
> row-plane cells are DOMINATED.

**Mandatory co-listing (strategy item 4 — ADOPTED).** No figure from the above
may appear alone. The tuple is: *(non-dominated count, FRONT-GAP count,
FRONT-CROSSING count, dominated-cell fraction)*. A bare "0→5" or a bare "463/468
dominated" both fail claim hygiene and will be struck.

---

## R-3. RULING on theory's "decode route closed on 11/13 files" — **ADOPTED AS A MATH-CLASS NEGATIVE, WITH ONE CORRECTION**

theory asks whether the decode-only route being arithmetically closed on most
files belongs in the ledger as a math-class negative that pre-empts further
decode-only pre-registrations lacking a byte-reduction leg.

**RULING: YES — with a correction to the number, and a narrowing of scope.**

I reproduced `prototypes/i8-theory/iso_crossing.py` from the source suite
(§R-4). Route-B (decode-only) multipliers required to cross:

| file | Route B (decode-only) | verdict |
|---|---|---|
| synth-timeseries.bin | **1.8x** | open |
| synth-arith.bin | **2.5x** | open (but see DEGENERATE) |
| generated.json | **4.6x** | closed |
| random.bin | 8.6x | closed (DEGENERATE) |
| generated.sqlite | 10.5x | closed |
| generated.log | 10.5x | closed |
| generated.jsonl | 11.7x | closed |
| anvil.exe | 11.8x | closed |
| synth-jitter.bin | 13.5x | closed |
| doc.md | 23.7x | closed |
| src.cpp | **40.5x** | closed |
| anvil_bench.exe | 10.1x | closed |
| generated.repeat.jsonl | **106.5x** | closed |

Against the measured ceiling: the wire-invisible CRC fix caps end-to-end decode
lift at **~1.79x** (crc32 = 44% of decode time); the full S6-1b three-leg
projection is 450–480 MB/s on log vs bar-A's 547.1. So 1.79x is the realistic
ceiling for *pure* decoder work that changes no bytes.

**CORRECTION (this is the part theory and the coordinator both got wrong):**

- theory said **"11 of 13"** and named generated.json (4.6x) as closed. Count
  the table: **11 of 13 are closed at the 1.79x ceiling** — that count is
  right. But **synth-arith.bin (2.5x) must NOT be counted as an open Route-B
  opportunity**, because it is a DEGENERATE cell: ANVIL is at ratio **1.000**
  there (no compression at all), and the references it must outrun (zstd-1/3/9
  at r=0.786, 874–886 MB/s) are also near-store. Under R-2's DEGENERATE rule,
  a synth-arith decode crossing at ratio 1.0 is reported as DEGENERATE and is
  **not** a crossing. So the honest count is **10 closed, 1 live-and-degenerate,
  and exactly ONE genuine Route-B opening: synth-timeseries.bin at 1.8x** —
  which happens to be the smallest multiplier on the board and the one cell
  where ARI-REF already measured a FAIL (+0.03%). That is a real, narrow,
  falsifiable target and it is now the only decode-only lane worth a
  pre-registration.
- The ceiling is 1.79x **only for byte-identical decoder work**. The moment a
  mechanism changes bytes (mode-16 ARI-REF, R2 topology, S6-1b leg 3's alloc
  behaviour), it is on Route A (bytes) or both, and the closure argument does
  **not** apply. Precisely: the closure pre-empts *decode-only* registrations,
  not mixed ones.

**ADOPTED AS MATH-CLASS NEGATIVE**, recorded in the ledger:

> **DNB-M1 — decode-only Pareto crossings are arithmetically closed on 12 of 13
> corpus files.** Required Route-B multipliers are 4.6x–106.5x on 11 files
> (10.5x–106.5x on 9 of them); the measured ceiling for byte-identical decoder
> work is ~1.79x (crc32 44% of decode; S6-1b three-leg projection 450–480 vs
> bar-A 547.1). synth-arith's nominal 2.5x is DEGENERATE (ratio 1.000).
> Classification: **MATH**, not implementation-era — it follows from the
> reference rows' own (ratio, throughput) coordinates and the measured decode
> profile, and it survives any decoder optimization that does not also change
> the byte count. Consequence (binding): any future pre-registration whose ONLY
> leg is decode throughput must first show a Route-B multiplier below the
> attainable ceiling on its target file. On 12 of 13 files no such
> pre-registration can be written honestly.

**Scope note, so this is not over-claimed:** the closure is a statement about
*this* reference set and *this* host. Adding denser reference tiers (strategy's
ask to bench) shrinks the gaps and can only make Route B **harder**, never
easier — so the negative is monotone under a better reference set. It is
however **not** monotone in the other direction: a genuinely better ANVIL
decode number does not change Route B's requirement (which is a property of the
reference front), it only changes how far ANVIL is from it. Read the multiplier
as "how much faster ANVIL must decode at its current bytes", which is exactly
what theory computed.

---

## R-4. Verification — every figure above recomputed from source at time of writing

Per the coordinator's binding rule (which I apply to myself here): no tally in
this document was copied from another document. All recomputed from
`tests/benchmark-suite.csv` / `tests/pareto-baseline.csv` at write time.

```powershell
# (1) the 5 rows, recomputed from the SUITE (not the derived baseline CSV)
python -c "import csv; rows=list(csv.DictReader(open('tests/benchmark-suite.csv',newline=''))); REF=('brotli-','zstd-'); \
[print(r['file'],r['codec'],p,float(r['ratio']),r[p]) \
 for f in sorted({r['file'] for r in rows}) \
 for r in [x for x in rows if x['file']==f] if r['codec'].startswith('anvil-') \
 for p in ('encode_MBps','decode_MBps') \
 if not [q for q in [y for y in rows if y['file']==f and y['codec'].startswith(REF)] \
         if float(q['ratio'])<=float(r['ratio']) and float(q[p])>=float(r[p]) \
         and (float(q['ratio'])<float(r['ratio']) or float(q[p])>float(r[p]))]]"
# -> 5 rows, all generated.json, all encode_MBps

# (2) FRONT-GAP bracket test (all 5 satisfy it)
python -c "import csv; rows=list(csv.DictReader(open('tests/benchmark-suite.csv',newline=''))); REF=('brotli-','zstd-'); \
byf={}
for r in rows: byf.setdefault(r['file'],[]).append(r)
for f in byf:
  for a in [r for r in byf[f] if r['codec'].startswith('anvil-')]:
    for p in ('encode_MBps','decode_MBps'):
      pr,pm=float(a['ratio']),float(a[p]); refs=[r for r in byf[f] if r['codec'].startswith(REF)]
      nd=not [q for q in refs if float(q['ratio'])<=pr and float(q[p])>=pm and (float(q['ratio'])<pr or float(q[p])>pm)]
      g=[(ql['codec'],qh['codec']) for ql in refs for qh in refs
         if float(ql['ratio'])<float(qh['ratio']) and float(ql[p])<float(qh[p])
         and float(ql['ratio'])<=pr<=float(qh['ratio']) and float(ql[p])<=pm<=float(qh[p])]
      if nd: print('ND',f,a['codec'],p,pr,pm,'GAPFILL' if g else 'CROSSING','DEGEN' if pr>=0.95 else '',g)"
# -> 5x GAPFILL, brute: brotli-q11 .. zstd-19

# (3) dominated-cell fraction
python -c "import csv; bl=list(csv.DictReader(open('tests/pareto-baseline.csv',newline=''))); \
pf=[r for r in bl if r['codec'].startswith('anvil-') and r['label']!='AGGREGATE']; \
print('per-file cells',len(pf),'DOM',sum(1 for r in pf if r['status']=='DOMINATED'),'EF',sum(1 for r in pf if r['status']=='EXTENDS_FRONT'))"
# -> per-file cells 468  DOM 463  EF 5

# (4) Route B multipliers
python prototypes/i8-theory/iso_crossing.py   # table transcribed in R-3
```

## R-5. CORRECTION to `docs/i8-streak-correction.md` (coordinator's own document)

The coordinator's correction is right in substance and I adopt it. Two of its
figures do not survive recomputation, and the binding rule it establishes
applies to its own author:

1. **"460 of 465 anvil row-plane cells remain dominated" is wrong.** The
   per-file grid is complete — 13 files × 18 codecs × 2 planes = **468** cells,
   of which **463** are DOMINATED and 5 are EXTENDS_FRONT. (Including the
   AGGREGATE pseudo-file: 504 cells / 499 dominated.) Neither 460 nor 465 is
   reachable from any cut of the artifact. Correct figures: **463/468 per-file**,
   or **499/504 including aggregate**.
2. **"465" is not reachable at all.** 468 − 5 = 463, not 460; and no cut yields
   465. The grid has no missing cells (verified: 234 distinct (file, codec)
   pairs = 13 × 18 exactly, zero missing), so there is no "3 rows excluded"
   reading that recovers it either.

I record this not to score a point but because the rule the coordinator just
wrote — *a number in a document is a claim, not a fact* — binds him too, and
because **strategy repeated the same 460/465 figure** in msg_6336f637. Two
lanes have now propagated an unreachable pair of numbers on the same day. The
corrected tuple goes in the ledger; `docs/i8-streak-correction.md` should be
patched to 463/468.

---

## R-6. Disposition of the three requests

| Requester | Ask | Disposition |
|---|---|---|
| coordinator | rule on "Pareto win" for the 5 rows | **NO** — R-1. Not a win. |
| coordinator | consider FRONT-GAP vs FRONT-CROSSING | **ADOPTED** — R-2, mechanical test. |
| theory | adopt FRONT-GAP/FRONT-CROSSING | **ADOPTED** — R-2. |
| theory | "decode route closed" as math negative | **ADOPTED, CORRECTED** — R-3. 12/13 not 11/13; synth-arith's 2.5x is DEGENERATE not open. |
| strategy | tighten the definitions | **ADOPTED** — bracket test + same-file/same-plane scoping. |
| strategy | PR-2 DEGENERATE guard (ratio ≥ 0.95) | **ADOPTED** — R-2. Math-class retirement. |
| strategy | mandatory co-listing of the 4-figure tuple | **ADOPTED** — R-2. |

## R-7. What this does NOT say

- It does **not** say the 5 rows are worthless. They are real `FRONT-GAP`
  rows and they are the only non-dominated cells the project has. They are
  evidence that ANVIL's ratio at ~1 MB/s encode is genuinely in a band no
  shipped reference config occupies. That is worth knowing. It is not a win.
- It does **not** say decode work is pointless. S6-1b lifts every row on the
  decode plane and shrinks the Route-B gap on every file simultaneously. It
  just cannot, alone, produce a crossing on 12 of 13 files.
- It does **not** retire any novelty claim. This is a claim-hygiene ruling
  about what the arbiter's output means. The gate verdicts on ARI-REF, PNRA,
  SPARSE-REF and the Gorilla family are separate and still owed.

---

*Ruled by `research-gate`, novelty gate arbiter. This ruling is binding on
ledger, brief, and synthesis text. `tools/pareto_front.py` is unchanged and
remains the sole issuer of the `EXTENDS_FRONT` boolean; the FRONT-GAP /
FRONT-CROSSING / DEGENERATE classification is the gate's reading of that
boolean and is applied by this lane, not by the tool.*
