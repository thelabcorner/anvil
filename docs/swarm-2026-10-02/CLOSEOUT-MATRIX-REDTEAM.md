# CLOSEOUT-MATRIX-REDTEAM — adversarial dependency DAG and minimal remote queue

> **RED-TEAM INPUT, NOT CURRENT DISPATCH AUTHORITY.**
> `COORDINATOR-FREEZE-v2.md` supersedes this file's proposed QBYTES percentage gates,
> Q5/Q9 dispositions, and execution ordering. Retain this file for the attacks and dependency
> analysis that led to the final freeze.

**Date:** 2026-10-02
**Type:** red-team synthesis. Additive. **This document authorizes nothing.**
**Supersession:** retained as adversarial evidence; current execution authority is `COORDINATOR-FREEZE-v2.md`.
**Scope:** all surviving Q0/Q0b/Q0c/Q1/Q2/Q3/Q4a/Q4b/Q5/Q6/Q7/Q8/Q9 plus blocked lanes B1–B4.
**Inputs read this session:** `COORDINATOR-STATE.md`, `COORDINATOR-CLOSEOUT-v1.md`,
`MASTER-BRIEF.md`, `Q0-DENSE-RETAINED-RESULT.md`, `REMOTE-EXPERIMENT-QUEUE.md` (all 616 lines
incl. three appended red-team sections and one dated correction),
`REMOTE-EXPERIMENT-QUEUE-FLEDGE-REVIEW.md`, `Q0C-LAMBDA-C-SPACE-BUNNY.md`,
`Q0C-LAMBDA-C-CRITIC.md`, `06-entropy-codesign-space-bunny-q3-preflight.md`,
`Q2-PARITY-PLAN-CRITIC.md`, `03-heldout-corpus-space-bunny.md`, `03-heldout-corpus-fledge.md`.

## 0. What I executed, and what I did not

**No codec ran.** No `anvil`, `anvil_bench`, `bench_suite.py`, fuzz, or benchmark.
**No network. No commit, push, reset, clean, stash, restore, rebase. No production edit. No git write.**
The only commands run were read-only filesystem enumeration, `Get-FileHash` over existing corpus
bytes, and read-only `git log`/`git ls-files`. The only file written is this one.

Evidence labels used below:

| label | meaning |
|---|---|
| `[V]` | **Verified by me this session**, from primary bytes on disk, reproducible |
| `[SRC]` | read from a named project artifact with a line anchor |
| `[DERIVED]` | arithmetic over `[V]`/`[SRC]` inputs, every input shown |
| `[PROJECTED]` | prior-agent estimate, unverified here, not usable in a gate |

---

## 1. Verification log — what I independently established

These are the load-bearing facts this matrix rests on. All are `[V]`.

### V1 — The corpus universe is **24 files / 21,127,883 B**, and one lane's census is wrong

`tests/corpus/CHECKSUMS.txt` carries **24** data rows totalling exactly **21,127,883 B**.
`tests/corpus/README.md:3` says "24 data files, ~21.1 MB total input (21,127,883 B)".
All 24 manifest members exist on disk; **0 size mismatches**; sampled SHA-256 matches the
manifest for `anvil_bench.exe`, `pe-where.exe`, `synth-columnar-align.bin`,
`synth-telemetry-f64.bin`. (26 files on disk = 24 data + `README.md` + `CHECKSUMS.txt`.)

> **The constructive corpus lane undercounted the corpus it was auditing by one file and
> ~1.33 MB.** `03-heldout-corpus-space-bunny.md` E5 states "header + **23** data rows" and E7
> "**23** data files, ~19.8 MB total", both tagged `[M]`.
> `03-heldout-corpus-fledge.md` §3.1 states 24 / 21,127,883 B, also tagged `[M]`. **The critic is
> correct and the constructive count is wrong.**

This is not pedantry. `independence_units` — the quantity that governs GATE-INDEP-2, the §13
portfolio gate, and every downstream promotion claim — is computed over a universe that one of the
two lanes authoring it enumerated incorrectly.

### V2 — `QP-CONSUMED` is byte-live, not a no-op

All **6/6** consumption-tombstone hashes in `REMOTE-EXPERIMENT-QUEUE.md` §Q-E5 match the on-disk
`pe-*.exe` bytes exactly. The tombstone is a real byte-identity gate and it will reject correctly.
**Do not weaken it, and do not replace it with a bookkeeping assertion.**

### V3 — **9 corpus members are untracked**, including 3 of the numeric/synthetic families

`pe-git.exe`, `pe-ninja.exe`, `pe-notepad.exe`, `pe-python.exe`, `pe-where.exe`, `pe-winver.exe`,
`synth-columnar-align.bin`, `synth-drift-stride.bin`, `synth-telemetry-f64.bin`.

Consequence for **B1 (numeric reference dual-bar)**: the numeric/telemetry material that exists is
(1) `make_synth_corpus.py` output, which the protocol forbids as held-out, and (2) untracked, so a
fresh clone has none of it. **B1 has no admissible input object in this repository at all.**

### V4 — Q2's G1 build-drift gate is **real, and the Q2 critic understated it**

`git log 9caeee9..HEAD -- src/anvil.cpp` returns **four** commits, not three:

```
c048e55  experiment: add auxiliary-index inverse BWT framing      (src/anvil.cpp, tests/fuzz.py)
e69a9ee  hardening: tighten aux BWT wire and CLI validation      (src/anvil.cpp, tests/fuzz.py)
dc5c47f  hardening: specify and bound BWT framing                (FORMAT.md, src/anvil.cpp, tests/fuzz.py)
770eeb4  fix: use portable CPUID path for clang on Linux         (src/anvil.cpp, 4 lines)
```

