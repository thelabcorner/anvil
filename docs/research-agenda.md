# ANVIL — Novelty-Gated Research Agenda (v2)

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
- **NEW (as a claim):** joint cost J = L_stream + λC_decode + μC_model-build
  + νW_cache selecting per stream among 256/512-state rANS (cache-resident
  tables) × stream suite (Huffman / default-with-sparse-exceptions /
  pair-rANS / raw). "Smallest wins, but the objective includes decode and
  model-build cost."
- **Falsifiable ablation:** fixed-rANS vs stream-suite per stream; report
  Δratio, Δdecode, Δmodel-build; verify the J-cost predicts the winner on
  ≥80% of streams. Decode win must be real (FLAG-A), not just ratio.
  **Measurement contract (research/gate lane):**
  `research/entropy-ablation-contract` — claim = mode 12 with
  precision/work-adaptive entropy per stream (256/512-state rANS +
  Huffman/exception/pair/raw suite, J-selected); control = same mode-12
  pipeline with fixed 4096-state rANS; pass = suite beats fixed-rANS
  decode on record-structured files (the cross-cutting decoder win), no
  ratio regression, J predicts winner on ≥80% of the 22 streams,
  random/repeat controls no-regression; target context Linux 0.1046 @
  957 MB/s decode on generated.json (host differences per host-spec).
- **Why Pareto:** stream anatomy shows 22 streams dominated by pair/Huffman
  on the big residual streams; precision-adaptive coding makes rich
  multi-model coding cheap to initialize/execute.

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

## Iteration-2 sequencing (as scheduled)

t2-shape → t2-entropy, t2-topology → t2-bench (full regression, both planes)
→ t2-ledger (claims written only against Windows A/B; failures recorded).
t2-gates (cheap adopts) runs in parallel. All claims pre-registered above;
the gate stays the arbiter.
