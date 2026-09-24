# ANVIL I10 — GROTLI G5A Ordering Attribution Preregistration

**Status:** FROZEN r2 BEFORE ANY G5A BUILD OR D1-D4 / V1 MEASUREMENT  
**Freeze note:** r2 closes two outcome-classification edge cases and makes the existing envelope-identity invariant explicitly machine-gated; no corpus outcome had been measured or built when frozen.  
**Date:** 2026-09-24  
**Parent evidence:** G3 `PASS-G3-NARROW`, G4 `NO-GO-G4`  
**Purpose:** isolate the causal contribution of byte ordering/locality under a fixed structured representation  
**Production authorization:** none

---

## 0. Why G5A exists

G3 established a real but narrow representation effect:

- discovery aggregate: **-5.5984%** versus raw Brotli;
- D3 regionized best: **-4.0690%**;
- held-out V1 regionized best: **+3.1900% worse** than raw Brotli.

G4 then failed to replace the expensive q11 leaf oracle with the tested cheap score
surfaces. Its frozen disposition is to return to representation/anatomy work rather
than expanding the typed basis to rescue the planner hypothesis.

G5A therefore asks a more primitive question:

> **How much compression effect is caused by the ordering/locality of the exact same
> structured scalar byte chunks before the same Brotli backend?**

This is an attribution experiment, not a new codec family and not a planner experiment.

---

## 1. Non-negotiable causal constraint

The three G5A structured arms must differ in exactly one mechanism:

> **the permutation of the same structured scalar chunks.**

For every input, all three arms must have:

- identical parser and frame split;
- identical structured/raw frame classification;
- identical exact-shape identity;
- identical shape templates;
- identical frame-to-group reconstruction map;
- identical raw residual bytes in identical order;
- identical structured scalar token bytes;
- identical scalar token length framing;
- identical structured scalar chunk multiset;
- identical uncompressed carrier-body byte count;
- identical Brotli implementation and parameters;
- no dictionary, integer, float, RLE, FSST, predictor, or other leaf transform.

No arm may omit, canonicalize, normalize, deduplicate, entropy-code, or otherwise
change a scalar token.

If carrier-body sizes differ, G5A has an implementation bug and produces no
scientific ruling.

---

## 2. Frozen parsing and shape semantics

G5A imports the exact G3 parsing/shape semantics from frozen G3 public SHA:

`1a3d18fed76adb6fb33264e1994f9c357306b3fa`

The implementation must include or compile against the frozen G3 source and the CI
workflow must verify the exact frozen source identity before measurement.

Frozen semantics include:

- LF/CRLF/final-remainder frame splitting;
- per-frame exact JSON lexical parsing;
- malformed/incomplete frame -> raw residual;
- byte-identical inter-value template parts define exact shape identity;
- structured shapes assigned in first-appearance order;
- frame reconstruction order explicitly represented.

G5A does **not** change parser eligibility or shape formation.

---

## 3. Common G5A carrier body

The carrier body is deliberately independent of order mode.

Conceptually:

```
magic = "G5AO"
version
source_len
frame_count
group_count
frame_group[frame_count]

group_descriptors[group_count]:
    group_kind
    member_count
    structured:
        slot_count
        template_part_count
        (part_len, part_bytes)...

raw_residual_section:
    for raw members in frozen source order:
        raw_len
        raw_bytes

structured_token_stream:
    exactly N chunks, each:
        token_len
        exact_token_bytes
```

The metadata and raw-residual section are byte-identical across all order arms.

The structured token stream contains the same exact chunks in each arm; only their
permutation changes.

### 3.1 Decoder-visible order mode

The decoder must know which permutation was used. That information is not free.

To prevent the mode byte itself from perturbing Brotli's model and confounding the
ordering comparison:

- the order mode is a **one-byte outer field**;
- it is charged in every arm's complete-byte total;
- it is **not** included in the bytes passed to Brotli;
- the compressed Brotli payload contains only the common carrier body.

Thus:

```
complete_bytes = 1 + brotli(common_body_with_arm_permutation).size()
```

The one charged byte is equal-cost across all three arms. Brotli sees no arm tag.

---

## 4. Frozen arms

Exactly three structured order arms exist.

### A1 — SOURCE_ORDER

Structured scalar chunks appear in original structured-frame order.

For each source frame:

- raw frame: contributes no structured chunk;
- structured frame: emit slot 0, slot 1, ... slot K-1 for that record.

This preserves source-local row adjacency among scalar values while still using the
same shape/template metadata as every other arm.

### A2 — SHAPE_ROW

Group structured records by exact shape in frozen first-appearance shape order.

Within each shape:

- emit members in source order;
- for each member emit slot 0 ... slot K-1.

This isolates the effect of **shape grouping** while preserving row-major locality
inside each shape.

### A3 — SHAPE_COLUMN

