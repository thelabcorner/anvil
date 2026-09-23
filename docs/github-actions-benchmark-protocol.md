# ANVIL GitHub Actions Benchmark Protocol

Status: normative statistical protocol for the GitHub Actions benchmark vehicle. The sanitized public repository and manual workflow were activated on 2026-09-23 at `https://github.com/thelabcorner/anvil`. The first remote smoke workflow now builds, fuzzes, and runs entirely on GitHub-hosted compute; this document remains the promotion gate for timing claims.

## 1. Purpose

All CPU-heavy ANVIL experimentation is moved off the workstation and homelab. The intended vehicle is a public GitHub repository using standard GitHub-hosted runners.

As of 2026-09-23, GitHub documents standard public `ubuntu-24.04` / `ubuntu-latest` runners as 4-vCPU, 16-GB RAM, 14-GB SSD fresh VMs, with standard-runner compute free and unlimited for public repositories. Larger runners remain billable and are not required for the initial ANVIL research loop.

This protocol deliberately distinguishes:
- **correctness evidence** — deterministic and portable;
- **byte-count evidence** — deterministic for a pinned build/configuration;
- **scout timing** — relative timing on shared hosted runners;
- **promotion timing** — stricter repeated evidence on a stable, explicitly identified runner class.

## 2. Never benchmark by comparing different runner jobs directly

A GitHub-hosted VM is not a fixed CPU laboratory. Two jobs can land on different host silicon or host-load conditions.

Any A/B performance statement must therefore compare arms **inside the same job on the same VM**.

Correct pattern:

```
job starts
  -> fingerprint runner
  -> build all A/B arms from one checkout/toolchain
  -> correctness gate
  -> warm both arms symmetrically
  -> interleave A/B measurements
  -> save every raw repetition
  -> bootstrap paired effect
  -> upload artifacts
job ends
```

Wrong pattern:

```
job A benchmarks main
job B benchmarks candidate
compare their absolute MB/s
```

Cross-job absolute timings are context only.

## 3. Runner fingerprint

Each benchmark artifact must contain at least:

```
git SHA / dirty state
workflow run ID / attempt
runner OS + image label
uname -a
lscpu
/proc/cpuinfo model name + relevant flags
logical CPU count
memory size
kernel
clang/gcc version
cmake/ninja version
Python version
Brotli version/commit
Zstd version
xz version
libsais tag/commit
CMake flags
ANVIL feature flags
thread count
taskset/affinity result if used
```

On x86, explicitly record at least:
`avx2 bmi1 bmi2 pclmulqdq popcnt sse4_2 avx512*`.

The result JSON should include a normalized `hardware_class` string. Paired measurements are valid within one run regardless of class; aggregating across runs should stratify by class.

## 4. Workflow classes

### correctness
No timing claim.

- Release build.
- direct encode/decode round trips;
- registry coverage;
- deterministic malformed-input checks;
- bounded fuzz campaign;
- optional ASan/UBSan in a separate job.

This can run on pull requests if runtime stays reasonable.

### scout
Manual `workflow_dispatch` initially.

- selected corpus subset;
- candidate vs exact baseline vs binding references;
- 2–3 warmups;
- at least 7 paired repetitions;
- randomized or alternating A/B order;
- one thread unless the experiment is explicitly multithreaded;
- raw wall times plus codec byte counts.

A scout can reject a hypothesis. A scout PASS promotes the hypothesis to a stronger run; it is not automatically a publication-grade throughput claim.

### pareto
Manual full-corpus run.

- canonical corpus + reference codecs;
- candidate/reference arms in the same job;
- enough repetitions to quantify noise;
- exact compressed bytes;
- encode/decode throughput;
- peak RSS;
- decoder binary size;
- complete Pareto recomputation;
- same-transform controls where a reversible transform is involved.

### microarch
For SIMD/hot-loop work.

- operation-specific harness;
- large enough working set and repetition count;
- scalar/SSE/AVX2 arms compiled in the same binary or same job;
- output/hash identity;
- CPU feature dispatch recorded;
- cycles/instruction/cache counters only if the hosted environment exposes them reliably.

Do not make a hardware-specific claim if the counter/affinity environment is unavailable or inconsistent.

## 5. Statistical treatment

For paired A/B timings, report:
- all raw repetitions;
- median;
- median absolute deviation or robust CV;
- paired log-ratio `log(t_candidate / t_control)`;
- seeded bootstrap 95% confidence interval for the paired effect;
- order of measurements.

Default gate proposal for scouts:
- byte output and reconstructed SHA must be exact;
- no dropped outliers;
- maximum arm CV <= 15%;
- null/control experiment should span zero effect;
- candidate CI must clear a pre-registered practical epsilon.

