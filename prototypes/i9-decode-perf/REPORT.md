# PR-1 — decode time-share profiles + decode-lead selection (decode-perf, I9)

Status: **COMPLETE for the branch decision.** Branch verdict is NO-GO on the
pre-committed condition (see §2). Current-path and BWT splits below. Every number
is labelled `[measured]`, `[derived]` (arithmetic on measured values) or
`[projection]` (model/upper bound). Raw per-rep vectors and CPU-load logs are in
this directory; the exact commands are in §9.

---

## 1. Provenance (what was measured)

| artifact | source | sha256 | notes |
|---|---|---|---|
| `anvil_snapshot.cpp` (HEAD build) | `git show HEAD:src/anvil.cpp`, HEAD `fc23d9a` | `FDC0FE561E42C8E0687CD68E9DF50DD3639793EA2EEEC86EBFE0E2AF4A2C9240` | **byte-identical to the I8 t3 profiler snapshot** (profile_tmp/snapshot_provenance.txt). Bytewise CRC32; no BWT backend; no decode_threads. |
| `anvil_wt_raw.cpp` (worktree build) | working tree copy, unmodified | `5DB16A1C329D2B215D1123B1E16D54F92DA376EAFE1733A353B20EC831890686` | arch's `decode_one_block(mode,p,...)` fix present; PCLMUL CRC + slicing-by-8 fallback; `decode_threads` present (all runs use **1**). |

Binaries (hashes in `attest/*.txt`): `prof_i9.exe` (HEAD), `prof_i9_wt.exe` /
`bwt_prof_wt.exe` / `snap_anvil_wt.exe` (worktree). Harness is a **copy** of
`src/anvil.cpp` compiled in-lane; `src/anvil.cpp` was never edited.

Method = t3 reproduced (`prototypes/profile_tmp/prof.cpp`): median-7
**interleaved** reps, instrumented decoder verified **byte-identical** to the real
decoder per block, HIGH priority, pinned to the last logical processor, decoder
threads = 1, QPC timing, CV reported. CPU-load sampler (1 Hz
`Win32_Processor.LoadPercentage`) runs for the whole window; logs in
`attest/<label>_cpuload.csv`. Raw per-rep seconds: `results/<label>_reps_*.csv`.

Containers were encoded by the matching snapshot CLI and round-trip verified
SHA-256 against the corpus file before timing (labels below carry the source).
Byte counts reproduce the canonical rows exactly (e.g. synth-timeseries
143,234 / 140,898 B; generated.json 89,589 B; BWT dickens 2,571,873 B /
webster 7,317,361 B).

> ⚠ **Re-validation rule (coordinator):** `src/anvil.cpp` was still moving while
> this ran. All current-path numbers are valid for worktree sha `5DB16A1C…` and
> must be re-run on arch's frozen sha before any *crossing* citation. The branch
> verdict below rests on the HEAD bytewise base (frozen, matches I8), not on the
> moving tree.

---

## 2. PRE-COMMITTED BRANCH — verdict

Pre-registered condition (PR-1): *if synth-timeseries CRC share < 40%, its
decode route dies and generated.json becomes the decode lead.* Bench's binding
anti-tie vocabulary v2: `C = 1/(1−s)`; **DECODE-GO** iff `C ≥ 1.02·R`,
**DECODE-TIE** iff `R ≤ C < 1.02·R`, **DECODE-SHORT** iff `C < R`; thresholds
<43.82% → SHORT, 43.82–44.95% → TIE, ≥44.95% → GO.

