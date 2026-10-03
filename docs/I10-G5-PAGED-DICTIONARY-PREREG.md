# ANVIL I10 — G5 Paged Base+Overlay Dictionary Preregistration

**Status:** FROZEN r1 BEFORE ANY G5D D1-D4 / V1 MEASUREMENT  
**Date:** 2026-09-24  
**Parent evidence:** G2 dictionary mechanism positive; G3 `PASS-G3-NARROW`; G4 `NO-GO-G4`; G5A ordering attribution required as context  
**Production authorization:** none  
**Research source:** `tools/grotli_g5_paged_dictionary.cpp` (not yet implemented by this preregistration)

---

## 0. Question and frozen ruling surface

G5D asks one isolated question:

> Can a decoder-cheap, byte-exact dictionary built from one shared root table
> plus fixed-size local overlay pages, with a mandatory raw escape, improve the
> complete G3 portfolio without adding a generic trie, a second entropy backend,
> or a representation-family confound?

This is an adoption/Pareto experiment. Exact dictionaries, page dictionaries,
prefix coding, FSST-like symbols, defaults, RLE, exception streams, and shape
hierarchies are prior-art-shaped mechanisms. No primitive mechanism novelty is
claimed.

The only possible project-level claim is an evidence-backed compiler/runtime
integration with exact accounting and a non-dominated size/encode/decode/memory
point. G5D cannot promote a production transform ID by ratio alone.

Exactly three G5D classifications are possible after valid discovery evidence:

- `PASS-G5D-DISCOVERY`;
- `NO-GO-G5D-DISCOVERY`;
- `INVALID-G5D`.

`PASS-G5D-DISCOVERY` authorizes only a separately frozen performance micro and
then a separately frozen held-out corpus. It does not authorize integration.

---

## 1. Evidence motivating G5D

### 1.1 Measured positive

G2 established that exact lexical dictionaries are a real typed mechanism:

- D2 CDISC changed from a G1 structured loss to a **16.4849%** complete-byte
  win over raw Brotli;
- D2 P-DICT selected 43 `EXACT_DICT` columns;
- D2 integer-only selection did not explain the crossing;
- D4 selected 724 dictionary leaves in P-DICT and 631 in P-MIXED.

Source of record:

- `docs/I10-GROTLI-G2-RESULTS.md`.

### 1.2 Measured limitation

Frozen G2/G3 `EXACT_DICT` uses:

```text
dict_count
for each exact first-occurrence token:
    token_length
    token_bytes
id_bit_width
fixed-width packed_ids
```

It has:

- one complete dictionary per exact shape-slot column;
- first-occurrence table order;
- every distinct token admitted;
- no escape code;
- no shared root across pages;
- no regional overlay;
- no adaptation to dictionary drift;
- no explicit frequency model, only the fixed-width IDs later seen by Brotli.

The wire semantics are frozen in
`docs/I10-GROTLI-G2-TYPED-PREREG.md` and implemented in
`tools/grotli_g3.cpp`.

### 1.3 Held-out negative

G3 regionization passed on D1-D4 but the frozen V1 result was **3.1900% worse**
than raw Brotli:

- raw: 2,112,235 B;
- region RAW_LEX: 2,184,665 B;
- region DICT/MIXED: 2,179,615 B;
- structured coverage: 100%;
- exact shapes: 3;
- exact-dictionary leaves: 19 in DICT, 48 in MIXED.

V1 had no framing or raw-residual failure. The current exact dictionary closed
only 5,050 B of the regionized penalty. G5D does not claim that a paged
dictionary will repair V1.

Source of record:

- `docs/I10-GROTLI-G3-RESULTS.md`.

---

## 2. Scope and exclusions

G5D may introduce only:

1. a new leaf operator `PAGED_BASE_OVERLAY_DICT`;
2. deterministic root-table selection;
3. deterministic page-overlay selection;
4. a mandatory escape representation;
5. explicit complete-carrier accounting and diagnostics;
6. the standalone research source and manual remote workflow required to test
   them.

G5D must not add or combine:

- FSST or learned substring symbols;
- BPE or arbitrary subword merges;
- prefix/front-coded dictionary entries;
- defaults, RLE, run maps, or sparse exception masks;
- shape hierarchy, schema union, optional-field normalization, or cross-column
  prediction;
- integer, float, Gorilla, ALP, or patched-FOR behavior;
- replay or generative decoding;
- a second entropy backend;
- a learned dictionary or transmitted predictor parameters;
- a production transform ID;
- edits to `src/anvil.cpp`.

Any failure of one excluded family does not authorize adding it to G5D.

---

## 3. Frozen parent semantics

### 3.1 Parser, framing, shapes, and reconstruction

G5D must compile against the materialized source at frozen G3 public SHA:

```text
1a3d18fed76adb6fb33264e1994f9c357306b3fa
```

The source identity must equal the G3 worktree blob:

```text
eedc7b7e6671c4a5fcaf7a5997671bd4be30bdde
```

The materialized source SHA-256 must equal:

```text
5b3ab1cdc67d1a8d8edd7eeb4265361a66f72e5b8620137149ce802233b58a9e
```

G5D inherits without modification:

- LF/CRLF/final-remainder frame splitting;
- per-frame exact lexical JSON eligibility;
- malformed/incomplete frame -> exact raw residual;
- exact structural shape identity;
- first-appearance shape order;
- explicit frame-to-group reconstruction order;
- exact lexical scalar bytes;
- no JSON normalization, unescaping, or reserialization.

The G5D decoder does not parse JSON and does not rediscover structure.

### 3.2 Raw fallback

`Brotli(original source bytes)` is a mandatory complete candidate in every
file. Raw ties win.

The final routed candidate is:

```text
min(
  RAW_BROTLI,
  frozen G2_WHOLE where available,
  frozen G3_REGION_RAW,
  frozen G3_REGION_DICT,
  frozen G3_REGION_INT,
  frozen G3_REGION_MIXED,
  every valid G5D page-policy/order arm
)
```

Raw/current-control ties win. A G5D arm must never increase selected bytes.

---

## 4. G5A ordering controls

G5A is an anatomy experiment over the same scalar chunk multiset. G5D must not
assume that shape-column ordering is universally beneficial.

The remote workflow must verify and archive the G5A source-of-record result
identity before measuring a G5D corpus object. Missing or unidentified G5A
evidence is a workflow gate failure, not permission to choose an order after
seeing G5D bytes.

Every G5D page policy is emitted in both frozen orders:

1. `SOURCE_ORDER`;
2. `SHAPE_COLUMN`.

For a fixed page policy, both orders must have:

- the same parser and frame split;
- the same structured/raw classification;
- the same exact shape identities and templates;
- the same raw residual bytes;
- the same original scalar token multiset;
- the same root-table content and order;
- the same overlay-table content and order;
- the same root/overlay admission decisions;
- the same original decoded source;
- the same backend implementation and parameters;
- a charged one-byte order selector.

The order selector is outside the Brotli body and included in every complete-byte
total. It is materialized by an explicit pack/unpack self-test.

G5D reports both orders. It may route either order, but it must not attribute
the A1/A3 difference to the paged dictionary. The V1 known-stress label from
G5A is context only and cannot select the G5D order.

---

## 5. Corpus roles and firewall

### 5.1 Discovery / regression controls

The immutable D1-D4 objects are reused:

| ID | Object | Bytes | SHA-256 |
|---|---|---:|---|
| D1 | Amazon cellphone NDJSON | 277,673 | `c1518fdaaed45e590c480ed707aa1adaaba8b84b10747f956bd431c708bd590e` |
| D2 | CDISC ADaM adverse-event NDJSON | 615,350 | `b795a59c5c92a8fc93d7d3f729fa0dae014e9b0f684a6a7a7bcf07e4104ba723` |
| D3 | GH Archive 10 MiB NDJSON | 10,485,760 | `a860af236f794779b5471b416b2e6d7ea68de00f6d17728dd3b088a7ecec8252` |
| D4 | CROVIA royalty receipts NDJSON | 3,585,053 | `0db7ace9e46ce458a7055f48b09aabc7df15f4f74a23553d3660b7de61d0916f` |

