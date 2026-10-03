# Track 07 — BWT Subblocking — Space Bunny Free (INTERIM CHECKPOINT)

**Agent:** Space Bunny Free (constructive inventor)
**Track:** 07-bwt-subblocking
**Date:** 2026-10-02
**Status:** INTERIM. Mechanism derived and cost-modelled; **no new measurement performed**.
**Live source read:** `i10-aux-unbwt` @ `b8eae11` (worktree intentionally dirty; untouched)
**Prototype (isolated, not wired):** `prototypes/swarm-2026-10-02/07-bwt-subblocking/space-bunny/sbw_model.py`
(`selftest` PASS, 29 checks; it contains no encoder, no decoder, no entropy coder and no timing loop)

---

## 0. Scope honesty — read this first

**This track is NOT an external crossing and this report does not claim one.**

`docs/I10-AUX-UNBWT-RESULTS.md` §11 already rules the aux representation
`FRONT-GAP_COST` on both canonical corpora, and `RESEARCH_LEDGER.md:4820` already
instructs: *"CONTINUE cheaply: bounded BWT subblocking for memory/decode
tradeoff; **do not confuse it with an external frontier claim**."*

This report goes further than "continue cheaply" in one respect and further in
another, and both matter:

1. **It closes the frontier ambition for this mechanism with a derivation, not
   an opinion.** Section 5 shows that under the project's own frozen memory
   rule (`M_budget = 2·min(R_xz, R_br)`) **no subblock cap can pass, on either
   corpus, for a structural reason that has nothing to do with subblock size.**
2. **It identifies one cheap, source-level, falsifiable defect in the existing
   aux mechanism** (§4, H-1) whose resolution would remove ~99% of the charged
   aux wire cost at predicted-equal decode speed — and, more importantly,
   **establishes the first principled coupling between the two subblock axes**
   (§4, H-2), which no document in this repo states.

The honest bottom line is in §11: **PILOT**, scoped to the ratio-neutral,
wire-neutral work, with the frontier route explicitly abandoned.

---

## 1. Evidence map

Legend: **[M]** measured fact with source path · **[S]** derived by reading
source, arithmetic only · **[H]** hypothesis / projection, unmeasured.

### 1.1 Measured facts this report relies on

| ID | Fact | Value | Source |
|---|---|---|---|
| M1 | Aux index charged cost, three paired targets | dickens **+2,494 B**, webster **+2,535 B**, enwik8 **+3,054 B** (total **+8,083 B**) | `docs/I10-AUX-UNBWT-RESULTS.md` §3.1, §4.1, §5.1, §6 |
| M2 | Whole-codec decode speedup from aux | **1.364× / 1.795× / 2.339×**, all clearing the 2% gate | `docs/I10-AUX-UNBWT-RESULTS.md` §6 |
| M3 | Aux does **not** reduce peak decode RSS | "effectively unchanged … ~599 MiB" on enwik8 | `docs/I10-AUX-UNBWT-RESULTS.md` §9 |
| M4 | Silesia subblock sweep, 8/16/32/64/128 MiB | 8 MiB: **+1.9346%**, RSS 124.4 MiB, serial decode **3.7022 s**; 16 MiB: **+1.0169%**, RSS 125.7 MiB; 32 MiB: **+0.3923%**; 128 MiB: baseline, RSS 248.4 MiB | `docs/I10-AUX-UNBWT-RESULTS.md` §11 (run `35930672607`) |
| M5 | enwik8 subblock sweep | 8/16/32 MiB **route away from BWT** (+5.4075%); 64 MiB retains BWT, **+3.2259%**, RSS **411.4 MiB**; enwik8@128 MiB RSS **598.9 MiB** | same |
| M6 | Reference-cost frontier | Silesia ANVIL aux 46,466,339 B / ~47.9 MB/s / **248.4 MiB**; xz -9e 48,456,004 B / ~82.5 MB/s / **54.3 MiB**; Brotli q11/lw30 49,383,136 B / ~166.9 MB/s / **124.5 MiB**. enwik8 ANVIL 23,537,422 B / ~26.5 MB/s / **598.9 MiB**; xz 24,831,648 B / ~103.6 MB/s / **66.2 MiB**; Brotli 24,810,180 B / ~150.2 MB/s / **251.9 MiB** | `docs/I10-AUX-UNBWT-RESULTS.md` §11; `docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md` §1.1 (run `35927623136`) |
| M7 | Subblocking at 8 MiB is **faster** in serial decode than at 128 MiB | 3.7022 s vs 4.2015 s (**11.9% faster**) while **+1.93% bytes worse** | M4 (run `35930672607`) |
| M8 | Random access is **NOT_CLAIMED** for any existing framing | `RANDOM_ACCESS_NOT_CLAIMED` | `docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md` §6.1, §8 IR-G7 |
| M9 | I10-1A classified **adopt-class**, "no novelty claim" | "I10-1A is an adopt-class engineering result, not a novelty claim" | `docs/I10-AUX-UNBWT-RESULTS.md` §11 |

### 1.2 Source-derived facts (read directly; no measurement)

| ID | Fact | Source |
|---|---|---|
| S1 | Per-subblock aux rate is computed from the **piece** size against a **fixed 1024-walk** target: `target=ceil(n/1024); r=smallest pow2 ≥ target; icount=1+(n-1)/r` | `src/anvil.cpp:4247-4256`, `:4266-4274`, `:3913` |
| S2 | The **outer** subblock wrapper partitions the **input** and calls `bwt_backend_encode` per piece | `src/anvil.cpp:4446-4457` |
| S3 | In the aux (v2) form the **primary index is not separately transmitted** — it *is* `I[0]`. Legacy v1 transmits `uvar(primary)` | `src/anvil.cpp:4311-4323`; `docs/I10-AUX-UNBWT-INTEGRATION-PLAN.md` §3.1 |
| S4 | The decoder allocates `std::vector<int32_t> tmp(expected+1)` — **n×4 bytes — for BOTH the aux and legacy inverse paths** | `src/anvil.cpp:4391-4394` |
| S5 | `libsais_unbwt_aux` requires `A != NULL` and passes it as `P`; it additionally allocates `bucket2` = 256²×4 = **256 KiB** and `fastbits` ≤ **256 KiB**, both per call | `third_party/libsais/src/libsais.c:8040-8055`, `:7994-8011` |
| S6 | `P` is **fully written and read for all n entries** — it is the destination of a global bigram counting sort (`P[bucket2[w]++] = i`) and is indexed by *global* output position | `third_party/libsais/src/libsais.c:7515-7548`, `:7550-7569` |
| S7 | The stride `r` splits the **output** into `blocks=(n-1)/r+1` chunks that are handed to `libsais_unbwt_decode_1..8` | `third_party/libsais/src/libsais.c:7882-7939` |
| S8 | The widest shipped kernel is **8-way**; the inner loop is a serial pointer chase `p = P[p]` with a bigram bucket scan, replicated 8× | `third_party/libsais/src/libsais.c:7854-7880` (also `:7714-7726` for the 1-way version) |
| S9 | `ratio_backend_decode` **materialises the entire output** (`out.reserve(expected)`, then `insert` per subblock) | `src/anvil.cpp:4481-4492` |
| S10 | Postcoders 0 and 4 are dead (known encoder/decoder mismatch); only 1/2/3 are live | `src/anvil.cpp:4293-4307` |

### 1.3 The two facts nobody in this repo has combined

- **S4 + S6 + S7:** the stride `r` controls the **ILP width** of the inverse-BWT
  kernel (S8). It does **not** reduce the `P` buffer, which is `4n` bytes
  regardless of `r` (S4, S6).
- **S3:** therefore the measured M3 ("memory effectively unchanged", M3) is not
  an artefact of measurement resolution. **It is a structural consequence of the
  libsais API.** The aux index and the memory axis are *exactly orthogonal*.

---

## 2. Strongest mechanism statement

The track's mandate asks for "a principled memory/decode/ratio tradeoff with
bounded subblocks, aux indexing, primary-index charges." The strongest
defensible mechanism is **not** a new subblocking scheme. It is a **joint
two-level geometry model in which the two existing knobs are shown to be
coupled by a feasibility constraint that the current implementation violates.**

