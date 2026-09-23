# I9 gate-lane close-out report (`research`/`gate` lane)

Author: `research`/`gate` lane, reconstituted ANVIL swarm. Date: 2026-09-20.
Scope: read-only audit of the I9 research record. This lane edited **no tracked file**;
the only artifact written is this file. Every figure below is a citation to a repo
artifact at a stated line, or arithmetic on such a citation.

**Integrity flag (read first).** `RESEARCH_LEDGER.md` is a **worktree-modified** file
(`git status` => ` M RESEARCH_LEDGER.md`). PART XIV is therefore an **uncommitted
working-tree artifact**, not a commit. This matters: the ledger's own PART XIV §3
(docs-resident `RESEARCH_LEDGER.md:4547`) rules that `463/468` "is **not recomputable
from any committed artifact**" and that only committed blobs are citation of record.
By the ledger's own standard, **PART XIV and its addenda are not yet citation of
record.** This is the sharpest integrity finding in this audit and neither the strategy
doc nor PART XIV states it about itself.

---

## (1) I9 CORRECTIONS AUDIT

Source: `docs/swarm-i9-strategy.md:166-219` (C-1..C-6), `:1060-1062`, `:1395`.

| id | what | self-correction of an earlier error? | recorded in ledger? | still open? |
|---|---|---|---|---|
| **C-1** | "needed %" convention erratum; corrects the **I8 erratum (AUDIT-4)**; freezes exact-byte Convention A; recomputes 11.63% (json) / 17.92% (jsonl) — I8's 11.3/17.2 came from mixing exact anvil bytes with rounded reference ratios (`docs/swarm-i9-strategy.md:166-179`) | **YES — second-order**: corrects a prior *correction* that "re-introduced the error it was correcting" (`:179`). The brief's original −11.6/−18.0 were right all along. | **NO.** No ledger occurrence of "needed %", "Convention A", 11.63 or 17.92 (grep). PART XIV §8 (`RESEARCH_LEDGER.md:4682`) logs C-4 as "MIXED" and the tuple as "PARTIAL REJECT"; **C-1 is not dispositioned by name.** | **OPEN on the ledger side.** The convention is frozen in the strategy doc and applies to §1.2; nothing in the ledger binds it. |
| **C-2** | I8 §2.7a corridor spot-check flipped: `87,013 B @187.0` is **DOMINATED by brotli-q11**; crosses at 188.0. Also one front count drifts 6 → 8 (`:180-186`) | **YES** — self-correction of this lane's I8 arithmetic. | **PARTIAL.** The pnra README records the flip (§6) and PART XIV §4 cites the bar `< 87,013 raw`; the **corridor flip itself has no PART XIV disposition** (AUDIT-2, `strateg:573`, remains "open -> PENDING"). | **PARTIAL / open** for the I8 front count; the corridor flip is settled in pnra-era text only. |
| **C-3** | record periods were **aliased**: `synth-timeseries.bin` = 20,000×14 B **P=14**; `synth-columnar-align.bin` = 12,000×23 B **P=23** (`:187-197`) | **NO** — a peer/coordinator erratum verified against the generator, not the author's own prior claim. | **YES — landed.** PART XIV §5 (`RESEARCH_LEDGER.md:4598-4608`) independently verifies P=14/P=23; §6.1 (`:4632-4642`) supersedes PART XIII §8, `gate-verdict-i8-ari-stride.md`, and `swarm-i8-strategy.md` §2.7c. **A5 (`:4839-4844`) confirms P=14/P=23 on the auto path.** | **CLOSED.** C-3's own "PENDING-LEDGER" tag (`:194`) is stale as of the A6/A5 entries. |
| **C-4** | decode-multiple provenance: **(a)** the Silesia `11.243x/13.015 MB/s` pair *does* recompute — author's "not recomputable" **withdrawn**; **(b)** the 20.33x Silesia replacement **rejected** pending PR-4; **(c)** the enwik8 correction verified — two of the author's own claims were wrong and are withdrawn in public (`:198`) | **YES — explicit self-correction.** Two claims withdrawn, one verified. | **YES — resolved.** PART XIV §6.2 (`RESEARCH_LEDGER.md:4643-4652`) records all three legs; A1 (`:4705-4729`) issues the definitive multiples (Silesia 14.597x, enwik8 23.946x). §8 logs C-4 as "MIXED". | **CLOSED as to classification; OPEN as to citation grade** — A1 (`:4711`) says threads are **not recorded** → PR-4-pending. |
| **C-5** | grid provenance: committed HEAD `fc23d9a` = 416 cells / 411 dominated; uncommitted worktree `c70179ea` = 468 / 463. Citation of record = HEAD (`:199-208`) | **NO** — a provenance upgrade from the coordinator, verified by research-gate. | **YES.** PART XIV §3 (`RESEARCH_LEDGER.md:4536-4557`) carries the correction of record and the blob ids (`4c986eb6…`). | **CLOSED** as a rule; **OPEN** in that bench's committed-grid refresh is still outstanding (`swarm-i9-strategy.md:1070-1073`). |
| **C-6** | best-ANVIL grid provenance: `140,898 B hotop-rlzp` exists **only** in the worktree grid; HEAD best is 143,132 B; decode bars/grid are grid-specific (`:209-219`) | **NO** — peer/coordinator erratum. | **PARTIAL.** A14/A18 (`RESEARCH_LEDGER.md:5043-5077`, `5177-5218`) carry the grid-dependence and the tuple table; the specific `140,898`-is-worktree-only fact is in the datastruct lane docs (§0, `:19`) and strategy, but no PART XIV line disposes C-6 by name. | **PARTIAL / open on naming.** |

