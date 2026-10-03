# Track 04 — Dense Pareto Measurement · Fledge Alpha Free (INDEPENDENT ADVERSARIAL REVIEW)

**Date:** 2026-10-02
**Agent:** Fledge Alpha Free, independent adversarial reviewer, track `04-dense-frontier`
**Status:** FINAL. Evidence reconstructed from source and frozen artifacts **before** reading
`04-dense-frontier-space-bunny.md`; reconciliation in §5.
**Writes:** this file only. No existing file modified. No commit/push/reset/clean/stash/restore/rebase.
No local corpus benchmark, sweep, or fuzz campaign. One isolated static linter under
`prototypes/swarm-2026-10-02/04-dense-frontier/fledge/`.

**Labels used throughout — never blurred:**
**[M]** measured/fact, with artifact I verified myself · **[A]** arithmetic on [M], re-checkable · **[H]** hypothesis · **[U]** unverified.

---

# §0. RULING

> ## **PILOT**
>
> **One ruling, one token.** The dense-frontier vehicle **as frozen must not be
> dispatched** (it is not a measurement of the thing it claims to measure). But its
> purpose is sound, one live hypothesis deserves a decisive test, and every defect I
> found is correctable without touching codec source. Therefore: **reject the frozen
> 20-arm dispatch; authorise exactly one corrected pilot job** (§10), whose first
> pre-registered step is the **block/window parity control** the coordinator raised.

**Cheapest decisive next step (§10.1, in order):**
1. **Free — retained-artifact only, no runner:** re-run the reference-density sweep with
   the **candidate-side configuration as the swept variable instead of the reference set**,
   and re-run it with the FRONT-GAP bracket predicate actually implemented. This alone
   decides whether the constructive lane's load-bearing "crossing is unreachable" result
   survives. No benchmark. Arithmetic over one existing CSV.
2. **If and only if (1) returns "crossing still unreachable":** one remote pilot job,
   6 arms × 2 panel files × 2 planes × 13 reps, in-process timing, parity-controlled.
   Everything else in §10 is deferred.

---

# §1. What I actually verified, and how

I did not take any number from the constructive lane. Sources read directly:

| Source | What I took from it |
|---|---|
| `docs/swarm-2026-10-02/MASTER-BRIEF.md` | doctrine, deliverable contract, 20 tracks |
| `docs/I10-DENSE-FRONTIER-PREREG.md` (262 L) | frozen arms, thresholds, gates, claim boundary |
| `.github/workflows/anvil-i10-dense-frontier.yml` (1027 L, **untracked**) | the instrument, line by line |
| `tools/paired_bench.py` @ pinned blob `f847c50e…` | the timing driver, line by line |
| `tools/pareto_front.py` | the project's arbiter of record |
| `tests/pr-4-measurement-window-protocol.md` | binding measurement doctrine (PR-4 v1.2) |
| `docs/gate-ruling-i8-pareto-win.md` | R-2 FRONT-GAP / DEGENERATE definitions |
| `docs/I10-REMOTE-BASELINE-CLOSURE.md` | provenance of the frozen reference byte totals |
| `docs/I10-AUX-UNBWT-RESULTS.md` | the one live ratio/decode hypothesis |
| `src/anvil.cpp:4845–4894`, `:4933`, `:5023–5026` | decode memory model, block self-containment, in-process timing pattern |
| `tools/bench_native.cpp:25,39,43` | the Class-A instrument configuration |
| `docs/swarm-2026-10-02/15-crossblock-memory-fledge.md` §0.1–§0.3 | the Class-A asymmetry claim (verified where I could; see §4) |
| `docs/swarm-2026-10-02/04-dense-frontier-space-bunny.md` | read **after** my own reconstruction, for §5 only |

**Correction I made to my own analysis, recorded because it is the method working:** I first read the closure table's columns backwards and concluded the vehicle's hard byte gates were guaranteed to fail (zstd off by 158,103 B, xz off by 96 B). That was **wrong**. The columns are `Linux remote` vs `Frozen I9`; the vehicle froze the `Linux remote` column. The gates are correct. I withdrew the finding before it reached a draft. It is recorded here because a reviewer who ships that error would have declared a healthy instrument dead.

**Isolated artifact (mine):** `prototypes/swarm-2026-10-02/04-dense-frontier/fledge/instrument_audit.py`
— a **static** linter. It parses workflow text, classifies each arm's *measurement instrument*,
and computes the dispatch budget. It runs **no codec, no corpus, no timer**. It cannot say
whether a number is fast; it says whether two arms were measured with the same instrument.
Two bugs in my own first version (branch extractor mis-scoped; two literal ANVIL arms
dropped from the count) were found by running it and fixed; the reported numbers below are
post-fix and re-run.

---

# §2. AUDIT RESULTS THAT SURVIVE — the instrument's genuinely good parts

These are **[M]**. A HOLD/PILOT verdict on a well-built instrument must say what is right.

**S1 — The byte axis is correctly sourced, and I proved it independently. [M]**
Summing the prereg's 12 frozen ANVIL legacy per-file values:

```
2,571,873 +13,806,173 +2,382,322 +1,365,712 +2,478,889 +2,584,657
+1,144,032 +3,761,931 +4,586,126 +7,317,361 +4,017,320 +430,599  =  46,446,995
```

which is **exactly** the frozen `ANVIL auto-direct` total in
`docs/I10-REMOTE-BASELINE-CLOSURE.md:21`. enwik8 legacy 23,534,368 also matches `:25`.
The reference totals (49,383,136 / 52,364,240 / 48,456,004 / 24,810,180 / 25,272,471 /
24,831,648) are the closure's `Linux remote` column. **Provenance of every hard byte gate
in the vehicle checks out.** I set out to break this and could not.

**S2 — External reference bytes are demonstrably toolchain-fragile. [M]**
Closure table, `Frozen I9 → Linux remote`: zstd-22-long27 **−158,103 B** (Silesia) and
**−61,224 B** (enwik8); xz-9e −96 B / −8 B; Brotli q11/lw30 **0 / 0**. So zstd's output on
the *same inputs* moved ~0.6% across toolchain versions while Brotli's did not move at all.
This is not an ANVIL defect — it is the reason the vehicle pins every package version, and
the pin is correct. It is also the reason §3-F9 exists.

**S3 — Identity pinning is real, not decorative. [M]**
`git cat-file -t` confirms `b6a243c9…` (tooling) and `0dce534e…` (candidate) both exist as
commits and `755df76a…` exists as a blob; tag `i10-aux-unbwt-final-v1` exists; the vehicle
checks 12 blob identities, asserts all three checkouts are clean, and `test "$(nproc)" = 4`.
`TOOLING_PAIRED_BLOB = f847c50e…` is the **HEAD blob** of `tools/paired_bench.py`
(`git rev-parse HEAD:tools/paired_bench.py`). **So the 183-line dirty working-tree copy
(`bb2b2a7e…`) is not what runs** — the vehicle is insulated from the live dirty driver.
This is correct and should be preserved.

**S4 — Output-freshness discipline is real and outside the timed interval. [M]**
`observe.sh` is generated, `rm -f`s the output, verifies absence, then `exec`s
(`workflow:467–508`); the driver re-hashes after every single repetition
(`paired_bench.py` `post_run_observation`); the workflow independently re-verifies every raw
row (`verify_raw`) and every final observation **outside** any timed interval, and
`--expected-sha256` is deliberately *not* passed so hashing never enters a timed region.
Stale-output contamination — the classic CI failure — is genuinely closed here.

**S5 — Round-trip and determinism are hard gates, not best-effort. [M]**
Every cell decodes and is SHA-256-compared to source; a payload that changes hash or size
across any repetition raises; missing/duplicate cells raise.

**S6 — Bootstrap quantile convention is correct. [M]**
`lo_i = int(0.025·20000) = 500` → 2.5001%; `hi_i = int(0.975·20000) = 19500` → 97.5049%.
No off-by-one. (I had this as a candidate defect; checked, **withdrawn**.)

