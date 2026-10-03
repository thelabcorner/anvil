# ANVIL I10 Corpus Lock and Validation Firewall Protocol

**Protocol version:** 1.0  
**Lock schema:** `anvil.corpus-lock/v1`  
**Status:** normative for corpus selection, acquisition, validation-role separation, and promotion evidence  
**Production authorization:** none

## 0. Purpose

This protocol defines an immutable corpus lock for ANVIL validation. A lock binds corpus bytes, provenance, role, license, independence, acquisition, and artifact rules to one preregistration. It exists to prevent favorable-file selection, role leakage, identity drift, aggregate-only reporting, and promotion from known data.

A lock is not a benchmark result and does not authorize a mechanism. Opening held-out data does not authorize production integration.

Normative terms **MUST**, **MUST NOT**, **SHOULD**, and **MAY** are used as defined by RFC 2119.

## 1. Non-negotiable rules

1. Every measured corpus object MUST have exactly one current role in an immutable corpus lock.
2. Corpus identity MUST be established before compression timing or byte outcomes are inspected.
3. SHA-256 is the content authority. Git blob SHA-1 is required additional identity for Git-derived objects and MUST NOT replace SHA-256.
4. Archive provenance MUST bind the archive hash, exact member identity, and extracted bytes.
5. Acquisition MUST fail closed before codec execution when any identity check fails.
6. Discovery and known-stress data MAY be inspected, tuned against, profiled, and used for regression analysis.
7. Held-out data MUST NOT be fetched, generated, locally benchmarked, inspected for compression anatomy, or used to select parameters before the validation gate authorizes it.
8. A corpus transitions irreversibly from `heldout` to `known_stress` after any compression anatomy or outcome is viewed. It MUST NOT return to `heldout`.
9. Changes to mechanism semantics, thresholds, arms, or implementation after held-out access MUST NOT be validated on the same held-out split.
10. Raw fallback, exact round trips, complete metadata accounting, malformed-input rejection, and preservation of invalid/negative evidence remain mandatory.
11. Per-file rows, skipped rows, invalid rows, and catastrophic regressions MUST remain visible in every summary.
12. A held-out PASS is necessary but not sufficient for production promotion.

## 2. Roles and lifecycle

### 2.1 Current role enum

| Role | Visibility | Permitted use | Promotion use |
|---|---|---|---|
| `discovery` | Open | Mechanism design, tuning, correctness, profiling, ablation | May support discovery evidence only |
| `known_stress` | Open | Regression testing, failure anatomy, known adversarial cases | May support safety/integrity evidence only |
| `heldout` | Sealed until authorized | One-shot confirmation on a frozen implementation | Required for a new mechanism's generalization claim |
| `external_development` | Open after lock | External calibration and mechanism development | Never held-out evidence |
| `external_test` | Protocol-sealed | One-shot external confirmation | External evidence; not a substitute for mechanism-specific new held-out data |
| `external_anchor` | Open | Frozen standard-corpus regression and frontier context | Never unseen generalization evidence |

### 2.2 Role transitions

```text
new -> discovery
new -> known_stress
new -> heldout
new -> external_development
new -> external_test
new -> external_anchor

heldout -> known_stress
external_test -> known_stress
```

A transition MUST be appended to the contamination ledger. The immutable lock records prior roles and replacement lineage; it is not edited to record consumption.

### 2.3 Live ANVIL role consequences

- G1-G5 D1-D4 are `discovery`.
- Sino-US DrugQA V1 is `known_stress`; it was legitimately opened by G3 and is not held out for G5A or later mechanisms.
- Silesia, enwik8, local synthetic fixtures, and the historical S6-2 PE set are open known/anchor data and MUST NOT be relabeled as unseen.
- AITDCC A-H is `external_development`.
- AITDCC I-P is `external_test` and MUST retain the original historical training/testing distinction.

## 3. Immutable lock identity

The authoritative file is `corpus-lock.json`. Its companion `corpus-lock.sha256` contains:

```text
<64 lowercase hex SHA-256>  corpus-lock.json
```

`corpus-lock.json` is serialized as UTF-8 with LF line endings, object keys sorted lexicographically, arrays preserved in their declared order, compact separators, and one terminating LF. Its SHA-256 is the lock identity used by every preregistration and run artifact.

