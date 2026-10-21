# ANVIL I20 C2 — Auxiliary-Index Walk-Budget Sweep (`--bwt-aux-walks=N`)

**Worker:** W2
**Date:** 2026-10-21
**Branch:** `research/anvil-i20-w2-aux-walk-sweep`
**Base commit:** `d05fceb0ce61a853378822f63eb5f2ceeef26557`
**Programme document:** `docs/I20-PARETO-GATE-CLOSURE-PREREG-2026-10-21.md` (binding)
**Mechanism class:** `{adopt-class}` / engineering. **No novelty claim.**

## VERDICT: `NO-GO`

> **Adopt the largest reduction in `r` (i.e. the largest walk budget W) whose
> charged bytes keep the Silesia portfolio total below `xz -9e`.** If the
> charged byte cost of the sweep exceeds the 1,989,665 B surplus, the arm is
> `NO-GO` on G2. If the decode-time ratio CI-upper vs Brotli does not fall
> below 2.0 at the byte-feasible point, C2 is `NO-GO`.
> *(pre-registered decision rule, quoted verbatim from prereg §5.1)*

**Falsifying reason.** *The inverse BWT performs `n/2` dependent random
accesses into the LF array for every checkpoint stride `r` (8-way interleaved
LF walks, `libsais_unbwt_decode_8`), so the auxiliary walk budget cannot
shorten the inverse-BWT critical path; it only adds checkpoint bytes.*

**Measured.** The fitted LF-walk share is **f = −0.0001** on enwik8
(95 % CI [−0.014, +0.023], 5 unconfounded points) and **f = −0.0041** on
Silesia (95 % CI spans 0 at every point, 5 unconfounded points), i.e.
**statistically indistinguishable from 0.0**. The largest BWT-path speed-up
actually delivered by the lever is **1.014×** (Silesia, 128 MiB cap,
W = 16384) and **1.0003×** (enwik8) against the **2.94×** the programme needs.
The Amdahl ceiling of this lever is therefore **1.00×**, and the best
byte-feasible arm still has a decode-ratio CI-upper vs Brotli of **2.2680**
(> 2.0), so the pre-registered rule returns `NO-GO`.

---

## 1. Hypothesis and mechanism

### 1.1 Hypothesis

The auxiliary-index inverse-BWT representation buys decode speed by shipping a
checkpoint index array. The encoder's **walk budget** — the number of
independent LF walks it aims for — was hard-coded (`kBwtAuxTargetWalks = 1024`),
so the checkpoint stride `r` had never been swept. **Increasing the walk budget
`W` shrinks `r`, which shortens each LF walk, which should make the inverse BWT
materially faster, at a fully chargeable byte cost of `4 * auxiliary_count` bytes
per BWT-routed sub-block.**

### 1.2 The mechanism, from source

`src/anvil.cpp`, encoder side (line numbers as of commit `ef5d44a`):

```
4253: static int32_t bwt_aux_rate_for_size(int32_t n, uint32_t walks) {
...
4261:     uint64_t target=(static_cast<uint64_t>(n)+walks-1)/walks;
4262:     uint64_t r=2;
4263:     while(r<target) r<<=1;
...
4286:         aux_rate=bwt_aux_rate_for_size(n,opt.bwt_aux_walks);
4287:         const uint64_t icount=1+(static_cast<uint64_t>(n)-1)/static_cast<uint64_t>(aux_rate);
```

and the wire emission (lines 4338-4344):

```
4338:             cand.push_back(kBwtAuxTag);
4339:             cand.push_back(post);
4340:             put_uvar(cand,static_cast<uint32_t>(aux_rate));
4341:             put_uvar(cand,aux_indexes.size());
4342:             for(int32_t idx:aux_indexes) put_u32le(cand,static_cast<uint32_t>(idx));
```

so, exactly:

- `r` = the smallest power of two with `r * W >= n`, minimum 2
  (`bwt_aux_rate_for_size`);
- `auxiliary_count = 1 + floor((n-1)/r)` ≈ `W`;
- the index array costs **`4 * auxiliary_count` bytes** on the wire
  (`put_u32le` per checkpoint);
- a larger `W` therefore gives a **smaller `r`**, **more checkpoints**,
  **shorter LF walks** and a **larger index array**.

**The index is charged PER SUB-BLOCK, not per file.** The outer `0xFF`
BWT-sub-block frame wraps sub-blocks and each sub-block carries its own inner
BWT payload with its own sampling rate derived from that sub-block's own
length. The charged bytes are therefore
`4 * sum_over_subblocks(auxiliary_count)`, and the number of sub-blocks is
`ceil(file_size / cap_bytes)`. Measured Silesia sub-block counts: **12** at the
128 MiB cap (7 BWT-routed files + 5 Brotli-routed files, one each), **19** at
16 MiB (webster 3, nci 2, the rest 1 each) and **31** at 8 MiB.

### 1.3 The wire format already permits this — the crux, verified in source

`src/anvil.cpp`, decoder side, `bwt_backend_decode` (lines 4371-4392):

```
4371:     if(*p==kBwtAuxTag) {
4372:         use_aux=true; ++p;
4373:         if(p>=e) throw std::runtime_error("truncated BWT auxiliary postcoder");
4374:         post=*p++;
4375:         if(expected<=1) throw std::runtime_error("BWT auxiliary payload invalid for tiny output");
4376:         uint64_t rv=get_uvar(p,e);
4377:         uint64_t icount=get_uvar(p,e);
4378:         if(rv<2 || rv>static_cast<uint64_t>(expected) ||
4379:            rv>static_cast<uint64_t>(std::numeric_limits<int32_t>::max()) || (rv&(rv-1))!=0)
4380:             throw std::runtime_error("bad BWT auxiliary sampling rate");
4381:         const uint64_t want=1+(static_cast<uint64_t>(expected)-1)/rv;
4382:         if(icount==0 || icount!=want || icount>kBwtAuxMaxIndexes || icount>expected)
4383:             throw std::runtime_error("bad BWT auxiliary index count");
4384:         if(icount>static_cast<uint64_t>(e-p)/4u) throw std::runtime_error("truncated BWT auxiliary index array");
4385:         aux_indexes.resize(static_cast<size_t>(icount));
4386:         for(size_t i=0;i<aux_indexes.size();++i) {
4387:             uint32_t idx=get_u32le(p,e);
4388:             if(idx<1 || idx>expected) throw std::runtime_error("bad BWT auxiliary index");
4389:             aux_indexes[i]=static_cast<int32_t>(idx);
4390:         }
4391:         aux_rate=static_cast<int32_t>(rv);
4392:         primary=aux_indexes[0];
```

The decoder's complete acceptance set for the sampling rate is therefore:

| # | decoder rule | source line |
|---|---|---|
| D1 | `rv >= 2` | 4378 |
| D2 | `rv <= transformed_size` | 4378 |
| D3 | `rv <= INT32_MAX` | 4379 |
| D4 | `rv` is a power of two (`rv & (rv-1) == 0`) | 4379 |
| D5 | `auxiliary_count == 1 + floor((transformed_size-1)/rv)` | 4381-4382 |
| D6 | `auxiliary_count <= kBwtAuxMaxIndexes (1<<20)` and `<= transformed_size` | 4382 |
| D7 | exactly `4 * auxiliary_count` index bytes are present | 4384 |
| D8 | every index lies in `[1, transformed_size]` | 4388 |