**S7 — MAD degeneracy at n=7 is real. [M]** `[1,1,1,1,1.4,1.9,3.0]` → median 1, deviations
`[0,0,0,0,.4,.9,2.0]`, MAD **0.0**, `robust_cv` **0.0000**, gate `<= 0.15` **PASSES with a
3× outlier present**. MAD is the 4th order statistic of 7 deviations; ≥4 coincident values
force MAD=0. Independently confirmed by hand, no code run.

---

# §3. DEFECTS — ranked by ability to change a verdict

### F1 — **CRITICAL** — `decode_peak_kib` is harness I/O architecture, not decoder cost, and it sits inside the dominance predicate

`dominates()` puts **both** RSS axes in the predicate (`workflow:891–892`), and
`descriptive_class` keys off `dominated_no_binary`, which still contains both (`:913`).

What actually determines those numbers, from source:

- **ANVIL**: `read_file()` returns the whole file as a `vector<uint8_t>`
  (`anvil.cpp:4933`) and `decompress` pre-reserves the full declared output:
  `reserve_hint = min(total, max(1 MiB, in.size()*256))` then `out.reserve(...)`
  (`anvil.cpp:4859–4860`). So ANVIL decode peak RSS **≈ packed + original**, by construction.
  For mozilla: `13,806,173 + 51,220,480` ≈ **65.0 MB ≈ 1.28× original**, deterministically.
- **Brotli** (dense vehicle): `std::vector<uint8_t> buffer(expected)` — same shape, ≈ same number.
- **zstd CLI** (`zstd -d`, `workflow:521,528`): streams, ~window-sized.
- **zstd-22-long27 decode**: `--long=27` ⇒ **windowLog 27 = 128 MB** decoder buffer.
- **xz-9e**: 64 MiB dictionary.

**So the decode-RSS ranking is decided by (a) whole-buffer vs streaming harness and (b)
window settings — not by decoder efficiency. Measured RSS will show zstd-1..19 best by
roughly an order of magnitude purely because it streams while the other two harnesses
buffer.** Any `DESCRIPTIVE_NON_DOMINATED` verdict on the decode plane is contaminated by a
factor that says nothing about compression.

**There is a real finding buried inside the artifact, and the vehicle mislabels it.**
ANVIL has **no streaming decode entry point**: the API is
`decompress(const vector<uint8_t>&, const Options&) -> vector<uint8_t>`. Decode RSS is
therefore **Θ(input + output) by design**, against zstd/xz/Brotli streaming APIs. That is a
genuine competitive cost — but it is a *product-API* property, not something a harness can
attribute, and the vehicle charges it as if it were a per-cell measurement.

**Correction.** Never let raw RSS enter a cross-family dominance test while families differ
in buffering discipline. Charge `resident_charge_kib = peak_kib − (input_bytes + output_bytes)/1024`,
report raw RSS descriptively, and promote the *API* fact to a format-track requirement
(track 19/18), not a bench cell.

### F2 — **CRITICAL** — the binary-size axis is not comparable by construction

`size_key` maps anvil/brotli → `decoder-linker-gc-harness` and zstd/xz → `system-cli`
(`workflow:873–883`), then `dominates(..., include_binary=True)` compares those stripped
bytes directly (`:895`). Prereg §9 explicitly forbids exactly this
("not treated as a directly comparable crossing axis"), and `summary.json` then sets
`binary_size_comparability: "descriptive-only"` — **a direct prereg↔code contradiction**
(`prereg:228` vs `workflow:912,945`).

The comparison is biased in **both** directions, so it is uninformative rather than merely
pessimistic: Brotli's decoder-only harness (libbrotlidec only, gc-sections) will look ~10×
smaller than ANVIL's decoder harness, while ANVIL's will look smaller than the full
multi-level zstd CLI and the all-filters xz CLI. A column named `dominated_full_cost` will
be decided largely by this contest. There is **no zstd or xz decoder-only measurement at all**
in the vehicle.

**Correction.** Delete `dominated_full_cost`, or restrict it to the one matched-scope pair
(anvil ↔ brotli decoder harnesses) and rename it `decoder_size_paired`. Add decoder-only
zstd/lzma harnesses built with the *same* flags, or admit the axis does not exist.

### F3 — **HIGH** — the paired design's CI is computed and then **never used** for the frontier test

This is my finding and the constructive lane does not have it. Inside every cell, control is
`anvil-legacy` in the *same driver invocation* and the driver returns a bootstrap 95% CI on
the paired log-ratio. That is the only drift-robust quantity the run produces. But the
classification step computes the rate axis as
`plane_mbps = source_bytes / candidate_median_s` (`:851`) — **the candidate's own absolute
median from its own invocation** — and `dominates()` compares those absolute MB/s across
arms (`:885–893`). Each arm was measured in a **separate driver process**, in a shuffled but
sequential order.

So: pairing is used to *gate* a cell, and ignored to *rank* it. Cross-invocation drift
(thermal, THP state, host neighbours on a shared GHA VM, page-cache state) becomes a direct
confound on frontier membership, and the permitted gate — **robust CV up to 15% per arm** —
is wider than the encode-rate gaps between adjacent arms. Arms 14% apart in true rate can be
ordered either way by a legal run.

**Correction.** Rank on the ratio-of-ratios scale that is already in `paired.csv`:
`log_rate_rel(arm) = −ln(paired_ratio(arm))` measured against the *same* control in the
*same* invocation. Never compare absolute MB/s across driver invocations.

### F4 — **HIGH** — common-mode additive process cost contaminates the plane that matters most

`plane_mbps`'s denominator is Python wall-clock around `subprocess.run`
(`paired_bench.py` `run_once`), and **every** timed invocation is routed through a generated
bash wrapper that `rm -f`s then `exec`s (`workflow:467–508`).

```
T_measured = T_wrapper_bash + T_fork + T_dynlink(arm) + T_codec(input)
```

Order-of-magnitude **[H/P] estimate** using Linux figures from `docs/CONTEXT.md`
(Brotli decode ~1,000–1,400 MB/s; wrapper+exec ≈ 5–7 ms):

| panel file | source B | est. decode signal | est. overhead | contamination |
|---|---:|---:|---:|---:|
| xml | 5,345,280 | ~4–5 ms | ~5–7 ms | **>100 %** |
| dickens | 10,192,446 | ~8–9 ms | ~5–7 ms | ~70 % |
| nci | 33,553,445 | ~25–30 ms | ~5–7 ms | ~20 % |
| webster | 41,458,703 | ~30–35 ms | ~5–7 ms | ~17 % |
| mozilla | 51,220,480 | ~40–45 ms | ~5–7 ms | ~14 % |

The decode plane — **the only plane where a live hypothesis exists** — is 14–>100 %
contaminated on 4–5 of 5 panel files. A fixed additive term also compresses every ratio
toward 1, i.e. the vehicle is **least sensitive exactly where the frontier is densest**.
Encode-plane contamination is comparable for `xml` at q1/zstd-1.

**And the A/A null cannot see it**, because the overhead is common-mode: both A/A arms pay
the same wrapper, so the null passes cleanly while the estimand is wrong. *A/A validates
noise; it does not validate the estimand.* The vehicle has no timing-degeneracy class at
all — its only degeneracy guard is `DEGENERATE_RATIO` at `ratio >= 0.95`, hard-coded at
`:913`, **absent from the prereg entirely** (a frozen-threshold violation of brief §5), and
dead code on these corpora (ratios 0.219–0.248).

**Correction.** Add `DEGENERATE_TIMING_SHORT` with a pre-registered overhead floor, and
report the raw and de-trended rate (§9 gives the cheap fix that already exists in-tree).

### F5 — **HIGH** — one bad cell out of 200 invalidates the whole job

