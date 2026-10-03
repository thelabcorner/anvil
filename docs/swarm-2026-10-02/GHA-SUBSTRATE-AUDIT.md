# GHA-SUBSTRATE-AUDIT — safest smallest first remote job after closeout

**Date:** 2026-10-02
**Type:** Static/statistical audit. Additive only.
**Authority:** none. This document does **not** authorize a commit, a push, a workflow
edit, or a dispatch. It produces a *design* plus a *blocking-defect list*.
**Non-actions (deliberate):** no codec was executed; no benchmark, corpus fetch, or
measurement was run; no network call was made; no `.github/workflows/*` file and no
coordinator file (`COORDINATOR-CLOSEOUT-v1.md`, `COORDINATOR-STATE.md`,
`MASTER-BRIEF.md`, `REMOTE-EXPERIMENT-QUEUE.md`) was modified. Nothing was dispatched.
The Q1a scaffold was **not executed** — every claim about it below is derived by reading
its source, its emitted schema definitions, and its already-committed-to-disk sibling
scaffold outputs.

---

## 0. FINAL RULING (coordinator, 2026-10-02) — BINDING

This ruling governs §5 and closes this audit. It is adopted verbatim in intent and
additionally hardens the design already derived in §5.

1. **No first benchmark workflow is recommended or authorized until source identity is
   bound end-to-end.** End-to-end means: dispatch input → immutable source SHA →
   asserted checked-out `HEAD` → asserted clean measurement checkout → asserted tool
   blob **and** tool SHA-256 → recorded `tool.source_sha256` inside the emitted
   artifact. Any workflow missing the last two links does not have source identity and
   is not dispatchable. On today's evidence **13 of 16 workflows fail this test**, and
   the two that pass it (`anvil-i10-dense-frontier.yml`, `anvil-i10-g5-paged-dictionary.yml`)
   are **untracked at HEAD**, so neither is currently dispatchable either. Effective
   dispatchable count: **0**.
2. **The first safe Actions job** must satisfy all of: manually dispatched
   (`workflow_dispatch` only), `permissions: contents: read` and no write scope, no
   secrets, **SHA-pinned actions** (never `@vN`), **immutable source SHA asserted**
   (not merely recorded), **tool blob and tool SHA-256 bound to that source ref**, and
   **fail-closed artifacts uploaded under `if: always()`** with `if-no-files-found: error`.
3. **Zero codec where the purpose is corpus admissibility.** No build, no `apt-get`,
   no backend, no fetch, no network. A codec cannot even be compiled in the designed job.
4. **Q1a's expected `CORPUS_BLOCKED` is a valid negative result, not a workflow
   failure.** The job's pass criterion is **artifact emission + identity verification**.
   The scaffold's exit code is *recorded as evidence*, never propagated as the job's
   verdict. A run that emits a complete, identity-verified, fail-closed artifact set
   whose verdict is `CORPUS_BLOCKED` has **succeeded**.
5. **This report authorizes no heavy benchmark dispatch.** It authorizes no dispatch at
   all. It is a substrate audit and a job design. Dispatch requires a separate
   coordinator act under CLOSEOUT §8 and §9.

The designed job (`Q1a-XRUN`, §5) satisfies ruling items 1–4 by construction. It is
**not recommended for dispatch** until the P0 blockers in §6 clear.

---

## 1. What was inspected

| Population | Count | Size |
|---|---:|---:|
| `.github/workflows/*.yml` | 16 files | 5,719 lines / 258,309 B |
| Corpus + benchmark helper scripts (`tools/gha_*.py`, `tools/paired_bench.py`, `tools/bench_suite.py`, `tools/close_i10_remote_baseline.py`, `tests/make_*_corpus.py`) | 7 primary | ~63 KB |
| CORPUS-ADMISSIBILITY-v1 scaffold (`prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/q1a_corpus_lock.py`) | 1 | 3,726 lines / 153,894 B |
| Track-03 lock scaffold outputs (`prototypes/swarm-2026-10-02/03-heldout-corpus/space-bunny/lockout/`) | 12 artifacts | ~74 KB |
| Corpus manifest + generator lineage evidence (`tests/corpus/CHECKSUMS.txt`, `README.md`, `make_smoke_corpus.py`, `make_synth_corpus.py`) | 4 | ~21 KB |
| Normative references (`docs/I10-CORPUS-LOCK-PROTOCOL.md`, `docs/swarm-2026-10-02/*`) | 5 | — |

**Scaffold identity at audit close** (this matters more than anything else in §4.11):

```
path     prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/q1a_corpus_lock.py
bytes    153894
lines    3726
sha256   05e9b981b10b5c0a2ad91a336e2c6e8fdfda7232990e35b158a8a59de3ac2b72
mtime    2026-10-03T01:56:04Z
git      ?? (untracked — absent from HEAD b8eae11)
```

---

## 2. Statistical substrate

### 2.1 Trigger / permission / lifecycle surface

| Property | Count | Verdict |
|---|---:|---|
| `workflow_dispatch`-only workflows | **16 / 16** | **Excellent.** Zero `push`, `pull_request`, `schedule`, `workflow_call`, `repository_dispatch`. Nothing can fire unattended. |
| Workflows declaring dispatch `inputs` | 6 / 16 (14 inputs total) | 10 workflows are zero-input single-shot jobs. |
| Workflows with any write permission scope | **0 / 16** | **Excellent.** Every workflow is `permissions: contents: read`. |
| Workflows using `secrets.*` | 0 / 16 | **Excellent.** No credential surface at all. |
| Workflows declaring `environment:` | 0 / 16 | No environment gating; also no environment protection needed today. |
| Workflows with `concurrency:` | **16 / 16** | **Excellent.** All 16 use `cancel-in-progress: false`, so a measurement is never truncated by a newer dispatch. |
| Workflows with `timeout-minutes:` | **16 / 16** | 30 min – 360 min. |
| Workflows branch-gated (`if: github.ref == …`) | **2 / 16** | `anvil-i10-aux-portfolio.yml:14`, `anvil-i10-aux-size.yml:14` (both pin `refs/heads/i10-aux-unbwt`). **14 are dispatchable from any ref**, including a fork branch. |
| `continue-on-error` occurrences | **0** | No silent-failure escape hatch anywhere. |
| `fail-fast: false` on matrices | 4 / 4 matrices | Correct for independent corpora. |
| `set -euo pipefail` coverage | 8 / 16 workflows have it (1–12 steps); **8 workflows have zero** | **Fail-open risk.** `anvil-i10-aux-{bench,portfolio,reference-cost,size}.yml`, `anvil-i10-bwt-subblock-frontier.yml`, `anvil-research-bench.yml`, `anvil-i10-grotli-g2-smoke.yml`, `anvil-grotli-g0.yml`. |
| Explicit `exit 1` guards | 3 workflows / 4 sites | All four are placeholder-guards or infra-fatal (`g5a:88`, `g4:…`, `dense-frontier` ×2). |

