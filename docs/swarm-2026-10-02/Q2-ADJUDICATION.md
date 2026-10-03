# Q2 — Block/window parity: CURRENT-STATE ADJUDICATION (against plan **Rev 3**)

**Date:** 2026-10-02
**Adjudicator:** independent adjudication (adversarial), pre-dispatch
**Target of record:** `Q2-PARITY-PLAN-SPACE-BUNNY.md` **Rev 3** (490 lines, supersedes Rev 2)
**Also read:** `Q2-PARITY-PLAN-CRITIC.md` (§1–§14 incl. the §10 coordinator correction and the
§12/§13 constructive audit), `CLOSEOUT-MATRIX-REDTEAM.md`, `Q0-DENSE-RETAINED-RESULT.md`,
`REMOTE-EXPERIMENT-QUEUE.md` §Q2, `COORDINATOR-CLOSEOUT-v1.md`, `MASTER-BRIEF.md`,
`RESEARCH_LEDGER.md:4508`, and primary source (`src/anvil.cpp`, `tools/bench_native.cpp`,
`FORMAT.md`, `tests/*.csv`, git index).

**This artifact supersedes the Rev-2-snapshot adjudication.** Findings that were true against Rev 2
and are false against Rev 3 are marked **SUPERSEDED** with the Rev 3 line that retires them, and are
not carried into the ledger. Live blockers are preserved unchanged.

**Constraints honored:** no codec invocation, no benchmark, no timing run, no build, no network, no
workflow dispatch, no queue/coordinator-file edit. All claims are read from committed source, a
retained artifact, or the git index; arithmetic is over integers already present in those artifacts.

---

## 0. Ruling of record (verbatim scope)

> **Q2 is a byte/provenance parity instrument, NOT a frontier reclassifier.**
> **No `FRONT-GAP` or `CROSSING` token may be emitted by Q2.**
> The 2×2 factors are clean: `G1−G0` and `G2−G3` are geometry main effects at fixed gate; `G0−G3` and
> `G1−G2` are gate main effects at fixed geometry; the interaction tests dependence.
> Keep `decode_threads=1` for the vehicle; parallelism is descriptive-only; BWT/`ratio` arms are
> forbidden. Retained timing remains unresolved elsewhere; Q2 cannot fix it.
> **The build-drift fidelity gate must fire before any geometry is interpreted.**

**Status: `HOLD`.**

`HOLD`, not `CANCEL`: the premise is live and cheap. Q0 ruled Q2 REQUIRED
(`Q0-DENSE-RETAINED-RESULT.md:84`); the critic ruled "Do not cancel"
(`Q2-PARITY-PLAN-CRITIC.md:751`); the red-team's own cancel condition (informative subset ≤ 4 files)
evaluates to **7** and does not fire.

`HOLD`, not `DISPATCHABLE`: Rev 3 resolved the *plan* defects. It did not resolve the *execution*
preconditions, and on two counts it moved **away** from the ruling. There is still no clean pinned
identity and no remote; **2 of the 13 control files are gitignored build artifacts**; the reference-
identity gate is still undefined; and **Rev 3 contains no byte-equality fidelity gate at all**, while
weakening G-B so the comparison the ruling mandates is expressly foreclosed.

---

## 1. What Rev 3 actually fixed — and the adjudication findings that die with it

Rev 3 is a genuine repair pass, not a re-labelling. Six of the findings from the Rev-2 adjudication
are **SUPERSEDED**.

| adjudication finding | status against Rev 3 | retiring line |
|---|---|---|
| **D1** — `GEOM-GATE-INVARIANT`/`CONFOUNDED` token; floor `11 × block_count(G0)`; band on a deterministic quantity; unrobust roll-up | **SUPERSEDED — resolved.** The floor, the 0.05 % secondary floor, and the two-value vocabulary are **withdrawn by name**. `materiality_threshold: null` and `epsilon: null` in every manifest; "no threshold and no comparison against one" in `q2_factorial.py`; the 11 B/block quantum survives only as `framing_bytes_descriptive`, "compared against nothing"; corpus roll-up is now the exact signed **sum** with "no weighting, no averaging of signs, no majority vote"; the token is `PARITY-GEOMETRY-REPORTED`, which "asserts that the contrasts were measured and published; it does not grade them" | Rev 3:178-194, 169-176, 396-399, 453-454 |
| **D4** — silent collapse of the 5 retained configs onto one `--stream-lambda=0.04` | **SUPERSEDED — resolved.** All five are restored with their exact original argv, three distinct λ values, a `config_id` → argv map, a machine-readable `retained_config_id → argv` join key, and a **collapse check** in G-A that fails on any duplicate `(parse, lambda)` pair | Rev 3:66-86, 416 |
| **D15** — Rev 2's false sentence "the only contrast in which geometry appears with the gate held at a single level is the interaction" | **SUPERSEDED — resolved.** Rev 3 names the converse error explicitly and rejects it: "`G1 − G0` and `G2 − G3` each already hold the gate fixed" | Rev 3:145-146 |
| **D16** — the ≥ 0.05 % noise floor used as a secondary decisional floor, over-attributed to the crossing criteria | **SUPERSEDED — resolved.** "The ≥ 0.05 % aggregate noise floor (`11-front-crossing-criteria.md:62`) is **not** used" | Rev 3:192 |
| **D3** (half 1) — the baseline diagnostic was unimplementable because a single-config G0 cannot reproduce a 468-cell tuple | **SUPERSEDED in substance.** Rev 3's provenance states the diagnostic "**reproduces Q0's frozen `435 / 28 / 5 / 0` tuple exactly** from the retained CSV" — i.e. it is a re-execution of the frozen sweep over the retained artifact, not a Q2 cell output. §5's wording still says "over G0", which no longer matches §15; that residual wording defect is logged as **R3-7** (minor), not as a blocker | Rev 3:479-481 |
| critic §12.1 `stream_lambda` trap (harness 0.04 vs CLI 0.01) | **SUPERSEDED — resolved.** Fixed exactly as the critic specified: λ passed explicitly per config, span `{0.04, 0.0, 0.01}` asserted, `ANVIL_STREAM_LAMBDA` made **absent** in three places (workflow, collector child env, operator shell) as gate **G-N**, plus the full bench-time default set pinned so a future default change cannot alter identity | Rev 3:74-100, 429 |

Two adjudication points also **survive Rev 3 unchanged** because they were never about the plan's
algebra:

* **`G0 − G3` is the gate main effect at small geometry; "pure window" is retired everywhere.**
  Rev 3 §2 now rejects the window reading "on the design's face" (`:142-146`) — the correct
  disposition, and it matches the ruling. `CLOSEOUT-MATRIX-REDTEAM.md:501-505` (C5.1) and its
  cancel-index row at `:578` still carry the retracted label and the retracted STOP gate.
* **`G2 − G3` is not a pure window effect either, and Rev 3 does not claim it is** — it calls it
  "geometry effect, gate OFF". For precision: with `--negate=off`, `small → large` moves window
  extension, **all** per-block encoder state resets (match-window truncation, entropy-table reset,
  recency-cache reset), **and** the per-block raw decision together. There is no arm that isolates
  window extension alone, because `src/anvil.cpp:4685` is the **only** raw-block path for
  `parse != "ratio"` (the raw fallback at `:4679-4680` is on the excluded `ratio` branch) — so a
  `--negate=off` arm on incompressible input can emit a block **larger than its input**. Rev 3's
  signed-contrast convention absorbs this; it should simply be declared.

---

## 2. Dispute 1 — is `G0−G3` a gate effect at small geometry, or a "pure window" effect?

**Adjudicated: gate main effect at small geometry.** No live disagreement between Rev 3, the critic
(§10.1), and the ruling. The critic's original §5 label was wrong and was self-retracted; Rev 3
rejects it independently; this adjudication rejects it. The only artifact still wrong is
`CLOSEOUT-MATRIX-REDTEAM.md`, which is not this lane's to edit.

