# Project Anvil — Frozen Closeout Matrix

> **SUPERSEDED FOR EXECUTION by `COORDINATOR-FREEZE-v2.md`.**
> R1 remains the retained first freeze. v2 incorporates the later Q1a current-digest closeout,
> explicit `audit_complete` semantics, and the final Q2 G-S/reference/parallelism/vocabulary fixes.
> Where R1 and v2 differ, v2 is authoritative.

**Freeze ID:** `ANVIL-CLOSEOUT-2026-10-02-R1`  
**Date:** 2026-10-02  
**Authority:** master coordinator dispatch/control-plane ruling  
**Status:** **FROZEN — NO CODEC CI DISPATCH AUTHORIZED YET**

This document is the single authoritative execution matrix for the 2026-10-02
40-agent closeout. It supersedes the **dispatch order, provisional thresholds,
and lane statuses** in:

- `REMOTE-EXPERIMENT-QUEUE.md`
- `CLOSEOUT-MATRIX-REDTEAM.md`
- earlier coordinator queue prose where it conflicts with this matrix

Those documents and all constructive/critic reports remain evidentiary inputs.
They are **not deleted or rewritten into agreement**. A superseded proposal is
still useful evidence; it simply has no dispatch authority.

---

## 0. Binding doctrine

1. **Pareto movement first.** A job must have a credible path to faster-than-Brotli
   or otherwise better Pareto performance, or it must collapse a prerequisite
   uncertainty that gates such work.
2. **Mechanism-level novelty only.** Novelty-by-difference, search gaps, local
   parameterization of occupied mechanisms, and adopt-class engineering are not
   novelty.
3. **Evidence classes never blur.** Every number is explicitly
   `MEASURED | DERIVED | PROJECTED`; deterministic byte counts do not make a
   discovery corpus held-out evidence.
4. **Charge the whole mechanism.** Complete emitted bytes, encode/decode work,
   peak memory/RSS where instrumented, binary/code cost, model/table/index
   overhead, restart/window geometry, and decoder-side work are charged.
5. **No post-hoc gates.** Preregistered gates stay frozen. If a gate is invalid,
   withdraw it; do not tune it after observing the sweep.
6. **Heavy codec/performance work is GitHub-Actions-only.** Local work is limited
   to static analysis, exact arithmetic, metadata/provenance auditing, tiny
   deterministic self-tests, and other zero/negligible-compute checks.
7. **Remote source identity is mandatory.** No authoritative Actions result
   exists unless the workflow, implementation, inputs, and evidence producer are
   pinned and attestable end-to-end.
8. **Manual-only Actions.** New ANVIL research workflows remain
   `workflow_dispatch` only, `contents: read`, no secrets, no automatic
   `push`/schedule trigger, and `cancel-in-progress: false`.
9. **Evidence-role firewall.** `discovery`, `synthetic_control`,
   `self_reference_control`, `known_stress`, and consumed/anchor material
   cannot authorize promotion. Passing the audit validates machinery, not the
   portfolio.
10. **Queue cancellation is real.** When an upstream result removes a premise,
    the downstream job is cancelled rather than run for completeness.
11. **No novelty lane receives compute without a new structural separator.**
    Interesting engineering alone is not a reason to reopen a closed novelty
    program.
12. **No frontier labels from diagnostic instruments.** Q1/Q2/Q4-style
    substrate jobs publish facts and exact contrasts; the coordinator performs
    any later frontier classification.

---

## 1. Freeze summary

### 1.1 What is known now

- The retained dense Class-A grid contains **435 DOMINATED / 28 DEGENERATE /
  5 unresolved FRONT-GAP / 0 FRONT-CROSSING** cells. No retained configuration
  is a verified crossing.
- No current mechanism-level novelty claim survives the prior-art kill pass
  cleanly. H2/C1 collapses to localized/parameterized BCJ; FLI separators are
  anticipated or weakened; CAM/model-state seeding and generic routing/CDR are
  occupied/adopt-class; TCOPY/PNRA residue is too small to justify a novelty
  program without new structural evidence.
- Track 12's PRA-1 empirical-`H0` pruning theorem is withdrawn: zero-order
  empirical entropy is not a valid lower bound on Brotli/LZ output.
- Track 17 parser work cannot move the canonical ratio frontier because
  `--parse=ratio` bypasses ANVIL's token parser.
