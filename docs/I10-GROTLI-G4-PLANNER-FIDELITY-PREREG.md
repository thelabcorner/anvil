# ANVIL I10 — GROTLI G4 Planner Fidelity Preregistration

**Status:** FROZEN BEFORE G4 D1-D4 DISCOVERY MEASUREMENT AND BEFORE CONSULTING ANY G3 V1 OUTCOME
**Date:** 2026-09-24
**Freeze revision:** r5 (r4 plus final ambiguity closure: DSTAR/ISTAR rule-vs-resolution wording and explicit G2_WHOLE exclusion from S3; applied before any G4 D1-D4 discovery measurement)
**Supersedes:** the earlier placeholder revision of this path (commit `e7c7010`)
**Parent evidence:** G3 discovery **PASS only** — workflow run `35947013432`
**Parent implementation:** frozen G3 regionized semantics and G3 leaf oracle
**New representation mechanism authorized:** **none**
**New wire semantics authorized:** **none**
**New corpus family authorized:** **none**
**Production integration authorized:** no
**Production source `src/anvil.cpp`:** must not be modified
**Heavy benchmarking:** GitHub Actions only

---

## 0. Freeze declaration

This document is frozen **while G3 held-out V1 run `35947013432`'s validation job
is still in flight**, and **its outcome has not been consulted**.

The only parent evidence permitted by this document is the **G3 discovery PASS**:

- D3 contains at least one structured frame and at least one raw residual frame;
- D3 regionized best (`1,240,155 B`) is strictly smaller than D3 raw Brotli
  (`1,292,757 B`);
- the four-file routed discovery aggregate is **-5.5984%** versus raw Brotli.

Nothing else about G3 is used, assumed, or required.

In particular:

- **G4 may not use the G3 V1 result as an input to any decision, parameter, gate,
  arm choice, or corpus choice.**
- If the G3 V1 outcome later becomes known, it **must not change** this
  document, the G4 arms, the G4 gates, the G4 tie-breaks, or the G4 selection
  hierarchy.
- Any G4 number reported without the V1 outcome being unknown-at-freeze is
  already invalid as a preregistered result.

This is a **pre-discovery-measurement implementation-audit freeze**, not a claim
of a pristine pre-implementation freeze. A tiny local G4 correctness prototype
existed while r2/r3 were finalized and was exercised only on synthetic fixtures
to verify compilation, exact roundtrip, deterministic scoring behavior, and
frozen-G3 semantic identity. No D1-D4 G4 discovery object was measured and no
G4 corpus outcome was available when r3 was frozen.

No D1-D4 G4 byte may be measured before this revision is committed and
published. Synthetic correctness fixtures are not discovery evidence and may
not be used to tune any G4 ranking surface, portfolio mask, gate, or tie-break.

---

## 1. Research oracle versus production planner

G3's frozen candidate builder scores each eligible leaf candidate by an exact
isolated Brotli q11 evaluation of the complete serialized isolated leaf object.

That is a **research oracle**. It produces faithful labels for
candidate ranking, but it is not a viable production planner: it performs on the
order of one expensive backend call per (column × eligible leaf), which is
`O(slots × candidates)` and therefore not scalable.

The production planner must not call q11 to rank candidates.

> **q11 is a verifier, not a ranking primitive.**

G4 does not re-derive this principle; G4 **measures** whether a cheap, deterministic
ranking surface can replace it without materially changing complete compressed
bytes at the carrier level.

### 1.1 What O11 is

The frozen O11 arm is the **reference planner**, i.e. G3's frozen isolated
per-candidate Brotli q11 scoring followed by raw-wins-ties argmin inside each
slot.

**O11 is a REFERENCE, not an assumed optimum.**

- O11 is not guaranteed to be globally optimal for complete-carrier bytes.
- O11 scores candidates **in isolation**, so it ignores cross-slot interactions
  by construction.
- The whole-carrier q11 bytes produced after O11 selection are a **reference
  number**, not a lower bound.
- A proxy may legitimately beat O11's complete bytes on some file. That is
  interesting, not an error, and must be reported but not celebrated.

### 1.2 What G4 is not

G4 is a **planner-fidelity experiment only**. G4:

- adds no new representation mechanism;
- adds no new wire semantics;
- adds no new corpus family;
- does not validate any new compression mechanism;
- does not establish global optimality of any planner;
- does not establish a Pareto crossing.

---

## 2. Frozen basis inherited unchanged from G3

G4 inherits the frozen G3 representation exactly. G4 may not alter:

- frame/region eligibility;
- the LF-inclusive frame rule (LF terminates and is included; CRLF retained
  inside the frame; final remainder is a frame);
- the exact lexical JSON parser;
- scalar span semantics;
- exact shape identity;
- first-occurrence deterministic shape IDs;
- exact shape dictionary;
- record/reconstruction-order shape-ID stream;
- structured-versus-residual frame classification;
- the leaf eligibility and payload grammars;
- the raw residual frame path;
- the raw Brotli q11/lgwin30 fallback;
- the exact metadata accounting;
- all G3 decoder strictness and adversarial behavior.

### 2.1 Frozen leaf basis

Exactly five leaves, unchanged:

1. `RAW_LEX`
2. `EXACT_DICT`
3. `INT_FOR`
4. `INT_DELTA_FOR`
5. `INT_DOD_FOR`

No sixth leaf. No modified payload format. No partition of an existing payload
into new streams. No reordering of an existing grammar.

### 2.2 Excluded from G4

Explicitly not present in G4:

- RLE / default-exception leaf;
- Gorilla;
- ALP;
- FSST / learned symbols;
- byte shuffle / bit planes;
- cross-column prediction;
- schema unioning / shape hierarchy;
- learned grammar;
- replay;
- TCOPY / PNRA;
- alternate entropy backend;
- multi-stream backend split;
- any change to the final Brotli q11/lgwin30 backend.

### 2.3 Production source

`src/anvil.cpp` remains **untouched**. G4 is a standalone research prototype in
the same style as the frozen G3 prototype. No production wire ID is allocated by
this document.

---

## 3. The two G4 questions

G4 asks exactly two questions, and must answer them separately.

### Q1 — Proxy fidelity

> Can cheap deterministic candidate-ranking surfaces produce the **frozen G3
> regional portfolio result** whose final Brotli q11/lgwin30 bytes are not
> materially worse than the result produced under the O11 reference ranking
> surface, while removing essentially all q11 candidate-ranking calls?

### Q2 — Per-slot separability

> Is per-slot leaf selection approximately separable, or are pairwise cross-slot
> interactions large enough that a production planner must plan jointly across
> slots?

These questions are independent:

- a proxy can be faithful while the slots are nonseparable (fine);
- a proxy can be unfaithful while the slots are separable (fine);
- **if the separability probe marks NONSEPARABLE, production adoption is blocked
  even when a cheap proxy matches O11** (see §10).

---

## 4. Frozen ranking surfaces and the frozen G3 regional portfolios

G4 has **two orthogonal frozen dimensions**, and it must never conflate them:

```
ranking surface   P in { O11, S0, S1, S2 }
family portfolio  F in { RAW, DICT, INT, MIXED }
```

- `P` decides **which leaf is chosen** in each eligible slot, using one frozen
  candidate score.
- `F` is a **fixed G3 family mask** that decides **which leaves are allowed at
  all** for that portfolio.

Every ranking surface `P` is evaluated **through all four frozen portfolios**
`F`. There is no arm in G4 that is "just S0", and there is no single mixed
carrier per ranking surface. G4 may **not** collapse G3's two-level architecture
into one carrier.

### 4.0 Frozen G3 regional portfolio masks (unchanged in G4)

G3 does not emit one all-leaf carrier per scoring surface. Its candidate scores
are reused to construct **four fixed regional portfolios**, each restricting the
allowed leaf set:

```
REGION_RAW    allowed = { RAW_LEX }
REGION_DICT   allowed = { RAW_LEX, EXACT_DICT }
REGION_INT    allowed = { RAW_LEX, INT_FOR, INT_DELTA_FOR, INT_DOD_FOR }
REGION_MIXED  allowed = { RAW_LEX, EXACT_DICT, INT_FOR, INT_DELTA_FOR, INT_DOD_FOR }
```

Freeze rules:

- these four masks are reproduced **exactly** in G4 for every ranking surface
  `P in {O11, S0, S1, S2}`;
