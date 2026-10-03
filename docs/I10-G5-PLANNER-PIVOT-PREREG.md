# ANVIL I10 — G5 Planner Pivot Pilot Preregistration

**Status:** FROZEN PILOT CONTRACT BEFORE ANY G5-PLANNER-PIVOT SOURCE OR D1-D4 MEASUREMENT  
**Freeze revision:** r1  
**Date:** 2026-09-24  
**Parent evidence:** G3 `PASS-G3-NARROW`; G4 `NO-GO-G4`; frozen G4 run `35952830106`  
**Frozen G4 implementation:** public SHA `6c556dc522112aff4b3566990965e000418c5a57`  
**Frozen G3 implementation:** public SHA `1a3d18fed76adb6fb33264e1994f9c357306b3fa`  
**Production source `src/anvil.cpp`:** must not be modified  
**Production transform ID:** none  
**Production planner authorization:** none  
**Held-out corpus:** none opened by this pilot  
**Workflow status:** not created; blocked on a conforming pilot source and immutable implementation identity

---

## 0. Freeze declaration and relationship to G4

G4 remains closed and valid:

- frozen G3 carrier identity passed 16/16 exact comparisons;
- S0, S1, and S2 failed the frozen fidelity and speed gates;
- Q2 was `SEPARABLE-ENOUGH` on its bounded 24-pair probe;
- the measured disposition was `NO-GO-G4`.

This document does not reopen, amend, rescue, or reinterpret G4. In particular, it does
not change any S0/S1/S2 arm, threshold, score, tie rule, corpus, or result.

This is a separately identified pilot:

> **G5-PLANNER-PIVOT**

It tests one materially different hypothesis:

> Can an encoder-only analytical MDL selector, corrected by at most 80 sampled q4
> evaluations and arbitrated through a bounded whole-carrier finalist ladder, approach
> frozen O11 complete-carrier quality without using q11 to rank individual candidates?

The pilot is a feasibility experiment, not a production planner. Its existence does
not override G4's instruction to return to representation/anatomy work. It may run
only under a new source freeze, a new manual CI workflow, and the gates in this
document.

The G5A ordering attribution and G5B ordinal experiments remain separate. Their
representation conclusions do not validate this planner.

---

## 1. Exact scope

### 1.1 Authorized

The pilot may:

- reuse the frozen G3 parsing, shape, candidate, carrier, and decoder semantics;
- materialize the frozen five G3 leaf candidates;
- compute analytical features from exact candidate objects;
- fit a fixed eight-feature ridge calibration against frozen O11 labels on D1-D4;
- evaluate a bounded sample of candidates with Brotli q4/lgwin30;
- construct P0 and P1 selections under the four frozen family masks;
- build and compare a bounded set of complete carriers at Brotli qualities 1, 4, 6,
  and 11 with lgwin 30;
- compare against frozen O11, raw Brotli, frozen G2 where eligible, and a bounded
  q11 K-main oracle;
- report bytes, planner stages, total encode time, decode time, call counts, and peak
  RSS.

### 1.2 Excluded

The pilot may not:

- modify `src/anvil.cpp`;
- allocate a production wire or transform ID;
- add, remove, or alter a leaf;
- add a candidate family;
- alter parser, framing, shape identity, template bytes, metadata, or raw residual
  bytes;
- use q11 to rank individual candidates;
- use a learned model whose features, solver, weights, or constants differ from this
  contract;
- open G3 V1 as validation;
- open a new held-out corpus;
- tune a threshold, arm, feature, coefficient procedure, or tie rule after D1-D4;
- promote a discovery result to production;
- report raw/G2 fallback savings as planner fidelity;
- call O11 an optimum or lower bound.

---

## 2. Frozen representation and backend basis

### 2.1 Frozen parser and carrier

The pilot must compile against a materialized source from frozen G3 SHA:

```text
1a3d18fed76adb6fb33264e1994f9c357306b3fa
```

CI must materialize the exact `tools/grotli_g3.cpp` blob from that commit, verify its
git blob and SHA-256, and compile the pilot against those materialized bytes. A
working-tree G3 source may not silently substitute for the frozen source.

The following remain byte-for-byte inherited from G3:

- LF/CRLF/final-remainder frame splitting;
- exact lexical JSON parsing;
- malformed/incomplete frame to raw residual;
- exact shape identity and first-occurrence shape IDs;
- shape templates and all scalar spans;
- frame reconstruction order;
- raw residual bytes and order;
- candidate eligibility and payload constructors;
- carrier grammar, bounds checks, decoder strictness, and roundtrip behavior.

### 2.2 Frozen leaves

Exactly five leaf IDs remain:

```text
RAW_LEX       = 0
EXACT_DICT    = 1
INT_FOR       = 2
INT_DELTA_FOR = 3
INT_DOD_FOR   = 4
```

No sixth leaf is permitted.

### 2.3 Frozen family masks

The pilot preserves the four separate G3 family masks:

```text
REGION_RAW    = { RAW_LEX }
REGION_DICT   = { RAW_LEX, EXACT_DICT }
REGION_INT    = { RAW_LEX, INT_FOR, INT_DELTA_FOR, INT_DOD_FOR }
REGION_MIXED  = all five leaves
```

The masks may not be collapsed, widened, narrowed, reordered, or chosen per file after
observing bytes.

The family order used for deterministic reporting and ties is:

```text
RAW, DICT, INT, MIXED
```

### 2.4 Fixed Brotli settings

Every backend call records its actual quality and window:

| Use | Quality | Window |
|---|---:|---:|
| isolated sampled calibration | 4 | 30 |
| first whole-carrier ladder round | 1 | 30 |
| second whole-carrier ladder round | 4 | 30 |
| third whole-carrier ladder round | 6 | 30 |
| bounded finalist verification | 11 | 30 |
| O11 reference | 11 | 30 |
| raw fallback | 11 | 30 |
| G2 fallback where eligible | 11 | 30 |

The linked Brotli library version, dotted version, package identity, compiler, runner,
and OS must be recorded. Backend settings may not vary by runner or corpus.

---

## 3. Frozen discovery population

The pilot reuses the already exposed D1-D4 objects. They are development/discovery
objects, not held-out evidence.

| ID | File | Bytes | SHA-256 |
|---|---|---:|---|
| D1 | `D1-amazon-cellphones.ndjson` | 277673 | `c1518fdaaed45e590c480ed707aa1adaaba8b84b10747f956bd431c708bd590e` |
| D2 | `D2-cdisc-adae.ndjson` | 615350 | `b795a59c5c92a8fc93d7d3f729fa0dae014e9b0f684a6a7a7bcf07e4104ba723` |
| D3 | `D3-gharchive-10mb.ndjson` | 10485760 | `a860af236f794779b5471b416b2e6d7ea68de00f6d17728dd3b088a7ecec8252` |
| D4 | `D4-crovia-dpi-receipts.ndjson` | 3585053 | `0db7ace9e46ce458a7055f48b09aabc7df15f4f74a23553d3660b7de61d0916f` |

Rules:

1. D1-D4 are already known and may fit development coefficients.
2. The Sino-US G3 V1 object is `KNOWN-STRESS` from G3 and is not fetched by this pilot.
3. No other corpus may be measured.
4. D1-D4 cannot support a generalization claim.
5. A future held-out workflow must use a new frozen V2 corpus and the exact model and
   implementation identities from a `PASS-PILOT-DISCOVERY` run.
6. If V2 is used for any tuning, it becomes development data and a new V3 is required.

---

## 4. Candidate object and analytical feature basis

### 4.1 Exact candidate object

For every eligible candidate `c` in slot `i`, the object is the exact frozen G3
isolated leaf object:

```text
o(c) = leaf_id || occurrence_count || payload_length || payload
```

All length fields use the frozen G3 serialization. `o(c)` is the same byte sequence
used by G4 O11 isolated scoring and S0.

The object includes every field used by the isolated backend. No header, occurrence
count, payload length, or dictionary field may be omitted from MDL features.

### 4.2 Basic quantities

For candidate `c`, define:

```text
N  = len(o(c))
m  = occurrence count
P  = len(payload)
R  = sum of exact lexical token byte lengths
D  = number of distinct exact lexical tokens
```

`N >= 1`. If `R == 0`, ratios that require `R` use the explicit value zero; an
implementation may not invent a denominator.

### 4.3 Order-0 feature

Compute the 256-bin byte histogram of `o(c)` and:

```text
H0_bits = -sum over b with count[b] > 0 of
          count[b] * log2(count[b] / N)
```

No smoothing is added. `H0_bits` is zero when `N <= 1` or all bytes are identical.

### 4.4 Order-1 feature

Use byte value 256 as the initial context and the preceding object byte thereafter.
For each observed context `x`, let `n_x` be its byte count. Then:

```text
H1_bits = -sum over observed (x,b) of
          count[x][b] * log2(count[x][b] / n_x)
```

No escape penalty or smoothing is added. `H1_bits` is zero when every observed context
contains one byte value.

### 4.5 Bounded match-coverage feature

The match feature is deterministic and bounded:

1. Hash every four-byte sequence in `o(c)`.
2. Scan object positions in ascending order.
3. At each position inspect at most the eight most recent prior positions with the
   same four-byte hash, newest first.
4. Choose the longest match, maximum length 258 bytes.
5. Accept it and advance by its length; otherwise advance one byte.
6. Overlapping matches are forbidden.
7. `match_coverage = matched_bytes / N`.

This is a feature, not a compressor and not a transmitted model. It is `O(8N)` with a
single 32-bit hash table and no unbounded chain growth.

### 4.6 Coded-payload feature

`coded_payload_bits` is the exact number of payload bits charged by the frozen G3
candidate encoding rules, excluding the isolated-object header but including all
integer bases, widths, dictionary identifiers, residual bytes, and packed bits.

```text
coded_bits_per_occurrence = coded_payload_bits / m
```

`m >= 1` for every eligible candidate.

### 4.7 Frozen eight-feature order

The ridge feature vector is exactly, in this order:

```text
x0 = log2(N)
x1 = log2(m)
x2 = D / m
x3 = log2(P) - log2(R), or 0 if R == 0
x4 = H0_bits / N
x5 = H1_bits / H0_bits, or 0 if H0_bits == 0
x6 = match_coverage
x7 = coded_bits_per_occurrence
```

The pilot may emit these raw features as diagnostics. It may not add, drop, reorder,
or transform a feature before model fitting except for the frozen standardization in
section 5.

---

## 5. O11 calibration model

### 5.1 O11 label

For every candidate row in D1-D4, the target is:

```text
y = log2(max(1, o11_isolated_q11_bytes))
```

where `o11_isolated_q11_bytes` is frozen G3's exact Brotli q11/lgwin30 size of
`o(c)`. It is not a complete-carrier byte value.

O11 remains an offline reference. No production or pilot selection path may read an
O11 score at input time.

### 5.2 File-balanced fitting weight

D3 has far more candidates than D1/D2/D4 and must not dominate solely by row count.
Each training file has total weight 1.0. Within file `f`:

```text
row_weight(f,c) = 1 / candidate_count(f)
```

The weighted mean, weighted standard deviation, and weighted ridge normal equations
use these exact weights.

