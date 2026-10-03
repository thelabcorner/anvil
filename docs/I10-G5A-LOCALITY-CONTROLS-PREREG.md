# ANVIL I10 — G5A Locality Controls Preregistration

**Contract status:** FROZEN r1 — no G5A-LC implementation, workflow dispatch, or D1-D4/V1 corpus measurement is authorized until this file is committed and its exact Git blob is pinned  
**Date:** 2026-09-24  
**Parent evidence:** G5A run `35985412906`, `ORDER-MATERIAL / COLUMN-DOMINANT`  
**Frozen parent preregistration:** `docs/I10-GROTLI-G5A-ORDERING-ATTRIBUTION-PREREG.md`, revision r6, blob `a84f42a3f414cc987d8c19cc70a578e2d4222467`  
**Production authorization:** none  
**New held-out corpus:** none  
**Local machine role:** control plane only

This preregistration is written after the G5A outcome was known and uses only the published G5A/G3 evidence and source review. D1-D4 and V1 are spent for G5A-LC. G5A-LC is a new causal-control experiment, not a retroactive reinterpretation or validation of G5A.

---

## 0. Question and scope

G5A established, for one fixed lexical-chunk carrier and one Brotli build, that:

- D1-D4 aggregate complete bytes were A0/A1/A2/A3 = `2,054,532 / 1,470,205 / 1,452,383 / 1,380,245`;
- shape grouping contributed `17,822 B`;
- column ordering contributed `72,138 B`, or `80.1889728768%` of the signed net ordering saving;
- A3 was smaller than A1 on 4/4 discovery files, although D2 saved only 34 B;
- shape grouping was positive on D3, zero on D1/D2, and adverse by 529 B on D4;
- the single frozen full-random draw A0 was 674,287 B larger than A3;
- V1 was adverse: A3 exceeded A1 by 1,721 B, while A1 was already 70,030 B worse than raw complete Brotli.

G5A did not distinguish:

1. exact-shape column locality from arbitrary coherent permutation;
2. the effect of a single full-random draw from a small frozen null distribution;
3. robustness to the common Brotli prefix and to raw-residual placement;
4. whether the ordering effect survives after dictionary or integer leaves transform the payload;
5. whether the result depends on exact A2/A3 permutations being live rather than byte-size ties.

G5A-LC asks exactly those questions.

It does **not** introduce type-aware grouping, semantic paths, shape hierarchy, learned planning, new leaf families, a new backend, a new corpus, or a production format.

---

## 1. Frozen evidence anchors

### 1.1 G5A floor

The mandatory parent floor is:

| Fact | Frozen value |
|---|---|
| Workflow run | `35985412906` |
| Run conclusion | `success` |
| Workflow head SHA | `996c2dbe67736c42288287abba7ed6f3307a0b34` |
| Frozen implementation commit | `e6714e81aeff1579c4502f1fd9af4d7f205a8b4b` |
| G5A source blob | `2772d7eaf0a64be1fdbbb377f5d668d21dab9cbe` |
| G5A source SHA-256 | `0fe9b8e599c922d3f489ffed54d595d867cfdf7f1ff0a6f6f785996cf88d382f` |
| G5A prereg blob | `a84f42a3f414cc987d8c19cc70a578e2d4222467` |
| G5A binary SHA-256 | `119f1e661993976e14b98948538887cd3260ce22062f45f9c6b8e8f3778e4b00` |
| Evidence artifact ID | `10801714249` |
| Evidence artifact name | `grotli-g5a-ordering-attribution-35985412906` |
| Evidence artifact size | `15,631 B` |
| Evidence artifact digest | `sha256:4765b317e7c01170cffbd96b08c639e001ec1e2f317152d1f83a20a5331a3278` |

CI must download this artifact through the GitHub API, verify its exact size and digest, and require these exact member paths:

- `results/g5a-ruling.json`;
- `results/g5a-discovery.jsonl`;
- `results/g5a-v1.jsonl`;
- `results/discovery-provenance.json`;
- `grotli-g5a-implementation-identity.txt`;
- `grotli-g5a-prereg.sha256`;
- `grotli-g5a-binary.sha256`;
- `grotli-g3-source.blob`;
- `grotli-g3-source.sha256`;
- `brotli-package-identity.txt`;
- `brotli-library-sha256.txt`.

Suffix-only or shallowest-path extraction is forbidden.

### 1.2 Frozen G3 parser and transform-selection evidence

All parsing, frame splitting, exact lexical shape formation, and transform selection must derive from frozen G3 public SHA:

`1a3d18fed76adb6fb33264e1994f9c357306b3fa`

Frozen G3 source identities:

| Fact | Frozen value |
|---|---|
| G3 source blob | `eedc7b7e6671c4a5fcaf7a5997671bd4be30bdde` |
| G3 source SHA-256 | `5b3ab1cdc67d1a8d8edd7eeb4265361a66f72e5b8620137149ce802233b58a9e` |

The G5A-LC implementation must compile in CI against a materialized copy of that exact source. A local or mutable working-tree `tools/grotli_g3.cpp` is not an admissible substitute.

Frozen G3 evidence artifacts:

| Role | Artifact ID | Size | Digest |
|---|---:|---:|---|
| D1-D4 discovery and transform selections | `10787387512` | `455,992 B` | `sha256:296281267bc87734016240f9681c24587c98c9bb4048a186ac2054808576bfa7` |
| V1 known-stress transform selections | `10789470445` | `7,575 B` | `sha256:b83b7bbbb4c30f514e02745016241c683ab95464391d49440451f8678d705959` |

Both belong to G3 run `35947013432`.

G3 run head SHA: `43400c4b0da06feb7a49c95e0f79d8458d7ac367`.

The discovery artifact must expose these exact members:

- `results/discovery.jsonl`;
- `results/discovery-ruling.json`;
- `results/discovery-provenance.json`.

The V1 artifact must expose these exact members:

- `results/validation.jsonl`;
- `results/validation-ruling.json`;
- `results/validation-provenance.json`;
- `results/g2row-v1.json`.

The artifacts are references, not mutable configuration. G5A-LC must fail closed if a required D1-D4/V1 transform-selection fact is absent or malformed.

### 1.3 Frozen backend

All complete-byte arms must use the same Brotli implementation and build:

