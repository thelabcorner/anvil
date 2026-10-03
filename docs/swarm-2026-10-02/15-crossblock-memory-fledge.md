# Track 15 — Long-Range / Cross-Block Memory — Fledge Alpha Free (ADVERSARIAL REVIEW)

**Date:** 2026-10-02
**Agent:** Fledge Alpha Free (independent adversarial reviewer), track `15-crossblock-memory`
**Status:** Reconciled synthesis. Evidence reconstructed independently from source
*before* reading `15-crossblock-memory-space-bunny.md`; disagreements are stated
explicitly in §4.
**Writes:** this file only. No existing file modified. No commit/push/reset/
clean/stash/restore/rebase. No local benchmark, sweep, or fuzz campaign. No
prototype created (§10.3).
**Labels:** **[M]** = measured, with artifact I verified myself. **[A]** =
arithmetic on **[M]**, shown so it can be re-checked. **[H]** = hypothesis.
**[U]** = unverified; includes every prior-art claim I could not confirm.

---

# §0. ROOT-CAUSE FINDING — lead with this

**The track is aimed at the wrong mechanism, and the repo already says so in its
own primary source.**

Three facts, each verified from source or from the primary artifact, together
localize the entire cross-block deficit and identify what caused it:

### 0.1 The deficit is **model-warmup tax**, not lost phrase reuse

> "**The entire regret is model-warmup tax**: independent blocks restart
> MTF/context state… **Framing is negligible; warmup is the whole story.**"
> — `docs/audit-2026-09-07/13-e4-block-routing-oracle.md:42-46` **[M]**

Measured, aggregate over the 5 mixed files, whole-file base **31,950,395 B**
(`13-e4-block-routing-oracle.md:26-31`) **[M]**:

| block size | regret | % | warmup tax |
|---:|---:|---:|---:|
| 256 KiB | +5,424,162 B | **+16.98 %** | +5,431,727 B |
| 1 MiB | +3,244,121 B | +10.15 % | +3,248,022 B |
| 4 MiB | +1,654,047 B | **+5.18 %** | +1,654,047 B |
| 16 MiB | +643,453 B | +2.01 % | +643,453 B |

webster at 4 MiB: Σ BWT blocks 8,211,644 vs whole-file BWT 7,317,329 = **+12.2 %**;
"**Brotli shows the same (+773,686 B, +9.3 %)**". **[M]**

**This attribution is the root cause and it is not in dispute anywhere.** It says
the cost of cutting a file into blocks is paid in *symbol distributions the model
has not yet learned* — not in *history the match finder has not yet indexed*.

### 0.2 E4 reopens on **model** carryover, and explicitly not on a phrase memo

> "Reopen condition: only if a backend with near-zero warmup cost appears, or with
> **cross-block model carryover (so independent blocks do not restart state)**."
> — `13-e4-block-routing-oracle.md:73-76` **[M]**

The reopen condition names **model** state. It does not name a phrase dictionary.
A phrase memo carries phrases. It carries **no entropy-model state**. By §0.1's
own attribution it therefore recovers **none** of the measured +5.18 % to +16.98 %.

### 0.3 The canonical grid does not even measure the regime where this is true

| evidence class | ANVIL block size | Brotli reference | verdict |
|---|---|---|---|
| **Class B** (Silesia/enwik8, remote) | `--parse=ratio` ⇒ **128 MiB ⇒ whole file** (`tools/reference_cost_gate.py:136`, `tools/bench_ratio.py:124-182`; override at `src/anvil.cpp:4999`) | one whole-file stream | **at parity** |
| **Class A** (13-file frozen grid) | **256 KiB** (default `src/anvil.cpp:3769`; `tools/bench_native.cpp:25` never assigns `o.block_size`, and never uses `parse="ratio"`) | one whole-file stream at `BROTLI_DEFAULT_WINDOW` = lgwin 22 = 4 MiB | **confounded — candidate has 1/16 the reference's cross-block memory** |

**[A]** Class A therefore charges ANVIL a 256 KiB-block deficit that the
reference never pays, on exactly the multi-block files where the deficit is
+16.98 %. ANVIL mode 17 is already configured at **lgwin 30 = 1 GiB**
(`FORMAT.md:135`, `src/anvil.cpp:3877`) — so this is **not** a capability limit.
It is a **measurement-configuration artifact**, independently verifiable in four
lines of source, and it is nowhere stated in
`docs/I10-FRONTIER-RECON-2026-09-24.md`.

**Net root cause.** The cross-block deficit in this project is (a) *real*, (b)
*measured*, (c) *attributed to model warmup*, (d) *already recovered for free* in
the Class B configuration by `--parse=ratio`, and (e) *mis-measured* in the Class
A configuration. The one quantity that a phrase memo would address has never been
measured, and the one quantity that has been measured is not what a phrase memo
addresses.

---

# §1. Framing cost is negligible — so nothing can be won or lost on framing

**[A]** Per-block framing is `uvar(blen) + mode byte + uvar(payload_len) +
4 B CRC-32` (`FORMAT.md:25-33`, `src/anvil.cpp:4669`) **[M]**. At a 256 KiB block:

| field | encoding | bytes |
|---|---|---:|
| `blen` = 262,144 | 19-bit uvarint ⇒ 3 B | 3 |
| mode | 1 B | 1 |
| `payload_len` ≈ 57,000 | 3 B | 3 |
| CRC-32 | fixed | 4 |
| **total** | | **11 B/block** |

Silesia = 211,938,580 B ⇒ **809 blocks** (`src/anvil.cpp:4672` partitions at
`min(block_size, remaining)`) ⇒ 809 × 11 = **8,899 B** against a whole-file total
of **46,446,995 B** (`docs/I10-FRONTIER-RECON-2026-09-24.md:62`) **[M]**:

```
8,899 / 46,446,995 = 0.0192 %
```

E4 independently reaches the same order: "framing is negligible (≥ 4 MiB, < 200 B
total)" `[M]`.

**Consequence for the mandate's "duplicate table charges" ask.** Framing is
0.019 % — it can neither decide nor distort any mechanism here. **Any candidate
that tries to win on framing is chasing 0.02 %.** The real accounting items are
the ones in §6 and §7, and they are orders of magnitude larger.

---

# §2. Model-state carryover ≠ phrase/dictionary reuse — the distinction the track collapses

This is the core of the adversarial case. The two mechanisms are often described
in the same words; they are structurally unrelated, target different measured
quantities, and have opposite economics.

| | **MSC — model-state carryover** | **PDR — phrase/dictionary reuse** |
|---|---|---|
| **what crosses the block boundary** | *symbol distribution* (context-model frequencies) | *byte content* (phrases / dictionary entries) |
| **what is paid at each block start** | cold adaptive model: every context's counts start at zero, so early symbols are coded at near-`log A` bits | a match finder must re-index; but LZ history is cheap to keep (§2.1) |
| **targeted deficit** | **warmup tax — MEASURED +5.18 %…+16.98 %** (`13-…:26-46`) | phrase reuse — **[U], never measured in this repo** |
| **named in E4's reopen condition** | **yes, verbatim** | no |
| **cost side** | **+P bytes/block**, P ≤ ~256 B ⇒ **0.11–0.45 %** (§8.2) | **+Σ(3 + 0.15·len) bodies** + page framing + any transmitted epoch state (§6.1, §7.2) |
| **cost side vs deficit** | **1–2 orders of magnitude BELOW** the deficit | **ABOVE or equal to** its own break-even (§6.1) |
| **blocks stay independently decodable** | **structurally guaranteed** — the prior is inside block *i* | **not achievable** with zero-wire eviction (§7.1) |
| **random access** | one block + its prior | requires replay of all preceding blocks' admission history |
| **parallel decode** (`src/anvil.cpp:4908`) | **preserved** | **serialized** |
| **decoder state growth** | `+P`, constant, transmitted, validated pre-allocation | `+D + directory`, prefix-dependent |
| **novelty status** | **[U]** — narrow residue, §8.4 | **adopt-class** by the constructive lane's own withdrawal (§5) |

### 2.1 Why PDR cannot be recovering the measured deficit even in principle

Two files can have **identical** long-range content opportunity and **different**
warmup tax: take one file's bytes and prepend 256 KiB of high-entropy noise. The
phrase-reuse structure is untouched. The warmup tax is destroyed. Therefore:

> **Warmup tax is orthogonal to phrase reuse.** No amount of phrase reuse removes
> it, and no amount of phrase-memo engineering targets it.

The converse also holds and is why the distinction is not academic in the other
direction: phrase reuse *is* free to keep in a real LZ stream — Brotli's ring
buffer already carries it across its internal blocks at zero metadata cost. The
only thing ANVIL's block reset destroys that a *window* would have kept is the
**adaptive model**, because a complete Brotli bitstream per block (`FORMAT.md:135`)
throws away the entropy coder's history too.