### 5.3 Frozen solver

The model is linear in standardized features:

```text
z_j = (x_j - mean_j) / scale_j
yhat = intercept + sum(j=0..7) coefficient[j] * z_j
```

Rules:

- arithmetic is IEEE-754 binary64;
- feature order is exactly section 4.7;
- unpenalized intercept;
- ridge lambda is exactly `1.0`;
- the intercept column is excluded from the ridge penalty;
- solve the fixed-order weighted normal equations directly;
- no iterative optimizer, regularization search, feature selection, interaction, or
  nonlinear term is permitted;
- if a feature standard deviation is zero, its scale is exactly 1.0;
- if the normal equations are singular or non-finite, the model is
  `INVALID-PILOT-MODEL`;
- coefficients, means, scales, and the model hash must be emitted in decimal JSON
  numbers without rounding.

### 5.4 Leave-one-file-out discovery discipline

For each D1-D4 file `f`:

1. fit the model using the other three files only;
2. emit `model-lofo-f.json`;
3. use only that LOFO model to score P0 on `f`.

After all four files have been evaluated, fit one final model on all D1-D4 and emit
`model-final.json` for a possible future held-out workflow.

The final all-four-file model must not affect any D1-D4 discovery result. Its only
admissible use is a separately frozen future held-out run.

### 5.5 Calibration diagnostics

Report, per LOFO fold and aggregate:

- weighted mean absolute log2 error;
- weighted median absolute log2 error;
- weighted RMSE in log2;
- top-one leaf agreement with O11 under `REGION_MIXED`, weighted by exact token bytes;
- unweighted top-one leaf agreement;
- near-tie diagnostic excluding O11 top-two gaps of at most one byte.

These are diagnostics. Final complete-carrier bytes remain authoritative.

---

## 6. P0 analytical policy

P0 uses the LOFO model appropriate to the measured file.

For each family mask and slot:

1. compute the frozen prediction for every eligible candidate;
2. choose the smallest `yhat`;
3. an exact score tie chooses `RAW_LEX`;
4. then the lower leaf ID;
5. then frozen candidate enumeration order.

P0 performs zero q1, q4, q6, or q11 calls.

P0 emits one complete carrier for each of the four family masks.

---

## 7. Bounded sampled q4 calibration

### 7.1 Total budget

P1 may perform at most:

```text
80 isolated Brotli q4/lgwin30 calls per file
```

This cap includes all strata, all rounds, and all q4 calibration inputs. It may not be
raised per file, after observing a file, or to rescue a failed arm.

If fewer than 80 distinct objects exist, the unused budget is not spent elsewhere.

### 7.2 Sample input

For a selected candidate object `o(c)`:

```text
sample_input(c) = o(c)                         if len(o(c)) <= 8192
                  first 8192 bytes of o(c)     otherwise
```

Every sampled call records:

- whether the input was exact or prefix-capped;
- full object bytes;
- sampled input bytes;
- quality 4 and window 30;
- actual encoded bytes;
- whether an identical sample input reused an earlier result.

Hashes order candidates but never establish equality. A sampled result may be reused
only after exact input-byte equality or exact full-object equality where applicable.

### 7.3 Strata

Each candidate belongs to exactly one stratum:

- leaf ID: one of the five frozen leaves;
- full-object size bin:
  - `S0`: 1..256 bytes;
  - `S1`: 257..4096 bytes;
  - `S2`: 4097..65536 bytes;
  - `S3`: greater than 65536 bytes.

There are at most 20 strata.

### 7.4 Outcome-blind order key

For candidate identity:

```text
order_key(c) = SHA-256(
    canonical_bytes(file_sha256)
    || 0x1F
    || shape_id_le_u64
    || slot_id_le_u64
    || leaf_id_le_u64
)
```

The digest is compared as an unsigned big-endian integer. Ties use lower
`(shape_id, slot_id, leaf_id)`.

No score, O11 label, final byte result, or policy result may enter this key.

### 7.5 First 40 calls

Within each stratum, take up to two candidates by ascending `order_key`.

If a stratum is absent or contributes fewer than two candidates, fill the unused
first-round positions from all remaining unsampled candidates by ascending
`order_key`, without duplicating an object.

The first round therefore performs `min(40, distinct_candidate_count)` q4 calls.

### 7.6 Residual and uncertainty

For each sampled object define:

```text
residual = log2(
    max(1, q4_sample_input_bytes)
    / max(1, exp2(p0_predicted_object_bytes))
)
```

The initial pooled population standard deviation is:

```text
sigma = sqrt(sum((residual_i - mean_residual)^2) / n)
```

If `n < 2` or the result is non-finite, `sigma = 0`.

### 7.7 Remaining 40 calls

Rank all unsampled candidates by:

```text
priority(c) =
    R(c) * (p0_predicted_log2_bytes(c) + 1.64 * sigma)
```

Descending priority wins. Ties use ascending `order_key`.

Evaluate up to 40 additional distinct sample inputs or until 80 total calls are
reached.

This is a deterministic contextual bandit/racing allocation. It uses no random seed,
no Thompson sample, and no policy outcome.

### 7.8 Stratum correction

For candidate `c`, let its stratum contain `n_s` sampled residuals. Define:

```text
median_residual_s =
    lower middle value                  if n_s is odd
    arithmetic mean of two middle values if n_s is even
    0                                  if n_s == 0

shrinkage_s = n_s / (n_s + 8)

stratum_correction(c) =
    shrinkage_s * median_residual_s
```

The correction constants are frozen:

```text
initial calls       = 40
total calls         = 80
UCB multiplier      = 1.64
shrinkage prior     = 8
sample input cap    = 8192
```

### 7.9 P1 policy

