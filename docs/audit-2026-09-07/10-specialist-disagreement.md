# 10 — Specialist Disagreement Map (O3)

> **Instruments, not labels.** Backend disagreement is a scientific instrument
> (09 §O3). The seven BWT wins and five Brotli wins over Silesia are not routing
> labels; they are a dataset about *what redundancy each family can see*. This
> document studies the **difference vector**, not just the winner, and uses it
> to locate **missing redundancy families** and to test the project's central
> reframing: *is ANVIL's advantage an adaptive portfolio architecture rather than
> one universal compressor?*

All byte counts in §1–§3 are **measured**, exact, from `tests/ratio-first-standard.csv`
(Brotli q11/lw30, xz -9e, zstd u22/l27, anvil-ratio) and `tests/bwt-backend-standard.csv`
(anvil-bwt-direct). Per-cell decode seconds are measured from the same CSVs (mixed
compiler build — see decode caveat in §4 and 11 §3). Oracle/landscape totals in §3
are **derived** from those measured cells; they are research targets, never results.

---

## 1. The 4-way Silesia table (measured)

Silesia canonical total: **211,938,580 bytes**.

| file | input | Brotli q11/lw30 | anvil-bwt-direct | xz -9e | zstd u22/l27 | min(Brotli,BWT,xz) | winner* |
|---|---:|---:|---:|---:|---:|---:|---|
| dickens | 10,192,446 | 2,827,779 | **2,571,873** | 2,831,220 | 2,849,381 | 2,571,873 | BWT |
| mozilla | 51,220,480 | 13,806,141 | 17,823,651 | **13,376,248** | 14,967,572 | 13,376,248 | xz |
| mr | 9,970,564 | 2,823,137 | **2,382,322** | 2,751,900 | 3,105,643 | 2,382,322 | BWT |
| nci | 33,553,445 | 1,497,411 | **1,365,712** | 1,449,280 | 1,610,427 | 1,365,712 | BWT |
| ooffice | 6,152,192 | 2,478,857 | 2,868,832 | **2,427,232** | 2,598,777 | 2,427,232 | xz |
| osdb | 10,085,684 | 2,816,279 | **2,584,657** | 2,844,564 | 3,098,444 | 2,584,657 | BWT |
| reymont | 6,627,202 | 1,332,159 | **1,144,032** | 1,315,600 | 1,347,556 | 1,144,032 | BWT |
| samba | 21,606,400 | 3,761,899 | 4,398,658 | **3,739,532** | 3,876,634 | 3,739,532 | xz |
| sao | 7,251,944 | 4,586,094 | 5,117,977 | **4,425,672** | 5,000,515 | 4,425,672 | xz |
| webster | 41,458,703 | 8,340,058 | **7,317,361** | 8,368,680 | 8,458,469 | 7,317,361 | BWT |
| x-ray | 8,474,240 | 4,682,754 | **4,017,320** | 4,491,272 | 5,155,752 | 4,017,320 | BWT |
| xml | 5,345,280 | **430,568** | 437,790 | 434,900 | 453,173 | 430,568 | Brotli |
| **total** | **211,938,580** | **49,383,136** | **52,030,185** | **48,456,100** | **52,522,343** | **45,782,529** | — |

\* `winner` = min over {Brotli, BWT, xz} only (xz is in the table as the
reference bar, not as an ANVIL backend). zstd is shown for landscape completeness.

---

## 2. Difference-vector classification

For each file we classify by the **BWT−Brotli** and **BWT−xz** deltas, since those
are the two ANVIL-relevant backends and the project's reference bar.

