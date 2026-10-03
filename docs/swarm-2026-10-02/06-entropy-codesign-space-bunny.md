# Track 06 — Entropy Backend Co-Design — Space Bunny Free (constructive)

**Agent:** Space Bunny Free · **Track:** 06-entropy-codesign
**Date:** 2026-10-02 · **Live tree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty
**Production integration authorized:** NO · **No commits / pushes / resets / stashes**
**Heavy measurement:** GitHub Actions only. Nothing in this report was measured locally.
**Prototype:** `prototypes/swarm-2026-10-02/06-entropy-codesign/space-bunny/ddmc_proto.cpp`
(bounded correctness sketch, **toy output, non-evidence** — see §7.4).

---

## 0. Ruling summary (read first)

| # | candidate | ruling | one-line why |
|---|---|---|---|
| **R1** | **New entropy coder as a frontier-crossing route** (FSE/tANS, novel ANS, adaptive range coder, multi-stream ctx fan-out) | **KILL** | Prior art + arithmetic, not effort: the reference ships **no adaptive entropy coder** (Brotli RFC 7932 = Huffman prefix codes only), ANVIL already ships rANS+suite+ctx-rANS and is still 4× behind on decode; and the decode-only plane is closed on 12/13 files by `DNB-M1` (MATH). |
| **R2** | **Cost-objective correction** (constant per-stream `c` → length-proportional measured `C_us` on the default path) | **PROMOTE-TO-REMOTE**, as the **single shared 05/06/18 arbiter** | Two selectors ship live in `src/anvil.cpp` with **different units**; the calibrated one is reachable from exactly one call site behind a default-off flag whose arbiter never ran. Byte-only falsifier, no mechanism build. |
| **R3** | **AOC** — order-1 context over the *existing* mode-15 opcode stream, zero-ISA | **PILOT** (adopt-class control, byte-only) | Cheapest remaining *byte* lever in this lane; must ride the R2 arbiter, not a new coder; it is a **control**, not a contribution. |
| **R4** | **CAM** — compact transmitted per-block model prior (warm-start without inter-block dependency) | **HOLD** (engineering candidate; not funded here) | Cost side is 1–2 orders of magnitude below the deficit it targets, but the deficit it targets is *outside* this track and its reuse/novelty residue is unadjudicated. Do not bundle with R2/R3. |
| **R5** | **DDMC** — decoder-side model derivation + bank (my own original mechanism) | **HOLD → deprioritized** | I withdraw it as a *crossing* candidate: its saving is per-**block** table amortization, and ANVIL already runs 256 KiB blocks with only ~22 streams/block ⇒ the denominator is ~0.2 KB/block (projection, §5.3). Reported for completeness. |

**No HOLD is issued against the track's information work.** The one thing I will fund is R2, and
it is cheap enough that a clean kill is a success.

---

## 1. Evidence map — measured vs projected (labels are binding)

### 1.1 Measured, cited, and not mine (inherited from the ledger/prior lanes)

| # | Fact | Source |
|---|---|---|
| F1 | rANS became the perf backend on an **8.1× decode win for +1.13 % bytes** (DP+rANS 11,232 B vs DP+arith 11,107 B; ~20.16 → ~163.72 MB/s). Adopted 2019-era. | `RESEARCH_LEDGER.md` Exp. D:34-40 |
| F2 | 4-way interleaved rANS = **~30 % decode speedup**; the rANS decode inner loop is deliberately ILP-tuned. | `docs/CONTEXT.md` (rans2x4 entry) |
| F3 | Context-switched rANS (mode 6): **−8.3 %…−16.2 % bytes** on the four record files, decode **−3 %…−10 %**. Recorded verdict verbatim: "RATIO mechanism, not the throughput primitive… **No Pareto claim**." | `RESEARCH_LEDGER.md` Exp. S:1480-1523 |
| F4 | Context clustering is **explicitly NOT novel** — Brotli RFC 7932 §7 maps decoded literal context to prefix trees via a compact context map. Recorded in *this repo* as a correction of an over-claim. | `RESEARCH_LEDGER.md` Exp. J:630-639 |
| F5 | ctx-rANS SQLite: depth-3 ≈ 379.3 KB vs brotli q4 ≈ 422.1 KB (ratio headroom) but **decode collapses to ~0.6–0.74 GB/s**. | `RESEARCH_LEDGER.md` Exp. J:640-648 |
| F6 | Context-switched **table-Huffman/direct** variant **REJECTED**: ~143.8 KB at K=12 (same size) but context-dependent prefix machinery is **slower** than clustered rANS once model/table setup is counted. | `RESEARCH_LEDGER.md` Exp. J:645-648 |
| F7 | Stream-suite J-selection: J-faithfulness **100 %** at λ=0.01 over ~1800 streams; ratio neutral-or-better (0.1278 vs 0.1279); `EXTENDS_FRONT` **NOT cleared**. | `RESEARCH_LEDGER.md` Exp. L:1036-1100 |
| F8 | λ=0 / 0.01 / 0.04 rows are **byte-identical on the corpus** — "the time term ≤0.16 B/stream never overrides the size winner". | `RESEARCH_LEDGER.md` PART IV:1163-1165 |
| F9 | **Exp. AA is INCOMPLETE** — whole-codec J + raw-stream budget landed, controls green, **formal bench arbiter NEVER RAN**; no bar verdict, no EXTENDS_FRONT. | `RESEARCH_LEDGER.md` Exp. AA:3785 |
| F10 | Measured decode calibration (median-7, ns/B, real mode-15 content): raw 0.04–0.10 bulk / 2.59–2.66 byte; rANS-4096 5.93–6.62; rANS-512 5.44–6.55; rANS-256 5.40–5.99; Huffman 4.08–9.0 (data-dependent); defexc 3.14–3.25; ctx-rANS pull ~7–8 / materialised 4.6–5.3. Incumbent verdict: **"right ORDER, wrong GAPS."** | `RESEARCH_LEDGER.md` Exp. AA:3883-3904 |
| F11 | `DNB-M1` (MATH-class): decode-only Route-B crossings **closed on 12/13** files. Only live cell `synth-timeseries.bin` at 1.8×. Byte-identical decoder ceiling ≈ **1.79×**. | `RESEARCH_LEDGER.md` §3:4223-4276 |
| F12 | `DNB-M3`: any non-dominated row at ratio **≥ 0.95 is DEGENERATE** — live on `synth-arith.bin`, `random.bin`. | `RESEARCH_LEDGER.md` §4:4284-4287 |
| F13 | `generated.log` already carries **22 entropy streams**; largest `distance_code` 15,680 B, `match_len_class` 8,788 B. | `docs/CONTEXT.md` (log stream anatomy) |
| F14 | Linux verdict on spending rate surplus for speed: the dominant trade is storing the shape-distance-delta stream **raw** (~19.8 KB) raising hot-book decode ~0.87 → ~0.99 GB/s; "~60 KB rate surplus = explicit compute budget". | `docs/CONTEXT.md` (hot-op hybrid sweep) |
| F15 | Reciprocal-rANS division replacement **measured SLOWER than hardware divide**. Do not re-burn. | `docs/CONTEXT.md`; `docs/audit-2026-09-07/06-do-not-reburn.md` D3 |
| F16 | Dual-bar rule: synth cells must be reported vs raw **and** transform-enabled reference bars; raw-bar-only wins are struck. | `RESEARCH_LEDGER.md` §5b:4305-4335 |
| F17 | GH Actions grading: bytes/ratio/hash **citation-grade**; MB/s and RSS **scout/ranking-grade**, paired same-job only; small shared-runner deltas are inconclusive. | `docs/GITHUB-ACTIONS-BENCHMARKING.md` §2,§3,§9 |
| **F18** | **Mode-15 decode floor profile (log): crc32 44 % → eager materialisation of the 7 macro streams 26 % (setup IS masks+resid) → token loop beyond setup 11 % (opcode entropy pulls 0.3 %) → concat/alloc/headers ~15 %.** Per-stream raw-storage: macro-masks **+21–27 % decode for +166,823 B**; macro-resid **+18–21 % for +84,956 B**; macro-dvar already raw (~1.2 cyc/B). "**THE HOT PATH IS CLEAN; the floor is everything AROUND it.**" Noise band: zero-byte-change control variants measured **+2.3–8.1 %**. | `RESEARCH_LEDGER.md`:3911-3936 |
| F19 | Zero raw-flips were **arithmetically certain** at λ=0.01: decode term spans ~0.02–0.66 B on 10–20 KB streams; first raw-flip candidate only at λ ≈ **44.6 B/μs**. | `RESEARCH_LEDGER.md`:3902-3910, 4002-4016 |

