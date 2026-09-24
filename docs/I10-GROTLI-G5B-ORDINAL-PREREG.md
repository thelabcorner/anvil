# ANVIL I10 - GROTLI G5B-ORDINAL Preregistration

**Status:** FROZEN BEFORE ANY G5B-ORDINAL D1-D4 / V1 CORPUS MEASUREMENT
**Revision:** r3 (second pre-outcome audit correction; supersedes the unrun r2 text)
**Date:** 2026-09-24
**Parent evidence:** G5A `ORDER-MATERIAL / COLUMN-DOMINANT` (run `35985412906`),
G4 `NO-GO-G4`, G3 `PASS-G3-NARROW`
**Purpose:** sharply causal interstitial probe - does the boundary between exact
lexical shapes fragment positional same-ordinal slot locality?
**Production authorization:** none

> **Pre-outcome correction (r1 -> r2).** The initial, unrun G5B-ORDINAL
> implementation commit (`661b534`) carried a **carrier-magic contradiction**: it
> claimed an "EXACT frozen G5A common carrier body" while using a distinct `G5BO`
> magic, which makes the mandatory B1/FLOOR reproduction of frozen G5A A3 byte-for-byte
> impossible (the envelope bytes, and therefore the envelope SHA-256 and complete-byte
> totals, cannot match). This was found by audit **before any G5B-ORDINAL D1-D4 or V1
> measurement was run** and is corrected in this revision (r2). No threshold, arm,
> corpus, or treatment outcome has been observed: **no D1-D4/V1 G5B-ORDINAL
> measurement has been run locally, in CI, or by any other means**, and no G5B-ORDINAL
> ruling exists. r1 was never dispatched. The original **semantic** G5B lane remains
> **OPEN**.

> **Pre-outcome correction (r2 -> r3).** A second read-only audit, again **before
> any G5B-ORDINAL D1-D4 or V1 measurement**, found and corrected four defects in
> the r2 text and its workflow/source: (1) the I10 floor gate read
> `carrier_quality`/`carrier_window` from the arm objects, but both are TOP-LEVEL
> row fields in frozen G5A and G5B rows, so the gate could never pass; (2) I11
> claimed `b1_eq_b2` implies `shared_ordinal_slots == 0`, which is FALSE (two
> distinct one-slot shapes are a legitimate multi-shape DEGENERATE case with
> `shared_ordinal_slots == 1`); (3) a V1 invariant failure preserved the discovery
> classification as the source-of-record ruling instead of routing it to
> INVALID-G5B-ORDINAL (I12); (4) the G5A prereg blob / Brotli encoder version
> pins and the G5A run head SHA were recorded but never asserted. No arm,
> corpus, threshold, null seed, backend parameter, representation, or
> classification threshold/precedence is changed. r2 was never dispatched and no
> G5B-ORDINAL measurement or ruling exists.

> **No D1-D4 or V1 outcome was observed before this freeze.** This preregistration
> and the frozen source and workflow it pins were written using only source
> inspection, source review, and correctness-only local compilation plus tiny
> synthetic selftests. No D1-D4 or V1 corpus measurement was run locally, remotely,
> or by any other means before this document was frozen. The one exception is the
> already-spent G5A floor result (a published, independent prior run), which is used
> ONLY as a mandatory reproduction floor and never as a treatment.

---

## 0. Why G5B-ORDINAL exists

G5A run `35985412906` established, on the frozen D1-D4 discovery set, that byte
ordering/locality of identical exact lexical scalar chunks is materially causal
(-6.1189% aggregate, 4/4 files, deterministic null passed by 674,287 bytes) and
that the effect is **column-dominated** (`column_share = 0.8019`): the dominant
favorable mechanism is emitting the **same slot across many rows** of an exact
shape after shapes are grouped.

The original **semantic** G5B lane - exact flat shapes versus an explicitly
preregistered structural/semantic hierarchy, and later precise semantic-path
fusion - remains **OPEN** and is not what this experiment is. G5B-ORDINAL is an
interstitial, sharply causal probe that asks only:

> **Does the boundary between exact lexical shapes fragment positional
> same-ordinal slot locality?**

"Ordinal" here means a **positional slot index within an exact lexical shape**
(slot 0, slot 1, ...). G5B-ORDINAL intentionally makes **no** logical-field or
semantic-path claim: it does not assert that slot j of shape A and slot j of
shape B are the same logical field, nor that any field/path alignment exists. It
tests only whether the *exact lexically-derived shape identity* is too fine a
grouping key for positional column locality - i.e. whether forcing column
emission at the **global ordinal** level (across shapes), rather than
per-exact-shape, changes compressed size.

This is a **3-arm** experiment (B0/B1/B2). Exactly **one** treatment mechanism
changes (the scope over which a fixed ordinal is held constant). B3 is
deliberately **omitted** from this first freeze so that only one treatment
mechanism differs.

---

## 1. Non-negotiable causal constraint

The G5B-ORDINAL structured arms must differ in exactly one mechanism:

> **the permutation of the same structured scalar chunks.**

For every input, all three arms must have:

- identical parser and frame split (frozen G3);
- identical structured/raw frame classification;
- identical exact-shape identity;
- identical shape templates (template parts);
- identical frame-to-group reconstruction map (`frame_group`);
- identical raw residual bytes in identical order;
- identical structured scalar token bytes;
- identical scalar token length framing;
- identical structured scalar chunk multiset;
- identical canonical token-multiset SHA-256 (machine-gated, section 6 I3);
- identical common-envelope SHA-256 (machine-gated, section 6 I4);
- identical body length, envelope length, and token-region length;
- an envelope byte-identical to frozen G5A A3 (magic `G5AO`, version 1, same prefix
  grammar), so the B1/FLOOR carrier body reproduces the archived G5A A3 body;