A changed corpus, role, license, policy, or membership requires a new lock revision and a new lock SHA-256. Files with the old lock SHA-256 MUST NOT be combined with files carrying the new lock without an explicit supersession record.

## 4. Exact `anvil.corpus-lock/v1` schema

Unknown top-level or entry fields are forbidden. Missing required fields, duplicate JSON keys, `NaN`, infinities, and non-canonical integer encodings are invalid.

### 4.1 Top-level fields

| Field | Type | Requirement |
|---|---|---|
| `schema` | string | Required; exact value `anvil.corpus-lock/v1` |
| `protocol_version` | integer | Required; exact value `1` |
| `lock_revision` | integer | Required; `>= 1` |
| `created_utc` | string | Required; RFC 3339 UTC with `Z` |
| `title` | string | Required; nonempty |
| `project` | object | Required |
| `lineage` | object | Required |
| `roles` | object | Required; role definitions and transition rules |
| `splits` | object | Required; exact role-to-corpus-ID membership |
| `entries` | array | Required; nonempty and sorted by `corpus_id` |
| `deduplication` | object | Required |
| `publisher_isolation` | object | Required |
| `acquisition` | object | Required |
| `aitdcc` | object | Required even when AITDCC is unused |
| `workflow` | object | Required |
| `artifacts` | object | Required |
| `promotion` | object | Required |

### 4.2 `project`

| Field | Type | Requirement |
|---|---|---|
| `name` | string | Required; exact value `ANVIL` |
| `repository` | string | Required; canonical repository URL |
| `protocol_path` | string | Required; repository-relative protocol path |
| `source_git_sha` | string | Required; 40 lowercase hex commit used to author the lock |
| `source_dirty` | boolean | Required; MUST be `false` |

### 4.3 `lineage`

| Field | Type | Requirement |
|---|---|---|
| `introduced_lock_revision` | integer | Required; equals `lock_revision` |
| `supersedes_lock_sha256` | string or null | Required; null for the first lock |
| `change_reason` | string or null | Required; null for the first lock, otherwise nonempty |
| `replacement_policy` | string | Required; fixed value `new-lock-only-no-silent-splicing` |

### 4.4 `roles`

| Field | Type | Requirement |
|---|---|---|
| `allowed` | array of string | Required; sorted unique role enum values |
| `default_transition` | object | Required; maps each consumable role to its post-open role |
| `forbidden_transitions` | array of string | Required; includes at least `heldout->heldout` and `external_test->external_test` |

`default_transition` MUST map `heldout` and `external_test` to `known_stress`.

### 4.5 `splits`

The object has exactly these keys:

```text
discovery
known_stress
heldout
external_development
external_test
external_anchor
```

Each value is a sorted unique array of entry `corpus_id` values. The union MUST equal the set of `entries[].corpus_id`, and each ID MUST appear in exactly one split. Each entry's `role` MUST equal the split containing it.

### 4.6 Entry fields

| Field | Type | Requirement |
|---|---|---|
| `corpus_id` | string | Required; regex `^[a-z0-9][a-z0-9._-]{2,63}$` |
| `display_name` | string | Required |
| `role` | string | Required; one current role enum |
| `class` | string | Required; controlled corpus class |
| `selection_frozen_utc` | string | Required; RFC 3339 UTC |
| `role_rationale` | string | Required |
| `independence` | object | Required |
| `license` | object | Required |
| `source` | object | Required |
| `content` | object | Required |
| `derivation` | object | Required |
| `validity` | object | Required |
| `synthetic` | object or null | Required; non-null exactly for generated entries |
| `lineage` | object | Required |

Controlled `class` values are:

```text
structured_json
structured_ndjson
text_log
source_code
executable
sqlite
numeric_telemetry
random_incompressible
repeated
mixed_validity
malformed_stream
external_mixed
external_test
```

### 4.7 Entry `independence`

