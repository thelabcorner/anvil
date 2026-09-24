# ANVIL I10 — GROTLI G2 Typed Column Expert Results

**Closed:** 2026-09-24
**Frozen implementation:** public SHA `c34b291f40db4037ae114a39dae181644260dfff`
**Workflow orchestration:** public SHA `5c21d492ce6d55c26a7d0ea836d3500c74c11cc1`
**Remote run:** `35943876855`
**Frozen binary SHA-256:** `ea0531f1b4cc868cf231348323eacaf2be46968ccd73d66e1e1fb90b4a5080f4`
**Production source changed:** no
**Discovery ruling:** **NO-GO-G2-DISCOVERY**
**Held-out V1 opened:** **no**

---

## 0. Executive ruling

G2 asked whether a deliberately small typed-column basis could broaden the
class-specific representation win established by G1 while keeping:

- exact source-byte reconstruction;
- raw Brotli as permanent fallback;
- the same Brotli q11/lgwin30 backend for the causal comparison;
- complete carrier accounting;
- a frozen discovery/validation firewall.

The result is mixed but highly informative.

G2 **does not pass its preregistered broad discovery gate** because the
four-file routed aggregate improves by only **2.0077%**, below the frozen
**3%** requirement.

However, the typed representation hypothesis itself produced two strong real
same-backend wins:

- D2 CDISC: **-16.4849%** versus raw Brotli;
- D4 CROVIA: **-22.5029%** versus raw Brotli.

The required count of structured families at or below -5% is therefore
**2/3**, exactly meeting that component of the gate.

The typed-arm attribution requirement also passes: at least one typed arm
improves by >=1% relative to G1R. D2 P-DICT improves by **19.3433%** relative
to G1R; D4 P-MIXED improves by **3.1758%**.

The binding failure is the aggregate routed portfolio.

Therefore the correct ruling is:

> **NO-GO-G2-DISCOVERY.**

Do not open V1 under G2.

Do not add float, FSST, RLE/default, replay, cross-column prediction, or a new
planner inside G2 after observing this result.

---

## 1. What G2 tested

The frozen carrier retained the exact G1 lexical/shape contract and added an
explicit leaf per shape/placeholder column.

Allowed leaves were exactly:

1. `RAW_LEX`
2. `EXACT_DICT`
3. `INT_FOR`
4. `INT_DELTA_FOR`
5. `INT_DOD_FOR`

Measured whole-file arms were:

- `RAW_BROTLI`
- `G1R` — every G2 leaf forced to RAW_LEX
- `P-DICT`
- `P-INT`
- `P-MIXED`
- `P-MARGINAL`

Every structured arm was serialized completely and then compressed by the same
Brotli q11/lgwin30 backend.

Raw Brotli won exact ties and remained available for every file.

---

## 2. Remote correctness and provenance

Run `35943876855` completed successfully.

Before the discovery ruling it passed:

- checkout of the frozen implementation SHA;
- implementation identity verification;
- clang-18 build;
- frozen G2 self-test;
- frozen corpus acquisition and identity verification;
- exact roundtrip for every emitted structured arm;
- D3 raw-fallback routing verification.

The validation job was **skipped** because discovery did not pass.

No held-out V1 artifact exists.

This preserves the Sino-US DrugQA object for a future materially different,
separately preregistered hypothesis.

---

## 3. Exact discovery result

| Family | Raw Brotli complete | G1R | P-DICT | P-INT | P-MIXED | P-MARGINAL | Selected | Selected delta |
|---|---:|---:|---:|---:|---:|---:|---|---:|
| D1 Amazon | 40,126 B | 39,470 | **39,299** | 39,470 | **39,299** | **39,299** | P-DICT | **-2.0610%** |
| D2 CDISC | 25,029 B | 25,916 | **20,903** | 25,986 | 20,910 | 24,865 | P-DICT | **-16.4849%** |
| D3 GH Archive 10 MiB | **1,292,757 B** | — | — | — | — | — | RAW_BROTLI | **0.0000%** |
| D4 CROVIA | 108,857 B | 87,128 | 84,375 | 86,889 | **84,361** | 86,505 | P-MIXED | **-22.5029%** |

