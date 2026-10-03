# Track 05 — Backend Frontier Beyond Brotli — Fledge Alpha Free (independent adversarial review)

**Author:** Fledge Alpha Free (independent adversarial reviewer), track `05-backend-frontier`
**Date:** 2026-10-02 · **Tree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty
**Files created:** this report only. No existing file modified. No commit/push. No local
benchmark, sweep, or fuzz run.
**Role discipline:** I did not use the constructive report as evidence. I reconstructed the
backend evidence from `RESEARCH_LEDGER.md`, `docs/I10-AUX-UNBWT-RESULTS.md`,
`docs/I10-AUX-UNBWT-INTEGRATION-PLAN.md`, `docs/I10-FRONTIER-RECON-2026-09-24.md`,
`docs/audit-2026-09-07/06-do-not-reburn.md`, `tests/bwt-backend-standard.csv`,
`tests/benchmark-suite.frozen-bdc90474.plus-xz.csv`, `src/anvil.cpp`, and
`scratch/bwt-subblock-35930672607-*`. I then read
`docs/swarm-2026-10-02/05-backend-frontier-space-bunny.md` (incl. §11–§12 addenda) and
reconcile explicitly in §9.

**Evidence labels:** `[M]` measured fact with artifact · `[M+arith]` arithmetic on measured
inputs · `[D]` derived/structural (provable, not measured) · `[P]` projection · `[A]`
assumption, unverified · `[X]` falsification.

---

## 0. Verdict in one paragraph

The constructive lane's **load-bearing inference is sound in direction and wrong in
magnitude**: it treats "the inverse-BWT LF walk is one dependent random access per output
byte" as the decode cost model, and from that derives a 3.33× whole-corpus target. The
project's own measured decoder-floor profile (`RESEARCH_LEDGER.md:3911-3931`) shows the
opposite structure for the *other* backend path — **CRC-32 ≈ 44%, stream materialization
≈ 26%, token loop ≈ 11%, opcode entropy pulls ≈ 0.3%** — i.e. entropy/dispatch is
**0.3% of end-to-end decode**, not a bottleneck. Track 06's finding that default stream
selection uses **length-independent** ns/B constants then propagates that mis-scaling into
every *selection* decision, which is a different failure from a timing-measurement failure.
My central kill argument is therefore not about WSI-BWT's code; it is that **no backend in
this project has ever had its decode time decomposed by stage**, so the one number the
pilot is gated on (`t_walk` share of backend decode) is currently unmeasured, and the
project has already spent three cycles rediscovering that unmeasured per-token structure is
not where the time is. Recommendation: **PILOT** — authorize exactly one paired remote
cost-reconciliation pass, and require the stage decomposition as a *gate input*, not a
narrative appendix.

---

## 1. Independent evidence map (what I verified myself)

| # | Fact | Value | Artifact | Label |
|---|---|---|---|---|
| V1 | Frozen ANVIL portfolio bytes | Silesia **46,446,995 B**, enwik8 **23,534,368 B** | `docs/I10-REMOTE-BASELINE-CLOSURE.md`, recon §1 | `[M]` |
| V2 | Frozen references | Brotli q11/lw30 49,383,136 B; xz-9e 48,456,004 B; zstd u22/l27 52,364,240 B (Silesia) | same | `[M]` |
| V3 | ⇒ byte lead | Silesia **−5.94%** vs Brotli, −4.14% vs xz, −11.30% vs zstd | V1/V2 | `[M+arith]` |
| V4 | Canonical frontier state | **0 FRONT-CROSSING**; Silesia `FRONT-GAP_COST`, enwik8 `TIMING_BLOCKED` | recon §1 | `[M]` |
| V5 | BWT routing | **7/12** Silesia files route to BWT; **0** route changes under aux | `I10-AUX-UNBWT-RESULTS.md` §8 | `[M]` |
| V6 | BWT-routed input share | 7 files = **120,362,284 B** of 211,938,580 B = **56.79%** | `docs/audit-2026-09-07/06-do-not-reburn.md` §F0.1 | `[M]` |
| V7 | Aux wire cost | dickens **+2,494 B**, webster **+2,535 B**, enwik8 **+3,054 B**; **+8,083 B** total | AUX-RESULTS §6 | `[M]` |
| V8 | Aux whole-decode speedup (paired, same-job) | 1.364× / 1.795× / 2.339×; encode neutral | AUX-RESULTS §3–§6 | `[M]` |
| V9 | Aux text growth | +2,976 B `.text`, +8,192 B ELF file | AUX-RESULTS §9 | `[M]` |
| V10 | Aux ruling | `FRONT-GAP_COST` both corpora; ANVIL/Brotli decode **3.4474×** [3.2680, 3.5059]; ANVIL/xz 1.7226× | AUX-RESULTS §11 | `[M]` |
| V11 | Decode peak RSS | Silesia ANVIL **248.4 MiB** vs Brotli 124.5, xz 54.3 (4.572× / 1.995×); enwik8 ANVIL **598.9 MiB** (9.043× xz, 2.378× Brotli) | AUX-RESULTS §11 | `[M]` |
| V12 | Subblock sweep (Silesia) | 128 MiB 46,466,339 B / 4.2015 s / 248.4 MiB · 16 MiB **46,938,837 B (+1.0169%) / 4.2232 s / 125.7 MiB** · 8 MiB 47,365,274 B (+1.9346%) / **3.7022 s** / 124.4 MiB | `scratch/bwt-subblock-35930672607-silesia/summary.md` | `[M]` |
| V13 | ⇒ tiling trade | 16 MiB: **+1.0169% bytes, +0.52% decode time, −49.4% peak RSS** vs 128 MiB | V12 | `[M+arith]` |
| V14 | Subblock sweep (enwik8) | 8/16/32 MiB **route away from BWT** (+5.4075% bytes, 0 BWT routes); 64 MiB retains BWT at +3.2259%; 128 MiB baseline | `scratch/.../enwik8/summary.md` | `[M]` |
| V15 | **Decoder-floor profile (mode-15 / semantic path)** | **crc32 ≈ 44%** (1.86–1.91 ns/B, per-byte, stable) · **eager macro-stream materialization ≈ 26%** · **token loop beyond setup ≈ 11%** · **opcode entropy pulls ≈ 0.3%** · concat/alloc/headers ≈ 15% | `RESEARCH_LEDGER.md:3924-3931` | `[M]` |
| V16 | Noise floor of that profile | within-run CV 0.6–10%; run-to-run drift ±2–4%; **zero-byte-change control variants moved +2.3–8.1%** | `RESEARCH_LEDGER.md:3918-3923` | `[M]` |
| V17 | J-objective cost constants are mis-scaled | incumbent constants "**right ORDER, wrong GAPS**"; frozen fits rANS-4096 **5.93–6.62 ns/B**, ctx-rANS **~7–8**; recommended fits `{0.1, 6.0, 6.0, 6.0, 4.3, 3.2, 7.5}` | `RESEARCH_LEDGER.md:3889-3905` | `[M]` |
| V18 | Known calibration defect, recorded not patched | "the flat 6.0 rANS fit **misses a size-dependent symtab effect** (rANS-4096 ≈5–12% slower/B than 512/256 on small streams)" | `RESEARCH_LEDGER.md:3906-3909` | `[M]` |
| V19 | Backend registry in production | `kRatioBackendBrotli=1`, `kRatioBackendBwt=2` (`src/anvil.cpp:3856-3857`); `encode_ratio_block` considers transform 0/1/2 × every enabled backend (`:4614-4639`); transform 3 (LZP) hard-disabled (`:4630-4634`) | source read | `[M]` |
| V20 | Ratio block size | `opt.block_size = 128 MiB` for `--parse=ratio` ⇒ **one BWT per Silesia/enwik8 file** | `src/anvil.cpp:4999-5000` | `[M]` |
| V21 | Entropy backends already present | rANS 4096/512/256, Huffman, default-with-sparse-exceptions, pair-rANS, ctx-rANS (clustered contexts), raw | `RESEARCH_LEDGER.md:3870-3876`, Exp. AA table | `[M]` |
| V22 | BWT-direct decode latency | 84.8–88.8 ns/B on 10–41 MB inputs; decode peak RSS 5.99–6.35× source | `tests/bwt-backend-standard.csv` | `[M]` **ranking-grade, LOCAL-ABL, 100 ms timer** |
| V23 | QLFC and LZP postcoders | **NO-GO measured**: QLFC +2,437,384 B (+3.23%), 0/13 win; LZP +279,419 B (+0.37%), wins 4/13 | do-not-reburn §G2 | `[M]` |
| V24 | Block-local backend routing | **decisive negative**: webster BWT wins 10/10 4 MiB blocks yet sum-of-blocks is **+894,315 B (+12.22%)** worse than whole-file; mozilla/samba/ooffice/sao have **zero** BWT-winning blocks | do-not-reburn §G1 | `[M]` |
| V25 | DEFLATE reconstruction | adopt-class prior art; ~1.36 MB recovered on mozilla | `FRONTIER-RESET-2026-09-23.md` §2.4 | `[M]` |
| V26 | CM/PPM rate gap | PAQ-class ≈28 MB Silesia vs ANVIL I9 ≈46.4 MB — a ~40% gap | `docs/I10-BREAKTHROUGH-PROGRAM.md` §1 | `[M]` |
| V27 | Grammar/RLZ-as-default | ratio win but **decode-loss 8.1–15.9%** on primary record files, encode 48–64× slower; root cause: eager whole-buffer materialization loses to fused per-byte pulls | Exp. Y | `[M]` |
| V28 | Context clustering non-novel | ruled non-novel in-project; Brotli's own context map is the cited prior art | `RESEARCH_LEDGER.md` Exp. J, PART II §3 | `[M]` |
| V29 | G-lane backend pinning | G0–G3 all pinned **Brotli q11/lgwin30**; G1 −1.6564%, G2 −2.007746% (both NO-GO vs −3% bar) | `docs/I10-BREAKTHROUGH-PROGRAM.md` §10 I10-1C | `[M]` |
| V30 | Transformed-reference ±BWT null | mode-17 cross-product exists in production and "No new ratio-transform win appeared in the baseline itself" | `docs/I10-REMOTE-BASELINE-CLOSURE.md:32-37` | `[M]` |

