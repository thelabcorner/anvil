# ANVIL I10 - GROTLI G5A Ordering Attribution Preregistration

**Status:** FROZEN r6 BEFORE ANY G5A D1-D4 / V1 CORPUS MEASUREMENT  
**Freeze note:** r6 is the final pre-corpus contract and supersedes r5. No D1-D4 or V1 G5A result existed when r6 was frozen. r6 makes contract/provenance hardening corrections only, EXCEPT the classification-precedence serialization (null-control placement, section 10), which is frozen NOW before the first corpus run. r6 corrections: (A) explicit null-first precedence when the ordering direction is favorable; (B) honest reconciliation of the pre-freeze tiny synthetic fixtures; (C) exact implementation carrier grammar replacing the conceptual rendering; (D) fail-closed V1 re-gating in the final ruling; (E) exact linked libbrotli library hashes as additive provenance; (F) strengthened I2 (total body length + envelope length + token-region length identities). None of these changes the arms, the thresholds, the corpus, or the scientific question.  
**Supersedes:** r5. Earlier revisions and their freeze notes are historical.  
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

The four G5A structured arms must differ in exactly one mechanism:

> **the permutation of the same structured scalar chunks.**

For every input, all four arms must have:

- identical parser and frame split;
- identical structured/raw frame classification;
- identical exact-shape identity;
- identical shape templates;
- identical frame-to-group reconstruction map;
- identical raw residual bytes in identical order;
- identical structured scalar token bytes;
- identical scalar token length framing;
- identical structured scalar chunk multiset;
- identical canonical token-multiset SHA-256 (machine-gated, section 6 I3);
- identical common-envelope SHA-256 (machine-gated, section 6 I4);
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

### 2.1 Frozen-inclusion requirement (audit blocker 1)

G5A must compile against a **materialized pinned frozen G3 source**, not the
mutable working-tree `tools/grotli_g3.cpp`.

The CI workflow must, before building G5A:

1. materialize `frozen-grotli_g3.cpp` from `$FROZEN_G3_SHA` and verify its git blob;
2. build G5A in a way that includes that **exact materialized file** (e.g. compile
   with `-DG5A_FROZEN_G3_HEADER=\"frozen-grotli_g3.cpp\"` and have the source
   `#include G5A_FROZEN_G3_HEADER`);
3. additionally assert that the materialized blob equals the blob of the G3 source
   the G5A source would otherwise include, so the two cannot silently diverge.

A separate frozen-G3 reference binary is built as well, but **it is not a
substitute** for G5A itself compiling against the pinned source.

The local build (no CI) may include the working-tree `tools/grotli_g3.cpp`; the
requirement is that the CI path provably compiles the pinned bytes.

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

The carrier body is deliberately independent of order mode. The following is the
EXACT implementation grammar (matching `tools/grotli_g5a_ordering.cpp`), not a
conceptual sketch. All lengths/counts are `uvar` = unsigned LEB128 (7 data bits
per byte, high bit = continuation, canonical shortest form).

```
BODY = ENVELOPE || TOKEN_STREAM            # byte concatenation
```

ENVELOPE (byte-identical across all four arms; equals body[0, prefix_len)):

```
magic              : 4 bytes = 'G','5','A','O'
version            : u8 = 1
source_len         : uvar   # original input length in bytes
frame_count        : uvar
 group_count        : uvar
frame_group[frame_count] : uvar each    # group id per frame, in source order

group_descriptors[group_count] (in group-id order):
    group_kind     : u8     # 1 = raw residual group, 0 = structured shape
    member_count   : uvar
    (structured only):
        slot_count : uvar
        template_part_count : uvar
        for each template part:
            part_len : uvar
            part_bytes : part_len raw bytes

raw_residual_section (only if a raw group exists), in frozen source order:
    for each raw member:
        raw_len  : uvar
        raw_bytes: raw_len raw bytes
```

TOKEN_STREAM (the ONLY region that changes between arms; length = token_region_len):

```
structured_token_stream: exactly N chunks, each:
    token_len  : uvar   # length framing of one exact lexical scalar token
    token_bytes: token_len raw bytes
```

