# Lane (a) slice-4 dispatch package — transform-4 codec integration + wire freeze

Status: SPEC (this file) → implement on `research/anvil-p41-slice3-20261117`
(base: slice-3 GO `13cfb63`). Manual-only `ubuntu-24.04` workflow; raw
artifact upload; no local compute. First slice that touches `src/anvil.cpp`
— additive only, default-off, existing paths byte-identical.

## Contract (frozen inputs — do NOT re-derive)

- Prereg: `docs/P41-DEFLATE-REPLAY-PREREG-DRAFT.md` FROZEN (gates 1-3;
  reopen/stop rules). No novelty claim, ever.
- Integration plan: `docs/I10-DEFLATE-REPLAY-INTEGRATION-PLAN.md`
  §§6-13 (representation, registry, scanner, acceptance, decoder,
  bounds, build, correctness gates).
- Gate ruling I9-5: 2,354 DEFLATE / 2,564 detected; 2,331 valid /
  2,898,132 B; samba 160/160.
- Slice-3 evidence (run 44065311538, `13cfb63`): native harness reproduces
  the frozen population exactly with vendored zlib 1.3.1
  (`engine=zlib-1.3.1-pinned-v1`); registry covers 100% (codes 0/1/2, zero
  escape needed). Vendored set = the 10-file static lib proven on CI:
  `{adler32,compress,crc32,deflate,inffast,inflate,inftrees,trees,uncompr,zutil}.c`.
- FORMAT.md: mode-17 envelope `transform_id | backend_id | uvarint len |
  backend payload`; transform IDs 0/1/2 shipped, 3 disabled-NO-GO.
- Fuzz precedent: `tests/fuzz.py::adversarial_rev2` treats transform id 4
  as `invalid-transform-id-4` (must flip: 4 becomes legitimate; malformed
  t4 BODIES must reject instead).

## What slice-4 builds (all additive; default behavior unchanged)

0. Vendored engine already in tree (`third_party/zlib/` + UPSTREAM.md).
   CMake: private static target `anvil_zlib_pinned` from the SAME 10-file
   set proven in slice-3 (verbatim: adler32 compress crc32 deflate inffast
   inflate inftrees trees uncompr zutil), `-O2 -DNDEBUG` both toolchains,
   linked into `anvil` ALWAYS (decoder must read t4 wire unconditionally).
   `ANVIL_HAVE_ZLIB_PINNED` define. MSVC + non-MSVC blocks mirror the
   libsais stanza. Decoder binary-size delta reported (plan §12).
1. Parameter registry (frozen, immutable): code 0 = (-15,6,8,0); code 1 =
   (-15,6,9,0); code 2 = (15,6,8,1); 255 = escape w/ explicit params
   (wbits zigzag-varint + level + memLevel + strategy, 4 trailing raw
   bytes). Mirrors harness `registry::Decode/Encode/IsValidParam` exactly.
2. ZIP method-8 scanner (encoder-only, fail-closed, plan §8): local-header
   + central-directory bounds with checked arithmetic; method 8; reject
   encryption; explicit data-descriptor handling (reject unless sizes known
   pre-read: v1 rejects descriptors); exact compressed sizes; reject
   overlaps (sort + interval check); cap entries at 10,000 and metadata at
   block size; never trust an offset pre-check; ZIP64 rejected in v1;
   unsupported = opaque (no transform emitted). Scanner input is the
   ENCODE block (not whole files) — offsets are block-relative in every
   record, so the transform is block-self-contained like t0/t1/t2.
3. Encoder (`--ratio-deflate=on|off`, DEFAULT OFF): scan block → for each
   candidate inflate via pinned zlib (bounded output = declared size cap)
   → try registry codes [0,1,2] in order → bounded fallback: full grid
   levels 1-9 × memLevel 1-9 × strategies {0,1,2,3,4} with a hard per-block
   search cap (20,000 deflate calls; exceeding the cap fails the candidate
   to raw, never aborts the block) → require byte-exact replay, else raw.
   Build t4 records sorted by offset; construct representation §6;
   compete t4 vs t0/t1/t2 under the SAME backend via existing `consider()`;
   existing smallest-payload-wins rule unchanged. With flag OFF (default):
   zero behavior change — `consider(4,…)` never runs; t4-off bytes are
   existing-path identical by construction.
