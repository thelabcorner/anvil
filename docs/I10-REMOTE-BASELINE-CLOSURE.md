# ANVIL I10-0 — Remote Baseline Closure

**Closed:** 2026-09-23
**Verdict:** PASS
**Frozen codec tag:** `i10-baseline-codec-20260923`

## Canonical runs

- Silesia: GitHub Actions run `35912470534`, public commit `95ce16a8cc15c4e43e9c77d6b8be029559d27d1d`
- enwik8: GitHub Actions run `35914309555`, public commit `c6914d86bc4fe86658052084fb283f05eec6862e`
- both commits contain the same `src/anvil.cpp` blob: `5082cdf0d1efc5eb170409e8f4181ed0520907bd`

The intervening public changes were documentation/CI tooling only.

## Hard deterministic gate

`tools/close_i10_remote_baseline.py` compared the downloaded raw CSV artifacts against the frozen I9 per-file `anvil-auto-direct` record.

| Corpus | Codec | Linux remote | Frozen I9 | Delta | Ruling |
|---|---|---:|---:|---:|---|
| Silesia | ANVIL auto-direct | 46,446,995 B | 46,446,995 B | 0 | **IDENTICAL** |
| Silesia | Brotli q11/lw30 | 49,383,136 B | 49,383,136 B | 0 | IDENTICAL |
| Silesia | zstd ultra-22 long27 | 52,364,240 B | 52,522,343 B | -158,103 B | new Linux reference |
| Silesia | xz -9e | 48,456,004 B | 48,456,100 B | -96 B | new Linux reference |
| enwik8 | ANVIL auto-direct | 23,534,368 B | 23,534,368 B | 0 | **IDENTICAL** |
| enwik8 | Brotli q11/lw30 | 24,810,180 B | 24,810,180 B | 0 | IDENTICAL |
| enwik8 | zstd ultra-22 long27 | 25,272,471 B | 25,333,695 B | -61,224 B | new Linux reference |
| enwik8 | xz -9e | 24,831,648 B | 24,831,656 B | -8 B | new Linux reference |

Every observed row round-tripped successfully.

`anvil-ratio-auto` selected exactly the same bytes as `anvil-auto-direct` on both canonical corpora in this baseline:

- Silesia: 46,446,995 B
- enwik8: 23,534,368 B

No new ratio-transform win appeared in the baseline itself.

## Reference-series interpretation

The external zstd/xz differences are not ANVIL regressions. They are a new reference series produced by the hosted Linux toolchain.

Canonical-run host fingerprint:

- CPU model: AMD EPYC 7763 64-Core Processor
- Ubuntu hosted runner
- Clang 18.1.3
- CMake 3.31.6
- zstd CLI 1.5.7
- xz 5.4.5
- Brotli development package 1.1.0-2build2
- zstd development package 1.5.5+dfsg2-2build1.1

Silesia and enwik8 happened to land on the same reported CPU model, but they were separate hosted VMs. Their absolute throughput remains scout/context evidence and is not spliced into the historical Windows timing series.

## Artifact-system validation

A separate smoke run, `35916542555`, validated the post-baseline artifact contract:

- correctness/fuzz passed;
- `manifest.json` generated successfully;
- `bytes.csv` generated successfully;
- artifact file hashes were captured;
- runner/toolchain identity was serialized.

That smoke run landed on a different host class (AMD EPYC 9V74), reinforcing why cross-job absolute timings are not promoted.

## I10-0 state transition

I10-0 is **CLOSED**.

The source freeze condition is satisfied:

1. canonical hashes/manifests passed;
2. canonical roundtrips passed;
3. ANVIL deterministic bytes are exactly frozen-I9-identical per file;
4. external reference changes are classified and fingerprinted;
5. the exact baseline codec source is tagged.

Codec-source experimentation may now resume.

The first isolated source experiment is I10-1A:

- `docs/I10-AUX-UNBWT-INTEGRATION-PLAN.md`
- auxiliary-index BWT representation is forced/default-off initially;
- legacy BWT output must remain byte-identical;
- exact auxiliary wire bytes are charged;
- candidate and control are measured in the same remote job with `tools/paired_bench.py`.

I10-1B DEFLATE replay remains a separate later commit/run.
