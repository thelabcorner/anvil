# Q3 Selector Preflight — Track 06 · Space Bunny Free

**Artifact requested as:** `Q3-SELECTOR-PREFLIGHT.md`
**Written as:** `docs/swarm-2026-10-02/06-entropy-codesign-space-bunny-q3-preflight.md`
(track-prefixed to match the other artifacts in this directory; same document)
**Date:** 2026-10-02 · **Live tree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty
**Codec execution performed: NONE.** No encode, no decode, no benchmark, no timing, no fuzz.
Every number below is arithmetic on **retained artifacts** already in the repository.
**No commits / pushes / resets / stashes.**

## 0. Verdict in one line

> **CANCEL Q3 timing.** The retained S6-1 selector manifest already measures the A0-vs-A1
> outcome: **complete bytes are identical** on all 23 sampled files, all 139 flips were
> **exact-`L` ties**, and the measured Amdahl ceiling on whole-codec decode is **0.037 %–0.055 %**
> against a **2.3 %–8.1 %** resolution floor — a factor of **42×–219×** below noise. The signed
> mid-range prediction is a **0.013 %–0.019 % decode *regression***, not a win. **Retain only
> the unit/documentation correction.**

---

## 1. Retained artifacts relied on (all pre-existing, unmodified)

| # | Artifact | Location |
|---|---|---|
| **E1** | S6-1 selector manifest: **792 selections = 88 blocks × 9 streams, 23 files**; **ZERO raw-flips**; `macro-dvar` never leaves raw; **139** flips, **ALL exact-`L` ties** (`legacy L == budget L` in 139/139), transitions exclusively rANS precision swaps, every flip `logical_eq=1 ∧ dec_eq=1`; zero flips on `synth-arith.bin`, `random.bin`, `generated.repeat.jsonl` ⇒ flips in 20/23 files. | `RESEARCH_LEDGER.md`:3965-3979 |
| **E2** | Complete-bytes equality, budget-on vs legacy, every corpus file: `log 175,550` (exact), `json 121,316`, `jsonl 222,381`, `sqlite 366,019` ⇒ **885,266 B** over the four record files. "bar B met at size-equality"; hashes differ, byte-difference mechanism fully attributed to precision flips. | `RESEARCH_LEDGER.md`:3998-4001 |
| **E3** | Already-recorded timing for this exact A/B: budget-on `log 234.4 MB/s` vs state-A band `231.6 / 235.8 / 240.4` ⇒ **UNCHANGED WITHIN NOISE**; CLI-scale median-3 **OFF 206.8 vs ON 201.0 MB/s**, inside the record's **5 %–19 %** noise band. | `RESEARCH_LEDGER.md`:3991-3997 |
| **E4** | Mode-15 decode floor profile on `generated.log`: crc32 **44 %**, eager macro-stream materialisation **26 %**, token loop **11 %**, **opcode entropy pulls 0.3 %**, concat/alloc/headers **~15 %**. "THE HOT PATH IS CLEAN." | `RESEARCH_LEDGER.md`:3911-3936 |
| **E5** | Zero-byte-change control variants measured **+2.3 %–8.1 %** ⇒ the resolution floor for any decode-timing claim on this cell. | `RESEARCH_LEDGER.md`:3927-3930 |
| **E6** | Measured per-codec decode cost, ns/B, median-7, real mode-15 content: rANS-4096 **5.93–6.62**, rANS-512 **5.44–6.55**, rANS-256 **5.40–5.99**; raw 0.04–0.10 bulk / 2.59–2.66 byte; Huffman 4.08–9.0; defexc 3.14–3.25; ctx pull ~7–8. Verdict on the table: "right ORDER, wrong GAPS". | `RESEARCH_LEDGER.md`:3883-3904 |
| **E7** | Flipped-stream in-block cost: "macro-types **26→46 cyc/B**, rANS-4096 symtab pressure"; "types + dflags + opcodes are **0.2 %–0.3 %** of decode ⇒ invisible end-to-end (**~0.001 % stakes**)". | `RESEARCH_LEDGER.md`:3991-3996 |
| **E8** | Flag provenance hazard: `--hotop-budget=<anything but "off">` enables the budget path; `encode_stream_budget` reachable from **exactly one** call site; default `g_hotop_budget=false`. | `src/anvil.cpp`:4979, 1617, 2868, 3786, 4664 (re-verified this session) |
| **E9** | Units statement under dispute: `FORMAT.md` documents `c ∈ {10..45}` integer cost units while `src/anvil.cpp:1618` ships `kBudgetNsPerByte = {0.1, 6.0, 6.0, 6.0, 4.3, 3.2, 7.5}`. λ = 0.01 ⇒ **1 byte ≡ 100 µs of decode**. | `SYNTH-PERF-FLEDGE.md`:176; `FORMAT.md`:636-663; `src/anvil.cpp`:1618-1626 |

