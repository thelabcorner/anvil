# ANVIL — Iteration 8 Shared Swarm Brief (Pareto offensive)

Author: coordinator. Date: 2026-09-04. Host: Windows 11 x64, AMD Ryzen 9 5900X
(12c/24t), 128 GB, clang-cl 22.1.8, CMake 4.4.2 + Ninja 1.13.2 (`tests/host-spec.md`).

**Mission: produce the first DECODE-PLANE (or both-plane) Pareto crossing, and
then make it systematic.**

> **CORRECTION (2026-09-04, coordinator, verified three ways — see
> `docs/i8-streak-correction.md` and blackboard `context/i8-streak-correction`).**
> An earlier version of this brief said "0 EXTENDS_FRONT in project history."
> **That is false. There are 5, committed at HEAD** (generated.json, ENCODE plane:
> anvil-mdl-rans 0.108/1.06, -l0 0.108/1.3, -l001 0.108/1.329, anvil-shape-rans
> 0.112/1.004, -l0 0.112/0.979; first appearance in 7b999c6). They are real per
> our sole arbiter, but they are a **front-knee artifact**: the reference encode
> front on that file is q11 (0.096, 0.674) -> zstd-19 (0.113, 1.963), so any point
> with ratio in (0.096, 0.113) and speed in (0.674, 1.963) is non-dominated
> automatically. All five are DOMINATED on the decode plane (113-180 vs q11's 507
> MB/s). 463 of 468 anvil row-plane cells remain dominated (499 of 504 including
> AGGREGATE). **The mission — a
> decode-plane or both-plane crossing at competitive ratio — is unchanged and
> unmet.** The error was propagating a tally from a prior synthesis without
> recomputing it; the binding rule is now that any cumulative number must be
> recomputed from source at the moment of writing, with the command shown.

This iteration the arbiter MUST run — Iteration 7 landed implementation and
controls but `bench` never executed, so no verdict existed. That failure is
explicitly forbidden to recur.

---

## 0. Environment (run this EVERY session, first thing)

```powershell
. .\env.ps1                 # clang, cmake, ninja + MSVC/SDK env
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_SUPPRESS_REGENERATION=ON
ninja -C build              # targets: anvil (CLI), anvil_bench (Brotli/Zstd comparison)
```

- Bench one file: `build\anvil_bench.exe <file> <reps>` (reps >= 3, median).
- Bench corpus: `python tools\bench_suite.py <files...> --bench build\anvil_bench.exe --reps 3 --out <out.csv>`
- Fuzz: `python tests\fuzz.py --exe build\anvil.exe --cases 50`
- Pareto arbiter: `tools/pareto_front.py` (the ONLY thing that may issue EXTENDS_FRONT).
- third_party/ (brotli + zstd static libs) is git-ignored; if missing run `tools\setup_third_party.ps1`.

## 1. FROZEN BASELINE — the gap we are attacking

Source: `tests/benchmark-suite.csv` (13 files x many codecs, median-3) and
`tests/benchmark-summary.csv`. Format: file / input bytes / best-ANVIL-ratio row
(bytes, enc MB/s, dec MB/s) vs best-reference row / required byte reduction to
reach the reference front.

| file | input | best ANVIL (bytes, enc, dec) | best reference (bytes, enc, dec) | needed |
|---|---|---|---|---|
| generated.json | 827,664 | anvil-mdl-rans **89,589** (1.06, 112.8) | brotli-q11 **79,172** (0.67, 507.4) | **-11.6%** |
| generated.jsonl | 2,815,267 | anvil-mdl-rans **183,506** (1.24, 173.2) | brotli-q11 **150,619** (0.54, 867.2) | **-18.0%** |
| generated.log | 1,942,280 | anvil-shape-rans **129,739** (1.22, 203.0) | brotli-q11 **81,946** (0.51, 947.5) | **-36.8%** |
| generated.sqlite | 1,740,800 | anvil-mdl-rans **323,014** (0.99, 122.3) | brotli-q11 **185,702** (0.52, 445.2) | **-42.5%** |
| synth-arith.bin | 256,000 | anvil-greedy-arith **256,022 r=1.000** (347, 389) | brotli-q11 **87,013 r=0.340** (0.60, 187.5) | **-66.0%** |
| synth-jitter.bin | 974,920 | anvil-mdl-rans **87,412** (1.59, 165.7) | brotli-q11 **62,717** (0.69, 844.3) | **-28.3%** |
| synth-timeseries.bin | 280,000 | anvil-hotop-rlzp **140,898 r=0.503** (0.04, 158.4) | brotli-q6 **120,669 r=0.431** (18.96, 282.2) | **-14.3%** |
| src.cpp | 32,512 | anvil-dp-arith **9,483** (2.21, 26.7) | brotli-q11 **7,991** (0.91, 252.8) | **-15.7%** |
| doc.md | 2,724 | anvil-dp-arith **1,444** (3.21, 19.9) | brotli-q11 **1,047** (0.84, 121.1) | **-27.5%** |
| anvil.exe | 268,800 | anvil-mdl-rans **107,468** (1.02, 89.1) | brotli-q11 **88,322** (0.36, 245.3) | **-17.8%** |
| anvil_bench.exe | 1,929,216 | anvil-mdl-rans **830,331** (1.06, 77.1) | brotli-q11 **646,675** (0.47, 205.1) | **-22.1%** |
| generated.repeat.jsonl | 940,000 | anvil-dp-arith **873** (0.22, 234.4) | brotli-q11 **162** (50.0, 633.1) | **-81.4%** |
| random.bin | 262,144 | anvil-greedy-arith **262,166 r=1.000** (337, 321) | brotli-q1 **262,149 r=1.000** (563, 1298) | tied (control) |