### 2.2 Supply chain

| Property | Count | Verdict |
|---|---:|---|
| Floating action references (`uses: …@v4`) | **40 across 15 / 16 workflows** (22 × `checkout`, 18 × `upload-artifact`) | **Unpinned.** |
| Commit-pinned action references | **4 in exactly 1 workflow** | `anvil-i10-dense-frontier.yml` — `actions/checkout@11d5960a326750d5838078e36cf38b85af677262` (×3), `actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02`. |
| Runner image pinning | 16 × `ubuntu-24.04` (never `-latest`) | **Excellent.** |
| Toolchain pinning | `clang-18` everywhere; `dpkg-query` package version recorded in 3 workflows | Good; not digest-pinned. |
| Signature verification (gpg / sigstore / cosign) | **0 / 16** | No upstream artifact signature is ever checked. |
| `sha256sum -c` (manifest re-verification) | **0 / 16** | Digests are *recorded*, never re-verified against a shipped manifest. |

### 2.3 Evidence artifacts

| Property | Count | Verdict |
|---|---:|---|
| `upload-artifact` sites | 20 across 16 workflows | — |
| Upload sites guarded by `if: always()` | **3 / 20** | `anvil-i10-grotli-g5a.yml:498`, `anvil-i10-g5-paged-dictionary.yml:560`, `anvil-i10-dense-frontier.yml:1021`. **17 / 20 lose all evidence on failure.** |
| `if-no-files-found: error` | 11 / 20 | 9 sites will upload a partial/absent artifact silently. |
| Explicit `retention-days` | 9 / 20 (30 ×8, 90 ×1) | 11 sites inherit the repo default (90 d). |
| Artifact naming schemes | 4 (`run_id`; `sha`+`run_attempt`; `matrix.corpus`+`sha`; `matrix.corpus`+`run_id`) | **No scheme embeds the frozen implementation SHA.** |
| Distinct `artifact-manifest.json` implementations | **3** (`tools/gha_manifest.py:214-262`; `anvil-i10-dense-frontier.yml:953-1001`; `anvil-i10-g5-paged-dictionary.yml:538-556`) | Triplicated logic, three different field sets. |
| Workflows emitting protocol §10.8 `gates.json` / §10.9 `verdict.json` | **0 / 16** | The protocol's machine-readable contract is implemented **only** in the untracked Q1a scaffold. |

### 2.4 Duplication (the strongest argument for a new minimal job)

Step names recurring in ≥3 workflows: `Install compiler and Brotli` (10),
`Freeze implementation identity` (6), `Fetch and verify frozen discovery corpora` (5),
`Upload discovery evidence` (5), `Fetch and verify held-out V1` (4),
`Upload held-out evidence` (4), `Verify frozen implementation identity` (4),
`Discovery ruling` (3), `Fetch canonical corpus` (3), `Held-out ruling` (3),
`Install pinned dependencies` (3). The 6 G-rotli workflows are ~90 % the same
skeleton with different constants.

---

## 3. Source-identity pinning

Five distinct classes exist. They are **not** comparable in strength, and the classes
are not documented anywhere.

| Class | Workflows | Mechanism | Weakness |
|---|---|---|---|
| **A — asserted triple pin** | `anvil-i10-grotli-g5a.yml`, `anvil-i10-g5-paged-dictionary.yml`, `anvil-i10-grotli-g3.yml` | `FROZEN_IMPLEMENTATION_SHA` env → `checkout ref:` + `fetch-depth: 0`; `test "$(git rev-parse HEAD)" = "$FROZEN…"`; `test -z "$(git status --porcelain)"`; `EXPECTED_*_BLOB` git-blob SHA-1 assertions; SHA-256 recorded | **This is the only class strong enough to build the first remote job on.** g5d additionally regex-validates every dispatch input (`:58-63`) and asserts both blob **and** SHA-256 of the parent result document (`:74-75`). |
| **B — SHA pin asserted, no blob assert** | `anvil-i10-grotli-g2.yml:26`, `anvil-i10-grotli-g4.yml:79` | `test rev-parse HEAD == FROZEN…` + clean-worktree; source SHA-256 computed and written, never compared | Commit pin is sufficient for identity; blob assert is belt-and-braces only. Acceptable. |
| **C — clean worktree, identity = dispatch ref** | `anvil-i10-grotli-g0.yml`, `anvil-i10-grotli-g1.yml` | `git status --porcelain` printed (g0) or `test -z` (g1); `git rev-parse HEAD` **printed, never compared** | Identity floats with the dispatch ref. g0 asserts nothing at all. |
| **D — mutable tag, record-only** | `anvil-i10-aux-size.yml:32` (`ref: i10-baseline-codec-20260923`), `anvil-i10-aux-reference-cost.yml:51`, `anvil-i10-bwt-subblock-frontier.yml:52` (both `ref: ${{ env.CANDIDATE_TAG }}` = `i10-aux-unbwt-final-v1`) | Resolved SHA written into `provenance.txt` and **never asserted** | **A moved tag silently changes the measured source and the run still reports success.** This is the single highest-severity identity defect in the substrate. |
| **E — commit-pinned multi-checkout + blob assert** | `anvil-i10-dense-frontier.yml:75,82,89,129-130` | 3 separate `checkout` paths (`vehicle/`, `tooling/`, `candidate/`) at 3 literal SHAs; per-path `git rev-parse HEAD:<path>` blob assertions; actions commit-pinned | Best structural containment; only workflow with no floating action reference. |
| **Legacy** | `anvil-grotli-g0.yml` | No freeze step; single fetch; git-blob SHA-1 verification only | Superseded by `anvil-i10-grotli-g0.yml`; should be deleted, not left as a second "g0". |

**Cross-cutting defects**

1. **Artifact identity is decoupled from source identity.** 6 workflows check out a
   `FROZEN_IMPLEMENTATION_SHA` that is *not* `github.sha`, yet name their artifacts by
   `github.run_id` or `github.sha`. A consumer holding an artifact cannot tell which
   source produced it without opening a file inside the archive. Only
   `anvil-i10-g5-paged-dictionary.yml` writes a first-class `identity.txt` (`:76-90`)
   *and* an `artifact-manifest.json` (`:538-556`).
2. **Mutable `master` source in the corpus fetcher.**
   `tools/gha_fetch_corpus.py:43-46` pulls `SilesiaCorpus/master/{name}.zip`. Every
   other corpus path in the repo pins `raw.githubusercontent.com/…/<40-hex>/…`. Class D
   and a mutable-upstream corpus fetch are the same defect class.
