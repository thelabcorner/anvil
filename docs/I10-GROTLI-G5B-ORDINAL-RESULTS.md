# ANVIL I10 - GROTLI G5B-ORDINAL Results

**Closed:** 2026-09-24  
**Preregistration:** `docs/I10-GROTLI-G5B-ORDINAL-PREREG.md`, frozen revision **r3**  
**Prereg git blob:** `1a45154f26ed98b7c5bc1d15bb1b5985d456347c`  
**Prereg SHA-256:** `b0c4c91de7a8a46a8fdea9f03742a01da68067fabb76f73fba3af6561ce3339f`  
**Frozen implementation commit:** `cf9306e34e98b35333b23757da88357cab92357e`  
**Frozen G5B source blob:** `ce8021a5f7f3d25319db1ccd22850c3144c03040`  
**Frozen G5B source SHA-256:** `910ddcf7ab8026ecca095199d921cfe823df64dd5bc9552617b9a1d73e0aa9f4`  
**Frozen G5B binary SHA-256:** `ea47f30c8f315db86d4e1b94c656323df53848dd574216ec10f8eb1fc776010f`  
**Authoritative workflow run:** `36011333908` (workflow_dispatch, `ANVIL I10 Grotli G5B-ORDINAL probe`, conclusion `success`)  
**Workflow head SHA:** `a443f37c0076591dd7efd2175859630e1e20f6c9`  
**Evidence artifact:** `grotli-g5b-ordinal-36011333908`, artifact id `10812278595`, size `196512`, digest `sha256:742dc2ee8a9b683104f6c00f93f26ed132274e41c22ae5b4113aaa4ab6e266af`  
**Frozen G5A floor:** run `35985412906`, artifact id `10801714249`, digest `sha256:4765b317e7c01170cffbd96b08c639e001ec1e2f317152d1f83a20a5331a3278`  
**Frozen G3 reference:** public SHA `1a3d18fed76adb6fb33264e1994f9c357306b3fa`, source blob `eedc7b7e6671c4a5fcaf7a5997671bd4be30bdde`, source SHA-256 `5b3ab1cdc67d1a8d8edd7eeb4265361a66f72e5b8620137149ce802233b58a9e`  
**Backend:** libbrotli `1.1.0-2build2` (Ubuntu 24.04), `BrotliEncoderVersion() = 16781312` (`1.1.0`), q11 / lgwin30  
**Decision:** **ORDINAL-ADVERSE** on frozen D1-D4 discovery

---

## 0. Executive ruling

G5B-ORDINAL asked one sharply causal question:

> Does lifting same-ordinal scalar slots across exact lexical-shape boundaries improve
> the G5A shape-column floor when every scalar chunk, charged envelope byte, source
> reconstruction fact, and Brotli backend parameter is held constant?

The answer on the frozen D1-D4 discovery set is **no**.

The experiment is valid. Every required provenance, build, corpus, correctness, floor,
and same-run replay gate passed:

- pinned G5B r3 implementation identity: **PASS**;
- warning-clean G5B build against materialized frozen G3 + selftest: **PASS**;
- separate frozen G3 reference build + selftest: **PASS**;
- archived G5A evidence identity and archive digest: **PASS**;
- D1-D4 + V1 corpus size/blob/SHA-256 identities: **PASS**;
- same-run frozen G5A replay: **PASS**;
- B1 byte/fact reproduction of frozen G5A A3 on D1-D4: **PASS**;
- G5B permutation, length, envelope, multiset, selector, and roundtrip invariants: **PASS**;
- evidence upload: **PASS**.

The frozen r3 first-match ruling was applied mechanically:

> **ORDINAL-ADVERSE**

because aggregate B2 `ORDINAL_BLOCKED` is larger than B1 `ORDINAL_FLOOR`:

```
B1 = 1,380,245
B2 = 1,403,029

B2 - B1 = +22,784 bytes
B2 / B1 = 1.0165072143
=> +1.650721% larger
=> ORDINAL-ADVERSE
```

This closes the **ordinal proxy only**. It does **not** close the semantic hierarchy /
path-aware lane, which remains **OPEN**. No production transform ID was allocated. No
new held-out corpus was opened. Raw Brotli remains the permanent fallback.

---

## 1. What was measured

G5B-ORDINAL retained the exact G5A carrier grammar and exact-shape representation and
changed only the ordering of the same canonical structured scalar chunks.

The three frozen arms were:

- **B0 `ORDINAL_NULL`**, selector 4 — one deterministic structure-destroying null;
- **B1 `ORDINAL_FLOOR`**, selector 3 — exact frozen G5A A3 `SHAPE_COLUMN`;
- **B2 `ORDINAL_BLOCKED`**, selector 5 — global ordinal blocking across exact shapes.

The common Brotli input body uses the frozen `G5AO` carrier magic. The one-byte arm
selector is charged outside Brotli exactly as preregistered.