`summary.json` status is `BLOCKED_TIMING` if `timing_failures` is non-empty, and the step
then `raise SystemExit(2)` (`workflow:935,948–950`). `timing_failures` is appended for
per-arm robust CV > 0.15, missing rows, and any A/A cell whose bootstrap CI fails to bracket
1.0 or which spuriously "clears" the speed gate (`workflow:826–848`). That is
**one fatal cell in a 200-cell Silesia run** or a 40-cell enwik8 run.

The A/A predicate is a bare point decision with no multiplicity control and no confidence
interval attached to the *decision*, taken 10× (Silesia) or 2× (enwik8) times; the per-arm CV
threshold of 15 % is 2.5× looser than the PR-4 §1.1 noise-floor discipline the rest of the
project holds itself to. On a hosted VM this is a coin flip, and the failure mode is
fail-closed — *safe*, which is why this is a PILOT/HOLD issue and not a correctness scandal.
The project has already paid this once: the ledger records an enwik8 arm ruled
`TIMING_BLOCKED` at control robust CV 0.1756, discarding a **10.15×** decode-ratio
measurement (cited by the constructive lane M15; I take the ledger as the artifact of record).

**Correction.** Treat individual cells as informative, not fatal: gate the *rate* of
admissible cells with a binomial CI, and reserve whole-run invalidation for
correctness/hash/identity failures only.

### F6 — **HIGH** — the mandated arbiter is not applied, and the tool the doctrine names does not exist

- `tools/pareto_verify.py` — the file PR-4 §4 item 2 mandates for the FRONT-GAP bracket test
  and the DEGENERATE guard — **does not exist** (`Test-Path` → `False`).
- `tools/pareto_front.py` has **no** FRONT-GAP, no bracket test, no DEGENERATE predicate and
  **no epsilon**: `dominated()` returns a dominator if `qr <= pr and qm >= pm` with *any*
  strict inequality. A reference that wins by 0.1 % — far below any noise floor — marks a
  row `DOMINATED`.
- The gate ruling is explicit that the bracket classification "is the gate's reading of that
  boolean and is applied by this lane, **not by the tool**"
  (`docs/gate-ruling-i8-pareto-win.md:279–281`). So FRONT-GAP is an **unenforced manual step**.
- The dense vehicle therefore automates a **third** arbiter — inline `dominates()` with 4–5
  axes — that is neither `pareto_front.py` nor the doctrine.
- The artifact contract lists `frontier-inputs.csv`, but `pareto_front.py` reads columns
  `codec` / `ratio` / `encode_MBps` / `decode_MBps` / `input_bytes` / `compressed_bytes`,
  while the vehicle emits `arm` / `plane` / `plane_mbps`. **The hand-off cannot execute.**

**Correction.** Either make the vehicle emit `pareto_front.py`'s schema and *run* it, or
freeze the inline arbiter as canonical and implement the adopted bracket predicate in code
with a pre-registered epsilon. Two incompatible arbiter definitions must not coexist.

### F7 — **MEDIUM** — no joint (ratio, encode, decode) dominance anywhere

The vehicle computes dominance **per plane** (`workflow:907–914`). Doctrine requires a row to
be non-dominated on **both** planes before it counts (`RESEARCH_LEDGER.md:4156`). So the
positive-sounding label `DESCRIPTIVE_NON_DOMINATED` is emitted for single-plane rows that
doctrine classifies as not a Pareto win. The constructive lane's §9.3 measures **0 of 5**
canonical non-dominated rows are jointly non-dominated — i.e. this vehicle would relabel
**every** one of them. Independently confirmed by me as a structural property of the code.

### F8 — **MEDIUM** — the A/A null exercises only one observation path

For `arm == "anvil-legacy"`, control and candidate are the *same* `manifest` dict entry
(`workflow:712–714`), so `--control-observe == --candidate-observe`: one path, one expected
hash. The A/A null therefore validates the **noise floor** while exercising **no**
two-distinct-path freshness handling. A dual-path defect in the observation plumbing — the
exact class the vehicle exists to prevent — passes A/A. Cheap fix with no new arm: an
`A/A′` cell pairing `brotli_dense 11` against the frozen `brotli_lw` helper, which the
vehicle **already proves byte-identical** (`workflow:599–610`) — different argv, different
binary, identical output.

### F9 — **MEDIUM** — 16 of 20 arms have no byte anchor, on an axis proven fragile by S2

My linter: only **3** arms are hard-gated (`brotli-q11-lw30`, `zstd-22-long27`, `xz-9e`)
plus the 2 ANVIL arms via the per-file table. The other **16** — brotli q1/q4/q6/q9 and
zstd 1/3/5/7/9/11/13/15/17/19/22 — have **no pre-existing byte total**; prereg §6 says so
openly and calls their first-run totals "new deterministic evidence". But S2 shows zstd's
bytes on identical inputs moved **−158,103 B** across toolchain versions. So the axis a
crossing claim most depends on is exactly the axis with no independent verification, on the
one family whose output is least stable. Silent re-derivation of a frontier.

**Correction.** Publish SHA-256 of every arm's packed output in a frozen baseline so a
re-run *detects* toolchain drift instead of silently producing a different frontier.

### F10 — **MEDIUM** — affinity recorded, never enforced; census and timing are not the same measurement

`set_affinity()` returns `None` when `sched_setaffinity` is unavailable and the driver
proceeds **unpinned** — while prereg §3 states "a missing or unsuccessful affinity setup is
invalid timing evidence". Nothing enforces `== {cpu}`. Separately, the RSS census measures
the **unwrapped** command (`workflow:558–559`) while timing measures the **wrapped** one, so
the RSS and timing columns come from different command constructions — and the census runs
**unpinned** while timing runs pinned, undeclared. Also: the driver's `env` is
`os.environ.copy()` with `--env-json` defaulting to `{}` and the workflow never passes it,
so ANVIL env knobs (e.g. `ANVIL_STREAM_SLACK`, which CONTEXT.md documents as trading rate
for speed) are inherited implicitly and asserted by nothing.

**Correction:** raise when `--cpu` is given and the result ≠ `{cpu}`; pin the census; pass an
explicit `--env-json` and record the child env for ANVIL arms in `arm-contract.json`.

### F11 — **MEDIUM** — the xz arm pays two bash spawns; nobody pays one

`xz-9e` is the only arm routed through `bash <script>` (`workflow:530–536`), on top of the
`observe.sh` bash every arm pays. So xz is the single arm carrying **two** extra shell
processes inside its timed interval and using shell-redirect I/O (`-c "$1" > "$2"`) instead
of the program's own `-o` path. It is a one-sided systematic handicap against exactly the
arm that currently holds the best Silesia ratio (48,456,004 B). On `xml`/`dickens`-scale
cells that is ~10 % — the same order as F4.

### F12 — **LOW–MEDIUM** — contract/code mismatch inside the artifact itself

`arm-contract.json` records `"anvil": {"legacy": "auto-direct", "aux": "auto-direct+bwt-aux"}`
(`workflow:653`) while the executed command is
`--parse=ratio --ratio-backend=auto --ratio-context=off --ratio-lines=off` (`:514–516`).
The artifact's own machine-readable contract misdescribes the arm it certifies.

### F13 — **LOW** — `/usr/bin/time %M` is a max over waited children

`ru_maxrss` is a **max**, not a sum. Correct today (single-process arms), but `xz` runs under
`bash`, so its max is over {bash, xz}; and it becomes wrong the moment any arm spawns a
worker thread with its own arena. Fragile, not currently wrong.

### F14 — **LOW** — dispatch budget, measured from my linter

| | Silesia | enwik8 |
|---|---:|---:|
| timing panel coverage of corpus bytes | **66.89 %** (5 of 12 files) | 100 % (1 of 1 file) |
| paired driver invocations | 20 arms × 5 files × 2 planes = **200** | **40** |
| codec invocations (2 warmup + 7 reps × 2 planes) | **1,620** | **324** |
| census codec invocations | **216** | **18** |
| raw timing rows emitted | **2,520** | **504** |

