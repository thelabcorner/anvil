# ANVIL Iteration 20 — Pareto Gate Closure: Preregistration, Candidate Enumeration, Shared Evidence Contract

**Date:** 2026-10-21
**Status:** BINDING for the I20 program. Pre-registers the goal, the baseline, the candidate
enumeration, the per-candidate decision rules, and the one shared evidence contract used by all six
I20 workers. No number in this document is a measurement; every number is either (a) quoted from a
named prior GitHub Actions run with its URL, or (b) exact arithmetic over such numbers.

**Authority:** this document. Where it conflicts with an older document, this document governs the
I20 program only. It does not reopen any closed novelty lane and does not re-litigate any
closed-by-measurement negative except where the *reopen condition* named in the do-not-reburn
register is explicitly satisfied and cited (see §5 C3).

**Parent documents:** `docs/I10-BREAKTHROUGH-PROGRAM.md`,
`docs/I10-AUX-UNBWT-RESULTS.md`, `docs/gate-ruling-i9-frontier-tokens.md`,
`docs/audit-2026-09-07/06-do-not-reburn.md`,
`docs/ANVIL-RESEARCH-SYNTHESIS-AND-OPEN-QUESTIONS-2026-10-07.md`,
`docs/github-actions-benchmark-protocol.md`, `docs/GITHUB-ACTIONS-BENCHMARKING.md`,
`tests/pr-4-measurement-window-protocol.md`, `tools/reference_cost_gate.py`,
`tools/paired_bench.py`, `tools/noise_floor.py`, `tools/pareto_front.py`, `FORMAT.md`.

---

## 1. Goal (measurable, machine-checkable)

Move the **canonical ANVIL ratio route** — `--parse=ratio --ratio-backend=auto` with the
auxiliary-index BWT representation (`--bwt-aux=on`) — from the ruling **`FRONT-GAP_COST`** to the
ruling **`COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE`** on the frozen canonical corpora, as computed by
`tools/reference_cost_gate.py` from a GitHub Actions run of the public repository
`thelabcorner/anvil`.

`COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE` is emitted by `tools/reference_cost_gate.py`
(lines 349–356) exactly when **all** of the following hold:

| # | Gate (pre-registered) | Silesia baseline | enwik8 baseline |
|---|---|---|---|
| G1 | timing evidence valid: both reference pairs `timing_valid` **and** the A/A null CI spans 1.0 and does not clear the speed gate | PASS | PASS |
| G2 | candidate total compressed bytes **strictly below** `xz -9e` | PASS (46,466,339 < 48,456,004) | PASS (23,537,422 < 24,831,648) |
| G3 | paired decode-time ratio **CI-upper ≤ 2.0 vs `xz -9e`** | PASS (1.8400) | **FAIL** (4.0088) |
| G4 | paired decode-time ratio **CI-upper ≤ 2.0 vs Brotli q11/lgwin30** | **FAIL** (3.5059) | **FAIL** (6.7596) |
| G5 | peak decode RSS ratio **≤ 2.0 vs `xz -9e`** | **FAIL** (4.572×) | **FAIL** (9.043×) |
| G6 | peak decode RSS ratio **≤ 2.0 vs Brotli q11/lgwin30** | PASS (1.995×) | **FAIL** (2.378×) |

Secondary (not a gate, but a claim ceiling): the encode axis and the decoder-only binary-size axis
are **not** ruled by this gate (`scope_note` in the tool). Any I20 statement must therefore stop at
`COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE` and must not be written as a FRONT-CROSSING.

**On Silesia the only failing gates are G4 (decode vs Brotli) and G5 (peak RSS vs xz).**
**On enwik8 four gates fail.** enwik8 is explicitly *best-effort*: the program target is the Silesia
gate closure; enwik8 closure is pursued if and only if the Actions budget allows it and is recorded
as such.

### 1.1 Baseline of record