These objects are known discovery/regression evidence, not held-out evidence.

### 5.2 Known stress

V1 is frozen Sino-US DrugQA:

- bytes: 15,374,047;
- git blob: `8d9e6acf3f534194487532fa60d4f1d43fe71d00`;
- SHA-256: `757b9bc7e5ee2d38ab5ed43877b1d87809cb5131621d106671bc27996da1c499`.

V1 is reported only as:

> `KNOWN-STRESS / NOT-HELD-OUT FOR G5D`

V1 cannot satisfy a promotion or generalization gate. Its result must not tune
page policies, root/overlay caps, admission rules, ordering, thresholds, or
implementation after outcomes are observed.

### 5.3 Held-out

This workflow opens no new held-out corpus.

After `PASS-G5D-DISCOVERY`, a separate preregistration must freeze at least:

- two previously unseen dictionary-relevant structured objects, including one
  with regional dictionary drift or high-cardinality values;
- one previously unseen negative-control object outside the target family;
- exact repository commits, paths, Git blobs, sizes, and SHA-256 values;
- the implementation SHA;
- all thresholds and artifact schema.

No held-out object may be fetched, previewed, or substituted before its
preregistration is frozen.

---

## 6. Frozen page policies

G5D evaluates exactly three fixed policies:

| Policy ID | Values per page |
|---:|---:|
| `P4K` | 4,096 |
| `P16K` | 16,384 |
| `P64K` | 65,536 |

Page boundaries are fixed by occurrence count within one exact shape-slot
column. Every page except the last contains exactly the policy size. The last
page contains the remaining occurrences and may be empty only when the column
has zero occurrences, which is not a dictionary-eligible leaf.

No adaptive page size, content-defined page boundary, cross-column page, trie
page, or parent pointer is legal in G5D.

The page-policy ID is transmitted once per dictionary leaf and charged. Page
identity during decoding is implicit from the occurrence counter; no per-token
page ID is transmitted or charged.

---

## 7. Dictionary model

### 7.1 Scope

The first G5D version has one dictionary namespace per exact shape-slot leaf.
It does not share a root across leaves. The root is amortized across all pages
and occurrences of that leaf; a cross-leaf shared root is a later, separate
hypothesis.

Every dictionary and escaped value is exact source-token bytes. No semantic
normalization is legal.

### 7.2 Root table

The root contains exact tokens selected from the entire leaf occurrence stream.

The candidate root-cap set is fixed:

```text
{0, 16, 64, 256, 1024, 4096, 16384, 65536}
```

The cap is clamped to the number of distinct tokens.

For a given cap, entries are ordered by:

1. descending occurrence count in the leaf;
2. ascending exact byte lexicographic order on ties.

The selected root count is transmitted. Counts and rank numbers are not
transmitted because the decoder needs only the resulting ordered byte table.
The ordering policy is fixed by this version.

### 7.3 Page overlay

For each page, a token not present in the root may enter that page's overlay.

The candidate overlay-cap set is the same fixed set:

```text
{0, 16, 64, 256, 1024, 4096, 16384, 65536}
```

The cap is clamped to the number of distinct non-root tokens in that page.

For a given cap, overlay entries are ordered by:

1. descending occurrence count within the page;
2. ascending exact byte lexicographic order on ties.

An overlay entry must be an exact token from that page. It must not duplicate a
root entry or another overlay entry in the same page.

### 7.4 Mandatory escape

Every page reserves one entry code after all root and overlay entries:

```text
root IDs:    [0, root_count)
overlay IDs: [root_count, root_count + overlay_count)
ESC:         root_count + overlay_count
```

An escaped occurrence is encoded as:

```text
exact_token_length canonical-uvar
exact_token_bytes
```

An escaped token must be non-empty under the frozen G3 scalar-token contract.

