# A70 protocol correction: independent acceptance contract

Date: 26 November 2026 UTC. This document is an experiment-control artifact,
not a benchmark result. The acceptance tests were authored by an independent
reviewer who does not own the scout, capture adapter, or workflow patches.
No local test, syntax check, compilation, codec run, or statistical computation
was performed while authoring them. Execute all acceptance work only in the
manual GitHub Actions protocol-only phase.

## Objective and scope

Repair three evidenced protocol defects without changing the source-frozen
statistical experiment: arbitrary nonzero exits accepted as malformed-input
success; seven-pair choices inconsistent with the nine-pair preregistration;
and loss of raw invocation evidence when a paired session is interrupted.
Output freshness and atomic completion are necessary controls for these fixes.

The original statistical driver remains byte-identical. Its independently
frozen SHA-256 is:

    9a42bf80f6a35bbec61f191a40daf416652e1ee9c35fd7461c5de13755209b08

The old codec baseline remains 2c5eeb9e570b1e1162cbff98d29e0a978edeca92.
Source review began from PR24 candidate f40a8ac28d87e851b67664131a801bd357d16a44.
The protocol correction must identify its own exact source commit; that commit
must not be silently substituted for the workflow-registration commit.

## Immutable measurement definition

| Item | Definition |
|---|---|
| Primary ratio | exp(median(log(candidate_seconds / control_seconds))) |
| Resampling | Complete paired log ratios, with replacement |
| Bootstrap samples | 20,000 |
| Schedule seed | 41246 |
| Bootstrap seed | 41246 XOR 0xA5A5A5A5 |
| Pair order | The unchanged original driver's seeded randrange(2) sequence |
| Warmups | Two symmetric rounds: control/candidate then candidate/control |
| Accepted A70 pair counts | 9 or 13 complete pairs |
| Practical threshold | epsilon 0.02, strict upper-CI comparison below 0.98 |
| Arm noise limit | 1.4826 * MAD / median, at most 0.15 |
| Ambient control | 5 repetitions, 250000 iterations, CV at most 0.10 |
| Output control | Canonical source SHA after every measured invocation |
| Timing boundary | Only the original driver's subprocess interval |
| Null controls | Old/old sessions before and after old/new |

No outliers may be removed. No bootstrap may turn an incomplete experiment into
an ordinary speed verdict. A changed estimator, schedule, confidence rule,
sample count, or source input requires a new preregistration.

## Discriminating hypotheses and acceptance

### A. Controlled outer-frame rejection

H1: the guard accepts only the source-established controlled rejection.
H0: a crash, usage error, timeout, or unrelated failure is counted as success.
The primary metric is zero false acceptances in the frozen negative table.

Accept only return code 1 with the exact diagnostic
"anvil: truncated block payload", optionally followed by one LF or one CRLF.
Reject return codes 0, 2, 7, -6, -9, -11, 124, 126, 127, 134, 137, and 139 even
if they carry the expected diagnostic. Reject an empty, different, prefixed,
multiline, or extra-blank-line diagnostic; timeout and launch exceptions;
and a final decoded output or .anvil-part left by the rejected command.
The guard truth table uses injected process results. Separate real GitHub
Actions subprocesses exercise the managed run helper's exit-code handling,
SIGTERM, and SIGSEGV, with core dumps disabled.

Preexisting truncation-output evidence is a dirty precondition: refuse before
decoder invocation and preserve it. Do not delete history and then report that
the decoder left no output.

This guard exercises ANVIL's outer frame truncation path. It does not prove
hostile BWT-payload safety, upgraded libsais malformed-input safety, fuzz
coverage, or full adoption correctness.

### B. Complete experiment and output ownership

H1: admissible A70 results contain 9 or 13 complete pairs and new output from
each measured invocation. H0: insufficient or stale evidence passes.

Invalid A70 pair counts must fail before experimental work. Valid sessions
contain exactly 18 or 26 verified timed invocations, with both arms exactly once
per pair. Warmups are recorded separately. Missing output or incorrect SHA
retains an error observation and elapsed evidence, but is not a verified arm.
The CLI, direct scout entry, direct paired entry, and capture entry are checked.
Fault-injected driver calls that omit or duplicate a post-run observation must
be refused before another command can create apparently complete coverage.