| Field | Type | Requirement |
|---|---|---|
| `publisher_id` | string | Required; stable organization/vendor identity |
| `project_id` | string | Required; stable dataset/project identity |
| `release_family_id` | string | Required; shared release line or dataset family |
| `producer_id` | string or null | Required; compiler, sensor project, or generator producer where applicable |
| `schema_cluster_id` | string | Required; `not-applicable` is allowed for untyped data |
| `exact_duplicate_group_id` | string | Required; singleton is the object's SHA-256 identity |
| `near_duplicate_group_ids` | array of string | Required; sorted and unique |
| `audit_artifact_sha256` | string | Required; SHA-256 of the pairwise independence report |

### 4.8 Entry `license`

| Field | Type | Requirement |
|---|---|---|
| `status` | string | Required: `permitted`, `public-domain`, `owned`, or `unknown-no-redistribution` |
| `name` | string | Required |
| `spdx` | string or null | Required; SPDX identifier when known |
| `url` | string or null | Required; canonical license or terms URL |
| `redistribution_allowed` | boolean or null | Required; null when unknown |
| `attribution_required` | boolean | Required |
| `attribution_text` | string or null | Required when attribution is required |

`unknown-no-redistribution` entries MUST be fetched outside the repository and MUST NOT be uploaded as artifacts.

### 4.9 Entry `source`

| Field | Type | Requirement |
|---|---|---|
| `kind` | string | Required: `git-raw`, `https-file`, `archive-extract`, or `generate` |
| `repository` | string or null | Required for `git-raw`; otherwise null unless a canonical repository is recorded |
| `commit` | string or null | Required 40-hex commit for `git-raw`; otherwise null |
| `path` | string or null | Required repository path for `git-raw`; otherwise null |
| `url` | string or null | Required for `git-raw`, `https-file`, and `archive-extract`; otherwise null |
| `archive` | object or null | Required non-null for `archive-extract`; otherwise null |

`source.archive` has exactly:

| Field | Type | Requirement |
|---|---|---|
| `archive_url` | string | Required |
| `archive_bytes` | integer | Required; `> 0` |
| `archive_sha256` | string | Required; 64 lowercase hex |
| `member_name` | string | Required; exact archive member basename/path |
| `member_bytes` | integer | Required; `> 0` |
| `member_sha256` | string | Required; 64 lowercase hex |
| `member_crc32` | string or null | Required; 8 lowercase hex for ZIP members, otherwise null |
| `extractor` | string | Required |
| `extractor_version` | string | Required |

Archive extraction MUST reject absolute paths, parent traversal, symlinks, and ambiguous duplicate member names.

### 4.10 Entry `content`

| Field | Type | Requirement |
|---|---|---|
| `canonical_filename` | string | Required; basename used inside the remote run |
| `bytes` | integer | Required; `>= 0` |
| `sha256` | string | Required; 64 lowercase hex of exact bytes |
| `git_blob_sha1` | string or null | Required; 40 lowercase hex for `git-raw`, otherwise null |
| `media_type` | string | Required |

For a Git object, CI computes:

```text
git_blob_sha1 = SHA1("blob " + decimal_byte_length + NUL + file_bytes)
```

Both `git_blob_sha1` and `sha256` MUST match the lock.

### 4.11 Entry `derivation`

| Field | Type | Requirement |
|---|---|---|
| `transform_chain` | array of object | Required; empty means the source bytes are the measured bytes |

Each transform step has exactly:

```text
kind
tool
tool_version
tool_sha256
parameters_sha256
input_sha256
output_sha256
```

Any newline normalization, decompression, truncation, concatenation, record injection, or mixed-validity construction MUST be represented as an explicit transform step. Silent preprocessing is forbidden.

### 4.12 Entry `validity`

| Field | Type | Requirement |
|---|---|---|
| `framing` | string | Required: `none`, `json-document`, `ndjson`, `log-lines`, `sqlite`, or `fixed-record` |
| `total_records` | integer or null | Required; null when not applicable |
| `valid_records` | integer or null | Required; null when not applicable |
| `residual_records` | integer or null | Required; null when not applicable |
| `final_newline` | boolean or null | Required; null when not applicable |
| `schema_signature_sha256` | string or null | Required; null only when `schema_cluster_id` is `not-applicable` |
| `counts_source` | string | Required: `curator`, `locked-generator`, or `not-applicable` |
| `counts_artifact_sha256` | string or null | Required when record counts are present |

Counts describe validity only. They MUST NOT include compressed size, throughput, or transform ranking.

### 4.13 Entry `synthetic`

