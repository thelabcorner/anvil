# Swarm 2026-10-02 — NOVELTY SPACE SYNTHESIS (Track 20, Space Bunny Free)

**Author:** Space Bunny Free, track 20-priorart-killteam · **Date:** 2026-10-02
**Tree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty
**Companion detail:** `docs/swarm-2026-10-02/20-priorart-killteam-space-bunny.md` (full claim
charts, separator axes, remote prereg, thresholds). This file is the *20-track matrix*.

**Evidence labels:** `MEASURED` = in-repo recorded measurement (path given) · `RECORDED` =
in-repo recorded ruling/decision · `SESSION` = retrieved by me this session (excerpt-level
web search) · `GAP` = acknowledged coverage hole, **never** a clearance · `PROJECTION` = my
arithmetic, not measured.

**Method rule applied throughout:** classification is by **semantic equivalence of the
mechanism**, not by the project's name for it. Where a mechanism is described in ANVIL
vocabulary that differs from the prior art's vocabulary, I resolved the equivalence
explicitly in the "what it actually is" column.

**Hard scope limits observed:** no measurement by this agent; no existing file modified; no
commit/push/reset/clean/stash/restore/rebase; no local corpus benchmark; no heavy fuzzing;
no local prototype execution. This file and the track-20 companion are the only files created.

---

## 0. The result in one paragraph

