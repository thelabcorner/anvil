# ANVIL Iteration 11 — "Pareto Dominance": Preregistration, Candidate Enumeration, Shared Evidence Contract

**Date:** 2026-10-22 · **Status:** BINDING for the I11 programme.
**Base commit:** `dc68b24` (the I20 record, merged into `main`).
**Programme branch:** `research/anvil-i11-pareto-dominance-20261021`.
**Compute locus:** GitHub Actions on `thelabcorner/anvil` only. **Zero local-compute evidence.**

> **This programme inherits the I20 closure and must not weaken it.** I20 closed the external
> reference-cost gate on Silesia at the named profile `--ratio-bwt-min-gain-pct 3`
> (`COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE`, 7 of 7 identical replications, run of record
> `39696090369`, commit `9c206adf`, artifact `anvil-i20-gate-closure-silesia-39696090369`). **I11's
> goal is strictly harder: actual dominance over a dense reference front on both canonical corpora,
> plus the two axes the I20 arbiter explicitly does not rule.** Nothing here relaxes a threshold.

---

## 1. The goal, in the repository's own terms

Make ANVIL an **overarching Pareto-dominant general-purpose lossless codec**: the canonical
`--parse=ratio --ratio-backend=auto --bwt-aux=on` route, at a **named, preregistered profile**,
must dominate — not merely fail to be dominated by — a **dense external reference front**, on the
frozen canonical corpora, measured **in the same job**, judged by the repository's **own frozen
arbiters**.

### 1.1 The dominance criterion (primary), operationalised

For **each** corpus in {Silesia 12-file, 211,938,580 B; enwik8, 100,000,000 B}:

Let `R` be the set of reference points measured in the same job:

- Brotli q1 / q4 / q6 / q9 / q11, each at window 22, and q11 additionally at window 24 and 30;
- zstd 1 / 3 / 9 / 19 / 22;
- xz -6 / -9 / -9e;
- lz4 1 / 9.

And let `A` be the ANVIL candidate arm.

`A` **dominates** iff, for every `r ∈ R`:

| axis | requirement | axis source |
|---|---|---|
| complete compressed bytes | `bytes(A) < bytes(r)` | the `prepare` stage of the reference-cost gate |
| whole-corpus decode wall time | `t_decode(A) <= t_decode(r)` | the paired same-job decode timing |
| peak decode RSS | `rss(A) <= rss(r)` | the per-process `/usr/bin/time -f %M` series |
| whole-corpus encode wall time | `t_encode(A) <= t_encode(r)` | **new axis — the I20 arbiter does not rule it** (§4.10) |
| decoder-only binary size | `size(A_decoder) <= size(r_decoder)` | **new axis — the I20 arbiter does not rule it** (§4.10) |

and the ruling must satisfy the **reproducibility** requirement:

- **≥ 5 of 5** independent same-job replications of the identical arm satisfy every axis;
- the A/A null CI spans 1.0 in every replication (a run whose null does not span 1.0 is
  `TIMING_BLOCKED` and counts for nothing);
- the candidate's **own** whole-corpus decode time varies by **< 1.3×** across the replications.

> **Why the < 1.3× clause exists, and it is not a formality.** In I20 the *candidate's* decode
> varied 1.32× while the *reference's* varied 1.98× on the same runner class. A gate defined as a
> ratio against a non-reproducible denominator is a Bernoulli variable; I20's first closure passed
> 3 times in 7 for that reason alone, and it took a **numerator**-reducing mechanism (the decode-
> budgeted routing profile) to get 7 of 7. **A dominance claim must therefore carry a stated
> stability bound on the candidate, and the programme must not re-open the "the reference was fast
> that day" explanation.**

### 1.2 The "dense enough" requirement

A dominance claim over a front that happens to omit a stronger reference is a GRID-THIN artifact.
The front above is **21 points across 5 reference families and 3 window tiers**. Before any
dominance claim is made, the programme must record the **dense byte bar** — the smallest reference
payload in the front — and show that the candidate beats it. If a reference family cannot be
measured on the hosted runner (e.g. a zstd tier the pinned build lacks), that gap must be
**declared**, not silently absorbed.

### 1.3 The Class-A grid criterion (secondary)

The local `tests/corpus` grid, under the **unmodified** `tools/pareto_front.py` and
`tools/beats_brotli.py`, must move from

