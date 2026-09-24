# ANVIL I10 — Grotli-Inspired Representation Synthesis Program

**Date:** 2026-09-23
**Status:** research + implementation architecture; no production wire authorized yet
**Goal:** determine whether adaptive semantic representation can make ANVIL beat raw Brotli on real structured data while retaining a raw-Brotli fallback
**Execution policy:** CPU-heavy compression experiments run on GitHub Actions only

---

## 0. Central thesis

The most important lesson from Grotli is not any one codec primitive.

It is:

> **A mature entropy backend can be beaten by itself when the encoder first
> presents it with a better reversible representation of the same information.**

Grotli already demonstrated this empirically on several structured workloads:

- arrays of objects benefited from columnization;
- numeric columns benefited from delta / delta-of-delta / frame-of-reference /
  bitpacking;
- low-cardinality fields benefited from dictionary/RLE-like representations;
- smooth floating-point/time-series structure benefited from Gorilla-style
  representations;
- a direct/general representation remained necessary for unstructured or unique
  data.

ANVIL should turn that empirical lesson into a general architecture.

The target is **not**:

    invent one transform that beats Brotli everywhere

The target is:

    raw bytes
      -> discover structure
      -> synthesize several exact representations
      -> compress each serious finalist with the same mature backend
      -> emit the smallest complete representation

with raw Brotli itself always remaining a candidate.

This creates a built-in candidate-level fallback and makes every transformed-
versus-raw backend gain attributable to representation. Final comparisons
against a standalone Brotli file must still charge ANVIL's own envelope/framing.

---

## 1. Why this is much stronger than "add columnar compression"

ANVIL already explored several isolated members of the Grotli family:

- fixed-stride / record-period delta;
- columnar residual contexts;
- Gorilla / vertical-XOR lineage;
- line-column transpose;
- arithmetic progression / field predictors;
- fixed field partitions.

Those experiments taught important negative and positive lessons:

1. **global lane transposition is not universally safe;**
2. field/record alignment matters;
3. supplied structure can look spectacular on synthetic data but fail to
   transfer;
4. auto-discovery is useful infrastructure, not novelty;
5. a strong same-transform control can erase an apparent "new codec" win;
6. per-field semantic transforms are often prior art and should be adopted on
   Pareto merit, not novelty theater.

The Grotli architecture is different from any one of those mechanisms.

Its value is:

> **adaptive composition and arbitration over a portfolio of reversible
> representations, with the final backend seeing the result.**

That architectural layer is still substantially open inside ANVIL.

---

## 2. The critical invariant: raw Brotli is always a candidate

For a region X, define:

    B0 = size(Brotli_q11(X))

For every synthesized representation Ri:

    Bi = size(header_i || Brotli_q11(serialize(Ri(X))))

ANVIL emits Ri only when:

    Bi < B0

after charging:

- representation/operator IDs;
- schema/shape information;
- dictionary contents;
- lengths;
- exception positions;
- transform parameters;
- nullability;
- residual streams;
- framing;
- backend payload;
- integrity bytes.

This is the single most important design rule.

It means a structured transform does not need to be universally good.

It only needs to create a better coordinate system on regions where it applies.

On unstructured/encrypted/compressed/high-entropy data:

    raw Brotli wins -> no structured route

On a favorable structured region:

    semantic carrier + Brotli wins -> representation intelligence pays

The same experiment can later be repeated with BWT or other leaf backends, but
raw-Brotli-vs-transformed-Brotli is the cleanest causal test.

---

## 3. Modern evidence that the Grotli pattern is broader than JSON

Several current systems independently validate pieces of this architecture.

### FastLanes

FastLanes uses flexible composable encoding expressions rather than relying
primarily on a generic compressor. It performs a two-phase search for encoding
expressions, supports multi-column compression, and designs layouts for
data-parallel decode.

Architectural lesson:

> representation composition can improve both rate and decode when the encoding
> vocabulary matches data semantics and hardware.

Reference:
- https://github.com/cwida/fastlanes
- https://ir.cwi.nl/pub/35881

### ALP

ALP adaptively chooses between two fundamentally different exact float
representations:

- decimal-like floats -> exact integer transform -> FOR / bitpacking;
- high-precision floats -> split representation with dictionary-compressed high
  bits and retained low bits.

It samples row groups and vectors rather than exhaustively trying every
representation on every value.

As of September 2026, ALP has also been adopted into the Apache Parquet format.
Parquet's current format guidance recommends sampling a small number of
exponent/factor candidates and evaluating those finalists per vector rather than
exhaustively searching the whole parameter space for every vector.

Architectural lesson:

> **sample -> classify -> choose representation -> vector-friendly leaf codec**

This is a particularly strong precedent for ANVIL because it shows that an
adaptive representation search can survive the transition from a research
prototype into a widely used interoperable file format.

Reference:
- https://github.com/cwida/ALP
- https://doi.org/10.1145/3626717
- https://parquet.apache.org/blog/2026/09/22/alp-adaptive-lossless-floating-point-encoding-in-apache-parquet/
- https://apache.googlesource.com/parquet-format/+/refs/heads/master/AlpEncoding.md

### BtrBlocks

BtrBlocks is particularly close to Grotli's **adaptive portfolio** lesson. It
builds a pool of type-specific lightweight schemes and recursively cascades
them. Rather than fully compress every block with every scheme, it first
generates statistics, rejects implausible schemes, and compresses representative
samples to estimate the winner.

The published evaluation reports that its default 10 x 64-tuple sampling
strategy samples about 1% of a 64K-tuple block, consumes about 1.2% of total
compression CPU, and lands roughly 3.3% above the exhaustive optimum on average.

Architectural lesson:

> **sampling-based pruning can make adaptive representation search cheap enough
> to deploy, while exact whole-block fallback keeps mistakes bounded.**

Reference:
- https://doi.org/10.1145/3589263
- https://github.com/maxi-k/btrblocks

