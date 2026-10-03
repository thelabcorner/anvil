# Track 18 — Future Decoder/Backend Architecture — Fledge Alpha Free (independent adversarial review)

**Track:** 18-future-decoder-architecture · **Role:** independent adversarial reviewer / validator
**Date:** 2026-10-02 · **Tree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty
**Status of this document:** audit + pre-registration. **No measurement was performed by this agent.**
No local benchmark, no local fuzz campaign, no production edit, no commit/push.
**Evidence rule:** every number below is labelled `[MEASURED]` (with in-repo source location),
`[DERIVED]` (arithmetic on measured values), `[ARCH-REASONING]` (microarchitecture argument, unmeasured
on this host), or `[HYPOTHESIS]`. Nothing here is promoted from the constructive lane's claims without
independent re-derivation.

---

## 0. Ruling (stated first, argued below)

> ### Verdict: **HOLD** on decoder/backend architecture *mechanisms*. **KILL** on the entire
> > token-lane / entropy-pull / dispatch / superinstruction / small-copy family — *on a decode-floor
> > profile this project already measured*, not on a projection.
> >
> > **KILL** (terminal): token lanes / entropy-pull SIMD (**measured pool 0.3 %** of e2e decode,
> > ceiling **1.003×**); dispatch optimization and superinstruction ISAs (pool ⊂ **11 %**, ceiling
> > **1.124×**, below the **1.364×** the project's one true decoder win delivered); small-copy
> > specialisation (same pool); **CAM/PPM compact model priors as a decoder mechanism** (§5.5 — the
> > 64–256 B state budget cannot hold the required state, and the mechanism attacks the wrong axis);
> > independent-stream scheduling (measured falsified); rANS lane scaling beyond 4-way on AVX2; further
> > CRC; any "novel resolved-graph decoder" claim (OpenZL).
> > **PILOT (byte/RSS-decided, remote):** per-stream **table-width tiering** — *reclassified by this
> > audit from a decode mechanism to a **peak-RSS** mechanism* (§5.1) — and **dense-integer side-stream
> > recoding**.
> > **PROMOTE-TO-REMOTE:** no Track-18-private CI job. The shared substrate work is folded into the
> > cross-lane queue review (`REMOTE-EXPERIMENT-QUEUE-FLEDGE-REVIEW.md`).

**Binding measured constraint (new; it settles most of this track).** The decode-floor profile at
`RESEARCH_LEDGER.md:3911-3936` already decomposes `generated.log` decode to the percent and attributes
**0.3 % of end-to-end decode to opcode entropy pulls**. See §1.5.

I explicitly do **not** inherit the constructive lane's verdict. Track 11's report
(`11-orbit-programs-space-bunny.md`) concludes **PILOT** on FLI with G1 as its first gate. I agree with
its *sequencing* and reject its *first gate* — and on the measured profile I go further than
"unmeasurable": **FLI's decode leg is arithmetically dead, not merely small.** Details in §6.3.

**Companion artifacts (same role, same lane):**
- `SYNTH-PERF-FLEDGE.md` — cross-lane performance economics: measured pool, per-mechanism audit of all 20
  live mechanisms, the three merged shared-substrate experiments (SX-1/2/3), and the final missing-measurement list.
- `REMOTE-EXPERIMENT-QUEUE-FLEDGE-REVIEW.md` — adversarial review of the coordinator's queue, including the
  Q3 A0/A1 label defect, the Q3 whole-codec timing gate, and the Q4b kill.

---

## 1. The two blockers that gate everything else in this track

These are not findings about a candidate mechanism. They are defects in the **measurement substrate**
and in the **selection objective itself**, and every decoder-architecture proposal in this project —
including Track 11's FLI, Track 05's backend selection, Track 06's entropy co-design, and Track 18's
own proposals — is inadmissible until they are fixed.

### BLOCKER B1 — the harness cannot resolve small decode gains; it has a measured 2.3–8.1 % perturbation band

`RESEARCH_LEDGER.md` (Experiment AA / decode-perf t3 floor profile, ~line 3919) records, as a
characterization of ANVIL's own profiling harness:

| quantity | value | label |
|---|---|---|
| within-run CV | 0.6 – 10 % | `[MEASURED]` |
| run-to-run drift | ±2 – 4 % | `[MEASURED]` |
| **zero-byte-change control variants** | **+2.3 – 8.1 %** | `[MEASURED]` |

The third row is the load-bearing one and it is routinely skipped in prose summaries of this project.
A variant of the decoder that **changes no output bytes at all** still costs **2.3–8.1 %** of decode
time. That is the floor on attributing any decode difference.

Cross-lane, on **frozen, byte-identical wire**, the band is worse still. Reconstructed from three
independent in-repo windows on the same containers:

| cell | wire (B) | output (B) | measurement | ms | label |
|---|---:|---:|---|---:|---|
| `synth-timeseries` hotop-rlzp | 140,898 | 280,000 | PR-1 §11 v3, 218.4 MB/s | 1.281 | `[MEASURED]` |
| " | " | " | PR-1 §4 v2, 216.9 MB/s | 1.291 | `[MEASURED]` |
| " | " | " | ALLOC-only window `w-arch-alloconly-20260912T1045Z` | 1.380 | `[MEASURED]` |
| " | " | " | MAT window `w-arch-mat-20260912T0905Z` | 1.450 | `[MEASURED]` |
| `generated.json` mdl | 89,589 | 827,662 | PR-1 §5, 220.5 MB/s (CV 6.5 %) | 3.754 | `[MEASURED]` |
| " | " | " | PR-1 §11, 212.5 MB/s | 3.894 | `[MEASURED]` |
| " | " | " | `w-bench-remeasure-20260912T072549Z`, 189.146 MB/s (CV 15.95 %) | 4.375 | `[MEASURED]` |

Output sizes are `[DERIVED]` from the recorded ratios (140,898 / 0.50321 and 89,589 / 0.10824).

**Spreads `[DERIVED]`: +13.2 % and +16.5 %** on identical wire.

**Two empirically-grounded sensitivity floors** — not assumed, derived from which comparisons the
project's own harness actually resolved:

| design | evidence | floor |
|---|---|---|
| unpaired / cross-lane / cross-window | the 13.2 % and 16.5 % spreads above | **≥ 13 %** |
| **paired, same-job, interleaved, same wire, reps ≥ 7** | MAT A/B resolved +1.4 % (called "neutral") and +27.6 % (called "falsified"); ALLOC A/B resolved **+35.2 %** and **+7.4 %** — and 7.4 % was *accepted, landed and retained in `src/anvil.cpp`* | **≈ 5–7 %** |

That is the single most important number in this report: **the project's best available measurement
design resolves ~7 %, and 1.4 % was not resolvable.** Anything below ~5 % is not measurable even with
perfect paired design; anything below ~13 % is not measurable at all without it.

### BLOCKER B2 — the DEFAULT decode-cost objective provably cannot move a decision

> **CORRECTED 2026-10-02 after coordinator challenge.** My first draft claimed `FORMAT.md` and the binary
> *contradict* each other. **That was wrong in substance** — I read a second function's presence as the
> shipped default. Verified call sites and defaults below; see
> `REMOTE-EXPERIMENT-QUEUE-FLEDGE-REVIEW.md` §C1. The defect is real but **weaker** than I first stated,
> and its correct form is sharper: it is a **provable** property of the default path, not an
> order-of-magnitude hand-wave.

**Two selectors exist. Only one is default.**

| | selector | objective form | default? | source |
|---|---|---|---|---|
| **A0** | `encode_stream` | `J = L + λ·c + μ·2 + ν·c`, **constant-c** `{10,40,35,30,22,20,45}`, λ=0.01 | **YES — this is the shipped default** | `src/anvil.cpp:1548-1581` |
| **A1** | `encode_stream_budget` | `C_us = S · kBudgetNsPerByte[codec] / 1000`, **size-proportional** `{0.1,6.0,6.0,6.0,4.3,3.2,7.5}` | **NO — default-off** | `src/anvil.cpp:1628-1660` |

**Default proof:** `g_hotop_budget = false` at `src/anvil.cpp:1617`, plumbed from `opt.hotop_budget` at
`:4664`; and there is **exactly one** call site — `src/anvil.cpp:2868`:
`auto z = g_hotop_budget ? encode_stream_budget(*v2) : encode_stream(*v2);`

**`FORMAT.md` is CORRECT for the default path.** `FORMAT.md:648`'s table
`{raw 10, Huffman 22, default-exc 20, rANS-256 30, rANS-512 35, rANS-4096 40, ctx-rANS 45}` matches
`encode_stream`'s literals exactly. The residual, much weaker defect: **`FORMAT.md` is silent on the second,
flag-gated `encode_stream_budget` path and its distinct calibration** — a documentation-*completeness* gap
in the normative spec, not a contradiction. (`docs/decoder-audit.md` already records this section being
corrected once, for the `λ = 0.04` multiplicative text.)

**The sharp finding — a proof, not a projection.** On the default path the cost term is `λ·c` with λ = 0.01
and `c ∈ {10..45}`, so its **entire dynamic range** is:

```
λ · (c_max − c_min) = 0.01 · (45 − 10) = 0.35   < 1 byte
```

`L` is an **integer byte count**. A cost term whose full span is **0.35 < 1** cannot reorder two candidates
whose lengths differ by even one byte. Therefore:

> **On the default path the decode-cost term is an *exact-length tie-breaker* and nothing else.** `[DERIVED]`

This is confirmed to the observation by the S6-1 selector manifest (`RESEARCH_LEDGER.md:3965-3979`):
**792 selections (88 blocks × 9 streams, 23 files) → ZERO raw-flips**, and all **139** flips were
**exact-L ties** (`legacy L == budget L` in **139/139**), every one with `logical_eq=1 ∧ dec_eq=1`.
**Arithmetic and measurement agree exactly.**

**The flag-gated A1 path, for completeness.** Its cost term is `0.01 · S · nsPerB/1000`, i.e. dynamic range
`7.4e-5 · S` bytes against `max|Δc| = 7.5 − 0.1 = 7.4 ns/B`. So it can break a ~1-byte difference only for
`S ≳ 13,514 B` `[DERIVED]`; below that it degenerates to the same exact-tie-only behaviour. **Only one of
`generated.log`'s 22 streams clears that bar** (`distance_code` 15,680 B; `match_len_class` 8,788 B per
`docs/CONTEXT.md`), and at 15,680 B the entire cost term is worth **1.16 bytes** `[DERIVED]` against
candidate byte differences on such a stream that are routinely hundreds to thousands.