| Fact | Required value |
|---|---|
| `BrotliEncoderVersion()` | `16781312` |
| Dotted version | `1.1.0` |
| Ubuntu package | `libbrotli1=1.1.0-2build2`, `libbrotli-dev=1.1.0-2build2` |
| Quality | `11` |
| Window | `30` |
| Mode | `BROTLI_MODE_GENERIC` |
| `libbrotlidec.so.1.1.0` SHA-256 | `64d8a5019d4c294b89fde1193343ea324bbd8603652554e5545f0a01595fa2c5` |
| `libbrotlienc.so.1.1.0` SHA-256 | `6e59301f6c3a05815ecc6cd8c56714280367da1f6737e9f077f5847d86c509f2` |
| `libbrotlicommon.so.1.1.0` SHA-256 | `a91ead095d2c80520c55a89057bbe10b031a075340442e63f44b310f93883a1b` |

The compiler must be Ubuntu Clang 18.x; the full compiler identity and binary SHA-256 must be recorded. The runner must be `ubuntu-24.04`, with image version and `uname -a` recorded.

A backend hash or package-version mismatch is `INVALID-G5A-LC-BACKEND`, not permission to reinterpret the run on the new backend.

---

## 2. Pre-CI identity freeze

The future standalone implementation path is frozen as:

`tools/grotli_g5a_locality_controls.cpp`

The future workflow path is frozen as:

`.github/workflows/anvil-i10-g5a-locality-controls.yml`

The future workflow must use a two-phase identity freeze:

1. commit the finalized implementation source;
2. commit this preregistration and any pre-run correctness-only amendments;
3. in a follow-up commit, add a workflow pinning:
   - the exact 40-hex implementation commit SHA;
   - the exact 40-hex implementation-source Git blob;
   - the exact 40-hex preregistration Git blob;
   - the exact source SHA-256;
   - the compiler/package/library identities in section 1.3;
4. before dispatch, record the exact workflow blob and exact target commit/run-head SHA in the external dispatch audit;
5. CI must recompute and emit its workflow blob and `github.sha`; the dispatch audit must compare both to the pre-dispatch values;
6. dispatch only after all values are concrete and mutually verified.

Forbidden pin tokens include `PLACEHOLDER`, `TODO`, `HEAD`, branch names, or expressions resolved at dispatch time.

Because a file cannot contain its own final Git blob without a circular identity, the self-identifying prereg blob is frozen in the workflow commit, while the workflow blob and run head are frozen in the external pre-dispatch audit. CI re-emits both for comparison. This is mandatory and does not authorize post-outcome prereg changes.

After the first D1-D4/V1 G5A-LC measurement begins, this contract is immutable. A correctness-only defect requires:

- a new revision `rN+1`;
- a new implementation commit;
- a new source/prereg/workflow blob set;
- a new run;
- explicit preservation of the invalid prior artifact and reason.

Arms, seeds, corpus, thresholds, and gates may not move after outcomes are observed.

---

## 3. Common parsing and coordinate model

G5A-LC inherits frozen G3 semantics exactly:

- LF, CRLF, and final-remainder frame splitting;
- byte-exact per-frame JSON lexical parsing;
- malformed or incomplete frame -> exact raw residual;
- byte-identical inter-value template parts define exact shape identity;
- exact shape IDs use first-appearance order;
- original record order is represented explicitly;
- no whitespace, key order, number spelling, or string bytes are normalized.

Every structured scalar has a coordinate:

```text
(shape_id, occurrence, slot)
```

`occurrence` is zero-based position among members of that exact shape in source order. `slot` is zero-based scalar position in the exact shape template.

The canonical token index list is source-record order:

```text
for structured frame in source order:
    for slot 0 .. K-1:
        emit canonical index for (shape_id, occurrence, slot)
```

All Stage N and Stage C arms are permutations of this exact canonical coordinate list. All Stage T arms are permutations of the same final transformed chunk coordinates within their transform family.

---

## 4. Common carrier, selector, and complete-byte accounting

### 4.1 One charged selector byte

Every G5A-LC record has exactly one outer selector byte. The selector is not passed to Brotli. Selector values `0..31` are frozen in section 8.

```text
complete_bytes = 1 + Brotli_q11_lgwin30(body).size()
```

The selector byte is charged in every arm.

A deterministic seed, context stream, or permutation rule is reconstructible from the frozen selector and implementation version. No seed string is transmitted. If any additional file, path, type, dictionary, or grouping fact is needed, it must be serialized and charged; deterministic reconstruction may not hide semantic rediscovery.

### 4.2 End-to-end selector materialization

The implementation selftest and every corpus row must exercise the actual record path:

1. construct `(selector_byte, Brotli(body))`;
2. unpack the selector;
3. validate it against the exact frozen selector table;
4. Brotli-decompress the payload to the expected body length;
5. decode the carrier using the recovered selector;
6. reconstruct the exact source bytes.

A selftest that only packs an uncompressed body and separately decodes using the original enum is insufficient.

### 4.3 Common G5A-derived body

Unless a frozen stage explicitly changes context or transform metadata, the body grammar is the exact G5A r6 grammar:

```text
magic "G5AO"
version 1
source_len
frame_count
group_count
frame_group[frame_count]

group_descriptors[group_count]
raw_residual_section (when present)

structured_chunk_stream
```

The envelope is the exact common prefix before the mutable chunk permutation region. It includes every frame/group fact, template, dictionary-independent shape descriptor, raw length, raw residual byte, and transform-independent reconstruction fact used by that stage.

### 4.4 Fully charged fields

Complete bytes must include, as applicable:

- the selector byte;
- source length and all counts;
- the complete frame-to-group map;
- all shape descriptors and exact template parts;
- all raw residual lengths and bytes;
- all scalar/final-codeword length framing;
- every lexical token or transformed codeword byte;
- every dictionary table and dictionary code-width fact;
- every integer transform ID and state/base/width parameter;
- neutral context bytes when used;
- residual-suffix length/bytes when used;
- all Brotli output bytes.

No envelope, path, context, dictionary, transform parameter, or exception fact is free. No seed string is serialized; the charged selector plus the frozen implementation identity must determine it uniquely.

### 4.5 Raw context only

Raw Brotli is context and fallback, never a G5A-LC mechanism gate:

```text
raw_complete_context = 1 + uvar_len(source_len) + Brotli(source).size()
```

This is not a production raw wire format. It is reported only to preserve the G5A context convention.

---

## 5. Hard invariants and fail-closed rules

Every measured row must satisfy all of these.

### I1 — corpus identity

Size, Git blob, and SHA-256 must exactly match section 6 before parsing or measurement.

### I2 — parser and source identity

