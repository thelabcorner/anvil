# SYNTH-PERF — Backend / Decode-Performance Synthesis (Space Bunny Free)

**Author:** Space Bunny Free · **Date:** 2026-10-02 · **Tree:** `i10-aux-unbwt` @ `b8eae11`, dirty
**Files created:** this file only. No existing file modified; no commit/push; no local
benchmark, sweep, or fuzz run.
**Scope:** the *performance* lane across the swarm — decode throughput, peak RSS, entropy/
selector cost, encode/decode bookkeeping, and the remote queue's Amdahl leverage. Ratio-side
mechanism work is out of scope except where it changes a performance conclusion.
**Labels:** `[M]` measured with artifact · `[M+arith]` arithmetic on measured inputs ·
`[D]` derived/structural · `[P]` projection · `[X]` falsification · `[A]` assumption.

**Headline: the BWT backend's decode axis is closed by arithmetic on already-recorded
shares. The single job the queue proposed to profile it (Q4b) must be CANCELLED, not run.
The live performance surface is the token lane plus measurement integrity — not the
inverse-BWT walk.**

---

## 1. The two recorded facts that reorganize the whole performance programme

### 1.1 The BWT backend's decode cost is dominated by the *postcoder*, not the walk

Reconstructed from primary recorded shares (full arithmetic in §3):

| file | postcoder share of BWT-path decode, **post-aux** | LF-walk share, post-aux | "other" |
|---|---:|---:|---:|
| dickens | **0.680** | 0.229 | 0.091 |
| webster | **0.606** | 0.309 | 0.085 |
| enwik8 | **0.642** | 0.291 | 0.067 |

`[M+arith]` from A16's recorded `s_p`/`s_u`/`s_other` and the measured aux stage factor
`k_u` (§3.1). The walk is the **minority** stage after the aux index was adopted.

### 1.2 On the token lane, the entropy coder is a rounding error

`RESEARCH_LEDGER.md:3924-3930` (decode-perf t3, recorded): `crc32` **44%**, eager
materialization of the 7 macro streams **26%**, token loop beyond setup **11%**, **opcode
entropy pulls 0.3%**, concat/alloc/headers 15%. `[M]` The hot path is clean; the floor is
everything *around* it.

**Consequence for the queue:** any job whose prize is "make entropy pulls faster" is
attacking **0.3%** of one decode path and is dead on share grounds. The two real token-lane
levers are `crc32` (44%, **already fixed by an adopted wire-invisible PCLMUL win** — master
brief item 12, ledger A21) and **materialization/setup (26%)**, which the ledger has already
traded once: macro-masks `+21-27%` decode for `+166,823 B`, macro-resid `+18-21%` for
`+84,956 B`, and macro-dvar is *already raw* at 0.01% share so the Linux
"+19.8 KB ⇒ +0.12 GB/s" trade does **not** transfer.

---

## 2. Strongest surviving performance case per non-killed mechanism

Ordered by measured bottleneck share × achievability. "Complete cost" = bytes + cycles +
RSS + decoder code, all charged.

