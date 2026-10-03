# Track 09 — Exact transformed backreferences: TCOPY/PNRA exact-invariant space

**Author:** Space Bunny Free (constructive inventor), swarm `swarm-2026-10-02`, track `09-exact-tcopy-pnra`
**Date:** 2026-10-02 · **Revision:** 3 (post second cross-lane reconciliation with
`20-priorart-killteam-space-bunny.md` **finalized** §16.2, `09-exact-tcopy-pnra-fledge.md`,
`03-heldout-corpus-*.md`, and a Track-20-critic arithmetic correction)
**Mandate boundary:** exact-invariant space only — implicit decoder-known parameters, RIP/PC-relative
fields, algebraic invariants, sparse exact corrections.

> **Labels.** `MEASURED` = recorded in a named artifact here, path + line. `DERIVED` = arithmetic on
> measured inputs, shown inline. `ESTIMATE` = projection with basis named. `HYPOTHESIS` = not
> measured. Cross-lane figures are `XL-MEASURED` / `XL-SESSION` and are **never** restated as my own
> measurement. **No benchmark was run for this report.**

---

## 0. Bottom line — revision 3 changes the recommendation

| Axis | Ruling | Reason |
|---|---|---|
| **CORRECTNESS** | **SURVIVES** | `Δ=−d` is genuinely zero-bit, decoder-derived, exact in ℤ/2³², strictly bounded; source-verified by the critic lane at `src/anvil.cpp:594-604, 2706-2735` (`XL-MEASURED`) |
| **ECONOMICS** | **DEAD as implemented** | TCOPY is worth **0.0566%** of corpus bytes vs its own control, **6.88% worse** than MDL, and is **DOMINATED by brotli q6 on bytes, encode AND decode** (`XL-MEASURED`) |
| **NOVELTY** | **H2 WITHDRAWN. TCOPY/PNRA parameter-provenance survives as the only narrow survivor, and its projected prize is ≤ ~0.02% end-to-end** | Track 20 §16.2 derives H2's geometry-derived transform as **BCJ composed with a position origin** (`liblzma_options_bcj::start_offset` + BCJ's `pos`), leaving only reference-local placement — which this project's own gate already rejected |

**I withdraw all H2 novelty language.** I attempted to rebut the `start_offset` derivation and could
not (§5.3). My mechanism claims N1/N2/N3 from revision 2 are withdrawn.

**Final ruling: PILOT — one byte-only remote CLOSURE/ATTRIBUTION job, re-ranked, with the
reference-local derived-topology arm DROPPED from scope. No mechanism build, no format commitment,
no Pareto claim, no novelty claim. Promotion is blocked on corpus acquisition and, independently, on
a novelty question that is now closed.**

---

## 1. Evidence map

### 1.1 Own lane, measured in this worktree

| ID | Fact | Source |
|---|---|---|
| E1 | Mode 14 `TCOPY` = SPARSE-REF + token type 3, implicit `Δ=−dist`, zero parameter bits; 8 substreams incl. a transform mask (1×u32 per 32 four-byte windows); `field_end ≤ dist`; strict decoder | `FORMAT.md:483-504` |
| E2 | Mode 14 vs mode 11 sparse, SAME greedy parse: `anvil.exe` 0.4056 vs 0.4081 (−0.6%); `anvil_bench.exe` 0.4438 vs 0.4442 (−0.1%); 519 transform fields in `anvil.exe` block 0; fuzz 540 PASS | `RESEARCH_LEDGER.md:950-959` |
| E3 | **TCOPY loses to exact-LZ MDL (mode 10)** on those executables — 0.406 vs 0.402. Recorded cause: *"the greedy approximate parse is the limiting factor, not the transform"* | `RESEARCH_LEDGER.md:971-974` |
| E4 | **The explicit-Δ control is not built**; the narrowed-claim test is INCOMPLETE | `RESEARCH_LEDGER.md:966-970` |
| E5 | `--pnra=on`: `anvil.exe` 108,518→108,514 B (−0.0037%); `anvil_bench.exe` 845,675→**846,050 B (+0.0443%)**; 11 non-PE files byte-identical; enc 9.679→9.223 MB/s; dec 172.0→166.0 MB/s; **NOT ADOPTED** | `RESEARCH_LEDGER.md:2537-2561` |
| E6 | Root cause, measured: per-token framing underpriced **and** alternative overpriced; modal shape is a single isolated 4-byte field at a scattered, often-far distance | `RESEARCH_LEDGER.md:2560-2591` |
| E7 | Frozen `γ=0.5` gate: 108,506 (−0.0110%) / 845,657 (−0.0021%); commits 429→29 / 1,599→80; **95.6% / 96.9% of flips have L≤8**; **author's own pre-registered prediction: held-out verdict FAIL** | `RESEARCH_LEDGER.md:3287-3345` |
| E8 | **MEASURED coded/raw stream ratios:** `masks 0.435`, `tmask 0.153`, `types 0.169`, `ml 0.59`, `ds 0.93` | `RESEARCH_LEDGER.md:3257-3259` |
| E9 | The S6-2 frozen verdict protocol (≥0.5% on six PEs) has **NEVER been run** | `RESEARCH_LEDGER.md:3183-3188`, `:3347-3388` |
| E10 | G4-amended: a zero-bit derived parameter is defensible **only where the transform is EXACT** | `docs/gate-ruling-i9-pr5-position-derived.md:37-43` |
| E11 | **AUDIT-7**: derivation must use bytes **already reconstructed at the point of use**; look-ahead quantities are transmitted information in disguise | same `:84-93` |
| E12 | PR-5 decisive fact: transmitted-step **12,921 B < derived 12,936 B** ⇒ derived parameter unnecessary ⇒ engineering/adopt | same `:212-236` |
| E13 | Surviving positions: (1) exact invariant indexing; (2) unification of multiple equivalence relations under one MDL parser (*systems* claim); (3) correction topology coding | same `:98-114` |
| E14 | Patent gate CLOSED on the narrowed implicit-Δ claim (5 passes + Intel `'148`/`'665`/`'382` full text, expired). **Experimental conditions only** | `docs/priorart-tcopy-external.md:487-505` |
| E15 | I8 audit §4 verbatim: the A1–A4 ablation *"has never once been executed in this project"* | `docs/gate-priorart-audit-i8.md:196-202` |
| **E16** | **PR-5 P-1 / P5-3 — the two rulings that close my only remaining separator:** per-reference placement *"lands the mechanism in the crowded **copy-with-edits** bucket (VCDIFF/bsdiff/Zdelta/Zucchini), where placement alone is not novelty"*; and *"**REJECT** the claim that per-reference scoping plus a discovered step suffices as separation"* | `docs/gate-ruling-i9-pr5-position-derived.md:28-33`, `:81-82` |

