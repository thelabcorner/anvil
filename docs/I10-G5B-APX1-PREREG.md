# ANVIL I10 — G5B APX-1 One-Level Exact Template Patch Preregistration

**Status:** FROZEN BEFORE ANY APX-1 GATE 0 OR GATE 1 D1-D4 / V1 MEASUREMENT  
**Freeze date:** 2026-09-24  
**Scope:** separately frozen Gate 0 anatomy / Gate 1 discovery pilot only  
**Parent evidence:** G3 `PASS-G3-NARROW`; G4 `NO-GO-G4`; corrected G5B ordinal result `ORDINAL-ADVERSE` with its floor verified  
**Production authorization:** none  
**Implementation status:** APX-1 source and workflow do not exist as of this freeze

This document is a new, separate preregistration. It does not amend, reinterpret, or rerun G3, G4, G5A, or the killed G5B ordinal proxy.

---

## 0. Question and decision boundary

APX-1 asks one bounded question:

> For frozen exact G3 shapes, is there enough byte-identical structural sharing to make a one-level parent-template plus sparse structural patch representation smaller than the exact flat-shape representation, after every model byte is charged, while preserving the same parser, exact lexical values, raw residuals, backend, and reconstruction order controls?

APX-1 is an **exact byte-grammar pilot**, not a semantic schema claim.

It tests:

- one parent archetype per connected component;
- one-level child patches only;
- exact parent byte ranges plus literal replacements;
- cross-shape coordinate reuse;
- a compiled template arena;
- unchanged exact lexical scalar values.

It does not test:

- semantic key/type/arity hierarchy;
- nested or recursive shape grammars;
- reusable local fragments beyond one parent template;
- defaults or RLE for values;
- sparse value exceptions;
- new dictionary, integer, float, string, cross-column, or entropy leaves;
- learned production planning;
- a production wire format.

The corrected G5B ordinal result is used only to narrow the hypothesis. `ORDINAL-ADVERSE` kills ordinal proximity as a proxy for useful hierarchy. APX-1 is admissible only if its parent graph is selected from exact bytes and exact serialized MDL, never from shape ordinals.

---

## 1. Source lineage and frozen semantics

### 1.1 G3 semantics

APX-1 must compile against a materialized, identity-verified copy of frozen G3 source:

`1a3d18fed76adb6fb33264e1994f9c357306b3fa`

The mutable working-tree `tools/grotli_g3.cpp` is not admissible evidence.

Frozen G3 semantics include:

- framing only at LF, including LF in the frame;
- CRLF retained exactly;
- final remainder handled as one frame;
- per-frame exact JSON lexical parsing;
- malformed or incomplete frame becomes a raw residual;
- byte-identical inter-value parts define exact shape identity;
- shapes retain frozen first-appearance IDs for serialization identity;
- structured/raw classification and frame reconstruction order remain explicit.

The exact shape key remains conceptually:

```
uvar(part_count)
for each part:
    uvar(part_length)
    part_bytes
```

APX-1 may encode an exact template more compactly, but the materialized child template must reproduce the same byte sequence and exact shape key.

### 1.2 G5A control lineage

The current frozen G5A source lineage is:

- repository commit: `b8eae11fa353bb5e3c88e8e757fe3ad41e4bbd24`
- `tools/grotli_g5a_ordering.cpp` blob: `5aa9dfe1593138b5e796afd7b627f1a8f8da8ddc`

Frozen G5A order semantics and invariants are controls, not mechanisms under test:

- source token order;
- shape-row token order;
- shape-column token order;
- deterministic random null remains lineage context only;
- canonical token multiset SHA-256;
- common raw residual section;
- charged one-byte order selector.

### 1.3 Future APX-1 implementation freeze

Before Gate 0 corpus access, the future standalone APX-1 implementation must be frozen at an exact commit and the CI workflow must verify:

- APX-1 source commit;
- materialized frozen G3 commit and blob;
- materialized G5A lineage commit/blob where G5A controls are rebuilt;
- compiler identity;
- Brotli library identity;
- corpus identities;
- no production `src/anvil.cpp` or ANVIL format dependency.

A source change after any D1-D4 or V1 outcome requires a new revision and invalidates the affected run as evidence for this frozen revision.

---

## 2. Non-negotiable causal isolation

For each input, all four primary arms must have identical:

- source bytes and source SHA-256;
- frame split and frame count;
- structured/raw classification;
- exact shape membership and occurrence assignment;
- exact materialized template bytes for every exact shape;
- exact scalar token bytes;
- scalar token length framing;
- canonical scalar token multiset;
- raw residual bytes and order;
- exact source reconstruction recipe semantics;
- Brotli implementation, quality, and window;
- no omitted, normalized, deduplicated, or transformed scalar token.

Primary arms:

1. `C0_FLAT_SRC` — normalized flat exact-shape carrier, source token order.
2. `C1_FLAT_COL` — normalized flat exact-shape carrier, shape-column token order.
3. `S0_APX_SRC` — APX-1 template patches and global coordinates, source token order.
4. `S1_APX_COL` — APX-1 template patches and global coordinates, coordinate-major token order.

`C0` and `C1` must be produced by the same arena-based carrier/decoder infrastructure as `S0` and `S1`. This prevents allocator or container-layout differences from being misreported as hierarchy gains.

Frozen G5A A1 and A3 remain external lineage controls. They do not replace the normalized controls because the live G5A decoder materializes per-token `Bytes` rows, while APX-1 requires a contiguous arena comparison.

No primary APX-1 arm may use a G3 typed leaf in this pilot. Gate 1 is lexical-only.

---

## 3. Exact APX-1 representation

### 3.1 Exact shape identity

An APX archetype or flat shape is identified internally by the exact SHA-256 of:

```
uvar(part_count)
for each exact inter-value part:
    uvar(part_length)
    part_bytes
```

The digest is encoder/evidence identity. It is not transmitted unless a future decoder needs it for reconstruction. An evidence-only digest is not a free archive byte.

Every child patch program must materialize byte-for-byte the original exact child parts. Failure to reproduce the exact shape key is invalid.

### 3.2 Parent patch program

Let parent archetype `p` have exact structural parts:

```
P[0], ..., P[p_parts-1]
```

Let child exact shape `c` have exact structural parts:

```
C[0], ..., C[c_parts-1]
```

A child is represented relative to one parent by one output part per exact child part. Each output part is a concatenation of at most four segments.

Segment kinds:

1. `PARENT_RANGE(parent_part, offset, length)`
2. `LITERAL(bytes)`

Frozen canonical segment construction:

1. Process each child part from left to right.
2. At the current child-part offset, search all parent parts for the longest exact byte match.
3. Minimum accepted match length is 4 bytes.
4. Emit preceding unmatched child bytes as one `LITERAL`.
5. Emit the selected match as one `PARENT_RANGE`.
6. Continue after the matched child range.
7. Parent ranges may be reused; the child ranges may not overlap.
8. Longest match wins.
9. Equal-length ties are resolved by:
   - lower parent part index;
   - then lower parent range offset.
10. If no match of length at least 4 exists, emit the remainder as one literal.
11. A child part is ineligible if its canonical program has more than four segments.

The parent-part index tie-break is a position inside one exact template. It is not a shape ordinal, source-frame ordinal, or shape-ID proximity rule.

### 3.3 Strong-copy eligibility

A child patch is eligible only if at least one copied range has:

- length at least 8 bytes; and
- at least one byte outside this structural/whitespace set:

```
space, tab, CR, LF, {, }, [, ], (, ), :, ,, ", backslash
```

Thus punctuation-only or whitespace-only overlap does not create a hierarchy edge. A key-bearing or otherwise lexical exact range can.

A valid patch program with no strong-copy range is not an APX edge. That shape remains flat or is evaluated as an explicit standalone alternative.

### 3.4 Component construction

Build an undirected graph whose vertices are exact shapes and whose edge `{p,c}` exists when:

- `p != c`;
- at least one direction has a canonical eligible child patch program; and
- strong-copy eligibility passes.

Graph components are encoder-only. Component membership is not transmitted because the decoder needs only chosen parent edges and exact patch programs.

### 3.5 Exact serialized MDL root selection

MDL in this preregistration means the **exact uncompressed APX-1 carrier model byte count under the frozen canonical encoding**. It is not an entropy estimate and may not use q11 outcomes.

For each connected component:

1. Evaluate every exact shape as the component's one required primary archetype.
2. The candidate root is serialized as a flat exact template.
3. Every other shape independently chooses the smaller exact cost of:
   - remaining a flat archetype; or
   - becoming a one-level child of the candidate root.