Base that matters for the pre-registration = the I8 t3 **bytewise-CRC** profile
(the 44% figure's own base). `[measured]`, HEAD v3 run:

| row (container) | baseline MB/s | CRC bytewise share **s** | C = 1/(1−s) | needed R (1.78× raw) | verdict |
|---|---:|---:|---:|---:|---|
| synth-timeseries hotop (143,234 B) | 137.6 | **26.3%** | 1.356× | 1.78× | **DECODE-SHORT** |
| synth-timeseries hotop-budget | 141.3 | **28.7%** | 1.402× | 1.78× | **DECODE-SHORT** |
| synth-timeseries hotop-rlzp (140,898 B) | 139.1 | **27.0%** | 1.369× | 1.78× | **DECODE-SHORT** |
| generated.json mdl (mode 10, 89,589 B) | 155.4 | 29.7% | 1.421× | 4.50–4.59× | DECODE-SHORT |
| generated.json hotop-budget | 206.2 | 39.4% | 1.649× | ≫ (ratio binds) | DECODE-SHORT |

Repeat runs (HEAD v1, v3 and the WT bytewise counterfactual) bound the
synth-timeseries share at **26.3–29.6%** — 14+ points below 40% and 14 points
below the SHORT threshold. Margin ≫ run noise.

### **BRANCH: NO-GO — synth-timeseries's CRC-only decode route dies; generated.json becomes the sole decode lead.**

Why the I8 44% did not transfer `[derived]`: the CRC share is
`s = crc_ns_per_byte × decode_speed`; 44% was **generated.log's** number (decodes
at ~235 MB/s with a 1.86 ns/B bytewise CRC). The timeseries row decodes at
137–150 MB/s on the same base, so its share is ~27%. The pre-registered transfer
assumption is falsified by direct measurement — exactly the outcome PR-1 was
written to test.

---

## 3. CRC-only lift table (anchored), both bases `[measured]+[derived]`

| row | s (bytewise, I8 base) | 1/(1−s) bytewise | s (current dispatch) | 1/(1−s) current |
|---|---:|---:|---:|---:|
| synth-timeseries hotop | 26.3% | 1.356× | 1.5% | 1.016× |
| synth-timeseries hotop-budget | 28.7% | 1.402× | 1.3% | 1.013× |
| synth-timeseries hotop-rlzp | 27.0% | 1.369× | 1.5% | 1.015× |
| generated.json hotop | 36.7% | 1.579× | 3.4% | 1.035× |
| generated.json hotop-budget | 39.4% | 1.649× | 2.3% | 1.023× |
| generated.json hotop-rlzp | 33.4% | 1.502× | 2.1% | 1.021× |
| generated.json mdl (mode 10) | 29.7% | 1.421× | 1.5% | 1.015× |

**The CRC leg is already landed** in the worktree: PCLMUL dispatch measured at
**0.067–0.079 ns/B (12.7–14.9 GB/s)** vs 1.91 ns/B bytewise and 0.445 ns/B
slicing-by-8. Consequence: no further CRC work exists (≤1.5% headroom). The
leg's *delivered* effect on synth-timeseries is 137.6 → 195.2 MB/s
(+1.42×) `[measured, HEAD vs WT same harness]`.

---

## 4. Current-path decomposition (worktree 5DB16A1C, PCLMUL CRC) `[measured]`

Shares of end-to-end median; windows/logs in `attest/i9_mode15_wt_v2.txt`.

| row | baseline MB/s | crc (dispatch) | mat = setup-only | token loop | alloc+concat+hdrs | memcpy |
|---|---:|---:|---:|---:|---:|---:|
| synth-timeseries hotop | 195.2 | 1.5% | 28.4% | 60.4% | 9.1% | 0.6% |
| synth-timeseries hotop-budget | 196.1 | 1.3% | 25.3% | 61.1% | 11.8% | 0.6% |
| synth-timeseries hotop-rlzp | 216.9 | 1.5% | 31.3% | 62.0% | 4.7% | 0.6% |
| generated.json hotop | 355.8 | 3.4% | 61.1% | 34.4% | −0.1% | 1.3% |
| generated.json hotop-budget | 315.8 | 2.3% | 55.2% | 30.2% | 11.3% | 1.0% |
| generated.json hotop-rlzp | 306.0 | 2.1% | 67.6% | 29.5% | −0.1% | 1.0% |
| **generated.json mdl (mode 10)** | **220.5** | **1.5%** | **51.4%** | **24.9%** | **21.5%** | **0.7%** |