`Q2-PARITY-PLAN-CRITIC.md` A3 lists three and asserts *"All three are BWT/aux-scoped"* — which
justifies presuming Class-A bytes are unchanged. **That presumption is wrong.** `770eeb4` is a
CPUID-path change on the same `src/anvil.cpp` that `tools/bench_native.cpp:2` `#include`s directly.
CPUID sits on the capability-detection path that timing and cycle accounting depend on. **G1 is not
a formality; it is the single most likely pre-dispatch tripwire in the program.**

### V5 — Q2's G7 artifact set is fully present; the fetcher defect is live

`tests/benchmark-suite.frozen-bdc90474.plus-xz.csv`, `parity_sweep.py`, `tests/noise-floor.csv`,
`tests/block-oracle.csv`, `tests/thread-attestation.csv`,
`tests/pr-4-measurement-window-protocol.md`, `tools/bench_native.cpp` all exist.
`tools/gha_fetch_corpus.py` exists — so `03-heldout-corpus-fledge.md` §15 (MD5-only verification
against the protocol's own "SHA-256 is the content authority" rule) is a live defect, not a stale
citation.

### V6 — Working state matches the brief

`HEAD` = `b8eae11` "ci: freeze G5A r5 ordering attribution workflow", matching
`MASTER-BRIEF.md:4`. `docs/swarm-2026-10-02/CLOSEOUT-MATRIX-REDTEAM.md` did not previously exist.

---

## 2. The dependency DAG

### 2.1 Node classes

| class | meaning |
|---|---|
| `DONE` | premise settled by a retained, recorded result |
| `KILL` | premise already invalidated; running it is a queue-invariant violation |
| `REVISE` | live question, invalid instrument as specified |
| `HOLD` | live, but blocked on a named external dependency |
| `FREE` | zero-dependency infrastructure; nothing upstream can cancel it |
| `PAPER` | zero-CI work misfiled into a "remote" queue |
| `CLOSED` | permanently unfunded |

### 2.2 Nodes

| node | class | decisive status | anchor |
|---|---|---|---|
| **Q0** | `DONE` | 435 DOM / 28 DEGEN / 5 GAP / **0 CROSSING**, 468/468 accounted; 0/18 configs cross. Q2 required | `Q0-DENSE-RETAINED-RESULT.md:29-35, 84-87` |
| **Q0b** | `PAPER` | H0 theorem withdrawn (D18). Only the written correction survives; zero CI | `COORDINATOR-STATE.md:131-134` |
| **Q0c** | `DONE`/`PAPER` | Both lanes issued. λ·c published; Q3 ruled CANCEL/REDEFINE. Residual = one coordinator stake decision | `Q0C-…-SPACE-BUNNY.md:577-591`; `Q0C-…-CRITIC.md:589-632` |
| **Q1a** | `REVISE` | Deterministic local audit. **Blocked on 7 mandatory additions**, one of which is a red-team rejection | `03-…-fledge.md:484-493` |
| **Q1b** | `HOLD` | CI halves (blind auditor, fault injection, cross-runner). Blocked on Q1a | queue E6 |
| **Q2** | `REVISE` | 3 independent blockers (B1/B2/B3) + 1 structural (S1). Question live, instrument invalid | `Q2-…-CRITIC.md:24-48` |
| **Q3** | `KILL` | Premise settled at a recorded machine-verified **zero**. Arms do not share a unit | `Q0C-…-SPACE-BUNNY.md:577-586` |
| **Q4a** | `HOLD` | Live as a byte question. Must be 4 files, Class-A-adjacent, never co-instrumented with Q2 | perf review item 7 |
| **Q4b** | `REVISE` | Duplicate-work kill stands; Amdahl kill **withdrawn**. Job reduces to a ~1-job ratio record | perf review C3 |
| **Q5** | `REVISE` | Diagnostic-only, `authorizes_prototype:false` unconditional. Needs a pre-gate it does not have | queue NQ-3 |
| **Q6** | `KILL` | Two independent kills not yet recorded: no baseline to decompose, invalidated denominator | §4.3 below |
| **Q7** | `HOLD` | **Highest measured byte EV in the queue.** Byte-only, no decode work. Dual control mandatory | `COORDINATOR-STATE.md:107` |
| **Q8** | `FREE` | Security infra. Zero dependencies. Cannot be cancelled upstream | `COORDINATOR-STATE.md:142-144` |
| **Q9** | `HOLD` | Byte/correctness only. Needs an aux-share pre-gate | queue §Q9 |
| **B1** | `CLOSED`-pending | No admissible input object exists (V3). Not "blocked" — unsatisfiable here | §4.6 |
| **B2** | `PAPER` | Track 02 vs Track 16 planner reconciliation. Zero CI. 19× prize asymmetry unresolved | `COORDINATOR-STATE.md:45-51` |
| **B3** | `KILL` | Premise (H0 safe-prune) withdrawn | `COORDINATOR-CLOSEOUT-v1.md:58-59` |
| **B4** | `CLOSED` | Reopen condition rewritten twice, **mutually incompatible**; neither is reachable | §4.5 |

### 2.3 Edges (hard dependencies, `A → B` = A's result can change B's fate)

```
Q0 ──► Q2                        (Q2 exists only because Q0 could not adjudicate parity)
Q0c ──► Q3                       (NQ-1 / QP-NOVELTY-2: no λ hardcode without calibration)
Q1a ──► Q1b
Q1a ──► B1                       (needs a locked real numeric family; V3: none exists)
Q1a ──► [all promotion claims]
Q2F(G1) ──► Q2                   (byte-equality vs frozen CSV; V4: 4 drift commits)
QBYTES ──► Q2, Q5, Q9            (census + mask-share + aux-share pre-gates; §5.1)
Q2 ──► Q4a                      (Q4a bytes are only interpretable against a fair-geometry baseline)
Q7  ──► (dense-grid reference statement)  (NQ-6: needs the frozen predicate, NOT Q2)
Q0c-residual ──► [coordinator stake on λ] ──► (any future cost-model work)
C5-rule ──► Q1b                   (fledge §13C: C5 is inert on the entire real corpus)
```

**Explicitly NOT edges (collapsing these would create confounds):**

- `Q4b ──► Q3`. Different objects (post-aux BWT decode vs stream-selection argmin). Merging them
  reproduces exactly the B1 pattern: multiple coupled variables in one arm.
- `Q9 ──► Q2`. `kBwtAuxTargetWalks = 1024` *is* Q9's W1024 (`Q2-…-CRITIC.md` §3.3), and aux
  overhead ∝ block *count*. These push the same byte lever in the same direction. They are
  separable **only because Q2 is Class-A-only and aux is gated on `parse=="ratio"`**. If Q2 is ever
  re-scoped to include BWT arms, this becomes an unavoidable confound. Treat Class-A-only as a
  **binding constraint on Q2**, not a preference.
- `Q2 ──► Q7`. Q7's dense-grid reference statement is against the *frozen* grid, which exists. Q7
  is not downstream of Q2.

---

## 3. Register of live contradictions — five rulings required before any dispatch

These are coordinator decisions, not implementer's choices. Each one currently blocks or
mis-scopes a job.

### X1 — `COORDINATOR-CLOSEOUT-v1.md` §4.3 states a kill that its own source retracted

Closeout §4.3: *"Post-aux measured/arithmetic stage shares show the LF walk is not large enough to
meet the required whole-codec acceleration even with an idealized walk."*

That is verbatim the claim the perf reviewer **retracted** in its dated correction C3: the
`6.05×/3.86×/6.60×` figures are **postcoder** requirements, not walk requirements, and the
`f_walk = 50–61 %` split is **pre-aux**. The post-aux walk share `w` is **unmeasured**.
`COORDINATOR-STATE.md:149` (D21) is the correct position: *measure, then kill.*

> **X1 is a live over-closure.** Left uncorrected it cancels Q4b for the wrong reason and destroys
> the only number (`w`) that can legitimately close the lane. **Required ruling:** restate §4.3 as
> *conditional on `w < 0.420`*, not as a finding.

### X2 — Closeout §7 contains three requirements that cannot all be satisfied

Closeout §7 requires (i) the frozen DOMINATED/DEGENERATE/FRONT-GAP/FRONT-CROSSING predicate,
(ii) timing can never create a crossing, and (iii) RSS and decoder parallelism reported "because
larger blocks change concurrency". `Q2-…-CRITIC.md` B2 proves (i) and (ii) are mutually
unsatisfiable in `parity_sweep.py` as implemented, and §3.6(c) proves (iii) is **structurally
unavailable** — `nthreads = min(decode_threads, nblocks)` is forced to 1 at parity.

> **Required rulings:** split the predicate into `eps_ratio` and `eps_speed`; declare whether the
> speed axis carries the crossing token at all; and rule that the parallelism column is
> **descriptive-only** rather than a deliverable.

### X3 — The Q0c lanes do not agree on scope, and the disagreement is load-bearing

Both agree the **arm** is dead (139/139 exact-`L` ties, complete emitted-byte delta = 0).
They disagree on whether the **question** is dead.

- Constructive §7.3/§7.4: CANCEL/REDEFINE; Q3 leaves the queue; retain a coverage census.
- Critic §8.3: the `1.226×` Amdahl ceiling is *"the ceiling on the 139 realised rANS-precision
  flips only … not a ceiling on the objective"*; on the macro streams carrying 52 % of decode, a
  ratio-60–75× raw↔rANS flip projects `≈2.05×` whole-codec decode, *"invisible to Q3 as posed."*
  Verdict: **CANCEL-the-arm, not CANCEL-the-question** (`:618-632`).

These are not reconcilable by averaging, and the critic says so against its own interest.

> **Red-team position.** The critic is right on scope. But the residue is **not a CI question**:
> the missing input is *what one wire byte is worth against decode time in ANVIL's deployment* —
> an application stake this project has never stated, and one that `Q0C-…-SPACE-BUNNY.md` C4
> (`:501-521`) correctly identifies as a coordinator re-specification requiring its own
> pre-registration. **Therefore Q3 leaves the remote queue entirely**, and the `≈2.05×` projection
> is recorded in the ledger as documented-unexploited upside pending a stake declaration.
> Spending CI before that declaration is waste.

### X4 — Q2's own existence depends on a corpus fact nobody has established

Q2 exists because 4/13 corpus files are single-block at 256 KiB, so the retained grid cannot speak
to parity (`Q0-DENSE-RETAINED-RESULT.md:46-58`). The Q2 critic's block-count census
(`Q2-…-CRITIC.md:154-183`) confirms: median file = 2 blocks, only 3 files exceed 8 blocks,
**4/13 are geometry-invariant**, and `random.bin` is *exactly* 262,144 B — one byte above the block
size it becomes 2 blocks and flips its `DEGENERATE` classification for arithmetic rather than
informational reasons.

> **Required ruling:** predeclare the informative subset (`nblocks ≥ 4` → 7 files) and the control
> subset (`nblocks ≤ 2` → 6 files) *before* the run, and state in advance what result count is
> too small to interpret. **[V]** this census is free and belongs in Q1a (§5.1), not in a remote arm.

### X5 — Five dispatch orders are in circulation and none is authoritative

| source | order |
|---|---|
| queue §Dispatch order | Q0 → Q1 → Q2 → Q3 → Q4a → Q4b → Q5/Q6/Q7 → Q8/Q9 → B1–B4 |
| queue NQ-1 edit | Q0 → **Q0c** → Q1a → Q1b → Q2 → Q3 |
| queue E6 (Fledge corpus) | Q0 → Q1a → Q1b → Q2 |
| perf review §5 | Q0, Q1, **Q3-Stage-1 before Q2**, Q2, Q4a, **Q7 above Q4b**, Q5, Q6, **Q4b demoted**, Q8/Q9, B1–B3, B4 |
| closeout §9 | reconcile Q0c → reconcile Q1a → reconcile Q2 → freeze queue → authorize first job |

Closeout §6 also omits Q0b, the Q1a/Q1b split, and the double B4 rewrite — **it is stale relative
to its own queue's appendices.** §9 below supersedes all five.

---

## 4. Premise-kill register

### 4.1 Q3 — premise already settled at a recorded zero

Complete emitted-byte delta = **0**, machine-verified before Q3 was written
(`RESEARCH_LEDGER.md:3998-4001`; `Q0C-…-SPACE-BUNNY.md:547-551`). All 139 flips are exact-`L`
ties among three rANS precisions that the frozen table prices **identically at 6.0 ns/B** — i.e.
declared exactly equal by the objective and resolved by insertion order. The arms do not share a
unit: A0's `c` is dimensionless, A1's is ns/B. Stage 2 is barred pre-emptively — the measured
zero-byte-change control band is +2.3–8.1 % (`Q0C-…-CRITIC.md:569-573`).

**Verdict: `KILL`.** Two derived jobs survive and are reclassified in §5.

### 4.2 Q4b's implementation half — duplicate of a landed, adopted, measured result

`libsais_unbwt_aux` already **is** a bi-gram LF-mapping walk acceleration with auxiliary indexes: it
delivered 1.364× / 1.795× / 2.339× whole-codec decode for +22,398 B (I10-1A, adopted). A second
walk-scheduler effort is a **re-burn of a closed result**, which `06-do-not-reburn.md` E5 and the
master-brief doctrine forbid.

This is a **duplication** argument, not an Amdahl argument, and the reviewer says so explicitly —
it survives the C3 retraction intact.

**Verdict:** no implementation prototype is authorized under any outcome. Q4b survives **only** as a
byte-only + ratio record whose sole deliverable is publishing `w` on the post-aux candidate.

### 4.3 Q6 — two independent kills, neither currently recorded

1. **No baseline to decompose.** Q6's stated purpose is to *"separately isolate root capacity,
   overlay capacity, paging, ordering, and escape costs"* — while the queue itself records that
   **zero G5D corpus bytes have ever been measured**. An ablation requires arms with something to
   attribute against. There is nothing.
2. **Its denominator is a mechanism absent from its arms.** Q6 is asked to report "warmup tax
   avoided" against the measured E4 figure. `Q2-…-CRITIC.md` §3.4 (`:289-303`) establishes that
   E4's tax is **model-warmup on MTF/context — a BWT-specific state reachable only via
   `parse=="ratio"`/`--ratio-backend=bwt`**. Q6 is a Class-A token-lane dictionary census. The
   denominator is a category error, and E4's corpus (Silesia, `mozilla` = 196 blocks) is 18× larger
   than Q6's largest file.

**Verdict: `KILL` as specified.** Refund condition: a first byte-only G5D measurement must exist
before a census can attribute anything. This is not in the queue and should be.

### 4.4 B3 — premise withdrawn

`COORDINATOR-CLOSEOUT-v1.md:58-59` and `COORDINATOR-STATE.md:131-134` (D18): the PRA-1 H0 bound is
false in general (periodic payloads have positive zero-order marginal entropy yet LZ/context coding
emits far fewer than `n·H0` bits). **Verdict: `KILL`.** Q0b survives only as the written
correction — which costs zero CI.

### 4.5 B4 (CAM) — two incompatible rewrites, neither reachable

| rewrite | author | reopen condition |
|---|---|---|
| NQ-7 | novelty red team | "Q2 demonstrates residual block warm-up under fair geometry AND a free larger-block null is exhausted." |
| perf §4 | perf red team | *"That condition cannot clear B4, because the binding defect is not geometry — it is the state dimension."* |

The perf reviewer's grounds are arithmetic and stand unaided: 64–256 B = 512–2,048 bits cannot hold
an order-2 context model over 2¹⁶ bins (≈32 KB at 4 bits/bin); the too-few-states failure is
**already measured** at **+3.46 %** (unconditional order-1 rejection); CAM is a **rate** mechanism
on the axis ANVIL already wins, while the binding deficits are decode and RSS; and in a static-table
rANS format the transmitted model *is* the prior, so the mechanism is largely vacuous.

**Verdict: close permanently.** Two independently-authored reviewers arrived at "geometry is not
the blocker" from opposite directions. Continuing to carry a reopen condition that neither reviewer
believes is reachable is queue noise. **No third revisit trigger.**

### 4.6 B1 (numeric reference dual-bar) — not blocked; unsatisfiable *here*

**[V]** (§1 V3): the only numeric/telemetry bytes in the tree are `make_synth_corpus.py` output —
which the protocol forbids as held-out — and three of them are untracked. There is **no admissible
input object**. B1 is parked on "a genuinely new locked real numeric family", which is a
networked acquisition decision that Track 03's constructive lane records as already failed once
(HTTP-401 cross-host redirect, `:390-395`).

**Verdict:** reclassify from `HOLD` to **formally abandoned pending a separate acquisition
authorization**, rather than sitting indefinitely in a blocked list. An indefinitely-blocked lane
is indistinguishable from a forgotten one.

---

## 5. Duplicate register and collapse register

### 5.1 COLLAPSE-1 — one byte-only accounting pass replaces three pre-gates *(highest leverage)*

Three separate requests for the same arithmetic over the same container bytes:

| request | requested by |
|---|---|
| achieved block count / mean block size per canonical file | `SYNTH-MEASUREMENT-FLEDGE` §7 item 4; perf review §5 item 2; `Q2-…-CRITIC.md` §5 output 1 |
| mask-share pre-gate (< 2 % of container ⇒ cancel Q5) | perf review §5 item 7 |
| aux-index byte share pre-gate for Q9 | perf review §5 item 10 |

Add the mask ideal-replacement ceiling and the per-block overhead/aux byte columns Q2 must emit.

> **Proposed single job `QBYTES`** — byte-only, no timing, one SHA-256, over the frozen Class-A
> ladder + the frozen I10-1A BWT candidate. Per (config, file): block count; per-block framing
> overhead bytes; mask-stream bytes + `mask_uniqueness_p50` + `mask_modal_accuracy`; the mask ideal
> ceiling with `ceiling_basis_role`; aux-index bytes (BWT arms only); `evidence_role` per row.
> `"authorizes_prototype": false` unconditional.

**This job can cancel Q5 and Q9 before either is dispatched, and can downgrade Q2 to a
configuration note.** It is the highest invalidation-per-runner job in the program. It must be
Wave 1.

**Its own thresholds must be coordinator-frozen before it runs** — I am proposing `2 %` mask share
and `0.1 %` aux share, and per queue doctrine item 5 I have no standing to fix them. If the
coordinator declines to freeze them, `QBYTES` degenerates into Q5-with-extra-rows and should not run.

### 5.2 COLLAPSE-2 — Q1a absorbs five free sub-tasks

Q1a is a deterministic local audit (~2 s, zero network, zero sealed bytes). Everything below rides
on the same pass over the same bytes:

1. **Corpus universe reconciliation** — resolve 23 vs 24 ([V] §1 V1). Must happen *first*; every
   other count depends on it.
2. **Independence units + `conflict_edges[]` + union-find components** — GATE-INDEP-1/2.
3. **Corpus-geometry census** — block counts per canonical file (feeds X4 and Q2's subset split).
4. **Toolchain attestation table** with an evidence hash — `03-…-fledge.md` §13A.6 (all six PEs
   report optional-header linker 12.1, which does **not** encode toolchain).
5. **`gha_fetch_corpus.py` MD5→SHA-256 audit** — live defect, confirmed present **[V]** §1 V5.
6. **Criterion c9 cross-container containment** — over already-opened bytes only; the
   12,000/12,000 `generated.sqlite` ⊂ `generated.json` containment is *invisible* to the 4 KiB
   content rule (verified: zero shared aligned blocks), which is a measured demonstration that
   rule 3 cannot see cross-container lineage.

> **Five or six uncertainties, one deterministic job, no runner.** This is COLLAPSE-2's whole
> value: `independence_units` is the number that governs every promotion claim, and it is the one
> number neither existing lane has produced.

### 5.3 COLLAPSE-3 — the "candidate table" log absorbs Q3's census **and** the critic's M4

Q3's retained deliverable (`Q0C-…-SPACE-BUNNY.md:592-615`) is a per-stream candidate table
`(block, stream, role, N, L_c ∀candidates, c_const, ns_c, J_const, J_prop)`. The Q0c critic's M4
(`Q0C-…-CRITIC.md:606`) is *"log, for every candidate: `L_c`, `n`, and the argmin under each policy
… the single highest-value logging change."* **These are the same artifact.**

It is emitted by a log statement beside the existing `g_stream_log_entries` mechanism — **it is not
a measurement at all.** Cost: one build, zero CI, zero corpus benchmarking.

> **Reclassified as `PAPER`/local-build, not a remote job.** This is the artifact Q4b needs and the
> substrate any future cost-model work requires. It also makes the objective-level disagreement rate
> *computable*, which the retained manifest cannot support (it records only the two chosen arms).

### 5.4 COLLAPSE-4 — Q7 is already minimal; confirm and do not expand

Q7 must carry, in one job: `precomp → xz -9e` (frozen external control, preserved per D10) **and**
`precomp → Brotli q11/lgwin30` (same-backend transform isolation) **and** the best-dense-grid
reference statement (NQ-6) **and** the `.text`/1 MiB decompressor-cap arm (perf §6 item 4) **and**
producer-identity replayability (D10's binding gate). It is one byte-only job and the perf reviewer's
promotion of it above Q4b **stands** — it requires no decode work, so no decoder result can
invalidate it.

### 5.5 Duplicates that are *not* duplicates — do not merge

| apparent pair | why it is not one job |
|---|---|
| Q4b ↔ Q3 residual | different objects (post-aux BWT decode vs stream-selection argmin). Merging = the B1 confound |
| Q2 ↔ Q4a | Q2 must be Class-A-only; Q4a introduces BWT. Co-instrumenting them reintroduces B1 (backend + geometry + revision moving together) |
| Q7 ↔ Q2 | Q7's dense-grid statement is against the **frozen** predicate, which exists. Q7 is not downstream of Q2 |
| Q9 ↔ Q2 | separable **only** under Class-A-only. See §2.3 |

### 5.6 B2 is not a remote job

B2 (Track 02 vs Track 16 planner routing) is a reconciliation of two written plans. D5: CDR
critic-killed on estimability/gate/denominator/carrier granularity. D6: Track 02's allowed fidelity
error is large relative to its measured planner prize, while the already-measured G5A ordering prize
is **~19× larger**. **Zero CI is required to settle either.** Reconcile off-queue; then fund at most
**one** pilot, chosen on information gain per expensive backend call.

---

## 6. Ordered minimal remote queue

**Precondition on everything below:** the Wave 0 rulings. Nothing in Waves 1–3 is dispatchable
until X1, X2, X5 and the Q1a blocking additions are resolved.

### WAVE 0 — zero-CI reconciliation (not remote jobs; unblocks or cancels everything below)

| # | item | cancels/creates |
|---|---|---|
| 0.1 | Restate closeout §4.3 as conditional on `w < 0.420` (X1) | revives Q4b for the right reason |
| 0.2 | Rule on the predicate: split `eps_ratio`/`eps_speed`; declare whether speed carries the crossing token; rule parallelism descriptive-only (X2) | makes Q2 implementable |
| 0.3 | Reconcile the Q0c lanes on scope; rule that Q3 leaves the remote queue; record the `≈2.05×` unexploited upside; state the λ stake as an open coordinator item or declare it out of scope (X3) | kills Q3 cleanly |
| 0.4 | Close B4 permanently; close B3; reclassify B1 as formally abandoned (§4.4–4.6) | shrinks the queue |
| 0.5 | Freeze `QBYTES` thresholds (mask share, aux share) or decline | decides whether COLLAPSE-1 exists |
| 0.6 | Issue the single authoritative dispatch order (supersedes the five in X5) | §9 |
| 0.7 | Q0b write-up; B2 reconciliation (both zero-CI) | §8 |
| 0.8 | Resolve corpus universe 23 vs 24 ([V] §1 V1) | precondition to Q1a |

### WAVE 1 — determinism and byte-only accounting *(highest invalidation per runner)*

**1. Q1a′ — corpus universe + independence audit.** Deterministic, local, ~2 s, zero network,
zero sealed bytes, zero codec. Enters the queue only after 0.8.

- **C1.1 CANCEL CONDITION — `independence_units ≤ 6`.** The fledge review predicted ≤ 5 and
  `[V]`-confirmed lineage facts (two generator lineages, 12,000/12,000 sqlite containment, one
  measured 4 KiB PE collision, 3 of 6 PEs from one OS build) all point the same way. If confirmed,
  **formally declare the promotion track dead in writing** and stop all promotion-shaped spend,
  rather than continuing to run discovery jobs under a promotion-shaped narrative.
- **C1.2 CANCEL CONDITION — Q1b is pointless** if a second, independently authored implementation
  does not return identical `conflict_edges[]` and components on the same lock.
- **C1.3 STOP CONDITION** — any archive member hash is computed for an unopened role ⇒ `PB-06`,
  burn nothing, escalate (queue E7).
- **C1.4 GATE** — GATE-INEP-1/2, c9, `host-installed` source kind, `git_tracked` recorded,
  thresholds moved from code defaults into the lock, L6 re-specified to compare **computed
  components**, not reason lists. Without these, LOCK-V1 can pass all nine gates while the
  anti-overfit claim is false.

**2. QBYTES — the shared byte-accounting pass** (§5.1). Byte-only, one binary, SHA-256 pinned,
`evidence_role` per row, `"citation-grade"` forbidden, `"authorizes_prototype": false`.

- **C2.1** mask share `< 2 %` of container ⇒ **Q5 is CANCELLED without dispatch.**
- **C2.2** mask uniqueness ≤ 90 % (already recorded) ⇒ Q5 may **never** be dispatched at all; the
  ceiling is bounded by uniqueness and closeout §4.5 forbids prototype authorization regardless.
- **C2.3** aux-index share `< 0.1 %` ⇒ **Q9 is CANCELLED without dispatch.**
- **C2.4** ≥ 8 of 13 corpus files single-block at 256 KiB ⇒ **Q2 downgrades to a configuration
  note**, not an experiment.
- **C2.5** `evidence_role` shows any input row in `{discovery, synthetic_control,
  self_reference_control, known_stress, external_anchor}` ⇒ `QP-NO-PROMOTE` fires; `"promotion_
  authorized": false` and no PASS variant. **Expect this to fire on every row.**

**3. Q2F — the G1 binary byte-equality gate.** Fold into `QBYTES` as its fidelity arm. Re-run the
retained Class-A configuration at `--block=262144` with all other `bench_native.cpp` defaults.

- **C3 STOP CONDITION — any file's bytes differ from `benchmark-suite.frozen-bdc90474.plus-xz.csv`.**
  The delta is **build drift, not a parity result**; the retained CSV is not a control; Q2's
  baseline must be re-established and Q2 defers. **[V]** there are **four** drift commits, and one
  (`770eeb4`) is a CPUID change, so this gate is likely to trip.

### WAVE 2 — the largest measured byte prize, zero decode work

**4. Q7 — DEFLATE reconstruction controls.** Byte-only, adopt-class engineering. Dual control
mandatory (D10), plus dense-grid reference (NQ-6) and `.text`/1 MiB arm.

- **C4.1** `precomp → Brotli q11/lgwin30` fails to beat `precomp → xz -9e` ⇒ **KILL.** xz is the
  frozen external bar; losing to it kills the mechanism regardless of same-backend success.
- **C4.2** A denser dense-grid reference (higher Brotli tier, `zstd-ultra-22-long27`) closes the
  gap ⇒ label **`BYTE-WIN`, not a frontier candidate**. `q11/lw30` alone is not an admissible sole
  control.
- **C4.3** `precomp` output is not byte-replayable under producer identity ⇒ **STOP.** Container
  containment is *not* the binding coverage gate; replayable stream bytes under producer identity
  are.
- **C4.4** Any code-size / RSS / decode claim enters without its own instrument and attestation ⇒
  **the claim is discarded**, not the job. There is no citation-grade RSS substrate in the project
  (`Q2-…-CRITIC.md` §3.6(b): `zstd` reports a constant 1.613 MiB peak decode RSS across a 5×
  input range — that is not a working set).

### WAVE 3 — the corrected parity job *(only if Waves 1–2 preserve its premise)*

**5. Q2 (revised).** Class-A token lane only, `parse != "ratio"`, `bwt_aux` unreachable,
`decode_threads = 1` in the classification vehicle. 2×2 in two non-size factors:
`--block ∈ {262144, 67108864}` × `--negate ∈ {on, off}` (G0/G1/G2/G3). **Five FRONT-GAP
configurations only** — adding `anvil-dp-rans` etc. converts Q2 into the 20-arm dense vehicle Q0
explicitly forbade. Decodes: `G1−G0` = total geometry; `(G1−G2)−(G0−G3)` = gate-sampling component
removed; **`G0−G3` = the pure window/entropy-restart answer.**

- **C5.1 STOP** — `G0 − G3 ≠ 0` bytes on the 6 files with `nblocks ≤ 2`. The harness is not
  geometry-controlled.
- **C5.2** declared predicted byte effect `< 7 %` on the informative subset ⇒ timing arms
  inadmissible. Byte arms may run, but the result is a configuration note.
- **C5.3 CANCEL CONDITION — informative subset (`nblocks ≥ 4`) ≤ 4 files.** Record as *"corpus
  structurally under-powered for the parity question"* — **not** as a null. Do not report an
  optimum; two block sizes identify a contrast, not a curve.
- **C5.4** No result from Q2 may promote anything: **every row is `evidence_role = discovery`**
  (`tests/corpus` = 24 files / ~4–5 units; zero held-out structured, zero numeric, per `[V]` V1–V3).
- **C5.5 MANDATORY DISCLOSURE** — parity deletes decode parallelism. At parity,
  `nthreads = min(threads, nblocks) = 1`, by construction, at any thread count. **A bytes-only
  parity win is a decode regression of order core-count**, far larger than the restart tax that
  motivated the job. Q2 may not recommend a block-size change without the non-classifying
  parallelism column, and may not authorize an adaptive-blocking mechanism claim (occupied by
  Brotli's meta-block splitter and zstd's `targetCBlockSize`).

### WAVE 4 — adopt-class engineering with a live premise

**6. Q4b — post-aux LF-walk share, byte-only + ratio record.** `dickens` + `webster`, frozen I10-1A
candidate, split postcoder / ISA / LF walk / CRC / framing+allocation. **Byte-identical containers to
the I10-1A candidate; instrumented decoder verified byte-identical on every block.**

- **Frozen thresholds (dimensionally correct, from the perf review C3):** `w < 0.420` on either cell
  ⇒ **KILL WSI**. `0.420 ≤ w < 0.488` ⇒ KILL on the enwik8-vs-xz path, conditional hold for
  Silesia. `w ≥ 0.647` ⇒ admissible, and the required kernel speedup
  `s ≥ 1/(1 − (1−1/R)/w)` must be published **before** any prototype.
- **C6.1** **No implementation prototype is authorized under any outcome.** `f_walk` optimisation is
  already landed as I10-1A.
- **C6.2** If Wave 0 item 0.1 does not correct closeout §4.3, **Q4b does not run** — it would be
  running to re-derive a kill that is already (wrongly) on the record.

**7. Q4a — typed/frontend basis × BWT byte cross-product, 4 F1 files**
(`mozilla`/`sao`/`ooffice`/`samba` = 64.7 %/24.2 %/7.8 %/3.4 % of the gap; all other files are 0).

- **C7.1** Must not be co-instrumented with, or run on the same factor space as, Q2. (B1.)
- **C7.2** If `QBYTES` shows BWT aux + per-block framing already consumes the cross-product's byte
  budget at parity geometry ⇒ answered negatively without dispatch.
- **C7.3** All results are `{adopt-class}`/`{engineering}`. A positive result is a **Pareto-candidate
  row pending Q0, never a crossing**.

**8. Q9 — aux W1024 vs W16, byte/correctness only.** Runs **only if** C2.3 did not fire.

- **C8.1** Any speed surprise that trades away the already-landed aux win ⇒ revert and record. The
  absolute byte prize is small; it is not worth the regression.
- **C8.2** Must report peak aux-index bytes.
- **C8.3** If Q2 is ever re-scoped to include ratio/BWT arms, **Q9 is suspended** — the two levers
  are not separable then.

### WAVE 5 — dependency-free infrastructure

**9. Q8 — declared-total / work-amplification probe.** Security infrastructure, no novelty claim.

- **C9 — there are no upstream cancel conditions.** This job cannot be cancelled by any other job's
  result. It should run as **Wave 1 filler**, not Wave 5, purely for scheduling reasons.
- Behind it: a 64 KiB rev-1 artifact can legally declare on the order of **585 GiB** of declared
  output/work under the current proxy. Correctness-consistent, operationally weak, fail-closed for
  memory safety. **Treat as security infrastructure. Do not redesign the format first.** Specify the
  caller-side output/work ceiling contract; run the cheapest synthetic byte-only probe.

---

## 7. Consolidated cancel-condition index

| condition | effect | fired by |
|---|---|---|
| `independence_units ≤ 6` | promotion track formally dead; stop promotion-shaped spend | Q1a′ |
| second implementation returns different components | Q1b pointless | Q1a′ |
| mask share `< 2 %` **or** uniqueness ≤ 90 % | **Q5 cancelled without dispatch** | QBYTES |
| aux share `< 0.1 %` | **Q9 cancelled without dispatch** | QBYTES |
| ≥ 8/13 files single-block at 256 KiB | Q2 → configuration note | QBYTES |
| any Class-A byte differs from frozen CSV | build drift; Q2 baseline void | Q2F |
| `precomp+Brotli` loses to `precomp+xz -9e` | **Q7 KILL** | Q7 |
| denser dense-grid reference closes the gap | label `BYTE-WIN`, not frontier | Q7 |
| `precomp` not producer-identity replayable | **Q7 STOP** | Q7 |
| `G0 − G3 ≠ 0` on `nblocks ≤ 2` files | harness not geometry-controlled; **STOP** | Q2 |
| predicted byte effect `< 7 %` | timing arms inadmissible | Q2 |
| informative subset ≤ 4 files | **Q2 CANCELLED** — under-powered, not null | Q2 |
| `w < 0.420` | **WSI KILL** | Q4b |
| `0.420 ≤ w < 0.488` | KILL on enwik8-vs-xz | Q4b |
| any outcome | **no WSI/lane-scheduler prototype** (I10-1A re-burn) | Q4b |
| BWT aux+framing consumes cross-product budget | Q4a answered without dispatch | QBYTES |
| any speed regression vs landed aux win | revert Q9 | Q9 |
| closeout §4.3 not corrected | Q4b does not run | Wave 0 |
| Q2 re-scoped to include BWT | Q9 suspended | Q2 |

---

## 8. Removed from the remote queue (misfiled — zero CI)

These are not experiments. Filing them as remote jobs implies runner cost and dispatch authority
they do not have.

| item | true nature | cost |
|---|---|---|
| **Q0b** | documentation of the withdrawn H0 theorem | 0 CI |
| **Q0c closure** | reconcile the two lanes, publish the ruling (`:577-591`) | 0 CI |
| **Q3 census / candidate table** | a log statement beside `g_stream_log_entries`; **identical to the critic's M4** | 1 local build, 0 CI |
| **B2** | Track 02 vs Track 16 plan reconciliation | 0 CI |
| **B4 / B3 closure** | record the rulings | 0 CI |
| **Q1a′** | deterministic artifact audit, ~2 s, 165 KB RAM | **local, not Actions** |

The candidate-table artifact (§5.3) is the highest-value item on this list and the cheapest. It
makes the objective-level disagreement rate computable, which no retained artifact supports, and it
is the substrate the Q0c critic calls *"the single highest-value logging change."*

---

## 9. Single authoritative dispatch order

Supersedes the five in X5.

```
WAVE 0   coordinator reconciliation (0 CI)  — X1, X2, X3, X4, thresholds, order, corpus count
   |
   +-- Q1a'   [local, ~2 s, deterministic]   -- CAN promote nothing; can kill the promotion track
   +-- QBYTES [byte-only, 1 runner]          -- CAN cancel Q5 and Q9 without dispatch; can downgrade Q2
   |     includes Q2F as its fidelity arm
   +-- Q8     [byte-only, no dependencies]   -- cannot be cancelled upstream; run as filler
   |
   +-- Q7     [byte-only]                    -- largest measured byte EV; no decode work
   |
   +-- Q2     [byte-only + descriptive parallelism]  -- only if QBYTES keeps the informative subset
   |
   +-- Q4b    [byte-only + ratio record]     -- only if Wave 0 corrected closeout 4.3
   +-- Q4a    [byte-only, 4 files]           -- only if Q2 established a fair-geometry baseline
   +-- Q9     [byte-only]                    -- only if QBYTES did not cancel it

CLOSED    Q3, Q3 timing, Q3 A0-vs-A1 choice, Q5 (conditional), Q6, Q4b implementation,
          B1, B2, B3, B4, WSI/lane-scheduler, TCOPY/PNRA novelty, FLI, H2/C1, RSC,
          PRA-1 H0 prune, parser-frontier mechanism, CAM novelty, CDR/routing novelty
```

**Queue invariant, restated to bite:** *a downstream job whose premise is invalidated by an upstream
result is **cancelled**, not run for completeness.* Applied literally, this rule cancels **seven**
items before the first runner is dispatched: Q3 (×3 formulations), Q4b's implementation half, Q6,
B3, and — conditionally and pre-emptively — Q5 and Q9 via `QBYTES`' pre-gates.

---

## 10. Standing gates every job inherits

Verbatim intent, applied queue-wide:

- **`QP-NOVELTY`** — a job tagged adopt-class/engineering/synthetic/diagnostic/measured **may not**
  emit novelty, mechanism, contribution, or crossing language. A positive result may authorize
  exactly one thing: an integration or validation run.
- **`QP-NO-PROMOTE` / `QP-CONSUMED`** — no promotion language if any input row is
  `discovery | synthetic_control | self_reference_control | known_stress | external_anchor`; no
  evidence object whose SHA-256 appears in the tombstone. **[V]** the tombstone verifies 6/6 and is
  therefore a real byte-identity gate. Passing Q1a **does not** relax either — Q1a validates the
  *machinery*, not the *portfolio*.
- **`evidence_role` per row; `ceiling_basis_role` on every ceiling; dual aggregate
  (`aggregate_all_roles` + `aggregate_heldout_only`); `"promotion_authorized": false` when
  `n_independence_units = 0`.** **Expect `heldout_only` to be null on every job in this queue.**
- **`"citation-grade"` is forbidden** for any job whose rows are not `heldout`/`external_test`.
  Byte-count determinism is a property of the **codec**, not of the **corpus**.
- **Synthetic buys sensitivity, never generalization.** All 8 `synth-*` files are one generator
  lineage and may not pre-register a threshold.

---

## 11. Claims hygiene of this document

- Nothing was executed except read-only enumeration, `Get-FileHash` over existing corpus bytes, and
  read-only `git log`/`git ls-files`.
- No production file, no source, no ledger, no queue file was modified. The queue file in
  particular remains the coordinator's; every edit proposed in it by other lanes is quoted, not applied.
- `[V]` items in §1 are reproducible from the commands implied by their descriptions.
- **Thresholds I propose (`2 %` mask share, `0.1 %` aux share) are proposals, not findings.** Per
  queue doctrine item 5, a threshold that moves after seeing the sweep is not a threshold. They must
  be coordinator-frozen before `QBYTES` runs or `QBYTES` should not run.
- **[PROJECTED] items** (`≈2.05×` decode upside, `585 GiB` amplification, `2.01–2.57×` walk
  ceilings) are carried forward from prior agents and are **not** usable in a gate. They are cited to
  show why a lane is *not yet* closed, never as evidence that one is open.
- I have **not** reconciled the Q0c constructive/critic disagreement. I have stated both positions
  and argued that the residue is a coordinator stake decision rather than a CI question; the
  reconciliation itself is Wave 0 item 0.3.