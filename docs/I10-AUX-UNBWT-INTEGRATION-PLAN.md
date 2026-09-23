# ANVIL I10-1A — Auxiliary-Index inverse-BWT Integration Plan

**Date:** 2026-09-23
**Status:** implementation-ready design; source change deferred until the I10 remote baseline is closed
**Evidence base:** `prototypes/i9-bwtinv/{RESULTS.md,INTEGRATION-SPEC.md}`
**Novelty:** none; adopt-class decoder engineering

## 1. Purpose

Integrate the already-measured `libsais_bwt_aux` / `libsais_unbwt_aux` mechanism into ANVIL's BWT ratio backend as a **separate, forced experiment first**.

The integration must preserve three things simultaneously:

1. existing BWT payloads remain byte-decodable;
2. the auxiliary index's wire bytes are fully charged;
3. the experiment can be forced on/off without silently changing the frozen I9 byte baseline.

The measured prototype result is strong enough to justify integration but not strong enough to justify changing the default representation before a remote end-to-end Pareto measurement.

---

## 2. Current source reality

Current symbols in `src/anvil.cpp`:

- `Options::bwt_post`
- `Options::bwt_subblock`
- `bwt_backend_encode(...)`
- `bwt_backend_decode(...)`
- `ratio_backend_encode(kRatioBackendBwt,...)`
- `ratio_backend_decode(kRatioBackendBwt,...)`

Current bare BWT payload:

`[post_id 0..4][primary uvar][postcoder payload]`

Large BWT inputs may be wrapped by the existing ratio-backend subblock frame:

`[0xFF][nsub uvar] { [decoded_len uvar][payload_len uvar][BWT payload] }...`

The outer `0xFF` tag is consumed by `ratio_backend_decode` before the individual BWT payload reaches `bwt_backend_decode`.

Current BWT post IDs are `0..4`, and the format contract says new IDs are added, never redefined.

---

## 3. Wire design

### 3.1 Use a BWT-payload sentinel, not an ambiguous in-band version byte

The I9 handoff proposed putting an auxiliary version after the postcoder byte. That has an ambiguity problem: an old payload's first primary-index varint byte can equal a putative version value.

Use a leading sentinel instead.

### Legacy v1 payload

`[post][primary uvar][postdata]`

### Auxiliary v2 payload

`[0xFE][post][r uvar][icount uvar][I[0..icount-1] u32le][postdata]`

Why `0xFE`:

- legacy BWT payloads begin with post IDs `0..4`;
- the BWT subblock wrapper's `0xFF` is handled one layer above;
- `0xFE` is therefore unambiguous at the `bwt_backend_decode` boundary;
- old files remain readable by the new decoder;
- no existing postcoder ID is redefined;
- the one-byte sentinel is negligible relative to the real auxiliary-index cost.

The v2 payload does **not** need a separate primary field. `I[0]` is the primary index produced by `libsais_bwt_aux`. If a postcoder requires primary semantics, it uses `I[0]`.

Do not repurpose `0xFF`; preserving the existing layer boundary makes malformed-input validation substantially simpler.

---

## 4. Encoder control

Add an encoder-only option:

`--bwt-aux=off|on`

with:

- default: `off`;
- decoder: auto-detects v1/v2 from the payload and requires no option.

Reason for default-off during integration:

- v2 deliberately adds wire bytes;
- the frozen I9 baseline must remain byte-identical unless the experiment is explicitly selected;
- a decoder speed feature should not silently change a ratio-only cell;
- the remote A/B can compare exactly the same parser/postcoder/backend with only the inverse-BWT representation changed.

A later routing policy may choose v2 automatically using a frontier-derived byte/cycle budget. That decision is **not** part of the first integration.

---

## 5. Sampling policy

For single-thread decode, use the already-measured ~1024-walk target:

```text
target_blocks = 1024
r = smallest power of two >= ceil(n / target_blocks)
r = max(r, 2)
icount = 1 + floor((n - 1) / r)
```

With ANVIL's current BWT input cap (`n <= INT32_MAX`), this computation can be performed safely in 64-bit arithmetic before narrowing `r` to `int32_t`.

For `n == 1`, retain the legacy v1 payload. There is no useful auxiliary-inversion work to parallelize, and the existing single-byte special case is already correct.

Do not add OpenMP in I10-1A. The citation axis is the measured same-core-count 1-thread auxiliary path. Multi-threaded auxiliary inversion is a separate experiment because it changes the resource axis.

---

## 6. Encode path

When `bwt_aux == off`:

- execute the current `libsais_bwt` path exactly;
- emitted bytes must remain byte-identical to the frozen source.

When `bwt_aux == on` and `n > 1`:

1. choose `r`;
2. allocate `I` with exact `icount`;
3. call `libsais_bwt_aux(..., r, I.data())`;
4. require return code `0`;
5. validate `I[0] in [1,n]` before serialization;
6. build the same BWT postcoder candidates from the resulting BWT bytes;
7. serialize every candidate with the v2 auxiliary header so candidate selection sees the complete charged payload;
8. retain the smallest complete candidate exactly as today.

Important: do **not** select the postcoder on a header-free payload and append `I` afterward. The complete candidate size is the only honest selection objective.

