# Track 07 — Bounded BWT Subblocking: Independent Adversarial Audit (Fledge Alpha Free)

**Status:** superseded-in-part — §13/§14/§15 added after reading
`docs/swarm-2026-10-02/07-bwt-subblocking-space-bunny.md`; earlier sections stand as written.
**Date:** 2026-10-02
**Status:** FINAL (post-pair-reconciliation). §13 adjudicates the constructive lane's H-1 against
`third_party/libsais/src/libsais.c`; §14 records per-claim AGREE/DISPUTE; §15 is the ruling.
Isolated proof: `prototypes/swarm-2026-10-02/07-bwt-subblocking/fledge/fledge_h1_check.py`
(integer-only; no corpus, no codec, no timing).
**Role:** independent adversarial reviewer. Reconstructed from source + ledger + remote artifacts.
**Sources of record used:** `docs/I10-AUX-UNBWT-RESULTS.md`, `docs/I10-AUX-UNBWT-INTEGRATION-PLAN.md`,
`docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md`, `docs/I10-BREAKTHROUGH-PROGRAM.md` §I10-1A.2,
`docs/audit-2026-09-07/06-do-not-reburn.md` §F0.1/§G1/§G2, `docs/audit-2026-09-07/12-bwt-theory-findings.md`,
`src/anvil.cpp`, `tools/bwt_subblock_sweep.py`, `.github/workflows/anvil-i10-bwt-subblock-frontier.yml`.
**No Space Bunny output was consulted. No local benchmark, build, or corpus measurement was run.**
No existing file was modified; no commit, reset, clean, stash, restore, or rebase was performed.

---

## 0. Label key (used throughout, no exceptions)

- **[M]** Measured — traceable to a named remote run/artifact or to exact source text.
- **[D]** Derived — arithmetic on [M] inputs only; no new measurement.
- **[P]** Projection/hypothesis — my estimate, explicitly not evidence.
- **[A]** Assumption — load-bearing belief that is *not* established.

---

## 1. What actually exists (the mechanism is one `if`, not a research contribution)

**[M]** The entire "bounded BWT subblocking" mechanism is `src/anvil.cpp:4446-4457`: split the
(already transformed) ratio-block input into fixed `cap`-byte pieces at **byte-aligned offsets**,
BWT+postcode each piece independently, and wrap the list in `0xFF / uvar(nsub) / {uvar(dlen),
uvar(plen), payload}*`. There is **no boundary search, no boundary optimization, no content-defined
chunking, no overlap/merge, no cross-subblock model carryover, and no parallel scheduler**.

**[M]** Decode is strictly serial: `ratio_backend_decode` (`src/anvil.cpp:4482-4490`) loops subblocks
in order, each `bwt_backend_decode` runs a full inverse BWT into a fresh vector, and the result is
**copied** into the accumulator via `out.insert`.

**[M]** `--bwt-subblock` defaults to `128u<<20` and is annotated in-source as
*"memory lever only, not a ratio lever"* (`src/anvil.cpp:3795`).

**[D]** Consequence: the track's entire mechanism surface is a **single integer knob** whose two
effects are (a) N independent adaptive postcoder warm-ups and (b) a smaller inverse-BWT working set.
There is no mechanism to be novel about. Everything else in track 07 is measurement methodology.

---

## 2. Prior-art map — the mechanism is fully anticipated, and the anticipation is *old*

| Prior art | What it already does | Separator that does **not** exist for ANVIL |
|---|---|---|
| bzip2 (1996) | 900 KB BWT blocks; chose a *small* block because BWT locality decays with block size — the exact tradeoff measured here | none; ANVIL's 128 MiB default is the same knob |
| libbzip2 / BOW/ed variants, `bzip2` `--block-size` tuning folklore | cap is a rate/RSS dial, small caps cost rate | none |
| libdivsufsort / libsais **induced-sorting** construction | BWT is produced by induced sorting in *blocks* + LF/merge; block-locality is an implementation-era artifact, not a coding choice | none |
| Ferragina–Manzini; **libsais `libsais_bwt_aux` / `libsais_unbwt_aux`** | sampled primary index → cache-blocked LF walking. **Already adopted by ANVIL** (`docs/I10-AUX-UNBWT-RESULTS.md`) | none — adopt-class, explicitly declared so |
| BWA / SPAdes / BCALM / assemblers | block-partitioned BWT + merge, multi-threaded inversion | none |
| zstd / libdeflate `--long`, window-log ladder | block/window size as a rate/RSS/speed dial routed per block | none |

**[D]** Doctrine item 1 (prior art as first-class constraint) plus doctrine item 9 in the brief
("**bounded BWT subblocking**" listed as an *active* direction) are in direct tension here. The
mechanism is 25+ years old, the specific sampled-index instantiation is already vendored and already
measured as adopt-class, and the *only* free variable is a threshold integer.

**[M]** Adopt-class status is not my inference: `docs/I10-AUX-UNBWT-RESULTS.md:5` —
*"Mechanism class: adopt-class decoder engineering; no novelty claim"*; and
`docs/I10-AUX-UNBWT-INTEGRATION-PLAN.md:6` — *"Novelty: none"*.

**Verdict on novelty: there is no novelty lane here to lose.** Whatever this track produces, it is
engineering. That is acceptable *only* if it moves a real Pareto axis; §5 shows it moves at most one, and only barely.

---

## 3. Ratio loss — mechanism, magnitude, and where it actually comes from

### 3.1 The measured curve **[M]** (`docs/I10-BREAKTHROUGH-PROGRAM.md:633-643`, run `35930672607`)

Silesia portfolio, relative to the frozen 128 MiB point (46,466,339 B / 4.2015 s / 248.4 MiB):

| Cap | Portfolio bytes | Δ vs 128 | Decode s | Peak RSS |
|---:|---:|---:|---:|---:|
| 8 MiB | 47,365,274 | **+898,935 (+1.9346%)** | 3.7022 | 124.4 MiB |
| 16 MiB | 46,938,837 | **+472,498 (+1.0169%)** | 4.2232 | 125.7 MiB |
| 32 MiB | 46,648,642 | **+182,303 (+0.3923%)** | 4.2104 | 203.4 MiB |
| 64 MiB | 46,466,339 | **0 (byte-identical)** | 4.2193 | 248.4 MiB |
| 128 MiB | 46,466,339 | — | 4.2015 | 248.4 MiB |

enwik8 **[M]**: 8/16/32 MiB **route away from BWT entirely** and cost **+5.4075%**; 64 MiB keeps the
BWT route at **+3.2259%**; enwik8 timing/Pareto classification is **blocked by the sweep's own
validity rule**.

### 3.2 The loss is *model warm-up*, not framing **[D]**

Framing cost is negligible and cannot explain the curve: for enwik8 at an 8 MiB cap there are 13
subblocks, so the `0xFF` frame costs `1 + uvar(13) + 13 × (uvar(dlen) + uvar(plen))` ≈ **~80 B**
against a +1.27 MB byte delta. Per-subblock postcoder headers are ~5 B each **[M]**,
`docs/audit-2026-09-07/12-bwt-theory-findings.md:75`.

