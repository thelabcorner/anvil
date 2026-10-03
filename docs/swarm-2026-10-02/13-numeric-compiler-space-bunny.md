# Track 13 — Numeric / Floating Representation Compiler

**Agent:** Space Bunny Free (constructive inventor)
**Track:** 13 — reversible typed lanes / mixed numeric data
**Date:** 2026-10-02
**Status:** **REVISED** (supersedes my interim checkpoint of the same path). Rebuilt around a cheap anatomy/census first, per coordinator instruction. No existing file modified. No commit/push/reset/clean/stash/restore/rebase. No local compression benchmark, sweep, or fuzz campaign.
**Branch/worktree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty (pre-existing).

Derivation labels per `docs/gate-verify-regime-i9.md`: `[measured]` recomputable from a named artifact with the stated command; `[in-session]` computed by the isolated prototype in this session (pure arithmetic, in-memory, no codec, no file); `[derived]` arithmetic on measured inputs, shown; `[projection]` predicted; `[design]` proposed, unmeasured; `[lit]` external published figure, quoted, never re-measured.

**Files written (the only two):**
- `docs/swarm-2026-10-02/13-numeric-compiler-space-bunny.md` (this report)
- `prototypes/swarm-2026-10-02/13-numeric-compiler/space-bunny/nac_census.py` (isolated, unwired, not referenced by any build target or workflow)

---

## 1. What changed from my checkpoint, and why

| # | Checkpoint position | Revised position | Cause |
|---|---|---|---|
| 1 | Mechanism = CANL typed-lane codec with copy-anchored state rebase; recommend **PILOT** | **KILL the typed-lane compiler as a novelty mechanism.** Replace the deliverable with an **adopt-class anatomy/reference instrument**. Recommend **HOLD (mechanism) + PILOT-ANATOMY (instrument)** | The project's own frozen metadata cap (ledger 4828-4829) makes the cap-satisfiable lane width `w >= 10-15 B`, which excludes interleaved record lanes (critic §5.1) — §5 below |
| 2 | Built plan = implement `M_RAW`/`M_XOR`/`M_VALDELTA` + six measurement arms | Built = **NAC**, a codec-free anatomy census (stride detection + per-lane width/S/cap-feasibility). No codec, no entropy coder, no timing | Coordinator instruction; and because Q-PREVALENCE precedes any implementation |
| 3 | Byte opportunity argued from `synth-arith` / `synth-timeseries` grid rows | Those rows retained **only as existence/ceiling tests on one synthetic lineage** — never as generalization or prevalence evidence | Track 03 both lanes: all 8 `synth-*` are **one independence unit** |
| 4 | Novelty framed as "residual surface = rebase rule, no prior art found" | Novelty is **not available at this level**. Accept the critic's ruling; add ALP/TDT findings that close more, not less | Ledger PORDER kill + ALP/ALP_rd/TDT |
| 5 | Promotion conditional on "a held-out numeric family" | Promotion **hard-blocked** until ≥2 real numeric/telemetry families from independent publishers are locked with SHA-256 manifests | `docs/I10-CORPUS-LOCK-PROTOCOL.md` §13; `03-heldout-corpus-fledge.md` §5 (currently **0**) |

---

## 2. Reconciliation with `13-numeric-compiler-fledge.md`

