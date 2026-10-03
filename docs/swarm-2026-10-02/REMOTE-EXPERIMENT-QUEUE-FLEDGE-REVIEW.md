# ADVERSARIAL REVIEW — `REMOTE-EXPERIMENT-QUEUE.md` (Fledge Alpha Free, perf red team)

**Target:** `docs/swarm-2026-10-02/REMOTE-EXPERIMENT-QUEUE.md` (DRAFT v0, coordinator-owned, 161 lines).
**Reviewer role:** independent adversarial / validator. **No measurement performed.** No local benchmark, no
local fuzzing, no production edit, no commit/push. The coordinator's draft is **not modified** — this is a
separate review artifact for the coordinator to merge or reject.
**Companions:** `18-future-decoder-architecture-fledge.md` (track-18 audit), `SYNTH-PERF-FLEDGE.md`
(cross-lane performance economics).

---

## 0. Ruling on the queue

> **The queue's ranking rule and tiering are sound. Three defects must be fixed before dispatch: (1) Q3's
> A0/A1 arms are mislabelled and would measure a third, unspecified thing; (2) Q3's timing trigger is
> stated in the wrong denominator and will authorise a sub-floor timing run; (3) Q4b's premise uses a
> superseded requirement (4.5×) and is therefore *stricter* than reality but still understates the kill —
> on the measured split, `f_walk` optimisation cannot meet the corrected requirement under any outcome, so
> WSI/lane-scheduler prototyping should be killed now, not gated.**
>
> **A reordering is proposed (§5).** Q3's byte-only Stage 1 moves ahead of Q2; Q7 (DEFLATE control) moves
> above Q4b; Q4b is downgraded to an arithmetic record.

**Three concrete defects, each checkable:**

| # | defect | evidence | consequence if not fixed |
|---|---|---|---|
| D1 | **Q3 A0/A1 labels are swapped.** The queue calls A0 "the exact current **constant-c** objective" and A1 "the exact existing **size-proportional** budget objective". In-repo truth is the **reverse**: `src/anvil.cpp:1635` ships `C_us = src.size() * kBudgetNsPerByte[codec]/1000.0` — **size-proportional**; `FORMAT.md:648` documents the **integer constant-c** table `{10,22,20,30,35,40,45}`. | `src/anvil.cpp:1618-1636` vs `FORMAT.md:631-663` | A0 is not reproducible from *either* document. The job would compare "binary behaviour" against "a spec the binary does not implement" and report the gap as a finding about the objective when it is a finding about **stale documentation**. Fix: rename arms `A_binary` / `A_FORMATmd` and add a third **definitional** arm `A_both` that pins both semantics, byte-only. |
| D2 | **Q3's timing trigger is in the wrong denominator.** "Timing only if Stage-1 choices differ materially and expected effect is above the known sensitivity floor." Stage 2 (side-stream materialization) is **26 %** of end-to-end decode on `generated.log` `[MEASURED]`. A **10 % improvement inside stage 2 is 2.6 % of end-to-end** — **below the ~7 % paired floor**. | `RESEARCH_LEDGER.md:3924-3930`; floors from `:3918-3920` | The trigger as written admits sub-floor timing runs. Fix: require the **implied whole-codec effect** `ΔJ_ns / e2e_ns ≥ 0.07` computed *before* dispatch, and require the Stage-1 disagreement count to be **> 0** as a hard gate. |
| D3 | **Q4b's requirement figure is superseded, and the Amdahl test is already decided.** The queue uses "about 4.5x". The corrected, pinned requirement is **6.05× dickens / 3.86× webster / 6.60× enwik8**; and the measured stage split gives `f_walk ≈ 50–61 %`, so an **infinite-speed walk caps at 1.64×–2.0×**. | `docs/anvil-i9-findings.md` §6.2; `prototypes/i9-decode-perf/REPORT.md` §11 v2 | The gate *looks* conditional, so a lane will be funded. On the measured numbers WSI/lane-scheduler prototyping is dead **under every outcome** of Q4b. See §3. |

---

## 1. Per-job audit: share attacked · Amdahl ceiling · minimum resolvable effect

### TIER 0

| job | timed? | share attacked | Amdahl ceiling | min resolvable | verdict |
|---|---|---|---|---|---|
| **Q0** dense-frontier correction | no (static) | none (classification axis) | n/a | n/a | **Keep first.** Add one check: a retained-artifact crossing that turns on the **decode** axis is **not reachable by any wire-invisible decoder change** — the realistic ceiling on the best-profiled cell is **1.351×** (`SYNTH-PERF-FLEDGE.md` §1.2). Q0 must not authorise a decoder lane on a decode-conditioned crossing. |
| **Q0b** safe-search theorem cleanup | no | none | n/a | n/a | **Keep.** No perf content. |

### TIER 1