Group structured records by exact shape in frozen first-appearance shape order.

Within each shape:

- emit slot 0 across all members;
- then slot 1 across all members;
- ...
- then slot K-1 across all members.

This is the pure corresponding-placeholder / column-local ordering arm.

There are no other G5A arms.

---

## 5. Context arms that do not enter the causal gate

The CI result may report, as clearly labeled context:

- raw Brotli of the original source;
- frozen G3 `REGION_RAW` complete bytes.

Those objects do **not** share the G5A common envelope and therefore cannot be used
to attribute ordering effect.

The G5A causal gate compares only A1/A2/A3.

---

## 6. Hard implementation invariants

Every measured file must satisfy all of the following before any size result is used.

### I1 — exact source roundtrip

Each A1/A2/A3 body plus its charged order mode must decode byte-for-byte to the
original source.

### I2 — body-size identity

```
len(A1_body) == len(A2_body) == len(A3_body)
```

### I3 — exact permutation coverage

The implementation must construct a canonical list of structured scalar chunk
identities and prove that each arm's token-stream order is a permutation containing
every canonical chunk index exactly once.

No duplicate, omission, or synthetic chunk is allowed.

### I4 — envelope identity

Before the structured token stream, the carrier-body prefix must be byte-identical
across A1/A2/A3. The raw-residual section must also be byte-identical.

### I5 — fixed backend

All three bodies use the same Brotli quality/window settings as frozen G3:

- quality 11;
- lgwin 30 where supported by the existing G3 build contract.

### I6 — no typed leaves

No G2/G3 typed leaf encoder is allowed in A1/A2/A3. The exact lexical scalar bytes
are the payload.

### I7 — complete accounting

The charged one-byte mode field is included in every reported G5A complete-byte
number.

Any invariant failure -> **INVALID-G5A**, fix correctness, rerun from a newly frozen
implementation. No size interpretation is permitted.

---

## 7. Frozen corpus roles

### 7.1 Discovery / attribution: D1-D4

Reuse the exact immutable D1-D4 objects already frozen and repeatedly identity-verified
through G1-G4:

| ID | Object | Bytes | SHA-256 |
|---|---|---:|---|
| D1 | Amazon cellphone NDJSON | 277,673 | `c1518fdaaed45e590c480ed707aa1adaaba8b84b10747f956bd431c708bd590e` |
| D2 | CDISC ADaM adverse-event NDJSON | 615,350 | `b795a59c5c92a8fc93d7d3f729fa0dae014e9b0f684a6a7a7bcf07e4104ba723` |
| D3 | GH Archive 10 MiB NDJSON | 10,485,760 | `a860af236f794779b5471b416b2e6d7ea68de00f6d17728dd3b088a7ecec8252` |
| D4 | CROVIA royalty receipts NDJSON | 3,585,053 | `0db7ace9e46ce458a7055f48b09aabc7df15f4f74a23553d3660b7de61d0916f` |

These are no longer unseen. G5A makes no generalization claim from them.

### 7.2 Known stress/anatomy object: V1

Sino-US DrugQA V1 is now **known**, because G3 legitimately opened it.

Frozen identity:

- bytes: **15,374,047**
- SHA-256: `757b9bc7e5ee2d38ab5ed43877b1d87809cb5131621d106671bc27996da1c499`

G5A may measure the exact same frozen A1/A2/A3 arms on V1 because the purpose is
failure anatomy, not validation.

V1 must be labeled:

> **KNOWN-STRESS / NOT HELD-OUT FOR G5A**

Its result cannot satisfy any generalization or promotion gate.

No new held-out corpus is opened in G5A.

---

## 8. Frozen metrics

For each file and each arm report:

- source bytes;
- carrier-body bytes;
- complete compressed bytes;
- exact roundtrip;
- token chunk count;
- structured token bytes;
- structured frame count;
- raw frame count;
- shape count;
- Brotli encode time as diagnostic only;
- carrier construction time as diagnostic only.

Derived deterministic byte effects:

```
shape_grouping_bytes = A1_complete - A2_complete
column_increment_bytes = A2_complete - A3_complete
total_order_bytes = A1_complete - A3_complete

shape_grouping_pct = (A2_complete / A1_complete - 1) * 100
column_increment_pct = (A3_complete / A2_complete - 1) * 100
total_order_pct = (A3_complete / A1_complete - 1) * 100
```

Negative percentages mean the later ordering is smaller.

Because Brotli output bytes are deterministic under the frozen build, G5A does not
use statistical significance language for byte differences. Timing is not a G5A
promotion axis.

---

## 9. Aggregate attribution

For D1-D4 define sums over complete bytes:

```
S1 = sum(A1_complete)
S2 = sum(A2_complete)
S3 = sum(A3_complete)

shape_bytes  = S1 - S2
column_bytes = S2 - S3
total_bytes  = S1 - S3
```