```
435 DOMINATED / 28 DEGENERATE / 5 FRONT-GAP / 0 FRONT-CROSSING
```

to **≥ 1 FRONT-CROSSING**, with the **co-listed tuple quoted whole** and its **grid provenance**
attached (the tuple is never quoted as a single number, and the grid identity is always named).
A crossing is `non-dominated AND not FRONT-GAP AND not DEGENERATE` under the frozen classification in
`docs/gate-ruling-i9-frontier-tokens.md`; the dual-bar same-transform control applies to any
synthetic cell.

### 1.4 enwik8 is IN SCOPE and is currently failing

enwik8 is a **recorded measured NO-GO**: run `39696093027`, job `gate-closure (enwik8)`, artifact
`anvil-i20-gate-closure-enwik8-39696093027` — G3 **3.3013** FAIL, G4 **4.6013** FAIL, G5 **7.2719×**
FAIL, ruling `FRONT-GAP_COST`. The cause is structural and measured: the sub-block caps that would
bound the inverse-BWT working set (≤ 32 MiB) flip the single 100,000,000 B unit to the Brotli
route (24,810,212 B, only 21,436 B of byte margin, peak RSS = Brotli's 251.4 MiB = 3.803× `xz -9e`),
while the caps that keep the BWT route (≥ 64 MiB) leave the peak RSS at 6.223–9.059× `xz -9e`.
**Re-running the same caps is forbidden**; enwik8 needs a new representation or a new inverse-BWT
locality strategy.

---

## 2. What the programme inherits (measured, not assumed)

| fact | value | source |
|---|---|---|
| canonical route bytes at the I20 profile | 48,281,716 B (174,288 B under `xz -9e`) | run `39696090369` |
| inverse-BWT LF access count | **`n/2` for every checkpoint stride `r`** | `third_party/libsais/src/libsais.c:7854-7892` |
| fitted walk-budget Amdahl share | `f = 0.000 ± 0.02` vs ≈0.70 required | run `39676039083` |
| BWT-path decode rate | ≈32 MB/s | I20 §2.6 |
| peak decode RSS setter (Silesia) | `mozilla` (51,220,480 B), Brotli-routed at **every** cap | run `39656702596` |
| reference-arm decode spread (hosted runners) | **1.98×** (0.767–1.519 s for Silesia) | 7 I20 replications |
| candidate decode spread | 1.32× (first closure) → **1.28×** (C7 profile) | I20 §4.7 |
| I20 closure | `COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE`, 7 of 7 | runs `39696090369`, `39697830840`, `39697833411`, `39697836346`, `39697838963`, `39697841540`, `39700010922` |

---

## 3. The shared evidence contract (identical for all six workers)

1. **Compute locus.** Every measurement is a GitHub Actions run on `thelabcorner/anvil`. **No local
   measurement is admissible as evidence.** A local number may be computed only as a *sanity check on
   a non-corpus input* and must then be labelled `PROJECTION`.
2. **Source identity.** Every artifact records: `git rev-parse HEAD` of the candidate, the SHA-256 of
   every built binary, the runner image, `uname -a`, `lscpu`, the toolchain versions, and the
   per-file corpus SHA-256s.
3. **Corpus lock.** Silesia 12 files (211,938,580 B) and enwik8 (100,000,000 B) via
   `tools/gha_fetch_corpus.py` (pinned size + MD5). Corpus bytes exist only in the runner
   workspace.
4. **Correctness before timing.** Every arm must round-trip **byte-exactly** (decoded SHA-256 ==
   source SHA-256, length equal) before any timing is consumed; the compressed bytes must be
   identical across all repetitions (a mismatch is a hard failure, never an average).
5. **Same-job measurement.** Every candidate/reference comparison is measured **inside one job**.
   Absolute throughput from one job is never compared with absolute throughput from another.
6. **Paired protocol.** `tools/paired_bench.py` with the frozen parameters
   (`--reps 9 --warmups 2 --seed 41246 --bootstrap 20000 --epsilon 0.02 --max-cv 0.15 --cpu 0
   --ambient-reps 5 --ambient-iterations 250000 --ambient-max-cv 0.10`), CPU affinity 0, **all raw
   repetitions retained** (never dropped, never outlier-filtered), and a mandatory **A/A null** whose
   CI must span 1.0 or the series is `TIMING_BLOCKED` and counts for nothing.
7. **Thread disclosure.** Every row states its thread counts; a row with an undisclosed thread
   imbalance is not arbiter-usable.
8. **Evidence classes.** `MEASURED` | `DERIVED` | `PROJECTION` are explicit. Derived and projected
   numbers never support a gate verdict.
9. **Six mandatory fields per claim:** `run_url`, `job`, `workflow` file, `commit_sha` (full 40 hex),
   `artifact` name, `corpus`.
10. **Durable evidence.** The artifacts a run uploads are downloaded into
    `tests/i11-evidence/runs/<run_id>/` and re-emitted by
    `tools/i11_evidence_manifest.py` into `tests/i11-evidence/MANIFEST.{json,md}`, which
    **recomputes the gate checks independently of the arbiter's own `gates` map**. A claim that is
    not in the manifest is not a claim.
11. **No threshold relaxation, no post-hoc gate.** Every candidate's decision rule is fixed in
    §5 **before** its measurement. A threshold changed after seeing data invalidates the candidate.
12. **Every non-win is recorded** as `NO-GO` or `INCONCLUSIVE` with its falsifying evidence, in
    `RESEARCH_LEDGER.md` and in the I11 results document.
13. **Claim ceiling.** A closure is always stated as **the arbiter's own token together with its
    scope note**, never as a FRONT-CROSSING unless the encode and decoder-binary-size axes are also
    ruled.

---

## 4. Why the six candidates are these six (and the order)

The goal's clause 4 names four mechanism measurements that must precede further arbiter spend. They
are folded into the six candidates below as **D1/D2/D3 (diagnostics that gate the mechanism
candidates)**, plus the two arbiter/grid extensions.

**Order of work (binding):** D1 and D2 are *diagnostics* and must complete before D3/D4 spend
compute; D5 (the arbiter extension) is a prerequisite for any dominance or FRONT-CROSSING claim,
because the I20 arbiter's own scope note excludes the encode and decoder-binary-size axes; D6 is
independent of the others and may run in parallel.

---

## 5. Candidate enumeration (six candidates, one per worker)

### D1 — BWT-path decode speed vs sub-block cap, routing held constant

**Hypothesis.** The inverse BWT's `n/2` dependent random accesses land in a `4C`-byte LF array
when the sub-block cap is `C`, so a smaller cap should make those accesses cache-resident and
therefore faster. The earlier sweep (I20 `39663273951`, `39676132197`) could not test this because
changing the cap also changes the **router's** backend choice, so the portfolio speed-up was
confounded with routing.

**Mechanism.** Measure the decode time of a **single file that is always BWT-routed**, at caps
{1, 2, 4, 8, 16, 32, 64, 128} MiB, with the backend and every other encoder setting held
constant, so only the LF-array working set changes. The file must be chosen so the router keeps it
on the BWT route at **every** cap in the sweep (Silesia `mr` and `x-ray` satisfy this at
`--ratio-bwt-min-gain-pct 3`; the sweep must verify the route at each cap and report it).

**Decision rule (fixed in advance).**
> **Report the curve, the fitted cache-locality gain, and the answer to one question: *is the
> BWT-path decode time a function of the cap once routing is held constant?* If the curve is flat
> within the run's own noise band (the A/A null's CV), the cache-locality hypothesis is
> **DEAD**, D3 must change the inverse *algorithm* rather than the cap, and the programme must not
> spend further arbiter minutes on cap tuning. If the curve is monotone and material (≥ 1.5× across
> the sweep), the cap becomes a first-class locality lever and D3 must exploit it.**

