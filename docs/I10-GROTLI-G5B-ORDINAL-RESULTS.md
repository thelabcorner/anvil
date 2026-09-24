# ANVIL I10 - GROTLI G5B-ORDINAL Results

**Closed:** 2026-09-24
**Preregistration:** `docs/I10-GROTLI-G5B-ORDINAL-PREREG.md`, frozen revision **r3**
**Prereg git blob:** `1a45154f26ed98b7c5bc1d15bb1b5985d456347c`
**Prereg SHA-256:** `b0c4c91de7a8a46a8fdea9f03742a01da68067fabb76f73fba3af6561ce3339f`
**Frozen implementation commit:** `cf9306e34e98b35333b23757da88357cab92357e`
**Frozen G5B-ORDINAL source blob:** `ce8021a5f7f3d25319db1ccd22850c3144c03040`
**Frozen G5B-ORDINAL source SHA-256:** `910ddcf7ab8026ecca095199d921cfe823df64dd5bc9552617b9a1d73e0aa9f4`
**Frozen G5B-ORDINAL binary SHA-256:** `ea47f30c8f315db86d4e1b94c656323df53848dd574216ec10f8eb1fc776010f`
**Workflow run:** `36011333908` (workflow_dispatch, `ANVIL I10 Grotli G5B-ORDINAL probe`, conclusion `success`)
**Workflow head SHA:** `a443f37c0076591dd7efd2175859630e1e20f6c9` (= `origin/main` at run time; run `createdAt` `2026-09-24T14:14:23Z`)
**Artifact:** id `10812278595`, name `grotli-g5b-ordinal-36011333908`, size `196512` bytes,
GitHub-exposed `digest` `sha256:742dc2ee8a9b683104f6c00f93f26ed132274e41c22ae5b4113aaa4ab6e266af`, `expired: false`
**Frozen G3 reference:** public SHA `1a3d18fed76adb6fb33264e1994f9c357306b3fa`, source blob `eedc7b7e6671c4a5fcaf7a5997671bd4be30bdde`, source SHA-256 `5b3ab1cdc67d1a8d8edd7eeb4265361a66f72e5b8620137149ce802233b58a9e`
**G5A floor reference:** run `35985412906` (head `996c2dbe67736c42288287abba7ed6f3307a0b34`), artifact id `10801714249`, digest `sha256:4765b317e7c01170cffbd96b08c639e001ec1e2f317152d1f83a20a5331a3278`
**Brotli backend:** libbrotli `1.1.0-2build2` (Ubuntu 24.04), `BrotliEncoderVersion()` = `16781312` (`1.1.0`), q11 / lgwin30
**Runner:** `Linux runnervmtr4k5 6.17.0-1022-azure #22-Ubuntu SMP ... x86_64`
**Compiler:** `Ubuntu clang version 18.1.3 (1ubuntu1)`
**Decision:** **ORDINAL-ADVERSE** (discovery D1-D4)

---

## 0. Executive ruling

G5B-ORDINAL asked a single interstitial, sharply causal question:

> Does the boundary between exact lexical shapes fragment positional same-ordinal
> slot locality?

Three arms over the byte-identical frozen G5A `G5AO` common carrier differ in
exactly one mechanism - the permutation of the same canonical structured scalar
chunks:

- **B0 `ORDINAL_NULL`** (selector byte 4) - deterministic structure-destroying null
  (frozen seed `G5B-ORDINAL-NULL-SEED-v1`, one draw);
- **B1 `ORDINAL_FLOOR`** (selector byte 3) - EXACT frozen G5A A3 `SHAPE_COLUMN`
  semantics, the already-spent floor, not a treatment;
- **B2 `ORDINAL_BLOCKED`** (selector byte 5) - the single treatment: hold the
  positional ordinal `j` constant **globally across all shapes** before advancing.

Every provenance, build, selftest, corpus-identity, floor-replay, and
correctness-invariant gate passed:

- pinned implementation identity, source blob, prereg blob: **PASS**;
- warning-clean build (`-Wall -Wextra -Wpedantic -Werror`) compiled against the
  **materialized pinned frozen G3 source** (`frozen-grotli_g3.cpp`; `grotli-g5b.d`
  records the include): **PASS**;