D3 remains structurally unavailable under the frozen whole-file exact parser:

> `unterminated JSON string`

That is the expected routing-control behavior for G2.

Aggregate:

- raw complete bytes: **1,466,769 B**
- selected complete bytes: **1,437,320 B**
- bytes saved: **29,449 B**
- routed aggregate delta: **-2.007746%**

Frozen requirement:

- selected structured wins <= -5%: **2/3** — PASS
- aggregate routed delta <= -3%: **FAIL**
- typed arm >=1% better than G1R on >=1 family: **PASS**

Final:

> **NO-GO-G2-DISCOVERY**

---

## 4. Important diagnostic: the structured expert itself is strong

The three structured-eligible families alone are not the frozen aggregate gate,
but they are useful mechanism diagnostics.

Across D1 + D2 + D4:

- raw complete bytes: **174,012 B**
- selected bytes: **144,563 B**
- diagnostic delta: **-16.9235%**

This number must **not** replace the preregistered four-file ruling.

It does show that G2's failure is not explained by a generally weak structured
representation.

The broad gate fails because the portfolio must also carry the large D3
raw-fallback population.

D3 contributes about **88.14%** of the four-file raw-Brotli byte total.

The aggregate gate misses its -3% threshold by approximately **14,555 B**.

That is failure anatomy, not permission to change the gate after the fact.

---

## 5. D2 is the strongest new causal result

G1 made D2 the critical negative control:

- raw Brotli: **25,029 B**
- G1 SHAPE_COLUMN: **25,654 B**
- G1 structured route: **+2.4971% worse** than raw

G2 changes the result decisively:

- G1R under the G2 carrier: **25,916 B**
- P-DICT: **20,903 B**
- P-INT: **25,986 B**
- P-MIXED: **20,910 B**
- selected: **P-DICT / 20,903 B**

Thus:

- selected versus raw Brotli: **-16.4849%**
- P-DICT versus G1R: **-19.3433%**
- integer-only versus G1R: **+0.2701%** approximately worse

Leaf selection explains the mechanism:

P-DICT:

- 296 RAW_LEX columns
- **43 EXACT_DICT columns**

P-INT:

- 326 RAW_LEX
- 11 INT_FOR
- 2 INT_DELTA_FOR
- 0 INT_DOD_FOR

P-MIXED:

- 285 RAW_LEX
- 42 EXACT_DICT
- 11 INT_FOR
- 1 INT_DELTA_FOR
- 0 INT_DOD_FOR

The causal conclusion is unusually clean:

> **D2 was not missing a more complicated entropy backend. It was missing an
> exact low-cardinality token representation before Brotli.**

Integer coordinate transforms do not explain the D2 crossing.

Exact dictionaries do.

---

## 6. D4 retains the G1 representation win and improves slightly

D4 remains the strongest shape/column locality population.

Measured G2:

- raw Brotli: **108,857 B**
- G1R: **87,128 B**
- P-DICT: **84,375 B**
- P-INT: **86,889 B**
- P-MIXED: **84,361 B**
- P-MARGINAL: **86,505 B**

Selected:

> **P-MIXED / 84,361 B / -22.5029% versus raw Brotli**

Historical frozen G1 SHAPE_COLUMN was **85,157 B**.

G2 therefore saves another **796 B** relative to that historical G1 result,
about **0.935%**.

Operator attribution again points mostly to dictionary coding.

P-DICT selects:

- 724 EXACT_DICT leaves
- 522 RAW_LEX leaves

P-INT selects:

- 99 INT_FOR leaves
- 1,147 RAW_LEX leaves
- no DELTA/DoD leaves

P-MIXED selects:

- 631 EXACT_DICT
- 99 INT_FOR
- 516 RAW_LEX
- no DELTA/DoD leaves

P-MIXED is only **14 B** smaller than P-DICT after final Brotli.

Therefore:

