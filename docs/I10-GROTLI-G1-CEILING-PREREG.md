# ANVIL I10 — GROTLI G1-CEILING Pre-Registration

**Frozen:** 2026-09-23
**Class:** representation-ceiling / engineering experiment
**Novelty claim:** none
**Production source changes:** forbidden until this gate closes
**Compute policy:** real corpus compression runs on GitHub Actions
**Predecessor:** `I10-GROTLI-G0-RESULTS.md`
**Corpus freeze:** `I10-GROTLI-G1-CEILING-CORPUS-FREEZE.md`

---

## 0. Why G1 exists

G0 decisively falsified one shortcut:

> blind padded row-position vXOR is not a useful representation for the tested
> real NDJSON families.

That result does **not** test the broader Grotli/SRS thesis.

The stronger thesis is:

> repeated structured records should be decomposed into a compact
> decoder-visible structural explanation plus homogeneous raw value streams,
> then scored by the actual downstream backend.

G1 asks whether that representation layer itself has enough complete-byte value
to justify automatic structure discovery and typed leaves later.

This is deliberately an **oracle ceiling**:

- input is known to be NDJSON/JSONL;
- a full byte-exact lexical parser is allowed;
- encoder cost may be high;
- no production parser/router is implied.

If the ceiling itself cannot beat raw Brotli, the broad JSON/NDJSON SRS lane
does not receive a large implementation budget.

---

## 1. Primary causal question

For source X and the exact same Brotli q11/lgwin30 leaf:

    B0 = complete_bytes(Brotli_q11_lw30(X))
    B1 = complete_bytes(Brotli_q11_lw30(SHAPE_ROW(X)))
    B2 = complete_bytes(Brotli_q11_lw30(SHAPE_COLUMN(X)))

Does either exact structured representation materially beat B0 on independent
real structured families after **all** representation metadata is charged?

No typed value codec is allowed in G1.

Therefore any win is attributable to:

- repeated structural-template elimination;
- shape coding;
- raw lexical value locality;
- not integer delta, dictionaries, Gorilla, ALP, FSST, or replay.

---

## 2. Input/framing contract

G1 accepts newline-delimited JSON records.

Record framing is byte exact:

- LF terminates a record and belongs to that record;
- CR before LF is ordinary source data and is preserved;
- a non-empty final unterminated suffix is one final record;
- no final empty record is synthesized;
- blank records or invalid JSON make the structured candidate unavailable.

For G1-v1:

> **any invalid or blank record makes both structured candidates unavailable for
> the whole file.**

Raw Brotli remains available.

This prevents an unregistered residual side path from becoming another source of
compression behavior.

---

## 3. Byte-exact lexical parse

The encoder parses JSON syntax but never normalizes it.

It must preserve exactly:

- object key spelling and escaping;
- object key order;
- duplicate keys;
- whitespace;
- punctuation;
- string escape spelling;
- UTF-8 bytes;
- numeric lexical spelling;
- negative zero;
- exponent spelling;
- CRLF versus LF;
- missing final LF.

A semantic parse followed by ordinary JSON serialization is **not** acceptable.

The decoder never parses JSON. It reconstructs bytes from the serialized
template/value representation.

### 3.1 Object keys

JSON strings used as object keys are **structural bytes**.

They remain in the template.

### 3.2 Scalar values

Every scalar value token is a placeholder:

- string value, including surrounding quotes and original escapes;
- number token;
- `true`;
- `false`;
- `null`.

The complete raw lexical token is stored as the placeholder value.

### 3.3 Containers

Objects and arrays are not themselves placeholder values.

Their braces/brackets, commas, colons, keys and whitespace remain structural;
scalar leaves recurse into placeholders.

### 3.4 Exact grammar requirement

The lexical parser must reject malformed JSON rather than attempting recovery.

At minimum validate:

- object/array grammar;
- required colon/comma placement;
- valid JSON number grammar;
- legal string escapes;
- four hex digits after `\u`;
- no unescaped control byte below 0x20 inside strings;
- exactly one JSON value per record followed only by JSON whitespace and the
  optional record LF already belonging to the record.

UTF-8 semantic normalization is forbidden. The source bytes are preserved.

---

## 4. Shape identity

Represent a record with N scalar values as:

    part_0
    value_0
    part_1
    value_1
    ...
    value_(N-1)
    part_N

where every part is the exact byte interval between extracted scalar tokens.

Two records have the same G1 shape iff:

- they have the same N;
- every corresponding `part_i` byte string is identical.

Scalar lexical type is **not** part of shape identity in G1.

This is intentional. G1 tests compression structure, not application semantics.

If the same structural position sometimes contains a number and sometimes a
string, the raw lexical value bytes still reconstruct exactly. A later typed
experiment may decide that type splitting is worthwhile.