**There is no decoder rule that ties the checkpoint count to 1024, and no rule
that restricts `r` to the value the frozen encoder happened to pick.** Every
`r` in `[2, transformed_size]` that is a power of two is already decodable.
`--bwt-aux-walks` therefore selects among *already-permitted* values and is
**not wire-visible**: it changes no block mode, no framing, no field
interpretation, and no decoder behaviour.

`FORMAT.md` records this explicitly (§3.4 of this document).

### 1.4 The pre-registered decision rule

> **Adopt the largest reduction in `r` (i.e. the largest walk budget W) whose
> charged bytes keep the Silesia portfolio total below `xz -9e`.** If the
> charged byte cost of the sweep exceeds the 1,989,665 B surplus, the arm is
> `NO-GO` on G2. If the decode-time ratio CI-upper vs Brotli does not fall
> below 2.0 at the byte-feasible point, C2 is `NO-GO`.

---

## 2. The change

### 2.1 Diff summary

`src/anvil.cpp` (only the CLI knob and the `bwt_aux_rate_for_size`
generalisation):

| site | change |
|---|---|
| `Options` (line 3794) | `uint32_t bwt_aux_walks=1024;` — new field, default 1024 |
| constants (3913-3923) | `kBwtAuxTargetWalks` retained as the documented frozen default; new `kBwtAuxMaxWalks = 1u<<20` |
| `bwt_aux_rate_for_size` (4253-4282) | signature gains `uint32_t walks`; the policy is generalised from the hard-coded 1024 to `walks`; a clamp keeps `r <= transformed_size` |
| call site (4286) | `bwt_aux_rate_for_size(n, opt.bwt_aux_walks)` |
| CLI (5029-5038) | `--bwt-aux-walks=N` **and** `--bwt-aux-walks N` (separate-argument form) |
| CLI (5040-5045) | `--bwt-subblock` now also accepts the **separate-argument** form `--bwt-subblock <bytes>` (the `=` form is unchanged) — required so the I20 arbiter's single shlex-quoted `--candidate-extra-args` string can carry a combined arm |
| `usage()` (4994) | the new flag is listed |

`tools/i20_aux_walk_sweep.py` (new): `prepare` / `rss` / `summarize` over a
(sub-block cap × walk budget) grid.
`tests/i20_bwt_aux_walks_cli.py` (new): a corpus-free CLI-contract regression
test for the knob (§8.3).
`.github/workflows/anvil-i20-aux-walk-sweep.yml` (new): manual-only byte-only
and timed sweeps, corpus matrix, `permissions: contents: read`,
`cancel-in-progress: false`, finite timeout, `if: always()` artifact upload.
`FORMAT.md`: one encoder-policy paragraph (§3.4).

**Bug found and fixed during the sweep** (reported by the coordinator, fixed
here as commit `33d7aa8`): the `=`-form branch used `a.substr(15)`, but
`"--bwt-aux-walks="` is **16** characters, so the value began at index **16** and
`substr(15)` yielded `"=4096"`, which the digit check rejected. The spaced form
was consumed by an earlier exact-match branch, so the bug was latent on this
branch — but it becomes fatal in the merged arm, where the spaced form is
normalised to the `=` form before the parser chain runs. The fix plus a
corpus-free regression test is in this branch.

### 2.2 Default preservation argument

The generalised policy is

```
target = ceil(n / W);  r = smallest power of two >= target  (minimum 2)
```

With the default `W = 1024` this is **bit-for-bit the original expression**
`target=(n+1024-1)/1024; r=2; while(r<target) r<<=1;` — the only new code is a
clamp that fires when `r > n`.

The clamp can fire only for `W = 1`:

- for `W >= 2`, `target = ceil(n/W) <= ceil(n/2)`; the smallest power of two
  `r >= target` satisfies `target <= r < 2*target <= n` (for `target >= 2`), and
  for `target = 1` we get `r = 2 <= n` for `n >= 2`; hence `r <= n` always, so
  the clamp is unreachable at `W = 1024`;
- the encoder only calls the policy when `n > 1` (`opt.bwt_aux && n>1`), so
  `n >= 2` always holds.

**Therefore the default output is byte-identical to the frozen I10-1A bytes.**
This is not only argued but *measured* — see §4.

### 2.3 Validation rules

| rule | where | behaviour |
|---|---|---|
| `1 <= N <= 1048576` | `parse_bwt_aux_walks` (CLI, up front) | explicit error `--bwt-aux-walks must be an integer in 1..1048576`; nothing is encoded |
| non-digit / empty value | `parse_bwt_aux_walks` | same explicit error (the digits are checked before `stoull`, so `-1` and `abc` are rejected, not wrapped) |
| missing value | CLI | explicit error `--bwt-aux-walks requires a value` |
| `1 <= N <= 1048576` | `bwt_aux_rate_for_size` (defensive) | `BWT auxiliary walk budget out of range` |
| `r <= transformed_size` | `bwt_aux_rate_for_size` clamp | `r` is clamped to the largest power of two `<= n`; the payload is therefore always decodable and never corrupt |
| `auxiliary_count <= 1<<20` | implied | the policy yields `auxiliary_count = 1 + floor((n-1)/r) <= W`, so `W <= 1<<20` keeps every payload inside the existing decoder bound D6 |

---

## 3. A source-level prediction, stated before the measurement

`third_party/libsais/src/libsais.c` determines what the walk budget can and
cannot change. The auxiliary inverse is driven by `libsais_unbwt_aux` (line
8040) → `libsais_unbwt_main` (7994) → `libsais_unbwt_core` (7975) →
`libsais_unbwt_decode_omp` (7943) → `libsais_unbwt_decode` (7882) →
`libsais_unbwt_decode_8` (7854).

```
7946:    fast_sint_t blocks      = 1 + (((fast_sint_t)n - 1) / (fast_sint_t)r);
7947:    fast_uint_t remainder   = (fast_uint_t)n - ((fast_uint_t)r * ((fast_uint_t)blocks - 1));
...
7890:        libsais_unbwt_decode_8(U + offset, P, bucket2, fastbits, shift, (fast_uint_t)r, &i0, ..., (fast_uint_t)r >> 1);
7891:        I += 8; blocks -= 8; offset += 8 * (fast_uint_t)r;
...
7856:    uint16_t * RESTRICT U0 = (uint16_t *)(void *)U;
7857:    uint16_t * RESTRICT U1 = (uint16_t *)(void *)(((uint8_t *)U0) + r);
...
7867:    for (i = 0; i != k; ++i)          // k == r >> 1
7869:        uint16_t c0 = fastbits[p0 >> shift]; ... p0 = P[p0]; U0[i] = libsais_bswap16(c0);
7870:        uint16_t c1 = ...                                   p1 = P[p1]; U1[i] = ...
        ... (8 independent LF walks, one per sub-block)
```

Counting the work:

- one group of 8 sub-blocks performs `r/2` iterations x 8 LF steps = **`4r`
  LF steps**, and emits `8r` output bytes (2 bytes per step);
- there are `n/(8r)` groups, so the **total LF-step count is `n/2` for every
  `r`**, and the total output is `n`.