| # | mechanism (lane) | share it attacks `[M+arith]` | strongest surviving case | complete cost that must still be paid | Amdahl verdict |
|---|---|---|---|---|---|
| **P1** | **BWT postcoder rewrite** (05/06/18; A16 requirement `k_p` **3.86-6.60×**) | **55.9-64.2%** post-aux | **Silesia decode parity with xz-9e is reachable** (`T_new` 2.421 s vs 2.569 s xz, 5.8% margin) with the postcoder made free *given* the current aux+leg-4 state. Not Brotli parity (needs 1.270 s), not enwik8 parity (needs 0.965 s, postcoder-free leaves 1.709 s) | transmitted static tables (A16: "static-table postcoder remains unauthorized / not built" ⇒ model cost is **uncharged**), `.text`, per-block table init | the **only** lever whose solo ceiling clears the Silesia xz bar; **insufficient alone** for the 40 MB/s BWT-route bar |
| **P2** | **Lane-partitioned streams / branchless renorm / copy superinstructions** (18 L1-L4) | entropy pulls **0.3%**; materialization 26%; crc32 44% (already fixed) | **not the entropy scheduler — L0 bulk-`symtab` and materialization elimination.** Entropy legs L1/L2 are DECODE-SHORT by arithmetic | ≤ +0.5% bytes target; L3 fixed-extent copies are near-zero wire; L2 is wire-invisible | L1/L2 **killed on share** (0.3%); L0/L3 survive on the 26% materialization term |
| **P3** | **Selector cost calibration** (06/Q3, byte-only) | n/a (economics correctness) | **highest information-per-CI-minute in the queue.** `C_decode` is a length-independent constant on the default path (`src/anvil.cpp:1553`; incumbent `10/20/22/30/35/40/45` = "right ORDER, wrong GAPS") while the correctly-scaled size-proportional fit (`kBudgetNsPerByte[7]`, `src/anvil.cpp:1618`) sits behind a default-off flag whose arbiter never ran | Stage 1 is byte-only ⇒ zero timing cost | not an Amdahl item; a **ranking-correctness** item that currently propagates a mis-scaled cost into every selection decision |
| **P4** | **Already-retained wire-invisible wins** (ALLOC 1.352×; CRC PCLMUL; aux index 1.364-2.339×; leg-4 1.67-1.73×) | — | all real, all measured, **none sufficient alone** (A12 `DECODE-SHORT`; A15 leg-4 falsified as the ~1.4x route) | already charged | individually ≤1.35×; A10/A21: the wire-invisible decode programme is **exhausted** |
| **P5** | **Lane scheduler over LF-walk segments (WSI-BWT)** (05) | walk = 22.9-30.9% post-aux | **NONE — cancelled by arithmetic (§3-4).** Requires an **≥5.2-13.3×** LF-walk speedup *even with a postcoder at A16's full requirement* | 0 bytes, 0 RSS, ≤+4 KiB `.text` — the cost was never the problem | **FAIL.** Practical MLP ceiling for one dependent DRAM load per output byte is ~2-4×, and `k_u`=3.40-3.49× is already banked |
| **P6** | **Block/window parity measurement** (04/15, Q2) | n/a — fixes the *denominator* | the only job that can certify whether any byte result is admissible at all (D12: Class-A 256 KiB blocks vs whole-file reference windows, restart tax 5-17%) | timing minutes | not Amdahl; **gate value** |
| **P7** | **G5D paged dictionary** (01) | table-transmission bytes | adopt-class; family HOLD, discovery design KILL; zero G5D corpus bytes measured | table bytes + paging RSS + escape bytes | byte-side only |
| **P8** | **CAM / per-block model prior** (15) | **structurally absent on the ratio path** | engineering HOLD; novelty closed by prior art | 0 B (CAM-0) but loses independent decodability if chained | ratio mode forces one 128 MiB block per file ⇒ **no restart tax to amortise** |
| **P9** | **Aux-index density W1024 → W16** (Q9) | ≤ **3,054 B** = 0.0066% of Silesia bytes | none worth CI minutes | byte-only, no timing | **defer on share** |
| **P10** | **DEFLATE reconstruction** (08) | n/a | adopt-class, HOLD until both controls frozen; bounded 2,289-stream / 2.86 MB population | decoder code size for the replay engine | n/a |
| **P11** | **Declared-total / work-amplification probe** (19, Q8) | n/a | real: the BWT backend costs **6n RSS and ~85 ns/B regardless of compressed size** `[M+arith]` ⇒ a decompression bomb is a *backend property*, not an edge case | synthetic byte-only probe | n/a |

---

## 3. A16 reconstructed from primary recorded shares (not from any lane's prose)

### 3.1 Primary inputs `[M]` — `RESEARCH_LEDGER.md:5390-5420`

