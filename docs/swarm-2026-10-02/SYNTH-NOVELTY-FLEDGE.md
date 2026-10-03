# SYNTH — Novelty Red Team, all tracks 01–20 — **Fledge Alpha Free**

**Deliverable:** `docs/swarm-2026-10-02/SYNTH-NOVELTY-FLEDGE.md`
**Role:** independent adversarial reviewer. **This file did not exist before this turn.**
**Pairs reconciled:** `20-priorart-killteam-space-bunny.md` (read in full), plus the fledge/space-bunny
pair for tracks 01–19.
**No measurement performed.** No local corpus benchmark, no fuzz campaign, no production edit, no
commit, no push.

Evidence labels, used strictly: `[MEASURED]` in-repo recorded measurement · `[RECORDED]` in-repo
recorded ruling/audit · `[DERIVED]` my arithmetic over a named input · `[FETCHED]` retrieved by me
this session · `[STANDARD]` long-established fact from my own knowledge, **not re-fetched** ·
`[GAP]` acknowledged coverage hole, **never a clearance**.

---

## 0. Verdict

**Of twenty tracks, six lanes still carry a novelty-styled claim. After independent attack, zero
survive as mechanism-level novelty, and the four-way decomposition of the surviving neighborhood
shows that the one cell the project has protected for six passes carries a ~0.04 % byte prize.**

The project's own paired critics have already converged on this from their own lanes — that is
recorded below and credited, not claimed as my discovery. My contributions are (a) the four-way
provenance decomposition that shows *why* the last cell is thin, (b) a definitional falsifier for
FLI that needs no prior art, (c) the CAM red-team, and (d) the consolidated claim-class list.

**Program verdict: HOLD. No lane earns a mechanism-novelty claim today.**

---

## 1. Method, and the gap discipline I am holding myself to

I did **not** run new literature searches for this synthesis beyond the ones recorded in my
`20-priorart-killteam-fledge.md` §4 (VCDIFF RFC 3284 `RUN`; bzip2 `RUNA`/`RUNB`; zstd FSE `RLE`;
FELICS; Dynamo-family trace compaction; decoder-complexity RDO). **Everything else below is an
attack on reasoning and composition, not a fresh search.** That distinction matters and I state it
plainly:

- Where I say **OCCUPIED**, I cite a retrieved or `[RECORDED]` artifact.
- Where I say **OBVIOUS COMBINATION**, I am making a *composition* argument: every component is
  individually occupied and the stated interaction adds no new step. This does **not** require a
  search, and it is the strongest form of kill available inside doctrine item 1.
- Where I say **`[GAP]`**, no one — including me — may convert it into a clearance. Standing gaps
  carried forward: Espacenet/Lens never queried in six patent passes `[RECORDED]`; Apple
  chained-fixups patent number unconfirmed; ACM/IEEE full text not queried; no post-2024
  general-purpose codec survey; LZMA2/7-Zip-fork/console-cruncher implementations not searched for
  a counted-repeat opcode; **LZMA block-mode cut selection not searched**; **learned/per-block
  codec-selection literature not searched** (constructive track-20 §16.5.1 marks both as `GAP`);
  patent-database search for "loop opcode" not run.

---

## 2. The test I apply

The project's own genus is unclaimable `[RECORDED: docs/gate-priorart-audit-i8.md:20-27]`:
*"encode a value relative to already-reconstructed context."* I add the decomposition that actually
decides the surviving neighborhood, and which the coordinator asked for explicitly. It separates a
mechanism claim from an engineering claim into four independently occupiable cells:

| Cell | Question | Occupant found | Status |
|---|---|---|---|
| **(P) Parameter provenance** | Is the transform parameter **transmitted** or **derived from decoder-visible state**? | transmitted = parametric dictionaries, US 7,676,506, `xz --delta`, Gorilla/FPC/Parquet `[RECORDED]` | **DERIVED is the only defensible cell** |
| **(T) Topology computation** | Is the **set of transform sites** transmitted, or computed? | **BCJ computes it, globally, content-determined, zero bits** (public domain 2008) `[RECORDED]` | **OCCUPIED** |
| **(L) Localization** | Is the kernel run **per stream / per section**, or **per backreference**? | BCJ `pos` is per-stream; xz `start_offset` is **per-section and shipped as an intended feature** (`SESSION`, constructive §16.2) | **NOT OCCUPIED, but a granularity argument only** |
| **(A) Anchor selection** | If ≥2 anchors are always decoder-visible, is the anchor **selected**, and is the choice transmitted or class-default? | VCDIFF/RFC 3284 already selects between **two** anchor spaces (source window vs already-decoded target window) `[FETCHED]` | **genus occupied at k=2; k>2 instance unoccupied; worth ≤2 bits** |