3. **`tools/close_i10_remote_baseline.py` (9,000 B) is referenced by zero workflows** —
   an orphan harness. Dead substrate.

---

## 4. Findings by requested audit dimension

### 4.1 `workflow_dispatch` inputs

- 6 / 16 workflows declare inputs: `anvil-i10-g5-paged-dictionary.yml` (4 free-form
  `type: string` SHA inputs), `anvil-research-bench.yml` (3 `choice`),
  `anvil-i10-aux-bench.yml` (3 `choice`), `anvil-i10-dense-frontier.yml` (2 `choice`),
  `anvil-i10-aux-reference-cost.yml` (1 `choice`), `anvil-i10-bwt-subblock-frontier.yml` (1 `choice`).
- **10 / 16 use the modern `inputs.*` context; 0 use the deprecated `github.event.inputs.*`.** Clean migration.
- **Only `anvil-i10-g5-paged-dictionary.yml` validates input *shape*** (`:58-63`):
  `wc -c == 40` + `grep -Eq '^[0-9a-f]{40}$'` for each SHA, `^[0-9a-f]{64}$` for the
  SHA-256. Every other workflow relies on `type: choice`, which is inherently
  constrained — safe by construction, but it means **no free-form input anywhere else
  is validated**. Any new free-form input MUST copy the g5d pattern verbatim.
- **0 / 16 pin `github.ref` at dispatch time**; 14 / 16 have no `if: github.ref` guard.
  A dispatch from any branch will run. For a job whose entire value is provenance,
  that is the wrong default.
- `anvil-i10-aux-bench.yml` declares `decode_reps`/`encode_reps` as quoted-string
  choices (`'7'`, `'9'`, `'15'`); YAML quoting is required and present. Consistent.

### 4.2 Artifact schemas

Three incompatible schema generations coexist:

| Generation | Schema id | Where |
|---|---|---|
| ad-hoc | `"schema": 1` | `anvil-i10-dense-frontier.yml:972,986`; `tools/gha_manifest.py:216`; `anvil-i10-g5-paged-dictionary.yml:555`; `anvil-i10-grotli-g5a.yml:222` (`discovery-provenance.json`) |
| protocol §10 | `anvil.corpus-lock/v1`, `anvil.verdict/…` | **0 workflows** |
| Q1a scaffold | `anvil.corpus-admissibility/v1`, `anvil.aggregate-contract/v1`, `anvil.consumed-controls/v1`, `anvil.dedup-report/v1`, `anvil.independence-units/v1`, `anvil.artifact-manifest/v1`, `anvil.verdict…` | `q1a_corpus_lock.py:80-84`, `ARTIFACT_FILES` |

The Q1a scaffold emits **15 artifacts** including the three the protocol §5.3 makes
mandatory (`dedup-report.json`, `publisher-split-report.json`, `license-report.json`)
plus `conflict-edges.json`, `independence-units.json`, `counters.json`,
`consumed-controls-report.json`, `threshold-lint.json`, `verdict.json`,
`artifact-manifest.json`. Each artifact is canonical JSON (sorted keys, compact
separators, one trailing LF) with a per-artifact SHA-256 in the manifest — this is
**materially better** than every existing workflow and should be the model.

Gaps: no `gates.json` (§10.8) and no protocol-shaped `verdict.json` (§10.9 — the
scaffold uses its own `verdict` object with a different field set and a different
status enum: `ADMISSIBLE_PILOT | CORPUS_BLOCKED | INVALID_INFRA` vs the protocol's
7-value `PASS_DISCOVERY … BLOCKED_AMBIENT`). A consumer written against §10.9 will not
read the scaffold output.

### 4.3 Fail-closed behaviour

**Good:** `timeout-minutes` 16/16; `continue-on-error` 0; `cancel-in-progress: false`
16/16; `if-no-files-found: error` 11/20; `if: always()` on the 3 most mature uploads;
`gha_manifest.py:124-157` raises `RuntimeError` on duplicate file rows **and** on any
aggregate that changes across repetitions; `paired_bench.py` is invoked under
`taskset -c 0`.

**Bad / missing:**

1. **8 workflows have no `set -euo pipefail` at all.** In a multi-step `run: |` block
   without it, a failed intermediate command does not stop the step; the step reports
   success and the run uploads "evidence" produced from a broken state.
2. **17 / 20 upload sites are not `if: always()`-guarded**, so the *most* informative
   failure mode (partial evidence) is the one that is discarded.
3. **`tools/gha_fetch_corpus.py` is not fail-closed on upstream loss in the protocol
   sense.** It raises on size/MD5 mismatch (fail-closed on *wrong bytes*), but it has no
   `INVALID_INFRA` classification, no single-retry rule (protocol §6.11), no
   allowed-host allowlist (§6.6), no temporary-path-then-atomic-promote (§6.7/§6.10),
   and no archive-digest verification before extraction (§6.8).
4. `tools/gha_fetch_corpus.py:104-110` skips the download when the destination file
   already exists and verifies the existing file — correct, but it means a **cached**
   corpus is verified only against MD5, never against the §6.12 SHA-256 +
   archive/derivation identity set.

### 4.4 SHA-256 verification

| Corpus path | Size check | Git blob SHA-1 | SHA-256 |
|---|---|---|---|
| `anvil-i10-grotli-g5a.yml:207-214` (5 objects) | asserted | **asserted** | **asserted** |
| `anvil-i10-g5-paged-dictionary.yml:74-75` (parent result doc) | — | **asserted** | **asserted** |
| `anvil-i10-grotli-g1/g2/g3/g4` (fetch steps) | asserted | asserted | **recorded only** (`sha256 = hashlib.sha256(...)`, no comparison) |
| `tools/gha_fetch_corpus.py` (Silesia ×12, enwik8) | asserted | — | **MD5 only** (`gha_fetch_corpus.py:28-41, 58-68`) |

Three concrete, citable conflicts with the substrate the Q1a job must feed:

- `tools/gha_fetch_corpus.py` verifies by **MD5**. The lock schema's acquisition block
  requires `identity_algorithm: "sha256"` and `md5_forbidden: true`
  (`q1a_corpus_lock.py:685-692`). **The existing fetcher is non-compliant by
  construction** and cannot be reused inside a Q1a job unchanged.
- `tools/gha_fetch_corpus.py:80-94` **decompresses archives**. Q1a hard-prohibits
  archive decompression (`q1a_corpus_lock.py:420-428`, gate `Q1A-4`/`Q1A-13`,
  `assert_zero_exposure`). The fetcher can never run inside a Q1a job as written.
