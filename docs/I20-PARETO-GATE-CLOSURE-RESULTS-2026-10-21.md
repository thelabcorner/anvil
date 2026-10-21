# ANVIL Iteration 20 — Pareto Gate Closure: Results

**Date:** 2026-10-21
**Status:** IN PROGRESS. Preregistration (binding): `docs/I20-PARETO-GATE-CLOSURE-PREREG-2026-10-21.md`.
**Goal:** move the canonical `--parse=ratio --ratio-backend=auto --bwt-aux=on` route from the ruling
`FRONT-GAP_COST` to `COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE` on frozen Silesia (enwik8 best-effort),
as ruled by `tools/reference_cost_gate.py` on a GitHub Actions run of `thelabcorner/anvil`.
**Compute locus:** GitHub Actions only. **No local-compute measurement is used as evidence anywhere in
this document.** Every number below is quoted from a named Actions run and artifact, or is exact
arithmetic over such numbers.

**Program base commit:** `research/anvil-i20-pareto-gate-closure-20261021` @ `edf8e04` (preregistration
`d05fceb`; arbiter workflow `cc37e24`; arbiter flag fix `edf8e04`).
`src/anvil.cpp` blob at the program base is **byte-identical** to the frozen I10-1A candidate
(`755df76ae53795aea040f4c99784d1effb43b3ab`), so the byte side of the baseline is reproducible by
construction.

---

## 1. Fresh baseline (MEASURED)

Two independent baseline reproductions were dispatched on the program branch. Both reproduce the
frozen I10-1A byte totals exactly, and both rule **`FRONT-GAP_COST`**.

### 1.1 Run A — frozen reference-cost gate (reproduction of run `35927623136`)

- **run_url:** `https://github.com/thelabcorner/anvil/actions/runs/39654699366`
- **job:** `reference-cost (silesia)` / `reference-cost (enwik8)`
- **workflow:** `.github/workflows/anvil-i10-aux-reference-cost.yml`
- **commit_sha:** `d05fceb0ce61a853378822f63eb5f2ceeef26557` (candidate `0dce534e5df40945d518c9ef4a722f4a0ecfc0e6`)
- **artifact:** `anvil-i10-aux-reference-cost-silesia-39654699366`, `anvil-i10-aux-reference-cost-enwik8-39654699366`
- **corpus:** silesia, enwik8

| Quantity | Silesia | enwik8 |
|---|---:|---:|
| ANVIL aux total bytes | **46,466,339** | **23,537,422** |
| `xz -9e` total bytes | 48,456,004 | 24,831,648 |
| Brotli q11/lgwin30 total bytes | 49,383,136 | 24,810,180 |
| **ANVIL byte lead vs `xz -9e`** | **1,989,665 B** | **1,294,226 B** |
| ANVIL byte lead vs Brotli | 2,916,797 B | 1,272,758 B |
| ANVIL decode (portfolio) | 48.452 MB/s (4.374 s) | 23.689 MB/s (4.221 s) |
| `xz -9e` decode | 71.344 MB/s | 93.750 MB/s |
| Brotli decode | 244.517 MB/s | 139.739 MB/s |
| ANVIL/`xz` decode-time ratio (CI95) | 1.4726 [1.4717, 1.4897] | 3.9552 [3.9010, 3.9585] |
| ANVIL/Brotli decode-time ratio (CI95) | 5.4547 [5.1604, 5.6033] | 5.8291 [5.4383, 5.9040] |
| A/A null ratio (CI95) | 1.0002 [0.9891, 1.0112] — **PASS** | 1.0002 [0.9879, 1.0119] — **PASS** |
| ANVIL peak decode RSS | 254,224 KiB (248.3 MiB) | 613,484 KiB (599.1 MiB) |
| `xz -9e` peak decode RSS | 55,408 KiB (54.1 MiB) | 67,880 KiB (66.3 MiB) |
| Brotli peak decode RSS | 125,228 KiB (122.3 MiB) | 256,976 KiB (251.0 MiB) |
| ANVIL/`xz` RSS ratio | 4.588× | 9.038× |
| ANVIL/Brotli RSS ratio | 2.030× | 2.387× |
| **Ruling** | **`FRONT-GAP_COST`** | **`FRONT-GAP_COST`** |

Gate status at baseline (Silesia): G1 timing **PASS**, G2 bytes<xz **PASS**, G3 decode ≤2× xz **PASS**,
G4 decode ≤2× Brotli **FAIL** (CI-upper 5.6033), G5 RSS ≤2× xz **FAIL** (4.588×), G6 RSS ≤2× Brotli
**FAIL** (2.030×). enwik8: G3/G4/G5/G6 all **FAIL**.