Shape IDs are assigned deterministically in first-occurrence order.

---

## 5. Shape dictionary

For every shape serialize:

    occurrence_count
    placeholder_count

    repeat placeholder_count + 1:
        part_length
        part_bytes

All integer fields use canonical unsigned varints.

All template bytes are charged.

No schema is assumed preinstalled.

---

## 6. Record-order stream

Serialize one shape ID for every source record in original order.

This stream is required even when value payloads are grouped by shape.

The decoder uses the shape-ID stream to choose which shape/value cursor to
advance for each reconstructed record.

This avoids per-group absolute row-index tables.

---

## 7. Candidate S1 — SHAPE_ROW

S1 isolates **structural-template deduplication**.

After the shape dictionary and record-order stream, serialize one value payload
per shape.

Within each shape:

    for each occurrence in source order:
        for each placeholder in record order:
            value_length
            raw_value_bytes

Values from the same record remain adjacent.

Nothing about scalar values is transformed.

S1 asks:

> If repeated JSON scaffolding is described once but value locality remains
> record-oriented, does Brotli gain?

---

## 8. Candidate S2 — SHAPE_COLUMN

S2 uses the exact same parser, shape dictionary and record-order stream.

Only value ordering changes.

Within each shape:

    for placeholder j in [0, N):
        for each occurrence in source order:
            value_length
            raw_value_bytes

Corresponding raw lexical values become adjacent.

Nothing about those values is otherwise transformed.

S2 asks:

> Does homogeneous field-position locality create additional backend value over
> template dedup alone?

---

## 9. Why both S1 and S2 are mandatory

A single "columnar" result would confound two effects:

1. removing repeated structure;
2. reordering values.

Interpretation:

- S1 wins, S2 similar/worse:
  template deduplication is valuable; columnization is not broadly valuable.
- S1 loses, S2 wins:
  field-position locality is the real gain.
- both win:
  both effects contribute.
- both lose:
  raw Brotli already exploits the structure better than this decomposition.

No typed codec may rescue a losing S1/S2 result inside G1.

---

## 10. Frozen logical carrier

Both structured candidates use the same logical header:

    magic
    version
    layout_id            // ROW or COLUMN
    decoded_source_len
    record_count
    shape_count

    shape_dictionary
    shape_id_stream
    value_streams

Decoder validation requires:

- exact canonical varints;
- bounded counts before allocation;
- shape IDs < shape_count;
- placeholder count consistency;
- occurrence counts equal shape-ID frequencies;
- value counts exactly match expected shape occurrences;
- sum of reconstructed record bytes equals `decoded_source_len`;
- exact carrier consumption;
- no trailing bytes.

The carrier itself is then compressed by Brotli q11/lgwin30.

---

## 11. Complete-byte accounting

Report both backend payload and common prototype-envelope bytes.

The comparison must charge:

- carrier magic/version/layout;
- source length;
- record count;
- shape count;
- every template byte;
- occurrence counts;
- every shape ID;
- every value length;
- every raw value byte;
- Brotli payload;
- any candidate-specific selector/envelope bytes.

No schema, dictionary, model or type information is free.

The primary ruling uses complete candidate bytes.

---

## 12. Raw control

Raw control:

    Brotli_q11_lw30(original source bytes)

using the same Brotli implementation/version and large-window settings as S1/S2.

The selected portfolio row is:

    min(RAW, SHAPE_ROW, SHAPE_COLUMN)

Raw wins exact ties.

A structured loss is a scientific result, not a CI failure.

---

## 13. Required diagnostics

For every file report:

### Source anatomy

- source bytes;
- source SHA-256;
- record count;
- valid-record count;
- scalar token count;
- scalar lexical bytes;
- structural bytes.

### Shape anatomy

- shape count;
- singleton shape count;
- rows covered by top 1/5/10/100 shapes;
- median/p95/max occurrences per shape;
- shape dictionary bytes;
- shape-ID bytes before Brotli;
- average placeholders/record;
- gross repeated-template bytes avoided before metadata.

### S1/S2 carrier

- carrier bytes before Brotli;
- header bytes;
- template bytes;
- shape-ID bytes;
- value-length bytes;
- raw value bytes;
- Brotli payload bytes;
- complete bytes;
- delta versus raw Brotli;
- exact SHA roundtrip.

Timing/RSS are diagnostic in this ceiling gate, not the primary representation
ruling. If a structured representation passes bytes, it receives a separate
paired timing/RSS promotion lane.

---

## 14. No hidden schema semantics

G1 does not:

- merge fields across different shapes by semantic key;
- canonicalize key order;
- normalize strings or numbers;
- infer integer/float/timestamp types;
- dictionary-code values;
- delta numbers;
- bitpack values;
- apply Gorilla/ALP/FSST;
- infer cross-column predictors;
- use replay.

