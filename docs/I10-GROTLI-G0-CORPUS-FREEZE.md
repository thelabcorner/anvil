# ANVIL I10 — GROTLI-ANVIL-0 Corpus Freeze

**Frozen:** 2026-09-23
**Applies to:** I10-GROTLI-G0-PREREG.md
**State:** frozen before G0 implementation or outcome measurement
**Purpose:** prevent favorable-file selection and validation leakage

---

## 0. Rules

G0 has three real structured-data families.

- D1 and D2 are **discovery** families.
- V1 is **validation-only**.
- V1 may be used to verify fetch/roundtrip plumbing before the codec exists, but
  its compression outcome may not be used to change G0 transform semantics,
  thresholds, pass bars, or carrier layout.
- Synthetic/local fixtures are correctness and provenance controls only.
- Negative/general controls do not count toward the real-structured pass bar.
- No corpus byte is committed into ANVIL merely to make the benchmark
  convenient. Fetch exact immutable upstream objects at run time.

The G0 pre-registration requires at least 256 KiB from each independent real
dataset family used for headline eligibility. All three frozen families exceed
that floor.

---

## 1. Immutable real-data manifest

### D1 — Amazon cellphone NDJSON

**Role:** discovery family A
**Shape:** object-per-line consumer/product records
**Repository:** simdjson/simdjson-data
**Repository commit:** 4197c425e857f0ec38e89822fdd0bd9ea21f4daf
**Path:** jsonexamples/amazon_cellphones.ndjson
**Git blob:** 9cbb6a071eaa2a10c24d9b4d1729dca501f2f72b
**Bytes:** 277,673
**Headline-floor eligible:** yes

Upstream describes the file as Amazon cell-phone data in newline-delimited JSON.

Reference:
- https://github.com/simdjson/simdjson-data

**Distribution note:** GitHub repository metadata does not currently expose an
SPDX license for simdjson-data. ANVIL therefore does not vendor or redistribute
this fixture. The benchmark fetches the pinned upstream object.

### D2 — CDISC ADaM adverse-event Dataset-JSON NDJSON

**Role:** discovery family B
**Shape:** standardized clinical Dataset-JSON; metadata record followed by data
rows represented as JSON arrays
**Repository:** cdisc-org/DataExchange-DatasetJson
**Repository commit:** a379f49a3f43c2aaed63bdeca761bdb7140df2c3
**Path:** examples/adam/adae.ndjson
**Git blob:** 4bb9ef50f650767967fc3f0a663c73d33c57c608
**Bytes:** 615,350
**Headline-floor eligible:** yes
**Repository license:** MIT

The Dataset-JSON NDJSON specification states that line 1 contains dataset
metadata/column definitions and subsequent lines contain data rows. This is
structurally different from D1 and is intentionally retained as a separate
family.

References:
- https://github.com/cdisc-org/DataExchange-DatasetJson
- https://github.com/cdisc-org/DataExchange-DatasetJson/blob/master/doc/dataset-json-ndjson1-1.md

### V1 — GH Archive 10 MiB NDJSON

**Role:** validation-only / competitive replication family
**Shape:** heterogeneous real GitHub event records
**Repository copy:** rushikeshmore/DataCortex
**Repository commit:** 9bf63551499974e7398b87b45d1a9d4b50936566
**Path:** corpus/json-bench/gharchive-10mb.ndjson
**Git blob:** 59d1d00825053e9894a7895fcea0665912524ec8
**Bytes:** 10,485,760
**Headline-floor eligible:** yes
**Repository license:** MIT

This file is deliberately validation-only for two reasons:

1. it is heterogeneous rather than a near-uniform schema;
2. DataCortex already uses the same pinned corpus file in its own benchmark
   suite, so it provides a direct competitive reality check.

DataCortex currently reports an author-measured compression ratio of about 8.0x
on its GH Archive 10 MB row versus about 7.7x for Brotli-11. That number is
context only and is not an ANVIL result.

Reference:
- https://github.com/rushikeshmore/DataCortex

No G0 parameter may be changed because V1 loses.

---

## 2. Why these three families

The split is deliberately heterogeneous.

### D1 tests repeated object structure

Expected opportunity:
- similar keys and lexical scaffolding;
- record-aligned byte positions;
- variable strings/numbers.

Risk:
- varying value lengths shift later byte positions and can destroy vXOR
  alignment.

### D2 tests fixed semantic columns without object-key repetition on every row

Expected opportunity:
- later rows are arrays with stable field order;
- similar lexical positions across data rows.

