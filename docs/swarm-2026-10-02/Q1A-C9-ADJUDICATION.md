# Q1a — `c9` cross-container containment adjudication

**Date:** 2026-10-02 · **Track:** q1a-corpus-lock / space-bunny
**Verdict: REVISE** (substantive, one criterion; plus six smaller defects that the same patch closes)

## 0. Pins, method, prohibitions

**Digest adjudicated.** `prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/q1a_corpus_lock.py`
= `c0c027e15f2a572d2816396129a1d0ed950bdd86dbf84d3c3d1e6310ccb399a4` (189,792 B, 4,492 lines).
This digest is a **moving target** — the constructive worker wrote it at 21:29:43 while this
adjudication was running, over `0662dd491f04d0fb1d2a78aad653418959ef10f3db52223f6d435a51e58b160f`
(188,698 B) and `29cc3ca44e9ccde8bfce971726a867bd3172207e006bc2d9c2fe95ad39d98757` (185,578 B,
the critic's digest). **Three separate digests were live during this session.** Every line
reference below is against `c0c027e1`; re-verify before applying the patch contract.

Companion: `build_real_lock.py` = `cd200f22f9a2ec8cd0b29b4cf9f21eb45c770fd001a555d7dacacde7c58aaa1d`
(21,031 B).

Real artifacts (all emitted 21:18:07 from `out/`): `real-corpus-lock.json`
`936f5fd30384079ed44c1dd679a605954f6e77926edd013691fca92defd06c18` (53,113 B), 24 entries,
all artifacts present.

**Prohibitions honored.** No edit to `q1a_corpus_lock.py` (worker active, per instruction).
No edit to any other prototype or production file. No codec, no network, no archive
decompression, no sealed byte. No git operation. Every corpus byte read below is an
**already-open `tests/corpus` file** (protocol §2.3 already-consumed discovery/synthetic
data), read through the tool's own `broker_open_role_bytes` where an audit was run. This
document is the only write.

**Evidence labels.** `[PROVEN]` — follows from the cited source text. `[MEASURED]` — measured
here, with the exact command shape reproducible from the pinned digest. `[DERIVED]`.

---

## 1. What c9 is *required* to mean

Five normative sources constrain this, and they agree.

| Source | Text | Normative force |
|---|---|---|
| `docs/I10-CORPUS-LOCK-PROTOCOL.md` §5.1 (`:323-338`) | The lock MUST include a **complete pairwise audit over all entries**. Eight blocking rules, none of which is containment. "Text normalization for duplicate detection is limited to CR-before-LF removal, Unicode NFC, whitespace collapse, and lowercase ASCII alphanumeric tokenization. **It MUST NOT be used to create measured bytes.**" | binding; note the eight-rule list is **closed** |
| `REMOTE-EXPERIMENT-QUEUE.md` Edit E6 (`:278-286`) | Q1a must "compute transitive union-find independence groups over the §5.1 conflict graph; emit `conflict_edges[]` and `independence_units`; **criterion c9 cross-container containment**; threshold lint; provenance completeness. **Group ids are an output; curator-supplied group ids are rejected.**" | binding |
| `SYNTH-CORPUS-FLEDGE.md` §8.1 (`:233-236`) | Gate 2: "**Independence units are computed, not asserted** … the tool must have no curator-supplied group ids." Gate 3: `conflict_edges[]` must include "the **measured** `generated.sqlite`⊂`generated.json` containment **[M]** — neither waived." Gate 4: "Cross-container containment (criterion c9) **implemented** — the fixed-4 KiB rule **provably cannot** see it **[M]**." | binding; "**measured**", "**implemented**" |
| `SYNTH-CORPUS-FLEDGE.md` §9.4 (`:257`) | "**`generated.sqlite` does not contain `generated.json`'s rows.** Falsifier: any non-verbatim payload. **Measured 12,000/12,000 verbatim [M]**." | the falsifier is stated; it has never been re-measured by the tool |
| `Q1A-CORPUS-ADMISSIBILITY-CRITIC.md` §BLOCKER 2 (`:66-104`, `:137-141`) | gate text **QP-C9-COMPUTED**: "A `c9` edge with `evaluated_from == "publisher_attestation"` and `verified == false` MUST NOT satisfy any gate. **Where both roles are open, `c9` MUST be derived from brokered bytes or the pair MUST be reported `not_evaluated`**; an audit containing unresolved open-role c9 pairs MUST NOT be presented as an independence verdict." | binding |

**The exact required semantics, distilled:**

> **c9(A→B) fires iff the decoded record set of A is contained in the decoded record set of B,
> where both A and B are open-role entries whose bytes were brokered by the guard.** The
> relation is **directed** (contained, container). It is a **merge-only, monotone** relation:
> firing can only *reduce* `independence_units`. When either side is sealed, or either side's
> bytes were not brokered, or a container framing cannot be decoded by a **pinned,
> declared-in-lock** decoder, the pair MUST be reported `not_evaluated` with a reason — never
> as clean. A curator attestation may substitute for the computation **only** where a side is
> sealed, MUST stay `verified: false`, and MUST NOT satisfy any gate.

Four properties are load-bearing and each is independently falsifiable:

1. **Mechanical, not curator-controlled** (queue E6 + Fledge gate 2). The unit count may not
   be conditioned on curator completeness for the criterion that catches cross-container
   lineage. This is the doctrine the adjudication turns on.
2. **Complete over all pairs** (§5.1 "complete pairwise audit"). A pair that was neither
   computed nor reported `not_evaluated` is a silent-zero defect; the tool already has a
   gate id for the general form (`Q1A-8`) and an `audit_complete` flag for the schema-bypass
   form.
3. **Merge-only.** False positives cost nothing but conservatism. False negatives inflate
   the independent-family count. This asymmetry is why the design below deliberately biases
   toward firing and accepts a bounded FP surface (§5).
4. **Byte-derived, hash-pinned evidence.** "Measured", per Fledge gates 3-4, means the
   artifact carries a value computed from the bytes in this run, not a string copied from a
   Markdown table.

---

## 2. The current implementation, on the real Q1 artifacts

### 2.1 The one c9 edge is an assertion, and its "measured" digest is a self-reference

`[MEASURED]` on `out/artifacts/conflict-edges.json` (204 edges; c7=126, c6=38, c8=38, c3=1,
**c9=1**):

```json
{"a":"generated-json","b":"generated-sqlite","criterion":"c9",
 "criterion_meaning":"cross-container containment (one entry's record set inside another)",
 "evaluated_from":"publisher_attestation",
 "evidence":{"attestation_kind":"project-doc-measurement",
             "evidence":"docs/swarm-2026-10-02/SYNTH-CORPUS-FLEDGE.md 1.3 (measured 12000/12000)",
             "record_set_sha256":"a51b59b1f9b7d68c757ce50d65049171651a6a2af06bf973890782442f0a9c94"},
 "verified":false,"waived":false}
```

`record_set_sha256` is the digest of a **prose sentence about the claim**, not of any record
set. Reproduced exactly `[MEASURED]`:

```
q.canon_sha256({"claim": "generated.sqlite embeds generated.json"})
  == a51b59b1f9b7d68c757ce50d65049171651a6a2af06bf973890782442f0a9c94
```

`build_real_lock.py:302-311` constructs it that way. The field is named `record_set_sha256`
and the schema constrains it to `HEX64_RE` (`:821`), so a reader has no way to know it is not
a record-set digest. This is worse than "unverified": it is **evidence-shaped and
self-referential**, and it is the *only* c9 evidence the portfolio will ever carry.

### 2.2 Nine open-role pairs with brokered bytes are reported `not_evaluated_no_bytes`

`[MEASURED]` running the pinned digest against the real lock with
`broker_open_role_bytes(lock, "<repo>", guard)` — 24 open entries brokered, `open_byte_reads`
48, all four zero-exposure counters 0:

```
verdict: INVALID_INFRA          audit_complete: True
independence_units: 4           graph_verification: "attestation_backed"
c9 edges: 1  -> ('generated-json','generated-sqlite','publisher_attestation',verified=False)
c9 not_evaluated: 9, every one with
   status = "not_evaluated_no_bytes"
   reason = "record-set decoding is not implemented in the prototype; declare a containment attestation"
Q1A-8: revise      failed_gates: Q1A-2, Q1A-8, QP-NO-PROMOTE
```

The reason string is **factually false on this run**. All nine pairs have brokered bytes;
`status: not_evaluated_no_bytes` asserts the opposite. That is a labeling defect in the one
artifact whose whole job is to be honest about what was not decided — and it is exactly the
silent-zero shape the critic's **QP-ACCOUNTING-HONEST** gate names. `[PROVEN]` the string is
hard-coded at `:1686`, unreachable except as a literal.

### 2.3 Blockers 1 and 2 from the critic: status on the pinned digest

| Critic item | Status on `c0c027e1` | Evidence |
|---|---|---|
| **B1a** fixture omits `project.local_worktree` | **FIXED** | `audit_complete` exists (`:2301`, `:2784`, `:2863`); `W4.1f` (`:3797`) asserts `audit_complete is False` + `verdict.audit_complete is False` + `status != ADMISSIBLE_PILOT`. `[MEASURED]` deleting `project.local_worktree` from `fixture_lock()` → `verdict CORPUS_BLOCKED`, `audit_complete False`, `Q1A-6 blocked`, `Q1A-1 fail`. |
| **B1b** bypass may report `not_evaluated: 0` | **FIXED for the schema path only** | `Q1A-8` is a `revise` status, and `revise` is in `failed_gates` (`:2752`), so `ADMISSIBLE_PILOT` is unreachable. But see §2.2: the *c9* `not_evaluated` statuses are mislabeled, so the accounting-honesty property is not yet met for this criterion. |
| **B2** c9 attestation-only | **NOT FIXED** | `sqlite3` count in source: **0**. `containment_min_records`: **0**. No decode path exists. §4 below is the patch. |
| self-test | **RUNS, 135 checks, 0 failed** | `[MEASURED]` `python q1a_corpus_lock.py selftest` → `exit=0`; self-reported `tool source_sha256` = the pinned digest. |
| verified properties (sealed firewall, broker, PB-11, tombstones, threshold lint, checkout scoping, canonical ordering) | unchanged | out of scope for re-litigation per critic `:186-189` |

### 2.4 Six further defects, all in or adjacent to the c9 channel

`[MEASURED]` against `fixture_lock()` + `fixture_byte_sources()` on the pinned digest.

**D1 — a containment claim naming an unknown corpus id crashes the audit.** Not a finding, an
uncaught `KeyError`:

```
claim contained="nope" container="fx-sqlite"   ->  CRASH KeyError: 'nope'   (both in
   build_conflict_graph and in audit(); no artifact is emitted, no verdict, no ledger row)
```

`:1631-1635` indexes `by_id[contained]` / `by_id[container]` before any existence check. Every
other curator-supplied identifier in this tool is schema-checked. **Fail-open by omission**: the
auditor dies instead of emitting `CORPUS_BLOCKED`.

**D2 — a self-pair claim is accepted as an edge.** `contained == container == "fx-json-a"`
yields a c9 edge `('fx-json-a','fx-json-a')`.

**D3 — a false claim merges two unrelated entries into one unit.** Claim
`fx-text-e ⊂ fx-sqlite` (no content relation whatsoever) drops `independence_units` from **8 to
7**. The merge is fail-safe *in direction* but it is **unaudited**: the unit count moves on a
proposition the tool never checked and has no mechanism to check. This is the doctrine failure
made concrete — and note the merge is **invisible in the verdict**: `blocking_findings` is
empty for that claim and the gate says `revise`, which is the same status the tool reports when
c9 is merely *incomplete*.

**D4 — duplicate claims double-count.** Two identical claims → two c9 edges for one pair.
`edges.sort` (`:1692`) does not deduplicate; union-find is idempotent so the unit count is
unaffected, but `conflict_edges[]` is a first-class published artifact (Fledge gate 3) and now
over-reports.

**D5 — the `(a,b)` ordering asymmetry corrupts the PB-11 subset check.** c1-c8 emit
`(a,b)` in sorted iteration order; c9 emits `(contained, container)` semantically (`:1648-1650`).
`computed_pairs` at `:2337` is built from the emitted tuple **unnormalized**, while the curator
annotation lookup at `:2342` uses `tuple(sorted(...))`. So when the contained id sorts *after*
the container id, a **truthful** curator annotation of a c9-detected duplicate is rejected:

```
claim fx-text-f ⊂ fx-arch-m   (fx-text-f > fx-arch-m lexicographically)
fx-text-f annotates ["fx-arch-m"]
  -> c9 edge ('fx-text-f','fx-arch-m'); PB-11 "annotated duplicate 'fx-arch-m' has no
     computed conflict edge"; Q1A-7 fail; verdict CORPUS_BLOCKED
```

The critic filed this as **minor** (`:126-131`). On the pinned digest it is a **blocking
false positive**: a curator who correctly declares a real duplicate is blocked by PB-11 for
declaring it. That inverts protocol §5.1's "A duplicate match MAY be adjudicated only by placing
both objects in the same independence group."

**D6 — `dedup-report.json` filters c9 out of `edges`.** `ARTIFACT_FILES` (`:3087-3100`) emits
`[e for e in r["conflict_edges"] if e["criterion"] in ("c1","c2","c3","c4","c5")]`. The
protocol §5.3 mandated artifact `dedup-report.json` therefore **never shows the containment
finding**, while its `not_evaluated` array *does* carry c9 rows. Verified on the real artifacts:
`out/artifacts/dedup-report.json` has `edges: [the c3 pe-where/pe-winver edge]` only, and
`not_evaluated: [9 c9 rows]`. An auditor reading the mandated dedup artifact sees zero c9 edges
and nine c9 unknowns — a state the tool can never report, because the graph knows about the
attested edge.

---

## 3. Doctrine ruling: does publisher-attestation-only c9 satisfy "mechanically derived"?

**No. It fails, and the failure is not a nuance of wording.**

The argument, stated as tightly as the sources allow:

1. `independence_units` is a union-find over the conflict graph (`:1702`, `:2308`). Group ids
   are outputs; curator-supplied ids are structurally rejected. All of that machinery is
   sound and verified.
2. But union-find is **only as complete as its edge set**. The edge set is
   `computed(criteria) ∪ asserted(containment_claims)`.
3. For c1-c8 the edge set is computed. For **c9** it is computed **only on the subset of pairs
   a curator chose to name**. Nine of the ten open structured/container pairs on the real
   corpus are named by nobody and are computed by nothing (`§2.2`).
4. Therefore the published `independence_units: 4` is `f(entries, bytes, curator completeness)`
   — not `f(entries, bytes)`. It is conditioned on exactly the variable the doctrine forbids,
   for exactly the criterion that exists because content-block and token-similarity rules
   provably cannot see cross-container lineage (Fledge gate 4: "**provably cannot**"; Fledge
   §13B.2: zero shared 4 KiB blocks, MinHash ≤ 0.0074).
5. `GATE-INDEP-1/2` are the *only* gates that make `independence_units` load-bearing. A count
   that is conditioned on curator completeness cannot be an input to a promotion gate. Fledge
   §10 (`:260`) states the symmetry explicitly: "**If `GATE-INDEP-1/2` are not added, a passing
   LOCK-V1 is not evidence that the corpus is clean.**" The same argument applies one level
   down: if c9 is not computed, a passing unit count is not evidence of an independent count.

**What the current code gets right, and must not be lost.** The honest-negative machinery is
already in place and is genuinely good: `Q1A-8` refuses to call c9 closed
(`c9_status == "byte_computed"` requires *every* c9 edge `verified is True`, `:2365-2371`);
`verdict.status` cannot reach `ADMISSIBLE_PILOT` unless `c9_status == "byte_computed"`
(`:2757`); `independence.graph_verification == "attestation_backed"` and
`authoritative_for_promotion == False` are published (`[MEASURED]` on the real lock). The tool
**cannot currently lie into a promotion**. That is worth stating plainly, because it changes
the severity from "unsafe" to "not authoritative": the current portfolio verdict is
`CORPUS_BLOCKED`/`INVALID_INFRA`, and it stays there after the patch.

**Ruling.** Publisher-attestation-only c9 is acceptable **only** as the sealed-side substitute
that the critic's QP-C9-COMPUTED text already describes. As the *sole* implementation for
open-role pairs it does not satisfy the doctrine, and the difference is not repairable by
better labeling, gate text, or `graph_verification` flags — those make the incomplete status
**legible**, which is necessary and is already achieved, but the count itself remains
conditioned on curator completeness.

---

## 4. Minimal deterministic exact computation for OPEN roles

Design constraints, in priority order: exact (no similarity, no threshold on content);
deterministic (no wall clock, no hash-order, no set-iteration in output); standard-library
only; no network, no codec, no archive decompression, no sealed byte; fail-closed.

### 4.1 Record model

A **record** is one addressable unit of an entry's decoded content. Per framing:

| Framing | Detection (bytes only) | Record |
|---|---|---|
| `json-document` | leading non-space byte ∈ `{` `[` **and** the whole payload parses as JSON | one record per top-level array element; the whole document if it is an object |
| `ndjson` | ≥1 non-empty line, **every** non-empty line parses as JSON | one record per line |
| `log-lines` | ≥1 non-empty line, not all lines parse as JSON | one record per line |
| `sqlite` | first 16 B == `b"SQLite format 3\x00"` | one record per **cell value**, every table, every column |
| `none` | no non-empty line and no JSON prefix | no records; c9 vacuously false for this entry, **not** `not_evaluated` |

Framing is **derived from the bytes**, never read from `entry.validity.framing`. That field is
curator-supplied and a curator who mislabels `sqlite` as `external_mixed` is precisely the
failure 13B.2 describes.

SQLite table/column enumeration is name-ordered and double-quote-escaped; cell values are read
via `CAST(col AS BLOB)` so an `INTEGER`/`REAL`/`TEXT` value reaches the canonicalizer as exact
stored bytes with **no float re-formatting** — this is what makes the computation immune to
Python-version float `repr` drift. `NULL` cells canonicalize to the single token `n`.

### 4.2 Canonicalization

Two-level, and the level tag is inside the digest so cross-level collisions are impossible:

```
canonical_value(v):
  bool   -> b"b1" / b"b0"
  int    -> b"i" + str(v)                        # exact decimal, arbitrary precision
  float  -> b"f" + struct.pack("<d", v).hex()     # exact IEEE-754 bits, no repr()
  null   -> b"n"
  str    -> b"u" + str(len(utf8)) + b":" + utf8
  bytes  -> b"x" + str(len) + b":" + raw
  list   -> b"a[" + concat(canonical_value(x)) + b"]"     # order-significant
  object -> b"o{" + sorted-by-key (len-prefixed json key + "=" + canonical_value(v)) + b"}"
```

`canonical_record(raw_bytes)` tries `json.loads(raw.decode("utf-8"))` **once**, under
`object_pairs_hook` that rejects duplicate keys and `parse_constant` that rejects
`NaN`/`Infinity`; on success the record is `canonical_value(parsed)`, on any exception the
record is `b"x" + len + b":" + raw`. Two independent layers therefore agree on JSON-valued
content regardless of whitespace, key order, or scalar spelling, and no input shape can raise
out of the function.

The entry's **record set** is
`R(X) = { sha256(canonical_record(rec)).hexdigest() : rec ∈ records(X) }`, and its published
digest is `sha256(canon(sorted(R(X))))` — `sorted()` so the set's iteration order can never
reach the artifact. The suffix-free hex64 form is directly comparable to the existing
`record_set_sha256` field, so the field becomes meaningful rather than decorative.

### 4.3 The rule

For each unordered open-role pair `(A, B)` with `R(A)`, `R(B)` both computable:

```
c9 fires for (X -> Y)  iff   R(X) ⊊ R(Y)          # strict, so R(X) != R(Y)
                        and  |R(X)| >= containment_min_records
                        and  |R(X)| >= 2
```

Direction is decided by the subset relation, so the edge is **unique** — `(A,B)` and `(B,A)`
cannot both fire (that would require `R(A) = R(B)`, excluded by strictness). No tie-break, no
curator choice, one canonical answer. `containment_min_records` is a **new required
`audit_parameters` field** so the threshold lint keeps its property: no decision threshold in
a code default.

### 4.4 Fail-closed

| Condition | Outcome |
|---|---|
| either role in `SEALED_ROLES` | `not_evaluated_sealed`, reason cites Edit E7 point 1. **Never** decoded. Unchanged. |
| broker refused / bytes absent for either side | `not_evaluated_no_bytes` — **and this status is now reachable truthfully** because it is emitted only when the broker genuinely had nothing |
| bytes present, framing is `none`, or `|R(X)| < 2` | c9 **not evaluated** for the pair, `not_evaluated_no_records`, `waived: false`. Recorded, never clean. |
| bytes present, framing decodable, `|R(X)| < containment_min_records` | c9 **not evaluated**, `not_evaluated_below_min_records`. Recorded. (The alternative — emitting a negative — would report "no containment" for a pair too small to decide.) |
| sqlite present but unreadable (bad page, unsupported, decode raise) | `not_evaluated_no_decoder`, **never** `clean`. The canonicalizer's total-function property (§4.2) means this is reachable only for a genuinely malformed container. |
| attestation present on a pair the computation also decides | one edge, `evaluated_from: "open_bytes"`, `verified: true`; the attestation is retained in `evidence.attestation` and **cross-checked**: `record_set_sha256` must equal the computed digest or a `C9.ATTEST_MISMATCH` finding fires |
| attestation present on an open pair with **no** containment | `C9.ATTEST_UNSUBSTANTIATED` finding, edge still emitted as a merge (fail-safe), `verified: false`, `graph_verification` → `attestation_backed`, `ADMISSIBLE_PILOT` unreachable |

`Q1A-8` promotes to `pass` **only** when every c9 edge is `verified is True`
**and** the `not_evaluated` list contains zero `criterion == "c9"` rows. A `revise` status
already forces `CORPUS_BLOCKED` (`:2752`).

### 4.5 New exposure counters

`sqlite_record_reads`, `sqlite_databases_decoded` — incremented in `derive_record_set`, asserted
`0` never. `sqlite3` joins the import allow-list explicitly: it must **not** be added to
`_FORBIDDEN_IMPORTS` (`:3019`) and **must** be named in the `tool` block of every artifact
(`invokes_codec: false, uses_network: false, decompresses_archives: false,
reads_sealed_bytes: false` — all four already emitted at `:2784-2792`). The module must assert
in-source that `sqlite3.connect` is called **only** with `":memory:"` and that `deserialize()`
is the only ingestion path — a path-based `connect()` would be an unguarded filesystem read and
a broker bypass. `ExposureGuard.open_bytes` already increments `open_byte_reads` for these
bytes (48 = 24 entries × 2 in the current run), so no new read path is introduced.

---

## 5. False-positive / false-negative boundaries, measured

### 5.1 The target, reproduced independently `[MEASURED]`

On `tests/corpus` with §4.1-4.2 as specified:

```
generated.json                 framing=json-document  records= 7,000  distinct= 7,000
generated.jsonl                framing=ndjson          records=12,000  distinct=12,000
generated.repeat.jsonl         framing=ndjson          records= 4,000  distinct=     1
generated.sqlite               framing=sqlite          records=48,000  distinct=19,803
synth-ndjson-columnar.ndjson   framing=ndjson          records=11,000  distinct=11,000
generated.log                  framing=log-lines       records=18,000  distinct=18,000

c9: generated.json -> generated.sqlite      (|R|=7,000  ⊊  19,803)
```

**Exactly one c9 edge across the corpus**, correct direction, in **1.23 s** total for all six
entries. Two independent framings of the same SQLite ingestion — native Python types with
`struct.pack` floats, and `CAST(col AS BLOB)` byte-level — both yield the same verdict
(`distinct` 19,803 native / 19,803 blob-level; 19,703 under integral-float folding, which also
yields exactly one edge). The byte-level form is specified because it has no float-`repr`
dependency.

The 12,000/12,000 claim is confirmed at source: `make_smoke_corpus.py:31` inserts
`json.dumps(rows[i % len(rows)], separators=(",", ":"))` for `i in range(12000)` — so
`generated.sqlite`'s `payload` column is 12,000 cells over the same 7,000 distinct values.
`[MEASURED]` `12000/12000` payload cells byte-equal a `generated.json` row; multiplicity
profile `[1, 2]`; 5,000 rows beyond the first 7,000 reuse an earlier value. **Set containment
`R(json) ⊊ R(sqlite)` holds, and multiset containment holds too** — the finding is robust to
either reading of "record set", so the patch does not have to pick.

Negative controls, all `[MEASURED]` correct (no spurious c9): `generated.json ⊄ generated.jsonl`,
`⊄ synth-ndjson-columnar.ndjson`, `generated.jsonl ⊄ generated.sqlite`. Fledge §13B.2's claim
that fixed 4 KiB blocks cannot see this is consistent: the only c3/c4 edge in the whole corpus
is `pe-where`/`pe-winver`.

### 5.2 False negatives — stated, not hidden

| FN class | Status | Note |
|---|---|---|
| **Field-superset records.** Container stores A's record *plus* an extra field. | **FN, by construction** | `[MEASURED]`: a superset record does not canonicalize equal and is correctly **not** contained. A project that widens a schema on the way into a DB defeats c9 by design. This is the single most important documented limit. Fledge's 12,000/12,000 is byte-identical payloads, so it is unaffected. |
| **Numeric re-spelling** (`1` vs `1.0` vs `1e0`) | partially closed | Exact `struct.pack` bits distinguish them → FN. An **integral-float folding** variant closes it (19,803 → 19,703 distinct; still exactly one edge) at the cost of treating `1` and `1.0` as one record. Recommend the exact form; note the fold as a pre-registered option. |
| **Reordering / nesting.** Records spread across tables, columns, or a nested array. | covered for tables/columns (cell-level); FN for nested arrays | cell-level extraction is what catches the payload-in-a-column shape that matters here |
| **Duplicate keys in a JSON object** | closed by `object_pairs_hook` | two byte-different objects that differ only in duplicate-key resolution are treated as opaque bytes → FN, deliberately conservative |
| **`NaN`/`Infinity`** | closed by `parse_constant` | rejected → opaque bytes → FN |
| **Lone surrogates** | closed | `[MEASURED]` `json.dumps("\ud800", ensure_ascii=False).encode("utf-8")` raises `UnicodeEncodeError`. The `try/except` must wrap **both** the parse and the canonicalization — a top-level-array path that canonicalizes without a guard crashes on `["\ud800"]` (`[MEASURED]`). This is a live crash risk in a naive patch; the contract's total-function requirement covers it. |
| **Sealed pairs** | `not_evaluated_sealed` forever | correct by doctrine (E7); the cost is permanent, and is disclosed in the artifact rather than hidden |
| **Equal record sets, different containers** | no c9 edge | strict subset excludes equality; c1/c3 already cover byte-identical, and *record*-equal-but-byte-different is a real gap worth naming |
| **Non-JSON log similarity** | not c9's job | c5 MinHash; c9 is exact-containment only |

### 5.3 False positives — bounded, and biased safe

| FP class | Bound | Note |
|---|---|---|
| **1-record entry trivially contained** | closed by `|R(X)| >= 2` and `containment_min_records` from the lock | `[MEASURED]`: `sqlite3` exposes 19,803 scalar-only records, and `int 1`, `float 0.0`, `str "alpha"` are all in it. A 1-record JSON entry could otherwise fire spuriously. |
| **Many tiny records in a scalar column** | bounded by `containment_min_records` | the residual risk is a ≥N-record structured entry whose every record happens to appear as a SQLite scalar. Acceptable: the direction is merge-only, so a FP can only lower `independence_units`. |
| **Template-generated siblings** (`generated.repeat.jsonl`, distinct = 1) | closed by the `>= 2` floor | `[MEASURED]` distinct=1 over 4,000 records; under `containment_min_records` it cannot fire as the contained side |
| **Boilerplate lines in two logs** | bounded, merge-only | two files sharing a small boilerplate line set do not fire c9 (whole-line records, and `containment_min_records`), but this is the class most likely to FP at low `containment_min_records`. Declare it high. |
| **Cross-framing scalar aliasing** | accepted | a log line `1` and a SQLite `INTEGER 1` canonicalize identically (`i1`). Both are the same value. |

**Asymmetry, stated plainly:** a c9 FP costs conservatism (fewer independent units, harder to
promote, never a false "clean"). A c9 FN costs *soundness* — it inflates the count the whole
corpus protocol exists to defend. Every design choice above resolves an ambiguity **toward
firing**, and every unresolved pair is emitted as a labeled `not_evaluated` row rather than
resolved silently.

---

## 6. Minimal patch contract

Ordered. Each item is independently verifiable; item 1-4 are the substance, 5-10 close the
defects from §2.4 so the patch cannot regress them. Target file:
`prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/q1a_corpus_lock.py`.

**0. Freeze.** The constructive worker must land first; re-derive line numbers against the
final digest. Three digests were live in one session.

**1. `sqlite3` + `struct` imports** (`:67-74`). Neither is in `_FORBIDDEN_IMPORTS` (`:3019`) and
neither may be added. Add a self-test asserting every `sqlite3.connect` argument in the AST is
the literal `":memory:"`, and that `deserialize` is the only ingestion path.

**2. `containment_min_records`** — add to `AUDIT_PARAM_OBJ` (`:880-899`) as `field(INT, ge=2)`.
It joins `REQUIRED_AUDIT_PARAMETERS` automatically, so `threshold_lint` (`:1940`) keeps its
property and an undeclared value is a `THRESH.undocumented` finding. Update
`fixture_lock()` (`:3312`) and `build_real_lock.py` (`:355-369`) to declare it. **No code
default.**

**3. `derive_record_set(data) -> (framing, record_set, record_count, distinct_count)`** — new
module function per §4.1-4.2. Requirements that are not optional:
- total function: no input raises. Wrap parse **and** canonicalize in one `try/except`;
  `parse_constant` rejects non-finite; `object_pairs_hook` rejects duplicate keys.
- floats via `struct.pack("<d", v).hex()`; never `repr`, never `%g`, never `str(float)`.
- SQLite cells read as `CAST(col AS BLOB)`; `NULL` → `n`; identifiers double-quote-escaped;
  table and column enumeration name-ordered.
- record set = `sorted({sha256(...)})`; published digest `sha256(canon(sorted_set))`.
- increments `sqlite_databases_decoded` / `sqlite_record_reads`.

**4. c9 evaluation in `build_conflict_graph`** (replaces `:1623-1691`). For each unordered pair
of open-role entries whose classes admit a record framing: compute both record sets, apply §4.3,
emit `evaluated_from: "open_bytes"`, `verified: true`. Retain the attestation loop for sealed
sides and cross-check attestations against computed digests (mismatch →
`C9.ATTEST_MISMATCH`; uncomputable → `C9.ATTEST_UNSUBSTANTIATED`). Reject unknown ids
(`C9.CLAIM_UNKNOWN_ID`), self-pairs (`C9.CLAIM_SELF_PAIR`), and duplicate pairs. Emit
`not_evaluated` with the **specific** status per §4.4. **Fix the reason string at `:1686`** —
`"record-set decoding is not implemented in the prototype; declare a containment attestation"`
is false the moment bytes exist and must not survive.

**5. `Q1A-8`** (`:2372-2390`): `pass` iff every c9 edge is `verified is True` **and** zero
`not_evaluated` rows have `criterion == "c9"`. Add the counts to `evidence`.

**6. `dedup-report.json`** (`:3087-3100`): the `edges` filter must include `"c9"`. A mandated
artifact may not omit the criterion whose gate text exists.

**7. Pair normalization for PB-11** (`:2337`): build `computed_pairs` from
`tuple(sorted((e["a"], e["b"])))`. Fixes D5. Emitting c9 `(a, b)` sorted with the direction
carried in `evidence` (`contained_corpus_id`, `container_corpus_id`) satisfies both the
annotation check and any consumer keying on `(a, b)`.

**8. `containment_verification` / `containment_evidence_kind`** (`:835`, `:890`): both are
currently **inert** — declared in the schema, set by the fixture and the real lock, and read
**nowhere**. Either have the audit set `containment_verification` from the computed verdict and
fail the lock if it claims `byte_computed` while `Q1A-8` is `revise`, or delete the fields.
`containment_evidence_kind: "open_bytes"` is currently the unimplemented enum value the critic
named (`:100`); after item 4 it becomes real, and a `declared_manifest` value on an open-role
pair with brokered bytes must be a finding.

**9. `_edge`** (`:1427`): add `"contained_corpus_id"` / `"container_corpus_id"` to c9 evidence
and the computed record-set digests of both sides, so the artifact carries the measurement
Fledge gate 3 asks for ("the **measured** … containment **[M]**") instead of a prose pointer.

**10. Positive control** (selftest, `:3693`). This is the test that separates a computed c9 from
an attested one, and it is the critic's re-review condition 5: build a lock whose container
entry has **disjoint** `publisher_id` / `project_id` / `release_family_id`, **no** shared
generator, **no** c6/c7/c8 edge, `R(container) ⊋ R(contained)` with a *small* slack (not
`fx-json-a`'s 900-identical-line fixture). Assert: **exactly one** c9 edge, `verified is True`,
`evaluated_from == "open_bytes"`, and c6/c7/c8 silent on that pair. Then delete the claim and
assert the edge **still fires** — that is the whole doctrine, as one assertion.

Also add negative controls: the equal-record-set pair must not fire; a 1-record pair must not
fire; a superset-field pair must not fire; `"naN"`, duplicate-key, and lone-surrogate inputs
must not raise.

---

## 7. Acceptance criteria

1. Source frozen; cited digest equals file SHA-256. **Non-negotiable** — three digests were live
   in one session.
2. `python q1a_corpus_lock.py selftest` → `0 failed` (currently 135 checks, 0 failed).
3. `validate_lock(fixture_lock())` → `[]`.
4. `validate_lock(build_real_lock)` → `[]`; the real lock declares `containment_min_records`.
5. Real-lock audit with brokered bytes → **exactly one** c9 edge,
   `('generated-json','generated-sqlite')`, `evaluated_from: "open_bytes"`, `verified: true`,
   `Q1A-8: pass`.
6. Zero `not_evaluated` rows with `criterion == "c9"` and `status == "not_evaluated_no_bytes"`
   when bytes were brokered for both sides.
7. `record_set_sha256` on that edge equals `sha256(canon(sorted(R(generated.json))))` and the
   container digest is present alongside it. Reproduced here for cross-check:
   `84250a193678056c4ebfbe5b85568be55bdd3e9eb46819efd758aeaedb173605` for `generated.json`,
   `619474fe7a999eb0694bc9ce8544918fefe3a695054d2cf70c3f414624de9486` for `generated.sqlite`.
8. `independence_units` is **4** on the real lock and does **not** change when
   `deduplication.containment_claims` is emptied — because `generated-json`/`generated-sqlite`
   already merge via c6/c7/c8. **This is the load-bearing assertion**: it demonstrates the
   unit count is no longer conditioned on curator completeness for c9. Note honestly that on
   *this* corpus the number is unchanged, so the patch's value is **authority**, not a
   different count. The count differs on any portfolio where the container pair has disjoint
   metadata.
9. `dedup-report.json` `edges` contains the c9 edge.
10. Unknown claim id, self-pair, duplicate claim → findings, no crash, artifacts still emitted.
11. Truthful c9 duplicate annotation with contained-id > container-id → **no** PB-11.
12. Counters: `network_ops == codec_invocations == archive_decompressions ==
    sealed_byte_reads == 0`; `zero_sealed_bytes: true`.
13. `verdict.status` remains `CORPUS_BLOCKED` (real portfolio). **A patch that reaches
    `ADMISSIBLE_PILOT` on this portfolio is a bug, not a success.**

## 8. Scope note

On the current portfolio the correct answer is and remains `CORPUS_BLOCKED` /
`INVALID_INFRA` (QP-NO-PROMOTE, QP-CONSUMED, and — on the real lock — PB-02 `source_dirty`,
which is *honestly derived*: `build_real_lock.py:302` sets `source_dirty: true` because the
subject root is the dirty working tree, not the pinned commit). This adjudication does not
change that and does not authorize any promotion, novelty claim, or benchmark. What it changes
is that after the patch, the blocked answer rests on a **computed** independence count rather
than on one whose completeness for cross-container lineage was asserted by a curator — which is
the whole job of this scaffold.