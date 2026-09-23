# 07 — Ranked Research Portfolio

The portfolio is ranked by expected project value, not novelty glamour.

Legend:

- **EV**: expected value if successful;
- **cost**: implementation/measurement effort;
- **risk**: probability the premise fails;
- **evidence**: current support.

## Tier 0 — make current evidence trustworthy

### P0.1 Backend-2 format + fuzz completion

**EV:** critical  
**cost:** low  
**risk:** low

Document BWT backend/postcoders, add backend-specific adversarial fuzz, create golden files.

Why first: no performance result matters if the format is underspecified or malformed input is unsafe.

### P0.2 Normalize the benchmark build

**EV:** critical  
**cost:** low  
**risk:** low

Pin one Windows compiler/config for ANVIL + libsais + Brotli helper and rerun timing rows that will be compared.

### P0.3 Decoder-only binary-size target

**EV:** medium-high  
**cost:** low-medium  
**risk:** low

Build `anvil_decode`, ZIP it, and begin reporting decoder-size tax alongside corpus bytes.

## Portfolio re-rank note (2026-09-07, from measured 4-way disagreement map, see doc 10)

The measured Silesia 4-way table + lead's independent recompute changed several EV rankings:

- **P1.1 (whole-file auto routing)** moves to the *single highest-EV* experiment in the project. It is now the only thing that converts the verified 46,446,836-B oracle (achievable target, +rev-2 framing) into an actual result. Cost is trivial (one command), risk low (the backend selection already exists). Its verdict is gated by P0.2/E1 normalization and by the decode-cost axis (11 §4.2).
- **P4.1 (DEFLATE reconstruction) is now the #1 next investment** — on two independent measured
  arguments (see also 06 §G3): (a) decode-axis — its target files (mozilla/sao/ooffice/samba) are
  Brotli/xz-routed with fast decode, so its ~0.66-MB gain costs **no** decode regression, unlike
  BWT/CM; (b) routing-axis — E4 (`findings/e4-block-routing-negative`) proves you **cannot** recover
  those files by carving out text regions (zero BWT-winning blocks there), so reconstructing the
  embedded compressed streams is the only route. The disagreement map quantified the missing family:
  664,307 B of the gap to the landscape bar (45,782,529) sits exactly in those files. Fund P4.1 next.
- **Decode-cost (M3 in doc 10)** is now an explicit blocker, not a footnote. BWT decode is 6.0× slower
  than xz / 11.2× slower than Brotli on the 7 routed files (now pinned clang-cl, E1 done). The win is
  FRONT-GAP on decode; region-scoped routing is a dead end (decode-amortization finding) and block-local
  routing is closed (E4). The only decode-axis fix is a faster BWT inverse or a fast-decode backend.
- **P3.1 (CM)** stays very-high-EV but is now ranked **below P4.1**: CM would likely decode *slower*
  than BWT, worsening the ~7× decode conflict, and E4/E6 have closed the two cheaper "more bytes" routes
  — P4.1 is the cleaner next bet. Order: routing (DONE) → DEFLATE recon → CM.
- **P1.2 / E5 (block-local routing) CLOSED-by-NEGATIVE (E4):** block routing is worse than whole-file
  everywhere; not runnable. **P1.3 / P1.4 (QLFC/LZP) NO-GO (E6):** measured ratio losses (+3.23% /
  +0.37%). Both redirect budget to P4.1 / P3.1.

## Tier 1 — highest near-term ratio EV

### P1.1 Measure whole-file backend auto routing

**EV:** very high → **now the single highest-EV experiment in the project**  
**cost:** trivial  
**risk:** low

The oracle from already-measured BWT/Brotli files is 46,446,836 B (achievable ANVIL target, +rev-2 framing; verified independently as `oracle_min(Brotli,BWT)`). Measure the actual `--ratio-backend=auto` output with transforms disabled first. **Verdict rule (11 §4.2):** it only counts as a frontier result if it beats the xz bar 48,456,100 B *and* stays within decode/peakmem margin; otherwise it is FRONT-GAP (bytes-only win).

This is the cheapest potentially multi-megabyte experiment in the project, and the one that converts a verified hypothesis into a number.

### P1.2 Block-local backend oracle and routing regret

**EV:** very high  
**cost:** low-medium  
**risk:** medium

On mixed files, independently compress fixed blocks with Brotli and BWT and compute oracle savings at 256 KiB / 1 / 4 / 16 MiB.

