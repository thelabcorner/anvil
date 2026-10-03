# SYNTH — Measurement Red Team · Fledge Alpha Free

**Question posed:** *what experiment could produce a persuasive but invalid result?*
**Date:** 2026-10-02 · **Agent:** Fledge Alpha Free (independent adversarial reviewer)
**Scope:** cross-track audit of Tracks 01–20 measurement exposure: provenance, splicing,
timing estimand, multiple comparisons, parity, RSS/binary comparability, output identity.
**Writes:** this file only. No existing file modified. No commit/push/reset/clean/stash/
restore/rebase. No local benchmark, sweep, or fuzz campaign.

**Read-honesty disclosure.** I read **Tracks 04 (both reports) and 15 (fledge) in full**, plus
every shared instrument directly (`anvil-i10-dense-frontier.yml`, pinned `paired_bench.py`,
`pareto_front.py`, the G5A/G5B/G5-paged preregs' identity sections, `PR-4 v1.2`,
`gate-ruling-i8-pareto-win.md`, `I10-REMOTE-BASELINE-CLOSURE.md`, `src/anvil.cpp`,
`tools/bench_native.cpp`). I did **not** read the other 17 track reports. Therefore:
claims about the **shared instrument** and about **`src/anvil.cpp` semantics** are [M] —
verified by me. Claims about **what an individual track proposes** are marked [U] and are
stated as *exposure surfaces to audit*, not as findings against those tracks. Extending §3 to
per-track verdicts requires reads I did not perform and I flag that rather than bluff it.

**Labels:** **[M]** verified by me · **[A]** arithmetic on [M] · **[H]** hypothesis · **[U]** unverified.

---

# §0. THESIS — the most dangerous experiment in this project is not wrong, it is *right-looking*

The failure mode that actually threatens this program is not a fabricated number. It is a
**reproducible, hash-verified, deterministic, correctly-gated artifact that answers a slightly
different question than the one being asked** — and that reads like a win.

I call the generator the **"one bright cell" pipeline**:

> Run one large paired job (~200 cells). Apply a per-cell gate at ε = 2 % with no family-wise
> error control. **Report the single cell that passes.** The result carries raw repetitions, a
> bootstrap CI, SHA-256-verified round-trips, pinned blob identities, and a clean
> `DESCRIPTIVE_NON_DOMINATED` label.

Every one of those attributes is a *correctness* property. **None of them is a *validity*
property.** They certify that the number is what was measured. They certify nothing about
whether the measurement answers the question.

**The queue-level arithmetic, which is the core of this report:**

- One dense-frontier-style Silesia job = 20 arms × 5 files × 2 planes = **200 cells**, of which
  **190 are non-null tests** (10 are A/A).
- Under the global null, with a per-cell one-sided 2 % practical-effect gate and realistic
  hosted-VM per-pair log-ratio dispersion, the **expected number of spurious `PASS_SPEED_GATE`
  cells is ≈ 5.7–9.5 per job** (constructive Track 04 §9/V4; I adopt the figure, and note in
  §2.4 the two corrections I would apply).
- **20 tracks, each selecting its best cell from its own family ⇒ an expected yield of roughly
  one persuasive-but-invalid "win" per track ⇒ ≈ 20 across the queue.**
- Now add §5: all 20 share **one driver blob** and **one reference grid**, so those ~20 are not
  independent. The coordinator sees a wall of mutually-corroborating reports.

**A single, correct, reproducible `FRONT-CROSSING` produced this way would be the most
valuable-looking and least trustworthy object the project can emit.** Every gate in the frozen
vehicle would be green.

**Pre-registered kill of this thesis (falsification criterion for me):** if the queue
demonstrably applies family-wise error control and reports **all** cells including nulls, then
"selection by max-p" is not occurring and §0's expected yield collapses from ~20 to ~0. I found
no FWER control in any of the six workflow/prereg files I read. **INV-6 (§6) is the check.**

---

# §1. The seven axes, audited against the shared instrument

## 2.1 Provenance

| Check | Status | Evidence |
|---|---|---|
| Codec source pinned by blob | **GOOD** | `0dce534e…`, `src/anvil.cpp` blob `755df76a…`, `CMakeLists.txt` `449ae00e…`; 12 identity assertions (`workflow:123–147`) **[M]** |
| Tooling pinned by commit + blob | **GOOD** | `b6a243c9…`; `paired_bench.py` pin `f847c50e…` = the **HEAD** blob, so the dirty worktree copy `bb2b2a7e…` is correctly insulated **[M]** |
| Checkouts asserted clean | **GOOD** | `test -z "$(git -C … status --porcelain)"` × 3 **[M]** |
| Toolchain pinned | **GOOD** | clang 18.1.3, cmake 3.31.6, ninja 1.13.2, python 3.12.3, brotli 1.1.0-2build2, zstd 1.5.7, zstd-dev 1.5.5+dfsg2, xz 5.4.5, nproc = 4 **[M]** |
| **Runner image version** | **PARTIAL** | `ImageOS`/`ImageVersion` are *recorded* (`workflow:108–109`) but **not asserted**. The closure doc already records a smoke run landing on a **different host class (EPYC 9V74 vs EPYC 7763)** — so host-class drift is *demonstrated to occur*, not hypothetical **[M]** |
| **Externally-produced byte totals** | **FRAGILE** | Closure table: identical inputs → zstd-22-long27 moved **−158,103 B** (Silesia) and **−61,224 B** (enwik8) across toolchain versions; xz-9e −96/−8 B; **Brotli q11/lw30 moved 0/0** **[M]** |
| **`src/anvil.cpp` ownership** | **UNRESOLVED** | 20 agents, one shared codec file. The working agreement in `docs/CONTEXT.md` says only the *arch* lane edits `src/anvil.cpp` **[M]**, but nothing *enforces* it per-dispatch. Two tracks each believing they own it produces two results pinned to **different blobs of the same file** — both hash-clean, both "provenance-complete" |

**Provenance red-team conclusion.** Blob pinning is genuinely strong **and irrelevant to the
main risk**: it certifies *which* bytes were compiled, not *whether that byte-column answers
the question*. The two live provenance risks are (i) unasserted host-class drift, and (ii)
`src/anvil.cpp` multi-owner contention across 20 concurrent tracks.

## 2.2 Splicing

- Absolute throughput must never cross a job boundary. The dense vehicle's prereg §10.2 says
  so, and closure §"Reference-series interpretation" records it as a rule **[M]**.
- **But the vehicle's own design defeats it.** Each arm is measured in its **own driver
  invocation**, and the frontier test ranks on **absolute MB/s across invocations** while the
  paired CI — the only drift-robust quantity produced — is **computed and then never used**
  (`workflow:851` vs `paired.csv` retention). **[M]** This is a splicing defect *inside a
  single job*, which is worse than splicing across jobs because it is not covered by any
  stated rule.
- PR-4 §10.3/§3c window exclusivity is a **Windows-host** concept. A GHA 4-vCPU VM has no
  bench-lane window owner, and `concurrency` groups only *per corpus*, so a Silesia job and an
  enwik8 job can run simultaneously on different VMs — fine for bytes, and for timing they are
  correctly forbidden from being combined. Recorded as **[M] compliant**.