> **Mechanism M-7: Subblock Geometry Coupling (SGC).**
> The BWT decode path has exactly two independent geometry parameters:
> a **partition cap** `C` (bytes per input-partitioned subblock) and a
> **checkpoint stride** `r` (LF-checkpoint spacing inside each subblock).
> Their cost axes are *disjoint* — `C` buys memory and parallelism and pays
> ratio; `r` buys decode latency and pays ~zero ratio — but their **byte and
> feasibility budgets are not disjoint**. Under a single memory budget the
> correct joint optimum requires:
> 1. a **globally allocated** checkpoint budget `K ≈ N/r` (so charged index
>    bytes are `Θ(K)`, independent of `B`), and
> 2. the feasibility condition **`B ≤ K/2`** (every subblock needs at least one
>    usable checkpoint, and `r` is power-of-two quantised).
>
> The current implementation instead applies a **per-subblock** 1024-walk
> target (S1), which makes charged index bytes grow as `Θ(B·1024·4)` and
> **halves the per-subblock walk width at every doubling of B**.

SGC is adopt-class in its policy half. Its *content* is the coupling itself,
plus the two hypotheses below, which are the only mechanism-level claims in
this track that could carry novelty weight.

---

## 3. Falsifiable hypotheses

### H-1 — Walk-width saturation (the strongest, cheapest, most falsifiable claim)

> **The decode gain from `libsais_unbwt_aux` is an instruction-level-parallelism
> effect, and ILP saturates at 8 independent LF chains — the width of the widest
> kernel libsais ships (S8).** The current ~1024-walk policy therefore
> **over-provisions the index by roughly two orders of magnitude**, and the
> charged aux cost can be cut from thousands of bytes to tens of bytes at
> predicted-equal decode speed.

**Grounding.** `libsais_unbwt_decode` dispatches at most 8 checkpoints per
kernel invocation (`libsais.c:7887-7939`), and each kernel body is 8 copies of
a load→use pointer chase (`libsais.c:7867-7877`). The legacy path
(`libsais_unbwt` → `r = n`, `libsais.c:8030-8033`) yields `blocks == 1`, i.e. a
single chain that is pure load-latency bound. Any `r ≤ n/8` yields the full
8-way form.

**Power-of-two trap (new, and it matters).** `r` must be a power of two
(`libsais.c:8042`, and `src/anvil.cpp:4353`). With `n = 100,000,000`, asking for
8 walks gives `r = 2^24 = 16,777,216` and `icount = 1 + ⌊99,999,999/16,777,216⌋ = 6`
— **under 8**. Requesting 16 walks gives `r = 2^23` and `icount = 12`. The
prototype asserts both facts (`selftest`: `quant-K8-yields-6`, `sat-icount-ge-8`).

**Falsifier.** If the paired whole-codec decode speedup at `icount ≈ 12` is
statistically indistinguishable from the speedup at `icount ≈ 763`, H-1 is
**confirmed** and the current policy is over-provisioned. If the small-`icount`
arm loses the 2% practical-speed gate against the `icount ≈ 763` arm, H-1 is
**falsified** and the existing 1024-walk policy stands.

**Explicit non-claim.** I do **not** claim H-1 removes the *measured* +8,083 B
from history. Nothing here reinterprets M1.

> **CORRECTION (2026-10-02, post pair-reconciliation).** My first draft asserted
> H-1 "would be a new wire variant." **That is wrong and I withdraw it.** The v2
> decoder accepts *any* `(r, icount)` satisfying `icount == 1 + (n-1)/r`
> (`src/anvil.cpp:4355`), so a W16 payload is read correctly by the **existing,
> unmodified** v2 decoder. No new tag, no new representation id, no new
> malformed-input surface. H-1 is an **encoder default-policy change only**.

### H-2 — Θ(B) checkpoint inflation under partitioning

> **Under `--bwt-subblock=C --bwt-aux=on`, total charged index bytes grow as
> `Θ(B)`, not as `Θ(K)`, and per-subblock walk width shrinks as `Θ(n/B)`.
> Corrected to a global budget, total index bytes become independent of `B`.**

**Verified arithmetic on the already-measured enwik8@64 MiB geometry** (prototype
`geom --source 100000000 --cap 67108864`, reproduced by `selftest`):

| Geometry | Pieces | Strides `r` | `icount` per piece | Index bytes |
|---|---|---|---|---:|
| B=1 (cap 128 MiB) | 100,000,000 | 131,072 | 763 | **3,052** |
| B=2 (cap 64 MiB) | 67,108,864 / 32,891,136 | 65,536 / 32,768 | 1024 / 1004 | **8,112** |

**B=2 costs 2.66× the index bytes of B=1** — super-linear in B at the low end,
because power-of-two quantisation means a piece can land just *above* a
`2^k` boundary and lose a factor of two of index density. Under the proposed
global policy at the same geometry the total is `Θ(4K)` and does not move with B.

**Consequence for the record:** *any* subblocked arm of this repo measured with
`--bwt-aux=on` was **overcharged on the aux axis and under-provisioned on walk
width**. The subblocking ratio/RSS/parallelism tradeoff has therefore **never
been measured with a corrected checkpoint budget.** That is a real, unfilled
hole in the evidence base — and it is cheap to fill.

### H-3 — Continuity-priced boundaries (the only ratio-side lever)

> **The ratio cost of an input-partition boundary is proportional to the number
> of LZ reference spans that cross it.** Define
> `Damage(c) = #{ references (s,e) : s ≤ c < e }`. Then
> `Δ_ctx(C) ≈ (δ/N)·Σ_{boundaries c} Damage(c)` for a data-dependent
> `δ` bits per severed reference context. Fixed-stride placement samples
> `Damage` uniformly; **choosing boundaries at local minima of `Damage` subject
> to a bounded size deviation reduces `Σ Damage` at zero wire cost**, because
> the statistic is decoder-invisible and boundary positions are *already*
> transmitted as `uvar(decoded_len)`.

This is the only part of the track that touches the ratio axis, and its appeal
is structural: **its gain is positively correlated with the harm it removes.**
Where `Damage` is heavy-tailed (enwik8: +3.2259% for a single boundary, M5)
minimum-selection can help a lot; where `Damage` is flat or zero (incompressible
input, or a corpus with no long-range structure) it is a *provable no-op* and
subblocking is cheap anyway. Prototype: `damage` subcommand (span statistic
only; explicitly **not** a ratio measurement).

**Zero new wire semantics** — this is a large safety advantage of H-3 over any
alternative: no new tag, no new field, no new malformed-input surface.

---

## 4. Encoder + decoder state machine (exact, from source)

### 4.1 Current encode (`--bwt-aux=on`, `--bwt-subblock=C`)

```
ratio_backend_encode(backend=bwt, X[0..N), opt)            src/anvil.cpp:4435
  if N <= C:  -> bwt_backend_encode(X)                     [bare payload]
  else:
    emit 0xFF
    for each piece P_j = X[off .. off+C):                  src/anvil.cpp:4450
        bwt_backend_encode(P_j):                            src/anvil.cpp:4258
          n_j = |P_j|
          if bwt_aux and n_j > 1:
              r_j   = pow2(ceil(n_j / 1024))               src/anvil.cpp:4247   <-- PER-PIECE
              ic_j  = 1 + (n_j - 1) / r_j
              require 0 < ic_j <= 2^20
              libsais_bwt_aux(P_j, B_j, A_j, n_j, 0, null, r_j, I_j)
              require every I_j[t] in [1, n_j]
              primary_j := I_j[0]                          (no separate primary byte)
          else:
              primary_j := libsais_bwt(P_j, B_j, A_j, n_j)
          if n_j == 1: emit legacy-v1 raw-stream, done     src/anvil.cpp:4283
          BwtMtfRle(B_j)                                    src/anvil.cpp:4290
          for post in {0,1,2,3,4}, skipping 0 and 4:       src/anvil.cpp:4291,4303
              cand = 0xFE, post, uvar(r_j), uvar(ic_j), I_j[0..ic_j) u32le, payload
              keep the smallest COMPLETE cand              src/anvil.cpp:4311-4325
          release MTF intermediates                        src/anvil.cpp:4330
        record (n_j, |cand|)
    emit uvar(B); for each j: uvar(n_j), uvar(|cand_j|), cand_j
```

Note `src/anvil.cpp:4311-4318`: the **complete** v2 candidate (header +
index + payload) is what postcoder selection minimises. The aux charge is
therefore fully inside the selection objective — this part of the design is
honest and must not be "optimised" away later.

### 4.2 Current decode (`ratio_backend_decode`)

