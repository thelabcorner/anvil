# A84 — Hutter Prize ideas as Pareto seeds, not ANVIL's objective

**Direct user clarification (19 Nov 2026):** PROJECT ANVIL seeks general-purpose Pareto dominance over common lossless algorithms across compressed bytes, encode/decode time, peak RSS, decoder footprint, correctness, streaming and safety. Winning the Hutter Prize, or minimizing text-only bits per character at any CPU cost, is NOT an objective. Use prize winners as a technical research library, never as the new optimization metric.

## Exact claim contract

ANVIL A dominates reference R on declared data iff all measured costs are no larger—B_wire(A)<=B_wire(R), encode_time(A)<=encode_time(R), decode_time(A)<=decode_time(R), RSS(A)<=RSS(R), decoder_size(A)<=decoder_size(R)—and at least one inequality is strict, with all hard correctness/safety/streaming requirements satisfied. Stronger aspirations of strict improvements on every axis require stronger measurements. The frozen R-2 `FRONT-CROSSING` is non-dominance in TWO axes; it is NOT full Pareto dominance, and neither are a Hutter ranking or enwik8/enwik9 payload bpb. Report separate comparisons against each mainstream reference and every relevant heldout family.

## Source-grounded mechanism transfer matrix

| Hutter winner / implementation | Extractable primitive | Potential benefit | Risk/negative gate | Disposition |
|---|---|---|---|---|
| **fast-cmix** (2023 winner) | Devirtualize hot loops, store hot context state contiguously, replace poor hash layouts | Same wire bytes; possible encode + decode CPU gains and lower pointer/cache overhead | No automatic ratio gain; identify actual ANVIL hot call site and cache miss bottleneck before change; binary footprint | **1. First performance seed** |
| **fx2-cmix** (2024 winner) | Skip low-error mixer updates, prune unhelpful predictors, select contexts sparsely | Less per-bit decode CPU, model state and potentially smaller binary | Different adaptive states affect future probabilities and compressed bytes; shadow prediction/state ablation before wire mode | **2. Conditional** |
| **PAQ8HP / ZPAQ** (2006/07 winner lineage) | Confidence-gated sparse bit histories, lightweight hashed match prediction, tiny SSE/APM calibration | More predictable residual bytes at modest tables | Per-bit arithmetic/range coding sequential decode may destroy existing fast rANS throughput; cold-start and state memory | **3. Exploratory only** |
| **fx2-cmix / cmix-lex** (2024/26 winners) | Reversible dictionary and word/type-aware preprocessing in compact, locally detectable regions | Possible text and structured ratio savings | Domain specificity, side information, dictionary footprint, lexical bit-exactness, seekability | **Conditional small transform** |
| **STARLIT** (2021 winner) | Group related articles for context reuse | Potential bulk-archive ratio | Global data reorder + permutation map, high encode CPU, random access / streaming harm | **PARK for default** |
| **cmix / fx2-cmix-T** (2010s–2026) | Thousands of context experts and LSTM/transformer prediction | Extremely strong text-only bpb | High per-bit decode CPU, memory, model weights and code size; fails all-axis premise without extraordinary speed evidence | **PARK for default** |

Primary sources: https://www.hutter1.net/prize/ (official winner dates); https://github.com/saurabhk/fast-cmix (devirtualization, cache locality and hash maps); https://github.com/kaitz/fx2-cmix (error-threshold update skipping and predictor pruning); https://mattmahoney.net/dc/dce.html (PAQ logistic mixing, SSE/ISSE and match models); https://github.com/amargaritov/starlit (Doc2Vec/article ordering); https://www.byronknoll.com/cmix.html (many models; better ratio but slower and more memory); https://github.com/blahem/cmix-lex (PHDA9 lexical/tail transform). These are prior art, NOT ANVIL benchmark results or novelty claims.

## ANVIL-specific measured constraints

- Research ledger A/D: adaptive Fenwick arithmetic decoded at ~20 MB/s, static separated rANS at ~164 MB/s on task document (about 8x faster for +1.13% bytes). Any adaptive predictor must prove it can preserve fast decoding; do not assume arithmetic coding is free.
- Experiment C: naive order-1 literals increased output 11,107→11,491 B (+3.46%) from cold starts. Higher order alone is already a documented negative, not a new hypothesis.
- A44: Silesia's cost gap is decode versus Brotli (~2.3618x timed candidate), although cap-8 has favorable total wire bytes relative to xz/Brotli. Fix a genuine decoder bottleneck before trading speed away for bpb.
- A58/A59: generated.json still has ~10,417 B deficit versus Brotli q11; it is synthetic, and sibling generated.jsonl/sqlite datasets do not constitute independent heldout validation (A74/A75).

## Decision-changing work order

1. Keep **A70 draft PR #24** libsais 2.12.2 version-only decoder scout as near-term systems priority, correctly controlling old-wire identity, same-host CPU/ISA, RSS, binary size, full output SHA and randomized paired/null timing. Do not promote from source copy alone.
2. Trace the actual decode critical path. If a time-significant hot site exists, test ONE fast-cmix-inspired devirtualization/layout/hash change under byte-identical wire, no-regression and binary-size controls. No CI unless it has an experiment that can flip the engineering decision.
3. Only if ratio still binds and causal/systems bounds permit it, measure an opt-in **small** confidence-gated sparse context/match expert on existing residuals; first estimate net wire bytes and decode cycles, then integrate. Never default-enable a neural expert or import cmix wholesale.
4. For promotion, compare against pinned zstd, Brotli, xz/LZMA, DEFLATE and speed-oriented references as appropriate, across genuinely unrelated heldouts, with full-wire/decoder-size/RSS/encode/decode/safety/streaming measurements and frozen Class-A labels kept separate.

**A83 correction:** docs/A83-PREQUENTIAL-PREDICTOR-RESEARCH-PREREG.md and tools/a83_prequential_oracle.py are *optional exploratory prior-art probes*, not the active roadmap. The oracle outputs ideal log-loss, NOT an actual compressed bitstream or ANVIL performance. Reject any Hutter-leaderboard or sub-1-bpb optimization as a replacement target. Research = VERIFY current speed mechanisms first / PARK heavy language models; execution = TEST bounded manual GitHub Actions only when the approved workflow is ready.

## Cost-of-selection theorem for ANVIL's auto codec (do not mistake a byte oracle for a Pareto oracle)

The existing `ratio_backend=auto` encoder computes Brotli q11 and aux-BWT, selects the smaller wire payload, and pays **both computational costs**. From A48's raw same-host medians on four files, the automatic encoder was only ~5–10% slower than Brotli alone, because q11 consumed most CPU time. A size-first portfolio therefore cannot demonstrate encode-time dominance over its fastest constituent simply because its chosen output is smaller.

A cheap predictor that chooses an encoder WITHOUT running both can save time but may choose a suboptimal payload. Exact no-regret behavior requires a sound certificate (for example, an algorithmic lower bound on the skipped candidate's possible wire size) or doing enough work to establish both sizes; empirical confidence alone is not a guarantee. Any speed-oriented policy must report **byte regret**, false-negative rate, predictor runtime/bytes, worst-case behavior and heldout evidence separately from the exact size-first policy. This is a serious mechanism-level constraint, not a mere user-interface tuning choice.

Likewise, a universal method cannot be claimed to dominate on all data because it won one corpus or benchmark. On incompressible random input, count headers, expansion, cache and scheduler costs; on streaming workloads, include resets, window memory and seeking overhead. Report per-reference, per-file dominance decisions and the aggregate only when scientifically appropriate, rather than averaging away hard failures.