### 1.2 Cross-lane evidence I adopt

| ID | Fact | Provenance as its author stated it |
|---|---|---|
| **X1** | TCOPY's whole effect is **1,477 B = 0.0566%** vs sparse-rANS; **167,853 B = 6.88% worse** than MDL-rANS | `XL-MEASURED` critic M1 from `tests/benchmark-suite.frozen-bdc90474.csv`; Class A frozen `bdc90474`; author notes the frontier doc and CSV are **not independent** |
| **X2** | TCOPY **DOMINATED by brotli q6 on all three axes**: +443,851 B (+20.5%), 6.7× slower encode, 2.8× slower decode | `XL-MEASURED` critic M2 from `docs/I10-FRONTIER-RECON-2026-09-24.md:36-43`; author labels it **"ranking-grade, not citation-grade — elevated ambient utilization"**, and that CSV **has no peak-RSS field** |
| **X3** | PNRA: bit-identical on 11/13 frozen files; **net +371 B worse**; on 6 PEs net −0.271% but aggregate encode **3.028→2.008 MB/s (−33.7%)**; whole PE gain is **one file** (`pe-git.exe`, −7,442 B) at **−39.5% encode** | `XL-MEASURED` critic M3; **author warns M1 and M3 are different corpora — not pooled here** |
| **X4** | TCOPY adds an 8th substream; per-substream bound `16*out_len+64` ⇒ worst-case transient allocation ceiling **112×out_len → 128×out_len (+14.3%)** — inherited, amplified, not a new vulnerability | `XL-MEASURED` critic §3.3, from `src/anvil.cpp:2592, 2653` |
| **X5** | PNRA index is `unordered_map<uint32_t, vector<uint32_t>>`, **per block, unconditional, no cap/eviction/reserve**: ≈**60–70 B per relocation field** ⇒ ≈7–8 MB encoder RSS + ~115k allocations per 3.26 MB `.text`, rebuilt per block, **zero decoder benefit** | `XL-MEASURED` critic §3.3, `src/anvil.cpp:858-868` |
| **X6** | `scan_candidate` (`src/anvil.cpp:590-614`) does **not** charge the per-token descriptor or the 0.25 bit/byte transform-mask rate. Block router does price everything (`:4764`) ⇒ intra-block only | `XL-MEASURED` critic §3.2 |
| **X7** | The shipped PNRA invariant **is** BCJ's canonical form: `src/anvil.cpp:847-848` names `I(v,p)=p+4+v` as *"the absolute branch target"* | `XL-MEASURED` critic M6 |
| **X8** | **Track 20 FINAL §16.2 — H2 is derivable, not merely adjacent.** (i) BCJ's transform is a function of the operand and absolute output position `pos` (XZ Embedded `xz_dec_bcj.c`, `Bra86.c`, public domain 2008). (ii) `liblzma_options_bcj::start_offset` is a **shipped, documented** option whose *verbatim* stated purpose is to *"set the start offset of the non-first sections so that the relative addresses of the cross-section branch/call/jump instructions will use the same absolute addresses as in the first section"*. (iii) Therefore **"normalize a copied phrase as if its bytes still lived at the source position" = `bcj(pos = source_position)` applied to the destination**, and the arithmetic H2 calls "geometry-derived `Δ=−d`" is **BCJ composed with a position origin**. Track 20 downgrades M-8 from `LIVE-BUT-UNTESTED` to `THIN` and from a novelty payoff to an engineering screen | `XL-SESSION` (liblzma 5.8.2 API field doc, full verbatim; kernel source excerpt) + `RECORDED` (six-pass in-repo audit) |
| **X9** | Track 20 §7.2: H2's whole byte advantage is the mask share of a mode-14 stream worth **≲0.6% over mode 11** ⇒ **≤ ~0.02% end-to-end byte prize** | `XL-MEASURED`/PROJECTION, explicitly *"a projection from a measured density, not a measurement of H2"* |
| **X10** | The six `pe-*` files are **open known/anchor data and MUST NOT be relabelled unseen** (protocol §2.3). **Executable held-out family = ZERO.** Independence: `pe-where`/`pe-winver` share an exact 4 KiB block ⇒ one unit; 3 of 6 are one OS release; honest score **1 of 6 from a genuinely different producer**; all six report optional-header linker 12.1, which **cannot** evidence toolchain diversity | `XL-MEASURED` Track 03 `E8`/`E9`, critic `§3.3` |
| **X11** | *"Any promotion claim requires appropriate locked held-out evidence; synthetic or previously consumed data may be discovery/control evidence only"*; Tracks 09/12/13 *"downgrade current corpus evidence to discovery/control and require newly locked executable / structured / numeric families for promotion"* | `docs/swarm-2026-10-02/COORDINATOR-STATE.md` D1 |

