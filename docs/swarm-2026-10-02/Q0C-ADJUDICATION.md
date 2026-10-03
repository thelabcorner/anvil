# Q0c Adjudication — `λ·c`, dimensional validity, historical gates, Q3 disposition

**Date:** 2026-10-02 · **Tree:** `ANVIL` @ `b8eae11`
**Inputs adjudicated:** `Q0C-LAMBDA-C-SPACE-BUNNY.md` (constructive, 668 L),
`Q0C-LAMBDA-C-CRITIC.md` (critic, 709 L), plus `06-entropy-codesign-space-bunny-q3-preflight.md`
(the Q3 preflight the critic argues against).

## 0. Method and prohibitions

Nothing was executed. No codec, no corpus byte, no benchmark, no timing, no network, no git
operation. No production or coordinator file was modified; this document is the only write.
Every load-bearing claim below was re-derived from `src/anvil.cpp`, `FORMAT.md`,
`RESEARCH_LEDGER.md`, or from the frozen `ns/B` table, in PowerShell.

**Evidence labels.** `[PROVEN]` — follows from source text or exact integer/real arithmetic, no
measurement. `[MEASURED]` — an already-recorded frozen measurement, cited to its record.
`[DERIVED]` — arithmetic over `[PROVEN]`/`[MEASURED]` inputs, all inputs shown. `[DISPUTED]` —
asserted by a lane and **not** reproducible from retained evidence. `[PROJECTED]` — not evidence.

This document deliberately does **not** restate either report's tables. It records only what
changes a decision.

---

## 1. Certification of the load-bearing claims

| # | claim | verdict | basis |
|---|---|---|---|
| A1 | `λ = 0.01 bytes/us = 1e-5 B/ns`; 1 wire byte ≙ 100 µs | `[PROVEN]` | `anvil.cpp`:1608 unit string; `0.01 B/µs × 1e-3 = 1e-5 B/ns`; `1 B / (1e-5 B/ns) = 1e5 ns` |
| A2 | Frozen `c` table `{0.1, 6.0, 6.0, 6.0, 4.3, 3.2, 7.5}` ns/B | `[MEASURED]` | `anvil.cpp`:1618-1626 = ledger `:3904-3906` verbatim; measurement at `:3883-3899` |
| A3 | Three selectors exist, not one; default = flat-constant `encode_stream` | `[PROVEN]` | `:1548`, `:1587`, `:1628`; `g_hotop_budget=false` `:1617`; sole dispatch `:2868`; call sites enumerated = 10 / 5 lines / 1 |
| A4 | v3 `J = L + λ·C_us` is dimensionally valid | `[PROVEN]` | `[B] + [B/µs]·[µs] = [B]` |
| A5 | v2 `J = L + λ·c` is **not a quantity** | `[PROVEN]` | `c ∈ {10..45}` is a bare `double` from a literal table `:1554-1568`; no time axis anywhere; `λ·c` ∈ {0.10..0.45} with no unit |
| A6 | Zero-raw-flips is arithmetically necessary at the frozen `c` | `[DERIVED]` **but narrower than claimed** | ΔL\* = `n·5.9e-5` B; recorded gap ">100 B" (`:4007`) ⇒ no flip for `n < 1.695 MB`; largest real stream 15,680 B ⇒ **108× of headroom**. See §2.2 |
| A7 | Whole-file decode of the log container = 744.5 µs | `[DERIVED]` over `[MEASURED]` | `175,550 B / 235.8e6 B/s`; 235.8 MB/s at `:3921` |
| A8 | Macro-mask raw trade needs 22,400× the file's entire decode time | `[DERIVED]` over `[MEASURED]` | `166,823 × 1e5 ns = 16.68 s`; `16.68 / 7.445e-4 = 22,406`; trade at `SYNTH-PERF-FLEDGE.md`:175 and ledger `:3932` |
| A9 | `μ = 0.01` was never exercised by any binary | `[PROVEN]` | ledger `:1044` says `λ = μ = 0.01`; `g_stream_mu`/`g_stream_nu` occur **only** at `:1472` (`= 0.0`) and `:1553`. No flag, no env, no knob. Both lanes report this; it is the cleanest single defect in the record |
| A10 | `bench/jcost-validation-contract` and `deliverable/pre-reg-s6-1` are absent | `[PROVEN]` | neither `bench/` nor `deliverable/` exists. Absence ≠ never-existed; the *designation* "pre-registered" is simply not checkable here |
| A11 | `--hotop-budget=offx` silently **enables** v3 | `[PROVEN]` | `:4979` `(a.substr(15)!="off")` |

