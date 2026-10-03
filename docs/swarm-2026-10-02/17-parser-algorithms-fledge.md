# 17 — Parser / candidate-generation algorithms: independent adversarial audit

**Track:** 17-parser-algorithms · **Role:** Fledge Alpha Free (adversarial reviewer)
**Date:** 2026-10-02 · **Worktree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty
**Status:** **INTERIM.** The paired constructive report
`docs/swarm-2026-10-02/17-parser-algorithms-space-bunny.md` **did not exist** when this
was written (`docs/swarm-2026-10-02/` contained only `MASTER-BRIEF.md` at audit time).
Nothing in it was read, assumed, or inherited. §10 lists the specific disagreements I
will adjudicate when it lands and exactly which of them can move this verdict.

**Mutation discipline:** exactly one new file written. No existing file modified. No
reset/clean/stash/restore/rebase, no commit, no push. No local corpus benchmark, no
sweep, no fuzzing, no prototype build — deliberately, see §7.1 (do-not-reburn §E5
requires the leverage check *before* the build, and the leverage check already fails).

**Evidence classes used throughout:** `[M]` measured in-repo with a named artifact ·
`[D]` derived arithmetically from `[M]` · `[C]` code-read fact with file:line ·
`[P]` projection/hypothesis, explicitly not measured · `[H]` hypothesis.

---

## 1. Bottom line

The track as chartered — improve candidate-generation / parser asymptotics for a
measured-cost MDL parse — **cannot move the canonical ANVIL frontier, and this is
settled by code inspection rather than by experiment.** The published remote frontier
arm does not contain a parser.

The published remote evidence uses `--parse=ratio`
(`docs/I10-DENSE-FRONTIER-PREREG.md:110-111`):

```
anvil-legacy: --parse=ratio --ratio-backend=auto --ratio-context=off --ratio-lines=off --quiet
```

and `encode_ratio_block` (`src/anvil.cpp:4614-4639`) is a transform × backend
portfolio with **no `MatchFinder`, no hash chain, no `parse_greedy`/`parse_dp`/
`parse_mdl`/`parse_sparse`, no TCOPY, no PNRA, no HOTOP, no candidate generation of
any kind**: `[C]`

```
consider(0,d);                                   // identity
if(opt.ratio_context) ratio_transform_ctx1  → consider(1,x);
if(opt.ratio_lines)   ratio_transform_lines → consider(2,x);
... for each: for each backend in {brotli, bwt}: keep smallest payload
```

With the frontier's own `--ratio-context=off --ratio-lines=off`, only `consider(0,d)`
survives, so the canonical arm is exactly *min(Brotli, BWT) over the identity
transform*. On that route the parser is **Brotli's** (or BWT's).

**And the size of the canonical win confirms it is a routing win, not a parser win.**
`[M]` Silesia legacy total from the frozen table
(`docs/I10-DENSE-FRONTIER-PREREG.md:157-169`) sums to **46,446,995 B**. Do-not-reburn
§F4 records `oracle_min(Brotli,BWT)` = **46,446,836 B** and §G2 records the E6 frozen
result as "46,446,995 B, **+159 B** over oracle"
(`docs/audit-2026-09-07/06-do-not-reburn.md:179-180,219`). So `[D]` **99.9997% of
ANVIL's canonical Silesia byte result is the two-codec routing choice; the entire
residual attributable to anything ANVIL itself does is 159 bytes — 0.00034%.**

Track 17's charter cannot touch that 159 bytes, because there is no candidate
generator on that code path.

---

## 2. Strongest falsification case (the kill)

### 2.1 MDL is strictly dominated by brotli q11 on all three axes simultaneously

`[M]` `tests/benchmark-summary.csv`, 13 files, total input **12,272,327 B**, Ryzen 9
5900X / clang 22.1.8 / median of 3 reps (`tests/host-spec.md:13,23,56`):

| codec | total out (B) | ratio | enc MB/s | dec MB/s |
|---|---:|---:|---:|---:|
| `anvil-greedy-rans` | 2,743,742 | 0.223571 | 14.151 | 146.911 |
| `anvil-sparse-rans` | 2,609,164 | 0.212605 | 8.525 | 152.369 |
| `anvil-hotop-rans` | — | 0.215307 | 7.963 | 171.750 |
| `anvil-shape-rans` | — | 0.204434 | 0.687 | 147.360 |
| `anvil-dp-rans` | 2,539,808 | 0.206954 | 1.124 | 160.896 |
| **`anvil-mdl-rans`** | **2,439,834** | **0.198808** | **0.718** | **136.550** |
| `brotli-q4` | 2,492,866 | 0.203129 | 101.037 | 497.077 |
| `brotli-q6` | — | 0.176318 | 55.128 | 515.585 |
| `brotli-q9` | — | 0.173063 | 19.269 | 506.355 |
| **`brotli-q11`** | **1,788,233** | **0.145713** | **0.573** | **450.937** |

I recomputed the ratio denominator independently: `Σout/Σin` reproduces the CSV's
`total_ratio` exactly for all rows above (e.g. MDL 2,439,834/12,272,327 =
0.198808 `[D]`), so the aggregate is a true byte-weighted ratio, not a mean of ratios.

`[D]` MDL vs q11: q11 is **0.798× the encode time**, **0.733× the bytes (26.7%
smaller)**, and **3.302× the decode throughput**. MDL loses on bytes, encode, *and*
decode against one existing reference point. This is not a near-miss on a
ratio/runtime trade; it is interior domination.

### 2.2 The encode-speed gap is not repayable by any parser change

`[D]` MDL's aggregate ratio advantage over q4 is **2.13%** (0.198808 vs 0.203129) and
costs **140.7×** encode throughput. Closing that would require a **140.7× encode
speedup at constant bytes.** The cheapest conceivable structural relief inside the
current design bounds the win far below that:

- relaxation count: windowed MDL does ≤4 candidates × ≤16 length cuts = **≤64 DP
  relaxations per input position per pass** (`src/anvil.cpp:3641-3654`), over up to 3
  passes plus a greedy seed. A single-relaxation-per-position selection cuts this by
  ≤**64×**;
- measurement encodes: 4 × 5 = **20 throwaway stream encodes per block**
  (`src/anvil.cpp:3605,3677,3696`) — reducible to ~1×, worth ~**4×**;
- combined optimistic bound ≈ **96×** on the relaxation term and ≈4× on the
  measurement term, leaving a residual **~1.5-20× short** of the 140.7× needed.

### 2.3 The decode axis is unreachable by this mechanism class — the decisive kill

`[D]` MDL decode is **136.550 MB/s vs q4's 497.077 MB/s = 3.64× slower to decode**,
for 2.13% fewer bytes. Parser/candidate-generation work is **encoder-only by
construction** — the MDL, sparse, shape, topology, TCOPY and HOTOP families all feed
`encode_tokens*` while the decoder paths (`decode_tokens_rans`,
`decode_tokens_sparse`, `decode_tokens_shape`, …) are untouched by which parse was
chosen. Therefore:

> **The mechanism class cannot address the axis on which its deficit is largest.**
> A 3.64× decode deficit paired with a 2.13% byte gain and a 140.7× encode deficit is
> not a Pareto point that encoder-side search can reach.

This is the cleanest statement of "ratio gains that can never repay encode cost" for
this track, and it is structural, not a tuning matter.

### 2.4 The measured gain is concentrated on a corpus the register already disqualifies

