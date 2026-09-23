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

- Working dir: `C:\Users\<user>\Documents\ANVIL` (this repo).
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

Live numbers live in `tests/benchmark-summary.csv` (aggregates) and
`tests/benchmark-suite.csv` (per-file rows, 8 files x 13 codecs, median 3
reps). Host spec: `tests/host-spec.md`. The rows below are the historical
first-pass numbers kept for context only.

Historical first pass — `tests/corpus/doc.md` (2,724 B, markdown):
  anvil-greedy-arith 0.537 / anvil-dp-arith 0.530 / anvil-greedy-rans 0.593
  / anvil-dp-rans 0.577 ; brotli q1 0.513 q4 0.470 q6 0.442 q9 0.442 q11 0.384 ;
  zstd 1 0.497 / 9 0.486 / 19 0.481

Historical first pass — `tests/corpus/generated.json` (827,664 B, structured
JSON): anvil-greedy-rans 0.175 / anvil-dp-rans 0.138 ; brotli q6 0.143 q9
0.137 q11 0.096 ; zstd 9 0.145 19 0.113

Fresh suite aggregate (8-file corpus, see summary CSV): anvil-dp-rans 0.1378
(encode ~1.1 MB/s, decode ~216 MB/s) vs brotli q9 0.1118 (34.8 MB/s / 829
MB/s) vs zstd-19 0.0997. Conclusion unchanged and sharper: on structured data
ANVIL dp-rans ratio ties/approaches brotli q9 only on the JSON-family files,
but aggregate ratio loses to brotli q9 and encode is ~30x slower → NOT a
Pareto win. Decode (rANS, ~216 MB/s) is already respectable; the gating
problems are encode speed (DP parser) and structured-data ratio depth.
Windows host numbers are directional; a shared Linux box gave ~2-3x higher
throughput.

SPARSE-REF anchor files (added by `bench`, t-setup, per agenda §1.4/§5):
- `tests/corpus/generated.jsonl` (2.82 MB, ~235 B records, mostly-identical
  skeleton, ~15% of bytes change per record at consistent positions): the
  target gap — anvil-dp-rans 0.087 vs brotli q9 0.073 vs zstd-19 0.059.
- `tests/corpus/generated.repeat.jsonl` (0.94 MB, identical records): the
  no-regression control — everything ~0.000-0.001; ANVIL exact-LZ 893-1157 B
  vs brotli 162-670 B, i.e. SPARSE-REF must not make this worse than exact LZ.
SPARSE-REF's falsifiable target is to attack the generated.jsonl gap at LZ-class
speed (agenda §1.3).

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

## Linux continuation (v2) — matured research state, measured Aug 2026

This is the state a parallel Linux codebase (`/mnt/data/anvil`, EPYC, GCC/clang,
src/anvil.cpp ~3300 lines, format rev 2) reached. The Windows swarm should build
on these results, not re-derive them. All ratios below are Linux-host numbers
(directional for Windows).

### Mechanism progression (all pass the novelty gate as implemented)

1. **Patch-phrase representation**: exact-LZ edge + `COPY_PATCH(dist,len,S,res)`
   — copy a prior phrase then apply k sparse residual ADDs. Decoder = copy +
   sparse stores. Parser families, cheapest first:
   - **RCM** (Residual-Continuation Matching): single-pass, 8-entry MRU
     distance cache; 64-bit XOR word compares; no DP/SA.
   - **SCM** (Structural-Channel Matching): separates recency cache from a
     persistent channel bank (12 slots, score decays with bytes elapsed,
     reinforced by long exact + successful patch phrases). Purely encoder-side
     state; bitstream unchanged.
   - **SRR/SSCM** (Synchronized Residual Reference / SCM): variant parsers with
     synchronized structural distance discovery.
   - **Negative gate**: cyclic difference-cover probe (mod-64, 10 phases) with
     content-hash thinning rejects incompressible blocks cheaply —
     random.bin encode measured 3,341 MB/s (vs 0.66 MB/s brotli q11); repeat
     copies at awkward mod-64 displacements still detected.
2. **Clustered residual backend** (mode 19): 16 learned residual classes +
     fallback; EM-style hard clustering over (patch-mask, slot) contexts with
     Jeffreys smoothing; class map transmitted once.
