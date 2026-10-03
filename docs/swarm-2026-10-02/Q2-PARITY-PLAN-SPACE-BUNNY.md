# Q2 — Block/window geometry parity: FINAL DESIGN (2×2 factorial, byte-only decision token)

**Agent:** Space Bunny Free · **Date:** 2026-10-02 · **Rev 7** — final adjudication fixes. Supersedes
Rev 6. Rev 7 changes **no algebra** and adds **no measurement scope**. It: (a) adds **G-S**, a
fail-closed retained-twin **preflight** run as a true first workflow stage on **G0 / stratum A only**, so
a drifted substrate is caught before any geometry arm is spent and the contrasts are **withheld** on
mismatch; (b) removes the last frontier-vocabulary leaks and scans every emitted surface; (c) emits
structural capacity **symbolically** as `min(N, nblocks) / block_count` rather than only at
`decode_threads=1`; (d) fixes reference identity honestly — R1/R3 are a **same-run matched
geometry-control pair**, R1 is **not** a reproduction of the retained rows (those used
`BROTLI_DEFAULT_WINDOW` = lgwin 22 against R1's explicit lgwin 30), and the Brotli producer is a
**HARD HOLD** until pinned end-to-end rather than weakened to an unversioned `libbrotli-dev`;
(e) **no self-authorization**. Rev 6's manual-only doctrine (`workflow_dispatch`, no push trigger) and
all Rev 5 items stand.
**Owners per queue:** Tracks 04 + 15 · **Class:** `adopt-class {engineering}` — **measurement parity only**
**Dispatch status:** **NOT DISPATCHED.** Verdict in §11: *dispatchable as a byte-only parity-geometry
census; explicitly NOT dispatchable as a classification, configuration-change, or novelty job.*

No local compression, sweep, benchmark, timing, or fuzz run was performed. No codec was invoked. No
commit, push, reset, clean, stash, restore, or rebase. **No coordinator file was created, edited, or
deleted.** `.github/` is untouched; the workflow draft lives in the isolated prototype directory.
Only new files under `docs/swarm-2026-10-02/` and `prototypes/swarm-2026-10-02/q2-parity/` were written.

---

## 0. Purpose, restated to match the coordinator's scope

Q2 exists to answer one measurement question:

> **How much do complete bytes move when candidate block/window geometry changes, and is that movement
> confounded by ANVIL's per-block incompressibility gate?**

It is **not** a tuning experiment and **not** a novelty experiment. It is a **measurement-parity**
instrument. Per the coordinator correction:

* **Q2 MUST NOT emit `FRONT-GAP` or `FRONT-CROSSING`** — or `DOMINATED`, `DEGENERATE`,
  `BASELINE-CROSSING-CANDIDATE`, or any other frontier class. The frontier predicate is **not run at
  all**: no dominance test, no bracket test, no class of any kind. `q2_factorial.py
  --assert-no-frontier-vocabulary` fails the job if any frontier token appears in any artifact.
* The retained baseline check is **byte and identity fidelity only** — does this run's G0 geometry
  reproduce the retained Class-A byte counts, and does the retained twin structure survive?
* Q2's **decision token is deterministic**: exact signed geometry-sensitivity / byte-movement
  contrasts. Timing, parallelism and the reference codecs are **descriptive and non-classifying**.
* **No post-hoc epsilon tuning.** Q2 has no epsilon at all.

---

## 1. The arm set — 5 retained configurations × 4 cells = 20 candidate cells, + 2 reference arms

### 1.1 Candidate factorial — a **complete, crossed 2×2** in geometry × incompressibility gate

| cell | geometry | `--negate=` | identity |
|---|---|---|---|
| **G0** | small (262,144 B) | `on` | the frozen Class-A geometry; the anchor cell |
| **G1** | large (67,108,864 B) | `on` | parity end point |
| **G2** | large (67,108,864 B) | `off` | factorial cell |
| **G3** | small (262,144 B) | `off` | factorial cell |

Because the design is **complete and crossed**, every main effect holds the other factor fixed
*exactly* — see §2. No contrast is confounded.

* `small` = `262144` = ANVIL's default block size, never assigned by the bench harness
  (`src/anvil.cpp:3769`; `tools/bench_native.cpp:24-30`). So **G0 is the frozen Class-A geometry.**
* `large` = `67108864` = the revision-1 block cap (`src/anvil.cpp:5002`). Every Class-A file is
  ≤ 2,815,267 B, so `large` is **exactly one block** for all 13 files.
* `--negate` is the **difference-cover negative gate for incompressible blocks**, default `true`
  (`src/anvil.cpp:3778`), applied **per block** at `src/anvil.cpp:4685`
  (`if(opt.negate && probe_incompressible(block))`). CLI: `--negate=on|off` (`:4971`).

**The 2×2 is run independently within EACH of the five retained FRONT-GAP configurations, with
their exact original settings. Nothing is collapsed onto the harness baseline.**

| `config_id` | retained row | bench site | exact original argv (config portion) |
|---|---|---|---|
| `mdl-l0.04` | `anvil-mdl-rans` | `tools/bench_native.cpp:70` | `--parse=mdl --literal=o0 --entropy=rans --shape-states=28 --stream-lambda=0.04` |
| `mdl-l0.00` | `anvil-mdl-rans-l0` | `:71` | `--parse=mdl --literal=o0 --entropy=rans --shape-states=28 --stream-lambda=0` |
| `mdl-l0.01` | `anvil-mdl-rans-l001` | `:72` | `--parse=mdl --literal=o0 --entropy=rans --shape-states=28 --stream-lambda=0.01` |
| `shape-l0.04` | `anvil-shape-rans` | `:73` | `--parse=shape --literal=o0 --entropy=rans --shape-states=28 --stream-lambda=0.04` |
| `shape-l0.00` | `anvil-shape-rans-l0` | `:74` | `--parse=shape --literal=o0 --entropy=rans --shape-states=28 --stream-lambda=0` |

> **Provenance catch (load-bearing).** The retained rows are produced by
> `tools/bench_native.cpp`, which sets `anvil::g_stream_lambda` **directly** (`:24`, `:30`) rather than
> through the CLI. The **harness baseline is 0.04**; the **CLI default is 0.01**
> (`src/anvil.cpp:3783`, parsed at `:4977`). The retained FRONT-GAP set spans
> `stream_lambda ∈ {0.04, 0.00, 0.01}` across {mdl, shape}. Each value is therefore passed
> **explicitly and separately**; `q2_arms.py` asserts the span `{0.04, 0.0, 0.01}` and refuses a
> duplicate `(parse, lambda)` pair (gate **G-A**, collapse check).

**Machine-readable join key.** `q2_arms.py --emit-retained-map` emits
`retained_config_id → argv`, where `retained_config_id` is the row name in the retained Class-A grid,
plus the originating `bench_call`, the bench source line, and `argv_template` (config argv plus the
two experimental factors and the pinned transport options). This is the join between the frozen CSV
and any Q2 cell.

**Environment identity (gate G-N).** `src/anvil.cpp:4959` reads `ANVIL_STREAM_LAMBDA` into
`opt.stream_lambda`; the explicit `--stream-lambda=` flag is parsed afterwards (`:4977`) and
`src/anvil.cpp:4662` propagates it into the `g_stream_lambda` global (`:1471`) that actually weights
the stream suite. The flag therefore wins, but Q2 does **not** rely on that ordering: the variable
must be **absent**, `q2_collect.py` strips it from the child environment, and the draft workflow
asserts it is unset before building anything. Arm identity is argv-determined.

Pinned on **every** arm of **every** cell: `--decode-threads=1` (`src/anvil.cpp:3796`) and
`--bwt-aux=off` (`src/anvil.cpp:3793`), plus the bench-time defaults the harness leaves unset
(`--boundary=off --channels=off --pnra=off --hotop-rlzp=off --hotop-budget=off --stream-suite=on
--chain=48 --max-match=65535 --surprise=12`), so a future default change cannot silently alter
identity.
> Every candidate arm passes it explicitly. Gate **G-A**.

### 1.2 Reference endpoints (2 arms, not 3)

| arm | streams | `lgwin` | isolates |
|---|---:|---:|---|
| **R1** | 1, whole file | 30 | descriptive same-job Brotli whole-file control |
| **R3** | `ceil(n/262144)`, independent | 30 | descriptive same-job 256 KiB fragmentation control |

`D_ref = Σpayloads(R3) − bytes(R1)` is a **same-job descriptive Brotli fragmentation delta**.
It is not a frozen-grid bar and it is not part of Q2 validity until the Brotli producer itself is
version-pinned. The candidate 2×2 contrasts do not depend on it.

**Why `R2` (the `lgwin=18` reach arm) was dropped.** It priced *reach vs restart* separately — a real
decomposition (Track 15 §11.4, prediction P2) but a **different question** from measurement parity. Under
a 6-arm cap with 4 cells reserved for the mandated factorial, the reach/restart split is the droppable
one: `D_ref` needs only `R3 − R1`. **Deferred, not cancelled.** Re-entry: any later run that needs to
attribute the reference tax to a component rather than merely subtract it.

---

## 2. The frozen 2×2 contrast algebra (coordinator-specified, implemented verbatim)

For each file, on **complete bytes**, signed as `treated − control`, negative = smaller:

| contrast | factor varied | factor held fixed | reading |
|---|---|---|---|
| geometry effect, gate **ON** | geometry | gate | `G1 − G0`: bytes change with block size while the gate stays ON |
| geometry effect, gate **OFF** | geometry | gate | `G2 − G3`: bytes change with block size while the gate stays OFF |
| **gate effect at SMALL** | gate | geometry | `G0 − G3`: bytes change with the gate while block size stays 262,144 B |
| **gate effect at LARGE** | gate | geometry | `G1 − G2`: bytes change with the gate while block size stays 67,108,864 B |
| **interaction** | — | — | `(G1 − G2) − (G0 − G3)`: does the **geometry effect depend on gate state**? |
| identity (asserted in code) | — | — | `interaction == (G1 − G0) − (G2 − G3)` |

**No contrast is confounded.** The design is a complete, crossed 2×2, so each main effect pins the
other factor *exactly*: within `G0 − G3` and within `G1 − G2` the block size — and therefore the
block count — is identical by construction; within `G1 − G0` and within `G2 − G3` the gate state is
identical by construction.

**Naming discipline — explicit and binding.** `G0 − G3` and `G1 − G2` are **gate** effects. The factor
they vary is the incompressibility router (`src/anvil.cpp:4685`), not block geometry. They are the same
factor estimated at two levels of geometry, and **their difference is the interaction**.

**Rejected claim, recorded so it cannot be reintroduced.** Any assertion that `G0 − G3` is a *"pure
window effect"* — or any window, reach, or geometry effect — is **rejected on the design's face**:
`G0` and `G3` share an identical block size, so that contrast does not vary geometry at all. Equally
rejected is the converse error, that only the interaction holds a factor fixed: `G1 − G0` and
`G2 − G3` each already hold the gate fixed.

Implemented in `q2_arms.py::CONTRASTS`, computed in `q2_factorial.py::contrasts_for_group`, and
verified symbolically by `q2_arms.py --verify-identity` (gate **G-J**).

---

## 3. What is published — exact signed contrasts and relative percentages, **no threshold**

### 3.1 Published quantities (the complete decision surface)

Per (configuration, file), on **complete bytes**, exact integers, plus the same quantity as a signed
percentage of the **declared denominator** `bytes(G0)` of the same configuration and file:

```text
geom_effect_gate_on    = G1 - G0        and  100*(G1-G0)/bytes(G0)
geom_effect_gate_off   = G2 - G3        and  100*(G2-G3)/bytes(G0)
gate_effect_at_small   = G0 - G3        and  100*(G0-G3)/bytes(G0)
gate_effect_at_large   = G1 - G2        and  100*(G1-G2)/bytes(G0)
interaction            = (G1-G2)-(G0-G3) == (G1-G0)-(G2-G3)
                                        and  100*interaction/bytes(G0)
```

Corpus level: the exact signed **sum** over every complete (configuration, file) group, and that sum
as a percentage of the summed `bytes(G0)`. No weighting, no averaging of signs, no majority vote.

Each published integer also carries a threshold-free **sign label** (`NEGATIVE` / `ZERO` /
`POSITIVE`), which is a restatement of the sign of a number already published and adds no cut value.

**Token:** `PARITY-GEOMETRY-REPORTED`. That is the whole vocabulary. It asserts that the contrasts
were measured and published; it does not grade them.

### 3.2 Removed in Rev 3 — no materiality threshold is invented or applied

Rev 2 proposed a physical "interaction floor" of `11 × block_count(G0)` bytes and a two-value
vocabulary (`GEOM-GATE-INVARIANT` / `GEOM-GATE-CONFOUNDED`) derived from it. **That is withdrawn.**
Introducing a cut value and then emitting a label from it is exactly the move that manufactures a
verdict after seeing data, and no such value is derivable from the codec or the corpus.

Consequently:

* `materiality_threshold: null` is carried in every manifest and every analysis output;
* `q2_factorial.py` contains **no threshold and no comparison against one**;
* the **11 B/block** framing quantum (`FORMAT.md:25-33`) survives only as a **descriptive
  annotation** of what geometry does to block count — `framing_bytes_descriptive` — and is never
  compared against anything;
* the ≥ 0.05 % aggregate noise floor (`11-front-crossing-criteria.md:62`) is **not** used;
* **no epsilon.** There is no `eps` parameter anywhere in Q2, and no bracket predicate to tune.

### 3.3 What the published numbers do and do not authorise

**Nothing.** The reader — the coordinator — draws every conclusion. Q2's entire contribution is that
the geometry effect, the gate effect, and their interaction are available per configuration as
**exact, unthresholded** integers with relative percentages, with the retained configuration identity
preserved so the numbers join back to the frozen grid. Q2 mints **no label**, **no void trigger**, and
**no classification** from any published number.

### 3.4 The output classes

| class | contents |
|---|---|
| **raw exact contrasts** | the four contrasts + the interaction, per (configuration, file) **and as role-stratified exact totals** (§3.6), in exact bytes, plus relative percentages against `bytes(G0)` and threshold-free sign labels |
| **round-trip / provenance validity** | `roundtrip_verified` by byte-compare, `complete_bytes`, `compressed_sha256`, `roundtrip_sha256` vs `source_sha256`, `evidence_role`, `identity_role`, the exact `argv_encode` per cell |
| **retained-twin fidelity** | whether the retained byte-identity of the twin configurations survives at each geometry (§3.5) |
| **retained byte fidelity** | whether this run's G0 geometry reproduces the retained Class-A byte counts per (configuration, file) — **bytes only**, `decisional: false` |

### 3.5 Retained-twin fidelity — recorded, not enforced

The retained Class-A grid has **exactly two distinct byte values across all five configurations, on
every one of the 13 files**: the three `anvil-mdl-rans*` rows are byte-identical to each other, and the
two `anvil-shape-rans*` rows are byte-identical to each other (on the two store-class files all five
coincide). That structure is declared in `q2_arms.py::RETAINED_TWIN_GROUPS` and measured per cell by
`q2_factorial.py::retained_twin_fidelity`.

Fidelity is **recorded with `decisional: false`**. A divergence at some geometry would mean the
retained identity does not survive that factor — a genuine observation about configuration identity.
It does **not** invalidate the run, void a contrast, trigger a label, or license a conclusion. It is
exactly the kind of reading the coordinator makes after measurement.

### 3.6 Role-stratified totals — Linux build outputs are never pooled as retained bytes

`corpus_totals` is **stratified**, never a single pooled number:

| stratum | contents | standing |
|---|---|---|
| **A — tracked Q0-compatible** | the 11 git-tracked files that are byte-identical to the retained Class-A objects | comparable to the retained grid |
| **B — build-output-under-test** | `anvil.exe`, `anvil_bench.exe` | on a hosted Linux run these are **fresh build artifacts**, *not* the historical Windows objects the retained grid measured. Recorded, never cited as retained bytes |
| **C — all 13, mixed** | A + B pooled | **explicitly mixed discovery aggregate.** Not comparable to any retained-grid total |

Every **per-file** contrast is published unchanged regardless of stratum, so nothing is lost by the
stratification and nothing is hidden inside an aggregate. `pooling_warning` is emitted alongside.

### 3.7 The 13-file population is derived from the frozen CSV, not from CHECKSUMS

`tests/corpus/CHECKSUMS.txt` has grown to **24** entries, and the public mirror intentionally omits
the later PE/synth expansion. Loading all 24 would silently change the population and pool objects
the retained grid never measured. Therefore:

* **authority** = the frozen Class-A CSV, which contains exactly **13** files
  (`q2_arms.py::frozen_population`);
* `CHECKSUMS.txt` supplies **identities only** (`--plan` is required to read it);
* the derivation is **fail-closed** (`q2_arms.py --population-report`, gate **G-R**): exactly 13 names,
  equal to the declared `Q0_FROZEN_FILES`, every name present in `CHECKSUMS.txt`, and every byte count
  agreeing with the frozen `input_bytes`;
* the two build-output slots are labelled `identity_role = build-output-under-test` and are **never**
  asserted against the historical Windows hashes.

---

## 4. Descriptive secondary readout — reference residual (byte-only, non-classifying)

```text
D_anvil      = bytes(G0) - bytes(G1)          # candidate geometry movement, gate on
D_ref        = SUM(payloads R3) - bytes(R1)   # generic geometry tax, framing-free
framing_ref  = bytes(R3_container) - SUM(payloads R3)   # our container's own cost, declared
parity_share = (D_anvil - D_ref) / D_anvil   # descriptive only while reference producer is unpinned
```

`framing_ref` is separated because it is **our artifact, not the codec's**. ANVIL's per-block framing is
11 B/block; the `R3` container's overhead is declared so the two taxes are compared on the same footing
rather than importing a second invisible constant. `parity_share` is reported with an explicit
`reference_producer_pinned: false` marker until the Brotli package/source is frozen. It is **not**
thresholded, is **not** a gate, and cannot license a parity conclusion.

---

## 5. Retained fidelity — bytes and identity only (no predicate replay)

Q2 **does not run the frontier predicate.** No dominance test, no bracket test, no classification
order, and no frontier vocabulary of any kind. What remains is an **infrastructure provenance**
precondition plus one non-decisional identity diagnostic:

**5.1 Retained byte fidelity — G-S, blocking.** Before any geometry contrast is computed or published,
compare each stratum-A tracked `G0` row against the retained Class-A `compressed_bytes` for the
matching row (`anvil-mdl-rans`, `anvil-mdl-rans-l0`, `anvil-mdl-rans-l001`,
`anvil-shape-rans`, `anvil-shape-rans-l0`). The two build-output-under-test slots are excluded
because their Linux bytes are not the retained Windows objects. Any missing or byte-different tracked
row emits **`BASELINE-VOID (build drift)`**, withholds every contrast, and exits non-zero. This is an
infrastructure gate, not a frontier class.

**5.2 Retained twin fidelity.** §3.5 — whether the retained byte-identity of the twin configurations
survives at each geometry.

**5.3 Enforcement.** `q2_factorial.py --assert-no-frontier-vocabulary` scans every emitted artifact for
`FRONT-CROSSING`, `FRONT-GAP`, `BASELINE-CROSSING-CANDIDATE`, `DOMINATED` and `DEGENERATE`, and exits
non-zero if any is found. Lines that *declare* the prohibition are exempt so the gate cannot fail on
its own constant.

> **Withdrawn in Rev 5.** Rev 4 ran the adopted predicate over the retained CSV as a "baseline
> diagnostic" against Q0's `435 / 28 / 5 / 0` tuple. Even though that replay was correctly labelled
> non-decisional, it emitted frontier vocabulary, so it is removed entirely. Q2's contribution to the
> predicate question is **none**, and that is the intended answer: the parity question is a **byte**
> question.

---

## 6. Descriptive-only measurements (never classify)

| quantity | how | label |
|---|---|---|
| in-process encode/decode MB/s | drop `--quiet`; parse `ANVIL c/d … MB/s=` from stderr — `steady_clock` wraps `compress`/`decompress` and `write_file` is **outside** the timed region (`src/anvil.cpp:5009`, `:5023-5024`) | `descriptive`, `in-process`, same *kind* as `bench_native.cpp:32-33` |
| **router counters** | same stderr line: `blocks=`, `compressed_blocks=`, `raw_blocks=` | `descriptive` — a **direct** per-block measurement of the gate firing, complementing the factorial interaction |
| peak RSS | `/usr/bin/time -v` → `Maximum resident set size`, decode process, output to `/dev/shm` | `descriptive`; scope-matched across arms (identical mechanism, single process) |
| **segment parallelism** | `segment_count = block_count`; `max_parallel_units = min(decode_threads, segment_count)` | `descriptive`, `non-classifying` — see §7 |
| decoder/binary size | recorded | `descriptive_only`, **`scope_comparable: false`** |

**Hard comparability rule.** ANVIL's MB/s is in-process; the Brotli helper's is process-level
(including exec/dynlink). Therefore **reference MB/s is comparable to reference MB/s only** — never to
the frozen CSV's in-process values and never to ANVIL's. Reference speed is reported and then
**ignored** by every token in this job.

**No predeclared ≥ 7 % timing endpoint exists**, because no decision in this job consumes timing. Rev 1
manufactured one; Rev 2 removes it. This is the direct consequence of the coordinator's steer and it
removes the Fledge-queue trigger condition rather than satisfying it.

---

## 7. Block geometry ⇄ decoder segment parallelism (coupling, declared, non-classifying)

`src/anvil.cpp:3796` — `decode_threads = 1` by default ("1 = serial (zero behavioral change)");
`src/anvil.cpp:4908` — `nthreads = min(opt.decode_threads, segs.size())`.

Consequences, all recorded as declared geometry fields on every cell:

* `--decode-threads=1` is **pinned explicitly** on every arm (it is the default, but pinned so it cannot
  drift) and recorded in the arm identity.
* `segment_count = block_count`. The structural capacity is emitted symbolically as
  `capacity(N) = min(N, block_count)`, plus the special measurement-vehicle value at
  `decode_threads=1`. Q2 does **not** collapse the structural capacity claim to `N=1`.
* **The coupling is an anti-parity term that byte-only measurement cannot see, and it favours the
  shipped default.** At 256 KiB, ANVIL has up to 11 independently-decodable segments per Class-A file
  (`anvil.exe`-class files: 1; `generated.jsonl`: 11). The single-stream Brotli reference has 1. Larger
  blocks **remove** a parallelism opportunity the reference never had.
* Per Fledge 15 §13.6, `P_loss ≥ 0.95` / `R_delta ≤ 1.05` is a **hard gate on any parity result**, not a
  footnote. **Q2 does not evaluate it** (descriptive only, hosted runners cannot establish it at
  promotion grade) and therefore **cannot license a configuration change** — see §8.

---

## 8. Why Q2 cannot close the configuration question (stated up front)

The tempting conclusion from a byte win is "raise the default block size." Three independent blockers,
each sufficient on its own:

1. **The interaction is unbounded and ungraded.** Q2 publishes its exact value and applies no cut to
   it. A non-zero interaction is a **measured geometry × gate interaction** — a result, not a defect,
   and never an invalidation.
2. **The decode-parallelism gate is unevaluated.** A byte win that costs > 5 % decode throughput is a
   Pareto regression (Fledge 15 §13.6), and Q2 measures no classification-grade throughput.
3. **A configuration change is `{engineering}`** and, per queue NQ-8, may authorise exactly one thing:
   an integration/validation run. It is never a contribution.

So Q2's admissible conclusions are exactly: **the raw exact contrasts, the round-trip/provenance
validity record, and retained-twin fidelity** (§3.4). Nothing else. A follow-up job may read those
numbers and propose a configuration change with a promotion-grade decode gate attached — that job is
not Q2 and is not authorised here.

---

## 9. Adversarial reconciliation — six items, explicitly resolved

| id | critic concern | resolution | where |
|---|---|---|---|
| **B1** | `--parse=ratio` is **rev 2** and wraps a *backend* (Brotli by default, `src/anvil.cpp:3790`, registry `:4400-4426`), not the LZ parser. A BWT block re-runs `libsais` per block and is penalised far harder by fragmentation than a compact rANS block, so **backend-2-vs-backend-1 is biased against backend 2 at 256 KiB** (Fledge 15 §13.6.1). Track 15's `AN-OVR` null and its gate "bytes(A3) == bytes(A1)" are therefore **not implementable as stated**. | **All rev-2 excluded.** Zero `--parse=ratio`, zero `--ratio-backend`, zero BWT arms. Asserted mechanically (gate **G-K**). Fledge §13.6.3 item 4 (quarantine backend-2-vs-backend-1 conclusions) is adopted and recorded as a live constraint on other lanes. | §1.1, §13 G-K |
| **B2** | The adopted predicate is **two-axis** (`ratio` *and* MB/s), so it is not computable from bytes; yet no hosted timing cell may create a crossing. Apparent contradiction. | Resolved by **separating the two jobs of the predicate**. The **ratio axis decides bracket membership** (`q_lo.ratio ≤ p.ratio ≤ q_hi.ratio`), which is byte-decidable; the **speed axis decides DOMINATED-vs-CROSSING** among cells that survive the bracket. So byte evidence is **sufficient to refute** a FRONT-GAP bracket and **insufficient to establish** a crossing. Q2 therefore needs no crossing token at all — it reports bracket-relevant byte movement and stops. The frozen predicate is confined to the G0 baseline diagnostic with a capped vocabulary. | §0, §5, `q2_factorial.py` |
| **B3** | The five retained FRONT-GAP cells are **encode-plane** rows (all on `generated.json`; on the decode plane each is dominated by `brotli-q11`), and hosted-runner timing is noisy — process overhead is **14 → >100 % of the decode-plane signal** and material on the encode plane for fast arms (Fledge 04 §7.2, F4). | Q2 **does not re-adjudicate the FRONT-GAP cells.** Their decode-plane dominance is unaffected by block geometry in any case (dominated by `brotli-q11` on the frozen grid), so the cells are encode-plane-only and are exactly where hosted noise is least defensible. Q2 reports the byte movement on `generated.json` and **does not touch its class**. Timing is descriptive, in-process, and non-classifying. | §0, §6 |
| **B4** | **Source drift** since the frozen CSV (`bdc90474`): the live tree is `i10-aux-unbwt`/`b8eae11` and dirty; rev-2/aux landed; the CLI `stream_lambda` default moved to 0.01. | **Resolved fail-closed for the candidate path.** Gate **G-S** requires byte-identical tracked `G0` rows *before* any contrast is computed; divergence emits `BASELINE-VOID (build drift)` and withholds geometry output. Same-job R1/R3 are descriptive only and never splice an unpinned Brotli producer into candidate validity. `source_tree_ref` + `evaluated_at` remain recorded; `source_dirty` is asserted about the pinned checkout, never the operator worktree. | §5, §13 |
| **B5** | Block geometry is coupled to decoder segment parallelism (`src/anvil.cpp:4908`); parity may improve bytes while destroying decode throughput. The aux sampling rate is a **second** geometry dimension (Fledge queue §C2.1.2). | **Parallelism:** `--decode-threads=1` pinned; `segment_count` and `max_parallel_units` declared per cell; the coupling is recorded as an unevaluated anti-parity term and is the reason Q2 cannot license a configuration change (§8). **Aux:** the rate is a **deterministic function of the outer block size** — `src/anvil.cpp:3913` `kBwtAuxTargetWalks = 1024`, `:4247-4256` `r = next_pow2(ceil(n/1024))`, computed from the block length at encode time. It therefore has **no independent dial**, it lives **only on the rev-2 BWT path**, and **excluding rev-2 (B1) pins it by construction**. One decision discharges both B1 and the aux dimension. Each arm records `aux_sampling_rate.policy = "absent"` plus `aux_rate_if_bwt_path` (declared, `not_applicable`), so a future rev-2 arm is visibly a different geometry. | §1.1, §7, §11 |
| **B6** | The incompressibility/negate gate is applied **per block** (`src/anvil.cpp:4685`), so changing block size changes **how many blocks** the router judges, and the 28 `DEGENERATE` cells (ratio ≥ 0.95, the two store-class files) are a **router outcome, not a geometry outcome**. | **The gate is a second factor of the design, not a hidden confound.** Because the 2×2 is complete and crossed, `G0 − G3` and `G1 − G2` estimate the gate effect with geometry held **exactly** fixed — same block size, same block count — so neither gate contrast is confounded by block count. `G1 − G0` and `G2 − G3` estimate the geometry effect with gate state held fixed. The **interaction** then answers the only genuinely coupled question: *does the geometry effect depend on gate state?* Its exact value is published unthresholded; Q2 does not grade it (§3.2). Two independent measurements support the reading: (i) the factorial contrasts; (ii) the **direct router counters** `compressed_blocks` / `raw_blocks` emitted per arm (`src/anvil.cpp:5011`). `G0 − G3` and `G1 − G2` are named **gate** effects and are **never** called window effects (§2). | §2, §3, §6 |

---

## 10. Pre-registered descriptive hypotheses (no triggers, no labels, no voids)

These are **hypotheses about what Q2 expects to observe**, frozen before measurement. They are
**descriptive only**. None is a validity gate, none triggers a label, none voids a contrast or the
experiment, and none is compared against a cut value. Interpretation belongs to the coordinator
after measurement.

| id | descriptive hypothesis | if the opposite is observed |
|---|---|---|
| **P1** | the interaction is **exactly zero** on every (configuration, file) group | a non-zero interaction is a **measured geometry × gate interaction**. It is a result, not a defect: it says the geometry effect depends on gate state. It does **not** invalidate the experiment and does **not** void any contrast. |
| **P2** | `D_anvil` and `D_ref` carry the **same sign** | opposite signs are a **descriptive divergence** between the candidate's geometry movement and the generic tax. It is **not** sufficient on its own to label the retained Class-A grid `CLASS-A-BLOCK-CONFOUNDED`; that labelling needs the coordinator's reading of the whole picture, not one sign. |
| **P3** | bytes are monotone non-increasing from `small` to `large` with the gate `on` | with only **two** geometry levels, a **larger-block byte increase is still a valid measured geometry effect**. It does **not** void the parity residual and does **not** invalidate the experiment; it is simply what the candidate did at that geometry. |
| **P4** | observed `G0` bytes reproduce the retained Class-A bytes for the same configuration and file | a difference means this run is its own measurement window relative to the retained grid — a provenance observation, reported with `decisional: false` |
| **P5** | `aux_rate_if_bwt_path` is `256` at 262,144 B (declared, never exercised) | declared-only; no arm exercises it, so it cannot fail — stated to prevent it being read as measured. |

**Removed in Rev 4.** Rev 3 turned P1-P3 into automatic triggers: an interaction floor produced a
`GEOM-GATE-CONFOUNDED` label and voided geometric conclusions; a sign or magnitude difference on the
parity residual labelled the retained grid `BLOCK-CONFOUNDED`; non-monotonicity voided the residual.
**All of that is withdrawn.** A measurement that comes out differently from a hypothesis is a
measurement, not an invalidation.

**Budgetary honesty (from Rev 1, retained because it sizes the opportunity, not the verdict).** On
`generated.json` (827,664 B) the frozen values are `anvil-mdl-rans` 89,589 B, `brotli-q11` 79,172 B,
`xz-9e` 76,952 B; beating Brotli q11 would need −11.63 % and beating the binding bar −14.10 %, against
a measured reference geometry tax of **+13.74 %**. So the prize sits inside the band. **This is context
for why parity is worth measuring. It is read by no code path, produces no label, and produces no
class.** Q2 reports byte movement; it does not adjudicate the bars.

---

## 11. Verdict

> **DISPATCHABLE** as a **byte-only, deterministic, within-run 2×2 parity-geometry census** —
> 5 retained configurations × 4 cells = 20 candidate cells, plus 2 descriptive reference arms —
> whose entire decision surface is the **exact signed contrast algebra** and its relative percentages.
>
> **NOT DISPATCHABLE** — and this job is structurally incapable of it — as a **classification** job, a
> **configuration-change** job, a **frontier** job, or a **novelty** job.

**HOLD PREREQUISITE (rev 6).** Even the measurement half is **not dispatchable yet**. The workflow is
`workflow_dispatch` only, per the repo's manual-only doctrine — no push trigger, not even branch-scoped.
A workflow file that exists only on a staging ref cannot be dispatched until its path is tracked and
approved on an approved remote/default branch. That is a **coordinator prerequisite**, and this document
does **not** claim dispatchability until it is met. The workflow draft is therefore written and reviewed
but **not installed**, and `.github/` remains untouched.

Preconditions, in order:

1. **Coordinator authorisation of this Rev 6** (2×2 algebra, no frontier vocabulary, non-classifying
   timing/parallelism). Nothing here is authorised by the document itself.
2. **G-S: retained candidate fidelity passes on every tracked stratum-A G0 row before contrasts.**
   Any difference is build drift and withholds all candidate geometry output. R1/R3 remain descriptive
   until their Brotli producer is independently version-pinned; their status cannot block or validate
   the core factorial.
3. **Coordinator ruling on the pinned commit, and on blob identity.** The checkout target is now the
   **workflow/staging commit itself** (`github.sha`), because local `b8eae11` does not exist on the
   sanitised mirror and checking out public base `564e2cd…` would delete the prototype directory.
   Correctness is therefore guaranteed by **fail-closed blob identity**, not by ref selection: five
   measurement-critical production blobs must match the coordinator-validated base blob ids, and the
   Q2 commit may add only prototype/docs/workflow files. Mismatch ⇒ **`INVALID_INFRA`** with **no codec
   invoked** (gates **G-O**, **G-P**).
4. **Confirmation that queue E1/E2/E4 bind**: `evidence_role = discovery`,
   `promotion_authorized: false`, dual aggregate emitted, no PASS variant.
5. **HOLD cleared**: the workflow path tracked and approved so `workflow_dispatch` can reach it. Until
   then this job is prepared, not dispatchable, and no auto-trigger is added in its place.

**Cost.** 4 candidate cells × 13 files + 2 reference arms × 13 files, encode+decode, largest file
2.8 MB. Dominated by R1/R3 (Brotli q11). Comfortably inside a 120-minute timeout. **No timing job.**

---

## 12. Frozen quantities and prohibitions, verbatim (for code review)

```text
[NO PREDICATE]
  Q2 runs no dominance test, no bracket test, and no classification order.
  No frontier vocabulary is computed or emitted anywhere, at any grade.

[Q2 REPORTED QUANTITIES, byte-only, no epsilon, no threshold]
  geom_effect_gate_on    = G1 - G0                      # geometry factor, gate HELD FIXED at ON
  geom_effect_gate_off   = G2 - G3                      # geometry factor, gate HELD FIXED at OFF
  gate_effect_at_small   = G0 - G3                      # gate factor,    geometry HELD FIXED small
  gate_effect_at_large   = G1 - G2                      # gate factor,    geometry HELD FIXED large
  interaction            = (G1-G2)-(G0-G3) == (G1-G0)-(G2-G3)
  pct_of_g0_bytes        = 100 * <each contrast> / bytes(G0)   # declared denominator
  totals                 = ROLE-STRATIFIED exact signed sums:
                             A tracked Q0-compatible | B build-output-under-test
                             C all 13, explicitly mixed discovery aggregate
  sign labels            = NEGATIVE | ZERO | POSITIVE  # restatements of published integers
  token                  = PARITY-GEOMETRY-REPORTED   # asserts measurement, not a grade
  materiality_threshold  = null          # no cut value is invented or applied
  epsilon                = null          # no eps parameter exists

[FORBIDDEN]
  emitting FRONT-CROSSING, FRONT-GAP, DOMINATED, DEGENERATE or any other frontier class ·
  inventing or tuning a materiality threshold · epsilon tuning (no eps parameter exists) ·
  timing, parallelism, RSS or reference bytes entering any contrast or classification ·
  pooling Linux build-output bytes as if they were retained bytes · cross-run splicing with the
  frozen Class A/B grid · calling G0-G3 a window, reach or geometry effect, or calling only the
  interaction a fixed-factor contrast · collapsing the five retained configurations onto one
  stream-lambda · ANVIL_STREAM_LAMBDA set · novelty / mechanism / contribution language
```

---

## 13. Validity gates

| id | gate | failure outcome |
|---|---|---|
| G-A | every candidate cell passes its **own** retained configuration argv verbatim — `--parse`, `--literal`, `--entropy`, `--shape-states=28`, its **own** `--stream-lambda` ∈ {0.04, 0, 0.01}, plus the pinned bench-time defaults. A duplicate `(parse, lambda)` pair is a **collapse failure** | `INVALID-Q2` |
| G-B | tracked corpus files match `tests/corpus/CHECKSUMS.txt`; `anvil.exe`/`anvil_bench.exe` are **this build's outputs under test** — identity recorded, never asserted against historical hashes | `INVALID-Q2` |
| G-C | `roundtrip_verified == true` for every cell, by **byte-compare**, never by exit code (E4 recorded a helper returning 0 with no output under load) | `INVALID-Q2` |
| G-D | `block_count == ceil(n/block)` (final block short), Σ block lengths == `n`, all four cells | `INVALID-Q2` |
| G-E | `bytes(R3) == Σpayloads + declared container framing`, reconciled exactly; `framing_ref` emitted | `INVALID-Q2` |
| G-F | R1/R3 are explicitly labelled **descriptive-only** while the Brotli producer is unpinned; no equality-to-frozen claim is made | any reference-derived parity claim is rejected; candidate factorial unaffected |
| G-G | router counters present for every candidate cell (`blocks`, `compressed_blocks`, `raw_blocks`) | `INVALID-Q2` |
| **G-S** | retained **byte** fidelity is evaluated first on every tracked stratum-A G0 row; all must be byte-identical before contrasts are computed | **`BASELINE-VOID (build drift)`**, contrasts withheld, non-zero exit |
| G-I | no arm string contains `--parse=ratio`, `--ratio-backend`, or `--bwt-aux=on` | `INVALID-Q2` |
| G-J | contrast identity `interaction == (G1-G0)-(G2-G3)` verified in code before any quantity is published | `INVALID-Q2` |
| G-K | `--decode-threads=1` present on every candidate cell and recorded | `INVALID-Q2` |
| G-L | dual-bar status emitted per cell: `NO-FROZEN-SAME-TRANSFORM-CONTROL` for the four `{synthetic}` cells (`tests/xz-transform-controls-i9.csv` covers only the tombstoned `pe-*` set — byte-identical to queue E5) | recorded, never silently cleared |
| G-M | both aggregates emitted (`aggregate_all_roles`, `aggregate_heldout_only` = `null`); `promotion_authorized: false` | `INVALID-Q2` |
| G-N | `ANVIL_STREAM_LAMBDA` unset in the workflow environment, in the collector's child environment, and in the operator shell (`src/anvil.cpp:4959`) | `INVALID-Q2` |
| G-O | measurement-critical production blobs equal the coordinator-validated base blob ids, asserted **before build and before any codec invocation**: `src/anvil.cpp` `755df76a…`, `tools/bench_native.cpp` `4f6ceb5a…`, `CMakeLists.txt` `449ae00e…`, the frozen suite CSV `69bd30c3…`, `tests/noise-floor.csv` `f7979be2…`. The Q2 commit may add **only** prototype/docs/workflow files. | **`INVALID_INFRA`** — job exits non-zero, no codec invoked |
| G-P | checkout target is the **workflow/staging commit itself** (`github.sha`), never a foreign ref: local `b8eae11` does not exist on the sanitised mirror, and checking out public base `564e2cd…` would delete `prototypes/swarm-2026-10-02/q2-parity/`. Correctness is guaranteed by G-O, not by ref selection. `workflow_commit_sha` and `base_parent_sha` recorded in provenance. | **`INVALID_INFRA`** |
| G-R | the 13-file population is derived from the **frozen Class-A CSV**, never from `CHECKSUMS.txt` (which has 24 entries); exactly 13 names, equal to `Q0_FROZEN_FILES`, all present in `CHECKSUMS.txt`, all byte counts agreeing with the frozen `input_bytes` | **`INVALID_INFRA`** — population derivation is fail-closed (`q2_arms.py --population-report`) |
| **G-S** | **retained-twin PREFLIGHT, first workflow stage.** G0 (262,144 B, gate ON — the frozen Class-A geometry) on **stratum-A tracked files only** must be **byte-identical** to the frozen per-file candidate bytes, checked **before any geometry arm runs and before any contrast is emitted or interpreted**. Build-output slots are excluded by construction. | **`BASELINE-VOID`** (build / corpus / configuration drift). Geometry contrasts are **withheld** (`records: []`, `contrasts_withheld: true`), the analyzer exits non-zero, and the G1/G2/G3 arms are **never spent**. Substrate outcome — no codec result, no interpretation. |
| **G-T** | the Brotli producer must be **pinned end-to-end** (version + commit/tag + verified artefact identity) for the R1/R3 matched geometry-control pair. `apt libbrotli-dev` is **not** a pin; its recorded version is provenance only. | **HARD HOLD** — `HOLD-BROTLI-PRODUCER-UNPINNED`. The reference half is held, **not weakened**: `D_ref` is withheld while the geometry factorial contrasts (which depend on no reference arm) stand unaffected. Cleared only by a coordinator-supplied pin. |

---

## 14. QP-NOVELTY, preserved verbatim (queue NQ-8)

> A block-size finding is a **configuration/adoption result** (`{engineering}`), not a mechanism.
> Q2 MUST NOT authorize a block-size-selection or adaptive-blocking mechanism claim: that framing
> is occupied by Brotli's meta-block splitter and zstd's `targetCBlockSize` split analysis, and
> **"our block size was mis-set" is not a contribution.**

Applied to every possible Q2 outcome:

| outcome | label |
|---|---|
| bytes move at large geometry | **`GEOM-SENSITIVITY` measurement result.** Not a mechanism. Not a configuration decision. |
| interaction non-zero | **a measured geometry × gate interaction.** Reported as a number; interpretation is the coordinator's. |
| larger geometry increases bytes | **a valid measured geometry effect** at two levels. Reported as a number. |
| parity residual diverges from the generic tax | **descriptive divergence.** Read by the coordinator; Q2 attaches no label. |
| any reading of the four contrasts as a mechanism | **forbidden** |

`QP-NOVELTY-2` is satisfied by construction, and more strongly than in Rev 2: Q2's analysis contains
**no tunable weight and no threshold at all** — `materiality_threshold` and `epsilon` are both `null`,
and the only constant in the baseline diagnostic (`DEGENERATE_RATIO = 0.95`) is adopted unchanged from
the ruling. The per-configuration `--stream-lambda` values are frozen *configuration identity* copied
from `tools/bench_native.cpp:24,70-74`, never a weight the analysis uses. The 11 B/block framing quantum
is a descriptive annotation of block-count change and is compared against nothing.

---

## 15. Deliverables and provenance

**Files written (all new, all isolated):**

* `docs/swarm-2026-10-02/Q2-PARITY-PLAN-SPACE-BUNNY.md` (this file)
* `prototypes/swarm-2026-10-02/q2-parity/README.md`
* `prototypes/swarm-2026-10-02/q2-parity/q2_arms.py`
* `prototypes/swarm-2026-10-02/q2-parity/q2_brotli_geom.cpp`
* `prototypes/swarm-2026-10-02/q2-parity/q2_collect.py`
* `prototypes/swarm-2026-10-02/q2-parity/q2_factorial.py`
* `prototypes/swarm-2026-10-02/q2-parity/anvil-q2-parity.DRAFT.yml`

**Not touched:** `.github/`, `src/`, `tools/`, `tests/`, `FORMAT.md`, `RESEARCH_LEDGER.md`, every
coordinator-owned document, every other lane's report.

*Provenance: every `[M]` names a frozen artifact or a source line; `[P]` marks a projection frozen before
measurement; P1-P5 and the §12 predicates are falsifiable and pre-registered. Local computation was
limited to bounded static validation — `py_compile`, the fixture-only predicate/algebra self-test, the
symbolic interaction-identity check, the environment-identity check, plan emission without `--execute`,
the forbidden-token scan, and pure arithmetic over the retained frozen CSV (the same measurement class
Q0's own ruling permits). **No codec was invoked, no corpus byte was compressed, no timer ran, no
network was used, nothing was dispatched, no build was run.** The baseline diagnostic **reproduces Q0's
frozen `435 / 28 / 5 / 0` tuple exactly** from the retained CSV, which validates this implementation of
the adopted predicate independently of any remote run.*

*Withdrawn across revisions, and not carried forward: Rev 1's `AN-256K/AN-4M/AN-WF` candidate ladder, its
`AUX-PIN` BWT/rev-2 probe, its predeclared ≥ 7 % decode timing endpoint, and its byte-superiority decision
rule; Rev 2's `INTERACTION_FLOOR_BYTES = 11 x block_count(G0)` materiality floor, the 0.05 % secondary
floor, the `GEOM-GATE-INVARIANT` / `GEOM-GATE-CONFOUNDED` vocabulary, the six-arm candidate ladder with
one collapsed `--stream-lambda=0.04`, the `D1` byte-superiority rule, and the erroneous claim that
`G0 − G3` confounds gate with block count. Each was replaced by something stricter: the crossed 2×2 makes
every main effect hold the other factor exactly fixed; no threshold replaced the invented floors; the
five retained configurations replace the collapsed lambda; exact unthresholded contrasts replace `D1`.*
