# I20 / C1 — ANVIL BWT decode stage decomposition (results)

**Worker:** W1 (swarm) · **Iteration:** ANVIL 20 — "Pareto gate closure"
**Branch:** `research/anvil-i20-w1-baseline-stage` · **Candidate:** C1 (prereg §5.1)
**Class:** `{adopt-class}` / `{engineering}`. **No novelty claim is made or authorized.**

> **This entry was completed by the I20 coordinator from W1's committed tool and workflow
> (`2f31e94`, `70dbaa6`). W1's provider session terminated before it could publish its own
> write-up, and the measurement it was commissioned to make did not complete on the hosted
> runner.** The verdict is therefore **INCONCLUSIVE**, not NO-GO: nothing here is evidence for or
> against C1's hypothesis.

---

## 1. Hypothesis and pre-registered decision rule

**Hypothesis (pre-registered, prereg §5.1, C1):** the whole-codec decode cost of the canonical
ANVIL ratio route (BWT backend + auxiliary-index v2 payload) is dominated by the inverse-BWT LF
walk, so the auxiliary-index walk-budget lever (C2) and/or the postcoder lever (C5) are the only
fundable decode levers; if the LF-walk share is small, C2 is dead and the postcoder/entropy path
must carry the programme.

**Pre-registered decision rule, quoted verbatim from prereg §5.1:**

> **Amdahl kill:** if the LF-walk share is **>= 78%**, then C2/C5 are the only funded levers and
> C4/C3 carry the memory gate; if the LF-walk share is **<= 55%**, the LF-walk levers (C2) are
> **KILLED** and the program must target the postcoder (C5) instead.

**Corroborating preregistration in the repository:** `docs/swarm-2026-10-02/REMOTE-EXPERIMENT-QUEUE.md`
§Q4b — *"The required whole-BWT-path acceleration is about 4.5x. Infinite-speed LF walk can meet
4.5x only if non-walk share is about <=22% (LF-walk >= about 78%). If the measured non-walk share
exceeds the frozen Amdahl ceiling, kill WSI/lane-scheduler implementation before writing it."* —
with §NQ-4 adding the enforcement: *"kill the implementation, not just the estimate."*

---

## 2. What was built (MEASURED artefacts exist; the run did not complete)

| item | value |
|---|---|
| tool | `tools/i20_stage_decompose.py` (this branch) — `prepare` / `rss` / `time` / `profile` / `summarize` |
| workflow | `.github/workflows/anvil-i20-stage-decompose.yml` (this branch; registered on `main` for dispatch) |
| method | **source-free** attribution on the unmodified frozen candidate: `valgrind --tool=callgrind` (deterministic per-function instruction counts) + `valgrind --tool=massif` (peak heap per allocation site) + `gprof` (wall-clock samples on a separate `-pg` build) + `nm`/`c++filt` symbol resolution; the attribution table (symbol → self Ir → assigned stage) is written into `profile.json` so every attribution is auditable |
| candidate source | tag `i10-aux-unbwt-final-v1` = commit `0dce534e5df40945d518c9ef4a722f4a0ecfc0e6`; `src/anvil.cpp` blob `755df76ae53795aea040f4c99784d1effb43b3ab` (byte-identical to the programme base commit) |
| corpus | the canonical Silesia and enwik8 sets, fetched with `tools/gha_fetch_corpus.py` (pinned size + MD5) |
| stages separated | (a) postcoder entropy decode · (b) MTF + zero-run RLE expansion · (c) inverse BWT (`libsais_unbwt_aux` → `libsais_unbwt_main` → … → `libsais_unbwt_decode_8`) · (d) residual: framing, allocation, CRC-32, file I/O |
| stages (a)/(b) separation | a dedicated `-g -fno-inline` attribution build is used so the entropy decoder, the MTF expansion and the `std::vector` writes are not fused; the fused `bwt_arith_decode` function is then split by `anvil.cpp` source line (`decode(ad)`/`decode_uvar_m` → (a); `bwt_mtf_emit_rank`/`out.insert`/`sym[j]=sym[j-1]` → (b)) and the exact line sets are emitted in `profile.json → source_line_map` |
| stage (c) exactness | `libsais` is a separate translation unit (`third_party/libsais/src/libsais.c`) and the build uses no LTO, so no `libsais` code is ever inlined into `anvil.cpp`; every instruction executed inside `libsais.c` is attributed to (c) regardless of inlining elsewhere |

**Dispatch attempts (all completed without a usable `profile.json`):**
`39659153216` (failure), `39659202582` (failure), `39659854677` (failure), `39660334510` (failure).
The cause is an environmental/host limitation of the hosted runner for the profiling steps
(callgrind over multi-megabyte corpora and massif over the same), not a finding about the codec.

---

## 3. Verdict

**C1 — BWT decode stage decomposition: `INCONCLUSIVE`.**

