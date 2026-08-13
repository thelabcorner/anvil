# ANVIL — Novelty-Gated Research Agenda (v1)

Owner: `research` lane. This document is the gatekeeper artifact for every
mechanism ANVIL may implement. Nothing enters `src/anvil.cpp` as a claimed
win unless it clears the four-step novelty gate below and its ablation is
recorded in `RESEARCH_LEDGER.md`.

## 0. The novelty gate (mandatory for every mechanism)

Per `docs/CONTEXT.md`, a candidate mechanism must state:

1. **Prior-art lineage** — what exists, why it lost, what Brotli/Zstd exploit.
2. **What is NEW** — a precise, mechanism-level statement. A renamed
   primitive, a new file format, or an arbitrary combination of known parts
   does NOT clear the gate.
3. **Why it should move the Pareto frontier** — a falsifiable claim about
   ratio vs. throughput, not just "it compresses better".
4. **Ablation** — an experiment isolating the mechanism (A/B with the rest of
   the pipeline fixed), showing the gain is from the mechanism, not noise.

Rule of thumb from the ground rules: *stand on prior art; re-test abandoned
ideas on modern hardware; separate mathematical limits from
implementation-era limits.*

---

## 1. SPARSE-REF — prior-art verdict (the flagship mechanism)

**Working title:** approximate self-reference with sparse correction.
Copy a prior phrase (self-referential, no byte-identity requirement) and
entropy-code a sparse correction mask + residuals. Decoder ≈ memcpy + sparse
stores. Joint MDL parse over exact-LZ edges AND sparse-corrected phrase edges.

### 1.1 Prior-art lineage

| Prior art | What it does | Why it did not become general-purpose |
|---|---|---|
| **bsdiff** (Percival, 2003) | Binary *differencing* (two files: old→new). Suffix-sorted match, then an "add" array of bytewise differences corrects copy-with-errors. | Two-file delta, not self-referential single-file compression. Correction array is a flat diff, not an entropy-coded sparse mask; no topology modeling; suffix-sort cost is heavy (not single-pass). |
| **Zdelta** (Trendafilov, Memon, Suel, 2002) | LZ77-based *delta* compression that explicitly allows mismatches: a COPY instruction is followed by an encoding of each mismatch position and value. | Delta (two-file) setting; mismatch positions coded as a fixed per-COPY list, not modeled as a reusable topology; web/source-target use case. |
| **GenCompress / CTW+LZ / DNAPack** (Chen et al. 1999; Matsumoto/Sadakane/Imai 2000) | Self-referential *single-sequence* compression using approximate repeats: find near-exact repeats, encode edit operations (substitutions etc.) for the mismatches. | Domain-specific (alphabet ≈ 4, long low-entropy repeats). Corrections coded as edit operations; no general byte-alphabet sparse-mask stream; no entropy modeling of correction *positions*. |
| **Relative genome compression** (Deorowicz & Grabowski 2011; MBGC2, Kowalski 2026) | LZ77-style with 1+ single-character mismatches allowed per match, for genome collections. | Multi-sequence/collection setting, small alphabet, specialized to genomes. Confirms the "LZ with mismatches" idea is active in niche domains but has not been generalized. |
| **Pattern-matching LZ with mismatches** (Navarro & Raffinot 2004; Atallah et al. 1995) | Theory: search/compress allowing k mismatches; probabilistic analysis. | Algorithmic/theoretical; not a shipped general-purpose codec with an entropy-coded correction stream. |
| **Brotli** (Alakuijala & Szabadka 2013–) | LZ77 + context modeling (context map, 2D distance/literal contexts) + ~120 KiB static dictionary + block switching + Huffman. | *Exact-match* LZ77 only. On "almost-identical records" (e.g., JSON rows that differ in one field value), the differing bytes fall out of match coverage and must be literal-coded — the context model helps but the record skeleton is re-encoded per record. Encode at q9–q11 is ~1–4 MB/s (not Pareto). |
| **Zstd** (Collet 2015–) | LZ77 + FSE/tANS, 3 repcodes (recent-offset reuse), trained dictionaries, long-distance mode. | Also *exact-match* only. Repcodes exploit *recent offsets*, not *structural distance across non-identical records*. |
| **LZMA** (Pavlov 1998–) | LZ77 + range coder + 4 repcodes + binary-tree match finder + optimal parse (expensive). | Exact-match. Optimal parse is the multi-pass cost that ANVIL already rejected as a throughput gate. |