4. Decoder (always on — decoders read all valid wire): parse version
   (must be 1), engine id (must be 1), bounded record count, per-record
   gap/compressed/plaintext/param with plan §11 bounds; copy gaps; replay
   via pinned engine; require replayed length == declared; exact
   consumption; exact final size; else throw (malformed). Wired into
   `decode_ratio_block` as `transform==4` arm; guard becomes
   `transform>4`; transform 3 rejection UNTOUCHED.
5. Fuzz (`tests/fuzz.py`, same commit — CI fuzz must stay green):
   flip `invalid-transform-id-4` → transform 4 is now legitimate; ADD
   malformed-t4 suite: bad version, bad engine id, bad param code,
   truncated varint at every byte of a small record, gap overflow,
   plaintext overflow, replayed-length mismatch, truncated tail,
   final-size mismatch, over-bound record count. All must reject (rc≠0).
   Keep every existing case green (incl. transform-3 rejection).
6. FORMAT.md: new `### Transform 4 (DEFLATE-REPLAY-v1)` section under the
   mode-17 registry: representation layout, registry table, engine-id
   table, bounds list, decoder algorithm reference. NO changes to existing
   sections (byte-identical rest of file).

## CI gates (manual workflow `anvil-p41-slice4.yml`, same conventions)

1. Build (clang-18 + clang++-18, cmake Release, Ninja; fingerprint +
   vendored-tree sha256 list + binary shas in provenance — mirrors
   research-bench + slice-3 patterns).
2. Frozen-population roundtrip via the REAL codec: for mozilla + samba,
   `anvil c --parse=ratio --ratio-deflate=on` → `anvil d` → sha256-exact
   vs source; assert t4-was-used evidence (backend payload differs from
   t4-off build on mozilla — i.e. the lane actually exercised, not silently
   skipped) AND assert valid-count accounting matches frozen
   (2331/2898132 + 160/160) from CI logs/CSVs.
3. Existing-path identity: same files with `--ratio-deflate=off` (and
   default flags) produce byte-identical outputs to the pre-slice-4
   baseline binary built from the parent commit on the same runner image
   (build both in one job; sha256 compare). Any divergence → STOP.
4. Malformed suite: harness selftest PASS + fuzz.py PASS (incl. new t4
   cases + all legacy rev2 cases) + adversarial ZIP fixtures
   (encryption/descriptors/malformed/ZIP64/duplicates/fake signatures/
   truncation — encoder must stay silent/opaque, never crash, never emit).
5. Forced registry coverage: fuzz/roundtrip with forced transform 4
   where applicable (registry codes 0/1/2 exercised; escape path exercised
   via synthetic vector since frozen population needs no escape).
6. Decoder binary-size delta: report `anvil` size vs pre-slice-4 baseline
   binary + pinned lib size (plan §12 inputs).

## STOP conditions (binding, from prereg + work-order)

- Replay-population divergence unexplained → STOP.
- Any roundtrip / malformed / scanner / fuzz failure → STOP.
- Decode/RSS cost erases the rate surplus vs the live front → STOP (note:
  slice-4 asserts correctness + identity only; the surplus ruling belongs
  to the promotion gate, explicitly NOT this slice).
- PNG/PDF/generic-corrections/recursive scope → STOP (out of prereg scope).
- Any change to existing wire bytes or decoder behavior for old modes →
  STOP and revert.
- No novelty claim, ever (adopt-class).

## Explicitly out of slice-4 (deferred, not dropped)

- Paired A/B promotion (direct/Brotli vs replay/same-Brotli with bootstrap
  CI, null spans 1.0, decode/RSS/encode/binary-size axes, same-transform
  reference control) — needs the I20 gate-closure vehicle; separate
  dispatch after slice-4 lands green.
- enwik8/structured corpora t4 measurement — same vehicle, later.
- Encoder search-speed work (bounded grid is fine for v1; the corpus proof
  is byte-exactness, not encode throughput).
- Escape-code wire use on real data (implemented + tested synthetic-only;
  frozen population needs none).