- frozen G3 source identity (`frozen_blob == implementation_tree_blob == expected`
  = `eedc7b7e…`): **PASS**;
- adversarial selftest of `grotli_g5b_ordinal` and of the separate frozen G3
  reference (`grotli_g3`): **PASS**;
- D1-D4 + V1 corpus identity (size + git blob + SHA-256, re-verified at fetch):
  **PASS**;
- frozen G5A floor acquisition + per-file B1 reproduction + same-run G5A reference
  replay (I10): **PASS** (`floor_ok: true`, `same_run_replay: true`);
- I1-I9 invariants on every row (roundtrip, body/envelope/token-region length
  identities, envelope identity, canonical token-multiset identity, exact
  permutations, selector pack/unpack, fixed backend, B0 present and reordering):
  **PASS** (`invariants_ok: true`);
- artifact upload (`if: always()`; `grotli-g5b-ordinal-36011333908`): **PASS**.

The frozen r3 classification was applied mechanically inside the run:

> **ORDINAL-ADVERSE**: `S2 (1,403,029) > S1 (1,380,245)`. The global-ordinal
> (cross-shape) permutation is **larger** than the per-exact-shape column floor by
> **+22,784 B (+1.6507%)** in aggregate over the frozen D1-D4 discovery set.

The known-stress diagnostic is:

> **V1-ORDINAL-FAVORABLE** (B2 slightly *smaller* than B1) - KNOWN-STRESS / NOT
> HELD-OUT FOR G5B-ORDINAL, diagnostic only.

No production transform ID was allocated. No new held-out corpus was opened. Raw
Brotli remains the permanent fallback. G4 remains **NO-GO**. The result is a
discovery-set anatomy finding only (`opens_new_held_out_corpus: false`,
`production_transform_id_allocated: false`, `semantic_hierarchy_lane_open: true`).

---

## 1. What was measured (frozen arm semantics)

Exactly the three frozen arms, differing only in the permutation of the same
canonical structured scalar chunks, over a byte-identical frozen G5A `G5AO`
common carrier (magic `G5AO`, version 1, source/frame/group counts, `frame_group`,
group descriptors, raw-residual section; then exactly N `(token_len, token_bytes)`
chunks), plus a constant charged one-byte out-of-band selector:

- **B0 `ORDINAL_NULL`** - deterministic pseudo-random permutation from a frozen seed
  and the canonical chunk index; destroys positional locality while preserving
  everything else;
- **B1 `ORDINAL_FLOOR`** - `for sid (first-appearance); for slot j; for occurrence`:
  emit `canonical_index[sid][occ][j]` (EXACT frozen G5A A3 `SHAPE_COLUMN`);
- **B2 `ORDINAL_BLOCKED`** - `for j (0..D-1); for sid (first-appearance) with
  j < slots(sid); for occurrence`: emit `canonical_index[sid][occ][j]` (ordinal held
  globally constant across shapes).

`complete_bytes = 1 + brotli(common_body_with_arm_permutation)`. The selector byte
is charged in every arm's total and is not passed to Brotli, so it is equal-cost and
cannot confound the ordering comparison. All three arms had, per file, identical
`body_bytes`, identical `envelope_len`, identical `token_region_len`, identical
`envelope_sha256`, and identical `canonical_token_multiset_sha256`.

---

## 2. Machine-readable evidence (authoritative)

Ruling: `results/g5b-ruling.json` and `results/g5b-ruling.md` in the run artifact
`grotli-g5b-ordinal-36011333908`. Per-file rows: `results/g5b-discovery.jsonl`
(D1-D4) and `results/g5b-v1.jsonl` (V1). All numbers below are copied from those
CI-produced objects; no local corpus measurement was performed.

### 2.1 Aggregate (D1-D4 complete bytes)

| Arm | Complete bytes |
|---|---:|
| B0 `ORDINAL_NULL` | **2,054,910** |
| B1 `ORDINAL_FLOOR` | **1,380,245** |
| B2 `ORDINAL_BLOCKED` | **1,403,029** |

Derived deterministic byte effects (frozen section 9 semantics):