| job | timed? | share attacked | Amdahl ceiling | min resolvable | verdict |
|---|---|---|---|---|---|
| **Q1** CORPUS-ADMISSIBILITY | no (0 codec invocations) | none | n/a | n/a | **Keep second.** Free add: have Q1 publish **block count / mean block size per canonical file** (`SYNTH-PERF-FLEDGE.md` §7 item 4) — it is a corpus-geometry census, it costs nothing here, and it is the denominator every per-block budget (Track 15 CAM's 64–256 B/block, Q2's geometry arms) needs. It is currently missing project-wide. |
| **Q2** block-window parity pilot | **yes** | stage 4 `concat/alloc/headers` = **15 %**; stage 2 = **26 %** | stage-4-only **1.176×**; stage 4+2 **1.351×** (and stage 2 is proven non-removable) | **≥ 7 % paired** | **Re-scope: RSS-primary, and the predicted decode sign is *negative*.** More blocks ⇒ more per-block concat/alloc ⇒ stage 4 grows ⇒ decode **slows**. The measured profile predicts this; Q2 should be funded to confirm **RSS relief** (measured: 16 MiB cap → 125.7 MiB at **+1.0169 %** bytes; 8 MiB → 124.4 MiB at **+1.9346 %**), not to find speed. Add the mandatory output **`stage-4 ns per block`** — without it a whole-codec-only readout cannot distinguish per-block fixed cost from size-proportional cost, and the delta is therefore unpredictable a priori. Timing arms admissible **only** with a declared ≥ 7 % predicted effect. |
| **Q3** selector arbiter | conditionally | stage 2 = **26 %** | **≤ 1.351×** absolute; **≈ 1.29×** if every stream re-selects to its cheapest measured codec (26 % → ≈ 3.3 % using `defexc` mat 0.48–0.56 ns/B vs `rans4096` mat 4.35–4.73 ns/B) | **≥ 7 % of e2e, not of stage 2** (D2) | **Keep Stage 1; fix D1 + D2; expect it to CANCEL the timing arm.** The measured baseline is already in the record: the S6-1 selector manifest logs **792 selections (88 blocks × 9 streams, 23 files) with ZERO raw-flips**, all **139** flips being **exact-L ties** (`logical_eq=1 ∧ dec_eq=1`). The incumbent objective is a **tie-breaker, not a selector**. Additionally: **"No third lambda-fit arm in Stage 1" is correct and I withdraw my earlier λ\* proposal to Stage 2** — Stage 1 should compare the two *existing* objective forms, not fit a third. |

### TIER 2

| job | timed? | share attacked | Amdahl ceiling | min resolvable | verdict |
|---|---|---|---|---|---|
| **Q4a** typed basis × BWT byte cross-product | no | none (bytes) | n/a | n/a | **Keep, but scope it to four files.** The measured denominator is `06-do-not-reburn.md` F1/F2: the 664,307-B landscape gap is **mozilla 429,893 (64.7 %)**, sao 160,422 (24.2 %), ooffice 51,625 (7.8 %), samba 22,367 (3.4 %), **all other files 0**. A mozilla-targeted mechanism that cannot plausibly recover **> ~430 KB** is not worth building. Do not spend cross-product budget on the other eight. |
| **Q4b** BWT stage decomposition / `f_walk` | **yes (stage timing)** | LF walk `f_walk` = **50.2 %** dickens / **61.0 %** webster | **infinite-speed walk = 1/(1−0.502) = 2.01× (dickens), 1/(1−0.610) = 2.57× (webster)** | stage **shares** are ratios measured in one paired job ⇒ resolvable well below 7 %; absolute stage **ns** need the zero-byte-change control ≤ 5 % | **Run as a 10-minute arithmetic record; KILL WSI/lane-scheduler now.** See §3 for the full derivation. |
| **Q5** MASK-CEILING | no (byte-only) | masks are a **byte** share, not a decode share | n/a | n/a | **Add a pre-gate that can cancel it.** Measured: correction masks are **~90 % unique** (top-32 covers **7–12 %**); `(k,slot)` modal accuracy is **~20 %**, not the 86.5 % originally assumed; mode-13 topology coding **lost +13.5 %..+21.0 %** on every record file. So the *ideal-replacement ceiling* is bounded by the ~**7–12 %** that a 32-symbol code actually captures. **If masks are < 2 % of the container, Q5 is byte-irrelevant — cancel rather than run.** Report the mask share first, in the same job. |
| **Q6** G5D census-first ablation | no | n/a | n/a | n/a | **Keep.** The arm that matters is **paging**, because paging is the mechanism that could *avoid* the E4 warmup tax. Report "warmup tax avoided" against the measured E4 denominator: `webster` sum-of-4-MiB-blocks **+894,315 B (+12.22 %)**; block oracle **+5.42 MB … +643 KB** across scales; *"finer granularity is monotonically worse."* |
| **Q7** DEFLATE reconstruction control | no (byte-only) | n/a | n/a | n/a | **Keep and PROMOTE** — and **add a mandatory size arm.** Measured EV: **1,362,177 B** recovered on `mozilla` (bar: > ~430 KB, so 3.17× the bar), beating mozilla xz -9e by **932,252 B**. It requires **no decode work at all**, so it cannot be invalidated by any decoder result. **But:** I10-1B already says "decoder code-size cost must be charged explicitly" and the queue omits it. Per the project's only measured exchange rate (aux-unbwt: **+2,976 B `.text` ≈ 1.36–2.34×**), a pinned zlib inflate is **plausibly far above 8 KiB of `.text`**. Q7 must therefore also report **`.text` growth and stripped-ELF size against the AITDCC 1 MiB decompressor cap**, or the win may be inadmissible on a size axis the project has already declared binding. Note the recorded caveat: the 1,362,177 B figure is **encode-only, measured on a build whose decode path was broken at measurement time** — the claim is **PENDING** native integration + roundtrip + fuzz. |