P1 uses the same LOFO P0 prediction and adds the frozen stratum correction:

```text
p1_score(c) = p0_score(c) + stratum_correction(c)
```

Selection and tie rules are identical to P0.

q4 sample results calibrate scores; they never directly choose a candidate and never
choose a final family.

---

## 8. Hierarchical candidate shortlist

Before constructing a family-masked selection, the pilot forms the following shortlist
for each slot:

1. always include `RAW_LEX`;
2. include `EXACT_DICT` when eligible;
3. include the lowest-P0-score eligible integer leaf;
4. include the two lowest-P0-score candidates overall;
5. union the above without duplicates.

P0 and P1 then choose only within the applicable frozen family mask and the frozen
slot shortlist.

Rules:

- the shortlist may not be changed after observing O11, q4, or final bytes;
- the shortlist size may not exceed the number of frozen eligible candidates;
- no future sixth leaf is authorized;
- a tied shortlist rank uses the P0 tie rule;
- raw must remain available to every mask.

---

## 9. Logical finalists and exact deduplication

P0 and P1 each produce four family-masked carriers. The bounded portfolio has eight
logical finalists:

```text
(P0, REGION_RAW)
(P0, REGION_DICT)
(P0, REGION_INT)
(P0, REGION_MIXED)
(P1, REGION_RAW)
(P1, REGION_DICT)
(P1, REGION_INT)
(P1, REGION_MIXED)
```

Logical policy order is `P0`, then `P1`.

A carrier may be reused only after exact carrier-byte equality. SHA-256 may accelerate
lookup, but collision handling must fall back to byte comparison. The output must
record distinct and reused result counts before and after deduplication.

Exact deduplication does not remove a logical finalist. A duplicated carrier may occupy
multiple logical ladder positions, and every reused result is counted separately from
a paid backend call.

---

## 10. Whole-carrier successive-halving ladder

### 10.1 Stage R1: q1

Evaluate every distinct carrier among the eight logical finalists once at Brotli
q1/lgwin30.

At most eight q1 calls are paid.

Rank logical finalists by:

```text
(q1_complete_bytes, family_rank, policy_rank, frozen_selection_hash)
```

Keep the first four logical positions.

### 10.2 Stage R2: q4

Evaluate each distinct carrier in the retained four at q4/lgwin30.

At most four q4 calls are paid.

Rank by:

```text
(q4_complete_bytes, q1_complete_bytes, family_rank, policy_rank,
 frozen_selection_hash)
```

Keep the first two logical positions.

### 10.3 Stage R3: q6

Evaluate each distinct carrier in the retained two at q6/lgwin30.

At most two q6 calls are paid.

Rank by:

```text
(q6_complete_bytes, q4_complete_bytes, q1_complete_bytes, family_rank,
 policy_rank, frozen_selection_hash)
```

Keep both positions.

### 10.4 Stage R4: q11

Evaluate each distinct retained carrier at q11/lgwin30.

At most two q11 planner-finalist calls are paid.

The ladder result is the smaller q11 complete-byte result of the two retained
positions. An exact tie uses the same frozen order.

No q11 result may rank more than these two finalists.

### 10.5 Final production-shaped portfolio

Compare:

1. the two ladder survivors;
2. raw Brotli q11/lgwin30;
3. frozen G2 whole-file q11/lgwin30 when eligible.

The selected complete-byte minimum is `C_prod`.

Exact complete-byte tie order is:

1. raw Brotli;
2. G2 whole;
3. ladder survivor with lower q11 bytes;
4. family rank;
5. policy rank.

The final portfolio may not select P0 or P1 by observed discovery bytes.

---

## 11. O11 and K-main controls

### 11.1 O11 reference

For each file, build frozen G3 O11 selections and four regional carriers. Define:

```text
C_ref(f) = min_F C(f,O11,F)
```

O11 must be rebuilt from frozen G3 in the same CI job and must match any reused G4
carrier identity records where available. It remains a reference, not an optimum.

### 11.2 K-main bounded oracle

After the ladder measurements, evaluate every distinct P0/P1 family carrier at q11 if
its q11 result was not already paid by the ladder.

Define:

```text
C_K(f) = min over P in {P0,P1}, F of C(f,P,F)
```

K-main requires at most eight distinct q11 calls in total. The two ladder q11 results
are reused; at most six additional K-main q11 calls are paid.

K-main is a bounded research oracle over the frozen eight finalists. It is not a
global optimum and may not enter production selection.

### 11.3 Historical G4 S2 control

G4 S2 remains a frozen negative control from run `35952830106`. It may be imported as
historical byte evidence with its artifact identity. It may not be used to select a
pilot arm, retune a threshold, or substitute for same-job timing.

---

## 12. Frozen byte and regret metrics

For each file define:

```text
C_ref(f)      = min_F C(f,O11,F)
C_P0(f)       = min_F C(f,P0,F)
C_P1(f)       = min_F C(f,P1,F)
C_K(f)        = min(P in {P0,P1}, F) C(f,P,F)
C_ladder(f)   = q11 bytes of the ladder survivor
C_prod(f)     = min(C_ladder(f), C_raw(f), C_g2(f) if eligible)
```

Primary fidelity metrics exclude raw and G2:

```text
P0_O11_regret(f) = (C_P0(f) - C_ref(f)) / C_ref(f)
P1_O11_regret(f) = (C_P1(f) - C_ref(f)) / C_ref(f)

P0_aggregate_regret =
    (sum_f C_P0(f) - sum_f C_ref(f)) / sum_f C_ref(f)

P1_aggregate_regret =
    (sum_f C_P1(f) - sum_f C_ref(f)) / sum_f C_ref(f)

ladder_K_regret(f) =
    (C_ladder(f) - C_K(f)) / C_K(f)

ladder_K_aggregate_regret =
    (sum_f C_ladder(f) - sum_f C_K(f)) / sum_f C_K(f)
```

