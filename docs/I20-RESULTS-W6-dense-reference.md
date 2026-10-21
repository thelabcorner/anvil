# I20 / C6 — ANVIL dense reference-grid control (results)

**Worker:** W6 (swarm) · **Iteration:** ANVIL 20 — "Pareto gate closure"
**Branch:** `research/anvil-i20-w6-dense-reference` · **Candidate:** C6 (prereg §5.1)
**Class:** `{adopt-class}` / `{engineering}` control measurement. **No frontier token is issued and
no novelty claim is made or authorized.**

---

## 1. Hypothesis and pre-registered decision rule

**Hypothesis (pre-registered, prereg §5.1, C6):** the canonical ANVIL ratio operating point is
**non-dominated against a dense reference grid** on the (bytes, decode throughput, peak RSS) axes —
i.e. there is no reference configuration that is simultaneously smaller-or-equal in bytes and
at-least-as-fast in decode (or with lower-or-equal peak RSS) at the measured geometry. The binding
gate compares the candidate against only **two** reference points (`xz -9e` and Brotli q11/lgwin30)
and the repository itself labels that grid **GRID-THIN**; a candidate that clears the gate against
a sparse grid may be a sparse-grid artifact.

**Pre-registered decision rule (quoted verbatim):**

> **The final candidate is reportable only if it is non-dominated against the dense grid on the
> same axes. If a denser reference point dominates it, the program closes `NO-GO` on the dense
> grid, and this is recorded as the binding result.**

**Corroborating control requirement in the repository:** `docs/swarm-2026-10-02/REMOTE-EXPERIMENT-QUEUE.md`
§`NQ-6` — a byte win must be stated against **the best reference point in the frozen dense grid**,
not only against `brotli q11/lgwin30`; `q11/lw30` alone is not an admissible sole control for a
crossing-flavoured claim.

---

## 2. Method

| item | value |
|---|---|
| tool | `tools/i20_dense_reference.py` (new, this branch) — `prepare` / `rss` / `summarize` |
| workflow | `.github/workflows/anvil-i20-dense-reference.yml` (new, this branch; registered on `main` for dispatch) |
| candidate | `--parse=ratio --ratio-backend=auto --ratio-context=off --ratio-lines=off --bwt-aux=on`, the canonical ratio route, built from this branch's `src/anvil.cpp` |
| corpus | the canonical Silesia 12-file set (211,938,580 B) and enwik8 (100,000,000 B), fetched with `tools/gha_fetch_corpus.py` (pinned size + MD5) |
| references | Brotli q1/q4/q6/q9/q11 at window 22 and q11 at windows 24/30; `xz -6/-9/-9e`; zstd 1/3/9/19/22; lz4 1/9; plus pinned-build cross-checks for Brotli and xz |
| correctness | every reference and the candidate round-trip **byte-exactly** (SHA-256) before any timing is consumed |
| provenance | runner fingerprint, tool/candidate commit SHA, binary SHA-256, per-file corpus SHA-256, and the exact reference version strings are all written into the artifact |

**Reference-version discipline (the `NQ-6` requirement applied to the producer, not just the
metric):** the binding Brotli arm is the repository's own **pinned large-window** build
(`brotli_lw`, window 30) — the same producer the binding reference-cost gate uses — and the
distribution `libbrotli-dev` build is retained as an explicit *cross-check* row so the two Brotli
producers can be compared rather than conflated. The same is done for `xz` (pinned vs distribution).
Every version string is recorded in the artifact.

---

## 3. Results — Silesia (MEASURED)

`run_url: https://github.com/thelabcorner/anvil/actions/runs/39669409292` · `job: dense reference
(silesia)` · `workflow: .github/workflows/anvil-i20-dense-reference.yml` · `commit_sha: 6065a36…` ·
`artifact: anvil-i20-dense-reference-silesia-39669409292` · `corpus: silesia`