The escape is mandatory in every dictionary arm, including a page with zero
root and zero overlay entries. Width-zero pages emit only the zero-valued escape
code and therefore carry no packed ID bits.

There is no no-escape candidate in the primary G5D arms.

### 7.5 Root and overlay selection

Selection is deterministic and encoder-side. For every candidate root cap and
page policy:

1. construct the frequency-ordered root table;
2. for each page independently evaluate every candidate overlay cap using the
   exact serialized G5D leaf cost defined in section 8;
3. choose the overlay cap with the smallest exact serialized leaf cost;
4. deterministic ties choose the smaller cap;
5. evaluate the complete root-cap candidates and choose the smallest exact
   serialized leaf cost;
6. deterministic root-cap ties choose the smaller cap.

This MDL selector is not assumed to predict q11. The workflow emits a complete
candidate for every fixed page policy and arbitrates those complete carriers
with the same q11 backend. G4's failed cheap-score result remains controlling.

---

## 8. Leaf wire contract

All integers are canonical unsigned varints unless stated otherwise.

Conceptual payload for one shape-slot leaf:

```text
leaf_payload_length

dictionary_id uvar
page_policy_id u8

root_count uvar
for root entry:
    exact_length uvar
    exact_bytes

for each page:
    overlay_count uvar
    for overlay entry:
        exact_length uvar
        exact_bytes
    code_width u8
    packed_entry_ids for every occurrence in this page
    for each ESC encountered in occurrence order:
        escaped_length uvar
        escaped_bytes
```

The number of pages is derived from the frozen occurrence count and transmitted
page policy and is checked against exact payload consumption; it is not
redundantly transmitted.

For page `p`:

```text
active_count = root_count + overlay_count
code_width = ceil(log2(active_count + 1))
```

Width zero is valid only when `active_count == 0`.

The packed ID bit order is deterministic little-endian bit order. Unused high
bits in the final byte must be zero.

The outer research carrier retains the frozen G3 magic/version change, source
length, frame/group tables, exact templates, residual bytes, leaf operator ID,
and leaf payload length. Every such byte is charged.

---

## 9. Complete MDL and wire accounting

For a G5D candidate, the exact pre-backend description is:

```text
L_total =
    L_outer_research_envelope
  + L_common_structure_metadata
  + L_raw_residual_bytes
  + L_leaf_operator_ids
  + L_leaf_payload_lengths
  + L_dictionary_ids
  + L_page_policy_ids
  + L_root_counts_and_entries
  + L_overlay_counts_and_entries
  + L_code_widths
  + L_packed_entry_ids
  + L_escape_lengths
  + L_escape_payloads
  + L_frequency_model
  + L_default_exception
  + L_fallback_description
  + L_backend_payload
  + L_integrity_bytes.
```

For G5D:

```text
L_frequency_model = 0
L_default_exception = 0
```

unless a byte is required by the common frozen research envelope and identified
in the component breakdown.

Frequency rank is represented by transmitted table order. No separate frequency
array is transmitted. Brotli's learned model is charged only through its final
compressed payload and is not double-counted as a free side model.

For page `p`:

```text
w_p = ceil(log2(root_count + overlay_count_p + 1))
L_entry_ids = sum_p occurrence_count_p * w_p bits.
```

The final evidence value is complete bytes:

```text
C_G5D =
    charged outer research envelope
  + Brotli_q11_lgwin30(complete G5D carrier).
```

The component breakdown must sum exactly to the uncompressed carrier body.
`other_charged_bytes` is permitted only if it is explicitly emitted and
reconciles the sum; it cannot hide dictionary, detection, escape, or fallback
bytes.

### 9.1 Detection

Charge:

- the new leaf operator ID;
- the dictionary ID;
- the page-policy ID;
- all counts and lengths required to identify pages/entries;
- any per-leaf raw fallback mode.

Encoder compute is not a wire cost, but construction, root/overlay selection,
and q11 evaluation are measured encode-throughput axes.

No learned detector is legal. Therefore no detector model bytes are permitted
in G5D.

### 9.2 Table IDs

`dictionary_id` is charged once per dictionary leaf even when an implementation
might derive it from leaf order.