Risk:
- the first metadata row is radically different;
- nullability and variable lexical widths can shift later positions.

### V1 tests schema heterogeneity

Expected opportunity:
- repeated GitHub event scaffolding exists.

Risk:
- multiple event types and nested payload shapes;
- record lengths and byte positions vary substantially.

This is intentionally difficult for G0's one global record-aligned predictor.
A G0 failure on V1 can still motivate G1 schema grouping only if D1/D2 first
establish that the exact vXOR representation itself has real value.

---

## 3. Synthetic/local controls

These do not count toward generalization.

### Positive/provenance

- tests/corpus/synth-ndjson-columnar.ndjson
- tests/corpus/synth-counters.log
- any historical Grotli fixture later recovered with explicit provenance

### Negative/general

At minimum:
- canonical enwik8;
- Silesia representative text;
- Silesia representative binary;
- deterministic uniform-random fixture;
- prose;
- irregular log/text records.

FORCED-vXOR is allowed to lose badly on negative controls.

The selected prototype portfolio always retains raw Brotli inside the common
prototype envelope.

---

## 4. Acquisition verification

Before any benchmark, the GitHub Actions fetch step must verify for every
pinned GitHub object:

1. repository;
2. commit;
3. path;
4. exact byte length;
5. Git blob object ID.

For downloaded bytes, compute the Git object hash as SHA1 over:

    "blob " + decimal_length + NUL + file_bytes

and require equality with the frozen blob ID.

Also compute and report SHA-256 in the run artifact.

The first valid remote fetch may append SHA-256 values to this document, but
that append is provenance only and may not alter corpus selection.

---

## 5. Invalid local hash attempt — explicitly excluded

During corpus freezing, an initial Windows PowerShell command redirected
gh-api text output with the PowerShell greater-than operator.

Windows PowerShell transcoded the UTF-8 NDJSON to UTF-16, approximately doubling
the expected byte sizes. The resulting SHA-256 values are invalid and are not
evidence.

The error was detected because downloaded sizes did not equal GitHub's immutable
blob metadata.

No value from that transcoded attempt may appear in a benchmark manifest.

The frozen source identity is the repository commit + path + Git blob + expected
byte size recorded in section 1.

---

## 6. Discovery/validation protocol

Implementation may use D1 and D2 to:
- fix correctness bugs;
- implement the already-frozen carrier;
- inspect charged cost decomposition;
- profile code;
- report why the frozen transform wins/loses;
- evaluate the already-frozen historical detector features.

Implementation may not change the G0 transform itself without issuing a new
pre-registration/version and forfeiting V1 as held-out validation for that new
version.

V1 is opened for compression-result inspection only after:

1. D1/D2 exact roundtrip passes;
2. malformed carrier tests pass;
3. raw and vXOR q11/lw30 arms are byte-accounting complete;
4. the implementation commit is frozen.

After V1 is measured, there is no tuning-and-rerun loop under G0.

A discovery-driven G0.1/G1 design may be created afterward with a new validation
split.

---

## 7. Competitive DataCortex reproduction is separate

DataCortex is close prior art for the future G1 JSON/NDJSON layer.

G0's V1 row does not attempt to reproduce DataCortex itself; it compares only:

    raw Brotli q11/lw30
    versus
    exact frozen G0 vXOR carrier + Brotli q11/lw30

A later competitive benchmark may build the pinned DataCortex release/commit
and compare complete archives and throughput on the same corpus.

That is a separate experiment because DataCortex uses substantially more
structure than G0:
- schema inference;
- uniform/grouped/selective columnar layouts;
- typed encodings;
- zstd/Brotli path arbitration;
- additional transform metadata handling.

Do not attribute a G0 loss to the whole SRS thesis merely because a much richer
competitor wins V1.

---

## 8. Frozen interpretation

The three-family split answers three progressively harder questions:

- D1: can exact record-position prediction improve ordinary object NDJSON?
- D2: does that signal transfer to a structurally different real standard?
- V1: does the simple predictor survive heterogeneous real event streams?

G0 is not expected to solve every one.

The pass ruling remains the one frozen in I10-GROTLI-G0-PREREG.md.

The strategic result can therefore be diagnostic:

- D1/D2 strong + V1 weak -> G1 needs grouping/shape-aware prediction;
- D1 strong + D2 weak -> object lexical scaffolding may be the real source;
- D1/D2/V1 strong -> record-position prediction is much broader than expected;
- all weak -> do not build a broad SRS solely from the old Grotli vXOR result.