### 1.2 Code-verified by me this session (read-only; `src/anvil.cpp`, 5,034 lines)

| # | Fact (verified, not projected) | Location |
|---|---|---|
| **C1** | The **default** selector `encode_stream` scores candidates `J = L + λ·c + μ·2 + ν·c` with `c` a **literal constant per candidate**: raw 10, defexc 20, Huffman 22, rANS-256 30, rANS-512 35, rANS-4096 40, ctx-rANS 45. **There is no stream-length term in `C_decode` anywhere on this path.** | `src/anvil.cpp:1548-1564` (esp. `:1553`) |
| **C2** | A **second** selector ships: `encode_stream_budget`, `C_us = src.size() * kBudgetNsPerByte[codec]/1000.0` — **length-proportional, measured ns/B** `{0.1, 6.0, 6.0, 6.0, 4.3, 3.2, 7.5}`. | `src/anvil.cpp:1628-1653` (table at `:1618-1626`) |
| **C3** | `encode_stream_budget` is called from **one** site, `:2868`, under `g_hotop_budget`, default **false** (`:1617`, `:3786`, `:4664`). Flag parse at `:4979` turns budget **ON for any value ≠ `"off"`** — a silent cost-model switch for any driver passing `--hotop-budget=1`. | `src/anvil.cpp:1617,2868,3786,4664,4979` |
| **C4** | All other stream emitters use constant-`c` `encode_stream`: modes 10-13 substreams, modes 14/15, RLZ/RePair inner codecs, and the size-estimation pass. `encode_stream_smallest` (ratio-only) is a third objective. | 11 call sites: 1720,1770,2161,2234,2500,2642,2868*,3169,3438,3605 (+`:1587`) |
| **C5** | Both selectors **early-return raw when `src.size() < 16`** ⇒ a ≤15 B stream can never be coded and always pays a `[mode=0][uvar len]` header (up to **13.3 % overhead** at 15 B). | `src/anvil.cpp:1550,1630` |
| **C6** | ctx-rANS (mode 6) is only a candidate when `src.size() >= 4096` ⇒ F3's ratio mechanism is **structurally unavailable** below 4 KB per stream. | `src/anvil.cpp:1568,1649` |
| **C7** | **Wire cost of ctx-rANS model is unconditionally 257 B** before per-class data (1 B `K` + 256 B map), then `Σ_classes (uvar nz + Σ(1 B sym + uvar freq))` ⇒ min ≈ `259 + 3K` B (K=12 ⇒ ~295 B). | `ctx_stream_bytes`, `src/anvil.cpp` |
| **C8** | **Wire cost of Huffman model is unconditionally 258 B**: 256 length bytes *always*, regardless of symbol count. | `huffman_stream_bytes`, `src/anvil.cpp` |
| **C9** | rANS models serialize **sparsely** (nonzero symbols only) ⇒ `4 + 2s` B min, ~516 B worst case at s=256. Strictly cheaper than Huffman/ctx on skewed streams. | `rans_stream_bytes`, `src/anvil.cpp` |
| **C10** | `μ = ν = 0` in shipped defaults, so the `2.0` framing term is **inert**; the live default objective is exactly `J = L + 0.01·c`. | `src/anvil.cpp:1471-1472,1553` |
| **C11** | `FORMAT.md` documents the **same** length-independent `J = L + λ·c + μ·2 + ν·c` with the integer `c` table ⇒ **docs and default code agree**; the size-proportional form is documented separately for the budget path only. | `FORMAT.md:639-660, 803-819` |
| **C12** | Mode 15 emits **9 book streams** + a book; the fused decoder materialises 7 macro streams **eagerly** and pulls only opcodes+literals. | `src/anvil.cpp:3012-3029` |

### 1.3 Projections (mine; explicitly not measurements)

| # | Projection | Basis |
|---|---|---|
| P1 | Per-block model-header cost ≈ **5.6 KB** at 256 KiB blocks on the 9-stream mode-15 layout (9 streams × ~620 B avg model) ⇒ **~1.6 %** of a 350 KB/block-scale output. | C9 × C12 × block count from `:3769`. Arithmetic only. |
| P2 | In `kBudgetNsPerByte`, **rANS-4096/512/256 are all exactly 6.0** ⇒ they are **exact J-ties** under the budget objective, broken by strict `<` to earliest-added (4096). | `:1618-1626`, `:1651-1652`. Reads off the table. |
| P3 | AOC (order-1 over opcodes) has an addressable redundancy of the size of the **opcode context entropy**, i.e. `H(op) − H(op|prev_op)`; the recorded opcode-pull *share* (0.3 %) bounds its decode upside but **not** its byte upside. | F3/F18; needs the byte-only dump. |

---

## 2. Reconciliation of the cost-objective discrepancy (the constructive core)

### 2.1 The critic's finding, independently re-derived — **CONFIRMED with one correction**

Critic §3.3 (H-2): default `C_decode` is length-independent and mis-prices decode cost by
1.7×–51×. I re-derived it from C1/C2 and **agree**, with the following sharpening and one
correction.

**Derivation (mine, arithmetic only).** Shipped term: `Δ_shipped = λ(c_max − c_min) = 0.01·(45−10) = 0.35 B`
for the full spread, or `0.01·(40−10) = 0.30 B` for raw→rANS-4096. Calibrated term:
`Δ_prop = λ·N·(ns_4096 − ns_raw)/1000 = 0.01·N·(6.0−0.1)/1000 = N·5.9e-5` B.

Crossover: `N* = 0.30 / 5.9e-5 ≈ 5,085 B`. Below `N*` the shipped term **over**-prices decode
(over-favours raw); above `N*` it **under**-prices it (under-favours decode speed, i.e. it prefers
smaller-but-slower codings on exactly the streams that carry the decode budget).

| N | shipped Δ (B) | calibrated Δ (B) | shipped is | direction |
|---:|---:|---:|---:|---|
| 100 | 0.300 | 0.0059 | **51× heavy** | over-favours raw |
| 512 | 0.300 | 0.030 | 10× heavy | over-favours raw |
| 2,048 | 0.300 | 0.121 | 2.5× heavy | over-favours raw |
| **5,085** | 0.300 | 0.300 | **equal** | only length where they agree |
| 15,680 (F13 `distance_code`) | 0.300 | 0.925 | 3.1× light | under-favours decode |
| 100,000 | 0.300 | 5.90 | 19.7× light | under-favours decode |

