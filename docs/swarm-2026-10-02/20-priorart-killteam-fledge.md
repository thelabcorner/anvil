# Track 20 — Novelty / Prior-Art Kill Team — **Fledge Alpha Free**

**Role:** independent adversarial reviewer. Constructive lane pairing: `Space Bunny Free` (not yet
written at the time of this interim). **Inherits nothing** from any Space Bunny report; every
mechanism below was re-derived from local prior-art records, `FORMAT.md`/`RESEARCH_LEDGER.md`,
and my own codec-family knowledge.

**Status:** INTERIM. Written under a synthesis deadline before the paired report landed.
Reconciliation section (§11) is explicitly PENDING and must be completed by a later pass.
**Nothing here modifies production, format, ledger, or source.**

---

## 0. Verdict up front

| Mechanism (source lane) | Novelty verdict | Engineering verdict | Track-20 ruling |
|---|---|---|---|
| **FLI** (Track 11) — fused counted loop instruction | **NONE at mechanism level.** S3 anticipated; S4's genus occupied; S2 is at best a systems/unification claim. **The decisive kill is not prior art — it is the definitional identity `LOOP_EQ` ≡ one periodic LZ match (§4.1).** | Byte leg unmeasured; now bounded by two cheap counters | **HOLD as adopt-class `{engineering}`; the novelty claim is KILL** |
| **SLX-REF / H2** (Track 09) — per-phrase computed-topology transformed copy | **NONE at species level.** `C1`-only scope is *localized BCJ*; the constructive lane's `start_offset` evidence (§11.2) confirms the position-origin parameter is shipped, intended, 20 years old. `C2` blocked + Courgette overlap. | Byte leg worth one counter | **HOLD as a claim; run the byte arithmetic as an engineering screen only** (upgraded from my interim PILOT on their evidence) |
| **CDR** (Track 16) — low-rank correction of local cost estimates from O(1) carrier probes | **NONE.** Closed by shipped Brotli block-splitting + zstd splitters + RFC 8878 raw fallback, and by my own obvious-combination finding. | Unassessed by me (report not present) | **Downgrade to `{engineering}` on arrival** |
| **Deflate reconstruction, aux-unBWT, CRC PCLMUL, ALLOC, leg-4, G5A planner, backend routing** | Already correctly labelled adopt-class / DECODE-SHORT in the record | Real | **No change.** Do not re-burn |

**Track-20 program verdict: HOLD.** Two mechanisms get exactly one cheap counter run; no mechanism
earns a novelty claim from the current evidence. No lane in this swarm currently holds a
mechanism-level novelty position that survives prior-art scrutiny.

---

## 1. What I read, and the provenance classes I use

Read this session (local, primary): `docs/swarm-2026-10-02/MASTER-BRIEF.md`,
`docs/priorart-tcopy-external.md` (passes 1–6), `docs/gate-priorart-audit-i8.md`,
`docs/anvil-i9-findings.md`, `docs/I10-DENSE-FRONTIER-PREREG.md`, plus the constructive lanes
`11-orbit-programs`, `09-exact-tcopy-pnra`, `02-finalist-planner`, `03-heldout-corpus`,
`04-dense-frontier`, `19-format-security`.

Evidence classes used throughout, never blurred:

- **[MEASURED]** — recomputable from a named committed artifact in this repo.
- **[DERIVED]** — my arithmetic over a cited input; the input is named.
- **[STANDARD]** — long-established codec/literature fact from my own knowledge, **not re-fetched
  this session**. Usable for lineage and for obviousness argument; **not** usable as a citation of
  record without a follow-up fetch pass.
- **[FETCHED]** — retrieved by me this session via web search.
- **[PROJECTION] / [HYPOTHESIS]** — explicitly labelled by the origin lane and not treated as fact.

---

## 2. Evidence / provenance audit

### 2.1 The project-wide provenance failure mode

The single most dangerous pattern in this repo is not a wrong claim — it is a **stale Linux number
carrying an entire decision**. It has happened at least four times and it is still load-bearing:

| Number | Origin | Status today | Used as a load-bearing input by |
|---|---|---|---|
| `1,761,776` vs `1,781,130` B (.text "density leg crossed") | Linux EPYC, `anvil_frontier`, one host | **self-flagged stale**; the same doc concedes ~100 KB of syntax inefficiency | Track 09 §1.2 — *cited by the lane itself as disconfirming* |
| `~270 MB/s` decode / `~16-39 MB/s` encode | same | stale | Track 09 §3.3 cycle model |
| context-clustered rANS `141,833 B`, K=12 | same | stale; already demoted to enabling infra after Brotli RFC 7932 | none now (correctly) |
| shape-predict `109,700 B` JSON | same | stale | Track 11 §9's *pre-registered primary cell* `generated.log` numbers come from this family |

**[Ruling R1]** No Linux-era number may appear in a threshold, a gate, or a cost model without the
`{linux-stale}` tag **and** a Windows-or-GHA re-measurement. Track 09 complies; **Track 11 §9 does
not** — it freezes `anvil-hotop-rans 175,550 B / 273.534 MB/s; brotli-q9 124,669 B / 619.508 MB/s`
as the primary cell. If those are Windows-I9 numbers I cannot confirm the provenance from the
lane text alone; **provenance of the Track 11 primary cell must be pinned before dispatch.**

### 2.2 A factor-1000 arithmetic error in Track 09 §3.1 — and why it does *not* overturn the verdict

Track 09 states:

> "With ~5,070 transformed phrases on 3.26 MB `.text` (L2), that is **≈0.63 B out of 1,761,776 B —
> about 0.000036%**."

**[DERIVED] Check:** 5,070 phrases × 1 bit/phrase = 5,070 bits = **633.75 B**, and
`633.75 / 1,761,776 = 3.598e-4 = **0.0360%**`. The published figure is **1000× too small on both
the byte count and the percentage**; "≈1 bit/phrase × 5,070 phrases" cannot be 0.63 B.

Consequence, and it matters procedurally:

- The lane's conclusion — *the parameter (implicit-Δ) axis is not where the money is* — **survives**.
  634 B on 1.76 MB is still ~0.036%.
- The lane's *supporting* claim — *"an implicit-vs-transmitted ablation run only on rel32 is not a
  meaningful novelty test… it cannot produce a resolvable difference"* — is **wrong on its own
  terms**. These arms are deterministic encoders over one frozen token multiset. A deterministic
  634 B difference is **exactly** resolvable with zero statistical noise. Track 09 pre-registers
  **G2 as a "deliberately underpowered expected-null" test**; under correct arithmetic it is a
  fully-powered deterministic test whose expected outcome is a small *real* win for `A3`.