### TIER 3

| job | timed? | share attacked | Amdahl ceiling | min resolvable | verdict |
|---|---|---|---|---|---|
| **Q8** declared-total / amplification probe | no | n/a | n/a | n/a | **Keep.** Security infrastructure, byte-only. No objection. |
| **Q9** aux W1024 vs W16 | no (byte/correctness only, explicitly) | n/a | n/a | n/a | **Keep byte-only — and make that binding.** The measured aux prize is **+2,474…+4,098 B/file** (Silesia portfolio total **+19,344 B**; Silesia+enwik8 **+22,398 B** = **+0.00718 %** of source). Absolute prize small, correctly stated. **The speed risk direction is unknown and must not be probed by accident:** W1024 → W16 means *fewer* checkpoints, i.e. *more* re-scan work per walk, so W16 may **cost** speed while saving bytes. Because the 1.364–2.339× decode win from aux is **already banked**, a W16 timing surprise would trade a landed win for ~3 KB. **Also require peak aux-index bytes** to be reported — this is the same missing measurement as `SYNTH-PERF-FLEDGE.md` §7 item 5 (peak model-table resident bytes, currently conflated with output buffer and BWT working set in every RSS number on record). |

### BLOCKED / CONDITIONAL

| item | verdict |
|---|---|
| **B1** numeric dual-bar | **Agree: HOLD.** Blocked on a real locked numeric family. No perf objection. |
| **B2** planner-routing preflight | **Agree: HOLD.** Do not fund two overlapping pilots. No perf objection. |
| **B3** Track-12 POOL-1 | **Agree: HOLD** after H0 correction. |
| **B4** CAM model-prior engineering | **Agree HOLD — but the stated reopen condition is wrong and should be corrected.** See §4. |

---

## 2. Explicit checks requested: Q3 must not time below sensitivity; Q4 must not prototype before `f_walk`

**Q3 — CONFIRMED and tightened.** The queue's Stage-1 rule is directionally right and I add the missing
denominator:

```
admissible(Q3 timing) :=
      (Stage-1 disagreement count > 0)                       # hard gate
  AND (Σ over disagreeing streams of |Δ decode ns| )
        / (e2e decode ns)  >= 0.07                          # whole-codec, NOT stage-2
  AND (A/A null CI spans 1.0)
  AND (zero-byte-change control arm <= 5%)
```

With stage 2 = 26 % of e2e, note the trap numerically: **a 20 % improvement inside stage 2 is 5.2 % of
e2e and must be refused**, even though it sounds large. The only way to clear 7 % from stage 2 alone is to
re-select essentially *all* side streams from `rans4096` (mat 4.35–4.73 ns/B) to `defexc`/`raw`
(mat 0.48–0.56 / 0.02–0.09 ns/B) — an **8–10×** change that the **792-selection manifest says has never
happened once** (0 raw-flips). So the honest prior is that **Q3 Stage 1 returns zero disagreements and the
timing arm is cancelled.** That is a good outcome and a cheap one.

**Q4 — CONFIRMED, and I go further: `f_walk` should not merely precede prototyping, it should *replace*
the prototyping lane.** Derivation in §3.

---

## 3. Q4b — why the LF walk cannot meet the requirement under any outcome

**Measured inputs** `[MEASURED]`, `prototypes/i9-decode-perf/REPORT.md` §11 v2, re-validated on pinned
sha `62BC6631`, quiet window (load 24 % median / 63 % max), containers re-encoded and byte-identical
(2,571,873 / 7,317,361 B):

| cell | postcoder | `libsais_unbwt` | alloc | residual | crc |
|---|---:|---:|---:|---:|---:|
| dickens | 43.9 % | **50.2 %** | 0.6 % | 5.0 % | 0.2 % |
| webster | 34.2 % | **61.0 %** | 0.7 % | 3.8 % | 0.2 % |

Author's own caveat: **variant-overhead ±5–10 % absolute**; the qualitative split (inverse transform is
the largest single stage, postcoder second) is stable across v1/v2.

**Requirement** `[MEASURED]`, `docs/anvil-i9-findings.md` §6.2 — the **corrected** postcoder requirement
after the real aux factor `k_p` is applied: **6.05× dickens / 3.86× webster / 6.60× enwik8**. The queue's
"about 4.5x" is the **superseded** figure.

**Amdahl** `[DERIVED]`:

| case | `f_walk` | non-walk | ceiling at infinite-speed walk | requirement | verdict |
|---|---:|---:|---:|---:|---|
| dickens (measured, favourable) | 0.502 | 0.498 | **2.01×** | **6.05×** | **fails by 3.0×** |
| webster (measured, favourable) | 0.610 | 0.390 | **2.57×** | **3.86×** | **fails by 1.5×** |
| queue's admissibility test | ≥ 0.78 | ≤ 0.22 | — | — | **measured `f_walk` is 50–61 %, i.e. ~1.7× short of the queue's own precondition** |

