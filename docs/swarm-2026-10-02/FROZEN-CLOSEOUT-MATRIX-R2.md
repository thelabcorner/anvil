# Project Anvil — Frozen Closeout Matrix R2

**Freeze ID:** `ANVIL-CLOSEOUT-2026-10-02-R2`  
**Date:** 2026-10-02  
**Authority:** master coordinator dispatch/control-plane ruling  
**Status:** **FROZEN — PUBLICATION REQUIRED BEFORE REMOTE DISPATCH**  
**Supersedes for execution:** `FROZEN-CLOSEOUT-MATRIX.md` R1 and every earlier queue/closeout document where they conflict.

R1 remains a provenance record. R2 exists because one R1 dependency was too strong: it coupled
**promotion-authoritative corpus independence (c9)** to **source/CI reproducibility** and therefore
blocked infrastructure/diagnostic work that does not make promotion claims. That coupling is withdrawn.

This file is the single execution authority for the 2026-10-02 swarm closeout.

---

## 0. R2 delta — exact R1 premise invalidated

R1 sequenced:

```text
finish Q1a c9
  -> Q1a-XRUN
  -> all authoritative remote work
```

That is unnecessarily serial and conflates two independent validity questions.

### R2 separates them

**Gate A — source / CI / producer identity**

> Are the implementation, workflow, inputs, reference producer, and emitted artifacts pinned and
> reproducible enough that a remote result means what it says?

This gate is answered by publication + **Q1a-XRUN**. XRUN is allowed—and expected—to reproduce a
semantic result of `CORPUS_BLOCKED` / `attestation_backed`. That is not a failed XRUN. It proves that
the infrastructure reproduces the same bounded result on another runner.

**Gate B — promotion / generalization admissibility**

> Are the corpus independence graph and held-out evidence strong enough to authorize a promotion,
> generalization, mechanism, or novelty claim?

This gate remains blocked by c9 plus the lack of genuinely new independent structured/numeric/
executable held-out families.

**Gate B does not block Gate-A-only work.** After Gate A passes, synthetic security probes,
measurement-parity experiments, and adopt-class discovery/engineering jobs may run when their own
prerequisites are satisfied, but their outputs remain discovery/engineering evidence and may not be
laundered into promotion/generalization claims.

No preregistered negative gate is moved by this change. R2 changes dependency semantics, not an
observed performance threshold.

---

## 1. Binding doctrine

1. **Pareto movement first.** Remote compute must either have a credible path to a faster-than-Brotli
   or otherwise better Pareto point, or collapse a prerequisite uncertainty that gates such work.
2. **Mechanism-level novelty only.** Search gaps, local/global placement changes, parameterization of
   occupied mechanisms, and novelty-by-difference are not novelty.
3. **Evidence classes never blur.** `MEASURED | DERIVED | PROJECTED` remain explicit. A deterministic
   discovery result is not held-out evidence.
4. **Charge the whole mechanism.** Complete emitted bytes, framing, tables, indices, code/binary cost,
   encode/decode work, peak memory/RSS where instrumented, initialization, window/reset geometry, and
   decoder-side work are charged.
5. **No post-hoc gates.** If a gate is mathematically invalid, withdraw it and document the error;
   never tune it after observing results.
6. **Heavy codec/performance work is GitHub-Actions-only.** Local work remains static analysis, exact
   arithmetic, metadata/provenance auditing, bounded deterministic self-tests, and similarly negligible
   checks.
7. **Remote source identity is mandatory.** A remote number is not authoritative merely because it ran
   on Actions.
8. **Manual-only Actions.** Research workflows are `workflow_dispatch` only, `contents: read`, no
   secrets, no automatic push/schedule trigger, and `cancel-in-progress: false`.
9. **Evidence-role firewall.** Discovery, synthetic, self-reference, known-stress, consumed, or anchor
   material cannot authorize promotion.
10. **Upstream kills cancel downstream jobs.** Dead jobs are not run for completeness.
11. **Closed novelty lanes receive zero compute** unless a new structural separator is stated before
    compute and carries a credible measured frontier prize.
12. **Diagnostic instruments emit facts, not frontier labels.** Q1a/Q2/Q4/Q8 do not manufacture
    crossing/novelty/promotion vocabulary.

---

## 2. Frozen evidence state

### 2.1 Retained frontier

The retained Class-A grid remains:

- **435 DOMINATED**
- **28 DEGENERATE**
- **5 unresolved FRONT-GAP**
- **0 FRONT-CROSSING**
- **468 / 468 cells accounted**

No retained ANVIL configuration is a verified crossing.

The five gap cells are unresolved timing/reference-envelope cases, not proof of a candidate win.
Q2 does not reclassify them.

### 2.2 Novelty

No currently funded mechanism-level novelty claim survives cleanly.

Closed as novelty programs:

- H2/C1 / localized transformed-reference normalization;
- TCOPY/PNRA zero-bit-parameter residue;
- FLI micro-program/ISA framing;
- CAM/model-state seeding as novelty;
- generic CDR/routing/adaptive block selection;
- parser search on the canonical ratio route;
- fixed-cap BWT subblocking;
- numeric/executable/correction-template variants whose separator is only placement or representation
  of occupied mechanisms;
- RSC contiguous relocation compiler.

A lane leaves this ledger only with a **new structural semantic separator**, prior-art comparison, and
a plausible measured frontier prize stated before compute.

### 2.3 Hard technical closures

- PRA-1 empirical `H0` safe pruning is withdrawn: empirical zero-order entropy is not a universal
  lower bound on Brotli/LZ output.
- Canonical `--parse=ratio` bypasses ANVIL's token parser; Track 17 is closed as a frontier route.
- Q3 remote selector arbiter is cancelled. The retained A/B byte result is already a zero and the
  objective formulations do not share one defensible physical exchange rate.
- Q9 is closed by exact serialized-field arithmetic at **22,024 B** absolute prize on an already
  byte-winning portfolio, with an adverse decode-work direction.
- The historical arithmetic Q5 kill is withdrawn. Actual mask-wire size/ceiling remains unknown.

---

## 3. Q1a final current-scope state

### 3.1 Frozen local implementation identity

`q1a_corpus_lock.py`

- SHA-256: `c0c027e15f2a572d2816396129a1d0ed950bdd86dbf84d3c3d1e6310ccb399a4`
- prospective Git blob: `0d2467c719daef37d301fa221fb8b4b3d069934b`
- size: 189,792 B
- bounded self-test: **135 / 135 PASS**

Implemented and verified:

- lawful `--subject-root` byte broker for OPEN roles;
- root/path confinement;
- SHA-256-before-use;
- sealed roles structurally excluded before byte materialization;
- zero-network / zero-codec / zero-archive-decompression / zero-sealed-byte counters;
- canonical ordering;
- externalized decision thresholds;
- consumed/tombstone role enforcement;
- unique gate IDs / unique `failed_gates`;
- explicit pinned-checkout vs operator-worktree semantics;
- schema-aborted graph work now reports **`audit_complete:false`**, so
  `0 edges / 0 not_evaluated` cannot be mistaken for exhaustive evaluation.

Current-scope ruling: **PASS-CURRENT-SCOPE**.

### 3.2 Real current-tree audit

The current 24-object local inventory still yields:

- 24 declared objects;
- 13 present at pinned local `b8eae11`;
- 11 absent/local-only;
- 4 graph components;
- 204 conflict edges;
- 9 unevaluated c9 cases;
- verdict **`INVALID_INFRA`** because the operator tree is intentionally dirty;
- graph verification **`attestation_backed`**;
- promotion-authoritative independence: **false**.

This is correct fail-closed behavior, not a failed research experiment.

### 3.3 c9 remains Gate B only

c9 cross-container containment is not byte-computed today. Attested c9 edges are explicitly
`verified:false`, and the graph is marked `attestation_backed`.

Therefore:

- a current independence-unit count may be used as discovery structure;
- it may **not** authorize promotion/generalization;
- an attestation-only c9 can never yield `ADMISSIBLE_PILOT`.

Promotion-authoritative c9 requires either:

1. direct computation over already-open bytes for every applicable structured/container pair; or
2. a separately retained producer with pinned implementation SHA, pinned input SHA set, exact output,
   explicit verification semantics, and independent reproducibility.

**This does not block XRUN or other Gate-A-only diagnostic work.**

---

## 4. Q1a-XRUN — first remote source/CI attestation

### 4.1 Purpose

XRUN asks exactly one question:

> Does a clean GitHub Linux checkout of the preregistered Q1a source reproduce the same canonical
> synthetic artifact bytes and fail-closed semantics as the local preregistration?