- **[DERIVED] The A1−A4 ablation at `RESEARCH_LEDGER.md:936-942` is therefore NOT underpowered by
  construction.** It has been described as underpowered for at least two iterations. The A1–A4
  ablation, long outstanding as the "sole separator for the entire transform-reference family"
  (`docs/gate-priorart-audit-i8.md:230-232`), should be run **as written** and its nulls reported as
  determinate, not as noise.

This is a correctness-of-record fix, not a verdict flip. It raises the value of the A1–A4 run.

### 2.3 Dual-bar coverage audit of the constructive lanes

The dual bar (`docs/anvil-i9-findings.md` §2) is the project's own strongest instrument and it is
the thing most likely to be quietly dropped.

| Lane | Same-transform control present? | Audit |
|---|---|---|
| Track 09 SLX-REF | **Yes** — `A4` global BCJ/E8-E9 + same backend, `A5` relocation-normalized, both binding as NO-GO `N2` on *every* held-out cell | **Correct and unusually rigorous. Keep.** |
| Track 11 FLI | `A4` grammar control (RePair fused) — but **no same-transform control** (no `xz --x86`, no `brotli+BCJ` on the same cells) | **Gap.** FLI on executables would need one; if FLI only fires on record-structured files, dual-bar does not bind and the `{synthetic}` tag must be explicit |
| Track 02 LCPS | declares its own dual-bar obligations | Plausible; not audited further |
| Track 03 corpus | n/a (protocol) | — |
| Track 04 arbiter | n/a (instrument) | Correctly refuses to emit a crossing token |

### 2.4 Projections being read as measurements — the two to watch

1. **Decode multiples.** `14.597×` (Silesia) / `23.946×` (enwik8) slower than brotli-q11 are
   **[MEASURED]** but *single-run* unless a PR-4 window is named (`docs/anvil-i9-findings.md:165-167`).
   Meanwhile every lane's forward-looking throughput claim rests on the projection
   *"portfolio ~25.7 MB/s vs the 40 MB/s bar"* (`:249`), which is **[PROJECTION] built on a
   still-pending `aux-unBWT` integration** (`:476`). **No lane may quote the 40 MB/s bar as
   reachable.**
2. **Decoder-size axis.** `docs/I10-DENSE-FRONTIER-PREREG.md:228` states binary size "is not
   treated as a directly comparable crossing axis until a separate decoder-only build policy is
   frozen." **No crossing claim may cite decoder size today.** This axis is currently unmeasured
   *and* structurally adverse: `src/anvil.cpp` is 273 KB of source with a very large decoder state
   surface (BWT + rANS + patch + hot-op + macro modes). Treat decoder size as a likely **loss**,
   not a free axis.

### 2.5 Frontier provenance

**[MEASURED]** Canonical frozen tuple `33 non-dominated | 5 FRONT-GAP | **0 FRONT-CROSSING** | 28
DEGENERATE | 435/468`; committed HEAD with xz-9e `4 | 4 | 0 | 412/416`
(`docs/anvil-i9-findings.md:189,206-209`). `GRID-THIN` is resolved only for the *frozen dense grid*;
denser tiers can only make crossings harder (`:242` of the dense prereg). **Every constructive lane
currently proposes a byte-only or counter-only experiment. Under PR-3, a bytes-only result is
`BYTE-WIN` and can never be a `FRONT-CROSSING`.** No lane has earned frontier language and none
should be given it by the coordinator on a byte-only run.

---

## 3. Prior-art map — the claim chart

The genus is fixed and unclaimable (**`docs/gate-priorart-audit-i8.md:20-27`**):

> Encode a value relative to already-reconstructed context.

Everything below is an attempt to escape that genus by *locality*, *computation*, or *execution*.

| # | Claim under test | Genuinely unoccupied residue | Verdict |
|---|---|---|---|
| **K1** | BCJ-class relative→absolute rewrite | none — public domain since 2008, `[FETCHED]`-confirmed in the local record | **OCCUPIED** |
| **K2** | Two-file executable delta with rel32 corrections (Zucchini, Microsoft `'999`/`'506`, Red Bend `'552`) | none for two-file | **OCCUPIED** |
| **K3** | Self-referential copy + corrections | none — **VCDIFF/RFC 3284 (2002)**, `[FETCHED]` this session: "the delta encoding consists of COPY, ADD and **RUN** instructions" | **OCCUPIED** |
| **K4** | Relocation-aware instruction compression | none — Philips `'302`, IBM/STMicro `'656314`; and **binary code movement** (Dynamo-family code motion / prologue–epilogue motion) applies exactly the *copy + fix up relative displacement by the move amount* semantics, **per moved span** | **OCCUPIED in effect** |
| **K5** | Hardware runtime address compaction | none; Intel `'148`/`'665`/`'382` expired, claims read in full, pass 5 | **OCCUPIED** |
| **K6** | Predictive-relative transforms (Gorilla, FPC, `xz --delta`, Parquet) | none | **OCCUPIED** |
| **K7** | Implicit parameter derived from match geometry (Δ=−d) | *unoccupied in a compressor* — the five-pass record stands | **DEFENSIBLE but WORTH ~1 bit/phrase [DERIVED §2.2]** |
| **K8** | **Computed (non-transmitted) transform topology** | **occupied in effect by K1** — BCJ's topology is content-determined and computed, not transmitted. Only the *anchor* differs | **NOT a residue** |
| **K9** | Per-phrase localization of a global pre-LZ filter | the only real delta: locality vs whole-file. This is **engineering placement**, not a new genus | **ENGINEERING** |
| **K10** | Exact transform at *multiple competing anchors*, class-selected | I found no shipped instance. BCJ has **one** anchor per filter; filter chaining (BCJ2, ARM64) selects by *byte pattern*, not by competing anchor for the same site | **narrow, unverified, low-mass slot** |

### 3.1 The decisive H2-vs-global-BCJ comparison (exact byte semantics + charged side information)

This is the comparison that decides H2, and it is favourable to *prior art*, not to ANVIL.