The object has exactly:

```text
generator_path
generator_git_blob_sha1
generator_sha256
seed
parameters
parameters_sha256
runtime
```

`parameters` MUST be a JSON object. `parameters_sha256` is the SHA-256 of its canonical JSON representation. `runtime` MUST include the generator language and exact version. Direct external objects use null.

### 4.14 Entry `lineage`

| Field | Type | Requirement |
|---|---|---|
| `introduced_lock_revision` | integer | Required |
| `previous_roles` | array of string | Required; sorted unique historical roles |
| `replaces_corpus_id` | string or null | Required |
| `replacement_reason` | string or null | Required when replacing an object |

A replacement does not reset the independence audit. The new object MUST be checked against all open and sealed objects.

## 5. Near-duplicate and publisher split rules

### 5.1 Pairwise audit

The lock MUST include a complete pairwise audit over all entries. A held-out or external-test object is blocked when any of the following holds against an open discovery, known-stress, external-development, or external-anchor object:

1. identical SHA-256;
2. identical Git blob SHA-1 and byte length;
3. a shared exact raw 4 KiB block;
4. a shared exact 4 KiB block after CR removal before LF only;
5. text MinHash estimated Jaccard similarity `>= 0.20` using 128 BLAKE2b-derived permutations over normalized 5-token shingles;
6. the same schema cluster plus the same publisher or release family;
7. the same repository owner, project, or release family;
8. shared generator lineage for synthetic data.

Text normalization for duplicate detection is limited to CR-before-LF removal, Unicode NFC, whitespace collapse, and lowercase ASCII alphanumeric tokenization. It MUST NOT be used to create measured bytes.

A duplicate match MAY be adjudicated only by placing both objects in the same independence group. It MUST NOT be waived to count as independent evidence.

### 5.2 Publisher isolation

For every held-out or external-test entry:

- `publisher_id`, `project_id`, and `release_family_id` MUST be new relative to all open roles;
- `schema_cluster_id` MUST be new for structured data;
- source repository owner and organization MUST be new;
- executable producer/compiler family MUST not be the sole selected dimension of diversity;
- selection MUST be by provenance, not by observed compression result.

A different filename or repository mirror does not create a new publisher family.

### 5.3 Required audit artifacts

The following machine-readable reports are mandatory:

```text
dedup-report.json
publisher-split-report.json
license-report.json
```

Each MUST identify the tool, tool version/hash, parameters hash, every compared pair, the decision, and evidence hashes.

## 6. Fail-closed acquisition

1. CI checks out only the frozen preregistration and implementation authorized for the current role.
2. CI reads the exact `corpus-lock.json` and verifies its companion SHA-256 before creating a corpus directory.
3. The fetcher accepts only locked corpus IDs. It MUST NOT accept arbitrary URLs, paths, or user-supplied identities.
4. The fetcher resolves the lock to one source object and applies the declared derivation chain.
5. It streams the download while enforcing the expected byte cap and exact final length.
6. Redirects to hosts outside the lock's allowed host set are rejected.
7. The file is written to a temporary path. No codec may access that path.
8. Size, SHA-256, Git blob when applicable, and archive-member identity are verified.
9. For a transform chain, every step's input/output hash and parameters hash is verified.
10. Only a fully verified file is atomically promoted to the measured corpus directory.
11. A mismatch deletes the temporary bytes, records `INVALID_INFRA`, and permits at most one clean retry. A second mismatch is terminal for that run.
12. Cached bytes MAY be reused only when their SHA-256 and all archive/derivation identities are reverified and the original acquisition provenance is retained.
13. Codec execution MUST begin only after all entries required by the current workflow role pass.

Upstream disappearance, TLS failure, changed bytes, or an unavailable archive is `INVALID_INFRA`, never a compression loss or win.

## 7. Contamination ledger

The append-only ledger is `corpus-access-ledger.jsonl`. Each line is one canonical JSON object with exactly these fields:

```text
event_id
utc
actor
action
lock_sha256
lock_revision
preregistration_sha256
implementation_git_sha
workflow_git_sha
run_id
job_id
corpus_id
role_at_event
host_class
heavy_measurement
result_artifact_sha256
reason
previous_event_sha256
event_sha256
```