**Assessment.** Of six labels, **two (C-1, C-2) are self-corrections of this lane's own
earlier work and one (C-4) withdraws two of the author's own prior claims** — i.e. **3 of
6 corrections are the strategy lane correcting itself**, two of them errors *inside
previous corrections*. The strategy doc admits this candidly (`:1059-1062`: "Two
iterations, two tally/provenance failures in this lane's own documents"). That is a real
methodology red flag about the strategy-lane recompute pipeline, but the doc's own
response (recompute-from-artifact rule, AUDIT-7..AUDIT-10) is the appropriate mitigation.
C-3/C-5/C-6 are peer corrections that were verified independently, which is the right
shape.

---

## (2) LEDGER CONSISTENCY FINDINGS

1. **C-1, C-2 (flip), C-6 are NOT recorded in the ledger as corrections.** "needed %",
   "Convention A", 11.63 and 17.92 have **zero** occurrences in `RESEARCH_LEDGER.md`
   (grep). PART XIV §8's disposition list (`:4681-4686`) names pnra, datastruct, the
   record-period erratum, the tuple, C-4, the format blocker, p0-arbiter, anti-tie/PR-4,
   the fuzz change and the citation rule — **it never names C-1, C-2, C-5 or C-6**.
   So the strategy doc's §7F.4 claim that "C-1..C-6" are corrections *accepted (public)*
   is true **of the strategy doc**, and **only partly true of the ledger**.
2. **C-3 and C-4 are fully recorded** (`:4598-4642`, `:4643-4652`, `:4705-4729`) — and
   landed *better* than the strategy doc knew: A5/A6 (`:4806-4870`) independently
   confirm P=14/P=23, which the strategy doc was still listing as PENDING-LEDGER at
   `:194`.
3. **PART XIV itself is uncommitted.** Its own §3 standard (`:4547`) says uncommitted
   artifacts are not citation of record. The ledger's own correction-of-record is
   therefore not yet of record. **Recommendation: commit PART XIV + addenda before I10
   opens**, or the record is formally self-invalidating.
4. **Dropped mechanism missing a ledger entry (CONTEXT.md:532).** Two I9 drops fail the
   "record WHY (math vs implementation-era)" rule:
   - **`transmitted-Δ` TCOPY submode (math-class rejection)** — decisive in
     CONTEXT.md:287-294 and docs/research-agenda.md (I2-5 lineage), but **no Experiment
     entry in the ledger**; only the I8-era Experiment O/K pre-registration
     (`RESEARCH_LEDGER.md:660-1017`) exists. The narrowing to implicit-Δ has **no
     MATH-class ledger entry**.
   - **`bwtinv` `r=n` + `_omp_t` decode lever (implementation-era falsification)** —
     recorded in `prototypes/i9-bwtinv/RESULTS.md:57-59`, **not** in PART XIV; A4
     (`:4781-4804`) covers only the aux wire charge.
   Also worth noting: **PNRA's Linux-lane status is unmirrored** — CONTEXT.md:318-380
   documents the event-driven PNRA inversion, but `docs/research-agenda.md:740-780`
   still says "pre-registered … not yet measured on Windows", and the I9 Windows pnra
   lane (`prototypes/i9-pnra/README.md`) measures a *different* mechanism (per-region
   stride reference), which PR-5 ruled **no novelty** (`:41-46`). The novelty claim
   survives on Linux evidence only, and nothing in the ledger reconciles the two.

---

## (3) PENDING ITEMS (explicit) AND CLOSERS

Item numbers follow `docs/swarm-i9-strategy.md` §7F.5 (`:1397-1404`) and §7 PENDING
register (`:1064-1182`).

| # | pending item | what would close it |
|---|---|---|
| 15 | strategy recompute of `tests/benchmark-suite.frozen-bdc90474.csv`; frozen tuple `33\|5\|0\|28\|435/468` cited as arbiter-owner-issued (`:1119-1122`) | an independent recompute of the frozen CSV; note the store-path anomaly is now **CLOSED** by A21 (`RESEARCH_LEDGER.md:5260-5288`) with window `w-arch-storepath-20260912T1300Z` + 3 arm shas, so 15's "unattributed" clause is stale |
| PENDING-LEDGER | corrected record periods P=14 / P=23 (`:188-197`) | **CLOSED** by PART XIV §5/§6.1/A5/A6; the strategy doc's `:194` tag is stale |
| PR-4-pending | decode multiples (Silesia 14.60x/7.89x; enwik8 23.95x/14.06x) lack thread counts (`:159`, `RESEARCH_LEDGER.md:4711`) | first fully-attested PR-4 window under measurement contract v1.2 (`swarm-i9-strategy.md:1084-1089`) |
| — | committed-grid refresh on the frozen build (`:1070-1073`, `:1404`) | bench refresh of the committed suite CSV; heads the citation rule |
| — | ratio-first CSVs untracked (`:1096`, `RESEARCH_LEDGER.md:4650`) | commit the four CSVs so the byte figures stop being "not citation of record" |
| — | P4.1 DEFLATE integration: 1,362,177 B **encode-only / roundtrip gated** (`:1128-1131`) | arch integration + fuzz; wire roundtrip is already sha256-verified on the green binary (`prototypes/i9-deflate/RESULTS.md:194-227`) |
| — | native integration + arbiter for pnra-stride / datastruct (`:1105-1127`) | default-off flag + byte-identity gate → two hash-identical arbiter runs → PR-4 window → gate ruling |
| — | `bwtinv` aux-unbwt integration + quiet rerun (3.40-3.55x ranking-grade) | quiet-window rerun + new wire id registration (`prototypes/i9-bwtinv/RESULTS.md:16-21`) |
| — | **strategy-lane recompute-integrity debt (C-1/C-2/C-6)** | decision: strategy or gate must enter these in the ledger as corrections of record, else the "public" record is incomplete |
| — | **PART XIV uncommitted** | commit, per §3's own standard |

---

## (4) NOVELTY GATE AUDIT — top candidate

**Top candidate: Transformation-Invariant Temporal Anchoring (PNRA)** — the strongest
mechanism-level candidate in the record. Source: CONTEXT.md:318-380;
`docs/research-agenda.md:740-780` (pre-registration, I3-4). The I9 *Windows* prototypes
(`prototypes/i9-pnra/`) measure a **different, non-novel** mechanism, so the gate below
is applied to the **Linux-evidence** formulation, and I say plainly where that leaves it.

Four-point gate:

**1. Prior-art lineage — PASS, but narrowed by the gate.** What exists: delta/copy-patch
codes (VCDIFF/bsdiff/Zdelta/Zucchini), recency/distance reuse, approximate matching,
invariant/sketch set reconciliation (2014) — all named as lineage, not claims
(`research-agenda.md:875-881`). What Brotli exploits: a compact literal context map
driven by previous decoded bytes (RFC 7932 §7) — and CONTEXT.md:204-211 explicitly
records that **"context clustering is NOT ANVIL novelty."** The gate's own nearest-prior-art
ruling for the *sibling* mechanism is recorded: PNRA's arm (B) is nearest
"adaptive DPCM / predictor selection / delta-of-delta / xz `--delta`"
(`RESEARCH_LEDGER.md:4566-4571`). Verdict: **PASS for the search-formulation claim**
(invariant indexing is a search formulation, not a re-name), with the caveat that the
prior-art survey was **desk-inference, not a patent/database search** — a real evidentiary
hole (`research-agenda.md:603-604`).

**2. Precise statement of what is NEW — PASS (as a claim), with a fidelity caveat.**
New = derive `I(x,p)` such that `I(T(x,θ), p') = I(x,p)` and index the temporal
dictionary over `I` rather than raw bytes, so discovery becomes
"invariant → exact hash lookup → cheap verification" instead of
"candidate generation → expensive approximate verification → discover transform"
(CONTEXT.md:345-356). For TCOPY's relocation field, `I(v,p) = v + p` (two-anchor
`(K1,K2,Δf)`). Ordinary LZ = identity-transform special case. This is genuinely a **new
search formulation**, not a renamed primitive. **Caveat:** the identity-reduction
ablation (d) is **pre-registered but never run on Windows** (`research-agenda.md:769-778`).