**Algebra note that matters for P1.** On a file that is a single block at **both** settings, `G1 ≡ G2`
and `G0 ≡ G3` in geometry, so `I = (G1−G2) − (G0−G3) = 0` **exactly, by arithmetic**. Verified over
the frozen suite (`tests/benchmark-suite.frozen-bdc90474.plus-xz.csv`), whose largest file is
2,815,267 B — so `large` = 67,108,864 B is exactly one block for all 13 files, confirmed:

| subset | rule | files | n |
|---|---|---|---:|
| informative | `nblocks ≥ 4` | `generated.jsonl` 11, `generated.log` 8, `anvil_bench.exe` 8, `generated.sqlite` 7, `synth-jitter.bin` 4, `generated.repeat.jsonl` 4, `generated.json` 4 | **7** |
| near-control | `nblocks = 2` | `anvil.exe`, `synth-timeseries.bin` | 2 |
| strict control | single block at **both** settings | `random.bin`, `synth-arith.bin`, `src.cpp`, `doc.md` | **4** |

So the interaction is arithmetically pinned at 0 on 4 files and informative on 9. (The critic's aside
at `Q2-PARITY-PLAN-CRITIC.md:863` — "I = −(G0 − G3) on these files by construction" — remains an
algebra slip; the correct value is `I = 0`.) **Rev 3 never states this subset split**, which is a
live gap: `CLOSEOUT-MATRIX-REDTEAM.md:229` requires it predeclared before the run. Logged **R3-6**.

---

## 3. Dispute 2 — is the 2×2 decision token scientifically valid?

**Adjudicated: the design is valid and Rev 3's token is now honest. One grant-by-the-back-door
remains, and it is in §14, not §3.**

What Rev 3 got right, on the record: the complete crossed 2×2 makes every main effect hold the other
factor *exactly* (`:133-136`), so no contrast is confounded; the interaction is published as an exact
integer with "a **reader** conclusion … not a token Q2 minted" (`:294-296`); the sign label is
correctly described as "a restatement of the sign of a number already published and adds no cut
value" (`:172-173`); and the corpus level is an exact signed sum, not a vote. This is the right
instrument. Rev 2's token problem is genuinely gone.

**The residual defect — R3-1.** Rev 3's own outcome table re-grades the job:

> `:444` — "bytes move at large geometry, interaction exactly zero → **`GEOM-SENSITIVITY` measurement
> result. Authorises one follow-up: a configuration experiment with a promotion-grade decode gate.**"

against

> `:201` — "**No outcome of Q2 authorises a configuration change**".

That is a direct internal contradiction, and the `§14` row is the losing side of it, for three
reasons:

1. It is a **two-valued outcome table keyed on a value with no band** — the same shape as the token
   Rev 3 withdrew, re-entering one section later, with the outcome names (`GEOM-SENSITIVITY` vs
   "geometry and gate are coupled") doing the grading work the deleted vocabulary used to do.
2. "interaction exactly zero" is near-certain to **fail**, so the authorising branch is nearly
   unreachable: `I ≡ 0` by arithmetic on 4 of 13 files, and on files where the gate's sampling grid
   shifts with block length — `probe_incompressible` uses `stride = n/1024` on the **block** length
   (`src/anvil.cpp:3808-3812`, called at `:4685`) — the two gate toggles are different computations
   (e.g. `synth-timeseries.bin`: a 17,856 B final block → stride 17, versus 280,000 B → stride 273),
   so `I ≠ 0`. The authorising branch is therefore both unprincipled and mostly dead.
3. P1 (`:328`) pre-registers "the interaction is **exactly zero** on every (configuration, file)" but
   attaches no consequence to failure, while §14 attaches the whole configuration question to
   success. A pre-registered statement whose falsifier carries no consequence is not a prediction.

**Mandated edit (small, textual):** delete the `Authorises one follow-up` clause at `:444`, leaving the
label as a measurement-result name only, so §14 and §3.3 agree and Q2 mints no grant. P1 then stands as
an exact, consequence-free observation, which is correct.

---

## 4. Dispute 3 — source drift and GitHub-publishability

**Adjudicated: publishability FAILS on five counts; drift is four commits. Unchanged by Rev 3.**

**Drift.** HEAD = `b8eae11`, branch `i10-aux-unbwt`. Tree **dirty**: 8 modified tracked files
(`.gitignore`, `RESEARCH_LEDGER.md`, `docs/I10-AUX-UNBWT-INTEGRATION-PLAN.md`,
`docs/I10-GROTLI-FRONTIER-ARCHITECTURE.md`, `tools/paired_bench.py`, `.opencode/swarms/*`) plus a
large untracked set. `Q2-PARITY-PLAN-CRITIC.md:536-543` lists **three** post-`9caeee9` commits and
asserts "All three are BWT/aux-scoped", which it then uses to presume Class-A bytes are unchanged;
`CLOSEOUT-MATRIX-REDTEAM.md:70-83` (V4) records **four**, the fourth being `770eeb4 fix: use portable
CPUID path for clang on Linux` — a CPUID change in the very `src/anvil.cpp` that
`tools/bench_native.cpp:2` `#include`s. Both halves of the critic's presumption are void; its
conclusion survives on corrected grounds.

**Rev 3's drift reasoning is sound but its drift *gate* is absent — R3-2 (blocking).** Rev 3 argues
(B4, `:316`) that the contrasts are "drift-immune by construction" because all five are within-run,
same binary, same job, same VM. That is correct and it is the right architecture. But the ruling
requires that "**the build-drift fidelity gate must fire before any geometry is interpreted**", and
Rev 3's gate table `G-A … G-N` (`:414-429`) contains **no gate that compares any Q2 cell to the
retained Class-A rows**. G-H merely says the baseline diagnostic is "compared against Q0 and reported
either way" (`:423`) — *reported*, not *blocking*. Worse, G-B (`:417`) says `anvil.exe` /
`anvil_bench.exe` are "**this build's outputs under test** — identity recorded, **never asserted
against historical hashes**". Rev 3 thus not only omits the fidelity gate, it writes into its own
validity table an explicit prohibition on the comparison the ruling mandates. The critic's G1 and
the red-team's Q2F (`CLOSEOUT-MATRIX-REDTEAM.md:469-475`, whose STOP outcome is "The delta is **build
drift, not a parity result**; the retained CSV is not a control; Q2's baseline must be re-established
and Q2 defers") are both dropped.

**Publishability — five failures, all verified.**

| # | failure | evidence |
|---|---|---|
| P1 | **No git remote configured.** `git remote -v` returns empty. No push target; no Actions dispatch destination | verified |
| P2 | **No LICENSE, no CONTRIBUTING** at the root | verified |
| P3 | **The workflows Q2 would need are untracked** (`.github/workflows/anvil-i10-dense-frontier.yml`, `anvil-i10-g5-paged-dictionary.yml`). Untracked workflow files do not exist in a remote checkout | 14 of 16 workflows tracked |
| P4 | **No clean pinned candidate.** `COORDINATOR-CLOSEOUT-v1.md:153`: "Actual GitHub Actions dispatch requires a clean pinned source identity and an explicitly approved workflow path. Existing local dirty state is not authority for remote source identity." The dirty tree is not that authority | verified |
| P5 | **2 of the 13 frozen-suite control files are gitignored build artifacts.** `anvil.exe` and `anvil_bench.exe` are named in `.gitignore`; the 11 data files are tracked. A clean pinned checkout **cannot reconstruct the fidelity gate's control set**, and those bytes are a function of the pinned toolchain, not of the commit | 15 tracked under `tests/corpus` vs 26 on disk |

P5 appears in **neither** Rev 3 nor the critic, and it is now the sharpest form of the fidelity
problem: for 2 of 13 files a byte-equality gate is **unevaluable from a pinned checkout** without a
reproducible-build attestation. Rev 3's G-B gestures at this ("this build's outputs under test") but
resolves it by declining to compare — which is precisely what the ruling forbids.

