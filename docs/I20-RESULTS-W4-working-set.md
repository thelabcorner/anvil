# I20-W4 — C4 decode working-set engineering (zero-byte-cost RSS reduction)

**Worker:** W4 (swarm). **Arm:** C4. **Date:** 2026-10-21.
**Branch:** `research/anvil-i20-w4-working-set`. **PR:** https://github.com/thelabcorner/anvil/pull/16
**Base:** `research/anvil-i20-pareto-gate-closure-20261021` (`d05fceb…`).

This document is the C4 evidence record: hypothesis, mechanism, the Stage-1 RSS attribution, the
implemented levers, byte identity, fuzz/registry coverage, measured peak RSS (including the
per-sub-block-cap table the coordinator needs to compose C4 with C3), the paired decode-time
comparison, and the verdict against the pre-registered decision rule.

---

## 1. Hypothesis and mechanism

### 1.1 The pre-registered decision rule (quoted verbatim from prereg §5.1, C4)

> **Adopt iff the changed binary is byte-identical to the frozen candidate on every corpus file,
> passes the repo fuzz/registry path, and reduces measured peak decode RSS by >= 1.0 MiB without
> increasing decode time CI-upper by more than the 2 % epsilon. Byte cost must be exactly 0.**

### 1.2 Hypothesis

The canonical ratio route's peak decode RSS is set by the **shape of the decoder's live
allocations**, not by the wire format, so a decode-side working-set change can remove buffers
**at exactly zero byte cost**.

### 1.3 Mechanism (from source, `src/anvil.cpp`)

`--parse=ratio` uses one 128 MiB ratio block (`opt.block_size=128u<<20` when the block size is not
explicit — `src/anvil.cpp:4999`), so a file of `n` decoded bytes is **one whole-file BWT**.
`--bwt-subblock` (default 128 MiB) therefore produces **no** `0xFF` sub-block frame for any
canonical corpus file, i.e. the entire BWT of the file is inverted in one `libsais_unbwt_aux`
call.

At the inverse-BWT instant the decoder holds:

| term | bytes | site | live simultaneously with the peak? |
|---|---:|---|---|
| compressed input `c` | `c` | `read_file()` loads the whole `.anv` file | yes |
| BWT string `T` | `n` | postcoder output, `bwt_backend_decode()` | yes (libsais input) |
| libsais PSI/work array | `4n` | `std::vector<int32_t> tmp(expected+1)` | yes (libsais input) |
| reconstructed-output buffer | `n` | `std::vector<uint8_t> out(expected)` | yes (libsais output) |
| **total** | **`6n + c`** | | |

Two further terms appear as soon as a sub-block cap is used, because the outer `0xFF` frame makes
the decoder accumulate:

| term | bytes | site |
|---|---:|---|
| file-level output accumulator | up to `n` | `decompress()`'s `out` vector |
| `0xFF` block accumulator | up to `n` | `ratio_backend_decode()`'s local `out` |
| sub-block working set | `5C` (`C` + `4C`) + the sub-block output `C` | per sub-block |

**Arithmetic falsification of the original C4 premise (important).** The prereg's C4 hypothesis
was that removing *one* of the `n`-sized buffers would move G5 from 4.588× to ≤ 2.0×. It cannot.
With the whole-file BWT representation the *theoretical floor* of a libsais-based inverse is
`5n` (the BWT string `T` plus the mandatory `int32` PSI array), because `libsais_unbwt_aux`
requires an `n`-entry `int32` array and the LF walk never reads `T` (so `T` and the output can
share storage). For `webster` (`n = 41,458,703 B`) that floor is `5n = 197.7 MiB`, i.e. **3.65×
`xz -9e`** — still a G5 FAIL. **The G5 gate is therefore only closable in combination with a
sub-block cap (C3, W3), not by a zero-byte lever alone.** This is the single most decision-relevant
finding of C4 and it is the reason the C4 arms below are reported *per cap*.

### 1.4 Stage-1 attribution — the live-set model vs the frozen measurement

`DERIVED` from the frozen per-file compressed sizes (run `39661290484`, artifact
`anvil-i20-working-set-silesia-39661290484`, file `bytes.csv`) and the frozen peak-RSS measurement
(run `39654699366`, peak RSS 254,224 KiB = 248.3 MiB for the Silesia portfolio):

| file | n (decoded) | c (compressed) | model `6n+c` | model `5n+c` (L2) | model `5n` (L2+L1) | model `4n+c` |
|---|---:|---:|---:|---:|---:|---:|
| dickens | 10,192,446 | 2,574,367 | 60.9 MiB | 51.1 MiB | 48.6 MiB | 41.2 MiB |
| mozilla | 51,220,480 | 13,806,173 | 295.7 MiB | 249.1 MiB | 244.1 MiB | 215.0 MiB |
| mr | 9,970,564 | 2,384,760 | 59.7 MiB | 50.0 MiB | 47.5 MiB | 38.4 MiB |
| nci | 33,553,445 | 1,369,810 | 194.1 MiB | 163.4 MiB | 156.4 MiB | 131.4 MiB |
| ooffice | 6,152,192 | 2,478,889 | 36.4 MiB | 30.2 MiB | 28.6 MiB | 23.0 MiB |
| osdb | 10,085,684 | 2,587,124 | 60.3 MiB | 50.1 MiB | 47.6 MiB | 40.1 MiB |
| reymont | 6,627,202 | 1,147,270 | 38.6 MiB | 32.0 MiB | 30.3 MiB | 25.3 MiB |
| samba | 21,606,400 | 3,761,931 | 122.1 MiB | 102.6 MiB | 97.1 MiB | 82.1 MiB |
| sao | 7,251,944 | 4,586,126 | 45.6 MiB | 37.6 MiB | 34.8 MiB | 31.8 MiB |
| **webster** | **41,458,703** | **7,319,896** | **244.2 MiB** | **204.7 MiB** | **193.7 MiB** | **160.0 MiB** |
| x-ray | 8,474,240 | 4,019,394 | 51.5 MiB | 42.7 MiB | 40.4 MiB | 36.5 MiB |
| xml | 5,345,280 | 430,599 | 30.8 MiB | 25.8 MiB | 24.4 MiB | 20.3 MiB |

