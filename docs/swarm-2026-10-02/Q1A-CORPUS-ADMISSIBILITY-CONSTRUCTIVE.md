# Q1a — CORPUS-ADMISSIBILITY-v1 constructive closeout

**Date:** 2026-10-02  
**Status:** **IMPLEMENTATION COMPLETE FOR CURRENT SCOPE / CORPUS BLOCKED / NOT REMOTE-AUTHORIZED**  
**Tool SHA-256:** `c0c027e15f2a572d2816396129a1d0ed950bdd86dbf84d3c3d1e6310ccb399a4`  
**Real-lock builder SHA-256:** `cd200f22f9a2ec8cd0b29b4cf9f21eb45c770fd001a555d7dacacde7c58aaa1d`

## Ruling

Q1a now does the job required of a fail-closed corpus-admissibility instrument without pretending the current corpus is promotion-grade.

**Post-critic closeout amendment:** schema-invalid audits now emit `audit_complete=false` with an
explicit reason, so `not_evaluated=[]` can no longer be mistaken for an exhaustive empty audit.
The bounded selftest is **135 checks, 0 failed** after this amendment, including an explicit regression that a schema-aborted graph reports `audit_complete=false`.

The implementation has a real `--subject-root` byte broker for OPEN roles, path confinement, SHA-256-before-use, sealed-role non-materialization, role/tombstone checks, canonical ordering, externally declared decision thresholds, unique gate IDs, explicit source-dirty scope, and machine-readable conflict/independence artifacts.

Criterion **c9 cross-container containment is intentionally not claimed as byte-computed**. The current implementation carries curator/publisher containment attestations as fail-safe edges, labels the graph `attestation_backed`, and forces `authoritative_for_promotion=false`. An attestation-only c9 can never yield `ADMISSIBLE_PILOT`.

## Bounded validation

Final local selftest, with no codec, benchmark, network, or heavy corpus sweep:

- **135 checks, 0 failed**
- fixture entries: 13
- fixture independence units: 7
- fixture conflict edges: 30
- fixture unevaluated criteria: 113
- fixture verdict: `CORPUS_BLOCKED`

The fixture deliberately contains non-promotable/consumed roles, so `CORPUS_BLOCKED` is the expected semantic result; the selftest PASS is about instrument behavior, not corpus promotion.

## Real current-corpus audit

The current 24-object corpus was regenerated from the live repository state and audited through the production CLI.

- declared data objects: **24**
- tracked at `b8eae11`: **13**
- absent from that pinned commit / local-only: **11**
- schema findings: **0**
- lock SHA-256: `936f5fd30384079ed44c1dd679a605954f6e77926edd013691fca92defd06c18`
- computed graph components: **4**
- conflict edges: **204**
- unevaluated c9 cases: **9**
- blocking findings: **1**
- verdict: **`INVALID_INFRA`**
- graph verification: **`attestation_backed`**
- independence units authoritative for promotion: **false**
- promotion authorized: **false**

The only current blocking finding is `PB-02`: the operator worktree is intentionally dirty and therefore is not a clean pinned checkout. The remaining failed gates are:

1. `Q1A-2` — pinned-checkout cleanliness is not satisfied in the live operator tree.
2. `Q1A-8` — c9 is not byte-computed; containment remains attestation-backed.
3. `QP-NO-PROMOTE` — the current evidence portfolio contains discovery/synthetic/known-stress/self-reference roles and zero admissible held-out structured/numeric units.

`QP-CONSUMED` is no longer spuriously duplicated or failing. Gate IDs and `failed_gates` are unique.

## Interpretation

The emitted **4 components are useful discovery structure but are not a promotion-grade independence count**. This distinction is mandatory: unverified c9 could only add conflict edges and reduce the effective count.

There is no need to build a generic record decoder merely to force a green status on this corpus. Promotion is independently blocked by the evidence portfolio. If a future acquisition phase needs a promotion-authoritative independence count, c9 must be implemented over already-open bytes (or an equally strong independently verified criterion) before that result can become authoritative.

## Remote status

**Do not dispatch Q1a as a benchmark.** It is a zero-codec methodology instrument.

A future cross-runner reproducibility job may be justified only after:
- the tool and lock are present in a clean pinned commit,
- source identity is bound end-to-end by the workflow,
- the workflow path itself is tracked and approved,
- and the remote job reproduces deterministic artifact hashes without fetching sealed roles.

Until then the current local result is the correct fail-closed result, not a failed research experiment.