A single scalar token is stored as its length framing followed by its exact
bytes; the multiset of `(token_len, token_bytes)` records is identical across
arms and only their ORDER differs. The envelope prefix and the raw-residual
section are byte-identical across all arms.

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

The one charged byte is equal-cost across all four arms. Brotli sees no arm tag.

### 3.2 Mode byte is out-of-band, and materialized in a pack/unpack selftest (audit blocker 5)

G5A is an **anatomy experiment**, not a wire format. The one-byte order-mode field is
explicitly **out-of-band**:

- it is **not** part of the Brotli-compressed body;
- G5A makes **no wire-format claim** and allocates **no production transform ID**;
- to keep the accounting honest, the source MUST materialize the mode byte in a
tiny deterministic pack/unpack round-trip selftest: pack `(mode_byte, body)` and
  unpack it, verifying the recovered mode selects the correct decoder permutation.

This makes the charged byte concrete without weakening the ordering-only comparison.

---

## 4. Frozen arms

Exactly four structured order arms exist.

### A1 - SOURCE_ORDER

Structured scalar chunks appear in original structured-frame order.

For each source frame:

- raw frame: contributes no structured chunk;
- structured frame: emit slot 0, slot 1, ... slot K-1 for that record.

This preserves source-local row adjacency among scalar values while still using the
same shape/template metadata as every other arm.

### A2 - SHAPE_ROW

Group structured records by exact shape in frozen first-appearance shape order.

Within each shape:

- emit members in source order;
- for each member emit slot 0 ... slot K-1.

This isolates the effect of **shape grouping** while preserving row-major locality
inside each shape.

### A3 - SHAPE_COLUMN

Group structured records by exact shape in frozen first-appearance shape order.

Within each shape:

- emit slot 0 across all members;
- then slot 1 across all members;
- ...
- then slot K-1 across all members.

This is the pure corresponding-placeholder / column-local ordering arm.

### A0 - RANDOM_PERMUTATION (deterministic null; audit blocker 3)

A deterministic pseudo-random permutation of the **same** canonical chunk list:

- the permutation is derived solely from a fixed frozen seed and the canonical
  chunk index; no score, no input content, and no observed byte ever influences it;
- the concrete rule: sort canonical indices by
  `SHA-256(canonical_bytes(seed_string) || 0x1F || index_le_u64)` compared as an
  unsigned big-endian 256-bit integer, ties by lower index;
- the seed string is frozen as the ASCII literal `G5A-RANDOM-PERMUTATION-SEED-v1`;
- A0 uses the same envelope, the same chunk multiset, the same body length, and the
  same backend as A1/A2/A3.

A0's scientific role is a **null**: it destroys structural locality while preserving
everything else, so A3's effect can be separated from "Brotli happens to like some
permutation".

### 4.1 A0 is a single deterministic null draw, not a p-value

Brotli's output is order-sensitive and non-monotone, so ONE random permutation is a
single point in the null distribution, not a test statistic. Therefore:

- A0 is frozen as exactly one deterministic draw (one seed);
- G5A must **not** use p-value, significance, or "random is worse on average"
  language from a single draw;
- A3's locality claim is only supported when A3 also beats A0 on the frozen
  comparison (section 10.1); otherwise the claim is unsupported even if A3 beats A1;
- adding more random draws is a **separate** future lane and may not be added after
  seeing D1-D4.

There are no other G5A arms.

---

## 5. Context arms that do not enter the causal gate

The CI result may report, as clearly labeled context:

- raw Brotli of the original source;
- frozen G3 `REGION_RAW` complete bytes.

Those objects do **not** share the G5A common envelope and therefore cannot be used
to attribute ordering effect.

The G5A causal gate compares only A1/A2/A3, with A0 as the frozen null reference
(section 10.1). Context objects (raw Brotli, frozen G3 `REGION_RAW`) are never
compared to A1/A2/A3 for attribution.

---

## 6. Hard implementation invariants

Every measured file must satisfy all of the following before any size result is used.

### I1 - exact source roundtrip