The G5A-LC build must compile against the materialized frozen G3 source. Its blob and SHA-256 must match section 1.2. Generated dependency output must prove inclusion of the materialized file.

### I3 — exact source roundtrip

Every selector must reconstruct the exact source bytes after actual compressed-record unpack, Brotli decode, and carrier decode.

### I4 — exact permutation coverage

For every arm:

```text
len(permutation) == chunk_count
```

and the permutation must contain every canonical coordinate index exactly once. Duplicate, omitted, or synthetic indices are invalid.

### I5 — byte-level chunk multiset identity

For lexical arms:

```text
record_i = uvar(len(chunk_bytes_i)) || chunk_bytes_i
multiset_sha256 = SHA-256(sort_lexicographically(record_1..record_N) joined)
```

For transformed arms, the same formula uses final transformed codeword records.

Within each causal comparison family, the multiset SHA-256 must be identical across all order arms.

### I6 — envelope identity

Within each causal comparison family, the envelope bytes and envelope SHA-256 must be byte-identical across all order arms.

### I7 — body and region length identity

Total body length, envelope length, chunk-region length, and context length must be identical across all order arms in one comparison family.

### I8 — backend identity

All arms in one run must use the exact section 1.3 backend. Backend version, package identity, linked-library SHA-256, q11, and lgwin30 must be emitted.

### I9 — selector validity

Only selectors `0..31` are valid. Unknown, truncated, duplicate-selector, or context/transform-mismatched selectors must fail closed.

### I10 — G5A same-run floor replay

CI must materialize and compile pinned G5A implementation `e6714e81...` against the same materialized frozen G3 source, then replay G5A on D1-D4 in the same job.

The replay must exactly reproduce, per file:

- G5A A1 complete bytes and body/envelope/token-region facts;
- G5A A2 complete bytes and body/envelope/token-region facts;
- G5A A3 complete bytes and body/envelope/token-region facts;
- G5A A0 complete bytes for the original seed;
- backend identity.

Any mismatch is `INVALID-G5A-LC-FLOOR`. Scientific interpretation is prohibited.

### I11 — raw transform reproduction

Stage T `RAW_LEX/SOURCE` and `RAW_LEX/COLUMN` must exactly reproduce G5A A1 and A3 body facts and complete bytes respectively, because their selector is out-of-band and the body grammar is unchanged.

### I12 — no-raw residual-suffix identity

Stage C `RESIDUAL_SUFFIX/A2` and `/A3` must be byte-identical to the corresponding `PREFIX_CURRENT` bodies on D1, D2, and D4, which have no raw residual frames. D3 is the only live residual-placement treatment.

### I13 — transformed-state safety

Every `INT_DELTA_FOR` or `INT_DOD_FOR` codeword must remain decodable when its containing `(shape, slot)` occurrence order is preserved. An order that changes dependency order is forbidden and invalid for that family.

### I14 — complete accounting

The emitted complete-byte equation must be mechanically recomputed from the actual Brotli payload plus one selector byte. Context, transform, and metadata bytes may not be omitted.

### I15 — timing is diagnostic only

A single encode/decode timing or RSS value may not decide any scientific gate. Any later Pareto claim requires a separately preregistered repeated remote benchmark.

Any I1-I15 failure routes to the exact invalid label in section 12. Invalid evidence is uploaded; it is never repaired by changing a gate.

---

## 6. Frozen corpus roles and identities

### 6.1 Discovery D1-D4

D1-D4 are spent discovery objects. They may be used to test mechanism controls but cannot validate generalization.

| ID | Object | Bytes | Git blob | SHA-256 |
|---|---|---:|---|---|
| D1 | Amazon cellphone NDJSON | 277,673 | `9cbb6a071eaa2a10c24d9b4d1729dca501f2f72b` | `c1518fdaaed45e590c480ed707aa1adaaba8b84b10747f956bd431c708bd590e` |
| D2 | CDISC ADaM adverse-event NDJSON | 615,350 | `4bb9ef50f650767967fc3f0a663c73d33c57c608` | `b795a59c5c92a8fc93d7d3f729fa0dae014e9b0f684a6a7a7bcf07e4104ba723` |
| D3 | GH Archive 10 MiB NDJSON | 10,485,760 | `59d1d00825053e9894a7895fcea0665912524ec8` | `a860af236f794779b5471b416b2e6d7ea68de00f6d17728dd3b088a7ecec8252` |
| D4 | CROVIA royalty receipts NDJSON | 3,585,053 | `24020a024a05a68731a2d4de4536bd7ca3eb8576` | `0db7ace9e46ce458a7055f48b09aabc7df15f4f74a23553d3660b7de61d0916f` |

Frozen URLs:

- D1: `https://raw.githubusercontent.com/simdjson/simdjson-data/4197c425e857f0ec38e89822fdd0bd9ea21f4daf/jsonexamples/amazon_cellphones.ndjson`
- D2: `https://raw.githubusercontent.com/cdisc-org/DataExchange-DatasetJson/a379f49a3f43c2aaed63bdeca761bdb7140df2c3/examples/adam/adae.ndjson`
- D3: `https://raw.githubusercontent.com/rushikeshmore/DataCortex/9bf63551499974e7398b87b45d1a9d4b50936566/corpus/json-bench/gharchive-10mb.ndjson`
- D4: `https://raw.githubusercontent.com/croviatrust/crovia-core/1798a9c9138c6efc3fcaf487ac5e9833008f6ad9/demo_dpi_2025-11/data/dpi_royalty_receipts.ndjson`

### 6.2 Known stress V1

V1 is `KNOWN-STRESS / NOT HELD-OUT FOR G5A-LC`.

| Fact | Frozen value |
|---|---|
| Bytes | `15,374,047` |
| Git blob | `8d9e6acf3f534194487532fa60d4f1d43fe71d00` |
| SHA-256 | `757b9bc7e5ee2d38ab5ed43877b1d87809cb5131621d106671bc27996da1c499` |
| Commit | `cd026a27a5e039863de9080b8d7d95acaf1f5a39` |
| Path | `data/release/all.jsonl` |
| URL | `https://raw.githubusercontent.com/DodgeLU/Sino-US-DrugQA/cd026a27a5e039863de9080b8d7d95acaf1f5a39/data/release/all.jsonl` |

V1 is reported as a sign and magnitude diagnostic. It cannot satisfy any discovery breadth, generalization, promotion, or Pareto gate.

### 6.3 Leakage prohibition

No new held-out object may be opened. D1-D4/V1 outcome-driven rule changes are forbidden. Type/path hierarchy remains a separate future lane.