| Property | Global BCJ / xz `--x86` / 7-Zip x86 filter | SLX-REF `C1` (per-phrase localization) |
|---|---|---|
| Transform applied to | every E8/E9 (and E8-config) site in the whole stream | sites inside an accepted back-referenced phrase only |
| Topology (which bytes are sites) | **content-determined, computed, zero bits** | **content-determined, computed, zero bits** |
| Parameter (the addend) | **derived from stream position, zero bits** | **derived from reference distance, zero bits** |
| Charged side information | **0 B** (no class id, no site map, no detection table) | **0 B** per the lane's own §3.4, plus unmeasured `A2` model-header savings and unmeasured `C2` detection machinery |
| Decoder kernel | one whole-stream pass: `target += position` (and the inverse) | one pass *inside* the copy: `target -= d` — **the same arithmetic with a different available register** |
| Post-transform effect | absolute targets become **position-independent**, so a template and its copy become **byte-identical** and match exactly | same benefit, but only for phrases the parser happened to find |
| Failure mode | none for correctness; cost is a full-file pass | false positives outside PE data (lane's own risk register) |
| Reverse-transform exactness | exact | exact |

**Addendum after reading the paired report — stronger evidence, and I accept it.** The constructive
lane retrieved `lzma_options_bcj::start_offset` (liblzma 5.8.2), documented verbatim as *"set the
start offset of the non-first sections so that the relative addresses of the cross-section
branch/call/jump instructions will use the same absolute addresses as in the first section"*, plus
the XZ-Embedded `Bra86.c` decoder whose transform state is documented as `pos` — *"absolute position
relative to the beginning of the uncompressed data"*. That is better evidence than my own derivation
in two respects: it is `SESSION`-sourced rather than `[STANDARD]`, and it establishes the position
origin as an **intended, shipped filter feature**, not merely an obvious composition.

Composition: `bcj(pos = source_position)` applied to the destination span is exactly "add `d` to the
already-copied word". That is H2's Δ=−d, expressed as the filter's own documented parameter. **I
accept this and I am downgrading my interim PILOT to HOLD-as-a-claim accordingly.** The one
residual — reference-local rather than section-local placement — is, in the constructive lane's
words and mine, a *granularity* argument; both of us now advise the project assume obviousness.

**The conclusion is structural, not empirical.** Because BCJ's transform makes relocation targets
*absolute and therefore identical across copies*, global BCJ already achieves the exact rate
property SLX-REF is claiming — "a copied instruction template matches with **no** correction mask
and **no** parameter bits" — with a **simpler** decoder, **zero** charged side information, and
**no dependence on the match parser finding the phrase**. The only thing SLX-REF adds is the
*parser-locality* of the anchor. That is the entire novelty residue, and it is a relocation, not a
new mechanism.

This does **not** prove BCJ dominates on bytes: a global filter also rewrites targets in regions no
phrase ever references, and it perturbs literal contexts. That is a real, measurable trade — and it
is precisely the trade the project has *already measured and retired once* in the neighbouring form
(global lane/field transposition destroyed contiguous phrase structure;
`docs/anvil-i9-findings.md:379`). My position: **SLX-REF's `C1`-only scope is a localization
experiment on a public-domain filter. It must be run as such, labelled `{engineering}`, and must
never be recorded as a novelty claim.**

`C2` (ABS32, `Δ=+d`) is the only place where no global filter can reach, because the anchor is the
*reference distance*, which no whole-file pass can see. That is a genuine structural gap — but the
lane itself blocks `C2` on a locked held-out corpus that does not exist (§1.3), and `C2` overlaps
Courgette's ABS32 handling (lane's own HIGH risk). **So `C2` — the only defensible residue — is
currently unbuildable, and the promotable scope is exactly the part that is occupied.**

---

## 4. Track 11 handoff: attacking FLI's separators S1–S4

Track 11 explicitly hands me S1–S4 and pre-registers `G0`: *"any of S1/S2 anticipated ⇒ NO-GO
(downgrade to adopt-class)"*. It also correctly states that its own two failed searches are **"a
gap, not a clearance."** I decline that clearance.

### S1 — *"No shipped general-purpose codec has an instruction that suppresses per-iteration entropy-coded symbols inside a loop body."*

**ANTICIPATED.** The claim is stated over *general-purpose codecs* and over *entropy-coded symbols*,
and it is defeated inside both restrictions:

- **VCDIFF / RFC 3284 `RUN`** — `[FETCHED]` this session (RFC 3284 text + open-vcdiff + draft-korn
  -vcdiff-04): `RUN` is an explicit counted instruction that emits N copies of one symbol.
  draft-korn-vcdiff-04 says it verbatim: *"The RUN instruction is a compact way to encode a
  sequence repeating the same byte even though such a sequence can be thought of as a periodic
  sequence with period 1."* A general-purpose, shipped, standardized **counted loop in the decode
  stream with zero per-iteration symbols**. The lane's own §3.6 concedes counted repetition is
  grammar-coder art and that periodic byte loops are "DEFLATE overlapping copies and LZMA
  rep-matches" — `RUN` is the cleaner instance and the lane does not list it.
- **bzip2 `RUNA`/`RUNB`** `[STANDARD]`: a run of equal MTF values is coded as **one** run symbol
  plus a length, with the MTF/RLE2 stage fused into the decoder. Zero per-iteration symbols, fused,
  in a general-purpose shipped codec. (Not re-fetched this session.)
- **Zstandard FSE `RLE` mode** `[STANDARD]`: a dedicated one-state table that emits N identical
  symbols from a single decode, i.e. the repetition loop is folded *into the entropy decoder*, not
  into a separate expansion buffer. (Not re-fetched this session.)
- **Coefficient-run coding in lossy codecs** (HEVC CAVLC `coeff_token` runs, CABAC) is the same
  mechanism in the entropy layer.

S1 as a *general-purpose-codec* claim is therefore **false**. S1 survives only as "no shipped
**token** loop over **multi-parameter** LZ operands" — which is a much narrower statement than the
lane writes, and is immediately attacked by S3.

**S1 verdict: the GENUS is ANTICIPATED; the narrow LZ-token instance survives.**

**Reconciled against the constructive lane (see §11.1).** The paired report read S1 as restricted to
counted loops over an **LZ token stream**, and on that restriction my VCDIFF `RUN` / bzip2
`RUNA`/`RUNB` / zstd `RLE` artifacts do *not* land, because each emits a repeated *single byte*,
not a multi-parameter copy token. **I therefore withdraw my interim claim that S1 is anticipated,
and concede the constructive lane's narrower reading.** What survives my artifacts is a *narrowing*
that still matters: the general technique "counted run that suppresses per-iteration entropy symbols"
is occupied, so any S1 claim must be stated at the LZ-token instantiation and cannot be sold as a new
technique. Both readings agree the claim is **not mechanism novelty**.

**This concession does not rescue FLI**, because FLI's dominant expected case fails on §4.1, which is
definitional and cites no prior art at all.

### S2 — *"No shipped grammar/superinstruction coder executes its loop fused in the consumer rather than expanding to a buffer."*

**ANTICIPATED, and additionally vacuous as stated.** I was asked to search threaded/superinstruction
decoders, grammar streaming decoders, looped LZ commands, and compressed instruction streams.