| # | Critic finding | My ruling | My evidence / concession |
|---|---|---|---|
| R1 | Typed numeric components heavily occupied by prior art (Gorilla, FPC, ALP, Parquet, ORC, FastLanes, BtrBlocks, LeCo, OpenZL, white-box) | **Accept in full** | I independently found **TDT** (arXiv 2506.18062, 2025-06) — typed byte-lane entropy-clustering in front of a *general* backend, benchmarked *against Brotli*, with a §6.2 critique of Brotli's context map as "coarse" for float byte entropies. I additionally found **ALP_rd / PseudoDecimals** (reversible doubles→integers when decimal-origin), which closes the "mixed-class union" variant, and ALP's **1024-value vectors with sample-then-block selection**, which closes my proposed *encoder architecture*. My checkpoint's residual novelty claim was overstated; it is withdrawn. |
| R2 | G2 selected DELTA/DoD **0×**; INT-only was **+0.2701 % worse** than the all-raw carrier | **Accept in full** | `docs/I10-GROTLI-G2-RESULTS.md:180,190,196-212,253-258`. My checkpoint quoted the `P-INT` number but did not draw the conclusion the critic draws. **The only real, frozen, same-backend typed-leaf experiment in the project measured numeric coordinate transforms as third behind a plain token dictionary, and never once selected a delta-of-delta leaf.** |
| R3 | No numeric held-out family exists | **Accept in full, and strengthen it** | `RESEARCH_LEDGER.md:4831-4833`; Track 03 both lanes: all 8 `synth-*` = **one generator, one author, seeds 90001-90008** (`03-heldout-corpus-fledge.md:69`), and `synth-telemetry-f64.bin` / `synth-ndjson-columnar.ndjson` are the **same value model** (`20.0 + 0.001*i + randint(-2,2)*0.0001`, `tests/make_synth_corpus.py:240-267`). Protocol §13 requires **2 real numeric/telemetry streams not derived from ANVIL generators**; count today = **0**. |
| R4 | Two numeric fixtures have no grid rows | **Accept in full** | `Select-String 'synth-telemetry' / 'synth-ndjson'` over all five `tests/benchmark-suite*.csv` + `suite-frozen-bdc90474-reps.csv`: **0 hits each** `[measured]`. |
| R5 | Favorable Gorilla/DoD figures are generator-dependent or provenance-defective | **Accept, and adopt two prohibitions** | (a) The `"+14.06 % B2 bloat"` figure has an unexplained denominator: only the exact invariant is citable — **B2 costs exactly 11 more bits than B1 for identical payload** (vs B1: `11/(2+mb)` = +17.74 % at mb=60, +78.6 % at mb=12; vs a 64-bit container: `13+mb > 64` for mb > 51). (b) A DoD zero-rate is a property of *timestamp quantisation*, not of Gorilla: σ=0.3 s jitter yields 75.9 % zeros when timestamps are quantised to 1 s and **0.0 %** when not. I therefore quote **no** DoD percentage and **no** single float b/val figure anywhere in this report. |
| R6 | Novelty should be routed to an adopt-class reference row, not a second mechanism | **Accept — this became the revised deliverable** (§6) | My §6 instrument is the same instrument the critic prescribes; I adopt their arm structure (`A2` transform-enabled dual bar; `A4`/`A5` adopt-class references; candidate judged against `min(A2,A4,A5)`, never against raw Brotli) rather than my own weaker `A1`-anchored framing. |
| R7 | — | **One point of principled difference** | The critic folds "typed-lane compiler" into the PORDER kill. My residual was *narrower and different*: CANL has no relation graph, no width-aware schedule, no per-lane expression DAG — only **state defined as a read-back of reconstructed output** rather than carried predictor state, so LZ copies re-establish it for free. I still maintain that is not a PORDER relabel. **But I now concede it is worth zero bytes**, because the rebase's entire mechanism-specific benefit is bounded by the predictor-window bytes it avoids re-transmitting — arithmetically of order `0.004 B/value` — and §5 shows the placement fails the project's own metadata cap regardless. **The difference does not change the verdict and I am not asking the coordinator to carry it forward.** |

---

## 3. Corpus-protocol correction applied (mandatory)

I have re-read both Track 03 reports and adopt their accounting verbatim:

1. **Every `synth-*` numeric/telemetry file is one generator lineage** — `tests/make_synth_corpus.py`, one author, seeds 90001-90008, one release (`03-heldout-corpus-space-bunny.md` E18/E21; `03-heldout-corpus-fledge.md`:69). They are **mechanism-controls / existence tests / ceilings**, never generalization evidence.
2. **`synth-telemetry-f64.bin` and `synth-ndjson-columnar.ndjson` are one value model in two framings** (`03-heldout-corpus-fledge.md`:114-116). Counting them as two numeric families is exactly the "file count as proxy for evidence count" defect that protocol §5.1 rule 8 forbids.
3. **Consequence for this track:** all `{synthetic}` cells carry the dual-bar qualifier (`RESEARCH_LEDGER.md`:4509) and cannot produce a crossing. A track-13 result on `synth-*` may support *existence*, *ceiling*, and *regression* claims only.
4. **Promotion precondition, stated in my own gate language:** *any* PILOT/PROMOTE language for this track requires **≥2 real numeric/telemetry families from independent publishers**, locked under `docs/I10-CORPUS-LOCK-PROTOCOL.md` with SHA-256 content authority, order-sorted entries, license report, publisher-split report and a `corpus-access-ledger.jsonl` — before any byte of a candidate implementation is encoded. Today this count is **0**, so the precondition is **unmet and recorded as unmet**.

---

## 4. Evidence map (measured, provenance-separated, never spliced)

**Citation rule v3 (`RESEARCH_LEDGER.md` A18, line 5483) honoured.** Rows in §4.1-4.2 come from ONE window: `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv`, sha256 prefix `AF44D9D3D40FB85C…`, independently reproduced by `docs/gate-verify-i9.py`, per-rep process invocations, byte identity 351/351, wire gate 494/494. Byte counts are citable. **Store-class throughput is excluded by the same ruling** (open store-path anomaly). §4.3 rows are from other artifacts and are reported in a separate table; they are never summed with §4.1-4.2. Command: `Import-Csv tests\benchmark-suite.frozen-bdc90474.plus-xz.csv`, filtered per file/codec.

### 4.1 `synth-arith.bin` (256,000 B) — `{synthetic}` mechanism-control, one lineage

8 `u32` AP columns, 8,000 values each, `v += stride + randint(-1,1)`, block-wise (`tests/make_synth_corpus.py`:72-81).

