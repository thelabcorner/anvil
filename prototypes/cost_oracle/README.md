# prototypes/cost_oracle/ — S6-3 Measured-Cost Candidate-Rejection Oracle (PROTOTYPE)

Task u2, swarm `anvil-i7-cost`. Standalone harness; **no `src/anvil.cpp` edits**
(verbatim copies only; `arch` owns that file). Integration recommendation for
`arch` is at the bottom.

## What was built

`cost_oracle.cpp` (single file, clang-cl, Experiment-V build precedent):

```
clang-cl /std:c++20 /MD /O2 /EHsc /DNDEBUG cost_oracle.cpp /Fe:cost_oracle.exe
```

Pipeline per 256 KiB block (production defaults: chain=48, max-match=65535,
surprise=12, boundary/channels off):

1. **Parse once** with the verbatim production `parse_sparse`
   (mode 11: types 0–2; mode 14 `--pnra=on`: types 0–3 incl. PNRA).
2. **Build the real token streams** (types/ll/ml/ds/lits/masks/resid/tmask)
   exactly as `encode_tokens_{sparse,tcopy}` do, then run the verbatim
   per-stream codec suite (J = L + λ·C_decode, λ=0.01) with **per-symbol
   measured-byte attribution**: rANS renorm bytes charged to the symbol whose
   push flushed them; Huffman exact bit lengths; defexc mask-bit accounting;
   raw 1:1. Fixed costs (headers, model tables, final state) tracked
   separately. Identity `Σper_sym + fixed == bytes.size()` asserted per
   stream; winner bytes asserted identical to `encode_stream` (startup
   self-test, 24 synthetic buffers).
3. **Calibration** (target a): for every committed match token (stride-sampled
   to `--max-sample`), record parse-time predicted cost (bits) vs measured
   attributed cost (bytes), plus both alternatives. Sign flip = heuristic
   committed but measurement says the literal alternative is cheaper.
4. **One rejection pass** (target b): class R1 = {keep > drop·(1+θ)} (measured
   sign, θ default 0); class R2 = {type==3 && len<=8} — EXP. X's framing class,
   a stated shape formula (not per-file tuned). Decision is made by
   **measurement, not model**: encode the block with {keep-all, drop-R1,
   drop-R1∪R2} and keep the smallest actual payload. Variant 0 = baseline
   payload, so **end-to-end delta ≤ 0 holds by construction**. Single pass,
   no DP re-parse (explicitly distinct from EXP. F).
5. Round-trip decode verified every rep; timing = median of `--reps` (default 3).

**Harness-relative labeling (binding):** ratios/throughput are this harness's
own encode/decode of token streams with production codecs — NOT comparable to
anvil.exe absolutes (no container router). Correctness anchor: this harness's
baselines reproduce the ledger's EXP. X wire sizes **byte-exactly**
(anvil.exe 108,514 pnra=on / 108,518 pnra=off; anvil_bench.exe 846,050 /
845,675).

## Results (Windows, clang-cl 22.1.8 Release, reps=3 medians)

### (a) Calibration — predicted vs measured cost, committed candidates

| file | n | median err | P90 err | sign flips |
|---|---:|---:|---:|---:|
| generated.json | 5,102 | −1.6% | +7.8% | 0 |
| generated.jsonl | 2,799 | +1.0% | +6.0% | 0 |
| generated.log | 3,786 | −9.5% | +0.5% | 0 |
| generated.sqlite | 16,144 | −7.6% | +5.0% | 0 |
| doc.md / src.cpp | 124 / 2,003 | −4.0% / +4.0% | +59.4% / +54.7% | 0 |
| anvil.exe / anvil_bench.exe (m14 pnra=on) | 8,685 / 73,586 | +1.3% / +10.5% | +50.0% / +55.4% | 0 |
| pe-git.exe (m14 pnra=on) | 296,803 | +5.6% | +82.7% | 0 |
| synth-timeseries.bin | 6,629 | +16.2% | **+2219%** | 0 |

Pooled: **473,985 sampled candidates, zero sign flips** — the parse never
committed a candidate whose measured coded cost exceeded its measured
order-0-literal alternative. Stated X%: **median |error| ≤ 16% on every file;
P90 ≤ +83% on binaries/text**; the single large tail (synth-timeseries,
+2219% at P90) is the heuristic's flat `len/8` mask term massively
OVER-pricing long clean matches (pessimistic direction — can cause missed
candidates, never over-commit).

### (b) End-to-end delta (exact; ratio CV = 0 by determinism)

