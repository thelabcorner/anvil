# ANVIL I12 — Frozen bounded innovations discovery outcome

**Date:** 2026-10-07
**Experiment:** manual [GitHub Actions run 37697658306](https://github.com/thelabcorner/anvil/actions/runs/37697658306), **all steps PASS**.
**Action artifact:** [Run evidence / artifact 11515254890](https://github.com/thelabcorner/anvil/actions/runs/37697658306/artifacts/11515254890).
**Source identity:** SHA-256 `3e6c24c90eb4026cca2520e88413caece25f847771f4e0c6513c48964a05992c`; Git blob `2e6ea6492f970ee56366b5530872da193a815226`. Workflow repository revision `1fae7bebe7e86ce9e5548d2d8c1257f4f2dd6924`.
**Control:** pinned google/brotli v1.1.0 `ed738e842d2fbdf2d6459e39267a633c4a9b2f5d`, q5/q11 lgwin22; both produced and measured on same Actions worker.
**Evidence class:** already-consumed synthetic/discovery, **NOT** held-out, Class-A promotion, general-purpose crossing, or mechanism novelty. No local compilation or CPU-intensive benchmarks.

## Frozen deterministic wire-size and paired decode observations

Decimal MB/s; timed decoders include the same every-byte digest. Shared GitHub-hosted runner timing is **scouting** evidence, not dedicated-machine causal confidence.

| File | Raw B | AVI2 B | Brotli q11 B | AVI2 model blocks | AVI2 decode MB/s | Brotli q11 decode MB/s |
|---|---:|---:|---:|---:|---:|---:|
| synth-arith.bin | 256,000 | **62,458** | 87,013 | **63/63** | **358.872** | 141.060 |
| synth-timeseries.bin | 280,000 | 280,214 | **134,718** | 0/69 | 626.174 | 135.796 |
| synth-jitter.bin | 974,920 | 975,643 | **62,717** | 0/239 | 623.742 | 376.669 |
| random.bin | 262,144 | 262,343 | **262,149** | 0/64 | 624.216 | 599.971 |
| generated.repeat.jsonl | 936,000 | 936,694 | **160** | 0/229 | 621.171 | 423.140 |
| generated.jsonl | 2,803,267 | 2,805,330 | **150,415** | 0/685 | 621.137 | 408.671 |
| src.cpp | 32,512 | 32,543 | **7,991** | 0/8 | 636.928 | 250.181 |

**Preregistered verdict:** `SPECIALIZED-DISCOVERY-CANDIDATE` (H1, H2, H3 true *only on synth-arith*). Complete compressed-size saving **24,555 B** = **28.22% fewer bytes relative to Brotli q11** for 256KB arithmetic file; paired digest-inclusive decode rate **2.54× Brotli q11**, arithmetic encode **28.820 MB/s**. Nontrivial codec selection is decisive: **63 actual modeled blocks**, not a RAW-copy illusion. In-source dense-jitter selftest selected 2 modeled blocks and passed bit-exact roundtrip. Seven tracked sources all passed full encode/decode/cmp. Binary source identity, pinned reference build and evidence artifact were verified; the remote workflow completed green.

**Other-family negative controls are binding**: the sparse record/JSONL/general-text/realistic time-series cohorts did not select any model blocks; do not claim any benefit from their RAW-dominated speed. AVI2 is not competitive against Brotli on them. Also no encode/memory/binary-size parity study, no equal geometry block/window, no independent real numeric held-out families, no matched typed-codec comparisons or qualifying statistical throughput confidence intervals. **Zero verified general-purpose ANVIL frontier crossings remains unchanged** (435 dominated, 28 degenerate, 5 unresolved).

## Causal interpretation

I11's sparse exact-affine + replacement mechanism failed; I12's **dense bounded innovation** representation reconstructs the same *already-known arithmetic* source compactly using modular median slope and bitpacked residuals. This establishes that sufficiently regular small signed errors are better charged as contiguous residual bits than as either exact-affine-or-overlong sparse exceptions. The mechanism is familiar FOR/delta/ZigZag/bitpacking engineering prior art, not novel.

The current representation is **one global stride/width per block**. It cannot separate interleaved small-entropy fields from high-entropy fields at independent positions. I13 should test whether a byte-exact, bounded **strided field-frame mechanism** isolates such fields and beats raw mixed-block fallback, using the same seven discovery inputs and pinned Brotli controls, before collecting any independent heldout corpus. It must charge all stride metadata, raw-column payload, tail handling, and full decoder scatter work.

## Post-I12 decision

- **RETAIN** the modeled arithmetic path as a specialized engineering control. Preserve AVI2 source/benchmark provenance.
- **DO NOT** integrate AVI2 into production or reclassify historical Class A.
- **PROTOTYPE** a separately registered strided-field transformation for mixed record sequences rather than modifying the frozen I12 workflow.
- **VALIDATE** against real independently sourced numeric telemetry only after corpus Gate B and matched modern typed competitors (FastLanes, Gorilla/Chimp, ALP, Sprintz, Zstd) are established.