The 360-min cap looks comfortable on my estimate, but `xml` is both the fastest cell (most
overhead-sensitive, F4) and the smallest — and the panel **omits the 7 largest Silesia
files**, including `samba` (21.6 MB) and `osdb` (10.1 MB). Correct per prereg §10.3
(bytes/RSS census is full-corpus), but it means the *timing* claim covers two-thirds of the
corpus and the *ratio* claim covers all of it.

---

# §4. COORDINATOR BLOCKER — Class-A block/window parity: **VERDICT = CONFIRMED as a reference-cost parity defect**

**Claim to verify:** "ANVIL native grid default 256 KiB blocks vs Brotli reference
whole-file/lgwin22, with measured E4 warmup tax up to +16.98 %."

**What I verified myself, from source:**

1. **Blocks are self-contained — the block size *is* the memory horizon. [M]**
   `src/anvil.cpp:4868–4890`: the serial decode loop calls
   `decode_one_block(mode, p, plen, blen, expected_crc, revision, block_size, e, opt)` and
   `decode_one_block_into(mode, p, plen, dst, blen)`. **Neither receives an
   already-decoded-history pointer.** `dst` is the current block's own output slot. So a
   match can reference only bytes inside its own block, and `block_size` is passed *in*,
   consistent with bounding distance codes. This is the structural precondition for the
   claim, and it is why "cross-block memory" and "cross-block model warmup" are both
   functions of one number.
2. **The Class-A instrument never sets it. [M]** `tools/bench_native.cpp:25`:
   `anvil::Options o; o.parse=parse; o.literal=lit; o.entropy=entropy; o.quiet=true; …`
   — **`o.block_size` is never assigned**, and `parse="ratio"` is never used (`:65–67` pass
   `"sparse"`). So Class-A ANVIL rows run at whatever the `Options` default is, while the
   same file's Brotli reference is pinned to `BROTLI_DEFAULT_WINDOW` (`:39,43`) = **lgwin 22
   = 4 MiB**, one whole-file stream.
3. **Therefore Class A compares a candidate at the default block size against a reference
   at a 4 MiB window, over whole files. [M]** The direction and the mechanism of the defect
   are established independently of any constant.

**What I did NOT verify (and will not assert):**
- the exact **`256 KiB` default constant** — I did not read the `Options` initialiser line;
  I take the value from the Track 15 critic's citation and mark it **[U]**.
- the **`+16.98 %`** E4 figure — that lives in
  `docs/audit-2026-09-07/13-e4-block-routing-oracle.md:26–46`, which I did **not** open.
  **I mark +16.98 % [U].** I did verify the arithmetic shape of the table it belongs to is
  plausible (`5,424,162 / 31,950,395 = 16.98 %` **[A]**, so the number is internally
  consistent with the stated base — but internal consistency is not provenance).
- that the dense vehicle is at parity. **It is not, and this is my own finding:**
  the vehicle's Brotli arm is `BrotliEncoderCompress(quality, **30**, …)` — **lgwin 30
  ≈ 1 GiB** (`workflow:291`) — against ANVIL at `--parse=ratio`, which per the Track 15
  critic's Class-B row is 128 MiB. So **Class B is also asymmetric, ~8×, not parity.**
  The critic's §0.3 Class-B "at parity" claim is **wrong**, and I say so against my own
  side's sibling report.

**Ruling: CONFIRMED — this is a reference-cost parity defect, not a codec limit.**
Class A charges the candidate a whole-file-blocking deficit the reference never pays, on
precisely the multi-block files where that deficit is largest. It is a
**measurement-configuration artifact**, not a capability limit.

## §4.1 Which Class-A conclusions still stand

| Conclusion | Stands? | Why |
|---|---|---|
| Byte totals are correct and reproducible | **YES** | deterministic; a window handicap changes *which* bytes, not whether the measurement is real (S1) |
| Decode is slower than Brotli | **YES** | the rate axis is unaffected by block size; and F4 makes the magnitude suspect, not the direction |
| Decode RSS deficit (Θ(input+output) API) | **YES** | independent of block size (F1) |
| ANVIL is smaller than Brotli on Silesia / enwik8 | **YES, and strengthened** | 46,446,995 vs 49,383,136 and 23,534,368 vs 24,810,180 — ANVIL wins the ratio axis **while carrying a 256 KiB / 128 MiB handicap the reference does not pay**. The handicap is a *headwind ANVIL is overcoming*, so it inflates, not deflates, this claim. |
| TCOPY / PNRA "does not yet beat q4 on .text" | **CONDITIONAL** | these are long-horizon mechanisms; a 256 KiB cap **systematically truncates exactly the displacement/temporal-horizon structure they mine**. A no-go at 256 KiB does not transfer to lgwin-30 parity. **Re-test at parity before any kill.** |
| PORDER as standalone novelty killed | **NOT a ratio claim** — unaffected (killed on prior art, brief §13) | — |
| "Crossing is unreachable at any reference density" | **NOT ESTABLISHED — see §5.2** | the sweep varied the *reference* set while holding the candidate handicap fixed |
| Parser-cost / MDL conclusions from Class-A encode rate | **YES for direction**, magnitude suspect (F4) | additive overhead compresses ratios toward 1 |

## §4.2 Required control before Class A may kill anything on bytes

> **Paired block-parity control.** In one job, same panel files, same reps, same
> control/candidate machinery: encode ANVIL at **both** the instrument default **and**
> `--block=` raised to the reference's window (mode supports up to 128 MiB,
> `anvil.cpp:4850`), against Brotli at the *same* window the reference arm uses.
> **Report the byte delta per file.** Any Class-A "mechanism M does not help on bytes"
> verdict is **void** unless M was evaluated at parity. This is one extra arm, not a
> rewrite.

---

# §5. RECONCILIATION with `04-dense-frontier-space-bunny.md`

I read it after my own reconstruction. **We agree on 9 of 11 substantive findings**,
independently. Where we agree I confirm rather than restate; the value is the two places we
diverge, one of which can change the verdict.

## §5.1 Agreement (independently confirmed — I do not defer to the analysis, I re-derived it)

| Their finding | My status |
|---|---|
| F1 RSS axes are buffering artifacts inside the predicate | **CONFIRMED**, and I add the exact mechanism and the `packed+total` arithmetic from `anvil.cpp:4859–4860` |
| F2 mixed binary-size scopes | **CONFIRMED** (`:873–883`, `:895`) |
| F3 no joint-plane dominance | **CONFIRMED** structurally; their §9.3 measurement (0/5 joint) is consistent |
| F4 additive process cost, least sensitive where densest | **CONFIRMED**, with my own contamination table |
| F5 `descriptive_class` still carries both RSS axes | **CONFIRMED** (`:913`) |
| F7 CV gate screens the wrong statistic; MAD=0 passes a 3× outlier | **CONFIRMED** (S7) |
| F8 A/A null is single-path | **CONFIRMED** (`:712–714`) |
| F9 affinity recorded not enforced; census unpinned | **CONFIRMED** |
| F10 undeclared `DEGENERATE_RATIO` threshold in the arbiter | **CONFIRMED** — and it is a brief §5 violation |
| bootstrap quantiles correct (their V2 withdrawal) | **CONFIRMED** (S6). Two independent withdrawals of the same non-defect is the correct outcome. |
| H-ENVELOPE refuted 0/5 | **Not re-run by me.** Their pre-registered refutation branch fired as written and the negative result is load-bearing for *their* proposal. I neither rely on nor dispute it. |

## §5.2 **DISAGREEMENT THAT CAN CHANGE THE VERDICT** — their §9.2 is a one-sided sweep

**Their claim:** `mean_crossing = 0.000` at k=9 and k=10 over 400 random reference subsets;
"a `FRONT-CROSSING` on this corpus is unreachable at any reference density"; the vehicle's
purpose (GRID-THIN removal) "is already achieved by the existing grid". They correctly flag
**U6** (corpus dependence) as able to flip the recommendation.