Those are separate experiments.

---

## 15. Search budget

There is no combinatorial representation search in G1.

Exactly three complete candidates exist:

1. RAW;
2. SHAPE_ROW;
3. SHAPE_COLUMN.

This is intentional.

G1 measures whether the base representation layer has value before ANVIL pays
the Grotli-v4-style planner/search cost.

---

## 16. Corpus split

Frozen manifest:

`I10-GROTLI-G1-CEILING-CORPUS-FREEZE.md`

Discovery:

- D1 Amazon cellphone NDJSON;
- D2 CDISC ADaM NDJSON;
- D3 GH Archive 10 MiB NDJSON;
- D4 CROVIA DPI royalty receipts NDJSON.

Held-out validation:

- V1 Sino-US DrugQA authoritative bilingual release.

D1-D3 are already open from G0.

D4 is new discovery data.

V1 remains unopened for G1 compression results until the discovery gate passes
and the implementation SHA is frozen.

---

## 17. Discovery gate

### PASS-G1-DISCOVERY

Discovery passes only when all are true:

1. RAW/S1/S2 roundtrip exactly for every eligible discovery file;
2. malformed/truncated/trailing carrier tests pass;
3. at least **two of four** discovery families have S1 or S2 at least **5%**
   smaller than raw Brotli complete bytes;
4. aggregate discovery selected bytes improve by at least **3%** versus aggregate
   raw Brotli complete bytes;
5. all representation metadata is charged.

Only PASS-G1-DISCOVERY authorizes opening V1.

### NO-GO-G1-DISCOVERY

If discovery does not pass:

- do **not** fetch/measure V1;
- close G1 base representation as NO-GO on discovery;
- retain V1 as unspent validation data;
- typed leaves require a new preregistration.

This is deliberately stricter than burning held-out data after a discovery
failure.

---

## 18. Held-out validation gate

The validation job must checkout the exact frozen implementation SHA emitted by
the discovery job.

No source change is allowed between discovery and validation.

### PASS-G1-BASE

If discovery passed, G1 base representation passes when:

1. V1 RAW/S1/S2 roundtrip exactly;
2. selected V1 complete bytes improve by at least **1%** versus raw Brotli;
3. metadata remains fully charged;
4. validation uses the same carrier semantics and Brotli helper as discovery.

This authorizes:

- production-oriented SRS prototype work;
- a separately preregistered typed-leaf ladder;
- automatic/weak-schema structure discovery research.

It does **not** establish mechanism novelty.

### PASS-G1-NARROW

Discovery passes but V1 does not retain the 1% win.

Interpretation:

- structure separation has real but narrow/class-dependent value;
- retain winning families as adopt evidence;
- do not call the representation a broad JSON/NDJSON base;
- inspect anatomy before deciding on any class-specific route.

### NO-GO-G1-BASE

If the discovery gate fails, or if no material representation signal survives
after complete accounting, do not build a broad SRS base around shape/template
separation.

This does not falsify type-specific numeric/string experts.

---

## 19. Typed leaves are explicitly deferred

No C1/C2/etc typed leaf is allowed inside G1 after seeing G1 outcomes.

A typed follow-up gets a new preregistration and must state which independent
evidence justifies it.

Potential future leaves include:

- exact-token dictionary/enum;
- integer FOR/delta/DoD;
- null/default bitmap;
- FSST-like string symbolization;
- Gorilla/ALP-like exact float representation.

This separation prevents outcome-driven mechanism accretion.

---

## 20. Prior-art boundary

G1 is **not** a mechanism novelty experiment.

Close/established work includes:

- DataCortex JSON/NDJSON schema inference, selective columnization, typed
  encodings and zstd/Brotli arbitration;
- CLP structured log decomposition;
- LogPrism compression-aware joint structure/variable modeling;
- BtrBlocks adaptive typed scheme selection;
- FastLanes encoding-expression selection;
- ALP adaptive exact numeric representation.

A G1 win supports **adoption/architecture viability**, not novelty.

ANVIL's broader open research remains above this layer:

- weak-schema/arbitrary-byte explanation discovery;
- predictor/explanation synthesis;
- reference/replay/generative experts;
- marginal expert basis selection;
- hardware-aware lowering;
- full byte/decode/RSS/code-size Pareto extraction.

---

## 21. Implementation order

1. freeze this preregistration and the G1 corpus manifest;
2. implement byte-exact lexical parser and roundtrip-only tests;
3. implement S1/S2 logical carriers;
4. implement malformed/truncated/trailing carrier tests;
5. run D1-D4 discovery on GitHub Actions;
6. freeze implementation SHA and discovery ruling;
7. **only if PASS-G1-DISCOVERY**, fetch/open V1 once;
8. close G1 before any production ANVIL integration.

No `src/anvil.cpp` changes before this gate closes.