It performs **no codec benchmark, no corpus fetch, no sealed-byte read, no archive decompression,
and no package installation**.

### 4.2 Frozen publication candidates

`q1a_xrun_fixture.py`

- SHA-256: `255ec7d56633bc4fa970024d446586cdd18afc3d8d7d5930257c85dc953dc3fb`
- prospective Git blob: `07720d4fbba9e4f77143e873fb70d8a05154da43`

`anvil-q1a-xrun.DRAFT.yml`

- SHA-256: `131cef5e4339d0bbe6509bbbe5bd163d09b43891661bf0fe9ea4f53a9658d74f`
- prospective Git blob: `433c2ad05684fdf031ba38e0fe2d91453ae2e9d0`

The draft uses commit-pinned actions:

- `actions/checkout@11d5960a326750d5838078e36cf38b85af677262`
- `actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02`

### 4.3 Cross-runner preregistration anchors

Two independent local fixture emissions were byte-identical across all **14** emitted files.

Frozen anchors:

- synthetic fixture lock SHA-256:
  `0c4090c84d9de954cbd1bbf3b9b239f4c31413f596c87ef6c55eae62575ce17f`
- artifact-manifest SHA-256:
  **`feaa7848967d8245dc16f5d531fcc1e5c9f2898e941b6f5a424182301c4fec5c`**
- XRUN summary SHA-256:
  **`112add224d0017e6bbb7fad083424d90c3818526416de546f3000e00216dcdaa`**

Expected semantic result:

- `verdict = CORPUS_BLOCKED`
- `audit_complete = true`
- `graph_verification = attestation_backed`
- `independence_units_authoritative_for_promotion = false`
- 7 fixture units
- 30 conflict edges
- 113 not-evaluated rows
- 13 manifest-listed artifacts
- zero network / codec / archive decompression / sealed-byte exposure.

**XRUN PASS means reproducible infrastructure. It does not mean corpus promotion.**

### 4.4 Publication caveat

The files are currently untracked locally and absent on remote `thelabcorner/anvil:i10-aux-unbwt`.
Git reports no attribute filter and `core.autocrlf=false`; the blob IDs above are the prospective clean
blob IDs of the current bytes.

If publication changes **any** Q1a tool/emitter byte, the tool SHA, blob ID, manifest anchor, and
summary anchor must be re-derived before dispatch. Do not preserve a stale expected hash merely to
make XRUN pass.

---

## 5. Q2 Rev 8 — fair block/window parity instrument

### 5.1 Status

**TECHNICALLY PREPARED / PUBLICATION HOLD ONLY.**

Q2 is a byte/provenance parity instrument, not a frontier reclassifier and not a configuration
promotion job.

Its final complete 2x2 is run independently inside all five retained configurations:

- G0 = 262,144 B geometry / incompressibility gate ON
- G1 = 67,108,864 B geometry / gate ON
- G2 = 67,108,864 B geometry / gate OFF
- G3 = 262,144 B geometry / gate OFF

Exact outputs:

```text
geometry effect, gate ON    = G1 - G0
geometry effect, gate OFF   = G2 - G3
gate effect, small geometry = G0 - G3
gate effect, large geometry = G1 - G2
interaction                 = (G1-G2) - (G0-G3)
                            = (G1-G0) - (G2-G3)
```

No epsilon. No materiality threshold. No frontier predicate. No class emitted from a sign, magnitude,
or non-zero interaction.

### 5.2 G-S retained-twin preflight

Before any G1/G2/G3 geometry work:

- run G0 only;
- stratum-A tracked files only;
- require exact per-file candidate byte equality to the frozen Class-A source window.

Mismatch ⇒ **`BASELINE-VOID`**:

- build/source/configuration drift;
- contrasts withheld;
- non-zero exit;
- geometry arms are never spent.

Fresh Linux build-output slots are deliberately excluded from this gate and remain in a separate
diagnostic role stratum.

### 5.3 Reference producer is now immutable

R1/R3 are a **same-run matched geometry-control pair**. They are not spliced to the historical
Brotli rows: historical rows used the default lgwin-22 path; R1 uses explicit lgwin 30.

The reference producer is frozen to official **google/brotli v1.1.0**:

- commit: **`ed738e842d2fbdf2d6459e39267a633c4a9b2f5d`**

The workflow:

- fetches that exact commit into runner temp;
- asserts fetched HEAD exactly;
- builds static `brotlienc`, `brotlidec`, `brotlicommon`;
- links `q2_brotli_geom` directly against those static libraries;
- never uses an unversioned `libbrotli-dev` as the reference producer.

### 5.4 Action/source pinning

Q2 uses the same commit-pinned action SHAs as XRUN.

It also asserts six coordinator-validated production/evidence Git blobs before any codec invocation.

The current workflow is `workflow_dispatch` only, `contents: read`, no secrets, no push/schedule,
finite timeout, and `if: always()` evidence upload.

### 5.5 Final static validation

Final Rev-8 local static checks:

- Python compile: clean
- factorial self-test: **0 failures**
- interaction identity: **PASS**
- environment identity: **PASS**
- frontier-vocabulary scan over workflow + README: **clean**
- floating action references: **0**
- workflow exact forbidden frontier tokens: **0**
- manual-only trigger check: **PASS**

Current SHA-256 identities:

| file | SHA-256 |
|---|---|
| `q2_arms.py` | `f527ae7206327386918a05bb0c494edb5e6291c67bd5e2432d536241403f8bf8` |
| `q2_factorial.py` | `841f861d96b03de894654d47425351cf6f3e6fbf944cdcd01a7d4ffede898f07` |
| `q2_collect.py` | `b9b1d2e93dc61eebcbe8066a12d7838c450cc72654f1900ea4f692d7c0e81dc7` |
| `q2_brotli_geom.cpp` | `6ff05fdf8ca5c3fe14997505edc2a5affa4b6a73cccf22d541c6960c41804cb1` |
| `anvil-q2-parity.DRAFT.yml` | `1fadf1bff755f434441a48de88d0d3d3583ab2d4815dd62734c2f6b6197c6503` |
| `README.md` | `d56b05949e34b4079bb6dcb583c04db668b85380d52c3c02e927fa9ff077179b` |
| Rev-8 plan | `247f41f177fdd27b1d4a72650a073fe3d710968074559f69d7f29eb0abee3dc3` |

Q2 still has **no dispatch authority until its workflow/prototype is tracked on an approved GitHub
branch**. That is now a control-plane publication constraint, not an unresolved scientific design
defect.

---

## 6. Authoritative status matrix

| Item | R2 status | Execution locus | What it may decide | Blocking gate / stop | Claim ceiling |
|---|---|---|---|---|---|
| Q0 retained grid | **DONE** | zero CI | 435/28/5/0 baseline | none | retained evidence |
| Q0b PRA theorem | **DONE / WITHDRAWN CLAIM** | zero CI | records invalid H0 bound | no safe bound => no search pilot | none |
| Q0c / Q3 | **REMOTE CLOSED** | optional local deterministic census | instrumentation substrate only | no physical exchange-rate prereg => no timed arbiter | engineering |
| Q1a tool | **PASS-CURRENT-SCOPE** | local static/audit | fail-closed corpus machinery | c9 still blocks promotion authority | infrastructure |
| **Q1a-XRUN** | **FIRST REMOTE / PUBLICATION HOLD** | GHA, zero codec | cross-runner source + artifact determinism | publish exact frozen files/workflow; any hash mismatch => STOP | infrastructure only |
| c9 / corpus promotion branch | **HOLD** | provenance/content audit | promotion-authoritative independence | compute c9 or verified retained producer + genuinely new held-out families | promotion only after pass |
| **Q8 work-amplification** | **READY AFTER XRUN** | GHA synthetic/byte-only | resource/output/work ceiling contract | source substrate must pass | security infrastructure |
| **Q2 Rev 8 parity** | **PREPARED / PUBLICATION HOLD** | GHA | fair geometry/gate byte contrasts | XRUN source substrate; runtime G-S; tracked approved workflow | engineering/discovery only |
| Q7 DEFLATE controls | **HOLD / CONDITIONAL** | GHA | reconstruction value under replay + dual controls | producer replayability + frozen xz/Brotli controls | adopt-class discovery |
| Q4b BWT stage decomposition | **CONDITIONAL** | GHA | stage shares only if outcome changes a named implementation decision | source substrate + decisional prereg | adopt-class decomposition |
| Q4a frontend x BWT bytes | **CONDITIONAL** | GHA | interaction after fair geometry | fair-geometry premise + exact complete bytes | adopt-class discovery |
| Q5 mask ceiling | **DEFERRED / NO STANDALONE RUNNER** | piggyback only | actual mask bytes/strict ceiling if free | already-authorized host emits needed counters | `authorizes_prototype:false` |
| Q9 aux W1024→W16 | **CLOSED** | zero CI | already answered: 22,024 B | no run | engineering tidiness |
| Q6 G5D as constituted | **CLOSED** | none | — | new measured separator required | adopt-class |
| numeric/executable promotion | **BLOCKED** | acquisition/provenance first | future valid families | new locked independent real families | none today |
| planner/routing duplication | **ZERO-CI RECONCILIATION** | local | cheap headroom diagnostics | no duplicate planner pilots | engineering |
| Track12 PRA search | **CLOSED** | none | — | valid backend-aware exact theorem required | none |
| CAM/model prior | **NOVELTY CLOSED / ENGINEERING PARKED** | none now | possible residual after Q2 only | fair-geometry residual | engineering |
| Track17 parser frontier | **CLOSED** | none | infrastructure only | canonical route would need to change | infrastructure |
| H2 / FLI / TCOPY novelty | **CLOSED** | none | — | new structural separator + measured prize | no novelty claim |