| Quantity | Silesia | enwik8 | Source |
|---|---:|---:|---|
| ANVIL candidate total bytes | 46,466,339 | 23,537,422 | run `35927623136` |
| `xz -9e` total bytes | 48,456,004 | 24,831,648 | run `35927623136` |
| Brotli q11/lgwin30 total bytes | 49,383,136 | 24,810,180 | run `35927623136` |
| ANVIL decode throughput (approx.) | 47.9 MB/s | 26.5 MB/s | run `35927623136` |
| `xz -9e` decode throughput (approx.) | 82.5 MB/s | 103.6 MB/s | run `35927623136` |
| Brotli decode throughput (approx.) | 166.9 MB/s | 150.2 MB/s | run `35927623136` |
| ANVIL peak decode RSS | 248.4 MiB | 598.9 MiB | run `35927623136` |
| `xz -9e` peak decode RSS | 54.3 MiB | 66.2 MiB | run `35927623136` |
| Brotli peak decode RSS | 124.5 MiB | 251.9 MiB | run `35927623136` |
| ANVIL/`xz` decode-time ratio (point) | 1.7226 | 3.9065 | run `35927623136` |
| ANVIL/`xz` decode-time ratio (CI95) | [1.6640, 1.8400] | [3.9003, 4.0088] | run `35927623136` |
| ANVIL/Brotli decode-time ratio (point) | 3.4474 | 5.6642 | run `35927623136` |
| ANVIL/Brotli decode-time ratio (CI95) | [3.2680, 3.5059] | [5.6528, 6.7596] | run `35927623136` |
| Peak RSS ratio vs `xz` | 4.572× | 9.043× | run `35927623136` |
| Peak RSS ratio vs Brotli | 1.995× | 2.378× | run `35927623136` |
| Ruling | `FRONT-GAP_COST` | `FRONT-GAP_COST` | run `35927623136` |

**Baseline run URL:** `https://github.com/thelabcorner/anvil/actions/runs/35927623136`
**Baseline workflow file:** `.github/workflows/anvil-i10-aux-reference-cost.yml`
**Baseline candidate source commit (frozen tag `i10-aux-unbwt-final-v1`):**
`c08078ec98e611d51d22961fe4c78e47b8c0c3f5` (public commit);
`src/anvil.cpp` blob `755df76ae53795aea040f4c99784d1effb43b3ab`.
**Program base commit:** `research/anvil-i19-origin-lock-20261007` @ `fc92308ee…` — whose
`src/anvil.cpp` blob is **byte-identical** to the frozen baseline candidate
(`755df76ae53795aea040f4c99784d1effb43b3ab`). Consequently the byte-side of the baseline is
reproducible by construction on the program base commit, and any I20 arm that changes
`src/anvil.cpp` must re-establish byte identity explicitly (§6, Adopt-Gate).

### 1.2 Byte surplus available to spend (exact arithmetic over §1.1)

| Surplus | Silesia | enwik8 |
|---|---:|---:|
| vs `xz -9e` (the G2 binding reference) | **1,989,665 B** | **1,294,226 B** |
| vs Brotli q11/lgwin30 | 2,916,797 B | 1,272,758 B |

Any mechanism that buys decode speed or reduces peak RSS by **spending bytes** is charged against
the `xz` surplus above. If a mechanism's charged byte cost exceeds the `xz` surplus, G2 fails and
the arm is dead regardless of its speed or memory result. This number is the single most important
budget constraint in the program.

### 1.3 Required improvement, stated as a target before measurement

- Silesia G4: decode-time ratio CI-upper vs Brotli must fall from **3.5059 to ≤ 2.0** — a
  **≥1.75× reduction of the upper confidence bound** (i.e. the candidate must decode in ≲57 % of
  its current time).
- Silesia G5: peak decode RSS ratio vs `xz` must fall from **4.572× to ≤ 2.0** — a **≥2.29×
  reduction** in peak RSS.
- enwik8 G3/G4/G6 (best effort): decode ratio vs `xz` 4.0088 → ≤2.0 (≥2.00× faster);
  vs Brotli 6.7596 → ≤2.0 (≥3.38× faster); RSS vs `xz` 9.043× → ≤2.0 (≥4.52×);
  RSS vs Brotli 2.378× → ≤2.0 (≥1.19×).

