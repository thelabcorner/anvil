# Q0c — Adversarial Audit of the λ / c Units and the J Objective

**Lane:** Q0c independent audit (adversarial). **Date:** 2026-10-02.
**Tree:** `C:\Users\slooshied\Documents\ANVIL` @ `b8eae11`.

**Independence statement.** This audit was written without reading the constructive
lane's *conclusions* before doing its own arithmetic, and it does not adopt them. Where
this document agrees with the constructive lane it is because the arithmetic forces
agreement, not because it was checked against. Where it dissents, it dissents on
recomputation from source.

**Method and prohibitions observed.**
- **No codec or corpus was executed.** No `anvil.exe`, no `anvil_bench`, no `bench_suite.py`,
  no fuzz, no timing of anything. Every number below is either read out of source/doc
  text or is arithmetic performed in a PowerShell session on those read values.
- **No network.**
- **No production edit.** `src/anvil.cpp`, `FORMAT.md`, `RESEARCH_LEDGER.md` untouched.
- **No git operation of any kind** — no commit, push, branch, reset, or stash.
- **Only file written by this audit:** this document.

**Units convention used throughout (per coordinator steer).** The frozen native unit
is **ns/byte**. Therefore `c` is published in **ns/B** and λ in **bytes/ns**:

```
lambda = 0.01 bytes/us = 1e-5 bytes/ns        1 byte  ==  100,000 ns  ==  100 us of decode
```

Cycles/B is **not** used: no pinned clock was consulted and it is not required.
All magnitudes below are absolute-time and therefore host-independent as written.

---

## 0. Rulings (up front)

| # | question put to Q0c | ruling |
|---|---|---|
| R1 | Is there one `J` in this codebase? | **No. There are three selectors with three different policies**, all reachable from shipped flags. `src/anvil.cpp:1548` (flat constant `c`, 10 call sites, **default**), `:1628` (size-proportional ns/B, 1 call site, flag-gated off), `:1587` (pure min-`L`, J ignored, 5 call sites). |
| R2 | Is `c` a total decode time or a per-byte budget? | **Neither, as documented.** `FORMAT.md:642` labels `c` a *per-byte* cost unit; the code applies it as a **per-stream total** with **no size dependence**; and the integer values are reconcilable with the per-byte table at **no single stream length** (§3.3). |
| R3 | Dimensional consistency of `J = L + λ·c` | **Fails on the default path.** `λ` is dimensionless there; the `λ·c` term has no length unit and cannot be summed with `L`. It is dimensionally valid **only** on the flag-gated budget path. |
| R4 | Hidden scaling factor | **Found, and it is the central defect.** The flat table is exactly the size-proportional objective evaluated at a hard-wired stream length. Against the *measured* table that length is not unique — it ranges **5.0 KB – 100 KB (20× spread)**. Consequence: the default path under-weights decode time by **2.6× (raw) to 51× (Huffman)** at a full 256 KiB block, and *over*-weights it by ~6.7× at 1 KB. |
| R5 | Can λ = 0.01 be interpreted consistently? | **No.** The same numeral carries two incompatible meanings. On the budget path it is a real exchange rate (1 B ≙ 100 µs). On the default path it is an **ad-hoc weight on a dimensionless count**, and its entire dynamic range is **0.35 bytes**. |
| R6 | Is λ = 0.01 economically meaningful? | **It is degenerate.** It prices 1 byte of compressed size at 100 µs of decode — at the project's own measured rates that is **1 B of size ≙ ~16,700 B of decode throughput**, a ~4½-order-of-magnitude statement that bytes are free next to time. The objective is length minimisation plus a sub-byte tie-break. |
| R7 | Is the recorded "J-agreement 100%, ≥80 % bar cleared" evidence? | **No. It is a theorem about the code, not a measurement.** Proven in §5: for any λ ≤ 1/35 = 0.0222 the reported metric **cannot** return anything but 100 %. λ = 0.01 sits inside that region with a factor 2.22 of margin. The gate was unfalsifiable as run. |
| R8 | Is the historical v1→v2 J-form change post-hoc tuning? | **In effect, yes — and it is checkable without any allegation of intent.** The change moved the decode term from **16 % of `L`** (multiplicative λ=0.04) to **≤0.35 B absolute** (additive λ=0.01), a ~3–4 order-of-magnitude weakening that moved the result from **76.7 % (FAIL vs the ≥80 % bar)** to **100 % (PASS)**. Compounding this: **both artifacts said to define the contract are absent from the tree** (§6.2). |
| R9 | Is the constructive lane's surviving deliverable (FORMAT.md unit/doc fix) post-hoc? | **No — explicitly cleared.** It is documentation-only, changes no byte and no λ/c value, and is correctly self-labelled *adopt-class*. This audit raises **no** objection to it. §7.4 warns about the one way it could be turned post-hoc. |
| R10 | **Is Q3's existing A/B scientifically interpretable?** | **NO.** See §8. It is interpretable **only** as a byte-safety / timing-neutrality equivalence check, and that question is already closed by retained evidence. As a test of the decode-cost objective it has **no discriminating power**, and that is provable before it runs. |
| R11 | Minimum redefinition | Stated in §8.3. Minimum: **one variable, not three; λ raised to a pre-registered threshold; tie-break frozen identically across arms; per-candidate `L_c` and `n` logged; falsifier pre-registered.** The existing cell is fine — the defect is the objective's *inertness*, not the cell. |

---

## 1. What is actually in the tree: three selectors, not one

Reconstructed from source, not from documentation.

### 1.1 Selector A — `encode_stream` (DEFAULT; flat constant `c`)

`src/anvil.cpp:1548-1581`. Selected by `J = L + λ·c + μ·2 + ν·c`, where `c` is a
**hard-wired per-codec constant that does not depend on the stream's length**:

```cpp
1553:  auto add=[&](std::vector<uint8_t> b, double cu){ double L=double(b.size());
        cands.push_back({std::move(b), L + g_stream_lambda*cu + g_stream_mu*2.0 + g_stream_nu*cu}); };
1554:  add(std::move(raw), 10.0);              // raw
1555:  add(rans_stream_bytes(src,kRans4096,1), 40.0);
1557:  add(rans_stream_bytes(src,kRans512,2), 35.0);
1558:  add(rans_stream_bytes(src,kRans256,3), 30.0);
1560:  add(huffman_stream_bytes(src,hlen), 22.0);
1564:  if(...) add(defexc_stream_bytes(src,def), 20.0);
1568:  if(g_stream_ctx && src.size() >= 4096) add(ctx_stream_bytes(src,kRans4096), 45.0);
```

Constants `10/40/35/30/22/20/45` are declared at `:1466-1470` as
"*C_decode = per-STREAM decode cost units*".

**Call sites (10, verified by enumeration):** `:1720`, `:1770`, `:2161`, `:2234`,
`:2500`, `:2642`, `:2868` (conditional, see §1.2), `:3169`, `:3438`, `:3605`.

**This is the default path.** `g_hotop_budget` defaults `false` (`:1617`), and the one
conditional site reads:

```cpp
2868:  auto z = g_hotop_budget ? encode_stream_budget(*v2) : encode_stream(*v2);
```

so on a default invocation the dispatch resolves to **Selector A**. Confirmed.

### 1.2 Selector B — `encode_stream_budget` (FLAG-GATED OFF; size-proportional ns/B)

`src/anvil.cpp:1628-1654`. `C` is **size-proportional, in microseconds**:

```cpp
1618:  static constexpr double kBudgetNsPerByte[7] = { 0.1, 6.0, 6.0, 6.0, 4.3, 3.2, 7.5 };
1633:  auto add=[&](std::vector<uint8_t> b, uint32_t codec){
1634:      double L = double(b.size());
1635:      double C_us = double(src.size()) * kBudgetNsPerByte[codec] / 1000.0;
1636:      cands.push_back({std::move(b), L + g_stream_lambda * C_us}); };
```

**One** call site (`:2868`), reachable only with `--hotop-budget=<anything but "off">`.
This is the **S6-1 whole-codec stream budget** arm.

### 1.3 Selector C — `encode_stream_smallest` (pure min-`L`; J discarded)

`src/anvil.cpp:1587-1604`. Explicitly ignores the decode term:

```cpp
1583:  // Ratio-only stream selector: exact smallest encoded byte count... This intentionally ignores
1585:  // the decode-cost J term: backend ratio experiments must not conflate a stream
1586:  // economics policy with the representation/postcoder comparison.
```

**Five call sites:** `:4224` (×2), `:4235`, `:4286`, `:4604`, `:4605`.

### 1.4 A third, conflicting J appears in a source comment

`src/anvil.cpp:1139` describes a **third** objective:

```
//   J = L + lambda * C_decode * L     (C_decode = per-byte decode cost units)
```

This is *multiplicative* and *per-byte* — the very form the ledger records as discarded.
**Three different J forms coexist in one file** (`1139` multiplicative-per-byte;
`1466/1553` additive-flat; `1636` additive-size-proportional), only one of which is
dimensionally valid. A reader arriving at `:1139` gets a formula the binary does not
implement anywhere.

### 1.5 The documented default is also wrong about the shape

`FORMAT.md:631-663` documents only Selector A. It **never mentions Selector B's
size-proportional objective**, and it does not mention Selector C. It also leaves
`μ`/`ν` described as "*reserved encoder-side hooks (both default `0.0`)*".

**Cross-check against the ledger, and a second documented-vs-shipped mismatch:**
`RESEARCH_LEDGER.md:1044` states the pre-registered binding constants as
"**λ = μ = 0.01 bytes/μs, ν = 0** (binding; verdict uses ONLY these)". But:

```
1472:  static double g_stream_mu = 0.0, g_stream_nu = 0.0;
```

`μ` and `ν` are `0.0` **and have no setter anywhere** — the only occurrences of
`g_stream_mu`/`g_stream_nu` in the entire file are `:1472` and `:1553`. There is no CLI
flag, no env var, no in-process knob (contrast `g_stream_lambda`, which has all three:
`--stream-lambda=`, `ANVIL_STREAM_LAMBDA`, `Options.stream_lambda`).

> **Finding:** the pre-registered `μ = 0.01` **was never exercised by any binary that
> has ever existed.** The ledger's assertion that the verdict "uses ONLY these" binding
> constants is false against the shipped source. `FORMAT.md`'s statement that `μ`
> defaults to `0.0` is the correct one; the ledger is wrong. This is a third
> spec/ledger/binary disagreement, independent of the λ question, and it is the kind
> that should have been caught by a units audit — which is precisely what Q0c is.

---

## 2. The units of `c`, settled

**Question put: is `c` a total decode time or a per-byte budget?**

**Answer: as implemented it is a per-stream *total*; as documented it is a *rate*; and
the two are mutually irreconcilable at any single stream size.**

### 2.1 The three cost tables in the record

| table | raw | defexc | huffman | rANS-256 | rANS-512 | rANS-4096 | ctx | where |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| (i) "earlier text" per-byte | 1.0 | 2.0 | 2.2 | 3.0 | 3.5 | 4.0 | 4.5 | `FORMAT.md:646-648` |
| (ii) **shipped flat `c`** | 10 | 20 | 22 | 30 | 35 | 40 | 45 | `anvil.cpp:1554-1568` |
| (iii) measured size-proportional ns/B | **0.1** | **3.2** | **4.3** | **6.0** | **6.0** | **6.0** | **7.5** | `anvil.cpp:1618` |

The ledger already concedes (i)→(ii)→(iii) is "*right ORDER, wrong GAPS*"
(`RESEARCH_LEDGER.md:3903`). The **coarse ordering is preserved in all three**
(raw < defexc < huffman < rANS-256 < rANS-512 < rANS-4096 < ctx), so the flat table is a
monotone surrogate. That is the only thing in its favour.

### 2.2 The exact dimensional test

`J = L + λ·c` is well-formed only if `λ·c` carries a **length**.

- **Selector B:** `λ = 1e-5 B/ns`, `c = ns/B`, `C = n·c = ns`, so `λ·C = B`. **Valid.**
- **Selector A:** `λ` multiplies a bare `double cu` whose unit is *unstated*. If `c` is a
  rate (ns/B), then `λ·c` is `B/(ns/B)` = `B²/ns` — a length per length per time. It
  cannot be added to `L`. If `c` is a total (ns), then `λ` must be `B/ns` — but `λ`'s
  value, 0.01, is documented as *bytes per microsecond*, which under that reading would
  make `c` a **µs count of 10…45 µs**. A 10 µs total for raw and 45 µs total for ctx
  are not plausible stream decode times for streams of 4 KB–256 KB.

> **Both readings of Selector A's `c` fail.** One is dimensionally meaningless; the
> other is physically implausible. There is no reading under which the shipped flat
> table is a correct decode-cost model.

### 2.3 The hidden scaling factor, quantified

Assume the charitable reading — `c` is a **total in tenths of µs**. Then `c` is what
the size-proportional objective `λ·n·c_ns/B` would yield at stream length
`n* = 1000·c_flat / c_measured`:

| codec | `c_flat` | `c` measured (ns/B) | **implied `n*`** |
|---|---:|---:|---:|
| rANS-256 | 30 | 6.0 | 5,000 B |
| huffman | 22 | 4.3 | 5,116 B |
| rANS-512 | 35 | 6.0 | 5,833 B |
| ctx-rANS | 45 | 7.5 | 6,000 B |
| defexc | 20 | 3.2 | 6,250 B |
| rANS-4096 | 40 | 6.0 | 6,667 B |
| **raw** | 10 | **0.1** | **100,000 B** |

**There is no single stream length at which the flat table equals the measured
size-proportional cost — the implied length spans 5.0 KB to 100 KB, a 20× spread, and
the outlier is `raw`, the single most consequential entry.** (`raw`'s 10× discrepancy
is not a rounding artefact: table (i) says raw = 1.0 ns/B, table (iii) says 0.1 ns/B
bulk. The two differ by 10× on the constant that sets the entire raw-vs-entropy trade.)

### 2.4 The consequence, as a ratio

Decode term of the size-proportional objective ÷ decode term of the flat objective:

| stream `n` | raw | defexc | huffman | rANS-4096 | ctx |
|---:|---:|---:|---:|---:|---:|
| 1,000 B | 0.01 | 0.16 | 0.20 | 0.15 | 0.17 |
| 16,384 B | 0.16 | 2.62 | 3.20 | 2.46 | 2.73 |
| **262,144 B** (max block, `:3769` `block_size=256*1024`) | **2.62** | **41.9** | **51.2** | **39.3** | **43.7** |

Read: **the default objective under-weights decode time by up to 51× on a full-size
block, and over-weights it by ~6.7× on a 1 KB stream.** The error is not a small
constant — it changes sign across the size range the codec actually operates on. A
single stream can be mis-weighted in *either* direction depending only on its length.