Field rules:

- `action` is one of `lock-created`, `identity-fetch`, `identity-inspected`, `corpus-fetched`, `local-measurement`, `remote-measurement`, `result-viewed`, `attempted-fetch`, `invalidated`, or `role-transition`.
- `host_class` is `local-control-plane`, `github-actions`, or `approved-remote-ci`.
- `heavy_measurement` is boolean.
- `previous_event_sha256` is null for the first event and otherwise the prior event hash.
- `event_sha256` is SHA-256 over the canonical event object with `event_sha256` set to null.
- The ledger is append-only. Corrections are new invalidating events; prior lines are never rewritten.

Viewing identity metadata does not consume a held-out role. Viewing compression bytes, anatomy, timings, or rankings does. A forbidden local held-out measurement burns the object even if the run crashes before producing a verdict.

## 8. AITDCC 2026 handling

The AITDCC lock MUST preserve original labels:

- A-H: `external_development`;
- I-P: `external_test`.

The lock MUST pin the official dataset page identity, official `SHA256SUMS` identity, every file name, byte length, and SHA-256. Individual I-P files are canonical; a complete archive is not a substitute for exact per-file identity.

A-H MAY be used for external development. If A-H influenced the implementation, results MUST be described as current independent evaluation, not reproduction of competition-time tuning.

I-P MUST remain unopened until:

1. corpus lock and preregistration are frozen;
2. discovery and known-stress gates pass;
3. implementation, workflow, gate rules, and references are frozen by SHA-256;
4. the protected held-out workflow is authorized.

Because A-P is public, I-P is historical-split and procedural blinding, not cryptographic blindness. Reports MUST state this limitation.

AITDCC evaluation MUST also enforce:

- peak memory `<= 8 GiB`;
- decompressor binary `<= 1 MiB`;
- exact round trip;
- complete per-file rows;
- official attribution to the AITDCC dataset and paper.

Passing AITDCC does not replace a new mechanism-specific held-out family when the claim concerns structured, mixed-validity, numeric, or executable routing.

## 9. Workflow split

### 9.1 `discovery`

Allowed roles:

```text
discovery
```

Required properties:

- checks out the frozen source and preregistration;
- may tune and iterate only within the frozen mechanism contract;
- MUST NOT possess held-out credentials, URLs, cache entries, or result artifacts;
- emits discovery and ablation evidence only;
- cannot emit a production PASS.

### 9.2 `known_stress`

Allowed roles:

```text
discovery
known_stress
```

Required properties:

- runs after a frozen implementation SHA;
- may diagnose failures and fix correctness/performance only when scientific semantics remain frozen;
- MUST report every open role used;
- cannot count known adversarial wins as generalization.

### 9.3 `heldout`

Allowed roles:

```text
heldout
external_test
```

Required properties:

- separate protected workflow and environment;
- exact frozen implementation, workflow, preregistration, corpus lock, and gate-rule hashes;
- no source checkout from a mutable default branch;
- one authorized execution for the semantic candidate;
- no output inspection before all identity checks pass;
- after any result view, every opened object transitions to `known_stress`;
- infrastructure-only reruns require a new workflow SHA and proof that no scientific code or corpus outcome changed.

A scientific fix after access MUST use a new held-out family.

### 9.4 `frontier`

Allowed roles:

```text
external_anchor
external_development
external_test
known_stress
```

This job runs only after held-out authorization. It may reconstruct the full Pareto frontier but MUST NOT retrofit held-out claims or hide known failures.

## 10. Machine-readable artifacts

Every remote validation run MUST upload the following logical artifacts under `if: always()`:

```text
corpus-lock.json
corpus-lock.sha256
acquisition-provenance.json
corpus-access-ledger.jsonl
dedup-report.json
publisher-split-report.json
license-report.json
runner-manifest.json
build-manifest.json
rows.jsonl
raw-reps.csv
bytes.csv
regret.csv
pareto.csv
gates.json
verdict.json
artifact-manifest.json
```

### 10.1 `acquisition-provenance.json`

Top-level fields:

```text
schema
lock_sha256
lock_revision
run_id
acquired_utc
entries
fetcher_name
fetcher_version
fetcher_git_blob_sha1
fetcher_sha256
overall_status
```

