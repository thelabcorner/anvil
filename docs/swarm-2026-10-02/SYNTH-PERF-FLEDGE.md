# SYNTH — Performance Economics Audit (Fledge Alpha Free, adversarial/validator role)

**Scope:** cross-lane synthesis for the 2026-10-02 swarm. **Companion to**
`docs/swarm-2026-10-02/18-future-decoder-architecture-fledge.md` (track 18 full audit); this file is the
compact, decision-grade version intended for Tracks 04, 05, 06, 07, 09, 11, 15, 17 and 18.
**Date:** 2026-10-02 · **Tree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty
**Discipline:** no measurement performed by this agent; no local benchmark; no local fuzzing; no production
edit; no commit/push. Every number is `[MEASURED]` (in-repo location given), `[DERIVED]` (arithmetic on
measured), `[ARCH-REASONING]`, or `[PROJECTION]`.

---

## 0. Ruling

> **KILL every proposed decoder-mechanism whose targeted share is below ~8 % of end-to-end decode on a
> cell with a measured profile. The token-lane / entropy-pull / dispatch / superinstruction /
> small-copy / CAM-prior family is KILL — not "unmeasurable", but arithmetically capped.**
>
> **MERGE** all shared substrate work into **three** experiments (§6), owned by named tracks and consumed
> by the rest. **No track files its own CI job for a shared substrate question.**
>
> **PILOT** exactly two mechanisms: per-stream **table-width tiering** (reclassified by this audit as an
> **RSS** mechanism, not a decode mechanism) and **dense-integer side-stream recoding**. Both are
> byte/RSS-decided; neither carries a timing claim.
>
> **The one measurement that changes the most: SX-2**, and it is a *re-measurement of an instrument that
> already exists*, not a new profiling program.

---

## 1. The measured pools (this is the whole audit)

### 1.1 `generated.log` decode-floor profile — `[MEASURED]` `RESEARCH_LEDGER.md:3911-3936`

Provenance is strong and I re-verified every field: payloads from CLI at `git HEAD 392e937`;
`gen_log.anv = 175,550 B` **exactly** equal to the frozen-gate baseline; harness
`prototypes/profile_tmp/prof.cpp`; HIGH priority; pinned; QPC + invariant-TSC; **median-of-7 interleaved**,
warmup 2; all 4 containers round-trip byte-exact; instrumented decoder **verified byte-identical to
`decode_tokens_hotop_fused` on every block**. Floor e2e **235.8 MB/s (CV 10 %)**; frozen reference
**273.5 MB/s**.

| # | stage | share of e2e | ns/B `[DERIVED]` | addressable? |
|---|---|---:|---:|---|
| 1 | `crc32` bytewise | **44 %** (43.8 % log) | 1.860 | **CREDITED AND SPENT** — PCLMUL landed (A21) |
| 2 | eager materialization of the 7 macro streams ("setup IS masks+resid") | **26 %** | 1.103 | **NO — measured non-removable** (MAT: lazy pull **+27.6 % slower**) |
| 3 | token loop beyond setup | **11 %** | 0.467 | partly — but see the sub-line |
| 3a | **of which opcode entropy pulls** | **0.3 %** | 0.0127 | this is what every token-lane proposal actually targets |
| 4 | concat / alloc / headers | **~15 %** | 0.636 | partly (ALLOC attacked this class) |
| 5 | unattributed residual | ~4.2 % | 0.178 | — |

Ledger's own summary line: **"THE HOT PATH IS CLEAN; the floor is everything AROUND it."**

### 1.2 The Amdahl ceilings that follow — `[DERIVED]`

| question | arithmetic | ceiling |
|---|---|---:|
| Absolute wire-invisible ceiling on `generated.log`, post-CRC | `1/(1−0.52)` | **2.083×** |
| Realistic wire-invisible ceiling (stage 2 proven non-removable) | `1/(1−0.26)` | **1.351×** |
| Smallest **true** measured decode win in project history | aux-unbwt / dickens | **1.364×** |
| Ceiling for the **entire token loop** (stages 3a+3) | `1/(1−0.11)` | **1.124×** |
| Ceiling for **entropy pulls alone** (stage 3a) | `1/(1−0.003)` | **1.003×** |

