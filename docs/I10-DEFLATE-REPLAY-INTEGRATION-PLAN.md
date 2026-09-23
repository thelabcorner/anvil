# ANVIL I10-1B — Bit-Exact DEFLATE Reconstruction Integration Plan

**Date:** 2026-09-23
**Status:** implementation-ready architecture; source change deferred until the I10 remote baseline is closed
**Evidence base:** prototypes/i9-deflate/RESULTS.md and docs/gate-ruling-i9-p41-deflate.md
**Novelty:** none; adopt-class representation

## 1. Objective

Integrate the verified P4.1 result as its own reversible mode-17 ratio transform.

The mechanism is:

original DEFLATE bytes → plaintext + compact replay description → stronger ANVIL backend → exact original DEFLATE reconstruction.

P4.1 is valuable because already-compressed data can hide redundancy from a general-purpose backend. The gain comes from making the underlying plaintext visible again while preserving binary identity of the outer file.

This is not a novelty claim. Precomp, preflate, preflate-rs, reflate and related systems occupy this family.

## 2. Frozen evidence

The mozilla prototype established:

- 2,564 detected compressed regions;
- 2,354 DEFLATE streams and 210 ZIP-stored entries;
- 2,331 of 2,354 DEFLATE streams replayed bit-exact under the bounded parameter search;
- charged prototype recovery: 1,362,177 B;
- local ceiling: 1,459,509 B;
- full transformed-file reconstruction: byte-exact.

The frozen census can be simplified further for production scope:

- ZIP DEFLATE compressed bytes: 3,132,001 B;
- PNG-zlib compressed bytes: 45,006 B;
- ZIP therefore accounts for about 98.6% of the measured DEFLATE bytes.

Replay population:

- ZIP valid: 2,289 streams, 2,856,886 compressed bytes, 8,549,850 plaintext bytes;
- ZIP diff: 21 streams, 275,115 compressed bytes;
- PNG valid: 42 streams, 41,246 compressed bytes;
- PNG diff: 2 streams, 3,760 compressed bytes.

The dominant valid ZIP replay parameter is raw DEFLATE, level 6, memLevel 8, default strategy: 2,282 streams.

## 3. Production v1 scope

Start with **ZIP method-8 raw DEFLATE only**.

Do not initially implement:

- PNG IDAT;
- PDF FlateDecode;
- generic zlib/gzip scanning;
- recursive embedded-container traversal;
- correction coding for the 23 non-exact streams.

This removes a large amount of parsing complexity while retaining essentially the entire high-value mozilla population.

A later extension must be justified by an external corpus, not by a desire for format completeness.

## 4. Ratio-transform identity

Current ratio transforms are:

- 0 direct;
- 1 previous-byte context partition;
- 2 line columns;
- 3 tombstoned LZP experiment.

Do not reuse transform 3.

Register **transform 4 = DEFLATE-REPLAY-v1**.

The outer mode-17 envelope remains unchanged:

transform ID, backend ID, transformed size, backend payload.

Transform 4 competes against transform 0 under the same backend. The primary first experiment is:

- A: direct + Brotli;
- B: DEFLATE replay + the same Brotli backend.

This preserves the causal question.

## 5. The replay engine is part of the format

Plaintext does not uniquely define a DEFLATE bitstream.

Exact bytes depend on compressor implementation, version, parameters and compression decisions. Therefore the decoder must not call whichever system zlib happens to be installed and assume that its output is format semantics.

For v1 use a **pinned replay-engine ID**.

Shortest implementation path:

1. vendor one known zlib source/configuration whose output is verified against the frozen exact-replay population;
2. link it privately into ANVIL;
3. assign it an immutable replay-engine ID;
4. treat a future engine/version as a new ID rather than silently changing semantics.

This makes decoder binary size a real cost. Record it in the Pareto report.

A later preflate-style predictor/correction engine may remove the pinned-zlib limitation, but that is a separate adopt-class project and should not block the first ANVIL portfolio experiment.

## 6. Transform representation

The Python prototype used a transformed carrier plus side metadata. Production v1 should use a direct reconstruction program.

Conceptual transform-4 representation:

    representation_version
    replay_engine_id
    record_count

    for each record:
        raw_gap_length
        original_deflate_length
        plaintext_length
        parameter_code
        raw_gap_bytes
        plaintext_bytes

    tail_length
    tail_bytes

Records are sorted by original compressed-stream offset and never overlap.

Do not transmit absolute offsets. The next offset is derivable from cumulative reconstructed output.

Do not rewrite ZIP metadata. Local headers, descriptors, central-directory data, CRCs and sizes remain raw-gap bytes. Because each reconstructed stream must reproduce exactly its original compressed length and bytes, all outer offsets remain valid automatically.

The ratio backend compresses the entire transform representation, including plaintext and metadata.

## 7. Parameter registry

Do not store four generic compression parameters for thousands of streams when the empirical distribution is extremely concentrated.

Use a small immutable parameter-code registry.

Initial codes can cover:

- raw DEFLATE, level 6, memLevel 8, default strategy;
- raw DEFLATE, level 6, memLevel 9, default strategy;
- an escape representation for explicitly encoded parameters.

The encoder may search parameters. The decoder does not search.

A record is accepted only if replay output is byte-for-byte identical to the original compressed payload.

