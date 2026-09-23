# ANVIL — Iteration-8 Strategy Synthesis (the Pareto-offensive theme, recomputed) + the Iteration-9 plan

Author: `strategy` (strategy synthesist, swarm anvil-i8-pareto). Status: **analysis
only** — no production code, no benchmark run by this lane, no arbiter run by this
lane, no novelty verdict. Every number below is either (a) recomputed by me from a
committed CSV or (b) arithmetic on a peer's landed measurement, explicitly labeled
`[PROJECTION]`. Where a result is pending it says PENDING, never a guess.

Hard constraints honored: I do not edit `src/anvil.cpp` (arch), I do not run
`tools/pareto_front.py` (bench), I do not issue novelty verdicts (research-gate).

**Patch discipline (inherited from I7):** this document is patched, not rewritten.
§7 (Endgame accounting) is the only section rewritten wholesale at iteration end.

---

## 0. HEADLINE — one erratum, one relocation

Two results from the first recomputation pass, both of which change what the
iteration is about:

1. **The mission premise is false.** `docs/swarm-i8-brief.md` and
   `docs/swarm-i7-strategy.md` §1 both assert "0 EXTENDS_FRONT in project
   history". There are **5**, committed at HEAD since `7b999c6` (2026-08-17).
   Full evidence in `deliverable/strategy-i8-audit` AUDIT-1. The coordinator
   accepted the correction and independently verified it; the narrative rule is
   **record, do not celebrate** — all five are single-plane (encode only), all
   five are dominated on the decode plane, and all five are a **front-knee
   artifact** (AUDIT-1b).

2. **The closest cell is NOT synth-arith.bin — it is synth-timeseries.bin.** The
   brief ranks synth-arith first on the byte axis (harness wire +2.7% from front)
   and lists synth-timeseries at "-14.3%" as a mid-table byte problem. But
   synth-timeseries is the *closest cell on both axes at once*: it needs **−14.3%
   bytes OR 1.78× decode** — the lowest requirement on the board on the decode
   side, and second-lowest on the byte side. synth-arith's apparent decode
   "escape" (§3.3) is **degenerate**: it is a ratio-1.000 file, so crossing there
   means "don't compress it, decode it fast," which is not a compression result.
   This is the single most important relocation in this synthesis.

---

## 1. The frontier arithmetic, recomputed (not inherited)

Arbiter rule, re-implemented from `tools/pareto_front.py` lines 28–41 (I did not
run the tool; I re-derived its predicate and reproduced its committed output
exactly — 468 anvil row-plane cells, 5 non-dominated, the same five):
`q dominates p iff q.ratio <= p.ratio AND q.mbps >= p.mbps, one strict`.

Source: `tests/benchmark-suite.csv` (13 files × 27 codecs, median-3), HEAD `fc23d9a`.

### 1.1 Frozen per-file gap — RECOMPUTED, two errata found

**Column semantics (corrected after theory's review — see §2.4).** The "needed"
column below is the gap to the **best reference row**, not the gap to the
crossing budget at ANVIL's current decode speed. They differ where the binding
reference at current speed is not the smallest row (e.g. src.cpp: −15.7% vs
q11, but −12.2% against the crossing budget). Both are correct; they answer
different questions. Theory's iso-crossing curve (`deliverable/theory-i8-iso-crossing`)
is the authority for the second quantity.

| file | input | best ANVIL (bytes, codec) | best REF | needed |
|---|---|---|---|---|
| generated.json | 827,664 | 89,589 mdl-rans | 79,172 q11 | **−11.3%** |
| src.cpp | 32,512 | 9,483 dp-arith | 7,991 q11 | −15.7% |
| synth-timeseries.bin | 280,000 | 140,898 hotop-rlzp | 120,669 q6 | **−14.3%** |
| generated.jsonl | 2,815,267 | 183,506 mdl-rans | 150,619 q11 | −17.2% |
| anvil.exe | 268,800 | 107,468 mdl-rans | 88,322 q11 | −17.7% |
| anvil_bench.exe | 1,929,216 | 830,331 mdl-rans | 646,675 q11 | −22.2% |
| synth-arith.bin | 256,000 | 256,022 **r=1.000** | 87,013 q11 | −66.0% |
| doc.md | 2,724 | 1,444 dp-arith | 1,047 q11 | −27.6% |
| synth-jitter.bin | 974,920 | 87,412 mdl-rans | 62,717 q11 | −28.6% |
| generated.log | 1,942,280 | 129,739 shape-rans | 81,946 q11 | −37.1% |
| generated.sqlite | 1,740,800 | 323,014 mdl-rans | 185,702 q11 | −42.5% |
| generated.repeat.jsonl | 940,000 | 873 dp-arith | 162 q11 | −81.4% |
| random.bin | 262,144 | 262,166 r=1.000 | 262,149 q1 | tied (control) |

**ERRATUM vs the brief (AUDIT-4):** the brief lists generated.json at **−11.6%**
and generated.jsonl at **−18.0%** and synth-timeseries at **−14.3%**. Recomputed:
**−11.3%**, **−17.2%**, **−14.3%**. The brief's json/jsonl figures are computed
against *rounded ratios* (0.108/0.096 and 0.065/0.054) rather than exact bytes;
exact-byte arithmetic gives 11.33% and 17.22%. Small, but this is exactly the
class of unrecomputed number the coordinator's binding rule now forbids. All
other rows reconcile to the brief within rounding.

### 1.2 Strong-crossing test — the honest headline

I tested a stronger predicate than the arbiter's: does any anvil row **dominate**
a reference row on *both* throughput planes (ratio, encode, decode)?

> **NONE.** Zero anvil rows dominate any brotli/zstd row on both planes.

And on (ratio + decode) only, ignoring encode, exactly three rows beat anything,
and what they beat is `brotli-q1` on synth-timeseries (hotop-rans 0.512/150.6,
hotop-budget 0.512/163.6, hotop-rlzp 0.503/158.4 vs q1 0.579/139.7). Beating
**q1** is audit-trap class (d) in my lane's brief — not a citable result.

So the truthful state of the project is: **5 EXTENDS_FRONT rows exist and are
real per the sole arbiter; 0 are strong crossings; 463 of 468 per-file anvil
row-plane cells are dominated** (499/504 including AGGREGATE).
"0 to 5" and "we crossed the frontier" are both false.

> **ERRATUM (self, corrected by research-gate).** Earlier versions of this
> document — and my messages to the coordinator, bench, arch and research-gate
> — stated "460 of 465". That pair is **not reachable from the artifact** and I
> have not been able to reconstruct where it came from; it appears to be a
> number I carried rather than recomputed. Recomputed: **468 per-file cells, 5
> non-dominated, 463 dominated**; AGGREGATE adds 36 cells, all dominated,
> giving **499/504** combined. This is the *same failure mode* as the
> coordinator's "0 EXTENDS_FRONT" tally error, with me as the author — one
> iteration after I caught it in him. Recorded here rather than quietly fixed.
> The 460/465 figure also propagated into `docs/i8-streak-correction.md` line 74
> (flagged to coordinator) and `docs/swarm-i8-brief.md` line 19 (flagged to
> coordinator). **The recompute rule binds me too.**

### 1.3 Front thinness — why the five rows exist

Reference-front point counts per plane (non-dominated reference rows):

| file | encFront | decFront |
|---|---|---|
| generated.json | 6 | **2** |
| generated.log | 6 | **2** |
| generated.sqlite | 7 | **2** |
| synth-arith / jsonl / src.cpp / anvil.exe | 6 | 3 |
| random.bin, repeat.jsonl | 1 | 1 |

The encode front on generated.json is 6 points but the **decode front is 2**
(q11 0.096/507.4, zstd-19 0.113/1506.5). A 2-point front means any point with
ratio in (0.096, 0.113) and decode in (507.4, 1506.5) is non-dominated
automatically. The five rows sit in the *encode* analogue of that rectangle.
This is a property of the shipped reference configuration space, not of ANVIL.

---

## 2. The decode-floor arithmetic (the lane's central question)

### 2.1 Reachable bands — recomputed, and they reconcile

decode-perf's t3 profile (`deliverable/t3-decode-floor-profile`, median-7
interleaved, generated.log payload): crc32 **44%**, masks+resid eager
materialization **26%**, block concat/alloc **~15%**, token loop **11%**.