- **Grammar streaming decoders.** Streaming/lazy grammar decompression is the standard formulation
  — it is the whole point of output-limited grammar coders and of the "lazy parsing" line. Every
  shipped LZ decoder (DEFLATE/zlib, LZMA, Brotli, zstd, LZ4) fuses its copy straight into the
  consumer's output buffer and materializes nothing. Fusion *is* the shipped norm.
- **The specific stronger form — fusing a loop body into a *consumer codec*** — is exactly
  **superinstruction formation**, and that is 20+ years of settled interpreter/JIT art
  (`[FETCHED]` Dynamo, and the surrounding trace-compilation literature; `[STANDARD]` Jalapeno,
  DynamoRIO trace compaction). Trace compaction is literally "encode a repeated slice once, with a
  count, and execute it fused in the consumer."
- **Compressed instruction streams.** Dynamo-family traces are compressed into trace macros with a
  repeat count and register-file operands, decoded and executed fused. Operands come from the
  decoder's own register/operand state — i.e. **zero-bit operand supply by decoder-visible state**,
  which is the lane's `S3` mechanism, already shipped in a trace compressor.

Two independent problems, either fatal:
1. **Obviousness.** "Counted loop + operands from decoder state + fused execution" is the textbook
   composition of superinstructions and rep-offset reuse. Doctrine item 1 forbids claiming it.
2. **Scope error.** If "fused" means "streaming", it is prior art. If it means "loop body fused
   into an LZ copy", it is the FLI idea itself and is an obvious combination.

**S2 verdict: ANTICIPATED ⇒ `G0` NO-GO fires (and the claim is malformed either way).**

### S3 — *"No shipped codec supplies a multi-parameter operand vector by an exact, encode-verified recurrence with a hard non-recursive ceiling."*

**ANTICIPATED in its functional form.** The mechanism is: *when the model is exactly correct over a
run, pay one count instead of k residual/operand symbols.* That is the classical **zero-residual
run** of predictive coding, and it is shipped:

- **DPCM / delta-of-delta predictors** (Gorilla `[STANDARD]`, all predictive codecs): on a linear
  ramp the residual is *exactly* zero and the encoder codes the whole ramp with one escape.
- **FELICS** `[FETCHED]` (Howard & Vitter 1993, IEEE): two-neighbour prediction with context modeling
  and explicit run/skip symbols — a shipped lossless codec that predicts, then codes a **run** where
  the prediction is exact.
- **JPEG-LS / CALIC** `[STANDARD]`: mode selection with exact-predictor escapes and run coding.
- **HEVC CAVLC / CABAC run coding** `[STANDARD]`: same, in the entropy layer.

The lane's own §3.4 argument — *"the residual is identically zero … G4-amended is satisfied not by
an estimator but by construction"* — is a correct description of the standard zero-residual-run
mechanism. That it is *arrived at by a different route* is not novelty; it is re-derivation.

Residual residue worth naming honestly: no shipped codec supplies a **multi-parameter** (length *and*
distance) **arithmetic** recurrence (`δL ≠ 0` or `δD ≠ 0`) over LZ operands with a hard
non-recursive ceiling. That is `LOOP_ARITH` and it is real. But it is one opcode with two varints —
the thinnest possible mechanism — and it is the half the lane itself calls "untested".

**S3 verdict: ANTICIPATED in form; a narrow `LOOP_ARITH`-only residue remains and is engineering.**

### S4 — *"No shipped codec gates a representation choice by a λ-weighted decode-cost objective at the token level."*

**THE GENUS IS OCCUPIED.** `[FETCHED]` decoder-complexity-aware rate-distortion optimization is an
established and actively patented line (e.g. WO2026008257A1; arXiv 2305.07678 on rate-distortion-
complexity in neural compression), and λ-weighted rate-vs-compute selection is the entire RDO
paradigm transplanted. **Honest limit of my own search:** I did **not** find a shipped *general-
purpose lossless* codec that gates by **token-level** decode cost — the instances I found are lossy /
neural and patent-form. So S4's *instance* may be unoccupied.

**But it does not matter, and this is the decisive point:** a claim whose genus is "Lagrangian
rate-vs-decoder-complexity selection" cannot be defended as mechanism novelty merely because no
lossless codec ships that particular instance. That is novelty-by-difference, doctrine item 1. S4
is also, on this project's own record, **already implemented as the `J` objective** — it is
instrumentation the project owns, not a contribution it can publish.

**S4 verdict: NOT DEFENSIBLE as novelty. Treat `J`-gating as project instrumentation.**

### 4.1 A structural falsifier the lane did not run — the strongest single kill in this section

Track 11 assumes `LOOP_EQ` (constant `L₀`, constant `D₀`, run of length `k`) is a new
representation. **It is not. It is exactly equivalent to a single ordinary LZ77 match of length
`k·L₀` at distance `D₀`, under the overlapping/periodic copy semantics the project already
implements for modes 11–15.**

Proof (self-contained): the decoder executes `emit_copy(D₀, L₀)` then advances `pos` by `L₀`, `k`
times. `emit_copy(D₀,L)` copies from `pos − D₀`. Because `pos` advances, iteration *i* copies
`out[i·L₀ + t] = history[i·L₀ + t − D₀]`, reading back into bytes written by earlier iterations.
That is character-for-character the definition of one periodic match `(D₀, k·L₀)` with `D₀ < k·L₀`,
which DEFLATE, LZMA and the project's own modes already permit and already charge once.

Therefore:

- The *representation* FLI introduces for its dominant expected case is **already in the baseline
  format**, with **one** token instead of `k`.
- `LOOP_EQ` is consequently a **token-stream compression / encoding-efficiency device** — run-length
  coding of match descriptors — **not** a new mechanism, and not even a new decoder state.
- FLI's byte win on `LOOP_EQ` is bounded above by the descriptor-redundancy it removes, and it is
  achievable *without any new opcode at all* by RLE/aliasing the existing match descriptors.
- The lane's gate `G7` compares `A2` against a **RePair grammar control** (`A4`) but against
  **no descriptor-RLE control**. That missing arm is the one that matters: if `A4'` = "RLE the
  match-descriptor stream of the same parse" captures the byte win, `LOOP_EQ` is dead as a
  mechanism and only `LOOP_ARITH` survives.

**This is a new required arm, and it is cheaper than anything else in the track.** It also removes
the dependence on my prior-art reading: even if S1–S4 were all clear, `LOOP_EQ` would still be
redundant with a single match.

---

## 5. Track 16 addendum — CDR: plain statement

The routing addendum asked whether anything anticipates **low-rank correction of local cost estimates
using O(1) whole-carrier probes**. Track 16's report was not present in the directory when this was
written, so this is a prior-art ruling in advance, not a review of text.

**Yes, it is occupied, and the occupancy is by combination — which is exactly what doctrine item 1
forbids claiming.**