Reading: after the CRC leg, the **materialization** leg is the only large
wire-invisible target (25–68% of e2e depending on row). "Token loop" is the
residual execution work (stream pulls, copy, dispatch) and is the floor.
`[projection]` full-removal caps: mat-only `1/(1−s_mat)`; mat+alloc
`1/(1−s_mat−s_alloc)` — **upper bounds only**; theory's ruling stands:
lazy materialization *defers* work and alloc is partly a consequence of eager
materialization, so the legs are negatively correlated and the real combined
saving is less than the product.

Required remaining lift on the current path `[derived]`:
- synth-timeseries hotop-rlzp: 282.2 / 216.9 = **1.30×** (mat cap 1.46×; mat+alloc cap 1.60×)
- synth-timeseries hotop: 282.2 / 195.2 = **1.45×** (mat cap 1.40× — mat alone insufficient; mat+alloc cap 1.60×)
- generated.json mdl: 517.5 / 220.5 = **2.35×** (mat cap 2.06× — mat alone insufficient; mat+alloc cap 3.69×) — **SUPERSEDED for citation:** on the re-validated `62BC6631` baseline (212.5 MB/s) this is **2.44×** (§12); quote **2.44×** for the pinned sha, 4.59× for the frozen CSV baseline.

---

## 5. generated.json mdl (mode-10) decomposition `[measured]`

The branch-designated lead. Container `generated_json_mdl.anv` = 89,589 B,
ratio 0.1082, 4 mode-10 blocks; baseline **220.5 MB/s** (3.754 ms, CV 6.5%).
Mode-10 instrumented split (5 eager rANS stream materializations vs token loop):

| stage | ms | share of e2e |
|---|---:|---:|
| setup-only10 (materialize 5 streams) | 1.929 | **51.4%** |
| token loop beyond setup | 0.936 | 24.9% |
| crc (PCLMUL dispatch) | 0.056 | 1.5% |
| memcpy-out | 0.027 | 0.7% |
| alloc + concat + headers (residual) | 0.806 | 21.5% |