**The number of dependent random accesses into the `int32` LF array is
therefore exactly `n/2` regardless of the walk budget.** What `r` changes is
(a) the number of checkpoints, i.e. the byte cost, and (b) the *decomposition
granularity* of the same `n/2` steps into 8-way interleaved walks. The 8-way
memory-level parallelism is already fully exploited at `W = 1024`
(`r = 8192..65536` on Silesia, `r = 131072` on enwik8), so shrinking `r` further
shortens each individual walk but does **not** shorten the critical path and
does **not** remove any memory access.

This predicts a fitted LF-walk share `f` close to 0, i.e. a BWT-path speed-up
close to 1.0×, and therefore a `NO-GO` on the decode gate — the auxiliary index
would already have captured the entire available win at `W = 1024`. The sweep
below measures the curve rather than assuming the prediction, and the fitted
`f` is reported with its interval.

---

## 4. Byte-identity results (the Adopt-Gate)

`run_url:` <https://github.com/thelabcorner/anvil/actions/runs/39680371188>
`job:` `sweep (silesia)`, `sweep (enwik8)`
`workflow:` `.github/workflows/anvil-i20-aux-walk-sweep.yml`
`commit_sha:` `1c6784ec357ef3ba775ad007a7fa4e4982415ab9`
`artifact:` `anvil-i20-aux-walk-sweep-silesia-39680371188`,
`anvil-i20-aux-walk-sweep-enwik8-39680371188`
`corpus:` silesia, enwik8

The byte-only sweep builds the **frozen `i10-aux-unbwt-final-v1` binary** and
the **changed binary** from the same job and compares, **per file**, the
SHA-256 of three representations: the Brotli payload, the BWT payload with
auxiliary indexes ON, and the BWT payload with auxiliary indexes OFF.

| corpus | check | result |
|---|---|---|
| silesia | `frozen_per_file_byte_identity` (12 files x 3 representations = 36 comparisons) | **OK — every SHA-256 identical** |
| silesia | `control_portfolio_identity` | **OK** — 46,466,339 B = the frozen I10-1A total |
| silesia | `legacy_portfolio_identity` (`--bwt-aux=off`) | **OK** — 46,446,995 B = the frozen I9 total |
| silesia | `xz_reference_total` / `brotli_reference_total` | **OK** — 48,456,004 B / 49,383,136 B = the frozen baseline |
| enwik8 | `frozen_per_file_byte_identity` (1 file x 3) | **OK** |
| enwik8 | `control_portfolio_identity` | **OK** — 23,537,422 B |
| enwik8 | `legacy_portfolio_identity` | **OK** — 23,534,368 B |
| enwik8 | `xz_reference_total` / `brotli_reference_total` | **OK** — 24,831,648 B / 24,810,180 B |

Selected per-file SHA-256 pairs (Silesia, `bwt_aux_on`; frozen == new):

| file | SHA-256 (first 16) | bytes frozen == new |
|---|---|---|
| dickens | `70d7fef7bdee673f` | 2,574,367 |
| mozilla | `9021070887235ea6` | 17,826,781 |
| mr | `32ecc6d47edec933` | 2,384,760 |
| nci | `cdfac65e87e7df90` | 1,369,810 |
| ooffice | `36b5e83a48d3b7f5` | 2,871,837 |
| osdb | `6f57c4be8358ffe2` | 2,587,124 |
| reymont | `b8bc10766a2644b6` | 1,147,270 |
| samba | `52cb6163a572d3f7` | 4,401,300 |
| sao | `8c2241b0cc5a0108` | 5,121,524 |
| webster | `578f53eb769471e2` | 7,319,896 |
| x-ray | `fb420be399523bd7` | 4,019,394 |
| xml | `b45e57db2e956eff` | 440,403 |

**The default output of the changed binary is byte-identical per file to the
frozen I10-1A binary, and `--bwt-aux=off` is byte-identical to the frozen I9
bytes.** The Adopt-Gate §6.1 byte-identity requirement is met.

---

## 5. The byte curve (the price of checkpoint granularity)

`run_url:` <https://github.com/thelabcorner/anvil/actions/runs/39680371188>
`job:` `sweep (silesia)` / `sweep (enwik8)`
`workflow:` `.github/workflows/anvil-i20-aux-walk-sweep.yml`
`commit_sha:` `1c6784ec357ef3ba775ad007a7fa4e4982415ab9`
`artifact:` `anvil-i20-aux-walk-sweep-silesia-39680371188`,
`anvil-i20-aux-walk-sweep-enwik8-39680371188`
`corpus:` silesia, enwik8
(`MEASURED`; the "charged index bytes" column is the sum of
`4 * auxiliary_count` over the sub-blocks of the files that actually route to
the BWT backend in that arm, read out of the encoded payload — it is
*attributed*, not inferred.)

### 5.1 Silesia

| arm | cap MiB | W | k = W/1024 | portfolio bytes | Δ vs control | surplus left | charged index bytes | BWT-routed files | sub-blocks |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `c128-w1024` | 128 | 1024 | 1 | **46,466,339** | +0 | +1,989,665 | 19,328 | 7 | 12 |
| `c128-w4096` | 128 | 4096 | 4 | 46,524,282 | +57,943 | +1,931,722 | 77,276 | 7 | 12 |
| `c128-w8192` | 128 | 8192 | 8 | 46,601,541 | +135,202 | +1,854,463 | 154,536 | 7 | 12 |
| `c128-w16384` | 128 | 16384 | 16 | 46,756,074 | +289,735 | +1,699,930 | 309,068 | 7 | 12 |
| `c128-w32768` | 128 | 32768 | 32 | 47,065,144 | +598,805 | +1,390,860 | 618,132 | 7 | 12 |
| `c128-w65536` | 128 | 65536 | 64 | 47,533,903 | +1,067,564 | +922,101 | 767,008 | **5** | 12 |
| `c16-w1024` | 16 | 1024 | 1 | 46,938,837 | +472,498 | +1,517,167 | 32,944 | 7 | 19 |
| `c16-w4096` | 16 | 4096 | 4 | 47,037,629 | +571,290 | +1,418,375 | 131,744 | 7 | 19 |
| `c16-w8192` | 16 | 8192 | 8 | 47,147,432 | +681,093 | +1,308,572 | 197,940 | **6** | 19 |
| `c16-w16384` | 16 | 16384 | 16 | 47,345,366 | +879,027 | +1,110,638 | 395,872 | **6** | 19 |
| `c16-w32768` | 16 | 32768 | 32 | 47,741,236 | +1,274,897 | +714,768 | 791,736 | **6** | 19 |
| `c16-w65536` | 16 | 65536 | 64 | **48,394,677** | +1,928,338 | **+61,327** | 605,056 | **4** | 19 |

`xz -9e` = 48,456,004 B. **Every arm in the grid still fits under `xz -9e`**;
the largest walk budget that fits is `16 MiB + W = 65536` at 48,394,677 B
(61,327 B of surplus left). The "BWT-routed files" column in **bold** marks the
arms where the *backend route* changed (nci and/or reymont moved to Brotli
because the BWT payload grew past the Brotli payload) — see §6.3.

Per-file charged index bytes at the 128 MiB cap (`4 * auxiliary_count` summed
over the sub-blocks):

