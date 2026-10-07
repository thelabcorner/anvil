# ANVIL I16 — Byte-Aligned Record Frames: Frozen Discovery Preregistration

**Date:** October 7, 2026. Frozen BEFORE any I16 C++ compile, codec operation or benchmark. CPU-heavy work must execute only in GitHub Actions; no delegates, swarms or local native compiler runs.

**Candidate source:** prototypes/i16-byte-frame/i16_byte_frame.cpp, SHA256 **682f8086e852b8a285f939879c19648ece500efc1a5071b4cde1914c44606f73**, Git blob **fda906e6590e0eda3c55ee23b492f2904d148c59**.
**Frozen I15 comparator:** prototypes/i15-field-diff/i15_field_diff.cpp, SHA256 ce9ff217ca32cae761362e11261ee9fdacaca7b27dcb86d9b2d41fb6a73ff612, Git blob fa60ec8fdab9931b5befae6e6575de8eb5ff0321.
**Pinned reference:** Brotli google/brotli commit ed738e842d2fbdf2d6459e39267a633c4a9b2f5d, q5/q11, lgwin22. Same runner, source-pinned binaries and seven fixed tracked discovery inputs.
**Role:** research/discovery only. No heldout claims, general-purpose Pareto frontier promotion, claimed novel mathematics, or release replacement.

## 1. Motivation and disjoint causal variable

I13's 32-bit-word stride extraction recovered 407 fields in the 280,000-byte synthetic time-series but exceeded Brotli q11 full bytes by 4,669. I14's local modular differences reduced arithmetic source to 33,196 bytes. I15 composed both, beating Brotli q11 on interleaved time-series with **128,560 AVI5 vs 134,718 Brotli q11 bytes**, model bearing 336 DIFF columns and ~2.96x digest-inclusive decode, while still losing to faster-encoding **Brotli q5 120,594 bytes** (by 7,966). The next distinct hypothesis is that incorrect field frame geometry is binding: the synthetic generator writes **14-byte** little-endian records with timestamp u64, float32, channel u16, while I15 models 32-bit strides and may straddle unrelated fields.

I16 tests **byte-accurate record framing** independent of special semantics. It does not hardcode any timestamp, float or channel field definition. The candidate stride list **{10,12,14,16,20,24,28,32} bytes** and phase list **{0, next file-origin-aligned record start}** are fixed *before* benchmarking. This does incorporate a file-origin alignment assumption, transparently; sources with headers or shifting record boundaries may not be discoverable. The particular 14-byte hypothesis is known from the *previously consumed* generator and is NOT independent evidence of general schema discovery.

## 2. Exact mathematical wire representation

A file contains independent max-4096B blocks, original AVI6 magic + canonical output-byte varint. Preserve all exact I15 existing choices: RAW outer0, whole-block AFF3, strided COL4 with field local DIFF submode, and whole-block DIFF5. Add only outer tag6 **FRAMED** with:

- Tag(1), canonical varint(block_bytes), stride(1), phase(1), literal first phase bytes
- Then a shortest-path partition of field offsets j∈[0,stride) into nonoverlapping contiguous spans. For a literal span: mode0, byte span length(1), record count × span width literal bytes transposed by record. For modeled fields: mode w∈{1,2,4,8}, first w bytes, median modular step w bytes, residual bitwidth 1 byte, bitpacked records−1 signed modular local innovations. Each uses exact complete serialized bytes.
- Then last (block_bytes−phase) mod stride literal bytes. Frame decoder reconstructs every source byte in original order with checked, bounded scatter and sequential fixed-width residual unpack.
- Record count is floor((block_bytes−phase)/stride) and must be >=3. No decoder search, no floating-point rounding or context history. Full 256MiB output cap, canonical varints, wire exhaustion and packed-zero-padding enforcement.

For each stride and each of at most two phases, compute shortest path via dynamic programming over field offsets. The objective is MINIMUM **ACTUAL SERIALIZED BYTES**, including mode tags, width, first word, step, bitstream, all literal data, prefix/tail, framing. Ties preserve existing candidates; FRAMED is selected only if its complete bytes beat ALL retained I15 block candidates. This is not full unconstrained table-schema inference and is not a novel primitive; similar transposition/predictive coding in Sprintz and FastLanes is prior art.

