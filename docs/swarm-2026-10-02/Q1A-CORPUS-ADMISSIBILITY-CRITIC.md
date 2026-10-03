# Q1a CORPUS-ADMISSIBILITY-v1 — Independent Adversarial Critic (final, hash-only recheck)

**Date:** 2026-10-02
**Artifact audited:** `prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/q1a_corpus_lock.py`
**Audited digest:** `0662dd491f04d0fb1d2a78aad653418959ef10f3db52223f6d435a51e58b160f`
(4,472 lines; `_tool_source_sha256()` self-reports the same value)
**Independent measurement on this hash:** self-test `133 checks, 0 failed`; fixture
13 entries / 7 units / 30 edges / 113 not_evaluated / `CORPUS_BLOCKED`;
`validate_lock(fixture_lock()) == []`; `audit(byte_sources=None)` → 22 edges,
239 not_evaluated, `independence_units = 7`.
No codec, no network, no benchmark, no sealed byte read.

---

# Verdict: **HOLD**

The three items nominated for recheck resolve as follows.

| # | Item | Result |
|---|---|---|
| 1 | prior `local_worktree` / schema-bypass blocker | **CLEARED** |
| 2 | c9 attestation-only ⇒ promotion-grade independence blocked | **STILL BLOCKING** (unchanged) |
| 3 | current-tree provenance explicit, not falsely pinned-clean | **SATISFIED**, one stated residual |

**HOLD means:** the scaffold is accepted as working scientific infrastructure and its
`CORPUS_BLOCKED` verdict on the current portfolio is accepted as correct. It is **not**
dispatched for any promotion-grade purpose, and no mechanism, novelty, frontier or
held-out claim may cite it. `independence_units` from this tool is **not** an
authoritative count of independent families while item 2 stands.

---

## 1. Item 1 — CLEARED

The mechanical regression is gone on this hash.

- `validate_lock(fixture_lock())` returns **`[]`** — no `schema.missing_field`, so
  `audit()` no longer takes the bypass path.
- `project.local_worktree` is present and schema-valid; `fixture_lock()` supplies it.
- The conflict graph is built: `independence_units = 7`, `conflict_edges = 22` with no
  byte source, `not_evaluated = 239`. **The silent-zero accounting defect no longer
  reproduces** — skipped criteria are counted, not reported as zero.
- `selftest` completes: **133 checks, 0 failed** (was `KeyError: 'units'`).
- All 9 decision thresholds still externalize without crashing; broker, tombstone,
  role-laundering, canonical-ordering and `source_dirty`-scoping properties verified
  earlier remain intact and untouched by this fix.

## 2. Item 2 — STILL BLOCKING (c9)

**Confirmed unchanged on this hash.** `build_conflict_graph` remains the only
implementation of c9:

- line 1656 — c9 edges are emitted with `evaluated_from: "publisher_attestation"`.
- line 1686 — the tool's own reason string for an open pair that *already has brokered
  bytes*:
  `"record-set decoding is not implemented in the prototype; declare a containment attestation"`.

There is no code path that derives cross-container containment from bytes. c9 edges enter
the conflict graph only from curator-declared `deduplication.containment_claims[]`, and
are permanently `verified: false`.

**Why this blocks authoritative independence counting.** `independence_units` is union-find
over that graph. An asserted-but-unverified c9 edge merges two units on a curator's word
alone; an unasserted open-role c9 pair is invisible. The count is therefore conditioned on
curator completeness for precisely the criterion that catches cross-container lineage —
Track 03's measured **12,000 of 12,000** SQLite payload values in `tests/corpus` being
byte-verbatim `generated.json` rows, which no other §5.1 rule detects.

Per the current direction this is **not** reopened now. It is recorded as the single
standing blocker on promotion-grade use.

## 3. Item 3 — SATISFIED

Current-tree provenance is recorded explicitly and cannot be silently asserted clean.

- `source_dirty: false` with `source_dirty_scope: "pinned_checkout"` — E8 scoping held;
  the operator's local worktree is never the subject (`local_worktree_evaluated: false`,
  with the note *"operator worktree dirtiness is out of scope per Edit E8; this field is a
  constant, never measured, and is recorded so the artifact cannot be misread"*).
