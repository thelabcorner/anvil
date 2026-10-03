# Track 20 — Novelty / Prior-Art Kill Team — Space Bunny Free

**Track:** 20-priorart-killteam · **Role:** constructive inventor **and** kill team
**Date:** 2026-10-02 · **Tree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty
**Mandate:** build a structured map from ANVIL's *surviving* candidate mechanisms to
literature, patents, and established codecs; identify exact claim separators, likely
novelty failures, and experiments that decisively distinguish mechanism novelty from
engineering.

**Status of this document:** interim checkpoint. **No measurement was performed by this
agent.** No local benchmark, no local corpus run, no local fuzzing, no build of production
source. No existing file was modified. One new file created (this one). No
`prototypes/` directory was created — see §12 for why that is deliberate at this stage.

**Evidence labels used throughout:** `MEASURED` = an in-repo recorded measurement, with
path. `RECORDED` = an in-repo recorded *audit/conclusion/decision*, path given.
`PROJECTION` = my arithmetic or design estimate, not measured. `SESSION` = observed by me
in this session (web search excerpts). `GAP` = an acknowledged coverage hole, never a
negative.

---

## 0. Verdict in one paragraph, and the current leaning

**Every mechanism ANVIL currently lists as surviving is either (a) already-adopted prior
art, (b) a systems/unification claim with a thin and probably indefensible separator, or
(c) exactly one narrow neighborhood — reference-local, geometry-derived, exact transform
parameters at copy level — where the *prior-art audit was actually executed with its
binding condition discharged* and returned NOT-FOUND.** That neighborhood is TCOPY/PNRA/H2.
Its decisive risk is not the literature: it is that **H2's decoder kernel is, statement for
statement, a BCJ/E8-E9 normalization pass restricted to a source span**, and if that
equivalence holds then the "reference-local placement" separator collapses and the entire
surviving novelty space becomes adopt-class. Nothing in my reading of the record rules
that equivalence out; nothing rules it in either. **Therefore the single decisive
experiment for this track is not a literature search — it is a byte-level
H2-vs-global-BCJ equivalence test with complete charged wire, plus the never-yet-run A1–A4
provenance ablation.**

**Leaning: PILOT** (byte-only remote Phase 0 coverage oracle + one job carrying the A1–A5
provenance ablation and the H2-vs-BCJ control), **not PROMOTE-TO-REMOTE** (no mechanism is
currently claim-supported), and **not HOLD** (the Phase-0 oracle is cheap, byte-only,
deterministic, and can return a decisive NO-GO in one CI job; and the cross-lane S1–S4
answer in §9 is itself actionable now).

For track 11 (FLI), my cross-lane answer after **four** searches is: **S3 is anticipated at
the abstract level** (ISLP, §9.2); **S1 is not anticipated but the clearance is a GAP, not a
negative**; **S2's *principle* is anticipated in grammar-compression research while its
*byte-stream* instantiation is not** (§16.1); **S4 is anticipated by Brotli at whole-file
granularity** (§16.4). Net: **track 11's own G0 threshold ("any of S1/S2 anticipated ⇒ NO-GO")
is not tripped**, but S3 and S4 must be dropped from the claim, leaving S1+S2 as the entire
defensible surface — which is a *systems* claim, not a mechanism claim.

**Second-checkpoint revision (same day).** Two load-bearing kill questions were then run to
completion and are written up as claim charts in §16. **The second one changed my
recommendation for H2/M-8 from "PILOT a byte-only screen" to "HOLD as a claim": xz's
documented `lzma_options_bcj::start_offset` already ships the exact insight H2 rests on
(§16.2).** The remaining PILOT is now only the Phase-0 byte arithmetic, which is an
engineering screen, not a novelty screen.

---

## 1. Method and honesty about coverage

| Aspect | What I actually did |
|---|---|
| Sources read in-repo | `MASTER-BRIEF.md`, `CONTEXT.md` (system reminder), `priorart-tcopy-external.md` (6 passes, 693 lines), `gate-priorart-audit-i8.md`, `gate-ruling-i9-pr5-position-derived.md`, `RESEARCH_LEDGER.md` PART XV + PART XIV addendum A1–A21, `FRONTIER-RESET-2026-09-23.md`, `I10-BREAKTHROUGH-PROGRAM.md`, `I10-GROTLI-FRONTIER-ARCHITECTURE.md`, `I10-AUX-UNBWT-RESULTS.md`, `I10-GROTLI-G2-TYPED-PREREG.md` §20, `I10-G5-PAGED-DICTIONARY-PREREG.md` §19, `I10-G5A-LOCALITY-CONTROLS-PREREG.md`, `I10-GROTLI-G5A-ORDERING-ATTRIBUTION-PREREG.md`, `research-agenda.md` §0–§5, `audit-2026-09-07/06-do-not-reburn.md`, `FORMAT.md` (modes 11–17, HOTOP/SHAPE/TCOPY/TOPOLOGY, backend registry, integrity), `docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md` |
| External searches this session | **4 web queries** (see §9.4 for the exact list and what each returned) |
| Patent databases | **Not queried this session.** Rely entirely on `docs/priorart-tcopy-external.md` passes 1–6. |
| Espacenet / lens.org | **Still never queried** (`gate-priorart-audit-i8.md` §5 records this as a standing gap) |
| ACM / IEEE full text | **Not queried this session** |
| FTO opinion | **None.** This document is a literature/patent classification record, **not legal advice** (`priorart-tcopy-external.md` §"FTO note"; `gate-priorart-audit-i8.md` §5) |
| Standard I apply | A search that returns nothing is a **GAP**, never a clearance (`gate-priorart-audit-i8.md` §5; `gate-priorart-audit-i8.md` §3 "the 'none found' statement is a gap, not a negative") |

**The single most important structural fact I inherit, and which organizes every verdict
below:** the project's own prior-art audit established that the *genus* — "encode a value
relative to already-reconstructed context" — is **never claimable**, and that what ANVIL
can own is only a *species*-level property: **where the transform lives, where its
parameter comes from, and how exactness is restored**
(`docs/gate-priorart-audit-i8.md` §0, §4). That audit then found, in the project's own
words, that the project "has twice built the expeditious form and called it progress": the
**transmitted** parameter works and is prior art; the **derived** parameter is defensible
and failed (`gate-priorart-audit-i8.md` §4; decisive instance:
`gate-ruling-i9-pr5-position-derived.md` P-7, transmitted 12,921 B < derived 12,936 B).

---

## 2. The surviving-mechanism inventory (what is actually still alive)

Sources for status: MASTER-BRIEF doctrine items 12–15; ledger PART XV "Portfolio decisions
after reconciliation"; `I10-GROTLI-FRONTIER-ARCHITECTURE.md` §11; `FRONTIER-RESET` §10/§11.