These are **targets derived from the gate thresholds**, not promises. Every arm that fails to reach
them is recorded as `NO-GO` or `INCONCLUSIVE` with its falsifying evidence.

---

## 2. Why this goal and not another

The retained Class-A adjudication is **435 DOMINATED + 28 DEGENERATE + 5 FRONT-GAP + 0
FRONT-CROSSING** (468/468). The five gap cells are unresolved, not wins. I11–I19 produced
source-attested *specialized* numeric results on consumed fixtures and did not change the
general-purpose crossing count. The one place where ANVIL holds a **real, reproducible, complete
wire advantage against the binding external references** is the canonical ratio route: it is
1,989,665 B smaller than `xz -9e` on Silesia and 1,294,226 B smaller on enwik8, with every
round-trip byte-exact.

That advantage is currently spent on a cost deficit (decode time, peak RSS) that the repo's own
`tools/reference_cost_gate.py` already measures, already rules on, and already emits a machine
token for. Closing that gate is therefore the highest-EV *general-purpose* frontier move available
to this program: it converts an existing, measured, byte-winning operating point into a
cost-competitive one, and the arbiter is the repo's own tool rather than a new metric. It also has
a clean kill condition: if no combination of the enumerated levers can buy the required decode
speed and RSS reduction inside the byte surplus, the program closes with a documented
`FRONT-GAP_COST` re-confirmation and a fully specified "next mechanism must change the
representation" requirement.

**Out of scope for I20:** novelty. Every I20 mechanism is `{adopt-class}` / `{engineering}`. The
novelty audit closed TCOPY/PNRA, fixed-cap BWT subblocking *as novelty*, H2/C1, FLI, CAM, generic
routing, and parser work on the canonical route. I20 claims **no** novelty. A positive I20 result
authorizes exactly one follow-up: an integration/validation run.

---

## 3. What the canonical ratio route actually does (mechanism facts, from source)

`--parse=ratio` writes revision-2 block mode 17: `transform_id`, `backend_id`,
`transformed_size`, then a backend payload. `backend_id = 2` is
**BWT + ANVIL postcoder portfolio**: `libsais` forward BWT → MTF → zero-run RLE → postcoder
(1 = order-0 adaptive arithmetic, 2 = order-1 adaptive arithmetic, 3 = raw) → inverse BWT.
`--bwt-aux=on` adds the additive auxiliary-index v2 inner payload: a sampling rate `r`
(power of two), `auxiliary_count = 1 + floor((n-1)/r)`, and `4 · auxiliary_count` bytes of
checkpoint indices, decoded by `libsais_unbwt_aux`.

Three facts bound this program:

1. **The encoder's aux sampling policy is `r = ` smallest power of two with
   `r·1024 ≥ n`** (`kBwtAuxTargetWalks = 1024`, `src/anvil.cpp:3913`, `bwt_aux_rate_for_size`,
   `src/anvil.cpp:4247`). The walk budget is a **hard-coded encoder policy with no CLI knob**;
   `r` is transmitted in the payload, so the wire format already supports any power-of-two `r`.
2. **The outer `0xFF` BWT subblock frame caps the per-piece working set** and is already exposed
   as `--bwt-subblock=N` (bytes, `src/anvil.cpp:4992`, default 128 MiB). It was swept at
   8/16/32/64/128 MiB in run `35930672607`; **4 MiB and below were never measured.**
3. **Peak decode RSS is dominated by the inverse-BWT working set**:
   `libsais` needs an `int32` array of `n+1` entries (4 n bytes) plus the BWT string (n bytes),
   plus the reconstructed output (n bytes), plus the compressed input. For a whole-file BWT of
   `n` bytes the inverse alone is ≈5 n bytes of live state. This is why the `128 MiB` cap
   (i.e. no cap) leaves Silesia at 248.4 MiB while `xz -9e` sits at 54.3 MiB.

---

## 4. Shared evidence contract (identical for all six I20 workers)

### 4.1 Compute locus

