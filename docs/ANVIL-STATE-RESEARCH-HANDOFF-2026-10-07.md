# PROJECT ANVIL — COMPREHENSIVE STATE, FRONTIER AND RESEARCH HANDOFF

> **Later October 7 evidence supersedes the time-of-inspection publication status below:** Q1a-XRUN [37694740386](https://github.com/thelabcorner/anvil/actions/runs/37694740386) independently cleared **R2 Gate A** (but **not Gate B**). [I18 corrected run 37703141058](https://github.com/thelabcorner/anvil/actions/runs/37703141058) produced synthetic numerical discovery; [I19 source attestation](I19-PHASE0-SOURCE-ATTESTATION-CLOSEOUT-2026-10-07.md) then verified four raw source bodies and four typed projections, **without any I19 codec efficacy benchmark**. The handoff's `PUBLICATION REQUIRED` discussion is a historical observation, not the live Gate A status. See [current evidence register](RESEARCH-EVIDENCE-INDEX-2026-10-07.md) and [research synthesis](ANVIL-RESEARCH-SYNTHESIS-AND-OPEN-QUESTIONS-2026-10-07.md); historical Class-A still **0 complete crossings**.
**Audit date:** 2026-10-07 (America/Chicago)  
**Classification:** Single-agent, read-only research reconciliation plus new documentation. **No delegates or swarm members contacted, spawned, awakened or resumed.**  
**Local source:** C:/Users/slooshied/Documents/ANVIL (OXP documents root: /documents/ANVIL)  
**Live Git checkout on inspection:** branch **i10-aux-unbwt**, HEAD **a5df2a5**; origin/i10-aux-unbwt-swarm-closeout points to that same HEAD at the time of inspection.  
**Authoritative research/control-plane freeze:** docs/swarm-2026-10-02/FROZEN-CLOSEOUT-MATRIX-R2.md (**ANVIL-CLOSEOUT-2026-10-02-R2**), augmented by docs/swarm-2026-10-02/REMOTE-PUBLICATION-MANIFEST-R2.md.  
**New context seed:** [RESEARCH-SEED-MIDDLE-OUT-2026-10-07.md](RESEARCH-SEED-MIDDLE-OUT-2026-10-07.md) — 2017 middle-out SIMD/XOR time-series compressor, reviewed as comparison and inspiration only.

> **Binding result: zero verified complete Pareto frontier crossings.** ANVIL has won compressed bytes on canonical inputs and has credible, sometimes large isolated decoder improvements, but its current rate-optimal path still loses substantially on decode work and memory. The immediate blocker to trustworthy next research is **clean, source-identified GitHub Actions evidence**, not a need to synthesize more untested codecs. The 40-agent discovery phase is CLOSED, and the frozen R2 control plane expressly forbids dispatch before source publication and attestation.

---

## 0. Scope, method, evidence and exclusions

This handoff reconciles the **live** repository, the history retained in research files, the final post-swarm October 2 R2 closeout, the existing workflow inventory, and upstream evidence from the middle-out repository. It is a status synthesis, **not** a new empirical compression experiment and **not** an override to R2. The current working tree contains extensive legitimate research and generated artifacts: git status reported **424 entries** (including tracked edited research/code, untracked prototype files and build products). No changes to those existing files were made by this audit; the only intended mutation is creating this document and its separately scoped research seed.

### 0.1 Evidence labels (binding on every claim)

- **MEASURED**: recorded bytes, timing, correctness or error from a named immutable local or remote experiment; the source and measurement window are specified.
- **DERIVED**: algebra from named measured/retained data, not independently timed.
- **RETAINED / HISTORICAL**: measurements from an earlier fixed binary, host or protocol, valid only in their original scope.
- **REVIEWED SOURCE**: documented algorithm, source behavior, or workflow/static inspection without executing a new test.
- **HYPOTHESIS / PROPOSED**: mechanism or experiment not demonstrated or not authorized.
- **UNRESOLVED / UNAVAILABLE**: absent reproducible evidence; do not equate with zero effect.

Decision priority is: source-of-record exact artifacts and CSVs > verified source/format > dated gate ruling and derivations > research ledger > historical context briefs. R2 supersedes R1 and all older queues **for execution decisions only**; old artifacts can still establish their own measurements. An old worker's suggested experiment is **not** a dispatch instruction.

### 0.2 Work not performed

No delegated OpenFork session, background child, new swarm, compute campaign, codec build, profiling, corpus benchmark, fuzz campaign, GitHub Actions dispatch, commit, push, reset, stash, clean or rebase was performed. Heavy compute remains **GitHub Actions only**. There are no new numeric results, no simulated confidence intervals, no prospective frontier upgrades and no new held-out corpus claims in this report.

### 0.3 Remote publication state

The public repo is https://github.com/thelabcorner/anvil (default branch main). The local R2 closeout and XRUN workflow are **not available at their requested paths on the public i10-aux-unbwt branch** at this audit: the remote file reads of R2 and .github/workflows/anvil-q1a-xrun.yml returned not found. Local .github/workflows contains 16 pre-existing historical workflows, not an installed XRUN/Q2 workflow. This is consistent with R2's **PUBLICATION REQUIRED** state. Do not infer an unpublished local document became reproducibly executable merely because some earlier changes are on a different remote tracking ref.

---

## 1. Executive operating verdict

| Dimension | 2026-10-07 finding | Claim ceiling |
|---|---|---|
| Research ambition | General-purpose lossless compression better than established reference Pareto points; mechanism novelty is desirable only if genuine and valuable | Objective, not accomplishment |
| Frozen Class-A Pareto verdict | **435 DOMINATED, 28 DEGENERATE, 5 unresolved FRONT-GAP, 0 FRONT-CROSSING** out of 468 accounted cells | RETAINED; 5 gaps are not wins |
| Canonical maximum-ratio bytes | ANVIL auto **46,446,995 B Silesia; 23,534,368 B enwik8**, beating listed xz/Brotli byte totals | MEASURED historical byte advantage |
| Ratio-path decoding | I9 large deficit; integrated auxiliary BWT narrows but **does not close** the external Brotli/xz gap | MEASURED same-job comparisons, still short |
| Aux-index inverse-BWT | **1.3645×–2.3392×** paired whole-codec speed gains on three targets for +2.5–3.1 KB each | ADOPT-CLASS engineering; default off |
| Structured representation | G3 discovery **−5.5984%** vs same-backend raw Brotli; V1 **+3.1900% worse** | Causal, narrow; not generalization |
| G5A chunk ordering | **−6.1189%** vs frozen A1 on D1–D4, predominantly column ordering; V1 adverse | Specific retained discovery effect |
| G5B ordinal coarsening | **+1.6507% worse** than G5A floor on D1–D4 | Rejected tested ordinal proxy |
| Funded novelty | **Zero** surviving mechanism-level novelty claims after prior-art review | No novelty promotion |
| Corpus | Q1a tool selftest 135/135; current 24-object operator audit INVALID_INFRA, c9 not verified; zero newly admissible held-out numeric/structured families | Infrastructure prepared, Gate B blocked |
| GitHub Actions | 16 historical workflows present; freeze requires exact source/producer/artifact pins and clean new XRUN publish | Gate A unpublished/unattested |
| First remote work | Q1a-XRUN zero-codec **after** clean approved publication; never skip this source-attestation gate | BLOCKED for now |
| Security | Reachable declared-total/work amplification in existing decode bounds; Q8 remote synthetic probe planned | Engineering/security, not novelty |
| middle-out | Bit-exact eight-lane XOR/AVX-512 time-series algorithm; new context seed created | UNTESTED external reference |

**Decision:** treat ANVIL as a research platform with several demonstrated rate/performance submechanisms, **not** as a product that has yet beaten Brotli on a fully measured, non-dominated general-purpose frontier. Concentrate on repairing causal-evidence infrastructure, fair geometry and strong controls; forbid retroactively reinterpreting a byte win or a prototype speedup as a frontier crossing.

---

## 2. Project north star, competing fronts and exact optimization problem

### 2.1 Production frontier (P-front)

Compete with Brotli across quality/window tiers, Zstd standard and high-compression tiers, xz/LZMA, fast LZ/Deflate anchors and ANVIL's own points on the complete vector:

1. complete encoded bytes, **including** dispatch headers, semantic program, framing, dictionaries, context tables, indices, recovery metadata and selection cost;
2. encoder wall time and throughput at declared hardware/thread count, inclusive of search, transforms, preprocessing, model building and initialization;
3. decoder wall time and throughput, including allocation, framing, CRC, entropy decoding, transformed reconstruction and any warmup;
4. peak working memory/RSS, bandwidth, cache locality and initialization state;
5. comparable decoder-only binary footprint, portability, startup, streaming, random-access and worst-case resource safety where relevant.

**Dominance** requires not being worse on all declared axes and being better on at least one, subject to evidence and experimental-frontier rules. ANVIL's project-specific **FRONT-CROSSING** classification additionally requires escaping every reference-front gap rectangle under the frozen rule; do not replace this with informal ratio comparisons. Missing RSS or noncomparable decoder-size builds cannot be counted as free wins.

### 2.2 Research/rate frontier (R-front)

High-ratio PAQ/CMIX/context-mixing or strong predictive and grammar-style oracles expose potentially explainable bytes but incur very different compute costs. A research lower rate does not establish a competitive product. Track any gap between P-front and modelling-heavy R-front as **structural diagnostic**, not directly comparable leaderboard placement.

### 2.3 The governing economics

The I10 design philosophy is that a decoder-visible cheap *explanation* removes the need to transmit a predictable byte. But the explanation itself costs bytes and cycles:

    Complete size ≈ L(explanation program)
                    + L(residual | explanation program, decoder state)

For an explanation e, a diagnostic yield is:

    Y(e) = (output_bytes_explained - descriptor_bytes - residual_bytes)
           / decode_work(e)

It is **not** a substitute for multi-axis Pareto analysis. A compression scheme with very good rate but BWT-class expensive inverse transformation can fail; a very fast new model with worse complete bytes can also fail. Accept a new primitive only after identifying both its causal rate prize and the decoder cycles/working-set budget that prize buys.

### 2.4 Strategic lesson of I9/I10

The core frontier problem is no longer to attach another generic entropy coder. The most promising mechanism-level frontier would explain more bytes through **cheap, exact deterministic reconstruction** without reintroducing expensive global search or serial decode. The old bit-level TCOPY ambition was valuable as a hypothesis but its original novelty separators did not survive. Future breakthroughs need a mechanism distinct from local BCJ, routine dictionary/grammar construction, ordinary numeric context coding, or generic model-state seeding.

---

## 3. Ratio evidence — the real win, correctly bounded

### 3.1 Frozen canonical full-corpus bytes (I9 legacy/default-off)

| Corpus | Original bytes | ANVIL ratio auto | xz -9e | Brotli q11/lgwin30 | ANVIL smallest among shown? |
|---|---:|---:|---:|---:|---|
| Silesia | 211,938,580 | **46,446,995** | 48,456,100 | 49,383,136 | Yes, **bytes only** |
| enwik8 | 100,000,000 | **23,534,368** | 24,831,656 | 24,810,180 | Yes, **bytes only** |

**MEASURED / HISTORICAL:** frozen docs/anvil-i9-findings.md and docs/FRONTIER-RESET-2026-09-23.md. These are legacy frozen reference results. The later I10 same-job comparison measured small differences for xz (Silesia 48,456,004; enwik8 24,831,648), which should be **retained as separate experimental reference versions/windows** rather than silently made equal. The multi-backend ANVIL auto representation draws its winning rate primarily from BWT-heavy paths and is not the same object as one cheap token-parser mode.

### 3.2 I9 headline decode deficit (historical)

- Silesia auto decode: **14.597× slower than Brotli q11**, **7.885× slower than xz**.
- enwik8 auto decode: **23.946× slower than Brotli**, **14.062× slower than xz**.

These report the frozen I9 state; later paired I10 aux measurements **supersede them for aux-ON mechanisms only**. Do not splice unlike builds/runner windows or mistake an I9 deficit estimate for a fresh I10 bound.

### 3.3 Complete historical Class-A thirteen-file grid

The aggregate below was independently reconciled in docs/I10-FRONTIER-RECON-2026-09-24.md from tests/benchmark-suite.frozen-bdc90474.plus-xz.csv. Throughput is **input bytes divided by summed per-file times**, not arithmetic averaging of rates. Windows 11 / Ryzen 9 5900X / clang-cl 22.1.8, limited repetition and elevated ambient activity; **ranking-grade, not citation-grade**. No peak-RSS column.

| Arm | Complete bytes | Ratio | Encode MB/s | Decode MB/s |
|---|---:|---:|---:|---:|
| Brotli q1 | 2,727,579 | 0.222254 | 432.106 | 471.702 |
| Brotli q4 | 2,492,866 | 0.203129 | 137.114 | 603.721 |
| Brotli q6 | 2,163,836 | 0.176318 | 69.262 | 624.797 |
| Brotli q9 | 2,123,887 | 0.173063 | 25.009 | 608.767 |
| Brotli q11 | 1,788,233 | 0.145713 | 0.681 | 528.346 |
| xz -9e | 1,720,520 | 0.140195 | 1.310 | 159.369 |
| ANVIL dp-rANS | 2,539,808 | 0.206954 | 1.321 | 357.571 |
| ANVIL sparse-rANS | 2,609,164 | 0.212605 | 10.567 | 222.692 |
| ANVIL TCOPY-rANS | 2,607,687 | 0.212485 | 10.374 | 226.017 |
| ANVIL MDL-rANS | 2,439,834 | 0.198808 | 0.874 | 278.323 |
| ANVIL shape-rANS | 2,508,878 | 0.204434 | 0.786 | 225.989 |
| ANVIL hot-op-rANS | 2,642,320 | 0.215307 | 10.462 | 330.374 |
| ANVIL hot-op-RLZP | 2,561,700 | 0.208738 | 0.157 | 264.753 |

**Interpretation:** this Class-A grid is **not** the full Silesia/enwik8 ratio portfolio, so do not conclude that ANVIL's auto ratio advantage implies these token/fast variants dominate q4/q6. Its reference configuration lattice is acknowledged **GRID-THIN** for omitted intermediary Zstd/window tiers.

### 3.4 Exact Class-A frontier classification

The final R2 retained grid accounts for **468/468** cells:
- **435 DOMINATED** — not competitive under the frozen reference/lattice rule;
- **28 DEGENERATE** — excluded by the pre-existing high-ratio/insufficient-compression guard, not near-wins;
- **5 historical FRONT-GAP** — unresolved non-domination inside timing dispersion/reference envelopes;
- **0 FRONT-CROSSING** — no defensible full frontier advance.

The five gap cells **must not** be turned into victories by Q2, retrospective tolerance adjustment, picking favorable runner noise, or dropping unfavorable memory axes. A future **separately preregistered PR-4-grade same-job paired timing experiment** may decide them if that experiment is independently warranted; none is currently authorized by R2.

---

## 4. I10 auxiliary-index inverse BWT — strongest integrated decode engineering

**Source:** docs/I10-AUX-UNBWT-RESULTS.md and docs/I10-FRONTIER-RECON-2026-09-24.md.  
**Nature:** adopt-class algorithm based on libsais BWT/inverse-BWT auxiliary indexes, existing transform/postcoder kept otherwise fixed. Default legacy rate mode remains **OFF**; alternative auxiliary representation remains supported.

### 4.1 Integrated same-job paired A/B wins

| Test | Aux incremental complete bytes | Integrated whole-codec decode speedup | Encode effect |
|---|---:|---:|---|
| Silesia dickens | **+2,494 B** | **1.3645×** | no reliable material change |
| Silesia webster | **+2,535 B** | **1.7946×** | no reliable material change |
| enwik8 | **+3,054 B** | **2.3392×** | no reliable material change |

All A/B configurations roundtripped; 9 paired measurements in the Webster study, robust dispersion/CI and 2% practical threshold applied. The reported 95% decode time-ratio CI for webster is [0.553921, 0.559169]; the enwik8 ratio is 0.427489 with [0.421034, 0.432620]. This is compelling **within-ANVIL** evidence; unlike unintegrated inverse-BWT microbenchmarks it includes postcoder/framing/allocation.

Correctness baseline includes a GitHub Actions smoke run with 480 roundtrip variants and 2,880 mutation cases, plus registered modes, golden inputs and aux-wire assertions. Paired Silesia/enwik8 **default-off bytes matched frozen I9 per file**, not just on aggregate.

### 4.2 Full portfolio actual outcome: still externally behind

| Same-job I10 class | ANVIL aux | xz -9e | Brotli q11/lgwin30 |
|---|---:|---:|---:|
| Silesia complete bytes | **46,466,339** | 48,456,004 | 49,383,136 |
| Silesia decode MB/s | **47.9** | 82.5 | 166.9 |
| Silesia peak decode RSS MiB | **248.4** | 54.3 | 124.5 |
| enwik8 complete bytes | **23,537,422** | 24,831,648 | 24,810,180 |
| enwik8 decode MB/s | **26.5** | 103.6 | 150.2 |
| enwik8 peak decode RSS MiB | **598.9** | 66.2 | 251.9 |

**MEASURED:** remote I10 experimental class, with paired timing authority in docs/I10-FRONTIER-RECON-2026-09-24.md. **Paired external decode time ratios:** Silesia ANVIL is **3.447×** Brotli time and **1.723×** xz time; enwik8 **5.664×** Brotli and **3.906×** xz. Peak RSS also materially worse. Full encode Pareto comparability is not yet supported by a corresponding paired full-portfolio encode experiment.

**Consequence:** byte surplus remains, decode and resident memory still gate crossing. The auxiliary mode is a valuable **new ANVIL internal Pareto representation**, not a demonstrated crossing of the external reference frontier or a novel fundamental compression scheme.

### 4.3 Why further inverse-BWT optimization is constrained

- In isolation an LF-walk accelerator can be dramatic, but full-path speedup is capped by the fraction of time **outside** the accelerated walk.
- Prior proposals to reach around **4.5×** whole BWT-path acceleration require LF walk to dominate roughly **78%** of the path even if its future implementation were infinitely fast (DERIVED Amdahl limit).
- No trusted measurement establishes the current per-stage shares for the needed newer experiment.
- R2 expressly **closes a duplicate Q4b scheduler/inverse-BWT implementation** that merely redoes integrated aux work. Stage attribution receives remote budget only when its possible results change a named decision.

---

## 5. Grotli representation-compiler program — exact results and failed hypotheses

The G0–G5 series asks whether exposing source structure prior to the **same exact Brotli q11/lgwin30 backend** can reduce complete serialized bytes. Every structure carrier must remain bit-exact and charge its metadata; raw Brotli remains an available fallback. The goal is *causal representation attribution*, not success by privately switching to a stronger backend.

### 5.1 G0: padded row-aligned XOR — KILLED

G0's padded vertical record-XOR representation was **NO-GO** on every trial:
- D1 Amazon: 40,126 B raw Brotli to **141,171 B** transformed (**+251.82% worse**).
- D2 CDISC: 25,029 B to **122,857 B** (**+390.86% worse**), ~14× padding expansion.
- Third input GH Archive: 1,292,757 B to **4,342,951 B** (**+235.94% worse**), ~35× padding expansion.

Why: one long outlier record sets stride; runs of synthetic zeros do not offset lost phrase locality, padding carrier, and entropy behavior. G0 rejection does **not** disprove all specialized layout/typed representations.

### 5.2 G1: exact lexical shapes and columns — narrow signal, broad NO-GO

G1 introduced shape reuse, exact value lexemes and group/column-major ordering. It improved the four-family discovery aggregate **24,296 B / 1.6564%**, below the frozen >=3% aggregate requirement, with only one discovery family >=5%. **NO-GO-G1-DISCOVERY** under preregistration. Still an informative class-specific shape-column signal.

### 5.3 G2: typed leaf basis — narrow strong controls, broad NO-GO

Five leaf choices: RAW_LEX, EXACT_DICT, INT_FOR, INT_DELTA_FOR, INT_DOD_FOR. Specific measured same-backend wins:
- D2 CDISC **−16.4849%** against raw Brotli;
- D4 CROVIA **−22.5029%**;
- routed 4-file aggregate **−2.0077%**, failing the frozen >=3% threshold.

Thus **NO-GO-G2-DISCOVERY**. Do not quietly add FSST, float coding or a new planner after observing the gate. Its D3 whole-file parser failure was important: one malformed incomplete record routed an otherwise structured 10 MiB object to raw.

### 5.4 G3: regionized strict-lexical + raw fallback — discovery PASS, held-out NEGATIVE

G3 preserves exact G2 syntax and leaf grammar but represents valid line-framed records structurally while keeping malformed or incomplete frames raw.

| G3 discovery corpus | Raw Brotli B | Best G3/retained B | Relative result |
|---|---:|---:|---:|
| D1 Amazon | 40,126 | 39,277 | −2.1158% |
| D2 CDISC | 25,029 | 20,861 | −16.6527% |
| D3 GH Archive | 1,292,757 | 1,240,155 | −4.0690% |
| D4 CROVIA | 108,857 | 84,361 | −22.5029% |
| **TOTAL** | **1,466,769** | **1,384,654** | **−5.5984%** |

D3 has **11,228 structured frames + 1 raw residual frame**, establishing that regionization fixed the G2 all-or-nothing availability defect and recovered **52,602 B** from D3.

**Frozen held-out V1** (Sino-US DrugQA, 15,374,047 raw source bytes): raw Brotli **2,112,235 B**, best G3 **2,179,615 B** = **+3.1900% worse**. There were **zero raw residual frames** in V1, so the failure cannot be attributed to a defective one-line regionizer. Ruling: **PASS-G3-NARROW**, not broad generalization; **no ANVIL production transform ID**.

**Evidence-role consequence:** V1 is now a **consumed known-stress reference** for later analyses, not a fresh held-out family for another unregistered rescue hypothesis.

### 5.5 G4: cheap leaf cost proxy for q11 oracle — NO-GO

G4 had strong implementation identity: **16/16** exact G3 carrier hash equalities. It tested proxy families against the q11 oracle with tight preregistered per-file and aggregate regret, weighted agreement and >=5× mandatory planner speedup. All failed. S2 gave measured **0.0591×** speedup (~17× slower than the oracle) and per-file regret **1.5388%** vs permitted 0.50%. Ruling **NO-GO-G4**. The relative success/failure of two metrics cannot be cherry-picked: it was a planner fidelity test, not a loose total-byte optimization.

### 5.6 G5A: isolate ordering without transforming tokens — positive causal discovery

Run **35985412906**; independent frozen corpus and carrier, all four arms use the same token multiset, byte count, parser, reconstructibility, complete envelope and Brotli build; they differ only in order.

| Treatment | D1–D4 complete bytes |
|---|---:|
| A0 deterministic random-permutation null | 2,054,532 |
| A1 fixed original baseline ordering | 1,470,205 |
| A2 shape grouping | 1,452,383 |
| A3 shape-column ordering | **1,380,245** |

- A1→A2 shape grouping: **17,822 B** saved.
- A2→A3 column ordering: **72,138 B** saved, **80.18897%** of A1→A3 total.
- A1→A3 overall: **89,960 B (~6.1189%)** saved.
- A3 improves over A1 on **4/4 discovery files**, but D2's gain is only **34 B**; shape grouping is adverse on D4 by 529 B.
- V1 is adverse on A3 vs A1 by **1,721 B**, while A1 itself is already **70,030 B worse** than raw complete Brotli on V1.

Ruling: **ORDER-MATERIAL / COLUMN-DOMINANT** for frozen discovery; **no evidence of broad held-out frontier**. Additional locality controls were preregistered to distinguish exact-column locality from arbitrary coherent permutations and shared-prefix effects; do not promote an unmeasured follow-on hypothesis.

### 5.7 G5B ordinal proxy and G5D paged dictionary

- **G5B-ORDINAL**, remote run **36011333908**: B0/B1/B2 = **2,054,910 / 1,380,245 / 1,403,029 B**. The tested cross-shape global-ordinal treatment **adds 22,784 B (+1.6507%)** over the exact G5A A3 floor on discovery. It is smaller on **1/4** discovery files; V1 favorable by 2,137 B is a consumed/known-stress observation, not a promotion rescue. Frozen ruling **ORDINAL-ADVERSE**. It closes **this proxy**, not every imaginable semantic hierarchy.
- **G5D paged dictionary**: adopt-class root/overlay/pages; close its existing discovery design because multiple varying factors are confounded and its warmup denominator addresses the wrong cost. The original overlay is inert for leaves whose occurrences do not reach the page policy. With **no measured G5D corpus result** there is no legitimate effect attribution to bootstrap an ablation. R2 **closes Q6 as constituted**; no remote budget until a substantially new, measured separator and correct controls.
- Neither line grants an ANVIL production transform ID, promotion or independent novelty.

---

## 6. ANVIL historical mechanism inventory and surviving value

### 6.1 Implemented families and what they taught

| Family | Mechanism | Status/lesson |
|---|---|---|
| Exact LZ + adaptive arithmetic | backward phrase references; adaptive literal probabilities | Correct foundational codec; overhead in hot decode and global search |
| Structured DP / MDL parser | estimate actual downstream token/stream cost | Better bytes on some structured files; expensive encode |
| Separated/interleaved rANS | decode-friendly physical entropy streams | Real internal speedup; routine known coding technology |
| Sparse-corrected reference / COPY_PATCH | copy prior phrase then apply sparse residual patch bytes | Valid general representation; search and syntax costs often overwhelm byte gain |
| RCM/SCM and synchronized residual candidates | recency/channel-bank approximate matches | Cheap discovery variants; many narrow/stale tradeoffs; not default frontier |
| SHAPE / grouped literal values | factor repeated templates and field locality | Demonstrates structured-domain anatomy; can lose against modern same-backend control |
| Macro-ops, hot-op instruction books | precompile frequent phrase semantics into smaller opcode stream | Decode/token cost experiments; total-cost and generalization gates binding |
| Compiled stream selector suite | rANS/Huffman/default-exceptions/pair coding as stream options | Useful backend engineering; no novel entropy-coder claim |
| Context-clustered literal models | decoder-visible prior bytes select clustered rANS table | Excellent literal subsystem, limited whole-codec by match/discovery and prior art in Brotli contexts |
| TCOPY(d,L,delta,mask,residual) | transformed self-copy with sparse arithmetic adjustment | Historic ELF signal; no sustained external frontier or novelty clearance |
| BWT + Brotli ratio backend | choose high-rate transform/backend | Explains canonical byte win **and** material decode/RSS deficit |
| libsais auxiliary inverse-BWT | sparse inverse-BWT auxiliary index | Retained measured internal Pareto point, still below external decode |
| Bit-exact DEFLATE reconstruction | recover compressed/embedded deflate semantics exactly | Prototype ~1.36 MB on mozilla; integration and dual-bar producer replay pending |
| PCLMUL accelerated CRC | ISA-native CRC work | Historically ~4.5× encode / ~3.4× decode store-path improvement, wire-identical; subpath only |

These are **not all competing production variants**; some are separate proof-of-value prototypes and some are enabled only for particular formats/flags. Always inspect the **actual selected path and registered wire mode**, not just the presence of its source code.

### 6.2 Canonical parse bypass — high-impact architectural fact

The canonical byte-winning **--parse=ratio** code path routes through datatype transforms plus **Brotli or BWT** and exits before the token MatchFinder/greedy/DP/MDL/SPARSE/TCOPY parser is invoked. This was independently verified by Track 17 and frozen as D22 in COORDINATOR-STATE.

**Implication:** performance work on old token search, parser DP, or token matcher **cannot move the canonical ratio frontier** unless the ratio wire/backend selection architecture itself changes. Fund parser work only as supporting infrastructure for a separate expressly selected fast codec product tier, or with an explicit verified bridge to the ratio path. Do not confuse token-path local win with ratio-portfolio external crossing.

### 6.3 Mechanism novelty audit — what no longer qualifies

The October prior-art kill team and R2 freeze leave **zero funded novelty claims**:

- H2 / transformed relocatable self-copy in its tested form is semantically near **parameterized, position-derived BCJ** and executable normalization/Courgette; merely moving BCJ into reference-local decoding is not a novelty separator.
- TCOPY/PNRA sparse masks and implicit transform parameters yield narrow byte effects; in the Windows corridor measured prizes are approximately a few bytes up to tens of bytes in some targeted tests, deeply dominated in the reference grid. Historical Linux savings in ELF text are **STALE cross-host representation evidence**, not promotion.
- FLI fused loop/grammar/micro-programs: the key counted-periodic loop can equal an ordinary periodic LZ reference; relevant grammar/compressed-domain execution literature anticipates the general ideas. Missing complete cost/robustness leverage.
- CAM adaptive model warmup priors, ordinary contextual coding and initialization are established, not new.
- CDR generic cost-driven block routing is occupied by existing Brotli/Zstd block engineering; the proposed formulation also had estimability/denominator confounds.
- Fixed-cap BWT subblocking is an adopt-class engineering approach, not a crossing mechanism.
- RSC contiguous relocated-run specialization had essentially zero empirical coverage; this does **not** falsify sparse transformed references in general but closes the RSC representation.
- Correction-topology template revivals, new entropy-coder-family substitution, plain dictionary paging, and numeric field compilation are not novel by differing in surface form.
- PRA-1 proposed safe search lower bound based on empirical **n·H0** for LZ/Brotli is **mathematically invalid** (a repeated/periodic string can compress below that zero-order entropy expression). Withdraw theorem; do not call the approximation a safe exact prune.

**Novelty gate for any future proposal:** (1) established semantic prior-art mapping, (2) precise new decoder semantics/explanation not reducible to existing transforms, (3) independent measured prize enough to move a complete frontier, (4) causal ablation including same-transform and external dual-bar controls, (5) real held-out independent evidence, (6) cost/robustness proof. Search gaps are not novelty clearance.

### 6.4 Do-not-reburn list

Avoid reimplementing known negatives just to retest the same hypothesis: untyped padded vXOR G0, failed G4 cheap cost proxy, G5B raw ordinal coarsening, generic context-model rebranding, unbounded global DP parser for the ratio route, reciprocal-multiply replacement of rANS division, global ELF field transposition destroying phrase locality, modal-mask augmentation with confounded comparisons, RePair/RLZ as a generic default, H0 unsafe pruning, PIM wrong-root-cause phrase memo, generic new entropy coder as a novelty program, or hardware scheduling that duplicates successful aux-BWT without a decisional gap.

---

## 7. Corpus and reproducibility — the first two gating problems

### 7.1 The reason a large corpus can still prove little

The project has many files and fixtures but **not** that many independent real source lineages. Synthetic controls from one generator do not establish independent generalization; archive/container copies may overlap source bytes, PE examples may share exact regions, and earlier “held-out” corpora cease to be held-out after being consumed. Without lineage/overlap/license checks and role segregation, a many-file aggregate can falsely look statistically powerful.

### 7.2 Q1a exact current-tree findings

The Q1a corpus-lock tool passed **135 / 135** bounded selftests in its recorded frozen local scope; its frozen tool SHA-256 is:

    c0c027e15f2a572d2816396129a1d0ed950bdd86dbf84d3c3d1e6310ccb399a4

Current 24-object audit:

| Audit field | Value / interpretation |
|---|---|
| Declared objects | **24** |
| Available at referenced commit b8eae11 | **13** |
| Local-only/untracked inputs | **11** |
| Conflict edges | **204** |
| c9 cases unresolved | **9** |
| Graph components | **4**, discovery structure only |
| Graph attestation | **attestation_backed**, not complete byte-computed proof |
| Audit verdict | **INVALID_INFRA** in dirty operator tree |
| Promotion-authoritative independent count | **false** |
| Newly admissible real held-out structured/numeric units | **zero** |

The last two items are decisive. A discovery graph's four components **cannot** be advertised as four validated independent promotion families. It is correct for the tool to fail closed in an intentionally dirty tree.

### 7.3 Two different gates, intentionally separated by R2

**Gate A — reproducible source/CI/producer identity:** does a clean, pinned GitHub Actions checkout produce the exact expected deterministic artifacts with exact implementation and known producer? This can be proven even if the semantic audit says **CORPUS_BLOCKED**. Once Gate A is established, independent **non-promotional** instrumentation and security work may proceed.

**Gate B — promotion/generalization admissibility:** does the evidence contain byte-verified independence/c9 containment, genuinely new locked structured/numeric/executable families, proper licensing/provenance, and a held-out role firewall? It remains blocked. A Gate A success **never** establishes Gate B.

R2 changes R1's mistaken serialization of these two validity gates without changing any measured scientific threshold. This is a major architecture/process improvement; a future coordinator must preserve it.

### 7.4 What c9 must establish

Every applicable cross-container content overlap/containment relationship needs direct bounded computation over already-OPEN bytes or a separately retained producer whose implementation, source/input blobs, exact output and independent reproducibility are verified. Existing attested edges with **verified=false** are not admissible. Synthetic controls, old historical PE and Sino-US DrugQA V1 are useful adversarial/reference controls but not unopened promotion material.

---

## 8. GitHub Actions — current trust problems and clean publication requirement

### 8.1 Historical substrate inventory (not a pass)

Local .github/workflows enumerates **16** existing workflow files. Static audit in docs/swarm-2026-10-02/GHA-SUBSTRATE-AUDIT.md found good baseline hygiene:
- manual-only workflow dispatch;
- contents read-only permission throughout;
- no secrets or organization write access;
- explicit timeouts, concurrency, cancel-in-progress false.

But reproducibility/evidence defects remain **material**:
- **40 floating @v4 action uses across 15/16 workflows**; only one fully commit-pinned action workflow at the time of audit;
- **17/20 upload sites** lack failure-path if: always();
- some input sources resolve mutable tags, and a corpus path relies on upstream mutable master / insufficient SHA-256 enforcement;
- artifact names sometimes identify GitHub run SHA rather than the frozen **actual implementation checkout**;
- several shell scripts lack strict fail-fast and cannot reliably distinguish a missing subcommand or partial artifact;
- no uniform authoritative gate/verdict artifact schema; MD5-only paths do not meet lock requirements.

This does not imply every old run was wrong. It means **future promotion-grade or critical mechanism decisions may not inherit source authority from the old workflow name alone**. Pin and assert independently.

### 8.2 Wave 0 / A1: Q1a-XRUN, FIRST allowed remote job after publication

**Prerequisites:** publish an approved **clean, narrow** Git commit with the exact frozen Q1a implementation and fixtures, relevant R2 authority and machine-readable lock/tombstone, and a **tracked** .github/workflows/anvil-q1a-xrun.yml whose content is byte-identical to the reviewed draft or fully re-preregistered if changed.

Draft path: prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/anvil-q1a-xrun.DRAFT.yml  
Fixture emitter: prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/q1a_xrun_fixture.py  
Tool: prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/q1a_corpus_lock.py

Frozen expected fixture information (R2):
- **7** fixture independence units / **30** conflicts / **113** not-evaluated audit rows;
- **13** manifest-listed artifact members, verified exact set;
- expected semantic verdict **CORPUS_BLOCKED**, audit_complete=true, attestation_backed, promotion authority=false;
- **zero** codec invocations, corpus fetches, archive decompressions, sealed-role bytes or network requests;
- fixture-lock SHA-256 **0c4090c84d9de954cbd1bbf3b9b239f4c31413f596c87ef6c55eae62575ce17f**;
- artifact manifest SHA-256 **feaa7848967d8245dc16f5d531fcc1e5c9f2898e941b6f5a424182301c4fec5c**;
- summary SHA-256 **112add224d0017e6bbb7fad083424d90c3818526416de546f3000e00216dcdaa**.

**Success** means a remote clean Linux runner matched the frozen exact source/artifact determinism, **not** that corpus independence was promoted. Source/blob/expected artifact mismatch => stop; no follow-on codec CI. This report does not authorize its dispatch, publication or source pin rewrite.

### 8.3 Publication mechanics

Use docs/swarm-2026-10-02/REMOTE-PUBLICATION-MANIFEST-R2.md as the file-by-file record of **prospective** blob identities. Verify exact staged Git blob IDs and working-byte SHA-256, source ref, tool producer and lock subject commit independently. The original local working tree is intentionally dirty: **never stage all, reset, stash, sweep, rebase or overwrite**. Use a separate clean publication construction only after explicit review/authorization. If any publication byte differs, re-derive the fixture/output SHA anchors; never patch expectations merely to satisfy CI.

### 8.4 Generic remote measurement contract

Future authoritative workflows:
1. workflow_dispatch only; contents: read; no secrets; finite timeout; pinned actions; no push/schedule trigger;
2. checkout exact immutable source SHA, assert HEAD, clean tree, production/tool/input Git blobs and SHA-256;
3. separately pin source-independent reference producers and their exact builds (e.g. Brotli v1.1.0 commit ed738e842d2fbdf2d6459e39267a633c4a9b2f5d);
4. verify corpus hashes, lineage, independent units and allowed experimental roles **before** consuming bytes;
5. specify identical geometry and window, hardware, threads/affinity, process warmup, samples and noise controls;
6. require exact roundtrip/malformed-case tests for each decoder-visible wire registry path;
7. collect complete compressed bytes first, then independent paired time measurements (if decisional), RSS, code cost and implementation versions;
8. fail closed, preserve failure evidence and emit machine-readable verdict including **INVALID_INFRA** when appropriate;
9. refrain from making novelty/frontier classifications from diagnostic-only experiments.

---

## 9. Q2: block/window parity and the hidden restart-cost confound

Current Class A candidate settings appear to use **256 KiB** independently decodable blocks while comparison reference windows are much larger. Prior ANVIL E4 measurements recorded **roughly 5–17%** block-model warmup/restart penalties. That is a substantial and **confounded** explanation for some rate gaps, not proof a larger default is always beneficial.

R2 Q2 Rev 8 is a *corrected, exact crossed factorial* for five original retained candidate identities:

| Cell | ANVIL block geometry | Incompressibility gate |
|---|---|---|
| G0 | 256 KiB | ON |
| G1 | 64 MiB | ON |
| G2 | 64 MiB | OFF |
| G3 | 256 KiB | OFF |

Exact signed byte contrasts are computed independently for each retained configuration:

    Geometry effect gate-ON  = G1 - G0
    Geometry effect gate-OFF = G2 - G3
    Gate effect small        = G0 - G3
    Gate effect large        = G1 - G2
    Interaction             = (G1 - G2) - (G0 - G3)
                            = (G1 - G0) - (G2 - G3)

**Mandatory G-S fidelity preflight:** reproduce the frozen tracked Class-A **G0 per-file bytes** exactly with the pinned source before running any G1/G2/G3. Any mismatch emits **BASELINE-VOID (build drift)**, withholds all contrasts, and terminates. Fresh build-output slots are separate, **never** “matched” to a prior tracked class by filename.

Reference Brotli R1/R3 are optional **descriptive matched-run** controls built from exact official Brotli v1.1.0 commit **ed738e842d2fbdf2d6459e39267a633c4a9b2f5d** (q11/lgwin30), never retroactively pasted over old default-window Brotli data.

**Q2 outputs:** exact signed bytes plus source/corpus provenance, with **no epsilon, materiality threshold, FRONT-CROSSING token, timing decision, peak RSS claim, or automatic default geometry recommendation**. It does **not** adjudicate the five Class-A historical FRONT-GAP cells. Its files and .github/workflows/anvil-q2-parity.yml are publication-held drafts, not remotely authoritative yet.

---

## 10. Security and format robustness — important independent engineering risk

**REVIEWED SOURCE** Track19 audit of source and FORMAT.md: decoder checks magic “ANV0”, revision 1/2, per-block length, CRC and exact input consumption; output allocation reserve is moderated, and malformed trailing bytes are rejected. These are concrete strengths.

However the file-level declared-output feasibility rule effectively permits:

    total <= (input_file_bytes / 7 + 2) * maximum_block_bytes

For a **64 KiB** input and revision-1 maximum **64 MiB** block, this proxy can allow a declared output/work scale **near 585 GiB** without a caller-supplied operational maximum. This is a **resource/work amplification policy weakness**, not proof arbitrary memory corruption or remote exploitation. The decoder has many per-block safety checks; the proxy nevertheless is too permissive to protect resource-constrained callers.

**Q8:** one remote, synthetic/byte-only, source-identified **resource-amplification** probe after Gate A; specify explicit application/caller output and work budget before considering a new VCA wire/decoder redesign. Reconcile full consumption, integer arithmetic safety, expansion limit, CPU budget and pre-allocation checks. Existing zigzag-wrap reachability is characterized as **low severity / fail-closed**, not reason to reorder the entire program. No Q8 run performed in this audit.

### 10.1 Required tests for all future wire extensions

- deterministic bitwise roundtrip including endian/range/overflow boundaries;
- reject unknown mode, invalid lengths, varint overflow, extra bytes, truncated payload and false CRC;
- strict reference distance/overlap, sparse mask capacity, reentry and dependency depth;
- output/work/RSS explicit ceilings and adversarial compressed-expansion cases;
- disallow compressed-domain loops or recursion that evade the work cap;
- no reliance on compiler-specific aliasing/unaligned C++ writes for safe wire interpretation.

---

## 11. Remaining measurement gates — not a blanket research queue

### 11.1 Q7 DEFLATE reconstruction — high byte-EV, CONDITIONAL

The bit-exact DEFLATE reconstruction prototype saved approximately **1.36 MB on mozilla** in the earlier controlled lane. This is unusually relevant because it can improve already-faster legacy routes rather than worsening the BWT decoder bottleneck. But a modern gate must reproduce both:
- frozen external **precomp → xz -9e** comparison, and
- same-backend **precomp → Brotli q11/lgwin30** transform-isolation comparison.

Also require producer identity/stream replayability (mere container containment is not enough), exact source bytes and decoder roundtrip, integrated output/framing/code costs, and frozen bad-case rules. R2 labels **HOLD / CONDITIONAL**, not approved benchmark. No novelty claim.

### 11.2 Q4a / Q4b — conditional BWT diagnosis

- **Q4a** typed/frontend × BWT byte cross-product: decide whether a typed structural input basis makes the costly BWT backend worthwhile after block/window parity is corrected. Byte-only first and four expressly named files; no novelty.
- **Q4b** stage decomposition: only if a concrete implement/not-implement decision depends on LF-walk/postcoder/index building/framing/CRC shares. Do not run generic performance profiling that merely restates known aux BWT wins or double-counts Q2.

### 11.3 Q5 and Q9 — tiny or unfounded byte ceilings

- **Q5 mask coding:** strict true mask-wire byte share/ideal ceiling **not retained**. Prior kill arithmetic using an incorrect reference denominator was retracted. **HOLD/DEFERRED**; piggyback instrumentation on an already-authorized byte job only; **no dedicated runner, no prototype authorization**.
- **Q9 W1024→W16 auxiliary index:** exact closed-form **22,024 B** maximum benefit on relevant Silesia+enwik8 BWT portfolio, and earlier discrepancy 19,328 payload vs 19,344 container resolved as +16 B framing. The source already wins bytes against the named refs, and reducing aux index risks its hard-earned decode gain. **CLOSED** as frontier work, no CI.

### 11.4 Q3 / Q6 / parser / novelty — closed as constituted

- Q3 A0 vs A1 constant-cost/size-proportional selector: complete emitted-byte difference already a retained **zero** and objective costs inhabit incomparable physical units; proposed timed difference lies in sensitivity floor. Cancel its remote experiment.
- Q6 G5D root/overlay attribution: wrong control and absent measured baseline; close the current plan.
- Old parser optimization: ratio path bypasses it.
- New isolated TCOPY, FLI, CAM, RSC, generic routing/correction-topology/entropy substitution with no new semantic separator: closed to frontier/novelty CI.
- Numeric/executable **promotion**: blocked until genuinely independent locked real families are acquired, verified and kept unopened for test.

---

## 12. Programmatic work order and dependency graph

The R2 operational DAG (not an instruction to dispatch automatically):

    [Existing R2 evidence: 435/28/5/0; no source identity claim]
                       |
             Publication actor review
                       |
         Clean, narrow approved Git commit
         + exact source/tool/fixture/workflow pins
                       |
           Q1a-XRUN on GitHub Actions
             (NO codec; expected CORPUS_BLOCKED)
                       |
         mismatch ------+------> STOP, rebind anchors
                       |
                Gate A = PASS
                       |
          +------------+---------------------------+
          |                                        |
    Engineering / diagnostic branch           Promotion branch
          |                                        |
        Q8 byte-only                          independent c9 audit
        Q2 strict G0 fidelity                   + newly held-out families
          then 2×2 contrasts                      + corpus-role firewall
        Q7 only with producer
          + both external/same-backend bars            |
        Q4 only if decision changes                 Gate B = PASS
        Q5 piggyback only                               |
          |                                    future claim-specific gate
    No frontier or novelty claims

A Gate A pass enables diagnostic engineering **despite** Gate B being blocked. An upstream FAIL cancels downstream jobs. New research must not silently bring back cancelled lanes through a minor rename or a new script.

### 12.1 Priorities based on information gained per trusted GitHub Actions minute

1. **First:** publish the exact R2 Q1a reproducibility bundle in an approved clean commit; dispatch **only XRUN** after separate authorization and identity checks.
2. **Second:** if XRUN passes, Q8 is cheap synthetic security infrastructure; Q2 is the most important fair-baseline scientific diagnostic (G0 equality before larger geometry). Gate B corpus acquisition proceeds independently.
3. **Third:** Q7 DEFLATE byte-controls only if producer replay is frozen; conditional Q4 only if the result changes a specific decoder/transform decision.
4. **Fourth:** return to mechanism design **after** the above closes its confounders; a fresh numeric/middle-out lane must not steal promotion evidence without a genuinely locked real corpus.
5. **Never:** spend remote compute to “complete” dead lanes or rescue a preregistered failed experiment post hoc.

---

## 13. The newly added middle-out reference and how it changes the research context

**New permanent repository seed:** [RESEARCH-SEED-MIDDLE-OUT-2026-10-07.md](RESEARCH-SEED-MIDDLE-OUT-2026-10-07.md)  
**Upstream:** https://github.com/schizofreny/middle-out

**REVIEWED SOURCE:** Middle-out splits a known-length 64-bit sample vector into **eight independent segments**, stores eight 64-bit initial predecessor states, XORs each stream's next value with the prior, emits an unchanged mask, per-changed-value three-bit byte offset and **one shared three-bit maximum significant residual length** per eight-lane frame, then payload and final raw tail. It supports scalar and AVX-512 paths and is designed around high throughput / parallel recurrence chains.

**REPORTED BY AUTHOR, NOT RE-MEASURED:** ~0.7–1.7 GB/s scalar encode, ~2.3–2.9 GB/s scalar decode, ~2.3–2.5 GB/s AVX-512 encode and ~3.4–4.8 GB/s decode on a single 2.0 GHz Skylake-X core, with example compression factor 1.3–3.3. Do **not** compare those raw numbers with ANVIL's differently measured q11/aux frontier or claim a novel “middle-out” mathematical method.

**Value to ANVIL:** a *cheap decoder-visible numeric explanation*, group-shared width model and SIMD-friendly multi-chain execution that can be assessed against Gorilla, Chimp, ALP, FOR/DoD and identical-backend controls. An outlier widens all eight changed lanes; segmentation seeds/footer have nonzero cost; scattered eight-chain memory access can hurt locality. Crucially, arbitrary JSON numeric lexemes cannot become typed floating-point values without bit-exact lexical recovery. The new seed includes boundary/fuzz/false-positive controls and a **future-only** proposed GHA comparison, **not** an approved experiment.

This seed is a prior-art/control **input** to the I10 explanation-oracle program. It does **not** reopen the blocked numeric promotion lane or R2 closed novelty routes.

---

## 14. Detailed blocker register — severity, closure evidence and next proof

| ID | Blocker / risk | Type | Current evidence | Required decision-changing proof |
|---|---|---|---|---|
| B-01 | Next remote results not yet reproducibly pinned | **CRITICAL process** | XRUN draft uninstalled; R2 files local/untracked; no authorized dispatch | Clean publication + exact XRUN hash reproduction, Gate A |
| B-02 | Dirty live worktree with extensive concurrent/legacy artifacts | **HIGH integrity** | 424 status entries, binaries/build debris plus real modified notes | Preserve it; publish from isolated, narrowly approved clean commit; never broad reset/stage |
| B-03 | No promotion-authoritative independent held-out corpus | **CRITICAL science** | Q1a INVALID_INFRA; c9 nine unresolved; 4 components discovery-only | Byte-verified containment + new independently locked real families, Gate B |
| B-04 | Class A compares unlike geometry/windows | **HIGH measurement** | 256KiB restart vs reference larger window; known 5–17% warmup | Q2 retained G0 byte identity + frozen 2×2 contrasts |
| B-05 | Five Class-A FRONT-GAP cells timing-sensitive | **HIGH claim** | 5 unresolved / 0 crossings | Separate preregistered PR4 same-job timing, if justified (not Q2) |
| B-06 | BWT ratio decode still far behind reference | **HIGH architecture** | Aux 47.9 vs 166.9 MB/s Silesia, 26.5 vs 150.2 enwik8 | New representation/decoder explanation with measured complete cost and real frontier surplus |
| B-07 | BWT peak memory adverse | **HIGH product** | Aux ~248MiB Silesia / ~599MiB enwik8 vs smaller external refs | Comparable whole-codec RSS/working-set controls and actual reduction |
| B-08 | Decoder-only binary size not comparable | **MEDIUM claim** | Mixed monolithic codec vs other reference build scopes | Frozen decoder-only build policy, matched comparable code-size measurements |
| B-09 | G3 structured gain does not generalize to V1 | **HIGH scientific negative** | V1 +3.1900% transformed vs raw | New independent held-out classes and genuinely new mechanism, no V1 recycling |
| B-10 | G5A grouping/order gain may be specialized | **MEDIUM causal** | D1–D4 −6.1189%, V1 adverse | True locality/random-null / leaf interactions on new approved evidence |
| B-11 | G4 planner proxy and G5D design closed | **CLOSED proposals** | G4 no-go; G5D wrong denominator/no measured baseline | A new fundamentally different premeasured decisional question |
| B-12 | DEFLATE producer/dual bar unresolved | **MEDIUM gating** | Historical ~1.36MB prototype, no replay-certified integrated result | Exact producer replay + precomp→xz & precomp→Brotli |
| B-13 | Declared-output work amplification | **HIGH security design** | Format proxy allows ~585GiB from 64KiB in rev1 | Caller work/output ceiling and bounded remote Q8 behavior |
| B-14 | Backend masking cost not known | **LOW/unknown** | Q5 wire/coding ceiling missing | Free counters in an already authorized byte job; no Q5-only runner |
| B-15 | Prior-art overlap of claimed novel mechanisms | **CRITICAL intellectual honesty** | Zero funded novelty claims surviving R2 | Demonstrate new nontrivial semantic separator, not renamed known compressor |
| B-16 | Wrong incumbent comparator or mismatched timing | **HIGH audit** | Multiple historical host windows and transform/rate controls | Same source, same backend transform, matched geometry, paired timing, properly frozen references |
| B-17 | middle-out numeric lane lacks independent promotion corpus | **HOLD** | 2017 external code read only; author benchmarks noncomparable | Gate A + new numeric held-out family + typed comparator matrix |
| B-18 | Existing workflow supply-chain/provenance hardening incomplete | **HIGH infrastructure** | floating actions, mutable tags, unverified artifact identity | R2 action SHA pin, corpus SHA256, failure artifacts, asserted manifests |

Do not report an unmeasured bound as a negative result: Q5 mask size, detailed BWT LF-walk fraction and new numeric performance remain **unknown**, whereas G4/G5B and historic vXOR failures are properly **measured negatives**.

---

## 15. Evidence pointers, reproduction identities and source navigation

### 15.1 Highest-authority local files (read first)

1. **docs/swarm-2026-10-02/FROZEN-CLOSEOUT-MATRIX-R2.md** — current execution ruling; Gate A/B split; 468 grid; Q1a/Q2; work queue.
2. **docs/swarm-2026-10-02/REMOTE-PUBLICATION-MANIFEST-R2.md** — exact expected publication sources/blob identities; not itself publication authorization.
3. **docs/swarm-2026-10-02/COORDINATOR-FREEZE-v2.md** — historical pre-R2 frozen findings and supporting detailed technical gates; overridden by R2 when contradictory.
4. **docs/swarm-2026-10-02/GHA-SUBSTRATE-AUDIT.md** — 16-workflow reproducibility/supply-chain audit.
5. **docs/swarm-2026-10-02/Q1A-C9-ADJUDICATION.md** and **Q2-ADJUDICATION.md** — adversarial consensus behind Q1a and Q2.
6. **docs/swarm-2026-10-02/FROZEN-CLOSEOUT-MATRIX.md** — R1 history only; review R2 delta before planning.
7. **docs/swarm-2026-10-02/COORDINATOR-STATE.md** — dated historical discoveries, not a worker dispatch queue.
8. **docs/anvil-i9-findings.md** — frozen canonical byte, throughput, correctness and frontier evidence.
9. **docs/I10-FRONTIER-RECON-2026-09-24.md** — Class-A full historical table and remote Class-B aux comparisons.
10. **docs/I10-AUX-UNBWT-RESULTS.md** — integrated aux ABI, correctness, paired measured decoding.
11. **docs/FRONTIER-RESET-2026-09-23.md** and **docs/I10-BREAKTHROUGH-PROGRAM.md** — explanation-yield hypothesis framework, historical I9 context. Older hypotheses are not dispatch authority.
12. **docs/swarm-2026-10-02/REMOTE-EXPERIMENT-QUEUE.md** — explicitly **SUPERSEDED FOR DISPATCH**; do not reactivate.
13. **FORMAT.md** and **src/anvil.cpp** — normative/implementation truth of decoding; source line numbers may drift after edit.

### 15.2 Grotli measured series and preregistrations

- docs/I10-GROTLI-G0-RESULTS.md — run 35935415091
- docs/I10-GROTLI-G1-RESULTS.md — run 35937406182
- docs/I10-GROTLI-G2-RESULTS.md — run 35943876855
- docs/I10-GROTLI-G3-RESULTS.md — run 35947013432
- docs/I10-GROTLI-G4-RESULTS.md — run 35952830106
- docs/I10-GROTLI-G5A-ORDERING-ATTRIBUTION-PREREG.md; docs/I10-G5A-LOCALITY-CONTROLS-PREREG.md — parent run 35985412906
- docs/I10-GROTLI-G5B-ORDINAL-RESULTS.md if present on relevant branch; public GitHub commit **a8218af59de2a2d26f73097a818eab33575df482**, parent run **36011333908**
- docs/I10-G5-PAGED-DICTIONARY-PREREG.md; docs/swarm-2026-10-02/01-g5d-dictionary-fledge.md — cancelled/superseded G5D question
- docs/swarm-2026-10-02/20-priorart-killteam-fledge.md — specific prior-art mapping and mathematical corrections; final R2 takes precedence over interim status

### 15.3 CI and reproducibility anchors

- Local HEAD **a5df2a5**; historical source baseline **b8eae11**; do not equate these refs.
- XRUN prospective tool SHA-256 **c0c027e15f2a572d2816396129a1d0ed950bdd86dbf84d3c3d1e6310ccb399a4**; proposed Git tool blob **0d2467c719daef37d301fa221fb8b4b3d069934b** at original freeze. Verify from actual published commit.
- XRUN fixture emitter SHA-256 **255ec7d56633bc4fa970024d446586cdd18afc3d8d7d5930257c85dc953dc3fb**; draft workflow SHA-256 **131cef5e4339d0bbe6509bbbe5bd163d09b43891661bf0fe9ea4f53a9658d74f**.
- Q2 Rev8 prereg/prototype identities in REMOTE-PUBLICATION-MANIFEST-R2.md; source modifications in local dirty working tree may have invalidated any previous frozen hashes, so **reverify before dispatch**.
- Commit-pinned actions example: actions/checkout **11d5960a326750d5838078e36cf38b85af677262**; actions/upload-artifact **ea165f8d65b6e75b540449e92b4886f43607fa02**. Resolve release/security policy at actual dispatch time without silently changing prereg semantics.
- official Brotli v1.1.0 frozen producer **ed738e842d2fbdf2d6459e39267a633c4a9b2f5d**.

### 15.4 External prior-art/reference stack

- Brotli RFC 7932 (literal contexts and block dictionaries) — source-defined ordinary context modelling must not be relabelled novel.
- BCJ/7-Zip/xz executable filters; Chromium Courgette executable symbolic normalization; VCDIFF/delta references.
- RePair/grammar, periodic LZ and low-level data-transformation literature.
- Gorilla, Chimp/Chimp128, ALP/ALP+adapt numeric compression families — controls for proposed SIMD/XOR leaves.
- [Middle-out source](https://github.com/schizofreny/middle-out), [upstream README](https://github.com/schizofreny/middle-out/blob/master/README.md), [scalar](https://github.com/schizofreny/middle-out/blob/master/scalar.cpp), [AVX-512](https://github.com/schizofreny/middle-out/blob/master/avx512.cpp). See linked seed; **no ANVIL experiment run**.

---

## 16. Exact continuation handoff — constraints that must survive the next session

1. Start from this report **and then refresh R2, the current Git status, open branch/HEAD, and publication manifest**; do not assume today's source/hash is tomorrow's.
2. Keep old 40-worker material archival. **No worker/session delegation was performed today**; no prior swarm should be awakened because its notes mention a blocked experiment.
3. Do not modify or clean the dirty worktree indiscriminately. Only the new report and seed are authored by this audit.
4. First prioritize **source trust**. A successful read of a local file is not GitHub source attestation. No science runner before Q1a-XRUN Gate A.
5. The next human/agent publication decision must explicitly authorize a **clean, narrow** commit/push. R2 and this report provide **no implicit permission**.
6. Treat exact source artifacts and byte identities as facts only at their original ref; any changed files cause re-freeze/recompute, not silent pin reuse.
7. The next science-grade run should probe a real decision (e.g. Q2 parity), not a fashionable unbounded mechanism family. Keep Gate B independent.
8. Never use synthetic files or consumed V1 as fresh held-out evidence; never call a byte-only win a full frontier crossing.
9. The first expensive/CPU-heavy measurement, once authorized, must run **exclusively on GitHub Actions** with same-job paired design and full evidence upload.
10. Prior-art and worst-case resource accounting are part of the architecture, not finishing polish.
11. middle-out remains a **context seed and comparative control**; its historical author throughput is not a measured ANVIL advantage and cannot bypass numeric corpus requirements.
12. Any new R3 freeze must specify precisely which R2 premise is invalidated, by which immutable new evidence, what status dependencies change and why no gate was moved post hoc.

### Final research judgment

ANVIL has made **real** compression progress: its canonical rate portfolio can beat named strong byte baselines, its auxiliary inverse-BWT path buys large integrated decode acceleration for very little bitstream cost, and its Grotli work identifies genuine field-locality effects under same-backend causal controls. At the same time its best ratio configuration still pays several-fold decoder time and major resident-memory overhead; specialized structured wins failed later broad validation; and post-swarm novelty review invalidated all currently funded novelty claims.

The optimal immediate investment is to make the **next experiment interpretable**, not merely fast to launch: publish exact source → attest Q1a on GitHub Actions → perform fair block/window/corpus controls → advance only mechanisms with a credible complete Pareto surplus. That is the frozen R2 frontier strategy, preserved here without dispatch or promotion.