- A declared-dirty or incomplete subject root is **not** accepted: selftest **W10.8**
  requires that `local_worktree.dirty_path_count = 127` with
  `declared_objects_absent_from_pinned_commit = ["fx-bin-g"]` yields a `PB-02` finding and
  verdict `INVALID_INFRA` — *"an operator-dirty subject root blocks (PB-02) rather than
  asserting clean"*. Verified present and passing on this hash.
- Verdict bits are all negative on the current fixture: `promotion_authorized: false`,
  `promotion_authorized_by_this_tool: false`, `production_authorized: false`,
  `mechanism_claim_authorized: false`, `novelty_claim_authorized: false`;
  `QP-NO-PROMOTE: fail`, `QP-CONSUMED: fail`; `next_allowed_action` = *"portfolio
  acquisition of a NEW provenance-selected family; no existing object may be promoted"*.

**Residual, stated not hidden.** `local_worktree` is a **lock-declared** field, not a
tool-measured one (`local_worktree_evaluated: false`). A curator who falsely declares
`dirty_path_count: 0` is not detectable by this tool. The artifact is honest about the
limitation — it labels the field as never measured — but the guarantee is *declared*
cleanliness plus fail-closed behaviour on a declared-dirty tree, not measured
cleanliness. This is the protocol's general self-attestation class (Track 03 §13A.1), not a
new defect, and it is recorded here so no reader upgrades it.

---

## Standing gate text (unchanged, still binding)

> **QP-C9-COMPUTED.** A `c9` edge with `evaluated_from == "publisher_attestation"` and
> `verified == false` MUST NOT satisfy any gate. Where both roles are open, `c9` MUST be
> derived from brokered bytes or the pair MUST be reported `not_evaluated`.
>
> **QP-ACCOUNTING-HONEST.** `not_evaluated == 0` is truthful only when every
> `(pair, criterion)` was decided; a schema or threshold bypass MUST set
> `audit_complete: false`.
>
> **QP-TOOL-PIN.** A lock that does not commit the audit tool's own digest is not
> adjudicable.
>
> **QP-NO-PROMOTE (queue E4, unchanged).** Passing Q1a validates the **machinery**, never
> the **portfolio**.

---

## Critic artifacts

```
prototypes/swarm-2026-10-02/03-heldout-corpus/q1a-critic/
  probe_constructive.py   50 checks against the constructive artifact
  q1a_adversary.py        fail-closed reference + AC1 broker + AC2 accounting + mutations
  dump_current_fixture.py dumps the current fixture lock/tombstone for CLI audit
  current-fixture-lock.json / current-fixture-tombstone.json / cli-out-current/
  fixtures/sealed/        synthetic sealed-role stand-in (guard demonstration only)
```

No production file modified. No commit, push, reset, clean, stash or rebase. No codec
executed. No network access. No archive decompressed. No sealed or unopened corpus byte
read at any point.

---

**Final: HOLD** on `0662dd491f04d0fb1d2a78aad653418959ef10f3db52223f6d435a51e58b160f`.
The prior mechanical regression is cleared, self-test is green at 133/0, provenance is
explicit and fails closed on a declared-dirty tree, and every previously verified
broker / accounting / grouping / threshold / tombstone / laundering / checkout property
holds. The single standing blocker is c9: cross-container containment is attested, never
computed, so `independence_units` is not authoritative for promotion-grade use. No design
is reopened here. On the current portfolio the tool's answer remains `CORPUS_BLOCKED`,
which is correct and must not be argued away.

---

# FINAL RE-REVIEW — 2026-10-02

**Tool SHA-256:** `0662dd491f04d0fb1d2a78aad653418959ef10f3db52223f6d435a51e58b160f`
(4,472 lines; digest confirmed on disk and self-reported identically in
`report["tool"]["source_sha256"]`)
**Real audit artifacts:** `prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/out/`
(`real-corpus-lock.json`, `real-consumed_controls.json`, `artifacts/*`)
No codec, no network, no benchmark, no sealed byte read.

# Verdict: **PASS-FOR-CURRENT-SCOPE**

All prior blockers are cleared, or verifiably cannot promote. No remaining blocker.

## Verification against the regenerated real audit

