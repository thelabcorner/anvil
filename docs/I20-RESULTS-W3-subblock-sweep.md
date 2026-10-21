# I20 / C3 — BWT subblock-cap sweep (results)

**Worker:** W3 · **Candidate:** C3 (prereg §5.1) · **Date:** 2026-10-21
**Branch:** `research/anvil-i20-w3-subblock-sweep` · **Base:** `d05fceb` (I20 preregistration)
**Status of this document:** measurement report for the I20 coordinator. Every number carries
its evidence fields (§6). No number in this document was produced on a developer machine.

---

## 1. Hypothesis, the premise that reopens the subblock question, and the decision rule

### 1.1 Hypothesis

The canonical ANVIL ratio route's **peak decode RSS** is dominated by the inverse-BWT
working set: `libsais` needs an `int32` array of `n+1` entries (4n bytes) plus the BWT
string (n bytes) plus the reconstructed output (n bytes) — roughly 6n bytes live for a
whole-file BWT. The revision-2 outer `0xFF` BWT-subblock frame caps that per-piece working
set, and `--bwt-subblock=N` (bytes, default 128 MiB = effectively uncapped for every
Silesia file) already exists as a CLI flag. A smaller cap should cut peak decode RSS by a
large factor, at a charged byte cost (each sub-block re-pays the BWT warm-up and loses
cross-piece context).

### 1.2 The new premise (this is *not* a re-litigation of the prior default)

