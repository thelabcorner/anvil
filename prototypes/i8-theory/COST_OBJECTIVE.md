# THEORY — THE JOINT OBJECTIVE: form, λ, and the two independent reasons S6-1's zero was correct

Status: **derivation, corrected**. My first draft of this document reached the
opposite conclusion on §3; the arithmetic below overturned it. I record the
correction rather than bury it, because the corrected version is the useful
one.

---

## 1. The additive form is the wrong functional form — and that is not why S6-1 found nothing

The incumbent objective is

    J = L + λ·C_decode,   λ = 0.01 B/µs frozen                      (1)

with C_decode a **size-proportional** price (ns/B × stream bytes). That form
asserts a single scalar exchange rate between bits and nanoseconds.

It is the wrong form, for a reason the t3 profile states explicitly. Decode
time decomposes as

    T = α·N_out                     [CRC + output write, 44%]        per OUTPUT byte
      + β(κ)                        [materialization + model build, 26%]   setup
      + ε                           [concat/alloc/headers, ~15%]     per BLOCK
      + γ·n                         [token loop, 11%]                per TOKEN
      + Σⱼ δ(κⱼ)·sⱼ                 [entropy pulls]                  per SYMBOL

and **Σ δ·sⱼ = 0.3% of T**. The C_decode table prices only that last term, but
form (1) applies it to the whole stream as if it were the whole decoder.

> **Fidelity failure, stated precisely: C_decode is a per-symbol price applied
> to a cost that is ~89% per-output-byte and setup, none of which the
> entropy-coder choice can move.**

**Immediate consequence (a hard bound, no λ involved):**

    max decode saving from per-stream codec choice ≤ 0.3% of T
    ⇒ speedup ≤ 1/(1 − 0.003) = 1.003x

So per-stream entropy-codec selection is **not a decode lever in this
architecture, for any λ whatsoever**. This retires the branch on math grounds
(not implementation-era grounds), and it is stronger than "λ = 0.01 was too
small".

## 2. Constrained vs Lagrangian — and a real limitation of form (1)

The assignment asks whether the problem should be `min L s.t. C ≤ C_budget`
(2), whose Lagrangian is (1) with λ determined by the constraint.

They are the same optimisation **up to a duality gap**. The achievable (L, C)
set is a finite set of discrete codec configurations, hence non-convex; so
sweeping λ traces only the **lower convex hull**, and configurations sitting in
a non-convex dent are **unreachable at any λ**. That is a genuine limitation of
form (1) worth recording, though second-order beside §1.

λ's meaning is unambiguous either way: it is the **shadow price of decode
time** — bytes you will spend per unit of throughput. It is not a taste
parameter.

## 3. λ calibration — and here my first draft was WRONG

**Draft claim (withdrawn):** I initially wrote that λ = 0.01 was 3–5 orders of
magnitude *below* the market price, and therefore S6-1's zero flips was a
statement about the constant rather than the formulation.

**That is backwards, and the arithmetic is unambiguous.** Calibrating λ from
the reference front (the only defensible source — the price the codecs we are
trying to beat actually pay):

    λ_frontier = ΔL / Δt  between consecutive vertices of the reference
                 decode front   [B per MB/s] == [B per µs]

Measured from tests/benchmark-suite.csv (`prototypes/i8-theory/lambda_calibration.py`):

| file | front step | ΔL (B) | Δt (MB/s) | λ_frontier (B/µs) |
|---|---|---:|---:|---:|
| generated.log | q11 → zstd-19 | 24,318 | 1,145.1 | **21.2** |
| generated.json | q11 → zstd-19 | 14,191 | 999.1 | **14.2** |
| generated.jsonl | q11 → zstd-19 | 16,074 | 1,117.4 | **14.4** |
| generated.sqlite | q11 → zstd-19 | 29,766 | 808.3 | **36.8** |
| synth-jitter | q11 → zstd-19 | 12,211 | 1,343.1 | **9.1** |
| synth-arith | q11 → zstd-19 | 19,601 | 138.0 | **142.0** |
| anvil.exe | q11 → zstd-19 | 5,081 | 202.4 | **25.1** |

Robust band (secants with Δt ≥ 100 MB/s): **min 0.4, median 17.1, max 723**
B/µs. The market pays on the order of **10–40 B/µs** on the large structured
files.

Now the flips S6-1 declined, priced the same way (IWTP = ΔL / Δt):

| flip | ΔL (B) | Δt (MB/s) | **IWTP (B/µs)** |
|---|---:|---:|---:|
| macro-masks → raw (log) | 166,823 | 49.5 | **3,369** |
| macro-masks → raw (sqlite) | 166,823 | 35.4 | **4,712** |
| macro-resid → raw (jsonl) | 84,956 | 61.6 | **1,378** |
| macro-resid → raw (sqlite) | 84,956 | 30.3 | **2,799** |
| shape-dist-delta → raw (Linux) | 19,800 | 120 | **165** |