**The length-independence claim, independently confirmed — for the flag-gated path.** Track 06's finding
is that the incumbent term is length-independent while a measured size-proportional calibration exists.
Checked against the calibration table itself (`RESEARCH_LEDGER.md` ~3891–3899), which is the input to
`encode_stream_budget` (A1), i.e. the *flag-off-by-default* path:

| codec | 4 K bulk/byte/mat | 16 K bulk/byte/mat | 64 K bulk/byte/mat |
|---|---|---|---|
| raw | 0.10 / 2.66 / 0.02 | 0.04 / 2.61 / 0.02 | 0.10 / 2.59 / 0.09 |
| rANS-4096 | 6.15 / 6.49 / 4.35 | 6.04 / 6.62 / 4.36 | 5.93 / 6.42 / 4.73 |
| rANS-512 | 5.44 / 5.79 / 3.88 | 5.93 / 6.55 / 4.49 | 6.05 / 6.36 / 4.50 |
| rANS-256 | 5.40 / 5.69 / 3.83 | 5.99 / 6.55 / 4.41 | 5.89 / 6.33 / 4.38 |

`[MEASURED]`. **Every rANS width is flat within noise across an 16× size range** (5.93–6.62 ns/B).
The shipped size-proportional form `S · c/1000` is therefore *formally* length-proportional but
*empirically* length-**independent** for the codecs that dominate. The only real size dependence in the
whole table is `raw` bulk (0.04–0.10) and `huffman` bulk (4.08–8.56, and the ledger marks it
data-dependent). The ledger itself records the consequence as an unpatched known limitation:

> "the flat 6.0 rANS fit misses a size-dependent symtab effect (rANS-4096 ≈ 5–12 % slower/B than
> 512/256 on small streams)." `[MEASURED]`

**Consequence for this track, stated bluntly.** The **default** objective is *provably* an exact-length
tie-breaker, and the flag-gated size-proportional alternative is empirically length-**independent** for the
dominant codecs *and* below its own decision threshold for all but one stream per file. Therefore **every
proposal in this project whose selection rule is "let the decoder-cost objective decide" is currently gating
on a term that cannot move a decision.** That includes Track 11's S4 and G10, Track 05's backend selection,
Track 06's entropy co-design, and Track 18's cost-aware decoder selection.

This is **not** a reason to discard those proposals. It is a reason that **none of them may be preregistered
against λ as it ships**, and it is a *cheap, byte-only* negative: the queue's Q3 Stage 1 (comparing A0 vs A1,
two existing objective forms, **no third λ-fit arm**) is the correct experiment, and I concur with the queue
over my own earlier λ\* proposal.

---

### BLOCKER B0 — the measured addressable pool already exists, and it binds every mechanism in this track

Coordinator correction accepted: this is **not** a request for a new generic decomposition. The instrument
exists (`prototypes/profile_tmp/prof.cpp`, decode-perf t3) and its final before-state is recorded at
`RESEARCH_LEDGER.md:3911-3936`. I re-verified every field of that record.

**Provenance (re-verified):** payloads from CLI at `git HEAD 392e937`; `gen_log.anv = 175,550 B`
**exactly** the frozen-gate baseline; harness `prototypes/profile_tmp/prof.cpp`; HIGH priority; pinned;
QPC + invariant-TSC; **median-of-7 interleaved**, warmup 2; all 4 containers round-trip byte-exact;
**instrumented decoder verified byte-identical to `decode_tokens_hotop_fused` on every block**. Floor e2e
**235.8 MB/s (CV 10 %)**; frozen reference **273.5 MB/s**. Absolute level runs **5–15 % below** the
bench-suite protocol — only *relative interleaved* deltas are valid. `[MEASURED]`

**The pool** `[MEASURED]`:

| # | stage | share of e2e | ns/B `[DERIVED]` | addressable? |
|---|---|---:|---:|---|
| 1 | `crc32` bytewise | **44 %** (43.8 % log) | 1.860 | **CREDITED AND SPENT** — PCLMUL landed (A21) |
| 2 | eager materialization of the 7 macro streams (*"setup IS masks+resid"*) | **26 %** | 1.103 | **NO — measured non-removable** (MAT: lazy pull **+27.6 % slower**) |
| 3 | token loop beyond setup | **11 %** | 0.467 | partly |
| 3a | **of which opcode entropy pulls** | **0.3 %** | 0.0127 | ← this is what every token-lane proposal targets |
| 4 | concat / alloc / headers | **~15 %** | 0.636 | partly (ALLOC attacked this class) |
| 5 | unattributed residual | ~4.2 % | 0.178 | — |

The ledger's own summary: **"THE HOT PATH IS CLEAN; the floor is everything AROUND it."**

**Ceilings** `[DERIVED]`:

| question | ceiling |
|---|---:|
| absolute wire-invisible, post-CRC | `1/(1−0.52)` = **2.083×** |
| **realistic wire-invisible** (stage 2 proven non-removable) | `1/(1−0.26)` = **1.351×** |
| **smallest TRUE measured decode win in project history** | aux-unbwt/dickens **1.364×** |
| **entire token loop** (stage 3) | **1.124×** |
| **entropy pulls alone** (stage 3a) | **1.003×** |