Each `entries` item MUST include:

```text
corpus_id
source_kind
url
archive_sha256
archive_bytes
member_name
member_crc32
bytes
sha256
git_blob_sha1
transform_steps
status
error
```

### 10.2 `runner-manifest.json`

MUST include:

```text
repository
workflow_git_sha
workflow_run_id
run_attempt
job_id
runner_os
runner_image
runner_image_digest
kernel
cpu_model
cpu_flags
logical_cpu_count
memory_bytes
affinity
compiler
compiler_version
build_flags
python_version
brotli_version
brotli_package_version
xz_version
zstd_version
libsais_version
hardware_class
```

### 10.3 `build-manifest.json`

MUST include:

```text
implementation_git_sha
source_tree_dirty
source_blob_sha256
binary_path
binary_bytes
binary_sha256
linked_libraries
build_command
build_toolchain
preregistration_sha256
gate_rules_sha256
corpus_lock_sha256
```

### 10.4 `rows.jsonl`

Every file/arm/repetition MUST produce one row. Required fields:

```text
schema
run_id
job_id
corpus_id
corpus_role
class
input_bytes
input_sha256
arm
selected
fallback_reason
complete_compressed_bytes
compressed_sha256
roundtrip
encode_seconds
decode_seconds
router_seconds
transform_seconds
backend_seconds
encode_peak_rss_bytes
decode_peak_rss_bytes
decoder_binary_bytes
repetition
measurement_order
timing_status
reference_arm
reference_complete_bytes
same_quality_fallback_regret_bytes
frontier_oracle_regret_bytes
```

Empty numeric metrics MUST be JSON null, never zero.

### 10.5 `bytes.csv`

One deterministic row per file and arm:

```text
corpus_id,corpus_role,class,input_bytes,input_sha256,arm,selected,complete_compressed_bytes,compressed_sha256,roundtrip
```

Repeated deterministic runs MUST have identical size and output hash.

### 10.6 `regret.csv`

One row per file and selected policy:

```text
corpus_id,reference_arm,reference_bytes,selected_bytes,same_quality_fallback_regret_bytes,frontier_oracle_regret_bytes,exploration_encode_seconds,raw_only_encode_seconds,exploration_encode_regret_seconds,exploration_peak_rss_regret_bytes
```

### 10.7 `pareto.csv`

Required fields:

```text
corpus_id,corpus_role,arm,complete_compressed_bytes,encode_seconds,decode_seconds,peak_rss_bytes,decoder_binary_bytes,dominated,dominated_by,confidence_status
```

### 10.8 `gates.json`

Required fields:

```text
schema
lock_sha256
preregistration_sha256
implementation_git_sha
workflow_git_sha
thresholds
per_gate
blockers
```

`per_gate` MUST include `correctness`, `adversarial`, `discovery`, `ablation`, `heldout`, and `frontier` entries. Each entry MUST include `status`, `evidence`, and `reason`.

### 10.9 `verdict.json`

Required fields:

```text
schema
lock_sha256
implementation_git_sha
workflow_git_sha
status
production_authorized
heldout_role_transition
aggregate
per_file_summary
catastrophic_rows
invalid_rows
blocked_rows
next_allowed_action
```

`status` is exactly one of:

```text
PASS_DISCOVERY
PASS_KNOWN_STRESS
PASS_HELDOUT
PASS_FRONTIER
FAIL_SCIENTIFIC
INVALID_INFRA
BLOCKED_AMBIENT
```

`PASS_HELDOUT` MUST NOT set `production_authorized` to true. Only a later integrated `PASS_FRONTIER` verdict plus repository verification may do so.

### 10.10 `artifact-manifest.json`

MUST contain every uploaded artifact with:

```text
relative_path
bytes
sha256
media_type
producer
producer_version
```

Raw corpus bytes MUST NOT be uploaded when license forbids redistribution.

## 11. Promotion blockers

The following are hard blockers. No aggregate win, majority vote, favorable timing sample, or later reinterpretation may override them.

### Identity and role blockers