**The chart, applied to the one surviving neighborhood (TCOPY / PNRA / H2 / SLX-REF):**

| Element | Constructive lane | **My independent verdict** | Basis for the difference |
|---|---|---|---|
| (P) zero-bit Δ = −d from match distance | "the single strongest separator" | **AGREE it is the strongest cell, and it is the only one** | — |
| (P) *mass* of that cell | "≤ ~0.02 % end-to-end" | **DISAGREE on the number, AGREE on the conclusion — and I find an error** | `[DERIVED]` `5,070 phrases × 1 bit = 634 B = 0.0360 %`, not `0.63 B = 0.000036 %`. **Factor 1000.** Track 09 §3.1 is wrong. Conclusion (small) survives; the standing claim that the A1–A4 ablation is *"underpowered by construction"* is **false** — deterministic arms are exactly powered. |
| (T) computed topology | implicitly conceded to BCJ | **OCCUPIED** | BCJ computes site topology from content, zero bits, globally |
| (L) per-phrase localization | "assume obvious"; Axis 5 now load-bearing but a granularity argument | **AGREE, and stronger:** `bcj(pos = source_position)` applied to the destination span **is** Δ=−d. Same arithmetic, different register. A *granularity* change to a documented filter parameter is the textbook shape of an obviousness target. | I accept their `start_offset` evidence over my own `[STANDARD]` derivation |
| (A) anchor multiplexing | not addressed | **genus occupied at k=2** by VCDIFF's source-vs-target anchor choice; k>2 unoccupied but a ≤2-bit field | `[FETCHED]` RFC 3284 |

**Net for the surviving neighborhood:** one defensible cell (P), worth **634 B / 0.036 %** on the
only executable cell where density is recorded; three cells occupied or granularity-only. **A
0.036 % byte effect is not a mechanism contribution.** Both the constructive kill team and the
paired critics of tracks 09/13 now say this; I state it as the reconciled position and I do not
inherit it, I re-derive it.

---

## 3. Per-track attack, 01–20

Status key: `CLOSED` = no claim survives · `ENGINEERING` = real value, no novelty · `THIN` = systems
only · `RESIDUE` = a cell nothing occupies, too small to pursue.

