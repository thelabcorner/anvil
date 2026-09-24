# ANVIL I10 — GROTLI G1-CEILING Corpus Freeze

**Frozen:** 2026-09-23
**Applies to:** `I10-GROTLI-G1-CEILING-PREREG.md`
**State:** freeze before G1 implementation/outcome measurement
**Purpose:** prevent favorable-file selection and held-out leakage

---

## 0. Split

G1 has five real structured-data families.

Discovery:

- D1 — Amazon cellphone NDJSON;
- D2 — CDISC ADaM adverse-event NDJSON;
- D3 — GH Archive 10 MiB NDJSON;
- D4 — CROVIA DPI royalty receipts NDJSON.

Held-out validation:

- V1 — Sino-US DrugQA authoritative bilingual release.

D1-D3 are already open because G0 measured them or exposed their anatomy. They
remain useful discovery families but no longer count as unseen validation.

D4 is new discovery data frozen before G1 implementation.

V1 is new validation data. Its immutable object identity may be verified before
implementation, but its G1 compressed result must not be inspected until the G1
implementation commit is frozen and discovery closes.

No corpus bytes are vendored into ANVIL for this experiment.

---

## 1. D1 — Amazon cellphone NDJSON

**Role:** discovery A
**Repository:** `simdjson/simdjson-data`
**Commit:** `4197c425e857f0ec38e89822fdd0bd9ea21f4daf`
**Path:** `jsonexamples/amazon_cellphones.ndjson`
**Git blob:** `9cbb6a071eaa2a10c24d9b4d1729dca501f2f72b`
**Bytes:** 277,673
**License note:** repository metadata does not expose an SPDX license; do not redistribute.

This is unchanged from G0 D1.

---

## 2. D2 — CDISC ADaM Dataset-JSON NDJSON

**Role:** discovery B
**Repository:** `cdisc-org/DataExchange-DatasetJson`
**Commit:** `a379f49a3f43c2aaed63bdeca761bdb7140df2c3`
**Path:** `examples/adam/adae.ndjson`
**Git blob:** `4bb9ef50f650767967fc3f0a663c73d33c57c608`
**Bytes:** 615,350
**License:** MIT

This is unchanged from G0 D2.

---

## 3. D3 — GH Archive 10 MiB NDJSON

**Role:** discovery C / heterogeneous event diagnostic
**Repository:** `rushikeshmore/DataCortex`
**Commit:** `9bf63551499974e7398b87b45d1a9d4b50936566`
**Path:** `corpus/json-bench/gharchive-10mb.ndjson`
**Git blob:** `59d1d00825053e9894a7895fcea0665912524ec8`
**Bytes:** 10,485,760
**License:** MIT

This was G0 V1 and is now fully open. G1 may use it for discovery/anatomy.

Its inclusion is strategically useful because the file is heterogeneous and is
also used by close prior art DataCortex. No G1 result should be represented as
held-out evidence on this file.

---

## 4. D4 — CROVIA DPI royalty receipts NDJSON

**Role:** new discovery D
**Repository:** `croviatrust/crovia-core`
**Commit:** `1798a9c9138c6efc3fcaf487ac5e9833008f6ad9`
**Path:** `demo_dpi_2025-11/data/dpi_royalty_receipts.ndjson`
**Git blob:** `24020a024a05a68731a2d4de4536bd7ca3eb8576`
**Bytes:** 3,585,053
**License:** MIT

The repository describes these as 3,718 real Data Provenance Initiative
finetuning-dataset attribution/royalty receipt records.

Why include it:

- object-per-line records;
- stable audit/receipt structure;
- metadata-rich strings and nested objects;
- independent provenance from the G0 families;
- materially larger than the minimum 256 KiB headline floor.

D4 is discovery, not held-out. Once G1 implementation work starts it may be
inspected for correctness/anatomy.

---

## 5. V1 — Sino-US DrugQA authoritative bilingual release

**Role:** held-out validation
**Repository:** `DodgeLU/Sino-US-DrugQA`
**Commit:** `cd026a27a5e039863de9080b8d7d95acaf1f5a39`
**Path:** `data/release/all.jsonl`
**Git blob:** `8d9e6acf3f534194487532fa60d4f1d43fe71d00`
**Bytes:** 15,374,047
**Repository metadata license:** NOASSERTION — do not redistribute

The repository identifies this file as the authoritative bilingual release and
documents 11,444 JSONL items.

Why it is a good held-out family:

- new corpus not used by G0;
- much larger than the discovery floor;
- repeated high-level record schema;
- multilingual/high-cardinality long text;
- nested arrays and mixed scalar fields;
- very different semantic domain from Amazon, clinical Dataset-JSON,
  GitHub events, and attribution receipts.

Only immutable provenance may be inspected before G1 freezes.

Do not inspect:

- RAW vs SHAPE_ROW/SHAPE_COLUMN compressed results;
- shape-count/coverage diagnostics if they could motivate layout changes;
- operator-specific result differences;

until the implementation commit is frozen.

---

## 6. Acquisition verification

Every workflow fetch must verify:

1. exact repository;
2. exact commit;
3. exact path;
4. exact byte length;
5. Git blob SHA-1.

For downloaded bytes compute:

    SHA1("blob " + decimal_length + NUL + bytes)

and require equality with the frozen blob.

Also compute SHA-256 and place it in the run artifact.

A first successful remote fetch may append SHA-256 values here as provenance
only. It may not change corpus selection or pass bars.

---

## 7. Discovery/validation firewall

Discovery implementation may use D1-D4 to:

- fix lexical-parser correctness;
- fix frozen carrier correctness;
- inspect exact cost decomposition;
- diagnose why S1/S2 win or lose;
- optimize implementation speed without changing representation semantics.

It may not change:

- placeholder semantics;
- shape identity;
- S1/S2 logical layout;
- pass thresholds;

without issuing a new G1 version and forfeiting V1 as unseen validation.

V1 may be opened only after:

1. D1-D4 roundtrip exactly;
2. malformed/truncated/trailing carrier tests pass;
3. RAW/S1/S2 byte accounting is complete;
4. the implementation commit SHA is frozen;
5. discovery outcome is recorded.

The validation job must checkout the exact frozen discovery SHA.

---

## 8. Headline interpretation

The discovery split intentionally contains both easy and hostile structured
families.

A useful architecture should not require every structured file to benefit.
Raw Brotli remains a mandatory fallback.

The held-out test asks the strongest near-term question:

> Does a representation design chosen without seeing V1's compression outcome
> retain any complete-byte advantage on a new, large, semantically different
> structured JSONL family?