Negative O11 regret means the pilot beat a reference planner. It must not be described
as beating a proven optimum.

`C_prod` and raw/G2 fallback savings are reported only as final-portfolio diagnostics.
They may not satisfy P1 fidelity or ladder gates.

Every byte result uses complete bytes, including all framing, envelope, carrier,
metadata, dictionary, raw residual, and backend bytes. Proxy transformed sizes and
isolated q4 scores are never complete-byte outputs.

---

## 13. Planner latency, encode time, decode, and RSS

### 13.1 Required stage timers

For every file and arm, separately report:

```text
structural_parse_ms
candidate_materialization_ms
mdl_feature_ms
o11_model_fit_ms                 # LOFO timing, reference only
sample_ordering_ms
sampled_q4_total_ms
carrier_build_ms_by_logical_arm
whole_q1_total_ms
whole_q4_ladder_total_ms
whole_q6_ladder_total_ms
whole_q11_ladder_total_ms
final_raw_q11_ms
final_g2_q11_ms
selection_arbitration_ms
decode_ms_by_finalist
```

### 13.2 Planner-only quantity

The pilot planner latency excludes only the O11 model fit and final q11 finalist/fallback
encodes:

```text
T_plan(P0) =
    structural_parse_ms
  + candidate_materialization_ms
  + mdl_feature_ms
  + selection_ms

T_plan(P1) =
    structural_parse_ms
  + candidate_materialization_ms
  + mdl_feature_ms
  + sample_ordering_ms
  + sampled_q4_total_ms
  + all ladder carrier-build time through q6
  + selection_arbitration_ms
```

P1 planner speedup is:

```text
speedup_plan = T_plan(O11 ranking+selection) / T_plan(P1)
```

O11 ranking time includes isolated q11 scoring and selection. It excludes O11's four
final carrier encodes so the quantity matches the gated ranking work.

### 13.3 Actual total encode time

Production-shaped encode time must include all paid work, including losing finalists:

```text
T_encode(P1) =
    T_plan(P1)
  + whole_q11_ladder_total_ms
  + final_raw_q11_ms
  + final_g2_q11_ms
  + all required carrier reconstruction/roundtrip checks
```

O11 total encode time is the same-job frozen-G3 end-to-end encode measurement.

```text
speedup_total_encode = T_encode(O11) / T_encode(P1)
```

The pilot may not report only the selected arm's encode time.

### 13.4 Repetition protocol

For each fresh process:

1. one complete warm-up measurement that is not included in timing aggregates;
2. five measured repetitions;
3. fixed arm order;
4. no dropping of a slow repetition;
5. report all five values, median, minimum, maximum, and paired median difference.

Bytes and carrier hashes must be identical across repetitions. A byte mismatch is a
correctness failure, not a timing outlier.

### 13.5 RSS

Peak RSS is measured once in a fresh process for:

- frozen O11;
- P0;
- P1;
- raw Brotli q11.

Use `/usr/bin/time -v` or an equivalent process-level maximum-RSS mechanism. The same
runner, limits, compiler, and corpus file are used.

### 13.6 Throughput

Report:

```text
encode_MB_s = source_bytes / T_encode(P1) / 1,000,000
decode_MB_s = source_bytes / median_final_decode_ms / 1000
```

Throughput is diagnostic for this pilot. It is not compared numerically with Class A
or other CI environments.

---

## 14. Frozen call accounting

### 14.1 Production-shaped P1 maximum

| Work class | Maximum paid calls |
|---|---:|
| isolated sampled q4 | 80 |
| whole-carrier q1 | 8 |
| whole-carrier q4 ladder | 4 |
| whole-carrier q6 ladder | 2 |
| whole-carrier q11 ladder | 2 |
| raw q11 | 1 |
| G2 q11 | 1 if eligible |
| q11 candidate ranking | 0 |

Therefore:

```text
q11_rank_calls(P1) = 0
final_q11_calls(P1) <= 4 per file
```

Exact deduplication may reduce paid calls. Reused results must be reported separately
and may never be described as saved calls that the production path did not make.

### 14.2 Research-only controls

| Work class | Maximum/count |
|---|---:|
| O11 isolated q11 | exactly frozen candidate count |
| O11 final q11 | 4 per file |
| K-main q11 total | at most 8 distinct per file |
| Additional K-main q11 after ladder | at most 6 per file |

O11 and K-main work is never counted as production cost.

### 14.3 Required counters

Every row must include exact integers for:

```text
candidates_enumerated
slots_eligible
o11_q11_candidate_calls
o11_final_q11_calls
p0_sampled_q4_calls
p1_sampled_q4_calls
p1_sampled_q4_reused_results
p1_sample_input_exact
p1_sample_input_prefix
p1_whole_q1_calls
p1_whole_q4_calls
p1_whole_q6_calls
p1_whole_q11_calls
k_main_q11_calls
k_main_q11_additional_calls
final_raw_q11_calls
final_g2_q11_calls
logical_finalists
distinct_finalist_carriers
reused_finalist_results
```

Call counters must be deterministic for identical input. A changed count is an
`INVALID-PILOT` measurement failure.

---

## 15. Correctness and identity gates

Every measured file must pass all of the following before a byte or timing result is
scientifically interpreted:

1. score-free candidate structure exactly matches frozen G3;
2. every P0/P1 family carrier round-trips byte-for-byte;
3. O11 carrier bytes and hashes match frozen G3 identity;
4. `REGION_RAW` is byte-identical across P0, P1, and O11;
5. every sampled q4 result is produced from the recorded exact sample input;
6. every ladder finalist has the recorded actual Brotli quality and lgwin;
7. all complete-byte values include the frozen envelope and metadata;
8. all peak-RSS and timing processes exit successfully;
9. all model and sample artifacts satisfy their schemas and hashes;
10. all mandatory G3 adversarial decoder tests pass unchanged;
11. the working implementation and prereg blobs match the frozen CI identities;
12. no corpus file differs from the frozen identity.

Any failure yields `INVALID-PILOT`. Invalid data may not be used to tune or rerun until
pass. A corrected implementation requires a new frozen source SHA and complete rerun.

---

## 16. Frozen scientific gates

### 16.1 Fidelity gates

P1 must satisfy both:

```text
P1_aggregate_regret <= 0.0025
max_f P1_O11_regret(f) <= 0.0050
```

Raw and G2 fallback may not rescue a failure.

### 16.2 Ladder-recovery gates

The ladder must satisfy both:

```text
ladder_K_aggregate_regret <= 0.0010
max_f ladder_K_regret(f) <= 0.0025
```

This tests whether low-quality rounds preserve the bounded eight-finalist portfolio.

### 16.3 Speed gates

Mandatory:

```text
speedup_plan >= 5.0
speedup_total_encode >= 5.0
```

Target:

```text
speedup_plan >= 10.0
speedup_total_encode >= 10.0
```

Passing bytes cannot rescue a speed failure. Passing speed cannot rescue a byte
failure.

### 16.4 RSS gate

```text
peak_rss(P1) <= peak_rss(O11)
```

Peak RSS is measured for a fresh P1 process that performs the complete bounded
portfolio, not a reduced analysis-only process.

### 16.5 Architectural-cost gate

The frozen production-simulation model must require no more than:

```text
65536 bytes of compiled encoder model/constants
```

The decoder model delta must be exactly zero. Diagnostic JSON model artifacts are
research evidence and are not assumed to be production binary content.

If the implementation needs dynamic code generation, an external model file, or a
decoder-visible model identifier, the pilot is outside this preregistration.

### 16.6 Call-budget gate

All section 14 counters must satisfy their exact maxima. Any q11 candidate-ranking
call is an unconditional failure.

---

## 17. Mechanical decision matrix

### `INVALID-PILOT`

Any of:

- source/prereg/corpus/backend identity failure;
- correctness, roundtrip, hash, schema, or call-counter failure;
- non-finite or singular model;
- infrastructure timeout before the complete frozen population is measured;
- incomplete or selectively dropped file.

No byte, speed, or planner conclusion is permitted.

### `NO-GO-PILOT-FIDELITY`

Correctness passes, but either P1 O11 fidelity gate fails.

No production planner exists. Do not tune features, sample constants, shortlist rules,
or q4 inputs on D1-D4.

### `NO-GO-PILOT-LADDER`

P1 fidelity passes, but a ladder-to-K-main gate fails.

The analytical selector may be diagnostic, but the low-quality finalist ladder is
rejected for this pilot.

### `NO-GO-PILOT-SPEED`

Byte gates pass, but a mandatory speed, RSS, architectural-cost, or call-budget gate
fails.

The result is planner-fidelity evidence only, not an operational planner.

### `PASS-PILOT-DISCOVERY`

All of the following hold:

- correctness and identity gates pass;
- P1 O11 fidelity gates pass;
- ladder-to-K gates pass;
- mandatory speed gates pass;
- RSS and architectural-cost gates pass;
- call budgets pass.

`PASS-PILOT-DISCOVERY` means only:

> On the already exposed D1-D4 objects, the frozen bounded P1 pilot approached
> frozen O11 complete-carrier bytes and passed the operational discovery gates.

It does not mean:

- production planner success;
- representation generalization;
- global optimality;
- a Pareto crossing;
- held-out validation;
- permission to modify `src/anvil.cpp`.

A future held-out V2 workflow may use the final model only if this exact discovery
implementation, model artifact, prereg, and binary identities are pinned.

---

## 18. Leakage and tuning rules

### 18.1 Fixed before measurement

Before any D1-D4 pilot run, freeze and publish:

1. this preregistration;
2. the conforming pilot source;
3. its immutable public commit SHA;
4. source and prereg git blob identities;
5. source and prereg SHA-256 identities;
6. frozen G3 source identity;
7. compiler and Brotli package identity;
8. D1-D4 identities;
9. all model, sample, shortlist, ladder, tie, and gate constants in this document;
10. the manual workflow source blob.

### 18.2 Forbidden after discovery measurement

The following may not change after any D1-D4 outcome is observed:

- a feature definition or feature order;
- ridge lambda, weighting, solver, or target;
- q4 sample cap, strata, order key, UCB multiplier, shrinkage, or prefix cap;
- P0/P1 policy definitions;
- shortlist rule;
- policy or family count;
- quality ladder;
- q11 finalist count;
- raw/G2 treatment;
- byte, regret, speed, RSS, memory, or call gates;
- tie order;
- corpus set;
- artifact schema in a way that changes scientific meaning.

### 18.3 Development versus held-out

- D1-D4 are development data.
- G3 V1 is known stress and is excluded.
- P0 LOFO prevents file-level target leakage inside D1-D4.
- The all-four-file model is not used on D1-D4.
- A future V2 corpus must be frozen by name, bytes, git blob, and SHA-256 before fetch.
- V2 thresholds may not be adjusted after its result.
- Any adjustment after V2 requires V3.
- Confidence intervals for future validation must be blocked by file or corpus family,
  never by individual candidate or slot.

---

## 19. Required machine-readable artifacts

