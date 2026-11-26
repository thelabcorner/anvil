# A70 — Pinned libsais 2.12.2 serial inverse-BWT scout (preregistration, NOT executed)

**Prepared:** 2026-11-18. **Research disposition:** VERIFY the strongest available *conventional* decoder baseline before constructing an unimplemented run-aware/blocked LF algorithm. **Execution disposition:** TEST, contingent on an allowed, manual-only GitHub Actions publication/dispatch path. Nothing in this preregistration authorizes bypassing an existing workflow-push restriction.

## Goal, decisive unknown, and prior evidence

ANVIL's full objective remains exact lossless decoding plus complete-wire size, encode/decode time, peak RSS, decoder and dependencies size, stream/safety controls, and held-out generalization. A narrowly better inverse-BWT algorithm is a useful *mechanism* observation, not automatically all-axis Pareto dominance or a full Class-A crossing.

- Frozen A44 cap-8 Silesia run `44370017321`, candidate `2c5eeb9e570b1e1162cbff98d29e0a978edeca92`, reported **47,365,274 compressed bytes**, **74.2 MiB peak RSS (1.450× xz)** and **ANVIL/Brotli decode-time ratio 2.3618** (paired 95% CI **[2.2788, 2.4446]**). The **Brotli decode-time guardrail <= 2.0** remains the binding Silesia cost axis. The cap experiment used default *byte-minimum* routing, **not active C7 percent-gain routing**; see research ledger A69 and the original raw job logs. Do not attribute the result to dormant C7 policy.
- A48 run `44380412243` hit its 240-minute limit; original [artifact `14480494484`](https://github.com/thelabcorner/anvil/actions/runs/44380412243/artifacts/14480494484) preserves four complete per-file benchmarks but **no 12-file R-2 ruling**. It has 5 scoped thin-grid crossing labels across eight evaluated file-plane cells, not the full Class-A frontier nor all-axis Pareto. No point in rerunning that unchanged multi-hour grid.
- The current vendored `third_party/libsais/{src/libsais.c,include/libsais.h}` identifies **libsais 2.10.4** (September 1, 2025). The upstream [official v2.12.2 release](https://github.com/IlyaGrebnov/libsais/releases/tag/v2.12.2), published **November 9, 2026**, reports improved inverse BWT in single- and multi-threaded operation. [Upstream CHANGES](https://github.com/IlyaGrebnov/libsais/blob/v2.12.2/CHANGES) reports **significantly** improved inverse BWT in **v2.12.0 (October 18, 2026)**, followed by additional optimization in **v2.12.2**. These are *upstream claims*, **not measured ANVIL improvements**. Version 2.12.0 also changed licensing to **Apache-2.0 WITH LLVM-exception** and tightened invalid-BWT safety behavior.
- The existing `prototypes/i9-bwtinv/unbwt_bench.cpp` is Windows/QPC-only and its `aux_r*`/OMP arms call existing libsais APIs. It does not implement an independent blocked LF traversal. Upstream's README already documents in-place 5n inverse BWT and bigram-LF optimization; do not mistake those features for a new ANVIL invention.
- Existing CMake builds libsais serial by default (`-O3 -DNDEBUG`, no `LIBSAIS_OPENMP`). Upstream's [CMake](https://github.com/IlyaGrebnov/libsais/blob/v2.12.2/CMakeLists.txt) similarly defaults `LIBSAIS_USE_OPENMP=OFF`, making a clean **serial version-only comparison** feasible without introducing multithread confounds.

## Amdahl bound: a screen before investment

To move 2.3618× Brotli decode time below 2.0× on the same fixed control, end-to-end ANVIL decode wall time must improve by **at least 1 − 2/2.3618 = 15.32%**. The inverse-BWT LF stage can only cause that change to the extent it actually occupies the original end-to-end critical path.

Let `p` be the measured fraction of candidate end-to-end time spent in inverse-BWT, and `s` its isolated speedup under otherwise equal work. The maximum achievable total-time reduction is `p·(1 − 1/s)`. Therefore `p > 0.1532` is a necessary condition to close the cost axis *even with an infinitely fast inverse*. Illustrative **conditional** requirements: `p=0.2 -> s≥4.27×`; `p=0.3 -> s≥2.04×`; `p=0.4 -> s≥1.62×`; `p=0.6 -> s≥1.34×`. These are mathematical planning bounds, **not observed shares or speedups**.

First profile/sum decode costs on exact BWT-routed cap-8 files, without attributing the Brotli-routed `mozilla` time to libsais. If `p≤0.1532`, PARK version-only decode as a solution to *this* Silesia gate, though the version can remain a maintenance/safety candidate. A positive microbench alone is insufficient.

## H1 / H0, pre-registered discriminators

**H1:** replacing only vendored libsais 2.10.4 with pinned upstream 2.12.2 significantly improves frozen, exact-byte inverse BWT decode; a same-run, end-to-end paired Silesia measurement closes or materially approaches the failing decode-vs-Brotli cost axis **without** violating unchanged wire bytes, source-hash, memory, decoder-size, or correctness gates.

**H0:** version change is neutral, slower, noisy, incompatible, or insufficient at *end-to-end* level to change the Silesia decision. In that case do not further promote a pure-version speed claim; try an explicitly distinct 2/4-thread test only if a new measurable reason exists.

**Competing hypothesis:** most latency lies outside libsais (I/O framing, MTF/RLE entropy decoding, or Brotli-routed files). A successful isolated inverse-BWT microbench would then yield little end-to-end change.

### Phase S: smallest legitimate manual GitHub Actions experiment (version ONLY)

1. **Freeze identities:** use candidate ANVIL SHA `2c5eeb9e570b1e1162cbff98d29e0a978edeca92`; vendor *exact* upstream **tag v2.12.2 resolved to immutable commit SHA at dispatch**. Record the exact old/new `libsais.c`, `libsais.h`, `LICENSE` SHA-256, full compiler and flags, binary hashes, uname/CPU topology. Two isolated build trees, same ANVIL code, same Clang 18/Release `-O3`, no OpenMP in either; no implicit optimization or candidate-policy changes.
2. **Fixed workloads:** decode **identical sha-pinned compressed cap-8 streams** produced by the frozen **old** encoder on Silesia and at least one non-BWT-routed negative control. Build two decoder variants differing *only* in libsais version. Keep existing BWT auxiliary sample indexes and `--bwt-subblock=8388608`. Compare real BWT-routed files with a Brotli-routed control so a seeming universal speedup cannot be attributed to just changing another codec path.
3. **Correctness before timing:** SHA-256 of each decoded output must equal frozen original, with intermediate BWT block goldens. Include empty/minimal, malformed/truncated, forced decoder-visible IDs, and hostile BWT invalid-input checks. If changed source alters forward encoding, **separately** measure complete-wire bytes and reject any unacknowledged format/selection confound; a decode-only proof uses precisely the **same old-encoded bytes** on both arms.
4. **Same-job statistics:** single-CPU affinity for both serial builds, symmetric warmups, random AB/BA interleave, ≥9 pairs, frozen `paired_bench.py` seed 41246, 20k bootstrap reps, A/A null CI containing 1.0, `timing_valid=true`, raw repetitions + actual subprocess argv. No cross-host absolute MB/s comparisons. Separate stage-internal LF time attribution from full ANVIL subprocess decode time.
5. **Resource and compatibility gates:** measure peak RSS with separate processes, stripped **decoder-only** binary size plus linked runtime/dependency footprint, heap/stack including malformed input safety, full-wire identity and decode headers, and unchanged format backward compatibility. Honor the new upstream Apache-2.0 WITH LLVM-exception license by carrying exact upstream license/attribution—not just copying the C file.
6. **Budget:** use a short bounded scout on representative BWT-routed `dickens` and `mr` plus Brotli-routed `mozilla`, and only if a genuine (CI-supported, Amdahl-feasible) signal exists promote to full frozen Silesia + enwik8 + structured/held-out verification. Do not repeat A48's 12-file x6 paired encode grid or expend hundreds of needless q11 recompressions; encode once and reuse frozen streams.

**Acceptance for adoption:** a fully valid same-job paired end-to-end improvement demonstrably moves the **Brotli decode-time threshold** while not making any hard byte, correctness, peak-RSS, safety, compatibility or binary-size gate fail. Improvements that do not move the threshold remain interesting measured engineering evidence but **not a Pareto win**.

### Phase O: separate OpenMP factorial, only if decision-relevant

Do NOT mix this with Phase S and attribute a speedup to source version. On the same host/frozen BWT streams, compare **(old 2.10.4 serial, new 2.12.2 serial, new 2.12.2 OMP-enabled build's serial entrypoint, new 2.12.2 `libsais_unbwt_aux_omp` at 2 and 4 threads)**. Match affinity masks to requested thread count; record true core quota, CPU-time cost, worker stack/heap, `libgomp` size, binary size, output hashes, A/A null and paired CI. No `--cpu=0` pin for multicore arms. Do not infer that threaded LF walk is parallel merely because an OMP API was called: sample rate `r`, number of aux walk blocks and actual per-thread work must be recorded. If fewer walk blocks than threads, do not report that case as a clean scalability test.

### Integrity, stopping, ownership

- This is an **adoption-class** experiment, not a new algorithm/novelty claim. Research status before evidence: **VERIFY/TEST**. Risk execution status: **AWAIT_AUTHORIZATION** if a new workflow cannot be published through an approved channel. A documented permission denial is never permission to route through another app to evade it.
- Run no workstation/homelab builds, benchmarks, fuzz or sweeps. No CI dispatch is authorized or performed by this document.
- Claim ladder: upstream release note → unverified in ANVIL → source-identified same-job measurement → reproduced corpus + held-out all-axis validation. Never collapse those levels.
- **Stop/park** on byte or correctness failure; performance null/noise; inadequate LF share by Amdahl; binary/RSS budget regression; unapproved workflow publication; or a result that cannot alter an architectural decision. Preserve all negative evidence.
- **Revisit triggers:** permitted workflow publication, verified old/new source and license hashes, or a concrete new mechanism that changes the predicted bound. If a conventional version refresh makes the cost viable, postpone blocked-LF invention. If not, require a distinct mechanism and causal ablation before implementation.

## Source-stage addendum — 18 November 2026 (no CI executed)

This preregistration was authored before the version-only candidate was prepared. **Source now IS staged** on this separate DRAFT branch, while the **experiment itself remains UNEXECUTED**. The C/header are official upstream libsais v2.12.2 commit `b1aa1fde090e00a4e2030b9fb54198698b84e463`; their vendored CRLF bytes normalize exactly to upstream LF source. The imported upstream LICENSE is exact and carries Apache-2.0 WITH LLVM-exception; `UPSTREAM.md` records original blob IDs. No ANVIL codec code, CMake flags, wire format or workflow registrations were changed. Do not interpret a successful source copy as binary compatibility or speedup evidence.

**New source-level performance controls identified by independent review:**

1. Upstream `src/libsais.c` conditionally selects `LIBSAIS_CPU_FEATURE_AVX2` only if the compiler defines `__AVX2__`; otherwise ordinary x86_64 Clang may choose the SSE2 path. Current ANVIL `CMakeLists.txt` builds libsais with `-O3 -DNDEBUG`, **not `-march=native` or `-mavx2`**. Both old/new arms must use identical flags and record the actual ISA code path. A subsequent ISA/AArch64/SIMD test is a *different causal factor*, not part of the version-only experiment.
2. New source declares `UNBWT_DECODE_16_MIN_SIZE = 8388608`, `UNBWT_CHAINS_LARGE_MIN_SIZE = 8388608`, `UNBWT_CHAIN_ANY_BLOCKS_MIN_SIZE = 33554432` and `UNBWT_CHAIN_MIN_SIZE_AUX = 1048576`. These cross the exact cap-8 subblock boundary. Record **actual** real BWT subblock sizes/aux sampling rates and not merely file-level MB/s; full blocks may activate an implementation path absent for smaller tails. This identifies a possible heterogeneity, NOT an observed performance gain.
3. The v2.12.2 LF source file grows from approximately 388 KB upstream LF (v2.10.4) to 708 KB LF. Source size is **not** binary size. The full decoder-only executable, static library and runtime dependency footprint must still be measured under the originally frozen budget.
4. A version-only import changes the available encoder-side `libsais_bwt_aux` implementation too; to isolate decoder causality, decode exactly the SAME frozen **old-encoded** wire inputs in both arms. Only after decode-only safety/performance tests should newly encoded complete-wire bytes be compared as a separate factor. The four public function signatures used by ANVIL are identical between versions, but correct aux-index semantics/encoded compatibility remain to be tested.

No heavy local builds, CI publication or dispatch have been performed. The draft remains intentionally unmerged.

## A70 audit update — authentic old-wire provenance and route classification, 19 November 2026

This supersedes any earlier assumption that the three selected files *necessarily* exercise the intended backend. A separate peer independently parsed the **actual frozen A48 artifact** (run `44380412243`, artifact `14480494484`, archive SHA-256 `71b6b288b1a2d3f2ae0adf132287e0b1a790642e937ba9526018f8b678ea3eab`) without running the codec. Source `tools/a48_grid.py` at the A48 branch uses the **identical** `--parse=ratio --ratio-backend=auto --ratio-context=off --ratio-lines=off --bwt-aux=on --bwt-subblock=8388608 --quiet` command as A70's old-encoder stage.

Actual full `ANV0` revision-2 header parses from those historical immutable streams:

| Input | Total old-wire bytes | SHA-256 of complete ANVIL file | Block mode/transform/backend |
|---|---:|---|---|
| dickens | 2,636,635 | `e2f188e5dc5879d622bc71278cdbb40e68baae73515379353a0fe498c0cb670d` | `17 / 0 / 2` = direct, **BWT** |
| mr | 2,413,023 | `9cf875f7a51c910ba5772d31f7983400e8b89ee3d217f74bd1bc67101b5eed7c` | `17 / 0 / 2` = direct, **BWT** |
| mozilla | 13,806,173 | `175c68610e213bb7af3275b7ef5cd51f008add01e2865e9a44020dc1a452443c` | `17 / 0 / 1` = direct, **Brotli** (negative control) |
| nci (not in A70) | 1,497,442 | `6e5079237863632dfd1a2652d87af36d62133ce30743e62864526f389f821a93` | `17 / 0 / 1` = direct, Brotli |

All four historical streams consist of **one revision-2 block**, declared source sizes match the frozen corpus, and the full input is consumed without trailing bytes. These are *historical* observations, not new A70 decode measurements; the A70 driver rechecks the freshly generated old-wire streams before every paired experiment.

**Implemented safeguards in staged PR #24:** `inspect_ratio_wire` scans source-accurate ANVIL framing to record each block's actual decode backend and transformation without decompressing. `require_preregistered_route` fails closed unless `dickens/mr` are BWT-only and `mozilla` Brotli-only (mixed/unknown modes are not admissible). `audit_frozen_old_wire` records whether freshly created old-encoder outputs **exactly reproduce** the independent previous A48 full-stream size and SHA-256. A mismatch is a conspicuous *HISTORICAL_WIRE_DRIFT* diagnostic that forbids claims of cross-run A48 byte comparability, **not an unconditional hard stop on a valid same-job old-versus-new decoder experiment**. Both decoders continue to consume the exact same freshly frozen wire, under independent mandatory route, output-SHA, and source/compile controls. The strict backend route is the causal prerequisite; historical cross-run byte identity is useful corroboration rather than an indispensable cause-control requirement.

**Scalar verification of staged files:** `python -m unittest discover -s tests -p test_a70_version_scout.py -v` in an isolated local scratch directory produced **20 tests discovered, 19 PASS, 1 Linux-only `/usr/bin/time` test SKIPPED**, exit 0. Nine new routing/malformed header cases, a malformed payload-boundary case, and three old-wire replay/diagnostic cases were included. One independently identified parser weakness—letting a ratio varint extend beyond its declared payload boundary—was repaired and tested. The scratch directory was deleted after inspection. This was **not** a codec build, LLVM/Clang check, dataset decompression, hosted GHA run or real performance measurement. Linux RSS and full old-wire regeneration still require the authorized manual-only hosted A70 workflow.

**Disposition:** `PROCEED_WITH_GUARDS` for the source-only protocol hardening following an independent read-only reviewer challenge (no parser/source framing mismatch found; historical SHA hard-stop disproven as necessary); `AWAIT_AUTHORIZATION` for publishing the new workflow and dispatching a bounded GHA run. No universal Pareto claim follows from this three-file decoder-scout protocol.

## Pre-outcome protocol correction — 26 November 2026

The user explicitly authorized continuation through OXP's GitHub tooling and
required **all CPU validation on GitHub Actions**, including unit and scalar
tests. This supersedes the older source-only execution disposition above.
Source editing and GitHub API operations are permitted locally. No tests,
builds, codec execution, profiling or benchmark calculations are run locally.
Real repository/tool permission failures remain binding; historical permission
claims are not treated as a newly observed denial.

This correction starts from reviewed draft PR #24 head
`f40a8ac28d87e851b67664131a801bd357d16a44`. Live workflow/run inspection on
26 November found no registered A70 workflow and no A70 run. No A70 performance
outcome was observed before this correction. The existing I20 stage-decomposition
workflow **is** registered, contrary to the stale session blocker, but its old
whole-corpus profile is not this bounded scout.

### Correction and independent acceptance gate

The evaluator-owned acceptance cases are frozen in
`A70-PROTOCOL-CORRECTION-ACCEPTANCE-20261126.md` before hosted execution.

1. **Controlled outer-frame rejection.** Arbitrary nonzero exit is no longer a
   pass. Both direct decoder processes must return exactly 1, emit the exact
   pinned `anvil: truncated block payload` diagnostic (with at most its normal
   terminal newline), and leave neither final output nor `.anvil-part` behind.
   Dirty output paths are rejected before execution. Process output and failure
   classification are archived before errors propagate. The fixture removes one
   final byte from the verified old wire. This exercises the outer frame length
   guard before libsais; it does not establish hostile-BWT or dependency safety.
2. **Complete repetition contract.** A70 admits only 9 or 13 pairs at workflow,
   CLI and callable boundaries. Nine remains the default. An interrupted session
   is never resumed, pooled with another run, shortened, or bootstrapped as though
   complete. No null control can be dropped to fit a resource budget.
3. **Durable observations with the frozen estimator.** Keep
   `tools/paired_bench.py` byte-for-byte unchanged, pinned SHA-256
   `9a42bf80f6a35bbec61f191a40daf416652e1ee9c35fd7461c5de13755209b08`.
   `a70_paired_capture.py` delegates its timed process and output-observation
   calls. It writes/fsyncs command starts, returned elapsed values and verified
   arm observations as separate events, outside the original subprocess timer.
   It checks complete schedule/raw/summary coverage before declaring COMPLETE.
   Reused journals, states, CSVs or summaries are refused. Each designated
   decoded output is cleared before its next timed invocation, outside the
   timer, so a successful process that writes nothing cannot reuse old bytes.
   These outputs are distinct from the immutable packed input. Inter-invocation
   journaling is present symmetrically; fake-clock parity establishes semantic
   clock/statistic parity, not a claim of zero physical system overhead. Fresh
   destination-file state is a deliberate observation-protocol revision applied
   to both arms. New timing stands on its own same-job controls and is never
   pooled with timing from an earlier observation protocol.
4. **Incomplete evidence remains visible.** Persist source/phase progress and
   each completed session. A current file is complete only after compatibility,
   RSS, both nulls and the comparison. A failure record names the phase and retains
   earlier complete files. Missing capture completion, including after SIGKILL,
   means incomplete. Partial diagnostic timings do not receive a speed verdict.
5. **Identity and bounded execution.** Workflow source and candidate source are
   recorded separately. Checkout requires an explicit full candidate commit and
   equality check. The workflow retains the old/new `cmp` checks for codec,
   CMake, frozen statistical runner and corpus tooling. The default manual phase
   is `protocol-only`; it cannot fall through into a codec build or scout. Review
   its hosted evidence before a separate manual `scout` dispatch.

### Unchanged scientific settings and resource limits

The version-only source import, old encoder, three named files and route guards,
same old wire on both decoder arms, serial Clang 18 Release flags, affinity rule,
two symmetric warmups, seed 41246, paired log-ratio median, 20,000 bootstrap
samples with seed `41246 ^ 0xA5A5A5A5`, practical epsilon 0.02, arm robust-CV 0.15,
ambient settings 5 / 250000 / 0.10, and pre/post A/A controls are unchanged.
Full CLI size and single-process RSS remain scoped scout diagnostics.

The hosted job remains limited to 45 minutes. Preparation has an absolute
900-second budget. Measurement has a maximum 1,200-second budget and an absolute
cutoff 2,100 seconds after job start; its internal budget is 15 seconds shorter
than the outer timeout. Each new paired session requires its complete frozen
600-second cap plus cleanup reserve. Failure to admit a session is
`NOT_STARTED_BUDGET`, not a reduced experiment. Process-group termination bounds
cleanup and preserves logs. Upload has its own five-minute cap, inside the job
reserve. An `always()` upload includes process diagnostics, JSONL journals and
failure/progress state. Loss of the entire hosted runner still cannot guarantee
artifact recovery.

Passing this infrastructure gate establishes protocol behavior only. Passing
the three-file scout establishes, at most, scoped same-job decode evidence.
Mozilla remains a Brotli-route negative control; an apparent change there is a
causal concern, not an inverse-BWT success. No scout label or green workflow
establishes full dependency safety, all-axis Pareto dominance, held-out
generalization, decoder-only distribution footprint or adoption readiness.
Draft PRs #23–26 remain unmerged and historical evidence remains unchanged.
