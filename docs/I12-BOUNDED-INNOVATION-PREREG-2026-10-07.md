# ANVIL I12 — Modular Predictor + Dense Bitpacked Innovations (Frozen Discovery Pilot)

**Preregistered:** 2026-10-07, before any I12 compilation or codec experiment.  
**Code:** \`prototypes/i12-bounded-innovation/i12_innovation.cpp\`  
**Source SHA-256:** \`3e6c24c90eb4026cca2520e88413caece25f847771f4e0c6513c48964a05992c\`  
**Git blob:** \`2e6ea6492f970ee56366b5530872da193a815226\`  
**Role:** engineering/discovery ONLY; no promotion, general-purpose frontier crossing, or novelty claim.  
**Execution:** all C++ builds, codec tests, fuzz/sanitizers, and benchmarks on GitHub Actions only. No delegates or swarms.

## 1. Causal motivation: authoritative I11 R2 failure

[I11-R2 Actions 37696816081](https://github.com/thelabcorner/anvil/actions/runs/37696816081) passed source pinning, pinned reference build, C++ compilation, selftests, seven file roundtrips, benchmark and evidence upload. Its *algorithmic* preregistration failed:

- \`synth-arith.bin\`: zero affine/patch blocks, AVI1 256,196 B vs Brotli q11 87,013 B;
- \`synth-timeseries.bin\`: zero affine/patch blocks, AVI1 280,214 B vs Brotli q11 134,718 B;
- \`synth-jitter.bin\`: one patched block, AVI1 975,604 B vs Brotli q11 62,717 B;
- remaining four discovery files: zero modeled blocks, all worse on bytes.
- All timings in this run include a decoded-byte digest. The raw-dominated AVI1 decode speed (roughly 3.3 GB/s) cannot be represented as a useful compressed-codec Pareto crossing.

The first \`synth-arith.bin\` words vary by roughly 63–65 per sample, instead of a single constant finite difference. Thus *dense bounded innovations* and *sparse replacement exceptions* ask substantially different questions. I11's robust median predictor still required exact matches in >=75% of fields and was the wrong residual code for dense noise.

## 2. Frozen mathematical reconstruction

For each independent block of <=4096 bytes and each aligned word width \`w ∈ {1,2,4,8}\` dividing block length:

1. Interpret values as little-endian unsigned \`8w\`-bit words \`x[i]\`.
2. Work in the ring \`Z/(2^(8w))\`. Convert modular differences to their signed two's-complement representatives.
3. Estimate \`d\` as median of all \`x[i] - x[i-1]\` signed modular differences. This is encoder-only \`O(n)\` via \`nth_element\`.
4. With \`s = x[0]\`, estimate base \`a = s + median_i(x[i] - (s + i*d))\`, with signed modular residuals and modulo arithmetic.
5. Reconstruct \`p[i] = a + i*d\`. Compute modular innovations \`e[i] = x[i] - p[i]\`.
6. ZigZag signed modular innovations to nonnegative \`z[i]\`. Set \`k = bit_length(max_i z[i])\`.
7. If \`k < 8w\` and \`k <= 56\`, serialize every \`z[i]\` as exactly \`k\` low bits into a contiguous, little-bit-order stream; zero extra padding bits. Otherwise reject the model candidate.
8. Select only when **complete serialized candidate bytes** are strictly smaller than the \`RAW\` candidate. Ties and adverse candidates choose RAW, never a speculative entropy estimate.

**AVI2 wire:** \`ASCII AVI2 | canonical uvarint(uncompressed_bytes) | tagged independent blocks\`.

- \`RAW\`: \`tag 0 | uvarint(n) | n original bytes\`.
- \`INNOV\`: \`tag 3 | u8 width | uvarint(count) | base[w] | step[w] | u8 k | ceil(count*k/8) packed bits\`.

INNOV byte cost is exactly \`3 + varint_size(count) + 2w + ceil(count*k/8)\` including tag/width/bitwidth. Decode uses only sequential fixed-width unpacking, modular arithmetic, and contiguous writes. Block maxima, total 256 MiB output ceiling, wire consumption, padding, varints, unknown tags and word widths are checked. No pointers, external dependencies, dynamic model search, or decoder-side sorting.

**Novelty:** median slope, delta transforms, ZigZag, bounded residuals, bitpacking and per-block adaptive selection all have substantial prior art. This is engineering research, not a newly claimed invention. Compare especially Frame of Reference/PForDelta, FastLanes, Sprintz, Gorilla/Chimp and ALP, with matching source schemas before claiming anything broader.

## 3. Frozen hypothesis matrix

- **H1 (arithmetic recoverability):** on *previously consumed discovery* \`synth-arith.bin\`, at least one I12 INNOV block is selected, with exact byte-identical roundtrip. A zero result falsifies this mechanism at its frozen geometry.
- **H2 (arithmetic complete bytes):** AVI2 full wire bytes < Brotli q11 complete bytes on \`synth-arith.bin\` at the frozen runner comparison (Brotli v1.1.0, q11, lgwin22). This tests whether denser mathematical coding carries enough benefit to overcome its new headers. The 4096B blocks and Brotli's window are *not equal geometry*; this is a discovery result only.
- **H3 (decoder efficiency):** paired same-run digest-inclusive AVI2 decoded MB/s > matched Brotli q11 decoded MB/s *on any source where INNOV is actually selected*. An AVI2 win with exclusively RAW blocks is explicitly disallowed as evidence for H3.
- **H4 (negative controls):** high entropy and repeated JSONL must report their actual full bytes, with no forced model; RAW overhead may lose heavily to Brotli, and this is not ignored. All seven original I11 discovery files are carried forward without omissions.
- **H5 (adversarial correctness):** empty, short, block boundaries, affine, bounded jitter, modular wraparound, random, invalid headers, invalid/truncated streams and padding must be rejected/reconstructed correctly.

No efficacy thresholds are changed after seeing the pilot. No held-out samples are opened; these are known discovery fixtures.

## 4. Scope and evidence contract

The seven fixed tracked source paths:

1. \`tests/corpus/synth-arith.bin\`
2. \`tests/corpus/synth-timeseries.bin\`
3. \`tests/corpus/synth-jitter.bin\`
4. \`tests/corpus/random.bin\`
5. \`tests/corpus/generated.repeat.jsonl\`
6. \`tests/corpus/generated.jsonl\`
7. \`tests/corpus/src.cpp\`

Workflow must assert exact commit checkout, source Git blob and SHA-256, per-input tracked blob and SHA-256, Brotli upstream commit \`ed738e842d2fbdf2d6459e39267a633c4a9b2f5d\`, clean checkout, complete roundtrip, actual benchmark TSV, JSON diagnostics, binary size/hash, pinned compiler/runner metadata, and artifact upload even on failure. Manual-only \`workflow_dispatch\`, \`contents: read\`, no secrets, no automatic triggers or matrix escalation.

Bytes are deterministic when the source and input identities match. GitHub shared-VM throughput is scouting evidence: do not splice independent runner generations or call an unpaired speed difference definitive. Report q5 and q11 separately, wall encode/decode + digest medians, selected mode counts and per-block aggregate residual bitwidth counts.

## 5. Strict continuation and kill decision

- Any C++ compile failure, bit-exact mismatch, source drift, malformed-input acceptance or reference-source mismatch ⇒ **INVALID-INFRA**, repair identity before measuring.
- H1 fails (no modeled arithmetic block) ⇒ **KILL this frozen predictor pilot**, pursue stride/field extraction only with new evidence; no secret threshold change.
- H1 passes but H2 fails ⇒ **NO-BYTE-WIN**, keep as possible transform control; require residual entropy or alternate representation *before* another pilot.
- H1 and H2 pass but H3 fails ⇒ **RATIO-ONLY DISCOVERY**, no Pareto claim, profile packed-bit decoding before more search.
- H1,H2,H3 pass ⇒ **SPECIALIZED DISCOVERY CANDIDATE**, not promotion: still missing fair block/window controls, RSS, binary footprint, independent genuine numeric families, admissible corpus graph and comparison against advanced typed codecs.
- Across all results, R2's **435 dominated / 28 degenerate / 5 unresolved / zero crossings** remains the Class-A state.

## 6. Follow-on branches (conditional and competing, not funded automatically)

**A. Better full-wire economics:** bitpack the dense residual as above, then compare only *complete* encoded streams against a Brotli-entropy residual carrier including backend framing and reset. Reject if residual output is merely relabeled original data.

**B. Correct strided structure:** detect record width/field offsets via reproducible low-work evidence (not unrestricted brute-force), then encode one or more predictable columns and a byte-exact interstitial stream. Compare to byte shuffle + Brotli, typed Gorilla, ALP/FastLanes, not just raw Brotli.

**C. Decoder fast path:** on real model-bearing files, evaluate 1/2/4/8-byte specialized unpack and SIMD tiled widths; ensure equal logical reconstruction, full byte consumption and same-job baseline. Measure memory/RSS and output-write bandwidth separately.

**D. Whole-system Pareto:** only if real independent data confirms improvement, integrate bounded model selection as an optional leaf of a high-throughput LZ backend. It must not silently replace canonical BWT, inherit unearned novelty, or reclassify R2 evidence.

**E. Better general compression:** in parallel, prioritize Q2 full-geometry attribution and fast LZ/entropy reference floors; one numeric win cannot establish general-purpose frontier superiority.
