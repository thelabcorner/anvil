# Project ANVIL

**ANVIL is an experimental general-purpose lossless-compression research project.**

Its objective is not to produce another wrapper around Brotli, Zstd, or LZMA. The research target is a materially better **compression / encode / decode / memory** Pareto frontier, with mechanism-level advances that remain useful on real corpora.

The central research thesis is:

> The cheapest byte to encode is a byte the decoder can reconstruct from information it already has.

That means ANVIL studies not only entropy coding, but the representation that exists *before* entropy coding: exact references, transformed references, reversible reconstruction, bounded generation rules, structure exposure, backend routing, and hardware-conscious decoder execution.

## Current status

ANVIL is a research prototype. The bitstream is unstable and the project is not a production replacement for established codecs.

The frozen Iteration-9 checkpoint demonstrated that ANVIL's ratio portfolio can beat the measured Brotli q11 and xz -9e byte totals on the project's canonical Silesia/enwik8 runs, but **it produced zero complete Pareto-front crossings** because the BWT-heavy path remained far too slow to decode.

Iteration 10 has now closed its first adopt-class improvement: auxiliary-index inverse BWT. On three paired same-runner targets it improved whole-codec decode by **1.364× to 2.339×** for a fully charged 2–4 KiB-per-file wire increase. Across canonical Silesia + enwik8, the size-first portfolio pays **22,398 B** (+0.00718% of source) with no routing changes. The legacy smaller representation remains the default for size-first mode; auxiliary BWT is retained as a distinct faster-decode Pareto option.

The same-runner external reference-cost gate has now closed that question: I10-1A still wins bytes, but is **FRONT-GAP_COST**, not a complete reference-front crossing. On Silesia it is 1.72x xz decode time but 4.57x xz peak RSS; on enwik8 it is 3.91x xz decode time with ~599 MiB peak RSS. Decode dependency depth and BWT working-set size therefore remain binding system axes.

The current research reset and execution program are documented in:

- [`docs/FRONTIER-RESET-2026-09-23.md`](docs/FRONTIER-RESET-2026-09-23.md) — post-I9 synthesis and mechanism frontier
- [`docs/FRONTIER-RESEARCH-ADDENDUM-2026-09-23.md`](docs/FRONTIER-RESEARCH-ADDENDUM-2026-09-23.md) — post-I10-1A program-synthesis, information-theory, and CPU-native decode research addendum
- [`docs/I10-EXPLANATION-GAP-ORACLE-DESIGN.md`](docs/I10-EXPLANATION-GAP-ORACLE-DESIGN.md) — concrete oracle architecture for identifying which explanatory family ANVIL is actually missing
- [`docs/I10-GROTLI-REPRESENTATION-SYNTHESIS.md`](docs/I10-GROTLI-REPRESENTATION-SYNTHESIS.md) — Grotli-derived adaptive representation compiler architecture: structure discovery, predictor/residual synthesis, exact downstream arbitration, and raw-Brotli fallback
- [`docs/I10-GROTLI-G0-PREREG.md`](docs/I10-GROTLI-G0-PREREG.md) — frozen first causal experiment: exact record-aligned vXOR+Brotli versus raw Brotli
- [`docs/I10-GROTLI-G0-CORPUS-FREEZE.md`](docs/I10-GROTLI-G0-CORPUS-FREEZE.md) — immutable discovery/validation split and upstream Git-object identities frozen before G0 implementation
- [`docs/I10-BREAKTHROUGH-PROGRAM.md`](docs/I10-BREAKTHROUGH-PROGRAM.md) — falsifiable Iteration-10 execution program
- [`docs/I10-AUX-UNBWT-RESULTS.md`](docs/I10-AUX-UNBWT-RESULTS.md) — closed I10-1A paired timing, byte economics, hardening and ruling
- [`docs/I10-REMOTE-BASELINE-CLOSURE.md`](docs/I10-REMOTE-BASELINE-CLOSURE.md) — closed Linux/GitHub baseline and deterministic I9 identity ruling
- [`docs/anvil-i9-findings.md`](docs/anvil-i9-findings.md)
- [`RESEARCH_LEDGER.md`](RESEARCH_LEDGER.md)
- [`docs/audit-2026-09-07/06-do-not-reburn.md`](docs/audit-2026-09-07/06-do-not-reburn.md)