- Class-A retained evidence has a real block/window parity confound; Class B is
  already at parity.
- The shipped/current cost-objective question no longer justifies a remote Q3:
  Q0c closes the remote arbiter; only optional local deterministic logging/census
  remains.
- Q9's aux-index rate change is closed by exact arithmetic: **22,024 B** maximum
  absolute serialized prize over the relevant retained portfolio, with no byte
  deficit on that portfolio and an adverse decode-work direction.
- Q5 is **not arithmetically killed**. The attempted 47,093 B bound was invalid
  and is retracted. Actual mask-wire size and a strict replacement-code ceiling
  are not retained; Q5 is deferred and gets no standalone runner.
- Q2's corrected 2x2 instrument exists as an isolated draft and is statically
  validated, but is not installed or dispatchable.
- The current Q1a scaffold self-test at tool SHA-256
  `0662dd491f04d0fb1d2a78aad653418959ef10f3db52223f6d435a51e58b160f`
  is **133/133 PASS**, and the independent critic rules the tool **PASS-FOR-CURRENT-SCOPE**. The real current-tree audit reports **24 objects / 204 conflict edges / 9 not-evaluated rows / 4 connected components / INVALID_INFRA**, with every promotion bit false. That does **not** make the corpus audit authoritative: all 9 not-evaluated rows are c9, `graph_verification` is `attestation_backed`, `independence_units_authoritative_for_promotion` is false, and the local current-tree subject is dirty/non-pinnable.
- The current local corpus inventory is **24 declared data objects, 13 present
  at the pinned local commit, 11 absent/untracked**. A current-tree audit is
  discovery of the audit machinery, not citation-grade corpus evidence.
- The GitHub Actions substrate audit found **0/16 existing workflows acceptable
  as the first authoritative run** under the frozen end-to-end identity
  doctrine.

### 1.2 Immediate consequence

> **No codec benchmark is dispatched from this freeze.**

The first remote action is an **evidence-substrate attestation**, and it occurs
only after Q1a's c9/provenance blockers are cleared and the required artifacts
exist at a clean, approved remote source identity.

---

## 2. Authoritative status matrix

