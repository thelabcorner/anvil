# ANVIL I13 — Frozen Strided Column Discovery Closeout (2026-10-07)

**Authoritative source:** [GitHub Actions 37698492483](https://github.com/thelabcorner/anvil/actions/runs/37698492483), run head `4c1d74d97206a04d1b6bcbe9b844f9cd2f928f0a`, all steps successful. Source Git blob `ca3f9e684917b276f6a4274de62c3b20ca73fa95`, SHA-256 `274be4c8a5de222b4bbb16c586fc04310dd1deadd455dacc7741110b8dfcb7c0`, pinned Brotli `ed738e842d2fbdf2d6459e39267a633c4a9b2f5d`. Evidence read from action artifact; discovery cohort is the seven pre-consumed tracked files, **not held out**.

## Binding preregistered decision: STRUCTURE-BUT-NO-BYTE-WIN

- H1 **PASS**: on 280,000-B `synth-timeseries.bin`, all **69/69** output blocks use COL; **407** columns are individually modeled.
- H2 **FAIL**: I13 **139,387 B** full wire vs Brotli q11 **134,718 B**: **4,669 B (3.47%) larger**. It is also larger than Brotli q5 **120,594 B**.
- H3 **PASS as scout-grade**: same-run byte-digest-inclusive I13 decode **598.592 MB/s** versus pinned Brotli q11 **189.402 MB/s**, ~**3.16x**; shared VM timing is not hardware-normalized.
- H4 **PASS**: arithmetic **62,256 B**, improving the earlier I12 **62,458 B** by 202 B.
- H5 **PASS**: selftest **19 cases**, all seven full roundtrips passed, source/runner/binary evidence retained.

| File | I13 bytes | Brotli q11 bytes | COL blocks | Modeled columns | I13 decode+digest MB/s | Brotli decode+digest MB/s |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| synth-arith.bin | 62,256 | 87,013 | 5 | 25 | 540.728 | 223.739 |
| synth-timeseries.bin | 139,387 | 134,718 | 69 | 407 | 598.592 | 189.402 |
| synth-jitter.bin | 975,641 | 62,717 | 1 | 1 | 846.880 | 531.464 |
| random.bin | 262,343 | 262,149 | 0 | 0 | 844.370 | 841.656 |
| generated.repeat.jsonl | 916,835 | 160 | 229 | 916 | 511.432 | 633.218 |
| generated.jsonl | 2,797,603 | 150,415 | 623 | 1,235 | 671.073 | 576.689 |
| src.cpp | 32,510 | 7,991 | 3 | 3 | 807.150 | 391.084 |

**Critical counterexample:** The column selector found inexpensive modeled fields in JSONL and even C++ text, but still retained almost all original bytes. It is not a robust general-purpose codec: high-level LZ redundancy remains almost completely unexploited. A strong outer LZ fallback is a requirement, not an optional enhancement.

## Causal diagnosis and next experiments

I13 correctly discovered an interleaved-field structure, yet its **global line residuals**, opaque columns, small 4 KiB blocks, row stride and metadata accumulated a 4,669-B deficit against Brotli q11. The result cannot isolate which mechanism causes the deficit. Distinguish with a frozen factorial experiment: (A) I13 raw/model columns, (B) I13 plus local-step innovations, (C) I13 plus bitmap-patched tail values, (D) explicit raw columns passed to a capable entropy/LZ carrier; every arm carries exact complete bytes, the same input field boundaries and decode work. Pre-register matching block/window controls before any promotion.

I14 tests the orthogonal **local innovations** hypothesis on the I12 flat blocks first, without hiding this I13 failure. Do not expand the I13 stride search silently or reuse the consumed discovery cohort as held out. Protect legal provenance and source-independent real numeric families before externally claiming generalized efficacy.

**General ANVIL Class-A status unchanged:** 435 dominated, 28 degenerate, 5 unresolved, **zero independently verified full Pareto crossings**.
