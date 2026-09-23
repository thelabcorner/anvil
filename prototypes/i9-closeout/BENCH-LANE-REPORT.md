# BENCH-LANE-REPORT — I9 close-out, independent recompute

Owner: `bench`. Date: 2026-09-20. Mode: read-only on all tracked files; no
benchmarks run; CSVs not modified. All figures recomputed from artifacts.

Method note: the Pareto predicate was re-implemented from the *source text* of
`tools/pareto_front.py` lines 28–41 only (`is_ref` = codec startswith
`(brotli-, zstd-, xz-)`; `q dominates p iff q.ratio <= p.ratio AND q.mbps >= p.mbps
and (qr<pr or qm>pm)`). The tool itself was NOT invoked. Independent script:
`%TEMP%\openfork\indep_pareto.py`. Aggregate rows use the same construction as the
tool (input-weighted harmonic throughput). Plane key = `encode_MBps` / `decode_MBps`.

---

## (1) INDEPENDENT PARETO RECOMPUTE

Command:
```
python %TEMP%\openfork\indep_pareto.py tests\benchmark-suite.frozen-bdc90474.csv
```

**Grid inventory actually in the file (`tests/benchmark-suite.frozen-bdc90474.csv`):**
- 351 data rows, header `file,input_bytes,codec,compressed_bytes,ratio,encode_MBps,decode_MBps,roundtrip`
- 13 files × 27 codecs (every file exactly 27 rows)
- 27 codecs = **18 anvil + 9 reference** (brotli q1/q4/q6/q9/q11; zstd 1/3/9/19)
- Codec inventory matches the strategy claim exactly (18 anvil + 9 ref). ✓

**Per-plane results (my recompute):**

| plane | anvil cells | non-dominated (EXTENDS_FRONT) | dominated | DEGENERATE (ratio==1.000) | non-degen non-dom |
|---|---:|---:|---:|---:|---:|
| encode_MBps | 234 | 23 | 211 | **36** | **5** |
| decode_MBps | 234 | 10 | 224 | 36 | **0** |

- **Per-file cells** = 234 per plane = 468 both planes (13×18 anvil). Confirms the
  `468` denominator. ✓
- **Non-dominated rows** = 23 encode + 10 decode = **33 total** ✓ (matches 33).
- **DEGENERATE (ratio==1.000 anvil cells)** = my count **36**, distributed as
  `random.bin` = 18, `synth-arith.bin` = 18 — **NOT 28**.
- **Dominated** = 435/468 ✓ (211+224 = 435).
- **AGGREGATE rows**: 18 anvil aggregates per plane; 0 non-dominated, 18 dominated
  on each plane. Combined with per-file: 435+18=453 … but the co-listed convention is
  `435/468 (471/504)`; 504 = 468 + 36 aggregate cells (18 anvil × 2 planes) and
  471 = 435 + 36 aggregate-dominated. So **471/504 is arithmetically consistent**
  only if all 36 aggregate anvil rows are counted as dominated — which my run
  confirms (18+18=36 dominated aggregates). ✓

**MY TUPLE vs CLAIMED:**

| field | claimed | my recompute | verdict |
|---|---|---|---|
| total per-file cells | 468 | 468 | ✓ |
| non-dominated | 33 | 33 | ✓ |
| FRONT-GAP | 5 | 5 | ✓ |
| FRONT-CROSSING | 0 | 0 | ✓ |
| DEGENERATE (ratio=1.000) | **28** | **36** | ✗ **MISMATCH** |
| dominated | 435/468 | 435/468 | ✓ |
| combined | 471/504 | 471/504 | ✓ |

