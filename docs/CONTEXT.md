# Project ANVIL — Shared Swarm Context (v1)

Mission: discover, design, implement, and experimentally validate a genuinely
new general-purpose lossless compression architecture that beats Brotli on a
meaningful compression/performance Pareto frontier. Not "optimize Brotli."

## Ground rules

- Novelty must be **mechanism-level** and earn its existence quantitatively.
  A new file format, renamed primitive, or arbitrary combination of known
  components is NOT sufficient. Novelty = a new mathematical model,
  representation, parsing strategy, prediction mechanism, coding method,
  search formulation, adaptation scheme, transform, or a non-obvious
  interaction of known techniques that moves the Pareto frontier.
- **Novelty gate** (every candidate mechanism must pass):
  1. prior-art lineage (what exists, why it lost, what Brotli exploits)
  2. precise statement of what is NEW
  3. reason the interaction should move the Pareto frontier
  4. ablation showing the gain comes from the mechanism, not noise
- Stand on prior art. Re-test abandoned ideas with modern hardware in mind
  (SIMD, cache hierarchies, parallel execution). Separate mathematical
  limitations from implementation-era limitations.
- Be falsifiable. Prefer a single-pass, cache-resident parser that preserves
  strong structured-data ratios over an expensive global DP parser that cannot
  ship.

## Environment (Windows x64 host)

- Working dir: `C:\Users\slooshied\Documents\ANVIL` (this repo).
- Load toolchain EVERY session:  `. .\env.ps1`  (adds clang, cmake, ninja +
  MSVC/SDK env via vcvars64.bat).
- Build:  `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_SUPPRESS_REGENERATION=ON` then
  `ninja -C build`.  (CMAKE_SUPPRESS_REGENERATION=ON is required — CMake 4.4 +
  Ninja otherwise loops on "manifest still dirty".)
- Targets: `anvil` (codec CLI) and `anvil_bench` (Brotli/Zstd comparison).
- Third-party brotli/zstd static libs: `tools/setup_third_party.ps1`
  (clones + builds into `third_party/install`). Already built and committed
  as infrastructure is NOT — third_party/ is git-ignored; re-run the script
  on a fresh clone.
- Bench single file:  `build\anvil_bench.exe <file> <reps>`
- Bench corpus:      `python tools\bench_suite.py <files...> --bench
  build\anvil_bench.exe --reps 3 --out tests\benchmark-suite.csv`
- Fuzz:              `python tests\fuzz.py --exe build\anvil.exe --cases 50`
- Compiler: clang 22 (clang-cl mode), C++20. Codec has no external deps.

## Codec CLI

```
build\anvil.exe c <in> <out> [--parse=greedy|dp|auto] [--literal=o0|o1|g4|g8|g16|auto] [--entropy=arith|rans|auto] [--block=N] [--chain=N] [--max-match=N]
build\anvil.exe d <in> <out>
build\anvil.exe verify <in> [options]
```

## Baseline measured on THIS Windows host (clang-cl Release)

`tests/corpus/doc.md` (2,724 B, markdown):
  anvil-greedy-arith 0.537 / anvil-dp-arith 0.530 / anvil-greedy-rans 0.593
  / anvil-dp-rans 0.577 ; brotli q1 0.513 q4 0.470 q6 0.442 q9 0.442 q11 0.384 ;
  zstd 1 0.497 / 9 0.486 / 19 0.481

`tests/corpus/generated.json` (827,664 B, structured JSON):
  anvil-greedy-rans 0.175 / anvil-dp-rans 0.138 ; brotli q6 0.143 q9 0.137
  q11 0.096 ; zstd 9 0.145 19 0.113

Conclusion: on structured data ANVIL dp-rans ratio (~0.138) already matches
brotli q9 but is ~10x slower at encode and ~3x slower at decode → NOT a Pareto
win. Throughput and parser cost are the gating problems. Windows host numbers
are directional; a shared Linux box gave ~2-3x higher throughput.

## Research history (Linux sessions, prior to this repo)

- Greedy LZ + adaptive arithmetic: correct but slow decode (Fenwick tree).
- DP parse minimizing estimated bit cost: -0.94% bytes, ~4x slower encode.
- Unconditional order-1 literal contexts: REJECTED (+3.46% bytes, cold-start
  sparsity). Confidence-gated order-1 kept only for cross-corpus tests.
- Separated-stream static rANS (5 streams: token type, lit-len varint bytes,
  match-len varint bytes, match-dist varint bytes, literals): ~8x faster
  decode at ~+1% bytes. rANS became the performance backend.
- Interleaved 4-way rANS states (rans2x4): ~30% decode speedup on Windows.
- Structured cost model DP ("dp2"): class+bucketed lengths and distances,
  recency-of-distance coding, two refinement passes. On the 26,904 B task doc
  reached 0.376 vs brotli q4 0.379 (ratio win) but ~1 MB/s-scale encode.
- Suffix-array match candidates ("dpsa") for DP: further small ratio gains,
  also slow.
- Compact rANS stream serialization (range lo..hi + symbol mask, frequencies
  only for non-last symbols): saved model-header bytes.
- Literal transform probe (xor-with-prev / delta, modes "rans2t"): measured;
  kept only when end-to-end bytes improve.

Key engineering conclusion carried forward: the 1-2 MB/s global DP parser is
the bottleneck; a single-pass, cache-resident parser with measured downstream
rANS costs is the priority, while preserving the strong structured-data ratio.

## Highest-priority mechanism under consideration

**Approximate self-reference with sparse correction** (working title:
"SPARSE-REF"). Instead of requiring an LZ match to be byte-identical, copy a
prior phrase and entropy-code a sparse correction mask + residuals. The decoder
stays essentially memcpy + sparse stores. Research questions: can an MDL parser
jointly choose exact LZ edges AND sparse-corrected phrase edges; can recurring
correction topology be entropy-coded; does it materially beat exact-match
parsing on structured data (JSON/logs/SQLite) where Brotli leans on context
modeling and large dictionaries?

Prior-art lineage to establish (not claimed yet): delta/copy-patch coding,
approximate matching, recent-offset reuse, grammar/LZ hybrids. Determine
whether the specific formulation (self-referential phrase propagating a
recently-discovered structural distance across non-identical records + jointly
coded sparse mismatch topology) is established prior art. Treat as a hypothesis
until the literature check + ablations support a novelty claim.

## Deliverable artifacts

- `src/anvil.cpp` — the codec (encoder + decoder + parsers + backends).
- `FORMAT.md` — bitstream/format spec; every new block mode documented and
  versioned here. Decoder must reject malformed input strictly.
- `RESEARCH_LEDGER.md` — experiment narratives, numbers, and decisions.
- `tests/benchmark-suite.csv` — per-file rows; `tests/benchmark-summary.csv`
  aggregates.
- `docs/research-agenda.md` — novelty-gated candidate mechanisms, ranked.

## Working agreements for the swarm

- Only the `arch` lane edits `src/anvil.cpp`; `format` owns `FORMAT.md` and
  decoder strictness; `bench` owns benchmark suite + CSVs + corpus;
  `research` owns the novelty gate + agenda + ledger narrative. Coordinate
  before touching a peer's lane.
- Every experiment: encode+decode must round-trip (verify) and be fuzzed
  before any ratio claim. Throughput measured with `anvil_bench` (median of
  >=3 reps).
- When a mechanism is dropped, record WHY in the ledger (math vs
  implementation-era).
- Never change an existing block mode's wire format; add a new mode and keep
  the old decoder path. `--parse=auto`/`--entropy=auto` brute-force research
  search is allowed but production defaults must be single-candidate.