| file | Δ BWT−Brotli | Δ BWT−xz | class | interpretation |
|---|---:|---:|---|---|
| dickens | −255,906 | −259,347 | **BWT≪Brotli** | sortable homogeneous contexts |
| mr | −440,815 | −369,578 | **BWT≪Brotli** | repetitive text; BWT locality wins |
| nci | −131,699 | −83,568 | **BWT≪Brotli** | DNA (4-symbol, long-range order) |
| osdb | −231,622 | −259,907 | **BWT≪Brotli** | structured record text |
| reymont | −188,127 | −171,568 | **BWT≪Brotli** | repetitive structured text |
| webster | −1,022,697 | −1,051,319 | **BWT≪Brotli** | large homogeneous text corpus |
| x-ray | −665,434 | −473,952 | **BWT≪Brotli** | 8-bit grayscale stream (sortable) |
| mozilla | +4,017,510 | +4,447,403 | **Brotli/xz≪BWT** | mixed/archive-heavy, hostile to BWT |
| samba | +636,759 | +659,126 | **Brotli/xz≪BWT** | mixed archive |
| ooffice | +389,975 | +441,600 | **Brotli/xz≪BWT** | archive (OLE) |
| sao | +531,883 | +692,305 | **Brotli/xz≪BWT** | float/unsortable numeric |
| xml | +7,222 | +2,890 | **≈equal** | tiny file; warmup dominates |

**Two derived findings (from `lead`, programmatic recompute of the table):**

1. **zstd u22/l27 wins zero Silesia files.** All 12 cells: zstd is beaten by both
   Brotli and xz on every file. Under min(Brotli,BWT,xz) the winners are BWT 7 /
   xz 4 / Brotli 1 / zstd 0. zstd is therefore *not a ratio bar* on Silesia — keep
   it for speed/landscape context only. In docs/11 the bytes reference grid is xz
   -9e = 48,456,100 B plus Brotli for contrast; zstd is demoted to context so the
   grid does not include a dominated point (which would manufacture false
   non-dominance — exactly the FRONT-GAP artifact of 01 §5.1).
2. **The 3rd-backend headroom is highly concentrated.** The 664,307 B gap between the
   2-backend oracle and the 3-way landscape breaks down as: mozilla 429,893 B
   (64.7%), sao 160,422 B (24.2%), ooffice 51,625 B (7.8%), samba 22,367 B (3.4%);
   every other file = 0. **mozilla alone is ~65% of it.** This is a sharp design
   constraint: the next backend should be chosen for *mozilla-class archive content
   specifically*, and it gives P4.1 (DEFLATE reconstruction) a concrete go/stop bar —
   ~430 KB on one file. A mozilla-specific mechanism that cannot plausibly recover
   >430 KB is not worth building.

**Key positive finding (verified independently by `lead`, blackboard
`findings/oracle-verified-3way`):** BWT beats **xz -9e** on **all 7** of its
winning files, not merely a handicapped Brotli. The BWT advantage is therefore a
*real compressor property*, not an artifact of Brotli being a weak contrast.
Per-file BWT vs xz:

```
webster  −1,051,319   x-ray  −473,952   mr     −369,578
osdb     −259,907     dickens −259,347  reymont −171,568
nci      −83,568
```

This matters for the whole project posture: on the BWT-favorable regime, ANVIL's
BWT backend is *already ahead of the reference bar the project is measured against*,
before any postcoder tuning (P1.3) or router (P2.1).

---

## 3. Oracle / landscape totals — LABELLED, not conflated

These are computed from the measured cells above.

- **`oracle_min(Brotli,BWT)` = 46,446,836 B** — the **achievable ANVIL target**
  once `--ratio-backend=auto` is measured and current rev-2 framing is added.
  = −2,009,264 B vs xz -9e (48,456,100). Use exact integer: 2,009,264 B = 1.916 MiB.
- **`oracle_min(Brotli,BWT,xz)` = 45,782,529 B** — **LANDSCAPE ONLY**. xz is not
  an ANVIL backend; this is the unachievable lower bound if ANVIL could also pick
  xz. = −2,673,571 B vs xz. **Not a target, not a result.**
- **Gap between them = 664,307 B**, and it is *concentrated entirely* in the four
  archive-heavy BWT-hostile files: mozilla 429,893 + sao 160,422 + ooffice 51,625
  + samba 22,367 = 664,307. xml is Brotli in both.