No per-token table ID is charged because the active page is implicit from the
occurrence counter. If a future cross-leaf/interleaved implementation requires
per-token table IDs, those bytes must be added and the experiment rerun; they
cannot be treated as free.

### 9.3 Fallback and regret

The routed portfolio is:

```text
C_selected = min(C_raw, C_frozen_G3_selected, all C_G5D)
```

Define:

```text
R_fallback = C_selected - min(C_raw, C_frozen_G3_selected, all C_G5D).
```

Exact arbitration requires `R_fallback == 0`. A positive value is
`INVALID-G5D`.

For a cheap-selector diagnostic:

```text
R_MDL = C_MDL_selected - C_bounded_q11_oracle_selected.
```

`R_MDL` is reported but is not allowed to replace complete q11 bytes.

---

## 10. Decoder and fast-path requirements

The decoder must build a direct lookup table of `(offset, length)` for root and
current-page overlay entries.

Required properties:

1. one operator dispatch per leaf, not per token;
2. one bit-reader state per homogeneous dictionary leaf;
3. no generic virtual call in the token loop;
4. no pointer chasing through a trie;
5. root and overlay lookups become direct indexed loads;
6. a predictable cold branch handles ESC;
7. page transitions are sequential;
8. control/data separation permits preparsing descriptors before bulk decode;
9. exact bounds are validated before allocation or expansion.

A split hot-ID/escape stream, alternate entropy decoder, SIMD transform, or
specialized page replacement is outside G5D. Decode speed is a measured gate, not
permission to add another mechanism.

---

## 11. Required arms

### 11.1 External controls

For every D1-D4 and V1 object, independently run and archive:

- `RAW_BROTLI`;
- frozen G2 whole-object portfolio where available;
- frozen `G3_REGION_RAW`;
- frozen `G3_REGION_DICT`;
- frozen `G3_REGION_INT`;
- frozen `G3_REGION_MIXED`.

Controls must come from binaries built in the same remote run and environment.
Historical numbers may be context but cannot replace reproduced controls.

### 11.2 Same-transform controls

For each order, emit:

- `G5D_RAWLEX_SOURCE_ORDER`;
- `G5D_RAWLEX_SHAPE_COLUMN`.

These use the G5D common carrier and ordering but force exact lexical scalar
bytes. They isolate the cost/value of the new dictionary leaf from outer
carrier and ordering effects.

### 11.3 G5D dictionary arms

For each order:

- `G5D_P4K`;
- `G5D_P16K`;
- `G5D_P64K`.

Every arm includes the mandatory escape.

No page policy, root cap, overlay cap, or order may be selected after observing
the final corpus result outside the exact complete-carrier arbitration above.

---

## 12. Required metrics

### 12.1 Identity and correctness

- implementation commit, source blob, source SHA-256;
- preregistration blob and SHA-256;
- frozen G3 source SHA, blob, and SHA-256;
- G5A source-of-record SHA, blob, and SHA-256;
- compiler, flags, OS, architecture, runner, CPU, and core allocation;
- Brotli implementation version and package identity;
- quality 11 and lgwin 30;
- corpus role, bytes, Git blob, and SHA-256.

### 12.2 Per dictionary arm

- source bytes;
- carrier body bytes;
- outer envelope bytes;
- backend payload bytes;
- complete bytes;
- leaf count;
- root entry count and bytes;
- overlay page count;
- overlay entry count and bytes;
- packed-ID bytes;
- escape count, length bytes, and payload bytes;
- common structure metadata bytes;
- raw residual bytes;
- component-sum reconciliation;
- dictionary-plan SHA-256;
- table-content SHA-256;
- original-token multiset SHA-256;
- order-specific token-stream SHA-256;
- build, encode, and decode time;
- dedicated decode peak RSS when the performance gate is run;
- exact round trip;
- pack/unpack success;
- malformed-input self-test result.

### 12.3 Aggregate