The capture adapter must remove only the designated per-invocation decoded
observation file before its timer. A no-write command cannot inherit correct
bytes from a warmup or prior measured invocation. Protected input/source and
capture artifact paths must never be deleted as observation files.

Reused raw, summary, journal, or state paths are refused before invocation.
Prior artifacts remain prior evidence, not a successful result of the refused
new attempt.

### C. Durable failure evidence and completion

H1: invocation evidence is durable before the next invocation, and completion
requires a committed state plus matching terminal journal event.
H0: evidence is written only at the end, or one completion indicator is enough.

An injected failure on the sixth measured invocation must leave five verified
timed observations, two complete pairs, and the exact failing pair/position.
A first-measurement timeout or launch failure leaves zero complete pairs and
an incomplete state. Output-hash failures retain the returned elapsed
observation but never verify it.
An actual journal-fsync fault is injected after the first measured observation;
the next command must not run and the result must remain incomplete.

Real isolated GitHub Actions subprocess tests send SIGTERM and SIGKILL after
three flushed verified observations. Those three observations must survive;
neither interruption may produce successful current-run completion.
SIGTERM should finalize INCOMPLETE; abrupt SIGKILL may leave RUNNING with
complete=false. Neither admits a scientific claim.

Inject a state-write failure after the COMPLETE journal record is flushed.
The adapter must raise, retain INCOMPLETE state, and refuse success. A terminal
COMPLETE event alone is explicitly insufficient. The scout consumer must require
both completion indicators, continuous journal sequence numbers, matching
last-event sequence, complete-pair coverage, and expected verified-arm count.

The real managed-timeout fixture includes a SIGTERM-ignoring parent and
grandchild. Timeout handling must stop the entire owned process group and
preserve process evidence. A briefly unreaped zombie counts as terminated, not
as a running workload.

### D. Exact statistical parity

H1: capture/freshness work preserves the original experiment.
H0: instrumentation changes measured intervals, pair assignment, or statistics.

The independent tests execute the unmodified source-frozen driver twice:
directly and through the adapter. Only external process/clock, affinity, and
ambient dependencies are mocked. The original main, schedule, estimator, and
bootstrap remain untouched.

Frozen fixtures use 9 and 13 pairs, heterogeneous deterministic durations, and
constant ratios 0.8 and 1.0. Output observation advances virtual time by 1024
seconds to expose accidental inclusion of hashing or journaling in the clock.
Primary acceptance is exact equality of baseline and wrapped schedules,
recorded elapsed values, raw CSV bytes, estimates, confidence bounds, noise
fields, scientific status, and output identities.

The secondary comparison to the mathematically ideal constant ratio allows an
absolute tolerance of 1e-10. This was specified before execution because
subtracting nearby large floating clock values can introduce roundoff. It
does not relax exact baseline-versus-wrapper parity.

## Evaluation ownership and operational gates

The owner freezes this document, tests/test_a70_capture_protocol.py, and the
necessary compatibility edits to tests/test_a70_version_scout.py outside the
implementation authors' edit scope before the first acceptance run.
Record exact test/evaluator source hashes in the run evidence. Changes after
seeing results require a documented correction, never silent test weakening.

The protocol-only workflow phase runs these tests and stops. It must not
download benchmark corpora, build codecs, or enter the scout phase. A later
explicit manual scout invocation must name the reviewed immutable candidate
SHA and preserve identical compiled ANVIL source/build settings between arms.
The revised adapter is one shared evaluator for both decoder arms.

Coverage boundaries: this independent suite implements the four acceptance
groups above. Full codec malformed-input safety, complete all-axis benchmark
acceptance, broader corpus representativeness, runner-host replication, and
interpretation of a future Mozilla negative-control effect remain later
evidence gates. These protocol tests do not claim those gates have passed.

Reserve bounded time for cleanup and artifact upload. Include journal JSONL,
state JSON, partial results, process stdout/stderr, and failure records in the
artifact paths. An always-upload step cannot recover observations never written.

Any failed acceptance gate blocks the scout. A passed protocol test suite is
evidence about orchestration only. A completed three-file version scout still
does not establish general-purpose Pareto dominance, full decoder footprint,
held-out correctness, or adoption safety.