### 1.1 The decisive one, restated exactly

The vacuity of the J-agreement gate is the single most consequential result in either report and
**both reports understate it**. From `:1577`:

```cpp
if(best->bytes.size() <= cands[lw].bytes.size()*101/100) ++g_j_agree;
```

This is `size_t` arithmetic, so the right side is `floor(1.01·L_min) = L_min + floor(L_min/100)`.
Therefore:

> **Metric disagreement on Selector A requires `λ·Δc > floor(L_min/100) + 1`, with `Δc ≤ 45 − 10 = 35`.**
>
> **Exact vacuity region: `λ ≤ (floor(L_min/100) + 1) / 35`** — i.e. it is **stream-length dependent**,
> not the length-independent `λ ≤ 1/35` either lane reports.

At the largest real stream (15,680 B, `[MEASURED]`) this is `λ ≤ 157/35 = 4.486`. At the
near-`lits` stream sizes (≈100 KB) it is `λ ≤ ~28.6`. **This is `[PROVEN]` and it is stronger than
both reports.** The published "J-selection faithfulness **PASS** (100 % at the pre-registered
λ = 0.01) … the ≥80 % bar is cleared outright" (ledger `:1079-1081`) is **not a passed test**. The
gate would return 100 % at λ = 0.0222, 0.0285, 0.001, 0, on any corpus. An unfalsifiable gate can be
discharged, not cleared.

---

## 2. Where the two lanes diverge — and where **both** are wrong

### 2.1 Critic §5 contains two arithmetic errors in its "load-bearing result"

The critic states the vacuity bound as `λ ≤ 1/35 = 0.0222`. **`1/35 = 0.0286`, not 0.0222**;
`0.0222 = 1/45`. The slip is using `c_max = 45` (the absolute maximum value) where the *spread*
`c_max − c_min = 35` is required. The same slip propagates into the margins: the true margin at
λ = 0.01 is `2.86×`, not `2.22×`; the λ = 0.04 row is `1.40×` outside, not `1.7×`.

The conclusion survives (λ = 0.01 is inside either region). But a document whose *central* result
carries a wrong constant and two wrong margins is not evidence of a code-level proof, and the
§1.1 refinement above is the corrected version.