> ### **1.351× < 1.364×.** The realistic wire-invisible ceiling on the best-profiled cell in this project is
> **below the smallest factor that has ever actually worked here.** And every architecture proposal on the
> current board names a pool smaller than that: **0.3 %** for entropy pulls, **≤ 11 %** for the token loop.

**Timing resolvability, from the same record** `[MEASURED]`: within-run CV 0.6–10 %; run-to-run drift
±2–4 %; **zero-byte-change control variants +2.3–8.1 %**, with the ledger's own note that
*"single-stream effects inside it [are] NOT resolvable."*

> **Kill rule: any mechanism naming a targeted share < 8 % cannot be certified on this harness.** 8 % is the
> perturbation band's own ceiling (8.1 %). The best paired design in project history resolved **+7.4 %**
> (ALLOC, landed and retained) and **failed to resolve 1.4 %** (MAT on mode-10).

**Per-mechanism pool audit.** Every timed/architectural proposal must name the measured share it attacks.
A proposal whose claimed speedup **exceeds the ceiling of the pool it names is inadmissible** unless the
wire-visible design *explicitly changes the decomposition* (aux-unbwt is the precedent: it is
representation-level, which is exactly why it escaped the ceiling and delivered 2.34×).

| proposal | share | ceiling | admissible? |
|---|---:|---:|---|
| entropy-pull / token-lane SIMD | **0.3 %** | 1.003× | **NO** — 27× inside the perturbation band |
| superinstructions / fused token loop | ≤ 11 % | 1.124× | **NO** — < 1.364× |
| dispatch / branch-prediction work | ⊂ 11 %, unattributed | ≤ 1.124× | **NO** — no measurement exists, and none can be created below 8 % |
| small-copy specialisation | ≤ 11 % | 1.124× | **NO** — and `generated.log` raw-hot already reaches 963 MB/s |
| rANS lane scaling > 4-way | — | — | **NO** — hardware-capped: 4×64-bit = 25 % of an AVX2 register; 8×64-bit needs AVX-512, absent on Zen 3 |
| concat/alloc/headers work | **15 %** | 1.176× | exhausted (ALLOC landed) |
| any wire-invisible decoder change | 26 % realistic | **1.351×** | **< 1.364× ⇒ the axis is closed** |
| **table-width tiering** | 26 % × ledger's 5–12 % relative band ⇒ **1.3–3.1 % of e2e** | 1.013–1.032× | **NO on decode** — reclassified to **RSS** (§5.1) |
| **dense-integer side-stream recoding** | 26 %, via a cheaper codec already in the suite (`defexc` mat **0.48–0.56 ns/B** vs `rans4096` mat **4.35–4.73 ns/B**, an **8–10×** spread) | up to 1.29× | **YES — byte-only decision, no clock required** |

**One caveat I must state against my own finding:** the §1 profile is on a **bytewise-CRC** base, so its
shares no longer describe the shipped decoder. Re-measuring it on the current build is the *only* profile
work still outstanding, and it is a re-run of an existing instrument — see `SYNTH-PERF-FLEDGE.md` §7 item 1.

---

## 2. Evidence audit and provenance

### 2.1 What is genuinely established `[MEASURED]`

| # | fact | source |
|---|---|---|
| E1 | The only decoder-architecture change in project history that produced a real whole-codec decode win: `libsais_unbwt_aux`. dickens **1.364×**, webster **1.795×**, enwik8 **2.339×**, all clearing a pre-registered 2 % gate, for **+22,398 B** on 311,938,580 B of source (**+0.00718 %**), zero routing changes. | `docs/I10-AUX-UNBWT-RESULTS.md` §3–6 |
| E2 | Its binary cost: **+2,976 B `.text`**, **+8,192 B** stripped-ELF file-size step. Final stripped combined CLI **453,512 B**. | `docs/I10-AUX-UNBWT-RESULTS.md` §9 |
| E3 | **After** E1, the external gate is still `FRONT-GAP_COST`: Silesia decode ratio vs xz **1.7226** (CI [1.6640, 1.8400], *inside* the 2× margin) but vs Brotli **3.4474** (CI [3.2680, 3.5059], *outside*); enwik8 vs xz **3.9065** (CI [3.9003, 4.0088]), vs Brotli **5.6642**. Peak RSS **4.572×/9.043× xz**, **1.995×/2.378× Brotli**. | `docs/I10-AUX-UNBWT-RESULTS.md` §11 |
| E4 | Wire-invisible decode optimization is **exhausted**: CRC spent (remaining share 1.3–3.4 %); MATERIALIZATION **falsified** (lazy `StreamPull` = +1.4 % on mode-10, **+27.6 % slower** on mode-15); ALLOC retained at 1.352×/1.074×; decode leg 4 (two-level tables + buffered renorm) retained at **1.179×** vs a ~1.4× target. | `prototypes/i9-arch/MAT-ALLOC-leg.md`, `prototypes/i9-arch/LEG4-postcoder.md`, `docs/anvil-i9-findings.md` §6 |
| E5 | Corrected postcoder requirement with the *real* aux factor: **6.05× dickens / 3.86× webster / 6.60× enwik8**. Corrected portfolio projection **~25.7 MB/s vs a 40 MB/s bar**. | `docs/anvil-i9-findings.md` §6.2 |
| E6 | `libsais_unbwt_omp` **falsified**: it parallelizes init only, no walk gain. | `docs/anvil-i9-findings.md` §6.1 |
| E7 | Grammar factorization of mode-15's book streams (RePair/RLZ, Exp. Y): **−4.15 %..−10.76 % bytes** but **−8.1 %..−15.9 % decode** on every primary record file, encode 48–64× slower. | `RESEARCH_LEDGER.md` Exp. Y |
| E8 | Correction masks are **~90 % unique** (top-32 covers 7–12 %); `(k,slot)` modal accuracy **~20 %**, not 86.5 %; mode-13 topology coding **lost +13.5 %..+21.0 %** on every record file. | `RESEARCH_LEDGER.md` Exp. I |
| E9 | RSS is a priced axis: Silesia 16 MiB BWT subblock cap → 125.7 MiB at **+1.0169 %** bytes; 8 MiB → 124.4 MiB at **+1.9346 %**. On enwik8, 8/16/32 MiB route **away from BWT** and cost **+5.4075 %** bytes. | `docs/I10-BREAKTHROUGH-PROGRAM.md` I10-1A.2 |
| E10 | Reconstructed per-symbol dispatch surcharge: MAT's lazy-pull arm added **1.400 ms** over the eager arm on the 140,898 B cell; PR-1 attributes ~31.3 % of e2e (0.454 ms of 1.4495 ms) to bulk `decode_stream`; at the measured 5.9–6.6 ns/B that materializes ≈ 73 kB of side-stream bytes, so the added dispatch is **≈ 5.5 ns/symbol** ≈ 23 cycles at 4.2 GHz. | `[DERIVED]` from E4 + the Exp. AA table |
| E11 | Entropy-floor arithmetic is structural: matching xz decode needs BWT-routed bytes ≤ **8.38 %**, but the BWT-favorable files are **56.79 %** of Silesia. Decode-time conflict ≈ **6.8×**. | `docs/audit-2026-09-07/06-do-not-reburn.md` F0.1 |
| E12 | Context clustering is **not** novelty — Brotli RFC 7932 already maps decoded literal context to several prefix trees via a compact context map. | `docs/CONTEXT.md` |

### 2.2 Projections the project has been treating loosely `[PROJECTION]`

- PR-1's `mat`-inclusive caps (2.06×, 3.69×, 1.60×) were **explicitly retired** as falsified
  (`prototypes/i9-decode-perf/REPORT.md` §12). Any reuse is a superseded-number revival.