1. **Generic adaptive block segmentation / routing** is occupied by content-defined chunking
   (FastCDC/Gear and predecessors), by rolling-hash routers, and inside this project by the
   BWT-vs-Brotli router that is already ANVIL's only byte win
   (`docs/anvil-i9-findings.md:287-293`).
2. **O(1) whole-carrier probes predicting a local decision** is the CDC anchor construction: sample
   at a stride, hash, and decide boundaries from the sample. This is *the* defining technique of
   content-defined chunking, `[STANDARD]`, and it is what makes CDC cheap enough to use.
3. **Cost-estimate pruning from cheap statistics** is what learned/discriminative index routing
   already does at scale (choosing an index/plan per segment from cheap features), `[STANDARD]`.
4. **Low-rank / PCA correction of a cost surface** is textbook dimensionality reduction applied to a
   matrix whose columns are per-segment local estimates.
5. Independently, **end-to-end learned/calibrated entropy modelling** — differential / differentiable
   compressors that learn and calibrate the model to minimize final output size — already occupies
   "calibrate the local cost estimate so the global artifact shrinks", `[STANDARD]`.

Items 1–4 compose without any inventive step, and item 5 additionally means the *objective* is
occupied. **Ruling: CDR novelty = NONE; classify `{engineering}`.** If the engineering pilot
survives, that is a legitimate result and should be reported as such — it just cannot be reported as
a mechanism contribution, and the coordinator should not let a passing G-gate rewrite the class of
the claim.

---

## 6. Hidden costs the lanes undercharge

### 6.1 Byte costs

| Item | Lane's treatment | My finding |
|---|---|---|
| Track 11 loop alphabet growth | "≤4 classes, adding ≤4 alphabet symbols", bounded | Correct and cheap. **But** a new opcode class in a shared Huffman/rANS table perturbs *all* symbol probabilities — the cost is not the new symbols, it is the re-optimization delta across the existing table. Unmeasured. |
| Track 11 `n` stream | "1–2 B" | `n` is a **raw** stream read by a **second cursor** (`S_n`, `S_dl`, `S_dd` — three raw cursors, §3.4). Three live raw cursors is a decode-side register/state cost the cycle ledger in §5.2 does not appear to charge. **Charge it.** |
| Track 09 mask elimination | 0.0592 B/covered byte, from measured coded/raw ratios | **[DERIVED] break-even `μ`:** clearing the lane's own `G1` bar of 0.5% aggregate requires `μ ≥ 0.005/0.0592 = ` **8.4% of all input bytes fully covered**. The lane states `μ` is UNMEASURED. **`μ` is the entire mechanism, and it is one counter.** |
| Track 09 `C1` parameter | "≈1 bit/phrase" | See §2.2: 634 B on the stale `.text` point. Real, deterministic, small. |
| Track 11 `repeat.jsonl` control | "must be ≤ same-build exact-LZ row" | Correct and important. `generated.repeat.jsonl` is the project's standing no-regression control (`docs/CONTEXT.md`, SPARSE-REF section). Keep it as a hard gate. |

### 6.2 Cycle costs

- **Track 11 `G1` is unmeasured and is the whole decode thesis.** The lane itself says the decode
  leg's size "spans *material* to *≈5% of whole-codec*" (§12.2). **[DERIVED] Against the real
  budget this is a coin-flip:** decode is copy- and literal-dominated on this architecture
  (postcoder = 91–93% of postcoder work in one arm family; `docs/anvil-i9-findings.md:236`), and the
  port that *is* copy-dominated is precisely where token-loop suppression buys least. `G1`'s 25%
  bar is the right bar; I expect it to **fail**, which is the lane's own stated most-likely outcome.
- **Track 09 cycle claim.** The ~0.164 c/B recognizer is **[ESTIMATE]** from "standard SIMD idiom,
  no ANVIL measurement exists" (lane's own label). The break-even statement — *"the recognizer pays
  for itself iff a single 32-bit entropy-coded mask word costs ≥ ~4.2 cycles"* — is **[DERIVED]**
  and, notably, **not a measurement**. Against the measured postcoder economics in this repo
  (89–91 ns/outB class, i.e. multi-cycle-per-symbol *tens*), 4.2 cycles/word is plausible — so the
  decode leg is not the blocker; **rate is.** Prioritize `μ`, not cycles.
- **Project-wide:** every lane's cycle model is anchored to the stale ~270 MB/s Linux prototype or to
  a 3.0 GHz assumption. Neither is admissible as a threshold input. **[Ruling R1] applies.**

### 6.3 Memory / RSS

- Track 11: 7 loop scalars + 3 raw cursors. **[DERIVED] ~70–100 B of decoder state**, plus the
  three-cursor dispatch. Genuinely negligible — I concede this. The RSS risk is not state, it is
  that the `LOOP_EQ` reformulation invites a **larger materialized match length** (up to `k·L₀`),
  which changes the `LEN_MAX`/`blen ≤ block_size` bound argument the lane relies on (§11 A1). A
  single match of length `k·L₀` must satisfy the *existing* length ceiling, not a new one — so if
  `k·L₀` exceeds the current `LEN_MAX`, the reformulation is not just redundant, it is **illegal**.
  The lane must assert `k·L₀ ≤ LEN_MAX`. This is a concrete correctness gap in §4.3's proof sketch.
- Track 09: 0 B state, +1.5–3 KB `.text` — consistent with the I10-1A calibration precedent
  (+2,976 B). Reasonable.
- Project-wide RSS: the portfolio peak is ~10.3 GiB, **inherited from brotli q11 `lgwin=30`** and not
  ANVIL's own allocation — but it is still an axis loss against xz (100–570 MiB), and *the reference
  helper's* RSS is being counted against ANVIL. That is a **reference-class fairness problem**, not
  an ANVIL win. The dense prereg fixes it by measuring each arm's own RSS; ensure the coordinator
  does not credit ANVIL with matching brotli's 10 GiB.

---

## 7. Decoder and resource risks (the security-shaped findings)

The most valuable thing I found in the lanes is on the format side, and it should not be lost.

1. **Track 19's VCA is attacking a real hole** and its finding is the one I'd most want promoted:
   the report says the RLZ path's attacker-controlled cost is unbounded and that a 64 KiB input can
   force a disproportionate decode bill. **[MEASURED-by-lane]** and consistent with the record's own
   `docs/decoder-audit.md` concern. If true, this is **format-relevant prior art in our own house**
   — deterministic resource ceilings are a *format* property, and every mechanism lane that adds an
   op/stream inherits the obligation. **I support Track 19's PILOT.**