**My objection — stronger and different:** the sweep varies the **reference set** and holds
the **candidate configuration fixed at one single value**. Per §4, that one value is the
Class-A default block size against a whole-file lgwin-22 reference. So the sweep proves:

> "varying which references are in the class does not produce a crossing"

and says **nothing** about

> "the candidate is not handicapped relative to the reference it is losing to."

Those are different claims, and only the first is a property of the *frontier*. A
candidate that is +16.98 % [U] worse on bytes than its reference would lose every dominance
comparison at every reference density **by construction** — the sweep would report
`mean_crossing = 0.000` and it would be **true and irrelevant**. The subspace swept
(density of the reference set) does not contain the axis along which the candidate is
confounded (window/block parity).

This matters because their recommendation is built on §9.2 being the **strongest** argument:
"this is stronger than 'unproven' — it is a measurement that the target is unreachable."
I accept the measurement and **reject the inference**. An unreachable *measured* target and
an unreachable *mis-measured* target are indistinguishable in that statistic.

Their own F-list does not contain this, because the defect is in the **substrate**, not the
arbiter — and no arbiter work can repair it.

**Resolution required before their §9.2 may be cited.** Re-run the sweep with the
**candidate-side block/window configuration** as an additional swept dimension (a 2-D sweep:
reference density × candidate window), against a Brotli arm at the *same* window. Pre-register:
if `mean_crossing` remains 0.000 across **both** candidate windows, §9.2 is upgraded from
"corpus-dependent" to "robust", and their recommendation stands unmodified. If any cell
crosses at parity, the dense plane regains priority **immediately** and their §5.2 minimal
matrix should be dispatched as-is.

**This is the cheapest decisive next step in this whole report: pure arithmetic over one
retained CSV, no runner, no benchmark.**

## §5.3 Where I am harsher than the constructive lane

Their verdict is *HOLD* + "downgrade the timing plane to margin measurement" + reallocate to
mechanism work. I go one step further on three points, because they each independently
invalidate a *cost-axis* verdict today, not merely a class label:

1. **F3 (paired CI never used for ranking)** is absent from their list. Their §5.2 drops
   arms but keeps the same cross-invocation absolute-rate ranking, so the matrix they propose
   would still rank arms by absolute MB/s measured in separate driver processes. **This must
   be fixed before dispatch, not after.**
2. **F5 (one fatal cell in 200)** is not quantified by them. Their matrix is *smaller*
   (14 arms × 4 files), which helps, but the fatal-per-cell policy is unchanged, so the
   invalidation probability — which is what matters — barely improves. Their §5.2 rep
   redistribution (7→9) raises the chance that a cell clears the gate; it does not lower the
   chance that some other cell fails it.
3. **F9 (16/20 arms unanchored on a toolchain-fragile axis)** is absent. It is the cheapest
   of all fixes (publish SHA-256 per arm) and is not in their promote-condition list (a)–(f).

## §5.4 Where I am softer than the constructive lane

Their §6.4 threat is the strongest thing in either report and I adopt it verbatim as a live
possibility: if `INVALID_SPEED_ACCOUNTING` (G4) means the speed axis is *permanently*
unusable on hosted runners, then **no dense Pareto timing run on Actions can ever support a
crossing claim**, and the vehicle's only defensible output is the byte/RSS census plus the
margin statistic. I cannot refute that with available evidence and I am not going to pretend
otherwise. It is why §10's pilot is scoped to *calibration*, not to a crossing hunt.

---

# §6. PRIOR-ART MAP — the measuring instrument is not novel either

Mandated for a measurement track: map the proposed/actual mechanisms against established
practice. Every element of this instrument is adopt-class, and several are *worse* than the
established practice they approximate.

| Element | Established practice | This vehicle | Verdict |
|---|---|---|---|
| Process-level A/B timing | Google Benchmark `DoNotOptimize` + in-process `steady_clock` around the operation, loop N iterations, **exclude process start and I/O**; Craig Wright's *Benchmarking* (2015) minimum-run-time rule | Python `perf_counter` around `subprocess.run`, including bash wrapper, fork, exec, dynlink, and file write | **Adopt-class and materially inferior.** The fix already exists in-tree: `src/anvil.cpp:5023–5026` times `decompress` internally with `steady_clock` and calls `write_file` *outside* the timed region. §9. |
| Warmup | discard ≥1 full iteration; report iterations-to-steady-state | fixed 2 alternating warmups, unverified sufficient | Adopt-class, **unvalidated** |
| Null control | A/A **and** A/B′ (two binaries, same output) to separate harness noise from treatment noise | A/A only, single observation path (F8) | **Incomplete** |
| Repetition/statistics | report min-run time, median-of-N, confidence interval; bootstrap CIs are standard | bootstrap CI on paired log-ratio: good; but robust-CV gate on the wrong statistic (F7) and no multiplicity control | Mixed |
| Multiplicity across a family of arms | Holm / BH within family | none | **Gap** |
| Memory accounting | report peak RSS **and** application-attributable working set; normalize for harness buffering | raw `%M` peak, mixed buffering families, used in the predicate | **Incorrect** (F1) |
| Binary/decode size | compare like with like; publish a decoder-only build policy first | linker-GC harness vs system CLI in one predicate | **Incorrect** (F2) |
| Streaming decoders | zstd/xz/Brotli all stream; RSS is window-bounded | ANVIL API is whole-buffer, Θ(in+out) | **Real product gap, misattributed to measurement** |
| Pareto classification | non-domination w.r.t. the reference front, with bracket/epsilon handling | two incompatible implementations, one unenforced by hand | **Gap** (F6) |
| Window/exclusivity protocol | one timing job at a time; pinned-core pre-window gate; ambient sampler | GHA 4-vCPU VM + concurrency group per corpus; ambient probe 5×250k **once per invocation**; census unpinned | Adopt-class, under-powered |

**The only genuinely new object in this track is a measurement *mechanism*.** The
constructive lane proposed one (ENVELOPE-dominance) and **pre-registered its own refutation**,
which fired (0/5). That is the correct scientific process and I record the kill rather than
quietly dropping it. There is nothing left to promote from the arbiter-rewrite direction.

---

# §7. HIDDEN-COST AUDIT — byte / cycle / memory accounting

## §7.1 Decoder-visible bytes on the wire

| Item | Silesia | enwik8 | Source |
|---|---:|---:|---|
| ANVIL legacy total (verified sum, S1) | **46,446,995** | **23,534,368** | prereg §6 |
| ANVIL aux total | **46,466,339** | **23,537,422** | prereg §6 |
| aux wire penalty | **+19,344 B** | +3,054 B | **[A]** per-file differences summed |
| aux penalty as % of corpus | **+0.00913 %** | +0.00305 % | **[A]** |
| Brotli q11/lw30 | 49,383,136 | 24,810,180 | closure |
| xz -9e | 48,456,004 | 24,831,648 | closure |
| zstd ultra-22 long27 | 52,364,240 | 25,272,471 | closure |
| **ANVIL margin vs best reference (xz)** | **−2,009,009 B (−4.15 %)** | −1,297,280 B (−5.22 %) | **[A]** |

**This is the single most important number in the track and it is fully determined.**
ANVIL's legacy configuration is **already 4.15 % smaller than xz -9e and 5.94 % smaller than
Brotli q11/lw30 on the full Silesia corpus** — deterministically, with hashes, citation-grade.
**The ratio axis is not ANVIL's problem; it is ANVIL's win.** And the aux arm buys
**1.36×–2.34× whole-codec decode** for **+0.0091 %** bytes
(`docs/I10-AUX-UNBWT-RESULTS.md` §3.2/§4.2/§5.2).

Put the two together honestly: **the aux-unBWT hypothesis is a real, large, cheap,
deterministic Pareto improvement — and `docs/I10-AUX-UNBWT-RESULTS.md:5` classes it
"adopt-class decoder engineering; no novelty claim".** So the crossing shape that exists
today is real *and* not novel. Track 04 must not let a large adopt-class win be reported as
mechanism novelty, and must not let "no novelty" be used to dismiss a real win.

