# 06 — Do-Not-Reburn Register

This register is intentionally strict. “Closed” does not mean mathematically impossible unless stated. It means **do not spend more time without satisfying the listed reopen condition**.

## A. Closed by project math / structural argument

### A1. Decode-only frontier work on the old local-corpus cells

Iteration-8 Route-B analysis showed decode-only changes cannot create a meaningful crossing on 12/13 measured local files at current byte counts. Most required multipliers far beyond the attainable byte-identical decoder ceiling.

**Reopen only if:** bytes also move materially, or the reference/front changes enough to invalidate the calculation.

### A2. Statistical zero-bit derived transform parameters

For noisy/statistical arithmetic relations, parameter-estimation error grows with reference distance; measured residual bits/word rose approximately with log distance. Better estimators changed the intercept, not the slope.

**Reopen only if:** the transform is exact, not estimated, or a new derivation invalidates the recorded distance-error argument.

### A3. Reading raw `EXTENDS_FRONT` as a win

Permanently closed by claim semantics. Reference-grid gaps can create non-dominance mechanically.

**Replacement:** FRONT-GAP / FRONT-CROSSING / DEGENERATE classification.

## B. Closed for the current formulation

### B1. `(k,slot)` topology/modal residual coding

Repeatedly lost. Modal accuracy and mask recurrence are insufficient; favorable synthetic structural data did not rescue final bytes.

**Reopen only if:** a new representation produces a measured stationary residual topology that clears a pre-computed coding break-even point before mode-13-like coding is implemented.

### B2. SRR synchronized period probing feeding the old topology coder

The probe can find more structural tokens but that evidence does not become compressed-byte value and can regress ratio.

**Reopen only if:** a different downstream representation directly monetizes the discovered period, not merely another mask-default variation.

### B3. Reinforce-taken structural channels

Did not change parse output because the parser’s existing choices were not the structural period the channel hoped to reinforce.

**Reopen only if:** discovery semantics are fundamentally different.

### B4. Current PNRA candidate injection

Finds real candidates but short/far matches do not amortize token framing; end-to-end result is wash/regression.

**Reopen only if:** exact real-wire cost prediction rejects the bad candidate class, or a new token amortizes multiple invariant fields per reference.

### B5. Current RLZ/RePair stream implementation as default

Ratio wins are real, but encode is tens of times slower and decode is worse on primary record files.

**Reopen only if:** construction complexity changes class or decoder materialization is eliminated enough to reverse measured economics.

### B6. Whole-codec stream budget at λ=0.01 as a decode lever

The objective was too small to flip any meaningful stream. This was arithmetically predictable.

**Reopen only if:** a new cost philosophy is pre-registered and the candidate decision boundaries are shown to actually move before implementation.

## C. Do not generalize from synthetic/repo evidence

### C1. Generated JSONL line transpose as a corpus win

Large self-test win, but canonical text/corpus evidence does not show general transfer. Keep as a stress test.

### C2. Generated record context gains as universal context proof

Context modeling is useful, but external transfer is backend-dependent. `mr` is the only current top-level ctx1 Silesia win.

### C3. ARI-REF synthetic arithmetic win as frontier evidence

The representation expresses arithmetic progressions, but transformed reference codecs are much stronger than the raw comparison originally implied.

## D. Low-value micro-optimization loops

### D1. More hot-op dispatch tuning without a new whole-codec profile

Fusion and hot-op books already moved the hot loop until CRC/materialization/allocation dominated.

**Reopen only after:** a current profile on a byte-competitive backend shows opcode dispatch dominant again.

### D2. More boundary-candidate heuristics on the old parser

Measured output was unchanged; the existing cost model already absorbed the signal.

### D3. Reciprocal-rANS replacement of division

Historically measured slower than hardware divide on the target host.

**Reopen only on:** materially different architecture/compiler with a microbenchmark showing the premise changed.

## E. Process traps