### 1.2 What is NEW (the specific formulation)

The building blocks — copy-with-errors, mismatch lists, approximate repeats —
are established. What is **not** established for *general-purpose, single-file,
self-referential* lossless compression is this combination:

1. **A sparse correction mask as a first-class entropy-coded stream.** The
   copy is a phrase pointer; the corrections are a compact mask of
   *positions* + a residuals stream of the differing bytes. This is unlike
   bsdiff's flat add-array and Zdelta's per-COPY mismatch list.
2. **Entropy coding of the correction *topology* itself.** In record-based
   structured data (JSON/logs/SQLite), the *positions* of corrections recur
   across records (the same field changes every row; the rest is skeleton).
   A model over correction masks — order-1 over previous masks / run-length
   of uncorrected spans — can compress the mask below its flat entropy.
   "Recurring correction topology can be entropy-coded" is, to our
   knowledge, not in the prior art for general-purpose codecs.
3. **Structural-distance propagation.** A sparse-corrected phrase edge can
   propagate a *recently discovered* structural distance (e.g., a record
   period) across non-identical records: once the parser learns "records are
   ~92 B apart with a value field at offset 40", it can issue
   distance=d copy + a 1-byte correction for every subsequent record — an
   edge type that exact LZ cannot express.
4. **Joint MDL parse over both edge types with measured downstream rANS
   cost.** The parser scores exact-LZ edges and sparse-corrected edges in one
   MDL objective whose cost model comes from the actual entropy backend
   (static rANS stream sizes), not a heuristic.

Claims 1–3 are the mechanism-level novelty. Claim 4 is the systems-level
interaction (parser ↔ representation ↔ entropy backend) that makes the
mechanism shippable.

### 1.3 Why it should move the Pareto frontier

- Brotli's structured-data ratio edge comes from context modeling + a large
  static dictionary, paid for with expensive encode (q9–q11: 1–4 MB/s). On
  generated JSON, ANVIL dp-rans already *ties* Brotli q9 ratio (0.138 vs
  0.137) while being ~10x slower at encode.
- SPARSE-REF attacks the *other* axis: keep decode LZ-fast (memcpy + sparse
  stores + rANS — all vectorizable, single-pass), keep encode cheap
  (hash-chain match finder, not global DP), and recover structured-data ratio
  from *repetition structure* instead of context models. If an MDL parse can
  pick sparse-corrected edges where exact LZ fails, ratio improves at
  near-LZ throughput rather than near-CM throughput.
- Falsifiable target: on record-structured corpora (generated.json,
  SQLite, logs), SPARSE-REF should reach within a few % of Brotli q9 ratio at
  ≥ 10x its encode speed and ≥ 3x its decode speed — i.e., beat Brotli on the
  ratio/throughput plane where Brotli currently owns the corner. On
  incompressible data it must degrade to the exact-LZ baseline (mask is
  empty or no edge chosen — no regression).

### 1.4 Ablation protocol (commits for `arch`)

- A/B with the exact-LZ pipeline fixed: `sparse-ref ON vs OFF` on the same
  parser/entropy backend, same corpus. Report Δratio, Δencode, Δdecode.
- Mask-topology ablation: flat mask entropy-coding vs. order-1/run-length
  topology model. Isolates claim 2.
- Edge-type ablation: exact-only parse vs. exact+sparse-corrected edges.
  Isolates claims 1+3 (does the *option* of sparse edges help, even if the
  final block is identical?).
- Distance-propagation ablation: with/without structural-distance reuse.
- Regression guard: full-corpus delta must be ≥ noise floor (≥3 median reps,
  round-trip verified, fuzzed) before any claim.
- If the mechanism does not clear its falsifiable target, the ledger records
  WHY (math vs. implementation-era), per working agreement.

---

## 2. Ranked mechanism agenda (all pass the gate)

Ranking = novelty strength × expected Pareto shift × implementation cost
(low rank = highest priority to build/measure).