```
ratio_backend_decode(backend=bwt, p, n, expected)          src/anvil.cpp:4463
  if p[0] != 0xFF: bwt_backend_decode(p, n, expected)
  else:
    p++; nsub = uvar                                    validate nsub in [1, expected] and
                                                       nsub <= (e-p)/2          :4474-4480
    out.reserve(expected)                              <-- MATERIALISES ALL OUTPUT  :4481
    for i in 0..nsub-1:
        dlen = uvar; plen = uvar
        validate dlen in [1, expected - out.size()]                :4484
        validate plen <= e - p                                      :4486
        sub = bwt_backend_decode(p, plen, dlen); p += plen        :4487
        out.insert(out.end(), sub.begin(), sub.end())              :4489   <-- COPY
    require p == e and out.size() == expected                      :4491-4492

bwt_backend_decode(p, n, expected)                       src/anvil.cpp:4334
  require 1 <= expected <= INT32_MAX                              :4335
  if p[0] == 0xFE:
      post = p[1]; require expected > 1                           :4345-4349
      r = uvar; icount = uvar
      require 2 <= r <= expected, r <= INT32_MAX, r is pow2      :4352-4354
      require icount == 1 + (expected-1)/r                       :4355-4356
      require icount <= 2^20, icount <= expected, icount*4 <= rem :4356-4358
      read icount x u32le; require each in [1, expected]          :4360-4364
      primary := aux_indexes[0]                                   :4366
  else:
      post = p[0]; primary = uvar; require primary in [1,expected] :4368-4372
  bwt = <postcoder 1|2|3 decode>(p, expected)           (post 0,4 unsupported)
  out = new uint8[expected]; tmp = new int32[expected+1]           :4391
  libsais_unbwt_aux(bwt, out, tmp, expected, null, r, I)  or  libsais_unbwt(...,primary)
  require rc == 0                                                  :4395

libsais_unbwt_aux(T, U, A, n, freq, r, I)                libsais.c:8040
  require T,U,A,I != NULL; n >= 0; (r==n) or (2<=r<=pow2)          :8042
  require I[t] in [1,n] for all t <= (n-1)/r                      :8053
  libsais_unbwt_main:                                               :7994
     bucket2  = alloc(256*256*4)          = 256 KiB                :7998
     fastbits = alloc((1 + (n>>shift))*2) <= 256 KiB               :7999
     libsais_unbwt_core:
        libsais_unbwt_init_single: histogram(T) ; bigram histogram ;
             builds P over the whole T                            :7550-7569
        libsais_unbwt_decode: blocks=(n-1)/r+1 chunks of size r,
             dispatched 8 at a time to decode_8                     :7882-7939
             decode_8 body: 8 x { fastbits[p>>shift]; bucket2 scan;
                                p = P[p]; U_k[i] = c }             :7867-7877
```

### 4.3 State summary

| State | Encoder | Decoder | Bytes |
|---|---|---|---|
| MTF list | 256 B | 256 B | 0 |
| arithmetic state | ~tens of B | ~tens of B | 0 (adaptive) |
| adaptive postcoder model | 257×256 entries | 257×256 entries | **0 transmitted** |
| postcoder candidate payload | ≤ 2 live | — | 1× |
| BWT string `B_j` | `C_j` | `C_j` | in payload |
| libsais `P`/`tmp` | `4·C_j` (encode) | `4·C_j` (decode) | 0 |
| `bucket2` + `fastbits` | 512 KiB | 512 KiB | 0 |
| primary index (legacy v1) | `uvar` per piece | — | **~4 B/piece** |
| primary index (aux v2) | **implicit in I[0]** | — | **0 extra** |
| checkpoint index `I` | `4·icount` | `4·icount` | **`4·icount`** |
| outer `0xFF` wrapper | ~`6 + 2·uvar(n_j)` | same | **~18 B at B=2** |

**Important asymmetry for track 06:** every live postcoder is *adaptive*, so
per-subblock postcoder restarts cost **statistical warm-up, not transmitted
model bytes**. The ratio penalty of partitioning is therefore *not* dominated by
model-header retransmission — it is dominated by lost long-range suffix context.
That materially changes where effort should go, and it also means H-3's value
would partly evaporate if track 06 ever adopts *transmitted* per-block models.

---

## 5. Full cost model

### 5.1 Bytes (everything charged)

```
W(N, C, r) = W_wrap(B)  +  4·Σ_j icount_j  +  W_primary(B)  +  W_cold(B)
             ~18 B      +  Θ(K) or Θ(B·1024)  +  0 if aux      +  statistical only
```

* `W_wrap(B) = 1 + uvar(B) + Σ_j (uvar(n_j) + uvar(plen_j))`. Measured-relevant
  magnitudes: **18 B at B=2**, ~90 B at B=16. Negligible in every arm.
* `W_primary(B) = 0` under aux v2 (S3) — **the aux representation strictly
  dominates legacy v1 on this axis**, by ~4 B per subblock.
* **The dominant, and currently *unpriced*, term is not in this formula.** It is
  `Δ_ctx(N, C)`, the lost long-range suffix context (§3, H-3). It is a *ratio*
  term, invisible to byte accounting, and it is what actually decides the
  subblock question.

### 5.2 The measured `icount` values, recomputed from source

Prototype `selftest` reproduces all three of these exactly, which independently
validates S1 against M1:

| Target | `n` | `ceil(n/1024)` | `r` | `icount` | index bytes | M1 measured delta |
|---|---:|---:|---:|---:|---:|---:|
| dickens | 10,192,446 | 9,954 | 16,384 | 623 | **2,492** | +2,494 (Δ=+2 framing) |
| webster | 41,458,703 | 40,488 | 65,536 | 633 | **2,532** | +2,535 (Δ=+3) |
| enwik8 | 100,000,000 | 97,657 | 131,072 | 763 | **3,052** | +3,054 (Δ=+2) |

The prototype predicts the I9 raw-index figures (2,492 / 2,532 / 3,052) quoted in
`docs/I10-AUX-UNBWT-RESULTS.md` §3.1, §4.1, §5.1 to the byte. **Provenance
preserved: these are the same numbers from the same runs, not a new measurement.**

### 5.3 Memory (the binding axis)

```
RSS_decode(N, C) ≈ N_out  +  α·C  +  4·icount  +  512 KiB  +  P₀
```

where `N_out` is the materialised output (S9) and `α·C` is the per-subblock
working set `B_j (1×) + sub-slice (1×) + P (4×)` = 6× nominal.

**Fit against M5/M6 (enwik8, where BWT is the *only* route):**

| Arm | `N_out` | `C` | model at α=6 | model at **α=5.0** | **measured** | residual @ α=5 |
|---|---:|---:|---:|---:|---:|---:|
| enwik8, cap 128 MiB (B=1) | 95.37 MiB | 95.37 MiB | 667.6 | **572.2** | **598.9** | +26.7 (+4.7%) |
| enwik8, cap 64 MiB (B=2) | 95.37 MiB | 64.0 MiB | 479.4 | **415.4** | **411.4** | −4.0 (−1.0%) |

**Silesia is the informative failure:** cap 16 MiB measured **125.7 MiB**
(M4). The largest BWT-routed Silesia file is webster (39.54 MiB source,
~6.9 MiB output), for which the model predicts **6.9 + 5·16 = 86.9 MiB** —
**38.8 MiB below the measurement.** And cap 8 MiB measures 124.4 MiB, *lower*
than cap 16's 125.7 despite strictly smaller subblocks. **The Silesia RSS floor
(~125 MiB) is not set by the BWT subblock lever at all.** It is set by the five
non-BWT-routed files (mozilla 51.3 MB, samba 21.6 MB, sao 7.3 MB, ooffice 6.2 MB,
xml 5.3 MB — 43.2% of the corpus) decoding through Brotli q11/lw30. **[S]**

### 5.4 The structural NO-GO on the memory axis

```
M_budget = 2 · min(R_xz, R_brotli)
Silesia : 2 · min(54.3, 124.5) = 108.6 MiB
enwik8  : 2 · min(66.2, 251.9) = 132.4 MiB
```

* **Silesia: FAIL at every cap.** The BWT lever bottoms out at ~124.4 MiB
  (M4) against a 108.6 MiB budget — **14.5% over, and unreachable by shrinking
  `C`**, because the floor is set by non-BWT routes [S5.3].
