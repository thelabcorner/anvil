# A48 original-run forensic closeout — cancelled run `44380412243` (18 November 2026)

**Nature:** independent, read-only recomputation from the actual downloaded artifact ZIP and GitHub job logs. This document is a *post-hoc partial-evidence audit*, **not** a result of the project's frozen full A48 classifiers, not a new ANVIL benchmark, and not evidence of all-axis Pareto dominance.

## Identifiers and source hierarchy

- [Original workflow run](https://github.com/thelabcorner/anvil/actions/runs/44380412243) — source workflow SHA `429364e32161c766d827e0f4c03314b3eaa53e31`; candidate SHA `2c5eeb9e570b1e1162cbff98d29e0a978edeca92`. This is NOT the updated draft PR #23 version.
- [Original archive artifact 14480494484](https://github.com/thelabcorner/anvil/actions/runs/44380412243/artifacts/14480494484), name `anvil-a48-44380412243`, 427,263,976 B. ZIP SHA256 `71b6b288b1a2d3f2ae0adf132287e0b1a790642e937ba9526018f8b678ea3eab`, matches GitHub job log; 140 ZIP entries.
- Workflow cancellation: 23:25:57Z at the approximately 240-minute job cap. Build, corpus integrity, and setup steps passed; the per-file grid step was cancelled. Frozen arbiter and final ruling steps were **skipped**. `if: always()` artifact upload succeeded at 23:26:24Z.
- Runner class: ubuntu-24.04, Intel Xeon Platinum 8573C, four advertised logical CPUs. Corpus: canonical 12-file Silesia; each source MD5 was validated by the workflow; original raw paired summaries are retained inside the uploaded ZIP.
- This independent pass read the ZIP directly, extracted JSON/CSV without rerunning the codec, recomputed paired medians and R-2 comparisons, and verified the archive's SHA-256.

## Surviving evidence — exact coverage

**4 complete files:** `dickens`, `mozilla`, `mr`, `nci`. Each has all six paired summaries: decode vs xz and Brotli, encode vs xz and Brotli, and decode/encode A/A null controls; 9 decode and 7 encode paired repetitions as preregistered. All 4 had `valid=True` and `rt=True` from `grid-build.log`; independently recomputed raw-pair medians agree with their JSON summaries, all paired `timing_valid` flags are true and both null CIs for each file enclose 1.0.

**1 partial file:** `ooffice` contains five of six JSON summaries; the encode A/A null is missing. It is EXCLUDED from classification. **7 not reached:** `osdb`, `reymont`, `samba`, `sao`, `webster`, `x-ray`, `xml`.

Neither `benchmark-suite.csv` nor final `grid-out/grid.csv`, timing-validity summary, or frozen arbiter outputs existed. The original A48 **12-file H1/H0 is therefore not established**.

### Independent per-file recomputation — measured MB/s

Every row is **ANVIL / xz -9e / Brotli q11/lgwin30**, in that order. Ratios are complete compressed bytes divided by the canonical file's input bytes; throughput is input MB divided by paired median wall seconds. Rounded MB/s below preserve the exact paired source summaries in the artifact.

| File | Complete compressed bytes: ANVIL / xz / Brotli | Encode MB/s: ANVIL / xz / Brotli | Decode MB/s: ANVIL / xz / Brotli | Frozen R-2 semantics on the **two available** references |
|---|---|---|---|---|
| dickens | 2,636,635 / 2,831,212 / 2,827,779 | 0.444 / 1.236 / 0.490 | 20.405 / 73.170 / 226.142 | encode FRONT-CROSSING; decode FRONT-CROSSING |
| mozilla | 13,806,173 / 13,376,240 / 13,806,141 | 0.326 / 1.731 / 0.351 | 159.585 / 55.187 / 128.169 | encode DOMINATED; decode FRONT-CROSSING |
| mr | 2,413,023 / 2,751,892 / 2,823,137 | 0.434 / 1.330 / 0.464 | 24.491 / 56.039 / 172.696 | encode FRONT-CROSSING; decode FRONT-CROSSING |
| nci | 1,497,442 / 1,449,272 / 1,497,411 | 0.489 / 1.431 / 0.511 | 390.915 / 281.826 / 396.866 | encode DOMINATED; decode DOMINATED |

**Partial four-file tally only:** 5 `FRONT-CROSSING`, 3 `DOMINATED` across 8 file-plane cells. This count is NOT A48's full 12-file result, NOT the historical frozen Class-A 468-cell figure, NOT the complete reference class, and NOT a newly validated Pareto win. In particular, dickens/mr get FRONT-CROSSING from the strict byte minimum despite decoding 11.1x/7.1x slower than Brotli; mozilla gets a decode-only crossing by being faster than both measured references despite a larger compressed size.

## New engineering measurement: where encoder time goes

The same-job paired median seconds for **ANVIL auto vs a pure Brotli q11 control** on the four completed files:

| File | ANVIL auto encode (s) | Brotli q11 encode (s) | Difference auto - Brotli (s) | Relative extra |
|---|---:|---:|---:|---:|
| dickens | 22.840 | 20.787 | +2.053 | +9.9% |
| mozilla | 156.708 | 146.103 | +10.605 | +7.3% |
| mr | 23.078 | 21.487 | +1.591 | +7.4% |
| nci | 68.896 | 65.650 | +3.246 | +4.9% |

The A48 encoder's `auto` method always generates the full Brotli q11 candidate and the BWT candidate; it then selects the shorter payload. These matched medians show that **Brotli q11 already accounts for about 90–95% of observed auto wall time** on the four files. This is directional attribution, not an independently timed pure-BWT decomposition. A cheap rule that *only* skips BWT on losers can at best recover a relatively modest observed difference; a strategy that can safely skip full Brotli on BWT winners has a potentially larger but unverified benefit and carries **byte-regret** risk.

The A64 hypothesis in the local ledger should be corrected: its proposed falsifier 'BWT is not the shortest payload on any of the six files where auto beats Brotli' is logically determined false by the code's min-payload selection. A useful non-tautological falsifier is whether an independently selected cheap predictor preserves the held-out complete-wire byte budget while achieving a preregistered encode-time reduction, or how often it guesses wrong. Preserve the exact size-first default; test a distinct explicit speed policy on a discovery/validation split.

## Source-level work and resource accounting

The original ZIP contains **75 regenerable `grid-out/work/` files** totaling **407.19 MiB compressed** and **779.12 MiB uncompressed**; the remaining 65 entries were approximately **0.263 MiB compressed**. The oversized upload stored redundant compressed streams, decoded Silesia outputs, and repeated reference encodings. Draft PR #23 now narrows artifact paths to raw CSV/JSON, manifests, source hashes and built binary, checkpoints each finished file atomically, and removes reproducible working output per file. This is a future protocol change: it was not executed in original A48.

**Secondary A53 protocol conflict:** `prototypes/i9-bwtinv/unbwt_bench.cpp` is Windows/QPC-based and includes `libsais_unbwt_aux_omp` in an optional OpenMP build, while ANVIL's current `CMakeLists.txt` compiles serial libsais and lacks the OpenMP compile/link flags. A proposed multithreaded LF oracle **cannot** be ruled by a benchmark pinned to exactly CPU 0. Evaluate serial algorithmic changes under single-core affinity; evaluate parallel variants with matching 2/4-core affinity, matched baseline, raw repetitions, correctness and working-set measurements, separately. This is a *protocol design observation*, not a demonstrated performance gain.

## Decision

- **Research:** `VERIFY` original A48's four-file partial observations, `PARK` unchanged A48 full rerun because the thin-grid positive is mathematically preordained on the six known byte-minimum files and its 240-minute sequential job exceeded its budget; `REFINE` the decode-vs-Brotli mechanism or held-out structured-ratio representation with a real falsifier.
- **Execution:** `PROCEED_WITH_GUARDS` on the draft audit, source cleanup and read-only evidence analysis; `TEST` only for a genuinely discriminating, pre-registered, manual GitHub Actions scout when publication is available. Do not bypass the session's recorded workflow publication restrictions.
- **Original project's unaltered goal:** full-wire correctness, paired encode/decode throughput, memory, decoder-only binary size, random-access/streaming and held-out generalization. No all-axis claim currently passes.

## Independent mechanism-attribution correction — C7 threshold was inert in A44/A46

After the partial-closeout, we inspected the exact GitHub job logs and the **pinned candidate source** rather than accepting the research ledger's description of the policy. [A44 cap-8 run 44370017321](https://github.com/thelabcorner/anvil/actions/runs/44370017321), Silesia job 135033950667, recorded `candidate_extra_args=[ --bwt-subblock=8388608]`. [A46 cap-4 run 44374141043](https://github.com/thelabcorner/anvil/actions/runs/44374141043), Silesia job 135048628180, recorded `candidate_extra_args=[ --bwt-subblock=4194304]`. **Neither supplied `--ratio-bwt-min-gain-pct`.**

At pinned candidate SHA `2c5eeb9e570b1e1162cbff98d29e0a978edeca92`, `src/anvil.cpp` defaults `ratio_bwt_min_gain_pct=0.0`; `encode_ratio_block()` makes the C7 minimum-gain check **only if `ratio_bwt_min_gain_pct > 0.0`**. The frozen `tools/reference_cost_gate.py` default candidate command uses `--parse=ratio --ratio-backend=auto --ratio-context=off --ratio-lines=off --bwt-aux=on` and appends only user-supplied `candidate_extra_args`. Therefore both jobs ran the **old byte-minimum policy**, not an active C7 decode-budgeted threshold. They still contain C7 source code, but that policy was dormant.

**Correct causal reading:** the cap-8/cap-4 subblock changes and existing C4/working-set decoder changes were active; crediting these exact runs' improvement to *C7 threshold activation* is invalid. Dedicated older trials explicitly passing a positive C7 gain threshold are separate evidence and remain valid. No new benchmark is required to establish this source/flag fact, but any publication or mechanism ledger should distinguish **source ancestry from active experimental treatment**. This is a factual reattribution, not a retraction of the measured compressed-byte, RSS or paired-time observations.