- sum of current G3 selected complete bytes;
- sum of current `G3_REGION_DICT` complete bytes;
- sum of G5D arms by order and page policy;
- sum of routed G5D portfolios by order;
- percentage change and absolute bytes saved;
- files improved versus same-transform/current dictionary controls;
- order effect with dictionary plan held fixed;
- MDL selector regret versus bounded complete q11 oracle;
- fallback regret, which must be zero;
- V1 known-stress result reported separately.

Timing is diagnostic in the discovery workflow and is not accepted as a
cross-machine Pareto result.

---

## 13. Mandatory malformed-input and correctness gates

The standalone source must pass all self-tests before corpus measurement.

At minimum:

1. all-valid LF NDJSON;
2. all-valid CRLF NDJSON;
3. valid records with a malformed middle frame and valid later frame;
4. valid records with an incomplete final frame;
5. all-raw input;
6. one structured record;
7. mixed exact shapes;
8. one-occurrence columns;
9. constant columns;
10. all-distinct columns;
11. zero root and nonzero overlay;
12. nonzero root and zero overlay;
13. zero root and zero overlay, requiring only escape;
14. page-boundary values immediately below, at, and above 4,096;
15. page-boundary values immediately below, at, and above 16,384;
16. page-boundary values immediately below, at, and above 65,536;
17. root/local duplicate token rejection;
18. duplicate entries within a table rejection;
19. empty table entries;
20. invalid page-policy ID;
21. root count greater than occurrence count;
22. overlay count greater than page occurrence count;
23. entry not present in the declared page;
24. invalid code width;
25. code equal to or above the active alphabet;
26. nonzero unused high bits;
27. truncated root table;
28. truncated overlay table;
29. truncated packed IDs;
30. truncated escape length;
31. truncated escape payload;
32. escape length zero or exceeding source bounds;
33. noncanonical varints;
34. count/length arithmetic overflow;
35. impossible reconstruction-order stream;
36. impossible group/occurrence consumption;
37. trailing carrier bytes;
38. bad order selector;
39. order pack/unpack mismatch;
40. source hash/token multiset mismatch;
41. malformed dictionary plan/table identity mismatch between orders;
42. adversarial round-trip fuzzing with fixed seeds.

Any failure yields `INVALID-G5D`. No size result from an invalid run may be
used.

---

## 14. Discovery gate

All conditions are required.

### 14.1 Validity

1. all source and corpus identities match;
2. G5A source-of-record identity is pinned;
3. frozen G3 source identity matches;
4. every self-test and malformed-input gate passes;
5. every emitted corpus arm round-trips exactly;
6. both orders and all three page policies are present;
7. all MDL components reconcile to the carrier body;
8. all required artifacts and hashes are emitted.

### 14.2 Routed ratio effect

For at least one order:

```text
sum_discovery(min(current_G3_selected, all G5D arms))
<= 0.995 * sum_discovery(current_G3_selected)
```

This requires at least **0.5% aggregate complete-byte improvement** over the
reproduced current G3 routed portfolio.

### 14.3 Same-transform dictionary effect

For the same order:

```text
sum_discovery(best_complete_G5D_dictionary_arm)
<= 0.995 * sum_discovery(G3_REGION_DICT)
```

and the G5D dictionary arm must be strictly smaller than
`G3_REGION_DICT` on at least **2 of 4** D1-D4 objects.

This prevents a win caused only by outer-carrier, shape, or ordering changes.

### 14.4 Ordering attribution

The source/column pair must use identical dictionary plans and table content.
The workflow reports:

```text
order_bytes = SOURCE_ORDER_complete - SHAPE_COLUMN_complete
```

The dictionary mechanism is credited only from dictionary versus same-order
RAW_LEX/current-dictionary controls, never from this order difference.

### 14.5 V1

V1 is reported with its known-stress label and has no pass/fail weight.

If any condition passes:

> `PASS-G5D-DISCOVERY`

Otherwise:

> `NO-GO-G5D-DISCOVERY`

Invalid evidence produces:

> `INVALID-G5D`

No threshold, page policy, order, or corpus may be changed after outcomes are
observed.

---