**Cross-check worth keeping:** PR-4 §3d's index charge is **19,328 B** while the integrated
wire delta is **19,344 B** — different quantities (rounded per-file index vs total framing),
both correct. They must never be cited interchangeably; the 16 B gap is not a discrepancy.

## §7.2 Cycle accounting (the instrument, per timed sample)

```
T_sample = T_wrapper_bash + T_fork/exec + T_dynlink(arm) + T_read + T_codec + T_write
          └──── common to all arms ────┘                  └─ arm ─┘
```
Only the last term is a property of the codec. On the decode plane the middle terms are
**14–>100 %** of the signal (F4, [H/P]). On the encode plane they are negligible for ANVIL
(`--parse=ratio` on multi-MB files) and material for the fast arms on `xml`.

## §7.3 Memory accounting (what `%M` will actually report)

| arm class | decode peak RSS composition | mozilla estimate **[H/P]** |
|---|---|---|
| anvil (buffered harness) | `packed` + `reserve(total)` + block scratch — **deterministic**, from `anvil.cpp:4859–4860` | ≈ 65.0 MB (1.28× source) |
| brotli (buffered harness) | `input` + `vector(expected)` — pre-sized | ≈ 65 MB |
| zstd 1–19 (streaming CLI) | window + fixed buffers | ≈ 10–30 MB |
| zstd-22-long27 | **windowLog 27 = 128 MB** | ≈ 130–190 MB |
| xz-9e | 64 MiB dictionary | ≈ 70–100 MB |

→ The decode-RSS axis will rank `zstd-1..19` best by ~an order of magnitude for reasons that
are **100 % harness and window configuration**. **F1.**

## §7.4 Instrument-side hidden costs

- **825 artifact files** per Silesia job, every one SHA-256'd by `manifest.json` (`:966–970`) —
  a per-file `read_bytes()` + hash, so the manifest pass re-reads the entire artifact set.
- **~96 GB** of disk read per Silesia job at reps=7 (panel 141.8 MB × 320 runs + census), all
  of it re-hashed after every repetition. **[A]** from my linter's invocation counts × sizes.
- `paired_bench.py` recomputes SHA-256 of each observation **after every timed repetition** —
  correct for integrity, and it is outside the timed region, so the cost is wall-clock and
  artifact size, not validity.

---

# §8. DECODER / RESOURCE RISKS (things the vehicle will *not* catch)

1. **No streaming decode API.** Θ(input+output) decode RSS by construction (§7.3). Against a
   project doctrine that includes a ≤1 MB decompressor constraint, this is a first-order
   product risk. **The vehicle surfaces it as a measurement and thereby neutralises it.**
2. **Whole-buffer encoder.** Same shape on the encode side; invisible to byte accounting,
   dominant in RSS.
3. **Block size = memory horizon** (`anvil.cpp:4868–4890`, §4). Raising it is a one-flag
   change with a real ratio payoff [U-pending] and a real memory cost. It is simultaneously
   the project's biggest measured ratio lever and a decoder-memory commitment — and the
   instrument currently varies it *by accident*.
4. **`reserve_hint` amplification bound.** `min(total, max(1 MiB, in.size()*256))` bounds a
   crafted tiny header, good — but for a legitimate 100× compression ratio the reserve is
   `total`, so peak RSS tracks *declared* size, i.e. it is attacker-influenceable up to the
   `256×` bound per block. That is a track-19 concern, not track 04's, but the RSS axis here
   would be one of its detectors and the vehicle has no such gate.
5. **Concurrency.** `cancel-in-progress: false` with a per-corpus group is correct; but
   Silesia and enwik8 are separate jobs on **separate hosted VMs**, and closure §"Reference-
   series interpretation" already records that a smoke run landed on a **different host
   class (EPYC 9V74)**. Absolute throughput must never be spliced across them (prereg §10.2
   agrees); my F3 argument makes this worse, not better.

---

# §9. ALTERNATIVE MECHANISM (justified): in-process timing harnesses

The mandate asks for a materially different mechanism if the main one fails. The main
mechanism here is the instrument, and its dominant defect (F4) has a fix that **already
exists in this repository**:

```cpp
// src/anvil.cpp:5023-5026 — read outside, time the operation, write outside
auto in=read_file(argv[2]); GlobalStats st;
t0=steady_clock::now(); out=decompress(in,opt); t1=steady_clock::now();
write_file(argv[3],out);
```

**Mechanism: replace every process-level timed invocation with a per-arm in-process timing
binary** that (a) loads the input once, (b) runs `iters` warm iterations, (c) times `iters`
timed iterations **internally** around the codec call only, (d) writes the output once
outside the timed region, and (e) prints the raw per-iteration times so the driver keeps the
same evidence discipline. `observe.sh` then guards only the *final* write.

**Why this is strictly better, not merely different:**
- removes `T_wrapper_bash`, `T_fork`, `T_dynlink` and `T_write` from the estimand (F4);
- turns a *ratio of two noisy wall clocks* into a *distribution of per-iteration times*
  whose median is far more stable at the same wall-clock cost — this directly relieves the
  CV-gate problem that already destroyed a 10.15× measurement (F5/F7);
- makes the RSS question separable: an in-process harness can report RSS *after* the input
  is loaded and *before* the output is allocated, yielding a genuine decoder-attributable
  working set instead of a buffering artifact (F1);
- costs **zero codec changes** and reuses `anvil_bench`-style structure already in-tree.

**Cost:** four small generated harnesses (anvil decode/encode already exist as
`anvil_decoder`/`brotli_dense`; zstd and xz need decode/encode harness variants), and one
change to the driver contract. That is strictly less work than the constructive lane's
14-arm matrix rebuild.

**Rejected alternative:** raising `--reps` to 13 instead. It buys √(13/7)=1.36× in noise
terms while leaving the 14–100 % common-mode overhead **completely untouched** — it reduces
variance around the wrong estimand. Cheaper, and wrong.

---

# §10. DECISIVE REMOTE EXPERIMENT — pre-registered, thresholds frozen NOW

## §10.1 Step 1 (free, no runner): the retained-artifact parity re-sweep

**Pre-registered before execution. Input:** `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv`
(read-only). **Tool:** a post-processor in my prototype dir. **Zero codec, zero timer.**

| ID | Pre-registered claim | Threshold — outcome |
|---|---|---|
| **T1** | Class-A ANVIL rows are measured at a block size far below the Brotli reference window | Confirmed iff `o.block_size` is unassigned in `tools/bench_native.cpp` **and** the `Options` default < `BROTLI_DEFAULT_WINDOW`. **I have confirmed the first half [M]; the second is [U].** |
| **T2** | The FRONT-GAP bracket predicate, when actually implemented, changes ≥1 of the 5 canonical non-dominated rows | **KILL** the "bracket rule is already satisfied" reading if 0 changes at ε ∈ {0.05,0.10,0.25,0.50,1.00} |
| **T3** | The reference-density sweep is **invariant to the candidate window** | **The verdict-flipping test.** Sweep density × candidate window. If `mean_crossing` > 0 at *any* candidate window ⇒ the constructive lane's §9.2 is a configuration artifact and **the dense plane regains priority immediately** |
| **T4** | Fraction of Class-A byte verdicts that would flip at parity | Report per-file `bytes(default)` vs `bytes(raised)` — **not computable without T1's constant**, so T1 gates T4 |

**T1 and T3 are the whole ballgame, and T3 is the cheapest thing in this report.**

## §10.2 Step 2 (only if T3 returns "still 0.000"): one corrected pilot job

Scope — deliberately minimal, because the object is **instrument calibration**, not a crossing hunt:

- **Panel:** `mozilla` (largest text+binary, Class-A parity stress) + `xml` (worst
  overhead contamination, F4) + `nci`. Drop `dickens`/`webster` (mozilla spans their
  character).
