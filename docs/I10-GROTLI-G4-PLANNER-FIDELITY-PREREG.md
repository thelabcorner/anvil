# ANVIL I10 — GROTLI G4 Planner Fidelity Preregistration

**Status:** FROZEN BEFORE G4 IMPLEMENTATION  
**Date:** 2026-09-24  
**Parent implementation:** frozen G3 representation semantics  
**New representation families authorized:** **none**  
**Production integration authorized:** no  
**Heavy benchmarking:** GitHub Actions only

---

## 0. Purpose

G4 is a planner experiment, not a compression-mechanism experiment.

The frozen G3 research implementation ranks leaf candidates with isolated
Brotli q11 evaluations.

That produces useful oracle labels but is far too expensive for a scalable
representation compiler.

G4 asks:

> **Can cheap deterministic candidate scores reproduce the frozen q11 oracle
> closely enough that complete-carrier compression stays effectively unchanged,
> while eliminating nearly all q11 leaf-ranking calls?**

G4 must not change:

- frame/region eligibility;
- lexical parser;
- shape identity;
- carrier grammar;
- leaf payload formats;
- leaf eligibility;
- raw fallback;
- final Brotli q11/lgwin30 backend.

---

## 1. Evidence motivating G4

Frozen G3 discovery run `35947013432` reports, on D3:

- 21,895 candidate columns;
- tens of thousands of isolated q11 leaf evaluations;
- candidate construction/local score time around 32 seconds;
- final local argmin selection itself around milliseconds.

An exploratory read-only analysis of the already-produced discovery artifact
also shows:

- choosing integer-family leaves by serialized payload size agrees with the
  isolated-q11 oracle on ~99.87% of columns;
- choosing dictionary-family leaves by serialized payload size agrees on only
  ~92.6%.

Those exploratory numbers are **not a G4 success result**.

They motivate separate surrogate treatment by family.

---

## 2. Frozen representation basis

Exactly the G3 basis:

- RAW_LEX
- EXACT_DICT
- INT_FOR
- INT_DELTA_FOR
- INT_DOD_FOR

No new leaf may be added in G4.

No payload format may change.

---

## 3. Oracle definition

For each eligible leaf candidate, the **oracle score** is the exact frozen G3
isolated score:

```
Brotli-q11-lgwin30(
    complete serialized isolated leaf object
)
```

RAW_LEX wins exact ties, matching G3.

The oracle is used only to label/evaluate G4 planner fidelity.

It is not the intended production planner.

---

## 4. Frozen surrogate arms

G4 evaluates these candidate-ranking surfaces.

### S0 — serialized payload bytes

Score:

```
serialized isolated leaf object bytes
```

No compression call.

Purpose:

- analytical floor;
- expected to be especially strong for compact integer leaves.

### S1 — order-0 byte estimate

Score the serialized isolated leaf object with:

- exact serialized byte length;
- empirical byte histogram;
- zero-order ideal-code estimate;
- fixed deterministic metadata charge.

No LZ match search.

No Brotli call.

This tests whether simple byte statistics recover dictionary-vs-raw choices
that payload length misses.

### S2 — lightweight backend bridge

Score the exact isolated leaf object with **Brotli quality 1**, same window
where API-compatible.

This is not the long-term target.

It tests whether a much cheaper member of the same codec family preserves q11
candidate ordering sufficiently well.

S2 may perform one q1 call per candidate but **zero q11 ranking calls**.

### S3 — family-hybrid planner

Use:

- S0 for integer-family competition;
- whichever of S0/S1/S2 has the best preregistered discovery fidelity for
  RAW_LEX vs EXACT_DICT.

Important:

S3's selection rule is frozen by the discovery ruling logic below. It may not be
manually changed after per-file complete-carrier results are inspected.

---

## 5. Discovery population

G4 discovery replays the exact frozen D1-D4 objects already used by G3.

This is **not** an independent generalization claim.

G4 discovery is a planner-fidelity test against an already-known oracle.

The original byte identities remain those frozen in
`I10-GROTLI-G2-CORPUS-FREEZE.md`.

No G4 implementation may inspect a new validation corpus before the discovery
rule selects/fails the planner.

---

## 6. Required measurements

For every file and every surrogate family:

### Candidate-level

