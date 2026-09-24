# ANVIL I10 — GROTLI G3 Regionized Residual Expert Preregistration

**Status:** FROZEN BEFORE IMPLEMENTATION
**Date:** 2026-09-24
**Freeze revision:** r2 — causal-hardening amendment made before any G3 implementation/workflow existed
**Initial public freeze:** `9c65ae1`
**Parent evidence:** `I10-GROTLI-G2-RESULTS.md`
**Production integration authorized:** no
**Held-out V1 authorized:** no

Revision r2 does not change the G3 hypothesis, corpus, leaf basis, or discovery
threshold. It closes two preregistration ambiguities before implementation:

1. structured frames retain the frozen G2 LF-inclusive frame/parser semantics;
   G3 may not gain from separately factoring line terminators;
2. held-out success must be earned by a **G3 regionized arm**; `G2_WHOLE` may
   remain a fallback/context candidate but cannot satisfy the G3 mechanism gate.

---

## 0. Question

G2 established that the small typed basis can produce large same-Brotli wins on
D2 and D4, but it failed the broad discovery gate because the frozen D3 object
was globally ineligible after an `unterminated JSON string`.

G3 asks one new question:

> **Can ANVIL make structured representation a local property of recoverable
> records/regions, while preserving unparseable material as exact raw residuals,
> so a local parse failure no longer disables the structured expert for the
> entire object?**

G3 does **not** ask whether more leaf codecs improve compression.

The G2 leaf basis remains frozen.

---

## 1. Causal hypothesis

For line-framed structured text, whole-file parser eligibility is unnecessarily
coarse.

A file can contain:

- many complete valid records;
- a small number of malformed/unsupported records;
- an incomplete final fragment;
- blank lines or other exact lexical material.

A reversible carrier can represent valid records structurally while preserving
all unsupported bytes as raw residuals and reconstructing exact original order.

The causal comparison remains:

    Brotli_q11_lgwin30(raw original bytes)

versus:

    Brotli_q11_lgwin30(complete reversible regionized carrier)

If G3 improves D3 while the G2 operator basis is unchanged, the gain is
attributable to **regionized structure availability / residualization**, not to a
new leaf codec.

---

## 2. Why this is materially different from G2

G2 eligibility is whole-object.

One parse failure can make the entire structured candidate unavailable.

G3 changes the representation domain:

    whole-file structured eligibility

becomes:

    exact line/record framing
      -> independently parseable records
      -> structured groups
      + raw residual records/fragments
      -> exact original-order reconstruction

This is a new representation hypothesis.

It does not modify G2 after seeing its result.

---

## 3. Frozen scope

### 3.1 Discovery format

G3's new regionized path is limited to **NDJSON / line-framed JSON records**.

Exact byte framing is by line boundaries.

The implementation must preserve:

- every byte of every record;
- original line terminators;
- blank lines;
- final unterminated fragment;
- malformed/unsupported lines;
- original record/residual order.

A line/fragment that cannot be represented by the exact lexical structured path
is encoded as raw residual bytes.

### 3.2 Existing whole-object path

For fully eligible files, the frozen G2 whole-object path remains available as a
comparison/control candidate.

G3 must not require regionization to replace a smaller G2 candidate.

### 3.3 Production source

Do not modify `src/anvil.cpp`.

G3 remains a standalone research prototype.

---

## 4. Frozen leaf basis

Structured records use exactly the G2 leaves:

1. `RAW_LEX`;
2. `EXACT_DICT`;
3. `INT_FOR`;
4. `INT_DELTA_FOR`;
5. `INT_DOD_FOR`.

No other leaf is allowed in G3.

Explicitly excluded:

- RLE/default-exception as a new leaf;
- Gorilla;
- ALP;
- FSST/string symbols;
- byte shuffle / bit-plane transforms;
- cross-column prediction;
- schema unioning;
- learned grammar;
- replay;
- TCOPY;
- alternate entropy backends.

This is necessary to keep the experiment attributable.

---

## 5. Frozen structured semantics

G3 reuses the G2 exact lexical JSON semantics.

For every valid structured record:

- syntax/template bytes remain exact;
- scalar tokens remain exact lexical bytes;
- duplicate-key behavior is whatever the frozen G2 lexical parser already
  supports/rejects;
- number spellings are not normalized;
- strings are not semantically unescaped/re-emitted;
- key order is not changed;
- whitespace that belongs to the record must reconstruct exactly.

Shape identity remains exact G2 shape identity.

No schema merging or semantic normalization is introduced.

---

## 6. Exact line/fragment framing

