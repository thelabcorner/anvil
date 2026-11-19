# A76 — External structured-validation candidates (not yet admitted)

**Status: candidate inventory only; no ANVIL test or heldout claim.** Recorded November 18, 2026. The scientific goal is to evaluate a frozen general-purpose structured-data mechanism on **non-sibling, source-identifiable** files after discovery has ceased. These candidates were discovered through public GitHub metadata and independently fetched at immutable commits; do not copy them into the ANVIL corpus lock or dispatch a job without source/license and gate review.

## Source candidates and immutable identities

| Corpus | Upstream repository and commit | Source path | Git blob SHA-1 | Verified bytes | Source/license notes |
|---|---|---|---|---:|---|
| npm CLI dependency graph | [npm/cli at 787f1520](https://github.com/npm/cli/blob/787f1520fa412e91a5792468f1c7f25b166867a6/package-lock.json) | `package-lock.json` | `e0d7e033db33bd3e2f776f3cee54918cccaf2c83` | 437,638 | Real package/dependency lockfile, npm CLI LICENSE identifies Artistic License 2.0; dependencies have their own licenses |
| JSON Resume example | [jviotti/binary-json-size-benchmark at 1075bc71](https://github.com/jviotti/binary-json-size-benchmark/blob/1075bc71c4e6e5aa67b87b70b67694bbbd6fc5ce/benchmark/jsonresume/document.json) | `benchmark/jsonresume/document.json` | `dd87510622284cb2de469ffab67ed38c49856e7f` | 3,916 | Adapted from SchemaStore, benchmark repo Apache-2.0; human-oriented nested JSON |
| ESLint configuration example | [jviotti/binary-json-size-benchmark at 1075bc71](https://github.com/jviotti/binary-json-size-benchmark/blob/1075bc71c4e6e5aa67b87b70b67694bbbd6fc5ce/benchmark/eslintrc/document.json) | `benchmark/eslintrc/document.json` | `d3895409aa74284273d165a438e1f65848ae683e` | 1,359 | Adapted from SchemaStore, benchmark repo Apache-2.0; contains tabs and nested settings |

**Byte-size correction:** JSON Resume is 3,916 UTF-8 bytes by GitHub's raw blob/tree metadata. An earlier GitHub text fetch returned 3,910 JavaScript string code units; character count is NOT byte count and cannot be used in compression or provenance accounting. The Git blob SHA-1 was correct and unchanged.

The two SchemaStore-derived documents are a **shared upstream family** and should not be counted as two statistically independent source families. These files are external to the ANVIL synthetic smoke generator. We confirmed blob identifiers and lengths at the specified commits using authenticated GitHub reads, but **did not run compression, execute a codec, or import these data into the frozen evaluation set**.

## Experimental eligibility controls

1. Do **not** disclose candidate content to mechanism developers after committing to a final holdout split. Metadata-only inspection for data licensing/provenance does not itself validate a compressor.
2. Check the actual upstream source/license and downstream attribution/PII needs before redistribution; source Git blob hashes do not prove license compliance or statistical independence.
3. The external benchmark includes tiny files unsuitable alone for meaningful throughput; use both a mid-sized real document and diverse small-document guards with a separate resource/latency protocol. Do not compare single-digit millisecond timings as if high-confidence speed evidence.
4. On approved GitHub Actions, fetch by **immutable commit**, verify the exact downloaded Git blob and SHA-256, preserve original raw bytes/formatting, and record independent acquisition provenance. Benchmark identical byte strings against properly pinned reference tools in the SAME job.
5. Compare *all* full-wire bytes and model/program overhead; test raw-output SHA-256, decode cost, peak RSS, decoder-only binary size, and varied schemas. A synthetic win is **discovery**, not evidence of general-purpose viability.

**Disposition:** REVIEW these external candidates for possible frozen validation admission; no code implementation, no new workflow, no dispatch. If additional third-party input is needed, prefer separate upstream authors over multiple siblings from a single benchmark source.