Record-file aggregate (generated.log/json/jsonl):
base 517,885 B vs oracle 517,885 B over 5,585,211 raw → **delta +0.0000%
(PASS, target ≤ 0)**. Delta is exactly 0 on **all 22 file-runs** (14 corpus
files in mode 11; the 8 PEs in mode 14+pnra): the measured-payload arbitration
finds no removal that shrinks any block (won0 everywhere; R1 fires 0 times in
the entire corpus; dropping R2 or even ALL type-3 tokens (`--force-t3`)
enlarges every block).

### (c) Encode cost vs greedy-class

Meaningful-scale files (>100 ms encode): **1.10×–1.40×** baseline
(jsonl 1.32×, log 1.38×, sqlite 1.15×, bench 1.03×, pe-git 1.10×).
PASS ≤ 2×. Outlier: generated.repeat.jsonl 2.36× — degenerate control
(940 KB → 1,182 B; parse is artificially ~free so the oracle's fixed O(n)
attribution dominates; absolute cost 30.8 ms). Absolute scale context: this
harness's greedy-class baseline on generated.jsonl is 13.4 MB/s (the
pre-registration's "~20 MB/s" illustration does not hold for this harness
either; the binding form is the ≤2× ratio, which passes).

### (d) Decode penalty

Files with ≥3 ms decode work: **−4.6% … +7.6%** (jsonl +1.3%, log +7.6%,
sqlite −0.3%, anvil_bench −4.6%, pe-git +0.25%). PASS ≤ 10%.
Sub-millisecond files swing ±15–136% on pure timer noise (random.bin's
+136% is 0.09→0.21 ms) — reported, not claimed.

## The two findings that matter (both measured)

**F1 — EXP. X's +0.044% regression is TRAJECTORY-borne, not token-borne.**
On anvil_bench.exe (pnra=on, 846,050 B): every committed type-3 token is
individually sound — type-3 median measured keep 4.45 B vs 43.0 B
order-0-literal alternative (R2 len≤8 subset: median 3.66 B vs 35.75 B;
full ranges 1.44–309.3 / 22.8–14,122), and **0 of 2,377 type-3 tokens flip**
(keep > drop) — removing the R2 class (len≤8, 1,470 tokens) enlarges every
block; removing ALL 2,377 type-3 tokens (`--force-t3`) still enlarges every
block; yet the pnra=off *parse* (different candidate trajectory, zero type-3)
is 845,675 B. The damage is done by PNRA commits **displacing better
exact/sparse matches downstream** (hash-chain insertion changes every later
decision), which no post-parse rollback can undo. Corroboration: `pnra-cost`'s
u5 PARSE-TIME threshold (changes trajectory) lands 845,657 < 845,675 on the
same bytes.

**F2 — the local heuristic is well-calibrated on what it commits.**
Median error ±16% (mostly small overestimates on binaries), zero sign flips
in 473,985 samples. EXP. X's "underpricing" diagnosis is thereby REFINED, not
contradicted: the formula's numbers are adequate per-candidate; the failure
mode is accepting candidates whose *global opportunity cost* (displacement)
exceeds their local gain — invisible to any per-token cost model, measurable
only end-to-end.

## Integration recommendation for arch

1. **Do NOT integrate a post-parse rollback pass expecting EXP. X recovery** —
   measured impossible (F1). The S6-2 parse-time threshold (pnra-cost u5,
   frozen formula) is the correct lever for the PNRA class.
2. **DO integrate the attribution machinery as diagnostic/calibration
   infrastructure** (`build_stream_attributed` + range-tagged token streams,
   ~200 lines, no decoder change): it gives exact per-token/per-stream measured
   costs for free whenever the encoder builds streams anyway, and directly
   powers S6-1's whole-codec stream budget and S6-2's FramingRaw terms with
   measured rather than estimated numbers.
3. **The measured-payload arbitration pattern (try K stated-formula variants,
   keep the smallest actual encoding) is the safe integration shape for ANY
   future refinement pass**: delta ≤ 0 by construction, cost = K−1 extra
   O(n) encodes (no re-search), and it cannot lie about interactions.
4. If a rejection pass is ever wanted for ratio, its classes must target
   *displacement* (e.g., re-parse spans without the offending candidate), not
   rollback of committed tokens — that is DP-class territory (EXP. F), out of
   S6-3's greedy-class scope.

## Files

- `cost_oracle.cpp`, `cost_oracle.exe` — harness (self-test at startup).
- `results_m11.csv`, `results_m14_pe.csv` — full-corpus rows (mode 11; mode 14 pnra=on, 8 PEs).
- `cal_jsonl.csv`, `cal_bench.csv` — per-candidate calibration dumps.
- Reproduce: `cost_oracle.exe --reps=3 --csv=r.csv tests\corpus\generated.jsonl`
  / `--mode=14 --pnra=on tests\corpus\anvil_bench.exe`.