### Pcodec

Pcodec explicitly decomposes numerical sequences into smoother latent variables
and then applies delta/binning to make those latents easier to model.

Architectural lesson:

> the correct latent variables may matter more than the final entropy coder.

Reference:
- https://arxiv.org/abs/2502.06112

### FSST

FSST learns a compact byte-string symbol alphabet and maps common 1-8 byte
substrings to single-byte codes before downstream storage/use.

Architectural lesson:

> even the alphabet itself is a representation decision.

Reference:
- https://github.com/cwida/fsst

These are not direct ANVIL implementations. They are evidence that Grotli's
basic strategy is a broad compression pattern rather than a JSON-specific
accident.

### DataCortex — direct JSON/NDJSON prior art for the lower SRS layer

DataCortex (public Rust project, 2026) is materially close prior art to a
JSON-specialized subset of the proposed SRS architecture.

Its current fast path publicly describes and implements:

1. JSON/NDJSON format detection;
2. schema inference;
3. uniform/grouped/selective columnar reorganization;
4. type-specific encodings;
5. multiple raw/preprocessed zstd/Brotli candidate paths;
6. final smallest-output arbitration;
7. reversible transform metadata and byte-exact reconstruction.

The source goes beyond the README in several important ways:

- NDJSON can be grouped by schema and optionally by a low-cardinality
  discriminator;
- high-cardinality long columns may deliberately remain row-major because
  zstd/Brotli can exploit inter-row locality better in the original layout;
- typed leaves include integer delta+zigzag+LEB128, boolean bitmaps, timestamp
  deltas, enum dictionaries, string representations, UUID packing, nullable
  bitmaps, and raw fallback;
- some apparently compact transforms are explicitly disabled/avoided when they
  make the downstream generic compressor worse;
- transform-chain metadata is serialized and may itself be compressed or
  embedded into the backend payload;
- fast mode evaluates several complete raw/preprocessed backend paths and picks
  the smallest including metadata/header overhead.

References:
- https://github.com/rushikeshmore/DataCortex
- https://docs.rs/datacortex-core/latest/datacortex_core/format/
- https://docs.rs/datacortex-core/latest/datacortex_core/format/ndjson/fn.preprocess.html

DataCortex reports substantial wins over Brotli-11/zstd-19 on its current JSON
benchmarks, but these are author-reported project measurements. They are useful
competitive evidence, not ANVIL evidence until independently reproduced.

**Prior-art ruling for ANVIL:**

The following are **not** plausible mechanism-level novelty claims by
themselves:

- infer JSON/NDJSON schema;
- reorganize records into columns;
- encode typed columns;
- selectively retain high-cardinality data in row order;
- try raw/preprocessed Brotli/zstd paths and choose the smallest;
- serialize reversible transform metadata.

These remain fully valid engineering components.

The open ANVIL layer is broader:

- arbitrary-byte / weak-schema structure discovery;
- compression-objective structure hypotheses rather than format semantics;
- target-directed predictor/explanation synthesis;
- reference/replay/generative experts beyond typed columns;
- representation x leaf factorization;
- marginal-expert basis selection;
- explicit byte/decode/RSS/code-size Pareto extraction;
- hardware-oriented fused lowering.

### CLP and LogPrism — structure extraction must serve compression, not semantics

CLP (OSDI 2021) already showed a successful domain-specific pattern for logs:
separate static logtype structure, timestamps, dictionary variables, and
non-dictionary variables, then apply a lightweight compressor to the encoded
streams.

Reference:
- https://www.usenix.org/conference/osdi21/presentation/rodrigues

LogPrism (2026) pushes the lesson further. It argues that the conventional
"parse then compress" boundary can itself lose compression because a
semantically accurate parser may:

- over-generalize templates and dump too much entropy into variable streams;
- over-fit templates and spend too much dictionary metadata;
- destroy correlations between a template and its variables;
- destroy inter-variable co-occurrence information.

Its Unified Redundancy Tree instead integrates structural extraction and
variable encoding under the compression objective.

Reference:
- https://arxiv.org/abs/2601.17482

This is a critical design correction for SRS:

> **ANVIL should infer the representation that minimizes complete decoder-visible
> cost, not the representation that most closely resembles an application's
> semantic schema.**

A "wrong" field partition can be the right compression explanation if it is
byte-exact, cheaper to describe, and creates lower residual cost.

Therefore semantic JSON/record schemas are only candidate explanations. They do
not receive privileged status over byte-position, predictor, reference, or
cross-field partitions.

---

## 4. Proposed architecture: Structured Representation Synthesizer (SRS)

SRS should initially live as an encoder-side research layer in front of the
existing mode-17 ratio backends.

Conceptual pipeline:

    bytes
      -> structure/anatomy discovery
      -> typed region graph
      -> candidate representation synthesis
      -> cheap pruning
      -> exact finalist serialization
      -> Brotli/BWT finalist compression
      -> complete-byte arbitration
      -> selected carrier

The decoder does **not** discover structure.

It receives the selected representation and executes it deterministically.

---

## 4.1 Predictor synthesis: the deeper Grotli abstraction

The representation portfolio can be expressed more generally as a family of
decoder-cheap predictors plus exact residuals.

For a source region X and decoder-visible state S:

    X = P(S, theta) +_op R

where:

- P is a cheap deterministic predictor;
- theta is compact predictor metadata;
- +_op may be XOR, modular addition, copy/patch, concatenation, or another exact
  reversible composition;
- R is the residual that the leaf coder must still represent.

Examples:

| Grotli / ANVIL representation | Decoder-visible predictor |
|---|---|
| vertical XOR | same byte position in previous record |
| delta | previous integer |
| delta-of-delta | previous delta / local linear trend |
| FOR | block base |
| dictionary | selected dictionary entry |
| default + exceptions | modal/default value |
| Gorilla XOR | previous IEEE-754 bit pattern + prior significant-bit window |
| TCOPY / copy+patch | referenced prior phrase plus transform parameters |
| cross-column residual | another field / column |
| DEFLATE replay | deterministic re-encoder given semantic plaintext + replay state |