**Aggregate (benchmark-summary.csv):**

| codec | ratio | enc MB/s | dec MB/s |
|---|---|---|---|
| anvil-mdl-rans | 0.198808 | 0.718 | 136.550 |
| anvil-shape-rans | 0.204434 | 0.687 | 147.360 |
| anvil-dp-rans | 0.206954 | 1.124 | 160.896 |
| anvil-hotop-rans | 0.215307 | 7.963 | 171.750 |
| anvil-greedy-rans | 0.223571 | 14.151 | 146.911 |
| brotli-q6 | 0.176318 | 55.128 | 515.585 |
| brotli-q9 | 0.173063 | 19.269 | 506.355 |
| brotli-q11 | 0.145713 | 0.573 | 450.937 |
| zstd-19 | 0.161437 | 2.137 | 1203.712 |
| zstd-9 | 0.191552 | 59.722 | 1178.502 |

**Read this correctly:** ANVIL's best-RATIO rows lose to brotli-q11 on bytes by
13-74%, AND their decode is 2-5x slower than q11's. ANVIL's best-DECODE rows
(hotop ~170-270 MB/s) have far worse ratio. Every ANVIL row is strictly
dominated. The only axis where ANVIL currently wins anything is encode vs
brotli-q11 (1.0 vs 0.57 MB/s aggregate) — at a 36% byte penalty.

## 2. MEASURED FACTS — do not re-derive, do not re-burn

- **t3 decode floor profile** (median-7 interleaved, mode-15, gen_log payload):
  crc32 **44%** of decode time (1.9 ns/B bytewise), masks+resid eager
  materialization **26%** (setup is 2.07 of 2.12 ms), token loop **11%** (opcode
  entropy pulls only 0.3%), block concat/alloc **~15%**. Consequence: wire-invisible
  CRC fix alone caps end-to-end decode lift at ~1/(1-0.44) ≈ **1.79x**.
- **C_decode calibration (ns/B):** raw bulk 0.1 / raw bytewise 2.6 / defexc 3.2 /
  Huffman 4.3 / rANS ~6.0 flat / ctx-rANS 7.5.
- **S6-1 stream budget at lambda=0.01 B/us** (`--hotop-budget=on`, mode-15): ZERO
  selection flips, size-equality 22/22, hash-differences fully attributed to
  precision transitions. Arithmetically necessary: the decode term spans 0.02-0.66 B
  on 10-20 KB streams. Retires "budget as a decode lever at lambda=0.01"; does NOT
  retire materially higher lambda (implied willingness-to-pay for the lits flip is
  ~44.6 B/us — a different cost philosophy, own pre-registration).
- **Experiment Y — RLZ/RePair book-stream codecs (modes 7/8, `--hotop-rlzp`)**:
  book bytes PASS (log -10.31%, jsonl -10.76%, json -4.15%, sqlite -0.64%,
  synth-drift-stride -28.6%, counters -19.1%, columnar-align -14.2%, PEs -0.5..-2.9%);
  decode FAIL (log -8.1%, jsonl -15.9%, json -11.0%); encode prohibitive (pe-git
  141.5 s vs 2.2 s = 64x). NOT ADOPTED AS DEFAULT. Decoder findings F1-F5 all
  fixed-with-test.
- **Mode-16 ARI-REF** pre-registered (`docs/pre-registrations/i8-ari-ref.md` slot;
  see `docs/verify-notes/i7-verification.md`): on synth-arith.bin the validated
  harness wire is **89,363 B (r=0.349)** vs brotli-q11 87,013 B — **+2.7% from
  the front**. This is the closest ANVIL artifact in project history. The end-to-end
  entropy-coded container is the open gate.
  **CORRECTION (research-gate, 2026-09-04 — see `docs/gate-verdict-i8-ari-stride.md`
  §4; claim-hygiene debt in this very line):** the "+2.7% from the front" framing
  measures against a bar that is not a frontier. brotli-q11 applied to the
  delta+zigzag-varint-transformed bytes of the same file yields **16,313 B**
  (pnra, reproduced by strategy) — **5.33x smaller** than its own raw score. The
  ARI-REF wire is therefore **5.48x above the transform-enabled ceiling**, not
  2.7% from any frontier, and the real gap on this cell is **-93.6%**, not -66%.
  87,013 B is brotli declining to apply a textbook filter it ships elsewhere.
  **Every "-66%" and "+2.7%" figure in the corpus narrative above is measured
  against that self-imposed handicap.** Any future synth-cell result MUST report
  against BOTH bars (raw 87,013 AND transform-enabled 16,313).
