# ANVIL I10 - GROTLI G5A Ordering Attribution Results

**Closed:** 2026-09-24
**Preregistration:** `docs/I10-GROTLI-G5A-ORDERING-ATTRIBUTION-PREREG.md`, frozen revision **r6**
**Prereg git blob:** `a84f42a3f414cc987d8c19cc70a578e2d4222467`
**Frozen implementation commit:** `e6714e81aeff1579c4502f1fd9af4d7f205a8b4b`
**Frozen G5A source blob:** `2772d7eaf0a64be1fdbbb377f5d668d21dab9cbe`
**Frozen G5A source SHA-256:** `0fe9b8e599c922d3f489ffed54d595d867cfdf7f1ff0a6f6f785996cf88d382f`
**Frozen G5A binary SHA-256:** `119f1e661993976e14b98948538887cd3260ce22062f45f9c6b8e8f3778e4b00`
**Workflow run:** `35985412906` (workflow_dispatch, `ANVIL I10 Grotli G5A ordering attribution`)
**Workflow head SHA:** `996c2dbe67736c42288287abba7ed6f3307a0b34` (= frozen head, = `origin/main` at run time)
**Frozen G3 reference:** public SHA `1a3d18fed76adb6fb33264e1994f9c357306b3fa`, source blob `eedc7b7e6671c4a5fcaf7a5997671bd4be30bdde`, source SHA-256 `5b3ab1cdc67d1a8d8edd7eeb4265361a66f72e5b8620137149ce802233b58a9e`
**Brotli backend:** libbrotli 1.1.0-2build2 (Ubuntu 24.04), `BrotliEncoderVersion()` = `16781312` (`1.1.0`), q11 / lgwin30
**Decision:** **ORDER-MATERIAL / COLUMN-DOMINANT** (discovery D1-D4)

---

## 0. Executive ruling

G5A asked a single primitive question:

> How much compression effect is caused by the ordering/locality of the exact same
> structured scalar byte chunks before the same Brotli q11/lgwin30 backend?

The experiment is fully valid. Every provenance, build, selftest, corpus-identity,
and correctness-invariant gate passed:

- pinned implementation identity: **PASS**;
- warning-clean build (`-Wall -Wextra -Wpedantic -Werror`) compiled against the
  **materialized pinned frozen G3 source** (`frozen-grotli_g3.cpp`, confirmed in
  `grotli-g5a.d`): **PASS**;
- adversarial selftest of `grotli_g5a_ordering` and of the separate frozen G3
  reference (`grotli_g3`): **PASS**;
- frozen G3 source identity (`frozen_blob == implementation_tree_blob == expected`): **PASS**;
- D1-D4 + V1 corpus identity (size + git blob + SHA-256, re-verified at fetch): **PASS**;
- I1-I9 invariants on every row (roundtrip, body/envelope/token-region length
  identities, envelope identity, canonical token-multiset identity, exact
  permutations, mode pack/unpack, fixed backend, A0 present): **PASS**;
- artifact upload (`grotli-g5a-ordering-attribution-35985412906`): **PASS**.

The frozen r6 classification was applied mechanically inside the run:

> **ORDER-MATERIAL** with attribution **COLUMN-DOMINANT** on the frozen D1-D4
> discovery set.

The known-stress diagnostic is:

> **V1-COLUMN-ADVERSE** (A3 slightly *larger* than A1) — KNOWN-STRESS / NOT
> HELD-OUT FOR G5A.

No production transform ID was allocated. No new held-out corpus was opened. Raw
Brotli remains the permanent fallback. The result is a discovery-set anatomy
finding only (`opens_new_held_out_corpus: false`,
`production_transform_id_allocated: false`).

---

## 1. What was measured (frozen arm semantics)

Exactly the four frozen arms, differing only in the permutation of the same
canonical structured scalar chunks, over a byte-identical common envelope
(`magic 'G5AO'`, version, source/frame/group counts, group descriptors, raw
residual section) and a constant charged one-byte out-of-band order mode:

- **A0 `RANDOM_PERMUTATION`** — deterministic structure-destroying null (frozen
  seed, one draw);
- **A1 `SOURCE_ORDER`** — structured chunks in original structured-frame order;
- **A2 `SHAPE_ROW`** — grouped by exact shape (first-appearance), row-major inside each shape;
- **A3 `SHAPE_COLUMN`** — grouped by exact shape (first-appearance), slot-major inside each shape.