> ### **1.351× < 1.364×.** The realistic wire-invisible ceiling on the best-profiled cell in this project is
> **below the smallest factor that has ever actually worked here.** That is not a judgement call — it is
> the measured profile plus the measured MAT result, combined.

### 1.3 Timing resolvability floor — `[MEASURED]`

`RESEARCH_LEDGER.md:3918-3920`: within-run CV **0.6–10 %**; run-to-run drift **±2–4 %**; **zero-byte-change
control variants +2.3–8.1 %**, with the ledger's own note that *"single-stream effects inside it [are] NOT
resolvable."*

Cross-lane spread on **frozen, byte-identical wire** — `[DERIVED]` from three in-repo windows:
`synth-timeseries` hotop-rlzp (140,898 B) spans 1.281–1.450 ms = **+13.2 %**; `generated.json` mdl
(89,589 B) spans 3.754–4.375 ms = **+16.5 %**.

Empirically calibrated sensitivity floors `[DERIVED from which comparisons the project actually resolved]`:

| design | floor | evidence |
|---|---:|---|
| unpaired / cross-lane | **≥ 13 %** | the spreads above |
| **paired same-job, interleaved, same wire, reps ≥ 7** | **≈ 5–7 %** | resolved +35.2 % and **+7.4 %** (ALLOC, landed & retained); **1.4 % was NOT resolved** (MAT on mode-10) |

> **Kill rule: any mechanism naming a targeted share < 8 % cannot be certified on this harness.** 8 % is
> the perturbation band's ceiling (8.1 %) and sits just above the paired floor.

---

## 2. Per-mechanism audit — every live mechanism in the swarm

Columns: **share targeted** (measured where a profile exists, else flagged) · **Amdahl ceiling** ·
**required factor to matter** · **byte cost** · **RSS / code cost** · **timing resolvability** · **verdict**.