| Arm | Kind | Portfolio bytes | Ratio | Decode MB/s (survey) | Peak RSS MiB | Ref front |
|---|---|---:|---:|---:|---:|---|
| **ANVIL ratio (candidate)** | — | **46,466,339** | 0.219244363 | — | 254,304 KiB (248.3 MiB) | — |
| xz -9e | binding | 48,456,004 | 0.228632295 | 70.613 | 54.1 | DOMINATED |
| xz -9e (pinned build) | crosscheck | 48,456,004 | 0.228632295 | 75.876 | 54.1 | FRONT |
| xz -9 | dense | 48,795,480 | 0.230234061 | 70.707 | 54.1 | DOMINATED |
| xz -6 | dense | 49,233,340 | 0.232300037 | 70.175 | 10.1 | DOMINATED |
| brotli q11/lw30 (gate) | binding | 49,383,136 | 0.233006827 | 248.300 | 122.3 | DOMINATED |
| brotli q11/lw30 (distro) | crosscheck | 49,383,136 | 0.233006827 | 257.150 | 122.3 | FRONT |
| brotli q11/lw30 | dense | 49,383,136 | 0.233006827 | 255.122 | 122.4 | DOMINATED |
| brotli q11/lw24 | dense | 49,564,563 | 0.233862863 | 267.057 | 82.5 | FRONT |
| brotli q11/lw22 | dense | 50,332,587 | 0.237486667 | 279.759 | 70.8 | FRONT |
| zstd-22 | dense | 52,364,240 | 0.247072713 | 472.953 | 53.0 | FRONT |
| zstd-19 | dense | 52,895,350 | 0.249578675 | 467.249 | 12.3 | DOMINATED |
| brotli q9/lw22 | dense | 56,682,211 | 0.267446404 | 327.110 | 73.1 | DOMINATED |
| brotli q6/lw22 | dense | 58,430,157 | 0.275693821 | 316.594 | 73.2 | DOMINATED |
| zstd-9 | dense | 59,182,179 | 0.279242123 | 533.372 | 8.5 | FRONT |
| brotli q4/lw22 | dense | 64,130,339 | 0.302589264 | 303.634 | 74.8 | DOMINATED |
| zstd-3 | dense | 66,216,569 | 0.312432824 | 521.577 | 6.8 | DOMINATED |
| zstd-1 | dense | 73,276,937 | 0.345746098 | 589.552 | 4.9 | FRONT |
| brotli q1/lw22 | dense | 73,429,961 | 0.346468118 | 258.671 | 77.1 | DOMINATED |
| lz4-9 | dense | 77,992,601 | 0.367996242 | 433.473 | 9.6 | DOMINATED |
| lz4-1 | dense | 100,934,427 | 0.476243764 | 440.793 | 9.7 | DOMINATED |

- (bytes, decode) status: **EXTENDS_FRONT**
- (bytes, peak RSS) status: **EXTENDS_FRONT**
- dense byte bar (`NQ-6`): the best reference point in the dense grid is **`xz -9e` at
  48,456,004 B**; the candidate is **−1,989,665 B**.

## 4. Results — enwik8 (MEASURED)

`run_url: https://github.com/thelabcorner/anvil/actions/runs/39669395005` · `job: dense reference
(enwik8)` · `workflow: .github/workflows/anvil-i20-dense-reference.yml` · `commit_sha: 6065a36…` ·
`artifact: anvil-i20-dense-reference-enwik8-39669395005` · `corpus: enwik8`
(and the earlier, smaller grid in run `39660612517`, artifact
`anvil-i20-dense-reference-enwik8-39660612517`)

| Arm | Kind | Portfolio bytes | Ratio | Decode MB/s (survey) | Peak RSS MiB | Ref front |
|---|---|---:|---:|---:|---:|---|
| **ANVIL ratio (candidate)** | — | **23,537,422** | 0.235374220 | — | 613,468 KiB (599.1 MiB) | — |
| brotli q11/lw30 (gate) | binding | 24,810,180 | 0.248101800 | 178.623 | 251.8 | FRONT |
| brotli q11/lw30 (distro) | crosscheck | 24,810,180 | 0.248101800 | 104.412 | 251.9 | DOMINATED |
| brotli q11/lw30 | dense | 24,810,180 | 0.248101800 | 140.791 | 252.1 | DOMINATED |
| xz -9e | binding | 24,831,648 | 0.248316480 | 105.332 | 66.3 | DOMINATED |
| xz -9e (pinned) | crosscheck | 24,831,648 | 0.248316480 | 95.874 | 66.3 | DOMINATED |
| xz -9 | dense | 24,865,244 | 0.248652440 | 100.435 | 66.3 | DOMINATED |
| zstd-22 | dense | 25,272,471 | 0.252724710 | 615.558 | 99.5 | FRONT |
| brotli q11/lw24 | dense | 25,742,001 | 0.257420010 | 217.747 | 139.9 | DOMINATED |
| xz -6 | dense | 26,375,764 | 0.263757640 | 99.246 | 10.3 | DOMINATED |
| zstd-19 | dense | 26,944,227 | 0.269442270 | 772.352 | 12.5 | FRONT |
| brotli q11/lw22 | dense | 27,045,057 | 0.270450570 | 204.517 | 129.2 | DOMINATED |
| brotli q9/lw22 | dense | 29,473,899 | 0.294738990 | 208.089 | 131.8 | DOMINATED |
| brotli q6/lw22 | dense | 31,003,210 | 0.310032100 | 208.295 | 133.2 | DOMINATED |
| zstd-9 | dense | 31,129,515 | 0.311295150 | 804.653 | 9.1 | FRONT |
| brotli q4/lw22 | dense | 33,507,797 | 0.335077970 | 207.215 | 135.3 | DOMINATED |
| zstd-3 | dense | 35,445,335 | 0.354453350 | 860.560 | 6.5 | FRONT |
| zstd-1 | dense | 40,675,947 | 0.406759470 | 958.354 | 4.9 | FRONT |
| lz4-9 | dense | 42,276,858 | 0.422768580 | 208.643 | 7.7 | DOMINATED |
| lz4-1 | dense | 57,285,984 | 0.572859840 | 222.220 | 9.7 | DOMINATED |