| codec | bytes | ratio | enc MB/s | dec MB/s |
|---|---:|---:|---:|---:|
| `anvil-sparse-rans` | 256,022 | **1.000** | store-class (excluded) | store-class (excluded) |
| `anvil-hotop-rans` | 256,022 | **1.000** | store-class (excluded) | store-class (excluded) |
| `brotli-q4` | 170,573 | 0.666 | 62.424 | 172.600 |
| `brotli-q11` | 87,013 | 0.340 | 0.610 | 200.188 |
| `brotli-q9` | 114,417 | 0.447 | 11.974 | 194.810 |
| `zstd-19` | 106,614 | 0.416 | 8.489 | 309.179 |
| `xz-9e` | **79,372** | **0.310** | 3.510 | 58.149 |

**What this legitimately establishes `[derived]`:** (i) ANVIL emits **store-class** output on a file whose structure is a pure arithmetic progression; (ii) the project's own diagnosis is already on record — `RESEARCH_LEDGER.md`:2355, *"LZ matching cannot exploit arithmetic-progression structure at all, regardless of channel/topology settings, because no current ANVIL token type encodes a 'delta from an arithmetic relation' — only byte-identical and (in TCOPY/PNRA) translation-invariant copies."* That is a **token-type gap**, precisely localised, and it is the most concrete capability gap this track ever identified.

**What it does not establish:** any prevalence, any generalisation, any promotion. It is one generator's output model, and its own generator comment says the structure was *"deliberately made strong instead of incidental"* (`:69-71`).

### 4.2 `synth-timeseries.bin` (280,000 B) — `{synthetic}` mechanism-control, one lineage

20,000 × 14 B `{u64 ts (+1000±50), f32 value (random walk ±0.1), u16 id (cyclic 0..999)}`.

| codec | bytes | ratio | enc MB/s | dec MB/s |
|---|---:|---:|---:|---:|
| `anvil-dp-rans` | 156,444 | 0.559 | 5.058 | 174.509 |
| `anvil-shape-ctxmap-rans` | 144,652 | 0.517 | 1.335 | 124.158 |
| `anvil-shape-rans` | 144,219 | 0.515 | 1.306 | 125.162 |
| `anvil-sparse-rans` | 143,132 | 0.511 | 3.945 | 165.367 |
| `anvil-tcopy-rans` | 143,138 | 0.511 | 4.020 | 163.944 |
| `anvil-tcopy-pnra-rans` | 143,138 | 0.511 | 3.931 | 165.299 |
| `anvil-hotop-rans` | 143,234 | 0.512 | 3.916 | 201.729 |
| `anvil-hotop-rlzp-rans` | **140,898** | **0.503** | 0.047 | 197.141 |
| `brotli-q1` | 162,015 | 0.579 | 197.810 | 202.429 |
| `brotli-q4` | 148,979 | 0.532 | 66.805 | 256.951 |
| `brotli-q6` | **120,669** | 0.431 | 34.129 | 294.706 |
| `brotli-q9` | **120,669** | 0.431 | 10.138 | 289.017 |
| `brotli-q11` | 134,718 | 0.481 | 0.445 | 190.697 |
| `zstd-19` | 147,162 | 0.526 | 7.446 | 608.960 |
| `xz-9e` | **117,448** | **0.4195** | 2.465 | 44.899 |

`[derived]`: the best ANVIL row is **+19.97 % larger than `xz-9e`** and **+16.75 % larger than `brotli-q6/q9`**. On the encode plane every ANVIL row is dominated by `brotli-q9`; the only axis on which ANVIL is non-dominated here is decode throughput versus `xz-9e` (44.899 MB/s). Decode anchor for cycle arithmetic below: `anvil-hotop-rans` = 143,234 B compressed at 201.729 MB/s input ⇒ **103.1 MB/s compressed output** `[derived]`.

### 4.3 Separate-window measurements — cited separately, never merged

| measurement | value | artifact |
|---|---:|---|
| `synth-timeseries` transform-enabled bar `xz -9e --delta=dist=14` | 89,564 B | `docs/gate-verify-i9-transform-bar.py` |
| `synth-timeseries` `brotli q11` **on delta14** | 80,650 B | same |
| `synth-timeseries` raw best reference | 120,669 B | same |
| `synth-columnar-align` raw best ref / `xz -9e --delta=dist=23` | 105,915 / **21,340 B** | same |
| datastruct-ts ladder | `ts.ref_field` 89,877; `ca.ref_field_true` 24,055; coder-only 19,761 | `prototypes/i9-datastruct/verify_artifacts.py` |
| record periods (verified) | **P=14** (20,000 rec), **P=23** (12,000 rec); supersedes P=28/P=184 | `docs/gate-verify-i9-corpus.py`; ledger 4632-4642 |
| G2 typed leaves | D2 `P-INT` 25,986 vs `G1R` 25,916 (**+0.2701 %**); DELTA/DoD selected **0×**; `P-DICT` 20,903 | `docs/I10-GROTLI-G2-RESULTS.md`:180-212 |
| G2 carrier tax | `G1R` 25,916 vs historical `G1` 25,654 (**+1.02 %**) | same:180 |
| sao share of the 3rd-backend gap | **160,422 B = 24.2 %** (mozilla 64.7 %, ooffice 7.8 %, samba 3.4 %) | `docs/audit-2026-09-07/10-specialist-disagreement.md`:73-77 |
| project's own verdict on this family | float/unsortable numeric = **"MEDIUM, small … a stride/delta probe rather than a new backend"** | same:135-140 |
| decode-RSS instrument status | G5D still emits a zero placeholder ⇒ **any RSS gate is currently measured with a known-bad instrument** | `RESEARCH_LEDGER.md`:4837-4840 |
| published expectation band for "typed representation + general backend" | **1.04x-1.17x** ratio on real float data; **1.08x-1.67x** on real integer data | `[lit]` TDT arXiv 2506.18062 §5.2, Table 5 |