For very large effects, the practical epsilon can be 2%. For smaller expected effects, pre-register a lower epsilon and increase repetitions instead of changing the gate after seeing the data.

## 6. Ambient-load gate

Copy the successful Snapdom principle: before consuming timing data, run a short CPU-noise probe.

If host contention is clearly excessive:
- mark the run `BLOCKED_AMBIENT`;
- do not interpret the timing;
- preserve fingerprint/logs;
- rerun later.

A blocked run is neutral, not a failure.

## 7. Corpus policy

Do not commit giant canonical corpora simply to make Actions work.

Prefer:
- tiny deterministic fixtures tracked in git;
- generated synthetic fixtures created from pinned scripts/seeds;
- external canonical corpora downloaded from stable sources and verified by SHA-256;
- Actions cache for download acceleration;
- raw corpus content never treated as a benchmark result.

Large corpus download scripts must fail closed on checksum mismatch.

### AITDCC 2026 should become an external generalization set

The 2026 Algorithmic Information Theory Data Compression Challenge is unusually well aligned with ANVIL's goals: 16 heterogeneous files, a public/hidden-test split from the original competition, Pareto analysis over 117 submissions, an 8-GB memory constraint, and a <=1-MB decompressor requirement. The complete A–P dataset is now public with per-file SHA-256 values and explicit reproducibility terms.

Use AITDCC as a **separate external validation class**, not as another corpus to tune against. Keep its training/testing distinction in reports even though both sets are now public. Fetch canonical files from the official site and verify every SHA-256.

ANVIL should additionally report decoder binary size on this class. That guards against “compress by shipping the corpus/model in the decoder” and makes the reconstruct-don't-store philosophy pay its complete description cost.

### Timing harnesses

The current `tools/bench_native.cpp` groups all repetitions for one codec before moving to the next and emits only medians. It remains suitable for rough scouting but does **not** satisfy the paired/interleaved protocol above.

`tools/paired_bench.py` is the promotion-path driver for two-arm experiments. It:

- warms both arms symmetrically;
- schedules seeded interleaved A/B or B/A pairs inside one job/VM;
- retains every raw repetition and measurement order;
- reports median, MAD-derived robust CV and paired `log(t_candidate/t_control)`;
- computes a seeded bootstrap 95% CI for the paired ratio;
- applies an ambient-jitter block rather than dropping timing outliers;
- can verify output size/SHA after each timed run, outside the timing interval.

The driver was self-checked with a synthetic faster-arm experiment that cleared the speed gate and an A/A null experiment whose CI spanned 1.0. Candidate-specific workflows must still provide the correctness, byte-count, runner-fingerprint and reference-front evidence required elsewhere in this protocol.

## 8. Reference policy

Reference versions are part of the experiment.

Pin:
- Brotli commit or release;
- Zstd release;
- xz/liblzma release;
- libsais tag/commit;
- compiler major/minor when a timing series is meant to be compared.

When the reference compiler or codec version changes, start a new benchmark series. Do not splice absolute throughput numbers across toolchain changes.

## 9. Artifact layout

Every benchmark run should upload one artifact directory:

```
anvil-results/
  manifest.json
  runner.txt
  build.txt
  raw-reps.csv
  bytes.csv
  summary.json
  pareto.csv
  stdout/
  hashes/
```

`manifest.json` is the authority for provenance.

The GitHub job summary should contain only a human-readable synopsis. Raw evidence stays in uploaded artifacts.

## 10. Workflow safety

Initial benchmark workflow should be `workflow_dispatch` only.

It must:
- use `permissions: contents: read`;
- never commit benchmark output;
- never push a baseline;
- never mutate releases;
- use a concurrency group with `cancel-in-progress: false`;
- have a finite timeout;
- upload artifacts under `if: always()`.

Once stable, pull requests may receive a lightweight correctness workflow. CPU-heavy Pareto/microarchitecture workflows remain manual or explicitly scheduled.

## 11. Public-repository publication prerequisite

The current local ANVIL repository must **not** be pushed wholesale.

The live tree currently contains tracked `.opencode/swarms/*.chunkdb*` state and many generated/untracked build/prototype artifacts. Adding them to `.gitignore` does not remove them from existing history.

Before public publication:
1. preserve the current local repository/history untouched;
2. create a sanitized public history/snapshot that excludes `.opencode/`, build directories, binaries, temporary corpora and generated benchmark containers;
3. include source, tests, research documents, build scripts and vendored third-party code only when license/provenance is explicit;
4. perform a secret/path review on the exported tree;
5. create the public GitHub repository from the sanitized tree;
6. enable the manual benchmark workflow there.

This is a publication-boundary operation, not a cleanup that should delete local research artifacts.