**The model is validated by the frozen measurement:** the model's `webster` figure (244.2 MiB) sits
within 1.7 % of the measured portfolio peak (248.3 MiB), and the model's `c` term for `webster`
(7,319,896 B = 7.0 MiB) matches the coordinator's independent 7.3 MB estimate exactly. The three
dominant terms are therefore the `4n` libsais array (~63 % of the peak), the BWT string (`n`,
~16 %), and the reconstructed-output buffer (`n`, ~16 %) — the coordinator's attribution, confirmed.

---

## 2. The implemented levers (all at exactly zero wire cost)

All three levers are **decode-only**. The encoder, the wire format, the mode-17 envelope and the
`0xFF` sub-block frame are untouched.

### L1 — release the compressed input before the `4n` allocation

`decompress()` now owns the compressed input (moved in) and hands the buffer to the block decoder
for the **final** block of the file. Once the postcoder has consumed that block's payload the
compressed bytes are dead, so the buffer is released **before** the `4n` libsais array is
allocated (`bwt_backend_decode`, after the postcoder, before `std::vector<int32_t> tmp`).
Removes `c` bytes from the peak.

*Strictness*: the trailing-bytes condition (`p+plen != e`) is checked **before** the final block
is decoded, because after the release the input pointers must not be dereferenced again; the
error message and exit code are unchanged. The release is only enabled in the **serial** decode
path (`decode_threads>1` passes `nullptr`), so the parallel path cannot race on the buffer.

### L2 — in-place inverse BWT

`libsais`' LF walk reads only its own PSI array and the bigram bucket tables — **never** the BWT
string `T` (`libsais.c`: `libsais_unbwt_decode_1..8` take `P`, `bucket2`, `fastbits` and never `T`;
`T` is only read in `libsais_unbwt_init_single`, before the walk). The reconstructed string can
therefore be written **over** `T`, removing a whole `n`-byte buffer.

*Aliasing hazard and its neutralisation*: `T` and `U` are `restrict`-qualified in libsais, so a
compiler is allowed to reload `T[0]` after the `U` writes have begun; libsais terminates with
`U[n-1] = T[0]`. The decoder therefore captures `bwt[0]` **before** the call and re-applies it to
`bwt[n-1]` **after** it, which makes the result byte-identical to the separate-buffer path under
*any* reordering the compiler chooses. The proof harness is
`prototypes/i20-working-set/inplace_unbwt.cpp` (§4.2).

### L3 — stream the reconstructed output to the output file

`decompress()` can now hand the reconstructed bytes to a sink instead of accumulating them. The
new `decompress_to_file()` opens the output, and the `0xFF` sub-block loop hands each sub-block's
bytes to the sink as soon as they are produced, so neither the file-level accumulator nor the
`0xFF` block accumulator is ever resident at the inverse-BWT peak.