```
b2_vs_b1_bytes = S1 - S2 = 1,380,245 - 1,403,029 =  -22,784   (negative = B2 LARGER)
b2_vs_b0_bytes = S0 - S2 = 2,054,910 - 1,403,029 =  651,881
b1_vs_b0_bytes = S0 - S1 = 2,054,910 - 1,380,245 =  674,665
b2_vs_b1_ratio = S2 / S1 = 1.0165072142989107  ->  +1.6507%
```

The B1 floor is byte-identical to the archived frozen G5A A3 aggregate
(`1,380,245` = G5A A3 `1,380,245`), as required by I10.

### 2.2 Classification gate evaluation (frozen r3 precedence)

```
invariants_ok: true    floor_ok: true    same_run_replay: true
S2 (1,403,029) >  S1 (1,380,245)?          YES  (S2 > S1  -> ADVERSE, first match wins)
=> classification: ORDINAL-ADVERSE
(identity check)  B1 vs frozen G5A A3 aggregate 1,380,245:  MATCH
```

Because the first matching rule is `ORDINAL-ADVERSE if S2 > S1`, the later
(null / ratio / breadth) gates are **not reached**. For the record only:
`S2 < S0` holds (null margin `S0 - S2 = 651,881` bytes), but the frozen precedence
places ADVERSE first, so **no favorable claim is made and none is permitted**.
`b2_smaller_nondegenerate_files = 1` of 4 (see 2.3); the breadth gate would not
have been satisfied even had the aggregate favored B2.

Per section 3.1/10, the single deterministic null is **one draw, not a
significance test**; no p-value or "random is worse on average" language is used.

### 2.3 Per-file facts (D1-D4) - B2 vs B1

| File | B0 | B1 | B2 | b2_vs_b1 bytes | b2_vs_b0 bytes | b1_vs_b0 bytes | shapes | shared ord slots | cross-shape ord tokens | b2_moved | b1_eq_b2 | degenerate | B2<B1 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|:--:|:--:|:--:|
| D1 amazon cellphones | 49,870 | 39,463 | 39,463 | 0 | 10,407 | 10,407 | 1 | 0 | 0 | 0 | true | **yes** | no |
| D2 cdisc adae | 92,210 | 25,717 | 25,855 | **-138** | 66,355 | 66,493 | 2 | 55 | 55 | 65,788 | false | no | no (B2 larger) |
| D3 gharchive 10 MiB | 1,693,435 | 1,230,007 | 1,253,880 | **-23,873** | 439,555 | 463,428 | 167 | 378 | 21,362 | 259,154 | false | no | no (B2 larger) |
| D4 crovia dpi receipts | 219,395 | 85,058 | 83,831 | **+1,227** | 135,564 | 134,337 | 42 | 44 | 1,201 | 81,934 | false | no | **yes** |

- **B2 is smaller than B1 on exactly 1 of 4 files** (D4, by 1,227 B). The frozen
  breadth gate requires >= 2 non-degenerate files; it is not met.
- **D1 is DEGENERATE** (`b1_eq_b2 == true`, `b2_moved_token_count == 0`,
  `shape_count == 1`, `shared_ordinal_slots == 0`). Per frozen section 5.1, D1
  cannot count toward breadth.
- D2 and D3 are the adverse mass: B2 is larger by 138 B and 23,873 B respectively,
  and D3 carries the dominant share of the adverse aggregate. D3 is also the only
  file with a raw frame (`raw_frame_count == 1`).
- D4 is the sole favorable non-degenerate file (`b2_vs_b1_bytes = +1,227`), but its
  saving cannot offset D3's 23,873 B adverse term.

### 2.4 Liveness / non-degenerate metrics

| File | shapes | max_slots | shared_ordinal_slots | cross_shape_ordinal_tokens | b1_eq_b2 | b2_moved_token_count | token chunks |
|---|---:|---:|---:|---:|:--:|---:|---:|
| D1 | 1 | 9 | 0 | 0 | true | 0 | 7,137 |
| D2 | 2 | 284 | 55 | 55 | false | 65,788 | 65,789 |
| D3 | 167 | 533 | 378 | 21,362 | false | 259,154 | 259,196 |
| D4 | 42 | 45 | 44 | 1,201 | false | 81,934 | 82,768 |

