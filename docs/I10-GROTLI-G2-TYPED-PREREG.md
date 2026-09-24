# ANVIL I10 — GROTLI G2 Typed Column Expert Pre-Registration

**Frozen:** 2026-09-24
**Class:** typed representation / planner experiment
**Novelty claim:** none
**Production source changes:** forbidden until this gate closes
**Compute policy:** compression search runs on GitHub Actions
**Predecessor:** I10-GROTLI-G1-RESULTS.md
**Corpus freeze:** I10-GROTLI-G2-CORPUS-FREEZE.md

---

## 0. Why G2 exists

G1 established three facts that justify one new causal experiment.

1. Column locality can make the same Brotli backend materially smaller:
   D4 SHAPE_COLUMN is 21.7717% below raw Brotli.
2. The effect is not universal:
   D2 loses 2.4971% even after its pre-Brotli carrier shrinks 10.5225%.
3. Historical Grotli and mature typed-column systems independently show that
   type-specific value representations can expose additional structure.

G2 asks:

> Can a deliberately small basis of typed column leaves broaden the measured
> G1 column-locality expert, while final whole-carrier Brotli bytes still make
> every real decision?

G2 is not allowed to add arbitrary transforms until something wins.

---

## 1. What remains frozen from G1

G2 reuses the exact G1 lexical/structural semantics.

Unchanged:

- LF-inclusive record framing;
- exact JSON lexical parser;
- exact object-key bytes remain structural;
- scalar lexical tokens are the reconstructable value units;
- exact shape identity is defined by byte-identical inter-value parts;
- first-occurrence deterministic shape IDs;
- shape dictionary;
- record-order shape-ID stream;
- raw Brotli q11/lgwin30 fallback;
- raw wins exact ties;
- invalid/incomplete structured input makes the expert unavailable rather than
  changing the raw bytes.

G2 does not normalize JSON.

The decoder reconstructs the exact original bytes.

---

## 2. Primary causal questions

G2 has two distinct questions.

### Q1 — operator value

Do known cheap typed leaves produce lower complete whole-carrier Brotli bytes
than the G1 raw lexical column representation?

### Q2 — planner value

Can a cheap local selector recover most of the value of a bounded exact
whole-carrier marginal search?

These questions must remain separate.

A typed operator can be valuable even if the cheap planner misses it.

A planner can appear good only because no typed operator has value.

---

## 3. Frozen typed leaf basis

Exactly five logical leaf families are allowed in G2.

### L0 — RAW_LEX

G1 column baseline.

For one shape/placeholder column, in occurrence order:

    for each token:
        token_length uvar
        exact lexical token bytes

This is always available.

### L1 — EXACT_DICT

Purpose:

- low-cardinality strings;
- booleans/null;
- enums;
- repeated exact numeric spellings;
- repeated arbitrary lexical tokens.

Eligibility:

- at least one value;
- dictionary entry count <= occurrence count;
- every dictionary entry is an exact source token.

Wire:

    dict_count uvar
    for each dictionary token in first-occurrence order:
        token_length uvar
        token_bytes
    id_bit_width u8
    packed_ids

IDs are zero-based dictionary indexes.

Width:

    ceil(log2(dict_count))

with width 0 for dict_count == 1.

Packed IDs have deterministic little-endian bit order.

No semantic normalization or token rewriting is permitted.

### L2 — INT_FOR

Eligibility requires **every** token in the column to be an exact canonical
base-10 signed integer whose value fits int64.

Canonical test is strict:

- grammar: -?(0|[1-9][0-9]*)
- token "-0" is excluded;
- no decimal point;
- no exponent;
- no leading plus;
- no leading zeros;
- parsing to int64 and formatting the int64 back to base-10 must reproduce the
  exact original token bytes.

Wire:

    base zigzag-uvar
    bit_width u8
    packed_residuals

where:

    base = min(values)
    residual_i = unsigned(value_i - base)