## 15. Performance gate after discovery

`PASS-G5D-DISCOVERY` authorizes a separately frozen remote micro, not held-out
opening by itself.

The performance micro must use the same runner class, pinned binaries,
alternating arm order, warmups, and median-of-at-least-nine measurements. It
must report complete bytes, encode throughput, dictionary reconstruction decode
throughput, end-to-end decode throughput, and peak RSS.

Promotion requires either:

### Ratio path

- complete bytes no worse than the current routed portfolio;
- dictionary reconstruction decode throughput at least **95%** of frozen
  current `G3_REGION_DICT`;
- end-to-end decode throughput at least **95%** of current routed decode;
- peak RSS at most **110%** of current routed decode;
- end-to-end encode time at most **2x** the frozen G5D control path unless a
  separately reported offline-only timing is used.

or:

### Speed path

- at least **10% faster dictionary reconstruction decode** than the current
  exact-dictionary path;
- complete-byte loss no greater than **0.25%** versus current routed bytes;
- peak RSS no greater than **110%**;
- exact round trips and malformed-input gates unchanged.

Timing from the discovery workflow cannot satisfy this gate.

---

## 16. Held-out gate before any integration proposal

The held-out preregistration must freeze at least two target objects and one
negative control before fetching them.

Integration may be proposed only if:

1. every held-out object and arm is exact and correctly identified;
2. the routed G5D portfolio never regresses versus raw on any held-out object;
3. the aggregate routed G5D portfolio is at least **1% smaller than raw
   Brotli**;
4. at least **2 of 3** held-out objects are at least **1% smaller than raw
   Brotli**;
5. the same performance gate passes on held-out objects;
6. V1 remains excluded from held-out counts;
7. all negative, invalid, and null-order results are preserved.

Passing these gates permits a separate integration proposal. It does not itself
edit production source or allocate a transform ID.

---

## 17. Artifact schema

The workflow must upload at least:

```text
results/discovery-provenance.json
results/g2-D*.json
results/g3-D*.json
results/g5d-D*.json
results/g3-V1*.json
results/g5d-V1*.json
results/g5d-ruling.json
results/g5d-ruling.md
results/artifact-manifest.json
```

Every JSON file is valid UTF-8 JSON, not a hand-edited log.

### 17.1 Per-file G5D row

Required top-level keys:

```json
{
  "schema": 1,
  "experiment": "G5D-PAGED-DICTIONARY",
  "file": "...",
  "file_role": "discovery-or-known-stress",
  "source_bytes": 0,
  "source_sha256": "...",
  "implementation_sha": "...",
  "frozen_g3_sha": "...",
  "g5a_results_sha": "...",
  "g5a_results_sha256": "...",
  "raw_brotli_complete_bytes": 0,
  "page_policies": [4096, 16384, 65536],
  "source_token_multiset_sha256": "...",
  "invariants": {},
  "orders": {
    "SOURCE_ORDER": {},
    "SHAPE_COLUMN": {}
  }
}
```

`invariants` must include boolean evidence for:

- exact round trip of every arm;
- same original token multiset across orders/policies;
- same dictionary plan across orders;
- same table content across orders;
- exact outer-reconstruction order;
- complete component accounting;
- fixed backend;
- mode pack/unpack;
- no excluded family;
- no production authorization.

Each order contains exactly three arms keyed by `4096`, `16384`, and `65536`.

Each arm must contain:

```json
{
  "page_policy": 4096,
  "complete_bytes": 0,
  "outer_envelope_bytes": 0,
  "carrier_body_bytes": 0,
  "brotli_payload_bytes": 0,
  "common_structure_metadata_bytes": 0,
  "residual_bytes": 0,
  "leaf_descriptor_and_detection_bytes": 0,
  "root_table_bytes": 0,
  "overlay_table_bytes": 0,
  "entry_id_bytes": 0,
  "escape_bytes": 0,
  "other_charged_bytes": 0,
  "component_sum_bytes": 0,
  "leaf_count": 0,
  "root_entry_count": 0,
  "overlay_page_count": 0,
  "overlay_entry_count": 0,
  "escape_count": 0,
  "dictionary_plan_sha256": "...",
  "table_content_sha256": "...",
  "token_order_sha256": "...",
  "build_ms": 0.0,
  "encode_ms": 0.0,
  "decode_ms": 0.0,
  "decode_peak_rss_kib": 0,
  "roundtrip": true,
  "pack_unpack_ok": true
}
```

