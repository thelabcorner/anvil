# I11 durable evidence set

This directory is the **durable evidence set** for ANVIL Iteration 11
(`docs/I11-PARETO-DOMINANCE-PREREG-2026-10-22.md`, §3.10).

## Layout

```
tests/i11-evidence/
  MANIFEST.json          machine-generated, all runs
  MANIFEST.md            machine-generated, human-readable
  runs/<run_id>/<artifact_dir>/...
                         the artefacts the GitHub Actions run uploaded
```

## Rules

1. **A claim that is not in `MANIFEST.md` is not a claim.** Every candidate's numbers must appear
   here, produced from the artefact the run uploaded.
2. `MANIFEST.json` is produced by `tools/i11_evidence_manifest.py` from the downloaded artefacts and
   **recomputes the gate checks independently of the arbiter's own `gates` map**, so the ruling can be
   re-derived from the raw files rather than restated from a summary.
3. A check that cannot be evaluated from the artefacts is reported as `null` and is **never** counted
   as a pass.
4. The bulky parts of an artefact (the packed payloads, the decoded outputs, the massif traces) are
   **not** committed; only the evidence files are. The full artefact remains in the Actions artefact
   store for its retention window.
5. Every manifest row carries the six mandatory fields: `run_id` + `run_url`, `job`, `workflow`,
   `commit_sha` (in `provenance`), `artifact` name, `corpus`.

## Rebuilding the manifest

```sh
python tools/i11_evidence_manifest.py tests/i11-evidence \
    --out tests/i11-evidence/MANIFEST.json \
    --md  tests/i11-evidence/MANIFEST.md
```

The tool measures nothing; it only re-emits what the runs produced.

## Predecessor

The I20 evidence set is at `tests/i20-evidence/` with the same layout and
`tools/i20_evidence_manifest.py`. It is the regression baseline for D5's arbiter extension.