4. No chosen child may become a parent for another shape.
5. Choose the root minimizing the exact complete model bytes for that component.
6. If two roots tie exactly, choose the lexicographically smaller SHA-256 of the root exact shape key.
7. If flat and child descriptor costs tie exactly, choose flat.
8. Singleton components have their sole shape as root.
9. A source may contain multiple components; each component chooses one primary archetype independently.

This is deliberately bounded and is not a claim to find a globally optimal forest.

#### Flat structural descriptor cost

For an exact shape:

```
uvar(member_count)
uvar(part_count)
for each part:
    uvar(part_length)
    part_bytes
```

The eventual carrier's group kind, coordinate namespace, recipe, and common sections are added once under the complete accounting formula.

#### APX child descriptor cost

For a child of parent ID `parent`:

```
uvar(parent_archetype_id)
uvar(part_count)
for each child part:
    uvar(segment_count)
    for each segment:
        uvar(segment_tag)
        if PARENT_RANGE:
            uvar(parent_part_index)
            uvar(parent_offset)
            uvar(length)
        if LITERAL:
            uvar(literal_length)
            literal_bytes
uvar(slot_count)
for each child slot:
    uvar(coordinate_id)
```

`segment_tag` is one byte:

- `0 = PARENT_RANGE`
- `1 = LITERAL`

All varints are canonical unsigned LEB128 and use the same length calculation as frozen G3/G5A.

### 3.6 One-level invariant

A transmitted child descriptor may reference only:

- itself — forbidden;
- a flat archetype — required;
- another child — forbidden.

The decoder therefore has no recursive shape expansion.

---

## 4. Coordinate mapping

### 4.1 Lexical anchor

For a shape with scalar slot `j`, the exact anchor is the structural part immediately before that scalar:

```
anchor(shape, j) = shape.parts[j]
```

A parent slot anchor is usable only if:

1. it contains at least one `"` byte; and
2. it occurs exactly once among the slot anchors of the chosen parent archetype.

A child slot maps to a parent coordinate only when:

1. the child anchor exactly equals that usable parent anchor; and
2. the mapping is not ambiguous within the chosen parent.

Otherwise the child slot receives a new coordinate.

This is an exact lexical mapping. It is not semantic type inference. Duplicate or non-unique anchors conservatively receive new coordinates.

### 4.2 Coordinate allocation

Coordinates are allocated in two passes:

1. For every flat archetype, allocate coordinates for its slots in archetype serialization order and slot order.
2. For every child descriptor, map eligible slots to its parent's coordinates; allocate new coordinate IDs in child serialization order and slot order for all unmapped slots.

Coordinate IDs are serialization labels. Their numeric order may not influence:

- root selection;
- parent selection;
- strong-copy eligibility;
- coordinate mapping;
- whether an edge is retained.

### 4.3 Coordinate streams and source order

For every global coordinate, its value stream contains the exact scalar token bytes for every structured frame/slot mapped to that coordinate, in source-frame order.

A raw residual frame contributes no coordinate value.

Each value is framed exactly as:

```
uvar(token_length)
token_bytes
```

Empty token chunks are forbidden by the frozen scalar parser semantics.

Gate 0 must report:

- flat coordinate count;
- APX coordinate count;
- mapped child-slot count;
- new child-coordinate count;
- coordinate streams spanning more than one exact shape;
- values per coordinate;
- distinct exact shapes per coordinate;
- maximum coordinate span in source frames.

---

## 5. Arms and token ordering

### 5.1 Token identity

Use a coordinate equivalent to frozen G5A:

```
(shape_id, occurrence, slot)
```

Each identity resolves to one exact token byte string.

### 5.2 `C0_FLAT_SRC`

- Flat exact templates.
- Flat coordinate namespace is `(shape_id, slot)`.
- Structured token chunks appear in exact G5A source-frame order.
- Raw residual bytes are unchanged and ordered by raw member occurrence.

### 5.3 `C1_FLAT_COL`

Identical to `C0` except structured token chunks appear:

1. by flat shape ID;
2. then slot;
3. then occurrence.

This is the normalized G5A A3 column-order control.

### 5.4 `S0_APX_SRC`

- APX-1 template model and global coordinates.
- Structured token chunks appear in exactly the same source-frame order as `C0`.
- This arm isolates APX-1 model/coordinate changes from token permutation.

Its structured token bytes and order must match `C0` exactly.

### 5.5 `S1_APX_COL`