computed with a wider intermediate so the full int64 range is handled safely.

Width 0 is valid for a constant column.

### L3 — INT_DELTA_FOR

Eligibility:

- same canonical int64 requirement;
- at least two values;
- every first difference fits int64.

Wire:

    first_value zigzag-uvar
    min_delta zigzag-uvar
    bit_width u8
    packed(delta_i - min_delta), i >= 1

All arithmetic uses wider intermediates and must reject rather than overflow.

### L4 — INT_DOD_FOR

Eligibility:

- same canonical int64 requirement;
- at least three values;
- first delta fits int64;
- every second difference fits int64.

Wire:

    first_value zigzag-uvar
    first_delta zigzag-uvar
    min_dod zigzag-uvar
    bit_width u8
    packed(dod_i - min_dod), i >= 2

Again, wider intermediates are mandatory.

---

## 4. Explicitly excluded from G2

Do not add:

- float/Gorilla/ALP;
- FSST or learned substring symbols;
- general byte shuffle/bit planes;
- cross-column prediction;
- global lane transpose;
- replay;
- grammar/program-copy;
- semantic key merging across different exact shapes;
- missing-field shape unification;
- trained dictionaries;
- external schema state;
- neural predictors;
- per-leaf alternate entropy coders;
- RLE as a separate leaf;
- default/null bitmap as a separate leaf.

Why RLE/default/null are excluded:

EXACT_DICT with one/few exact tokens already tests the high-value low-cardinality
case without adding another independent wire family. If dictionary IDs are not
enough, RLE/bitmap can be separately preregistered later.

This keeps G2 attributable.

---

## 5. G2 carrier

G2 retains the G1 outer shape model but gives every shape slot an explicit leaf.

Logical carrier:

    magic/version
    decoded_source_len
    record_count
    shape_count

    shape_dictionary
    shape_id_stream

    for each shape in shape-ID order:
        for each placeholder in placeholder order:
            leaf_id
            leaf_payload_len
            leaf_payload

Every leaf reconstructs exactly the occurrence-count lexical tokens for that
shape slot.

The decoder then replays source record order using the G1 shape-ID stream.

Leaf payload length is charged even though some leaf grammars could be parsed
without it. This simplifies strict bounded decoding and exact consumption.

---

## 6. Mandatory G1 reproduction arm

Before interpreting typed results, G2 must reproduce the G1 SHAPE_COLUMN baseline
under the new leaf carrier.

Arm G1R:

- every leaf is RAW_LEX;
- G2 outer header/leaf descriptors are present.

Because G2 adds explicit per-leaf IDs and payload lengths, G1R may differ
slightly from the old G1 carrier.

Report both:

- historical G1 SHAPE_COLUMN bytes from the frozen result;
- G1R bytes in the G2 carrier.

This isolates carrier-version overhead from typed-leaf value.

If G1R is materially worse, do not hide that cost.

The final portfolio always includes raw Brotli.

---

## 7. Cheap local selector

The first typed planner is deliberately simple and production-shaped.

For every eligible leaf candidate for one column:

1. serialize the complete leaf payload;
2. construct a standalone leaf object containing:
   - leaf ID;
   - occurrence count;
   - payload length;
   - payload;
3. compress that standalone leaf object with the same Brotli q11/lgwin30;
4. choose the candidate with smallest complete standalone Brotli bytes;
5. RAW_LEX wins exact ties.

This creates three global local-planner arms.

### P-DICT

Allowed leaves:

- RAW_LEX
- EXACT_DICT

### P-INT

Allowed leaves:

- RAW_LEX
- INT_FOR
- INT_DELTA_FOR
- INT_DOD_FOR

### P-MIXED

Allowed leaves:

- all G2 leaves.

After per-column selection, serialize the entire G2 carrier and compress it
again with Brotli q11/lgwin30.