This is the strongest conceptual bridge between Grotli and the ANVIL Explanation
Machine:

> **Search for decoder-cheap state that makes the source predictable, then pay
> only for the predictor description and what it failed to predict.**

The final backend is deliberately secondary. A residual can be:

- passed through Brotli;
- bitpacked directly;
- Huffman/rANS coded;
- represented as sparse exceptions;
- routed through BWT;
- left literal.

This formulation also gives the hardware lane something concrete to optimize.
Many good predictors map cleanly to regular kernels:

- previous-row XOR -> wide XOR loads/stores;
- FOR -> subtract base + bitpack;
- default/exceptions -> bulk fill + sparse patches;
- dictionary -> packed IDs + table lookup;
- copy/patch -> bulk copy + event masks.

The decoder should execute the predictor; it should never rediscover it.

---

## 5. Stage A - structure/anatomy discovery

The biggest difference from old fixed-stride experiments is that SRS should
recognize multiple structure classes.

Initial detector classes:

### A1 — structured text

- NDJSON;
- JSON array-of-objects;
- CSV/TSV-like records;
- logfmt/key-value records;
- repeated textual record shapes.

Detector output should include:

- record boundaries;
- field/key identities;
- inferred scalar types;
- null/missing pattern;
- repeated-shape frequency;
- field cardinality;
- per-field length/value statistics.

### A2 — fixed/near-fixed binary records

- periodic record size;
- stable offsets;
- inferred 1/2/4/8-byte lanes;
- constant/slow/changing offsets;
- candidate endianness;
- monotonicity;
- signedness likelihood;
- float-like exponent/mantissa structure.

This is the engineering gap explicitly left open by the earlier datastruct
ruling: automatic period + field-partition discovery.

### A3 — homogeneous numeric arrays

Detect likely:

- u8/u16/u32/u64;
- i8/i16/i32/i64;
- f32/f64;
- timestamp-like sequences;
- counters;
- identifiers;
- monotone values.

### A4 — repeated shape without semantic parser

When a formal parser is unavailable, use:

- repeated separator positions;
- local record-length clustering;
- byte-position stability;
- vertical entropy profiles;
- periodic autocorrelation;
- lane-change masks.

This can produce a **virtual schema** without claiming to know the application
format.

---

## 6. Stage B — typed region graph

Do not flatten everything immediately into one transformed byte stream.

Represent discovered structure as a bounded typed graph:

    RecordGroup
      -> Field(name/id, type, null-map, values)
      -> Field(...)
      -> ...

Possible leaf types:

- BYTES
- STRING
- ENUM
- BOOL
- UINT(w)
- SINT(w)
- FLOAT32
- FLOAT64
- TIMESTAMP
- OPAQUE

The type is an encoder hypothesis, not a semantic claim.

Every candidate must be byte-exact.

If a supposedly numeric field cannot roundtrip exactly, it falls back to bytes.

The carrier should record only what the decoder actually needs:

- field order;
- record count;
- lengths where not derivable;
- missing/null positions;
- selected representation per field;
- residual/exception data.

---

## 7. Stage C — Grotli++ representation portfolio

The initial portfolio should reuse proven ideas rather than invent exotic math.

### C0 — RAW

Exact original field/region bytes.

Mandatory fallback.

### C1 — DICTIONARY

For low-cardinality strings/integers:

- local dictionary;
- frequency-ranked IDs;
- bitpacked IDs;
- null/escape path.

Charge dictionary bytes completely.

### C2 — RLE / default + exceptions

For low-transition fields:

- run lengths;
- dominant/default value;
- sparse exception bitmap/positions;
- exception payload.

### C3 — FOR + bitpack

For clustered integer ranges:

    base = min(values)
    residual = value - base
    bitpack residuals to required width

Optionally patched FOR when a few outliers determine width.

### C4 — DELTA

For locally smooth integers:

    d_i = x_i - x_(i-1)

then choose:

- zigzag;
- FOR;
- bitpack;
- range/offset;
- small exception stream.

### C5 — DELTA-OF-DELTA

For counters/timestamps/arithmetic trends.

This is adopt-class Gorilla lineage.

### C6 — XOR / Gorilla float

For repeated/smooth float bit patterns:

- xor previous bits;
- zero xor case;
- reuse leading/trailing-zero window;
- new window;
- exception/raw fallback.

### C7 — ALP-like float representation

Research-only initially:

- test exact decimal-to-integer representation;
- verify raw IEEE bits reconstruct exactly;
- FOR + bitpack integerized values;
- exception side stream;
- alternate high/low-bit split for high-precision floats.

Do not import ALP as novelty. Treat it as a reference/adopt-class leaf.

### C8 — FSST-like string symbols

For high-cardinality strings where whole-string dictionary coding is poor:

- learned 1-8 byte symbols;
- compact local symbol table;
- escape raw bytes.

### C9 — vertical byte/bit lanes

For fixed-width values:

- byte shuffle;
- high/low byte split;
- bit-plane split;
- vertical XOR.

This is useful only when the leaf backend benefits enough to pay the layout
metadata.

### C10 — shape dictionary

For JSON/object-like records:

- encode repeated key/order/type shape once;
- values become field streams;
- rare alternate shapes carry shape ID + deviations.

### C11 — cross-column predictor

Later experiment:

- field B predicted from field A;
- transmit base/reference field + residual;
- only when correlation survives exact-cost scoring.

FastLanes multi-column compression is relevant lineage.

### C12 — REPLAY

DEFLATE replay and future deterministic serialization reconstruction can be
viewed as another representation operator:

    semantic payload + replay program -> original serialized bytes

This makes I10-1B conceptually part of the same architecture.

---

## 8. The major upgrade over Grotli v3: score the *real final payload*