| ID | Mechanism | Status | Where recorded |
|---|---|---|---|
| M-1 | Auxiliary-index inverse BWT | **CLOSED / ADOPT**; real 1.364×/1.795×/2.339× whole-decode, +8,083 B charged | `docs/I10-AUX-UNBWT-RESULTS.md` §11 |
| M-2 | DEFLATE reconstruction (P4.1) | **adopt-class, NO novelty**; recovered 1,362,177 B on `mozilla` prototype | ledger PART XIV A2; `docs/gate-ruling-i9-p41-deflate.md` |
| M-3 | G5A-informed bounded finalist planner | **PROMOTE to remote pilot** (engineering) | ledger PART XV |
| M-4 | G5D paged base+overlay exact dictionary | **PROMOTE to remote pilot**; "claims no primitive novelty" | `docs/I10-G5-PAGED-DICTIONARY-PREREG.md` §19 |
| M-5 | Bounded BWT subblocking | **CONTINUE cheaply**; sweep closed, default unchanged | `docs/I10-AUX-UNBWT-RESULTS.md` §11 tail |
| M-6 | New held-out families | **CONTINUE**; protocol work | `docs/I10-CORPUS-LOCK-PROTOCOL.md` |
| M-7 | Decoder/backend co-design, SIMD, entropy co-design | **CONTINUE cheaply** | ledger PART XV |
| M-8 | **H2 decoder-derived relocation transform copy** | **strong candidate; audit required** | `docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H2 |
| M-9 | **H1 IGS-IR** | hypothesis, novelty not established | `docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H1 |
| M-10 | **H3 restricted iterated span generation** | anatomy first | `docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H3 |
| M-11 | **H4 innovation-channel factoring** | architecture hypothesis | `docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H4 |
| M-12 | **H5 integer-metadata recoding** | "enabling infrastructure, no novelty claim" | `docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H5 |
| M-13 | **Exact TCOPY / PNRA** (the narrow neighborhood) | the only audit-discharged claim | `docs/priorart-tcopy-external.md` pass 5; `FORMAT.md` mode 14 |
| M-14 | **FLI fused loop instruction** (track 11) | PILOT, conditional on this track's S1–S4 | `docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md` §7 |
| M-15 | **Derived-mask anatomy oracle** (I10-3) | queued, never run | `docs/I10-BREAKTHROUGH-PROGRAM.md` §10 I10-3; `FRONTIER-RESET` §11 Phase C |
| M-16 | Correction-topology coding | **KILLED by measurement**: modal 17–23%, masks ~90% unique | `FORMAT.md` §TOPOLOGY |
| M-17 | Shape-conditioned displacement P(d\|s) | validated ratio, narrow, ablation outstanding | `research-agenda.md` R4/FLAG-D |
| M-18 | G5A column ordering | ORDER-MATERIAL, COLUMN-DOMINANT, **V1 adverse**; PORDER killed | ledger PART XV G5A + portfolio decisions |

Note that **M-1, M-2, M-3, M-4, M-5, M-7, M-12, M-16 are already settled** and my job on
them is one line each: they are adopt-class or killed. The kill team's value is concentrated
on **M-8 / M-9 / M-10 / M-11 / M-13 / M-14 / M-15**, and on the *separators* that decide
whether any of them can be promoted.

---

## 3. The claim-separator framework

`gate-priorart-audit-i8.md` gives three axes. Five years of LZ practice since 1977 give two
more. I use five, and I state for each exactly what it does and does **not** buy.

### Axis 1 — **Where does the transform live?**
`RECORDED`: per-reference ("inside the reference") is a real placement distinction from a
global pre-LZ filter (`gate-ruling-i9-pr5-position-derived.md` P-1). **But** the same ruling
records that placement alone lands the mechanism in the crowded **copy-with-edits** bucket
(VCDIFF RFC 3284, bsdiff, Zdelta, Zucchini, US 12,373,439, US 2024/0211132) where
placement is not novelty — `gate-priorart-audit-i8.md` §3 lists self-reference + exact COPY
+ corrections as **fully anticipated** by VCDIFF, and mask-only filings as **partly**
anticipated.

*Verdict: Axis 1 separates "not a BCJ filter" and nothing more.*

### Axis 2 — **Where does the parameter come from?**
`RECORDED`: **transmitted = prior art, full stop** (parametric dictionary compression;
US 7,676,506; xz `--delta`; and the project's own twice-repeated error). **Derived with
zero transmitted bits = the only defensible side** (`gate-priorart-audit-i8.md` §4).

*Verdict: this is the project's single strongest separator and it has already been paid for
twice. It is load-bearing, not decorative.*

### Axis 3 — **How is exactness restored?**
`RECORDED`: G4-amended — a zero-bit derived parameter is defensible **only where the
transform is exact (identically zero residual)**; on statistical transforms the family has
**no defensible novelty position at all**, and the family is closed because residual entropy
grows like `log2(d)` with no estimator able to remove the slope
(`gate-ruling-i9-pr5-position-derived.md` P-2; ledger PART XIII §7 DNB-M2, measured
0.646–0.691 bits/doubling over d=4..1024).

A third category exists and is named: **bounded-nonzero-residual predictive basis** —
buildable as engineering, **not claimable** (`gate-ruling-i9-pr5-position-derived.md` P-3.2),
and *measured unnecessary* (P-7).

*Verdict: Axis 3 is the sharpest one in the framework, and it is the reason the surviving
neighborhood is exactly "exact transforms".*

### Axis 4 (mine) — **How many parameters, and by what rule are per-iteration operands derived?**
This axis is not in the existing audits and it is what determines whether a *recurrence*
mechanism is a new species or a re-parameterisation of Delta-of-delta. Discriminators:

* **1 scalar, from a cache of transmitted values** → LZMA rep0–rep3, zstd repeat offsets,
  Brotli distance cache. Occupied.
* **k scalars, transmitted once per block** → prior art (`gate-priorart-audit-i8.md` §4).
* **k scalars, per-iteration values defined by an exact recurrence over i, with zero
  per-iteration transmitted symbols** → this is the only sub-slot with a live question, and
  §9.2 shows it is **already occupied in the abstract** by ISLP.
* **statistical recurrence (DPCM / DoD / Gorilla)** → occupied, and non-exact → Axis 3 kills
  it for claims anyway.

*Verdict: Axis 4 is what I add. It converts "recurrence" from a claim into a
parameterization, and it is the axis on which FLI's S3 falls.*

### Axis 5 (mine) — **Where and when does the normalization kernel run?**
This is the axis H2 lives or dies on, and it is the axis the existing audits do not name.

* **Filter-local**: one pass over the whole stream before/during LZ, normalizing all
  E8/E9 (or all detected references) regardless of whether any copy references them.
  BCJ/E8-E9 (`priorart-tcopy-external.md` pass 2/4), ZPAQ/PCOMP, Philips US 5,787,302,
  IBM/ST US 6,564,314. Public domain (LZMA SDK 4.62, 2008).
* **Disassembly-local**: parse the file into an assembly-like representation with symbolic
  pointer targets, then reconstruct exact bytes. Courgette. Recorded as
  "CLOSE-BUT-DIFFERENT" and as a real boundary risk
  (`priorart-tcopy-external.md` pass 4; `FRONTIER-RESET` §6.4;
  `docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H2 prior-art boundary).
* **Reference-local**: run the *same* normalization kernel, but only over the bytes that a
  specific copy instruction is about to reference, and only for the fields that the copy is
  going to adjust.

**The hard truth I must state plainly:** the third bullet is not a different *algorithm*
from the first. It is the first, applied to a subset. The only defensible differences are
(i) the transform parameter is derived from **copy geometry** (source↔destination offset)
rather than from absolute file position, and (ii) the kernel is executed once **per
reference**, so a byte that is never referenced is never normalized. Whether (i)+(ii) is
enough to survive an obviousness attack is **the open question of this entire project**, and
no in-repo document has tested it.

*Verdict: Axis 5 is where the whole surviving novelty space reduces to a single testable
comparison, and it has never been run.*

---

## 4. The structured map — surviving mechanism → prior art → separator → likely failure

Legend for **Risk of novelty failure**: `CLOSED` (no claim survives), `THIN` (a systems /
unification claim only), `LIVE-NARROW` (one specific separator carries everything),
`LIVE-BUT-UNTESTED` (separator identified, no measurement).