**The transferable rule these encode:** `ts.ref_field` (89,877 B) lost to `xz --delta=dist=14` (89,564 B) and to `brotli q11` **on the same transform** (80,650 B) — i.e. **+1.1 % and +11.5 % worse than references it was never compared against during construction.** Any track-13 claim expressed against *raw* Brotli is void.

---

## 5. The decisive result: a closed form that kills the promotion path

This is the section that changed my recommendation, and it needs no benchmark.

The project's frozen surviving condition from the PORDER ruling is: *"metadata/codebook cost at most **20 % of gross savings**"* (`RESEARCH_LEDGER.md`:4828-4829). Let a lane have input width `w` bytes, decoder-visible per-lane descriptor `m` bytes, and achieved saving fraction `S`. Then

```
(m / w) / S  <=  0.20      <=>      S  >=  5 * m / w
```

with `m ≈ 2-3 B` (end-offset uvar + width code + operator selector + exception-model field). Evaluated:

| `w` | required `S`, `m=2` | required `S`, `m=3` | satisfiable at all? |
|---|---:|---:|---|
| 4 B (u32 field) | 250 % | 375 % | **NO** |
| 8 B (f64 / u64 field) | 125 % | 187.5 % | **NO** |
| 16 B | 62.5 % | 93.75 % | yes, m=2 only in practice |
| 32 B | 31.25 % | 46.875 % | yes |
| 1024 B (contiguous column) | 0.98 % | 1.46 % | trivial |

**First satisfiable width: 10 B at `m=2`, 15 B at `m=3` `[derived]`.** Every interleaved record lane a *general-purpose* codec can discover has `w ≤ 16` and, for the ordinary `w ∈ {4,8}` field widths, is **structurally impossible** — not hard, impossible. The only geometry that satisfies the cap is a **long contiguous column run**, which is exactly where Parquet/ORC/HDF5/netCDF already carry a schema and where ALP, FastLanes, BtrBlocks and LeCo already operate, and which a general-purpose codec cannot even name because it has no schema.

Independent confirmation from the project's own side: the G2 carrier tax was **+1.02 %** just for per-leaf IDs and payload lengths, before any leaf was chosen, and G2 explicitly charged leaf payload lengths **even though the grammar could parse without them** (`docs/I10-GROTLI-G2-TYPED-PREREG.md`:258-259). A lane map has the same property and must be charged the same way.

**Consequence, stated flatly:** the CANL mechanism cannot reach a promotable state at any interleaved placement, and at contiguous-column placement it is a format adapter, not a general-purpose compressor. This is arithmetic, independent of every prior-art judgement, and it is why my recommendation is now HOLD rather than PILOT.

---

## 6. Revised deliverable: NAC — Numeric Anatomy Census (+ the missing dual-bar reference row)

The mechanism survives only in its **instrument** form, which is what the critic identified as the real residual value: without an adopted, dual-bar'd numeric reference, every future numeric claim in ANVIL will be compared against raw Brotli and will manufacture FRONT-GAP illusions of exactly the `ts.ref_field` kind.

### 6.1 NAC — precise definition

A **census** is a codec-free pass producing, per file: the minimal fixed record stride `L` (null-calibrated), the per-lane anatomy at every offset, and a **cap-feasibility verdict per lane**. Three stages:

1. **Data-derived period candidates.** Bounded head-prefix period vote over `p ∈ [8,64]`, then a **primitivity prune**: an exact multiple of a surviving candidate is dropped (it carries the same field structure with fewer samples per residue class, which inflates its apparent entropy deficit). *Provenance of this rule:* an earlier revision of my own tool used a hand-listed stride set `(8,12,14,16,20,23,24,28,32,40,44,48,64)` and consequently could not see the 18-byte stride of its own reference pattern, reporting `48`. That defect is recorded here because it is exactly the alias/minimality failure the ledger already warns about for periods (P=28 vs P=14).
2. **Structure score, null-calibrated.** For candidate `L`, split the byte stream into `L` residue classes and score `structure_gain(L) = H_global − mean_L H_residue`. A typed-numeric lane *is* a fixed-stride record field, so real record structure raises this. It is deliberately a **structure** test, weaker than any predictor a lane would use, so it cannot flatter the mechanism. `null(L)` is the same score at `L+1`; a stride is admitted only if `margin = gain − null ≥ 0.05` and `gain ≥ 0.10` bits. Minimality: among strides within 0.02 bits of the best, take the **smallest**.
3. **Lane anatomy + cap verdict.** Per lane: `w`, `n`, integer first-difference mean bits and zero fraction, float XOR-of-bit-patterns mean bits and Case-A fraction, admitted mode, `S`, and `cap_feasible[m] = (S ≥ 5m/w)` for `m ∈ {2,3}`. Two geometries are reported separately and never merged: **interleaved** (`w ∈ {2,4,8}`, offsets inside the record) and **contiguous-column** (whole record span as one lane, `w ∈ {16,…,64}`).