| # | mechanism | share targeted | ceiling | byte cost | RSS / code | resolvable? | verdict |
|---|---|---|---:|---|---|---|---|
| 1 | **I10-1A aux-unbwt** `[MEASURED]` | changes the *representation*, so not pool-bound | **1.364× / 1.795× / 2.339×** | **+22,398 B** on 311,938,580 B = **+0.00718 %**; per-file +2,494…+4,098 B | RSS ~unchanged (599 MiB enwik8); **+2,976 B `.text`**, +8,192 B ELF | **YES — measured, CI-paired, A/A-null passed** | **ADOPTED.** Exchange rate ≈ **3 KiB `.text` ≈ 1.36–2.34×** |
| 2 | Store-path CRC (PCLMUL) `[MEASURED]` | **44 %** (bytewise base) | 1/(1−0.44) = 1.79×, realised **~3.4× on the store path** | **0 B** — wire-invisible | +.text, already landed | YES | **SPENT.** Residual share now 2.1–3.4 % → below kill rule |
| 3 | ALLOC (direct decode into output) `[MEASURED]` | concat/alloc/headers ≈ 15 % | 1.176× | **0 B** | negligible | YES (+35.2 % json) / **+7.4 % borderline** (timeseries) | **RETAINED, EXHAUSTED** |
| 4 | Decode leg 4 (two-level tables + buffered renorm) `[MEASURED]` | BWT postcoder ≈ 34–44 % | 1.35–1.65× | **0 B** | small | YES (+17.9 % / +16.9 %) | **RETAINED, target 1.4× falsified** |
| 5 | MATERIALIZATION (lazy `StreamPull`) `[MEASURED]` | stage 2, **26 %** | 1.351× | 0 B | — | YES | **FALSIFIED: +1.4 % / +27.6 % slower.** Stage 2 is *not* removable |
| 6 | G4 / G5B planner, G4 direct proxy, PORDER | — | — | — | — | byte-only | **KILLED** (master-brief item 13) |
| 7 | **FLI / loop ISA (Track 11)** | **opcode entropy pulls = 0.3 %**; realistically the 11 % token loop | **1.003× / 1.124×** | Δ bytes/loop = `2 − (k−1)·H_op`; H_op **unmeasured** | ≤200 B state, 0 RSS; ≤4 alphabet symbols ≈ ≤30 B/block | **NO** — 0.3 % is 27× inside the perturbation band | **KILL the decode leg on arithmetic.** Byte leg only if Track 11's G2–G5 pass, as adopt-class ratio work |
| 8 | **Token lanes / entropy-pull SIMD (Track 18)** | **0.3 %** | **1.003×** | — | — | **NO** | **KILL.** Attacks the smallest measured share in the profile |
| 9 | **Superinstructions / fused token loop (Track 18)** | ≤ 11 % | **1.124×** | often **negative** — the fully concrete opcode book already bloated `generated.log` to ~652 KB at ~935 MB/s | — | marginal at best | **KILL.** Ceiling 1.124× < 1.364× |
| 10 | **Small-copy specialisation (Track 18)** | ≤ 11 % | **1.124×** | 0 B if free | — | NO | **KILL.** `generated.log` raw-hot already reaches 963 MB/s |
| 11 | **Dispatch / branch-prediction work (Track 18)** | ⊂ 11 %, unattributed | ≤ 1.124× | 0 B | — | NO (only indirect evidence: MAT +27.6 % ⇒ ≈5.5 ns/symbol `[DERIVED]`) | **KILL.** Structurally unpredictable while opcodes are entropy-coded |
| 12 | **Table-width tiering (Track 18/06)** | stage 2 **26 %** × ledger's 5–12 % relative band ⇒ **1.3–3.1 % of e2e** | **1.013–1.032×** | precision loss, **byte-only decidable** | **the real target: 32 KiB packed table = 100 % of Zen 3 L1d → 1–2 KiB** | **NO on decode** | **PILOT on RSS + bytes.** Decode claim **dropped** |
| 13 | **Dense-integer side-stream recoding (Track 18/06)** | stage 2 (26 %) via cheaper materialization: `defexc` mat **0.48–0.56 ns/B** vs rANS mat **4.35–4.73 ns/B** `[MEASURED]` — an **8–10× cheaper path already in the suite** | up to **1.35×** if the streams re-select | byte-only decidable | low | marginal; **decide on bytes** | **PILOT byte-only.** Highest expected value in this table |
| 14 | **CAM / PPM compact model priors (Track 15)** | **wrong axis** — see §3 | — | 64–256 B × blocks; blocks/file **unmeasured** | sequential mixing *adds* serial work to the 11 % pool | NO | **KILL as decoder mechanism**; retain as I10's designated **O1 diagnostic control** |
| 15 | **Long-range / dual-timescale dictionary (Track 15)** | decoder **memory**, not decode cycles | — | dictionary bytes are decoder-visible and must be charged | directly targets the **4.57×–9.04× xz RSS deficit** | n/a (memory axis) | **PILOT on the RSS axis**, byte-charged |
| 16 | **BWT subblocking (Track 07)** | peak RSS | — | **+1.0169 %** bytes at 16 MiB (125.7 MiB, Silesia); enwik8 8/16/32 MiB **route away → +5.4075 %** | direct | YES | **CLOSED / no default change** (I10-1A.2) |
| 17 | **TCOPY / derived-mask E8E9 (Track 09)** | rate axis only | — | `.text`+`.rodata` ≈ +1.48 KB for 81-byte string masks `[DERIVED]` | negligible | byte-only | **PILOT byte-only**, subject to Courgette/BCJ prior-art gate |
| 18 | **Independent-stream scheduling** | — | — | — | — | — | **KILL — measured falsified**: `libsais_unbwt_omp` parallelises init only, no walk gain |
| 19 | **Resolved-graph / decode-recipe decoder (Track 18 headline)** | — | — | — | — | — | **KILL as novelty** — this is **Meta OpenZL**, shipped, open source (arXiv 2605.09928). Adopt-class principle only |
| 20 | **rANS lane scaling > 4-way** | — | — | — | 4×32-bit = 50 % of an AVX2 register; 4×64-bit = 25 %; 8×64-bit needs AVX-512, absent on Zen 3 `[ARCH-REASONING]` | NO | **KILL — hardware-capped on the evidence hosts** |

