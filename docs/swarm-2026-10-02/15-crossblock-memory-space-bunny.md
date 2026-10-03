# Track 15 — Long-Range / Cross-Block Memory — Space Bunny Free (REVISED, POST-RECONCILIATION)

**Date:** 2026-10-02 (rev 2; supersedes rev 1 of the same day)
**Agent:** Space Bunny Free (constructive inventor), track `15-crossblock-memory`
**Status:** **Split ruling. `KILL-PIM` (phrase/dictionary reuse, as specified).
`HOLD-CAM` → conditional `PILOT-CAM`,** gated on one byte-only remote census.
Rev 1 of this report proposed PIM. It is withdrawn. Nothing in it survives except
its prior-art work and its negative arithmetic, both of which are retained below
*as the reason for the kill*.

**Read first:** `docs/swarm-2026-10-02/15-crossblock-memory-fledge.md` (adversarial
review). Its root-cause finding is correct and I do not contest it. This revision
concedes all three kill grounds, corrects my own broken threshold, withdraws one
independence claim, and pivots the family to model-state carryover — plus four
additions the critic's census lacks.

Labels: **[M]** measured, artifact cited. **[A]** arithmetic on **[M]**, inputs shown.
**[H]** hypothesis/projection. **[U]** unverified (includes all remaining prior art).

---

## 0. Verdict up front

| item | ruling | ground |
|---|---|---|
| **PIM** (self-extracting phrase memo, distance-independent IDs) as specified in rev 1 | **`KILL`** | §3: wrong deficit (**no measurement of any phrase-reuse component exists**), arithmetically dead (its own gate `≥6` vs derived `k*≈11`, and `k≈41` for a 1 % win), and **not implementable as specified** (zero-wire eviction ⇔ prefix-dependent IDs). None of the three requires a measurement. |
| **Track 15 as a phrase-reuse track** | **`KILL-CURRENT`** | also re-treads R9, whose evidence base reads "**none yet**" (`docs/research-agenda.md:211,314-324`). |
| **CAM family** (compact adaptive-model prior per block) | **`HOLD`** → **`PILOT-CAM`** iff the §7 census promotes | targets the **one measured, primary-source-attributed** component; costs **0.11–0.45 %** against a **5–17 %** tax; preserves independent decodability, random access and parallel decode **structurally**. |

**The single number that decides the family is `D_cross`** (§7.3): ANVIL's own
fragmentation deficit minus the reference's pure fragmentation tax at identical
block sizes. `D_cross ≤ 0.5 %` ⇒ close the track permanently. `D_cross ≥ 2 %` ⇒
CAM earns a pilot **and only CAM**. This is the critic's discriminator; I adopt
it and add three arms to it (§7.4).

---

## 1. What I concede to the critic, and what I add

### 1.1 Conceded without reservation

1. **Wrong deficit.** The measured +2.01 %…+16.98 % is model-warmup tax, named as
   such in the primary source, and E4's reopen condition names **model** carryover
   (`docs/audit-2026-09-07/13-e4-block-routing-oracle.md:42-46,73-76`). A phrase
   memo carries no entropy-model state, so it recovers **none** of the measured
   component. Rev 1's §1.2 quoted the same regret as "the headroom" without
   carrying the attribution across — that was my error, and it is the error the
   reconciliation was meant to catch.
2. **My promote gate contradicted my own arithmetic.** Rev 1 §7 set
   `median mult(64) ≥ 6`; rev 1 §3.2 derived `k* ≈ 11` for *break-even* under the
   same parameters. Corrected in §3.2 below. **This was a real defect**: in a
   document whose entire value is pre-registration, a gate below its own
   break-even is a gate that cannot fail.
3. **My independence claim was violated by my own eviction design.** Rev 1 §2.3
   asserted "a block … needs only the entry contents it names — **not the memo
   prefix**", while §2.4 specified FIFO eviction over promotion order. Those are
   mutually exclusive. **Withdrawn in full** (§3.3). I attempted to rescue it with
   "eviction is a pure function of the entry stream, so the decoder applies it for
   free" — true for *linear* replay, useless for *random access*, which is the
   property the claim was sold on.
4. **Free alternative dominates.** `--block=` is already parsed
   (`src/anvil.cpp:4965`) and `--parse=ratio` already forces whole-file
   (`src/anvil.cpp:4999`). Zero metadata, zero state, zero cycles, zero novelty
   risk. A memo cannot beat a flag.
5. **Adopt-class representation.** Distance-independent content-addressed
   backreferences and differential near-duplicate admission are published
   (arXiv:1701.04451, arXiv:1901.02720; see §6). I stand by that withdrawal.
   The critic adopts it without verifying my citations; so do I not assert them
   as independently confirmed.

### 1.2 What I could not refute, and what I add instead

I tried three source-level escapes from the kill and each one closed:

| attempted escape | why it fails |
|---|---|
| "the regret must partly be phrase reuse, because it is 13–17 %" | No. **B1 (§2.2)** measures the regret curve of a codec with **no LZ phrase reuse at all** and it is the same size. Nothing needs to be assumed. |
| "drop eviction, keep prefix-free IDs" | Works — and lands exactly on **R9**, pre-existing, evidence "none yet". It deletes the only novelty residue (drift/lifecycle), so it is not an escape, it is a surrender to the same prior art. |
| "self-primed models can be charged as phrase reuse because they also fix reach" | False. Priming the entropy model from a block's own prefix cannot restore *history*; and history loss is addressable **only** by a larger block, i.e. the free flag. |

**Additions I contribute (none of them rescue PIM):**

- **A1 — B1: a measured regret curve with and without phrase reuse** (§2.2), from
  existing artifacts, which converts the critic's *attribution* into a *bound*.
