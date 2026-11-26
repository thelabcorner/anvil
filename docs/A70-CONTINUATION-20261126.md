# A70 continuation — 26 November 2026

## Current disposition

**Source repair published; computational validation not yet performed.**
The source and independent acceptance tests are on draft PR #24 at
[`420b88404dcb24369f5048deac7236729d5592ea`](https://github.com/thelabcorner/anvil/commit/420b88404dcb24369f5048deac7236729d5592ea),
a non-force fast-forward from reviewed head
`f40a8ac28d87e851b67664131a801bd357d16a44`.
The PR remains open and draft. The codec, build configuration, corpus tool,
frozen statistical runner and existing libsais import are unchanged by this
repair. No draft was merged, and the pre-existing dirty workstation checkout
was not edited, stashed, reset or cleaned.

The user explicitly authorized continuation through OXP GitHub tooling and
required all CPU validation on GitHub Actions. That includes unit/scalar tests,
syntax checks, builds, benchmarks, corpus processing, fuzzing and profiling.
Only source reading/editing and GitHub/API operations were performed locally.

## Published implementation

| Area | Change | Validation state |
|---|---|---|
| Truncated-frame refusal | Exact controlled exit 1, exact pinned diagnostic, no final or temporary output | Source reviewed; hosted test pending |
| Repetition admission | A70 accepts only complete 9 or 13 pairs at callable and CLI boundaries | Source reviewed; hosted test pending |
| Timing evidence | New capture wrapper journals and fsyncs starts, elapsed returns and verified arm observations | Source reviewed; hosted test pending |
| Frozen evaluator | Original paired_bench.py unchanged and SHA-256 pinned; independent differential parity fixtures | Hosted test pending |
| Output ownership | Fresh decoded output per invocation, protected input paths, refusal to reuse prior evidence paths | Hosted test pending |
| Failure handling | Group termination, streamed diagnostics, partial phase/file checkpoints and explicit incomplete state | Hosted test pending |
| Measurement budget | Absolute deadline; no session starts without its complete 600-second allowance plus cleanup | Hosted test pending |
| Independent tests | Exact parity at 9/13 pairs, controlled-exit matrix, stale output, fsync faults, TERM/KILL interruption and descendant cleanup | Authored and frozen; not executed |

The complete pre-outcome contract is
[`A70-PROTOCOL-CORRECTION-ACCEPTANCE-20261126.md`](A70-PROTOCOL-CORRECTION-ACCEPTANCE-20261126.md).
The historical preregistration has an appended correction, without rewriting
its earlier evidence or claiming an outcome.

## Actual publication boundary observed this continuation

These observations were made through OXP's existing authenticated `gh` tooling
on 26 November 2026, around 07:52–07:54 UTC:

1. `POST /repos/thelabcorner/anvil/git/trees` succeeded for the six source,
   test and documentation files. The returned tree was
   `ed4a99faf78619e4eca4faa410c10f379d1605d0`.
2. GitHub commit creation and the non-force PR24 branch update succeeded,
   producing commit `420b88404dcb24369f5048deac7236729d5592ea`.
3. A subsequent request to the same tree-creation endpoint, using that valid
   base tree and changing only
   `.github/workflows/anvil-a70-libsais-version-scout.yml`, returned HTTP 404
   with `Not Found`. No workflow commit or ref update followed that failure.
4. A read of the base tree returned HTTP 200, confirming that the base object
   and repository were accessible.
5. The authenticated repository response reported a public repository and
   push/admin permission. Its `X-OAuth-Scopes` header was exactly
   `gist, read:org, repo`; it did not include `workflow`.
6. The PR readback still reported open/draft at `420b884`. The Actions query for
   that source SHA returned zero runs at this checkpoint.

The HTTP 404 body itself does not name a permission reason. The observed scope
set lacks the permission GitHub documents for adding or updating workflow files,
which explains why code-write access alone is insufficient. This is a current,
reproducible access boundary, rather than an assumption copied from an older
session or an OXP automatic-approval rejection.

GitHub's primary documentation:
[OAuth scopes](https://docs.github.com/en/apps/oauth-apps/building-oauth-apps/scopes-for-oauth-apps)
and [workflow dispatch](https://docs.github.com/en/rest/actions/workflows).
The existing I20 stage-decomposition workflow is registered (ID `392077796`),
so the older session statement that no stage workflow exists is incorrect.
That older full-corpus job is not an A70 protocol-only entrypoint.

An independent read-only check of the existing registry and workflow sources
found no suitable generic A70 test lane. The two registered unittest workflows
hardcode I19 test files and expose no test selector; research-bench, Q1a and A48
have fixed unrelated commands or source pins. All 37 workflow files shared by
main and the repaired PR24 ref have identical Git blobs. The inherited A70 YAML
is unregistered, tests only the older scout file, has no protocol-only phase,
and lacks the new artifact/deadline controls. No unrelated workflow was
repurposed or dispatched as an access workaround.

## Exact workflow proposal, awaiting publication access

The reviewed replacement is preserved as ordinary, non-executing source here:

[`proposals/anvil-a70-libsais-version-scout.yml`](proposals/anvil-a70-libsais-version-scout.yml)

Its intended active path is
`.github/workflows/anvil-a70-libsais-version-scout.yml`.
Keeping the proposal under `docs/proposals/` makes the exact change reviewable;
it does not register a workflow or execute anything. The active workflow file
on the draft branch still contains its previous version because the attempted
workflow modification failed.

The proposed workflow has only a manual trigger, defaults to `protocol-only`,
requires an explicit full immutable candidate SHA, and accepts only 9 or 13
pairs. Protocol-only execution ends after source checks and the independent
test suite. A separate `scout` dispatch enables builds and measurement. The
45-minute job bounds preparation to 900 seconds, measurement to at most 1200
seconds with an absolute cutoff, and reserves a five-minute artifact upload
step. It includes journals, streamed logs, failure records and partial state.

## Next executable steps

The required access change is narrow: add GitHub OAuth `workflow` scope to the
OXP host's existing GitHub CLI authorization, or have an authorized maintainer
publish the exact reviewed workflow. For the CLI account used above, the
installed CLI's documented refresh command is:

```sh
gh auth refresh --hostname github.com --scopes workflow
```

The account owner completes the normal GitHub authorization in their browser.
No token or credential needs to be pasted into chat. This continuation did not
silently enlarge credentials or attempt another authentication path.

After access is available:

1. Publish the reviewed workflow, preserving the source repair and existing
   drafts, and register the same manual workflow on the default branch.
2. Read back the exact workflow source and registration. Dispatch
   `phase=protocol-only`, `reps=9`, with the frozen implementation SHA
   `420b88404dcb24369f5048deac7236729d5592ea` (or a subsequently reviewed correction).
3. Review hosted protocol logs and artifacts. A failed gate blocks the scout;
   fix the demonstrated issue and preserve the failed run evidence.
4. Only after protocol acceptance, separately dispatch the bounded three-file
   scout. Inspect complete journals, hashes, both A/A controls, timing validity,
   RSS diagnostics and the Mozilla negative control.
5. Use that evidence to decide whether stage attribution or a distinct mechanism
   is warranted. Do not promote infrastructure success into a decoder gain or
   strict Pareto claim.

No new speed, compatibility, safety, full-corpus, or Pareto result is claimed.