---

## 3. Focused audit — Track 15's CAM-style compact model priors

**Question put to this reviewer:** can a 64–256 B/block compact prior summarize enough adaptive state to
offset the measured warmup tax while preserving independent blocks, or is the claimed state dimension too
large?

**Verdict: the state dimension is too large, and independently the mechanism attacks the wrong axis.**
Three independent reasons, each resting on a measured in-repo fact:

1. **"Preserving independent blocks" is the condition ANVIL has already measured as harmful.** Block-local
   reset at 64–256 KiB makes `.text` **worse** — TCOPY milestone 1 (`docs/CONTEXT.md`): useful transformed
   phrases are not local. E4 (`06-do-not-reburn.md`) closed block-local routing by negative: worse than
   whole-file at **every** scale; `webster` sum-of-4-MiB-blocks **+894,315 B (+12.22 %)** vs whole-file;
   *"finer granularity (256 KiB / 1 MiB) is monotonically worse — warmup tax grows as blocks shrink, and
   16 MiB is already near-zero tax."* A per-block model budget buys block independence by *paying bytes on
   an axis where blocks are already the cheapest granularity*, while the binding deficits are decode
   (3.9–5.7×) and RSS (4.6–9.0×).

2. **The state dimension does not fit.** 256 B = 2,048 bits. An order-2 binary context model over 2¹⁶ bins
   needs ≈ 32 KB at 4 bits/bin; 256 B holds roughly 256 8-bit quantized weights. Two *measured* ANVIL facts
   bracket this from both sides: unconditional order-1 literals were **rejected at +3.46 % bytes**
   (cold-start sparsity), and the later refinement established that the rejection was about **physical
   model multiplicity** — hundreds of cold adaptive models — *not* the information signal. So the gain
   requires many states; 256 B cannot hold many states; too few states was the measured +3.46 % failure.
   The budget and the mechanism are mutually exclusive.

3. **Axis mismatch, and the direction of the effect is wrong.** CAM/PPM is a **rate** mechanism, and rate is
   the axis ANVIL already wins (46,446,995 B vs xz 48,456,100 B = **−2,009,105 B**). The binding axes are
   decode and RSS. Worse, a sequential mixer inside the decoder *adds* serial dependent work — it lands in
   the **11 %** pool and pushes it **up**. There is no version of this mechanism that makes the measured
   profile better.