All JSON uses UTF-8, sorted object keys where practical, integers for counts and
bytes, and finite JSON numbers for timing and regression outputs. Unknown fields are
allowed; missing required fields are invalid.

### 19.1 `results/discovery-provenance.json`

```json
{
  "schema": "anvil.g5-planner-pivot.provenance",
  "schema_version": 1,
  "experiment": "G5-PLANNER-PIVOT",
  "role": "discovery",
  "prereg": "docs/I10-G5-PLANNER-PIVOT-PREREG.md",
  "prereg_sha256": "64 lowercase hex",
  "implementation_sha": "40 lowercase hex",
  "implementation_source_path": "tools/grotli_g5_planner_pivot.cpp",
  "implementation_source_sha256": "64 lowercase hex",
  "implementation_source_blob": "40 lowercase hex",
  "frozen_g3_sha": "40 lowercase hex",
  "frozen_g3_source_sha256": "64 lowercase hex",
  "compiler": "string",
  "brotli_package_identity": ["string"],
  "runner": "string",
  "corpus": [
    {
      "id": "D1|D2|D3|D4",
      "name": "string",
      "bytes": 0,
      "git_blob": "40 lowercase hex",
      "sha256": "64 lowercase hex",
      "role": "discovery-development"
    }
  ],
  "opens_held_out_corpus": false
}
```

### 19.2 Model artifact

```json
{
  "schema": "anvil.g5-planner-pivot.model",
  "schema_version": 1,
  "model_role": "lofo|final",
  "training_file_ids": ["D1"],
  "feature_order": [
    "log2_object_bytes",
    "log2_occurrences",
    "distinct_per_occurrence",
    "log2_payload_minus_raw",
    "h0_bits_per_byte",
    "h1_over_h0",
    "match_coverage",
    "coded_payload_bits_per_occurrence"
  ],
  "mean": [0.0],
  "scale": [1.0],
  "coefficient": [0.0],
  "intercept": 0.0,
  "ridge_lambda": 1.0,
  "row_weight_rule": "equal_file_total_weight",
  "target": "log2_o11_isolated_q11_bytes",
  "solver": "weighted_binary64_normal_equations_v1",
  "implementation_sha": "40 lowercase hex",
  "prereg_sha256": "64 lowercase hex",
  "training_labels_sha256": "64 lowercase hex"
}
```

Exactly eight numeric entries are required in `mean`, `scale`, and `coefficient`.
All scale values must be finite and nonzero.

### 19.3 Sampled q4 artifact

One JSON object per file:

```json
{
  "schema": "anvil.g5-planner-pivot.q4-samples",
  "schema_version": 1,
  "file_id": "D1",
  "file_sha256": "64 lowercase hex",
  "total_calls": 0,
  "total_reused_results": 0,
  "initial_call_budget": 40,
  "total_call_budget": 80,
  "ucb_multiplier": 1.64,
  "shrinkage_prior": 8,
  "sample_input_cap_bytes": 8192,
  "pooled_sigma": 0.0,
  "samples": [
    {
      "shape_id": 0,
      "slot_id": 0,
      "leaf_id": 0,
      "occurrence_count": 0,
      "full_object_bytes": 0,
      "sample_input_bytes": 0,
      "sample_input_kind": "exact|prefix",
      "sample_input_sha256": "64 lowercase hex",
      "q4_complete_payload_bytes": 0,
      "p0_predicted_object_bytes": 0.0,
      "log2_residual": 0.0,
      "order_key_sha256": "64 lowercase hex",
      "round": 0,
      "priority": 0.0,
      "reused": false
    }
  ]
}
```

### 19.4 Per-file discovery row

```json
{
  "schema": "anvil.g5-planner-pivot.file-row",
  "schema_version": 1,
  "experiment": "G5-PLANNER-PIVOT",
  "file": {
    "id": "D1",
    "name": "string",
    "bytes": 0,
    "sha256": "64 lowercase hex"
  },
  "identity": {
    "implementation_sha": "40 lowercase hex",
    "prereg_sha256": "64 lowercase hex",
    "frozen_g3_sha": "40 lowercase hex",
    "brotli_encoder_version": 0,
    "brotli_encoder_version_dotted": "string"
  },
  "correctness": {
    "candidate_identity": true,
    "all_roundtrips": true,
    "o11_identity": true,
    "raw_family_identity": true,
    "schema_valid": true
  },
  "counts": {
    "candidates_enumerated": 0,
    "slots_eligible": 0,
    "logical_finalists": 8,
    "distinct_finalist_carriers": 0
  },
  "p0": {
    "arms": {},
    "calibration": {}
  },
  "p1": {
    "arms": {},
    "sample_artifact": "string",
    "calibration": {}
  },
  "o11": {
    "arms": {},
    "C_ref": 0
  },
  "k_main": {
    "C_K": 0,
    "q11_calls": 0,
    "additional_q11_calls": 0
  },
  "ladder": {
    "r1_q1": [],
    "r2_q4": [],
    "r3_q6": [],
    "r4_q11": [],
    "C_ladder": 0
  },
  "final_portfolio": {
    "C_ladder": 0,
    "C_raw": 0,
    "C_g2": 0,
    "C_prod": 0,
    "selected_label": "string"
  },
  "regret": {
    "P0_O11_regret": 0.0,
    "P1_O11_regret": 0.0,
    "ladder_K_regret": 0.0
  },
  "timing_ms": {
    "o11_ranking_selection": 0.0,
    "p0_plan": 0.0,
    "p1_plan": 0.0,
    "p1_total_encode": 0.0,
    "o11_total_encode": 0.0,
    "final_decode": 0.0
  },
  "repetitions": {
    "measured": 5,
    "p1_total_encode_ms": [0.0],
    "o11_total_encode_ms": [0.0]
  },
  "peak_rss_bytes": {
    "o11": 0,
    "p0": 0,
    "p1": 0,
    "raw_q11": 0
  },
  "call_budget": {}
}
```