---

## 2. (a) Bytes represented by the 139 flipped streams

### 2.1 What the retained manifest does **not** contain

The manifest records **selection counts and codec IDs**. It does **not** record per-stream
`raw_n` / decoded byte counts. Therefore a byte-share figure for the flipped set **cannot be
computed** from retained artifacts, and I decline to invent one.

### 2.2 What *is* computable

| quantity | value | derivation |
|---|---|---|
| selections in the sample | **792** | E1 |
| flipped selections | **139** | E1 |
| selection-share of flips | **139 / 792 = 17.55 %** | arithmetic |
| files with ≥1 flip | **20 / 23** | E1 (3 files had zero flips) |
| **decode-TIME share of the flipped streams** | **0.2 %–0.3 %** (E7), i.e. `~0.001 %` end-to-end stakes | E7 |

**The second row is the decisive one and it is measured, not derived.** Byte share is the wrong
axis for this question: a stream's decode cost scales with the **ns/B** of the codec applied to it
(E6), and the flipped streams' *time* share is 0.2 %–0.3 % (E7). A byte-share figure could only be
*worse* (less favourable) if the flipped streams were disproportionately small, and it could not
make the timing case any better. **The Amdahl bound in §4 is therefore computed on the measured
time share, not on an unavailable byte share.**

**Structural consistency check that the byte share is small.** The flipped codecs are exclusively
rANS-4096/512/256 — never raw (E1). The streams carrying real volume in a mode-15 block are
`macro-masks` and `macro-resid` (E1/E2 byte-difference attribution; measured raw trades of
+166,823 B and +84,956 B, `SYNTH-PERF-FLEDGE.md`:175), while E7 names the flipped set as
`types + dflags + opcodes` — three structurally tiny, low-alphabet streams. `macro-dvar` is
**already raw** and never left raw (E1). Consistent; no contradiction in the record.

---

## 3. (b) Codec transition counts

Directly from E1:

| transition | count | share of 139 | destination direction |
|---|---:|---:|---|
| `m3` (rANS-256) → `m2` (rANS-512) | **93** | 66.9 % | toward **larger** table (4096→512→256 ordering is by `L` in the wire enum; 512 > 256 in `TOT`) |
| `m3` (rANS-256) → `m1` (rANS-4096) | **35** | 25.2 % | toward **largest** table |
| `m2` (rANS-512) → `m1` (rANS-4096) | **11** | 7.9 % | toward **largest** table |
| **total** | **139** | 100 % | — |
| flips involving raw (mode 0) | **0** | 0 % | — |
| flips involving Huffman / defexc / ctx | **0** | 0 % | — |

**Sum check:** `93 + 35 + 11 = 139`. ✔ Arithmetic verified.

**Two facts follow immediately, and both are adverse to Q3:**

1. **Every flip is a pure precision tie-break.** Because all three rANS precisions carry the
   *same* 6.0 ns/B in `kBudgetNsPerByte` (E9), they are **exact J-ties** under A1 and are resolved
   by strict `<` to the earliest-added candidate. So these are not economic decisions at all — they
   are artefacts of tie order. (This is `AF-6` in my main report; E1 confirms it on real content.)