Per file, all arms preserved:

- source bytes and exact roundtrip;
- structured/raw frame classification;
- exact lexical shapes and templates;
- frame-to-group reconstruction map;
- raw residual bytes;
- scalar token bytes;
- token count and token length framing;
- body length, envelope length, token-region length;
- common-envelope SHA-256;
- canonical token-multiset SHA-256;
- q11 / lgwin30 backend identity.

Therefore B1-vs-B2 isolates the effect of crossing exact-shape boundaries by **slot
ordinal only**.

---

## 2. Authoritative evidence

Machine-readable evidence is in artifact
`grotli-g5b-ordinal-36011333908`:

- `results/g5b-ruling.json`
- `results/g5b-ruling.md`
- `results/g5b-discovery.jsonl`
- `results/g5b-v1.jsonl`
- `results/floor-check.json`
- `results/g5a-replay-discovery.jsonl`
- `results/discovery-provenance.json`
- archived G5A floor evidence under `floor_evidence/`
- compiler/backend/source identity objects.

The authoritative measurement is GitHub Actions run `36011333908`; no workstation or
homelab result is used as evidence.

### 2.1 Aggregate D1-D4 complete bytes

| Arm | Complete bytes | vs B1 |
|---|---:|---:|
| B0 `ORDINAL_NULL` | **2,054,910** | +674,665 |
| B1 `ORDINAL_FLOOR` | **1,380,245** | baseline |
| B2 `ORDINAL_BLOCKED` | **1,403,029** | **+22,784 (+1.650721%)** |

The B2 null margin remains large:

```
B0 - B2 = 651,881 bytes
```

but the preregistered ruling does not permit the null to rescue a treatment that is
already worse than its causal floor. `B2 > B1` triggers **ORDINAL-ADVERSE** before
materiality/breadth/null promotion gates are considered.

### 2.2 Per-file facts

| File | B0 | B1 floor | B2 ordinal | B2-B1 | B2 vs B1 | Degenerate? |
|---|---:|---:|---:|---:|---:|:--:|
| D1 amazon cellphones | 49,870 | 39,463 | 39,463 | 0 | 0.0000% | yes |
| D2 cdisc adae | 92,210 | 25,717 | 25,855 | +138 | +0.5366% | no |
| D3 gharchive 10 MiB | 1,693,435 | 1,230,007 | 1,253,880 | **+23,873** | **+1.9409%** | no |
| D4 crovia dpi receipts | 219,395 | 85,058 | 83,831 | **-1,227** | **-1.4425%** | no |

Only D4 is smaller under B2 among non-degenerate files. D1 has one exact shape and
therefore correctly degenerates to B1 == B2 with zero moved tokens.

Frozen liveness facts:

| File | shapes | max slots | shared ordinal slots | cross-shape ordinal tokens | B2 moved tokens |
|---|---:|---:|---:|---:|---:|
| D1 | 1 | 9 | 0 | 0 | 0 |
| D2 | 2 | 284 | 55 | 55 | 65,788 |
| D3 | 167 | 533 | 378 | 21,362 | 259,154 |
| D4 | 42 | 45 | 44 | 1,201 | 81,934 |

The treatment is therefore live on D2-D4; the adverse aggregate is not explained by
a no-op treatment.

### 2.3 B1 exactly reproduces frozen G5A A3

B1 complete bytes equal the authoritative frozen G5A A3 values exactly:

| File | frozen G5A A3 | G5B B1 |
|---|---:|---:|
| D1 | 39,463 | 39,463 |
| D2 | 25,717 | 25,717 |
| D3 | 1,230,007 | 1,230,007 |
| D4 | 85,058 | 85,058 |
| **aggregate** | **1,380,245** | **1,380,245** |

The floor gate additionally verified per-file body bytes, envelope length,
token-region length, envelope SHA-256, canonical token-multiset SHA-256, structural
counts, backend version, archived G5A implementation/source identity, and same-run
G5A replay.

`results/floor-check.json`: **`ok: true`**.

---

## 3. Frozen r3 ruling recomputation

The authoritative ruling records:

```
floor_ok: true
invariants_ok: true
B0 = 2,054,910
B1 = 1,380,245
B2 = 1,403,029
b2_vs_b1_bytes = -22,784  # B1 - B2
b2_vs_b0_bytes = +651,881 # B0 - B2
b2_smaller_nondegenerate_files = 1
```

The r3 first-match order checks invalid states before scientific classification, then
checks adverse/neutral before promotion gates.

Since:

```
B2 > B1
```

the first applicable scientific label is:

> **ORDINAL-ADVERSE**

The 0.99 materiality threshold, two-file favorable breadth threshold, and null gate
are downstream and therefore moot for classification. This is deliberate
preregistered precedence, not post-result interpretation.

Arithmetic was re-verified against `results/g5b-discovery.jsonl`; no discrepancy was
found.

---

## 4. V1 known-stress diagnostic