> **The flips cost 1,378–4,712 B/µs. The market rate is 10–40 B/µs. The flips
> are 40–160x MORE expensive per unit of throughput than simply moving along
> the reference front.**

**S6-1 declined them correctly — and it would have been correct at ANY λ in
the market band, not just at 0.01.** The zero-flip result was not a
mis-calibrated constant. It was the right answer for the right reason, and the
"materially higher λ" escape hatch in the post-S6-1 note should be **closed**,
not opened: to make those flips look attractive you would need λ ≈ 1,400–4,700
B/µs, i.e. 40–160x above what the frontier itself pays. A λ that high is not a
price, it is a different objective.

### 3.1 Convergence check (independent route to the same verdict)

Rather than trust the ratio, I checked the flips directly against the
iso-crossing budget `L*(t) = N·R_ref(t)`:

| file | before: gap to budget | after masks→raw: gap to budget | |
|---|---:|---:|---|
| generated.log | −48,163 | **−214,986** | worse |
| generated.json | −10,133 | **−176,956** | worse |
| generated.jsonl | −31,482 | **−198,305** | worse |
| generated.sqlite | −136,748 | **−303,571** | worse |

Every flip moves the row *further* from the front, not closer. Two independent
computations (exchange-rate ratio, and direct budget comparison) agree. **The
byte-for-speed trade is not merely expensive; it is counterproductive on every
tested file.**

## 4. What follows — the formulation I recommend

    J = L + λ·[ α·N_out + β(κ) + γ·n + Σⱼ δ(κⱼ)·sⱼ + ε ]             (3)

with each term measured separately and λ calibrated to λ_frontier. Practical
consequences, in priority order:

1. **Stop trying to buy decode speed with per-stream codec selection.** It
   controls 0.3% of T (§1) and every candidate flip costs 40–160x the market
   rate (§3). Two independent reasons. This is a math-class negative.
2. **Spend on α, β, ε** — CRC (wire-invisible, 1.79x end-to-end cap), lazy
   materialization (26%), alloc/concat (15%). These are *not* expressible in
   form (1) at all, which is why the Linux "J-selection at the WHOLE-CODEC
   budget level" framing was the right instinct and the per-stream framing was
   not. Note these legs are **free in bytes** — the CRC fix is wire-invisible —
   so they are not trades at all and λ does not enter.
3. **The byte route and the speed route are not interchangeable.** Per the
   iso-crossing deliverable, on 11 of 13 files the speed route needs 4.6x–40x
   while the measured floor caps the wire-invisible lift at ~1.79x. And now:
   buying speed with bytes is priced 40–160x above market. **On this corpus,
   bytes must be found, not bought or traded for.**

## 5. Assumptions and falsification

**Assumptions.**
- (A1) The t3 profile (crc 44% / mat 26% / concat 15% / token 11%; entropy
  pulls 0.3%) is representative. **It is ONE file (generated.log) and ONE mode
  (15).** If other files/modes shift the shares materially, §1's 0.3% and the
  1.003x bound change per file.
- (A2) IWTP figures reuse S6-1's measured ΔL (masks +166,823 B; resid
  +84,956 B) and decode-perf's relative gains (21–27%, 18–21%) applied to the
  t3 floor speeds. I did **not** re-measure ΔC end-to-end.
- (A3) λ_frontier is a *secant* slope between front vertices, so it is a coarse
  proxy for the local shadow price. I exclude secants with Δt < 100 MB/s as
  unstable (those produce the 723 and 4,137 outliers).
- (A4) The Linux shape-distance-delta flip (165 B/µs) is included for
  completeness but uses Linux microbench numbers on a different host.

**Falsification conditions.**
- **F1.** Measure entropy-pull share on ≥3 files spanning structured and binary
  classes. If any file exceeds ~5% (not 0.3%), §1's bound fails for that file
  and per-stream selection becomes a real lever there. *This is the single
  assumption most worth testing, because the whole §1 conclusion rests on a
  one-file measurement.*
- **F2.** If any per-stream flip at λ = 0.01 produces an end-to-end decode
  change outside the 5–19% noise band, the 1.003x bound is falsified.
- **F3.** My §3 verdict is falsified by a flip whose IWTP is BELOW the local
  λ_frontier on the same file. I predict none exists in the current stream
  inventory; the cheapest measured flip is 1,378 B/µs against a 10–40 B/µs
  market. A flip under ~40 B/µs would change the recommendation.
- **F4.** Secant-based λ is only a proxy for the shadow price. A denser
  reference front (more brotli levels, more zstd levels) would give a better
  local estimate. If bench adds levels, re-run
  `prototypes/i8-theory/lambda_calibration.py`; do not reuse these numbers.