**3. Reason the interaction should move the Pareto frontier — WEAK / UNPROVEN.** The
mechanism's stated value is *encoder throughput on the ELF lane*
(candidate generation ~474 MB/s; event-driven form: 115,490 structural events vs 1.4M
parser positions — CONTEXT.md:336-344). But the measured end-to-end result **does not
beat brotli**: event-driven PNRA 1,794,886 B vs q4 1,781,130 B (**gives back ~13.8 KB**,
CONTEXT.md:340-343), and the two-anchor pair-index 1,755,244 B is still above q4. The
"density leg crossed" result (1,761,776 vs 1,781,130 = −1.1%, CONTEXT.md:311-317) is
**on a different arm** and is a *prototype wire*, and the density win is only **−1.1%**
against q4 — deep inside the repo's own noise and grid-thinness caveats. There is **no
landed ANVIL row** and no frontier measurement. Verdict: **UNPROVEN.**

**4. Ablation showing the gain comes from the mechanism — FAIL / NOT RUN.** The
pre-registered ablation set is (a) PNRA-on vs PNRA-off for the same transform family,
(b) invariant-based vs byte-based indexing, (c) LZ = identity-invariant reproduction,
(d) Windows ablation (`research-agenda.md:769-778`). **None has been run on Windows.**
The I9 Windows lane measured a per-region **stride** reference, which PR-5 ruled
**"engineering / adopt, no novelty"** — decisively because the zero-bit derived form was
**15 B LARGER** than the transmitted form (`RESEARCH_LEDGER.md:4583-4587`,
`prototypes/i9-pnra/README.md:260-265`), so the measured byte result does not require the
derived parameter at all. There is therefore **no ablation in the record that isolates
the invariant-indexing mechanism.**