**All measurement runs on GitHub Actions. Zero local-compute measurements are admissible as
evidence.** Local activity is limited to editing, static inspection, exact arithmetic over
already-collected artifacts, and git/GitHub operations. A local `anvil`/`xz`/`brotli`
invocation that produces a timing or byte number is **not** evidence; if it is performed at all it
must be reported as `PROJECTION` and must not be cited as a measurement.

### 4.2 Source identity

- Base commit for all branches: `research/anvil-i20-pareto-gate-closure-20261021`.
- Every workflow asserts, in the provenance artifact: `git rev-parse HEAD` of the checked-out
  source, the SHA-256 of every built binary, the runner image, `uname -a`, `lscpu`, compiler
  versions, and the corpus file SHA-256s.
- A candidate that changes `src/anvil.cpp` must additionally record the **pre-change** and
  **post-change** `src/anvil.cpp` SHA-256.

### 4.3 Corpus lock

- Silesia: the 12 canonical files, total **211,938,580 B**, fetched by `tools/gha_fetch_corpus.py`
  with the pinned size+MD5 checks already implemented; per-file SHA-256 recorded in the artifact.
- enwik8: `enwik8.zip` → `enwik8`, 100,000,000 B, MD5 `a1fa5ffddb56f4953e226637dabbb36a`.
- Corpus bytes exist only in the runner workspace; they are never committed.
- A candidate's byte result is only admissible if the **same corpus bytes** are used, and the
  per-file SHA-256s are recorded.

### 4.4 Measurement window and timing protocol (PR-4 v1.2, binding)

- Same-job paired A/B only. **Never** compare an absolute MB/s from one run to another run.
- Driver: `tools/paired_bench.py`. Defaults for this program:
  `--reps 9 --warmups 2 --seed 41246 --bootstrap 20000 --epsilon 0.02 --max-cv 0.15 --cpu 0
  --ambient-reps 5 --ambient-iterations 250000 --ambient-max-cv 0.10`.
- An **A/A null pair** is mandatory in every job that produces a claim-bearing timing number; the
  null CI must span 1.0 and must not clear the speed gate, else the arm is `TIMING_BLOCKED`.
- All raw repetitions are retained in the artifact (never dropped, never outlier-filtered).
- Thread counts: every arm is single-threaded and this must be stated on the row. A row with an
  undisclosed thread imbalance is not arbiter-usable.
- Ambient load: the ambient gate is part of the driver; a run that is `BLOCKED_AMBIENT` is
  **neutral, not a failure**, and must be re-run, not reinterpreted.
- Evidence classes are explicit: `MEASURED` | `DERIVED` | `PROJECTION`. Derived and projected
  numbers never support a gate verdict.
- Silesia/enwik8 reference arms must be measured **in the same job** as the candidate.

### 4.5 Correctness gate (before any timing is consumed)

- Every arm round-trips **byte-exactly**: decoded output SHA-256 equals source SHA-256, and
  output length equals source length.
- The candidate's compressed bytes must be **identical across all repetitions**; a mismatch is a
  hard failure, not an average-away.
- Any change to `src/anvil.cpp` additionally requires the repo's fuzz/registry path
  (`tests/fuzz.py::registry_coverage_roundtrip`, `forced_postcoder_roundtrip`, and the
  auxiliary-BWT assertions) to pass on the changed binary in the same job, and the **default-off**
  (`--bwt-aux=off`) byte output to be byte-identical to the frozen I9/I10 bytes per file.

### 4.6 Reference class

- Binding references: **`xz -9e`** and **Brotli q11 with window 30** (the repo's pinned
  `brotli_lw` helper, `tools/brotli_lw.cpp`; the reference-cost gate uses the same helper).
- The reference grid is **GRID-THIN** (zstd 4–22 and additional Brotli window tiers are not
  measured in the binding gate). Any claim must carry the `GRID-THIN` label. Worker W6 exists
  specifically to test the candidate against a **denser** grid so that a positive result is not a
  sparse-grid artifact (this is the `NQ-6` control requirement).

### 4.7 Decision rule discipline

- Every candidate has **one** decision rule, stated in §5 **before** measurement, with numeric
  thresholds. No post-hoc threshold changes.