The prior sweep (I10-1A.2, run `35930672607`) kept the 128 MiB cap because the then-current
objective was the **max-ratio / size-first default** (`docs/I10-AUX-UNBWT-RESULTS.md` §11:
*"No smaller cap dominates the 128 MiB max-ratio point across the frozen corpora. The default
therefore remains unchanged."*). That objective is **not** the I20 objective.

The I20 objective is **closing the external reference-cost gate**
(`tools/reference_cost_gate.py`), in which **peak decode RSS is a hard gate** (G5: peak decode
RSS ratio ≤ 2.0 vs `xz -9e`; G6: ≤ 2.0 vs Brotli q11/lgwin30). Under that objective the
question *"which cap meets the RSS gate at an acceptable byte cost?"* is a **new, narrower,
engineering** question, opened by the preregistration in §3 fact 2 (*"It was swept at
8/16/32/64/128 MiB in run `35930672607`; **4 MiB and below were never measured**"*) and
§5.1 C3, which explicitly reopens fixed-cap BWT subblocking **as engineering only** with a
**new stated premise and a new decision rule** (`FROZEN-CLOSEOUT-MATRIX-R2.md` §2.2 as cited
in prereg §5.2). No novelty claim is made; nothing in prereg §5.2 is re-litigated.

### 1.3 Pre-registered decision rule (quoted verbatim from prereg §5.1, C3)

> **Adopt the smallest cap whose charged bytes keep the Silesia portfolio total below `xz -9e`
> and whose measured peak decode RSS ratio vs `xz` is ≤ 2.0.** If no cap ≤ 8 MiB simultaneously
> satisfies both, C3 is `NO-GO` on Silesia (it may still be reported as a measured rate/memory
> trade).

### 1.4 The prior sweep this run extends (run `35930672607`, artifact
`anvil-i10-bwt-subblock-silesia-35930672607` / `...-enwik8-...`)

| Cap | Silesia portfolio bytes | Δ vs 128 MiB | Silesia peak decode RSS | enwik8 portfolio bytes | enwik8 peak decode RSS |
|---:|---:|---:|---:|---:|---:|
| 8 MiB | 47,365,274 | +898,935 | 124.4 MiB | 24,810,212 | 251.5 MiB |
| 16 MiB | 46,938,837 | +472,498 | 125.7 MiB | 24,810,212 | 251.5 MiB |
| 32 MiB | 46,648,642 | +182,303 | 203.4 MiB | 24,810,212 | 251.5 MiB |
| 64 MiB | 46,466,339 | +0 | 248.4 MiB | 24,296,722 | 411.4 MiB |
| 128 MiB (control) | 46,466,339 | +0 | 248.4 MiB | 23,537,422 | 598.9 MiB |

Caps of **4 MiB and below were never measured** — that is the gap this report fills.

---

## 2. Method

### 2.1 Tool and workflow

* Tool: `tools/i20_subblock_sweep.py` (new, this branch). It does **not** mutate codec
  behaviour; it drives the existing `--bwt-subblock` flag.
* Workflow: `.github/workflows/anvil-i20-subblock-sweep.yml` (new, this branch; `workflow_dispatch`
  only, `permissions: contents: read`, `cancel-in-progress: false`, finite `timeout-minutes: 240`,
  artifact upload `if: always()`).
* The workflow asserts, in its provenance step, that the built candidate's `src/anvil.cpp`
  git blob is the **frozen I10-1A baseline blob** `755df76ae53795aea040f4c99784d1effb43b3ab`
  (identical to the program base commit's blob), so all numbers are on the **unmodified
  baseline binary** and compose cleanly with any zero-byte-cost lever measured on the same
  binary. The assertion is a hard step failure if the blob differs.

### 2.2 What the tool does

1. **prepare** — for every corpus file, encodes the canonical ratio payloads:
   * ANVIL ratio-mode **Brotli backend** payload (cap-independent),
   * ANVIL ratio-mode **BWT backend** payload with `--bwt-aux=on --bwt-subblock=<cap bytes>`
     for every cap (a cap larger than the file yields the identical *unsplit* payload and is
     encoded once, exactly as the frozen sibling tool `tools/bwt_subblock_sweep.py` does),
   * the two **binding references**: `xz -9e` and Brotli q11/lgwin30 via the repo's pinned
     `brotli_lw` helper (`tools/brotli_lw.cpp`).
   The canonical `--ratio-backend=auto` portfolio is then reconstructed **exactly** the way
   `encode_ratio_block()` does it (`src/anvil.cpp:4614`): the Brotli payload is considered
   first and is replaced by the BWT payload only on a **strict** size improvement, so Brotli
   wins ties. The reconstruction is gated by a hard byte-identity assertion: the 128 MiB
   control portfolio must equal the frozen canonical total (46,466,339 B Silesia /
   23,537,422 B enwik8) or the run fails. Every arm is round-tripped and checked
   byte-exactly (decoded SHA-256 == source SHA-256) at prepare time.
2. **rss** — per-file peak decode RSS for **every arm** (every cap's selected portfolio, plus
   both references) via `/usr/bin/time -f "%e %M"`, i.e. per-process maximum resident set size,
   the same instrument the prior sweep used.
3. **timing** — same-job paired/interleaved whole-codec decode timing through the repo's
   `tools/paired_bench.py`, with the preregistered parameters (§2.4).
4. **verify** — after the timed pass, every arm is decoded again (SHA-256 vs source) and
   **every compressed payload is re-hashed** against its prepare-time hash, proving the
   compressed bytes are identical across repetitions and untouched by the timing run.
5. **summarize** — per-cap table, gate verdicts, and the ruling token produced by the repo's
   own frozen arbiter `tools/reference_cost_gate.py summarize` (invoked once per timed cap
   with a synthesized `anvil-aux` arm, so the ruling is emitted by the repo, not re-derived).

### 2.3 Measurement locus and provenance

All measurement is on GitHub Actions `thelabcorner/anvil` (runs in §6). No local build of
`anvil`, `xz` or `brotli` was run against any corpus file; local activity was editing,
static source reading, exact arithmetic over downloaded artifacts, and git/gh operations.
Every claim in this document is `MEASURED` or `DERIVED` as labelled; nothing is `PROJECTION`.

### 2.4 Timing protocol (prereg §4.4, binding)

`tools/paired_bench.py` with `--reps 9 --warmups 2 --seed 41246 --bootstrap 20000
--epsilon 0.02 --max-cv 0.15 --cpu 0 --ambient-reps 5 --ambient-iterations 250000
--ambient-max-cv 0.10`, single-threaded arms, affinity pinned to CPU 0, **all raw repetitions
retained** in `paired-*.csv`, **no outlier deletion**, and a mandatory **A/A null pair**.
Both binding references were measured **in the same job** as the candidate arms, which is
what G3/G4 read.

### 2.5 The prior sweep's enwik8 timing rule (honoured)

`tools/bwt_subblock_sweep.py::summarize` propagates a single invalid pair to the **whole**
corpus-level timing classification: `all_timing_valid` is the AND over every row, and the
Pareto column is rendered `BLOCKED` (not "inferred") when it is false. In the prior run
`35930672607` the enwik8 sweep is `BLOCKED` in **every** row because exactly one pair,
`paired-cap-64.json`, was `BLOCKED_AMBIENT` (ambient robust CV > 0.10) — this was read
directly from that artifact. This report honours the same rule: **a pair that is not
timing-valid is reported as `INCONCLUSIVE`/neutral for its gate and is never reinterpreted
as a failure, and the corpus-level classification is `BLOCKED` if any pair in the series is
invalid.** A blocked series must be re-run, not explained away (prereg §4.4).

---

## 3. Results

### 3.1 Silesia — charged bytes, routing and remaining surplus

`run_url: https://github.com/thelabcorner/anvil/actions/runs/39656702596` ·
`job: sweep` · `workflow: .github/workflows/anvil-i20-subblock-sweep.yml` ·
`commit_sha: ab88e4aa612696e414050bfdd718ba264a382452` ·
`artifact: anvil-i20-subblock-silesia-39656702596` · `corpus: silesia` · **MEASURED**

| Cap | Portfolio bytes | Δ vs 128 MiB control | Δ vs `xz -9e` (48,456,004 B) | **Remaining surplus vs xz** | BWT routes | Brotli routes | Routing changes vs 128 MiB |
|---:|---:|---:|---:|---:|---:|---:|---|
| 1 MiB | 48,673,521 | +2,207,182 | **+217,517** | **−217,517** | 2 | 10 | dickens, nci, osdb, reymont, webster: bwt→brotli |
| 2 MiB | 48,347,006 | +1,880,667 | −108,998 | **+108,998** | 5 | 7 | nci, webster: bwt→brotli |
| 4 MiB | 47,941,299 | +1,474,960 | −514,705 | **+514,705** | 6 | 6 | nci: bwt→brotli |
| 8 MiB | 47,365,274 | +898,935 | −1,090,730 | **+1,090,730** | 6 | 6 | nci: bwt→brotli |
| 16 MiB | 46,938,837 | +472,498 | −1,517,167 | +1,517,167 | 7 | 5 | none |
| 32 MiB | 46,648,642 | +182,303 | −1,807,362 | +1,807,362 | 7 | 5 | none |
| 64 MiB | 46,466,339 | +0 | −1,989,665 | +1,989,665 | 7 | 5 | none |
| 128 MiB (control) | 46,466,339 | +0 | −1,989,665 | +1,989,665 | 7 | 5 | none |

**Routing is a first-class result, not a hidden detail:** the cap does not merely change the
BWT payload, it changes *which backend the router picks*. `nci` leaves the BWT route already
at 4 MiB (BWT payload 1,561,771 B → 1,369,810 B at 16 MiB but 1,561,771 B > Brotli's
1,497,442 B once the payload is split), `webster` and `dickens`/`osdb`/`reymont` follow at
1–2 MiB, and at 1 MiB only **2 of 12** files remain BWT-routed. The Brotli-routed files are
then decoded by ANVIL's Brotli backend, whose peak RSS is **cap-independent** — this is the
structural reason the portfolio RSS floor does not fall (see §3.3).

### 3.2 enwik8 — charged bytes, routing and remaining surplus

`run_url: https://github.com/thelabcorner/anvil/actions/runs/39656707628` ·
`job: sweep` · `workflow: .github/workflows/anvil-i20-subblock-sweep.yml` ·
`commit_sha: ab88e4aa612696e414050bfdd718ba264a382452` ·
`artifact: anvil-i20-subblock-enwik8-39656707628` · `corpus: enwik8` · **MEASURED**

| Cap | Portfolio bytes | Δ vs 128 MiB control | Δ vs `xz -9e` (24,831,648 B) | Remaining surplus vs xz | Route | BWT payload bytes (not selected) | BWT sub-blocks |
|---:|---:|---:|---:|---:|---|---:|---:|
| 1 MiB | 24,810,212 | +1,272,790 | −21,436 | **+21,436** | brotli | 30,306,660 | 96 |
| 2 MiB | 24,810,212 | +1,272,790 | −21,436 | +21,436 | brotli | 28,832,135 | 48 |
| 4 MiB | 24,810,212 | +1,272,790 | −21,436 | +21,436 | brotli | 27,649,939 | 24 |
| 8 MiB | 24,810,212 | +1,272,790 | −21,436 | +21,436 | brotli | 26,637,618 | 12 |
| 16 MiB | 24,810,212 | +1,272,790 | −21,436 | +21,436 | brotli | 25,703,416 | 6 |
| 32 MiB | 24,810,212 | +1,272,790 | −21,436 | +21,436 | brotli | 24,857,914 | 3 |
| 64 MiB | 24,296,722 | +759,300 | −534,926 | +534,926 | **bwt** | 24,296,722 | 2 |
| 128 MiB (control) | 23,537,422 | +0 | −1,294,226 | +1,294,226 | **bwt** | 23,537,422 | 1 |

On enwik8 the cap flips the **whole** file to the Brotli route at ≤ 32 MiB. That is still
below `xz -9e` (by only 21,436 B) but leaves essentially no byte budget for any other lever,
and the ANVIL arm then *is* a Brotli payload, so the G4 comparison against the Brotli
reference becomes a comparison of ANVIL's Brotli-backend decode against the pinned
`brotli_lw` reference.

### 3.3 Peak decode RSS — per file, and the portfolio maximum

Silesia (`run 39656702596`, `artifact anvil-i20-subblock-silesia-39656702596`, **MEASURED**,
KiB, per-process `/usr/bin/time -f %M`):

| File | Input B | xz -9e | Brotli q11 | 1 MiB | 2 MiB | 4 MiB | 8 MiB | 16 MiB | 32 MiB | 64 MiB | 128 MiB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| dickens | 10,192,446 | 12,136 | 26,248 | 26,620 ᵇ | 39,224 | 51,424 | 56,088 | 66,448 | 66,528 | 66,496 | 66,504 |
| **mozilla** | 51,220,480 | 52,212 | 125,020 | **125,416 ᵇ** | **125,572 ᵇ** | **125,548 ᵇ** | **125,564 ᵇ** | 125,600 ᵇ | 125,576 ᵇ | 125,556 ᵇ | 125,560 ᵇ |
| mr | 9,970,564 | 11,892 | 25,904 | 32,448 | 38,444 | 50,728 | 55,716 | 64,944 | 64,796 | 64,932 | 64,980 |
| nci | 33,553,445 | 34,984 | 70,540 | 70,832 ᵇ | 70,836 ᵇ | 70,856 ᵇ | 70,880 ᵇ | 120,496 | 202,200 | 202,096 | 202,180 |
| ooffice | 6,152,192 | 8,172 | 18,352 | 18,760 ᵇ | 18,660 ᵇ | 18,748 ᵇ | 18,756 ᵇ | 18,752 ᵇ | 18,704 ᵇ | 18,652 ᵇ | 18,688 ᵇ |
| osdb | 10,085,684 | 12,020 | 26,028 | 26,328 ᵇ | 39,012 | 51,256 | 56,108 | 66,008 | 65,848 | 65,972 | 65,996 |
| reymont | 6,627,202 | 8,564 | 17,712 | 18,208 ᵇ | 30,820 | 32,700 | 44,304 | 44,336 | 44,312 | 44,340 | 44,288 |
| samba | 21,606,400 | 23,280 | 61,184 | 61,596 ᵇ | 61,580 ᵇ | 61,620 ᵇ | 61,544 ᵇ | 61,600 ᵇ | 61,640 ᵇ | 61,636 ᵇ | 61,508 ᵇ |
| sao | 7,251,944 | 9,172 | 22,156 | 22,432 ᵇ | 22,500 ᵇ | 22,492 ᵇ | 22,444 ᵇ | 22,404 ᵇ | 22,488 ᵇ | 22,480 ᵇ | 22,516 ᵇ |
| **webster** | 41,458,703 | 42,608 | 117,800 | 118,228 ᵇ | 118,200 ᵇ | 117,684 | 109,440 | 126,524 | 208,280 | 254,284 | 254,304 |
| x-ray | 8,474,240 | 10,484 | 24,828 | 31,100 | 37,016 | 49,284 | 57,404 | 57,680 | 57,656 | 57,620 | 57,648 |
| xml | 5,345,280 | 7,356 | 14,460 | 14,876 ᵇ | 14,944 ᵇ | 14,908 ᵇ | 14,928 ᵇ | 14,940 ᵇ | 14,924 ᵇ | 14,932 ᵇ | 14,920 ᵇ |
| **portfolio max** | | **52,212** | **125,020** | **125,416** | **125,572** | **125,548** | **125,564** | **126,524** | **208,280** | **254,284** | **254,304** |

ᵇ = the router selected the **Brotli** backend for that file at that cap (the peak RSS is
then the cap-independent Brotli-path cost, not a BWT cost).

**The single most important structural fact in this report:** the portfolio maximum peak
decode RSS is set by **`mozilla`**, which the router keeps on the **Brotli** route at *every*
cap (its split-BWT payload is larger than its Brotli payload at every cap: 17,773,492 B at
8 MiB vs 13,806,173 B for Brotli, and the split payload only grows as the cap shrinks).
`mozilla`'s ANVIL decode RSS is 125,416–125,600 KiB at every cap, so **the portfolio RSS
floor is 122.5 MiB no matter how small the cap is**. The floor is 2.40× the in-job `xz -9e`
peak (52,212 KiB = 51.0 MiB), i.e. **G5 cannot be met by the sub-block cap alone at any
cap**.

The largest BWT-routed file, **`webster` (41,458,703 B)**, *does* fall below the G5 budget
at a 8 MiB cap (109,440 KiB = 106.9 MiB, versus a 2.0× xz budget of 104,424 KiB — close but
still above it) and at a 4 MiB cap (117,684 KiB = 114.9 MiB — still above the budget, because
the *whole reconstructed output* 41.5 MB plus the compressed input is live regardless of the
inverse-BWT piece size). `nci` (33,553,445 B) is 70,856 KiB at 4–8 MiB (Brotli route) and
202,180 KiB at 128 MiB (BWT route). `osdb` (10,085,684 B) scales with the cap as expected:
26,328 / 39,012 / 51,256 / 56,108 / 66,008 KiB at 1 / 2 / 4 / 8 / 16 MiB.

The per-file RSS of the BWT path therefore has **two** terms, not one: a cap-proportional
inverse-BWT term (≈ 6 × cap, as hypothesised) and a file-proportional term (the whole
reconstructed output + the compressed input), which is why a smaller cap cannot push the
largest file below the G5 budget.

### 3.4 Gate verdicts — Silesia

`run 39656702596` (bytes/RSS) and `39663273951` (timed) · **MEASURED / DERIVED**

| Cap | G2 bytes < xz | G5 RSS ≤ 2.0× xz | G6 RSS ≤ 2.0× Brotli | peak RSS | RSS/xz | RSS/Brotli | G3 decode CI-upper vs xz | G4 decode CI-upper vs Brotli | Arbiter ruling |
|---:|---|---|---|---:|---:|---:|---|---|---|
| 1 MiB | **FAIL** (+217,517 B over xz) | **FAIL** | PASS | 122.5 MiB | 2.40× | 1.00× | *see §3.7* | *see §3.7* | *see §3.7* |
| 2 MiB | PASS | **FAIL** | PASS | 122.6 MiB | 2.41× | 1.00× | *see §3.7* | *see §3.7* | *see §3.7* |
| 4 MiB | PASS | **FAIL** | PASS | 122.6 MiB | 2.40× | 1.00× | *see §3.7* | *see §3.7* | *see §3.7* |
| 8 MiB | PASS | **FAIL** | PASS | 122.6 MiB | 2.40× | 1.00× | *see §3.7* | *see §3.7* | *see §3.7* |
| 16 MiB | PASS | **FAIL** | PASS | 123.6 MiB | 2.42× | 1.01× | not timed | not timed | not timed |
| 32 MiB | PASS | **FAIL** | PASS | 203.4 MiB | 3.99× | 1.67× | not timed | not timed | not timed |
| 64 MiB | PASS | **FAIL** | **FAIL** | 248.3 MiB | 4.87× | 2.03× | not timed | not timed | not timed |
| 128 MiB (control) | PASS | **FAIL** | **FAIL** | 248.3 MiB | 4.87× | 2.03× | *see §3.7* | *see §3.7* | *see §3.7* |

### 3.5 Reproducibility cross-check against the prior sweep (run `35930672607`)

| Quantity | Prior run `35930672607` | This run | Verdict |
|---|---:|---:|---|
| Silesia 8 MiB portfolio bytes | 47,365,274 | **47,365,274** | **EXACT MATCH** |
| Silesia 16 MiB portfolio bytes | 46,938,837 | **46,938,837** | **EXACT MATCH** |
| Silesia 32 MiB portfolio bytes | 46,648,642 | **46,648,642** | **EXACT MATCH** |
| Silesia 64/128 MiB portfolio bytes | 46,466,339 | **46,466,339** | **EXACT MATCH** |
| Silesia 8 MiB peak decode RSS | 124.4 MiB | 122.6 MiB | −1.4 % (runner RSS granularity) |
| Silesia 128 MiB peak decode RSS | 248.4 MiB | 248.3 MiB | −0.04 % |
| enwik8 8 MiB portfolio bytes | 24,810,212 | **24,810,212** | **EXACT MATCH** |
| enwik8 64 MiB portfolio bytes (+3.2259 %) | 24,296,722 | **24,296,722** | **EXACT MATCH** |
| enwik8 64 MiB peak decode RSS | 411.4 MiB | **411.4 MiB** | **EXACT MATCH** |
| enwik8 128 MiB peak decode RSS | 598.9 MiB | **598.9 MiB** | **EXACT MATCH** |

Every byte total reproduces **exactly**; the RSS values reproduce to ≤ 1.4 % (per-process RSS
is runner-sensitive; the byte totals are deterministic and match bit-for-bit). **There is no
disagreement to investigate.**

### 3.6 The auxiliary-index interaction with the sub-block cap (measured, not assumed)

**Yes, the cap changes the per-sub-block auxiliary-index policy.** `ratio_backend_encode()`
splits the BWT input into pieces and calls `bwt_backend_encode()` per piece
(`src/anvil.cpp:4446-4457`); each piece therefore gets **its own** auxiliary index with its
own sampling rate `r = bwt_aux_rate_for_size(piece_size)` (`src/anvil.cpp:4247`, target 1024
walks/piece). The index bytes per cap are **DERIVED** exactly from that policy
(`r = max(2, next_pow2(ceil(m/1024)))`, `index bytes = 4·(1 + (m−1)//r)` per piece):

| File | n (B) | 128 MiB | 8 MiB | 4 MiB | 2 MiB | 1 MiB | index growth at 1 MiB | measured BWT payload growth at 1 MiB | index share |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| webster | 41,458,703 | 2,532 B | 20,244 B | 40,488 B | 80,976 B | 161,952 B | +159,420 B | +1,829,334 B | **8.7 %** |
| mozilla | 51,220,480 | 3,128 B | 28,048 B | 52,624 B | 101,776 B | 200,080 B | +196,952 B | +727,929 B | 27.1 % |
| nci | 33,553,445 | 4,096 B | 16,384 B | 32,768 B | 65,536 B | 131,072 B | +126,976 B | +668,964 B | 19.0 % |
| enwik8 | 100,000,000 | 3,052 B | 48,832 B | 97,660 B | 195,316 B | 392,132 B | +389,080 B | +6,769,238 B | **5.7 %** |

So the index growth is real and grows linearly in the number of pieces (enwik8: 1 piece →
3,052 B, 96 pieces → 392,132 B), but on the large files it is a **minority** of the byte
cost: 5.7 % (enwik8) and 8.7 % (webster) of the measured BWT payload growth at a 1 MiB cap.
The **dominant** cost of the cap is the loss of cross-piece BWT context (the remaining
94.3 % / 91.3 %). On the small files the index share is large (up to ~60-98 %, e.g. `sao` at
2 MiB: 12,496 B of index against a 12,771 B payload growth) but the absolute amounts are
tiny (≤ 25 KB).

### 3.7 Decode time - Silesia (same-job paired, prereg parameters)

`run_url: https://github.com/thelabcorner/anvil/actions/runs/39663273951` · `job: sweep` ·
`workflow: .github/workflows/anvil-i20-subblock-sweep.yml` · `commit_sha: 2662c04…` ·
`artifact: anvil-i20-subblock-silesia-39663273951` · `corpus: silesia` · **MEASURED**

"control" is the 128 MiB-cap arm of the same job (the uncapped canonical route). The
candidate column is the whole-Silesia-portfolio decode time.

| Cap | candidate portfolio decode (s) | vs 128 MiB control (CI95) | speed-up vs control | vs `xz -9e` (CI95) | vs Brotli q11/lw30 (CI95) | G2 bytes<xz | G3 | G4 | G5 RSS/xz |
|---:|---:|---|---:|---|---|---|---|---|---|
| 1 MiB | 2.4733 | 0.4431 [0.4368, 0.4504] | **2.26× faster** | **0.8049 [0.7889, 0.8215]** | **1.5258 [1.4649, 1.6260]** | **FAIL** (+217,517 B) | PASS | **PASS** | FAIL (2.40×) |
| 2 MiB | 3.4257 | 0.6185 [0.6121, 0.6230] | 1.62× faster | 1.1337 [1.1172, 1.1650] | **2.2104 [2.1108, 2.3185]** | PASS (+108,998 B left) | PASS | **FAIL by ≈10 %** | FAIL (2.41×) |
| 4 MiB | 4.7795 | 0.8591 [0.8486, 0.8817] | 1.16× faster | 1.6071 [1.5648, 1.6361] | 3.0388 [2.9125, 3.0751] | PASS (+514,705 B) | PASS | FAIL | FAIL (2.40×) |
| 8 MiB | 4.7287 | 0.8792 [0.8693, 0.8956] | 1.14× faster | 1.6467 [1.6183, 1.6469] | 3.0437 [2.8836, 3.1116] | PASS (+1,090,730 B) | PASS | FAIL | FAIL (2.40×) |
| 128 MiB (control) | 5.3309 | 1.0 (self) | 1.0 | 1.8761 [1.8385, 1.9071] | 3.5013 [3.3915, 3.5396] | PASS (+1,989,665 B) | PASS | FAIL | FAIL (4.87×) |

All series report `timing_valid: true`; the A/A null CI is [0.9999, 1.0000] (§2.5).

**The cap is a strong decode-speed lever, and the knee is the 2 MiB cap.** The mechanism is
cache residency: the inverse BWT performs `n/2` *dependent* random accesses into the LF array,
and a smaller cap makes that array `4C` bytes instead of `4n` bytes. The 1 MiB cap is
**2.26× faster** than the uncapped route (2.4733 s vs 5.3309 s) and is the only measured cap
whose decode gates both pass (G3 0.8049 and G4 1.5258 versus `xz -9e` and Brotli) — but it is
**+217,517 B over `xz -9e` (G2 FAIL)**. The 2 MiB cap is the byte-feasible knee (108,998 B of
surplus left) and misses G4 by ≈10 %.

### 3.8 Decode time and gates - enwik8

`run_url: https://github.com/thelabcorner/anvil/actions/runs/39663277790` · `job: sweep` ·
`workflow: .github/workflows/anvil-i20-subblock-sweep.yml` · `commit_sha: 2662c04…` ·
`artifact: anvil-i20-subblock-enwik8-39663277790` · `corpus: enwik8` · **MEASURED**

| Cap | candidate decode (s) | vs 128 MiB control (CI95) | vs `xz -9e` (CI95) | vs Brotli q11/lw30 (CI95) | G2 bytes<xz | G3 | G4 | G5 RSS/xz |
|---:|---:|---|---|---|---|---|---|---|
| 1 MiB | 0.4216 | 0.0986 [0.0985, 0.0986] (**10.1× faster**) | 0.3329 [0.3327, 0.3581] | 1.0150 [1.0141, 1.0156] | PASS (+21,436 B) | PASS | **PASS** | FAIL (3.803×) |
| 8 MiB | 0.4217 | 0.0975 [0.0975, 0.1090] (**10.3× faster**) | 0.3327 [0.3325, 0.3331] | 1.0146 [1.0133, 1.1362] | PASS (+21,436 B) | PASS | **PASS** | FAIL (3.804×) |
| 64 MiB | 4.3273 | 1.0119 [1.0118, 1.0229] | 3.4544 [3.3252, 3.4555] | 10.5333 [10.4196, 10.5420] | PASS (+534,926 B) | FAIL | FAIL | FAIL (6.223×) |
| 128 MiB (control) | 4.2765 | 1.0 (self) | 3.3730 [3.2466, 3.3772] | 10.2852 [10.2656, 10.3087] | PASS (+1,294,226 B) | FAIL | FAIL | FAIL (9.059×) |

On enwik8 the 1/8 MiB speed-up is **10.1×**, but it is a *routing* effect rather than a
cache effect: at those caps the router selects the **Brotli** backend for the whole file, so the
candidate decodes at Brotli speed. The bytes then sit 21,436 B below `xz -9e` (essentially no
budget) and the peak RSS is Brotli's 251.4 MiB = 3.803× `xz -9e`. **enwik8 has no cap that
satisfies G2–G6 simultaneously**, for two independent reasons: the caps that bound the working
set (≤32 MiB) leave the BWT route, and the caps that keep the BWT route (≥64 MiB) leave the
peak RSS at 6.223–9.059× `xz -9e`.

---

## 4. Remaining byte surplus (for the coordinator's combined arm)

Silesia surplus vs `xz -9e` (48,456,004 B) at each cap, and what is left after subtracting
the ≈ 430,000 B that the auxiliary-index walk-budget lever (W2) is expected to charge over
the 7 BWT-routed files at W = 16384:

| Cap | Cap in BYTES (`--bwt-subblock=`) | Portfolio bytes | Surplus vs xz | Surplus **after** −430,000 B (W2's lever) | RSS/xz | RSS/Brotli | G2 / G5 / G6 | Room for W2's lever? |
|---:|---:|---:|---:|---:|---:|---:|---|---|
| 1 MiB | 1048576 | 48,673,521 | −217,517 | −647,517 | 2.40× | 1.00× | FAIL / FAIL / PASS | **no** (over xz before W2) |
| 2 MiB | 2097152 | 48,347,006 | +108,998 | −321,002 | 2.41× | 1.00× | PASS / FAIL / PASS | **no** |
| 4 MiB | 4194304 | 47,941,299 | +514,705 | +84,705 | 2.40× | 1.00× | PASS / FAIL / PASS | barely (84,705 B left) |
| 8 MiB | 8388608 | 47,365,274 | +1,090,730 | +660,730 | 2.40× | 1.00× | PASS / FAIL / PASS | **yes** (660,730 B left) |
| 16 MiB | 16777216 | 46,938,837 | +1,517,167 | +1,087,167 | 2.42× | 1.01× | PASS / FAIL / PASS | yes |
| 32 MiB | 33554432 | 46,648,642 | +1,807,362 | +1,377,362 | 3.99× | 1.67× | PASS / FAIL / PASS | yes |
| 64 MiB | 67108864 | 46,466,339 | +1,989,665 | +1,559,665 | 4.87× | 2.03× | PASS / FAIL / FAIL | yes |
| 128 MiB (control) | 134217728 | 46,466,339 | +1,989,665 | +1,559,665 | 4.87× | 2.03× | PASS / FAIL / FAIL | yes |

enwik8 surplus vs `xz -9e` (24,831,648 B):

| Cap | Portfolio bytes | Surplus vs xz | Surplus after −430,000 B | RSS/xz | RSS/Brotli | G2 / G5 / G6 | Room for W2's lever? |
|---:|---:|---:|---:|---:|---:|---|---|
| 1–32 MiB | 24,810,212 | +21,436 | −408,564 | 3.80× | 1.00× | PASS / FAIL / PASS | **no** |
| 64 MiB | 24,296,722 | +534,926 | +104,926 | 6.22× | 1.63× | PASS / FAIL / PASS | barely |
| 128 MiB (control) | 23,537,422 | +1,294,226 | +864,226 | 9.06× | 2.38× | PASS / FAIL / FAIL | yes |

---

## 5. Verdict

**C3 — BWT sub-block cap: `NO-GO` on the gate's memory axis (Silesia and enwik8), and a
measured, adoptable decode-speed trade.**

Pre-registered rule, quoted verbatim:

> **Adopt the smallest cap whose charged bytes keep the Silesia portfolio total below `xz -9e`
> and whose measured peak decode RSS ratio vs `xz` is <= 2.0.** If no cap <= 8 MiB simultaneously
> satisfies both, C3 is `NO-GO` on Silesia (it may still be reported as a measured rate/memory
> trade).

**Falsifying reason, in one sentence: *no sub-block cap meets G5 at any cap, because the
portfolio maximum peak decode RSS is set by `mozilla` (51,220,480 B), which the router keeps on
the Brotli route at every cap because its split-BWT payload always exceeds its Brotli payload
(17,773,492 B at the 8 MiB cap vs 13,806,173 B) and only grows as the cap shrinks — so the
portfolio RSS floor is 122.5 MiB = 2.40× the in-job `xz -9e` peak (52,212 KiB), and the
cap-independent Brotli-path cost cannot be reduced by any cap.***

The rule is therefore **not satisfied at any cap** and C3 is NO-GO on the gate's memory axis on
both corpora. The measured trade is nevertheless real and must be recorded as such:

- **Decode speed (the positive result):** the cap is a strong decode-speed lever — 2.26× faster
  portfolio decode at 1 MiB (2.4733 s vs 5.3309 s) and 1.62× at 2 MiB on Silesia, and 10.1× on
  enwik8 (a routing effect there, not a cache effect). The mechanism is cache residency of the
  inverse BWT's `n/2` dependent random accesses into a `4C`-byte LF array instead of a `4n`-byte
  one.
- **Bytes (the cost):** the cap is chargeable and the 1 MiB point is **out of budget**
  (+217,517 B over `xz -9e` → G2 FAIL). The byte-optimal byte-feasible operating points are
  **8 MiB** (47,365,274 B; 1,090,730 B of surplus left) and **16 MiB** (46,938,837 B;
  1,517,167 B left); the decode-optimal byte-feasible point is **2 MiB** (48,347,006 B;
  108,998 B left), whose decode gates miss G4 by ≈10 %.
- **Reproduction:** the byte side matches the prior sweep `35930672607` **exactly** at every cap.

**What C3 does and does not authorise:** it authorises a rate/memory operating point on the
canonical ratio route (and it is the only lever measured in this programme that improves decode
throughput), and it is the constraint that any later arm must respect — the byte budget at the
decode-optimal cap is nearly exhausted. It does **not** authorise a frontier claim: G5 is
unreachable at every cap, and the gate ruling stays `FRONT-GAP_COST`.

---

## 6. Evidence fields for every number

| # | Claim | run_url | job | workflow | commit_sha | artifact | corpus | label |
|---|---|---|---|---|---|---|---|---|
| E1 | Silesia per-cap bytes / routing / RSS (all 8 caps) | https://github.com/thelabcorner/anvil/actions/runs/39656702596 | sweep | `.github/workflows/anvil-i20-subblock-sweep.yml` | `ab88e4aa612696e414050bfdd718ba264a382452` | `anvil-i20-subblock-silesia-39656702596` | silesia | MEASURED |
| E2 | enwik8 per-cap bytes / routing / RSS (all 8 caps) | https://github.com/thelabcorner/anvil/actions/runs/39656707628 | sweep | `.github/workflows/anvil-i20-subblock-sweep.yml` | `ab88e4aa612696e414050bfdd718ba264a382452` | `anvil-i20-subblock-enwik8-39656707628` | enwik8 | MEASURED |
| E3 | Silesia paired decode timing (caps 1/2/4/8 + control vs xz/Brotli) | https://github.com/thelabcorner/anvil/actions/runs/39663273951 | sweep | `.github/workflows/anvil-i20-subblock-sweep.yml` | `2662c04…` | `anvil-i20-subblock-silesia-39663273951` | silesia | MEASURED |
| E4 | enwik8 paired decode timing (caps 1/8/64 + control vs xz/Brotli) | https://github.com/thelabcorner/anvil/actions/runs/39663277790 | sweep | `.github/workflows/anvil-i20-subblock-sweep.yml` | `2662c04…` | `anvil-i20-subblock-enwik8-39663277790` | enwik8 | MEASURED |
| E5 | Prior-sweep numbers being extended | https://github.com/thelabcorner/anvil/actions/runs/35930672607 | frontier (silesia/enwik8) | `.github/workflows/anvil-i10-bwt-subblock-frontier.yml` | `2701fa746c7aae0c7f31bff2c0637fb0332a914a` | `anvil-i10-bwt-subblock-silesia-35930672607`, `anvil-i10-bwt-subblock-enwik8-35930672607` | silesia, enwik8 | MEASURED |
| E6 | Auxiliary-index byte model per cap | — | — | — | — | — | silesia, enwik8 | DERIVED (exact arithmetic over `src/anvil.cpp:4247` + measured file sizes) |
| E7 | Frozen baseline the program compares against | https://github.com/thelabcorner/anvil/actions/runs/35927623136 | reference-cost (silesia/enwik8) | `.github/workflows/anvil-i10-aux-reference-cost.yml` | `c08078ec98e611d51d22961fe4c78e47b8c0c3f5` | `anvil-i10-aux-reference-cost-*` | silesia, enwik8 | MEASURED (quoted) |

---

## 7. Next steps / blockers

1. **The cap is now the programme's primary decode lever and the byte budget is nearly exhausted at
   its best point.** The byte-feasible decode-optimal cap is **2 MiB** (108,998 B of surplus left)
   and it misses G4 by ≈10 %; the decode-optimal 1 MiB cap is +217,517 B out of budget. Any further
   byte-side help must be a *zero-byte* or *negative-byte* change.
2. **G5 cannot be met by any cap** (see §5). The remaining memory lever is a decoder-side change
   that removes the file-proportional term — the whole reconstructed output (51,220,480 B for
   `mozilla`) plus the compressed input — from the decode peak. That is candidate C4 (the
   working-set levers L1/L2/L3), and it is the only remaining candidate that can clear G5. **It
   must cover the Brotli backend path as well as the BWT path**, because the peak-setting file is
   Brotli-routed.
3. **The auxiliary-index walk budget is a separate, independent NO-GO** and must not be re-burnt:
   the inverse BWT performs `n/2` dependent random accesses for every checkpoint stride `r`
   (8-way interleaved walks, `libsais_unbwt_decode_8`), so the walk budget cannot shorten the
   critical path; it only adds checkpoint bytes. The coordinator measured this on the frozen
   arbiter: W = 8192 → 16384 made the same-job decode ratio *worse* (1.2866 → 1.3986) for
   +419,744 B.
4. **Recommended arm for the coordinator to rule next:** the 2 MiB cap combined with the C4
   working-set levers, because the 2 MiB cap is the byte-feasible knee and the C4 levers are
   zero-byte-cost. The cap ladder between 1 and 2 MiB (e.g. 1.25 MiB = 1,310,720 B and
   1.5 MiB = 1,572,864 B) is worth ruling, because `--bwt-subblock` accepts any byte count and
   the G2/G4 trade-off may have a feasible interior point.
5. **Blockers:** none for the measurements above. The 16/32 MiB Silesia caps were not timed in
   this run (the 8 MiB point already dominates them on the byte/decode trade, and their RSS is
   worse); the 2/4/16/32 MiB enwik8 caps were not timed for the same reason (their routing
   outcome is identical to the 1/8 MiB points).