Also recorded: `docs/swarm-2026-10-02/` is **entirely untracked**, so neither plan nor this
adjudication is under version control. `CLOSEOUT-MATRIX-REDTEAM.md:60-66` confirms the same pattern
wider out (`pe-*.exe`, `synth-columnar-align.bin`, `synth-drift-stride.bin`,
`synth-telemetry-f64.bin` are untracked ⇒ a fresh clone has none of them), so no queue statement of the
form "the corpus is in the repo" is true for those families.

---

## 5. Dispute 4 — can Q2 run before a clean pinned candidate exists?

**Adjudicated: NO. Hard prerequisite plus mandatory ordering. Unchanged by Rev 3, and now sharper,
because the fidelity gate that would have caught the risk has been removed from the plan.**

1. **Policy.** `COORDINATOR-CLOSEOUT-v1.md:153` requires a clean pinned source identity for dispatch;
   `REMOTE-EXPERIMENT-QUEUE.md:6` makes heavy corpus/performance measurement Actions-only.
2. **Sequencing.** The red-team places Q2 in **Wave 3**, conditional on Waves 0–2, and makes Q2's
   fidelity gate an *arm of* the Wave-1 `QBYTES` pass (`CLOSEOUT-MATRIX-REDTEAM.md:469-475, 495-497,
   625`). Q2 is downstream of the fidelity pass, not parallel to it. Rev 3 does not mention this
   dependency.
3. **Expected-stop risk.** Four drift commits, one CPUID. With no byte-equality gate in Rev 3, this
   risk is currently **undetectable at dispatch**.
4. **Two control files are unproducible from a pinned checkout** (§4 P5). Pinning does not fix this;
   it needs a build attestation.

**Mandatory order:** clean pinned candidate + corpus/build attestation → Wave-1 `QBYTES`/`Q2F`
fidelity pass (per-file byte-equality verdict) → *then* Q2.

**Reference identity remains unresolved — D5 (live, blocking for the parity half).**
`Q2-PARITY-PLAN-SPACE-BUNNY.md:102-110` proposes `R1` at `lgwin=30` from a newly authored helper
(`prototypes/swarm-2026-10-02/q2-parity/q2_brotli_geom.cpp`), while the retained harness uses
`BROTLI_DEFAULT_WINDOW` = `lgwin 22` (`tools/bench_native.cpp:39`). Q0 precondition 2 requires
"preserve the frozen reference/candidate source identities"
(`Q0-DENSE-RETAINED-RESULT.md:72`). `R3` (256 KiB independent streams) has **no frozen counterpart at
all** — a new quantity from a new producer. G-F (`:421`) and §11 precondition 2 (`:361`) still read
"R1 reproduces the published whole-file Brotli total within **frozen tolerance**": *frozen tolerance*
is undefined, and the check is on a **total**, not per file — unauditable as written. Two residual
slips: `R1` is called "the bar" citing `11-front-crossing-criteria.md:21`, which names **two** binding
bars (Brotli q11/lw30 **and** `xz -9e`, with `xz -9e` the host bar); and the `+13.74 %` in §10
(`:342`) is imported uncited from a Silesia-scale blocked-vs-whole census
(`15-crossblock-memory-space-bunny.md:127-140`, bases ≈ 33 MB). The latter is now explicitly marked
"context only … not read by any code path" (`:337, 343`), which mitigates it to minor — but it should
still be cited, because it is the only number in §10 that came from another lane's measurement.

**Note on what Rev 3 got right here.** G-F's failure outcome is now correctly scoped: "the parity
residual becomes diagnostic-only; **the factorial contrasts are unaffected**, since they depend on no
reference arm" (`:421`). Rev 2's phrasing — "the *interaction token still stands*" (`:363`) — referred
to a token that no longer exists. Logged as stale residue (**R3-3**).

---

## 6. Dispute 5 — block/parallelism, without converting structural capacity into a throughput claim

**Adjudicated: one parallelism statement is admissible, and it is arithmetic. Rev 3 still does not
make it — R3-4 (live).**

**Admissible — structural capacity, exact integers, zero measurement.** From `src/anvil.cpp:4908`,
`nthreads = min(opt.decode_threads, segs.size())`. Every Class-A file is a **single** block at
67,108,864 B, so `segs.size() == 1`, so **`decode_threads > 1` is unreachable at large geometry at any
thread count** — the CRC-gated per-block parallel path (`:4862-4867`, `:4906-4912`) is structurally
dead there. This follows from source plus file sizes. It needs no runner, no clock, no corpus.

| file | bytes | `capacity(G0)` = `nblocks` | `capacity(G1)` | delta |
|---|---:|---:|---:|---:|
| `generated.jsonl` | 2,815,267 | 11 | 1 | 10 |
| `generated.log` | 1,942,280 | 8 | 1 | 7 |
| `anvil_bench.exe` | 1,929,216 | 8 | 1 | 7 |
| `generated.sqlite` | 1,740,800 | 7 | 1 | 6 |
| `synth-jitter.bin` | 974,920 | 4 | 1 | 3 |
| `generated.repeat.jsonl` | 940,000 | 4 | 1 | 3 |
| `generated.json` | 827,664 | 4 | 1 | 3 |
| `synth-timeseries.bin` | 280,000 | 2 | 1 | 1 |
| `anvil.exe` | 268,800 | 2 | 1 | 1 |
| `random.bin` | 262,144 | 1 | 1 | 0 |
| `synth-arith.bin` | 256,000 | 1 | 1 | 0 |
| `src.cpp` | 32,512 | 1 | 1 | 0 |
| `doc.md` | 2,724 | 1 | 1 | 0 |

Emit as `structural_capacity_units`, `measurement: none`, `decisional: false`, with
`throughput_estimate: null`.

**R3-4.** Rev 3 §7 pins `--decode-threads=1` and then computes `max_parallel_units = min(1,
block_count) = 1` for **every** arm (`:275-278`), which evaluates the capacity at the single setting
that erases the one structural finding the job exists to disclose. Fix, with no run required: keep the
**vehicle** at 1 and declare `capacity(N) = min(N, block_count)` symbolically, with the table above.

**Forbidden — any magnitude.** "2–4× decode regression" (`Q2-PARITY-PLAN-CRITIC.md:43-45,578-581`) and
"decode regression of order core-count" (`CLOSEOUT-MATRIX-REDTEAM.md:515`) are **unmeasured throughput
claims**. Each requires linear scaling in thread count, a compute-bound (not memory-resident) working
set, and a runner whose vCPU count equals realised benefit — none established, and the largest file
here is 2.8 MB (L2/L3-resident). They also sit against the project's own measured decode dispersion
of **CV 5.5–35.5 %** (`tests/noise-floor.csv`, via `Q2-PARITY-PLAN-CRITIC.md:437-441`) and hosted
pre-window utilisation of **26.9 % / 9.2 %** against a **< 5 %** gate (`:459`).

**Consequence, unchanged and correct.** Q2 cannot evaluate `P_loss ≤ 0.95` / `R_delta ≤ 1.05`
(`15-crossblock-memory-fledge.md:514, 1034-1035` — a hard gate on any parity result) and therefore
cannot license a configuration change; a block-size result is `{engineering}` and one write-up from an
adaptive-blocking claim already occupied by Brotli's meta-block splitter and zstd's
`targetCBlockSize` (`Q2-PARITY-PLAN-CRITIC.md:711-714`). Rev 3 §7/§8/§14 keep this disposition
correctly, subject to R3-1.

---

## 7. Prerequisites — exact, ordered, mechanically checkable