- `REGION_RAW` has exactly one allowable leaf, so it is planner-independent by
  construction: `C(f,P,RAW)` must be the same bytes for every `P`. That equality
  is itself a G4 correctness invariant (G5.6);
- `C(f,O11,*)` must reproduce the frozen G3 regionized carriers byte-for-byte
  (G5.7);
- no portfolio mask may be widened, narrowed, reordered, or made per-file;
- no "best family" routing logic may be introduced; family arbitration is a
  separate future lane.

### 4.1 Frozen leaf IDs

Leaf IDs are the frozen G3 enumeration, **zero-based**:

```
RAW_LEX        = 0
EXACT_DICT     = 1
INT_FOR        = 2
INT_DELTA_FOR  = 3
INT_DOD_FOR    = 4
```

These values are used for serialization, for the deterministic enumeration order
(§6.2), and for the tie-break in §6.1.

### 4.2 O11 — isolated Brotli q11 reference

For each eligible slot, each eligible leaf candidate is serialized as the exact
frozen G3 isolated leaf object:

```
leaf_id
occurrence_count
payload_length
payload
```

and scored:

```
score_O11(candidate) = Brotli_q11_lgwin30( serialized isolated leaf object )
```

Per slot, restricted to the portfolio's allowed leaves: argmin over candidates;
**scalar score tie -> `RAW_LEX` wins, then lower leaf ID** (§6.1).

This is the G3 frozen scoring surface, reproduced bit-for-bit in G4, including
the fact that its scores are then reused for all four regional portfolios.

### 4.3 S0-WIRE — serialized leaf-object byte size

```
score_S0(candidate) = byte length of the exact serialized isolated leaf object
```

No compressor. No histogram. No metadata beyond what is already inside the
serialized object.

**All metadata is charged**: the serialized object includes leaf ID, occurrence
count, and payload length, so the proxy cannot win by ignoring header bytes.

Per slot, restricted to the portfolio's allowed leaves: argmin;
**scalar score tie -> `RAW_LEX` wins, then lower leaf ID** (§6.1).

Purpose: analytical floor. Expected to be strong for compact integer leaves and
weak for dictionary-versus-raw decisions (G2/G3 precedent: serialized size can
point the wrong direction once the backend sees the whole carrier).

### 4.4 S1-H0 — order-0 byte entropy estimate over the complete serialized object