> This is the answer to "hidden scaling factors": **there is exactly one, and it is
> `n` itself.** The default objective has no notion of stream size. It is
> length-independent by construction, which is not a modelling simplification — it is a
> dimensional error that happens to be *conservative in one direction and aggressive in
> the other*.

---

## 3. Can λ = 0.01 be interpreted consistently?

**No. The numeral is overloaded across two incompatible quantities, and only one of
them has a unit.**

| | Selector A (default) | Selector B (budget) |
|---|---|---|
| `λ` value | `0.01` | `0.01` |
| what it multiplies | dimensionless count `c ∈ {10..45}` | microseconds `C = n·c` |
| `λ`'s unit | **none / undefined** | **B/µs = 1e-5 B/ns** |
| `λ·c` range | **0.10 – 0.45 B** | 0 – ~19.4 B (at 256 KiB) |
| total dynamic range | **0.35 B** | up to ~19.4 B, scaling with `n` |
| "1 byte ≡ ?" | **ill-posed** | 100,000 ns = 100 µs |

The same numeral is a *unit-ful exchange rate* on one path and a *unit-less weight* on
the other. No single sentence can be true of both. **Any document, experiment, or brief
that says "λ = 0.01" without naming the selector is therefore ambiguous, and the
ambiguity is not cosmetic: it is the difference between a 0.35-byte range and a
19-byte range.**

### 3.1 Is the budget path's interpretation defensible? Yes, and it is the portable one

`λ = 1e-5 B/ns` means **1 byte of compressed size is worth 100 µs of decode time.**
Publishing λ in bytes/ns rather than bytes/cycle is the correct choice, and the shipped
budget table is already in exactly those units. Absolute-time pricing also has a real
merit: as a host's decoder gets faster, `C` in ns falls, so the objective
*automatically* becomes more length-dominated on faster hardware — which is correct,
because time genuinely is cheaper there. A cycles-based λ would need re-derivation per
microarchitecture. **The budget path's convention is sound. The flat path's is not a
convention at all.**

### 3.2 But the rate itself is economically extreme

At λ = 1e-5 B/ns and the project's own measured decode rates (~220 MB/s ⇒ ~4.5 ns/B
whole-codec; rANS streams at 6.0 ns/B):

> **100,000 ns of decode at 6.0 ns/B is the throughput of ≈ 16,700 decoded bytes.**

So λ = 0.01 asserts that **one byte of compressed output is worth ~16,700 bytes' worth
of decode work** — a statement that bytes are effectively free next to time, to within
about 4½ orders of magnitude. That is not a "conservative" setting. It is the
degenerate corner of the objective.

This single fact explains, with no further assumptions, **every null result in the
record**: the 100 % J-agreement (`RESEARCH_LEDGER.md:1067,1168`), the byte-identical
λ = 0 / 0.01 / 0.04 rows (`:1163-1165`), the zero raw-flips (`:3968-3979`), and the
139 tie-only flips (`:3971-3974`). At λ = 0.01 the objective *is* min-`L`. The ledger's
own phrase for this — "*the time term ≤0.16 B/stream never overrides the size winner*"
(`:1165`) — is the correct diagnosis, recorded correctly, and then not followed to its
conclusion.

---

## 4. The "pre-registered λ = 0.01" claim, attacked

`RESEARCH_LEDGER.md:1044` and the format's §7.3 both rest on the constant being binding
in advance. Three independent problems:

**(a) The cited pre-registration artifacts are not in the tree.** The ledger cites
`bench/jcost-validation-contract` v2 "(signed off)" (`:1038-1040`) and
`deliverable/pre-reg-s6-1` (`:1035`, `FORMAT.md:635`). Neither path resolves:

```
MISSING: bench\jcost-validation-contract*
MISSING: deliverable\pre-reg-s6-1*
MISSING: bench          (directory does not exist)
MISSING: deliverable    (directory does not exist)
```

Fourteen unrelated `*-PREREG.md` files exist elsewhere in `docs/`, so pre-registration
*is* a practice this project honours and retains elsewhere. The absence here is
therefore notable rather than systemic. **Consequence: the designation "pre-registered"
for the additive form and for λ = 0.01 is not independently checkable from this
repository.** Absence from the tree is not proof it never existed — but a binding
constant whose contract is unretained is, for audit purposes, indistinguishable from
one chosen after the fact.

**(b) `μ = 0.01` was never in any binary** (§1.5). A pre-registration that specifies a
constant with no implementation path cannot have been executed as written. This is a
checkable contradiction *inside* the record, and it does not depend on the missing files.

**(c) The constant is in its own degenerate regime.** Even granting perfect
pre-registration, a binding constant that provably cannot change any non-tied decision
(§5) does not constrain anything. Pre-registering an inert value is not a stronger
guarantee than picking one.

---

## 5. The vacuity theorem — the 100 % J-agreement is not evidence

This is the audit's load-bearing result and it is exact, code-level, and independent of
any measurement.

**The metric** (`src/anvil.cpp:1572-1577`):

```cpp
1575:  size_t lw=0; for(...) if(cands[i].bytes.size()<cands[lw].bytes.size()) lw=i;
1577:  if(best->bytes.size() <= cands[lw].bytes.size()*101/100) ++g_j_agree;
```

So a selection *agrees* if `L_best ≤ 1.01·L_min`.

**Theorem.** On Selector A, `J`-disagreement with the size-argmin requires

```
L_best − L_min  >  λ·(c_best − c_min) ,  with L_best − L_min ≥ 1   (integers)
```

*Proof.* `best` is the `J`-argmin, so `J_best < J_min`, i.e.
`L_best + λc_best < L_min + λc_min`, i.e. `L_best − L_min < λ(c_best − c_min)`.
`L_best > L_min` because a tie is an agreement. `L` is an integer byte count, so
`L_best − L_min ≥ 1`. Hence a disagreement needs `λ·Δc > 1`. ∎

**The constant.** `Δc_max = c_ctx − c_raw = 45 − 10 = 35`.

> Therefore **for any `λ ≤ 1/35 = 0.0222`, J-agreement is identically 100 % for every
> input, at every stream size, on Selector A.**

**Margins at the three λ values in the record:**

| λ | regime | status of the reported 100 % |
|---|---|---|
| **0.00** | degenerate | theorem (J ≡ min-`L`) |
| **0.01** (pre-registered, shipped) | **1/35 ÷ 0.01 = 2.22× inside the guaranteed region** | **theorem — not evidence** |
| **0.04** (v1 sweep row, `:1168`) | **1.7× outside** | a genuine measurement |

That last row is the tell. The λ = 0.04 run *could* have failed and did not — that is
information. **The λ = 0.01 run could not have failed.** Reporting both under one
headline ("size-faithfulness 100 % at λ = 0.01 **AND** 0.04 **AND** 0.0", `:1168`)
conflates a measurement with two tautologies and credits the gate with the tautologies.

**Therefore:** the record's "J-selection faithfulness **PASS** (100 % at the
pre-registered λ = 0.01) … the pre-registered ≥80 % bar is cleared outright"
(`:1079-1081`) **is not a passed test.** It is a statement about the code. The gate
would have reported PASS for λ = 0.0222, for λ = 0.02, for λ = 0.001, and for any
corpus whatsoever. **An unfalsifiable gate cannot be cleared, only discharged.**

---

## 6. Post-hoc tuning: is the v1 → v2 correction legitimate?

### 6.1 What changed