- **Arms:** `anvil-legacy@default`, `anvil-legacy@block=134217728` (**the parity control**),
  `anvil-aux@raised`, `brotli-q11@lgwin30`, `xz-9e`, plus **`A/A′`**
  (`brotli_dense 11` vs frozen `brotli_lw` — different binary, byte-identical, F8).
  **6 arms.** All other 14 reference arms stay in the **byte census only**, where they are
  deterministic and cheap (F9: their absence from timing costs nothing, since they cannot
  change a calibration verdict).
- **Timing:** in-process harnesses (§9), `--reps 13`, `--warmups 2`, `--cpu 0` enforced.
- **Thresholds — frozen, and NOT to be moved after seeing data:**

| Gate | Threshold |
|---|---|
| G1 overhead floor | `overhead_share = wrapper+exec / candidate_median` **≤ 5 %** on every cell, else cell is `DEGENERATE_TIMING_SHORT` |
| G2 A/A′ | bootstrap CI of paired log-ratio must contain 0, **and** the two binaries' outputs must be byte-identical |
| G3 A/A | CI must contain 0; **cell-level failure is informative, not fatal**; whole-run invalidation reserved for correctness/hash/identity (F5) |
| G4 dispersion | `MAD(log_ratios)` **with a MAD floor** (V1/S7): `MAD ≥ 0.02·median|log ratio|`, so MAD=0 cannot pass |
| G5 practical effect | `ε = 0.02` on the *corrected* rate, declared **before** dispatch |
| G6 multiplicity | Holm across the arm family within each (plane, file); report `p_holm` |
| G7 parity control | `bytes(raised) − bytes(default)` reported per file; **any Class-A byte kill is void unless the mechanism was evaluated at parity** |
| G8 RSS | `resident_charge_kib = peak − (input+output)/1024`; **raw RSS may not enter dominance** |
| G9 binary size | `dominated_full_cost` **deleted**; size dominance restricted to the matched-scope anvil↔brotli pair |
| G10 arbiter | joint-plane `dominates_joint` over `(compressed_bytes, encode_mbps, decode_mbps, resident_charge)`; per-plane labels are diagnostics only |
| G11 ranking | rank on `−ln(paired_ratio)` against the same in-invocation control; **absolute MB/s across invocations is forbidden** (F3) |

## §10.3 Kill / promote thresholds for the vehicle itself

**KILL the dense-frontier vehicle (do not dispatch, in any form) if ANY holds:**
- T1 confirms the parity defect **and** the window is not raisable by a flag (i.e. the
  handicap is structural) — then the byte axis cannot be compared to the reference class at all;
- T3 shows any crossing at parity — the vehicle is then *designed for the wrong question*
  and the arm set must be rebuilt before, not after, dispatch;
- `census.csv` from a pilot shows ≥16 arms whose bytes move >0.1 % across a runner-image
  bump (F9/S2) with no SHA-256 anchor to detect it.

**PROMOTE-TO-REMOTE (the corrected pilot) requires ALL of:**
G1–G11 implemented and frozen; parity control present; `pareto_front.py` schema emitted
**and** run, or the inline arbiter frozen as canonical with the bracket predicate implemented
(F6); `ε` and margin thresholds declared pre-dispatch; `A/A′` present; **and** the success
criterion is **"the instrument's overhead_share ≤ 5 % and dispersion clears G4 on ≥ 4 of 5
cells"** — explicitly **not** "a crossing appeared", because §5.2 shows crossings are not
what this run can deliver.

**HOLD (default if neither fires):** ship the byte/RSS census + the margin statistic; do not
let any Class-A byte verdict kill a mechanism; reallocate to mechanism work on held-out
families (track 03).

---

# §11. FINAL RULING

> # **PILOT**
>
> **Reject the frozen 20-arm dense-frontier dispatch.** It has never run (M2), it is
> untracked, it automates a third arbiter, its paired CI is computed and then discarded for
> ranking, its decode plane carries 14–>100 % common-mode overhead that its own A/A null is
> structurally blind to, its RSS and binary-size axes are not comparable by construction,
> one bad cell in 200 invalidates the job, and — decisively — **its Class-A substrate
> compares the candidate at a default block size against a whole-file reference window, so
> the headline "crossing is unreachable at any reference density" is a statement about a
> mis-measured candidate, not about the frontier (§5.2).**
>
> **But do not close the track.** Two things survive intact and are stronger than the
> defects: (i) the byte axis is correctly sourced and citation-grade, and ANVIL is
> **already 4.15 % smaller than xz -9e on full Silesia while carrying a window handicap the
> reference does not pay** (§7.1); (ii) the one live hypothesis — aux-unBWT — buys
> **1.36–2.34× decode for +0.0091 % bytes**, and its **exact wire charge is known and
> independently re-derived** (§7.1). That is a real, cheap, adopt-class win which no
> arbiter defect can touch.
>
> **Cheapest decisive next step:** the retained-artifact parity re-sweep (**T1 + T3**,
> §10.1) — arithmetic over one existing CSV, no runner, no benchmark, no codec. It decides
> whether the constructive lane's load-bearing result stands or is an artifact. Authorise
> the corrected 6-arm pilot (**§10.2**) only if T3 returns "still unreachable".

**Falsification criteria for this report, stated so I can be wrong in public:**
1. If `tools/bench_native.cpp` *does* set `o.block_size` somewhere I did not read, and the
   default is ≥ 4 MiB, then §4 collapses, T3 loses its motivation, and the constructive
   lane's §9.2 stands unopposed — **my central objection is void**.
2. If `paired_bench.py`'s pinned blob in fact routes timed invocations through an in-process
   path I misread, F4's magnitude estimate is void (its *structure* — additive common-mode
   term — would still hold, and the A/A blindness would still hold).
3. If a retained Silesia/enwik8 suite exists and shows crossings at the Class-B configuration
   (128 MiB), then the constructive lane's U6 resolves *against* my §5.2 and its
   recommendation is vindicated.
4. If the reported `xml` decode signal is ≫10 ms on the runner (e.g. Brotli decode
   materially slower than the 1 GB/s I used), F4's contamination column shrinks — though the
   `>100 %` cell is the weakest of my estimates and I have labelled it [H/P] throughout.

---

# §12. UNCERTAINTIES

| ID | Uncertainty | How to settle | Can it flip the verdict? |
|---|---|---|---|
| **X1** | The exact `Options.block_size` default constant [U] | one grep of the `Options` initialiser | **YES** — gate on T1 |
| **X2** | The `+16.98 %` E4 figure's provenance (I did not open `13-e4-block-routing-oracle.md`) [U] | read that file | No — the *direction* is established by `anvil.cpp:4868–4890` + `bench_native.cpp:25` |
| **X3** | Additive overhead magnitude `c + τ_arm` on the runner [U] | 200 in-job `observe.sh … true` samples | No — it sets G1's threshold empirically, not the ruling |
| **X4** | Whether a retained Silesia/enwik8 timing suite exists (U6 in the constructive lane) | artifact inventory | **YES** — see falsification criterion 3 |
| **X5** | Whether the 360-min cap is real headroom | derive from census `encode_s` already in `census.csv` — **no new run** | No (matrix is 6 arms, not 20) |
| **X6** | Whether `INVALID_SPEED_ACCOUNTING` (G4) permanently disables the speed axis | coordinator ruling + U5 ledger recompute | **YES** — would make the correct output census-only, which is my HOLD branch |

**X1 and X4 are the only two that can still change this report's ruling.**

---

# §13. AMENDMENT A1 (closeout) — X1 resolved, one self-correction, verdict-changing vs descriptive, pilot schema frozen

*(Appended after closeout. Corrects §4 and §12/X1 in place; §0 ruling unchanged: **PILOT**.)*

## A1.0 X1 RESOLVED — and it corrects a claim of mine