2. **Amplification audit for any loop/recur mechanism.** Track 11 registers `KITER_MAX` and a
   non-recursive ceiling. Cross-check: with `n ≤ KITER_MAX` and `L ≤ LEN_MAX` per iteration, the
   worst case is `KITER_MAX × LEN_MAX` output bytes from one token. The lane asserts the existing
   `blen ≤ block_size` bound dominates — **I cannot verify that from the lane text alone**, and the
   `LOOP_EQ`-as-one-match reformulation (§4.1) makes it *more* true, not less, because it forces
   the product into a single length field that the existing ceiling does bound. **This is an
   argument in FLI's favour that the lane did not make.** Require the assertion in code.
3. **Fuzz obligation.** A new mode ID ⇒ forced encode→decode registry test, per the project's own
   standing rule (`docs/anvil-i9-findings.md:82-84`). Track 11 accepts this. **No mechanism may be
   integrated without it**, and it is cheaper than the mechanism.
4. **Mask/correction uniqueness.** The record's own finding (masks ~90% unique) forecloses any
   mechanism whose payoff is *cloning* correction topology. Both FLI and SLX-REF correctly avoid
   depending on it — SLX-REF's whole pitch *removes* topology, which is the right direction.

---

## 8. The strongest surviving case — stated at full strength, against myself

I am obliged to record the case that defeats my own §4, and it is not weak.

**The strongest surviving argument for the frontier** is not any individual mechanism. It is this:

> The project's *own evidence base* is now the asset. Six I9 rulings, a frozen 13-file grid, a
> dual-bar convention, transform controls, and a byte-exact frozen canonical binary constitute a
> reproducible apparatus for answering "does this mechanism beat its own same-transform control?"
> — a question that, as far as I can establish from this session's searches, **no shipped general-
> purpose codec paper reports**. The decisive `A4`/`A5` control design in Track 09 is a better
> experimental design than anything I found in the external record.

And a second, narrower technical survival:

> **`C2` (ABS32, `Δ=+d`) is a genuine structural gap.** No global pre-LZ filter can reach it,
> because the anchor is the *reference distance*, which does not exist to a whole-file pass. The
> two-file delta family (Zucchini, Microsoft, Red Bend) can reach it, and this project's own record
> contains *no* single-file self-referential compressor that does. That is a real, small,
> unoccupied slot with a clear causal interpretation (co-translating absolute addresses in data
> pools / GOT-style tables).

**Why this does not change my verdict:** `C2` is blocked by the lane's own §1.3 (no locked held-out
corpus), it overlaps Courgette's ABS32 handling, and — per §3 — **everything currently promotable is
`C1`-only, which is localized BCJ.** The defensible core is the part that cannot be built.

---

## 9. Alternative mechanism, if one is justified

The brief requires me to propose a materially different alternative if the main ideas fail. They do,
on novelty. My alternative is deliberately small, and I state its ceiling honestly.

**Anchor-multiplexed exact transform classes (AMX).**

- One decoder kernel. `≥2` candidate anchors are *always* decoder-visible (stream position; the last
  `j` copy distances; `pos − d₁, pos − d₂, pos − d₃`).
- The transform class is **content-determined** (E8/E9 vs E8-config vs data-pool co-translation).
- The anchor within a class is chosen by the **encoder** and transmitted **only when it is not the
  class default**; when it *is* the default, the parameter costs 0 bits.
- Topology remains computed, never transmitted.

**Why this and not the status quo:** it is the *smallest* change that makes the one structurally
unreachable case (`C2`) expressible, and it generalizes Track 09's dead `C2` into something that
still costs zero bits in its dominant configuration.

**Honest ceiling, stated before any data:** the individual idea *class-selects the anchor from a
decoder-visible set*. I found no shipped instance (K10), but (a) the slot is small, (b) it is an
obvious extension of BCJ plus a 2-bit field, and (c) **this project has already been wrong about
"small slots" twice** — implicit-Δ (defensible, failed) and transmitted-Δ (worked, prior art). **I
therefore pre-classify AMX as `{engineering}` and would need a separator better than "no shipped
instance" to promote it.** I do not have one. AMX is offered as the *hypothetical* next
mechanism, not as a recommendation to build it.

---

## 10. The one decisive REMOTE-ONLY experiment (frozen before any datum)

**Name:** `killteam-counters-r1` — **counter/byte-only. No timing. No mechanism build. No production
wiring. One GitHub Actions job.** Tier R1 per `docs/I10-BREAKTHROUGH-PROGRAM.md`.

**Why counters and not throughput:** both lanes' binding uncertainty is *magnitude of a quantity
nobody has measured*, and both magnitudes are **counters**, not wall-clock. Per
`docs/I10-DENSE-FRONTIER-PREREG.md` §12 and the master brief item 7, no local measurement occurs;
this runs remote, byte/counter-only, which needs no PR-4 window.

**Arms (all read-only instrumentation of existing code paths):**

| Arm | Path | Emits | Answers |
|---|---|---|---|
| **K1** | frozen `--pnra=on` path, instrumented | `μ` = fraction of input bytes inside phrases **fully classifiable by `C1`** (every E8/E9 site covered, zero residual, zero tmask); plus `C2`-eligible byte count | Track 09 `G1`; the 8.4% break-even (§6.1) |
| **K2** | frozen mode-15 decode, instrumented | per-iteration counts: entropy pulls, varint reads, macro-path entries, copy bytes, literal bytes; report **pulls/output-byte** | Track 11 `G1` (25% bar) |
| **K3** | paper only | published `λ · c` arithmetic reconciled to one product | Track 11 `H5` — **do this before K2, it costs an afternoon** |
| **K4** | descriptor-RLE control, **byte-only** | RLE the match-descriptor stream of the frozen mode-15 parse; report the byte delta of `LOOP_EQ` against it | §4.1 structural falsifier |

**Corpus:** frozen held-out PE set (6 files, 5,540,960 B, 4 producers / 2 toolchains — Track 09's
binding split) **plus** `generated.{json,jsonl,log,sqlite}` and the frozen Silesia timing panel.
**Calibration:** `anvil.exe`/`anvil_bench.exe` only. **No held-out file may tune a constant.**

**Mandatory controls (any FAIL ⇒ VOID):** byte-identity of `--fli=off` / feature-off against the
frozen binary; `generated.repeat.jsonl` no-regression; `random.bin` unchanged; byte-exact roundtrip
on every file in every arm; forced-registry coverage for any new mode ID; seed/CI pinned; every raw
counter row retained (no outlier deletion).

### 10.1 Frozen thresholds

**SLX-REF / H2 (`C1` only):**

