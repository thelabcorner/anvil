# A83 — Causal Prediction as Lossless Compression: research preregistration

**Scope:** Independent, low-cost conceptual/oracle lane. **Status:** EXPLORE / not benchmarked / no new codec or CI. Keep A70 libsais version-only scout and frozen Class-A adjudication separate.

## Origin and fundamental equation

On Nov 19, 2026 the user supplied the transcript of the Nov 17 3Blue1Brown video explaining arithmetic coding, shared next-symbol predictors, model-size accounting, and deterministic decoder-replayed online adaptation. For a causal predictor rho, the ideal conditional content bits for a string x of n symbols are approximately B(x)=SUM over t of -log2(rho(x_t|x_<t)) + O(1). Actual transmitted bytes include framing/length, integer CDF quantization, tables, model/program/weights, residuals, and decoder dependencies. Model cross-entropy != source entropy: test on out-of-family heldout data. Prefix memorization does not prove general-purpose compression.

## Verified 2026 prior art and falsifying baselines

- Official Hutter Prize https://www.hutter1.net/prize/ lists the ACCEPTED enwik9 (1e9 source bytes) best total as 100,424,672 B including compressor plus self-extracting archive, Vladimir Ivanov fx2-cmix-T (Jul 24 2026, awarded Sep 30). Award threshold <99,420,425 B; time ~50h, one CPU core and <10 GB RAM. Other ~99.3MB 2026 entries on https://mattmahoney.net/dc/text.html are *pending* and must not be labeled accepted.
- Current winning lineage mixes bitwise statistical experts and arithmetic coding, with a ~6M-parameter quantized transformer added in 2026. Predictor mixing, arithmetic coding, on-the-fly updates, and small transformers are prior art, not novel ANVIL mechanisms.
- Nacrith https://arxiv.org/abs/2602.19626 reports enwik8 0.9389 payload bpb with SmolLM2-135M, 24-bit CDF, n-gram, adaptive bias and skip gate, but its ~500MB weights are not included in that content bpb and its enwik8 score is not comparable directly to Hutter enwik9 total bytes.
- Efficient Mismatch-Tolerant Coding for Model-Driven Compression (ICML 2026) https://web.mit.edu/jstang/www/papers/icml_2026.pdf explains floating-point prediction disagreement and PMATIC overhead. Exact encoder/decoder probabilities are a correctness constraint.
- Algorithmic Compression via Pretrained Neural Networks (Entropy 2026) https://doi.org/10.3390/e28060596 formalizes prequential entropy coding, Bayesian mixtures, cumulative log loss and complexity penalties.

## ANVIL-specific scientific baseline

Ledger Experiment A/D: adaptive arithmetic decoder ~20.16 MB/s versus separated static-rANS ~163.72 MB/s (+8x decode at +1.13% compressed bytes on the original small task document). An adaptive coder threatens ANVIL's decode Pareto axis. Experiment C: naive literal order-1 increased compressed bytes 11,107->11,491 (+3.46%) due sparse cold starts. Existing MDL parser, shape contexts, BWT, rANS and ratio selector are real mechanisms; new predictive modeling is an OPT-IN experimental candidate only.

A forward causal state-updating predictor naturally pairs with a forward range/arithmetic coder. Standard rANS encodes in reverse symbol order; using it with a forward online model requires buffering/replay of probability traces or a separate entropy protocol, with nonzero memory/latency costs. Do not imply that swapping the existing static rANS backend suffices.

## H1: cheap deterministic predictor before any LLM

Test a tiny integer-only smoothed bitwise context bank (order-0, 1, 2, 4, 8) and confidence-gated mixture. Every state transition must depend only on bytes ALREADY decoded; predict then code/decode then update. Use bounded count rescaling, fixed probability quantization and deterministic tie breaks. The initial research artifact is a **read-only prequential log-loss oracle, NOT a compressor**. Calculate ideal bits and CDF rounding overhead, not false real archive lengths; record cold-start penalty and state-memory maximum.

Break-even for ONE charged M-byte model asset over n source bytes is Δbpb > 8*M/n. A 3,000,000-byte model therefore costs 0.024 bpb over 1,000,000,000-byte enwik9 or 0.24 bpb over 100,000,000-byte enwik8 when counted ONCE. **Hutter-specific crucial correction:** its score is S_comp + S_archive, so if the same asset is embedded in BOTH the compressor and self-extracting archive, the effective model asset penalty is approximately **2*M**, hence 0.048 bpb on enwik9 or 0.48 bpb on enwik8. Reused/overlapping binary layouts must be accounted for using the actual two submitted file sizes, not an automatic 2x assumption. This also costs memory and compute. No sub-1-bit-per-byte claim without actual full-wire results.

## Preregistered gates before implementation

1. Freeze source/license/hash and genuinely independent validation families BEFORE tuning; A74 forbids generated.json/jsonl/sqlite siblings as separate holdout sources. Include random/repeat, unlike-source text and genuinely independent structured data. Do not optimize on Hutter's enwik9 and then claim untouched generality.
2. Compare prequential per-file log loss on smoothed order-0, order-1, order-2, fixed mixtures and an online mixer; test context cold start; use paired data but NO timing conclusion from a Python oracle. Predefine a minimum practical heldout bit saving NET of model/header/coder precision tax.
3. Fail closed if predictor probability is zero for any byte/bit, encoder/decoder state hashes differ, floating-point prediction changes with architecture, or noncausal lookahead appears. Test block resets, malformed/truncated streams and exact raw-byte restoration including JSON lexical details.
4. Only if log loss is decisively favorable: implement a separate versioned *opt-in* range-coded mode. Charge all compressed bytes, model data, program and decoder-only code size. Match existing Brotli, xz, zstd, ANVIL, and an actual context-mixing baseline rather than merely comparing to gzip.
5. On **manual-only hosted GitHub Actions** use a bounded scout, same-job encode/decode and RSS, exact source hashes, 9+ paired raw reps, null CIs, heldout/frozen Class-A classifier, random/repeat no-regression. Reject as general-purpose Pareto if decode, memory, wire, safety or encoder speed regresses beyond preregistered gates.
6. Causal ablation: smoothed order-0 -> confidence-gated higher order -> fixed mixer -> online mixer -> *optional* small quantized neural expert, one factor at a time. Online context mixing, PPM, CTW, PAQ, arithmetic coding and ANS are established prior art. Novelty requires a truly distinct causal mechanism and source-backed comparison.

**Disposition:** TEST only a cheap scalar oracle next, with explicit high-risk decoder-CPU headroom and no unsupported promise of a full Pareto crossing. Source-level planning only; no heavy compute, workflow dispatch or merging.
