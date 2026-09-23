# ANVIL I10 — Remote Baseline Series

**Series:** `linux-gha-i10-2026-09-23`
**Purpose:** establish the first post-I9 Linux/GitHub-hosted correctness + canonical-corpus baseline without consuming workstation or homelab compute.
**Timing class:** GitHub-hosted shared-runner scout/ranking evidence unless promoted by the normative paired protocol in `docs/github-actions-benchmark-protocol.md`.

## 1. Source line

The canonical benchmark source for the initial Silesia/enwik8 pair is the public commit:

- `95ce16a8cc15c4e43e9c77d6b8be029559d27d1d`
- public repository: `thelabcorner/anvil`

That commit contains:

- the sanitized public research snapshot;
- the manual-only GitHub Actions benchmark vehicle;
- the Linux workflow-context fix;
- the portable Clang/GCC CPUID include/dispatch fix.

Later public commits in the same repository are documentation-only and do **not** alter the source being measured by the already-dispatched Silesia/enwik8 jobs.

Do not splice absolute throughput from this Linux series into the old Windows I9 timing series. Deterministic byte results may be compared when the wire/input/configuration is demonstrated equivalent; throughput starts a new hardware/toolchain series.

---

## 2. Infrastructure bring-up

### Run 35911682778 — smoke bring-up

URL:
`https://github.com/thelabcorner/anvil/actions/runs/35911682778`

Result: **FAIL — infrastructure/compiler portability, no codec verdict.**

Failure:

- Ubuntu Clang 18 treated the x86 Clang branch as using MSVC's `<intrin.h>`;
- Linux Clang requires the GCC-compatible `<cpuid.h>` path.

Correction:

- local commit `770eeb4`;
- public commit `95ce16a`;
- `_MSC_VER` now selects `<intrin.h>`;
- GCC/Clang select `<cpuid.h>`.

No compression conclusion is permitted from this run.

### Run 35911975800 — smoke/correctness

URL:
`https://github.com/thelabcorner/anvil/actions/runs/35911975800`

Result: **PASS.**

Observed runner/toolchain:

- x86_64;
- 4 logical CPUs exposed;
- AMD EPYC 9V74 80-Core Processor host class;
- Ubuntu Clang 18.1.3;
- Brotli development package 1.1.0-2build2;
- Zstd development package 1.5.5+dfsg2-2build1.1.

Correctness gate:

- `PASS seed=41246`
- `roundtrip_variants=280`
- `mutations=1680`
- `deterministic_rev2=9`
- `deterministic_bwt=24`
- `golden_bwt=4`
- `forced_postcoders=20`
- `registry_block_modes=14`
- `registry_transforms=5`

The smoke benchmark also produced scout throughput/ratio rows, but those rows are **not promotion-grade timing** because the current `bench_native` harness groups codecs instead of performing the normative interleaved paired A/B protocol.

This run establishes that the public Linux source builds and the remote correctness/fuzz vehicle is operational.

---

## 3. Canonical corpus runs

### Run 35912470534 — Silesia

URL:
`https://github.com/thelabcorner/anvil/actions/runs/35912470534`

Dispatched source:

- `95ce16a8cc15c4e43e9c77d6b8be029559d27d1d`

Requested suite:

- `silesia`
- independent reps: `1`
- canonical manifest verification enabled;
- CPU affinity enabled;
- codecs:
  - `anvil-auto-direct`
  - `anvil-ratio-auto`
  - `brotli-q11-lw30`
  - `zstd-ultra-22-long27`
  - `xz-9e`

Status at ledger creation: **IN PROGRESS**.

Promotion note:

- compressed byte totals + roundtrip are deterministic evidence if the run completes cleanly;
- one-repetition throughput is context/scout evidence only;
- no new Pareto timing claim follows from this run alone.

### enwik8 queue correction

The original workflow used one global concurrency key. GitHub Actions retains at most one pending run for a concurrency group, so later pending dispatches can replace an older pending run even when `cancel-in-progress: false`.

Three runs were therefore retired **before producing benchmark evidence**:

- `35913398693` — original enwik8 dispatch; cancelled while pending when a later run occupied the single pending slot;
- `35913973522` — manifest-smoke validation; cancelled/replaced while pending;
- `35914200895` — enwik8 requeue under the old global key; manually cancelled after the concurrency design was corrected.

No codec result is attributed to any of those runs.

The workflow now keys concurrency by suite:

`anvil-research-bench-<suite>`

so independent Silesia, enwik8, and smoke jobs can occupy independent hosted VMs while same-suite overlap remains controlled.

### Run 35914309555 — enwik8

URL:
`https://github.com/thelabcorner/anvil/actions/runs/35914309555`

Dispatched source:

- public commit `c6914d86bc4fe86658052084fb283f05eec6862e`;
- `src/anvil.cpp` blob `5082cdf0d1efc5eb170409e8f4181ed0520907bd`.

The Silesia baseline commit `95ce16a8cc15c4e43e9c77d6b8be029559d27d1d` contains the **same** `src/anvil.cpp` blob `5082cdf0d1efc5eb170409e8f4181ed0520907bd`. Intervening public commits are documentation/CI-tooling changes, not codec-source changes.

Requested suite:

- `enwik8`
- independent reps: `1`
- canonical manifest verification enabled;
- CPU affinity enabled;
- same codec reference set as the Silesia run;
- machine-readable `manifest.json` / `bytes.csv` artifact generation enabled.

Current status: **IN PROGRESS**, canonical enwik8 benchmark step active.

This run and Silesia are now executing concurrently on independent GitHub-hosted VMs.

---

## 4. Series interpretation rules

1. **Correctness failures beat performance.** Any roundtrip/hash failure invalidates the associated byte row.
2. **Bytes and time are separate evidence classes.** Deterministic compressed bytes can be authoritative while hosted-runner timing remains scout-grade.
3. **No cross-series MB/s comparison.** The Windows I9 host and this Linux EPYC-hosted series are different timing series.
4. **No single-point "beats codec X" claim.** Reference-class density remains a requirement for a frontier statement.
5. **Same-transform controls remain binding.** A reversible transform must be compared to a practical reference with the equivalent transform when applicable.
6. **Raw artifacts remain authoritative.** Human prose is a synopsis, not the source of record.

---

## 5. Next state transition

When both canonical jobs finish:

1. download/inspect the Actions artifacts;
2. verify every canonical corpus hash and roundtrip result;
3. compare deterministic ANVIL bytes against the frozen I9 byte record;
4. classify any difference as:
   - expected toolchain-independent identity,
   - implementation/source change,
   - or unexplained divergence requiring a stop;
5. record the Linux runner/toolchain fingerprint as a new benchmark series;
6. only then begin I10-1 integration experiments:
   - `libsais_unbwt_aux`;
   - P4.1 DEFLATE reconstruction.

The two I10-1 changes must remain separate experiments and must not be combined before their individual deltas are measured.