**TOP-CANDIDATE VERDICT: the four-point gate is NOT passed. Points 1-2 pass as a claim;
point 3 is UNPROVEN; point 4 is NOT RUN.** PNRA is a **live, well-formed, mechanism-level
hypothesis** — the best in the project — but as of I9 it is **not a demonstrated novelty
result**, and the gate must not let it be cited as one. The strongest *gate-cleared*
mechanism remains R2/correction-topology (`research-agenda.md:220-234`), which is
**BLOCKED and NOT ADOPTED** (Experiments I and N: loses to flat-A by 14-21%,
`RESEARCH_LEDGER.md:1631`).

---

## (5) I9 NET RESEARCH VERDICT

**The strategy doc's headline claim is SUPPORTED: I9 produced no new frontier-moving
ANVIL-row measurement, and there is zero FRONT-CROSSING on every grid and reference
class.** Verified independently against PART XIV: HEAD `5|5|0|0|411/416` (`:4538`),
worktree `5|5|0|0|463/468` (`:4524`), frozen grid `33|5|0|28|435/468` (`:5188`), recon
`3|3|0|0` (`:5025`), and the ledger's own verdict line `RESEARCH_LEDGER.md:4690-4695`
("0 FRONT-CROSSING … 411/416"). The five surviving rows are unchanged `generated.json`
encode-plane FRONT-GAP rows (`:4514-4518`), i.e. the same five that existed before I9.