| Item | Frozen status | Execution locus | What it can still decide | Mandatory prerequisite / stop condition | Promotion ceiling |
|---|---|---|---|---|---|
| **Q0 retained reclassification** | **DONE** | zero-CI retained arithmetic | Establishes 435/28/5/0 baseline | none | none; retained evidence only |
| **Q0b PRA-1 theorem cleanup** | **DONE / DOCUMENTATION** | local, zero CI | Records withdrawal of invalid H0 lower bound | no backend-valid bound => no search pilot | none |
| **Q0c / Q3 objective reconciliation** | **REMOTE CLOSED** | optional local census only | Deterministic candidate/log table if useful | no physical lambda respecification without a new application-level stake and preregistration | engineering only |
| **Q1a corpus admissibility** | **TOOLING PASS / AUTHORITATIVE HOLD** | local deterministic audit first | Validates evidence machinery and computes an explicitly non-authoritative graph until c9 is trustworthy | c9 verified; schema-bypass accounting explicit; audit tool self-pinned; source clean/pinned | cannot promote anything by itself |
| **Q1a-XRUN** | **FIRST REMOTE, BLOCKED** | GitHub Actions, no codec/network | Cross-runner determinism + source/input identity attestation | Q1a final tool/lock/tombstone tracked at clean approved commit; c9 cleared; workflow tracked; exact expected digests preregistered | infrastructure only |
| **Q2F retained fidelity** | **BUNDLED PREFLIGHT** | first stage of Q2 Actions job | Exact per-file Class-A byte identity vs retained baseline | any admitted control byte mismatch => STOP as build/source drift before geometry arms | diagnostic only |
| **Q2 block/window parity** | **PREPARED / HOLD** | GitHub Actions only | Exact geometry, gate, and interaction byte contrasts under matched design | Q1a/source substrate green; manual workflow tracked; Q2F exact fidelity green; 13-file population fixed; build-output slots reproducibly attested or predeclared/excluded | engineering/configuration evidence only; no frontier labels |
| **Q8 work-amplification security probe** | **READY AFTER SUBSTRATE** | GitHub Actions, synthetic/byte-only | Operational output/work ceiling contract for current decoder | source identity substrate green; no format redesign before probe | security infrastructure only |
| **Q7 DEFLATE reconstruction** | **HOLD / CONDITIONAL** | GitHub Actions only | Whether reconstruction survives producer-identity replay and dual external controls | replayable producer coverage; frozen `precomp->xz -9e`; frozen `precomp->Brotli q11/lgwin30`; best dense-grid reference also reported | adopt-class engineering; BYTE-WIN != crossing |
| **Q4b BWT stage decomposition** | **CONDITIONAL** | GitHub Actions only | Postcoder / ISA construction / LF walk / CRC / framing+allocation share; only if result can change a concrete implementation decision | source substrate green; no generic decoder re-burn; exact instrumented/uninstrumented identity; no WSI/lane-scheduler prototype merely because a share is interesting | adopt-class speed decomposition |
| **Q4a frontend/typed basis x BWT byte cross-product** | **CONDITIONAL** | GitHub Actions only | Whether frontend basis and BWT backend interact in bytes after fair geometry | fair-geometry premise established; exact complete bytes; no factor-space splice with Q2 | adopt-class Pareto-candidate data only |
| **Q5 mask ceiling** | **DEFERRED / NO STANDALONE RUNNER** | piggyback only | Actual mask-wire bytes and strict coding ceiling, if obtainable at zero incremental CI cost | may ride only on an already-authorized job that is already emitting the needed per-stream columns; otherwise remains deferred | `authorizes_prototype:false` unconditionally |
| **Q9 aux W1024->W16** | **CLOSED** | zero CI | already answered: 22,024 B absolute prize | no run | engineering tidiness only |
| **Q6 G5D census/program** | **CLOSED AS CONSTITUTED** | none | no remaining frontier/novelty question worth runner cost | genuinely new measured separator required to reopen | adopt-class |
| **B1 numeric held-out route** | **BLOCKED / NO PROMOTION** | acquisition/provenance work only | could create a future valid evidence family | genuinely new locked real numeric/telemetry family | none until acquired |
| **B2 Track02/Track16 planner-routing** | **ZERO-CI RECONCILIATION** | local docs/static | reconcile useful cheap routing diagnostics | no duplicate planner pilots | engineering only |
| **B3 Track12 POOL/PRA search** | **CLOSED** | none | invalid safe-prune theorem withdrawn | backend-valid exact lower bound or genuinely different search theorem required to reopen | none |
| **B4 CAM/model prior** | **NOVELTY CLOSED; ENGINEERING PARKED** | none now | only residual model-table-delta engineering after free block-size null | Q2 must show residual warmup after fair geometry and larger-block null exhausted | engineering only |
| **Track17 parser frontier** | **CLOSED** | none | parser can still be infrastructure | canonical ratio route must materially change before frontier spend is possible | infrastructure |
| **H2/C1 novelty** | **CLOSED** | none | BCJ-local engineering only | genuinely new semantic separator vs global/parameterized BCJ required | no novelty claim |
| **FLI novelty** | **CLOSED / WEAKENED** | none | no current high-leverage separator | new mechanism-level separator required | no novelty claim |
| **TCOPY/PNRA novelty** | **CLOSED AS FRONTIER PROGRAM** | none | optional engineering/claim closure only | new measured structural prize, not zero-bit residue | no novelty claim |
| **RSC** | **CLOSED** | none | measured near-zero contiguous coverage | new mechanism required | none |
| **generic CDR/routing novelty** | **CLOSED** | none | cheap headroom/routing diagnostics may survive | no novelty framing | engineering only |

---

## 3. Q1a: exact blocker definition

Q1a is deliberately **not** being allowed to hide uncertainty behind a
conservative union-find result.

### 3.1 What is verified in the current scaffold

At tool SHA-256
`0662dd491f04d0fb1d2a78aad653418959ef10f3db52223f6d435a51e58b160f`:

- bounded self-test: **133 checks, 0 failed**;
- fixture: 13 entries, 7 computed units, 30 conflict edges,
  113 explicitly not-evaluated criterion rows;
- lawful subject-root broker exists;
- sealed-role access is refused before bytes are produced;
- path traversal/absolute-path escapes are refused;
- open-role SHA-256 is checked before use;
- missing root / missing local path / tampered bytes fail closed;
- curator duplicate annotations do not override computed grouping;
- thresholds are externalized into the lock;
- consumed controls are tracked by byte identity and cannot become held-out
  evidence merely by relabeling;