| # | prerequisite | check | status |
|---|---|---|---|
| **P1** | Clean pinned candidate: commit SHA in the job text; tree clean at that SHA; `source_tree_ref` + `evaluated_at` recorded; `source_dirty` asserted **about the pinned checkout**, never the operator's worktree (`REMOTE-EXPERIMENT-QUEUE.md:316-324`) | `git status --porcelain` empty at SHA | **UNMET** |
| **P2** | Candidate is publishable: remote configured, license present, required workflow path tracked | `git remote -v`; LICENSE; `git ls-files .github/workflows` | **UNMET** (all three) |
| **P3** | **Control set producible**: `anvil.exe` and `anvil_bench.exe` reproducible from the pinned checkout under a recorded toolchain — **or** the fidelity gate scoped to the 11 tracked data files with the 2 exclusions declared before the run | reproducible-build attestation for 2 binaries | **UNMET** |
| **P4** | **Byte-equality fidelity gate fires first** and reports per-file byte equality against `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv`. Any Class-A byte difference ⇒ STOP, report as build drift, Q2 defers. **Absent from Rev 3; G-B must also drop its "never asserted against historical hashes" clause** | per-file byte comparison; 4 drift commits acknowledged (`770eeb4` is CPUID) | **NOT RUN — GATE MISSING (R3-2)** |
| **P5** | Canonical argv recorded per config, all three λ values, `ANVIL_STREAM_LAMBDA` absent, no duplicate `(parse, lambda)` | Rev 3 G-A, G-N | **SATISFIED IN SPEC** |
| **P6** | No threshold, no epsilon, no grading token anywhere in the emitted artifacts; exact signed contrasts published; **and** §14's "Authorises one follow-up" clause deleted so Q2 mints no grant | Rev 3 §3.1/§3.2 pass; §14 fails | **PARTIAL (R3-1)** |
| **P7** | No `FRONT-GAP` or `FRONT-CROSSING` label emitted by any Q2 artifact; the token scan must cover **both** strings, not only `FRONT-CROSSING` | `--assert-no-crossing-token` currently scans one string only (`:242-243`); §5 vocabulary still emits `FRONT-GAP` (`:235`) and §12's predicate block keeps it (`:383`) | **FAILED (R3-5)** |
| **P8** | Reference identity resolved: **per-file** byte equality of `R1` against the frozen Brotli rows (not a total, not an undefined tolerance), Brotli library identity pinned, `xz -9e` bar named or its exclusion declared | per-file equality | **UNMET (D5)** |
| **P9** | Informative (7) / near-control (2) / strict control (4) subsets predeclared before the run; control assertion is `G1−G0 = 0` and `G2−G3 = 0` on the 4 single-block files | `CLOSEOUT-MATRIX-REDTEAM.md:229`; critic §10.2 | **UNMET (R3-6)** |

---

## 8. Falsifiers — exact conditions that change the status

**`HOLD → DISPATCHABLE`** (all required): P1–P4 and P6–P9 satisfied; P4 returns **byte-exact equality on
the admitted control set**; P5 already satisfied in spec.

**`HOLD → CANCEL`** (any one sufficient):

| condition | consequence |
|---|---|
| Any Class-A byte differs from the frozen CSV (P4) | **build drift** — the retained CSV is not a control; Q2's baseline must be re-established and Q2 defers |
| Informative subset (`nblocks ≥ 4`) ≤ 4 files after the census | *corpus structurally under-powered for the parity question* — **a null, not a result** |
| `G1−G0 ≠ 0` or `G2−G3 ≠ 0` on any strict-control file (the 4 single-block files) | harness/attribution leak — geometry cannot act there |
| G-F cannot be made a **per-file exact** check (Brotli library unpinnable, or `R1` not byte-equal to the frozen rows) | the parity residual becomes diagnostic-only; the exact contrasts still stand |
| Any arm contains `--parse=ratio`, `--ratio-backend`, `--bwt-aux=on` | B1 violated — a backend win would be reported as a geometry win |
| Any Q2 artifact emits `FRONT-GAP` or `CROSSING` | **ruling violated** — reject the artifact, not the job |
| Any parallelism magnitude (×, "order core-count") appears without a promotion-grade throughput instrument | unmeasured throughput claim — reject the artifact |
| Arm set re-scoped to include ratio/BWT | Q9 suspended; B1 re-enters |
| Pinning resolves only by dirtying the tree, or by asserting `source_dirty` about the worktree | P1 unsatisfiable; escalate — dispatch is impossible under `COORDINATOR-CLOSEOUT-v1.md:153` |

**Unconditional, regardless of Q2's fate — D19 (live).** The 5 retained `FRONT-GAP` cells stay
unresolved and **unowned**: they are held out of `DOMINATED` by a 7.8 % encode margin inside the
measured 5.5–35.5 % dispersion band (`Q2-PARITY-PLAN-CRITIC.md:454-459`), and the retained rows
predate the threading plumbing (`tests/thread-attestation.csv:2`). Per the ruling, **Q2 cannot fix this
and does not try.** The successor remeasurement must re-establish the retained window under PR-4
attestation and must carry the deferred predicate/ε defects (`parity_sweep.py:44-77`, mixed axes;
`:123`, `eps = 0.0` vacuous at the decisive setting) into its own preregistration. **No owner exists.**

---

## 9. Defect ledger — current state

**Live against Rev 3:**

| id | defect | where | severity | disposition |
|---|---|---|---|---|
| **R3-2** | **No byte-equality fidelity gate exists** (G-A…G-N); G-B expressly forbids comparing build products to historical hashes. The ruling mandates the gate; the critic's G1 and Q2F are both absent | `:414-429`, esp. `:417` | **blocking** | add the gate; amend G-B |
| **R3-5** | **`FRONT-GAP` is still emitted.** §5 vocabulary (`:235`) and §12 predicate block (`:383`) retain it; `--assert-no-crossing-token` scans only the bare string `FRONT-CROSSING` (`:242-243`). Ruling: no `FRONT-GAP` **or** `CROSSING` token | `:235,242-243,383` | **blocking** | drop `FRONT-GAP` from the vocabulary; widen the scan to both strings |
| **D5** | G-F undefined ("frozen tolerance"), checks a total not per file; `R1` `lgwin=30` vs frozen `lgwin=22`; `R3` has no frozen counterpart; Q0 precondition 2 unmet | `:102-110,361,421` | **blocking** | per-file exact equality + pinned library identity |
| **P1–P4** | No clean pinned candidate; no remote; no license; required workflows untracked | §4 | **blocking** | P1, P2 |
| **P5 / D7** | `anvil.exe` / `anvil_bench.exe` are gitignored — 2 of 13 control files unproducible from a pinned checkout | §4 P5 | **blocking** | P3 |
| **R3-1** | §14's "Authorises one follow-up" contradicts §3.3's "No outcome of Q2 authorises a configuration change"; re-grades the job on an exact value with no band, keyed on a predicate (`interaction exactly zero`) that `I ≡ 0` arithmetic pins on 4 files and that §14 makes near-dead | `:201, 328, 444-445` | severe | delete the clause; keep the label as a measurement name |
| **R3-4** | Capacity computed at `decode_threads=1`, erasing the structural finding | `:275-278` | severe | declare `min(N, block_count)` symbolically |
| **D8** | (same as R3-4) | — | — | merged into R3-4 |
| **R3-6** | No predeclared informative/control subset; `CLOSEOUT-MATRIX-REDTEAM.md:229` requires it | absent from Rev 3 | moderate | P9 |
| **D13** | G-L counts "four `{synthetic}` cells"; the frozen 13-file suite contains **3** (`synth-jitter`, `synth-timeseries`, `synth-arith`) | `:427` | moderate | correct the count |
| **R3-3** | Stale Rev-2 residue: title still says "byte-only decision token" (`:1`); §0 still calls it "Q2's decision token … geometry sensitivity" (`:35-36`); §11 precondition 1 authorises "this **Rev 2**" (`:359`); §11 precondition 2 says "the *interaction token still stands*" (`:363`); §6 says "Rev 2 removes it" (`:264`) | `:1,35-36,264,359,363` | moderate | retarget to Rev 3 |
| **R3-4b** | §11 cost arithmetic stale: "4 candidate cells × 13 files + 2 reference arms × 13 files" ignores the 5-config multiplier. Actual shape = 5 × 4 × 13 = **260** candidate cells + 26 reference | `:372` | minor | restate; still well inside a 120-minute budget |
| **R3-7** | §5 says the diagnostic runs "over G0" while §15 states it reproduces the 468-cell tuple "from the retained CSV" — the tuple cannot come from 130 G0 cells | `:224-225` vs `:479-481` | minor | reword §5 |
| **D14** | `+13.74 %` imported uncited from a ~33 MB Silesia blocked-vs-whole census; now marked context-only and not read by any code path | `:342` | minor | cite, or drop |
| **D17** | No rev-1 raw fallback: `--negate=off` arms can emit blocks larger than input (`src/anvil.cpp:4685` is the only raw path for `parse != "ratio"`) | absent from Rev 3 | minor | declare; contrasts are signed |
| **D18** | `docs/swarm-2026-10-02/` is untracked — neither plan nor this adjudication is under version control | verified | moderate | commit before any dispatch decision |
| **D19** | Successor timing remeasurement unowned; the 5 `FRONT-GAP` cells unresolved | §8 | **blocking (elsewhere)** | create and assign |

