# Track 11 — Orbit/Program Synthesis Compression — INDEPENDENT ADVERSARIAL AUDIT

**Author:** Fledge Alpha Free (independent adversarial reviewer / validator)
**Lane:** swarm `swarm-2026-10-02`, track `11-orbit-programs`
**Date:** 2026-10-02 · **Tree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty
**Mandate:** red-team orbit/program-synthesis compression compression at the *program/ISA* level; attack decoder complexity, VM overhead, grammar/SLP/LZ prior art, Kolmogorov-search impracticality, overfit, instruction metadata, and boundedness; propose a smaller alternative.
**Artifacts read:** `MASTER-BRIEF.md`; `docs/ORBIT_PROGRAM_COMPRESSION.md`; `docs/FRONTIER-RESET-2026-09-23.md` (§6.2, §6.5, §6.7, §7); `docs/gate-priorart-audit-i8.md`; `docs/gate-ruling-i9-pr5-position-derived.md`; `docs/priorart-tcopy-external.md`; `docs/decoder-audit.md`; `docs/GITHUB-ACTIONS-BENCHMARKING.md`; `FORMAT.md`; `RESEARCH_LEDGER.md` (PART X Exp. Z, Exp. I/R/S/Y/AA, PART XIII); `src/anvil.cpp` (read-only, cited by line); `prototypes/orbit_ariref/results_run3.txt`; and the three paired swarm reports `11-orbit-programs-space-bunny.md`, `19-format-security-space-bunny.md` §10, `20-priorart-killteam-space-bunny.md` §9.

**No measurement was performed by me.** No local corpus benchmark, no sweep, no fuzz campaign, no prototype was built or run. No existing file was modified. No commit/push/reset/clean/stash/restore/rebase.

---

## 0. Label discipline

| Tag | Meaning |
|---|---|
`MEASURED` | a number or guard recorded in a named in-repo artifact, cited by path:line.
`DERIVED` | arithmetic on `MEASURED` inputs, arithmetic shown inline. No new measurement.
`HYPOTHESIS` | not measured, not derived; a proposition to be tested.
`PRIOR-ART` | a claim about the literature/record. Weighted by the coverage honesty of its source.
`SPEC-AUDIT` | a specification-level defect read against live `src/anvil.cpp` guards. **Not** a claim that code is broken; the mechanism has no code.

Every quantitative claim below carries one of these tags. Where the constructive report (`11-…-space-bunny.md`) and I disagree, the disagreement is stated in §8 with the arithmetic that settles it.

---

## 1. Bottom line

1. **The constructive lane's gating quantity `H1` is not "unmeasured" — an instrumented decode-floor profile of the incumbent exists in this repo, on the exact primary cell.** Track 11 §1.2 lists `H1` ("per-token symbol coding is a material but unquantified share of mode-15 decode cost") as unmeasured and makes it the gate on its whole decode leg (`G1`, `G6`). `RESEARCH_LEDGER.md:3920-3931` contains an instrumented, interleaved, median-7, QPC+invariant-TSC profile of mode-15 on `generated.log` reporting measured shares of **end-to-end decode**: **crc32 44 %; eager materialization of the 7 macro streams 26 %; token loop beyond setup 11 %; concat/alloc/headers ~15 %**, annotated *"THE HOT PATH IS CLEAN; the floor is everything AROUND it."* Within that 11 % it further reports **opcode entropy pulls = 0.3 %**.
2. **The decode leg is therefore bounded by a hard whole-codec ceiling, and the projected effect sits below this project's own noise floor. Three facts, denominators kept exact:**
   * **(a) Hard ceiling.** All work FLI removes — entropy pull, varint read, symbol dispatch, state update — lies strictly inside "token loop beyond setup" = **11 % of whole-codec decode**. So the *absolute* best FLI could achieve is removing that entire pool: `1/0.89` = **1.124x**. That is an **upper bound**, not an estimate: the pool contains the `memcpy` copy throughput FLI's own inner loop preserves (constructive §4.2 keeps `emit_copy` inside the loop), so the realizable ceiling is **strictly below 1.124x**.
   * **(b) Measured component.** Opcode entropy pulls alone are **0.3 % of whole-codec decode** — i.e. **2.7 % of the 11 % pool**. This is the only part of the pool the profile separately attributes.
   * **(c) Tension and noise, not refutation.** The constructive projection is **≈5 %** whole-codec (§5.2); `G6` requires **1.08x** (a 7.4 % time reduction). A 5 % whole-codec saving sits **below this project's own recorded noise band** for this harness: within-run CV 0.6-10 %, run-to-run drift ±2-4 %, and **zero-byte-change control variants measured +2.3-8.1 %** (same source). The decode leg is therefore **noise-limited**: a real 5 % effect is not resolvable by the instrument that produced this profile, and cannot be certified under `docs/GITHUB-ACTIONS-BENCHMARKING.md`'s "a small timing delta on a shared runner is inconclusive" standard.
   * **Explicit non-claim.** I do **not** assert that `G1` is mathematically falsified. `G1`'s bar is defined as **(entropy + dispatch) / per-iteration token cost** — a ratio *inside* the token loop. The profile reports the **entropy-pull** term inside that loop (0.3 % whole-codec); it does **not** separately isolate dispatch, so **0.3/11 = 2.7 % is not an upper bound on `(entropy+dispatch)/token-cost`**. Dispatch is precisely the term FLI exists to remove, and it is unreported. `G1` is **unresolved, not failed** — the honest consequence is that the decode leg **cannot be sized**, and its projected magnitude sits under the noise floor. **My KILL does not rest on `G1`/`G6`.**
