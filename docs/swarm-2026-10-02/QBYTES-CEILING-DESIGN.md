# QBYTES — ceiling design and ruling: threshold-free exact byte accounting

**Date:** 2026-10-02
**Type:** red-team evaluation of the `CLOSEOUT-MATRIX-REDTEAM.md` §5.1 `QBYTES` proposal. Additive.
**This document authorizes nothing.** It contains no production edit, no coordinator edit, no codec
execution, no benchmark, no network, and no git write.

**Coordinator ruling under evaluation:** do not freeze the proposed `2 %` mask-share and `0.1 %`
aux-share thresholds. Replace them with threshold-free exact decision leverage — for each lever
compute the idealized maximum removable bytes and compare against the frozen decision-relevant byte
bar for that lever's exact corpus. A lever that cannot reach any frozen byte bar even under its ideal
ceiling is **CLOSED as frontier work**; engineering value alone does not keep it funded.

> **Revision note (2026-10-02, second pass).** §3's `47,093 B` "ideal mask-replacement ceiling" and
> all `13/13` Q5 kill claims derived from it are **RETRACTED as mathematically invalid**. See §3.1
> for the error and §3.4 for the corrected disposition. **Q5 is HOLD / DEFERRED, not closed.** Q9's
> closed-form `22,024 B` result and the `19,328` / `19,344 B` reconciliation are unaffected and
> stand as written in §2.

---

## 0. Evidence classes used here

| label | meaning |
|---|---|
| `[PROVEN]` | read directly from primary source or format spec in this tree at the cited line; reproducible by reading |
| `[MEASURED]` | recorded in a named retained artifact; not re-executed here |
| `[DERIVED]` | arithmetic over `[PROVEN]`/`[MEASURED]` inputs; every input shown |
| `[UNKNOWN]` | not determined by any retained artifact |

No `[PROJECTED]` value is used in any gate below.

---

## 1. Both proposed percentages are deleted as gates

These two arguments are structural and **do not** depend on the retracted §3 ceiling.

### 1.1 The `0.1 %` aux threshold is not well-formed `[DERIVED]`

It has no stated denominator, and the two admissible denominators in the retained record give
**opposite verdicts** on the same charge:

| denominator | `19,328 B` reads as | vs `0.1 %` |
|---|---:|---|
| Silesia source `211,938,580 B` | `0.00912 %` `[MEASURED]` `RESEARCH_LEDGER.md:5069` | **fires** ⇒ cancel |
| xz byte margin `2,009,105 B` | `0.962 %` `[MEASURED]` `RESEARCH_LEDGER.md:5070` | **does not fire** |

`04-dense-frontier-fledge.md:591-592` already observed the shape problem ("different quantities");
`07-bwt-subblocking-fledge.md:584` uses a third denominator (`0.042 %`). A threshold whose verdict
flips on an unstated choice of denominator is not a threshold. **Deleted.**

### 1.2 The `2 %` mask threshold is non-discriminating by construction `[PROVEN]`

The correction mask is **not an entropy-coded stream**. `FORMAT.md:580` and `FORMAT.md:868` specify it
as raw flat 32-bit words:

```
S_mask(block) = SUM over type-2 tokens t of  4 * ceil(len_t / 32)     bytes      [PROVEN]
```

Two exact consequences:

- `S_mask >= P2 / 8`, where `P2 = SUM len_t` is total sparse-corrected phrase length. A `4 B` charge
  per `32 B` of phrase is a flat **one-eighth of every type-2 phrase**.
- `S_mask >= 4 * N2`, where `N2` = type-2 token count, since `len_t >= 4` implies
  `ceil(len_t/32) >= 1`.

Therefore `mask_share = S_mask / C >= P2 / (8 C)`. For a `< 2 %` gate to fire, type-2 phrase bytes
must be under `16 %` of the container — i.e. sparse-corrected matching must be essentially unused.
The gate therefore cannot separate *"the mask wire is small"* from *"there is no sparse-matching
mechanism in play."* As a pre-gate it is **non-discriminating and therefore invalid**, independent of
any ceiling claim. **Deleted.**

> This is a statement about the gate, not about Q5's value. §3.4 keeps Q5 open on that distinction.

---

