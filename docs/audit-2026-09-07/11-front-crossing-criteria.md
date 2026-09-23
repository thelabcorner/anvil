# 11 — Front-Crossing Criteria (Operational)

> **Why this document exists.** Project rule (01 §5.1): a raw `EXTENDS_FRONT` flag
> or a one-plane gap is **not** a Pareto/frontier win. This document makes the
> FRONT-CROSSING decision operational and then applies it to the BWT/auto-routing
> evidence so the verdict is computed, not argued. Every criterion below is stated
> so a peer can reproduce the classification from `tests/*.csv` and a named ANVIL
> command. **No claim in this document is a result until the cited measurement
> exists.** Derived oracle/landscape numbers are labelled and quarantined from the
> verdict.

---

## 1. Reference grid (what "the frontier" is measured against)

A claim is only meaningful relative to a fixed grid. The grid for ANVIL's
general-purpose standing is:

| axis | reference configurations |
|---|---|
| **bytes** | Brotli q11/lw30 (clang-cl, lw30) and **xz -9e** as the binding bar. Host bar = **xz -9e = 48,456,100 B** (Silesia), **24,831,656 B** (enwik8). zstd u22/l27 is **context only, NOT on the grid** — it wins zero Silesia files (beaten by both Brotli and xz on all 12); including a dominated point would manufacture false non-dominance (the FRONT-GAP artifact of 01 §5.1). |
| **encode time** | the reference codec's own wall time, measured median ≥3 reps under the SAME build as ANVIL (after E1 normalization). |
| **decode time** | same. |
| **peak memory (encode/decode)** | `peak_wset` as recorded by `tools/bench_ratio.py` (VALID_100MS_NONRECURSIVE cells). |
| **decompressor size** | zipped decoder-only binary (`anvil_decode`, P0.3) — NOT the full encoder executable. |

**Grid discipline rules:**
1. The grid is the *local fresh* build of each reference, not published historical
   tables (02 §2). Published numbers orient; local builds decide direct claims.
2. ANVIL and every reference in a compared cell must share **one compiler/toolchain
   + flags + CPU-affinity/power state** (01 §4, E1). Cross-build timing is rejected
   as evidence (06 §E2).
3. The grid is computed **per file and aggregate**. A per-file non-domination is not
   an aggregate frontier crossing; an aggregate win that is decode-dominated is a
   FRONT-GAP, not a crossing (see §3).
4. xz -9e is the *binding* bar. Beating Brotli alone is necessary, not sufficient
   (coordinator standing order). State explicitly whether any ANVIL total beats
   48,456,100 B.

---

## 2. Dominance definition (all axes)

A candidate point `A` (an ANVIL configuration) **dominates** the grid iff on every
axis it is ≤ the best reference on that axis *within the decision-relevant margin*,
and strictly better on at least one axis:

```
dominates(A) :=
    bytes(A)   <= min_ref bytes        (strict improvement required somewhere)
 AND encode(A) <= min_ref encode * M_t
 AND decode(A) <= min_ref decode * M_t
 AND peakmem(A)<= min_ref peakmem * M_m
 AND decsize(A)<= min_ref decsize * M_s
 AND (bytes(A) < min_ref bytes   OR   decode(A) < min_ref decode   OR ...)
```

**Margins required (must be stated, not assumed):**

| axis | required margin `M` | rationale |
|---|---|---|
| bytes | **strictly <** best reference; report Δ exactly. For an *aggregate* crossing, require the aggregate to beat xz by ≥ the framing/measurement noise floor (suggest ≥ 0.05% ≈ 24 KB on Silesia) so it is not a roundtrip/alignment artifact. | bytes is the primary axis; small deltas are noise. |
| encode | ≤ 3× reference as a *weak* claim; true crossing wants ≤ reference. ANVIL's BWT encode is far slower, so encode is almost never the winning axis — treat as a cost to report, not a claim. | ANVIL is not an encode-speed project yet. |
| decode | **≤ reference × 2** to even be called "acceptable"; ≤ reference for a clean crossing. Decode is the current blocking axis (see §3). | BWT decode is **11.2× Brotli / 6.0× xz** over the 7 BWT-routed files (mixed-build, provisional until E1); this must be bounded. |
| peak mem | ≤ reference × 2 for a plausible deployment claim. | BWT peaks hundreds of MiB (04 §5). |
| dec size | ≤ reference class; report zipped decoder. | Multi-backend tax (04 §9). |