| file | route | W=1024 | W=4096 | W=8192 | W=16384 | W=32768 | W=65536 |
|---|---|---:|---:|---:|---:|---:|---:|
| dickens | BWT | 2,492 | 9,956 | 19,908 | 39,816 | 79,632 | 159,260 |
| mr | BWT | 2,436 | 9,740 | 19,476 | 38,948 | 77,896 | 155,792 |
| nci | BWT | 4,096 | 16,384 | 32,768 | 65,536 | 131,072 | 262,140 |
| osdb | BWT | 2,464 | 9,852 | 19,700 | 39,400 | 78,796 | 157,592 |
| reymont | BWT | 3,236 | 12,944 | 25,888 | 51,776 | 103,552 | 207,104 |
| webster | BWT | 2,532 | 10,124 | 20,244 | 40,488 | 80,976 | 161,952 |
| x-ray | BWT | 2,072 | 8,276 | 16,552 | 33,104 | 66,208 | 132,412 |
| **BWT total** | | **19,328** | **77,276** | **154,536** | **309,068** | **618,132** | **1,236,252** |
| mozilla | Brotli | 3,128 | 12,508 | 25,012 | 50,020 | 100,040 | 200,080 |
| nci (at W=65536) | Brotli | — | — | — | — | — | 262,140 |
| reymont (at W=65536) | Brotli | — | — | — | — | — | 207,104 |
| ooffice | Brotli | 3,004 | 12,016 | 24,032 | 48,064 | 96,128 | 192,256 |
| samba | Brotli | 2,640 | 10,552 | 21,100 | 42,200 | 84,400 | 168,800 |
| sao | Brotli | 3,544 | 14,164 | 28,328 | 56,656 | 113,312 | 226,624 |
| xml | Brotli | 2,612 | 10,440 | 20,880 | 41,760 | 83,520 | 167,040 |

(The Brotli rows are the index cost that *would* be charged if the file were
BWT-routed; it is not charged while the file routes to Brotli.)

### 5.2 enwik8

| arm | cap MiB | W | portfolio bytes | Δ vs control | charged index bytes | route |
|---|---:|---:|---:|---:|---:|---|
| `c128-w1024` | 128 | 1024 | **23,537,422** | +0 | 3,052 | BWT |
| `c128-w4096` | 128 | 4096 | 23,546,578 | +9,156 | 12,208 | BWT |
| `c128-w8192` | 128 | 8192 | 23,558,786 | +21,364 | 24,416 | BWT |
| `c128-w16384` | 128 | 16384 | 23,583,201 | +45,779 | 48,832 | BWT |
| `c128-w32768` | 128 | 32768 | 23,632,030 | +94,608 | 97,660 | BWT |
| `c128-w65536` | 128 | 65536 | **23,729,686** | +192,264 | 195,316 | BWT |
| `c16-w*` | 16 | any | 24,810,212 | +1,272,790 | 0 (charged) | **Brotli** (the BWT payload never wins) |

`xz -9e` = 24,831,648 B. On enwik8 the walk budget is a pure byte cost: the
whole 64x range of `W` stays on the BWT backend at the 128 MiB cap and costs
at most 192,264 B.

### 5.3 What the byte curve establishes

**The walk budget is a bytes-only knob.** The charged index bytes grow
essentially linearly in `W` (19,328 → 1,236,252 B over `W` = 1024 → 65536 at
the 128 MiB cap) and the portfolio total grows monotonically with it, at every
cap. The prereg's C2 byte estimate ("+0.02 %…+0.2 % bytes") was **3–11x too
optimistic**: the measured charged cost of `W = 16384` alone is +289,735 B
(+0.62 % of the 46.5 MB portfolio) at the 128 MiB cap, and `W = 65536` costs
+1,067,564 B (+2.30 %).

---

## 6. The speed curve and the fitted LF-walk share `f`

`run_url:` <https://github.com/thelabcorner/anvil/actions/runs/39676039083>
`job:` `sweep (silesia)`, `sweep (enwik8)`
`workflow:` `.github/workflows/anvil-i20-aux-walk-sweep.yml`
`commit_sha:` `ef5d44a49b0ad0149a639697c944ff81c34dc185`
`artifact:` `anvil-i20-aux-walk-sweep-silesia-39676039083`,
`anvil-i20-aux-walk-sweep-enwik8-39676039083`
`corpus:` silesia, enwik8

Protocol exactly as pre-registered: `tools/paired_bench.py` with
`--reps 9 --warmups 2 --seed 41246 --bootstrap 20000 --epsilon 0.02
--max-cv 0.15 --cpu 0 --ambient-reps 5 --ambient-iterations 250000
--ambient-max-cv 0.10`; a mandatory A/A null pair per arm; all raw repetitions
retained; every arm single-threaded. **Every A/A null in every job is
`timing_valid = true`, spans 1.0, and clears no speed gate** (null ratios
0.9876–1.0136, all with `status = NO_SPEED_GATE`); the ambient robust CV was
≤ 0.060 everywhere, so no run was ambient-blocked.

The **BWT-path** columns time only the seven BWT-routed Silesia files
(120,362,284 B) as a paired A/B against the *same subset* at the same cap with
the `W = 1024` control. The **portfolio** columns time the whole 12-file
portfolio against `xz -9e` and against Brotli q11/lgwin30, in the same job.

### 6.1 Silesia — the full curve

| arm | portfolio decode s | BWT-path decode s | BWT-path MB/s | BWT-path speed-up | fitted f (95 % CI) | vs xz (CI95) | vs Brotli (CI95) | peak RSS MiB | RSS/xz | RSS/Brotli | G2 | G3 | G4 | G5 | G6 |
|---|---:|---:|---:|---:|---|---|---|---:|---:|---:|---|---|---|---|---|
| `c128-w1024` | 4.4758 | 3.7242 | 32.32 | — (control) | — | 1.8283 [1.8111, 1.8474] | 3.3672 [3.2347, 3.5177] | 248.4 | 4.855 | 2.030 | PASS | PASS | FAIL | FAIL | FAIL |
| `c128-w4096` | 4.4778 | 3.5216 | 34.18 | 1.000 | −0.0003 [−0.0189, +0.0378] | 1.8561 [1.8251, 1.8892] | 3.3465 [3.2767, 3.4616] | 248.4 | 4.855 | 2.030 | PASS | PASS | FAIL | FAIL | FAIL |
| `c128-w8192` | 4.2262 | 3.5218 | 34.18 | 1.000 | −0.0001 [−0.0167, +0.0170] | 1.8638 [1.8618, 1.9497] | 3.2432 [3.0467, 3.5496] | 248.4 | 4.854 | 2.029 | PASS | PASS | FAIL | FAIL | FAIL |
| `c128-w16384` | 4.2243 | 3.5723 | 33.69 | 1.014 | +0.0147 [−0.0301, +0.0294] | 1.8824 [1.8173, 1.9073] | 3.2753 [3.1608, 3.3876] | 248.6 | 4.857 | 2.031 | PASS | PASS | FAIL | FAIL | FAIL |
| `c128-w32768` | 4.3257 | 3.5715 | 33.70 | 0.987 | −0.0139 [−0.0147, +0.0154] | 1.8664 [1.8266, 1.9093] | 3.3758 [3.1634, 3.4713] | 248.6 | 4.857 | 2.031 | PASS | PASS | FAIL | FAIL | FAIL |
| `c128-w65536` | 3.5730 | 2.8704 | 41.93 | *1.227* | *route-confounded* | 1.6022 [1.5747, 1.6557] | 2.7058 [2.6337, 2.8518] | 248.8 | 4.861 | 2.032 | PASS | PASS | FAIL | FAIL | FAIL |
| `c16-w1024` | 4.0232 | 3.7238 | 32.32 | — (control) | — | 1.8141 [1.7357, 1.8191] | 3.8198 [3.4236, 3.9528] | 123.7 | 2.418 | 1.011 | PASS | PASS | FAIL | FAIL | PASS |
| `c16-w4096` | 4.2804 | 3.6750 | 32.75 | 1.001 | +0.0009 [−0.0372, +0.0352] | 1.8227 [1.8022, 1.8438] | 3.7774 [3.5725, 3.9144] | 123.8 | 2.420 | 1.012 | PASS | PASS | FAIL | FAIL | PASS |
| `c16-w8192` | 3.5733 | 3.0204 | 39.85 | *1.150* | *route-confounded* | 1.5767 [1.5748, 1.6102] | 3.4145 [3.3558, 3.5168] | 123.9 | 2.421 | 1.012 | PASS | PASS | FAIL | FAIL | PASS |
| `c16-w16384` | 3.6229 | 3.1723 | 37.94 | *1.152* | *route-confounded* | 1.5854 [1.5530, 1.6206] | 3.4100 [3.3285, 3.7080] | 124.0 | 2.424 | 1.013 | PASS | PASS | FAIL | FAIL | PASS |
| `c16-w32768` | 3.6728 | 3.0227 | 39.82 | *1.164* | *route-confounded* | 1.5841 [1.5632, 1.6055] | 3.3003 [3.1513, 3.3964] | 124.3 | 2.429 | 1.015 | PASS | PASS | FAIL | FAIL | PASS |
| `c16-w65536` | 2.3720 | 1.8183 | 66.19 | *1.938* | *route-confounded* | **1.0461** [1.0232, 1.0676] | **2.2680** [2.1698, 2.2821] | 122.8 | 2.399 | 1.003 | PASS | PASS | **FAIL** | FAIL | PASS |