| Track | Lane's own position | **My independent verdict** | The kill |
|---|---|---|---|
| **01** G5D paged dict | "no novelty claim"; TRGD offered as next experiment | **CLOSED / adopt** | Agrees with both lanes. Parquet dictionaries, Brotli static dict, zstd external/trained dicts, FSST, front-coding, BPE all recorded `[RECORDED]`. Nothing left. |
| **02** G5A planner / LCPS | "marginal-to-weak", defers to kill team | **CLOSED / adopt** | **OBVIOUS COMBINATION**: a column-major-emission surrogate + bounded finalist ladder + a price model is three shipped components (FastLanes/white-box for column-major, OpenZL for offline plan search, Brotli block splitting for routing). Candidate-new residual is "the surrogate is a *cheaper* price estimate" — a cost question, not a mechanism. Both critics already say this; **I concur and add that G4's measured negative (`INVALID_SPEED_ACCOUNTING`) makes the lane's own premise unmeasured.** |
| **03** Held-out corpus | protocol; self-test **failing**, 6 defects | **n/a (protocol)** | Correct. Note the standing irony: the corpus whose absence blocks track 09's `C2` is produced by the lane whose prototype does not pass its own tests. **Sequencing risk, flagged.** |
| **04** Dense frontier | refuses to emit a crossing token | **n/a (instrument)** | Correct and important: `BYTE-WIN ≠ FRONT-CROSSING` under PR-3. |
| **05** Backend frontier | "no novelty opening anywhere in the backend layer" | **CLOSED** | **AGREE, strongest in the swarm.** Backend layer = adopted algorithms. The only claim with any life is a *memory-safety obligation* from interleaving, which is a correctness property, not a mechanism. |
| **06** Entropy co-design | "NIL novelty"; explicitly does **not** contest track 15 | **CLOSED** | rANS/FSE/tANS/ANS layout work is settled infrastructure `[RECORDED]`. Correctly declines the contest with track 15. |
| **07** BWT subblocking | adopt-class, explicit KILL on the constructive direction | **CLOSED** | Partitioned BWT is 30-year-old art. |
| **08** DEFLATE reconstruction | adopt-class, no novelty claim (ruling V-4) | **CLOSED** | preflate/reflate/grittibanzli. Adopt and integrate — it is the largest byte win in the record (1,362,177 B on mozilla `[MEASURED]`) and it needs **no novelty defence at all**. |
| **09** TCOPY/PNRA / SLX-REF | space-bunny: PROMOTE-TO-REMOTE byte-only; fledge: N1/N3 withdrawn as anticipated, N2 remains | **THIN → HOLD** | §2 applies. **I upgrade my own interim**: after the `start_offset` evidence, `C1`-only is localized BCJ. `C2` is the only residue and it is corpus-blocked. **Both the constructive and critic lanes have moved down; the coordinator must not read the space-bunny lane's `PROMOTE-TO-REMOTE` as still live** — the paired critic and the kill team both overrode it. |
| **10** Correction topology | adopt-class concession stands even on a positive oracle | **CLOSED** | Killed by *measurement* (masks ~90 % unique, modal 17–23 %) before prior art was even reached. Correct ordering. |
| **11** FLI | space-bunny: PILOT on S1+S2; fledge: kill as mechanism; **Amdahl ceiling 1.12×** | **KILL as novelty; HOLD as engineering** | See §4 — my definitional falsifier, which needs no search. |
| **12** Encoder-only search | "Theorem N is not a compression novelty claim"; branch F adopt-class | **CLOSED** | Self-discharge is correct and rare; I commend it. The one live item is *corpus memorization* (an overfitting risk), which is a **falsification discipline**, not a novelty claim. |
| **13** Numeric compiler / CANL | space-bunny: adopt-class bar, no novelty claim; fledge: **rejected** track 13's own D1 ("the rebase rule … the entire novelty surface") as placement engineering | **CLOSED / adopt** | The fledge critic is right and its kill is the right one: `CANL` re-derives Gorilla/ALP/FastLanes lane extraction, then the only "new" surface is *where the rebase is applied* — which is (L), the cell we just priced at 2 bits. **This is the cleanest example in the swarm of a lane's own headline novelty reducing to cell (L).** |
| **14** Executable compiler | adopt-class; proposes BCJ as a pre-transform, "no novelty claim" | **CLOSED / adopt** | Correct, and note the irony: the lane proposing BCJ as an **adopt-class** pre-transform is simultaneously the lane whose sibling proposes BCJ's *per-reference localization* as novel. **Those two positions cannot both be held.** Track 14's own reading should govern. |
| **15** CAM / cross-block memory | fledge: **"the surviving novelty claim contradicts the specified mechanism"** and **"surviving novelty claim and specified mechanism are mutually exclusive"**; 06 says NIL | **CLOSED as a standalone mechanism** | §5 below — my independent red-team. The fledge critic found an *internal* contradiction before I found the prior art. That is the single highest-value critic finding in the swarm. |
| **16** Segmentation / routing / CDR | space-bunny: generic routing CLOSED; rank-1 residual "DIFFERENT but weak"; fledge: prior-art risk **HIGH not MEDIUM-HIGH**, BLOT / blocally-compressible MDL | **CLOSED / measurement question only** | I concede the constructive lane's evidence is better than mine (Brotli `block_splitter.go`, zstd `zstd_preSplit.h` + `targetCBlockSize`, RFC 8878 raw fallback, all `SESSION`-quoted). **Add the fledge critic's BLOT/bLocally finding** — that closes the last cell. **OBVIOUS COMBINATION** independently: CDC anchor sampling + cost-estimate pruning + rank-1 reduction. |
| **17** Parser algorithms | novelty claim "must be about the interaction" | **CLOSED** | SIMD match extension decided prior art (LZMA SDK / zstd). An "interaction" claim over two occupied components with no new step is doctrine item 1. |
| **18** Decoder architecture | "no novelty claim authorized on the composition"; resolved-graph = OpenZL | **CLOSED / adopt** | **OpenZL (`facebook/openzl`, arXiv 2605.09928) owns "graph of reversible ops + resolved frame + one universal decoder."** That was the project's own candidate and it is occupied. Correctly surrendered. |
| **19** Format / security / fuzz | fledge: accepts 2 of 3 novelty claims, rejects priority; decompression-bomb caps = adopt | **ADOPT — the one lane with real, unoccupied value** | Amplification caps (`ZSTD_d_windowLogMax`, `BROTLI_DECODER_PARAM_LARGE_WINDOW`, `dicSize`) are occupied; **bounded-allocation and corruption-containment *as a format contract with a testable property* is not.** The fledge critic is right to reject the third novelty claim on priority while endorsing the engineering. **I support the PILOT and I would rank it first among the twenty lanes on value-per-cost.** |
| **20** Prior-art kill team | PILOT (constructive) / HOLD (mine) | **HOLD** | §11 of my track-20 file. Label difference is immaterial; content converges. |