**My correction to the critic (material, and it cuts against my own lane's framing).** The
critic writes that at λ=0.01 the shipped term "systematically prefers smaller-but-slower codings".
F8 and F19 say something stronger and *measured*: **on this corpus the term never changes the
winner at all** — λ=0, 0.01 and 0.04 produce **byte-identical** output, and the decode term spans
0.02–0.66 B on 10–20 KB streams, with the first raw-flip candidate arriving only at
λ ≈ 44.6 B/μs. So the default constant-`c` term is not "actively mis-selecting"; it is
**inert-but-wrong-by-construction**. That distinction matters for the remedy:

> Raising λ is the wrong remedy and would make small-stream mis-pricing *worse*. The remedy is
> the **shape** of `C_decode` (length-proportional), not its magnitude.

Second correction: F10 grades the calibration table **"right ORDER, wrong GAPS"**. The budget
table then **collapses** rANS-4096/512/256 to a single 6.0 ns/B (P2). So the "measured" form is
not more faithful than the constant form in the dimension that matters (separating the three rANS
precisions); it is better on the **raw↔coded** axis (a 60× vs 4× dynamic range) and *worse* on the
**precision** axis (zero resolution). That is a real, previously unstated trade and it is what the
arbiter in §3 must measure rather than assume.

### 2.2 Are the default constants *intentionally* per-stream? — **Yes, documented; but the documentation is incomplete**

`FORMAT.md:639-660` documents `J = L + λ·c + μ·2 + ν·c` with an integer **cost-unit** table and
calls `c` a "per-byte decode-cost unit" (`FORMAT.md:642-643`). The source comment calls `cu`
"per-stream decode cost units" (`src/anvil.cpp:1468-1471`). So:

- **Intentional and self-consistent:** yes. `c` is a dimensionless *ordering* weight whose scale is
  absorbed into λ. `μ·2` models the fixed substream framing; `ν` is reserved.
- **Objectively defective in two ways:**
  1. **Unit incoherence under λ.** `λ` has documented units B/μs (`0.01 B/μs`). If `c` were
     "per-byte decode-cost units", `λ·c` would be B/μs·(1/μs) — not bytes. It is only a *byte*
     term if `c` is "μs of decode per stream", which then makes the term length-independent by
     construction. Either reading is defensible in isolation; the two cannot both be true, and
     the arbiter must fix **one** and restate `FORMAT.md`.
  2. **Two live objectives in one binary with no arbiter.** C1/C2/C4 ship both, reachable
     independently, with **different units**, and the calibrated one never had its formal run (F9).
- **One labelling bug, code-verified:** `FORMAT.md:642` calls `c` a *per-byte* cost unit; for
  modes 10-13 the value is charged **once per stream**, not per byte. The doc and the code agree
  with each other but the unit word is wrong.

### 2.3 Does a cost-corrected selector change actual stream choices or complete bytes? — **Unknown, and that is the falsifier**

Three mutually exclusive possibilities, only one of which is interesting:

- **H-A (F8/F19 world):** argmin is invariant ⇒ complete bytes identical ⇒ the defect is
  documentation/hygiene only ⇒ **KILL the workstream as a compression contribution**.
- **H-B:** argmin changes on a minority of streams but the **complete** byte total is unchanged
  (tie-flips) ⇒ promote as a **correctness-of-experiment fix** (every J-gated claim in tracks
  05/11/18 must be re-derived) but **not** as a contribution.
- **H-C:** argmin changes and complete bytes move by ≥1 % ⇒ promote as an engineering correction
  with a byte leg — the only branch that survives the R2 Pareto bar on its own.

I hold **no position** on which branch is live. Deciding it is one byte-only CI job.

### 2.4 Do not build a third selector. A justifiable corrected scale, stated without post-hoc fitting

Any arm beyond {constant-`c`, size-proportional} risks being fitted to the corpus, which
doctrine item 5 forbids. The only scale change I can justify **a priori** is a **global λ
recalibration to preserve the current large-N ordering**, namely:

> choose λ' such that `λ'·(c_4096 − c_raw)` equals the calibrated gap at the **corpus-median
> stream length** N_med — one number, read off the byte-only stream dump *before* any arm is run,
> frozen in the artifact, never re-tuned.

This is **scale calibration, not re-ranking**: it preserves the constant table's ordering at the
length that actually dominates decode time, and it is falsifiable by "does the re-ranked choice
match the size-proportional ranking at N ≥ N*?" I commit to **not** adding a fourth candidate
set and **not** touching `FORMAT.md` constants before the arbiter reports.

---

## 3. ONE shared remote arbiter for Tracks 05 / 06 / 18

Coordinator merge instruction: a single arbiter, not an entropy-lane job. Tracks 11 and 18
independently found the same defect; Track 18's critic found instrumentation can perturb decode
by 2.3–8.1 %. Everything below is **one** pre-registered job.

### 3.0 Already-measured prerequisite — do **not** re-profile

`RESEARCH_LEDGER.md`:3911-3936 (F18) already decomposes mode-15 decode on log: crc32 44 %,
eager macro-stream materialisation 26 %, token loop 11 % (of which opcode entropy pulls 0.3 %),
concat/alloc/headers 15 %. **Consequence: re-measuring opcode-pull share is redundant and is
struck from this lane.** Three further items are already answered there and must not be re-run:

- raw-storage trades for macro-masks/macro-resid are **measured** (+21–27 % / +18–21 % decode for
  +166,823 B / +84,956 B) and macro-dvar is **already raw** (~1.2 cyc/B) — the Linux "+19.8 KB ⇒
  +0.12 GB/s" trade does **not** transfer (F18);
- CRC is already a retained real win (PCLMUL); the 44 % slice is addressed upstream of me;
- the noise band is **+2.3–8.1 %** for zero-byte-change controls (F18), which sets the
  sensitivity floor for every timing arm below.

### 3.1 Arm definitions (frozen before dispatch)

| arm | selector | flags | wire |
|---|---|---|---|
| **A0 (control)** | constant-`c` `encode_stream` — today's default | `--hotop-budget` **absent** | baseline |
| **A1** | size-proportional `encode_stream_budget`, **only** at the existing single site | `--hotop-budget=off` is A0; budget arm must pass the flag **explicitly and recorded verbatim in the artifact** | budget |
| **A2** | constant-`c` with λ′ from §2.4 | `--hotop-budget=absent` + recorded λ′ | cal-λ′ |
| **A3 (optional, R3)** | A0 selector + **AOC** order-1 opcode context (zero-ISA, new substream mode id) | recorded | aoc |

Provenance hazards that must be recorded per arm, not assumed (C3, C5, C6, F19):
exact CLI string, `j_agree/j_total`, block count, stream count, and the byte-only stream dump.

### 3.2 Stage 1 — byte-only, citation-grade, no build required

Dump per stream: `(stream_index, block_index, role, N, chosen_codec, L_c for every candidate,
J_const, J_prop, J′)`. Then report, deterministically:

1. **argmin disagreement rate** `argmin[L+0.10·c] ≠ argmin[L+N·ns·1e-5]`, overall **and by
   length bucket** (`<16`, `16-512`, `512-4096`, `4096-16384`, `≥16384`).
2. **complete-bytes delta** per arm: `Σ_block (Σ_stream L_chosen)` — i.e. what actually ships.
3. **per-codec flip counts** and **whether any flip is a precision flip (4096↔512↔256)** — this is
   the P2 degeneracy check.
4. **stratified J-faithfulness** (`src/anvil.cpp:1574-1578` already computes it; re-emit per
   bucket), with the `<16 B` bucket reported separately because C5 locks it to raw and **cannot**
   falsify J — this is the attack on F7's unstratified 100 %.
5. **AOC stage-1 measurement** (R3): `H(op)` vs `H(op|prev_op)` over the *existing* opcode stream,
   byte-only. No decode timing in stage 1.

### 3.3 Stage 2 — paired timing, **only above the sensitivity floor**

Per F17/F18, timing is scout/ranking-grade and only interpretable as a same-job paired A/B:

- **C1 clean paired control:** every pair runs twice in the same job — both arms counters-OUT,
  both arms counters-IN. Report `tax_arm` for each.
- **C2 tax budget:** admissible iff `tax ≤ 2 %` on both arms **and** `|tax_A − tax_B| ≤` same-job
  CV. Otherwise the decomposition is **non-attributive** and may not enter any gate.
- **C3 byte-identity under instrumentation:** SHA-compare counters-OUT vs counters-IN on every
  file; any mismatch ⇒ **VOID**.
- **C4 partition closure:** the decomposition must *close* against output bytes (sum of token-loop
  + materialisation + concat/alloc/headers = 100 %). A list is not a decomposition.
- **C5 closure test:** predicted cycles from (counts × measured ns/B) vs measured whole-codec
  decode; `|predicted − measured| > 20 %` ⇒ VOID. **This is the falsifier that the Exp-AA table
  ("right ORDER, wrong GAPS") has never had to pass.**
- **C6** no per-region timers as primary; **C7** stamp stage-2 output scout-grade in the artifact
  header so no downstream brief promotes it.

### 3.4 Frozen thresholds (fixed now; not moved after data)

| gate | PROMOTE | KILL |
|---|---|---|
| **K1 defect materiality** | argmin disagreement **≥ 10 %** of streams on **≥ 2** corpus files | **≤ 2 %** everywhere ⇒ defect immaterial on real content ⇒ KILL the workstream as a compression contribution; keep as hygiene |
| **K2 band** (2 % < rate < 10 %) | **≥ 5 %** of disagreement inside `16-512` or `512-4096` ⇒ PROMOTE as *experiment-correctness* fix (all J-gated claims re-derived), **not** as a contribution | disagreement confined to `<16 B` ⇒ immaterial, KILL |
| **K3 completeness** | **complete-bytes delta ≥ 1.0 % on ≥ 2 of 4 record files** in the winning arm | bytes delta < 1 % on all ⇒ no byte leg ⇒ record the selection change as a decode-policy finding only |
| **K4 F7 stratification** | J-faithfulness `< 95 %` in `16-512` while `≥ 99 %` in `≥4096` ⇒ the "100 %" headline is stratification-inflated; Exp. L / C7 must be **restated** with the bucket table | `≥ 99 %` in every bucket ⇒ F7 survives; no restatement |
| **K5 attribution** | C2 tax ≤ 2 % both arms, `|Δtax| ≤` CV, C3 byte-identity, C5 closure within 20 % | any failure ⇒ stage-2 timing is **non-attributive**; the selector verdict rests on stage-1 bytes alone |
| **K6 new entropy coder (any)** | must cut bytes **≥ 1.0 % on ≥ 2 of 4 record files at ≤ 2 % decode regression**; a decode-only argument is **pre-refused** by `DNB-M1` (F11) and a degenerate-cell argument is **void** by `DNB-M3` (F12) | decode-only or degenerate ⇒ immediate NO-GO, no measurement |
| **K7 AOC** (R3) | byte-only `H(op)−H(op|prev_op) ≥ 0.5 %` of complete bytes on **≥ 2** record files, achieved at **0 new ISA semantics** and ≤ 1 new decoder-visible substream mode id | < 0.5 % everywhere ⇒ KILL as a byte lever; record the opcode context entropy as a measured negative |

---

## 4. Candidates, classified

### 4.1 R1 — New entropy coder as a crossing route — **KILL**

**Prior-art + arithmetic kill, not an effort judgement.**

- **Prior art.** rANS/ANS: occupied (adopted here, F1). **FSE/tANS: occupied and already shipped
  by a reference** (zstd, RFC 8878, for LL/ML/OF) — adopting it is a lateral move against a
  *non-reference*, worth revisiting only for a byte win. Context-mapped literals: occupied by
  Brotli RFC 7932 §7 (F4). Context-switched table-Huffman: **measured REJECTED here** (F6).
  Reciprocal-rANS: **measured slower** (F15). Range/Fenwick decode: re-burns Exp. A.
- **Arithmetic.** The reference, Brotli, ships **Huffman-derived prefix codes only** — no
  arithmetic/rANS/ANS/FSE coder. ANVIL already ships rANS + a 7-codec adaptive suite +
  context-switched rANS and is nonetheless ~4× behind on decode and behind on bytes
  (`RESEARCH_LEDGER.md` PART IV:1153-1180). **The deficit is therefore not located in entropy
  coding.** Independently, the only axis a new backend could improve alone — decode throughput —
  is **arithmetically closed on 12/13 files** by `DNB-M1` (F11, MATH-class), and the single live
  cell (`synth-timeseries.bin`, 1.8×) is degenerate-flagged by `DNB-M3` (F12).
- **Anti-overclaim correction (binding on this report).** The above says the *deficit* is not in
  entropy coding and that *coder-family substitution* cannot cross. It does **not** say entropy
  coding is irrelevant to ratio. F3 (−8.3…−16.2 % bytes) and F5 (ctx-rANS at 379 KB vs q4 422 KB
  on SQLite) are measured ratio wins from the entropy layer, and F6 shows a *different* entropy
  choice at the same ratio was slower. So: **modeling/coder layout demonstrably moves bytes**;
  what is closed is the claim that a *better coder family* is the missing axis. Any brief that
  says "entropy cannot matter" is overclaiming in the other direction and is struck.

### 4.2 R2 — Cost-objective correction — **PROMOTE-TO-REMOTE** (shared arbiter, §3)

- **Engineering correction, not a mechanism.** It changes *which already-implemented coder is
  chosen*, never the candidate set, never the decoder ISA, never the stream inventory.
- **Why it is legitimately fundable:** two selectors ship live with different units (C1/C2/C4); the
  calibrated one is reachable from one site behind a default-off flag whose arbiter never ran
  (F9); and the mis-scaling has a closed-form sign flip at `N* ≈ 5,085 B` (§2.1) that means the
  shipped objective is biased *against* the very "rate surplus buys decode speed" strategy that
  Tracks 05/11/18 and F14 are built on. Fixing it either rescues or kills those proposals; the
  information is worth one job.
- **Novelty status: NONE CLAIMED.** "Selection objective weighted in bytes-per-microsecond" is an
  engineering policy. zstd's `Compression_Mode` and RFC 7932 block types select on **ratio**;
  nobody publishes a μs-weighted objective, but absence-of-prior-art is not a mechanism claim and
  must never be reported as one.
- **Separation of engineering from novelty, stated once:** R2's *value* is calibration hygiene.
  Its *only* possible contribution is a byte move (branch H-C), which would be an
  adopt-class improvement, not a new mechanism.

### 4.3 R3 — AOC: order-1 context over the existing mode-15 opcode stream — **PILOT** (adopt-class control)

Per the cross-lane merge instruction, AOC belongs here, not as a parallel Track-11 mechanism, and
it is a **control**, not a contribution.

- **Mechanism.** Context the opcode stream on the previous decoded opcode: transmit a 256×256
  (or sparse) order-1 table — or, cheaper, transmit the previous-block order-1 table as a **prior**
  and let the current block's own histogram refine it. Decoder cost is **one extra indexed load
  per opcode** on an already-serial chain, and **zero new ISA semantics**: the opcode value is
  already decoded by the existing pull (`src/anvil.cpp:3035-3050`), so the context bit is free.
- **Explicitly not a new coder.** It is a context choice over the incumbent rANS. It must ride
  the R2 arbiter (stage-1 byte-only, K7), never a new backend (K6), and must never be bundled
  with the rejected multi-stream ctx fan-out: per the C6/C7 arithmetic, ctx-rANS on 9 book
  streams would cost `9 × ~295 B ≈ 2.65 KB` of decoder-visible model **per block**, which is
  exactly the fan-out that Exp. S rejected. AOC's wire is a *single* sparse table on a *single*
  stream — the "one physical stream, NOT multi-stream fan-out" discipline (F3).
- **Relationship to FLI / DDMC:** FLI (Track 11) is a superinstruction that *removes* opcode
  draws; AOC makes the remaining draws cheaper to code. They are complementary, not competing;
  if FLI lands first, AOC's addressable base shrinks and its byte case must be re-measured on the
  post-FLI opcode stream, not on the incumbent one.
- **Novelty status: adopt-class.** Conditional-order coding on an existing stream is textbook and
  is what Brotli's context maps and virtually every production entropy stage already do (F4).
  The honest claim is "zero-ISA, no new decoder-visible coder, measurable byte delta".

### 4.4 R4 — CAM: compact transmitted per-block model prior — **HOLD**

- **What it is (Track 15 §8.1):** block *i* carries a **compact transmitted frequency prior inside
  its own payload**, applied before decoding block *i*'s symbols ⇒ **zero inter-block dependency,
  zero prefix requirement, zero cross-block identifier space**. Blocks stay independently
  decodable and random-accessible.
- **What could be summarised in 64–256 B (entropy-lane answer, my analysis).** From this lane the
  only decoder-visible entropy state that is *per-block and cold* is: (a) each rANS stream's
  normalized `freq`/`start` arrays (C9: `4 + 2s` B, up to 516 B at s=256); (b) the ctx-rANS
  256 B context map + K class tables (C7: ~295 B at K=12); (c) the Huffman 256 length bytes (C8).
  A CAM prior is therefore most naturally a **top-N quantized frequency sketch over the block's
  own streams** (e.g. 32 symbols × (1 B sym + 2 B quantized freq) = 96 B per stream) that seeds
  the *initial* model, with the exact table still transmitted if the seed cannot be normalized to
  the required `TOT`. **Critical caveat:** for a static rANS stream the decoder's *model is the
  transmitted table*, so a prior only helps if the format permits a **transmitted seed plus a
  transmitted correction** — i.e. it is a *model-framing* change with a new decoder-visible
  field, registered in `FORMAT.md`, with the strict-rejection surface enlarged accordingly.
  It does **not** seed a ctx map or MTF model without a new decoder-side derivation step.
- **Amortization (Track 15 §8.2, arithmetic, theirs):** on Silesia at 256 KiB blocks (809 blocks,
  base 46,446,995 B), a 64 B prior costs 51.8 KB = **0.112 %**, a 256 B prior 207 KB = **0.446 %**
  of complete bytes, versus E4's measured 4 MiB-block warmup tax of **5.18 %**. So the cost side
  is 1–2 orders of magnitude below the deficit — *if* the warmup tax transfers to a 256 KiB
  block configuration, which is **not** established (E4's tax was measured at 4 MiB).
- **Why HOLD and not PILOT here.** (i) The deficit CAM targets is a **cross-block** phenomenon
  owned by Track 15 and gated on E4/E15-0 re-opening **on model carryover**; (ii) its novelty
  residue is explicitly **[U] not claimed** by its own author and must go to Track 20 before
  naming; (iii) it enlarges the decoder-visible model surface, which is exactly what this lane's
  C7/C8 arithmetic shows is expensive; (iv) **bundling it with R2/R3 is forbidden** (Track 15
  §8.4 exclusion discipline). **From the entropy lane the only defensible statement is:** CAM is
  cost-feasible and independence-preserving; it is not yet authorized, and if authorized it must
  be arbitered on the same Stage-1 byte-only substrate as R2/R3.

### 4.5 R5 — DDMC: my own mechanism, and why I withdraw it as a crossing candidate

**Mechanism as sketched (`prototypes/.../space-bunny/ddmc_proto.cpp`).** Replace the per-block
rANS model header with a 1-byte reference into a decoder-resident bank of quantized tables; the
encoder derives candidate tables from symbols the decoder already holds (a replica-interleaved
symbol histogram maintained during decode, so no re-scan of the output), so a table is
*transmitted once per distinct quantized model* rather than once per block. Encoder and decoder
share one integer `normalize()` (floor + largest-remainder, `O(σ log σ)`), so derivation is
bit-identical on both sides; bank occupancy is a bounded ring (`BANK_SLOTS = 8`), decoder-visible
and validated before use.

**Why I deprioritize it (self-refutation, recorded because the arithmetic is the useful part):**

1. **The denominator is small.** ANVIL already runs 256 KiB blocks (`src/anvil.cpp:3769`), with
   ~9–22 streams per block (C12, F13). Per-block table cost is on the order of
   `9 × ~620 B ≈ 5.6 KB` (P1) against a per-block payload of hundreds of KB ⇒ **~1.6 %** of output
   at absolute best, and only for *repeated* tables across blocks.
2. **Repeat rate is unmeasured and plausibly low.** rANS tables are built from *per-block* counts;
   with 256 KiB blocks, `nz ≈ 256` and quantization noise dominates, so two blocks' tables are
   **not** bit-identical and the bank misses. My toy self-test (§7.4) shows D/B ranging from 1.0
   (drift) down to ~0.1–0.25 on stationary regimes — i.e. the entire mechanism's value hinges on
   a repeat rate nobody has measured on real corpus content.
3. **It is not novel in substance.** Transmitted-table reuse across blocks is what zstd's repeat
   mode does; deriving a model from already-decoded data is what every adaptive coder does.
   The narrow residue (content-addressed interning of *quantized static* tables) is engineering.
4. **The decoder-visible cost it *adds* is exactly what this lane's own arithmetic flags:** the
   mechanism trades wire bytes for **per-symbol derivation work on the rANS serial chain**, which
   F2/F18 identify as the ILP-critical path.

**Kept, because it is a cheap discriminator:** the bank/repeat-rate census is a **byte-only**
Stage-1 addition (dump, per block, the hash of each rANS `freq` array; report distinct-count per
stream role). If distinct-per-block ≈ 1 on real content, DDMC is dead on arithmetic and no build
is warranted. **This is the single cheapest test in the whole lane** and it is folded into §3.2.

---

## 5. Cost model — bytes, cycles, RSS, table/state (complete)

### 5.1 Byte cost per stream header (code-verified, C7/C8/C9)

| codec | decoder-visible header | formula | note |
|---|---:|---|---|
| raw | **2 B** (len ≤ 127) | `1 + uvar(N)` | always a candidate |
| rANS-256/512/4096 | **`4 + 2s` B**, ≤ ~516 B | sparse nonzero-symbol table | cheapest real coder on skewed streams |
| Huffman | **258 B always** | 1 + uvar + **256 length bytes** | pays 256 B even for a 3-symbol stream |
| defexc | `3 + uvar + N/8` | mask + exceptions | `O(N/8)` mask even at low exception rate |
| ctx-rANS | **`259 + 3K` B**, ~295 B at K=12 | **256 B unconditional map** + per-class tables | — |

### 5.2 Cycle cost of the objective itself (why a pure selector change cannot move decode much)

The selector runs **encoder-side only**; the decoder executes whatever codec the stream byte says.
So a selector change can only move decode **through its effect on chosen codec and stream
layout**, and the ledger already prices the available levers on real content (F18):

| lever (measured) | decode effect | byte effect |
|---|---|---|
| CRC → PCLMUL | retained real win, wire-invisible | 0 |
| ALLOC-only (decode straight into output slot) | **1.352×** on generated.json mdl | 0 (identical wire) |
| decode leg 4 (buffered renorm + two-level cumulative) | **1.179×** aggregate | 0 |
| macro-masks → raw | +21–27 % decode | **+166,823 B** |
| macro-resid → raw | +18–21 % decode | **+84,956 B** |
| macro-dvar → raw | already raw | 0 |

Aggregate arithmetic consequence (**mine, derived**): a completed selector change could at most
capture a fraction of the two raw trades; those two charges alone are **+251,779 B**, which on a
175,550 B log-scale output class is far larger than any header saving this lane can offer. **So
the realistic value of R2 is (a) removing a bias that five tracks' pending claims rest on, and
(b) re-deriving F14's "~60 KB rate surplus = compute budget" statement, which on the arithmetic in
§2.1 was priced at 0.30 B where the true delta is ~1.17 B (3.9× under-charged).**

### 5.3 Decoder state / RSS accounting for the items this lane could add

| item | decoder-visible state | per block / per stream | bounded? |
|---|---:|---|---|
| AOC order-1 table | `256×256` full = 64 KiB, or sparse top-N ≈ 96–512 B | sparse only; full form is **refused** | yes if sparse-only; **no** if full |
| ctx-rANS per stream (rejected fan-out) | ~295 B | **9 × 295 B ≈ 2.65 KB** | yes — and this is the number that refutes fan-out |
| CAM prior | `+P` B, P ∈ [64, 256] | per block | yes, transmitted, validated pre-alloc |
| DDMC bank | `8 × (256 freq + 256 start) × 2 B ≈ 8 KB` + 8 × 4096 B symtab **on build** | persistent across blocks | yes; bank is fixed-size |
| R2 selector change | **0 B**, 0 state, 0 ISA | — | — |

RSS note: the ledger's own reference rows carry peak-RSS ratios of 2.0×–9.1× on the BWT route
(`RESEARCH_LEDGER.md` A16/L4766+), so any lane proposal must report RSS explicitly; every item
above is **O(1) in input size**, which is the strongest available answer to the bounded-allocation
mandate.

---

## 6. Adversarial failure cases (for each live candidate)

| id | failure mode | expected behaviour / required control |
|---|---|---|
| **AF-1** | Flag asymmetry: `--hotop-budget=1` silently switches cost models mid-experiment (C3). | Every arm's **exact CLI string** is recorded in the artifact; a missing arm record ⇒ VOID run. |
| **AF-2** | Selection confound: counters observe a codec mixture the mis-scaled J already produced, so they describe the incumbent and cannot answer "what would a corrected model choose?" | Resolve by construction: **stage 1 is byte-only and evaluates both objectives on the same candidate set** (C1/critic §7.2 item 5). |
| **AF-3** | Instrumentation perturbs decode by 2.3–8.1 % (Track 18 critic; F18's own control band), or *asymmetrically* between arms. | C1/C2: clean paired control + `tax ≤ 2 %` both arms + `|Δtax| ≤` CV ⇒ else non-attributive (K5). |
| **AF-4** | Codegen perturbation: an extra live value in an ILP-tuned, register-pressure-bound rANS loop spills a state and destroys the 4-way interleaving win (F2) — a **multiplicative**, not additive, change. | C3 byte-identity + differential microbench only; no per-region timers as primary (C6). |
| **AF-5** | Circularity: the cost table used to convert counts→cycles is the very table under test. | C5 closure test (predicted vs measured whole-codec decode, 20 % band). If it fails, the decomposition is VOID and no J-gated claim may cite it. |
| **AF-6** | P2 degeneracy: under the budget table, rANS-4096/512/256 are **exact ties at 6.0 ns/B**, resolved by strict `<` to the earliest-added candidate. A "precision-adaptive" arm could therefore be an artefact of tie order, not economics. | Stage 1 must report **flip type** (precision vs raw-vs-coded). Precision flips are reported as **tie-breaks, not wins**. |
| **AF-7** | C5 dead zone: `<16 B` streams are locked to raw and cannot falsify J, inflating F7's unstratified 100 %. | K4: stratified J-faithfulness with the `<16 B` bucket reported separately; if stratified, F7/Exp. L/C7 must be **restated**. |
| **AF-8** | ctx-rANS fan-out regression: any proposal enabling mode 6 per stream adds `~295 B × n_streams` per block. | C6/C7 gate stays; add a **static assertion or format test** that n_streams-with-ctx == 1 per block, so the guard cannot silently rot. |
| **AF-9** | DDMC/AOC/CAM all inflate the decoder-visible model surface, enlarging the malformed-input rejection surface; a new decoder-visible field needs a **forced** encode→decode test or the path rots. | New field ⇒ `FORMAT.md` registration + forced encode/decode roundtrip + fuzz ≥ 500 cases before any byte claim. |
| **AF-10** | Synth-cell win reported against the raw bar only. | **Struck** per F16 dual-bar rule; applies to any Stage-1 delta on `synth-*`. |
| **AF-11** | AOC measured on the incumbent opcode stream, then FLI lands and removes 75–77 % of hot-op draws. | Re-measure AOC's addressable base **on the post-FLI opcode stream** before any promotion; do not stack byte claims. |
| **AF-12** | CAM is bundled with PIM/FSST/front-coding/trie/hierarchy to make its numbers look good. | Forbidden (Track 15 §8.4). CAM is arbitered alone or not at all. |

---

## 7. Minimum prototype, and what it is *not*

### 7.1 Required before any Stage-2 work
None. Stage 1 is **byte-only and needs no build**: the selectors, the candidate sets, and the
`J`-component values are all already computed inside `encode_stream` /
`encode_stream_budget` (`src/anvil.cpp:1548-1653`); the dump is an added log beside the existing
`g_stream_log_entries` mechanism (`:1578`). This is the cheapest substantive measurement in the
swarm.

### 7.2 If AOC is promoted (R3), the minimum is:
one new substream mode id in the existing `decode_stream` registry, one sparse transmitted order-1
table on the opcode stream only, decoder change confined to one indexed load per opcode, and the
forced encode→decode + fuzz gate of AF-9. **Zero new ISA semantics.**

### 7.3 DDMC sketch status
`prototypes/swarm-2026-10-02/06-entropy-codesign/space-bunny/ddmc_proto.cpp` — compiles clean
(`clang++ 22.1.8 -std=c++20 -O2`), contains a tiny deterministic roundtrip self-test for the
decoder-side derivation path and exact header byte accounting. **It is not wired to production,
not built into any mode, and its numbers are not evidence.**

### 7.4 Local-run discipline (coordinator correction honoured)
Local execution was limited to a **tiny deterministic correctness self-test** (synthetic
in-process symbols, no timing, no corpus, no ratio claim). All scale/ratio/throughput work is
remote-only. **Every number produced locally in this lane is labelled TOY / NON-EVIDENCE** and
must never be cited in a gate. The toy output additionally cannot support the DDMC repeat-rate
claim, because the repeat rate on real corpus content is precisely the unmeasured quantity (§4.5
item 2) and is folded into Stage 1 as a byte-only census.

---

## 8. Recommendation — final rulings

**Track-06 verdict: the track's literal scope (a new entropy backend that crosses the frontier)
is KILLed; one shared remote arbiter is PROMOTED.**

1. **New entropy coder as a crossing route → KILL.** Prior art (FSE in zstd; context maps in
   RFC 7932) plus `DNB-M1`/`DNB-M3` arithmetic plus the fact that the reference has no adaptive
   entropy coder at all. **Zero build budget.** Do not invent a coder; the anti-overclaim
   correction in §4.1 stands (modeling moves bytes — F3/F5 — but coder-family substitution is not
   the missing axis).
2. **Cost-objective correction (constant `c` vs size-proportional `C_us`) → PROMOTE-TO-REMOTE**,
   as the **single shared 05/06/18/11 arbiter** of §3, Stage-1 byte-only first, Stage-2 paired
   timing only above the sensitivity floor, frozen gates K1–K5. Engineering correction, **no
   novelty claim**. Substrate verdict returned to all four tracks: either the argmin is invariant
   on real content (defect is hygiene; five tracks' J-gated claims survive as stated) or it is
   material (H-B ⇒ re-derive them; H-C ⇒ a byte leg exists and is adopt-class).
3. **AOC (order-1 opcode context) → PILOT**, adopt-class zero-ISA control, measured byte-only on
   the same substrate, gated by K7, never as a coder change (K6), never bundled with the rejected
   ctx fan-out (AF-8), and re-based on the post-FLI opcode stream if FLI lands (AF-11).
4. **CAM (compact per-block model prior) → HOLD.** Cost-feasible and independence-preserving per
   Track 15 §8.2 arithmetic, but unauthorized from this lane: cross-block deficit ownership,
   unadjudicated novelty residue (Track 20), enlarged decoder-visible model surface, and an
   explicit no-bundling rule. From the entropy lane its only defensible content is the §4.4
   statement of what is summarisable in 64–256 B and the observation that a static-rANS prior is a
   **model-framing** change, not a seeding change.
5. **DDMC (my own mechanism) → HOLD, deprioritized to a Stage-1 census.** Block-size arithmetic
   (P1) caps its ceiling near ~1.6 % of output, its repeat rate on real content is unmeasured,
   its substance is occupied, and it adds work to the ILP-critical chain. The cheap part — the
   per-block table-hash census — is folded into Stage 1 and, if distinct-per-block ≈ 1, kills the
   mechanism on arithmetic without a build.
6. **Not re-running, by instruction and by evidence:** opcode-pull share re-measurement (F18
   already answers it: 0.3 % of decode), macro-mask/resid raw trades (already measured), reciprocal
   rANS (F15), FSE (occupied), ctx table-Huffman (F6), multi-stream ctx fan-out (C6/C7),
   range/Fenwick decode (Exp. A).

**Budget ask:** one CI job. The decisive output is a single number — **argmin disagreement rate
on real corpus content** — which either retires a five-track dependency or forces its
re-derivation. A clean KILL here is a success, not a loss.
---

# ADDENDUM A — dated correction, issued 2026-10-02, pre-queue-freeze

**Status of the body above: SUPERSEDED IN PART.** The original text is preserved verbatim as the
record of what I wrote and why; every correction below is binding and is **not** silently merged
back into it. Corrections A1–A5 correspond one-to-one to the coordinator's five instructions.

## A1 — CAM is CLOSED-BY-PRIOR-ART (reclassifies R4)

I had R4 at **HOLD** with the novelty residue marked `[U] not claimed` (Track 15 §8.4) and
"must go to Track 20 before naming". That question is now answered. Per
`docs/swarm-2026-10-02/SYNTH-NOVELTY-SPACE-BUNNY.md` §3:

> **US 2010/0098181 A1** (*Entropy slices for parallel entropy decoding*) directly anticipates
> per-slice transmitted context-model initialization, and states ANVIL's motivation **verbatim**:
> *"the context model probability states … are reset to their initial, default states at the
> beginning of each entropy slice. **As the entropy slice size is decreased, the resets of the
> context models occur more frequently.**"* — plus *"context model initialization information to
> be used by a CABAC entropy decoder to initialize context models prior to decoding the entropy
> slice"*, and initialization *"based on the final states of the context models from one or more …
> entropy slices"*.

Corroborating: P1 (CABAC init tables are already a compact transmitted per-slice prior with a
signalled table selector), P2 (HEVC `m_lastSliceSegmentEndContextState`), P7 (`install_symbol`,
Moffat/Neal/Witten 1998), P9 (zstd `Treeless_Literals_Block` — carry-over model reuse, shipped),
P8 (RFC 7932 §2/§6 — meta-block independence is a *specified* constraint, so preserving it is not
a discovery).

**Reclassification, binding:**
- **R4 CAM → ADOPT-CLASS ENGINEERING. No novelty claim. No novelty ambiguity. Not a Track-06
  contribution under any framing.**
- The single unoccupied cell is *"port a video-codec entropy technique to a general-purpose
  byte-stream compressor"*, which the novelty lane itself classes as a port, not a mechanism.
  §2 of this report already reached the same practical answer for a different reason ("cost side
  is 1–2 orders of magnitude below the deficit"); the prior-art route is the stronger reason and
  now governs.
- **Unchanged:** the R4 ruling itself (**HOLD**) and the no-bundling rule (AF-12). Hold is now
  justified by *prior art + cross-block deficit ownership*, not by an open novelty question.
- **Consequence for this lane:** the only defensible CAM content is the §4.4 technical statement —
  what fits in 64–256 B, and that for a static rANS stream a prior is a **model-framing** change
  (transmitted seed + transmitted correction), not a seeding change. Engineering guidance, not a
  novelty claim.

## A2 — CORRECTION: the frozen A0 objective was written with a 10x wrong lambda

§3.2 item 1 specified the disagreement test as `argmin[L + 0.10*c]`. **That is wrong and is
struck.** Source-verified shipped value is `lambda = 0.01` (`src/anvil.cpp:1471`,
`g_stream_lambda = 0.01`), and `mu = nu = 0` (`:1472`), so the live default objective is exactly

```
A0:  argmin_c  [ L_c + 0.01 * c_c ]     c = {raw 10, defexc 20, huffman 22,
                                          rans256 30, rans512 35, rans4096 40, ctx 45 }
A1:  argmin_c  [ L_c + 0.01 * (N * ns_c / 1000) ]
        ns = {raw 0.1, rans4096 6.0, rans512 6.0, rans256 6.0, huffman 4.3, defexc 3.2, ctx 7.5}
```

**All occurrences of `0.10*c` / `L + 0.10` in the body above denote A0 and are replaced by
`0.01*c` / `L + 0.01*c`.** This is a *test-definition* fix only: the §2.1 crossover arithmetic
(`N* ~= 5,085 B` and the 51x/10x/2.5x/3.1x/19.7x mis-pricing table) was **already derived at
lambda = 0.01** and is unaffected.

**Scaling check (arithmetic, both arms now at lambda = 0.01, consistent):**
`D_shipped = 0.01*(40-10) = 0.30 B`; `D_prop = 0.01*N*(6.0-0.1)/1000 = N*5.9e-5 B`;
`N* = 0.30 / 5.9e-5 ~= 5,085 B`. The two objectives can only disagree when the size term spans the
whole raw-to-coded spread, i.e. **only for N >= N***. Below N*, `D_shipped > D_prop` so A0 charges
more decode cost and thus **over-favours raw**; at N = 100 B that is `0.300 B` vs `0.0059 B`, a
**51x** over-charge. The direction of the bias is therefore **sign-flipping in N**, which is why the
disagreement must be stratified by length bucket and why the arbiter reports per-bucket rates.

## A3 — A1 CLI semantics, frozen explicitly (AF-1 made non-optional)

Parser semantics verified at `src/anvil.cpp:4979`:
`opt.hotop_budget = (a.substr(15) != "off")`. Therefore **any** value other than the literal
`"off"` enables the budget path, and the flag's *absence* leaves `hotop_budget=false` (`:3786`,
applied at `:4664`). Frozen arm semantics, recorded verbatim in the artifact:

| arm | exact CLI fragment | enables | note |
|---|---|---|---|
| **A0 (control)** | `--hotop-budget=off` **or flag entirely absent** | `g_hotop_budget = false` => `encode_stream` (`:2868`) | record which of the two forms was used; both are A0 |
| **A1 (budget)** | **`--hotop-budget=on`** — one literal, frozen | `g_hotop_budget = true` => `encode_stream_budget` | **only** `on` permitted as the enabling token |

**Why only `on` is allowed (rationale corrected, 2026-10-02).** `1`, `true`, `yes`, and `budget`
are **banned for provenance and reproducibility only**. Per the parser
(`(a.substr(15) != "off")`) they all set the *same* boolean and therefore select the **same** budget
path — they are not distinct code paths, and this report does not claim they are. The reason to
freeze one literal is that the parse is **permissive**: a typo such as `--hotop-budget=offx`, or a
driver that passes `--hotop-budget=1`, silently **enables** the budget path, so the arm's CLI
identity is not recoverable from the value alone. Freezing exactly one spelling makes
"which cost model ran?" a decidable question from the recorded command line, which is the whole
point of AF-1.

Additional frozen provenance requirements per arm: full CLI string, `j_agree/j_total`, block
count, stream count, and the Stage-1 byte-only stream dump. **A missing arm record => VOID run**
(AF-1). This is the concrete form of AF-1: the hazard is one string comparison in the flag parser.

## A4 — A2 (lambda-prime recalibration) is REMOVED from the first arbiter

§2.4's globally-rescaled-lambda arm is **deferred**. It is struck from the first arbiter and may not
be built until **A0 vs A1 is shown to have material choice disagreement** (K1 fires at >= 10 % on
>= 2 files, or K2 fires with the band inside 16-4096 B).

Reason, recorded so it is not relitigated: fitting a third policy before proving the **two
shipped** objectives diverge is calibration on unproven ground and risks the post-hoc threshold
motion that doctrine item 5 forbids. If later authorized it is a **second-stage experiment under a
separately frozen rule**, fixed before its own data exists: `lambda'` chosen so
`lambda'*(c_4096 - c_raw)` equals the calibrated gap at the corpus-median stream length `N_med`
read off the **Stage-1 dump**, with `N_med` recorded in the pre-registration. No per-file,
per-codec, or per-bucket tuning. **No third candidate set may be added under any circumstance.**

Net effect: **two arms, not three.** The first job is strictly a *divergence test between two
objectives that already ship in the binary* — cheaper and cleaner than a three-way comparison.

## A5 — AOC: full charge, and census-before-format

The body (§4.3, §5.3) listed AOC's order-1 table as "sparse top-N ~= 96-512 B" with full
256x256 (64 KiB) "refused", while §3.2 item 5 scheduled the byte-only census in the same stage
as the mechanism work. Both are tightened here.

**A5.1 — the conceptual table is NOT free, and is charged in full.** Complete accounting for a
sparse order-1 model over the opcode alphabet, on ANVIL's existing sparse-table framing
(`rans_stream_bytes`; code-verified header `4 + 2s` B, §5.1):

| component | charge |
|---|---|
| stream-mode id + length prefix | `1 + uvar(N)` ~= **2 B** |
| order-1 table, **sparse** (nonzero context/symbol pairs only) | `1 + uvar(nz) + sum_nz (1 B sym + uvar freq)`; `uvar freq <= 2 B` so **`2 + 3*nz` B** |
| realistic `nz` on the mode-15 opcode stream (alphabet <= 255, one dominant hot opcode, several near-dominant macro classes) | **projection [P]**, `nz ~= 40-120` so **122-362 B per block** |
| **worst case** (dense over the full opcode x opcode product) | **`2 + 3*65,280` ~= 196 KiB** so **REFUSED**, no measurement needed |
| decoder-side table | `O(nz)` entries; decode lookup is `prev_op` -> row so **+1 indexed load per opcode on the rANS serial chain**; scattered sparse rows make this a **second dependent load**, exactly the hazard flagged in §3.5/R5 |
| decoder state / RSS | `O(nz)*(2 B freq + 1 B sym)` ~= **0.5-1 KB**, bounded and `O(1)` in input size — **not free**, and it competes for L1 with the existing `book[]` and opcode pull state |

So AOC is **not** a zero-cost context flip: **+~2 B + 3*nz per block** (~=122-362 B projected) plus
one extra dependent load per opcode, in exchange for `H(op) - H(op|prev_op)`. On generated.log that
header is ~0.07 % of a 175,550 B output — small but not zero, and it is a **per-block** charge that
scales with block count while the saving scales with token count. That asymmetry must be inside
the census, not discovered later.

**A5.2 — census BEFORE format.** The **only** AOC work authorized by this report is the
**byte-only conditional-entropy census**, requiring **no wire format and no build**: per corpus
file, over the existing mode-15 opcode stream, compute `H(op)`, `H(op | prev_op)`, and the
`2 + 3*nz B` header charge at the *realized* `nz`; report
`net_bytes = (H(op) - H(op|prev_op)) - header_charge` per file and per block-count bucket. K7 is
rewritten against that net number:

| gate | PROMOTE | KILL |
|---|---|---|
| **K7 (revised)** | `H(op) - H(op|prev_op)` >= **0.5 %** of complete bytes on **>= 2** record files **after** subtracting the realized sparse header charge, at **0 new ISA semantics** and **<= 1** new decoder-visible substream mode id | net <= 0.5 % everywhere => KILL as a byte lever; record the opcode context entropy as a measured negative |

Only if K7 fires does a wire format get written, and only then does §7.2's minimum prototype
apply. **AOC remains a control, not a contribution** (unchanged from §4.3), and remains barred
from instantiation as per-stream ctx-rANS fan-out (AF-8, C6/C7: `9 * ~295 B ~= 2.65 KB` per block).

## A6 — Consolidated first-arbiter scope after this addendum (binding)

**Two arms (A0, A1), byte-only first, one CI job, exact CLI frozen by A3, objective exactly as
source by A2, no third policy by A4.**

Stage 1 (citation-grade, no build):
1. per-stream dump `(stream_index, block_index, role, N, chosen_codec, L_c for all c, J_A0, J_A1)`;
2. **argmin disagreement rate** `argmin[L+0.01*c] != argmin[L+N*ns*1e-5]`, overall and by bucket
   `<16 / 16-512 / 512-4096 / 4096-16384 / >=16384`;
3. **complete-bytes delta** per arm (`sum_block sum_stream L_chosen`) — what actually ships (K3);
4. per-codec flip counts split into **precision flips** (rANS 4096<->512<->256, which under the
   budget table are **exact ties at 6.0 ns/B**, broken by strict `<` to earliest-added — AF-6:
   report as tie-breaks, never as wins) vs **raw<->coded flips**;
5. stratified J-faithfulness with `<16 B` isolated (K4, AF-7);
6. **DDMC bank census** — per block and stream role, the hash of each rANS `freq` array and its
   distinct-count; distinct-per-block ~= 1 => DDMC killed on arithmetic, no build;
7. **AOC conditional-entropy census** per A5.2, net of header charge (K7 revised).

Stage 2 (scout/ranking-grade, **only** if K1/K2 fire and only above the F18 +2.3-8.1 %
sensitivity floor): paired same-job timing with C1-C7 (clean paired control, `tax <= 2 %` both arms
and `|Dtax| <=` CV, byte-identity under instrumentation, partition closure, 20 % predicted-vs-measured
closure test, no per-region timers as primary, artifact header stamped scout-grade).

**Explicitly NOT re-run** (already measured, F18): opcode-pull share (0.3 % of decode),
macro-mask/macro-resid raw trades (+166,823 B / +84,956 B for +21-27 % / +18-21 % decode),
macro-dvar (already raw), CRC (PCLMUL retained). **Explicitly NOT proposed:** any third cost
policy, any new entropy coder (K6), any per-stream ctx fan-out (AF-8).

## A7 — Rulings after correction (final; supersedes §0 and §8 where they differ)

| # | candidate | final ruling | changed by this addendum? |
|---|---|---|---|
| R1 | New entropy coder as a crossing route | **KILL** | no — reinforced: the project novelty matrix classifies all of Track 06 as **ADOPT-CLASS** (`SYNTH-NOVELTY-SPACE-BUNNY.md` §2, Track 06 row) |
| R2 | Cost-objective correction (A0 vs A1) | **PROMOTE-TO-REMOTE**, shared 05/06/18/11 arbiter, **two arms only** | **yes** — lambda fixed to 0.01 (A2), lambda-prime arm deferred (A4) |
| R3 | AOC order-1 opcode context | **PILOT**, adopt-class control, **census-before-format** | **yes** — fully charged (A5.1), K7 rewritten net-of-header (A5.2) |
| R4 | CAM model-prior | **HOLD — ADOPT-CLASS ENGINEERING, CLOSED-BY-PRIOR-ART** | **yes** — novelty ambiguity removed (A1) |
| R5 | DDMC | **HOLD**, deprioritized to a Stage-1 census | no |

**Unchanged headline:** one CI job; the decisive number is the **A0-vs-A1 argmin disagreement rate
on real corpus content**; a clean KILL is a success. The anti-overclaim correction from §4.1
remains binding on every downstream brief — the *deficit* is not located in entropy coding and
coder-family substitution is closed, but measured modeling wins (F3: -8.3...-16.2 % bytes; F5:
379 KB vs 422 KB) prove entropy-layer choices move bytes, and striking "entropy cannot matter" in
that direction is equally wrong.