One historical Grotli weakness was that candidate choice could optimize the
packed intermediate rather than the actual final compressed result.

ANVIL should never repeat that.

Use a two-stage selector.

### Stage 1 — cheap candidate score

For every candidate compute cheaply:

- exact carrier metadata bytes;
- transformed length;
- zero-order / sampled entropy;
- bit width;
- exception density;
- run count;
- dictionary size/cardinality;
- estimated backend affinity;
- estimated decoder work.

Reject obviously dominated candidates.

### Stage 2 — actual backend arbitration

For only the best K finalists:

    serialize candidate completely
    compress with the actual production leaf backend
    measure exact output bytes

Then choose by exact bytes/profile.

For max-ratio:

    winner = min exact serialized backend output

For balanced/speed profiles:

retain a local Pareto set over:

- exact bytes;
- predicted/measured decode class;
- memory.

No candidate is promoted from a proxy score alone.

---

## 9. Avoid Grotli v4's planner bottleneck

Grotli's Python implementation eventually showed the danger of evaluating too
many full candidates recursively.

ANVIL's planner must be native and hierarchical.

### 9.1 Two-level sampling

Borrow the ALP/FastLanes strategy:

1. sample the record group / region;
2. identify plausible representation families;
3. sample vectors within the selected family;
4. build only a few whole-region finalists.

Do not serialize q11 Brotli for dozens of candidates per field.

### 9.2 Vector granularity

Initial structured vector:

    1024 values

is worth testing because it aligns with ALP/FastLanes precedent and bounds
metadata/exception locality.

It is **not** a format commitment.

Sweep:

- 256;
- 512;
- 1024;
- 2048;
- 4096 values

on representative typed streams.

### 9.3 Candidate beam

Keep at most a small beam per field, e.g.:

- best size proxy;
- best speed proxy;
- best robust/fallback candidate.

Then evaluate complete record-group combinations using branch-and-bound.

### 9.4 Exact direct fallback

A whole region can always fall back to RAW + Brotli.

That prevents detector confidence from becoming correctness or size risk.

---

## 10. Carrier design: representation first, generic backend second

The first experiment should deliberately stay simple.

### v0 carrier

One structured carrier is serialized as bytes:

    magic/version
    record_count
    shape/schema descriptor
    field_count

    for each field:
        field descriptor
        representation ID
        parameter bytes
        payload length
        representation payload

    record reconstruction metadata

Then the **entire carrier** is fed to Brotli q11/lw30.

Why this is the correct first experiment:

- exactly matches the successful Grotli architecture;
- requires only one generic backend;
- reuses ANVIL's existing mode-17 transform/backend abstraction;
- isolates the question "does semantic representation improve Brotli?";
- avoids premature per-field entropy-backend complexity.

Only if carrier+Brotli wins should ANVIL consider per-field backend selection.

---

## 11. Later architecture: per-stream leaf arbitration

A second generation may allow each semantic stream to choose its own leaf:

- raw/bitpacked;
- Huffman;
- rANS;
- Brotli;
- BWT;
- FSST-like symbol stream;
- specialized numeric leaf.

But this should not be v0.

Per-stream backend selection introduces:

- many small headers;
- poor backend amortization;
- allocator/buffer cost;
- more dispatch;
- more search combinations.

The v0 carrier+Brotli experiment can tell us how much opportunity exists before
paying this complexity.

---

## 12. How this maps onto current ANVIL mode 17

Current mode 17 already has:

    transform_id
    backend_id
    transformed_size
    backend_payload

and already chooses the smallest complete payload across:

- direct;
- ctx1;
- line-columns;

and:

- Brotli;
- BWT.

This is unusually convenient.

A future structured candidate could conceptually become:

    transform = STRUCTURED_CARRIER
    transformed bytes = serialized SRS carrier
    backend = Brotli or BWT

The current selection loop already contains the most important rule:

    if candidate payload is smaller -> candidate wins

However, do **not** assign a transform ID yet.

First prove the carrier externally/prototypically.

The production decoder-visible format should be designed only after the
representation opportunity is measured.

---

## 13. Critical interaction with earlier ANVIL negative results

### 13.1 Do not re-burn global lane transpose

Old finding:

> globally transposing arbitrary binary lanes before LZ can destroy locality and
> lose badly.

SRS avoids this by routing only a region for which a structured carrier is
itself the complete representation.

The structured transform is not a mandatory pre-LZ filter.

### 13.2 Do not rely on supplied schema for generalization claims

Synthetic period/field knowledge produced spectacular results before.

SRS therefore has two explicit tiers:

**oracle-schema**
- structure supplied;
- measures upper opportunity.

**discovered-schema**
- structure inferred from bytes;
- real candidate.

The delta between them is the **structure-discovery tax**.

### 13.3 Same-transform references remain mandatory

If SRS uses delta/FOR/etc., compare against relevant reference transforms where
possible.

A "win versus raw Brotli" is useful engineering evidence but not necessarily a
mechanism novelty claim.

### 13.4 Block-alternative, not mandatory pipeline

For structured numeric/time-series routes, the earlier Gorilla pre-registration
already established the safer integration model:

> whole block/region routes to the specialized representation instead of being
> blindly transformed before every other parser.

SRS generalizes that rule.

---

## 14. The strongest immediate target: NDJSON / JSON arrays-of-objects

This should be the first real structured experiment because it reproduces the
domain where Grotli already demonstrated strong wins while remaining common and
easy to verify.

### 14.1 Exact parse

Support:

- JSON array of objects;
- NDJSON object per line.

Preserve byte identity, including:

- key order;
- whitespace;
- numeric lexical spelling;
- string escaping;
- absent versus null;
- duplicate keys;
- line ending style.

This is important.

A semantic JSON parser that reserializes normalized JSON is **not** lossless
byte compression.

### 14.2 Representation

Separate:

- structural template/shape;
- keys;
- whitespace/style tokens;
- scalar values;
- record shape IDs.