This 664,307 B gap is the **quantitative backbone of the complementarity thesis**
(07 §Portfolio interactions, 09 §O1): the redundancy ANVIL's two backends cannot
capture overlaps almost exactly with what xz captures on `mozilla`/`samba`/
`ooffice`/`sao`. Those are precisely the files where **DEFLATE reconstruction (P4.1)**
and a **CM backend (P3.1)** are hypothesized to pay. The disagreement map *predicts*
where the next backend should be deployed.

---

## 4. Missing redundancy families (the point of O3)

The difference vector localizes gaps. Each gap is a candidate family, not a
vague "BWT could be better."

### M1. Embedded-DEFLATE reconstruction (HIGH, concentrated)
`mozilla`/`ooffice`/`samba` lose to BWT by hundreds of KiB–MB and are *won by xz*,
which has stronger deflate/preflate-class reconstruction. ANVIL currently stores
these regions as opaque Brotli or, worse, as catastrophic whole-file BWT. No ANVIL
path yet parses embedded DEFLATE. **This is the single largest quantified missing
family: ~0.66 MB of the gap to the landscape bar sits here.** Maps to P4.1.

### M2. Float / unsortable numeric (MEDIUM, small)
`sao` (floating-point scientific data) is the one BWT-hostile file that is NOT an
archive. BWT's sort destroys float locality; LZ (Brotli/xz) exploits repeated
value patterns better. No transform or backend currently targets float records.
Small headroom (~0.16 MB to landscape) but a distinct family worth a stride/delta
probe (P4.2) rather than a new backend.

### M3. BWT decode speed (COST axis, not redundancy)
BWT's weakness here is not bytes — it is throughput. Summing the measured
per-backend decode cells over the **7 BWT-routed files** (dickens, mr, nci, osdb,
reymont, webster, x-ray; 120,362,284 B input; source:
`tests/bwt-backend-standard.csv` + `tests/ratio-first-standard.csv`):

| codec | decode sum | MB/s |
|---|---:|---:|
| ANVIL BWT | **9.248 s** | **13.0** |
| xz -9e | **1.529 s** | **78.7** |
| Brotli q11/lw30 | **0.823 s** | **146.3** |

BWT decode is **11.2× slower than Brotli and 6.0× slower than xz**. Per-file:
webster 3.517 / 0.405 / 0.204 s, nci 1.908 / 0.103 / 0.104 s, dickens 0.906 /
0.204 / 0.103 s, osdb 0.905 / 0.204 / 0.103 s, mr 0.805 / 0.205 / 0.103 s,
x-ray 0.704 / 0.306 / 0.103 s, reymont 0.503 / 0.103 / 0.103 s (BWT/xz/Brotli).

> **Provenance caveat:** these come from the mixed-build confound (BWT measured on
> the MSVC recovery build, Brotli/xz references on clang-cl). Throughput across
> build groups is **directional only** (01 §4). The 6× gap is far too large to be
> compiler noise, so the qualitative conclusion holds, but the exact figures are
> **not binding until bench-normalize's E1 rebuild**.

So a routed portfolio that wins bytes is **decode-dominated**. This is not a
missing *redundancy* family; it is a missing *cost* family and must be classified
as a cost regression (see 11 §3), not argued away as a ratio win.

### M4. Block-local heterogeneity (MEASURED — NEGATIVE, lane closed)

Originally hypothesized as a "HIGH potential" missing family: that `mozilla`/`samba`/`ooffice`
contain text regions where BWT would win inside an archive that as a whole loses to BWT, and that
a block-level oracle (E4) could recover part of M1 without full DEFLATE reconstruction.