- `tools/gha_manifest.py:163-168, 196` emits **float** values (`ratio`) into artifacts.
  `q1a_corpus_lock.py:325-340` canonicalises with `allow_nan=False` and asserts "no
  float is ever produced". `gha_manifest.py` output is therefore not admissible inside
  a Q1a artifact set. (It also embeds `platform.*`, `lscpu`, `dpkg-query` — correct for
  a provenance manifest, but it makes the bytes environment-dependent, which is fine
  for `manifest.json` and wrong for a determinism-comparison digest.)

`tests/corpus/CHECKSUMS.txt` **is** SHA-256 and complete (24 payload entries; only
`README.md` and `CHECKSUMS.txt` itself excluded), but its format is
`<sha256>  <bytes>  <name>` — a 3-column layout that **`sha256sum -c` cannot consume**.
Converting it (or shipping a companion `SHA256SUMS`) is a prerequisite for any
`if-no-files-found`-style verification step.

### 4.5 Evidence roles

- **Only one workflow labels roles at all**: `anvil-i10-grotli-g5a.yml:216-219` emits
  `{"role": "discovery" | "known-stress-not-held-out"}` in
  `results/discovery-provenance.json`.
- **Role relabelling across workflows for the same object.** Object `V1-sino-us-drugqa-all.jsonl`
  is fetched by `anvil-i10-grotli-g3.yml` / `g4.yml` under a step named
  **"Fetch and verify held-out V1"** and ruled under **"Held-out ruling"**, while
  `anvil-i10-grotli-g5a.yml:198,218` correctly reclassifies the identical object as
  **known-stress, explicitly "not held-out"** because G3 already opened it. The G5A
  comment is the correct reading. The earlier workflows' step *names* are an
  admissibility error that survives in the artifact layout of those runs.
- **0 workflows implement queue Edit E1** (`evidence_role` per result row; "citation-grade"
  banned for non-held-out). The per-row role field does not exist anywhere in CI output.
- `anvil-i10-aux-portfolio.yml`, `anvil-i10-aux-size.yml`,
  `anvil-i10-aux-reference-cost.yml`, `anvil-i10-bwt-subblock-frontier.yml`,
  `anvil-i10-aux-bench.yml`, `anvil-research-bench.yml` record **no role for the corpus
  they measure at all** (Silesia/enwik8 rows are role-less).
- Corpus self-description is partial: `tests/corpus/README.md` gives per-file
  provenance and seeds, but there is **no role column and no license column**. Per the
  Track-03 lockout (`publisher-split-report.json`), the corpus has 5 publisher groups
  and 7 release families with `isolation_ok: false`.

### 4.6 `conflict_edges` / union-find

Implemented in the Q1a scaffold, and structurally the right shape:

- `build_conflict_graph(...)` walks all `n(n-1)/2` pairs and emits `edges[]` +
  `not_evaluated[]`, each edge carrying `criterion`, `criterion_meaning`, `evidence`,
  `evidence_sha256`, `evaluated_from`, `verified`, `waived: false`
  (`q1a_corpus_lock.py:1258-1520`).
- `independence_components(...)` is a correct union-find with path compression and a
  deterministic `min`-root tie-break; the component id is content-addressed as
  `"iu-" + sha256(canon(sorted members))[:16]`, so group ids are **stable across runs,
  machines and Python minor versions** (`:1528-1566`).
- **Curator-supplied group ids are structurally impossible.** `TOP_FIELDS` rejects
  unknown fields (`_validate_object:862`) and `FORBIDDEN_CURATOR_FIELDS` (`:771-781`)
  additionally emits a legible `GATE-INDEP-1.curator_group_id` finding for
  `entries[].independence_unit_id` / `group_id` / `deduplication.groups` / `group_map` / …
- **Curator duplicate annotations must be a subset of computed edges** (gate `Q1A-7`,
  `:2140-2165`) — an annotated duplicate with no computed edge is `PB-11`. This is the
  correct anti-inflation direction.
- Cross-check on coverage: the Track-03 scaffold's `dedup-report.json` reports
  `pairs_compared: 276` = C(24,2) — the pairwise audit is genuinely complete.

Defects, in decreasing severity:

1. **c3/c4 under-detection biases toward *more* independence units (fail-open).**
   `block_digests` (`:1168-1172`) hashes only **non-overlapping, start-aligned**
   `block_bytes` windows and drops the trailing partial block. A shared 4 KiB region at
   a non-aligned offset in one of the two files is invisible, and the final short block
   of every file is never compared. Protocol §5.1 rules 3/4 say "a shared exact raw
   4 KiB block" with no alignment qualifier. Given all 24 entries are open roles, every
   pair is subject to this.
2. **`SHINGLE_SCREEN_MAX = 4096` is declared and never used** (single occurrence, its own
   definition). The rare-block screen that §4.6's frequency bound anticipates does not
   exist, so `rare_block_max_entries` has nothing to screen.
3. **c6/c7 mark `verified: true` on metadata-only inference.** `build_conflict_graph`
   passes `verified=True` for c6/c7/c8 (`:1331, 1341, 1351, 1360, 1372, 1381`) although
   the evidence is curator-declared `publisher_id` / `project_id` /
   `release_family_id` / `producer_id` / `generator_*` with no byte or attestation
   backing. Compare c1/c2, which correctly set `verified = (a in byte_indexes and b in
   byte_indexes)`. A `verified: true` on a declared string is a claim the artifact makes
   about its own evidence that the artifact cannot support.
4. **c6/c7 are extremely aggressive and will over-merge.** Merging on shared
   `project_id` or `release_family_id` alone means every entry from one upstream
   project collapses into one unit. That is the *safe* direction for promotion, but it
   makes `independence_units` uninterpretable as a portfolio metric if it saturates to
   1–2.
5. **`build_conflict_graph` is skipped entirely when the lock is schema-invalid**
   (`:2110`). `edges=[]`, `units=[]`, `Q1A-6 = "blocked"`. Downstream,
   `aggregate_contract` sees `n_independence_units == 0` for held-out rows and blocks
   promotion. **Fail-closed — correct**, but it means a schema-invalid lock yields a
   *schema* verdict, never an *independence* verdict. See §6 blocking item B3.
6. `estimated_jaccard` (`:1223-1228`) returns the exact signature-equality rate, which
   is the correct MinHash similarity estimator and is emitted as an integer fraction
   (no float) — good for determinism.

### 4.7 Cross-container containment (criterion c9)

- Declared as **amendment A6** (`q1a_corpus_lock.py:279-289`), adding
  `deduplication.containment_claims[]` with `{contained_corpus_id, container_corpus_id,
  record_set_sha256, evidence, attestation_kind, attestation_sha256}`
  (`:661-668`) — an explicit **merge-only** channel for roles whose bytes may not be
  read. The direction is right: containment may only *merge* units, never split them.