| ID | GO | KILL |
|---|---|---|
| **T1 — `μ`** | `μ ≥ 8.4%` on **≥4/6** held-out PEs | `μ < 8.4%` on ≥3/6 ⇒ **KILL on rate**, mechanism dead |
| **T2 — coverage reality** | `C2`-eligible bytes `> 0` on **≥2/6** PEs | `= 0` on all ⇒ the only non-occupied residue is unreachable ⇒ **novelty KILL, keep the engineering measurement** |
| **T3 — dual bar** | `A3 < A4` (global BCJ, same backend) **and** `A3 < A5` on **every** held-out cell | any cell where a same-transform reference wins ⇒ **FRONT-GAP**, strike as a crossing, label `{engineering}` |

**FLI:**

| ID | GO | KILL |
|---|---|---|
| **T4 — decode thesis (`G1`)** | per-iteration entropy+dispatch ≥ **25%** of per-iteration decode cost | < 25% ⇒ **KILL the decode leg**; per Track 11's own condition this is KILL, not "ratio-only adopt" |
| **T5 — `LOOP_EQ` redundancy** | descriptor-RLE control (K4) captures **< 50%** of `LOOP_EQ`'s byte win | ≥ 50% ⇒ `LOOP_EQ` is **redundant with a single match** ⇒ KILL the mechanism; only `LOOP_ARITH` may proceed |
| **T6 — `J` calibration** | `λ·c ≥ 1e-3` bytes per suppressed symbol | `< 1e-3` ⇒ `J` cannot make the decision ⇒ **HOLD all decode-cost-gated tracks (05, 06, 11, 18)** until resolved |

**Novelty gate (pre-registered, and already decided):** `G0` is **NO-GO** for Track 11 — S1, S2
anticipated; S4's genus occupied. Track 09's `C1`-only scope is `{engineering}` by §3. **No data
from `killteam-counters-r1` can upgrade a claim class.** Thresholds govern *worth building*, not
*novelty*. They are frozen now and will not be moved after data (master brief item 5).

### 10.2 Non-negotiable labelling

R1 byte-only ⇒ the output is `BYTE-WIN` or nothing. **PR-3 forbids a `FRONT-CROSSING` from a
bytes-only oracle.** Any lane reporting a crossing from `killteam-counters-r1` is in breach of the
contract and the artifact is void.

---

## 11. Reconciliation with the paired constructive report — **COMPLETE**

Read in full: `docs/swarm-2026-10-02/20-priorart-killteam-space-bunny.md` (956 lines). **I did not
inherit its verdict.** Where it has better evidence I say so; where I have better or independent
reasoning I say so; where we agree I say so. There are **no remaining contradictory Track-20
verdicts** — the table in §0 has been rewritten to match this section.

### 11.1 Point-by-point

| # | Constructive finding | My position now | Basis for the change |
|---|---|---|---|
| **R1** | **S1 is not anticipated, but the clearance is a GAP, not a negative** | **CONCEDE.** I withdraw "S1 ANTICIPATED". My artifacts (VCDIFF `RUN`, bzip2 `RUNA`/`RUNB`, zstd FSE `RLE`) anticipate the *genus* but not the *LZ-token instance*, and the constructive reading of S1's scope is the better one. | §4.1. **I gained nothing by losing this: FLI's kill never depended on S1.** |
| **R2** | **S2 not anticipated as a mechanism; principle anticipated in grammar research** (arXiv 2607.24971 streaming grammar matrix engine; Sakamoto 2014 PMLR v34 naming streaming grammar decode as the next-generation framework; Cleary et al. random-access-without-materializing) | **AGREE, and my evidence is worse.** Their artifacts are `SESSION`-sourced; mine (Dynamo-family superinstruction/trace compaction) were `[STANDARD]`. I add that S2 is *also* non-obvious on composition grounds, which is a separate and stronger objection than anticipation. | §4 S2. Net: S2 = systems/unification only, exactly as they say. |
| **R3** | **S3 anticipated at the abstract level (ISLP)** | **AGREE, and I add a second independent kill they do not have.** ISLP anticipates the abstract mechanism; I additionally show the concrete LZ instantiation is *redundant* — `LOOP_EQ` ≡ one periodic match (§4.1). | §4.1. This is my strongest FLI contribution and it is **prior-art-independent**. |
| **R4** | **S4 anticipated by Brotli whole-file gating (`c/enc/encode.c`)**; residual is only "per-construct" | **AGREE that S4 must be dropped.** I go slightly further and for a different reason: even the per-construct instance is not claimable, because λ-weighted rate-vs-decoder-complexity is an occupied *genus* (decoder-complexity-aware RDO, `[FETCHED]` this session: WO2026008257A1; arXiv 2305.07678). An unclaimed *instance* of an occupied *genus* is novelty-by-difference, doctrine item 1. Their own conclusion — *"a claim that is both narrow and unmeasurable should not be carried"* — is the right one. | §4 S4. |
| **R5** | **H2 downgraded `LIVE-BUT-UNTESTED` → `THIN`; HOLD as a claim**, on `lzma_options_bcj::start_offset` + `Bra86.c` `pos` | **ACCEPT the evidence; UPGRADE my downgrade.** My interim was "PILOT the byte measurement only, claim class forced to `{engineering}`". Theirs is "HOLD as a claim; byte arithmetic is an engineering screen". The latter is stricter and better sourced. **My verdict moves to theirs.** | §3.1 addendum. |
| **R6** | **Track 16 generic routing CLOSED** on Brotli `block_splitter.go`, zstd `zstd_preSplit.h` + `targetCBlockSize`, RFC 8878 raw fallback | **CONCEDE — their evidence is better than mine.** My in-advance ruling was an obvious-combination argument; theirs is three shipped implementations with in-source quotes. Same outcome (`{engineering}`), better provenance. I add one independent reason: their own `H5` finding — *a λ-weighted cost objective the project cannot calibrate cannot be load-bearing* — kills the rank-1 residual too. | §5. |
| **R7** | **Their §2.2-equivalent: H2's byte prize ≈ ≤0.02% end-to-end** | **AGREE on conclusion, disagree on the arithmetic path.** My independent check (§2.2) found a **factor-1000 error** in Track 09 §3.1: `5,070 phrases × 1 bit = 634 B`, not `0.63 B`, and `0.0360%`, not `0.000036%`. Their "≤0.02%" and my "0.036%" are the same order and the same conclusion. **I stand by my arithmetic**; it does not change any verdict but it does change the standing claim that the A1–A4 ablation is "underpowered by construction" — it is **deterministically powered**. | §2.2. |
| **R8** | **Final leaning: PILOT** | **HOLD** (unchanged from my interim). **We differ on the label only, and I will not move it.** Their pilot is Phase-0 + A1–A5 byte-only; mine is the same work plus the `μ ≥ 8.4%` counter and the `λ·c` arithmetic. The substantive content converges almost completely. The label difference is not worth a coordinator ruling and I record it as **immaterial**. | §12. |

