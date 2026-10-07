# ANVIL I16 — Full-File Hybrid Envelope: Source-Attested Discovery Result

**Date:** October 7, 2026. **Adjudication:** HYBRID-ARCHITECTURE-VALIDATED-DISCOVERY. **General-purpose Pareto crossing:** NO. **Novel mechanism:** NO. **Production adoption authorized:** NO.

**Authoritative job:** [GitHub Actions run 37700550185](https://github.com/thelabcorner/anvil/actions/runs/37700550185) — every workflow stage succeeded. **Artifact:** [11516559551](https://github.com/thelabcorner/anvil/actions/runs/37700550185/artifacts/11516559551), zip SHA-256 fdbe506d1d6b234ece73439576394926075ecf77435f81f3c8ff7120cb73a006.
**Checked-out commit:** 516b2942855a2a02708ff32f148a4e8342e73b10.
**I16 exact code:** Git blob 617147803d519e12cf3a6c74878c7125aef6b96c; SHA-256 0162ccfab90607393698e12d410509e3d09e31cf77336139e42a830a4837a418.
**I14 original numeric leaf:** Git blob 365348ac5b8295f32eed97fb9427bb5516c43b16; SHA-256 f4f75b098feee9753084fa5cc132e31064f107fbd8ffa621f5e415b57d825150.
**Brotli:** pinned commit ed738e842d2fbdf2d6459e39267a633c4a9b2f5d. Ubuntu 24.04, GCC 13.3.0, CMake 3.31.6; full seven-file byte exact roundtrip and malformed-wire selftest pass. 15 selftest cases; AVI4 selected in three synthetic selftest cases, Brotli in 11; the empty case uses AVI4 but does not count as a nonempty modeled case.

## 1. Complete paired metrics (do not remove the negative controls)

All rates are GitHub-runner discovery scouts, not Class-A robust estimates. Decode times consume reconstructed bytes through the common digest.

| Fixture | Raw B | AVH1 full B | Choice | Brotli q5 B | Brotli q11 B | I16 encode MB/s | I16 decode+digest MB/s | Brotli q11 decode+digest MB/s |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| synth-arith.bin | 256,000 | **33,204** | AVI4 DIFF (63/63) | 115,107 | 87,013 | 0.484 | **366.564** | 140.804 |
| synth-timeseries.bin | 280,000 | **120,602** | Brotli q5 | 120,594 | 134,718 | 0.354 | 175.443 | 132.022 |
| synth-jitter.bin | 974,920 | 62,725 | Brotli q11 | 86,663 | 62,717 | 0.612 | 374.670 | 376.178 |
| random.bin | 262,144 | 262,157 | Brotli q5 (tie) | 262,149 | 262,149 | 3.065 | 584.777 | 604.140 |
| generated.repeat.jsonl | 936,000 | **168** | Brotli q11 | 178 | 160 | 15.476 | 420.904 | 422.509 |
| generated.jsonl | 2,803,267 | 150,424 | Brotli q11 | 207,699 | 150,415 | 0.528 | 408.509 | 409.822 |
| src.cpp | 32,512 | 7,999 | Brotli q11 | 8,626 | 7,991 | 0.840 | 251.481 | 252.342 |

**Byte-oracle identity was exact on all seven fixtures.** The overhead is eight bytes on inputs whose total length needs a three-byte varint (all but generated.jsonl, which needs four bytes and nine bytes overhead). On the numeric source the wrapper adds eight bytes to I14's original 33,196 B. Arithmetic is 61.84% smaller than the q11 reference and its same-run decode+digest rate is 2.60x q11. Those are **synthetic specialization** results, not independent real-world generalization.

Importantly, the 936 KB repeated-JSONL failure is contained: AVI4 alone stored 936,694 B, while AVH1 picks the 160-B q11 stream and adds exactly eight bytes. Likewise, on time-series, q5's 120,594 B is superior to q11's 134,718 B; AVH1 correctly selects q5 at 120,602 B. These are *borrowed* reference-compressor performance, not new entropy representation.

## 2. Frozen gate adjudication

- H1: PASS — arithmetic selects AVI4, 63 modeled local-diff blocks, below q11 bytes.
- H2: PASS — repeated JSONL selects Brotli q11 and pays eight bytes.
- H3: PASS — time-series selects the smaller Brotli q5 option.
- H4: PASS — exact minimum-size selector oracle equals full wire for all seven.
- H5: PASS **scout** — arithmetic AVI4 envelope decode+digest exceeds same-run q11 speed, 366.564 vs 140.804 MB/s.
- H6: PASS — independently compiled source-pinned run, malformed-wire selftest, seven full encoded-file roundtrips, byte-consuming decoded output, binary hash, retrievable artifact.
- All PASS means the hybrid architecture is valid as an engineering baseline. It is neither a novel compression method nor a certified multi-axis Pareto crossing.

## 3. The real performance frontier—why the whole-file selector is not sufficient

The **encoder eagerly computes AVI4, Brotli q5, and Brotli q11 on every file** to know which compressed payload is shortest. Thus arithmetic *decode* is ~2.60x q11, but *encode* is only 0.484 MB/s versus q11 alone 0.503 MB/s, despite AVI4's standalone 24.136 MB/s in an earlier separate run. On time-series encode is 0.354 MB/s vs q5 standalone 29.886 MB/s, an ~84x penalty relative to the chosen fast reference because the encoder still spends time on q11. No mode-selection oracle can avoid this cost without either prediction or less expensive candidate evaluation.

The unstripped statically Brotli-linked I16 binary is 989,192 B on this runner. No representative RSS, cold-start cost, full streaming/random access, independent legal numeric holdout, NUMA/core/clock-normalized throughput confidence intervals or typed-codec competitors were measured.

**Measurement note:** the encode timing consumes selected archive *length* and executes the codec functions; unlike decoded payload timing, it does not digest the selected compressed bytes. A future instrumentation revision should consume the complete encoded buffer to rule out compiler-elision artifacts. This does not affect deterministic wire-size, roundtrip or mode-selection findings.

## 4. Next branching engineering decisions

1. **I17 fast portfolio search, separately preregistered:** compare full minimum-size profile against a q5-only + AVI4 fast profile and an approximate q11-trigger profile. Explicitly report byte regret on every fixture, encode work, decode, RSS and separate reference throughput; do not pretend approximate selection preserves the exact-size oracle. Derive the trigger exclusively from cheap encoder-side features on discovery and freeze it before real heldout validation.
2. **I17-B mathematical residual coding, separately gated:** on I14, 32,370 of 33,196 arithmetic wire bytes are packed innovation payload. Analyze signed innovation frequency, max-width outliers and per-block exact entropy bounds on GH Actions, then test bounded-radix or patched low-width coding if a *complete-byte theoretical upper bound* justifies it. Small-alphabet digit packing (e.g. five ternary residuals in one byte) is strong known prior art; novelty requires a qualitatively distinct mechanism plus benchmark, not using base three.
3. **I18 heterogeneous segments:** perform actual mode switching inside one mixed file, paying per-segment headers and reset costs. Compare against a whole-file Brotli dictionary that can share repetition globally; per-block resets can destroy the gains we are trying to preserve.
4. **I19 real-sensor typed controls:** procure provenance-locked independent numeric families; compare Sprintz/FastLanes/Gorilla/Chimp/ALP and q5/q11/Zstd/LZ4 under schema-equivalent byte semantics, including peak RSS and binary.
5. **Return to Class-A audit:** freeze source, data provenance, window geometry and robust repeated timings. Historical R2 remains 435 dominated, 28 degenerate, 5 unresolved, zero confirmed general-purpose Pareto crossings.

No delegated workers/subagents were spawned by this experiment. All CPU-intensive compilation/selftests/benchmarking ran exclusively on GitHub Actions. The intentionally dirty canonical ANVIL checkout was not modified. AVH1 remains research-only and must not be merged into production as a general-purpose breakthrough.