The 21 frozen ZIP diff streams therefore remain raw in v1.

## 8. ZIP scanner requirements

The encoder scanner may fail closed and make transform 4 unavailable. It must never guess.

It must:

- validate local-header and central-directory bounds with checked arithmetic;
- identify method 8;
- reject encryption;
- explicitly handle or reject data-descriptor layouts;
- identify exact compressed sizes;
- reject overlapping candidate ranges;
- cap entry count and metadata lengths;
- never trust an offset before validating it against input length.

ZIP64 may be rejected in v1 if it is not needed for the target corpus. Unsupported structure is not an encoder error; it simply remains opaque input.

Fake PK signatures inside arbitrary payload bytes must not become accepted records unless structural parsing validates them.

## 9. Encoder acceptance rule

For every candidate method-8 payload:

1. inflate to plaintext under strict bounds;
2. try the known parameter registry first;
3. perform bounded fallback parameter search only when necessary;
4. recompress;
5. require exact byte equality;
6. if equality fails, leave the region raw.

After constructing transform 4:

- compress transform 0 and transform 4 with the same backend;
- charge every transform byte;
- emit transform 4 only if its complete backend payload is smaller.

No proxy can override the final real-wire comparison.

## 10. Decoder algorithm

The decoder does not scan ZIP.

It executes the resolved transform program:

1. parse representation version and replay-engine ID;
2. parse bounded record count;
3. for each record:
   - parse gap length, original compressed length, plaintext length and parameter code;
   - validate every length before allocation/read;
   - copy the raw gap;
   - read the plaintext;
   - replay through the pinned engine;
   - require replayed byte count exactly equals the declared original compressed length;
   - append the replayed bytes;
   - ensure reconstructed output never exceeds the original block length;
4. copy the tail;
5. require exact transformed-input consumption;
6. require reconstructed output length exactly equals the original block length.

Any discrepancy is malformed input.

## 11. Resource and security bounds

Before allocation or pointer movement:

- bound record count by transformed/original size;
- checked-add every cumulative length;
- require gap length <= remaining output;
- require plaintext length <= transformed bytes remaining;
- cap replay output by declared compressed length;
- reject impossible parameter codes;
- reject any final-size mismatch.

The decoder performs no recursive parsing and no unbounded parameter search.

The encoder must also cap parameter-search work so adversarial inputs cannot turn a compression attempt into unbounded CPU work.

## 12. Build architecture

If the pinned-engine path is used:

- vendor the exact upstream source and license under third_party;
- compile a private static replay target;
- pin build configuration that can alter compression output;
- expose a narrow wrapper to ANVIL;
- do not leak zlib state through the core codec;
- report decoder binary-size delta.

A host/system zlib can be used in research tools, but not as format-defining decoder behavior.

## 13. Required correctness gates

Before any size/performance claim:

### Existing-path identity

With transform 4 disabled, all old ratio output must remain byte-identical.

### Frozen replay population

The native implementation should reproduce the expected ZIP-valid population from the frozen mozilla evidence:

- approximately 2,289 exact streams;
- 2,856,886 original compressed bytes addressable under the frozen engine/parameter behavior.

Any unexplained divergence is a STOP.

### Full forced transform

original → transform 4 → backend → backend decode → inverse transform 4 → exact original.

Cryptographic hash must match.

### Malformed transform

Direct forced tests for:

- unknown representation version;
- unknown engine ID;
- impossible record count;
- truncated varints;
- gap overflow;
- plaintext overflow;
- invalid parameter code;
- replayed length mismatch;
- truncated tail;
- final reconstructed-size mismatch.

### Encoder scanner

Adversarial ZIP fixtures for:

- encryption;
- data descriptors;
- malformed extra fields;
- ZIP64;
- duplicate/overlapping headers;
- fake signatures;
- truncated central directory.

## 14. Remote causal experiment

I10-1B is a separate commit and run from the auxiliary-unBWT experiment.

Primary measurement:

- direct/Brotli versus replay/Brotli;
- same source checkout;
- same input;
- same outer ANVIL block framing;
- complete metadata charged;
- exact roundtrip.

Report deterministic:

- final compressed bytes;
- count/bytes of replayed streams;
- raw-gap bytes;
- plaintext bytes;
- transform metadata bytes;
- decoder binary size.

Report performance separately:

- encode time;
- decode time;
- peak RSS.

Promotion-grade timing uses the same-job paired/interleaved protocol. One hosted-runner timing is only scout evidence.

## 15. Promotion gate

Promote only if:

1. bit-exact reconstruction is robust under fuzz/malformed tests;
2. complete bytes materially improve against the same-backend direct control;
3. decoder replay cost does not erase the rate surplus against the relevant front;
4. the replay-engine code-size cost is explicit;
5. no system-library/version dependency is hidden;
6. broader reference controls are present before any general codec claim.

## 16. Explicit non-goals

I10-1B does not include:

- auxiliary inverse BWT;
- a new entropy coder;
- PNG/PDF support;
- preflate-style generic corrections;
- recursive archive extraction;
- novelty claims.

The experiment answers one question:

> Can ANVIL expose the plaintext hidden inside common ZIP DEFLATE payloads, preserve the exact original archive bytes, and gain enough complete-payload compression to justify the replay machinery?