The decomposition is exact:

```
shape_bytes + column_bytes == total_bytes
```

If `total_bytes > 0`, component shares are reported:

```
shape_share  = shape_bytes  / total_bytes
column_share = column_bytes / total_bytes
```

Shares may be outside [0,1] if one component is adverse and the other more than
compensates. That is valid anatomy and must not be clipped.

---

## 10. Frozen classification

Correctness invariants dominate every classification.

### ORDER-ADVERSE

If `S3 > S1`, shape-column ordering is worse than source ordering in aggregate.

### ORDER-NEUTRAL

If `S3 == S1`, the aggregate byte effect of shape-column versus source ordering is exactly zero.

### ORDER-WEAK

If A3 is smaller than A1 but aggregate improvement is less than **1.0%**:

```
S3 < S1 && S3 / S1 > 0.99
```

The effect is real but below the frozen engineering-materiality threshold.

### ORDER-CONCENTRATED

If aggregate improvement is at least **1.0%** but A3 is smaller than A1 on fewer than
**2 of 4** D1-D4 families, classify the result as `ORDER-CONCENTRATED` rather than
calling the ordering mechanism broadly material on the frozen discovery set.

This closes the classification surface without weakening the two-family requirement.

### ORDER-MATERIAL

Require all:

1. all I1-I7 invariants pass;
2. `S3 / S1 <= 0.99` — at least **1.0% aggregate** improvement;
3. A3 is smaller than A1 on at least **2 of 4** D1-D4 families.

Only then is ordering called materially causal on the frozen discovery set.

### Attribution label inside ORDER-MATERIAL

Using signed aggregate byte components:

- **COLUMN-DOMINANT** if `column_bytes >= 0.60 * total_bytes`;
- **SHAPE-DOMINANT** if `shape_bytes >= 0.60 * total_bytes`;
- otherwise **MIXED-ORDERING**.

If one component is negative, the positive component may exceed 100% of the net
saving; report the signed shares exactly.

These labels describe D1-D4 anatomy only. They do not authorize production.

---

## 11. V1 diagnostic classification

V1 is reported separately.

- `V1-COLUMN-FAVORABLE` if A3 < A1;
- `V1-COLUMN-NEUTRAL` if A3 == A1;
- `V1-COLUMN-ADVERSE` if A3 > A1.

Also report A1->A2 and A2->A3 byte decomposition.

This diagnostic is intended to answer whether the G3 held-out failure is compatible
with an ordering/locality failure even when parsing coverage is 100%.

It is not a held-out test.

---

## 12. What G5A may conclude

G5A may conclude only:

- whether exact byte ordering is materially causal on D1-D4;
- whether shape grouping or column grouping contributes more to that effect;
- whether known V1 reacts favorably or adversely to the same frozen permutations.

G5A may **not** conclude:

- that a production planner exists;
- that the current representation generalizes broadly;
- that shape hierarchy is beneficial;
- that any new leaf family should be added;
- that G3 is production-ready;
- that V1 validates a G5A hypothesis.

---

## 13. What G5A does not contain

Explicitly excluded:

- S0/S1/S2 planner rescue;
- learned scoring;
- new q11 leaf search;
- dictionary leaves;
- integer leaves;
- float leaves;
- RLE/default-exception;
- FSST/string symbols;
- cross-column predictors;
- schema union or hierarchy;
- SIMD optimization;
- production wire IDs.

Those are separate future lanes.

---

## 14. Execution discipline

1. commit this preregistration before G5A source exists;
2. implement a standalone G5A prototype;
3. local workstation work is limited to compile + tiny selftests;
4. do **not** measure D1-D4 or V1 on the workstation or homelab;
5. freeze the implementation source SHA;
6. create a GitHub Actions workflow pinned to that exact implementation;
7. verify frozen G3 source identity;
8. fetch and verify corpus identities remotely;
9. run D1-D4 attribution remotely;
10. run V1 as known-stress anatomy remotely;
11. apply the frozen classification mechanically;
12. upload complete evidence;
13. write source-of-record results;
14. only then proceed to G5B.

No post-outcome threshold or arm changes are allowed.

---

## 15. Relationship to G5B

G5B remains a separate experiment:

> exact flat shapes versus explicitly preregistered structural hierarchy.

G5A must close first. G5B may use the G5A result as motivation and architecture
context, but it may not retroactively change G5A arms or gates.

---

## 16. Production disposition

Regardless of whether G5A is ORDER-ADVERSE, ORDER-NEUTRAL, ORDER-WEAK, ORDER-CONCENTRATED, or ORDER-MATERIAL:

- no production transform ID is allocated;
- no production ANVIL source is changed by G5A;
- no planner is promoted;
- raw Brotli remains the permanent fallback principle.

G5A exists to learn what the representation is actually doing before ANVIL spends
more complexity on it.