- current-tree provenance is required to expose dirtiness rather than assert a
  clean pinned checkout.

### 3.2 What is **not** verified

Criterion c9 is still load-bearing and incomplete:

- c9 edges are sourced from `deduplication.containment_claims[]`;
- those edges are labelled `evaluated_from: publisher_attestation`;
- they are `verified:false`;
- for an open structured pair with bytes available and no claim, the current
  implementation explicitly reports that record-set decoding/containment is not
  implemented.

Therefore the resulting connected components are **attestation-backed**, not a
fully computed content graph. A conservative attestation may merge units, but it
cannot make the independence count authoritative for promotion.

### 3.3 Q1a release condition

Q1a leaves **AUTHORITATIVE HOLD** only when **one** of these is true:

1. c9 is computed directly for every applicable open-role structured/container
   pair from brokered bytes under the frozen lock; **or** c9 comes from a
   separately retained producer whose implementation SHA, input SHA set, exact
   output, and verification semantics are pinned and independently reproducible;
2. every graph-bypass path makes incompleteness machine-readable: either populate
   the corresponding `not_evaluated` rows or emit `audit_complete:false`; a zero
   count must never be ambiguous with an exhaustive audit;
3. the evidence contract commits the audit tool's own SHA-256 (not merely a
   transform-step digest or a self-reported hash); and
4. the lock is instantiated from a genuinely clean pinned subject tree that
   contains every declared object intended to participate.

Only after these local conditions are true does Q1a-XRUN test cross-runner /
cross-interpreter reproducibility of the exact frozen artifact set.

---

## 4. Q2 frozen interpretation contract

Q2 is a parity instrument, not a reclassifier.

For each retained configuration/file:

```
D_ship       = G1 - G0
D_gateoff    = G2 - G3
D_gate_small = G0 - G3
D_gate_large = G1 - G2
I            = (G1 - G2) - (G0 - G3)
             = (G1 - G0) - (G2 - G3)
```

Where:

- G0 = small geometry, gate ON
- G1 = large geometry, gate ON
- G2 = large geometry, gate OFF
- G3 = small geometry, gate OFF

Binding rules:

- no epsilon;
- no materiality threshold;
- no `FRONT-GAP`, `FRONT-CROSSING`, `DOMINATED`, `DEGENERATE`, or
  substitute frontier label emitted by Q2;
- no "pure window" claim — the geometry contrast bundles window extension,
  removal of per-block encoder-state resets, and block-raw-decision changes;
- `decode_threads=1` for the classification vehicle;
- structural parallel capacity may be reported as
  `min(N, block_count)`; no throughput magnitude follows without a timing
  instrument;
- ratio/BWT arms are outside Q2;
- any retained-baseline mismatch is **build/source drift**, not a parity result.

The frozen 13-file population is mechanically derived from the retained
Class-A CSV. The 24-file current corpus must not silently replace it.

---

## 5. Zero-CI arithmetic closures

### 5.1 Q9

The aux-index W1024->W16 lever is fully determined by serialized field
arithmetic.

- Silesia payload convention: 19,328 B
- Silesia complete container delta: 19,344 B
- reconciliation: `19,328 + 48 framing - 32 legacy-primary-varints = 19,344`
- Silesia + enwik8 total absolute prize: **22,024 B**

The relevant retained portfolio is already ahead on bytes. The change also
reduces checkpoints and therefore can increase inverse-walk work. No runner is
spent to widen an already-won byte bar by 22,024 B while risking the binding
decode leg.

### 5.2 Q5 correction

The previously proposed

```
min(mask-bearing rows) - min(mask-free rows)
```

is **not an upper bound** on removable mask overhead because the mask-bearing
parse can have better match structure than every exact-only mask-free parse.

The prior 47,093 B / 13-of-13 kill is retracted. The honest retained state is:

- actual encoded mask-wire size: unknown;
- per-block P2/N2: not retained;
- strict coding ceiling: unknown;
- masks are recorded as highly unique and the historical modeled variant is
  adverse, but those facts do not manufacture an exact byte ceiling.

Hence DEFERRED, not CLOSED.

---

## 6. Single remote order