3. **Macro-ops** (mode 23): 16-bit semantic ops (literal class / kind+len-class
     +distance-code) as one fused command stream; decoder skips separate
     type/class/distance entropy passes for the common path.
4. **Compiled hot-op instructions** (modes 26/27/28): encoder synthesizes a
     decoder instruction book of concrete semantics (kind, len, dist,
     patch-selector); the hot stream is a small opcode index, rare tokens fall
     back to macro-ops. Coverage on generated.log: 75-77% of tokens hot.
     Variants: raw opcode stream (mode 27), raw rare/pmeta (mode 28).
5. **Stream-codec suite inside blocks** (all evaluated per stream, smallest
     wins): mode 1-4 rANS (1/4-state × full/compact), mode 5 Huffman,
     mode 6 default-with-sparse-exceptions, mode 7 pair-rANS. `ANVIL_STREAM_SLACK`
     env trades a small % of rate for decode speed (0 = strict smallest).

### Measured highlights (Linux, directional)

- **Context-switched rANS / context-distribution clustering (current Linux
  frontier, anvil_frontier tree)** — the answer to the literal-model
  fan-out penalty. The old unconditional-o1 rejection was about PHYSICAL
  MODEL MULTIPLICITY (hundreds of cold adaptive models), not the information
  signal: predecessor-byte conditioning is real, and only a small subset of
  contexts carries most of the gain. Formulation: cluster the 257
  decoder-visible predecessor-byte contexts into a small learned set (Pareto
  knee 8–12 classes) of probability classes; keep ONE physical literal rANS
  stream; the already-reconstructed previous byte selects the class/table at
  decode; context identity costs zero bits per literal (encoder mirrors in
  reverse). Results on SQLite's 268,873 literal bytes: K=12 ≈ 140,990 B total
  (~141.6–141.8 KB with models) vs ~205 KB zero-order — essentially matching
  the 32-context model while the physical coder jumps ~60→200 MB/s encode and
  ~250→300–320 MB/s decode. K=20 ~140.4 KB, K=32 ~139.3 KB (extra cost not
  worth it). The 0.4 s hard-EM learner was the blocker; sparse-support hard
  assignment with precomputed per-class log-cost tables cut K=12 learning from
  ~408 ms → ~15 ms → ~1.65 ms (≈250x) with no measured rate loss. Full-codec
  caps: clustered literals make depth-2/3 SQLite comfortably smaller than
  brotli q4, but the linked match finder still caps full encode ~77–78 MB/s vs
  q4 ~130 MB/s — literal entropy alone does not complete the lane; combine
  with shallow/direct history (depth-1 direct temporal head is 1.4–1.8x faster
  than chain-based depth-1; chain is pure overhead when one predecessor is
  consulted) or cache-line set-associative history if shallow quality is
  insufficient. Do NOT re-burn: reciprocal-rANS (precomputed reciprocal
  multiply division replacement) was measured SLOWER than the hardware divide.
  **Refined learner + numbers (anvil_frontier, later same day):** the K=12
  learner now runs in ~14–18 ms (was 0.4 s) with 3 restarts — inits: sorted-
  stripe / mode-byte / conditional-entropy-stripe; EM with precomputed float
  log-cost tables, ~12 iters, sparse-support over count[c][v]>0. Restart
  quality on SQLite: nllB 145,542 → 139,320 → 137,017 (conditional-entropy-
  stripe init wins). Final K=12 bits=12: data=137,624 + meta=4,209 =
  **141,833 B total**, enc ~199 MB/s, dec ~289 MB/s (literal subsystem;
  4-way rANS64, ONE physical stream, previous byte selects class table).
  K=8 bits=12 → 146,207 B; K=16 → 141,140 B (knee 8–12). Fast model builder:
  floor(count·2^bits/n) + largest-remainder redistribution (both sum< and
  sum> directions), O(alphabet log alphabet) — this replaced the per-model
  256-symbol rescan normalization and is worth ~3.3x literal encode vs the
  weaker prototype. Set-associative EAM sweeps: eam_setassoc_sqlite bits=18-20
  K=2-8 → 418–498 KB @ up to ~1,207 MB/s decode; setassoc_context_fine JSON
  bits=18-19 K=3-7 → 234–259 KB.
  **Verdicts from full integration (do not re-derive):** (1) context-switched
  rANS is a RATIO mechanism, not the missing throughput primitive — full
  SQLite depth-2/K12 ≈ 404.7 KB, depth-3/K12 ≈ 379.3 KB vs brotli q4 ~422.1
  KB (huge ratio headroom), but the decoder falls to ~0.6–0.74 GB/s. (2) The
  "spend rate surplus on a decoder-cheaper code" branch — context-switched
  table Huffman/direct codes over the same classes — was REJECTED: ~143.8 KB
  at K=12 (same size) but context-dependent prefix machinery is SLOWER than
  clustered rANS once model/table setup is counted. (3) PRIOR-ART HONESTY:
  Brotli itself maps decoded literal context to several literal prefix trees
  with a compact context map driven by previous decoded bytes (RFC 7932) —
  context clustering is NOT ANVIL novelty. The narrow, defensible contribution
  is the ~1.65 ms sparse-support quantizer making decoder-visible context
  modeling economical inside ANVIL's rANS/semantic architecture; treat as
  enabling infrastructure unless an ablation shows a genuinely new
  interaction. (4) Keep the K≈8–12 quantizer as reusable infrastructure;
  branch the search to the ELF/source lane.
  **ELF lane:** direct depth-1 temporal map reproduces the exact ELF parse at
  ~2x chain-search speed; clustered literals (K≈12–20) cut the literal stream
  ~370–410 KB yet the whole file stays far above brotli q4/q6 — the ELF gap
  is REPRESENTATION/MATCHABILITY, not entropy. BCJ-style x86 CALL/JMP
  normalization (control, not novelty) improves the ELF payload ~117–130 KB
  at ~1.6 GB/s transform and makes literal contexts more predictable, but
  BCJ + clustered literals still stays above q1/q4. Next: ELF section-layout
  analysis to decide between a generalized transformed-reference primitive
  vs a field/record structure detector.
  **ELF section breakdown (localizes the failure):** .text (~3.26 MB) is the
  main gap — ANVIL exact ~1.858 MB vs brotli q4 ~1.781 MB / q6 ~1.585 MB;
  .rodata (~2.25 MB) second — ~773 KB vs q4 ~699 KB; .eh_frame also has a
  meaningful gap; .data and .PyRuntime are small and already highly
  compressible. Approximate self-reference does NOT fix these sections (on
  .text it is worse unless expensive global history is allowed). Verdict: the
  next frontier is cheap REVERSIBLE STRUCTURE EXPOSURE before matching/coding
  — position-dependent code and fixed-width binary fields — with lane/field
  transposition as a generic control and broader executable normalization as
  the deeper follow-up. (Working hypothesis for a future primitive; not yet
  implemented on the Windows line.)
  **Lane-transpose control REJECTED (decisive):** global lane/field
  transposition improves byte-lane entropy but destroys the contiguous phrase
  structure ANVIL references exploit — every tested ELF section grew.
  Informative conclusion: the transform must live INSIDE the reference, not
  globally before LZ — preserve local instruction/data layout and let a
  reference explain only the changing fields (this is precisely the
  sparse-corrected phrase copy ANVIL already has, applied to binaries).
  Next: mine ELF approximate-repeat pairs for reusable mismatch structure
  (aligned 32-bit relocation fields, common additive deltas, recurring field
  masks) before defining any transformed-copy opcode — candidate residual
  types for a field-aware patch reference (e.g. additive-delta on aligned
  32-bit fields as a residual class in the SPARSE-REF family).
  **TCOPY — transformed-copy self-reference (NEW, strongest signal of the
  iteration):** ELF mismatch anatomy: among sampled .text approximate-repeat
  candidates (8-byte anchor match, ≤~33% byte mismatch over ≤256 B), ~81%
  contain ≥2 32-bit fields whose destination differs from source by exactly
  −distance — the algebra of PC/RIP-relative relocation when the same
  instruction template appears at a different file position; those fields
  explain ~46% of mismatch bytes. A single repeated 32-bit additive delta
  explains ~59% of mismatch bytes (.text), ~49% (.eh_frame), ~61% (.rodata).
  Proposed reference family: TCOPY(d,L,Δ,M,R) — copy a prior phrase, add a
  common 32-bit delta at sparse field offsets M, then apply sparse residual
  bytes R. For executable-relative fields Δ=−d is IMPLICIT (zero bits for the
  transform parameter). Ordinary LZ = special case M=R=∅. Prototype plan:
  isolated enabling-primitive ablation (not premature integration);
  falsifiable question — can phrase-level transformed self-reference explain
  ELF .text mismatches materially better than exact LZ at decoder cost ≈ copy
  + sparse 32-bit adds + sparse stores? First prototype EXCLUDES overlapping
  TCOPY refs (dist<len) so semantics stay unambiguous; transform fields and
  residual bytes get SEPARATE statistical domains so the gain is attributable
  to the transformed reference itself, not a better entropy coder. If it
  gains density, overlap/periodic transformed references are the later
  extension. (Strong novelty candidate — pre-register at the gate.)
  **TCOPY first ablation (Linux, .text, directional):** round-trip passed
  after fixing a genuine search-index bug (hash-bucket collision mistaken for
  a verified 8-byte anchor — caught before any number was counted). At the
  same search depth/backend: experimental exact stream 1,962,808 → 1,889,719 B
  (~73 KB / 3.7%) using ~5,070 transformed phrases with ~16,500 implicit
  relocation-field corrections. Still does not beat brotli q4 in the crude
  wire, but the existing EAM .text representation was only ~77 KB behind q4 —
  the transform gain is approximately the right magnitude to close that lane
  once integrated with the stronger syntax coder. Encoder search is
  intentionally awful (single-digit MB/s at deeper probes) — representation
  value is now separated from discovery cost; next: check .eh_frame/.rodata
  generalization, then attack near-LZ-cost discovery.
  **TCOPY section boundary (clean):** .text — implicit Δ=−distance TCOPY is
  useful. .eh_frame — ZERO TCOPY phrases survive. .rodata — essentially zero
  useful implicit-relocation phrases survive. This matches the anatomy:
  executable code has a displacement-derived transform; .eh_frame/.rodata
  showed common additive deltas NOT tied to reference distance. Generalization
  (smallest evidence-backed family): TCOPY(d,L,Δ,M,R) with TWO submodes —
  (a) implicit Δ=−d (zero bits), (b) one transmitted 32-bit additive Δ, which
  survives only when multiple fields amortize transmitting Δ. No "arbitrary
  transforms".
  **Transmitted-Δ submode REJECTED (useful falsification):** despite the
  anatomy showing repeated 32-bit deltas in .eh_frame/.rodata, actually
  transmitting a general Δ and letting the greedy parser use it made BOTH
  sections larger, and discovery became extremely expensive. The
  sliding-window correlation was real but did not translate into an
  economical phrase representation. TCOPY is narrowed to the scientifically
  stronger case: IMPLICIT displacement-derived transformation in executable
  code only (Δ=−d, zero parameter bits, clear causal interpretation). Next:
  couple implicit TCOPY to the shape/displacement instruction representation
  (  the crude 10-stream prototype leaves ~100 KB of syntax efficiency vs EAM).
  **TCOPY milestones (Linux, .text, directional):** (1) Block-local
  reset (64–256 KiB) makes .text WORSE — useful transformed phrases are not
  local; TCOPY wants a longer temporal horizon (do not bolt on Brotli-style
  block locality). (2) Correction-stream ablation: TCOPY CANNOT be bolted
  onto the existing sparse-reference wire — the old approximate matcher finds
  <0.6% of correction bytes reinterpretable as implicit −distance fields; the
  capability must live inside candidate verification/search, not just
  serialization (enabling-technology case: representation useful, old matcher
  can't see it). (3) Fast wordwise verifier (skip equal 64-bit spans, examine
  first differing byte — RCM-analogous): ONE temporal candidate already finds
  ~4,200 transformed phrases at ~38–40 MB/s parse; deeper chains buy
  diminishing density at large cost. (4) Confound fixed: control parser only
  recognized exact matches ≥8 B while production ANVIL uses 4–7 B phrases —
  short-match sidecar added so the A/B no longer penalizes TCOPY's exact
  representation. (5) DENSITY LEG CROSSED: strongest .text point = ANVIL
  TCOPY prototype **1,761,776 B vs brotli q4 1,781,130 B (−1.1%)**; encode
  ~16 MB/s vs ~60, decode ~208 vs ~267 — still not a win, but density beats
  q4 on .text. Next optimization (quantitative): most decoder work is
  hundreds of thousands of tiny 4–7 B exact tokens from the sidecar; with
  ~19 KB of q4 rate headroom, sweep the minimum exact-match length to spend
  surplus on fewer decoder instructions/search updates.