Complete bytes = `1 + brotli(common_body_with_arm_permutation)`. The mode byte is
charged in every arm's total and is not passed to Brotli, so it is equal-cost and
cannot confound the ordering comparison. All four arms had, per file, identical
`body_bytes`, identical `envelope_len`, identical `token_region_len`, identical
`envelope_sha256`, and identical `canonical_token_multiset_sha256`.

---

## 2. Machine-readable evidence (authoritative)

Ruling: `results/g5a-ruling.json` and `results/g5a-ruling.md` in the run artifact
`grotli-g5a-ordering-attribution-35985412906`. Per-file rows:
`results/g5a-discovery.jsonl` (D1-D4) and `results/g5a-v1.jsonl` (V1). All numbers
below are copied from those CI-produced objects; no local measurement was performed.

### 2.1 Aggregate (D1-D4 complete bytes)

| Arm | Complete bytes |
|---|---:|
| A0 `RANDOM_PERMUTATION` | **2,054,532** |
| A1 `SOURCE_ORDER` | **1,470,205** |
| A2 `SHAPE_ROW` | **1,452,383** |
| A3 `SHAPE_COLUMN` | **1,380,245** |

Signed decomposition (section 9, exact):

```
shape_bytes  = S1 - S2 = 1,470,205 - 1,452,383 =    17,822
column_bytes = S2 - S3 = 1,452,383 - 1,380,245 =    72,138
total_bytes  = S1 - S3 = 1,470,205 - 1,380,245 =    89,960
shape_bytes + column_bytes = 17,822 + 72,138 = 89,960 = total_bytes   (decomposition_ok: true)
```

Derived effects:

| Quantity | Bytes | Percent |
|---|---:|---:|
| shape grouping (A1->A2) | 17,822 | **-1.2122%** |
| column increment (A2->A3) | 72,138 | **-4.9669%** |
| total ordering (A1->A3) | 89,960 | **-6.1189%** |
| null margin `random_null_bytes = S0 - S3` | 674,287 | — |

Shares (signed, unclipped): `shape_share = 0.1981`, `column_share = 0.8019`.

### 2.2 Classification gate evaluation (frozen r6 precedence)

```
invariants_ok: true    decomposition_ok: true
A3 (1,380,245) >  A1 (1,470,205)?          NO   (A3 < A1  -> favorable direction)
A3 == A1?                                  NO
A3 >= A0 (2,054,532)?                      NO   (null PASSED: A3 < A0)
S3 / S1 = 0.93881 <= 0.99?                 YES  (>= 1.0% aggregate)
a3_smaller_files = 4 >= 2?                 YES
=> classification: ORDER-MATERIAL
column_share = 0.8019 >= 0.60?             YES
=> attribution: COLUMN-DOMINANT
```

`A3 / S1 = 0.938811` → **-6.1189%** aggregate. The A0 null was passed by a wide
margin (`S0 - S3 = 674,287` bytes, ≈ **-32.8%** of A1). Per section 10.1 and 4.1,
this is **one deterministic null draw, not a significance test**; no p-value or
"random is worse on average" language is used.

### 2.3 Per-file facts (D1-D4)

| File | A0 | A1 | A2 | A3 | shape bytes | column bytes | total bytes | A1->A3 | A3<A1 | chunks | shapes | raw frames |
|---|---:|---:|---:|---:|---:|---:|---:|---:|:--:|---:|---:|---:|
| D1 amazon cellphones | 49,919 | 43,083 | 43,083 | 39,463 | 0 | 3,620 | 3,620 | -8.4024% | yes | 7,137 | 1 | 0 |
| D2 cdisc adae | 92,317 | 25,751 | 25,751 | 25,717 | 0 | 34 | 34 | -0.1320% | yes | 65,789 | 2 | 0 |
| D3 gharchive 10 MiB | 1,693,167 | 1,293,490 | 1,275,139 | 1,230,007 | 18,351 | 45,132 | 63,483 | -4.9079% | yes | 259,196 | 167 | 1 |
| D4 crovia dpi receipts | 219,129 | 107,881 | 108,410 | 85,058 | **-529** | 23,352 | 22,823 | -21.1557% | yes | 82,768 | 42 | 0 |

Two structural facts are visible in this table:

- **A1 == A2 exactly (to the byte) on D1 and D2.** Both files have 1 and 2 shapes
  respectively; with source order already grouping runs of a single/mostly-single
  shape contiguously, the shape-row permutation degenerates to the identity. In
  these files the *entire* measured effect is the column increment.
- **D4's shape component is negative** (`shape_bytes = -529`, A2 slightly *larger*
  than A1) while its column component is the largest single-file effect
  (`-21.54%`). The signed shares are therefore not clipped: the column component's
  saving (23,352 B) exceeds D4's net saving (22,823 B) by exactly the 529 B that
  shape grouping gave back.

### 2.4 Context objects (NOT attribution evidence)

Reported as clearly labeled context only, sharing neither the common envelope nor
the causal gate (prereg section 5). Raw Brotli of the original source complete
bytes: D1 **40,126**, D2 **25,029**, D3 **1,292,757**, D4 **108,857**. These are
never compared to A0/A1/A2/A3 for attribution.

### 2.5 V1 known-stress diagnostic

V1 = Sino-US DrugQA (`all.jsonl`), 15,374,047 bytes, SHA-256
`757b9bc7e5ee2d38ab5ed43877b1d87809cb5131621d106671bc27996da1c499`. Role:
**KNOWN-STRESS / NOT HELD-OUT FOR G5A**.

| Arm | Complete bytes |
|---|---:|
| A0 | 2,542,767 |
| A1 | 2,182,265 |
| A2 | 2,182,432 |
| A3 | 2,183,986 |

`A1->A2 = -167` B, `A2->A3 = -1,554` B, `A1->A3 = +1,721` B (**+0.0789%**).
V1 invariant re-gate inside the final ruling heredoc: **clean** (all top-level
identities true, all arms roundtrip/permutation/mode-clean, envelope and multiset
hashes identical, q11/lgwin30). Therefore the permitted label **V1-COLUMN-ADVERSE**
was emitted: the shape-column permutation is *slightly larger* than source order on
V1, and the negative effect is concentrated entirely in the column increment
(A2->A3), with shape grouping essentially neutral-to-marginally-helpful
(A1->A2 = -167 B).

V1 is a diagnostic, not a held-out test. It cannot satisfy any generalization or
promotion gate, and it does not validate any G5A hypothesis.

---

## 3. Causal interpretation (discovery D1-D4 only)

### 3.1 Ordering/locality is materially causal for the frozen representation

Holding source bytes, parsing, frame split, structured/raw classification, exact
shapes, shape templates, frame->group map, raw residuals, scalar token bytes,
length framing, token multiset, common envelope, body length, and backend all
constant, and changing *only* the permutation of the same scalar chunks:

- the frozen shape-column ordering is **-6.1189%** smaller than source ordering in
  aggregate, and is smaller on **4/4** families;
- the same permutation beats a deterministic structure-destroying null
  (random permutation of the identical chunk multiset and identical body length) by
  **674,287 bytes** in aggregate.

Because the null destroys only locality while preserving multiset, body length,
envelope, and backend, the A1->A3 effect is attributable to **locality/order
structure of the exact same bytes**, not to "some permutation Brotli happens to
like", not to changed metadata, and not to a codec/leaf change. This is the direct
causal answer G5A was built to obtain: **byte ordering and locality are a real,
material contributor to the measured G3-style representation effect.**

### 3.2 The effect is column-dominated, i.e. a corresponding-scope (inter-row, same-slot) effect

The exact decomposition attributes **72,138 of 89,960 bytes (80.19%)** to the
**column increment** (shape-row -> shape-column) and only **17,822 bytes (19.81%)**
to **shape grouping** (source -> shape-row). Causally this says the dominant favorable
mechanism is **not** merely "collect equal shapes together". It is that, *after*
shapes are grouped, emitting the **same slot across many rows** (a
corresponding-placeholder / field-local coordinate stream) is what Brotli favors —
joint values of one field are brought adjacent, so repeated per-field structure and
near-repetitions become short-distance matches, whereas row-major order interleaves
different fields and buries that locality under high-entropy value changes.

### 3.3 Shape grouping is real but weak, and file-dependent — and can be locally adverse

The shape-grouping term is small (-1.21% aggregate) and **not uniformly favorable**:

- **D1 and D2: exactly zero.** With 1-2 shapes the shape-row permutation is the
  identity of source order (`A1 == A2` to the byte), so shape grouping has *no*
  effect there — a clean null-of-mechanism instance rather than a measured loss;