2. **All three directions move toward a *more expensive* table.** rANS-4096 > 512 > 256 in both
   `TOT` (table-build size) and measured mid-range ns/B (E6). So the A1 arm's realised selections
   were, if anything, **mildly worse** for decode than A0's — which is the opposite of the
   "rate surplus buys decode speed" thesis the A1 arm was meant to advance.

---

## 4. (c) Amdahl upper bound on whole-codec decode improvement

Let `s` = the flipped streams' share of whole-codec decode time = **0.002–0.003** (E7).
Whole-codec normalised decode time `T = 1`. A flip from codec X to codec Y rescales that stream's
cost by `ns_Y / ns_X`.

### 4.1 Unconstrained upper bound (extreme range endpoints — deliberately generous)

Largest ratio obtainable from any single allowed flip, using the cheapest observed source and the
most expensive observed destination from E6:

```
ratio_max = max(ns) / min(ns) = 6.62 / 5.40 = 1.2259x        (rANS-4096 high → rANS-256 low)
T' = (1 - s) + s / 1.2259
whole-codec improvement = 1 - T' = s * (1 - 1/1.2259) = s * 0.1844
```

| `s` | whole-codec improvement |
|---|---|
| 0.002 | **0.037 %** |
| 0.003 | **0.055 %** |

This is an **upper bound that is not attainable**: it assumes every flip simultaneously realises
the most favourable endpoint of both measured ranges, which the transition counts in §3 forbid
(100 % of flips go the *other* way).

### 4.2 Signed mid-range prediction (the realistic sign)

Mid-points from E6: rANS-4096 **6.275**, rANS-512 **5.995**, rANS-256 **5.695** ns/B.

| transition | count | mid-range cost change |
|---|---:|---:|
| 256 → 512 | 93 | `5.995 / 5.695 = 1.0527` ⇒ **+5.27 %** |
| 256 → 4096 | 35 | `6.275 / 5.695 = 1.1019` ⇒ **+10.19 %** |
| 512 → 4096 | 11 | `6.275 / 5.995 = 1.0467` ⇒ **+4.67 %** |

Weighted: `0.669·5.27 + 0.252·10.19 + 0.079·4.67 = 3.526 + 2.568 + 0.369 = 6.46 %` average
increase in per-byte cost on the flipped streams.

```
whole-codec change = s * 6.46%  =  0.013 % – 0.019 %   SLOWER (regression, not a win)
```

Sign is **negative**: A1's realised flips make decode marginally *worse*, not better.

### 4.3 Comparison against the resolution floor

| comparison | value |
|---|---|
| best-case signal (unconstrained) | **0.055 %** |
| signed prediction (mid-range) | **0.013 %–0.019 % regression** |
| measured noise band, zero-byte-change controls (E5) | **2.3 %–8.1 %** |
| ratio, floor ÷ best-case signal | **42×** (2.3 / 0.055) |
| ratio, floor ÷ signed prediction | **121×–177×** (2.3/0.019 … 8.1/0.013) |

**Independent confirmation from the record itself:** E3 already ran this A/B on timing —
`log 234.4 MB/s` inside a state-A band of `231.6 / 235.8 / 240.4`, and CLI median-3 `OFF 206.8
vs ON 201.0 MB/s` "inside the record's 5 %–19 % noise band". The measurement the arbiter would buy
**has already been bought**, and its answer is "UNCHANGED WITHIN NOISE". The Amdahl bound above is
not a prediction of that null; it is the arithmetic showing the null was **arithmetically
guaranteed**.

---

## 5. (d) Are complete bytes provably identical?

**Yes — for the recorded arms on the sampled 23-file / 88-block / 792-selection population.**
The proof is two independent legs, both retained:

1. **Per-flip exactness.** All 139 flips satisfy `legacy L == budget L` (139/139), i.e. each flip
   is an **exact-length tie** between two rANS precisions (E1). Each flip also satisfies
   `logical_eq=1 ∧ dec_eq=1` — the two encodings decode to byte-identical logical content (E1).