A point that is non-dominated on bytes but fails decode/peakmem margin is
**FRONT-GAP (cost)**, not FRONT-CROSSING.

---

## 3. Classification: FRONT-GAP vs FRONT-CROSSING vs DEGENERATE

| class | definition | decision |
|---|---|---|
| **FRONT-CROSSING** | non-dominated on bytes AND within margin on decode, peak-mem, dec-size; strictly better on ≥1 axis vs the full grid; measured end-to-end (not oracle). | Supports "crossed the frontier." Fund next. |
| **FRONT-GAP** | non-dominated on bytes but only because the grid leaves a rectangle uncovered, OR non-dominated on bytes but violates a cost margin (decode/peakmem/decsize). | Report as "beats xz on bytes, loses on <axis>." Do NOT call a frontier win. Close the gap or state it as a known cost. |
| **DEGENERATE** | ratio ≥ 0.95 of raw, or the "win" is an artifact of missing grid cells / unmeasured axis. | Not frontier evidence. |

**Procedure (apply in this order):**
1. Is the number *measured* (exact roundtrip, named command) or *derived* (oracle)?
   Derived → not eligible for any crossing claim; label as target.
2. Does it beat xz -9e on bytes (the binding bar)? If no → it is at best a Brotli-class
   result; do not call a crossing.
3. On the cost axes, is it within margin? If bytes win but decode/peakmem fail →
   FRONT-GAP (cost).
4. Is the grid complete on all axes cited? If a claimed axis was never measured →
   DEGENERATE/insufficient, not a win.

---

## 4. Application to current evidence

### 4.1 Per-file BWT direct vs xz -9e (measured, but single-backend)

BWT direct beats xz on 7 files (webster −1,051,319; x-ray −473,952; mr −369,578;
osdb −259,907; dickens −259,347; reymont −171,568; nci −83,568) — **measured, exact
roundtrip, from `tests/bwt-backend-standard.csv`**. On bytes this is a real per-file
result vs the binding bar. **Classification: FRONT-GAP**, because:
- it is single-backend, not the routed portfolio (the actual product);
- decode margin is violated: over the 7 BWT-routed files BWT decode = 13.0 MB/s vs
   xz 78.7 MB/s (6.0×) and Brotli 146.3 MB/s (11.2×) (mixed build, directional — even
   allowing the confound, an 11× gap is far outside the ×2 margin of §2);
- peak memory violated (webster BWT decode 248 MiB vs xz 78 MiB ≈ 3×).
Positive bytes signal, but not a crossing until decode/peakmem are normalized and
shown acceptable or amortized.

### 4.2 Whole-file `--ratio-backend=auto` (MEASURED — bench-normalize E2, verdict APPLIED)

This is the only eligible aggregate crossing candidate. Applying the verdict
template (E1-normalized build, pinned clang-cl 22.1.8):

```
measured auto total Silesia = 46,446,995 B  (< 48,456,100 B xz, roundtrip OK)
    bytes axis: NON-DOMINATED vs grid  -> passes §3 step 2
    decode(aggregate) = ~6.7 MB/s BWT vs xz 78.7 MB/s -> 11.2x OVER margin
    peakmem = ~10.3 GiB flat -> OVER margin (vs xz)
       NB: root-caused to Brotli q11/lgwin30 (1 GiB window), NOT an ANVIL defect.
       The reference helper build/brotli_lw.exe peaks 10,273 MiB identically, so
       this is NOT a relative disadvantage vs the Brotli bar - only vs xz
       (~100-570 MiB). Still fails this axis; not fixable in ANVIL code.
       AUTO-SPECIFIC: --ratio-backend=auto runs a Brotli candidate encode on EVERY
       file (incl. the 7 that route to BWT) to pick the winner, so peak ~10.3 GiB
       on all inputs; --ratio-backend=bwt keeps peak 68-718 MiB.
    -> FRONT-GAP (cost): "beats xz by 2,009,105 B on bytes,
       but decode 11.2x / peakmem ~110x over margin"
enwik8 auto = 23,534,368 B  (< 24,831,656 B xz, roundtrip OK)
    -> same FRONT-GAP classification (decode ~11.2x over margin)
```