For each repeated field:

- integer candidates;
- float candidates;
- string dictionary / FSST;
- boolean/null bitmaps;
- raw lexical fallback.

### 14.3 Reconstruction

The decoder should replay exact lexical fragments around the typed value
streams.

Do not canonicalize JSON.

---

## 15. Second target: generic fixed/near-fixed records

This directly attacks an ANVIL capability gap from I9.

Use anatomy to infer:

- candidate period P;
- per-offset entropy;
- likely multi-byte lanes;
- endian hypothesis;
- field boundaries based on correlated change masks;
- lane type hypotheses.

Then synthesize carriers under several partitions.

The critical research question is:

> Can a cheap byte-level detector recover enough of the useful field partition
> that the transformed+Brotli win survives without a schema?

This is a much stronger test than another supplied-stride synthetic.

---

## 16. Third target: homogeneous numeric sequences

This is the cleanest bridge from Grotli to ALP/Pcodec/FastLanes.

For a buffer that can be interpreted exactly under candidate widths:

- 1/2/4/8-byte integers;
- f32/f64;

try:

- raw;
- byte shuffle;
- FOR;
- delta;
- DoD;
- PFor;
- Gorilla;
- ALP-like float exact transform;
- range/offset.

Then compress the complete carrier with Brotli.

The surprising but important possibility is:

> a specialized lightweight encoding may already be so compact that adding
> Brotli is unnecessary.

Therefore record both:

    carrier bytes
    Brotli(carrier) bytes

A representation that is smaller *and* vastly faster without Brotli is a
different Pareto point and should not be hidden.

---

## 17. Cross-field/multi-column opportunity

Grotli v3 mostly chose per-column representations.

ANVIL can eventually go one step further.

Examples:

### correlated timestamps

    ts_end = ts_start + duration

Store:
- ts_start stream;
- duration stream;

instead of two independent timestamp streams.

### repeated IDs / foreign-key-like values

Use:
- shared dictionary;
- local integer IDs;
- reference one field's dictionary from another.

### monotonic pair

If:

    B_i ~= A_i + small residual

store:
- A;
- residual(B-A).

### shape-conditioned values

A value's representation may depend on record shape/type.

This is consistent with FastLanes' multi-column compression direction.

The search must remain bounded.

Cross-field transforms should require a large sampled mutual-information /
residual-cost signal before full evaluation.

---

## 18. Search architecture: from Grotli heuristic selector to ANVIL synthesis

The long-term SRS search can become a concrete restricted Explanation Machine.

### Region grammar

    REGION :=
      RAW
      | RECORDIZE(layout, fields)
      | CONCAT(REGION...)
      | COLUMN(type, OP)
      | SHAPE_DICT(...)
      | REPLAY(...)

    OP :=
      RAW
      | DICT
      | RLE
      | FOR
      | DELTA
      | DOD
      | XOR
      | GORILLA
      | ALP_FLOAT
      | FSST
      | RANGE_OFFSET
      | BITPLANE
      | BYTE_SHUFFLE
      | DEFAULT_EXCEPT

The encoder searches this bounded space.

The decoder executes the chosen tree.

This is essentially Grotli generalized into ANVIL's program-synthesis thesis.

### 18.1 Preserve Grotli's search shape, not its scalar objective

The final Grotli architecture already converged on a useful four-layer router:

    estimate
      -> exploit/bandit choice
      -> best-of-K confirmation
      -> dispatch

That should carry forward.

The cheap estimator answers:

- which structure family is plausible?
- which operators are obviously impossible/dominated?
- is the region stationary enough to trust the estimate?

For high-confidence/stationary regions:

- exploit the predicted representation;
- still retain raw Brotli as the exact fallback.

For uncertain regions:

- serialize/compress only the best K finalists;
- choose from their actual complete outputs.

The Grotli router used a scalar J combining bytes and modeled decode cost. ANVIL
should **not** inherit that part literally.

Instead, keep explicit profiles.

**max-ratio**
- exact smallest complete archive bytes;
- hard decode/memory ceilings only.

**balanced**
- retain candidates within an explicit byte budget of the size winner;
- choose among the non-dominated bytes/decode/memory points.

**speed**
- hard rate-loss budget;
- choose fastest candidate inside it.

This preserves Grotli's compute-saving router architecture without hiding a
Pareto decision inside one arbitrary lambda.

### 18.2 Preserve Grotli's E1/E2 distinction as a later experiment

Grotli also separated:

- **E1:** concatenate descriptor/streams and run one Brotli pass;
- **E2:** compress semantic streams independently with a per-stream suite.

This is exactly the right ordering for ANVIL.

G0/G1 should begin with E1 because it:

- minimizes framing;
- lets Brotli exploit cross-stream redundancy itself;
- gives the cleanest raw-Brotli causal comparison.

Only after E1 demonstrates a strong structured win should E2 be evaluated.

E2 can then answer whether semantic streams want fundamentally different leaf
representations/codecs, e.g.:

- bitpack numeric residuals directly;
- FSST-like string stream;
- Huffman/rANS descriptor stream;
- Brotli raw string/literal stream;
- BWT on a selected residual.

The no-double-credit rule from Grotli remains binding:

> if one E1 Brotli pass already captures an effect, ANVIL does not get to claim
> the same cross-stream gain again as a separate mechanism.

---

## 18.3 Portfolio monotonicity and expert admission

There is a useful mathematical property to the representation-portfolio
architecture.

Let P be the current candidate set and cost(x,p) the complete encoded bytes for
candidate p on region x.

The max-ratio selector emits:

    C_P(x) = min over p in P of cost(x,p)

Adding a new representation expert E yields:

    C_(P union E)(x) <= C_P(x)

for every region where all old candidates remain reachable.

So **candidate-level compressed bytes are monotone non-increasing as the
representation portfolio grows.**

That is a powerful property, but it is not a free-lunch theorem for the whole
system.

Every expert can add:

- encoder search work;
- decoder text/rodata;
- tables;
- branch/dispatch surface;
- fuzz/security surface;
- maintenance burden;
- representation IDs/framing;
- potentially worse instruction-cache behavior even when not selected.

Therefore expert admission must be based on **marginal portfolio contribution**,
not isolated benchmark wins.

For expert E define:

    marginal_gain(E) =
        bytes(portfolio_without_E)
        -
        bytes(portfolio_with_E)

on the frozen validation portfolio.

Report:

- aggregate marginal bytes saved;
- number and source bytes of regions uniquely won;
- median/p90 win where selected;
- added decoder text + rodata bytes;
- decode-time delta when selected;
- encode-search cost;
- overlap with existing experts.

A representation that looks spectacular against raw Brotli but is never chosen
once an existing ANVIL expert competes contributes **zero** and should not earn a
permanent decoder ID.

This also gives a natural pruning loop:

1. evaluate candidate pool;
2. measure each expert's leave-one-out marginal contribution;
3. remove zero/tiny-contribution experts;
4. re-evaluate because contribution can change after pruning;
5. keep a compact complementary basis.

This is essentially an empirical basis-selection problem for lossless
representations.

### 18.4 Separate representation search from leaf-codec search

The long-term candidate should be factored as:

    Candidate = Representation x LeafBackend

rather than creating one monolithic codec ID for every combination.

Example compatibility matrix:

| Representation | Direct lightweight | Brotli | BWT | entropy streams |
|---|---:|---:|---:|---:|
| RAW | yes | yes | yes | limited |
| vXOR / record residual | possible | yes | maybe | yes |
| FOR / bitpack | **yes** | optional | usually low-EV | metadata only |
| Dictionary | **yes** | optional | low-EV | IDs yes |
| Gorilla / ALP float | **yes** | optional | low-EV | descriptors yes |
| Shape/lexical carrier | possible | **yes** | maybe | per-stream later |
| REPLAY | residual-dependent | yes | maybe | corrections yes |

Not every Cartesian-product pair should be evaluated.

Each representation declares a short list of compatible leaf families. Sampling
then prunes representations first, and leaf arbitration occurs only for
surviving finalists.

This prevents the search from growing as:

    number_of_representations x number_of_backends

on every region.

### 18.5 Encoder-only learned routing is allowed

ANVIL can spend intelligence at encode time without making the decoder
intelligent.

Every exact finalist evaluation produces a supervised label:

- anatomy features;
- candidate byte sizes;
- winning representation;
- winning leaf;
- measured encode/decode cost.

That creates a free training set for an encoder-side routing model.

Possible router evolution:

1. hand-written sampled heuristics;
2. online per-file bandit / win-rate model;
3. small decision tree or gradient-boosted model trained on prior exact results;
4. more expensive learned model only if its avoided candidate evaluations pay
   for its encoder cost.

The router can change between encoder versions without changing the wire.

The decoder receives only:

    selected representation ID + parameters + payload

and never runs the learned model.

For long heterogeneous files, an online bandit is especially attractive:

- explore several candidates in early blocks;
- condition on anatomy features;
- exploit reliable winners in stationary regions;
- re-open exploration after a detected distribution shift.

Exact raw/general fallback remains available, so router uncertainty is an
encode-cost problem before it is a correctness problem.

---

## 19. Key difference from Brevis

Brevis starts with typed tensor structure.

ANVIL often starts with arbitrary bytes.

Therefore the first problem is not only:

    which program compresses this typed region?

It is:

    is there a useful typed interpretation of these bytes at all?

This motivates a two-layer explanation:

    bytes
      -> structure hypothesis
      -> representation program

The structure hypothesis itself has a cost.

If ANVIL has to transmit a 5 KB schema to save 3 KB of field payload, the
hypothesis loses.

This keeps "semantic understanding" honest.

---

## 20. First falsifiable experiment: GROTLI-ANVIL-0

The binding pre-registration for this experiment is
[I10-GROTLI-G0-PREREG.md](I10-GROTLI-G0-PREREG.md).

Do **not** start with the full representation portfolio.

The first experiment should finish the exact experiment that Grotli itself left
one step short of a production claim.

### 20.1 What Grotli actually established

Direct inspection of the external Grotli `refs/FINAL.md` shows:

- the strongest reported lane was **json-lines / NDJSON**;
- vertical record-aligned XOR reduced the transformed stream to approximately
  **0.637 bits/byte H0** with **93.2% zero density** on that fixture;
- the reported size model was approximately **0.139** versus **0.218** for the
  raw-Brotli baseline, i.e. about **36.3% smaller**;
- Grotli correctly labelled that result **measured-approx / proportional**, not
  a lossless-verified final crossing;
- the decisive unresolved issue was that the vertical-XOR carrier did not yet
  serialize all record-length information needed for exact byte reconstruction;
- arbitrary prose/log/random lanes were neutral and were intended to fall back
  to Brotli.

Therefore the highest-EV ANVIL experiment is not "try many structured codecs."

It is:

> **Take the exact strongest Grotli representation, make its wire fully
> reversible, run real Brotli on both raw and transformed bytes, and see whether
> the gain survives.**

### 20.2 G0 representation

Implement only:

    input records
      -> exact record boundary detection
      -> stride = max record length (or exact bounded equivalent)
      -> zero-padded row matrix
      -> vertical XOR against previous row
      -> exact record-length side stream
      -> descriptor
      -> one Brotli q11/lw30 pass

Decoder:

    Brotli decode
      -> parse descriptor + record lengths
      -> inverse vertical XOR with rolling previous-row state
      -> trim each row to original length
      -> concatenate exact original bytes

No semantic JSON normalization is required for G0.

This is important: the representation is **byte structural**, not JSON
semantic. It can preserve whitespace, escaping, number spellings, key order,
duplicate keys, and all other lexical details automatically because it never
parses values.

### 20.3 Controls