| # | Mechanism | Novelty | Expected Pareto shift | Est. cost | Verdict |
|---|---|---|---|---|---|
| 1 | SPARSE-REF (sparse-corrected phrase copy) | HIGH (claims 1+3+4) | ratio on structured data at LZ speed | med | **PASS** — flagship |
| 2 | Correction-topology entropy coding | HIGH (claim 2) | further ratio on repetitive-structured | low-med | **PASS** — sub-mechanism of 1 |
| 3 | Single-pass cache-resident MDL parser (measured rANS costs) | MED-HIGH | encode speed (kills the DP bottleneck) | high | **PASS** — throughput pillar |
| 4 | Structural-distance propagation | MED-HIGH (claim 3) | ratio where record periods recur | low | **PASS** — sub-mechanism of 1 |
| 5 | Self-extracted cross-block phrase dictionary, MDL-gated | MED | ratio on multi-block files | med | **PASS** (conditional) |
| 6 | Distance repcoding (class + bucket + recency) | LOW-MED (interaction with 1) | ratio via cheaper distances | low | **PASS** (as interaction, not alone) |
| 7 | Confidence-gated literal models (re-test, modern gating) | LOW-MED | small ratio on text | med | **PASS** (re-test of rejected idea) |

Details and gate entries for each:

### R1. SPARSE-REF — sparse-corrected phrase copy
See §1. Highest priority: it is the mechanism the swarm exists to test.

### R2. Correction-topology entropy coding
- **Lineage:** flat mismatch lists (Zdelta), flat diff arrays (bsdiff), edit
  ops (DNA compressors). No general-purpose codec models the *distribution of
  correction positions across repeated records*.
- **NEW:** a dedicated correction-mask stream (e.g., run-length/order-1 over
  mask words, or per-record mask deltas) that exploits topology recurrence.
- **Why Pareto:** corrections are usually a tiny fraction of the phrase; the
  mask, not the residuals, becomes the dominant correction cost — compressing
  the mask is pure ratio win with near-zero decoder cost.
- **Ablation:** mask flat vs. modeled; measure Δratio and Δdecode on
  generated.json / sqlite / logs.

### R3. Single-pass cache-resident MDL parser with measured rANS costs
- **Lineage:** LZMA's optimal parse (multi-pass, expensive); Brotli/zstd use
  greedy + heuristics; ANVIL's own dp/dpsa (correct but 1–5 MB/s). The
  lesson carried in CONTEXT.md: the global DP parser is the bottleneck.
- **NEW:** a *cache-resident* parser whose per-edge costs come from the
  actual downstream rANS stream construction (measured, not sampled), doing
  bounded lookahead / iterative refinement in a single pass over a block —
  MDL-quality decisions at greedy-class speed. The "measured-entropy-cost,
  single-pass, cache-resident" combination is the new claim.
- **Why Pareto:** kills the encode-speed gate directly (from ~3 MB/s to
  LZ-class), while keeping dp-class ratio where the downstream cost model is
  faithful.
- **Ablation:** greedy vs. dp vs. new parser on the same rANS backend;
  report Δratio/Δencode; verify cost-model fidelity (predicted vs. actual
  stream size).