### 11.2 The one thing I add that they do not have

**FLI's `LOOP_EQ` is exactly one LZ77 match.** Their report carries FLI to "LIVE on S2 only". My §4.1
shows the dominant expected case (`LOOP_EQ`, constant `L₀`, constant `D₀`, run of length `k`) is
**character-identical to a single periodic match `(D₀, k·L₀)`**, a construct the baseline format
already contains, already permits (`D₀ < L₀` overlapping copy, modes 11–15), and already charges
once. Consequences they do not draw:

1. FLI's remaining separator is **not** "one weak claim" — it is **one weak claim about the *minority*
   opcode** (`LOOP_ARITH`), plus a token-stream RLE device for the majority. That is thinner than
   "LIVE on S2 only".
2. Their `A4` control is a **RePair grammar** control. The decisive control is **descriptor-RLE on
   the same parse** — if RLE of the match-descriptor stream captures `LOOP_EQ`'s byte win, the
   mechanism is dead with no build required. This is arm `T5`/`K4` in my §10.
3. It makes FLI **prior-art-independent** to kill, which matters because four of its separators now
   rest on searches that are, correctly, recorded as **gaps**.

### 11.3 What we agree on, stated once, so the coordinator has one place to look

**Exactly one neighborhood survives: reference-local, geometry-derived, *exact*, zero-bit-parameter
transform copies in executable code (TCOPY/PNRA/H2).** Its only strong axis is **parameter
provenance**; its only sharp axis is **exactness**; its placement axis is now occupied by
`start_offset`; its topology axis is occupied by BCJ. Both lanes independently conclude that the
**mass** of that neighborhood's prize is small (§2.2 here; their §7.2). **A defensible claim there
would be a claim about a ~0.04% byte effect** — which is not a mechanism contribution, and both
lanes now say so.

---

## 12. Final ruling

# **HOLD**

**Ruling detail, per mechanism (reconciled against the paired report, §11):**

- **FLI novelty — KILL.** Two independent reasons, and they do not depend on each other:
  (a) **prior art**: S3 anticipated at the abstract level (ISLP); S4's genus occupied
  (decoder-complexity RDO) and its instance anticipated at whole-file granularity (Brotli
  `encode.c`); S2 at best a systems/unification claim; S1's *genus* occupied though its LZ-token
  instance survives; (b) **definitional redundancy**: `LOOP_EQ` ≡ one periodic LZ match
  `(D₀, k·L₀)`, a construct the baseline already has. **Only `LOOP_ARITH` and token-descriptor RLE
  survive, and both are engineering.** Track 11's own `G0` no longer needs S1 to fire.
- **FLI engineering — HOLD**, bounded by two counters (`T4` per-iteration overhead ≥25%; `T5`
  descriptor-RLE control must capture <50%).
- **SLX-REF / H2 / TCOPY / PNRA novelty — KILL as a mechanism claim; HOLD the byte arithmetic as an
  engineering screen.** `C1`-only is localized BCJ with identical transform semantics, identical
  zero-bit parameter, and **zero charged side information in both cases** — and the constructive
  lane's `start_offset` establishes the position-origin parameter as a shipped, intended feature.
  The surviving neighborhood is real but its prize is ~0.04%, which is not a mechanism contribution.
- **SLX-REF `C2` — HOLD.** It is the only structurally unoccupied residue (no global filter can
  reach an anchor that does not exist to a whole-file pass), and it is blocked on a corpus that
  does not exist, overlapping Courgette. **This is the single highest-value unbuilt item in the
  program**, and the block is a corpus-lock problem, not a mechanism problem.
- **CDR (track 16) — KILL on novelty; `{engineering}` on arrival.** Closed by shipped Brotli
  block-splitting, zstd splitters, RFC 8878 raw fallback, and by my own obvious-combination finding.
- **Track 19 VCA — I support the PILOT.** Format-relevant, its risk is real, every mechanism lane
  inherits the obligation it names.
- **CAM (track 15) — KILL on novelty as a standalone mechanism; see
  `SYNTH-NOVELTY-FLEDGE.md` §4.** In a static-table rANS format, transmitting the model *is*
  transmitting a prior; the narrow residue (transmit a *delta* on the previous block's model) is a
  table-delta engineering mechanism.
- **Project-level — the binding constraint is not novelty and not ratio.** It is the **decode plane
  and the memory plane**, and both are structural: `0 FRONT-CROSSING` on every grid and reference
  class, `14.597×`/`23.946×` decode multiples measured, ~10.3 GiB inherited peak RSS, decoder size
  unmeasured and structurally adverse. **A new opcode class cannot fix a frontier gap that is
  10–24× on decode.** Any lane that proposes a byte-only mechanism as a *crossing* route is
  mis-specified, and this project has now recorded that error three separate times.

**One cheapest decisive next experiment/check:**

> **`killteam-counters-r1`, arms K3 → K1 → K2 → K4, one remote byte/counter-only CI job, zero timing.**
> K3 (publish `λ·c`) and K1 (measure `μ`) are each an afternoon and each decides a track: K1 decides
> SLX-REF against an **`μ ≥ 8.4%` break-even** that requires building nothing; K3 decides whether
> *any* decode-cost-gated representation on tracks 05/06/11/18 is optimizing a real quantity at all.
> K2 and K4 follow only if those pass. No local corpus benchmark, no production edit, no commit.

**Explicit falsification criteria for my own ruling:** if the paired report produces a fetched,
claim-level citation that **none** of VCDIFF `RUN`, bzip2 `RUNA`/`RUNB`, zstd FSE `RLE`, FELICS
zero-run, trace-compaction superinstruction fusion, or decoder-complexity RDO anticipates S1–S4 —
*and* the constructive lane supplies a proof that `LOOP_EQ` is not expressible as a single periodic
match — then my §4 collapses and FLI returns to a live novelty discussion. I have found no such
citation, and the `LOOP_EQ` equivalence is a definitional identity that no citation can overturn.

---

*Fledge Alpha Free, independent adversarial reviewer, track 20. Interim artifact — §11 pending the
paired report. This is a prior-art classification record and an experiment-design review, **not
legal advice**; FTO counsel review remains recommended before any commercial claim. Search gaps
recorded honestly: `[STANDARD]` items (bzip2 `RUNA`/`RUNB`, zstd FSE `RLE`, JPEG-LS/CALIC, DynamoRIO,
CDC/learned-index routing, differentiable compression) were **not** re-fetched this session and
should get one confirmation pass before any of them is cited as a citation of record. Espacenet and
Lens were not queried by me either; the patent-landscape reliance is inherited from the local
six-pass record, which I audited for internal consistency and did not re-verify.*