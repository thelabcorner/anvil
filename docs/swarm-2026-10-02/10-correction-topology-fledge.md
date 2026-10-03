# Track 10 — Correction Topology Coding: INDEPENDENT ADVERSARIAL AUDIT (Fledge Alpha)

**Agent:** Fledge Alpha Free (independent adversarial reviewer / validator)
**Lane:** `10-correction-topology`
**Date:** 2026-10-02
**Worktree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty — **unmodified by this audit**
**Status of this document:** REVISION 2 (reconciled). Rev 1 = interim, written before any
constructive-lane report existed. Rev 2 (this revision) reads
`10-correction-topology-space-bunny.md`, audits two coordinator-flagged correctness defects against
`src/anvil.cpp`, independently re-derives the Step-0 economics, and fills the reconciliation table
(§10). §11 carries the revised recommendation.

**Standing of this audit.** I did **not** read, and do not rely on, any Space Bunny report. At the
time of writing, `docs/swarm-2026-10-02/` contained only `MASTER-BRIEF.md`. Every number below is
either (a) re-derived from a named artifact in this repository that I read directly, or (b) explicitly
labelled ASSUMPTION / PROJECTION. I ran **no** corpus benchmark, **no** sweep, **no** fuzz campaign.
I created exactly one file (this one) and read RFC 7932 over the network to verify my single most
load-bearing prior-art claim. Nothing was committed, reset, cleaned, stashed, restored, or rebased.

---

## 1. Bottom line first

**Recommendation: KILL** the correction-topology family as constituted (per-`(k,slot)` modal
residual + exception mask), on **three independent prongs**, any one of which is sufficient:

1. **Arithmetic ceiling (math-class, decisive).** The mechanism wins iff modal accuracy
   `p > h_e / H̄(r)`. The project's own measurement puts `p = 0.17–0.23` (`RESEARCH_LEDGER.md:606-611`),
   which caps the end-to-end ceiling at roughly **+2% to +5.5%** on the very corpus the mechanism
   targets — *below* the 7–9% gap to `brotli q9` on that corpus, and therefore incapable of producing
   a frontier advance even at theoretical best. Derivation and numbers in §4.
2. **The reopen condition has already been consumed — DEMOTED in rev 2 to supporting.** The
   constructive lane is right that mode 13 *augmented* rather than *replaced* the flat-A mask
   (`src/anvil.cpp:3414-3430`), so those three losses are augmentation measurements rather than
   topology-code measurements. I accept the correction and demote this prong, because prong 1 needs
   no experiment at all. Original statement: the recorded blocker was "blocked on
   structural-distance propagation (R4) / SRR" with a standing promise to retest. That promise was
   kept and spent: the SRR probe (`RESEARCH_LEDGER.md:1525-1661`) and its deliberately-favourable
   synthetic retest (`RESEARCH_LEDGER.md:2233-2324`) both landed, and topology coding lost again,
   by the same margin, on purpose-built data engineered to make it win. `docs/audit-2026-09-07/06-do-not-reburn.md`
   items **B1/B2** are therefore satisfied-as-spent, not merely asserted.
3. **Decode-leg direction is backwards.** The binding project constraint is decode throughput
   (MASTER-BRIEF item 12; FLAG-A), and this mechanism *adds* a ninth entropy stream whose symbols are
   overwhelmingly the most frequent symbol (exceptions) — it spends decode cycles where a decoder
   spends the most. On the target file, mode 11 already sits at ~215 MB/s decode vs `brotli q9`
   ~900 MB/s (`RESEARCH_LEDGER.md:136-150`); a 9th stream cannot close a 4× gap.

**The single most important finding of this audit is diagnostic, not negative:** the recorded verdict
is **not attributable to the topology mechanism**. Mode 13 is *"Same token/dist coding as mode 12"*
plus topology — the code says so itself (`src/anvil.cpp:3363`), and mode 12 is a different distance
coder. The published A/B was run against **mode 11**, not mode 12. So a three-way delta was booked
against a one-way hypothesis, and the ledger's stated root cause ("the corpus lacks exploitable
structure", "blocked on R4") is **wrong**. The real root cause is arithmetic (§4) plus implementation
cost (§6). Correcting this is worth more to the project than any further pilot.

**REV-2 SCOPE CORRECTION (important, and against my own rev-1 framing).** Read against the
constructive report, the KILL above is **narrower than "the correction-topology neighbourhood"**. It
kills a *transmitted modal-default table + exception bitmap*, not every way of coding correction
position. Two things are explicitly **outside** it: (a) mode 11 / SPARSE-REF, and (b) **WCT**, which
re-grids the *position* code and predicts no residual values, so §4's inequality does not touch it.
WCT gets its own verdict — **HOLD, conditional PILOT, re-scoped Step 0** — in §11.2.

---

## 2. Evidence base actually read (provenance)

| Artifact | Locator | Date | Used for |
|---|---|---|---|
| `MASTER-BRIEF.md` | `docs/swarm-2026-10-02/` | 2026-10-02 | doctrine, track definition, deliverable contract |
| `RESEARCH_LEDGER.md` | Part II/III/IV/X/XIII/XIV/XV | 2026-08-21 → 2026-09-24 | Experiment E, I, J, N, T, W, Z; C6–C9; A18–A21 |
| `src/anvil.cpp` | L3362-3543, L2187-2254, L1055-1124, L449-453, L572-710, L4672-4691 | live tree | mechanism reconstruction, decoder audit |
| `docs/audit-2026-09-07/06-do-not-reburn.md` | B1, B2, B3, B4, A2, A3 | 2026-09-07 | prior-closure register |
| `docs/CONTEXT.md` | §"Linux continuation (v2)", §Highest-priority mechanism | prior | Linux-line claims (86.5% modal accuracy) |
| `docs/research-agenda.md` | §1, R1-R3, I2-4 | prior | pre-registered R2 contract |
| RFC 7932 | fetched in-session | live | Brotli context-map / block-type subsumption (§5) |
| `tests/benchmark-suite.csv`, `tests/benchmark-summary.csv` | mtime **2026-08-21 15:29** | stale | provenance audit only (§7) |
| `tests/corpus/` (23 files) | mtimes 2026-05 → 2026-08-21 | — | corpus/selection audit (§7) |

---

## 3. The mechanism, reconstructed from source (not from any report)

**Mode 11 — SPARSE-REF (7 streams).** `encode_tokens_sparse`, `src/anvil.cpp:2202-2239`. Token types
`0` literal / `1` exact match / `2` sparse-corrected match. For a type-2 token of length `L` with `k`
corrections:

* **mask**: `4·⌈L/32⌉` raw bytes → `L/8` bytes → **1 bit per byte of phrase covered**, *independent of k*;
* **residual**: `k` raw bytes through `encode_stream` (order-0 static rANS or raw).

**Mode 13 — TOPOLOGY (9 streams + modal table).** `encode_tokens_topology`, `src/anvil.cpp:3372-3443`.
Identical token stream and mask stream, plus:

* a **modal table**: `(k, slot) → most-frequent residual value`, transmitted once per block, 3 B/entry
  (`L3394-3395`), capped at 4096 entries by the decoder (`L3459`);
* an **exception bitmap**: `⌈k/8⌉` bytes per type-2 token, bit `j` = "correction `j` deviates from
  the modal default" (`L3416-3422`);
* **residuals coded only for exception bits** (`L3420`).

**Decoder state/cost (`decode_tokens_topology`, `L3445-3543`).** Persistent per-block state:
`modal` + `has_modal` tables, `65×64` entries each ≈ **8.3 KB fixed**; `last[2·kShapeClasses]` per-shape
displacement state; 9 fully-materialized substreams. Per type-2 token the decoder does a memcpy, then
per correction: one `countr_zero` loop iteration, one exception-bit test, and either a 1-byte stream
read or a table lookup. **This is `memcpy + N sparse stores + N bit-tests + 1 extra stream init`
— i.e. it is strictly *more* decoder work per byte than mode 11, for a ratio-only claim.**

**Parser emission (`L1066`, `L1084`, `L1101`, `L1115`).** `bk ≥ 1` is required for a sparse candidate
(`L642`, `L709`), so type-2 tokens always carry `k ≥ 1`; type-1 (exact) is a separate branch. Recorded
fact, and it matters: *the "+2.2% repeat-control overhead" attributed in the ledger to "flat-A mask cost
when corrections are zero" cannot be an `off.empty()` bug — there is no zero-correction type-2 token.
The real pathology is **mask granularity**: a token with `k=1` over a 235-byte record still pays
`4·⌈235/32⌉ = 32 B` of mask to save one residual byte.* (§8, Alternative A.)

---

## 4. STRONGEST KILL ARGUMENT — the break-even arithmetic

This argument uses **only** numbers the project has already measured. It needs no new experiment.

### 4.1 The inequality

Per type-2 token, shared mask cost cancels. Let `p` = modal accuracy, `h_e` = entropy of the exception
bitmap in bits/flag, `H̄` = entropy of a residual byte in bits.

```
gain(mode 13 − mode 11) per correction  =  p·H̄  −  h_e
mode 13 wins  ⟺  p > h_e / H̄   := p*
```