What I9 actually produced is **five prototype landings, none of which is an ANVIL row**:

1. **datastruct** — 89,877 B on synth-timeseries; **NOVELTY: NO** (PART XIV §5,
   `:4598-4628`); FRONT-GAP dual-bar (loses to `xz -9e --delta=14` = 89,564 and
   `brotli q11+delta14` = 80,650). Net: an adopt-class capability + a large, honest
   **negative** finding that global lane transposition destroys phrase structure.
2. **pnra-stride** — 12,936 B on synth-arith; **no novelty** (PR-5, `:4559-4596`);
   the mechanism reduces to transmitted per-region delta/stride coding.
3. **deflate P4.1** — 1,362,177 B recovery on mozilla, **adopt-class prior art, no
   novelty** (precomp/preflate); encode-only at the time, now wire-roundtrip verified
   on the green binary.
4. **bwtinv** — aux-unbwt 3.40-3.55x at 1 thread (ranking-grade, load-degraded window),
   +19,328 B Silesia wire charge; the `r=n`+threads lever **falsified**.
5. **decode-perf PR-1** — profiles + the **CLOSED** decode axis: CRC spent, MAT
   falsified/retired, ALLOC insufficient, leg-4 falsified as a route but retained as a
   ~1.18x zero-byte win (`:5098-5113`).

**Net research output of I9: negative and infrastructural.** The iteration's genuine
contribution is a **clean, well-classified map of what does NOT work** — lane transpose,
transmitted-Δ TCOPY, materialization liveness, r=n parallel unbwt, static-table
postcoders — plus five adopt-class capabilities that are all FRONT-GAP or dual-bar
rejected. That is respectable falsification work, but it is **not** a novelty delivery
and the record should not be read as one. Two of six corrections being self-corrections
of earlier self-corrections, with three strategy/gate corrections (C-1, C-2, C-6) still
unnamed in the ledger and PART XIV itself uncommitted, means the **process** output is
also weaker than "corrections accepted (public)" implies.

---

## (6) I10 RECOMMENDATION (ranked)

Ranked by (novelty-gate headroom) × (evidence already landed) × (cost to a falsifiable
result). Mapped to the validated CONTEXT.md:460-481 targets.