2. **Per-file size equality.** Budget-on output equals legacy output on every corpus file:
   `log 175,550` (exact), `json 121,316`, `jsonl 222,381`, `sqlite 366,019` (E2). "bar B met at
   size-equality."

Since `complete bytes = Σ over all selections of the chosen encoding's length`, and no selection's
chosen length changed, **complete bytes are invariant.** Hashes differ, but only in the precision
fields — which is exactly why byte-equality and hash-inequality are both true (E2).

**Three scope limits on that "provably", stated so this is not over-read:**

- The manifest is a **23-file sample** (88 blocks × 9 streams), not the full corpus
  (`SYNTH-PERF-FLEDGE.md`:266 makes the same observation).
- It was produced by a **SEL-logging diagnostic build over `fc23d9a`**, not the current dirty tree;
  per the project's own binary rule a src move invalidates sha-specific results until re-stamped.
- It covers the **mode-15 hot-op path only** (the single `encode_stream_budget` call site, E8).
  It says **nothing** about the constant-`c` selector's behaviour on the other ~10 call sites.

Within those limits, byte-identity is established. Outside them it is **projected**, not measured.

---

## 6. Consequence for the Track-06 arbiter (verdict change)

My main report's **Addendum A6** scheduled Stage 2 (paired same-job timing) for the A0-vs-A1
comparison, conditional on K1/K2 firing. This preflight **pre-empts that stage**:

| arbiter stage | disposition after this preflight |
|---|---|
| Stage 2 — A0-vs-A1 **decode timing** | **CANCEL.** Effect size 0.037 %–0.055 % (ceiling) / 0.013 %–0.019 % (signed) vs a 2.3 %–8.1 % floor. Not a CI question. E3 already answered it. |
| Stage 1 — A0-vs-A1 **byte-only census** | **RETAIN — but for a narrower reason than "it settles bytes".** See §6.1. The retained manifest records only the two *chosen* arms' codecs and lengths; it does **not** record `L_c` for every candidate, so the **objective-function-level** disagreement rate is **not** computable from it. What is already settled is the *realised-selection* outcome on the sample. The census still has a distinct job: it extends coverage (full corpus, not 23 files; all ~11 call sites, not the single budget site) and measures disagreement as a property of the *objective* rather than of one sample. |
| Stage 1 — **AOC** conditional-entropy census (byte-only) | **UNAFFECTED — this is now the only substantive Stage-1 payload worth running.** It is a different question: a *new context* on the opcode stream, not a tie-break among precisions. K7-revised stands. |
| Stage 1 — **DDMC** distinct-table census | **UNAFFECTED**, but low value: with 256 KiB blocks the per-block table census is cheap to fold into the AOC run if one job happens anyway. |

**Net effect: the shared 05/06/18 substrate shrinks from "three arms, two stages" to "one
byte-only job carrying the AOC and DDMC censuses, plus a documentation change."** That is a
material reduction in the cost of the shared arbiter, and it is the main thing this preflight
returns to Tracks 05, 06, and 18.

### 6.1 Why the Stage-1 census survives even though its byte outcome is now predictable

Stated precisely so this is not read as a contradiction of §0:

| question | answered by retained artifacts? |
|---|---|
| Did the A0-vs-A1 arms produce **identical complete bytes** on the sample? | **YES** — E2, §5 |
| Were all flips **exact-`L` ties**? | **YES** — E1, 139/139 |
| Is the **decode-time effect** large enough to measure? | **NO** — §4; bounded 42×–219× below floor |
| What is the **argmin disagreement rate of the two objective functions** across the full candidate set? | **NO — not computable from retained artifacts.** The manifest records `legacy_codec`, `arm_codec`, `legacy_L`, `arm_L`; it does **not** record `L_c` for every candidate, nor per-stream `N`, nor the bucket stratification. |
| Does the behaviour hold on the **full corpus** and on the **other ~10 `encode_stream` call sites**? | **NO.** The manifest is a 23-file sample of the single mode-15 budget call site. |