- **Record-set decoding is not implemented.** For structured pairs with no declared
  claim, the sweep records
  `status: "not_evaluated_no_bytes"`, reason *"record-set decoding is not implemented in
  the prototype; declare a containment attestation"* (`:1505-1516`). Honest, correctly
  labelled, and **not** silently clean.
- **Sealed pairs get a dual record**: a `not_evaluated_sealed` row *and* a `c9` edge
  with `verified: false` (`:1461-1484`). Union-find then merges them. Safe direction,
  but the artifact simultaneously says "not evaluated" and "conflict"; a consumer must
  read `verified` to disambiguate.
- **`audit_parameters.containment_evidence_kind` (`declared_manifest | open_bytes`) is
  declared in the schema and never enforced.** `build_conflict_graph` accepts every claim
  and always labels it `publisher_attestation`, regardless of what the lock declared.
- **A structural analogue already exists in CI and is reusable as a design precedent:**
  `anvil-i10-dense-frontier.yml` checks out three independent trees into three separate
  workspace paths and asserts each path's blob identity before use. That is source-tree
  containment, not record-set containment, but it is the same discipline.

### 4.8 Threshold pinning

The *shape* is right and unusually disciplined:

- `AUDIT_PARAM_OBJ` (`:721-733`) requires exactly 12 named parameters, including
  `minhash_threshold_num` / `minhash_threshold_den` as an **integer fraction** (no
  float), and `declared_by` / `declared_utc` / `declared_sha256` as provenance.
- `threshold_lint` (`:1765-1827`) enforces **exact key set equality** — missing keys →
  `THRESH.undeclared`, extra keys → `THRESH.unknowable` — and rejects a non-positive
  denominator (`THRESH.unusable`). It then AST-walks its own module source and flags any
  module-level `int` constant not in `PINNED_NUMBERS` (`THRESH.code_default`). This is
  exactly SYNTH-CORPUS-FLEDGE 11.5's requirement, implemented mechanically.

**But the lint is not enforcing what it appears to enforce**, and it fails on itself:

1. **Dead parameters.** `rare_block_max_entries` and `minhash_screen_k` appear only in
   the schema (`:722, 727`), in the `params` copy (`:2079`), in
   `REQUIRED_AUDIT_PARAMETERS`, and in the self-test. **No comparison anywhere reads
   them.** The lint reports them as "declared" and the artifact presents them as audited
   thresholds. They are decorative.
2. **`declared_sha256` is never recomputed.** It is echoed into `threshold-lint.json`
   (`:1824`) and nothing checks it against `canon_sha256(audit_parameters)`. The pin on
   the thresholds is therefore presentational — exactly the "post-hoc gate movement"
   surface the doctrine forbids, with a check that looks like it prevents it.
3. **`containment_evidence_kind` is never enforced** (§4.7).
4. **The lint flags the tool's own constants, so it can never pass.** Five module-level
   `int` constants exist and none is exempt:

   ```
   :81   PROTOCOL_VERSION = 1          (structural, but not exempt)
   :1142 BLOCK_DIGEST_BYTES = 8        (structural)
   :1143 TEXT_LIKENESS_MIN_NUM = 90    <-- a REAL decision threshold (the 0.90 text gate)
   :1144 TEXT_LIKENESS_MIN_DEN = 100   <-- ditto
   :1146 SHINGLE_SCREEN_MAX = 4096     <-- dead
   ```

   `PINNED_NUMBERS` contains only `MINHASH_PRIME` (which is an `ast.BinOp`, so the
   `ast.Constant` test never matches it — it is exempt by accident, not by design) and
   `B64_LEN` (**never defined anywhere in the file**). Consequently
   `threshold_lint(_read_own_source())["code_default_constants"]` is non-empty, the
   self-test check `W12.1 no code defaults` fails, gate **`Q1A-18` fails**, the verdict
   rule `elif failed_gates or blocking: status = "CORPUS_BLOCKED"` fires, and the CLI
   returns exit 1 (`return 0 if … == "ADMISSIBLE_PILOT" else 1`).
   **Consequence: the scaffold returns `CORPUS_BLOCKED` for every lock, forever, and
   the self-test is red.** Of the 5, only `TEXT_LIKENESS_MIN_{NUM,DEN}` is a true
   threshold-in-code violation; the fix is to move the text-likeness gate into
   `audit_parameters` and delete `SHINGLE_SCREEN_MAX`, and to introduce a
   `STRUCTURAL_CONSTANTS` allow-list with a stated reason per entry.

### 4.9 Consumed tombstones

`Tombstone` (`:1029-1136`) is well built:

- Validates `schema == "anvil.consumed-controls/v1"`, rejects duplicate keys, and
  **requires `consumed: true`, `promotion_forbidden: true`, and
  `role_after_consumption == "known_stress"`** on every row — so a tombstone row cannot
  be weakened in place.
- Matches on **three SHA-256 identities** (`content.sha256`,
  `source.archive.member_sha256`, `source.archive.archive_sha256`) **and four
  ledger-name identities** (`corpus_id`, `display_name`, `independence.project_id`,
  `content.canonical_filename`) via `normalize_name` (lowercase, strip non-alphanumeric).
  Over-matching via name normalization is the safe direction.
- **Stale-tombstone binding (Edit E5, `E5.stale_tombstone`)** compares
  `consumed_controls_ref.sha256` against the canonical digest of the tombstone actually
  supplied *and* `consumed_controls_ref.entry_count` against the loaded entry count
  (`:2244-2256`). A lock audited against a superseded tombstone fails closed.
- Gate `QP-CONSUMED` is not waivable and is ANDed into `promotion_authorized`.

**Blocking gaps:**

- **No `anvil.consumed-controls/v1` document exists anywhere in the repository.** No
  JSON file in the tree contains that schema string. Amendment A5's
  `consumed_controls_ref` therefore cannot be satisfied, and `Tombstone.__init__` will
  raise `tombstone.schema` on anything supplied.
- **No `corpus-access-ledger.jsonl` exists** either — required by protocol §7 and
  reported as live blocker `PB-09` in the Track-03 scaffold's `blocking-gates.json`
  (`still_live_after_scaffold: ["PB-09","PB-26"]`).
- The `git_tracked` requirement is correctly **recorded, not required true**
  (`:1594-1595`), honouring E5's withdrawal of the git-tracking advice.

### 4.10 `promotion_authorized = false` logic

This is the strongest part of the scaffold. Defence in depth, five independent locks:

1. `aggregate_contract` (`:1846-1920`) emits `promotion_authorized` false if any row
   lacks a valid `evidence_role`, if `aggregate_heldout_only` is null, or if held-out
   rows map to zero independence units; it emits the all-roles aggregate with
   `citation_grade_forbidden: true`, and `citation_grade_permitted` is true only if
   **every** row is a promotion role and unconsumed.