**Consequential corollary, adverse to the critic.** The critic's sharpest rhetorical move is that
the λ = 0.04 row "could have failed and did not — that is information." It is not. At λ = 0.04
metric disagreement requires `1.4 > floor(L_min/100) + 1`, i.e. **`L_min < 100 B`** — streams of
16–99 B only, the window just above the `:1550` 16-byte gate. Every recorded corpus stream is far
larger (`distance_code` 15,680 B, `match_len_class` 8,788 B). **λ = 0.04 was vacuous on the entire
recorded corpus too.** The three-row sweep (ledger `:1163-1168`, "byte-identical at λ = 0 /
0.01 / 0.04") is one unfalsifiable result reported three times, not three corroborations. The
critic's own §5 table gives it more credit than it has earned.

### 2.2 Critic §7.4 states the wrong cap, and in the wrong direction

The critic writes: *"the block cap (262,144 B) is 6.4× inside that [envelope]."* 262,144 B is the
**default** `block_size` (`:3769`), not the cap. The cap is revision-bounded:

```
4850: const uint64_t max_block = revision==1 ? (64ull<<20) : (128ull<<20);
5002: if(opt.parse!="ratio" && (opt.block_size==0 || opt.block_size>(64u<<20))) throw ...
```

so `--block` may be raised to **64 MiB**. At 64 MiB, `ΔL* = 5.9e-5 × 67,108,864 = 3,959 B`, which
**exceeds** the ">100 B" lower bound — a raw flip becomes arithmetically possible and the
"arithmetic necessity" framing of ledger `:4002-4011` does **not** extend across the legal block
range. The critic's envelope *number* (1.69 MB) is right; the "6.4× inside" claim is wrong by a
factor of 39.6 and points the wrong way.

**This does not disturb the recorded result** — the S6-1 manifest ran at default 256 KiB, where
`ΔL* = 15.47 B` against a `>100 B` gap, and no corpus stream approaches 1.7 MB. But it does
identify *where* the objective stops being inert, and that is the one genuinely new, actionable
thing either lane produced: **`λ`-gated selection becomes feasible for streams of order 1.7 MB and
above, a regime never sampled.** Space-bunny's N = 15,680 B bracket is the correct
real-content bracket; the critic's 256 KiB bracket is the most permissive vacuous one.

### 2.3 Critic §8.3's counter-proposal fails its own bar

The critic argues the constructive lane's Amdahl ceiling is "over-generalised" and that the
objective's true upside is ≈2.05× whole-codec decode. Two charges against it:

1. **`s ≈ 0.52` is not supported.** The critic writes "26 % + 26 % of decode." Ledger `:3924-3931`
   records the log breakdown as **crc32 44 % / macro materialization 26 % / token loop 11 % /
   concat-alloc-headers ~15 %**. The macro streams are **26 %, once**. Recomputing the critic's own
   formula at `s = 0.26`: `1/(1 − 0.26 + 0.26/60) = 1.344×` (and `1.345×` at r = 75).
   **1.34× is below the 1.53×–1.81× residual token-lane bar the critic cites two lines later
   (`18-future-decoder-architecture-space-bunny.md`:156, 723).** The counter-argument does not clear
   the threshold it names.
2. **The upside is uncharged.** A raw flip on those streams is not free: it is the *measured*
   `+166,823 B (+95 %)` / `+84,956 B (+48 %)` wire trade on a 175,550 B container
   (`SYNTH-PERF-FLEDGE.md`:175). Under the frozen exchange rate the whole file's decode is worth
   `0.01 × 744.5 µs = 0.0074 B` of `J`. Repaying `+84,956 B` therefore needs `λ ≈ 1.14e7 B/µs` —
   **`2.6e5×` the ledger's own `lits` reference of 44.6 B/µs**. The `2.05×` is economically
   self-refuting at any λ the shipped table can price, and the trade is recorded as
   *"measured dead on the incumbent cell"* because raw is ratio-infeasible under bar B.

This is the clearest doctrinal failure in either report: an upside projection presented without its
byte charge, in a project whose queue ranks on *"complete byte/cycle/RSS/code accounting"*
(`REMOTE-EXPERIMENT-QUEUE.md`:613, NQ-3 rationale).

### 2.4 Critic §2.4 mislabels the default path's defect

The critic finds a *"hidden scaling factor"* and concludes the default objective *"is a dimensional
error that happens to be conservative in one direction and aggressive in the other."* The implied
length `n* = 1000·c_flat/c_measured` spanning 5.0 KB–100 KB is `[DERIVED]` and correct (I reproduce
all seven values). But v2 is not **mis-scaled** — it is **dimensionally void**. It has no unit to be
mis-scaled *against*, and its own source comment (`:1467`) declares it "per-STREAM decode cost
units". A length-independent dimensionless rank weight is a legitimate heuristic ranking function;
it is not a mis-calibrated quantity. Space-bunny §4.3 ("`v2`'s `c` is a monotone rank code")
characterises it correctly and is the characterisation that determines the fix.

**The sharper statement, which neither lane makes, comes from `FORMAT.md` itself.** `FORMAT.md`:646
derives the shipped integers as *"integer tenths of the `C_decode` values used in earlier text:
raw 1, Huffman 2.2, … ctx-rANS 4.5"* — i.e. `c_flat = 10 × c_per-byte` exactly (verified: 1→10,
2.2→22, 2.0→20, 3→30, 3.5→35, 4→40, 4.5→45).

> **The v1→v2 bug is one clause long: the multiplier `×10` survived and the `×L` was dropped.**
> `FORMAT.md`'s own text proves the shipped table is a per-byte rate scale, and the code applies it
> as a per-stream total. That is a self-documented dimension error, established from `FORMAT.md`
> without appeal to the measured table at all — and it is why the repair is a unit declaration
> (C1), not a refit.

### 2.5 A stale number in the ledger that neither lane caught

Ledger `:1165` diagnoses the λ-sweep null as *"the time term ≤0.16 B/stream never overrides the
size winner."* Under the shipped additive table the term spans `0.01 × (45 − 10) = 0.35 B`. **0.16 B
is the multiplicative-era figure** (`0.04 × 4.0`, from the earlier-text rANS-4096 value at
`FORMAT.md`:647). The ledger's own summary of the null therefore describes a superseded objective.
Harmless to the conclusion — the diagnosis "never overrides" is right and the exact bound is
inert either way — but it is a fourth independent spec/ledger/binary disagreement and it means
`:1165` must not be cited as a magnitude anywhere. Owner: Track 06 / format lane.