- **The subtler splice:** the *same* reference grid is reused by every track that cites
  "beats brotli q4". That is one measurement quoted N times (§5).

## 2.3 Timing estimand

The estimand is **whole-process wall time** measured by Python `perf_counter()` around
`subprocess.run`, with every invocation additionally routed through a generated bash wrapper
that `rm -f`s then `exec`s (`workflow:467–508`; `paired_bench.py` `run_once`) **[M]**.

```
T_measured = T_wrapper_bash + T_fork + T_dynlink(arm) + T_codec(input) + T_write
```

Only the `T_codec` term is a property of the codec. Three consequences:

1. **Common-mode, therefore A/A-invisible.** The A/A null pays the same overhead as its
   treatment arm, so it passes cleanly while the estimand is wrong. *A/A validates noise; it
   does not validate the estimand.* **[M]** — this is the single most important structural
   claim in this report.
2. **Order-of-magnitude contamination on the decode plane** (my [H/P] estimate from Linux
   figures in `docs/CONTEXT.md`, ~1,000–1,400 MB/s Brotli decode, ~5–7 ms wrapper+exec):
   `xml` >100 %, `dickens` ~70 %, `nci` ~20 %, `webster` ~17 %, `mozilla` ~14 %.
   A claimed 2–5 % decode effect sits **inside** the contamination band on 4 of 5 panel files.
3. **The RSS census and the timing run are different command constructions.** The census times
   the **unwrapped** command; timing uses the **wrapped** one, and the census runs **unpinned**
   while timing is pinned — undeclared **[M]**.

**Cheapest available fix already in-tree:** `src/anvil.cpp:5023–5026` times `decompress`
internally with `steady_clock` and calls `write_file` *outside* the timed region **[M]**. The
project's own CLI already does the right thing; only the bench harnesses do not.

## 2.4 Multiple comparisons

**Measured/inherited:** ~190 non-null tests per Silesia job, per-cell one-sided gate at
ε = 2 %, **no FWER control anywhere** in the driver, the prereg, or the workflow **[M]**.
Expected spurious passes 5.7–9.5 per job (constructive §9/V4).

**Two corrections I would apply to that figure, and why the direction matters:**

- The 5.7–9.5 estimate is **conservative for the queue** because each track reports its
  **maximum** over its family. Selecting the max of ~190 cells inflates both the effect size
  and the confidence, so the *reported* false-positive rate is far above the per-test rate.
- Conversely it is **optimistic in one respect**: MAD-based `robust_cv` can be **exactly 0**
  for a set containing a 3× outlier (deviations `[0,0,0,0,.4,.9,2.0]` ⇒ MAD = 0 ⇒
  `robust_cv = 0.0000` ⇒ gate passes) **[M, hand-verified]**. So a subset of cells passes the
  dispersion gate with gross dispersion present, which further inflates yield.