> **On the measured split, a perfect LF walk delivers 2.01–2.57× against a 3.86–6.05× requirement. No
> implementation quality closes that. WSI/lane-scheduler prototyping should be KILLED now, not gated on
> Q4b.**

**And the duplicate-work argument, which is stronger.** `f_walk` optimisation **has already been built,
integrated, measured and landed**: `libsais_unbwt_aux` is precisely a bi-gram LF-mapping walk acceleration
with auxiliary indexes. It delivered **1.364× / 1.795× / 2.339×** whole-codec decode for **+22,398 B**
(I10-1A, adopted). A second walk-scheduler effort is therefore **re-burning a closed, adopted, measured
result** — which `docs/audit-2026-09-07/06-do-not-reburn.md` E5 and the master-brief doctrine forbid.

**What Q4b is still worth** — a **byte-only + ratio** record, ~one job: confirm the stage split on the
frozen I10-1A candidate and publish the residual-requirement arithmetic above so the kill is on the
record with numbers. **Do not** fund an implementation prototype under any outcome.

**Where Q4b's effort should go instead:**
- **RSS**, not decode: the measured binding axis on the BWT route is **598.9 MiB enwik8 = 9.043× xz**,
  **248.4 MiB Silesia = 4.572× xz**, with relief priced at ~1 % bytes per 2× RSS on Silesia and
  *unpurchasable* on enwik8 (8/16/32 MiB route away → **+5.4075 %**).
- **Bytes, with no decode work at all**: Q7 DEFLATE replay (1,362,177 B on `mozilla`).

---

## 4. B4 (CAM) — the reopen condition is miswritten; my independent audit result

The queue's B4 says: *"Revisit only after Q2 proves residual block warmup under fair geometry and free
larger-block/model-carryover nulls are exhausted."*

**That condition cannot clear B4, because the binding defect is not geometry — it is the state
dimension**, and no block-geometry measurement can fix it.

**My audit result, three independent grounds:**

1. **The state dimension does not fit the stated budget.** 64–256 B = 512–2,048 bits. An order-2 binary
   context model over 2¹⁶ bins needs ≈ **32 KB** at 4 bits/bin; 256 B holds roughly 256 8-bit quantized
   weights. Two measured in-repo results bracket both sides: unconditional order-1 literals were
   **rejected at +3.46 % bytes** (cold-start sparsity), and the later refinement established that the
   rejection was about **physical model multiplicity** (hundreds of cold adaptive models), *not* the
   information signal. The gain needs many states; 256 B cannot hold many states; too few was the measured
   +3.46 % failure. **Budget and mechanism are mutually exclusive.**
2. **"Preserving independent blocks" is the condition already measured as harmful.** Block-local reset at
   64–256 KiB makes `.text` **worse**; E4 closed block-local routing by negative at every scale
   (`webster` +12.22 % at 4 MiB; finer granularity monotonically worse). A per-block model budget buys
   independence by paying bytes on the axis where blocks are already cheapest, while the binding deficits
   are decode (3.9–5.7×) and RSS (4.6–9.0×).
3. **Axis mismatch with the wrong sign.** CAM/PPM is a **rate** mechanism; rate is the axis ANVIL already
   wins (**−2,009,105 B vs xz** on Silesia). A sequential mixer inside the decoder *adds* serial dependent
   work to the token loop — which the measured profile puts at **11 %**, with **opcode entropy pulls at
   0.3 %**. It makes the binding profile worse.

**Recommended rewrite of B4's reopen condition:**

> Engineering revisit requires **all** of: (a) a byte-only demonstration that the intended state
> configuration **fits the 64–256 B budget** (an explicit bin-count vs budget table, not an assertion);
> (b) that configuration beating the measured **+3.46 %** order-1 rejection; (c) a profile showing the
> target stream class is **not** dominated by the side-stream materialization share; and (d) survival of
> the CTW/PPM diagnostic-control designation (`FRONTIER-RESET` §4.8, `I10-BREAKTHROUGH-PROGRAM` §3 O1:
> *"No production claim follows automatically"*). **Q2's geometry result alone is not a sufficient
> precondition.**

---

## 5. Proposed reordering (evidence-supported)

Current order: `Q0 → Q1 → [if Q0 alive] Q2 → Q3 → Q4a → Q4b → Q5/Q6/Q7 → Q8/Q9 → B1–B4`.