| file | `s_p` postcoder | `s_u` unbwt | `s_other = 1−s_p−s_u` | `R_base` MB/s | `k_u` aux (measured) | leg-4 `k_p` |
|---|---:|---:|---:|---:|---:|---:|
| dickens | 0.439 | 0.502 | 0.059 | 11.15 | **3.40** | 1.674 |
| webster | 0.342 | 0.610 | 0.048 | 12.46 | **3.49** | 1.733 |
| enwik8 (`s_u` **estimated**) | 0.378 | 0.582 | 0.040 | 10.74 | **3.40** (recovered below) | 1.671 |

Validity check that the reconstruction is the *same* arithmetic as A16, not a paraphrase:

```
dickens   : s_p/1.674 + s_u/3.40 + s_other = 0.26225+0.14765+0.05900 = 0.46890 → 2.1324×   (A16: 2.134× ✓)
webster   : 0.19734+0.17479+0.04800 = 0.42013                                    → 2.3798×   (A16: 2.379× ✓)
enwik8    : 0.22621+0.17118+0.04000 = 0.43739                                    → 2.2864×   (A16: 2.286× ✓)
enwik8 k_u recovery: solving 1/(0.378/1.671 + s_u/k_u + 0.040) = 2.286 gives s_u/k_u = 0.17118 ⇒ k_u = 3.40
```

**Recorded uncertainty (carried, not smoothed):** shares derive from measured splits
(`i9_bwt_wt_v2`); `s_u` for enwik8 is **estimated**; **a direct stage-level A/B was NOT
measured** — "the whole-vs-stage conversion carries that uncertainty" (A16 labels). **There
is no CI on these shares.** §4 therefore reports a sensitivity band, not a point.

### 3.2 Pre-aux vs post-aux walk share — the distinction that must not be blurred

| quantity | dickens | webster | enwik8 |
|---|---:|---:|---:|
| `s_u` = **pre-aux** measured walk share | **0.502** | **0.610** | 0.582 (est) |
| post-aux residual time `R_aux = s_p + s_u/k_u + s_other` | 0.64565 | 0.56479 | 0.58918 |
| **post-aux** walk share `= (s_u/k_u)/R_aux` | **0.2287** | **0.3095** | **0.2905** |
| post-aux postcoder share `= s_p/R_aux` | **0.6799** | **0.6055** | **0.6416** |

The pre-aux figures (50.2 / 61.0 / 58.2%) are the ones that look large enough to matter, and
they are the wrong basis for any requirement computed from a post-aux current state. Using
them against a post-aux requirement is the **first dimensional error**.

### 3.3 Dimensional check: A16's `k_p` is a *postcoder* requirement

A16's `k_p = 6.05 / 3.86 / 6.60` are the factors required of the **postcoder**, from
`k_p = s_p / (R_base/40 − s_u/k_u − s_other)`. The perf red-team's reuse of them as
**LF-walk** requirements is a category error and is rejected here. The walk's own
requirement is derived in §3.4 from `f_walk` and Amdahl, and it is a different (larger, and
in the relevant case infeasible) quantity.

### 3.4 The LF-walk-specific requirement, derived

Basis: the current shipped state is **aux + leg-4** (A16 canonical update: frozen src
`BDC90474`, "ALLOC + leg 4 retained"). Targets are the recorded ones.

**Target A — A16's 40 MB/s BWT-route bar** (`T_tgt = R_base/40`, i.e. 0.27875 of base time,
for every file):

```
f_walk/k_w + f_post + f_other  ≤  0.27875
```

| file | `f_walk`(aux+leg4) | `f_post` | `f_other` | free-walk minimum | walk requirement |
|---|---:|---:|---:|---:|---|
| dickens | 0.3149 | 0.5593 | 0.1258 | **0.6851** | **infeasible at any `k_w`** |
| webster | 0.4160 | 0.4697 | 0.1143 | **0.5839** | **infeasible at any `k_w`** |
| enwik8 | 0.3914 | 0.5172 | 0.0915 | **0.6087** | **infeasible at any `k_w`** |