**Live, in documents this lane does not own (retirements to be applied by the coordinator):**

| id | defect | where |
|---|---|---|
| **D9** | "Pure window" naming plus a STOP gate built on it: `G0 − G3 = the pure window/entropy-restart answer`; STOP if `G0−G3 ≠ 0` on `nblocks ≤ 2` — would abort a healthy run on `random.bin` | `CLOSEOUT-MATRIX-REDTEAM.md:501-505, 578` |
| **D10** | Critic's G8 forbids emitting `FRONT-GAP` **at all**, which would make reproducing Q0's `5` impossible; narrower than needed and inconsistent with the ruling's actual text | `Q2-PARITY-PLAN-CRITIC.md:897, 990` |
| **D11** | "I = −(G0 − G3) on these files by construction" is an algebra slip; correct value is `I = 0` | `Q2-PARITY-PLAN-CRITIC.md:863` |
| **D12** | C2.4's predicate "≈8/13 single-block at 256 KiB" is wrong (4/13 single-block, 6/13 at ≤ 2); does not fire on either reading | `CLOSEOUT-MATRIX-REDTEAM.md:463, 573` |

**Superseded against Rev 3 — do not re-raise:** the Rev-2 materiality floor and
`GEOM-GATE-INVARIANT`/`CONFOUNDED` token (**D1**); the single-`stream_lambda` config collapse
(**D4**); the false "only the interaction holds a factor fixed" sentence (**D15**); the 0.05 %
secondary floor (**D16**); the unimplementable-tuple objection in substance (**D3**, half 1);
and the `stream_lambda` harness-vs-CLI trap from critic §12.1.

---

## 10. What this adjudication did not do

No codec was invoked. No corpus byte was compressed or decompressed. No timer ran, no benchmark ran,
no build ran. No network call and no workflow dispatch. No coordinator-owned file
(`REMOTE-EXPERIMENT-QUEUE*.md`, `COORDINATOR-*.md`, `MASTER-BRIEF.md`, `CLOSEOUT-MATRIX-REDTEAM.md`) and
no other lane's report was created, edited, or deleted — Rev 3 itself was read only. The only file
written is this one. Every retirement and every mandated edit recorded here is **for the coordinator
and the plan's owner to apply**; none was applied to another document.

---

## 11. Verdict

> ## `HOLD`
>
> **Not `CANCEL`.** The premise is live and cheap. Q0 ruled Q2 REQUIRED; the critic ruled "do not
> cancel"; the red-team's own cancel condition (informative subset ≤ 4) evaluates to **7**.
>
> **Not `DISPATCHABLE`.** Rev 3 repaired the plan and the plan is now scientifically sound: the
> complete crossed 2×2 holds each factor exactly fixed, the invented materiality floor and its
> two-value vocabulary are withdrawn by name, corpus level is an exact signed sum rather than a vote,
> all five retained configurations are restored with their three distinct `stream_lambda` values, the
> `ANVIL_STREAM_LAMBDA` environment-identity trap is gated in three places, and the "pure window"
> mislabel is rejected on the design's face. **The plan is no longer the blocker. The execution
> preconditions are.**
>
> **Five live blockers, none of them an algebra problem:** no clean pinned identity and no remote; no
> LICENSE; required workflows untracked; **2 of the 13 control files are gitignored build artifacts**
> that no pinned checkout can reproduce; and the reference-identity gate still says "frozen tolerance"
> against a total rather than per file.
>
> **Two places where Rev 3 moved away from the ruling and must be corrected before dispatch.** First,
> **`R3-2`**: the ruling requires the build-drift fidelity gate to fire before geometry is
> interpreted, and Rev 3's `G-A … G-N` contains **no byte-equality gate at all**, while G-B writes in
> that the build products are "never asserted against historical hashes" — prohibiting the very
> comparison the ruling mandates. With four drift commits including a CPUID change, the gate is the
> single most likely tripwire in the program and it has been deleted from the plan.
> Second, **`R3-5`**: `FRONT-GAP` is still in §5's emitted vocabulary and in §12's predicate block, and
> the forbidden-token scan checks only `FRONT-CROSSING`. The ruling bars both tokens.
>
> **Two bounded edits settle the rest.** Delete the "Authorises one follow-up" clause at `:444` so
> §14 stops contradicting §3.3 and Q2 mints no configuration grant (**R3-1**); and declare
> `capacity(N) = min(N, block_count)` symbolically instead of evaluating it at `N = 1` (**R3-4**),
> because structural capacity — not a throughput number — is the only parallelism claim Q2 can make.
>
> **`G0 − G3` is the gate main effect at small geometry, confirmed.** "Pure window" remains live in
> `CLOSEOUT-MATRIX-REDTEAM.md` C5.1 and its cancel-index row, where its STOP gate would abort a healthy
> run on `random.bin`; those are coordinator retirements. On the 4 single-block files the interaction is
> `0` by arithmetic, not by measurement, and the 7/2/4 subset split must be predeclared so that
> arithmetic is never mistaken for evidence.
>
> **One obligation is unowned and outlives this ruling:** the 5 retained `FRONT-GAP` cells remain
> unresolved, held out of `DOMINATED` by a 7.8 % encode margin inside the measured 5.5–35.5 %
> dispersion band, on rows that predate the threading plumbing. No byte-primary job can move that
> axis. A successor remeasurement under PR-4 attestation must be created, owned, and must inherit the
> deferred predicate/ε defects.

---

# 12. Rev-6 reconciliation (additive)

**Appended 2026-10-02. This section supersedes §0, §7 and §9 of this artifact for anything Rev 6
changed. Sections 1–11 are left intact as the Rev-3-snapshot record; where the two disagree, §12
governs.**

**Target of record:** `Q2-PARITY-PLAN-SPACE-BUNNY.md` **Rev 6** (569 lines).
**Prototype bytes read, not just cited:** `q2_arms.py` (42,459 B), `q2_factorial.py` (28,938 B),
`q2_collect.py` (14,390 B), `q2_brotli_geom.cpp` (10,367 B), `anvil-q2-parity.DRAFT.yml` (19,172 B),
`README.md` (9,343 B).

**Verdict unchanged: `HOLD`.** Three of the six verification points are now satisfied, two are not,
and one is satisfied except for a single uncovered emission path. The environment blockers from §4
are untouched. No codec, network, or heavy run was performed; this is static reading of the plan, the
prototype sources, and the workflow draft.

## 12.1 The six verification points