### E1. Benchmarking only raw reference inputs when ANVIL gets a reversible transform

Always run same-transform reference controls where possible.

### E2. Comparing throughput across compiler/toolchain changes

Current BWT/reference timing confound is the live example.

### E3. Treating candidate count, detector hit rate, or entropy proxy as final value

All are diagnostics. Final value is complete payload bytes + runtime + memory + decoder size.

### E4. Letting old docs outrank current artifacts

`docs/CONTEXT.md` already contains stale build/dependency claims. Recompute.

### E5. Building the full mechanism before checking economic leverage

Before implementation, calculate whether:

- enough bytes are exposed;
- metadata can amortize;
- the cost objective can actually alter a decision;
- the reference codec is given the same representational opportunity.

If not, stop early.

## F0. Closed by measurement in the 2026-09-07 session

> **Note:** the E4 and E6 negatives are written up in full in **§G** (G1–G3)
> below. This section holds only the one entry §G does not cover.

### F0.1 Amortizing BWT decode cost by routing (closed by arithmetic)

Matching xz decode (78.7 MB/s) requires BWT-routed bytes ≤ **8.38%**, but the
BWT-favorable files are **56.79%** of Silesia (120,362,284 of 211,938,580 B).
Aggregate decode = `1/(f/13.0 + (1−f)/146.3)`; at the actual f it is
**21.4 MB/s = 3.7× slower than xz**. The byte win and the decode win conflict by
roughly **6.8×** — this is structural, not a tuning problem.

**Consequence:** routing cannot fix the decode axis. Only three options remain:
(a) accept FRONT-GAP-on-decode and report it honestly; (b) make the BWT
inverse/postcoder itself faster (the highest-leverage engineering target);
(c) win bytes from a fast-decode backend (see G3).

**Reopen only if:** BWT decode throughput materially improves (≳40 MB/s), which
changes the arithmetic and would re-enable broader BWT routing.

---

## F2. Do-not-reburn consequences of the specialist-disagreement map (2026-09-07)

The measured 4-way map (doc 10) tightens several existing gates with concrete numbers. These are not new closed lanes; they are **sharper reopen conditions** derived from where BWT/Brotli actually lose.

### F1. One-file-substrate mechanisms must clear a quantified bar (derived from doc 10 §3)

The residual gap to the landscape bar is concentrated: mozilla 429,893 B (64.7%),
sao 160,422 B (24.2%), ooffice 51,625 B (7.8%), samba 22,367 B (3.4%); all other
files 0.

**Reopen / fund only if:** a candidate mechanism (e.g. DEFLATE reconstruction P4.1,
float-aware transform for sao) can plausibly recover a comparable fraction of its
file's share of the 664,307-B gap. A mozilla-targeted mechanism that cannot recover
>~430 KB is not worth building (see 07 §P4.1 go/stop bar). This operationalizes E5
("enough bytes exposed") with a measured denominator instead of an estimate.

### F2. Do not re-open the "BWT is universally better than Brotli" lane

The map is explicit: BWT wins 7 files, loses 5 (4 of them to xz, not Brotli). Any
claim that BWT should be the default backend is closed by the measured per-file
split. Routing (P1.1/P1.2) is the only sanctioned path; a global-BWT default is
rejected by doc 10 §2.

### F3. zstd is not a ratio bar — do not re-burn comparisons against it

zstd u22/l27 loses every Silesia file to both Brotli and xz (doc 10 §1-2). Comparisons
that show ANVIL "beating zstd" as evidence of frontier progress are FRONT-GAP
artifacts (01 §5.1 / 11 §1) and must not be used to support a crossing claim. zstd
remains valid only for speed/landscape context.

### F4. Oracle/landscape figures must never be reported as results (process trap, reinforced)