| proposed | job | why it moves | evidence basis |
|---|---|---|---|
| 1 | **Q0** unchanged | static; can cancel the live crossing hypothesis at zero cost | queue rule 6 |
| 2 | **Q1** unchanged, **+ free add** (block count / mean block size per canonical file) | corpus-geometry census with no codec cost; denominator for every per-block budget | `SYNTH-PERF-FLEDGE.md` §7 item 4 — missing project-wide |
| 3 | **Q3 Stage 1 only** *(pulled forward from position 3 → before Q2, and narrowed to byte-only)* | cheapest job that can kill a whole family; needs no timing; its result may rebase Q2's arms because it establishes whether selection moves at all | 792-selection manifest: **0 raw-flips**, 139 exact-L ties → strong prior that it cancels the timing arm and the λ-gating family |
| 4 | **Q2**, **re-scoped RSS-primary** with mandatory `stage-4 ns/block` output and a declared ≥ 7 % predicted effect before any timing arm | its predicted decode sign is *negative* (more blocks ⇒ more stage-4 work); the resolvable axis is RSS (1.0169 % bytes per ~2× RSS, measured) | stage 4 = **15 %**, ceiling 1.176×; I10-1A.2 RSS/byte pairs |
| 5 | **Q4a**, **scoped to `mozilla`/`sao`/`ooffice`/`samba` only** | 64.7 %/24.2 %/7.8 %/3.4 % of the gap; other files are 0 | `06-do-not-reburn.md` F1/F2 |
| 6 | **Q7 DEFLATE control**, **promoted above Q4b**, **+ mandatory `.text`/1 MiB size arm** | largest measured byte EV in the whole queue (1,362,177 B vs a 430 KB bar); requires **no decode work**, so no decoder result can invalidate it | `anvil-i9-findings.md` §7.2; I10-1B |
| 7 | **Q5**, **with a mask-share pre-gate (< 2 % of container ⇒ cancel)** | masks ~90 % unique, top-32 covers 7–12 %, mode-13 lost 13.5–21 % → the ceiling is bounded and possibly trivial | `RESEARCH_LEDGER.md` Exp. I |
| 8 | **Q6** unchanged; report "warmup tax avoided" against the measured E4 denominator | paging is the arm that can avoid the tax | E4 measured deltas |
| 9 | **Q4b downgraded** to a byte-only + ratio **arithmetic record**; **WSI/lane-scheduler KILLED now** | measured `f_walk` 50–61 % ⇒ perfect-walk ceiling 2.01–2.57× vs corrected requirement 3.86–6.05×; and `f_walk` optimisation is **already landed** as I10-1A | `i9-decode-perf/REPORT.md` §11 v2; `anvil-i9-findings.md` §6.2; I10-1A |
| 10 | **Q8 / Q9** unchanged, spare capacity; Q9 must report peak aux-index bytes | security + adopted-engineering; both byte-only | queue Tier 3 |
| 11 | **B1–B3** unchanged HOLD; **B4** reopen condition **rewritten** per §4 | current condition cannot clear the real blocker | §4 |

**Why Q3-Stage-1 moves ahead of Q2 specifically.** Three reasons, all evidence-based: (a) it is strictly
cheaper — a selector census needs no timing and no paired decode; (b) its result is *upstream* of Q2's arm
design, because if selection never moves then Q2's question reduces to pure geometry and the stage-2 share
is a constant rather than a variable; (c) running it first means Q2's timing gate can be evaluated against
a *known* stage-2 attribution instead of the pre-PCLMUL split, which the profile itself flags as stale.

**What does not move and why.** Q0 and Q1 stay first (they can cancel everything downstream at zero codec
cost). Q8/Q9 stay in spare capacity (both byte-only, neither upstream of anything). B1–B3 stay blocked
(no evidence in this review touches them).

**Queue invariant, endorsed:** a downstream job whose premise is invalidated upstream is *cancelled, not
run for completeness*. On that rule, **Q4b's implementation half is cancelled by the corrected requirement
figure alone**, before Q4b is ever dispatched.

---

## 6. Summary of required edits to the queue (for the coordinator to apply)

1. **§Q3** — swap the A0/A1 descriptions to match `src/anvil.cpp:1618-1636` and `FORMAT.md:631-663`;
   rename to `A_binary` / `A_FORMATmd`; add `A_both` as a definitional byte-only arm. Record the
   FORMAT.md/source drift as a `format`-lane defect.
2. **§Q3** — replace "expected effect is above the known sensitivity floor" with the whole-codec formula
   in §2 above, including the `> 0` disagreement-count gate.
3. **§Q4b** — replace "about 4.5x" with the corrected **6.05× / 3.86× / 6.60×**; record the measured
   `f_walk` = **50.2 % / 61.0 %**; and change the gate from "prototype only if measured non-walk ≤ ~22 %"
   to **"WSI/lane-scheduler KILLED"**, because 39–50 % non-walk caps any walk optimisation at
   **2.01–2.57×** and the walk optimisation is already landed as I10-1A.
4. **§Q7** — add a `.text` growth / stripped-ELF / 1 MiB decompressor-cap arm.
5. **§Q5** — add the mask-share pre-gate.
6. **§Q2** — re-scope to RSS-primary; add mandatory `stage-4 ns/block`; require a declared ≥ 7 % predicted
   effect before any timing arm.
7. **§Q4a** — scope to the four F1 files.
8. **§Q1** — add the block-count/mean-block-size census as a free output.
9. **§B4** — rewrite the reopen condition per §4.
10. **Dispatch order** — apply the §5 reordering.

---