- **A2 — the zero-wire variant** (§4.2): CAM's prior can be derived from the
  block's **own already-transmitted prefix**, making the transmitted prior
  strictly unnecessary. This is my main constructive contribution: it dominates
  the transmitted-prior variant on bytes, on independence, and on random-access
  latency.
- **A3 — the shuffled-prior null arm** (§7.2, arm M3). Without it the ablation
  cannot distinguish "the prior carries information" from "any warm start helps",
  and the whole run is uninterpretable. The critic's §9 census has no such arm.
- **A4 — parse-freeze by token-stream hash** (§5.3): makes byte deltas
  *provably* model-state-only, which is a stronger form of the matched-length
  control arm the critic demands in its §6.5.

---

## 2. Evidence map

### 2.1 Root-cause facts (all **[M]**, critic-verified, re-confirmed by me)

| # | Fact | Source |
|---|---|---|
| M1 | Default `block_size = 256*1024` | `src/anvil.cpp:3769` |
| M2 | `--parse=ratio` ⇒ 128 MiB (whole file) | `src/anvil.cpp:4999` |
| M3 | `--block=` already parsed | `src/anvil.cpp:4965` |
| M4 | "**The entire regret is model-warmup tax**: independent blocks restart MTF/context state… **Framing is negligible; warmup is the whole story.**" | `13-e4-block-routing-oracle.md:42-46` |
| M5 | Reopen condition: "near-zero warmup backend, or **cross-block model carryover**" | same `:73-76` |
| M6 | Grid = Brotli q11/lgwin30 + xz −9e; margins bytes < xz, encode ≤3×, decode ≤2×, peak RSS ≤2× | `docs/audit-2026-09-07/11-front-crossing-criteria.md:19-25,58-69` |
| M7 | Frozen frontier 33 non-dominated / 5 FRONT-GAP / **0 FRONT-CROSSING** | `RESEARCH_LEDGER.md:4711-4714` |
| M8 | Class A bench rows never set `block_size`; Brotli rows are one whole-file stream at `BROTLI_DEFAULT_WINDOW` | `tools/bench_native.cpp:25,39` |
| M9 | Parallel decode: `nthreads = min(decode_threads, segs.size())` — block independence is **monetized** | `src/anvil.cpp:4908` |
| M10 | Modes 11–15 are block-stateful (MRU, channels, per-shape displacement **delta chain**, hot-op book) | `FORMAT.md:35-51` |
| M11 | Per-block framing = 11 B; 809 blocks over Silesia ⇒ 8,899 B = **0.0192 %** of 46,446,995 B | `FORMAT.md:25-33`, `src/anvil.cpp:4669,4672` |

### 2.2 **B1 (new, mine): the fragmentation-regret curve with and without LZ phrase reuse**

`tests/block-oracle.csv` covers the **same 127,689,719 B** of the 5 mixed Silesia
files at all four block sizes — `blen_sum` is identical (490/123/33/11 rows at
256 KiB/1 MiB/4 MiB/16 MiB), so the sums below are complete, not truncated **[A]**,
and every `brotli_bytes`/`bwt_coded_bytes` figure is **[M]** from that CSV, with
whole-file baselines **[M]** from `tests/block-oracle-wholes.csv`.

| block size | Brotli Σ-blocked | BWT Σ-blocked |
|---:|---:|---:|
| — (whole file) | **32,973,049** | **37,526,319** |
| 256 KiB | 37,505,108 | 42,325,838 |
| 1 MiB | 35,809,594 | 40,107,682 |
| 4 MiB | 34,506,081 | 38,498,798 |
| 16 MiB | 33,586,712 | 37,904,675 |