**Also decisive procedurally:** CTW/PPM/CAM are *already designated* in this project as **diagnostic
controls, not production** — `docs/FRONTIER-RESET-2026-09-23.md` §4.8 ("ANVIL should use CTW/PPM-class
models as diagnostic controls") and `docs/I10-BREAKTHROUGH-PROGRAM.md` §3 O1 ("**No production claim
follows automatically**"). A production CAM prior would have to overturn that designation with new evidence,
and the evidence above says it should not.

**One honest salvage.** The profile's parenthetical — *"setup IS masks+resid"* — identifies where stage 2's
26 % actually sits: the entropy decode of the mask and residual streams themselves, **not** model
construction. A compact prior cannot attack it. What *can* attack it is selecting a cheaper codec for
those streams, and a **8–10× cheaper path already exists in the suite** (`defexc` mat 0.48–0.56 ns/B vs
rANS mat 4.35–4.73 ns/B `[MEASURED]`), currently selected only when a stream is ≥ 50 % one value. That is
row 13 of §2, not CAM. **The right answer to Track 15's question is "the state dimension is too large" —
and the right next step is the codec-selection one, which is byte-only decidable.**

---

## 4. Hidden costs that proposals routinely omit

| cost class | measured basis | requirement |
|---|---|---|
| **Code size** | aux-unbwt: **+2,976 B `.text`**, **+8,192 B** ELF step; final stripped CLI **453,512 B**; AITDCC decompressor cap **1 MiB** | report `.text` **separately** from stripped-ELF size (the file-size step is 8 KiB-granular and masked a 2,976 B text delta). **Cap: `.text` growth ≤ 8 KiB**, which at the measured ≈3 KiB ↔ 1.36–2.34× rate demands **≥ ~4× decode** — nothing on the board projects that |
| **Peak RSS** | enwik8 **598.9 MiB** = **9.043× xz**, 2.378× Brotli; Silesia **248.4 MiB** = **4.572× xz**; relief priced at **~1 % bytes per 2× RSS** on Silesia, **unpurchasable** on enwik8 | every decoder-visible structure must charge RSS. Table-width tiering's honest axis is here |
| **Amplification guards** | RePair/RLZ needed per-rule, cumulative-expansion **and** recursion-depth caps (three new caps, all found by crafted probes) | any new loop/generate opcode must prove **no new amplification class** or ship the caps |
| **Registry / fuzz** | **14** registered block modes, each with a forced encode→decode test | a new decoder-visible mode needs a forced test from day one; new *stream-codec* IDs need none if existing suite IDs are reused |
| **Wire bytes on the metadata streams** | raw macro-masks **+166,823 B** for +21–27 % decode; raw macro-resid **+84,956 B** for +18–21 % — on a 175,550 B container that is **+95 %** and **+48 %** | the "spend rate surplus on a raw stream" trade is **measured dead** on the incumbent cell. macro-dvar is *already* raw (0.01 % share) and the Linux "+19.8 KB ⇒ +0.12 GB/s" result **does not transfer** |
| **Units in the cost objective** | `src/anvil.cpp:1618` ships `kBudgetNsPerByte = {0.1, 6.0, 6.0, 6.0, 4.3, 3.2, 7.5}`; `FORMAT.md:636-663` still documents `c ∈ {10..45}` integer-tenths | **normative spec and binary disagree.** λ = 0.01 against microseconds added to a byte count ⇒ **1 byte ≡ 100 µs of decode**; the term reaches 1 byte only for streams > **13.5 KB** `[DERIVED]` |

---

## 5. Provenance and claim-hygiene audit

- **The selector objective is settled, and not in anyone's favour.** The S6-1 selector manifest
  (`RESEARCH_LEDGER.md:3965-3979`) records **792 selections (88 blocks × 9 streams, 23 files)** with
  **ZERO raw-flips**: no stream enters or leaves mode 0, and the S6-1 target stream `macro-dvar` **never
  leaves raw**. All **139** flips were **exact-L ties** (`legacy L == budget L` in 139/139), exclusively
  rANS precision swaps, every one with `logical_eq=1 ∧ dec_eq=1`.
  > **Measured: the incumbent objective is a tie-breaker, not a selector.** This *confirms* the Track 06
  > finding and the §4 arithmetic above with 792 observations — a separate Track-18 recalibration job
  > would re-derive a known answer.
- **Absolute MB/s must never be compared across windows.** The floor profile itself notes its absolute level
  runs **5–15 % below** the bench-suite protocol and that only *relative, interleaved* deltas are valid.
- **Superseded numbers still circulating:** PR-1's `mat`-inclusive caps (2.06× / 3.69× / 1.60×) were
  **retired as falsified** (`prototypes/i9-decode-perf/REPORT.md` §12). And the PR-1 stage splits
  (mat 25–31 %, token loop 60–62 %) are for the `synth-timeseries`/`generated.json` rows — **they do not
  describe `generated.log`**, where the measured split is 26 % / 11 % / **0.3 % pulls**. Conflating the two
  is how a 0.3 % pool gets reported as a 60 % pool.
- **Projection hygiene.** `docs/CONTEXT.md`'s Linux decoder numbers (957 MB/s, "possibly the first three-axis
  crossing") are self-flagged as *"timing moved with CPU state (not called yet)"* and were never promoted.
  Not admissible as decoder-architecture evidence.

---

## 6. Merged shared-substrate experiments (one per axis; no per-track duplicates)

### SX-1 — selector-cost census · **owner: Track 06** · consumers: 05, 06, 18

- **Reuse, do not rebuild.** The arch selector-manifest harness already exists (SEL-logging diagnostic
  build over `fc23d9a`). Extend it to three λ arms: **0.01** (incumbent) / **0** (pure length) / **λ\*** set
  so the cost term reaches 1 byte on a 16 KB stream.
- **Complete-byte accounting:** every arm must be byte-identical to freeze-HEAD per file, SHA-256; full
  round-trip on every corpus file and PE; forced-registry coverage.
- **Paired uninstrumented control:** a **zero-byte-change control variant built from the same source**, run
  in the same job, published as the job's noise floor **before** any other number is read. Skip it and the
  job is VOID.
- **Paired decode only where differences exceed the floor:** decode timing is run **only** on streams whose
  census shows a byte difference **≥ 1 %** — i.e. probably none. Most likely output is a census, not a
  timing.
- **Single artifact, consumable by 05/06/18:** `selector_flip_census.csv` — one row per selection:
  `{file, block, stream, legacy_codec, arm_codec, legacy_L, arm_L, ΔJ, flipped?, logical_eq, dec_eq}`.
- **Frozen thresholds:** (a) validity = incumbent arm byte-identical **and** control published; (b) **KILL**
  if λ\* arm is byte-identical to the incumbent on **every** file ⇒ no λ-gated decoder proposal is
  admissible; (c) **SURVIVE** if λ\* improves total bytes by ≤ −0.10 % on ≥ 4 files, in which case
  `FORMAT.md` needs a units-correct rewrite (bytes per microsecond).

### SX-2 — post-PCLMUL stage shares on the existing instrument · **owner: Track 18** · consumers: 06, 11, 15, 17

- **This is a re-measurement of `prototypes/profile_tmp/prof.cpp` — the instrument already exists.** No new
  generic profiling program. Same protocol: HIGH priority, pinned, QPC + invariant-TSC, median-7 interleaved,
  warmup 2, containers round-trip byte-exact, instrumented decoder verified byte-identical to the production
  decoder on every block.
- **Why it is still needed:** the §1.1 profile is on a **bytewise-CRC** base that has since been fixed, so
  its shares no longer describe the shipped decoder.
- **Three outputs, in priority order:**
  1. **current-dispatch stage shares** on `generated.log` + the mode-15 record cells (replaces the
     pre-PCLMUL split);
  2. **`φ/L` — the match-length histogram weighted by bytes covered** (never measured; decides Track 11
     G1/G6 *before* any mechanism is built, and is byte-only);
  3. **dispatch's share inside the 11 % token loop** (only indirect evidence exists today: MAT's +27.6 %).