So the census is retained for **coverage and for the function-level statistic**, not because its
byte outcome is in doubt. Its **predicted** byte outcome is "identical" (E2 generalised), and that
prediction is itself the falsifiable pre-registration: if the census finds a ≥ 1 % complete-byte
delta (K3), the retained sample was unrepresentative and that result **overrides** E2's
extrapolation to the full corpus.

---

## 7. What is retained: unit / documentation correction only

No code change, no codec change, no new flag, no new mode, no new ISA. Documentation only:

1. **`FORMAT.md` — units-correct rewrite of the selector objective.** As shipped, the spec
   documents `J = L + λ·c + μ·2 + ν·c` with integer cost units `c ∈ {10..45}` while the binary
   ships `kBudgetNsPerByte = {0.1, 6.0, 6.0, 6.0, 4.3, 3.2, 7.5}` on a different, size-proportional
   objective reachable from one call site (E8, E9). The spec must state, unambiguously:
   - which objective is **normative on the default path** (answer: the length-independent constant-`c` one);
   - the **units** of λ (`0.01` bytes per microsecond of decode ⇒ **1 byte ≡ 100 µs**);
   - that the size-proportional objective is a **separate, flag-gated, single-site** feature, not
     the documented default;
   - that the default objective's decode term is **length-independent by construction**, so it is
     a tie-break weight, not a decode-cost model.
2. **Same section, the rANS-tie disclosure.** Because all three rANS precisions are 6.0 ns/B (E9),
   any size-proportional arm **cannot distinguish them**; flips there are tie-breaks resolved by
   insertion order. A reader must not be able to interpret a precision flip as an economic decision.
3. **Recorded null, for future arms.** State the E1/E2/E3 result in the format/ledger record so no
   later brief re-runs the A0-vs-A1 timing (this preflight's entire purpose).
4. **Optional, cheap, and worth one line:** a note that the argument-parse asymmetry (`!= "off"`)
   makes `--hotop-budget` a provenance hazard, and that Track 06 freezes exactly one enabling
   spelling (`on`) for experimental arms.

**Nothing above changes compressed bytes.** It is a specification-accuracy fix, and it is
labelled as such: **engineering/documentation correction, adopt-class, zero novelty claim.**

---

## 8. Recommendation

| item | ruling |
|---|---|
| **Q3 timing (rerun A0-vs-A1 selector choices with paired timing)** | **CANCEL** |
| **Q3 byte-only rerun of A0-vs-A1** | **RETAIN, narrowed** — retained for full-corpus / all-call-site coverage and the function-level disagreement statistic (§6.1), **not** for the byte outcome, which is predicted identical; byte-identity is the falsifiable pre-registration, not an assumption |
| **Unit/documentation correction to `FORMAT.md` + rANS-tie disclosure** | **RETAIN — the only surviving deliverable** |
| **AOC byte-only conditional-entropy census (K7-revised)** | **UNAFFECTED — retain; now the primary Stage-1 payload** |
| **DDMC distinct-table census** | **RETAIN as a cheap rider on the same byte-only job** |

**Basis for CANCEL, in one sentence:** the retained manifest already establishes byte-identity
(d) and exact-tie flips (b), and the measured time share of the flipped streams (0.2 %–0.3 %)
combined with the measured per-codec ns/B ranges (E6) bounds the whole-codec decode effect at
**0.037 %–0.055 %** — signed prediction **0.013 %–0.019 % regression** — against a **2.3 %–8.1 %**
resolution floor, i.e. **42×–219× below noise**, and the timing run has already been executed and
recorded as UNCHANGED WITHIN NOISE.

**Claim hygiene:** every number in this document is arithmetic on retained artifacts, labelled by
source. No local codec was executed. No throughput or ratio figure here is new; none is citable as
a new measurement, and none needs to be — the point is that the question is already closed.