Do **not** implement a heuristic router first. Measure oracle headroom and optimal scale.

Especially target:

- `mozilla`;
- `samba`;
- `ooffice`;
- `sao`.

If block-level oracle barely beats file routing, stop. If it buys hundreds of KiB+, then build a cheap router.

### P1.3 BWT postcoder: QLFC/local-frequency control

**EV:** high  
**cost:** medium  
**risk:** medium

Plain MTF/RLE arithmetic already wins seven files. The obvious prior-art gap is QLFC/local-frequency ranking, which preserves symbol-associated local frequency information rather than reducing everything to anonymous MTF ranks.

Implement clean-room from the published algorithm description or use libbsc only as an oracle/reference executable. Do not silently import its coder into ANVIL.

Required ablation:

```text
same BWT
  -> current MTF/RLE best
  -> QLFC-like postcoder
  -> raw-BWT control
```

### P1.4 LZP → BWT control

**EV:** high on repetitive/mixed text  
**cost:** medium  
**risk:** medium

libbsc exposes LZP ahead of BWT. ANVIL should test the interaction explicitly rather than assuming BWT alone should recover all repetition.

Key experiment: LZP on/off with the same BWT/postcoder and exact full-payload accounting.

## Tier 2 — build a cheap learned router, not an expensive encode-all production path

### P2.1 Offline oracle-label router

**EV:** high  
**cost:** medium  
**risk:** medium

This is the most promising “outside the box but practical” direction.

Generate a training table from real blocks:

- features computable in one cheap scan;
- true Brotli/BWT winner and byte delta from encode-all oracle.

Candidate features:

- byte histogram / H0;
- sampled H1 / conditional entropy;
- run-length statistics;
- zero-byte / ASCII fractions;
- distinct 4-byte hash rate;
- repetition-distance samples;
- simple delta/stride entropy;
- container signatures;
- cheap BWT-preview proxy on a small sample;
- block size / file offset.

Fit a **tiny deterministic decision tree or integer score**, not a neural model. The model is encoder-only; decoder already reads backend ID.

Objective should be regret-weighted: misclassifying a 10-byte difference is cheap; misclassifying `mozilla` by megabytes is catastrophic.

This can replace expensive encode-all routing after the research oracle is characterized.

### P2.2 Change-point segmentation

Instead of fixed blocks, detect distribution changes and route homogeneous segments. Candidate signals: rolling byte-distribution divergence, repetition-rate change, container boundaries.

Only pursue if fixed-block oracle shows substantial within-file heterogeneity.

## Tier 3 — decode-safe specialists (ranked ABOVE the general CM, re-rank 2026-09-07)

> **Re-rank consequence of the decode-amortization analysis (`findings/decode-amortization-deadend`):** the byte win and the decode win are in structural conflict (~7× over budget); region-scoped BWT routing cannot rescue decode because the BWT-favorable files are 56.8% of Silesia. Therefore a backend's *decode axis* is now a first-class ranking input, not a footnote. **P4.1 (DEFLATE reconstruction) ranks ABOVE P3.1 (CM)** because P4.1's target files (mozilla/sao/ooffice/samba) are Brotli/xz-routed with fast decode — its ~0.66-MB gain costs **no** decode regression. CM (P3.1) would very likely decode *slower* than BWT, worsening the axis. The full P4.1 write-up is in Tier 4 below; it is promoted here in rank.

### P3.0 Bit-exact DEFLATE reconstruction — promoted above CM

See **P4.1** (Tier 4) for the full pipeline and the ~430 KB mozilla go/stop bar. Promoted
in *rank* (not moved) because it is the only currently-visible byte win that does not
fight the decode axis. Fund after P1.1/P1.2 prove the routing substrate.

## Tier 3.5 — the main general-ratio event (CM)

### P3.1 Fixed-point context-mixing backend

**EV:** very high  
**cost:** high  
**risk:** medium-high

The published lpaq/mcm/cmix family shows several megabytes of Silesia and enwik8 headroom beyond Brotli/BWT class.

Architecture:

- byte contexts order 1…6;
- word model;
- match model;
- fixed-point logistic-domain mixing;
- SSE/APM refinement;
- binary arithmetic coder;
- fixed wire memory/model parameters for version 1.

Important: first objective should be **a reproducible point on the rate/time/RAM curve**, not “beat cmix.” Pick memory and throughput bounds before implementation.

The backend registry makes this lower-risk: CM can be a specialist ID 3 while Brotli/BWT remain fallbacks.