- **PNRA — Position-Normalized Relocation Anchoring / Transformation-Invariant
  Temporal Anchoring (NEW, strongest mechanism-level advance; Linux .text,
  directional)** — the idea that changes generalized-reference discovery:
  for a TCOPY field with v_dst = v_src − (p−q), the quantity v + absolute
  field position is INVARIANT. So hash fields into a representation already
  invariant to the transform being discovered — no approximate-nearest-
  neighbor byte matching needed. Prototypes (prototypes/pnra/ in this repo):
  - Single-anchor PNRA: raw-history TCOPY 1,766,167 B → PNRA-only 1,741,092 B
    → combined 1,730,689 B vs brotli q4 1,781,130 B (combined ~50 KB below
    q4). Naive version too expensive (single normalized CALL target
    insufficiently selective — common callees → huge candidate populations).
  - Two-anchor invariant (K1, K2, Δf) with K_i = v_i + position(v_i) and Δf
    the spacing between transformed fields: far more selective; empirically
    2,862/4,583 transformed phrases contain ≥2 E8/E9 fields. Pair-index:
    1,755,244 B @ ~29.5 MB/s parser / ~279 MB/s decoder, only ~186k
    expensive verifications; aggressive one-pair config ~1,769,263 B @ ~34.6
    MB/s parser.
  - EVENT-DRIVEN PNRA (invert the computation: structural event → historical
    invariant match → candidate synthesis → O(1) parser lookup): only
    ~115,490 relevant relocation-pair events in 3.26 MB .text vs ~1.4M
    parser decision positions. Measured: 13,752 matching pair signatures,
    13,752 verifications, 5,581 precomputed candidates, 2,751 selected
    TCOPYs; candidate generation ~474 MB/s. Full experiment: 1,794,886 B vs
    q4 1,781,130 B (gives back ~13.8 KB; encode ~39.3 vs ~60 MB/s; decode
    ~270 vs ~267 MB/s) — does not beat q4 YET, but transformed-search is no
    longer the dominant encoder bottleneck; ordinary exact-match
    history/parsing is now the expensive component.
  - **Formulation: Transformation-Invariant Temporal Anchoring.** Derive
    I(x,p) such that I(T(x,θ), p') = I(x,p) for the relevant transform;
    build the temporal dictionary over I, not raw bytes. Changes discovery
    from "candidate generation → expensive approximate verification →
    discover transformation" into "transformation invariant → exact hash
    lookup → cheap verification". Ordinary LZ = identity-transform special
    case. Broader program: derive cheap invariants for useful transformation
    families and index equivalence classes of generative explanations.
  - Next frontier (the asymmetry): make exact history event/admission-driven
    too, or derive raw-match invariants so ANVIL stops indexing every byte —
    expensive temporal state only for information with demonstrated
    predictive value.
  - Gate note: mechanism-level novelty candidate (new search formulation —
    invariant-based indexing). Pre-register at the gate; the C1/TCOPY
    separators now include "transformation-invariant anchoring" as an
    enabling primitive.
  - NEWEST BOUNDARY + STRATEGY (Linux, .text): (a) Decoupled search
    experiment — PNRA + a small raw-matchability cache restores
    transformed-reference density but does NOT improve encode enough (best
    tested gate still ~28–34 MB/s end-to-end, only approaching q4 density);
    ordinary exact-history search is the residual encoder cost. (b)
    RE-TARGET: brotli q6 encodes .text at only ~18.7 MB/s — ANVIL's
    transformed search is already faster than that. The favorable comparison
    is q6-class density at ~300 MB/s decode, not chasing q4-class encode.
    Combined rate budget under study: TCOPY/PNRA + executable normalization
    + ONE physical context-switched literal coder (single context-switched
    stream, NOT multi-stream fan-out — the rejected fan-out stays rejected).
    (c) Reduced exact-depth test: one/two-candidate exact table + PNRA may
    preserve q4-class density; K=1/K=2 still allocate four positions per
    hash bucket (hardwired bucket type — implementation-era artifact);
    specialize the temporal table width PHYSICALLY (K=1 → ~1 MB direct table
    instead of ~4 MB four-slot) — no candidate-decision change, pure cache
    economics test. (d) The event-driven inversion stands as the algorithmic
    step: search work proportional to structural events (~115k relocation
    pairs in 3.26 MB .text) rather than input length (~1.4M parser
    positions);     parser does O(1) candidate lookup at phrase starts.
- **HOT-OP HYBRID + STREAM BUDGET (Linux log corpus, directional — the
  closest thing to a three-axis crossing yet):** (1) WRONG FORMULATION
  exposed: a fully concrete opcode book reaches ~935 MB/s decode but bloats
  the log to ~652 KB (absolute-distance specialization destroys the
  excellent shape-conditioned distance coding) — Pareto rejection, not a
  hot-op failure. (2) CORRECTED INTERACTION: hot concrete instructions
  COEXIST with the per-shape displacement state; only commands whose
  concrete semantics save enough metadata get compiled; the long tail keeps
  shape-predict coding; hot commands still update the same shape state so
  future distance deltas stay cheap. Log 466,579 B → ~452 KB. (3) A
  self-inflicted O(127) linear shape lookup on every hot instruction was the
  remaining decoder cost — fix: compile the shape-state index directly into
  each hot-book entry → constant-time opcode → semantics → state update →
  copy/patch. (4) First corrected run: 80–112-entry hybrid ~0.87 GB/s decode
  vs same-run q9 ~0.84 GB/s at ~452 KB vs 513 KB — potentially the first
  three-axis crossing, but timing moved with CPU state (not called yet).
  (5) PAIRED FULL-CODEC TEST (incl. parse + book synthesis + entropy
  encode): encode ~33 MB/s vs q9 ~19.6 MB/s, winning ALL 61 paired encode
  trials, at 452,548 B vs 512,901 B. Decode is the only uncertain axis
  (allocator + stream-materialization details matter). (6) STREAM-BUDGET
  SWEEP: the dominant trade is storing the SHAPE-DISTANCE-DELTA stream raw —
  costs only ~19.8 KB (ANVIL at 472,356 B, still ~7.9% smaller than q9) and
  raises hot-book decode to ~0.99 GB/s (microbench). Other raw-stream choices
  buy far fewer cycles per byte. This is J-selection at the WHOLE-CODEC
  budget level, not per-stream. ~60 KB rate surplus = explicit compute
  budget.
- **Parser economics — surprise-budget sweep (current frontier)**: the
  hand-tuned local score / mismatch budget ("surprise budget", default 6)
  inherited from the generalized parser does NOT suit the shape-predict
  representation. Sweeping it under the new representation: budget 5 +
  shallower approximate-anchor probe → JSON 268,059 B at ~60 MB/s parse
  (vs 273,700 B at ~25 MB/s for budget 6 — smaller AND ~2.4x faster);
  budget 3 → 256,713 B, only ~3.5% above brotli q6 (248,087 B). The budget
  is an entropy-control variable; optimum is below the historical default.
  Extend sweep below 3 + tune min approximate-reference length. **Action for
  the Windows line: expose and sweep the parser mismatch budget; do not keep
  hand-tuned defaults.**
- **FAILED THE PARETO GATE (ablation only, do not re-burn)**: (a) static
  per-shape displacement manifolds — faster than the dynamic last-distance
  predictor but give back too much ratio; (b) learned (shape, Δdistance)
  delta-superinstructions compiled into the opcode vocabulary — top-127 pairs
  cover ~26% of hot refs (JSON) / ~47% (logs), yet they recover only a little
  decode speed while giving back enough compression to fail the gate. The
  dynamic per-shape last-displacement predictor stays the baseline.
- **Shape-frontier matched table (generated.json, 827,664 B)** — the
  reference for the next prototype decisions (Linux EPYC, clang, median reps):
  | codec | bytes | ratio | enc MB/s | dec MB/s |
  |---|---|---|---|---|
  | anvil shape-abs (semantic shape book, absolute dist) | 315,221 | 0.1204 | 36.8 | 1099 |
  | anvil shape-predict (per-shape last-dist delta) | 273,700 | 0.1046 | 36.4 | 957 |
  | brotli q1 | 354,400 | 0.1354 | 471.7 | 597.6 |
  | brotli q4 | 327,787 | 0.1252 | 86.6 | 1411.8 |
  | brotli q6 | 248,087 | 0.0948 | 47.6 | 1218.5 |
  | brotli q9 | 234,876 | 0.0897 | 22.0 | 1210.9 |
  Verdict: shape-predict beats brotli q4 ratio at ~1 GB/s decode and ~36
  MB/s encode; still DOMINATED by q6/q9 on ratio and by q4+ on decode. The
  ratio/runtime jump vs the earlier generalized stream (320.9 KB @ ~0.37
  GB/s) came from factoring semantic shape (kind, len, patch topology) from
  displacement and predicting displacement per shape. **Workflow rule:
  brotli q11 dropped from inner iteration loops (it dominates runtime); use
  q1/q4/q6/q9 for prototypes, q11 only for final validation.**
- generated.log (1,924,280 B): SCM+s6+macro mode 23 → **0.056 ratio, 47.4
  encode, 913 decode** (brotli q9 0.065, 27.6 enc, 1,460 dec). Raw-hot +
  flat-decoder experiment reached 81,208 B (0.042) / 50 enc / 963 dec.
- generated.log stream anatomy: 22 entropy streams, dominated by
  pair-rANS/Huffman on the big residual streams; distance_code 15,680 B and
  match_len_class 8,788 B were the largest rANS streams.
- generated.json: SRR + shape-conditioned displacement (below) → **109,700 B
  (0.1325) @ ~541 MB/s decode** vs brotli q9 113,284 B (0.137) — ratio beat,
  decode ~0.5 GB/s vs ~1.0 GB/s (still not Pareto; encode also slower).
- Slot-default analysis (generated.log): 128 recurring patch masks, 598
  (mask,slot) contexts, modal residual accuracy 86.5% → a per-slot default
  residual + exception mask is a strong coding target (R2 topology coding).
- Distance analysis (generated.json): distance is NOT well predicted by
  recency (top-16 MRU ≈ 11-20%); shape-conditioned displacement works —
  top-1020 shapes, ~6.4% exact, 44.6% within 64, log-proxy ≈ 9.4 bits vs ~13
  bits unconditional. LZ source positions align with prior token boundaries:
  66-92% of references start within ±8 B of a prior token start.

### Next iteration targets (validated, in priority order)

1. **Shape-conditioned displacement prediction P(d|s)**: hot semantic opcode
   selects a tiny per-shape displacement state; first occurrence = absolute,
   later = signed delta from shape's last displacement (zigzag + class/extra).
   Implemented on Linux: JSON 112,941 → 109,700 B, decode 534 → 541 MB/s.
   Ablate per-shape vs global recency; keep only if end-to-end bytes improve.
2. **Precision/work-adaptive entropy**: 4096-state rANS is overkill for small
   streams; a 256/512-state variant with smaller cache-resident tables plus
   the stream suite (Huffman/exception/pair/raw) makes rich multi-model coding
   cheap to initialize/execute. Cost: J = L_stream + λC_decode + μC_model-build
   + νW_cache. This benefits every stream simultaneously.
3. **Mutation-template / slot-default residual coding (R2)**: per-(mask,slot)
   modal residual as decoder-visible default + exception mask; JSON log-proxy
   indicates ~0.30-0.45 exception fraction → strong ratio, near-zero decode
   cost.
4. **Boundary-aligned candidate generation**: since LZ sources align to prior
   token starts ±8, generate candidates from token-boundary-indexed positions
   (cheap, raises hit rate).
5. **Difference-cover negative gate**: adopt the mod-64 cyclic cover to skip
   expensive search on incompressible blocks (huge encode win; no ratio cost).

### Known traps (do not re-burn time)

- Unconditional order-1 literals: rejected (+3.46%). xor/delta literal
  transforms: router-only, no novelty claim. Fused on-demand decoder cursors
  (all-streams-live): failed, single fused decode table per stream is better.
  `parse=auto` brute-force: research-only, never the production default.
- MTR (mutational-template backend): compiled but not yet measured cleanly
  (build fixes pending) — slot-default analysis above is the cheaper path to
  the same idea.
- Many Linux experiments were one-off builds (anvil_*.pre-*, bench_*);
  keep the Windows tree a single evolving anvil.cpp with mode numbers, and
  record drop reasons in the ledger.

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