- identical Brotli implementation and parameters (quality 11, lgwin 30);
- no dictionary, integer, float, RLE, FSST, predictor, or other leaf transform;
- no new metadata and no representation-family change.

No arm may omit, canonicalize, normalize, deduplicate, entropy-code, or otherwise
change a scalar token.

If carrier-body sizes differ across arms, G5B-ORDINAL has an implementation bug
and produces no scientific ruling.

The carrier body is the **exact frozen G5A carrier grammar** (magic `G5AO`,
version, source/frame/group counts, `frame_group`, group descriptors, raw-residual
section, then exactly N `(token_len, token_bytes)` chunks). G5B-ORDINAL adds **no**
new field to that grammar. Only the permutation of the identical token chunks
changes between arms.

**Byte-compatibility requirement (r2 correction).** The carrier magic is the frozen
G5A magic `G5AO` and the version is `1`, exactly as in the frozen G5A carrier
(section 2.2 below). A distinct `G5BO` magic is **forbidden**: because the envelope
(`carrier_body[0 .. prefix_len)`) is hashed and because B1/FLOOR is required (I10) to
reproduce frozen G5A A3 byte-for-byte, a distinct magic would make that reproduction
impossible by construction. Only the arm selector changes, and it is out-of-band
(section 2.3). Consequence, pinned as an open item: the G5B-ORDINAL carrier body is
shared with frozen G5A, so `G5AO` + selector 3 is exactly a frozen G5A A3 record.

---

## 2. Frozen parsing, shapes, and carrier grammar (inherited from G5A)

G5B-ORDINAL imports the exact frozen G3 parsing / frame-split / shape semantics,
via exactly the frozen G5A mechanism:

- Frozen G3 public SHA: `1a3d18fed76adb6fb33264e1994f9c357306b3fa`
- Frozen G3 source blob: `eedc7b7e6671c4a5fcaf7a5997671bd4be30bdde`
- Frozen G3 source SHA-256: `5b3ab1cdc67d1a8d8edd7eeb4265361a66f72e5b8620137149ce802233b58a9e`

### 2.1 Frozen-inclusion requirement

G5B-ORDINAL must compile in CI against a **materialized pinned frozen G3 source**,
not the mutable working-tree `tools/grotli_g3.cpp`. The CI workflow must, before
building G5B-ORDINAL:

1. materialize `frozen-grotli_g3.cpp` from `$FROZEN_G3_SHA` and verify its git blob;
2. build with `-DG5B_FROZEN_G3_HEADER=\"frozen-grotli_g3.cpp\"` so the source
   `#include`s that exact materialized file;
3. prove via the generated dependency file that the materialized file was included;
4. assert the materialized blob equals the blob of the G3 source the G5B source
   would otherwise include (so the two cannot silently diverge).

Local builds with no define fall back to the working-tree `tools/grotli_g3.cpp`.

Frozen semantics include:

- LF/CRLF/final-remainder frame splitting;
- per-frame exact JSON lexical parsing;
- malformed/incomplete frame -> raw residual;
- byte-identical inter-value template parts define exact shape identity;
- structured shapes assigned in first-appearance order;
- frame reconstruction order explicitly represented.

G5B-ORDINAL does **not** change parser eligibility or shape formation.

### 2.2 Frozen common carrier body (exact grammar)

Conceptually (identical to frozen G5A; the `coordinate label map` in section 4 is
experimental bookkeeping and is NOT serialized):

```
magic = "G5AO"          # value 0x4735_414F ; 4 bytes G5AO (EXACT frozen G5A magic)
version                 # 1 byte, value 1
source_len              # uvar
frame_count             # uvar
group_count             # uvar
frame_group[frame_count]# uvar each

group_descriptors[group_count]:
    group_kind          # 1 byte: 0 = structured, 1 = raw
    member_count        # uvar
    structured:
        slot_count      # uvar
        template_part_count   # uvar (= slot_count + 1)
        (part_len, part_bytes) ...

raw_residual_section:
    for raw members in frozen source order:
        raw_len         # uvar
        raw_bytes

structured_token_stream:
    exactly N chunks, each:
        token_len       # uvar
        exact_token_bytes
```

Envelope = `carrier_body[0 .. prefix_len)` = magic through the raw-residual
section inclusive. The metadata and raw-residual section are byte-identical across
all three arms; only the order of the identical token chunks in the
structured_token_stream changes.

### 2.3 Decoder-visible order mode (one charged byte, outside Brotli)

The decoder must know which permutation was used. That information is not free.

- the order mode is a **one-byte outer field**;
- it is charged in every arm's complete-byte total;
- it is **not** included in the bytes passed to Brotli;
- the compressed Brotli payload contains only the common carrier body.

Thus:

```
complete_bytes = 1 + brotli(common_body_with_arm_permutation).size()
```

The one charged byte is equal-cost across all three arms. Brotli sees no arm tag.
The charged byte is out-of-band; G5B-ORDINAL makes no wire-format claim and
allocates no production transform ID. The source MUST materialize the mode byte in
a tiny deterministic pack/unpack round-trip selftest (section 6 I7).

**Frozen selector values (r2 correction).** Because the carrier body now shares the
frozen G5A `G5AO` envelope, the selector values are chosen to avoid semantic aliasing
between the G5B-ORDINAL arms and the G5A A0/A1/A2/A3 modes:

- `B1 ORDINAL_FLOOR` = **3**, exactly the frozen G5A A3 `SHAPE_COLUMN` selector, so
  that `G5AO` + selector 3 is a byte-identical reproduction of a frozen G5A A3 record;