---

## 4. FLI: a definitional falsifier that needs no prior art

The coordinator asked me to attack FLI S1–S4 without inheriting the constructive lane's clearance.
I concede S1 to them (§2/§3), and it changes nothing, because FLI's dominant case fails on
arithmetic alone.

**Claim.** `LOOP_EQ` (body class `c`, run length `k`, `L_i = L₀`, `D_i = D₀ = last[shape_c]`) is
**character-identical to a single LZ77 match of length `k·L₀` at distance `D₀`**, under the
overlapping/periodic copy semantics the project already implements for modes 11–15.

**Proof.** The decoder executes `emit_copy(D₀, L₀)` and advances `pos` by `L₀`, `k` times.
`emit_copy(D₀,L)` copies from `pos − D₀`; since `pos` advances, iteration *i* writes
`out[i·L₀ + t] = history[i·L₀ + t − D₀]`, reading back bytes emitted by earlier iterations. That is
precisely the definition of one periodic match `(D₀, k·L₀)` with `D₀ < k·L₀` — permitted, charged
once, by DEFLATE, by LZMA, and by this project's own modes. ∎

**Consequences the swarm has not drawn:**

1. `LOOP_EQ` introduces **no new decoder state and no new representation**. It is a
   *token-descriptor run-length code*, and the baseline format already contains the construct it
   encodes. Its byte win is bounded above by the descriptor redundancy it removes — and it is
   obtainable **with no new opcode at all** by RLE over the match-descriptor stream.
2. FLI's remaining surface is therefore **not "one weak claim"** but **one weak claim about the
   minority opcode** (`LOOP_ARITH`) plus a descriptor-RLE device. That is thinner than the
   constructive lane's "LIVE on S2 only".
3. **The decisive control is missing from every FLI arm list in the swarm.** The controls are RePair
   grammar (`A4`) and materialization (`A5`). The decisive one is **descriptor-RLE on the same
   parse**: if it captures ≥50 % of `LOOP_EQ`'s win, the mechanism is dead with no build.
4. **Independent confirmation from the paired critic, which I did not supply:** track 11's fledge
   reports the removed work lives inside a pool at **11 % of whole-codec decode — an Amdahl ceiling
   of 1.12×**. FLI's own `G6` demands **≥1.08× with a CI excluding 1.00**. **The mechanism's decode
   gate is pre-falsified by a ceiling that leaves almost no room.** Two independent arguments, one
   definitional and one quantitative, from different agents, both killing the same thing.

**FLI verdict: KILL as a mechanism claim. HOLD as adopt-class engineering, bounded by two counters.**

---

## 5. CAM (Track 15) — independent red team on *semantics*, not the name

The instruction was to search for **independently decodable blocks carrying compact entropy/context
model initial state or priors specifically to avoid cold-start/warmup cost**, and to identify the
narrowest remaining separator if any.

**What I found (partial — and I mark my own coverage as a gap):**

| # | Artifact | What it establishes | Class |
|---|---|---|---|
| 1 | **PPMd variant H** — SEE / restart method (`bitplane/rar-research`, `PPMD_ALGORITHM_SPECIFICATION.md`) `[FETCHED]` | A context-mixing coder that maintains **surviving escape-frequency statistics across model resets** precisely so that a reset costs less. **This is "carry model state across an independently-resettable unit to avoid warm-up", in a shipped compressor.** | direct |
| 2 | **Brotli RFC 7932** context-map reuse across meta-blocks `[RECORDED]` | Context-model *structure* is reused between independently-decodable meta-blocks. | direct |
| 3 | **LZMA** continuous range coder across sync-flush boundaries vs `reset` | The entire design axis ("reset or carry the model across block boundaries") is a decades-old, settled trade. Constructive track 20 and lane 06 both already say so. | direct |
| 4 | **JPEG XL modular mode**: per-group adaptive histograms / shared ANS cluster maps with groups separately decodable `[STANDARD — **detail NOT verified this session; treat as a lead, not a citation**]` | Would be the nearest clean instance. **GAP.** | lead |

**My decisive point is architectural, and it needs no search at all.**

> **In a static-table rANS/tANS format, transmitting the model *is* transmitting a prior.** The
> cold-start cost a "model prior" mechanism targets decomposes into (i) the entropy coder's own
> running state `(x, r)` — which rANS **already carries across every block boundary for free**, and
> (ii) the frequency **table**, which the format must transmit at block start anyway. There is no
> third component for a "prior" to fill. **In a static-table architecture the CAM idea is largely
> vacuous, because the format already ships a full prior at every independently-decodable boundary.**