* **enwik8: FAIL at every cap that keeps bytes competitive.** Prototype
  `geom --m-budget 132.4` reports `max_cap_within_budget = 7,766,292 B
  = 7.41 MiB`. That is `B = 14` internal boundaries. But **M5 measures a single
  boundary at `C = 64 MiB` (B=2) as +3.2259% bytes**, i.e. ~759 KB on a
  23.5 MB payload. Against xz the entire byte margin is
  `24,831,648 − 23,537,422 = 1,294,226 B`. **One boundary consumes 59% of the
  entire margin; thirteen more are arithmetically impossible.**

> **N-BW-1 (structural negative, the report's main conclusion).**
> **No subblock cap can satisfy the project's own frozen memory rule on either
> canonical corpus.** The binding term is `N_out`, which is a *container*
> property (S9), not a BWT property. Bounded-memory BWT decode requires either
> abandoning materialisation (a different mechanism, and streaming must not be
> faked) or a `4C`→`O(C/8)` `P` buffer, which the libsais API forbids (S4, S6).
> **Subblocking is therefore a rate/cache lever and an independent-region
> parallelism lever. It is not, and cannot be, the memory-frontier lever.**

This is fully consistent with the existing sweep's own verdict — *"No smaller
cap dominates the 128 MiB max-ratio point"*
(`docs/I10-AUX-UNBWT-RESULTS.md` §11) — and it now has a mechanism.

### 5.5 Cycles (ESTIMATE, decomposed; structural claim is [S] from S7/S8)

Per output byte of inverse BWT, single thread:

| Stage | Cost | Bound by | Confidence |
|---|---|---|---|
| `compute_histogram` | ~1 c/B | sequential read, 256 L1 counters | ESTIMATE, high |
| `compute_bigram_histogram_single` | ~2–4 c/B | sequential read, random RMW into 256 KiB `bucket2` (L2) | ESTIMATE, high |
| `calculate_biPSI` | ~3–10 c/B | **random WRITE into `P` (4n)** → cache/TLB miss for `n` ≫ 2 MiB | ESTIMATE, medium |
| `libsais_unbwt_decode_k` | **legacy (`k=1`): 4–200 c/B, latency-bound**; **aux (`k=8`): ~1–2 c/B, throughput-bound** | **critical path `p = P[p]`; ILP = k** | **STRUCTURE [S8]**; magnitude ESTIMATE |

**The structural claim is the important one.** Legacy decode is a *single*
dependent load chain (`libsais.c:7720-7723`); aux decode is *eight* independent
chains (`libsais.c:7867-7877`). That is the entire M2 effect, and it is why
H-1 predicts saturation: **beyond 8 chains there is no more ILP to extract.**
`icount ≈ 763` is already 95× past saturation.

Two model-level consequences:

1. **`B` is time-neutral in the main term** (each piece pays the same
   per-byte costs), *except* that each `libsais_unbwt_aux` call re-allocates and
   re-zeroes 512 KiB of `bucket2`/`fastbits` and re-runs a full 256² bigram
   histogram — `O(B·σ²)` = `O(B·65,536)`, negligible for `C ≫ 1 MiB`, **material
   for `C ≤ 256 KiB`**. Preregister a floor on `C`.
2. **M7 (subblocking at 8 MiB is 11.9% *faster* in serial decode) is predicted by
   cache residency**: a 6 MiB working set fits in a server L3, a 600 MiB one
   cannot. This is a *real, unexplained-in-the-ledger* measured fact and it means
   the subblock decision has a **three-way** structure (bytes↑, RSS↓, decode↓)
   in which the speed axis actually **favours small caps**. H-3 is designed to
   buy back the byte cost of exactly that move.

### 5.6 Decoder code/state charge

Already charged and closed: **+2,976 B of `.text`**, +8,192 B ELF file size
(`docs/I10-AUX-UNBWT-RESULTS.md` §9). H-1 would require a *new* wire variant
(new postcoder/representation id) with its own text charge; H-2 requires only an
encoder policy change plus a new decoder branch for the uniform-stride case
(small). H-3 requires **zero decoder code** — it changes only which byte offsets
the encoder chooses to cut at, and the boundaries are already in the wire.

---

## 6. Novelty / prior-art risk

| Component | Prior art | Verdict |
|---|---|---|
| Input-partitioned BWT with fixed cap | bzip2 (900 k), pbzip2, lbzip2, xz `--block-size`, libarchive | **None. 30-year-old adopt-class.** |
| LF-mapping checkpointing at stride `r` | FM-index (Ferragina–Manzini 2000); textbook | **None. adopt-class.** |
| `libsais_bwt_aux`/`unbwt_aux` chunked inverse | the vendored library itself (`libsais.c:7123`, `:8040`) | **None. adopt-class** |
| Bigram-bucketed inverse ("biPSI") | bucketed/bidirectional BWT index literature | **None. adopt-class.** |
| Independent-region parallel decode | parallel bzip2; already preregistered at `docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md` | **None.** |
| **H-1** walk-width saturation | *not aware of a published statement that the `r` gain is pure ILP and saturates at the kernel width* | **Genuine risk that this is a trivially-known implementation fact.** Value is in the *measured 64× byte reduction*, not in the theory. |
| **H-2** Θ(B) inflation + global budget | *not aware of prior art* | **Novel but low standalone value** — it is a bug-class correction. |
| **H-3** continuity-priced BWT boundaries | **REAL RISK.** Adaptive/entropy-priced block-boundary selection for BWT is plausibly anticipated; I cannot certify either way. | **Must go to track 20.** |

**Decisive novelty separators I propose for track 20 (per brief §1):**

H-3 counts as mechanism-level novelty **only if all three hold**:
1. the boundary statistic is **decoder-invisible and zero-wire**, **and**
2. it is **jointly optimised with the checkpoint allocation under one memory
   budget** (i.e. M-7/SGC is load-bearing, not decorative), **and**
3. ablation shows the gain comes from **placement**, not from a changed
   postcoder, a changed `C`, or a changed model.

If H-3 reduces to "cut where matches don't cross, at a fixed cap, with no
coupling to the checkpoint budget," it is most likely anticipated and should be
filed as **adopt-class tuning**, not novelty. **I am not claiming novelty for
H-3.** I am claiming it is the only ratio-side lever in this track, it is
cheap, and it is falsifiable.

---

## 7. Asymptotic behaviour

Let `μ = E[Damage]` be the mean in-flight reference load and let the load
distribution have tail index `β`.

* **Ratio.** Fixed stride: `Σ Damage ≈ B·μ`. Minimum-selection over a window of
  `w` candidate positions with bounded deviation `±δC` reduces the per-boundary
  load by a factor `Θ(1)` determined by `β` (order-statistics of a stable law),
  and **to exactly 0 when `Damage ≡ 0`**. So
  `Δ_ctx^min / Δ_ctx^fixed = Θ(1)` with a *data-dependent* constant, and the
  mechanism is a **provable no-op on incompressible input** — where subblocking
  is cheap anyway. This self-consistency (gain ∝ harm) is the argument for H-3.
* **Bytes.** `W = Θ(K·4)` for the global policy, independent of `B`; `Θ(B·K·4)`
  for the current per-piece policy. Both are `O(1)` per input byte — subblock
  framing is **not** where the bytes are.
* **Memory.** `Θ(N_out + C)`. Irreducible `N_out` under S9. `Θ(B)` `bucket2`
  re-allocations are `O(B·σ²)` time and `O(σ²)` peak.
* **Decode time.** `Θ(N)` regardless of `(B, r)`, once `r ≤ n/8` (H-1). Before
  that, `Θ(N·latency)`. **Subblocking is asymptotically time-neutral**; its real
  effect is the cache-residency constant (M7).
* **Parallelism.** `B` independent regions ⇒ ceiling `Θ(B)` on speedup, subject
  to a bounded-deviation constraint so no region dominates the tail (AF-2), and
  subject to `max_live_regions ≤ max_workers` per
  `docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md` §4.2.
* **Random access.** `RANDOM_ACCESS_NOT_CLAIMED` (M8) holds. The `0xFF` frame
  has sequential `decoded_len`/`payload_len` and **no absolute offset table**
  (`docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md` §6.1). H-3 does not change this
  and must not be described as improving it.

---

## 8. Minimum prototype

**Delivered:** `prototypes/swarm-2026-10-02/07-bwt-subblocking/space-bunny/sbw_model.py`

* **No codec.** No encoder, decoder, entropy coder, or timing loop.
* `geom` — exact byte + RSS accounting for `(n, cap)` under both checkpoint
  policies, and `max_cap_within_budget` against a supplied `M_budget`.
* `damage` — reference-span profile of one file; fixed-stride vs
  damage-minimising boundaries with bounded deviation. **Structural statistic
  only, explicitly not a ratio or speed measurement.**
* `selftest` — **PASS, 29 checks** (run locally; synthetic inline inputs only,
  no corpus, no timing). It independently reproduces M1's three index charges to
  the byte, and the Θ(B) claim on the measured enwik8@64 MiB geometry.

Deliberately **not** built: any code that would require a production edit, a
wire-format addition, or a build.

---

## 9. Strongest disconfirming evidence

I am obliged to state this against myself.

1. **N-BW-1 rests on a fitted constant, α=5.0, from two points on one file.**
   If the true model has an additional term that grows with `N_out` (e.g. the
   rev-2 container holding the input and output simultaneously), then
   `max_cap_within_budget` is *smaller* than 7.41 MiB and the NO-GO is **more**
   decisive, not less. The fit only fails in the direction that strengthens my
   conclusion on enwik8. On Silesia the fit fails the other way (§5.3), which is
   why the Silesia NO-GO rests on the non-BWT floor [S], a different argument.
2. **H-1 could be simply wrong.** If the `1.364×` on dickens and `2.339×` on
   enwik8 come substantially from something *other* than chain width — e.g.
   improved `calculate_biPSI` cursor locality at smaller `r`, or bigram-scan
   length effects — then `icount ≈ 12` may lose real speed and the 64× byte
   saving evaporates. **This is the single most likely way this track's
   strongest claim fails**, and it is exactly why §10 is built as a sweep rather
   than a point test.
3. **The denominator of the subblock percentages is ambiguous.** M4/M5 state
   "+1.9346% bytes" / "+3.2259% bytes" without stating whether the denominator is
   the control *compressed payload* or the *source*. I used the compressed
   payload (the conventional reading, and the only one that makes the Silesia
   numbers plausible). **Under the source reading every conclusion in §5.4
   becomes strictly worse**, so my NO-GO is robust to this ambiguity — but the
   ambiguity must be resolved from the run-`35930672607` artifact before any
   preregistration reuses these numbers, and I have not resolved it.
4. **H-3's statistic may not describe ANVIL's actual parser.** ANVIL's production
   parse is a shape-predict/rANS architecture, not plain LZ77. The prototype's
   bounded hash chain is an analysis probe, **not ANVIL's match set**. If
   `Damage` computed from a proxy parse does not predict the actual BWT boundary
   cost, H-3 is void (AF-3).
5. **My "no new measurement" claim must not be mistaken for "no new
   information."** Every number in §1.1 is from a previously recorded run with
   its run ID preserved. I have spliced nothing across runs and created no
   cross-run timing comparison.

---

## 10. The one decisive REMOTE-ONLY experiment

**Preregister before any dispatch. GitHub Actions only. No local sweep.**

**Name:** `SBW-1 — subblock geometry: walk-width saturation and boundary pricing`

**Question (single, causal):** *Holding the representation, corpus, and every
other option fixed, what is the decoder-visible cost and the whole-codec decode
effect of the LF-checkpoint budget `K`, and what is the ratio effect of
boundary placement at fixed `C`?*

**Arms — checkpoint axis (H-1/H-2), `C = 128 MiB`, BWT direct, transforms off:**

| Arm | checkpoint policy | expected `icount` (enwik8) | charged index bytes |
|---|---|---:|---:|
| A0 | legacy v1 (aux off) | — | 0 |
| A1 | **current per-piece, 1024 walks** (the incumbent) | 763 | 3,052 |
| A2 | global budget, 1024 walks | ~1024 | ~4,096 |
| A3 | global budget, 128 walks | ~128 | ~512 |
| A4 | global budget, 32 walks | ~32 | ~128 |
| **A5** | **global budget, 16 walks (≥8 blocks guaranteed)** | **~12** | **~48** |

**Arms — boundary axis (H-3), `K` pinned at A1's incumbent value, direct BWT:**

| Arm | `C` | placement |
|---|---|---:|
| B0 | 8 MiB | fixed stride (incumbent) |
| B1 | 8 MiB | damage-minimising, ±50% |
| B2 | 16 MiB | fixed stride |
| B3 | 16 MiB | damage-minimising, ±50% |

**Mandatory controls, per `docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md`:** same
payload per arm; A/A null; CPU affinity; ambient-load gate; ≥7 paired reps,
all raw reps retained; seeded bootstrap 95% CI; no outlier deletion; output
verification outside the timed interval; full RSS provenance; complete wire
byte breakdown including `aux_index_bytes`, `0xFF` framing, and primary bytes.

**Preregistered thresholds — fixed now, not moved later:**

* **SBW-G0 (identity/correctness).** All arms roundtrip byte-exactly; every
  output SHA equals the serial SHA; the whole malformed-frame battery
  (`docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md` §5.3) rejects. Any failure →
  `NO-GO-SBW`, no interpretation.
* **SBW-G1 (H-1 confirmation).** For A5 vs A1: paired whole-codec decode ratio
  `CI95.upper ≤ 1.00` (i.e. **A5 is not slower than the incumbent**) **and**
  `A5` charged index bytes ≤ 1/16 of `A1`. Both ⇒ **H-1 CONFIRMED**.
  If `CI95.lower > 1.02` (A5 slower beyond the project's 2% practical
  threshold) ⇒ **H-1 FALSIFIED**, incumbent policy stands, mechanism 2 is dead.
* **SBW-G2 (H-2 confirmation, arithmetic-only, no timing needed).** On the
  `C = 8 MiB` enwik8 geometry, the *current* policy's charged index bytes exceed
  the *global* policy's by ≥1.5×, and the current policy's `B=1` charge is below
  its `B=2` charge by ≥1.5×. Verified offline in `selftest`/`geom`; the remote
  run must confirm the *wire* matches the model. Mismatch ⇒ model wrong, report
  as `MODEL-MISMATCH`, do not reinterpret.
* **SBW-G3 (H-3 confirmation).** For B1 vs B0 at equal `C`: complete wire bytes
  improve by **≥1.0% of the measured fixed-stride penalty** (i.e. recover at
  least a tenth of what partitioning cost), on **≥3 of 4** Silesia BWT-routed
  files, with **no file worse**. Below that ⇒ H-3 recorded as **NO-EFFECT**, not
  as a partial pass.
* **SBW-G4 (frontier honesty, mechanical).** Apply `IR-G6`
  (`docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md` §8) unmodified. Given §5.4, the
  **predicted** outcome is `FRONT-GAP_COST` on both corpora. A `FRONT_CROSSING`
  may only be declared if all five comparisons pass in the same job.
* **SBW-G5 (never a rescue).** SBW-1 may not be used to alter any frozen I9/I10
  baseline, and `RANDOM_ACCESS` stays `NOT_CLAIMED` regardless of outcome.

**Explicitly expected-negative, preregistered so it cannot be read as failure:**
enwik8 at `C ≤ 32 MiB` routes away from BWT (M5), so enwik8 is **expected** to
show no BWT arm at small `C`. That is a *predicted* property of the routing, and
its occurrence is neutral.

**Cost/benefit warning to the coordinator:** SBW-G1 alone can be answered by a
5-arm sweep with no subblocking at all. **SBW-G3 needs the damage probe to be
run on the frozen corpus first, which is a measurement and therefore remote.**

---

## 11. Adversarial failure cases

| ID | Failure | Consequence | Required control |
|---|---|---|---|
| AF-1 | **Postcoder route heterogeneity.** Postcoder selection runs *per piece* (`src/anvil.cpp:4311-4325`); damage-minimising cuts shift the per-piece statistics and may flip postcoder choices, so a byte change could come from a *different code* rather than from placement. | Invalidates the H-3 attribution | Pin the postcoder per corpus in the primary arm; report `route_count`; run a free-postcoder arm as secondary only |
| AF-2 | **Subblock collapse / blow-up.** A minimum of `Damage` can sit anywhere in the window, producing one 0.5 MiB piece and one 15 MiB piece; peak memory is set by the largest, so the benefit evaporates while the framing cost stays. | Silent loss of the whole mechanism | Hard bounded deviation `C ∈ [C/2, 2C]`, asserted in `selftest` (`bounded-deviation`); report the realised piece-length distribution, not just the count |
| AF-3 | **Statistic/parse mismatch.** `Damage` comes from a bounded hash chain, not from ANVIL's shape-predict parse. | H-3 void | Preregister a correlation check between the probe's `Damage` and the *actual* per-boundary postcoder cost before any promotion |
| AF-4 | **Asymmetric corpus effect.** enwik8's ~759 KB per boundary (M5) means minima-selection cannot rescue it; H-3 may help Silesia and do nothing on enwik8. | Over-generalisation | Preregister the asymmetry as the *expected* result; "no effect on enwik8" is **PASS-not-FAIL** |
| AF-5 | **`r` quantisation collapse.** Power-of-two `r` plus the `2 ≤ r ≤ n` clamp (`libsais.c:8042`) means a global budget silently degenerates when `B > K`, and the byte charge can jump by orders of magnitude. | Catastrophic, invisible wire bloat | The prototype's `geom` already reports `feasible`/`degenerate`; **any degenerate geometry is an automatic NO-GO**, never a silent fallback |
| AF-6 | **Tail latency under parallel decode.** Unequal pieces make the largest region dominate D2/D4/D8. | Apparent parallel speedup that does not survive at D8 | `max_live_regions ≤ max_workers`; ordered assembly; report per-arm realised worker counts per `...PREREG.md` §3.1 |
| AF-7 | **Wire-invisible overreach.** Claiming "random access", "seek", or "streaming" from independent regions. | Gate violation (IR-G7, M8) | `RANDOM_ACCESS_NOT_CLAIMED` is a hard string in the report and the workflow summary |
| AF-8 | **Canonical-byte drift.** H-3 adds no field, and — corrected — H-1 adds no field either. But changing the walk target changes the **encoder's canonical v2 output**, so a frozen byte baseline moves. | Unfrozen baseline | Decoder compatibility is unaffected (any self-consistent `(r,icount)` is accepted, `src/anvil.cpp:4355`); only the encoder default changes, and that must be an explicitly authorised decision, never a silent one. Contrast `src/anvil.cpp:3903` ("NEW IDs ARE ADDED, NEVER REDEFINED"), which this does **not** require |
| AF-9 | **Silent default flip.** Reducing the aux index from ~3 KB to ~48 B is so cheap it could be mistaken for licence to make it the default. | Unfrozen baseline change | Any default change is a separate, explicitly authorised decision after SBW-G1, exactly as `docs/I10-AUX-UNBWT-RESULTS.md` §11 required for I10-1A |
| AF-10 | **Cross-run splicing.** Combining M4/M5 (run `35930672607`, subblock sweep) with M6 (run `35927623136`, reference cost) as if same-job. | Invalid evidence | Every reported figure keeps its run ID; the model in §5.3 is explicitly labelled a **fit across two recorded runs**, not a same-job measurement |

---

## 12. Recommendation

# PILOT

**Specifically: pilot the ratio-neutral, wire-neutral, cheap work. Abandon the
frontier route. Do not promote anything to a remote sweep until SBW-1 is
preregistered.**

**PILOT (do now, cheap, no production edit):**
1. **H-2 is already demonstrated offline.** Charge the checkpoint budget
   *globally*, not per piece. This is arithmetic (`selftest`, `geom`), it costs
   nothing, and it means **no future subblocked arm is measured with a knowingly
   inflated aux charge**. Highest value per unit of effort in this track.
2. **Preregister SBW-1** (§10) with the thresholds already fixed above. The
   G1 sweep needs **no subblocking at all**, so it is cheap and it is the one
   arm that could change the standing representation's wire cost by ~64×.
3. **Hand the Silesia RSS floor to tracks 05/16 as a cross-track corollary.**
   §5.3 shows the Silesia ~125 MiB floor is set by the **non-BWT** routes
   (mozilla/samba/sao/ooffice/xml, 43.2% of the corpus on Brotli q11/lw30), not
   by BWT. `M_budget(Silesia) = 108.6 MiB`, so **the entire remaining Silesia
   gap to an `IR-G6` crossing is ~15% of peak RSS on routes that have nothing to
   do with subblocking** — plausibly reachable with a bounded `lgwin` on
   `mozilla`. **This is out of my track's mandate and I am flagging it, not
   claiming it.**

**Explicitly KILL:**

* **"Subblock to meet `M_budget`."** `NO-GO-SBW-MEM` by §5.4, on both corpora,
  for a structural reason. Do not spend another subblock sweep on it.
* **Any framing of this track as an external crossing.** `0 FRONT-CROSSING`
  stands; §5.4 predicts `FRONT-GAP_COST` and §5.5 explains why.

**HOLD (pending SBW-G1/G3):**
* H-1 (walk-width saturation) — **hold**. Cheap, decisive, single-threaded, and
  it is the only hypothesis here that could make an already-adopted
  representation materially cheaper. But it is untested and my most likely
  failure (§9.2).
* H-3 (continuity-priced boundaries) — **hold pending AF-3**. Genuine
  ratio-side content, zero wire cost, but **novelty uncertified** and it must
  clear track 20's separators (§6) before it is called a mechanism.

**PROMOTE-TO-REMOTE:** nothing yet. The remote promotion is `SBW-1`, and the
correct next action is to **freeze its preregistration**, not to dispatch it.

---

## 13. Provenance

* Worktree `i10-aux-unbwt` @ `b8eae11`, left dirty and untouched. No reset,
  clean, stash, restore, rebase, commit, or push.
* **No existing file was modified.** Created: this report, and
  `prototypes/swarm-2026-10-02/07-bwt-subblocking/space-bunny/sbw_model.py`.
* **No local corpus benchmark, sweep, or fuzz campaign was run.** The only local
  execution was `sbw_model.py selftest`, a correctness self-test on inline
  synthetic strings with no corpus, no timing, and no ratio claim.
* No external temp paths were requested or used; all working files are inside
  the repository.
* Every measured figure in §1.1 retains its originating document and run ID. No
  cross-run timing comparison is asserted anywhere in this report.

---

# PART II — PAIR RECONCILIATION (Fledge Alpha Free)

**Date:** 2026-10-02 · **Adversarial input:** `docs/swarm-2026-10-02/07-bwt-subblocking-fledge.md`
**Label key:** **[M]** measured · **[S]** derived from source, arithmetic only · **[H]** hypothesis, unmeasured

---

## 14. Scope separation: fixed-cap subblocking vs the aux-wire defect

Fledge §1 is correct and I adopt it: **the whole mechanism surface of Track 07 is
`src/anvil.cpp:4446-4457`** — a byte-aligned input split plus a `0xFF` wrapper,
with no boundary search, no content-defined chunking, no scheduler. Fledge §2's
prior-art map is also correct: that knob is fully anticipated (bzip2 1996,
`--block-size` folklore, induced-sorting block locality). **I withdraw nothing
from my §5.4 negative — I restate it more sharply in §14.1.**

The coordinator's question is therefore the right one: **is my aux-wire defect
actually Track 07 work, or is it misfiled?** Answer: **it is two different
defects, and only one of them is mine.**

| Claim | Surface | Present at `B=1` (no subblocking)? | Verdict |
|---|---|---|---|
| **H-2** Θ(B) checkpoint inflation | `src/anvil.cpp:4247-4256` × `:4446-4457` | **No** — requires subblocking to multiply payloads | **Stays in Track 07.** Uniquely a subblocking accounting defect. |
| **H-1** walk-width saturation | `src/anvil.cpp:3913`, `:4247-4256`, `:4311-4318`; `libsais.c:7854-7880` | **Yes** — applies to a single unsplit BWT payload | **MISFILED. Hand to Track 18** (new decoder/backend architecture), protocol half to Track 04. |
| **H-3** continuity-priced boundaries | boundary choice inside `:4450-4453` | N/A | Track-07-resident but **demoted to diagnostic**, §14.4. |

**H-1 has no subblocking in it.** At `B=1` — where `--bwt-subblock` never fires —
the claim is unchanged: the 1024-walk target over-provisions the LF-checkpoint
index relative to the widest kernel libsais ships. Fledge independently reached
the same defect as **R3/R10** and correctly named it *"a real, fixable,
byte-negative defect."* That is **convergence, not dispute**, and I record it as
such. But Fledge assigned it to Track 07; by the track's own definition (§1 of
both reports) it belongs to decoder/backend infrastructure.

