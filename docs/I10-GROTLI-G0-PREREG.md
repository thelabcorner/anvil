# ANVIL I10 — GROTLI-ANVIL-0 Pre-Registration

**Frozen:** 2026-09-23
**Closed:** 2026-09-23 — **NO-GO-REPRESENTATION**; see [I10-GROTLI-G0-RESULTS.md](I10-GROTLI-G0-RESULTS.md)
**Class:** engineering / representation-capability experiment; no mechanism-novelty claim
**Production source changes:** forbidden until this experiment closes
**Compute policy:** compression/decompression measurements run on GitHub Actions only
**Frozen corpus/split:** [I10-GROTLI-G0-CORPUS-FREEZE.md](I10-GROTLI-G0-CORPUS-FREEZE.md)

---

## 0. Question

Grotli's strongest structured-data result suggested that record-aligned vertical
XOR could make a JSON-lines-like byte stream much easier for Brotli to compress.

The historical result was not a complete production-wire result because the
representation did not fully charge/serialize all information required to
reconstruct variable record lengths.

G0 asks exactly one question:

> **After making the strongest Grotli vXOR representation completely
> byte-reversible and charging every required byte, does Brotli q11/lw30 still
> compress that representation materially smaller than the original bytes?**

Nothing else is allowed into G0.

---

## 1. Hypothesis

For some real record-structured byte streams:

    Brotli_q11_lw30(exact_vxor_carrier(X)) < Brotli_q11_lw30(X)

by a material amount after charging the carrier version, record count, stride,
all record lengths, transformed bytes, and any unequal outer framing.

If true, the gain is a representation gain because the entropy backend is
identical in both arms.

If false, the historical Grotli signal did not survive a complete exact wire and
G1/SRS does not receive automatic authorization.

---

## 2. Frozen representation

### 2.1 Record extent

G0 is byte-structural. It does not parse JSON values.

For the line-record lane:

- split after every LF byte (0x0A);
- a preceding CR is ordinary record data and is preserved;
- the LF itself belongs to the record ending at that LF;
- the final unterminated suffix, if nonempty, is one final record;
- empty records are permitted by the byte transform.

Therefore record_len is the complete byte extent including the delimiter when a
delimiter exists.

The stride is:

    stride = max(record_len)

Because record_len already includes the line ending, this is equivalent to the
older requirement that the stride include the full record plus delimiter extent.

### 2.2 Matrix

For R records and S = max(record_len):

    M[r,c] = record[r][c]   if c < record_len[r]
             0              otherwise

Zero padding is an intermediate representation, not source data.

### 2.3 Vertical XOR

Frozen G0-A transform:

    D[0,c] = M[0,c]

    D[r,c] = M[r,c] XOR M[r-1,c]     for r > 0

Serialize D in row-major order.

G0 deliberately does not add a column transpose. That would be a different
transform and would re-open old global-transpose questions.

### 2.4 Exact carrier before Brotli

Logical carrier:

    magic/version
    decoded_source_len
    record_count
    stride

    repeat record_count:
        record_len

    xor_matrix_len
    xor_matrix_bytes

All integer fields are canonical unsigned varints in the prototype.

Requirements:

- sum(record_len) equals decoded_source_len;
- max(record_len) equals stride;
- xor_matrix_len equals record_count times stride;
- multiplication/length checks occur before allocation;
- decoder rejects trailing or underconsumed carrier bytes;
- reconstruction consumes the carrier exactly.

The entire carrier, including the record-length stream, is fed through the same
Brotli q11/lw30 backend used by ANVIL's current external reference rows. No
representation metadata is free.

### 2.5 Raw control

Control representation:

    Brotli_q11_lw30(source_bytes)

The benchmark reports both raw Brotli payload bytes and complete
prototype-envelope bytes. Any unequal envelope bytes are charged.

---

## 3. Roundtrip contract

Decoder:

1. Brotli-decode the carrier.
2. Parse and bound-check metadata.
3. Reconstruct the matrix row by row:
   - row 0 equals D row 0;
   - row r equals D row r XOR the previous reconstructed padded row.
4. Emit only the first record_len[r] bytes of each row.
5. Verify emitted byte count equals decoded_source_len.

Required identity:

    SHA256(decoded) == SHA256(source)

for every candidate measured.

No semantic JSON parse or reserialization is allowed. Consequently whitespace,
number spelling, escape spelling, key order, duplicate keys, line endings, and
a missing final LF are preserved exactly.

---

## 4. Why G0 uses padded row-major vXOR

Obvious possible upgrades exist:

- omit padded cells;
- compress length deltas specially;
- group records to reduce outlier stride;
- transpose columns;
- shape-aware alignment;
- field-aware transforms.

They are intentionally excluded from G0.

G0 is a closure experiment on the historical Grotli signal. If it wins, those
become G0.1/G1 ablations. If it loses, the failure decomposition tells us which
charged term killed the signal.

---

## 5. Hard safety/resource bounds

Raw Brotli is always reachable.

The vXOR candidate may be skipped, never forced, when a hard resource bound is
violated.

Prototype bounds:

- record_count at least 8 for vXOR eligibility;
- stride at most 4 MiB;
- record_count times stride at most 2^34 bytes;
- all products checked for overflow before allocation;
- encoder should stream rows into the carrier/Brotli encoder where practical;
- decoder should retain only previous/current padded rows where practical.

These are resource guards, not compression heuristics.

---

## 6. Measurement lanes

### 6.1 FORCED-vXOR

Every eligible line-record input gets an exact vXOR candidate.

Purpose:

> measure pure representation opportunity and failure causes.

A forced-vXOR loss is valid evidence.

### 6.2 SELECTED portfolio

For every file:

    selected_bytes = min(
        complete_raw_brotli,
        complete_vxor_brotli
    )

Raw wins ties.

Purpose:

> measure the achievable candidate-level no-regression result inside the common
> prototype envelope. External standalone-Brotli comparisons still charge any
> unequal ANVIL/container framing.

### 6.3 Detector-scout

Detector effectiveness is measured separately from representation value.

Preserve the historical Grotli-derived signals as features rather than
retunable outcome-driven constants:

- record-length distribution;
- delimiter record-length CV;
- sampled vXOR zero density;
- sampled raw/vXOR H0;
- optional first 512-byte anatomy windows.

The preserved Grotli pre-registration used delimiter CV below 0.20 as a
record-alignment signal and required residual-entropy improvement before
engaging specialized routes.

For G0, detector misses must not hide representation opportunity.

Report:

- true-positive rate against exact final vXOR wins;
- false-positive rate;
- candidate evaluations avoided;
- bytes foregone by false negatives.

Do not tune detector constants on the held-out set.

---

## 7. Exact metrics

For each file record:

### Source

- source bytes;
- SHA256;
- record count;
- min/median/p95/max record length;
- record-length CV;
- padding expansion: record_count times stride divided by source bytes.

### Raw control

- Brotli q11/lw30 bytes;
- encode time;
- decode time;
- peak encode RSS;
- peak decode RSS.

### vXOR representation

- carrier bytes before Brotli;
- descriptor bytes;
- record-length bytes before Brotli;
- xor-matrix bytes;
- padding bytes;
- vXOR zero density;
- raw H0 diagnostic;
- vXOR H0 diagnostic;
- carrier+Brotli q11/lw30 bytes;
- byte delta and percent versus raw Brotli;
- encode/decode time;
- peak encode/decode RSS;
- SHA roundtrip.

### Portfolio

- selected arm;
- selected complete bytes;
- aggregate selected bytes;
- aggregate raw-Brotli bytes;
- aggregate delta.

Diagnostics such as H0 are not the ruling. Exact compressed bytes are.

---

## 8. Corpus contract

Synthetic/generated Grotli-derived fixtures are regression/provenance controls,
not generalization evidence.

### 8.1 Positive/regression controls

- synth-ndjson-columnar.ndjson;
- historical Grotli fixture if provenance permits inclusion;
- synth-counters.log as a structurally different text-record probe.

### 8.2 Independent real structured datasets

Before the citation-grade run, pin commit/hash provenance for at least two
independent real NDJSON/JSONL sources.

Headline eligibility requires each independent real dataset family to contribute
at least **256 KiB** of source bytes after any deterministic concatenation of its
pinned files. Tiny examples may remain correctness/provenance controls but cannot
satisfy the >=5% pass rows by themselves.

Current candidates:

1. simdjson-data
   - jsonexamples/amazon_cellphones.ndjson;
   - independent parser corpus with Amazon-record data.

2. CDISC Dataset-JSON NDJSON examples
   - standard-shaped clinical data exchange examples;
   - metadata row plus data rows gives a structurally different record class.

Additional real datasets may be added before the frozen run if they are publicly
redistributable, content-hash pinned, and not generated to favor vXOR.

Candidate provenance:
- https://github.com/simdjson/simdjson-data
- https://github.com/swhume/dataset-ndjson
- https://github.com/cdisc-org/DataExchange-DatasetJson

### 8.3 Negative/general controls

At minimum:

- enwik8;
- representative Silesia text/binary files;
- random incompressible bytes;
- prose;
- irregular log/text records.

FORCED-vXOR may lose on these. The selected portfolio must retain raw Brotli.

---

## 9. Discovery / validation split

Real structured files are divided before implementation tuning.

Discovery data may be used to debug correctness, understand failure causes,
inspect detector features, and profile implementation.

Validation data must not be used to change transform semantics, detector
thresholds, representation gates, or success thresholds.

If the public corpus is too small for a meaningful split, collect more
independent data instead of repeatedly tuning on the same files.

---

## 10. Pre-registered ruling

### PASS-REPRESENTATION

G0 passes representation viability when all are true:

1. exact SHA roundtrip on every candidate;
2. at least two independent real structured datasets achieve complete
   vXOR+Brotli bytes at least 5% below complete raw-Brotli bytes;
3. aggregate real-structured selected bytes are smaller than raw Brotli after
   every carrier byte is charged;
4. a held-out real structured set retains at least 1% aggregate improvement;
5. within the prototype envelope, raw Brotli wins ties so the structured route
   cannot enlarge the selected candidate; final external Brotli comparisons still
   charge any ANVIL/container framing not shared by standalone Brotli;
6. inverse vXOR + carrier reconstruction is no worse than 1.25x raw-Brotli
   decode time on the balanced scout;
7. malformed-wire/allocation tests pass.

This authorizes G1/SRS research.

### PASS-WEAK

Exact real-data improvement exists but the 5% / held-out bar is not met.

Action:

- record it as adopt-class evidence;
- inspect whether one clearly charged cost dominates;
- do not automatically authorize a broad SRS implementation.

### NO-GO-REPRESENTATION

After complete wire accounting, vXOR does not beat raw Brotli materially on
independent real data.

Action:

- record why;
- do not re-burn by retuning thresholds/metadata until another oracle identifies
  a specific missing representation.

---

## 11. Required ablations if G0 passes

Only after PASS-REPRESENTATION:

### A1 — length-stream cost

Compare canonical uvar lengths against delta/RLE length representation, with
Brotli applied to the complete carrier in both cases.

### A2 — outlier-stride tax

Compare the frozen global carrier against predeclared fixed record groups:

- 64;
- 256;
- 1024 records/group.

Each group gets its own stride and complete header.

### A3 — padded versus compact row residual

Test whether padded cells should exist in the logical carrier at all. Any
compact form must stay exactly reconstructable and charge its presence metadata.

### A4 — Brotli quality

Primary ruling remains q11. Optional q4/q6/q9 rows may characterize encode-time
tradeoffs but cannot replace the q11 causal result.

### A5 — backend substitution

Only after the Brotli result is established:

- BWT(carrier);
- direct lightweight carrier with no heavyweight backend.

This asks whether the representation is intrinsically good or specifically
Brotli-friendly.

---

## 12. What G0 must not become

G0 does not include:

- JSON semantic parsing;
- column dictionaries;
- numeric delta/DoD;
- Gorilla float;
- ALP;
- FSST;
- field inference;
- multi-column predictors;
- DEFLATE replay;
- new ANVIL transform IDs;
- source changes in src/anvil.cpp.

Those belong to G1/SRS only if G0 establishes the core causal thesis.

---

## 13. Planner lesson carried into G1

If G0 passes, G1 uses a sampled portfolio architecture rather than Grotli v4's
expensive recursive full-candidate evaluation.

External precedent supports this:

- BtrBlocks uses statistics + samples to prune/select lightweight schemes;
- ALP uses two-level row-group/vector sampling and a small candidate preset;
- FastLanes uses a two-phase encoding-expression search.

G1 rule:

    anatomy
      -> eliminate impossible families
      -> sample viable candidates
      -> retain small beam K
      -> serialize/compress only finalists
      -> exact downstream-size arbitration

Raw Brotli remains in the finalist set by construction.

References:
- https://doi.org/10.1145/3589263
- https://doi.org/10.1145/3626717
- https://doi.org/10.14778/3749646.3749718

---

## 14. Claim discipline

A G0 pass supports:

> "record-aligned reversible representation can make Brotli compress some real
> structured byte streams materially smaller than Brotli on the original
> representation."

It does not support a universal ANVIL>Brotli claim, novelty of XOR/delta/columnar
encoding, a full Pareto crossing, or generalization beyond measured structured
classes.

The strategic importance is causal:

> **If the same Brotli backend becomes materially smaller solely because ANVIL
> found a better exact representation, adaptive representation synthesis is a
> validated route toward an ANVIL portfolio that beats raw Brotli overall.**
