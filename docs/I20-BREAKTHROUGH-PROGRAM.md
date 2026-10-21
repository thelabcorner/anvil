# ANVIL Iteration 20 — "Pareto Gate Closure": Programme Report

**Date:** 2026-10-21 · **Programme branch:** `research/anvil-i20-pareto-gate-closure-20261021`
**Binding preregistration:** `docs/I20-PARETO-GATE-CLOSURE-PREREG-2026-10-21.md` (commit `d05fceb`)
**Numbers of record:** `docs/I20-PARETO-GATE-CLOSURE-RESULTS-2026-10-21.md`
**Ledger entry:** `RESEARCH_LEDGER.md` → A27
**Compute locus:** GitHub Actions only, on the public repository `thelabcorner/anvil`. **Zero
local-compute measurements are cited anywhere in this programme.**

---

## 1. What the programme was

ANVIL's canonical ratio route (`--parse=ratio --ratio-backend=auto --bwt-aux=on`) holds a
**reproducible complete-wire advantage** over `xz -9e` — **1,989,665 B on Silesia** (46,466,339 B
vs 48,456,004 B) and **1,294,226 B on enwik8** — and is ruled `FRONT-GAP_COST` because its decode
cost and peak decode RSS exceed the reference envelope. The repository's own frozen arbiter
(`tools/reference_cost_gate.py`) defines the closing condition: bytes below `xz -9e` **and** paired
same-job decode-time ratio CI-upper ≤ 2.0 against **both** `xz -9e` and Brotli q11/lgwin30 **and**
peak decode RSS ratio ≤ 2.0 against **both**.

I20 was a six-worker, one-candidate-each programme to close that gate, under one shared evidence
contract, with every candidate's decision rule fixed before measurement.

## 2. Outcome — the external reference-cost gate is CLOSED

**Ruling: `COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE` on Silesia**, emitted by the **frozen**
`tools/reference_cost_gate.py` on GitHub Actions run
[`39684313068`](https://github.com/thelabcorner/anvil/actions/runs/39684313068) (job
`gate-closure (silesia)`, workflow `.github/workflows/anvil-i20-gate-closure.yml`, commit
`7b9ea3f18397d61bab6f41cd14f0eb50da40b36b`, artifact
`anvil-i20-gate-closure-silesia-39684313068`, corpus silesia). The arm is
`--candidate-extra-args "--bwt-subblock 2097152"` on the merged arm (W2's encoder-policy knob +
W4's C4 levers), `reps = 9`, and the `--expected-candidate-total 48347006` byte-identity assertion
**passed**.

| gate | condition | measured | verdict |
|---|---|---:|---|
| G1 | timing valid; A/A null spans 1.0 and clears no speed gate | 1.00076 [0.99993, 1.02066] | **PASS** |
| G2 | bytes strictly below `xz -9e` (48,456,004 B) | **48,347,006 B** (lead 108,998 B) | **PASS** |
| G3 | decode ratio CI-upper ≤ 2.0 vs `xz -9e` | 0.9078 [0.8896, 0.9081] | **PASS** |
| G4 | decode ratio CI-upper ≤ 2.0 vs Brotli q11/lw30 | **1.6261 [1.4929, 1.6821]** | **PASS** |
| G5 | peak decode RSS ratio ≤ 2.0 vs `xz -9e` | 78,092 KiB (76.3 MiB) → 1.4903× | **PASS** |
| G6 | peak decode RSS ratio ≤ 2.0 vs Brotli q11/lw30 | → 0.6229× | **PASS** |

Baseline → closed arm: bytes 46,466,339 → 48,347,006 B (94.5 % of the surplus spent); decode
ratio vs Brotli CI-upper 5.6033 → 1.6821; peak decode RSS 254,224 → 78,092 KiB (−172 MiB); RSS
ratio vs `xz` 4.588× → 1.4903×; ruling `FRONT-GAP_COST` → **`COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE`**.

**The claim is bounded exactly as the tool bounds it:** this is **not** a FRONT-CROSSING — the
tool's own scope note says it "does not establish a crossing because encode and decoder-only
binary-size axes are not ruled here". The retained Class-A ruling is unchanged (435 DOMINATED /
28 DEGENERATE / 5 FRONT-GAP / 0 FRONT-CROSSING). What I20 established is that the canonical ratio
route's decode-cost and peak-RSS deficits are closable.