## 2. Q9 — aux-index `W1024` vs `W16`: exact absolute bytes, closed form from source

### 2.1 The charge is a closed-form integer function of block length `[PROVEN]`

`kBwtAuxTargetWalks = 1024` (`src/anvil.cpp:3913`). Rate and count are pure integer arithmetic with no
data dependence (`src/anvil.cpp:4247-4256`, `:4268`):

```
target(W,n)  = ceil(n / W)
r(W,n)       = 2 ^ ceil(log2(target(W,n)))            smallest power of two >= target
icount(W,n)  = 1 + floor((n-1) / r(W,n))
```

Serialisation, `src/anvil.cpp:4314-4318` (aux) vs `:4321-4322` (legacy v1):

```
aux=on :  tag(1) + post(1) + uvar(r) + uvar(icount) + 4*icount
aux=off:  post(1) + uvar(primary)
```

`uvar` is LEB128 (`src/anvil.cpp:289-291`); `uvar_len(x) = floor(log2 x / 7) + 1` for `x >= 1`.
The **exact container-byte delta** for one BWT block of length `n` is

```
Delta(W,n) = 4*icount(W,n) + uvar_len(r) + uvar_len(icount) + 1 - uvar_len(primary)
```

`primary` is the BWT primary index of the string — a function of the input only, not of `W`. It is the
sole term not in closed form, and it is pinned by measurement (§2.3).

### 2.2 The Q9 prize cancels that term exactly `[DERIVED]`

Q9 changes `W` and nothing else. `n` and `primary` are identical in both arms, so `uvar_len(primary)`
**cancels exactly**. The removable bytes are fully closed-form:

```
Q9_prize(n) = 4*(icount(1024,n) - icount(16,n))
            + (uvar_len(r(1024,n)) - uvar_len(r(16,n)))
            + (uvar_len(icount(1024,n)) - uvar_len(icount(16,n)))
```

| file | `n` | `r(1024)` | `icount(1024)` | `r(16)` | `icount(16)` | **Q9 prize (B)** |
|---|---:|---:|---:|---:|---:|---:|
| dickens | 10,192,446 | 16,384 | 623 | 1,048,576 | 10 | **2,453** |
| mr | 9,970,564 | 16,384 | 609 | 1,048,576 | 10 | **2,397** |
| nci | 33,553,445 | 32,768 | 1,024 | 2,097,152 | 16 | **4,032** |
| osdb | 10,085,684 | 16,384 | 616 | 1,048,576 | 10 | **2,425** |
| reymont | 6,627,202 | 8,192 | 809 | 524,288 | 13 | **3,184** |
| webster | 41,458,703 | 65,536 | 633 | 4,194,304 | 10 | **2,492** |
| x-ray | 8,474,240 | 16,384 | 518 | 1,048,576 | 9 | **2,037** |
| **Silesia 7-file** | | | | | | **19,020** |
| enwik8 | 100,000,000 | 131,072 | 763 | 8,388,608 | 12 | **3,004** |
| **total** | | | | | | **22,024** |

`icount(1024)` reproduces `RESEARCH_LEDGER.md:5057-5064` exactly (623/609/1024/616/809/633/518,
total `19,328 B` of payload) — an independent check that the closed form is the shipped policy.

**Absolute prize: `22,024 B`, of a currently charged `22,398 B`.** No percentage is reported, per the
standing instruction to express this lever in absolute bytes only.

### 2.3 Reconciliation of the `19,328` vs `19,344 B` Silesia discrepancy `[DERIVED]`

The two published figures are **different quantities**, and the difference is fully accounted:

| term | value | basis |
|---|---:|---|
| `SUM 4*icount(1024,n)` — index payload only | **19,328 B** | `[MEASURED]` `RESEARCH_LEDGER.md:5066`, PR-4 §3d |
| `SUM [1 + uvar_len(r) + uvar_len(icount)]` — framing added | **+48 B** | `[DERIVED]` 7 files; per-file 7,7,7,7,6,7,7 |
| `SUM uvar_len(primary)` — legacy v1 varint replaced | **−32 B** | `[DERIVED]`, pinned by the measurement below |
| `SUM Delta(1024,n)` — true container-byte delta | **19,344 B** | `[MEASURED]` `docs/I10-AUX-UNBWT-RESULTS.md:397` |