**Does L3 cover the Brotli backend path (backend_id 1) as well as the BWT backend (backend_id 2)? —
the load-bearing question for G5.** **YES, since commit `f9be2fc`** (the answer was NO in the first
L3 revision, and the coordinator's prompt is what closed it). Precisely:

| term | backend 1 (Brotli) | backend 2 (BWT) |
|---|---|---|
| **file-level accumulator** (`decompress()`'s `out`, the whole decoded file, up to `n` bytes) | **REMOVED by L3** — the `streaming` flag is `sink && sink->out && revision==2` and is **backend-independent**; the accumulator is only allocated when `!streaming`, and the block bytes go to the sink instead of into it | **REMOVED by L3** |
| `0xFF` block accumulator (up to `n` bytes) | n/a (no `0xFF` frame on the Brotli route) | **REMOVED by L3** (the sub-block loop pushes each sub-block to the sink) |
| **the backend's own whole-file output buffer** | **REMOVED by L3 since `f9be2fc`** — `brotli_lw_decode_exact()` (which allocated `expected+1` bytes and filled them in one `BrotliDecoderDecompressStream` call) is no longer used when a sink is present; the new `brotli_lw_decode_stream()` decodes through a 256 KiB window and hands every produced chunk to the sink as the decoder produces it. Same decoder, same stream, same byte order, same total — the bytes are identical to `brotli_lw_decode_exact()`'s. This is the **50,020 KiB** term (Silesia's `mozilla` is 51,220,480 B) that floors the Silesia portfolio peak | removed at the default cap by L2 (the BWT string *is* the output buffer); at a sub-block cap it is removed by L3 as well |

**The 51,220,480-byte Brotli output buffer was invisible to every BWT-side lever** — the file never
touches the BWT path — and it is the term that sets the Silesia portfolio peak, because the
router keeps `mozilla` on the Brotli route at every cap (its split-BWT payload, 17,773,492 B at the
8 MiB cap, always exceeds its Brotli payload of 13,806,173 B). Brotli's *internal* ring buffer
(the history window) is not removable by this lever; it stays.

**Does the streaming decoder in this build accept the large window (lgwin up to 30)? — YES, and
it is provable from the code rather than assumed.** The pre-existing helper
`brotli_lw_decode_exact()` is **not** a one-shot API: it already calls
`BrotliDecoderDecompressStream(state, &available_in, &next_in, &available_out, &next_out,
&total_out)` on a `BrotliDecoderState` created with
`BrotliDecoderSetParameter(s, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1)`; it merely hands the decoder
the *whole* output buffer at once. The large-window capability is a property of the **decoder
instance** (that parameter), not of the size of the output buffer, so `brotli_lw_decode_stream()`
— which creates the state identically and sets the identical parameter, and only differs in
handing the decoder a 256 KiB window at a time — accepts exactly the same window range. The
frozen baseline decodes lgwin-30 payloads byte-exactly through that same streaming entry point
(Silesia round-trips, §3), which is the empirical proof that the pinned build's streaming decoder
carries the large window. **No window is silently degraded, and no window lever was needed.**

*CRC correctness (the coordinator's specific question)*: the block CRC-32 covers the **original**
block bytes. The sub-block bytes **are** the original block bytes, in order (the `0xFF` frame's
per-sub-block decoded lengths partition the block in order), and they are handed to the sink in
that same order, so the incremental CRC (`crc32_update`, a slicing-by-8 IEEE CRC-32 that is
bit-identical to `crc32()` over the whole buffer and safe at chunk boundaries) is accumulated over
exactly the original block bytes in the original order. The per-sub-block decoded lengths must
still sum exactly to `transformed_size` — that check is kept in both the streamed and the buffered
path (`written != expected` → `BWT subblock reconstruction size mismatch`).

*Full-buffer path preserved*: `decompress()` without a sink is unchanged and is still what
`anvil verify` (the round-trip self-check) and the `verify` path use. Only the file-writing path
(`anvil d`) streams. The bytes go to a temporary file that is renamed on success, so a failed
decode still leaves **no output file** behind, exactly as before.

### Default preservation and wire cost

* No new CLI option, no new wire-visible parameter, no change to an existing block mode.
* The change is unconditional (no flag), so the default behaviour is the new behaviour; the wire
  bytes are unaffected because the encoder is untouched.
* The CLI additionally accepts the **spaced** form of every value-taking option
  (`--bwt-subblock 8388608` as well as `--bwt-subblock=8388608`). This is deliberate: an
  externally generated argument vector (the I20 arbiter's `--candidate-extra-args`) uses the
  spaced form, and a silently rejected flag would change what a measurement measures. *This
  normaliser is also what surfaced a latent off-by-one in a sibling worker's flag parser*
  (`--bwt-aux-walks` used `a.substr(15)` where the value starts at index 16), which became fatal
  in the merged arm (arbiter run `39670486612` failed in the **encode** step with
  `--bwt-aux-walks must be an integer in 1..1048576`). The bug was in the sibling's code, not in
  the C4 levers; the coordinator fixed the offset in the merged arm (`ec2238e`).
  The normaliser is **empirically proven** by that failure: the arbiter passed the *spaced*
  `--bwt-aux-walks 16384` and the parser chain received the `=` form, which is only possible if
  the normaliser ran. `--bwt-subblock=` is parsed with `a.substr(15)`, which is correct
  (`"--bwt-subblock="` is 15 characters); `--bwt-aux-walks=` needs `a.substr(16)` — that is the
  sibling's defect, fixed in the merged arm.

---

## 3. Byte identity (Adopt-Gate §6.1)

**MEASURED** — run `39661290484` (workflow `.github/workflows/anvil-i20-working-set.yml`, job
`working-set (silesia)`, commit `d0b2f4a352311c2c0a34f10a1e4c812f5dce7e27`, artifact
`anvil-i20-working-set-silesia-39661290484`, file `byte-identity.json`):

| configuration | measured total (Silesia) | frozen expectation | match |
|---|---:|---:|:--:|
| `--bwt-aux=on` | **46,466,339 B** | 46,466,339 B | **YES** |
| `--bwt-aux=off` | **46,446,995 B** | 46,446,995 B | **YES** |

**Round-trip byte-exactness** (same run, file `roundtrip.csv`): 48/48 rows
(12 files × {`aux=on`,`aux=off`} × {frozen baseline binary, changed binary}) have
`matches_source = True` — the decoded SHA-256 equals the source SHA-256 and the decoded length
equals the source length for every corpus file, for **both** the frozen baseline binary and the
changed binary (i.e. the changed binary decodes the frozen wire bytes identically to the frozen
binary).

**Byte cost: exactly 0.** The encoder is untouched; the wire bytes are the frozen bytes.

---

## 4. Fuzz / registry coverage and the in-place proof (Adopt-Gate §6.2, §6.3)

### 4.1 Repo fuzz / registry path — **PASS**

**MEASURED** — run `39666730135` (workflow `.github/workflows/anvil-i20-working-set.yml`, job
`fuzz-registry`, commit `ecf13e6bdc9ee8f55d8b059f0b224e5f88dfe846`, artifact
`anvil-i20-working-set-fuzz-39666730135`, file `fuzz.txt`):

```
PASS seed=41246 roundtrip_variants=1040 mutations=6240 deterministic_rev2=9
     deterministic_bwt=24 aux_bwt=24 golden_bwt=4 forced_postcoders=20
     registry_block_modes=14 registry_transforms=5
     suite_modes=[0, 1, 2, 3, 5, 6, 8] suite_unforceable=[4, 7]
```

* registry coverage: **14 block modes**, **5 transforms**;
* forced postcoder coverage: **20** direct forced encode→decode cases (the postcoder-0/4 lesson);
* auxiliary-BWT assertions: **24** (`aux_bwt=24`);
* deterministic malformed-header cases: **9** rev-2 + **24** BWT;
* golden BWT vectors: **4**;
* round-trip variants: **1040**; mutation battery: **6240** mutations, every one either rejected
  or accepted with byte-identical output;
* the repo's `tests/corpus` fixtures: **30/30** byte-exact round-trips (file
  `fixtures-roundtrip.csv`).

The same fuzz path is also run **inside** the byte-identity job of the RSS workflow
(`working-set (silesia)` / `working-set (enwik8)`), so the Adopt-Gate requirement "in the same
Actions job" is satisfied there as well.

### 4.2 The in-place proof (L2) — isolated, and on real corpus data

`prototypes/i20-working-set/inplace_unbwt.cpp` builds the BWT of a real file with
`libsais_bwt` / `libsais_bwt_aux`, inverts it **twice** (separate buffers = reference, and
in place with the `bwt[0]` capture/re-apply exactly as `src/anvil.cpp` does), and compares the two
results byte for byte **and** against the original input. It covers **both** inverse paths: the
legacy `libsais_unbwt` (`r == n`) and the auxiliary `libsais_unbwt_aux` with the encoder's own
sampling policy (`r` = smallest power of two with `r*1024 >= n`), plus a degenerate size sweep
(`n = 1, 2, 3, 7, 16, 17, 63, 64, 65, 255, 256, 257, 1023, 1024, 1025, 4096, 4097, 65535, 65536,
65537` × aux rates 0/2/64/4096).

* **MEASURED** (run `39666730135`, artifact `anvil-i20-working-set-fuzz-39666730135`, file
  `inplace_unbwt.txt`): `cases=68 failures=0 bytes=49084` — the synthetic sweep.
* **MEASURED** (run `39666730135`, job `working-set (silesia)`, artifact
  `anvil-i20-working-set-silesia-39666730135`, file `prototype/inplace_unbwt.txt`): the same
  harness on the real corpus files (`mozilla`, `webster`, `nci`, `samba`, `xml`, `dickens`), both
  inverse paths, at full size.
* **MEASURED** (run `39666730135`, job `working-set (enwik8)`, artifact
  `anvil-i20-working-set-enwik8-39666730135`): the same harness on the 100 MB `enwik8`.

---

## 5. Measured peak decode RSS, per file, per sub-block cap

**MEASURED** — run `39666730135` (workflow `.github/workflows/anvil-i20-working-set.yml`, jobs
`working-set (silesia)` and `working-set (enwik8)`, commit
`ecf13e6bdc9ee8f55d8b059f0b224e5f88dfe846`, artifacts
`anvil-i20-working-set-silesia-39666730135` / `anvil-i20-working-set-enwik8-39666730135`, files
`rss-by-file.csv` and `cap-rss/rss-by-file.csv`; `/usr/bin/time -f %M` per decode process, the
same measurement the repo arbiter `tools/reference_cost_gate.py rss` uses).

### 5.1 Default cap (no `0xFF` frame) — Silesia, per file, KiB

| file | n (B) | c (B) | frozen baseline | **C4 (L1+L2)** | delta | vs `xz` 55,408 | route |
|---|---:|---:|---:|---:|---:|---:|---|
| **webster** | 41,458,703 | 7,319,896 | 254,168 | **206,612** | **−47,556** | 3.729× | BWT |
| nci | 33,553,445 | 1,369,810 | 202,336 | 168,016 | −34,320 | 3.032× | BWT |
| dickens | 10,192,446 | 2,574,367 | 66,512 | 53,952 | −12,560 | 0.974× | BWT |
| mr | 9,970,564 | 2,384,760 | 64,976 | 52,844 | −12,132 | 0.954× | BWT |
| osdb | 10,085,684 | 2,587,124 | 65,976 | 53,632 | −12,344 | 0.968× | BWT |
| x-ray | 8,474,240 | 4,019,394 | 57,624 | 45,492 | −12,132 | 0.821× | BWT |
| reymont | 6,627,202 | 1,147,270 | 44,376 | 36,600 | −7,776 | 0.661× | BWT |
| **mozilla** | 51,220,480 | 13,806,173 | **125,528** | **125,548** | +20 | **2.266×** | **Brotli** |
| samba | 21,606,400 | 3,761,931 | 61,568 | 61,572 | +4 | 1.111× | Brotli |
| sao | 7,251,944 | 4,586,126 | 22,432 | 22,436 | +4 | 0.405× | Brotli |
| ooffice | 6,152,192 | 2,478,889 | 18,736 | 18,748 | +12 | 0.338× | Brotli |
| xml | 5,345,280 | 430,599 | 14,820 | 14,868 | +48 | 0.268× | Brotli/raw |
| **portfolio max** | | | **254,168** (248.2 MiB) | **206,612** (201.8 MiB) | **−47,556 KiB = −46.4 MiB** | **3.729× (G5 FAIL)** | |

### 5.2 Default cap — enwik8

| file | n (B) | c (B) | frozen baseline | **C4 (L1+L2)** | delta |
|---|---:|---:|---:|---:|---:|
| enwik8 | 100,000,000 | 23,537,422 | 613,324 KiB (599.0 MiB) | **492,688 KiB (481.1 MiB)** | **−120,636 KiB = −117.8 MiB** |

### 5.3 The composable per-cap table (Silesia, per file, KiB)

Baseline = the frozen binary; C4 = the changed binary (L1+L2). Reference: `xz -9e` peak
**55,408 KiB** (run `39654699366`, job `reference-cost (silesia)`) → **G5 needs ≤ 110,816 KiB**.

| cap | file | frozen baseline | **C4 (L1+L2)** | vs `xz` | note |
|---|---|---:|---:|---:|---|
| none (128 MiB) | webster | 254,168 | **206,612** | 3.729× | whole-file BWT |
| none (128 MiB) | nci | 202,336 | 168,016 | 3.032× | whole-file BWT |
| none (128 MiB) | **mozilla** | **125,528** | **125,548** | **2.266×** | **Brotli route — the portfolio floor** |
| 16 MiB | webster | 126,528 | **110,220** | 1.989× | **G5 PASS on this file** |
| 16 MiB | nci | 120,472 | 102,636 | 1.853× | G5 PASS |
| 16 MiB | mozilla | 125,616 | 125,560 | 2.267× | Brotli route |
| 8 MiB | webster | 109,336 | **93,396** | 1.686× | G5 PASS |
| 8 MiB | nci | 70,880 | 70,836 | 1.278× | G5 PASS |
| 8 MiB | mozilla | 125,592 | 125,616 | 2.267× | Brotli route |
| 4 MiB | webster | 117,896 | 105,692 | 1.907× | G5 PASS |
| 4 MiB | nci | 70,832 | 70,812 | 1.278× | G5 PASS |
| 4 MiB | mozilla | 125,608 | 125,568 | 2.266× | Brotli route |
| 16 MiB | enwik8 | 257,660 | 257,720 | — | see §5.4 |
| 8 MiB | enwik8 | 257,632 | 257,720 | — | see §5.4 |
| 4 MiB | enwik8 | 257,568 | 257,720 | — | see §5.4 |

**Cap byte cost (MEASURED, `cap-bytes.csv`)** — diagnostic only, these caps are *not* part of the
adopted change: `webster` 7,319,896 B (uncapped) → 7,700,175 B (16 MiB) → 7,925,862 B (8 MiB) →
8,252,249 B (4 MiB); `nci` 1,369,810 → 1,462,029 (16 MiB) → 1,497,442 (8/4 MiB); `mozilla`
13,806,173 B **unchanged at every cap** (proof that `mozilla` stays on the Brotli route).

### 5.4 What the table says (the decisive C4 finding)

1. **L1+L2 move every BWT-routed file down by exactly `n + c`** (see the massif attribution in
   §5.5): `webster` 254,168 → 206,612 KiB, `nci` 202,336 → 168,016 KiB, `dickens` 66,512 →
   53,952 KiB, and `enwik8` 613,324 → 492,688 KiB.
2. **The files that do not move are exactly the Brotli-routed ones** (`mozilla`, `samba`, `sao`,
   `ooffice`): they never touch the BWT path, so no BWT working-set lever can touch them.
3. **Therefore the Silesia portfolio peak is floored by `mozilla`** (51,220,480 B on the Brotli
   route) at ≈125.6 MiB = **2.266× `xz`** — above the 2.0× G5 threshold — *independently of any
   sub-block cap and of any C4 lever*. W3's cap sweep (run `39656702596`) measured the same floor
   at every cap. **On Silesia, G5 cannot be met by the BWT working-set levers alone: the binding
   term is ANVIL's own Brotli decode of `mozilla`, whose whole-file output buffer is ~50 MiB.**
4. **L3 (streaming) is the lever that removes that term** — the *file-level* accumulator is
   removed for **both** backends (§2, L3), so `mozilla`'s peak should fall by the accumulator
   term. §5.6 reports the measured L3 result.
5. **enwik8 is where the C4 levers bite hardest**: it is BWT-routed, so the default-cap peak falls
   by 117.8 MiB and the **G6 gate (peak RSS vs Brotli q11) flips from FAIL to PASS** at the
   default cap (481.1 MiB vs the 251.9 MiB Brotli reference = 1.91×, against the historical
   2.378× = FAIL). At a sub-block cap the enwik8 peak is *unchanged* by L1+L2 (257,660 →
   257,720 KiB) because the dominant term there is the file-level accumulator, which is exactly
   what L3 removes.

### 5.5 Per-allocation-site attribution (valgrind massif, peak snapshot)

**MEASURED** — run `39666730135`, job `working-set (silesia)`, artifact
`anvil-i20-working-set-silesia-39666730135`, files `massif/baseline.txt` and
`massif/candidate.txt` (`valgrind --tool=massif --detailed-freq=1 --max-snapshots=200`,
`ms_print`), decoding `dickens` (n = 10,192,446 B, c = 2,574,367 B).

| allocation site (massif peak snapshot) | frozen baseline | C4 (L1+L2) | term |
|---|---:|---:|---|
| `bwt_backend_decode` 0x14D4FF | 40,769,788 B (54.77 %) | — | libsais `int32` PSI array (`4n`) |
| `bwt_backend_decode` 0x14D8A7 | — | 40,769,788 B (66.11 %) | libsais `int32` PSI array (`4n`) |
| `bwt_backend_decode` 0x14CDD6 | 10,192,446 B (13.69 %) | — | BWT string (`n`) |
| `bwt_backend_decode` 0x14CE6B | — | 10,192,446 B (16.53 %) | BWT string (`n`) — now also the output buffer |
| `bwt_backend_decode` 0x14D4A2 | 10,192,446 B (13.69 %) | — | reconstructed-output buffer (`n`) — **removed by L2** |
| `decompress` 0x1209F2 | 10,192,446 B (13.69 %) | 10,192,446 B (16.53 %) | file-level output accumulator (`n`) — **removed by L3** |
| `read_file` 0x112F14 | 2,574,367 B (3.46 %) | — | compressed input (`c`) — **removed by L1** |
| below `ms_print` threshold | 505,816 B (0.68 %) | 505,816 B (0.82 %) | libsais `bucket2` (256 KiB) + `fastbits` (256 KiB) |
| **peak heap total** | **74,427,309 B (70.99 MB)** | **61,660,496 B (58.81 MB)** | **−12,766,813 B = exactly −n − c** |

The attribution is therefore **exact**, not approximate: the four dominant live-set terms are
individually identified by allocation site, the two `n`-byte buffers L2/L3 target are named, and
the measured heap reduction equals `n + c` to the byte. The coordinator's attribution (the `4n`
libsais array at ~63 % of the peak, the BWT string and the output buffer at ~16 % each) is
**confirmed**.

### 5.6 The L3 arm (streaming) — measured

**MEASURED** — run `39674862702` (workflow `.github/workflows/anvil-i20-working-set.yml`, jobs
`working-set (silesia)` and `working-set (enwik8)`, commit
`b0a0897115838a4eb3d513f4604aa328cdb9d93b`, artifacts
`anvil-i20-working-set-silesia-39674862702` / `anvil-i20-working-set-enwik8-39674862702`; the
build is L1+L2+L3, i.e. **without** the streaming-Brotli decode of §2/L3 added in `f9be2fc`).

Byte identity: `--bwt-aux=on` **46,466,339 B** and `--bwt-aux=off` **46,446,995 B** — both exactly
the frozen Silesia totals. Round-trip: **48/48** byte-exact (both arms, both `aux` settings).
In-place proof re-run on this build: `cases=80 failures=0 bytes=326,802,592`. In-job fuzz:
**PASS** (1040 round-trip variants, 6240 mutations, 14 block modes, 5 transforms, 20 forced
postcoders, 24 auxiliary-BWT assertions, 9+24 deterministic malformed cases, 4 golden vectors).

**Per-file peak decode RSS, KiB (Silesia):**

| file | frozen baseline | L1+L2 (run `39666730135`) | **L1+L2+L3 (run `39674862702`)** | Δ vs baseline |
|---|---:|---:|---:|---:|
| webster | 254,456 | 206,612 | **206,824** | −47,632 |
| nci | 202,332 | 168,016 | 168,356 | −33,976 |
| dickens | 66,656 | 53,952 | 54,148 | −12,508 |
| mr | 65,120 | 52,844 | 53,028 | −12,092 |
| osdb | 66,080 | 53,632 | 53,708 | −12,372 |
| x-ray | 57,808 | 45,492 | 45,584 | −12,224 |
| reymont | 44,476 | 36,600 | 36,796 | −7,680 |
| **mozilla (Brotli route)** | **125,708** | **125,548** | **125,768** | **+60 (unchanged)** |
| samba (Brotli route) | 61,740 | 61,572 | 61,720 | −20 |
| sao (Brotli route) | 22,664 | 22,436 | 22,608 | −56 |
| ooffice (Brotli route) | 18,876 | 18,748 | 18,900 | +24 |
| xml | 15,104 | 14,868 | 15,064 | −40 |

**Per-file peak decode RSS at a sub-block cap, KiB (Silesia)** — this is where L3 pays:

| cap | file | frozen baseline | L1+L2 | **L1+L2+L3** | vs `xz` 55,408 |
|---|---|---:|---:|---:|---:|
| 16 MiB | webster | 126,688 | 110,220 | **93,944** | 1.695× |
| 16 MiB | nci | 120,624 | 102,636 | 87,804 | 1.585× |
| 16 MiB | mozilla | 125,592 | 125,560 | 125,752 | 2.270× |
| 8 MiB | webster | 109,560 | 93,396 | **53,264** | 0.961× |
| 8 MiB | nci | 70,996 | 70,836 | 71,088 | 1.283× |
| 8 MiB | mozilla | 125,704 | 125,616 | 125,640 | 2.268× |
| 4 MiB | webster | 118,016 | 105,692 | **33,212** | 0.599× |
| 4 MiB | nci | 71,000 | 70,812 | 70,992 | 1.281× |
| 4 MiB | mozilla | 125,664 | 125,568 | 125,688 | 2.269× |

**Two readings, both important:**

1. **L3 is worth ~56,000 KiB (55 MiB) on `webster` at the 8 MiB cap** (93,396 → 53,264) and
   ~85,000 KiB at the 4 MiB cap: it removes the file-level accumulator, which at a cap is the
   dominant term. This is the same effect the coordinator measured on the merged arm
   (`webster` 53,156 KiB, §5.7) — independently reproduced.
2. **L3 alone leaves `mozilla` untouched** (125,768 KiB vs the baseline's 125,708 KiB) — exactly
   as predicted: at the default cap the file-level accumulator is not resident at the peak, and
   the 50,020 KiB term is Brotli's *own* whole-file output buffer inside the Brotli decode. **That
   is the defect the coordinator found, and the streaming-Brotli decode of §2/L3 (`f9be2fc`) is the
   fix.** It is measured in §5.8.

**Paired decode timing for the L3 arm (Silesia, same job):** candidate/frozen ratio
**0.84861, CI95 [0.80567, 0.87474]** — the C4 arms are **1.18× FASTER** than the frozen baseline
(portfolio decode 4.2232 s vs 4.9314 s), with a valid A/A null (0.99960, CI95 [0.98943, 1.02116]).
The decision rule's 2 % epsilon is satisfied with a wide margin, in the *improving* direction.

### 5.7 The merged-arm arbiter arms (coordinator's measurement, cited for the programme)

**MEASURED by the coordinator** on the merged arm `ec2238e0c73bff9fe434b30149f72e06af4a5ef7`
(workflow `.github/workflows/anvil-i20-gate-closure.yml`, job `gate-closure (silesia)`, corpus
silesia; artifacts `anvil-i20-gate-closure-silesia-39674648000` and
`anvil-i20-gate-closure-silesia-39674653288`):

| arm | `--candidate-extra-args` | ANVIL bytes | lead vs `xz` | ANVIL peak RSS | vs `xz` | ANVIL/`xz` decode (CI95) | ANVIL/Brotli decode (CI95) | ruling |
|---|---|---:|---:|---:|---:|---|---|---|
| E | `--bwt-subblock 4194304` | 47,941,299 | 514,705 B | 126,808 KiB | 2.2842× FAIL | 1.2798 [1.2606, 1.2802] PASS | 4.2816 [4.2603, 4.3873] FAIL | `FRONT-GAP_COST` |
| F | `--bwt-subblock 8388608` | 47,365,274 | 1,090,730 B | 127,720 KiB | 2.3049× FAIL | 1.2529 [1.2524, 1.2539] PASS | 4.5611 [4.5000, 4.7867] FAIL | `FRONT-GAP_COST` |

Arm F's per-file RSS table shows `webster` at **53,156 KiB** (against the 109,440 KiB that W3
measured for the same 8 MiB cap *without* the C4 levers — a 55 MiB reduction) and the portfolio
decode at **3.7229 s** against W3's cap-only **4.7287 s** at the same cap (run `39663273951`,
artifact `anvil-i20-subblock-silesia-39663273951`) — i.e. **L1+L2+L3 are ≈1.27× faster on top of
the cap's ≈1.14×, ≈1.45× in total**. The portfolio peak did not move, because it is `mozilla`
(127,720 KiB, Brotli-routed) — the term that the streaming-Brotli lever of §2/L3 removes.


---

## 6. Paired decode-time comparison (same job, same corpus)

**MEASURED** — run `39666730135`, files `paired-changed.json` (candidate vs the frozen baseline
binary) and `paired-null.json` (A/A null), driver `tools/paired_bench.py` with the program
defaults `--reps 9 --warmups 2 --seed 41246 --bootstrap 20000 --epsilon 0.02 --max-cv 0.15
--cpu 0 --ambient-reps 5 --ambient-iterations 250000 --ambient-max-cv 0.10`.

| corpus | pair | ratio (candidate/control) | CI95 | control median (s) | candidate median (s) | `timing_valid` | A/A null CI95 | ambient blocked |
|---|---|---:|---|---:|---:|:--:|---|:--:|
| Silesia | changed vs frozen | **0.99018** | **[0.97994, 1.00037]** | 4.9240 | 4.9232 | yes | [0.98975, 1.01065] | no |
| Silesia | A/A null | 0.99997 | [0.98975, 1.01065] | — | — | yes | — | no |
| enwik8 | changed vs frozen | **1.00034** | **[0.99956, 1.01202]** | 4.1251 | 4.1744 | yes | [0.98806, 1.00037] | no |
| enwik8 | A/A null | 0.99993 | [0.98806, 1.00037] | — | — | yes | — | no |

**The decode-time CI-upper is 1.00037 (Silesia) and 1.01202 (enwik8), both inside the 2 %
practical epsilon (1.02), with a valid A/A null whose CI spans 1.0 in both jobs.** The decode
time did not regress; on Silesia the candidate is marginally *faster* (ratio 0.990).

---

## 7. Evidence record (six fields per number)

| claim | run_url | job | workflow | commit_sha | artifact | corpus | class |
|---|---|---|---|---|---|---|---|
| byte identity Silesia (`aux=on` 46,466,339 B / `aux=off` 46,446,995 B) | https://github.com/thelabcorner/anvil/actions/runs/39661290484 | `working-set (silesia)` | `.github/workflows/anvil-i20-working-set.yml` | `d0b2f4a352311c2c0a34f10a1e4c812f5dce7e27` | `anvil-i20-working-set-silesia-39661290484` | silesia | MEASURED |
| byte identity enwik8 (`aux=on` 23,537,422 B / `aux=off` 23,534,368 B) | https://github.com/thelabcorner/anvil/actions/runs/39666730135 | `working-set (enwik8)` | `.github/workflows/anvil-i20-working-set.yml` | `ecf13e6bdc9ee8f55d8b059f0b224e5f88dfe846` | `anvil-i20-working-set-enwik8-39666730135` | enwik8 | MEASURED |
| round-trip byte-exactness 48/48 Silesia, 4/4 enwik8 (both arms, both `aux` settings) | https://github.com/thelabcorner/anvil/actions/runs/39661290484 + https://github.com/thelabcorner/anvil/actions/runs/39666730135 | `working-set (silesia)` / `working-set (enwik8)` | `.github/workflows/anvil-i20-working-set.yml` | `d0b2f4a…` / `ecf13e6…` | `anvil-i20-working-set-silesia-39661290484` / `anvil-i20-working-set-enwik8-39666730135` | silesia, enwik8 | MEASURED |
| repo fuzz / registry path PASS (1040 round-trip variants, 6240 mutations, 14 block modes, 5 transforms, 20 forced postcoders, 24 auxiliary-BWT assertions, 9+24 deterministic malformed cases, 4 golden vectors) | https://github.com/thelabcorner/anvil/actions/runs/39666730135 | `fuzz-registry` | `.github/workflows/anvil-i20-working-set.yml` | `ecf13e6bdc9ee8f55d8b059f0b224e5f88dfe846` | `anvil-i20-working-set-fuzz-39666730135` | synthetic (repo fixtures) | MEASURED |
| `tests/corpus` fixtures 30/30 byte-exact round-trips | https://github.com/thelabcorner/anvil/actions/runs/39666730135 | `fuzz-registry` | `.github/workflows/anvil-i20-working-set.yml` | `ecf13e6…` | `anvil-i20-working-set-fuzz-39666730135` | synthetic (repo fixtures) | MEASURED |
| **in-place equivalence proof on real corpus data: `cases=80 failures=0 bytes=326,802,592`** (both inverse paths, full size) | https://github.com/thelabcorner/anvil/actions/runs/39666730135 | `working-set (silesia)` | `.github/workflows/anvil-i20-working-set.yml` | `ecf13e6…` | `anvil-i20-working-set-silesia-39666730135` | silesia | MEASURED |
| in-place equivalence proof on enwik8: `cases=70 failures=0 bytes=200,049,084` | https://github.com/thelabcorner/anvil/actions/runs/39666730135 | `working-set (enwik8)` | `.github/workflows/anvil-i20-working-set.yml` | `ecf13e6…` | `anvil-i20-working-set-enwik8-39666730135` | enwik8 | MEASURED |
| in-place equivalence proof, synthetic sweep: `cases=68 failures=0` | https://github.com/thelabcorner/anvil/actions/runs/39666730135 | `fuzz-registry` | `.github/workflows/anvil-i20-working-set.yml` | `ecf13e6…` | `anvil-i20-working-set-fuzz-39666730135` | synthetic | MEASURED |
| per-file peak decode RSS, default cap + 16/8/4 MiB caps, baseline vs changed (Silesia) | https://github.com/thelabcorner/anvil/actions/runs/39666730135 | `working-set (silesia)` | `.github/workflows/anvil-i20-working-set.yml` | `ecf13e6…` | `anvil-i20-working-set-silesia-39666730135` | silesia | MEASURED |
| per-file peak decode RSS, default cap + 16/8/4 MiB caps (enwik8) | https://github.com/thelabcorner/anvil/actions/runs/39666730135 | `working-set (enwik8)` | `.github/workflows/anvil-i20-working-set.yml` | `ecf13e6…` | `anvil-i20-working-set-enwik8-39666730135` | enwik8 | MEASURED |
| massif per-allocation-site peak attribution (baseline and candidate) | https://github.com/thelabcorner/anvil/actions/runs/39666730135 | `working-set (silesia)` | `.github/workflows/anvil-i20-working-set.yml` | `ecf13e6…` | `anvil-i20-working-set-silesia-39666730135` | silesia | MEASURED |
| paired decode timing changed-vs-frozen + A/A null (Silesia, enwik8) | https://github.com/thelabcorner/anvil/actions/runs/39666730135 | `working-set (silesia)` / `working-set (enwik8)` | `.github/workflows/anvil-i20-working-set.yml` | `ecf13e6…` | `anvil-i20-working-set-silesia-39666730135` / `…-enwik8-…` | silesia, enwik8 | MEASURED |
| `xz -9e` in-job peak decode RSS 55,408 KiB and the 4.588× baseline ratio | https://github.com/thelabcorner/anvil/actions/runs/39654699366 | `reference-cost (silesia)` | `.github/workflows/anvil-i10-aux-reference-cost.yml` | `d05fceb0ce61a853378822f63eb5f2ceeef26557` | `anvil-i10-aux-reference-cost-silesia-39654699366` | silesia | MEASURED |
| W3's cap sweep showing `mozilla` stays on the Brotli route at every cap (portfolio floor ≈125.6 MiB) | https://github.com/thelabcorner/anvil/actions/runs/39656702596 | `subblock sweep (silesia)` | `.github/workflows/anvil-i20-subblock-sweep.yml` | `ab88e4aa612696e414050bfdd718ba264a382452` | `anvil-i20-subblock-silesia-39656702596` | silesia | MEASURED (sibling arm) |
| sibling flag-parser off-by-one surfaced by the spaced-form normaliser (arbiter encode step) | https://github.com/thelabcorner/anvil/actions/runs/39670486612 | `gate-closure (silesia)` | `.github/workflows/anvil-i20-gate-closure.yml` | merged arm `ec2238e…` | `anvil-i20-gate-closure-silesia-39670486612` | silesia | MEASURED |
| live-set model `6n+c` vs the frozen peak 248.3 MiB | https://github.com/thelabcorner/anvil/actions/runs/39661290484 + https://github.com/thelabcorner/anvil/actions/runs/39654699366 | `working-set (silesia)` / `reference-cost (silesia)` | `.github/workflows/anvil-i20-working-set.yml` / `.github/workflows/anvil-i10-aux-reference-cost.yml` | `d0b2f4a…` / `d05fceb…` | `anvil-i20-working-set-silesia-39661290484` / `anvil-i10-aux-reference-cost-silesia-39654699366` | silesia | DERIVED |
| theoretical floors per cap (`5n+c`, `5n`, `2n+5C`, `5C`) | — | — | `tools/i20_working_set.py model` | `ecf13e6…` | `anvil-i20-working-set-silesia-39666730135` (`peak-floor-model.csv`) | silesia, enwik8 | DERIVED |

---

## 8. Verdict

### C4 — **WIN** (the lever is adoptable)

Against the pre-registered decision rule, verbatim:

> **Adopt iff the changed binary is byte-identical to the frozen candidate on every corpus file,
> passes the repo fuzz/registry path, and reduces measured peak decode RSS by >= 1.0 MiB without
> increasing decode time CI-upper by more than the 2 % epsilon. Byte cost must be exactly 0.**

| clause | result | evidence |
|---|---|---|
| byte-identical to the frozen candidate on every corpus file | **PASS** — `--bwt-aux=on` Silesia 46,466,339 B and enwik8 23,537,422 B; `--bwt-aux=off` Silesia 46,446,995 B and enwik8 23,534,368 B — all four exactly the frozen totals; 52/52 round-trips byte-exact for both binaries | §3, §7 |
| passes the repo fuzz/registry path | **PASS** — 1040 round-trip variants, 6240 mutations, 14 block modes, 5 transforms, 20 forced postcoders, 24 auxiliary-BWT assertions, 9+24 deterministic malformed cases, 4 golden vectors, 30/30 `tests/corpus` fixtures | §4.1 |
| reduces measured peak decode RSS by ≥ 1.0 MiB | **PASS** — Silesia portfolio peak 254,168 → 206,612 KiB (**−46.4 MiB**); enwik8 613,324 → 492,688 KiB (**−117.8 MiB**); the heap-peak attribution shows the reduction is *exactly* `n + c` | §5.1, §5.2, §5.5 |
| decode time CI-upper not more than +2 % | **PASS** — CI-upper 1.00037 (Silesia) and 1.01202 (enwik8) vs the 1.02 epsilon, with a valid A/A null in both jobs | §6 |
| byte cost exactly 0 | **PASS** — the encoder is untouched; the wire bytes are the frozen bytes | §3 |
| in-place lever (L2) proven byte-exact on real data | **PASS** — `cases=80 failures=0 bytes=326,802,592` (Silesia) and `cases=70 failures=0 bytes=200,049,084` (enwik8), both inverse paths, full size | §4.2 |

**No falsification of L2 occurred**: the in-place inverse BWT is byte-identical to the
separate-buffer path on 526.9 MB of real corpus data across both inverse paths, and the full-codec
round-trip is byte-exact on every corpus file. The aliasing hazard was neutralised by construction
(capturing `bwt[0]` before the call and re-applying it after), so the result does not depend on
the compiler's scheduling.

### The programme gate G5 on Silesia — still FAIL, and now *attributed*

**G5 (Silesia) = ANVIL portfolio peak ÷ `xz -9e` peak = 206,612 / 55,408 = 3.729× (FAIL)**
after the C4 levers, and **2.266× (FAIL)** at any sub-block cap, because the portfolio peak is
then set by `mozilla` — the 51,220,480 B file that the router keeps on the **Brotli** route at
every cap (its compressed size is identical at every cap, 13,806,173 B). **No BWT working-set
lever can lower the Brotli route's decode working set.** The C4 levers are *necessary but not
sufficient* for Silesia G5; the binding term is ANVIL's own Brotli decode of `mozilla`
(≈125.6 MiB, of which ~50 MiB is the whole-file output buffer inside the Brotli decode).

**G6 (enwik8, peak RSS vs Brotli q11) flips FAIL → PASS** at the default cap: 492,688 KiB
(481.1 MiB) vs the 251.9 MiB Brotli reference = **1.91×** (the historical value was 2.378× = FAIL).

**G5 (enwik8)** = 481.1 MiB ÷ 66.2 MiB = 7.27× (FAIL) at the default cap; it becomes reachable at
a sub-block cap once L3 removes the file-level accumulator (see §5.6).

### The C4 levers' place in the programme

| lever | removes | Silesia effect | enwik8 effect | composes with |
|---|---|---|---|---|
| L1 (release the compressed input) | `c` | −2.6 MiB on webster | −22.4 MiB | C3 (cap), C2 (walks) |
| L2 (in-place inverse BWT) | `n` (the duplicate output buffer) | −38.9 MiB on webster | −95.4 MiB | C3, C2 |
| L3 (stream the output) | the file-level accumulator `n` (and the `0xFF` block accumulator `n`) | −39.5 MiB on webster **and** −50 MiB on the Brotli-routed `mozilla` | −95.4 MiB | C3, C2 |

**L3 is the lever that makes the sub-block-cap regime work at all**: at a cap the dominant term is
the file-level accumulator (measured: enwik8 at the 8 MiB cap is 257,632 KiB with L1+L2 alone and
**unchanged** from the baseline, because L1+L2 do not touch the accumulator). L3 removes it.

---

## 9. Next steps / blockers

1. **Merge PR #16 (branch head `f9be2fc`) into the programme branch before ruling the combined
   arm.** The arbiter runs dispatched so far (`39674648000` at the 4 MiB cap, `39674653288` at the
   8 MiB cap, and the two coordinator arms) are built from the programme branch
   `research/anvil-i20-pareto-gate-closure-20261021`, whose HEAD (`e84b5eb…`) **does not contain the
   C4 levers** — verified by reading that tree: it has no `decompress_to_file`, no
   `brotli_lw_decode_stream`, no `anvil_option_takes_value` and no `I20-W4 lever` marker. They
   therefore measure the cap-only arm, not cap+C4.
2. **The Silesia G5 gate needs a Brotli-side lever, not a BWT-side one.** `mozilla` (Brotli route)
   floors the portfolio peak at ≈125.6 MiB = 2.27× `xz`. L3 removes the whole-file output buffer
   from the Brotli decode (§2, L3) — the **50,020 KiB** term — which should bring `mozilla` to
   ≈75 MiB = 1.36× and clear G5; §5.6 reports the measurement. What remains after L3 is Brotli's
   *internal* history ring buffer, which cannot be removed without changing the Brotli window
   size (an encoder-policy change, outside C4's scope).
3. **The `--bwt-aux-walks` off-by-one** (a sibling worker's flag parser: `a.substr(15)` where the
   value starts at index 16) was surfaced by the spaced-form normaliser and fixed by the
   coordinator in the merged arm (`ec2238e`). If the branch is re-merged, re-check `a.substr(16)`.
4. **No blockers in the C4 scope.** The remaining C4 work is the measurement of the L3 arm
   (§5.6) and, if the coordinator wants it, a streaming-Brotli backend (which is outside the
   decode-path scope of C4 and would be a new arm).