The prototype established that `libsais_bwt_aux` produces the same BWT bytes as `libsais_bwt`; the integration test must reassert this on the remote runner rather than assuming it.

---

## 7. Decode path and validation

At entry to `bwt_backend_decode(p,n,expected)`:

### v1

If first byte is not `0xFE`, execute current semantics unchanged.

### v2

If first byte is `0xFE`:

1. require another byte for `post`;
2. parse `r` and `icount` as uvarints;
3. validate before allocation/read:
   - `expected > 1`;
   - `r >= 2`;
   - `r` is a power of two;
   - `r <= INT32_MAX`;
   - `icount == 1 + (expected - 1) / r`;
   - `icount <= expected`;
   - `icount * 4 <= remaining payload` using overflow-safe arithmetic;
4. read exactly `icount` little-endian `uint32` values;
5. require every `I[t] in [1,expected]`;
6. decode the postcoder payload into exactly `expected` BWT bytes;
7. call `libsais_unbwt_aux(..., r, I.data())`;
8. require return code `0`.

ANVIL should validate all of the above itself rather than relying on libsais to reject malformed wire data.

The vendored libsais implementation accepts `r == n` for the legacy single-primary inverse path even when `n` is not a power of two, but **v2 will never serialize that control form**. v2 uses only power-of-two `r <~ n`; legacy v1 remains the representation for the single-primary path.

---

## 8. Memory and overflow discipline

New hot allocations for v2:

- encoder: `I[icount] * 4`;
- decoder: same index vector in addition to the existing `out(expected)` and `tmp(expected+1)`.

At the ~1024-block policy this is only a few KiB for normal whole-file BWT blocks.

Still enforce:

- 64-bit computation of `icount` and serialized byte count;
- bounds before `vector::resize`;
- no `expected+1` overflow before allocating the libsais temporary array;
- no pointer advance before checking `remaining >= icount*4`.

No new unbounded allocation may be controlled solely by hostile wire metadata.

---

## 9. Required tests before benchmarking

### 9.1 Legacy byte-identity gate

With `--bwt-aux=off`:

- same source/input/options;
- encoded bytes must be exactly identical to the pre-integration binary for all existing forced BWT postcoders that are valid today;
- normal fuzz/registry tests remain green.

This is the most important regression gate.

### 9.2 v2 direct forced roundtrip

Add direct tests that force:

- BWT backend;
- auxiliary v2;
- postcoder 1;
- postcoder 2;
- postcoder 3;
- sizes around policy boundaries;
- `n=2`, small random, repetitive, text, binary, and a multi-MiB input.

Each must encode -> decode byte-exact.

### 9.3 BWT/index equivalence

For sampled deterministic inputs:

- `libsais_bwt` BWT bytes == `libsais_bwt_aux` BWT bytes;
- `legacy_primary == I[0]`;
- both inverses reconstruct the original input.

### 9.4 Malformed-wire tests

At minimum:

- truncated after `0xFE`;
- invalid post;
- `r=0/1`;
- non-power-of-two `r`;
- impossible `icount`;
- truncated index array;
- index 0;
- index > expected;
- truncated postdata;
- trailing data violations already enforced by each postcoder.

### 9.5 Subblock composition

Force `--bwt-subblock` low enough to create multiple BWT subblocks and verify:

- outer `0xFF` framing + inner `0xFE` payloads compose correctly;
- decoded length sum is exact;
- no sentinel ambiguity exists.

---

## 10. Remote experiment design

Do not use the canonical baseline job as the candidate measurement.

Create a dedicated same-job A/B experiment from one source checkout:

- A: `--bwt-aux=off`;
- B: `--bwt-aux=on`;
- same postcoder/backend/transform/input;
- exact compressed bytes;
- exact decoded hash;
- same runner;
- interleaved decode repetitions;
- raw repetitions retained;
- CPU affinity;
- ambient-load gate;
- paired effect estimate under `docs/github-actions-benchmark-protocol.md`.

Primary deterministic cost:

`aux_bytes = size(B) - size(A)`

Primary performance effect:

`paired decode-time ratio B/A`

The experiment should report both BWT-stage-specific harness timing and whole-codec decode timing. The stage speedup is mechanism evidence; the whole-codec result determines Pareto value.

---

## 11. Promotion rule

I10-1A can become a default-capable representation only if all of the following hold:

1. legacy-off byte identity is exact;
2. v2 roundtrip/fuzz is green;
3. measured whole-codec decode improvement is material;
4. the added index bytes are fully charged;
5. the resulting point improves or enables movement toward the production reference front;
6. no hidden thread-count/resource change is involved.

Even after promotion, keep the v1 decoder indefinitely. New wire semantics are additive.

---

## 12. Explicit non-goals

I10-1A does **not** include:

- OpenMP inverse BWT;
- a new postcoder;
- postcoder optimization;
- changing BWT subblock size;
- changing ratio transforms;
- P4.1 reconstruction;
- automatic frontier routing;
- a novelty claim.

Those are separate causal experiments.

The purpose of I10-1A is to answer one clean question:

> **What does the measured auxiliary-index inverse-BWT mechanism buy ANVIL end-to-end when its exact wire cost is charged and every other representation choice is held fixed?**