- **PB-01:** Missing, malformed, duplicated, or stale corpus lock.
- **PB-02:** Lock SHA-256, preregistration SHA-256, implementation SHA, workflow SHA, or gate-rule SHA does not match the run binding.
- **PB-03:** Any measured object is absent from the lock or its role does not match the workflow split.
- **PB-04:** Any required size, SHA-256, Git blob, archive, member, or derivation identity is missing or mismatched.
- **PB-05:** Codec execution began before all required objects passed acquisition.
- **PB-06:** A held-out or external-test object was fetched or measured locally before authorization.
- **PB-07:** Held-out anatomy or results influenced mechanism, arm, threshold, or implementation choices.
- **PB-08:** The same held-out family is reused after scientific code changes or a role transition.
- **PB-09:** The contamination ledger is missing, non-append-only, unchained, or inconsistent with workflow access.

## Independence and license blockers

- **PB-10:** Exact, near-duplicate, schema-family, publisher, project, release, or generator-lineage conflict exists across supposedly independent evidence.
- **PB-11:** A duplicate was waived to inflate the independent-family count instead of being grouped.
- **PB-12:** License terms forbid redistribution but corpus bytes or derived identifying artifacts were uploaded.
- **PB-13:** Required attribution is absent.
- **PB-14:** AITDCC A-H/I-P labels are conflated, or the official split and attribution are not preserved.

## Evidence and execution blockers

- **PB-15:** Any valid input fails exact round trip or any malformed input causes a crash, hang, OOM, unsafe allocation, wrong output, or prohibited acceptance.
- **PB-16:** Raw fallback, complete framing/model/dictionary accounting, or decoder-visible metadata is missing.
- **PB-17:** Per-file, invalid, skipped, blocked, or catastrophic rows are omitted, dropped as outliers, or counted as wins/ties.
- **PB-18:** Deterministic compressed bytes or hashes differ across repetitions without an explicit investigation and new series identity.
- **PB-19:** A/B timing came from different jobs, environments, builds, or reference versions without a valid paired design.
- **PB-20:** Raw repetitions, runner identity, toolchain identity, uncertainty, or comparability flags are absent.
- **PB-21:** A claim is based only on ratio while throughput, RSS, decoder footprint, or total architectural cost is required.
- **PB-22:** A proposed frontier point is dominated, or confidence is insufficient to distinguish it from a dominating point.
- **PB-23:** A catastrophic per-file regression is hidden by aggregate totals, file-count wins, or fallback selection.
- **PB-24:** AITDCC exceeds 8 GiB peak memory or 1 MiB decompressor size, regardless of compression ratio.
- **PB-25:** Negative, invalid, or infrastructure-failure evidence is overwritten, relabeled as a scientific loss, or omitted from the final ruling.
- **PB-26:** No genuinely new held-out family exists for a claim that requires generalization.
- **PB-27:** Production integration would change established wire behavior without byte-identity/regression evidence for unaffected modes.
- **PB-28:** Required lint, typecheck, build, correctness, fuzz, malformed-input, and full-cost verification has not completed for the proposed integration.

## 12. Promotion sequence

A candidate MUST follow this order:

1. Immutable corpus lock and contamination ledger.
2. Frozen preregistration, arms, thresholds, and gate rules.
3. Correctness and malformed-input gate.
4. Fuzz/sanitizer/adversarial gate.
5. Discovery and ablation gates.
6. Frozen implementation and workflow identity.
7. One-shot new held-out gate.
8. Full external/frontier reconstruction.
9. Legacy-mode byte-identity and integration verification.
10. Final source-of-record result preserving PASS, FAIL, INVALID, and BLOCKED states.

Discovery PASS authorizes only a held-out freeze. Known-stress PASS cannot authorize production. Held-out PASS authorizes only a frontier run. Frontier PASS still requires integration and repository verification.

## 13. Minimum new held-out portfolio for broad claims

A broad general-purpose claim requires at least:

- three independently published structured JSON/NDJSON families;
- two independently published mixed-validity or malformed-source families with substantial residual material;
- three executables spanning at least two unrelated producers and two unrelated toolchains;
- two real numeric/telemetry or scientific streams not derived from ANVIL synthetic generators;
- standard external anchors and AITDCC external testing in their declared roles.

Selection MUST use only provenance, size, format, validity, license, and independence criteria. Compression outcomes MUST NOT influence selection. If these families do not exist and pass the lock audit, broad held-out promotion remains blocked.