`19,328 + 48 − 32 = 19,344`. **The discrepancy is exactly the `+16 B` of net per-file framing.** No
arithmetic error exists in either figure; the ledger prices the index payload, the I10 integration
record prices the container delta.

The implied `SUM uvar_len(primary) = 32` is independently consistent per file:

| file | payload | `Delta` closed form | measured | implied `uvar_len(primary)` |
|---|---:|---|---:|---:|
| dickens | 2,492 | `2,498 − L` | +2,494 | 4 |
| webster | 2,532 | `2,538 − L` | +2,535 | 3 |
| enwik8 | 3,052 | `3,058 − L` | +3,054 | 4 |

Enwik8 closes the portfolio: `19,344 + 3,054 = 22,398 B` `[MEASURED]`
`docs/I10-AUX-UNBWT-RESULTS.md:437`.

**Ruling: `19,328 B` is the payload convention and `19,344 B` is the container convention. Both are
correct. For gating, the closed form `Delta(W,n)` supersedes both, and the Q9 prize is `22,024 B` with
no residual term.**

### 2.4 Q9 exact decision test `[DERIVED]`

Q9's exact relevant corpus is the I10-1A BWT candidate (Silesia + enwik8) — aux is reachable only
under `parse=="ratio"` (`src/anvil.cpp:4667`), so the Class-A ladder is not in scope. The frozen byte
bars on that corpus `[MEASURED]` `docs/anvil-i9-findings.md:489-491`:

| bar | ANVIL | reference | ANVIL margin |
|---|---:|---:|---:|
| Silesia vs `xz -9e` | 46,446,995 | 48,456,100 | **+2,009,105 ahead** |
| Silesia vs `brotli q11/lw30` | 46,446,995 | 49,383,136 | **+2,936,141 ahead** |
| enwik8 vs `xz` | 23,534,368 | 24,831,656 | **+1,297,288 ahead** |
| enwik8 vs `brotli q11/lw30` | 23,534,368 | 24,810,180 | **+1,275,812 ahead** |

**There is no byte deficit on Q9's corpus. Every bar is already won.** The lever can only widen an
existing margin, and doctrine 10 (`COORDINATOR-CLOSEOUT-v1.md:18`) holds that a byte-only win is not a
`FRONT-CROSSING`. The smallest bar on the corpus is `1,275,812 B`; the ideal ceiling is `22,024 B`.

Unlike Q5, this bound **is** valid: the lever's entire output difference is its own serialised field,
which §2.2 computes exactly. No other quantity is in play.

### 2.5 Q9 ruling

> **Q9 — CLOSED as frontier work.** Reached by exact arithmetic over retained bytes and one source
> constant. No threshold. No runner. The absolute prize is `22,024 B` against already-won bars, and
> the trade direction is adverse: `W1024 → W16` means fewer checkpoints and more re-scan per walk,
> which risks trading the banked `1.364×/1.795×/2.339×` decode win (`README.md:19`) for `22,024 B`
> on the binding decode axis. Engineering tidiness does not fund it.

---

## 3. Q5 — mask ceiling: RETRACTED as an arithmetic kill; disposition HOLD / DEFERRED

### 3.1 The retracted claim and why it was invalid

The withdrawn argument asserted a *prior* bound:

> A mask-replacement mechanism cannot change the parse, so its output cannot beat what a mask-free
> configuration already emits; hence `Q5_ceiling(f) = X_f - M_f` upper-bounds the prize.

**This is false.** `M_f` is the minimum over mask-free configurations — `anvil-greedy-*`,
`anvil-dp-*`, `anvil-mdl-*` — which are all **exact-only** parses with **no sparse-correction
representation at all** (`src/anvil.cpp:2161` lists exactly `{types, ll, ml, ds, lits}`, no mask
stream). A sparse parse can have genuinely better match structure than any exact-only parse. Removing
or re-coding only a mask-bearing parse's mask overhead, **holding that parse fixed**, can absolutely
produce output below `M_f`. `X_f − M_f` is therefore the prize of *"make the mask-bearing row tie the
best mask-free row"* — a floor-like reference point, **not an upper bound**.