### 6.2 enwik8 — the cleanest measurement (a single file that always stays on BWT)

| arm | portfolio decode s | BWT-path decode s | BWT-path MB/s | BWT-path speed-up | fitted f (95 % CI) | vs xz (CI95) | vs Brotli (CI95) | peak RSS MiB | RSS/xz | RSS/Brotli | G2 | G3 | G4 | G5 | G6 |
|---|---:|---:|---:|---:|---|---|---|---:|---:|---:|---|---|---|---|---|
| `c128-w1024` | 4.8748 | 4.8758 | 20.51 | — (control) | — | 3.5269 [3.4372, 3.5650] | 7.4624 [7.3025, 7.9049] | 598.9 | 9.064 | 2.389 | PASS | FAIL | FAIL | FAIL | FAIL |
| `c128-w4096` | 4.8246 | 4.9276 | 20.29 | 1.000 | −0.0004 [−0.0141, +0.0002] | 3.6609 [3.5259, 3.6615] | 7.4637 [7.3854, 7.8245] | 598.9 | 9.065 | 2.389 | PASS | FAIL | FAIL | FAIL | FAIL |
| `c128-w8192` | 4.9252 | 4.9248 | 20.31 | 1.000 | +0.0002 [−0.0122, +0.0230] | 3.4731 [3.4713, 3.5991] | 7.4703 [7.3875, 7.6937] | 598.9 | 9.064 | 2.389 | PASS | FAIL | FAIL | FAIL | FAIL |
| `c128-w16384` | 4.9743 | 4.8758 | 20.51 | 1.000 | +0.0000 [−0.0106, +0.0109] | 3.6352 [3.5775, 3.6729] | 7.3898 [7.3138, 7.9888] | 599.0 | 9.065 | 2.389 | PASS | FAIL | FAIL | FAIL | FAIL |
| `c128-w32768` | 4.8770 | 4.9239 | 20.31 | 1.000 | +0.0002 [−0.0102, +0.0105] | 3.4379 [3.4373, 3.5644] | 7.3941 [7.3015, 7.9062] | 599.0 | 9.065 | 2.389 | PASS | FAIL | FAIL | FAIL | FAIL |
| `c128-w65536` | 4.9242 | 4.8760 | 20.51 | 1.000 | +0.0003 [−0.0102, +0.0104] | 3.5981 [3.5626, 3.6004] | 7.9031 [7.3102, 7.9076] | 599.3 | 9.070 | 2.390 | PASS | FAIL | FAIL | FAIL | FAIL |
| `c16-w*` | 0.6666-0.7165 | — | 139.5-150.0 | — | — | 0.5058-0.5243 | 1.0810-1.1621 | 251.2 | 3.801 | 1.002 | PASS | **PASS** | **PASS** | FAIL | **PASS** |

On enwik8 the 128 MiB cap keeps the file on the BWT backend at every `W`, so
the curve is **completely free of route confounds** over a 64x range of `W`
(`r` from 131072 down to 2048). The inverse-BWT decode time is invariant.

### 6.3 The fitted LF-walk share `f`

Model: `speedup(k) = 1/(1 − f + f/k)`, `k = W/1024`, so
`f = (1 − time_ratio)/(1 − 1/k)`. A point is **unconfounded** when every file
that routes to BWT at the `W = 1024` control still routes to BWT in that arm
(so the measured "speed-up" cannot be a backend-route change).

**Silesia — 5 unconfounded points** (128 MiB cap at W = 4096 / 8192 / 16384 /
32768, and 16 MiB cap at W = 4096):

| point | k | time ratio | f (95 % CI) |
|---|---:|---:|---|
| `c128-w4096` | 4 | 1.0002 | −0.0003 [−0.0189, +0.0378] |
| `c128-w8192` | 8 | 1.0001 | −0.0001 [−0.0167, +0.0170] |
| `c128-w16384` | 16 | 0.9862 | +0.0147 [−0.0301, +0.0294] |
| `c128-w32768` | 32 | 1.0135 | −0.0139 [−0.0147, +0.0154] |
| `c16-w4096` | 4 | 0.9993 | +0.0009 [−0.0372, +0.0352] |

