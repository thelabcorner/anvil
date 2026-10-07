# ANVIL I11 — Work-Bounded Affine Reconstruction: Frozen Pilot Protocol
**Date:** 2026-10-07  
**Status:** Discovery / adopt-class engineering ONLY, no promotion, no production format, no novelty or frontier crossing claim.  
**Predecessor gate:** GitHub Actions Q1a-XRUN run [37694740386](https://github.com/thelabcorner/anvil/actions/runs/37694740386), green with uploaded evidence.  
**Execution:** GitHub Actions only. No local CPU-heavy compilation, fuzzing, performance, or codec benchmarks. No delegates.

## Hypothesis and strategic separation

ANVIL's current best-ratio BWT route wins selected compressed-byte comparisons yet remains substantially slower to decode than Brotli. An orthogonal low-work reconstruction primitive may win a useful **specialized, constrained** point on generated arithmetic/telemetry data without requiring BWT-like dependent-memory inversion. This pilot explores that substrate and **must not** be used as a general-purpose codec claim, held-out result, or retrospective alteration to R2.

Separate four hypotheses before measuring:
- H1 **representation**: for byte-exact columnar modular arithmetic sequences, full AVI1 serialized bytes are strictly less than same-input Brotli q11 with fixed lgwin22 (non-equivalent block windows; discovery only). The control is intentionally strong.
- H2 **decode efficiency**: for arithmetic sequences where AVI1 actually chooses AFF, its paired runner median decoded MB/s exceeds the Brotli q11 control. This may hold despite losing encode throughput, memory, or generality.
- H3 **negative control**: for high entropy and identical-record repetition inputs, AVI1's byte cost is no worse than RAW per 4KiB block plus dispatch/framing. It need not beat Brotli; a loss is expected and diagnostically meaningful.
- H4 **structure/selection**: arbitrary byte sources or misaligned 23-byte records may yield no AFF or PATCH selection. A false negative is important evidence that discovering field boundaries is a separate expensive problem.

None of the above establishes a general-purpose ANVIL frontier crossing.

## Frozen mechanism

Input limited to 256 MiB. Header: `"AVI1" || uvarint(uncompressed_byte_length)`. Each independent block is at most 4096 bytes. Per-block candidates:
- `RAW = 0 || uvarint(nbytes) || bytes`
- `AFF = 1 || uint8(word_width) || uvarint(count) || base_LE(width) || step_LE(width)`
- `PATCH = 2 || uint8(word_width) || uvarint(count) || base_LE(width) || step_LE(width) || uvarint(nexceptions) || (uvarint(gap), actual_LE(width))^k`

For widths `w in {1,2,4,8}`, with `N = 8w`, predict `p_i = (x_0 + iΔ) mod 2^N`, where `Δ = (x_1 − x_0) mod 2^N`. Exact mismatches are stored with their absolute replacement value at monotonically increasing indices via gap coding. `PATCH` accepted only up to `floor(count / 4)` mismatches (plus a byte-limited early abort). Candidate choice is by **actual serialized bytes**; ties choose RAW. No entropy backend, LZ match finder, dictionaries, format version negotiation, forward references, or transitive pointer chasing. Decoder is bounded sequential initialization plus sparse scatter. Every byte of descriptor metadata is charged.

Pseudo-claim `Δ²x_i=0` on pure affine streams is an occupied finite-difference mechanism; sparse corrections and FOR/delta coding are established prior art. **There is no mechanism novelty claim**.

## Exact source contract

- Implementation: `prototypes/i11-work-bounded-affine/i11_affine.cpp`.
- Pre-experiment SHA-256: `c530b278c8b007af58a4f6b9cd3c1adbec60e5c11588d8683867ea1ba8b5a285`.
- Pre-experiment Git blob: `69f7ca555b15dc5397411f86b6250b3772a8ce9b`.
- C++20 build: runner g++ optimized O3, supplied `I11_WITH_BROTLI` for Brotli q5/q11 controls.
- Brotli source: official `google/brotli` commit `ed738e842d2fbdf2d6459e39267a633c4a9b2f5d`, asserted after fetch, linked statically. Brotli window `lgwin=22`.
- Workflow checkout: exact `github.sha` and source Git blob; fail closed on mismatch.
- Workload is **already visible discovery material**, NOT promotion-authoritative heldout:
  1. `tests/corpus/synth-arith.bin`
  2. `tests/corpus/synth-timeseries.bin`
  3. `tests/corpus/synth-columnar-align.bin`
  4. `tests/corpus/synth-jitter.bin`
  5. `tests/corpus/random.bin`
  6. `tests/corpus/generated.repeat.jsonl`
  7. `tests/corpus/generated.jsonl`
  8. `tests/corpus/src.cpp`

Each input is checked against *its repository-committed Git blob and SHA-256* during the run; the accompanying report stores source/runner/compiler and corpus hashes.

## Executable checks

1. C++ source-identity and clean Git checkout check before any output.
2. Build of specific frozen Brotli reference at fixed commit, link static reference.
3. Build the AVI1 isolated experimental binary.
4. Deterministic selftests including empty, short, pure affine, sparse exceptions, random, truncated and malformed wire, with actual roundtrip equality.
5. Per-file full byte-exact roundtrip for all controls. Roundtrip mismatch fails the job.
6. Timed encode/decode medians (AVI1 encode 5 repeats, AVI1 decode 7; Brotli q5 encode 5/decode 7; Brotli q11 encode 3/decode 7) on same GitHub runner; report throughput as decimal MB/s, not MiB/s.
7. Capture complete bytes and mode-selection counts, logs, frozen runner/source hashes, and evidence artifact with `if: always()`.

**Work estimates are observational**, not constant-CPU guarantees: allocator effects, cache geometry and timing noise are controlled only to the extent of a same-run paired host and median; no size/RSS/binary footprint parity yet. Do not equate this pilot with production or Class-A frontier.

## Explicit stop/kill criteria

- **Correctness or malformed-header failure:** red workflow, no size/perf interpretation; repair and rerun with a new SHA and new prereg note.
- **Brotli source mismatch:** red workflow, no reference comparison.
- **Raw/affine wire size not charged honestly:** red workflow; no beneficial finding accepted.
- **No AFF selection on synthetic arithmetic:** mechanism inadequate; investigate width/block alignment rather than claim the test is unrepresentative.
- **Synthetic-only improvement:** isolate to discovery role, fund a future field-discovery/real-family pilot only after evidence constraints are satisfied.
- **AVI1 loses q11 compressed bytes on most datasets:** do not make a general ratio claim, regardless of speed.
- **Missing RSS/code size/strong matched-block comparisons:** does not qualify as full Pareto crossing even if any two dimensions look competitive.

## Branching future work (hypotheses only, not automatically funded)

A. **Architecture:** consolidate linear-run, RLE, and LZ-like copying in a common bounded-work reconstruction IR; benchmark exact same serialization overhead before funding dispatch improvements.  
B. **Candidate discovery:** learn field alignments by low-budget SIMD scans/event evidence; forbid O(n²) candidate lookup and accidental corpus leakage.  
C. **Exception factoring:** run/gap bitmaps, SIMD patched-lane scatters versus scalar baseline, including exceptions per cacheline and branches per decoded byte.  
D. **Entropy residual:** wire-level Brotli/Zstd on residuals only if cost of headers/framing/book survives exact sum; avoid comparing transformed payload size alone.  
E. **Rate versus work:** use epsilon-constrained candidate selection (max decode cycles/source byte and memory first, then minimize emitted bytes); do not invent uncalibrated exchange rates.  
F. **Adversarial falsification:** compare against FOR, PForDelta, Gorilla, Sprintz, FastLanes, ALP, OpenZL and fixed-DSL synthesis prior art; no novelty inferred merely from combination.

## Claim ledger

This experiment is designed to produce **evidence of viability or falsification**, not an automatic adoption decision. Existing Class-A baseline remains **435 dominated / 28 degenerate / 5 front-gap / 0 crossing** until a separate, clean, comparable whole-system preregistration and independent admissible held-out campaign establishes otherwise.

## Pre-codec-run population correction — 2026-10-07

GitHub Actions run [37695571938](https://github.com/thelabcorner/anvil/actions/runs/37695571938) **failed its input provenance stage, before building or invoking any codec**. The tracked GitHub `main` checkout does not contain `tests/corpus/synth-columnar-align.bin`, despite its description in local/historical corpus documentation. This is an infrastructure absence, not a negative or positive algorithm result. The missing file is excluded from the I11 discovery pilot rather than reconstructed without a provenance lock or smuggled from the operator worktree. The corrected, still discovery-only cohort has **seven**, all tracked, existing inputs (`synth-arith.bin`, `synth-timeseries.bin`, `synth-jitter.bin`, `random.bin`, `generated.repeat.jsonl`, `generated.jsonl`, `src.cpp`). Previous eight-path list remains above as historical intent, not as the executable revised cohort. No hypothesis threshold is moved, no rate/timing measurement has yet occurred, and this correction is not a post-hoc cherry-pick of performance. Future population additions require a new frozen source/input identity and separate preregistration. I11 source blob/hash is unchanged.
