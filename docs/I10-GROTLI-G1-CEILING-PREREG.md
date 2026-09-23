# ANVIL I10 — GROTLI G1-CEILING Pre-Registration

**Frozen:** 2026-09-23
**Class:** representation-ceiling experiment; no novelty claim
**Production source changes:** forbidden
**Compute:** GitHub Actions only
**Predecessor:** G0 NO-GO in `I10-GROTLI-G0-RESULTS.md`

## 0. Question

G0 established that blind padded row-position vXOR is the wrong representation.

G1 asks the actual Grotli-derived question:

> **If repeated record structure is represented sparsely and semantically,
> without materializing absent coordinates, can the same Brotli q11/lgwin30
> backend compress the complete exact carrier smaller than raw Brotli?**

This is an **oracle-schema ceiling** first. It deliberately separates
representation power from automatic structure discovery.

If even the oracle ceiling cannot beat raw Brotli, do not build an expensive
automatic SRS planner for this class.

## 1. Frozen first mechanism: exact JSON/NDJSON shape + lexical field streams

G1-C0 operates on JSON/NDJSON records that parse successfully.

It must reconstruct the **original bytes exactly**. Semantic equivalence is not
enough.

The encoder separates each record into:

1. a structural/lexical skeleton;
2. shape identity;
3. ordered value slots;
4. exact raw lexical token bytes for each value.

The first ceiling experiment intentionally does **not** normalize value tokens.

Examples preserved verbatim:

- `1` versus `1.0`;
- exponent spelling;
- escaped versus unescaped string spellings;
- whitespace;
- key order;
- duplicate keys;
- CRLF/LF;
- final newline presence.

### 1.1 Shape

A shape describes the exact non-value lexical fragments surrounding ordered
value slots.

Conceptually:

    prefix_0
    VALUE_0
    prefix_1
    VALUE_1
    ...
    suffix

Identical shapes are dictionary-coded once.

Each record emits a shape ID plus its value tokens.

This is a sparse representation: there is no max-record-length matrix and no
padding.

### 1.2 Field/value streams

For G1-C0, slots that correspond across identical shapes are grouped into
streams.

Each stream initially stores:

- token byte length;
- exact token bytes.

The complete carrier is then Brotli-compressed.

This isolates whether **structure separation alone** helps.

## 2. Frozen ablation ladder

Only after C0 bytes are measured, evaluate the following cumulative or
individually attributable leaves. Report every stage separately.

### C0 — SHAPE+RAW

Shape dictionary + shape IDs + exact raw value-token streams.

### C1 — SHAPE+DICT

For string/scalar token streams with repeated exact lexical tokens:

- local exact-token dictionary;
- ID stream;
- raw escape.

Charge dictionary completely.

### C2 — SHAPE+INTEGER

Only tokens that parse as integers and roundtrip to the exact lexical token under
an explicit lexical descriptor may use numeric coding.

Candidates:

- FOR + bitpack;
- delta + zigzag + bitpack;
- delta-of-delta.

If exact lexical spelling cannot be reconstructed cheaply, retain raw token.

### C3 — SHAPE+PRESENCE

For optional/missing/null-like slots across related shapes:

- compact presence bitmap;
- default + exceptions.

No padded absent values.

### C4 — SHAPE+STRING-SYMBOL

FSST-like substring symbolization is an optional later leaf only if C0/C1 show
string-stream opportunity. It is not required to rule the basic ceiling.

Float/Gorilla/ALP leaves are deferred until the integer/string/shape ceiling is
known.

## 3. Controls

Every file has:

    R0 = Brotli_q11_lgwin30(original_bytes)
    C0 = Brotli_q11_lgwin30(complete_shape_raw_carrier)
    C1...
    selected = min(R0, C0, C1, ...)

Raw wins ties.

The same canonical `brotli_lw` helper is used for every arm.

## 4. Corpus discipline

The G0 GH Archive file has been opened and is **not held out anymore**.

G1 therefore needs a new validation family before any outcome-driven tuning.

Discovery may reuse:

- Amazon cellphone NDJSON;
- CDISC ADaM NDJSON;
- GH Archive 10 MiB as a now-open diagnostic family.

Before implementation tuning, freeze at least two additional independent real
JSON/NDJSON families, with one designated validation-only.

Prefer datasets with materially different structure:

- repeated object records;
- arrays/fixed-column records;
- heterogeneous events/logs.

Pin immutable commit/path/blob/size exactly as G0 did.

## 5. Complete-cost contract

Charge:

- carrier magic/version;
- decoded source length;
- record count;
- shape count;
- every shape's lexical fragments;
- shape-ID stream;
- stream descriptors;
- stream lengths;
- token-length/presence streams;
- dictionaries;
- numeric parameters;
- exception streams;
- backend payload;
- any outer envelope difference.

Report both carrier bytes before Brotli and final Brotli bytes.

## 6. Required diagnostics

Per file/stage:

- source bytes;
- raw Brotli bytes;
- carrier bytes;
- carrier+Brotli bytes;
- delta vs raw Brotli;
- shape count;
- top-shape coverage;
- shape dictionary bytes;
- shape-ID bytes;
- value-stream bytes;
- length/presence bytes;
- dictionary bytes;
- numeric payload bytes;
- exception bytes;
- exact SHA roundtrip;
- encode/decode wall time;
- peak RSS when promoted to timing lane.

Also report:

    structure_gain = raw_Brotli - C0
    typed_leaf_gain = C0 - best_typed

This distinguishes the architectural gain from leaf-specific coding.

## 7. Pass/fail

### PASS-CEILING

Authorize automatic SRS discovery work when:

1. every emitted candidate roundtrips exactly;
2. C0 or a small typed extension beats raw Brotli by >=5% on at least two
   independent discovery families;
3. newly frozen validation retains >=1% aggregate improvement;
4. metadata is fully charged;
5. raw fallback prevents selected-byte regression;
6. no one giant schema/dictionary artifact explains the result unfairly.

### PASS-TYPED-ONLY

C0 structure separation does not win, but one small typed leaf repeatedly
produces material complete-byte wins.

Action:

- pursue that typed expert;
- do not claim broad schema separation is valuable.

### NO-GO-SRS-JSON

Even oracle schema + the small typed portfolio cannot materially beat raw
Brotli.

Action:

- stop broad JSON SRS work;
- retain only any independently successful typed experts;
- move explanation research elsewhere.

## 8. Why this is the correct successor to G0

G0's strongest negative evidence was not merely that XOR was bad.

It showed:

- Amazon: only 1.394x padding expansion, yet vXOR still destroyed Brotli's useful
  literal/dictionary structure;
- ADaM/GH Archive: residual H0 looked spectacular while the coordinate support
  exploded 14x/35x.

Therefore G1 changes the **representation class**, not a threshold.

It preserves:

- repeated shape once;
- homogeneous values together;
- sparse presence;
- exact lexical reconstruction;

without manufacturing a rectangular byte domain.

That is much closer to the mechanism behind the strongest historical Grotli
results and to modern columnar systems such as ALP/FastLanes/BtrBlocks.

## 9. Implementation order

1. freeze new G1 corpus/split;
2. implement byte-exact JSON lexical tokenizer with roundtrip-only tests;
3. implement C0 shape/raw carrier;
4. run discovery;
5. if C0 shows opportunity, add C1 dictionary;
6. only then add integer/presence leaves;
7. freeze implementation;
8. open validation once;
9. close gate before any production ANVIL integration.

No `src/anvil.cpp` changes before this gate closes.
