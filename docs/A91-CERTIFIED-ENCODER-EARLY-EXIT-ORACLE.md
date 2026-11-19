# A91 — Certified early stopping for competing encoders (mechanism proposal)

**Scientific status:** EXPLORE / not tested. This is a candidate way to reduce ANVIL's **encoding cost without sacrificing exact compressed bytes**, more aligned with the human's all-axis Pareto objective than accepting unmeasured byte regret from a speculative fast codec selector. **Do not implement or dispatch until the cheap oracle falsifier below.**

## Grounded starting point

At pinned `src/anvil.cpp:3920-3937`, Brotli q11/lgwin30 currently runs through the **one-shot** `BrotliEncoderCompress()` API. At `:4794-4841`, `encode_ratio_block()` first computes Brotli for each transform, then computes the BWT candidate and retains the smaller complete payload. Hence the *auto* encoder pays both CPU costs; skipping BWT on loser files saves at most the BWT side of the work, while skipping an entire q11 encode requires certifying that doing so cannot change the final selection.

The official [Brotli v1.1.0 source](https://github.com/google/brotli/blob/v1.1.0/c/enc/encode.c) implements the one-shot call by creating a streaming encoder with QUALITY, LGWIN, MODE, SIZE_HINT and possibly LARGE_WINDOW, then calling `BrotliEncoderCompressStream(...BROTLI_OPERATION_FINISH,...)`. However, the one-shot function **can fall back to an uncompressed Brotli stream** on failure or an output-size bound. Thus merely assuming an arbitrary partial stream is a prefix of the final one-shot result is unsafe.

## Branch-and-bound hypothesis

Compute a complete, deterministic BWT candidate first (keeping exact byte identity and selection policy), then run **the same Brotli settings and process** in a resumable streaming fashion. Observe an immutable prefix of emitted compressed bytes `e`. If a certified lower bound on the **final one-shot Brotli candidate size**, including any fallback behavior and the ANVIL transform/backend envelope, is **strictly larger** than the current complete BWT candidate size `B`, Brotli cannot win. Abandon that Brotli arm early and select BWT. Otherwise let Brotli finish.

A sufficient bound for the success path is emitted prefix length plus required outer header. But the **fallback path must also be bounded**: for compressible inputs where `B < input_bytes`, a fallback uncompressed stream cannot beat B assuming its complete wire length is at least the input size (must confirm this invariant in pinned Brotli source). Use the **minimum of all possible completion-path lower bounds**, not simply `e`; fail closed when fallback or streaming equivalence is uncertain.

**Strict `>`, never `>=`:** the current implementation tries Brotli first and only replaces the incumbent on a strictly smaller payload; ties favor Brotli. A reordered BWT-first implementation must preserve that tie behavior and thus cannot abandon Brotli while an equal-byte completion remains possible.

Importantly, API output bytes and processed-input count may become visible only **after most expensive q11 computation has already occurred**. Output-size progress is not a measurement of CPU-time remaining. The overall speed gain could be zero or negative, and reordering can increase memory lifetime.

## Existing A48 data give only a rough ceiling on stream-output fraction

On complete A48 file results, approximate BWT/standalone-Brotli size ratios were:

| File | Smaller ANVIL full-wire B | Standalone Brotli q11 | B / Brotli | Possible prefix threshold |
|---|---:|---:|---:|---|
| dickens | 2,636,635 B | 2,827,779 B | 93.24% | Brotli must already have emitted ~93% of final compressed bytes |
| mr | 2,413,023 B | 2,823,137 B | 85.47% | Brotli must have emitted ~85% of final compressed bytes |

The ratios are **illustrative and not exact payload-to-payload comparisons** (ANVIL includes an additional wrapper), and emitted-output fraction is NOT input consumed or CPU work completed. There is no justification for claiming a 6–15% time saving.

## Smallest discriminating experiments

1. **Oracle (first):** on canonical pinned files, instrument an *independent* Brotli streaming q11/lgwin30 encoder with precisely the one-shot parameters, including SIZE_HINT/LARGE_WINDOW and BWT-first incumbent. Record at each visible output return `(compressed_output_bytes,input_consumed,elapsed_cpu,total_would_be_fallback)`; stop only on a certified safe lower bound. When allowed to finish, require bit-for-bit equality with the original one-shot Brotli output and exact wrappers/transform; else PARK.
2. **Causal comparison:** distinguish a Brotli-output-prefix certificate, a fallback-size certificate, and a no-certificate control; do not conflate cancellation due resource limits with a proof of best-size selection.
3. **Promotion:** only if actual early cutoff returns before significant CPU cost, implement an opt-in auto-mode selector with **identical full ANVIL wire SHA on every input** (including ties, errors, incompressible controls, changed transforms), then independently verify paired encode/decode, RSS, decoder size, source hashes and streaming on manual hosted GitHub Actions.
4. **Stop:** no early prefix until q11 has done most CPU work, one-shot/streaming outputs differ, fallback cannot be safely bounded, added memory outweighs benefits, or control-arm encode timing does not improve under paired CI. Do not trade byte regret for a claimed Pareto win.

Prior-art category: branch-and-bound encoder pruning, not new information theory. This is a **different** mechanism from A90's arithmetic divide sharing and A70's existing libsais decoder update, so only one should consume the next scarce CI slot according to its predicted decision value.