---

## 7. R2 execution DAG

There are now **two independent branches after source attestation**.

```text
LOCAL FREEZE / PUBLICATION
  |
  |  track exact Q1a tool + XRUN fixture/workflow
  |  track exact Q2 Rev-8 prototype/workflow
  |  no semantic result yet
  v
Q1a-XRUN  [FIRST REMOTE, zero codec]
  |
  +-- source/blob/artifact mismatch ------------------------> STOP all authoritative remote work
  |
  +-- reproducibility PASS
        |
        +================ GATE A: SOURCE / CI TRUST =================+
        |                                                            |
        |                                                            |
        v                                                            v
   NON-PROMOTIONAL REMOTE WORK                                  PROMOTION BRANCH
        |                                                            |
        +-- Q8 security probe                                       +-- compute/verify c9
        |                                                            +-- lock new independent families
        +-- Q2 Rev8                                                  +-- held-out role firewall
        |     G-S first                                              |
        |     mismatch -> STOP Q2                                    v
        |
        +-- Q7 only if producer controls frozen                 GATE B: PROMOTION TRUST
        |
        +-- Q4b/Q4a only when decisional
                                                                    |
                                                                    v
                                                       promotion/generalization claims allowed
                                                       only after their own mechanism gates
```

### Binding interpretation

- A `CORPUS_BLOCKED` XRUN can be an **XRUN PASS**.
- Q8 does not need c9 because it is synthetic security infrastructure.
- Q2 does not need c9 to measure fair geometry because it is discovery/engineering and explicitly
  cannot emit promotion/frontier labels.
- Q7/Q4 may produce adopt-class discovery evidence before Gate B, but cannot claim generalization.
- Gate B remains mandatory before any held-out promotion, mechanism-generalization, or novelty claim.

This separation increases information throughput **without weakening any claim boundary**.

---

## 8. Remote order after publication

### Wave 0 — publish and attest

1. Publish the frozen R2 bundle on an approved branch.
2. Install the XRUN workflow at a tracked `.github/workflows/` path with byte-identical content or
   re-derive all expected identities if publication changes bytes.
3. Dispatch **Q1a-XRUN**.
4. Any source/blob/artifact anchor mismatch ⇒ stop and repair publication; no codec CI.

### Wave 1 — high-information substrate

After XRUN passes:

- **Q8** may dispatch immediately: synthetic security/resource probe.
- **Q2 Rev 8** may dispatch when its tracked workflow path is approved.
  - G-S runs before geometry.
  - G-S mismatch ⇒ `BASELINE-VOID`; no geometry arms.
- Q7 remains conditional on its producer/control freeze.
- Gate-B c9/new-family work proceeds in parallel.

### Wave 2 — conditional engineering

- Q4b only if the coordinator states the exact implementation decision each possible stage-share
  result changes.
- Q4a only after fair-geometry premise survives and its result is not a duplicate of Q2.
- Q5 may piggyback only at zero incremental runner cost.
- Q9/Q3/closed novelty lanes never dispatch.

---

## 9. GitHub Actions substrate contract

Every authoritative research workflow must satisfy:

1. tracked on the approved GitHub source ref;
2. manual `workflow_dispatch` only;
3. `permissions: contents: read`;
4. no secrets/write token;
5. `cancel-in-progress: false`;
6. action dependencies pinned by commit SHA;
7. checkout/source ref asserted after checkout;
8. implementation/tool/input identities asserted before measurement;
9. clean checkout asserted;
10. producer identities explicit—no mutable tag or unversioned package as a load-bearing producer;
11. source identity present in artifacts;
12. failure evidence uploaded with `if: always()` where relevant;
13. no MD5-only or mutable corpus identity path for claim-bearing inputs;
14. expected negative semantic verdicts are data, not workflow failure.

A project-level LICENSE file is **not** a measurement-validity prerequisite. Per-corpus provenance/license
evidence remains part of Gate B.

---

## 10. Publication bundle — no commit/push in this freeze

The project brief forbids this coordinator from committing or pushing the dirty local worktree.
Therefore R2 ends with an explicit publication handoff rather than silently violating that constraint.

Minimum files that must be tracked before the first remote run:

### Q1a / XRUN

- `prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/q1a_corpus_lock.py`
- `.../q1a_xrun_fixture.py`
- install byte-identical `anvil-q1a-xrun.DRAFT.yml` as an approved tracked workflow
- relevant Q1a closeout docs / R2 control docs.

Prospective current blobs:

- tool: `0d2467c719daef37d301fa221fb8b4b3d069934b`
- fixture emitter: `07720d4fbba9e4f77143e873fb70d8a05154da43`
- XRUN workflow content: `433c2ad05684fdf031ba38e0fe2d91453ae2e9d0`

### Q2

Track the Rev-8 prototype files and install the draft as the approved Q2 workflow. Current prospective
blobs include:

- `q2_arms.py`: `d6a090731c4eb2495487771a1e3c5fe581c436e5`
- `q2_factorial.py`: `2d2a73062b80a337f79143e766c7936ba6fb5777`
- `q2_collect.py`: `2a0b58da023bf3b84f9e76b756e0bcd75bfa6b7f`
- `q2_brotli_geom.cpp`: `65e05514415afb663b43bf9a9cafa35c7780d68a`
- Q2 workflow content: `8fd9c0d3a94ef3be63f2bfa8f46febd67be3eb18`
- Q2 README: `4c5a50fbaac54da31de91116cc94cd4d9a165b07`
- Rev-8 plan: `db0a17dae25557492c772bdfd1427411d64bad8c`

These are **prospective** blob identities from the current clean-filter rules. The publication actor must
verify the committed blob IDs after staging/commit. If bytes differ, update the preregistration before
dispatch rather than forcing old hashes.

Remote verification already established:

- repository: `thelabcorner/anvil`
- default branch: `main`
- remote branch `i10-aux-unbwt` exists
- as of this freeze, the Q1a tool, R2 artifacts, and Q2 workflow are **not present** on that branch.

---

## 11. Unknowns that remain explicit

R2 does not manufacture answers for:

1. the five retained Class-A timing/noise gap cells;
2. promotion-authoritative c9 containment;
3. genuinely new independent held-out structured/numeric/executable families;
4. actual correction-mask wire/coding ceiling;
5. DEFLATE producer replayability;
6. BWT stage share, unless a future decision makes that measurement worth a runner.

These unknowns are queue gates, not invitations to invent new mechanisms.

---

## 12. Freeze-change protocol

A future R3 may change R2 only by recording:

- exact R2 premise invalidated;
- new evidence and its evidence class;
- exact source/input/producer identities;
- whether a preregistered gate fired or was proven invalid;
- which downstream rows become authorized or cancelled.

Do not silently edit R2 thresholds or reinterpret historical measurements.

---

## 13. Coordinator closeout

The 40-agent search has converged from mechanism proliferation to a much smaller problem:

> **make the next measurement trustworthy, then spend remote compute only where the result changes a
> real decision.**

R2 therefore deliberately increases parallel information gain without weakening claim hygiene:

- source/CI reproducibility is tested first;
- diagnostic/security/fair-measurement work may proceed after that substrate is trusted;
- promotion/generalization remains separately blocked by c9 and new held-out evidence;
- closed novelty lanes remain at zero compute.

The next useful action is **publication of the frozen source/workflow bundle, followed by Q1a-XRUN**.