### 14.0 One correction Fledge must make before implementing anything

Fledge §8 R10 and §11 arm 3 propose the fix as *"target walks per MiB of source,
i.e. **`r ∝ cap`**"*. **As written that is a no-op: it is what the code already
does.** The encoder computes `r` from the **piece** length `C_j`
(`src/anvil.cpp:4267`, `bwt_aux_rate_for_size(n_j)`), so `r ∝ C` already holds
today. The correction is **`r ∝ n_total`** — allocate the walk budget *globally*
and give every piece the same stride. Implementing Fledge's phrasing literally
changes nothing and would produce a false null. I flag this to avoid a wasted
remote run.

---

## 14.1 The subblocking question, restated and now agreed

Both reports independently reach **KILL bounded BWT subblocking as a Pareto
lever**, and I adopt Fledge's K2. Two of Fledge's supporting arguments I accept
outright because they are stronger than mine:

* **Fledge §3.3 (new to me, and decisive):** no Silesia file exceeds 64 MiB, so
  at 64/128 MiB caps *nothing splits* and those rows are byte- and
  RSS-identical; at 32 MiB only webster splits; at 16 MiB only webster (3); at 8
  MiB only webster (5). **The entire published "+1.9346% / +1.0169% Silesia
  portfolio" is a one-file webster effect inflated by a 12-file denominator.**
  Per source byte, 16 MiB costs **+1.14% on the one file that changes**. This is
  a genuine and important correction to the track's apparent generality, and it
  strengthens the kill. **Any future report must quote per-file, never the
  portfolio ratio.**
