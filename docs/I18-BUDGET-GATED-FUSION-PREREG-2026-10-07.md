# I18 — Budget-Gated AVI6 + AVI4 + Brotli q5 Portfolio (Pre-Measurement Registration)

Date: 2026-10-07 (America/Chicago). Status: **UNMEASURED PROPOSAL** until a source-attested GitHub Actions job finishes. Research only; not production format, Class-A certification, or a mechanism-novelty claim. No delegated agents. All nontrivial compilation, selftests, fuzzing and timing on GitHub Actions only.

## Source lineage and epistemic limits

- Parent I17 immutable checkpoint: `a26d770c400cd05b26195180ee4da157a8ee9c73`, confirmed run [37701773624](https://github.com/thelabcorner/anvil/actions/runs/37701773624) (all six limited discovery gates passed).
- I17 `prototypes/i17-fast/i17_fast.cpp`: sha256 `727469a41c5506d73f85d0fdc1405f067c1206ad5de94063f1a06605b15c9cf6`. Its frozen I16-envelope/AVI4 dependencies remain untouched.
- I16 fusion exact source, copied verbatim for isolated inclusion as `prototypes/i18-budgeted/i16_fusion_frozen.inc`: sha256 `b214dcff0355293dd588bd762c7328dfce6e114749669f2b7a32d8c052f51dc3`, source commit `9ca860f45d7dad1b617267372e378f18a93f2721`. It is namespaced with no semantic edits; its independent AVI6 inverse is invoked only for AVI6 mode.
- Static pinned Brotli source `ed738e842d2fbdf2d6459e39267a633c4a9b2f5d`, q5 and q11/lgwin22, identical control workload.
- Seven previously consumed inputs: synth-arith.bin, synth-timeseries.bin, synth-jitter.bin, random.bin, generated.repeat.jsonl, generated.jsonl, src.cpp. They are discovery inputs and **must never** be described as fresh holdout.
- I17 legacy full/fast outputs are independent controls; new AVH2 profile is not ABI compatible with AVH1. Every frame has its own count and mode, and all overhead counted.

## Frozen proposed AVH2 strategy

For each nonempty input, encode Brotli q5 once. Initialize selected payload as raw. If q5 output is strictly shorter, select it. Skip both mathematical candidate encoders entirely whenever q5 output size is at most one eighth of the uncompressed length, or when a bounded, cheap 32-bit strided local-difference probe sees no highly regular lane. Otherwise, encode AVI4 then AVI6 and select only strictly shorter **complete payloads**. The probe is a work-bounded rejection filter and can reject numeric data: this is acceptable for correctness, but it creates measurable opportunity loss. The filter may produce false positives and encoder-work cost. Every selected mode has an independent exact decoder. q11 is *not* evaluated by the new budgeted encoder.

AVH2 framing: 4-byte ASCII magic, canonical bounded ULEB128 uncompressed-byte count, one-byte mode, then raw/AVI4/AVI6/Brotli payload. All modes reject invalid magic, unknown mode, truncated payload, trailing bytes where the underlying codec can detect them, wrong output length, over-cap output and noncanonical counts. RAW exact payload-length validation and empty RAW roundtrip are required. This is a distinct experimental wire; do not introduce it into existing ANVIL production code.

## Competing hypotheses and locked verdicts

- **H0 correctness / provenance:** pinned blob/sha256 of frozen parents; all selftests, canonical framing mutation tests, and all seven files roundtrip in budget/legacy-fast/legacy-full; encoded and decoded contents consumed by timed digest. Any failure => INVALID-INFRA, no efficacy conclusion.
- **H1 mathematical gain:** on the arithmetic discovery fixture, AVI6 selected; AVH2 complete output **strictly smaller than 33,204 B** I17 fast, with no change to the AVI6 leaf. If not => NO-NUMERIC-INTEGRATION.
- **H2 retaining fast path:** on time-series, budget output must be no bigger than I17 fast 120,602 B and select q5. If it does not => BUDGET-SELECTOR-REGRESSION.
- **H3 negative-control work:** on jitter, random, repeated/generated JSONL, and src.cpp, the per-input decision to skip expensive numerical candidates, final wire, all extra candidate calls and paired encode rates are recorded. No retrospective exception is allowed. Failure to skip most of these => NEGATIVE-CONTROL-WORK-GATE-FAIL, regardless of H1.
- **H4 encode throughput:** on arithmetic, budgeted encode must exceed same-job I17 full encode; on time-series, budgeted encode must exceed I17 full encode. Otherwise => NO-THROUGHPUT-RETURN. Separately report whether budget is faster/slower than I17 fast and Brotli q5. No target is changed after seeing results.
- **H5 size regret:** publish complete byte differences to I17 full, I17 fast, q5 and q11, per input and aggregated; the arithmetic improvement must not conceal degradation elsewhere. This is an evidence-completeness gate.
- **H6 decoding:** on arithmetic when AVI6 selected, real output-observed AVH2 decode rate must exceed paired q5 and q11; memory/RSS and binary bytes measured separately. If not => NO-DECODER-RETURN.
- **H7 source- and workload-equality:** all runner inputs match named HEAD Git blobs; one binary contains all four experimental controls and same-run pinned Brotli; no multi-run speed conclusions.
- **H8 structural non-novelty:** neither conditional candidate selection nor 16-value innovation tiling is presumed novel. Even all gates passing => **BUDGETED-SPECIALIZED-DISCOVERY**, zero general-purpose Class-A crossings without separate Gate A and independent real-origin heldout.

Benchmark rates include cold allocations/copies, candidate discovery, framing and selected output digests. All seven fixed files measured on one Ubuntu-24.04 runner with GCC C++20 -O3; 5+ encode and 7+ decode median trials unless unusual cost requires preregistered non-efficacy caveat. Capture peak RSS with separate `/usr/bin/time -v` one-shot trials (not conflated with throughput). Preserve machine-readable TSV, decision report, SHA provenance, source hashes, complete output archive sizes, logs and failure artifacts.

## Stopping and promotion

A green CI result means the workflow completed, **not** that the hypotheses succeeded. Record all failed gates. Prior historical frozen R2 frontier remains 435 DOMINATED / 28 DEGENERATE / 5 unresolved FRONT-GAP / zero FRONT-CROSSING. This I18 experiment does not lift the Q1a source-attestation / heldout publication block for Class-A promotion. Next experiment, if I18 passes, must use genuinely independent real structured files, matched typed-codec controls and a preregistered multi-axis comparison; otherwise terminate this branch's tuning and retain its negative results.

## Compile-only infrastructure correction (after preregistration, before any measurements)

The first I18 [Actions run 37702906749](https://github.com/thelabcorner/anvil/actions/runs/37702906749) passed exact source/corpus attestation and pinned Brotli build, then failed during C++ compilation because frozen I17 itself includes an I16 source mirror that defines and undefines the preprocessor `main` macro. Attempting to rename I17 by an enclosing `#define main` was therefore undone by the nested leaf, leaving two global entrypoints. This is **INVALID-PREMEASUREMENT**, not a failed codec gate.

The correction preserves the source-pinned I17 pilot byte-for-byte and introduces a derived `prototypes/i17-fast/i17_fast_entry_renamed.inc` with exactly one entrypoint identifier substitution, checked byte-for-byte against the frozen source on Actions. SHA-256 of the mirror: `dbd0b11cac966a7c5aedf603c3ba7d30b8a4f63b039deb48cebbadbc37d02615`. The corrected I18 prototype includes that derived mirror instead of the original source; there is no other algorithmic change, altered corpus, gate, or reference. I18 SHA-256 becomes `e5fd1a2c44d697f7cfd49427e1ca3654b192cfff07168e67409007b6f97d65e2`. Preserve both runs, and base any efficacy conclusions only on the re-attested corrected revision.