For every file measure:

    C0 = Brotli_q11(raw bytes)
    C1 = Brotli_q11(full exact vXOR carrier)

Also record:

    carrier size before Brotli
    descriptor bytes
    record-length bytes
    matrix padding bytes
    transformed zero density / H0
    exact SHA roundtrip

The raw path is always a candidate.

### 20.4 Detection lanes

Run two separate lanes.

**Oracle-record lane**
- newline/known record boundaries supplied;
- measures pure representation opportunity.

**Discovered-record lane**
- detector must infer the framing from bytes;
- measures real deployment value.

The difference is the structure-discovery tax.

Do not mix the two in one headline.

### 20.5 Corpus

Use:

- Grotli's original NDJSON fixture as provenance/regression context only;
- independently sourced real NDJSON/event/log datasets;
- JSONL from public APIs/datasets;
- CSV/log controls;
- prose;
- Silesia/enwik8 negative/general controls;
- random incompressible bytes.

The original Grotli synthetic/generated data cannot establish generalization.

### 20.6 Decision gate

G0 passes if all of the following hold:

1. **bit-exact wire** on every accepted input;
2. at least two independent real NDJSON datasets beat raw Brotli q11/lw30 by
   **>=5% complete bytes**;
3. aggregate real-NDJSON gain is positive after descriptor/record-length/padding
   cost;
4. negative/general controls fall back to raw Brotli;
5. native inverse vXOR is cheap enough not to erase the byte win at the balanced
   profile;
6. detector-discovered framing retains a material fraction of the oracle-record
   gain.

A result near the old ~36% signal would be exceptionally strong, but **36% is
not the gate**. The gate is a real, bit-exact, independently reproduced win.

### 20.7 Why G0 comes before SRS

If G0 fails after honest wire:

- do not generalize it into a large structured architecture;
- record whether the failure came from padding, record metadata, detection, or
  real Brotli behavior.

If G0 passes:

- it proves that, on the measured structured class, the same Brotli backend can
  produce materially fewer complete bytes when ANVIL supplies a better exact
  representation;
- it justifies G1/SRS.

---

## 21. Second experiment: GROTLI-ANVIL-1 / Structured Representation Synthesizer

Only after G0 passes, build the isolated native/prototype carrier described in
sections 4-19.

### Inputs

Use a real-data structured set, not only synthetics:

- NDJSON event/analytics data;
- API response arrays;
- structured logs;
- CSV;
- public telemetry;
- existing Grotli fixtures as provenance controls;
- ANVIL canonical files as negative/general controls.

### Candidate representations

Initial high-value set:

- raw;
- exact lexical record/field columnization;
- dictionary;
- RLE/default-exception;
- integer FOR;
- delta;
- DoD;
- bitpack;
- Gorilla float;
- byte shuffle;
- simple shape dictionary.

### Backends

Primary causal comparison:

    Brotli q11/lw30(raw)
    Brotli q11/lw30(carrier)

Secondary context:

    BWT(raw)
    BWT(carrier)

### Metrics

Per file:

- input bytes;
- raw Brotli bytes;
- carrier bytes before backend;
- carrier+Brotli bytes;
- byte improvement over raw Brotli;
- metadata share;
- discovery time;
- candidate search count;
- encode time;
- decode time;
- peak RSS;
- exact SHA roundtrip;
- selected field operators.

### Success bar

The experiment is interesting if:

1. real structured files repeatedly beat raw Brotli by a material margin;
2. complete metadata is charged;
3. detector-discovered structure retains most of oracle-schema gain;
4. unstructured controls fall back cleanly;
5. a small operator set explains most wins.

Suggested promotion threshold:

- >= 5% aggregate improvement versus raw Brotli on at least two independent
  real structured classes;
- >= 1% on a held-out structured set;
- no emitted regression where raw Brotli is available as fallback;
- decoder representation remains bounded and native-friendly.

This threshold is a research gate, not a claim of novelty.

---

## 21.1 Why this path can move both ratio and decode speed

G0 intentionally keeps Brotli on both sides because it is the cleanest causal
test of representation value.

G1 should not make Brotli mandatory after every successful representation.

For every mature semantic carrier there should eventually be at least two
finalist shapes:

    specialized_carrier_direct

and:

    Brotli(specialized_carrier)

plus the existing raw/general candidates.

This matters because lightweight structured encodings can be substantially
cheaper to decode than a heavyweight general-purpose entropy/LZ codec.

Current external evidence supports the possibility:

- ALP has now been standardized in Apache Parquet as a lightweight exact float
  encoding designed for SIMD/GPU-friendly decode and random access; Parquet's
  published evaluation positions ALP without an additional heavyweight codec
  near the ratio of heavyweight alternatives on its target data while decoding
  much faster;
- FastLanes deliberately favors composable data-parallel encodings and reports
  improved ratio over Parquet while also accelerating decompression;
- BtrBlocks was built around the same storage/CPU observation: a small portfolio
  of fast type-specific schemes can outperform a generic format+codec stack on
  both storage and scan economics.

References:
- https://parquet.apache.org/blog/2026/09/22/alp-adaptive-lossless-floating-point-encoding-in-apache-parquet/
- https://doi.org/10.14778/3749646.3749718
- https://doi.org/10.1145/3589263

ANVIL implication:

> **Brotli is the fallback and representation-value oracle, not the final
> destination of every winning structured route.**

A structured carrier that is 3% larger than carrier+Brotli but decodes 10x
faster may be a valuable speed/balanced Pareto point.

A structured carrier that is itself smaller than raw Brotli and faster to decode
would be especially important: it moves the two axes currently keeping ANVIL
from an external full-front crossing.

---

## 22. The potentially enormous upside

Suppose on a heterogeneous corpus:

- 50% of bytes are structureless -> raw Brotli;
- 20% are repeated structured text -> 20% better than Brotli;
- 15% are numeric/time-series -> 30% better;
- 10% are replayable embedded compression -> 20% better;
- 5% miscellaneous -> raw Brotli.