Amortized-multiplicative model: removing time-share `s` yields lift `1/(1-s)`
(sequential-removal product). Bands:

| fix set | removed share | lift |
|---|---|---|
| CRC only | 0.44 | **1.79×** |
| CRC + materialization | 0.70 | **3.33×** |
| CRC + mat + alloc @75% | 0.8125 | **5.33×** |
| CRC + mat + alloc @100% | 0.85 | **6.67×** |

**Method validation:** CRC-only gives 1/(1−0.44) = **1.786×**, which reproduces
decode-perf's own stated "~1.79x" cap exactly. The model is the right one.
The multi-leg bands are `[PROJECTION-UPPER-BOUND]` — they assume the shares are
independent and each leg is fully (or 75%) recoverable. They are **not**
measurements, and per theory's review (§2.1a) they are **upper bounds, not
estimates**: lazy materialization *defers* work rather than deleting it (the
bytes still must be produced before the token loop consumes them), and
concat/alloc is partly a *consequence* of eager materialization. The legs are
therefore negatively correlated and their combined saving is **less** than the
product of the individual lifts. Only the CRC-only 1.79× is **ANCHORED** (it
reproduces decode-perf's own independently stated cap).

### 2.1a theory's review of the band model — multi-leg bands are UPPER BOUNDS

theory answered the objection I raised against my own model
(`deliverable/theory-i8-cost-objective`): the legs are **not independent**, so
the multi-leg bands are optimistic in a way the CRC-only figure is not.

- Lazy materialization **defers** work rather than deleting it — the masks+resid
  bytes still have to be produced before the token loop consumes them, so the
  cost moves into the token loop rather than out of the decoder.
- Concat/alloc is partly a **consequence** of eager materialization (you
  allocate because you materialize into a buffer), so removing materialization
  should *reduce* alloc. Negatively correlated legs ⇒ combined saving is less
  than the product of individual lifts.

Accepted in full: **CRC-only 1.79× = ANCHORED** (reproduces a measurement);
**CRC+mat 3.33× and CRC+mat+alloc 5.33× = PROJECTION-UPPER-BOUND**. This matters
for the ranking because generated.json's 4.50× requirement was compared against
the 5.33× upper bound — the margin there is thinner than the table implies, and
if the true three-leg lift lands below 4.50× then generated.json's decode route
closes too, leaving synth-timeseries as the sole decode cell.

theory also **overturned their own λ conclusion** and asked that the
"budget selection at higher λ" escape hatch be closed rather than opened:
measured `λ_frontier` (dL/dt along the reference decode front) is 14.2–142.0
B/µs across files, while the implied willingness-to-pay of the flips S6-1
declined is 1,378–4,712 B/µs — i.e. the flips were **40–160× more expensive
than the market rate**, so S6-1 declined them correctly at any λ in the market
band. Independent convergence: against `L*(t)`, every flip moves the row
*further* from the front on all four files tested. **No higher-λ line exists in
this plan and none will be added.**

### 2.2 Crossed against the per-file gaps — the practical target list

Decode bar = strictly exceed the max decode among references whose ratio ≤ the
anvil row's ratio. Byte bar = strictly beat the min ratio among references at
least as fast as the anvil row's current decode.

| file | anvil ratio | anvil dec | **decode bar** | **lift needed** | band that clears |
|---|---|---|---|---|---|
| **synth-timeseries.bin** | 0.503 | 158.4 | 282.2 (q6) | **1.78×** | **CRC-only (1.79×)** |
| **synth-arith.bin** | 1.000 | 362.5 | 885.8 (zstd-3) | 2.44× | CRC+mat (3.33×) — **DEGENERATE, see §3.3** |
| **generated.json** | 0.108 | 112.8 | 507.4 (q11) | **4.50×** | **CRC+mat+alloc@75 (5.33×)** |
| random.bin | 1.000 | 320.6 | 2,699.7 | 8.42× | none |
| src.cpp | 0.292 | 26.7 | 1,083.7 | 40.55× | none |
| doc.md | 0.530 | 19.9 | 469.7 | 23.62× | none |
| repeat.jsonl | 0.001 | 234.4 | 24,479.2 | 104.45× | none |
| sqlite / log / jitter / jsonl / PEs | 0.09–0.19 | 122–203 | 1,253–2,187 | 10.3–13.2× | none |

**Read this correctly — three cells, and only three, are reachable on the decode
plane under the t3 profile at unchanged bytes:**

- **synth-timeseries.bin: 1.78× needed vs 1.79× available.** This is the closest
  cell in the project. It is also *razor-thin*: 0.6% of margin, against a t3
  measured CV of ~10%. It is not a prediction, it is a **coin-flip that must be
  measured**, and it is the highest-value single measurement in Iteration 9.
- **generated.json: 4.50× needed vs 5.33× projected** for the three-leg fix.
  Genuine (ratio 0.108 is real compression, and at 5.33× the binding reference
  becomes zstd-19 at 0.113, which ANVIL *already beats* on ratio at 0.108).
  This is the first crossing that would survive a scientific reading — **if**
  the three-leg decode fix actually lands in that band.
- **synth-arith.bin: 2.44× needed vs 3.33× projected** — but see §3.3, it is
  degenerate and must not be claimed.

Everything else (8.4×–104×) is out of reach of any wire-invisible decode fix.
For those cells the **byte** axis is the only axis, and the bars are the §1.1
column: src.cpp −15.7%, sqlite −42.5%, log −37.1%, and so on.

### 2.4 Independent corroboration (theory) — and one decisive generalisation

`theory` published `deliverable/theory-i8-iso-crossing` in parallel: the
iso-crossing curve `L*(t) = N·R_ref(t)` where `R_ref(t) = min{q.ratio :
q.decode ≥ t}`, a decreasing step function with cliffs at each reference row's
throughput. Their Route-A/Route-B table is computed independently of mine and
**agrees cell by cell** with my §2.2 bars:

| file | my decode bar (§2.2) | theory's Route-B multiple | verdict |
|---|---|---|---|
| synth-timeseries | 1.78× | 1.8× | match |
| generated.json | 4.50× | 4.6× | match |
| synth-arith | 2.44× | 2.5× | match |
| src.cpp | 40.55× | 40.5× | match |
| doc.md | 23.62× | 23.7× | match |

Two lanes, two methods, same numbers. **The decode-band arithmetic is now
cross-validated and I treat it as settled.**

Their headline generalisation, which I adopt verbatim: **on 11 of 13 files the
decode route is mathematically closed** (4.6×–40× required vs a ~1.79×
wire-invisible cap); only synth-timeseries (1.8×) and synth-arith (2.5×) have a
Route-B multiple under 3×. Combined with my degeneracy finding (§3.3), the
consequence is sharper than either lane stated alone:

> **synth-timeseries.bin is the ONLY cell in the corpus where the decode route
> is both arithmetically reachable AND non-degenerate.** generated.json is
> reachable only via the *three-leg* projection (5.33×), which is a projection,
> not the 1.79× measured cap.

Two further corrections theory's curve supplies to my §2.2, both accepted:

- **generated.json has a second, cheaper crossing than I credited.** At ≥517.5
  MB/s decode the byte budget jumps from 79,456 B to **93,526 B**, and ANVIL's
  current best row is 89,589 B — i.e. **+3,937 B of slack already exists**. So
  generated.json's decode bar is a *step*, and the useful step is lower than my
  uniform 4.50× suggested once the q11 row drops out of the min.
- **src.cpp is −12.2%, not −15.7%.** Theory's curve gives the Route-A budget as
  +1,160 B on 9,483 (12.2%), where my §1.1 figure compares against brotli-q11's
  ratio 0.246 → 7,991 B. The discrepancy is which reference row binds at
  ANVIL's *current* decode; at 26.7 MB/s the binding row for the byte budget is
  not q11. **My §1.1 column is "gap to the best-reference row"; theory's is
  "gap to the crossing budget at current speed". They are different quantities
  and I should have labelled mine. §1.1 is renamed accordingly.**

- **The front-gap formalism.** Theory proposes: a row is a **FRONT-CROSSING**
  iff non-dominated AND outside every axis-rectangle gap of the reference
  front; otherwise a **FRONT-GAP FILL**. That is the mechanical version of my
  AUDIT-1b and my proposed PR-3 token. I withdraw my own wording in favour of
  theirs — it is stricter and it is arithmetically testable.

### 2.5 synth-arith.bin is not degenerate after all — pnra re-opens it

`deliverable/pnra-i8-invariant-stride` (pnra) lands the generative structure of
synth-arith.bin, measured on this host: it is **piecewise-linear with periodic
reseeding** — 64,000 uint32 values in **8 runs of exactly 8,000**, each run an
integer-slope affine sequence (slopes 64, 83, 79, 69, 96, 48, 91, 2). A single
global fit is garbage (global lstsq slope −1.35, residual std 266,809) — which
is *why* the file looks incompressible and ANVIL sits at ratio 1.000.

Their modelled ceiling: per-run slope + modal intercept, zero-order entropy of
the residual → **49,585 B total**, vs brotli-q11 87,013 B (**−43.0%**) vs ANVIL
today 256,022 B (**−80.6%**). Stated as a *modelled ceiling* — entropy-coder
overhead and parameter transmission are NOT included, so the real landing is
above it. Even so the headroom over q11 is ~37 KB.

**This changes the cell's status and I am amending my own ranking.** I had
marked synth-arith rank 3 and its decode route degenerate. On pnra's numbers the
**byte** route is materially more promising than the brief or I credited: the
file is not incompressible, it is *unmodelled*. Two consequences:

1. The degeneracy guard (AUDIT-5) still binds and must stay — a decode-plane
   row at ratio ≥0.95 is still not a compression result. But pnra's result means
   synth-arith should be attacked on **bytes**, where the ceiling is −43% below
   the front, not on decode.
2. Their negative result scopes the mechanism honestly: a **single global**
   slope shows NO signal on drift-stride, timeseries, jitter, or sqlite. The
   mechanism must discover piecewise-homogeneous linear regions. So this does
   **not** help synth-timeseries (my rank 1) — the two cells need different
   mechanisms, and no single lane covers both.

pnra's Windows PE .text numbers (anvil.exe .text: ANVIL-proto raw+pnra 126,578
vs brotli q6 81,829; anvil_bench .text 854,525 vs q6 526,371) are a declared
**control/baseline, not a win** — 30–55% above q6. Consistent with the Linux ELF
verdict; recorded, not ranked.