*Prepared by Fledge Alpha Free (independent adversarial reviewer, perf red team). The coordinator's
`REMOTE-EXPERIMENT-QUEUE.md` was read but **not modified** — lane/file ownership is the coordinator's. Every
number re-derived from in-repo artifacts with locations named; no measurement performed by this agent.*
---

# DATED CORRECTION — 2026-10-02 (Fledge Alpha Free)

Coordinator challenged three items. **All three challenges are upheld.** Two retractions, one dimensional
correction. Recorded rather than silently edited, per this reviewer''s own provenance rule.

---

## C1 — D1 **RETRACTED**. The queue''s Q3 A0/A1 labels are **CORRECT**. My error.

I verified the **call sites and defaults**, not merely the existence of both functions:

| evidence | line | content |
|---|---|---|
| default selector | `src/anvil.cpp:1548` | `encode_stream(...)` — `J = L + g_stream_lambda*cu + g_stream_mu*2.0 + g_stream_nu*cu` with **constant-c** `add(raw,10.0)`, `add(rANS-4096,40.0)`, `add(rANS-512,35.0)`, `add(rANS-256,30.0)`, `add(huffman,22.0)`, `add(defexc,20.0)`, `add(ctx-rANS,45.0)` |
| size-proportional selector | `src/anvil.cpp:1628` | `encode_stream_budget(...)` — `C_us = src.size() * kBudgetNsPerByte[codec]/1000.0` with `kBudgetNsPerByte = {0.1,6.0,6.0,6.0,4.3,3.2,7.5}` |
| **call sites** | `src/anvil.cpp:2868` | `auto z = g_hotop_budget ? encode_stream_budget(*v2) : encode_stream(*v2);` — **exactly one** call site |
| **default** | `src/anvil.cpp:1617` | `static bool g_hotop_budget = false;` |
| flag plumbing | `src/anvil.cpp:4664` | `g_hotop_budget = opt.hotop_budget ? true : false;` |

**Conclusion: the default encode path is constant-c.** The size-proportional path exists only behind
`--hotop-budget=on`, which is **default-off**. Therefore:

- The queue''s **A0 = "exact current constant-c objective with lambda=0.01"** is **correct**.
- The queue''s **A1 = "exact existing size-proportional budget objective using one frozen non-off flag
  literal"** is **correct**, and the flag-literal requirement is precisely right, because the arm is
  unreachable without naming the literal.
- My D1 was the error. I read a second function''s presence as the shipped default. **Retracted in full.**

**Corroboration:** Track 06''s independent code-read reached the same conclusion before this correction.
Two independent reads agreeing is the reason this is retracted rather than re-argued.

### C1.1 The sharper, dimensionally correct version of the finding I was reaching for

The default objective''s cost term is `lambda * c` with `lambda = 0.01` and `c` drawn from
`{10,20,22,30,35,40,45}`. Its **entire dynamic range** is:

```
lambda * (c_max - c_min) = 0.01 * (45 - 10) = 0.35
```

`L` is an **integer byte count**. Since the cost term''s full span is **0.35 < 1**, the J-ordering can differ
from the pure-`L` ordering **only when two candidates have identical byte length**. That is a **proof, not a
projection**:

> **On the default path, the decode-cost term is an exact-length tie-breaker and nothing else.**

This is precisely what the S6-1 selector manifest measured: **792 selections, ZERO raw-flips, 139 flips,
all 139 exact-L ties** (`legacy L == budget L` in 139/139), every flip `logical_eq=1 ∧ dec_eq=1`
(`RESEARCH_LEDGER.md:3965-3979`). **The arithmetic and the measurement now agree exactly.** My earlier
"1 byte = 100 µs of decode" and "streams must exceed 13.5 KB" claims were computed on the **flag-gated**
path and do not describe default behaviour; they are retained below only as the A1-arm characterisation.

**Flag-gated (A1) arm, for completeness:** cost term = `0.01 * S * nsPerB/1000`, so its dynamic range is
`7.4e-5 * S` bytes. It can break a ~1-byte difference only for `S` ≳ **13.5 KB** `[DERIVED]`; below that it
degenerates to the same exact-tie-only behaviour. A1 is therefore a genuine length-keyed experiment — and
it is a **flag-on, one-site** path, so its baseline is the documented chain
(`budget-OFF == legacy` 22/22, `budget-ON == legacy SIZE-equal` 22/22).

### C1.2 Consequential narrowing of my `` FORMAT.md drift '' claim

I claimed `FORMAT.md:631-663` contradicts the binary. **That was wrong in substance.** `FORMAT.md:648`''s
constant-c table `{10,22,20,30,35,40,45}` **matches the default `encode_stream` path exactly**.

The residual, much weaker defect: **`FORMAT.md` is silent on the second, flag-gated
`encode_stream_budget` path and its distinct `kBudgetNsPerByte` calibration.** That is a
documentation-**completeness** gap in the normative spec, not a contradiction — and `docs/decoder-audit.md`
already records that this section was corrected once (the `lambda = 0.04` multiplicative text). Severity
downgraded accordingly; the "`format`-lane defect" wording in my §0/§6 is withdrawn. **B2 in the track-18
audit is corrected to match.**