**What is NOT in the evidence base (the gap this report is about):** there is **no measured
stage split of BWT backend decode**. `tests/bwt-backend-standard.csv` reports only whole-file
`decompress_s`. The split between (a) postcoder decode, (b) ISA construction, (c) LF walk,
(d) allocation/CRC/framing **has never been measured on either backend path** `[X: my
finding]`.

---

## 2. The strongest surviving case (constructive lane, stated at its best)

I will state the strongest form of the opposing case before attacking it, because it is
genuinely strong on two points.

**Survivor-1 — the byte lead really is a backend effect, and I independently confirm it.**
BWT wins on 7/12 files (V5), carries 56.79% of input bytes (V6), and the byte lead is
−5.94% vs Brotli (V3). The remaining 5 files are essentially at Brotli (mozilla: ANVIL
13,806,173 vs Brotli 13,806,141 — 32 B). Therefore: **the ratio result of this project is
produced by one backend on 56.79% of the corpus.** Any claim that "the backend is not the
mechanism layer" is refuted. This is the strongest thing in the constructive report and it
survives my audit intact. It also implies the constructive report's §2 reframing is correct:
**budget spent on frontend×Brotli cells is largely redundant**, because mode 17 already
offers transform×backend and it produced nothing (V30).

**Survivor-2 — the memory-axis trade is real, cheap, and already paid for.** 16 MiB tiling
gives −49.4% peak RSS for +1.0169% bytes and +0.52% decode time (V13). This is a *measured*
Pareto point with a *small* byte charge, and it is orthogonal to any decode mechanism. It is
the single best-evidenced backend result in the project after the aux index itself, and it
is independent of whether WSI-BWT works.

---

## 3. The strongest falsification case (my kill argument)

### 3.1 The measured profile contradicts the assumed cost structure of *this project's* decoder

`RESEARCH_LEDGER.md:3924-3931` is the only end-to-end decoder decomposition this project has
ever produced, and it is unambiguous `[M, V15]`:

| component | share of mode-15 end-to-end decode |
|---|---|
| CRC-32 | **≈44%** |
| eager macro-stream materialization | **≈26%** |
| token loop beyond setup | **≈11%** |
| opcode entropy pulls (the dispatch/entropy cost) | **≈0.3%** |
| concat / alloc / headers | ≈15% |

The constructive mechanism's entire thesis is that per-symbol entropy/dispatch cost is the
lever ("remove per-iteration entropy-coded symbols"). On the measured path, **that cost is
0.3% — three orders of magnitude below the threshold at which it could matter.** Any track-05
claim that implicitly assumes entropy/dispatch dominates decode is falsified by V15 *for the
semantic path*. Track 05's WSI-BWT acts on the **BWT path**, which V15 does not cover — but
that is precisely the problem: **the project has generalized from one path's measured profile
to a second path's assumed profile without measuring it.**

### 3.2 The 3.33× target is arithmetically correct and strategically wrong

`[M+arith]` Required decode to match Brotli q11/lw30 = 211,938,580 / 166.9e6 = **1.2700 s**.
Against the 16 MiB control 4.2232 s that is ratio **≤ 0.3007**. The arithmetic is right and
I confirm it. But note what it requires, and do the dilution arithmetic the report does at
§11.6 `[M+arith]`:

- Non-BWT bytes = 211,938,580 − 120,362,284 = **91,576,296 B**, decoding at Brotli's rate
  ≈ 0.549 s.
- Implied control BWT-path time ≈ 4.2232 − 0.549 = **3.674 s**; required BWT-path time =
  1.2700 − 0.549 = **0.721 s** ⇒ BWT-path required factor ≈ **5.09×**, not 3.33×.
  (**Self-correction of record:** the first draft of this report stated ≈4.5× here. That was an
  arithmetic error on my part — 3.674 / 0.721 = 5.09, not 4.47. The correction *strengthens*
  the kill argument: the requirement is larger than I first wrote. The constructive report's
  own §11.6 provisional figure of 5.06× (derived on the 128 MiB control, 4.2015 s) agrees
  within 0.7%.)
- **Even an instant BWT decode** leaves `0.549 / 4.2232 = 0.130`, so dilution is not binding
  — the report is right about that.