V1 Sino-US DrugQA remains **KNOWN-STRESS / NOT HELD-OUT**.

| Arm | Complete bytes |
|---|---:|
| B0 | 2,543,441 |
| B1 | 2,183,986 |
| B2 | 2,181,849 |

B2 is 2,137 bytes smaller than B1:

```
B2 / B1 - 1 = -0.097849%
```

so the mechanically emitted diagnostic label is:

> **V1-ORDINAL-FAVORABLE**

This does not alter the D1-D4 **ORDINAL-ADVERSE** ruling. V1 was already known-stress
and is not evidence of held-out generalization.

The sign reversal itself is informative: ordinal coarsening can help some structural
regimes, but it is not a reliable compatibility relation across heterogeneous exact
shapes.

---

## 5. Causal interpretation

### 5.1 G5A survives; the coarse ordinal explanation does not

G5A established that ordering/locality materially changes compressed size while
holding the byte multiset and representation fixed. Its dominant component was
same-slot-across-rows **within exact shape**.

G5B tested the simplest possible cross-shape extension of that fact:

> treat slot ordinal as if it were enough to identify compatible fields across
> different exact shapes.

That extension is falsified on the frozen discovery set.

The result does **not** contradict G5A. Instead it narrows the mechanism:

- corresponding-slot locality **inside a coherent exact shape** is valuable;
- blindly merging equal ordinal positions **across different shapes** can destroy
  useful locality;
- therefore ordinal index is not a sufficient semantic compatibility key.

D3 provides the strongest evidence: the treatment is highly live
(`21,362` cross-shape ordinal tokens, `259,154` moved tokens) and gives back
`23,873` bytes relative to the exact-shape column floor.

D4 proves the opposite regime can exist: cross-shape ordinal blocking saves
`1,227` bytes. That prevents the stronger claim that cross-shape locality is
inherently harmful. The supported conclusion is narrower: **ordinal alone is an
insufficient grouping rule**.

### 5.2 The next question is semantic/path compatibility

The most direct hypothesis left by G5A + G5B is that the useful locality unit is not
"slot number" but a more meaningful coordinate such as structural path, field/key
identity, or another compatibility signature that survives shape variation.

This is not yet a result and must not be smuggled into the G5B conclusion.

The next anatomy experiment should therefore ask whether a **path-aware / field-aware
cross-shape grouping** can improve or at least preserve the exact-shape B1 floor
while keeping the same causal discipline:

- same frozen source/corpora and clearly labeled corpus roles;
- same scalar bytes and complete explanation-cost accounting;
- same reversible carrier facts;
- explicit metadata charging if path identity requires new transmitted information;
- exact roundtrip and multiset gates;
- a deterministic null/control;
- B1 exact-shape column floor retained as an already-spent baseline;
- no production promotion without a new held-out gate.

If path information can be derived deterministically from already-transmitted
structure, the experiment must prove that derivation and charge only genuinely new
information. If it requires additional metadata, those bytes must be included in
the complete carrier before any rate claim.

---

## 6. Infrastructure-invalid precursor run

Run `36010649558` is **infrastructure-invalid / premeasurement** and is **not
scientific evidence**.

It passed implementation/build/selftest provenance but failed while downloading the
archived G5A artifact: Python `urllib` forwarded the GitHub bearer token across the
artifact API's cross-host 302 redirect to signed blob storage, which returned HTTP
401.

No corpus measurement step executed. The fail-closed workflow emitted
`INVALID-G5B-ORDINAL` and prohibited B2 interpretation.

Workflow-only commit `a443f37c0076591dd7efd2175859630e1e20f6c9` is a transport-only
correction: it changed the downloader to use `add_unredirected_header`, so credentials
are sent to `api.github.com` but not forwarded to the signed-storage host. It did not
change the frozen r3 implementation, preregistration, corpora, arms, selectors,
thresholds, backend, or ruling semantics.

The authoritative scientific run is exclusively `36011333908`.

---

## 7. Disposition

**Closed: G5B-ORDINAL = ORDINAL-ADVERSE.**

Retain:

- G4 remains **NO-GO** (`NO-GO-G4`); no cheap planner-proxy claim is reopened by G5B;
- G5A `ORDER-MATERIAL / COLUMN-DOMINANT` as the established ordering/locality
  anatomy result;
- exact-shape B1/A3 as the floor for the next hierarchy experiment;
- raw Brotli as the permanent fallback;
- all provenance and causal-accounting rules.

Close:

- global same-ordinal-across-shapes as a candidate explanation for the G5A column
  effect;
- any attempt to rescue that proxy by tuning thresholds or adding post-hoc
  exceptions.

Keep open:

- semantic/path-aware cross-shape hierarchy;
- representation designs that recover compatible field identity without hiding
  metadata cost;
- later held-out validation only after a mechanism survives discovery causally.

No production transform ID. No new held-out corpus. No generalization claim.
