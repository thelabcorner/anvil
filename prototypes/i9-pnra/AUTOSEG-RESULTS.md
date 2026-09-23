# AUTOSEG-RESULTS — step-region discovery without supplied run boundaries (pnra, I9 follow-up)

**Task `task_5ebbd7e0d89c41bd8c55e4cccdd76800`. Status: COMPLETE — byte claims
independently verified by `research-gate` (ledger PART XIV addendum A5); timing
measured by `bench` co-run (ranking-grade, §7). Adopt-class engineering
`{engineering}`, NO NOVELTY CLAIM. Prototype
wire, not an ANVIL row. No crossing language anywhere.**

Question being settled: the PR-5 wire (12,936 B on synth-arith) used a segmenter
that discovers run boundaries from the data; this task verifies that explicitly,
removes any supplied structure, charges every discovery parameter, and measures
where the segmenter succeeds and where it fails.

Artifacts: `autoseg.cpp` / `autoseg.exe` (new study tool; `stride_ref.cpp`/`.exe`
remain FROZEN as the PR-5 artifact), `autoseg-results.csv` (all rows below),
`timing-autoseg.ps1` + `sampler-autoseg.ps1` (kept for reproduction; the
attested run was executed by `bench` under `w-bench-pnra-autoseg-20260912T091754Z`),
this document.

---

## 1. Segmenter (data-only, no oracle)

`autoseg` copies the frozen container/decoder and adds the discovery study:

- **Rule (auto):** a *step region* starts at word `i` and extends maximally while
  the running min/max of first differences stays within a width-2 window
  (`max-min ≤ 2`, i.e. all deltas in a 3-consecutive-integer set). A span is
  accepted only if `span ≥ min-step` (default 8 words); everything else becomes
  *literal* regions. Regions tile the file contiguously.
- **Transmitted (charged) discovery state:** region length + type per region
  (`--table=compact`: no gap, no startup count — the decoder's own state machine
  derives the step from decoded deltas; see PR-5 README §2a). The v1 container
  remains available for byte-comparison with PR-5.
- **Oracle arm (`--force-lens=`)**: fixed true run lengths forced; used only as
  the comparison target, never as an input to the auto arm.
- **`stats` mode**: prints the discovered region table and compares discovered
  boundaries against a truth list (`--truth-lens=`).

No supplied anatomy is used by the auto arm: the boundary positions, region
count, lengths, per-region step and base are all found from the byte stream.

## 2. Headline: synth-arith (the PR-5 file) `{synthetic}`

| arm | regions | wire B | ratio | roundtrip |
|---|---|---|---|---|
| auto, v1 container (same as PR-5) | 8 (all step) | **12,936** | 0.050531 | PASS |
| auto, compact table | 8 (all step) | **12,920** | 0.050469 | PASS |
| oracle forced true lens, compact | 8 (all step) | 12,920 | 0.050469 | PASS |
| literal-only control (compact) | 1 literal | 256,048 | 1.000188 | PASS |