The retained ladder refutes the bound on its own data. On two files the mask-bearing row is
**already better than every mask-free row**:

| file | `M_f` (mask-free min) | `X_f` (mask-bearing min) | `X_f − M_f` |
|---|---:|---:|---:|
| generated.log | 132,434 | **129,739** | **−2,695** |
| synth-timeseries.bin | 155,739 | **140,898** | **−14,841** |

`generated.log` `anvil-shape-rans` is the sole `FRONT-GAP` class row in the frozen grid
(`Q0-DENSE-RETAINED-RESULT.md:40-42`). A mask-bearing configuration already sits ahead of every
mask-free configuration there. Any claim of the form "a mask mechanism cannot do better than
mask-free" is contradicted by the frozen artifact.

**Retracted:** the `47,093 B` figure, the `B_f < D_f` `13/13` result, and the arithmetic Q5 kill.
Retained as descriptive only, not as a bound: mask-free ladder total `2,424,640 B`, mask-bearing
ladder total `2,471,733 B`, difference `47,093 B`, with mask-free strictly better on 9 of 13 files,
tied on 2, worse on 2.

### 3.2 What the correction does *not* retract

The following remain valid and are independent of the invalid bound:

- **Mask-bearing classification `[PROVEN]`.** `anvil-sparse-*` (mode 11 `S5`, `FORMAT.md:868`),
  `anvil-shape-*` (mode 12 stream 7, `FORMAT.md:580`), `anvil-tcopy-*` (mode 14), and
  **`anvil-hotop-*`** (mode 15 `mmasks`/`mresid`, `src/anvil.cpp:2824`). This corrected a first-pass
  error that had placed the three `hotop` rows in the mask-free set.
- **`S_mask >= P2/8` and `S_mask >= 4*N2` `[PROVEN]`** (§1.2). Pure structure, no ceiling claim.
- **Decoder coupling `[PROVEN]`.** `len(residuals) == popcount(mask)` is a decoder invariant
  (`FORMAT.md:881`), so a mask bit cannot be dropped without either losing the correction or paying
  for it elsewhere.
- **Threshold deletion.** §1.1 and §1.2 stand on their own.
- **Frozen bar `D_f` `[DERIVED]`.** `D_f = min ANVIL row − min reference row` over
  `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` is the correct decision bar and remains the
  quantity any future Q5 result must be compared against. Aggregate `D = 705,385 B`.

### 3.3 What is genuinely unknown

| quantity | status |
|---|---|
| actual correction-mask wire bytes, in isolation | **`[UNKNOWN]`** — never measured. Track-10 and `REMOTE-EXPERIMENT-QUEUE.md:88` both list it as the thing to be *measured*, i.e. it is not on record. |
| `P2`, `N2` per block | **`[UNKNOWN]`** — not emitted by any retained artifact. |
| strict coding ceiling on the mask stream | **`[UNKNOWN]`** — requires the mask stream's own length and symbol distribution, neither retained. |
| mask uniqueness / top-32 coverage / `(k,slot)` modal accuracy | `[MEASURED]` `FORMAT.md:554-558`: masks **~90 % unique**; top-32 masks cover **7–12 %** of type-2 tokens; modal accuracy **17–23 %** (not the Linux 86.5 %). |
| mode 13 — the one measured mask-model implementation | `[MEASURED]` `FORMAT.md:554-558`: **lost +13.5 %…+21.0 %** on every record file. |

The recorded uniqueness/modal statistics constrain the *achievability of a mask model*, but they do
**not** determine the ceiling, because the ceiling is bounded by the mask stream's actual length,
which nobody has measured. Quoting `7–12 %` as if it were the ceiling — as the fledge review's
`< 2 %` pre-gate implicitly did — repeats the same category error in the opposite direction.

### 3.4 Q5 ruling