Crossing arithmetic `[derived/projection]`: bar = 517.5 MB/s (the generated.json
step; theory's iso-crossing). Remaining from my measured baseline = 2.35×.
mat-only cap = 2.06× (insufficient); mat+alloc cap = 3.69× (sufficient **only in
the limit**, negatively correlated). **Open contingency:** the frozen CSV row
says 112.8 MB/s → 4.59× remaining, and then even the mat+alloc upper bound does
NOT clear. A canonical re-measure of this row on the fixed build is required
before the lead is declared arithmetically open (bench's P0 re-verify was held
by the build blocker).

---

## 6. BWT-routed Silesia decode split (bwtinv request) `[measured]`

Worktree snapshot, canonical flags `--parse=ratio --ratio-backend=bwt
--ratio-context=off --ratio-lines=off`; containers match canonical bytes exactly
(dickens 2,571,873 / webster 7,317,361). Postcoder id 2 (arith-o1), transform 0,
single block. Baseline 10.0 MB/s (dickens) / 12.5 MB/s (webster); threads = 1,
libsais_unbwt single-thread.

| stage | dickens ms / share | webster ms / share |
|---|---:|---:|
| postcoder decode (arith-o1 -> BWT bytes) | 409.7 / **40.1%** | 1167.0 / **35.2%** |
| alloc out+tmp | 6.2 / 0.6% | 25.8 / 0.8% |
| libsais_unbwt (B1+B2 minus A minus B1) | 647.3 / **63.3%** | 2302.4 / **69.4%** |
| transform inverse | 0.9 / 0.1% | 4.5 / 0.1% |
| crc (dispatch) | 2.1 / 0.2% | 8.6 / 0.3% |
| memcpy concat | ~0 / 0.0% | ~0 / 0.0% |
| residual (parse+alloc+concat) | −42.7 / −4.2% | −177.2 / −5.3% |

Caveat: independent-variant sum exceeds the end-to-end baseline by 4–5%
(variant overhead), so individual shares carry ±5% relative uncertainty.
**Conclusion for bwtinv: the inverse transform dominates (≈⅔), postcoder ≈⅓.**
The inherited "postcoder ≈40%" is confirmed; it is *not* the majority cost.
Container CRC/concat is negligible (≤0.3%). Harness v1 reported a wrong B2
(double-counted stageA); fixed in `bwt_prof.cpp`; raw medians unchanged and the
table above is the corrected derivation from those medians.

---

## 7. Ranked decode-lead list

1. **generated.json mode-10 mdl (89,589 B) — the branch-designated lead.**
   mat 51.4%, alloc 21.5%, CRC spent, need 2.35× (my baseline) / 4.59× (frozen
   CSV, contingency open). First leg: **materialization**.
2. **synth-timeseries hotop-rlzp (140,898 B)** — cheapest remaining lift on the
   board: **1.30×**; mat 31.3%, alloc 4.7%; mat+alloc cap 1.60×. Same leg; best
   staging cell for the byte-identity gate.
3. synth-timeseries hotop / hotop-budget (143,234 B) — 1.45×; mat 25–28%.
4. generated.json mode-15 rows (ratio 0.1405–0.1466) — **decode-closed by
   ratio**: the binding reference set at those ratios is q9/zstd-19
   (739–1506 MB/s). Do not build.
5. BWT-routed Silesia — separate program: unbwt is the target (bwtinv).

`[projection]` warnings carried forward: every mat/alloc cap is an **upper
bound** (deferral + negative correlation); no leg may be cited as a crossing
until it is landed with byte-identity + PR-4 attestation + arbiter sign-off.

---

## 8. Handoff to arch (exact)

1. **Do not build more CRC work.** PCLMUL is landed; remaining share 1.3–3.4%
   (synth-timeseries / generated.json). Any further CRC effort is noise.
2. **Build the materialization leg first** — lazy/on-demand pull of the mode-10
   five rANS streams (types / ll / ml / dist / literals) replacing the eager
   `decode_stream` calls, byte-identical wire, staged behind the existing
   ablations.
   - **Branch cell (per PR-1 pre-commitment): generated.json mode-10 mdl
     (89,589 B, ratio 0.1082)** — mat 51.4% of e2e; needs mat+alloc together
     (2.35× vs mat-alone 2.06×).
   - **Staging cell (smallest lift): synth-timeseries hotop-rlzp (140,898 B)** —
     1.30× needed; mat 31.3%; same load-bearing code path (hotop fused decoder),
     so the byte-identity gate is cheapest there and the container is 5× smaller.
3. **Then the alloc/concat leg on mode-10** (21.5% residual: per-block vectors +
   final concat + headers). Pre-requisite for generated.json: bench's canonical
   re-measure of the mdl row decode (CSV says 112.8, this lane measures 220.5 —
   the whole remaining-lift arithmetic differs by ~2×).
4. **Do not** chase the generated.json mode-15 rows (decode-closed by ratio) or
   re-open rANS division micro-opts (DNB-D3).

---

## 9. Reproduce (exact commands)

```powershell
cd prototypes\i9-decode-perf

# HEAD-based profile (bytewise CRC = the I8 t3 base)
powershell -NoProfile -File .\build_i9.ps1 -Head
powershell -NoProfile -File .\run_mode15.ps1 -Reps 7 -Label i9_mode15_head_v3 -SkipEncode

# current-worktree profile (PCLMUL CRC; arch's decode fix present)
powershell -NoProfile -File .\build_wt.ps1
powershell -NoProfile -File .\run_mode15.ps1 -Reps 7 -Label i9_mode15_wt_v2 -Wt -SkipEncode

# BWT-routed Silesia split (dickens, webster)
powershell -NoProfile -File .\run_bwt.ps1 -Reps 7 -Label i9_bwt_wt -Wt -Files dickens,webster
```

Results land in `results/<label>_summary.txt` (+ per-container `_results_*.txt`),
raw per-rep seconds in `results/<label>_reps_*.csv`, attestation + 1 Hz CPU-load
logs in `attest/`. Recompute any share by taking the median of a variant's rep
column and dividing by the baseline median column.

## 10. Attestation summary (PR-4 v1 fields)

| label | window (UTC) | CPU load med/max | notes |
|---|---|---|---|
| `i9_mode15_head_v3` | 2026-09-12T06:13:09.9Z–06:13:14.7Z | 47/47% (min 28, 2 samples) | HEAD bytewise base; snapshot `FDC0FE56…` |
| `i9_mode15_wt` | 2026-09-12T06:33:51.2Z–06:33:54.9Z | 40/40% (min 29) | worktree `5DB16A1C…` |
| `i9_mode15_wt_v2` | 2026-09-12T06:53:34.1Z–06:53:37.8Z | 37/37% (min 36) | + mode-10 ablation (v2 is the cited worktree run) |
| `i9_bwt_wt` | 2026-09-12T06:36:48.1Z–06:39:20.3Z | 41/94% (min 16, 87 samples) | BWT split; max load spike noted |

Threads = 1 everywhere (comparability with single-threaded brotli/xz; worktree's
`decode_threads` explicitly left at the default 1 by the harness). No peer timing
job was observed on the pinned LP during the mode-15 windows; the BWT window's
94% load spike is disclosed and raises that table's CVs (baselines CV 9.1/9.4%).

---

## 11. RE-VALIDATION on arch's frozen gate sha `62BC6631…` (format-pinned)

`src/anvil.cpp` moved from `5DB16A1C…` to `62BC6631…` (the sha pinned by format's
green gate, PENDING-0 cleared by the coordinator). Rebuilt the lane snapshot
unmodified at `62BC6631` and re-ran both attested passes:

- **mode-15, label `i9_mode15_wt_v3`** (window 2026-09-12T07:55Z-ish; load 25%):
  - PCLMUL dispatch re-measured **0.063–0.073 ns/B (13.7–15.8 GB/s)** — confirms
    the 0.067 ns/B figure (arch's requested re-validation). Slice8 0.445–0.493,
    bytewise 1.908–1.953 ns/B.
  - bytewise-counterfactual CRC shares: synth-timeseries **26.9 / 27.8 / 29.7%**;
    generated.json **41.9 / 35.9 / 37.0%**; mdl mode-10 **29.1%**. Branch verdict
    unchanged (synth DECODE-SHORT).
  - baselines: synth hotop 190.0, budget 199.0, rlzp 218.4 MB/s; json hotop 365.4,
    budget 281.4 (CV 15.9%), rlzp 301.3, mdl 212.5 MB/s — all within run CV of the
    `5DB16A1C` numbers (v2: 195.2 / 196.1 / 216.9 / 355.8 / 315.8 / 306.0 / 220.5).
  - mode-10: setup-only10 = 1.916 ms of 3.894 ms baseline = **49.2%** (v2: 51.4%);
    token 20.9% (v2 24.9%); residual 27.6% (v2 21.5%) — mat share stable ≈50%.
- **BWT, label `i9_bwt_wt_v2`** (quiet window: load 24% median / 63% max; containers
  re-encoded on the new sha and still byte-identical: 2,571,873 / 7,317,361):
  - dickens 11.2 MB/s: postcoder **43.9%**, libsais_unbwt **50.2%**, alloc 0.6%,
    residual 5.0%, crc 0.2%.
  - webster 12.5 MB/s: postcoder **34.2%**, libsais_unbwt **61.0%**, alloc 0.7%,
    residual 3.8%, crc 0.2%.
  - Cross-run range: postcoder 34–44%, unbwt 50–69% (variant-overhead ±5–10%
    absolute; the qualitative split — inverse transform is the largest single
    stage, postcoder second — is stable). The v1 B2-recompute caveat no longer
    applies to this run (corrected formula in `bwt_prof.cpp`).

**Citation status:** branch-relevant shares are re-validated at `62BC6631` and
stable. Absolute MB/s are lane-build (`/O2`, t3 protocol) numbers; when arch
publishes the canonical `./build` binary sha, a cross-check of the end-to-end
decode MB/s on that binary is the remaining step for any *crossing* arithmetic
(PR-4 quiet window + bench sign-off — bench owns that call).


---

## 12. ERRATUM — materialization leg measured NON-REMOVABLE (arch paired A/B); both PR-1 cells now decode-CLOSED

arch ran a paired A/B (reps=7, interleaved, same wire, core18, 1t) removing the
eager macro-stream materialization in favor of lazy `StreamPull`:
`generated.json` mode-10 mdl **4.1545 ms eager vs 4.2117 ms lazy (1.4% slower)**;
`synth-timeseries` mode-15 hotop-rlzp **1.4495 ms eager vs 1.8499 ms lazy (27.6%
slower)** (ref: arch finding msg_daf183f4). The measured "setup(mat)" share is
real *cost attribution* but **not removable liveness overhead**: the same entropy
decode work happens either way, and per-symbol pull dispatch costs more than bulk
decode + indexing. This supersedes every `mat`-inclusive cap in §4/§5/§7
(`[PROJECTION-UPPER-BOUND]` → **FALSIFIED**).

Re-derived removable set per row `[derived from §4/§11 measured shares]` — with
`mat = 0`, what remains is alloc/concat/headers (+memcpy):
- `synth-timeseries` hotop-rlzp: residual 4.5% + memcpy 0.6% → **C = 1.054×**,
  required 282.2/218.4 = **1.29×** → **decode-CLOSED**.
- `synth-timeseries` hotop: residual 10.2% + memcpy 0.6% → **C = 1.121×**,
  required ≈1.45× → **decode-CLOSED**.
- `generated.json` mode-10 mdl: residual 27.6% + memcpy 0.7% → **C = 1.394×**,
  required 517.5/212.5 = **2.44×** this-lane (4.59× frozen CSV) → **decode-CLOSED**.

Combined with the PR-1 branch (CRC-only DECODE-SHORT), the decode picture is now:
**both PR-1 cells have no live wire-invisible decode route.** The only remaining
live decode program is the BWT postcoder (leg-4: token arithmetic+Fenwick decode
is 91–93% of it and needs ~1.38× aggregate — see `POSTCODER-SPEC.md`). CRC and
materialization are spent/irreducible; alloc/concat cannot reach the bars.
Labels: arch's A/B = measured (their window); caps above = derived arithmetic on
this report's measured shares. No crossing claims.


### §12 closure — ALLOC-only leg IMPLEMENTED and measured (arch, 2026-09-12)

arch implemented the remaining removable set (ALLOC-only: modes 10/15 decode
straight into the output slot, no per-block vector, no concat copy, bounded
reserve) and measured it paired/interleaved (reps=7, core18, 1t, identical wire):
- `generated.json` anvil-mdl-rans (89,589 B): **4.1604 → 3.0766 ms = 1.352×**
- `synth-timeseries` anvil-hotop-rlzp-rans (140,898 B): **1.4829 → 1.3802 ms = 1.074×**

Against the requirements this **falsifies the crossing**: 1.352× ≪ R′ = 2.35–2.78×
and 1.074× < 1.30×. My derived caps in this §12 were **1.394× / 1.054×** — the
measured implementation lands within ~3–4% of the derived ceiling (alloc removal
was in fact worth slightly more on hotop-rlzp and slightly less on mdl), so the
attribution method is corroborated. Source now `38409E26…`; canonical
`build\anvil.exe E8AA2E48…`, `build\anvil_bench.exe 56092B9B…` (gates green:
442/442 roundtrip, fuzz 50 PASS, encode identity 494/494).

**Final closure statement:** both PR-1 cells are decode-CLOSED by measurement —
CRC spent (A3/A8), materialization falsified (A9), alloc implemented and
insufficient (this section). No wire-invisible decode route exists for either
cell. The only live decode program in I9 remains the BWT postcoder (leg-4).

**Provenance note (canonical move):** canonical is now src `38409E26…` /
`build\anvil.exe E8AA2E48…` / `build\anvil_bench.exe 56092B9B…`; the encoder wire
is identical to `62BC6631` (494/494 identity), so compressed-byte totals are
unchanged. The leg-3 decode numbers in this report remain valid for `62BC6631`
(snapshot `FDC0FE56…`) and need a re-run on the new sha before any citation tied
to it. Labels: arch's A/B and ALLOC deltas = **ranking-grade pending bench
attestation** (coordinator msg_722b6cb2); caps = derived. No crossing claims.