* **Fledge §3.2:** the tax is >99.99% adaptive-postcoder warm-up plus lost
  long-range sort context; framing is ~80 B against a +1.27 MB delta on enwik8.
  **Confirmed** — and note this is the same conclusion my §4 state table reached
  from the other direction: every live postcoder is *adaptive*, so partitioning
  costs statistical warm-up, **not** retransmitted model bytes.

**One correction I return, because Fledge builds K1 on it.** Fledge §5.2 calls
the 5-point RSS series *"the single most important finding in the audit"* and
makes **K1** fire on its failure, retracting the published sentence *"8 MiB
roughly halves peak decode RSS."* Fledge's floor model (`n + 6C`, `n` = webster's
39.6 MiB) predicts 135.6 MiB at the 16 MiB cap against a measured corpus-max of
125.7 MiB.

The floor arithmetic is right; **the inference is not.** `max_peak_rss_kib` is
taken over **the whole 12-file corpus**, and five of those files
(mozilla/samba/sao/ooffice/xml — 43.2% of source) are **not BWT-routed** and
decode through Brotli q11/lw30. Fledge lists exactly this as candidate (b) at
§5.2 and then does not adopt it. Under (b), 125.7 MiB is mozilla's Brotli
ring-buffer-plus-output peak, not webster's BWT peak, and **no floor is
violated**. So:

* the series is **not invalid**;
* the published sentence *"8 MiB roughly halves peak decode RSS"* is **true as
  stated** (248.4 → 124.4 is a halving of the corpus max) and **should not be
  retracted**;
* what *is* invalid is **attributing it to the BWT subblock lever**, which is my
  §5.3 finding and Fledge's own R6.

**K1 as written would retract a correct sentence. I recommend Fledge restate K1
as: "the corpus-max RSS series must not be attributed to the subblock lever;
report per-file RSS and separate the BWT-routed from non-BWT-routed maxima."**
The *conclusion* is unaffected: Fledge's K2 (kill as a Pareto lever) and my
N-BW-1 both stand, and under (b) my version of the argument is the cleaner one —
the Silesia RSS floor is set by routes that subblocking cannot touch.

---

## 14.2 H-1 stated precisely, and relabelled **HYPOTHESIS**

The coordinator is right to demand this precision. **I cannot prove the wire
reduction without new code, so it is a hypothesis, not a result.**

### What is **[S]** (proved, source + arithmetic, no measurement)

**SBW-1a — per-`r` instruction count and memory traffic are invariant for
`blocks ≥ 8`.** Read directly from `libsais.c`:

1. `libsais_unbwt_aux` → `libsais_unbwt_main` → `libsais_unbwt_core`
   (`:8040`, `:7994`, `:7975`).
2. `libsais_unbwt_core` calls `libsais_unbwt_init_single` **exactly once**
   (`:7987`). That function computes the global histogram and the 256² bigram
   histogram over the whole `T`, then builds `P` via
   `libsais_unbwt_calculate_biPSI(T, P, bucket1, bucket2, index, **0, n**)`
   (`:7572`) — block range **`(0, n)`, the entire input, with no `r` in
   scope**. **The entire `P` construction is `r`-independent.**
3. `libsais_unbwt_decode_omp` (`:7943`) computes `blocks = 1 + (n-1)/r` and
   `remainder = n - r·(blocks-1)`. On the ANVIL path `threads == 1`, so
   `omp_num_threads = 1`, `omp_block_start = 0`, `omp_block_size = blocks`, and
   it issues **one** call to `libsais_unbwt_decode(U, P, n, r, I, …, blocks,
   remainder)` (`:7969`).
4. `libsais_unbwt_decode` (`:7882`) drains `while (blocks > 8)` eight chunks at a
   time (`:7887-7892`) and then dispatches an 8/7/…/1 tail. Chunks **tile
   `[0,n)` exactly**. Each chunk of size `k` runs the same body `k` times.
   **Total chain-body executions = `n`, for every `r`.**
5. The 8-way body (`:7867-7877`) is 8 copies of one chain step
   (`fastbits[p>>shift]` → `bucket2` scan → `p = P[p]` → `U_k[i] = c`). For any
   `r` giving `blocks ≥ 8`, the body is **byte-for-byte the same 8 copies**.

⇒ **Instruction count, `P` traffic volume, `bucket2`/`fastbits` traffic volume,
allocation count and allocation sizes are all invariant in `r` once `blocks ≥ 8`.**
Only three things change: the trip count of the outer drain loop, the eight seed
values, and the output-interleave stride.

### What is **[H]** (projected, NOT proved)

**SBW-1b — wall-clock decode is unchanged.** The seeds `i0..i7` come from `I[]`, so
changing `r` changes *which* `P`/`T` cells are touched and in what order. The
access-pattern *class* is unchanged (8 chains, `n/8` positions each, scattered),
but the specific offsets differ, so cache/TLB behaviour can differ. **I cannot
prove zero difference, only that there is no principled reason to expect a
multiplier.** This is the half of the claim that only a paired timing run can
settle, and it is the half most likely to fail.

### Exact bytes eliminated — **[H] projection, closed form**

The change is one constant: `kBwtAuxTargetWalks` 1024 → 16
(`src/anvil.cpp:3913`). Closed form: `target = ceil(n/16)`, `r = pow2 ≥ target`,
`icount = 1 + (n-1)/r`, index bytes `= 4·icount`.

| Target | n | incumbent `icount` | incumbent index B | W16 `r` | W16 `icount` | W16 index B | **bytes eliminated** |
|---|---:|---:|---:|---:|---:|---:|---:|
| dickens | 10,192,446 | 623 | 2,492 | 2²⁰ = 1,048,576 | 10 | **40** | **2,452** |
| webster | 41,458,703 | 633 | 2,532 | 2²² = 4,194,304 | 10 | **40** | **2,492** |
| enwik8 | 100,000,000 | 763 | 3,052 | 2²³ = 8,388,608 | 12 | **48** | **3,004** |
| **3-target total** | | | **8,076** | | | **128** | **7,948** |

The first three rows are **[S]** — `n` is published and my prototype's `selftest`
reproduces the incumbent `icount`/byte figures to the byte (§5.2). **The
W16 column is [H]**: it is arithmetic on an encoder change that has not been
made. Against the measured +8,083 B total (M1), the projected residual is
**~135 B (1.7%)**, i.e. **98.3% of the charged aux cost removed**.

Applying the same closed form to the other four BWT-routed Silesia files gives a
projected portfolio index charge of ~280 B against the measured **+19,344 B**
(**98.6%**), i.e. **+0.00913% → ~+0.00014% of source.** *Those four `n` values are
standard Silesia sizes not verified in-repo; the remote control must record
actuals and must not rely on this table.*

**Frontier honesty, stated before anyone is tempted:** recovering ~19 KB of the
**1,989,665 B** Silesia byte margin to xz is **0.96% of the margin**. **This is
not a Pareto move.** Its entire value is that an adopt-class representation
becomes ~98% cheaper to carry, and therefore plausibly becomes the default —
an engineering simplification, not a crossing, and **not novelty**.