- Same APX-1 model and coordinate mapping as `S0`.
- Structured token chunks appear by global coordinate ID.
- Within one coordinate, values remain in source-frame order.
- Raw residual bytes remain unchanged.

Its structured token bytes and order must match `S0` exactly.

### 5.6 External G5A controls

Frozen G5A A1 and A3 may be reported as external lineage controls. They do not enter the primary Gate 0/1 causal gate because their decoder memory layout differs from the normalized arena controls.

### 5.7 Raw fallback

`RAW_BROTLI` remains mandatory context and permanent fallback. It is not a causal APX-1 control.

A final research portfolio may select the smallest of raw fallback, `C0`, `C1`, `S0`, and `S1`, with raw winning exact ties. Such arbitration does not turn a losing APX arm into scientific evidence.

---

## 6. Carrier and complete-byte accounting

### 6.1 Research outer envelope

To prevent an order tag from perturbing the Brotli model:

- one outer arm-selector byte identifies the arm/order mode;
- the byte is charged in every primary arm;
- the byte is not passed to Brotli;
- pack/unpack is materialized in a deterministic selftest;
- invalid selector bytes are rejected.

The compressed carrier contains:

```text
magic = "A1P0"
version = 1
source_len
frame_count
frame_recipe
archetype_and_child_descriptors
coordinate slot maps
raw residual section
structured token stream
```

The exact source reconstruction recipe is explicit. The decoder never parses JSON or rediscovers frame boundaries.

### 6.2 Complete-byte equation

For all primary arms:

```text
complete_bytes = 1 + brotli_q11_lw30(carrier).size()
```

The one-byte selector is equal cost in every arm.

### 6.3 Charged bytes

Charge every decoder-visible byte:

- selector byte;
- carrier magic and version;
- source length;
- frame count;
- frame recipe;
- raw/structured group kind;
- member and shape counts;
- exact flat archetype part lengths and bytes;
- parent archetype IDs;
- child part counts;
- segment tags, part indices, offsets, lengths, literal lengths, and literal bytes;
- slot-to-coordinate maps;
- token lengths and exact token bytes;
- raw residual lengths and bytes;
- all counts, IDs, bounds, and discriminators;
- complete Brotli payload.

Do not charge encoder-only diagnostics as if transmitted, but do not omit them from CI evidence.

The following are encoder/evidence-only unless a decoder needs them:

- connected components;
- candidate parent scores;
- MDL root scores;
- strong-copy diagnostics;
- exact shape SHA-256 digests;
- planning timings;
- rejected candidate descriptors.

If any diagnostic is serialized, it becomes a decoder-visible byte and must be charged.

---

## 7. Decoder arena contract

### 7.1 Required arena layout

The normalized flat controls and APX arms must use the same decoded arena strategy.

At minimum:

1. **Template arena** — contiguous exact structural-part bytes for every flat archetype or materialized child.
2. **Value arena** — contiguous exact scalar token bytes.
3. **Derived offset arrays** — template-part and token offsets/cursors built after parsing; not charged when derivable.
4. **Shape metadata** — member counts, slot counts, template-part ranges, and coordinate maps.
5. **Coordinate cursors** — one cursor per flat `(shape, slot)` or APX global coordinate.
6. **Recipe cursor** — one source-order frame cursor.
7. **Output buffer** — exact source length.

The decoder must not retain a `std::vector<Bytes>` per scalar token.

### 7.2 APX arena construction

APX patch descriptors are compiled once:

1. Validate every parent reference and byte range.
2. Materialize every distinct child template part exactly once into the template arena.
3. Verify the reconstructed exact shape key against encoder evidence.
4. Release or stop retaining patch-construction temporaries before source reconstruction when memory permits.
5. Reconstruct source frames by arena copies and coordinate cursor reads.

No structural patch is interpreted once per record.

### 7.3 Fair memory control

Report separately:

- compressed input bytes;
- decompressed carrier bytes;
- temporary patch-compilation peak;
- template arena bytes;
- value arena bytes;
- metadata/offset bytes;
- output bytes;
- total peak RSS;
- allocation count.

A lower APX RSS caused only by a different flat-control container layout is invalid evidence.

---

## 8. Exact correctness invariants

Every primary arm must satisfy all of the following.

### I1 — source identity

The decoded source SHA-256 equals the frozen source SHA-256.

### I2 — exact roundtrip