### R4. Structural-distance propagation
- **Lineage:** LZMA/Brotli/zstd repcodes reuse *recent offsets*; none reuse a
  *structural distance discovered once and applied across non-identical
  records* (exact LZ cannot — the phrase isn't byte-identical).
- **NEW:** sparse-corrected edges carry a learned record period; the parser
  can issue "distance=d + small correction" for every record after one good
  exemplar. Distance now encodes *structure*, not just recency.
- **Why Pareto:** this is the mechanism that turns "repetitive but not
  identical" files (JSON logs, SQLite, CSVs) from context-model territory
  into LZ territory.
- **Ablation:** with/without structural-distance edge generation (R1 A/B +
  targeted corpus).

### R5. Self-extracted cross-block phrase dictionary (dual-timescale), MDL-gated
- **Lineage:** Brotli ships a fixed ~120 KiB trained dictionary; Zstd supports
  externally trained dictionaries. Both are *static/external* and
  *exact-match*.
- **NEW:** ANVIL extracts a phrase dictionary from *the input's own earlier
  blocks* (in-band, no training data), and the block router keeps it only if
  MDL says the dictionary header amortizes. Interaction with SPARSE-REF
  (dictionary entries can also be sparse-corrected) is not in the prior art.
- **Why Pareto:** long-range reuse on multi-block files (logs, DBs) without
  shipping a trained dictionary or needing an external corpus.
- **Ablation:** with/without cross-block dict; dict-vs-no-dict on ≥2-block
  files; reject if header cost > savings.

### R6. Distance repcoding (class + bucket + low bits + recency)
- **Lineage:** LZMA repcodes, Brotli distance context maps, zstd repcodes,
  ANVIL's earlier dp2 recency coding. Alone: NOT novel.
- **NEW (as interaction):** distance classes tuned jointly with SPARSE-REF's
  structural-distance edge — a distance alphabet that is a *mixture* of
  recency slots and structural periods, entropy-coded in one model.
- **Why Pareto:** cheaper distance coding compounds every sparse-corrected
  edge; low risk, low cost.
- **Ablation:** distance model A/B on the same token stream; report Δratio.

### R7. Confidence-gated literal models (re-test with modern gating)
- **Lineage:** ANVIL Experiment C rejected unconditional order-1 (+3.46%).
  PPM escape/confidence mechanisms are the classic answer; Brotli uses
  context maps.
- **NEW:** cheap confidence gating over a *small* set of record-relative
  literal contexts (e.g., "inside a value field at structural offset X"),
  which only makes sense once structural positions exist (R4). Re-test of an
  abandoned idea *with the new representation* providing the contexts.
- **Why Pareto:** literal bytes in correction residuals are the one place
  context helps; gating keeps cold-start sparsity away.
- **Ablation:** o0 vs. gated-o1 on residuals only; reject unless end-to-end
  bytes improve (same rule as Experiment C).

---

## 3. Mechanisms that DO NOT clear the gate (recorded, not pursued)

| Candidate | Lineage | Why it fails the gate |
|---|---|---|
| Static rANS 4-way interleave (rans2x4) | Duda ANS; standard interleaving (Giesen ryg_rans, k-rANS) | Pure engineering; already implemented as backend infra, not a mechanism claim |
| Literal xor/delta transforms (rans2t) | DPCM / IFF 8SVX; xz --delta; TSDB XOR | Alone = standard transform; only keep as router option, no novelty claim |
| Unconditional order-1 literals | PPM lineage | Already ablated and rejected (+3.46%); re-trying without a new context source is repetition |
| Brute-force block router (`--parse=auto`) | Research search | Arbitrary combination of known components; production default must be single-candidate |
| Bigger hash table / longer chain | Standard LZ engineering | Implementation tuning, not mechanism |
| Generic arithmetic vs rANS | Fenwick arith vs ANS | Backend choice already measured (8.1x decode); not a novelty item |

---

## 4. Ablation & falsifiability protocol (shared with bench/arch/format)

1. Every claim: encode+decode round-trip verified, fuzzed (tests/fuzz.py),
   median of ≥3 reps, corpus = tests/corpus/* (md, cpp, json, sqlite, log,
   random + any new structured files bench adds).
2. Mechanism ablations are A/B with **everything else fixed**; record the
   exact CLI flags in the ledger row so it is reproducible.
3. Noise floor: a mechanism's Δratio must exceed run-to-run variance on the
   same file (bench owns the variance estimate from ≥3 reps).
4. Throughput measured with build\anvil_bench.exe; ratio with the codec's own
   byte counts (no cross-tool inconsistencies).
5. Drop decisions record math-vs-implementation-era in RESEARCH_LEDGER.md.
6. Pareto claim = a row strictly inside/left of the Brotli/Zstd front on the
   ratio-vs-decode and ratio-vs-encode planes, per-file and aggregate.

## 5. Open questions for peers (feeding t-sparse / t-format / t-bench)

- **arch:** Prototype sparse-corrected copy as a *new block mode* (do not
  touch existing modes' wire format). Candidate mask representations to
  measure: (a) flat bitmask word per 32 bytes, (b) run-length of uncorrected
  spans, (c) per-record mask deltas (order-1 topology). Start with (a) —
  cheapest to ship; the topology models are R2 follow-ups.
- **format:** New block mode needs strict bounds: mask length ≤ copy length,
  residual count == popcount(mask), distance validity, record-period sanity,
  CRC over the reconstructed block. Decoder must reject truncated mask /
  residual streams.
- **bench:** Add at least one strongly *record-structured* corpus file
  (generated.log / generated.sqlite exist; consider a CSV or multi-record
  JSON-lines file) so SPARSE-REF's target is measured, not assumed.

---

*Status: v1 agenda. SPARSE-REF verdict: **PASS** the novelty gate as a
mechanism-level combination (claims 1–3 new; claim 4 the shippable
interaction); building blocks (bsdiff/Zdelta/DNA-approximate-LZ) are
acknowledged prior art. Ranked list = build order for t-sparse → t-parser →
t-bench → t-ledger.*