---

# §3. Reconstruction ledger (verified by me, before reading the constructive lane)

| # | Fact | Where |
|---|---|---|
| **M1** | `struct Options { uint32_t block_size=256*1024; … }` | `src/anvil.cpp:3769` |
| **M2** | `if(opt.parse=="ratio" && !block_explicit) opt.block_size=128u<<20;` | `src/anvil.cpp:4999` |
| **M3** | `--block=` is already parsed | `src/anvil.cpp:4965` |
| **M4** | Container partitions at `min(block_size, remaining)`, one block header each | `src/anvil.cpp:4669,4672` |
| **M5** | Backend 1 = **"one complete Brotli bitstream"** per block, policy q11 / **`lgwin=30`** / generic | `FORMAT.md:135` |
| **M6** | Decoder enables large-window | `src/anvil.cpp:3877` |
| **M7** | Every distance is validated against **block-local** output, never the file; `FORMAT.md:736-738` states the 64 KiB encoder window is an *advisory* and "a future encoder may widen the window without a new mode" | `src/anvil.cpp:2925,3045,3225,3316,3501,3756` |
| **M8** | Serial decode does `out.insert(out.end(),…)` — **all blocks appended into one buffer** | `src/anvil.cpp:4889` |
| **M9** | Parallel path builds `vector<vector<uint8_t>> blocks` then joins ⇒ peak ≈ **2× output** | `src/anvil.cpp:4907-4909` |
| **M10** | `nthreads = min(opt.decode_threads, segs.size())` — block independence is **monetized as parallel decode** | `src/anvil.cpp:4908` |
| **M11** | Measured decode peak RSS: Silesia **248.4 MiB**, enwik8 **598.9 MiB**; ratios **2.027×/2.379× Brotli**, **4.572×/1.995× xz**, **9.043×/2.378×** (paired, run `35927623136`) | `docs/I10-FRONTIER-RECON-2026-09-24.md:63-78` |
| **M12** | Measured decode throughput (same run): ANVIL aux **47.9 MB/s** Silesia, **26.5 MB/s** enwik8; Brotli q11/lw30 **166.9 / 150.2 MB/s** | same |
| **M13** | Block modes 0–17; modes 11–15 are **stateful across the whole block** (MRU cache, structural channels, per-shape displacement delta chain, hot-op book) | `FORMAT.md:35-51` |
| **M14** | Standing bar is Brotli q11/lgwin30 + xz −9e (Silesia 48,456,004 B, enwik8 24,831,648 B); margins bytes < xz, encode ≤ 3×, decode ≤ 2×, peak mem ≤ 2× | `docs/audit-2026-09-07/11-front-crossing-criteria.md:19-25,58-69` |
| **M15** | Frozen frontier: **33 non-dominated, 5 FRONT-GAP, 0 FRONT-CROSSING**, `GRID-THIN` binding | `RESEARCH_LEDGER.md:4711-4714` |
| **M16** | R9 "Self-extracted cross-block phrase dictionary (dual-timescale), MDL-gated" is **already on the agenda** — novelty MED, evidence "**none yet**", verdict "PASS (conditional)" | `entropy-mix-wt/docs/research-agenda.md:312-324,211` |

**[A] M8+M11 ⇒ ANVIL's decode RSS is Θ(output), not Θ(window)**
(248.4 MiB / ~211.9 MB output ≈ 1.17×). Therefore:

- "Bounded decoder memory" cannot be a lever ANVIL wins — the axis is already
  2.0–2.4× behind Brotli and 4.6–9.0× behind xz;
- any dictionary is added *on top of* an already-Θ(n) decoder, so its marginal
  RSS is ≈ **+0.5 %** (§6.3) — **neutral**;
- **M10 is the real cost of cross-block memory and nobody has costed it:** block
  independence is already converting into parallel decode. A prefix-dependent
  reuse layer converts it back to sequential.

---

# §4. Reconciliation with the constructive lane

### 4.1 Agreed

Cross-block memory is worth real bytes and the deficit is large; reach is not a
lever (211.9 MB ≪ the already-configured 1 GiB); the representation is
adopt-class and the withdrawal is correct; an unmeasured economic base cannot
justify a prototype; measurement-first, remote-only, no-prototype-yet are right;
the memo's marginal RSS and decode cost are negligible; if the kill team resolves
the coupling adversely the answer is KILL, not re-scope into G5D.

### 4.2 Disagreed — six points

1. **Wrong deficit.** PIM targets phrase reuse. The measured +16.98 %/+9.3 % is
   **model warmup**, attributed by name, and PIM carries no model state. → §0, §2.
2. **The promote gate is below its own break-even.** Its gate is median
   multiplicity ≥ 6; its own table gives `k* ≈ 11`; a 1 % win needs **`k ≈ 41`**.
   → §6.1.
3. **Independent decodability is contradicted by its own eviction and
   differential-admission design.** → §7.1. This kill needs **no measurement**.
4. **It states the Class A block confound (its own M12) and does not escalate
   it**, filing the `--block` A/B as PROMOTE condition #5 rather than **#0**. It
   is a provenance defect in the canonical grid and the cheapest experiment in the
   programme. → §0.3, §9.
5. **Asymmetric pre-registration.** 14 bits to kill, 22 to promote — biases the
   gate toward PROMOTE in a document whose value *is* pre-registration. → §9.4.
6. **Contamination unaddressed.** Its F7 guards only the local state write.
   Replacing K matches perturbs **every subsequent** parse decision in the block
   (M13: displacement is a delta chain), so the byte delta is unattributable
   without a **matched-length control arm**. → §6.5.

### 4.3 Where it is stronger than me

Its §1.5 prior-art work exceeds anything I produced. I **adopt its conservative
conclusion without verifying its citations** rather than parroting them (§5). Its
cost-model arithmetic is correct where I differ from it — I independently
verified `(1 + 64·0.15)·8 = 84.8` bits and `84.8/8 ≈ 11`. And its §4.1 already
names "the null hypothesis is free … this is the objection that decides the
track"; my contribution is only that the report does not follow its own
conclusion through.

### 4.4 Charter fact

**This track re-treads R9** (M16), whose evidence base reads "**none yet**" after
being pre-registered with a conditional PASS. R9's own gate was never met. Any
proposal here must either satisfy R9's gate or explicitly supersede it.

---

# §5. Prior-art map