The exact sum identity is:

```text
component_sum_bytes ==
  common_structure_metadata_bytes
  + residual_bytes
  + leaf_descriptor_and_detection_bytes
  + root_table_bytes
  + overlay_table_bytes
  + entry_id_bytes
  + escape_bytes
  + other_charged_bytes
```

and:

```text
complete_bytes == outer_envelope_bytes + brotli_payload_bytes.
```

### 17.2 Ruling object

`g5d-ruling.json` must include:

```text
schema
experiment
classification
invalid_reasons[]
identity{}
g5a_ordering_evidence{}
backend{}
environment{}
corpus[]
thresholds{}
per_file{}
aggregate{
  current_g3_selected_sum
  current_g3_region_dict_sum
  g5d_by_order_and_policy{}
  routed_by_order{}
  fallback_regret_bytes
  mdl_oracle_regret_bytes
}
v1_known_stress{}
performance_gate{
  status
  reasons[]
}
held_out{
  opened
  corpus
}
production{
  transform_id_allocated
  source_changed
}
```

The ruling must distinguish discovery ratio evidence, performance evidence,
held-out evidence, and production authorization. Missing evidence is not a
pass.

---

## 18. Execution discipline

1. Freeze this preregistration before implementation corpus measurement.
2. Implement only the standalone `tools/grotli_g5_paged_dictionary.cpp` and its
   self-tests.
3. Do not modify `src/anvil.cpp`, existing ledgers, G2/G3/G4/G5A sources, or
   unrelated workflows.
4. Local workstation work is limited to source review, warning-clean compile,
   and tiny correctness fixtures.
5. No local corpus compression, benchmark, sweep, fuzz campaign, or statistical
   experiment.
6. Commit the implementation and this preregistration before corpus access.
7. Close and pin the G5A source-of-record result.
8. Dispatch `.github/workflows/anvil-i10-g5-paged-dictionary.yml` manually.
9. The workflow must materialize and verify frozen G3 before building G5D.
10. Preserve all raw JSON rows, logs, identities, timing diagnostics, and the
    machine ruling.
11. Apply the frozen discovery gate mechanically.
12. Do not edit thresholds, page policies, caps, ordering, corpus, or source
    after outcomes are visible.
13. If discovery passes, freeze a separate performance and held-out program.

---

## 19. Prior-art boundary

The following are established lineages and cannot be claimed as G5D novelty:

- exact/dictionary-page and bit-packed dictionary IDs, including Parquet-style
  column-chunk dictionaries;
- Brotli static dictionaries and trained/external Zstd dictionaries;
- local/page dictionaries and adaptive block layouts;
- RLE/default and sparse exception representations;
- front/prefix coding;
- FSST 1-8 byte symbols and escapes;
- BPE/subword and frequency-ordered tokenization;
- grammar/structural token coding;
- JSON schema inference, columnization, schema union, and shape hierarchy.

DataCortex, CLP, LogPrism, BtrBlocks, FastLanes, ALP, FSST, Parquet, Brotli, and
historical columnar encodings are close lineage. G5D claims no primitive
novelty. A future paper may discuss a compiler-level system only if the exact
causal accounting, fallback, bounded-memory decode, and held-out Pareto evidence
are independently established.

---

## 20. Final disposition rule

Regardless of the discovery classification:

- raw Brotli remains a permanent candidate;
- no production source is changed;
- no production transform ID is allocated;
- V1 remains known stress;
- invalid evidence is quarantined;
- a negative G5D result closes this exact root-plus-overlay-page formulation;
- it does not authorize a trie, defaults, FSST, front coding, hierarchy, or
  BPE in the same experiment.