| # | point | result |
|---:|---|---|
| 1 | no frontier vocabulary emitted anywhere, incl. diagnostics and scanners | **SUBSTANTIALLY RESOLVED — one live leak** |
| 2 | byte-equality retained-twin fidelity gate exists and precedes geometry interpretation | **NOT SATISFIED — exact missing gate identified** |
| 3 | R1 reference identity per-file exact / pinned | **NOT SATISFIED — partially improved** |
| 4 | plan does not self-authorize a follow-up from a sign/zero/non-zero result | **RESOLVED** |
| 5 | parallel capacity symbolic `min(N, nblocks)`, not measured at N=1 | **NOT SATISFIED** |
| 6 | build-output rows handled honestly | **RESOLVED, well** |

### (1) Frontier vocabulary — resolved except for one uncovered emission path

The predicate is **gone in fact, not just in prose**. A token scan across all six prototype files
returns `FRONT-CROSSING`, `FRONT-GAP`, `BASELINE-CROSSING-CANDIDATE`, `DOMINATED`, `DEGENERATE` only in
source declarations (`q2_factorial.py:10,64-69`; `q2_arms.py` naming which retained rows to run;
`README.md:63-64`). There is **no** `DEGENERATE_RATIO`, no `eps`, no bracket test, and no dominance test
anywhere in the prototypes.

The scanner is real and well built: `assert_no_frontier_vocabulary` (`q2_factorial.py:453-483`) covers
all five tokens (`:64-70`), uses a narrow three-marker declaration exemption so it cannot fail on its own
constant (`:71-75`, `:472-474`), and the workflow invokes it over **eight** emitted artifacts
(`anvil-q2-parity.DRAFT.yml:294-307`).

**Live leak (R6-5).** `anvil-q2-parity.DRAFT.yml:320` writes the string
`5 retained FRONT-GAP configs x 4 cells = 20 candidate cells` into the Actions **step summary**. The step
summary is **not** among the eight scan targets at `:298-307`. A frontier token is therefore emitted
into a published run artifact outside the scanner's own coverage — the only surviving path by which Q2
emits the vocabulary it forbids. One-line fix: reword to "5 retained configurations" (the class label
carries no information the reader needs) and/or add the summary file to the scan set.

### (2) Fidelity gate — the exact missing gate

**What exists.** `retained_byte_fidelity()` (`q2_factorial.py:397-446`) compares this run's G0 bytes to
the retained Class-A `compressed_bytes` per (configuration, file) and emits `retained_bytes`,
`observed_g0_bytes`, `byte_identical`, `delta_bytes`, `delta_pct_of_retained` (`:428-434`).
`retained_twin_fidelity()` (`:292-333`) does the twin-structure check. Plan §5.1–§5.3 (`:281-297`) and
gates G-H/G-B describe them. The **computation** is correct and is exactly what the ruling asked for as a
question.

**What is missing, precisely — two things, and both are required.**

1. **Precedence.** `main()` assembles the contrast result — including `retained_twin_fidelity` at
   `q2_factorial.py:212` — and only *afterwards* assigns `result["retained_byte_fidelity"]` at `:604`.
   The contrasts are therefore computed and would be written before retained byte fidelity is even
   evaluated. Nothing establishes the required ordering "fidelity first, geometry second."
2. **Blocking.** Every return path sets `decisional: False` (`:405, 419, 431, 437`) and the emitted note
   states differences are "provenance observations, not invalidations, and Q2 attaches no label to them"
   (`:442-444`). Plan G-H agrees: "recorded only — a difference is a provenance observation, **not** an
   invalidation" (`:498`), and §5.1 says a difference "is **not** an invalidation" (`:286-288`).

This is the gate the standing ruling requires — *"the build-drift fidelity gate must fire before any
geometry is interpreted"* — and it is the gate Rev 3 lost and **Rev 6 has not restored**. Against four
drift commits including a CPUID change, this is the single most likely pre-dispatch tripwire in the
program, and today it is structurally incapable of stopping anything.

**Named the missing gate — G-S.**

```
G-S  Retained byte fidelity is evaluated FIRST on stratum A (the 11 git-tracked files)
      and must be byte-identical before any contrast is published or read.
      Required:  retained_byte_fidelity(A) computed before compute_contrasts output is written,
                 and every stratum-A pair byte_identical == true.
      On any divergence: non-zero exit; contrasts WITHHELD; verdict BASELINE-VOID (build drift).
      Stratum B (anvil.exe, anvil_bench.exe) is exempt by construction -- see (6).
```

Scope note: `G-S` must bind on **stratum A only**, which is why point (6)'s resolution is a prerequisite
for point (2) rather than a cosmetic improvement. The 11 tracked files are the only objects a pinned
checkout can reproduce, so they are the only rows against which byte-identity is even well-posed.

### (3) R1 reference identity — partially improved, still failing

**Improved, and correctly.** `--lgwin` is now **mandatory and never a library default**
(`q2_brotli_geom.cpp:131-134, 146-147`), validated to 10..30 (`:79`), asserted per arm
(`q2_arms.py:484-485`), and the container records `lgwin` explicitly (`q2_brotli_geom.cpp:170-173`) so
the setting is auditable from the bytes. Toolchain and library versions are captured in provenance
(`anvil-q2-parity.DRAFT.yml:189-195`, including a `dpkg-query` record for `libbrotli-dev` and
`libzstd-dev`).

**Still failing, on four counts.**

1. **G-F is unchanged and unauditable.** Plan `:496` and §11 precondition 2 `:432` both still read
   "R1 reproduces the published whole-file Brotli **total** within **frozen tolerance**". *Frozen
   tolerance* is undefined, and a total cannot localise a divergence to a file.
2. **No per-file exact comparison exists in code.** The token scan finds no retained-Brotli row lookup,
   no tolerance constant, and no `BrotliEncoderVersion()` capture anywhere in the prototypes. G-F has no
   implementation counterpart at all.
3. **The producer is not pinned.** `libbrotli-dev` is installed **unversioned**
   (`anvil-q2-parity.DRAFT.yml:161-162`), so R1/R3 bytes depend on whatever the runner's package archive
   serves at run time — not on the pinned commit. Recording the version afterwards is provenance, not
   identity. The fail-closed blob set (`:121-126`) covers `src/anvil.cpp`, `tools/bench_native.cpp`,
   `CMakeLists.txt`, the frozen suite, `tests/noise-floor.csv`, `CHECKSUMS.txt` — and **no** reference-row
   or library artifact.
4. **The comparison is not well-posed as written.** R1 is lgwin **30**; the retained grid's Brotli rows
   were produced at `BROTLI_DEFAULT_WINDOW` = lgwin **22** (`tools/bench_native.cpp:39`). Byte-equality
   against the retained Brotli rows therefore cannot succeed by construction, and the plan does not say
   so. It must choose: either compare against a **re-derived lgwin-30** reference — in which case the
   result is *not* "the retained grid" and must not be described as such — or drop the byte-equality
   claim and restate G-F purely as "reference identity pinned by explicit lgwin + package version".

Also unchanged: R1 is still called "the bar" citing `11-front-crossing-criteria.md:21`, which names
**two** binding bars (Brotli q11/lw30 **and** `xz -9e`). xz remains absent from the arm set.

### (4) Self-authorization — RESOLVED (this closes R3-1)