2. `QP-NO-PROMOTE` fails if **any** input row's role is in the 6-role blocklist.
3. `QP-CONSUMED` fails on any tombstone match.
4. `:2299-2312` ANDs all three into `promotion_authorized`, appends the specific
   failure reason, and ANDs `pass_variant_emitted` with it.
5. The `verdict` object hard-codes `promotion_authorized: False`,
   `production_authorized: False`, `mechanism_claim_authorized: False`,
   `novelty_claim_authorized: False`, `promotion_authorized_by_this_tool: False`,
   exposing only `aggregate_promotion_authorized` as a possibly-true sub-field.

`QP_NO_PROMOTE_ROLES` is the queue's 5 roles **plus `external_development`**, and the
deviation is disclosed in-band via `QP_GATE_DEVIATIONS` (`:158-172`) rather than applied
silently. That is the correct handling of a spec/implementation gap.

**Two consequences the preregistration must state, or the first run will be misread:**

- `status == "ADMISSIBLE_PILOT"` requires `promotion_authorized == True`, which requires
  **every** row to be `heldout`/`external_test` and unconsumed. A mixed-role lock can
  therefore **never** return `ADMISSIBLE_PILOT`. Combined with §4.8's self-failing lint,
  the tool's only reachable statuses today are `CORPUS_BLOCKED` and `INVALID_INFRA`.
- `hard_codes = ("PB-01","PB-02","PB-03","PB-04","PB-06","PB-09")` maps to
  `INVALID_INFRA`. **`PB-05` (codec invocation) is absent from that tuple**, so a codec
  invocation would be reported as `CORPUS_BLOCKED` — a scientific verdict for what is an
  infrastructure violation. One-line fix; worth doing before any run.

### 4.11 Dirty-worktree / `source_tree` semantics

**The local tree** (`C:\Users\slooshied\Documents\ANVIL`): branch `i10-aux-unbwt` at
`b8eae11fa353bb5e3c88e8e757fe3ad41e4bbd24`, **127 dirty paths — 119 untracked,
8 modified.** Untracked includes the entire `docs/swarm-2026-10-02/` set,
`docs/I10-CORPUS-LOCK-PROTOCOL.md`, both best-in-class workflows
(`anvil-i10-dense-frontier.yml`, `anvil-i10-g5-paged-dictionary.yml`), and the Q1a
scaffold itself. `.gitignore` covers `build/`, `third_party/*`, `*.anv`, `*.out`,
bare binary names and `scratch/` — **not `build-*/` (7 dirs) or `prototypes/`**, which is
why 119 of the 127 entries are noise. Per MASTER-BRIEF doctrine 8 this dirt is
intentional and must not be cleaned.

**The semantics are right; the enforcement is missing.**

- `check_source_scope` (`:991-1021`) implements Edit E8 exactly: `source_dirty_scope`
  must be `pinned_checkout`; `source_tree_ref` must be 40-hex; `evaluated_at` must be
  RFC 3339 `Z`; **`project.source_git_sha` must equal `source_tree_ref`** (otherwise
  `source_dirty` has no referent → `E8.authoring_commit_unbound`); and
  **`source_dirty` must be exactly `False`** (`PB-02`). `local_worktree_evaluated: False`
  is emitted as a constant with a note that it is "never measured". That is the right
  design and matches `checkout-cleanliness.json`'s principle statement.
- **But nothing binds the artifact to the source that produced it.** The tool has no git
  access (correctly — it must stay a pure function), so `tool.source_sha256`
  (`:1938-1943`, a self-file read) and `project.source_tree_ref` are two independent
  unverified strings. **The binding must be supplied by the workflow** — exactly as
  `anvil-i10-g5-paged-dictionary.yml:74-75` does with
  `test "$(git hash-object frozen-g5a-results.md)" = "$G5A_RESULTS_BLOB"` and
  `test "$(sha256sum … )" = "$G5A_RESULTS_SHA256"`.
- **The existing lock makes a false provenance claim.**
  `prototypes/swarm-2026-10-02/03-heldout-corpus/space-bunny/lockout/corpus-lock.json`
  declares `project.source_git_sha = b8eae11fa353bb5e3c88e8e757fe3ad41e4bbd24` — but
  that commit **does not contain the lock file** (untracked). A lock that names a commit
  in which it does not exist is precisely the laundering vector E8 was written to close.
- **The lock is on a different schema generation than the tool.** The lock's top-level
  keys are `acquisition, aitdcc, artifacts, created_utc, deduplication, entries,
  lineage, lock_revision, project, promotion, protocol_version, publisher_isolation,
  roles, schema, splits, title, workflow`. It is missing **all six** amendment groups:
  `project.source_tree_ref` / `evaluated_at` / `source_dirty_scope` (A1),
  `audit_parameters` (A2), `roles.evidence_split_map` (A3b),
  `source.git_tracked` / `host_install` + `entry.toolchain` (A4),
  `consumed_controls_ref` (A5), `deduplication.containment_claims` (A6). Feeding this
  lock to the current tool produces `Q1A-1 = fail` on missing fields, which (§4.6 item 5)
  suppresses the entire conflict graph and yields a *schema* verdict.
  The lock's own sibling `schema-validation.json` already reports `valid: false` with 6
  `PB-04` violations — all six are `https-file requires a url` on the six host-installed
  PE binaries, which is exactly what amendment **A4** (`source.kind: "host-installed"`)
  exists to fix. **The lock must be re-emitted under A1–A6 before it can be audited.**
- **The scaffold is a moving target.** During this audit its bytes changed three times:
  94,150 B (a mid-write fragment that did not parse) → 151,204 B → **153,894 B**
  (`05e9b981…`, mtime `2026-10-03T01:56:04Z`). It is untracked, so it has no identity
  at all. **No pinned remote source identity for CORPUS-ADMISSIBILITY-v1 currently
  exists**, and CLOSEOUT §8 makes a clean pinned source identity a precondition for any
  dispatch.
- **The decisive-number disagreement is live.** Track-03's scaffold reports
  `independence_unit_count: 7`, `admissible_units: 0`, `heldout_units: 0`, from
  `dedup-report.json` `pairs_compared: 276 / pairs_blocking: 156 / waiver_path_present: false`
  (**every blocking pair attributed to `c7` alone**). The Q1a tool computes a *different*
  number by construction: it adds c3/c4 over real open-role bytes (which Track-03 never
  did), and merges on `project_id`/`release_family_id` from `independence` metadata. The
  queue (Edit E6) predicted `≤ 5`. **Three mutually inconsistent predictions now exist:
  ≤5 (Fledge), 7 (Track-03), and ≤7-but-fewer (Q1a, once c3/c4 apply).** Reconciling
  that disagreement is the single most valuable zero-codec output available.

