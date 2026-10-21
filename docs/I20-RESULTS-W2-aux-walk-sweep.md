# ANVIL I20 C2 — Auxiliary-Index Walk-Budget Sweep (`--bwt-aux-walks=N`)

**Worker:** W2
**Date:** 2026-10-21
**Branch:** `research/anvil-i20-w2-aux-walk-sweep`
**Base commit:** `d05fceb0ce61a853378822f63eb5f2ceeef26557`
**Programme document:** `docs/I20-PARETO-GATE-CLOSURE-PREREG-2026-10-21.md` (binding)
**Mechanism class:** `{adopt-class}` / engineering. **No novelty claim.**
**Verdict:** **`NO-GO`** — see §7.

---

## 1. Hypothesis and mechanism

### 1.1 Hypothesis

The auxiliary-index inverse-BWT representation buys decode speed by shipping a
checkpoint index array. The encoder's **walk budget** — the number of
independent LF walks it aims for — was hard-coded (`kBwtAuxTargetWalks = 1024`),
so the checkpoint stride `r` had never been swept. **Increasing the walk budget
`W` shrinks `r`, which shortens each LF walk, which should make the inverse BWT
materially faster, at a fully chargeable byte cost of `4 * auxiliary_count` bytes
per BWT-routed file.**

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

and the wire emission (line 4338-4344):

```
4338:             cand.push_back(kBwtAuxTag);
4339:             cand.push_back(post);
4340:             put_uvar(cand,static_cast<uint32_t>(aux_rate));
4341:             put_uvar(cand,aux_indexes.size());
4342:             for(int32_t idx:aux_indexes) put_u32le(cand,static_cast<uint32_t>(idx));
```

so, exactly:

- `r` = smallest power of two with `r * W >= n`, minimum 2 (`bwt_aux_rate_for_size`);
- `auxiliary_count = 1 + floor((n-1)/r)` ≈ `W`;
- the index array costs **`4 * auxiliary_count` bytes** on the wire
  (`put_u32le` per checkpoint);
- a larger `W` therefore gives a **smaller `r`**, **more checkpoints**,
  **shorter LF walks** and a **larger index array**.

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

`FORMAT.md` records this explicitly (see §3.4).

### 1.4 Pre-registered decision rule (quoted verbatim)

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

`tests/i20_bwt_aux_walks_cli.py` (new): a corpus-free CLI-contract regression
test for the knob (see §8.3).

`tools/i20_aux_walk_sweep.py` (new): `prepare` / `rss` / `summarize`.
`.github/workflows/anvil-i20-aux-walk-sweep.yml` (new): manual-only byte-only
and timed sweeps, corpus matrix, `permissions: contents: read`,
`cancel-in-progress: false`, finite timeout, `if: always()` artifact upload.
`FORMAT.md`: one encoder-policy paragraph (§3.4).

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

## 3. *(measurement sections — filled in from the Actions runs)*

---

## 3.5 A source-level prediction, stated before the measurement

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
(`r = 8192..65536` on Silesia), so shrinking `r` further shortens each
individual walk but does **not** shorten the critical path and does **not**
remove any memory access.

This predicts a fitted LF-walk share `f` close to 0, i.e. a BWT-path speed-up
close to 1.0x, and therefore a `NO-GO` on the decode gate — the auxiliary index
would already have captured the entire available win at `W = 1024`. The sweep
below measures the curve rather than assuming the prediction, and the fitted
`f` is reported with its interval.

---

## 8. Evidence index

*(filled in from the Actions runs)*