Every arm object must include:

```text
label
policy
family
complete_bytes
carrier_bytes
carrier_sha256
roundtrip
quality
lgwin
build_ms
encode_ms
decode_ms
```

### 19.5 Ruling artifact

```json
{
  "schema": "anvil.g5-planner-pivot.ruling",
  "schema_version": 1,
  "implementation_sha": "40 lowercase hex",
  "prereg_sha256": "64 lowercase hex",
  "frozen_g3_sha": "40 lowercase hex",
  "frozen_g4_sha": "6c556dc522112aff4b3566990965e000418c5a57",
  "frozen_g4_run": "35952830106",
  "files": {
    "expected": ["D1", "D2", "D3", "D4"],
    "measured": ["D1", "D2", "D3", "D4"]
  },
  "aggregate": {
    "C_ref": 0,
    "C_P0": 0,
    "C_P1": 0,
    "C_K": 0,
    "C_ladder": 0,
    "C_prod": 0
  },
  "gates": {
    "correctness": true,
    "P1_fidelity": true,
    "ladder_recovery": true,
    "speed": true,
    "rss": true,
    "architectural_cost": true,
    "call_budget": true
  },
  "failing_gates": [],
  "decision": "INVALID-PILOT|NO-GO-PILOT-FIDELITY|NO-GO-PILOT-LADDER|NO-GO-PILOT-SPEED|PASS-PILOT-DISCOVERY",
  "opens_held_out_corpus": false,
  "production_transform_id_allocated": false,
  "production_source_modified": false
}
```

The ruling must be generated mechanically from frozen constants. No human may select a
different arm after seeing D1-D4.

---

## 20. Required CI workflow contract

A workflow at `.github/workflows/anvil-i10-g5-planner-pivot.yml` may be created only
after all of the following exist:

1. `tools/grotli_g5_planner_pivot.cpp`;
2. a committed preregistration blob;
3. an immutable public implementation commit SHA;
4. source and prereg blob/SHA-256 identities;
5. a warning-clean source-only implementation review;
6. synthetic-only correctness evidence sufficient to authorize remote corpus work;
7. the required CLI and artifact schema.

Until then, a runnable workflow would be scientifically invalid because it could not
pin a real implementation. This preregistration intentionally does not add a
placeholder workflow.

The future workflow must be `workflow_dispatch` only and must:

1. check out the pinned implementation SHA with full history;
2. fail if the SHA, source path, prereg path, source blob, or prereg blob is a
   placeholder or mismatched;
3. require a clean pinned checkout;
4. record source/prereg SHA-256 and git blobs;
5. install pinned clang-18, Brotli, Python, and timing dependencies;
6. materialize and verify frozen G3 source before building;
7. build the pilot warning-clean against the materialized frozen G3 source;
8. build a separate frozen G3 reference and run both selftests;
9. verify D1-D4 identities remotely;
10. run LOFO fits and P0/P1 discovery;
11. run same-job O11, raw, G2, ladder, and K-main controls;
12. run one warm-up and five measured repetitions;
13. measure fresh-process peak RSS;
14. apply section 17 mechanically;
15. upload the complete machine-readable artifact set even for a negative result.

The workflow may not fetch V1 or any new held-out corpus.

---

## 21. Required source interface for a future pilot

A conforming source should provide at least:

```text
selftest
fit-model
measure-discovery
schema-check
```

Required properties:

- `selftest` covers all frozen G3 adversarial decoder cases plus MDL determinism,
  sample ordering, exact dedup, and ladder tie determinism;
- `fit-model` emits LOFO and final model artifacts without manually supplied
  coefficients;
- `measure-discovery` accepts only a frozen model artifact, corpus path, output path,
  and repetition count fixed by the workflow;
- `schema-check` fails closed on missing fields, non-finite numbers, call-budget
  excess, duplicate row labels, or non-deterministic bytes;
- no command may fetch a corpus or dispatch CI;
- no command may modify production source.

The exact CLI may differ only if the future workflow and source freeze preserve every
semantic requirement in this document.

---

## 22. Execution order

1. review and publish this preregistration without measuring D1-D4;
2. implement the isolated pilot source in a later authorized pass;
3. run synthetic-only correctness checks without corpus measurement;
4. freeze the public implementation commit;
5. record source and prereg git blobs and SHA-256 identities;
6. create the manual fail-closed workflow pinned to that commit;
7. review the workflow without dispatching it;
8. dispatch once for D1-D4 discovery;
9. preserve every artifact, including invalid and negative outcomes;
10. apply the frozen ruling mechanically;
11. close as no-go or `PASS-PILOT-DISCOVERY`;
12. only after a discovery pass, freeze a new held-out V2 corpus and workflow.

No production integration is authorized at any step in this pilot.

---

## 23. Production disposition

Regardless of the ruling:

- G4 remains `NO-GO-G4`;
- S0/S1/S2 remain rejected;
- no direct proxy retrofit is permitted;
- no q11 candidate-ranking planner is permitted;
- no production source change is permitted;
- no transform ID is allocated;
- raw Brotli remains the permanent fallback;
- a discovery pass is not a held-out pass;
- a held-out pass would authorize only a separate production-engineering decision.

The pilot closes planner research for this frozen representation if it fails fidelity,
ladder recovery, mandatory speed, RSS, architectural-cost, or call-budget gates. A
failed pilot may not be rescued by changing constants on D1-D4.