---

## 5. Design — the safest smallest first remote job

### 5.1 Is a remote job justified? (honest answer)

**Yes, but not for Q1a as such.** Queue Edit E6 already rules that Q1a's decisive half
is local: *"There is no resource, isolation, or reproducibility argument for putting it
on Actions."* Running the audit itself remotely would be theatre.

What a hosted runner adds, and what nothing local can supply, is **one** property:

> an **independent machine** reproduces the **byte-identical canonical artifact** from a
> **pinned commit**, and does so with **zero codec, zero network, zero sealed bytes**.

Per ruling item 1, this is the *only* property worth a runner, and it is worthless
unless the source identity is bound end-to-end first. Identity binding is therefore the
gate; the determinism comparison is the payload.

That is Q1b's "cross-runner reproducibility check that Q1a's determinism assertion
depends on", reduced to its smallest form. It converts "the coordinator ran it on his
laptop" into an independently attested fact, and it is the precondition for the blind
second-auditor half that follows. It is also the cheapest possible remote job: one job,
one runner, no apt, no fetch, no build, seconds of CPU.

**Recommended name:** `Q1a-XRUN` — CORPUS-ADMISSIBILITY-v1 cross-runner determinism
attestation. Workflow path: `.github/workflows/anvil-i10-corpus-admissibility.yml`.

### 5.2 Dispatch contract (g5d's validated pattern, verbatim shape)

```yaml
on:
  workflow_dispatch:
    inputs:
      implementation_sha:   {required: true, type: string}   # 40-hex commit holding tool+lock+tombstone
      lock_sha256:           {required: true, type: string}   # 64-hex companion digest of corpus-lock.json
      tombstone_sha256:      {required: true, type: string}   # 64-hex of the consumed-controls doc
      expected_report_sha256:{required: true, type: string}   # 64-hex of admissibility-report.json, produced locally
      expected_units:        {required: true, type: string}   # preregistered independence_units prediction
```

Every input is shape-validated before any other step (`wc -c` + `grep -Eq '^[0-9a-f]{40}$'` /
`^[0-9a-f]{64}$`), copied from `anvil-i10-g5-paged-dictionary.yml:58-63`. No free-form
input is ever consumed unvalidated.

### 5.3 Job contract

| Property | Value | Why |
|---|---|---|
| `runs-on` | `ubuntu-24.04` | Matches 16/16 existing workflows; never `-latest`. |
| `permissions` | `contents: read` | Matches 16/16; zero write scopes in the whole repo. |
| `concurrency.group` | `anvil-i10-corpus-admissibility-${{ inputs.implementation_sha }}` | Never cancels an in-flight attestation. |
| `cancel-in-progress` | `false` | Same. |
| `timeout-minutes` | `15` | The job is seconds of CPU; 15 min is a hard ceiling, not a budget. |
| `env.FROZEN_IMPLEMENTATION_SHA` | pinned literal | Class-A identity. |
| `checkout` | `ref: ${{ env.FROZEN_IMPLEMENTATION_SHA }}`, `fetch-depth: 0` | Full history so `git rev-parse <sha>:<path>` works. |
| Actions | commit-pinned, not `@v4` | Only 1/16 workflows does this today (`anvil-i10-dense-frontier.yml`). |
| `apt-get` | **none** | `python3` is preinstalled. Omitting apt removes the entire package-supply-chain surface — no compiler, no brotli, no cmake, **no codec can even be built**. |
| Network | none | No fetch step. No `urllib`, no `curl`. |
| Codec | none | No build, no `anvil`, no brotli, no `./grotli_*`. |
| Sealed bytes | none | The audit reads only `discovery`/`known_stress` bytes. `heldout`/`external_test` roles are passed through `ExposureGuard`, which raises `PB-06` before producing a byte. |
| Artifact upload | `if: always()`, `if-no-files-found: error`, `retention-days: 90` | The g5d/g5a/dense-frontier pattern; 90 d because this artifact *is* the evidence of record. |
| Artifact name | `corpus-admissibility-${{ inputs.implementation_sha }}-${{ github.run_id }}-${{ github.run_attempt }}` | **Source identity in the artifact name** — the defect class identified in §3. |

### 5.4 Step ladder (each step fail-closed, `set -euo pipefail` in every `run:`)

Ruling compliance is structural, not incidental: steps 1–4 *are* the end-to-end source
identity binding demanded by ruling item 1, and step 9's artifact upload is the
fail-closed artifact demand of ruling item 2.

| # | Step | Assertion | On failure |
|---|---|---|---|
| 1 | Validate dispatch inputs | 40/64-hex regex for all 5 inputs | exit 1 — `INVALID_INFRA` |
| 2 | Checkout pinned commit | `test "$(git rev-parse HEAD)" = "$FROZEN_IMPLEMENTATION_SHA"` | exit 1 |
| 3 | Assert clean measurement checkout | `test -z "$(git status --porcelain)"` **and** `test -z "$(git diff --stat)"` **and** `test -z "$(git stash list)"` | exit 1 |
| 4 | Bind tool + inputs to source identity | `git rev-parse HEAD:<tool>` == expected blob; `sha256sum <tool>` == expected SHA-256; same for `corpus-lock.json` and the tombstone | exit 1 |
| 5 | Verify lock companion | `test "$(sha256sum corpus-lock.json \| cut -d' ' -f1)" = "$LOCK_SHA256"` (protocol §6.2) | exit 1 |
| 6 | Run the audit | `python3 q1a_corpus_lock.py audit --lock … --tombstone … --emit-dir out` — emit only; **do not** let its exit code be the job's verdict | step records the exit code |
| 7 | Zero-exposure assertion | `counters.json` shows `network_ops: 0, codec_invocations: 0, archive_decompressions: 0, sealed_byte_reads: 0` | exit 1 |
| 8 | Determinism comparison | `sha256(out/admissibility-report.json)` == `inputs.expected_report_sha256` | exit 1 |
| 9 | Preregistered-number comparison | `independence-units.json` `independence_unit_count` vs `inputs.expected_units` | **recorded as a finding, NOT a failure** — a disagreement is the *result* |
| 10 | Emit artifact manifest + upload | `if: always()`; SHA-256 per file; `artifact-manifest.json` | — |

### 5.5 Preregistered expectation (must be written before dispatch)

1. **Expected verdict: `CORPUS_BLOCKED` — a valid negative result, per ruling item 4.**
   This is a result about the **evidence base**, not about the job, and it is the
   designed outcome. Statically derivable today, without running anything:
   `independence-units.json` reports `heldout_units: 0`, and the lock has 0 entries in
   the `heldout`/`external_test` splits, so `aggregate_heldout_only` is null and
   `promotion_authorized` is false with reason *"aggregate_heldout_only is null"*.