### 2.6 pnra's measured delta ladder — REPRODUCED BYTE-EXACT, but it is not an ANVIL result

pnra retracted their T7 piecewise-linear figure (a genuine bug: length-1
segments give an identically-zero residual while the per-segment base is not
charged — the transform was paid for deleting data and not for carrying it).
Good hygiene, recorded here because I had cited their numbers.

They then published a **transform ladder scored by brotli** on synth-arith.bin
(raw 256,000 B, brotli-q11-on-raw = 87,013 B). I re-implemented T1/T2/T9 from
their description and **reproduced all three byte-exactly**:

| transform | pnra | my reproduction | vs 87,013 |
|---|---|---|---|
| T0 identity (raw) | 87,013 | 87,013 | — |
| T1 delta u32 LE | 17,311 | **17,311** | −80.1% |
| T2 delta + zigzag varint | 16,313 | **16,313** | −81.3% |
| T9 delta + byte-lane split | 16,195 | **16,195** | −81.4% |

**Read this correctly — it is the largest number on the board and the easiest to
misread.** This is *brotli doing the coding on transformed bytes*. It is not an
ANVIL codec result and must never be entered as one (pnra insists on the same).
What it establishes is that synth-arith.bin has **~16 KB of recoverable
structure** where brotli-on-raw sees 87 KB. The strategic question is whether
ANVIL can express delta/stride **natively**; every byte of that gap ANVIL
captures natively is a direct gain against the 87,013 B front.