Even an **infinitely fast** walk leaves 16-24 MB/s against a 40 MB/s bar. Postcoder-free
alone also fails (dickens 25.3 MB/s). **Both** stages must improve:

```
dickens : f_walk/k_w ≤ 0.27875 − 0.5593/6.05 − 0.1258 = 0.0605  ⇒  k_w ≥ 5.21×
webster : f_walk/k_w ≤ 0.27875 − 0.4697/3.86 − 0.1143 = 0.0313  ⇒  k_w ≥ 13.30×
enwik8  : f_walk/k_w ≤ 0.27875 − 0.5172/6.60 − 0.0915 = 0.0746  ⇒  k_w ≥ 5.25×
```

⇒ **the LF-walk-specific requirement is `k_w ≥ 5.2-13.3×`, and only if the postcoder is
already at A16's full 3.86-6.60×.** `[M+arith]` This is consistent with, and independent of,
A16's own recorded conclusion: *"Either read: the 40 MB/s BWT-only bar is not reachable by
the wire-invisible route."*

**Target B — Silesia portfolio decode parity with xz-9e** (2.5690 s vs current 4.2015 s):
with the derived non-BWT time `t_nb ≈ 0.549 s` `[derived, provisional]` and `t_bwt = 3.6528 s`,
the *minimum achievable* whole time with a free walk is
`0.549 + 3.6528 × 0.60384 = 2.775 s` (Silesia 2-file proxy: `f_post`+`f_other` = 0.48730+
0.11654) — **7.4% slower than xz at infinite walk speed.** Postcoder-free instead gives
`0.549 + 3.6528 × 0.51256 = 2.421 s` ⇒ **xz parity IS reachable via the postcoder, and not
via the walk.** Against **Brotli q11/lw30** (1.2700 s) every combination except "everything
free" fails by ≥1.9×. **enwik8** vs xz (0.96525 s): postcoder-free leaves 1.709 s (1.77×
short); walk+postcoder both free leaves 0.324 s.

**Why `k_w` is not reachable `[D]`:** the walk's recurrence is `p ← ISA[p] − 1`, one
*dependent* random load into a 4n table per output byte (measured working set 5.99-6.35×n).
Latency hiding is bounded by memory-level parallelism, which is bounded by outstanding misses
(≈8-16 usable), and beyond that the loop becomes bandwidth-bound at ~6 B of demand traffic per
output byte ⇒ single-core ceiling of order 2-4×. `k_u` = 3.40-3.49× is already banked; the
remaining structural headroom is smaller than that, not larger.

---

## 4. Q4 verification and **CANCEL-Q4b** recommendation

**Q4's stated threshold needs correction.** Q4 asserts "the required whole-BWT-path
acceleration is about 4.5x; infinite-speed LF walk can meet 4.5x only if non-walk share is
about ≤22% (LF-walk ≥ about 78%)". The *arithmetic inside that sentence is correct*
(`f ≥ 1 − 1/4.5 = 0.778`), but the **4.5× target is not the project's recorded target** and the
threshold is **target-relative**, not absolute:

| target | whole/BWT-path factor | required `f_walk` for an infinite-speed walk |
|---|---:|---:|
| A16 40 MB/s BWT-route bar | 1.68× (dickens) / 1.35× (webster) | 0.406 / 0.259 → **and both are infeasible once `f_post+f_other` is respected (§3.4)** |
| Silesia xz-9e parity | 1.636× whole | 0.3886 (whole basis) → **infeasible: free-walk floor is 2.775 s vs 2.569 s** |
| enwik8 xz-9e parity | 3.664× | 0.7271 → **infeasible: free-walk floor 2.166 s vs 0.965 s** |
| Silesia Brotli parity (my §11 G-B1) | 3.308× whole | 0.8043 → **infeasible by 2.2×** |