`S1` scores the **complete serialized isolated leaf object** (identical bytes to
S0's input: leaf ID, occurrence count, payload length, payload), never a
sub-part of it.

Define, in exact integer arithmetic:

```
obj            = exact serialized isolated leaf object bytes for the candidate
N              = len(obj)                        # N >= 1 for every candidate
count[b]       = number of occurrences of byte value b in obj, b in 0..255
                 (so sum_b count[b] == N)

# integer order-0 ideal-code cost, in fractional bits scaled by 256:
#   for each b with count[b] > 0:
#       term_b = 256 * count[b] * ( ceil_log2(N) - floor_log2(count[b]) )
#   where log2 here is exact floor/ceil over integers:
#       floor_log2(x) = position of the highest set bit of x, x >= 1
#       ceil_log2(x)  = 0 if x == 1 else floor_log2(x - 1) + 1
#   term_b is non-negative because count[b] <= N.
H0_scaled      = sum over b of term_b
```

Scoring is monotone in `H0_scaled`, so **no division and no floating point are
used in the ranking path**:

```
score_S1(candidate) = ( H0_scaled, S0_size, leaf_id )
```

compared lexicographically as unsigned integers.

Per slot, restricted to the portfolio's allowed leaves: score each eligible
candidate as above, then argmin.

Tie handling for S1 is **lexicographic on the full tuple**, and this is frozen:

```
RAW_LEX wins a tie ONLY IF the FULL tuple (H0_scaled, S0_size) ties exactly.
Otherwise the strictly smaller tuple wins, even if H0_scaled is equal.
If the FULL tuple ties, then lower leaf ID wins.
```

Concretely: if candidate A has `H0_scaled = 1000, S0_size = 40` and candidate B
has `H0_scaled = 1000, S0_size = 41`, then A wins on the tuple, and **B does not
win merely because B is RAW_LEX**. Equal `H0_scaled` alone is not a tie.

Notes that are part of the freeze:

- `H0_scaled` is a deterministic **upper-bound-flavored** estimate;
  `ceil_log2(N) - floor_log2(count[b])` overestimates the Shannon term for
  non-power-of-two counts. That is deliberate: it is exact, integer, portable,
  and identical on every runner.
- No LZ match search occurs in S1.
- No Brotli call of any quality occurs in S1.
- The metadata charge is implicit and exact: S0's size is the S1 tie-break, and
  the metadata bytes are inside `obj` and therefore inside the histogram.
- A **fixed deterministic metadata charge** in the sense of a synthetic
  constant is explicitly **not** added, because the real metadata is already in
  `obj`; adding a second constant would double-charge. This is frozen now to
  prevent a post-hoc "we forgot metadata" reinterpretation.

Purpose: test whether simple byte statistics recover dictionary-versus-raw
choices that payload length misses.

### 4.5 S2-BR1 — isolated Brotli q1 score

```
score_S2(candidate) = Brotli_q1_lgwin30( serialized isolated leaf object )
```

- exactly **zero** q11 candidate-ranking calls;
- at most **one q1 call per candidate**;
- identical serialized-object input as S0/S1;
- same window setting where the API permits, and the actual window/quality
  values used must be recorded in the result row.

Per slot, restricted to the portfolio's allowed leaves: argmin;
**scalar score tie -> `RAW_LEX` wins, then lower leaf ID** (§6.1).

Purpose: test whether a much cheaper member of the same codec family preserves
q11 candidate ordering well enough to be a bridge proxy.

S2 is a **fallback proxy**, ranked last in the adoption hierarchy (§7). It is not
the long-term target.

### 4.6 No other ranking surface

G4 has exactly four ranking surfaces `P`, crossed with exactly the four frozen
portfolios `F` of §4.0: `O11`, `S0`, `S1`, `S2`. There is no fifth ranking
surface, no cross-surface fused per-column score, no family arbitration, and no
learned or tuned planner in G4.

`S3` is **not** a ranking surface. It is the frozen **complete-carrier finalist
portfolio** defined in §4.7, built entirely from already-frozen `(P, F)` carriers
and arbitrated by exact whole-carrier bytes.

Any further family-wise or hybrid planner is a **later, separately preregistered**
experiment.

### 4.7 S3 — frozen complete-carrier finalist portfolio

S3 exists because S0/S1/S2 candidate-score **magnitudes are not commensurate**:
they measure different things (bytes, entropy estimate, q1 bytes) and must never
be compared across surfaces.

Therefore S3 never compares scores from different `P` surfaces. It compares only
**exact complete-carrier Brotli q11/lgwin30 bytes** of a **fixed, bounded finalist
set**.

#### 4.7.1 Per-surface, per-family selection (no cross-surface score comparison)

For every `P in {O11, S0, S1, S2}` and every `F in {RAW, DICT, INT, MIXED}`, the
leaves are selected using **only** `P`'s own ranking surface and `P`'s own frozen
tie-break, restricted to `F`'s allowed leaf set (§4.0, §5, §6). This is exactly
the `(P, F)` machinery already frozen in §5.

> **No candidate score from one surface is ever compared to a candidate score
> from another surface, at any level, for any purpose.**

#### 4.7.2 `DSTAR` — the dictionary surrogate family, chosen mechanically

```
DSTAR =
    S0   if S0 passes the frozen DICT-specific fidelity gate (G8.10)
    else S1 if S1 passes G8.10
    else S2 if S2 passes G8.10
    else (none; DSTAR unavailable)
```

The order `S0, S1, S2` is the already-frozen proxy preference order of §7. It may
not be reordered, and it may not be informed by observed bytes.

#### 4.7.3 `ISTAR` — the integer surrogate family, chosen mechanically

```
ISTAR =
    S0   if S0 passes the frozen INT-specific fidelity gate (G8.11)
    else S1 if S1 passes G8.11
    else S2 if S2 passes G8.11
    else (none; ISTAR unavailable)
```

Same frozen order, same prohibition.

#### 4.7.4 Frozen S3 finalist set

Per file, `S3` evaluates the following **fixed** bounded finalist set:

```
1. RAW_BROTLI
2. ISTAR_REGION_INT      = C(f, ISTAR, INT)
3. DSTAR_REGION_DICT     = C(f, DSTAR, DICT)
4. ISTAR_REGION_MIXED    = C(f, ISTAR, MIXED)
5. DSTAR_REGION_MIXED    = C(f, DSTAR, MIXED)
```

Rules that are frozen now:

- the set is **minimal** and **fixed before measurement**; it may not be
  enlarged, shrunk, or per-file tuned after seeing D1-D4;
- if `DSTAR` or `ISTAR` is unavailable, its finalists are **omitted** and the
  omission is reported explicitly; it may not be substituted by another surface's
  carrier on a per-file basis;
- frozen `G2_WHOLE` is **not an S3 finalist**. It remains a separately reported
  overall-ANVIL diagnostic/fallback in §G8.9, but excluding it from S3 keeps the
  production-shaped G4 diagnostic focused on the new cheap-planner regional
  candidates plus permanent `RAW_BROTLI`. It may never rescue a Q1 fidelity
  failure or an S3 result;
- no `(P, F)` pair outside the list above may be added to S3;
- byte-identical finalists may be **deduplicated only by exact bytes/hash**. The
  report must name which labels collapsed, report distinct reused-result counts
  before/after dedup, and must still report `s3_additional_q11_calls = 0`.

#### 4.7.4a S3 reuses already-measured verifier results (no new q11 calls)

Every S3 finalist **is already one of the `P × F` carriers** whose exact q11
complete bytes were measured for Q1, or the already-measured `RAW_BROTLI` result.
S3 therefore **reuses those measured results** and **must not recompress them**.

Freeze:

```
s3_additional_q11_calls     = 0
s3_q11_rank_calls           = 0
```

- the actual verifier cost was already paid by the `P × F` plan, where
  `final_q11(P) = 4` per proxy per file (§G8.5). S3 adds **nothing** to it;
- **reused results must never be labeled as new q11 calls.** A reused measured
  result is a *consulted result*, not a *call*;
- frozen `G2_WHOLE` is outside S3 and is reported only through the generic
  diagnostic portfolio of §G8.9;
- S3 may read the `(P, F)` complete-byte matrix and `RAW_BROTLI`, but it may not run a compressor
  to produce its own numbers;
- the distinction is mandatory in the report:

```
sum of "calls"  = paid verifier calls   (final_q11(P) = 4 per proxy per file)
sum of "reuses" = consulted results     (S3 finalists, deduplicated)
```

These two quantities are reported separately and are never added together, and a
reuse is never reported as a call reduction attributable to S3.

#### 4.7.5 S3 arbitration (P6, not ranking)

S3's final selection is the exact complete-carrier bytes over the fixed finalist
set:

```
C_S3(f) = min over the frozen finalist set of complete q11/lgwin30 bytes
```

Frozen label order for exact ties:

```
RAW_BROTLI, then ISTAR_REGION_INT, then DSTAR_REGION_DICT,
then ISTAR_REGION_MIXED, then DSTAR_REGION_MIXED
```

This is **P6 verification / final arbitration, NOT ranking**:

- it uses no candidate scores;
- it performs no cross-surface score comparison;
- `q11_rank(S3) = 0`;
- `s3_additional_q11_calls = 0`: it **reuses** the already-measured `(P, F)`
  verifier results (§4.7.4a) and recompresses nothing;
- reused results are reported as consulted results, **separately** from paid
  verifier calls and **separately** from ranking calls.

This S3 tie order is **authoritative for S3** and supersedes the generic
diagnostic carrier-level tie order of §6.3 for S3 arbitration. The two orders are
not interchangeable: §6.3 orders final portfolio *labels over the whole diagnostic
candidate set*, whereas §4.7.5 orders *S3's own fixed finalist labels only*.

---

## 5. Frozen carrier construction rule (two-level)

G4 reproduces G3's **two-level** planner exactly: a ranking surface chooses the
leaves, and a fixed family portfolio decides which leaves were allowed.

For every discovery file `f`, every ranking surface `P in {O11,S0,S1,S2}`, and
every frozen portfolio `F in {RAW,DICT,INT,MIXED}`:

1. Build the frozen G3 structural/regional analysis once (frames, structured
   versus residual classification, shapes, groups, columns, eligible
   candidates). This step is **ranking-surface-independent and
   portfolio-independent** and must be byte-identical for all `(P, F)`.
2. Materialize **all** eligible leaf candidates' complete serialized isolated
   leaf objects **once** per file (shared across all `P`). No score is computed
   in this step.
3. For each eligible slot, score candidates with `P`'s frozen score (§4) and
   choose one leaf by `P`'s frozen tie-break (§6.1), **restricted to `F`'s
   allowed leaf set** (§4.0). A slot with no allowed candidate uses `RAW_LEX`,
   which is allowed in every `F`.
4. **Assemble the complete frozen G3 carrier** for `(P, F)` using those selected
   leaves, including all G2/G3 shape/group/leaf/reconstruction-order metadata,
   all raw residual frames, all header/envelope bytes, and all payload-length
   bytes.
5. Run **Brotli q11/lgwin30 exactly once** over that complete carrier.
6. Record the complete bytes as `C(f, P, F)`.

### 5.1 Frozen definitions

```
C(f, P, F)      = complete q11 bytes of the (P, F) carrier for file f
C_region(f, P)  = min over F of C(f, P, F)
```

Frozen family tie-break for `C_region(f,P)`: if two or more portfolios tie on
exact complete bytes, the **nominal** family is chosen in the order
`RAW, DICT, INT, MIXED`, matching G3 portfolio reporting order. This tie-break
never changes the number `C_region(f,P)` and never decides a gate; it only fixes
the reported `selected_family` label.

### 5.2 What a proxy is judged by

A proxy is judged by **complete-regional bytes**:
- An arm with 99.9% slot agreement can still fail on complete bytes.
- An arm with lower slot agreement can still pass on complete bytes.
- Slot agreement is reported (§8) but is **never** sufficient to pass.
- Complete-byte regret is authoritative.
- `REGION_RAW` is planner-independent, so `C(f, P, RAW)` must equal
  `C(f, O11, RAW)` for every `P`. Any difference is an implementation defect.
- The portfolio selector is evaluated **inside** each ranking surface. G4 does
  **not** compare "proxy's best family" against "O11's mixed family"; it compares
  min-over-F to min-over-F.

---

## 6. Frozen tie-breaks

All ties below are resolved **exactly**, with no floating point, no randomness,
and no iteration-order dependence.

### 6.1 Candidate ranking ties inside a slot

Tie handling is defined **per ranking surface**, because S1 has a two-component
score. When candidates are restricted by the portfolio mask (§4.0), the tie rules
apply only over that restricted candidate set.

**O11, S0, S2 (scalar scores).** If two or more candidates share the identical
scalar score:

1. `RAW_LEX` (leaf ID 0) wins;
2. then lower leaf ID (`RAW_LEX = 0`, `EXACT_DICT = 1`, `INT_FOR = 2`,
   `INT_DELTA_FOR = 3`, `INT_DOD_FOR = 4`);
3. then the first-occurrence index of the candidate in the deterministic
   candidate enumeration order (§6.2).

**S1 (tuple score).** The score is the tuple `(H0_scaled, S0_size)`, compared
lexicographically as unsigned integers. If two or more candidates share the
identical **full** tuple:

1. `RAW_LEX` (leaf ID 0) wins;
2. then lower leaf ID;
3. then the first-occurrence index in the deterministic enumeration order.

> An equal `H0_scaled` with a different `S0_size` is **not** a tie. The smaller
> tuple wins outright. `RAW_LEX` wins S1 ties only when the **entire**
> `(H0_scaled, S0_size)` tuple ties.

> **All RAW ties win, on the arm's full comparison key.** There is no ranking
> surface in G4 where a non-RAW leaf may win an exact tie on that surface's full
> key.

### 6.2 Candidate enumeration order

Deterministic and frozen:

1. file identity (as listed in §9);
2. shape ID (first-occurrence order);
3. slot/placeholder ID (shape-local order);
4. leaf_id ascending.

### 6.3 Family-level and carrier-level ties

**Family level (inside `C_region`).** Ties on exact complete bytes between
portfolios are resolved nominally by the §5.1 family order
`RAW, DICT, INT, MIXED`. This does not change `C_region`.
**Carrier level (generic diagnostic portfolio, reporting only).** Within one
file, if two candidates of the generic diagnostic portfolio (`§G8.9`: `RAW_BROTLI`,
frozen `G2_WHOLE`, and the `(P,F)` carriers) produce identical complete bytes, the
report must list all tied candidates; the **nominal generic** selected-candidate
tie-break is:

1. `RAW_BROTLI` if raw is among the ties (raw fallback is permanent);
2. then `O11`;
3. then `S0`, then `S1`, then `S2`.

This generic order is a **diagnostic reporting label only**. It is **not** S3's
tie order: S3 arbitration uses its own explicit finalist order in §4.7.5, which is
authoritative for S3. The two must not be substituted for one another, and neither
decides the G4 GO/NO-GO.

---

## 7. Frozen adoption hierarchy

After discovery, if G4 is to be adopted as a production candidate planner, the
choice among proxies is **mechanical** and follows a fixed hierarchy:

1. **Prefer `S0`** if and only if S0 passes **all** applicable gates in §8.
2. **Else prefer `S1`** if and only if S1 passes all applicable gates.
3. **Else prefer `S2`** if and only if S2 passes all applicable gates.
4. **Else `NO-GO-G4`.**

This hierarchy is frozen and may **not** be reordered after seeing results.

Explicitly forbidden:

- picking the proxy with the **best observed ratio**;
- picking the proxy with the **best observed speed**;
- picking the proxy with the **best observed slot agreement**;
- picking a different proxy per file;
- picking a different proxy per column or per shape.

If `S1` is smaller than `S0` but `S0` passes every gate, the adopted proxy is
`S0`. If `S2` is smaller than both but `S0` passes, the adopted proxy is `S0`.

S0 is ranked first deliberately: it is the cheapest and has no compressor call.
The hierarchy exists so that cost discipline cannot be traded away for a small
observed byte advantage after the fact.

---

## 8. Frozen discovery gates

Let, for each discovery file `f` and each proxy ranking surface `P`:

```
C(f, P, F)     = complete q11 bytes of the (P,F) carrier for file f  (§5)
C_region(f, P) = min over F in {RAW,DICT,INT,MIXED} of C(f, P, F)
```

Define:

```
per_file_regret(f, P) = ( C_region(f,P) - C_region(f,O11) ) / C_region(f,O11)
aggregate_regret(P)   = ( sum_f C_region(f,P) - sum_f C_region(f,O11) )
                        / sum_f C_region(f,O11)
```

with `sum_f` over the frozen D1-D4 discovery files only, each file counted once
with the exact frozen byte identity.

**Explicitly excluded from these fidelity numbers:** `RAW_BROTLI` and the frozen
`G2_WHOLE` arm. They remain legitimate final-portfolio diagnostics and fallback
candidates (§4.0, §8.9) but they must **never** be used to compute, dilute, or
mask `C_region(f,P)`. A proxy is never rescued by raw fallback.

A proxy `P` **passes G4 discovery** if and only if **every** condition below
holds.

### G8.1 Correctness (all ranking surfaces, all portfolios, all files)

1. exact roundtrip for every emitted `(P,F)` carrier on every discovery file;
2. identical frozen leaf eligibility and payload semantics as G3;
3. no metadata omission — every decoder-visible byte charged;
4. all mandatory adversarial decoder tests retained and passing;
5. `RAW_BROTLI` present and roundtripping for every file.

Any roundtrip failure is an immediate, unconditional proxy failure.

### G8.2 Aggregate complete-byte regret

```
aggregate_regret(P) <= 0.0025        # <= 0.25%
```

### G8.3 Per-file complete-byte regret

```
per_file_regret(f, P) <= 0.0050      # <= 0.50%   for every discovery file f
```

No exception, no averaging away, no dropped file.

### G8.4 Slot agreement (primary surface: `REGION_MIXED`)

Slot agreement is measured on **one unambiguous common candidate surface**: the
`REGION_MIXED` selection of the proxy versus the `REGION_MIXED` selection of
`O11`. `REGION_MIXED` is chosen because it is the richest common candidate
surface (all five leaves allowed) and therefore the most informative comparison;
it is also the surface on which the separability probe of §10 operates.

Definitions. For each eligible `REGION_MIXED` slot `i`, let `w_i`
be the exact total source-byte length of all lexical tokens in that slot, and let
`I_i(P)` equal 1 when `P` and O11 choose the same leaf and 0 otherwise:

```
weighted_agreement(P) = sum_i( w_i * I_i(P) ) / sum_i( w_i )
raw_agreement(P)      = sum_i( I_i(P) ) / eligible_slot_count
```

If `sum_i(w_i) == 0`, the run is a measurement error. G4 may not invent a
fallback weighting rule after observing such a case.

```
weighted_agreement(P) >= 0.95        # >= 95%, source-byte-weighted, MIXED vs MIXED
```

- `raw_agreement(P)` must be **reported** and must be **reported separately**.
- `raw_agreement(P)` is **not** a pass condition.
- `DICT` and `INT` agreements (proxy `REGION_DICT` vs O11 `REGION_DICT`, and
  proxy `REGION_INT` vs O11 `REGION_INT`) must also be reported, as
  **diagnostics only**. They are not pass conditions.
- Near-tie columns must not be allowed to dominate the weighted number: a column
  whose O11 `REGION_MIXED` top-2 candidates differ by `<= 1` isolated q11 byte is
  a near-tie column. The report must give (i) the count of such columns and
  (ii) the weighted and raw agreement restricted to non-near-tie columns.
- `weighted_agreement(P)` in G8.4 includes **all** columns (near-tie and
  non-near-tie) and is the number the gate uses. The non-near-tie figures exist
  so a passing or failing result can be attributed honestly.
- Slot agreement is **never** sufficient for a pass (§5.2); the byte gates G8.2
  and G8.3 remain authoritative.

### G8.5 q11 ranking-call budget

Let `q11_rank(P)` be the number of q11 calls used to **rank leaf candidates**.

```
q11_rank(S0) = 0
q11_rank(S1) = 0
q11_rank(S2) = 0                       # S2 may use q1 only
q11_rank(O11) = candidates_enumerated  # reference only, never gated
```

- `q1_rank(S0) = q1_rank(S1) = 0`; `q1_rank(S2)` is bounded by the candidate
  count and must be reported exactly.
- An arm whose q11 candidate-ranking calls scale with `slots × candidates` fails
  G8.5. O11 fails G8.5 by construction; that is expected and it is why O11 is
  only the reference.

**Final q11 carrier verification is a separate, fixed cost** and is **not** a
ranking call:

```
final_q11(P) = 4 per proxy per file
               = exactly the four REGION_RAW / REGION_DICT / REGION_INT /
                 REGION_MIXED portfolio encodes
               # no deduplication of byte-identical portfolios is permitted
               # in G4; always exactly four encodes, in the frozen order
               # RAW, DICT, INT, MIXED
```

Freeze:

- **always four**: G4 does **not** deduplicate even when two portfolios would be
  byte-identical, so the call count is trivially deterministic and comparable
  across ranking surfaces. A later lane may preregister safely-deduplicated
  counting with explicit semantics;
- `final_q11(O11) = 4` as well, so every surface pays the identical fixed
  verifier cost, including the two extra `REGION_RAW`-independent encodes;
- final q11 carrier verification is **excluded** from the G8.6 speedup numerator
  and denominator, because every surface pays the same four calls;
- `RAW_BROTLI` whole-file encoding is also outside the ranking-scoring budget.

### G8.6 CPU cost (separate speed gate)

Measured **within the same GitHub Actions job**, same runner, same build, warm
symmetrically, interleaved where practical. The gated quantity is the
**rank/scoring** cost only:

```
speedup(P) = ranking_score_ms_with_selection(O11)
           / ranking_score_ms_with_selection(P)
```

Freeze:

```
speedup(P) >= 5x     mandatory
speedup(P) >= 10x    target
```

This is a **separate** gate and is **excluded** from the four fixed final q11
verifier calls (G8.5).

> Timing **cannot** rescue a byte failure, and byte success **cannot** be
> declared without a passing or failing timing measurement. A byte-passing proxy
> that is slower than 5x fails adoption; a byte-failing proxy that is 100x faster
> fails adoption.

**End-to-end candidate-planner time must also be reported**:

```
candidate_planner_ms(P) =
    structural_parse_ms            (shared, measured once)
  + candidate_materialization_ms   (shared across P)
  + ranking_score_ms(P)
  + selection_ms(P)
```

Freeze:
- `structural_parse_ms` and `candidate_materialization_ms` are **shared** and
  must be measured once per file, separately from any per-surface timing;
- `candidate_materialization_ms` means building every eligible leaf's complete
  serialized object with **no score backend of any quality**;
- **no additional adoption threshold is added** on `candidate_planner_ms(P)` in
  G4 beyond the frozen 5x/10x ranking gate;
- if the reported end-to-end planner time is dominated by the shared
  `candidate_materialization_ms`, the **later production lane must address it**;
  G4 only records it;
- the four final carrier build/encode times must be reported per portfolio.

The same end-to-end quantity is reported for `O11` so the shared terms can be
compared directly.

### G8.7 Deterministic call-count reporting

The result row for every ranking surface must contain, as exact integers:

- `q11_rank_calls`;
- `q1_rank_calls`;
- `final_q11_carrier_calls` (exactly 4 per proxy per file, §G8.5);
- `candidates_enumerated`;
- `slots_eligible`;
- `slots_where_proxy_and_o11_agree_MIXED`;
- `slots_where_proxy_and_o11_disagree_MIXED`;
- `slots_where_proxy_and_o11_agree_DICT` (diagnostic);
- `slots_where_proxy_and_o11_agree_INT` (diagnostic).

For S3, additionally and separately:

- `s3_distinct_finalist_results_before_dedup`;
- `s3_distinct_finalist_results_after_exact_carrier_dedup`;
- `s3_dedup_collapsed_labels` (exact label list);
- `s3_reused_q11_verifier_results` (count of distinct existing `P × F` /
  `RAW_BROTLI` results consulted);
- `s3_additional_q11_calls` (always **0**);
- `s3_q11_rank_calls` (always 0);
- `s3_verifier_calls_are_ranking_calls` (always `false`).

> **Three quantities are never summed into one number.** `q11_rank_calls` counts
> candidate ranking only. `final_q11_carrier_calls` counts **paid** whole-carrier
> verifier calls (`final_q11(P) = 4` per proxy per file). `s3_reused_q11_verifier_results`
> counts **consulted existing results** and is explicitly not a call count. S3
> performs zero ranking calls and zero additional q11 calls, and it is never
> credited with eliminating ranking calls it never made, nor with a call reduction
> from reusing results it did not pay for.

Call counts are deterministic and must be identical across runs on identical
input. A run whose call counts differ from a previous identical run is a
measurement failure and invalidates that arm's result.

For S3, `s3_distinct_finalist_results_before_dedup`,
`s3_distinct_finalist_results_after_exact_carrier_dedup`, and
`s3_reused_q11_verifier_results` are also deterministic integers and are checked
the same way.

### G8.8 Provenance

Each `(P,F)` row must also carry:

- implementation SHA;
- prototype binary SHA-256;
- corpus file name, byte length, git blob SHA-1, and SHA-256;
- runner identity string;
- brotli library version actually linked.

### G8.9 Final portfolio diagnostics (outside fidelity gates)

For each file, in addition to the four `(P,F)` regional encodes, report:

- `RAW_BROTLI` complete bytes;
- frozen `G2_WHOLE` complete bytes where eligible, else `unavailable`;
- the nominal final portfolio selection over
  `{RAW_BROTLI, G2_WHOLE, C(f,O11,RAW), C(f,O11,DICT), C(f,O11,INT), C(f,O11,MIXED),
    C(f,S0,*), C(f,S1,*), C(f,S2,*)}`
  using the §6.3 **generic diagnostic** carrier-level tie-break (which does not
  govern S3; S3 uses §4.7.5).

These numbers are **diagnostics and fallback candidates only**. They are
**excluded** from `C_region(f,P)`, from G8.2, and from G8.3 (§8 preamble), so a
good raw or `G2_WHOLE` row can never mask a bad G4 ranking surface.

### G8.10 DICT-specific fidelity gate (`DSTAR` eligibility)

A ranking surface `P` passes G8.10 if and only if, aggregated over D1-D4 and
evaluated **only on `REGION_DICT`**:

```
P passes G8.1 correctness for REGION_DICT on every file;
sum_f C(f,P,DICT) <= sum_f C(f,O11,DICT) * 1.0025    # <= 0.25% aggregate
max_f ( C(f,P,DICT) - C(f,O11,DICT) ) / C(f,O11,DICT) <= 0.0050   # <= 0.50% per file
P passes G8.5 (q11_rank(P) = 0 for S0/S1/S2);
P passes G8.6 (rank/scoring speedup >= 5x vs O11).
```

`DSTAR` is then selected by §4.7.2's frozen order. This gate is defined
**entirely before measurement** and may not be tuned after D1-D4.

### G8.11 INT-specific fidelity gate (`ISTAR` eligibility)

Identical structure to G8.10, evaluated **only on `REGION_INT`**:

```
sum_f C(f,P,INT) <= sum_f C(f,O11,INT) * 1.0025    # <= 0.25% aggregate
max_f ( C(f,P,INT) - C(f,O11,INT) ) / C(f,O11,INT) <= 0.0050   # <= 0.50% per file
P passes G8.1 correctness for REGION_INT on every file;
P passes G8.5; P passes G8.6.
```

`ISTAR` is then selected by §4.7.3's frozen order.

### G8.12 S3 reporting and non-substitution

- `DSTAR` and `ISTAR` are chosen **globally** from G8.10/G8.11 and applied
  identically across all D1-D4. Per-file switching is a gate violation.
- S3's finalist set is fixed (§4.7.4). Post-hoc addition or removal of a finalist
  after D1-D4 is a gate violation.
- S3 does **not** participate in G8.2/G8.3 proxy-fidelity gates. Those gates are
  `C_region(f,proxy)` versus `C_region(f,O11)` and remain the Q1 answer.
- S3's own result is reported as the final portfolio row, including which label
  won and whether `RAW_BROTLI` won an exact tie.

---

## 9. Discovery population

G4 discovery replays **the exact frozen D1-D4 discovery objects already used by
G2 and G3**, with the exact frozen byte identities recorded in
`I10-GROTLI-G2-CORPUS-FREEZE.md` and re-verified by the frozen G3 workflow:

| ID | name | bytes | git blob |
|---|---|---|---|
| D1 | `D1-amazon-cellphones.ndjson` | 277673 | `9cbb6a071eaa2a10c24d9b4d1729dca501f2f72b` |
| D2 | `D2-cdisc-adae.ndjson` | 615350 | `4bb9ef50f650767967fc3f0a663c73d33c57c608` |
| D3 | `D3-gharchive-10mb.ndjson` | 10485760 | `59d1d00825053e9894a7895fcea0665912524ec8` |
| D4 | `D4-crovia-dpi-receipts.ndjson` | 3585053 | `24020a024a05a68731a2d4de4536bd7ca3eb8576` |

Frozen rules:

- **no corpus substitution** after this document is frozen;
- **no new corpus family** may be introduced inside G4;
- **no G4 implementation may inspect a new validation corpus** before the
  discovery rule selects or fails a proxy;
- discovery is a **planner-fidelity test against an already-known oracle**, not
  an independent generalization claim.

### 9.0 G4 is discovery-only; held-out is a separate later freeze

G4 closes at **discovery**. Discovery PASS does **not** authorize any planner
generalization claim.

If G4 discovery passes, a **new planner-validation corpus must be frozen before
its bytes are measured**, in a **separate** G4 corpus-freeze document that
records identities, commits, byte sizes, and hashes. That corpus should contain
at least:

- one high-cardinality structured population;
- one low-cardinality/dictionary-friendly population;
- one heterogeneous multi-shape population.

**G3 V1 can never serve as an unseen G4 validation set.** G3 V1 is being opened
for G3 while this document is frozen; once its outcome is exposed it is no longer
unseen, and it may not be used for G4 planner validation, tuning, column
selection, gate calibration, or narrative.

Until a separate G4 validation corpus is frozen and measured, G4 may close only
as:

> **planner-fidelity discovery evidence**

and never as broad planner generalization.

### 9.1 G3 discovery observations to record, without overclaiming

These are recorded as **context and engineering budget**, not as G4 evidence:

- D3 regionized discovery produced on the order of **50k-ish isolated Brotli q11
  candidate evaluations**;
- candidate construction and local scoring together took on the order of
  **~32 s**;
- the final selector itself took on the order of **~2 ms**;
- G3 four-file routed discovery aggregate: **-5.5984%** versus raw Brotli;
- D3 region best: **-4.069%** versus D3 raw Brotli;
- on D4, the **whole-file `G2` arm remained the selected portfolio arm**.

Interpretation constraints, frozen now:

- these numbers **motivate** the planner-fidelity question;
- they do **not** demonstrate proxy fidelity;
- they do **not** demonstrate separability;
- they are **not** a G4 result and must never be quoted as one;
- the ~2 ms selector time is **not** an argument that the planner is already
  cheap: the expensive part is candidate scoring, which is exactly what G4
  attacks.

---

## 10. Bounded, outcome-blind pairwise separability probe (Q2)

### 10.1 Purpose

Test whether per-slot selection is approximately separable, or whether pairwise
cross-slot interactions are large enough to require joint planning.

The probe is defined **explicitly and only on `O11 REGION_MIXED`**, because that
is the surface where all five leaves compete and therefore the surface on which
full-leaf local separability is actually testable.

Freeze:

- the probe holds **all non-sampled slots at their `O11 REGION_MIXED` choices**;
- the probe does **not** assert that `REGION_MIXED` is the best G3 portfolio on
  every file. `C_region` (§5.1) remains the fidelity number, and a file whose best
  family is `DICT` or `INT` is unaffected by this choice;
- the probe is therefore a **diagnostic of candidate interaction**, not family
  arbitration, and it may not be used to pick a family or to re-rank portfolios.

### 10.2 Column selection must be outcome-blind

Eligible columns are those with **at least 2 eligible leaves**.

Sampling is frozen and outcome-blind:

```
order_key(file, shape, slot) =
    SHA-256( canonical_bytes(file_identity)
             || 0x1F || shape_id_le_u64
             || 0x1F || slot_id_le_u64 )
```

where `canonical_bytes(file_identity)` is the frozen file name as ASCII.

Columns are sorted ascending by `order_key` (raw 32-byte digest interpreted as an
unsigned big-endian integer; ties impossible in practice, and on a digest tie the
lower `(shape_id, slot_id)` wins). Take the **first N sampled columns per
discovery file**, with:

```
N = min(4, eligible_column_count(file))
```

`N = 4` is the frozen default cap; it is bounded and defensible because the
probe is exploratory and must stay `O(pairs)`.

Explicitly forbidden:

- choosing columns because they look "interesting";
- choosing columns after seeing any score;
- choosing columns from G3 V1;
- choosing columns from any file other than D1-D4;
- adapting `N` per file based on observed interaction sizes.

### 10.3 Pair enumeration

For each discovery file, from its sampled column set `S` (|S| <= 4), enumerate
**all** unordered pairs `{i, j}` with `i < j` in the sampled order:

```
max_pairs = C(4, 2) = 6 per file
max_pairs_total = 24 across D1-D4
```

### 10.4 Independent reference

Let `A_i*` be `O11 REGION_MIXED`'s chosen leaf for sampled column `i`.

Define the **independent O11 MIXED carrier** as the complete G3 carrier in which:

- every sampled column uses `A_i*`;
- **every non-sampled column uses its `O11 REGION_MIXED` choice**;
- all other carrier semantics are unchanged (`REGION_MIXED` mask, i.e. all five
  leaves remain legal in every slot).

Compute its complete q11 bytes: `C_indep` (meaning `C_indep_MIXED`).

### 10.5 Exact pair enumeration

For a pair `{i, j}`:

- let `L_i` be column `i`'s eligible leaves, `L_j` column `j`'s;
- for each `(l_i, l_j)` in `L_i × L_j`:
  - set column `i` to `l_i`, column `j` to `l_j`;
  - keep all other columns (sampled and non-sampled) at their independent
    `O11 REGION_MIXED` choices;
  - assemble the complete frozen G3 carrier;
  - run Brotli q11/lgwin30 **exactly once**;
  - record complete bytes.

The best pair choice is:

```
C_pair(i,j) = min over L_i x L_j of complete_bytes(i, j, l_i, l_j)
```

with the frozen candidate tie-break of §6.1 applied inside the pair.

Cost freeze, stated explicitly:

```
q11 calls for pair {i,j} = |L_i| * |L_j|
bounded per file, per pair, and in total; NOT O(all slots x candidates)
```

The exact `|L_i| * |L_j|` per pair must be reported.

### 10.6 Frozen interaction metric and gate

```
pair_gain_pct(i,j) = ( C_indep_MIXED - C_pair(i,j) ) / C_indep_MIXED * 100
```

i.e. the **complete-byte improvement** of exact pair optimization over
independent `O11 REGION_MIXED` choices, in percent, when all other columns stay
at their `O11 REGION_MIXED` choices. The gain is measured on **complete carrier
bytes**, never on isolated candidate scores.

Frozen gate:

- let `max_pair_gain = max over sampled pairs of pair_gain_pct(i,j)`;
- let `agg_pair_gain` = aggregate sampled-pair improvement, computed as
  `( sum over pairs C_indep_MIXED - sum over pairs C_pair(i,j) ) / sum over pairs C_indep_MIXED * 100`
  where `C_indep_MIXED` is the independent-`O11 REGION_MIXED` complete bytes
  measured once per file and reused for each of that file's pairs.

Then:

> **NONSEPARABLE** if `max_pair_gain > 0.50` **or** `agg_pair_gain > 0.25`.
>
> **SEPARABLE-ENOUGH** otherwise.

### 10.7 Nonseparability blocks production adoption

The separability gate is **orthogonal** to the proxy gates and is **not**
rescuable by proxy quality:

> If the probe is **NONSEPARABLE**, production adoption of any G4 proxy is
> **blocked even if that proxy matches O11 exactly on all four files**.

Rationale, frozen now: a proxy that faithfully reproduces O11 is a faithful
reproduction of a **separable-looking but actually insufficient** planner; the
correct engineering response is joint planning, not planner optimization.

Consequences that must be reported, not hidden:

- G4 may still close as **planner-fidelity discovery evidence** (Q1 answered)
  while simultaneously reporting Q2 NONSEPARABLE.
- In that case the adoption ruling is **CONDITIONAL-JOINT**, and the next lane is
  a separately preregistered joint-planning experiment.
- A NONSEPARABLE result is a **successful scientific outcome**, not a failure of
  G4.

### 10.8 Bounded cost of the probe

Worst case per file with `N = 4` and, say, up to 5 eligible leaves per column:

```
6 pairs * 25 = 150 whole-carrier q11 evaluations per file
<= 600 across D1-D4
+1 C_indep_MIXED per file
```

The exact count must be reported.

Workflow resource budget freeze:

- the discovery workflow timeout and resource budget are **bounded** and are
  declared in the workflow file, not in this document;
- if the bounded workflow **fails to complete** the probe for infrastructure,
  timeout, or resource reasons, the correct verdict is
  **INCONCLUSIVE / INFRA**, not a relaxed probe;
- an INCONCLUSIVE/INFRA probe is **not** permission to shrink `N`, drop files,
  drop pairs, or re-sample columns after seeing partial results;
- any re-run must repeat the **entire** frozen probe on the **entire** frozen
  population, with both the failed attempt and the re-run reported.

---

## 11. Required outputs and diagnostics

### 11.1 Per file, per ranking surface, per portfolio

For every `(P, F)` row:

- file name, source bytes, git blob SHA-1, SHA-256;
- implementation SHA and prototype binary SHA-256;
- BROTLI/q1 window and level actually used;
- `C(f, P, F)` complete bytes;
- exact roundtrip boolean for this `(P,F)` carrier;
- `selected_family` and `C_region(f,P)` for this `P` (reported on the `P` row);
- `per_file_regret(f, P)` versus `C_region(f, O11)`;
- decode time for this carrier;
- the four final carrier build/encode times.

Per file, per ranking surface `P`:

- `C_region(f, P)` and `selected_family` label;
- `per_file_regret(f, P)`;
- `q11_rank_calls`, `q1_rank_calls`, `final_q11_carrier_calls`;
- `candidates_enumerated`, `slots_eligible`;
- `structural_parse_ms` (shared), `candidate_materialization_ms` (shared);
- `ranking_score_ms(P)`, `selection_ms(P)`;
- `candidate_planner_ms(P) = structural_parse_ms + candidate_materialization_ms
   + ranking_score_ms(P) + selection_ms(P)`;
- weighted slot agreement and raw slot agreement versus O11
  (`REGION_MIXED` primary; `REGION_DICT` and `REGION_INT` diagnostic);
- near-tie column count and non-near-tie agreements;
- peak RSS.

Per file, diagnostics outside the fidelity numbers:

- `RAW_BROTLI` complete bytes and roundtrip;
- frozen `G2_WHOLE` complete bytes where eligible, else `unavailable`;
- nominal final portfolio selection and its label (§G8.9).

### 11.2 Per proxy, aggregate

- `aggregate_regret(P)`;
- `max_f per_file_regret(f, P)`;
- `speedup(P)` with the raw numerator and denominator CPU times;
- `candidate_planner_ms(P)` reported against `candidate_planner_ms(O11)`;
- `q11_rank(P)`, `q1_rank(P)`, and the fixed `final_q11(P)`;
- pass/fail per gate with the failing gate named explicitly.

### 11.2b S3 (frozen complete-carrier finalist portfolio)

- `DSTAR` label and the G8.10 numbers that selected it, for every surface in the
  frozen order S0, S1, S2 (including the failing ones, with the failing
  condition named);
- `ISTAR` label and the G8.11 numbers, same form;
- the exact finalist label set per file, before and after dedup;
- `s3_distinct_finalist_results_before_dedup`;
- `s3_distinct_finalist_results_after_exact_carrier_dedup`;
- `s3_reused_q11_verifier_results` (consulted existing results, not calls);
- `s3_additional_q11_calls = 0` and `s3_q11_rank_calls = 0`;
- confirmation that every S3 finalist byte value is the **reused** measured
  `(P,F)` / `RAW_BROTLI` value, with no recompression;
- `C_S3(f)`, the winning label, and whether `RAW_BROTLI` won an exact tie;
- for each finalist, its complete bytes and its delta versus `C_S3(f)`.

### 11.3 Separability probe (all on `O11 REGION_MIXED`)

- sampled columns per file with their `order_key` digests;
- eligible leaf lists per sampled column;
- `C_indep_MIXED` per file;
- confirmation that all non-sampled slots are at their `O11 REGION_MIXED` choices;
- `C_pair(i,j)` and `pair_gain_pct(i,j)` for every pair;
- `|L_i| * |L_j|` q11 calls per pair;
- `max_pair_gain`, `agg_pair_gain`;
- **SEPARABLE-ENOUGH** / **NONSEPARABLE** / **INCONCLUSIVE-INFRA** verdict with
  the triggering number.

### 11.4 Family attribution (diagnostic only)

- selected leaf counts by family per `(P,F)` per file;
- disagreement counts by family between each proxy and O11;
- whether any proxy failure is concentrated in a single family;
- the full `C(f,P,F)` matrix, so a pass or failure can be attributed to a
  specific portfolio rather than hidden by the min.

Diagnostics never promote a proxy. Only the §8 gates and the §10.6/§10.7 gate do.

---

## 12. Decision matrix

Let:

- `P_O` = O11 gate status (reference only; O11 is expected to fail G8.5 by
  construction and is never adopted);
- `P_S0`, `P_S1`, `P_S2` = each proxy's §8 gate status;
- `SEP` = separability verdict from §10.7;
- `ADOPT` = the §7 hierarchy winner, if any;
- `DSTAR`, `ISTAR` = the §4.7.2 / §4.7.3 mechanical selections from G8.10/G8.11;
- `S3` = the §4.7.4 frozen finalist portfolio, arbitrated by §4.7.5.

| # | Correctness (G8.1) | Bytes (G8.2/G8.3) | Agreement (G8.4) | Calls (G8.5) | Speed (G8.6) | SEP | Ruling |
|---|---|---|---|---|---|---|---|
| 1 | all pass | S0 pass | S0 pass | S0 pass | S0 pass | SEPARABLE-ENOUGH | **GO-G4-S0** — adopt S0 as production-candidate planner |
| 2 | all pass | S0 fail, S1 pass | S1 pass | S1 pass | S1 pass | SEPARABLE-ENOUGH | **GO-G4-S1** |
| 3 | all pass | S0,S1 fail, S2 pass | S2 pass | S2 pass | S2 pass | SEPARABLE-ENOUGH | **GO-G4-S2** |
| 4 | all pass | any proxy pass | any slot agreement | passes | passes | **NONSEPARABLE** | **CONDITIONAL-JOINT** — Q1 answered, Q2 says joint planning required, adoption blocked |
| 5 | any roundtrip or accounting failure | — | — | — | — | any | **NO-GO-G4** — fix correctness; no planner claim |
| 6 | all pass | all proxies fail byte gates | — | — | — | any | **NO-GO-G4** — no cheap proxy replaces O11 at carrier level |
| 7 | all pass | bytes pass but no proxy passes G8.6 speed | — | — | — | any | **CONDITIONAL-SPEED** — fidelity established, operationally insufficient; report as planner-fidelity evidence only |
| 8 | all pass | bytes pass, calls pass, speed pass | but G8.4 agreement < 0.95 while bytes still pass | — | — | SEPARABLE-ENOUGH | **CONDITIONAL-DISAGREEMENT** — bytes are authoritative, but the disagreement pattern must be explained before adoption |

Ruling definitions:

- **GO-G4-X** — proxy `X` is the adopted production-candidate planner, subject
  to later production integration work. This is **not** production integration.
- **CONDITIONAL-** — the fidelity question is answered but an orthogonal
  condition blocks adoption; the next lane is named explicitly.
- **NO-GO-G4** — no proxy may be adopted from this evidence.

Ordering of matrix rows is the frozen evaluation order: correctness, then bytes,
then agreement, then calls, then speed, then separability. A later row may not
override an earlier failure.

### 12.0 S3 is reported alongside, and never in place of, the Q1 gates

S3 is the **production-shaped finalist portfolio**. It is reported as the final
portfolio row for every file, and it is evaluated with the same fixed machinery
regardless of which proxies pass:

- if both `DSTAR` and `ISTAR` exist, S3 contains exactly the five frozen
  finalists of §4.7.4;
- if only one of them exists, S3 contains that one's finalists plus `RAW_BROTLI`;
- if neither exists, S3 degenerates to `RAW_BROTLI` alone, and this degeneracy
  must be reported explicitly;
- **S3's bytes never substitute for `C_region(f, proxy)` in G8.2/G8.3**, and S3
  winning a file never converts a failed Q1 proxy into a passing one;
- S3 makes **zero** ranking calls (§4.7.5), so it neither improves nor worsens
  any G8.5 result.

S3 answers a different question from Q1 and Q2:

> Does a bounded, production-shaped portfolio of faithful cheap planners,
> arbitrated by exact whole-carrier verification, preserve or improve complete
> bytes while eliminating q11 candidate ranking?

S3 does **not** validate a new compression mechanism (§13.2), and S3's result is
**not** evidence of proxy fidelity.

### 12.1 What a GO-G4 ruling means and does not mean

A **GO-G4** means:

> On the frozen D1-D4 discovery population, the named cheap ranking surface
> produced the frozen G3 regional portfolio result (`C_region`) within the frozen
> regret bounds, while eliminating essentially all q11 candidate-ranking calls,
> and per-slot selection was separable enough that a per-slot planner is a
> defensible production shape.

A **GO-G4** does **not** mean:

- universal planner generalization;
- anything about V1;
- production Pareto superiority;
- that O11 was optimal;
- that a new compression mechanism was validated;
- that any new ANVIL wire format is authorized.

---

## 13. Explicit non-claims and anti-post-hoc rules

Frozen now, before measurement.

1. **O11 is a reference planner, not an assumed global optimum.** O11 ranks
   candidates in isolation and can be beaten on complete bytes. Any result where
   a proxy beats O11 must be reported as measured, and must **not** be
   reinterpreted as "O11 was buggy" or as a proxy victory claim.
2. **G4 does not validate new compression mechanisms.** No leaf, no transform, no
   backend, and no wire change is tested by G4.
3. **G4 cannot be used to make claims about V1**, because V1 is being opened for
   G3 while this document is frozen, and its outcome is unknown here. V1 is not
   available to G4 for tuning, column selection, gate calibration, or narrative.
4. **Heavy measurements occur on GitHub Actions only.** No discovery measurement
   may be produced on the workstation or any local machine.
5. **`src/anvil.cpp` remains untouched.** G4 has no production wire ID.
6. **All metadata is charged**, and **raw Brotli whole-file fallback is
   retained** in every portfolio as a diagnostic/fallback candidate that is
   excluded from the fidelity numbers (G8 preamble, §G8.9).
7. **G3 discovery observations (§9.1) are context, not results.**
8. **No threshold in this document may be changed after seeing any G4 number.**
   Any change requires a new revision with an explicit reason, and the old
   revision remains the reported one for any already-measured run.
9. **No file may be dropped** from the aggregate because it failed.
10. **No arm may be re-run until it passes.** A rerun for a non-measurement
    reason (hardware flake, CI container error) must be justified and both runs
    reported.
11. **No "closest to passing" proxy may be adopted.** The hierarchy is absolute.
12. **A tie in complete bytes is not evidence of equivalence.** Ties are
    reported; the hierarchy and gates decide.
13. **The near-tie column adjustment must not be used to convert a failure to a
    pass.** It exists only to attribute a pass or failure honestly.
14. **The separability probe is not a search for an improved carrier.** Its
    purpose is to measure interaction magnitude. Reporting its best pair as an
    alternative "better planner" is forbidden; a joint planner must be
    separately preregistered.
15. **The four frozen G3 portfolio masks may not be collapsed.** G4 may not
    report a single mixed carrier as "the" proxy result, and `REGION_RAW` is
    planner-independent by construction.
16. **Family arbitration is out of scope.** The probe (§10) is a candidate-
    interaction diagnostic on `O11 REGION_MIXED` and may not be used to argue
    that `MIXED` (or any other family) is the best portfolio.
17. **An INCONCLUSIVE/INFRA probe is not a relaxed probe.** See §10.8.
18. **No cross-surface score comparison.** S0/S1/S2/O11 candidate score
    magnitudes are not commensurate. No candidate score from one surface may be
    compared to a candidate score from another surface, at any level. In
    particular, an `S3_dictS*`-style per-column fused choice that compares
    cross-surface scores is **prohibited**.
19. **The `DSTAR`/`ISTAR` selection rule is frozen before measurement; the
    labels resolve mechanically after the G8.10/G8.11 measurements and are then
    applied globally.** The frozen order is `S0, S1, S2`. Selecting them by
    observed final S3 bytes is prohibited; per-file switching is prohibited.
20. **S3's finalist set is fixed.** Adding, removing, or substituting a finalist
    after seeing D1-D4 is prohibited (§4.7.4, §G8.12).
21. **S3 is P6 verification, not ranking.** `q11_rank(S3) = 0`; S3 reuses
    already-paid whole-carrier verifier results and reports those reuses separately
    from ranking calls and paid verifier calls (§4.7.4a, §G8.7).
22. **S3 reuses measured verifier results and makes zero additional q11 calls.**
    `s3_additional_q11_calls = 0`. Its finalists' bytes are the **already-measured**
    `(P,F)` / `RAW_BROTLI` results. Reused results must never
    be reported or counted as new q11 calls, and S3 may not recompress a finalist
    to inflate or deflate any count. The paid verifier cost remains
    `final_q11(P) = 4` per proxy per file.
23. **S3's tie order is its own.** §4.7.5 is authoritative for S3 and is not the
    generic §6.3 diagnostic order. The two orders may not be substituted.
24. **S3 does not validate a new compression mechanism**, does not repair a
    failed Q1 proxy, and its artifacts may not be quoted as proxy-fidelity
    evidence.

---

## 14. Relationship to G3 and the G3 oracle

- G4's `O11` reproduces the frozen G3 isolated q11 scoring surface exactly,
  **and reproduces G3's two-level planner architecture**: scores are reused to
  build the four fixed regional portfolios `REGION_RAW`, `REGION_DICT`,
  `REGION_INT`, `REGION_MIXED` (§4.0), and the best of them is the regional
  result (§5.1).
- G4 does **not** change G3's representation, carrier, framing, parser, shapes,
  leaves, or backend.
- G4's `O11 REGION_*` serialized carriers must be **byte-identical to the
  corresponding carriers produced by a separately built frozen-G3 reference**
  from implementation SHA `1a3d18fed76adb6fb33264e1994f9c357306b3fa` on the
  same frozen source object. The CI reference wrapper may expose carrier bytes or
  hashes, but may not change G3 semantics. Require both serialized identity and
  complete-byte equality for every portfolio:

```
carrier_sha256(f,O11,RAW)   == g3_reference_carrier_sha256(f,REGION_RAW)
carrier_sha256(f,O11,DICT)  == g3_reference_carrier_sha256(f,REGION_DICT)
carrier_sha256(f,O11,INT)   == g3_reference_carrier_sha256(f,REGION_INT)
carrier_sha256(f,O11,MIXED) == g3_reference_carrier_sha256(f,REGION_MIXED)

C(f,O11,RAW)   == g3_reference_complete_bytes(f,REGION_RAW)
C(f,O11,DICT)  == g3_reference_complete_bytes(f,REGION_DICT)
C(f,O11,INT)   == g3_reference_complete_bytes(f,REGION_INT)
C(f,O11,MIXED) == g3_reference_complete_bytes(f,REGION_MIXED)
```

  If any serialized-carrier identity or complete-byte equality fails, that is a
  G4 implementation defect and blocks G4 entirely.
- `C(f, P, RAW)` must equal `C(f, O11, RAW)` for every proxy `P`, since
  `REGION_RAW` allows exactly one leaf. A violation is an implementation defect.
- G4 inherits all G3 decoder strictness and adversarial self-tests unchanged.
- G4's oracle is used **only** to label and score planner fidelity. It is never
  proposed as the production planner.
- S3 selects leaves only through the frozen `(P, F)` machinery of §5 and §4.7.1;
  it introduces no new representation, no new leaf, no new wire, and no
  cross-surface score arithmetic.

---

## 15. Implementation order

1. commit and publish this final preregistration revision **before any D1-D4 G4
   discovery measurement**. A tiny synthetic-only prototype already exists as
   disclosed in §0; it creates no discovery evidence and authorizes no gate
   changes;
2. do **not** consult or import any G3 V1 outcome;
3. realign the existing standalone G4 research prototype to this final freeze,
   preserving the frozen G3 representation semantics;
4. implement the four ranking surfaces crossed with the four frozen portfolios
   so that structural analysis, candidate materialization, and carrier
   construction are shared code and only the ranking surface differs;
5. prove the per-portfolio equality invariants of §14 on D1-D4 using the frozen
   G3 prototype's own selections as truth:
   `C(f,O11,F) == frozen G3 REGION_*` for all four `F`, and
   `C(f,P,RAW) == C(f,O11,RAW)` for all `P`;
6. implement S0, then S1 with the exact integer formula and tuple tie-break of
   §4.3/§6.1, then S2;
7. implement the bounded outcome-blind separability probe of §10 on
   `O11 REGION_MIXED`;
8. implement S3 as a pure consumer of already-built `(P,F)` carriers: apply the
   frozen G8.10/G8.11 gates to the measured `C(f,P,DICT)` and `C(f,P,INT)`
   matrices, derive `DSTAR`/`ISTAR` mechanically, assemble the fixed finalist set
   of §4.7.4, and arbitrate by exact bytes with the §4.7.5 tie order;
9. pass all mandatory G3 adversarial decoder tests unchanged;
10. freeze the G4 implementation SHA publicly;
11. create a GitHub Actions discovery workflow pinned to that SHA;
12. run D1-D4 remotely, once per ranking surface, with deterministic call
    counting and exactly four final q11 verifier calls per proxy per file; S3
    performs **no** additional q11 calls — it reuses those measured results
    (§4.7.4a) and reports reused-result counts separately from paid call counts;
13. apply §8 gates, §8.10/§8.11 DSTAR/ISTAR selection, §10.6 gate, §10.7 block,
    and §12 matrix mechanically;
14. write source-of-record results, including every failure;
15. only then consider any follow-up lane (joint planning, or production
    integration of a GO-G4 proxy).

S3 is derived entirely from already-measured `(P,F)` rows. No additional ranking
measurement, no per-file finalist tuning, and no new corpus is introduced by
step 8.

No production wire ID is authorized by this document.

---

## 16. After G4 (not part of G4)

Depending on the ruling, the next lane is exactly one of:

- **GO-G4-X** → production-shaped planner engineering for proxy `X`, with its own
  freeze and its own decode/throughput budget;
- **CONDITIONAL-JOINT** → a separately preregistered **joint planning**
  experiment over the same frozen basis and corpus;
- **CONDITIONAL-SPEED** → a separately preregistered planner-cost reduction lane;
- **NO-GO-G4** → return to representation/anatomy work; do **not** expand the
  typed basis to rescue the proxy hypothesis.

New representation families remain out of scope for all of the above.
