# SYNTH-MEASUREMENT — measurement-validity gate for the remote experiment queue

**Author:** Space Bunny Free (Track 04, Dense Pareto Measurement)
**Date:** 2026-10-02
**Purpose:** map every proposed remote experiment to its measurement class; mark each current instrument **VALID / INVALID / NEEDS-CORRECTION**; name the measurement invariants a job must carry before it may enter the queue; and audit `docs/swarm-2026-10-02/REMOTE-EXPERIMENT-QUEUE.md` for invalid estimands or missing invariants.
**Measurement class of this document itself:** retained-artifact arithmetic + static source reading + static linter. **No codec invocation, no timing, no corpus benchmark, no build, no network.**
**Source of truth for Q0:** `docs/swarm-2026-10-02/Q0-DENSE-RETAINED-RESULT.md` (coordinator-frozen). I re-executed the arithmetic independently and **reproduce it exactly** (§1.2). No arithmetic error found; nothing to correct.

> **Gate statement.** No job in the queue is cleared to dispatch by this document. Three of the four Tier-0/Tier-1 jobs that carry a *timing* estimand are currently **INVALID** as specified. Q2 is **REQUIRED** and must be re-specified as a **byte-primary parity experiment** before it can be queued.

---

## 1. Provenance and reproducibility

### 1.1 Input identities (SHA-256, computed this session)

| Artifact | SHA-256 |
|---|---|
| `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` | `AF44D9D3D40FB85C7E35758FD7936ADB562C8DB6FF455B30371AB355EBE7B65E` |
| `prototypes/swarm-2026-10-02/04-dense-frontier/space-bunny/parity_sweep.py` | `DEF341C3DF1DDAC62CFDE443B13D5B1ED0E0E9E76899D2321C92C9293467E9E2` |
| `prototypes/swarm-2026-10-02/04-dense-frontier/space-bunny/envelope_arbiter.py` | `16BEE257397EF65CEB44982B6A382FD8E7DA33ABEA87F2AE960E7EDBA45C021C` |
| `src/anvil.cpp` (working tree) | `770EA0F5F2FF32F84A775C8C1548DCF52B55F9D2A7C7A3BFD0EBAAD4C59F5D90` |
| `tools/bench_native.cpp` | `EE4B2B8D09A22114657BA9A9FCB0241986C5324653B592E2C1C583B3D0743395` |