Decoded bytes equal the original source byte-for-byte, including LF, CRLF, whitespace, numeric spelling, escapes, duplicate keys, and final remainder bytes.

### I3 — exact shape materialization

For every exact shape:

- materialized template bytes equal the original exact inter-value parts;
- slot count equals part count minus one;
- exact shape key SHA-256 matches encoder evidence.

### I4 — token permutation coverage

For each matched order pair:

- `C0` and `S0` have identical structured token bytes and source order;
- `C1` has exactly the frozen G5A shape-column permutation;
- `S0` and `S1` have identical structured token bytes;
- every token identity occurs exactly once;
- no synthetic token is present.

### I5 — canonical token multiset

All four primary arms have the same canonical token-multiset SHA-256, defined as:

```text
record_i = uvar(len(token_bytes_i)) || token_bytes_i
multiset_sha256 = SHA256(
    lexicographically sorted record_1 ... record_N
    concatenated without separators
)
```

### I6 — raw residual identity

All arms have the same raw residual bytes, order, lengths, and SHA-256.

### I7 — body-size identity by order pair

```text
len(C0_body) == len(C1_body)
len(S0_body) == len(S1_body)
```

Cross-arm body sizes may differ because the APX model is the treatment.

### I8 — one-level graph validity

- no self-parent;
- no child parent;
- no recursive or forward parent reference;
- every parent ID is a transmitted flat archetype;
- all child patch materializations consume exactly the declared parent ranges.

### I9 — coordinate validity

- every child slot map entry is in range;
- every new coordinate is allocated exactly once;
- every declared coordinate receives exactly its predicted value count;
- raw frames contribute no coordinate value;
- coordinate order is used only when selected by the charged mode byte.

### I10 — complete consumption

- every frame recipe entry is consumed exactly once;
- every raw body is consumed exactly once;
- every token is consumed exactly once;
- no trailing carrier bytes remain;
- final output length equals `source_len`.

### I11 — backend identity

All primary arms use the same linked Brotli implementation, quality 11, and lgwin 30. Integer and dotted version identity are recorded.

### I12 — no production change

No ANVIL production source, transform registry, or wire format changes.

Any invariant failure yields `INVALID-G5B-APX1` and no size interpretation.

---

## 9. Malformed-input and adversarial contract

The decoder must validate before unsafe allocation, indexing, copying, or expansion.

Mandatory rejection classes:

- noncanonical or overflowing varint;
- invalid magic or version;
- invalid outer selector;
- source length above research safety bound;
- frame/member count mismatch;
- invalid group kind;
- self-parent;
- child used as parent;
- missing parent;
- parent part index out of range;
- parent offset/length overflow or out of range;
- zero or excessive segment count;
- literal length overflow;
- child part count mismatch;
- child slot count mismatch;
- coordinate ID out of range;
- duplicate coordinate allocation;
- missing coordinate consumption;
- unexpected or missing token;
- empty token;
- token length overflow;
- invalid raw residual length;
- incomplete raw residual section;
- count overflow;
- overlong expansion;
- trailing bytes;
- final source length mismatch;
- any source SHA-256 mismatch after exact reconstruction.

Mandatory correctness fixtures include:

1. all-valid LF NDJSON;
2. all-valid CRLF NDJSON;
3. valid records with a malformed middle frame;
4. incomplete final remainder;
5. blank-line raw residual;
6. all-raw input;
7. one structured shape only;
8. multiple exact shapes with no eligible copy;
9. near-identical shapes sharing a key-bearing range;
10. optional inserted field with new coordinate;
11. whitespace-only template difference with no strong copy;
12. duplicate-key anchors;
13. zero-scalar `{}` and `[]` shapes;
14. exact edge cases for match length 3, 4, 7, and 8;
15. child part at the four-segment boundary and above it;
16. maximum legal parent-range reuse;
17. truncated patch descriptor;
18. invalid parent graph;
19. invalid coordinate map;
20. malformed token and raw-residual spans;
21. noncanonical varints;
22. trailing bytes.

Fuzzing and all CPU-heavy fixture execution run remotely in CI. No local heavy work is part of this preregistration pass.

---

## 10. Corpus contract

### 10.1 D1-D4 discovery

Use the exact immutable objects already frozen for G3-G5A:

| ID | Object | Bytes | SHA-256 |
|---|---|---:|---|
| D1 | Amazon cellphone NDJSON | 277,673 | `c1518fdaaed45e590c480ed707aa1adaaba8b84b10747f956bd431c708bd590e` |
| D2 | CDISC ADaM adverse-event NDJSON | 615,350 | `b795a59c5c92a8fc93d7d3f729fa0dae014e9b0f684a6a7a7bcf07e4104ba723` |
| D3 | GH Archive frozen 10 MiB NDJSON | 10,485,760 | `a860af236f794779b5471b416b2e6d7ea68de00f6d17728dd3b088a7ecec8252` |
| D4 | CROVIA royalty receipts NDJSON | 3,585,053 | `0db7ace9e46ce458a7055f48b09aabc7df15f4f74a23553d3660b7de61d0916f` |

D3 is the primary causal treatment. D1, D2, and D4 are known-data regression/anatomy controls.

No corpus substitution, truncation, resampling, or post-outcome family selection is allowed.

### 10.2 V1 known stress

Use only after Gate 0 passes and the APX-1 implementation commit is frozen:

| ID | Object | Bytes | SHA-256 |
|---|---|---:|---|
| V1 | Sino-US DrugQA `all.jsonl` | 15,374,047 | `757b9bc7e5ee2d38ab5ed43877b1d87809cb5131621d106671bc27996da1c499` |

V1 is:

> **KNOWN-STRESS / NOT HELD-OUT FOR G5B-APX1**

V1 previously contained 11,444 fully structured frames and only three exact shapes, while the G3 structured carrier was adverse. Therefore:

- V1 cannot validate APX-1;
- V1 cannot be used to change parents, bounds, arms, or gates;
- V1 may be `APX1-INAPPLICABLE` if no strong-copy component exists;
- `APX1-INAPPLICABLE` on V1 is informative, not an implementation failure;
- if APX-1 is applicable, V1 is reported diagnostically only.

No new held-out corpus is opened in this pilot.

---

## 11. Required evidence fields

For every D1-D4 and applicable V1 file, emit machine-readable rows containing at least:

### Source and identity

- corpus ID;
- source bytes;
- source SHA-256;
- frozen G3 commit/blob;
- APX-1 implementation commit;
- compiler identity;
- Brotli encoder version integer and dotted form;
- quality and window;
- CI run ID and runner image identity.

### G3 anatomy

- frame count;
- structured frame count;
- raw frame count;
- structured source bytes;
- raw source bytes;
- exact shape count;
- singleton shape count;
- top-1/top-5/top-10/top-100 shape rows;
- exact template bytes;
- exact scalar token count and bytes.

### APX anatomy

- eligible graph edge count;
- component count and sizes;
- non-singleton component count;
- rows in eligible components;
- primary archetype count;
- flat fallback archetype count;
- child shape count;
- child rows;
- copied/literal segment counts;
- maximum segments per child part;
- strong-copy range count;
- copied structural bytes;
- literal patch bytes;
- flat structural model bytes;
- APX structural model bytes;
- structural MDL saving bytes and percent;
- coordinate count;
- mapped slots;
- new child coordinates;
- multi-shape coordinate count;
- coordinate occupancy histogram;
- exact MDL root-choice diagnostics.

### Carrier accounting

For every arm:

- selector bytes;
- magic/version bytes;
- source/frame/count bytes;
- recipe bytes;
- flat template-model bytes;
- parent/child/patch bytes;
- coordinate-map bytes;
- token-framing and token bytes;
- raw residual length and bytes;
- complete uncompressed carrier bytes;
- Brotli payload bytes;
- complete charged bytes;
- exact roundtrip;
- invariant hashes;
- peak RSS and arena byte breakdown.

Timings are diagnostic in the anatomy/discovery pilot. They do not replace later paired micro or Pareto gates.

---

## 12. Gate 0 — anatomy eligibility

### 12.1 Purpose

Gate 0 asks whether exact structural patching has enough serialized MDL headroom to justify q11 discovery.

Gate 0 must not inspect compressed outcomes for parent selection. It constructs exact carriers under the frozen MDL rule and reports uncompressed complete model bytes.

### 12.2 Required sequence