## What exists today

The codebase contains several research generations under one decoder/format framework:

- block framing with raw fallback and CRC-32;
- exact LZ parsing and multiple match-finder/parser experiments;
- sparse-corrected self-reference (`COPY_PATCH`);
- implicit transformed copy (`TCOPY`);
- transformation-invariant candidate experiments (PNRA);
- semantic shape and hot-op representations;
- adaptive arithmetic and static rANS backends;
- multiple rANS precision modes;
- context-switched rANS experiments;
- Huffman / default-with-exceptions / stream-selection infrastructure;
- BWT backend built on pinned `libsais`;
- Brotli/BWT whole-file backend routing;
- reversible transform experiments;
- strict malformed/truncation checks;
- adversarial/golden/fuzz testing;
- native Brotli/Zstd/xz comparison tooling;
- Pareto/reference-class analysis tooling.

Not every implemented mode is considered successful. Failed mechanisms are deliberately kept in the research record so they are not rediscovered and re-burned.

## Iteration-9 checkpoint

The canonical I9 report records the following deterministic byte totals:

| Corpus | Input | ANVIL auto | Brotli q11/lw30 | xz -9e |
| --- | ---: | ---: | ---: | ---: |
| Silesia | 211,938,580 B | 46,446,995 B | 49,383,136 B | 48,456,100 B |
| enwik8 | 100,000,000 B | 23,534,368 B | 24,810,180 B | 24,831,656 B |

Those byte results coexist with a major decode deficit. See `docs/anvil-i9-findings.md` for the exact reference class, provenance, timing grades, and the `FRONT-GAP` / `FRONT-CROSSING` rules.

The project does **not** describe the above as a general Pareto breakthrough.

## Current research direction

ANVIL is moving away from the question:

> How do we entropy-code these bytes a little better?

and toward:

> How can the decoder explain more output bytes from compact, cheap, deterministic state?

The working research abstraction is a bounded set of **decoder-visible explanations**:

- `LITERAL` — explicit irreducible residual;
- `COPY` — exact temporal reference;
- `COPY_PATCH` — reference plus sparse correction;
- `TRANSFORM_COPY` — reference under a compact deterministic transform;
- `GENERATE` — bounded rule that produces many output bytes;
- `REPLAY` — reconstruct an encoded/container representation from normalized data plus correction metadata.

This is a research model, not a novelty claim. Each concrete mechanism still has to pass ANVIL's prior-art and quantitative gates.

The first high-EV I9 carry-forward is now closed:

1. **auxiliary-index inverse BWT — ADOPTED as an explicit faster-decode Pareto option**, while the byte-smaller legacy representation remains the size-first default.

The next isolated engineering experiment is:

2. **native bit-exact DEFLATE reconstruction (P4.1)**, followed by direct roundtrip/fuzz, complete wire/code-size accounting, and same-backend causal measurement.

New mechanism work is expected to begin with **anatomy/oracle probes**, not full codec modes.

## Research discipline

Every mechanism is expected to answer four questions before it becomes a claim:

1. What is the prior-art lineage?
2. What, precisely, is new?
3. Why should it move a real Pareto frontier?
4. Which controlled ablation proves that the mechanism caused the gain?

ANVIL also maintains a strict do-not-reburn register. A correlation, detector hit rate, lower entropy proxy, or synthetic win is not enough. Final value is measured in complete payload bytes plus runtime, memory, decoder cost, and correctness.

## Hardware-conscious architecture

The decoder is treated as a dataflow engine, not just a mathematical inverse.

The project actively investigates:

- PCLMUL-backed CRC;
- SIMD-friendly mismatch verification;
- multi-state entropy decoding;
- control/data separation;
- cache-resident decode tables;
- dense integer metadata coding;
- branch reduction and fused semantic operations;
- auxiliary-index BWT inversion;
- runtime ISA dispatch with a scalar fallback.

A wire-format idea that fundamentally requires expensive pointer chasing or branch-heavy interpretation has to justify that cost against the bytes it removes.

## Building

### Linux

Requirements:

- CMake
- Ninja
- C++20 compiler
- Brotli development libraries for the ratio backend/helper
- Zstd development libraries for the native comparison harness

Example:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build --parallel
```

The vendored `libsais` primitive is pinned under `third_party/libsais`.

### Windows

The historical Windows research environment uses `clang-cl` and Ninja. See [`docs/CONTEXT.md`](docs/CONTEXT.md) and [`tests/host-spec.md`](tests/host-spec.md) for the exact frozen environment associated with old measurements.

## CLI

The research CLI evolves with the format. Run:

```text
anvil
```

for the current option set.

Typical examples:

```bash
# research-oriented ratio path
./build/anvil c input.bin output.anv --parse=ratio --ratio-backend=auto

# decode
./build/anvil d output.anv restored.bin

# verify a parser/backend path
./build/anvil verify input.bin --parse=hotop --entropy=rans
```

Do not assume an experimental mode is a current recommendation merely because it remains available from the CLI.

## Correctness

The fuzz harness exercises roundtrips, malformed inputs, decoder-visible registry IDs, BWT goldens, and deterministic format cases.

```bash
python3 tests/fuzz.py --exe build/anvil --cases 50
```

Format changes must add decoder bounds and direct forced coverage. Selection-based coverage is not sufficient.

## Benchmarking

CPU-heavy ANVIL research is designed to run remotely through GitHub Actions rather than on the developer workstation.

See:

- [`docs/github-actions-benchmark-protocol.md`](docs/github-actions-benchmark-protocol.md) — normative statistical/promotion protocol
- [`docs/GITHUB-ACTIONS-BENCHMARKING.md`](docs/GITHUB-ACTIONS-BENCHMARKING.md) — operational CI/corpus/publication companion
- [`.github/workflows/anvil-research-bench.yml`](.github/workflows/anvil-research-bench.yml)

The workflow is manual-only and uploads raw artifacts. Shared GitHub runners are treated as a **scout/ranking environment for timing**, while deterministic bytes, hashes, and roundtrip results are independently reproducible quantities.

Canonical external corpora are downloaded at CI runtime and verified against pinned manifests; they are not committed to the repository.

## Repository map

| Path | Purpose |
| --- | --- |
| `src/anvil.cpp` | current encoder/decoder research implementation |
| `FORMAT.md` | current experimental wire-format contract |
| `RESEARCH_LEDGER.md` | experiment history, numbers, verdicts, corrections |
| `docs/` | research agenda, audits, prior-art notes, frozen iteration reports |
| `tests/` | fuzzing, corpus manifests, benchmark/verification artifacts |
| `tools/` | benchmark, Pareto, corpus, build and analysis tooling |
| `prototypes/` | isolated mechanism/oracle experiments |
| `third_party/libsais/` | pinned BWT/suffix-array primitive and upstream license |

## Reproducibility and claims

When documents disagree, prefer the most direct frozen artifact:

1. exact executable/CSV output and hashes;
2. current source + `FORMAT.md`;
3. explicit gate/ruling documents;
4. `RESEARCH_LEDGER.md`;
5. old context/brief documents.

Historical throughput numbers are host- and protocol-specific. Never transplant them to a different compiler/CPU and present them as current performance.

## Public research snapshot

The public repository is a sanitized, fresh-history snapshot of the research tree. Local OpenCode state, build products, workstation-specific caches, and private working metadata are intentionally excluded.

The absence of a file from the public snapshot therefore does not imply it never existed in the internal research process.