- **Boundary recovery: 8/8 regions, 7/7 truth boundaries exact, 0 missing,
  0 spurious, 0 split runs.** Discovered spans are exactly the 8×8,000-word
  runs; per-region delta widths are all 2 with exactly 3 distinct deltas
  (the generator's `stride + randint(-1,1)` signature).
- **Auto v1 output is byte-identical to the frozen `stride_ref.exe` artifact**
  (sha256 `0EAFB385B7E4F0C58C10088EE5311E84E5DA4E4D7A2471AA64BA4813F4B2F0E1`).
- **Discovery cost charged:** 24 B compact (0.19% of wire) / 40 B v1 (0.31%).
  A zero-table (impossible) bound is 12,896 B; the discovery cost is therefore
  **+24 B over an oracle that pays no table at all**.
- **Factor vs the oracle 12,936 B:** auto compact = **0.9988×** (slightly
  smaller, because the compact table also shrinks parameters); identical
  container = **1.0000×**.

Section accounting (compact): `44 + 32 + 24 + 13 + 16 + 12,791 = 12,920` —
byte-exact, every discovery parameter counted.

## 3. Randomized anatomies — no prior knowledge of K, lengths, strides, bases

Ten files generated in-lane (`var_00..09`, `var_manifest.txt`): K ∈ {8,16,32}
random runs, lengths 300–5,000, strides 1–200, random bases, `±1` jitter.

| check | result |
|---|---|
| files with exact boundary recovery | **10 / 10** |
| truth boundaries recovered exactly | **166 / 166** |
| missing / spurious / split runs | **0 / 0 / 0** |
| wire factor auto vs forced-truth-oracle | **1.0000 on every file** |

Auto wire equals the forced-oracle wire byte-for-byte on all ten files: in this
generative class, discovery is free *modulo the charged region table* (~3 B per
region compact; e.g. var_03: 31 boundaries, oracle == auto == 17,339 B).

**Merges are correct, not failures.** `var_seamless.bin` declares 6 generator
runs but the data is one homogeneous sequence (no boundary in the values):
the segmenter discovers **1 region**, missing all 5 declared boundaries, and its
wire is **881 B vs 923 B** for the forced-declared arm (−4.6%). The segmenter
finds structure in the data, not the generator's intent.

**Outliers are cheap and correctly placed.** Injecting K isolated large deltas
into a 20,000-word run: K=0 → 1 region, 4,055 B; K=5 → 6 regions, 4,099 B
(+8.8 B/split); K=20 → 20 step regions (+1 one-word literal), 4,223 B
(+8.4 B/split). Each split costs the table entry plus the new region's first-word
literal; no cascading mis-segmentation.

## 4. Failure modes (honest bounds)

### 4a. No fixed-grid homogeneous runs → clean fallback, no gain
`synth-drift-stride.bin` (615,376 B, drifting record stride), `generated.log`
(1,942,280 B, repo smoke corpus), `synth-counters.log` (1,107,326 B), `random.bin`
(control): the segmenter finds **zero step regions ≥6 words** and falls back to
one literal region — wire = raw + **48 B** (44 header + 4 region entry):

| file | raw B | auto compact wire | overhead | brotli-q11 (context) | mechanism factor |
|---|---|---|---|---|---|
| synth-drift-stride.bin | 615,376 | 615,424 | +0.0078% | 100,948 (local) | 6.10× larger |
| generated.log | 1,942,280 | 1,942,328 | +0.0025% | 81,946 (committed suite) | 23.7× larger |
| synth-counters.log | 1,107,326 | 1,107,374 | +0.0043% | 100,811 (local) | 11.0× larger |
| random.bin | 262,144 | 262,192 | +0.018% | 262,149 (q1) | store-class |

Failure mode: these files carry counters/timestamps as *formatted text* or at
*drifting* record offsets — there is no fixed 4-byte grid on which a constant
step region can form. The u32 interpretation is the mechanism's native domain;
the data is out of class. The fallback is safe (no regression beyond 48 B) but
carries no compression.

### 4b. Windows exist only at noise scale (drift-stride min-step curve)
| min-step | regions | step words | wire B | vs literal |
|---|---|---|---|---|
| 2 | 76,557 | 153,844 | 782,386 | **+27.1%** (table ~230 KB ≫ savings) |
| 3 | 1,645 | 2,610 | 615,347 | −77 B |
| 4 | 225 | 480 | **614,977** (best) | **−447 B (−0.073%)** |
| 5 | 65 | 160 | 615,217 | −207 B |
| 8 (default) | 1 | 0 | 615,424 | literal fallback |

The curve has a real optimum (min-step 4) but the best effect is **noise-class
(−0.073%)** and comes from accidental short windows, not structure; at min-step 2
over-segmentation is catastrophic (+27%). There is no scale at which this file
becomes a step-region target.

### 4c. Jitter outside ±1 → representation does not cover the data
`var_jitter2.bin` (16,000 words, increments `stride + randint(-2,2)`, width 4):
**915 regions** (473 step, 442 literal), step coverage only 4,486/16,000 words
(28.0%), wire **51,656 B** (ratio 0.807, PASS) vs a width-5 entropy floor of
4,644 B (derived) — a ~11× fragmentation loss. Forcing the true runs **aborts**:
`derived startup rule violated (lo=82 hi=82 d=86)`. The width-2 window is an
exact-representation assumption; where it fails, the mechanism fragments rather
than silently degrading.

## 5. Discovery-cost accounting and the decoder-derivable alternative

- Transmitted discovery state (compact) = `len varint + type byte` per region
  ≈ 3 B/region; measured: 24 B for 8 regions (synth-arith), 3 B/region on the
  variants. This is the entire discovery cost; everything else is payload.
- Alternative: derive boundaries at decode time via an escape symbol in the main
  stream. Cost (derived): the residual alphabet becomes 4-ary (2.0 bits/symbol at
  4 symbols/byte) instead of 3-ary (1.6 bits/symbol) → +0.4 bits × 63,953 symbols
  = +3,198 B, versus **24 B** for the transmitted table. **Transmitting the
  discovered boundaries wins by ~133×**, so the "zero-bit discovery" framing is
  strictly worse engineering even where it is possible.
- Zero-table bound (not implementable, shown for scale): 12,896 B on synth-arith.

## 6. Success statement

**Independent verification (research-gate, ledger PART XIV addendum A5):** all
four byte claims reproduce — auto v1 sha `0EAFB385...` equals the documented and
live `stride_ref` encode; compact 12,920 B vs oracle 12,936 B; both decode
byte-exact; stats 7/7 boundaries; randomized 10/10 files and 166/166 boundaries
with auto wire hash-identical to forced-oracle on all ten; failure bounds exact
(raw+48 B); escape cost 3,197.7–3,198 B vs 24 B. Verdict: adopt-class
`{engineering}` byte-acceptance; the PR-5 wire is not oracle-dependent in class;
no novelty; no crossing. The gate sharpens the class boundary: fixed-grid
generative only, with generated.jsonl/sqlite-class data out of class.

- **Success (in class):** on synth-arith the auto segmenter recovers the true
  anatomy exactly and produces **12,920 B compact (0.9988× the 12,936 B oracle;
  1.0000× in the identical container) with all discovery cost charged (24 B)**.
  On 10/10 randomized anatomies it matches the forced-oracle wire exactly
  (factor 1.0000; 166/166 boundaries).
- **Failure (out of class, honest bound):** on drift-stride / logs / random the
  mechanism finds nothing above the noise scale and falls back to store + 48 B;
  on ±2 jitter it fragments (915 regions, 11× the width-5 floor) and the exact
  forced form aborts. The mechanism is **not general**; it is a fixed-grid
  arithmetic-run codec that degrades safely outside its class.
- **Verdict on the PR-5 wire:** it was **not oracle-boundary-dependent**. The
  boundaries were discovered from data by this same rule and transmitted as part
  of the counted wire. Removing the supplied structure changes the wire by
  −16 B (table compaction) and nothing else.

## 7. Timing (bench co-run, PR-4 v1.2) — RANKING-GRADE only

Window `w-bench-pnra-autoseg-20260912T091754Z` (2026-09-12T09:17:57Z–09:18:02Z),
single-thread, pinned logical core 19 HIGH, `autoseg.exe` sha256
`3518AA83BE0D3E3DAA6CA5785586E0866B1AB88286E4A49C60F0FAFB7038768D` (matches the
claimed binary), command set exactly `bench <file> --backend=0 --param=1
--table=compact`, all rc=0. Label:
**`measured (parallel window; ranking-grade)` — NOT citation-grade.**

| file | reps | dec med ms | min / max ms | CV% | dec MB/s | wire B | roundtrip |
|---|---|---|---|---|---|---|---|
| synth-arith.bin | 801 | 0.5683 | 0.5679 / 0.8768 | 11.05 | 450.5 | 12,920 | PASS |
| synth-drift-stride.bin | 501 | 1.0421 | 1.0405 / 1.6132 | 10.46 | 590.5 | 615,424 | PASS |
| generated.log | 201 | 4.0376 | 3.8871 / 4.8601 | 4.22 | 481.1 | 1,942,328 | PASS |

Caveats stated by bench: pre-window pinned-core utilisation 14.16% **fails the
<5% core gate**, and the 250 ms sampler job returned no rows, so during-window
load is not captured (ambient total CPU ≈ 33–42%). Effect direction is far
beyond the arm CV, but **absolute MB/s must not be cited as citation-grade**;
per contract v1.2 this is ranking context only. No crossing language follows
from it. (The drift-stride and generated.log rows decode the literal-fallback
path — memcpy + container — not the step main stream.)

## 8. Reproduce

```
. .\env.ps1
cd prototypes\i9-pnra
clang-cl /O2 /EHsc /std:c++20 /MD /nologo /DNDEBUG ^
  /I ..\..\third_party\install\include /Feautoseg.exe autoseg.cpp ^
  /link /LIBPATH:..\..\third_party\install\lib ^
  brotlienc.lib brotlidec.lib brotlicommon.lib
.\autoseg.exe selftest
.\autoseg.exe stats ..\..\tests\corpus\synth-arith.bin --truth-lens=8000,8000,8000,8000,8000,8000,8000,8000
.\autoseg.exe c   ..\..\tests\corpus\synth-arith.bin x.bin --backend=0 --param=1 --table=compact
.\autoseg.exe d   x.bin y.bin
```

## 9. Labels and handoff

`{engineering}` / adopt-class; `{synthetic}` on synth-arith, drift-stride and
`var_*`; `{corpus-generated}` on generated.log (`tests/make_smoke_corpus.py`) and
synth-counters.log. No novelty claim; no crossing language; byte counts
deterministic; timing is bench co-run ranking-grade per §7.

**Out-of-class cross-reference (measured by `datastruct`, peer lane):** the
record-period/columnar family does *not* absorb these files either on
non-synthetic data — `deliverable/task_c1dabc9354c44b5abc56dcc4da510fe8` reports
generated.jsonl auto-discovery at 1,199,898 B vs brotli-q6 206,840 (5.8×) and
generated.sqlite at 1,359,687 vs 252,591 (5.4×), cause: phase instability.
So the honest statement is symmetric: the step mechanism is exact and free for
its generative class and inert outside it; the record-period mechanism is
successful on fixed-period synthetic files and an honest bound on the real ones.
Neither generalizes to the counter-bearing text/soaked files yet.