**Novelty status is the ranking problem.** pnra states plainly that T1/T2/T9 are
delta coding + byte-lane transposition — both textbook prior art — and that
**global byte-lane transposition before LZ is already a REJECTED idea in this
project**. So the transform carries no novelty; the only defensible claim is
putting a *discovered, position-derived* transform **inside the reference**
(a transformed/patch copy), which is the separation TCOPY already drew against
BCJ. Referred to research-gate (§5, PR-5).

**Anatomy partially reproduced — discrepancy flagged to pnra.** My
recomputation confirms the delta alphabet is small (31 distinct deltas
file-wide, ~4 per claimed 8000-run), consistent with 8 base slopes plus jitter.
But the claimed per-run slopes (64, 83, 79, 69, 96, 48, 91, 2) do **not** match
my measured mean delta per claimed run (−43.3, +9.2, +15.8, −56.3, +83.1,
−11.7, −31.9, +2.0) — only 2 of 8 agree. The 8000 boundary is therefore
questionable. This does **not** touch the verified T1/T2/T9 numbers, but the
anatomy should not be cited until pnra reconciles it.

### 2.7a arch's crossing corridors on synth-arith — VERIFIED, and they correct me twice

arch recomputed the synth-arith crossing with the arbiter's own `dominated()`
(`prototypes/i8-arch/crossing_check.py`) and challenged my §2.2/§2.5 treatment.
I verified their corridors independently and **they reproduce exactly**:

| decode band | max bytes still EXTENDS_FRONT | binding ref |
|---|---|---|
| ≤187.518 MB/s | 86,911 B | brotli-q11 (0.340) |
| (187.5, 325.5] | **106,368 B** | zstd-19 (0.416) |
| (325.5, 885.8] | 201,088 B | zstd-1/3/9 (0.786) |

Spot checks confirm their four published numbers: 87,013 B @187.0 →
EXTENDS_FRONT; 89,363 B @188.0 → EXTENDS_FRONT; 106,000 B @200 →
EXTENDS_FRONT; 107,000 B @200 → DOMINATED by zstd-19.

**Correction 1 (accepts arch, corrects the brief and me).** The brief's bar of
"≤87,013 B" is wrong because it ignores the decode cliff at q11's own speed.
Above 187.5 MB/s decode the bar widens to 106,368 B — a **19.5 KB corridor, not
1.65 KB**. The 89,363 B harness wire crosses anywhere above 187.5 MB/s, and
today's synth-arith row already decodes at ~362–390 MB/s. So mode-16 does not
need to beat q11's size; it needs to beat q11's *speed*, which it already does.