**Kill condition.** Any cap sweep that changes the route is void.

### D2 — The postcoder's share of BWT-path decode (same-job differential)

**Hypothesis.** The MTF + zero-run RLE + adaptive-arithmetic postcoder may be a large share of
BWT-path decode, in which case the entropy-coder swap (D4) is the only remaining speed lever
short of a new inverse algorithm.

**Mechanism.** A **differential, same-job** measurement — **not** callgrind, which failed four
times on the hosted runner. For a fixed always-BWT file, measure in one job:
(a) the postcoder stage alone (MTF/RLE/entropy → the BWT string), (b) the inverse BWT alone over
that same BWT string, (c) the end-to-end decode, (d) the reference `brotli_lw` one-shot decode of
the same bytes, and (e) ANVIL's **streaming** Brotli decode (256 KiB output window) versus the
reference's one-shot full-buffer decode — which also settles whether the 256 KiB window in
`brotli_lw_decode_stream` is a material cost.

**Decision rule (fixed in advance).**
> **If the postcoder is ≥ 35 % of BWT-path decode, D4 (the entropy-coder swap) is FUNDED and must
> be executed. If the postcoder is < 20 %, D4 is `NO-GO` on Amdahl grounds and the programme's
> speed effort pivots entirely to D3. If the 256 KiB streaming window costs ≥ 10 % against the
> one-shot reference decode, enlarging it is a mandatory part of any later arm.**