## Tier 4 — structure exposure where external evidence says it pays

### P4.1 Bit-exact DEFLATE reconstruction — **#1 next investment (re-rank 2026-09-07)**

**EV:** high but concentrated → **now the highest-EV next investment**  
**cost:** high  
**risk:** medium

Published precomp/cmix evidence localizes large Silesia gains to `mozilla` and `samba`, exactly two files where direct BWT performs poorly.

**New quantified hook (doc 10 §3):** the BWT-vs-xz disagreement map shows the 664,307-B gap between the 2-backend oracle (46,446,836) and the 3-way landscape oracle (45,782,529) sits *entirely* in mozilla/samba/ooffice/sao — the archive-heavy files xz already wins. That is a measured, file-localized ~0.66-MB missing family, not a speculation. **Concentration:** mozilla alone is 429,893 B (64.7%) of that gap; sao 24.2%, ooffice 7.8%, samba 3.4%. So P4.1's concrete go/stop bar is **~430 KB recoverable on mozilla** — a mozilla-specific DEFLATE-reconstruction mechanism that cannot plausibly recover >430 KB is not worth building.

**Why #1 (two independent measured arguments, 06 §G3):**
(a) **Decode-axis:** P4.1's target files (mozilla/sao/ooffice/samba) are Brotli/xz-routed with fast decode, so its gain costs **no** decode regression — the only byte win that does not fight the ~7× decode conflict (unlike BWT/CM).
(b) **Routing-axis:** E4 (`findings/e4-block-routing-negative`) proves you **cannot** recover those files by carving out text regions (zero BWT-winning blocks there), so reconstructing the embedded compressed streams is the only route. bwt-theory T2 confirms mozilla's BWT loss is heterogeneous compressed regions, not structural hostility.

Fund P4.1 next (ahead of CM/P3.1 and after the now-DONE routing substrate P1.1).

This complementarity is strategically attractive and now numerically grounded.

Pipeline:

```text
container scan
  -> DEFLATE parse
  -> plaintext + reconstruction info
  -> validate bit-exact replay during encode
  -> compress transformed representation with Brotli/BWT/CM
  -> opaque fallback on any failure
```

Microsoft `preflate-rs` is a strong architectural reference: predictor parameters + encoded corrections, supporting multiple DEFLATE implementations and unknown-stream fallback.

### P4.2 Binary stride/lane transforms

**EV:** medium-high  
**cost:** medium  
**risk:** high

The old simplistic hypothesis “low H1 means ctx1 wins” is false. Instead measure candidate transforms end-to-end under **each backend**.

For `x-ray`, 2-byte grayscale delta/transpose remains a falsifiable first candidate. For `mr`, ctx1 already gives signal, but BWT direct gives a much larger win—so stride transforms must beat that stronger baseline, not old Brotli.

### P4.3 Specialized media/container recompression

Brunsli/JPEG, packJPG-like, PNG/DEFLATE reconstruction, etc. High local gains possible but every specialization increases decoder-size and maintenance cost. Admit only with corpus-frequency and program-size accounting.

## Tier 5 — broaden falsification

### P5.1 Pizza&Chili Real + Logs

Essential for repetitive-data claims and grammar/long-range mechanisms. Record δ, z, r, g, H0–H8 as diagnostics, not score targets.

### P5.2 Small-block regime

Measure 1/4/16/64 KiB independent blocks and quantify:

- framing cost;
- model warmup;
- static/trained dictionary benefit;
- backend-selection overhead.

This is especially important because BWT’s tiny-file control already shows warmup/header weakness.

### P5.3 Long-range dedup / FastCDC

Useful only if new repetitive corpora demonstrate meaningful redundancy beyond backend windows. Measure chunk-dedup oracle before integrating.

## Portfolio interactions worth exploiting

The most interesting new strategy is **complementarity**, not isolated winner chasing:

- BWT is strong on `webster`, `x-ray`, `mr`, text-like/homogeneous regions;
- Brotli is strong on archive/mixed regions;
- deflate reconstruction specifically targets archive-heavy `mozilla`/`samba`;
- CM can cover general statistical redundancy later.

The backend portfolio should be evaluated as a set-cover problem over corpus failure modes.

## What “outside the box” means here

Not adding exotic transforms randomly. The higher-leverage reframing is:

> **Compression as hierarchical model selection over reversible representations and specialist backends, with the encoder spending expensive computation only where an oracle says the regret matters.**

That is compatible with the project’s strongest engineering lessons and the new third-party data.