- total candidate columns;
- candidate leaves;
- oracle winner;
- surrogate winner;
- exact winner agreement count;
- confusion matrix by leaf family;
- oracle-score regret of surrogate choice;
- candidate score construction time;
- q11 ranking call count;
- q1 ranking call count where applicable.

### Carrier-level

Construct the exact frozen G3 complete carrier using the surrogate-selected
leaves.

Then measure:

- carrier bytes;
- Brotli q11 bytes;
- complete bytes;
- exact roundtrip;
- delta versus oracle-selected G3 carrier;
- delta versus raw Brotli.

Final q11 carrier evaluation is permitted because G4 is testing planner
fidelity, not removing the final backend.

---

## 7. Discovery gates

A surrogate family is **planner-faithful** only if all applicable conditions
hold.

### Correctness

1. exact roundtrip for every carrier;
2. same frozen leaf eligibility and payload semantics;
3. no metadata omission.

### Candidate fidelity

For integer-family choices:

- winner agreement >= **99.5%** aggregate;
- isolated-oracle regret <= **0.25%** aggregate.

For dictionary-vs-raw choices:

- winner agreement >= **95%** aggregate;
- isolated-oracle regret <= **1.0%** aggregate.

These are diagnostic gates; complete-carrier fidelity remains authoritative.

### Complete-carrier fidelity

For each D1-D4 object:

- surrogate-selected complete bytes must be <= **oracle + 0.50%**.

Across the four-file portfolio:

- aggregate surrogate-selected complete bytes must be <= **oracle + 0.20%**.

No file may regress by >1% versus the corresponding oracle-selected carrier.

### Oracle-call reduction

For S0/S1:

- q11 ranking calls = **0**.

For S2:

- q11 ranking calls = **0**;
- q1 calls are reported separately.

The final whole-carrier q11 calls do not count as ranking calls.

---

## 8. Discovery selection of S3

Selection is mechanical.

For the dictionary family, consider S0, S1, S2.

Reject any arm failing complete-carrier fidelity.

Among remaining arms choose by:

1. lowest q11 ranking calls;
2. then lowest q1 ranking calls;
3. then lowest same-job candidate-scoring wall time;
4. then lowest aggregate complete bytes;
5. then S0 > S1 > S2 as deterministic final tie order.

Integer family uses S0 if it passes the frozen integer fidelity gate.

If S0 fails integer fidelity, use the best passing family under the same
mechanical order.

No manual per-file routing is allowed in G4.

---

## 9. Held-out policy

G4 does not reuse G3 V1 as an unseen planner-validation set once G3 validation
has exposed its outcome.

If G4 discovery passes, a **new planner-validation corpus** must be frozen before
its bytes are measured.

That corpus should contain at least:

- one high-cardinality structured population;
- one low-cardinality/dictionary-friendly population;
- one heterogeneous multi-shape population.

The identities, commits, byte sizes, and hashes must be committed in a separate
G4 corpus-freeze document before the validation workflow can fetch them.

Until that occurs, G4 may close only as:

> **planner-fidelity discovery evidence**

not broad planner generalization.

---

## 10. Timing protocol

Heavy timing is GitHub-Actions-only.

Within one runner/job:

- build oracle and surrogate from the same source revision;
- warm symmetrically;
- interleave candidate/oracle timing where practical;
- record runner fingerprint;
- report wall time and peak RSS;
- report deterministic call counts separately from noisy timing.

The primary G4 success criterion is **byte fidelity + oracle-call elimination**.

Timing determines whether a faithful surrogate is operationally worthwhile, not
whether its byte result is true.

---

## 11. What G4 may conclude

If G4 passes:

> the frozen G3 representation can be planned with a substantially cheaper
> ranking surface without materially changing complete compressed size on the
> measured discovery population.

G4 may not conclude:

- universal planner generalization;
- FSST usefulness;
- ALP usefulness;
- shape hierarchy usefulness;
- production Pareto superiority;
- a new ANVIL wire format.

---

## 12. What happens after G4

Only after G4 closes should new representation families be added.

Highest-EV post-G4 lanes:

1. ordering-only attribution;
2. shape hierarchy / singleton-shape reduction;
3. genuine mixed-validity regional corpus;
4. FSST-like high-cardinality string symbols;
5. default/RLE exception coding;
6. float/ALP lane on a true float corpus;
7. cross-column predictor + sparse residual;
8. SIMD/decode-production planner work.

Each remains a separate preregistered mechanism test.