3. **Even at its own optimistic byte bound, FLI cannot move the primary cell.** Track 11's own projection ceiling is −6.6 % at `H_op = 1.0` on a 452 KB Linux output (§5.1). Scaled to the frozen Windows cell: 175,550 B × 6.6 % = **11,586 B**, against a recorded deficit of **50,881 B** to brotli q9 (`RESEARCH_LEDGER.md:3828-3830,3845`, "starts 41 % BEHIND q9"). FLI closes **≤23 % of the gap** at the most optimistic point of its own hypothesis band, and ~7 % at the mid-band point. `DERIVED`, §4.4.
4. **The novelty claim has been reduced by cross-lane review to `S1`+`S2` only, and it is now a systems/unification claim rather than a primitive-novelty claim** (Track 20 §16.1: *"S2 survives only as a systems/unification claim"*; §16.3 item 2: *"Track 11 must drop S4 … and must not carry S3"*; §16.4: `S4` restated as "per-construct decode-cost gating is unclaimed" and downgraded). I draw the distinction the coordinator asked for explicitly, in §4.6: **`S2` is *not exactly anticipated as a wire instruction*** (no shipped codec instruction with FLI's exact semantics was retrieved; Track 20's own searches returned nothing and are a **gap**, with its `G9` demanding ≥2 independent full-text sources) — **but that is categorically different from *having defensible primitive novelty*.** What remains after `S3` and `S4` are removed is a **bytes-and-engineering** property: a fusion and dispatch schedule over an already-anticipated recurrence. The novelty axis therefore **cannot rescue a decode leg already bounded by the measured 11 % pool / 1.124x ceiling and the recorded noise band** (§4.3), nor a byte leg bounded at ≤30 % of the recorded frontier gap (§4.4).
5. **Two preregistered thresholds are invalid as written, independent of any measurement:** `G2`/`G3` are calibrated against an instrument (`loop_oracle` on a min-match-4 greedy trace) whose report itself labels its coverage figure *"an optimistic bound relative to what the production parse would admit"* (track 11 §8). A screen that over-reports its own metric cannot certify "≥40 % of match bytes". And `KITER_MAX`/`LEN_MAX` "occur **only as symbols** … with **no numeric value anywhere in the document**" (`19-…-space-bunny.md:592-596`), so the track's own probes are unrunnable and its `Lemma 3` is not a bound.
6. **I disagree with Track 19 on one point, and I say so explicitly: the `KITER_MAX ≤ 15` ceiling does *not* kill `G2`/`G3`.** Track 19 states it "would largely dissolve the mechanism, since the documented rationale is runs 'just above the byte-neutrality threshold'". That is rhetoric, not arithmetic. The byte-neutrality threshold is `k ≥ 1 + 2/H_op` (track 11 §5.1), which is `k ≥ 3…8` across the entire `H_op` hypothesis band — all **below 15**. `DERIVED`, §5.1. The cap binds on the *tail* (k>15) and on the *security* economics, not on the gates.
7. **A materially smaller alternative exists and is strictly better-controlled: give the existing mode-15 opcode stream an order-1 context instead of adding an ISA.** Zero new opcodes, zero new substreams, zero new decoder-visible state beyond one cached previous symbol, zero change to amplification class, and it uses machinery already shipped (stream mode 6 `StreamPull` context-switching, the mechanism behind the project's single largest measured ratio win, Exp. S: −8.3 %…−16.2 %, `RESEARCH_LEDGER.md:1495-1522`). Under a stated-but-unmeasured conditional-entropy hypothesis it can capture **more** of FLI's byte win than FLI itself. §6.
8. **VERDICT: KILL** the FLI micro-program ISA as a novelty-bearing or frontier-relevant mechanism. **Narrowed HOLD** on exactly one zero-ISA, byte-only, remote measurement (§6.3) whose purpose is to close the track with data rather than with argument. Explicitly **declining PILOT** and **PROMOTE-TO-REMOTE**.

---

## 2. Evidence map — what is measured and must not be re-burned

| ID | Fact | Tag | Source |
|---|---|---|---|
| E1 | Decode floor on `generated.log`, instrumented, interleaved, median-7, QPC+invariant TSC: **crc32 44 %** (1.86–1.91 ns/B, stable across files: 43.8 % log / 36.4 % json / 56.1 % jsonl / 31.4 % sqlite); **eager materialization of the 7 macro streams 26 %**; **token loop beyond setup 11 %**, of which **opcode entropy pulls 0.3 %**; **concat/alloc/headers ~15 %**. Annotation: *"THE HOT PATH IS CLEAN; the floor is everything AROUND it."* Absolute level 5–15 % below bench-suite protocol; **relative** deltas valid; within-run CV 0.6–10 %; run-to-run drift ±2–4 %; zero-byte-change control variants moved **+2.3–8.1 %** | `MEASURED` | `RESEARCH_LEDGER.md:3911-3931` |
| E2 | Frozen cell baselines: `anvil-hotop-rans` **175,550 B / 273.534 MB/s decode / 19.681 MB/s encode**; `brotli-q9` **124,669 B / 619.508 MB/s / 29.533 MB/s**. Ledger states Windows `generated.log` "starts **41 % BEHIND** q9" | `MEASURED` | `RESEARCH_LEDGER.md:3828-3830,3845` |
| E3 | `C_decode` calibration (median-7, real mode-15 stream content, ns/B): raw 0.10 bulk / 2.59–2.66 byte; rANS-4096 5.93–6.62; ctx-rANS ~7–8 pull / 4.6–5.3 mat. Verdict on incumbent constants: **"right ORDER, wrong GAPS"** | `MEASURED` | `RESEARCH_LEDGER.md:3883-3909` |
| E4 | Under λ=0.01 B/μs the decode term spans **0.02–0.66 B** on 10–20 KB streams — it "can decide only near-exact ties". The Linux trade's implied willingness-to-pay was **460.2 B/μs**, 4.66 orders of magnitude above λ; first candidates appear only at λ ≈ 44.6 B/μs. **Zero raw-flips were arithmetically certain.** | `MEASURED` | `RESEARCH_LEDGER.md:4002-4011` |
| E5 | Exp. Y (RePair/RLZ over mode-15 book streams): bytes **−10.31 %** (log), −10.76 % (jsonl), −4.15 % (json); **decode −8.1 % / −15.9 % / −11.0 %** (sqlite +4.6 %); encode **48–64× slower**. Root cause *measured*: "eager whole-buffer materialization + plain byte walk LOSES to the fused per-byte pulls". Dropped-idea classification: **implementation-era, not math** | `MEASURED` | `RESEARCH_LEDGER.md:3723-3781` |
| E6 | Mode-15 hot-op book: decode **1.17×/1.23×/1.25×/1.34×** vs fused mode-12 on log/json/jsonl/sqlite at near-preserved ratio; the ≥2× target **NOT met**; named remaining floor = "opcode-stream entropy decode + copy throughput" | `MEASURED` | `RESEARCH_LEDGER.md:1452-1478` |
| E7 | Exp. S context-switched rANS (stream mode 6): **−8.3 %…−16.2 %** bytes, decode −3 %…−10 %; "the largest single-mechanism ratio win in the project" | `MEASURED` | `RESEARCH_LEDGER.md:1495-1522` |
| E8 | Correction masks **~90 % unique** (top-32 cover 7–12 %); (k,slot) modal accuracy ~20 %; mode-13 topology coding **lost +13.5 %…+21.0 %** | `MEASURED` | ledger Exp. I (as cited by track 11 M6) |
| E9 | Exp. Z / ARI-REF: transmitted Δ on `synth-arith.bin` **89,363 B**; **implicit Δ +38.35 %** vs transmitted on the *same* 2,160 tokens; implicit *wins* −15.38 % only on an exact progression | `MEASURED` | `RESEARCH_LEDGER.md:2939-2974`; `prototypes/orbit_ariref/results_run3.txt:7-8` |
| E10 | Transformer-enabled bar: brotli q11 on delta+zigzag bytes of `synth-arith.bin` = **16,313 B** vs 87,013 B raw = **5.33×** | `MEASURED` | `docs/gate-priorart-audit-i8.md:70-77` |
| E11 | Prior-art rulings: transmitted parameter = **PRIOR ART**; self-referential copy-with-edits = **VCDIFF RFC 3284**; mask-only approximate match = patent art; A1–A4 provenance ablation **never executed since I2-5** | `PRIOR-ART` | `docs/gate-priorart-audit-i8.md:139-155,187-202`; `docs/priorart-tcopy-external.md:236-264` |
| E12 | For the arithmetic family, **transmitted step (12,921 B) < derived zero-bit step (12,936 B)**; the derived form is "unnecessary as well as non-exact" | `MEASURED` | `docs/gate-ruling-i9-pr5-position-derived.md:212-236` |
| E13 | Zero-bit derived parameters defensible **only where the transform is exact**; bounded-nonzero-residual derived parameters = engineering/adopt, no claim; AUDIT-7 causality: derivation inputs must exist before use | `MEASURED` | `docs/gate-ruling-i9-pr5-position-derived.md:37-43,84-93,98-114` |
| E14 | Decoder strictness in force: `1 ≤ blen ≤ block_size`; `total ≤ (in.size()/7+2)·2^26`; per-stream `raw_n ≤ 16·out_len+64`; `dist ∈ [1,out.size()]`; `len ≤ 65536` (`kSparseMaxLen`); mask-bit bounds; `popcount(mask)==residuals`; **full substream consumption** + `payload trailing bytes`; CRC-32 | `MEASURED` | `FORMAT.md:929-965`; `docs/decoder-audit.md:36-90`; `src/anvil.cpp:2911,2979-2981,3004` |
| E15 | Track 19 §10 FLI audit: **bounds NOT sufficient**. MG-1 block-end guard uses the addition form `pos+L0≤block_end` where the whole codebase uses subtraction `L0 ≤ block_end-pos` → fails open exactly when `L0` drifts. MG-2 no typing/cap/post-check on `δL`/`δD`; the house zigzag `(zz&1)? -int64_t((zz+1)>>1) : int64_t(zz>>1)` at `src/anvil.cpp:2947` **applies no cap to `zz`**. MG-3 `L_i≥1` per iteration mandatory. MG-4 three new substreams **not** added to the full-consumption contract → byte-laundering past J accounting, breaks the format-evolution invariant. MG-5 `K` is a per-block uvarint (`:2994`) capped `kHotMaxOps=254` (`:2797`) and the opcode stream is a **byte** stream (`:2915`) ⇒ `K+4` is unrepresentable for `K>251`. MG-6 escape-class bound must apply **post-accumulation**. MG-7 `KITER_MAX`/`LEN_MAX` are unpinned symbols. MG-9 `memcpy` forbidden when `D0<L_i` | `SPEC-AUDIT` | `19-format-security-space-bunny.md:498-658`; `src/anvil.cpp` cited therein |
| E16 | Track 20 §9: **S3 ANTICIPATED AT THE ABSTRACT LEVEL** (ISLP, Jeż/Navarro/Olivares/Urbina, LATIN 2024 — iteration rules `A → ∏_{i=k1}^{k2} B_i^{c1}···B_t^{ct}`, exponents linear in `i` = exact arithmetic recurrence, zero per-iteration symbols, bounded non-recursive expansion = FLI `LOOP_ARITH`). **S1 NOT ANTICIPATED but clearance is a GAP** (LZO's cross-instruction `state` occupies the zero-bit-inheritance sub-slot). **S2 UNVERIFIED and now load-bearing.** **S4 UNVERIFIED**, and the repo's own λ·`C_decode` is mis-scaled (E4). Track 20's own `G9` requires S1 **and** S2 recorded NOT-ANTICIPATED with ≥2 independent full-text sources each | `PRIOR-ART` | `20-priorart-killteam-space-bunny.md:401-499,616` |
| E17 | Format-constant value that caps the ISA growth: `kHotMaxOps = 254`; live dispatcher uses exactly `0..K` (`op<K` at `:2917`, `op==K` at `:2929`, else throw at `:2977`), so `K+1…` are unused slots today | `MEASURED` | `src/anvil.cpp:2797,2915-2917,2929,2977,2994` |
| E18 | `generated.repeat.jsonl` control: every codec 0.000–0.001; `random.bin` ≈ 262,166–262,179 B unchanged is a standing encode-time negative gate | `MEASURED` | `docs/CONTEXT.md`; `RESEARCH_LEDGER.md:3851-3853,3953` |

**Gaps I must name (not in the record):**

- No measured conditional (order-1) entropy of mode-15's opcode stream. Its unconditioned value `H_op` is likewise unmeasured. Both are byte-only and cheap (§6.3).
- No measured histogram of admissible-run length over the **production** parse trace. The only oracle specified uses a greedy min-match-4 trace that its own author labels optimistic.
- No measured `K` (book size) distribution over real blocks — needed to price the MG-5 byte cliff.
- Brotli's 2026 "learned/program-synthesis" claims (Brevis 30.87 %/6.61 GB/s; Pcodec; OpenZL; AIT seed-cracking entrants) remain **operator-reported and unverified**; `docs/gate-priorart-audit-i8.md:174-181` says they are "uncitable as evidence". I do not cite them.

---

## 3. Is the *track-level* premise still alive? (Orbit / program-synthesis compression, above FLI)

The mandate is the orbit/program-synthesis *family*, not only FLI. I audit the family and find it is closed on five axes, with one narrow survivor.

| Family direction | Status | Basis |
|---|---|---|
**(1) Bounded micro-program VM over bytes** | **KILL** | Grammar-with-loops is prior art (RePair Larsson–Moffat 1998; SEQUIT; XMill/MillView; ISLP per E16), *and* the one measured instance in this repo is **decode-negative**: Exp. Y lost 8–16 % decode (E5). A novelty review cannot rescue a representation that is already measured slower here. |
**(2) Transmitted-parameter program ISA** | **KILL** | E11/E12: transmitted parameters are prior art, foreclosed twice in this project, and in the one measured arithmetic cell transmitted **beat** the zero-bit form. The project's own audit names the repeated error: "the project has twice built the expeditious form and called it progress" (`docs/gate-priorart-audit-i8.md:196-199`). |
**(3) Estimator-derived implicit parameters (any width, any residual bound)** | **KILL** | E13: MATH-class closed (0.646–0.691 bits/doubling residual growth); bounded-residual variants are engineering-only; the transmitted control is smaller (E12). |
**(4) Mask / correction-topology cloning** | **KILL** | E8: masks ~90 % unique; mode 13 lost 13.5–21 %. |
**(5) Gram/Orbit search as the contribution** | **KILL as novelty** | `docs/FRONTIER-RESET-2026-09-23.md:701-718` already concedes "compression is a graph/program of transforms" is established (OpenZL, grammar compressors, bidirectional macro schemes, generalized dedup, reconstruction systems) and that ANVIL "should not claim novelty for a graph or IR". Search-as-discovery is already owned by TCOPY/PNRA (invariant indexing) and by H2 (decoder-derived transform sites). |
**(6) Decoder-*execution* axis: zero-symbol counted repetition over compressed token semantics** | **see §4–§5** | FLI. The only unoccupied *slot*, but the slot's contents are (a) already anticipated abstractly (E16/S3), (b) bounded to a security-fixed ceiling that is above but not far above the byte-neutrality threshold (§5.1), and (c) measured to have ≤0.3 % entropy-pull headroom on the target cell (E1). |

**Kolmogorov-search impracticality (the mandate's explicit attack surface).** FLI is the *good* case here and I will not over-claim against it: track 11 §4.1–§4.2 makes encoder passes P2/P3 strictly `O(N)` with a constant under ~10 ops/token and **4 registers**, no index, no superlinear structure, and explicitly no global DP — consistent with the project's standing "single-pass, cache-resident parser" agreement. A genuine A*/BFS grammar search (the Brevis shape) would be a real encoder-time disaster and I treat its exclusion as correct. **However**, two residual Kolmogorov-side costs survive and are *not* in the constructive report's ledger:

- **The encoder must materialize a per-token trace before the recurrence scanner can run.** P1 is described as "the existing parse → token trace `T = [t_1..t_N]`", and P2 then walks `T` requiring each token's decoder-side effect `(kind_i, len_i, shape_i, dist_i)`. That is an `O(N)` **side buffer** proportional to the token count, i.e. `Θ(N_tokens)` additional working memory and a second full pass over it. This is precisely the *shape* of Exp. Y's measured failure ("eager whole-buffer materialization … LOSES to the fused per-byte pulls", E5) and it sits in direct tension with track 11's own claim (§5.3) of "no new data structures beyond 4 registers". Either the trace already exists in cache (unverified — the parse currently streams into the entropy coder) or the claim is false. `SPEC-AUDIT`, unmeasured.
- **Search-vs-verification inversion is not claimed and cannot be.** Under E16/S3 the abstract mechanism is prior art; what remains is an implementation. See §4.6.

---

## 4. Hidden-cost audit, with the arithmetic

### 4.1 Decoder-visible bytes FLI adds (recomputed independently)

Track 11 §5.1 charges `≤30 B/block` for `≤4` new alphabet symbols plus `~12 B/block` for three stream headers. My independent recomputation, using E17 (`kHotMaxOps=254`) and E14 (each substream carries a mode byte + length varint, and its length is bounded relative to `out_len`):

| Term | Track 11's charge | My independent charge | Tag |
|---|---|---|---|
`LOOP_EQ` header | 1 opcode + `uvarint n` | 1 opcode (`H_op` B) + 1–2 B `n` | `DERIVED` |
`LOOP_ARITH` header | + δL, δD | + 1–2 B each (zigzag varint, no cap per E15/MG-2) | `DERIVED` |
3 new substreams | ~12 B/block | 3 mode bytes + 3 length varints (1–3 B each) = **12–24 B/block**, before any model/frequency header the selected per-stream codec may add | `DERIVED` from `FORMAT.md:929-965` |
`loop_class` in block header | uvarint once/block | 1–2 B/block, plus **one more block-header field that changes the block header's own length** | `DERIVED` |
new alphabet symbols | ≤4 codes ≈ ≤30 B | **conditional on `K`**: representable only for `K ≤ 251`; for `K ∈ [252,254]` the block must be *rejected* (E15/MG-5) or `kHotMaxOps` lowered — which **changes the incumbent's book capacity**, i.e. it is a change to the *baseline*, not just to FLI | `MEASURED`+`DERIVED` |
amortization on `generated.log` at 64 KiB blocks | not stated | 1,942,280/65,536 = **29.6 → 30 blocks** ⇒ fixed cost ≈ **0.7–1.7 KB** on a 175,550 B output = **0.4–1.0 %** of the cell | `DERIVED` |

**Finding (hidden cost 1).** FLI's *fixed* per-block cost alone is **0.4–1.0 % of the primary cell's output** (`DERIVED`). Track 11's byte ledger (§5.1) charges only the per-loop terms and states the alphabet cost as "≤30 B per block" without amortizing it. On a cell whose total projected win is 2.0–6.6 % (§4.4), a 0.4–1.0 % fixed tax consumes **6–50 %** of the projected benefit depending on where in the band the truth lands. The tax is not fatal; it is **unstated**, which under doctrine 2 ("charge every decoder-visible byte") is a defect in the ledger, not in the mechanism.

**Finding (hidden cost 2 — the severe one).** E15/MG-4: FLI adds three materialized substreams and does **not** extend the block's full-consumption contract (`src/anvil.cpp:2979-2981`, `:2911`). Bytes hidden in an unused remainder of `S_n`/`S_dl`/`S_dd` are then **accepted and silently discarded**. This is not primarily a security bug — it is an **accounting** breach: those bytes are decoder-visible-in-the-file, present in no rate objective, and invisible to CRC-over-decoded-bytes (which cannot see them by construction). For a track whose entire thesis is byte economics, an unaudited byte sink in its own wire format is disqualifying until fixed. It is cheap to fix now (add the three cursors to the consumption tuple) and expensive later. I adopt MG-4 as a **precondition**, agreeing with Track 19.

**Finding (hidden cost 3 — decoder binary).** Track 11 does not price decoder code size. Honest independent estimate: two fused inner loops (bulk-insert vs byte-at-a-time `push_back`, E15/MG-9), per-iteration `1 ≤ L_i ≤ 65536` and `D_i ∈ [1,0xFFFFFFFF]` post-checks, subtraction-form block-end guard, int64 drift accumulation, plus 3 stream cursors and a 4-case switch ≈ **200–350 B of x86-64 Release code**, against the frozen 370,688 B `anvil.exe` = **+0.05–0.09 %**. **This is negligible and I decline to inflate it.** Under `docs/FRONTIER-RESET-2026-09-23.md:766` ("if an opcode book saves 80 KB but adds 100 KB to the universal decoder binary…") the 2 KB RSS/`D_c` axes are not where FLI dies.

### 4.2 Decode cycles — recomputed, and the arithmetic that settles it

Track 11 §5.2 charges per iteration: expanded `6.0 + 2.0 + 1.0 + M(L)` vs FLI `3.0 + M(L)`, saving **≈8 ns/iteration independent of L**, with `M(L)=max(2.0, 0.12·L)` labelled a projection needing confirmation. My independent restatement:

- The 8 ns saving is dominated by `c_pull + c_dispatch + c_state = 9 ns`, of which `c_pull` is an **entropy pull**. E1 measures opcode entropy pulls at **0.3 %** of end-to-end decode. If the saving were dominated by `c_pull`, the saving could not exceed 0.3 % of decode; the constructive report's own 0.20 ns/B of 3.66 ns/B (= **5.5 %**) figure therefore must come from `c_dispatch + c_state`, which E1 does **not** separately report. That is an unlabelled dependency in the projection.
- **I decline to ratify the projection and substitute the two things that *are* measured:** the hard ceiling `1/(1-0.11) = 1.124x` (§4.3a) and the noise band a ~5 % effect must beat (E1's own zero-byte-change control variants, +2.3-8.1 %). Note the consequence for `G1`: because dispatch is unmeasured, `P1c` is **not** superseded by E1 — it remains the right experiment, and my §8 R1 records that I withdraw any framing of `G1` as already decided.

### 4.3 Decode leg — exact denominators, hard ceiling, noise limit

Let `T` = 1 s of end-to-end decode on `generated.log`. By E1, measured whole-codec shares: CRC 0.44, macro-materialization 0.26, **token loop beyond setup 0.11**, concat/alloc/headers ~0.15, residual ~0.04. `DERIVED`.

**(a) Hard whole-codec ceiling.** Every component FLI removes — entropy pull, varint read, symbol dispatch, per-token state update — lies inside the 0.11 pool. Removing *all* of it gives `1/(1-0.11)` = **1.124x**. That is an **upper bound**, not an estimate: the pool also contains the copy throughput FLI's own inner loop preserves, so the realizable ceiling is **strictly below 1.124x**. `DERIVED` from `MEASURED` E1.

**(b) The one measured component.** Opcode entropy pulls = **0.3 % of whole-codec decode**, i.e. `0.003/0.11` = **2.7 % of the pool**. `MEASURED` (E1); ratio `DERIVED`.

**(c) What `G1` is and is not — stated precisely, because the denominators differ.** `G1` is defined on a *different denominator*: `(entropy + dispatch) / per-iteration token cost`. The profile reports the **entropy-pull** term inside the token loop; it does **not** isolate dispatch. Therefore:

- `0.3 %` is **not** an upper bound on `(entropy+dispatch)/token-cost`.
- `2.7 %` is the entropy-pull **share of the pool**, not a bound on `G1`'s numerator.
- **Dispatch is precisely the term FLI exists to remove, and it is unreported.** `G1` is **UNRESOLVED, NOT FALSIFIED.** I record this as a gap in the *constructive* lane's favour-plan: `P1c` is the right experiment and I withdraw any framing of `G1` as already decided.

**(d) The tension that does survive — resolvability, not refutation.** `G6` requires **1.08x** = a 7.4 % time reduction. The constructive projection is **≈5 %**. The hard ceiling is 1.124x. So the projection consumes **≈68 % of the entire available headroom** (5/7.4) while leaving the copy-dominated majority of the pool untouched — i.e. the projection is optimistic against its own ceiling. More importantly, **a 5 % whole-codec saving is below this project's own measured noise band**: within-run CV 0.6-10 %, run-to-run drift ±2-4 %, and **zero-byte-change control variants moved +2.3-8.1 %** (E1 provenance). A 5 % effect is **not resolvable** by the instrument that produced E1. **The decode leg is noise-limited, not refuted.** `DERIVED`.

**(e) Anti-correlation corollary (holds regardless of how `G1` resolves).** S6-1b (CRC-32 bytewise → slicing-by-8 → PCLMUL) is pre-registered and measured at 3.4x decode on the store path (`RESEARCH_LEDGER.md:5513-5552`). When CRC's 0.44 shrinks toward ~0.5-0.9 ns/B, the token loop's *share* rises from 11 % to roughly `0.11/(1-0.44+0.07)` ~ **20 %**, while FLI's *absolute* ns saving is unchanged. The lane that actually moves decode **inflates FLI's denominator**: the same absolute improvement becomes a *smaller* fraction of a faster decode. **FLI is anti-correlated with the project's real decode win.** `DERIVED`.

**(f) Why the ceiling is economically irrelevant for the frontier, even if the noise is beaten.** Suppose the full 1.124x were achieved — an upper bound, not a forecast. The frozen cell has `anvil-hotop-rans` decode at **273.534 MB/s** and brotli q9 at **619.508 MB/s** (E2). At 1.124x, ANVIL reaches **307.5 MB/s** — still **50 % short** of q9. The ceiling therefore cannot produce a decode-plane crossing even if fully realized, and `G6`'s own 1.08x (295.8 MB/s) is farther still. **The decode leg is not merely noise-limited; its absolute ceiling is frontier-irrelevant.** `DERIVED`.

**(g) Why this still does not rescue the track.** Grant `G1` every benefit of the doubt and assume dispatch is large: the ceiling is 1.124x, the projection is noise-limited, and the ceiling itself is frontier-irrelevant (f). And the **byte** leg — deterministic, citation-grade, free of all this measurement ambiguity — independently fails §4.4. A mechanism whose *certain* leg fails cannot be promoted on the strength of its *uncertain* leg.

### 4.4 The byte leg — recomputed against the actual frontier gap

Track 11 §5.1: `Δ bytes/loop = 2 − (k−1)·H_op`; byte-neutral at `k ≥ 1 + 2/H_op`; projected whole-file win **0 %…−7 %**, width set entirely by the unmeasured `H_op`; and at `k=8`, `H_op=0.5` → ≈−8.9 KB on 452 KB (−2.0 %); at `H_op=1.0` → ≈−30 KB (−6.6 %). `PROJECTION` in its own report; I treat the band as unmeasured.

On the frozen Windows cell (E2):

| `H_op` | k | projected Δ | × 175,550 B | vs 50,881 B gap to q9 |
|---|---|---|---|---|
0.5 B | 8 | −2.0 % | −3,511 B | **6.9 %** of gap closed |
1.0 B | 8 | −6.6 % | −11,586 B | **22.8 %** of gap closed |
1.5 B | 8 | ≈−9.2 % | −16,150 B | 31.7 % of gap closed |
— | — | fixed per-block tax (§4.1) | −0.7…−1.7 KB | −1.4…−3.3 % of gap given back |

`DERIVED`. **Finding:** even at the top of its own hypothesis band, and after paying its own fixed cost, FLI is worth ~30 % of the primary cell's deficit. A mechanism that leaves ~70 % of the recorded gap untouched is **not a crossing instrument** on that cell, and there is no cell in the frozen tuple where a ~2–7 % move is decisive — `docs/anvil-i9-findings.md:189-213` records the canonical tuple as `33 non-dominated | 5 FRONT-GAP | 0 FRONT-CROSSING | 28 DEGENERATE | 435/468 dominated`, with `GRID-THIN` binding.

### 4.5 Overfit and provenance of the *instrument*

- The only screen specified for `G2`/`G3` runs on a **greedy min-match-4** trace (`loop_oracle`, track 11 §8), which the report itself says "is **not** ANVIL's MDL-gated parse; consequently **its coverage figure is an optimistic bound**". `G2` = "≥40 % of match bytes … inside admissible runs", `G3` = "median admissible run ≥8". **A threshold certified by a knowingly optimistic instrument is not a threshold.** Additionally, E6 measured that mode-15's actual hot coverage is 75–77 % of *tokens* (Linux, stale-flagged) — a different quantity from "match bytes inside runs" — so the two are not even commensurable as written.
- `P1a` (compute `H_op`) is the one part of the plan that is simultaneously (a) byte-only, (b) deterministic, (c) citation-grade, and (d) needed by *any* evaluation of FLI's byte leg. It should run regardless of FLI's disposition. §6.3.
- **Threshold-stability doctrine breach.** `KITER_MAX`/`LEN_MAX` are unpinned symbols (E15/MG-7), yet `G3`, `Lemma 3`, and the §11 probes all reference them. Under doctrine 5 ("do not move thresholds after seeing results") a gate whose threshold is a symbol cannot be honoured: whoever pins it after `P2` reports a run-length histogram will be accused, correctly, of having tuned a security parameter to fit a measured ratio distribution. **The cap must be pinned before P2 runs, from the amplification ceiling alone.** §5 gives the derivation I would accept.

### 4.6 Novelty, restated against the surviving separator

With S3 anticipated (E16), the claim reduces to: *"a counted loop over compressed token semantics, executed fused in the consumer without materializing an intermediate representation."* My adversarial read, and the reason this is not a mechanism:

1. **Materialize-vs-fuse is an implementation choice, not a mathematical one.** The same instruction stream decodes identically either way; only the memory traffic differs. The repo's own classification of the one measured instance is explicit: Exp. Y's loss is "**implementation-era** (the ratio mechanism is real and measured; the decode/encode economics fail on this host/build), **not math**" (`RESEARCH_LEDGER.md:3778-3781`). By that standard S2 is an implementation-era property and cannot carry mechanism novelty.
2. **The constructive report's own strongest evidence is against S2-as-novelty.** Its S2 bullet cites *Exp. Y* — this repo's own negative — as the proof that fusion matters. That is evidence that fusion is a **real engineering lever**, i.e. evidence *for* adopt-class value and *against* mechanism novelty. The bullet is self-undermining as a novelty separator.
3. **S1/S2 clearance is a gap, not a negative, by the standard Track 20 itself adopted.** Its `G9` demands ≥2 independent full-text sources each; its queries returned LLVM `LoopFuse` (irrelevant, self-recorded) and no codec hit. It also flags the unsearched territory — patents, proprietary console/embedded packers, the 18-month blackout (`docs/priorart-tcopy-external.md:170-176`). Under `docs/gate-priorart-audit-i8.md:203-218` ("a gap, not a negative"), S1/S2 are **not cleared**.
4. **What remains after S3 and S2 is not novel** either: "operands inherit from prior decoder state at zero bits" is occupied by LZMA rep0–rep3, zstd repeat-offsets, Brotli's distance cache, and — per Track 20's own retrieval — **LZO's documented cross-instruction `state`**.

**Both branches of the fork terminate in non-promotion.** (i) If `S2` **is** anticipated by a shipped coder — the open search Track 20 has not run — FLI is anticipated and the kill is immediate with no measurement needed. (ii) If `S2` is **not** anticipated, FLI still has no defensible *primitive* novelty, because what remains is a fusion/dispatch schedule over an already-anticipated recurrence, and the repo's own classification of exactly this distinction is "implementation-era, not math" (`RESEARCH_LEDGER.md:3778-3781`). **I do not need the literature search to reach the verdict; it can only accelerate it.** And on this cell the bytes and cycles do not both win anyway — the decode leg is bounded at 1.124x and frontier-irrelevant (§4.3f), and the byte leg independently fails (§4.4). §8.1 BLOCKER 3 states this distinction formally.

---

## 5. Security economics: does `KITER_MAX ≤ 15` kill the gates?

Coordinator's direct question, answered independently rather than adopted.

### 5.1 Derivation of the cap (I accept the direction, and derive the number)

Track 19 §10.3 argues: FLI's inner loop performs "no entropy decoder call, no varint read, no dispatch on a symbol" (track 11 §4.2), so a ≈3-byte loop header buys up to `KITER_MAX × LEN_MAX` bytes of work at the cheapest cycles-per-output-byte in the format; the only defensible `LEN_MAX` is `kSparseMaxLen = 65536` because the book `len` is already validated to `[1, 65536]` (`src/anvil.cpp:3004`); hence work-per-input-byte `= KITER_MAX × 65536 / 3`, versus `≈3.4×10^5×` for the existing RLZ path ⇒ `KITER_MAX ≤ 15`. **I re-derive and accept this arithmetic.** `DERIVED` from `MEASURED` E14 + the spec.

**Two independent confirmations of the direction:** (a) E15/MG-1 shows the block-end guard must be the subtraction form `L_i ≤ block_end − pos` for FLI's *drifted* `L_i`, so the cap is the only remaining magnitude limit; (b) E15/MG-3 requires `1 ≤ L_i ≤ LEN_MAX` **per iteration**, so a `δL = −1` stream yields `KITER_MAX` iterations of dispatch with zero bytes produced — the cap is directly the DoS bound.

### 5.2 Does the cap kill `G2`/`G3`? **No.** (`DERIVED` — I disagree with Track 19 here.)

- `G3` = median admissible run ≥ 8. Cap 15 ≥ 8. The gate is *numerically* unaffected.
- `G2` = ≥40 % of match bytes inside runs of k ≥ 4. Cap 15 ≥ 4. The gate's run-length floor is unaffected.
- The byte-neutrality threshold is `k ≥ 1 + 2/H_op` (track 11 §5.1) = **k ≥ 3** at `H_op = 1.5`, **5** at `0.5`, **8** at `0.3`. The whole `H_op` hypothesis band sits at `k ∈ [3,8]`, entirely below 15.
- Truncation cost for a long run: a run of length `k` under cap 15 must be cut into `⌈k/15⌉` loops. Each seam costs ≈ 1 opcode + 1 `n` varint + (for `LOOP_ARITH`) one `(δL, δD)` pair ≈ `H_op + 3` bytes. For `k = 40`: 3 seams ≈ `3(H_op+3)` ≈ 10.5 B at `H_op=0.5`, i.e. 0.26 B/iteration against a per-iteration saving of 0.5 B — **the cap costs ~50 % of the marginal win on long runs and 0 % on runs of 8–15**, which §4.4 shows is the operative region.
- Track 19's phrase "which would largely dissolve the mechanism" is therefore **not supported by the mechanism's own byte formula**. What the cap *does* dissolve is Track 19's *amplification* claim: with `KITER_MAX = 15`, FLI's worst case is `15 × 65536 / 3 = 3.3×10^5×`, i.e. **on par with, not above, the existing RLZ path**. So the cap converts the security objection from "new amplification class" to "must be metered at parity".

### 5.3 What the cap *does* kill — three concrete items

1. **`KITER_MAX` can never be a ratio knob.** Its value is fixed by the format's amplification ceiling. Every proposal to raise it is a deliberate, documented, preregistered format-security decision. `HYPOTHESIS→now doctrine` per E15 §10.3.
2. **The `ρ` resource-meter row becomes a precondition, not follow-up** (E15 §10.3 cross-lane obligation): FLI must be charged `n × L_i` bytes, `n` dispatch iterations, and `n × (periodic-vs-bulk class)`, at the same rate as the tokens it replaces. Otherwise FLI is the format's cheapest amplification primitive and every bound in `FORMAT.md:929-965` must be re-derived around it. **I adopt this.**
3. **The tail is unexploitable, which caps the byte ceiling below §4.4's own projection.** Runs longer than 15 pay seams for zero additional saving, so the realistic `k` distribution is truncated at 15 — which further erodes an already-insufficient ~2–7 % projection.

### 5.4 Dynamic opcode allocation near `K = 254` (coordinator's second question)

Independently audited against E17 (`src/anvil.cpp:2797` `kHotMaxOps = 254`; `:2994` `K = get_uvar(p,e)` per block; `:2915` `for (uint8_t op : s[0])`; `:2917` `op < K`; `:2929` `op == K`; `:2977` else throw):

- **No collision today.** FLI's `K+1…K+3` are genuinely unused slots; the dispatcher dispatches `0..K` and throws otherwise. Credit where due — this part of the constructive report is sound and Track 19 says so.
- **The cliff is real and is a wire defect in the spec.** The opcode alphabet is a **byte** stream, so the maximum representable code is 255. FLI's `K+4 → class+1` escape requires `K + 4 ≤ 255`, i.e. **`K ≤ 251`**. For `K ∈ [252, 254]` the specified opcodes do not exist. Two admissible repairs: (a) reject any block declaring loop opcodes when `K > 251`; (b) lower `kHotMaxOps` to 251 — which **reduces incumbent book capacity** and is therefore a change to the *baseline*, requiring its own byte-identity argument. (a) is the correct choice.
- **Economic consequence, which is the part that matters for the verdict.** Rejecting loop opcodes for `K > 251` means FLI is **silently unavailable** exactly in the blocks with the largest books — i.e. the blocks with the most hot classes and plausibly the longest admissible runs. Alternatively, capping the book at 251 pushes the encoder toward **splitting blocks** to regain class headroom, which costs header bytes and changes `last[]` reset semantics (E15: FLI never crosses a block boundary, track 11 §11 A11) and so *reduces* admissible run lengths. **Both repairs attack the very runs `G3` measures.** Neither cost is in the constructive report's byte ledger. `DERIVED`/`HYPOTHESIS`.
- **Plus `K`'s per-block nature invalidates an unbounded claim.** "≤4 codes ≈ ≤30 B per block" (track 11 §5.1) is a *constant*, but `K` is read per block and the alphabet cost is only bounded if `K` is capped below 251 — which is itself the change above. The three claims "`K ≤ 254`", "adding ≤4 loop codes", and "bounded, charged" cannot all be true simultaneously. `SPEC-AUDIT`.

---

## 6. Alternative mechanism (materially smaller, better controlled)

### 6.1 Why the *smaller* thing is better, on this repo's own numbers

FLI's byte win comes from the fact that `k` consecutive tokens are identical, so their `k` opcode symbols are redundant. **A redundancy in a coded symbol stream is, by definition, a job for the entropy model, not for the ISA.** The existing mode-15 opcode stream is a byte stream under a per-stream suite that already includes context-switched coding (`FORMAT.md:614-630`, stream mode 6; Exp. S, E7). Giving that stream an **order-1 context — "is this opcode equal to the previous opcode?" — costs one cached previous symbol and one conditional frequency lookup, and changes no byte count, no opcodes, no substreams, no block header, and no amplification class.**

### 6.2 AOC — "opcode-repeat context" (proposed control, not a claim)

| Property | FLI | AOC |
|---|---|---|
New opcodes | 3–4 | **0** |
New substreams | 3 | **0** |
New decoder state | 7 scalars + 3 cursors | **1 cached symbol** |
Block-header change | yes (`loop_class`) | **no** |
Amplification class change | no (after `KITER_MAX ≤ 15`) | **none at all** |
Format-evolution invariant (`FORMAT.md:782`) | needs MG-4 fix | **untouched** |
Decoder binary | +200–350 B (`DERIVED`) | **+0** |
Requires Track 19 sign-off | yes (MG-1…MG-9, `ρ` row) | **no** |
Guards needed beyond existing | 6 new | **0** |
Prior-art status of the *idea* | E16: S3 anticipated, S2 unverified | **pure adopt-class, openly labelled** |
Byte win (hypothesis) | 0…−7 % | see below |

**The byte comparison is the point, and it is a hypothesis I state explicitly rather than a result.** `HYPOTHESIS`: for a run of `k` identical hot opcodes, unconditional cost `≈ k·H_op`; order-1-conditional cost `≈ H(same|same)·(k−1) + H(first)`, where `H(same|same)` is plausibly in **0.1–0.3 B** if the repeated opcode is the modal symbol of the stream. Under that hypothesis, at `k=8`, `H_op=0.5`: FLI saves `(8−1)·0.5 − 2 = 1.5 B`; AOC saves `7·0.5 − (7·0.2 + 0.5) = 3.5 − 1.9 = 1.6 B`. **AOC ≥ FLI.** And if `H(same|same)` is lower — which is what a strongly repetitive stream implies — AOC wins by more. `DERIVED` under a stated, unmeasured assumption; **this is precisely why it must be measured, and the measurement is byte-only, deterministic and trivial** (it is a conditional-entropy pass over the opcode stream, i.e. a two-line extension of the already-planned `P1a`).

### 6.3 The one experiment I will fund (narrowed HOLD scope)

A single remote, byte-only, R1-tier job. No mechanism build, no ISA, no new decoder-visible state, no new streams. Corpus and protocol per `docs/GITHUB-ACTIONS-BENCHMARKING.md` §2/§5/§8.1/§9 and the I10 ladder; primary cell `generated.log`; paired with Silesia, enwik8, the locked `u3` PE set, and `generated.{json,jsonl,sqlite}`.

**Report exactly six numbers, per file:**

1. `H_op` — unconditioned opcode entropy (bits/token) of the frozen mode-15 opcode stream.
2. `H(same | same)` — conditional entropy of the opcode given it equals its predecessor; plus `H(same | diff)`, so the model's *loss* is visible, not assumed.
3. `AOC` byte delta vs the frozen baseline, applied **as a pure re-modeling of the existing opcode stream** (no code change beyond the stream-mode selection that already exists).
4. `FLI` byte delta under track 11's own formula at the four `H_op` points, with the §4.1 fixed-cost tax (0.4–1.0 % on the primary cell) **subtracted**, and with runs truncated at `KITER_MAX = 15`.
5. **Admissible-run histogram over the PRODUCTION parse trace** (not the greedy oracle), reporting match-byte coverage and median run length. This replaces the invalid `G2`/`G3` instrument.
6. `K` distribution (book size per block) with the count of blocks at `K > 251` — pricing the §5.4 cliff.

**Pre-registered thresholds (fixed before the job is dispatched; not movable after):**

| Gate | GO | NO-GO |
|---|---|---|
**F1** (decode thesis) | - | **NO-GO on the hard ceiling and on frontier relevance**, decided before measurement from `MEASURED` E1: all FLI-removed work is inside the 11 % pool ⇒ ceiling **1.124x**; and 273.534 MB/s × 1.124 = **≈307 MB/s vs q9's 619.508 MB/s**, so even a fully-realized ceiling is frontier-irrelevant (§4.3a/f). **Explicitly NOT a NO-GO on `G1`**, which is unresolved because dispatch is unmeasured (§4.3c). FLI may not be funded on a decode thesis. |
**F2** (`H(same|same)`) | `≤ 0.30 B` at `H_op ≥ 0.5 B` | `> 0.45 B` ⇒ the repetition is already cheap under the unconditional model and **neither** FLI nor AOC has a byte thesis |
**F3** (AOC vs FLI) | `ΔAOC ≤ ΔFLI − 1.0 %` of cell bytes | `ΔAOC ≥ ΔFLI` ⇒ **FLI is adopt-class at best and AOC is strictly cheaper** ⇒ FLI **KILL**, adopt AOC |
**F4** (coverage, production trace) | `≥ 40 %` of match bytes in runs `k ∈ [4,15]` on ≥3/4 record files | below ⇒ `G2` fails on a valid instrument and the tail argument is moot |
**F5** (frontier relevance) | `ΔAOC` or `ΔFLI` closes `≥ 25 %` of the recorded per-cell gap to the co-listed q9 row | below ⇒ **no crossing instrument on that cell**, regardless of mechanism |
**F6** (book cliff) | `≤ 1 %` of blocks have `K > 251` | above ⇒ the §5.4 repair path is materially expensive and must be re-priced |

`F5` is the gate I would not have written if I were the constructive lane, and it is the one that matters: it converts "did the mechanism work" into "could the mechanism have mattered", which is the question the project has repeatedly failed to ask before funding a mechanism build.

**Any single F2–F6 NO-GO ⇒ FLI is KILL as a mechanism and the track closes on measurement.** All six GO ⇒ the honest disposition is *adopt-class ratio engineering on the AOC side*, still not a novelty claim, and still not a crossing (F5 bounds the gap closure).

---

## 7. Strongest falsification case, stated as the coordinator would want it

> **Track 11 proposes to add three opcodes and three substreams to a decoder whose instrumented profile shows that all of the work it removes lives inside a pool measured at 11 % of whole-codec decode — an absolute ceiling of 1.124x, of which the copy throughput FLI preserves is a large share, and whose projected ≈5 % effect is smaller than this project's own measured noise band (zero-byte-change controls moved +2.3–8.1 %). Even that ceiling is frontier-irrelevant: fully realizing it would take the frozen cell from 273.534 MB/s to ≈307 MB/s, still ~50 % short of brotli q9's 619.508 MB/s. Its certain leg is the byte leg, and that leg closes at most ~30 % of a recorded 50,881 B deficit to q9 on the primary cell, before paying its own 0.4–1.0 % fixed per-block tax. Its novelty, after `S3` is anticipated (ISLP) and `S4` is downgraded, is a systems/unification claim about a fusion schedule — not exactly anticipated as a wire instruction, but with no defensible primitive novelty and no frontier-relevant magnitude to attach it to. Its thresholds `G2`/`G3` are certified by an instrument its own author labels optimistic, and its `KITER_MAX` is an unpinned symbol, which makes its own falsification probes unrunnable. The mechanism cannot be a frontier instrument on the cell where it was designed, and cannot carry a novelty claim anywhere.**

**Note on what this case deliberately does not claim.** It does not claim `G1` is falsified (§0.2(2c), §4.3(c)); `G1` is unresolved because dispatch is unmeasured. It does not claim `S2` is anticipated; Track 20 correctly declines that. It does not claim any performance number of my own; every figure is `MEASURED` with a cited protocol or `DERIVED` from such.

## 8. Reconciliation with the paired constructive report

| # | Constructive position (`11-…-space-bunny.md`) | My position | Resolution |
|---|---|---|---|
R1 | `H1` is **unmeasured**; `P1c` must run first; `G1` gates the decode leg (`:26-29`, `:64`, `:450-456`, `:560-564`) | **Partly agree — and I correct my own earlier overstatement.** E1 bounds `H1` from above (all FLI-removed work sits inside the measured 11 % pool ⇒ ceiling 1.124x) and measures the entropy-pull component (0.3 % whole-codec), but it **does not isolate dispatch**, so `G1`'s own denominator is unreported and `G1` is **unresolved, not failed**. | **Correction, not a win.** `P1c` is **still required** and remains valuable to tracks 06/18; E1 supplies only the ceiling and the noise band. `P1c` is *not* in my retained scope (§10.2) because it cannot change FLI's verdict: §4.3f shows the ceiling is frontier-irrelevant even if `G1` passes. **I withdraw my earlier claim that `G1` "fails now".** |
R2 | Decode saving `≈8 ns/iteration`, `≤~5 %` whole-codec (`:294-311`) | Direction correct; **magnitude not ratifiable** — the 5.5 % figure requires `c_dispatch + c_state` to dominate a pool E1 measures at 0.3 % for the pull component | **E1 supersedes.** Not a contradiction of sign, a refusal to ratify a number built on an unreported term. |
R3 | `KITER_MAX` implied to be a free parameter for ratio; run-length gates `G2`/`G3` at k≥4 and median ≥8 | `KITER_MAX ≤ 15` is forced by the amplification ceiling (§5.1) — **but this does not kill `G2`/`G3`** (§5.2); it kills the tail, the freedom to tune it, and makes the `ρ` meter row a precondition | **I split the difference against both lanes.** Track 19's "largely dissolve" is arithmetically wrong (§5.2). Track 11's silence on the cap is a doctrine-5 breach. The cap is real, binding, and **not** fatal to the gates — so it is not, by itself, a kill argument. My kill rests on R1/R4/R5. |
R4 | Byte ceiling 0 %…−7 %; primary-cell projection ≈−2.0 % at `H_op=0.5` (`:281-289`) | **Agreed, and completed**: scaled to the frozen Windows cell the band is −3.5 KB…−16.2 KB against a 50,881 B gap; after the §4.1 fixed tax, ≤30 % of the gap closes (§4.4) | **I prevail by extension.** The constructive report does not connect its own projection to the recorded frontier gap; that connection is the kill. |
R5 | Novelty claim rests on S1–S4 with `G0` = "any of S1/S2 anticipated ⇒ NO-GO" (`:356-385`, `:491`) | S3 is **anticipated** (ISLP, Track 20 §9.2) and must not be carried; S1/S2 clearance is a **gap**; and the S2 bullet's own cited evidence (Exp. Y) is an adopt-class engineering result | **I prevail.** `G0` cannot be recorded as passed on the current record. Under the constructive report's own G0 wording, the gate is **unpassed**, not failed — and a gate that cannot be passed is a reason not to fund, not a reason to wait. |
R6 | `P1d`/`H5`: `λ·C_decode` is unreconciled between `FORMAT.md` units and the Exp-AA ns/B table; resolving it is "the highest-leverage finding in this report" (`:568-572`) | **Confirmed and already answered by measurement**: E4 shows at λ=0.01 the decode term spans 0.02–0.66 B, the Linux implied WTP was 460.2 B/μs, and **zero raw-flips were arithmetically certain**. So `J` is currently pure-length and any loop-vs-hot-op gating built on it is optimizing the wrong quantity | **I prevail with a correction**: this is not an open question awaiting an afternoon; the *direction* is settled (J cannot gate). `G10` ("J-gating never costs material bytes") is therefore **unrunnable as a scientific test** — it can only confirm that a mis-scaled gate is a length gate. Track 11 should have reported E4 as an existing measurement rather than as a hypothesis. |
R7 | Recommendation **PILOT** (build/measure P1+P2 remotely, do not build the mechanism) (`:545-565`) | **KILL** the mechanism; **narrowed HOLD** on the six-number byte-only job of §6.3 | **I prevail.** P1c is unnecessary (R1); P2 uses an invalid instrument (R5/§4.5); the residual value is one conditional-entropy number plus one production-trace histogram, which is a fraction of P1+P2 and answers the same questions. |
R8 | Self-assessment: "(a) G1 fails … (b) G2/G3 fail" as most likely (`:580-583`) | I agree with the **ordering** but not the framing: (a) `G1` is **not decided** in either direction (§4.3c) — what *is* decided is that the ceiling is frontier-irrelevant (§4.3f); (b) `G2`/`G3` are **unmeasurable as specified** (§4.5) rather than failing | **Partial agreement.** A useful diagnostic; not a preregistration, and item (a) should not have been asserted as a decision. |

### 8.1 Coordinator-raised blockers — explicit resolutions

**BLOCKER 1 — "Does `KITER_MAX <= 15`, forced by preserving the incumbent amplification ceiling, kill FLI's preregistered run-length and byte gates?"**

**ANSWER: NO. It kills neither `G2` nor `G3`, nor the byte gate. `DERIVED`:**

- `G3` = "median admissible run length >= 8". Cap 15 >= 8 ⇒ numerically unaffected.
- `G2` = ">= 40 % of match bytes inside admissible runs (k >= 4)". Cap 15 >= 4 ⇒ the gate's run-length floor is unaffected.
- `G4`'s byte neutrality is `k >= 1 + 2/H_op` (constructive §5.1) = `k >= 3` at `H_op = 1.5 B`, `k >= 5` at `0.5 B`, `k >= 8` at `0.3 B`. The **entire** `H_op` hypothesis band sits at `k` in `[3,8]` — entirely below 15.
- Truncation tax: a run of length `k` under cap 15 splits into `ceil(k/15)` loops, each seam costing ~1 opcode + 1 `n` varint + (for `LOOP_ARITH`) one `(dL,dD)` pair ~ `H_op + 3 B`. At `k = 40`, `H_op = 0.5`: 3 seams ~ 10.5 B = 0.26 B/iteration against a 0.5 B/iteration saving ⇒ **the cap costs ~50 % of the marginal win at `k = 40` and 0 % at `k` in `[8,15]`**, which is the operative region per §4.4.
- What the cap *does* destroy is not a gate but a **freedom**: `KITER_MAX` becomes a security constant fixed by the amplification ceiling (§5.1), so it can never be a ratio knob; the tail `k > 15` is unexploitable (which further erodes an already-insufficient projection); and Track 19's `rho` resource-meter row becomes a **wire-landing precondition** rather than follow-up work.

**I therefore explicitly reject Track 19 §10.3's closing assertion** that the cap "would largely dissolve the mechanism, since the documented rationale is runs 'just above the byte-neutrality threshold'". That is rhetoric; the byte-neutrality threshold is 3-8 and the cap is 15. **I also decline to use the cap as a kill argument for my own verdict** — my kill rests on §4.3(f), §4.4, §4.6 and §4.5. Borrowing a peer's rhetorical escalation as my kill would be exactly the laundering FM-9 names.

**BLOCKER 2 — "Audit the dynamic opcode allocation when `K` approaches 254."**

**ANSWER: there is a hard byte-representability cliff at `K = 251`, and both available repairs attack the very runs `G3` measures.** `SPEC-AUDIT` + `DERIVED`, against `src/anvil.cpp:2797` (`kHotMaxOps = 254`), `:2994` (`K = get_uvar(p,e)` — **per block, not a constant**), `:2915` (`for (uint8_t op : s[0])` — the opcode stream is a **byte** stream), `:2917/:2929/:2977` (dispatcher uses exactly `0..K`, else throw):

- **No collision today** — credit where due, and Track 19 says the same: `K+1 … K+3` are genuinely unused slots.
- **Cliff:** max representable code is 255, so `K+4 -> class+1` requires `K + 4 <= 255`, i.e. **`K <= 251`**. For `K` in `[252,254]` the specified opcodes **do not exist**.
- **Repair (a)** — reject any block declaring loop opcodes when `K > 251`. Consequence: **FLI is silently unavailable in exactly the blocks with the largest books**, i.e. the blocks with the most hot classes and plausibly the longest admissible runs.
- **Repair (b)** — lower `kHotMaxOps` to 251. Consequence: **reduces incumbent book capacity**, i.e. changes the *baseline*, requiring its own byte-identity argument; and it pressures the encoder toward **block splitting** to regain class headroom, which resets `last[]`, which shortens runs (constructive §11 A11), which attacks `G3` again.
- **Ledger inconsistency:** "`K <= 254`", "adding <= 4 loop codes", and "bounded, charged in §5.1" **cannot all be true simultaneously**. `K` is read per block, so the alphabet cost is bounded only if `K` is itself capped below 251 — which is one of the two changes above. Neither repair's byte cost appears in the constructive ledger; I price the fixed per-block tax separately at **0.4-1.0 % of the primary cell** (§4.1).

**BLOCKER 3 — "Is `S2` — fused execution of a counted repetition over compressed token semantics — genuinely distinct from existing grammar/LZ/superinstruction decoders, or merely an implementation detail?"**

**ANSWER: two different questions, and the coordinator's framing requires separating them.**

- **(i) Is it *exactly anticipated as a wire instruction*?** **No — and this is a GAP, not a clearance.** Track 20 §9.3/§16.1 records no retrieved shipped codec with FLI's exact semantics; its queries returned LLVM `LoopFuse` (irrelevant, self-recorded). Its own `G9` demands >= 2 independent full-text sources for S1 **and** S2 before G0 may be recorded; patents, proprietary console/embedded packers and the 18-month blackout remain unsearched (`docs/priorart-tcopy-external.md:170-176`). Per `docs/gate-priorart-audit-i8.md:203-218`, that is a **gap, not a negative**.
- **(ii) Does that leave defensible *primitive* novelty?** **No.** Three grounds, none of which depend on literature I failed to retrieve:
  1. **The decoded function is invariant.** The same instruction stream yields the same bytes fused or materialized; only memory traffic differs. The repo's own standard is explicit: Exp. Y's loss is classified "**implementation-era** … **not math**" (`RESEARCH_LEDGER.md:3778-3781`). A property that changes only the schedule is not a mathematical contribution.
  2. **The constructive report's own `S2` evidence is adopt-class evidence.** Its `S2` bullet cites *Exp. Y* — this repo's measured materialization negative — as proof fusion matters. That demonstrates a real **engineering lever**, which is what adopt-class art is. I record this as a reconciliation, not a gotcha.
  3. **The residue is not novel either.** Strip fusion and what remains is "per-iteration operands inherit from prior decoder state at zero bits" — LZMA rep0-rep3, zstd repeat-offsets, Brotli's distance cache, and per Track 20's own retrieval **LZO's documented cross-instruction `state`**.

**Both branches terminate in non-promotion.** If `S2` *is* anticipated (Track 20's open search), FLI is anticipated. If `S2` is *not* anticipated, FLI is **adopt-class** under (i)+(ii) above. That asymmetry is why the verdict does not wait on the literature search — it can only accelerate it.

**BLOCKER 4 — "Distinguish orbit/program-synthesis as a broad research family from this FLI instantiation."** See §3 for the family audit: **six of seven family directions are KILL** on prior art or on this repo's own measurements (grammar VM decode-negative per E5; transmitted-parameter ISA prior art + measured-worse per E11/E12; estimator-derived parameters MATH-class closed per E13; mask/topology coding measured −13.5…−21 % per E8; Orbit *search* conceded established by `FRONTIER-RESET-2026-09-23.md:701-718` and owned by TCOPY/PNRA/H2). **FLI is the family survivor** — the only direction with an unoccupied slot — and it is the one I am killing. **The kill is therefore of the instantiation, and it leaves the family verdict unchanged: the family is not rescued by FLI, and FLI is not rescued by the family.** One genuinely open family item survives and belongs to **track 12**, not track 11: **encoder-only search over reversible representations** (§3's residual Kolmogorov costs are on the *encoder*, and `docs/ORBIT_PROGRAM_COMPRESSION.md` rank-7 puts the AI/learned prior on the encoder side with a stupid decoder — the opposite risk profile from FLI).

### 8.2 Disagreements that could still change my verdict (re-opened only for these)

1. **A new full-text retrieval demonstrating fused, non-materializing execution of a counted repetition in a *shipped* codec.** This would collapse `S2` to anticipated and make the verdict **KILL with no measurement at all** — i.e. §6.3 becomes unnecessary. Track 20 §9.3/§16.1 names this exact search and has not run it. **Single highest-value open item in the track.** Note the asymmetry: it can only *accelerate* the kill, never reverse it (BLOCKER 3).
2. **A measured `H(same|same) > 0.45 B`** at `H_op >= 0.5 B` (gate F2 NO-GO). That would mean opcode repetition is *not* cheap conditionally — restoring FLI's relative byte position versus AOC and making F3 the decisive gate rather than a formality. It would **not** revive the decode leg (§4.3f ceiling is frontier-irrelevant regardless) or a novelty claim (§4.6).
3. **A corrected production-trace coverage measurement showing `>= 40 %` of match bytes in runs `k` in `[4,15]`, with `ΔAOC >= ΔFLI + 2 %`.** That combination points the project at AOC plus an *encoder-side* run-length prior rather than a decoder-side loop — a different mechanism, and the only variant I would fund differently.
4. **A new measured decode profile that isolates dispatch and shows `(entropy+dispatch)/token-cost >= 25 %`**, i.e. `G1` passing. That would reopen the *sizing* of the decode leg — but not its verdict, because §4.3(f) shows the 1.124x ceiling is frontier-irrelevant and §4.4 shows the byte leg fails independently. **No single measurement makes FLI promotable; only a combination that also revives the byte leg would.**
2. **A measured `H(same|same) > 0.45 B`** at `H_op ≥ 0.5 B` (gate F2 NO-GO). That would mean the opcode repetition is *not* cheap conditionally — which would restore FLI's relative byte position versus AOC and make F3 the decisive gate rather than a formality. It would not revive the decode leg (R1) or the novelty claim (R5).
3. **A corrected, production-trace coverage measurement showing `≥40 %` of match bytes in runs `k ∈ [4,15]`** with `ΔAOC ≥ ΔFLI + 2 %`. That combination (strong coverage, AOC clearly ahead) would point the track at AOC + an *encoder-side* run-length prior rather than a decoder-side loop — a different mechanism, and the only variant I would fund differently.

---

## 9. Failure modes of the mechanism (independent additions)

Beyond track 11 §11 and Track 19 §10.2, on the security-economics and accounting axes:

| # | Attack / failure | Assessment | Risk |
|---|---|---|---|
FM-1 | **Cap-bound DoS within the cap.** With `KITER_MAX = 15` and `L_i ≤ 65536`, a 3-byte header buys `15 × 65536 ≈ 0.98 MB` of decode work. Against a 64 KiB block cap this is ~15 blocks' worth of output from one instruction — bounded, but **15× the per-instruction work of any incumbent token**. Requires the E15 `ρ` row or it is a free amplification primitive. | `SPEC-AUDIT`, `DERIVED` | **medium-high** until metered |
FM-2 | **MG-4 byte laundering.** Unconsumed `S_n`/`S_dl`/`S_dd` remainders are accepted and discarded: bytes in the file, in no rate objective, invisible to CRC-over-output. Directly violates doctrine 2 and `FORMAT.md:782`. | `SPEC-AUDIT` (E15/MG-4) | **high as accounting** |
FM-3 | **MG-5 book cliff.** `K ∈ [252,254]` makes `K+4` unrepresentable; repairing by lowering `kHotMaxOps` shrinks *incumbent* capacity and pressures the encoder into block splitting, which resets `last[]` and shortens runs — attacking `G3` directly. | `DERIVED` §5.4 | **medium** |
FM-4 | **Trace materialization.** `O(N_tokens)` side buffer + second pass (§3) is the same shape as Exp. Y's measured materialization loss, and contradicts the report's own "no new data structures" claim. | `HYPOTHESIS` (unmeasured whether the trace already exists) | **medium** |
FM-5 | **Adversarial neutrality.** Crafted input with runs sized at `k = 1 + 2/H_op` just above the byte-neutrality threshold harvests ≈0 bytes and saves ~8 ns/iteration — i.e. **an attacker-tunable decode amplifier against the incumbent's own tokens**. With FLI the attacker pays 2 B and skips `(k−1)·H_op`; against mode-15's already-compiled hot book the win is the *cycle* saving alone. `ρ` metering is the only real mitigation. | `DERIVED` from track 11 §5.1 + E15 | **medium** |
FM-6 | **MG-1 guard-form failure.** The addition-form block-end check fails open exactly when `L0` drifts (i.e. in `LOOP_ARITH`, FLI's generality half) — the guard is bypassable in precisely the mode the report wants to claim novelty for. | `SPEC-AUDIT` (E15/MG-1) | **high** if shipped as specified |
FM-7 | **MG-2 zigzag cap.** The house zigzag applies no cap to `zz` (`src/anvil.cpp:2947`); `zz = UINT64_MAX` makes `(zz+1)` wrap and the delta decode as `0`. Benign for CRC-protected correctness, but FLI would be *relying* on that path for a **length**, where `0` is illegal and where the wrap is not self-announcing. | `SPEC-AUDIT` (E15/MG-2) | **medium** |
FM-8 | **Threshold laundering by pinning.** `KITER_MAX` pinned *after* `P2` reports the run-length histogram is threshold-fitting on a security parameter (doctrine 5). Must be pinned from the amplification ceiling first; §5.1 is the derivation I accept. | procedural | **high if done wrong** |
FM-9 | **Claim laundering.** A byte-only AOC/FLI result reported as "program-synthesis compression beats Brotli". Pre-empted by: dual-bar on synth cells (E10), `{synthetic}`/`{engineering}` labelling, `GRID-THIN` on every citation, `0 FRONT-CROSSING` canonical tuple, and the FRONT-GAP→FRONT-CROSSING bracket test. | procedural | **high if ignored** |
FM-10 | **Track duplication.** §6.3's six-number job overlaps track 18 (decoder architecture cost decomposition) and track 12 (encoder-only search). One job, one owner — do not build two harnesses. | procedural | medium |

---

## 10. Recommendation

### 10.1 Dispositions

| Item | Disposition | Basis |
|---|---|---|
Bounded micro-program **VM** / grammar VM over bytes | **KILL** | Prior art (RePair/SEQUIT/XMill/ISLP) **and** measured decode-negative here (E5). No novelty review rescues it. |
Transmitted-parameter program ISA | **KILL** | E11, E12: prior art; transmitted measurably beat zero-bit in the one measured cell. |
Estimator-derived implicit parameters (any width/residual bound) | **KILL** | E13: MATH-class closed; engineering-only variants measured unnecessary. |
Mask / correction-topology cloning | **KILL** | E8: masks ~90 % unique; mode 13 lost 13.5–21 %. |
Grammar/Orbit **search** as the novelty | **KILL** | `FRONTIER-RESET-2026-09-23.md:701-718` concedes the IR is established; search owned by TCOPY/PNRA/H2. |
`LOOP_ARITH` as a **generality** claim | **KILL** | E16: ISLP anticipates it at the abstract level (Track 20 §9.2). |
**FLI as a decode-leg mechanism** | **KILL** | §4.3: hard ceiling **1.124x** (all FLI-removed work is inside the measured 11 % pool); projected ≈5 % effect sits **below** the project's own noise band (zero-byte-change controls +2.3–8.1 %); and the ceiling itself is **frontier-irrelevant** — 273.534 MB/s × 1.124 = ≈307 MB/s vs q9's 619.508. `G1` is **unresolved, not failed** (§4.3c) and is not part of this basis. |
**FLI as a novelty-bearing mechanism** | **KILL** | `S3` anticipated (ISLP); `S4` downgraded; `S2` **not exactly anticipated as a wire instruction** but leaves **no defensible primitive novelty** — it is a fusion/schedule property, classified by the repo's own standard as implementation-era (E5). |
**FLI as ratio-only adopt-class** | **KILL** | §4.4: ≤30 % of the recorded gap after its own fixed tax; F5 shows it is not a crossing instrument on any frozen cell. |
**AOC (order-1 opcode context)** | **HOLD, byte-only, remote, §6.3** | Adopt-class by construction; possibly the whole of FLI's byte win at zero ISA cost. Not a novelty claim. |

### 10.2 Final verdict: **KILL**

**KILL** the Track 11 mechanism (FLI / bounded reversible micro-program ISA over compressed token semantics) as a novelty-bearing or frontier-relevant contribution.

**Narrowed HOLD** on exactly one remote, byte-only, six-number job (§6.3) whose purpose is to close the track with data rather than argument, and to test whether the *smaller* alternative (AOC) captures the whole benefit at zero security surface. That job is: (i) two entropy numbers on the existing opcode stream (`H_op`, `H(same|same)`), (ii) AOC's byte delta, (iii) FLI's byte delta with the fixed tax and the 15-cap applied, (iv) the admissible-run histogram **on the production parse trace**, (v) the `K` distribution. Any F2–F6 NO-GO closes the track as KILL-on-measurement.

**Explicitly declining PILOT** and **PROMOTE-TO-REMOTE**. PILOT would fund a mechanism build whose decode ceiling is 1.124x and frontier-irrelevant (§4.3f), whose projected effect is under the noise band (§4.3d), and whose byte leg cannot reach its own cell's frontier gap (§4.4), on thresholds that are invalid as written (§4.5) and unpinnable without a doctrine-5 breach (§5.3). PROMOTE-TO-REMOTE would additionally require recording a novelty claim that cross-lane review has reduced to a systems/unification claim over an anticipated abstract recurrence.

**Threshold freeze.** The `F1`–`F6` gates of §6.3, the constructive `G1`–`G10`, and `KITER_MAX = 15` / `LEN_MAX = 65536` are **frozen as of this document**. No gate is moved, re-weighted or reinterpreted after any measurement. `KITER_MAX` is pinned from the amplification-ceiling derivation of §5.1 **before any run-length data is seen**, which is what makes the pinning legitimate under doctrine 5. `F1` is recorded as **NO-GO on the hard ceiling**, decided before measurement from `MEASURED` E1, and is therefore not re-openable by data — only by a new cited profile that contradicts E1's methodology, which would be a correction of record, not a threshold move.

**Is any zero-ISA opcode-context (AOC) measurement worth retaining after FLI is killed? — YES, conditionally, and it is the only thing I am retaining.** The reason is not that AOC rescues FLI — §6.2 argues it may *beat* FLI, which is precisely why measuring it is informative. AOC is a **re-modeling of a stream that already exists**, needing: no new opcodes, no new substreams, no new decoder state beyond one cached symbol, no block-header change, **no security sign-off from track 19**, and no change to the amplification class. Its value is that it converts the *same* question FLI asked — "how cheap is opcode repetition?" — into a **citation-grade byte-only measurement with no safety surface**, and its answer is reusable by tracks 05, 06 and 18 without any of FLI's preconditions. **Retained scope: numbers 1, 2 and 6 of §6.3 only** (`H_op`, `H(same|same)`, `K` distribution) — these are pure entropy/book statistics computable from the frozen mode-15 opcode stream with no re-encode and no code change. **Numbers 3, 4 and 5 are dropped** with FLI: AOC's byte delta and FLI's byte delta are only meaningful as a comparison, and the production-trace run histogram exists solely to validate FLI's `G2`/`G3`. If the retained three numbers show `H(same|same) <= 0.30 B` and `blocks with K > 251 <= 1 %`, AOC becomes a candidate for the ratio-only adopt-class lane under its own (new, separate) pre-registration; if they miss, the whole track closes with no residue.

**If the coordinator nonetheless proceeds with FLI**, the preconditions are, in order: (1) pin `KITER_MAX = 15` and `LEN_MAX = 65536` **from §5.1 before any run-length data is seen**; (2) fix MG-1, MG-2, MG-3, MG-4 (substitution-form guards, `int64` drift with post-checks on both sides, per-iteration `1 ≤ L_i ≤ LEN_MAX`, full-consumption extension to the three new streams) and MG-5 (reject loop opcodes when `K > 251`); (3) add the E15 `ρ` resource-meter row as a wire-landing precondition; (4) re-derive `G2`/`G3` against the production parse trace; (5) ship a forced registry-coverage test for every new mode ID (`FORMAT.md:907-910`). Failing (1)–(3) is not a reviewer's preference — it is Track 19's sign-off condition.

### 10.3 Cheapest decisive remote next step (the one thing I am asking to be funded)

**One remote, byte-only, R1-tier job. Deterministic and citation-grade: no timing, no RSS, no fuzz, no codec change, no ISA, no new decoder-visible state, no build of any new component.** Protocol per `docs/GITHUB-ACTIONS-BENCHMARKING.md` §2/§5/§8.1/§9 and the I10 ladder; manual dispatch only; primary cell `generated.log`; paired with Silesia, enwik8, the locked `u3` PE set, and `generated.{json,jsonl,sqlite}`.

**It is the three retained numbers of §6.3, executed as a post-pass over artifacts that already exist.** `H_op`, `H(same|same)` and the `K` distribution are computed from the **already-frozen mode-15 opcode stream** — a conditional-entropy pass plus a book-size histogram. **No re-encode, no new build, one CI job.**

**Why this is the cheapest decisive step.** It is the only open question that (a) is byte-only, hence free of every measurement ambiguity in §4.3; (b) needs no security sign-off, since it touches nothing; (c) directly tests the *smaller* alternative rather than the mechanism; and (d) is reusable by tracks 05/06/18 regardless of Track 11's outcome. It closes Track 11's accounting residue with data rather than argument, and it is the honest successor to the constructive plan's `P1c` (whose dispatch question is now answered *not at all* by the profile, and remains valuable only to tracks 18/06) and `P2` (whose instrument is invalid, §4.5).

**Explicitly not funded:** any FLI mechanism build, the A0–A6 ablation arms, `loop_oracle` in its specified form, any timing measurement (the ceiling in §4.3f makes timing decisions moot for FLI), and any local execution.

### 10.4 What I am *not* claiming

I ran nothing. Every cycle, RSS and MB/s figure in §4 is either `MEASURED` (cited to an in-repo artifact and its protocol) or `DERIVED` (arithmetic shown). I make no claim about FLI's performance on any hardware I did not measure, and none about any corpus I did not run. **I specifically do not claim that `G1` is falsified** — its denominator is `(entropy+dispatch)/per-iteration token cost` and the profile reports only the entropy-pull term, so `G1` is unresolved (§0.2(2c), §4.3c). My prior-art statements inherit the coverage honesty of `20-…-space-bunny.md` §9 and `docs/priorart-tcopy-external.md` §"Access / coverage gaps": **no patent database was queried by me, and "not retrieved" remains "not retrieved"** — so `S2` is recorded as *not exactly anticipated*, never as *cleared*. §8.1 BLOCKER 3 is my *argument* that `S2` leaves no primitive novelty; it is not a claim that `S2` is anticipated, which Track 20 correctly declines to assert. Track 19's §10.3 arithmetic and specification audit are load-bearing external inputs to my §5 and §9; I re-derived the cap rather than adopting it, and **I explicitly reject its escalation of the cap to a mechanism-kill** (§8.1 BLOCKER 1).