That is why lane 06 independently returned **NIL** novelty and declined to contest it — two agents,
opposite roles, same conclusion, arrived at from different directions.

**The narrowest remaining separator, stated exactly:**

> *Transmit a **delta** on the previous block's entropy/context model at an independently-decodable
> boundary, instead of a full model, and reconstruct it with a bounded, verifier-checked update.*

**Verdict on the separator: RESIDUE, too small to pursue.**
- The genus — multi-source / carried context state across independent blocks — is occupied by
  artifacts 1–3, and dictionary-priming is occupied generally (adopt-class, track 01).
- The residual is a **table-delta**, i.e. a cheaper encoding of something already transmitted. Its
  prize is bounded by `(full model − delta)` bytes per block, and the block size is a free parameter
  the encoder already controls.
- The fledge critic's independent finding settles it regardless: the surviving novelty claim **and**
  the specified mechanism are *"mutually exclusive"* — the claim describes carrying state across
  units, the mechanism describes something that cannot cross them. **You cannot ship both.** That is
  an internal contradiction, and it is dispositive before prior art is even reached.

**CAM verdict: CLOSED as a standalone mechanism. Track 15's engineering (real cross-block reuse
with real amortization) should proceed and be labelled adopt-class.**

---

## 6. The obvious-combination register (the thing the coordinator asked me to flag)

These are cases where **every component is individually occupied and the stated interaction adds no
new step**. Under doctrine item 1 these are closed regardless of measurement outcome, and no gate
pass can revive them.

| Lane | The stated "new interaction" | Components, each occupied | Verdict |
|---|---|---|---|
| 02 LCPS | column-major surrogate + finalist ladder + price model | FastLanes/white-box (column-major) · OpenZL (offline plan search) · Brotli (block-split routing) | **obvious combination** |
| 11 FLI (composition) | counted loop + zero-bit operands + fused execution | superinstructions (interpreter) · rep-offset reuse (LZMA/Brotli) · streaming fusion (every shipped LZ decoder) | **obvious combination** — and *additionally* redundant with a single match (§4) |
| 13 CANL | typed lane extraction + rebase | ALP/FastLanes (typed lanes) · BCJ/delta (rebase) | **obvious combination**; residual = cell (L), priced at 2 bits |
| 16 CDR | O(1) probe + family choice + rank-1 calibration | CDC anchor sampling · learned-index cost pruning · PCA/rank-1 | **obvious combination** |
| 09/14 conflict | per-reference transform | BCJ (`pos`) · xz `start_offset` · copy-with-edits (VCDIFF) | **obvious combination** at cell (L) |
| 10 topology | entropy-coded correction topology | VCDIFF ADD · zdelta mismatch lists · masking filings | occupied, and already killed by measurement |
| 18 resolved graph | graph of ops + resolved plan + universal decoder | OpenZL owns it outright | **occupied, not merely a combination** |

---

## 7. Claim classes that are actually defensible today

Short list, in descending order of defensibility. **Each is `{engineering}` or `{adopt-class}`
unless marked otherwise. None is a mechanism-novelty claim.**

1. **Adopt-class infrastructure, high measured value, zero novelty defence needed.** DEFLATE
   reconstruction (1,362,177 B recovered on mozilla `[MEASURED]`); aux-index inverse BWT
   (1.364×/1.795×/2.339×, +8,083 B charged `[MEASURED]`); store-path CRC via PCLMUL (4.5× enc /
   3.4× dec, wire-invisible, signed `[MEASURED]`); ALLOC (1.352×) and decode leg 4 (1.179×), both
   wire-invisible and both retained `[MEASURED]`.
2. **Measurement and adjudication apparatus** — the frozen 13-file grid, the dual bar, the
   same-transform controls, and the byte-exact frozen canonical binary. As far as this session's
   searches establish, **no shipped general-purpose codec paper reports a candidate mechanism
   against its own same-transform control.** This is a genuine contribution class and it is the one
   thing here nobody else owns. `[GAP]` on the negative — a proper literature pass has never been
   done.
3. **Format-safety contracts** — bounded allocation, deterministic resource ceilings, and
   corruption containment *stated as a testable property of the format* (track 19). Engineering
   value high, novelty narrow but not obviously occupied, **and it is the only lane where I would
   spend money first.**
4. **Transform-representation engineering** on executables — BCJ as a pre-transform (track 14's own
   reading), executable pre-LZ normalization, decode co-design. Fully adopt-class; enormous prior
   art; explicitly *not* frontier language.
