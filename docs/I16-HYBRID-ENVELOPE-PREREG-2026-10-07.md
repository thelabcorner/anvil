# ANVIL I16 — Whole-File Adaptive Envelope (Frozen Discovery Preregistration)

**Date:** 2026-10-07. **Class:** experimental fallback engineering, never mechanism novelty or general-purpose Pareto promotion. Registered before any I16 compilation, tests or benchmarks.

**Source:** prototypes/i16-envelope/i16_envelope.cpp; SHA-256 0162ccfab90607393698e12d410509e3d09e31cf77336139e42a830a4837a418; git blob 617147803d519e12cf3a6c74878c7125aef6b96c.
**Numeric leaf:** frozen I14 source prototypes/i14-differential/i14_differential.cpp; SHA-256 f4f75b098feee9753084fa5cc132e31064f107fbd8ffa621f5e415b57d825150; git blob 365348ac5b8295f32eed97fb9427bb5516c43b16.
**Reference:** pinned google/brotli commit ed738e842d2fbdf2d6459e39267a633c4a9b2f5d, q5 and q11, lgwin22, generic, built on matched GitHub Actions Ubuntu 24.04.
**Discovery population:** seven previously consumed tracked files in fixed order: synth-arith.bin; synth-timeseries.bin; synth-jitter.bin; random.bin; generated.repeat.jsonl; generated.jsonl; src.cpp from tests/corpus. Not heldout.
**Execution:** all C++ compiles, benchmarks, tests, fuzz and CPU-intensive tasks GitHub Actions ONLY. No workers/delegates/swarms. Canonical dirty checkout untouched.

## Causal question
I14's special arithmetic advantage (33,196 B against Brotli q11 87,013 B) coexists with catastrophic RAW-only losses on repetitive JSONL (936,694 B against Brotli q11 160 B). This is fundamentally a missing dictionary/entropy fallback, not a minor numerical predictor shortcoming. ANVIL must be capable of using its mathematical reconstruction only when cheaper than competent generic compression.

## Exact AVH1 format
The complete wire is a four-byte ASCII magic AVH1, canonical unsigned LEB128 uncompressed byte count (bounded at 256 MiB), one-byte payload codec ID (1 frozen AVI4, 2 Brotli q5, 3 Brotli q11), followed by the complete independent codec stream. Encoder runs all candidates for every nonempty source and chooses the shortest *fully serialized candidate*; ties go AVI4, then q5, then q11. Empty source uses AVI4. Decoder rejects invalid magic/tag/size/varint/truncation and checks reconstructed size and nested decoder result.

For nonempty byte sequence x of length n, exact wire-size oracle:
    
    B_AVH1(x) = 5 + uleb128_bytes(n) + min(B_AVI4(x), B_Brotli_q5(x), B_Brotli_q11(x)).

This proves an explicit bounded framing regret, NOT general compression superiority. If the reference codec wins, its CPU cost and binary library still belong to the envelope; q11 search is extremely expensive and this selector is not a fast encoder.

## Frozen hypotheses
- **H1** arithmetic selects real DIFF-bearing AVI4, beats full q11 bytes.
- **H2** repeated-JSONL selects Brotli (q5 or q11), closing RAW catastrophe within precisely charged framing overhead.
- **H3** timeseries selects q5: on the pre-consumed input q5 120,594 B beats q11 134,718 B; cannot assume q11 is always the smaller reference.
- **H4** on all seven files, fully encoded AVH1 byte sizes and chosen IDs exactly match independent complete-byte oracle, using deterministic tie rules. All source and corpus hashes asserted.
- **H5** arithmetic AVH1 digest-inclusive decode > same-run q11 decode scout, with encode and decode rates separately reported and no fabricated confidence interval.
- **H6** every full-file roundtrip is bit-exact, source-pinned C++ selftests including malformed/truncated wires pass, every-byte decode digest consumed; binary hash/size retained, artifacts uploaded on failure.

## Frozen adjudication and falsification
Source mismatch, invalid codec, benchmark-oracle mismatch, missing evidence, build failure, roundtrip failure, malformed acceptance => INVALID-INFRA. H1 or H2 false => FALLBACK-SELECTION-FAILED. H1,H2,H4,H6 true and any H3/H5 false => HYBRID-ARCHITECTURE-PARTIAL-DISCOVERY. All true => HYBRID-ARCHITECTURE-VALIDATED-DISCOVERY. None is a general-purpose multi-axis Pareto-frontier crossing, or mechanism novelty. The frozen Class-A 435 dominated, 28 degenerate, five unresolved, zero crossings remains unchanged.

## Follow-ons, strictly independent
A whole-file envelope cannot model heterogeneous numeric/JSON segments within one input; a future segmented causal experiment must charge mode boundaries, context/dictionary reset, warmup, decoder memory, encoding search and window parity. A cheap prediction-first gate that skips expensive q11 is allowable only as a new preregistered speed/ratio tradeoff (no longer the exact minimum-size oracle). Compare independent real numeric sources and typed codecs before promotion. No AVH1 release-format integration until separate compatibility/security/corpus review. No representative RSS, cold startup, or reproducible throughput CI is available in this pilot; do not claim them.