### 6.2 What NAC explicitly does *not* do

- It does not compress, decompress, invoke a codec, entropy-code, or time anything, and it emits **no ratio or throughput number**.
- It does not claim any lane is worth building. It reports whether the cap *could* be met, which is a necessary condition, never a sufficient one.
- It does not retune any detector constant. `docs/prereg-s62g-timeseries-gates.md` §4 adopts grotli's constants verbatim and forbids re-tuning inside this scope; NAC's constants are different quantities and are frozen for the same reason.
- It does not substitute synthetic files for real ones. If only `synth-*` files are available, its output is `{synthetic}` and non-load-bearing by construction.

### 6.3 The reference-row instrument (adopt-class, harness-only)

Arms, adopting the critic's structure because it is the correct one:

| arm | content | role |
|---|---|---|
| `R0` | raw bytes | floor |
| `R1` | Brotli q11/q9 on raw | the **weak** bar; reporting only |
| `R2` | `xz -9e --delta=dist=P`, both `P ≤ 256` and `P > 256` reported | **transform-enabled dual bar** (`xz --delta` covers 1..256 and both project record periods sit inside it — ledger 4436-4443) |
| `R3` | ANVIL current best config, same build | own-pipeline control |
| `R4` | Gorilla-class A/B1/B2 + DoD, native, two-lane `u32` state, normalised `u64` accumulator | adopt-class reference; cheapest faithful bar |
| `R5` | ALP-class: decimal-factor ladder + exponent delta + mantissa residual + exception mask | adopt-class reference; the **strong** ratio bar |
| `R6` | TDT-class: entropy-cluster the byte lanes, reorder to equal-entropy streams, same backend | adopt-class reference; the "typed lanes in front of the same backend" bar |
| `R7` | any future CANL-class candidate | judged against `min(R2,R4,R5,R6)`, **never `R1`** |

Every arm reports complete bytes with every header/table/framing charged, encode MB/s, decode MB/s, decode peak RSS, and binary-size delta; median ≥3 paired reps, same run, same build, per-rep process invocations, per-run output hashes recorded.

---

## 7. Novelty and prior-art: no novelty is available at this level

Component occupancy is total. I now add the two closures my checkpoint missed and one I withdraw:

| element | occupant | status |
|---|---|---|
| XOR of consecutive doubles + leading/trailing-zero window reuse | Gorilla (VLDB 2015) | closed |
| exponent/mantissa split | FPC; Chimp/Chimp128 (VLDB'22); Patas; Elf (VLDB'23) | closed |
| decimal-factor ladder + exponent delta + patched outliers | ALP (SIGMOD'24), adopted by Parquet | closed |
| FOR / delta / delta-of-delta / bit-packing | Parquet, ORC, PForDelta, FastLanes, `xz --delta` | closed |
| per-column/lane codec dispatch against a real backend | ALP; FastLanes; project G2 §7/§20 | closed |
| **typed byte-lane clustering in front of a general backend** | **TDT (arXiv 2506.18062, 2025-06)** — benchmarked against Brotli, 1.04x-1.17x ratio on real float data | closed |
| **sample-then-block predictor selection** | **ALP §1/§2.6** — "first samples row-groups and then vectors", 1024-value vectors | closed (this closes the *encoder architecture* my checkpoint proposed as `[design]`) |
| **"doubles that are really decimals" routing** | **PDE / PseudoDecimals** as strengthened in **ALP_rd** | closed (kills the mixed-class variant) |
| row-copy + per-column residual contexts | `datastruct-ts` — **NOVELTY: NO** (ledger 4609) | closed |
| relation graph over typed lanes × width-aware schedule | **PORDER — KILLED** (ledger 4824-4829); only a frozen 2×2 probe survives, requiring ≥1 % beat over **both** single-factor arms and metadata ≤20 % of gross savings | closed |
| transmitted per-lane transform parameter | G3 (FAIL, NOT DEFENSIBLE); ARI-REF transmitted-Δ (prior art); TCOPY transmitted-Δ (REJECTED) | closed, twice failed in-project |
| global lane transposition | measured decisive negative; KILLED | closed |
| context-mapped literal models | Brotli RFC 7932 §7 | closed, conceded twice in-project |
| **copy-anchored state rebase (state-as-read-back-of-output) inside an LZ token stream** | no located prior art in two targeted searches | **residual — and withdrawn**: worth ≈ 0.004 B/value by construction, and blocked by §5 regardless |

**Ruling:** no mechanism-level novelty claim is available for track 13. Any future report offering one is relabelling ALP, TDT or PORDER.

---

## 8. Minimum prototype — BUILT, self-test green

`prototypes/swarm-2026-10-02/13-numeric-compiler/space-bunny/nac_census.py`. Not referenced by any CMake target, workflow, or production file. `--self-test` runs pure in-memory arithmetic and touches no file. Command run this session:
`python prototypes\swarm-2026-10-02\13-numeric-compiler\space-bunny\nac_census.py --self-test`

```
PASS NAC self-test (pure arithmetic, in-memory, no codec, no file)
  cap condition: (m/w)/S <= 0.20  <=>  S >= 5m/w
    w=    4 B  S required: m=2 250.0000%  m=3 375.0000%  satisfiable: NO
    w=    8 B  S required: m=2 125.0000%  m=3 187.5000%  satisfiable: NO
    w=   16 B  S required: m=2  62.5000%  m=3  93.7500%  satisfiable: yes
    w=   32 B  S required: m=2  31.2500%  m=3  46.8750%  satisfiable: yes
    w=   64 B  S required: m=2  15.6250%  m=3  23.4375%  satisfiable: yes
    w= 1024 B  S required: m=2   0.9766%  m=3   1.4648%  satisfiable: yes
  measured float XOR regimes (mean XOR bits/value):
    decimal-origin drift 20.0+0.001*i :  40.02  case_a=0.994  (XOR BAD)
    exponent ramp 20.0*2^(i//8)      :   6.74  case_a=0.998  (XOR GOOD)
  the census reports a regime, never a single float figure
```

All of the above `[in-session]`. What the self-test actually establishes:

1. **The cap closed form reproduces exactly**, including that `w = 4` and `w = 8` are unsatisfiable and `w = 15`/`16` is the boundary.
2. **Data-derived period discovery recovers the true minimal stride** (18) from a pattern whose stride is in neither a plausible hand list nor the previous revision's list, and **refuses genuinely random bytes** (SHA-512 counter mode; margin below floor).
3. **A pure-AP `u32` lane at `w=4` has a real `S = 0.875` and is still cap-BLOCKED** — the decisive demonstration that `S` is not the binding variable; `w` is.
4. **The same data as a contiguous `w=32` column IS cap-feasible.** The mechanism's viability is a function of lane geometry, not lane quality.
5. **Float XOR is regime-dependent and the census measures which regime it is in**: decimal-origin drift (the ALP use case) gives **40.02 bits/value mean XOR residual** — XOR is *bad* there, which is exactly why ALP needs a decimal-factor transform; an exponent ramp gives **6.74 bits** with Case-A 0.998 — XOR is good there. This is the mechanism-level justification for never quoting a single float b/val figure.

Defects found and fixed during the build, recorded because each is a reusable trap: (i) float XOR was computed on Python `float` XOR instead of on bit patterns (`TypeError`) — must XOR the *bit pattern*; (ii) integer residual unpack used a fixed width, breaking on `w=2` lanes; (iii) hand-listed stride candidates hid an 18-byte stride; (iv) a repeated SHA-512 digest was used as an "incompressible" control, which is *perfectly periodic* at 64 B and made the null test pass spuriously.

---

## 9. The one decisive remote-only experiment (revised: anatomy first)

**E13-NAC.** GitHub Actions only, pinned Ubuntu runner, 5 reps, PR-4 window, per-rep process invocations, per-run output SHA-256. No local measurement.

**Stage 0 — corpus gate (blocking, and currently failing).** ≥2 real numeric/telemetry files from **independent publishers**, locked under `docs/I10-CORPUS-LOCK-PROTOCOL.md` with SHA-256 content authority, license and publisher-split reports. At least one must be **not** decimal-like, so ALP's advantage cannot decide the run. `synth-*` files are **diagnostics only** and are tagged `{synthetic}`. *If Stage 0 cannot be satisfied, E13-NAC is NOT RUNNABLE and this track converts from HOLD to KILL — pre-committed so it cannot later be rationalised into "run it on synth".*

**Stage 1 — anatomy census (cheap, first, no codec).** Run `nac_census.py` over Stage-0 files plus `sao` (the only real float file already in the landscape grid; **calibration, not held-out**). Output: prevalence, lane-width histogram, `S` distribution, cap-feasible prevalence per `m`. Runs before any byte is compressed, because it can kill the direction for the price of a parse.

**Stage 2 — reference grid row (adopt-class).** Arms `R0`-`R6` from §6.3 on the same files. This discharges the dual bar's "the reference codec is given the same representational opportunity" requirement and closes the FRONT-GAP-illusion hole. It is valuable **even if every track-13 mechanism is dead**, and that is the argument for running it.

**Stage 3 — candidate (only if Stage 1 shows `cap_feasible_prevalence > 0` on ≥2/3 files).** A CANL-class candidate, judged against `min(R2,R4,R5,R6)`.

### 9.1 Pre-registered thresholds (frozen before any run; movement after results is VOID)

**Correctness — any failure is an immediate run failure, not an interpretation:**
- C1 byte-exact roundtrip on every arm × every file.
- C2 bit-pattern equality, not float equality: NaN payloads, ±0, sign flips, denormals, max-`u64`. `float ==` will accept a wrong decoder.
- C3 malformed/truncated/trailing/oversized-declared-count rejection suite green; reject if declared lane values exceed `2^20`, or any declared offset exceeds the source bound, or reconstructed length ≠ declared length.
- C4 incompressible-input tripwire: reduction **exactly 0**, else the run FAILS. (Measured: Gorilla-class inflates incompressible input, so an ε gate is mandatory, not optional.)
- C5 no output outside the run's own directory.

**Prevalence gate (cheap, runs first):**
- P1 `cap_feasible_prevalence ≥ 0.10` on ≥2/3 Stage-0 files. If typed-numeric lane bytes cannot even satisfy the project's own 20 % metadata cap, no lane mechanism can be justified and the track is **KILL**.
- P2 `mean residual width` and `Case-A fraction` reported per lane **together**; a lane is not admitted on one number alone.

**Promotion gate (a candidate must satisfy ALL):**
- N1 `bytes(candidate) ≤ 0.97 × min(R2,R4,R5,R6)` on ≥2/3 Stage-0 files. *Origin of 3 %:* the frozen PORDER probe retained a 1 %-over-single-factor bar (ledger 4828); 3 % raises that to clear the observed carrier and routing taxes (G2 carrier +1.02 % on D2).
- N2 decoder-visible metadata + codebook ≤ **20 % of gross savings** (the frozen condition, applied literally). **Note from §5: this alone implies `w ≥ 10-15 B`, so N2 is a geometry constraint before it is a byte constraint.**
- N3 decode ≥ **1.0 GB/s** and ≥ 0.90 × the fastest reference arm's decode.
- N4 decode peak RSS ≤ **1.25 ×** max(fastest reference, control) — **and only after the project's RSS instrument is closed** (G5D currently emits a zero placeholder, so today this gate is unmeasurable).
- N5 encode ≤ **20 ×** `R1` encode (a ratio gate; absolute MB/s is not portable across runners).
- N6 binary-size delta ≤ 40 KiB.
- N7 held-out: N1-N6 applied **once** to a single locked real file; any failure ⇒ KILL.
- N8 **the claim must be carried by ≥1 non-synthetic family.** A synthetic-only pass is FRONT-GAP (dual-bar) and is recorded as such.

**Adopt-class ceiling (fixed in advance from external literature, not from this project):** published typed-lane work delivers **1.04x-1.17x** on real float data and **1.08x-1.67x** on real integer data `[lit]`. A result inside those bands is **adopt-class by construction** even if every gate passes. A result *above* them is above the published state of the art and must be treated as a **falsification target** — check generator idealisation, lineage leakage, and incomplete charge before believing it.

---

## 10. Adversarial failure cases

1. **Fixture substitution.** Synthetic telemetry used to satisfy the corpus gate. ⇒ Void; converts HOLD to KILL.
2. **Regime cherry-pick.** Quoting the exponent-ramp XOR row (6.74 bits) without the decimal-drift row (40.02 bits), or Gorilla's cheap regime without its expensive one. NAC emits both and they must be reported together.
3. **Provenance-defective percentages.** Any B2-bloat percentage, or any DoD zero-rate without (distribution, quantisation, seed). ⇒ use the exact invariant instead: B2 costs exactly 11 more bits than B1 for identical payload.
4. **Carrier omission.** Excluding lane descriptors, type tags, or the routing map because a grammar could infer them. ⇒ Void; G2 charged lengths even when parseable.
5. **Raw-bar comparison.** Reporting a win against `R1`. ⇒ Void by §4.3's rule.
6. **Width-blind analysis.** Judging a lane on `S` while ignoring `w`. ⇒ this is the exact defect that would have produced a false GO; §5 and self-test items 3-4 exist to catch it.
7. **Stride aliasing.** Accepting a multiple of the true period (e.g. 36 or 48 for an 18-byte record) and overstating lane structure. NAC's primitivity prune plus minimality rule exist for this; the earlier P=28-vs-P=14 alias correction is the precedent.
8. **Non-periodic "incompressible" control.** A repeated digest is periodic and passes null tests spuriously — a defect found in my own build this session.
9. **Meter-instrument blindness.** Any RSS conclusion drawn with the unclosed G5D instrument.
10. **Measured-cost attribution confound.** Attributing a gain to lane typing when it came from the search (G2's operator/planner confound).
11. **Held-out opened early or twice**, converting discovery into validation.
12. **Corpus-lineage inflation.** Counting `synth-telemetry-f64` and `synth-ndjson-columnar` as two numeric families — they are one value model in two framings.

---

## 11. Byte / cycle / memory cost model for the revised deliverable

**NAC itself.** Bytes: 0 on the wire — it emits no coded output. Cycles `[in-session]`: dominated by the bounded period vote, `O(period_prefix × 57)` byte comparisons = 1.87 M comparisons for a 32 KiB prefix, plus ≤6 candidates × 2 × `O(min(N, 2^20))` histogram passes. Memory: `O(256 × L)` counters (≤16 KiB at `L=64`) plus the file buffer. Decoder state: n/a. RSS: bounded and independent of file size apart from the buffer.

**A hypothetical CANL-class candidate, for costing completeness.** Bytes: `(5 B/lane)·R·S_regions` descriptor + Σ per-value `[1-2 control + payload]` bits + non-numeric literal entropy. **Governed by `S ≥ 5m/w`, which fails for `w ∈ {4,8}` regardless of `S`** (§5). Cycles: decode is *not* the risk — the adversarial review's measured 2.75-6.86 GB/s decode for numeric lanes is decode-competitive with the project's best Brotli-class decode, and I do not contest that. Encode/planner is the risk, with an adverse measured precedent (G2: 510 ms-2.68 s local scoring, 6.8-121 s marginal search, and the marginal planner **lost** to dense local composition on both real families). Memory: decoder state is small (two `u32` lanes + a normalised bit accumulator is the whole Gorilla state); **but** any lane map needs a region table, and per §5 its charged size is what breaks the cap. I also inherit the measured implementation traps rather than rediscovering them: a normalised `u64` accumulator is mandatory (bit-at-a-time measured 3-8x slower), `bits == 64` must be special-cased (clang masks shift-by-64 and this silently corrupts after the first value), 8-byte chunking is mandatory for vectorisation (byte-wise XOR measured 5x slower), and `13 + mb` overflows a 64-bit container for `mb > 51`.

---

## 12. Final recommendation

> ## **HOLD** the typed-numeric-lane mechanism. **PILOT-ANATOMY** the instrument.
> **Explicitly NOT PROMOTE-TO-REMOTE, and NOT KILL.**

**HOLD the mechanism** because three independent legs each suffice, and none of them is a matter of taste:
1. **Novelty is unavailable** — every component is published, and the composition that matters is PORDER (killed) / ALP / TDT / the columnar-LZ default.
2. **The project's own frozen metadata cap makes the placement structurally impossible** for the `w ∈ {4,8}` lanes a general-purpose codec can discover, and reduces the viable geometry to schema-carrying contiguous columns — i.e. a format adapter, not a general-purpose compressor (§5).
3. **The measured in-project evidence already ranks numeric coordinate transforms third**, behind a plain token dictionary, on the only real typed-leaf data available, and never selected a DELTA/DoD leaf once.

**PILOT-ANATOMY the instrument** because it is cheap, is not blocked by the novelty ruling, discharges a real hole in the measurement apparatus (the missing transform-enabled dual-bar row), and can kill the direction for the price of a parse:
- **E13-NAC Stage 1** (census, codec-free) answers Q-PREVALENCE and Q-BAR's precondition.
- **E13-NAC Stage 2** (reference grid row `R0`-`R6`, adopt-class, harness-only, never an ANVIL mode) converts the numeric axis from an unmeasured hope into a measured row and stops future numeric claims from producing FRONT-GAP illusions.
- Both stages are GitHub-Actions-only and need **no ANVIL source change**, which matters because this repo is in a named publication-blocker state (ledger 4953) and local builds are additionally blocked by a missing local RC toolchain (ledger 4866-4874).

**KILL is not yet right** because: the token-type gap is real and precisely localised (§4.1, ledger 2355); `sao` is 24.2 % of the identified 3rd-backend gap and no ANVIL mechanism touches binary float records; and killing outright would leave a known hole in the grid. **PROMOTE is not right** because there is nothing to promote and the corpus precondition is unmet (§3).

**Pre-committed conversions, frozen now:**
- No real numeric family locked ⇒ **HOLD → KILL** for the mechanism; the instrument may still proceed.
- Stage-1 `cap_feasible_prevalence < 0.10` on ≥2/3 real files ⇒ **KILL** the mechanism without implementing anything.
- Any candidate result inside the published 1.04x-1.17x band ⇒ recorded **adopt-class engineering**, not a novelty claim, regardless of gate outcomes.

**The single thing the coordinator should decide next** is not a codec question: it is whether to authorise acquisition of **≥2 real numeric/telemetry families from independent publishers**. That one decision unblocks Stage 0 for this track and is simultaneously the G1 admissibility blocker named by both Track 03 reports. Everything else in this report is downstream of it.

---

## 13. Provenance and compliance

- No existing file was modified. Two files created: this report and the isolated prototype.
- No commit, push, reset, clean, stash, restore, or rebase.
- **No local compression benchmark, sweep, or fuzz campaign.** Every throughput and ratio number in §4.1-4.3 is quoted from a named existing artifact with its window named; incompatible windows are kept in separate tables and never combined. All §4.1-4.2 rows are from the single frozen grid `AF44D9D3D40FB85C…`; §4.3 rows are from other artifacts and are separately labelled.
- The only local execution was `nac_census.py --self-test`: pure arithmetic on in-memory patterns, no file read, no codec, no timing. Corpus census execution is remote-only and was not performed.
- External figures are labelled `[lit]`, quoted with their source, and never re-measured or spliced into project measurements. Search provenance for the ALP/TDT read is recorded in §7's table and was two targeted searches plus one full-text fetch.
- No prototype is wired into production: no `src/` change, no `FORMAT.md` change, no CMake target, no workflow reference.