Decoder recurrence is x_i = (x_(i−1) + d + unzig(e_i)) mod 2^(8w). All arithmetic is unsigned modular, with signed modular median only encoder-side. Never learn fields, sort data, infer phase or fit a model in the decoder.

## 3. Frozen seven-file old discovery population

- tests/corpus/synth-arith.bin
- tests/corpus/synth-timeseries.bin
- tests/corpus/synth-jitter.bin
- tests/corpus/random.bin
- tests/corpus/generated.repeat.jsonl
- tests/corpus/generated.jsonl
- tests/corpus/src.cpp

Each input verified by Git blob and SHA256 at runner checkout. Absolutely no new held-out sources; benchmark on the exact old synthetic cohort. Full byte-identical roundtrips for both I15 and I16 before any performance conclusion.

## 4. Hypotheses preregistered BEFORE CPU execution

- **H1 structure:** I16 must select >=1 actual FRAMED block with >=1 actual FRAMED modeled field on synth-timeseries.bin, roundtrip byte identical.
- **H2a incremental bytes:** I16 full wire on that file strictly < *same-run* I15 full wire. H2b **stronger**: I16 must strictly beat *same-run* Brotli q5 full wire (historical 120,594B), not merely q11.
- **H3 decode rate:** I16 decode+every-byte-digest on frame-bearing timeseries > same-run Brotli q5 decode+digest; report q11 and I15 too. A RAW-only speed win does not qualify.
- **H4 retained-choice invariant:** for every one of seven files, complete AVI6 bytes <= same-run AVI5 bytes (identical I15 candidates unchanged); synthetic arithmetic must not exceed existing 33,196B.
- **H5 decode safety:** 7/7 complete byte-exact file roundtrips for both source-pinned candidates; in-source positive FRAMED selftest (14-byte mixed-field opaque+predictable+tail) and malformed wire/truncation/padding/phase/stride/count adversarial checks, source attestation and retrievable artifact.
- **H6 work budget:** enumerate complete encode, decode, binary size, memory estimate/peak RSS where feasible, per-mode adoption, framed metadata cost and negative controls. Compare I16 encoding rate against I15. A size win that makes encoding substantially slower is a Pareto tradeoff, not dominance.

## 5. Frozen rulings

- Any provenance mismatch, compilation failure, invalid expected selftest or roundtrip, malformed accepted, missing data/artifact => INVALID-PREMEASUREMENT; repair and re-pin without efficacy claim.
- H1 false => KILL-FRAME under frozen catalog. No silent expansion.
- H1 true / H2a false => STRUCTURE-SELECTED-NO-INCREMENT.
- H2a true, q5 byte gate H2b false => FRAMED-BYTE-ADVANCE-Q5-ADVERSE.
- H2a/H2b true, H3 false => FRAMED-RATIO-ONLY-DISCOVERY.
- H1/H2a/H2b/H3 all true => FRAMED-SPECIALIZED-DISCOVERY-CANDIDATE **only**.
- H4 falsified is an algorithm/accounting defect: invalidate publication until corrected.

## 6. Mandatory controls and limitations

- Compare known numeric source with matched actual Brotli q5/q11 and frozen I15, not an optimistic substream or isolated residual.
- No general-purpose claim: numeric pilots remain grossly worse than Brotli on JSONL/text. Need a strong general-purpose fallback with accounted whole-file mode header/encode cost.
- Phase anchoring partly assumes source starts in record alignment, and known 14-byte stride is in the candidate catalog, so I16 success measures representation ceiling more than general structural intelligence.
- Simulator-like synthetic data cannot certify real sensory/telemetry utility. Independent provenance-locked real numeric families, FastLanes/Sprintz/Gorilla/ALP and byte-shuffle+Brotli are mandatory for promotion.
- Encoder dynamic programming is heavier than I15; ratio results without encode-rate sustainability cannot be called Pareto wins.
- Historic Class-A frontier remains 435 dominated / 28 degenerate / 5 unresolved / zero verified full crossings.

## 7. Build hygiene

I16 lives in isolated /documents/ANVIL-I16 worktree with script tools/i16_assemble.py deterministically generating its source from frozen I15. No existing R2 format, I15 measurement, shared canonical dirty checkout, agent state or corpus file is modified. All native code work runs in GitHub Actions. Source Git blob and SHA frozen above; any premeasurement source correction must be recorded with new hashes before rerun.