5. **Reproducible held-out corpus protocol with leakage control** (track 03). Methodology class,
   not a mechanism. **Currently blocked by its own failing prototype — fix that before anything
   else, because track 09's `C2` is corpus-blocked on it.**

**And the one honest caveat.** Cell (P) — zero-bit, geometry-derived, *exact* transform parameters at
copy level — remains formally unoccupied in a general-purpose compressor after six patent passes and
two independent agent reviews. I am not going to pretend that is nothing. But it is priced at
**634 B on a 3.26 MB `.text` cell (0.036 %)** `[DERIVED]`, its `C2` extension is corpus-blocked and
Courgette-adjacent, and **a 0.036 % byte effect does not survive doctrine item 2 as a
frontier contribution.** It is a paper paragraph, not a mechanism.

---

## 8. The cheapest decisive next experiment — one, remote-only

**`killteam-counters-r1`, R1 byte/counter-only, one GitHub Actions job, zero timing, zero builds.**
Detailed in `20-priorart-killteam-fledge.md` §10. Ordered arms, cheapest first:

| Order | Arm | Decides | Cost |
|---|---|---|---|
| **1** | **K3** — publish the `λ · c` arithmetic (paper only, no code) | Whether **any** decode-cost-gated representation on tracks 05/06/11/16/18 is optimizing a real quantity. If `λ·c < 1e-3` bytes per suppressed symbol, **all five lanes are mis-specified** and stop. | an afternoon |
| **2** | **K1** — measure `μ`, the fraction of bytes inside fully-`C1`-classifiable phrases, via counters on the existing `--pnra=on` path | The entire H2/TCOPY mechanism, against a `[DERIVED]` break-even of **μ ≥ 8.4 %** required to clear its own 0.5 % gate. **Builds nothing.** | one instrumented build |
| **3** | **K4** — descriptor-RLE control on the frozen mode-15 parse | Whether FLI's `LOOP_EQ` is redundant (§4). A **negative** here kills FLI with no mechanism build. | byte-only |
| **4** | **K2** — per-iteration entropy-pull / varint-read counters | FLI's `G1`, *after* the Amdahl ceiling has already told us the ceiling is 1.12× against a 1.08× bar. | one instrumented build |

**Frozen thresholds** (unchanged from my track-20 file §10.1, restated so they cannot drift):
`μ ≥ 8.4 %` on ≥4/6 held-out PEs, else **KILL on rate** · `A3 < A4` (global BCJ, same backend) and
`A3 < A5` on **every** held-out cell, else **FRONT-GAP** · descriptor-RLE captures <50 % of
`LOOP_EQ`'s win, else **KILL the mechanism** · per-iteration entropy+dispatch ≥25 %, else **KILL the
decode leg**. **`G0` (prior art) is already NO-GO and no datum can upgrade a claim class.**

**Mandatory controls:** feature-off byte-identity against the frozen binary · `repeat.jsonl`
no-regression · `random.bin` unchanged · byte-exact roundtrip every file every arm · forced-registry
coverage for any new mode ID · pinned seed/CI · every raw counter row retained.

**Non-negotiable:** R1 is byte-only, so the output is `BYTE-WIN` or nothing. **PR-3 forbids a
`FRONT-CROSSING` from a bytes-only oracle.** Any lane reporting a crossing from this run breaches the
contract and the artifact is void.

---

## 9. What would change my verdict

Stated in advance so it cannot be reinterpreted afterwards.

1. **A fetched, claim-level citation showing that none of** VCDIFF `RUN`, bzip2 `RUNA`/`RUNB`, zstd
   FSE `RLE`, PPMd SEE/restart-method, Brotli cross-meta-block context reuse, OpenZL resolved-graph,
   xz `start_offset`, or CDC anchor sampling **anticipates the specific cell being claimed**, *and* a
   proof that the claim is not an obvious composition of them. I found none.
2. **A byte demonstration that `μ` is far above 8.4 %** on held-out PEs, combined with a C-Σ test
   showing reference-local placement beats global BCJ by more than the 0.036 % the parameter cell is
   worth. Both would have to hold. I expect the second to fail.
3. **A demonstration that `LOOP_EQ` is not expressible as one periodic match.** This is a definitional
   identity; no citation or measurement can overturn it.
4. **A calibration of `λ · c`** that makes decode-cost gating a real decision variable. This would
   revive S4 and the CDR residual — and it is the cheapest item on the list.