- **D3: -1.42%** (167 shapes, the largest shape count) — a real gain where shape
  heterogeneity actually exists;
- **D4: +0.49% adverse** (42 shapes) — grouping rows by shape *lost* 529 bytes, and
  the net D4 win came entirely from column ordering.

So the second-order shape-grouping effect tracks shape heterogeneity (it appears
where many distinct shapes exist, is exactly nil where shapes degenerate to one or
two, and can be outweighed by intra-shape row adjacency on a file with a moderate
shape count). This is exactly why G5A freezes shape-grouping and column-ordering as
**separable** components rather than one "structured ordering" knob.

### 3.4 The ordering effect is not uniform: it is magnitude-variable across families

The A1->A3 effect ranges from **-0.13%** (D2, 2 shapes, near-degenerate) through
**-4.91%** (D3) to **-21.16%** (D4). The high-shape-count but receipt-like D4 —
where the field-local column stream is strongly repetitive — shows by far the
largest response, while D2 (only 2 shapes, extremely residual-heavy structured
content) shows almost none. Ordering/locality is therefore a real mechanism, but its
*yield is corpus-shape-dependent*, which is consistent with the broader G-series
picture: the representation helps where it exposes locality and does little where
locality is already implicit or where shapes barely differentiate.

### 3.5 The known-stress diagnostic: ordering is not the (whole) V1 failure story

On V1 (3 shapes, 180,738 chunks) the same frozen permutation is **+0.0789% adverse**:
column ordering is marginally *worse* than source order and the loss is entirely in
the column increment (`A2->A3 = -1,554 B`), with shape grouping essentially neutral
(`A1->A2 = -167 B`). This is a diagnostic, **not** a held-out test and **not** a
validation. It is informative in one bounded direction only: on this known-stress
population — whose G3 held-out result was already **+3.19% worse** than raw Brotli —
switching to shape-column ordering would *not* by itself recover the loss; the
column permutation is not a rescue for the V1 failure. The magnitude of the V1
column effect (+0.08%) is tiny relative to the G3 V1 shortfall (+3.19%), so nothing
here rules ordering/locality in or out as a *component* of that failure; G5A simply
does not supply a favorable ordering signal on V1.

### 3.6 What this does and does not establish

Established (discovery set D1-D4 only):

1. Byte ordering/locality of identical scalar chunks is **materially causal**
   (-6.12% aggregate, 4/4 files, null passed).
2. The effect is **column-dominated** (80.19% of the net saving from same-slot
   across-row ordering).
3. Shape grouping is a **real but weak and file-dependent** second-order term
   (zero on degenerate-shape files, sizable on high-shape-count files, locally
   adverse on D4).

Not established (and explicitly out of scope per prereg sections 12-13):

- any production planner, transform ID, or wire format;
- broad generalization of the current representation (D1-D4 are no longer unseen);
- that shape hierarchy is beneficial (that is G5B, separate);
- that V1 validates any G5A hypothesis;
- any claim from the pre-freeze tiny synthetic fixtures (inadmissible, r6 7.1a) or
  from the context objects (raw Brotli / G3 regionized), which are not attribution
  evidence.

---

## 4. Discipline and provenance notes

- **No local measurement.** No D1-D4/V1 or heavy benchmark was run on the
  workstation. CI artifact/logs are the only measurement evidence; the artifact was
  downloaded read-only into the in-repo gitignored `scratch/` path as a local copy of
  the GitHub artifact and is not itself treated as evidence.
- **No moving gates.** Arm set, thresholds (0.99 aggregate, 2-of-4 files, 0.60
  shares), corpus, backend, and classification precedence are exactly the frozen r6
  values; the run applied them mechanically.
- **Provenance chain verified before/independently of outcome:** implementation
  commit, source blob, prereg blob, frozen G3 head/blob/SHA-256,
  `frozen_blob == implementation_tree_blob`, `grotli-g5a.d` showing compilation
  against `frozen-grotli_g3.cpp`, both selftests PASS, and runtime libbrotli `.so`
  SHA-256s captured additively (r6 provenance E).
- **Negative/unfavorable facts preserved:** the D4 shape term is adverse; V1's
  ordering diagnostic is adverse; three of four D1-D4 rows show a sub-1% overall
  effect; the aggregate -6.12% is dominated by two files (D3, D4). None of this was
  smoothed, clipped, or reframed.