### 2.6 Both over-weight the `--hotop-budget` parse hazard

`anvil.cpp`:4970-4992 — the `!= "off"` idiom is used by **twenty** flags (`--boundary`, `--negate`,
`--channels`, `--pnra`, `--stream-suite`, `--stream-ctx`, `--fused-decode`, `--hotop-rlzp`,
`--hotop-budget`, `--ariref`, `--ratio-context`, `--ratio-lines`, `--ratio-backend`, …). It is the
house convention, not an S6-1 defect. Space-bunny's O5 states the hazard correctly but as a
standalone finding overstates its distinctiveness; a fix scoped to `--hotop-budget` alone would be
inconsistent with the other nineteen. Real, low severity, and **not** an arm-identity blocker for
any redefinition, because the census logs the selected codec explicitly.

### 2.7 Critic R8 should be split, and its "necessity" leg is refuted

Critic R8 ("post-hoc tuning — in effect, yes") bundles three different assertions of different
strength. Unbundled:

- **(i) PROVEN — a gate-design defect.** At additive λ = 0.01 the J-agreement metric *cannot*
  return below 100 % (§1.1). A gate that cannot fail cannot be reported as cleared.
- **(ii) DERIVED — a process finding, not a tuning finding.** The sequence recorded in the ledger
  is: multiplicative form reports 76.7 % against a ≥80 % bar (`:1067`, `:1070-1073`); the form is
  "corrected" (`:1047`); the additive form reports 100 % (`:1079`); the bar is declared cleared.
  Because the corrected form is *provably incapable of failing*, "the fix made the gate pass" is
  near-unfalsifiable reasoning from inside the record. Compounded by A10: neither artifact said to
  contain the signed contract exists in the tree. This is diagnosable as post-hoc **without any
  allegation of intent** — a well-intentioned engineer who fixed a form, saw the bar pass, and
  moved on produces exactly this record.
- **(iii) NOT SUPPORTED — the λ argument.** Both lanes lean on λ being cut 0.04 → 0.01 (§6.1 /
  critic §6.2). Critic's "necessity" ground says choosing 0.01 *"specifically"* is what placed the
  constant in the unfalsifiable region. But **additive λ = 0.04 would also have passed** (ledger
  `:1168` records 100 %), and by §1.1 it too was vacuous on the whole corpus. The λ cut was
  **not** necessary to reach the passing result, so it is not the load-bearing post-hoc act.
  Space-bunny's "≈6,500× weakening at 16 KB" (`2,621 B → 0.40 B`) is arithmetically correct but is
  a statement about a term that is inert at both endpoints.

The defensible charge is the **form** change, not the λ change. R8 as written is stronger than the
evidence for (iii) and weaker than the evidence for (i).

**One number both lanes overstate:** the "16 % of `L`" for the v1 multiplicative term holds only
when `L ≈ n` (an incompressible 16 KB stream). It is not a general figure and should be quoted as
"`2,621 B` on a 16 KB rANS stream", not "16 % of `L`".

---

## 3. Historical gates — verdict

| record | status |
|---|---|
| ledger `:1038-1044` "Gate pre-registration … λ = μ = 0.01" | **NOT VERIFIABLE** (A10) and **internally contradicted** by A9. The pre-registration specifies a constant that no binary could execute |
| ledger `:1067` "J-agreement 100 % at the pre-registered λ = 0.01 … bar cleared outright" | **DISCHARGED, NOT PASSED.** Theorem (§1.1) |
| ledger `:1079-1081` "J-selection faithfulness: PASS" | **MUST BE RELABELLED.** A theorem about the code, not a measurement |
| ledger `:1163-1168` λ ∈ {0, 0.01, 0.04} byte-identical | **ONE** vacuous result reported three times (§2.1) |
| ledger `:1165` "time term ≤0.16 B/stream" | **STALE**, multiplicative-era (§2.5) |
| ledger `:4002-4011` "zero raw-flips: arithmetic necessity, machine-verified" | **SURVIVES for the recorded run**; the *necessity* is valid for `n < 1.7 MB` and the recorded streams are 15,680 B, but is **false across the full legal `--block` range up to 64 MiB** (§2.2). Demote "necessity" to "necessity at the executed configuration" |
| ledger `:1165`/`:4004` decode-term span "0.02–0.66 B" / "≤0.16 B" | **DISPUTED / irreproducible.** The retained manifest records no per-stream `raw_n`. `[PROJECTED]` reconciliation: 0.02 B ≡ raw bulk-pull at 20 KB; 0.66 B ≈ rANS at ≈11 KB — consistent with a raw↔rANS bracket at one nominal size, **not** the full seven-codec spread (which is 0.010–1.200 B over 10–20 KB). Neither lane's number may be cited |
| `FORMAT.md`:631-663 documents only Selector A | **DEFECT.** Selector B (size-proportional, flag-gated) and Selector C (pure min-`L`, 5 call sites, `:1587`) are undocumented. `FORMAT.md`:642-643's "per-byte decode-cost unit" is a category error, not a typo (§2.4) |
| ledger `:4073-4079` tie-break ruling + explicit non-retirement | **BINDING AND DECISIVE.** The gate "retires *stream budget at λ = 0.01 as a decode lever on this host*; **does NOT retire** (a) budget-level selection at materially higher λ (reference λ ≈ 44.6 B/μs … own pre-registration)" |