The r3 fail-closed liveness invariants held on every discovery row:
`shared_ordinal_slots <= max_slots`; single-shape file (D1) has
`shared_ordinal_slots == 0`; and `b1_eq_b2 == (b2_moved_token_count == 0)` on all
rows. The D1 single-shape DEGENERATE case is exactly the legitimate
`shared_ordinal_slots == 0` form; the withdrawn r2 claim (that DEGENERATE implies
`shared_ordinal_slots != 0` is impossible) did not arise on D1-D4 and no invariant
rejected a valid case.

### 2.5 V1 known-stress diagnostic

V1 = Sino-US DrugQA (`all.jsonl`), 15,374,047 bytes, SHA-256
`757b9bc7e5ee2d38ab5ed43877b1d87809cb5131621d106671bc27996da1c499`. Role:
**KNOWN-STRESS / NOT HELD-OUT FOR G5B-ORDINAL**.

| Arm | Complete bytes |
|---|---:|
| B0 | 2,543,441 |
| B1 | 2,183,986 |
| B2 | 2,181,849 |

`B1->B2 = +2,137` B (**-0.0978%**), `B0->B2 = +361,592` B. V1 invariant re-gate
inside the final ruling: **clean** (all top-level identities true, all arms
roundtrip/permutation/selector-clean, envelope and multiset hashes identical,
q11/lgwin30). Permitted label **V1-ORDINAL-FAVORABLE** was emitted: on V1 the
global-ordinal permutation is slightly *smaller* than the per-exact-shape column
floor.

V1 is a diagnostic, not a held-out test. It cannot satisfy any generalization or
promotion gate, it is reported **after** re-gating correctness, it never affects the
discovery ruling, and it does not validate any G5B-ORDINAL hypothesis. In
particular, V1's favorable sign **does not** qualify or rescue the adverse D1-D4
discovery ruling.

### 2.6 Rejected / infrastructure-invalid predecessor runs (provenance honesty)

Two predecessor attempts are recorded for the immutable evidence trail; neither is
a measurement:

- **Run `36010649558`** (workflow_dispatch, head `10490d5101801f78de17f0ab81e41f926e30d3fd`,
  conclusion **`failure`**, `2026-09-24T14:08:45Z`). This run is
  **infrastructure-invalid / premeasurement**: the pinned API metadata checks
  succeeded, but `urllib` forwarded the GitHub bearer token across the artifact API
  302 to signed blob storage, which returned 401. It reached **no** D1-D4/V1
  measurement and produced no ruling. Corrected by `a443f37c`.