---

## 5. Disposition and next scientific decision

**Disposition:** G5A is **CLOSED** as a discovery-set anatomy result. No production
transform ID, no promotion, no production source change, no new held-out corpus. Raw
Brotli remains the permanent fallback.

**Permitted successor (per frozen `docs/I10-GROTLI-FRONTIER-ARCHITECTURE.md` section
11): G5B — shape hierarchy** (exact flat shapes versus explicitly preregistered
structural hierarchy, leaf vocabulary fixed). G5B is a *separate* experiment; it may
use this result as motivation and architecture context but may not retroactively
change G5A arms or gates (prereg section 15). **This document does not dispatch
G5B** and no existing pre-frozen continuation rule requires an automatic G5A->G5B
dispatch.

**Highest-information next experiment from the frozen result.** The dominant result
is that the favorable mechanism is *same-slot-across-rows locality*
(column-dominated, 80.19%), and that it is strongly shape-count/heterogeneity
dependent. That does **not** by itself motivate abandoning flat shapes for
hierarchy. It motivates the opposite falsifiable question, which is exactly G5B's:
**is exact flat shape identity the right abstraction, or does a single flat
"shape" over-merge records that a cheap, explicitly preregistered structural
refinement would separate — such that field-local column ordering improves further
once over-merged shapes are split?**

Falsifiable hypothesis (for a future frozen G5B prereg, not implemented here):

> H-G5B: Splitting exact flat shapes by a frozen, content-independent structural key
> (a preregistered hierarchy level) increases the column-ordering gain measured under
> G5A's exact protocol, i.e. it lowers `S3/S1` versus the G5A flat-shape A3 on the
> same frozen D1-D4 objects and the same Brotli q11/lgwin30 backend, beyond the
> frozen materiality threshold, while beating the same A0-style null.

Minimal arms (each a single frozen mechanism change; all share G5A's common
envelope, charged mode byte, invariant suite I1-I9, and null discipline):

- **B1 = G5A A3 (flat SHAPE_COLUMN)** — control/floor;
- **B2 = refined SHAPE_COLUMN** — same column emission, but shapes partitioned by one
  frozen hierarchy key (candidate keys to be preregistered, e.g. a coarse field-count
  or logical-type signature), so that members of one flat shape may be split into
  narrower groups before slot-major emission;
- **B3 = refined SHAPE_ROW** — the matching row-major arm, to keep shape-grouping vs
  column-ordering separable exactly as in G5A;
- **B0 = deterministic null** — same frozen null discipline as G5A A0.

Frozen pass/fail must reuse G5A's ratio thresholds and null test, must re-gate all
I1-I9 invariants, and must be judged only on the frozen D1-D4 discovery set with a
separate known-stress diagnostic. If B2/B3 fail to beat B1 beyond threshold (or fail
the null), the negative result closes flat-vs-hierarchy ordering and the architecture
should stop spending complexity on shape hierarchy.

**Do not** (from G5A's frozen evidence and doctrine): add more random draws to G5A;
retro-fit the D4 shape adverseness or the V1 diagnostic into a favorable story; treat
the discovery-set -6.12% as a generalization claim; allocate a production transform
ID; or expand the typed leaf basis to rescue a planner hypothesis (G4's standing
NO-GO).

---

## 6. Reproduce

1. Workflow: `.github/workflows/anvil-i10-grotli-g5a.yml`
   (prereg `docs/I10-GROTLI-G5A-ORDERING-ATTRIBUTION-PREREG.md`, r6).
2. Dispatch on head `996c2dbe67736c42288287abba7ed6f3307a0b34`; the workflow checks
   out `e6714e8...`, verifies source/prereg blobs, materializes and verifies the
   pinned frozen G3 source, builds warning-clean against it, selftests both binaries,
   fetches and verifies D1-D4 + V1, measures A0/A1/A2/A3, applies the frozen
   classification, and uploads `grotli-g5a-ordering-attribution-<run_id>`.
3. Authoritative objects in that artifact: `results/g5a-ruling.json`,
   `results/g5a-ruling.md`, `results/g5a-discovery.jsonl`, `results/g5a-v1.jsonl`,
   `results/g5arow-*.json`, `results/discovery-provenance.json`, plus the
   provenance/provenance-additive identity files.