**1. R2 slot-default / correction-topology coding — OPEN I10 WITH THIS.** This is the
only mechanism in the project whose **novelty claim is not yet spent** and whose
*evidence* is landed: 128 recurring patch masks, 598 (mask,slot) contexts, modal residual
accuracy 86.5%, exception fraction projected ~0.30-0.45 (CONTEXT.md:451-453, :472-475;
`research-agenda.md:220-234`, Claim 3 `RESEARCH_LEDGER.md:378-386`). It is *uniquely*
well-posed: no new format, no new search, a decoder-visible default + exception mask, near-zero
decode cost. It has been twice blocked on *alignment*, not on its own merits
(`:619`, `:918-920`, `:1631`) — so the I10 task is precisely the missing prerequisite,
**not** a re-run. **Gate:** pts 1-2 pass (the interaction — per-(mask,slot) modal
residual as a decoder-visible default + entropy-coded exceptions inside a
self-referential patch phrase — is the non-obvious interaction, and the 2026-09 patent
gate closed on TCOPY's relocation algebra, not on topology); pt 3 is the 0.30-0.45
exception fraction → rate win at ±0 decode cost; **pt 4 ablation is trivially
constructible** (modal-default vs flat-A on the same tokens), which is exactly what the
gate needs and what PNRA lacks. This also targets CONTEXT.md #3 directly.

**2. Shape-conditioned displacement prediction P(d|s) — highest-certainty rate
mechanism.** CONTEXT.md:462-466 target #1; measured Linux 112,941 → 109,700 B, decode
534 → 541 MB/s. Narrow novelty only (FLAG-D, `research-agenda.md:246-262`), and it is
the *baseline* in the shape representation. Open it as the **control/rate-bearing
mechanism that R2's ablation rides on**, not as the headline. Gate pt 4 pass.

**3. Precision/work-adaptive entropy (256/512-state rANS + stream suite).** CONTEXT.md #2.
Explicitly **enabling infrastructure, not novel** (`research-agenda.md:472-495`) — like
the K≈8-12 quantizer. Rank it third because it multiplies every stream's J-selection and
is cheap, and because it is the honest home for the "context clustering is not ANVIL
novelty" finding (RFC 7932). **Do not let it be claimed as novelty.**

**4. PNRA / transformation-invariant anchoring — keep alive, but gate it properly.** Do
**not** open I10 on it as a novelty play; open it as the **next iteration's
pre-registered falsification** with the ablation set that was never run (LZ-identity
reproduction + invariant-vs-byte indexing on Windows, per `research-agenda.md:769-778`).
Only if point 4 passes does point 3 become arguable. Pair it with **TCOPY invariant
anchoring** (CONTEXT.md:245-265) since the two share the Δ=−d algebra, and with the
**event-driven asymmetric admission rule** (search work ∝ ~115k structural events, not
input length) — that inversion is the one genuinely new *search formulation* in the
record and is worth one honest measurement.

**5. R2-slot-default residuals via the datastruct/pnra discovery machinery — the
bridge.** Both I9 adopt-class capabilities solve *period/region discovery*; R2 needs
*mask/slot topology alignment*. Unify them under one parser (the gate's own surviving
position #2, "unification of equivalence relations under one MDL parser",
`RESEARCH_LEDGER.md:4594-4596`). This is the non-obvious interaction with the best
shot at pt 3, and it reuses landed code.

**Not recommended for I10 opening:** any further wire-invisible decode work (axis
CLOSED, `:5110-5113`); P4.1 integration as a *novelty* play (prior art, adopt only);
anything on the frozen grid until the store-path protocol control is signed (now A21-closed,
so this is unblocked but not novel).

---

## (7) RECOMMENDED CLOSE-OUT ACTIONS FOR THIS LANE

1. **Enter C-1, C-2 (corridor flip + I8 front count), C-5 and C-6 into the ledger** as
   recorded corrections-of-record, with the strategy doc's own recompute labels. §2
   above shows three of six corrections exist only in the strategy doc; the ledger is
   the record and currently understates them.
2. **Commit PART XIV + its addenda.** By §3's own standard (`:4547`) an uncommitted
   ledger section is not citation of record, so the I9 gate rulings are formally
   provisional until committed. Also commit the four untracked ratio-first CSVs.
3. **File two missing DNB/drop entries:** transmitted-Δ TCOPY (MATH-class) and the
   `r=n`+threads unbwt falsification (implementation-era). CONTEXT.md:532 requires a
   WHY per dropped mechanism; both are documented in lane docs only.
4. **Reconcile PNRA's two lives** (Linux novel search formulation vs Windows
   no-novelty stride reference) in one ledger entry, with the explicit statement that the
   Windows lane does not test the novelty claim.
5. **Correct the frozen-tuple / store-path staleness:** items 15 and PENDING-0 in the
   strategy doc are closed by A18-A21; the strategy doc's `:1119-1122` "unattributed"
   clause and `:66` "PENDING-0" clause predate the closure.
6. **Publish the C-3 status escalation:** strategy still lists PENDING-LEDGER (`:194`)
   for record periods that A5/A6 independently verified; the tag should be struck.
7. **Open I10 with an R2 pre-registration that names its ablation before any code runs**
   — the missing-ablation failure mode (PNRA point 4) is the project's recurring
   weakness and the gate should refuse to accept another candidate without the ablatable
   claim named up front (PART XIV §8 already requires "prior-art lineage + an ablatable
   claim"; enforce it at pre-registration time, not at ruling time).

---

*End of report. This lane edited no tracked file. The ledger, gate docs, strategy doc
and all prototypes were read-only inputs.*