- **Commit `a443f37c0076591dd7efd2175859630e1e20f6c9`** ("ci: strip auth on
  artifact redirects", head of run `36011333908`) is a **transport-only
  correction**: it changes only the artifact-download auth handling
  (`add_unredirected_header` so authentication is sent to `api.github.com` but not
  propagated cross-host) in
  `.github/workflows/anvil-i10-grotli-g5b-ordinal.yml`. Frozen G5B r3
  source/prereg, corpora, selectors, thresholds, backend, and ruling semantics are
  unchanged.

Run `36011333908` is the sole source-of-record measurement for G5B-ORDINAL r3.

---

## 3. Causal interpretation (discovery D1-D4 only)

### 3.1 Raw ordinal cross-shape coarsening is falsified on the frozen D1-D4 set

Holding source bytes, parsing, frame split, structured/raw classification, exact
shapes, shape templates, `frame_group`, raw residuals, scalar token bytes, length
framing, token multiset, common envelope, body length, and backend all constant, and
changing *only* the scope over which a fixed positional ordinal is held constant:
the global-ordinal (cross-shape) permutation is **+1.6507% LARGER** in aggregate than
the frozen per-exact-shape column floor, and is smaller on only **1 of 4** families.

Within the frozen r3 contract, this is a **falsification of the raw ordinal
cross-shape coarsening proxy**: on this frozen discovery set, forcing column
emission at the *global* positional-ordinal level (across shape boundaries) does not
improve over per-exact-shape columns; it is adverse in aggregate and on the majority
of files.

### 3.2 The adverse effect is concentrated, and not a uniform law

The aggregate adverse term is dominated by D3 (23,873 B, of 22,784 B net adverse
aggregate), with a smaller adverse term on D2 (138 B) and a favorable term on D4
(1,227 B). D1 is DEGENERATE and contributes nothing. So the effect is
**file/heterogeneity-dependent**, not a uniform property. A single deterministic null
draw passed (`B2 < B0` by 651,881 B), but the frozen precedence places ADVERSE first,
and the frozen breadth gate (>= 2 non-degenerate files) is not met regardless.

### 3.3 What this establishes about the *mechanism* (and what it does not)

The favorable G5A mechanism was **same-slot-across-rows locality within an exact
shape** (`column_share = 0.8019`, column-dominated). G5B-ORDINAL tested the natural
next coarsening - "hold the ordinal constant globally, across shapes" - and it
**did not help**. This is consistent with the favorable locality being
**exact-shape-internal**, such that the crude cross-shape ordinal adjacency B2
creates (21,362 cross-shape-ordinal tokens on D3, 1,201 on D4, 55 on D2) destroys
rather than enhances Brotli's positional locality.

This does **not** establish that positional ordinals are meaningless, that slot `j`
of one shape is or is not the same logical field as slot `j` of another, that any
semantic/path alignment exists, or that flat shapes are the right abstraction. The
experiment intentionally made **no** logical-field or semantic-path claim.

### 3.4 Semantic hierarchy lane remains OPEN

Per the frozen r3 disposition (prereg section 11 "Non-pass disposition" and section
16): a non-pass (ADVERSE / NEUTRAL / UNSUPPORTED-BY-NULL / WEAK / CONCENTRATED)
**closes only the ordinal proxy**. It does **not** close the semantic-hierarchy lane.
The original **semantic** G5B lane - exact flat shapes versus an explicitly
preregistered structural/semantic hierarchy, and later precise semantic-path fusion
- remains **OPEN** (`semantic_hierarchy_lane_open: true`) and is not answered by this
result. G5B-ORDINAL may serve as motivation and architecture context for that lane
but may not change that lane's own frozen contract.

### 3.5 V1 is a known-stress diagnostic only

V1's favorable sign (B2 smaller by 2,137 B) is descriptive. V1 is not held out and
cannot promote anything; it neither contradicts nor rescues the D1-D4 ruling. Its
different sign is itself consistent with 3.2: the ordinal-cross-shape effect is
heterogeneity-dependent, and V1's anatomy (`shape_count == 3`, `max_slots == 17`,
`shared_ordinal_slots == 16`) differs sharply from D3's.

---

## 4. Discipline and provenance notes

- **No local measurement, no threshold movement.** No D1-D4/V1 or heavy benchmark
  was run on the workstation, and no codec/source/workflow/prereg threshold was
  changed by this closure. The artifact was downloaded read-only from the GitHub API
  into the in-repo gitignored `scratch/` path as a local copy of the GitHub artifact
  and is not itself treated as evidence.
- **Immutable source of record.** CI artifacts/logs of run `36011333908` are the
  only measurement evidence. The GitHub-exposed artifact `digest`
  (`sha256:742dc2ee…`) is pinned for the record; the local read-only download's
  archive-zip SHA-256 is recorded alongside it in `scratch/` for cross-check.
- **Provenance verified before/independently of outcome:** implementation commit,
  source blob, prereg blob, frozen G3 head/blob/SHA-256,
  `frozen_blob == implementation_tree_blob`, `grotli-g5b.d` showing compilation
  against `frozen-grotli_g3.cpp`, both selftests PASS, the frozen G5A floor artifact
  (id/digest/size) and its archived provenance, and the same-run G5A reference replay.
  Runtime libbrotli `.so` SHA-256s captured: `libbrotlidec.so.1.1.0` `64d8a501…`,
  `libbrotlienc.so.1.1.0` `6e59301f…`, `libbrotlicommon.so.1.1.0` `a91ead09…`.
- **Negative/unfavorable facts preserved:** the aggregate ruling is adverse; B2 is
  larger on D2 and D3, and on D3 by a wide margin; the breadth gate is unmet
  (1/4); D1 is DEGENERATE. None of this was smoothed, clipped, or reframed. V1's
  favorable sign is reported but explicitly quarantined as a non-held-out diagnostic.
