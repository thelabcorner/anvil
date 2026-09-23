# ANVIL — GitHub Actions Research Benchmark Protocol

**Status:** active protocol for remote experimentation  
**Reason:** ANVIL benchmark/fuzz/anatomy workloads must not consume the developer workstation or homelab unless that policy is explicitly changed.

## 1. Scope

GitHub Actions is ANVIL's default compute vehicle for:

- corpus sweeps;
- expensive ratio experiments;
- fuzz/sanitizer matrices;
- anatomy/oracle scans;
- same-build A/B performance scouts;
- compiler/ISA experiments;
- parameter sweeps.

Local work should be limited to source/document inspection, lightweight static checks, editing, and Git operations.

The initial workflow is **manual-only** (`workflow_dispatch`). Publishing a commit therefore cannot accidentally start a long benchmark.

## 2. What a shared GitHub runner can and cannot prove

A standard public GitHub-hosted runner is excellent for reproducible code execution and deterministic byte measurements. It is **not** a dedicated performance laboratory.

Treat measurement classes separately.

### Deterministic/citation-grade by construction

These are valid when their inputs, code, dependencies, and commands are pinned and the result is reproduced:

- compressed byte count;
- ratio;
- byte-identical roundtrip;
- hash/checksum;
- selected codec/mode;
- parser/token counts;
- model size;
- instruction-stream size;
- wire-format properties.

### Scout/ranking-grade

These are useful for deciding what deserves deeper work but are noisy on shared VMs:

- MB/s;
- ns/byte;
- wall-clock encode/decode time;
- peak RSS;
- CPU-counter-derived estimates when PMU access is available.

For a speed claim, prefer **paired same-job A/B measurements** on the same runner. Never compare a candidate's absolute MB/s from one Actions run to a baseline measured in another run and call the delta a codec improvement.

## 3. Three measurement tiers

### Tier A — correctness

Required for every format/decoder change:

- clean Release build;
- deterministic roundtrip;
- forced decoder registry coverage;
- malformed/truncation rejection where applicable;
- fuzz;
- corpus hashes recorded.

### Tier B — scout

Required before funding an optimization:

- candidate and baseline built in the same job;
- same compiler flags;
- same input bytes;
- same CPU affinity;
- interleaved repetitions when a dedicated A/B harness exists;
- raw per-repetition data retained;
- exact wire bytes recorded separately from timing.

A small timing delta on a shared runner is **inconclusive**, not a win.

### Tier C — frontier evidence

A result can enter ANVIL's Pareto ledger only when:

- deterministic byte result is independently reproducible;
- A/B timing clears a predeclared noise/effect gate;
- reference codecs are measured in the same environment;
- the grid is dense enough that an apparent hole is not just `GRID-THIN`;
- same-transform controls are present where ANVIL gets a reversible transform;
- encode, decode, ratio, memory, and relevant decoder-size costs are all reported.

GitHub-hosted measurements may remain scout-grade for absolute throughput even when their deterministic byte results are fully authoritative.

## 4. Provenance captured on every job

The workflow writes a provenance artifact containing at least:

- repository + commit SHA;
- workflow run/attempt/job identifiers;
- runner image / architecture;
- `uname -a`;
- `lscpu`;
- CPU flags;
- compiler versions;
- CMake/Ninja/Python versions;
- Brotli/Zstd/xz versions;
- configured CMake cache/flags;
- corpus file hashes;
- exact benchmark command;
- benchmark artifacts.

Raw artifacts are never rewritten into the repository by CI.

## 5. CPU affinity

When Linux exposes `taskset`, benchmark processes are pinned to one logical CPU by default.

Why:

- reduces scheduler migration;
- prevents a nominally single-thread codec from wandering among cores;
- makes child processes inherit the same affinity;
- keeps the comparison closer to ANVIL's single-thread research objective.

This does **not** remove noisy-neighbor effects from a shared VM, so the result remains scout-grade unless independently gated.

Multi-thread experiments must be explicitly labeled and must never be compared silently against single-thread rows.

## 6. Corpora

### Development/smoke corpus

The repository's deterministic `tests/corpus` is useful for:

- fast correctness;
- mechanism-specific stress tests;
- regressions;
- synthetic controls.

It is not a standard-corpus performance claim.

### Silesia

CI downloads each canonical file from the public SilesiaCorpus mirror and verifies the exact size and MD5 already pinned by `tools/bench_ratio.py`.

Expected aggregate size: **211,938,580 B**.

### enwik8

CI downloads `enwik8.zip` from Matt Mahoney's benchmark site and verifies:

- size: **100,000,000 B**
- MD5: **a1fa5ffddb56f4953e226637dabbb36a**

The download helper is `tools/gha_fetch_corpus.py`. Corpus bytes live only in the runner workspace.

## 7. Reference-class policy

ANVIL's I9 report correctly labels its existing grid `GRID-THIN`.

The remote platform should progressively expand the reference class instead of optimizing against only one Brotli and one xz point.

Core families to include on the same runner:

- Brotli quality/window tiers;
- zstd regular + high-compression tiers;
- xz/LZMA presets;
- libdeflate/Deflate where the data/use case makes sense;
- LZ4-class speed anchor;
- ANVIL's own historical checkpoints.

For a candidate reversible transform, also provide the strongest practical reference with the equivalent transform when one exists.

Do not call "beats zstd" or "beats Brotli" a frontier result without naming the exact level/window and showing the local reference front.

## 8. Workflow inputs

`.github/workflows/anvil-research-bench.yml` is deliberately manual.

The intended suites are:

- `smoke` — remote build + fuzz + development corpus;
- `silesia` — canonical Silesia ratio/reference run;
- `enwik8` — canonical enwik8 ratio/reference run;
- `all` — smoke plus both standard corpora.

`reps` controls independent scout repetitions for standard-corpus timing. Deterministic byte counts should be identical across repetitions; a mismatch is a hard failure.

## 9. Promotion rules for a new mechanism

Before a new decoder-visible mechanism is integrated, its remote anatomy job should answer:

1. How many raw bytes can this explanation cover?
2. What is the lower bound on instruction/metadata bytes?
3. What residual remains after the explanation?
4. How many candidates must the encoder inspect?
5. What is the expected decoder work per reconstructed byte?
6. Does a same-transform reference erase the apparent advantage?
7. Is the opportunity concentrated in synthetic inputs or present in real corpora?

If an oracle cannot plausibly clear the economic bar, the mechanism stops before production implementation.

## 10. Public-repository safety

The local ANVIL working tree contains research state that must not be published blindly, including local OpenCode swarm databases and generated/build artifacts.

The public repository should therefore be created as a **sanitized snapshot with fresh Git history**, not by pushing the existing local `.git` object graph.

Minimum exclusions:

- `.opencode/`;
- build directories;
- local caches;
- generated executables/objects/PDBs;
- temporary/profiling outputs;
- workstation-specific binary snapshots that are not intentionally redistributable;
- credentials/tokens/secrets.

The local research repository remains the source of private working state. Public CI is the compute vehicle and reproducibility surface.
