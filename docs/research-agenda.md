# ANVIL — Novelty-Gated Research Agenda (v2)

> **Historical agenda / superseded ranking.** Keep this file as mechanism provenance, but do not use its ranking table as the current queue. Later I8/I9 evidence closed several items listed here. The synthesized post-I9 research direction and binding do-not-reburn constraints are in `docs/FRONTIER-RESET-2026-09-23.md`, `docs/I10-BREAKTHROUGH-PROGRAM.md`, `docs/anvil-i9-findings.md`, and `docs/audit-2026-09-07/06-do-not-reburn.md`.

Owner: `research` lane. This document is the gatekeeper artifact for every
mechanism ANVIL may implement. Nothing enters `src/anvil.cpp` as a claimed
win unless it clears the four-step novelty gate below and its ablation is
recorded in `RESEARCH_LEDGER.md`.

**v2 changelog (evidence injection, Linux continuation v2 — see
docs/CONTEXT.md §"Linux continuation (v2)"):** SPARSE-REF upgraded from
hypothesis to **ratio-validated**; new validated sub-mechanisms added
(shape-conditioned displacement P(d|s), mutation-template/slot-default
residual coding, boundary-aligned candidates, difference-cover negative
gate); enabling-technology target (precision-adaptive entropy) confirmed;
fail-gate additions recorded. **The gate stays the arbiter: "ratio beat" is
NOT "Pareto win" — see §1.3 flags.**

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

**Status: RATIO-VALIDATED (Linux v2), Pareto UNPROVEN.** Building blocks are
implemented and measured (RCM/SCM/SRR parsers, patch-phrase copy, clustered
residual backend); see §1.3 for the evidence and the explicit flags on what it
does and does not support.

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
   *[Linux v2: implemented as COPY_PATCH(dist,len,S,res) + clustered residual
   backend — validated.]*
2. **Entropy coding of the correction *topology* itself.** In record-based
   structured data (JSON/logs/SQLite), the *positions* of corrections recur
   across records (the same field changes every row; the rest is skeleton).
   A model over correction masks — order-1 over previous masks / run-length
   of uncorrected spans — can compress the mask below its flat entropy.
   "Recurring correction topology can be entropy-coded" is, to our
   knowledge, not in the prior art for general-purpose codecs.
   *[Linux v2: slot-default analysis gives HARD evidence — 128 recurring
   patch masks, 598 (mask,slot) contexts, modal residual accuracy 86.5% on
   generated.log. Validated as a coding target; the mutation-template
   encoder itself is R2.]*