> **Q5 — HOLD / DEFERRED. Not dispatched standalone.**
>
> The actual mask wire is unmeasured and the strict coding ceiling is undetermined from retained
> artifacts, so the coordinator's exact-leverage test **cannot currently be applied to Q5**. That is
> a statement of ignorance, not a kill.
>
> Under the program's high-information / Pareto doctrine, a standalone dispatch is not justified:
> `authorizes_prototype: false` is unconditional; the one measured implementation of the mechanism
> lost `13.5–21 %` on every record file; and the only field it would add is a length and ceiling for
> a stream whose *consumers* are already recorded as choosing mask-free configurations on 11 of 13
> files. Its information value per runner is low relative to Q7 (largest measured byte EV, no decode
> work) and Q8 (zero upstream cancel conditions).
>
> **Condition for revival, cost-free only:** Q5 may ride along on an already-authorized byte-only
> instrument that is already emitting per-block framing and per-stream byte columns — the Q2F fidelity
> arm, or Q4a's byte-only cross-product, both of which already owe such columns under `C7.2`. It must
> **not** be given a runner of its own, and it must not be sequenced ahead of an authorized job. If no
> such host is dispatched, Q5 stays deferred indefinitely; an indefinitely-deferred lane must be
> recorded as deferred, not as closed.
>
> `authorizes_prototype: false` remains **unconditional** in any such piggyback row.

---

## 4. Q2 geometry census — free, and its own thresholds do not fire

Not a byte lever, but it is the third item COLLAPSE-1 bundled into `QBYTES`. It is closed-form
arithmetic over the retained manifest, so it is not a job either.

```
nblocks(f) = ceil(input_bytes(f) / 262144)              [block-size regime, src/anvil.cpp:4999-5000]
```

| census | count |
|---|---:|
| `nblocks = 1` | **4** (`synth-arith.bin`, `src.cpp`, `doc.md`, `random.bin`) |
| `nblocks <= 2` (geometry-invariant control) | **6** |
| `nblocks >= 4` (informative) | **7** |
| `nblocks >= 8` | **2** |

This independently reproduces the split cited in `CLOSEOUT-MATRIX-REDTEAM.md` §X4 (7 informative /
6 control).

Both of the proposal's own Q2 thresholds **do not fire**:

- `C2.4` (`>= 8 of 13` single-block ⇒ downgrade): actual is **4**. Does not fire.
- `C5.3` (informative `<= 4` ⇒ cancel): actual is **7**. Does not fire.

### 4.1 Structural parallel capacity only

The decoder sets `nthreads = min(decode_threads, nblocks)` (`Q2-PARITY-PLAN-CRITIC.md` §3.6(c)). So
**structural parallel capacity** — the maximum concurrency the block count admits, independent of any
timing measurement — is:

| arm | block size | capacity 1 | capacity 2 | capacity >= 4 |
|---|---:|---:|---:|---:|
| G0/G2 | 256 KiB | 4 files | 2 files | 7 files |
| G3/G1 | 64 MiB | **13 files** | 0 | 0 |

Every corpus file is smaller than 67,108,864 B (largest is `generated.jsonl` at 2,815,267 B), so the
64 MiB arm is single-block on 13 / 13 and admits capacity 1 everywhere. The 256 KiB arm already admits
capacity 1 on 4 files. **Nine files therefore lose all structural parallel capacity in the geometry
arm.** No throughput, regression, or speedup magnitude is claimed or implied here — this is a
structural property of the block count under a pinned formula, and any timing consequence requires its
own instrument and attestation (C4.4).

---

## 5. Required inputs

Inputs needed for the rulings actually reached:

| input | location | class |
|---|---|---|
| `compressed_bytes` for all 364 rows | `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` | `[MEASURED]` |
| `input_bytes` per canonical file | same CSV, column 2 | `[MEASURED]` |
| config → parse-mode map | `tools/bench_native.cpp:57-74` | `[PROVEN]` |
| mask-bearing vs mask-free stream sets | `src/anvil.cpp:2161`, `:2824`; `FORMAT.md:580,868,881` | `[PROVEN]` |
| aux rate + count policy | `src/anvil.cpp:3913`, `:4247-4256`, `:4268` | `[PROVEN]` |
| aux serialisation | `src/anvil.cpp:4314-4322`; `put_uvar` `:289-291` | `[PROVEN]` |
| aux measured delta | `RESEARCH_LEDGER.md:5050-5067`; `docs/I10-AUX-UNBWT-RESULTS.md:397,437` | `[MEASURED]` |
| Silesia / enwik8 byte bars | `docs/anvil-i9-findings.md:489-491` | `[MEASURED]` |
| block-size regime | `src/anvil.cpp:4999-5000` | `[MEASURED]`/`[PROVEN]` |

