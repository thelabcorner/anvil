# ANVIL I10 — GROTLI G1-CEILING Results

**Closed:** 2026-09-24
**Frozen implementation:** public SHA b82d9c83c9c8528861eb65595fface6605cf0e7a
**Remote run:** 35937406182
**Production source changed:** no
**Broad G1 base ruling:** **NO-GO-G1-DISCOVERY**
**Class-specific representation signal:** **strong positive; SHAPE_COLUMN beats the same Brotli backend materially on D4**
**Held-out V1 opened:** **no**

---

## 0. Executive ruling

G1 tested the causal representation question left open by G0:

> Can exact JSON/NDJSON structure separation and raw lexical value
> columnization make the **same Brotli q11/lgwin30 backend** smaller than raw
> Brotli, after charging the complete reconstruction carrier?

The answer is:

> **Yes on some real structured classes, but not broadly enough to pass the
> frozen G1 base gate.**

The discovery portfolio improves over raw Brotli in aggregate by **24,296 B /
1.6564%**, but G1 required:

- at least two of four discovery families with >=5% structured wins; and
- at least 3% aggregate selected improvement.

Only one family crosses 5%, and aggregate improvement is below 3%.

Therefore the correct broad ruling is:

> **NO-GO-G1-DISCOVERY for the frozen untyped exact-shape base.**

The held-out Sino-US DrugQA V1 corpus was not fetched or measured. It remains
available for a future separately preregistered experiment.

This is **not** a rejection of the Grotli/representation-compiler direction.
One discovery family produced a very large same-backend win, and the row/column
ablation identifies the mechanism cleanly.

---

## 1. What was tested

Exactly three complete candidates existed.

### RAW

    Brotli_q11_lgwin30(original bytes)

### SHAPE_ROW

- byte-exact JSON lexical parse;
- object keys and syntax remain structural template bytes;
- every scalar value remains its exact original lexical token;
- repeated exact shapes stored once;
- record-order shape-ID stream;
- raw scalar tokens grouped by shape but kept occurrence/row-major;
- complete carrier passed to the same Brotli q11/lgwin30 backend.

### SHAPE_COLUMN

Identical to SHAPE_ROW except raw scalar tokens for a shape are serialized
placeholder/column-major.

No typed leaf was allowed:

- no token dictionary;
- no integer transform;
- no FOR/delta/DoD;
- no null/presence coding;
- no Gorilla/ALP;
- no FSST;
- no replay.

This isolates:

1. repeated lexical-template elimination; and
2. homogeneous field-position locality.

---

## 2. Remote correctness/provenance

Run 35937406182 completed the following before any ruling:

- clang-18 build: **PASS**;
- parser/roundtrip/malformed carrier self-test: **PASS**;
- D1-D4 immutable size/Git-blob verification: **PASS**;
- all four measurement processes: **PASS**.

Pinned discovery SHA-256 values:

| Family | Bytes | SHA-256 |
|---|---:|---|
| D1 Amazon cellphone | 277,673 | c1518fdaaed45e590c480ed707aa1adaaba8b84b10747f956bd431c708bd590e |
| D2 CDISC ADaM | 615,350 | b795a59c5c92a8fc93d7d3f729fa0dae014e9b0f684a6a7a7bcf07e4104ba723 |
| D3 GH Archive 10 MiB | 10,485,760 | a860af236f794779b5471b416b2e6d7ea68de00f6d17728dd3b088a7ecec8252 |
| D4 CROVIA receipts | 3,585,053 | 0db7ace9e46ce458a7055f48b09aabc7df15f4f74a23553d3660b7de61d0916f |

Every emitted structured candidate round-tripped exactly.

---

## 3. Exact discovery results

| Family | Raw Brotli complete | SHAPE_ROW | Row delta | SHAPE_COLUMN | Column delta | Selected |
|---|---:|---:|---:|---:|---:|---|
| D1 Amazon | 40,126 B | 43,103 B | +7.4191% | **39,530 B** | **-1.4853%** | column |
| D2 CDISC | **25,029 B** | 25,768 B | +2.9526% | 25,654 B | +2.4971% | raw |
| D3 GH Archive | **1,292,757 B** | ineligible | — | ineligible | — | raw |
| D4 CROVIA receipts | 108,857 B | 108,334 B | -0.4804% | **85,157 B** | **-21.7717%** | column |