**Additional queue-level multiplier the track reports do not account for:** there is also
**corpus selection**. If tracks choose which held-out family to report on *after* seeing which
family showed a win, that is a second, undeclared selection stage — and the "held-out" label
becomes false. **Track 03 owns this control and it is the single highest-leverage
process control in the queue.** [H on prevalence; the control's importance is structural]

## 2.5 Parity

| Dimension | Status | Evidence |
|---|---|---|
| **Block size vs reference window** | **BROKEN on Class A; OK on Class B** | `anvil.cpp:3769` default `block_size = 256*1024`; `bench_native.cpp:25` never assigns `o.block_size` and never uses `parse="ratio"` ⇒ Class A = 256 KiB vs `BROTLI_DEFAULT_WINDOW` = lgwin 22 = 4 MiB (**16× asymmetry**) **[M]**. Meanwhile `anvil.cpp:4999` raises `--parse=ratio` to `128u<<20` when `--block=` is absent, and every file in Silesia (max 51,220,480 B) and enwik8 (100,000,000 B) is < 134,217,728 ⇒ **Class B is whole-file, at parity** **[M/A]** |
| **Blocks are self-contained** | **[M]** | `anvil.cpp:4868–4890`: `decode_one_block(…, block_size, e, opt)` and `decode_one_block_into(…)` receive **no already-decoded-history pointer** ⇒ match distance is bounded by block size. So block size *is* the memory horizon |
| **Silent override precedence** | **[M] — HIGH** | `anvil.cpp:4965` sets `block_explicit` from `--block=`; `:4999` honours it **over** `--parse=ratio`. Adding `--block=` silently changes bytes by up to the measured E4 regret (+16.98 % @256 KiB) **while the arm name and `arm-contract.json` are unchanged** |
| Thread counts | **DOCUMENTED, asserted weakly** | ANVIL harness sets `decode_threads = 1`; zstd `-T1`; `xz -9e -T1`; libbrotli is single-threaded by API **[M]**. But `set_affinity()` returns `None` when `sched_setaffinity` is unavailable and the driver proceeds **unpinned** — prereg §3 calls unpinned timing invalid, and nothing enforces it **[M]** |
| Warmup | **UNVALIDATED** | fixed 2 alternating warmups, never checked for sufficiency **[M]** |
| Same-transform controls | **CORRECTLY EXCLUDED but REQUIRED** | prereg §5 keeps `xz --delta`/`--x86` out of the reference class; PR-4 §4a/A13 requires them as a dual-bar qualifier with any crossing claim **[M]** — an easily-dropped requirement |
| Env parity | **UNASSERTED** | driver uses `os.environ.copy()` with `--env-json` default `{}`, never passed by the workflow, so ANVIL env knobs (e.g. the documented `ANVIL_STREAM_SLACK`, which trades rate for speed) are inherited implicitly and asserted by nothing **[M]** |

**Parity conclusion.** Two of the seven parity dimensions are broken or unenforced, and the
block/window one is **the** defect: it does not merely add noise, it **structurally
disadvantages the candidate on the ratio axis by up to ~17 %**, and it is invisible in the arm
name. Four tracks are intrinsically exposed (§3).

## 2.6 RSS / binary-size comparability

- **RSS is harness architecture.** ANVIL's decoder harness `read_file()`s the whole input and
  `decompress` pre-reserves the declared output — `reserve_hint = min(total,
  max(1MiB, in.size()*256))`, `out.reserve(...)` (`anvil.cpp:4859–4860`) ⇒ peak RSS
  **≈ packed + original, deterministically**. mozilla ⇒ ≈ 65.0 MB ≈ 1.28× source **[M/A]**.
  Brotli's harness does the same via `vector(expected)`. **zstd/xz stream.** `zstd-22-long27`
  allocates a **windowLog 27 = 128 MB** decoder buffer; `xz-9e` a 64 MiB dictionary **[M]**.
  ⇒ the decode-RSS axis ranks `zstd-1..19` best by ~an order of magnitude **for reasons that
  say nothing about compression**.
- **Both RSS axes sit inside the dominance predicate** (`workflow:891–892`), and
  `descriptive_class` keys off the variant that still contains both (`:913`) **[M]** — so even
  the most optimistic label is structurally unreachable for a whole-buffer codec.
- **Binary size compares incompatible scopes.** `size_key` maps anvil/brotli →
  `decoder-linker-gc-harness` and zstd/xz → `system-cli` (`workflow:873–883`), and
  `dominated_full_cost` compares those stripped bytes directly (`:895`), while prereg §9
  forbids exactly that and `summary.json` sets `binary_size_comparability:
  "descriptive-only"` — a **direct prereg↔code contradiction** **[M]**. The bias runs **both
  ways** (Brotli's decoder-only harness will look ~10× smaller than ANVIL's; ANVIL's will look
  smaller than the full zstd CLI), so the column is uninformative, not merely pessimistic.
- **No zstd or xz decoder-only measurement exists** in the vehicle **[M]**.

**Red-team requirement:** the honest reading is that **two different claims are fused**. (a)
*Measured*: peak RSS of these particular harnesses. (b) *Real*: ANVIL has **no streaming
decode API** — `decompress(const vector<uint8_t>&, const Options&) -> vector<uint8_t>` — so
Θ(input+output) decode RSS is a **product** property. (a) is invalid as a cross-family axis;
(b) is a genuine competitive risk. **The vehicle neutralises (b) by mislabelling it as (a).**
Escalate (b) to Tracks 18/19; forbid (a) in dominance.

## 2.7 Output identity

| Check | Status | Note |
|---|---|---|
| Freshness before every warmup and timed invocation | **GOOD** | generated `observe.sh` `rm -f` + absence check + `exec` **[M]** |
| Re-hash after **every** repetition | **GOOD** | `post_run_observation` per rep, deliberately with `--expected-sha256` **not** passed so hashing stays out of timed regions **[M]** |
| Independent re-verification outside timing | **GOOD** | `verify_raw` + final observation check **[M]** |
| Round-trip byte-identity per cell | **GOOD** | decode SHA-256 vs source, hard fail **[M]** |
| Frozen ANVIL byte table provenance | **VERIFIED** | my independent sum of the 12 legacy per-file values = **46,446,995** = the closure's frozen `ANVIL auto-direct` total, exactly **[A/M]** |
| Frozen reference totals provenance | **VERIFIED** | they are the closure's `Linux remote` column, not the `Frozen I9` column. I initially misread the columns and nearly declared a healthy gate dead; **withdrawn** and recorded because that is exactly the error a red team must not ship **[M]** |
| **Silent config override** | **BROKEN [M]** | `--block=` beats `--parse=ratio` with no arm-name or contract change (§2.5) |
| **`arm-contract.json` fidelity** | **BROKEN [M]** | contract says `"legacy": "auto-direct"`; executed command is `--parse=ratio --ratio-backend=auto --ratio-context=off --ratio-lines=off` (`workflow:653` vs `:514–516`) |
| **`tools/bench_ratio.py` vs the vehicle** | **WATCH [U]** | `tools/bench_ratio.py` exists in the tooling pin but I did not confirm the vehicle uses it rather than a generated helper. If a generated helper is used, a second encode path exists with unknown config |

**Output-identity red-team conclusion.** The identity machinery is among the best work in the
repository — and it is **orthogonal** to validity. A cell can be perfectly hash-verified and
still answer the wrong question. Worse, the two places where identity *fails* (silent
`--block=` precedence, contract/code mismatch) both **change which configuration was measured
without changing any recorded label**.

---

# §3. Track exposure map

Exposure = "does this track's headline claim transit the broken axis?" Not a verdict on the
track; the track reports themselves were not read except 04 and 15. **[U] on prevalence.**

| Track | Broken axis it transits | Exposure |
|---|---|---|
| **01** G5D paged base+overlay dictionary | **block/window parity** — a cross-block dictionary is *defined* by crossing block boundaries, and at 256 KiB the mechanism's whole purpose is truncated | **HIGHEST** |
| **07** BWT subblocking | **block/window parity** — subblocking *is* block structure; a "subblocking helps" result at 256 KiB may be measuring de-blocking | **HIGHEST** |
| **09** TCOPY / PNRA | **block/window parity** — long-horizon temporal anchoring is truncated by a 256 KiB horizon; plus timing for search cost | HIGH |
| **15** long-range / cross-block memory | **block/window parity** (self-identified; the only track that raised it) | HIGH |
| **02** G5A finalist planner | timing estimand (encode), multiple comparisons (selector searched over many arms) | HIGH |
| **05** backend frontier beyond Brotli | timing estimand; reference-density completeness | HIGH |
| **06** entropy backend co-design | timing estimand (decode throughput is the axis); decoder-size comparability | HIGH |
| **11** orbit / program synthesis | timing estimand (decode); decoder memory (bounded decode is its premise) | HIGH |
| **12** encoder-only search | timing estimand (encode); **overfitting/selection** — a search that reports its best parse is a max over a huge family | HIGH |
| **16** segmentation / routing | timing estimand; selector economics vs planner | MEDIUM–HIGH |
| **17** parser / candidate generation | timing estimand (encode asymptotics) | MEDIUM–HIGH |
| **13 / 14** numeric / executable compilers | block parity (field/record structure); silent `--block=` override | MEDIUM |
| **10** correction topology | byte-only ⇒ safe **if** parity holds; MDL claims are deterministic | MEDIUM |
| **04** dense measurement | **the instrument itself** | n/a |
| **03** held-out corpus | **leakage control — its failure invalidates the whole queue** | n/a, but decisive |
| **18 / 19** decoder arch / security | the mismeasured streaming-API gap (§2.6); declared-size-influenceable RSS (`anvil.cpp:4859`) | MEDIUM |
| **20** prior art / kill team | **not measurement-exposed**; exposed to "the byte win is real ⇒ the mechanism is novel" — and the one live win is explicitly **adopt-class** | MEDIUM |

**Cross-cutting exposure:** every track that reports a *ratio* on any file > 256 KiB measured
with the Class-A instrument. Every track that reports a *speed* anywhere. Every track that
selects a family after seeing results. Every track that cites "beats brotli q4".

---

# §4. Five concrete "persuasive but invalid" results, each with the artifact it emits

### P1 — *Sub-block regret misattributed to a mechanism*
**Artifact:** "Track 07: BWT subblocking improves Silesia ratio by 4.2 % (deterministic,
round-trip verified, SHA-256 pinned)." **Invalid because:** the comparison arm differs in
**block structure**, not only in mechanism. At 256 KiB the *baseline* carries up to ~16.98 %
block regret (measured E4); part or all of a 4.2 % "gain" can be de-blocking, not subblocking.
**Why persuasive:** largest, most deterministic, most reproducible effect class in the project.
**Required control:** same-rep block-parity pair (default vs `--block=` raised), §6 INV-7.

### P2 — *Harness overhead mistaken for a decode effect*
**Artifact:** "Track 06: new entropy layout decodes 1.04× faster (paired CI [0.962, 0.999],
A/A green)." **Invalid because:** the decode estimand carries 14–>100 % common-mode
additive overhead on 4–5 of 5 panel files, and **A/A is structurally blind to it** (§2.3).
A 4 % effect is inside the contamination band. **Why persuasive:** it clears the pre-registered
2 % ε, has a CI that excludes 1, and passes every gate in the vehicle. **Required control:**
`G1 overhead_share ≤ 5 %` with an in-process estimand, §6 INV-3.