### Invariants that would invalidate SBW-1

| ID | Invariant | Consequence if broken |
|---|---|---|
| **INV-1** | **Thread ceiling.** `libsais_unbwt_decode_omp` sets `max_threads = min(blocks, threads)` (`libsais.c:7950`, guarded `if(max_threads > 1 && n >= 65536)`). Dropping `icount` 763 → 12 **drops the per-payload thread ceiling from 763 to 12**. | **HARD SCOPE LIMIT: W16 is valid only for single-threaded decode.** If OpenMP BWT decode is ever adopted, W16 caps intra-payload parallelism at 12 workers. Must be stated in any promotion. Interacts with H-2. |
| **INV-2** | Seed/cache equivalence (`[H]`, §14.2) | Speed regression; the byte win stands but the "free" framing collapses |
| **INV-3** | `r ≥ 2` floor (`src/anvil.cpp:4252`) makes both policies **identical for `n ≤ 2048`** | W16 is a no-op on small blocks; harmless, but do not report small-block deltas as evidence |
| **INV-4** | `libsais.c:8053` validates `icount` entries: 763 checks → 12 | Biases *in favour of* W16 by ~750 integer compares against seconds. Disclose so it is never misread as a confound in the other direction |
| **INV-5** | Canonical-byte drift. Decoder compatibility is total (`src/anvil.cpp:4355` accepts any self-consistent `(r,icount)`), but the **encoder's** canonical v2 output moves. | Frozen baselines shift → explicit authorisation required; cf. `docs/I10-AUX-UNBWT-RESULTS.md` §11, which declined exactly this for I10-1A |

---

## 14.3 The one remote check — **SBW-B1**, byte-only, paired

**Deliberately byte-only.** No timing, no RSS. This is a direct response to
Fledge §9, which destroyed the previous sweep's evidence on exactly those two
axes: single-shot cross-job RSS with no error bars, and an un-decomposed byte
column. SBW-B1 needs neither, so neither can contaminate it.

**Setup.** One isolated prototype binary exposing `--bwt-aux-walks={1024,16}`,
nothing in production touched. Flags identical to the closed sweep so the control
must reproduce published numbers:
`--parse=ratio --ratio-backend=bwt --ratio-context=off --ratio-lines=off`.
Files: the 7 BWT-routed Silesia files + enwik8. **Prepare each payload once per
arm; decode byte-exactly; verify output SHA-256 outside any timed region.**

**Arms:** `W1024` (control) and `W16` (candidate). Two arms, one variable.

**Mandatory columns — this is also the deliverable that closes Fledge §4's named
gap** ("no published artifact separates aux-index bytes from postcoder warm-up
bytes"): `r`, `icount`, `aux_index_bytes`, `0xFE` header bytes, `primary_bytes`
(`0` for v2, `uvar` for v1 — per S3), `postcoder_bytes`, `0xFF` framing bytes,
`subblock_count`, `complete_wire_bytes`, `roundtrip`, `output_sha256`.

| Gate | Condition | If failed |
|---|---|---|
| **B1-G0 identity** | `W1024` reproduces the **published** per-file aux deltas exactly: dickens **+2,494**, webster **+2,535**, enwik8 **+3,054**, Silesia portfolio **+19,344** | `MODEL-MISMATCH` — **stop**. The prototype binary is not validated and no other gate may be read. |
| **B1-G1 decomposition** | The `W1024 → W16` byte delta is accounted for **100%** by `aux_index_bytes` plus `uvar` length changes in `r`/`icount`. Exact, not 99%. | **The closed form is wrong.** Record as `MODEL-MISMATCH`; do not reinterpret. |
| **B1-G2 the claim** | `aux_index_bytes(W16) ≤ 64` on **every** file, **and** `complete_wire_bytes(W16) < complete_wire_bytes(W1024)` on **every** file | SBW-1b (byte half) **falsified** |
| **B1-G3 roundtrip** | Every arm decodes byte-exactly to the canonical source SHA | `NO-GO`; no interpretation |

**What SBW-B1 can and cannot settle.** It settles the **byte half** of H-1 and
produces the missing decomposition. It **cannot** settle SBW-1b (wall-clock
equality) — that needs a paired-timing arm with A/A nulls and an ambient gate,
which is **Track 18's** experiment, not this one. **After a full SBW-B1 pass,
H-1's status is "byte half confirmed, speed half untested."** It remains a
hypothesis. I will not describe it as a result before the timing arm exists.

---

## 14.4 H-3 is demoted — concession to Fledge

Fledge §3.2/§3.3 break the part of H-3 I cannot defend:

* the tax is **postcoder warm-up**, which is placement-**invariant** by
  construction — N adaptive restarts cost what they cost regardless of where you
  cut, so no boundary rule can touch the dominant term;
* the residual sort-context term is confined to **webster alone** (§14.1), so the
  corpus over which H-3 would be measured has almost no signal;
* and the population where boundary placement *should* pay — record-structured
  data with heavy local variation in reference-span load — is **largely not
  BWT-routed**. BWT routes on dickens/webster/nci/osdb/reymont/mr/x-ray; the
  project's JSON family is not among them.

**Disposition: H-3 is demoted from mechanism candidate to cheap diagnostic.** The
`damage` subcommand in the prototype stays, but as a *corpus-characterisation
probe* whose only job is to report whether `Damage` has usable variance at all.
If `μ_min/μ > 0.9` on the BWT-routed files, H-3 is dead on arithmetic and no
preregistration is warranted. **I withdraw H-3 from the mechanism lane.** Fledge
is right that "boundaries are already byte-aligned and placement is therefore not
a free variable" in the sense that matters here.

---

## 14.5 I also accept Fledge's zero-byte engineering, with a routing caveat

Fledge §5.1 item 2 / R2 (`sub` is a pure double buffer,
`out.insert(out.end(), sub…)`, `src/anvil.cpp:4489`) and §10 Alternative M
(stream, never materialise) are both real and both **byte-neutral**. Accepted.

But for scope hygiene: eliminating the `sub` copy requires changing
`bwt_backend_decode` to write into a caller-supplied buffer instead of returning
by value — **a decoder-infrastructure change**, not a Track-07 knob. And
Alternative M attacks the `n` accumulator, which is the one term no subblock cap
can touch — but it is a **streaming** change, which is exactly the boundary
`RANDOM_ACCESS_NOT_CLAIMED` (M8) and Fledge's own §10 falsification criteria
guard. **Alternative M belongs to Tracks 18/19, and any result from it must not
be described as random access or seek support.**

---

## 14.6 Revised final recommendation

# PILOT — but the pilot has changed shape, and it is now Track-18 work

| Item | Lane | Disposition |
|---|---|---|
| Subblocking as a Pareto lever | 07 | **KILL.** Agreed with Fledge K2. My N-BW-1 and Fledge's K2 stand; Fledge's per-file correction makes it stronger. |
| Silesia portfolio ratios | 07 | **CORRECTION REQUIRED.** Any future citation must be per-file. The published portfolio figures are a webster effect. |
| K1 (retract published RSS sentence) | — | **I dispute as written** (§14.1): the sentence is a true corpus-max statement. Restate as "do not attribute the corpus max to the subblock lever." |
| **H-2** global checkpoint budget | 07 (low priority) | **PILOT, arithmetic only.** Fix `r ∝ n_total`, not Fledge's `r ∝ cap`. Value is honesty of future subblocked arms, **not** Pareto. Corrects the published curve's pessimism. |
| **H-1** walk-width saturation | **18** (protocol → 04) | **HAND OFF as a hypothesis.** Run **SBW-B1** (§14.3) byte-only. **Explicitly not novelty, explicitly not a Pareto move** — it is engineering cleanup of an adopt-class representation. |
| **H-3** boundary placement | 07 | **KILL as mechanism; retain as diagnostic** (§14.4). |
| Alternative M / in-place decode | **18/19** | Out of my lane; flagged. Streaming claims prohibited. |
| Non-BWT RSS floor on Silesia | **05/16** | Still flagged from §12: the remaining Silesia gap to an `IR-G6` crossing is ~15% of peak RSS on routes subblocking cannot touch. |

**The honest one-line summary:** the only thing in this track worth spending money
on is a **two-arm byte-only check of an encoder default constant**, it belongs to
another track, and if it passes it makes an existing representation ~98% cheaper
to carry without moving the Pareto frontier by even 1% of the byte margin. **That
is worth knowing and is not worth a mechanism claim.**