Aggregate:

- raw complete bytes: **1,466,769 B**;
- selected complete bytes: **1,442,473 B**;
- selected saving: **24,296 B**;
- aggregate selected delta: **-1.6564%**.

Frozen discovery threshold:

- >=5% wins required: **2/4**;
- observed: **1/4**;
- aggregate <= -3% required;
- observed: **-1.6564%**.

Therefore discovery does not pass.

---

## 4. D3 eligibility and workflow-ruling defect

The D3 GH Archive 10 MiB frozen object produced:

    structured_eligible = false
    ineligible_reason = "unterminated JSON string"

Under the frozen G1 representation contract, an invalid/incomplete JSON record
makes the structured candidate unavailable and raw Brotli remains the fallback.

The first workflow-ruling script was stricter than the preregistration: it
treated any ineligible frozen discovery file as an infrastructure failure.
That caused the GitHub run's Discovery ruling step to be marked failed rather
than emitting the scientific NO-GO ruling.

This does **not** affect the compression conclusion.

Applying the frozen semantics:

- D3 contributes raw Brotli;
- D3 cannot count as a >=5% structured win;
- only D4 crosses the 5% requirement;
- aggregate selected improvement remains only -1.6564%.

Therefore G1 fails the preregistered discovery gate independently of the
workflow-ruling defect.

No rerun is required to establish the G1 size ruling.

The validation job was skipped and no validation artifact exists.

---

## 5. The most important result: D4 is a clean representation win over Brotli

D4 is the strongest evidence produced by the Grotli lane so far.

Source:

- 3,585,053 B;
- 3,718 records;
- 42 exact shapes;
- 82,768 scalar tokens;
- 1,516,276 structural bytes;
- 2,068,777 raw scalar-token bytes.

Shape anatomy:

- top shape: 1,941 rows;
- top 5 shapes: 3,367 / 3,718 rows;
- top 10 shapes: 3,548 / 3,718 rows;
- singleton shapes: only 9;
- unique template bytes: 21,463 B;
- gross repeated-template bytes avoided: 1,494,813 B.

Complete uncompressed G1 carrier:

- 2,178,134 B;
- **39.2440% smaller than source before Brotli**.

Carrier components:

- header: 13 B;
- shape dictionary: 22,839 B;
- template bytes inside dictionary: 21,463 B;
- shape IDs: 3,718 B;
- value lengths: 82,787 B;
- exact raw scalar values: 2,068,777 B.

But the key ablation is downstream.

Same Brotli q11/lgwin30:

- raw source: **108,857 B complete**;
- SHAPE_ROW: **108,334 B** / -0.4804%;
- SHAPE_COLUMN: **85,157 B** / **-21.7717%**.

The row carrier already removed almost 1.5 MB of repeated lexical scaffolding,
yet after Brotli it saves only 523 B.

Changing **only the ordering of the same exact scalar tokens** from row-major to
column/placeholder-major saves another 23,177 B versus SHAPE_ROW and 23,700 B
versus raw Brotli.

This is very strong causal evidence for the Grotli thesis:

> **The valuable operation is not merely deleting repeated syntax. It is
> reorganizing semantically corresponding values into a representation where
> the mature backend sees much simpler local distributions and match/context
> structure.**

No new entropy coder is involved.

---

## 6. D1 supports the same mechanism at smaller magnitude

D1 has only:

- 8,723 structural bytes / 3.14% of source;
- one exact shape across all 793 records;
- nine scalar positions per record.

Uncompressed carrier:

- 277,047 B;
- only 626 B / 0.2254% smaller than source.

Downstream:

- SHAPE_ROW: **+7.4191%** worse than raw Brotli;
- SHAPE_COLUMN: **-1.4853%** better.

Again, template deduplication alone is not enough.

Column locality reverses a substantial row-layout loss into a small win.

This is the same directional mechanism as D4.

---

## 7. D2 is the critical negative control

D2 is especially informative because the pre-Brotli carrier looks promising.

Source:

- 615,350 B;
- 1,192 records;
- only two shapes;
- 1,191 rows in the dominant shape;
- 55.19 scalar placeholders/record;
- gross repeated-template bytes avoided: 132,090 B.

Uncompressed carrier:

- 550,600 B;
- **10.5225% smaller than source**.

Yet:

- SHAPE_ROW: **+2.9526%** worse than raw Brotli;
- SHAPE_COLUMN: **+2.4971%** worse.

This proves again:

> **pre-backend byte shrink is not the objective.**

Raw Brotli already represents this repeated structure/value population so
efficiently that the explicit shape/length decomposition loses after complete
backend coding.

This directly validates ANVIL's rule that representation candidates must be
judged by final downstream bytes, not zero density, H0, carrier size, or
semantic elegance.

---

## 8. Planner/runtime observations

The standalone implementation was deliberately an oracle ceiling, not a
production planner.

Per-process wall / peak RSS for all three arms together:

| Family | Wall | Peak RSS |
|---|---:|---:|
| D1 | 1.01 s | 25.8 MiB |
| D2 | 2.07 s | 32.9 MiB |
| D3 raw-only | 13.44 s | 150.7 MiB |
| D4 | 7.86 s | 61.9 MiB |

On eligible files, parser/anatomy itself is tiny relative to Brotli q11 encode
time:

- D1 analyze: ~0.77 ms;
- D2: ~3.76 ms;
- D4: ~8.37 ms.

The obvious production risk is therefore not lexical parsing in this prototype.
It is evaluating expensive leaf/backend candidates.

That reinforces the historical Grotli-v4 lesson:

> **candidate generation should be cheap; expensive backend evaluation must be
> heavily pruned.**

---

## 9. Broad ruling versus expert value

Two statements are simultaneously true.

### Broad base

The frozen untyped exact-shape base does **not** generalize strongly enough to
earn a universal JSON/NDJSON base route under the preregistered threshold.

Ruling:

> **NO-GO-G1-DISCOVERY**

### Expert/portfolio value

The same representation architecture produces a **21.77% same-backend win**
on an independent real structured family and a smaller win on another.

Because raw Brotli remains a fallback, the discovery portfolio itself is
**1.6564% smaller in aggregate** than raw Brotli across all four frozen
families.

Therefore:

> **Column-locality is a viable ANVIL representation expert, but the frozen
> shape/raw expert is not a universal structured-data base.**

This is exactly the kind of specialist portfolio behavior ANVIL was designed to
exploit.

---

## 10. What G1 authorizes next

G1 does **not** authorize production src/anvil.cpp integration.

It does authorize a new, separately preregistered structured-expert experiment
because:

1. D4 is a large real same-backend win;
2. D1 has a same-direction column win;
3. D2 shows a different population where raw lexical columns are insufficient;
4. historical Grotli independently found large gains from typed column leaves;
5. modern ALP/FastLanes/BtrBlocks/FSST/Pcodec lineage supplies mature reference
   designs.

The next experiment should not "fix G1."

It should ask a new question:

> **Can a small typed leaf basis make the column-locality expert win across more
> structured families, while exact final Brotli bytes still choose the winner
> and raw Brotli remains the permanent fallback?**

Candidate basis should remain deliberately small:

- RAW lexical column;
- exact-token dictionary/enum;
- canonical integer FOR/delta/DoD;
- default/null bitmap only where it is a genuine stream property.

Float/string-symbol leaves should remain deferred until the simpler basis is
measured.

This is a new G2 experiment and requires a fresh preregistration.

---

## 11. Held-out status

The frozen G1 V1 corpus:

- Sino-US DrugQA;
- 15,374,047 B;
- blob 8d9e6acf3f534194487532fa60d4f1d43fe71d00;

was **not fetched or measured by run 35937406182**.

The only uploaded run artifact is:

    grotli-g1-discovery-35937406182

The validation job is recorded as skipped.

Therefore V1 remains available as genuinely unspent validation data for a
future preregistered G2 experiment.

---

## 12. Final interpretation

G0 taught:

> blind positional residualization can destroy the structure Brotli already
> understands.

G1 teaches:

> **semantic-position reordering can make Brotli materially better than itself,
> but only when the structure/value population supports it.**

The path forward is no longer speculative.

ANVIL has now directly demonstrated a real class-specific instance of:

    representation intelligence
      + same mature entropy backend
      < raw mature entropy backend

The research question has moved from:

> "Can this happen?"

to:

> **"Can ANVIL discover the right specialist representation cheaply enough and
> often enough that the routed portfolio moves the general-purpose Pareto
> frontier?"**