**Measured (`findings/e4-block-routing-negative`, `deliverable/E4`): this hypothesis is FALSIFIED.**
Block-local routing is **worse than whole-file on every configuration** (aggregate +1,653,661 B at
4 MiB / +272,636 B at 16 MiB; go-bar >250 KiB not met). Cause: per-block **model warmup** tax, not
framing. Decisive proof — `webster` BWT wins **10/10** 4 MiB blocks yet Σ-blocks BWT (8,211,644) >
whole-file (7,317,329) by +894,315 B (+12.2%); Brotli the same way. And `mozilla`/`samba`/`ooffice`/`sao`
have **ZERO BWT-winning blocks** at every size — those files are BWT-hostile *throughout*, not merely
on average.

**Consequence:** M4 is not a recoverable family; the only route into those files is M1 (DEFLATE
reconstruction, P4.1), which is exactly why P4.1 is now ranked #1 (06 §G3, 07 §P4.1). E5 (byte-regret
router) is not runnable — closed-by-negative. Reopen only on a near-zero-warmup backend or cross-block
model carryover.

### What is NOT missing
- A universal context model: the BWT and Brotli wins already cover the
  homogeneous-text and mixed/LZ regimes respectively. The gap is *complementary*,
  not a single missing general model.
- Transform wins: `mr` ctx1 is the only transform win and BWT direct beats it;
  transforms are not where the frontier moves (E3 pending to confirm on BWT).

---

## 5. Verdict on the central reframing (O1)

> *"ANVIL's path may be a rigorously routed portfolio of complementary reversible
> representations and backends, not one monolithic compressor."* (09 §Part III)

The measured disagreement map **supports the premise, with one hard caveat**:

1. **Complementarity is real and quantified.** BWT and Brotli/xz win on disjoint
   file regimes (7 vs 4+1), and the residual gap to the landscape bar is exactly
   the archive-heavy regime a third specialist (DEFLATE-reconstruction / CM) would
   address. This is a *set-cover over failure modes*, the strongest form of the
   portfolio argument (07 §Portfolio interactions).
2. **The portfolio is not yet a measured result.** `oracle_min(Brotli,BWT)` is
   derived; the actual `--ratio-backend=auto` number (bench-normalize, E2) is the
   only thing that proves end-to-end selection + framing. Until that is measured
   and beats xz on **bytes** (bar = 48,456,100), the portfolio is a hypothesis.
3. **The portfolio loses on the decode axis by 11.2× vs Brotli / 6.0× vs xz** on the
    7 BWT-routed files (summed measured per-cell decode, pinned clang-cl build — E1 is
    now DONE, so this is no longer a compiler-confound artifact). A multi-axis frontier
    verdict (11 §3–§4) therefore classifies the bytes win as **FRONT-GAP (cost)** — it
    *beat the binding bar on the bytes axis* on both corpora, but it is **not** a frontier
    crossing because decode (11.2× over margin) and peak-mem (~10.3 GiB, Brotli q11/lgwin=30 —
    not an ANVIL defect, see 12 §4 risk 6) fail. Routing
    BWT only to regions where its decode cost is amortized does **not** rescue it
    (`findings/decode-amortization-deadend`); the only remaining routes are a faster BWT
    inverse or a fast-decode backend (P4.1).

**Bottom line:** the disagreement map is positive evidence the BWT lane is real
(it beats the *xz* bar on 7 files) and that the portfolio thesis is the right
architecture to fund. The map is now backed by a *measured* byte win (E2: Silesia
46,446,995 B / enwik8 23,534,368 B, both beating xz) but that win is a **FRONT-GAP**
— decode cost (11.2× slower than Brotli) and peak memory (~10.3 GiB flat; root-caused to
Brotli q11/`lgwin=30`, which the reference helper pays identically — a cost vs xz only,
not an ANVIL defect) are open
axes. The map simultaneously tells us *where to spend next*: M1 (DEFLATE reconstruction,
P4.1) is the highest-EV missing family **and is now ranked #1** — it is the only byte
win that costs no decode regression, and E4 proves you cannot recover M4's files by
block carving (M4 is a falsified hypothesis, not a recoverable family). A decode-cost
fix for M3 (faster BWT inverse) is the only other route that keeps the byte win.