The "Authorises one follow-up" clause is **gone**. §14's outcome row now reads "**Not** a mechanism.
**Not** a configuration decision." (`:522`); §3.3 is "**Nothing.**" (`:201-205`); §8's three blockers
stand unchanged (`:349-365`); P1–P3 are explicitly trigger-free with the Rev-4 withdrawal note at
`:397-401` ("A measurement that comes out differently from a hypothesis is a measurement, not an
invalidation"). Code agrees: `dispatch_authorized: False` and `promotion_authorized: False`
(`q2_arms.py:618, 622`), no other `authoriz*` string in the tree, and stratum interpretation is
"coordinator-only; Q2 attaches no label to any stratum" (`q2_factorial.py:269`).

**R3-1 is closed.** No sign, zero/non-zero, monotonicity, or residual result authorizes anything.

### (5) Parallel capacity — NOT SATISFIED

The symbolic capability now exists but is never used. `decode_units(segments, decode_threads=…)`
correctly returns `min(decode_threads, segments)` per `src/anvil.cpp:4908` (`q2_arms.py:273-279`) — the
parameter is there. But it is called exactly once, with the default, at `q2_arms.py:369`
(`"max_parallel_decode_units": decode_units(segs)`), and `PINNED_DECODE_THREADS = 1` (`:74`). Every
emitted value is therefore `1`, and the plan says so plainly (`:337`: `max_parallel_units = min(1,
block_count) = 1` for every arm). Same defect as Rev 3: the structural finding is evaluated at the one
setting that erases it.

**Fix, no run required.** Emit a declared, non-measured field alongside the vehicle value:

```
capacity_vehicle          = min(1, block_count)          # the measured comparison vehicle
capacity_at_policy        = min(N, block_count)         # N = shipped default decode policy; declared, not run
capacity_delta            = capacity_at_policy(G0) - capacity_at_policy(G1) = block_count - 1
throughput_estimate       = null   # reason: unmeasured; magnitude not derivable from capacity
```

Per-file, verified against the frozen suite: `block_count` = 11, 8, 8, 7, 4, 4, 4, 2, 2, 1, 1, 1, 1 →
`capacity_at_policy(G1) = 1` for all 13 (every file ≤ 2,815,267 B < 67,108,864 B); deltas 10, 7, 7, 6,
3, 3, 3, 1, 1, 0, 0, 0, 0. Magnitude language ("2–4×", "order core-count") remains forbidden.

### (6) Build-output rows — RESOLVED, and better than required

This is the strongest part of Rev 6 and it converts the §4 P5 blocker into a declared non-cited stratum:

* Strata **A** (11 tracked, Q0-compatible) / **B** (2 build-output-under-test) / **C** (explicitly mixed
  discovery aggregate) are computed in code with a `pooling_warning` (`q2_factorial.py:249-269`).
* `identity_role` is set per slot (`q2_arms.py:782`; `BUILD_OUTPUT_SLOTS` at `:106`), the population
  report splits `build_output_slots` / `tracked_slots` (`:756-757`), and the collector hash-asserts
  **only** `identity_role == "tracked"` (`q2_collect.py:299-301`) — so a fresh Linux build artifact is
  never compared to a historical Windows hash.
* Per-file contrasts are published unchanged regardless of stratum, so stratification hides nothing
  (`q2_factorial.py:243-247`; plan `:239-240`).
* The self-test asserts stratum membership (`:545-550`), and gate **G-R** makes population derivation
  fail-closed from the **frozen CSV** rather than `CHECKSUMS.txt` — a real trap (24 entries vs 13) that
  Rev 5 caught and closed.

**Consequence for (2):** byte-identity can only bind on the 11 tracked files, so **G-S must be scoped to
stratum A**. That is the correct scope and it is achievable — provided G-S exists at all.

## 12.2 Current blockers only

Everything in §9's live ledger that Rev 6 fixed is dropped here and not carried. These five remain.

| id | blocker | exact location | exact fix |
|---|---|---|---|
| **B1** | **No clean pinned candidate, no remote, no LICENSE, workflow path untracked.** `COORDINATOR-CLOSEOUT-v1.md:153` requires a clean pinned source identity and an approved workflow path. Rev 6 names this itself as a "HOLD PREREQUISITE" (plan `:421-426, 445-446`) — honest, but it does not satisfy it | verified: `git remote -v` empty; no LICENSE; `.github/workflows/anvil-i10-dense-frontier.yml` and `…-g5-paged-dictionary.yml` untracked | coordinator: pin a clean SHA, open/point a remote, add a license, track and approve the workflow path |
| **B2** | **No byte-equality fidelity gate, and no precedence.** Contrasts are computed before retained byte fidelity is assigned (`q2_factorial.py:212` vs `:604`); every fidelity path is `decisional: False` (`:405,419,431,437`, `:442-444`); plan `:286-288, 498` | as left | add **G-S** (§12.1 point 2): fidelity first, stratum-A byte-identity required, divergence ⇒ contrasts withheld + `BASELINE-VOID (build drift)` |
| **B3** | **R1 reference identity not pinned.** "frozen tolerance" on a total (plan `:496, 432`); no per-file check in code; `libbrotli-dev` unversioned (`anvil-q2-parity.DRAFT.yml:161-162`); R1 lgwin 30 vs retained lgwin 22 (`tools/bench_native.cpp:39`); xz bar still absent | as left | either pin the producer and drop the byte claim, or compare per-file against a re-derived lgwin-30 reference and stop calling it the retained grid |
| **B4** | **Parallel capacity still evaluated only at N=1.** `decode_units` is symbolic but called with the default (`q2_arms.py:369`, `:74`); plan `:337` | as left | emit `capacity_at_policy = min(N, block_count)` declared per file + `throughput_estimate: null` (§12.1 point 5) |
| **B5** | **Frontier token emitted outside scanner coverage.** `anvil-q2-parity.DRAFT.yml:320` puts `FRONT-GAP` in the step summary; the summary is not in the scan list (`:298-307`) | as left | reword to "5 retained configurations"; add the summary to the scan set |

## 12.3 Carried minors and doc-level residue (do not block)

| id | item | where |
|---|---|---|
| D13 | G-L still counts "four `{synthetic}` cells"; the frozen 13-file suite has **3** (`synth-jitter`, `synth-timeseries`, `synth-arith`) | plan `:502` |
| R3-3 | Stale revision residue: title still says "byte-only decision token" (`:1`); §11 precondition 1 authorises "this **Rev 2**" (`:430`) and precondition 2 says "the *interaction token still stands*" (`:434`) — a deleted token; §6 "Rev 2 removes it" (`:323`); §3.2 header "Removed in Rev 3" (`:182`) | plan |
| R3-4b | §11 cost still reads "4 candidate cells × 13 files" (`:448`), ignoring the 5-config multiplier. Real shape: 5 × 4 × 13 = **260** candidate cells + 26 reference | plan `:448` |
| — | Doc-level residue only, **not** emission: §9 B2 (`:374`) still says the predicate "is confined to the G0 baseline diagnostic"; §9 B4 (`:376`) still describes `BASELINE-REPRODUCED`/`BASELINE-NOT-REPRODUCED`; §14 (`:530`) still cites `DEGENERATE_RATIO = 0.95` as a surviving constant in a plan that runs no predicate; §15 (`:558-560`) still claims the baseline diagnostic "reproduces Q0's frozen `435 / 28 / 5 / 0` tuple exactly". The code correctly runs none of these. Documentation drift only — no artifact emits frontier vocabulary (point 1 passes apart from B5) | plan |
| — | Prototype header cites the wrong revision: `q2_factorial.py:6` says "Design authority … (Rev 5)" against a Rev 6 plan | `q2_factorial.py:6` |
| D14 | `+13.74 %` still uncited from a ~33 MB Silesia census, though now marked "read by no code path" | plan `:406` |
| D17 | No rev-1 raw fallback: `--negate=off` arms can emit blocks larger than input (`src/anvil.cpp:4685` is the only raw path for `parse != "ratio"`) | plan |
| D18 | `docs/swarm-2026-10-02/` is untracked — neither plan nor this adjudication is under version control | verified |
| — | Informative/control subsets (7 / 2 / 4) still not predeclared; `CLOSEOUT-MATRIX-REDTEAM.md:229` requires it. Matters because `I ≡ 0` **by arithmetic** on the 4 single-block files, which must never be read as evidence | plan |

## 12.4 Retirements still owed by the coordinator (other documents)

Unchanged from §9 and not owned by this lane: **D9** `CLOSEOUT-MATRIX-REDTEAM.md:501-505, 578`
("pure window" label plus a STOP gate that would abort a healthy run on `random.bin`); **D10**
`Q2-PARITY-PLAN-CRITIC.md:897, 990` (G8 bars `FRONT-GAP` outright, which would make reproducing Q0's
`5` impossible — over-broad against the ruling's actual text); **D11** `:863` (`I = −(G0 − G3)`; the
correct value is `I = 0`); **D12** `CLOSEOUT-MATRIX-REDTEAM.md:463, 573` ("≈8/13 single-block" is wrong —
4/13 single-block, 6/13 at ≤ 2). **D19** stands: the successor timing remeasurement for the 5 retained
`FRONT-GAP` cells is still **unowned**, and per the ruling Q2 cannot fix it.

## 12.5 Final verdict

> ## `HOLD`
>
> **Rev 6 is a real improvement and three of the six checks now pass.** The frontier predicate is gone in
> fact, not just in prose — no `DEGENERATE_RATIO`, no `eps`, no bracket test anywhere in the prototypes —
> and the scanner covers all five class tokens across eight emitted artifacts. **Point 4 is resolved**:
> the plan mints no label, no void trigger, and no authorisation from any published number, and the code
> carries `dispatch_authorized: False` / `promotion_authorized: False`. **Point 6 is resolved well**:
> build outputs are a declared stratum B, never hash-asserted against historical Windows objects, with a
> `pooling_warning` and a fail-closed 13-file population derived from the frozen CSV rather than the
> 24-entry `CHECKSUMS.txt`.
>
> **Three checks still fail, and they are the ones that decide dispatchability.**
> **Point 2 — the byte-equality fidelity gate does not exist.** The *computation* is right
> (`q2_factorial.py:397-446`), but contrasts are assembled before it is assigned (`:212` vs `:604`) and
> every path is `decisional: False`, so the gate the ruling requires cannot stop anything. **G-S** is the
> exact missing gate: fidelity first, stratum-A byte-identity required, divergence ⇒ contrasts withheld
> and `BASELINE-VOID (build drift)`. **Point 3 — R1's identity is recorded, not pinned**: `--lgwin` is now
> mandatory and container-recorded, but G-F still says "frozen tolerance" on a total, no per-file check
> exists in code, `libbrotli-dev` is installed **unversioned**, and R1 at lgwin 30 cannot byte-match a
> retained grid produced at lgwin 22. **Point 5 — parallel capacity is still computed at `N = 1`**: the
> symbolic `min(decode_threads, segments)` exists and is called only with its default, so every emitted
> value is `1`.
>
> **Point 1 passes except for one path.** `anvil-q2-parity.DRAFT.yml:320` writes `FRONT-GAP` into the
> Actions step summary, and the step summary is not among the eight artifacts the scanner reads. That is
> the only surviving route by which Q2 emits the vocabulary it forbids.
>
> **Not `DISPATCHABLE`** because of B1–B5. **Not `CANCEL`** because the premise is live and cheap: Q0
> ruled Q2 REQUIRED, the critic ruled "do not cancel", the informative subset is 7 files so the red-team's
> own cancel condition does not fire, and Rev 6 now handles its two weakest prior liabilities — self
> authorization and build-output identity — correctly.
>
> **Path to dispatchable is four bounded edits and one coordinator action:** add **G-S** (B2); pin or
> re-scope the reference producer (B3); emit `capacity_at_policy` symbolically (B4); reword the step
> summary (B5); and the coordinator supplies the clean pinned identity, remote, license, and approved
> workflow path (B1). Nothing in the remaining list requires a new experiment, a new arm, or a new
> measurement scope.
>
> **Unchanged and outliving this ruling:** the 5 retained `FRONT-GAP` cells remain unresolved and
> **unowned**, held out of `DOMINATED` by a 7.8 % encode margin inside the measured 5.5–35.5 % dispersion
> band, on rows that predate the threading plumbing. No byte-primary job can move that axis, and Q2 does
> not attempt it.

---

## 13. Final bounded verification of the four coordinator edits

Static reading only. No codec, no network, no run.

| fix | verdict | evidence in current bytes |
|---|---|---|
| **G-S retained byte fidelity before contrasts, withholding on failure** | **VERIFIED** | `retained_byte_fidelity` is now `decisional: True`, scoped to **stratum-A tracked rows only** — `build-output-under-test` skipped (`q2_factorial.py:416-417`); `passed = compared > 0 and differing == 0` (`:446`); `BASELINE-VOID (build drift)` (`:447-448`); **fail-closed** when no suite is supplied (`:409-410`). In `main()` fidelity is computed at `:614`, **before** `compute_contrasts` at `:631`; failure writes `"contrasts_withheld": True` and `return 2` (`:615-629`). Plan §5.1 (`:283-288`), precondition 2 (`:433-434`), gate **G-S** (`:499`) |
| **symbolic `min(N, block_count)` + measured N=1** | **VERIFIED** | `structural_capacity` emitted per cell with `"symbolic": "min(N, nblocks=S) / block_count=S"`, `"evaluated_at_pinned_N"`, and the note that the **symbolic form is authoritative** and the N=1 value "must not be read as the structural capacity of the geometry"; `classifying: False`, `timing_implied: False` (`q2_arms.py:401-415`); same treatment for the reference arms (`:474-489`) |
| **Actions summary emits no frontier vocabulary** | **VERIFIED** | Token counts across the whole workflow draft: `FRONT-CROSSING` 0, `FRONT-GAP` 0, `BASELINE-CROSSING-CANDIDATE` 0, `DOMINATED` 0, `DEGENERATE` 0. Summary line `:320` now reads "5 retained **Class-A** configs" |
| **R1/R3 descriptive-only, producer unpinned, no frozen-equality claim** | **VERIFIED** | `reference_producer_pinned: False` with a same-job descriptive note (`q2_factorial.py:373-374`); `descriptive_only_reference_residual` (`:214`); `withheld_while_unpinned` (`q2_arms.py:776`); plan `:110-113` (descriptive same-job controls), `:272` (marker explicitly **not** used as validity), `:497` (G-F reduced to a labelling statement with **no** validity dependency) |

**Two non-blocking notes, recorded so they are not reintroduced.**

1. `max_parallel_decode_units` survives as a *declared* name in `NON_CLASSIFYING_AXES`
   (`q2_arms.py:129`) and the row field passthrough (`q2_factorial.py:191`) but is no longer emitted by
   the arm builder (`q2_arms.py:399-417`). It is a dead name, not a value — nobody should re-add an
   N=1 number under that key now that `structural_capacity` is authoritative.
2. **G-S is a hard stop on first run, and that is correct.** Four drift commits exist on the local
   branch, so `BASELINE-VOID (build drift)` is a genuinely likely first outcome. It is mitigated but not
   eliminated by **G-O**, which asserts six production blobs equal the coordinator-validated public base
   before build and before any codec invocation (`anvil-q2-parity.DRAFT.yml:121-126`, incl.
   `src/anvil.cpp` `755df76a…`, `CMakeLists.txt` `449ae00e…`). If G-O holds, `src/anvil.cpp` is
   byte-identical to the validated base and any Class-A byte divergence would implicate toolchain or
   corpus rather than codec drift. Whether public base `564e2cd…` reproduces the frozen
   `bdc90474` Class-A bytes is the one genuinely unknown, and G-S is the correct instrument to answer
   it. A void on the first run is a **finding**, not a defect.

### 13.1 Status

**Only external B1 remains.** No harness blocker survives.

**`HOLD`** — and the hold is now entirely external and coordinator-owned: a tracked, approved workflow
path plus a clean pinned source identity with a remote and a license
(`COORDINATOR-CLOSEOUT-v1.md:153`; verified `git remote -v` empty, no LICENSE, the two
`anvil-i10-*` workflows untracked). Rev 6/7 satisfies B2, B3, B4 and B5 in code and in the plan.

B2–B5 and §12.3's doc-level residue are **closed or demoted**. **D19** is unchanged and remains outside
Q2: the successor timing remeasurement for the 5 retained `FRONT-GAP` cells is still unowned.

*Worker closed.*