### 1.3 The Linux density leg — mandatory label

`[SECTION-ONLY · LINUX · DIRECTIONAL · DID NOT TRANSFER]`

`.text`-only, one ELF section, one host, a crude 10-stream prototype wire. Best points: TCOPY
1,761,776 B vs q4 1,781,130 B; PNRA combined 1,730,689 B. Also: **`.eh_frame` ZERO TCOPY phrases
survive; `.rodata` essentially zero**; transmitting a general Δ made both sections **larger**;
64–256 KiB block-local reset made `.text` **worse**; ~115,490 relocation-pair events vs ~1.4M parser
positions in 3.26 MB `.text`. Source `docs/CONTEXT.md`, whose own header declares it *"historical
shared context … intentionally preserved stale intermediate priorities."*

X1/X2 (frozen Class A, whole-file, production representation) say the mechanism is worth 0.0566% and
is 20.5% behind q6. `docs/CONTEXT.md` says a section-only leg beat q4 by 1.1%. The critic names this a
*live provenance defect* and recommends the CONTEXT TCOPY block be relabelled at its first line. **I
endorse that action.** These Linux numbers are used in exactly one place below — the arithmetic in
§3.3 — and in no gate.

### 1.4 Gaps

- No measured brotli **decode** throughput on an ELF `.text` cell.
- **No locked held-out executable family exists** ⇒ promotion is blocked on corpus acquisition.
- `μ` (bytes covered by sparse/type-3 phrases) measured nowhere ⇒ all mask costs are
  per-covered-byte.
- **No RSS claim is admissible from any existing artifact** (X2's note); X4/X5 are derived from
  source structure, not measured.

---

## 2. CORPUS STATUS (coordinator instruction + Track 03)

| Stratum | Files | Role |
|---|---|---|
| **Calibration** | `anvil.exe`, `anvil_bench.exe` | calibration only |
| **Control / known-anchor** | `pe-winver`, `pe-where`, `pe-notepad`, `pe-python`, `pe-ninja`, `pe-git` (5,540,960 B) | **`known_stress` / consumed anchors.** Measurable; **discovery/control evidence only.** Never pooled with a future locked family; **never** cited as held-out |
| **Known stress** | Sino-US DrugQA V1 (15,374,047 B) | already burned by G3 (`RESEARCH_LEDGER.md:4759-4760`) |
| **Held-out executable** | **none — ZERO** | **binding promotion blocker** |

Protocol §13's minimum new portfolio (≥3 executables spanning ≥2 producers and ≥2 toolchains, plus
structured / mixed-validity / non-synthetic numeric families) does not exist. Per X11 this lane's
current corpus evidence is **discovery/control** only.

---

## 3. Mechanism — SLX-REF: retained as documentation, **not proposed for build**

Revision 2 proposed `XREF(d, L)`: copy `L` bytes at distance `d`, then a pure span-local recognizer
`R(S, d, L)` over the just-reconstructed source span applies `σ_c·d` per site (`σ=−1` for
`E8/E9` `C1 REL32`; `σ=+1` for 12-aligned `C2 ABS32-P` triples). Invariants: non-overlap `i+4 ≤ d`
(hard); purity; `C2` claims sites before `C1`; exactness is an encoder-side precondition with no
decoder verification branch; every recognizer input already reconstructed (AUDIT-7 clean by
construction).