| item | status |
|---|---|
| Static/trained dictionaries — Brotli RFC 7932 (~120 KiB), Zstd trained dicts | prior art **[M]** (`entropy-mix-wt/docs/research-agenda.md:313-316`) |
| In-band / paged exact dictionary with escape — G5D | promoted **adopt-class**, claims no primitive novelty **[M]** (`I10-G5-PAGED-DICTIONARY-PREREG.md:1014-1079`) |
| Distance-independent content-addressed backreferences; differential near-duplicate admission | **[U]** in the dedup literature (lane's arXiv:1701.04451 / 1901.02720 / 2409-06066). **Lane withdraws the claim; I concur without asserting the citations.** |
| CDC + dedup + local reconstruction — Borg / restic / bup class | **[U]** — cross-block content-addressed reuse, bounded rebuildable index, independent block recovery. This is exactly the lane's surviving "coupling" claim. **Sharpest track-20 target.** |
| **[U]** Brotli static-dictionary reference is *itself* distance-independent, content-addressed, zero-metadata, and prefix-free | **the lane's sharpest missed prior-art point.** If confirmed against RFC 7932 §8, the residue is only "self-extracted and grows to megabytes with in-band eviction" — a capacity/lifecycle claim, not a mechanism claim. **Verify before any prototype.** |

**Frozen separator for track 20:**
> Is there published art in which (i) a long-range content-addressed reuse layer
> coexists with **independently decodable blocks**, (ii) reuse memory is **capped
> and its identifier space is prefix-independent**, and (iii) eviction is **in-band
> deterministic at zero wire cost**?

Clause (ii) is where I expect the kill — §7.1 shows the specified mechanism cannot
satisfy it while also satisfying (iii).

---

# §6. Falsification of the constructive mechanism, on its own numbers

### 6.1 The promote gate is below its own break-even — needs no measurement

At the lane's promote-side credit (δ = 22 bits, hot id = 10, flag = 4) net saving
is **8 bits = 1 B/reference**. A 64 B entry at γ = 0.15 costs
`(1 + 64·0.15)·8 = 84.8` bits, so **`k* = 84.8/8 ≈ 11`**. Its gate is **≥ 6**.

Solve for the multiplicity needed to clear a **1 % net win** at its own 1 MiB cap.
1 % of 46,446,995 B = **464,470 B** **[A]**:

| memo size | entries C | B/entry | admission | **k for +1 % net** |
|---:|---:|---:|---:|---:|
| 64 KiB | 1,024 | 12.6 | 12.7 KB | **k ≥ 466** |
| **1 MiB (its cap)** | 16,384 | 12.6 | 206 KB | **k ≈ 41** |
| 4 MiB | 65,536 | 12.6 | 826 KB | **k ≈ 20** |
| 1 MiB, γ=0.05 differential | 16,384 | 4.2 | 69 KB | **k ≈ 33** |
| 1 MiB, δ=26 ⇒ 12 bits/use | 16,384 | 12.6 | 206 KB | **k ≈ 28** |

**[A] Under every parameterization of its own model — including the *pessimistic*
credit it uses to kill — the required median multiplicity is ≥ 28, versus its
gate of ≥ 6.** Note the last row: raising the credit from 8 to 12 bits/use moves
`k` only 41 → 28, because entry-admission cost is fixed. **The mechanism cannot be
rescued by a more generous distance assumption.**

Coverage implication at `k = 20`, 4 MiB cap: 65,536 × 20 × 64 B = **83.9 MB** of
references must exist ≈ **40 % of Silesia's 211.9 MB** covered by ≥ 64 B
long-range content recurring ≥ 20× each. At `k = 41`, 1 MiB cap: **43 MB ≈ 20 %**.
Both are far outside any plausible reuse distribution, and neither is the quantity
its gate measures.

### 6.2 The credit is arm-dependent; PIM is dominated on both arms

The credit `− Σ_refs d_dist(distance_class)` equals the distance code the
reference *would* have paid — correct **only against a whole-file reference**.

- **Against the bar** (Brotli q11 whole file): cross-block content is *already* a
  match at ~14–22 bits ⇒ credit 8–12 bits ⇒ §6.1 applies ⇒ **loses**.
- **Against fragmented ANVIL** (256 KiB): the same content is coded as
  **literals**, so the credit looks huge. **But `--block=` whole file captures
  those same bytes at zero metadata, zero state, zero cycles, zero novelty risk**
  (§7.3).

### 6.3 Cycle and memory accounting — both neutral

**[H, labelled]**

- per reference: **+3–6 cycles** (directory load + fingerprint branch + tier bit
  read) on top of an 8–15 cycle match token;
- per output byte: **0 extra cycles** (same `memcpy`, separate buffer, no overlap
  loop);
- **[A]** Silesia match-covered bytes ≈ 0.819 × 211.9 MB ≈ 174 MB; at 20–30 B mean
  match ⇒ 6–9 M matches ⇒ **+18–54 M cycles**. Against 47.9 MB/s (M12) ≈ 0.97 s
  ≈ 3.4 G cycles ⇒ **+0.5 % to +1.6 % decode throughput**;
- **[A]** memory: +~1.2 MiB (hot 64 KiB + directory 96 KiB + bodies 1 MiB) on
  248.4 MiB ⇒ **+0.5 %**.

**It spends +0.5–1.6 % decode and +0.5 % RSS to buy a byte win that §6.1 says
does not exist. There is no offsetting axis.**

### 6.4 Three of the mandate's four pillars yield no lever

| mandate ask | status | why |
|---|---|---|
| bound dictionary state | **satisfied, irrelevant** | decoder is already Θ(n) (§3, M8/M11); +0.5 % on an axis already 2.0–2.4× behind |
| random-access damage | **actively worsened** | prefix-dependent ID space (§7.1); today block independence buys parallel decode (M10) |
| update cost | **O(blocks), not O(1)** | ID space is a function of all preceding blocks' admission history |
| duplicate table charges | **under-charged** | §7.2; and framing is only 0.019 % anyway (§1) |

### 6.5 Contamination — the A/B cannot attribute its own delta

**[A] from M13.** Modes 11–15 are block-stateful: MRU cache, structural channels
(scores reinforced/decayed by match length/distance), per-shape displacement as a
**delta chain**, hot-op book. Replacing `K` matches with memo refs perturbs
**every subsequent parse decision in the block**. The byte delta is contaminated
by second-order parse-path change and **cannot be attributed to the memo**.

Its F7 guards only the local invariant ("after a `MEMO_REF`, shape/displacement
state bit-identical to pre-ref"). Necessary, insufficient. Any pilot needs a
**matched-length control arm** covering the same bytes at the same lengths and
positions so the perturbation is common to both arms and cancels. **Neither
report specifies this.**

---

# §7. Decoder correctness: the surviving novelty claim contradicts the specified mechanism

### 7.1 Independent decodability is asserted, not achieved

The lane retains novelty only for *"reuse memory separable from parse state, so
blocks stay independently decodable"* + *"zero-wire deterministic eviction"*, then
asserts *"No prefix of the file is required."* Three independent contradictions,
each sufficient:

1. **Eviction history.** FIFO over *promotion order* means the live set at block
   *i* is determined by what blocks `< i` promoted and evicted. To resolve
   `MEMO_REF(id=500)` at block 900 the decoder needs the admission history of
   blocks 1..899 — **recorded only inside those blocks**. Unless the epoch table
   is materialized into the directory it must be **transmitted**, and "zero-wire"
   collapses. **You cannot have both zero-wire eviction and prefix-independent
   identifiers.**
2. **Differential admission.** "Encode the entry against the existing memo … rather
   than verbatim." The entry's **stored bytes** then depend on memo contents **at
   promotion time**; the base must be pinned forever, which **conflicts with FIFO
   eviction**. The design does not say which wins.
3. **Decode order.** A block with a `MEMO_REF` cannot decode before the blocks that
   promoted the entry. Under M10's parallel segment decoder it either
   **serializes** (unmeasured throughput loss) or, naively, reads a
   not-yet-populated memo buffer (correctness hazard). Its F9 self-test
   (entry-table SHA-256 at every epoch boundary) tests *linear replay* and
   **cannot catch the random-access case**.

**Net: the mechanism that survives the prior-art downgrade is the one the
specified mechanism cannot implement.** Either IDs become prefix-dependent (novelty
lost), or eviction/bases must be transmitted (zero-wire lost), or blocks are not
independently decodable (framing lost). **Internal kill; independent of any
measurement, benchmark, or prior-art search.**

### 7.2 Hidden byte charges

| item | lane's charge | correct charge |
|---|---|---|
| page framing | 4 B/page ⇒ 1,024 B / 256 pages | `varint(page_len)` for 4,096 B = **2 B** + 4 B CRC ⇒ **6 B/page ⇒ 1,536 B** — **50 % undercount** **[A]** |
| directory entries | not in §3.1 ledger | `page, offset, len` ≈ 6 B × 16,384 = **98 KB** **[A]** |
| epoch/eviction state | **0 B** ("zero-wire") | must be **transmitted** for prefix-independent IDs; scales with `n/blocksize` **[A]** |
| differential-admission base pinning | **0 B** | pinned bases must never be evicted ⇒ **unbounded** memo growth, defeating the cap **[A]** |

### 7.3 The free alternative, quantified

`--block=` is already parsed (M3); `--parse=ratio` already forces 128 MiB (M2).

| property | `--block=` whole file | PIM memo |
|---|---|---|
| transmitted metadata | **0 B** | ≥ 1.5 KB framing + 98 KB directory + Σ(3 + 0.15·len) bodies + epoch state |
| decoder state | **0 B** | +~1.2 MiB |
| decode cycles | **0** | +0.5–1.6 % |
| reach | **lgwin 30 = 1 GiB, already wired** (M5/M6) | 1 MiB memo — **1024× shorter** |
| novelty risk | **none** | adopt-class (§5) |
| parallel decode | **preserved** (M10) | **destroyed** (§7.1-3) |

**On this project's canonical corpora the flag strictly dominates.** Both Silesia
(211.9 MB) and enwik8 (~100 MB) are far below the already-wired 1 GiB window, and
the lane itself concedes reach is not a lever.

---

# §8. The family that should be held open: CAM

### 8.1 Statement

**CAM — Cross-block Adaptive-Model prior carryover.** Keep blocks fully
independent and fully random-accessible, but stop each block's entropy model
starting cold: block *i* carries a **compact transmitted frequency prior inside
its own payload**, applied before decoding block *i*'s symbols. **Zero
inter-block dependency. Zero prefix requirement. Zero cross-block identifier
space.**

### 8.2 Cost accounting — decisive and cheap

**[A]** Byte cost on Silesia at 256 KiB blocks (809 blocks), base 46,446,995 B:

| prior size P | total | % of complete bytes | vs E4's 4 MiB warmup tax (5.18 %) |
|---:|---:|---:|---:|
| 64 B | 51.8 KB | **0.112 %** | 46× surplus |
| 256 B | 207 KB | **0.446 %** | 11.6× surplus |
| 1 KiB | 828 KB | **1.78 %** | 2.9× surplus |
| 4 KiB | 3.31 MB | **7.13 %** | **deficit — fails** |

**A compact prior must be ≤ ~256 B/block to stay under 0.5 %.** ANVIL's own rANS
headers are already compact by design ("range lo..hi + symbol mask, frequencies
only for non-last symbols") **[M, `docs/CONTEXT.md`]**, so a top-N-symbol prior
over hot contexts landing in the 64–256 B band is **plausible [H]**.

**Cycles [H]:** `O(P)` once per 262,144 output bytes ⇒ **< 0.01 %**.
**Memory [A]:** `+P` bytes, constant, transmitted, bounded, validated before
allocation — the strongest possible answer to the mandate's bounded-memory ask.

**This is the decisive comparison: CAM's cost side is 1–2 orders of magnitude
*below* the deficit it targets (a measured one); PIM's is *above* its own
break-even against an unmeasured one.**

### 8.3 CAM's frozen falsifier

CAM dies if a **≤ 256 B** per-block prior recovers **< 0.45 %** of complete
bytes. Below that the prior cost (0.446 %) exceeds the recoverable warmup.

### 8.4 CAM's honest novelty status

**[U] — not claimed.** Cross-block/warm-start entropy-model carryover is not
obviously novel *as a container policy*: zstd and LZMA carry history across their
internal blocks precisely by not cutting the stream. The defensible residue is
narrow: **a compact transmitted prior that buys warm-start without introducing
any inter-block dependency** — warm-start *without* surrendering independent
decodability. Send to track 20 before calling it anything.
**CAM must not be bundled with PIM, defaults, FSST, front coding, a trie, or
hierarchy** — the G5D §2 exclusion discipline.

---

# §9. Cheapest remote census that discriminates model-state carryover from phrase/dictionary reuse

**GitHub Actions only. No codec change, no new mode, no production edit, no local
execution.** This is deliberately *not* the constructive lane's E15-0 as written.

### 9.1 The discriminator, in one sentence

**Compare ANVIL's own fragmentation deficit against a *pure* fragmentation tax
measured on the reference at identical block sizes.** If ANVIL's deficit is
fully explained by generic warmup, then the deficit is **model state** (hold CAM,
kill PIM). If a residue survives, a **representation** component exists and only
then is a phrase layer worth discussing.

```
D_anvil  = bytes(A1) - bytes(A0)    # ANVIL's own cross-block deficit
D_brotli = bytes(B1) - bytes(B0)    # reference's fragmentation tax = pure warmup
D_cross  = D_anvil - D_brotli        # REPRESENTATION-specific residue  ← the discriminator
```

**This is the cheapest experiment in the programme** because the `B` arms need no
ANVIL build at all — one thin wrapper over the already-pinned Brotli library used
by `tools/bench_native.cpp`, no library change, no version change. E4 already
measured B1/B2 in substance; the only genuinely new measurement is **A1 vs A0 in
ANVIL's own configuration**, which has never been done.

### 9.2 Frozen arms

Same job, same pinned binaries, alternating arm order, warmups, median of **≥ 9**
reps, paired A/A control, ambient/CV gate per
`docs/github-actions-benchmark-protocol.md` (`BLOCKED_AMBIENT` on control robust
CV over the frozen bound). Discovery set only: Silesia 12 files + enwik8 at
`tests/corpus/CHECKSUMS.txt` SHA-256. **No held-out object is opened.**

| arm | command | purpose |
|---|---|---|
| **A0** | `anvil c … --parse=ratio` (⇒ 128 MiB, whole file) | ANVIL **at parity** = Class B regime |
| **A1** | `anvil c … --block=262144` | ANVIL in the **shipped default / Class A regime** |
| **A2** | `anvil c … --block=4194304` | intermediate |
| **A3** | `anvil c … --parse=ratio --block=262144` | **proves the override is what Class A lacks** |
| **B0** | Brotli **q11 / lgwin 30**, whole file | the bar, **window-matched to A0** (removes the lgwin22-vs-lgwin30 asymmetry of §0.3) |
| **B1** | Brotli **q11 / lgwin 30**, cut into 256 KiB independent streams | **pure fragmentation tax at controlled scale** |
| **B2** | Brotli **q11 / lgwin 30**, cut into 4 MiB independent streams | reproduces E4's +9.3 % at controlled scale |

**Emitted per arm/file:** complete bytes, ratio, encode MB/s, decode MB/s, peak
decode RSS, block count, `decode_threads` actually used, output SHA-256 (bytes are
deterministic; the hash is the identity check).

Also emitted, at zero extra cost, the **CAM prior-size feasibility histogram**:
per block, how many active-context alphabet entries are zero-count or low-count —
i.e. what a ≤ 256 B prior could plausibly populate. If that population is large,
CAM deserves its own Stage 1.

### 9.3 Frozen thresholds

**`KILL-TRACK-15-CURRENT` (the PIM lane) if any of:**

1. **`D_cross ≤ 0.5 %` on both corpora** — the deficit is entirely generic warmup,
   so no representation/phrase mechanism has anything to add beyond the flag.
   *This is the discriminating outcome.*
2. `D_anvil ≤ 0.5 %` aggregate — nothing to recover.
3. `P_loss = decode(A1)/decode(A0) < 0.95` **or** `R_delta = rss(A1)/rss(A0) > 1.05`
   for any arm — fragmenting costs throughput/RSS, so the shipped default pays for
   something that does not pay for itself; the correct action is a config fix.
4. `bytes(A3) ≠ bytes(A1)` for any file — an unaccounted ratio-path/`--block`
   interaction; arm design invalid.

**`HOLD-FAMILY` (keep CAM open, build nothing) if `D_cross > 0.5 %` but CAM's
own §8.3 falsifier is unmet** — i.e. a deficit is real and representation-shaped,
but neither the phrase layer (adopt-class, §6.1/§7.1) nor a compact model prior
clears its cost. No prototype; a re-run requires a **different frozen hypothesis
recorded in advance**, never a moved threshold.

**`PROMOTE-TO-PILOT` (CAM only, never PIM) if all of:**
1. `D_cross ≥ 2.0 %` on **both** corpora;
2. `D_brotli ≥ 5.0 %` at 256 KiB — proving the warmup tax is real and large, so
   the problem is worth attacking at all;
3. `P_loss ≥ 0.95` and `R_delta ≤ 1.05` — so a fix pays no new throughput or
   memory cost;
4. `bytes(A0) < bytes(B0)` — ANVIL at parity must be in the right neighbourhood;
5. the CAM prior-feasibility histogram shows a prior-sized population of
   zero/low-count contexts **≥ 5× the number of distinct contexts a 256 B prior
   can carry** — otherwise the prior cannot do the work.

**Independent of the outcome, the census must rule on §0.3**: confirm or refute
`CLASS-A-BLOCK-CONFOUNDED`, with per-file `bytes(A0)` against the published Class A
ANVIL totals. **A refutation is as valuable as a confirmation** — if ANVIL at
128 MiB does not beat its own 256 KiB result on any Class A file, §0.3 is
immaterial and **I am wrong**. Arm A3 exists to be able to prove that.

### 9.4 If (and only if) Stage 1 is ever authorized for a phrase layer

Three corrections to the constructive lane's E15-0, all mandatory:

1. **One frozen credit value for both arms.** Its KILL test uses δ = 14 and its
   PROMOTE test δ = 22 — that asymmetry biases toward PROMOTE. Freeze **one**
   value (median realized `bits_per_ref` measured in Stage 1) and apply it
   symmetrically. **[A]**
2. **Replace the multiplicity gate with the derived one:** median
   `mult_hist(64) ≥ 41` at a 1 MiB cap for a 1 % net win (§6.1) — **not** 6.
   Below the derived break-even the answer is KILL regardless of every other axis.
3. **Add the matched-length control arm** (§6.5). Without it the byte delta is
   unattributable and the run is `INVALID` regardless of its total.

---

# §10. Verdict

> **Reconciled final form (§11b.5 is authoritative and supersedes the
> single-verdict form below):**
>
> - **KILL** the mechanism family — phrase/dictionary cross-block reuse;
> - **RUN** the repaired measurement census (§9, repaired per §11b.4) as
>   engineering hygiene, not as a novelty gate;
> - **HOLD** the model-state carryover family (CAM), converting to PILOT only on
>   `D_cross ≥ 2.0 %` plus a cleared §8.3 falsifier.
>
> Three rulings rather than one, because novelty and engineering value are not the
> same decision and have different owners. **The track's mechanism is KILL-ed
> while the track's measurement is promoted.**

## 10.1 Split ruling (mechanism + family)

### `KILL-TRACK-15-CURRENT` — the phrase/dictionary-reuse mechanism as specified

KILL, on three independent grounds, **none of which requires a measurement**:

1. **Wrong deficit.** The measured +5.18 %…+16.98 % is model-warmup tax
   (`13-…:42-46`), named as such by the primary source; E4's reopen condition says
   **model carryover**, not phrase memo; and Class B already runs whole-file so the
   deficit is not present in the canonical remote configuration (§0).
2. **Arithmetically dead.** Its promote gate (median multiplicity ≥ 6) is below its
   own break-even (`k* ≈ 11`) and far below the derived `k ≈ 41` for a 1 % win.
   Its own model requires ≥ 28 under every parameterization (§6.1).
3. **Not implementable as specified.** Zero-wire eviction forces prefix-dependent
   identifiers; prefix-independent identifiers force transmitted epoch state. The
   surviving novelty claim and the specified mechanism are mutually exclusive
   (§7.1).

Plus two independent rejections on top: the representation is adopt-class by its
own withdrawal (§5), and the free flag dominates on every axis (§7.3). And it
re-treads R9, whose evidence base is "**none yet**" (§4.4).

### `HOLD-FAMILY` — model-state carryover (CAM)

**HOLD**, not KILL, and not PROMOTE. The targeted deficit is **measured and large**,
it is named by the primary source, it is **named verbatim** in E4's reopen
condition, its fix costs **0.11–0.45 %** against a **5.18–16.98 %** deficit, it
preserves independent decodability and random access **structurally**, and it
preserves the parallel decode that ANVIL already monetizes (`src/anvil.cpp:4908`).
**But nobody has ever measured ANVIL's own fragmentation deficit**, and the honest
discriminating experiment costs one measurement-only remote run (§9).

HOLD ends the moment §9.1's `D_cross` is known. If `D_cross ≤ 0.5 %`, the family
closes with the phrase lane. If `D_cross ≥ 2 %`, CAM earns a pilot **and only CAM**.

### 10.2 What each verdict forbids

**KILL-CURRENT forbids:** any `MEMO_REF` token, any new block mode, any container
field, any CDC/Buzhash boundary machinery, any differential-admission path, any
paged/exact in-band dictionary (that is G5D's lane, and it must stay sealed), any
edit to `src/anvil.cpp`, `FORMAT.md`, `tools/bench_native.cpp`, or any workflow.

**HOLD-FAMILY permits:** exactly the §9 census. Nothing else. No held-out corpus.
No production edit.

### 10.3 Why no prototype was written

`prototypes/swarm-2026-10-02/15-crossblock-memory/fledge/` is intentionally
absent. Every open question in this track is answerable by **measurement** or
**prior-art adjudication**, not by code; §7.1 shows the specified mechanism is not
implementable in the form that carries its novelty claim. Writing a prototype now
is the most expensive available way to produce a number that cannot change the
decision. The one artifact worth writing is the §9.2 census driver — a
measurement harness, which belongs to whichever track executes it after
authorization.

### 10.4 The single number

If forced to name one: **`D_cross`**. Below 0.5 % this track is closed permanently
and no fallback is warranted. Above 2 %, model-state carryover deserves a pilot
and phrase reuse does not.

---

# §11. Frozen falsification criteria, restated for the coordinator

Track 15 closes permanently if **any** holds:

1. `D_cross ≤ 0.5 %` of complete bytes on **both** Silesia and enwik8 — the
   fragmentation deficit is fully explained by generic Brotli warmup;
2. `D_anvil ≤ 0.5 %` aggregate — nothing to recover;
3. fragmenting ANVIL to 256 KiB costs **> 5 %** decode throughput or **> 5 %**
   decode RSS — the shipped default is paying for something that does not pay for
   itself;
4. a ≤ 256 B per-block model prior recovers **< 0.45 %** of complete bytes;
5. median 64 B long-range reuse multiplicity is below the derived break-even
   (**41** at a 1 MiB memo cap) — relevant only if a phrase layer is ever reopened;
6. the realized median `bits_per_ref` at long range falls below the single frozen
   credit value applied symmetrically;
7. the prior-art kill team resolves the
   independent-blocks + bounded-prefix-independent-reuse coupling adversely.

Any one ⇒ close. **Thresholds move only by recording a different frozen hypothesis
in advance, never after seeing a result.**

---

# §11b. PAIR RECONCILIATION — can E15-0 distinguish cross-block reuse value from the free null hypothesis?

Direct answer to the coordinator's question: **No. E15-0 as specified cannot
distinguish them, and its one gate that gestures at the question is confounded in
exactly the direction that would promote a dead mechanism.** The defect is
structural, and it is fixable with one added arm plus three redefined statistics.

### 11b.1 The measurement is taken in the arm where the null hypothesis is already in force, for free

E15-0's instrumentation clause, in substance: *"wrap the **reference** encoder
(Brotli q11/lgwin30, the actual grid bar) to log, per emitted match: distance,
length, and the realized output-bit cost attributed to that token."*

Quantities 1–4 — `f_long(d)`, `bits_per_ref(d)`, `mult_hist(L)`, `addressable_bits`
— are **all** measured on **Brotli q11 whole-file at lgwin 30**. That is exactly
the configuration in which cross-block reuse is **already captured at zero
marginal metadata cost** by the codec's own ring buffer (`FORMAT.md:135`,
`src/anvil.cpp:3877`).

> **A positive `addressable_bits` therefore does not establish that a phrase memo
> is worth building.** It establishes that cross-block reuse *exists* — which in
> the measured arm is already being exploited for free by the window.

E15-0 computes the **size of the reuse opportunity**, not the **size of the unmet
reuse opportunity**. Only the second is worth building against.

**`addressable_bits` double-counts, specifically.** It is defined as
`Σ bits_per_ref(d) − 14` and labelled a "gross ceiling". But in the arm where it
is measured, those bits are *already being spent, near-optimally, by Brotli's own
distance model*. Subtracting 14 bits/ref and calling the remainder "addressable"
presumes the memo would code something Brotli currently codes at `bits_per_ref(d)`.
The memo must code it at **strictly less** — which is the entire `k` analysis of
§6.1. So `addressable_bits` ≈ the *existing cost of the reuse*, not the delta a
memo could capture. As a promote gate it is close to a tautology.

### 11b.2 `mult_hist` is measured on a population selected for being referenced

E15-0: *"the reuse-multiplicity distribution of long-range content, **restricted to
content covered ≥ 1 time** at d > 256 KiB."*

That population is **conditioned on Brotli having already chosen to emit a
long-distance match there**. But a long-distance match is emitted only when the
distance code is cheaper than the literal alternative — which **selects for content
that recurs and is well modeled**. The complement (long-range content the window
holds but the parser did not reference) is never sampled.

Consequence: the measured median multiplicity is biased **upward** relative to the
multiplicity distribution of long-range content *as a whole* — and that complement
is precisely the population a memo would have to store in order to beat the window.
**The promote gate (`median mult_hist(64) ≥ 6`) is therefore biased toward PROMOTE
by an amount the run itself cannot bound.** Its KILL twin (`< 3`) is biased toward
KILL. Neither is a valid discriminator; they are two differently-biased estimators
of the same confounded quantity.

### 11b.3 Scorecard: which E15-0 conditions can actually discriminate?

| E15-0 condition | What it really measures | Discriminating? |
|---|---|---|
| KILL-1 `f_long(256 KiB) < 8 %` | a **property of the corpus**, read through Brotli's parser | **No** — a corpus can have `f_long = 20 %` (pass) with **zero** unmet value, or `f_long = 7 %` (kill) with a **huge** warmup deficit. A corpus descriptor masquerading as a mechanism gate. |
| KILL-2 `median mult_hist(64) < 3` | multiplicity **conditioned on being referenced**, wrong arm | **No** — §11b.2 selection bias |
| KILL-3 `bits_per_ref < 12` | **Brotli's distance model**, not the memo | **No** — and irrelevant to the model-state family |
| KILL-4 `addressable_bits < 0.5 %` | gross reuse in an arm where reuse is **already free** | **No** — §11b.1 |
| **PROMOTE-5** "ANVIL `--block` A/B confirms a cross-block deficit ≥ 3 % aggregate" | `D_anvil` — **not** `D_cross` | **Only partly.** Filed **last** rather than first; and it returns **POSITIVE in a world where the free flag already collects 100 % of the deficit**, because `D_anvil` contains the generic warmup term in full |
| PROMOTE-1/2/3/4 (`f_long`, `mult_hist`, `bits_per_ref`, `addressable_bits`) | as above | **No** — inherit §11b.1 / §11b.2 |

**Score: 0 of 4 KILL conditions and 0 of 5 PROMOTE conditions are valid
discriminators for the free-null-hypothesis question. PROMOTE-5 is the only
condition that gestures at it, and it is both last-listed and confounded.**

This is the sharpest form of the answer: **E15-0 would emit
`PROMOTE-TO-PROTOTYPE` in a universe where the only available cross-block mechanism
is already optimal as shipped, because the free flag is never subtracted.**

### 11b.4 Minimal repair — four changes, all free or near-free

1. **Add arms B0 / B1 / B2** — Brotli q11 **lgwin 30**, whole file / cut into 256 KiB
   independent streams / cut into 4 MiB independent streams. **No ANVIL build
   required**; a thin wrapper over the Brotli library already pinned at
   `tools/bench_native.cpp:39`. This single addition converts PROMOTE-5 from
   `D_anvil` to the discriminating `D_cross = D_anvil − D_brotli`, which *is* the
   question. E4 already measured B1/B2 in substance; the genuinely new information
   is A1-vs-A0 in ANVIL's own configuration, which has never been done.
2. **Re-target the instrumentation to the fragmented arm.** Log the token streams of
   **A0 and A1**, and report `f_long`, `mult_hist` and `bits_per_ref` for **both**,
   with the promote gates applied to the **A0-minus-A1 difference** — i.e. to the
   reuse that fragmentation destroyed, not to the reuse the window already holds.
3. **Compute `mult_hist` over *all* long-range content**, using E15-0's own proposed
   independent content-occurrence pass, rather than over referenced content only.
   Apply the promote gate to **that** number; keep the Brotli-referenced number as a
   diagnostic. (Caveat, in the conservative direction: aligned 64 B units miss
   unaligned reuse, so this **underestimates** multiplicity — acceptable for a kill
   gate, not for a promote gate; a pass here is necessary but not sufficient.)
4. **Replace `addressable_bits` with `unmet_bits`:**
   `Σ_{references destroyed by fragmentation} [bits_per_ref(d) − 14]`, computed from
   the A0-minus-A1 token streams. That is the actual delta a mechanism could
   capture, and it is what the promote threshold should be stated against.

**Discriminator decision rule, frozen now:**

| result | reading |
|---|---|
| `D_cross ≤ 0.5 %` on both corpora | **zero unmet cross-block reuse value.** The deficit is entirely model warmup; the free flag is optimal; **PIM KILL, and CAM (§8) is the only surviving family** |
| `D_cross ≥ 2.0 %` on both corpora | a **representation-specific** reuse deficit exists. This is the *only* outcome that reopens a phrase-layer discussion — and even then PIM is still KILL on §6.1 + §7.1, so the survivor remains CAM |

**A note on the cost asymmetry that makes the repair worth doing.** The repaired
census needs no new encoder, no dictionary, no content hashing beyond a single
occurrence pass, and no new decoder. The unrepaired E15-0 needs a wrapped reference
encoder, distance-decile bit accounting, and a `BrotliEncoderCompress` round-trip
boundary — **and still cannot answer the question.** The repair is strictly cheaper
than the thing it repairs.

### 11b.5 Novelty and engineering value, kept separate

The coordinator asked for these to be separated. They are two different decisions
with two different owners, and conflating them is precisely how a dead mechanism
gets promoted.

| | **Novelty** | **Engineering value** |
|---|---|---|
| **PIM / phrase-dictionary reuse** | **ZERO** — the constructive lane's own withdrawal (§1.5 P1–P3: distance-independent content-addressed backreferences are published and analysed order-optimal; the representation is adopt-class). The residual systems-coupling claim is **[U] unverified**, and **not implementable as specified** (§7.1). Route to track 20. | **NEGATIVE, and positively harmful**: it would add prefix-dependent block ordering to a decoder that currently monetizes block independence as parallel decode (`src/anvil.cpp:4908`), to buy a byte win §6.1 puts below zero. |
| **CAM / model-state carryover** | **[U] NOT CLAIMED.** The container-policy residue is likely prior-art; the narrow defensible claim is *warm-start without surrendering independent decodability*. | **POSITIVE and unmeasured.** Targets the one deficit this repo has actually **measured** (+5.18 %…+16.98 %), is named **verbatim** in E4's reopen condition, costs **0.11–0.45 %** against it, and preserves independent decodability, random access, and parallel decode **structurally**. |

**The load-bearing separation, stated as a rule:**

> **Run the repaired census as measurement hygiene regardless of the novelty
> ruling. Do not run E15-0 as a novelty gate, because it cannot serve that role.**

That is not a compromise position; it is the correct decomposition. The repaired
census produces three outputs that are **valid unconditionally**, none of which
depends on any mechanism surviving:

1. **A provenance ruling on the canonical grid.** It settles
   `CLASS-A-BLOCK-CONFOUNDED` (§0.3): whether the frozen Class A grid — the number
   the project's 0 FRONT-CROSSING tuple rests on — compared ANVIL at **256 KiB**
   against a Brotli reference holding **4 MiB whole-file**. This is a defect in the
   project's own measurement apparatus and is **independent of track 15 entirely**.
   **If the census returns `HOLD` or `KILL` on the mechanism, this output is still
   worth having run.**
2. **ANVIL's own fragmentation deficit, `D_anvil`** — never measured. E4 measured
   it on Brotli; nobody has measured it on ANVIL's modes 10–15.
3. **The CAM prior-feasibility histogram** — how many active-context alphabet
   entries are zero-count or low-count, i.e. what a ≤ 256 B per-block prior could
   populate. Free byproduct of the same run.

**Therefore the two-track ruling I recommend the coordinator adopt:**

- **Mechanism ruling (track 15, the phrase/dictionary family): `KILL`.** Grounded in
  §0 (wrong deficit), §6.1 (below its own break-even), §7.1 (not implementable as
  specified), §5 (adopt-class). This ruling is independent of the census and does
  not wait for it.
- **Measurement ruling (the census): `RUN` — repaired per §11b.4.** Justified by
  engineering value alone: it audits the canonical grid's provenance and measures
  ANVIL's fragmentation deficit for the first time.
- **Family ruling (model-state carryover): `HOLD`**, converting to `PILOT` only if
  the repaired census returns `D_cross ≥ 2.0 %` **and** the §8.3 CAM falsifier is
  cleared.

Note the shape of that: **the track's mechanism is KILL-ed while the track's
measurement is promoted.** That is the honest outcome, and it is only expressible
because novelty and engineering value were kept apart. A single verdict could not
carry it.

---

# §13. CLOSEOUT — freezing the block-parity control, and auditing whether it isolates warmup from window loss

## 13.1 The audit question, answered honestly first

> *Does the block-parity control actually isolate model warmup from phrase-window
> effects?*

**Answer: no — not by itself, and I got this wrong in §9 as originally written.**
`D_cross = D_anvil − D_brotli` is a difference of **two equally-conflated**
quantities. Brotli suffers warmup *and* window loss when cut; ANVIL suffers warmup
*and* window loss when cut; subtracting them cancels neither. §9 as written could
not have separated the two components, and §11b's repair did not fix that either —
it fixed the *free-null* confound but not the *warmup-vs-window* confound.

The reason is structural and worth stating as a finding in its own right:

> **In every existing LZ codec, the window and the entropy model live in the same
> stream state, and "cut the stream" resets both. There is no knob that resets one
> without the other.** Therefore **strict isolation is impossible from byte totals
> alone**, for any off-the-shelf codec, at any block size.

Isolation requires either (a) instrumenting the *unfragmented* stream, or
(b) a **non-adaptive model**, which makes warmup ≈ 0 by construction. §13.2
freezes both. §13.5 states the agreement test that decides whether the isolation
succeeded, and §13.6 states the residual bias.

## 13.2 Frozen null arm — geometry only, warmup annihilated

### 13.2.1 The arm that exists in the codebase already

ANVIL **block mode 6 is `default-with-sparse-exceptions`** (`FORMAT.md` suite table;
`docs/CONTEXT.md` "mode 6 default-with-sparse-exceptions"). Its model is a
**constant default plus a sparse exception list**. There is **no adaptive state to
re-prime**: cutting it costs the exception list, which is small and cheap by
construction. Modes 10/12/15 (`separated-stream rANS`, `shape`, `hot-op`) are
**fully adaptive** and therefore carry warmup in full.

**Freezing this as the zero-warmup reference `N-W`:**

| arm | mode | model class | geometry sweep | what it measures |
|---|---|---|---|---|
| **N-W(B)** | mode **6** (default + sparse exceptions) | **non-adaptive** | `--block=B`, B ∈ sweep | **phrase-window loss, warmup ≈ 0** |
| **N-M(B)** | modes **10 / 12 / 15** (rANS family) | **adaptive** | `--block=B`, same B set | **warmup + window loss** |
| **N-M(B)** | mode **14** (TCOPY) | adaptive | same B set | second adaptive point, transformed-reference arm |

`warmup(B) ≈ N-M(B) − N-W(B)` — **in derivative form, not in absolute bytes**
(§13.2.2). This changes **only block geometry** between arms, holds the window
identical (mode 6 and modes 10–15 share the same block-local `dist ≤ out.size()`
validation and the same 64 KiB advisory encoder window, `src/anvil.cpp:2925…3756`,
`FORMAT.md:736-738`), and uses the non-adaptive model to annihilate warmup.

**This is the null arm the coordinator required: geometry-only variation, warmup
removed by construction rather than estimated after the fact.** No new codec, no
new mode, no dictionary, no wrapped reference encoder.

### 13.2.2 The statistic is a normalized geometry-sensitivity, not a byte difference

Modes 6 and 10–15 have **different representations**, so their absolute byte totals
are not comparable and `N-M(B) − N-W(B)` is **not** a byte-quantity. The valid
comparison is the **sensitivity to geometry**, normalized to be representation-free:

```
alpha_mode(B)  =  [ bytes_mode(B) − bytes_mode(n) ]  ·  B / n
```

where `n` = file size and `bytes_mode(B)` = complete bytes for that mode at block
size `B`. `alpha` is "bytes lost per block, normalized": it is **directly comparable
across modes with different representations** because it divides out both the
representation's intrinsic size and the file length.

**Pre-registered acceptance, frozen now:**

> `alpha_mode6(B)` **must be near-zero relative to** `alpha_modes10_12_15(B)` across
> the whole sweep — if mode 6's geometry-sensitivity is *comparable* to the adaptive
> modes', then mode 6 is **not** a valid zero-warmup reference and the isolation is
> **`INVALID-ISOLATION`**. Do not proceed to any warmup/window claim.

**[H]** The expectation, with the reason: mode 6's per-block cost is O(exception
list) not O(priming the adaptive tables), so `alpha_mode6` should be well under
`alpha_rans`. If it is not, mode 6's exception list is itself behaving as an
adaptive model (i.e. exceptions are dense enough to re-prime), and the arm is
void. **This is the audit's own falsifier and it can fail.**

### 13.2.3 The second half: token-level window accounting (free, and the actual isolator)

Independently of the mode-6 arm, instrument the **A0 whole-file token stream**
(no re-compression required; A0 already exists) and compute, for each match token
`i` at output position `p_i` with distance `dist_i` and length `len_i`:

```
unreachable(i, B)  ⟺  floor((p_i − dist_i) / B)  <  floor(p_i / B)      [exact, free]
```

`W_bytes(B) = Σ_{unreachable} len_i` — the bytes the window cannot reach under
B-blocking. These are the bytes the phrase-window mechanism **specifically** cannot
reuse; the rest of the deficit must be warmup or residual.

Emit `W_bytes(B)` at every sweep point, **plus** the count of tokens whose
`dist_i > B` (a weaker, arm-independent proxy for the same quantity, useful as a
cross-check that does not depend on block alignment).

## 13.3 Audit verdict on the §9 control, restated

| question | verdict |
|---|---|
| Does `D_cross = D_anvil − D_brotli` isolate warmup from window loss? | **No.** Both terms conflate both components; subtraction cancels neither. |
| Does it at least isolate *ANVIL-specific* deficit from *generic* fragmentation? | **Partially — yes.** This much is sound: `D_brotli` is a like-for-like generic baseline at matched block size and matched window, so `D_cross` does remove the part of ANVIL's deficit that a generic codec also suffers. **The §9 KILL/PROMOTE thresholds on `D_cross` stand.** |
| Does anything in §9/§11b separate warmup from window loss within `D_cross`? | **No.** That was the gap; §13.2 closes it. |

**So the §9/§11b census survives intact as the free-null test.** What the closeout
adds is the *second*, orthogonal decomposition — warmup vs window — which the
census needs before any mechanism (PIM or CAM) can be credited with anything.

## 13.4 The bias sign analysis (frozen, because both estimates are biased)

**Direction of bias in `warmup(B) ≈ N-M(B) − N-W(B)`:**

- **Over-attributes to warmup [A].** Under B-blocking the *parse* changes, not just
  the reachable set: long matches vanish, so the parser takes different — possibly
  shorter, possibly cheaper — local matches, and the entropy coder adapts to a
  different symbol distribution. So `D_anvil` **exceeds** `warmup + window_loss`,
  and the residual lands in warmup.
- **Under-attributes window loss [A].** Some content that becomes unreachable will
  not become *literals*; it will be re-covered by a shorter match to nearer history
  or by a different route. So `W_bytes` over-states the unrecoverable residue.
- **The two partly cancel, and the net is not sign-definite [A].** That is exactly
  why the point estimate may not arbitrate — **§13.5's agreement test does.**

**Direction of bias in `W_bytes(B)`:**
- `W_bytes` counts *reachable-in-principle* bytes, so it **over-states** true
  window loss (it ignores re-coverage by nearer history).
- It is **exact** on the alignment test, not an approximation — worth stating,
  because it means the *only* error is the re-coverage one, which is one-directional
  and small.

## 13.5 The agreement test — this is the audit

Three independently-derived estimates of the same split must agree:

1. `alpha_window(B) = alpha_mode6(B)` — from the non-adaptive arm (§13.2.2).
2. `alpha_warmup(B) = [alpha_rans(B) − alpha_mode6(B)]` — from the adaptive-minus-
   non-adaptive contrast.
3. `alpha_window(B) ≈ W_bytes(B)/n` — from token accounting (§13.2.3), normalized.

**Pre-registered acceptance (frozen before any run):**

| criterion | tolerance |
|---|---|
| `alpha_mode6(B) / alpha_rans(B)` | **≤ 0.25** for every B in the sweep — else `INVALID-ISOLATION` (§13.2.2 falsifier) |
| `|alpha_window(derived) − W_bytes(B)/n| / alpha_window(derived)` | **≤ 0.35** for every B — else `INVALID-ISOLATION` |
| `warmup_hat(B) = D_anvil(B) − W_bytes(B)` vs `alpha_warmup(B)·n/B` | residual **≤ 20 % of `D_anvil(B)`** at every B — else `INVALID-ISOLATION` |

**On any failure: `INVALID-ISOLATION`.** No warmup claim, no window claim, no
mechanism attribution, no PIM/CAM ruling. The run's byte totals remain valid as
byte totals; only the *decomposition* is void. Record and stop.

**On success**, the run emits the split, and the mechanism question becomes
answerable for the first time:

| outcome | ruling |
|---|---|
| `alpha_window / alpha_rans ≥ 0.5` | a **phrase-window** deficit exists and is material. **Reopens PIM's premise** (though PIM is still KILL on §6.1/§7.1 — the *premise* reopens, the mechanism does not). |
| `alpha_warmup / alpha_rans ≥ 0.5` | the deficit is **model state**, as `13-…:42-46` says. **Confirms CAM as the correct family** and KILLs PIM's premise outright. |

## 13.6 Which existing Class-A conclusions become unreliable if parity changes bytes materially

Class A is the 13-file frozen grid (`tests/benchmark-suite.frozen-bdc90474.plus-xz.csv`,
aggregated in `docs/I10-FRONTIER-RECON-2026-09-24.md` §1), produced by
`tools/bench_native.cpp` with **ANVIL at 256 KiB** (default `src/anvil.cpp:3769`,
never assigned at `bench_native.cpp:25`) against **Brotli whole-file at lgwin 22**
(`bench_native.cpp:39`), while ANVIL's own backend-1 encoder pins
`BrotliEncoderCompress(11, 30, …)` — **q11 / lgwin 30** (`src/anvil.cpp:3866`).

**Threshold for "materially": `|bytes(A0) − bytes(A1)| ≥ 0.5 %` of the Class A
aggregate complete bytes.** Frozen now, before any run.

### 13.6.1 Conclusions that become UNRELIABLE

| # | Class-A conclusion | Why it breaks | severity |
|---|---|---|---|
| 1 | The 13-row aggregate complete-bytes / ratio table for **all 7 ANVIL rows** | every ANVIL row carries the same un-subtracted 256 KiB fragmentation tax | **critical** |
| 2 | **`0 FRONT-CROSSING`** canonical I9 tuple (33 non-dominated / 5 FRONT-GAP / 28 DEGENERATE / 435-of-468 dominated) | computed from the aggregate byte column and the Pareto non-domination set | **critical** |
| 3 | **`GRID-THIN`** binding status | asserted partly because the grid is thin; if the byte column moves, the density conclusion about the grid is re-derived, not inherited | **high** |
| 4 | Any claim of the form **"ANVIL backend 2 (BWT) loses to backend 1 (Brotli)"** | **the penalty is not common-mode across backends.** A compact rANS block re-primes cheaply; a **BWT block re-runs `libsais` on that block** (`FORMAT.md:145-156`), so fragmentation penalizes BWT far harder on both bytes and encode time. The A/B is **biased against backend 2 at 256 KiB.** | **critical** |
| 5 | Any **ANVIL-vs-ANVIL rank order** among `{dp-rans, sparse-rans, TCOPY-rans, MDL-rans, shape-rans, hot-op-rans}` | same non-common-mode problem as #4, smaller magnitude | **high** |
| 6 | **Encode-throughput** comparisons for any BWT/arm row | per-block suffix-array rebuild scales with block count; a 809× block count is a large, unreported encode-time tax | **high** |
| 7 | The ordering `brotli q11 < xz −9e < ANVIL < … < brotli q1` — i.e. the claim that **xz is the binding byte bar and Brotli is the binding decode bar** | the byte column moves; the *decode* column may move the **opposite** way (see §13.6.2) | **medium** |
| 8 | Any **GRID-THIN-dependent** argument about missing intermediate zstd/Brotli window tiers | unchanged as a measurement gap, but the frontier tuple it supports is not | **medium** |

### 13.6.2 Conclusions that are **NOT** affected — and one that moves the *wrong* way

- **Class B is untouched.** The remote Silesia/enwik8 ANVIL rows use
  `--parse=ratio` (`tools/reference_cost_gate.py:136`, `tools/bench_ratio.py:124-182`)
  ⇒ 128 MiB ⇒ **whole file, already at parity.** So these stand unchanged:
  ANVIL aux 46,466,339 B / 47.9 MB/s / 248.4 MiB vs xz 48,456,004 / 82.5 / 54.3 vs
  Brotli 49,383,136 / 166.9 / 124.5 (Silesia); enwik8 23,537,422 B / 26.5 MB/s /
  598.9 MiB, ruling `TIMING_BLOCKED` (Brotli control CV 0.1756).
- **The "ANVIL is 5.95 % / 5.14 % smaller than Brotli on Silesia/enwik8 but decodes
  5.96× / 20.05× slower with 2.17× / 2.87× the decode RSS" claim is a Class B claim**
  and is **not** invalidated by parity.
- **⚠ Parity is not a free win, and this must be stated before anyone celebrates a
  byte improvement.** `src/anvil.cpp:4908` sets
  `nthreads = min(opt.decode_threads, segs.size())`. At 256 KiB, Silesia has **809
  independently-decodable units**; at 128 MiB it has **1**. So:
  - A1 (fragmented) can use **809-way** block-parallel decode;
  - A0 (parity) is forced to **1-way**.

  **Therefore parity may improve bytes while simultaneously destroying decode
  throughput**, and possibly RSS (peak is transiently ≈ 2× output on the parallel
  path, `src/anvil.cpp:4907-4909`). This is why §9.3 condition 3 — `P_loss ≥ 0.95`
  and `R_delta ≤ 1.05` — is a **hard gate on the parity result itself**, not a
  footnote. A parity run that improves bytes but loses > 5 % decode throughput is a
  **Pareto regression**, and the correct outcome is neither "parity is better" nor
  "parity is worse" but **"the shipped 256 KiB default is buying decode parallelism
  with bytes, and the trade must now be priced explicitly."**

### 13.6.3 What must be re-issued if the material-change threshold trips

Frozen now, so no one decides this after seeing numbers:

1. **Relabel, do not retract.** Every Class A row acquires
   `CLASS-A-BLOCK-CONFOUNDED (ANVIL@256KiB vs Brotli@4MiB-whole-file)`.
   Class B rows acquire `CLASS-B-AT-PARITY`.
2. **The 0 FRONT-CROSSING tuple is withdrawn and re-derived** from a parity grid.
   It may come back identical; it may not. **Its withdrawal is automatic and is not
   a claim that ANVIL crosses.**
3. **The canonical bar is restated** as the *parity* grid, not the Class A grid.
   Every "beats Brotli" / "loses to xz" statement must name which grid it uses.
4. **Backend-2-vs-backend-1 (BWT) conclusions are quarantined** until re-measured at
   parity — they are the most badly confounded row set (#4 above).
5. **`tools/bench_native.cpp` must pin `o.block_size` explicitly and emit it as a
   CSV field.** One-line fix, so the confound cannot recur silently. *Not made here
   — no production edits.*

## 13.7 Frozen INVALID rules for the closeout census

1. Any `INVALID-ISOLATION` per §13.5 ⇒ no decomposition claim admissible; byte totals
   retained, mechanism attribution void.
2. `alpha_mode6(B)/alpha_rans(B) > 0.25` at any B ⇒ mode 6 is not a valid zero-warmup
   reference; **the null arm fails and no substitute may be improvised** — a new null
   arm requires a new frozen preregistration, not an in-run substitution.
3. `bytes(A3) ≠ bytes(A1)` for any file ⇒ the `--parse=ratio` / explicit-`--block`
   interaction is unaccounted; arm design invalid.
4. Control robust CV over the frozen bound ⇒ `BLOCKED_AMBIENT`, per
   `docs/github-actions-benchmark-protocol.md`.
5. Any cross-run splicing with the frozen Class A/B grid ⇒ inadmissible. The census
   numbers belong to their own measurement window.

---

# §12. Provenance statement

Written under `docs/swarm-2026-10-02/MASTER-BRIEF.md`. The evidence map (§3) and
the root-cause finding (§0) were reconstructed from `src/anvil.cpp`, `FORMAT.md`,
`tools/bench_native.cpp`, `tools/reference_cost_gate.py` and
`docs/audit-2026-09-07/13-e4-block-routing-oracle.md` **before** reading
`15-crossblock-memory-space-bunny.md`; §0, §3 and §13 are not present in that report.

**Correction recorded against my own §9 design (§13.1).** The §9 `D_cross` statistic is **sound as a generic-fragmentation control** — it removes the part of ANVIL's deficit that a generic codec also suffers, which is exactly what the free-null question needs — but it **does not separate model warmup from phrase-window loss**. `D_anvil` and `D_brotli` each conflate both components; subtracting two equally-conflated quantities cancels neither. The structural reason: in every existing LZ codec the window and the entropy model live in the same stream state, so "cut the stream" resets both, and **strict isolation is impossible from byte totals alone**. §13.2 supplies the missing zero-warmup null arm — **ANVIL block mode 6** (`default-with-sparse-exceptions`, non-adaptive by construction, already in the shipped suite) contrasted against the adaptive rANS / shape / hot-op / TCOPY modes at matched block sizes, plus free token-level window accounting on the unfragmented A0 stream — and §13.5 freezes the three-way agreement test that decides whether the isolation succeeded, with `INVALID-ISOLATION` as the failure outcome.

Source anchors verified directly: `src/anvil.cpp:3769` (default 256 KiB), `:3866` (`BrotliEncoderCompress(11, 30, ...)` — ANVIL pins q11/lgwin 30), `:3877` (`LARGE_WINDOW`), `:3889` and `:4907-4909` (whole-output residency, ~2x parallel-path peak), `:4908` (`nthreads = min(decode_threads, segs.size())`), `:4965` (`--block=` parsing), `:4999` (`--parse=ratio` implies 128 MiB); `tools/bench_native.cpp:25` (`o.block_size` never assigned, `parse` never `"ratio"`), `:39` (`BROTLI_DEFAULT_WINDOW` implies lgwin 22, whole file); `tools/reference_cost_gate.py:136` and `tools/bench_ratio.py:124-182` (`--parse=ratio` implies Class B at parity).

§13.6 records that **parity is not a free win**: because `nthreads = min(decode_threads, segs.size())`, the shipped 256 KiB default yields **809** independently-parallel decode units on Silesia while the 128 MiB parity configuration yields **1**, so parity may improve bytes while regressing decode throughput — which is why §9.3 condition 3 (`P_loss >= 0.95`, `R_delta <= 1.05`) is a **hard gate on the parity result itself**, not a footnote. And §13.6.1 flags the most severely confounded Class-A conclusion: the **backend-2 (BWT) versus backend-1** comparison, because a BWT block re-runs `libsais` per block and is therefore penalized far harder by fragmentation than a compact rANS block, making that A/B **biased against backend 2 at 256 KiB**.

No commit, push, reset, clean, stash, restore, or rebase was performed. No
existing file was modified. No local compression benchmark, sweep, statistical
experiment, or fuzz campaign was run. No prototype was created. Every prior-art
claim I could not verify in this session is marked **[U]** and not asserted as
fact; every derived number is marked **[A]** with inputs shown so it can be
recomputed independently. The constructive lane's prior-art conclusion is adopted
on its own conservatism, not on my verification of its citations.