2. **Expected scaffold exit code: 1. The job still succeeds.** The exit code is captured
   into the artifact set as evidence and is **not** propagated to the job verdict. The
   job's pass criterion is: steps 1–8 and 10 all asserted, artifact set complete and
   `if-no-files-found: error`-clean. Red-on-verdict with green-on-identity is the
   success shape of this job, and the preregistration must say so before dispatch so it
   cannot be re-read post hoc as a broken run.
3. **Expected `independence_units`: ≤ 7, and plausibly < 7**, because the Q1a tool adds
   c3/c4 over real bytes (which Track-03 never applied) on top of the c7 merges. If it
   returns exactly 7, the byte criteria contributed nothing and that itself is a
   finding about `block_digests` coverage (§4.6 item 1).
4. **Expected `Q1A-18 = fail`** until §4.8 item 4 is fixed. Predicted so the first run is
   not read as a surprise.

### 5.6 What this job must NOT do

No codec. No network. No `apt-get`. No corpus fetch. No timing, no throughput, no ratio,
no RSS. No held-out or sealed byte read. No artifact naming that omits the source SHA. No
upload without `if: always()`. No promotion, novelty, mechanism, or frontier language —
`promotion_authorized`, `mechanism_claim_authorized`, `novelty_claim_authorized` are all
hard-coded `False` in the tool and the job must not reinterpret them. No
`--parse=ratio`, no fuzz, no bench suite.

---

## 6. Blocking defects — coordinator-owned, must clear before dispatch

| ID | Blocker | Evidence | Severity |
|---|---|---|---|
| **B1** | No commit exists for the tool, protocol, or lock. Nothing is pinnable. | `git ls-tree HEAD` → 0 entries under `docs/swarm-2026-10-02/` and `prototypes/swarm-2026-10-02/`; q1a scaffold `??` | **P0** |
| **B2** | The scaffold is not self-consistent: `Q1A-18` self-fails on 4–5 of its own module constants, so the verdict is permanently `CORPUS_BLOCKED` and self-test `W12.1` is red. | `q1a_corpus_lock.py:81,1142,1143,1144,1146` vs `PINNED_NUMBERS` (`:93-96`) and `threshold_lint` (`:1765-1827`) | **P0** |
| **B3** | The lock predates amendments A1–A6 → `Q1A-1 = fail` → the entire conflict graph is suppressed → a *schema* verdict, never an *independence* verdict. | lock top-level keys vs `TOP_FIELDS` (`:742-764`) | **P0** |
| **B4** | No `anvil.consumed-controls/v1` document exists; `A5.consumed_controls_ref` unsatisfiable. No `corpus-access-ledger.jsonl` either (live `PB-09`). | repo-wide search: 0 hits for the schema string; `blocking-gates.json` `still_live: ["PB-09","PB-26"]` | **P0** |
| **B5** | `audit_parameters` values are undefined. Protocol §5.1 rule 5 fixes the shape (128 permutations, 5-token shingles, ≥0.20) and must be supplied as **lock** values, not code. | `AUDIT_PARAM_OBJ:721-733`; `CRITERIA["c5"]:301` | **P1** |
| **B6** | `rare_block_max_entries`, `minhash_screen_k`, `containment_evidence_kind`, `declared_sha256` are declared-and-linted but never enforced. Threshold pinning is presentational. | §4.8 items 1–3 | **P1** |
| **B7** | `PB-05` is absent from `hard_codes`, so a codec invocation would be reported as `CORPUS_BLOCKED` rather than `INVALID_INFRA`. | `:2534` (`hard_codes` tuple) | **P2** |
| **B8** | The track-03 lock declares `source_git_sha = b8eae11f…`, a commit that does not contain it. A false provenance claim. | `lockout/corpus-lock.json` vs `git ls-files` | **P1** |
| **B9** | 14/16 workflows are ref-ungated and 15/16 float `@v4` actions; a new job must not inherit either. | §2.1, §2.2 | **P2** |
| **B10** | `tools/gha_fetch_corpus.py` verifies by **MD5** and **decompresses archives** — mutually exclusive with `md5_forbidden: true` and `zero_archive_decompression`. It must not be reachable from a Q1a job. | `gha_fetch_corpus.py:28-41,58-68,80-94` vs `ACQ_OBJ:685-692` and `ExposureGuard.decompress:420-428` | **P1** |

**Recommended sequencing.** B2 → B3 → B4 are tool/lock repair and are best done and
verified **locally** (Edit E6). Only after a local green `admissibility-report.json`
exists with a known digest does B1 (one coordinator-authorized commit) and then the
`Q1a-XRUN` dispatch become meaningful. Dispatching before B1–B4 clear produces an
artifact that proves nothing and burns the one clean-pinned-identity opportunity the
project has.

**B1 is the gate for every remote job, not just this one.** Per ruling item 1, B1
(`source identity bound end-to-end`) is a precondition for the *first* Actions job of
any kind — including any heavy benchmark workflow. It is currently unmet, and it is
unmet for a mundane reason: the material that would supply the identity (the Q1a
scaffold, the corpus-lock protocol, both best-in-class workflows, and the entire swarm
document set) is untracked, and MASTER-BRIEF doctrine 8 forbids a worker from
committing it. **Unblocking B1 is a coordinator act and is the single highest-leverage
action available.** Until it is taken, the correct answer to "what do we dispatch first"
is: nothing.

---

## 7. Non-actions

Not executed: any codec (`anvil`, `anvil_bench`, `brotli_lw`, `grotli_g0…g5`, `g5a`,
`g5_paged_dictionary`); any benchmark or corpus fetch; the Q1a scaffold itself (including
its `selftest`); any network operation. Not modified: all 16 workflow files, all
coordinator files, the Q1a scaffold, the Track-03 lockout, and `tests/corpus/*`.
Written: this document only, at `docs/swarm-2026-10-02/GHA-SUBSTRATE-AUDIT.md`.

Two findings in this document are **stale-by-construction** and are flagged as such: the
scaffold's byte identity (§1) and any line number cited into it may shift, because the
file changed three times during this audit. Every citation into
`q1a_corpus_lock.py` is valid only against
`sha256 = 05e9b981b10b5c0a2ad91a336e2c6e8fdfda7232990e35b158a8a59de3ac2b72`.

**Not authorized by this report:** any heavy benchmark dispatch, any Q2/Q4/Q5/Q6/Q7/Q9
dispatch, any codec execution on any runner, any network fetch, and any commit or push.
Per ruling item 5 this report carries **no dispatch authority whatsoever**. Its entire
output is: a substrate audit, a defect list, and one job design held pending the P0
blockers in §6.
