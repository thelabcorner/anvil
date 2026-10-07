# I18 — Budget-Gated Fusion: Source-Attested Discovery Closeout

**Date:** 2026-10-07 (America/Chicago). **Classification:** `BUDGETED-SPECIALIZED-DISCOVERY`; **not** a general-purpose Pareto crossing, novel mechanism, independent validation, or production format.

## Provenance, corrections and evidence

- Preregistration: [I18-BUDGET-GATED-FUSION-PREREG-2026-10-07.md](I18-BUDGET-GATED-FUSION-PREREG-2026-10-07.md), fixed before numerical measurements.
- First workflow [37702906749](https://github.com/thelabcorner/anvil/actions/runs/37702906749), source `40cdb8412dc4ff86c6171583a9d26f7c97c96196`: **INVALID-PREMEASUREMENT** (nested frozen I17 source undefines an entrypoint-renaming macro; duplicate C++ main symbol). Do not count against the codec.
- Corrected workflow [37703141058](https://github.com/thelabcorner/anvil/actions/runs/37703141058), source `7ec50e8dadd1b3cc12d815c80007a9fcc2bfd490`: **SUCCESS**. Its sole code correction introduces an attested I17 source mirror with exactly one entrypoint token changed. The source-pinned I17 algorithm and fusion leaf were not retuned.
- Artifact: `anvil-i18-7ec50e8dadd1b3cc12d815c80007a9fcc2bfd490-run1`, API artifact ID `11518358032`; locally fetched outside Git worktrees to `/documents/ANVIL-I18-EVIDENCE-20261007/`. Original artifact is authoritative; neither JSON nor TSV is fabricated from this document.
- I18 source SHA-256 `e5fd1a2c44d697f7cfd49427e1ca3654b192cfff07168e67409007b6f97d65e2`; frozen I16 fusion SHA-256 `b214dcff0355293dd588bd762c7328dfce6e114749669f2b7a32d8c052f51dc3`; pinned Brotli Git commit `ed738e842d2fbdf2d6459e39267a633c4a9b2f5d`. Ubuntu 24.04, GCC 13.3.0, C++20 -O3. Every input is a previously consumed discovery fixture; no heldout source was tested.
- Evidence: `benchmark.tsv`, `decision.json`, `roundtrips.txt`, `selftest.txt`, `provenance.txt`, `binary-hashes.txt`, `binary-sizes.txt`, and six one-shot peak RSS logs. All eight workflow phases passed, including SHA/source attestation, frozen controls, pinned Brotli, build, deterministic selftests, 21 corpus/profile roundtrips, paired digest-observed measurements, decision report and RSS capture.

## Complete-wire results (bytes; lower is better)

| Previously consumed discovery input | Input bytes | I18 AVH2 budget | I17 fast AVH1 | I17 full AVH1 | Brotli q5 native | Brotli q11 native | I18 mode | Math calls |
|---|---:|---:|---:|---:|---:|---:|---|---:|
| synth-arith.bin | 256,000 | **19,445** | 33,204 | 33,204 | 115,107 | 87,013 | AVI6 | 2 |
| synth-timeseries.bin | 280,000 | 120,602 | 120,602 | 120,602 | 120,594 | 134,718 | Brotli q5 | 2 |
| synth-jitter.bin | 974,920 | 86,671 | 86,671 | **62,725** | 86,663 | 62,717 | Brotli q5 | 0 |
| random.bin | 262,144 | 262,152 | 262,157 | 262,157 | 262,149 | 262,149 | RAW | 0 |
| generated.repeat.jsonl | 936,000 | 186 | 186 | **168** | 178 | 160 | Brotli q5 | 0 |
| generated.jsonl | 2,803,267 | 207,708 | 207,708 | **150,424** | 207,699 | 150,415 | Brotli q5 | 0 |
| src.cpp | 32,512 | 8,634 | 8,634 | **7,999** | 8,626 | 7,991 | Brotli q5 | 0 |
| **Total (heterogeneous mixed discovery only)** | **5,544,843** | **705,398** | **719,162** | **637,279** | **801,016** | **705,163** | — | **4** |

Note: Native Brotli comparator wires do not contain the AVH2 eight-or-nine-byte per-file envelope. The I18-vs-q5 and I18-vs-q11 differences must NOT be interpreted as pure backend advantage without accounting for this difference. The AVH1-vs-AVH2 control comparison also includes distinct framing, so the 5-byte random-file delta is not an algorithmic compression advance.

I18 budget saves **13,764 B** vs I17 fast over this selected mix, with **13,759 B** attributable to the arithmetic file and 5 B to random-file framing. It uses **68,119 B more** than I17 full. It is **235 B larger** than the sum of native Brotli q11 files, so even the discovery aggregate is not an unconditional byte win over q11.

## Paired throughput (MB/s; higher is better)

| Input | I18 encode | I17 fast encode | I17 full encode | Brotli q5 encode | I18 decode | Brotli q5 decode | Brotli q11 decode |
|---|---:|---:|---:|---:|---:|---:|---:|
| synth-arith.bin | 4.089 | 11.473 | 0.482 | 21.580 | 381.422 | 161.660 | 141.568 |
| synth-timeseries.bin | 3.647 | 11.246 | 0.353 | 29.291 | 175.439 | 176.611 | 135.343 |
| synth-jitter.bin | 111.414 | 25.928 | 0.624 | 114.900 | 414.223 | 407.071 | 379.697 |
| random.bin | 245.025 | 14.635 | 3.109 | 250.206 | 634.056 | 616.792 | 616.428 |
| generated.repeat.jsonl | 1024.369 | 21.194 | 15.680 | 1022.708 | 419.467 | 426.824 | 426.577 |
| generated.jsonl | 91.566 | 16.523 | 0.543 | 88.367 | 427.072 | 421.573 | 412.818 |
| src.cpp | 50.167 | 12.539 | 0.849 | 49.739 | 279.777 | 286.801 | 253.942 |

Arithmetic: I18 is **8.48x** the I17 full encoder throughput but **0.356x** the I17 fast encoder throughput. I18 decoding is **2.36x** paired q5 and **2.69x** paired q11; these are same-run directional measurements, not cross-hardware or population confidence intervals. On time-series I18 is **10.33x** the full selector encode speed but only **0.324x** the fast selector, while preserving identical AVH1/AVH2 total bytes. The selector expended two numeric candidates only to choose q5, the clearest identified opportunity for bounded work savings.

One-shot arithmetic AVH2 peak RSS (not throughput medians): encode **10,964 KiB**, decode **3,800 KiB**. The multi-control benchmark binary is **1,062,832 B**, **not** a standalone codec footprint. The run lacks matched baseline peak RSS and standalone leaf binary-size measurements; do not infer memory/binary Pareto wins from these values.

## Frozen hypothesis adjudication and limitations

- H1 AVI6 on arithmetic, <33,204 complete bytes: **PASS**, AVI6 19,445.
- H2 q5 on time-series, <=120,602 bytes: **PASS**, q5 120,602.
- H3 at least four of five negative inputs skip expensive math: **PASS**, 5/5.
- H4 faster encode than full on both numeric fixtures: **PASS**; slower than I17 fast on both.
- H5 all complete-file byte regrets retained: **PASS**; major regret on jitter (23,946 B), JSONL (57,284 B), and src.cpp (635 B).
- H6 arithmetic AVI6 decode faster than both q5 and q11: **PASS**.
- H0/H7 source attestation, output correctness and paired-workload checks: **PASS** within this finite suite; 21 exact roundtrips and deterministic selftests do not establish broad fuzz robustness.
- H8 mechanism novelty / Class-A status: **NO CLAIM**. Existing differential/bitpacking and candidate routing retain extensive prior art. Historical global Class-A score remains zero verified complete crossings.

## Interpretation and next branch

The most promising component is the low-decoder-work AVI6 representation on the synthetic arithmetic fixture. The actual missing result is whether these advantages hold on independently sourced, real, typed numerical streams against Sprintz, FastLanes, Gorilla/Chimp, Zstd, matching Brotli, and matched shuffle/bitshuffle baselines. **Freeze source provenance and split by origin before tuning.** Discovery, validation and sealed holdout may never share an upstream origin.

Do not silently modify I18 thresholds or the preregistration. Any follow-up candidate-gating or format work belongs to a new, separately attested iteration. Next useful hypotheses: (1) avoid the two wasted time-series numeric calls with a work-bounded predictor of complete-payload gain and measured false negatives; (2) test real sensor/tabular files and negative controls; (3) report multi-objective nondominated points with uncertainty intervals and matched peak RSS, not a single ratio or sum-of-bytes score.

**Ruling: RETAIN I18 as an intermediate fast/full discovery option; NO general-purpose or novelty promotion.**
