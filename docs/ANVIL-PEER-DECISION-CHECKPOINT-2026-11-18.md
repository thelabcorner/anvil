# PROJECT ANVIL — independent peer decision checkpoint (18 November 2026)

**Research state:** VERIFY the A48 measurement contract; REFINE only a mechanism with a specific new causal test.  
**Execution disposition:** PROCEED_WITH_GUARDS for read-only audit/source fixes; TEST for a *separately preregistered* manual-only GitHub Actions experiment.  
**Preservation rule:** do not mutate the original A48 run/commit; keep its artifacts and the historical Class-A benchmark untouched. Draft PR #23 is a proposed *future* gate correction, not part of that run.

## 1. Observations (not forecasts)

- The authoritative goal is a credible **general-purpose, full-wire, multi-axis** compression portfolio: correctness, bytes, encode/decode speed, decode memory, decoder size and operational constraints. A 2D per-file non-dominance token is weaker.
- The original [A48 run 44380412243](https://github.com/thelabcorner/anvil/actions/runs/44380412243) checks out workflow commit `429364e32161c766d827e0f4c03314b3eaa53e31`, candidate `2c5eeb9e570b1e1162cbff98d29e0a978edeca92`, and compares cap-8 ANVIL to **only** xz-9e and Brotli q11/lw30 for the timed Silesia classification. Do not quote subsequent workflow repairs as evidence of what the original run executed.
- Ledger A44 (run `44370017321`): cap-8 Silesia **47,365,274 B** vs xz **48,456,004 B**; ANVIL decode **60.152 MB/s**; candidate/Brotli paired decode-time **2.3618** [2.2788, 2.4446]; candidate peak RSS **74.2 MiB**, **1.450x xz**. Its measured last open reference-cost gate is decode relative to Brotli; encode and decoder-binary-size remain unruled by that gate.
- Ledger A46 (run `44374141043`): cap-4 uses **47,941,299 B** and decode/Brotli **3.5133** [3.3760, 3.5662] with a **74.1 MiB** peak. Reference speed differs substantially across hosted runs: compare **paired same-job** results, never cross-job absolute throughput.
- Ledger A43/A44 locates Silesia cap-8 peak RSS near **mozilla**, a Brotli-routed file. Therefore changing the BWT inverse's work array alone cannot reduce this specific portfolio peak unless its peak file identity changes. It may still help BWT per-file decode/RSS. This is a critical boundary on an O(r)-memory LF hypothesis.

## 2. A48 H1 is mostly a mathematical consequence of existing byte evidence

Frozen `docs/gate-verify-i9.py` defines `DOMINATED` on a (ratio, throughput) plane if some reference has ratio <= candidate ratio and throughput >= candidate throughput (at least one strict). It defines `FRONT-GAP` only if a same-plane reference `q_lo` has ratio <= candidate ratio (and is slower). If ANVIL is **strictly smaller in bytes than every included reference** and its own ratio <0.95, neither predicate can hold. The gate therefore **must** call that row `FRONT-CROSSING` at *any positive* throughput, even an extremely slow one.

Ledger A47/A49 already reports strictly best bytes for cap-8 on six Silesia files (dickens, mr, osdb, reymont, webster, x-ray) versus the two A48 controls. Subject to exact identity, correct roundtrip, and usable timing evidence, **a positive two-reference R-2 label is nearly predetermined**. Thus A48 is valuable for exact corpus/candidate identity and paired cost data, but is *low information* as a discriminator between an architectural breakthrough and a weak/trivial size-edge crossing. A48 H1/H0 cannot be read as Class-A's original 468-cell 435/28/5/0 transition, or as all-axis Pareto dominance.

**Interpretation rule:** On artifact arrival, separately report (a) per-file and aggregate labels, (b) byte advantage for every file, (c) per-file paired decode/encode confidence and A/A null, (d) RSS and decoder footprint from the appropriate source, (e) coverage limitations of the thin reference set. Do not sum scope counts or convert invalid timing into H0.

## 3. Mechanism and prior-art discipline

**Read-only independent scout:** a run-length / move-structure LF mapping could replace a dense `4n`-byte inverse-BWT permutation array with `O(r)` metadata (`r` is BWT run count). But if derived **at decode** from the existing transmitted BWT, wire bytes are **unchanged**: that is a decode-working-set mechanism, *not* a new compression-ratio mechanism. A separate run-coded **wire** variant must pay complete syntax and be tested against the current MTF+zero-run postcoder; do not conflate the two.

A rough structural screening bound for a run index requiring `b` bytes per BWT run is `b*r < 4*n` merely to beat the dense LF table; for a 50% LF-memory reduction, `r/n < 2/b` (e.g. <0.125 for b=16, <0.083 for b=24). These are **necessary accounting inequalities**, not measured feasibility or predictions of speed. Run-aware random predecessor searches may be slower than a dense LF table. Since mozilla sets Silesia's peak, even a successful BWT-only memory reduction may leave the *portfolio RSS unchanged*.

**Closest prior art, no novelty claimed:** SEA 2026 *Bounding the Average Move Structure Query for Faster and Smaller RLBWT Permutations* (DOI [10.4230/LIPIcs.SEA.2026.9](https://doi.org/10.4230/LIPIcs.SEA.2026.9)) demonstrates O(r) construction for LF-related move structures; [b-move](https://pmc.ncbi.nlm.nih.gov/articles/PMC11160816/) and [RLBWT Tricks](https://pmc.ncbi.nlm.nih.gov/articles/PMC11327918/) describe relevant tradeoffs. The DEFLATE-reconstruction lineage already includes [Microsoft preflate-rs](https://github.com/microsoft/preflate-rs), so DEFLATE exact-replay itself cannot be presented as a wholly new concept.

## 4. Next smallest decision-changing experiments (choose, do not blindly dispatch both)

**Option M — Brotli peak/cost attribution (first if strict portfolio RSS is the binding objective).** Before changing the decoder, use the existing per-file CI evidence and allocation traces to decompose *mozilla*'s ~74 MiB peak into compressed input, Brotli dictionary/history, framing, output and temporary buffers. Identify a concrete removable live allocation and measure its lifetime at the peak. A mere removal of the ~13.8 MB compressed payload cannot mathematically guarantee a fall from ~74 MiB to xz's ~51 MiB; additional savings would be needed. **Falsifier:** no recoverable live overlap large enough to cross the declared memory target, or time/byte cost of removal exceeds the predeclared guardrail. No new mode, no run without a pre-registered allocation question.

**Option L — BWT run-aware inverse oracle (only if decode speed or BWT-specific RSS is the decision objective).** Sample **frozen real BWT-routed** subblocks including at least one high-run control; compute `n,r`, actual metadata bytes, LF lookup steps, candidate memory lifetime, exact reconstructed bytes/hash. Compare incumbent dense LF and an unaccelerated run-coded index. If no plausible decode-latency improvement and no meaningful per-file RSS improvement, PARK. **Do not promote** a run-count oracle to a timing claim. If it survives, preregister a minimum viable single-job controlled decode ablation, keeping the wire identical, raw repetitions and stable null tests.

**Decode cost threshold:** A44's cap-8 candidate/Brotli time ratio is 2.3618, so to achieve a 2.0 point threshold under an unchanged matched control requires roughly 15.3% less *end-to-end* candidate time (1 - 2/2.3618); exceeding the CI-upper 2.4446 benchmark requires ~18.2% reduction under comparable noise. These estimates describe the size of the obstacle, **not** promised mechanism uplift; actual acceptance must come from the same-job paired CI.

## 5. Decision gate and stop rule

- If A48 produces valid, correctly scoped positive labels: **VERIFY**, not a new architectural victory. One additional broader experiment only if it can flip the choice of codec route or prove a previously unmeasured all-axis claim.
- If A48 has incomplete/incorrect roundtrip or invalid timing: **INCONCLUSIVE/INVALID**; use [draft PR #23](https://github.com/thelabcorner/anvil/pull/23) to repair the instrument, then run the *smallest* source-attested follow-up needed. Do not label it H0.
- If original time limit prevents a completed grid: retain any partial evidence from the run's surviving artifacts; future run should checkpoint per file/shard work and protect against wasted 240-minute jobs.
- Do not restart cap-only or prior explicitly falsified transform sweeps without a **different** causal mechanism and falsifier.
- Research gets one of EXPLORE / REFINE / VERIFY / PIVOT / PARK / CONCLUDE / STOP; execution separately gets PROCEED / PROCEED_WITH_GUARDS / TEST / MITIGATE / AWAIT_AUTHORIZATION / STOP_OR_REDESIGN. No false confidence percentages or unbounded risk investigations.

**Revisit trigger:** new immutable A48 evidence, a source-attested explanation of *mozilla*'s peak RSS, a BWT run-count oracle that changes the LF working-set decision, or explicit change to the overall goal. Not another lengthy speculative summary.