**Of 20 tracks, exactly one mechanism survives as a novelty survivor: TCOPY/PNRA's exact,
geometry-implicit transform parameter — and even that is now conditional on an ablation that
the project's own audit predicts will fail.** Everything else is adopt-class, closed by prior
art, method-only (no novelty claim), or an unresolved search gap. **The single most important
new result of this pass is that Track 15's CAM is CLOSED BY PRIOR ART by a 2010 patent
application whose stated motivation is verbatim the same motivation** ("as the entropy slice
size is decreased, the resets of the context models occur more frequently"). CAM's engineering
value is untouched; its novelty claim is dead.

That is now the third time this project has reached the same structural conclusion from three
different directions (H2 ≡ BCJ + a position origin; H3/FLI-S3 ≡ ISLP; CAM ≡ CABAC entropy-
slice context initialization). **The pattern itself is the finding**: ANVIL's surviving
"novelty" is consistently a *placement/granularity/parameterization* shift inside a mature,
standardized mechanism, which is exactly the profile of obviousness rather than of novelty.

---

## 1. Classification legend

| Class | Meaning | Budget consequence |
|---|---|---|
| **NOVEL-SURVIVOR** | At least one claim separator identified that no located source anticipates, *and* the separator is not placement-only | may receive prototype/CI budget **with** a claim pre-registration |
| **ADOPT-CLASS** | Established technique; ANVIL may implement it as engineering with complete accounting and ablation | may receive prototype/CI budget **without** any novelty claim |
| **CLOSED-BY-PRIOR-ART** | A located source anticipates the mechanism itself (not merely its name) | no claim; may still be built as adopt-class if the economics justify it |
| **NO-NOVELTY-CLAIM** | Infrastructure, methodology, measurement, or verification machinery — claims nothing by construction | fund freely; never cite as novelty |
| **SEARCH-GAP** | Could not resolve with the coverage available; **the uncertainty itself is the finding** | budget a targeted pass before any spend, or declare adopt-class conservatively |

---

## 2. The 20-track novelty matrix

### Track 01 — G5D paged base+overlay exact dictionary → **ADOPT-CLASS**

| | |
|---|---|
| Semantically | paged/learned dictionary of exact byte phrases, decoder-visible page table, mandatory escape |
| Prior art | Parquet column-chunk dictionaries; Brotli static dictionary (RFC 7932) + `-D` raw dictionary; Zstd trained/external dictionaries; FSST; front/prefix coding; BPE; local/page dictionary layouts |
| Source | `RECORDED` — the mechanism's own prereg already says "**claims no primitive novelty**" and lists DataCortex, CLP, LogPrism, BtrBlocks, FastLanes, ALP, FSST, Parquet, Brotli as close lineage: `docs/I10-G5-PAGED-DICTIONARY-PREREG.md` §19 |
| Note | `SESSION` — Brotli ships `-D FILE, --dictionary=FILE` "use FILE as LZ77 dictionary; same dictionary MUST be used both for compression and decompression" (`c/tools/brotli.md`), confirming the external-dictionary form is a shipped CLI feature, not a proposal |

### Track 02 — G5A-informed bounded finalist planner → **ADOPT-CLASS**

| | |
|---|---|
| Semantically | cheap analytic feature scoring → bounded finalist set → few real backend encodes → emit winner |
| Prior art | OpenZL (offline/training-time plan search, resolved graph embedded in frame, one universal decoder); LogPrism; Grotli/DataCortex-style structure discovery + columnization + typed representation + backend arbitration; Brotli block splitting (`SESSION`, `c/enc/encode.c`, `block_splitter.go`) |
| Project evidence | `RECORDED` **G4 is `NO-GO-G4`** — cheap proxy surfaces missed the frozen q11 carrier choices; speed column withdrawn as `INVALID_SPEED_ACCOUNTING` (ledger PART XV) |
| Note | a *measurement-fidelity* question, not a novelty question |

### Track 03 — held-out corpus protocol → **NO-NOVELTY-CLAIM**

Methodology by construction. `docs/I10-CORPUS-LOCK-PROTOCOL.md`. Fund freely; cite never as
novelty. Note the corollary from §16.4 of the companion: **Brotli tuned a shipped constant on
Silesia**, which is ANVIL's frozen corpus — the lock protocol's value is now externally
justified, not merely internal discipline.

### Track 04 — dense Pareto measurement → **NO-NOVELTY-CLAIM**

Measurement machinery. `tools/pareto_front.py`, `docs/I10-DENSE-FRONTIER-PREREG.md`.
Harmonic input-byte aggregation is byte-exact and independently reproduced in ledger PART XV.
No novelty claim possible.

### Track 05 — backend frontier beyond Brotli → **ADOPT-CLASS** (with one unresolved corner)

ANS/rANS is settled infrastructure: `RECORDED` `docs/gate-priorart-audit-i8.md` §3 lists RFC
8478 (Zstd), LZFSE, Draco, CRAM, nvCOMP, DivANS, BCPack, JPEG XL, JPEG AI and rules "**no
headroom here for ANVIL; this is settled infrastructure.**" Any new entropy backend is
adopt-class by that ruling. The *search formulation* on top of a backend may still be a
survivor, and is folded into track 17's row.

### Track 06 — entropy backend co-design → **ADOPT-CLASS**

Same `RECORDED` basis as track 05. In-repo measured results already establish the interaction
without novelty: context-switched rANS is a ratio mechanism, not a throughput primitive;
per-cluster quantizer is reusable infrastructure. Adopting `RECORDED` verdict.

### Track 07 — BWT subblocking → **ADOPT-CLASS**

`SESSION`: `libsais` auxiliary-index/subblock APIs; BWT postcoder portfolio IDs 1/2/3
(`FORMAT.md`). `RECORDED`: `--bwt-subblock` sweep closed, default unchanged
(`docs/I10-AUX-UNBWT-RESULTS.md` §11). Partition-based BWT variants are a long-standing family.

### Track 08 — DEFLATE reconstruction (P4.1) → **CLOSED-BY-PRIOR-ART**

`RECORDED` ledger PART XIV A2, verbatim: "**NOVELTY: NO.** DEFLATE reconstruction is prior art
(precomp, preflate, preflate-rs, reflate, grittibanzli; container recompression). Recorded as
**adopt-class prior-art-deployed infrastructure**." The same ruling records that the portfolio
composition property is "a routing property, not a compression mechanism." Engineering value
(1,362,177 B recovered on `mozilla` prototype) is real and unaffected.

### Track 09 — exact transformed backreferences (TCOPY / PNRA) → **NOVEL-SURVIVOR (sole, conditional)**

**This is the only NOVEL-SURVIVOR in the matrix, and it is the only mechanism where the
project's own prior-art audit was executed to completion with its binding condition discharged.**

| | |
|---|---|
| Semantically | single-file self-referential copy of a prior phrase; an embedded 32-bit relocation/branch-immediate field is corrected by an amount **derived from the copy distance** (`Δ = −d`, **zero transmitted parameter bits**); residual identically zero |
| Separators that carry the claim | (i) **parameter provenance** — derived/zero-bit vs transmitted, where transmitted is prior art (`gate-priorart-audit-i8.md` §4: "the project has twice built the expeditious form and called it progress"); (ii) **exactness** — G4-amended requires zero residual, closing the whole statistical family (`gate-ruling-i9-pr5-position-derived.md` P-2, DNB-M2 measured 0.646–0.691 bits/doubling over d=4..1024) |
| Audit state | `RECORDED` `docs/priorart-tcopy-external.md` passes 1–6, including the **binding** full-text claims review of Intel US 7,111,148 / 7,010,665 / 7,617,382 (expired; CPU μop-storage compaction; no LZ layer; no match distance) which **discharged** the outstanding gap |
| **Material weakening** | companion §16.2: `lzma_options_bcj::start_offset` (`SESSION`, `tukaani.org/xz/liblzma-api/structlzma__options__bcj.html`) documents verbatim setting a region's position origin "so that the relative addresses ... use the same absolute addresses as in the first section." Combined with the BCJ kernel's `pos` state, H2's "geometry-derived Δ=−d" is **derivable as BCJ composed with a position argument.** The *explicit-delta-free* property survives; the *"novel geometric derivation"* framing does not |
| **Predictive negative** | `RECORDED` the A-vs-B ablation was already lost once: transmitted step **12,921 B < derived step 12,936 B** (`gate-ruling-i9-pr5-position-derived.md` P-7). The A1–A4 provenance ablation that decides this family **has never once been executed** (outstanding since I2-5; `gate-priorart-audit-i8.md` §6 action 5) |
| Verdict | **Survivor, but the survivor is narrow (parameter provenance + exactness only) and its economic case is weak**: companion §7.2 projects the byte prize of the only untested extension at **≤ ~0.02% end-to-end** |

### Track 10 — correction topology coding → **CLOSED-BY-PRIOR-ART + CLOSED BY MEASUREMENT**

`RECORDED` `gate-priorart-audit-i8.md` §3: the mask-only formulation exists (US 12,373,439 /
US 2024/0211132); self-reference + exact COPY + corrections is **fully anticipated** by VCDIFF
RFC 3284; entropy-coding correction *topology* was the one NOT-FOUND element — **and that
element is now measured dead in this project**: `FORMAT.md` §TOPOLOGY, modal (k,slot)
accuracy 17–23% (not 86.5%), masks ~90% unique (top-32 cover 7–12%), mode 13 lost bytes on
every record file. `RECORDED` no-reburn B1.

### Track 11 — orbit / program synthesis → **split verdict; the generic claim is CLOSED**

| Sub-mechanism | Class | Basis |
|---|---|---|
| General grammar/micro-program VM over bytes | **CLOSED-BY-PRIOR-ART** | RePair, SEQUIT, XMill/MillView, CRUSH, LZH, Breach; and `RECORDED` Exp. Y measured it decode-negative *in this repo* (materializing RePair/RLZ cost 8–16% decode for a 4–11% byte win) |
| Any **transmitted-parameter** program ISA | **CLOSED-BY-PRIOR-ART** | parametric dictionary compression; US 7,676,506; xz `--delta`; `RECORDED` foreclosed twice |
| **Estimator-derived implicit parameters** | **CLOSED-BY-PRIOR-ART / MATH-class** | DNB-M2 `RECORDED`; no estimator removes the log₂(d) slope |
| **H3 iterated spans / FLI's `LOOP_ARITH` generality** | **CLOSED-BY-PRIOR-ART** | **`SESSION` ISLP** — Jeż/Navarro/Olivares/Urbina, *Iterated Straight-Line Programs*, LATIN 2024 (`users.dcc.uchile.cl/~gnavarro/ps/latin24.2.pdf`): iteration rules `A → ∏_{i=k1}^{k2} B_i^{c1}···B_t^{ct}` with exponents **linear in i**, i.e. a counted loop with per-iteration operands from an exact recurrence, zero per-iteration symbols, hard bounded non-recursive expansion. Also L-systems; morphisms/NU-systems (`FRONTIER-RESET` §4.1); GSLP arXiv 2404.07057 |
| **FLI S2 (fused consumer-side counted execution, no materialization)** | **ADOPT-CLASS as a mechanism; survives only as a systems/unification claim** | **`SESSION`** arXiv 2607.24971 computes "directly on the compressed grammar, without materializing uncompressed vectors"; Cleary et al. (`par.nsf.gov/10555943`) non-materializing random access; **and `SESSION` Sakamoto 2014 PMLR v34 published the streaming-grammar-compression problem *as open in 2014*** — which is simultaneously the best and worst citation for FLI |
| **FLI S1 (suppress per-iteration entropy symbols in a loop)** | **SEARCH-GAP** — *not* a clearance | `SESSION` four queries found no shipped token-loop codec; `SESSION` LZO's documented `state` (literals-copied-by-previous-instruction) occupies the narrower zero-bit-operand-inheritance slot; `GAP` LZ-family implementation forks, proprietary/console packers, 18-month publication blackout (`priorart-tcopy-external.md` pass 3) |

### Track 12 — encoder-only search / learning → **ADOPT-CLASS**

Search over already-known representations. `RECORDED` G4 is the negative; OpenZL's offline plan
search is the reference architecture (`FRONTIER-RESET` §4.4). The zero-model-decoder property is
a systems property, not a novelty one.

### Track 13 — numeric / floating representation compiler → **CLOSED-BY-PRIOR-ART**

Gorilla (VLDB 2015), FPC (DCC 2006 / IEEE TC 2009), Chimp/PDE, ALP (arXiv/DOI 10.1145/3626717),
FastLanes, Parquet's ALP adoption (2026-09), columnar RLE/bitpack/delta. `RECORDED`
`docs/priorart-tcopy-external.md` pass 6 documents this in detail and rules **no conflict** with
ANVIL — because it shares the *genus* (encode relative to reconstructed context), which the
`RECORDED` I8 audit already declares never claimable. Bit-exact typed-lane framing is
engineering; ALP-style exception vectors are prior art.

### Track 14 — executable / binary representation compiler → **CLOSED-BY-PRIOR-ART**

BCJ/E8E9 (LZMA SDK, public domain 2008); ZPAQ E8E9; `SESSION` Wikipedia BCJ (rev. 2025-07-13):
present in Microsoft's 1996 cabinet file for LZX, and the explicit observation that **bsdiff
"circumvents the need of writing architecture-specific BCJ tools by encoding bytewise
differences"** — i.e. the BCJ route and the sparse-delta route are publicly recognized as
alternatives; Courgette; Microsoft delta lineage US 6,466,999 / 7,509,636 / 7,676,506;
`RECORDED` five-pass audit NOT-FOUND for the *transformed-copy* species only.

### Track 15 — long-range / cross-block memory → **see §3 below; the two halves split**

| Sub-mechanism | Class |
|---|---|
| dual-timescale / cross-block phrase dictionary | **ADOPT-CLASS** (same basis as track 01) |
| **CAM — compact per-block transmitted adaptive-model prior** | **CLOSED-BY-PRIOR-ART** ← the headline result of this pass |

### Track 16 — segmentation / routing theory → **split verdict**

| Sub-mechanism | Class | Basis |
|---|---|---|
| Generic / per-region cost routing with raw fallback | **CLOSED-BY-PRIOR-ART** | **`SESSION`** Brotli `c/enc/metablock_literal.go` `blockSplitterLiteral` (greedy per-category splitter, `min_block_size_`, `split_threshold_`, three-way split action, entropy-estimate driven), `quality.go` `minQualityForBlockSplit=4` / `minQualityForHqBlockSplitting=10` / `maxNumDelayedSymbols=0x2FFF`; zstd `zstd_compress_internal.h` `@postBlockSplitter` *"more accurate but consumes more resources"* vs `@preBlockSplitter_level`, plus `ZSTD_c_targetCBlockSize=130`; and **RFC 8878 §3.1.1.3 verbatim**: *"If a compressed block is larger than its uncompressed content, it is recommended to send it uncompressed (i.e., a Raw_Block)"* — which **is** ANVIL's own "raw input is always a candidate" invariant, already standardized |
| Rank-1 / low-rank whole-carrier residual calibration | **SEARCH-GAP**, weak | no artifact retrieved; but (i) low-rank is a well-worn modeling choice, (ii) `RECORDED` G4 already measured cheap proxies missing the frozen q11 choices, so this enters a lane that has failed once, (iii) the same mis-calibrated λ (`RECORDED` Exp. AA "right ORDER, wrong GAPS") that weakens track 11's S4 weakens this. File as a **measurement** question, not a claim |

### Track 17 — parser / candidate generation → **ADOPT-CLASS**

Hash chains, suffix arrays, boundary-aligned candidate generation, difference-cover negative
gates, mod-64 sampling — all standard LZ search engineering. `RECORDED` §3 of
`research-agenda.md` already lists "bigger hash table / longer chain" and boundary heuristics as
"standard LZ engineering … not a mechanism." Any contribution is **search formulation**, i.e. a
systems claim at best.

### Track 18 — new decoder / backend architecture → **ADOPT-CLASS**

Superinstructions (interpreter literature), SIMD kernels, control/data separation
(**Stream VByte**, arXiv 1709.08990 — `RECORDED` adopted in `FRONTIER-RESET` §5.2), independent
entropy states (ryg_rans, jkbonfield/rans_static). `RECORDED` all ANVIL decode wins to date are
wire-invisible engineering (PCLMUL CRC 4.5×/3.4×, ALLOC 1.352×, postcoder leg-4 1.179×) and
master-brief item 12 declares that class exhausted.

### Track 19 — format / security / fuzz → **NO-NOVELTY-CLAIM**

Verification and hardening machinery by construction. `FORMAT.md` §Integrity/§Registry
coverage. Fund freely; cite never.

### Track 20 — prior-art / novelty kill team itself → **NO-NOVELTY-CLAIM** (it is the arbiter)

---

## 3. CAM — the model-prior prior art, stated once, in full

**CAM as defined to this agent** (coordinator-supplied definition, not read from track 15's own
document — flagged as a provenance limitation): *a compact per-block transmitted adaptive-model
prior that preserves independent/random-access decode while reducing measured model-warmup tax.*

**Four searches run this session. Result: a direct anticipation, with matching motivation.**

### 3.1 The controlling artifact — `SESSION`

**US 2010/0098181 A1**, *Entropy slices for parallel entropy decoding*
(`ptacts.uspto.gov/.../download-documents?artifactId=H1aQfCrHjiXtOYZLu3q7JFUiEftDaSYTtbzgXEbTzTdDdBhqht6AIIA`).
Verbatim excerpts:

> *"The entropy slice header includes information identifying the header as that of an entropy
> slice and **context model initialization information to be used by a CABAC entropy decoder to
> initialize context models prior to decoding the entropy slice**."*

> *"a CABAC entropy decoder can use this additional information to initialize the state of
> selected context models **rather than resetting the context models to their default initial
> states**."*

> *"the parallel decoding of entropy slices is structured such that **information from
> previously decoded entropy slices may [be] used to estimate the initial context states for
> context models in subsequent entropy slices**."*

> *"the context models for entropy slice X+M can be initialized with **estimated values based on
> the final states of the context models from one or more ... entropy slices**."*

And — the passage that matches ANVIL's motivation verbatim:

> *"The first reason is the requirement that the context model probability states used in CABAC
> are reset to their initial, default states at the beginning of each entropy slice. **As the
> entropy slice size is decreased, the resets of the context models occur more frequently.**"*

### 3.2 Supporting artifacts (all `SESSION`)

| # | Artifact | What it establishes |
|---|---|---|
| **P1** | `iphome.hhi.de/marpe/cabac.html` + Fraunhofer HHI CABAC page | CABAC context init is **already a transmitted, compact, per-slice, per-model prior**: a pair of init parameters per model describing a modeled linear relationship between `SliceQP` and the model probability; the encoder **signals its choice** among three tables. Fraunhofer's own text: pre-adaptation is *"especially in the case of using small slices at low to medium bit rates"* |
| **P2** | **HEVC `TDecSlice.h`** (`hevc.hhi.fraunhofer.de`, HM reference) | `TDecSbac m_lastSliceSegmentEndContextState;` — *"context storage for state at the end of the previous slice-segment (used for dependent slices only)"*. Plus `m_entropyCodingSyncContextState` for wavefront/entropy-coding-sync |
| **P3** | **US7365659B1** | *"a method which can quickly initialize context models for a new data slice in the process of context adaptive binary arithmetic coding"* |
| **P4** | **US8344917B2** (Misra, Segall / Cisco) | "Methods and systems for context initialization in video coding and decoding" — *"a plurality of context models used for entropy coding may be initialized, using a context table, at the start of an entropy slice"* |
| **P5** | JCTVC-B111 (ITU-T archive) | *"Context model initialization: context modes are initialized or reset to predefined states at the start of the entropy slice"* — the baseline that CAM and US'818A1 both modify |
| **P6** | JCTVC-I Notes / JCTVC-J Notes | *"Simplified CABAC Initialization for WPP"*; *"CABAC Context Initialisation to Reduce Parallel Encoding Losses"*; *"it is assessed that this initialization mechanism requires a buffer for storing the states of the CABAC probability table before it is used"* — i.e. the exact cost/rate trade CAM proposes was **debated in the standards process in 2012** |
| **P7** | Moffat, Neal, Witten, *Arithmetic Coding Revisited*, ACM TOIS 16(3), 1998 | `install_symbol()` exists on **both** sides and its purpose is *"to allow contexts to be primed, as if text had preceded the beginning"* — the general coder's toolkit already contains model priming |
| **P8** | **RFC 7932 §2 / §6** (Brotli) | *"codes for each meta-block are independent of those for previous or subsequent meta-blocks"* — the independence requirement is a **specified, standardized constraint** of a shipped format, not a CAM insight. Brotli also ships `-D/--dictionary` (P-tracked above). Brotli does **not** transmit probability-state priors across meta-blocks → the general-purpose half is a `GAP`, but it is a gap *below* a fully-anticipated video-codec prior |
| **P9** | **RFC 8878 §3.1.1.2** (zstd `Treeless_Literals_Block`) | *"this is a Huffman-compressed block, using Huffman tree from previous Huffman-compressed literals block. Huffman_Tree_Description will be skipped."* — the **carry-over** variant of model reuse, shipped and standardized |
| **P10** | arXiv 2605.02904, StateSMix | *"A warm-up schedule applies more iterations to early chunks, bootstrapping the model quickly before the n-gram tables have accumulated enough observations"* — warm-up acceleration is an active, published line of work in a different form |

### 3.3 Claim chart — CAM

| Element of CAM | Verdict | Basis |
|---|---|---|
| "Transmit a compact initialization/prior for adaptive context models instead of resetting to default" | **SAME — anticipated, in a published application, 2010** | **US 2010/0098181 A1**, verbatim |
| "The prior is per-chunk and does not break independent chunk decoding" | **SAME** | US'818A1 entropy slices are *by construction* independently decodable — that is their purpose (parallel decoding) |
| "Motivation: finer chunking increases model-reset/warm-up frequency" | **SAME — stated verbatim in the same document** | US'818A1: *"As the entropy slice size is decreased, the resets of the context models occur more frequently."* |
| "The prior is derived from the final model states of previous chunks" | **SAME** | US'818A1: *"initialized with estimated values based on the final states of the context models from one or more ... entropy slices"* |
| "Compactness of the prior is charged (per-model init parameters + a signaled table selector)" | **SAME** | P1 (CABAC init tables + `cabac_init_idc`), P6 (the buffer cost was debated in 2012) |
| "Carry-over instead of reset, when independence is not required" | **SAME** | P2 (HEVC dependent slice segments), P9 (zstd treeless literals block) |
| "Priming an adaptive coder's contexts" | **SAME** | P7 (`install_symbol`, 1998) |
| "Applying this to a **general-purpose lossless byte-stream compressor** with per-block independent decode + random access" | **DIFFERENT — the only unoccupied cell** | P8 shows Brotli deliberately keeps meta-blocks independent and does **not** transmit probability priors; no general-purpose codec was found that does |
| "The mechanism is worth doing for a general-purpose codec's measured warm-up tax" | **UNCERTAIN, engineering** | not a novelty question — see §3.4 |

### 3.4 Verdict on CAM — novelty vs engineering, kept separate

**NOVELTY: CLOSED-BY-PRIOR-ART.** Every element of CAM other than its application domain is
anticipated in published art, with the *same stated motivation*, and the domain-restricted
variant is explicitly what makes CABAC/H.264/HEVC patents and standards work. A general-purpose
byte-stream instantiation is the only open cell, and **"port a video-codec entropy technique to
a general-purpose compressor" is the project's recorded definition of adopt-class**, not of a
mechanism: `RECORDED` `docs/gate-priorart-audit-i8.md` §1 already ruled that adapting
predictor-relative transforms (Gorilla/Parquet/`xz --delta`) into ANVIL's architecture is
*"Gorilla/Parquet with a different jacket"* and recorded mode-16 ARI-REF as
*"engineering / integration, no mechanism-level novelty."*

**ENGINEERING VALUE: INTACT AND PROBABLY REAL.** Three independent reasons, none of which is
about novelty:

1. **The warm-up tax is `MEASURED` in this repo.** `RECORDED` `docs/audit-2026-09-07/06-do-not-reburn.md` §G1: webster wins BWT on **10/10** 4 MiB blocks, yet sum-of-blocks BWT is **+894,315 B (+12.22%)** over whole-file; Brotli the same way (+773,686 B). Block-locality is closed *as a mechanism* ("**do not re-burn**"), and the register names its own reopen condition: *"a backend with near-zero model warmup appears, **or cross-block model carryover is implemented**."* **CAM is that reopen condition.** So CAM is not re-opening a closed lane — it is the one item on the reopen list.
2. **P6 shows the trade is real and contested**, i.e. there is genuine rate to buy, but also that its size is not obvious — which is why it belongs in an *engineering* gate, not a novelty claim.
3. **The `GAP` cell is worth an engineering experiment**: no general-purpose codec transmits a
   probability-state prior across blocks while keeping blocks independently decodable. If ANVIL
   measures that this is worth bytes, it is a genuine *engineering* differentiator against
   Brotli/zstd/xz — a Pareto point, not a paper.

**RECOMMENDATION TO TRACK 15: re-file CAM as `ADOPT-CLASS` (engineering) with an
adopt-class pre-registration, and cite US 2010/0098181 A1, US7365659B1, US8344917B2, the HHI
CABAC init specification, HEVC dependent slice segments, and RFC 8878 treeless literals
voluntarily.** A voluntarily-cited anticipation is a strong gate record; an
uncited one that a reviewer finds is a credibility failure. Do **not** put the word "novel"
anywhere in the CAM artifacts.

---

## 4. Ranking — novelty-status-only, ranked by *could this change a research decision?*

Engineering value, ratio prospects, and throughput are **deliberately excluded** per the task
instruction. A mechanism is ranked high here only if resolving its novelty status would change
whether a lane receives prototype/CI budget.

| Rank | Mechanism | Track | Why its novelty status is decision-changing | Current status | Cost to resolve |
|---:|---|---|---|---|---|
| **1** | **CAM — per-block transmitted adaptive-model prior** | 15 | **Just changed, decisively.** Directly gates whether track 15 may write a novelty claim at all. | **CLOSED-BY-PRIOR-ART** (resolved, US 2010/0098181 A1) | **RESOLVED — no further work needed** |
| **2** | **H2 — reference-local derived-mask transform copy** | 09 | Decides whether the last *untested* separator in the only surviving species gets a build | **CLOSED-BY-PRIOR-ART as a claim** (BCJ + `start_offset`; companion §16.2). Engineering screen still optional | **RESOLVED as a claim**; the remaining C-Σ test is a *measurement*, not a novelty question |
| **3** | **FLI S1/S2 — fused counted loop, per-construct** | 11 | Decides track 11's own pre-registered G0, and G0 is currently written as a claim gate | S3 **CLOSED** (ISLP); S4 **CLOSED** (Brotli `encode.c`); S1 **SEARCH-GAP**; S2 **ADOPT-CLASS as mechanism / survives only as systems claim** | One targeted pass on shipped-coder loops + patent search on "loop opcode" |
| **4** | **TCOPY/PNRA exact implicit Δ=−d** | 09 | The **only** surviving claim in the program; resolving it either keeps or closes the project's entire novelty position | **NOVEL-SURVIVOR (sole), conditional** — separator = parameter provenance + exactness; audit discharged | The never-run **A1–A4 provenance ablation** (remote, byte-only) — outstanding since I2-5 |
| **5** | **Generic / per-region cost routing with raw fallback** | 16 | Decides whether track 16's framing can carry a claim | **CLOSED-BY-PRIOR-ART** (Brotli block splitter + zstd split analysis + RFC 8878 raw rule) | **RESOLVED** |
| **6** | **Rank-1 whole-carrier residual calibration** | 16 | Only remaining cell in a lane whose generic form just closed | **SEARCH-GAP**, and weak (G4 already negative; λ mis-calibrated) | One literature pass on learned/low-rank rate prediction + treat output as a measurement |
| **7** | **Correction-topology coding** | 10 | Could in principle have been the fallback novelty position | **CLOSED-BY-PRIOR-ART and by measurement** (masks ~90% unique; mode 13 lost 13.5–21%) | **RESOLVED — no further novelty work** |

Everything else in §2 is **ADOPT-CLASS** or **NO-NOVELTY-CLAIM** with a recorded in-repo ruling
behind it; none of their novelty statuses can change a research decision, so none is ranked.

---

## 5. Cross-cutting observations the coordinator should act on

1. **Three of this pass's four decisions closed mechanisms by finding a *parameterization or
   placement* already documented elsewhere** — `start_offset` for H2, ISLP for H3/FLI-S3,
   CABAC entropy-slice init for CAM. **ANVIL's surviving novelty keeps turning out to be a
   granularity shift inside a standardized mechanism.** That is the profile of obviousness. I
   recommend the gate ask a new standing question for every candidate: *"what is the separator
   that is not a granularity or parameterization choice inside a standardized mechanism?"* If
   the answer is empty, the honest disposition is adopt-class, and this program should stop
   funding novelty hunts and start funding measurement.
2. **Patent anticipation searches pay off far better than paper searches** for this project.
   All three of the decisive hits (CABAC entropy slices + two CABAC-init patents; Brotli's
   `start_offset`) came from **patent and standards documents**, not from the compression
   literature. `RECORDED` `docs/gate-priorart-audit-i8.md` §5 records that ACM/IEEE were never
   queried and arXiv was rate-limited — and yet the *papers* are the softer part of the problem.
   **Recommendation: fund patent/standards search as the primary novelty instrument, and treat
   arXiv/CS literature as secondary.**
3. **Every closed-by-prior-art verdict above should be cited voluntarily in the
   corresponding preregistration.** Sakamoto 2014 (streaming grammar compression, open
   problem), US 2010/0098181 A1 (CAM), RFC 8878 (raw fallback), Brotli `start_offset` — all are
   directly on point and all are better found by the project than by a reviewer.
4. **Three standing `GAP`s remain** and are recorded, not resolved: LZ-family implementation
   forks and proprietary/console packers; LZMA block-mode/cut selection; learned per-block
   codec selection. None should be read as absence.

---

## 6. Bottom line

One survivor out of twenty tracks. That survivor's separator is *parameter provenance plus
exactness* — a real, audit-discharged distinction — and its economics are projected weak. Three
mechanisms that looked like the program's next generation (H2, H3/FLI-S3, CAM) closed this week
against patents and standards documents, not against peer-reviewed compression literature. **The
correct reading of this matrix is not "ANVIL has no innovation"; it is "ANVIL's innovation is
engineering-class and should be budgeted, labelled, and measured as such, while the residual
novelty program narrows to one mechanism with one remaining decisive experiment."**

---

---

# PART II — ADVERSARIAL QUEUE REVIEW (NOVELTY)

**Reviewed:** `docs/swarm-2026-10-02/REMOTE-EXPERIMENT-QUEUE.md` (DRAFT v0, 161 lines).
**Scope of this review:** Tier 2 (Q4, Q4a, Q4b, Q5, Q6, Q7) and BLOCKED/CONDITIONAL
(B1–B4), as instructed. Tier 0/1/3 examined only for novelty-language leakage that would
affect a Tier-2/Blocked decision; Tier 3 is self-labelled "no novelty claim" and I concur.
**Nothing in the queue file was modified.** This review is advisory.

## 7. Headline of the queue review

**The queue is in unusually good shape on claim class, and it has already absorbed all four of
this pass's novelty closures** (FLI, H2, CAM ×2 — `B4` and the do-not-dispatch list). I found
**zero** items in Tier 2 or Blocked whose claim class is misclassified as NOVEL-SURVIVOR.

**I did find three real defects, and one omission that is more important than all three:**

- **D1 (omission, highest value).** **The only job that could definitively close the program's
  sole surviving novelty claim is not on the queue.** The queue lists "TCOPY/PNRA provenance
  matrix as frontier work … Optional claim-closure only" in DO-NOT-DISPATCH, but **no funded
  item anywhere carries the A1–A4 provenance ablation**. That ablation is `RECORDED` as
  outstanding since I2-5 and never executed (`docs/gate-priorart-audit-i8.md` §6 action 5).
  §2 of this file classifies TCOPY/PNRA as the **only** NOVEL-SURVIVOR. A queue that funds
  measurement substrate (Q0–Q3) while leaving the one claim-bearing experiment unfunded is
  **rationally consistent only if the coordinator has decided to abandon the claim**. If that
  decision has *not* been made, the queue is missing the single highest-information job in the
  program. See §7.6 for the exact recommendation.
- **D2.** Ranking rule 5 says novelty requires a separator that "must survive semantic
  prior-art mapping" — but it **never names the failure mode that has actually killed four
  mechanisms this week**: a separator that is only a *placement / granularity /
  parameterization / composition* shift inside occupied art. Rule 5 as written would pass H2,
  CAM, and H3. Fix rule 5 textually.
- **D3.** Three Tier-2/Blocked items are correctly *classed* but sit in a tier or framing that
  invites the novelty reading back in after a positive result. Each needs one explicit
  prohibition line.

## 8. Per-item verification — Tier 2

| Item | Stated class | Verified class | Verdict |
|---|---|---|---|
| **Q4a** typed/frontend basis × BWT byte cross-product | *(no class stated)* | **NO-NOVELTY-CLAIM / measurement** | **Correct but unlabelled — needs a class line.** See D3-a |
| **Q4b** BWT stage decomposition / f_walk | *(no class stated)* | **ADOPT-CLASS measurement** | **Correct but unlabelled — needs a class line.** The Amdahl kill condition (~4.5× required, LF-walk must be ≥ ~78%) is properly pre-registered |
| **Q5** Track-10 MASK-CEILING | *(no class stated)* | **NO-NOVELTY-CLAIM / bounded engineering measurement** | **Correct and admirably self-aware** — it already states "Historical mode11↔mode13 A/B is confounded and cannot establish topology value." See D3-b |
| **Q6** Track-01 G5D census-first ablation | "adopt-class family" | **ADOPT-CLASS** | **Correct.** Explicitly self-labels adopt-class. See D3-c |
| **Q7** DEFLATE reconstruction control | "adopt-class engineering" | **ADOPT-CLASS** | **Correct, and the strongest-specified item in the queue.** Requiring *both* `precomp→xz -9e` and `precomp→Brotli q11/lgwin30` is exactly the dual-bar discipline this project has needed three times. Track 08 is `CLOSED-BY-PRIOR-ART` (§2) and the queue matches |

## 9. Per-item verification — Blocked / Conditional

| Item | Verified class | Verdict |
|---|---|---|
| **B1** Numeric reference dual-bar (Track 13) | **CLOSED-BY-PRIOR-ART**, correctly blocked on a corpus that does not exist | **Correct.** Gorilla / FPC / ALP / FastLanes / Parquet-ALP (§2 track 13). "No CANL prototype" is the right disposition, not merely the right sequencing |
| **B2** Planner-routing preflight | **ADOPT-CLASS / accounting reconciliation** | **Correct as HOLD.** Track 16's generic CDR is `CLOSED-BY-PRIOR-ART` (`SESSION`: Brotli `blockSplitterLiteral`; zstd `@postBlockSplitter`/`@preBlockSplitter_level`/`ZSTD_c_targetCBlockSize`; RFC 8878 §3.1.1.3 raw-block rule). The preflight's ceiling is **accounting reconciliation**, never novelty establishment. See D3-d |
| **B3** Track-12 POOL-1 / selector census | **ADOPT-CLASS, claim-withdrawn** | **Correct.** Properly gated on H0 correction and restricted to diagnostics "independent of the withdrawn safe-prune theorem" |
| **B4** CAM model-prior engineering | "Novelty CLOSED-BY-PRIOR-ART. Engineering HOLD" | **Correct and complete.** Matches §3 of this file exactly, including the reopen dependency on Q2. Recommend adding the citations so the gate record is self-standing — see §7.5 |

## 10. Defect detail

### D3-a — Q4a/Q4b carry no class line, and Q4a's *premise* is the occupied idea

Q4a's stated purpose is to test whether **"backend is the mechanism layer"** is true. I verified
this is the right question and that **"representation compiler + backend arbitration" is
`CLOSED-BY-PRIOR-ART` as a mechanism**: OpenZL (configurable graph of reversible ops, offline
plan search, resolved graph in frame, one universal decoder), LogPrism, and the
Grotli/DataCortex structure-discovery pattern are all `RECORDED` in
`docs/I10-GROTLI-FRONTIER-ARCHITECTURE.md` §13 and §2 of this file. Q2/Q4a asking *whether it
wins bytes* is legitimate; **Q4a winning must not be reported as a mechanism result.**

*Required line:* "**Class: NO-NOVELTY-CLAIM (measurement). A positive result establishes a
Pareto point, not a mechanism. Composition of a representation compiler with a backend
arbiter is prior art (OpenZL, LogPrism, Grotli/DataCortex).**"

### D3-b — Q5 is correctly bounded but sits under a "FRONTIER FALSIFIERS" heading

Track 10 is `CLOSED-BY-PRIOR-ART` **and** `CLOSED BY MEASUREMENT` (§2 track 10: masks ~90%
unique; `RECORDED` `FORMAT.md` §TOPOLOGY modal 17–23%, mode 13 lost 13.5–21%). Q5's own text
already refuses the topology-value reading. The residual risk is purely positional: a positive
MASK-CEILING under a "frontier falsifier" heading reads as reopening a closed lane.

*Required line:* "**Class: NO-NOVELTY-CLAIM / bounded engineering measurement. A positive
ceiling does NOT revive topology coding: the formulation is anticipated (US 12,373,439 /
US 2024/0211132; VCDIFF RFC 3284) and the project has measured it negative twice.**"

### D3-c — Q6's "paging" component is a granularity shift inside an occupied family

Track 01 is `ADOPT-CLASS` (§2): Parquet column-chunk dictionaries, Brotli static dictionary +
`-D`, Zstd trained/external dictionaries, FSST, front coding, local/page dictionary layouts.
Q6 correctly self-labels adopt-class. But its component list — *"root capacity, overlay
capacity, **paging**, ordering, and escape costs"* — contains one element whose *only* possible
"new" story is granularity (root → page → overlay), which is precisely the trap. Paging may be
**measured** as a cost structure; it may not be **reported** as a mechanism.

*Required line:* "**Paging is a granularity decomposition of an adopted dictionary family. It
is measured as a cost, never claimed as a mechanism.**"

### D3-d — B2's ceiling must be written down

B2 is the right call but does not say what a *successful* B2 would license. Left implicit, a
reconciled 19× prize asymmetry reads as permission to run two planner pilots with a novelty
framing. It is not.

*Required line:* "**A successful B2 licenses exactly one adopt-class planner pilot with frozen
cost accounting. It does not license a routing/segmentation novelty claim: generic and
per-region cost routing with raw fallback is anticipated (Brotli block splitting; zstd split
analysis; RFC 8878 §3.1.1.3).**"

### D2 — ranking rule 5 needs the granularity trap named

Current rule 5: *"if novelty is claimed, the separator must survive semantic prior-art
mapping."* By that rule alone, all four of this pass's closures would have passed:
- **H2** — separator = reference-local placement (passes "semantic mapping"; **obvious**).
- **H3 / FLI-S3** — separator = iteration-rule parameterization inside SLP (passes; **anticipated by ISLP**).
- **CAM** — separator = per-block granularity + parameterization inside CABAC init (passes; **anticipated by US 2010/0098181 A1**).
- **H2 vs BCJ** — separator = composing a public filter with its own documented `start_offset` parameter (passes; **derivable**).

*Suggested replacement for rule 5:*
> **Novelty honesty.** A claimed separator must survive semantic prior-art mapping **and must
> not be only a placement, granularity, parameterization, or composition choice inside a
> mechanism that is itself anticipated.** If the only remaining difference from cited art is
> *where* or *how finely* an established mechanism is applied, the class is
> **ADOPT-CLASS**, not a novelty claim. "This variant is new because it is applied per-block /
> per-region / per-reference rather than globally" is a **disposition, not a claim.**

## 11. Correctly-labelled items where the framing still needs a guard

These are **not defects** — the classes are right — but each has a plausible positive result
that a reader could mis-cite. One line each is cheap insurance.

| Item | Plausible mis-citation to forbid |
|---|---|
| **Q2** block-window parity | If Q2 measures a real restart tax, the natural follow-on is "fix the warm-up by carrying models across blocks." That follow-on is **ADOPT-CLASS by anticipation**: CABAC entropy-slice initialization (US 2010/0098181 A1, US7365659B1, US8344917B2; HHI CABAC init tables + `cabac_init_idc`), HEVC dependent slice segments, RFC 8878 treeless literals blocks. Q2 may establish *the deficit*; it may not establish *the fix's novelty* |
| **Q3** selector arbiter | `SESSION`: Brotli `c/enc/encode.c` already gates a representation choice on `entropy[1] - entropy[2] < 0.2` *"in exchange for faster decoding speed"*, with constants tuned on Silesia. Per-construct gating is the narrow unclaimed cell — but `RECORDED` Exp. AA says the project's λ is *"right ORDER, wrong GAPS"*, so a disagreement result is a **calibration finding**, not a mechanism demonstration |
| **Q9** aux-index W1024 vs W16 | Correctly adopt-class. No guard needed beyond the existing "byte/correctness only first; no timing sweep" |
| **Q7** DEFLATE | Correctly adopt-class. The one guard worth adding: precomp/preflate/reflate/grittibanzli are cited as prior art in the job description so the artifact is self-standing |

## 12. Novelty language actually present in Tier 2 / Blocked — audit result

`SESSION`-equivalent string audit of the reviewed region returned **no** instance of "novel",
"novelty" (outside the two *correct* closures B4 and the do-not-dispatch list), "first",
"unprecedented", "novel-by-difference", or any superiority phrasing attached to a Tier-2 or
Blocked item. The queue's language is already disciplined; the defects above are **structural
(tier placement and omitted class lines), not lexical.** That is worth stating, because the
usual failure mode in a queue like this is wording, and this one avoided it.

## 13. Net effect on the queue — unchanged in dispatch order, with two additions

**No dispatch-order change is warranted by this review.** The queue's own invariant — *"a
downstream job whose premise is invalidated by an upstream result is cancelled, not run for
completeness"* — already implements the dependency structure correctly, and `B4`'s dependency on
Q2 is the right shape.

**Two additions I recommend, in priority order:**

1. **Add one funded, byte-only, remote job: the A1–A4 provenance ablation** (D1). This is the
   only experiment in the entire program whose outcome can close the only claim that survives
   §2's matrix. It is cheap (`RECORDED` as never-run since I2-5), it needs no timed
   implementation, and **I have pre-registered its expected outcome as NEGATIVE** in the
   companion file (§0: "the realistic and explicitly expected outcome is that the project's last
   novelty claim closes"). Pre-registering a negative is what makes this job safe to fund: it
   cannot be reinterpreted afterwards as a win. If the coordinator has already decided to
   abandon the claim, then instead **delete** the NOVEL-SURVIVOR row in §2 and say so, rather
   than leaving an unfunded survivor on the books.
2. **Apply D2 (rule 5 replacement text) and the four D3 class/prohibition lines.** Pure text
   edits to a DRAFT file, zero measurement cost, and they are what prevents the next
   positive-result cycle from re-litigating H2/CAM/H3 from scratch.

## 14. What this review does *not* change

My §2 classification of all 20 tracks stands unchanged. Nothing found in the queue moved any
mechanism between classes. Specifically: **TCOPY/PNRA remains the sole NOVEL-SURVIVOR** (not
removed — just unfunded, which is a *decision* for the coordinator, not a *finding* of mine),
**CAM stays CLOSED-BY-PRIOR-ART with engineering value intact**, and **the seven
ADOPT-CLASS / five NO-NOVELTY-CLAIM items stay exactly as classed.**

---

*Written by Space Bunny Free, track 20-priorart-killteam, 2026-10-02. No measurement performed
by this agent; no existing file modified; no commit, push, reset, clean, stash, restore, or
rebase performed. Literature- and claim-classification record, not legal advice; FTO attorney
review remains recommended (`docs/priorart-tcopy-external.md` §FTO note,
`docs/gate-priorart-audit-i8.md` §5).*