`oracle_min(Brotli,BWT)` = 46,446,836 B and `oracle_min(Brotli,BWT,xz)` = 45,782,529 B
are derived from measured cells (doc 10 §3). They are targets, not achievements. Any
session that reports either as an "ANVIL result" has conflated a research target with
a measurement — this is the 01 §5.2 failure mode. bench-normalize's measured
`--ratio-backend=auto` total is the only thing that counts, and only if it beats the
xz bar (48,456,100) on bytes *and* stays within decode/peakmem margin (11 §3–§4).

---

## G. Do-not-reburn consequences of E4 + E6 (2026-09-07, measured)

Two independent measurements closed the two obvious "get more bytes" routes behind the
E2/xz win. Treat them as terminal, not as invitations to re-tune.

### G1. Block-local / sub-file backend routing is closed (E4, DECISIVE NEGATIVE)

`findings/e4-block-routing-negative`: block-local backend routing is **worse than whole-file**
on every tested file. webster BWT wins **10/10** 4 MiB blocks yet sum-of-blocks BWT =
8,211,644 vs whole-file 7,317,329 = **+894,315 B (+12.22%)** warmup tax; Brotli the same way
(+773,686 B). mozilla/samba/ooffice/sao have **zero** BWT-winning blocks at 4 MiB, so the
"mixed files contain BWT-friendly text regions" hypothesis (03 §6 / I5 / P1.2) is **falsified**.

- **Do not re-burn:** P1.2 (block-local routing oracle) and E5 (cheap router from oracle labels)
  are **PARKED / not runnable** — E5 was pre-gated on E4 passing, and E4 failed at every scale
  (block oracle strictly worse than whole-file: 256 KiB +5.42 MB … 16 MiB +643 KB). Mark them
  PARKED-by-negative, not "pending" and never "completed." bench-normalize corrected an earlier
  stray "completed" on E5 to CANCELLED/PARKED — the negative E4 is the useful outcome.
- **Reopen only if:** a backend with near-zero model warmup appears, or cross-block model
  carryover is implemented. Finer granularity (256 KiB / 1 MiB) is monotonically worse — warmup
  tax grows as blocks shrink, and 16 MiB is already near-zero tax for ooffice/sao.

### G2. QLFC and LZP postcoders are NO-GO (E6, measured)

`deliverable/E6` (bwt-theory, exact independent harness): QLFC (ID4) = 78,001,524 B vs baseline
75,564,140 B → **+2,437,384 B (+3.23%)**, 0/13 files win; static tables cannot track the
fast-changing MTF-rank distribution. LZP (ID5) = 75,843,559 B → **+279,419 B (+0.37%)**, wins on
only 4/13 — none of the headline BWT winners (webster +102 KB, x-ray +46 KB).

- **Do not re-burn:** implement `--bwt-post=4/5` as ratio gains. They are proven losses; ship
  only as ablation/scientific controls if arch-bwt wants them.
- This is consistent with the frozen E2 byte result (46,446,995 B, +159 B over oracle): postcoder
  work did not and will not move bytes. Redirect that budget to P3.1 / P4.1.

### G3. DEFLATE reconstruction (P4.1) is now the #1 next investment — on two independent arguments

(a) **Decode-axis:** its target files (mozilla/sao/ooffice/samba) are Brotli/xz-routed with fast
decode, so its gain costs **no** decode regression (unlike BWT/CM) — the only byte win that does
not fight the ~7× decode conflict.
(b) **Routing-axis:** E4 proves you **cannot** recover mozilla/samba/ooffice/sao by carving out
text regions (zero BWT-winning blocks there) — the only route into those files is reconstructing
the embedded compressed streams. Combined with bwt-theory T2 (mozilla's BWT loss is heterogeneous
compressed regions, not structural hostility), P4.1 is the clear highest-EV bet.

- **Fund/stop bar (unchanged from F1):** a mozilla-targeted mechanism must plausibly recover
  >~430 KB on mozilla alone (64.7% of the 664,307-B gap); otherwise not worth building.