**Correction 2 (accepts arch, and it is a real error in my PR-2 guard).** I
wrote the degeneracy guard against the **FILE** (synth-arith is at ratio 1.000,
therefore degenerate). arch is right that degeneracy is a property of the
**ROW**: a mode-16 row at ~89,363 B has ratio 0.349, not ≥0.95, and the whole
point of mode-16 is to move the cell from 1.000 to ~0.349. My guard as drafted
would have vetoed a legitimate crossing. **PR-2 is amended: the ≥0.95 test
applies to the row's own ratio, not to the file's current best row.** What
survives of AUDIT-5 is the real phenomenon — a decode-plane row at ratio ≈1.000
is store-vs-fast-store — but it must not be used to disqualify a row that has
actually compressed. I thank arch for pressing the distinction rather than
accepting the skip.

**Consequence: synth-arith is promoted back to co-Rank 1.** It is the one cell
where a *measured* ANVIL artifact (89,363 B harness wire) sits inside a wide,
arbiter-verified corridor, and where the byte route and a modest decode floor
are simultaneously satisfiable. Ranking patched to v4.

### 2.7b pnra's 8 slopes — REPRODUCED; my discrepancy report was MY bug

pnra refuted my §2.6 anatomy discrepancy and they are right; the error was
mine. Chunking the **delta array** at multiples of 8000 computes a telescoping
mean, `mean(d[a:b]) = (v[b]−v[a])/(b−a)`, which spans the reseed boundary and
swallows one large negative jump (−858,072 for run 0, etc.). I verified the
identity holds to 1e-9 on all 8 chunks. Measuring deltas **strictly inside**
each run (`sd[k*8000 : (k+1)*8000 − 1]`) reproduces pnra exactly:

| run | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| mean in-run delta | 64.004 | 83.004 | 79.004 | 68.991 | 96.000 | 48.004 | 91.015 | 1.996 |
| pnra's slope | 64 | 83 | 79 | 69 | 96 | 48 | 91 | 2 |
| distinct deltas | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 3 |

Every run has **exactly 3 distinct deltas at {s−1, s, s+1}** — the signature of
`v += stride + randint(-1,1)`. That is decisive: any misalignment of the
boundary would inject a jump and blow the distinct count past 3. Structure,
boundaries and slopes are all correct.

**The NOT-CITABLE mark on pnra's §1 anatomy is released.** I also ran the
AUDIT-7 check pnra proposed on the ladder: ~12,680 B of residual entropy
(8000·log₂3 per run) plus 8 slopes, 8 bases and 7 boundaries is consistent
with the measured 16,195 B — the parameters are charged. **PASS.**

Two things I am carrying forward from this exchange: (i) my §2.6 discrepancy
was a boundary-attribution error, which pnra correctly identified as a milder
cousin of AUDIT-7 — not an uncharged parameter, but a boundary artifact
attributed to the model; AUDIT-7's description is widened to cover both.
(ii) I was wrong in public and corrected by a peer; that is the mechanism
working, and it is recorded rather than smoothed over.

### 2.7b-i T9 convention RECONCILED (coordinator vs me): both right

The coordinator independently re-implemented the ladder and got T9 = **15,933 B**
where I reported **16,195 B** (a 262 B / 1.6% gap). Reconciled: the two
conventions differ only in whether zigzag is applied **before** the 4-lane
transpose.

| convention | T9 bytes |
|---|---|
| raw-delta LE u32, then 4-lane transpose (mine) | **16,195** |
| zigzag-mapped LE u32, then 4-lane transpose (coordinator) | **15,933** |

Both reproduce exactly; neither is wrong. The structure claim survives
untouched (a 1.6% convention difference inside an 81% effect). **Cite T9 as
"~15.9–16.2 KB, convention-dependent" until a single convention is frozen.**

### 2.7b-ii The decisive finding: the synth-arith gap is REPRESENTATIONAL

The coordinator ran ANVIL's **own** codec on the pre-transformed buffers — the
sharpest evidence yet on this cell:

| buffer | anvil-mdl-rans | brotli-q11 |
|---|---|---|
| T1 delta | **17,301 B** (r 0.068) | 17,311 B |
| T9 | 18,035 B | 15,933 B |
| raw file | 256,022 B (r 1.000) | 87,013 B |

Once the delta structure is exposed, **ANVIL's own entropy coder matches
brotli-q11 on the same buffer (T1: 17,301 vs 17,311)**. So the −66% gap is
**not** an entropy-coding deficit — it is entirely parser/transform discovery.
`[Caveat, mandatory: pre-transformed input, transform uncharged, NOT an ANVIL
codec result.]` This is exactly the research-gate framing that the 87,013 B
"front" is brotli declining to apply a filter it ships elsewhere, and it
sharpens PR-5's sequencing question: before building a new token type, the
cheaper question is why ANVIL does not find a delta it can already code.

### 2.7c synth-timeseries.bin structure — CONFIRMED (28-byte records)

I independently verified pnra's record-structure finding on my Rank-1 cell:
280,000 B = **exactly 10,000 × 28-byte records**. Per-byte-offset distinct-value
counts match pnra exactly (offsets 4–7 and 18–21 → **1 distinct**, i.e. fully
constant; 3 and 17 → 3 distinct, top freq 0.839; 11/25 → 14–15; 12/26 → 128;
13/27 → 4). Layout: `[0..7]` u64 timestamp, `[8..15]` f64, `[16..23]` u64
timestamp, `[24..27]` u32 counter.

This **explains my Rank-1 cell's failure mode**: it is a columnar/record-stride
problem, not an arithmetic one — which is why pnra's global-slope probe showed
no signal there, and why ANVIL's LZ/reference machinery sees 140,898 B where
brotli-q6 sees 120,669 B. The natural invariant is `I(x,p) = x[p] − x[p − 28]`
(record-period delta), a *different* mechanism from synth-arith.

**Consequence: my two top cells are confirmed to need different mechanisms, and
neither is covered by a mechanism currently in `src/anvil.cpp`.** That is the
central planning fact of Iteration 9.

### 2.3 What this says about the asymmetry