---

## 7. Low-cost permutation diagnostics

These diagnostics are computed from the frozen G5A plan and permutations. They add no new candidate search and no new Brotli call beyond the mandatory same-run G5A floor replay.

For A1, A2, and A3, emit per file:

1. `chunk_count`;
2. `permutation_sha256`;
3. `a1_eq_a2`;
4. `a2_eq_a3`;
5. `a1_a2_moved_token_count`;
6. `a2_a3_moved_token_count`;
7. `shape_transition_count`;
8. `slot_transition_count`;
9. `same_shape_adjacent_pair_fraction`;
10. `same_shape_slot_adjacent_pair_fraction`;
11. `adjacent_equal_token_length_fraction`;
12. `token_length_histogram`;
13. `token_length_run_histogram`;
14. per-shape emitted chunk count and token bytes;
15. per-slot occurrence count, token bytes, distinct-token count, and token-length range;
16. envelope bytes and token-region bytes;
17. raw frame count, raw bytes, and raw frame position class;
18. first/last token coordinate and first/last token length;
19. permutation liveness class.

Define:

```text
permutation_sha256 =
  SHA-256(uvar(N) || uvar(perm[0]) || ... || uvar(perm[N-1]))
```

Transitions count adjacent emitted token pairs whose coordinate field differs. Fractions use `max(1, N-1)` as denominator and are emitted with numerator and denominator.

Permutation liveness classes:

- `IDENTITY`: permutation equals canonical source order;
- `MOVED`: permutation is valid and differs from canonical source order;
- `EQUAL_TO_REFERENCE`: valid and byte-equal to the named reference permutation.

### Diagnostic kill conditions

- A1 must equal A2 on a one-shape file; any inequality is an implementation bug because A1 and A2 are provably identical there.
- Any moved count must equal the exact Hamming distance between the two permutation vectors.
- Any transition count must be independently recomputable from the emitted permutation.
- Any mismatch is `INVALID-G5A-LC-DIAGNOSTIC`.

D1-D2 equal compressed A1/A2 sizes are not accepted as proof of equal permutations. The emitted permutation facts decide this.

---

## 8. Frozen selector table

Exactly 32 selectors exist.

| Selector | Arm | Stage |
|---:|---|---|
| 0 | `RANDOM_FULL_SEED_V1` | N |
| 1 | `SOURCE_ORDER` | N / T raw control |
| 2 | `SHAPE_ROW` | N / C control |
| 3 | `SHAPE_COLUMN` | N / T raw control |
| 4 | `RANDOM_FULL_SEED_02` | N |
| 5 | `RANDOM_FULL_SEED_03` | N |
| 6 | `RANDOM_FULL_SEED_04` | N |
| 7 | `RANDOM_FULL_SEED_05` | N |
| 8 | `RANDOM_FULL_SEED_06` | N |
| 9 | `RANDOM_FULL_SEED_07` | N |
| 10 | `RANDOM_FULL_SEED_08` | N |
| 11 | `ROW_SHUFFLE_V1` | N |
| 12 | `COLUMN_OCCURRENCE_SHUFFLE_V1` | N |
| 13 | `COLUMN_BLOCK_SHUFFLE_V1` | N |
| 14 | `SHAPE_BLOCK_SHUFFLE_V1` | N |
| 15 | `PREFIX_CURRENT_A2` | C |
| 16 | `PREFIX_CURRENT_A3` | C |
| 17 | `NEUTRAL_PREFIX_4096_A2` | C |
| 18 | `NEUTRAL_PREFIX_4096_A3` | C |
| 19 | `NEUTRAL_SUFFIX_4096_A2` | C |
| 20 | `NEUTRAL_SUFFIX_4096_A3` | C |
| 21 | `RESIDUAL_SUFFIX_A2` | C |
| 22 | `RESIDUAL_SUFFIX_A3` | C |
| 23 | `RAW_LEX_SOURCE` | T |
| 24 | `RAW_LEX_COLUMN` | T |
| 25 | `RAW_LEX_BLOCK_REORDER` | T |
| 26 | `G3_DICT_SOURCE` | T |
| 27 | `G3_DICT_COLUMN` | T |
| 28 | `G3_DICT_BLOCK_REORDER` | T |
| 29 | `G3_INT_SOURCE` | T |
| 30 | `G3_INT_COLUMN` | T |
| 31 | `G3_INT_BLOCK_REORDER` | T |

---

## 9. Stage N — full-random and locality-preserving nulls

### 9.1 Purpose

Stage N separates:

- full structural scrambling;
- row coherence;
- column contiguity;
- within-column source order;
- column-block order;
- shape-block order.

All Stage N arms use the exact G5A envelope and exact lexical scalar chunks.

### 9.2 Eight frozen full-random seeds

The exact seed strings are:

```text
G5A-RANDOM-PERMUTATION-SEED-v1
G5A-LC-RANDOM-SEED-02
G5A-LC-RANDOM-SEED-03
G5A-LC-RANDOM-SEED-04
G5A-LC-RANDOM-SEED-05
G5A-LC-RANDOM-SEED-06
G5A-LC-RANDOM-SEED-07
G5A-LC-RANDOM-SEED-08
```

For every seed:

```text
key(i) = SHA-256(canonical_bytes(seed) || 0x1F || u64le(i))
```

Sort canonical indices by unsigned big-endian `key(i)`, ties by lower canonical index. This is exactly the G5A A0 construction generalized to the frozen seed list.

The eight D1-D4 aggregate complete-byte totals are sorted. With eight values, the frozen median is the arithmetic mean of the 4th and 5th values.

No p-value, confidence interval, “random is worse on average,” or significance language is permitted. These are eight deterministic diagnostic points.

Selector 0 must exactly reproduce G5A A0 on D1-D4 and V1.

### 9.3 `ROW_SHUFFLE_V1`

Seed:

`G5A-LC-ROW-SHUFFLE-SEED-v1`

For each shape, independently stable-sort occurrences by:

```text
SHA-256(seed || 0x1F || u64le(shape_id) || u64le(occurrence))
```

Ties use lower occurrence.

Emission:

```text
for shape in first-appearance order:
    for occurrence in shuffled order:
        for slot in ascending slot order:
            emit chunk
```

This preserves all slots from one source record as an adjacent row but destroys global source-record order and column contiguity.

### 9.4 `COLUMN_OCCURRENCE_SHUFFLE_V1`

Seed:

`G5A-LC-COLUMN-OCCURRENCE-SHUFFLE-SEED-v1`

For every `(shape, slot)`, independently stable-sort occurrences by:

```text
SHA-256(seed || 0x1F ||
        u64le(shape_id) || u64le(slot) || u64le(occurrence))
```

Ties use lower occurrence.

Emission:

```text
for shape in first-appearance order:
    for slot in ascending slot order:
        for occurrence in shuffled occurrence order:
            emit chunk
```

This preserves exact column blocks and each column's length multiset, but destroys source order within each column and cross-column row correspondence.

### 9.5 `COLUMN_BLOCK_SHUFFLE_V1`

Seed:

`G5A-LC-COLUMN-BLOCK-SHUFFLE-SEED-v1`

Stable-sort all `(shape, slot)` blocks by:

```text
SHA-256(seed || 0x1F || u64le(shape_id) || u64le(slot))
```

Ties use `(shape_id, slot)` ascending.

Within each block, occurrences remain in source order.

This preserves every column's exact byte sequence and changes only the global order of column blocks.

### 9.6 `SHAPE_BLOCK_SHUFFLE_V1`

Seed:

`G5A-LC-SHAPE-BLOCK-SHUFFLE-SEED-v1`

Stable-sort shapes by:

```text
SHA-256(seed || 0x1F || u64le(shape_id))
```

Ties use lower shape ID.

Within each shuffled shape, emit the exact A3 shape-column sequence: slot ascending, occurrence source order.

This preserves each exact-shape column sequence and changes only shape-block order.

### 9.7 Stage N metrics

For every arm/file emit:

- all G5A metrics;
- selector and seed;
- permutation SHA-256;
- moved count versus A1 and A3 where meaningful;
- shape/slot transition diagnostics from section 7;
- complete bytes;
- diagnostic encode/decode time.

Define D1-D4 aggregate savings:

```text
saved(control, candidate) =
  sum(control.complete_bytes) - sum(candidate.complete_bytes)

saved_pct(control, candidate) =
  saved(control, candidate) / sum(control.complete_bytes)
```

Positive means the candidate is smaller.

### 9.8 Frozen Stage N gates

`LC_COLUMN-CORE-ROBUST` requires all:

1. selector 3 versus selector 2 saves at least 1.0% aggregate;
2. selector 3 is smaller than selector 2 on at least 3 of D1-D4;
3. selector 3 is no larger than selector 2 on each of D1, D3, and D4;
4. selector 3 beats `ROW_SHUFFLE_V1` by at least 1.0% aggregate;
5. selector 3 beats `COLUMN_OCCURRENCE_SHUFFLE_V1` by at least 1.0% aggregate;
6. selector 3 complete bytes are strictly below the median of the eight frozen full-random totals.

`LC_EXACT-BLOCK-ORDER-ROBUST` additionally requires:

- selector 3 beats `COLUMN_BLOCK_SHUFFLE_V1` by at least 0.5% aggregate; and
- selector 3 beats `SHAPE_BLOCK_SHUFFLE_V1` by at least 0.5% aggregate.

If the core gate passes but the exact-block gate fails, the result is:

`LC_COLUMN-CORE-ROBUST / BLOCK-ORDER-OPEN`.

If the core gate does not pass:

- `LC_COLUMN-CORE-UNSUPPORTED` applies when A3 aggregate complete bytes are greater than or equal to A2 aggregate complete bytes;
- `LC_COLUMN-CORE-WEAK` applies when A3 is smaller in aggregate but one or more core-robustness requirements fail.

Shape grouping is classified on a separate axis:

- `LC_SHAPE_ROBUST`: A2 saves at least 1.0% aggregate and is smaller on at least 3/4;
- `LC_SHAPE_CONCENTRATED`: A2 saves at least 1.0% aggregate but is smaller on fewer than 3/4;
- `LC_SHAPE_WEAK`: A2 saves more than zero but less than 1.0% aggregate;
- `LC_SHAPE_NEUTRAL`: aggregate A1-A2 saving equals zero;
- `LC_SHAPE_ADVERSE`: aggregate A1-A2 saving is negative.

A2 byte equality on a multi-shape file never proves permutation identity; the section 7 diagnostics decide liveness.

### 9.9 Stage N kill conditions

- If `LC_COLUMN-CORE-ROBUST` fails, no general or context-invariant column-order claim is allowed. Stages C and T remain eligible because they diagnose context and transform dependence.
- If exact A3 does not beat `COLUMN_OCCURRENCE_SHUFFLE_V1` by 1%, the claim is limited to column contiguity, not source-ordered within-column locality.
- If A3 does not beat `ROW_SHUFFLE_V1` by 1%, row coherence is not isolated as favorable relative to arbitrary stable row grouping.
- If any full-random seed is smaller than A3, that seed must be reported. It does not invalidate the run and does not become a p-value.
- Any permutation, multiset, envelope, selector, or roundtrip failure is invalid, not an adverse scientific result.

---

## 10. Stage C — raw-residual and Brotli-context perturbation

### 10.1 Frozen neutral context bytes

Define exactly 4,096 bytes:

```text
for i = 0 .. 127:
    neutral_digest[i] =
      SHA-256("G5A-LC-NEUTRAL-CONTEXT-v1" || 0x1F || u64le(i))
```

Concatenate the 128 32-byte digests in ascending `i`.

The bytes are fully charged. No length field is needed because the selector and frozen implementation define exactly 4,096 bytes.

### 10.2 Contexts

For every context, run exact `SHAPE_ROW` and `SHAPE_COLUMN` token orders.

#### `PREFIX_CURRENT`

Selectors 15/16.

Body is the exact G5A r6 body layout. It must reproduce G5A A2/A3 body and complete bytes exactly.

#### `NEUTRAL_PREFIX_4096`

Selectors 17/18.

Body is:

```text
neutral_4096 || G5A_current_body
```

The neutral bytes precede the G5A magic and therefore alter Brotli's initial context. A2 and A3 bodies are identical except for the token permutation.

#### `NEUTRAL_SUFFIX_4096`

Selectors 19/20.

Body is:

```text
G5A_current_body || neutral_4096
```

The neutral bytes follow the token stream and alter terminal Brotli context without changing the leading envelope.

#### `RESIDUAL_SUFFIX`

Selectors 21/22.

For no-residual files, this must be byte-identical to `PREFIX_CURRENT`.

For D3:

```text
structured_prefix_without_raw_residual_section
|| token_stream
|| raw_residual_section
```

