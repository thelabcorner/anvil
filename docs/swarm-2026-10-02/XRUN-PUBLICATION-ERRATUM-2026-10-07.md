# ANVIL R2 publication erratum — XRUN source closure (2026-10-07)

**Status:** publication-stage defect correction; NOT an R3 reclassification or performance measurement.
**Baseline:** `thelabcorner/anvil:main` at `b6a243c9657776058a37e5f0ee2eaae2ff925dff`.
**Workspace:** `ANVIL-PARETO-GATE-20261007`, isolated linked worktree. Original `ANVIL` working tree left untouched.
**Authority:** `docs/swarm-2026-10-02/FROZEN-CLOSEOUT-MATRIX-R2.md`; Gate A source/CI trust precedes all heavy CI and all Gate-B promotion claims.

## Frozen-source reconciliation

The working copy of `q1a_corpus_lock.py` in the original, intentionally dirty ANVIL checkout has been modified after the frozen R2. Its SHA-256 is `14452295619e25bdca9ba896183f18085ef4f35150970838b1ceec85adfd4701`; Git blob `773dc7200907e7dcfc78cbb1aad7dc5a3b928fb9`. The change contains an unfinished real-byte c9 implementation and related tests. **This is NOT the attested Q1a source**, and its bytes must not be silently substituted into XRUN.

The clean publication uses the **committed** original at `a5df2a5883b0d9cbe45757e59e78f35aa08c2385`:
- Q1a tool SHA-256 `c0c027e15f2a572d2816396129a1d0ed950bdd86dbf84d3c3d1e6310ccb399a4`, Git blob `0d2467c719daef37d301fa221fb8b4b3d069934b`.
- Fixture emitter SHA-256 `255ec7d56633bc4fa970024d446586cdd18afc3d8d7d5930257c85dc953dc3fb`, Git blob `07720d4fbba9e4f77143e873fb70d8a05154da43`.
- Expected fixture lock `0c4090c84d9de954cbd1bbf3b9b239f4c31413f596c87ef6c55eae62575ce17f`.
- Expected deterministic artifact manifest `feaa7848967d8245dc16f5d531fcc1e5c9f2898e941b6f5a424182301c4fec5c`.
- Expected summary `112add224d0017e6bbb7fad083424d90c3818526416de546f3000e00216dcdaa`.

These anchors are preserved, and **no local heavy benchmark or corpus test was run**.

## Workflow defect discovered

In the frozen draft, `OUT_DIR` was set to `${{ github.workspace }}/q1a-xrun-results`, then `mkdir -p "$OUT_DIR"` ran **before** `git status --porcelain` was asserted empty. **An empty directory alone does not make Git report a dirty checkout**, so this is a proactive isolation hardening, not a demonstrated failure of the current fresh-run assertion. Any files created there before a later cleanliness assertion would contaminate that assertion and confound its meaning.

The installed `.github/workflows/anvil-q1a-xrun.yml` changes only the output root to `${{ runner.temp }}/q1a-xrun-results` and adds a rationale comment. It remains:
- manual `workflow_dispatch` only, read-only permissions, no secrets;
- commit-pinned checkout and artifact upload;
- source/blob assert → bounded self-test → twice-built deterministic synthetic fixtures → fixed hash/semantic checks → failure-or-success evidence;
- no codec, no real corpus, no archive decompression, no network access in the Q1a tool itself.

**Workflow blob identity differs from the prospective draft blob by design.** That prospective blob must not be asserted as the installed workflow identity. Before any run, record the committed installed workflow blob and job SHA; if tool/emitter identity changes, stop and recompute *all* dependent anchors rather than weakening assertions.

## Result meanings and stop rules

XRUN is infrastructure attestation only. Expected fixture result is `CORPUS_BLOCKED`, `audit_complete=true`, `attestation_backed`, promotion authority false, 7 units, 30 edges, 113 unevaluated rows, and zero external-byte exposures.

- XRUN failure: Gate A remains closed, do not dispatch codec workloads; inspect job and artifact evidence.
- XRUN pass: Gate A opens for separately preregistered synthetic security/parity tasks only; Gate B remains blocked by c9 and independent held-out families.
- No frontier crossing or mechanism-level novelty may be inferred from a successful infrastructure job.
- The dirty local c9 source is **not** adopted or discarded by this publication.

## Verification checklist

- [x] Dedicated clean-worktree branch created from exact remote main.
- [x] Frozen Q1a tool SHA-256 and blob match.
- [x] Frozen emitter SHA-256 and blob match.
- [x] Original R2 freeze copied byte-exact.
- [x] Output-directory checkout contamination removed from workflow.
- [ ] Git commit and publication SHA recorded.
- [ ] Default-branch workflow is available for manual dispatch.
- [ ] Runner artifact manifest/summary checks pass.
- [ ] Gate A explicitly adjudicated from immutable remote artifacts.

Do **not** use this erratum as authorization to change any historical frontier metric or silently advance to the next experiment.