| | v1 (discarded) | v2 (shipped, "corrected") |
|---|---|---|
| form | `J = L + λ·C_decode·L` **multiplicative** | `J = L + λ·C_decode` **additive** |
| λ | **0.04** | **0.01** |
| per-byte `c` (ns/B) | 4.0 (rANS-4096) | 4.0 |
| decode term on a 16 KB rANS stream | `0.04 × 4.0 × 16384` = **2,621 B (16 % of `L`)** | `0.01 × 40` = **0.40 B (flat)** / 0.98 B (size-prop.) |
| J-agreement reported | **76.7 %** — **FAIL** vs the ≥80 % bar | **100 %** — **PASS** |
| `FORMAT.md:634` status | "*Earlier revisions of this document described the selection objective as multiplicative … λ = 0.04*" | "*the implemented — and … canonical — objective is **additive**"* |

### 6.2 The charge, stated carefully

The ledger's defence is that the **additive form was always the contract** and the
multiplicative form was an implementation bug (`:1047`, `:1071-1073`). If the signed-off
contract said additive λ = 0.01, then v1 was a genuine defect, the correction is
legitimate engineering, and **the discarded v1 numbers deserve exactly the distrust the
ledger gives them.**

**But the magnitude change is not a dimensional correction — it is a ~4-order-of-
magnitude weakening of the objective's discriminating power, and it happened immediately
after a pre-registered threshold was missed.** A pure unit fix would convert a
dimensionally meaningless term into a meaningful one **at equal strength** (e.g.
multiplicative λ = 0.04 → additive λ = 0.04, keeping the 16 % weight). Instead λ was
*also* cut 4×, and because the additive form is flat rather than size-proportional, the
effect on large streams is not 4× but ~6,500× (2,621 B → 0.40 B at 16 KB).

**The change is diagnosable as post-hoc without any allegation about intent**, on three
grounds:

1. **Direction.** The edit moved a failing measurement (76.7 % < 80 %) to a passing one
   (100 % ≥ 80 %). A strictly monotonic weakening of the objective applied after the
   threshold is missed is the *definition* of the failure mode, whatever the motive.
2. **Necessity.** Nothing in the dimensional argument requires λ to fall. Additive
   λ = 0.04 is just as well-formed as additive λ = 0.01, and at λ = 0.04 Selector A can
   actually flip a 1-byte decision — which is precisely the run that produced the
   informative 100 % at `:1168`. Choosing 0.01 *specifically* is what placed the
   shipped constant inside the unfalsifiable region.
3. **Unverifiability.** The artifact that would settle it is not retained (§4a).

**Ruling on the historical change: POST-HOC IN EFFECT; the claim that it was a pure
form correction is not supported by the record and is contradicted by the magnitude
change.** I record this as a *process* finding. It does not impugn the retained
measurement work, and it does not require anyone to have acted in bad faith — a
well-intentioned engineer who fixed the form, saw the bar pass, and moved on has
produced the same record.

**Crucially, the §5 theorem then removes any remaining benefit of doubt about the
*result*: even if the correction was entirely legitimate, the 100 % it produced is
vacuous.** The post-hoc question and the vacuity question converge on the same ruling,
which is why I state both.

### 6.3 Is λ ≈ 44.6 B/µs post-hoc? No — it is a correction in the right direction

`RESEARCH_LEDGER.md:4008-4010` records that the Linux trade's implied willingness to
pay was **460.2 B/µs**, and that the first raw-flips would appear only at
**λ ≈ 44.6 B/µs** — **4.66 orders of magnitude above the shipped 0.01**. The ledger
correctly labels S6-1 as having tested the conservative-budget question and correctly
refuses to call the formulation discredited. **This is the single most scientifically
honest passage in the objective's record** and it is *not* post-hoc: it was written
*against* the shipped constant's interest, in the ledger's own words
("*this is the small-λ limit behaving exactly as its constants say*"). It should be
promoted out of a negative-result note and into the format as the governing statement
of what λ = 0.01 does.

---

## 7. Independent break-even table

All figures from the frozen ns/B table (`anvil.cpp:1618`). λ = 1e-5 B/ns.
Block cap 262,144 B (`anvil.cpp:3769`).

### 7.1 Byte ↔ ns: the indifference locus

At λ = 1e-5 B/ns, a size gap of `ΔL` bytes is exactly offset by a decode-time gap of
`ΔC = ΔL × 100,000` ns. Expressed in decoded bytes at each codec's measured rate:

| size gap `ΔL` | decode gap that offsets it | ≈ decoded bytes @ **raw 0.1 ns/B** | @ **defexc 3.2** | @ **rANS 6.0** | @ **ctx 7.5** |
|---:|---:|---:|---:|---:|---:|
| 1 B | 100,000 ns (100 µs) | 1,000,000 | 31,250 | 16,667 | 13,333 |
| 10 B | 1,000,000 ns (1 ms) | 10,000,000 | 312,500 | 166,667 | 133,333 |
| 100 B | 10,000,000 ns (10 ms) | 100,000,000 | 3,125,000 | 1,666,667 | 1,333,333 |
| 1,000 B | 100,000,000 ns (100 ms) | 1e9 | 31,250,000 | 16,666,667 | 13,333,333 |

**Read the top row.** To be indifferent between two codecs that differ by **one byte**,
their decode costs must differ by the throughput of **16,667 decoded bytes** at rANS
rates. No pair of codecs in the suite is anywhere near that far apart on real content —
the whole measured spread (`:3889-3896`) is 0.1 → 9.0 ns/B. **At λ = 0.01 the objective
is arithmetically incapable of preferring any codec for its speed, on any content, at
any size.** It only ever breaks exact ties.

### 7.2 Break-even bytes: how much worse may a slower codec be and still win?

Budget path. `ΔL* = n · Δc · λ`.

| `n` (stream) | raw→rANS-4096 | raw→ctx | defexc→huffman | huffman→rANS | defexc→rANS | rANS→ctx |
|---:|---:|---:|---:|---:|---:|---:|
| 64 B | 0.0038 | 0.0047 | 0.0007 | 0.0011 | 0.0018 | 0.0010 |
| 256 B | 0.0151 | 0.0189 | 0.0028 | 0.0044 | 0.0072 | 0.0038 |
| 1,024 B | 0.0604 | 0.0758 | 0.0113 | 0.0174 | 0.0287 | 0.0154 |
| 4,096 B | 0.2417 | 0.3031 | 0.0451 | 0.0696 | 0.1147 | 0.0614 |
| 16,384 B | 0.9667 | 1.2124 | 0.1802 | 0.2785 | 0.4588 | 0.2458 |
| 65,536 B | **3.8666** | **4.8497** | 0.7209 | **1.1141** | **1.8350** | 0.9830 |
| **262,144 B** (max block) | **15.4665** | **19.3987** | **2.8836** | **4.4564** | **7.3400** | **3.9322** |

**Two structural facts fall out, and both are adverse.**

1. **Every threshold below 16,384 B is sub-byte.** For streams under ~16 KB the
   objective cannot trade even one byte for any speed. Given `--block` defaults to
   256 KiB but many substreams (types, flags, opcodes) are far smaller, the effective
   regime is *almost entirely sub-byte*.
2. **Even at the maximum block, the entire decode-time economics of one stream is worth
   ≤ 19.4 bytes of size** — against a 175,550 B container (0.011 %). The objective's
   ceiling on byte-level influence in the mode-15 cell is ~0.01 %.