1. Materialize and verify frozen G3 and G5A lineage.
2. Build and verify the frozen APX-1 implementation.
3. Run remote correctness and malformed-input fixtures.
4. Verify `C0`, `C1`, `S0`, and `S1` exact roundtrip on D1-D4.
5. Build exact flat and APX carriers for D1-D4.
6. Emit complete model-byte and anatomy evidence.
7. Apply Gate 0 mechanically.
8. If Gate 0 fails, stop before q11 and emit `NO-GO-G0-APX1`.
9. If Gate 0 passes, freeze the implementation commit before Gate 1.

### 12.3 Gate 0 pass conditions

All correctness invariants must pass, and all of the following are required:

1. On D3, `S0_complete_model_bytes < C0_complete_model_bytes`.
2. `S0_complete_model_bytes < C0_complete_model_bytes` on at least two of D1-D4.
3. Aggregate D1-D4 complete-model saving is at least **0.10%**:

```text
1 - sum(S0_complete_model_bytes) / sum(C0_complete_model_bytes) >= 0.001
```

4. Aggregate structural-model saving is at least **1.0%**:

```text
1 - sum(APX_structural_model_bytes) / sum(flat_structural_model_bytes) >= 0.01
```

5. At least 25% of structured rows belong to non-singleton eligible components on at least two of D1-D4.
6. No file relies on child recursion, an uncharged descriptor, or an out-of-bound expansion.

If any condition fails:

> `NO-GO-G0-APX1`

No Gate 1 corpus compression is run, and no threshold or arm may be changed to rescue the pilot.

---

## 13. Gate 1 — lexical discovery

### 13.1 Purpose

Gate 1 measures whether the Gate 0 model headroom survives the frozen Brotli backend and the two causal order controls.

### 13.2 Required sequence

1. Require a valid `PASS-G0-APX1` artifact and frozen implementation commit.
2. Run D1-D4 `C0`, `C1`, `S0`, and `S1` at q11/lw30 in one pinned CI environment.
3. Verify all I1-I12 invariants per file/arm.
4. Emit complete bytes and model breakdowns.
5. Apply the hierarchy and hierarchy-plus-column classifications mechanically.
6. Only after the D1-D4 ruling is frozen, run V1 as known stress.
7. Upload all positive, negative, invalid, and inapplicable evidence.

### 13.3 Hierarchy classification

`PASS-HIERARCHY-APX1` requires:

1. D3 `S0_complete_bytes < C0_complete_bytes`.
2. `S0_complete_bytes < C0_complete_bytes` on at least two of D1-D4.
3. Aggregate hierarchy saving is at least **0.25%**:

```text
1 - sum(S0_complete_bytes) / sum(C0_complete_bytes) >= 0.0025
```

4. All correctness and accounting invariants pass.

This is the primary APX-1 test. `S0` uses the same token order as `C0`, so it isolates hierarchy model and coordinate metadata from permutation.

### 13.4 Hierarchy-plus-column classification

`PASS-HIERARCHY-COLUMN-APX1` additionally requires:

1. `S1_complete_bytes < S0_complete_bytes` in aggregate.
2. `S1_complete_bytes < C1_complete_bytes` on at least two of D1-D4.
3. Aggregate column-order saving is at least **0.50%**:

```text
1 - sum(S1_complete_bytes) / sum(C1_complete_bytes) >= 0.005
```

4. All correctness and accounting invariants pass.

This label describes APX-1 plus coordinate-major order. It does not retroactively change G5A.

### 13.5 Failure labels

- `INVALID-G5B-APX1`: any correctness, identity, accounting, source-pin, or invariant failure. No size interpretation.
- `NO-GO-G0-APX1`: exact anatomy/MDL headroom absent. Stop before q11.
- `NO-GO-HIERARCHY-APX1`: Gate 0 passed, but `S0` did not beat `C0` under the frozen hierarchy gate.
- `HIERARCHY-ONLY-APX1`: hierarchy passed, hierarchy-plus-column failed.
- `PASS-HIERARCHY-COLUMN-APX1`: both gates passed.

Raw fallback selection or V1 results cannot upgrade a failed hierarchy label.

---

## 14. Kill thresholds and scope boundary

APX-1 is killed for this frozen revision if any of the following occurs:

1. Gate 0 complete-model, structural-model, D3, breadth, or eligible-row threshold fails.
2. `S0` fails the primary hierarchy gate against `C0`.
3. A gain exists only in `S1` while `S0` is neutral/adverse; that is ordering evidence, not APX-1 hierarchy evidence.
4. The result depends on ordinal shape adjacency, first-appearance rank, shape-ID distance, or source-position proximity.
5. The result requires child recursion, more than four segments per child part, or removal of a semantic strong-copy requirement.
6. The result disappears under exact MDL model accounting.
7. Correct carriers require a different flat-control allocator or arena to create the reported win.
8. Any decoder-visible byte is omitted from complete accounting.
9. D1-D4 or V1 corpus identity is substituted after outcomes are known.
10. A JSON-only result is described as general-purpose validation.