So the honest form of the gate is **≈5.09× on the BWT path** (exact derivation in §12), and the
report's own §11.5 G-B2 uses a *ratio* rather than a factor, which is the right call. But this
reframes the risk: a 5.09× on a serial pointer chase is not a routine engineering factor. It
requires the walk to be (a) the dominant cost of BWT decode — **unmeasured** — and (b)
latency-bound rather than TLB-bound — **unmeasured**.

The exact Amdahl condition, frozen with its derivation, is **§12**. Headline: **unless the
walk is ≥ 80.4% of BWT-path decode — and ≥ 91.8% if only 8 lanes are achievable — the
mechanism cannot reach Brotli parity at any k.**

**This is my decisive falsification and it is cheap to settle:** `f_walk` (the walk's share
of BWT decode) is a single unmeasured scalar that decides the mechanism before a single lane
is written. Nobody has measured it.

### 3.3 RSS: the "cannot be fixed" claim is stated too strongly

The report argues Ω(n) randomly-addressed state makes sublinear inverse BWT a math-class
dead end `[P, §7.2]`. That is directionally right for *rate-preserving, single-pass* exact
inversion. It is **not** a general lower bound: tiled/partitioned and multi-pass external
BWT trade time for space, and the project's own sweep already measures that trade curve
(V12/V13/V14). The honest statement is: **RSS is a Pareto trade, not a clever win** — which
is what the report says in effect. I mark the "math-class dead end" *framing* as overstated
and the *conclusion* as correct. No verdict impact.

### 3.4 The G-lane × BWT question is the real open question, and it is bytes-only

The report's §9 item 4 flags correctly that "BWT is transform-invariant-ish" is a hypothesis,
not a measurement (V30 shows the cross-product produced nothing, but V29 shows G-lane gains
were measured *only against Brotli*). The decisive test — run the frozen G2 typed basis
through `kRatioBackendBwt` on the sealed families at **bytes-only** cost — is cheap,
deterministic, R1-tier, and either confirms or kills the report's central reframing
(§2: frontend work is redundant). **I rank this above the k-sweep.** It costs no timing
budget and it is the only test that can falsify the premise the whole track rests on.

### 3.5 Hidden metadata and code costs the report under-charges

| hidden cost | assessment | label |
|---|---|---|
| Lane state `8k` B | correctly stated as negligible | `[M]`-trivial |
| New decoder build flag `--bwt-lanes=K` | report calls WSI-BWT "no format revision"; a new CLI surface is still a compatibility surface. The report does list `--bwt-lanes=K` in §8 arms — consistent, but §0.4's "zero new wire bytes, no format revision" should be read as *no wire change*, not *no new interface*. Minor wording risk, no verdict impact. | `[A]` |
| `.text` +1.0–2.5 KiB projection | must be gated, and the report does gate at +4 KiB (G-B5). But note the measured precedent: aux cost **+2,976 B `.text`** (V9) for strictly less logic. A 4 KiB gate on a lane scheduler is *plausible but unproven*. | `[P]` |
| Malformed-input surface | `I[]` forging. Existing bounds checks require `1 ≤ I[j] ≤ n`, `S == 1+(n-1)/r`, `4S ≤ remaining` — but **do not** require that segments *tile* `[0,n)` disjointly. Interleaving makes non-tiling a **silent double-write** rather than a rejection (report §4.2 invariant 2 concedes this). **This is a real new correctness obligation and it is not covered by the existing framing checks.** | `[D]` — new obligation, must be an explicit fuzz gate |
| Decompression-bomb | BWT decode costs ~6n RSS and ~85 ns/B regardless of compressed size, so a 1 MiB→4 KiB payload still costs ~6 MiB and ~90 ms. The report correctly attributes this to the *incumbent* backend, not to WSI. Agreed; carry to track 19. | `[M]`-structural |
| Instrumentation perturbation | report §12 adopts 2.3–8.1% (V16) and structurally separates binaries. **This is correct and better than most tracks' handling.** But note G-B4 is then correctly demoted to diagnostic. | `[M]` |

---

## 4. Prior-art map: which backend ideas are which

Per my mandate, I classify each *apparent* backend idea. This is the part of track 05 that
must not be soft.

| Apparent backend idea | Verdict | Nearest art / project evidence | Label |
|---|---|---|---|
| BWT + MTF/RLE/arith postcoders | **Not novel.** Adopted rate engine. | bzip2, divsufsort, xz; libsais vendored | `[M]` |
| **Context-clustered rANS** (cluster predecessor-byte contexts into K learned tables, one physical stream) | **NOT novel — project-measured non-novel.** | **Brotli RFC 7932 itself** maps decoded literal context to several prefix trees with a compact context map driven by previous decoded bytes; `RESEARCH_LEDGER.md` Exp. J / PART II §3 rules clustering non-novel. ANVIL's contribution is the fast quantizer (~1.65 ms), which is enabling infrastructure, not a mechanism. | `[M]` |
| **rANS-4096 / 256 / 512 / Huffman / defexc / pair-rANS as "backends"** | **Variants, already implemented.** | `RESEARCH_LEDGER.md:3870-3876` stream suite | `[M]` |
| **FSE** as an entropy backend | **Adopt-class only.** FSE = finite-state entropy coding; a different tANS/rANS implementation is not a backend novelty. *No FSE measurement exists in this repo* — that is an evidence gap, not a novelty opening. | literature; absent from repo | `[D]` |
| **CM / context-mixing / PPM / CTW postcoder over BWT** | **Not novel; adverse on the binding axis.** BWT→CM is an established family (libbsc/PAQ-BWT class). Closing it costs MTF/RLE structure *and* makes decode slower — the binding deficit. | V26 (≈40% rate gap) | `[P]` on the decode cost |
| **PAQ/CM/neural as the shipped backend** | **Kill as backend; retain as oracle.** Rate gap real and large (V26), but per-byte model updates ⇒ decode 1–20 MB/s. A backend that is 5–15× *slower* than Brotli on the binding axis while winning 40% rate is a **rate/decode exchange, not a Pareto win**. Falsifiability rule 11 forbids it. | V26 | `[M]` |
| **Grammar / SLP / RLZ / bidirectional-copy backend** | **Kill as backend.** Measured: ratio win, **decode −8.1…−15.9%**, encode 48–64× slower; root cause = eager materialization beats fused per-byte pulls on this host. | V27 | `[M]` |
| **QLFC / LZP postcoders** | **Kill — already measured NO-GO.** QLFC +3.23%, 0/13 wins; LZP +0.37%, 4/13. Static tables cannot track the MTF-rank distribution. | V23 | `[M]` |
| **Block-local backend routing** | **Kill — decisive negative.** Sum-of-blocks is worse than whole-file on *every* tested file; webster +894,315 B (+12.22%) at 4 MiB; mozilla/samba/ooffice/sao have **zero** BWT-winning blocks. The "mixed files contain BWT-friendly text regions" hypothesis is **falsified**. | V24 | `[M]` |
| **zstd ultra-22/long27 as ANVIL's ratio backend** | **Kill.** 52,364,240 B = 11.30% *worse* than ANVIL's own portfolio — it loses on the axis the project must win. | V2/V3 | `[M+arith]` |
| **xz/LZMA as a new arm** | **Kill — it is the bar, not a candidate.** | V2 | `[M]` |
| **DEFLATE reconstruction (bit-exact replay of embedded streams)** | **Adopt-class prior art, real value** (~1.36 MB on mozilla); targets files already routed to fast legacy backends, so it costs no decode regression. Track 08 owns. | V25 | `[M]` |
| **Static/transmitted dictionary (G5D paged base+overlay)** | **Adopt-class.** Dictionary *transport* is prior art (FSST, zstd CDict, typed dicts). Track 01 owns. | V25-adjacent | `[M]` |
| **BWT subblocking / tiling** | **Measured trade, not novelty.** The curve exists (V12/V14). Adopt the 16 MiB point as an explicit named Pareto profile; do not re-derive. | V12/V13 | `[M]` |
| **Aux-index inverse BWT** | **Adopt-class engineering, already measured and adopted.** +8,083 B for 1.364–2.339×. The index exists for *restart*, not concurrency. | V7/V8 | `[M]` |
| **Lane-interleaved inverse BWT (WSI-BWT)** | **Adopt-class engineering, zero novelty claim — and that classification is correct.** Any segmented/parallel inverse BWT obtains latency hiding identically; the only ANVIL-specific observation is that ~600–800 already-paid segments are consumed sequentially. | no project evidence either way; prior art is extensive | `[D]` |