3. **rANS-256 / rANS-512 / rANS-4096 are exactly tied at `Δc = 0`** — by construction,
   not by measurement. Any flip among them is a tie-break artefact. This is recorded
   in the ledger (`:3971-3974`) and it is a *design* consequence of flattening three
   distinct measured ranges (5.40–6.62 ns/B) to a single 6.0.

### 7.3 Break-even λ: what λ would be needed to matter?

Required λ to make a `ΔL`-byte size gap decisive against raw→rANS (`Δc = 5.9 ns/B`),
in B/ns, with the shipped value marked:

| size gap | `n`=256 B | 1 KB | 4 KB | 16 KB | 64 KB | 256 KB |
|---:|---:|---:|---:|---:|---:|---:|
| 1 B | 6.6e-4 | 1.7e-4 | 4.1e-5 | 1.0e-5 | 2.6e-6 | 6.5e-7 |
| 10 B | 6.6e-3 | 1.7e-3 | 4.1e-4 | 1.0e-4 | 2.6e-5 | 6.5e-6 |
| **100 B** | 6.6e-2 | 1.7e-2 | 4.1e-3 | 1.0e-3 | 2.6e-4 | **6.5e-5** |
| 1,000 B | 6.6e-1 | 1.7e-1 | 4.1e-2 | 1.0e-2 | 2.6e-3 | 6.5e-4 |

Shipped λ = **1.0e-5** B/ns. Against the recorded reality on the real-volume streams
(gaps "> 100 B", per `RESEARCH_LEDGER.md:4005-4007`) at a **full 256 KiB block**, the
break-even λ is **6.5e-5 — 6.5× the binding value**. At the actual sizes of those
streams it is far higher still. Against the ledger's `lits` figure the requirement is
**λ ≈ 4.46e-2 B/ns = 4,462× the binding value**, and the Linux trade's implied
willingness to pay is **4.60e-2 B/ns = 4,602×**.

**The gap between the binding constant and the smallest λ that would change any
recorded selection is between 6.5× and 4,602×, depending on which stream you mean.
That span *is* the finding. Q3 cannot resolve it by being run twice.**

### 7.4 Independent confirmation of the zero-raw-flip result

The ledger attributes zero raw-flips to arithmetic necessity (`:4002-4011`). I reach
the same conclusion by a different route, and I can state its robustness precisely:

- The retained manifest does **not** record per-stream `raw_n`, so the ledger's own
  quoted span "~0.02–0.66 B on 10–20 KB streams" is **not reproducible** from retained
  evidence. **I decline to rely on it.** (This is a real weakness in that passage,
  correctly flagged in kind by the constructive lane's §2.1.)
- **My route:** from §7.2, the raw-vs-entropy break-even at the *maximum* possible
  stream length (256 KiB, the block cap) is **15.47 B**. The recorded entropy-coding gaps
  on `lits`/`macro-masks` are **>100 B**. Since 15.47 < 100, no flip to raw is possible
  at any stream length the codec can produce.
- **Robustness envelope:** the argument holds while `n·5.9·1e-5 < 100`, i.e. for all
  `n < 1.69 MB`. The block cap (262,144 B) is 6.4× inside that. `--block` can raise the
  cap (`max_block` validation at `:4851`), so the envelope is **6.4× of headroom**.

**So the ledger's *conclusion* survives my independent attack; its *stated arithmetic*
does not.** That is a distinction worth preserving, because the conclusion is what
routed work and the arithmetic is what a future auditor will try to reproduce.

---

## 8. Ruling on Q3's existing A/B

### 8.1 What the A/B is

| arm | selector | `c` | objective | tie-break |
|---|---|---|---|---|
| **A0** (control) | `encode_stream` (`:1548`) | flat 10…45, dimensionless | `L + 0.01·c` | **distinct `c`, toward cheaper** |
| **A1** (treatment) | `encode_stream_budget` (`:1628`) | 0.1…7.5 **ns/B**, size-proportional | `L + 1e-5·n·c` | **flat 6.0 ⇒ exact ties, broken by candidate order** |

Tie-break provenance: `RESEARCH_LEDGER.md:4048-4050` ("legacy ties broken by distinct
cu toward cheaper; budget ties by candidate order under flat 6.0 — spec'd, not
accidental"), corroborated at `:3971-3974`.

### 8.2 Why it is not scientifically interpretable — five independent reasons

**(1) The flag changes three variables, not one.** `--hotop-budget=on` simultaneously
changes (i) the cost-unit table, (ii) the *size-proportionality* of `C`, and (iii) the
tie-break rule. An A/B that moves three coupled parameters cannot attribute an outcome
to any of them. **Observed flips: 139/139 (100 %) attributable to (iii) alone** — every
one an exact-`L` tie among the three rANS precisions, which have `Δc = 0` by
construction (§7.2). **The treatment arm's entire measured signal is a change in
arbitrary ordering among candidates that the objective declares exactly equal.**

**(2) The discriminating power is provably ~0 in both arms.** §7.2: below ~16 KB every
break-even is sub-byte; at the 256 KiB cap the whole decode budget is ≤19.4 B. The
observed byte gaps on the streams that carry volume are >100 B. The retained outcome
(complete bytes identical, timing unchanged) was **arithmetically certain before the
run**, not discovered by it.

**(3) `λ` is held at the one value that cannot discriminate.** §5: at λ = 0.01 the
J-agreement metric is a theorem. §7.3: the smallest λ that could change a recorded
selection is 6.5×–4,602× higher. Holding λ fixed at the inert value makes the A/B a
null by construction, not by measurement.

**(4) The scale at which the result would be observable is excluded from the design.**
The retained measurement puts the flipped streams at **0.2–0.3 % of decode**
(`:3993-3996`). A cost ratio of 1.226× on 0.25 % of decode is a **0.055 % ceiling**
against a **2.3–8.1 % resolution floor** — 42×–219× below noise. No sample size fixes
this; it is an Amdahl statement.

**(5) The result does not generalise even where it holds.** The manifest covers one
call site of ten (`:2868` vs the nine unconditional `encode_stream` sites), one
flag-gated path, 23 files, and a diagnostic build over `fc23d9a` — not the current
tree. It is silent about Selector A's behaviour, which is the default and the
documented one.

> ### **RULING (Q3 existing A/B): NOT SCIENTIFICALLY INTERPRETABLE.**
>
> It is a **valid null on a narrow engineering question** — "*is swapping the cost
> table and adding size-proportionality byte-safe and timing-neutral on the mode-15
> path?*" — and that question is **already answered** by retained evidence. It carries
> **no** information about whether a decode-cost objective trades bytes for time, which
> is the question the objective exists to answer.

### 8.3 Minimum redefinition

Five changes, all of which are *pre-registration* changes, not code changes.

| # | requirement | why it is minimal-and-sufficient |
|---|---|---|
| **M1** | **Declare `c` size-proportional in ns/B in the objective's normative definition, and publish λ in B/ns.** State the exchange rate as a number with a stated justification (e.g. "1 B ≙ 100 µs of decode, chosen so that a stream's total decode economics equal ≤X % of its compressed size"). | Removes the §2.3 hidden scaling factor. `FORMAT.md` already has the ns/B table (`anvil.cpp:1618`); it needs to be promoted from "flag-gated feature" to "normative". |
| **M2** | **Freeze one tie-break rule, identical in both arms**, e.g. *min `J`, then min codec id*, and **record it**. | Kills reason (1). Without this, ~100 % of observed signal is ordering noise. Note `RESEARCH_LEDGER.md:4073-4076` already ruled against restoring legacy tie-breaks post-hoc on evidentiary-hygiene grounds — that ruling is *correct* and this redefinition is its forward-looking analogue, not a reversal. |
| **M3** | **Raise λ to a pre-registered discriminating value, with the control retained.** Minimum: a three-point ladder at λ ∈ **{1e-5 (inert control), 6.5e-5, 4.46e-2 B/ns}** — the second is the smallest λ that could flip a 100 B gap at a full 256 KiB block (§7.3), the third is the ledger's own `lits` reference (`:4010`). | Removes reason (3). Keeping 1e-5 as the control costs nothing and preserves the recorded null as a real data point rather than an assumption. |
| **M4** | **Log, for every candidate: `L_c`, `n`, and the argmin under each policy.** The current manifest records only the two *chosen* arms' codec and length (correctly noted in the constructive lane's §6.1), so the **objective-level disagreement rate is not computable**. | Without this the census measures one sample, not the function. This is the single highest-value logging change and it is byte-only/count-only — no timing, no codec execution beyond one encode pass. |
| **M5** | **Pre-register the falsifier and the byte gate.** Falsifier: *"if no stream exists where the `J`-argmin differs from the `L`-argmin by a size the exchange rate prices above the noise floor, the decode-economics objective is inert at every λ tested and must be re-derived — not re-run."* Byte gate: the ledger's *bar B* ceiling, because the recorded raw trades on the macro streams cost **+166,823 B (+95 %)** and **+84,956 B (+48 %)** on a 175,550 B container (`SYNTH-PERF-FLEDGE.md:175`). | Makes the run able to return a decision in either direction, and blocks the failure mode where a λ sweep finds only ratio-infeasible trades. |

**Crucially, M1–M5 do not require changing the cell.** And the redefinition is worth
running, because the *upside is large and currently unexploited*: the constructive
lane's Amdahl ceiling of 1.226× is the ceiling **on the 139 realised rANS-precision
flips only**. It is *not* a ceiling on the objective. On the macro streams that carry
the time — 26 % + 26 % of decode (`RESEARCH_LEDGER.md:3928-3931`), *s* ≈ 0.52 — a
raw↔rANS flip at ratio 60× or a raw↔ctx flip at 75× would project:

```
gain = 1/(1 − 0.52 + 0.52/r)   →   r=60: 2.046×      r=75: 2.054×
```

**≈2.05× whole-codec decode, versus a 2.3 % resolution floor and the 1.53×–1.81×
residual token-lane bar (`18-future-decoder-architecture` §2.1).** That is a
first-order question sitting one λ away from where the objective currently sits, and it
is **invisible to Q3 as posed** because Q3 only ever looks at the 0.25 % of decode where
the objective is inert.

> **Fair criticism of the constructive lane, stated against my own interest in
> disagreeing:** the preflight's §4.1 ceiling is generous *against its own proposal*
> and therefore over-generalised. "0.037 %–0.055 % ceiling" is correct for **this arm
> at λ = 0.01**; read as "the objective cannot do better", it is false by ~40×. The
> correct reading is **CANCEL-the-arm, not CANCEL-the-question.** Its headline verdict
> (CANCEL Q3 *timing*) survives; its §4 framing would, if propagated into a format
> document, foreclose the one genuinely live question in this area.

### 8.4 What is NOT required

- **No new codec, no new mode, no wire change.** `encode_stream_budget` already
  implements the size-proportional objective; M1–M3 are flag/`λ`/tie-break choices.
- **No re-run of the recorded A/B.** Its null stands and is already recorded.
- **No clock derivation.** M1–M5 are all in ns/B and B/ns.
- **No change to the AOC / DDMC censuses**, which the constructive lane correctly
  identifies as unaffected.

---

## 9. Minimum documentation corrections, and one hazard to avoid

Consistent with §6.3 of the constructive lane, and adding this audit's findings. All are
**documentation-only and change no byte**:

1. **Name the selector.** Every mention of "λ" must name its selector. Three policies
   ship; two are undocumented.
2. **Publish the units once, correctly:** `c` in **ns/B**, λ in **B/ns**
   (λ = 0.01 B/µs = 1e-5 B/ns), and state the exchange rate as **1 byte ≙ 100,000 ns**.
3. **State that the default objective's `c` is length-independent by construction** and
   therefore carries a **maximum dynamic range of 0.35 bytes**, not a decode-cost model.
   (Correct in the constructive lane's §7.1; this audit adds the 0.35 B figure and the
   `n*` = 5–100 KB implied-length spread.)
4. **The tie-break disclosure** for the flat 6.0 rANS family. (Correct in §7.2; add that
   it is a *consequence of flattening* three measured ranges, 5.40–6.62 ns/B.)
5. **The unfalsifiability note.** State plainly: at λ ≤ 1/35 the J-agreement metric is
   **100 % by construction** and must not be reported as a pass. Move it out of the
   ledger's PASS language.
6. **Promote the λ ≈ 44.6 B/µs reference** out of a negative-result note into the
   governing statement of what λ = 0.01 does (§6.3).
7. **Record `μ`'s unreachability:** the ledger's "λ = μ = 0.01" cannot have been
   executed; `μ` is a hard-wired `0.0` with no setter (`anvil.cpp:1472`).
8. **Fix `src/anvil.cpp:1139`**, which documents a multiplicative per-byte `J` the binary
   does not implement anywhere.

### 9.1 Hazard: how item 1 could be turned into post-hoc tuning

The dimensional correction to the flat table is **correct** and **overdue** — §2.3
proves the current table mis-weights decode time by 2.6×–51× depending on stream size.
**But making `c` size-proportional in the default path would change which codec is
selected on real content, which changes compressed bytes.** It is therefore *not* a
unit correction; it is a byte-visible default-path change.

> **It must not be shipped inside the documentation change.** It requires its own
> pre-registration, its own byte census, and the M2 tie-break freeze to be interpretable
> at all — without a frozen tie-break, a size-proportional `c` on the default path
> would move bytes for reasons that are pure ordering noise. Sequence it **after**
> M1–M5, as its own gated change, or the "unit correction" becomes the fourth instance
> of the §6.2 pattern.

---

## 10. Claim hygiene of this document

- **Nothing was executed.** No codec, no corpus, no benchmark, no timing, no fuzz. All
  arithmetic is over values read from source and documents, in PowerShell.
- **No production file was modified.** `src/anvil.cpp`, `FORMAT.md`,
  `RESEARCH_LEDGER.md` are byte-unchanged by this audit.
- **No network, no git operation.**
- Every source claim carries a `file:line` anchor; every retained-result claim carries
  a `RESEARCH_LEDGER.md` line anchor or a `docs/swarm-2026-10-02/` anchor.
- **One number of mine is deliberately not reproducible and I say so:** I did not rely
  on the ledger's "~0.02–0.66 B on 10–20 KB streams" span, because the retained manifest
  lacks the per-stream `raw_n` needed to reproduce it. §7.4 substitutes an argument that
  is reproducible and states its robustness envelope.
- **Two absences are reported as absences, not as proof of nonexistence:** the
  `bench/jcost-validation-contract` and `deliverable/pre-reg-s6-1` artifacts (§4a) are
  not in this tree. My finding is that the pre-registration claim is **not
  independently checkable here**, not that it never existed.
- **Where I agree with the constructive lane** (§8.2 reason 4's Amdahl arithmetic,
  the §6.1 manifest-coverage gap, the §7 documentation-only disposition, the CANCEL of
  Q3 timing) it is because independent recomputation produced the same result.
  **Where I dissent** (§8.3's "cell is fine, λ is the defect", and §8.3's note that the
  1.226× ceiling is not a ceiling on the objective) the disagreement is on the scope of
  the conclusion, not its arithmetic.

---
---

# 11. ADDENDUM — Q0c self-correction: dimensional repair vs economic λ selection

*Issued after challenge to M3. The challenge is sustained. §8.3's M3 is withdrawn and
replaced by M3′/M3″ below. §8.3's rulings, including the cancellation of Q3, are
unaffected — §11.6 gives the proof that they were never load-bearing on M3.*

---

## 11.1 The defect in M3, stated plainly

§8.3's M3 proposed an arm ladder of **λ ∈ {1e-5, 6.5e-5, 4.46e-2 B/ns}**. Both
non-baseline values were derived from already-observed corpus facts:

- **6.5e-5** was computed (§7.3) as the λ at which a **100 B** size gap becomes
  decisive — and "100 B" is *the recorded entropy-coding gap on ANVIL's own corpus*
  (`RESEARCH_LEDGER.md:4005-4007`).
- **4.46e-2** is the ledger's own `lits` figure (`:4008-4010`), itself computed from a
  measured size gap on a measured stream.

So I proposed testing values of λ **because they had been observed to change selections
on the corpus that would be tested.** Under this project's gate discipline that is
result-conditioning in exactly the sense §6.2 charged against the historical v1→v2
change: a parameter chosen after seeing which parameter values move the answer is not a
policy choice, however honestly labelled.

**M3 is withdrawn.** The charge in §6.2 stands, and I had committed the same error one
section after diagnosing it. I record the correction rather than quietly restating the
ladder, because the error is instructive: it is easy to write "raise λ to the smallest
discriminating value" and thereby smuggle a corpus reading into what reads as a
methodological instruction.

---

## 11.2 The distinction the correction turns on

The objective has **two layers that must be kept apart**, because they carry different
evidentiary status.

### Layer D — Dimensional repair. **Zero degrees of freedom.**

| item | content | status |
|---|---|---|
| D1 | `c` published in **ns/B**, size-proportional: `C = n · c [ns]` | not a choice — the only form in which `λ·C` is a length |
| D2 | λ published in **B/ns** | not a choice — fixes the unit of the one free parameter |
| D3 | The flat dimensionless table (`:1554-1568`) is **not** a decode-cost model and **cannot be repaired in place** — it has no length dimension to repair | fact about the code, §2.2–§2.3 |
| D4 | The default path's decode term has a **0.35-byte** dynamic range | consequence of D3 |

**Layer D is settled by dimensional analysis alone.** No corpus, no experiment, no λ
value, no flip. It is *mandatory*, and it is the entire legitimate content of a "unit
correction." Nothing in it is tunable. **All of §9's documentation items and M1, M2, M4,
M5 belong here or to experimental protocol; none of them is a λ recommendation.**

### Layer E — Economic λ selection. **A policy choice with an independent mandate.**

Layer E is the question *"how much is one byte of stored size worth in decode
microseconds?"* It is a **preference**, not a derivation. It may legitimately vary by
deployment, and two projects facing the same corpus may correctly choose different λ.
Therefore:

> **λ may not be chosen by reference to what it does to ANVIL's outputs.** A policy λ
> must be justified in terms that exist *before* the corpus is looked at. The corpus may
> then be used to *evaluate* the policy — never to *derive* it.

This is the ordinary pre-registration asymmetry, and it is the same asymmetry §6.2 found
violated historically. The correction is to enforce it in my own recommendation.

---

## 11.3 Corrected M3 — what replaces the ladder

**M3 is replaced by two clauses: one that produces the artifact, one that governs the
parameter.**

### M3′ — Publish the **analytic breakpoint / parametric selector map**. (The deliverable.)

Not a sweep, not a ladder, not a recommendation. **A complete corpus-free
characterisation of the selector's behaviour across its parameter domain**, so an arbiter
can read the regime structure *before* choosing a policy point. Derived in closed form
in §11.4. Properties that make this the right deliverable:

- **Corpus-free** — every quantity is a function of `c`, λ, and `n` only.
- **Recommendation-free** — it characterises the map; it selects no point.
- **Decision-enabling** — it identifies which regions are *inert*, hence where a λ choice
  could matter at all. Strictly more useful than a ladder, and stable under corpus
  change.
- **Reusable** — unlike a ladder fitted to today's corpus, it does not expire.

### M3″ — Policy λ must come from a **named owner** under a declared **admissible** justification.

| class | justification | admissible as |
|---|---|---|
| **A** | A **declared deployment/SLO exchange rate**: a published statement of the form "*storage is worth X against a decode-latency budget of Y per file, the exchange taken from Z*" — where Z is a deployment fact (storage tier cost, SLO, SLA), **not** a codec measurement | **policy λ** |
| **B** | A **normative exchange from an external cost model**, with its own citation and error bars — e.g. a published storage-vs-CPU cost ratio | **policy λ** |
| **C** | An **exhaustive sweep of the `x` domain**, reported as *sensitivity analysis*, arbiter choosing the operating point afterwards — **conditional on the arbiter committing, before results, to the criterion by which it will choose** | **policy λ** |
| **R** | A landmark from this project's own history (λ ≈ 4.46e-2 B/µs at `:4010`; the Linux implied 4.60e-2 B/µs at `:4008`) | **`[REFERENCE — NOT POLICY]` only.** May appear in the map. **May not be an arm in a confirmatory test.** Permitted only as a labelled point inside a class-C sweep. |

Explicitly **inadmissible** — and the specific thing M3 got wrong:

- ❌ "*the smallest λ at which a flip appears on file F*" — fitting λ to the observed
  answer.
- ❌ "*the λ that reproduces the Linux trade*" — same defect; the trade's size gap is a
  measured quantity, and the fit targets a desired outcome.
- ❌ "*λ derived from a recorded gap on the corpus under test, for any reason, including
  robustness arguments.*" Robustness is a motive, not a source.

**Consequence for §7.3.** My break-even table stands **as the map itself** — it is
parameterised over a hypothetical gap of 1/10/100/1000 B, i.e. a characterisation, not a
chosen gap. It must be read as *"here is the map; the arbiter supplies the operating
point under M3″."* It is **not** a recommendation of 6.5e-5, nor of the 100 B column in
particular. **The 4,462× and 4,602× readings of §7.3 are demoted to class R
`[REFERENCE]`** and must be labelled as such wherever cited.

---

## 11.4 The parametric selector map (corpus-free, closed form)

This is M3′'s content. All of it derives from `anvil.cpp:1618` alone.

### 11.4.1 The objective depends on `n` and λ only through their product

With `C = n·c` (ns), the objective is

```
J_c = L_c + λ·n·c_c = L_c + x·c_c ,        x ≡ λ·n   [B²/ns]
```

**`x` is the only free scalar.** `n` and λ do not act independently:

> **The size-proportional selector is invariant along the hyperbola `n·λ = const`.** A
> stream twice as long requires λ half as large for *identical* selection behaviour.

This is a dimensional identity, not a modelling choice, and it is the most useful single
thing the map publishes: **"which λ" is not well-posed on its own. The well-posed
question is "which `x`."** It also dissolves the framing that made M3 look like a ladder.

Shipped operating point for reference: `x = 1.02e-2` at 1 KB, `0.164` at 16 KB, `2.62`
at the 256 KiB block cap.

### 11.4.2 The inertness criterion — the map's key structural result

`L` is an **integer** byte count. From §5's argument, carried to the size-proportional
form:

> **The objective is INERT — provably incapable of changing any non-tied selection — for
> any pair with cost gap `Δc` exactly when**
> ```
> x · Δc  <  1 byte          i.e.   x  <  x_BP ≡ 1/Δc
> ```
> Equivalently at fixed λ: `n < n_wake ≡ 1/(λ·Δc)`.

This is a **guarantee of equivalence to min-`L`**, not an empirical observation.

### 11.4.3 The breakpoint table (complete, corpus-free)

At the shipped λ = 1e-5 B/ns:

| pair | `Δc` (ns/B) | **`x_BP = 1/Δc`** (B²/ns) | **`n_wake`** — smallest stream where J can differ |
|---|---:|---:|---:|
| raw ↔ ctx-rANS | 7.4 | 0.135135 | **13,514 B** |
| raw ↔ rANS (any precision) | 5.9 | 0.169492 | 16,949 B |
| raw ↔ Huffman | 4.2 | 0.238095 | 23,810 B |
| raw ↔ defexc | 3.1 | 0.322581 | 32,258 B |
| Huffman ↔ ctx-rANS | 3.2 | 0.312500 | 31,250 B |
| defexc ↔ rANS | 2.8 | 0.357143 | 35,714 B |
| Huffman ↔ rANS | 1.7 | 0.588235 | 58,824 B |
| rANS ↔ ctx-rANS | 1.5 | 0.666667 | 66,667 B |
| defexc ↔ Huffman | 1.1 | 0.909091 | 90,909 B |
| **rANS-256 ↔ rANS-512 ↔ rANS-4096** | **0.0** | **∞** | **never, at any `x`** |

**Two corpus-free results follow.**

1. > **At the shipped λ, the objective is provably identical to min-`L` on every stream
   shorter than 13,514 bytes, for every pair of codecs.** The most favourable pair
   (raw ↔ ctx, the widest cost gap) is the *earliest* to wake; the typical pair wakes at
   **17 KB–91 KB**. Because this minimises over pairs, it is an unconditional floor, not
   a typical-case statement.
2. > **Three of the seven suite codecs — the entire rANS precision family — have
   `Δc = 0` and are therefore *never* economically distinguishable by the size-proportional
   objective, at any λ, at any `n`, on any input.** This is a *design* property of
   flattening three measured ranges (5.40–6.62 ns/B) to one constant, not an empirical
   finding about content. Roughly 3/7 of the candidate set is permanently outside the
   objective's reach.

Result 1 independently reproduces, and supplies the *bound* for, the "13.5 KB" figure at
`SYNTH-PERF-FLEDGE.md:176` — confirming that figure derives from the raw ↔ ctx pair and
is therefore the **optimistic** end of a 13.5 KB–91 KB range. Result 2 is the corpus-free
version of the "139 flips are tie-breaks" observation, and the stronger claim: the flips
are tie-breaks **by construction, not by coincidence**.

---

## 11.5 Corpus-independent restatement of the Q3 cancellation

§8.2 rested on five reasons; reasons (2), (3) and (4) invoked observed corpus
quantities. **None of the five is load-bearing.** The cancellation can be carried on
structure plus retained *measurements* — and a measurement is evidence, not a fitted
parameter:

| leg | statement | corpus-free? |
|---|---|---|
| **C1** | At λ = 1e-5 the objective is inert for **every** stream below 13,514 B, all pairs (§11.4.3) | **yes** — closed form |
| **C2** | The J-agreement metric returns 100 % for **any** λ ≤ 1/35; λ = 0.01 is 2.22× inside (§5) | **yes** — code theorem |
| **C3** | Three of seven candidates have `Δc = 0`, **never** economically separable in either arm at any λ | **yes** — design property |
| **C4** | A0 and A1 differ in **tie-break rule** as well as objective, so every observed flip is confounded by construction | **yes** — design property |
| **C5** | The default path (10 of 11 call sites; the documented normative one) has a **0.35-byte** dynamic range and is inert at *every* stream size | **yes** — code + arithmetic |
| **C6** | *Timing leg only:* measured flipped-stream time share 0.2–0.3 % × measured cost ratio 1.226× → **0.037 %–0.055 %** against a **2.3–8.1 %** resolution floor | uses **measurements**, not a fitted λ |

> ### **Ruling unchanged: Q3's existing A/B remains NOT scientifically interpretable,
> and is CANCELLED on the timing leg.**

Five of six legs are corpus-free. C6 bounds an effect size using retained measurements
and is **never** used to choose a parameter. The cancellation was therefore never
contingent on M3, and is not weakened by M3's withdrawal.

### 11.5a One wording of my own that I must correct

§7.4 argued the zero-raw-flip result is "arithmetically necessary," using the recorded
**">100 B"** gap. That number **is** corpus-derived, and "necessary" was overstated. The
correct corpus-free statement:

> At λ = 1e-5, a raw selection requires the entropy coder to save **fewer than
> `n·5.9e-5` bytes** on that stream — at most **15.47 B** at the 256 KiB block cap, less
> for any smaller stream. So **zero raw-flips is not arithmetically impossible; it
> requires only that no stream present an entropy-coding gain below that threshold.** The
> retained manifest then *measures* zero such flips.

My §7.4 conclusion and its robustness envelope (holds while `n` < 1.69 MB) are
unchanged, but "necessary" becomes **measured and consistent with the map, not implied by
it.** A future auditor deserves that distinction.

---

## 11.6 Disposition — what survives, what changes

| item | disposition |
|---|---|
| §8.3 **M1** (normative ns/B `c`; λ in B/ns) | **SURVIVES** — Layer D, zero degrees of freedom |
| §8.3 **M2** (freeze one tie-break across arms) | **SURVIVES** — experimental protocol, no λ content |
| §8.3 **M3** (λ ladder 1e-5 / 6.5e-5 / 4.46e-2) | **WITHDRAWN** — result-conditioned; replaced by M3′ + M3″ |
| §8.3 **M4** (log `L_c`, `n`, per-candidate argmins) | **SURVIVES, and is promoted** — under M3′ it is what lets the map be checked against reality, the only legitimate corpus use of a census |
| §8.3 **M5** (pre-registered falsifier + bar-B byte gate) | **SURVIVES, strengthened** — the falsifier is now stated over the `x` domain: *"if no stream exists whose `L` gap exceeds `x·Δc` at any tested `x`, the objective is inert across the tested domain and must be re-derived, not re-run"* |
| §7.3 break-even table | **SURVIVES as the map**; 6.5e-5 and the 4,462× / 4,602× readings **demoted to class R `[REFERENCE]`** |
| §8.2 reasons (2), (3) | **REPLACED** by corpus-free C1–C5 |
| §8 ruling; §8.2 five reasons; §8.3 cell-is-fine; §8.3 note on the 1.226× ceiling | **ALL SURVIVE UNCHANGED** |
| §9 documentation items; §9.1 hazard | **SURVIVE UNCHANGED** |
| §6.2 post-hoc ruling on the historical v1→v2 change | **SURVIVES** — and §11.1 extends the charge to my own M3 |

**Net effect: no ruling changes.** What changes is the *basis* — from partly
corpus-conditioned to corpus-free — and one recommendation is withdrawn because it
committed the error this audit was written to detect.

**Standing instruction left for the arbiter:** publish the map (M3′); have a named owner
set the policy point from a deployment or external-cost source (M3″); use the corpus only
to test the result. **Do not let a λ be chosen because it moves a selection.**