So "78%" is only the right number for one specific aggressive target, and **every** recorded
target fails on the reconstructed shares. **Q4b is answering a question that recorded data has
already answered.**

### CANCEL-Q4b — explicit recommendation

> **CANCEL Q4b (BWT stage decomposition / `f_walk` profiling).** Do not dispatch it, in any
> form, including "for completeness" or as a cheap pilot. The LF-walk-specific requirement
> derived from primary A16 shares is **`k_w ≥ 5.2-13.3×` for the least favourable recorded
> target and is arithmetically infeasible for all others**, while the reconstructed post-aux
> walk share is **0.229-0.310**. The lane-scheduler / `--bwt-lanes` prototype (my §14 HOLD)
> is therefore **KILL-ed before implementation**, and my own earlier `f_walk ≥ 0.80` gate is
> **withdrawn as superseded**: it was derived from a cross-path cost transfer
> (§13.1 of my report) that omitted the postcoder stage, which A16 shows is the majority
> stage.
>
> **Reallocate Q4b's CI minutes** to: (i) the direct stage-level A/B that A16 explicitly
> records as **not measured** — scoped narrowly as *postcoder-only vs full decode* on the
> BWT path, because the postcoder is 55.9-64.2% and is the only lever whose solo Amdahl
> ceiling clears the Silesia xz bar; and (ii) nothing else in the BWT backend.
> **No further BWT-walk work is authorized in this cycle.**

**Preserved unchanged: Q4a** (typed/frontend basis × BWT backend, **bytes-only**). It asks a
*different* question — whether "the backend is the mechanism layer" is true — and it costs no
timing minutes. It stays live and, per §5, is ranked **above** every timed job.