### P3 — *Buffering architecture mistaken for decoder cost*
**Artifact:** "Track 18: ANVIL decode peak RSS is 2.0–9.1× Brotli." **Invalid as stated:**
measured with a whole-buffer harness against another whole-buffer harness, then compared to
streaming CLIs in the same predicate. **But there is a real finding inside it** — no streaming
decode API — which the artifact *converts from a design commitment into a bench artifact*, so
it will be treated as a measurement problem rather than a product requirement. **Why
persuasive:** the ratio is large, reproducible, and points the wrong way for the competitor.
**Required control:** charge `peak − (input+output)/1024`; split the API claim out (§6 INV-8).

### P4 — *Density/margin noise mistaken for a frontier crossing*
**Artifact:** "Track 16: 1 non-dominated row → `FRONT-CROSSING`." **Invalid because:**
`tools/pareto_front.py` has **no epsilon** (any positive margin wins) and **no FRONT-GAP /
DEGENERATE predicate at all**; the mandated `tools/pareto_verify.py` **does not exist**
(`Test-Path` → `False`); the R-2 bracket classification is applied **by hand, by the gate**,
explicitly not by the tool. The dense vehicle meanwhile automates a **third** arbiter inline,
and `frontier-inputs.csv` **cannot be fed to `pareto_front.py`** (emits `arm`/`plane`/
`plane_mbps`; the tool reads `codec`/`ratio`/`encode_MBps`). **Why persuasive:** a clean
monotone token with a bracket story attached. **Required control:** encode the predicates
with a frozen ε (§6 INV-9).

### P5 — *Correlated consensus mistaken for independent replication*
**Artifact:** "8 of 8 tracks confirm the ANVIL ratio advantage over brotli q4." **Invalid
because:** all 8 read the same reference grid through the same driver blob at the same pinned
versions, and one arm-contract mismatch is already documented. **This is not 8 measurements;
it is 1 measurement quoted 8 times, with 8 selection filters layered on top.**
**Why persuasive:** consensus is the single strongest rhetorical device available and it is
the hardest to audit by reading. **Required control:** INV-12 — a track may not cite another
track's measurement as independent confirmation.

### P6 (bonus, verified from source, cheap to miss) — *Silent config override*
**Artifact:** "Track 13: numeric-lane compiler, +2.1 % ratio at parity." **Invalid because:** if
the arm was re-specified with `--block=` (which silently overrides `--parse=ratio`'s 128 MiB at
`anvil.cpp:4965/4999`) the byte column moved for a **configuration** reason, and nothing in the
arm name, `arm-contract.json`, or artifact records it. **Why persuasive:** it passes every
identity gate — every hash is correct for what was run. **Required control:** INV-5 — resolved
effective configuration serialized per cell.

---

# §5. The correlated-failure problem, stated plainly

The queue's 20 tracks do not have 20 independent chances to be wrong about measurement. They
have **one** measurement instrument, **one** reference grid, **one** pinned driver blob, and
**one** toolchain contract, shared by all of them **[M]**.

Two consequences that no per-track report can see:

1. **A single instrument defect produces 20 apparently independent confirmations.** My §2.3
   finding — A/A is structurally blind to common-mode overhead — does not threaten one track.
   It threatens *all* of them identically, and their agreement is not evidence against it.
2. **Reference-grid reuse means "beats brotli q4" is one measurement, not N.** The dense
   vehicle's whole purpose is to make that statement trustworthy once. Until it passes, every
   track that says it is borrowing credibility it has not earned — and the borrowing is
   invisible because the citation is to a *file*, not to a *number with a grade*.

**This is the queue's real exposure, and it is not on any track's critical path.** It belongs
to the coordinator as a queue-level object. Hence §6.

---

# §6. MANDATORY QUEUE-LEVEL INVARIANTS

Testable. Each is a predicate over a published artifact. **No 20-arm-style dispatch, and no
track-level FRONT-GAP/CROSSING citation, until the invariants its own rows touch are green.**

### Identity & provenance
- **INV-1 — Row completeness.** Every published throughput row carries: `binary_sha256`,
  `src_blob`, `tooling_blob`, `runner_image`, `cpu_model`, `raw_seconds[]`, `reps`,
  `affinity_mask`, `enc_threads`, `dec_threads`, `ref_threads`, and `estimand_id`.
  A row missing any field is **descriptive-only**.
- **INV-2 — Host assertion.** `ImageOS`/`ImageVersion`/CPU model are **asserted**, not merely
  recorded. A host-class change starts a new series and may not be spliced (EPYC 7763 vs 9V74
  already occurred).
- **INV-5 — Resolved configuration.** Every cell serialises its **effective** configuration
  after all overrides: `block_size_effective`, `parse`, `ratio_backend`, `brotli_lgwin`,
  `brotli_quality`, `zstd_level`, `zstd_long`, `xz_preset`, `thread_count`, `env_delta`.
  **No silent override may exist** — specifically `--block=` must not be able to change bytes
  without changing the recorded configuration (§2.5, P6).
- **INV-10 — Single codec owner.** Exactly one owner for `src/anvil.cpp` per dispatch window,
  and every result pins the `src/anvil.cpp` blob it was built from. Two tracks must never pin
  different blobs of the same file in the same window.

### Splicing & estimand
- **INV-3 — Named estimand, no wall-clock ranking.** The ranking quantity is
  `−ln(paired_ratio)` against the **same control in the same invocation**, measured with the
  codec call timed **internally** (load and write outside the timed region). **Absolute MB/s
  from separate invocations is forbidden for any dominance verdict.** An `estimand_id` naming
  which is in use is mandatory (INV-1).
- **INV-4 — No cross-job splicing.** Absolute throughput never crosses a job, VM, or runner-image
  boundary. Silesia and enwik8 absolute numbers are never combined.

### Statistics
- **INV-6 — FWER control, all cells reported.** Every timing family declares its family
  **before** dispatch; **Holm (or BH) within the family**; **all cells published including
  nulls and A/A**; `p_holm` alongside the raw column. **Reporting only the best cell of a
  family is a protocol violation.** This is the single invariant that would collapse §0's
  expected ~20 false wins to ~0.
- **INV-7 — Block/window parity control.** No ratio claim on any input **> 256 KiB** is
  admissible without a same-job parity pair (instrument default vs `--block=` raised to the
  reference window), per file, with the byte delta reported. Class-A verdicts on files
  ≤ 256 KiB are admissible (default ⇒ single block ⇒ parity).
- **INV-9 — Encoded frontier predicates.** `FRONT-GAP` (R-2 bracket), `DEGENERATE` (ratio
  ≥ 0.95 **and** timing degeneracy), and joint-plane `dominates_joint` are **implemented in
  code** with a **pre-registered ε**, not applied by hand. **Exactly one arbiter is
  canonical**; if `pareto_front.py` is it, artifacts must emit its schema. Any
  `DESCRIPTIVE_NON_DOMINATED` label may not be reported as a frontier result.