> **Dictionary representation is again the dominant typed contribution. Integer
> FOR is useful but secondary on this population.**

---

## 7. D1 remains a small specialist win

D1:

- raw Brotli: **40,126 B**
- G1R: **39,470 B**
- P-DICT: **39,299 B**
- selected delta: **-2.0610%**

Only two dictionary substitutions are selected.

The typed improvement over G1R is **0.4332%**, below the G2 typed-attribution
threshold by itself.

D1 therefore supports the same direction but not the broad crossing.

---

## 8. Planner result: dense local composition beats the bounded marginal search

G2 deliberately separated operator value from planner value.

The result exposes an important limitation in the frozen P-MARGINAL design.

### D2

P-DICT applies **43** dictionary leaves and reaches **20,903 B**.

P-MARGINAL accepts the maximum **8** substitutions and stops at **24,865 B**.

Its exact whole-carrier trace is:

`25916 -> 25664 -> 25461 -> 25371 -> 25225 -> 25134 -> 25036 -> 24979 -> 24865`

The accepted substitutions are all EXACT_DICT.

### D4

P-DICT applies **724** dictionary leaves.

P-MIXED uses 631 dictionaries + 99 integer-FOR leaves and reaches **84,361 B**.

P-MARGINAL accepts only **4** dictionary substitutions and stops at **86,505 B**.

Trace:

`87128 -> 86856 -> 86836 -> 86668 -> 86505`

This does **not** show that whole-carrier scoring is unimportant.

It shows that the frozen marginal search is structurally biased toward sparse
portfolios:

- start from all RAW;
- consider top 24 single substitutions;
- accept one per round;
- stop after at most 8 accepted substitutions.

The measured winners can instead require **many individually modest local
representation changes**.

Therefore:

> **Future planners should support dense/batched representation composition.
> One-at-a-time whole-carrier greedy search is not a useful ceiling when the
> winning explanation is distributed across tens or hundreds of columns.**

No G2 result is changed by this observation; selected bytes already include
P-DICT/P-MIXED.

---

## 9. Encode-cost lesson

G2 is a representation-first experiment, not a production encode benchmark.

Still, its diagnostics show where the oracle cost lives.

Approximate CI timings:

### D1

- anatomy: **0.74 ms**
- local candidate construction/scoring: **510 ms**
- marginal search: **6.80 s**

### D2

- anatomy: **3.97 ms**
- local candidate construction/scoring: **784 ms**
- marginal search: **75.10 s**

### D4

- anatomy: **8.03 ms**
- local candidate construction/scoring: **2.68 s**
- marginal search: **121.28 s**

The structural parser is cheap.

Repeated Brotli q11 candidate evaluation is expensive.

This reinforces the representation-synthesis architecture:

1. structure discovery should remain cheap;
2. candidate families need inexpensive pruning/sampling;
3. expensive backend scoring should be reserved for a small number of complete
   finalists;
4. the production planner should not reproduce the G2 oracle search literally.

---

## 10. What G2 proves

G2 supports all of the following statements.

### Proven on the measured populations

1. Exact lexical shape/column representation can make the **same Brotli backend**
   materially smaller than raw Brotli.
2. Exact-token dictionary representation can turn a G1 structured loss into a
   large same-backend win.
3. The useful typed operator basis can be extremely simple.
4. Whole-file raw fallback prevents selected-byte regressions.
5. Representation value is heterogeneous: D1 is small, D2/D4 are large, D3 is
   unavailable under the frozen whole-file parser.
6. A dense set of local typed choices can outperform a sparse greedy
   whole-carrier search.
7. The expensive part of the research oracle is repeated backend evaluation,
   not structural parsing.

### Not proven

G2 does **not** prove:

- a universal JSON/NDJSON route;
- a general-purpose Pareto crossing;
- production encode economics;
- broad held-out generalization;
- a novel dictionary/FOR mechanism;
- that adding more leaf families would necessarily close the aggregate gap.

---

## 11. Why the broad gate failed

The failure is best classified as:

> **insufficient routed prevalence under whole-file structured eligibility**