The claimed tuple is internally consistent ONLY because it is written
`33 | 5 | 0 | 28 | 435/468` — i.e. it asserts 33 = 5 real + 28 degenerate. My
recompute says 33 = 5 real + **36** degenerate; i.e. 36 (not 28) of the 33
non-dominated rows are ratio==1.000 store-path cells. **The "28" is wrong by 8
cells** (two files × 18 anvil rows, not two files × 14). Both
`docs/anvil-i9-findings.md:31` and `tests/suite-frozen-bdc90474.md` ("the 28-
DEGENERATE count is a real store-path speed effect") carry the wrong count.

Reconciling note: 351 rows × 2 planes does NOT equal 468; the 468 denominator is
anvil-only (234 × 2). The 28 vs 36 error appears to be a carryover from the
prior worktree grid where only `random.bin` stored (18 cells) plus a partial
count; the frozen grid stores on **both** `random.bin` and `synth-arith.bin`.

---

## (2) NON-DOMINATED ROWS

**The 5 real (non-degenerate) non-dominated rows — all encode plane, all
`generated.json`:**

| file | codec | ratio | encode_MBps | decode_MBps | decode-dominated by brotli-q11? |
|---|---|---:|---:|---:|---|
| generated.json | anvil-mdl-rans | 0.108 | 1.596 | 274.707 | **YES** (557.462 ≥ 274.7, 0.096 ≤ 0.108) |
| generated.json | anvil-mdl-rans-l0 | 0.108 | 1.595 | 275.319 | **YES** |
| generated.json | anvil-mdl-rans-l001 | 0.108 | 1.587 | 273.400 | **YES** |
| generated.json | anvil-shape-rans | 0.112 | 1.429 | 233.164 | **YES** |
| generated.json | anvil-shape-rans-l0 | 0.112 | 1.396 | 208.076 | **YES** |

**Confirmations:**
- All 5 are on `generated.json`'s **encode** plane. ✓
- All 5 are **decode-dominated by brotli-q11** (brotli-q11 decode = 557.462 MB/s,
  ratio 0.096). ✓
- Encode-plane bracket: the surviving rows sit between `brotli-q11`
  (ratio 0.096, enc **0.780** MB/s) and `zstd-19` (ratio 0.113, enc **2.839** MB/s).
  The strategy's bracket numbers "(0.096, 0.674)" and "(0.113, 1.963)" are the
  **prior-grid / v2-plus-xz values, not the frozen-grid values**; on the frozen
  grid the correct bracket is (0.096, **0.780**) … (0.113, **2.839**). Directionally
  the claim holds (the rows escape xz-9e's 1.32 MB/s but not zstd-19's 2.839), but
  the quoted bracket encodings are stale.
- **The other 28 (my count: 36) "non-dominated" rows** are ratio==1.000 store-path
  cells on `random.bin` + `synth-arith.bin`, encode plane (18) and decode plane (10);
  they are DEGENERATE under gate R-2 and are not crossings.

---

## (3) PROGRAM B BYTE FIGURES

**Silesia (12-file) auto portfolio:**
- `tests/auto-routing.csv`, codec `anvil-auto-direct`, 12 rows:
  Σ `input_bytes` = 211,938,580; Σ `compressed_bytes` = **46,446,995** ✓ (claimed 46,446,995)
- xz -9e 12-file Silesia total = **48,456,100** — located in
  `tests/ratio-first-benchmark.csv`, **`corpus == silesia` subset** (12 rows). ✓
  **NOTE: it is NOT in `ratio-first-standard.csv`.** That file's Silesia xz sum is
  **73,287,756** (13 rows incl. an enwik8 row) and its 12-file Silesia subset sums to
  a different value; neither equals 48,456,100. The strategy doc lists the wrong
  source file for the xz reference.
- Delta: 46,446,995 − 48,456,100 = **−2,009,105** ✓ **VERIFIED**.

**enwik8:**
- `tests/enwik8-bwt.csv`, `anvil-auto-direct` (== `anvil-bwt-direct` bytes) =
  **23,534,368** ✓ (claimed)
- xz -9e = **24,831,656** (`ratio-first-standard.csv`, enwik8 row) ✓
- Delta = 23,534,368 − 24,831,656 = **−1,297,288** ✓ **VERIFIED**
- Brotli q11/lw30 = 24,810,180 → delta **−1,275,812** ✓ **VERIFIED**

**Verdict on byte figures:** Silesia −2,009,105 **VERIFIED**; enwik8 −1,297,288
**VERIFIED**; enwik8-vs-brotli −1,275,812 **VERIFIED**. All three reproduce
exactly from raw CSV sums. One provenance defect: the Silesia xz figure's source
file is mis-cited (it is `ratio-first-benchmark.csv`, not `ratio-first-standard.csv`).

Also note: `ratio-first-standard.csv`'s `anvil-ratio` Silesia sum is **49,328,971**
— this is a *different codec* than the auto portfolio and must not be substituted
for 46,446,995.

---

## (4) ADMISSIBILITY TABLE (item-15)

Command: `git ls-files --error-unmatch <f>` + `git status --porcelain -- <f>`.

| CSV | tracked? | admissible as citation of record? |
|---|---|---|
| tests/ratio-first-standard.csv | UNTRACKED (??) | **NO** (uncommitted; also mis-cited as xz source) |
| tests/ratio-first-benchmark.csv | UNTRACKED (??) | **NO** — but holds the real 48,456,100 xz row |
| tests/bwt-backend-standard.csv | UNTRACKED (??) | **NO** |
| tests/auto-routing.csv | UNTRACKED (??) | **NO** — holds 46,446,995 |
| tests/enwik8-bwt.csv | UNTRACKED (??) | **NO** — holds 23,534,368 |
| tests/ratio-reverify-i9.csv | UNTRACKED (??) | **NO** |
| tests/benchmark-suite.frozen-bdc90474.csv | UNTRACKED (??) | **NO** — the "FROZEN CANONICAL GRID" is an untracked artifact |
| tests/benchmark-suite.[head-fc23d9a|c70179ea].plus-xz.csv | UNTRACKED (??) | **NO** |
| tests/thread-attestation.csv | UNTRACKED (??) | **NO** |
| tests/pr-4-measurement-window-protocol.md | UNTRACKED (??) | **NO** |
| tests/benchmark-suite.csv | TRACKED | yes, but dirty (M) |
| tests/benchmark-summary.csv | TRACKED | yes, but dirty (M) |
| tests/pareto-baseline.csv | TRACKED | yes, but dirty (M) |

**Every single Program B artifact is untracked.** Therefore **no Program B byte
figure (−2,009,105 / −1,297,288 / −1,275,812) is admissible as a citation of
record** until these files are committed (`git add`). The corpus
(`tests/corpus/…`) likewise shows `tests/corpus/CHECKSUMS.txt` and `README.md`
modified and six `pe-*.exe` plus five synth-* corpus files untracked. This is the
item-15 defect: the entire byte-axis win rests on uncommitted working-tree files.

---

## (5) PR-4 REQUIREMENTS AND CURRENT SATISFACTION

`tests/pr-4-measurement-window-protocol.md` (v1.2, BINDING) exact requirements —
**all nine fields mandatory** for any citable throughput number:

1. Tool + binary: exact cmdline, **SHA-256 of executable**, build dir.
2. Input: path, SHA-256, byte count.
3. Window: `window_start_utc` / `window_end_utc`, ISO-8601, second precision.
4. **Reps: raw per-rep values; median; reps ≥ 3** (≥ 7 for a headline).
5. Dispersion: `cv_pct = stddev/mean*100`; must not exceed `noise-floor.csv` CV for
   the same `(file,codec,metric)`, else 42% provisional ceiling.
6. Host load: 1 Hz non-idle CPU% sampler; report median + max; "none" for concurrent jobs.
7. Pinning/priority: affinity mask + priority actually set; logical cores named.
8. **Thread counts** for every row: `enc_threads`, `dec_threads`, `ref_threads`.
   Missing → treated as **1 only if known single-threaded**, else **unattested**.
   Any multi-thread anvil row vs single-thread ref must disclose
   `THREAD-IMBALANCE: anvil <N>t vs ref 1t` next to the row.
9. Label: `measured` | `derived` | `projection` (derived/projection never support a crossing).

Quiet gate (§3a): zero anvil*/ninja/clang*/cmake jobs; **pinned-core mean < 5% over
30 s**; during-run sampler; announce start/stop.
Citation-grade (§3b): all fields, same-window paired arms, affinity + pinned-core
gate, reps ≥ 3, external load ≤ 5 pp over ambient, arms interleaved.

**Current satisfaction — NONE satisfy PR-4 for any decode multiple:**

| decode multiple | source CSV | PR-4 satisfied? | missing |
|---|---|---|---|
| Silesia 12f auto 9.435 MB/s → 14.60x / 7.89x | auto-routing.csv | **NO** | binary SHA (pre-de9b4caf, PENDING); window_start/end absent; reps absent; cv_pct absent; host load absent; affinity absent |
| enwik8 auto 5.890 MB/s → 23.95x / 14.06x | enwik8-bwt.csv | **NO** | same as above |
| enwik8 bwt-direct 6.730 → 20.96x | enwik8-bwt.csv | **NO** | same |
| Silesia 7f bwt 13.015 → 11.243x | bwt-backend-standard.csv | **NO** | same |

Thread counts: `tests/thread-attestation.csv` asserts anvil ratio-first rows are
**1t by construction** (`tools/bench_ratio.py` has no threading code) and refs are
1t — that satisfies field 8 *provisionally*, but field 8 says missing counts are
treated as 1 only if the binary is *known single-threaded*, and these rows predate
`de9b4caf` with binary SHA **PENDING**. The CSV schema (`…,compress_s,
decompress_s,compress_peak_mib,…`) contains **no window, reps, cv, threads or
affinity columns** — so fields 3,4,5,6,7 are structurally absent from the artifacts
themselves. Every source CSV header is `timing_status = VALID_100MS_NONRECURSIVE`,
which is a *measurement* method label, not an attestation.

**These decode multiples are PR-4-UNATTESTED. All four are ranking-grade at best
and cannot support any bar, limit, or crossing arithmetic.**

---

## (6) ARITHMETIC THAT FAILED TO REPRODUCE

1. **DEGENERATE count = 28 → actual 36.** `random.bin` (18) + `synth-arith.bin`
   (18). Carried in `docs/anvil-i9-findings.md:31` and
   `tests/suite-frozen-bdc90474.md` ("28-DEGENERATE count", "33 non-dominated
   (5 real + 28 DEGENERATE)"). The tuple's DEGENERATE slot and the parenthetical
   `5 + 28 = 33` are wrong; correct is `5 + 36 = 41` non-dominated *if* degenerates
   are counted, or `33` only if the 36 are netted differently. As written the
   claimed tuple is self-inconsistent with its own prose.
2. **Bracket encoding values stale.** Strategy says brotli-q11 (0.096, 0.674) and
   zstd-19 (0.113, 1.963); frozen-grid measured values are (0.096, 0.780) and
   (0.113, 2.839). The *ordering* holds; the numbers do not reproduce on the grid
   the tuple is quoted for.
3. **Silesia xz reference source mis-cited.** Strategy lists
   `tests/ratio-first-standard.csv` as a Program B source for the xz byte figure,
   but 48,456,100 lives in `tests/ratio-first-benchmark.csv` (Silesia subset). The
   12-file Silesia xz sum in `ratio-first-standard.csv` is not 48,456,100.
4. **Program B source list omits `ratio-first-benchmark.csv`** — the file that
   actually contains the xz reference row.
5. Non-reproducing-but-consistent: HEAD `5|5|0|411/416` is the v1
   (brotli/zstd-only) tuple; the plus-xz file yields `4|4|0|412/416` — this matches
   the doc's own co-listed v2 row, so it is a grid-label, not a math, issue. My
   recomputes: HEAD+ xz = 4 non-dom / 412 dominated; C70179EA + xz = 1 / 467
   (matches the doc's v2 worktree row). ✓

---

## (7) RECOMMENDED I9 CLOSE-OUT ACTIONS (bench lane)

1. **Commit the artifacts before any citation.** All Program B CSVs, the frozen
   grid, both plus-xz grids, `thread-attestation.csv` and the PR-4 protocol are
   untracked. Item-15 is unresolved until `git add`/commit lands (bench owns these
   paths). Until then every Program B byte figure is ineligible as record.
2. **Correct the DEGENERATE count 28 → 36** in `docs/anvil-i9-findings.md` and
   `tests/suite-frozen-bdc90474.md`, and restate the tuple as
   `33 non-dominated (5 real FRONT-GAP + 36 DEGENERATE-eligible…)` with the
   consistent denominator, or explicitly define the 28 exclusion (only `random.bin`).
3. **Fix the Program B source list**: add `tests/ratio-first-benchmark.csv` as the
   home of the Silesia xz 48,456,100 row; stop citing `ratio-first-standard.csv`
   for it.
4. **Refresh the encode-bracket figures** on the frozen grid to (0.096, 0.780) /
   (0.113, 2.839), or label them as prior-grid values.
5. **Do not cite any decode multiple as attested.** Either re-run the four
   ratio-first decode windows under PR-4 v1.2 (binary SHA, window, reps≥3, cv,
   pinned-core <5%, affinity) or keep them explicitly `PR-4-UNATTESTED /
   ranking-grade`. No crossing arithmetic may use them either way.
6. **Freeze + hash** `indep_pareto.py`-equivalent recompute output into
   `tests/pareto-verdict.*` (tracked) so the arbiter tuple is reproducible from a
   committed artifact, not from an untracked frozen CSV.