Git: branch `i10-aux-unbwt` at `b8eae11`; `git rev-parse HEAD:src/anvil.cpp` = `755df76ae53795aea040f4c99784d1effb43b3ab` (matches the dense vehicle's pinned candidate blob).

**Script-hash provenance note:** the coordinator's frozen Q0 records `parity_sweep.py` at `2117B5C2…`. That was the **pre-correction** version. My copy had a real correctness blocker — `classify_cell` applied `DOMINATED → FRONT-GAP → CROSSING` and **omitted the DEGENERATE predicate**, which promotes every incompressible row (`ratio ≥ 0.95`) to CROSSING. On this grid that misreports **28 cells**. Fixed to the adopted order `DOMINATED → DEGENERATE → FRONT-GAP → FRONT-CROSSING` (`RESEARCH_LEDGER.md:4508`); current hash above. **The coordinator's frozen counts are the corrected ones** and are reproduced exactly by the fixed script. This is recorded because the omission is precisely the defect class the queue's item 4 exists to prevent.

### 1.2 Q0 reproduced (source of truth confirmed)

```
TOTAL (18 configs)   DOM=435  DEGEN=28  GAP=5  CROSS=0      total cells = 468
configs producing >=1 CROSSING: 0 of 18
FRONT-GAP cells: anvil-mdl-rans, anvil-mdl-rans-l0, anvil-mdl-rans-l001,
                 anvil-shape-rans, anvil-shape-rans-l0
```

Identical to the coordinator's frozen table (`Q0-DENSE-RETAINED-RESULT.md:25-42`), including the per-configuration FRONT-GAP attribution and the 18 × 13 × 2 = 468 cell closure. **No arithmetic error. Coordinator result stands as source of truth.**

---

## 2. Measurement-class taxonomy used throughout

Adopted from `docs/github-actions-benchmark-protocol.md:29-47` and extended with the two classes this queue actually needs.

| Class | Definition | May it emit a frontier token? |
|---|---|---|
| **C-BYTE** | Compressed bytes, ratio, round-trip hash, parser/token counts, model size, wire properties. Deterministic for a pinned build+inputs. | **Yes**, for the byte axis only, and only with complete byte accounting and matched geometry |
| **C-SCOUT** | Throughput, ns/byte, wall-clock, peak RSS on a shared hosted runner. Ranking-grade within one job. | **No** |
| **C-STATIC** | Source/arithmetic/theorem analysis over retained artifacts. | **No** (it can *invalidate*, not promote) |
| **C-SYNTH** | Same as C-BYTE but with one component swapped for a reference implementation to isolate attribution. | **Yes**, for the isolated component |

Two additions this queue forces:

- **C-PARITY** — a byte claim is only C-BYTE if the *geometry* (block/window/stream discipline) of the compared arms is matched. An unmatched byte claim is **C-BYTE-CONFOUNDED**: the number is real, the comparison is not. This is the class the whole Q2 dispute is about, and the current queue does not name it.
- **C-TIMING-SHORT** — a timing cell whose signal is small relative to fixed per-invocation overhead. Its estimand is *not* throughput; it is unresolvable. Reporting such a cell as a rate is an invalid estimand, not a noisy one.

---

## 3. Instrument status register

Each row: instrument → status → the invariant it violates → what must change.

| # | Instrument / practice | Status | Violated invariant | Required correction |
|---|---|---|---|---|
| I1 | **Class-A byte grid at default 256 KiB blocks** (`bench_native.cpp:25` never assigns `o.block_size`; `anvil.cpp:3769` default `256*1024`) vs reference windows (Brotli `BROTLI_DEFAULT_WINDOW`=lgwin22=4 MiB, `third_party/brotli/c/include/brotli/encode.h:88`) | **INVALID as a comparative instrument; VALID as a record** | C-PARITY. Block size *is* the memory horizon: every parse entry point takes only the block slice (`anvil.cpp:4671-4673`, `parse_sparse(block,…)`, `encode_tokens(block,…)`), so matches cannot cross a block boundary | Every Class-A ANVIL byte row on files > 256 KiB is geometry-confounded. Admissible only for files ≤ 256 KiB, or after a parity re-measure. **This is the Q2 blocker.** |
| I2 | **Dense-vehicle ANVIL arm (`--parse=ratio`, no `--block`)** | **VALID — at parity** | — | `anvil.cpp:4999` raises the block to 128 MiB when `parse=="ratio"` and `--block` was not explicit. Silesia's largest file (mozilla, 51,220,480 B) and enwik8 (100,000,000 B) are both < 134,217,728, so **every dense-vehicle cell is one whole-file block**, against Brotli `lgwin 30` (`workflow:291`). Class B is *not* asymmetric. Record this so I2 is not re-audited as broken. |
| I3 | **Paired timing driver** (`tools/paired_bench.py`, statistical core byte-identical in pinned `f847c50e…` and hardened `bb2b2a7e…`) | **NEEDS-CORRECTION** | The gate screens the wrong statistic | Per-arm `robust_cv` conflates within-pair differential noise (contaminates the ratio) with across-pair common-mode drift (does not). **Measured harm:** enwik8 was ruled `TIMING_BLOCKED` at Brotli control CV 0.1756, discarding a measured **10.1499×** decode ratio (`RESEARCH_LEDGER.md:4801-4802`, `:4941-4942`). Compounded by **MAD degeneracy at n=7**: `[1,1,1,1,1.4,1.9,3.0]` → MAD 0 → CV 0.0000 → gate passes **with a 3× outlier present**. Screen on `MAD(log_ratios)` with a floor (≥ 0.02·median\|log ratio\|) so MAD=0 cannot pass. |
| I4 | **Ranking scale of the frozen 20-arm vehicle** | **INVALID** | Estimand substitution | `plane_mbps = source_bytes / candidate_median_s` (`workflow:851`) is the arm's **own absolute** median from its **own driver process**, and `dominates()` compares those absolutes across arms (`:885-893`). The paired CI is computed, written to `paired.csv`, and **never used for ranking**. Permitted gate (CV ≤ 15% per arm) is *wider* than the encode-rate gaps between adjacent arms. Rank on `log_rel_rate(arm) = −ln(paired_ratio(arm, anvil-legacy))` from the same invocation. **Absolute MB/s across driver invocations is forbidden.** |
| I5 | **Process-level timing estimator** (`perf_counter` around `subprocess.run`; every timed invocation through a generated `observe.sh` bash wrapper, `workflow:467-508`) | **INVALID for decode; NEEDS-CORRECTION for encode** | C-TIMING-SHORT + common-mode contamination | `T_measured = T_wrapper + T_fork + T_dynlink + T_codec + T_write`. Only `T_codec` is a codec property. The A/A null is **structurally blind** to this: both A/A arms pay the identical wrapper, so the null passes cleanly while the estimand is wrong. **A/A validates noise, not the estimand.** Fix exists in-tree: `src/anvil.cpp:5023-5026` times the codec operation internally with `steady_clock` and writes output outside the timed region. Adopt in-process timing, or declare a `DEGENERATE_TIMING_SHORT` floor. |
| I6 | **RSS axis** | **INVALID in cross-family dominance** | Unmatched buffering discipline | `dominates()` carries both RSS axes (`:891-892`) and `descriptive_class` keys off a variant that still carries them (`:913`). ANVIL decode peak RSS is **Θ(input+output) by construction** — `decompress` returns a `vector<uint8_t>` and pre-reserves `min(total, max(1 MiB, in.size()*256))` (`anvil.cpp:4859-4860`) — while zstd/xz stream. Measured: Silesia ANVIL 248.4 MiB vs Brotli q11 114.4 MiB; enwik8 598.9 vs 209.0 MiB (`RESEARCH_LEDGER.md:4930-4934`). Charge `resident_charge_kib = peak − (input+output)/1024`; raw RSS descriptive only. **The streaming-API gap is a product finding for tracks 18/19, not a bench cell.** |
| I7 | **Binary-size axis** (`dominated_full_cost`) | **INVALID** | Scope mismatch | Compares `decoder-linker-gc-harness` (anvil, brotli) against `system-cli` (zstd, xz) stripped bytes (`:873-883`, `:895`). Prereg §9 explicitly forbids exactly this, and `summary.json` then asserts `binary_size_comparability: "descriptive-only"` (`:945`) — a **prereg↔code contradiction**. There is **no zstd or lzma decoder-only measurement at all** in the vehicle. Delete the column, or restrict to the one matched-scope pair (anvil↔brotli) renamed `decoder_size_paired`. |
| I8 | **Arbiter** | **INVALID / MISSING** | Adopted predicate not encoded | Emits 3 classes (`DEGENERATE_RATIO`, `DESCRIPTIVE_NON_DOMINATED`, `DOMINATED`, `:913`) where doctrine has 4 in fixed order. `tools/pareto_verify.py` — which `tests/pr-4-measurement-window-protocol.md` §4 names for the bracket predicate — **does not exist**. `pareto_front.py` has no bracket, no DEGENERATE, no epsilon. FRONT-GAP is an **unenforced manual step**. The vehicle automates a third arbiter. **Q0 §1.2 shows the missing DEGENERATE alone misreports 28/468 cells.** Also: no joint-plane dominance (`:907-914`) although doctrine requires non-domination on **both** planes. |
| I9 | **Run-level invalidation policy** | **INVALID** | Fatal-per-cell | `status = BLOCKED_TIMING` if **any** cell fails, then `raise SystemExit(2)` (`:935`, `:948-950`). That is one fatal cell in a 200-cell Silesia run or a 40-cell enwik8 run, with the A/A predicate a bare point decision repeated 10×/2× and no multiplicity control. Fail-closed is safe, but it makes the whole job a lottery. **Gate the rate of admissible cells with a binomial CI; reserve whole-run invalidation for correctness/hash/identity only.** |
| I10 | **Multiplicity** | **INVALID** | Family-wise error | 20 arms × 5 files × 2 planes = 200 cells, 190 non-null one-sided 95% tests (enwik8: 38). Expected false `PASS_SPEED_GATE` under the global null ≈ **5.7–9.5** per Silesia job, 1.1–1.9 for enwik8. `paired.csv` ships 190 rows with an uncorrected `status` and no multiplicity column. Holm–Bonferroni within (plane, file); ship `p_holm`. |
| I11 | **A/A null** | **NEEDS-CORRECTION** | Single observation path | For `arm == "anvil-legacy"` control and candidate are the same manifest entry (`:712-714`), so `--control-observe == --candidate-observe`: one path, one expected hash. It validates the noise floor but exercises **no** dual-path freshness handling — the exact failure class the vehicle exists to prevent. Add **`A/A′`**: `brotli_dense 11` vs frozen `brotli_lw`, which the vehicle already proves byte-identical (`:599-610`). Different binary, different argv, identical output. |
| I12 | **Affinity / census symmetry** | **NEEDS-CORRECTION** | Undeclared asymmetry | `set_affinity()` returns `None` when unavailable and proceeds unpinned, while prereg §3 says unpinned timing is invalid; nothing enforces `== {cpu}`. Census measures the **unwrapped** command (`:558-559`) while timing measures the **wrapped** one, and census runs **unpinned**. `env` is inherited implicitly with `--env-json` never passed, so ANVIL knobs (e.g. `ANVIL_STREAM_SLACK`) are asserted by nothing. Raise on affinity failure; pin census; pass explicit `--env-json` and record the child env. |
| I13 | **Silesia/enwik8 byte gates** | **VALID** | — | Independently re-derived: the 12 frozen ANVIL legacy per-file values sum to exactly **46,446,995**, matching `docs/I10-REMOTE-BASELINE-CLOSURE.md:21`; enwik8 23,534,368 matches `:25`. Reference totals are the closure's Linux column. **Provenance of every hard byte gate checks out.** Preserve this; it is the strongest thing in the vehicle. |
| I14 | **Reference-family byte stability** | **NEEDS-CORRECTION** | No drift detector | Only 3 arms are hard-gated by total (`brotli-q11-lw30`, `zstd-22-long27`, `xz-9e`) plus the 2 ANVIL arms per-file. The other **16** have no byte anchor — yet on identical inputs zstd moved **−158,103 B** (Silesia) and **−61,224 B** (enwik8) across toolchain versions, while Brotli moved **0**. Publish SHA-256 per arm so a re-run *detects* drift instead of silently producing a different frontier. |
| I15 | **"One bright cell" post-hoc selection** | **INVALID wherever present** | Endpoint pre-declaration | Any design that reports the best-looking cell after observing the family is an invalid estimand. Note: **my own first parity-sweep draft did this** — it printed a top-8-by-score table of ANVIL configurations. Corrected to a full 18-row listing with the note *"every tested cell is reported; nothing is ranked or cherry-picked."* I flag my own instance because it is the same failure mode the coordinator raised. |
| I16 | **Timing sensitivity floor** | **VALID (as a limit), never satisfied in practice** | Instrument resolution | MDE ≈ 1.96·σ_log-ratio/√n. At n=13 a 5% effect needs per-pair log-ratio SD ≤ 9.20%; at n=7 a 2% effect needs ≤ 2.70%. **Hosted-runner noise on this vehicle has already produced CV 0.1756 (M15).** Therefore: **sub-5% throughput differences are unresolvable on this vehicle**, and any claim of such a difference is unfalsifiable rather than inconclusive. |

---

## 4. Experiment → measurement-class map

| Job | Estimand (what the result *is*) | Class | Status | Primary endpoint must be | Blocking invariant |
|---|---|---|---|---|---|
| **Q0** | Class counts of the retained ANVIL ladder under the frozen predicate | **C-STATIC** | **VALID — EXECUTED** | class tuple `435/28/5/0` of 468 | single frozen window; predicate frozen before execution; DEGENERATE applied. All satisfied. |
| **Q0b** | Analytic statement about search completeness | **C-STATIC** | **VALID** | the accepted/forbidden statement pair | none; no measurement |
| **Q1** | Corpus independence-unit admissibility | **C-STATIC** | **VALID** | conflict graph + SHA-256 + role assignment | 0 codec invocations, 0 sealed bytes; blind second auditor |
| **Q2** | **Whether ANVIL byte classification changes when block/window geometry is matched** | **C-BYTE / C-PARITY** | **REQUIRED — currently INVALID as written** | **bytes + round-trip hashes.** Throughput secondary/scout | geometry recorded in arm identity; DEGENERATE+FRONT-GAP implemented; all cells reported |
| **Q3** | Per-stream candidate lengths and selected codec under two frozen objectives | **C-BYTE → C-SYNTH** | **VALID** (Stage 1) | complete emitted-byte delta; disagreement buckets | one frozen non-off flag literal; no third λ arm; timing only above the floor |
| **Q4a** | Byte cross-product of typed basis × BWT backend | **C-BYTE** | **VALID** | exact byte totals | no new mode |
| **Q4b** | Fraction of BWT time outside the LF walk | **C-SCOUT** | **NEEDS-CORRECTION** | non-walk share vs the frozen ≤22% Amdahl ceiling | in-process instrumented/uninstrumented, same job; else the ≤22% test sits **inside the timing floor** (I16) |
| **Q5** | Actual correction-mask wire bytes vs ideal ceiling | **C-BYTE** | **VALID** | mask wire bytes; ideal ceiling | byte-only; historical mode11↔13 A/B already ruled confounded — keep it that way |
| **Q6** | Separate root/overlay/paging/ordering/escape capacities | **C-BYTE** | **VALID** | per-component byte deltas | **complete accounting**: components must sum to body size, or the decomposition is inadmissible |
| **Q7** | DEFLATE reconstruction value at matched transform | **C-BYTE / C-SYNTH** | **VALID if both controls present** | bytes with precomp→xz-9e **and** precomp→Brotli q11/lgwin30 | binding gate is producer-identity replayability; comparing precomp'd output against codecs that did **not** receive the transform is the same-transform-control violation |
| **Q8** | Legal declared-output / work amplification | **C-BYTE** | **VALID** | declared total vs work performed | amplification bound declared before; do not redesign the format first |
| **Q9** | aux-index W1024 vs W16 bytes and correctness | **C-BYTE** | **VALID** | byte delta + round-trip | byte/correctness only; **no timing sweep** |
| **B1–B4** | — | — | **BLOCKED** | — | B4 is correctly gated on Q2 proving residual block warmup under fair geometry |

---

## 5. Queue audit — invalid estimands and missing invariants

Defects found in `REMOTE-EXPERIMENT-QUEUE.md` itself.

**A1 — No job carries a per-arm measurement-class label. (MISSING INVARIANT)**
Ranking-rule item 3 requires "measurement validity", but no job body declares which arms are C-BYTE vs C-SCOUT. Without that label, a reader cannot tell which rows may support a token. **Every job that emits bytes must label each arm's class explicitly, and must state that a C-SCOUT row cannot emit a frontier token on its own.**

**A2 — "one bright cell" is not forbidden anywhere in the queue. (INVALID ESTIMAND RISK)**
Neither the ranking rule nor any job forbids post-hoc endpoint selection. **Amendment:** every timed job must predeclare its primary endpoint(s) and its family-level null procedure before dispatch, must report **all** tested cells including nulls, and may not select a best cell after observation.

**A3 — GHA timing is not barred from emitting a token. (INVALID ESTIMAND)**
The queue's doctrine says heavy measurement is Actions-only, and the protocol calls hosted throughput "scout-grade" — but no queue job repeats the bar. **Amendment:** *no GHA timing result, alone, may emit `FRONT-CROSSING`.* A timing cell may only corroborate a C-BYTE result.

**A4 — Q2's success criterion is not predeclared. (INVALID ESTIMAND)**
Q2 lists design requirements but no primary endpoint and no kill/promote threshold. As written, Q2 could be declared a success by any bright cell. **Amendment:** Q2's primary endpoint is the **per-file byte delta between ANVIL at the reference-comparable geometry and ANVIL at the Class-A default**, plus the resulting **classification tuple**. Throughput is reported but is not an endpoint.

**A5 — Q4b's frozen ≤22% Amdahl ceiling is compared against an unstated instrument resolution. (MISSING INVARIANT)**
The queue states the required BWT acceleration (~4.5×) and the implied ≤22% non-walk share, but not what precision the instrument can deliver. Per I16, the timing floor is ~5–10% relative on this vehicle. A 22% vs 30% distinction may be **at or inside the floor**. Q4b must predeclare its minimum resolvable non-walk share and declare the test void if the floor exceeds it.

**A6 — held-out lock dependency is stated but not wired to the byte-only jobs. (MISSING INVARIANT)**
The queue states no structured/numeric/executable promotion claim is authorized until Q1 is valid. But Q3, Q6, Q7, Q9 all produce byte-only deltas that will be *tempted* into promotion claims. **Amendment:** every byte-only job must carry the clause *"this result is discovery/negative-control/anatomy/byte-ceiling evidence only until Q1 admits the family"*, and none of them may be cited as a held-out win.

**A7 — no job declares an A/A or family-level null. (MISSING INVARIANT)**
Q2 has G2/G3. Nothing else does. Q3's A0-vs-A1, Q4b's instrumented-vs-uninstrumented, and Q9's W1024-vs-W16 are all comparisons that need a stated null or a stated attribution method.

**A8 — the "one fatal cell" policy is not carried into the queue. (INVALID ESTIMAND)**
I9's fail-closed whole-run policy is a property of the *frozen vehicle* only; if any replacement job inherits it, dispatch probability becomes a lottery. Cell-level failures must be informative; only correctness/hash/identity failures may invalidate a run.

**A9 — Class-A vs Class-B are not distinguished anywhere in the queue. (MISSING INVARIANT, resolved fact)**
I1 vs I2. Class A is geometry-confounded; Class B is at parity. Any future job that draws on `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` for a *comparative* byte claim inherits I1.

---

## 6. Q2 corrected design — byte-primary parity experiment

**Q2 is REQUIRED** (Q0 frozen ruling, reproduced in §1.2). It is *not* a 20-arm rerun and *not* an RSS or subblocking experiment. It is one geometry question.

**Primary endpoint (predeclared, single):** for each panel file, the **byte delta** `Δ = bytes(reference-comparable geometry) − bytes(Class-A default geometry)` for the ANVIL control configuration, with round-trip SHA-256 on both.

**Secondary (reported, never an endpoint):** all other arms' bytes; resident-charge; descriptor count; whole-file scout throughput.

**Geometry arms (all ANVIL, same `--parse`, differing only in `--block`):** `--block=262144` (the Class-A default, control) · `--block=4194304` (Brotli lgwin 22 parity) · `--parse=ratio` (128 MiB, whole-file; the parity reference arm). **Note the cap:** non-ratio parse is limited to 64 MiB (`anvil.cpp:5002`), ratio to 128 MiB (`:5000`).

**References:** Brotli q11 at the window the compared arm uses; xz-9e; zstd-22-long27. All at declared geometry, recorded in the arm table.

**Mandatory invariants for Q2 to enter the queue:**
1. Complete bytes + round-trip hashes as the citation-grade primary data; all transformed intermediates charged.
2. Geometry explicit in arm identity; no implicit defaults.
3. `DOMINATED → DEGENERATE → FRONT-GAP → CROSSING` implemented in code, with the adopted predicate frozen **before** dispatch and **no epsilon applied afterward to manufacture a crossing** (coordinator Q0 §"Non-decisive diagnostics").
4. All cells reported; no best-cell selection.
5. Joint-plane dominance as the primary arbiter; per-plane labels are diagnostics.
6. Raw RSS and binary size **excluded** from cross-family dominance; `resident_charge_kib` used instead.
7. Throughput, if run at all, is C-SCOUT: in-process timing, predeclared endpoint, no token emission, `DEGENERATE_TIMING_SHORT` floor declared.
8. `A/A` and `A/A′` both present; cell-level timing failure informative, run-level invalidation only for correctness/hash/identity.
9. Holm correction within (plane, file); `p_holm` shipped.
10. Per-arm measurement-class labels shipped (A1).
11. Explicit clause: no structured/numeric/executable promotion claim from this job (A6).

**Kill/promote:** if the byte delta at matched geometry is **< 1% of the Class-A ANVIL byte total**, the parity question is settled as immaterial for ratio purposes and Class-A rows may be used descriptively with the confound declared. If **≥ 1%**, all Class-A comparative byte verdicts are suspended pending re-measurement. Thresholds frozen before dispatch.

---

## 7. What remains undecided

| ID | Open question | Settled by | Can flip a ruling? |
|---|---|---|---|
| X-A | Magnitude of the Class-A geometry tax (E4 restart/model-warmup penalty; prior estimates 5–17% at 256 KiB) | Q2 primary endpoint | **Yes** — decides whether Class-A byte verdicts are usable at all |
| X-B | `overhead_share` magnitude for the process-level estimator (I5) | in-job `observe.sh … true` probe | Sets Q2's `DEGENERATE_TIMING_SHORT` floor |
| X-C | Whether `INVALID_SPEED_ACCOUNTING` (G4, `RESEARCH_LEDGER.md:4943-4944`) permanently disables the speed axis on hosted runners | coordinator ruling + recompute the retained enwik8 `raw.csv` | **Yes** — would make census-only the correct permanent output |
| X-D | Whether the adopted bracket predicate should carry a materiality epsilon at all | doctrine ruling | Changes 5 FRONT-GAP → CROSSING at ε≥0.02. **Coordinator has already ruled this is definition-sensitivity, not evidence. I concur and do not use it.** |

---

## 8. Claim hygiene

- Every number here comes from one frozen window (`w-bench-frozen-suite-20260912T120527Z` + pass2, `src/anvil.cpp` BDC90474). Its provenance file states throughput is **ranking-grade** and bytes are **citation-grade**; I use throughput only for *relative* dominance, which is all the arbiter uses it for.
- No Actions series, Windows series, and Linux series are compared to one another anywhere in this document.
- Q0 is reported as the coordinator's result; my independent run is offered as **corroboration**, and I explicitly decline to claim an arithmetic error where none exists.
- Two of my own defects are recorded rather than quietly fixed: the missing DEGENERATE predicate (which misreported 28/468 cells) and the top-8-by-score table (the one-bright-cell failure mode).
- One of my own earlier claims — "crossing is unreachable at any reference density" — is **withdrawn as an inference**: the density sweep certifies a 256 KiB candidate configuration the dense vehicle does not emit (I1 vs I2). The measurement stands; the inference did not.
- No existing file was modified. No codec run, no timing, no corpus benchmark, no commit, push, reset, clean, stash, restore, or rebase.