The raw group descriptor and frame/group map remain in the prefix. Raw residual bytes and lengths move unchanged from the prefix to the common suffix. The suffix is identical between A2 and A3.

### 10.3 Context invariants

Within each context:

- A2/A3 envelope or structured-prefix hash must match;
- residual suffix bytes/hash must match;
- neutral context bytes/hash must match;
- total body length and chunk-region length must match;
- lexical chunk multiset hash must match;
- every body must roundtrip.

Cross-context absolute sizes are not expected to match.

### 10.4 Frozen context gates

For each context `c`, define:

```text
column_saved(c) =
  sum(A2_c.complete_bytes) - sum(A3_c.complete_bytes)
```

`LC_CONTEXT_ROBUST` requires, for all four contexts:

1. aggregate column saving is at least 1.0% of that context's A2 aggregate;
2. A3 is smaller than A2 on D1, D3, and D4;
3. no context produces an A3-over-A2 reversal on D1, D3, or D4.

`LC_NEUTRAL_CONTEXT_ROBUST` additionally requires:

- `NEUTRAL_PREFIX_4096` aggregate column saving is at least 50% of `PREFIX_CURRENT` aggregate column saving; and
- `NEUTRAL_SUFFIX_4096` aggregate column saving is at least 50% of `PREFIX_CURRENT` aggregate column saving.

`LC_RESIDUAL_POSITION_ROBUST` requires:

- D3 `RESIDUAL_SUFFIX` column saving is positive; and
- it is at least `22,566 B`, equal to 50% of the observed G5A D3 `PREFIX_CURRENT` column saving rounded down.

Failure labels:

- any context reverses D1/D3/D4: `LC_CONTEXT-SENSITIVE`;
- either neutral context retains less than 50%: `LC_NEUTRAL-CONTEXT-DEPENDENT`;
- D3 residual suffix saving is nonpositive: `LC_RESIDUAL-POSITION-ADVERSE`;
- D3 residual suffix saving is positive but below 22,566 B: `LC_RESIDUAL-POSITION-WEAK`.

### 10.5 Stage C kill conditions

- C0 mismatch with G5A A2/A3: `INVALID-G5A-LC-CONTEXT-CURRENT`.
- C3 mismatch on D1/D2/D4: `INVALID-G5A-LC-RESIDUAL-IDENTITY`.
- Neutral bytes not exactly 4,096 bytes or not charged: `INVALID-G5A-LC-CONTEXT-COST`.
- A context failure blocks context-invariance claims. It does not erase a separately valid Stage N result, and it does not cancel Stage T.

---

## 11. Stage T — reorder-after-transform controls

### 11.1 Purpose

G5A used exact lexical scalar chunks. Stage T asks whether the same order effects remain after frozen G3 dictionary or integer leaves transform the payload.

It does **not** test planner quality, leaf-search quality, or dictionary/integer superiority across families. It tests only the conditional effect of order after each transform family is fixed.

### 11.2 Frozen transform families

Exactly three families exist:

1. `RAW_LEX`;
2. `G3_REGION_DICT`;
3. `G3_REGION_INT`.

The transform selection is read from the pinned G3 evidence artifacts. No q11 candidate search, isolated leaf scoring, dictionary optimization, integer parameter search, or planner rerun is allowed.

`G3_REGION_DICT` uses the exact per-shape/per-slot `region_dict_leaf` selection from the G3 row. Non-`RAW_LEX` entries must be `EXACT_DICT`.

`G3_INT` uses the exact per-shape/per-slot `region_int_leaf` selection. Non-`RAW_LEX` entries must be one of:

- `INT_FOR`;
- `INT_DELTA_FOR`;
- `INT_DOD_FOR`.

No new transform ID is permitted.

### 11.3 Final transformed chunk definition

For each family, every structured scalar must have exactly one final codeword chunk.

The canonical codeword is produced by the frozen encoder under the source coordinate order. A serialized chunk is:

```text
codeword_len : uvar
codeword     : exact codeword_len bytes
```

Requirements:

- chunk count equals G5A structured token chunk count;
- the final codeword multiset is identical across source, column, and block-reorder arms;
- all transform-specific dictionary/state/width facts are in the common envelope;
- all length prefixes are charged;
- decode-visible state dependencies are evaluated in occurrence order within each `(shape, slot)` column.

For stateful delta/DoD codewords, source order and column order both preserve occurrence order within each column. Block reorder also preserves it because an entire block moves intact.

### 11.4 Frozen transform orders

For every family:

#### `SOURCE`

```text
for structured frame in source order:
    for slot ascending:
        emit transformed chunk
```

#### `COLUMN`

```text
for shape in first-appearance order:
    for slot ascending:
        for occurrence in source order:
            emit transformed chunk
```

#### `BLOCK_REORDER`

Use the exact `COLUMN_BLOCK_SHUFFLE_V1` key and ordering from section 9.5. Emit every shuffled `(shape, slot)` block with its transformed chunks in source-occurrence order.

Selectors are frozen as:

| Family | Source | Column | Block reorder |
|---|---:|---:|---:|
| `RAW_LEX` | 23 | 24 | 25 |
| `G3_REGION_DICT` | 26 | 27 | 28 |
| `G3_REGION_INT` | 29 | 30 | 31 |

### 11.5 Per-family common envelope

Within a family, source/column/block arms must have byte-identical envelopes containing:

- carrier magic/version/source/frame/group facts;
- exact shape templates;
- raw residual section;
- transform family ID;
- per-shape/per-slot transform ID;
- all dictionary tables and code-width facts;
- all integer bases, widths, miniblock/state parameters, and exception/escape facts;
- all reconstruction facts.

No transform parameter or dictionary byte may be arm-specific or omitted.

### 11.6 Raw floor and degeneracy

`RAW_LEX/SOURCE` must exactly reproduce G5A A1.

`RAW_LEX/COLUMN` must exactly reproduce G5A A3.

A family/file is `DEGENERATE` when either:

- source and column permutations are identical; or
- a transformed family has no non-`RAW_LEX` substitution on that file.

A degenerate file contributes to aggregate bytes but not to breadth credit.

### 11.7 Frozen per-family order gates

For each family `f`, define:

```text
source_total(f) = sum(D1-D4 SOURCE complete bytes)
column_total(f) = sum(D1-D4 COLUMN complete bytes)
block_total(f)  = sum(D1-D4 BLOCK_REORDER complete bytes)
```

`LC_ORDER-POST-TRANSFORM-ROBUST(f)` requires:

1. `1 - column_total(f) / source_total(f) >= 0.01`;
2. COLUMN is smaller than SOURCE on at least 2 nondegenerate files;
3. no nondegenerate D1, D3, or D4 file regresses by more than 0.5%;
4. all final chunk multiset, envelope, state, roundtrip, and complete-byte gates pass.

`LC_EXACT_TRANSFORM_BLOCK_ORDER(f)` additionally requires:

- COLUMN beats BLOCK_REORDER by at least 0.5% aggregate; and
- COLUMN is no larger than BLOCK_REORDER on any nondegenerate file.

Cross-family labels:

- `LC_ORDER-SURVIVES-ALL-TRANSFORMS`: robust for `RAW_LEX`, `G3_REGION_DICT`, and `G3_REGION_INT`;
- `LC_ORDER-TRANSFORM-CONDITIONED`: robust for raw lexical chunks but not for at least one transformed family;
- `LC_ORDER-DESTROYED-BY-TRANSFORM`: no transformed family is robust.

The absolute best family is not selected by this stage. Dictionary/integer gains and ordering gains are separate axes.

### 11.8 Stage T kill conditions

- Final transformed chunk count differs from structured scalar count: `INVALID-G5A-LC-TRANSFORM-CHUNKS`.
- Source/column/block multiset hash differs within a family: `INVALID-G5A-LC-TRANSFORM-MULTISET`.
- Common transform envelope differs within a family: `INVALID-G5A-LC-TRANSFORM-ENVELOPE`.
- Stateful integer dependency order is broken: `INVALID-G5A-LC-TRANSFORM-STATE`.
- RAW_LEX does not reproduce G5A A1/A3: `INVALID-G5A-LC-TRANSFORM-FLOOR`.
- One family failing does not cancel the other independent families. The failed family is quarantined and receives no order conclusion.

---

## 12. Mechanical invalid labels and scientific axes

### 12.1 Invalid labels

| Label | Condition |
|---|---|
| `INVALID-G5A-LC-IDENTITY` | future implementation/source/prereg/workflow pin missing, mutable, or mismatched |
| `INVALID-G5A-LC-CORPUS` | corpus size/blob/SHA mismatch |
| `INVALID-G5A-LC-BACKEND` | Brotli/package/library/parameter mismatch |
| `INVALID-G5A-LC-BUILD` | warning-clean build, frozen-G3 inclusion, or selftest failure |
| `INVALID-G5A-LC-FLOOR` | G5A artifact or same-run replay mismatch |
| `INVALID-G5A-LC-DIAGNOSTIC` | permutation diagnostic contradiction |
| `INVALID-G5A-LC-PERMUTATION` | coverage, moved-count, or permutation hash failure |
| `INVALID-G5A-LC-MULTISET` | chunk multiset mismatch within a comparison |
| `INVALID-G5A-LC-ENVELOPE` | envelope mismatch within a comparison |
| `INVALID-G5A-LC-LENGTH` | body/envelope/chunk/context length mismatch |
| `INVALID-G5A-LC-SELECTOR` | selector or actual packed-record roundtrip failure |
| `INVALID-G5A-LC-COMPLETE-COST` | complete-byte equation or charged-field failure |
| `INVALID-G5A-LC-CONTEXT-CURRENT` | C0 does not reproduce G5A A2/A3 |
| `INVALID-G5A-LC-RESIDUAL-IDENTITY` | C3 differs on a no-residual file |
| `INVALID-G5A-LC-CONTEXT-COST` | neutral/residual context bytes not exact or not charged |
| `INVALID-G5A-LC-TRANSFORM-CHUNKS` | transformed chunk count mismatch |
| `INVALID-G5A-LC-TRANSFORM-MULTISET` | transformed final chunk multiset mismatch |
| `INVALID-G5A-LC-TRANSFORM-ENVELOPE` | transform metadata/envelope mismatch |
| `INVALID-G5A-LC-TRANSFORM-STATE` | integer dependency corruption |
| `INVALID-G5A-LC-TRANSFORM-FLOOR` | transformed raw floor mismatch |
| `INVALID-G5A-LC-RULING` | machine ruling disagrees with recomputation |

No invalid path emits a favorable scientific label.

### 12.2 Independent scientific axes

The final ruling must report separate axes rather than one first-match promotion label:

1. `column_axis`: `LC_COLUMN-CORE-ROBUST`, `LC_COLUMN-CORE-WEAK`, or `LC_COLUMN-CORE-UNSUPPORTED`;
2. `exact_block_axis`: `LC_EXACT-BLOCK-ORDER-ROBUST` or `LC_BLOCK-ORDER-OPEN`;
3. `shape_axis`: `LC_SHAPE_ROBUST`, `LC_SHAPE_CONCENTRATED`, `LC_SHAPE_WEAK`, `LC_SHAPE_NEUTRAL`, or `LC_SHAPE_ADVERSE`;
4. `context_axis`: `LC_CONTEXT_ROBUST` or `LC_CONTEXT-SENSITIVE`;
5. `neutral_context_axis`: `LC_NEUTRAL-CONTEXT-ROBUST` or `LC_NEUTRAL-CONTEXT-DEPENDENT`;
6. `residual_axis`: `LC_RESIDUAL-POSITION-ROBUST`, `LC_RESIDUAL-POSITION-WEAK`, or `LC_RESIDUAL-POSITION-ADVERSE`;
7. `transform_axis[f]`: `LC_ORDER-POST-TRANSFORM-ROBUST` or `LC_ORDER-POST-TRANSFORM-NOT-ROBUST`;
8. `transform_summary`: `LC_ORDER-SURVIVES-ALL-TRANSFORMS`, `LC_ORDER-TRANSFORM-CONDITIONED`, or `LC_ORDER-DESTROYED-BY-TRANSFORM`.

Axes may disagree. No axis may erase another.

---

## 13. Exact execution ladder

### Phase 0 — freeze, no CI

1. commit this preregistration;
2. implement only the standalone research source;
3. use local compile and tiny synthetic correctness fixtures only;
4. freeze implementation commit/source hash/blob;
5. commit the follow-up workflow with exact identity pins;
6. record workflow blob/run-head identity in a non-mutating pin record.

No D1-D4/V1 local measurement is allowed.

### Phase 1 — CI identity and build