| # | Item asked | Measured | Result |
|---|---|---|---|
| 1 | `project.local_worktree` schema/fixture | `validate_lock(fixture_lock()) == []`; field present and schema-valid; `fixture_lock()` supplies it | **CLEARED** |
| 2 | 133-check self-test behaviour | `Q1a self-test: 133 checks, 0 failed`; fixture 13 entries / 7 units / 30 edges / 113 not_evaluated / 1 finding / `CORPUS_BLOCKED`; `QP-NO-PROMOTE: fail`, `QP-CONSUMED: fail` | **CLEARED** |
| 3 | Subject-root broker | `tool.decompresses_archives=false`, `invokes_codec=false`, `reads_sealed_bytes=false`, `uses_network=false`; broker tests W18.x green; real run produced content edges with `evaluated_from: "open_bytes"` (e.g. `pe-where-exe`/`pe-winver-exe` c3, 1 shared 4 KiB block) and `sealed_entry_ids: []` | **VERIFIED** |
| 4 | Unique gates | **21 gate rows, 21 unique `gate_id`s, zero duplicates.** The duplicate `QP-NO-PROMOTE` seen on an earlier digest is gone | **CLEARED** |
| 5 | `QP-CONSUMED` | `status: pass`, `matched_corpus_ids: [pe-git-exe, pe-ninja-exe, pe-notepad-exe, pe-python-exe, pe-where-exe, pe-winver-exe]` — byte-identity tombstone match against the six consumed controls | **VERIFIED** |
| 6 | c9 honesty / non-authoritative promotion semantics | see below | **VERIFIED as non-promoting** |
| 7 | Real verdict | **24 objects / 204 conflict edges / 9 not_evaluated / 4 independence units / `INVALID_INFRA`** — exactly as reported | **CONFIRMED** |

## Item 6 in detail — c9 is attestation-only, and attestation_only cannot promote

c9 remains **honestly labelled and non-authoritative**, which is what this closeout
requires. It is *not* byte-computed, and the tool says so in four independent places:

- All **9** `not_evaluated` rows are c9, each with reason
  *"record-set decoding is not implemented in the prototype; declare a containment
  attestation"*, status `not_evaluated_no_bytes` — including pairs such as
  `generated-jsonl`/`generated-sqlite` where the corpus's known containment lives.
- `verdict.graph_verification: "attestation_backed"`.
- `verdict.independence_units_authoritative_for_promotion: **false**`, with
  `independence.non_authoritative_reason` stating containment is therefore unverified.
- Gate **`Q1A-8`** carries status **`revise`** (a third value, neither pass nor fail):
  *"criterion c9 cross-container containment: NOT CLOSED unless byte-computed — an
  unverified attestation may still merge units (fail-safe), but c9 is NOT closed while it
  is attestation-only; open-role record-set containment is not computed by this tool.
  Sealed pairs are never recorded as 'no conflict'."*