### Cost axes
- **INV-8 — Comparable RSS charge only.** Dominance uses
  `resident_charge_kib = peak_kib − (input+output)/1024`. **Raw peak RSS may not enter any
  cross-family dominance test** while buffering discipline differs. ANVIL's missing streaming
  decode API is logged as a **product requirement** (Tracks 18/19), never as a bench cell.
- **INV-11 — Binary size scope-matched or descriptive-only.** `system-cli` sizes never
  compete against `decoder-linker-gc-harness` sizes in a predicate. Without a frozen
  decoder-only build policy for zstd/lzma, the size axis does not exist.

### Validity & controls
- **INV-13 — Dispersion gate on the paired statistic, with a MAD floor.** Gate on
  `MAD(log_ratios)`, requiring `MAD ≥ 0.02·median|log ratio|`, so MAD = 0 cannot pass a
  family containing a gross outlier. Per-arm robust CV is recorded as a diagnostic, not a gate.
- **INV-14 — Both nulls, and cells are not fatal.** A **two-path `A/A′`** (two binaries,
  byte-identical output) is present per (plane, file) alongside A/A. A single cell's
  CV/A-A failure is **informative**; whole-run invalidation is reserved for
  correctness/hash/identity failure.
- **INV-15 — Overhead floor.** `overhead_share = (wrapper + exec)/candidate_median ≤ 5 %`, else
  the cell is `DEGENERATE_TIMING_SHORT` and may not support any verdict.
- **INV-16 — Affinity enforced.** A run that *requested* pinning and did not achieve it is
  **invalid**, not merely annotated. Census and timing must use the **same** command
  construction, both pinned.
- **INV-12 — Anti-correlated-evidence rule.** **A track may not cite another track's measurement
  as independent confirmation.** Cross-track agreement must be reported as *N measurements
  through one instrument*, with the shared driver blob, reference grid, and host named.
- **INV-17 — Held-out means held-out.** A family used for any selection (threshold, arm,
  feature, or "this looks promising") is **not** admissible as the confirmation set for the
  claim it was selected on. Track 03 owns the enforcement.
- **INV-18 — Thresholds frozen.** Practical ε, dispersion floor, FWER procedure, parity rule,
  and degeneracy thresholds are declared **before** data and are not moved after. Thresholds
  embedded in code but absent from the prereg are a violation (the live example: the
  hard-coded `DEGENERATE_RATIO` at `workflow:913`, absent from the prereg, dead code at these
  corpora but still a violation).
- **INV-19 — Negatives preserved.** A blocked/failed/invalid run is retained as evidence and
  never replaced by a favourable subset. (The project has already paid the opposite bill: an
  enwik8 arm ruled `TIMING_BLOCKED` at robust CV 0.1756 discarded a **10.15×** decode-ratio
  measurement — **real evidence destroyed by an over-tight gate**, i.e. INV-13 cuts both ways.)

---

# §7. How to verify these without running a single benchmark

All nineteen invariants are checkable as **static predicates over published artifacts** plus
**arithmetic over retained CSVs**. None requires a runner, a corpus, or a timer:

- INV-1/5/10/16 → read the artifact's `pin-checks.json`, `arm-contract.json`, per-cell
  configuration records, and the workflow's assertion list.
- INV-2/4 → read `provenance.txt` and the host fingerprint; compare series labels.
- INV-6/13/15 → recompute from the retained `raw.csv` per cell (Holm, MAD floor, overhead
  share). `raw.csv` is already required by the artifact contract.
- INV-7 → recompute from retained `census.csv`/`bytes.csv` if a parity arm was dispatched.
- INV-8/9/11 → static read of the dominance function's axis list and scope labels.
- INV-12/17/18/19 → read the reports and the preregs; compare declared ε against code
  constants.

**The single highest-value, zero-cost check in the queue:** take any published
"ANVIL beats brotli q4 on ratio" claim and verify INV-7 (was the input ≤ 256 KiB, or was a
parity control run?) and INV-6 (is Holm applied, and are all cells published?). Those two
questions decide whether the claim is a result or an artifact. I estimate they are the two
most frequently failing.

---

# §9. QUEUE REVIEW — adversarial audit of `REMOTE-EXPERIMENT-QUEUE.md` (DRAFT v0)

**Reviewed:** `docs/swarm-2026-10-02/REMOTE-EXPERIMENT-QUEUE.md` (161 lines, read in full).
**Method:** per-job hunt for a result that would be *persuasive but invalid*, against the
seven axes of §1. Verdicts are about the **job as written**, not about the track's merit.
**Circularity disclosed:** Q2's precondition is "measurement synthesis approves instrument,"
i.e. this document. The queue is waiting on this review; I do not treat that as authority.

## 9.0 Coordinator timing rule, adopted and sharpened

The coordinator's rule is **correct and I adopt it as binding**, with three sharpenings:

> **No GHA timing cell may become a FRONT-CROSSING by itself. GHA throughput is
> scout/ranking-grade. Any multi-cell timed pilot requires preselected primary endpoints
> *or* a family-level max-statistic/null procedure, all-cell reporting, and independent
> same-commit replication before a speed claim is carried forward. Prefer reducing the
> experiment family over post-hoc multiplicity correction.**

Three sharpenings:
1. **The unit of inference is the replicated family, not the cell.** A cell may nominate; only
   an independent same-commit replication of the *preselected endpoint* may confirm.
2. **"Independent same-commit replication" must be defined or it is theatre.** I require:
   same commit SHA, same corpus SHA-256, same pinned tooling blob, **a different runner
   instance**, and the replicate's own CI must contain the original point estimate. Two runs
   on the same VM are not independent.
3. **Reducing the family beats correcting it, and must be the *first* option.** A pilot that
   responds to multiplicity by adding Holm is already too big. If the family cannot shrink,
   the correct answer is **do not run the timing**, because Q0 (§10) shows the ratio axis —
   where the only real content is — is deterministic and needs no timing at all.

## 9.1 Per-job verdicts