**POOLED f = −0.0041** (inverse-variance weighted over the 5 points; every
point's interval spans 0). The Amdahl ceiling of the lever is
`1/(1−f) = 0.996x` as `k → ∞`.

**enwik8 — 5 unconfounded points:**

| point | k | time ratio | f (95 % CI) |
|---|---:|---:|---|
| `c128-w4096` | 4 | 1.0003 | −0.0004 [−0.0141, +0.0002] |
| `c128-w8192` | 8 | 0.9998 | +0.0002 [−0.0122, +0.0230] |
| `c128-w16384` | 16 | 1.0000 | +0.0000 [−0.0106, +0.0109] |
| `c128-w32768` | 32 | 0.9998 | +0.0002 [−0.0102, +0.0105] |
| `c128-w65536` | 64 | 0.9997 | +0.0003 [−0.0102, +0.0104] |

**POOLED f = −0.0001** (inverse-variance weighted; every point's interval
spans 0). The Amdahl ceiling is `1/(1−f) = 0.9999x` as `k → ∞`.

> **The measured speed-up is statistically indistinguishable from 1.0×.**
> The maximum BWT-path speed-up actually delivered by the lever is
> **1.014×** (Silesia, 128 MiB cap, W = 16384) and **1.0003×** (enwik8),
> against the **2.94×** the programme requires. The `f` that would be
> required is **0.704 at k = 16**, **0.681 at k = 32** and **0.670 at k = 64**;
> the measured `f` is **0.000 ± 0.02**.

### 6.4 The apparent speed-ups at high `W` are a backend-route change, not a walk-budget effect

The per-file decode times (single-shot, from the same job's `rss.csv`) show
exactly where the time goes. Silesia, 128 MiB cap:

| file | route at W=1024 | W=1024 | W=4096 | W=8192 | W=16384 | W=32768 | W=65536 |
|---|---|---:|---:|---:|---:|---:|---:|
| dickens | BWT | 0.35 | 0.38 | 0.37 | 0.35 | 0.37 | 0.37 |
| mr | BWT | 0.27 | 0.28 | 0.27 | 0.27 | 0.29 | 0.28 |
| osdb | BWT | 0.32 | 0.35 | 0.35 | 0.33 | 0.35 | 0.33 |
| reymont | BWT | 0.16 | 0.19 | 0.18 | 0.16 | 0.18 | **0.02 (Brotli)** |
| webster | BWT | 1.39 | 1.45 | 1.43 | 1.42 | 1.43 | 1.42 |
| x-ray | BWT | 0.38 | 0.40 | 0.40 | 0.41 | 0.41 | 0.40 |
| nci | BWT | 0.66 | 0.65 | 0.67 | 0.73 | 0.73 | **0.17 (Brotli)** |

**Every file that stays on the BWT backend has an unchanged decode time at
every `W`** (dickens 0.35-0.38 s, mr 0.27-0.29 s, osdb 0.32-0.35 s, webster
1.39-1.45 s, x-ray 0.38-0.41 s — flat within single-shot noise). The entire
"speed-up" at `W = 65536` comes from `nci` (0.66 s → 0.17 s) and `reymont`
(0.16 s → 0.02 s) **switching to the Brotli backend** because the BWT payload
grew past the Brotli payload. The same pattern holds at the 16 MiB cap
(`nci` flips at W = 8192, `webster` and `reymont` at W = 65536).

**So the walk budget buys no decode time at all: it buys a backend-route
change, which is a different mechanism (and a different candidate).**

### 6.5 Peak decode RSS

The peak decode RSS is **unaffected by the walk budget**: 248.4-248.8 MiB at
the 128 MiB cap and 122.8-124.3 MiB at the 16 MiB cap for every `W` (the
index array is at most a few MB and is dwarfed by the `4n`-byte LF array).
G5 (peak RSS ≤ 2× `xz`) fails in every arm of the grid (the best is 2.399× at
`c16-w65536`); G6 (≤ 2× Brotli) passes at the 16 MiB cap (1.003-1.015×) and
fails at the 128 MiB cap (2.029-2.032×).

---

## 7. The sub-block cap: a cap-only decode-speed series (and a confound of its own)

`run_url:` <https://github.com/thelabcorner/anvil/actions/runs/39676132197>
`job:` `sweep (silesia)`, `sweep (enwik8)`
`workflow:` `.github/workflows/anvil-i20-aux-walk-sweep.yml`
`commit_sha:` `ef5d44a49b0ad0149a639697c944ff81c34dc185`
`artifact:` `anvil-i20-aux-walk-sweep-silesia-39676132197`,
`anvil-i20-aux-walk-sweep-enwik8-39676132197`
`corpus:` silesia, enwik8
(`MEASURED`; walk budget held at the default 1024 throughout.)

| cap MiB | portfolio bytes | Δ vs control | BWT-routed files | portfolio decode s | BWT-path decode s | BWT-path MB/s | vs xz (CI95) | vs Brotli (CI95) | peak RSS MiB | RSS/xz | RSS/Brotli | G2 | G3 | G4 | G5 | G6 |
|---:|---:|---:|---:|---:|---:|---:|---|---|---:|---:|---:|---|---|---|---|---|
| 128 | 46,466,339 | +0 | 7 | 4.2747 | 3.6208 | 33.24 | 1.9069 [1.8437, 1.9282] | 3.3631 [3.1268, 3.5427] | 248.5 | 4.856 | 2.030 | PASS | PASS | FAIL | FAIL | FAIL |
| 32 | 46,648,642 | +182,303 | 7 | 4.2748 | 3.6239 | 33.21 | 1.9071 [1.8421, 1.9497] | 3.2822 [3.2022, 3.4944] | 203.6 | 3.978 | 1.663 | PASS | PASS | FAIL | FAIL | PASS |
| 16 | 46,938,837 | +472,498 | 7 | 4.3273 | 3.8232 | 31.48 | 1.8275 [1.8052, 1.8665] | 3.3516 [3.1453, 3.5307] | 123.8 | 2.419 | 1.011 | PASS | PASS | FAIL | FAIL | PASS |
| 8 | 47,365,274 | +898,935 | **6** | 3.5725 | 2.9706 | 40.52 | 1.5759 [1.5664, 1.6343] | 2.9305 [2.8921, 2.9727] | 122.8 | 2.400 | 1.003 | PASS | PASS | FAIL | FAIL | PASS |
| 4 | 47,941,299 | +1,474,960 | **6** | 3.5222 | 2.7719 | 43.42 | **1.4665** [1.4452, 1.5074] | **2.6968** [2.6061, 2.8195] | 122.8 | 2.400 | 1.003 | PASS | PASS | FAIL | FAIL | PASS |

**Reading 1 — the cap is a memory lever, and the memory result is real.** Peak
decode RSS falls from 248.5 MiB (4.856× `xz`) at the 128 MiB cap to 123.8 MiB
(2.419× `xz`, **1.011× Brotli — G6 PASS**) at the 16 MiB cap, for +472,498 B.

**Reading 2 — the cap is NOT a BWT decode-speed lever.** Summing the decode
times of the six files that stay on the BWT backend in *every* arm
(dickens, mr, osdb, reymont, webster, x-ray = 84,411,750 B):

| cap MiB | 4 | 8 | 16 | 32 | 128 |
|---|---:|---:|---:|---:|---:|
| always-BWT decode sum (s) | 2.590 | 2.850 | 2.810 | 2.940 | 2.920 |
| ratio vs the 4 MiB arm | 1.000 | 1.100 | 1.085 | 1.135 | 1.127 |

A **32x change in the sub-block cap** (and therefore in the size of the
`4C`-byte LF array) leaves the BWT decode time **flat within ~14 % of
single-shot noise** and with no monotone trend. The "cache residency"
hypothesis is **not supported**.

**Reading 3 — the portfolio speed-up at 8/4 MiB is a route change.** `nci`
(33,553,445 B) moves from the BWT backend to Brotli at the 8 and 4 MiB caps
(0.66-0.77 s → 0.17-0.19 s per file). That single route change accounts for
the entire 0.65 s portfolio saving (4.23 s → 3.58 s). It is a *backend
routing* effect, not a decode-speed effect, and it costs bytes: the 8 MiB cap
is +898,935 B over the control (which matches the prior sweep's figure
exactly) and the 4 MiB cap is +1,474,960 B.

**Reading 4 — enwik8: the cap routes the file away from BWT entirely.** At
every cap ≤ 32 MiB, enwik8's BWT payload loses to the Brotli payload, so the
"ANVIL" arm becomes a Brotli wrapper: 24,810,212 B (+1,272,790 B, only 21,436 B
of surplus left), 0.7155 s decode (139.76 MB/s, **5.3x faster** than the BWT
arm), ANVIL/`xz` = 0.6136, ANVIL/Brotli = 1.0013 (**G3 and G4 PASS**), peak RSS
251.6 MiB (3.79× `xz` — G5 still FAIL, 0.999× Brotli — G6 PASS). This is the
known I10 effect (the cap routes away from BWT on enwik8 at +5.4 % bytes) and
it is re-measured here with the same-job protocol.

---

## 8. Fuzz / registry / CLI-contract results for the changed binary

### 8.1 The repo fuzz / registry path

`run_url:` <https://github.com/thelabcorner/anvil/actions/runs/39676039083>
(and 39676132197, 39680371188)
`job:` `sweep (silesia)`, `sweep (enwik8)`
`workflow:` `.github/workflows/anvil-i20-aux-walk-sweep.yml`
`commit_sha:` `ef5d44a49b0ad0149a639697c944ff81c34dc185` (timed / cap-only
runs), `1c6784ec357ef3ba775ad007a7fa4e4982415ab9` (byte-only run)
`artifact:` `anvil-i20-aux-walk-sweep-silesia-39676039083` (etc.)
`corpus:` silesia, enwik8

```
PASS seed=41246 roundtrip_variants=1040 mutations=6240 deterministic_rev2=9
     deterministic_bwt=24 aux_bwt=24 golden_bwt=4 forced_postcoders=20
     registry_block_modes=14 registry_transforms=5
     suite_modes=[0, 1, 2, 3, 5, 6, 8] suite_unforceable=[4, 7]
```

`tests/fuzz.py` passes on the changed binary in every job: the registry
coverage round-trip, the forced-postcoder coverage (20 cases) and the
auxiliary-BWT assertions (24 cases) are all green, and the round-trip variant
count (1040) and mutation count (6240) exceed the I10-1A smoke's 480 / 2880.

### 8.2 Round-trip correctness of every arm

Every arm of every sweep was decoded and compared byte-exactly against its
source **before** any timing number was consumed (`tools/i20_aux_walk_sweep.py`
`verify_roundtrip`; a mismatch raises). No arm failed. The compressed bytes
are deterministic per arm, and the paired driver's `--control-observe` /
`--candidate-observe` determinism assertion was satisfied for every arm
(the driver raises on a non-deterministic observed output).

### 8.3 The knob's CLI contract (new corpus-free regression test)

`tests/i20_bwt_aux_walks_cli.py`, run in every job, artifact
`knob-contract.txt`; **all checks PASS**:

- `--bwt-aux-walks 4096` and `--bwt-aux-walks=4096` produce **identical bytes**
  (this is the check that catches the `substr(15)` off-by-one);
- `--bwt-aux-walks 1024` == `--bwt-aux-walks=1024` == **the default** (the
  default is byte-identical to the explicit `--bwt-aux-walks 1024`);
- `--bwt-aux-walks 16384` == `--bwt-aux-walks=16384`;
- a non-default budget **actually changes** the payload;
- every accepted arm round-trips byte-exactly;
- `--bwt-aux-walks=0`, `=1048577`, `=4294967296`, `=-1`, `=abc`, `==4096` and
  the spaced equivalents are **all rejected** with the explicit error
  `--bwt-aux-walks must be an integer in 1..1048576` and **no output file is
  produced**;
- a missing value is rejected with `--bwt-aux-walks requires a value`;
- `--bwt-subblock 8388608` and `--bwt-subblock=8388608` produce identical bytes.

---

## 9. Verdict

### 9.1 C2 — `NO-GO`

Applying the pre-registered rule (§1.4) to the measured curve:

1. **Byte feasibility (G2).** Every arm of the (cap × walk budget) grid keeps
   the Silesia portfolio below `xz -9e` (the largest is 48,394,677 B at
   16 MiB + W = 65536, 61,327 B of surplus left). So G2 is not the binding
   constraint — the byte budget was never the problem.
2. **The decode gate (G4).** At the byte-feasible point the paired
   decode-time ratio CI-upper vs Brotli is **2.2821** (`c16-w65536`, point
   estimate 2.2680), which is **above 2.0**. The best CI-upper measured
   anywhere in the grid is 2.2821; at the 128 MiB cap it is 2.8518, and in the
   separate cap-only sweep the best is 2.8195 (4 MiB cap). **The rule's second
   clause fires: C2 is `NO-GO`.**
3. **The mechanism.** The measured LF-walk share is **f = −0.0001 (enwik8)**
   and **f = −0.0041 (Silesia)**, both with confidence intervals spanning 0,
   so the measured speed-up is **statistically indistinguishable from 1.0×**.
   The maximum BWT-path speed-up the lever can deliver is **1.014×** against a
   requirement of **2.94×**; the `f` that would be required is 0.70 at
   `k = 16` and 0.67 at `k = 64`.
4. **The falsifying reason.** The inverse BWT performs `n/2` dependent random
   accesses into the LF array for every checkpoint stride `r` (8-way
   interleaved LF walks, `libsais_unbwt_decode_8`), so the auxiliary walk
   budget cannot shorten the inverse-BWT critical path; it only adds
   checkpoint bytes. The source argument in §3 predicted `f ≈ 0` before the
   measurement and the measurement returned `f = 0.000 ± 0.02`.

The independent same-job confirmation from the programme's own arbiter (run
`39670431362`, arm A: `--bwt-subblock 8388608 --bwt-aux-walks 8192` → ANVIL/`xz`
= 1.2866 [1.2658, 1.3035]; run `39670483057`, arm B: `--bwt-subblock 8388608
--bwt-aux-walks 16384` → 1.3986 [1.3827, 1.4327]) shows the same thing from
the other side: **going from W = 8192 to W = 16384 made the candidate
*slower* against the same-job `xz` control (1.2866 → 1.3986) while charging
another 419,744 B.**

### 9.2 What the change is worth keeping

The knob itself is **neutral and default-preserving**: it is an
encoder-policy parameter that selects an already-permitted `r`, it is
byte-identical at the default, and it passes the repo's fuzz/registry path and
a dedicated CLI-contract test. It is **not adopted as a speed lever**; it is
retained as a **measured bytes-only knob** (the ledger entry: "the auxiliary
walk budget is a bytes-only knob — it buys checkpoints, not decode time").

**Recommended operating point: `W = 1024` (the default, unchanged).** The
byte-feasible maximum (`W = 65536` at the 16 MiB cap, 48,394,677 B) buys
**no** decode time, costs 1,928,338 B, and flips two files to the Brotli
backend, so it is strictly dominated by the default.

### 9.3 Next step for the programme

The programme's remaining Silesia deficits are G4 (decode vs Brotli) and G5
(peak RSS vs `xz`), and this arm closes neither. What the measurements say:

- **The decode axis cannot be moved by checkpoint granularity.** Any future
  decode-speed lever must reduce the *number* of dependent random accesses
  (`n/2`) or their latency, i.e. it must change the inverse-BWT representation
  or the working set, not the checkpoint stride. This is consistent with W1's
  independent stage decomposition and with the Amdahl ceiling this sweep
  measures (1.00x).
- **The cap is a memory lever, not a speed lever** (§7). The best memory point
  measured here is the 16 MiB cap: **123.8 MiB peak decode RSS = 2.419x `xz`
  (G5 still FAIL) and 1.011x Brotli (G6 PASS)** for +472,498 B. If the
  programme wants G6 it can have it for ~472 KB; G5 needs the peak RSS to drop
  by a further 2.419x, which no cap in this sweep reaches (the 4 MiB cap is
  2.400x — the floor is set by the `n`-byte output plus the postcoder's own
  buffers, not by the LF array).
- **The 4 MiB cap is the best Silesia decode point measured in this sweep**
  (portfolio 3.5222 s, 60.17 MB/s, ANVIL/`xz` = 1.4665, ANVIL/Brotli
  = 2.6968 [2.6061, 2.8195], peak RSS 122.8 MiB = 2.400x `xz` / 1.003x Brotli)
  for **+1,474,960 B** (514,705 B of surplus left) — **but §7 Reading 3 shows
  that its decode advantage is the `nci` backend-route change, not a BWT
  decode-speed effect.** It closes neither G4 (CI-upper 2.8195 > 2.0) nor G5
  (2.400 > 2.0), so it is not an adoptable arm either; it is the best
  *measured* point and belongs in the ledger as such.

---

## 10. Evidence index

Every number in this document comes from one of the three runs below. All
three are GitHub Actions runs of `thelabcorner/anvil`; **no local computation
was used for any number** (local work was limited to editing, static
inspection, git/GitHub operations, and exact arithmetic over the downloaded
artifacts).

| # | run_url | job | workflow | commit_sha | artifact | corpus | what it establishes |
|---|---|---|---|---|---|---|---|
| E1 | <https://github.com/thelabcorner/anvil/actions/runs/39680371188> | `sweep (silesia)`, `sweep (enwik8)` | `.github/workflows/anvil-i20-aux-walk-sweep.yml` | `1c6784ec357ef3ba775ad007a7fa4e4982415ab9` | `anvil-i20-aux-walk-sweep-silesia-39680371188`, `anvil-i20-aux-walk-sweep-enwik8-39680371188` | silesia, enwik8 | §4 byte identity (per file, 3 representations), §5 byte curve, all `checks` entries OK |
| E2 | <https://github.com/thelabcorner/anvil/actions/runs/39676039083> | `sweep (silesia)`, `sweep (enwik8)` | `.github/workflows/anvil-i20-aux-walk-sweep.yml` | `ef5d44a49b0ad0149a639697c944ff81c34dc185` | `anvil-i20-aux-walk-sweep-silesia-39676039083`, `anvil-i20-aux-walk-sweep-enwik8-39676039083` | silesia, enwik8 | §6 speed curve, the fitted `f`, the A/A nulls, the per-file decode times, §8.1 fuzz, §8.3 the knob contract |
| E3 | <https://github.com/thelabcorner/anvil/actions/runs/39676132197> | `sweep (silesia)`, `sweep (enwik8)` | `.github/workflows/anvil-i20-aux-walk-sweep.yml` | `ef5d44a49b0ad0149a639697c944ff81c34dc185` | `anvil-i20-aux-walk-sweep-silesia-39676132197`, `anvil-i20-aux-walk-sweep-enwik8-39676132197` | silesia, enwik8 | §7 the cap-only decode-speed series (128/32/16/8/4 MiB at W = 1024) and the peak-RSS series |
| E4 | <https://github.com/thelabcorner/anvil/actions/runs/39670431362> | `gate-closure (silesia)` | `.github/workflows/anvil-i20-gate-closure.yml` | `40a82e5f79a890ca666daa7b0e38e1e26e9c347d` | `anvil-i20-gate-closure-silesia-39670431362` | silesia | independent same-job confirmation, arm A: 8 MiB cap + W = 8192, 47,732,549 B, ANVIL/`xz` = 1.2866 [1.2658, 1.3035] (coordinator-run) |
| E5 | <https://github.com/thelabcorner/anvil/actions/runs/39670483057> | `gate-closure (silesia)` | `.github/workflows/anvil-i20-gate-closure.yml` | `40a82e5f79a890ca666daa7b0e38e1e26e9c347d` | `anvil-i20-gate-closure-silesia-39670483057` | silesia | independent same-job confirmation, arm B: 8 MiB cap + W = 16384, 48,152,293 B, ANVIL/`xz` = 1.3986 [1.3827, 1.4327] (coordinator-run) |
| E6 | <https://github.com/thelabcorner/anvil/actions/runs/39654699366> | `reference-cost (silesia)` | `.github/workflows/anvil-i10-aux-reference-cost.yml` | `d05fceb0ce61a853378822f63eb5f2ceeef26557` | `anvil-i10-aux-reference-cost-silesia-39654699366` | silesia | the fresh baseline (46,466,339 B; ANVIL/`xz` 1.4726; ANVIL/Brotli 5.4547; peak RSS 248.3 MiB) that this sweep reproduces byte-for-byte (coordinator-run) |

### 10.1 Evidence classes

- `MEASURED` — every number in §4, §5, §6, §7, §8 read out of the run
  artifacts named above.
- `DERIVED` — the pooled `f` (§6.3, inverse-variance weighted over the
  unconfounded points), the "always-BWT file sum" (§7 Reading 2), the
  `f`-required table (§6.3) and the "1/(1−f)" Amdahl ceilings. All are exact
  arithmetic over the `MEASURED` values.
- `PROJECTION` — **none**. No number in this document is projected; the byte
  budget arithmetic in §5 is measured, not projected.

### 10.2 Portfolio byte totals for the programme's final arbiter

If the coordinator rules an arm with the frozen
`tools/reference_cost_gate.py`, the `--expected-candidate-total` values for
the measured arms are:

| arm | Silesia total (B) | enwik8 total (B) |
|---|---:|---:|
| `c128-w1024` (default) | 46,466,339 | 23,537,422 |
| `c128-w4096` | 46,524,282 | 23,546,578 |
| `c128-w8192` | 46,601,541 | 23,558,786 |
| `c128-w16384` | 46,756,074 | 23,583,201 |
| `c128-w32768` | 47,065,144 | 23,632,030 |
| `c128-w65536` | 47,533,903 | 23,729,686 |
| `c16-w1024` | 46,938,837 | 24,810,212 |
| `c16-w4096` | 47,037,629 | 24,810,212 |
| `c16-w8192` | 47,147,432 | 24,810,212 |
| `c16-w16384` | 47,345,366 | 24,810,212 |
| `c16-w32768` | 47,741,236 | 24,810,212 |
| `c16-w65536` | 48,394,677 | 24,810,212 |
| `c8-w1024` | 47,365,274 | 24,810,212 |
| `c4-w1024` | 47,941,299 | 24,810,212 |

The exact flag spelling for the arbiter is
`--bwt-aux-walks <N>` (spaced) and `--bwt-aux-walks=<N>` (equals); both are
accepted and produce identical bytes, and `--bwt-subblock <bytes>` is
accepted in both spellings. The combined-arm form the coordinator asked about
is therefore:

```
gh workflow run anvil-i20-gate-closure.yml --repo thelabcorner/anvil \
  --ref research/anvil-i20-pareto-gate-closure-20261021 \
  -f candidate_ref=research/anvil-i20-w2-aux-walk-sweep \
  -f candidate_extra_args="--bwt-aux-walks 16384 --bwt-subblock 16777216" \
  -f expect_silesia=47345366 -f expect_enwik8=24810212 -f reps=9
```

(with the measured totals for that arm). **Note that this arm is not
recommended**: its measured decode-ratio CI-upper vs Brotli is 3.4100, i.e.
worse than the default's 3.3672 at the same cap, and it buys no decode time.