`[M]` per-file `anvil-mdl-rans` vs `brotli-q4` and vs `anvil-greedy-rans`:

| file | MDL vs q4 | MDL vs greedy |
|---|---:|---:|
| generated.log | **−40.24%** | −29.67% |
| generated.json | **−34.88%** | −30.14% |
| generated.jsonl | **−19.50%** | −33.42% |
| synth-jitter.bin | −6.20% | −12.44% |
| anvil_bench.exe | −0.52% | −3.62% |
| generated.sqlite | **+15.06%** | −17.45% |
| generated.repeat.jsonl | +504.21% | 0.00% |
| src.cpp | +2.21% | −3.77% |
| anvil.exe | +2.69% | −4.40% |
| synth-timeseries.bin | +4.54% | 0.00% |
| doc.md | **+18.19%** | −4.84% |
| synth-arith.bin | +50.10% | 0.00% |
| random.bin | +0.01% | 0.00% |

MDL's *large* wins are on `generated.{json,jsonl,log}` — the exact family
do-not-reburn **§C1** classifies as "Large self-test win, but canonical text/corpus
evidence does not show general transfer. Keep as a stress test"
(`06-do-not-reburn.md:65-68`), reinforced by §C2 for context modeling
(`:69-72`). Meanwhile on natural-language/source data a general-purpose codec must
handle — `doc.md` **+18.19%**, `generated.sqlite` **+15.06%**, `src.cpp` +2.21%,
`anvil.exe` +2.69% — MDL is **worse than brotli q4**.

`[D]` On `doc.md` and `generated.sqlite` the measured-cost MDL parse is an active
**regression against the reference**, not merely a slow win. A mechanism that is
8-18% worse than q4 on markdown and SQLite cannot be a general-purpose parser.

---

## 3. Mechanism-correctness defects found by code read (independent of cost)

These are real defects in the chartered mechanism. Each is `[C]` with file:line.

### 3.1 The cost model's distance term is unrealizable by construction

`MatchFinder::find` truncates the candidate set by **length only** before any cost
model sees it:

```cpp
std::sort(out.begin(), out.end(), [](auto&a, auto&b){ if(a.len!=b.len)return a.len>b.len; return a.dist<b.dist; });
out.resize(8);
```
— `src/anvil.cpp:517-520` (in `walk`) and `:550-553` (in `find`)

Distance survives only as a tiebreak among **equal-length** candidates. This directly
contradicts the mechanism's own stated design in two comments:

- `src/anvil.cpp:538-539` — "aligned sources win via the cost model's near-distance
  preference"
- `src/anvil.cpp:3623` — "near distances dominate measured ds cost"

`[D]` The MDL objective *does* charge per-distance varint cost (`varint_cost_ms(m.dist-1,c.ds)`,
`:3647`), so it is distance-aware; but the candidate set it is allowed to choose from
was already length-ranked. The objective is being asked to optimize over a set that
was filtered by a different criterion. **Greedy is the mirror image**: it consumes all
≤8 by longest-then-nearest (`:745`), so greedy sees distance only as a final tiebreak
too — which is why the two families' ratio ordering is not a clean cost-model story.

### 3.2 The empirical-lag cost model cannot extend the greedy symbol support

```cpp
out[i]=cnt[i]?std::clamp(-std::log2(double(cnt[i])/tot),0.1,16.0):20.0; // absent syms never chosen
```
— `src/anvil.cpp:3584` (in `measure_stream_costs`)

`MdlCosts` is measured from the **previous** pass, and the seed pass is
`parse_greedy` (`:3676`). Any symbol absent from the previous pass is priced at
**20 bits**, so the next pass's DP will not select it. `[H]` The cost model can
therefore only ever *shrink* the greedy parse's symbol support, never discover new
symbol usage — a ratchet, not an MDL iteration.

`[M]` This is directly observable: **4 of 13 files are byte-identical to greedy under
MDL** (`generated.repeat.jsonl` 1148 B, `random.bin` 262,166 B,
`synth-arith.bin` 256,022 B, `synth-timeseries.bin` 155,739 B — all exactly equal to
the greedy row). On `synth-timeseries.bin` MDL pays **1.611 vs greedy 7.837 MB/s =
4.86× the encode cost for exactly 0 bytes.** `[D]` A mechanism that provably cannot
change the output on 31% of the corpus while costing 4.86× is not converging; it is
idling.

### 3.3 The windowed DP is not the global DP its comment claims

`src/anvil.cpp:3611-3613` claims the windowed pass "makes the same GLOBAL edge
choices as the old whole-block DP". `[C]` **This is false as written**: `kWin = 16384`
(`:3620`), match lengths are clamped to the window remainder
(`cap = std::min(m.len, win_remain)`, `:3639,3643`), and no edge crosses the window
boundary. The claim must be demoted from comment-as-fact to hypothesis. `[D]` The
byte cost of forced re-tokenization at window edges is small (≤16 forced extra tokens
per 256 KiB block), but the **16 KiB bounded lookahead** is a genuine decision-quality
restriction on exactly the long-range structure that dominates `.text` and logs — and
§4.1 shows the MDL cost model is *harming* precisely on long-range text.

### 3.4 Non-monotone iteration: the cost model decouples from the retained parse

```cpp
if(t2<best_total){ best_total=t2; best=std::move(cand); }   // :3697  retained only if improved
mark_starts(best);
if(it>1 && t2+1>=best_total) break;                          // :3699  compares NEW pass to OLD best
c=cm; total=t2;                                              // :3700  cost model updated UNCONDITIONALLY
```
— `src/anvil.cpp:3694-3702`

`best` is retained only on improvement, but `c = cm` is adopted **unconditionally**,
including from a *rejected* pass. `[H]` The iteration therefore converges toward the
last-rejected pass's stream statistics while the retained parse is an older one, and
the `t2+1>=best_total` exit test compares a new pass against the retained best, so a
two-state oscillation can terminate early. The header claim "converges in a few passes"
(`:3556`) is unverified. Deterministic (so reproducible), but not monotone.

### 3.5 "Measured cost" is charged twice — the clearest "never repays" mechanism

```cpp
for(const auto* v:{&types,&ll,&ml,&ds,&lits}) total+=encode_stream(*v).size();
```
— `src/anvil.cpp:3605`

`measure_parse` **actually runs the entropy encoder on all five streams purely to
compute the objective, then discards the bytes** (`:3606` returns only the cost
struct and the size). It is called once for the seed and once per refine iteration
(`:3677`, `:3696`) — up to **4 calls × 5 streams = 20 throwaway stream encodes per
block** — and *then* `consider_parse` re-encodes the winner (`:4709`).

`[D]` So the MDL parser spends ≈4 full-block entropy encodes deciding *which* parse to
use, before the chosen parse is encoded at all. Combined with §2.2, the encode cost
of *choosing* is the same order as the encode cost of *compressing*. No ratio gain of
the measured 2.13% magnitude can service that.

### 3.6 Default-`false` boundary machinery is computed and discarded

`mark_starts` writes a ±8-byte dilation (17 writes per token per iteration) into
`bseed` on every iteration (`src/anvil.cpp:3685-3698`), but `bseed` is consumed **only**
when `boundary` is true (`:3624`), and `opt.boundary` defaults to **false** — annotated
in the source itself as "measured neutral on corpus" (`:3777`). `[D]` Pure overhead
proportional to token count × iterations, thrown away by default.