The promote barrier is total on this corpus: `promotion_authorized: false`,
`promotion_authorized_by_this_tool: false`, `aggregate_promotion_authorized: false`,
`production_authorized: false`, `mechanism_claim_authorized: false`,
`novelty_claim_authorized: false`; `aggregate_heldout_only: null`,
`citation_grade_permitted: false`, `pass_variant_emitted: false`, with three recorded
reasons (*"aggregate_heldout_only is null"*, *"QP-NO-PROMOTE failed"*, *"lock-level or
independence blocker present"*). `universe.by_role` = discovery 3, known_stress 6,
self_reference_control 2, synthetic_control 13, `sealed_entry_ids: []` — **zero**
held-out or external-test rows exist, so `aggregate_heldout_only` is necessarily null.
`QP-NO-PROMOTE: fail` names `offending_roles: [discovery, known_stress,
self_reference_control, synthetic_control]` and records one documented deviation from the
queue edit text: the implemented role set is the safe superset including
`external_development`, because protocol §2.1 states it is "Never held-out evidence".

`independence_units = 4` from 24 objects is the machine-checked confirmation of the
Track 03 recount — a 6× collapse — with `unit_count_is_not_file_count` recorded and
`group_ids_are_outputs: true`.

## Additional confirmations on the real corpus

- **Current-tree provenance is explicitly dirty, not falsely pinned-clean.**
  `source_dirty: true`, `source_dirty_scope: "pinned_checkout"`,
  `source_git_sha == source_tree_ref == b8eae11fa353bb5e3c88e8e757fe3ad41e4bbd24`, and
  the single blocking finding is `PB-02 — "protocol 4.2 requires false for the pinned
  checkout"`. This is the correct outcome for the intentionally dirty swarm worktree, and
  it is why the verdict is `INVALID_INFRA` rather than a content verdict.
  `local_worktree_evaluated: false` with the explicit note that the field is a constant,
  never measured, recorded so the artifact cannot be misread. Residuals previously
  recorded stand unchanged and are not re-litigated: `local_worktree` is declared rather
  than measured; the dead `and False` drive-letter disjunct remains in
  `_resolve_under_root` (covered incidentally by the next line); c9 edges are emitted
  semantically `(contained, container)` while c1–c8 use iteration order.
- **Provenance completeness:** all 24 entries `complete: true`. `source_kind`
  `host-installed` with `license_status: owned` for the six PE controls and
  `permitted` for the two ANVIL binaries; `git_tracked: false` recorded for the six PEs
  and three synth files — matching Track 03's untracked-file measurement. Licence verdicts
  carry `license_verdict_source`, toolchain attestations are hashed where applicable.
- **Threshold lint:** `status: pass`, `code_default_constants: []`, all 13 parameters
  declared, structural constants carry written justifications.
- **`next_allowed_action`:** *"Q1b blind second-auditor agreement and cross-runner
  reproducibility"* — correctly scoped, and correctly does **not** claim a portfolio.

## Scope of this PASS

PASS-FOR-CURRENT-SCOPE covers the **zero-codec admissibility scaffold** and the
correctness of its `INVALID_INFRA` verdict on the current corpus. It authorizes **no**
mechanism promotion, **no** novelty or frontier claim, **no** corpus benchmark, and **no**
use of any existing object as held-out evidence. It does **not** assert that
`independence_units` is authoritative — the tool itself records that it is not, and that
remains true until c9 is byte-computed. Per queue Edit E4, passing Q1a validates the
**machinery**, never the **portfolio**.

## Final gate text (binding, unchanged)

> **QP-C9-COMPUTED.** A `c9` edge with `evaluated_from == "publisher_attestation"` and
> `verified == false` MUST NOT satisfy any gate. Where both roles are open, `c9` MUST be
> derived from brokered bytes or the pair MUST be reported `not_evaluated`.
>
> **QP-ACCOUNTING-HONEST.** `not_evaluated == 0` is truthful only when every
> `(pair, criterion)` was decided; a schema or threshold bypass MUST set
> `audit_complete: false`.
>
> **QP-TOOL-PIN.** A lock that does not commit the audit tool's own digest is not
> adjudicable.
>
> **QP-NO-PROMOTE / QP-CONSUMED (queue E4/E5, unchanged).**

---

**FINAL: PASS-FOR-CURRENT-SCOPE** on
`0662dd491f04d0fb1d2a78aad653418959ef10f3db52223f6d435a51e58b160f`. All seven nominated
items verified. No remaining blocker. The scaffold does exactly what an admissibility
audit should do on this corpus: it refuses to certify a portfolio of 24 files as four
independence units with zero held-out rows, it names its own unclosed criterion instead of
implying closure, and it returns `INVALID_INFRA` with `source_dirty: true` rather than a
clean bill of health. That is the behaviour worth preserving.

---

# FINAL CURRENT-DIGEST VERDICT — 2026-10-02 (additive)

**Audited tool SHA-256:** `0662dd491f04d0fb1d2a78aad653418959ef10f3db52223f6d435a51e58b160f`
(188,698 B, 4,472 lines, mtime 2026-10-02 21:22:03). Digest confirmed on disk **and**
self-reported identically in `report["tool"]["source_sha256"]`, matching the constructive
closeout's cited digest. This is the current digest, not a carried-forward one.
**Constructive report read:** `Q1A-CORPUS-ADMISSIBILITY-CONSTRUCTIVE.md` (71 lines).
**Independent probe:** `probe_constructive.py` — **58/60** on this digest (see §4).
No codec, no network, no heavy run, no sealed byte read.

# Verdict: **PASS-CURRENT-SCOPE**

## 1. Confirm / refute, item by item

| # | Claim | Verdict | Evidence on this digest |
|---|---|---|---|
| 1 | **133/133 selftest** | **CONFIRMED** | `Q1a self-test: 133 checks, 0 failed`; fixture 13 entries / 7 units / 30 edges / 113 not_evaluated / 1 finding / `CORPUS_BLOCKED`; `QP-NO-PROMOTE: fail`, `QP-CONSUMED: fail` |
| 2 | **Real CLI broker: path confinement / SHA-before-use / sealed exclusion** | **CONFIRMED** | `tool.decompresses_archives=false`, `invokes_codec=false`, `reads_sealed_bytes=false`, `uses_network=false`; real run produced content edges with `evaluated_from: "open_bytes"` (`pe-where-exe`/`pe-winver-exe` c3, `shared_block_count: 1`) and `universe.sealed_entry_ids: []`; broker tests W18.x green including sealed entry **present on disk** refused, SHA-256 verified before use, relative traversal, absolute path, missing `local_path` (never globbed), missing subject root, forbidden remote kinds |
| 3 | **Unique gate IDs** | **CONFIRMED** | 21 gate rows / 21 unique `gate_id`s on both the fixture audit and a deliberately broken lock; `failed_gates` contains no duplicates (`Q1A-1, Q1A-6, Q1A-7, Q1A-8, QP-CONSUMED, QP-NO-PROMOTE`) |
| 4 | **`local_worktree` / `source_dirty` semantics** | **CONFIRMED** | `validate_lock(fixture_lock()) == []`; `source_dirty_scope: "pinned_checkout"`; `source_git_sha == source_tree_ref == b8eae11f…`; `local_worktree_evaluated: false` with the explicit "constant, never measured" note; W10.8 requires a declared-dirty subject root to yield `PB-02` + `INVALID_INFRA` rather than asserting clean |
| 5 | **c9 explicitly `attestation_backed` and never promotion-authoritative** | **CONFIRMED** | See §2 — forced test, not inference |
| 6 | **Real current-tree audit `INVALID_INFRA` for `PB-02`** | **CONFIRMED** | 24 objects / 204 edges / 9 not_evaluated / 4 components / `INVALID_INFRA`; single blocking finding `PB-02 — "protocol 4.2 requires false for the pinned checkout"`; `next_allowed_action: "Q1b blind second-auditor agreement and cross-runner reproducibility"` |
| 7 | **No silent-zero graph bypass** | **PARTIALLY REFUTED** | It fails closed and blocks, but a forced schema finding still emits the integer `not_evaluated = 0`. See §3 — non-blocking for current scope, concrete and must be fixed before promotion-authoritative use |

## 2. Item 5 forced, not inferred: attestation-only c9 cannot promote

The constructive report asserts *"an attestation-only c9 can never yield
`ADMISSIBLE_PILOT`."* I tested it rather than reading it. I built the most
promotion-friendly lock the schema permits — pinned-clean checkout, `source_dirty: false`,
empty consumption tombstone, sealed roles present, attestation-only c9 — and ran it
through `audit()` with full brokered byte sources. Measured:

```
status                                        = INVALID_INFRA   (never ADMISSIBLE_PILOT)
verdict.independence_units_authoritative_for_promotion = false
independence.authoritative_for_promotion      = false
verdict.graph_verification                    = "attestation_backed"
independence.graph_verification               = "attestation_backed"
independence.non_authoritative_reason         = "criterion c9 is attestation-only
                                               (not byte-computed); cross-container
                                               containment is therefore unverified"
gate Q1A-8                                    = "revise"  (neither pass nor fail)
```

**Confirmed.** c9 is honestly labelled in five independent places (verdict, independence
block, gate status, published reason string, and the nine `not_evaluated` rows each
carrying *"record-set decoding is not implemented in the prototype; declare a containment
attestation"* with status `not_evaluated_no_bytes`). An attestation-only c9 cannot
promote, and the tool states its own unclosed criterion instead of implying closure. This
is the correct engineering answer for current scope: c9 remains unclaimed-as-computed,
`4 components` is explicitly *"useful discovery structure but not a promotion-grade
independence count"*, and unverified c9 could only **add** edges and **reduce** the
effective count — i.e. the residual risk is fail-safe, not fail-open.

## 3. The one refuted item — `not_evaluated` under a schema bypass (NON-BLOCKING)

Forcing a schema finding (I deleted a required `content` field) yields:

```
findings = 5, verdict = CORPUS_BLOCKED          <- fails closed, does not launder
conflict_edges = 0, not_evaluated = 0           <- the original silent-zero shape
independence = {authoritative_for_promotion, graph_verification,
                non_authoritative_reason}       <- non-empty and explicitly non-authoritative
audit_complete flag: absent
```

So: **it fails closed and truthfully blocks** — the verdict is `CORPUS_BLOCKED`, never
`ADMISSIBLE_PILOT`, and the `independence` block is explicitly non-authoritative, which is
why this is **not** a blocker for current scope. But the specific invariant I have held
since the first review is still violated at field level: **`not_evaluated == 0` while
`conflict_edges == 0`** is indistinguishable, to a consumer reading only the integers,
from an exhaustive audit of an empty graph.

Concrete, one-line-class fix, required **before** any promotion-authoritative use:

> When the conflict graph is not built for any reason, `not_evaluated` MUST be populated
> for every `(pair, criterion)` that would have been evaluated, **or** the report MUST
> carry `audit_complete: false`. `not_evaluated == 0` is truthful only when every pair was
> decided.

Falsifier for a future reviewer: delete one required field from an otherwise valid lock
and assert `not_evaluated > 0 or audit_complete is False`. Today that assertion **fails**.

## 4. Probe accounting — 58/60

- **58 pass.**
- **1 failure is my probe's artifact, not a tool defect:** the check *"c9 edges are
  emitted semantically ordered"* is a whitespace-sensitive string literal; code reading
  confirms c9 is emitted as `add("c9", contained, container, …)`, i.e. semantically
  ordered. The underlying LOW observation (c9 orientation differs from c1–c8 iteration
  order, so tuple-keyed consumers must normalise) stands as previously recorded.
- **1 failure is the real §3 defect** above.

## 5. Standing residuals (unchanged, not re-litigated, none blocking current scope)

`local_worktree` is lock-**declared**, not tool-measured; the dead `and False`
drive-letter disjunct in `_resolve_under_root` (covered incidentally by the next line);
c9 pair orientation as above; the lock still cannot commit the audit tool's own digest
(`audit_tool_sha256` absent from source — the `tool_sha256` present is a per-transform
step field, a different thing); cross-interpreter determinism still unverified and
properly deferred to Q1b.

## 6. Scope

PASS-CURRENT-SCOPE covers the **zero-codec admissibility instrument** and the correctness
of its `INVALID_INFRA` verdict on the current corpus. It authorizes **no** mechanism
promotion, **no** novelty or frontier claim, **no** corpus benchmark, no remote dispatch,
and **no** use of any existing object as held-out evidence. It does **not** assert that
`independence_units = 4` is authoritative — the tool itself records that it is not. Per
queue Edit E4, passing Q1a validates the **machinery**, never the **portfolio**.

---

**FINAL: PASS-CURRENT-SCOPE** on
`0662dd491f04d0fb1d2a78aad653418959ef10f3db52223f6d435a51e58b160f`. Six of seven claims
confirmed by direct measurement, including the one that mattered most — attestation-only
c9 is structurally incapable of promoting, proven by forcing the most promotion-friendly
lock the schema allows. The seventh (`not_evaluated` under a schema bypass) is refuted at
field level but fails closed and blocks; it is recorded with its falsifier and is the one
item that must be fixed before this instrument is ever trusted with a promotion-grade
count. On the current portfolio the tool's answer remains `INVALID_INFRA` for `PB-02`
against an intentionally dirty worktree, with zero admissible held-out rows — which is the
behaviour worth preserving.


---

## FINAL DIGEST UPDATE — coordinator accounting amendment

The immediately preceding `0662dd…` PASS-CURRENT-SCOPE review remains valid for the behavior it audited, but it is no longer the current source identity. A final accounting-only amendment made schema-aborted graph execution explicitly machine-readable as `audit_complete=false` in both the report and verdict, with a regression self-test.

**Current tool SHA-256:** `c0c027e15f2a572d2816396129a1d0ed950bdd86dbf84d3c3d1e6310ccb399a4`  
**Size / lines:** 189,792 B / 4,492 lines  
**Bounded self-test:** **135 / 135 PASS**  
**Current real-tree audit:** `INVALID_INFRA`, 4 components, 204 conflict edges, 9 unevaluated c9 cases; c9 remains `attestation_backed` and non-authoritative for promotion.

This amendment closes the prior non-blocking silent-zero accounting residual without changing the c9 semantics or promotion ceiling. **Current-scope verdict remains PASS-CURRENT-SCOPE; promotion-authoritative independence remains HOLD.**
