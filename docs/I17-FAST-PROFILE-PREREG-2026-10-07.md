# ANVIL I17 — Two-Point Encoder-Work Portfolio: Frozen Preregistration

**2026-10-07. Discovery only.** No delegation, swarms or local CPU-heavy testing. Build, selftest, corpus, benchmarks and artifact checks via manually dispatched GitHub Actions ONLY.

**Candidate:** prototypes/i17-fast/i17_fast.cpp; SHA-256 a81d7de528992da94dce305a5099370f1083c7e03d70b8520dbb8a8b8e9138d0; git blob 4ae591f7a04b3a9836721ceb77fd2057d69ed7fd.
**Frozen full-selector leaf:** prototypes/i16-envelope/i16_envelope.cpp; git blob 617147803d519e12cf3a6c74878c7125aef6b96c; SHA-256 0162ccfab90607393698e12d410509e3d09e31cf77336139e42a830a4837a418.
**Frozen DIFF leaf:** prototypes/i14-differential/i14_differential.cpp; blob 365348ac5b8295f32eed97fb9427bb5516c43b16; SHA-256 f4f75b098feee9753084fa5cc132e31064f107fbd8ffa621f5e415b57d825150.
**Reference:** pinned Brotli ed738e842d2fbdf2d6459e39267a633c4a9b2f5d; quality 5 and 11, same compiler/run, lgwin22.
**Population:** seven fixed already-consumed sources in the same order as I16. This is not a heldout test or general-purpose Pareto claim.

## The two distinct objective profiles

I16 exact-size profile pays for running AVI4, Brotli q5, and expensive Brotli q11 on every nonempty source. This gives the smallest of the three **complete nested archive** sizes, but I16 timeseries encoder rate was only 0.354 MB/s while the *selected* q5 standalone reference encoded at 29.886 MB/s. The bottleneck is encoder candidate *search work*, not decoding.

I17 introduces a **fast** profile, running only AVI4 + Brotli q5; it does not compute q11 inside its timed encoder path. The AVH1 wire and decoder are unchanged, allowing exactly comparable framing; the fast output may be larger than the full output.

Exact oracle for raw size n>0:

    B_fast(x) = 5 + uleb128_size(n) + min(B_AVI4(x), B_Brotli_q5(x)).
    B_full(x) = 5 + uleb128_size(n) + min(B_AVI4(x), B_Brotli_q5(x), B_Brotli_q11(x)).

Therefore byte regret B_fast-B_full is >=0, measured and published per source. This is classic portfolio selection, not new entropy coding or mathematical novelty. The fast mode trades this regret for encode work. If the two modes compress identically, the faster one may dominate within this limited portfolio; external competitor viability must still be checked.

## Frozen hypotheses and negative criteria

- **H1:** fast mode selects I14 DIFF on synthetic arithmetic, yields exactly the same compressed wire length as full, and outperforms paired q5 byte size and decode rate.
- **H2:** all seven fast/full exact byte-oracle equalities and selected IDs pass, with same-run codec reference sizes; any byte inconsistency INVALID.
- **H3:** on arithmetic and time-series, fast encoder MB/s exceeds full profile encoder MB/s in matched timing with *every selected encoded byte digested*.
- **H4:** same-run time-series fast encoded size equals full encoded size and chooses q5; fast source still roundtrips independently.
- **H5:** repeated JSONL, generated JSONL, jitter, source code and random negative controls included with full per-file exact byte regret. The fast mode does not get to hide its loss when q11 wins substantially.
- **H6:** all files roundtrip under both profiles, malformed/truncated inputs rejected by parent decoder, source/corpus hashes/pinned Brotli recorded, results retained on failed job. Full and fast compiled into one same-run binary so no runner confounding.

**Adjudication:** any build, correctness, source or oracle failure = INVALID-PREMEASUREMENT, no efficacy interpretation; repair only under new source revision. All H1-H6 pass = FAST-ENCODE-TRADEOFF-DISCOVERY (not a general-purpose verified Pareto crossing). H1/H2/H6 pass but speed fails = FAST-SEARCH-NO-THROUGHPUT-RETURN. H1 fails = NO-SPECIALIZED-FAST-PATH. Byte regret is a measured cost, not a threshold to adjust post hoc.

All experiments obey frozen window/geometry from I14's 4 KiB numeric blocks plus independent whole-file q5/q11. Codec library binary/RSS/startup remain separate axes; reference selector inherits Brotli's decoder. Underlying code includes only the two source-pinned prior leaves and a small fast-selector; original research modes are immutable.

**Next if successful:** replace expensive all-candidate size search with a cheap conservative selector, but only under a new preregistration with a fixed maximum byte-regret or encode-time budget and independent real-source validation. Do not call a synthetic Class-A crossing. Return to typed numeric codecs Sprintz/FastLanes and modern LZ controls before mechanism claims.