The source is scanned as bytes using the **same framing rule as frozen G2**.

G2's `split_records()` ends a frame only on byte `0x0A` (LF) and includes that
byte in the frame extent. Therefore G3 freezes the same rule:

- every extent ending at an LF includes the LF byte;
- CRLF is not normalized or split specially: the CR and LF both remain inside
  the exact frame bytes;
- if bytes remain after the final LF, the entire remainder is one final frame;
- no other byte sequence is promoted to a record boundary in G3.

For a structured frame, the **entire original frame extent**, including its
trailing LF/CRLF when present, is passed to the frozen G2 lexical parser. The
parser already treats CR/LF as JSON whitespace, and the exact trailing bytes are
therefore retained in the G2 structural/template parts.

For a residual frame, the complete original frame bytes are retained literally.

The primary G3 carrier must not introduce an independent line-terminator code or
terminator side stream. That would be a second representation change and would
confound the region-availability experiment.

The decoder never discovers line boundaries or line endings heuristically. It
reconstructs exact source order from decoder-visible carrier state.

---

## 7. Structured versus residual record classification

For each exact G2-compatible frame extent:

1. pass the **entire frame bytes** to the frozen G2 exact lexical parser;
2. if accepted, classify the frame as structured and derive the same exact
   scalar spans / structural parts / shape identity G2 would derive for that
   frame;
3. otherwise classify the complete original frame as raw residual.

Blank/whitespace-only frames are raw residuals because the frozen parser rejects
them as blank JSON records.

A malformed record must never make another valid record unavailable.

A final remainder that is incomplete/invalid is retained exactly as raw
residual. A final remainder that is itself a complete valid JSON value remains
eligible for the structured path, matching G2 semantics.

The classifier itself need not be transmitted; only the chosen representation
and reconstruction data are decoder-visible.

---

## 8. Grouping

Structured records are grouped by the same exact G2 shape identity.

No discriminator-field inference is allowed in G3.

No schema unioning is allowed.

No optional-field normalization is allowed.

No cross-shape column synthesis is allowed.

This intentionally tests the smallest regionization change.

---

## 9. Regionized carrier

The G3 carrier must be independently decodable and bounded.

Conceptually:

    magic/version
    original_source_len
    frame_count
    reconstruction_order_stream

    structured_shape_groups:
        exact shape/template metadata
        occurrence count
        G2 leaf descriptors/payloads

    raw_residual_frames:
        exact frame bytes

Structured templates retain their original LF/CRLF bytes exactly as frozen G2
does; raw residual frames retain their complete bytes. No separate terminator
stream is part of the primary G3 hypothesis.

The exact wire is an implementation detail, but every decoder-visible byte must
be charged.

No encoder-only state may be assumed by the decoder.

---

## 10. Required candidate arms

Every discovery file must report:

### C0 — RAW_BROTLI

    Brotli(original bytes)

Mandatory fallback.

### C1 — G2_WHOLE

The frozen G2 whole-object portfolio result where the object is eligible.

If the object is globally ineligible, report unavailable.

### C2 — G3_REGION_RAW

Regionized structured carrier using only `RAW_LEX` structured columns plus raw
residuals.

This isolates regionization + structured grouping without typed leaves.

### C3 — G3_REGION_DICT

Same regionization, with the frozen G2 dictionary local selector.

### C4 — G3_REGION_INT

Same regionization, with the frozen G2 integer local selector.

### C5 — G3_REGION_MIXED

Same regionization, with the frozen G2 mixed local selector.

A bounded G2-style marginal-search arm is **not required** for the primary G3
gate. G2 already showed that repeated exact q11 whole-carrier search is expensive
and can be weaker than local selection when many substitutions are beneficial.

If retained as a diagnostic, it must not affect the frozen primary ruling.

---

## 11. Final candidate portfolio

For each file:

    selected =
      min(
        RAW_BROTLI,
        G2_WHOLE if available,
        G3_REGION_RAW,
        G3_REGION_DICT,
        G3_REGION_INT,
        G3_REGION_MIXED
      )

Raw Brotli wins exact ties.

This guarantees that adding G3 cannot create a selected-byte regression.

---

## 12. Same-backend rule

Every non-raw complete carrier used for the primary size comparison is passed
through:

    Brotli quality = 11
    lgwin = 30

The backend is intentionally held constant.

G3 is testing representation value, not a new entropy engine.

---

## 13. Complete-byte accounting

Charge:

- candidate envelope;
- carrier magic/version;
- source length;
- frame count;
- reconstruction-order metadata;
- any frame/group length metadata not derivable from the carrier grammar;
- shape dictionary;
- shape IDs;
- leaf IDs;
- leaf payload lengths;
- dictionaries;
- packed IDs;
- integer bases/widths;
- residual record/frame lengths;
- raw residual bytes;
- all backend bytes.

Do not exclude metadata because it is small.

Do not treat encoder classification as free decoder knowledge.

---

## 14. Decoder contract

The decoder must validate before unsafe allocation or expansion:

- canonical varints;
- source-length bounds;
- frame/group counts;
- shape IDs and occurrence counts;
- structured/residual reconstruction counts;
- frame/group length and ordering bounds;
- leaf IDs;
- leaf payload lengths;
- exact leaf consumption;
- dictionary widths/IDs;
- bitpack extent;
- unused high bits;
- integer overflow;
- canonical integer reconstruction;
- raw residual lengths;
- exact reconstruction-order consumption;
- final source length;
- no trailing carrier bytes.

Malformed carriers reject cleanly.

The decoder does not parse JSON and does not rediscover structure.

---

## 15. Mandatory adversarial tests

Self-tests must include at least:

1. all-valid LF NDJSON;
2. all-valid CRLF NDJSON;
3. valid records + malformed middle line + valid later records;
4. valid records + incomplete final fragment;
5. blank lines between records;
6. all-invalid/raw input;
7. one structured record only;
8. mixed shapes;
9. every G2 leaf;
10. INT64_MIN/MAX;
11. delta/DoD overflow ineligibility;
12. malformed/truncated residual payload;
13. malformed reconstruction-order stream;
14. impossible structured/residual counts;
15. bad frame/group length metadata;
16. bad dictionary ID/width;
17. bad bitpack high bits;
18. noncanonical varints;
19. trailing carrier bytes.

Every accepted source must round-trip byte-for-byte.

---

## 16. Discovery corpus

Reuse the exact frozen G2 discovery corpus identities from
`I10-GROTLI-G2-CORPUS-FREEZE.md`.

This is intentional.

G3 is a mechanistic follow-up to G2, and D3 is the causal treatment.

Discovery:

- D1 Amazon;
- D2 CDISC;
- D3 GH Archive frozen 10 MiB object;
- D4 CROVIA.

No corpus substitution is allowed after implementation begins.

---

## 17. D1/D2/D4 role

D1, D2 and D4 are known-data regression controls.

G3 must report both:

- frozen G2 selected bytes;
- new G3 regionized candidate bytes.

Their purpose is not to establish new generalization.

They answer:

- does regionization preserve the established D2/D4 expert value?
- does the new framing overhead erase prior wins?
- can the final portfolio retain G2 unchanged when regionization is unfavorable?

The portfolio may select the frozen-compatible G2 whole arm on these files.

---

## 18. D3 role

D3 is the primary causal treatment.

G2 reports:

    structured_eligible = false
    ineligible_reason = "unterminated JSON string"
    RAW_BROTLI = 1,292,757 B

G3 must report:

- total frame count;
- structured frame count;
- raw residual frame count;
- structured source bytes;
- raw residual source bytes;
- raw residual fraction;
- shape count among structured frames;
- complete bytes for every G3 regionized arm;
- exact roundtrip.

No assumption is made in advance about how many D3 frames will parse.

---

## 19. Discovery gate

### PASS-G3-REGION

All of the following must hold:

1. every emitted arm round-trips exactly;
2. all malformed/framing/residual decoder self-tests pass;
3. D3 contains at least one structured frame and at least one raw residual frame;
4. D3's selected regionized candidate is strictly smaller than D3 raw Brotli;
5. the four-file final routed portfolio is at least **3% smaller than raw
   Brotli**, preserving the G2 broad aggregate bar;
6. at least two of D1/D2/D4 remain >=5% selected wins versus raw Brotli;
7. every decoder-visible byte is charged;
8. no new leaf family or schema-unification mechanism is introduced.

Because D1/D2/D4 are already measured, condition 5 is deliberately demanding.
Condition 6 is a regression/integrity requirement, **not independent new evidence**:
`G2_WHOLE` already gives known >=5% wins on D2 and D4. The new causal size evidence
in G3 is whether regionization makes D3 useful without sacrificing the routed
portfolio.

With their frozen G2 selected bytes held constant, D3 would need approximately
**14,555 additional bytes** of saving, about **1.126%** of its raw-Brotli
complete bytes, for the aggregate to cross -3%.

That arithmetic is recorded before G3 implementation and is not a post-hoc
threshold.

### NO-GO-G3-REGION

If discovery fails:

- do not open V1;
- do not add float/FSST/RLE/schema-union mechanisms inside G3;
- classify whether failure is due to:
  - too little valid structured coverage;
  - exact-shape fragmentation;
  - residual/framing metadata;
  - leaf weakness on D3;
  - backend interaction.

---

## 20. Held-out V1 gate

Only PASS-G3-REGION authorizes the validation workflow to fetch V1.

V1 remains:

- Sino-US DrugQA;
- exact frozen identity from the G2 corpus freeze.

Validation must use the exact frozen G3 implementation SHA that passed discovery.

### PASS-G3-BASE

Define:

    V1_REGION_BEST = min(
      G3_REGION_RAW,
      G3_REGION_DICT,
      G3_REGION_INT,
      G3_REGION_MIXED
    )

Require:

1. exact roundtrip for every emitted V1 arm;
2. **V1_REGION_BEST** is at least **1% smaller than raw Brotli**;
3. no representation/planner changes from discovery;
4. complete metadata accounting.

`G2_WHOLE` remains a legitimate final portfolio fallback/context arm, but it
**cannot satisfy condition 2**. This prevents an already-established G2
representation from validating a new G3 mechanism.

Also report whether V1 contains any raw residual frames. If it contains none, a
PASS-G3-BASE validates the regionized carrier on an independent fully structured
population, but it does **not** establish held-out generalization of mixed
structured+residual coverage; that stronger claim remains unsupported until a
separate independent mixed-validity family is measured.

### PASS-G3-NARROW

If discovery passes but `V1_REGION_BEST` does not reach -1%, classify the G3
mechanism as class-specific even if `G2_WHOLE` or the overall fallback portfolio
beats raw Brotli on V1.

No tuning after opening V1.

---

## 21. Required diagnostics

Per file:

- source bytes/hash;
- raw Brotli bytes;
- G2 whole bytes if available;
- every G3 regionized arm;
- selected bytes/delta;
- exact roundtrip;
- parse/framing time;
- carrier-build time;
- local leaf-scoring time;
- backend encode time;
- decode time;
- peak RSS.

Regionization:

- frame count;
- structured frame count;
- residual frame count;
- structured source bytes;
- residual source bytes;
- LF/CRLF/final-remainder counts as **encoder diagnostics only** (not transmitted
  terminator side information);
- structured coverage fraction;
- residual fraction;
- shape count;
- singleton shape count;
- top-shape coverage.

Per structured shape/column:

- all diagnostics already required by G2.

Planner:

- isolated leaf evaluations;
- selected leaf counts by family.

No diagnostic is itself a promotion criterion unless listed in the frozen gate.

---

## 22. Runtime status

G3 remains representation-first.

Primary scientific ruling is exact complete bytes.

Do not claim a full Pareto crossing from G3.

However the workflow must record timing/RSS because G2 showed that q11 candidate
evaluation can dominate runtime.

A successful G3 representation would receive a separate production-planner /
decode-kernel lane.

---

## 23. Prior-art boundary

Regionized structured compression with raw residuals is established engineering,
not a mechanism-level novelty claim.

Relevant lineage includes:

- DataCortex grouped NDJSON/schema paths with residual rows;
- CLP / LogPrism structured log decomposition;
- general columnar formats with fallback/exception streams.

References:

- https://github.com/rushikeshmore/DataCortex
- https://docs.rs/crate/datacortex-core/latest/source/src/format/ndjson.rs
- https://www.usenix.org/conference/osdi21/presentation/rodrigues
- https://arxiv.org/abs/2601.17482

G3's purpose is experimental architecture selection for ANVIL.

---

## 24. Do-not-reburn constraints

G3 must not be represented as:

- revival of global transpose;
- supplied-stride synthetic schema;
- Gorilla single-mechanism research;
- a new entropy coder;
- a universal JSON claim;
- proof of general arbitrary-byte structure discovery.

The specific hypothesis is narrower:

> **partial structured coverage can be worth encoding even when the containing
> object is not globally parseable.**

---

## 25. Implementation order

1. commit and publish this preregistration;
2. preserve the frozen G2 results and implementation;
3. create a separate G3 standalone prototype;
4. implement exact line/framing roundtrip first;
5. implement raw residual classification;
6. reuse frozen G2 shape/leaf semantics for valid records;
7. pass mandatory self-tests;
8. freeze implementation SHA publicly;
9. create GitHub Actions discovery workflow pinned to that SHA;
10. run D1-D4 remotely;
11. close discovery;
12. only on PASS-G3-REGION, open V1 once.

No production wire ID is authorized by this document.
