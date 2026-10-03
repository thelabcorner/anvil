# ANVIL I10 Dense Frontier Vehicle Preregistration

**Status:** FROZEN BEFORE ANY DENSE-FRONTIER RUN
**Date:** 2026-09-24
**Production authorization:** none
**Purpose:** fill the canonical reference-density gap without changing codec source or wire format
**Parent evidence:** I9 frozen grid, I10-1A aux-unBWT closure, and the completed G4/G5 lineage

This document and `.github/workflows/anvil-i10-dense-frontier.yml` are an isolated research vehicle. They do not modify production source, ledgers, formats, or other workflows. A run is a measurement of frozen arms, not a promotion decision. The pinned `paired_bench.py` is used unchanged; the workflow generates and retains `observe.sh` to remove stale output before every warmup and timed invocation, while post-run size/SHA-256 validation remains outside the timed command.

## 1. Question and claim boundary

The vehicle asks:

> On the exact Silesia and enwik8 inputs, where do legacy ANVIL, aux-unBWT ANVIL, dense Brotli, dense zstd, and xz points sit when bytes, paired encode/decode time, peak RSS, and binary size are all retained?

The run may establish a reproducible descriptive frontier. It may not be cited as a complete Pareto crossing unless a later gate has all of the following:

1. exact deterministic bytes and round-trip hashes;
2. same-job paired timing with raw repetitions and confidence intervals;
3. valid ambient and A/A controls;
4. complete reference density for the claimed reference class;
5. comparable decoder-size accounting;
6. the same thread, affinity, and toolchain contract on both sides of every comparison.

The vehicle emits no automatic `FRONT-CROSSING` token. A byte win alone is always labelled `BYTE-WIN`, never `P-CROSSING`.

## 2. Frozen source and tooling identities

The workflow checks every identity below before building or measuring.

| Object | Frozen identity |
|---|---|
| Vehicle checkout | `github.sha` for the dispatching commit |
| Measurement tooling | `b6a243c9657776058a37e5f0ee2eaae2ff925dff` |
| `tools/paired_bench.py` blob | `f847c50e38d3b39b61deeb6b21bcf12e9c132fd5` |
| `tools/gha_fetch_corpus.py` blob | `c8f5cc71f9ade44db3b975d10aad39e901e26ebc` |
| `tools/bench_ratio.py` blob | `6f2393e5673714b8ffc8d9637f994505269c14fb` |
| Candidate tag label (not checkout authority) | `i10-aux-unbwt-final-v1` |
| Candidate commit | `0dce534e5df40945d518c9ef4a722f4a0ecfc0e6` |
| Candidate `src/anvil.cpp` blob | `755df76ae53795aea040f4c99784d1effb43b3ab` |
| Candidate `CMakeLists.txt` blob | `449ae00e0dd56cac5584e08fa725288870573667` |
| Candidate `tools/brotli_lw.cpp` blob | `c9a55893b827f6f45e6855a79514a91e0787346d` |
| Checkout action | `actions/checkout@11d5960a326750d5838078e36cf38b85af677262` |
| Upload action | `actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02` |

The candidate checkout is built outside the checkout tree. The immutable candidate commit is the checkout authority; the annotated tag is retained as a provenance label and is not silently substituted if its target changes. No source file is edited, generated into, or committed by the workflow.

## 3. Frozen platform and toolchain

The workflow requires `ubuntu-24.04`, four logical CPUs, and the following identities:

- Clang/Clang++ **18.1.3**;
- CMake **3.31.6**;
- Ninja **1.13.2**;
- Python **3.12.3**;
- Brotli development/runtime package **1.1.0-2build2**;
- zstd CLI **1.5.7**;
- libzstd development package **1.5.5+dfsg2-2build1.1**;
- xz/liblzma **5.4.5**;
- GNU `/usr/bin/time` and `util-linux` `taskset`.

The runner image label, kernel, CPU model, CPU flags, memory, and dirty state are recorded. A toolchain identity mismatch is an infrastructure failure, not a new timing series. A runner image change is recorded as a new series and is not silently spliced to an older run.

All codec processes are one-thread by contract. `taskset -c 0`/the paired driver affinity result is recorded. A missing or unsuccessful affinity setup is invalid timing evidence.

## 4. Frozen corpus identities

The corpus helper is the pinned `gha_fetch_corpus.py`. Every downloaded file is checked against the size and MD5 below, and the workflow records a SHA-256 for every source file in `corpus-manifest.json`.

### Silesia

Total source bytes: **211,938,580**.