**The honest novelty verdict for track 05: there is no novelty opening in the backend layer.**
Every entry above is either already implemented, already measured-and-killed, adopt-class
prior art, or engineering. The one structural fact worth writing down is **[D]**: inverse BWT
requires Ω(n) *randomly addressed* state, therefore *sublinear-memory rate-preserving exact
inversion does not exist*; RSS is a Pareto trade, not a cleverness target. That is a routing
policy, and routing policy is track 16, not a backend mechanism.

---

## 5. Decoder/resource risk audit

1. **The `6n` working set is structural and confirmed.** `T(n) + L(n) + ISA(4n)` reproduces
   measured 5.99–6.35× source across four files `[M, V22]`. Lanes add `8k` B and **zero**
   allocation — provided no second `L` copy. **Gate: RSS ≤ control × 1.02** (G-B5). Correct.
2. **Latency vs TLB is unresolved and is the mechanism's real risk.** The walk touches a
   4n-byte table; at 40 MB blocks that is a 160 MB random-access footprint. Whether 8
   concurrent streams help or *hurt* depends on whether the limiter is DRAM latency or TLB
   miss/page-walk bandwidth. **Unmeasured.** The report names this as its own top
   disconfirming risk (§9.1) — credit for honesty, and it is correct to name it.
3. **Bandwidth ceiling is a real floor.** At 6 B/output of random traffic, a single-core
   random-access ceiling of ~1.5–3 GB/s implies **250–500 MB/s** maximum `[P]`. The report
   needs 166.9 MB/s, i.e. it must capture **~33–67% of the entire random-access bandwidth
   budget** while also being ~3.3–5.1× faster than now (the 3.33× portfolio line is weaker than
   the ~5.1× BWT-path line; §12). Tight, not impossible.
4. **Amdahl on the *pipeline*, not just the walk.** Even a perfect walk leaves postcoder +
   ISA + CRC + framing. `1/f_walk` caps the achievable factor. **Unmeasured** — see §6.
5. **Memory-safety obligation is new.** Interleaving converts a bounds-check problem into a
   *partition* problem. Must assert segments tile `[0,n)` disjointly **before the first
   store**, and fuzz must include non-tiling `I[]`. Not covered by existing framing checks.
6. **Degenerate segments (adversarial case A1).** Single-cycle LF walks collapse segments; the
   report's own gate `k_eff ≥ 2 on ≥80% of panel blocks` is the right shape, but `k_eff` must
   be **measured per block**, not inferred from `distinct(I[j]) == S` (the latter is
   structurally forced by the encoder and proves nothing about walk diversity).

---

## 6. The coordinator's cross-track questions, answered

### 6.1 Track 06: does length-independent stream-cost selection invalidate Track 05 comparisons?

**Track 06's finding:** default stream selection uses length-independent ns/B constants
(`RESEARCH_LEDGER.md:3903`: "right ORDER, wrong GAPS"), and the flat 6.0 rANS fit **misses a
size-dependent symtab effect** (rANS-4096 ≈5–12% slower/B than 512/256 on small streams) —
a defect recorded and never patched `[M, V17/V18]`.

**My answer, split by question:**

- **Does it invalidate Track 05's *timing comparisons*?** **No.** Track 05's gates (G-B1,
  G-B2, G-B3, G-B6, G-B7) are **paired same-job ratios of uninstrumented binaries on
  identical bytes**. Selection mis-scaling is a *within-codec byte-vs-cycle trade-off* that
  affects what the encoder picks; it is **common to both arms of a paired A/B** and
  therefore cancels. It cannot invalidate a paired ratio. **This is why §12's
  binary-separation discipline matters more than the calibration does.**