### D3 — Making the inverse BWT's `n/2` accesses locality-bounded

**Hypothesis.** The inverse BWT's cost is a *count* of `n/2` dependent random accesses into a
`4n`-byte array (I20, source-verified). A representation or algorithm that bounds the working set
— rather than a finer checkpoint grid, which provably cannot change the count — would raise the
BWT path off its ≈32 MB/s floor.

**Candidate mechanisms to be trialled, in this order** (each is a prototype under
`prototypes/i11-inverse-locality/`, never a silent `src/anvil.cpp` change):
1. **Blocked / two-pass inverse.** Invert in windows of `W` bytes so the LF array slice is
   cache-resident, at the cost of an extra pass and a boundary-correction rule.
2. **Narrower index.** Determine whether the LF array can be narrowed (libsais mandates `int32`) or
   whether a self-describing index with a smaller per-entry cost permits a smaller working set.
3. **Post-transform reorder.** Apply a cheap reversible transform *before* the BWT so that the LF
   walk's accesses become local (the transform is part of the payload, so the decoder inverts it).

**Decision rule (fixed in advance).**
> **A mechanism is adopted iff it decodes the canonical always-BWT corpus files at **≥ 1.5× the
> baseline BWT-path speed** with **byte-identical reconstructed output** and **byte-identical
> compressed output** to the baseline, and the improvement is measured **same-job paired** with a
> valid A/A null. Otherwise the mechanism is `NO-GO` with the measured number recorded. A
> mechanism that changes the compressed output is **not** a locality mechanism and is out of scope
> for D3 (it belongs to a byte-mechanism candidate).**

### D4 — The postcoder entropy-coder swap (funded only by D2)

**Hypothesis.** Re-coding the **same** MTF token stream with a cheaper member of ANVIL's own
stream suite (rANS 256/512/4096, canonical Huffman, default-with-exceptions) trades bytes for
decode throughput.

**Pre-condition (binding).** **D2 must have measured the postcoder's share at ≥ 35 %.** If D2 has
not, D4 does not run and is recorded as `INCONCLUSIVE` (not `NO-GO` — the hypothesis is untested).

**Decision rule (fixed in advance).**
> **Adopt iff the charged complete-byte cost fits inside the Silesia byte surplus left by the
> operating profile, the BWT-path decode improves by **≥ 1.3×** measured same-job paired against
> the shipped postcoder with a valid A/A null, and every corpus file round-trips byte-exactly
> through the new postcoder. The byte cost is charged in full; a marginal delta is never charged.**

**Prior-art discipline.** An entropy-coder swap is **occupied prior art** (Brotli, zstd, FSE, ANS
all do it). D4 is `{adopt-class}` engineering and claims no novelty.

### D5 — Extend the arbiter to rule the encode axis and the decoder-only binary-size axis

**Hypothesis.** The I20 arbiter's own `scope_note` says it "does not establish a crossing because
encode and decoder-only binary-size axes are not ruled here". **A genuine dominance or
FRONT-CROSSING claim is impossible until those two axes are ruled.** D5 builds that extension.

**Mechanism.** Extend `tools/reference_cost_gate.py` with **two additional axes and no change to any
existing threshold**: whole-corpus encode wall time (candidate ≤ every reference point) and
decoder-only binary size (candidate's decoder ≤ every reference point's decoder, measured as the
size of the decoder binary actually used, with the measurement method and the compiler flags
recorded). The extension must be a **strict superset** of the current arbiter.