**`src/anvil.cpp:3769` → `uint32_t block_size = 256*1024;` = 256 KiB default. [M]**
No longer **[U]**. The Class-A claim is now **fully verified from source**:
`tools/bench_native.cpp:25` never assigns `o.block_size` and never uses `parse="ratio"`
(`:65–67`), so Class-A ANVIL rows run at **256 KiB** against a Brotli reference at
`BROTLI_DEFAULT_WINDOW` = lgwin 22 = 4 MiB (`bench_native.cpp:39,43`) — a **16× asymmetry**,
on exactly the multi-block files where the E4 regret is largest.

**Self-correction — I was wrong about Class B.** §4 claimed "Class B is also asymmetric, ~8×".
**False.** `src/anvil.cpp:4999`: `if(opt.parse=="ratio" && !block_explicit) opt.block_size = 128u<<20;`
with `block_explicit` set by `--block=` at `:4965`. The dense vehicle's ANVIL command is
`--parse=ratio` with **no** `--block=` (`workflow:514–516`) ⇒ **128 MiB**. Every file in both
target corpora is smaller than 134,217,728 — Silesia's largest is `mozilla` 51,220,480 B,
enwik8 is 100,000,000 B — so **every dense-vehicle cell is a single whole-file block, and
ANVIL is at parity with the lgwin-30 Brotli arm.** [A]

**This sharpens §5.2 rather than weakening it.** The constructive lane's §9 density sweep runs
on `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv`, a **Class-A (256 KiB)** artifact,
while the vehicle it is used to justify would run **Class B (128 MiB, single block)**. The
sweep therefore certifies a **candidate configuration the vehicle does not emit** — a
substrate/configuration mismatch, not merely a one-sided sweep.

**Standing rule:** a Class-A byte verdict is admissible only for inputs ≤ 256 KiB (⇒ single
block ⇒ parity), or after de-blocking with the measured E4 regret curve (+16.98 % @256 KiB,
+5.18 % @4 MiB, +2.01 % @16 MiB [U-provenance per §12/X2]). **All Class-A ANVIL byte rows on
files > 256 KiB are confounded by up to ~16.98 %.**

## A1.1 Verdict-changing vs descriptive

**Verdict-changing (blocking):** **F3** (paired CI computed then discarded; cross-invocation
absolute MB/s ranking) · **F1/F2** (RSS + binary size inside the predicate; non-comparable by
construction) · **F5** (one fatal cell in 200 invalidates the job) · **F6** (no encoded
FRONT-GAP/DEGENERATE; `tools/pareto_verify.py` does not exist; schema hand-off cannot execute)
· **F7** (CV gate screens the wrong statistic; MAD=0 passes a 3× outlier) · **F4**
(common-mode additive overhead, 14–>100 % on decode; A/A structurally blind) · **A1.0**
(Class-A 256 KiB substrate) · **§5.2** (sweep substrate ≠ vehicle configuration; gate = T3).

**Descriptive (fix cheaply, do not block):** F7-adjacent affinity/census-pinning/env-assertion
(all arms share the runner; fails toward *rejection*) · F8 A/A single path (add `A/A′`) ·
F9 16/20 arms unanchored (a drift *detector*, not a bias) · F10 undeclared
`DEGENERATE_RATIO` (dead code at ratios 0.219–0.248; brief §5 hygiene) · F11 xz pays two bash
spawns (one-sided handicap against the best-ratio reference; cannot create a false ANVIL win) ·
F12 `arm-contract.json` misdescribes the arm · F13 `%M` max-not-sum (correct while
single-process) · F14 dispatch budget / 66.89 % panel coverage.

**Verdict-changing but not track 04's to fix — escalate:** **F1-API.** ANVIL has **no
streaming decode API**; `decompress(const vector<uint8_t>&, const Options&) ->
vector<uint8_t>` with `out.reserve(min(total, max(1MiB, in*256)))` (`anvil.cpp:4859–4860`) makes
decode RSS **Θ(input+output) by construction**. This is a real product risk that the vehicle
*neutralises* by mis-attributing it to measurement. **It must not be recorded as a bench cell.**
Route to tracks 18/19.

## A1.2 FROZEN six-arm corrected pilot schema

**A. Estimand — paired-relative, in-job, in-process.** Reported quantity is
`log_rel_rate(arm) = −ln(paired_ratio(arm, control=anvil-legacy))` from the **same driver
invocation**, timing the codec operation **internally** (`steady_clock` around the call, the
`src/anvil.cpp:5023–5026` pattern) with input load and output write **outside** the timed
region. **Absolute MB/s from any source is forbidden for ranking (F3).** `observe.sh` guards
only the final write.

**B. Arms — exactly six.** (1) `anvil-legacy@--parse=ratio` — control, 128 MiB = whole file;
(2) **`anvil-legacy@--block=262144` — the block/window parity control**; (3)
`anvil-aux@--parse=ratio`; (4) `brotli-q11@lgwin30`; (5) `xz-9e`; (6) **`A/A′`** =
`brotli_dense 11` vs frozen `brotli_lw` — different binary, byte-identical, a true two-path
null (F8). The other 14 reference arms stay in the **byte census only**. **Parity declared in
the arm table:** ANVIL ≥128 MiB block vs Brotli lgwin 30, whole-file on every file in both
corpora (A1.0); arm 2 exists to *measure* the 256 KiB regret Class A silently charged, not to
compete.

**C. Panel.** `mozilla` (51.2 MB, parity stress) · `nci` (33.5 MB) · `xml` (5.3 MB, worst
overhead contamination). `dickens`/`webster` dropped as character-duplicates of mozilla.

**D. Gates — frozen.** `G1 overhead_share ≤ 5 %` else `DEGENERATE_TIMING_SHORT` · `G2` A/A′ CI
contains 0 and outputs byte-identical · `G3` A/A cell failure **informative, not fatal** (run
invalid only on correctness/hash/identity) · `G4 MAD(log_ratios) ≥ 0.02·median|log ratio|` so
MAD=0 cannot pass · `G5 ε = 0.02` on the corrected rate · `G6 Holm within (plane, file)` ·
`G7` parity control reported per file; **Class-A byte kills void without it** · **`G8` RSS
charge = `peak_kib − (input+output)/1024`; raw RSS may not enter dominance** · **`G9`
`dominated_full_cost` DELETED; size dominance only on matched scopes (anvil↔brotli
decoder-harness); `system-cli` rows descriptive-only** · `G10` joint-plane
`dominates_joint(compressed_bytes, encode, decode, resident_charge)` · **`G11`
FRONT-GAP/DEGENERATE implemented in code with pre-registered ε and the R-2 bracket predicate,
plus a DEGENERATE guard that also fires on G1 timing degeneracy, not only ratio ≥ 0.95.**

**E. Success criterion — explicitly NOT a crossing.** The pilot passes when
`overhead_share ≤ 5 %` and dispersion clears G4 on **≥ 2 of 3** panel files per plane, and the
parity control's byte delta is reported for all three. §5.2 establishes a crossing is not what
this instrument can deliver; a pilot whose success test is "a crossing appeared" is
unfalsifiable-by-construction and must be rejected at the gate.

**F. Gate order.** T1+T3 (retained-artifact, free, no runner) → 6-arm pilot → only then any
20-arm dispatch. **No 20-arm dispatch is authorised until the six-arm pilot clears A, B, D, E.**

---

# §14. CLAIM HYGIENE

- Every [M] in this report cites a file and line I opened in this session, or arithmetic I
  performed and showed. Every [A] is re-checkable from the shown numbers.
- Exactly one [U] load-bearing item is flagged in-line and gated by a named experiment (X1).
- One of my own candidate findings was **withdrawn** before drafting (§1) and one was
  **corrected against the sibling track's position** (§4, Class-B parity). Both are recorded.
- No Actions series, Windows series, or Linux series is compared to another. All Linux MB/s
  figures I quote are labelled **[H/P]** estimates used only for order-of-magnitude overhead
  reasoning, never as measurements.
- No existing file was modified. No commit, push, reset, clean, stash, restore, or rebase.
  No local corpus benchmark, sweep, or fuzz campaign. The one artifact I created runs no
  codec and no timer.