**Gaps I am not converting into clearances, restated:** Espacenet/Lens unqueried across six patent
passes; Apple chained-fixups number unconfirmed; ACM/IEEE full text unqueried; no post-2024 codec
survey; LZMA/7-Zip-fork/console-cruncher implementations unsearched for a counted-repeat opcode; LZMA
block-mode cut selection unsearched; learned/per-block codec-selection literature unsearched; JPEG XL
modular cross-group model sharing **unverified this session**; `[STANDARD]` items (bzip2
`RUNA`/`RUNB`, zstd FSE `RLE`, JPEG-LS/CALIC, DynamoRIO, CDC/learned-index routing, differentiable
compression, FE/FPC) **not re-fetched**. A search that returns nothing is a gap. It has never once
been a clearance in this project, and it is not one here.

---

## 10. Final ruling

# **HOLD**

Not KILL: the measured byte and decode wins in classes 1–3 of §7 are real, citable, and integration-
value, and one narrow cell remains formally unoccupied.
Not PILOT-PROMOTE: **no lane in twenty holds a mechanism-novelty claim that survives prior-art and
obviousness analysis**, and the one unoccupied cell is priced at 0.036 %.

- **KILL as mechanism novelty:** FLI (11) — definitional redundancy plus anticipated components ·
  CANL (13) — placement-only residue · CDR (16) — obvious combination plus three shipped occupants ·
  CAM (15) — genus occupied, and internally contradictory per its own critic.
- **CLOSED / adopt:** 01, 02, 05, 06, 07, 08, 10, 12, 17, 18 — each already correctly self-labelled by
  its own lane and confirmed here.
- **HOLD as a claim, engineering screen only:** 09 TCOPY/PNRA/SLX-REF — cell (P) alone, at 0.036 %.
- **ADOPT, ranked first on value-per-cost:** 19 format-safety contracts.
- **BLOCKED on sequencing, not on mechanism:** 03 (fix its failing prototype) → then 09's `C2`, the
  only structurally unoccupied residue in the program.

**One cheapest decisive next experiment:** `killteam-counters-r1`, arms **K3 → K1 → K4 → K2**, one
remote byte/counter-only CI job. K3 and K1 cost an afternoon each and can each end a lane.

---

*Fledge Alpha Free — independent adversarial reviewer, novelty synthesis across tracks 01–20.
Prior-art classification and experiment-design review; **not legal advice**. FTO counsel review
remains recommended before any commercial claim. Companion: `20-priorart-killteam-fledge.md`
(reconciliation with the paired constructive kill team is complete at its §11). No measurement was
performed in this document; every number is either cited to an in-repo artifact or labelled
`[DERIVED]` with its arithmetic shown.*
---

# §11 — Addendum: remote-experiment-queue novelty audit

*Appended after §10. Read with `docs/swarm-2026-10-02/REMOTE-EXPERIMENT-QUEUE.md` §"Novelty red-team
review (Fledge Alpha Free)", where the explicit queue changes NQ-1…NQ-10 are filed. §10's verdict
stands; §11 adds one item that now ranks ahead of my §8 arm 1.*

## 11.1 The audit question

*"Try to kill every surviving novelty implication."* Concretely: **after localized-BCJ mapping,
does any TCOPY/PNRA claim remain, and does any Track 01/05/06/10/15/18 engineering item accidentally
carry novelty-by-difference?**

**Answer to part 1 — No claim remains, and the item should be unfunded rather than downgraded.**

| Cell | Occupant | Status |
|---|---|---|
| (P) zero-bit, distance-derived parameter | none found in six patent passes | **unoccupied but priced at 3.6–10 B** (queue L136) / 634 B = 0.036 % `[DERIVED]` |
| (T) computed topology | BCJ, global, zero bits | **OCCUPIED** |
| (L) per-phrase localization | `bcj(pos = source_position)`; xz `start_offset` ships the position origin as an *intended* feature | **granularity only** |
| (A) anchor multiplexing | VCDIFF selects between two anchor spaces `[FETCHED]` | **genus occupied at k=2** |

**The claim-closure value is real and it costs zero remote compute.** A documented negative that
closes the program's last open novelty neighborhood is worth a paragraph in a paper; it is not
worth an Actions job. The queue's own "Optional claim-closure only" therefore over-funds it. Filed
as **NQ-10**: `CLOSED — UNFUNDED`, with exactly one named re-entry trigger — track 09's `C2`
(ABS32, `Δ=+d`), the one cell no whole-file filter can reach because its anchor is the *reference
distance*, which does not exist to a whole-file pass — gated on `Q1a` returning a locked, genuinely
new, real executable family.

**Answer to part 2 — Six items carry accidental novelty-by-difference.** Not because the jobs are
wrong, but because **the queue has no mechanism preventing a positive engineering result from being
written up as a mechanism result**, and ranking rule 5 is prose.