- **Gates:** zero-byte-change control arm **≤ 5 %**, else VOID; `Σ components` must reconcile to whole-codec
  within **35 %**, else attribution is invalid.
- **Single artifact:** `decode_floor_postcrc.csv` + `token_length_hist.csv`.

### SX-3 — table-width sweep + residency census · **owner: Track 18** · consumers: 06, 15

- **Byte/RSS-only first.** Sweep per-stream width ∈ {256, 512, 1024, 4096} under a length-keyed rule;
  report total bytes and **peak model-table resident bytes**, separated from output buffer and BWT working
  set.
- **Timing admitted only if** the predicted decode effect is **≥ 7 %** — which §2 row 12 says it is not.
  Default expectation: **no timing claim**.
- **Frozen thresholds:** GO if total byte cost **≤ −0.30 %** and ≥ 4 of 6 canonical files improve **and**
  peak table residency falls **≥ 4×**; KILL if any frozen ratio-first corpus file regresses on bytes, or
  if peak table residency does not fall.

---

## 7. Genuinely missing measurements — final list (5)

Everything else the swarm has asked for either already exists or is covered by SX-1/2/3.

1. **Post-PCLMUL stage shares on `generated.log`.** The §1.1 profile predates the CRC fix. *(SX-2)*
2. **`φ/L` — match-length histogram weighted by bytes covered.** Never measured; decides Track 11's gates
   without any timing. *(SX-2, byte-only)*