## 3. The six candidates and their verdicts

| ID | Candidate | Verdict | The one line that matters |
|---|---|---|---|
| C1 | BWT decode stage decomposition | **INCONCLUSIVE** (the profiling workflow failed four times on the hosted runner; the worker's session ended before the write-up) | — |
| C2 | Auxiliary-index walk budget | **NO-GO** | the inverse BWT performs `n/2` dependent random accesses for **every** checkpoint stride, so the walk budget cannot shorten its critical path — it is a byte knob only |
| C3 | BWT sub-block cap | **NO-GO on the gate's memory axis; adoptable decode-speed trade** | the cap is worth **2.26×** on the Silesia portfolio decode, but no cap meets G5 by itself because the portfolio RSS peak is a file the cap never reaches — it is the cap that made the *closed* arm possible |
| C4 | Decode working-set engineering | **WIN** (and it is what closed G5) | three zero-byte-cost decode levers; the peak fell 254,168 → 206,812 KiB, and with the Brotli-backend streaming fix the capped peak is 76,348–78,412 KiB = **1.38–1.42× `xz` → G5 PASS** |
| C5 | Postcoder decode-cost swap | **INCONCLUSIVE** (the worker's session ended before the measurement was published) | — |
| C6 | Dense reference-grid control | **WIN (control)** | 21 reference points per corpus; **no denser reference dominates the candidate**, so the byte advantage is not a sparse-grid artifact |

Full evidence, per candidate, with the six mandatory fields (run URL, job, workflow file, commit
SHA, artifact name, corpus), is in the results document and in the workers' own documents
(`docs/I20-RESULTS-W*.md`, PRs #16–#19).

## 4. What was learned that is not a verdict

1. **The inverse BWT's cost is a count, not a schedule.** `libsais_unbwt_decode_8` runs eight
   interleaved LF walks of `r/2` steps with `offset += 8r` per group, so the total number of
   dependent random accesses into the `4n`-byte LF array is `n/2` **for every checkpoint stride
   `r`**. Every "make the BWT decode faster by giving the decoder more checkpoints" idea is
   therefore dead on arrival, and the programme's own measurement confirmed it (W = 8192 → 16384
   made the same-job decode ratio *worse*, 1.2866 → 1.3986, for +419,744 B).
2. **What does make the inverse BWT faster is making its working set smaller.** A smaller
   sub-block cap shrinks the LF array from `4n` to `4C` bytes and turns the `n/2` random accesses
   into cache-resident ones: the 1 MiB cap decodes the Silesia portfolio **2.26× faster**
   (2.4733 s vs 5.3309 s). The decode-speed lever in this programme was **not** the checkpoint
   grid; it was the **working set**.
3. **The gate's memory axis is decided by a single file.** The Silesia portfolio's peak decode RSS
   is set by `mozilla` (51,220,480 B), which the router keeps on the **Brotli** route at every cap
   because its split-BWT payload always exceeds its Brotli payload. No sub-block cap and no BWT
   working-set lever can touch it; only streaming the Brotli backend's output can.
4. **G5 is unreachable by any Brotli-backed route**, because Brotli q11/lw30's own peak decode RSS
   is 2.26× (`xz`) on Silesia and 3.81× on enwik8. The decode gates and the memory gate are
   reachable by *disjoint* routes, and on enwik8 the two are irreconcilable: the caps that bound the
   working set flip the file to Brotli (G3/G4 pass, G5 fails at 3.803×), and the caps that keep the
   BWT route leave the peak at 6.223–9.059×. **enwik8 is therefore structurally out of reach for
   this gate, with the arithmetic recorded.**
5. **The gate's G4 verdict is reference-speed dependent.** Brotli's measured decode speed on this
   runner class varied by up to ~1.7× between runs while the candidate's own absolute decode time
   was stable to a few percent. Any G4 claim must therefore be replicated in a second run, and
   both the ratio and the absolute times must be reported.

## 5. The adopted change

C4's three levers are **decode-only** and cost **exactly 0 bytes**: L1 releases the compressed
input before the `4n` libsais array is allocated; L2 writes the reconstructed string in place over
the BWT string; L3 streams the reconstructed output to the output file with an incremental block
CRC-32, **now including the Brotli backend path** via the large-window streaming Brotli decoder.
The Adopt-Gate was met in full: all four frozen byte totals are exact (46,466,339 / 23,537,422 with
`--bwt-aux=on`; 46,446,995 / 23,534,368 with `--bwt-aux=off`), 52/52 + 4/4 round-trips are
byte-exact, the repo fuzz/registry path passes in the same job (1040 round-trip variants, 6240
mutations, 14 block modes, 5 transforms, 20 forced postcoders, 24 auxiliary-BWT assertions, 30/30
corpus fixtures), the in-place inverse BWT is byte-exact on 526.9 MB of real corpus data with zero
failures across both inverse paths, and the measured decode-time CI-upper is 1.00037 (Silesia) /
1.01202 (enwik8) against the 2 % epsilon — i.e. the levers are *faster*, not merely neutral.

The **encoder-side** change is the sub-block cap plus the walk budget, both of which are encoder
policy and wire-invisible.

## 6. Next ranked leads

1. **Rule the closed arm and replicate it.** The 1.75 MiB cap is the only measured configuration
   that passes G2 (48,454,402 B, 1,602 B below `xz -9e`), G3 and G4 simultaneously; the one gate it
   still failed was G5, and C4's Brotli-backend streaming fix is what closes G5. The decisive arm
   and a replication run are in `docs/I20-PARETO-GATE-CLOSURE-RESULTS-2026-10-21.md` §4.
2. **The byte margin is thin and must be stated as such.** 1,602 B on 48,456,004 B is 0.0033 %. It
   is *deterministic* (byte counts are exact and reproduce run-for-run), but any future change to
   the router or to the cap invalidates it. The 2 MiB cap (108,998 B of surplus) is the robust
   alternative and needs ≈10 % more decode speed; a smaller walk budget is the cheapest way to look
   for it, because a *small* `W` shrinks the per-sub-block index array without slowing the decode.
3. **The next mechanism must change the representation, not the schedule.** The ratio route's byte
   advantage is real and survives a 21-point dense grid; its decode deficit is an `n/2`-random-access
   floor. An inverse BWT with better locality (or a representation that does not need one) is the
   only way past it; more checkpoints are not.
4. **The DEFLATE-reconstruction byte lever (Q7 / P4.1) remains the top unstarted byte investment**
   and was rejected *for this programme only* — it does not touch the decode or memory deficits
   that the gate turns on.
5. **Re-open the enwik8 objective with a different gate.** enwik8's route is now understood
   precisely (Brotli for speed, BWT for bytes, never both), which is a better starting point than
   the one this programme had.

## 7. Honest limits

* **C1 and C5 are INCONCLUSIVE, not NO-GO.** C1's profiling workflow failed four times on the
  hosted runner and its worker's session ended before the write-up; C5's session ended before its
  measurement was published. Neither is evidence for or against its hypothesis, and neither is
  counted as a win.
* **The gate's memory axis compares *maxima*, and the reference maxima move between runs.** The
  G5/G6 numbers inherit that instability, as the G4 numbers do.
* **No novelty is claimed anywhere in I20.** Every result is `{adopt-class}` / `{engineering}`.
* **The retained Class-A ruling is unchanged: 435 DOMINATED / 28 DEGENERATE / 5 FRONT-GAP / 0
  FRONT-CROSSING.** I20 is a single-route, single-gate programme; it does not re-adjudicate the
  general-purpose frontier.
