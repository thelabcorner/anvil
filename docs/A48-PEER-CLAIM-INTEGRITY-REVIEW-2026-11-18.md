# ANVIL A48 — Independent Peer Claim-Integrity Review (2026-11-18)

**Status:** independent engineering review; NOT an A48 result, a replacement classifier, or authorization to promote a finding.  
**Base:** `429364e32161c766d827e0f4c03314b3eaa53e31`, from `research/anvil-a48-crossing-20261118`.  
**Active preregistered GitHub Actions run:** [44380412243](https://github.com/thelabcorner/anvil/actions/runs/44380412243). At review time the `classify` job's step 8 (`Build per-file paired grid`) was in progress; prior six setup/build/corpus steps had passed. No artifacts or A48 outcome existed at that checkpoint.

## Scope protection

Project ANVIL's ultimate goal is a **credible general-purpose compression improvement across total wire bytes, correctness, encode speed, decode speed, working memory, binary size, and practical format constraints**. A48 is a *scoped* Silesia per-file 2D experiment with exactly **ANVIL cap-8, xz-9e and Brotli q11/lw30**. The historic 468-cell Class-A benchmark and the 12-file Silesia A48 grid are separate experiments. Do not transplant counts or throughput between them.

The frozen R-2 gate (`docs/gate-verify-i9.py`) defines a crossing as non-dominated, ratio < 0.95, with no reference gap bracket. **Mathematical edge case:** when ANVIL's ratio is smaller than *every included reference's* ratio, no reference with ratio <= ANVIL exists. Therefore neither dominance nor a `q_lo` gap anchor is possible; a `FRONT-CROSSING` label follows **regardless of ANVIL's slow decode/encode throughput**. This is a legitimate *within-sampled-grid* nondominance result, not a global all-axis win.

## Source-grounded defects and minimum repairs

1. **Correctness is not a hard gate.** `tools/a48_grid.py` computes `rt = all(sha256(...))` then emits rows with `roundtrip=FAIL`; it does not raise on failure. The A48 workflow later determines H1/H0 from only crossing count and timing validity, not `roundtrip`. A false positive H1 on a wrong-decoding codec is therefore possible. **Repair:** fail immediately on any non-OK roundtrip; independently check row-level roundtrip before promoting a verdict. Keep the raw evidence artifact in `always()` path.

2. **Noise/invalidity is misclassified as experimental H0.** Workflow's `[ "$V" = True ] || { echo "TIMING INVALID -> no crossing claim possible"; }` does not fail the step and later sets `H0_FRONT_CROSSING_ZERO` whenever `VALID != True` even if crossing count is positive. **Repair:** make three mutually exclusive dispositions: `INVALID_CORRECTNESS/INCOMPLETE`, `INCONCLUSIVE_TIMING`, and *valid* H1/H0 on the sampled grid. An unusable instrument cannot disprove a mechanism.

3. **Crossing count double-counts scopes.** Workflow's `grep -o '[0-9]* FRONT-CROSSING'` matches the `per-file :` line and the `AGGREGATE:` line in the frozen gate output. The sum is not a per-file crossing count; an aggregate-only crossing can independently trigger H1. **Repair:** parse both counts by their explicit headings (and retain both); preregister which one defines A48 H1.

4. **The reference robustness scan is ratio-only, not a dense frontier.** `ratio_scan()` in `tools/a48_grid.py` computes sizes for Brotli q1/3/5/9/11, Zstd 1/3/9/19, xz-9e and Brotli q11/lw30. It does **not** measure same-job timing of those additional tiers or feed them to frozen `gate-verify-i9.py`. A no-lower-byte anchor is useful, but the scan is not proof that the full ratio/throughput frontier has been crossed; nor does it cover all possible tuned configurations or xz transform controls. **Repair:** reserve any positive `FRONT-CROSSING` label for **SCOPED / THIN-GRID** wording; preregister a denser reference-envelope check only if A48 is valid and a positive changes the decision.

5. **Output sinks are asymmetrical.** In `paired_bench.py`, stdout defaults to `DEVNULL`. `xz -c` encode and `xz -dc` decode discard stdout; ANVIL and `brotli_lw` create output files. Thus I/O and page-cache work differ across candidates/references. The bias likely makes ANVIL look *slower*, but source and OS interactions mean it should not be asserted as harmless without a controlled sink-matched A/B. **Repair:** all measured arms use equivalent output handling and observed byte/hash checks; keep source/format identical.

6. **Per-repetition correctness is not observed.** `paired_bench.py` supports output observation but `a48_grid.py`'s `paired()` call does not pass paths/hashes; the once-per-file roundtrip is not a guarantee that all timed outputs were exact or stable. **Repair:** add observations where technically supported, or document the weaker assurance, without redefining the frozen metric.

7. **Resource cost and stopping conditions.** The 12 files x six paired comparisons x 7–9 timed reps plus warmups and ten ratio-reference encodes cause many costly, single-threaded xz/Brotli encodes. Current 240-minute GitHub job limit may truncate results. **Repair for future, not retroactive A48:** preflight a small representative scout, choose only decision-changing comparisons, shard jobs, or reuse exact deterministic byte-size results. Preserve preregistration and independently label protocol changes; avoid repeated multi-hour `gh run watch` loops.

## Correct evidence disposition after run

- **P0:** retrieve exact run SHA, immutable artifact ID/hash, corpus identity, binary SHA, full-wire counts, candidate flags and environment fingerprint; preserve all paired raw CSV/JSON, null CIs and control timing.
- **P1:** require every Silesia file, every expected row, `roundtrip=OK`, `timing_valid_all=true`, both null CIs enclosing 1.0, and equal/reasonable input byte counts. If any fails, report **INVALID** or **INCONCLUSIVE**, not H0.
- **P2:** independent recomputation of the frozen per-file and aggregate gate verdicts, **separately**. Record actual counts and any disagreements; no post-hoc selection of just positive files.
- **P3:** if valid crossing(s) exist, label *provisional 2D crossing versus measured Silesia thin references*; **do not** claim the original 468-cell frozen Class-A frontier is conquered or that all-axis Pareto is satisfied.
- **P4:** decide whether a fuller reference envelope, paired end-to-end throughput, RSS, decoder footprint, and held-out reproduction would actually change the next research/implementation choice. If no, park extra measurements and proceed with the more valuable decode/RSS mechanism. Do not repeat old negative mechanisms without new falsifier/evidence.

## Mechanism-distinct next research probes (not automatic work authorization)

- **Bit-exact DEFLATE reconstruction (P4.1):** Microsoft's `preflate-rs` demonstrates a prior-art approach: infer compressor behavior and encode token-choice corrections to regenerate an exact original DEFLATE bitstream. Its existence narrows novelty claims. Test ANVIL's distinct reconstruction mechanism only if total additional correction/program bits predict a verifiable net benefit; include bit-exact input hash, hostile/malformed streams, measured decoder burden, and comparable prior-art baseline. Source: https://github.com/microsoft/preflate-rs
- **BWT inverse memory/depth:** Brown & Langmead, *Bounding the Average Move Structure Query for Faster and Smaller RLBWT Permutations* (SEA 2026), offers length-capped move structures for run-length BWT LF permutations. An `O(r)` memory bound is useful only when BWT run count `r` is small enough to beat ANVIL's measured working-set/index overhead. First run an oracle-level `r/n` and projected metadata+lookup-cost calculation on **frozen corpora**, then an isolated CI comparison if that bound crosses the existing C7/L3 baseline. Source: https://arxiv.org/abs/2602.11029
- Keep other specialist typed/shape and exact reconstruction lanes in the existing portfolio; no unbounded fan-out. Use independent proposer/verifier evidence, not independent model votes.

## Risk and research decision

**Risk disposition:** `TEST` for existing A48 without touching its running workflow; `PROCEED_WITH_GUARDS` for read-only audit and isolated follow-up preparations. Avoid modifying the preregistered runner midflight.  
**Research disposition:** `VERIFY` any favorable A48 claim; `PARK` claims blocked solely by invalid/noisy timing until an instrument repair is justified; `REFINE` a mechanism only with a concrete falsifier and no duplicate prior negative.  
**Revisit trigger:** artifact arrival (or run timeout) plus a proven decision-changing difference; not simply another long agent response.

This review applies *risk-informed-engineering v1.0.0* and *breakthrough-oriented-research v1.0.0*, with hard gates for actual correctness, experimental validity and reproducibility. It creates no new format mode, changes no frozen arbiter, and runs no workstation/homelab benchmark.
