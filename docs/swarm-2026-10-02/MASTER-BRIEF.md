# Project ANVIL — 40-Agent Master Swarm Brief

> **HISTORICAL DISCOVERY BRIEF — NOT CURRENT EXECUTION AUTHORITY.**
> The 40-agent discovery phase is closed. For current lane status, dispatch order, and permissions,
> use **`COORDINATOR-FREEZE-v2.md`**. Any "active/promoted" wording below describes the swarm's
> starting state and is superseded where the v2 freeze says otherwise.

**Coordinator date:** 2026-10-02
**Live worktree:** `i10-aux-unbwt` at `b8eae11`, intentionally dirty.
**Mission:** discover and close the next genuinely defensible path toward a new general-purpose lossless compressor that beats Brotli on a reproducible ratio/performance Pareto frontier.

## Binding doctrine

1. Mechanism novelty, not novelty-by-difference. Prior art must be treated as a first-class constraint.
2. No ratio-only wins. Charge every decoder-visible byte, table, model, dictionary, framing field, selector, planner cost, and reversible carrier.
3. Raw input is always a candidate. Every transform/representation is judged end-to-end against the same backend.
4. Correctness is non-negotiable: deterministic bounded decode, byte-exact roundtrip, explicit malformed-input behavior.
5. Do not move thresholds after seeing results. Define gates and kill criteria before measurement.
6. Do not cherry-pick. Discovery, validation, and held-out families must be separated.
7. CPU-heavy compression/performance benchmarking is GitHub Actions only. Do not run local corpus benchmarks, sweeps, or heavy fuzz campaigns.
8. The local worktree is intentionally dirty. **Do not reset, clean, stash, restore, overwrite, rebase, commit, or push.**
9. Do not modify existing production/source/docs files unless your task explicitly requires it. Prefer isolated new files.
10. Preserve provenance: every factual claim must identify source artifact/code location or be clearly labeled hypothesis/projection.
11. Current canonical frozen frontier remains **0 FRONT-CROSSING**; GRID-THIN remains relevant.
12. I9 wire-invisible decode optimization is exhausted as a crossing route. CRC PCLMUL is a real retained wire-identical win; materialization is retired; ALLOC and postcoder leg-4 are real but DECODE-SHORT. Do not re-burn those exact experiments.
13. G4 direct planner proxy rescue is closed NO-GO. G5B ordinal proxy is adverse. Global lane transposition is killed. PORDER as standalone novelty is killed.
14. Active/promoted I10 directions include: G5A-informed bounded finalist planning, paged base+overlay exact dictionaries, decoder/backend co-design, bounded BWT subblocking, new held-out families, and mechanism-level novelty beyond merely wrapping Brotli.
15. Existing strong evidence: G5A ordering is material and column-dominant; G5D source-level blockers were fixed but publication/dispatch was not authorized; DEFLATE reconstruction is adopt-class prior art with real engineering value; exact TCOPY/PNRA-style transforms remain one defensible novelty neighborhood; statistical zero-bit derived transforms are closed.

## Deliverable contract

Each worker must:
- read this brief plus the relevant ledger/docs/source for its track;
- produce a concise but rigorous report at the exact unique path named in its task;
- include: current evidence, falsifiable hypothesis, mechanism definition, decoder state/cost model, novelty/prior-art risks, complexity/performance model, minimum prototype, remote-only benchmark plan, pre-registered GO/NO-GO thresholds, failure modes, and a final recommendation: **KILL / HOLD / PILOT / PROMOTE-TO-REMOTE**;
- distinguish measured facts from projections;
- avoid duplicating already-closed work;
- if code is useful, place only bounded isolated prototypes under `prototypes/swarm-2026-10-02/<track>/<agent>/`; do not wire into production;
- no commits/pushes and no heavy local benchmarks.

## 20 paired tracks

01. G5D paged base+overlay exact dictionary: cost model, dictionary drift, escape economics, decode locality.
02. G5A-informed finalist planner: analytical MDL features, bounded q4 calibration, regret bounds, selector economics.
03. Held-out corpus protocol: independent structured/mixed/executable/numeric families, leakage controls, corpus locking.
04. Dense Pareto measurement: reference density, timing/RSS/binary-size validity, robust statistics, arbiter semantics.
05. Backend frontier beyond Brotli: identify promising new/replaceable backends and what must be novel vs adopt-class.
06. Entropy backend co-design: rANS/FSE/arithmetic/context-mixing layouts with decoder throughput and model-cost accounting.
07. BWT subblocking: memory/decode/ratio tradeoff, auxiliary indexing, block-boundary economics.
08. DEFLATE reconstruction integration: clean-room architecture, routing economics, exactness/fuzz/security, portfolio effect.
09. Exact transformed backreferences: TCOPY/PNRA exact invariants, implicit decoder-known parameters, executable-relative fields.
10. Correction topology coding: sparse residual structure, positional/mask coding, novelty boundary and parser interaction.
11. Orbit/program synthesis compression: bounded reversible micro-programs, MDL parser, decoder ISA, proof of boundedness.
12. Encoder-only search/learning: zero-model decoder, search over reversible representations, bounded inference cost, anti-overfit.
13. Numeric/floating representation compiler: reversible typed lanes, Gorilla/FPC/ALP-like prior art boundaries, mixed data.
14. Executable/binary representation compiler: relocations, instruction fields, sections, exact transforms without BCJ cloning.
15. Long-range/cross-block memory: dual-timescale dictionaries, chunk reuse, bounded random access and decoder memory.
16. Segmentation/routing theory: region discovery, candidate pruning, regret bounds, fallback guarantees.
17. Parser/candidate-generation algorithms: asymptotics, SIMD/hash/index structures, measured-cost MDL without encode collapse.
18. New decoder/backend architecture: data layout, vectorization, superinstructions, independent-stream scheduling for future formats.
19. Format/security/fuzz: bounded allocations, corruption containment, deterministic resource ceilings, adversarial format design.
20. Novelty/prior-art kill team: map candidate mechanisms to literature/patents/codecs and define decisive novelty separators.

## Pairing rule

For every track:
- **Space Bunny Free** is the constructive inventor: find the strongest defensible mechanism and concrete pilot.
- **Fledge Alpha Free** is the independent adversarial reviewer: try to falsify the Space-Bunny-shaped direction without relying on its output, search for prior art/hidden costs, and propose a materially different alternative if the main idea fails.

Do not communicate assumptions as facts. The coordinator will reconcile contradictions after all 40 reports settle.