rather than:

- typed operator basis failure;
- complete absence of representation value;
- integer coding failure alone;
- metadata accounting failure.

D3 is a large raw-only routing control because one incomplete/invalid region
makes the frozen whole-file expert unavailable.

That behavior was correct under G2.

It is also now the most obvious architectural limitation exposed by G2.

ANVIL's target architecture is an adaptive representation compiler over
arbitrary bytes, not a file-level boolean JSON transform.

A single malformed/incomplete record should not necessarily disqualify every
other independently reconstructable structured region in a future architecture.

That is a **new hypothesis**, not a G2 patch.

---

## 12. Highest-EV next hypothesis: regional structured routing

The next experiment should be separately preregistered as a new G3-style lane.

Recommended causal question:

> **Can ANVIL preserve the measured G2 typed-column wins while routing only
> structurally valid record regions through the structured carrier and preserving
> invalid/incomplete/opaque spans literally, with raw Brotli still available for
> the entire file?**

The first regional experiment should **not** add new leaf families.

Reuse the measured G2 basis:

- RAW_LEX
- EXACT_DICT
- INT_FOR
- INT_DELTA_FOR
- INT_DOD_FOR

This isolates region routing from operator expansion.

A conceptual reversible mixed carrier is:

```
file
  -> ordered regions
       -> STRUCTURED_RUN(G2-compatible carrier)
       -> RAW_SPAN(exact bytes)
       -> STRUCTURED_RUN(...)
       -> ...
  -> Brotli(same complete mixed carrier)
```

with whole-file `Brotli(raw)` still a candidate.

For NDJSON-like data, a natural first boundary source is complete LF-delimited
records.

An incomplete final record can remain a RAW_SPAN without invalidating the valid
prefix.

The decoder receives every region decision explicitly; it performs no structure
discovery.

---

## 13. Why regional routing has higher expected value than immediately adding more leaves

This is not a claim that G3 will pass.

It is a prioritization based on the measured failure.

G2 already obtains:

- **-16.48%** on D2;
- **-22.50%** on D4.

The four-file aggregate misses -3% by only about **14.6 KiB**.

Because D3 contributes ~88% of the aggregate raw-compressed bytes, a future
regional expert would need only about **1.13%** improvement on that D3 byte
population, holding the other measured results constant, to bridge the current
aggregate shortfall.

That arithmetic is only an explanation of leverage.

It is **not** a new gate and it must not be used to tune a G3 implementation
after outcomes are observed.

The correct next action is to freeze G3 semantics and gates **before** measuring
whether D3's structurally valid regions benefit at all.

---

## 14. G3 discipline

Before implementation:

1. freeze a regional-carrier preregistration;
2. freeze exact region-boundary and malformed-tail semantics;
3. retain D1/D2/D4 as regression/attribution controls;
4. retain D3 as the already-known whole-file-ineligible discovery population;
5. do not inspect D3 structured-prefix compression outcomes before the
   preregistration is frozen;
6. keep V1 unopened;
7. define whether V1 is sufficient held-out evidence for the new hypothesis or
   whether a second region-routing-specific held-out family is required;
8. keep all heavy compression evaluation on GitHub Actions.

Do not integrate a production transform ID before that experiment closes.

---

## 15. Final interpretation

G0 showed that a superficially structured transform can destroy Brotli's useful
locality.

G1 showed that semantic-position columnization can make Brotli beat itself
materially on the right population.

G2 now shows that:

> **a tiny typed basis—especially exact-token dictionaries—can broaden that
> representation win dramatically, but whole-file eligibility is too brittle to
> satisfy the frozen routed aggregate gate.**

That is a useful NO-GO.

The next problem is no longer:

> "Do typed representations help?"

They clearly do on D2 and D4.

The next problem is:

> **"Can the representation compiler apply those experts to only the regions
> they explain, while preserving raw bytes everywhere else?"**

The follow-up is now frozen before implementation in
[I10-GROTLI-G3-REGION-PREREG.md](I10-GROTLI-G3-REGION-PREREG.md).