- **PNRA / TCOPY (transformation-invariant temporal anchoring)**: for a relocation
  field with `v_dst = v_src - (p-q)`, the quantity `v + position(v)` is INVARIANT.
  Hash the invariant, not the bytes. Implicit `Delta = -d` (zero parameter bits) is
  the ONLY surviving submode; **transmitted-Delta submode REJECTED** (made
  .eh_frame AND .rodata larger, discovery prohibitively expensive). Section
  boundary: .text useful, .eh_frame ZERO, .rodata ~zero.
- **Context-switched rANS (K=8-12)**: a RATIO mechanism, not a throughput one.
  SQLite literals 205 KB -> 141.8 KB total, but full-codec decode falls to
  0.6-0.74 GB/s. **PRIOR-ART HONESTY: Brotli already maps decoded-literal context
  to several prefix trees via a context map (RFC 7932) — context clustering is NOT
  ANVIL novelty.** Keep as enabling infrastructure only.
- **Lane/field transposition applied GLOBALLY before LZ: REJECTED (decisive).**
  Every tested ELF section grew. The transform must live INSIDE the reference, not
  before it — i.e. a transformed/patch copy that explains only the changing fields.
- **Difference-cover negative gate** (cyclic mod-64, 10 phases, content-hash
  thinning): random.bin encodes at **3,341 MB/s** vs brotli-q11 0.66 MB/s.
- **Parser "surprise budget" sweep (Linux, shape-predict representation): budget 5
  is SMALLER AND 2.4x FASTER than budget 6** (JSON 268,059 B @ 60 MB/s vs 273,700 B
  @ 25 MB/s); budget 3 -> 256,713 B. The historical default of 6 is wrong.

### Known traps (never re-burn)
Unconditional order-1 literals (+3.46% bytes, rejected). xor/delta literal
transforms (router-only, no novelty claim). Fused all-streams-live decoder cursors
(rejected — single fused decode table per stream wins). `parse=auto` brute force
(research-only, never the default). Reciprocal-rANS (measured SLOWER than hardware
divide). Static per-shape displacement manifolds (give back too much ratio).
Learned (shape, delta-distance) delta-superinstructions (top-127 pairs cover 26-47%
of hot refs, fail the gate). Context-dependent prefix/Huffman over clustered
classes (SLOWER than clustered rANS once setup is counted).

## 3. HARD WORKING AGREEMENTS

1. **ONLY `arch` edits `src/anvil.cpp`.** Everyone else prototypes under
   `prototypes/i8-<lane>/` and writes docs under `docs/`.
2. **Never change an existing block mode's wire format.** Add a new mode number and
   keep the old decoder path. `FORMAT.md` must document every new mode.
3. **No ratio claim without round-trip verification + fuzz.** No throughput claim
   without median-of->=3 via `anvil_bench`.
4. **`tools/pareto_front.py` is the sole issuer of EXTENDS_FRONT.** Nothing else
   may claim a Pareto crossing.
5. **Brotli q11 is OUT of inner iteration loops** (it dominates runtime). Use
   q1/q4/q6/q9 for prototypes; q11 only for final validation.
6. Every mechanism must clear the four-step novelty gate
   (`docs/research-agenda.md` §0): prior-art lineage / what is NEW / falsifiable
   Pareto claim / ablation. "It compresses better" is not a Pareto claim.
7. When a mechanism is dropped, record WHY in `RESEARCH_LEDGER.md` — math vs
   implementation-era.
8. **Peer-to-peer first.** Message the lane that owns a topic directly
   (`swarm_find`, `swarm_message`); do not route everything through the
   coordinator. Publish measurements to the blackboard as they land.

## 4. Key reading (in priority order)

- `docs/CONTEXT.md` — full research history, including the Linux continuation
  (v2) section. Read this before designing anything.
- `docs/research-agenda.md` — novelty gate + SPARSE-REF lineage.
- `RESEARCH_LEDGER.md` — every experiment, number, and drop reason.
- `docs/swarm-i7-strategy.md` — the Iteration-7 synthesis and the I8 plan.
- `docs/ledger-i7-cost.md` — I7 cost/ledger detail.
- `FORMAT.md` — bitstream spec; one section per block mode.
- `docs/decoder-audit.md` — decoder strictness findings.
- `tests/host-spec.md` — host + reproducible build/bench commands.
