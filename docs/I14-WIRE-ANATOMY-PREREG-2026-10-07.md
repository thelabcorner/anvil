# I14 — Exact AVI3 Wire-Anatomy Preregistration

**Date:** 2026-10-07, before analysis execution. **Class:** deterministic byte-cost infrastructure only; no speed, novelty, compression, held-out or Class-A frontier claim. **Runner:** manually triggered GitHub Actions, no local heavy work, no delegates.
**Baseline:** fully green I13 [run 37698492483](https://github.com/thelabcorner/anvil/actions/runs/37698492483), frozen AVI3 source SHA-256 274be4c8a5de222b4bbb16c586fc04310dd1deadd455dacc7741110b8dfcb7c0, blob ca3f9e684917b276f6a4274de62c3b20ca73fa95.

## Objective
Explain the 4,669 B complete-wire deficit of AVI3 versus Brotli q11 on previously consumed synthetic time-series, by exact conservation of AVI3 bytes and a bounded, codec-independent measurement of fixed-group width *capacity*. No alternative bitstream is produced. Preserve the fixed q11 134,718-byte reference from I13 as a deterministic historic comparison; do not splice timings.

## Immutable observed inputs
Source-pinned I13 code compiled only on GitHub Ubuntu runner; encode and compare roundtrip for exactly three tracked existing discovery fixtures:
- tests/corpus/synth-timeseries.bin: expected AVI3 bytes **139387**
- tests/corpus/synth-arith.bin: expected AVI3 bytes **62256**
- tests/corpus/generated.repeat.jsonl: expected AVI3 bytes **916835**

Workflow must assert both instrument and I13 source SHA+Git blob, HEAD and clean checkout, and all input Git blobs; source variations fail closed. Analyzer byte-parses actual AVI3 wire (not the C++ mode counters or a reconstructed proxy), with size and canonical varints, no overrun, legitimate tags, widths, zero padding, output cap <=256 MiB, and total output count matching the original input. Decode and compare using exact I13 C++ pilot. Artifact includes per-family JSON and full plaintext summary.

## Exact byte-cost conservation
Partition disjointly:
- outer magic + length varint
- RAW block metadata and data
- whole-block model tag/word width/count/base/step/bit width and payload
- COL group tag/size varint/stride
- each COL raw tag + raw values
- each COL modeled tag/base/step/width + packed residuals
- COL literal record-tail bytes
No bytes unassigned; serialized sum must equal true file length for all three files. On mismatch, **INVALID-ACCOUNTING**, no mechanism recommendation.

## Capacity-only microtile scan
On each already-modeled column, read ZigZag packed unsigned residuals and compute exact cost for independent 16, 32, 64, 128-value chunks, each with 1-byte bit width and ceil(count*tile_maxbit/8) bit payload, including tail chunks and padding. Report signed difference versus frozen per-column flat bitpack including no new source header. This is NOT a valid encoded mode until implemented, so do not call capacity "saved bytes."

Report:
- counts by group stride, modeled vs RAW lane and per-column bits
- total modeled descriptors, raw bytes, residual packed bytes, tail bytes and framing
- modeled-lane theoretical maximal 9-byte parameter/width removal (upper bound only)
- each microtile capacity after group width metadata
- per-group and per-column minimum/proportion, including adverse cases.
If width microtiling does not cover a material portion of 4,669 B, downgrade width-only codec integration; prioritize record alignment, cross-block reuse, raw-column structure, or strong LZ backend.

No gate is tuned post-results. Other possible alternatives require their own preregistered full-wire implementation, actual lossless decoder, and matched remote benchmark.

**R2 role firewall:** Gate A was green XRUN 37694740386; Gate B c9/independent new families still closed. All conclusions restricted to already consumed discovery data. No alterations to production codec, no third-party code copied into ANVIL.