The following do **not** kill APX-1:

- V1 being adverse;
- V1 being `APX1-INAPPLICABLE` because it has too few exact shapes;
- hierarchy passing while coordinate-column order fails;
- a future production planner being absent from this research pilot.

---

## 15. Why APX-1 remains distinct from the killed ordinal proxy

The corrected `ORDINAL-ADVERSE` result rejects a tested ordinal proxy. APX-1 is a different hypothesis with a different admissible feature set.

### 15.1 Forbidden APX-1 selection features

APX-1 parent/root/edge selection must not use:

- absolute or relative shape-ID distance;
- first-appearance rank as a similarity score;
- source adjacency;
- frequency rank as a substitute for byte MDL;
- source-order position;
- an ordinal schema level;
- any “nearest shape ID” heuristic.

### 15.2 Required APX-1 evidence features

APX-1 may use only:

- exact structural-part bytes;
- exact `PARENT_RANGE` matches;
- literal replacement bytes;
- exact scalar-slot lexical anchors;
- exact serialized descriptor lengths;
- exact MDL carrier cost;
- exact source reconstruction requirements.

Shape IDs and coordinate IDs remain serialization labels. They are not similarity features.

### 15.3 Causal separation

`S0` and `C0` have the same source token order. Therefore a `S0 < C0` result cannot be explained by a different ordinal permutation: no token permutation changed.

`S1` is a separately labeled coordinate-order result. It cannot rescue or redefine a failed `S0` hierarchy result.

Gate 0 may report overlap with nearest-ordinal edges as a diagnostic, but such overlap has no vote, weighting, or gate role. The killed ordinal proxy is not rerun or combined with APX-1.

### 15.4 Scope of the corrected negative result

The corrected result kills:

> “Ordinal shape identity/proximity is sufficient evidence that hierarchy is beneficial.”

It does not kill:

> “Exact byte-identical parent ranges, fully charged sparse patches, and exact MDL may produce a different bounded hierarchy representation.”

APX-1 survives only if its byte-derived evidence passes Gates 0 and 1. If it fails, the wider hierarchy lane is not automatically refuted, but this one-level exact-patch mechanism is closed for the frozen revision.

---

## 16. Production and follow-up boundary

Regardless of Gate 0 or Gate 1 classification:

- no ANVIL production transform ID is allocated;
- no production source or format is changed;
- no production planner is claimed;
- raw Brotli remains the permanent fallback;
- D1-D4 are discovery, not held-out proof;
- V1 remains known stress;
- no generalization claim is authorized;
- no novelty claim is authorized for generic hierarchy, patching, schema union, or coordinate streams.

A later production proposal would require, at minimum:

1. a cheaper bounded parent-selection planner;
2. typed-leaf ablation under the same hierarchy;
3. paired remote micro measurements;
4. peak-RSS and decoder-code accounting;
5. frozen held-out corpora not used here;
6. prior-art review specific to any claimed mechanism-level novelty;
7. production integration only after the research mechanism is independently verified.

---

## 17. Frozen execution sequence

The admissible sequence is:

```text
G5B-APX1-R0  Freeze this preregistration and source lineage
G5B-APX1-R1  Remote compile + correctness/malformed fixtures
G5B-APX1-G0  D1-D4 exact anatomy and serialized MDL model construction
G5B-APX1-G0G Apply frozen Gate 0 mechanically
             fail -> NO-GO-G0-APX1, stop
G5B-APX1-R2  Freeze implementation SHA after Gate 0 pass
G5B-APX1-G1  D1-D4 C0/C1/S0/S1 q11/lw30 discovery
G5B-APX1-G1G Apply hierarchy and hierarchy-plus-column gates
G5B-APX1-V1  Run V1 only as known stress/inapplicability diagnosis
G5B-APX1-R3  Preserve artifacts and final negative/positive/invalid ruling
```

No local heavy build, benchmark, sweep, fuzz run, or statistical experiment belongs to this preregistration pass. No CI is dispatched by this document creation pass.