Regret **[A]** (`Σblocked − whole`, and as % of that codec's own whole-file total):

| block size | **Brotli** (has LZ phrase reuse) | **BWT** (**no** LZ phrase reuse) | difference |
|---:|---:|---:|---:|
| 256 KiB | +4,532,059 (**+13.74 %**) | +4,799,519 (**+12.79 %**) | **0.95 pp** |
| 1 MiB | +2,836,545 (+8.60 %) | +2,581,363 (+6.88 %) | −1.72 pp |
| 4 MiB | +1,533,032 (+4.65 %) | +972,479 (+2.59 %) | 2.06 pp |
| 16 MiB | +613,663 (+1.86 %) | +378,356 (+1.01 %) | 0.85 pp |

**Two consequences, both load-bearing.**

1. **BWT contains no LZ dictionary, no match finder and no phrase reuse of any
   kind** — it is a block-sort transform with a restarting MTF/context model — and
   it pays **+12.79 %** at 256 KiB. Therefore the *pure* warm-up floor at 256 KiB
   is ≈12.8 pp, and the fraction of Brotli's +13.74 % that is **not** warm-up is
   **at most ≈0.95 pp** (and is a generous bound: BWT's context model is far more
   expensive to warm than Brotli's, so the true warm-up share of Brotli's regret is
   plausibly higher and the phrase-reach residue correspondingly lower). At 4 MiB
   the bound is ≈2.06 pp.
2. **The ordering is not even stable** (BWT is *worse* than Brotli at 1 MiB), so
   the two curves cannot be differenced into a clean decomposition. The honest
   statement is a **bound plus an attribution**, not a decomposition — and the
   bound is what kills PIM: **even under maximum generosity to phrase reuse, the
   entire addressable share of the measured regret is ~1–2 pp, and PIM needs
   ≥0.42 pp of *net* win at a 1 MiB cap merely to break even (§3.2)** — inside its
   own noise band, against a mechanism that costs independence.

This is my answer to the coordinator's demand to *prove* whether PIM attacks a
measured component: **it does not, and the size of the residual it could possibly
address is now bounded at ~1–2 pp by artifacts already in the repo.**

### 2.3 The confound that must be ruled on first

**[M8 + M2]** Class A (the 13-file frozen grid) encodes ANVIL at 256 KiB blocks
and Brotli as one whole-file stream. So the Class A comparison charges ANVIL a
fragmentation deficit the reference never pays, on exactly the files where the
deficit is largest. Class B (Silesia/enwik8 remote, `--parse=ratio`) runs ANVIL
whole-file and is **at parity**. **A refutation is as valuable as a confirmation**
and arm A3 exists to produce one.

---

## 3. Why PIM is killed (retained as the negative record)

### 3.1 Wrong target

PIM carries byte content. The measured tax is paid in symbol distributions
(M4, M5). By B1 (§2.2) the phrase-reuse share is ≤~1 pp at 256 KiB and ≤~2 pp at
4 MiB. PIM spends decode cycles (+0.5–1.6 % **[A,H]**) and RSS (+~0.5 % **[A]**,
on an axis already 2.0–2.4× behind Brotli) to chase ≤2 pp against an
unattributable residue. **No offsetting axis.**

### 3.2 The arithmetic, with the threshold corrected

Rev 1's model, kept verbatim so the correction is auditable. Admission cost for
an entry of length `L` at memo compression factor `γ`, referenced `k` times:
`(1 + L·γ)·8 / k` bits/use. Credit per use: `δ` (measured distance-class cost) −
10 (hot id) − 4 (type flag).

| `δ` | net/use | 64 B entry, γ=0.15 ⇒ `k*` (break-even) | γ=1.00 (pessimistic) |
|---:|---:|---:|---:|
| 14 bits | **0 bits** | **never pays** | never pays |
| 22 bits | 8 bits | **≈ 11** | ≈ 65 |
| 26 bits | 12 bits | **≈ 7** | ≈ 43 |

**Corrected gates (symmetric credit, one frozen `δ`, per the critic's §9.4):**

- **Rev 1's `median mult(64) ≥ 6` is withdrawn.** It is below `k*` at `δ=22`.
- **Break-even gate: median `mult(64) ≥ 11`** at `δ=22`, `γ=0.15`.
- **PROMOTE gate for a 1 % net win at the 1 MiB cap: median `mult(64) ≥ 41`** **[A]**
  (1 % of 46,446,995 B = 464,470 B; 16,384 entries × 12.6 B = 206 KB of admission;
  464,470 B / 12.6 B ≈ 36,868 references / 16,384 entries ≈ 2.25… **recomputed
  below** — see correction note).
- **Credit must be frozen to a single measured `δ`** applied to both kill and
  promote arms. Rev 1 used 14 to kill and 22 to promote; that asymmetry biases a
  pre-registration toward PROMOTE and is withdrawn.

**Correction note (honest arithmetic fix):** the 1 %-win derivation must be done as
`k = (admission_bytes × 8) / (net_bits_per_use × references_needed)`. With
16,384 entries at 12.6 B each = 206 KB admission, net 8 bits/use = 1 B/use, a 1 %
net win of 464,470 B needs **464,470 references** = **28.3 uses per entry**. So the
correct 1 %-win threshold at the 1 MiB cap is **`mult ≈ 28`, not 41**; the critic's
41 corresponds to a 4 MiB-scale or γ-adjusted configuration. I record **both** and
freeze the **stricter (`≥ 41`)** so the gate cannot be accused of being tuned. It
is immaterial: both are far above any plausible reuse distribution, and both exceed
the entire ≤2 pp residue from B1.

### 3.3 Independence claim **withdrawn**

Rev 1 §2.3 claimed block *i* needs "only the entry contents it names — not the memo
prefix". With FIFO eviction over promotion order, resolving `MEMO_REF(id=500)` at
block 900 requires the admission+eviction history of blocks 1–899, which is
recorded only inside those blocks. Three escapes, all closed:

| escape | cost |
|---|---|
| transmit the epoch/eviction table | +`O(blocks × entries)` bytes and **prefix dependence survives in the *values***; also defeats the cap |
| make the ID the physical entry index (prefix-free) | eviction becomes impossible ⇒ no drift adaptation ⇒ novelty residue collapses onto R9 |
| differential admission pins bases forever | pinned bases cannot be evicted ⇒ **unbounded memo growth**, defeating the cap |

And a fourth, independent of any of these: **decode order.** A block containing a
`MEMO_REF` cannot decode before the blocks that promoted the entry, so it either
**serializes** the parallel decoder that M9 shows ANVIL already monetizes, or
reads a not-yet-populated buffer (correctness hazard). Rev 1's F9 self-test
(entry-table SHA-256 at epoch boundaries) tests *linear replay* and cannot catch
random access. **Claim withdrawn; the internal contradiction needed no measurement.**

### 3.4 Byte charges rev 1 under-counted **[A]**

| item | rev 1 | correct |
|---|---|---|
| page framing | 4 B ⇒ 1,024 B | `varint(page_len)` (2 B for 4,096) + 4 B CRC ⇒ **6 B ⇒ 1,536 B** |
| directory | absent from ledger | `(page, offset, len)` ≈ 6 B × 16,384 ⇒ **98 KB** |
| epoch/eviction state | "0 B" | must be transmitted ⇒ scales with `n/blocksize` |
| differential bases | "0 B" | pinned ⇒ unbounded growth |

Framing is 0.019 % of the budget (M11), so none of this decides anything — which is
precisely why I now treat it as bookkeeping rather than as the reason for the kill.
**The reason for the kill is §3.1–3.3.**

---

## 4. The pivot: CAM — Cross-block Adaptive-Model prior

### 4.1 Statement

Keep blocks fully independent and fully random-accessible. Stop each block's
**entropy model** starting cold, by seeding it from a prior that the block itself
already carries. **Zero inter-block dependency, zero identifier space, zero
prefix requirement.** The reopen condition's own words (M5).

### 4.2 Three variants, ranked (this is the constructive work)

| variant | prior source | wire cost | independence | random access | parallel decode |
|---|---|---|---|---|---|
| **CAM-0 self-primed (primary)** | the block's **own first K bytes** | **0 B** | structural | unchanged | preserved |
| **CAM-1 transmitted prior** | previous block's tail statistics, stored **inside this block's payload** | `P` B/block | structural | unchanged | preserved |
| **CAM-2 checkpointed snapshot** | full model snapshot at block *i*, chained | `P` B/block **+ periodic checkpoints** | **prefix-dependent between checkpoints** | granularity becomes K′ blocks | serialized between checkpoints |

**CAM-0 is my contribution and it dominates CAM-1 on every axis except one.**
Mechanism: the first `K` bytes of the block are coded exactly as today (cold
models — the status quo cost, unchanged); at byte `K` the decoder re-initialises
every adaptive model from the statistics it has *already observed inside this
block*. No byte is transmitted because the information was already in the block.
`K` is derived from `blen`, which is **already transmitted** in the block header
(`FORMAT.md:29`):

```
K = clamp(blen / 16, 256, 16384)          # 0 transmitted bytes
```

Why this attacks warm-up specifically **[A/H]**: an adaptive model's excess cost
over bytes 1..j is a decaying function of j, driven by how many symbols each
context has seen. Priming at `K` replaces "cold for the whole block" with "cold
for the first K bytes only" — a **bounded** residual of `K/B` of the block instead
of an unbounded one. At `K = 4 KiB`, `B = 256 KiB`: residual warm-up exposure is
**1.6 % of the block** instead of 100 %.

**The one thing CAM-0 loses:** if the block is *internally* drifting (its prefix is
atypical of its body), the intra-block prior is worse than a cross-block one. That
is the **only** case where CAM-1 beats CAM-0, and it is a *measurable* question,
not an assumption — which is exactly what the M1-vs-M2 arms settle (§7.2).

**CAM-2 is charged and, I argue, dominated:** it is the only variant that costs
independence, so it is the fallback only if CAM-0 and CAM-1 both fail.

### 4.3 CAM's byte budget, recomputed (coordinator request)

Silesia = 211,938,580 B ⇒ **809 blocks** of 262,144 B (`ceil` of M1) **[A]**;
whole-file ANVIL base **46,446,995 B** **[M, `docs/I10-FRONTIER-RECON-2026-09-24.md:62`]**.
enwik8 ≈ 100,000,012 B ⇒ **382 blocks** **[A]**; base **23,537,422 B**
(`RESEARCH_LEDGER.md:4796-4798`) **[M]**.

| prior `P` | Silesia total | % of complete bytes | enwik8 total | % |
|---:|---:|---:|---:|---:|
| 64 B | 51,776 B | **0.1115 %** | 24,448 B | 0.1039 % |
| 256 B | 207,104 B | **0.4459 %** | 97,792 B | 0.4155 % |
| 1024 B | 828,416 B | **1.7838 %** | 391,168 B | 1.6619 % |

Folding the prior **inside the existing payload** (payload length already
transmitted) costs **0 additional framing bytes**. If a separate length field is
wanted instead: +`uvar` ≤2 B/block = +1,618 B = **+0.0035 %** (immaterial; the
folded variant is primary). **CAM-0 = 0 B in all three columns.**

**Required model-attributable share `s`** — the fraction of the fragmentation tax
that must be *entropy-model* state for the prior to break even **[A]**:

| `P` | at 256 KiB (13.74 % tax) | at 1 MiB (8.60 %) | at 4 MiB (4.65 %) |
|---:|---:|---:|---:|
| 64 B | **0.81 %** | 1.30 % | 2.40 % |
| 256 B | **3.25 %** | 5.18 % | 9.59 % |
| 1024 B | 12.99 % | 20.7 % | **38.4 %** |
| **CAM-0 (0 B)** | **0 %** | **0 %** | **0 %** |

**Decision-grade reading:** `P = 64 B` needs under 3 % of the tax to be
model-state *at every granularity*; `P = 256 B` needs ≤9.6 %; **`P = 1024 B` is
admissible only at ≤256 KiB blocks** and fails at 4 MiB unless ≥38 % of the tax is
model state. **CAM-0 has no break-even threshold at all** — it is the only variant
whose sign is decided purely by the measurement.

### 4.4 Cycle and memory cost

| item | cost | label |
|---|---|---|
| CAM-0 priming, per block | `O(M · K)` where `M` = number of active contexts ≈ `n_ctx × 257` updates once; at `K = 4 KiB`, `M ≈ 10^3`–`10^4` ⇒ **<0.01 %** of a 262,144-byte block's decode cycles | **[H]** |
| CAM-0/1 per-symbol decode | **0 extra cycles** — identical model layout, different initial counts | **[H]** |
| CAM-0/1 extra RSS | **0 B** (reuses the existing model arrays) | **[A]** |
| CAM-1 extra RSS | `+P` bytes, constant, validated before allocation | **[A]** |
| CAM-2 extra RSS | `+P` per retained snapshot; requires retention of *all* snapshots since the last checkpoint ⇒ `+K′·P` unless snapshots are themselves coded | **[A]** |

---

## 5. Attribution: how the byte delta is made *provably* model-only

### 5.1 The contamination problem

**[M10]** Modes 11–15 carry an MRU cache, structural channels, a per-shape
displacement **delta chain** and a hot-op book. Re-running the encoder with a warm
model changes the *cost estimates*, which changes *parse decisions*, which changes
*every downstream token*. A naive cold-vs-warm byte delta is therefore
unattributable — the critic is right and rev 1 did not solve it.

### 5.2 The fix: freeze the parse, vary only the model

The token stream is produced **once** and then re-encoded under different model
initialisations. Match finder, block size, window, and every parse decision are
bit-identical across arms. Only the entropy coder's starting counts differ.

### 5.3 The checkable invariant that proves it

Each arm emits `token_stream_sha256`. **The run is `INVALID` unless all of
M0/M1/M2/M3 emit the identical hash.** This is stronger than the critic's
"matched-length control arm" (§6.5): it proves the phrase/window behaviour is
identical by construction *and* by check, rather than by a control that merely
cancels in expectation. Consequence: **every byte of every delta in §7.2 is
attributable to model state and to nothing else.**

**Implementation cost, stated honestly:** ANVIL does not currently serialise its
token stream, so arm construction needs a bounded **token-dump/restore research
path**. That is a new tool under
`prototypes/swarm-2026-10-02/15-crossblock-memory/space-bunny/`, **not** a
production change, and it is the only code this track is ever authorised to
write. Until it exists, the ablation cannot be run and CAM stays at HOLD.

---

## 6. Prior-art status of CAM — honest `[U]`

- **Warm-starting / priming adaptive models is not novel** in the
  arithmetic-coding and PPM literature, and periodic model *reset* (the PPMd/PPMII
  "restart methods") is adjacent **[U]**. `docs/CONTEXT.md` records the related
  in-repo negative that unconditional order-1 literals fail on cold-start sparsity
  — priming is the same sparsity problem addressed from the other side.
- **Cross-block model carryover trivially exists** by not cutting the stream
  (zstd, LZMA, Brotli all carry it internally); so the container-policy framing is
  adopt-class.
- **Defensible residue, if any:** *warm-starting without surrendering independent
  decodability or random access* — and, sharper, **at zero transmitted bytes, by
  deriving the prior from the block's own already-transmitted prefix** (CAM-0).
- **Frozen separator for track 20:** *"is there published art in which an adaptive
  entropy model is warm-started inside an independently-decodable block from
  information already present in that block, at zero additional transmitted
  bytes?"* If yes, CAM is adopt-class and the only remaining deliverable of this
  track is the **measurement** (`D_cross` and the model-only ablation), which is
  worth running anyway.
- **CAM must not be bundled** with PIM, paged dictionaries (G5D's sealed lane),
  defaults, FSST, front coding, a trie, or hierarchy — the G5D §2 exclusion
  discipline (`docs/I10-G5-PAGED-DICTIONARY-PREREG.md:106-134`).

---

## 7. The remote census / ablation (byte-only; no prototype)

**GitHub Actions only.** Same job, pinned binaries, alternating arm order,
warmups, median ≥9, paired A/A null, ambient/CV gate per
`docs/github-actions-benchmark-protocol.md`. **Discovery set only: Silesia 12 +
enwik8 at `tests/corpus/CHECKSUMS.txt` SHA-256. No held-out object is opened.**

### 7.1 Stage 0 — the discriminator (no ANVIL change, no new tool)

| arm | what | purpose |
|---|---|---|
| **A0** | `anvil c … --parse=ratio` (⇒ whole file) | ANVIL at parity (Class B regime) |
| **A1** | `anvil c … --block=262144` | ANVIL in the shipped Class A regime |
| **A2** | `anvil c … --block=4194304` | intermediate |
| **A3** | `anvil c … --parse=ratio --block=262144` | proves the override is what Class A lacks |
| **B0** | Brotli q11/**lgwin 30**, whole file | the bar, window-matched to A0 |
| **B1** | Brotli q11/lgwin30 cut into 256 KiB independent streams | pure fragmentation tax |
| **B2** | same, 4 MiB | reproduces E4's effect at controlled scale |

```
D_anvil  = bytes(A1) - bytes(A0)
D_brotli = bytes(B1) - bytes(B0)
D_cross  = D_anvil - D_brotli          # representation-specific residue — THE number
```

Stage 0 also **rules on §2.3**: per-file `bytes(A0)` against the published Class A
ANVIL totals. A refutation is as valuable as a confirmation.

### 7.2 Stage 1 — the byte-only model ablation (needs the §5.3 token-dump tool)

**Byte-only: this stage emits compressed bytes and identity hashes. It makes no
timing or throughput claim of any kind.** Fixed across all arms: block size,
window, parse, `token_stream_sha256`.

| arm | model initialisation | wire cost | answers |
|---|---|---|---|
| **M0** | cold (today) | 0 | baseline |
| **M1** | **CAM-0** self-primed from the block's own first `K` bytes, `K = clamp(blen/16, 256, 16384)` | **0 B** | how much of the tax is intra-block warm-up? |
| **M2** | **CAM-1** transmitted prior, `P ∈ {64, 256, 1024}` B, folded into the payload | `P` B/block | does cross-block context beat intra-block, and at what `P`? |
| **M3** | **shuffled/null prior**: a prior of the same size `P` built from a *different block's* statistics (deterministic, seeded) | `P` B/block | **is the gain information, or just "not starting from zero"?** |
| **M4** | re-parse with warm model (diagnostic **only**) | — | contaminated by §5.1; never used for attribution |

**M3 is load-bearing.** If `M2 ≈ M3`, the gain is a warm-start artifact carrying no
cross-block information and CAM-1 is not worth its bytes. If `M2 ≫ M3`, cross-block
statistics carry real signal. **Without M3 the ablation is uninterpretable**, and
this is the single most important difference between my census and the critic's.

### 7.3 Frozen thresholds

**`KILL-TRACK-15` (whole family) if any holds:**

1. `D_cross ≤ 0.5 %` on **both** corpora — the deficit is entirely generic
   warm-up; nothing for a representation layer to add beyond the flag. **This is
   the discriminating outcome.**
2. `D_anvil ≤ 0.5 %` aggregate — nothing to recover.
3. `bytes(A3) ≠ bytes(A1)` for any file — arm design invalid.
4. Median 64 B long-range reuse multiplicity below the **corrected** break-even
   (§3.2) — relevant only if a phrase layer is ever reopened. *(Moot unless 1 fails.)*
5. Track 20 resolves the CAM separator in §6 adversely **and** Stage 0 shows
   `D_cross ≤ 0.5 %`.

**`HOLD-FAMILY` (build nothing) if** `D_cross > 0.5 %` but CAM's own falsifier
(`M1` recovery) is below its zero-wire noise floor — i.e. a deficit is real but
neither variant clears it. No prototype; a re-run requires a **different frozen
hypothesis recorded in advance**, never a moved threshold.

**`PROMOTE-TO-PILOT-CAM` (CAM-0 first, then CAM-1; never PIM) if all hold:**

1. `D_cross ≥ 2.0 %` on **both** corpora;
2. `D_brotli ≥ 5.0 %` at 256 KiB — the tax is real and large;
3. **M3 control passes:** `bytes(M1) − bytes(M3·equivalent) < bytes(M1) −
   bytes(M0)` — i.e. the recovered bytes are attributable to *content*, not to a
   generic warm start;
4. **CAM-0:** `bytes(M0) − bytes(M1) ≥ 0.30 %` aggregate at **zero** wire cost
   (threshold set from the 64 B/1024 B break-even band of §4.3, deliberately above
   the 0.11 % floor so a null cannot promote);
5. **CAM-1** (only if CAM-0 lands below 0.30 %): `bytes(M0) − bytes(M2) > P`
   charges — i.e. strict net win **after** the `P` bytes are charged;
6. Stage 0's §2.3 confound is **confirmed or refuted** and recorded either way.

**`INVALID`** if any arm's `token_stream_sha256` differs across M0–M3, or if the
A/A null or ambient gate fails.

### 7.4 Why Stage 0 comes first

Stage 0 needs **no new tool and no ANVIL change** — the `B` arms are a thin wrapper
over the already-pinned Brotli library. It is the cheapest experiment in the
programme and it can return `KILL-TRACK-15` on its own, which would make Stage 1
and the token-dump tool unnecessary. **I therefore re-rank the track: free flag →
Stage 0 → Stage 1 → phrase reuse only if a residue survives.**

---

## 8. Adversarial failure cases (CAM-specific)

| # | Attack / failure | Required behaviour |
|---|---|---|
| C1 | Block prefix atypical of block body (intra-block drift) | M1 underperforms M3 ⇒ `HOLD`; do not rescue with a bigger `K` |
| C2 | Adversary crafts a block whose prefix is designed to poison the model | worst case = cold model, i.e. no worse than today; bound the priming `M` updates and require the arm to never exceed M0 bytes on any single block |
| C3 | `K` derivation must not depend on untransmitted data | `K` derives from `blen` only (already in the block header); self-test asserts equality across arms |
| C4 | Priming must not touch parse state | invariant: MRU, channels, displacement chain, hot-op book bit-identical before/after priming |
| C5 | Truncated/over-declared block | unchanged: `blen`/`payload_len` checks and block CRC-32 (`FORMAT.md:25-33`) |
| C6 | CAM-2 checkpoint corruption / mid-stream seek | CAM-2 is out of scope for the pilot; if ever built, per-checkpoint CRC + `K′`-block granularity must be charged and measured |
| C7 | `P` chosen after seeing results | forbidden; the `P ∈ {64,256,1024}` set is frozen (§4.3) and each `P` is charged |
| C8 | Interaction with `--parse=ratio` mode-17 path and DEFLATE replay | out of scope; must be an explicit NO, not a silent omission |
| C9 | Prior built from a *different* block (M3) accidentally shipped | M3 arms are research-only and must never be selectable in any encoder path |
| C10 | Zero-byte-cost claim falsified by an implicit side channel | self-test: total file bytes must equal the M0 total plus `Σ_refs` deltas exactly; no unaccounted bytes |

---

## 9. What each verdict forbids

**`KILL-PIM` forbids:** any `MEMO_REF` token, any new block mode, any container
field, any CDC/Buzhash boundary machinery, any differential-admission path, any
paged/exact in-band dictionary (G5D's sealed lane), any edit to `src/anvil.cpp`,
`FORMAT.md`, `tools/bench_native.cpp`, or any workflow.

**`HOLD-FAMILY` permits:** exactly Stage 0 (§7.1). Nothing else. No held-out
corpus, no production edit, no prototype.

**`PROMOTE-TO-PILOT-CAM` permits additionally:** the bounded token-dump/restore
tool under `prototypes/swarm-2026-10-02/15-crossblock-memory/space-bunny/`, and
the Stage 1 arms. Still no production edit and no transform-ID allocation.

---

## 10. Final recommendation

**`KILL-PIM`** — on three independent grounds, none of which required a
measurement: wrong target (§3.1, now *bounded* at ~1–2 pp by B1), arithmetically
dead (§3.2, threshold corrected and my broken gate withdrawn), and not
implementable as specified (§3.3, independence claim withdrawn). Plus two
independent rejections: adopt-class representation (§1.1.5) and a free flag that
dominates on every axis.

**`HOLD-CAM`** — the targeted component is measured, primary-source-attributed,
named verbatim in the reopen condition, costs **0 B** (CAM-0) to **0.45 %**
(CAM-1 at `P=256`) against a **5–17 %** tax, and preserves independent decoding,
random access and parallel decode *structurally*. Not promoted yet because nobody
has ever measured ANVIL's own fragmentation deficit, and the one number that
decides it (`D_cross`) costs one measurement-only remote run.

**Recommended sequence, cheapest first:** (1) Stage 0 census, executable as written
in **§11** and shareable with Track 04 → (2) token-dump tool + Stage 1 ablation
*only if* Track 20 has ruled on the CAM separator **and** Stage 0 returns
`D_cross ≥ 2 %` → (3) phrase reuse **never**, unless a residue survives *and* the
corrected multiplicity gate (`≥ 41`, not `≥ 6`) is met.

**CAM is frozen.** No `CAM-0`/`CAM-1` build, no wire change and no Stage 1 work
before Track 20 returns prior-art status (§11.10). Stage 0 may run now, because it
advances no mechanism.

**The single number: `D_cross`.** ≤ 0.5 % ⇒ this track closes permanently. ≥ 2 % ⇒
CAM-0 deserves a pilot.

---

## 11. Closeout protocol — the block/reference asymmetry census (shared with Track 04)

**Byte-only. Measurement-only. No ANVIL change, no new mode, no production edit.**
This is the Stage 0 protocol of §7.1, written to be executed by another agent
without recourse to me. Track 04 owns the dense frontier; this protocol is
**additive** and must not alter any frozen grid row (§11.9).

### 11.1 The question, stated as an asymmetry

The canonical comparison is **asymmetric by construction** **[M8]**: the ANVIL
candidate is cut into 256 KiB independent blocks while the Brotli reference is a
single whole-file stream. Any byte difference therefore confounds three
separable causes:

1. **coder restart** — each block restarts the entropy model (E4's measured
   "model-warmup tax", `13-e4-block-routing-oracle.md:42-46`);
2. **reach loss** — a block cannot reference anything before its own start;
3. **representation** — ANVIL modes 10–15 vs whole-file mode 17 are different
   representations, not different block sizes.

This protocol separates (1) from (2) **entirely on the reference**, and holds (3)
constant on the ANVIL side by construction (§11.5).

### 11.2 Non-negotiables

- **Byte-only.** No timing, throughput, RSS or cycle value is emitted, claimed, or
  joined from any other run. Arms from different measurement windows are never
  spliced; if a timing question is later asked it requires its own same-job
  controls (Track 04's remit).
- Corpus, binaries and parameters are frozen **before** dispatch, with SHA-256 for
  every artifact.
- Every arm is round-trip verified **by decode + byte-compare** before its bytes
  are accepted. E4 recorded that the Brotli helper "occasionally returns exit 0
  with no output under load" (`13-e4-…:89-91`), so exit code alone is never
  accepted as evidence.
- One job, one image, one runner class; arm order alternated; a paired **A/A
  null** must reproduce byte-identically.
- Discovery set only. **No held-out object is opened by this protocol.**

### 11.3 Frozen inputs

| item | identity source |
|---|---|
| local corpus | `tests/corpus/CHECKSUMS.txt` (SHA-256 per file) |
| Silesia 12 + enwik8 | identities pinned in `docs/I10-CORPUS-LOCK-PROTOCOL.md`; re-verify against `docs/I10-FRONTIER-RECON-2026-09-24.md` |
| ANVIL binary | built from the live worktree commit; record commit, source blob, source SHA-256, binary SHA-256 |
| Brotli library | the **already-pinned** library used by `tools/bench_native.cpp`; record version + package identity. No version change, no new dependency |
| Brotli parameters | quality **11**, mode **GENERIC**, `lgwin` **explicit per arm** (never `BROTLI_DEFAULT_WINDOW`, which is the confound) |
| runner | per `docs/github-actions-benchmark-protocol.md` + `docs/GITHUB-ACTIONS-BENCHMARKING.md`; ambient/robust-CV gate per `tests/pr-4-measurement-window-protocol.md` |

### 11.4 The arm set

**ANVIL side (representation held constant at modes 10/12/15):**

| arm | block size | note |
|---|---|---|
| `AN-256K` | `--block=262144` | the shipped default regime **[M1]** |
| `AN-4M` | `--block=4194304` | mid granularity |
| `AN-WF` | `--block=` **whole-file-equivalent** | see §11.5; **must be the same mode**, never `--parse=ratio` |
| `AN-OVR` | `--parse=ratio --block=262144` | proves the override is what the Class A regime lacks **[M2, M3]** |

**Brotli side — the decisive 3-arm decomposition** (all one job, same library):

| arm | streams | `lgwin` | isolates |
|---|---|---|---|
| `BR-WF` | 1 (whole file) | **30** | the bar **[M6]** |
| `BR-WIN` | 1 (whole file) | **log2(block)** — 18 for 256 KiB, 22 for 4 MiB | **reach loss alone** (coder continuous) |
| `BR-SPLIT` | N = ceil(size/block) independent streams | **30** (a 256 KiB stream cannot reach past its own length regardless) | **coder restart alone** (reach = block) |

```
D_anvil       = bytes(AN-256K) - bytes(AN-WF)          # candidate's fragmentation deficit
D_brotli      = bytes(BR-SPLIT) - bytes(BR-WF)         # reference's fragmentation tax
D_cross       = D_anvil - D_brotli                     # representation-specific residue
reach_penalty = bytes(BR-WIN)  - bytes(BR-WF)          # reach component, reference-side
restart_pen.  = bytes(BR-SPLIT) - bytes(BR-WIN)        # restart component, reference-side
```

`BR-WIN` and `BR-SPLIT` have **identical reach capability** (both <= block back)
and differ only in coder continuity, so their difference is the restart component
with no reach term mixed in. `BR-WIN` and `BR-WF` have **identical coder
continuity** and differ only in reach. This is a clean 2x2 and it needs no ANVIL
build at all.

> **Falsifiable prediction, frozen now:** `restart_penalty >> reach_penalty`
> (predicted >=3x at 256 KiB), consistent with E4's attribution and with the
> <=0.95 pp bound of §2.2. If `reach_penalty` dominates instead, §2.2's reading of
> the E4 data is wrong and must be re-derived before anything else proceeds.

### 11.5 "Where format permits" — whole-file-equivalent eligibility, computed not assumed

Revision 1 caps a block at **64 MiB** and revision 2 at **128 MiB**
(`src/anvil.cpp:4850`); `--parse=ratio` forces 128 MiB **[M2]**. So a
whole-file-equivalent arm in modes 10–15 is **only representable for files
<= 64 MiB** (e.g. mozilla 51.1 MB, `samba`/`ooffice`/`sao`), and **not** for larger
ones (e.g. `webster` 139.7 MB).

Rules, applied mechanically:

1. For each file, if `size <= 64 MiB`, emit `AN-WF` in **modes 10/12/15**
   (`--block=67108864`) — representation-matched, clean.
2. If `size > 64 MiB`, do **not** substitute `--parse=ratio` into the `AN-WF`
   column (that would change representation and confound (3)). Instead emit
   `AN-WF-NA` with `whole_file_equivalent_possible: false`,
   `governing_cap_bytes: 67108864`,
   `reason: "file exceeds revision-1 block cap"`, and report `AN-4M` as the
   finest permitted granularity.
3. Record the eligibility decision **per file in the artifact**. Never extrapolate
   a blocked number to whole-file, and never average across mixed representations.

### 11.6 Emitted rows

One JSON object per (file x arm), plus one run-level manifest:

```json
{
  "schema": 1,
  "protocol": "TRACK15-CLOSEOUT-ASYMMETRY",
  "arm": "BR-SPLIT",
  "file": "webster", "file_role": "discovery",
  "source_bytes": 0, "source_sha256": "",
  "arm_bytes": 0, "compressed_sha256": "",
  "block_size_requested": 262144, "block_count": 0,
  "brotli_quality": 11, "brotli_lgwin": 30,
  "whole_file_equivalent_possible": true,
  "governing_cap_bytes": 67108864,
  "roundtrip_verified": true,
  "implementation_sha": "", "binary_sha256": "",
  "timing_emitted": false
}
```

Mandatory: `timing_emitted: false` on every row, so a later join cannot silently
mix windows.

### 11.7 Validity gates — any failure => `INVALID-TRACK15-CLOSEOUT`

1. `compressed_sha256` identical across an A/A repeat for every arm/file.
2. `roundtrip_verified == true` for every arm/file (decode + byte-compare).
3. `block_count == ceil(source_bytes / block_size_requested)`, except the final
   block; and the sum of reconstructed blocks == `source_bytes` for every arm.
4. `BR-SPLIT` total equals the sum of its per-block payloads + per-block framing,
   with framing reconciled explicitly (11 B/block at 256 KiB, **[M11]**).
5. `AN-OVR` bytes equal `AN-256K` bytes per file — a mismatch means an unaccounted
   `--block`/`--parse=ratio` interaction and invalidates the arm design.
6. `BR-WF` reproduces the published whole-file Brotli total for the corpus slice
   within the frozen tolerance, else the library/parameter identity differs and
   the run is diagnostic-only.
7. Corpus SHA-256s match `tests/corpus/CHECKSUMS.txt` and the Silesia/enwik8 lock.

### 11.8 Expected reproduction targets — **directional, not pass/fail**

From §2.2 **[M]**, on the 5-file Silesia slice with `lgwin30` q11:

| | Brotli whole | 256 KiB split | 4 MiB split | 16 MiB split |
|---|---:|---:|---:|---:|
| expected bytes | 32,973,049 | 37,505,108 | 34,506,081 | 33,586,712 |
| expected regret | — | +13.74 % | +4.65 % | +1.86 % |

These were produced by E4's helper on a different build, so a mismatch is **not**
automatically a bug: check binary/library identity first and, if identities differ,
report the run as its own measurement window (§11.2) rather than splicing.

### 11.9 What Track 04 must not do with these rows

- Do **not** write them into `tests/pareto-*.csv`, `tests/benchmark-*.csv`, or any
  frozen grid without a separate explicit ruling; they are an additive arm set.
- Do **not** compute `EXTENDS_FRONT` / `FRONT-CROSSING` from them: the protocol
  emits **bytes only**, and `11-front-crossing-criteria.md:58-69` requires the cost
  axes (decode <=2x, RSS <=2x, decoder size) to be measured in the same window.
  A byte-only row can be `BYTE-WIN` at most.
- Do **not** combine `AN-WF` from files <=64 MiB with `AN-WF-NA` from larger files
  into one aggregate without flagging the representation mix (§11.5.3).

### 11.10 Dependency gate — CAM is frozen until Track 20 rules

Binding, recorded here so it cannot be lost:

| step | may run when |
|---|---|
| §11 protocol (Stage 0, measurement only) | **now** — it advances no mechanism |
| Stage 1 `M0–M3` ablation + the token-dump tool | **only after** Track 20 returns prior-art status for the CAM separator in §6, **and** Stage 0 returns `D_cross >= 2.0 %` |
| any `CAM-0`/`CAM-1` build or wire change | Stage 1 passes §7.3 `PROMOTE-TO-PILOT-CAM` |

**If Track 20 resolves the separator adversely, CAM is `KILL` and the only
remaining deliverable of this track is the measurement itself** — which is still
worth archiving, because it converts the project's one unexplained large loss
(the 256 KiB/whole-file gap) into an attributed one.

---

### Provenance statement

Rev 2 written after reading `docs/swarm-2026-10-02/15-crossblock-memory-fledge.md`.
The §2.2 regret curves are computed from **existing** artifacts
(`tests/block-oracle.csv`, `tests/block-oracle-wholes.csv`); coverage was verified
by summing `blen` per block size and confirming all four totals equal
127,689,719 B, so the sums are complete rather than a truncated resumable file.
No compression benchmark, sweep, timing experiment, statistical experiment, or
fuzz campaign was run locally. No commit, push, reset, clean, stash, restore, or
rebase was performed. No pre-existing file was modified; this report is the only
file I have written. Every prior-art claim I could not verify is marked **[U]**.
Every derived number is marked **[A]** with its inputs shown so it can be
recomputed independently — including one place (§3.2) where recomputation
corrected the critic's figure and where I froze the **stricter** of the two values.