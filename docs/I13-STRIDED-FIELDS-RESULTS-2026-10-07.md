# ANVIL I13 — Strided-Field Discovery: Verified Outcome

**Date:** 2026-10-07  
**Protocol:** [I13 pre-registration](I13-STRIDED-FIELDS-PREREG-2026-10-07.md) locked before first code execution.  
**Action:** [GitHub Actions run 37698492483](https://github.com/thelabcorner/anvil/actions/runs/37698492483), **green across checkout, source identity, pinned Brotli build, C++ compiler, 19 selftest cases, seven real file roundtrips, comparative benchmark, evidence upload**.  
**Artifact:** [11516711106](https://github.com/thelabcorner/anvil/actions/runs/37698492483/artifacts/11516711106), 26,518-byte zip, SHA-256 `b9d0ca08e4d892d32acdd772c194304f9e7dee760cba2d44471df94aff7d4021`.  
**Dispatched commit:** `4c1d74d97206a04d1b6bcbe9b844f9cd2f928f0a`. Source SHA-256 `274be4c8a5de222b4bbb16c586fc04310dd1deadd455dacc7741110b8dfcb7c0`, Git blob `ca3f9e684917b276f6a4274de62c3b20ca73fa95`.  
**Matched control:** pinned google/brotli v1.1.0 `ed738e842d2fbdf2d6459e39267a633c4a9b2f5d`, q5/q11, window22, within same GitHub-hosted runner.  
**Evidence class:** discovery only, previously consumed synthetic. No heldout, no full-objective Pareto crossing, no mechanism novelty. CPU-bound build and bench solely on GitHub Actions, no agent delegates.

## Complete-wire measurements and timed reconstruction

Decimal MB/s; both decoders timed with their actual decoded bytes consumed by the same digest. Time measurements are single shared-run scouting observations without confidence intervals. Avoid cross-run speed comparisons.

| Discovery fixture | Raw B | AVI3 B | Brotli q11 B | 4KiB column blocks | Modeled columns | AVI3 decode MB/s | Brotli q11 decode MB/s |
|---|---:|---:|---:|---:|---:|---:|---:|
| synth-arith.bin | 256,000 | **62,256** | 87,013 | 5 | 25 | **540.728** | 223.739 |
| synth-timeseries.bin | 280,000 | 139,387 | **134,718** | **69** | **407** | **598.592** | 189.402 |
| synth-jitter.bin | 974,920 | 975,641 | **62,717** | 1 | 1 | 846.880 | 531.464 |
| random.bin | 262,144 | 262,343 | **262,149** | 0 | 0 | 844.370 | 841.656 |
| generated.repeat.jsonl | 936,000 | 916,835 | **160** | 229 | 916 | 511.432 | **633.218** |
| generated.jsonl | 2,803,267 | 2,797,603 | **150,415** | 623 | 1,235 | 671.073 | 576.689 |
| src.cpp | 32,512 | 32,510 | **7,991** | 3 | 3 | 807.150 | 391.084 |

## Prespecified hypothesis verdict

- **H1 selected real strided structure: PASS.** On the 280 KB time series, all 69 blocks use COL encoding; 407 columns have mathematical reconstruction, and 100% of the fixture roundtrips.
- **H2 complete wire under Brotli q11: FAIL.** AVI3 **139,387 B**, reference **134,718 B**; **4,669 B** excess = **3.466% of Brotli q11 compressed bytes** (or **3.35% of AVI3**).
- **H3 model-bearing decode speed above paired reference: PASS.** AVI3 **598.592 MB/s**, Brotli q11 **189.402 MB/s** (~**3.16×**, same-run only).
- **H4 I12 arithmetic non-regression: PASS.** AVI3 is **62,256 B**, down **202 B** from the frozen I12 **62,458 B**. The ABI is different and the decode speed is not directly comparable across distinct runner executions.
- **H5 exactness/safety: PASS.** Compiled C++ and pinned controls, selftests including interleaved/noise/tail and malicious/truncated wire, seven source roundtrips and artifact upload all green.

**Binding decision:** `STRUCTURE-BUT-NO-BYTE-WIN`. The tested field-local reconstruction is real and fast, but does not yet beat Brotli q11 on this newly targeted synthetic source; **zero ANVIL Class-A Pareto crossings remains unchanged**. Its remaining size deficit is small enough to warrant *controlled cost attribution*, not an unpreregistered retune.

## Cost-accounting implications and next gate

The 69 selected record blocks cost an average **67.67 additional compressed bytes per 4 KiB block** relative to the Brotli q11 *whole-file* stream. Do not interpret that number as a literal per-block Brotli cost because the reference has a cross-block window and context. It is merely a budget decomposition target.

AVI3 emits 407 predictive columns, each with approximately **10 descriptor bytes** (`tag + base + step + width`) before the actual bitpacked residuals; approximately **4,070 B** of modeled-field descriptors are explicitly charged, of the same order as the entire 4,669 B reference gap. This does **not** mean those descriptors can be deleted or that fewer bytes are guaranteed: a decoder must recover the necessary base, step and packing information. Other charges: 69 stride/frame headers, per-column raw tags, residual padding and up to 27 literal tail bytes per 4 KiB block for a 7-word record; all already included in the exact wire.

The correct next step is to instrument a byte-conservation ledger per column, separate descriptor/raw/residual/tail expenses, and compare against reference block/window controls. Only then consider an explicitly preregistered design with amortized predictive headers across blocks, bounded fixed-size patch exceptions (PForDelta prior art), whole-stream block alignment, or a fast LZ backend. Rank every proposal by *complete-wire bytes AND decode work*, with encoder cost/RSS/binary size as additional axes. Never claim new mathematics for existing bitshuffle/FOR/patch techniques.

**Other families remain negative.** Repetitive JSONL is an especially strong rejection: AVI3's striding improves RAW but is nowhere near LZ's 160 bytes. The global ANVIL goal therefore requires a competitive general-purpose entropy/LZ core alongside any typed reconstruction improvements.

**Promotion gates still open:** independently collected, legally admissible real numeric streams; matched modern typed codecs (FastLanes/ALP/Gorilla/Sprintz etc.), full memory/RSS and binary footprint, equal compression window/block geometry, held-out Gate B and repeated runner calibration. No existing R2 reclassification.