3. **Structural-distance propagation.** A sparse-corrected phrase edge can
   propagate a *recently discovered* structural distance (e.g., a record
   period) across non-identical records: once the parser learns "records are
   ~92 B apart with a value field at offset 40", it can issue
   distance=d copy + a 1-byte correction for every subsequent record — an
   edge type that exact LZ cannot express.
   *[Linux v2: SRR/SSCM "synchronized structural distance discovery" +
   persistent channel bank (12 slots, score decay) implemented; distance
   analysis shows recency alone is weak (top-16 MRU ≈ 11-20%) — structural
   channels are the mechanism that supplies the "recently discovered
   structural distance". Validated directionally.]*
4. **Joint MDL parse over both edge types with measured downstream rANS
   cost.** The parser scores exact-LZ edges and sparse-corrected edges in one
   MDL objective whose cost model comes from the actual entropy backend
   (static rANS stream sizes), not a heuristic.
   *[Linux v2: RCM is single-pass with 8-entry MRU distance cache + 64-bit
   XOR word compares — no DP/SA; parser family validated. Full
   measured-cost MDL (R3) still to be proven.]*

Claims 1–3 are the mechanism-level novelty. Claim 4 is the systems-level
interaction (parser ↔ representation ↔ entropy backend) that makes the
mechanism shippable.

### 1.3 Evidence and Pareto status — what IS and ISN'T supported

**Supported (Linux v2, directional numbers):**

- **generated.log** (1,924,280 B): SCM + stream-suite + macro-op mode 23 →
  **0.056 ratio, 47.4 MB/s encode, 913 MB/s decode** vs Brotli q9 0.065 /
  27.6 / 1,460. Ratio AND encode beat; **decode loses ~1.6x**.
  Raw-hot + flat-decoder experiment reached 0.042 / 50 / 963 (single
  experiment, not default config).
- **generated.json** (827,664 B): SRR + shape-conditioned displacement →
  **109,700 B (0.1325) @ ~541 MB/s decode** vs Brotli q9 113,284 B (0.137).
  Ratio beat; **decode loses ~2x** (Brotli q9 decode ≈ 956 MB/s on this file
  per Windows suite) and encode also slower.
- **Slot-default analysis** (generated.log): 86.5% modal-residual accuracy
  across 598 (mask,slot) contexts → the R2 topology-coding target has hard
  evidence behind it.
- **Distance anatomy** (generated.json): unconditional displacement ~13 bits,
  shape-conditioned log-proxy ~9.4 bits; top-1020 shapes 6.4% exact / 44.6%
  within 64 → **P(d|s) is a real, exploitable signal**.
- **Boundary alignment**: 66–92% of LZ reference starts land within ±8 B of a
  prior token start → boundary-indexed candidate generation is cheap and
  effective.
- **Negative gate**: cyclic difference-cover probe (mod-64, 10 phases) +
  content-hash thinning → random.bin encodes at **3,341 MB/s** vs 0.66 MB/s
  Brotli q11, while awkward-displacement repeats are still detected.

**NOT supported (flags — the gate stays the arbiter):**

- **FLAG-A — no Pareto win yet.** The falsifiable target in §1.3 (v1) was
  "within a few % of Brotli q9 ratio at ≥10x encode AND ≥3x decode speed."
  Linux v2 achieves the ratio leg but **decode is still ~1.5–2x slower than
  Brotli q9** on the two measured files. On the ratio-vs-decode plane these
  rows do NOT extend the front. Decode throughput (not ratio) is now the
  binding constraint — the falsifiable target must be re-stated around
  decode (see §1.4).
- **FLAG-B — two files, directional host.** Evidence is generated.log +
  generated.json on a Linux EPYC host; numbers are directional for Windows.
  No full-corpus evidence yet (md/cpp/sqlite/random/repeat control), so no
  no-regression claim can be made. The Windows full-corpus regression
  (t-bench) with the ported modes is the Pareto arbiter.
- **FLAG-C — not all numbers are A/B.** Raw-hot/flat-decoder (0.042) and
  some parser variants were one-off Linux builds; per §4.2 the ledger claim
  requires A/B with everything else fixed. Treat headline numbers as
  directional until reproduced on the Windows tree with the agreed protocol.
- **FLAG-D — P(d|s) lineage must be stated precisely** (see R4): Brotli
  already uses distance *context maps* and LZMA uses match-state-dependent
  distance coding. The NEW claim is *per-shape displacement state with
  signed-delta re-use* driven by a semantic opcode, not a generic context
  map. Novelty is real but narrow; the ablation must separate "shape" from
  "context-map-equivalent".

### 1.4 Ablation protocol (commits for `arch`) — updated for v2

- A/B with the exact-LZ pipeline fixed: `sparse-ref ON vs OFF` on the same
  parser/entropy backend, same corpus. Report Δratio, Δencode, Δdecode.
- Mask-topology ablation: flat mask entropy-coding vs. order-1/run-length
  topology model vs. slot-default/mutation-template (R2). Isolates claim 2.
- Edge-type ablation: exact-only parse vs. exact+sparse-corrected edges.
  Isolates claims 1+3.
- Distance-propagation ablation: global-recency-only vs. structural-channel
  vs. shape-conditioned P(d|s). Isolates claim 3 + R4. **Must separate
  P(d|s) from a generic context map (FLAG-D).**
- **Decode-throughput ablation (NEW, from FLAG-A):** decode cycles/byte for
  each representation choice (mask rep, residual backend, macro-ops vs
  separated streams). The gate now requires the decode leg, not just ratio.
- **No-regression guard (two levels — both required):**
  - *Mechanism level (t-sparse ablation):* A/B with everything else fixed;
    on the no-regression controls (generated.repeat.jsonl, random.bin) the
    mechanism row must not be larger than the exact-LZ baseline row of the
    SAME build (ratio Δ ≥ 0 = regression). Catches the mechanism itself.
  - *Release level (t-bench regression table):* every new mode appears in
    the full-corpus table INCLUDING the repeat/random controls, compared
    against the same-build exact-LZ baseline. Catches cross-mode interaction
    and default-enabled regressions (a mode may exist but not be the
    default if it regresses a control). Pass criterion: compressed bytes on
    repeat/random controls must be ≤ same-build baseline; on random the
    ratio is already ~1.0 so the relevant metric is encode speed with the
    negative gate, with ratio unchanged.
- Regression guard: full-corpus delta ≥ noise floor (ratio CV = 0.000% per
  bench, so ratio deltas are exact; timing only on usable files ≥ ~100 KB),
  round-trip verified, fuzzed — before any claim.
- If the mechanism does not clear its falsifiable target, the ledger records
  WHY (math vs. implementation-era), per working agreement.

---

## 2. Ranked mechanism agenda (all pass the gate)

Ranking = novelty strength × expected Pareto shift × implementation cost
(low rank = highest priority to build/measure). **v2: rankings updated with
Linux evidence; validated mechanisms marked [V], directional [D].**

| # | Mechanism | Novelty | Evidence | Expected Pareto shift | Verdict |
|---|---|---|---|---|---|
| 1 | SPARSE-REF phrase copy (COPY_PATCH) | HIGH (claims 1+3) | **[V]** log+JSON ratio beats | ratio ✓; decode leg ✗ | **PASS** — flagship, decode-limited |
| 2 | Correction-topology / mutation-template residual coding (R2) | HIGH (claim 2) | **[V]** 86.5% modal accuracy | ratio on residuals; ~0 decode cost | **PASS** — hard evidence |
| 3 | Single-pass cache-resident parser (RCM family) | MED-HIGH | **[D]** RCM/SCM/SRR implemented; encode 47.4 MB/s log | encode speed | **PASS** — throughput pillar |
| 4 | Shape-conditioned displacement prediction P(d|s) | MED-HIGH (narrow; FLAG-D) | **[V]** JSON 112,941→109,700 B, decode 534→541 | ratio on distances; small decode cost | **PASS** (conditional on ablation) |
| 5 | Structural-distance propagation (channels) | MED-HIGH (claim 3) | **[D]** SRR/SSCM; MRU weak (11-20%) | ratio where record periods recur | **PASS** — sub-mechanism of 1 |
| 6 | Boundary-aligned candidate generation | LOW-MED (search formulation) | **[V]** 66-92% within ±8 B | encode speed (hit rate), no ratio cost | **PASS** — cheap, adopt |
| 7 | Difference-cover negative gate | LOW-MED (search formulation) | **[V]** random 3,341 MB/s | encode speed on incompressible | **PASS** — cheap, adopt |
| 8 | Precision/work-adaptive entropy (stream suite) | MED (enabling; joint cost model is the claim) | **[D]** 22-stream anatomy; pair/Huffman dominate residuals | decode + model-build on rich streams | **PASS** (as enabling tech) |
| 9 | Self-extracted cross-block phrase dictionary, MDL-gated | MED | none yet | ratio on multi-block files | **PASS** (conditional) |
| 10 | Distance repcoding (class + bucket + recency) | LOW-MED (interaction with 1/4/5) | **[D]** MRU weak → classes needed | ratio via cheaper distances | **PASS** (as interaction) |
| 11 | Confidence-gated literal models (re-test, modern gating) | LOW-MED | none new | small ratio on text | **PASS** (re-test) |

Details and gate entries for each:

### R1. SPARSE-REF — sparse-corrected phrase copy [V]
See §1. Highest priority. **v2 status: ratio-validated, decode-limited
(FLAG-A).** Build order on Windows: port COPY_PATCH + RCM/SCM as new modes;
measure decode cycles/byte before claiming Pareto.

### R2. Correction-topology / mutation-template residual coding [V]
- **Lineage:** flat mismatch lists (Zdelta), flat diff arrays (bsdiff), edit
  ops (DNA compressors), PPM-style template/escape coding. No general-purpose
  codec models the *distribution of correction positions* across repeated
  records.
- **NEW:** a per-(mask,slot) **modal residual as decoder-visible default** +
  exception mask ("mutation template"). The decoder emits the slot default and
  only sparse exceptions cost bits — topology + residual value coded jointly.
  *[V: 86.5% modal accuracy across 598 (mask,slot) contexts on logs.]*
- **Why Pareto:** residuals are the dominant stream; default-then-exceptions
  collapses most of it at near-zero decode cost (default is a table lookup).
- **Ablation:** flat residuals vs. slot-default vs. order-1 mask model; report
  Δratio and Δdecode on generated.jsonl/sqlite/logs.

### R3. Single-pass cache-resident parser (RCM family) [D]
- **Lineage:** LZMA optimal parse (multi-pass); Brotli/zstd greedy+heuristics;
  ANVIL dp/dpsa (1–5 MB/s). CONTEXT.md: the global DP parser is the
  bottleneck.
- **NEW:** RCM = single-pass, 8-entry MRU distance cache, 64-bit XOR word
  compares, no DP/SA — greedy-class speed with patch-phrase edges.
  SCM/SRR add encoder-side structural channels (bitstream unchanged).
- **Why Pareto:** kills the encode-speed gate (v2: 47.4 MB/s on log vs 27.6
  Brotli q9); keeps ratio where the downstream cost model is faithful.
- **Ablation:** greedy vs. dp vs. RCM/SCM on the same rANS backend; Δratio /
  Δencode; cost-model fidelity (predicted vs. actual stream size).

### R4. Shape-conditioned displacement prediction P(d|s) [V] (narrow novelty — FLAG-D)
- **Lineage:** Brotli distance context maps; LZMA match-state distance coding;
  zstd repcodes; ANVIL dp2 recency. Distance prediction with context is NOT
  new.
- **NEW:** a hot semantic opcode selects a **tiny per-shape displacement
  state**; first occurrence absolute, later = signed delta from the shape's
  last displacement (zigzag + class/extra). Distance is predicted by *what
  kind of token* it is, not a generic context map. *[V: JSON 112,941 →
  109,700 B, decode 534 → 541 MB/s; top-1020 shapes 6.4% exact / 44.6% within
  64.]*
- **Why Pareto:** distance streams are large (distance_code 15,680 B on log);
  ~13 → ~9.4 bits/displacement is pure ratio at tiny decode cost.
- **Ablation (mandatory, per FLAG-D):** P(d|s) vs. an equivalent-size generic
  context map vs. global recency. Keep only if per-shape beats
  context-map-equivalent end-to-end — otherwise it is a renamed context map
  and fails the gate.

### R5. Structural-distance propagation (persistent channels) [D]
- **Lineage:** repcodes reuse recent offsets; none reuse a structural
  distance across non-identical records (exact LZ cannot).
- **NEW:** SCM channel bank (12 slots, score decays with bytes elapsed,
  reinforced by long exact + successful patch phrases) supplies
  "recently-discovered structural distance" to patch-phrase edges.
  *[D: MRU weak — top-16 ≈ 11-20% — so channels, not recency, carry the
  signal.]*
- **Why Pareto:** turns "repetitive but not identical" files from
  context-model territory into LZ territory.
- **Ablation:** with/without channel bank; MRU-only vs. channels; reinforce
  rule A/B.

### R6. Boundary-aligned candidate generation [V]
- **Lineage:** hash-chain heads, suffix-array sampling, block-sorting
  heuristics — candidate generation from data positions is standard.
- **NEW (search formulation):** since 66–92% of LZ reference starts fall
  within ±8 B of a prior token start, generate candidates from
  token-boundary-indexed positions instead of every byte.
- **Why Pareto:** raises hit rate per probe, cuts match-finder cost — encode
  speed with no ratio cost.
- **Ablation:** byte-indexed vs. boundary-indexed candidate sets; Δencode +
  hit-rate; ratio must be unchanged.

### R7. Difference-cover negative gate [V]
- **Lineage:** deflate stored-block decisions, zstd/lz4 incompressible
  detection, rsync rolling checksums — "skip when it won't help" is known.
- **NEW (search formulation):** cyclic difference-cover probe (mod-64, 10
  phases) + content-hash thinning rejects incompressible blocks cheaply while
  still detecting repeat copies at awkward mod-64 displacements.
- **Why Pareto:** random.bin 3,341 MB/s encode vs 0.66 MB/s Brotli q11 — pure
  encode win on incompressible data, no ratio cost.
- **Ablation:** gate on/off on random + adversarial mod-64 repeats; Δencode,
  ratio unchanged.

### R8. Precision/work-adaptive entropy (enabling technology) [D]
- **Lineage:** rANS/ANS precision choices (Duda; Giesen ryg_rans; FSE);
  per-stream codec selection is partly explored in Brotli/zstd block types.
- **NEW (as a claim):** a *joint cost model* J = L_stream + λC_decode +
  μC_model-build + νW_cache selecting per stream among rANS (1/4-state ×
  full/compact), Huffman, default-with-sparse-exceptions, pair-rANS — "the
  smallest wins, but the cost function includes decode and model-build".
  4096-state rANS is overkill for small streams.
- **Why Pareto:** stream anatomy shows 22 entropy streams dominated by
  pair/Huffman on residual streams; precision-adaptive coding makes rich
  multi-model coding cheap to initialize/execute — benefits every stream.
- **Ablation:** fixed-rANS vs. stream-suite on each stream; Δratio, Δdecode,
  Δmodel-build; verify the J-cost predicts the winner.

### R9. Self-extracted cross-block phrase dictionary (dual-timescale), MDL-gated
- **Lineage:** Brotli ~120 KiB trained static dictionary; Zstd external
  trained dictionaries. Both static/external and exact-match.
- **NEW:** extract a phrase dictionary from *the input's own earlier blocks*
  (in-band, no training data); block router keeps it only if MDL says the
  header amortizes. Interaction with SPARSE-REF (correctable dict entries)
  not in prior art.
- **Why Pareto:** long-range reuse on multi-block files without shipping a
  trained dictionary.
- **Ablation:** with/without cross-block dict on ≥2-block files; reject if
  header cost > savings.

### R10. Distance repcoding (class + bucket + low bits + recency)
- **Lineage:** LZMA repcodes, Brotli distance maps, zstd repcodes, dp2. Alone:
  NOT novel.
- **NEW (as interaction):** distance alphabet mixing recency slots,
  structural channels (R5), and P(d|s) shapes (R4), entropy-coded in one
  model.
- **Why Pareto:** compounds every cheaper-distance mechanism; low risk.
- **Ablation:** distance model A/B on the same token stream.

### R11. Confidence-gated literal models (re-test with modern gating)
- **Lineage:** Experiment C rejected unconditional order-1 (+3.46%); PPM
  escapes; Brotli context maps.
- **NEW:** cheap gating over record-relative literal contexts (e.g., "inside
  a value field at structural offset X"), meaningful once R5 provides
  positions.
- **Why Pareto:** literal/residual bytes are the one place context helps;
  gating keeps cold-start sparsity away.
- **Ablation:** o0 vs. gated-o1 on residuals only; reject unless end-to-end
  bytes improve.

---

## 3. Mechanisms that DO NOT clear the gate (recorded, not pursued)

| Candidate | Lineage | Why it fails the gate |
|---|---|---|
| Static rANS 4-way interleave (rans2x4) | Duda ANS; standard interleaving (Giesen ryg_rans, k-rANS) | Pure engineering; already backend infra, not a mechanism claim |
| Literal xor/delta transforms (rans2t) | DPCM / IFF 8SVX; xz --delta; TSDB XOR | Alone = standard transform; router option only, no novelty claim |
| Unconditional order-1 literals | PPM lineage | Ablated and rejected (+3.46%); re-trying without a new context source is repetition |
| Brute-force block router (`--parse=auto`) | Research search | Arbitrary combination of known components; production default must be single-candidate |
| Bigger hash table / longer chain | Standard LZ engineering | Implementation tuning, not mechanism |
| Generic arithmetic vs rANS | Fenwick arith vs ANS | Backend choice already measured (8.1x decode); not a novelty item |
| Fused all-streams-live decoder cursors | v2 experiment, rejected | Single fused decode table per stream beats all-streams-live; measured failure, do not re-burn |
| MTR (mutational-template backend) as a standalone build | v2 experiment (build fixes pending) | Superseded by slot-default analysis (R2) — cheaper path to the same idea; implement R2, not MTR |

---

## 4. Ablation & falsifiability protocol (shared with bench/arch/format)

1. Every claim: encode+decode round-trip verified, fuzzed (tests/fuzz.py),
   median of ≥3 reps, corpus = tests/corpus/* (md, cpp, json, sqlite, log,
   random + new structured files bench adds).
2. Mechanism ablations are A/B with **everything else fixed**; record the
   exact CLI flags in the ledger row so it is reproducible. One-off Linux
   builds do NOT become ledger claims until reproduced (FLAG-C).
3. Noise floor (measured by bench, t-setup prep — blackboard
   bench/pareto-tooling): **ratio CV = 0.000% across all 13 codecs**
   (compressed bytes deterministic per build) → ratio deltas are exact, no
   variance term needed. **Timing is the only noise**: encode CV 0.9–17.3%
   by codec on generated.jsonl (anvil-dp-rans 1.6%); doc.md (small) unusable
   for throughput (up to 33% CV) → **small files excluded from throughput
   claims**. tools/pareto_front.py implements the §4.6 verdict
   (DOMINATED / EXTENDS_FRONT per file + plane + aggregate); baseline:
   every anvil row DOMINATED.
4. Throughput measured with build\anvil_bench.exe; ratio with the codec's own
   byte counts. Throughput claims only on files with usable timing CV
   (≥ ~100 KB).
5. Drop decisions record math-vs-implementation-era in RESEARCH_LEDGER.md.
6. Pareto claim = a row strictly inside/left of the Brotli/Zstd front on the
   ratio-vs-decode and ratio-vs-encode planes, per-file and aggregate, using
   tools/pareto_front.py (DOMINATED = no claim; EXTENDS_FRONT = candidate).
   **v2 (FLAG-A): ratio beat alone is insufficient — the row must not be
   DOMINATED on the decode plane either.**

## 5. Open questions for peers (feeding t-sparse / t-format / t-bench)

- **arch:** Port order from Linux v2 (already implemented there, modes
  19/23/26-28): COPY_PATCH + RCM/SCM/SRR first (R1/R3/R5), then clustered
  residual backend (mode 19) and macro-ops (mode 23). Decode cycles/byte is
  now the binding gate (FLAG-A) — measure it per representation choice.
  Re-use the v2 parsers as reference; do not re-derive.
- **format:** New modes need strict bounds: mask length ≤ copy length,
  residual count == popcount(mask), distance validity, record-period sanity,
  CRC over reconstructed block; reject truncated mask/residual streams.
  Multi-mode wire (19/23/26-28 + stream-suite selection) needs clear
  versioning. flag to `research` the exact P(d|s) wire (shape selectors,
  zigzag deltas) so R4's ablation can be reproduced.
- **bench:** Full-corpus regression must include the v2 anchor files
  (generated.log exists; generated.jsonl + generated.repeat.jsonl already
  added) AND the no-regression controls (random, repeat). Decode throughput
  plane is now co-arbiter with ratio — ensure pareto_front.py reports the
  ratio-vs-decode plane prominently (FLAG-A).
- **coordinator/research:** CONTEXT.md §"Linux continuation (v2)" is the
  source of truth for the Linux evidence; this agenda mirrors it with gate
  annotations. Keep the two in sync when new v2 results land.

---

*Status: v2 agenda. SPARSE-REF verdict upgraded: **RATIO-VALIDATED (Linux
v2), Pareto UNPROVEN** — building blocks (patch-phrase copy, RCM/SCM/SRR
parsers, clustered residuals) empirically validated on two files; decode
throughput remains the binding constraint (FLAG-A). New validated
sub-mechanisms ranked (P(d|s), mutation-template, boundary-aligned candidates,
difference-cover gate); fail-gate additions recorded. Build order for the
Windows tree: port v2 validated mechanisms as new modes (t-sparse) → parser
(t-parser) → full-corpus regression incl. decode plane (t-bench) → ledger
claims written only against Windows A/B evidence (t-ledger).*

---

# PART II — Iteration 2 gate criteria (throughput architecture)

*Mission (coordinator, Aug 2026): close the throughput gap. Iteration 1
proved the ratio is competitive (mdl aggregate 0.1307 vs brotli q9 0.1118;
SPARSE-REF mode 11 −9.6% on jsonl) but every config is Pareto-DOMINATED:
decode 3.5-4.5x and encode 24-40x slower than brotli q9. The Linux reference
line proves the path: semantic shape book + per-shape displacement reached
0.1046 ratio @ 957 MB/s decode on generated.json (vs brotli q4 0.1252 @
1411, q9 0.0897 @ 1210). This Part II sets the falsifiable gate criteria for
each Iteration-2 mechanism BEFORE implementation, so the claims are
pre-registered. Ledger must record every decision, including failures.*

**Iteration-2 Pareto target (the gate):** any Iteration-2 config must beat
brotli on the ratio-vs-decode plane (or ratio-vs-encode) per
tools/pareto_front.py on ≥1 corpus file, per-file AND/or aggregate —
EXTENDS_FRONT, not just a ratio beat. Round-trip + fuzz before any claim.
Inner-loop iterate against brotli q1/q4/q6/q9; q11 only for final validation.

## I2-1. Shape-book + per-shape displacement prediction P(d|s)

- **Lineage:** Brotli distance context maps; LZMA match-state distance
  coding; zstd repcodes. Distance prediction with context is NOT new.
- **NEW (narrow — FLAG-D binds):** compile the shape vocabulary
  (kind/len/patch-topology) into a decoder instruction book; each shape
  carries a tiny per-shape displacement state — first occurrence absolute,
  later = signed delta from the shape's last displacement (zigzag +
  class/extra). The claim is *per-shape state via semantic opcode*, not a
  generic context map.
- **Falsifiable ablation (MANDATORY, pre-registered):** P(d|s) vs an
  equivalent-size generic context-map vs global recency, everything else
  fixed. KEEP only if per-shape beats context-map-equivalent end-to-end —
  otherwise it is a renamed context map and FAILS the gate.
  **Measurement contract (bench lane):** `bench/flag-d-ablation-contract`
  — claim row `anvil-shape-rans` (per-shape persistent state) vs control
  `anvil-shape-ctxmap-rans` (same shape vocabulary, generic
  context-map-equivalent, NO per-shape state), everything else fixed; pass
  = claim beats control on record-structured files (generated.json/jsonl/
  log/sqlite) with the Pareto verdict vs the all-DOMINATED iteration-1
  baseline; claim == control on structured files ⇒ FLAG-D fails.
- **Evidence to match:** Linux 0.1046 @ 957 MB/s decode on generated.json;
  per-shape log-proxy ~9.4 bits vs ~13 unconditional. Windows A/B is the
  arbiter (FLAG-B).

## I2-2. Precision/work-adaptive entropy

- **Lineage:** rANS/ANS precision (Duda; ryg_rans; FSE); Brotli/zstd block
  type selection. Per-stream codec choice is partly explored.
- **Prior-art honesty (coordinator, Linux v2 — RFC 7932):** **context
  clustering is EXPLICITLY NOT novel.** Brotli itself maps decoded literal
  context to several literal prefix trees via a compact context map driven
  by previous decoded bytes (RFC 7932). Any I2-2 claim that leans on
  "decoder-visible context modeling" must NOT be framed as new. The narrow,
  defensible contribution is the **~1.65 ms sparse-support quantizer**
  (K≈8–12 learned probability classes, one physical rANS stream, previous
  byte selects class at decode, context identity costs zero bits/literal)
  making decoder-visible context modeling *economical inside ANVIL's
  rANS/semantic architecture* — **treat as enabling infrastructure unless
  an ablation shows a genuinely new interaction.**
- **Measured Linux verdicts (record — do not re-derive):**
  - Context-switched rANS = a **RATIO mechanism, not the missing throughput
    primitive**: SQLite depth-2/K12 ≈ 404.7 KB, depth-3 ≈ 379.3 KB vs
    brotli q4 ~422.1 KB (huge ratio headroom) but decoder falls to
    **~0.6–0.74 GB/s** — not a decode win by itself.
  - Context-switched table **Huffman/direct variant: REJECTED** (~143.8 KB
    at K=12, same size as clustered rANS, but the context-dependent prefix
    machinery is SLOWER once model/table setup is counted).
  - Keep the K≈8–12 quantizer as reusable infrastructure.
- **NEW (as a claim — narrowed):** joint cost J = L_stream + λC_decode +
  μC_model-build + νW_cache selecting per stream among 256/512-state rANS
  (cache-resident tables) × stream suite (Huffman / default-with-sparse-
  exceptions / pair-rANS / raw). "Smallest wins, but the objective includes
  decode and model-build cost." **The I2-2 novelty, if any, is the
  J-selection interaction itself** (predicting the measured winner on ≥80%
  of streams) — not context clustering; and it must clear the I2 Pareto
  target (EXTENDS_FRONT), not just add ratio.
- **Falsifiable ablation:** fixed-rANS vs stream-suite per stream; report
  Δratio, Δdecode, Δmodel-build; verify the J-cost predicts the winner on
  ≥80% of streams. Decode win must be real (FLAG-A), not just ratio.
  **Measurement contract (bench, signed off 2026-08-13):**
  `bench/jcost-validation-contract` v2 — claim = mode 12 with
  precision/work-adaptive entropy per stream (256/512-state rANS +
  Huffman/exception/pair/raw suite, J-selected); control = same mode-12
  pipeline with fixed 4096-state rANS; pass = suite beats fixed-rANS
  decode on record-structured files (the cross-cutting decoder win), no
  ratio regression, J predicts winner on ≥80% of the 22 streams,
  random/repeat controls no-regression; target context Linux 0.1046 @
  957 MB/s decode on generated.json (host differences per host-spec).
  **Pre-registered constants (binding, threshold-fit invalidates the
  gate):** λ = μ = 0.01 bytes/μs, ν = 0 (256 KB revisit clause); verdict
  uses ONLY these; (λ,μ) sensitivity table {0, 0.01, 0.1, 1} reported for
  robustness, never decisive. **Per-class co-arbiter (amendment B):**
  aggregate ≥80% AND no stream class <60% absolute; a failing class is a
  real cost-model finding recorded even if the aggregate passes.
- **Why Pareto:** stream anatomy shows 22 streams dominated by pair/Huffman
  on the big residual streams; precision-adaptive coding makes rich
  multi-model coding cheap to initialize/execute. But per the Linux
  verdicts, the suite must earn its throughput claim — clustered rANS alone
  is a ratio mechanism; the decode win must come from the J-selection +
  cache-resident-tables interaction.

## I2-3. Cheap adopts (C5): surprise-budget sweep + boundary candidates + negative gate

- **Lineage:** deflate stored-block decisions; zstd/lz4 incompressible
  detection; standard search formulations.
- **NEW (search formulations, low novelty — gate passes on engineering
  value + measured effect, not mechanism novelty):** (a) parser
  surprise-budget sweep — Linux: budget 5/3 beats default 6; it is an
  *entropy-control variable*, expose and sweep, don't hand-tune (arch's
  compile-time constants are the known caveat); (b) boundary-aligned
  candidate generation (66-92% of LZ sources within ±8 B of prior token
  starts); (c) mod-64 cyclic difference-cover negative gate (random 3,341
  MB/s Linux).
- **Falsifiable ablation:** each adopt ON/OFF; encode Δ (must be real),
  ratio Δ (must be ~0). Random/repeat controls no-regression.

## I2-4. R2 correction-topology coding (per-slot modal residual + exception mask)

- **Lineage:** flat mismatch lists (Zdelta); flat diff arrays (bsdiff);
  edit ops (DNA); PPM templates.
- **NEW:** per-(mask,slot) modal residual as decoder-visible default +
  exception mask. Slot-default accuracy measured 86.5% on logs (Linux v2);
  Windows ablation is the open item.
- **Falsifiable ablation:** flat-A baseline vs slot-default vs order-1 mask
  model on the same sparse tokens; Δratio and Δdecode. The flat-A mask
  overhead (repeat control +2.2%) is the headroom it must recover.
  *(Windows I2-4 verdict, Experiment I: NOT ADOPTED — modal accuracy 17-23%
  on this corpus, masks ~90% unique; blocked on structural-distance
  propagation R4. Contract stands for revisit.)*

## I2-5. TCOPY — implicit-parameter transformed copy (PRE-REGISTERED, post-t2-bench candidate)

- **Status:** PRE-REGISTERED at the novelty gate (coordinator injection,
  Linux frontier evidence, docs/CONTEXT.md). Candidate for the binary lane
  AFTER t2-bench — do not schedule before the I2 regression completes.
- **Prior-art lineage:** SPARSE-REF/bsdiff copy-with-corrections; delta
  encoding; binary-patch machinery (relocation-aware patching exists as
  *external* tooling, e.g. Courgette). What is NOT established: a
  *self-referential* (single-file) lossless compressor whose transformed
  copy derives the transform parameter from the reference itself, with the
  transform implicit at zero bits for executable-relative fields.
- **What is NEW (mechanism-level claim):** **implicit-parameter transformed
  copy** — a generalized match where the transform parameter is derived
  from the reference (the copy distance d), not transmitted. Reference
  family TCOPY(d,L,Δ,M,R): copy a prior phrase, add a common 32-bit delta Δ
  at sparse field offsets M, then apply sparse residual bytes R. For
  PC/RIP-relative fields Δ=−d is IMPLICIT (zero bits for the transform
  parameter). Ordinary LZ = special case M=R=∅.
- **Evidence (Linux, directional — do not re-derive):** ELF mismatch
  anatomy: ~81% of sampled .text approximate-repeat candidates contain ≥2
  32-bit fields differing by exactly −distance (the algebra of PC/RIP-
  relative relocation when the same instruction template appears at a
  different file position), explaining ~46% of mismatch bytes; a single
  repeated 32-bit additive delta explains ~59% of mismatch bytes (.text),
  ~49% (.eh_frame), ~61% (.rodata).
- **Why Pareto:** position-dependent code is the dominant reason exact-LZ
  fails on ELF — TCOPY turns the largest mismatch class into copy +
  sparse 32-bit adds + sparse stores, decoder cost ≈ the existing
  sparse-corrected path.
- **Falsifiable ablation (prototype plan, isolated — not premature
  integration):** can phrase-level transformed self-reference explain ELF
  .text mismatches materially better than exact LZ at decoder cost ≈ copy +
  sparse 32-bit adds + sparse stores? First prototype EXCLUDES overlapping
  refs (dist<len) so semantics stay unambiguous; transform fields and
  residual bytes get SEPARATE statistical domains so the gain is
  attributable to the transformed reference itself, not a better entropy
  coder. If it gains density, overlap/periodic transformed references are
  the later extension.
- **Gate verdict:** pre-registered PASS-as-candidate pending isolated
  ablation; the novelty claim (implicit-parameter transformed copy) is
  mechanism-level and distinct from context clustering (I2-2) and
  structural-distance propagation (R4) — it is a new *transform-in-reference*
  family. Tied to the rejected lane-transpose control (global
  lane/field transposition destroyed contiguous phrase structure): the
  transform must live INSIDE the reference, not globally before LZ.
- **Prior-art research (research lane, blackboard arch/priorart-tcopy):**
  BCJ/E8-E9 = GLOBAL pre-LZ transform (orthogonal; distinction confirmed);
  Courgette = relocation-aware but TWO-FILE + disassembler-based + global
  (self-referential implicit-Δ NOT found); no prior art for long-range/
  overlapping transformed refs. **PATENT CHECK = REQUIRED GATE STEP before
  any novelty claim** (no patent-database access this session; Microsoft/
  Qualcomm relocation families flagged). TCOPY = NARROW-to-NEW-INTERACTION,
  conditional on patent check + isolated Windows ablation; queued
  post-t2-bench.
- **PATENT GATE RESULT (research, t3-patent — blackboard
  research/patent-tcopy): CONDITIONAL PASS (narrowed).** The relocation-
  algebra delta (Δ = position difference for relative fields) is
  DISCLOSED: US7676506B2 (Reinsch/Qualcomm, priority 2003) explicitly
  recomputes branch displacements as
  (targetV2−addrV2) = (targetV1−addrV1) + (targetStartV2−targetStartV1)
  − (startV2−startV1) — the same math as TCOPY's Δ=−d — but in a TWO-FILE
  delta with transmitted map-file/symbol hints and global pre-processing.
  Microsoft's "Minimum delta generator" family (US7058941/7681190/7685590,
  priority 2000) is CFG-based two-file binary delta; IBM (US6374250B2,
  1997) is exact-match block-move. **The SELF-REFERENTIAL single-file
  implicit-parameter formulation (Δ derived from the match distance
  itself, zero bits, no external hints, inside a match) is NOT FOUND in
  any surveyed family.** Defensible novelty claim is now precisely:
  *self-referential implicit-parameter transformed copy*. The isolated
  Windows ablation must demonstrate the gain is from the implicit-parameter
  self-reference, not from transformed copy per se — if the gain is mostly
  "transformed copy helps binaries", the novelty is NARROW-to-NONE (flag
  for the binary-lane gate). Freedom-to-operate attorney review recommended
  before any commercial claim (this is a classification gate, not legal
  advice).
- **Isolated ablation contract (research, t3-tcopy prep — blackboard
  research/tcopy-ablation-contract):** (a) separate statistical domains
  for transform fields vs residuals; (b) exclude overlapping refs first;
  (c) round-trip strict (verify + fuzz); (d) **narrowed-claim test**:
  A/B implicit Δ=−d (zero bits) vs transmitted Δ (same representation,
  delta coded explicitly) vs exact-LZ — if implicit ≈ transmitted on
  ELF/PE .text, novelty is NARROW-to-NONE (transformed copy is prior art
  per US7676506B2) and the claim FAILS the gate; if implicit materially
  beats transmitted, the self-referential implicit-parameter claim
  stands. Windows A/B is the arbiter (FLAG-B).
- **External corroboration (coordinator, docs/priorart-tcopy-external.md,
  committed):** independent web pass CONFIRMS the patent gate — no prior
  art teaches single-file self-referential implicit-Δ transformed copy.
  Adds: Microsoft lineage (US7861224B2 "delta compression using multiple
  pointers"; US20050022175A1/US7600225B2/EP1501196A1 intra-package delta;
  symbol-aware executable delta US 6,466,999 — two-file, full claims not
  retrieved); Apple dyld chained-pointer relocation compaction
  (close-but-different: pointer/rebase METADATA, not LZ-copied bytes);
  BCJ/E8-E9 public-domain confirmation (LZMA SDK 4.62, Dec 2008, global
  pre-LZ). Coverage gaps flagged for the record: US 6,466,999 full claims;
  Qualcomm/IBM targeted queries; Apple chained-fixup patent number;
  Espacenet/lens.org not queried; FTO attorney review still recommended.
  **Residual gaps are OPEN items for the binary-lane gate, not blockers
  on the narrowed claim's current status (CONDITIONAL PASS).**
- **Three-pass convergence (coordinator, docs/priorart-tcopy-external.md
  passes 2+3, committed):** THREE independent passes now agree — the
  single-file self-referential implicit-Δ transformed copy remains
  UNCLAIMED in the searched record. Closest art on all three passes:
  (a) global once-per-file relocation/branch adjustment (BCJ/E8-E9
  public-domain, PCOMP, IBM US6564314), or (b) two-file copy-with-edits
  (Red Bend US6546552, Microsoft US7509636/WO2005071542, MS-RDC,
  Courgette, Zdelta). Additional close-but-different: Apple US10229282
  (dyld pointer-stub), Qualcomm US9300320 (cache-line dict), FaStore
  US9223794 (symbol-level edits), US12373439 + US20240211132A1
  (approximate-match MASKING only — no additive transform), and
  self-referential LZ77-with-edits THEORY (Gawrychowski 2011/2021;
  Kreft & Navarro 2013 — Hamming/Levenshtein bounds, no relocation
  arithmetic). **Cross-reference for SPARSE-REF (C1):** the masking-only
  filings (US12373439, US20240211132A1) are closest-art touchpoints for
  ANVIL's sparse-mask claim too — the mask-without-transform formulation
  exists; ANVIL's additive/implicit-transform + self-referential
  combination is what separates C1 from them. Residual gaps recorded:
  18-month publication blackout, paywalled corpora, US 6,466,999 full
  claims; FTO attorney review still recommended. **The CONDITIONAL-PASS
  verdict is now strongly supported by three-pass convergence.**
- **Pass 4 (coordinator, most rigorous scan — committed):** verdict
  UNCHANGED (NOT-FOUND) with material updates folded in:
  - **REQUIRED ITEM — Intel US 7,111,148 B1 / US 7,010,665 B1
    ("compressing/decompressing relative addresses", prio 27-Jun-2002):
    full-text/claims review is REQUIRED before the executable-specific
    novelty boundary is treated as closed** — titles too close to wave
    away; claims not retrievable in the pass. This is a binding
    binary-lane-gate closeout item.
  - **VCDIFF / RFC 3284 (June 2002) — touches SPARSE-REF C1:** single-file
    self-reference + exact COPY + ADD/RUN corrections (incl. overlapping
    target copies) is STANDARDIZED prior art. C1's separator must remain
    the sparse-correction-mask-as-first-class-entropy-stream +
    implicit-transform combination — NOT self-reference per se.
  - **Zucchini (Chromium) = strongest executable near-hit:** copy +
    correction + rel32 handling exists, but two-file patching.
  - **BCJ boundary QUALIFIED:** defensible distinction is *pre-LZ filter
    layer* (per xz filter-chain docs), not "file-wide" per se.
  - **Corrections:** US 12,373,439 REMOVED as compression art (OptumSoft
    table matching); US 6,564,314 is STMicroelectronics, not IBM.
  - GenCompress/RLZAP noted as close-but-different approximate-match art.
  - Closest combined art: VCDIFF/Microsoft (self-ref COPY), GenCompress
    (ref-plus-edits), BCJ/Philips US5787302 (relocation normalization),
    Zucchini (executable-aware copy-plus-correction).

## Iteration-2 sequencing (as scheduled)

t2-shape → t2-entropy, t2-topology → t2-bench (full regression, both planes)
→ t2-ledger (claims written only against Windows A/B; failures recorded).
t2-gates (cheap adopts) runs in parallel. All claims pre-registered above;
the gate stays the arbiter. TCOPY (I2-5) is queued post-t2-bench as the
binary-lane candidate.

---

# PART III — Iteration-3 gate criteria (Pareto frontier push)

*Mission (coordinator, I3): PUSH THE PARETO FRONTIER — EXTENDS_FRONT on at
least one plane on at least one file class. Two iterations delivered
validated enabling infrastructure but zero Pareto wins; decode ~4x behind
brotli q9; the 22-stream/fused architecture + SRR synchronized probe are
the binding levers. Pre-registered below; round-trip + fuzz before any
claim; every failure recorded with reason.*

## I3-1. Fused shape-stream decode (t3-fuse) — PARTIAL PASS (recorded Experiment M)
Fusion gain real and attributable (~1.2x record files, 1.76x repeat) at
preserved ratio; the 2x pre-registered target NOT MET — per-field entropy
pulls are the decode floor in both paths; the 2x+ regime needs the
compiled hot-op instruction book (Linux modes 26-28), recorded as the
remaining lever. Contract: fusion-vs-separated on the same wire A/B.

## I3-2. R4 structural-distance propagation (t3-r4) — NOT ADOPTED (recorded Experiment N)
Reinforcement-of-taken channels cannot discover the record period (69% of
greedy sparse matches are far-distance). The missing mechanism is the
SRR/SSCM *synchronized structural-distance probe* — recorded as the
genuine R4 remainder. R2 topology stays BLOCKED on alignment.

## I3-3. TCOPY implicit-Δ (t3-tcopy, mode 14) — mechanism VALIDATED, claim UNDECIDABLE (recorded Experiment O)
Transform fields fire on real PE (519 in anvil.exe block 0); small gains
at same greedy parse; headline density NOT reproduced (greedy parse
limiting factor). **Narrowed-claim test INCOMPLETE**: explicit-Δ control
unbuilt — required follow-ups before any novelty claim: (1) explicit-Δ
control ablation (implicit-vs-transmitted separation), (2) Intel
US 7,111,148 / US 7,010,665 full-text review (binding closeout item).
FTO attorney review recommended. See four-pass record
(research/patent-tcopy) + ablation contract (research/tcopy-ablation-
contract).

## I3-4. PNRA — transformation-invariant temporal anchoring (PRE-REGISTERED, Linux advance)

- **Status:** PRE-REGISTERED at the novelty gate (coordinator sync, Linux
  .text evidence — docs/CONTEXT.md + prototypes/pnra/). Mechanism-level
  novelty candidate: a NEW SEARCH FORMULATION.
- **Prior-art lineage:** approximate-match candidate generation +
  expensive verification (LZ with mismatches, Zdelta, bsdiff add-arrays,
  TCOPY's own field detection); indexing over raw bytes. The prior art
  discovers the transform AFTER candidate generation.
- **What is NEW:** **transformation-invariant indexing** — derive I(x,p)
  such that I(T(x,θ), p') = I(x,p) for the relevant transform, and build
  the temporal dictionary over I, not raw bytes. For TCOPY's relocation
  fields, v + absolute field position is invariant under the Δ=−d
  transform (two-anchor invariant (K1, K2, Δf) with K_i = v_i +
  position(v_i)). Changes discovery from "candidate → expensive
  approximate verification → discover transformation" into
  "transformation invariant → exact hash lookup → cheap verification".
  **Ordinary LZ = identity-transform special case.**
- **Evidence (Linux, directional — do not re-derive):** single-anchor PNRA
  1,766,167 → 1,741,092 B → combined 1,730,689 B vs brotli q4 1,781,130 B
  (~50 KB below q4); two-anchor pair-index 1,755,244 B @ ~29.5 MB/s
  parser / ~279 MB/s decoder (2,862/4,583 transformed phrases contain ≥2
  E8/E9 fields); event-driven PNRA 1,794,886 B @ ~39.3 MB/s encode / ~270
  MB/s decode (13,752 matching pair signatures, candidate generation ~474
  MB/s — transformed search no longer the dominant encoder bottleneck).
- **Why Pareto:** transformed-search is now the expensive component's
  removal — it converts the binary lane from approximate-match cost to
  exact-hash cost; combined with the q6 re-target (~300 MB/s decode at
  q6-class density is the favorable comparison, not q4-class encode).
- **Falsifiable ablation (pre-registered):** (a) PNRA-on vs PNRA-off for
  the SAME transform family (TCOPY) on ELF/PE .text — the gain must be
  from the invariant indexing, not the transform; (b) invariant-based
  indexing vs raw-byte indexing at equal candidate counts (isolates the
  formulation); (c) LZ = identity-transform special case (PNRA with the
  identity invariant must reproduce the exact-match baseline); (d)
  Windows A/B is the arbiter (FLAG-B); round-trip + fuzz; no-regression
  controls.
- **Gate verdict:** pre-registered PASS-as-candidate pending isolated
  Windows ablation; the C1/TCOPY separators now include
  "transformation-invariant anchoring" as an enabling primitive.

## Iteration-3 sequencing

t3-fuse → t3-r4 (→ R2 retest) → t3-tcopy (patent gate: four-pass record +
Intel review) → t3-bench (both planes, pareto_front.py; inner loop vs
q1/q4/q6/q9, q11 final-only) → t3-ledger (claims written only against
Windows A/B; failures recorded). PNRA (I3-4) pre-registered as the binary-
lane follow-on; hot-op instruction book (I3-1 remainder) queued for the
decode leg. All claims pre-registered above; the gate stays the arbiter.

---

# PART IV — Iteration-4 gate criteria (close the throughput leg; open Orbit-Program)

*Mission (coordinator, I4): three iterations produced zero Pareto wins (all
rows DOMINATED); the binding gap is decode/encode throughput. Strategic
direction: docs/ORBIT_PROGRAM_COMPRESSION.md — conditional-program
discovery REF(d,L,P,θ,R) with y = P(x;θ,context)⊕R, objective min
description, Orbit-LZ equivalence-class matching via canonical/hashable
invariants (PNRA = first measured instance). Success = EXTENDS_FRONT on at
least one plane on at least one file class. Every mechanism below is
pre-registered with the four-part gate (lineage / what-is-new /
why-Pareto / falsifiable ablation); round-trip + fuzz before any claim;
failures recorded with reason.*

## I4-1. Compiled hot-op instruction book (decode leg)

- **Lineage:** macro-ops + compiled hot-op instructions (Linux modes
  23/26-28: encoder synthesizes a decoder instruction book of concrete
  semantics — kind/len/dist/patch-selector; hot stream = small opcode
  index, rare tokens fall back to macro-ops; 75-77% of tokens hot on
  generated.log); Brotli fused insert-and-copy codes (RFC 7932 §5, 704-
  alphabet static book); zstd sequences (fused lit-len/offset/match-len);
  Sequitur/Re-Pair (grammar instruction sets). Static/fused vocabularies
  EXIST; a TRANSMITTED adaptive compiled book with hot/rare split is the
  interaction (research/priorart-entropy §3).
- **NEW:** port the Linux pattern to the Windows codec — encoder-synthesized
  instruction book (transmitted once), hot-opcode-index stream, rare-token
  macro-op fallback. The fused decode path (t3-fuse, Experiment M) topped
  out ~1.2x because per-field entropy pulls are the floor; the book removes
  per-field pulls by turning hot tokens into a single small-index lookup.
- **Why Pareto:** the measured remainder after fusion — the decode leg
  (binding gap per FLAG-A; decode ~4x behind brotli q9).
- **Falsifiable target (pre-registered):** **≥2x decode on record files at
  preserved ratio** vs the fused mode-12 baseline (same wire or clean A/B),
  on generated.json/jsonl/log; no-regression on repeat/random; round-trip
  + fuzz; FLAG-A both-plane Pareto verdict via bench's tools.

## I4-2. SRR synchronized structural probe (alignment)

- **Lineage:** SRR/SSCM "synchronized residual reference" (Linux v2 —
  structural channels that PROBE distances rather than reinforce-taken);
  R4's channel bank (Experiment N — reinforcement-of-taken FAILED because
  69% of greedy matches are far-distance; the probe design is the unbuilt
  remainder). Recency/repcode distance coding (LZMA/Brotli/zstd) is prior
  art for reuse, not for structural discovery.
- **NEW:** the synchronized probe — parser actively tests candidate
  structural distances (record periods) against history, discovering
  alignment instead of waiting for matches that never come. This is the
  missing prerequisite for R2 topology alignment AND TCOPY's headline
  binary density.
- **Why Pareto:** unblocks R2 (per-slot modal residuals, 86.5% modal
  accuracy on Linux logs) and TCOPY's density leg — both blocked on
  alignment since I2/I3.
- **Falsifiable target (pre-registered):** (a) the R2 retest (mode 13)
  after alignment: slot-default coding must beat flat-A on record files
  (recover the +2.2% repeat-control headroom — the original I2-4 target);
  (b) mask-recurrence metric: top-32 masks coverage must rise materially
  above the flat 12.3% on log; (c) TCOPY density retest on .text; (d)
  no-regression controls; round-trip + fuzz.

## I4-3. Precision-adaptive entropy economics (single context-switched literal coder)

- **Lineage:** context clustering is EXPLICITLY NOT novel (Brotli RFC 7932
  §7 context maps; Experiment J record). The ~1.65 ms sparse-support
  quantizer (K≈8-12) is ENABLING INFRASTRUCTURE (not standalone novelty).
  J-selection is validated 100% faithful (Experiment L corrected; bench
  median-3). Multi-stream literal fan-out was REJECTED (Linux verdicts).
- **NEW (as interaction):** integrate the stream suite with ONE physical
  context-switched literal coder (previous byte selects the class table) —
  the quantizer makes it economical; the J-objective (pre-registered
  constants λ=μ=0.01, ν=0) selects per stream. The interaction is
  "cheap decoder-visible context modeling inside the rANS/semantic
  architecture," not context clustering per se.
- **Why Pareto:** literal entropy is a large stream; context-switched
  coding at ~equal decode cost (single physical stream) is ratio for free;
  benefits every file class.
- **Falsifiable target (pre-registered):** (a) ratio improvement vs
  fixed-rANS on the same wire at equal-or-better decode (FLAG-A); (b)
  J-faithfulness maintained ≥80% (per-class co-arbiter); (c) no-regression
  on repeat/random; round-trip + fuzz.

## I4-4. Orbit-LZ: multi-invariant anchoring extension of PNRA

- **Lineage:** parameterized matching (Baker predecessor encoding —
  canonical invariant: strings parameterize-match iff invariant
  representations identical); set parameterized matching (Lewenstein &
  Porat 2026, randomized linear time via multilayer hashing); pattern
  matching under polynomial/linear transformation (Butman et al. 2011);
  set reconciliation/sketching (2014); PNRA (ANVIL I3-4 — translation
  family, invariant I(v,p)=v+p, exact hash lookup, ~474 MB/s event-driven
  discovery). **These are LINEAGE, not claims** — the invariant trick per
  class is established; what is NOT established is a practical LZ-family
  compressor unifying multiple equivalence relations under one MDL parser.
- **NEW:** extend transformation-invariant anchoring beyond translation —
  candidate invariants: parameterized-symbol invariant via predecessor
  encoding; finite-difference invariant for degree-1 polynomial fields;
  stride/bitplane invariants. Each new invariant = a new index
  (H_exact, H_additive, H_difference, H_parameterized, H_polynomial,
  H_stride, H_bitplane) feeding ONE MDL parser over actual encoded bits.
  The claim is the *multi-index unification under an MDL objective* — not
  any single invariant (each is prior art).
- **Why Pareto:** the math target — near-linear-time minimum-description
  transformed backreference over a useful transformation algebra;
  invariants/sketches prune (reference × transform) pairs. Binary lane
  density + general structured data.
- **Falsifiable target (pre-registered):** (a) per-invariant ablation —
  each new index (vs PNRA-only) must improve end-to-end encoded bits on
  its target class at O(1) parser cost, attributable to the invariant not
  the entropy backend (separate statistical domains); (b) the unified MDL
  parser must beat single-index baselines on the class mix; (c)
  LZ = identity-transform special case must reproduce the exact-match
  baseline; (d) Windows A/B arbiter; round-trip + fuzz; no-regression.

## Prior-art note — program-synthesis compressors (lineage, not claims)

Per docs/ORBIT_PROGRAM_COMPRESSION.md §"Disciplines retained": Brevis
(2026, lossless compression as program synthesis over a typed DSL, learned
prior + bounded A*, decoder executes only the compact program — reports
30.87% smaller on 2.13 TB checkpoints @ 6.61 GB/s decode), LZ77
k-sensitivity/pre-editing (arXiv 2602.19649, ~3x total improvement in
favorable cases — philosophically TCOPY), AIT Compression Challenge
entrants (seed recovery → generator descriptor; mutual-information byte-
distance contexts), KoLMogorov test (shortest-program framing), Pcodec
(latent-variable decomposition for columnar), OpenZL (composable
reversible graph primitives), Diffuse-to-Compress (kb/s-scale — NOT the
decoder ANVIL wants). **All are LINEAGE for the program-discovery framing —
none are ANVIL novelty claims.** ANVIL's defensible lane: cheap
deterministic programs (tiny VM + copy + sparse stores + integer adds),
encoder-side intelligence only (decoder stays LZ4-class), and the
invariant-based search formulation. The 2026 citations are
operator-reported — verify each before citing in the ledger.

## Iteration-4 sequencing

t4-gate (this pre-registration) → t4-hotop (decode leg; unblocks t4-srr +
t4-entropy) → t4-srr (SRR probe; unblocks t4-orbit + R2/TCOPY retests) →
t4-orbit (multi-invariant) → t4-bench (both planes, inner loop q1/q4/q6/q9,
q11 final-only) → t4-ledger (claims written only against Windows A/B;
failures recorded). All claims pre-registered above; the gate stays the
arbiter.

# PART V - I10 Grotli representation series (G0-G5B; ledger A22)

> Append-only status tracker for the I10 Grotli series. G4/G5A/G5B entries are
> **results records**, not new agenda claims; nothing here reopens a closed
> result or authorizes a production transform. Authoritative results docs are
> named per entry. No new architecture claim is introduced.

- **G0-G3 (representation) - closed.** Corpus freezes / preregs / results in
  `docs/I10-GROTLI-G0-*` .. `G3-*`; frozen G3 identity
  `1a3d18fed76adb6fb33264e1994f9c357306b3fa` (blob `eedc7b7e…`) is the shared
  reference for G5A/G5B-ORDINAL.
- **G4 - NO-GO-G4.** Standing; no planner promoted, no typed-leaf rescue.
- **G5A ordering attribution - CLOSED (run `35985412906`).**
  **ORDER-MATERIAL / COLUMN-DOMINANT** on frozen D1-D4
  (-6.1189% aggregate, 4/4 files, null passed by 674,287 B, `column_share =
  0.8019`). Results: `docs/I10-GROTLI-G5A-RESULTS.md`. No production transform.
- **G5B-ORDINAL r3 - CLOSED (run `36011333908`).** **ORDINAL-ADVERSE** on
  frozen D1-D4 (B0 `2,054,910` / B1 `1,380,245` / B2 `1,403,029`;
  `b2_vs_b1 = -22,784`, +1.6507%; B2 smaller on 1/4 files; D1 DEGENERATE).
  Results: `docs/I10-GROTLI-G5B-ORDINAL-RESULTS.md`; prereg
  `docs/I10-GROTLI-G5B-ORDINAL-PREREG.md` (r3). **Raw ordinal cross-shape
  coarsening is falsified** as a favorable mechanism on the frozen discovery
  set; this closes only the **ordinal proxy**. V1 known-stress diagnostic
  (not held out): B2 smaller by 2,137 B (`V1-ORDINAL-FAVORABLE`), descriptive
  only. Predecessor run `36010649558` is infrastructure-invalid/premeasurement;
  commit `a443f37c` is a transport-only workflow correction.
- **Semantic G5B lane (flat shapes vs preregistered structural/semantic
  hierarchy; later semantic-path fusion) - OPEN.** G5B-ORDINAL did not answer
  or close it; it supplies architecture context only. Any successor must be its
  own frozen prereg and may not retune G5A/G5B-ORDINAL arms, thresholds,
  corpora, or ruling semantics. `semantic_hierarchy_lane_open: true`.

Boundary rules for this series: raw Brotli remains the permanent fallback; no
held-out corpus is opened by G5A/G5B-ORDINAL; no production transform ID is
allocated; do not add random draws, retune thresholds, or reframe the adverse
D1-D4 ruling using the favorable V1 diagnostic.