The whole-carrier result, not the local score, is the reported archive result.

Local scoring is only planner logic.

---

## 8. Why local Brotli scoring is not sufficient evidence

G1 already proves that pre-backend carrier size can point in the wrong
direction.

Likewise, Brotli(column) is not the same objective as Brotli(whole carrier).

Cross-column/block context can change the answer.

Therefore G2 includes a bounded exact marginal ceiling.

---

## 9. Bounded exact whole-carrier marginal search

Arm P-MARGINAL is an encoder-side ceiling, not a proposed production planner.

Start state:

    all RAW_LEX leaves

Candidate substitutions:

- every eligible non-RAW G2 leaf for every column.

Ranking:

- calculate isolated same-Brotli local delta versus RAW_LEX;
- sort best-first by isolated byte delta;
- deterministic ties:
  1. shape ID;
  2. placeholder ID;
  3. leaf ID.

Search budget:

- retain top **24** not-yet-fixed substitutions per round;
- exact-evaluate each retained substitution by rebuilding the entire carrier and
  running Brotli q11/lgwin30;
- accept the single substitution that yields the smallest complete whole-carrier
  bytes if and only if it improves by at least 1 byte;
- one column can have only one non-RAW leaf;
- repeat for at most **8 accepted substitutions**;
- stop immediately when no evaluated substitution improves the whole carrier.

Maximum exact whole-carrier q11 evaluations per file:

    24 * 8 = 192

plus baseline/final arms.

This is intentionally expensive but bounded.

It asks:

> Is the cheap local selector leaving obvious typed-leaf value on the table?

It does not claim global combinatorial optimality.

---

## 10. Family attribution arms

In addition to P-MIXED and P-MARGINAL, G2 must emit:

- P-DICT;
- P-INT.

This is required even if neither arm is selected by the final portfolio.

Interpretation:

- P-DICT wins, P-INT does not:
  repeated exact tokens are the primary missing representation.
- P-INT wins, P-DICT does not:
  numeric coordinate transforms are the primary missing representation.
- both help:
  typed portfolio is justified.
- only P-MARGINAL helps:
  operators are useful but local planner is inadequate.
- none help:
  do not expand the typed basis merely to rescue the hypothesis.

---

## 11. Final candidate portfolio

For every file report exact complete bytes for:

1. RAW_BROTLI;
2. historical G1 SHAPE_COLUMN where available;
3. G1R;
4. P-DICT;
5. P-INT;
6. P-MIXED;
7. P-MARGINAL.

The selected G2 portfolio row is:

    min(
      RAW_BROTLI,
      G1R,
      P-DICT,
      P-INT,
      P-MIXED,
      P-MARGINAL
    )

Raw Brotli wins exact ties.

Historical G1 bytes are comparison context, not an emitted G2 candidate unless
the new carrier exactly reproduces that wire.

No file can regress in selected bytes because raw Brotli remains available.

---

## 12. Exact decoder contract

Decoder validates before allocating/expanding:

- canonical varints;
- source length bound;
- record/shape counts;
- shape ID range/frequencies;
- placeholder count;
- per-leaf payload length;
- exact leaf payload consumption;
- reconstructed token count equals shape occurrence count;
- dictionary ID range;
- dictionary bit width matches dictionary count;
- bitpacked stream exact bit/byte extent;
- integer arithmetic overflow;
- reconstructed canonical integer spelling;
- final reconstructed source length;
- exact carrier consumption;
- no trailing bytes.

Malformed leaf payloads must reject cleanly.

The decoder does not run the planner.

---

## 13. Integer implementation rules

### ZigZag

Use a defined uint64 mapping for every int64, including INT64_MIN.

Do not rely on signed overflow or implementation-defined right shifts.

### FOR subtraction

Use a signed/unsigned 128-bit intermediate.

The difference between INT64_MAX and INT64_MIN is UINT64_MAX and is valid.

### Delta