There is one authoritative order. Every arrow is a gate, not a suggestion.

```
ZERO-CI FREEZE
    |
    +-- finish Q1a c9 + clean/published source identity
    |
    v
Q1a-XRUN
    cross-runner evidence/source attestation
    NO codec, NO network fetch, NO timing
    |
    +-- fail identity/determinism => STOP ALL authoritative codec CI
    |
    v
Q8
    bounded work-amplification/security probe
    |
    +-------------------------------+
    |                               |
    v                               v
Q7 (if producer controls frozen)    Q2 (Q2F preflight first)
                                    |
                                    +-- baseline mismatch => STOP as drift
                                    |
                                    v
                           fair Class-A geometry evidence
                                    |
                         +----------+----------+
                         |                     |
                         v                     v
                    Q4b only if          Q4a only if
                    decisional           premise survives
```

Additional rules:

- Q5 may piggyback only when its required counters are already free.
- Q9 never dispatches.
- Q3 remote never dispatches.
- closed novelty lanes receive zero compute.
- Q7 and Q2 may swap wall-clock order only when CI scheduling makes that cheaper;
  neither is allowed to consume the other's premise or evidence class.
- Q4b is not run merely to publish an interesting stage percentage. Before
  dispatch, the coordinator must state the exact implementation decision that
  each possible result changes.

---

## 7. GitHub Actions substrate gate

Before **any** authoritative remote job:

1. workflow file exists in the approved remote source tree;
2. `workflow_dispatch` only;
3. `permissions: contents: read`;
4. no secrets and no write token;
5. `cancel-in-progress: false`;
6. action dependencies commit-pinned;
7. checkout source identity asserted after checkout;
8. implementation/tool/input blob identities asserted before measurement;
9. clean checkout asserted;
10. artifacts include source identity in their manifest/name;
11. artifact upload executes on failure where evidence of the failure matters;
12. no mutable corpus fetch or MD5-only identity path;
13. expected negative verdicts (for example `CORPUS_BLOCKED`) are recorded as
    scientific results rather than conflated with workflow failure.

Until these hold, GitHub Actions being "remote" does not make a result
authoritative.

---

## 8. Closed novelty ledger

The following receive **zero compute as novelty programs** under R1:

- H2/C1 / localized transformed-reference novelty;
- TCOPY/PNRA zero-bit-parameter novelty;
- FLI micro-program/ISA novelty;
- CAM/model-state seeding novelty;
- generic CDR/routing novelty;
- parser-search novelty on the ratio frontier;
- fixed-cap BWT subblocking novelty;
- correction-template/rep tricks whose only separator is where an occupied
  representation is applied;
- CANL/numeric-compiler novelty absent a new mechanism and real locked family;
- RSC contiguous relocation compiler.

A lane can leave this ledger only with a **new structural separator** documented
before compute, including the semantic prior-art comparison and the concrete
measured frontier prize it could plausibly recover.

---

## 9. What remains unresolved

R1 intentionally does **not** manufacture answers for:

1. the five retained Class-A FRONT-GAP timing cells — Q2 does not resolve their
   timing/noise status;
2. fully verified corpus independence — c9 is not yet content-computed;
3. the actual correction-mask wire/coding ceiling — Q5 remains deferred;
4. BWT stage share — unmeasured and only worth a runner if decisional;
5. DEFLATE reconstruction producer replayability — Track 08 remains HOLD;
6. a genuinely independent structured/numeric/executable promotion portfolio —
   current material is discovery/control/consumed.

These are explicit unknowns, not permission to proliferate mechanisms.

---

## 10. Freeze-change protocol

A future coordinator may change R1 only by adding a new frozen revision that
states:

- the exact R1 premise invalidated;
- new evidence and its class;
- source/input identities;
- whether a preregistered gate fired or was proven invalid;
- which downstream rows become newly authorized or newly cancelled.

Do **not** silently edit an old threshold or reinterpret a historical diagnostic.

---

## 11. Coordinator closeout

The 40-agent swarm has served its intended purpose: it collapsed a large
mechanism search into a much smaller set of substrate questions. The program is
now bottlenecked by **evidence validity and fair measurement**, not by a shortage
of mechanisms.

The next useful byte of compute is therefore not another codec idea. It is the
minimum work required to make the next measurement trustworthy.