The last row is the hinge. **The record has already authorised, in its own frozen words, the
higher-λ question that the critic's redefinition proposes** — and has forbidden restoring legacy
tie-breaks post hoc. The critic's M2 (freeze one tie-break rule identically in both arms) is the
correct forward-looking analogue of that ruling, not a reversal of it. Both lanes are aligned
with the ledger here.

---

## 4. Q3 disposition

**RULING: `Q3` — CANCEL both constituted stages; RETAIN one redlined census under a new name.**

Both lanes reach this; the adjudication differs on the census's λ ladder and on what the census is
*for*.

### 4.1 CANCEL — Stage 1 (byte-only A0-vs-A1), as written

Its primary output is a **recorded, machine-verified zero** (ledger `:3998-4001`: log 175,550
exact, json 121,316, jsonl 222,381, sqlite 366,019 — size-identical; 139/139 flips exact-`L`
precision ties, `:3971-3974`). Re-running it re-prints the ledger. Queue invariant: *"a downstream
job whose premise is invalidated by an upstream result is cancelled, not run for completeness."*
Independently fatal: the arms share the numeral `0.01` while scoring in **untyped rank units**
(A0) and **B/µs** (A1). The comparison is well-defined *as code* and uninterpretable *as
economics* — it inherits the defect it was constituted to arbitrate. `[PROVEN]`.

### 4.2 CANCEL — Stage 2 (paired timing), pre-emptively

Two independent bars. (i) Track 06's own K5 requires `tax ≤ 2 %` on both arms against a **measured
zero-byte-change control band of +2.3–8.1 %** (ledger `:3918-3919`) — inadmissible by 1.15–4.05×
before any arm runs. (ii) With zero raw-flips no codec *choice* changes, so there is no
decode-cost delta to attribute; the Amdahl ceiling on the realised flips is `s × 0.1844` with
`s ∈ {0.002, 0.003}` ⇒ **0.037 %–0.055 %**, against a 2.3 %–8.1 % floor — 42×–219× below noise.
An Amdahl statement, not a sample-size problem. `[DERIVED]` over `[MEASURED]`.

### 4.3 Do NOT retain a Q3 whose stated purpose is "choose between A0 and A1"

Per NQ-2a (`:446-456`) Q3 is already confined to `adopt-class {engineering}`, no mechanism claim;
Track 06's own critic returned *"Novelty: NIL. Adopt-class."* A job that cannot license a choice
and is barred from a claim is **not dispatchable at any budget**. Per NQ-2b, an A0-vs-A1 result
MUST NOT be written up as an improved decode-cost model — that is S4 reborn, and S4 is closed
prior art per NQ-2 (`:434-444`). This is a laundering hazard the *review* identified and the
*experiment* would re-open.

### 4.4 RETAIN — one redlined census, byte-only, under a new name (`Q3-CENSUS`, not `Q3`)

Not a re-measurement: a **deterministic, local, zero-network, sealed-byte-free** log-emission job
that costs no runner. It exists because the artifact it produces **does not exist in the repository**:
`:1578` logs only `{chosen, l_winner, chosen_L, min_L}` for the **two chosen arms**. The
objective-level disagreement rate is therefore **not computable from retained evidence**, and the
argument in §2.2 cannot be closed without it. Emit, per stream:

1. `(block, stream, role, N)` and, **for every candidate**: `L_c`, `c_const`, `ns_c`, `J_const`,
   `J_prop`. This is the substrate for every future cost-model question.
2. `argmin` under A0, under A1, and under pure-`L`, with the **flip type separated**: raw-vs-coded
   (real) from rANS-precision (exact-`J`-tie, broken by candidate order — a *tie-break*, not a
   win, a direct consequence of flattening 5.40–6.62 ns/B to 6.0).
3. Disagreement **stratified by length bucket** (`<16`, `16–512`, `512–4096`, `4096–16384`,
   `≥16384`). Mandatory, because v2's bias is *provably sign-flipping in `N`*: v2's raw→rANS-4096
   price is the constant `0.01 × (40 − 10) = 0.30 B`, while v3's is `N × 5.9e-5 B`, equal at
   `N* = 5,085 B`. **Note:** those two quantities are *not* comparable — 0.30 B is a rank
   tie-break price, `N × 5.9e-5 B` a time price. `N* ≈ 5,085 B` is where two conventions coincide,
   not a physical crossover. Track 06's "mis-pricing 51×/19.7×/10×" ratios are ratios of
   incommensurable quantities and must not be cited. What survives is the sign-flip theorem only.

**Corrected λ ladder** (this is where the adjudication overrides both lanes). Critic M3 proposes
`{1e-5, 6.5e-5, 4.46e-2 B/ns}`. **The middle rung is a null by construction.** `6.5e-5 B/ns` is
`λ_required` for a 100 B gap evaluated at the **256 KiB default block**, i.e. at a stream length the
corpus never produces; the largest real stream is 15,680 B, where the same 100 B gap needs
`> 1.081 B/µs = 1.08e-3 B/ns` (**108×** shipped), and 20,000 B needs `> 8.48e-4 B/ns` (**85×**).
Corrected ladder:

| rung | λ (B/ns) | × shipped | what it is |
|---|---|---|---|
| control | `1e-5` | 1× | the inert shipped value; preserves the recorded null as a data point, not an assumption |
| **first useful** | `1.1e-3` | **108×** | minimum that could flip a `>100 B` gap at the largest **real** stream |
| reference | `4.46e-2` | 4,462× | the ledger's own `lits` figure (`:4010`) |

The two useful rungs are 40× apart — enough to separate "inert" from "active". **Do not put the
macro-stream raw trades in the ladder**: they need `λ ≈ 1.14e7 B/µs` (§2.3) and are measured dead.

### 4.5 Not required

No new codec, no mode, no wire change, no build, no runner, no clock derivation, no re-run of the
recorded A/B (its null stands), no change to the AOC/DDMC censuses. The byte gate must be the
ledger's **bar B** ceiling, because the recorded raw trades cost `+166,823 B (+95 %)` and
`+84,956 B (+48 %)` on a 175,550 B container — a λ sweep must not be able to "find" a
ratio-infeasible win.

### 4.6 One hazard, binding

Making `c` size-proportional in the **default** path is the correct dimensional repair (§2.4) but it
is **byte-visible**: it changes which codec is selected on real content. It must not ride inside a
documentation change. Sequence it **after** `Q3-CENSUS`, with the tie-break frozen, under its own
pre-registration. Shipping it as a "unit correction" would be the fifth instance of the §2.7(iii)
pattern, and the ledger has already ruled (`:4073-4076`) that post-hoc selector changes are barred.

---

## 5. Minimum non-duplicative recommendation

Seven items. Each is documentation- or log-only and changes no emitted byte. Nothing here is
already stated by either lane in a form that can be adopted as-is.