- The frozen-CSV decode baselines (`generated.json` mdl 112.8 MB/s vs the lane's 212.5–220.5 MB/s)
  disagree by ~2× and were flagged as an open contingency; they remain unreconciled, which is why B1's
  spread is the safer citation.
- `docs/CONTEXT.md` Linux figures (shape-predict 957 MB/s, "possibly the first three-axis crossing")
  are explicitly self-flagged as "timing moved with CPU state (not called yet)" and were never promoted.
  They are not admissible as decoder-architecture evidence.

### 2.3 Provenance defect found by this audit — **CORRECTED, weaker than first stated**

**Superseded claim (withdrawn).** I first asserted that `FORMAT.md` §"Stream-codec selection objective"
**contradicts** the shipped decoder. **That was wrong.** I had read the presence of a second, size-proportional
selector as if it were the shipped default.

**Verified position.** `src/anvil.cpp:2868` has exactly one call site, selecting between
`encode_stream_budget` (size-proportional) and `encode_stream` (constant-c); `g_hotop_budget` defaults to
`false` (`:1617`). So the **default** is constant-c, and `FORMAT.md:648`'s table **matches it exactly**.

**The real, weaker defect.** `FORMAT.md` does not document the second, flag-gated `--hotop-budget=on` path,
nor its distinct `kBudgetNsPerByte` calibration (`src/anvil.cpp:1618-1626`). This is a
documentation-**completeness** gap in the normative spec, not a contradiction — worth a `format`-lane note,
worth nothing more. `docs/decoder-audit.md` records this section being corrected once already (the
`λ = 0.04` multiplicative text), which is why I should have checked before asserting a second drift.

---

## 3. Prior-art map for track 18's subject

Track 18's brief is "new decoder/backend architecture: data layout, vectorization, superinstructions,
independent-stream scheduling for future formats." Mapped:

| axis | prior art | novelty verdict |
|---|---|---|
| **resolved-graph / recipe decoder** (encode-time search → embed a resolved plan → one universal decoder executes it) | **Meta OpenZL** — arxiv 2605.09928, `github.com/facebook/openzl`, FB Engineering 2025-10-06; v0.2 ships a graph-native LZ path and aggressive operator fusion | **OCCUPIED AND SHIPPED.** This is the track-18 headline, and it is a 2025–2026 open-source product. **No novelty claim is possible.** |
| fused command loop / resolved semantic stream | Brotli meta-block `insert-and-copy` quadruples + length splits + distance cache + block switching; LZMA rep0–rep3; DEFLATE block symbol stream | adopt-class |
| superinstruction / opcode book | interpreter literature; Brotli command aliasing; ANVIL's own Exp. R (hot-op) and Exp. Y | adopt-class, and E7 shows the grammar variant is measured decode-**negative** |
| SIMD rANS, multi-state | ryg_rans; jkbonfield `rans_static` (AVX2 multi-state); Intel ISA-L | adopt-class; see §4.2 for the AVX2 lane cap |
| dense integer side-streams | Stream VByte (arXiv 1709.08990), TurboPFor / FastPFOR, PForDelta, ALP (10.1145/3626717), FastLanes, Parquet-ALP adoption (2026-09) | adopt-class — but see §5, this is one of only two survivors |
| hardware polynomial CRC | ISA-L; PCLMUL — **already landed** | spent (E4) |
| BWT inverse | libsais bi-gram LF + `unbwt_aux` — **already landed** (E1) | spent as far as architecture goes |
| columnar / lane-oriented decode | ALP, FastLanes, BtrBlocks, LogComp, CLP | adopt-class |
| **independent-stream scheduling** | **measured falsified in-repo** (E6) | **KILL — terminal** |

**Net prior-art ruling:** the *architecture* track 18 names is Meta's. Track 18's only defensible output
is a narrow, non-obvious **interaction** plus an honest measurement instrument — exactly the posture
Track 11 takes for FLI, and exactly the posture track 20 must be asked to police.

---

## 4. Hidden byte / cycle / memory costs

### 4.1 Table footprint saturates L1 — the strongest architectural finding in this track

`[ARCH-REASONING]`, grounded in `[MEASURED]` host facts:

- Measurement host: **AMD Ryzen 9 5900X**, base **4200 MHz** (`tests/host-spec.md`) → **Zen 3**:
  **32 KiB L1d**, 512 KiB L2 per core. Remote evidence hosts: **EPYC 7763 (Zen 3)**, **EPYC 9V74
  (Zen 4)** (`docs/I10-AUX-UNBWT-RESULTS.md` §3, §5).
- ANVIL's rANS normalizes frequencies to total **4096** — the decoder *rejects* any model whose
  frequencies do not sum to 4096 (`docs/decoder-audit.md`, `decode_stream`). A 4096-entry decode table at
  4 B/entry is **16 KiB**; packed as (state, symbol) 8 B/entry it is **32 KiB**.

> **A packed rANS-4096 decode table occupies 100 % of Zen 3's L1d — there is zero co-residency left for
> the hot-op book, the output write window, or the side-stream byte cursors.**

`docs/FRONTIER-RESET-2026-09-23.md` §8.3 already states the rule ("A decode table that misses L1/L2 can
lose more than it saves in arithmetic… A smaller 256/512-state table may beat a 4096-state table for
tiny streams even when the latter saves bits") and `docs/CONTEXT.md`'s next-iteration target #2 names
"precision/work-adaptive entropy: a 256/512-state variant with smaller cache-resident tables" as the
cheapest remaining enablement. **This is the one track-18 direction that is (a) unnamed-by-no-one-here,
(b) wire-visible so outside the exhausted class, and (c) byte-only-decidable.** It is §5's survivor 1.

Corroborating measurement already in hand: the ledger's own unpatched note that rANS-4096 is
**5–12 % slower/B than 512/256 on small streams** — i.e. the footprint/setup effect is real and already
quantified. Note the honest tension: **the low end of that band (5 %) is inside B1's perturbation floor,
so only the 12 % end is timing-measurable.** Hence byte-first.

Table *build* cost, for completeness `[DERIVED]`: a 4096-entry fill is ≈ 2048 cycles ≈ 0.5 µs; 22
streams ≈ 11 µs against `generated.log`'s 7.03 ms decode = **0.16 %** — negligible. **The problem is
footprint and miss traffic, not construction.**

### 4.2 Serial entropy dependency and the AVX2 lane cap — why "more lanes" is dead

`[ARCH-REASONING]`: rANS state update `x_{t+1} = F(x_t, s_t)` is a **dependent chain** whose critical
path passes through a table access indexed by `x & mask`. That access is a **gather**, and a gather
cannot hide a *dependent* latency, so it cannot be SIMD-accelerated in place. Throughput therefore
comes only from interleaving **independent** states — exactly what `docs/CONTEXT.md` records as already
landed ("interleaved 4-way rANS states (rans2x4): ~30 % decode speedup on Windows").

Lane geometry on the actual evidence hosts:

| configuration | lanes used of a 256-bit AVX2 register | width |
|---|---|---|
| 4 × 32-bit states | 4 / 8 | 50 % |
| 4 × 64-bit states | 4 / 8 | 25 % |
| 8 × 64-bit states | requires AVX-512 | unavailable on Zen 3 |

> **KILL: rANS lane-count scaling beyond 4-way on AVX2/Zen 3 is unmeasurable by construction.** There is
> no mechanism left on this axis; the only remaining lever on the entropy leg is *table width*
> (§4.1) or *symbols per output byte* (a ratio question, not an architecture question).

Also terminal: reciprocal-rANS (D3, measured slower than hardware divide), threaded unbwt (E6), further
CRC (share now 1.3–3.4 %).

### 4.3 Branch predictors — dispatch is structurally unpredictable, and is already paid

`[ARCH-REASONING]`: mode-15 dispatch is an indirect switch on an **entropy-decoded** opcode. While the
opcode stream is entropy-coded, the target is information-theoretically unpredictable to the predictor;
no amount of kernel work fixes this. LZMA, Brotli and zstd all accept the same floor.

`[DERIVED, E10]` the measured magnitude of the *surcharge* for adding dispatch is **≈ 5.5 ns/symbol
(≈ 23 cycles)** — MAT's lazy-pull arm paid exactly that, per symbol, to deliver the *same* entropy work
the eager path delivered in bulk. Two consequences:

1. This **corroborates** Track 11's `c_pull + c_dispatch ≈ 6–8 ns` estimate — it is not fantasy.
2. It simultaneously shows the incumbent bulk path **already pays** that dispatch as indexing cost.
   Any "fuse the entropy decode away" saving must therefore be measured **against the incumbent**, not
   against a hypothetical, and MAT is the standing proof that naive fusion *loses*.

### 4.4 Gather/scatter — the one place it cannot be improved, and the one where it can

`[ARCH-REASONING]`: mode-11 sparse correction writes `out[start + 32*w + b]` per residual byte. With
masks ~90 % unique (E8) the *positions* are incompressible, so scatter placement is **not vectorizable
by construction** — and mode-13 already lost +13.5…+21.0 % attempting to model exactly that topology
(E8). **Correction scatter is a dead axis.**

Gather/scatter *is* available in the **dense integer side streams**, because Structure-of-Arrays layout
is already the wire layout there. That is §5's survivor 2, and it is adopt-class prior art
(Stream VByte / TurboPFor).

### 4.5 Decoder binary size — there is exactly one measured exchange rate in this project

From E1 + E2:

> **≈ 3 KiB of `.text` buys 1.36–2.34× whole-codec decode.** `[DERIVED]`

Two operational consequences:

- Any candidate whose `.text` growth exceeds **~8 KiB** must demonstrate **≥ ~4×** decode to be
  defensible on this exchange rate. **Nothing on the current board projects 4×.**
- The I10-1A audit found the ELF *file-size* step is 8 KiB-granular while actual text growth was 2,976 B
  (§9 there). **Report `.text` separately from stripped-ELF size or the number is meaningless** — this
  is a live reporting trap for any new-mode proposal.

Context: 14 block modes are already registered, each with a forced encode→decode test
(`docs/anvil-i9-findings.md` §3). Registry and fuzz cost per new decoder-visible mode is real and
routinely underestimated in proposals.

### 4.6 Are claimed speedups compatible with the ratio goals? — the arithmetic

From E3, **after** the best decoder architecture the project has ever measured:

| corpus | vs xz -9e | vs Brotli q11 | vs 2× predeclared margin |
|---|---:|---:|---|
| Silesia | 1.7226 | 3.4474 | vs xz **inside**; vs Brotli **outside** |
| enwik8 | **3.9065** | **5.6642** | **outside both** |

> **The binding deficit that remains after a measured 2.34× decoder-architecture win is 3.9× (vs xz) and
> 5.7× (vs Brotli) on the large cell.** `[DERIVED]`

A proposal must therefore clear **≥ ~4× decode** to be in the conversation at all on enwik8-class cells,
before any byte win is counted. Combine with B1's ~7 % paired floor and §4.5's 8 KiB `.text` cap and the
admissible window collapses to a very short list — which is why §5 has only two entries, and why both are
byte-only-decided.

---

## 5. Alternative mechanisms that survive (materially different from the constructive lane)

Track 11's FLI is a **token-loop** mechanism. So is Track 18's own "superinstruction" and "fusion"
framing. §6 argues that family is measurement-inadmissible. The two directions below are admissible
because their GO/NO-GO is decidable **with no timing claim at all** — and per master-brief item 5 and
rule E5 (`docs/audit-2026-09-07/06-do-not-reburn.md`), that is the decisive property.

### Survivor 1 — per-stream table-width tiering (precision/work-adaptive entropy) `[PILOT, byte-only]`

- **Mechanism.** Choose the rANS normalization width per substream from its length, not globally:
  `{256, 512, 1024, 4096}` with width selected by a byte-only rule; transmit the width in the existing
  per-stream mode byte (no new stream-codec ID needed beyond what the suite already has — modes 1/2/3
  are already rANS-4096/512/256).
- **Why it is the right target.** §4.1: a packed 4096 table consumes 100 % of Zen 3 L1d; 256/512 tables
  are 1–2 KiB and restore co-residency. The ledger already quantifies the effect as **5–12 % slower/B for
  rANS-4096 vs 256/512 on small streams**.
- **Byte cost is deterministic and needs no clock.** Sweep the width rule byte-only across the whole
  corpus; the wire cost of precision loss is exactly computable.
- **Wire status.** **Wire-visible** — therefore *not* in the exhausted wire-invisible class (master-brief
  item 12), and it reuses the proven S6-1 selection machinery.
- **Honest scoping — and a self-correction.** I first wrote this as a decode mechanism on the strength of
  the ledger's 5–12 % band. **§1/B0 falsifies that reading**: applied to the 26 % materialization share
  the effect is **1.3–3.1 % of e2e**, a ceiling of **1.013–1.032×**, far below the ~7 % paired floor. **The
  decode claim is dropped.** What survives is the **memory** claim: the table is a **32 KiB** packed
  resident, against a measured **9.043×** RSS deficit vs xz on enwik8 and **4.572×** on Silesia. So the
  decision rule is **bytes + peak table residency**, and a timing arm is admissible only if a *re-measured*
  post-CRC profile predicts ≥ 7 % e2e.
- **Class.** Adopt-class infrastructure. No novelty claim. Its value is that it converts a *measured*
  cache-footprint defect into a cheap byte-only decision.

### Survivor 2 — dense-integer side-stream recoding for the SoA metadata streams `[PILOT, byte-only]`

- **Mechanism.** Replay the current SoA metadata streams (lengths, distance codes, residual positions,
  correction masks — exactly the list in `docs/FRONTIER-RESET-2026-09-23.md` §8.2) through FOR +
  bitpack / delta-of-delta / Stream-VByte-style control-and-data split / sparse exceptions. **No parse
  change.**
- **Evidence it is not already done and not already dead.** It is named as next-iteration target #2 in
  `docs/CONTEXT.md` and as H5 in `docs/I10-BREAKTHROUGH-PROGRAM.md`, both still open. The Exp. AA table
  already proves a *cheap materialize* path exists for default-heavy streams (`defexc` mat
  **0.48–0.56 ns/B** vs rANS mat 4.35–4.73 ns/B `[MEASURED]`) — an 8–10× spread the current objective
  is too mis-scaled (B2) to exploit. **B2 and this are the same defect seen from two sides.**
- **Prior art.** Stream VByte, TurboPFor/FastPFOR, ALP. Adopt-class, explicitly. Track 13 and Track 17
  touch adjacent ground; coordinate rather than duplicate.
- **Class.** Adopt-class. Byte-only decidable.

**Neither survivor is novel. That is the honest state of the decoder-architecture axis at this point in
the project's life, and it should be recorded as such rather than dressed up.**

---

## 6. Reconciliation with the constructive lane (Track 11 §9 P1) and with Track 06

### 6.1 Agreement

- **Sequencing is right.** Track 11 §12.3 puts cost decomposition before mechanism build; I agree and
  go further (§7): the decomposition must be split into a **counter job** and a **timing job** that
  share no timing claim.
- **The short-copy length histogram is the decisive unknown.** Track 11 §5.2 concedes the decode leg is
  worth "≤ ~5 % whole-codec" *unless* per-token cost turns out much larger than the mode-15 description
  implies. My §6.2 sensitivity shows the whole range turns on exactly the quantity Track 11 §8's oracle
  does **not** measure: `(length bucket) × (token category) × (coverage)`, i.e. iterations per output byte
  `φ/L`. **Add that histogram to P2; it is byte-only and it decides G1/G6 before any mechanism is built.**
- **P1a/P1b/P1d are cheap and correct.** P1d is in fact the highest-leverage item in the whole swarm —
  and I have now *answered* it rather than leaving it open (§2/B2): λ as shipped cannot move a decision.

### 6.2 Disagreement 1 — Track 11's G1 is not a decidable gate as written

Track 11 §10 G1: *"GO iff measured per-iteration **entropy+dispatch overhead ≥ 25 %** of measured
per-iteration decode cost."* §9 P1c proposes to obtain this with **counters, not timers**:
entropy-pull count, varint-read count, copy bytes, macro entries, literal bytes.

**This is a unit error and it is fatal.** Counters produce *operations*; G1 demands a *share of measured
time*. Dividing one by the other mixes ns and counts. P1c cannot decide G1 even if the harness were
perfect, and under B1 the harness is not perfect.

Worse, the counters would *appear* to pass, and the trap is a **denominator conflation**. The
`synth-timeseries` / `generated.json` stage splits (side-stream `mat` **25–31 %**, token loop **60–62 %**)
are real `[MEASURED]` but they belong to **different cells**. On `generated.log` — the one cell with a
complete, verified profile — the split is **26 % / 11 %**, and **opcode entropy pulls are 0.3 %** (§1/B0).
So a counter reading of "entropy+dispatch" on the log cell reports ≈ 26–37 %, comfortably past G1's 25 %,
while the component Track 11's mechanism actually deletes — the per-token **entropy pull** — is **0.3 %**.
**G1 would pass on a share that FLI cannot collect**, and the sharing cell that makes G1 look tight is not
the cell FLI was sized on.

> **Corrected gate (my §8, T3):** require the *cycle* share attributable to **pull + dispatch + state
> update inside the token loop**, obtained from within-job kernel timings weighted by the counter
> histogram, and require its implied ceiling `C_max = 1/(1−s)` to be **≥ 1.36×** (the smallest *true*
> measured decode win in this project, E1/dickens) to justify building anything, and **≥ 2.0×** to
> justify frontier language.

### 6.3 Disagreement 2 — MEASURED KILL: the pool is 0.3 %, not "straddling the floor"

Track 11's own sensitivity model concedes that FLI removes "one entropy-coded opcode plus one varint n" per
loop — i.e. it targets **entropy pulls**. §1/B0 measures that pool on the only fully-profiled cell:
**opcode entropy pulls = 0.3 % of end-to-end decode.**

> **`C_max` for FLI's actual target is `1/(1−0.003)` = 1.003×.** Even crediting FLI with the *entire*
> remaining 10.7 % of the token loop (dispatch + state + copy) — which it cannot, since it does not remove
> copy or the state update — the ceiling is **1.124×**. **Both are below the 1.364× that the project's one
> true decoder win delivered, and the pulls-only figure is 27× inside the 2.3–8.1 % perturbation band.**

This is a stronger conclusion than §6.3's original "unmeasurable". It is not a forecast about how well FLI
might be implemented; it is the ceiling of the pool FLI names, taken from a profile this project already
measured. **FLI's decode leg is dead on arithmetic.** Its byte leg survives only if G2–G5 pass, and then
only as adopt-class ratio work (master-brief item 12's pattern).

#### 6.3.1 Counterfactual retained for completeness — the sensitivity band, and why it is now moot

If `H1` were true and per-token cost were dominated by work *outside* the measured profile, FLI's
whole-codec gain would be `φ·8/(L·3.656)` `[DERIVED]` (baseline 3.656 ns/B = the recorded 273.534 MB/s):

Track 11 §5.2's whole-codec decode gain from FLI is `φ·8/(L·3.656)` `[DERIVED]`:

| φ (coverage of output bytes) | mean iteration L | projected decode gain | measurable? (paired floor ≈ 7 %) |
|---:|---:|---:|---|
| 0.40 | 40 | **2.2 %** | **NO** — below perturbation band |
| 0.60 | 64 | 2.1 % | **NO** |
| 0.80 | 24 | 7.3 % | marginal |
| 0.60 | 8 | 41.1 % | yes |
| 0.60 | 6 | 21.9 % | yes |
| 0.40 | 4 | 21.9 % | yes |

(Baseline 3.656 ns/B = the recorded 273.534 MB/s `[MEASURED]`, `RESEARCH_LEDGER.md` Exp. AA.)

**The outcome is bimodal and entirely determined by the short-copy histogram.** In the long-copy regime
Track 11 assumes as its worked example (L = 40), FLI's gain is **2.2 % — unmeasurable on this project's
harness by any design, including perfect pairing** (MAT failed to resolve 1.4 %). In the short-copy regime
the same mechanism is worth 20–40 % and would be the largest single decode win since E1.

That is a genuinely interesting scientific question. It is **not** a licence to build FLI, because:

- master-brief item 12 retires wire-invisible decode work, and FLI is wire-visible so it survives that —
  but the whole-codec *binding deficit* is 3.9–5.7× (E3), and FLI's projected 2.2 %/21.9 % band straddles
  a bar it cannot be shown to clear;
- **if** FLI lands in the short-copy regime at ~22 %, the honest conclusion is that **ANVIL's token loop is
  dominated by 4–7 B iterations**, which is a *representation* finding (the parser is emitting too many
  tiny tokens), not a decoder-architecture finding. `docs/CONTEXT.md` already records that
  "4–7 B sidecar tokens were previously a major token-count problem" and that the TCOPY work identified
  "hundreds of thousands of tiny 4–7 B exact tokens" as the decoder's dominant work — with the *same*
  proposed remedy, spending rate surplus to reduce token count. **That remedy belongs to Track 09/14's
  lane, not to a new decoder ISA.**

### 6.4 Agreement with Track 06's finding — and it is stronger than stated

Track 06 reports the incumbent stream-selection objective uses a **length-independent** decode-cost term
while a measured size-proportional calibration exists elsewhere. My independent check (§1/B2) confirms
both halves and adds three things Track 06 did not state:

1. **The spec is wrong, not just the source.** `FORMAT.md:636–663` still documents `c ∈ {10..45}`
   integer-tenths; `src/anvil.cpp:1618` ships `kBudgetNsPerByte = {0.1, 6.0, 6.0, 6.0, 4.3, 3.2, 7.5}`.
   The normative format and the binary disagree.
2. **The units are incoherent.** λ multiplies *microseconds* and the result is added to a *byte* count.
   The implied exchange rate is **1 byte ≡ 100 µs of decode**.
3. **The size-proportional form is empirically length-independent for the dominant codecs.** The Exp. AA
   table is flat for every rANS width across a 16× size range. So even after recalibration, the term is
   blind to the only real size effect in the data (rANS-4096 being 5–12 % slower/B than 256/512 on small
   streams).

**Therefore: do not recalibrate λ and then gate on it.** First run F-1 (§8) to establish whether λ can
flip *any* real decision; only a positive result justifies spending effort on calibration. My §5 survivor 1
is deliberately positioned so that it does **not** depend on λ at all — its decision rule is byte-only and
length-keyed, which is what B2 says the objective cannot currently express.

---

## 7. Measurement design: counters vs timing, with no benchmark-window splicing

This is the cross-track instrument Track 11 §9 P1 asks for, hardened for B1/B2. **The rule is that no
single timing claim may depend on more than one job's machine state, and no counter from one job may be
combined with a timing from another without a within-job validation arm.**

### Job C — counters. **Byte-only. Emits no timing claim whatsoever.**

- Instrumented build, flags default-OFF, counters accumulated under a runtime flag.
- **Admissible outputs: integer counts and histograms only.** The job records no wall-clock value and no
  ns/B may be quoted from it. Its perturbation is therefore irrelevant — that is the entire point of
  separating it.
- Required gates **before any count is admitted**: 442/442 round-trip; `fuzz.py --cases 50` PASS; every
  row byte-identical by SHA-256 to the freeze-HEAD binary; forced registry coverage per `FORMAT.md`
  §"Registry coverage"; instrumentation flag OFF must be byte- **and time-identical** to freeze-HEAD.
- **Counts required** (this is the corrected version of Track 11 P1c — histograms, not scalars):
  1. entropy-pull count per stream, per token category;
  2. varint-read count per token;
  3. **match-length histogram weighted by bytes covered** — the decisive `φ/L` artifact;
  4. macro-path entries, literal bytes, correction-mask bits set, scatter byte count;
  5. **opcode alphabet histogram** (= `H_op`, Track 11 P1a, folded in here so it is one job);
  6. side-stream decoded lengths (feeds F-1's break-even stream size of 13,514 B).

### Job T+K — timing and kernel microbenchmarks. **Same job, same runner, same core pin, interleaved.**

This is the anti-splicing mechanism: `ns-per-component` and `whole-codec ns` come from the *same* machine
state, so the validation `Σ components ≡ whole-codec` is meaningful.

- **T-arm 0 (baseline):** freeze-HEAD binary, uninstrumented, on the pre-registered primary cells.
- **T-arm A/A null:** same binary run twice under the identical protocol. CI must span 1.0 or the job is
  VOID. *(Protocol precedent: `docs/I10-AUX-UNBWT-RESULTS.md` §11 ran exactly this and it passed.)*
- **T-arm Z (zero-byte-change control):** a variant built from the *same* source that changes no output
  bytes. **Its measured cost is the job's declared noise floor and must be reported before any other
  number.** Pre-registered: if Z > 5 %, all decode numbers in the job are VOID.
- **K-arms:** standalone binaries containing **no codec code**, replaying traces emitted by Job C:
  (k1) rANS pull chain at each width; (k2) indirect dispatch through a switch vs a computed-index table,
  fed by the measured opcode histogram; (k3) state update; (k4) short-copy kernels for
  L ∈ {4, 6, 8, 16, 32, 64, 256} using the measured length histogram; (k5) per-stream table-width build +
  first-touch cost at {256, 512, 1024, 4096}.
- **Validation arm:** `Σ (Job-C counts × Job-TK per-unit ns)` must reconcile to the Job-TK whole-codec ns.
  Report the residual. **If the residual exceeds 35 %, the attribution is invalid and no component claim
  from this job may be promoted.**
- Pinning: `taskset -c 0` per `docs/GITHUB-ACTIONS-BENCHMARKING.md` §5; record `lscpu`, `uname -a`,
  compiler, CMake, dependency, CPU flags, runner image, job/run ID, corpus hashes, exact command line,
  per-rep raw timings per §4/§8.1 of that document. Shared-runner results are **scout/ranking-grade**
  only; absolute MB/s across runs is never quoted.

### Why this cannot be spliced

1. Counts (Job C) never produce a time claim.
2. Times (Job TK) never depend on a counter from another job except through the *within-job* K-arms,
   which are re-measured every run.
3. Every ratio in every gate is computed **inside one job** from arms measured in that job.
4. The single legal cross-job composition is `counts(C) × ns-per-unit(TK)`, and it is admissible **only**
   because the same job that produced `ns-per-unit` also produced the whole-codec number it is validated
   against.

### Paired uninstrumented control required for any recalibration experiment

Per the coordinator's requirement: **any λ-recalibration experiment must carry, in the same job, an
uninstrumented paired control.** Concretely — arms `R0` (freeze-HEAD, `--hotop-budget=on`, λ = 0.01
incumbent) and `R1` (λ = 0), plus **arm Z0: a zero-byte-change control variant built from the same
source**. Gates, in order:

1. `R0` must reproduce the frozen 175,550 B on `generated.log` **exactly** and must be
   **time-identical to freeze-HEAD within the paired band**; otherwise VOID.
2. `Z0`'s measured perturbation is published as the job's noise floor **before** any other number is read.
3. Only then may `R2` (calibrated λ*) be read.

An experiment that skips step 2 is VOID by the precedent already set in this repo (the 2.3–8.1 % control
band exists *because* someone eventually ran the control).

---

## 8. Decisive remote-only falsifier, with frozen thresholds

### F-1 (primary, cheapest, byte-only) — can λ move a decision at all?

**One GitHub Actions job. No timing is used or reported as a claim. Deterministic. ~1 CI cycle.**

Arms, same binary (`--hotop-budget=on`), single checkout:

| arm | λ | purpose |
|---|---|---|
| R0 | 0.01 (incumbent) | baseline; must be byte-identical to freeze-HEAD |
| R1 | 0 (pure length) | exposes whether λ ever changed anything historically |
| R2 | λ\* = 1000 / (7.4 · 16384) ≈ **0.00822 per byte·ns** scaled so the cost term reaches ~1 byte on a 16 KB stream | calibrated λ — makes the term *just* able to flip a 1-byte difference |
| R3 | 10⁶ (cost-approaching) | reveals the term's actual decision boundary |
| Z0 | — zero-byte-change control from same source | declares the noise floor |

Corpus: full Silesia + enwik8 + `generated.{log,json,jsonl,sqlite}` + the held-out PE set.

**Frozen thresholds (fixed before the run; not movable after):**

- **T-F1.1 (validity):** R0 byte-identical to freeze-HEAD on every file, **and** Z0 perturbation published.
  Else VOID.
- **T-F1.2 (kill):** if `bytes(R2) == bytes(R0)` **bit-for-bit on every corpus file**, then λ cannot
  arbitrate any decision at this corpus scale ⇒ **KILL every λ-gated decoder-architecture proposal**
  (Track 11 S4/G10, Track 05 backend selection, Track 06 entropy co-design, Track 18 cost-aware
  selection) **and** record the FORMAT.md/source drift as a `format`-lane defect to fix.
- **T-F1.3 (survive):** if `bytes(R2) < bytes(R0)` on **≥ 4 files** with total `≤ −0.10 %`, λ *can* move
  decisions ⇒ the objective is worth calibrating properly and the family is **re-opened**, at the cost of
  a real FORMAT.md rewrite (units: bytes per microsecond) plus a byte-identity gate.
- **T-F1.4 (intermediate):** any outcome between T-F1.2 and T-F1.3 ⇒ **HOLD**; report the actual
  break-even stream size and re-derive λ\* from the *measured* stream-length distribution rather than
  from my 13,514 B estimate.

**My prediction (recorded before seeing data):** T-F1.2. The arithmetic in §1/B2 makes bit-identity the
near-certain outcome. That is the correct shape for a pre-registered falsifier — cheap, deterministic,
and its most likely result is informative.

### F-2 (secondary, follows F-1 only if T-F1.3) — table-width tiering, byte-only

Same job structure, same Z0 control. Sweep per-stream width ∈ {256, 512, 1024, 4096} under a
length-keyed rule.

- **T-F2.1 GO:** total corpus byte cost `≤ −0.30 %` **and** ≥ 4 of 6 canonical files improve.
- **T-F2.2 KILL:** total byte cost `> 0 %` on any of the frozen ratio-first corpora
  (`tests/ratio-first-standard.csv`, `tests/bwt-backend-standard.csv`) — a ratio regression on the byte-win
  portfolio is terminal regardless of decode.
- **T-F2.3 timing precondition:** a timing attempt is admissible **only if** the ledger's 5–12 % effect
  band is predicted to land **≥ 7 %** (B1's paired floor). Otherwise the mechanism is recorded
  byte-only and explicitly labelled `[PROJECTION]` on the decode axis, per the project's own precedent.

### F-3 (only if F-1 or F-2 survive) — the corrected cost decomposition

Job C + Job TK of §7, with T3 as the gate:

- **T3 GO:** `C_max = 1/(1−s_pull+dispatch+state) ≥ 1.36` on **≥ 2 of 3** primary cells, residual ≤ 35 %,
  Z0 ≤ 5 %.
- **T3 KILL:** `C_max < 1.36` anywhere, **or** the reconciliation residual > 35 %, **or** the dominant
  cost class turns out to be `copy` or `bulk side-stream entropy decode` (in which case §4.1/§4.2 apply
  and the token loop is not the target at all).
- **T3 note:** `1.36` is deliberately set at the *smallest true measured decode win in this project*
  (E1/dickens). A mechanism whose ceiling is below the smallest thing that has ever worked here is not
  worth funding.

---

## 9. What would change my verdict

Stated in advance so the coordinator can pre-commit to the reconciliation:

| finding | would move me to |
|---|---|
| F-1 returns T-F1.3 (λ can move decisions) | cost-gated decoder proposals become **PILOT-able**; my B2 blocker downgrades from "inoperative" to "mis-scaled but functional" |
| Job C shows `φ/L ≥ 0.15` (mean token iteration ≤ ~6.7 B) with ≥ 60 % coverage | FLI's decode leg moves from "unmeasurable at L=40" to "measurably worth 20–40 %" — I would **still not endorse a new ISA**, because the finding properly belongs to the representation lanes (Track 09/14), but I would withdraw my objection to the *measurement* |
| F-2 returns T-F2.1 with `≤ −0.30 %` bytes | table-width tiering becomes **PROMOTE-TO-REMOTE** for integration, not just PILOT |
| F-3 shows `C_max ≥ 2.0` on the enwik8-class cell | a decoder-architecture mechanism becomes **PROMOTE-TO-REMOTE**; I had held this at HOLD on the strength of the 3.9× binding deficit (E3) and would be shown to be over-conservative |
| Track 20 anticipates **OpenZL** on the "resolved-graph decoder / decode recipe" separator | this is already my position (NOVELTY NO, §3); no change |
| a proposal demonstrates **≥ 4× decode** on enwik8-class cells within the 7 % paired floor | nothing in the current board does; this would be the single result that overturns my HOLD |

---

## 10. Final ruling

> ## **HOLD** — decoder/backend architecture mechanisms.
> ## **KILL** — the token-lane / entropy-pull / dispatch / superinstruction / small-copy family
> (**measured pools 0.3 % and ≤ 11 %; ceilings 1.003× and 1.124×, both below the 1.364× the project's one
> true decoder win delivered** — §1/B0); CAM/PPM compact model priors as a decoder mechanism (§5.5);
> independent-stream scheduling (E6, measured); rANS lane scaling beyond 4-way on AVX2/Zen 3 (§4.2,
> architectural); further CRC work (E4, residual share 1.3–3.4 %); correction-topology scatter modelling
> (E8, mode-13 lost +13.5…+21.0 %); any novelty claim for the resolved-graph/recipe decoder (OpenZL, §3).
> ## **PILOT (byte/RSS-decided, remote)** — table-width tiering, **reclassified from decode to RSS**
> (survivor 1) and dense-integer side-stream recoding (survivor 2). Neither is novel; both exploit a defect
> this audit documented rather than a mechanism this track invented.
> ## **PROMOTE-TO-REMOTE** — **no Track-18-private CI job.** Shared substrate work is merged into the
> cross-lane queue review (`REMOTE-EXPERIMENT-QUEUE-FLEDGE-REVIEW.md`, SX-1/SX-2/SX-3 in
> `SYNTH-PERF-FLEDGE.md` §6), owned by named tracks.

**Strongest kill argument, in one paragraph.** On the only cell with a complete, verified decode-floor
profile, the largest share any wire-invisible decoder change can remove is **26 %** — and that 26 % is
already proven non-removable (MAT: **+27.6 % slower**) — giving a realistic ceiling of **1.351×**, which is
**below the 1.364×** that `libsais_unbwt_aux` actually delivered. Every architecture proposal currently on
the board names a *smaller* pool: **0.3 %** for entropy pulls (`1.003×`) and **≤ 11 %** for the whole token
loop (`1.124×`). The harness cannot certify any of them: its own **zero-byte-change control variants cost
2.3–8.1 %** and its best paired design resolved **7.4 %** while failing to resolve **1.4 %**. Track 11's
FLI names the 0.3 % pool explicitly — it deletes "one entropy-coded opcode plus one varint n" per loop —
so its decode leg is **arithmetically dead**, and its proposed gate G1 cannot see this because counters
report the **26 %** materialization share on the wrong cell. **The measurement substrate, not the
mechanism, is the binding constraint on this axis.**

**Strongest surviving case, in one paragraph.** Exactly one track-18 direction survives B1 because its
GO/NO-GO needs no clock at all: **per-stream table-width tiering**. A packed rANS-4096 decode table
occupies **100 % of the measured host's 32 KiB Zen 3 L1d**, leaving no co-residency for the hot-op book or
the output window; the ledger has *already measured* rANS-4096 at **5–12 % slower/B than 256/512 on small
streams**; `docs/FRONTIER-RESET-2026-09-23.md` §8.3 and `docs/CONTEXT.md` target #2 both name it; it is
**wire-visible**, so it escapes the exhausted wire-invisible class; and it reuses the already-proven S6-1
per-stream mode-byte machinery. Its byte cost is deterministic and clock-free. That is the highest
expected-value use of one CI cycle in this track — and it is adopt-class, which is the honest label.

**One cheapest decisive next experiment.** Not a new profile: **re-measure the instrument that already
exists** — `prototypes/profile_tmp/prof.cpp` on the current post-CRC build for `generated.log` and the
mode-15 record cells, emitting refreshed stage shares **plus the `φ/L` match-length histogram weighted by
bytes covered** (`SYNTH-PERF-FLEDGE.md` §6 SX-2). One job, one existing instrument, no new generic
decomposition. It is the single measurement that (a) replaces the stale bytewise-CRC profile, (b) closes
the only unattributed line inside the 11 % token loop, (c) prices Track 15's per-block budget and Q2's
geometry arms, and (d) gives Tracks 06/11/15/17/18 a **post-CRC pool denominator** instead of a superseded
one. Its gates: zero-byte-change control **≤ 5 %** and `Σ components` within **35 %** of whole-codec, else
VOID.

**Withdrawn proposal, recorded honestly.** §8's original F-1 λ-recalibration job is **withdrawn as a
Track-18 CI job**: it overlaps Track 06's cost-selection defect, and the S6-1 selector manifest has already
**measured** the answer — **792 selections, ZERO raw-flips, all 139 flips exact-L ties**, every one with
`logical_eq=1 ∧ dec_eq=1`. Combined with the **proof** in B2 (default-path cost range **0.35 < 1 byte** ⇒
exact-length tie-breaker only), the incumbent objective is a **tie-breaker, not a selector**, and a separate
Track-18 job would re-derive a known answer. The remaining live question (a *calibrated* or
size-proportional objective) belongs to the coordinator's queue item **Q3 Stage 1**, which compares the two
**existing** objective forms with **no third λ-fit arm** — I concur with the queue over my own earlier
proposal.

---

*Prepared by Fledge Alpha Free, independent adversarial reviewer, track 18. Independent of the
constructive lane's output; every number re-derived from in-repo artifacts with the location named.
Recommendation is mine and does not inherit any constructive verdict. Reconciled against
`docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md` §9 P1 and §10 G1/G10/S4, against Track 06's
stream-selection-objective finding, and against the coordinator's cross-lane instruction requiring
counter-vs-timing job separation and a paired uninstrumented control for any recalibration experiment.*