- A candidate is `WIN` only if: byte-exact round-trip verified, reproducible under the paired
  protocol with a valid A/A null, the effect clears the pre-registered 2 % practical epsilon with
  a CI excluding 1.0 in the improving direction, and the charged byte cost keeps the portfolio
  total below `xz -9e`.
- Everything else is `NO-GO` (measured and falsified) or `INCONCLUSIVE` (measurement not
  conclusive — with the reason recorded: blocked ambient, insufficient reps, timing invalid, etc.).
  There is no third state.
- A `WIN` is **not** a FRONT-CROSSING. The arbiter token is produced by
  `tools/reference_cost_gate.py` (`summarize`), and the report must quote that token verbatim.

### 4.8 Evidence record format (mandatory, per worker)

Every worker MUST record, for every claim it makes:

```
run_url:      https://github.com/thelabcorner/anvil/actions/runs/<id>
job:          <job name>
workflow:     .github/workflows/<file>.yml
commit_sha:   <full 40-hex>
artifact:     <artifact name>
corpus:       silesia | enwik8
```

A claim without all six fields is not admissible.

---

## 5. Candidate enumeration (Phase 1 — exhaustive, no compute)

### 5.1 Live candidates and their pre-registered decision rules

| ID | Candidate | Mechanism | Expected effect | Cheapest falsifying measurement | Pre-registered decision rule | Worker |
|---|---|---|---|---|---|---|
| **C1** | BWT decode **stage decomposition** | attribute whole-codec decode time to postcoder-entropy / MTF / inverse-BWT LF walk / framing+alloc | identifies whether the LF-walk is the dominant cost | one same-job timed job with per-stage instrumentation on Silesia BWT-routed files | **Amdahl kill (from the repo's own queue, `NQ-4`):** if the LF-walk share is ≥ 78 %, then C2/C5 are the only funded levers and C4/C3 carry the memory gate; if the LF-walk share is ≤ 55 %, the LF-walk levers (C2) are **KILLED** and the program must target the postcoder (C5) instead | W1 |
| **C2** | **Auxiliary-index walk-budget sweep** (add `--bwt-aux-walks=N`; smaller `r` ⇒ shorter LF walks) | trade checkpoint bytes for LF-walk length | decode speedup up to ~8× at +0.02 %…+0.2 % bytes | byte-only sweep first (no timing), then one paired timed sweep | **Adopt the largest reduction in `r` whose charged bytes keep the Silesia portfolio total below `xz -9e`.** If the charged byte cost of the sweep exceeds the 1,989,665 B surplus, the arm is `NO-GO` on G2. If the decode-time ratio CI-upper vs Brotli does not fall below 2.0 at the byte-feasible point, C2 is `NO-GO` | W2 |
| **C3** | **BWT subblock-cap sweep below 8 MiB** (`--bwt-subblock=1|2|4 MiB`) | cap the per-piece inverse-BWT working set | peak RSS ÷2–4 at +1 %…+3 % bytes | byte-only sweep (the cap is already a CLI flag) | **Adopt the smallest cap whose charged bytes keep the Silesia portfolio total below `xz -9e` and whose measured peak decode RSS ratio vs `xz` is ≤ 2.0.** If no cap ≤ 8 MiB simultaneously satisfies both, C3 is `NO-GO` on Silesia (it may still be reported as a measured rate/memory trade) | W3 |
| **C4** | **Decode working-set engineering** (stream the reconstructed output instead of materialising it; eliminate the duplicate BWT-string copy) | reduce peak RSS at **zero** byte cost | peak RSS − n bytes (≈39.5 MiB on webster) | RSS-only job, no timing needed for the first pass | **Adopt iff the changed binary is byte-identical to the frozen candidate on every corpus file, passes the repo fuzz/registry path, and reduces measured peak decode RSS by ≥ 1.0 MiB without increasing decode time CI-upper by more than the 2 % epsilon.** Byte cost must be exactly 0 | W4 |
| **C5** | **Postcoder decode-cost swap** (replace the adaptive-arithmetic MTF token coder with a cheaper coder from the existing stream suite — rANS/Huffman — over the same MTF token stream) | spend bytes to buy entropy-decode throughput | decode speedup 1.2–2× at +0.5 %…+2 % bytes | standalone prototype microbench in `prototypes/` (no `src/` change) | **Adopt iff the prototype's charged complete-byte cost stays inside the 1,989,665 B surplus for its measured decode gain, and the postcoder is ≥ 35 % of whole-codec decode time (per C1).** If C1 shows the postcoder is < 20 % of decode time, C5 is `NO-GO` on Amdahl grounds | W5 |
| **C6** | **Dense reference-grid control** (brotli q6/q9/q11 × window tiers, zstd 19/22, `xz -9e`/`-9`, and the best available point) | test whether any gate-closing point is a sparse-grid artifact | a defensible (or a falsifying) reference front | one same-job reference-measurement job | **The final candidate is reportable only if it is non-dominated against the dense grid on the same axes. If a denser reference point dominates it, the program closes `NO-GO` on the dense grid, and this is recorded as the binding result** | W6 |

### 5.2 Rejected on paper (with reasons — not re-litigated)

These are **not** dispatched. Each is either closed by the repo's own measurement, closed by
prior-art audit, or outside this program's gate.

| Rejected | Reason | Source |
|---|---|---|
| Unconditional order-1 literals | +3.46 % bytes measured | ledger Experiment C |
| `(k,slot)` topology / modal-residual coding (mode 13) | lost on **every** file; modal accuracy 17–23 % vs the 86.5 % premise | ledger Experiment I; `do-not-reburn` B1 |
| Lane/field transposition | every tested section grew — destroys phrase structure | `docs/CONTEXT.md` |
| TCOPY transmitted-Δ submode | both sections grew; discovery prohibitively expensive | ledger Experiment K |
| Static per-shape displacement manifolds | failed the Pareto gate | `docs/CONTEXT.md` |
| Global BWT default | closed by measurement (BWT wins 7/12, loses 5/12) | `do-not-reburn` F2 |
| Block-local *backend routing* | +894,315 B on webster; 0 BWT-winning blocks on 4/12 files | `do-not-reburn` G1 |
| QLFC / LZP postcoders (IDs 4/5) | +2,437,384 B and +279,419 B measured | `do-not-reburn` G2 |
| Reciprocal-rANS (replace hardware divide) | measured slower | `do-not-reburn` D3 |
| Whole-codec stream budget at λ=0.01 as a decode lever | arithmetically predictable, closed | `do-not-reburn` B6 |
| RLZ / RePair stream coders as default | encode 10–40× slower, decode worse | `do-not-reburn` B5 |
| `zstd` as a ratio bar | loses every Silesia file to Brotli/xz | `do-not-reburn` F3 |
| TCOPY / PNRA novelty (any framing) | four-cell decomposition: only the zero-bit-parameter cell is even arguably free, priced at 3.6–10 B | `docs/swarm-2026-10-02/20-priorart-killteam-fledge.md`, `NQ-1` |
| Fixed-cap BWT subblocking **as novelty** | closed as a novelty program | `FROZEN-CLOSEOUT-MATRIX-R2.md` §2.2 — **C3 reopens it as engineering only, with a new stated premise (§5.1) and a new decision rule** |
| Parser work on the canonical route | `--parse=ratio` bypasses the token parser entirely; Track 17 closed | `FROZEN-CLOSEOUT-MATRIX-R2.md` §2.3 |
| I11–I19 numeric/typed-numeric lanes | specialised, consumed fixtures, no general-purpose crossing; and their corpora are custody-blocked | `docs/ANVIL-RESEARCH-SYNTHESIS-AND-OPEN-QUESTIONS-2026-10-07.md` §3, §10 |
| **DEFLATE reconstruction (Q7 / P4.1)** — the ledger's "#1 next byte investment" | **Rejected for *this program*, not in principle.** The I20 gate is not byte-limited: G2 already passes with 1,989,665 B to spare, and the failing gates are decode cost (G3/G4) and peak RSS (G5/G6). A byte-only lever on a Brotli-routed file (`mozilla`) cannot move a single failing gate. It is also the largest build in the repo (a pinned bit-exact DEFLATE replay engine) against occupied prior art (preflate-rs / precomp). It is **retained as the top next lead** for a program whose objective is bytes. | `do-not-reburn` G3; `FROZEN-CLOSEOUT-MATRIX-R2.md` §6 (Q7 HOLD) |
| Grotli G1/G3/G4/G5B lanes | G1 broad NO-GO, G3 adverse on validation, G4 NO-GO, G5B adverse | `docs/I10-BREAKTHROUGH-PROGRAM.md` §I10-1C |
| Any "novelty" framing of an I20 mechanism | I20 is `{adopt-class}`/engineering; novelty needs a separate pre-registration | `FROZEN-CLOSEOUT-MATRIX-R2.md` §1.2, `NQ-9` |

### 5.3 Expected information per Actions minute (ranking rationale)

1. **C1** — cheapest, most decisive: one job, no source change, and it can *kill* C2 or C5 outright
   by the repo's own Amdahl rule. Highest information per minute by a wide margin.
2. **C3** — no source change (the CLI flag exists), a byte-only pass first, and it attacks the
   single failing Silesia gate G5 that C1/C2 cannot touch.
3. **C2** — needs a small, wire-compatible encoder-policy knob; the byte cost is trivially small and
   the decode upside is the largest available. Sequenced after C1 because C1 can kill it.
4. **C4** — zero byte cost, but it needs real decoder surgery and is gated by byte identity + fuzz.
5. **C5** — needs a prototype and an Amdahl precondition from C1; can be large in bytes.
6. **C6** — needed for the final verdict, but it is a *control* job, not a lever; it is dispatched
   against the **baseline** arm in parallel and re-run against the winner if there is one.

---

## 6. Adopt-Gate (binding on any adopted change)

No I20 change is adopted unless **all** of the following hold, and the evidence for each is in a
GitHub Actions artifact with the §4.8 fields:

1. **Byte identity against the frozen default.** With the change disabled (or with default
   options), the compressed output is **byte-identical per file** to the frozen I10-1A bytes
   (Silesia 46,466,339 B total; enwik8 23,537,422 B total) and to the frozen I9 legacy bytes
   (Silesia 46,446,995 B; enwik8 23,534,368 B) with `--bwt-aux=off`.
2. **Registry / format consistency.** Any new wire-visible parameter is specified in `FORMAT.md`
   with its exact bounds, rejection rules, and consumption accounting; any new ID is added to the
   decoder-visible registry and is covered by a **direct forced** encode→decode test (selection-only
   coverage is insufficient — this is the postcoder 0/4 lesson).
3. **Fuzz / registry coverage.** `tests/fuzz.py` registry coverage, forced-postcoder coverage and
   auxiliary-BWT assertions pass on the changed binary, plus the repo's mutation battery.
4. **Round-trip byte-exactness** on every corpus file and on the repo's `tests/corpus` fixtures.
5. **The change is additive.** No existing block mode's wire semantics are changed; a new encoder
   policy knob must be default-preserving.

---

## 7. Program-level decision rule (stated before measurement)

The program closes as:

- **`GATE-CLOSED-SILESIA`** — `tools/reference_cost_gate.py` emits
  `COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE` for the Silesia corpus on a named Actions run, with a
  valid A/A null, byte-exact round-trips, and the charged byte total still below `xz -9e`.
- **`GATE-CLOSED-ENWIK8`** — the same, for enwik8.
- **`NO-GO`** — the best measured arm still fails G3/G4/G5/G6, and the binding gate is named.
- **`INCONCLUSIVE`** — the measurement was not conclusive (ambient block, invalid timing, or
  exhausted budget); the specific reason is named.

In all cases every arm is recorded in `RESEARCH_LEDGER.md` and in this program's results
document with its run URL, job, workflow, commit SHA, artifact, and its `MEASURED`/`DERIVED`/
`PROJECTION` label. A negative result with falsifying evidence is a **complete** program outcome
and is written up as one.

---

## 8. Worker mandates (six @swarm members, distinct and non-overlapping)

| Worker | Mandate | Owns (exclusive) | May not touch |
|---|---|---|---|
| **W1** | C1 stage decomposition **+ fresh baseline reproduction** of the §1.1 baseline on the program base commit | `tools/i20_stage_decompose.*`, `.github/workflows/anvil-i20-stage-decompose.yml` | `src/anvil.cpp` |
| **W2** | C2 aux walk-budget sweep (adds `--bwt-aux-walks=N`) | `src/anvil.cpp` (the CLI knob only), `tools/i20_aux_walk_sweep.py`, `.github/workflows/anvil-i20-aux-walk-sweep.yml` | other `src/anvil.cpp` regions |
| **W3** | C3 BWT subblock-cap sweep at 1/2/4 MiB (+ re-measure 8 MiB as control) | `tools/i20_subblock_sweep.py`, `.github/workflows/anvil-i20-subblock-sweep.yml` | `src/anvil.cpp` |
| **W4** | C4 decode working-set engineering (RSS reduction at zero byte cost) | `src/anvil.cpp` (decode path), `.github/workflows/anvil-i20-working-set.yml` | encoder policy regions |
| **W5** | C5 postcoder decode-cost swap prototype + Amdahl accounting | `prototypes/i20-postcoder-swap/**` | `src/anvil.cpp` |
| **W6** | C6 dense reference-grid control (and the final canonical recompute) | `tools/i20_dense_reference.py`, `.github/workflows/anvil-i20-dense-reference.yml` | `src/anvil.cpp` |

Ownership of `src/anvil.cpp` is shared by W2 and W4 only; they work in **separate git worktrees and
separate branches**, and the coordinator performs any merge. W1/W3/W5/W6 never modify
`src/anvil.cpp`.

---

## 9. Records to be updated at close

- `RESEARCH_LEDGER.md` — one entry per candidate: hypothesis, mechanism, exact charged bytes,
  measured decode/RSS, verdict, evidence fields, and the reason for a `NO-GO`.
- `docs/I20-PARETO-GATE-CLOSURE-RESULTS-2026-10-21.md` — the program's results and canonical
  ruling, quoting the `tools/reference_cost_gate.py` token verbatim.
- `FORMAT.md` — only if an adopted change is wire-visible.
- `docs/I20-BREAKTHROUGH-PROGRAM.md` (new) — the program narrative, the six worker findings, and
  the next ranked leads.

---

## 10. Coordinator tooling (additive, default-preserving)

Two pieces of coordinator-owned tooling exist so that a winning arm is judged by **exactly** the
same arbiter that produced the baseline ruling. Neither changes the arbiter's behaviour.

1. **`tools/reference_cost_gate.py` — new optional flag `--candidate-extra-args`** (default
   `[]`). The flag appends encoder-policy flags to the ANVIL candidate arm's encode command so a
   candidate arm (for example `--bwt-aux-walks 16384 --bwt-subblock 4194304`) can be ruled by the
   frozen arbiter. **With the default the generated command is byte-identical to the pre-I20
   form** (verified by inspection of the command construction and by the baseline reproduction
   run asserting the frozen portfolio totals). No other function of the tool is touched: the
   corpus lock, the round-trip gate, the paired protocol, the A/A null, the RSS measurement and
   the `summarize` ruling function are unchanged.
2. **`.github/workflows/anvil-i20-gate-closure.yml`** — a coordinator-owned workflow that builds a
   candidate from an arbitrary git ref, runs the frozen `prepare` / `paired_bench` / `rss` /
   `summarize` sequence, and uploads the ruling. It is `workflow_dispatch` only,
   `permissions: contents: read`, `cancel-in-progress: false`, with a finite timeout and
   `if: always()` artifact upload. It emits the ruling token produced by the frozen tool and
   nothing else.

Both are recorded here so that the arbiter's integrity is auditable: the I20 program's final
verdict is a `tools/reference_cost_gate.py` token, not a coordinator opinion.

---

*Preregistered 2026-10-21 by the I20 coordinator. All thresholds above are fixed before
measurement and will not be altered after results are observed.*