**Reason, stated precisely:** the measurement the candidate was commissioned to make did not
complete on the hosted runner, and the worker's session terminated before its write-up was
published. **No stage share was measured, so neither branch of the pre-registered Amdahl kill
rule (>= 78 % or <= 55 %) can be evaluated.** This is a missing measurement, not a falsification.

**This does not leave the programme without an answer, because two other measurements in the
programme now answer the question C1 was commissioned to ask — and they do so more directly:**

1. **W2's fitted LF-walk share** (run `39676039083`, workflow
   `.github/workflows/anvil-i20-aux-walk-sweep.yml`, job `sweep (silesia)` / `sweep (enwik8)`,
   commits `1c6784e` / `ef5d44a`, artifacts `anvil-i20-aux-walk-sweep-silesia-39676039083` /
   `...-enwik8-39676039083`). Fitting `speedup(k) = 1/(1−f+f/k)` on **route-unconfounded** points
   only: **f = −0.0001 (enwik8) and f = −0.0041 (Silesia), every CI spanning 0**, against a
   required **f ≈ 0.70**. The Amdahl ceiling of the walk-budget lever is **1.0000×**.
2. **The mechanism, read from the vendored source** (`third_party/libsais/src/libsais.c`,
   `libsais_unbwt_decode_8`, lines 7854–7892): the inverse BWT runs **8 interleaved LF walks** of
   `r/2` steps with `offset += 8r` per group, so the **total number of dependent random accesses
   into the `4n`-byte LF array is `n/2` for every checkpoint stride `r`**. The walk budget cannot
   shorten the critical path.

**Taken together, these place C1's answer on the *falsifying* side of the pre-registered rule
(the LF-walk lever is dead), and the programme acted on it: C2 was ruled NO-GO.** But that is the
convergence of two other measurements, not C1's own measurement, and C1's own verdict stays
**INCONCLUSIVE**.

**What C1 would still add, and is worth a follow-up worker slot:** the **instruction share versus
the wall-clock share** of the LF walk. The walk is a cache-miss-bound loop (a dependent random
access into a 4n-byte array per step) while the postcoder is compute-bound, so its *wall-clock*
weight should materially exceed its *instruction* share. A callgrind-only attribution would
understate the walk's cost, and that contrast is the one measurement the programme still lacks.

---

## 4. Evidence fields for every number

| # | Claim | run_url | job | workflow | commit_sha | artifact | corpus | label |
|---|---|---|---|---|---|---|---|---|
| E1 | the stage-decomposition tool and workflow exist and are committed | — | — | `.github/workflows/anvil-i20-stage-decompose.yml` | `2f31e94`, `70dbaa6` | — | silesia, enwik8 | — |
| E2 | four dispatch attempts, none produced a usable profile | https://github.com/thelabcorner/anvil/actions/runs/39659153216 · …/39659202582 · …/39659854677 · …/39660334510 | stage-decompose | `.github/workflows/anvil-i20-stage-decompose.yml` | `2f31e94`, `70dbaa6` | — | silesia, enwik8 | MEASURED (the failures) |
| E3 | the fitted LF-walk share that answers C1's question by another route | https://github.com/thelabcorner/anvil/actions/runs/39676039083 | sweep (silesia) / sweep (enwik8) | `.github/workflows/anvil-i20-aux-walk-sweep.yml` | `ef5d44a` | `anvil-i20-aux-walk-sweep-silesia-39676039083`, `...-enwik8-...` | silesia, enwik8 | MEASURED (W2's) |
| E4 | the LF-step count of n/2 for every stride r | — | — | — | `755df76ae53795aea040f4c99784d1effb43b3ab` (`third_party/libsais/src/libsais.c:7854-7892`) | — | — | DERIVED (exact source reading) |
| E5 | the frozen baseline the programme compares against | https://github.com/thelabcorner/anvil/actions/runs/39654699366 | reference-cost (silesia) / (enwik8) | `.github/workflows/anvil-i10-aux-reference-cost.yml` | `d05fceb…` | `anvil-i10-aux-reference-cost-silesia-39654699366`, `...-enwik8-...` | silesia, enwik8 | MEASURED (quoted) |

---

## 5. Next steps / blockers

1. **Re-run the profile step on a machine that can afford it** (a longer self-hosted runner, or
   a single-file / single-block scope) to obtain the LF walk's instruction share and wall-clock
   share. The tool and workflow are already committed and need no change.
2. **Do not re-litigate C2** — the walk budget is dead by the source argument and by W2's fit.
3. **The measurement that would most improve the programme is not a stage decomposition** but a
   reproducible *reference* measurement: the arbiter's same-job Brotli reference varied 1.98× across
   six identical runs of the closed arm, which is what made the G4 verdict a Bernoulli variable
   (3 of 6). See `docs/I20-PARETO-GATE-CLOSURE-RESULTS-2026-10-21.md` §4.4.