- **Selector discipline preserved (I7):** frozen selector bytes B1/B0/B2 = 3/4/5 on
  the shared `G5AO` carrier; selector 0/1/2 remain invalid (G5A A0/A1/A2 aliases);
  pack/unpack round-trip clean; selector never passed to Brotli.

---

## 5. Disposition and next scientific decision

**Disposition:** G5B-ORDINAL is **CLOSED** as a discovery-set anatomy result
(`ORDINAL-ADVERSE`). No production transform ID, no promotion, no production source
change, no new held-out corpus. Raw Brotli remains the permanent fallback. G4
remains **NO-GO**. The `ORDINAL-*` proxy is closed; the semantic-hierarchy lane
remains **OPEN**.

**What is now closed:** raw ordinal cross-shape coarsening - lifting the
positional-ordinal crossing from per-exact-shape (B1) to global-across-shapes (B2) -
is falsified as a favorable mechanism on the frozen D1-D4 discovery set. Any future
proposal to spend complexity on ordinal cross-shape coarsening must treat this as a
closed negative result.

**What remains open:** the original semantic G5B lane (exact flat shapes versus an
explicitly preregistered structural/semantic hierarchy, and later precise
semantic-path fusion). This result **does not** motivate abandoning flat shapes for
hierarchy, nor does it motivate the reverse; it removes one cheap surrogate and
leaves the semantic-hierarchy question exactly where the frozen G5A results left it.

**Highest-information next experiment (for a future frozen prereg, not implemented
here):** a hypothesis for the semantic-hierarchy lane must separate *positional*
ordinal structure from *semantic* structure explicitly - e.g. whether a frozen,
content-independent, explicitly preregistered structural/semantic refinement of flat
shapes improves field-local column ordering where the crude positional ordinal
proxy does not - while reusing the frozen G5A thresholds, the same frozen D1-D4
discovery objects, the same Brotli q11/lgwin30 backend, the same null discipline,
and the same fail-closed invariant suite. Any such experiment must be its own frozen
prereg; it may not retroactively change G5A or G5B-ORDINAL arms or gates.

**Do not:** add more random draws to G5B-ORDINAL; retune thresholds; retro-fit
V1-ORDINAL-FAVORABLE into a favorable discovery story; treat the adverse discovery
result as a generalization claim; allocate a production transform ID; or expand the
typed leaf basis to rescue a planner hypothesis (G4's standing NO-GO).

---

## 6. Reproduce

1. Workflow: `.github/workflows/anvil-i10-grotli-g5b-ordinal.yml`
   (prereg `docs/I10-GROTLI-G5B-ORDINAL-PREREG.md`, r3).
2. Dispatch on head `a443f37c0076591dd7efd2175859630e1e20f6c9` (run
   `36011333908`); the workflow checks out `cf9306e…`, verifies source/prereg blobs,
   materializes and verifies the pinned frozen G3 source, builds warning-clean
   against it, selftests both binaries, acquires and verifies the frozen G5A floor
   evidence (run `35985412906`, artifact `10801714249`, pinned digest), runs the
   same-run frozen G5A reference replay on D1-D4, fetches and verifies D1-D4 + V1,
   measures B0/B1/B2, verifies B1/FLOOR exact reproduction, applies the frozen
   section-10/10.1 classification mechanically (fail-closed, I12), and uploads
   `grotli-g5b-ordinal-<run_id>` with `if: always()`.
3. Authoritative objects in that artifact: `results/g5b-ruling.json`,
   `results/g5b-ruling.md`, `results/g5b-discovery.jsonl`, `results/g5b-v1.jsonl`,
   `results/g5brow-*.json`, `results/floor-check.json`,
   `results/discovery-provenance.json`, `results/g5a-reference-identity.txt`,
   `results/g5a-replay-discovery.jsonl`, plus the provenance/identity files
   (`grotli-g5b-implementation-identity.txt`, `grotli-g3-source.sha256`,
   `brotli-package-identity.txt`, `brotli-library-sha256.txt`).
4. Predecessor runs for the record: `36010649558` (infrastructure-invalid /
   premeasurement, `failure`) and the transport-only correction commit
   `a443f37c0076591dd7efd2175859630e1e20f6c9`.