Each A1/A2/A3 body plus its charged order mode must decode byte-for-byte to the
original source.

### I2 - body-size identity

```
len(A0_body) == len(A1_body) == len(A2_body) == len(A3_body)
```

r6 STRENGTHENING: in addition to total body length, the implementation MUST
expose, gate, and emit the ENVELOPE length and the TOKEN-REGION length identities
across all four arms:

```
envelope_len       = prefix_len        (identical across A0/A1/A2/A3)
token_region_len   = body_len - prefix_len   (identical across A0/A1/A2/A3)
```

These are pure derivations of the existing exact prefix/body construction and MUST
NOT change any body byte. They are emitted as `envelope_len_identity`,
`token_region_len_identity`, `envelope_len`, `token_region_len` and gated in both
the source invariants and the CI row gates.

### I3 - exact permutation coverage AND canonical token-multiset identity

The implementation must construct a canonical list of structured scalar chunk
identities and prove that each arm's token-stream order is a permutation containing
every canonical chunk index exactly once. No duplicate, omission, or synthetic chunk
is allowed.

Index bijection alone is tautological and is **not** sufficient. Additionally, for
every arm, the implementation MUST compute and emit a byte-level **canonical
token-multiset SHA-256** defined as:

```
record_i = uvar(len(token_bytes_i)) || token_bytes_i     (decode-visible framing)
multiset_sha256 = SHA-256( sort_lexicographically_ascending(record_1, ..., record_N)
                           joined with no separator )
```

The multiset SHA-256 must be **identical across A0/A1/A2/A3** for every measured
file. Any difference is an INVALID-G5A implementation defect.

### I4 - envelope identity (byte-level)

Before the structured token stream, the carrier-body prefix must be byte-identical
across A0/A1/A2/A3, and the raw-residual section must be byte-identical. This is
machine-gated by a byte-level **envelope SHA-256**:

```
envelope_bytes = carrier_body[0 .. prefix_len)
envelope_sha256 = SHA-256(envelope_bytes)
```

The envelope SHA-256 must be identical across A0/A1/A2/A3 for every measured file,
and must equal the SHA-256 of the stored plan prefix. Any difference is INVALID-G5A.

Additionally the implementation must prove that each arm's body equals the common
envelope followed immediately by the arm's token-permutation region (no bytes are
inserted, removed, or reordered outside the token region).

### I5 - fixed backend

All four bodies use the same Brotli quality/window settings as frozen G3:

- quality 11;
- lgwin 30 where supported by the existing G3 build contract.

### I6 - no typed leaves

No G2/G3 typed leaf encoder is allowed in A0/A1/A2/A3. The exact lexical scalar bytes
are the payload.

### I7 - complete accounting

The charged one-byte mode field is included in every reported G5A complete-byte
number, and is materialized in the pack/unpack selftest of section 3.2.

### I8 - backend implementation identity (audit blocker 4)

All arms must use the same Brotli implementation **build**, not merely the same
quality/window parameters. Each result row MUST record:

- `BrotliEncoderVersion()` (the linked library version integer and its dotted form);
- the Brotli library identity as reported by the CI build (package version);
- quality 11 and lgwin 30.

r6 ADDITIVE PROVENANCE (E): the CI workflow MUST additionally capture and upload
the exact SHA-256 of the actually linked libbrotli shared libraries, resolved at
runtime from the built binary (e.g. via `ldd`/`readlink -f` to the real `.so`
paths, then `sha256sum` each), together with the compiler identity and the
distribution package versions. These are additive provenance fields; they do not
change any arm, threshold, or measurement.

The CI workflow must record the Brotli library package identity it linked against.

### I9 - A0 null arm is required

A measurement that omits A0, or whose A0 does not satisfy I1-I8, is INVALID-G5A.

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

### 7.1a Pre-freeze tiny synthetic fixtures are inadmissible evidence (r6)

During development, hand-written correctness fixtures existed under the gitignored
`scratch/g4-r5-local/` directory (the largest about 305 bytes) and were used only
for local compile/selftest and candidate-revision smoke checks before any corpus
run. They are explicitly:

- **NOT** D1-D4 or V1, and not drawn from any frozen corpus;
- of **no attribution value** and **inadmissible as G5A evidence**;
- superseded by this frozen contract;
- never used to set, tune, or justify any arm, threshold, or gate.

Only CI-produced rows measured on the frozen D1-D4 objects under the frozen
implementation identity count as G5A evidence.

### 7.2 Known stress/anatomy object: V1

Sino-US DrugQA V1 is now **known**, because G3 legitimately opened it.

Frozen identity:

- bytes: **15,374,047**
- SHA-256: `757b9bc7e5ee2d38ab5ed43877b1d87809cb5131621d106671bc27996da1c499`

G5A may measure the exact same frozen A1/A2/A3 arms on V1 because the purpose is
failure anatomy, not validation.

### 7.2a Fail-closed V1 re-gating in the final ruling (r6)

The final ruling MUST re-load and re-gate the V1 row's correctness invariants
inside the ruling heredoc itself (not only inside the measurement step). If any V1
invariant fails - body-size identity, envelope-length identity, token-region-length
identity, envelope identity, canonical multiset identity, exact permutations, mode
pack/unpack, roundtrip, backend identity - the run MUST fail closed and MUST NOT
emit any `V1-COLUMN-*` label. The permitted outcomes are then either an abort or an
explicit `V1-INVALID-G5A` marker; a V1 correctness failure may never be silently
reported as a favorable ordering diagnostic.

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
- carrier construction time as diagnostic only;
- `canonical_token_multiset_sha256` (identical across all arms);
- `envelope_sha256` (identical across all arms);
- `brotli_encoder_version` (integer and dotted form; identical across all arms).

Derived deterministic byte effects (with A0 as the frozen null; A0 complete bytes
reported for every file):