**D2 (Q3 timing denominator) is UNAFFECTED and STANDS.** It concerns the whole-codec denominator, not which
objective is default, and remains correct: stage 2 = 26 % of e2e, so a 20 % improvement *inside stage 2* is
5.2 % of e2e and must be refused against the ~7 % paired floor.

---

## C2 — Q2: my "RSS-primary rewrite" **RETRACTED**. The queue''s scope is correct.

I misread Q2 as a BWT-subblocking/RSS-tuning job. It is not. On re-reading, Q2 is a **block/window PARITY
instrument**: its purpose is to make Class-A (ANVIL 256 KiB independent blocks) and Class-B (128 MiB
ratio-mode blocks) and the reference arms geometrically comparable **so that ratio/frontier classification
is fair**. That is a **bytes + classification** job, with a paired timing estimator present only so that
dominance is not decided by noisy timing.

- **My proposal to re-scope Q2 to "RSS-primary" is retracted.** The queue never intended subblock tuning;
  I imported that from the I10-1A.2 subblock sweep, which is a different, already-closed experiment.
- **Q2''s framing is right:** bytes/classification primary; RSS may be **descriptive**; the timing estimator
  is a **classification guard**, and Q2''s timing must never be read as a performance claim.

### C2.1 Two residual points that survive re-scoping (classification validity, not performance)

1. **The "5–17 %" restart tax is a BYTE quantity, and the queue''s wording is ambiguous.** E4''s finding is
   byte-based (`webster` sum-of-4-MiB-blocks **+894,315 B (+12.22 %)** vs whole-file; block oracle
   **+5.42 MB … +643 KB** across scales). It is **not** a timing percentage. Recommend the queue label it
   explicitly as a **byte** restart tax so no lane reads it as a decode-speed figure.
2. **Parity now has a SECOND geometry dimension that the queue does not yet name.** I10-1A added an inner
   aux-index layer inside the outer BWT subblock (`0xFE` aux tag inside `0xFF` subblocks). So
   "block/window geometry" is now **(outer subblock size) x (inner aux sampling rate)**. Fair parity requires
   **both** pinned, and Q2 must pin the aux rate explicitly — otherwise **Q9** (`aux W1024 vs W16`) becomes a
   silent classification variable for every arm that runs after it. This is a **design-integrity** point,
   not a performance claim, and it is the one thing I would still add to Q2.

---

## C3 — Q4b: `k_p` figures are dimensionally **wrong** for the walk. Corrected derivation; WSI kill is
## NO LONGER established by my §3, though the prior still favours it.

**My error.** The `6.05x / 3.86x / 6.60x` figures (`docs/anvil-i9-findings.md` §6.2) are **postcoder**
requirements derived with the `k_p` correction, i.e. they state what the **postcoder** must deliver
**assuming a free inverse-BWT walk**. Citing them as the **LF-walk''s** requirement compares an object with a
different object. **Withdrawn from the walk argument.** They remain valid only as postcoder requirements.

### C3.1 The correct quantities: post-aux overall target, and the unknown walk share

**Current post-aux whole-codec decode** `[MEASURED]`, `docs/I10-AUX-UNBWT-RESULTS.md` §11:

| corpus | ANVIL aux | xz -9e | Brotli q11/lw30 | ratio vs xz | ratio vs Brotli |
|---|---:|---:|---:|---:|---:|
| Silesia | 46,466,339 B / **47.9 MB/s** | 82.5 MB/s | 166.9 MB/s | **1.7226x** | **3.4474x** |
| enwik8 | 23,537,422 B / **26.5 MB/s** | 103.6 MB/s | 150.2 MB/s | **3.9065x** | **5.6642x** |

**Current post-aux overall acceleration still required** to reach the pre-declared **2x margin** `[DERIVED]`:

| path | current | to reach 2x margin |
|---|---:|---:|
| Silesia vs xz | 1.7226x | **already inside** |
| **Silesia vs Brotli** | 3.4474x | **1.724x** |
| **enwik8 vs xz** | 3.9065x | **1.953x** |
| **enwik8 vs Brotli** | 5.6642x | **2.832x** |

> **Post-aux overall requirement = 1.724x (minimum) to 2.832x (maximum).** `[DERIVED]`

**The post-aux LF-walk share is NOT MEASURED.** The 50.2 % / 61.0 % splits I used in §3 come from the
**pre-aux** pinned sha `62BC6631` with `libsais_unbwt` (`prototypes/i9-decode-perf/REPORT.md` §11 v2).
`libsais_unbwt_aux` *is itself a walk acceleration*, so the post-aux walk share is necessarily **lower** than
50–61 %. Using the pre-aux share to argue about post-aux walk headroom is exactly the error the coordinator
identified.

### C3.2 The dimensionally correct walk-specific Amdahl test

Let `w` = post-aux LF-walk share of BWT-path decode, and let a walk kernel achieve speedup `s`. Whole-path
gain:

```
G(w,s) = 1 / ( 1 - w * (1 - 1/s) )
```

Since `1 - 1/s < 1`, a **necessary** condition for reaching whole-path requirement `R` is:

```
w >= 1 - 1/R
```