| # | action | why it is not duplicative |
|---|---|---|
| **R1** | **Relabel the J-agreement row** in `RESEARCH_LEDGER.md`:1079-1081 from **PASS** to **DISCHARGED — VACUOUS AT λ = 0.01**, and publish the exact bound `λ ≤ (floor(L_min/100)+1)/35`. | Neither lane states the `L_min`-dependent form (§1.1), and the label change is the one edit that removes a false claim from the permanent record |
| **R2** | **Split the numeral in the format doc**: every occurrence of `λ` names its selector; `FORMAT.md`:642-643's "per-byte decode-cost unit" is replaced by "**dimensionless per-codec rank weight, no time axis**"; `FORMAT.md`:803-806 gains the missing `B/µs` unit; document Selectors B and C, which `FORMAT.md`:631-663 omits entirely | Extends C1/C2 to `FORMAT.md`:631-663's undocumented-selector gap (both lanes missed it) |
| **R3** | **Record the `×10`-survives-`×L`-dropped mechanism** (from `FORMAT.md`:646's own derivation) as the root cause of v2 | Sharper than "unit incoherence" and sharper than "hidden scaling factor"; it is what makes R2 a declaration rather than a refit (§2.4) |
| **R4** | **Correct ledger `:1165`** ("≤0.16 B" → 0.35 B span) and annotate `:4002-4011` "arithmetic necessity" as valid for `n < 1.7 MB` **at the executed 256 KiB configuration**, not across the 64 MiB legal block range | Two stale/over-broad ledger statements; the second is the *only* place either lane found an error in the direction of the recorded null |
| **R5** | **File the corrected λ ladder of §4.4** and reject `6.5e-5 B/ns` as a rung | Overrides critic M3 on arithmetic (§4.4) |
| **R6** | **Record that `μ = 0.01` was never in any binary**, against ledger `:1044` | Both lanes report it; neither connects it to the *pre-registration's* validity, which is where it bites (§3) |
| **R7** | **State the `>= 1.7 MB` stream regime as the one place a λ-gated objective could bind**, and mark it never sampled | The single new finding of this adjudication (§2.2); it is the re-entry trigger for any future higher-λ job |

**Explicitly not recommended** (each was proposed by a lane and is declined here):

- **Decline critic M1** as written. Promoting the `ns/B` table to normative *in the default path* is
  the byte-visible change of §4.6, not a documentation act. Split it: declare units now, change the
  default selector later under its own pre-registration.
- **Decline the critic's §8.3 upside projection entirely** (§2.3): wrong `s` by 2×, below its own bar
  at the corrected `s`, and uncharged on bytes.
- **Decline any λ re-specification inside this program** (space-bunny C4 is right). A rate/decode
  exchange rate is an application-level stake; ANVIL has no deployment that states it, and choosing
  one after outcomes are visible is a coordinator-level re-specification of a frozen EXP. L
  constant with its own pre-registration. The §4.4 ladder is a **diagnostic sweep**, not an
  adoption — and must be labelled that way in its pre-registration.
- **Decline any re-run of the recorded A/B** and any restore of legacy tie-breaks
  (ledger `:4073-4076`).

---

## 6. Falsifiers

Each recommendation carries the observation that kills it. If a falsifier fires, the recommendation
is withdrawn, not adjusted.

| # | falsifier |
|---|---|
| **F1** (R1) | Exhibit a stream where `encode_stream` at λ = 0.01 selects a candidate whose `L_best > L_min + floor(L_min/100)` while `λ·(c_min − c_best) ≤ 0.35`. Impossible by §1.1; an exhibit refutes the theorem, not the label. Alternatively: exhibit a pre-verdict measurement of the metric at additive λ = 0.01 that returned **<100 %**. |
| **F2** (R2) | Exhibit any record in which `FORMAT.md`'s "per-byte" reading of `c` is used to **price** a trade (not rank). If every use is ranking, the category error is documentation-only and R2 is confirmed at low priority. |
| **F3** (R3) | Exhibit a `FORMAT.md` revision where the shipped table is *not* `10 ×` the earlier-text per-byte table. That would relocate the root cause to the 0.04→0.01 λ change, and R3 must be re-derived from scratch. |
| **F4** (R4) | Exhibit a corpus with a raw-vs-entropy gap `< 3,959 B` **and** a stream length `> 1.7 MB`. That flips the retained "arithmetic necessity" from an explanation into a coincidence and re-opens Q3 on the *original* arms. Conversely, if the true gap (not the `>100 B` lower bound) is `≥ 3,959 B`, the necessity generalises across all legal block sizes and R4's second half softens to a footnote. |
| **F5** (R5) | Exhibit a stream with `N ≤ 20,000 B` and a raw-vs-entropy gap `≤ 17 B`. Then `6.5e-5 B/ns` is discriminating and critic M3's ladder stands unchanged. I predict no such stream exists — the recorded gaps are `>100 B` — but the prediction is not a proof. |
| **F6** (R6) | Exhibit a git history in which `g_stream_mu` was non-zero with a setter. Absent that, ledger `:1044`'s `μ = 0.01` is a documentation-only error and the pre-registration is merely imprecise rather than unexecutable. |
| **F7** (R7, and the whole redefinition) | Produce, from `Q3-CENSUS`, **any** stream whose `J`-argmin differs from the `L`-argmin at `λ ∈ {1e-5, 1.1e-3, 4.46e-2} B/ns`. One such stream **kills** the "both live objectives are length objectives with a tie-break" standing statement, invalidates the "≥85×/108×" bracket, and requires the Q0c publication to be re-derived rather than re-run. This is the single decisive experiment and it is byte-only and free. |
| **F8** (governs the whole adjudication) | Exhibit a signed pre-registration artifact establishing that the additive form **and** λ = 0.01 were both binding before the 76.7 % result was observed. That does not make the gate falsifiable — §1.1 stands regardless — but it downgrades §2.7(ii) from "post-hoc in effect" to "form corrected on contract", and the historical-gate finding narrows to the μ error (A9) and the missing artifacts (A10) alone. |
| **F9** (bounds the critic's §8.3 further) | Exhibit a macro-stream raw trade below bar B. Then §2.3's charge is void and the `s = 0.26` upside becomes `1.34×` of *free* speedup. This is **not** expected — `SYNTH-PERF-FLEDGE.md`:175 records both macro trades as ratio-infeasible and ledger `:3933-3936` records that the Linux `+19.8 KB ⇒ +0.12 GB/s` result does not transfer — but it is the only path by which the critic's redefinition becomes an engineering bet rather than a census. |

---

## 7. Residual opens (owner-named; none blocks the §4 ruling)

| id | item | owner |
|---|---|---|
| O1 | `λ_required` not uniquely determined: `>100 B` is a lower bound; the recorded 44.6 B/µs back-solves to an unprinted ≈5.3 KB gap at N = 20,000. Direction and order (≥85×) certifiable; the point value is not | Track 06 |
| O2 | Recorded "0.02–0.66 B" decode-term span is irreproducible from retained evidence; reconciliation is `[PROJECTED]` | Track 11 |
| O3 | v1's multiplicative `J = L + λ·C_decode·L` (λ = 0.04) was dimensionally coherent and was superseded on implementation-fidelity grounds (`FORMAT.md`:635-637), not validity. No reason recorded | Track 06 / format |
| O4 | Ledger `:3906-3909`'s own recorded limitation: the flat 6.0 rANS fit misses a 5–12 % size-dependent symtab effect. Never patched post-hoc, by rule. `c` carries a known inaccuracy exactly where the break-even is tightest | Track 06 / decode-perf |
| O5 | `>= 1.7 MB` stream regime never sampled; `--block` may reach 64 MiB. The only regime where a λ-gated objective can bind | Track 06 / 18 |
| O6 | The `!= "off"` permissive-parse idiom across 20 flags (`:4970-4992`) makes arm identity unrecoverable from a flag *value*. Systemic, not S6-1-specific | Track 06 / arch |
| O7 | Ledger `:1165` "≤0.16 B/stream" is a multiplicative-era figure; `:4002-4011` "arithmetic necessity" is configuration-scoped (§2.2, §2.5) | Track 06 |

---

## 8. Compliance

| constraint | status |
|---|---|
| 0 codec invocations / 0 corpus bytes / 0 benchmarks / 0 network | met |
| 0 production or coordinator file edits | met — `src/anvil.cpp`, `FORMAT.md`, `RESEARCH_LEDGER.md`, `docs/swarm-2026-10-02/COORDINATOR-STATE.md`, `REMOTE-EXPERIMENT-QUEUE.md` read-only |
| 0 git operations | met |
| PROVEN / MEASURED / DERIVED separated | met (§0 labels, applied throughout) |
| files read | `Q0C-LAMBDA-C-SPACE-BUNNY.md`, `Q0C-LAMBDA-C-CRITIC.md`, `06-entropy-codesign-space-bunny-q3-preflight.md`, `REMOTE-EXPERIMENT-QUEUE.md`, `COORDINATOR-STATE.md`, `SYNTH-PERF-FLEDGE.md`, `18-future-decoder-architecture-space-bunny.md`, `src/anvil.cpp` (windows 1460–1704, 3760–3789, 4655–4694, 4950–4994 + symbol search), `FORMAT.md` (628–672, 795–834), `RESEARCH_LEDGER.md` (1033–1087, 1158–1187, 3880–4020, 4040–4084) |
| file written | this one |