```
shape_grouping_bytes = A1_complete - A2_complete
column_increment_bytes = A2_complete - A3_complete
total_order_bytes = A1_complete - A3_complete
random_null_bytes = A0_complete - A3_complete   # > 0 means A3 beats the null

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
No favorable claim is made, so no null test is applied.

### ORDER-NEUTRAL

If `S3 == S1`, the aggregate byte effect of shape-column versus source ordering is exactly zero.
No favorable claim is made, so no null test is applied.

### ORDER-UNSUPPORTED-BY-NULL

If `S3 < S1` (a favorable ordering direction) but `S3 >= S0`, the shape-column
ordering did NOT beat the deterministic structure-destroying null draw. Any
A1-vs-A3 improvement is then not attributable to the specific locality mechanism.
The null test is applied BEFORE the strength gates below, so an otherwise-MATERIAL
result cannot be claimed when it fails the null.

### ORDER-WEAK

Favorable direction and null passed (`S3 < S1` and `S3 < S0`) but aggregate
improvement is less than **1.0%**:

```
S3 < S1 && S3 < S0 && S3 / S1 > 0.99
```

The effect is real but below the frozen engineering-materiality threshold.

### ORDER-CONCENTRATED

Favorable direction, null passed, aggregate improvement at least **1.0%**, but A3
is smaller than A1 on fewer than **2 of 4** D1-D4 families:

```
S3 < S1 && S3 < S0 && S3 / S1 <= 0.99 && a3_smaller_files < 2
```

This closes the classification surface without weakening the two-family requirement.

### ORDER-MATERIAL

Require all:

1. all I1-I9 invariants pass;
2. `S3 / S1 <= 0.99` - at least **1.0% aggregate** improvement;
3. `S3 < S0` - A3 beats the single frozen null draw in aggregate;
4. A3 is smaller than A1 on at least **2 of 4** D1-D4 families.

Only then is ordering called materially causal on the frozen discovery set.

### Frozen classification precedence (r6)

Evaluated in this exact order; the first matching rule wins:

```
INVALID               if invariants/decomposition fail
ADVERSE               if A3 > A1
NEUTRAL               if A3 == A1
UNSUPPORTED-BY-NULL   if A3 < A1 and A3 >= A0
WEAK                  if A3 < A1 and A3 < A0 and A3 / A1 >  0.99
CONCENTRATED          if A3 < A1 and A3 < A0 and A3 / A1 <= 0.99 and a3_smaller_files <  2
MATERIAL              if A3 < A1 and A3 < A0 and A3 / A1 <= 0.99 and a3_smaller_files >= 2
```

ADVERSE and NEUTRAL are checked before the null because they make no favorable
claim. The single deterministic null is NOT a statistical significance test.

### Attribution label inside ORDER-MATERIAL

Using signed aggregate byte components:

- **COLUMN-DOMINANT** if `column_bytes >= 0.60 * total_bytes`;
- **SHAPE-DOMINANT** if `shape_bytes >= 0.60 * total_bytes`;
- otherwise **MIXED-ORDERING**.

If one component is negative, the positive component may exceed 100% of the net
saving; report the signed shares exactly.

These labels describe D1-D4 anatomy only. They do not authorize production.

### 10.1 Null-control requirement (audit blocker 3)

Let `S0 = sum(A0_complete)` over D1-D4 (the deterministic random-permutation null).

The null test is applied to any FAVORABLE ordning direction BEFORE the strength
gates (see the frozen precedence in section 10):

```
S3 < S0        # A3 beats the single frozen null draw in aggregate
```

If the direction is favorable (`A3 < A1`) but `A3 >= A0`, the classification is
**ORDER-UNSUPPORTED-BY-NULL**: the shape-column ordering did not beat a
structure-destroying permutation of the same chunks, so any A1-vs-A3 improvement
is not attributable to the specific locality mechanism. A0 comparison uses ONE
deterministic draw and MUST NOT be reported as a significance test; no p-value,
no "random is worse on average" language is permitted.

Report `random_null_bytes = S0 - S3` in the ruling.

---

## 11. V1 diagnostic classification

V1 is reported separately.

- `V1-COLUMN-FAVORABLE` if A3 < A1;
- `V1-COLUMN-NEUTRAL` if A3 == A1;
- `V1-COLUMN-ADVERSE` if A3 > A1.

Also report A1->A2 and A2->A3 byte decomposition.

FAIL-CLOSED (r6, section 7.2a): these labels are emitted ONLY if every V1 row
invariant re-checks clean inside the final ruling heredoc. If any V1 invariant
fails, the run fails closed and no `V1-COLUMN-*` label is produced.

This diagnostic is intended to answer whether the G3 held-out failure is compatible
with an ordering/locality failure even when parsing coverage is 100%.

It is not a held-out test.

---

## 12. What G5A may conclude

G5A may conclude only:

- whether exact byte ordering is materially causal on D1-D4;
- whether shape grouping or column grouping contributes more to that effect;
- whether A3 beats a deterministic structure-destroying null permutation;
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

1. commit this final r4 preregistration before any D1-D4 or V1 corpus measurement;
   earlier r2/r3 source and tiny correctness fixtures are superseded and are not evidence;
2. freeze an r4 implementation source commit that conforms exactly to this contract and
   compiles against the pinned frozen G3 source via section 2.1;
3. local workstation work is limited to compile + tiny synthetic correctness selftests;
4. do **not** measure D1-D4 or V1 on the workstation or homelab;
5. freeze the implementation source SHA;
6. create a GitHub Actions workflow pinned to that exact implementation and that
   materializes and verifies the pinned frozen G3 source before building G5A;
7. verify frozen G3 source identity and record Brotli build identity;
8. fetch and verify corpus identities remotely;
9. run D1-D4 attribution remotely (A0/A1/A2/A3);
10. run V1 as known-stress anatomy remotely;
11. apply the frozen classification mechanically (including the A0 null gate);
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

Regardless of whether G5A is ORDER-ADVERSE, ORDER-NEUTRAL, ORDER-WEAK, ORDER-CONCENTRATED, ORDER-UNSUPPORTED-BY-NULL, or ORDER-MATERIAL:

- no production transform ID is allocated;
- no production ANVIL source is changed by G5A;
- no planner is promoted;
- raw Brotli remains the permanent fallback principle.

G5A exists to learn what the representation is actually doing before ANVIL spends
more complexity on it.