3. **Dispatch's share inside the token loop.** Only indirect evidence exists (MAT ⇒ ≈5.5 ns/symbol
   `[DERIVED]`). *(SX-2)*
4. **Block count / mean block size per canonical file.** Required to price *any* per-block budget (Track
   15's 64–256 B/block among them). The 792-selection manifest is a 23-file **sample** (88 blocks × 9
   streams), not a per-file census. *(SX-1, byte-only)*
5. **Peak model-table resident bytes, separated from output buffer and BWT working set.** Every RSS number
   on record conflates them; without the split, table-width tiering cannot be priced on the axis it
   actually targets. *(SX-3)*

---

## 8. Final ruling

> ## **KILL** — token lanes / entropy-pull SIMD (pool **0.3 %**, ceiling **1.003×**); superinstructions and
> fused-token-loop ISAs (pool ≤ 11 %, ceiling **1.124×** < the **1.364×** smallest true win); small-copy
> specialisation (same pool); dispatch/branch-prediction work (structurally unpredictable, pool unattributed
> and ⊂ 11 %); **CAM/PPM compact model priors as a decoder mechanism** (§3 — state dimension too large,
> axis mismatched, already designated a diagnostic control); independent-stream scheduling (measured
> falsified); rANS lane scaling > 4-way on AVX2/Zen 3 (hardware-capped); any novelty claim for the
> resolved-graph/decode-recipe decoder (OpenZL).
>
> ## **MERGE** — all shared substrate work into **SX-1 / SX-2 / SX-3** (§6), one owner each, one artifact
> each, consumed by Tracks 05/06/18 and others. **No track files a private CI job for a shared question.**
>
> ## **PILOT (byte/RSS-decided, remote)** — per-stream **table-width tiering** (RSS axis: 32 KiB packed table
> = 100 % of Zen 3 L1d; decode claim **dropped**) and **dense-integer side-stream recoding** (targets the
> 26 % stage with an 8–10× cheaper path already in the suite). Track 15's **long-range dictionary** continues
> on the RSS axis. Track 09's **TCOPY** continues byte-only, behind its prior-art gate.
>
> ## **Strongest kill argument.** On the only cell with a full measured profile, the largest share that is
> actually removable by *any* wire-invisible decoder change is **26 %** (and that 26 % is already proven
> non-removable by MAT), giving a realistic ceiling of **1.351×** — below the **1.364×** that the project's
> one real decoder-architecture win delivered. Every architecture proposal currently on the board names a
> smaller pool than that: **0.3 %** for entropy pulls, **≤ 11 %** for the token loop. And the harness cannot
> certify anything below **~7 %** even with perfect paired design, because its own zero-byte-change controls
> cost **2.3–8.1 %**.
>
> ## **Strongest surviving case.** **Dense-integer side-stream recoding.** It attacks the largest addressable
> share (26 %), an **8–10× cheaper materialization path already exists inside the current suite**
> (`defexc` 0.48–0.56 ns/B vs rANS 4.35–4.73 ns/B `[MEASURED]`), it needs **no new stream-codec ID**
> (modes 1/2/3/5 already exist), its cost is **wire-visible and byte-only** so it escapes the exhausted
> wire-invisible class, and its decision needs **no clock at all**. It is adopt-class prior art — and it is
> still the highest expected-value spend on the decode side.
>
> ## **One cheapest decisive next experiment.** **SX-2** — re-measure the *existing* t3 harness on the
> post-PCLMUL build for `generated.log`, and emit `φ/L` alongside the refreshed stage shares. One job, one
> existing instrument, byte-only artifact for the histogram, and it is the single measurement that decides
> Track 11's G1/G6, prices Track 15's per-block budget, and fixes the pool denominator for Tracks 06, 17 and
> 18 at once.

---

*Prepared by Fledge Alpha Free (independent adversarial reviewer). Independent of constructive-lane
verdicts; every number re-derived from in-repo artifacts with locations named. Cross-lane synthesis for the
2026-10-02 swarm; full track-18 audit in
`docs/swarm-2026-10-02/18-future-decoder-architecture-fledge.md`.*