| Job | Verdict | Persuasive-but-invalid risk | Required change |
|---|---|---|---|
| **Q0** retained-artifact correction | **REJECT AS SCOPED** → split | **Its stated cost ("no codec execution") is incompatible with its stated purpose** (candidate-geometry sweep). You cannot derive `bytes(block=raised)` from a CSV that was encoded at 256 KiB. Either it is artifact-only (then it cannot answer parity) or it runs an encoder (then it is not Tier 0). | Split into **Q0-static** (bracket predicate, axis exclusion, DEGENERATE, joint-plane — pure post-processing, genuinely free) and **Q0-encode** (candidate-geometry sweep — byte-only encoder runs, Tier 1, no timing). |
| **Q0b** Track-12 theorem cleanup | **COMPLIANT** | none — proof/audit, no measurement | none |
| **Q1** CORPUS-ADMISSIBILITY-v1 | **HOLD — threshold gap** | It gates every downstream ratio claim, yet "blind second-auditor agreement" has **no threshold**. A second auditor agreeing 60% is not agreement, and the threshold would be chosen *after* seeing agreement — the exact post-hoc trap. Ordering is also wrong: Q1 certifies input admissibility, so it must precede Q0's *interpretation*, not merely Q6. | Freeze the agreement threshold (suggest **≥90% on conflict edges, 100% on SHA-256/role assignment**) **in the queue text**, before execution. Move Q1 ahead of Q0-interpret. |
| **Q2** corrected six-arm parity pilot | **AGREE — with 3 added preconditions** | Lowest-risk timed job in the queue because bytes are primary and timing is explicitly scout-grade (§9.0). Residual risks: (a) its parity denominator cites "5–17%" restart tax with **no artifact identity**; (b) it must not inherit ε=0 from Q0 (§10.3); (c) ε feasibility must be checked against retained `raw.csv` dispersion *before* dispatch, and if infeasible **shrink the family rather than raise reps**. | Pin the E4 regret-curve artifact by SHA-256 or re-measure it byte-only in-job. Add ε-derivation from `tests/noise-floor.csv`. Add the §9.0 replication requirement. |
| **Q3** stream-cost selector arbiter | **AMEND** | Stage 1 is byte-only and legitimately so. But the selector **chooses a codec per stream by argmax on the same emission it then reports** ⇒ the emitted-byte delta is a fit statistic. "Precision-codec flips" will read as a large win and means nothing about the frontier. | Report the **runner-up gap distribution**, not the sign of the delta. Label all Stage-1 output non-promotable to held-out (INV-17). |
| **Q4a** typed/frontend × BWT byte cross-product | **COMPLIANT** | byte-only, deterministic | none |
| **Q4b** BWT stage decomposition / f_walk | **BEST-SPECIFIED JOB IN THE QUEUE — and it has a missing dependency** | It is the only job with a genuinely frozen numeric kill (≈4.5× ⇒ non-walk share must be ≤~22%; kill the lane otherwise). Two attacks: (i) the Amdahl ceiling is only falsifiable if the shares **sum to 1** — any omitted component makes ≤22% unfalsifiable; (ii) an "instrumented/uninstrumented pair" measures **two different programs**, so the instrumentation tax must be reported and <5% or the shares are re-measured. | Require `Σ shares = 1.00 ± 0.5%` else BLOCKED; report instrumentation tax; **declare the dependency on the Q2 in-process instrument** — a subprocess wall clock cannot attribute 22% of a path. |
| **Q5** MASK-CEILING | **COMPLIANT** | It explicitly disowns its own historical confound (mode11↔mode13). Good practice. | none; verify it needs no encode (mask bytes are readable from an existing payload) |
| **Q6** G5D census-first ablation | **AMEND — highest-leverage process risk** | **"remote discovery only" is the most dangerous label in the file.** Five factors (root/overlay/paging/ordering/escape) × ≥2 levels with no FWER control ⇒ "factor X helps" is a max-of-many artifact, and discovery data measured on a corpus Q1 has not certified will later be quoted as evidence. | Declare outputs **DISCOVERY / non-promotable**, name and lock the confirmation family before Q6 runs, and **preselect the primary factor** or collapse to one-factor-at-a-time. |
| **Q7** DEFLATE reconstruction control | **AGREE** | It already requires both same-transform controls (precomp→xz -9e **and** precomp→Brotli q11/lgwin30) and gates on producer-identity replayability — the strongest framing discipline in the queue. Residual: both references are the **slowest** arms in the vehicle, and xstditionally pays the double-bash-spawn handicap (§2 F11), so any timing phase is biased against xz. | Declare Q7 **byte-only**, or require the Q2 in-process instrument before timing. |
| **Q8** declared-total / amplification probe | **COMPLIANT with one scope caveat** | Synthetic-only ⇒ a family-scoped property. If its ceiling is quoted as a format-wide bound it over-generalises. High synergy: `anvil.cpp:4859` `reserve_hint = min(total, max(1MiB, in*256))` **is** the amplification surface — probe that path directly. | State the family explicitly; require ≥1 real-corpus spot-check before the bound is quoted as general. |
| **Q9** aux-index W1024 vs W16 | **AMEND — most likely to be mis-reported** | This is the best-powered job in the queue (2 levels) and therefore the most likely to produce a *number* that gets over-read. The current aux charge is **+19,344 B on Silesia = 0.00913%**; W16 might remove most of it. **That is ~4 orders of magnitude below the 2% practical-effect epsilon on every other axis in the program.** Expressed as a percentage it looks like a result; in absolute bytes it is a rounding error against the 46,446,995 B frozen total. | **Report absolute bytes only. Forbid percentage or ratio expression.** Pass criterion = a citation-grade byte total with a stated absolute bound, never a relative improvement. |
| **B1–B4, DO-NOT-DISPATCH** | **COMPLIANT and well-ordered** | B4 is correctly gated on "Q2 proves residual block warmup under fair geometry" — the right dependency, already present. | none |

**Queue-level defects in the file itself:**
- **No FWER / all-cell invariant.** The queue has "cancelled, not run for completeness" (good) but
  nothing forbidding max-of-family reporting. **Add §9.0 as a queue invariant.**
- **No ε anywhere.** Q0's own purpose is predicate application, and the queue states no ε and no
  degeneracy threshold. **Add a frozen ε line to Q0.**
- **Missing dependency edges:** (i) Q2 instrument → Q4b/Q7 *timing* phases; (ii) Q1 → Q2's
  *interpretation*; (iii) retained `raw.csv` dispersion → Q2's ε feasibility.
- **Honest note:** I attack Q0 hardest and it is the job most aligned with my own §0 thesis.
  That is not a conflict of interest — it is the same objection arriving from the queue side.

---

# §10. INDEPENDENT AUDIT OF `Q0-DENSE-RETAINED-RESULT.md`

**Method: I reproduced it.** New independent implementation
(`prototypes/swarm-2026-10-02/04-dense-frontier/fledge/independent_arbiter.py`) — my own code,
written from `docs/gate-ruling-i8-pareto-win.md` R-2, not from Q0's script, which I have **not**
read. Pure arithmetic over the retained CSV. **No codec, no corpus, no timer.**

## 10.1 REPRODUCED — exactly

```
csv sha256 : af44d9d3d40fb85c7e35758fd7936adb562c8db6ff455b30371ab355ebe7b65e   <- matches Q0
rows 364 = 13 files x 28 codecs
ANVIL configs 18 | reference class 10: brotli-q1,q4,q6,q9,q11, zstd-1,3,9,19, xz-9e
expected ANVIL row-plane cells = 18*13*2 = 468

eps=0.00   DOM 435 / DEG 28 / GAP 5 / CROSS 0   total=468
```

**I reproduce Q0's `435 / 28 / 5 / 0` on 468 cells with a byte-identical input digest. [M]**
The five FRONT-GAP cells are all on `tests\corpus\generated.json`, encode plane:
`anvil-mdl-rans`, `-l0`, `-l001`, `anvil-shape-rans`, `-l0`. **Confirmed. [M]**
28 DEGENERATE cells, all 18 ANVIL configs on `synth-arith.bin`. **Confirmed. [M]**