1. verify workflow/run-head/source/prereg/workflow pins;
2. verify G5A and G3 artifact identities;
3. fetch and verify D1-D4/V1 identities;
4. verify compiler/backend/library identities;
5. materialize and verify frozen G3 source;
6. build warning-clean with Clang 18;
7. prove generated dependencies include frozen G3;
8. run synthetic permutation, selector, residual-context, transform-state, and malformed-record selftests.

Failure stops the run and uploads invalid evidence.

### Phase 2 — same-run G5A floor

Build pinned G5A and replay selectors 0-3 on D1-D4. Require exact archived and same-run reproduction before any G5A-LC treatment interpretation.

### Phase 3 — low-cost diagnostics

Build the G5A-LC plan once per file. Emit section 7 diagnostics for A1/A2/A3. No treatment outcome is used to alter arms.

### Phase 4 — Stage N

Run selectors 0-14 on D1-D4, then V1 diagnostic. Apply section 9 gates mechanically.

### Phase 5 — Stage C

Run selectors 15-22 on D1-D4, then V1 diagnostic. Apply section 10 gates mechanically.

### Phase 6 — Stage T

Run selectors 23-31 on D1-D4, then V1 diagnostic. Apply section 11 gates independently per family.

### Phase 7 — ruling and evidence

1. recompute every aggregate from per-file complete bytes;
2. re-gate all invalid conditions;
3. emit independent section 12 axes;
4. upload machine-readable rows, diagnostics, transform selections, identities, logs, and timings;
5. record artifact ID, size, and digest;
6. write a source-of-record result only after the ruling is immutable.

A scientific Stage N failure does not cancel independent Stage C/T explanatory work. It blocks only the corresponding favorable claim. An invariant failure stops all scientific interpretation.

---

## 14. Kill and stop conditions

### 14.1 Immediate whole-run kill

Stop scientific interpretation immediately on any:

- mutable or missing identity pin;
- corpus mismatch;
- backend mismatch;
- frozen-G3 inclusion mismatch;
- G5A floor replay mismatch;
- permutation diagnostic contradiction;
- selector/materialized-record roundtrip failure;
- permutation coverage failure;
- chunk multiset mismatch;
- envelope mismatch within a comparison;
- body/region length mismatch;
- incomplete byte accounting;
- exact source roundtrip failure;
- ruling recomputation mismatch.

### 14.2 Stage-local quarantine

Quarantine only the affected stage/family on:

- context C0/C3 identity failure;
- transformed multiset/envelope/state failure;
- non-degenerate raw transform floor mismatch.

### 14.3 Scientific claim kills

- Stage N core failure kills `column-locality is robust across coherent nulls`.
- Stage N block failure kills only exact block-order optimality, not column-core evidence.
- Shape gate failure kills general shape-grouping claims.
- Context failure kills context-invariant ordering claims.
- Residual failure kills raw-residual-position neutrality.
- Any transformed-family failure kills post-transform claims for that family only.
- Failure of both transformed families kills `ordering survives dictionary/integer transforms`.
- V1 adverse behavior may not be called a held-out failure or success; it remains known stress.

No raw fallback may rescue a scientific mechanism gate. Raw fallback remains the permanent production arbitration rule.

---

## 15. Required machine-readable outputs

Each row must include:

- schema and experiment revision;
- file ID/path/role/bytes/blob/SHA-256;
- implementation commit/source blob/source SHA-256;
- prereg blob;
- workflow blob/run head;
- G5A floor artifact ID/digest;
- G3 source/artifact identities;
- compiler/package/library/backend identities;
- quality/window/mode;
- stage, selector, selector name, seed/context/transform family;
- source, frame, structured, raw, shape, scalar, and chunk counts;
- envelope length/hash;
- chunk-region length;
- context length/hash;
- residual-suffix length/hash;
- transform metadata hash;
- final chunk multiset hash;
- permutation hash;
- moved count and liveness;
- complete and raw-context bytes;
- exact roundtrip and actual packed-record selector roundtrip;
- diagnostic build/encode/decode timings.

Aggregate output must include:

- all eight full-random totals and frozen median;
- all locality-null totals;
- per-context A2/A3 totals;
- per-transform-family source/column/block totals;
- every saved-byte and saved-percentage term used by a gate;
- all independent section 12 axes;
- all invalid reasons, including empty arrays when valid.

No CSV-only evidence is sufficient. JSON or JSONL is authoritative.

---

## 16. Prohibited claims and exclusions

G5A-LC may conclude only whether, on spent D1-D4:

- exact-shape column order beats specified coherent nulls;
- the effect is sensitive to shape, row, column, block, or full-random ordering;
- the effect survives fixed neutral prefix/suffix context;
- D3's column effect survives moving its 501-byte raw residual after the token stream;
- fixed dictionary or integer codeword order retains the source-versus-column effect.

It may not conclude:

- held-out generalization;
- a p-value or statistical significance from eight seeds;
- a universal best ordering;
- a production planner;
- a production transform or wire ID;
- semantic/path/type hierarchy;
- dictionary or integer family superiority from cross-family absolute bytes;
- encode/decode/RSS Pareto movement;
- a new corpus result.

Excluded mechanisms:

- semantic parsing or path extraction;
- type-aware grouping;
- shape hierarchy;
- learned order scoring;
- new dictionaries or integer transforms;
- q11 leaf search;
- new backend, quality, or window sweep;
- new held-out corpus;
- changes to G5A, G5B, production source, or existing wire modes.

---

## 17. Production and research disposition

Regardless of axis outcome:

- no production transform ID is allocated;
- no production source is changed;
- no planner is promoted;
- no existing wire mode changes;
- raw Brotli remains the permanent fallback;
- D1-D4/V1 provide no held-out promotion evidence;
- negative and invalid artifacts remain preserved.

A successful G5A-LC result authorizes only a separately preregistered successor question. It does not authorize integration by itself.

---

## 18. Reproduction checklist

A future admissible workflow must:

1. check out the exact frozen implementation commit;
2. verify source/prereg/workflow/run-head identities;
3. verify the G5A artifact digest and exact member paths;
4. verify G3 source and transform-evidence artifact identities;
5. fetch and verify D1-D4/V1;
6. verify libbrotli package and linked-library hashes;
7. compile against materialized frozen G3;
8. run all correctness and malformed-record selftests;
9. replay G5A A0-A3 on D1-D4 in the same job;
10. compute diagnostics;
11. run Stages N, C, and T in the frozen selector order;
12. apply all gates mechanically;
13. upload complete evidence with `if: always()`;
14. preserve every invalid and negative result.

No CI is authorized by this document alone until the implementation and exact identity pins described in section 2 exist.