| ID | Prior art / lineage (exact refs) | The only separator that could carry a claim | Likely novelty failure | Risk |
|---|---|---|---|---|
| **M-13** TCOPY / PNRA (exact, implicit Δ=−d) | VCDIFF RFC 3284 (self-ref COPY + corrections, standardized 2002); Zdelta; bsdiff; Zucchini (copy + rel32 correction, two-file); GenCompress; RLZAP; BCJ/E8-E9 (global pre-LZ); Philips US 5,787,302; Microsoft two-file lineage US 6,466,999 / 7,509,636 / 7,676,506; Apple chained fixups; **Intel US 7,111,148 / 7,010,665 / 7,617,382 read in full, EXPIRED, CPU μop-storage compaction, no LZ, no distance** (`docs/priorart-tcopy-external.md` pass 5, the binding item) | **Axis 2 + Axis 3 together**: a *single-file, self-referential* copy whose additive transform parameter is derived **implicitly from the backreference distance** (Δ=−d, zero transmitted bits) and whose residual is **identically zero** | Axis 5: the kernel is a restricted BCJ pass. Also: the audit is *literature classification*, not FTO; Espacenet/lens never queried; Apple chained-fixups patent number still unconfirmed; pass 4→5 corrected two citations, so earlier-pass descriptions carry known error bars | **LIVE-NARROW** — the only audit actually discharged |
| **M-8** H2 decoder-derived relocation transform copy (reference-local, mask-free) | Same set as M-13, plus **Courgette** as the named boundary risk (`docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H2 explicitly forbids claiming: discovering relocation-bearing instructions; converting addresses to a normalized/symbolic representation; reconstructing exact machine-code from normalized pointer metadata) | **Axis 5 only.** Everything else in H2 is already forbidden by its own prereg | If the E8/E9 site identification requires an instruction-length disassembly, H2 *is* Courgette's disassembler + BCJ, and Courgette is on the record | **LIVE-BUT-UNTESTED** |
| **M-9** H1 IGS-IR | OpenZL (graph of reversible ops + resolved frame + universal decoder); grammar/SLP/ISLP family; generalized deduplication (arXiv 1901.02720); bidirectional macro schemes; zdelta/bsdiff; Newton/Zucchini; LZRR | "the interaction of event-driven invariant discovery + typed span-generation relations + a hardware-oriented bounded-depth schedule" — stated in `docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H1 as the hypothesis | A four-way *conjunction* of known techniques is the definition of an obviousness target. `FRONTIER-RESET` §6.7 already concedes "compression is a graph/program of transforms is already established" | **THIN** (systems/unification only) |
| **M-10** H3 restricted iterated span generation | **ISLP — iterated straight-line programs**, iteration rules `A → ∏_{i=k1}^{k2} B_i^{c1}···B_t^{ct}` with exponents linear in i (`users.dcc.uchile.cl/~gnavarro/ps/latin24.2.pdf`, SESSION excerpt; family already in the repo's own permanent set, `FRONTIER-RESET` §13); L-systems; morphisms NU-systems (Navarro & Urbina 2025, `FRONTIER-RESET` §4.1); RePair loop rules; XMill repeated parse patterns | **Axis 5** again: fused, non-materialized, non-recursive, over an **LZ token stream** rather than a grammar | The abstract mechanism — counted loop, per-iteration operands from an exact recurrence, zero per-iteration symbols — is published. See §9.2 | **LIVE-BUT-UNTESTED**, and the abstract claim is already occupied |
| **M-11** H4 innovation-channel factoring | STC (digit-run extraction + side streams, arXiv 2606.03570, reported ~2.63 MB on enwik9); ALP; Gorilla; Parquet byte-stream-split; columnar codecs | **Detector generality across field classes** vs one hand-picked class | `FRONTIER-RESET` §4.9 already names the principle as a *systems pattern*. "Isolate the small irregular channel" is stated by STC, ALP, Gorilla | **CLOSED** as a principle claim; only the detection machinery is engineering |
| **M-12** H5 integer-metadata recoding | FOR/bitpacking/PFor; patched FOR; delta/delta-of-delta; Stream VByte (arXiv 1709.08990); TurboPFor; ALP | — | Its own prereg says **no novelty claim** | **CLOSED** |
| **M-14** FLI loop instruction (track 11) | ISLP (Axis 4 kill, §9.2); RePair/XMill counted expansion; LZMA rep0–rep3; Brotli distance cache; LZO's documented cross-instruction `state` variable (SESSION); superinstructions (interpreter literature); Brevis `repeat/scan` (operator-reported, **unverified**) | **S1** (suppress per-iteration symbols) and **S2** (fused, no materialization) | S3 occupied at abstract level; S1/S2 clearance is a GAP; Exp. Y in this repo already **measured** that materializing the grammar loses 8–16% decode | **LIVE on S2 only** |
| **M-17** P(d\|s) | Brotli distance context maps (RFC 7932); LZMA match-state distance coding; zstd repcodes | Nothing unless it beats an *equivalent-size generic context map* end-to-end | `research-agenda.md` FLAG-D already states the failure mode: "otherwise it is a renamed context map and fails the gate". The context-clustering precedent is this project's own public wound — Brotli's RFC 7932 context map, demoted to enabling infrastructure (`docs/CONTEXT.md`, `gate-priorart-audit-i8.md` header) | **CLOSED unless the named ablation passes** |
| **M-16** correction-topology coding | VCDIFF ADD/instructions; zdelta per-COPY mismatch lists; bsdiff flat add-array; masking filings US 12,373,439 / US 2024/0211132 | Only "entropy-code the correction **topology**" was ever a NOT-FOUND element | **Killed by measurement**: masks ~90% unique, modal 17–23% (not 86.5%), mode 13 lost bytes | **CLOSED** |
| **M-4** G5D paged base+overlay exact dictionary | Parquet column-chunk dictionaries; Brotli static dictionary; Zstd trained/external dictionaries; local/page dictionaries; FSST; front coding; BPE | — | Its own prereg: "claims no primitive novelty"; lists DataCortex, CLP, LogPrism, BtrBlocks, FastLanes, ALP, FSST, Parquet, Brotli as close lineage (`docs/I10-G5-PAGED-DICTIONARY-PREREG.md` §19) | **CLOSED** (adopt-class) |
| **M-3** G5A-informed finalist planner | OpenZL offline plan search; LogPrism; Grotli/DataCortex; Brotli block splitting; **G4 is `NO-GO-G4`** and its speed column is withdrawn as `INVALID_SPEED_ACCOUNTING` | — | Planning is an implementation architecture. G4 already measured that cheap proxies miss the q11 carrier choices (ledger PART XV) | **CLOSED** (engineering) |
| **M-2** DEFLATE reconstruction | preflate, preflate-rs, reflate, grittibanzli, container recompression (ledger PART XIV A2) | — | Recorded as "**NOVELTY: NO** … adopt-class prior-art-deployed infrastructure" | **CLOSED** |
| **M-1** aux-index BWT | libsais `unbwt_aux`; index-based LF-walking is standard BWT engineering | — | "adopt-class decoder engineering; no novelty claim" (`docs/I10-AUX-UNBWT-RESULTS.md` header) | **CLOSED** |
| **M-5** BWT subblocking | partition-based BWT, libsais subblock framing | — | Sweep closed, no default change | **CLOSED** |
| **M-18** G5A column ordering | FastLanes, white-box compression, OpenZL, Corra, LeCo, BtrBlocks, ALP (ledger PART XV portfolio decisions) | — | **PORDER killed** as a standalone graph/layout novelty; G5A is V1-adverse | **CLOSED** |

**The map in one sentence: of eighteen surviving items, exactly one (M-13) has an
audit-discharged claim, one (M-8) has an identified-but-untested separator, one (M-14)
survives on a single separator, one (M-10) has its abstract claim already occupied, and the
other fourteen are adopt-class, engineering, or killed.**

---

## 5. Likely novelty failures — ranked by how much they hurt

| # | Failure | Severity | Why it is likely | Can it be beaten? |
|---|---|---|---|---|
| **F1** | **H2 ≡ BCJ restricted to a source span** | **fatal to the whole surviving space** | The decode action is identical; only the trigger and the parameter source differ. Courgette already normalizes with a disassembler; BCJ is public domain | Only by proving the *placement* difference is economically load-bearing (i.e. filter-local normalization is worse on the same backend) **and** that the parameter is geometry-derived |
| **F2** | Recurrence/loop mechanisms are re-parameterizations of ISLP / Delta-of-delta | high | §9.2; `gate-priorart-audit-i8.md` §1's "Gorilla/Parquet with a different jacket" pattern | Only by keeping the claim at the *execution/unification* level and never at the parameter level |
| **F3** | Any proposed mechanism is a **conjunction of known techniques** | high | OpenZL owns "graph of transforms + resolved frame"; grammar owns "program + loop"; LZ owns "reference"; this project owns none of the parts | Only a systems claim, and it needs the *system* to be measurably Pareto-positive, not just novel-sounding |
| **F4** | Reference is too thin | high | `gate-priorart-audit-i8.md` §5: arXiv rate-limited, ACM/IEEE not queried, "no post-2024 general-purpose codec" is a **gap**. Espacenet/lens never queried in 6 passes | Only by paying for a real database pass; my 4 searches do not move this |
| **F5** | The decisive claim has **never once been ablated** | high | `gate-priorart-audit-i8.md` §6 action 5: the A1–A4 provenance ablation "has never once been executed in this project" — outstanding since I2-5 | **Yes. This is the single cheapest high-value action in the whole track, and it is remote-byte-only** |
| **F6** | Positive results get reported as frontier wins | high, procedural | MASTER-BRIEF doctrine 2/6; the dual-bar rule (`gate-priorart-audit-i8.md` §1); `{synthetic}`/`{engineering}` labelling | Yes — procedural, and must be enforced in the write-up |
| **F7** | **Ratio-only novelty**: a mechanism wins bytes and nothing else | high | MASTER-BRIEF item 12 retires wire-invisible decode work; item 14 demands "mechanism-level novelty beyond merely wrapping Brotli" | Only with a paired decode measurement in the same job |

---

## 6. The falsifiable mechanism I advance (constructive half of the role)

I do not advance a new compression mechanism. A kill team's honest constructive output is a
**falsifiable claim about the claim-space** plus the experiment that decides it. Here it is.

### Claim **C-Σ** (falsifiable, cheap, and decisive)

> **C-Σ.** If the A1–A5 provenance ablation is executed with complete charged wire against
> the same backend, then for every real-corpus cell in the frozen suite, the ranking
> `A5 (reference-local derived-mask) ≤ A4 (global BCJ + same backend)` will hold; i.e. the
> entire measurable value of the surviving "reference-local" novelty reduces to what a
> **global pre-LZ normalization pass** already achieves.

**Why this is the right thing to bet on.** It is the only hypothesis in the project's entire
document set that, if **true**, closes the last novelty claim; and if **false**, it is the
first *quantitative* evidence that placement carries independent value — which would be the
project's first genuine mechanism-level result in three iterations. Either outcome is
worth one CI job. There is no third outcome where the job is wasted.

**Falsification condition.** C-Σ is **falsified** if, on at least one frozen executable cell,
A5 is **strictly smaller in complete charged bytes** than A4 by ≥ 1.0% at equal or better
decode cycles. Falsification is a GO for the claim; survival is a KILL for the claim.

**Scoping honesty.** C-Σ is about the *executable* lane only. It says nothing about M-14/M-10
(byte-stream recurrence) and nothing about M-11/M-9. Those have their own separators
(§9) and their own verdicts.

### The mechanism C-Σ actually measures (H2, precisely specified enough to charge)

```
type-3 transformed copy, DE-MASKED form:
  operands:  type byte (0..3), literal-run len, match len, match dist
  semantics: copy dist/len from prior output (periodic if dist < len);
             then, for every 4-aligned window w of the reference such that
               target32[w] == source32[w] - dist,
             add dist to target32[w]  (equivalently Δ = -dist, implicit, 0 bits)
  transform sites: DERIVED at decode time from the copied source bytes by an
                   E8/E9 rel32 site scanner, NOT transmitted
  streams:     7 (mode 14's 8, minus the transform-mask stream)
```

**AUDIT-7 line-by-line** (the condition from `gate-ruling-i9-pr5-position-derived.md` P-3.4,
satisfied line by line):

| Input to the derivation | Available before use? | Transmitted bits |
|---|---|---|
| `dist` | yes — decoded immediately before the copy executes | 0 (it was already the operand) |
| source span `[p-dist, p-dist+len)` | yes — fully reconstructed | 0 |
| alignment/field geometry (4-aligned windows, `field_end ≤ dist`) | yes — deterministic from `len`, `dist` | 0 |
| E8/E9 site set | **the open question** — see below | **0 if derived by scanning; N bytes if transmitted** |
| `+dist` addend | yes — it is `dist` | 0 |

**The honest hole in that table, and it is the crux of F1.** To know *which* 4-aligned
windows are E8/E9 **opcode positions** rather than arbitrary bytes that happen to hold 0xE8,
the decoder needs either (a) the positions **transmitted** (defeats the mechanism), or
(b) an **instruction-length decoder** for x86 — which is exactly Courgette's primitive
disassembler, exactly what `docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H2 forbids claiming, and
exactly the kernel whose cost must be charged against 1.0–1.5 ns/B. A third option,
(c) **scan all bytes equal to 0xE8/0xE9 at 4-aligned positions** and apply Δ=−d
unconditionally where the value matches: this is *unsound* in general (it rewrites
non-operand words) and therefore **can only be used as an encoder-side filter whose
admission is verified by roundtrip**, which makes it a *parser* restriction, not a
decoder derivation. **Option (c) is the form I recommend the Phase-0 oracle to measure,
and it is the form whose novelty is weakest.** I record that plainly rather than discovering
it after a build.

---

## 7. Encoder / decoder state and full cost accounting

`FORMAT.md` is the normative source for mode 14's current wire; the deltas below are mine.

### 7.1 State delta (decoder)

| Item | Mode 14 (current, `FORMAT.md` §TCOPY) | H2 de-masked (proposed) | Δ |
|---|---|---|---|
| Stream count | **8** | **7** (transform-mask stream removed) | −1 stream header + its length varint |
| Per-block constant | +1 stream id/len varint for the mask stream | 0 | **−2 to −3 B/block** (PROJECTION) |
| Per type-3 phrase, len L | mask = `4·⌈L/128⌉` B (one u32 per 32 four-byte windows) | 0 | **−4 B per 128 phrase bytes = −3.125% of phrase bytes** |
| New decoder state | none | site-scanner state | **0 B if stateless over the source span; O(1) if a length-disassembler keeps instruction-boundary state (≤ ~64 B)** — PROJECTION, both unmeasured |
| Shape/displacement state | shared with mode 12 (`num_states ∈ {1,28}`) | unchanged | 0 |
| Output buffer | unchanged | unchanged | 0 |
| New amplification class | none | **none** — worst case is the phrase's own declared `len ≤ 65536` (`FORMAT.md` §TCOPY strictness) | 0 |
| Recursion | impossible | impossible | 0 |

### 7.2 Full bytes accounting per type-3 phrase

Let `L` = phrase length, `c_f` = number of transform fields, `H` = entropy bits/byte of the
6 remaining coded streams.

```
mode 14 bytes  = streams(1..6,8) + 4·⌈L/128⌉  + c_f · residuals
H2     bytes   = streams(1..6)              + c_f · residuals  +  C_scan
Δ bytes       = −4·⌈L/128⌉ − (2..3 per block)  −  C_scan_bits
```

where `C_scan` is the *decoder-side* cost that must be repaid: either
(a) transmitted site list → `c_f·⌈log2(L/4)⌉` bits, which for `c_f ≈ 2..5` and `L ≈ 64..256`
is **~6–20 bits — i.e. H2-with-transmitted-sites is roughly byte-neutral to mode 14 and can
easily be worse**, or (b) a scanning kernel at ~0.3–1.5 ns per scanned byte of phrase.

**Byte verdict (PROJECTION, and this is the number that matters):** H2's entire byte
advantage is `4·⌈L/128⌉` per type-3 phrase = **3.125% of the transformed phrase bytes**, and
it is *negative* (a loss) under option (a). For context on scale, `FORMAT.md` §TCOPY
records the measured density: **519 transform fields in `anvil.exe` block 0** and TCOPY
beats plain sparse (mode 11) by only **~0.1–0.6% on executables**, while **exact-LZ MDL
(mode 10) still wins on those executables** — the greedy approximate parse, not the
transform, is the limiting factor. So the byte prize H2 removes is bounded by roughly the
mask's share of a mode-14 stream that is itself worth ≲0.6% over mode 11.

> **That is a ≤ ~0.02% end-to-end byte prize, against a decoder-side scan of the whole
> referenced text.** Unless the executable cell's type-3 phrase byte volume is much larger in
> the frozen corpus than in `anvil.exe` block 0, **H2 is economically dead as a byte win and
> must be justified (if at all) on decode-side locality only.**

This is the single most important number in this checkpoint and it is a **PROJECTION from a
measured density**, not a measurement of H2. It is why Phase 0 is a coverage/byte oracle
and not a mechanism build.

### 7.3 Full cycle accounting

| Item | Cost | Label |
|---|---|---|
| `anvil.exe` decode, frozen (MEASURED, `docs/CONTEXT.md` / ledger A7) | ~216 MB/s rANS → ~4.6 ns/B | MEASURED, host-directional |
| Whole-codec portfolio decode, Silesia (MEASURED, ledger PART XIV A1) | 9.435 MB/s vs brotli-q11 14.597× slower | MEASURED |
| Mode-14 type-3 phrase execution today | 1 entropy pull + dispatch + periodic copy + per-field 32-bit add | structural, from `FORMAT.md` §TCOPY |
| H2 added per phrase | E8/E9 site scan over `L` bytes + `c_f` adds | PROJECTION: 0.3–1.5 ns/B scanned |
| H2 net decode effect on the *scanned* bytes | **negative** (scan cost > saved mask entropy) | PROJECTION |
| H2 net decode effect on the *unsanned* bytes | positive only if fewer entropy streams must be initialized | PROJECTION, ≤ ~1 stream setup per block |
| Decoder code size | E8/E9+modrm+imm scanner ≈ 200–600 B of `.text`; a length-disassembler is substantially larger and must be charged against the ≤1 MiB decompressor bar (`FRONTIER-RESET` §4.10, AITDCC) | PROJECTION, unbounded risk |

### 7.4 Memory / RSS accounting

| Item | Δ | Label |
|---|---|---|
| Decode peak RSS, H2 vs mode 14 | **≈ 0** — no new allocation, no side table, scan is stateless or O(1) | PROJECTION |
| Context: Silesia aux peak RSS (MEASURED) | 248.4 MiB ANVIL vs 124.5 MiB Brotli; enwik8 598.9 vs 251.9 MiB (`docs/I10-AUX-UNBWT-RESULTS.md` §11) — H2 does not touch the BWT axis at all | MEASURED |
| Encoder cost | the site decision is **encoder-only** (the decoder derives or is transmitted sites); no new search | PROJECTION |

---

## 8. Algorithmic complexity

| Component | Time | Space | Note |
|---|---|---|---|
| A1–A5 provenance ablation, encoder | `Θ(n)` per arm, same match-finder, arm only changes the *admission/serialization* rule | `Θ(window)` per arm, same as baseline | no new asymptotics |
| A4 global-BCJ control | `Θ(n)` single pass | `Θ(1)` | BCJ is a one-pass filter |
| H2 decoder scan (option b) | `Θ(L)` per type-3 phrase, `Θ(n)` total | `Θ(1)` | strictly linear, no back-edge |
| H2 decoder scan (option a, transmitted sites) | `Θ(c_f)` | `Θ(1)` | cheaper cycles, more bytes |
| Phase-0 coverage oracle | `Θ(n)` with a single forward E8/E9 event scan + hash index | `Θ(#E8/E9 occurrences)` encoder-only | the index never exists in the decoder |
| Frontier arbiter (`tools/pareto_front.py`) | as already implemented | — | harmonic input-byte aggregation is byte-exact and reproducible (ledger PART XV verification addendum) |

No candidate in this track introduces a superlinear term. The **only** superlinearity risk
in the whole family is a naive "for each start, extend the run" loop formulation (track 11
A8) — not applicable here.

---

## 9. Cross-lane answer to Track 11 — S1 / S2 / S3 / S4 anticipation

**Requested by track 11 (cross-lane coordination note, 2026-10-02).** Track 11 defines
FLI and hands this track four separators; it states that anticipation of **S1 or S2 is a
claim-kill** and asks for exact references. Here is the answer, with the coverage honesty
attached to each.

### 9.1 S1 — suppressing per-iteration entropy-coded symbols inside a counted loop
**Verdict: NOT ANTICIPATED in any general-purpose codec I reached; this is a GAP, not a
clearance.**

Nearest things I actually retrieved this session (`SESSION`, excerpt-level):

* **LZO (Linux kernel documented format)** — `kernel.org/doc/html/v5.14/staging/lzo.html`
  and `v6.13`, `v7.2-rc1`. Its instruction set carries a decoder `state` variable equal to
  "the number of literals copied by previous instructions", which is *consumed by the next
  instruction's operand decoding*. This is a real, shipped, zero-bit operand **inheritance
  across instructions**. It is **not** a loop and **not** per-iteration suppression, so it
  does not anticipate S1 — but it does occupy the "operands inherit from prior decoder state
  at zero bits" sub-slot that track 11's S3 argument leans on.
* **FastLZ** (`github.com/ariya/FastLZ`) and **`lzr`** (`github.com/ckmjreynolds/lzr`,
  4 sequence formats, explicit repeat/literal/extended-repeat) — fixed-length opcodes with
  length+distance; **no** token loop, **no** counted repeat of a previous instruction.
* **"Scroll" Rust LZ77 examples** — `LZ77Token(1,6,"")` repeats the previous character:
  that is DEFLATE-style `dist=1` periodicity inside one token, not a token loop.

**What would change this:** a database pass over *packer/interpreter* formats
(LZMA-family variants, proprietary console crunchers, dictionary-coder patents) is where an
anticipated S1 would live, and I did not query patents this session. `priorart-tcopy-external.md`
pass 3 explicitly flags "non-public applications within the 18-month publication blackout"
and "unindexed proprietary game-console/embedded packers (custom demoscene/console
crunchers)" as unsearched territory.

### 9.2 S3 — exact encode-verified **multi-parameter** recurrence operands
**Verdict: ANTICIPATED AT THE ABSTRACT LEVEL. This is the finding that matters for track 11.**

* **Iterated Straight-Line Programs (ISLP)** — Jeż, Navarro, Olivares, Urbina,
  *Iterated Straight-Line Programs*, LATIN 2024, `users.dcc.uchile.cl/~gnavarro/ps/latin24.2.pdf`.
  Retrieved this session (`SESSION`, abstract + rules + size statements). Its **iteration
  rules** have the form

  ```
  A → ∏_{i=k1}^{k2} B_i^{c1} ··· B_t^{ct},   1 ≤ k1 ≤ k2,  0 ≤ c1,…,ct ≤ d
  ```

  where the exponents `c_j` are **linear in i**. The decoder for such a rule is *exactly* a
  counted loop whose per-iteration operands are produced by an **exact arithmetic
  recurrence**, with **zero per-iteration transmitted symbols** and a **hard non-recursive,
  bounded expansion**. That is FLI's `LOOP_ARITH` semantics, stated as published grammar
  compression.

  **Precisely what this does and does not kill.** It kills the *abstract* claim
  "per-iteration operands supplied by an exact recurrence with zero per-iteration symbols
  and a bounded ceiling". It does **not** kill FLI's instantiation over an **LZ token
  stream** with a **fused, non-materializing, non-recursive consumer-side loop** — because
  an ISLP rule's decoder materializes/expands through the grammar. So: **S3 must be
  downgraded from "no shipped instance" to "anticipated in the abstract, unanticipated only
  in the LZ-token-stream instantiation."** Track 11 must not carry S3 in its claim, and
  per its own G0 wording ("any of S1/S2 anticipated ⇒ NO-GO") S3 is **not** by itself a
  claim-kill — but it removes the generality half of the claim.
* This family is **already in the repo's own permanent research set** —
  `docs/FRONTIER-RESET-2026-09-23.md` §4.1 (morphisms/NU-systems), §4.2 (GSLP,
  arXiv 2404.07057), §13 ("Iterated Straight-Line Programs" is listed there). Track 11's
  §2 lists "grammar with loops" as conceded lineage but cites RePair/XMill rather than
  ISLP specifically; this is the correction.

### 9.3 S2 — fused consumer-side loop execution without materialization
**Verdict: UNVERIFIED — and this is now load-bearing.** My two relevant queries returned
only LLVM `LoopFuse`/`LoopFusion` (compiler loop fusion — **irrelevant**, recorded so a later
reader does not re-find it) and no codec result.

Why it is load-bearing: §9.2 removes S3, so S2 is one of only two remaining separators for
FLI. And the risk is asymmetric: **grammar streaming decoders that generate output directly
without a full intermediate buffer are a real and long-studied class** (bounded-cache and
streaming SLP decoding), and track 11's own Exp. Y measured a closely-related distinction
on this host (materializing RePair/RLZ cost 8–16% decode while shrinking bytes 4–11%).
If any *shipped* coder executes a repeated construct fused, S2 falls and FLI is
adopt-class.

**Action for track 11 and for me (next, cheap):** one targeted search pass over
*streaming grammar decoding with bounded cache* and over *decompression without
materialization*, restricted to shipped implementations, before FLI's G0 is recorded.

### 9.4 S4 — token-level λ-weighted decode-cost gating
**Verdict: UNVERIFIED. I did not search this specifically; I am not going to fabricate a
clearance.** What I can say from the in-repo record: the project's own J objective is
**adopt-class systems work** and the repo records that its constants are **"right ORDER,
wrong GAPS"** (track 11 M11, Exp. AA) and that `λ·C_decode` is not reconciled between the
`FORMAT.md` cost units and the measured ns/B table (track 11 H5). A mechanism whose claim
includes a gating rule that the project cannot currently calibrate is **claiming an
unmeasured thing**; that is a project-integrity finding, not a prior-art finding, and it
stands regardless of what the literature says.

### 9.5 My four queries, verbatim, with what each returned
1. `lossless codec instruction repeat previous match length distance count opcode run of identical matches` → FastLZ format; Linux LZO kernel doc; Stanford LZ77 teaching page; Run-length encoding (Wikipedia); MS-RDPNSC RLE; IEEE "Improved LZ77". **No token-loop codec.**
2. `superinstruction fused loop decode repeated command without materialization compressed stream` → LLVM `LoopFuse.cpp`, `LoopFusion.html`. **No codec result.**
3. `iterated straight-line program μ-SLP parameter decoder loop affine exponents compression` → **ISLP (latin24.2.pdf)**, Ganardi et al. contracting SLPs, GSLP/SPiRe random access, Wikipedia SLP. **This is the S3 hit.**
4. `LZ77 codec instruction "repeat" previous match length and distance without re-encoding operand zero bits opcode` → FastLZ, LZO, `lzr`, `deflate` crate, Scroll examples, IEEE "Improved LZ77". **No token-loop codec; "zero-bit operand inheritance" occupied narrowly by LZO's `state`.**

---

## 10. Minimum prototype (byte-only; **not written in this checkpoint**, with the reason)

**What should exist** (all new files, nothing wired to production, per
`docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md` §8's precedent):

```
prototypes/swarm-2026-10-02/20-priorart-killteam/space-bunny/
  README.md              what it computes, how it builds REMOTE, thresholds, what it does NOT do
  demask_oracle.cpp      byte-only; E8/E9 event scan over frozen PE .text; measures, for
                         reference-local de-masked type-3 copies:
                           - eligible phrase bytes (4-aligned, field_end <= dist, L>=32)
                           - sites found by scan / sites actually transformable by Δ=-d
                           - mask bytes that would be REMOVED  = 4*ceil(L/128)
                           - bytes that would be ADDED if sites must be transmitted
                             (c_f * ceil(log2(L/4)) bits)
                           - ratio: removed / added   -> the H2 byte-prize number in §7.2
```

**Why I did not write it yet.** Three reasons, stated so the coordinator can overrule them:

1. It is **byte-only and analytic** — it reads a file and prints counts. That is not a
   "local corpus benchmark" under MASTER-BRIEF item 7, but its *first* execution must still
   be remote, and shipping unrun code whose only value is its printed table buys little
   before the thresholds exist.
2. **H2's numbers are already known to be bad (§7.2: ≤ ~0.02% end-to-end byte prize).** The
   prototype's most likely output is a NO-GO. I would rather record a *precisely scoped*
   negative than produce an artifact whose execution confirms it.
3. The **decision-relevant** prototype for this track is not an oracle — it is the
   **A1–A5 provenance ablation**, which requires wiring arms into a real parse and is
   `arch`/`bench` lane property under the working agreements. I cannot write it without
   touching source I am forbidden to modify.

**Recommendation:** fold the §7.2 byte-prize arithmetic into the prereg's *pre-registered
prediction* (I have already computed it above), and let the remote job measure the ratio
directly. If the coordinator prefers a written oracle, it is ~150 lines and I will write it
on request.

---

## 11. Remote GitHub-Actions pre-registration

Protocol per `docs/GITHUB-ACTIONS-BENCHMARKING.md` (tiers A/B/C, §5 `taskset` pin, §8.1
`manifest.json` + `bytes.csv`, §9 promotion questions 1–7) and the I10 ladder
(`R0 build/correctness → R1 byte-only oracle → R2 scout throughput → R3 promotion
evidence`). **Manual dispatch only. No local corpus benchmark. No local fuzz campaign.**
Provenance rules from MASTER-BRIEF item 10 and the ledger's measurement contract v1.2 apply:
every row carries runner id, CPU, `lscpu`, toolchain, commit SHA, corpus SHA-256, exact
argv, and per-repetition raw timings; bytes are deterministic and citation-grade, MB/s and
RSS are **scout/ranking-grade** and only from **paired same-job A/B**; cross-run absolute
comparisons are forbidden.

### Phase 0 — `R1`, byte-only, deterministic, no mechanism (the H2 prize)
* Frozen executable cells: the `u3` held-out PE set + `anvil.exe` (the file whose block 0
  density `FORMAT.md` already records as 519 transform fields).
* Measure: eligible phrase bytes; scan-found sites; true Δ=−d sites; mask bytes removed;
  site-list bytes added; the removed/added ratio; per-file.
* Also emit the **predicted** end-to-end byte prize using the frozen mode-14 vs mode-11
  delta as the denominator.

### Phase 1 — `R1`, byte-only, one job, one build: **the never-run A1–A5 provenance ablation**
Same corpus, same backend, same seed, **arm is the only variable**; every descriptor,
stream, table, mask, and framing byte charged.

| Arm | What it isolates | Register |
|---|---|---|
| **A1** | exact-LZ baseline | prior art floor |
| **A2** | sparse-corrected copy, **transmitted** 32-bit Δ (the *able*, prior-art form) | `gate-priorart-audit-i8.md` §4 |
| **A3** | sparse-corrected copy, **implicit** Δ=−d from distance, mask transmitted (current mode 14) | the audited claim |
| **A4** | **global BCJ/E8-E9 + same backend** | `FRONTIER-RESET` §6.4 named control |
| **A5** | **reference-local derived-mask** (H2), all descriptors charged | the target |
| **A6** | *(control)* raw input, same backend | the "raw is always a candidate" invariant |

**Mandatory controls (any FAIL ⇒ run VOID):**
1. `--fli`-equivalent byte-identity for the untouched arm: A1 and A6 must be **byte-identical**
   to the frozen baseline binary's output, chained by SHA-256.
2. `--bwt-aux=off` remains byte-identical per file (the identity closure already demonstrated
   in `docs/I10-AUX-UNBWT-RESULTS.md` §7.1 is the precedent).
3. Full byte-exact roundtrip on **every** file in **every** arm, plus forced-registry
   coverage for every decoder-visible ID touched (`FORMAT.md` §Registry coverage; MASTER-BRIEF
   item 12's format-strictness discipline).
4. No-regression controls must be present and reported: `random.bin` and the
   `generated.repeat.jsonl` exact-LZ control (ledger A18; `research-agenda.md` §1.4).
5. **Dual bar on every synthetic cell** — both the raw reference bar and the
   transform-enabled reference bar (`gate-priorart-audit-i8.md` §1; ledger PART XIII §5b).
6. Do **not** delete outliers. Ambient-load violations void the row, not the run.
7. Prohibited inference: **no frontier language from bytes alone** (dual bar + R-2/R-3
   classification + two hash-identical arbiter runs).

### Phase 2 — `R2`, scout only, paired same-job
Only if Phase 1 shows A5 ≤ A1 in bytes. Instrumented paired A/B: entropy pulls per output
byte, varint reads, scan bytes, copy bytes, per-process peak RSS, per-run output hashes.
**Scout/ranking-grade only.** The G4 precedent (`INVALID_SPEED_ACCOUNTING`, ledger PART XV)
is the standing warning: equivalent work must be timed on both sides of the ratio.

### Phase 3 — cross-lane, Track 11 G0
One job, literature-only, **no codec measurement**: S1/S2 anticipation search restricted to
*shipped* implementations, with the four verbs track 11 asked for and the ISLP finding of
§9.2 recorded as S3-anticipated. Owner: this track, on request.

---

## 12. GO / NO-GO thresholds (fixed **before** any measurement)

| Gate | GO | NO-GO / KILL |
|---|---|---|
| **G0 — prior art (this track's own gate)** | M-13's separator (Axis 2 + Axis 3) re-confirmed at claim level with ≥1 *additional* independent full-text database pass beyond the six recorded | any pass finds a single-file self-referential copy with a **distance-implicit** additive parameter → **KILL M-13 as a claim** |
| **G1 — Phase-0 byte prize** | removed/added mask-byte ratio ≥ **2.0** on ≥2 executable cells **and** projected end-to-end prize ≥ **0.10%** | ratio < 1.5 on every executable cell → **NO-GO on H2 as a byte mechanism**; kill H2 as a *mechanism*, keep BCJ as the adopt-class route |
| **G2 — A5 vs A4 (the C-Σ test)** | A5 ≤ A4 − **1.0%** complete bytes on ≥2 executable cells, with A5 decode cycles ≤ A4 · 1.02 paired | A5 ≥ A4 on both executable cells → **C-Σ survives → KILL the "reference-local" novelty position**; report H2 as BCJ-restricted-to-span (adopt-class) |
| **G3 — A3 vs A2 (provenance ablation, the never-run one)** | A3 (implicit Δ) ≤ A2 (transmitted Δ) on ≥2 executable cells → the zero-bit parameter carries measurable value | A2 ≤ A3 (this already happened once, `gate-ruling-i9-pr5-position-derived.md` P-7) → **KILL the implicit-parameter claim as unnecessary**, per `gate-priorart-audit-i8.md` §4's "the project has twice built the expeditious form" |
| **G4 — A5 vs A1 (does the mechanism earn anything at all)** | A5 ≤ A1 − **0.5%** complete bytes on ≥1 cell, decode within **1.10×** | A5 ≥ A1 → H2 has **no** value proposition on this architecture → **KILL** |
| **G5 — exactness / G4-amended** | residual identically zero and AUDIT-7 satisfied line by line with **every** derivation input available before use, and **zero look-ahead** | any look-ahead-derived parameter, or any bounded non-zero residual on the claimed path → **NOT CLAIMABLE** (`gate-ruling-i9-pr5-position-derived.md` P-3.1) |
| **G6 — decoder cost** | paired A/B shows A5 decode ≥ A5' (mask-transmitted) decode · **1.00** and within **1.05×** of A1 | A5 decode > A1 · **1.10** → **DECODE-SHORT**; record bytes as engineering only |
| **G7 — scan cost** | measured scan cost ≤ **0.25 ns/byte** of referenced text on the pinned runner | > **0.75 ns/byte** → the kernel eats the ≤0.02% byte prize → **KILL H2** |
| **G8 — decoder code budget** | added `.text` ≤ **4 KiB**, keeping the combined executable well under the 1 MiB decompressor bar | added `.text` > **16 KiB**, or a full length-disassembler is required → escalate as a **format/size decision**, not a silent change |
| **G9 — cross-lane (Track 11)** | S1 **and** S2 recorded NOT-ANTICIPATED with ≥2 independent full-text sources each | either anticipated → track 11 **NO-GO** per its own G0 |
| **G10 — scope discipline** | every artifact carries `{engineering}` / `{synthetic}` / `{adopt-class}` labels; dual bar reported on every synth cell; no frontier language without two hash-identical arbiter runs | any unlabelled ratio-only claim → the run's claims are withdrawn |

**Non-negotiable:** thresholds are frozen before measurement (MASTER-BRIEF item 5). G2, G3,
and G7 are the ones that can *close* the last novelty claim; G1 and G4 are the ones that
decide whether H2 is worth a mechanism build at all.

---

## 13. Adversarial failure modes of **this track's own analysis**

| # | Attack | Expected behaviour | Risk |
|---|---|---|---|
| **N1** | **My §7.2 byte-prize argument is wrong** because the frozen corpus's type-3 phrase volume is far larger than `anvil.exe` block 0's 519 fields | Phase 0 measures the ratio directly and can only raise the estimate | medium — this is why G1 is measured, not assumed |
| **N2** | **H2 requires a real x86 length-disassembler**, making it Courgette, and Courgette's patent/prior-art position was never audited | G8 escalates; the claim is withdrawn as "normalized-representation reconstruction", which `docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H2 already forbids claiming | **high if discovered after a build** — hence G8 and the explicit line-by-line AUDIT-7 table in §6 |
| **N3** | **Option (c) (unconditional scan with encoder-side admission verification) is unsound** — it rewrites non-operand 4-byte words and is only "correct" because the encoder checked | It is a **parser restriction**, not a decoder derivation; if I let it be called "derived", the claim launders a transmitted decision into a zero-bit claim | **high** — this is exactly the AUDIT-7 failure the project already made once with `pnra`'s retracted T7 error |
| **N4** | **A4 (global BCJ) is unfairly handicapped** — the in-repo rule is that transform-enabled references must be given the same representational opportunity | the same-transform control is mandatory (`FRONTIER-RESET` §11 Phase C item 1; ledger PART XIII §5b); an unpaired A4 result voids G2 | medium |
| **N5** | **A2 (transmitted Δ) beats A3 (implicit Δ) again**, exactly as at `synth-arith` | G3 NO-GO; the zero-bit claim is killed as unnecessary and the neighborhood shrinks to nothing | high — and it is the outcome the audit already predicts. **This is the single most likely result of Phase 1.** |
| **N6** | **Corpus selection decides the verdict** — executables are the only cells where H2 can fire | frozen corpus + locked families (`docs/I10-CORPUS-LOCK-PROTOCOL.md`); the PE set is already the held-out `u3`; no new held-out family may be opened before a discovery effect clears its gate (ledger PART XV) | medium |
| **N7** | **Timing contention** — an executable-heavy byte job invites clock measurements on a shared runner | Phase 1 is byte-only and deterministic; Phase 2 is paired same-job with pinned core and A/A null | medium |
| **N8** | **Cross-lane claim laundering**: my §9.2 ISLP finding gets quoted as "track 11 is KILL" | §9.2 states precisely what it kills (the abstract generality claim) and what it does not (the LZ-stream, fused, non-materializing instantiation); track 11's own G0 keys on S1/S2, not S3 | medium |
| **N9** | **My own novelty framing is the error**: I propose a *sixth* axis that is itself obvious | Axis 5 is stated as a question ("is the kernel a restricted pass?") with a pre-registered test that can kill it, not as a claim | low |
| **N10** | **Scope creep into tracks I must not duplicate** (11's P1c cost decomposition; 09's executable lane; 14's executable compiler) | Phase 1 is a *prior-art/claim* deliverable plus a byte-only ablation request to `arch`; I propose no new production mechanism | low |

---

## 14. Recommendation

### 14.1 Dispositions

| Item | Disposition | Why |
|---|---|---|
| M-1 aux-index BWT, M-2 DEFLATE reconstruction, M-5 BWT subblocking, M-12 integer-metadata recoding, M-4 G5D dictionary, M-3 planner, M-11 H4 innovation-channel factoring, M-16 correction topology, M-17 P(d\|s) unless its named ablation passes, M-18 G5A ordering | **KILL as claims** (adopt-class / engineering / already closed) | §4 map; each row cites its own project's recorded ruling |
| Any **transmitted-parameter** mechanism, anywhere, any time | **KILL** | prior art, foreclosed twice (`gate-priorart-audit-i8.md` §4) |
| Any **statistical/estimated** derived parameter | **KILL as claim** | DNB-M2, MATH-class, measured slope |
| Any **bounded non-zero-residual** derived parameter | **KILL as claim**; engineering only | `gate-ruling-i9-pr5-position-derived.md` P-3 |
| **H1 IGS-IR as a mechanism claim** | **HOLD** | conjunction-of-known-techniques obviousness target; its own kill conditions are already well specified and should be run as *oracles*, not as a format |
| **H3 iterated spans / FLI's `LOOP_ARITH` as a *generality* claim** | **HOLD, downgraded** | ISLP anticipates the abstract mechanism (§9.2) |
| **H2 reference-local derived-mask (M-8)** | **PILOT — byte-only Phase 0 + one A1–A5 job** | the only untested separator that carries a claim; its byte prize is *already projected to be tiny* (§7.2), so the pilot is a **cheap screen**, not a build |
| **M-13 TCOPY/PNRA exact implicit-Δ** | **HOLD as the project's only LIVE-NARROW claim**, contingent on G2/G3 | audit discharged, but Axis 5 untested and G3 has a strong prior |
| Track 11 **S3** | **DOWNGRADE to "anticipated in the abstract"**; must not be carried in the claim | §9.2, ISLP |
| Track 11 **S1, S2** | **NO VERDICT — GAP**, not clearance; one targeted pass owed | §9.1, §9.3 |

### 14.2 Final: **PILOT**

*Not* **PROMOTE-TO-REMOTE**: no mechanism currently clears its gate, the strongest one has a
projected ≤0.02% byte prize, and the project's own doctrine requires prior art to be a
first-class constraint before funding a build.

*Not* **HOLD**: (a) Phase 0 is byte-only, deterministic, cheap, and can return a decisive
NO-GO in one CI job — a NO-GO here is a *large* positive result for the project, because it
converts the last live novelty claim into an adopt-class item and frees the whole program;
(b) the A1–A5 ablation is **fifty+ iterations overdue** (`gate-priorart-audit-i8.md` §6
action 5) and is the only instrument that separates *mechanism* from *engineering* anywhere
in this project; (c) the §9 cross-lane answer is already actionable and changes track 11's
claim today, at zero measurement cost.

**Leaning confidence:** the expected outcome of Phase 1 is **G3 NO-GO** (A2 ≤ A3 again) and
**G2 NO-GO** (A5 ≥ A4), i.e. **the surviving novelty space closes**. I recommend the job be
framed to make that outcome a *positive, publishable-internal result* rather than a
disappointment: it is the first time this project would have a **measured, pre-registered
demonstration that its own best mechanism is prior art**, and that is worth more than
another ambiguous oracle.

**One thing I will not do:** claim that any of this constitutes a freedom-to-operate
opinion. `priorart-tcopy-external.md` §"FTO note" and `gate-priorart-audit-i8.md` §5 both
record that FTO attorney review remains recommended, Espacenet/lens.org were never queried
across six passes, and pass 4→5 corrected two citations — meaning earlier-pass descriptions
carry known error bars that I have propagated rather than re-verified.

---

## 15. Open uncertainties that would change this verdict

1. **A second full-text patent pass** (Espacenet/lens) on the M-13/M-8 neighborhood. Six
   passes used Google Patents + FreePatentsOnline only. If a single-file distance-implicit
   transform copy exists, G0 fails and M-13 is KILL. **This is the cheapest possible
   verdict-changer and it is not a code action.**
2. **Does H2 need a length-disassembler?** (N2/F1). Resolves G8 and decides whether the
   only LIVE-NARROW claim survives at all.
3. **Does A4, given a fair same-transform control, match or beat A5?** (G2 / C-Σ). Decides
   whether the "reference-local" separator is real.
4. **Is S2 anticipated by a shipped coder?** (§9.3). Decides track 11's G0.

Items 2–4 are all answered by the single Phase-0/Phase-1 job plus one literature query.
Item 1 is a literature query only.

---

# SECOND-CHECKPOINT ADDENDUM — the two load-bearing kill questions, run to completion

Scope of this addendum, per coordinator focus instruction: **no further broadening.** Two
questions only, each answered with the closest *concrete* artifact I could retrieve, plus
an explicit `SAME / DIFFERENT / UNCERTAIN` chart. A search that returned nothing is recorded
as `GAP` and is never used as a clearance. One previously-requested addendum (§16.5, Track
16 routing) is included because the searches were already spent and the material is decisive.

---

## 16.1 Question 1 — **FLI S2: fused consumer-side counted execution without materialization**

**Question.** Does any shipped codec or any paper implement a *counted loop executed in the
consumer, producing output directly, with no intermediate expansion buffer* — over a
byte-stream LZ token representation?

### Closest concrete artifacts retrieved

| # | Artifact | What it actually does | Retrieved |
|---|---|---|---|
| **A** | **arXiv 2607.24971**, *Right Multiplication on Grammar-Compressed Matrices: A Streaming, Memory-Bounded GPU Engine* (Tosoni & Mencagli) | Runs arithmetic **directly on the compressed grammar**, "without materializing uncompressed vectors"; a level-synchronous double-buffered streaming sweep over a grammar DAG; reports ~7× footprint reduction on real chromosomes | `SESSION`, abstract + method + Lemma 3.1 excerpt |
| **B** | **Sakamoto 2014**, PMLR v34 (sakamoto14a), *Grammar Compression: Grammatical Inference by Compression and Its Application to Real Data* | Frames **streaming** grammar compression and names the obstacle precisely: grammar compression's advantage "shrinks ... since there is **no working space for storing the whole data** supplied from data stream"; proposes stream grammar compression as *the framework for the next generation* | `SESSION`, abstract + §1 excerpt |
| **C** | **Cleary et al.**, *Revisiting the Folklore Algorithm for Random Access to Grammar-Compressed Strings* (par.nsf.gov/10555943) | Random access to a grammar-compressed string **without materializing it**; "the folklore algorithm requires ... `2m lg(m+σ)` bits to represent the grammar and `m lg n` bits for rule string lengths"; works directly on any grammar; `O(log n)` expected on Re-Pair grammars | `SESSION`, Algorithms 1–2 excerpt |
| **D** | Brotli / LZO / DEFLATE / LZMA token loops | **Fused** consumer execution — yes. **Counted loop over repeated tokens with operand suppression** — **no**. LZO's `state` carries literals-copied-by-previous-instruction into the next instruction's operand decode; LZMA rep0–rep3 is single-scalar reuse | `SESSION`, §9.1 |

### Claim chart — S2

| Element of S2 | Verdict | Basis |
|---|---|---|
| "Execute on the compressed representation rather than an expanded one" | **SAME (anticipated)** | **A** states it as its central contribution; **C** is an established algorithm family for grammars |
| "No intermediate/whole buffer is materialized" | **SAME (anticipated)** | **A**'s explicit claim; **B** identifies this as the known unsolved problem for *streaming decode* specifically |
| "Consumed in a counted loop that emits many output bytes per iteration" | **DIFFERENT** | A's counted loop is over grammar *levels/rules* with a matrix/vector consumer; C is random access, not streaming emission; D has no counted loop |
| "Consumer is an LZ byte-copy / periodic-memcpy kernel over an LZ token stream" | **DIFFERENT** | none of A–D is an LZ-family codec; D has the consumer but not the loop |
| "The counted loop's operands come from a zero-bit exact recurrence" | **DIFFERENT for S2; SAME for S3** | §9.2 (ISLP) |
| "Fused is *load-bearing* rather than incidental" | **UNCERTAIN** | this repo measured it (Exp. Y: materializing RePair/RLZ cost 8–16% decode), but no external artifact establishes that anyone chose fusion deliberately over materialization for this reason |

### Verdict on Question 1

**S2 is NOT anticipated as a mechanism; its *principle* is anticipated in an adjacent
domain.** Per the project's own standard — "a single invariant/technique is established art;
what has never shipped is a practical LZ-family compressor unifying them under one MDL
parser" (`gate-priorart-audit-i8.md` §2, restated at `gate-ruling-i9-pr5-position-derived.md`
P-4 item 2) — **S2 survives only as a systems/unification claim.**

Two consequences that bind track 11:

1. **Artifact B is the most dangerous citation for FLI, and it is a 2014 paper arguing the
   problem was still open.** That is *good* for novelty (nobody had solved it) and *bad* for
   defensibility (the problem was named and published 12 years ago, so FLI is "we solved a
   published open problem", not "we found a new phenomenon"). A reviewer will read it that
   way. Track 11 should cite B **voluntarily** in its prereg; not citing a directly
   on-point prior open-problem statement is the kind of omission that reads as concealment.
2. **Track 11 must drop S4** (§16.4) and **must not carry S3** (§9.2). What remains is S1+S2,
   which is one claim, not four.

**GAPs for Question 1 (not clearances):** I did not search LZ-family *implementations* for a
counted-repeat opcode (LZMA2 variants, 7-Zip forks, console/embedded crunchers), and I did
not search patent databases on "loop opcode". `priorart-tcopy-external.md` pass 3 already
flags proprietary packers and the 18-month publication blackout as unsearched territory.

---

## 16.2 Question 2 — **H2 reference-local exact transform vs global BCJ/E8-E9 equivalence**

**Question.** Is "apply a position-normalizing branch/call/jump transform only to the bytes a
particular copy instruction is about to reference" a documented idea, or is it genuinely
unclaimed?

### Closest concrete artifacts retrieved

| # | Artifact | What it actually documents | Retrieved |
|---|---|---|---|
| **E** | **liblzma `lzma_options_bcj::start_offset`**, `tukaani.org/xz/liblzma-api/structlzma__options__bcj.html` (liblzma 5.8.2) | Verbatim: *"Start offset for conversions. This setting is useful only when the same filter is used separately for multiple sections of the same executable file, and the sections contain cross-section branch/call/jump instructions. In that case it is beneficial to **set the start offset of the non-first sections so that the relative addresses of the cross-section branch/call/jump instructions will use the same absolute addresses as in the first section**."* | `SESSION`, full field doc |
| **F** | **XZ Embedded / LZMA SDK BCJ decoder**, `xz_dec_bcj.c` + `Bra86.c`, Collin & Pavlov (public domain 2008) | The transform's state is `pos`, documented as *"Absolute position relative to the beginning of the uncompressed data (in a single .xz Block)"*, plus `x86_prev_mask` and a per-ISA Alignment/Look-ahead table (x86: alignment 1, look-ahead 3) | `SESSION`, kernel source excerpt |
| **G** | Wikipedia **BCJ (algorithm)**, rev. 2025-07-13 | BCJ "replacing relative branch addresses with absolute ones"; present in Microsoft's 1996 cabinet file for LZX; ZPAQ calls its variant "E8E9"; and the explicit observation that **bsdiff "circumvents the need of writing architecture-specific BCJ tools by encoding bytewise differences"** — i.e. the BCJ route and the sparse-delta route are publicly recognized alternatives | `SESSION` |
| **H** | Hoerning **US 4,021,782** (1975), via the comp.compression FAQ patent index | *"A primitive form of LZ77 with implicit offsets (compare with previous record)"* | `SESSION` |
| **I** | The in-repo six-pass audit, `docs/priorart-tcopy-external.md` passes 1–6 | Five passes concluded NOT-FOUND for single-file self-referential COPY with a distance-implicit additive transform; the binding Intel US 7,111,148/'665/'382 review found hardware μop-storage compaction with no LZ layer | `RECORDED` |

### The derivation that decides the question

Artifacts E and F together make H2 **derivable** rather than merely adjacent:

1. F establishes that a BCJ filter's transform is a function of the operand and the
   **absolute output position** `pos`.
2. E establishes, as a **shipped, documented option**, that setting the position origin of a
   second region so that "the relative addresses ... use the same absolute addresses as in the
   first section" is *the intended use* of the filter's position parameter.
3. Therefore: **normalizing a copied phrase as if its bytes still lived at the source
   position is exactly `bcj(pos = source_position)` applied to the destination.** The
   arithmetic H2 calls "geometry-derived Δ=−d" is the composition of BCJ with a position
   origin, and that composition is documented in the liblzma API.

### Claim chart — H2

| Element of H2 | Verdict | Basis |
|---|---|---|
| "Relative→absolute branch normalization so duplicate targets match" | **SAME** | G, and the entire BCJ lineage |
| "The transform's parameter is an output-position origin" | **SAME (shipped, documented, 20 years old)** | F (`pos`), E (`start_offset`) |
| "Set the position origin so two different regions produce identical normalized addresses" | **SAME (shipped, documented)** | **E, verbatim** |
| "Normalize **only the bytes a given copy instruction is about to reference**", i.e. reference-local / lazy | **DIFFERENT** | E's granularity is **per-section**, F's is **per-stream**, both applied to the whole region before LZ sees it. No artifact applies the transform selectively per backreference. |
| "Δ is a 32-bit **additive** adjustment on 4-aligned windows, rather than an absolute rewrite" | **DIFFERENT in mechanics** | F rewrites the operand to an absolute value; H2 adds `d` to the already-copied word. Algebraically equal in effect, mechanically distinct. |
| "Residual is identically zero; no corrections" | **DIFFERENT** | BCJ is a pure pre-filter; it says nothing about sparse correction |
| "Multi-field amortization: one copy descriptor, several relocation fields fixed" | **DIFFERENT** | BCJ normalizes once globally, so it needs no amortization argument |
| **"Therefore the whole idea is non-obvious"** | **UNCERTAIN — and this is now the load-bearing cell** | The *idea* is documented and its parameterization is documented. What is unclaimed is a *placement granularity* change: section-level → reference-level. Whether a competent examiner regards that as obvious is exactly the Axis-5 question from §3, and **I now believe the project should assume it is obvious** |

### Verdict on Question 2 — and the recommendation change this forces

**Axis 1 in §3 ("where does the transform live?") is the *only* separator H2 has left, and
the project's own gate already ruled that separator insufficient on its own.** From
`gate-ruling-i9-pr5-position-derived.md` P-1: per-reference placement *"lands the mechanism in
the crowded **copy-with-edits** bucket (VCDIFF/bsdiff/Zdelta/Zucchini), where placement alone
is not novelty."* And from P-5 item P5-3: *"REJECT the claim that per-reference scoping plus
a discovered step suffices as separation."*

What this addendum contributes on top of the existing gate ruling is the missing piece those
rulings did not have: **`start_offset` proves the position-origin trick is not merely known but
is an intended, documented feature of the closest prior-art filter.** Without E, "derive the
transform from reference geometry" looked like a fresh observation. With E, it is *BCJ composed
with a position argument* — and reference-local placement is the only residual delta.

**Therefore: H2 / M-8 is downgraded from `LIVE-BUT-UNTESTED` to `THIN`, and from "PILOT a
byte-only screen with a novelty payoff" to "HOLD as a claim; run the byte arithmetic only as
an engineering screen."** Two things must both be true for any residual claim to survive:
(i) reference-local placement must be *measurably better* than global BCJ (the C-Σ test,
G2), **and** (ii) the byte prize must be non-trivial — and §7.2 already projects the prize at
**≤ ~0.02% end-to-end** on the one executable cell whose density is recorded. Those two facts
together make an H2 claim hard to motivate even if it passes.

I record this as a **downgrade I am making against my own earlier framing**, on the evidence of
one liblzma API field. It is not a KILL: the C-Σ test could still falsify C-Σ and would then be
the project's first *quantitative* evidence that placement carries independent value. But the
claim is now **engineering-first, novelty-second**, and no claim text should be drafted until
G2 passes.

---

## 16.3 Net effect on the five separator axes

| Axis | Status after both kill questions |
|---|---|
| 1 — where the transform lives | **Weakest possible.** Placement-only is explicitly rejected by the project's own gate; `start_offset` shows the placement parameter is a documented filter feature |
| 2 — parameter provenance (transmitted vs derived) | **Unchanged, still the strongest.** Prior art on transmitted; the derived form is the only defensible side |
| 3 — exactness (zero residual) | **Unchanged, still sharp.** G4-amended; non-exact derived parameters are engineering-only and were measured unnecessary |
| 4 — operand count and recurrence rule | **Weakened.** ISLP anticipates exact per-iteration recurrence with zero per-iteration symbols |
| 5 — kernel placement / when the filter runs | **Now the single load-bearing axis, and it is a granularity argument, not a mechanism argument** |

**Honest summary:** of five axes, **one** (parameter provenance) is strong, **one**
(exactness) is sharp, and **three** are weak or weakened. The project's defensible novelty
surface is therefore **narrow, exact, derived-parameter mechanisms only** — which is precisely
TCOPY/PNRA, and precisely the thing whose economics §7.2 shows is weak. I state that plainly
because it is the most decision-relevant sentence in this document.

---

## 16.4 S4 restated and corrected — **downgrade recorded**

Full chart in §9.4. Summary: Brotli's `c/enc/encode.c` gates a representation/model choice on
`entropy[1] - entropy[2] < 0.2` with the in-source comment *"in exchange for faster decoding
speed"*, and the constants are *"tuned by compressing the individual files of the silesia
corpus."*

| Element of S4 | Verdict | Basis |
|---|---|---|
| Gate a representation choice by an explicit rate-vs-decoding-speed trade | **SAME** | Brotli `encode.c` |
| Do it from a cheap analytic estimate, not real encodes | **SAME** | Brotli `encode.c` (entropy estimates) |
| Fit the trade constant empirically on a benchmark corpus | **SAME** | Brotli's own in-source comment (Silesia) — and this is ANVIL's own frozen corpus |
| Decide **per construct** (per token / per region / per carrier) | **DIFFERENT** | Brotli decides once, whole-file |
| Use a λ-weighted multi-axis objective including memory | **DIFFERENT** | Brotli's gate is two-axis (bits, speed) with a hand constant |

**S4 must be restated as: "per-construct decode-cost gating is unclaimed."** And it is
unclaimed *at the same time that this project cannot calibrate it* (Exp. AA: "right ORDER,
wrong GAPS"). A claim that is both narrow and unmeasurable should not be carried.

---

## 16.5 Track 16 addendum — per-region routing / low-rank whole-carrier calibration

Requested before the focus narrowing; the searches were already spent and the material is
decisive, so it is recorded here compactly. **No further search was run for it.**

### 16.5.1 Generic routing prior art — CLOSED, by three shipped implementations + one standard rule

| Prior art | Exact mechanism, as documented | Reference |
|---|---|---|
| **Brotli meta-block splitting** | A **greedy per-category block splitter** (`blockSplitterLiteral` / command / distance) with `min_block_size_`, `split_threshold_`, and a three-way action per split: *"(1) emits the current block with a new block type; (2) emits the current block with the type of the second last block; (3) merges the current block with the last block."* Split decisions come from **entropy estimates** of the running histogram. Quality gates: `minQualityForBlockSplit = 4`, `minQualityForHqBlockSplitting = 10`, `maxNumDelayedSymbols = 0x2FFF`. | `SESSION`: `skia.googlesource.com/external/github.com/google/brotli` — `c/enc/metablock_literal.go`, `c/enc/block_splitter.go`, `quality.go`, `c/enc/encode.c` |
| **zstd block splitting** | Two splitters with an explicit accuracy/cost trade documented in-source: `@postBlockSplitter` *"executes split analysis after sequences are produced, it's more accurate but consumes more resources"*; `@preBlockSplitter_level` *"splits before knowing sequences"*; *"Highest `@preBlockSplitter_level` combines well with `@postBlockSplitter`."* Plus a **target-compressed-block-size** parameter `targetCBlockSize` exposed as `ZSTD_c_targetCBlockSize=130` (v1.5.6+). | `SESSION`: `facebook/zstd` `lib/compress/zstd_compress_internal.h`, `lib/compress/zstd_preSplit.h`, `doc/zstd_manual.html` |
| **zstd / RFC 8878 raw-fallback rule** | Verbatim: *"If a compressed block is larger than its uncompressed content, it is recommended to send it uncompressed (i.e., a Raw_Block)."* **This is ANVIL's own "raw input is always a candidate" invariant, standardized since RFC 8878.** | `SESSION`: `doc/zstd_compression_format.md`, RFC 8878 §3.1.1.3 |
| **LZMA block-mode / cut selection** | **GAP** — not searched this session. Recorded as unverified, not as absent. | — |
| **Learned / per-block codec-selection literature** | **GAP** — not searched this session. This is a large and active literature; treat "no anticipation" as unsupported until a pass is run. | — |

**Verdict:** *route by an analytic cost estimate, with a raw escape, at region or block
granularity* is **fully occupied**. Brotli occupies entropy-estimated greedy splitting with
per-category block types; zstd occupies accuracy-vs-resource split analysis and a target
compressed-block size; RFC 8878 occupies the raw-fallback rule. **Track 16 cannot claim
generic routing, per-region routing, or the "cost-dominated routing with fallback" framing as
novelty on any of these grounds.**

### 16.5.2 The narrower claim — `DIFFERENT`, but weak

| Element of "low-rank whole-carrier residual calibration" | Verdict | Basis |
|---|---|---|
| Predict a region/carrier's compressed cost cheaply | **SAME** | Brotli entropy estimates; zstd split analysis; G4's own frozen q11 oracle concept |
| Two-stage: sample first, then choose family, then local parameters | **SAME** | ALP / FastLanes two-stage adaptation (adopted in `docs/I10-GROTLI-FRONTIER-ARCHITECTURE.md` §6 P3) |
| Offline/expensive search, cheap resolved decoder | **SAME** | OpenZL's resolved graph in frame |
| **Rank-1 (low-rank) calibration fitted on the whole-carrier residual** | **DIFFERENT in form** | no artifact retrieved; closest is ALP/FastLanes' exponent×factor *product form* (ALP encodes values as `exponent × mantissa + exceptions`) |
| **Calibration specific to *this* carrier family with charged table bytes** | **DIFFERENT in discipline only** | this is ANVIL's standing accounting rule, not a mechanism |
| Whether rank-1 suffices / beats order-0 | **UNCERTAIN — untested** | **G4 already measured that cheap proxies MISS the frozen q11 carrier choices** (ledger PART XV; speed column withdrawn as `INVALID_SPEED_ACCOUNTING`) |

**Verdict:** the rank-1 residual-calibration claim is **formally unoccupied** but **generically
weak for three independent reasons:** (i) a rank-1/low-rank structure is itself a well-worn
modeling choice in adjacent fields; (ii) ANVIL's own G4 lane already produced a **measured
negative** for cheap proxies at exactly this job, so this enters a lane that has already failed
once; (iii) the same `H5`/`Exp-AA` calibration defect that weakens S4 weakens this too — a
λ-weighted cost objective the project cannot calibrate cannot be the load-bearing element of a
claim. My recommendation to track 16: file it as a **measurement** question ("does rank-1
predict complete-carrier bytes better than order-0 entropy, at O(1) planner cost?") and **not**
as a novelty claim, and run it only after G5A-informed planning, since G5A is the only ordering
result in the program that survived its held-out open (`ORDER-MATERIAL / COLUMN-DOMINANT`,
ledger PART XV).

---

## 16.6 What this addendum does to the recommendation

| Item | Before this addendum | **After** |
|---|---|---|
| M-13 TCOPY/PNRA | HOLD as only LIVE-NARROW | **HOLD as only LIVE-NARROW — unchanged; it is now the *sole* survivor** |
| M-8 H2 reference-local derived-mask | PILOT a byte-only screen (novelty payoff) | **HOLD as a claim**; run the byte arithmetic as an engineering screen only |
| M-9 H1 IGS-IR | HOLD | HOLD |
| M-10 H3 iterated spans / S3 | HOLD, downgraded | **KILL as a claim** (ISLP anticipates the abstract mechanism) |
| M-14 FLI | PILOT conditional | **PILOT conditional, narrowed to S1+S2 only; S4 dropped** |
| Track 16 routing | not yet assessed | **generic routing CLOSED; rank-1 calibration = measurement question, not a claim** |

**Final leaning, unchanged in direction but sharper in content: PILOT** — but the pilot is now
narrowly two byte-only remote jobs (Phase 0 + the A1–A5 provenance ablation with the C-Σ test)
whose realistic and explicitly expected outcome is that **the project's last novelty claim
closes**. I want that outcome, and I have written it down in advance so that it cannot be
reinterpreted afterwards.

---

*Interim checkpoint written by Space Bunny Free, track 20-priorart-killteam, 2026-10-02.
Evidence classes: MEASURED / RECORDED / PROJECTION / SESSION / GAP as defined in the header.
This is a literature- and claim-classification record, not legal advice. No measurement was
performed by this agent; no existing file was modified; no commit, push, reset, clean,
stash, restore, or rebase was performed.*