- **Does it invalidate Track 05's *selection assumptions*?** **Yes, in one specific place.**
  The constructive report treats the 16 MiB tiling + aux + lanes configuration as "the"
  candidate point. But if stream-codec selection is mis-scaled, then the *comparison of
  candidate configurations* by projected decode cost is unreliable — and the report's
  §11.7 explicitly *adopts* track 11's P1d reconciliation as its shared cost unit. **Adopting
  an unreconciled product as a cost unit is circular**: P1d has not run, so §11.7 currently
  adopts nothing. The report handles this gracefully ("my cost model stays a projection and
  the k-sweep is run on measured wall-clock only; every GO/NO-GO threshold is a wall-clock
  ratio") — **that sentence is the correct resolution and it is load-bearing.** Keep it.
- **Where it DOES bite:** any claim of the form "this configuration is Pareto-optimal". The
  16 MiB point is a **measured** point (V12), so it is safe as a measurement; it is *not*
  safe as an optimum. Preregister: report the k-sweep as a **curve**, never select a
  default from it without a `C_decode` reconciliation.

### 6.2 Track 15 CAM: real measured warmup term, or invented cost center?

**Verdict: Track 15's CAM attacks a real term, but that term is on the *frontend* path, and
it does not relieve the binding backend deficit.** Reasoning:

- The only *measured* warmup term in this project is on the semantic path: **eager
  macro-stream materialization ≈ 26%** of mode-15 end-to-end decode `[M, V15]`. That is
  setup/masks+residuals, i.e. exactly a cross-block model-carryover cost. So CAM's target is
  **not invented** — it is the second-largest measured decode component after CRC.
- **But** (a) that 26% lives in the mode-15 semantic path, which serves the 43.21% of
  Silesia input bytes routed to **Brotli** (mozilla, ooffice, samba, sao, xml), while the
  binding 3.4474×/5.6642× decode deficits (V10) are on the **BWT path** — different code,
  different profile, no measurement overlap `[M/D]`; (b) fixing setup on the semantic path
  cannot move the byte lead, which is 100% produced by BWT on 56.79% of bytes (V5/V6);
  (c) do-not-reburn **G1** is a decisive measured negative against exactly the
  "improve it by carrying state across blocks" family at the *backend-routing* level:
  sum-of-blocks was worse than whole-file on every file, +12.22% on webster, with zero
  BWT-winning blocks on the four mozilla/samba/ooffice/sao files `[M, V24]`. Cross-block
  carryover *within* the semantic path is not the same experiment, but G1 removes the
  project's prior that cross-block anything is free.
- **Therefore:** CAM is **legitimate but non-binding for track 05's verdict**, and must not
  be used as evidence that the backend decode axis is addressable.

### 6.3 Does the mode-15 cost-decomposition protocol (track 11 §9 P1) cover Track 05?

**Partially — and the constructive report's §11.3 slot-mapping has a specific defect I must
flag.** Track 11's P1 schema (per-token category counts, `pulls_per_output_byte`,
`varint_reads_per_output_byte`, `copy_bytes`, `macro_entries`) is a **per-token** schema. The
BWT path has **no tokens**; it has a postcoder stream and a pointer walk. The report's
mapping (`macro_entries` → "lane retirements + lane pulls", `copy_bytes` → "LF-walk byte
assignments") is *nominally* consistent but **structurally unable to answer the question
that matters**: what fraction of decode is the walk (`f_walk`). The report partially
recognizes this — its B8 postcoder-only probe is exactly the right instrument — but then
**reclassifies B8 as `DIAGNOSTIC_NOT_A_CODEC_ARM` and non-gating in §12.3**.

**That is the single most important disagreement in this report.** `f_walk` should be a
**gate input**, because:
- §12 shows the mechanism is arithmetically dead if `f_walk < 0.918` at k=8 (or < 0.804 at any k);
- §12's threshold (≈5.09× BWT-path) is the real gate, not the 3.33× headline;
- a diagnostic that can void a mechanism without blocking it is not a diagnostic, it is an
open-ended research bill.

---

## 7. Alternative architecture (one, and only if defensible)

**Alternative: a cost-reconciliation-first decision gate, not a second mechanism.**

I considered and rejected proposing a competing backend mechanism, because §4 shows the
entire backend mechanism space is occupied or measured-dead. What is genuinely missing is not
a mechanism but a **measurement that arbitrates every backend decision**, and the project
has now burned three separate cycles (I9 wire-invisible, I10-G4 speed accounting, S6-1
uncalibrated J) on decisions taken against mis-scaled or unmeasured cost constants.

So the defensible alternative is:

> **Measure `f_walk` and the per-stage BWT decode split once, remotely, paired, and make it
> the entry gate for *all* backend promotions (05, 06, 07, 15, 18) — the same role track 11's
> P1 plays for the semantic path.**

**Why this is better than the alternative mechanisms considered:**
- **vs. "threaded inverse BWT"**: multi-thread is a resource-axis change, explicitly excluded
  by the I10-1A citation axis (`I10-AUX-UNBWT-INTEGRATION-PLAN.md` §5) — it would answer a
  different question.
- **vs. "adaptive lane count / dynamic k"**: adds an encoder-visible selector, which is
  *routing* (track 16), and needs `C_decode` to be meaningful (it isn't, per §6.1).
- **vs. "BWT walk-order (fused) postcoder"**: genuinely retired for decode — an arithmetic/
  entropy decoder is serial, so fusing forbids interleaving; and the rate risk is structural
  (MTF/RLE structure is defined in string order and is destroyed in walk order). Correctly
  retired; **retain only as a memory-only variant**.
- **vs. "sub-Ω(n)-space inverse BWT"**: math-class dead end as a rate-preserving replacement
  (§4, §3.3). Anyone proposing it in a later swarm should be routed to §4 first.

**Is it defensible?** Yes: it is adopt-class, cheap, remote-only, and it is the only action
that can *falsify* the constructive lane's premise before the lane spends a mechanism build.

---

## 8. The decisive REMOTE-ONLY falsification experiment (one job, pre-registered)

Per the coordinator's shared-measurement instruction: **no new bespoke benchmark.** Reuse the
frozen protocol chain (`docs/github-actions-benchmark-protocol.md`, dense-frontier prereg
`docs/I10-DENSE-FRONTIER-PREREG.md` §3–§8, driver `tools/paired_bench.py`), and reuse track
11 §9 P1's counter binary for the semantic arm. Everything below is **byte-only or
uninstrumented-timing**; **no instrumented timing is used** (per §12's own 2.3–8.1%
perturbation band, V16).

### Job R1 — "backend cost reconciliation + premise falsifier"

| arm | content | binary | tier | what it decides |
|---|---|---|---|---|
| **R1-A** | Frozen `anvil --parse=ratio --bwt-aux=on --bwt-subblock=16MiB`, plus `--bwt-lanes ∈ {1,2,4,8,16,32}` | **uninstrumented `anvil`** | R2/R3 timing | The k-curve. Replaces the speculative 3.33× target with a measured curve. |
| **R1-B** | **G-lane typed basis (G2/G3 frozen) × `kRatioBackendBwt`**, sealed families | uninstrumented | **R1 byte-only** | Falsifies/refirms report §2's premise that frontend work is redundant against BWT. **Highest information per second; run first.** |
| **R1-C** | **Stage split of BWT decode** — an *uninstrumented* variant that decodes the postcoder stream and discards the walk, vs the full path | uninstrumented | R2 timing, **`DIAGNOSTIC_NOT_A_CODEC_ARM`** | Measures `f_walk` = 1 − `t_postcoder_only / t_full`. **This is the gate input §6.3 requires.** |
| **R1-D** | track 11 §9 P1 semantic-arm counters, same job | `anvil-telemetry` | R1 byte-only | Shared instrument; no duplication. |
| **R1-E** | A/A-1 protocol null + A/A-2 perturbation null | both | R2 | Validity. |
| controls | `C0` byte-identity vs frozen 46,446,995 B / 23,534,368 B; per-rep output SHA-256; `repeat.jsonl` no-regression; `random.bin` unchanged; dual-bar on synth cells | | | Any FAIL ⇒ VOID. |

### Pre-registered thresholds (frozen now, before any data — master-brief item 5)

| gate | GO | NO-GO / VOID |
|---|---|---|
| **G-1 (`f_walk`, the premise gate — three-way, see §12.3/§12.6)** | `f_walk ≥ 0.918` point **and** CI lower ≥ 0.918 on ≥ 3 of 5 BWT-routed panel files (dickens, mr, nci, webster, x-ray) | CI upper < **0.804** on ≥ 3 of 5 ⇒ **HARD NO-GO** (assumption-free kill, §12.4); any CI straddling 0.804–0.918 ⇒ **TIMING_BLOCKED** — no GO and no NO-GO |
| **G-2 (front-end redundancy premise)** | R1-B shows ≤ 0.10% byte gain for the G typed basis × BWT on ≥ 3 of 4 families | ≥ 0.50% gain on ≥ 2 families ⇒ report §2 is **wrong**, and frontend×backend work becomes the higher-EV direction — re-open and re-rank tracks 09/10/14 |
| **G-3 (BWT-path factor)** | paired BWT-path decode ratio point ≤ **0.196** (≈5.09×, §3.2/§12) with bootstrap CI upper ≤ 0.22 | point > 0.30 ⇒ **NO-GO**; 0.196 < point ≤ 0.30 ⇒ **HOLD** (sub-8% findings are below the 2.3–8.1% perturbation band and may not justify a default change) |
| **G-4 (charge)** | complete bytes ≤ control + 0.02%; peak RSS ≤ control × 1.02; `.text` ≤ +4 KiB; segments asserted to tile `[0,n)` disjointly before first store; full fuzz incl. non-tiling `I[]` | bytes > +0.05%; RSS > ×1.05; `.text` > +4 KiB; **any** non-tiling acceptance ⇒ **HARD NO-GO** |
| **G-5 (validity)** | A/A-1 CI spans 1.0; A/A-2 within [0.92, 1.081]; ambient robust CV ≤ 10%; arm CV ≤ 15%; all reps SHA-256 identical; C0 byte-identity PASS | any FAIL ⇒ `TIMING_BLOCKED`/VOID; **no claim** |
| **G-6 (novelty honesty)** | any positive result recorded as **adopt-class engineering**, never as mechanism novelty, never as `P-CROSSING` without dual-bar + label checklist | a novelty claim without track-20 clearance ⇒ VOID |

**Explicitly NOT authorized by this report:** any local corpus benchmark, local sweep, local
fuzz campaign, any instrumented-timing gate, any new driver, any threshold movement.

---

## 9. Reconciliation with the constructive lane

I read `docs/swarm-2026-10-02/05-backend-frontier-space-bunny.md` (770 lines, incl. §11–§12
addenda) before finalizing. Explicit reconciliation:

| constructive claim | my finding | resolution |
|---|---|---|
| §0/§2: the backend layer **is** the mechanism layer; frontend work is largely redundant | **CONFIRMED independently** (V5/V6/V29/V30) | **Agreement.** This is the track's strongest surviving result. |
| §0/§8: WSI-BWT is adopt-class with **zero novelty claim** | **Confirmed** and required by the project's novelty doctrine | **Agreement.** I go further: I find *no* novelty opening anywhere in the backend layer (§4). |
| §6: 3.33× is "the decisive line" | **Arithmetically correct, strategically wrong.** The real BWT-path requirement is **≈5.09×** (`[M+arith]`, §12.2), agreeing within 0.7% with the report's own provisional 5.06× at §11.6 | **Resolved disagreement.** Both lines must be reported: 3.33× is the *portfolio* line, 5.09× is the *mechanism* line. A mechanism can clear the portfolio line and still fail the mechanism line. |
| §4.4: walk is "one dependent DRAM round trip per byte" | **Unmeasured inference.** No stage split exists for the BWT path | **Disagreement of kind, not of value.** The claim may be true; it is not evidence. This is the reason G-1 exists. |
| §11.3: BWT path fits track 11's per-token counter schema | **Structurally cannot answer `f_walk`** — the BWT path has no tokens | **Disagreement.** R1-C is the required instrument; §12.3's demotion of B8 to non-gating must be reversed. |
| §11.7: adopts track 11's P1d as shared cost unit | Circular if P1d has not run — but the report immediately notes every gate is a wall-clock ratio, which resolves it correctly | **Agreement with the resolution.** Keep the wall-clock-ratio formulation; do not let a modelled cycle count into any gate. |
| §12: instrumentation perturbs 2.3–8.1%; separate binaries; A/A-2 null | **Correct and better than most tracks' handling.** Applies project-wide. | **Agreement, escalated.** I restate it as binding on all tracks 05/06/07/11/15/18 sub-8% gates. |
| §12.6 flag 1: track 11's G6 (1.08×) lies inside the perturbation band | **Correct.** | **Agreement.** |
| §5/§7.1: walk-order postcoder retired for decode (fused entropy decode forbids interleaving; MTF/RLE structure destroyed in walk order) | **Correct**, and I independently endorse | **Agreement.** Retain as memory-only variant only. |
| §7.2: sub-Ω(n)-space inverse BWT is a math-class dead end | Conclusion **correct**; "math-class" framing **overstated** (tiled/multi-pass external BWT trade time for space, and the project already measures that curve) | **Minor correction.** No verdict impact. |
| §9 item 4: G-lane × BWT is the cheap decisive test | **Agreed and I promote it above the k-sweep** (R1-B, G-2) | **Escalation.** Bytes-only, no timing budget, falsifies the track's own premise. |
| §8: minimum prototype "none in this repo" | **Agreed** — local measurement is forbidden and worthless as evidence | **Agreement.** |

**No disagreement currently changes the constructive lane's mechanism description.** My
objections are to (a) the *evidence status* of the cost model, (b) the *magnitude* of the
target, and (c) the *gate status* of the stage split. Those are exactly the three things one
cheap remote job settles.

---

## 10. Disposition table

| item | disposition |
|---|---|
| WSI-BWT (lane-interleaved inverse BWT) | **PILOT** — conditional on G-1 (`f_walk ≥ 0.918` at k=8; §12.3). Adopt-class, no novelty claim. |
| G-lane typed basis × BWT backend (R1-B) | **PROMOTE first** — byte-only, cheap, tests the track's own premise |
| BWT stage-split measurement (R1-C) | **PROMOTE as gate input** — reversed from constructive §12.3 |
| 16 MiB tiling as a named Pareto profile | **ADOPT as a measured point** (V12/V13), **not** as an optimum |
| Denser segment index (`r/2`, `r/4`) | **HOLD** — only after the k-curve shows latency still binding at k=32 |
| Walk-order (fused/reverse) postcoder | **KILL for decode; retain memory-only** |
| Sub-Ω(n)-space / rate-preserving inverse BWT | **KILL (math-class)** — route future proposals here first |
| zstd ultra-22/long27 as ANVIL ratio backend | **KILL** (11.30% worse bytes) |
| CM/PPM/neural backend | **KILL as backend; RETAIN as oracle** |
| Grammar/RLZ/BMS backend | **KILL** (measured decode −8.1…−15.9%, encode 48–64×) |
| QLFC / LZP postcoder | **KILL** (already measured NO-GO) |
| Block-local backend routing | **KILL** (decisive negative; do not re-derive) |
| Context-clustered rANS as novelty | **KILL as novelty** (Brotli RFC 7932; ruled non-novel in-project) |
| FSE as entropy backend | **HOLD — adopt-class, and an evidence gap (never measured here)** |
| DEFLATE replay / G5D paged dictionary | **ADOPT via tracks 08 / 01** — not re-opened here |
| Track 15 CAM | **Legitimate but non-binding for track 05** — its 26% target is real and measured, on the *other* path (§6.2) |
| Cross-block anything, as a *routing* lever | **KILL** (G1 decisive negative) |

---

## 11. FINAL RECOMMENDATION: **PILOT**

**Not PROMOTE-TO-REMOTE** because the mechanism's premise (`f_walk` dominant) is **unmeasured**
and the arithmetic says the mechanism is dead if `f_walk < 0.918` at k=8 (§12.3); funding a mechanism build
ahead of that is the project's characteristic failure mode (I9 wire-invisible, G4 speed
accounting, S6-1 uncalibrated J — three prior instances).

**Not HOLD** because (a) the k-curve is a zero-byte-cost, already-instrumented measurement;
(b) R1-B is byte-only and can falsify the track's central premise for ~0 CI budget;
(c) R1-C is a single uninstrumented variant answering a question nobody has ever answered.

**Not KILL** because the mechanism is real, free in bytes, and adopt-class — but note that
**KILL becomes the correct verdict the moment G-1 reports `CI_upper(f_walk) < 0.804` on ≥3 of 5 panel
files**, and I pre-commit to that here so the threshold cannot move after data.

### Cheapest decisive remote next step

> **One GitHub Actions job, `tools/paired_bench.py`, no new driver, no new harness.**
> **Frozen order (§12.7): R1-B first, unconditionally, because it is byte-only and because it
> is upstream of the kill threshold.** `f_walk` is only worth measuring if BWT remains the
> byte engine; R1-B can change `N_bwt`, `N_non`, `t_nonBWT`, `R_req` and therefore `f`'s own
> threshold. Running R1-C first risks freezing a threshold against a premise R1-B refutes.
>
> ```
> R1-B  byte-only  G-lane typed basis (G2/G3 frozen) × kRatioBackendBwt on sealed families
>        └─ ≥0.50% byte gain on ≥2 families ⇒ premise FALSIFIED, re-rank tracks 09/10/14, STOP
> close  t_nonBWT  per-file uninstrumented decode seconds (single 128 MiB block ⇒ per-file ==
>        backend; §12.5). Turns R_req from [M+arith]-on-an-assumption into [M].
> R1-C  timing     uninstrumented postcoder-only probe ⇒ f_walk  (G-1 three-way verdict)
>        └─ CI_upper < 0.804 ⇒ HARD NO-GO, mechanism dead, do not build lanes
>        └─ CI straddles band ⇒ TIMING_BLOCKED, more reps + closed t_nonBWT, no NO-GO
>        └─ ≥0.918 confirmed ⇒ proceed
> R1-A  timing     k-curve {1,2,4,8,16,32} only if G-1 did not return HARD NO-GO/TIMING_BLOCKED
> ```
> Same job also carries `C0` byte-identity vs frozen 46,446,995 B / 23,534,368 B, A/A-1 and
> A/A-2 nulls, and track 11's `anvil-telemetry` counters for the semantic arm.
>
> Cost: one CI job. Local work: **zero**. Threshold movement: **prohibited.**
> **Only quantity that genuinely still needs remote measurement: `f_walk`** — nothing in the
> repo identifies it, and the existing mode-15 profile (`RESEARCH_LEDGER.md:3911-3931`) covers
> the semantic path only. Everything else in this report is `[M]`, `[M+arith]`, or `[D]`.

**Quantified accounting summary (the numbers a reviewer will be asked for):**

| axis | WSI-BWT mechanism charge | 16 MiB tiling charge | total vs frozen default |
|---|---|---|---|
| bytes | **0** (decoder constant `k`) | **+472,498 B (+1.0169%)** on Silesia | +1.0169%; still −4.94% vs Brotli |
| decode cycles | removes ≈8 ns/iteration-equivalent *only if* G-1 passes; unproven | +0.52% measured | **target ≈5.09× on the BWT path — unproven; requires f_walk ≥ 0.918 at k=8 (§12)** |
| decoder state | +8k B (≤256 B at k=32) | none | ≤256 B, O(1) |
| peak RSS | 0 (no second `L` copy); gate ×1.02 | −49.4% measured (248.4 → 125.7 MiB) | 1.01× Brotli, **2.31× xz** — likely the residual `FRONT-GAP_RSS` axis |
| `.text` | +1.0–2.5 KiB `[P]`, gated ≤ +4 KiB | none | within the 1 MiB decoder-size constraint (current CLI 453,512 B) |
| expansion / amplification | no new class; existing `blen ≤ block_size` dominates | none | unchanged |
| correctness obligation | **NEW**: assert segments tile `[0,n)` disjointly before first store; fuzz non-tiling `I[]` | none | unfunded today — must be gated (G-4) |

---

## 12. FROZEN PRECONDITION: `f_walk` as a gate, with the exact Amdahl derivation

Coordinator closeout requirement: freeze `f_walk` as a **true precondition**, state the exact
Amdahl kill threshold implied by the measured required BWT-path factor, and require
instrumentation-perturbation controls. Done here. **All arithmetic below is `[M+arith]` on
V12/V15-referenced inputs and is frozen before any data is collected.**

### 12.1 Inputs (frozen, with provenance)

| symbol | value | provenance |
|---|---|---|
| `N` (Silesia source) | 211,938,580 B | `docs/I10-DENSE-FRONTIER-PREREG.md` §4 `[M]` |
| `N_bwt` (BWT-routed input) | 120,362,284 B | do-not-reburn §F0.1 `[M]` |
| `N_non` | 91,576,296 B = `N − N_bwt` | `[M+arith]` |
| `R_brotli` (Brotli q11/lw30 Silesia decode rate) | 166.9 MB/s | AUX-RESULTS §11 `[M]` |
| `T_ctl` (control, aux 16 MiB) | 4.2232 s | subblock sweep `[M]` |
| `T_ctl'` (control, aux 128 MiB) | 4.2015 s | subblock sweep `[M]` |
| `R_req` (required BWT-path factor) | **5.09×** | §12.2 |

### 12.2 Derivation of the required BWT-path factor

```
T_target  = N / R_brotli              = 211,938,580 / 166.9e6 = 1.27003 s
t_nonBWT  = N_non / R_brotli          =  91,576,296 / 166.9e6 = 0.54869 s     [A: see 12.5]
T_bwt_ctl = T_ctl − t_nonBWT          = 4.2232 − 0.54869     = 3.67451 s
T_bwt_req = T_target − t_nonBWT       = 1.27003 − 0.54869   = 0.72134 s
R_req     = T_bwt_ctl / T_bwt_req     = 3.67451 / 0.72134   = 5.094
```
Cross-check on the 128 MiB control: `(4.2015 − 0.54869)/(1.27003 − 0.54869) = 5.064`
⇒ **R_req = 5.08 ± 0.03**, consistent with the constructive report's independent provisional
5.06× (§11.6). Agreement within 0.7%; no dispute.

### 12.3 The exact Amdahl condition

Let `f = f_walk` be the walk's share of BWT-path decode, and let lanes reduce the walk's
time by a factor `k_eff`. Then

```
T_bwt_after / T_bwt_ctl  =  (1 − f) + f/k_eff          … (A)
promotion requires        T_bwt_after / T_bwt_ctl  ≤  1/R_req  = 0.1963
⇒  f · (1 − 1/k_eff)  ≥  1 − 1/R_req  = 0.8037
⇒  f  ≥  0.8037 / (1 − 1/k_eff)                        … (B)  THE FROZEN THRESHOLD
```

`f ≥ 0.8037 / (1 − 1/k_eff)` is the exact precondition. Evaluated `[M+arith]`:

| achievable `k_eff` | required `f_walk` | verdict if `f_walk` is lower |
|---:|---:|---|
| 8 (the lane count the report models as "realistic") | **0.9183** | dead |
| 16 | **0.8571** | dead |
| 32 (max swept) | **0.8284** | dead |
| ∞ (free walk) | **0.8037** | dead below this even at infinite lanes |

Inverted — **the minimum lane count required for a given measured `f_walk`** (`k ≥ f/(f−0.8037)`):

| measured `f_walk` | lanes needed | reachable in the `{1,2,4,8,16,32}` sweep? |
|---:|---:|---|
| 0.98 | 5.6 → 6 | yes |
| 0.95 | 6.5 → 7 | yes |
| 0.90 | 9.3 → 10 | yes |
| 0.85 | 18.3 → 19 | yes |
| 0.83 | 31.4 → 32 | marginal (at the sweep edge) |
| ≤ 0.8284 | ≥ 32 | **no — dead at every k in the sweep** |

### 12.4 Why `f_walk ≥ 0.804` is an *assumption-free* kill threshold

The `t_nonBWT` estimate is the one assumption in §12.2. But `t_nonBWT ≥ 0` always holds, so
`R_req ≤ T_ctl / T_target = 4.2232 / 1.27003 = 3.3253`, and therefore
`f ≥ (1 − 1/3.3253)/(1 − 1/k_eff) = 0.6993/0.875 = **0.7992**` at k=8.

> **`f_walk` below 0.80 is a sufficient NO-GO under every admissible value of `t_nonBWT`,
> including `t_nonBWT = 0`.** This half of the threshold is arithmetic, not measurement.

Sensitivity of the *point* threshold to the assumption `[M+arith]`:

| assumed `t_nonBWT` | `R_req` | `f` needed at k=8 |
|---:|---:|---:|
| 0.549 s (proportional, used) | 5.094 | **0.9183** |
| 0.300 s | 3.788 | 0.8412 |
| 0.000 s (lower bound) | 3.325 | 0.7992 |

**The point threshold ranges 0.799–0.918 across the full admissible range.** That band is why
G-1 is specified as a **three-way** verdict, not a pass/fail.

### 12.5 The `[A]` that must be closed, and how

`[A]` `t_nonBWT` is obtained by applying Brotli's **corpus aggregate** decode rate to the
**non-BWT subset**. This is biased: the Silesia aggregate is a harmonic mean over the whole
corpus, while the non-BWT subset (mozilla, ooffice, samba, sao, xml) excludes the hardest
Brotli files (webster, nci, x-ray, mr) and so likely decodes **faster** per byte ⇒ true
`t_nonBWT` is probably **smaller** than 0.549 s ⇒ `R_req` **larger** than 5.09 ⇒ the point
threshold **stricter** than 0.918.

**Closure (same job, no new harness):** with `--parse=ratio` the block size is 128 MiB
(`src/anvil.cpp:4999-5000`), so every Silesia/enwik8 file is a **single block** and
per-file uninstrumented wall-clock decode time **is** per-backend decode time (constructive
report §12.3, which I endorse). Emit per-file decode seconds in the same job; routing per
file is byte-deterministic. **`t_nonBWT` then becomes `[M]`, and `R_req` becomes `[M+arith]`
on measured inputs.** No assumption remains in the promotion path.

### 12.6 Instrumentation-perturbation controls (binding)

`f_walk` is a **difference of two measured times**, so it is the most perturbation-exposed
quantity in the plan. Controls, all mandatory:

1. **Two binaries, never mixed.** `anvil` (counters compiled out) is the **only** source of
   `T_full` and `T_postcoder_only`. `anvil-telemetry` contributes **byte-only counters** and
   **zero timing numbers**. No gate, CI, table, or ratio-bearing sentence may reference a
   telemetry timing.
2. **A/A-1 (protocol null):** `anvil` vs `anvil`, same flags, same pass. CI must span 1.0.
3. **A/A-2 (perturbation null):** `anvil-telemetry` vs `anvil`, same arm, same Pass-2
   protocol, reported as a paired ratio. Recorded, never gated as win/loss. Validity band
   **[0.92, 1.081]** (from the measured 2.3–8.1% band, `RESEARCH_LEDGER.md:3918-3923`); outside
   it the telemetry **counter set** is VOID and re-run. Timing arms are unaffected either way.
4. **Three-way G-1 verdict, mandatory** — because `f` is a difference of two timings, its CI
   is wider than either input's:
   - `CI_upper < 0.804` on ≥3 of 5 panel files ⇒ **HARD NO-GO** (assumption-free, §12.4).
   - `CI_upper ≥ 0.918` and `CI_lower ≥ 0.918` on ≥3 of 5 ⇒ **PREMISE CONFIRMED**, proceed to
     the k-curve.
   - **any** CI straddling the band ⇒ **TIMING_BLOCKED**: no NO-GO, no GO. Remedy is more
     repetitions and the closed `t_nonBWT` (§12.5) — **never a smaller epsilon, never a moved
     threshold** (master-brief item 5).
5. **Discriminator if G-1 lands in 0.60–0.80** (dead but informative): the stage probe as
   specified lumps ISA construction in with "non-walk". One additional **uninstrumented**
   probe that builds ISA and discards the walk separates them. If ISA build (not the walk)
   dominates, the correct disposition is not "WSI-BWT KILL" but "**different mechanism, and
   ISA-build parallelism is a new track-05 candidate that this report does not assess.**"
   Recording this now prevents a later agent from re-deriving it.
6. **Project-wide escalation:** any gate with effect size < 8% across tracks 05/06/07/11/15/18
   inherits rule 12.6 — most notably **track 11's G6 (`decode ≥ 1.08×`)**.

### 12.7 Sequencing: R1-B must run before any timing job

Coordinator question answered **affirmatively**, and the reason is stronger than "it is
cheap".

1. **It can falsify the premise the whole track rests on.** §4 of the constructive report and
   my §1/V29/V30 both assert frontend work is largely redundant because mode 17's
   transform×backend cross-product produced no win (V30). That is a **negative** result for one
   specific cross-product; R1-B tests the *G-lane typed basis*, which has never been run
   against `kRatioBackendBwt` (V29: G0–G3 were all pinned to Brotli q11/lgwin30). If it wins,
   the track's central reframing is **wrong** and frontend×backend work outranks the decode
   axis.
2. **It is upstream of the kill threshold, not merely cheap.** `f_walk` is only worth
   measuring if BWT stays the byte engine. If a frontend transform cuts bytes *against BWT*,
   the BWT-routed share `N_bwt` changes, which changes `N_non`, `t_nonBWT`, `R_req`, and
   therefore `f`'s threshold in §12.3. **Running R1-C first risks freezing a threshold
   against a premise that R1-B would have refuted.**
3. **It costs no timing budget whatsoever.** R1-B is byte-only / R1 tier — no paired
   repetitions, no ambient gate, no A/A null, no contamination of the timing arms. Running it
   first is strictly dominant.
4. **Honest counter-risk `[A]`:** R1-B needs the frozen G2/G3 artifacts *and* the sealed
   families published. `docs/I10-FRONTIER-RECON-2026-09-24.md` §3 records that the dense-frontier
   and G5D files are **local, uncommitted, and blocked pending explicit authorization**. So R1-B
   may be dispatch-blocked while R1-C/R1-A are not. This is an *authorization* dependency, not
   a scientific one, and it must not be used to justify reordering — the sequencing argument
   above holds regardless of dispatch order.

**Frozen execution order: `R1-B` (bytes-only) → close `t_nonBWT` → `R1-C` (`f_walk`) → G-1
three-way verdict → `R1-A` (k-curve) only if G-1 does not return HARD NO-GO or
TIMING_BLOCKED.**