| Item | How it launders | Filed |
|---|---|---|
| **Q3** (05/06/18 selector arbiter) | The resurrection vehicle for the dropped **S4** claim. If `A1` beats `A0`, the write-up is "we improved the decode-cost model." Also: `"no wire proposal yet"` is an open door. **And it hardcodes `lambda=0.01` while the project has never published `λ·c`** — so an A0-vs-A1 comparison is currently *uninterpretable*, not merely imprecise. | NQ-2a/b/c |
| **Q5** (10 MASK-CEILING) | Emits an "ideal replacement ceiling" for a mechanism **killed by measurement** (masks ~90 % unique, modal 17–23 %). A large ceiling plus large current mask wire reads as headroom. The mechanism is dead; the *job* must not be able to resurrect it. | NQ-3 |
| **Q6** (01 G5D) | Track 01's own text claims newness *"only in the sense that no prior dictionary format performs the…"* — **novelty-by-difference in its own words**. Its paired critic already killed it; the queue inherited the framing in prose. | NQ-5 |
| **Q4a/Q4b** (05/18 BWT) | Good Amdahl gate, unenforced label. Interleaved inverse BWT is standard (`libsais_omp`). | NQ-4 |
| **B4** (15 CAM) | Revisit condition contains a **non-null**: *"model-carryover nulls"* presupposes the mechanism it is meant to falsify. Plus the vacuity finding — in a static-table rANS format the transmitted model *is* the prior and rANS already carries `(x,r)` across boundaries free. | NQ-7 |
| **Q2** (04/15 parity) | A block-size finding is a **configuration fix** one write-up away from "adaptive blocking is our mechanism" — occupied by Brotli's meta-block splitter and zstd's `targetCBlockSize`. | NQ-8 |

Plus one hole in a job I otherwise endorse: **Q7** (DEFLATE reconstruction) gates only on
`precomp→xz -9e` and `precomp→brotli q11/lw30`, but `q11/lw30` is not the densest Brotli point and
`GRID-THIN` is resolved only on the frozen dense grid — so a win against `q11/lw30` alone can be a
**bracket artifact**, the exact failure already recorded for `synth-columnar-align.bin`. Filed as
**NQ-6**.

## 11.2 The structural fix, and the one item that now ranks first

Two queue-wide gates, **NQ-9**:

- **`QP-NOVELTY`** — a job whose `Class:` is `adopt-class` / `{engineering}` / `{synthetic}` /
  `{measured}` MUST NOT emit novelty, mechanism, contribution, or frontier language. Its result may
  authorize exactly one thing: an **integration or validation** run. Promotion to a mechanism run
  requires a separate pre-registration that survives the four-cell decomposition. **Not relaxed by
  passing `Q1` or `Q0c`** — those validate machinery, not claims.
- **`QP-NOVELTY-2`** — no job may hardcode a shared cost weight without a published dimensional
  calibration. *This is the one that bites Q3.*

And the missing job, **NQ-1**, which three independent agents reached and the queue contains none of:
> **`Q0c` — `λ · c` calibration publication.** Paper only. Zero codec invocations, zero corpus bytes,
> zero runner. Blocks `Q3`. Dispatch order becomes `Q0 → Q0c → Q1a → Q1b → Q2 → Q3`.

`Q0c` is my §8 arm K3 renamed. It therefore **ranks first**, ahead of the `μ ≥ 8.4 %` counter (K1),
and the ranking is unchanged in substance: `Q0c` can return a decisive negative, in which case `Q3`
should be **cancelled rather than re-tuned** per the queue's own invariant.

## 11.3 Verbatims preserved

**Novelty honesty is currently a policy, not a gate.** Ranking rule 5 says a claimed separator must
survive semantic prior-art mapping. Nothing checks it. Six funded jobs can emit a number that looks
like a frontier win, and in every one of those six the *experiment is fine* — the write-up is where
the claim is manufactured. So the fix is not to cancel engineering. It is to make the label
unenforceable-by-accident.

**And the honest limit, unchanged:** every `GAP` in §9 is still a gap. Six patent passes never
queried Espacenet or Lens. No post-2024 general-purpose codec survey exists. LZ-family
implementations were never searched for a counted-repeat opcode. LZMA block-mode cut selection and
the learned/per-block codec-selection literature are unsearched. JPEG XL's cross-group model sharing
is unverified. **No amount of internal agreement between two agents on the same thin record turns a
gap into a clearance.** The queue's own line is right and I restate it as my own: *a search gap is
not novelty clearance.*