The brief's asymmetry statement is correct but incomplete. ANVIL loses on bytes
by 11–66% AND on decode by 2–13×. The decode-floor arithmetic says the decode
gap is **closable on exactly two cells** (timeseries, json) with the profile's
own measured shares, and **not closable anywhere else by any wire-invisible
means**. So decode work is not a general strategy — it is a **two-cell
strategy**, and every other cell is byte-blocked permanently until a ratio
mechanism lands. That is the correct shape of the Iteration-9 plan.

---

## 3. Audit findings (self-deception, named)

Full text: `deliverable/strategy-i8-audit`. Summary:

- **AUDIT-1 (CRITICAL, ERRATUM).** "0 EXTENDS_FRONT in project history" is false;
  5 exist, committed, since `7b999c6`. Reproduced three ways (file, `git show
  HEAD:`, re-implementation of the arbiter predicate). **Lane named: coordinator
  (brief) and I7 synthesis.** Coordinator accepted and recorded it as his own
  error in `docs/i8-streak-correction.md`.
- **AUDIT-1b (self-correction).** The five rows are a **front-knee artifact**,
  not a crossing. They occupy a ratio×speed rectangle that the 2–3 point
  reference front does not cover. My own first read ("a genuine hole-crossing")
  was wrong and is retracted on the record in the same document.
- **AUDIT-2 (open).** No lane has produced an I8 decode number yet; nothing to
  audit. Pre-registered trap watch active: (b) decode win on a grown wire,
  (e) Linux number as Windows number, (f) EXTENDS_FRONT without the arbiter.
- **AUDIT-3 (open, pre-emptive).** `docs/CONTEXT.md` is saturated with Linux/EPYC
  numbers (shape-predict 1,099 MB/s decode; log 913 MB/s; hot-op hybrid
  0.87–0.99 GB/s; TCOPY .text 1,761,776 B vs q4 1,781,130 B; PNRA 1,730,689 B).
  None is measured on this host or corpus. Per I7 §10's own rule none may be
  cited as evidence on the Windows line. I will strike any lane that does.
- **AUDIT-4 (new).** The brief's −11.6%/−18.0% for json/jsonl are computed from
  rounded ratios; exact bytes give −11.3%/−17.2%. See §1.1.
- **AUDIT-5 (new, and the important one).** **Degenerate escape.** A cell where
  ANVIL achieves ratio ≈ 1.000 can become "non-dominated" purely by decoding
  fast, because the only faster references are also at ratio ≈ 1.000. This is
  true for **synth-arith.bin (r=1.000, bar 885.8 MB/s)** and **random.bin
  (r=1.000, control)**. A decode-plane EXTENDS_FRONT on either would be a
  **store-vs-fast-store** comparison, not a compression result. **Any such row
  must be reported as DEGENERATE and is not a crossing.** I am pre-registering
  this so it cannot be celebrated by accident.
- **AUDIT-6 (new).** **Measurement isolation is a live audit exposure.** Per the
  coordinator's advisory, 7 lanes on a 12c/24t host benchmarking concurrently
  will corrupt each other's timings; `tests/noise-floor.csv` shows reference
  decode CVs of 25–42% on doc.md/jsonl (brotli-q4 41.8%, zstd-9 38.5%, zstd-19
  33.8%). Any throughput number published without a stated measurement window is
  **provisionally non-citable** in my synthesis. This directly threatens the
  synth-timeseries 1.78×-vs-1.79× call, whose entire margin is 0.6%.

---

## 4. Ranked lanes by expected Pareto movement per unit of remaining effort

LANDED numbers only; no mechanism ranked above a measurement.

**Rank 1 (tie) — mode-16 ARI-REF on synth-arith.bin (arch).** `[PATCH v4 —
promoted back from rank 4]` arch's corridor arithmetic (§2.7a) shows the bar is
106,368 B above 187.5 MB/s decode, not 87,013 B: the harness wire at 89,363 B
already sits inside a 19.5 KB corridor, and the current row decodes at
~362–390 MB/s. This is the only cell where a **measured** ANVIL artifact is
already inside an arbiter-verified corridor. Gating risks: the container has
never been measured end-to-end, and PR-5's novelty question applies to the
native-stride form. My earlier "degenerate" demotion was wrong (§2.7a).

**Rank 1 (tie) — the synth-timeseries decode profile + CRC-only fix (decode-perf,
then arch).** Needed 1.78×, available 1.79× per the t3 profile. Caveat: the t3 profile
was measured on **generated.log, mode-15 hot-op streams** — its transfer to
synth-timeseries is an *assumption*, and the margin is inside the noise band.
**The single highest-value action in I9 is measuring the actual decode profile of
synth-timeseries (and generated.json) directly.** Effort: low (one profile run).
EV: it either converts the closest cell into a measured crossing or kills it
honestly. If the timeseries CRC share is materially below 44%, this rank dies and
generated.json (Rank 2) becomes the decode lead.

**Rank 2 — three-leg decode fix → generated.json decode crossing (arch +
decode-perf).** 4.50× needed vs 5.33× projected for CRC+mat+alloc@75%. Genuine,
non-degenerate, and at that band the binding reference flips to zstd-19 (0.113)
which ANVIL already beats at 0.108. Effort: medium-high (three staged legs, each
needing byte-identity). This is the first crossing that survives a scientific
reading. Sequencing gate: leg attribution must be staged (CRC → mat → alloc) so
a partial result is attributable.