**Decision rule (fixed in advance).**
> **D5 is adopted iff the extended arbiter (a) changes **no** existing verdict — re-running it on
> the frozen I20 baseline must reproduce `FRONT-GAP_COST` on Silesia and enwik8 exactly, and the
> I20 closure arm must still reproduce `COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE` — and (b) emits the
> two new axes for every arm. If (a) fails, D5 is `NO-GO` and the dominance claim is stated with
> the scope note instead.**

**Hard constraint.** **No existing threshold may be relaxed, widened or re-weighted.** The new
axes may only *add* conditions. Any change to an existing threshold is a programme failure, not a
tuning decision.

### D6 — Close the local Class-A grid's last FRONT-GAP cells

**Hypothesis.** The five FRONT-GAP cells (all on `tests/corpus/generated.json`, encode plane, all
Brotli/q11 or zstd/19 bracket fills) can be closed to a real FRONT-CROSSING by an ANVIL
configuration that is non-dominated, outside the reference bracket, and not degenerate.

**Decision rule (fixed in advance).**
> **D6 is adopted iff, under the unmodified `tools/pareto_front.py` + `tools/beats_brotli.py` and
> the frozen classification in `docs/gate-ruling-i9-frontier-tokens.md`, the co-listed tuple moves
> to **≥ 1 FRONT-CROSSING**, quoted **whole** with its **grid provenance** attached, the crossing row
> is non-dominated against the **v2 reference class (brotli + zstd + xz -9e)**, the **dual-bar
> same-transform control** applies to any synthetic cell, and the row's `ratio` is **< 0.95** (not
> DEGENERATE). The `GRID-THIN` label must accompany the citation and the unmeasured reference
> families must be listed.**

**Non-goal.** D6 must not change the frozen arbiter; it changes only *candidates*.

---

## 6. Paper-rejected candidates (not dispatched; reasons recorded)

| Rejected | Reason |
|---|---|
| A finer auxiliary-index checkpoint grid as a *speed* lever | **Mechanism-level NO-GO, source-verified**: the LF-step count is `n/2` for every `r`; fitted `f = 0.000 ± 0.02` (run `39676039083`) |
| Re-running the existing sub-block cap sweep on enwik8 | Forbidden by §1.4: the caps are exhausted there; enwik8 needs a new representation or a new inverse strategy |
| Relaxing the arbiter's 2.0× decode or memory margins | Programme constraint 11: no threshold relaxation, ever |
| Counting the I20 Silesia closure as a dominance or FRONT-CROSSING claim | The I20 arbiter's scope note excludes the encode and decoder-binary-size axes; D5 exists to fix that |
| Re-running the QLFC (postcoder 4) or LZP (postcoder 5) byte experiments | Closed by the do-not-reburn register §G2 |
| Re-running the C2 walk-budget sweep as a throughput experiment | Closed by D1/D2's own evidence; the knob is a byte knob |
| TCOPY / PNRA variants as an inverse-BWT speed mechanism | Closed as novelty by the prior-art audit; and a transform before the BWT changes the compressed output, so it is not a D3 locality mechanism |
| The auxiliary-index walk budget as an *encode-time* lever | The walk budget is an encoder policy over a sampling rate; its encode cost was never measured and is bounded by the `--bwt-aux-walks` default remaining 1024 |

---

## 7. Records to be updated at close

- `RESEARCH_LEDGER.md` — one entry per candidate: hypothesis, mechanism, exact charged bytes,
  measured decode/RSS/encode/binary-size, verdict, falsifying reason for every non-win.
- `docs/I11-PARETO-DOMINANCE-RESULTS-2026-10-22.md` — the results, with raw artifact quotes and
  the six mandatory evidence fields per number.
- `docs/I11-PARETO-DOMINANCE-PROGRAM.md` — the programme report and the next ranked leads.
- `tests/i11-evidence/MANIFEST.{json,md}` — the machine-generated evidence manifest.
- `FORMAT.md` — only if an adopted change is wire-visible (none of D1–D4 is expected to be).
- `tools/reference_cost_gate.py` — only if D5 is adopted, and then as a strict superset.

---

*Preregistered 2026-10-22. Every threshold, decision rule and kill condition above is fixed before
measurement and will not be altered after results are observed.*