A first difference outside int64 makes DELTA and DOD unavailable for the
column. It does not corrupt/fail the whole file.

### DoD

A second difference outside int64 makes DOD unavailable.

### Bitpacking

Deterministic little-endian bit order.

Unused high bits of the final byte must be zero and are validated on decode.

---

## 14. Dictionary implementation rules

Dictionary identity is **exact lexical bytes**.

No string unescaping.

No number normalization.

No semantic type conversion.

First-occurrence dictionary order is frozen.

IDs are fixed-width bitpacked, not varints.

This makes dictionary cost deterministic and easy to decode.

A dictionary candidate may be larger than RAW_LEX before Brotli; the local
selector is allowed to test it because final isolated Brotli bytes determine
local planner choice.

For G2, construct and score EXACT_DICT for **every** eligible column.

Do not prune dictionary candidates from transformed size, distinct ratio, H0, or
any other pre-Brotli proxy. G1 demonstrated that pre-backend size can point in
the wrong direction, so such pruning would confound the operator-value question.

Report dictionary cardinality and raw/dictionary payload sizes as diagnostics,
but use actual isolated q11 bytes for the local planner and actual whole-carrier
q11 bytes for archive rulings.

If later production work needs a cheaper dictionary prefilter, it must be
derived from G2 evidence and evaluated as a separate planner-quality question.

---

## 15. Required diagnostics

### Per file

- source bytes/hash;
- raw Brotli bytes;
- G1 historical row/column bytes if available;
- G1R bytes;
- P-DICT/P-INT/P-MIXED/P-MARGINAL bytes;
- selected arm/delta;
- exact roundtrip for every emitted arm;
- wall time;
- peak RSS.

### Per shape/column

- occurrence count;
- token bytes;
- distinct exact-token count;
- dictionary cardinality ratio;
- canonical-int eligible yes/no;
- min/max int when eligible;
- delta range;
- DoD range;
- RAW leaf bytes;
- DICT/FOR/DELTA/DOD leaf bytes;
- isolated Brotli bytes for every eligible leaf;
- selected local leaf;
- selected marginal leaf if any.

### Search

- total columns;
- typed-eligible columns;
- candidate substitutions;
- isolated q11 leaf evaluations;
- marginal candidates evaluated;
- accepted marginal substitutions;
- exact whole-carrier evaluations;
- best-byte trace after every accepted substitution.

These diagnostics are required to distinguish operator weakness from planner
weakness.

---

## 16. Discovery corpus/routing

Frozen manifest:

I10-GROTLI-G2-CORPUS-FREEZE.md

Structured discovery families:

- D1 Amazon;
- D2 CDISC;
- D4 CROVIA.

Raw-fallback routing control:

- D3 GH Archive frozen 10 MiB object.

Held-out validation:

- V1 Sino-US DrugQA, still unopened.

---

## 17. G2 discovery gate

### PASS-G2-DISCOVERY

All of the following must hold:

1. every emitted candidate roundtrips exactly;
2. malformed/truncated/trailing leaf/carrier tests pass;
3. D3 remains clean raw fallback;
4. at least **two of the three structured discovery families** achieve selected
   G2 complete bytes at least **5% smaller than raw Brotli**;
5. the four-file routed aggregate is at least **3% smaller than raw Brotli**;
6. at least one typed arm (P-DICT, P-INT, P-MIXED, or P-MARGINAL) improves a
   structured family by at least **1% relative to that file's G1R complete
   bytes**;
7. every decoder-visible carrier and leaf byte is charged; no encoder-only
   planner state is treated as decoder-shared side information.

Only PASS-G2-DISCOVERY authorizes opening V1.

### NO-GO-G2-DISCOVERY

If discovery fails:

- do not fetch V1;
- preserve V1 for a future materially different hypothesis;
- report whether the failure is:
  - operator basis;
  - planner;
  - carrier overhead;
  - insufficient prevalence.