| File | Bytes | MD5 |
|---|---:|---|
| dickens | 10,192,446 | `88334708559f6db57d79096bc0aca07e` |
| mozilla | 51,220,480 | `c7789a2097f1ff944b0c737430a339b3` |
| mr | 9,970,564 | `38e623e3093b7bf2003ca4b1bbc19927` |
| nci | 33,553,445 | `31f85bc8706f3c921104e7c169e2e2e1` |
| ooffice | 6,152,192 | `573c4ae915e36631d8f2dcffb9b9b66d` |
| osdb | 10,085,684 | `e734b0c48e6a982adfb5802da3032ecd` |
| reymont | 6,627,202 | `d8f54d78105079775f32d76dc55fc671` |
| samba | 21,606,400 | `154eaea7ea70e89f6339ff0abf4112ca` |
| sao | 7,251,944 | `79e95a22e18cd82b7e42bf91b380d30b` |
| webster | 41,458,703 | `474931ad907ac27bf962c75ded46c069` |
| x-ray | 8,474,240 | `9baec32ad14ec3eff487d254382cb91c` |
| xml | 5,345,280 | `9b09c0c80104adb8aae910b7d7db003e` |

Timing panel: `dickens`, `mozilla`, `nci`, `webster`, `xml`. The panel is fixed before measurement and is used only for paired throughput; byte/RSS census remains full-corpus.

### enwik8

Total source bytes: **100,000,000**.

| File | Bytes | MD5 |
|---|---:|---|
| enwik8 | 100,000,000 | `a1fa5ffddb56f4953e226637dabbb36a` |

Timing panel: the single canonical `enwik8` file. Byte/RSS census remains full-corpus by definition.

## 5. Frozen arms

All arms use complete payload accounting. No transformed intermediate, model, dictionary, or framing byte is excluded.

### ANVIL arms

Both arms use the frozen candidate binary and one source checkout:

- `anvil-legacy`: `--parse=ratio --ratio-backend=auto --ratio-context=off --ratio-lines=off --quiet`;
- `anvil-aux`: the same command plus `--bwt-aux=on`.

The decoder-only timing binary is a temporary linker-GC harness built from the frozen `src/anvil.cpp`; it is not a production decoder target. Full CLI size and decoder-harness size are both retained.

### Brotli arms

The vehicle uses libbrotli **1.1.0-2build2**, generic mode, `lgwin=30`, one thread, and exactly these qualities:

`q1`, `q4`, `q6`, `q9`, `q11`.

The q11/lw30 arm is checked against the frozen `brotli_lw` helper. Dense Brotli helpers are generated remotely from the workflow heredoc; their source and binary hashes are retained in the artifact.

### Zstd arms

The vehicle uses the pinned zstd CLI, one thread, and these regular levels:

`1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 22`.

It additionally measures the canonical approved long-window reference:

`zstd-ultra-22-long27` = `--ultra -22 --long=27`.

No zstd row is silently treated as equivalent to the Brotli or xz row; each command, level, long-window setting, and output hash is retained.

### xz arm

`xz-9e` uses `xz -9e -T1`. The compressed output and decoded output are both retained by hash. The vehicle does not add xz delta/BCJ side-channel controls; those remain outside this reference class.

## 6. Frozen byte and correctness gates

The following are hard failures, not warnings:

- any source size/MD5 mismatch;
- any candidate/tooling/blob identity mismatch;
- any nonzero codec exit;
- any source/output size mismatch;
- any source/output SHA-256 mismatch;
- any missing or duplicate arm/file cell;
- any payload that changes hash or size across a timing repetition;
- any ANVIL per-file byte mismatch against the frozen table below;
- any reference total mismatch against the pinned Linux baseline below.

### Frozen ANVIL bytes

| File | Legacy | Aux |
|---|---:|---:|
| dickens | 2,571,873 | 2,574,367 |
| mozilla | 13,806,173 | 13,806,173 |
| mr | 2,382,322 | 2,384,760 |
| nci | 1,365,712 | 1,369,810 |
| ooffice | 2,478,889 | 2,478,889 |
| osdb | 2,584,657 | 2,587,124 |
| reymont | 1,144,032 | 1,147,270 |
| samba | 3,761,931 | 3,761,931 |
| sao | 4,586,126 | 4,586,126 |
| webster | 7,317,361 | 7,319,896 |
| x-ray | 4,017,320 | 4,019,394 |
| xml | 430,599 | 430,599 |
| enwik8 | 23,534,368 | 23,537,422 |

### Frozen reference totals

| Corpus | Brotli q11/lw30 | zstd ultra-22/long27 | xz -9e |
|---|---:|---:|---:|
| Silesia | 49,383,136 | 52,364,240 | 48,456,004 |
| enwik8 | 24,810,180 | 25,272,471 | 24,831,648 |

Dense zstd levels and dense Brotli qualities have no pre-existing byte total in this vehicle. Their first-run totals are recorded as new deterministic evidence, subject to all identity and round-trip gates.

## 7. Measurement sequence