**One framing correction:** this tuple is **not new information.** It is the frozen canonical
tuple already recorded at `RESEARCH_LEDGER.md:4927–4928` (33 non-dominated | 5 FRONT-GAP |
0 FRONT-CROSSING | 28 DEGENERATE | 435/468). Q0's headline claim — "0/18 ANVIL configurations
produces even one FRONT-CROSSING" — was **already enumerated in that tuple** (468 = 18×13×2).
Q0 is a valid **reproduction/consistency check** of a ledger number, and that has real value;
but the queue should not read it as a *new* closure of the crossing hypothesis. **Weight
correction, not a correctness dispute.**

## 10.2 AGREE — Q2 is REQUIRED, and I had already reached it independently

Q0's reason: the retained CSV contains **no candidate block-size sweep**, so geometry parity is
not computable from it. That is exactly the conclusion I reached in my Track 04 report §5.2
from the opposite direction (the density sweep is computed on a **Class-A 256 KiB** artifact
while the vehicle would run **Class-B 128 MiB**, `anvil.cpp:4999`). Two independent routes, same
conclusion. **I agree with `Q2 REQUIRED`.**

Q0's preconditions 3, 6, 7, 8, 9 already encode the coordinator's timing rule and the RSS/binary
scope rule. **I agree with all nine**, with additions in §11.

## 10.3 **DISAGREE** — Q0's ε-sensitivity claim is definition-dependent, and ε is outcome-determining

Q0 states: *"adding a material epsilon to the bracket predicate can relabel the five retained
FRONT-GAP cells as CROSSING at epsilon >= 0.02."*

**My sweep does not reproduce that** under the natural reading (ε = minimum bracket **separation**
on both axes): **the tuple is invariant at ε ∈ {0, 0.02, 0.05, 0.10, 0.25} — 435/28/5/0 at every
value. [M]** So the claim is true only under a *different* ε placement.

I then found the placement that makes it true, and it is the one that matters. Extracting the
actual brackets for `anvil-shape-rans` (ratio 0.112, encode 1.429 MB/s) against the retained
reference rows on `generated.json`:

```
lo = xz-9e     ratio 0.092975   encode 1.325 MB/s
hi = zstd-19   ratio 0.113      encode 2.839 MB/s
```

The candidate's margins to the bracket:

| edge | axis | candidate vs edge | margin |
|---|---|---|---:|
| hi (`zstd-19`) | **ratio** | 0.1120 vs 0.113 | **+0.9 %** |
| hi (`zstd-19`) | encode | 1.429 vs 2.839 | −49.7 % |
| lo (`xz-9e`) | ratio | 0.1120 vs 0.0930 | −20.4 % |
| lo (`xz-9e`) | encode | 1.429 vs 1.325 | **+7.8 %** |

**So ε must be an "inscribed margin" ε** — the candidate must lie strictly inside the bracket by
more than ε on both axes. The tightest margin is **0.9 % on ratio**. **At ε = 0.02 the bracket
dissolves and all five become FRONT-CROSSING.** Q0's claim is therefore **correct and precisely
diagnosable** — I withdraw the general objection and replace it with the sharper one:

> **The project's single most contested classification — the only five non-dominated cells it
> has — is decided by a 0.9 % ratio margin, which is less than half the project's own 2 %
> practical-effect epsilon.** The same numeric 2 % is simultaneously used as (a) the floor below
> which a *speed* difference is not real, and (b) the inscribed margin below which a *bracket*
> is not real. Those two uses are contradictory in direction: (a) says sub-2 % differences are
> noise; (b) says sub-2 % separations are not real gaps. **The same number cannot mean both.**

**And the direction of the error is unfavourable.** Per `tests/noise-floor.csv`, **`ratio` has
CV = 0.000 on every row** (byte-deterministic) while **`encode_MBps` CV runs 1.5 %–35.5 %** on the
available cells **[M]**. Therefore:

- the **0.9 % ratio margin is real and deterministic**;
- the **7.8 % encode margin against `xz-9e` is the *only* thing standing between these cells and
  DOMINATED**, and 7.8 % sits **inside the project's own measured encode dispersion band**.
- The retained window is itself **ranking-grade**, not citation-grade (pinned-core pre-window
  util 26.9 % / 9.2 %, both above the <5 % gate).

**Consequence:** `FRONT-GAP` requires trust in **both** axes, but on this substrate the
load-bearing axis is a speed margin smaller than the substrate's own noise. **The five
FRONT-GAP labels are not admissible at citation grade.** The honest label is **INCONCLUSIVE** —
and Q0's four-class vocabulary `DOMINATED → DEGENERATE → FRONT-GAP → FRONT-CROSSING`
**cannot express "this cell's class is decided by a measurement we know is not citation-grade."**
That is a structural gap in the doctrine, not a defect in Q0's arithmetic.

**Retained tuple restated honestly:** `435 DOMINATED | 28 DEGENERATE | 5 FRONT-GAP* | 0
FRONT-CROSSING`, where `*` = **substrate-limited**, class flips inside the ranking-grade band.
This is *more* reason for Q2, and it changes Q2's deliverable (§11).

## 10.4 **DISAGREE** — Q0 dismisses its strongest diagnostic

Q0 notes that the audit script "treats Brotli, zstd, and xz ladder rows as candidate families
against subsets of the other reference codecs and observes many 'crossings'," then sets it aside
as non-decisive.

**That is the most important result in the whole exercise and it is filed as an aside.** If
*shipped codecs at shipped settings* produce many crossings against each other, then:

1. the reference class is **not internally non-dominated**, so `DOMINATED` is a statement about
   *which subset* was chosen, not about the candidate;
2. this is the direct, retained-artifact confirmation of §5.2's density sweep
   (`mean_bracket_gap` 28.98 → 5.00 monotonically in *k*);
3. it means **"435/468 DOMINATED" is grid-conditional evidence** and must be reported with the
   reference subset named, or it is not citable.

**Q0 should promote this from non-decisive diagnostic to a first-class result**, together with a
**leave-one-reference-out sensitivity table**: for each of the 10 reference arms, how many of the
5 FRONT-GAP labels survive its removal. That table is free (retained-artifact arithmetic) and it
tells the coordinator exactly how much the five labels depend on `xz-9e` — which, per §10.3, is
the arm carrying the entire 7.8 % speed margin.

## 10.5 Minor

- Q0 cites the restart tax as "5–17 %" with **no artifact identity**. Same provenance discipline
  the queue applies to everything else must apply here: pin
  `docs/audit-2026-09-07/13-e4-block-routing-oracle.md` by SHA-256, or re-measure byte-only in Q2.
- Q0 should **enumerate the 28 DEGENERATE cells** so the DEGENERATE guard is auditable
  (I verified they are the 18 ANVIL configs on `synth-arith.bin`, 28 of 36 row-plane cells).

---

# §11. ADDITIONS TO THE QUEUE INVARIANTS