**Do-not-reburn §D2 is already CLOSED on this**: "More boundary-candidate heuristics
on the old parser — Measured output was unchanged; the existing cost model already
absorbed the signal" (`06-do-not-reburn.md:85-87`). Track 17 must not re-propose it.

---

## 4. Hidden costs the ledger does not charge

### 4.1 Cycle accounting: the cost is relaxation count, not memory

Host clock 4.2 GHz base (`tests/host-spec.md:15`) `[C]`; boost clock unmeasured, so
cycle figures below are `[P]` projections, clearly labelled.

`[P]` At MDL's measured 0.718 MB/s, encode is 1.393 µs/input-byte ≈ **5,850
cycles/byte** at 4.2 GHz. Greedy's measured 14.151 MB/s ≈ 70.7 ns/B ≈ **297
cycles/byte**. `[D]` The ≈20× ratio is consistent with the relaxation-count structure
(≤64 relaxations/position/pass × ~3 passes + seed, vs greedy's 1 decision/position),
not with any memory-bandwidth term.

**I falsified my own leading memory hypothesis arithmetically, and it should be
recorded as rejected rather than repeated by the next reviewer:**

`[C]` `MatchFinder` allocates and zero-fills **four** arrays unconditionally
(`src/anvil.cpp:524-526`): `head_` (2^18 × 4 B = **1 MiB**), `prev_` (`d.size()` × 4 B =
1 MiB at the default 256 KiB block), `bhead_` (**1 MiB**), `bprev_` (1 MiB) = **4 MiB
per construction** — of which `bhead_`+`bprev_` = **2 MiB is dead weight when
`boundary=false`**, i.e. by default.

`[C]` `parse_mdl` constructs **four** `MatchFinder`s per block: one inside
`parse_greedy` (`:3676` → `:739`) and one per `parse_mdl_pass` (`:3623`, up to 3
passes). `[D]` 4 × 4 MiB = **16 MiB zero-filled per 256 KiB block = 64 bytes of memset
per input byte.**

`[D]` **But at 1.393 µs/B budget and a plausible 10-30 GB/s memset bandwidth, 64 B/B
costs ≈2-6 ns/B = 0.15-0.45% of the MDL budget.** The waste is real, it is a clean 2×
when boundary is off, and it is **immaterial**. Do not present it as the bottleneck; do
present it as a free 2× if someone ever revisits this lane.

The 16 KiB window keeps `dp`/`prev` L1/L2-resident (`:3625-3626`) — a genuinely good
design choice, and the reason the windowed DP beats the whole-block DP on speed at
all. The whole-block `parse_dp` instead allocates `dp` (n+1 doubles = 2 MiB) and
`prev` (n+1 × 16 B = 3 MiB) (`:772-773`) plus the 4 MiB finder = **≈9 MiB live
working set for a 256 KiB block**, with the inner relaxation scattering stores into
`dp[j]` at stride up to 64 KiB (`:785-791`). `[M]` Measured result: whole-block DP is
**1.124 MB/s and 2,539,808 B**; windowed MDL is **0.718 MB/s and 2,439,834 B**. `[D]`
So MDL's *entire* measured advantage over the plain global DP is **4.06% bytes for
1.57× encode** — and both sit 90-140× behind q4's encode. A 4.06%/1.57× trade inside
an already-losing region is not a frontier.

### 4.2 Sparse-path hidden divide

`[C]` `src/j % dist` — a **runtime-divisor modulo per scanned byte** in the
overlapping-copy case (`src/anvil.cpp:592`). `[P]` On Zen 3 (host is a Ryzen 9 5900X,
`tests/host-spec.md:13`) a 32-bit `div` is ~14-20 cycles and is poorly pipelined in a
dependent chain, and a runtime divisor cannot be strength-reduced into a counter by
clang. This feeds modes 11/12/13/14/15 — i.e. every sparse-derived family.
`[P]` **Not measured; do not cite as fact.** It is plausibly mitigated by the
`dead_band` early exit (`:613`) and the per-block work cap (`:591`, `kSparseBlockBudget`
= 64 MiB scanned bytes). It is listed because it is a *specific, cheap-to-test,
encoder-side* hypothesis and it is the one cycle-level lever in this track that is not
already closed by `06 §D3` (reciprocal-rANS: measured slower than hardware divide).

### 4.3 Candidate explosion: the shipped default is an unmeasured 24-encode portfolio

`[C]` `opt.parse` defaults to **`"auto"`** (`src/anvil.cpp:3772`). This directly
contradicts `docs/CONTEXT.md`'s own known-trap entry: "`parse=auto` brute-force:
research-only, never the production default". `[C]` A documented-vs-code contradiction.

Under `auto` the block router runs **8 parse/representation families** — greedy, dp,
mdl, sparse, shape (over 2 token sets), topology, tcopy, hotop
(`src/anvil.cpp:4713-4783`) — and `consider_parse` triggers **up to 6 full-block
encodes per token set** (arith literal modes o0/o1/g4/g8/g16, then rANS;
`:4697-4711`) for greedy, dp and mdl.

`[D]` Total per block: 6 + 6 + 6 + 1 + 2 + 1 + 1 + 1 = **24 full-block entropy
encodes**, plus 8 parser runs, plus MDL's internal 20 throwaway stream encodes (§3.5)
— before a single byte is emitted.

`[M]` **There is no `anvil-auto` row anywhere in `tests/benchmark-suite.csv`.** I
enumerated all 27 codec labels (`anvil-greedy-arith` … `zstd-19`); none is the default
configuration. `[D]` **The shipped CLI's default configuration is entirely unmeasured
in this repo.** Every parser claim in this track therefore rests on a configuration
that is not the product, and the product is unmeasured.

### 4.4 Provenance and leakage audit of the only parser comparison that exists

| Property | Status | Source |
|---|---|---|
| Corpus | 13 files, `tests/corpus/*`, **local, discovery** | `tests/host-spec.md:52` |
| Aggregate | total-in 12,272,327 B, byte-weighted | verified `[D]` |
| Family | `generated.*` = **stress-only per `06 §C1/C2`** | `06-do-not-reburn.md:65-72` |
| Hyperparameter tuning | `surprise` default 12, annotated "**swept → 12 beats 6 on corpus**" | `src/anvil.cpp:3775` |
| Same on `shape_states`? | default 28, "per-shape" | `src/anvil.cpp:3776` |
| `boundary` | default off, "measured **neutral** on corpus" | `src/anvil.cpp:3777` |
| Reps | median of **3** | `tests/host-spec.md:56` |
| Paired CIs / bootstrap | **absent** | vs `I10-DENSE-FRONTIER-PREREG.md:199-211` |
| Robust CV / ambient / A/A null | **absent** | same |
| CPU affinity pin | **absent** | required by prereg `:210` |
| Output hash validation | round-trip flag only | `tests/host-spec.md:56` |
| Reference density | 5 Brotli levels only; **xz absent** | prereg `:117-119`, `:173-175` |
| Canonical corpus (Silesia/enwik8) | **never used for this family** | prereg `:4,71-100` |
| Held-out families available | **none** — "No new held-out structured corpus is currently available" | `RESEARCH_LEDGER.md:4831-4832` |
| Published frontier arm | `--parse=ratio` — **a different parser entirely** | prereg `:110-111` |

`[D]` The parser hyperparameters were selected on the same 13 files whose rows are
reported, three of which (`generated.{json,jsonl,log}`) carry the mechanism's headline
wins. This is discovery-set evidence with in-sample tuning, presented in a table whose
neighbours are remote-grade. Under MASTER-BRIEF doctrine 6 (no cherry-picking;
discovery/validation/held-out separated) and doctrine 10 (provenance), **no ratio
number in §2 may be cited as a validated result.**

---

## 5. Prior-art map (adopt-class assessment)

| Element of the charter | Prior art | Status |
|---|---|---|
| Iterated reparse against a measured downstream cost, no fixed-point guarantee | **Zopfli** (maza, 2013) | adopt-class |
| Bounded-window optimal parse over a probability cost model, probabilities updated between passes | **lzma `GetOptimum`** (7-Zip SDK `LzmaEnc.c`) | adopt-class |
| Per-block cost model + block-switching + static/dynamic code cost comparison | **RFC 7932 Brotli** | adopt-class |
| Length-descending candidate truncation with distance tiebreak | LZ77 / zlib `longest_match`, zstd match finder, LZMA rep0-3 | adopt-class |
| Cost oracle = "call the real entropy coder and measure" | iterative-refinement / encode-and-measure family | adopt-class |

`[D]` The only defensible novelty in the charter is the **cost oracle's granularity**:
measuring against ANVIL's actual rANS stream construction *including* the per-stream
raw-vs-rANS selection (`encode_stream`, `:1548`), rather than against a probability
model. `[D]` But an oracle refinement is worth exactly zero bytes **if it does not
change the selected parse** — and §3.1/§3.2 identify two defects that mean the current
oracle *is* changing selections in the wrong direction on natural text (§2.4: +18.19%
on `doc.md`, +15.06% on `generated.sqlite`).

`[P]` **Honest novelty verdict: the parsing *formulation* is adopt-class.** Any track-17
novelty claim must be about the interaction (oracle granularity × a specific
representation), and per MASTER-BRIEF doctrine 14 that interaction must move the
Pareto frontier — which §1 and §2.3 show it cannot, because it is not on the frontier
route.

---

## 6. Decoder / resource risks

`[D]` Track-17 mechanisms add **zero decoder-visible bytes** — they are encoder-side
selection over representations the decoder already implements. That is the one thing
the charter gets right, and it must be stated as a genuine positive before the
criticism: no new framing field, no model, no dictionary, no table.

The resource risk is elsewhere and is already banked against the canonical route:

- `[M]` I10-1A aux-unBWT: **+8,083 B** across three paired targets for 1.364×/1.795×/
  2.339× whole-codec decode gains, classified **FRONT-GAP_COST**, not a crossing
  (`RESEARCH_LEDGER.md:4715-4718`).
- `[M]` Diagnostic reference-cost run `36061511123`: ANVIL aux peak RSS **248.4 MiB**
  (Silesia) and **598.9 MiB** (enwik8) vs brotli q11/lw30 **124.5 / 251.9 MiB** —
  `[D]` **2.03× and 2.38× the reference's decode memory**, while decode is
  3.53-3.68× *faster* (`docs/I10-AUX-UNBWT-RESULTS.md:594-607`).
- `[M]` enwik8 that run is `TIMING_BLOCKED` (Brotli control robust CV 0.1756) — i.e.
  even the reference-cost axis is not currently clean (`RESEARCH_LEDGER.md:4794-4804`).
- `[C]` Decoder strictness is present and real (`decode_one_block` rejects unknown
  modes, truncated payloads, out-of-range distances, trailing arithmetic bytes,
  `:3764`, `:4807-4834`; `decode_ratio_block` bounds `xlen` to `2*out_len+4096`,
  `:4646-4647`). Track 17 adds no new decoder surface, so no new fuzz surface either.

---

## 7. Alternative mechanism

### 7.1 First, a negative: I built no prototype, deliberately

`06-do-not-reburn.md:113-122` (§E5) requires, *before* implementation, that enough
bytes are exposed, metadata can amortize, **the cost objective can actually alter a
decision**, and the reference gets the same representational opportunity. §1 answers
the last two negatively for the canonical route. Building a prototype first would
violate the register the brief binds me to. No file was created under
`prototypes/swarm-2026-10-02/17-parser-algorithms/fledge/`.

### 7.2 The only in-scope lower-complexity mechanism: cost-ranked single-pass selection (CRSS)

Keep **one** pass. Delete the windowed DP, the reparse loop, and `measure_parse`'s
discarded encodes. Change exactly two things inside the existing single-pass parser:

1. **In `find()`, replace length-descending truncation** (`:517-520`, `:550-553`) with
   selection of the argmin of the measured incremental cost
   `ttype[1] + ml_cost(len-4) + ds_cost(dist-1) − len × avg_lit` over the ≤8
   candidates. This directly repairs §3.1 — it gives the cost oracle the distance term
   it is already paying for, at O(8) per position.
2. **Relax 2 edges per position instead of ≤64**: the literal edge and the single
   cost-argmin match edge. This is the entire relaxation-count reduction.

`[P]` Predicted encode ≥10 MB/s (within ~1.4× of greedy's measured 14.151), i.e.
~10× behind q4 instead of **140.7×**. **This is a projection and is explicitly not
measured.** `[H]` Whether it captures any of MDL's ratio signal is the open question.

**Distinctness check against closed lanes** (this matters, and it is the reason CRSS
is a PILOT and not a duplicate): `06 §D2` closed **boundary-\*source\*** heuristics
(adding candidate *sources*); `§B3` closed **channel reinforcement** (reinforcing a
period the parser already picked); `§B4` closed **PNRA candidate injection**; `§B5`
closed **RLZ/RePair stream construction**. CRSS changes the **selection criterion over
candidates the finder already returns** and **removes** work rather than adding it. It
is adjacent to §D2 and must be argued as distinct; if the coordinator judges it
in-distinct, treat it as closed.

### 7.3 The alternative that is *actually* on the frontier route (and is not mine)

`[D]` The canonical route's only lever is the transform × backend portfolio, its
measured win is 159 B above a 2-codec oracle, and block-local routing is decisively
closed (`06 §G1`: sum-of-4 MiB-blocks **+894,315 B / +12.22%** warmup tax; 256 KiB
**+5.42 MB**; zero BWT-winning blocks in mozilla/samba/ooffice/sao). Therefore the
only unexhausted lever on that route is **additional cheap whole-block reversible
transforms that change which backend wins**, each with a decoder-cost proof and a byte
pre-filter so only payload-shrinking transforms survive into the router.

`[C]` I am explicitly **downgrading** this to "not a track-17 mechanism". It contains
no parser, no candidate index, and no asymptotic content; it belongs to track 16
(segmentation/routing) and tracks 13/14 (representation compiler), and it runs into
the `06 §A2` zero-bit-derived-transform closure and the killed global lane
transposition (MASTER-BRIEF doctrine 13). I record it so the coordinator can route it,
not to claim it.

---

## 8. Decisive remote-only falsification experiment (thresholds frozen NOW)

**Thresholds below are frozen before any run and must not move after seeing results
(doctrine 5).**

**Vehicle:** extend the existing frozen dense-frontier workflow
`.github/workflows/anvil-i10-dense-frontier.yml` — reused unchanged protocol, **no new
timing methodology**. Pinned tooling `b6a243c9657776058a37e5f0ee2eaae2ff925dff`;
`tools/paired_bench.py` blob `f847c50e…`; 7 paired reps (9/13 allowed), 2 warmups,
seed 41246 + frozen per-file offset, bootstrap 20,000, practical-effect epsilon 2%,
max arm robust CV 15%, ambient 5 × 250,000 with max robust CV 10%, `taskset -c 0`,
one thread, 3,600 s timeout, Silesia 12-file byte census + frozen 5-file timing panel,
enwik8 both (`I10-DENSE-FRONTIER-PREREG.md:32-100,199-215`).

**Corpus:** Silesia + enwik8 **only**. `[C]` The local 13-file `tests/corpus` set is
**excluded by construction** — it is the discovery set on which `surprise=12` was swept
(`src/anvil.cpp:3775`).

**Question Q1 (route):** does any legacy-parse candidate-generation change alter mode-17
bytes? `[C]` **Already answered by `src/anvil.cpp:4614-4639`: no — `encode_ratio_block`
contains no parser.** Recorded as a structural fact, not an experiment. No compute is
spent re-asking it.

**Question Q2 (the one decisive run):** on the *held-out* corpus, at paired-protocol
rigor, does CRSS recover parse-decision ratio at greedy-class encode cost?

| Arm | Command | Purpose |
|---|---|---|
| A0 | `--parse=greedy --entropy=rans` | fast-parser control |
| A1 | `--parse=mdl --entropy=rans` | the 140.7× incumbent |
| A2 | `--parse=crss --entropy=rans` (isolated prototype flag; **no production default change**) | the falsifier |
| A3 | brotli q4 / q9 / q11, xz -9e | reference class, full density |

Also record, per arm: complete payload bytes, per-process peak encode RSS
(`time -f %M`), paired encode + decode seconds, original and `strip --strip-unneeded`
sizes, output SHA-256 per repetition. Hash stability across reps is a hard gate; A/A
null is `A0` vs `A0`.

**Pre-registered decision rule** (aggregate over Silesia + enwik8, bytes):

- **KILL track 17** (mechanism class, not just the arm) if **any** of:
  - `A2` total bytes ≥ brotli **q4** total bytes (fails to reach parity with q4 at
    ≥10 MB/s); **or**
  - `A2` recovers **< 50%** of `A1`'s byte gain over `A0`. 50% of the *measured local*
    A1-over-A0 gap is the frozen bar — `[D]` on the local aggregate that gap is
    2,743,742 − 2,439,834 = **303,908 B**, so the local-equivalent bar is
    **≤ 2,591,788 B**; the remote run uses its own Silesia+enwik8 equivalents and this
    local number is *context, not the gate*; **or**
  - `A2` ≥ 2× `A0` encode time **without** ≥3% aggregate byte gain over `A0`; **or**
  - `A2` is byte-regressing versus `A0` on **≥3 of the 5 timing-panel files**; **or**
  - `A2` encode peak RSS > 2× `A0`'s.
- **PILOT** (authorize exactly one further remote run; never production) if `A2`
  recovers ≥50% of `A1`'s gain over `A0`, runs ≥10 MB/s aggregate encode, and regresses
  on **0 of 5** timing-panel files including a natural-text file.
- **HOLD** if `A2` recovers ≥50% of `A1`'s gain but aggregate encode remains >5× q4's
  (i.e. correct science, still non-Pareto) — park as enabling infrastructure per the
  `CONTEXT.md` context-clustering precedent.
- **PROMOTE-TO-REMOTE: not available for track 17 as chartered.** No parser byte reaches
  the canonical route (§1). Any promotion must be labeled **infrastructure**, and must
  satisfy `06 §F1`-style accounting: metadata/codebook cost ≤20% of gross savings.

**Explicit falsification criteria I will accept against myself:**
1. If `A2` matches `A1`'s bytes at ≥10 MB/s, my §2.2/§2.3 kill is **wrong** and the
   relaxation-count argument collapses — track-17 would be rehabilitated.
2. If `A2` beats q4 on held-out bytes at ≥10 MB/s, the §2.4 corpus-transfer attack is
   **wrong**.
3. If a full `anvil-auto` census (§4.3) shows the default portfolio is *not* 24 encodes
   per block, my §4.3 count is wrong.

---

## 9. Verdict

# KILL — track 17 as chartered ("parser/candidate-generation algorithms for
# measured-cost MDL") · retain §7.2 CRSS only as an infrastructure PILOT candidate

**KILL is justified on four independent grounds, any one of which is sufficient:**

1. **Structural (decisive).** `[C]` The canonical frontier arm is `--parse=ratio`, and
   `encode_ratio_block` (`src/anvil.cpp:4614-4639`) contains no parser, no hash chain,
   no DP, no candidate generation. `[D]` 99.9997% of the canonical Silesia result
   (46,446,995 B vs a 46,446,836 B two-codec oracle; residual **159 B**) is a routing
   choice between two existing codecs. Track 17 has **zero leverage** on the frontier
   route. This is settled by inspection, not by a benchmark.
2. **Measured domination.** `[M]/[D]` `anvil-mdl-rans` (0.198808 / 0.718 MB/s /
   136.550 MB/s) is beaten by `brotli-q11` (0.145713 / 0.573 / 450.937) on **bytes
   (−26.7%), encode (−20.2% time), and decode (−3.30× throughput) simultaneously.**
   Total domination by a single existing point.
3. **Mechanism-class unrepayability.** `[D]` The 2.13% byte gain over q4 costs 140.7×
   encode, and the 3.64× *decode* deficit is unreachable by any encoder-side mechanism.
   A ratio gain that cannot address the axis where the deficit lives is not a Pareto
   route.
4. **Provenance insufficiency.** `[C]/[D]` The only parser comparison in-repo is a
   13-file local discovery corpus with in-sample hyperparameter sweeps
   (`surprise` "swept → 12 beats 6 on corpus", `src/anvil.cpp:3775`), no paired CIs, no
   CV, no A/A null, no affinity, no xz reference, and **no held-out structured corpus
   exists** (`RESEARCH_LEDGER.md:4831-4832`). No track-17 ratio claim is admissible
   under doctrine 6/10.

**What survives, honestly stated** — this is the strongest case against my own verdict
and it is why §8 exists rather than a bare KILL:

`[M]` The parse-decision signal is **large and real where it appears**: −29.67% to
−33.42% vs greedy on `generated.{log,json,jsonl}`, −17.45% on `generated.sqlite`,
−12.44% on `synth-jitter.bin`. `[D]` No fast ANVIL parser captures it — the best of
them is `anvil-hotop-rans` at 0.215307 / 7.963 MB/s and `anvil-sparse-rans` at
0.212605 / 8.525 MB/s, both far worse than MDL's 0.198808. So roughly **5 percentage
points of aggregate ratio is attributable purely to parse decisions and is currently
reachable only at ~1 MB/s.** §3.1 and §3.2 identify two *specific, cheap* reasons the
incumbent mechanism is both 140× too slow and actively harmful on natural text: the
candidate set is length-filtered before the cost oracle can use distance, and the
empirical-lag cost model with its 20-bit exclusion floor can only shrink the greedy
symbol support (visibly: 4/13 files byte-identical to greedy, one of them at 4.86×
the encode cost for zero bytes).

`[H]` So the *scientific* question — **is the parse-decision signal reachable at
greedy-class encode cost?** — is open, is not closed by any `06` entry (§D2 is
boundary-*source* heuristics; §B3 channel reinforcement; §B4 PNRA injection; §B5
RLZ/RePair), and is answerable in **one** remote run. That is a PILOT of the
*falsifier*, not a promotion of the mechanism. If the coordinator's bar for a track is
"is there a cheap decisive experiment left?", the answer is yes; if the bar is "can
this move the frontier?", the answer is no.

**Explicitly NOT recommended:** any prototype wired into production; any new default;
any change to `--parse=auto`'s default; any reuse of `anvil-mdl-rans` rows as validated
ratio evidence; any re-proposal of boundary-candidate work (`06 §D2` closed); any
broadening of track 17 into the mode-17 transform portfolio (not mine — route it to
track 16 / 13 / 14).

**Cheapest decisive next check (one, remote-only):** the §8 Q2 three-arm run
(A0 greedy / A1 mdl / A2 CRSS) on Silesia + enwik8 under the unmodified frozen
dense-frontier protocol, judged solely by the frozen decision rule. One run, no new
methodology, no new corpus, no local measurement. It cannot change §1 (which is
structural), so **its only possible effect is to convert this KILL into a bounded
infrastructure PILOT** — and that is exactly the right size of bet to leave open.

---

## 10. Reconciliation slot for the constructive report

`docs/swarm-2026-10-02/17-parser-algorithms-space-bunny.md` was absent at audit time.
I did not read, infer, or anticipate it. When it lands, I will adjudicate these points
and **only** these, in this order:

1. **Does it claim any parser/candidate optimization reaches `encode_ratio_block` /
   mode 17?** If yes, that claim is false on inspection (`:4614-4639`) and the item is
   downgraded to infrastructure per the coordinator's own instruction. If it concedes
   this, there is no disagreement and the KILL stands unchanged.
2. **Does it cite `anvil-mdl-rans` 0.198808 as a ratio win?** If yes, it must be
   answered with §2.1 (dominated by q11 on all three axes) and §2.4 (the win is on a
   `06 §C1` stress-only family, and MDL is +18.19% on `doc.md`, +15.06% on
   `generated.sqlite`).
3. **Does it propose a *new* candidate index, SIMD structure, or hash layout?** Those
   are the one class I have *not* independently falsified, because none exists in
   `src/anvil.cpp` to falsify. I will hold it to §4.1: any such proposal must show a
   measured relaxation-count or memory-traffic reduction **on the canonical route**,
   and per §1 there is no candidate generation there to accelerate. Absent that, it is
   infrastructure.
4. **Does it report measured numbers I cannot trace to an artifact?** Per doctrine 10
   each must name its artifact or be relabeled projection. Any number I cannot source
   in-repo I will mark `[U]` (unverified) and it will not move this verdict.
5. **Does it propose backend/BWT ratio-route economics as a parser result?** Those are
   track 18/16 territory and are governed by `06 §G1` (block-local routing closed) and
   the `FRONT-GAP_COST` / `TIMING_BLOCKED` rulings (`RESEARCH_LEDGER.md:4715-4718`,
   `4794-4804`). Parser speedups and backend/BWT route economics are **not
   interchangeable axes** and must not be netted against each other.

**What would change this verdict to HOLD or PILOT:** evidence that a candidate-generation
or cost-oracle change moves mode-17 bytes on Silesia+enwik8 — which requires
`encode_ratio_block` to acquire candidate generation, i.e. a format/representation
change, not a parser change. **What would change it to PROMOTE-TO-REMOTE:** nothing
within track 17's charter. That verdict is structural.

---

# APPENDIX R — Pair reconciliation (constructive report read; verdict NOT inherited)

**Source read:** `docs/swarm-2026-10-02/17-parser-algorithms-space-bunny.md` (520 lines,
FINAL, stamped after my interim). **I did not inherit its conclusions.** Below: what I
independently confirm, what I correct, what I add, and what survives of my own §8.

## R1. Convergence on the load-bearing finding — and it is now doubly settled

Both lanes reached the same structural fact by **different routes**, which strengthens
it: I derived it from `encode_ratio_block` (`src/anvil.cpp:4614-4639`) plus the Silesia
frozen byte table; they derived it from `tools/reference_cost_gate.py:136-138` plus a
whole-file grep proving every parser symbol sits after the `continue` at `:4681`.

`[C]` Both independently confirm: `--parse=ratio` executes zero ANVIL candidate-generation
lines; the frontier arm is `min(Brotli, BWT)` over identity.

`[D]` **Additive confirmation neither lane had:** their §4 arithmetic cap (escape at
**59.722 MB/s** aggregate encode at fixed bytes) and my §2.2/§2.3 (140.7× repay gap,
3.64× decode deficit unreachable by encoder-side work) are *the same wall from two
sides*. Their cap is derived from the measured dominator set; mine from the
mechanism-class/repayability identity. `[H]` They are mutually reinforcing: even a
*perfect* encoder-side parser that changes zero bytes stays inside their cap, and even a
*free* parser speedup stays inside my decode-deficit argument. Neither route to a
crossing exists.

**Verdict difference, stated precisely rather than smoothed over.** They conclude
`HOLD — infrastructure`. I conclude `KILL` for the mechanism as chartered, with the
residual parked as adopt-class engineering. `[D]` The difference reduces entirely to
their **U4** ("will token-LZ be promoted to a ratio backend?") — a coordinator product
decision. **I resolve U4 negatively for now (§R6)**, which collapses HOLD-infrastructure
into KILL-plus-parked-code. I do not think their HOLD is wrong; I think it is
conditional on a decision nobody has made, and §R6 shows the arithmetic says don't make
it yet.

**Integrity note, minor but on the record:** their §11 says "two zero-risk adopt-class
encoder fixes" while their §12 tables **three** (F1, F2, F3). I audit all three.

## R2. F1 — the inner-loop modulo: semantics-preserving, but NOT a trivial edit

**Claim:** `src/j % dist]` (`:592`) → phase counter.

**Semantics audit `[C]`.** The `dist > 0` ternary is **dead**: every one of the three
`scan_candidate` call sites guarantees `dist ≥ 1` — `find_sparse_at` rejects
`dist == 0` (`:632`) then sets `q = pos - dist`; `find_sparse` breaks on `q >= pos`
(`:701`); `find_pnra_at` rejects `q >= pos` (`:661`). So the replacement must reproduce
`j % dist` exactly, and no zero-division path exists. Verdict: **equivalent in
principle.**

**The bug the constructive report misses `[C]`.** `scan_candidate` has **two** increment
paths: `++j` in the for-header (`:590`) and `j += 3; continue;` in the TCOPY
transform-field branch (`:600-601`). A naive `++k` per iteration **desynchronizes the
phase by 3** after the first transform field. The correct fix must advance the counter
by 3 on that path.

`[D]` Useful mitigating fact: the TCOPY branch requires `j + 4 <= dist` (`:594`), so
while it fires `j < dist` always holds, `j % dist == j`, and the skip cannot cross a
period boundary — which is exactly why the bug is *silent* rather than loud.

`[D]` **Failure mode is silent ratio change, never corruption.** `cpy` only decides
whether a byte is scored as a match or recorded as a correction in `off`/`val`; the
decoder copies and applies *stored* corrections, so roundtrip stays byte-exact while
density silently moves. Gate 1 would catch it only where the corpus exercises TCOPY
transform fields. This raises the review bar on the diff, it does not change the verdict.

**Routing finding the report misses `[C]`.** `scan_candidate` is called only from
`find_sparse_at` (`:641`), `find_pnra_at` (`:666`) and `find_sparse` (`:705`) — i.e.
the **sparse** family. `parse_mdl` reaches candidates through `MatchFinder::find`
(`:3638`) → `walk` → `match_length` (`:477-487`), and **never calls `scan_candidate`**.
Therefore **F1 cannot affect rev-1 MDL encode at all**, and cannot affect §4's 59.722
MB/s cap, which is computed on the MDL row. F1's blast radius is
`anvil-sparse-rans` and modes 12/13/14/15.

**Speedup bound from existing data `[P]`.** Host is Zen 3 (`tests/host-spec.md:13`);
32-bit `DIV` is ~14-20 cyc latency, ~1/6-1/13 throughput, and a runtime divisor is not
strength-reduced. A counter is ~1-2 cyc. Ceiling **on the loop** ≈ 1.4-2×, because the
body also carries FP `score` updates on `double` (`:599,607,610`) and 4-byte `memcpy`
pairs (`:596-597`). The loop is a minority of sparse-family encode, and `dead_band`
early-exit (`:613`) plus the work cap (`:591`) cut scanned bytes further. **F1's
aggregate speedup is not bounded by existing data and is plausibly small.**

## R3. F2 — per-position heap traffic: semantics-preserving; the bound is parser-dependent

**Claim:** `walk()` returns `std::vector<Match>` by value (`:500-522`); `find()` then does
`insert` / `std::sort` / `resize(8)` (`:536-555`).

**Semantics audit `[C]`.** A fixed-capacity `std::array<Match,8>` + count reproduces the
sort-and-truncate exactly (`std::sort` on ≤8 elements with the same comparator, then
`resize(8)` is equivalent to "keep min(8, n)"). No observable difference: `find()`'s
result is consumed read-only at both call sites (`parse_greedy:743-745`,
`parse_mdl_pass:3638-3643`). Verdict: **semantics-preserving**, and it removes the only
per-byte heap traffic on the MDL path.

**Speedup bound — and here the routing matters `[P]`.** Per position: 2 vector
constructions with ~3 growth reallocs each, plus one `insert`-driven realloc, plus a
sort of ≤16, plus 2 destructor pairs ≈ **6-8 malloc/free pairs**. At ~20-40 cyc per
hot pair that is **~120-320 cyc per input byte** (one `find()` per byte per pass).

`[D]` Applied to the measured rows, this gives sharply *different* bounds:

| row | measured cyc/B | allocator share | implied ceiling |
|---|---:|---:|---:|
| `anvil-greedy-rans` (14.151 MB/s) | 297 | ~40-100% | **~1.7-2.9×** |
| `anvil-mdl-rans` (0.718 MB/s) | 5,849 | ~2-5% | **~1.02-1.06×** |

`[H]` **This is the load-bearing bound: F2 cannot repair the rev-1 MDL encode collapse,
because MDL's budget is dominated by the 105 measurement codings and the ~120
relaxations/byte of DP, not by allocator traffic.** F2's large win lands on the
*fast* parsers — which are not the mechanism under study.

**And F2 cannot create a crossing even where its win is large `[D]`.** Making greedy
faster does not help, because `anvil-greedy-rans` ratio **0.223571** is already
**10.07% worse** than `brotli-q4` **0.203129**. An infinitely fast greedy row is still
ratio-dominated by q4. Same argument as §2.1, one row over.

## R4. F3 — verified safe, and confirmed to be a third item

**Semantics audit `[C]`, independently verified.** Every reader and writer of
`bhead_`/`bprev_` is guarded by `use_boundary_`: read at `:542-544` inside
`if (use_boundary_)`; writes only inside `insert_boundary` (`:531-534`), called only at
`:3624` (guarded) and at `:993,1035,1071,1088,1105,1117,1123` (all `if(boundary)`).
`boundary_active()` (`:535`) is **declared and never called** — a dead accessor.
Therefore conditional allocation is **semantics-preserving**. Verdict: safe.

`[D]` Matches my §4.1 arithmetic exactly (2.00 MiB of 4.00 MiB dead per instance;
16 MiB/block under `--parse=auto`, of which ~8 MiB used and ~4 MiB needed). Note both
of us independently concluded this is **immaterial to the MDL budget** (me ≈ 0.15-0.45%
of it) and affects **encoder RSS only, never canonical peak RSS**.

## R5. Decisive independent arithmetic: AMCP cannot reach its own KILL line

This is my principal addition. I accept their code-read multiplier of **105 complete
stream codings per block** (3 × 5 × 7, `parse_mdl:3677,3696` × `measure_parse:3604-3606`
× `encode_stream:1548`, with the 7 codings read by them at `:1554-1568` — **accepted as
code-read by them; I did not independently re-read that body**). Against their measured
5,849 cyc/B for MDL:

`[P]` 105 codings over streams dominated by a ~block-size literal stream ⇒ **~105
byte-codings per input byte**. At **8-20 cyc per byte-coding** (raw memcpy ≈ 1-2,
rANS-4096 ≈ 5-15, Huffman ≈ 10-20, averaged over the 7-coding suite) the measurement
phase is **M ≈ 15-35% of MDL's budget**.

Decomposing `M + R + W = 1` with `W` (find/walk/alloc) ≈ 0.10 and `R` (DP relaxations,
~120/byte × 2 passes) ≈ 0.55-0.75:

| programme | new budget | implied encode | vs their Gate 2 KILL line (2.137) | vs escape (59.722) |
|---|---:|---:|---|---|
| **C1 alone** (delete the 105 codings) | ×1.18-1.54 | **0.85-1.10 MB/s** | **4.7-5.0× below KILL** | 54-70× short |
| **C1 + C2** (also cut K·L 60→20) | ×1.94-2.20 | **1.39-1.58 MB/s** | **1.35-1.54× below KILL** | 38-43× short |
| **C1+C2+C3** (C3 is byte-neutral) | same as C1+C2 | **1.39-1.58 MB/s** | **still below KILL** | 38-43× short |

`[D]` **The entire AMCP programme, at its theoretical maximum with all three components
and zero implementation loss, lands at ~1.4-1.6 MB/s — below its own Gate-2 KILL
threshold of 2.137 MB/s.** And their §10.1 names C1 as "the most likely single cause of
total pilot failure"; if C1 fails, the residual is C2+C3 alone ≈ 1.0-1.3 MB/s, deeper
under the line.

`[H]` **Therefore the measurement is not needed to decide this. The arithmetic decides
it in advance, which is precisely what `06 §E5` ("calculate whether … the cost objective
can actually alter a decision" — here: whether the mechanism can alter the *frontier
outcome*) demands before spending compute.** Spending GitHub Actions minutes to confirm a
sub-KILL-line result is exactly the "no ratio-only wins / don't buy dominated rows"
pattern doctrine 2 forbids.

## R6. Do F1/F2/F3 merit CI spend? **No. Zero CI.**

Direct answers to the coordinator's question:

- **Semantics-preserving?** F1 yes-in-principle with a **real two-path phase bug** the
  constructive report missed (`j += 3` at `:600-601`); F2 yes; F3 yes, independently
  verified against every guarded access.
- **Can expected speedups be bounded from existing data?** **F2: yes, and the bound is
  row-dependent — ~1.7-2.9× on greedy-class, ~1.02-1.06× on MDL (§R3). F1: no; only a
  ~1.4-2× ceiling *on its own loop*, which is a minority of a route MDL never touches
  (§R2).** The programme-level bound *is* derivable and is sub-KILL (§R5).
- **Merit CI spend, or remain infrastructure?** **Remain infrastructure. No CI.**
  Rationale, all from data already in hand: (a) F1/F2 do not touch MDL, so neither can
  move the 59.722 MB/s cap; (b) even maximal AMCP lands below its own KILL line;
  (c) F2's large win accrues to greedy-class, which is 10.07% ratio-dominated by q4, so
  it cannot produce a crossing; (d) all three are rev-1-only, and rev-1's frontier
  relevance is gated on their **U4** product decision; (e) `06 §E3` forbids counting
  "allocations removed" as a result.

**Cheapest settlement instead of CI — two checks, both zero-CI and both local-static:**
1. **Equivalence proof**: the §R2/§R3/§R4 static arguments above, plus a reviewer diff
   check specifically for the `j += 3` phase path. No build, no corpus.
2. **Their U2**: one disassembly of the Release `scan_candidate` object to settle whether
   clang-cl elides the 12 KiB value-init. Explicitly local, no corpus, no codec run.

`[D]` If rev-1 is ever promoted to a live ratio backend (U4 = YES), F1/F2/F3 become
worth ~one ordinary engineering PR each, gated on **hash identity** — exactly as the
constructive report says, and I agree: their gate is hash identity, not Pareto. But
that day is gated on a decision, and §R5 shows the decision should not be made to
rescue a parser.

## R7. Correction I owe my own §8

`[D]` **My §8 Gate-2 bar ("≥10 MB/s") was defective in exactly the way the constructive
report corrected its own interim "4.0 MB/s" bar.** Both thresholds were set without first
asking whether *any* speed escapes domination. Their §4 escape threshold — **59.722
MB/s** — is the correct one, and it is derived from the measured dominator set
(`zstd-9` 0.191552 @ 59.722 is the binding row; `brotli-q6` 55.128 is second).

Therefore: **my §8 Q2 CRSS experiment is withdrawn as specified.** At its projected
~10 MB/s, CRSS would retain MDL's 0.198808 ratio and be dominated by `zstd-9`,
`brotli-q6`, `brotli-q9` *and* `brotli-q11`. `[H]` CRSS's premise — that cost-ranked
selection repairs §3.1 (length-filtered candidates hiding the cost oracle's distance
term) and might recover parse-decision ratio at greedy-class cost — **remains a real
scientific question**, but it is now correctly classified as rev-1-lane science with no
frontier consequence, i.e. it inherits the same downgrade. It is demoted from
"infrastructure PILOT candidate" to **parked**, and it is not worth remote compute on
the current tree.

`[D]` Note the honest asymmetry: §3.1's defect is real and §2.4's regression on
`doc.md` (+18.19%) and `generated.sqlite` (+15.06%) is measured. Fixing them would make
rev-1 *less wrong on natural text*. That is a quality argument for rev-1, not a frontier
argument, and it is the strongest thing I can still say for this track's mechanism.

## R8. Disagreements adjudicated

| # | their claim | my adjudication |
|---|---|---|
| 1 | canonical route has no ANVIL parser | **Confirmed independently** by a different route; doubly settled |
| 2 | frontier pilot withdrawn; track = infrastructure | **Confirmed**; I additionally show the arithmetic forecloses the frontier claim itself, not just this mechanism |
| 3 | "two encoder fixes" (§12 lists three) | **Minor integrity defect**; all three audited, F3 verified safe |
| 4 | F1 is a provable-equivalence fix | **Qualified**: equivalent *in principle*, but the report's one-counter formulation is **incorrect** — it must mirror the `j += 3` TCOPY skip at `:600-601`, and the failure mode is a silent density change |
| 5 | 105 codings/block; 3×5×7 | **Accepted as code-read by them**; I flag that I did not re-read `encode_stream`'s body and derive the *budget share* (M ≈ 15-35%) myself |
| 6 | the 12 KiB `scan_candidate` init is probably elided | **Agreed, and it strengthens my §4.1 rejection of the memory hypothesis**; no claim rests on it either way |
| 7 | SIMD `match_length` = adopt-class, not offered | **Agreed** and recorded in my §4.2 as the one untested cycle-level lever left; still not frontier-reachable |
| 8 | MDL bytes beat q4 (0.198808 vs 0.203129) | **Contested but immaterial.** True as a ratio statement; my §2.1 adds that q11 dominates MDL on *all three axes* (−26.7% bytes, −20.2% encode time, −3.30× decode) and §2.4 adds the win is on a `06 §C1` stress-only family while MDL is +18.19%/+15.06% on markdown/SQLite. Their own `HOLD` already concedes the point |
| 9 | §6.3 "16 MiB where ~8 used, ~4 needed" | **Confirmed** verbatim from my independent §4.1; both conclude immaterial to MDL and encoder-RSS-only |

## R9. FINAL RULING (unchanged, and now hardened)

# KILL — track 17 as chartered: "parser/candidate-generation algorithms for measured-cost MDL"

**Unchanged from my interim, and now hardened by the pair reconciliation:**

1. **Structural, doubly settled.** The canonical frontier arm runs zero ANVIL parser
   lines; 99.9997% of the canonical Silesia result (46,446,995 B vs a 46,446,836 B
   two-codec oracle) is a routing choice, residual **159 B**.
2. **Arithmetic cap, now independently derived from both sides.** Non-domination for any
   byte-fixed rev-1 parser requires **≥59.722 MB/s** aggregate encode; maximal AMCP
   yields **~1.4-1.6 MB/s**, i.e. **below its own 2.137 MB/s KILL line** and 38-43×
   short. No measurement is required to decide this.
3. **Measured domination.** `anvil-mdl-rans` is beaten by `brotli-q11` on bytes, encode
   *and* decode simultaneously.
4. **Mechanism-class unrepayability.** The 3.64× decode deficit is unreachable by any
   encoder-side mechanism.
5. **Provenance.** 13-file local discovery corpus, in-sample hyperparameter sweeps, no
   paired CIs / CV / A-A / affinity, and **no held-out corpus exists**.

**Residual disposition — explicitly NOT a pilot, NOT CI-funded:**

| item | class | action |
|---|---|---|
| F1 modulo → phase counter | adopt-class engineering | **park**; land only if U4=YES; must mirror `j += 3` (`:600-601`) |
| F2 vector → fixed array | adopt-class engineering | **park**; ~1.02-1.06× on MDL, ~1.7-2.9× on greedy-class, neither frontier-relevant |
| F3 conditional boundary arrays | adopt-class engineering | **park**; verified safe; encoder-RSS only |
| AMCP C1/C2/C3 | adopt-class, sub-KILL by arithmetic | **KILL as a mechanism claim**; do not prototype |
| CRSS (§7.2 mine) | hypothesis, rev-1-lane | **park**; my §8 gate withdrawn as specified (§R7) |
| admission-gated index | real mechanism, right asymptotics | **route to rev-1 lane**; frontier-relevant only on U4=YES |

**Final answer to the coordinator's question:** the two/three encoder fixes are
semantics-preserving (F1 with one correction), their speedups are bounded for F2 and
unbounded-but-small for F1, and **none of them merits a single minute of CI.** They
remain infrastructure, gated on a product decision (U4) that §R5 shows should not be
made in order to rescue a parser.

**One cheapest decisive check, zero CI, no production edit, no corpus run:** the static
equivalence audit of §R2-§R4 — specifically, whether the F1 phase counter mirrors the
`j += 3` TCOPY skip — plus their U2 disassembly. Both are local-static and settle the
only two open questions that are still cheap. **No remote gate is warranted for any
rev-1 parser mechanism.**