- (bytes, decode) status: **EXTENDS_FRONT**
- (bytes, peak RSS) status: **EXTENDS_FRONT**
- dense byte bar (`NQ-6`): the best reference point is **`brotli q11/lw30` at 24,810,180 B**; the
  candidate is **−1,272,758 B**.

---

## 5. Verdict

**C6 — dense reference-grid control: `WIN` (a control measurement; no frontier token issued).**

Against the pre-registered rule, verbatim:

> **The final candidate is reportable only if it is non-dominated against the dense grid on the
> same axes. If a denser reference point dominates it, the program closes `NO-GO` on the dense
> grid, and this is recorded as the binding result.**

**No denser reference point dominates the candidate on either corpus**, on either the
(bytes ∧ decode) or the (bytes ∧ peak RSS) plane:

* **Silesia:** the candidate is **1,989,665 B below the densest grid's best byte point**
  (`xz -9e`, 48,456,004 B) — no reference configuration is smaller, so none can dominate it on
  any plane. The 21-point grid contains Brotli q1–q11 at windows 22/24/30, `xz -6/-9/-9e`,
  zstd 1/3/9/19/22 and lz4 1/9.
* **enwik8:** the candidate is **1,272,758 B below the densest grid's best byte point**
  (`brotli q11/lw30`, 24,810,180 B).

**The `NQ-6` control requirement is therefore satisfied: the candidate's byte advantage is stated
against the best point in the frozen dense grid, not only against `brotli q11/lw30`.**

### Two secondary findings the control produced (both useful to the programme)

1. **The dense grid identifies the real speed/memory competitors, and they are the zstd tiers.**
   On Silesia `zstd-22` decodes at 472.953 MB/s with a 53.0 MiB peak and `zstd-9` at 533.372 MB/s
   with 8.5 MiB — but at 52,364,240 B and 59,182,179 B they are **5.9 MB and 12.7 MB larger** than
   the candidate, so they do not dominate it. They are, however, the points a *decode-speed* claim
   must be measured against, and the binding gate measures only Brotli and xz.
2. **The Brotli window is a first-class memory axis, and the grid quantifies it.** Brotli
   q11/lw30's Silesia peak is 122.3 MiB; q11/lw24 is 82.5 MiB; q11/lw22 is 70.8 MiB — i.e.
   **the same Brotli quality with a smaller window costs ≈42 % of the peak RSS** while costing
   +949,451 B at lw22. This is the same lever the candidate could use internally, and it is the
   reason the candidate's Brotli-routed `mozilla` file (51,220,480 B) sets the programme's peak
   decode RSS at 2.27–2.40× `xz -9e` regardless of the sub-block cap.

---

## 6. Evidence fields for every number

| # | Claim | run_url | job | workflow | commit_sha | artifact | corpus | label |
|---|---|---|---|---|---|---|---|---|
| E1 | Silesia dense grid (21 arms) + candidate non-dominance | https://github.com/thelabcorner/anvil/actions/runs/39669409292 | dense reference (silesia) | `.github/workflows/anvil-i20-dense-reference.yml` | `6065a36…` | `anvil-i20-dense-reference-silesia-39669409292` | silesia | MEASURED |
| E2 | enwik8 dense grid + candidate non-dominance | https://github.com/thelabcorner/anvil/actions/runs/39669395005 | dense reference (enwik8) | `.github/workflows/anvil-i20-dense-reference.yml` | `6065a36…` | `anvil-i20-dense-reference-enwik8-39669395005` | enwik8 | MEASURED |
| E3 | earlier enwik8 dense grid (smaller) | https://github.com/thelabcorner/anvil/actions/runs/39660612517 | dense reference (enwik8) | `.github/workflows/anvil-i20-dense-reference.yml` | `6065a36…` | `anvil-i20-dense-reference-enwik8-39660612517` | enwik8 | MEASURED |
| E4 | the binding reference-cost baseline the candidate is compared against | https://github.com/thelabcorner/anvil/actions/runs/39654699366 | reference-cost (silesia) / (enwik8) | `.github/workflows/anvil-i10-aux-reference-cost.yml` | `d05fceb…` | `anvil-i10-aux-reference-cost-silesia-39654699366`, `...-enwik8-...` | silesia, enwik8 | MEASURED (quoted) |

---

## 7. Next steps / blockers

* The control has done its job: **the candidate's byte advantage is not a sparse-grid artifact**,
  and the programme's remaining deficits are decode cost and peak RSS — not bytes, and not a
  mis-specified reference set.
* The control's Brotli-window row (q11/lw22 at 70.8 MiB vs q11/lw30 at 122.3 MiB) is the
  quantitative basis for the coordinator's note that the *candidate's own* Brotli backend window is a
  candidate-side memory lever; it is recorded here as a measured reference-grid fact, not as a
  candidate result.
* No blockers. The grid is 21 points per corpus and covers every family the programme's prereg
  named (Brotli quality/window tiers, zstd regular + high-compression tiers, xz presets, a fast LZ
  anchor). The remaining `GRID-THIN` caveat is limited to zstd tiers between 19 and 22 and to
  Brotli windows between 24 and 30, and both are reported.