**Net:** a real, reproducible **byte win vs xz on both canonical corpora** — i.e. ANVIL
*beat the binding bar on the bytes axis* — but it is a **decode + peakmem FRONT-GAP**,
not a frontier crossing, and never an unqualified Pareto win. Report as "bytes beat the
binding bar, decode/peakmem fail," not as a crossing.
Measured is +159 B over the 46,446,836 oracle (framing only); the oracle was
*validated*, not beaten. See 12 §1 / §2.5.

**Challenge to the team (will be enforced):** the oracle `min(Brotli,BWT)` =
46,446,836 B beats xz by 2,009,264 B, but that is **derived + excludes rev-2
framing + excludes the cost axes**. A measured auto total near 46.5 MB that still
loses on decode by ~3× vs xz is a **FRONT-GAP (cost)**, not a crossing. Do not let
a bytes-only number be read as "ANVIL crossed the frontier."

### 4.3 The 3-way landscape number (NOT a claim)

`oracle_min(Brotli,BWT,xz)` = 45,782,529 B is landscape only (xz not an ANVIL
backend). It is **ineligible** for any ANVIL frontier claim. It is recorded here
solely to size the missing-family gap (10 §3, M1).

### 4.4 enwik8 BWT / auto (MEASURED — bench-normalize E7, verdict APPLIED)

Measured enwik8 auto = BWT direct = **23,534,368 B** (router chose BWT; independently
verified by bwt-theory E7 and lead from `tests/enwik8-bwt.csv`). Bar xz = 24,831,656 B
→ **−1,297,288 B**; also beats Brotli 24,810,180 B by 1,275,812 B. The previously-missing
enwik8 result is **filled and cross-corpus-confirmed**: the Silesia byte win replicates on
enwik8. Same FRONT-GAP classification as §4.2 (decode ~6.7 MB/s = 11.2× over margin). enwik8
is **in** the frontier discussion as of E7; combined Silesia+enwik8 auto = 69,981,363 B.

---

## 5. Standing challenge list (portfolio-strategy → team)

1. **Auto beats xz, not just Brotli?** When bench-normalize reports E2, the first
   question is Δ vs 48,456,100, not Δ vs 49,383,136.
2. **Decode regression — CLOSED SUB-QUESTION: region-scoped BWT routing does NOT
   amortize it.** (`findings/decode-amortization-deadend`, lead.) Aggregate decode is
   the harmonic mix `1/(f/13.0 + (1−f)/146.3)` MB/s, f = fraction of bytes BWT-routed.
   To merely *match* xz decode (78.7 MB/s) requires **f ≤ 8.38%**. But the BWT-favorable
   files are **56.79%** of Silesia (120,362,284 of 211,938,580 B) — the byte win lives
   in most of the corpus, not a small region. At actual f the implied aggregate is
   **21.4 MB/s = 3.7× slower than xz**; matching xz would forfeit ~6.8× of BWT-routed
   bytes and nearly the whole 2,009,264 B win. **The byte win and the decode win are in
   direct structural conflict (~7× over budget) — not a tuning problem.** This closes
   the "route BWT only to high-savings regions" escape hatch. Three real options remain:
   - **(a) Accept FRONT-GAP-on-decode** and state it honestly (correct for now);
   - **(b) Make the BWT inverse itself faster** (13 → ~50+ MB/s). This is an
     **engineering** problem (postcoder / inverse-BWT throughput), NOT a routing problem,
     and is the only path that keeps the byte win;
   - **(c) Get the byte win from a backend that decodes fast.** CM (P3.1) would likely
     decode *slower* than BWT, worsening the axis; **DEFLATE reconstruction (P4.1) targets
     mozilla/sao/ooffice/samba — all Brotli/xz-routed with fast decode — so its gains cost
     NO decode regression** (see 07 re-rank: P4.1 now ranks above P3.1).
   Rates are mixed-build provisional (01 §4) pending E1, but 13 vs 78.7 MB/s cannot be
   compiler noise, so the conclusion is directionally safe.
3. **Framing honesty.** The measured auto total must include current rev-2 framing.
   If it is materially above the 46,446,836 oracle, inspect candidate framing/
   selection before claiming any win (08 E2 stop condition).
4. **No oracle as result.** Any peer who reports 46.4 MB / 45.8 MB as "ANVIL" will
   be corrected: those are targets; only the measured auto total is a result.