**Status after X8: the mechanism is retained as a documented capability and is NOT proposed for a
build.** Its distinguishing content — deriving the transform from copy geometry — is now recorded as
BCJ composed with a position origin, and its only residual delta, reference-local placement, is
pre-emptively rejected as a separator by this project's own gate (E16). The `C2 ABS32-P` extension
was already HOLD: it needs a second recognizer, a second prior-art audit (Courgette's ABS32), and a
locked held-out family, none of which exists — and it inherits the same Axis-2 closure, since the
transmitted-vs-implicit question has now gone against the implicit form **three times** (§8.4).

### 3.3 ARITHMETIC CORRECTION (Track-20-critic record fix)

My revision-2 statement — *"≈0.63 B out of ~1.76 MB ≈ 0.000036%"* — was **wrong by 1000×** and is
withdrawn. Correct arithmetic:

```
5,070 transformed phrases x 1 bit/phrase      = 5,070 bits
5,070 bits / 8                                = 633.75 B
633.75 B / 1,761,776 B                        = 3.596e-4  =  0.0360%
```

The premise is unchanged and was not the error: on the `C1 REL32` class the geometric parameter is
`Δ ≡ −d` and `d` is **already transmitted as the distance field**, so a transmitted-Δ control needs
**1 bit per phrase** to say "Δ = minus the distance", not 32 bits.

**Consequences, which are the opposite of what revision 2 concluded:**

1. The `A1` (explicit-Δ) vs `A2` (implicit-Δ) arm is **fully resolvable**. 633.75 B on a 1.76 MB cell
   is ~1000× the noise scale, and byte counts are deterministic integers (ratio CV 0.000%), so there
   is **no measurement-noise floor at any corpus size** — only a materiality question.
2. On the Windows PE control stratum the same quantity is **1,599 bits = 199.9 B / 846,050 B =
   0.0236%**; under the frozen γ=0.5 gate, 80 commits ⇒ **10 B / 845,657 B = 0.0012%**. Resolvable;
   immaterial. **Resolvable ≠ material.**
3. `COLLAPSE-C3` therefore changes from "pre-declared expected null, underpowered" to
   **"decisive, deterministic, and the single highest-information arm in the job."** Thresholds are
   **unchanged** (per coordinator); only the resolvability characterization is corrected.
4. My revision-2 sentence *"an implicit-vs-transmitted ablation run only on `E8/E9 rel32` is
   underpowered by construction — it cannot resolve a difference"* is **withdrawn as wrong.**
   The correct statement is: it **can** resolve the difference, the difference is **small**, and
   resolving it is still worth doing because it closes the project's last survivor.

---

## 4. Cost model (retained; two corrections stand)

### 4.1 Bytes

Per **128 covered phrase bytes**, from E1 geometry and E8 ratios:

| Component | raw bits | × measured ratio | coded bits | coded bytes |
|---|---:|---:|---:|---:|
| residual mask (1×u32 per 32 B) | 128 | 0.435 | 55.68 | 6.96 |
| transform mask (1×u32 per 128 B) | 32 | 0.153 | 4.90 | 0.61 |
| **total** | **160** | | **60.58** | **7.57** |

DERIVED: **0.473 coded bits per covered byte = 0.0592 B/covered byte.** Per-file it is
`0.0592 × n × μ`; **μ is unmeasured**, so no per-file figure is admissible.

I concede to the critic's C4 verbatim: **Δ is zero-bit; the topology M is transmitted at 0.25 raw
bit/byte of phrase.** The implicit half defends the cheap half of a mechanism whose expensive half
is prior art.

### 4.2 The byte prize is bounded

Adopting X9: SLX-REF removes the mask share of a mode-14 stream worth **0.0566%** against its own
control (X1) and **6.88% worse than MDL** (X1). Therefore **PROJECTION: SLX-REF's end-to-end byte
prize is ≤ ~0.02%** — a perfect mask elimination cannot lift a mechanism that is 20.5% behind brotli
q6 (X2) to a frontier row.

### 4.3 Cycles — corrected against my own revision 1

- `C1` scan: I estimated 0.13–0.19 c/B; **Track 20's independent estimate is 0.3–1.5 ns/B
  (~1–4.5 c/B), 2–12× higher. I adopt theirs.** (No ANVIL measurement exists.)
- Per site: **+0.75–1.25 c** over a copied 4-byte word.
- Site density **1 per ~198 B** (stale-flagged Linux count).
- Total **≈1.0–4.5 c/B** ⇒ **6–40%** of an 11–17 c/B decode budget.

**Against a ≤0.02% byte prize, a 6–40% decode-cost band on scanned bytes is a bad trade on both
axes.** This is the second independent reason the mechanism is not built.

### 4.4 Memory, RSS, decoder code and state

| Item | Cost |
|---|---|
| New decoder **state** | 0 B (recognizer O(1), no tables, no heap) |
| **Decoder allocation ceiling** | **112×out_len → 128×out_len (+14.3%)** from the 8th substream (`XL-MEASURED` X4; derived from source, not measured) |
| Decoder `.text` | ≈200–600 B scanner (Track 20's lower figure adopted over my +1.5–3 KB); a length-disassembler variant is much larger and must be charged against the decompressor size bar |
| **Encoder** PNRA index | ≈60–70 B/field, **uncapped**, rebuilt per block, ~7–8 MB RSS + ~115k allocations per 3.26 MB `.text`, **zero decoder benefit** (`XL-MEASURED` X5) |
| RSS from existing artifacts | **inadmissible** |

---

## 5. Novelty — H2 WITHDRAWN, and the attempt to rebut it

### 5.1 What survives

Track 20 §16.3's own summary: of five separator axes, **one** (parameter provenance: derived vs
transmitted) is strong, **one** (exactness: zero residual) is sharp, and **three** are weak or
weakened. The defensible surface is therefore *"narrow, exact, derived-parameter mechanisms only —
which is precisely TCOPY/PNRA, and precisely the thing whose economics is weak."* I concur.

### 5.2 The only narrow survivor, stated exactly

> A single-file self-referential LZ reference whose additive transform parameter is derived
> **implicitly from the backreference distance** (zero transmitted bits) with **identically zero
> residual**, in executable code. (E14 records the patent classification as closed; E15 records that
> its decisive experimental condition has never been run.)

That is **TCOPY/PNRA only**. Its projected prize is ≤0.02% end-to-end (X9) and its measured in-repo
effect is 0.0566% (X1).

### 5.3 My attempt to rebut X8, and why it fails

The coordinator required a concrete mechanism-level distinction or withdrawal. I tried four:

| Attempted distinction | Why it fails |
|---|---|
| **(a) Placement granularity is per-backreference, not per-section** | This is the *only* real delta (X8's claim chart agrees). But **E16 already rejected it twice by name**: P-1 places per-reference scoping in the copy-with-edits bucket (VCDIFF/bsdiff/Zdelta/Zucchini) where placement is not novelty; P5-3 *"REJECT the claim that per-reference scoping plus a discovered step suffices as separation."* A pre-registered project ruling is not something I can overturn with a re-framing |
| **(b) H2 adds `d` to a copied word rather than rewriting to absolute** | X8: *"algebraically equal in effect, mechanically distinct."* A mechanical variant of an anticipated transform is not a mechanism distinction, and additive-on-copied-words is exactly what every copy-with-edits engine does |
| **(c) Exactness / zero residual / AUDIT-7 line-by-line** | That is a *gate on claimability* (Axis 3), not a separator against prior art. BCJ is itself exact and lossless. Confirms claimability; creates no distinction |
| **(d) `C2 ABS32-P`: `Δ=+d` on co-translating absolute fields is not a BCJ operation** | True but insufficient. `v_dst = v_src + d` is the *same* position-origin shift as (a), generalized from a relative operand to an absolute one — `start_offset` with an origin delta of `+d`. So `C2` is the same trick on a second operand class, hence also anticipated in form; it additionally requires a claim-level Courgette ABS32 audit that does not exist, a locked held-out family that does not exist, and it inherits the Axis-2 closure (three prior losses for implicit-derived parameters) |

**No concrete mechanism-level distinction survives. H2 novelty is withdrawn.** I record the attempt
rather than only the conclusion, so a later reader can see exactly what was tried.

### 5.4 What is withdrawn

My revision-2 claims **N1** (computed topology), **N2** (multi-class unification), **N3** (exact-only
mask-free phrase) are **withdrawn as novelty claims**. N1/N3 are anticipated per X8; N2 remains a
*systems* framing that the project's own audits already label thin
(`docs/gate-priorart-audit-i8.md:118-127`) and that has no measured support. Retained as
documentation: the mechanism description, the invariants, the AUDIT-7 derivation, and the prototype.

---

## 6. Minimum prototype — built, compile-checked, never run, and now **out of scope**

`prototypes/swarm-2026-10-02/09-exact-tcopy-pnra/space-bunny/` (`slxref.cpp`, `README.md`). Isolated,
not on any build target, not wired into `src/anvil.cpp`.

It implements the span-local recognizer and an `Arm`-parameterized serializer emitting **one token
multiset in five representations**; the decoder **recomputes** the site set rather than reading it.
Enforced in code: non-overlap (recognition, deserialization, apply); transmitted-topology
disjointness; encoder-side bit-exactness gate; strict bounds + full substream consumption.

Verification: `clang-cl /nologo /std:c++20 /c /O2 /EHsc /W4 /WX /D_CRT_SECURE_NO_WARNINGS` ⇒
**EXIT=0, warning-clean**. Object written outside the repo. **Not linked, not executed.**

**Scope reduction, stated plainly.** The derived-topology arm (revision 2's `A4`) is **dropped from
the job in §7**, so this prototype will never be run to produce numbers. That is a reduction driven
by three recorded facts — withdrawn novelty (X8 + E16), a ≤0.02% prize (X9), and a 6–40% decode-cost
band (§4.3) — and it means **no experiment will ever measure my own proposal.** I record that as the
correct outcome, not as a reason to keep the arm alive.

---

## 7. The single byte-only remote job — re-framed as CLOSURE / ATTRIBUTION, and re-ranked

**Name:** `TCOPY-PROVENANCE-BCJ`. One job, one build, one arm-variable, byte-only, remote tier
**R1** (`docs/I10-BREAKTHROUGH-PROGRAM.md:541-553`). Same rANS backend, block size, seeds, binary.
**Every decoder-visible byte charged.**

**It is no longer justified as a novelty-clearance experiment.** Its two jobs are now:

- **J1 — closure.** Resolve `A1` vs `A2` (633.75 B, §3.3) to close Axis 2, the last survivor.
- **J2 — attribution.** Establish, by complete bytes, whether global BCJ subsumes mode 14, so the
  project can **retire an inert custom mechanism in favour of an adopt-class filter** — an
  engineering simplification, honestly labelled.

### 7.1 Arms, re-ranked by information gain per CI call

| Rank | Arm | Content | Status | Job |
|---:|---|---|---|---|
| **P0** | **A0** | exact-LZ / mode-11 sparse | exists | floor |
| **P0** | **A1** | **explicit/transmitted Δ**, identical representation | **MUST BE BUILT — never built** (E4) | **J1** |
| **P0** | **A2** | implicit `Δ=−d`, transform mask transmitted | exists (mode 14) | control of record |
| **P1** | **A3** | **global BCJ/E8-E9 + same backend** | **MUST BE BUILT** | **J2** |
| **P1** | **A5** | A2 + PNRA (`--pnra=on`) | exists | confirm-only |
| **P2** | **A6** | raw input, same backend | exists | "raw is always a candidate" |
| **—** | ~~A4~~ | ~~reference-local derived-mask / SLX-REF~~ | **DROPPED** | prize ≤0.02%, decode cost 6–40%, novelty withdrawn |

**Why A1 is now P0 and outranks the BCJ arm:** §3.3 makes `A1 vs A2` a deterministic 633.75 B
comparison that closes the project's only surviving novelty axis. `A3` is worth real engineering
value but cannot change a novelty verdict, because novelty is closed on paper (X8 + E16).

### 7.2 Mandatory controls (any failure ⇒ run VOID, not FAIL)

1. Byte-exact roundtrip in **every** arm on **every** file; forced-registry coverage for every
   decoder-visible ID touched (`FORMAT.md` Registry; MASTER-BRIEF item 12).
2. `A6` byte-identical to the frozen baseline's output, SHA-256 chained.
3. No-regression controls reported: `random.bin`, `generated.repeat.jsonl` exact-LZ control, and
   every non-PE file byte-identical across all arms (E5's gating guarantee).
4. Dual bar on any synthetic cell — raw **and** transform-enabled (`RESEARCH_LEDGER.md:5287-5297`).
5. A/A null: repeat the run, require hash-identical output.
6. **Prohibited inference:** no frontier language from bytes alone
   (`docs/I10-BREAKTHROUGH-PROGRAM.md:812-821`).

### 7.3 Corpus strata — reported separately, never pooled

Calibration: `anvil.exe`, `anvil_bench.exe` only. Control stratum (consumed known anchors,
`{known_stress}`): the six `pe-*` files, per-file and as a stratum, never pooled, never cited as
held-out (X10, X11). Discovery stratum: synthetic files, `{synthetic}` + generator citation. Held-out
stratum: **does not exist**; the job must emit this as an explicit blocker field.

### 7.4 Thresholds — UNCHANGED (coordinator), with arm-scope noted

**Group A — answerable on the control stratum.**

| Gate | Condition | Effect |
|---|---|---|
| **K1** | `A1 ≤ A2` on every cell ⇒ **derived-parameter claim WITHDRAWN AS UNNECESSARY** (prior-art-shaped transmitted-parameter copy-with-edits) | closure of Axis 2 |
| **K2** | `A3 ≤ A2` on every executable cell ⇒ mode 14 is **subsumed by an adopt-class filter**; file as `{engineering}`, no claim | retirement |
| **K3** | `A2 ≤ A0` by ≥ +0.10% on the PE stratum, or ≥ +0.05% on the full corpus ⇒ **KILL TCOPY** | economics |
| **K4** | `A5 − A2 ≥ +0.05%` on the full corpus, or `A5` encode < 0.75 × `A2` on the PE stratum, or `A5 − A2 ≥ 0` on ≥4 of 6 PE files ⇒ **KILL PNRA-current** (already satisfied by X3 — this is a confirmation run) | economics |
| **K5** | any roundtrip / fuzz / identity control fails ⇒ **VOID** | validity |
| **K6** | *(scope note, not a threshold change)* the dropped `A4` arm has no gate | — |

**Group B — requires a newly locked held-out executable family. BLOCKED.**

`B1`: `A? ≤ A2 − 0.50%` on a **newly locked held-out executable family** (the promotion gate).
`B2`: same-family dual bar. `B3`: paired R2 scout + peak RSS (**no existing artifact supplies
RSS**). No control-stratum evidence substitutes for these (X11).

### 7.5 Permitted terminal outcomes

**KILL** (K1/K3/K4) · **HOLD-ENGINEERING** (K2 fires without K1: mechanism retired to adopt-class,
novelty already withdrawn) · **HOLD-PENDING-LOCK** (nothing decisive). The job **cannot** return
PROMOTE. Pre-declared so no Group-A pass can be read as promotion.

### 7.6 Expected outcome, pre-declared

**K1 fires.** Three prior instances of transmitted-beats-implicit in this project (ARI-REF, PR-5's
step family, and now the rel32 geometry where the derived value *is* the transmitted distance), the
parameter axis is worth only ~0.036% on the one cell where density exists, and the claim it would
rescue is already worth ≤0.02% end-to-end. I expect the job to **close the family's last survivor and
confirm two KILLs**, leaving only `correctness SURVIVES` and `economics DEAD`. I want that outcome
and have written it down so it cannot be reinterpreted afterwards.

---

## 8. Strongest disconfirming evidence

1. **Three-axis dominance by brotli q6** (X2): +20.5% bytes, 6.7× slower encode, 2.8× slower decode.
2. **Whole effect 0.0566%**, and **6.88% worse than an existing mode** (X1); the measured A/B used
   the wrong baseline.
3. **PNRA-current net-worse, −33.7% aggregate encode**, whole gain from one file at −39.5% encode
   (X3).
4. **The shipped invariant is BCJ's canonical form by the repo's own comment** (X7), and now the
   *parameterization* is BCJ + a documented position origin (X8).
5. **H2's prize ≤0.02%** (X9) and **its decode cost 6–40%** of a byte budget (§4.3) — bad on both axes.
6. **My own parameter-axis arithmetic was wrong by 1000×** (§3.3) and my revision-2 framing of it
   was wrong in *direction*. Recorded, not buried.
7. **The density leg is section-only, single-host, and did not transfer** (§1.3).
8. **Promotion is blocked on corpus acquisition**, not on science (X10, X11).
9. **My mechanism will never be measured** (§6). I accept that.

---

## 9. Adversarial failure cases

1. **Recognizer/encoder disagreement** ⇒ silent corruption. Mitigation: encoder-side bit-exactness
   gate; harness asserts exact reconstruction; production relies on block CRC.
2. **Order dependence under overlapping copy.** Mitigation: non-overlap `i+4 ≤ d` is **hard** and
   asserted. (The critic notes lifting it needs periodic-transform algebra and is a prior-art
   capability we are not using — `VCDIFF` supports overlapping target copies. Recorded, not pursued.)
3. **Recognizer state leakage.** Mitigation: purity; assert in production.
4. **`C2` false positives on non-executable data.** Out of scope (§3, §6).
5. **Adversarial rate/decode attack.** Mitigation: bounded-constant ceiling as a fuzz target; the
   difference-cover / surprise-budget negative gate.
6. **Entropy-coder restructuring** when a stream is deleted. Mitigation: charge stream headers.
7. **Trajectory divergence flips signs** (E7). Mitigation: whole-parse arms only; **no post-hoc
   candidate dropping** — dropping always enlarges the block.
8. **Overfitting γ.** Reusing γ=0.5's *value* is a comparison convenience, not a licence to re-tune.
9. **Reference-class drift.** Any v2 citation co-lists xz-9e and carries `GRID-THIN`.
10. **Contamination.** Pooling the control stratum with a locked family, or citing the six PEs as
    held-out, is a protocol violation.
11. **Instrument contamination.** The A/B is cheap enough that instrumentation could swamp a
    633.75 B effect; an uninstrumented paired control is mandatory, and — given §3.3 — the
    byte-only tier needs **no instrumentation at all**, which is a further reason to keep the job at
    R1.
12. **Attribution trap.** A `K2` result (BCJ subsumes mode 14) must be labelled an **engineering
    retirement**, not a "we lost to prior art so the mechanism was wrong" verdict — the
    implementation was correct and inert.

---

## 10. Verification performed

- Read: `MASTER-BRIEF.md`; `RESEARCH_LEDGER.md` §660–839, 930–1039, 2437–2619, 3181–3498, 4470–4561,
  5050–5561; `docs/priorart-tcopy-external.md` (693 lines);
  `docs/gate-ruling-i9-pr5-position-derived.md` (251 lines); `docs/gate-priorart-audit-i8.md` §81–220;
  `docs/I10-BREAKTHROUGH-PROGRAM.md` §198–397, 528–647, 755–834; `FORMAT.md` §478–577;
  `09-exact-tcopy-pnra-fledge.md` (601 lines); `20-priorart-killteam-space-bunny.md` (956 lines,
  incl. finalized §16.2); `03-heldout-corpus-space-bunny.md`; `03-heldout-corpus-fledge.md`;
  `COORDINATOR-STATE.md`.
- Measured on this host (read-only): `tests/corpus/` listing (26 entries; PE sizes match E10); the
  `.github/workflows/` listing (17 files, none for this track).
- Prototype: compile-only, `EXIT=0`, `/W4 /WX`. **Not linked, not executed.**
- **Not performed:** no encode, decode, benchmark, sweep, fuzz, production build, source
  modification, or corpus role change.

---

## 11. Corrections I make to my own revisions

1. **H2 novelty withdrawn** (§5.3, four attempts recorded and failed).
2. **Arithmetic: 633.75 B / 0.0360%, not 0.63 B / 0.000036%** (§3.3). Consequence: the explicit-vs-
   implicit ablation is **resolvable**, not underpowered; `COLLAPSE-C3` becomes the job's
   highest-information arm. **Thresholds unchanged.**
3. `PROMOTE-TO-REMOTE` (rev 1) ⇒ **withdrawn** (rev 2) ⇒ **PILOT, closure-only** (rev 3).
4. "Recognizer ≈0.165 c/B / ~1.5%" ⇒ **1.0–4.5 c/B (6–40%)**, adopting Track 20's estimate over mine.
5. "Decoder memory delta 0 B" ⇒ **corrected**: state delta 0, but the 8th substream raises the
   allocation ceiling **+14.3%** (X4), and the PNRA index costs **~7–8 MB encoder RSS per block,
   uncapped** (X5).
6. "Verdict set: the six held-out PEs" ⇒ **control stratum / consumed anchors** (§2).
7. `C2 ABS32-P` ⇒ **out of scope**; `A4` ⇒ **DROPPED from the job** (§6, §7.1).

## 11a. Reconciliation with the critic lane and Track 20

- **Critic §11.1 (Pareto-competitive / transferable Linux figures):** **no**, and I withdraw the
  framing. §1.3 carries the mandatory label; I endorse the critic's action to relabel the CONTEXT
  TCOPY block.
- **Critic §11.2 (are A1 and A3 controls present?):** **yes, both** — arms A1 and A3 of §7.
- **Critic §11.3 (implicit ⇒ zero-bit cost):** **retracted.** Δ is zero-bit; topology is transmitted
  at 0.25 raw bit/byte; and §3.3 now puts a corrected number on the implicit half.
- **Critic §11.4 (BCJ subsumption):** **faced, and conceded** — X8 is decisive and I add no rebuttal.
- **Critic §11.5 (M1–M6):** **agreed in full**, each with its own provenance caveat, none restated
  as my own measurement. **No disagreement on any measured number.** The one disagreement I raise is
  against *my own* revision 2 (§3.3), not against the critic.
- **Track 20 §16.2:** adopted as decisive. Its "downgrade against my own earlier framing" is matched
  by mine.
- **Track 20 §7.2 prize and §12 gate architecture:** adopted, including the conservative scan estimate.

## 11b. Final ruling, per axis, then overall

| Axis | Ruling |
|---|---|
| **Correctness** | **SURVIVES** — retain the capability, the `FORMAT.md` mode-14 documentation, and the fuzz record |
| **Economics** | **KILL as implemented.** PNRA-current KILL; TCOPY HOLD-at-best (dominated by q6 on all three axes); SLX-REF KILL as a byte mechanism (≤0.02% prize, 6–40% decode cost) |
| **Novelty** | **H2 WITHDRAWN.** Axis 2 (derived vs transmitted) survives as the only narrow survivor and is worth ≤0.02%; the job exists to **close** it, not to promote it |

**OVERALL: PILOT — one byte-only remote job, `TCOPY-PROVENANCE-BCJ`, re-ranked P0 = A0/A1/A2,
P1 = A3/A5, A4 dropped, running as CLOSURE + ATTRIBUTION only. No mechanism build, no format
commitment, no Pareto claim, no novelty claim. Promotion blocked on corpus acquisition and,
independently, on a novelty question now closed.**

Two cheap parallel actions, both **`arch` lane** (I may not modify production source):
- **cost-model calibration** (critic §6): charge the per-token descriptor and the transform-mask bit
  rate inside `scan_candidate`. ~10 lines, no new mechanism, no new prior-art surface; converts an
  uncalibrated objective into an approximately-MDL one. Highest information per hour in this family.
- **cap the PNRA index** (X5): bound entries or window it; charge the encoder cost. `pe-git.exe`'s
  −39.5% encode is the signature to beat.

---

*Space Bunny Free, track 09, revision 3. Measured / cross-lane-measured / derived / estimated /
hypothesis / projection labels kept separate. No existing file modified; no reset, clean, stash,
restore, rebase, commit or push; no local benchmark, sweep or fuzz campaign. All measurement
GitHub-Actions-only, byte-only, strata reported separately, control stratum never cited as held-out.*