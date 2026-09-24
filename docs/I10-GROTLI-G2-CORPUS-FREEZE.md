# ANVIL I10 — GROTLI G2 Typed Column Expert Corpus Freeze

**Frozen:** 2026-09-24
**Applies to:** I10-GROTLI-G2-TYPED-PREREG.md
**State:** freeze before G2 implementation/outcome measurement
**Purpose:** preserve discovery/validation discipline after G1

---

## 0. Relationship to G1

G2 reuses the exact immutable corpus objects frozen for G1.

G1 opened only D1-D4 discovery.

The held-out Sino-US DrugQA V1 object was never fetched or measured because G1
did not pass discovery. The G1 Actions run contains only a discovery artifact
and records validation as skipped.

Therefore V1 remains available as a genuinely unspent held-out validation family
for G2.

G2 does not retroactively redefine G1.

---

## 1. Discovery families

### D1 — Amazon cellphone NDJSON

Repository: simdjson/simdjson-data
Commit: 4197c425e857f0ec38e89822fdd0bd9ea21f4daf
Path: jsonexamples/amazon_cellphones.ndjson
Git blob: 9cbb6a071eaa2a10c24d9b4d1729dca501f2f72b
Bytes: 277,673
SHA-256 observed in G1:
c1518fdaaed45e590c480ed707aa1adaaba8b84b10747f956bd431c708bd590e

G1 status: structured eligible; SHAPE_COLUMN -1.4853% versus raw Brotli.

### D2 — CDISC ADaM adverse-event NDJSON

Repository: cdisc-org/DataExchange-DatasetJson
Commit: a379f49a3f43c2aaed63bdeca761bdb7140df2c3
Path: examples/adam/adae.ndjson
Git blob: 4bb9ef50f650767967fc3f0a663c73d33c57c608
Bytes: 615,350
SHA-256 observed in G1:
b795a59c5c92a8fc93d7d3f729fa0dae014e9b0f684a6a7a7bcf07e4104ba723

G1 status: structured eligible; raw Brotli wins; best structured arm +2.4971%.

### D3 — GH Archive 10 MiB object

Repository: rushikeshmore/DataCortex
Commit: 9bf63551499974e7398b87b45d1a9d4b50936566
Path: corpus/json-bench/gharchive-10mb.ndjson
Git blob: 59d1d00825053e9894a7895fcea0665912524ec8
Bytes: 10,485,760
SHA-256 observed in G1:
a860af236f794779b5471b416b2e6d7ea68de00f6d17728dd3b088a7ecec8252

G1 status: exact lexical parser reports an unterminated JSON string.

G2 role:

> **raw-fallback routing control only.**

Do not count D3 in the denominator of structured-family win requirements.

Do include its raw Brotli bytes in aggregate routed-portfolio accounting so the
portfolio cannot hide the cost/benefit of an unavailable expert.

### D4 — CROVIA DPI royalty receipts NDJSON

Repository: croviatrust/crovia-core
Commit: 1798a9c9138c6efc3fcaf487ac5e9833008f6ad9
Path: demo_dpi_2025-11/data/dpi_royalty_receipts.ndjson
Git blob: 24020a024a05a68731a2d4de4536bd7ca3eb8576
Bytes: 3,585,053
SHA-256 observed in G1:
0db7ace9e46ce458a7055f48b09aabc7df15f4f74a23553d3660b7de61d0916f

G1 status: structured eligible; SHAPE_COLUMN -21.7717% versus raw Brotli.

---

## 2. Held-out V1 — Sino-US DrugQA

Repository: DodgeLU/Sino-US-DrugQA
Commit: cd026a27a5e039863de9080b8d7d95acaf1f5a39
Path: data/release/all.jsonl
Git blob: 8d9e6acf3f534194487532fa60d4f1d43fe71d00
Bytes: 15,374,047

Status at G2 freeze:

> **unopened for ANVIL compression/anatomy outcomes.**

The object identity was pinned before G1 implementation, but the G1 validation
job was skipped and no validation artifact exists.

G2 must not fetch V1 until:

1. D1/D2/D4 typed discovery completes;
2. D3 raw fallback is verified;
3. G2 discovery satisfies its preregistered gate;
4. G2 implementation SHA is frozen.

Validation must checkout that exact frozen G2 implementation SHA.

---

## 3. Acquisition contract

Every remote run verifies:

- exact repository/commit/path;
- exact byte length;
- Git blob SHA-1;
- SHA-256.

No corpus bytes are vendored into ANVIL.

Any upstream mismatch is infrastructure failure, not a compression result.

---

## 4. Discovery/validation firewall

Discovery may inspect D1-D4.

Discovery may tune implementation correctness/performance only while preserving
the frozen G2 representation/search semantics.

If discovery does not pass:

- do not fetch V1;
- close the G2 broad gate;
- V1 remains unspent.

If discovery passes:

- freeze the exact implementation SHA;
- open V1 once;
- no representation/search change may occur before V1 ruling.

---

## 5. Why no new discovery family is added in G2

G2 is not a fresh generalization claim.

Its causal question is narrower:

> Do a small number of typed column leaves broaden the exact G1
> column-locality expert?

D1, D2 and D4 already provide three materially different structured populations:

- D1: one exact shape, mostly high-cardinality values;
- D2: dominant tabular array shape where G1 loses despite a much smaller
  pre-Brotli carrier;
- D4: multi-shape object records where G1 columnization wins strongly.

D3 remains an explicit unavailable-expert routing control.

A new held-out structured family is unnecessary because V1 is still genuinely
unspent.

If G2 passes V1, later production/generalization work must add additional
external families rather than treating one held-out JSONL file as universal
proof.