**[D]** Therefore >99.99% of the ratio loss is *adaptive postcoder re-convergence* plus loss of
long-range BWT sort context — exactly the "warm-up tax" independently measured in E4:
`docs/audit-2026-09-07/06-do-not-reburn.md:196-198` — webster BWT wins **10/10** 4 MiB blocks taken
individually, yet sum-of-blocks BWT = 8,211,644 vs whole-file 7,317,329 = **+894,315 B (+12.22%)**.
Brotli taxed the same way costs +773,686 B. **[M]** This is a *representation-level* tax that scales
with block count, not a boundary-placement artifact — boundaries are already byte-aligned and
placement is therefore not a free variable.

### 3.3 Adversarial finding: the Silesia curve is a **one-file** effect **[D]**

**[M]** No Silesia file exceeds 64 MiB; the largest BWT-routed file is webster at 41,458,703 B
(39.6 MiB) **[M]** (`docs/I10-AUX-UNBWT-RESULTS.md:85`).

**[D]** Therefore at 64 MiB and 128 MiB caps *no Silesia file is split at all* — which is why those
two rows are byte-identical and RSS-identical (the sweep itself calls 64 MiB "internally
dominated"). At 32 MiB only webster splits (2 subblocks). At 16 MiB only webster splits (3).
At 8 MiB only webster splits (5).

**[D]** So the whole published Silesia portfolio delta (+472,498 B at 16 MiB) is *arithmetically
forced* to be webster's. Cross-check: E4 measured webster alone at 4 MiB for +894,315 B **[M]**;
at 16 MiB (3 blocks) ~1/3 of that warm-up tax is +300-470 KB, which matches the observed
+472,498 B to within the residual of the other 11 files.

**[D] Consequence:** "+1.0169% for 2× less memory on the Silesia portfolio" is a **webster-only**
result inflated by a 12-file denominator. Reported per source byte the effect is
+472,498 / 41,458,703 = **+1.14% on the one file that changes**. The track's headline generality is
a denominator artifact. Any promotion must quote per-file numbers, never the portfolio ratio.

**[A]** Load-bearing and unverified: that the other 11 files are *individually* insensitive down to
~4 MiB. E4 measured mozilla/samba/ooffice/sao at 4 MiB as having **zero** BWT-winning blocks **[M]**,
which is consistent, but their direct-BWT loss at small caps was never measured because they are
Brotli-routed and therefore invisible in a min-route portfolio.

---

## 4. Primary/aux index charges — a hidden cost that grows *as you shrink the cap*

**[M]** Aux policy (`docs/I10-AUX-UNBWT-INTEGRATION-PLAN.md:112-118`):
`icount = 1 + floor((n-1)/r)`, `r` = smallest power of two ≥ `ceil(n/1024)`, `r ≥ 2`;
index is **4 B per entry**, charged per BWT payload (`[0xFE][post][r uvar][icount uvar][I…u32le]`).

**[D]** Subblocking makes this charge **per subblock**. For a subblock of size m, `icount(m)` lands in
(512, 1024], i.e. **2–4 KiB per subblock**, and the subblock count is `ceil(n/cap)`. Therefore:

```
aux_index_bytes(n, cap) ≈ 4 KiB × ceil(n / cap)
```

**[D]** Worked checks against **[M]** anchors:
- enwik8, one 100 MB block, 128 MiB cap: `r = 131072`, `icount = 764` → 3,056 B; measured aux delta
  3,054 B with framing **[M]** — model matches to 2 B.
- enwik8, 8 MiB cap: 13 subblocks × ~4 KiB (m=8 MiB → `r = 8192`, `icount = 1024`) ≈ **53 KB**,
  i.e. **~17× the 128 MiB index charge**, an incremental **+50 KB** on top of the +1.27 MB
  subblocking tax.

**[D] The structural point:** the auxiliary-index charge is **anti-correlated with the memory lever**.
Shrinking the cap — the only thing that reduces peak RSS — monotonically *increases* index bytes,
because `icount` floors at 512 per subblock and subblock count grows as `n/cap`. Projection for a
1 GB file **[P]**: cap 1 MiB → 1,024 subblocks × ~4 KiB ≈ **4 MiB of index** (+0.4% of source, and
1024 separate postcoder warm-ups). The cheap-looking lever becomes quadratically expensive in
per-region fixed costs.

**[D]** No published artifact separates aux-index bytes from postcoder warm-up bytes. The prereg's
`IR-G5` accounting demands `aux_index_bytes` and `postcoder_bytes` separately
(`docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md:512-521`); the executed sweep (`tools/bwt_subblock_sweep.py`)
has **no column for either** — it records only `selected_bytes`, `bwt_bytes`, `bwt_subblocks`. So the
byte tax measured in §3.1 is an **un-decomposed aggregate**. Attributing it to warm-up is my **[D]**
inference from the framing arithmetic, not a measurement.

---

## 5. Memory: the coefficient moves, the asymptote does not — and the published RSS series fails an arithmetic self-check

### 5.1 Cost model **[D]** (from source, not from artifacts)

Decode peak RSS, subblock path, cap C, block length n **[M]** (`src/anvil.cpp:4391, 4481-4489`):

```
accumulator  out.reserve(expected)      n            (whole block, allocated up front)
per subblock bwt vector                 C
per subblock sub (returned by value)    C            <- pure copy, avoidable
per subblock tmp int32[expected+1]      4C           <- libsais LF-walk scratch, unavoidable
per subblock aux index                  ~4 KiB
────────────────────────────────────────────────
floor(cap C) = n + 6C          bare path (n ≤ C) = 6n
```

**[D]** Three consequences:
1. The floor is **O(n) with a hard coefficient ≥ 2n** (accumulator + libsais `tmp` at minimum).
   Subblocking changes the *coefficient* of the `C` term only. It **cannot** make BWT decode
   O(1)-memory. A 4 GB input has a ≥ 8 GB decode-RSS floor regardless of cap. Any "bounded memory"
   language is bounded only in the constant.
2. `sub` is a **pure double buffer** (`out.insert(out.end(), sub.begin(), sub.end())`,
   `src/anvil.cpp:4489`) — C bytes of avoidable copy at every cap. That is free engineering, and it
   is *not* in the shipped numbers.
3. Serial decode means no subblock can overlap another, so subblocking cannot buy throughput on one
   thread. There is no threaded inner scheduler in the tree; `decode_threads` is outer-block only
   (`docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md:88`), and §3.3 of that prereg concedes the inner
   threaded arm is `INCONCLUSIVE-INFRA` until a research-only scheduler exists.

### 5.2 The measured RSS series contradicts the model **[D]** — decisive credibility problem

Applying §5.1's floor to webster (n = 39.6 MiB, the largest BWT-routed Silesia file) against the
published peak-RSS maxima:

| Cap | webster subblocks | Model floor `n+6C` | Bare `6n` | **Measured max (corpus)** | Verdict |
|---:|---:|---:|---:|---:|---|
| 8 MiB | 5 | 87.6 MiB | — | 124.4 MiB | measured **above** floor — OK |
| 16 MiB | 3 | **135.6 MiB** | — | **125.7 MiB** | measured **7% BELOW the floor — impossible** |
| 32 MiB | 2 | **231.6 MiB** | — | **203.4 MiB** | measured **12% BELOW the floor — impossible** |
| 64 MiB | 1 (unsplit) | — | 237.3 MiB | 248.4 MiB | OK (+4.7%) |
| 128 MiB | 1 (unsplit) | — | 237.3 MiB | 248.4 MiB | OK (+4.7%) |

**[D]** A 32 MiB cap forces `tmp = 4 × 32 MiB = 128 MiB` live simultaneously with a 32 MiB `bwt`
vector, a 32 MiB `sub` vector and a 39.6 MiB pre-reserved accumulator — **231.6 MiB cannot be
under-run**. The artifact reports 203.4 MiB. Same contradiction at 16 MiB. Meanwhile 64/128 MiB land
within 5% of `6n`, and 8 MiB lands above its floor. **The series therefore does not follow any single
allocation structure: it is over-determined at one end, under-determined in the middle.**

Candidate resolutions, none verified:
- (a) `peak_rss_kib` (`tools/bwt_subblock_sweep.py:448`, single `/usr/bin/time -f %M` per file, **one
  shot, no repeats, no pairing, no A/A**) is simply noisy or mis-attributed;
- (b) the maximum is being reported from a *different file* than webster at 16/32 MiB because
  `max_peak_rss_kib` is taken over the whole corpus including Brotli-routed files;
- (c) the wire does not contain the subblock counts the artifact claims.

**[M]** Resolution (c) is partly excluded: `prepare()` hard-fails if the parsed `0xFF` subblock count
disagrees with `ceil(n/cap)` (`tools/bwt_subblock_sweep.py:297-301`). So the wire is as recorded.

**This is the single most important finding in the audit.** The *entire* memory justification for
this track rests on a 5-point RSS series that (i) has **no error bars**, (ii) is **single-shot**,
(iii) is **cross-job** relative to the `R_xz`/`R_br` budget it is compared against — which the
prereg itself forbids (`docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md:266`: *"must not be combined with
the new multi-thread measurements as if it were a same-job comparison"*) — and (iv) **violates an
allocation floor derivable from the shipped source at its two most favorable points.** No memory claim
from this track is currently admissible.

### 5.3 Decode-speed: the lever is anti-correlated with itself **[M]**

Silesia decode: 8 MiB → 3.7022 s (**1.135× faster** than 128 MiB); 16/32/64 MiB → 4.2232 / 4.2104 /
4.2193 s, i.e. **within 0.5% of the control — no gain at all**. So the *only* cap that buys decode
speed is the one that costs the *most* bytes (+1.9346%). Speed appears only where the LF walk fits in
cache, and that is a cache-regime cliff, not a dial: one point gains 13.5%, three points gain nothing.

---

## 6. Strongest falsification case (the kill argument, in its strongest form)

> **Bounded BWT subblocking cannot produce a frontier crossing, and the arithmetic says so before any
> new experiment is run.**
>
> `IR-G6` (`docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md:529-538`) requires *all* of: bytes <
> min(Brotli q11, xz), decode ≤ 2× min(...), peak RSS ≤ 2× min(...), decoder binary ≤ reference,
> with ≥1 axis strictly better. Subblocking changes **bytes up** and **RSS down**; it does not change
> decode time (§5.3: ≤1.135× at best, and only at the most byte-expensive cap).
>
> Same-job reference costs **[M]** (`docs/I10-AUX-UNBWT-RESULTS.md:594-611`), budget
> `M = 2·min(R_xz, R_br)` **[M]** (same doc, RSS rows):
>
> - **Silesia:** `M = 2 × min(54.3, 124.5) = 108.6 MiB`. Control peak RSS = 248.4 MiB → **2.29× M, FAIL**.
>   Decode 1.7226× vs xz, CI [1.6640, 1.8400] → **inside** the 2× margin, PASS.
>   Bytes 46,466,339 < 48,456,004 (xz) → PASS with **1,989,665 B of headroom**.
>   Subblocking moves only RSS. Best measured RSS = 124.4 MiB (8 MiB cap) → **still 1.15× over M**.
>   To clear `M` the model demands `n + 6C ≤ 108.6` with n = 39.6 MiB → **C ≤ 4.9 MiB**, a regime
>   never probed by the closed sweep and where E4 measured webster at **+12.22%** **[M]**.
> - **enwik8:** `M = 2 × min(66.2, 251.9) = 132.4 MiB`; decode ratio vs xz **3.9065**, CI
>   [3.9003, 4.0088] → **decode gate FAILS by ~1.95×**, and subblocking does not touch decode.
>   8/16/32 MiB additionally *lose the BWT route entirely* (+5.4075%). **enwik8 can never cross via
>   subblocking, by arithmetic.**
>
> So the track's entire value reduces to a single narrow question: *does some cap ≤ ~5 MiB put Silesia
> peak RSS under 108.6 MiB while costing < 1.99 MB of bytes?* Everything else in track 07 is
> methodology. And note the shape of that question: it is not a compression question at all. It is
> "can we shave a memory counter", whose answer — if yes — is a **narrow engineering Pareto point on
> one corpus**, earned with a ratio *regression* on the file that matters, using 25-year-old prior art.

Add to the kill: **the loss is structural, not tuning.** E4 measured the warm-up tax at four
granularities and found it *monotonically worse* as blocks shrink **[M]**
(`docs/audit-2026-09-07/06-do-not-reburn.md:203-208`). §3.2 shows framing is ~0.006% of the tax, so
there is no framing-side optimization available. There is no parameter left to move except the cap,
and the cap's optimum is bracketed by E4 below (expensive) and by the sweep above (insufficient).

---

## 7. Strongest surviving case (why PILOT rather than KILL)

There is exactly one live, quantified, unfalsified hypothesis — and the closed sweep's own grid
**missed the only region where it could be true**:

- **[M]** Silesia is **RSS-bound only**. Decode passes the 2× gate (1.72×, CI upper 1.84). Bytes pass
  with 4.1% headroom. Binary size is not in question (453,512 B stripped, +8,192 B for aux **[M]**).
  One axis is failing, and subblocking is the *only* lever in the entire project that moves it.
- **[M]** The 128 MiB point sits at **1.995× Brotli RSS** (248.4 / 124.5). A ~1.99× ratio that fails a
  2× gate by 0.5% is exactly the kind of near-miss that a memory lever should be aimed at.
- **[M]** The published grid starts at 8 MiB. 8 MiB yields 124.4 MiB — above `M`. The clearing cap
  must be ≈4–5 MiB **[D from the floor model]**, and **that cap was never measured**.
- **[D]** Byte budget is available: reaching ~4 MiB costs webster on the order of +0.5–0.9 MB
  (interpolating E4's +894,315 B at 4 MiB against the sweep's +472,498 B at 16 MiB), against
  1,989,665 B of headroom to xz. **The byte axis can absorb it.**
- **[D]** And two free engineering wins are already on the table and *cost zero bytes*: decode
  sub-blocks **in place** (kill the C-byte `sub` copy, §5.1 item 2), and skip the accumulator
  pre-reserve. Removing 1C from the floor changes the clearing cap from ≈4.9 MiB to ≈7.4 MiB **[P]** —
  i.e. engineering we already owe the decoder may move the answer into a *measured* cap. That
  materially changes the experiment's prior and is not in any artifact.

So the surviving case is narrow, single-corpus, byte-negative, RSS-positive, and prior-art-free of
any novelty — but it is **real, cheap to test remotely, and currently untested in the one region that
matters.**

---

## 8. Decoder / resource / security risks

| # | Risk | Status | Source |
|---|---|---|---|
| R1 | Inner subblocks are decoded **serially**; no overlap ⇒ subblocking cannot buy single-thread throughput | **[M]** confirmed | `src/anvil.cpp:4482-4490` |
| R2 | `sub` double-buffer wastes C bytes of peak RSS at every cap; avoidable at zero byte cost | **[M]** confirmed | `src/anvil.cpp:4487-4489` |
| R3 | Per-subblock aux index grows as `4 KiB × ceil(n/cap)` — the memory lever *adds* wire at small caps | **[D]** | §4 |
| R4 | Hostile `nsub` is bounded (`nsub==0 \|\| nsub>expected \|\| nsub>(e-p)/2`) before any allocation — **adequate** | **[M]** | `src/anvil.cpp:4479` |
| R5 | `dlen`/`plen` bounds and `out.size()!=expected` post-check present; trailing bytes rejected — **adequate** | **[M]** | `src/anvil.cpp:4484-4492` |
| R6 | Peak RSS is **O(n)** with a ≥2n floor at every cap; subblocking is not a bounded-memory mechanism, only a lower-coefficient one | **[D]** | §5.1 |
| R7 | Cap applies to **transformed** bytes (`--ratio-lines` can inflate a block toward `2·out_len+4096`), so effective original-bytes-per-subblock is transform-dependent and not a stable unit | **[M]** + **[D]** | `src/anvil.cpp:4646-4653, 4446` |
| R8 | Subblock boundaries land in transformed space and are **not** byte-aligned in the original ⇒ "subblock = independent decode region of original bytes" is false whenever a transform is on | **[D]** | §5.1 ordering: backend decode completes before inverse transform |
| R9 | No random access / seek: `0xFF` stores only sequential `dlen`/`plen`; offsets must be scanned | **[M]** | `docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md:322-332` |
| R10 | Aux index sampling policy is **fixed at 1024 walks regardless of subblock size**, so small subblocks oversample | **[M]** + **[D]** | integration plan §5 vs §4 |

**[D]** R3/R10 together are a design bug worth naming plainly: the aux sampling policy is defined
per **BWT payload**, but subblocking multiplies BWT payloads, so the policy silently becomes
cap-dependent. A correct policy would target walks per **MiB of source**, i.e. `r ∝ cap`. That is a
real, fixable, byte-negative defect — and it means the published +% figures are *pessimistic* about
subblocking at small caps. It also means the sweep cannot be used to argue subblocking is byte-inefficient
in general. Both directions of error are currently unmeasured.

---

## 9. Benchmark-window provenance audit (where "measured" is weaker than it looks)

| Claim as published | Actual provenance | Verdict |
|---|---|---|
| "128 MiB portfolio identity OK (46,466,339 B)" | genuine re-encode + wire parse, plus a hard equality assert **[M]** | **sound** — the best artifact in the track |
| Portfolio bytes per cap | `route = "bwt" if bwt_bytes < brotli_bytes else "brotli"` computed **in Python**, not by the codec **[M]** (`tools/bwt_subblock_sweep.py:331`) | **reconstruction**, valid only because the tested config disables transforms and `encode_ratio_block` keeps the strict minimum; not a codec measurement |
| Decode time per cap | paired/interleaved vs the 128 MiB script; but the control point is the **mean of the A/A control and candidate medians**, not a direct 128 MiB timing **[M]** (`tools/bwt_subblock_sweep.py:521-524`) | mixed; acceptable, but the reported `decode_s` column is **not** a direct measurement |
| Peak RSS per cap | **one** `/usr/bin/time -f %M` per (cap,file), no repeats, no pairing, no A/A, no per-process attribution **[M]** | **too weak to carry a memory claim**; contradicted by §5.2 |
| RSS vs `M_budget` | `R_xz`/`R_br` come from a **different run** (`35927623136`) than the sweep (`35930672607`) | **cross-job composition**, explicitly disallowed by `IR-…-PREREG.md:266` |
| enwik8 speed/Pareto | blocked by the sweep's own validity rule **[M]** | correctly blocked — do not quote |
| Sweep artifacts | not tracked in-repo; 30-day retention **[M]** (workflow line 170) | already expired or unreviewable; must be regenerated to be re-checkable |
| Fuzz/malformed coverage of the `0xFF` path | hardening passes green; `0xFF` count/length/trailing rejections added **[M]** | **adequate** |

---

## 10. Alternative mechanism, if the bounded-memory question is worth answering at all

The floor model says the reducible terms are `n` (pre-reserved accumulator) and `C` (the `sub`
copy) — *not* the number of subblocks. So the alternative that attacks the actual cost structure is
**I/O restructure, not transform restructure**:

> **Alternative M: single-pass bounded decode that never materializes the whole block.**
> Stream the rev-2 outer block's BWT `0xFF` subblocks, decode each into a **fixed** C-byte ring
> buffer, and write straight to the output file — eliminating the `n` accumulator and the `C` `sub`
> copy. Wire cost: **0 bytes**. Ratio cost: **0 bytes**. Decode-time cost: bounded extra I/O, and
> with the existing aux index the LF walk is already the dominant term.

**[D]** Why this is materially better than more subblocking:
1. It attacks the `2n` floor (the term that **no cap can touch**) instead of the `6C` term;
2. it is **byte-neutral**, so it does not spend the 1.99 MB headroom the byte lever needs;
3. it composes with the outer `decode_threads` path that already exists **[M]**, instead of
   requiring the never-written inner threaded scheduler;
4. it is *still* adopt-class, so it must be labeled as such — but it is adopt-class that costs zero
   ratio, which is a strictly better use of the same engineering.

**[P]** Projection, not evidence: peak RSS at an 8 MiB cap would fall from `n+6C` to roughly
`6C + input-buffer`, i.e. ~48-56 MiB for webster, clearing `M = 108.6 MiB` with **zero** byte cost
and **zero** warm-up tax. If that projection is even half-right, it dominates every point in the
subblock sweep and makes the cap question moot.

**Falsification of the alternative:** it is dead if (a) the decoder must produce the whole block
before writing (container contract), or (b) output ordering forces the accumulator. Both are checkable
in source and neither is currently a stated requirement.

---

## 11. Decisive REMOTE-ONLY experiment (single job, Silesia, one hypothesis)

**Name:** `I10-07B — subblock cap floor resolution + downward grid + byte-neutral decode restructure A/B`.

**Hypothesis H (single, falsifiable):**
> ∃ cap `C ∈ {2,3,4,6,8} MiB` such that Silesia **peak decode RSS ≤ `M = 2·min(R_xz,R_br)` measured in
> the same job**, at a complete-wire byte cost **≤ +2.0% vs the 128 MiB point**, with paired decode
> no worse than 1.05× the control.

**Frozen before dispatch** (per doctrine 5 — these are not to move after seeing results):
- Corpus: Silesia 12 files only. enwik8 is **excluded by arithmetic** (decode gate 3.9065 > 2).
- Candidate tag `i10-aux-unbwt-final-v1`, one checkout, one binary, SHA recorded.
- Flags: `--parse=ratio --ratio-backend=bwt --ratio-context=off --ratio-lines=off --bwt-aux=on`,
  identical to the closed sweep, so the 128 MiB control identity (46,466,339 B) must reproduce exactly.
- Grid: `{2,3,4,6,8,16,128}` MiB. **No cap outside this grid.**
- **Same-job references:** xz -9e and Brotli q11/lw30 encoded and decoded in this job, so
  `R_xz`, `R_br`, `M` are same-run. No cross-job RSS arithmetic.
- **RSS protocol (fixes §5.2/§9):** ≥5 repeats per (cap,file), report median **and** min/max; record
  the allocation breakdown per region (`out`, `sub`, `bwt`, `tmp`, aux index) by instrumenting the
  research build — a run whose peak falls below the §5.1 floor is an **instrumentation failure**, and
  must be reported as such rather than smoothed.
- **Timing:** paired/interleaved, ≥9 reps, 2 warmups, seed fixed, bootstrap 20 000, ε = 2%,
  max CV 0.15, CPU 0, ambient gate 0.10, A/A null included, all reps retained.
- **Per-file output is mandatory**; portfolio ratios are secondary (§3.3).
- Mandatory columns: `aux_index_bytes` and `postcoder_bytes` **separated** (fixes §4), plus
  `0xFF` framing bytes, subblock count, per-file bytes at every cap.
- Second, independent arm (zero byte cost, may run in the same job): **Alternative M** — in-place
  subblock decode (delete the `sub` copy) — measured at caps `{8,16,128}` for bytes (must be
  **identical** to control: byte-identity is the gate) and peak RSS.
- Third arm, cheap and decisive for the "only engineering" question: **cap-relative aux sampling**
  (`r ∝ C`, targeting 1024 walks per 8 MiB rather than per subblock). Gate: does it recover any of
  the small-cap byte tax? This tests whether the published curve is an artifact of a mis-specified
  policy rather than a property of subblocking.

**Pre-registered thresholds (mechanical, evaluated in this order):**

| Gate | Condition | Outcome if failed |
|---|---|---|
| **G0 identity** | 128 MiB control == 46,466,339 B; all arms roundtrip byte-exact; A/A CI spans 1.0 | `BLOCKED-INFRA` |
| **G1 floor consistency** | every (cap,file) peak RSS ≥ the §5.1 floor (`n+6C`, or `6n` unsplit) **−5% tolerance** | `RSS-ARTIFACT-INVALID` → **the entire existing memory series is retracted**; no memory claim may be promoted |
| **G2 RSS gate** | median peak RSS ≤ `M` (same job) | no candidate cap → `NO-GO-SUBBLOCK` |
| **G3 byte gate** | complete bytes ≤ **+2.0%** vs 128 MiB control | cap disqualified |
| **G4 decode gate** | paired decode ratio CI upper ≤ **1.05** (no material regression) | cap disqualified |
| **G5 attribution** | `aux_index_bytes` + `postcoder_bytes` + framing account for ≥99% of the byte delta | result is **INCONCLUSIVE** on mechanism |
| **G6 per-file honesty** | the RSS/byte effect is reported for **every** file that splits, not only portfolio aggregates | result is **INCONCLUSIVE** on generality |
| **G7 Alternative M** | in-place decode is byte-identical **and** reduces median peak RSS by ≥10% at fixed cap | alternative is dead → the `n` floor is structural |
| **G8 frontier honesty** | even a G2+G3+G4 pass is labeled `GO-SUBBLOCK-BALANCED-ENGINEERING`, **never** `FRONT-CROSSING` | mislabeling = reporting failure |

**Promotion thresholds (all required, no substitution):**
1. G0–G6 pass for **one named cap**;
2. that cap's byte cost is reported **per file** and the affected file's regression is ≤ +2.0%;
3. `M` is same-job;
4. the result is filed as adopt-class engineering with an explicit "no novelty claim" header.

**Kill thresholds (any one fires immediately):**
- **K1** G1 fails → the existing RSS evidence is retracted; **all** subblocking memory claims, including
  the "8 MiB roughly halves RSS" sentence in `docs/I10-AUX-UNBWT-RESULTS.md:625-627` and
  `docs/I10-BREAKTHROUGH-PROGRAM.md:636-639`, must be corrected in place by their owners.
- **K2** G2 fails at every cap → subblocking cannot clear the memory gate in the probed range →
  **KILL bounded BWT subblocking as a Pareto lever**; retain 128 MiB default permanently.
- **K3** G3 fails at the smallest cap that passes G2 (i.e. clearing `M` requires > +2% bytes) →
  **KILL**: the lever is a byte-for-memory swap with no net frontier movement.
- **K4** G5 shows the byte tax is *not* postcoder warm-up → the mechanism is unidentified →
  **KILL** pending a new hypothesis, not a re-run.
- **K5** Any result is quoted as a crossing → **KILL the lane** (doctrine 2/§11).

**Explicitly out of scope** (do not let a passing run expand into these): threaded inner subblock
decode; random access; backend replacement; transform changes; new corpus; any source edit to
`src/anvil.cpp` outside an isolated prototype.

---

## 12. Current leaning

**PILOT** — narrowly, for the single remote job in §11, and *not* for any cap adoption.

Reasoning, stated as inference:
1. **Not KILL** because one live hypothesis survives that the closed sweep's grid structurally could
   not have found: the clearing cap (≈4–5 MiB, or ≈7 MiB after free in-place engineering) lies below
   the sweep's 8 MiB floor, and Silesia's byte headroom (1.99 MB) plausibly absorbs the tax. A cheap
   remote job can settle it.
2. **Not HOLD** because the memory evidence that currently justifies the direction is **not
   admissible** (§5.2: single-shot, cross-job, and below an allocation floor derivable from the
   shipped source at its two most favorable points). Holding on that basis would be holding on a
   number I have shown to be internally inconsistent.
3. **Not PROMOTE-TO-REMOTE as a mechanism** because there is no mechanism: prior art is total (§2),
   the loss is structural (§3.2), and the whole surface is one integer (§1). The correct label for
   any pass is adopt-class engineering.
4. **Enwik8 is dead by arithmetic** — decode gate 3.9065 vs a 2× ceiling, untouched by subblocking —
   and must be removed from this track's success criteria.

**What would flip me:** G1 failure ⇒ immediate **KILL** plus a correction request on two published
docs. G2+G3 both failing ⇒ **KILL**. G7 passing ⇒ the byte-neutral Alternative M becomes the
recommended mechanism and the cap question is dropped.

**Superseded in part by §13/§14/§15** (post-pair-reconciliation, after reading the constructive
report and the libsais source). The leaning is unchanged at PILOT for the cap-floor question in §11,
but the checkpoint-index arm (their H-1) is **KILLed as stated**, their H-3 is **void as formulated**,
and their Silesia leg of the structural NO-GO is **overturned in mechanism (unproven) while its
conclusion survives over the probed grid**.

---

## 13. Adjudication of the constructive lane's H-1 (the ~99% claim)

**The claim under audit** (`07-bwt-subblocking-space-bunny.md:120-125, 581`): the 1024-walk policy
"over-provisions the index by roughly two orders of magnitude", so the charged aux cost "can be cut
from thousands of bytes to tens of bytes at predicted-equal decode speed", with arm A5
(≈`icount` 12, ≈48 B) as the decisive test.

**Verdict: FALSE as stated, in a specific and quantified way. The core intuition is TRUE. Arm A5 is
predicted ~2× SLOWER and would manufacture a false falsification.** What survives is worth ~3 KB per
file and should not consume a remote lane (§13.5).

### 13.1 What I verified myself, in `third_party/libsais/src/libsais.c`

| Fact | Line | Note |
|---|---|---|
| `r` **must** be a power of two | `8042`: `((r != n) && ((r < 2) \|\| ((r & (r - 1)) != 0)))` | hard library constraint; confirms their power-of-two trap |
| Exactly `B = 1 + (n-1)/r` seeds are validated | `8053`: `for (t = 0; t <= (n - 1) / r; ++t) { if (I[t] <= 0 \|\| I[t] > n) return -1; }` | **no slack, no optional entries** |
| `B` blocks, dispatched 8 at a time | `7887`: `while (blocks > 8)`; `7891`: `I += 8; blocks -= 8;` | confirms their S7 |
| Kernel width saturates at 8 | `7854` `libsais_unbwt_decode_8`; `7867-7877` 8 replicated chains; `7894-7940` tails of width 1–7 | confirms their S8 |
| Legacy path is a **single** chain | `8030-8033`: `libsais_unbwt(...)` = `libsais_unbwt_aux(..., n, &i)` ⇒ `r == n`, `B == 1`, `decode_1` | confirms their S8 |
| `U0..U7` are spaced exactly `r` apart | `7857-7863` | this is what makes the strided tail decomposition correct |

### 13.2 Are the samples logically necessary at the proposed density? **YES — 1:1 with chains.**

Block `j` covers output bytes `[j·r, min((j+1)·r, n))` and is inverted by an LF chain seeded **only**
by `I[j]` (`7889`: `i0 = I[0], i1 = I[1], … i7 = I[7]`). `I[j]` cannot be derived from `I[j−1]` at
decode time except by performing the `r`-step walk it encodes — which is exactly the work being
parallelised. Therefore:

> **`index_bytes = 4 × (number of parallel LF chains)`, exactly.**

**The aux index is not "metadata about the BWT". It *is* the parallel schedule.** There is therefore no
independent "index density" knob: reducing density and reducing ILP are the *same operation*. The floor
is `4 × 8 = 32 B` (8 chains = kernel width), reachable only when a power of two lands in
`((n−1)/8, (n−1)/7]`.

**Credit where due:** in the aux form the primary index **is** `I[0]` (`src/anvil.cpp:4366`), so the aux
representation already charges **zero** primary bytes — as they state (S3). The track mandate's
"primary-index charge" line is **already fully resolved**; there is nothing left to remove.

### 13.3 The kill: `r` must be a power of two, so reachable chain counts are sparse and n-dependent

Because `r = 2^k` only, reachable chain counts are `B = 1 + ⌊(n−1)/2^k⌋` — a **sparse set whose members
depend on the binary digits of `n`**.

Isolated integer-only check: `prototypes/swarm-2026-10-02/07-bwt-subblocking/fledge/fledge_h1_check.py`
(no corpus, no codec, no timing; it replays the `libsais_unbwt_decode` control flow literally and costs
each invocation `k / min(m, 8)`):

| target | n | B incumbent | index | **best reachable B within +2%** | index | **cut** | B ignoring width | predicted cost |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| dickens | 10,192,446 | 623 | 2,492 B | **312** | 1,248 B | **49.9%** | 1 (4 B) | 63.5× |
| webster | 41,458,703 | 633 | 2,532 B | **40** | 160 B | **93.7%** | 1 (4 B) | 60.3× |
| enwik8 | 100,000,000 | 763 | 3,052 B | **24** | 96 B | **96.9%** | 1 (4 B) | 62.4× |
| probe `2^27` | 134,217,728 | 1024 | 4,096 B | **8** | 32 B | **99.2%** | 1 (4 B) | 64.0× |
| probe `2^27+2^20` | 135,266,304 | 516 | 2,064 B | **516** | 2,064 B | **0.0%** | 1 (4 B) | 62.5× |

**Four falsifications of "~99%, data-independent, at equal speed":**

1. **dickens: 49.9%, not ~99%.** No reachable `B ≡ 0 (mod 8)` exists below 312; the next candidates
   are absurd (`B = 1,274,056`).
2. **probe `2^27 + 2^20`: exactly 0%.** The incumbent `B = 516` **is already the reachable optimum**.
   The policy is not universally over-provisioned — on some inputs it is already correct, so the
   "defect" is a no-op.
3. **The real range is 1.0× – 64×**, not "two orders of magnitude", and it is a function of `n`'s
   binary digits, not of any algorithmic property.
4. **Arm A5 is predicted 2.0× SLOWER, not equal.** `B = 12` = one full 8-wide invocation **plus a
   width-4 tail invocation**, because `7891` decrements by 8: `12 → 4`. `B = 24` (`r = 2^22`) is a
   clean multiple of 8 and costs **0.990×** the incumbent. Their `selftest` assertion
   `sat-icount-ge-8` is satisfied by `B = 12` while the width actually achieved is 4 + 8.

   | arm | r | B | index | `B mod 8` | predicted rel. cost |
   |---|---:|---:|---:|---|---:|
   | A1 incumbent | 131,072 | 763 | 3,052 B | 3 | 1.000 |
   | **A5 (proposed)** | 8,388,608 | 12 | 48 B | 4 | **1.999** |
   | **A4 (correct)** | 4,194,304 | 24 | 96 B | **0** | **0.990** |

   Running SBW-G1 exactly as preregistered would report **H-1 FALSIFIED** for a claim that is true in
   corrected form. That is a preregistration defect, not a mechanism defect.

### 13.4 What survives, stated correctly

> **H-1′ (corrected, source-verified, unmeasured):** the whole-codec decode benefit of
> `libsais_unbwt_aux` saturates at the 8-chain kernel width, so the incumbent 1024-walk policy
> over-provisions the checkpoint index by a factor that is **1.0× – 64× and depends on the binary
> digits of each block length**. Choosing `r` as the largest power of two with `B ≡ 0 (mod 8)`
> preserves the full-width schedule and cuts the charged index on the three measured targets by
> **49.9% / 93.7% / 96.9%**, at predicted whole-codec decode cost **1.003× / 1.000× / 0.990×** — inside
> the project's own 2% practical gate.

Real, cheap, correct engineering. **Not** "~99%", and **not** what A5 tests.

### 13.5 Materiality kill: the entire prize is ~3 KB per file

| Quantity | Value | Source |
|---|---:|---|
| Total charged aux wire, 3 paired targets | **8,083 B** | `I10-AUX-UNBWT-RESULTS.md` §6 **[M]** |
| enwik8 aux charge as % of payload | **0.01297%** | 3,054 / 23,537,422 **[M]** |
| enwik8 byte margin already won over Brotli | **1,272,758 B** | 24,810,180 − 23,537,422 **[M]** |
| ⇒ H-1's *maximum* possible value | **0.24% of an already-won margin** | **[D]** |
| Silesia charged aux (7 files) | 19,344 B = **0.042%** of the 1,989,665 B xz margin | **[D]** from **[M]** |
| Memory prize | **zero** | `P`/`tmp` is `4n` regardless of `r` — their S6, which I verified as orthogonal and **credit** |

**Risk/reward asymmetry, plainly:** upside ≤ 3 KB of bytes per file and 0 bytes of memory; downside =
losing a **1.364×–2.339×** measured decode speedup. Roughly **five orders of magnitude against**.
And the proposed test spends a 5-arm paired timing sweep — the most expensive measurement class in
this project — to resolve a difference whose entire prize is 0.013% of one payload.
**The demanded precision exceeds the value at stake.**

### 13.6 Ruling on H-1

- **KILL** the claim "cut from thousands of bytes to tens of bytes at equal decode speed, at any `n`".
- **KILL** arm A5 / `icount ≈ 12` as the decisive test; substitute A4 / `B = 24` (or, per target, the
  largest reachable `B ≡ 0 mod 8`).
- **SALVAGE** H-1′ as a one-line encoder-policy correction bundled with H-2. Adopt-class, zero
  novelty, ~3 KB/file — a **policy bug-class fix**, not a mechanism, not a lane.
- **DO NOT DISPATCH SBW-G1 as a timing sweep.** If verified at all, verify **bytes + correctness
  only**: confirm `libsais_bwt_aux(T, ·, r, ·)` returns the **same `T` for every reachable `r`** (so the
  byte delta is exactly `4·ΔB` and nothing else moves), and that every reachable `B` roundtrips
  byte-exactly through the existing malformed battery. If `T` is **not** `r`-invariant, the byte
  saving is not clean and the correction is void.
- **My own falsifier:** if a timing run is ever authorised, **A4 must land within ±2% of A1**. If A4
  is materially slower, my dispatch-cost model is wrong and the entire reachability table above —
  including the 49.9% / 93.7% / 96.9% figures — is void.

---

## 14. Reconciliation with the constructive lane

`docs/swarm-2026-10-02/07-bwt-subblocking-space-bunny.md`, read in full. Per-claim verdict.

### 14.1 AGREEMENTS (independent convergence — the strongest evidence in this track)

| Their claim | My independent finding |
|---|---|
| Prior art is total; all but H-1/H-2/H-3 are adopt-class (§6) | **AGREE** — my §2 reached the same map independently (bzip2 900 k, FM-index, induced-sorting block+merge, sampled LF walking) |
| Ratio tax is **statistical warm-up, not transmitted model bytes** (§4.3) | **AGREE** — my §3.2 derived this from framing arithmetic (~80 B vs a 1.27 MB delta) |
| Framing negligible at every `B` (§5.1) | **AGREE** — my §3.2, same magnitude |
| Aux index grows `Θ(B)` under partitioning (H-2) | **AGREE, numerically identical** — my §4 predicted `4 KiB × ⌈n/cap⌉`; their enwik8@64 MiB table (3,052 → 8,112 B) matches my model exactly. Two independently built models, same answer |
| `r` controls ILP width, not the `4n` buffer ⇒ **index and memory are exactly orthogonal** (§1.3) | **AGREE — and I under-weighted it.** A genuinely good observation; it *strengthens* my §5.1: the `4C` term is irreducible, so the cap lever cannot reach O(1) memory |
| libsais S7/S8 (chunked dispatch, 8-wide saturation, legacy = 1 chain) | **AGREE** — verified at `libsais.c:7887`, `7854-7880`, `8030-8033` |
| Denominator ambiguity in M4/M5 flagged, not silently resolved (§9.3) | **AGREE** — good discipline; their conclusion is robust to it, because the source reading is worse for their own NO-GO |
| Subblocking cannot be the memory-frontier lever (§5.4, enwik8 leg) | **AGREE on enwik8** — `max_cap_within_budget` = 7.41 MiB ⇒ `B = 14`, while one measured boundary at 64 MiB already costs ~759 KB = **59% of the entire 1,294,226 B byte margin**; thirteen more are arithmetically impossible. Clean arithmetic, endorsed |

### 14.2 DISPUTES

**D1 — H-1's magnitude and its arm A5: FALSE.** See §13. Over-provisioning is 1.0×–64× and
n-dependent; `B = 12` is predicted 2× slower. Their `selftest` checks the wrong invariant
(`icount ≥ 8` instead of `icount ≡ 0 mod 8`).

**D2 — H-3 is self-contradictory and, as formulated, undefined for this pipeline.** Their §4.3
correctly states the penalty "is dominated by **lost long-range suffix context**". Their H-3 then
prices the boundary as `Damage(c) = #{LZ reference spans (s,e) : s ≤ c < e}` — **there is no LZ parse
in the ratio/BWT path.** `--parse=ratio --ratio-backend=bwt` sends the block straight to
`bwt_backend_encode` (MTF+RLE+post, `src/anvil.cpp:4435-4457`); no match set exists to damage. Their
AF-3 ("the probe is not ANVIL's match set") understates it: the statistic has **no referent** in this
pipeline. A valid reformulation needs a **BWT-suffix** boundary statistic (LF-walk edges or
induced-sorting dependencies crossing the cut; change in MTF-rank entropy at the cut) — a different
and substantially harder quantity. **H-3 as written is void, not merely uncertain.**

**D3 — the Silesia RSS-floor resolution is internally inconsistent.** Their §5.3 attributes the
~125 MiB Silesia floor to the five Brotli-routed files. Test it against their own four points:
- cap 8 MiB: webster floor (α=6) 87.6 MiB < measured 124.4 ⇒ max plausibly Brotli. **Works.**
- cap 16 MiB: webster floor **135.6 MiB > measured corpus max 125.7 MiB** ⇒ the maximum is *below* a
  webster-only floor. Brotli routes **cannot** explain a max under the BWT route's own floor. **Fails.**

The fitted constant does not rescue it: with α=6 the Silesia 16/32 MiB rows are **arithmetically
impossible** (floors 135.6 and 231.6 MiB vs measured 125.7 and 203.4); with **their** α=5 those rows
fit (103.6, 199.6) but the enwik8 128 MiB row is then 4.7% off and the 64 MiB row 1.0% off. **No
single allocation model fits all four points.** We agree the series is anomalous; we disagree on the
cause; neither of us has an instrumented measurement. Verdict: **unresolved — which is exactly why my
§11 G1 exists.** Their N-BW-1 Silesia leg is therefore **overstated in mechanism**: the conclusion
"no *measured* cap passes" stands, the structural mechanism does not, and the 4–5 MiB region remains
unprobed.

**D4 — M7 (8 MiB is 11.9% faster) is treated as a real speed axis (§5.5.2) on one unreplicated
point.** If the 11.9% were cache residency of the `6C` working set, 16 MiB (96 MiB) and 32 MiB
(192 MiB) should show *partial* gains on a 32–256 MiB EPYC L3. They show **none** (4.2232 / 4.2104 vs
4.2015 s). A three-way structure (bytes↑, RSS↓, decode↓) is being built on a point that (a) has a
single paired measurement against a control whose own value is the **mean of an A/A pair**
(`tools/bwt_subblock_sweep.py:521-524`), and (b) sits in a series whose RSS sibling is already shown
inadmissible. Treat M7 as **BLOCKED pending replication**, not as an axis.

**D5 — an interaction missing from their failure list.** H-2's *global* stride must satisfy
`r ≤ min(piece size)` per libsais call (`libsais.c:8042`, plus ANVIL's `2 ≤ r ≤ expected` at
`src/anvil.cpp:4352-4354`). Under H-3's bounded-deviation placement, pieces lie in `[C/2, 2C]`, so a
global `r` is capped at `C/2`. H-2 and H-3 are **not independent**; AF-5 states the degeneracy check
for H-2 alone and misses this.

### 14.3 NOT VERIFIED BY ME (no dispute, simply unchecked)

Their S6 (`P` fully written for all `n` entries, `libsais.c:7515-7569`) — plausible and consistent with
the `tmp(expected+1)` allocation I did verify at `src/anvil.cpp:4391`, but I did not read
`libsais_unbwt_init_single`. Their S10 (postcoders 0 and 4 dead) — not checked. Their
`sbw_model.py` `damage` probe — not run.

### 14.4 Net effect of the pairing on my own conclusions

- §11's **PILOT** stands, now *narrower*: the memory lever is dead on enwik8 by arithmetic (their
  N-BW-1, endorsed), the Silesia floor cause is unresolved, and the one cheap lever with real content
  is my Alternative M (§10), which neither report costed as the primary candidate.
- §4 (aux index `Θ(B)`) is **independently confirmed** — the strongest single agreement in this track.
- §13 replaces their H-1 with a 1.0×–64×, n-dependent correction worth ~3 KB/file.
- Their H-2 is **confirmed** and should proceed as a one-line policy fix bundled with H-1′: pure
  arithmetic, no experiment, and it stops future subblocked arms from being measured with a knowingly
  inflated charge.

---

## 15. Revised recommendation and thresholds after reconciliation

**Track 07 overall: PILOT, scoped to three items, with four explicit KILLs and two corrections.**

**KILL (explicit):**
1. **H-1 as stated** — "~99% index reduction at equal decode speed, data-independent". False
   (49.9% / 93.7% / 96.9%; 0% on some `n`); arm A5 predicted 2× slower. **KILL.**
2. **H-3 as formulated** — priced on an LZ reference set that does not exist in the ratio/BWT
   pipeline. **KILL pending reformulation in BWT-suffix terms.**
3. **"Subblock to meet `M_budget`"** — enwik8 leg endorsed by arithmetic (14 boundaries needed, 1
   affordable); Silesia leg holds empirically over the probed grid but its stated mechanism is
   unproven. **KILL the memory-frontier framing.**
4. **SBW-G1 as a 5-arm paired timing sweep** — not dispatched. Prize ≤3 KB/file (0.013% of one
   payload); risk = a 1.4–2.3× decode regression. If verified at all, verify **bytes + correctness
   only** (§13.6).

**PILOT (now, cheap, no production edit, no remote dispatch):**
1. **H-2 + H-1′ as one encoder-policy correction** — charge the checkpoint budget globally and pick
   `r` as the largest power of two with `B ≡ 0 (mod 8)`. Both are arithmetic on already-measured `n`;
   neither needs a timing experiment. Bundle them so no future subblocked arm is measured with a
   knowingly inflated or wrongly-quantised aux charge.
2. **Alternative M (§10)** — in-place subblock decode plus streaming assembly. Byte-neutral, attacks
   the `2n` floor that **no cap can touch**, and is the only item in either report aimed at the term
   that actually binds. Remains my primary candidate.
3. **My §11 remote job**, unchanged, Silesia-only, with G1 as the first gate.

**Pre-registered thresholds — unchanged from §11 except as added:**
- **K1** (any measured point below the allocation floor) ⇒ retract the published RSS claims in
  `docs/I10-AUX-UNBWT-RESULTS.md:625-627` and `docs/I10-BREAKTHROUGH-PROGRAM.md:636-639`, and
  **withdraw the Silesia leg of N-BW-1** pending instrumentation.
- **K2** (no cap clears same-job `M`) ⇒ KILL bounded BWT subblocking as a Pareto lever; the 128 MiB
  default stands permanently.
- **K3** (clearing `M` requires > +2% bytes) ⇒ KILL; the lever is a byte-for-memory swap with no
  frontier movement.
- **New K6** — for H-1′: if `libsais_bwt_aux` does **not** return an `r`-invariant `T`, the byte
  saving is not `4·ΔB` and the correction is **void**. Bytes-only check; the only verification H-1′ needs.
- **New K7** — for Alternative M: if in-place decode is not byte-identical, or does not cut median
  peak RSS by ≥10% at fixed cap, the `2n`/`6C` floor model is wrong and the memory discussion in both
  reports must be reopened.

**Final label: PILOT — no novelty claim, no frontier claim, and an explicit KILL on the constructive
lane's headline mechanism claim.**