Do not add float/string/replay leaves inside G2 after seeing the result.

---

## 18. Held-out gate

If discovery passes, validation checks out the exact frozen G2 implementation
SHA and opens V1 once.

### PASS-G2-BASE

Require:

1. exact roundtrip for every emitted V1 arm;
2. selected V1 bytes at least **1% smaller than raw Brotli**;
3. no representation/search change from discovery;
4. complete metadata accounting.

Also report whether a typed arm is selected.

If V1 passes only because G1R/raw-column layout wins and typed arms do not, that
supports the structured representation but not the typed-basis hypothesis.

### PASS-G2-NARROW

Discovery passes but V1 does not reach -1%.

Interpret as class-specific portfolio value, not broad structured-data
generalization.

---

## 19. Timing/memory status

G2 remains a representation-first gate.

Primary ruling is exact bytes.

However report diagnostic:

- parser/anatomy time;
- candidate construction time;
- local isolated-Brotli planner time;
- exact marginal-search q11 time;
- decode time per final arm;
- peak RSS.

A byte-passing G2 receives a separate promotion lane with:

- production-shaped planner;
- paired decode;
- encode budget;
- code size;
- memory;
- external reference class.

Do not call G2 a full Pareto crossing.

---

## 20. Prior-art boundary

No G2 leaf is a novelty claim.

Close/established lineage includes:

- dictionary encoding;
- frame-of-reference;
- delta/delta-of-delta;
- bitpacking/PFor;
- Gorilla;
- BtrBlocks;
- FastLanes;
- ALP;
- Pcodec;
- FSST;
- DataCortex;
- Dataset-JSON/columnar structured compression.

G2 is an architecture/adoption test.

Its purpose is to learn whether a **small routed expert basis evaluated against
real downstream bytes** has enough value to justify ANVIL's broader
representation-compiler research.

---

## 21. Do-not-reburn constraints

G2 must not be represented as a revival of:

- global lane transpose;
- the old supplied-stride synthetic column oracle;
- the old Gorilla single-mechanism gate;
- old modal residual coding;
- old arbitrary lambda size/speed selection.

Differences:

- columns are decoder-visible exact lexical shape positions;
- raw Brotli remains mandatory;
- final decisions use actual whole-carrier Brotli bytes;
- operator and planner effects are separately measured;
- held-out data is preserved;
- no size/speed scalarization occurs.

---

## 22. Success interpretation

The strongest G2 outcome would be:

- D4 retains its large G1 win;
- D1 and/or D2 cross 5% using a small typed basis;
- P-MIXED approximates P-MARGINAL closely;
- V1 retains >=1%;
- typed selections concentrate in a few interpretable leaf classes.

That would justify building a production-shaped structured representation
expert and then measuring full Pareto economics.

A weaker but useful result is:

- P-MARGINAL wins materially but P-MIXED does not.

That means the operator basis is useful but planner/search quality is the next
problem.

A clean NO-GO is also useful:

- if dictionary/integer experts cannot broaden the win, do not respond by
  adding every known columnar codec.
- return to explanation-gap/anatomy work and ask what D4 has that D1/D2 lack.

---

## 23. Implementation order

1. freeze this preregistration and corpus freeze publicly;
2. fork the G1 standalone tool into a G2 research prototype;
3. reproduce G1R and raw bytes first;
4. implement EXACT_DICT with direct malformed tests;
5. implement INT_FOR with overflow/adversarial tests;
6. implement INT_DELTA_FOR;
7. implement INT_DOD_FOR;
8. implement P-DICT/P-INT/P-MIXED local selection;
9. implement bounded P-MARGINAL exact search;
10. run D1-D4 discovery on GitHub Actions;
11. close discovery;
12. only if PASS-G2-DISCOVERY, open V1 on the frozen SHA.

No production ANVIL wire ID is authorized by this document.