| binding path | required whole-path `R` | necessary walk share `w` |
|---|---:|---:|
| Silesia vs Brotli (minimum bar) | 1.724x | **w >= 0.420** |
| enwik8 vs xz | 1.953x | **w >= 0.488** |
| enwik8 vs Brotli (maximum bar) | 2.832x | **w >= 0.647** |

> **If the post-aux walk share `w` is below 0.420, then NO walk acceleration — however fast — can reach even
> the minimum 1.724x whole-path requirement, and WSI/lane-scheduler work is dead under every outcome.**
> On the stricter enwik8-vs-xz path the threshold is **0.488**.

**What this changes in my §3.** My statement that "WSI is dead under all outcomes" is **no longer supported by
my own evidence** and is withdrawn as a conclusion. What survives:

- `f_walk` optimisation **has already been built, integrated, measured and landed** (`libsais_unbwt_aux`,
  1.364x/1.795x/2.339x, +22,398 B, I10-1A). A *second* walk-scheduler effort is therefore at high risk of
  re-burning an adopted, measured result — which `docs/audit-2026-09-07/06-do-not-reburn.md` E5 and the
  master-brief doctrine forbid. **That is a duplication argument, not an Amdahl argument, and it stands.**
- Because aux explicitly targeted the walk, `w` **plausibly sits near or below 0.420**. That is a **prior**,
  not a result.

### C3.3 Corrected Q4b specification

**Run Q4b as a byte-only + RATIO record. It is cheap and it is not floor-limited** — stage shares are ratios
of one paired whole-codec measurement, so no absolute-ns attribution and no zero-byte-change control is
required to resolve them, unlike every other timed job in this queue.

**Mandatory output:** `w` = post-aux LF-walk share of BWT-path decode, on the frozen I10-1A candidate, for
`dickens` and `webster`, split into **postcoder / ISA construction / LF walk / CRC / framing+allocation**.

**Frozen thresholds:**

- **T-Q4b.1 (kill WSI):** if `w < 0.420` on either cell ⇒ **KILL** WSI/lane-scheduler. If
  `0.420 <= w < 0.488` ⇒ **KILL on the enwik8-vs-xz path**; at most a *conditional* hold for Silesia.
- **T-Q4b.2 (fund):** `w >= 0.647` ⇒ a walk lane is admissible; publish the achievable `s` requirement as
  `s >= 1/(1 - (1 - 1/R)/w)` per path before any prototype is written.
- **T-Q4b.3 (validity):** A/A null CI spans 1.0; containers byte-identical to the I10-1A candidate; the
  instrumented decoder verified byte-identical to the production path on every block (the t3 protocol's own
  requirement).

**Ordering consequence:** Q4b''s measurement must be taken **on the post-aux candidate**, not by reusing the
pre-aux §11 table. That makes it a genuine new measurement — but a ratio-only, byte-only one.

---

## C4 — Status of my other queue findings

| item | status after correction |
|---|---|
| D1 (Q3 A0/A1 swap) | **RETRACTED** — queue correct; my error was conflating binary presence with default behaviour |
| D1.1 (new) | **NEW, PROVEN**: on the default path the cost term is an **exact-length tie-breaker only** (range 0.35 < 1 byte); matches the 792-selection manifest exactly |
| D2 (Q3 whole-codec timing denominator) | **STANDS** |
| D3/Q4b | **CORRECTED** — `k_p` figures withdrawn from the walk argument; WSI kill now conditional on the **unmeasured** post-aux walk share `w`, with threshold **0.420 / 0.488 / 0.647** by path. Duplication argument against a second walk effort **stands** |
| Q2 RSS-primary rewrite | **RETRACTED** — queue scope correct; only the "5–17 % is bytes, not timing" labelling point and the new **outer-subblock x inner-aux-rate** parity dimension survive |
| Q0 decode-conditioned-crossing check | **STANDS** |
| Q1 block-count census | **STANDS** (and pairs with C2.2) |
| Q4a four-file scope (F1/F2) | **STANDS** |
| Q5 mask-share pre-gate | **STANDS** |
| Q6 warmup-tax denominator | **STANDS** |
| Q7 mandatory `.text` / 1 MiB decompressor-cap arm | **STANDS** — strongest single addition in this review |
| B4 CAM reopen condition rewrite | **STANDS** — unaffected by this correction; it rests on the state-dimension arithmetic and on CTW/PPM being a designated diagnostic control, none of which touch the objective's default path |
| §5 reordering | **PARTIALLY WITHDRAWN** — the Q3-Stage-1-before-Q2 move **stands**; the Q2 re-scope **is withdrawn**; the Q4b "downgrade + kill" **becomes** "downgrade to a ratio-only record, kill conditional on `w`"; the Q7-above-Q4b move **stands** (Q7 is bytes-only and requires no decode work) |

---

*Correction appended by Fledge Alpha Free, 2026-10-02. Three coordinator challenges upheld, two
retractions, one dimensional correction. `REMOTE-EXPERIMENT-QUEUE.md` remains unmodified — it is the
coordinator''s file. Provenance rule honoured: corrections recorded, not silently edited.*