Then overall improvement is:

    0.20*0.20 + 0.15*0.30 + 0.10*0.20
    = 10.5% fewer bytes

without requiring one universal new entropy coder.

That is only illustrative arithmetic.

But it shows why the portfolio architecture is powerful:

> **ANVIL can beat a general-purpose codec by being a better representation
> selector, even if many of its leaves are established codecs.**

This is probably a more plausible path to a meaningful general-purpose ratio
advance than squeezing another fraction of a percent from one entropy engine.

---

## 23. What "ANVIL beats Brotli" should mean

There are three progressively stronger claims.

### Level 1 — class-specific crossing

On a defined structured class:

    ANVIL structured route < Brotli raw

with complete bytes and equal losslessness.

Grotli already gives strong reason to expect this is achievable.

### Level 2 — portfolio crossing

On a heterogeneous corpus:

    sum ANVIL selected bytes < sum Brotli raw bytes

because ANVIL selects raw Brotli on unfavorable files and better
representations where available.

This is the most strategically important near-term target.

### Level 3 — full Pareto crossing

ANVIL's byte advantage survives:

- decode time;
- encode time;
- peak memory;
- binary size;

against Brotli/xz/Zstd reference points.

This is the final systems goal.

Do not conflate Level 1 with Level 3.

---

## 24. Strongest architectural bet

The best current hypothesis is:

> **ANVIL should become an adaptive representation compiler whose fallback leaf
> is a mature general-purpose compressor.**

The encoder is allowed to be intelligent:

- discover structure;
- infer types;
- search representations;
- use expensive sampling/oracles;
- generate a compact reconstruction program.

The decoder should remain simple:

- parse a bounded carrier;
- run fast fixed kernels;
- decode the selected leaf streams;
- reconstruct exact bytes.

Grotli was an early domain-specific proof of this idea.

ANVIL can make it general.

---

## 25. Immediate next work

Before any production integration:

1. build **GROTLI-ANVIL-0** as a standalone carrier prototype;
2. start with NDJSON/JSON arrays-of-objects;
3. preserve exact lexical bytes;
4. implement only the small high-value operator set;
5. compare raw Brotli q11/lw30 against carrier+Brotli q11/lw30 on GitHub Actions;
6. keep raw Brotli as mandatory fallback;
7. separate oracle-schema from discovered-schema;
8. record which operators actually win;
9. only then decide whether structured representation deserves a mode-17
   transform ID.

Parallel research:

- finish BWT subblock memory sweep;
- continue explanation-gap oracle design;
- keep I10-1B DEFLATE replay isolated.

No source-format change is justified until the GROTLI-ANVIL-0 experiment proves
that the representation layer retains Grotli's advantage on real, independently
sourced structured data.

---

## 26. Measured G0/G1 outcome — representation value is real but specialist

The original execution order above is retained as the historical preregistration
logic. The measured program has now advanced through two causal gates.

### G0 — padded row-major vXOR

Closed **NO-GO**.

The exact reversible carrier lost badly to raw Brotli on all measured real
NDJSON families. The failure was not merely metadata: positional XOR destroyed
literal/dictionary structure that Brotli already exploited well.

Source of record:

- [I10-GROTLI-G0-RESULTS.md](I10-GROTLI-G0-RESULTS.md)

### G1 — exact lexical shape separation + raw value columns

Closed broad **NO-GO-G1-DISCOVERY**, but with a strong specialist signal.

Frozen run 35937406182, public implementation
b82d9c83c9c8528861eb65595fface6605cf0e7a:

- D1 Amazon: SHAPE_COLUMN **-1.4853%** versus raw Brotli;
- D2 CDISC: raw Brotli wins; best structured arm **+2.4971%**;
- D3 frozen GH Archive excerpt: structured candidate unavailable under the
  byte-exact parser, so raw fallback;
- D4 CROVIA receipts: SHAPE_COLUMN **-21.7717%** versus raw Brotli.

The four-family routed portfolio is **-1.6564%** versus raw Brotli, below the
frozen -3% aggregate gate and with only one >=5% family.

Source of record:

- [I10-GROTLI-G1-RESULTS.md](I10-GROTLI-G1-RESULTS.md)

### The causal lesson from G1

D4 separates the mechanism unusually cleanly.

The row carrier already deduplicates almost 1.5 MB of repeated lexical
structure, yet improves final Brotli bytes by only **0.4804%**.

With the same templates, values and metadata, changing only value serialization
from row-major to corresponding-placeholder/column-major improves final bytes by
**21.7717%** versus raw Brotli.

Therefore the strongest measured mechanism is:

> **homogeneous semantic-position locality changes the statistical/match problem
> seen by the downstream codec enough that the same Brotli implementation can
> beat itself materially.**

D2 is the equally important negative control: its structured carrier is
10.5225% smaller than the original source *before* Brotli but 2.4971% worse
after Brotli. This directly validates the architecture rule that ANVIL must
score real downstream candidate bytes rather than transformed size, H0, zero
density, or semantic elegance.

### Current next step

Do not integrate G1 into production ANVIL.

The next structured experiment should be a separately preregistered **G2 typed
column expert**, retaining:

- raw Brotli as permanent fallback;
- the exact G1 lexical/shape reconstruction contract;
- actual final Brotli arbitration;
- the still-unspent Sino-US DrugQA corpus as held-out validation.

The first typed basis should remain deliberately small and attributable:

1. raw lexical column;
2. exact-token dictionary/enum;
3. canonical integer FOR/delta/DoD;
4. default/null bitmap only where it is a genuine column property.

Do not add float specialization, FSST, cross-column prediction, replay, or a
large combinatorial planner until this basis earns them.

The goal of G2 is not to rescue every G1 loss. It is to test whether a **small
basis of known cheap experts plus routing** can turn the observed class-specific
representation win into a broader structured-data portfolio win.