**Rank 2 — synth-arith byte route via a NATIVE delta/stride reference (arch,
with pnra's transform ladder as the target).** `[PATCH v3]` The target is now
**measured, not modelled**: pnra's T9 ladder reaches **16,195 B** where
brotli-on-raw sees 87,013 B — and I reproduced it byte-exactly (§2.6). That is
−81.4%, the largest gap-to-front on the board by a wide margin. **But the
number is brotli-coded-transformed bytes, not an ANVIL row**: the ranking value
is not "we have 16 KB", it is "there are ~71 KB of structure here and the
question is how much ANVIL can capture natively". The enabling work is a
**native delta/stride reference** (a parser feature), not invariant algebra —
pnra's own conclusion is that the sigma/slope formulation is the wrong basis
on this file (their grid-sigma linear-invariant run reached only 84,429 B vs
the delta ladder's 16,195 B). **Novelty is unresolved and is the gating risk:
delta coding and byte-lane transposition are textbook, and global pre-LZ
transposition is already retired here. Only "discovered position-derived
transform INSIDE the reference" is defensible — referred to research-gate
(PR-5).** If the gate rejects that, this rank collapses and the cell is
byte-blocked at −66% with no mechanism.

**Rank 3 — three-leg decode fix → generated.json (arch + decode-perf).** 4.50×
needed vs 5.33× projected for CRC+mat+alloc@75%. Non-degenerate, and at that
band the binding reference flips to zstd-19 (0.113) which ANVIL already beats
at 0.108. Sequencing gate: legs staged for attribution.

**Rank 4 — mode-16 ARI-REF end-to-end on synth-arith.bin (arch).** The I7
rank-1, **demoted twice** — first behind the decode cells, now behind the
invariant route on its own file. Why still ranked: +2.7% from the front is a
real artifact and the frozen gate bar (≤0.80× = ≤204,818 B) is deliberately
conservative. **Hard conditions: (i) check any landing against AUDIT-5 — at
ratio ≈1.000 the decode row is DEGENERATE and not claimable; (ii) compare
against pnra's 49,585 B ceiling — if ARI-REF lands above it, the invariant
route wins the file.**

**Rank 5 — synth-timeseries byte axis (datastruct).** −14.3% needed
(140,898 → 120,669 vs brotli-q6). The only cell where a byte win pays on
**both** axes. Gorilla/vXOR family (I7 §10) is the pre-registered direction.
**Scope caveat (pnra §4):** the piecewise-linear invariant shows no signal on
timeseries, so this needs a *different* mechanism — the top two cells are not
covered by one lane.

**Rank 6 — reference-front reconnaissance on the 9 newer corpus cells (bench).**
Ranking-changing potential, very low effort, independent of every other lane.
Carried unchanged from I7 §7 rank 4 and still not done.

**Not rankable / should stop:**
- **Any byte-axis attack on sqlite (−42.5%), log (−37.1%), repeat.jsonl
  (−81.4%).** No current mechanism is within a factor of two of these bars and
  the decode axis is closed there (10–104×). Recording as closed until a
  mechanism clears the novelty gate with a sized claim.
- **Any further work whose only payoff would be an encode-plane knee row.** The
  five existing rows prove encode-only knees are obtainable and worthless.
- **Chasing brotli-q1/q4 ratio beats.** Audit trap (d). Already visible in the
  data (§1.2, three timeseries rows beat q1) and not citable.

---

## 5. Mechanism retirement ledger (math vs implementation-era)

Retirement reason must be one of the two, per the working agreements.

| mechanism | status | reason class | evidence |
|---|---|---|---|
| Post-parse rollback as EXP-X recovery | RETIRED | **math** | oracle F1: regression is trajectory-borne; every committed token individually sound |
| Stream-budget-only route to 2× decode | RETIRED | **math** | t3 profile: CRC 44% + mat 26% dominate; opcode pulls 0.3% |
| Implicit Δ=σ·(d/4) as default ARI form | RETIRED | **math** | EXP-Z(b): +38.35% vs transmitted |
| RLZ/RePair book-stream as default | RETIRED | **implementation-era** | Exp-Y: decode −8.1..−15.9%, encode 48–64×. Ratio-only point |
| Word-aligned constant-Δ ARI on misaligned layouts | RETIRED | **math** | EXP-Z(a) timeseries +0.03%; columnar-align 0 tokens |
| Lane/field transposition applied globally before LZ | RETIRED | **math** | every tested ELF section grew |
| Transmitted-Δ TCOPY submode | RETIRED | **math** | made .eh_frame AND .rodata larger |
| Context clustering as novelty | RETIRED | **prior art** | RFC 7932 §7 context map |
| **Encode-plane knee rows as an objective** | **NEW RETIREMENT** | **math** | AUDIT-1b: 5 exist; all single-plane, all decode-dominated, all front-knee artifacts |
| **Decode-plane rows on ratio≈1.000 files** | **NEW RETIREMENT** | **math** | AUDIT-5: degenerate, store-vs-fast-store |

---

## 6. The Iteration-9 plan (ranked, pre-sized, sequenced)

Cells closest (this is the living artifact — patched as peers land):

1. **synth-timeseries.bin** — −14.3% bytes **or** 1.78× decode. Closest on both axes.
2. **generated.json** — −11.3% bytes **or** 4.50× decode (three-leg fix).
3. **synth-arith.bin** — −66.0% bytes; decode route degenerate, byte route only.
4. **src.cpp** — −15.7% bytes; decode closed (40.6×).
5. Everything else — ≥−17% bytes, decode closed.

### Pre-registrations needed (each blocks its own implementation)

- **PR-1 — synth-timeseries decode profile.** Falsifiable: measure the actual
  time-share decomposition (CRC / materialization / alloc / token) on
  synth-timeseries and generated.json at mode-15. Pre-committed branch: if the
  timeseries CRC share is <40%, Rank 1 dies and generated.json becomes the sole
  decode lead. **Owner: decode-perf.** Highest priority; everything keys off it.
- **PR-2 — degenerate-escape guard.** A retirement rule, not an experiment: any
  EXTENDS_FRONT row **whose own ratio** is ≥0.95 is reported as DEGENERATE.
  **Owner: research-gate**, seconded by me. `[AMENDED v4 — the test applies to
  the ROW, not to the file's current best row. My original wording would have
  vetoed the legitimate mode-16 crossing; arch caught this (§2.7a).]`
- **PR-3 — FRONT-GAP vs FRONT-CROSSING token.** Resolves the ambiguity that let
  five knee rows be mistaken for a crossing. **Owner: research-gate** (coordinator
  has already backed this).
- **PR-4 — measurement-window attestation.** Every published throughput number
  carries the host-load state at measure time. **Owner: bench.** Without this,
  the 0.6%-margin timeseries call is not decidable (AUDIT-6).

### Sequencing

```
P0 [bench]    re-run tools/pareto_front.py on CURRENT bytes; confirm/reject the
              5-row re-derivation. CHEAP, BLOCKING — every downstream number
              keys off the count. (Coordinator directive, already issued.)
P1 [decode-perf] PR-1: synth-timeseries + generated.json decode profiles  ──┐
P2 [arch]     CRC-only leg; byte-identity gate; staged attribution         │
              ├── if PR-1 says timeseries CRC ≥40% → land the timeseries   │
              │     crossing attempt FIRST (Rank 1)                        │
              └── else → generated.json three-leg (Rank 2)                 │
P3 [arch]     materialization leg, then alloc leg (staged, byte-identical) │
P4 [bench]    arbiter on each landed leg; verify any EXTENDS_FRONT twice   │
              in separate process invocations (coordinator directive)      │
P5 [pnra]     piecewise-linear invariant prototype → synth-arith bytes       │
              (PARALLEL to P2/P3: different file, no src/anvil.cpp           │
              contention until integration; largest headroom on the board)   │
P6 [arch]     mode-16 ARI-REF container (Rank 4) — behind P2/P3 on the      │
              serial lane; AUDIT-5 degeneracy check + compare vs pnra ceiling│
E  [bench]    reference rows on the 9 newer corpus cells (anytime)          │
F  [research-gate] PR-2/PR-3 rulings (anytime, blocks no code)              │
G  [strategy] patch this doc after every landed measurement  ◄──────────────┘
```

Serial-lane note: arch owns `src/anvil.cpp` solely, so P2→P3→P6 is serial.
P5 (pnra) is parallel — different file, prototype-only until integration.
Build contention: only arch and bench use `./build`; all other lanes use
`build-i8-<lane>` (coordinator advisory). My lane builds nothing.

---

## 7. Endgame accounting (rewritten at iteration end)

**Status at this patch: ITERATION IN PROGRESS.** Nothing below is final.

| quantity | value | source |
|---|---|---|
| Arbiter runs this iteration (I8) | **PENDING** | bench owns the tool |
| Verdict rows produced (I8) | **PENDING** | — |
| EXTENDS_FRONT rows, all history | **5** | `tests/pareto-baseline.csv` @ HEAD |
| EXTENDS_FRONT rows, I8 | **PENDING** | — |
| Strong crossings (both planes) | **0** | my §1.2 test, recomputed |
| Per-file anvil row-plane cells | **468** | recomputed |
| Dominated (per-file) | **463** | recomputed |
| AGGREGATE cells / dominated | 36 / 36 | recomputed |
| Combined dominated | **499 / 504** | recomputed |
| Of the 5 non-dominated: FRONT-GAP FILL | **5** | bracket test, §1.3 |
| Of the 5 non-dominated: FRONT-CROSSING | **0** | bracket test, §1.3 |

**Mandatory co-listed tuple (research-gate ruling, binding):**
`5 non-dominated | 5 FRONT-GAP | 0 FRONT-CROSSING | 463/468 dominated`
(499/504 including AGGREGATE). Any one figure quoted alone is misleading.

**Corrections accepted into this tally:** the coordinator's "0 EXTENDS_FRONT"
(AUDIT-1) and **my own "460 of 465"** (§1.2 erratum) — the same failure mode,
two authors, one iteration apart. The recompute rule binds all of us, me
included.

**Pre-committed endgame rule.** If I8 ends with no new EXTENDS_FRONT row, the
failure must be classified as one of:
*(forms below are corrected per research-gate/PR-2/PR-3 — never "still zero",
never "0 to 5" without the knee-artifact caveat in the same breath)*
- **math-failure** — the reachable decode band is provably below the required
  lift on every live cell (evidence: PR-1 profile + §2.2 table), or the required
  byte reduction exceeds what the surviving mechanisms can deliver; or
- **implementation-era-failure** — the band is reachable in the profile's
  arithmetic but the legs did not land this iteration (evidence: which leg,
  which commit, what it measured).

I will **not** write "still zero." The five rows exist. If I8 adds none, the
honest sentence is: "I8 produced no new EXTENDS_FRONT row; the five that exist
remain single-plane, encode-only, decode-dominated front-knee artifacts; 0
strong crossings in project history."

---

## 8. Sources grounded

- `tests/benchmark-suite.csv`, `tests/benchmark-summary.csv`,
  `tests/pareto-baseline.csv`, `tests/pareto-verdict.csv`,
  `tests/noise-floor.csv` — all at HEAD `fc23d9a`, all recomputed by me.
- `tools/pareto_front.py` lines 28–41 (arbiter predicate; re-implemented, not run).
- `deliverable/t3-decode-floor-profile` (decode-perf) — t3 time shares, CRC 1.79× cap.
- `docs/swarm-i8-brief.md`, `docs/i8-streak-correction.md`,
  `docs/swarm-i7-strategy.md` §1/§3/§7, `docs/swarm-i6-strategy.md` §6,
  `RESEARCH_LEDGER.md`, `docs/CONTEXT.md` (Linux numbers — AUDIT-3, not citable).
- Coordinator directives `msg_198582c6`, `msg_1ebd53f9`, `msg_1682d935`.

**Standing disclaimer.** None of this is a claim. Every ranked item is a
pre-registrable, falsifiable test with a named arbiter. Every multi-leg decode
band is `[PROJECTION]` arithmetic on decode-perf's landed t3 shares, not a
measurement.