**Sensitivity band (no CI exists on A16's shares), for honesty about the cancellation:**

| reading | `f_walk` post-aux (Silesia proxy) | effect on the verdict |
|---|---:|---|
| stage-factor reading (`k_u` applied to the stage) | 0.294 | infeasible for every target |
| conservative whole-decode reading (measured aux 1.364× applied to whole time ⇒ 53.1% of the walk stage removed) | **0.321** | still infeasible; `f_post+f_other` alone exceeds every target |
| most walk-favourable defensible (post-aux+leg-4, webster-weighted) | **0.416** | still short: needs `k_w ≥ 13.3×` even with a postcoder at 3.86× |

The cancellation is robust across the band. **The only reading under which a walk lane could
survive is one in which the postcoder requirement is met *first*** — i.e. the postcoder is the
prerequisite, not the walk.

---

## 5. Queue ranking (Q2-Q9): information gain per CI minute × Amdahl leverage

Scoring: **IG** = cross-track information gain (how many lanes a result can kill/rebase),
**AM** = Amdahl leverage (share actually attacked), **$** = CI minutes. Rank is
`IG × AM / $`, with cheap falsifiers promoted regardless (queue rule 6).

| rank | job | IG | AM | $ | disposition |
|---|---|---|---|---|---|
| **1** | **Q0** dense-frontier retained-artifact correction | high (gates Q2 entirely) | n/a | **0** (static) | **RUN FIRST** — unchanged |
| **2** | **Q1** corpus admissibility | highest (gates *all* promotion) | n/a | **0** | **RUN** — unchanged |
| **3** | **Q3** selector arbiter, Stage 1 bytes-only | high (05/06/11/12/16/18 all consume `C_decode`; D14) | n/a (ranking correctness) | ~0 timing | **RUN Stage 1**; Stage 2 timing **only** if choices differ materially *and* the effect clears the 8% sensitivity floor |
| **4** | **Q4a** typed basis × BWT bytes-only | high (inverts/rebases 01/02/09 vs 05/18) | n/a | ~0 timing | **RUN** — preserved, promoted above all timed work |
| **5** | **Q5** MASK-CEILING (track 10) | medium (kills a whole namespace for ~0 timing) | byte-side | ~0 | **RUN** — cheap ceiling, real kill |
| **6** | **Q2** corrected dense-frontier / block-window parity pilot | high (the only admissible frontier substrate; fixes D12's 5-17% confound) | n/a — denominator | **highest $** | **RUN, but as the single timed job**; do not expand to the 20-arm vehicle |
| **7** | **Q6** G5D census-first ablation | medium (track 01 only; family HOLD) | byte-side | low | **RUN after Q3/Q4a rebasing** |
| **8** | **Q8** declared-total / work-amplification probe | medium; ties to the measured 6n/85 ns-per-byte backend property | n/a | low | **RUN as infrastructure** |
| **9** | **Q7** DEFLATE controls | low-medium (adopt-class, bounded population, HOLD) | n/a | medium | **DEFER** until both controls are frozen; not on the critical path |
| **10** | **Q9** aux W1024 vs W16 | low | **≈0** (≤3,054 B = 0.0066% of Silesia bytes; no decode effect) | low | **DEFER on share grounds.** Only revisit if it is bundled into an existing byte-only job at zero marginal cost |
| **—** | **Q4b** | — | — | — | **CANCEL (§4)** |

**Explicit small-share kills this ranking enforces:** (i) any job whose prize is entropy-pull
throughput — **0.3%** of the token path, `RESEARCH_LEDGER.md:3929`; (ii) any BWT LF-walk
speed job — walk is 22.9-30.9% post-aux *and* the requirement is 5.2-13.3×, infeasible (§4);
(iii) any job priced off the 40 MB/s BWT-route bar alone — A16 records it unreachable by the
wire-invisible route and §3.4 reproduces that from primary shares.

---

## 6. Proposed edits to `REMOTE-EXPERIMENT-QUEUE.md` (appended here; the queue file itself is not modified)

```text
## QUEUE EDITS — perf synthesis, Space Bunny Free, 2026-10-02 (proposed, not applied)

Q4b — BWT stage decomposition / f_walk   →  STATUS: CANCELLED (perf synthesis §4)
  Reason: LF-walk requirement derived from primary A16 shares is k_w >= 5.2-13.3x for the
  least favourable recorded target and is arithmetically infeasible for every other recorded
  target (40 MB/s BWT-route bar, Silesia xz parity, enwik8 xz parity, Silesia Brotli parity).
  Post-aux reconstructed walk share is 0.229-0.310; postcoder share is 0.606-0.680.
  The lane-scheduler / --bwt-lanes prototype is KILL-ed BEFORE implementation.
  Do not dispatch in any form, including "for completeness".
  The earlier "~78% LF-walk threshold" is TARGET-RELATIVE, not absolute: 78% corresponds to a
  4.5x target that is not a project target. Recorded thresholds for reference:
    40 MB/s BWT bar 1.68x/1.35x -> free-walk floors 0.685/0.584 (both INFEASIBLE)
    Silesia xz parity 1.636x    -> free-walk floor 2.775 s vs xz 2.569 s (INFEASIBLE)
    enwik8  xz parity 3.664x    -> free-walk floor 2.166 s vs xz 0.965 s (INFEASIBLE)
    Silesia Brotli parity 3.308x-> free-walk floor 2.775 s vs Brotli 1.270 s (INFEASIBLE)
  Correction of record: A16 k_p = 6.05/3.86/6.60 are POSTCODER requirements and must not be
  reused as LF-walk requirements. Pre-aux f_walk (50.2/61.0/58.2%) must not be compared with a
  post-aux-derived requirement.

Q4a — typed/frontend basis x BWT bytes-only   →  STATUS: PRESERVED, PROMOTED
  Now ranks above every timed job. Zero timing minutes; can invert the "backend is the
  mechanism layer" reframing and thereby rebase tracks 01/02/09 against 05/18.

Q4c — NEW (replaces Q4b, minimal scope) — postcoder-only vs full-decode stage A/B, BWT path
  Owner: 05 + 18 + 06.  Scope: ONE uninstrumented stage-split binary (postcoder-only decode,
  output discarded, labelled STAGE_PROBE_NOT_A_CODEC_ARM) vs the production decode, same job,
  same protocol, plus the direct stage-level A/B A16 records as not measured.
  Justification: the postcoder is 55.9-64.2% of post-aux BWT-path decode and is the ONLY
  lever whose solo Amdahl ceiling clears the Silesia xz-9e decode bar (2.421 s vs 2.569 s).
  Predeclared NO-GO: if the measured postcoder share is < 50% on >= 3 of 3 BWT-routed files,
  the BWT decode axis is terminal and no postcoder work is funded this cycle.
  No LF-walk profiling. No lane scheduler.

Q3 — selector arbiter   →  STATUS: Stage 1 CONFIRMED (bytes-only, ~0 timing minutes)
  Stage 2 timing remains conditional on a material choice divergence AND an effect above the
  8% sensitivity floor. Instrumented counters and timing stay separate (C1-C3 discipline).

Q2 — dense-frontier / block-window parity   →  STATUS: retained as the SINGLE timed job.
  Do not expand to the 20-arm vehicle. Fixes the D12 5-17% restart-tax confound, which is the
  admissibility denominator for every byte claim.

Q9 — aux W1024 vs W16   →  STATUS: DEFER on share grounds
  Prize <= 3,054 B = 0.0066% of Silesia bytes; no decode effect. Bundle or drop.

Small-share kills to enforce across all lanes:
  - entropy-pull throughput work: opcode entropy pulls are 0.3% of the token decode path
    (RESEARCH_LEDGER.md:3929). Dead on share.
  - BWT LF-walk speed work: cancelled above.
  - any job priced only against the 40 MB/s BWT-route bar: recorded unreachable by the
    wire-invisible route (A16) and reproduced from primary shares.
  - any new entropy-coder family as a novelty route: adopt/prior-art (unchanged).

Dispatch order after the above: Q0 -> Q1 -> Q3(S1) -> Q4a -> Q5 -> Q2 -> Q4c -> Q6 -> Q8.
Q7 and Q9 deferred. B1-B4 remain blocked. Queue invariant unchanged: a job whose premise is
invalidated upstream is cancelled, not run for completeness.
```

---

## 7. Statement for the coordinator

1. **The BWT backend's decode axis is closed by arithmetic on recorded shares.** The
   postcoder is 55.9-64.2% of its post-aux decode; the walk is 22.9-30.9%; the walk's own
   requirement is `k_w ≥ 5.2-13.3×` and is infeasible for every recorded target. **Q4b is
   cancelled and the lane scheduler is killed before implementation.**
2. **My own earlier recommendation is withdrawn in its load-bearing part.** The `f_walk ≥ 0.80`
   gate came from a cross-path cost transfer that omitted the postcoder stage; the
   reconstructed shares invert the conclusion. The surviving case is the postcoder, and only
   for **Silesia xz decode parity** — not Brotli parity, not enwik8 parity.
3. **The token lane's remaining Amdahl leverage is `crc32` (already fixed) and materialization
   (26%)** — not entropy scheduling (0.3%). Track 18's legs L1/L2 are DECODE-SHORT on share
   grounds; L0/L3 survive.
4. **Five jobs earn the next cycle's CI minutes, in this order:** Q0, Q1 (both free), Q3
   Stage 1 (bytes-only), Q4a (bytes-only), then **Q2 as the single timed job**, with **Q4c**
   (postcoder-only stage A/B) as the only new timed experiment justified by measured share.
   Q9 and Q7 defer; Q5/Q6/Q8 are cheap and follow.
5. **Every threshold above is frozen before data.** The one number I could not close is a CI on
   A16's stage shares; §4's sensitivity band shows the cancellation holds across it.