- `B0 ORDINAL_NULL` = **4** (new; disjoint from G5A's 0..3 selector space);
- `B2 ORDINAL_BLOCKED` = **5** (new; disjoint from G5A's 0..3 selector space).

Selector bytes 0, 1, and 2 are deliberately **unused and invalid** in G5B-ORDINAL: on
a shared `G5AO` carrier they would alias G5A A0/A1/A2 and could be misread as a G5A
record. Every arm still pays exactly one selector byte, the selector value is never
passed to Brotli, and the selector is `uint8`.

---

## 3. Frozen arms (exactly three)

There are exactly **three** B-arms. There is no B3 in this freeze. Each arm has a
frozen out-of-band selector byte (section 2.3): B0 = 4, B1 = 3, B2 = 5.

### B0 - ORDINAL_NULL (deterministic null)

A deterministic pseudo-random permutation of the same canonical chunk-index list:

- **selector byte:** `4` (new; disjoint from G5A's 0..3 selector space);
- derived solely from a fixed frozen seed and the canonical chunk index; no score,
  no input content, and no observed byte ever influences it;
- concrete rule: sort canonical indices by
  `SHA-256(canonical_bytes(seed_string) || 0x1F || index_le_u64)` compared as an
  unsigned big-endian 256-bit integer, ties by lower index;
- **frozen seed string literal:** `G5B-ORDINAL-NULL-SEED-v1` (a new literal,
  distinct from G5A's `G5A-RANDOM-PERMUTATION-SEED-v1`);
- one draw, never a p-test (section 3.1);
- same envelope, same chunk multiset, same body length, same backend as B1/B2.

B0's scientific role: destroy positional locality while preserving everything
else, so B2's effect can be separated from "Brotli happens to like some
permutation".

### B1 - ORDINAL_FLOOR (EXACT frozen G5A A3 SHAPE_COLUMN)

This is the **already-spent floor**, not a treatment. It must be the **exact**
frozen G5A A3 semantics, including the A3 selector byte:

- **selector byte:** `3` (EXACTLY the frozen G5A A3 `SHAPE_COLUMN` selector);
- for sid in frozen first-appearance shape order;
- for slot j increasing (`0 .. slots(sid)-1`);
- for occurrence increasing (`0 .. members(sid)-1`);
- emit `canonical_id[sid][occ][j]`.

B1 is byte-for-byte the same permutation G5A A3 used, **and** its carrier envelope is
the exact frozen G5A `G5AO` envelope (r2 correction), so that (section 6 I10) its
D1-D4 complete bytes must exactly reproduce the archived authoritative G5A run. With
selector 3 and the shared `G5AO` carrier, a B1 record is byte-identical to a frozen
G5A A3 record.

### B2 - ORDINAL_BLOCKED (the single treatment)

Let `D = max over shapes of slots(shape)` (the maximum structured slot count).

- **selector byte:** `5` (new; disjoint from G5A's 0..3 selector space);
- for j = 0 .. D-1:
  - for sid in frozen first-appearance shape order, with `j < slots(sid)`:
    - for occurrence increasing (`0 .. members(sid)-1`):
      - emit `canonical_id[sid][occ][j]`.

That is: hold the **positional ordinal j constant globally** across all shapes
(only shapes that actually have slot j participate at that ordinal), before
advancing to j+1. Every canonical token index is emitted exactly once. Like
B0/B1, B2 adds no data and changes only the permutation of the identical chunks.

B2's scientific question: if the exact lexical shape boundary **fragments**
positional same-ordinal locality, then lifting the ordinal-crossing to the global
level (B2) should beat per-exact-shape columns (B1). If instead locality is
genuinely exact-shape-internal, B2 should be neutral or adverse.

### 3.1 B0 is a single deterministic null draw, not a p-value

Brotli's output is order-sensitive and non-monotone, so ONE random permutation is a
single point in the null distribution, not a test statistic. Therefore:

- B0 is frozen as exactly one deterministic draw (one seed);
- G5B-ORDINAL must **not** use p-value, significance, or "random is worse on
  average" language from a single draw;
- B2's locality claim is only supported when B2 also beats B0 on the frozen
  comparison (section 10); otherwise the claim is unsupported even if B2 beats B1;
- adding more random draws is a **separate** future lane and may not be added after
  seeing D1-D4.

There are no other G5B-ORDINAL arms.

---

## 4. Frozen coordinate label map (bookkeeping only; citable)

The canonical token identity list is built in SOURCE order exactly as in frozen
G5A: walk structured records in source-frame order, and for each structured record
emit its slots `0 .. K-1`. Each canonical index i therefore has a stable
coordinate label `(shape, occurrence, slot)`.

- `canonical_index[sid][occ][j]` = the canonical index for shape `sid`, occurrence
  `occ`, slot `j`, exactly as built in SOURCE order.
- B1 permutation: for sid (first-appearance); for j; for occ: emit
  `canonical_index[sid][occ][j]`.
- B2 permutation: for j (0..D-1); for sid (first-appearance) with j < slots(sid);
  for occ: emit `canonical_index[sid][occ][j]`.

This coordinate map is **only** experimental bookkeeping used to construct B0/B1/B2
and to compute the liveness metrics of section 5. It is **not serialized**; the
carrier body is unchanged from frozen G5A (no new metadata). The decoder needs only
the arm mode byte plus the common carrier grammar.

---

## 5. Frozen liveness / degeneracy metrics

For each file the implementation MUST emit and CI MUST record these frozen
liveness metrics (all computed from the frozen coordinate structure; no extra
bytes):

- `shape_count` - number of distinct exact shapes;
- `max_slots` - `D = max over shapes of slots(shape)`;
- `shared_ordinal_slots` - number of ordinals `j` such that at least two distinct
  shapes both have `j < slots(shape)` (i.e. ordinals that B2 actually shares across
  shapes);
- `cross_shape_ordinal_tokens` - number of tokens emitted by B2 whose
  immediately-preceding token in B2 belongs to a **different** shape at the same
  ordinal `j` (the crude count of the cross-shape-at-same-ordinal adjacency B2
  creates);
- `b1_eq_b2` - true iff the B1 permutation equals the B2 permutation on this file;
- `b2_moved_token_count` - number of canonical indices whose position in B2 differs
  from B1.

### 5.1 Degeneracy (frozen)

If `b1_eq_b2` is **true**, the file is **DEGENERATE**: the treatment changes
nothing on that file, so that file **cannot count toward breadth** in any breadth
requirement. `b1_eq_b2` is expected to be true on single-shape files
(`shared_ordinal_slots == 0`), where both B1 and B2 equal the identity of the
per-shape column order. It is also true on a **multi-shape** file when every
shape has exactly one slot: such a file shares ordinal 0 across shapes
(`shared_ordinal_slots == 1 > 0`), yet B1 (shape-major) and B2 (ordinal-major)
emit the same order. **DEGENERATE therefore never implies
`shared_ordinal_slots == 0`** (r3 correction), and no invariant may reject that
legitimate case.

A `MATERIAL` ruling is **impossible** unless B2 is smaller than B1 on **at least
two NON-DEGENERATE D1-D4 files**.

---

## 6. Hard implementation invariants

Every measured file must satisfy all of the following before any size result is
used. I1-I9 mirror frozen G5A; I10-I12 are G5B-ORDINAL-specific.

### I1 - exact source roundtrip every arm

Each B0/B1/B2 body plus its charged order mode must decode byte-for-byte to the
original source.

### I2 - body / envelope / token-region length identity

```
len(B0_body) == len(B1_body) == len(B2_body)
envelope_len(B0) == envelope_len(B1) == envelope_len(B2)
token_region_len(B0) == token_region_len(B1) == token_region_len(B2)
```

where each arm body = common envelope (`prefix`) immediately followed by that arm's
token-permutation region.

### I3 - exact permutation coverage AND canonical token-multiset identity

Each arm's token stream must be a permutation containing every canonical chunk
index exactly once. No duplicate, omission, or synthetic chunk is allowed. Index
bijection alone is tautological and is NOT sufficient. Additionally, for every arm
the implementation MUST compute and emit a byte-level **canonical token-multiset
SHA-256**:

```
record_i = uvar(len(token_bytes_i)) || token_bytes_i     (decode-visible framing)
multiset_sha256 = SHA-256( sort_lexicographically_ascending(record_1, ..., record_N)
                           joined with no separator )
```

Identical across B0/B1/B2 for every measured file. Any difference is INVALID.

### I4 - envelope identity (byte-level) and body layout

The carrier-body prefix must be byte-identical across B0/B1/B2, and the
raw-residual section must be byte-identical. Machine-gated by a byte-level
**envelope SHA-256**:

```
envelope_bytes = carrier_body[0 .. prefix_len)
envelope_sha256 = SHA-256(envelope_bytes)
```

Identical across B0/B1/B2 for every measured file, and equal to the SHA-256 of the
stored plan prefix. Additionally the implementation must prove each arm's body
equals the common envelope followed immediately by that arm's token-permutation
region (no bytes inserted, removed, or reordered outside the token region).

### I5 - fixed backend and build/provenance identity

All arms use the same Brotli quality/window: quality 11, lgwin 30. All arms must
use the same Brotli implementation **build**, not merely the same parameters.

### I6 - no typed leaf / new representation family

No G2/G3 typed leaf encoder is allowed in B0/B1/B2. The exact lexical scalar bytes
are the payload. No new representation family and no new metadata are introduced.

### I7 - exactly one charged selector byte, pack/unpack roundtrip, fail closed

The charged one-byte mode field is included in every reported complete-byte number
and is materialized in a pack/unpack selftest. The selector byte must be validated
against exactly the three frozen `G5BOrdinal` values `{3,4,5}` (B1=3, B0=4, B2=5;
section 2.3) before it is accepted; any other selector byte must be rejected
deterministically (fail closed). In particular selector bytes 0, 1, and 2 are
INVALID in G5B-ORDINAL because on the shared `G5AO` carrier they alias G5A A0/A1/A2.
The selftest must additionally assert that the B1/FLOOR selector is exactly 3 and
that the B0/B2 selectors do not alias G5A's 0..3 selector space.

### I8 - compiler / package / libbrotli provenance

Each result row MUST record `BrotliEncoderVersion()` (integer and dotted form),
quality 11 and lgwin 30, and the CI MUST record the compiler identity, the Brotli
package identity, and the SHA-256 of the actually linked libbrotli shared libraries.

### I9 - deterministic null present and actually reorders

B0 must be present and satisfy I1-I8. B0 must actually reorder on nontrivial
fixtures: the implementation selftest must prove the B0 permutation differs from
the identity on a multi-chunk fixture. A measurement that omits B0, or whose B0
does not satisfy I1-I8, is INVALID-G5B-ORDINAL.

### I10 - B1/FLOOR exact reproduction of frozen G5A A3 (CI-mandatory)

B1 must be the exact frozen G5A A3 SHAPE_COLUMN permutation and the exact frozen G5A
`G5AO` carrier envelope (r2 correction). CI MUST verify, per D1-D4 file, B1 against
the authoritative archived G5A run `35985412906`, and MUST fail closed as
**INVALID-B1-FLOOR** on any required comparable fact that differs. The comparison is
**per file** and uses the *authoritative archived* A3/per-file facts (not a re-derived
or locally-inferred baseline). The strengthened minimum fact set is:

- **complete bytes**: `B1_complete == G5A A3 complete_bytes`;
- **envelope facts**: `B1 envelope_sha256 == G5A A3 envelope_sha256` and
  `B1 envelope_len == G5A A3 envelope_len`;
- **body length**: `B1 body_bytes == G5A A3 body_bytes`;
- **token-region length**: `B1 token_region_len == G5A A3 token_region_len`;
- **canonical multiset**: `B1 canonical_token_multiset_sha256 == G5A A3`
  `canonical_token_multiset_sha256`;
- **carrier parameters**: `carrier_quality == 11` and `carrier_window == 30` as
  **TOP-LEVEL row fields** (r3 correction) on the current B1 row, on the archived
  G5A top-level row, and on the same-run replay top-level row; the arm objects
  carry no carrier fields and must not be read for them;
- **archived structural counts where stable/useful**: `shape_count`,
  `token_chunk_count`, `frame_count`, `structured_frame_count`, `raw_frame_count`,
  and `structured_token_bytes` must equal the archived G5A row when the archived row
  provides them.

The multiset identity and the body/token-region **length** facts are supporting
invariants only and are explicitly **NOT sufficient proof** by themselves: two
different carrier bodies can share a multiset and a length, so byte-level envelope
identity, complete-byte equality, and the archived provenance comparison below are
also required.

**Backend / build provenance comparability (mandatory).** B1's recorded backend
provenance must match the archived G5A provenance where the archive provides it:

- `BrotliEncoderVersion()` integer **and** dotted form;
- the `libbrotli` dpkg package version (`libbrotli-dev` and `libbrotli1`);
- the SHA-256 of the actually linked libbrotli shared libraries
  (`libbrotlienc.so.1.1.0`, `libbrotlidec.so.1.1.0`, `libbrotlicommon.so.1.1.0`).

If any required comparable provenance fact differs (for example a different
`BrotliEncoderVersion`, dpkg version, or linked-library SHA-256), the run MUST emit
**INVALID-B1-FLOOR before any treatment interpretation** — the archived facts are not
comparable. A compiler-binary hash is recorded for the record but is **not** itself a
comparability requirement, because it does not change the Brotli backend; the linked
Brotli identity and encoder version ARE required.

**Same-run control replay (mandatory).** In addition to reading the archived facts,
CI MUST run a same-run **frozen G5A reference replay** (section 7.3): it materializes
and compiles the pinned frozen G5A implementation identity and the frozen G3 source,
runs on D1-D4 **only after** corpus identity verification, and compares its A3
per-file facts to the archived run and to B1. This makes environment drift (e.g. a
newer libbrotli on a newer runner image) **diagnosable** rather than silently
mis-scored: if the replayed A3 facts do not reproduce the archive, the environment
itself has drifted and the run is **INVALID-B1-FLOOR**. The replay is a control / gate,
not a new experiment, and it does not change any arm.

If the archived G5A floor artifact cannot be obtained/decoded, or if a required
archived fact is missing or not comparable, or if the same-run replay does not
reproduce the archived A3 facts, the run must emit **INVALID-B1-FLOOR** and must
prohibit B2 interpretation. A floor that fails on actual bytes is
**INVALID-B1-FLOOR** (a correctness problem), distinct from a generic
INVALID-G5B-ORDINAL.

### I11 - liveness / degeneracy

CI MUST emit `shape_count`, `max_slots`, `shared_ordinal_slots`,
`cross_shape_ordinal_tokens`, `b1_eq_b2`, `b2_moved_token_count` (section 5). If
`b1_eq_b2`, the file is DEGENERATE and cannot count toward breadth. A `MATERIAL`
ruling is impossible unless B2 is smaller on **>= 2 NON-DEGENERATE D1-D4 files**.
The definitions of `shared_ordinal_slots` and `cross_shape_ordinal_tokens` are the
precise ones in section 5, and the implementation must **fail closed on the valid
invariants** (r3 correction):

- `shared_ordinal_slots <= max_slots` (each ordinal counted at most once);
- a single-shape file (`shape_count < 2`) must have `shared_ordinal_slots == 0`;
- `b1_eq_b2 == (b2_moved_token_count == 0)` (the two must agree; a contradiction is
  an implementation bug and must never fall through to a favorable classification).

The r2 bullet "a DEGENERATE file (`b1_eq_b2`) cannot have
`shared_ordinal_slots != 0`" was **false** and is **withdrawn** (r3 correction):
two distinct one-slot shapes are a legitimate multi-shape DEGENERATE case with
`shared_ordinal_slots == 1 > 0`, `b1_eq_b2 == true`, and
`b2_moved_token_count == 0` (section 5.1). The implementation selftest MUST prove
that case is valid rather than rejected.

### I12 - no failure falls through to a favorable classification

Any floor, provenance, invariant, or liveness failure MUST be routed to
**INVALID-B1-FLOOR** or **INVALID-G5B-ORDINAL**, and artifact upload MUST use
`if: always()` so invalid evidence is preserved. `V1` remains a known-stress
diagnostic only and is separately re-gated (section 10.1); a `V1` invariant failure
fails closed and emits no `V1-ORDINAL-*` label. In addition (r3 correction), a
`V1` invariant/liveness failure MUST route the **source-of-record classification**
to **INVALID-G5B-ORDINAL** (not merely annotate a `V1-INVALID-G5B-ORDINAL`
sub-label): a potentially favorable discovery classification (`ORDINAL-*`) must
never remain the ruling or the workflow output. Valid discovery anatomy facts
(aggregates, per-file counts) may be preserved only in clearly-labeled
**diagnostic** fields, and `b2_interpretation_prohibited` MUST be set. A failure
in any gate may never be allowed to fall through to a favorable classification.

### Fail-closed additions

The implementation may add any necessary fail-closed invariants discovered during
implementation, but must not weaken, relax, or retune I1-I12, the arms, the
thresholds, the corpus, or the classification precedence.

Any I1-I12 failure -> **INVALID-G5B-ORDINAL** (or **INVALID-B1-FLOOR** for an I10
floor / provenance / replay failure), fix correctness, rerun from a newly frozen
implementation. No size interpretation is permitted.

---

## 7. Frozen corpus roles

### 7.1 Discovery: D1-D4 (reused, unchanged)

| ID | Object | Bytes | SHA-256 |
|---|---|---:|---|
| D1 | Amazon cellphone NDJSON | 277,673 | `c1518fdaaed45e590c480ed707aa1adaaba8b84b10747f956bd431c708bd590e` |
| D2 | CDISC ADaM adverse-event NDJSON | 615,350 | `b795a59c5c92a8fc93d7d3f729fa0dae014e9b0f684a6a7a7bcf07e4104ba723` |
| D3 | GH Archive 10 MiB NDJSON | 10,485,760 | `a860af236f794779b5471b416b2e6d7ea68de00f6d17728dd3b088a7ecec8252` |
| D4 | CROVIA royalty receipts NDJSON | 3,585,053 | `0db7ace9e46ce458a7055f48b09aabc7df15f4f74a23553d3660b7de61d0916f` |

These are no longer unseen. G5B-ORDINAL makes no generalization claim from them.
Exact source URLs, byte counts, git blobs, and SHA-256 are pinned in the CI
workflow, identical to frozen G5A.

### 7.2 Known-stress / anatomy object: V1

Sino-US DrugQA V1 is known.

- bytes: **15,374,047**
- SHA-256: `757b9bc7e5ee2d38ab5ed43877b1d87809cb5131621d106671bc27996da1c499`

Label: **KNOWN-STRESS / NOT HELD-OUT FOR G5B-ORDINAL**. Its result cannot satisfy
any generalization or promotion gate. No new held-out corpus is opened in
G5B-ORDINAL. V1 correctness is re-gated inside the final ruling; on any V1
invariant failure the run fails closed, emits no `V1-ORDINAL-*` label, and routes
the source-of-record classification to INVALID-G5B-ORDINAL (I12).

### 7.3 Frozen G5A floor artifact (mandatory reproduction reference)

The authoritative G5A evidence is the published CI run `35985412906`, artifact
`grotli-g5a-ordering-attribution-35985412906`.

**Artifact metadata (determined read-only at correction time, via the GitHub REST
API on 2026-09-24).** The GitHub API **does** expose a stable artifact `digest`, so it
is pinned:

- run id: `35985412906`; run conclusion: `success` (= `head_sha`
  `996c2dbe67736c42288287abba7ed6f3307a0b34`; CI MUST assert this head SHA as a
  checked identity pin, not merely record it);
- artifact id: `10801714249`;
- artifact name: `grotli-g5a-ordering-attribution-35985412906`;
- artifact size: `15631` bytes;
- artifact `digest` (GitHub-exposed):
  `sha256:4765b317e7c01170cffbd96b08c639e001ec1e2f317152d1f83a20a5331a3278`;
- artifact `expired`: `false`, `expires_at`: `2026-10-24T10:11:27Z` (retention 30
  days from `2026-09-24T10:11:28Z`);
- artifact created: `2026-09-24T10:11:28Z`.

The independently recomputed SHA-256 of the downloaded archive zip was
`4765b317e7c01170cffbd96b08c639e001ec1e2f317152d1f83a20a5331a3278`, which equals
the GitHub-exposed `digest`. Both the archive-zip digest and the API `digest` are
pinned; extraction MUST require the **exact** expected member paths (see below), and
the run MUST fail closed if the artifact is expired or the digest differs.

CI obtains the artifact read-only via the GitHub API and verifies it against the
frozen expected identities in the workflow (workflow run id, run head SHA, artifact
name, artifact id, artifact size, artifact SHA-256/digest, frozen implementation
SHA, G5A source blob, G5A prereg blob, frozen G3 identity, BrotliEncoderVersion,
and the archived per-file A3/envelope/multiset/body facts). These are fail-closed
identity assertions, not recorded metadata.
**Extraction requires exact expected member paths** (e.g.
`results/g5a-discovery.jsonl`, `results/g5a-ruling.json`), **not** suffix or
shallowest-path matching, so a decoy member cannot satisfy a required path. If the
archive contains provenance files (e.g. `brotli-package-identity.txt`,
`brotli-library-sha256.txt`, `grotli-g5a-implementation-identity.txt`,
`grotli-g3-source.blob`), their **digests/contents are verified**, and the archived
G5A backend provenance (BrotliEncoderVersion integer+dotted, dpkg package version,
linked libbrotli SHA-256) is compared against the G5B run's provenance (I10).

In addition, CI MUST run a **same-run frozen G5A reference replay** on CI D1-D4 only
(after corpus identity verification): it materializes and compiles the pinned frozen
G5A implementation identity and the frozen G3 source, runs ONLY the D1-D4 discovery
arms there, and compares that replay's A3 per-file facts to the archive and to G5B
B1. This control gate exists to make environment drift (e.g. a newer runner image or
libbrotli) diagnosable: if the replayed A3 facts differ from the archive, the
environment has drifted and the run is INVALID-B1-FLOOR. The replay is **not** a new
experiment and does **not** change the arms, thresholds, or corpus.

If G5A floor evidence cannot be obtained or is not comparable, or if the same-run
replay does not reproduce the archived A3 facts, the run emits INVALID-B1-FLOOR (I10).

---

## 8. Frozen metrics

For each file and each arm report: source bytes; carrier-body bytes; envelope
length; token-region length; complete compressed bytes; exact roundtrip; token
chunk count; structured token bytes; structured frame count; raw frame count;
shape count; Brotli encode time (diagnostic only); carrier construction time
(diagnostic only); `canonical_token_multiset_sha256` (identical across arms);
`envelope_sha256` (identical across arms); `brotli_encoder_version` (integer and
dotted form; identical across arms).

Derived deterministic byte effects (B0 is the frozen null; B0 complete bytes
reported for every file):

```
b2_vs_b1_bytes = B1_complete - B2_complete     # > 0 means B2 beats the floor
b2_vs_b0_bytes = B0_complete - B2_complete     # > 0 means B2 beats the null
b1_vs_b0_bytes = B0_complete - B1_complete     # > 0 means floor beats the null

b2_vs_b1_pct = (B2_complete / B1_complete - 1) * 100
```

Negative percentages mean the later ordering is smaller. Because Brotli output
bytes are deterministic under the frozen build, G5B-ORDINAL does not use
statistical significance language for byte differences. Timing is not a promotion
axis.

---

## 9. Aggregate attribution

For D1-D4 define sums over complete bytes:

```
S0 = sum(B0_complete)
S1 = sum(B1_complete)
S2 = sum(B2_complete)

b2_vs_b1_bytes = S1 - S2
b2_vs_b0_bytes = S0 - S2
```

Report the per-file A3-floor comparison and the non-degenerate file count
explicitly.

---

## 10. Frozen ruling (first match wins)

Correctness invariants dominate every classification. Apply in this exact order;
the first matching rule wins:

```
INVALID-B1-FLOOR         if the B1 archived G5A floor / provenance / replay is a mismatch (I10)
INVALID-G5B-ORDINAL      if any invariant (incl. selector byte I7) / corpus / backend / ruling-integrity failure
ORDINAL-ADVERSE          if S2 > S1
ORDINAL-NEUTRAL          if S2 == S1
ORDINAL-UNSUPPORTED-BY-NULL  if S2 < S1 and S2 >= S0
ORDINAL-WEAK             if S2 < S1 and S2 < S0 and S2 / S1 > 0.99
ORDINAL-CONCENTRATED     if <= 0.99 but B2 smaller on < 2 non-degenerate D1-D4 families
ORDINAL-MATERIAL         otherwise
```

Explicit precedence form:

```
INVALID-B1-FLOOR      if B1 floor/provenance mismatch
INVALID-G5B-ORDINAL   if invariants/selector/corpus/backend/ruling fail
ADVERSE               if S2 > S1
NEUTRAL               if S2 == S1
UNSUPPORTED-BY-NULL   if S2 < S1 and S2 >= S0
WEAK                  if S2 < S1 and S2 < S0 and S2 / S1 >  0.99
CONCENTRATED          if S2 < S1 and S2 < S0 and S2 / S1 <= 0.99 and b2_smaller_nondegenerate_files <  2
MATERIAL              if S2 < S1 and S2 < S0 and S2 / S1 <= 0.99 and b2_smaller_nondegenerate_files >= 2
```

The thresholds are the **frozen G5A thresholds**:

- aggregate ratio gate `S2 / S1 <= 0.99` (>= 1.0% aggregate improvement);
- breadth gate: at least **2** non-degenerate D1-D4 files where B2 < B1;
- null gate `S2 < S0` (one deterministic draw; never a significance test).

They must not be tuned.

ADVERSE and NEUTRAL make no favorable claim and precede the null test. The single
deterministic null is NOT a statistical significance test; no p-value, no
"random is worse on average" language is permitted.

### 10.1 V1 diagnostic (separate; descriptive only)

V1 is reported separately, AFTER re-gating correctness, and never affects the
discovery ruling:

- `V1-ORDINAL-FAVORABLE` if B2 < B1;
- `V1-ORDINAL-NEUTRAL` if B2 == B1;
- `V1-ORDINAL-ADVERSE` if B2 > B1;
- `V1-INVALID-G5B-ORDINAL` if any V1 invariant fails (fail closed; no ordering
  label is emitted).

On the `V1-INVALID-G5B-ORDINAL` path (r3 correction, I12) the source-of-record
classification is routed to **INVALID-G5B-ORDINAL**, `b2_interpretation_prohibited`
is set, and any discovery `ORDINAL-*` value survives only inside a clearly-labeled
diagnostic field; it is never the ruling and never the workflow output. The
ordinary valid-V1 path is unchanged.

Also report the B0/B1/B2 bytes and the B1->B2 decomposition. This diagnostic is not
a held-out test.

---

## 11. What G5B-ORDINAL may conclude

G5B-ORDINAL may conclude only:

- whether lifting ordinal-crossing from per-exact-shape (B1) to global ordinal
  (B2) materially changes compressed size on the frozen D1-D4 discovery set;
- whether B2 beats the deterministic structure-destroying null (B0);
- whether any effect is concentrated or degenerate by file;
- whether known-stress V1 reacts favorably or adversely to the same frozen
  permutation (descriptive only).

G5B-ORDINAL may **not** conclude:

- that slots are logical fields, or that slot j of one shape is the same logical
  field as slot j of another;
- that any semantic/path alignment exists or has been established;
- that the flat-vs-semantic-hierarchy question is answered;
- that the representation generalizes broadly;
- that a production planner or transform exists;
- that V1 validates any G5B-ORDINAL hypothesis.

**Interpretation of PASS / MATERIAL:** a `MATERIAL` ruling means only that the
exact-shape boundary materially **fragments positional same-ordinal locality** on
D1-D4, and therefore justifies investing in the original semantic/path hierarchy
experiment. It does **not** mean slots are logical fields, does **not** establish
generalization, and does **not** validate a production transform.

**Non-pass disposition:** a non-pass (ADVERSE / NEUTRAL / UNSUPPORTED-BY-NULL /
WEAK / CONCENTRATED) closes only the **ordinal proxy**. It does **not** close the
semantic-hierarchy lane. The original semantic G5B lane remains OPEN regardless.

---

## 12. Explicit disposition

Regardless of the ruling:

- G4 remains **NO-GO**; no planner is promoted;
- raw Brotli remains the **permanent fallback**;
- no production transform ID is allocated;
- no production ANVIL source is changed by G5B-ORDINAL;
- no held-out promotion occurs; no new held-out corpus is opened;
- V1 is a known-stress diagnostic only, never a generalization or promotion gate.

---

## 13. Rejected alternatives (recorded)

The following candidate keys/mechanisms were considered and rejected for this
first causal freeze:

- **null/non-null subgroup key** - scalar-dependent, not structural; it would key
  on which slots happen to be null in a record, entangling the treatment with
  observed scalar content.
- **optional-field presence within an exact shape** - vacuous, because the exact
  template already changes with presence; the shape identity would already have
  split records that differ in presence, so the key adds nothing.
- **member refinement by parser depth / parent kind** - the frozen parser syntax is
  effectively shape-level, so these are not a clean within-shape key and would
  import parser-internal structure rather than a citable positional ordinal.
- **semantic path fusion from current gap parts without explicit semantics** -
  would smuggle a semantic claim into an ordinal experiment; it belongs to a
  separate future semantic-hierarchy experiment and is explicitly excluded here.
- **frame-neighbor / context key** - a contextual clustering key, not a positional
  hierarchy/ordinal mechanism; it changes locality context rather than scope of an
  ordinal.
- **B3 source-interleave** - a deferred diagnostic, not needed for this first
  causal freeze; omitting it keeps the experiment 3-arm so only one treatment
  mechanism (B2) changes.

---

## 14. What G5B-ORDINAL does not contain

Explicitly excluded:

- S0/S1/S2 planner rescue;
- learned scoring;
- new q11 leaf search;
- dictionary, integer, float leaves;
- RLE/default-exception;
- FSST/string symbols;
- cross-column predictors;
- schema union, semantic hierarchy, or semantic path fusion;
- SIMD optimization;
- production wire IDs;
- any new metadata or representation family.

Those are separate future lanes.

---

## 15. Execution discipline

1. commit this preregistration and the frozen implementation source BEFORE any
   G5B-ORDINAL D1-D4 or V1 corpus measurement;
2. the implementation must compile against the pinned frozen G3 source via section
   2.1;
3. local workstation work is limited to compile + tiny synthetic correctness
   selftests (no D1-D4/V1, no heavy benchmarks);
4. freeze the implementation source SHA in a **first** commit, then pin that SHA (and
   the source/prereg blobs) in the workflow in a **second** commit, so the workflow
   never has to reference its own commit's SHA;
5. create a GitHub Actions workflow pinned to that exact implementation that
   materializes and verifies the pinned frozen G3 source before building;
6. CI verifies frozen G3 source identity and records compiler/package/libbrotli
   provenance (compiler hash recorded; linked Brotli identity and encoder version
   required for comparability);
7. CI fetches and verifies D1-D4 + V1 corpus identities remotely;
8. CI acquires and verifies the frozen G5A floor evidence (run `35985412906`, artifact
   id `10801714249`, pinned digest) with exact member-path extraction, verifies the
   archived provenance, and verifies B1 exact reproduction on D1-D4 (I10);
9. CI runs a same-run frozen G5A reference replay on D1-D4 only (section 7.3) and
   compares its A3 per-file facts to the archive and to B1; the replay control gate
   makes environment drift diagnosable;
10. CI runs B0/B1/B2 on D1-D4 (discovery) and on V1 (known-stress);
11. CI applies the frozen section-10/10.1 ruling mechanically, fail-closed (I12);
12. CI uploads complete evidence with `if: always()`;
13. only then is a source-of-record result written.

No post-outcome threshold, arm, or gate changes are allowed. This worker does NOT
dispatch the workflow.

---

## 16. Relationship to the original semantic G5B lane

The original G5B lane remains:

> exact flat shapes versus explicitly preregistered structural hierarchy.

G5B-ORDINAL is an interstitial ordinal proxy that may be used as motivation and
architecture context for that lane, but it may not change that lane's own frozen
contract, and it does not itself answer the semantic-hierarchy question. A
G5B-ORDINAL non-pass closes only the ordinal proxy, not semantic hierarchy.

---

## 17. Reproduction

Authoritative frozen identities for the CI run:

- frozen implementation SHA: pinned in the workflow in a **second, follow-up commit**
  (staged two-commit pinning plan; the freezing commit cannot pin its own SHA);
- G5B-ORDINAL source blob: pinned in the workflow (second commit);
- G5B-ORDINAL prereg blob: pinned in the workflow (second commit);
- frozen G3 SHA `1a3d18fed76adb6fb33264e1994f9c357306b3fa`, blob
  `eedc7b7e6671c4a5fcaf7a5997671bd4be30bdde`, SHA-256
  `5b3ab1cdc67d1a8d8edd7eeb4265361a66f72e5b8620137149ce802233b58a9e`;
- G5A floor reference run: `35985412906` (run head SHA
  `996c2dbe67736c42288287abba7ed6f3307a0b34`, asserted by CI as a checked pin);
- G5A floor artifact: id `10801714249`, name
  `grotli-g5a-ordering-attribution-35985412906`, size `15631`, digest
  `sha256:4765b317e7c01170cffbd96b08c639e001ec1e2f317152d1f83a20a5331a3278`,
  expired `false`, expires `2026-10-24T10:11:27Z`;
- frozen G5A implementation identity (for the same-run replay): commit
  `e6714e81aeff1579c4502f1fd9af4d7f205a8b4b`, source blob
  `2772d7eaf0a64be1fdbbb377f5d668d21dab9cbe`, prereg blob
  `a84f42a3f414cc987d8c19cc70a578e2d4222467`;
- backend: libbrotli `1.1.0-2build2` (Ubuntu 24.04), `BrotliEncoderVersion()` =
  `16781312` (`1.1.0`), q11 / lgwin30.