1. Initialize the evidence directory.
2. Checkout the vehicle, pinned tooling, and pinned candidate.
3. Assert source, blob, runner, and toolchain identities.
4. Build the frozen candidate, `brotli_lw`, and temporary dense/decode-only helpers outside source trees.
5. Fetch and verify the selected corpus.
6. Encode every arm/file once for the full-corpus byte and RSS census; decode each payload once and verify the source hash.
7. Run paired encode repetitions on the frozen timing panel through the generated observation wrapper.
8. Run paired decode repetitions on the same panel and payloads through the generated observation wrapper.
9. Measure per-process encode/decode peak RSS during the census.
10. Measure original and stripped binary sizes for the full CLIs, generated dense helpers, decoder-only harnesses, and system zstd/xz binaries.
11. Validate every raw observation against the prepared payload/source hash.
12. Emit `bytes.csv`, `census.csv`, `paired.csv`, `frontier-inputs.csv`, `pareto.csv`, `summary.json`, and `manifest.json`.

## 8. Paired timing and statistics

The control for every non-null comparison is `anvil-legacy` in the same job. The candidate is each other arm. The `anvil-legacy` self-comparison is the A/A null for every panel file and plane.

Defaults are frozen unless the manual input selects a larger repetition count:

- paired repetitions: **7 minimum**, allowed `7`, `9`, `13`;
- warmups: **2**, alternating order;
- schedule: seeded interleaved AB/BA, seed **41246** plus the frozen per-file offset;
- bootstrap samples: **20,000**;
- practical effect epsilon: **2%**;
- maximum arm robust CV: **15%**;
- ambient repetitions: **5**;
- ambient iterations: **250,000**;
- ambient maximum robust CV: **10%**;
- affinity: logical CPU **0**;
- command timeout: **3,600 s**.

No timing outlier is deleted. A blocked ambient probe, excessive CV, missing raw row, hash mismatch, or A/A failure is reported as blocked/invalid timing and cannot support a crossing claim.

For every plane, the report retains median seconds, MAD-derived robust CV, paired log-ratio, point ratio, bootstrap 95% CI, raw pair order, raw seconds, and observed output hash. Before every warmup and timed command, `observe.sh` unlinks the declared output path and `exec`s the codec; the pinned driver then records the output size and SHA-256 after the timed subprocess returns, and `verify_raw` compares every raw repetition to the prepared identity outside the timed interval. The wrapper source and artifact hash are retained with the run.

## 9. RSS and binary-size accounting

RSS is measured per process with GNU `time -f %M`; the workflow reports KiB and MiB without mixing encode and decode. One-shot census RSS is context, while paired timing remains the performance authority.

Binary-size records include:

- original and `strip --strip-unneeded` byte counts;
- SHA-256 after stripping;
- `size` output and ELF section summary;
- scope label `combined-cli`, `decoder-linker-gc-harness`, or `system-cli`.

The ANVIL and Brotli decoder-only sizes are linker-GC harnesses, not a promise of a minimal production decoder. System zstd/xz sizes are CLI sizes, not decoder-only sizes. Therefore binary size is retained and reported but is not treated as a directly comparable crossing axis until a separate decoder-only build policy is frozen.

## 10. Comparability and interpretation rules

1. Same-job paired ratios are comparable under the pinned toolchain and thread contract.
2. Absolute throughput from separate Silesia and enwik8 jobs is never spliced.
3. The timing panel is not silently presented as a full-corpus timing measurement; full-corpus bytes and RSS are reported separately.
4. The Windows I9 q-ladder, the earlier one-rep GHA baseline, and this dense run are distinct timing series.
5. `zstd-22-long27` is a distinct reference point from regular zstd level 22.
6. Full CLI size, decoder-harness size, and system CLI size carry their scope labels.
7. A non-dominated row is descriptive only until the full reference class, confidence, memory, and decoder-size gates are reviewed.
8. A failed or blocked run is preserved as evidence; it is not deleted, retuned, or replaced by a favorable subset.
9. No production transform ID, default, format change, or source integration is authorized by this vehicle.

The listed Brotli qualities, zstd tiers, and xz point remove the prior `GRID-THIN` gap for this explicitly frozen grid. Unmeasured Brotli windows, codecs, transforms, and decoder-size variants remain outside the claim.

## 11. Required artifact contract

Every run, including failed and blocked runs, must retain:

- `provenance.txt`, `pin-checks.json`, and `toolchain.txt`;
- `corpus-manifest.json` and `arm-contract.json`;
- `census.csv`, `bytes.csv`, and `prepare-summary.json`;
- `paired.csv`, `frontier-inputs.csv`, and `pareto.csv`;
- every raw paired repetition CSV and summary JSON;
- the generated `observe.sh` observation-integrity wrapper;
- `binary-sizes.tsv`, `binary-sizes.json`, and `size-report.txt`;
- `summary.json`, `manifest.json`, and `run-status.json`;
- command records, logs, hashes, and any partial failure record.

The artifact is the source of record. Human-readable job summaries are not authoritative.

## 12. Frozen disposition

This vehicle is authorized for manual remote execution only after the workflow file is committed and its pinned checkouts are available. It creates no local benchmark result and authorizes no production change. A dense run may support a later frontier gate; it cannot itself promote a mechanism.