with `h_e = H₂(1−p)` because `encode_stream` entropy-codes the exception bitmap (so a skewed bitmap is
cheaper than 1 bit/flag — mode 13's *best* case).

### 4.2 Plugging the project's own measured `p`

`p = 0.20` → `h_e = H₂(0.80) = 0.722` bits. `p = 0.23` → `0.778` bits. Margin `= p·H̄ − h_e`:

| `H̄` (residual byte entropy) | `p*` @ p=0.20 | margin/corr | max saving/corr |
|---:|---:|---:|---:|
| 8.0 bits (near-uniform) | 0.090 | 0.878 bits | 0.110 B |
| 6.0 bits | 0.120 | 0.478 bits | 0.060 B |
| 4.5 bits (text field values) | 0.160 | 0.178 bits | 0.022 B |

**The uncertainty that decides the case is `H̄`, and `H̄` is nowhere measured in this repository.**
The target domain is record-structured *text* (JSON-lines, logs), where corrected bytes are field
values — digits, short ASCII runs, timestamps — so the honest prior for `H̄` is the **low** end
(4–6 bits), which is exactly the regime where the mechanism dies.

### 4.3 Turning the margin into an end-to-end ceiling

Let `R` = residual-stream bytes in the winning parse and `N` = correction count, so `R = N·H̄/8`.
Measured reference: mode 11 on `generated.jsonl` = **222,409 B** total output
(`RESEARCH_LEDGER.md:598`); input 2,815,267 B. Hence `N ≤ 8·(222,409 − R)/H̄`.

```
max end-to-end saving  =  N · (p·H̄ − h_e)/8  ≤  (222,409 − R)/H̄ · (p·H̄ − h_e)/8
```

Taking a **generous** `R = 50% of output` (the residual stream dominating the output is itself
implausible for a 0.079-ratio result, so this understates the cap):

| `H̄` | `N` | max saving | as % of 222,409 B |
|---:|---:|---:|---:|
| 8.0 bits | 111,205 | 12,199 B | **5.5%** |
| 6.0 bits | 148,273 | 8,867 B | **4.0%** |
| 4.5 bits | 197,698 | 4,409 B | **2.0%** |

**Ceiling = 2–5.5% at measured `p`, in the most favourable arithmetic regime, before subtracting the
ninth stream's fixed cost or the fragmentation loss.** Compare: `brotli q9` is 0.073 vs mode 11's
0.079 on this file (≈7.6% gap, `docs/CONTEXT.md` §SPARSE-REF anchor). **A mechanism whose ceiling is
below the gap it must close cannot produce EXTENDS_FRONT, and this project's doctrine explicitly
refuses ratio-only wins.** That is a math-class kill requiring no benchmark.

### 4.4 What would rescue it — and it is not a rescue

Only `p ≈ 0.865` (the Linux-line claim, `docs/CONTEXT.md`) makes this large: `h_e = H₂(0.135) = 0.567`,
margin `(0.865·H̄ − 0.567)/8` → 0.66 / 0.51 / 0.36 B per correction at `H̄` = 8/6/4.5, i.e. ceilings of
**37% / 28% / 20%** of output. **So the entire family's value rests on one unverified Linux-line
statistic (`p = 0.865`, `docs/CONTEXT.md`, "86.5% modal accuracy across 598 (mask,slot) contexts on
logs") that the Windows line has never reproduced and that the Windows line's own measurement
contradicts by a factor of ~4** (`p = 0.17–0.23`, `RESEARCH_LEDGER.md:606-611`). Note also that
`docs/CONTEXT.md` labels that 86.5% as Linux-line *directional*, and `RESEARCH_LEDGER.md:613-617`
records the project's own explanation of the discrepancy: 86.5% came from a *structural-channel parser
that aligned corrections to a record frame*; ANVIL's greedy parser lets positions drift. Experiment W
then showed that even purpose-built zero-jitter and jittered synthetic files do not fix it
(`RESEARCH_LEDGER.md:2296-2314`). **A ceiling that requires reproducing a statistic which three
independent attempts have failed to reproduce is not a ceiling; it is a wish.**

---

## 5. Prior-art map — subsumption is total

Verified in-session by fetching RFC 7932. Verbatim, §2:

> "The particular prefix code used can depend on two factors: the block type … and the context of the
> value. For the case of literals, the context is the previous two bytes in the uncompressed data; and
> in the case of distances, the context is the copy length from the same command. … the context is
> mapped to a context ID in the range 0..63 for literals and 0..3 for distances. The matrix of the
> prefix code indexes for each block type and context ID, **called the context map**, is encoded in a
> compact form in the meta-block header."

| Prior art | Mechanism it already owns | Subsumes mode 13's | Confidence |
|---|---|---|---|
| **Brotli RFC 7932 §7.1/§7.3 context map** (VERIFIED in-session) | decoder-visible map from an *already-decoded* context to one of ≤64 probability clusters, compactly coded in the header | **The whole idea.** `(k, slot)` is a context composed of already-decoded quantities (the mask precedes residuals on the wire). Brotli's version additionally uses the *entire* distribution, not one mode — strictly more information per bit, with **no exception bitmap and no extra stream** | **Verified quote** |
| **Context mixing** (Witten/Neal/Cleary 1995) | per-context *predictor* + arithmetic-coded "was the prediction right" flag | per-slot modal default + exception mask = literally a binary predictor and its match bit | High (knowledge; not fetched this session) |
| **WFC / SymConst / SCL / KISS** | domain + arc distribution with one dominant arc, exceptions explicitly coded | same structure, mature implementations, no novelty | High (knowledge) |
| **Canonical Huffman with escape** | 2-symbol code {default, escape} | "modal default + exception" is a degenerate Huffman code | High (knowledge) |
| **VCDIFF RFC 3284** | default window ("target window initially identical to source") + ADD/DELETE/COPY with address/segment tables | copy-with-corrections + a transmitted index/segment table | High (knowledge) |
| **bsdiff** | suffix-array `add` array + control stream of `(len, newbytes)` pairs | sparse-difference values with explicit exception/control coding | High (knowledge) |
| **Zdelta** | copy-with-differences, CREATE mode difference ops | the R1 lineage the ledger already names | High (knowledge) |
| **LZ77 rep distances** (LZMA, DEFLATE, Zstd `rep0-3`) | **reuse the previous token's parameter instead of transmitting it** | the strongest prior art for the "topology template" alternative (§8-C) | High (knowledge) |
| **PAQ-family match model (PFM)** / stateful predictors | predict the next match's bytes from the previous match's bytes+distance | "copy the correction structure, not just the bytes" | Medium (knowledge; verify before citing) |
| **Sparse-bit-vector coding** (Elias/Fano; Waldvogel 1995 sparse bit vectors; PForDelta Poisson coder) | index/bitmask-position coding proper | the mask stream (§8-A) | High (knowledge) |
| **Brotli §9 `NDIRECT`** | N literal bytes followed by N length codes | bitmap + exceptions, already standard | Verified (TOC §9 + §2 read) |
| **ELF/PE strip / Courgette** | relocation algebra as external machinery | the TCOPY neighbourhood | High (knowledge) |

**Verdict on novelty: ≈ 0.** The project's own Experiment J note
(`RESEARCH_LEDGER.md:630-639`) already recorded the governing principle — *"Context clustering is
EXPLICITLY NOT novel … any ANVIL claim leaning on decoder-visible context modeling must not be framed
as new."* Mode 13 is a strictly *weaker instance* of exactly that: it maps a two-symbol context to a
*single* default value instead of to a probability cluster, and pays an extra stream to do it. Under
MASTER-BRIEF doctrine item 1 (mechanism novelty, not novelty-by-difference), **this is adopt-class at
best and it is a worse instance of the adopted thing.**

*Caveat, stated plainly:* rows marked "knowledge" were **not** fetched in this session. Only RFC 7932
was verified. The coordinator must independently confirm the PAQ match-model row before any gate
ruling leans on it; the KILL does not depend on it.

---

## 6. Hidden-cost audit

| Cost | Where | Magnitude | Note |
|---|---|---|---|
| **Extra substream** | `L3437` — 9 vs 7 streams | +1 entropy model description + 1 varint length prefix **per block** | Amortizes over `opt.block_size`; measurable but second-order |
| **Modal table on the wire** | `L3390-3396` | 3 B/entry + varint count; ≤4096 entries → **≤12.3 KB/block** worst case | Bounded and cheap; not the problem |
| **Stream fragmentation** | residual split into `exc` + `resid` (`L3420-3422`) | Each fragment is shorter → **higher per-symbol model cost, less context, worse rANS rate** | This is the likely bulk of the unexplained measured loss |
| **Decoder extra state** | `L3454-3455` | **8.3 KB fixed** per block (`65×64×2` arrays) | Immaterial to RSS |
| **Decoder extra work** | `L3524-3538` | +1 bit-test + branch per correction, +1 stream init, +1 table (4 KB) | **Directionally wrong for the decode leg** |
| **Mask granularity** | `L2222-2229` | `L/8` bytes per token, *independent of k*; `k=1` over `L=235` costs **32 B of mask to encode 1 B** | **The largest and least-examined cost in the whole family.** See §8-A |
| **Malformed-input allocation bound** | `L3469-3477` | `max_sub = 16·out_len + 64` applied to **each of 9** streams → worst-case decode allocation **144·out_len** (mode 11: 112·; mode 10: 80·) | Pre-existing family defect that topology makes **28.6% worse**. Belongs to track 19; must be registered, not fixed here |
| **Cross-stream invariant fragility** | encoder `L3419` vs decoder `L3533` | For `k > 64` the encoder force-sets every exception bit (`t.off.size() <= 64` fails ⇒ `m` false), and the decoder's `pc > 64` guard then never fires. **Correct today, but correct only via an implicit cross-stream invariant** | I traced both sides: **no live bug.** Should become an explicit corruption-containment test case |

**Headline hidden cost:** the family's dominant decoder-visible charge is the **mask**, not the
residual topology. Mode 11 already pays `L/8` bytes per sparse token; mode 13 adds an exception stream
*on top of* a mask it never reduces. **Nothing in the family attacks the dominant cost.**

---

## 7. Benchmark-window provenance audit

1. **The summary CSVs are stale relative to the tree.** `tests/benchmark-suite.csv` and
   `benchmark-summary.csv` both carry mtime **2026-08-21 15:29**; `src/anvil.cpp` carries mtime
   **2026-09-23**. Every mode-11/13 byte count that traces back to those files describes a codec
   binary **33 days older than the one in the worktree**. Byte counts remain *ranking-grade directional*
   only; **no throughput claim from that window is citable.**
2. **The old harness demonstrably moves 3–6×.** `RESEARCH_LEDGER.md` A18/A20/A21: store-class rows
   moved 3.2–6.1× on *both* planes with byte-identical output, root-caused to the `crc32` slicing-by-8 /
   PCLMUL fast path (`4.5×` enc / `3.4×` dec, wire-identical). So an in-process median-3 from August is
   not evidence about September. The only bench-signed protocol is the frozen I9 grid
   (`docs/github-actions-benchmark-protocol.md`, A21 arm shas). **Any new topology throughput number
   must come from that protocol or it does not exist.**
3. **Selection on the test set.** `p = 0.17–0.23` was measured on `generated.log`, `generated.json`,
   `generated.jsonl` (`RESEARCH_LEDGER.md:606-611`) — the *same three files* the A/B verdict is read off.
   The 86.5% Linux figure came from `generated.log`; `generated.log` is also in the evaluation set.
   **The topology evidence is entirely in-sample.** Track 03 (`03-heldout-corpus`) exists precisely to
   fix this; no topology verdict should be re-adjudicated without it.
4. **Cross-run contamination in the ledger's own numbers.** Mode 12's headline (`log 0.0756`,
   `RESEARCH_LEDGER.md:1131`) and mode 13's (`log 0.1099 = 213,433 B`, `L596`) come from **different
   sessions**. Their cross-run ratio implies topology costs **+45%** on `generated.log` versus its true
   sibling — *not* the +19.3% the ledger reports. **Flagged as ASSUMPTION requiring same-run
   verification** (§9 Q0), but directionally it makes the verdict *worse*, never better.
5. **Frozen-frontier context.** Canonical tuple is **0 FRONT-CROSSING, 33 non-dominated, 5 FRONT-GAP,
   28 DEGENERATE, 435/468 dominated** (`RESEARCH_LEDGER.md:4711-4714`). Any topology pilot must be
   scored against that, in the FRONT-GAP/FRONT-CROSSING/DOMINATED vocabulary (item A3), never raw
   `EXTENDS_FRONT`.

---

## 8. What remains new and performance-plausible — the honest answer

**Mechanism-level: nothing in this namespace.** The arithmetic cap (§4) and the subsumption (§5) are
independent, and either alone closes it. I say this without hedging: **no interaction in
residual/correction-topology coding is both new and performance-plausible for this project.**

Three survivors survive at *lower* ambition levels, honestly labelled:

### A. Context-modelled **mask** coding — the cheapest admissible alternative (novelty ≈ 0, value plausible)
Replace the order-0 byte model on the `masks` stream with a bit-level model conditioned on
`(previous mask byte in the same token, bit index mod 8, L mod 32)`, or code the mask as a sparse
bit-vector (Elias/Fano/Waldvogel). **Same stream count, same decoder control flow, one table lookup per
32-bit word instead of per byte.** Rationale: the mask is the dominant decoder-visible cost
(`L/8` bytes/token), is currently modelled with *zero* structural knowledge, and is mostly zeros with
strong intra-word position structure — exactly what order-0 byte rANS cannot see. Honest ceiling, using
the same arithmetic as §4: mask raw cost `0.125 bit` per covered byte → a good bit-model plausibly
reaches `0.02–0.05 bit` per covered byte, i.e. **2–3% of output on `generated.jsonl`** — *also* below
the 7.6% gap. **So this is an engineering adopt at best, not a frontier route.** It is nonetheless the
highest value-per-cycle item I found in this namespace, because it is the only candidate that attacks
the dominant cost with zero added decode work. **Its ceiling is unknown because the mask stream has
never been measured in isolation** (§9, Q1).

### B. Single-stream inline flag (the *fair* version of mode 13) — a falsification tool, not a mechanism
Delete the `exc` substream and code a 1-bit "equals default" flag **inline in the existing residual
stream**. Removes the fragmentation penalty and the extra model entirely while preserving the modelling
idea. Prior art: still context-mapped coding (§5). Its only value is **diagnostic**: if it still loses
by >3%, the `(k,slot)` context is the problem and the family is dead *on the merits*; if it wins ≥3%,
the published +13.5–21% was a fragmentation artifact and the ledger's root-cause narrative must be
rewritten. This is the honest way to close the confound in §7.4.

### C. Distance-keyed **template reuse** (rep-style) — narrow-new, weak
Copy the previous sparse token's mask+residual template at the *same distance* instead of transmitting
it (`rep` semantics applied to the correction structure, zero bits when the template matches). Cost:
a tiny cache (≤8 entries × ≤256 B ≈ **≤2 KB RSS**), decoder work unchanged (memcpy + N stores).
Prior art is strong and *specifically* pointed: LZ77 rep distances already generalise "reuse the
previous token's parameter instead of transmitting it" (LZMA/DEFLATE/Zstd `rep0-3`), and PAQ match
models generalise it to match content. Novelty ≈ "rep applied to masks", i.e. novelty-by-difference,
which doctrine item 1 rejects. **Its ceiling is also bounded by the mask**: if correction *positions*
drift — which is the measured failure mode cited in `RESEARCH_LEDGER.md:613-617` and
`RESEARCH_LEDGER.md:1574-1588` — the template does not match and the bytes are unchanged.
**Verdict: do not build.** If anyone revives it, it must be A/B'd against an equal-strength control
that *also* uses rep distances for masks, or the rep trick itself will be booked as the novelty.

---

## 9. Decisive REMOTE-ONLY experiment (one experiment, pre-registered)

**Name:** `T10-MASK-ANATOMY` — *measure the quantity nobody has measured, before writing another line
of topology code.*

**Rationale.** Four quantities are simultaneously (a) unmeasured, (b) each individually capable of
closing or opening this whole namespace, and (c) obtainable from **one instrumented run** with no new
codec, no new mode, and no new corpus. This is the cheapest decisive experiment available and it is
purely diagnostic.

**Instrumentation (isolated; must live behind a flag, `src/anvil.cpp` untouched by default):**
in `encode_tokens_sparse` and `encode_tokens_topology`, after the existing per-token loop, emit
per-block and whole-file counters — no wire change, no new block mode:

| # | Quantity | Symbol |
|---|---|---|
| Q1a | covered bytes `Σ L_i` over type-2 tokens | `cover_bytes` |
| Q1b | `Σ 4·⌈L_i/32⌉` — raw mask bytes | `mask_raw` |
| Q1c | actual encoded `masks` stream size | `mask_wire` |
| Q1d | `Σ k_i` corrections | `n_corr` |
| Q1e | `H̄` — order-0 + order-1 entropy of the residual byte stream, and the top-16 residual values | `H_resid` |
| Q1f | actual encoded `resid` stream size | `resid_wire` |
| Q1g | `p` — measured modal accuracy, **(k,slot) and 4 alternative contexts** | `p_ctx` |
| Q1h | bits/correction by correction-index histogram (tests position drift directly) | `corr_by_j` |
| Q1i | mean/median `k_i`, and `mask_raw/n_corr` (the granularity tax) | `k_stats` |

**Files:** `tests/corpus/generated.{json,jsonl,log,sqlite,repeat.jsonl}` +
`tests/corpus/synth-{timeseries,jitter,columnar-align,drift-stride}.bin` + the 6 held-out PEs.
**Execution:** GitHub Actions only, under `docs/github-actions-benchmark-protocol.md`, per-rep process
invocation, pinned core, ≥5 reps interleaved, median + CV reported, byte counts exact (CV 0.000%).
Corpus locked and checksummed **before** the run.

**Pre-registered decision rules — fixed now, before any number is seen:**

* **KILL the family (default outcome expected) if ANY of:**
  * `H_resid ≤ 6.0 bits` **and** measured modal accuracy `≤ 0.25` in every context tested → the
    §4.1 inequality fails at the project's own measured operating point; record the arithmetic
    break-even `p* = h_e/H̄` as the closure citation.
  * `mask_wire < 10%` of total output on ≥3 of 4 record files → §8-A is dead too, and the sparse
    correction family has no un-attacked dominant cost left. **This alone closes the namespace.**
  * `mode13_instream_inline` (§8-B) loses to `mode12` by **> 3.0%** on **≥3 of 4** record files at the
    same run → the `(k,slot)` context is the defect; no structural alignment can rescue it.
* **PROMOTE to PILOT (a real reopen) only if ALL of:**
  * `mask_wire ≥ 15%` of total output on **≥3 of 4** record files, **and**
  * a bit-level mask model cuts `mask_wire` by **≥ 20%** on those files (this is the §8-A ceiling test
    measured rather than guessed), **and**
  * measured modal accuracy `≥ 0.45` under at least one context that is **decoder-derivable at zero
    cost** (e.g. `(k, j)` refined by `(L mod 32)`, or drift-compensated `j'`), **and**
  * the win holds on **held-out** families only (≥50% of the win must come from files not used to
    choose the context), **and**
  * decode regression ≤ **3%** (this is a decode-leg project; a ratio-only win is a rejection by
    doctrine item 12 and FLAG-A).
* **DECODE-LEG hard veto, independent of bytes:** any candidate whose decode MB/s regresses >3% on the
  record files is **rejected regardless of bytes**. A 2% byte win at −20% decode is a Pareto loss.

**Explicit falsification criteria for my own analysis above (I must be able to lose):**
1. If measured `H_resid ≥ 7.5 bits` **and** `p ≥ 0.30`, then my §4.3 ceiling is understated by ~2× and
   a 5–10% win becomes arithmetically possible — I would withdraw prong 1 and fall back to prongs
   2 and 3 (which stand independently).
2. If a same-run 3-way A/B shows `mode13 ≈ mode12` (within 2%), then the §7.4 confound is smaller than
   I claim and the published +13.5–21% is attributable to mode 12's distance coder, not topology — I
   would owe the ledger a *different* correction than the one I propose. **This is the single most
   likely way I am wrong.**
3. If `mask_wire ≥ 15%` and a bit-model wins ≥20%, my §8 "nothing survives" is wrong for the mask
   sub-namespace and §8-A should be promoted. **Update (rev 2):** §8-A has now been independently
   re-derived from the constructive side as **rep C / rep D of `docs/mask-stream-advisory.md:23-25`**
   (order-1 mask modelling), and §10.5 promotes it from "alternative" to **mandatory comparison arm**.
   See also §9.1: my `T10-MASK-ANATOMY` and the constructive `Step 0` measure the same quantities, so
**they must be run as one run, not two.**

### 9.1 CONSOLIDATION: do not run two measurement plans — run one

Rev 2 discovery: **my `T10-MASK-ANATOMY` (§9, Q1a–Q1i) and the constructive report's `Step 0` measure
the same quantities on the same corpus in the same arms.** Running them separately would burn two CI
slots and produce two series that cannot be compared (both would violate the splice discipline of
§7.1, and my §7.2 warns that the August harness numbers are stale anyway).

**Unified single run — the only measurement either lane needs:**

| Quantity | Mine | SB | Must record |
|---|:---:|:---:|---|
| nominal mask bytes (`Σ 4·⌈L/32⌉`, `Σ 4·⌈W/32⌉`) | Q1b | — | exact ints, per file, per mode |
| **actual** encoded mask bytes | Q1c | **Step 0 metric** | exact ints → gives **alpha** |
| topology share of complete compressed bytes | §9 KILL rule | **S0-KILL** | per file, post-entropy |
| corrections `N`, differing bytes `s`, mean `L` | Q1d, Q1i | Step 0 per-token table | distributions, not means only |
| residual-stream bytes and entropy `H` | Q1e, Q1f | — | **the §4 load-bearing unknown** |
| modal accuracy `p` over ≥4 candidate contexts | Q1g | — | incl. at least one zero-bit decoder-derivable context |
| corrections by index `j` (drift test) | Q1h | — | histogram |
| density `rho = d/W` distribution | Q1i | **S0-KILL-2** | median + tail |

**One line the constructive plan is missing and this table makes mandatory: `alpha` and `alpha'`
(nominal vs actual, per topology stream, per file).** Without it the run answers kill/continue but
**cannot adjudicate the 1.5% bar**, because the bar is in actual bytes (§10.3.4).

**Arms:** one checkout, one job, interleaved: `mode11`, `mode14`, and `mode11 + rep-C order-1 mask
model` (one encoder-side model change, no format change — §10.5 C6). Same files as SB Step 0.

**Decision rules, pre-registered and unchanged from §9 / SB §9:**
* **alpha < 0.30 on ≥3 record files** → **KILL WCT.** *My own prediction is the opposite* (alpha ≈ 0.4,
  topo_share 5–20%); if alpha collapses the track dies on my own arithmetic and I will say so.
* `topo_share < 3.0%` on ≥3 record files → **KILL** (SB's S0-KILL; I independently confirm this is a
  *conservative* threshold, §10.3.3).
* median `rho > 0.50` on ≥3 record files → **KILL** (SB's S0-KILL-2, dense-correction inversion).
* `H ≥ 6.0 bits` **and** `p ≤ 0.25` in every context → **KILL the modal-default family**, citing the
  §4.1 break-even `p* = h_e/H`. **This single condition closes the whole of track 10's original
  charter**, and it is the one number nobody has ever measured.
* Otherwise → S0-PASS = *removal of the cheap excuse only*; proceed to the four-arm Step 1, in which
  `B` (rep C) decides WCT by substitution (§10.5).

---

## 10. RECONCILIATION WITH THE CONSTRUCTIVE LANE (rev 2)

I read `10-correction-topology-space-bunny.md` (WCT, 372 lines). It is a substantially better piece of
work than the mode-13 line it descends from: it correctly identifies that mode 13 **augmented** the
flat-A mask rather than replacing it, it reads the source correctly, it discloses its own worst case,
and it gates on measurement. **Two of its load-bearing statements fail my audit; its central economic
projection is wrong by ~4.4x; and it never raises the objection that most likely decides its own fate.**

First, an honesty correction against myself: **WCT is not the mechanism my §4 arithmetic kills.**
§4 is about modal prediction of residual *values*; WCT predicts no residual values at all — it re-grids
the *position* code. I will not claim my KILL covers it. Equally, **WCT does not escape my §5
subsumption for free**: the *coupling* argument (mutual exclusivity of transform and residual windows)
is a representation fact I verified in source, and it is genuinely not a context map. That part of SB's
novelty argument is correct and I concede it.

### 10.1 Coordinator issue (1) — 3 bits cannot encode 15 subsets. **CONFIRMED; no such invariant exists.**

**Verdict: lossy wire spec; round-trip correctness fails as written (doctrine 4).**

The encoder emits **one independent correction per differing byte**, with no run structure and no
per-window cap (`src/anvil.cpp:605-608`):

```cpp
if (k >= kSparseScanMax) break;
off[k] = j; val[k] = tgt[j];
score -= litcost[tgt[j]] + 0.125;
++k;
```

The transform-window branch `continue`s (`src/anvil.cpp:594-603`), so it never emits a residual — that
part of SB's mutual-exclusivity claim is **confirmed in source**. But for a RESID window the differing
byte positions form an **arbitrary subset of {0,1,2,3}**, so all **15 nonempty** values are reachable.
Reachable counterexample to any 8-value family: bytes at `j=4` and `j=7` differ while `j=5,6` match,
i.e. subset `{0,3}` = `0b1001`. That is a *non-prefix, non-adjacent* subset, excluded by every natural
8-symbol encoding (prefix runs; size<=2 for a fixed pair; etc.). **There is no unstated invariant in
`src/anvil.cpp` that restricts it.** SB's T7b as specified cannot encode 7 of 15 reachable cases.

**Fixes, in preference order — note fix 2 makes the mechanism cheaper and arguably more novel:**

| Fix | Wire | Cost/window | Note |
|---|---|---|---|
| **1** | 4 bits/submask (16 values, 1 unused) | `(d + 4r)/8` | Trivial; this is what the economics must assume |
| **2 (recommended)** | one **joint** prefix code over `{CLEAN} ∪ {XFORM} ∪ {15 RESID subsets}` = 17 symbols | `H(distribution) <= 4` bits | Strictly cheaper when subsets are localised; **dissolves the arbitrary T7a/T7b split**, strengthening the coupling claim |
| 3 | restrict the parser to emit prefix runs | 3 bits | **Rejected** — changes the parse, which §3.3 promises not to do |

**Economic impact, on SB's own worked example (`L=235`, `d=r=6`):** `7.34 + (6+18)/8 = 10.34 B/token`
becomes `7.34 + (6+24)/8 = 11.09 B/token`; projected saving `26.38 -> 25.63 B/token` = **−2.8%**.
**Correctness-critical, economically minor. Does not threaten the 1.5% bar.**

*Process signal:* SB §10 case 1 is where this should have surfaced — it contains a visible
self-correcting ramble ("...and **loses** at 4 differing bytes in a 32-byte-granular comparison? No:
mode 11 pays 1 bit/byte = 4 bits/window too."). The submask alphabet was never enumerated.

### 10.2 Coordinator issue (2) — patch-before-copy. **CONFIRMED; it is an out-of-bounds write, not merely a lost correction.**

The established decoder loop shape is **copy, then patch into an already-materialised destination**
(`src/anvil.cpp:2287-2295`):

```cpp
size_t start=out.size();                                          // destination captured BEFORE copy
for(uint64_t k=0;k<len;++k) out.push_back(out[out.size()-dist]);  // COPY FIRST (periodic when dist<len)
for(uint64_t w=0;w<nwords;++w){ ... out[start+...+b]=s[6][ip_res++]; }   // THEN patch
```

SB's WCT pseudocode (`10-correction-topology-space-bunny.md:138-147`) runs the per-window patch loop
**before** `copy(len, dist)`. Two consequences:

1. **Out-of-bounds write / UB.** At patch time `out.size() == pos` (the copy has not run), so
   `out[pos + 4w + b]` indexes **at or beyond `size()`** on a `std::vector<uint8_t>`, and
   `operator[]` does not bounds-check. That is heap corruption, not a wrong answer. The coordinator's
   "pre-extension access" is exactly right.
2. **Silent loss *and* propagation even if (1) were patched.** The subsequent `copy` overwrites every
   patched byte; and because mode 11's copy is **periodic** when `dist < len`
   (`out.push_back(out[out.size()-dist])`, `src/anvil.cpp:2288`), any pre-copy patch inside
   `[pos, pos+len)` is re-read as *source* by later iterations and re-emitted — corrupting the whole
   phrase, not just the correction.

**Fix:** move `copy(len, dist)` above the patch loop and index via `start = out.size()` captured before
the copy, exactly as `src/anvil.cpp:2287`. The **read** side of the XFORM case (`out[pos-dist+4w]`) *is*
safe, since `4w+4 <= dist` places the source strictly before the destination. **Verdict: trivial to
fix, zero economic impact** (identical op sequence and byte counts) — but it shows §3.4 was never
executed against the existing loop shape, which is why §4.2's hand-derived cycle estimate carries no
evidential weight.

### 10.3 Step-0 economics, independently verified — **the projection is nominal-vs-actual. CONFIRMED DEFECT.**

SB §4.1 computes both arms in **nominal** bytes (today `5L/32`, WCT `L/32 + (d+3r)/8`), then its prose
sensitivity paragraph reasons about the **actual** stream ("if the real mask stream is only 6% of total
bytes, a 71% cut yields ~4%"). **Two different quantities are mixed.** This is exactly the
coordinator's suspicion, and it is real.

**Mechanism of the error.** The baseline mask does **not** cost `L/8` bytes on the wire. `masks` is one
of the streams passed to `encode_stream` (`src/anvil.cpp:2233` mode 11; `:3438` mode 13), which selects
per stream among raw / order-0 static rANS / Huffman. A sparse correction mask is a run of
`0x00000000` words with rare single-bit words, so order-0 byte coding already removes a large fraction
of the nominal. Define

> **alpha = actual_encoded_topology_bytes / nominal_topology_bytes** (topology-coder compression factor)

Then, with **alpha' approximately alpha** for WCT's T6/T7a/T7b (same coder family, similarly skewed
streams — **ASSUMPTION**, and Step 0 must measure both rather than assume equality):

```
today_actual = alpha  * 5L/32
WCT_actual   = alpha' * (L/32 + (d + 4r)/8)
gain%        ~= topo_share_actual * [ 1 - (alpha'/alpha) * (L/32 + (d + 4r)/8)/(5L/32) ]
```

At `L=235, d=r=6`, `alpha'/alpha = 1` gives bracket = `1 - 11.09/36.72` = **0.698**.

**Consequences, in order of importance:**

1. **SB's §4.1 "26 B/token" is nominal and must be relabelled `alpha * 26 B/token`.** Worked
   independently at a plausible `alpha ≈ 0.4`: mode-11 mask `235 * 0.4 = 94` bits; WCT dirty
   `59 * 0.4 = 24` bits + class `~ 5` + submask `~ 18` gives **actual saving ≈ 47 bits/token ≈ 5.9
   B/token, not 26 B/token — the projection is optimistic by ≈ 4.4x.**
2. **The correct prediction rule is `gain% ≈ 0.70 * topo_share_actual`.** SB's prose sensitivity
   (6%->4%, 2%->1.4%) already implies bracket ≈ 0.67–0.70, so **the prose conclusion is right and the
   table is the defect.** Good news — but the table is what a reader would quote.
3. **The 1.5% bar does NOT move, and must not.** `topo_share = 3.0%` gives predicted gain `≈ 2.1%`, i.e.
   **1.4x above the H1 bar.** The S0-KILL threshold of 3.0% is correct and marginally *conservative*.
   Nothing has been measured yet, so moving either number now would be a doctrine-5 violation in the
   opposite direction. **What must change is the derivation the bar is checked against, not the bar.**
4. **Step 0's metric is correct in kind but INCOMPLETE — my single most important correction.**
   `topo_share` alone yields a kill/continue signal but **cannot check the 1.5% bar**, because the bar
   is in actual bytes and the projection needs alpha. Step 0 must additionally record **nominal and
   actual bytes per topology stream per file** (hence alpha and alpha'). One line of metric, and it is
   *load-bearing* for the 1.5% verdict.

**A falsifiable prediction for Step 0 (mine, not SB's):** by the Massey bound, a length-`L` mask with `k`
ones carries at least `log2 C(L,k) ≈ k·log2(L/k)` bits — for `L=235, k=12` that is **≈ 50 bits ≈ 4.2
bits/correction**, *comparable to the residual bytes' own entropy* on text (≈ 3–4 bits/byte). The
topology budget on record files is therefore plausibly **5–20% of output, not the 1–3% SB's prose
contemplates. Prediction: S0-KILL will probably NOT fire; expect S0-PASS.** The coordinator must
therefore be told explicitly: **a Step-0 PASS removes the cheap excuse, it is not evidence for WCT.**
Falsifier: measured `topo_share < 3%` on >=3 record files — then I am wrong and the track closes as SB
specifies.

### 10.4 Per-claim reconciliation table

| SB claim | My position | Agree? | Settling evidence |
|---|---|---|---|
| §0/§3.5: mode 13 **augmented** the flat-A mask, so all three falsifications are augmentation tests, not topology-code tests | **Confirmed, and important.** It materially weakens my §1 prong-2, which I downgrade to "supporting" | **Agree** | `src/anvil.cpp:3414-3430` (flat mask written verbatim, then `exc` + modal table added) |
| §3.1: transform and residual windows are **mutually exclusive by construction** | **Confirmed in source** — verified `continue` at `src/anvil.cpp:597-603` | **Agree** | source read |
| §3.1/§6: therefore this is *not* merely "context modeling" but a representation fact about a coupled alphabet | **Conceded.** RFC 7932 §7 subsumes context-to-cluster selection, not a mutual-exclusivity coupling | **Agree** | source + RFC 7932 §2 quote, §5 above |
| §3.2: **T7b = 3 bits** for the differing-byte subset of a window | **REFUTED — 15 reachable subsets, no invariant** (§10.1) | **Disagree** | `src/anvil.cpp:605-608` |
| §3.4: decoder patches before copying | **REFUTED — out-of-bounds write + periodic-source propagation** (§10.2) | **Disagree** | `src/anvil.cpp:2287-2295` |
| §4.1: WCT saves **≈ 0.11·L B/token**, "≈26 B/token" at `L=235` | **Nominal, not actual. Actual ≈ 5.9 B/token at alpha≈0.4; projection optimistic ≈4.4x** (§10.3) | **Disagree — correct the label, keep the sign** | `encode_stream`, `src/anvil.cpp:2233/3438` |
| §4.1 prose: "6%->~4%, 2%->~1.4%, 1%->fails" | **Consistent with my bracket 0.698** — this is the *correct* form of the prediction | **Agree** | derivation, §10.3 |
| §9: S0-KILL at `topo_share < 3.0%` | **Correct and marginally conservative** (-> ≈2.1% predicted gain vs 1.5% bar) | **Agree** | §10.3 item 3 |
| §9: Step-0 metric = post-entropy actual bytes | **Correct in kind, INCOMPLETE: must also record alpha, alpha' (nominal vs actual per stream)** | **Agree, with required amendment** | §10.3 item 4 |
| H1 bar = **>=1.5%** bytes, decode >=0.97x | **Keep. Do not move.** Nothing measured yet; 3.0% topo_share predicts ≈2.1% | **Agree** | §10.3 item 3 |
| §7: WCT is **1.5x worse** than mode 11 when corrections are dense (rho>0.5) | **Confirmed and under-weighted.** Compounded by E22: the parser charges a hardcoded `0.125`/byte and cannot see the trade, so WCT-C's measured-cost feedback is load-bearing, not optional | **Agree — and I rank this above novelty as the top technical risk** | `src/anvil.cpp:582,607`; SB §7 |
| §4.2/§0: first correction-topology proposal that does **not** trade against the decode gap | **Overstated.** §4.2's own projection is "3–8% decode, possibly indistinguishable from noise", and the mask bytes are a small share of a copy-dominated loop (`src/anvil.cpp:2288`). "Does not worsen" is not "does not trade" | **Disagree (framing)** | §4.2 self-caveat |
| §6 risk 3: PORDER precedent may kill the novelty | **Agreed and, with my §5, promoted to the most likely rejection argument.** SB correctly routes it to track 20 | **Agree** | `RESEARCH_LEDGER.md:4824-4829`; my §5 |
| §10 case 8: most likely outcome is a measured-but-non-novel adopt-class result | **I go further: the likely outcome is *substitution*, not rejection** — §10.5 | **Agree, sharpened** | §10.5 |

### 10.5 The objection WCT does not raise about itself: it is a **transient** win, and its substitute is cheaper

**This is my strongest independent contribution to the track, and it is arithmetic, not taste.**

WCT's entire value is *nominal* bitmap waste: it emits `L/4` dirty bits per token where mode 11 emits
`L` mask bits. But the **information content** of that mask is only `≈ k·log2(L/k)` bits. Therefore:

* WCT's headroom is the gap between the *nominal* mask length (`L` bits) and its *entropy*
  (`≈ k·log2(L/k)` bits).
* **The project's own advisory already lists the cheaper way to collect exactly that gap.**
  `docs/mask-stream-advisory.md:23-25` names **rep C = "topology model (order-1 over masks)"** and
  **rep D = "per-record mask delta"**. SB cites these (E18) as *never implemented*.

Order-1 modelling of the mask captures the same `L -> k·log2(L/k)` compression **without** a format
change, a new block mode, a new coupled alphabet, a novel mutual-exclusivity argument, a
dense-correction worst case, or any parser change. It reuses the `masks` stream in place
(`src/anvil.cpp:2217-2229`), exactly like the infrastructure pattern this project has already accepted
twice (C7 J-selection; clustered-rANS K≈8-12 as "enabling infrastructure", `RESEARCH_LEDGER.md:634-649`).

**So WCT and rep C are substitutes, not complements — and the relationship is one-directional: the
better the project eventually codes masks (rep C/D), the less WCT has left.** WCT is transient by
construction; rep C is the durable fix. And because rep C's *gain* is bounded by the same
`k·log2(L/k)` quantity that bounds WCT's, **if rep C alone reaches >= WCT's gain, WCT is dead on
substitution grounds and the entire coupled-alphabet argument — the only novelty WCT claims — is never
needed.**

**Required amendment, pre-registered before any data:** Step 1 must carry **four** arms, not two:
`A` = current HEAD baseline; `B` = **rep C (order-1 mask model), no format change**; `C` = WCT-0;
`D` = WCT-C. **Decision rule:** `C`/`D` may advance only if `max(C,D) − B >= 0.3%` aggregate bytes. If
`B >= max(C,D)`, WCT is **KILLed on substitution**, independent of novelty, and the honest deliverable
is rep C as an adopt-class engineering result. That arm costs one encoder-side model change and no new
format — the cheapest possible test of the one question that decides the track.

This does not contradict my §8-A, which proposed precisely this as "Alternative A" before WCT existed.
It converges from the opposite direction, which is the point of an independent lane.

---

## 11. Final recommendation (rev 2)

Two verdicts, because these are two different mechanisms.

### 11.1 Correction-topology coding *as constituted* (transmitted modal-default + exception bitmap) — **KILL**

Unchanged from rev 1, with one prong downgraded by SB's §0 finding:

* **Prong 1 (arithmetic ceiling, math-class) — STANDING, and load-bearing.** Break-even `p > h_e/H`; at
  the project's own measured `p = 0.17–0.23` the ceiling is **2–5.5%** on the target corpus, below the
  7.6% gap to `brotli q9`. It depends on no prior falsification and cannot be reopened by R4/R5
  alignment.
* **Prong 2 (reopen condition spent) — WEAKENED.** SB correctly shows mode 13 *augmented* rather than
  *replaced* the flat-A mask, so the three recorded losses are augmentation measurements. Reduced from
  "dispositive" to "supporting"; superseded by Prong 1, which needs no experiment at all.
* **Prong 3 (decode-leg direction) — STANDING.** A ninth stream whose symbols are mostly the most
  frequent symbol spends cycles where a decoder spends the most; FLAG-A binds.
* **Prong 4 (confound) — STANDING.** The A/B ran against mode 11, not mode 12.
* **Prong 5 (subsumption) — STANDING.** RFC 7932 §7.1 context map, verified in-session.

Scope, stated so it cannot be over-read: this kills a **transmitted table of per-context modal residual
values plus an exception bitmap on a separate stream** — mode 13 as built and any re-skin of it. It does
**not** kill mode 11 / SPARSE-REF (separately ratio-validated, separately Pareto-dominated — untouched),
and it does **not** kill WCT (re-grids positions; predicts no residual values).

### 11.2 WCT (Window-Coupled Topology) — **HOLD, conditional PILOT, with a re-scoped Step 0**

Not KILL, because the mutual-exclusivity coupling is **verified in source**
(`src/anvil.cpp:597-603`) and is a genuine representation fact that RFC 7932 §7 does not subsume; the
mask is **provably a dominant, currently-uncoded cost** (`L/8` nominal bytes vs `≈ k·log2(L/k)` bits of
content — Massey bound); and the corrected economics `gain% ≈ 0.70 * topo_share_actual` put S0-KILL at
a **2.1% predicted gain against a 1.5% bar** — the gate is passed on paper.

Not PROMOTE-TO-REMOTE: there is **no WCT measurement of any kind**, and two live correctness defects
plus a 4.4x-optimistic projection mean the present document cannot support a build authorization.

**Conditions, all pre-registered, all fixable without new data — required before Step 0 dispatch:**

| # | Condition | Basis |
|---|---|---|
| **C1** | **Fix T7b**: 4-bit submask, or (preferred) joint 17-symbol prefix code over `{CLEAN, XFORM, 15 subsets}`. Restate §4.1's cost table with `4r`, not `3r` | §10.1 |
| **C2** | **Reorder the decoder pseudocode**: `copy` first, patch second, index via `start` captured pre-copy — matching `src/anvil.cpp:2287-2295` | §10.2 |
| **C3** | **Restate §4.1 in actual bytes** with alpha explicit; relabel "26 B/token" as `alpha * 26 B/token`; state the prediction rule as `gain% ≈ 0.70 * topo_share_actual` | §10.3 |
| **C4** | **Add alpha and alpha' to the Step-0 metric** (nominal *and* actual bytes per topology stream per file). Without alpha the 1.5% bar is not checkable | §10.3 item 4 |
| **C5** | **Do NOT move the 1.5% bar or the 3.0% S0-KILL gate.** Both are correct. Only the derivation changes | doctrine 5; §10.3 item 3 |
| **C6** | **Add rep C (order-1 mask model) as a mandatory arm**, with the `>=0.3%` substitution decision rule | §10.5 |
| **C7** | Record the **dense-correction risk as co-equal with novelty**, not subordinate — E22's hardcoded `0.125` makes WCT-C's measured-cost feedback load-bearing, not optional | §7 worst case |

**Warning to the coordinator (my own prediction, pre-registered):** by the Massey bound the topology
budget is plausibly **5–20%**, not 1–3% — so **expect S0-PASS**. **A Step-0 PASS is removal of the
cheap excuse, not evidence for WCT.** It must not escalate the track.

**The falsification that decides it, in one line:** if `B` (rep C, no format change) is `>= max(C,D)`
(WCT), WCT is KILLed on **substitution**, independent of novelty, and the deliverable becomes rep C as
an adopt-class engineering result — which is where I expect this track to end.

### 11.3 Ledger action I request (not performed by me — no existing file was modified)

Correct the root-cause attribution for mode 13 from *"blocked on R4 / corpus lacks exploitable
structure"* to: *"arithmetic ceiling at the measured modal accuracy (break-even `p > h_e/H`; measured
`p = 0.17–0.23` gives <=2–5.5% ceiling), plus stream-fragmentation implementation cost; mode 13
augmented rather than replaced the flat-A mask, and the published A/B ran against mode 11 instead of the
correct mode-12 control."* Leaving the current attribution in place is the one genuinely harmful
outcome of this audit going unreconciled, because it invites a future lane to spend a rotation on R4/R5
alignment for a family that cannot pay.

### 11.4 Verification of this audit's own claims (rev 2)

* **Measured facts:** §2 provenance table; §3 source line numbers; §5 RFC 7932 quote (fetched
  in-session); §6 source line numbers; §7.1-7.3, 7.5 (filesystem + ledger).
* **Independently re-derived this revision:** §10.1 (15-subset reachability + the 2.8% delta), §10.2
  (copy-then-patch loop shape from `src/anvil.cpp:2287-2295`), §10.3 (alpha formulation, the 0.698
  bracket, the 4.4x correction, the Massey-bound ~50-bit / 12-correction figure), §10.5 (substitution
  argument).
* **Derived-from-measured:** §4.1-4.3 arithmetic; §8-A ceiling; §10.3 prediction rule.
* **ASSUMPTION / requires verification:** §7.4 cross-run `mode12` vs `mode13`; `H`'s value (never
  measured — the load-bearing unknown for §4); §5 non-RFC rows; all `docs/CONTEXT.md` Linux-line figures
  (labelled directional there); **`alpha' ≈ alpha` in §10.3** — which is exactly why C4 requires
  measuring both; and the type-2/3 token count on the target files, also unmeasured.
* **I ran no benchmark, no sweep, no fuzz. I modified no existing file. No commit, no push, no
  reset/clean/stash/restore/rebase.** This revision read `src/anvil.cpp`, the ledger, the mask advisory,
  the gate rulings and the constructive report, and fetched RFC 7932.