> **Runner-dependence note (important, and it is a caution against the historical baseline).**
> The historical run `35927623136` measured the Silesia Brotli reference at 166.9 MB/s; this
> baseline measured it at 244.5 MB/s, and `xz -9e` at 71.3 MB/s rather than 82.5 MB/s. Because the
> gate compares the candidate against references measured **in the same job**, the *ratio* is what
> governs, and on this runner the ratio is materially worse (ANVIL/Brotli CI-upper 5.6033 rather
> than 3.5059). The programme's Silesia target must therefore be judged against the *current*
> baseline: **the Silesia portfolio decode must fall from 4.374 s to ≤ 1.733 s (ANVIL ≥ 122.3 MB/s),
> a ≥2.52× portfolio speed-up.** The five Brotli-routed files (127,526,830 B ≈ 0.522 s) leave
> ≈1.211 s for the seven BWT-routed files (84,411,750 B), i.e. the BWT decode path must reach
> **≈69.7 MB/s (≈3.2× faster)**. All I20 thresholds are re-derived from this measured baseline, not
> from the historical one.

### 1.2 Run B — coordinator arbiter path (the tool that will judge the final arm)

- **run_url:** `https://github.com/thelabcorner/anvil/actions/runs/39655813000`
- **job:** `gate-closure (enwik8)` (and `gate-closure (silesia)`)
- **workflow:** `.github/workflows/anvil-i20-gate-closure.yml`
- **commit_sha:** `cc37e24af95c8e3450f45e0db650715ae17f670c` (`src/anvil.cpp` sha256 `770ea0f5f2ff32f84a775c8c1548dcf52b55f9d2a7c7a3bfd0ebaad4c59f5d90`)
- **artifact:** `anvil-i20-gate-closure-enwik8-39655813000`
- **corpus:** enwik8 (silesia job in flight at time of writing)

enwik8 ruling from the coordinator arbiter path: **`FRONT-GAP_COST`**, with
ANVIL 23,537,422 B / xz 24,831,648 B / Brotli 24,810,180 B, decode ratio vs xz 3.9552
[3.9010, 3.9585] and vs Brotli 5.8291 [5.4383, 5.9040], A/A null [0.9879, 1.0119] PASS, RSS ratios
9.038× xz / 2.387× Brotli. The `--expected-candidate-total 23537422` assertion **passed**, i.e. the
coordinator's candidate arm reproduced the frozen enwik8 total byte-for-byte.

---

## 2. Candidate verdicts

Populated as each candidate's pre-registered decision rule is applied to its own Actions evidence.
Verdicts are only `WIN`, `NO-GO` or `INCONCLUSIVE`.

| ID | Candidate | Worker | Verdict | Key numbers | Evidence |
|---|---|---|---|---|---|
| C1 | BWT decode stage decomposition | W1 | *in flight* | — | — |
| C2 | Auxiliary-index walk-budget sweep | W2 | *in flight* | — | — |
| C3 | BWT subblock-cap sweep below 8 MiB | W3 | *in flight* | — | — |
| C4 | Decode working-set engineering (0-byte RSS) | W4 | *in flight* | — | — |
| C5 | Postcoder decode-cost swap | W5 | *in flight* | — | — |
| C6 | Dense reference-grid control | W6 | *in flight* | — | — |

---

## 3. Combined-arm plan (the arm the coordinator will rule)

The gate's three binding Silesia deficits are decode-vs-Brotli (G4), RSS-vs-xz (G5) and
RSS-vs-Brotli (G6). No single lever clears all three; the programme's working combination is:

1. **C2 — auxiliary-index walk budget.** More checkpoints ⇒ shorter LF walks ⇒ faster inverse BWT.
   Charged cost ≈ 4 B × aux_count per BWT-routed file (≈ +430 KB over seven files at W = 16384).
2. **C3 — BWT subblock cap.** Bounds the per-piece inverse-BWT working set. Prior run `35930672607`
   measured 8 MiB at +898,935 B with peak RSS 124.4 MiB; 4 MiB and below were never measured.
3. **C4 — decode working-set reduction.** Zero byte cost; removes the materialised block output
   (≈39.5 MiB on webster) and the retained compressed input (≈7.3 MiB) from the peak.

The combined byte cost must stay inside the **1,989,665 B** Silesia surplus, and the combined peak
RSS must fall to ≤108.2 MiB (2.0 × the measured `xz -9e` 54.1 MiB) for G5.

The final arm is ruled by the **frozen** `tools/reference_cost_gate.py` via
`.github/workflows/anvil-i20-gate-closure.yml`, with the candidate built from the merged branch and
the encoder-policy flags passed through the additive, default-preserving
`--candidate-extra-args` flag (one shlex-quoted string; empty reproduces the pre-I20 command byte
for byte).

---

*Results are appended as each candidate's decision rule is applied. Nothing in this document is
asserted without a named GitHub Actions run URL, job, workflow file, commit SHA, artifact name and
corpus.*