Inputs that Q5 would need and that **do not exist** (§3.3): per-block mask-wire bytes, `P2`, `N2`,
and the mask-stream symbol distribution. Absent, not merely unmeasured locally.

---

## 6. Recommendation

| item | recommendation | cost |
|---|---|---|
| **QBYTES as a remote job** | **DO NOT DISPATCH.** Of its three requested outputs, two (Q9, Q2 census) are closed-form from retained bytes and one (Q5) is not measurable from retained artifacts at all. A job cannot report a quantity no instrument on record produces. | 0 |
| `2 %` mask-share threshold | **DELETED** — non-discriminating by construction (§1.2). | 0 |
| `0.1 %` aux-share threshold | **DELETED** — not well-formed; verdict flips on an unstated denominator (§1.1). | 0 |
| **Q9** | **CLOSED as frontier work** — `22,024 B` prize (exact closed form), no deficit on its corpus, adverse trade direction (§2.5). Absolute bytes only. | 0 CI |
| **Q5** | **HOLD / DEFERRED** — mask wire unmeasured, ceiling undetermined. Not killed, not funded. No dedicated runner; cost-free piggyback on Q2F or Q4a only (§3.4). `authorizes_prototype: false` unconditional. | 0 CI |
| Q2 census | **RECORD as a deterministic fact** (§4). Replace `C2.4`/`C5.3` with the 7-vs-6 statement; add the structural parallel-capacity table (§4.1) to Q2's non-classifying disclosure. | 0 CI |
| `19,328` vs `19,344 B` | **RECONCILED** — net `+16 B` framing; payload vs container conventions (§2.3). | 0 |
| Per-block framing bytes (Q4a `C7.2`) | Not closed-form — per-block container framing is not a function of `n`. Leave inside Q4a's existing byte-only arm. | deferred to Q4a |

**Net effect on the queue:** one proposed pre-gate (Q9) is answered exactly at zero CI. The other
(Q5) is shown to be **currently undecidable** rather than decidable — which is a different and weaker
result than the red-team proposal assumed, and must not be recorded as a kill. Q5 must be carried as
`HOLD / DEFERRED`, matching the treatment of an indefinitely-blocked lane elsewhere in the matrix.

**Superseded.** `CLOSEOUT-MATRIX-REDTEAM.md:459` (`C2.1`, the `2 %` mask pre-gate) and its §5.1 note
at `:352-354` and `:672-674` are superseded by this document. `C2.3`'s direction is confirmed but its
`0.1 %` instrument is replaced by the exact closed form in §2. That file is coordinator-owned and is
left unmodified; this ruling is additive and names what it overrides.

---

## 7. Claims hygiene

- Nothing was executed. Only read-only file reads, source/format inspection, and integer arithmetic
  over `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` and the retained ledger tables.
- No codec, benchmark, fuzz, build, or network. No production file, source, ledger, queue, or
  coordinator file was modified. The only file written is this one.
- **Retraction record.** Three claims are withdrawn as invalid and must not be cited:
  1. `Q5_ceiling(f) = X_f − M_f` as an upper bound on removable mask bytes.
  2. The `47,093 B` aggregate "ideal mask-replacement ceiling."
  3. The `B_f < D_f` `13/13` result and the arithmetic Q5 kill derived from it.
  The cause is stated in full in §3.1. The `64,168 B` first-pass figure is separately withdrawn as a
  classification error (`anvil-hotop-*` mis-placed in the mask-free set) and is superseded by neither
  retracted nor retained figure — both are void.
- The mask-bearing ladder totals in §3.1 are retained **as descriptive ladder facts only**, carrying no
  bound.
- §4.1 reports structural parallel capacity only. The earlier draft's characterisation of parity as a
  "decode regression of order core-count" was an unmeasured performance claim and has been removed.
- No `[PROJECTED]` value appears in any gate. `SYNTH-MEASUREMENT-FLEDGE.md:483`'s "2 % practical-effect
  epsilon" is a comparison figure, not a bar, and is not used here.