- **INV-20 — No GHA timing cell is a crossing.** GHA throughput is **scout/ranking-grade**.
  A `FRONT-CROSSING` may be emitted only from an axis that is citation-grade (bytes, ratios,
  hashes) or from an **independently replicated** speed endpoint. Preselected primary endpoints
  **or** a family-level max-statistic/null procedure; **all cells reported**; replication =
  same commit SHA, same corpus SHA-256, same tooling blob, **different runner**, replicate CI
  containing the original estimate. **Shrink the family before correcting for multiplicity.**
- **INV-21 — FRONT-GAP requires both axes to be citation-grade.** A non-dominance margin on a
  ranking-grade axis below that cell's own measured noise floor is **not admissible** as a
  bracket; report it as substrate-limited. Add an explicit inconclusive class to the vocabulary,
  or gate FRONT-GAP assignment on PR-4 §3b citation grade.
- **INV-22 — One ε, derived, not inherited.** A bracket epsilon must be **derived from the
  substrate's measured noise floor** (`tests/noise-floor.csv`) and frozen in the job text
  **before** the artifact is opened. The same constant may not serve as both a practical-effect
  floor on one axis and a bracket-inscription margin on another. **ε = 0 is not neutral** —
  preserving 0 crossings by retaining an ε-free predicate is itself a threshold choice.
- **INV-23 — Reference-subset conditioning is disclosed.** Any `DOMINATED` count is reported with
  its reference subset named, plus a leave-one-reference-out sensitivity table. `435/468` without
  that disclosure is not citable.

---

# §12. RULING (amended)

> ## **HOLD** — queue-wide, unchanged in direction, with **one agreement and two disagreements**
> recorded against `Q0-DENSE-RETAINED-RESULT.md`.
>
> **AGREE:** **Q2 is REQUIRED.** I reached it independently by a different route before Q0
> existed, reproduced Q0's `435/28/5/0` tuple exactly from an independent implementation with a
> matching input digest, and endorse all nine of Q0's stated preconditions plus the coordinator's
> timing rule as binding.
>
> **DISAGREE 1:** Q0 is **mis-scoped for Tier 0** — its purpose (candidate-geometry sweep) cannot
> be met without running an encoder. Split Q0-static (free, genuinely retained-artifact) from
> Q0-encode (byte-only encoder runs).
>
> **DISAGREE 2:** Q0's decisive result **overstates its novelty** (it reproduces a frozen ledger
> tuple) and **understates its own strongest diagnostic** (reference-internal crossings ⇒ the
> `DOMINATED` count is grid-conditional). And its headline tolerance is wrong in *direction*: the
> five live cells are held out of FRONT-CROSSING by a **0.9 % ratio margin** and a **7.8 % encode
> margin that sits inside the project's own measured dispersion**, on a ranking-grade window.
> **The honest retained tuple is `435 | 28 | 5* | 0` with `*` = substrate-limited**, which
> strengthens the case for Q2 and changes Q2's success criterion.
>
> **Cheapest decisive next step, still no runner:** the **leave-one-reference-out sensitivity
> table** over the 10 retained reference arms (§10.4), plus recomputation of `MAD(log_ratios)`
> from any retained `raw.csv`. Both are retained-artifact arithmetic. Together they decide
> whether the project's only five non-dominated cells are a measurement or a margin.

**Falsification criteria for this section, stated so I can be wrong in public:**
1. If the retained window's pinned-core pre-window utilization was actually **< 5 %** — i.e. if
   the frozen suite *is* citation-grade for throughput — then the 7.8 % margin is admissible,
   the five FRONT-GAP labels stand, and my §10.3 challenge collapses. I take the ranking-grade
   label from `tests/suite-frozen-bdc90474.md`; **I have not read that file's utilization record
   directly**, and it is the single load-bearing input of my challenge.
2. If `parity_sweep.py` (which I did not read) defines FRONT-GAP by a rule materially different
   from R-2, my reproduction's agreement at 435/28/5/0 could be coincidence. The exact match on
   468 cells with 5 identified cells and 28 identified DEGENERATE cells makes coincidence
   unlikely, but it is not excluded.
3. If a leave-one-reference-out table shows all five labels robust to removing `xz-9e`, then
   §10.3's claim that `xz-9e` carries the load-bearing margin is wrong, though the
   ranking-grade objection still stands on its own.

# §8. RULING — as first issued (SUPERSEDED by §12 after the queue review and Q0 audit)

> ## **HOLD — at the queue level, with one carve-out**
>
> **HOLD** all Track 01–20 measurement dispatches that transit a broken axis (§3): the
> block/window-parity-exposed tracks (01, 07, 09, 15), the timing-estimand-exposed tracks
> (02, 05, 06, 11, 12, 16, 17), and every track whose ratio claim rests on a Class-A row for
> an input > 256 KiB. **HOLD** the 20-arm dense dispatch.
>
> **Carve-out — PROMOTE-TO-REMOTE one narrow pilot**, and it is not a crossing hunt: the
> six-arm corrected pilot with the frozen schema in my Track 04 report §A1.2, whose success
> criterion is **instrument validity** (`overhead_share ≤ 5 %`, dispersion clears the MAD
> floor, parity byte-delta reported), explicitly **not** "a crossing appeared". A crossing is
> not what this instrument can currently deliver, so a pilot gated on a crossing is
> unfalsifiable-by-construction.
>
> **KILL, now, met:** any queue plan whose success criterion is "N of M tracks confirm X"
> without INV-6 and INV-12 — consensus across 20 tracks through one instrument is not
> evidence, it is one measurement with 20 selection filters.
>
> **Cheapest decisive next step, no runner required:** for every currently-published ratio or
> speed claim in the swarm, run the two-question audit in §7 (INV-6, INV-7). That is pure
> arithmetic over artifacts already on disk, and it converts the queue from *apparently
> 20-times-confirmed* to *known-count*.

**Falsification criteria for this report, stated so I can be wrong in public:**
1. If any track already applies Holm/BH **and** publishes all cells including nulls, §0's
   expected ~20 false-win yield collapses and INV-6 is satisfied in practice — my headline
   is then overstated, though every axis audit in §1 stands on its own.
2. If `tools/bench_native.cpp` sets `o.block_size` somewhere I did not read **and** the
   default exceeds the reference window, the §2.5 parity defect is void and INV-7 is
   unnecessary. I read the file's `Options` construction at `:25` and the reference window at
   `:39,43`, and `anvil.cpp:3769` fixes the default at 256 KiB, so I regard this as closed —
   but it is one unread file section away from falsification and I say so.
3. If `paired_bench.py`'s pinned blob contains an in-process timing path I misread, §2.3's
   *magnitude* collapses; its *structure* (a common-mode additive term that A/A cannot see)
   would still stand.
4. If a track's headline claim is byte-only on inputs ≤ 256 KiB with all cells published and
   no selection, my audit does not touch it — and I would rather that be the common case than
   pretend otherwise.

**Boundaries honoured:** no existing file modified; no commit, push, reset, clean, stash,
restore, or rebase; no local corpus benchmark, sweep, or fuzz campaign; no corpus bytes read;
no codec invoked. This file is additive only.