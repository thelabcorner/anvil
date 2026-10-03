# Track 17 — Parser / candidate-generation algorithms — Space Bunny Free

**Status: FINAL.** Convergent with `17-parser-algorithms-fledge.md` (read after
my interim; the critic audited without it, and I have adjudicated their §10 slot).
Agent role: constructive inventor. Date: 2026-10-02.
Worktree: `i10-aux-unbwt` @ `b8eae11`, intentionally dirty.
No production file modified. No commit, push, reset, clean, stash, restore,
rebase, local corpus run, local benchmark, or fuzz campaign.

---

## 0. VERDICT

# KILL — parser/candidate-generation work as a current frontier route.

Retained as **clearly scoped infrastructure / parser R&D**, each bound to a named
**non-ratio** production mode:

| retained item | touches | disposition |
|---|---|---|
| **CRSS** — cost-ranked single-pass selection | rev-1 mode 10 (`--parse=mdl`), and the rev-1 `auto` router | **PILOT candidate**, one remote run |
| **F1** — `j % dist` modulo in scan inner loop | rev-1 modes 11/12/13/14/15 | cheap, gated on hash identity |
| **F2** — by-value `std::vector<Match>` per byte | rev-1 modes 10/11/12 | cheap, gated on hash identity |
| F3 — dead `bhead_`/`bprev_` when `--boundary=off` | rev-1 all | **DEMOTED — immaterial, see §7.3** |

**None of these is a frontier mechanism, and none is claimed to be.** No
Silesia/enwik8 impact is claimed anywhere in this document, because **there is no
integration point into `encode_ratio_block`** — see §2. No local `generated.*`
number is used as frontier evidence. No rev-1 discovery number is spliced with a
W-I10-REMOTE number.

---

## 1. Evidence map (W-LOCAL-2026-08-21 only)

| Window | Artifact | Stamp | Use |
|---|---|---|---|
| **W-LOCAL-2026-08-21** | `tests/benchmark-suite.csv` (28,528 B), `tests/benchmark-summary.csv` (1,245 B) | both `2026-08-21 15:29:05` | every byte/encode/decode figure below, and **only** within that one grid |
| **W-I10-REMOTE** | runs `35952830106` / `35985412906` / `36011333908` / `36061511123`; `docs/I10-AUX-UNBWT-RESULTS.md` | 2026-09-24 | **route identification and rulings only**, cited as ledger text |
| W-HOST | `tests/host-spec.md` | `2026-08-12T23:06:56Z` | Ryzen 9 5900X, base 4.20 GHz, clang-cl 22.1.8 |

Total input across the 13-file grid: **12,272,327 B** (recomputed from the CSV).
`06 §E2` records cross-machine comparison as a live confound; no window is mixed.

### 1.1 Per-file `anvil-mdl-rans` vs `brotli-q4` (both rows from the same CSV)

| file | MDL | q4 | Δ |
|---|---:|---:|---:|
| `generated.log` | 132,434 | 221,613 | **−40.24%** |
| `generated.json` | 89,589 | 137,575 | **−34.88%** |
| `generated.jsonl` | 183,506 | 227,945 | **−19.50%** |
| `synth-jitter.bin` | 87,412 | 93,189 | −6.20% |
| `anvil_bench.exe` | 830,331 | 834,690 | −0.52% |
| `random.bin` | 262,166 | 262,149 | +0.01% |
| `src.cpp` | 9,491 | 9,286 | +2.21% |
| `anvil.exe` | 107,468 | 104,655 | +2.69% |
| `synth-timeseries.bin` | 155,739 | 148,979 | +4.54% |
| `generated.sqlite` | 323,014 | 280,741 | **+15.06%** |
| `doc.md` | 1,514 | 1,281 | **+18.19%** |
| `synth-arith.bin` | 256,022 | 170,573 | **+50.10%** |
| `generated.repeat.jsonl` | 1,148 | 190 | +504.21% |

**MDL's large wins are confined to the `generated.*` family**, which `06 §C1`
disqualifies as generalization evidence (*"Large self-test win, but canonical
text/corpus evidence does not show general transfer. Keep as a stress test"*)
and `06 §C2` reinforces for context modelling. On **natural text and SQLite —
data a general-purpose codec must handle — MDL is 8–18% WORSE than brotli q4**,
and 50% worse on `synth-arith.bin`. This is an *active regression against the
reference*, not a slow win. **This is the finding I did not report in my interim
and it is the single strongest disconfirmation of the charter.**

### 1.2 Aggregate domination, corrected

`tests/benchmark-summary.csv`, all from one window:

| codec | ratio | enc MB/s | dec MB/s | dominates MDL on |
|---|---:|---:|---:|---|
| `anvil-mdl-rans` | 0.198808 | 0.718 | 136.550 | — |
| **`brotli-q6`** | **0.176318** | **55.128** | **515.585** | **all three axes** |
| `brotli-q9` | 0.173063 | 19.269 | 506.355 | all three |
| `brotli-q11` | 0.145713 | 0.573 | 450.937 | bytes + decode only |
| `zstd-9` | 0.191552 | 59.722 | 1178.502 | all three |
| `zstd-19` | 0.161437 | 2.137 | 1203.712 | all three |
| `brotli-q4` | 0.203129 | 101.037 | 497.077 | none (ratio is worse) |

**`brotli-q6` beats `anvil-mdl-rans` simultaneously on bytes (0.176318 vs
0.198808), on encode (55.128 vs 0.718 MB/s = 76.8x faster) and on decode
(515.585 vs 136.550 = 3.78x).** That is total domination by one existing point,
and it is the kill.

**Non-domination threshold.** With MDL's ratio held fixed, escaping every
dominator requires beating the encode of the largest: `zstd-9` at 59.722 MB/s =
**83.2x the measured 0.718 MB/s** (derived). No constant-factor parser work
reaches it.

---

## 2. Why there is no integration point (the structural kill)

### 2.1 The canonical arm runs no parser

`tools/reference_cost_gate.py:136-138` and
`docs/I10-DENSE-FRONTIER-PREREG.md:110-111` both specify
`--parse=ratio --ratio-backend=auto --ratio-context=off --ratio-lines=off`.

`compress()` branches at `src/anvil.cpp:4675-4682`; the ratio path calls
`encode_ratio_block` and `continue`s. `encode_ratio_block`
(`src/anvil.cpp:4614-4639`) enumerates identity / `ratio_transform_ctx1` /
`ratio_transform_lines` over `{brotli, bwt}` and keeps the smallest payload.
**With the frontier's own `--ratio-context=off --ratio-lines=off`, only
`consider(0,d)` survives: the canonical arm is exactly min(Brotli, BWT) over the
identity transform.**

Grep over the whole file: every `parse_greedy` / `parse_dp` / `parse_mdl` /
`parse_sparse` / `MatchFinder` / `find_sparse` / `find_pnra` call site lies at
`src/anvil.cpp:4713-4783`, strictly after the `continue` at `:4681`. **No parser
symbol appears inside `encode_ratio_block`, `ratio_wrap`,
`ratio_backend_encode`, or `bwt_backend_encode`.** The BWT backend — ANVIL's own
frontier code — has no match finder and no candidate generation.

### 2.2 The size of the win confirms it is routing, not parsing

`docs/I10-AUX-UNBWT-RESULTS.md:368` Silesia legacy total **46,446,995 B**;
`06 §F4` records `oracle_min(Brotli,BWT)` = **46,446,836 B**; `06 §G2` records the
frozen result as *"+159 B over oracle"*. **Derived: 99.9997% of the canonical
Silesia result is the two-codec routing choice; the entire residual attributable
to anything ANVIL does is 159 B = 0.00034%.** Track 17 has zero leverage on it.

### 2.3 Three integration points, all fatal

| # | point | location | verdict |
|---|---|---|---|
| I1 | third `ratio_backend_ids` entry | `src/anvil.cpp:4400-4428` | `decode_ratio_block` hard-rejects any backend outside {1,2} at `:4645`. Adding one is a **wire-format change** owned by the `format` lane, and lands **adopt-class** by the DEFLATE precedent (`RESEARCH_LEDGER.md:5001-5033`, A2). Not a parser change. |
| I2 | 4th `consider()` transform | `src/anvil.cpp:4635-4637` | transform 3 permanently reserved for the LZP negative, hard-throw at `:4634`/`:4652`; `06 §G2` LZP = +279,419 B (+0.37%), 4/13, none of the headline winners. |
| I3 | parse/postcode layer in `bwt_backend_encode` | `src/anvil.cpp:4589` | that slot is where QLFC died (**+2,437,384 B, +3.23%, 0/13**) and LZP died; `06 §F2`: *"Postcoder work did not and will not move bytes."* |

The one frontier-reachable bounded-candidate-set problem — the
`(transform x backend)` choice at `:4616-4629` — is **already owned** by the
promoted G5A bounded finalist planner (`RESEARCH_LEDGER.md:4810-4813`); G4 is
closed `NO-GO-G4`. It is not a track-17 mechanism.

---

## 3. Convergence with the critic, and my three corrections

The critic audited my interim independently and reached the same structural
conclusion. **I accept their kill in full.** Their four grounds are all sound,
and grounds 3 and 4 I had under-developed.

### 3.1 I ACCEPT — new mechanism defects they found that I missed

These strengthen the kill and are **code-read facts**, not measurements:

1. **Candidate truncation is length-only; distance never enters the objective.**
   `src/anvil.cpp:517-520` and `:550-553` sort by `len` descending and
   `resize(8)`, so distance survives only as a tiebreak among equal lengths —
   while the code's own comments at `:538-539` ("aligned sources win via the cost
   model's near-distance preference") and `:3623` ("near distances dominate
   measured ds cost") assert the opposite. **The MDL objective is charged
   `varint_cost_ms(m.dist-1, c.ds)` at `:3647` over a candidate set that was
   already length-filtered.** This is a correctness defect in the chartered
   mechanism and it is the strongest justification for CRSS (§5).
2. **The empirical-lag cost model is a ratchet, not an iteration.**
   `src/anvil.cpp:3584` prices any symbol absent from the previous pass at
   **20 bits**; `MdlCosts` is measured from the previous pass and the seed is
   `parse_greedy` (`:3676`). The model can therefore only ever *shrink* the
   greedy symbol support, never discover new symbol usage. Observable: **4 of 13
   files are byte-identical to greedy under MDL**, one of them at 4.86x the encode
   cost for zero bytes. (This is a sharper statement of the `repeat.jsonl`
   pathology I flagged at interim §4.2.)
3. **The iteration is non-monotone.** `:3697` retains `best` only on improvement,
   but `:3700` adopts `c = cm` **unconditionally**, including from a *rejected*
   pass, so the loop converges toward a discarded pass's statistics while the
   retained parse is older; the `t2+1>=best_total` exit at `:3699` then compares a
   new pass against the retained best. Deterministic, reproducible, not monotone.
4. **`bseed`/`mark_starts` is computed every iteration and discarded by default.**
   `:3682-3698` writes a ±8 B dilation, consumed only when `boundary` is true
   (`:3624`), and `opt.boundary` defaults **false** (`:3777`, annotated
   "measured neutral on corpus"). `06 §D2` is **already closed** on boundary work.
5. **`opt.parse` defaults to `"auto"`** (`:3772`), which runs 8 parse/representation
   families and up to 24 full-block encodes per block — and **there is no
   `anvil-auto` row anywhere in `tests/benchmark-suite.csv`.** The shipped CLI
   default configuration is **unmeasured in this repo**, and `docs/CONTEXT.md`
   itself calls `parse=auto` *"research-only, never the production default."*
   **Documented-vs-code contradiction; every parser row here is a non-default
   configuration.**

### 3.2 I CORRECT — the critic's three-axis domination claim is inverted

Fledge §2.1 asserts `brotli-q11` beats MDL on *"bytes (−26.7%), encode (−20.2%
time), and decode (−3.30x throughput) simultaneously."*

**The encode term is wrong.** Aggregate encode is 0.573 (q11) vs 0.718 (MDL)
MB/s; **lower MB/s is slower**, so q11 takes `0.718/0.573 = 1.25x MORE` time
than MDL. The ratio 0.573/0.718 = 0.798 that Fledge quotes is a ratio of
*throughputs*, not of times. **q11 therefore does not dominate MDL on encode.**

**The conclusion survives, and survives more strongly**, via `brotli-q6`:
0.176318 vs 0.198808 bytes, **55.128 vs 0.718 MB/s = 76.8x faster encode**, and
515.585 vs 136.550 = 3.78x decode. One existing reference point dominates MDL on
all three axes. The kill is intact; only the named dominator changes.

### 3.3 I CORRECT — the throwaway-encode count

Fledge §3.5 states *"up to **4 calls x 5 streams = 20** throwaway stream encodes
per block."* `parse_mdl` performs **3** `measure_parse` calls, not 4: the seed at
`:3677` plus two loop iterations at `:3696`, because `iters=3` gives
`for it=1; it<iters` = `{1,2}` (`:3694`), and the caller passes `3` explicitly at
`:4716`. So it is **3 x 5 = 15 `encode_stream` calls**, each internally emitting
up to 7 codings (`src/anvil.cpp:1554-1568`) => **up to 105 stream codings per
block**. My interim figure stands; Fledge's undercounts by 5.

Either way the structural point is unchanged: `measure_parse` runs the entropy
encoder on all five streams to compute an objective and **discards the bytes**
(`:3604-3606` returns only the cost struct and the size), so the parser spends
~3 full-block entropy encodes *deciding which parse to use* before the chosen
parse is encoded at all.

### 3.4 I CONCEDE — my memory hypothesis is immaterial

Fledge §4.1 falsifies my F3 lead arithmetically and correctly. `MatchFinder`
zero-fills 4 arrays (`:524-526`) and `parse_mdl` constructs 4 finders per block
(`:3676` + up to 3x `:3623`) => **16 MiB zero-filled per 256 KiB block = 64 B of
memset per input byte**. At 10-30 GB/s that is **2-6 ns/B = 0.15-0.45% of MDL's
1.393 us/B budget**. The waste is real and is a clean 2x when boundary is off,
but it is **immaterial** and must not be presented as a bottleneck. **F3 is
demoted to a footnote, not a dispatch item.** Recording this so the next reviewer
does not repeat my error.

### 3.5 Points where we agree and I defer

Fledge's §2.3 (**the mechanism class is encoder-only and cannot address the
decode axis where the deficit is largest**), §2.2 (the 140.7x encode gap vs an
optimistic ~96x+4x structural bound), §4.4 (in-sample tuning of `surprise=12`,
no paired CIs / CV / A/A null / affinity / xz reference, no held-out structured
corpus per `RESEARCH_LEDGER.md:4831`), and §5 (Zopfli / LZMA `GetOptimum` /
RFC 7932 make the parsing *formulation* adopt-class) are all **stronger than my
interim treatment** and are adopted verbatim. **Fledge's §9 KILL verdict is
accepted without reservation.**

---

## 4. The charter's one true strength, stated plainly

Parser/candidate-generation mechanisms here add **zero decoder-visible bytes** —
no framing field, no model, no dictionary, no table, no new fuzz surface. That is
real, and it is the thing the charter gets right. It does not survive contact
with §1.1 and §2, but the coordinator should not read this KILL as "encoders are
wrong" — it is "**this particular encoder-side signal is not on the shipped
route, and where it is measured it regresses on natural text.**"

---

## 5. Retained mechanism: CRSS — cost-ranked single-pass selection

Scoped to **rev-1 mode 10** (`--parse=mdl`) and the rev-1 router. **Not wired into
`encode_ratio_block` under any circumstances.**

Keep **one** pass. Delete the windowed DP, the reparse loop, and
`measure_parse`'s discarded encodes (`src/anvil.cpp:3604-3606`, `:3694-3702`).
Change exactly two things:

1. **In `find()`, replace length-descending truncation** (`:517-520`, `:550-553`)
   with the argmin of the measured incremental cost
   `ttype[1] + ml_cost(len-4) + ds_cost(dist-1) - len * avg_lit` over the <=8
   candidates the finder **already returns**. This directly repairs §3.1-1: it
   gives the cost oracle the distance term it is already paying for, at O(8) per
   position, with **zero new candidate generation**.
2. **Relax 2 edges per position instead of <=64**: the literal edge plus the
   single cost-argmin match edge. This is the whole relaxation-count reduction.

*Projection, explicitly not measured:* encode >= 10 MB/s, i.e. within ~1.4x of
greedy's measured 14.151 and ~10x behind q4 instead of 140.7x. Whether it
captures any of MDL's ratio signal is the open question — and given §1.1, the
prior expectation should be that it recovers ratio on `generated.*` while
**improving** the natural-text regression, not reproducing it.

**Distinctness, argued because it is required:** `06 §D2` closed
boundary-**source** heuristics (adding candidate *sources*); `§B3` closed channel
reinforcement; `§B4` closed PNRA candidate injection; `§B5` closed RLZ/RePair
stream construction. CRSS changes the **selection criterion over candidates the
finder already returns** and **removes** work rather than adding it. It is
adjacent to `§D2` and must be argued as distinct; **if the coordinator judges it
in-distinct, treat it as closed and the track has nothing left.**

---

## 6. Decoder / resource accounting

| item | charge |
|---|---|
| new stream / token / opcode / model header / framing field / table | **none** |
| new decoder branch, decoder code size, decoder RSS, decoder cycles | **0** |
| wire format revision | **unchanged** (mode 10, `src/anvil.cpp:2158`, `:2167`) |
| effect on canonical Silesia/enwik8 bytes, decode, or peak RSS | **none** — the rev-1 parser is not on that route |
| encoder transient | 4 MiB per `MatchFinder` x 4 per block (§3.4: immaterial) |

Reporting any rev-1 encoder RSS or speed as a canonical-route result would be an
`06 §E3`-class error.

---

## 7. Complexity, asymptotics, and what is *not* achievable

Rev-1 `--parse=mdl`, per block of `n` bytes, `C = min(48,16) = 16` (`:3623`),
`L <= 16` cuts (`:3621`), `K = 4` (`:3640`), `p = 3` (`:3671`), `kWin = 16384`:

| component | complexity | cycles @0.718 MB/s = 5,849 cyc/B (derived, 4.20 GHz base) |
|---|---|---|
| cost measurement | `Theta(p*S)`, constant = 5 streams x up to 7 codings | up to 105 codings/block |
| candidate search | `Theta(p*n*C*ell)` | `mf.find` at every byte (`:3638`) |
| DP relaxations | `Theta(p*n*K*L)` | <= 64/position/pass |

CRSS reduces this to `Theta(n*(C*ell) + 2)` with **no** measurement phase —
**a constant-factor result only**. Anyone claiming a complexity-class improvement
is wrong.

**Hard limits, recorded so they are not re-argued:**

- Non-domination needs **>= 59.722 MB/s** (§1.2) = 83.2x. CRSS's ~10 MB/s
  projection lands **inside the dominated region**.
- The decode axis (3.78x behind q6) is unreachable by **any** encoder-side
  mechanism.
- MDL-vs-`parse_dp` is **4.06% bytes for 1.57x encode** — a trade inside an
  already-losing region.

The genuinely asymptotic lever in this track is **admission-gated indexing**
(search proportional to structural events, not input length; the project has
already measured ~474 MB/s candidate generation for the PNRA event-driven lane,
`docs/CONTEXT.md`). **It is a real mechanism and it inherits this track's
§2 downgrade — it is rev-1-only.** It is recorded for the coordinator, not claimed.

---

## 8. Remote-only preregistration (thresholds frozen now)

Per `MASTER-BRIEF` §5/§7. Extend the frozen dense-frontier workflow
`.github/workflows/anvil-i10-dense-frontier.yml` **unchanged** — no new timing
methodology, no new corpus. Paired protocol per
`docs/I10-DENSE-FRONTIER-PREREG.md:32-100,199-215`: 7 paired reps (9/13 allowed),
2 warmups, seed 41246, bootstrap 20,000, practical-effect epsilon 2%, max arm
robust CV 15%, ambient 5 x 250,000 with max robust CV 10%, `taskset -c 0`, one
thread. Record per arm: complete payload bytes, peak encode RSS, paired encode
and decode seconds, `strip`ed binary size, output SHA-256 per repetition. **Hash
stability across reps is a hard gate.** A/A null = `A0` vs `A0`.

**Corpus: Silesia + enwik8 only.** The local 13-file set is **excluded by
construction** — it is the discovery set on which `surprise=12` was swept
(`src/anvil.cpp:3775`, per Fledge §4.4).

| Arm | command |
|---|---|
| A0 | `--parse=greedy --entropy=rans` |
| A1 | `--parse=mdl --entropy=rans` (incumbent) |
| A2 | `--parse=crss --entropy=rans` (**isolated prototype flag; no default change**) |
| A3 | brotli q4/q6/q9/q11, zstd-9/19, xz -9e |

**Q1 is already answered by inspection and no compute is spent on it:**
`encode_ratio_block` (`src/anvil.cpp:4614-4639`) contains no parser, so no
legacy candidate-generation change can alter mode-17 bytes. Structural fact.

**Decision rule (aggregate Silesia + enwik8 bytes):**

- **KILL track 17** if **any** of:
  - A2 bytes >= brotli **q4** bytes; **or**
  - A2 recovers **< 50%** of A1's byte gain over A0 (local-equivalent
    context: 2,743,742 - 2,439,834 = 303,908 B gap, so <= 2,591,788 B locally —
    **context, not the gate**; the remote run uses its own Silesia+enwik8
    equivalents); **or**
  - A2 >= 2x A0 encode time without >= 3% aggregate byte gain; **or**
  - A2 byte-regresses vs A0 on **>= 3 of 5** timing-panel files; **or**
  - A2 encode peak RSS > 2x A0's.
- **PILOT** (one further remote run, never production) if A2 recovers >= 50% of
  A1's gain, runs >= 10 MB/s aggregate encode, and regresses on **0 of 5**
  timing-panel files **including a natural-text file** — the last clause is added
  by me specifically to target §1.1.
- **HOLD as infrastructure** if A2 recovers >= 50% of A1's gain but encode stays
  > 5x q4's. Park as enabling infrastructure, per the `CONTEXT.md`
  context-clustering precedent.
- **PROMOTE-TO-REMOTE: not available to this track as chartered.** No parser byte
  reaches the canonical route. Any promotion must be labelled **infrastructure**
  and satisfy `06 §F1`-style accounting: metadata cost <= 20% of gross savings.

**Falsification criteria I accept against myself:**

1. If A2 matches A1's bytes at >= 10 MB/s, my §1/§2/§7 kill is **wrong** and the
   track is rehabilitated as a ratio mechanism.
2. If A2 beats q4 on held-out bytes at >= 10 MB/s, the §1.1 corpus-transfer
   attack is **wrong**.
3. If an `anvil-auto` census shows the default portfolio is not ~24 encodes per
   block, Fledge §4.3 is wrong.

---

## 9. Adversarial failure modes

1. **CRSS is a retune dressed as a mechanism.** If it merely moves the
   length/dist tradeoff along the same frontier, it recovers ratio only where
   MDL already did, and the natural-text regression persists. Gate
   "regresses on 0 of 5 including natural text" is the specific test.
2. **In-distinct from `06 §D2`.** If the coordinator rules CRSS in-distinct, the
   track retains only F1/F2 — two provable-equivalence micro-fixes on a lane
   that is not the product.
3. **The rev-1 lane may be retired.** If token-LZ is never promoted to a ratio
   backend (I1), F1/F2 improve code that is never shipped. That is a product
   decision I do not own, and it is the real gate on all of §5 and §12.
4. **Trajectory contamination.** Any candidate-selection change alters
   hash-chain insertion (`:3655`) and can displace better downstream matches —
   *"un-recoverable by ANY post-parse rollback"* (S6-3 F1,
   `RESEARCH_LEDGER.md:3139-3151`). A byte regression is a **valid negative**,
   not a tuning opportunity.
5. **Discovery-set overfitting.** `surprise=12` was swept in-sample on the very
   13 files reported; `generated.{json,jsonl,log}` carry the headline wins.
   Under doctrine 6/10 **no ratio number here is admissible as validated**.
6. **The default is unmeasured.** `opt.parse="auto"` has no benchmark row, so any
   claim about "ANVIL's parser" as a product is currently unsupported.
7. **Cross-machine splicing.** W-LOCAL-2026-08-21 (Ryzen 5900X/clang-cl) vs
   W-I10-REMOTE (Linux) — `06 §E2`.

---

## 10. Final recommendation

**`KILL` — parser/candidate-generation algorithms as a current frontier route.**
Retain **CRSS** as the single scoped infrastructure PILOT candidate against
rev-1 mode 10, plus **F1** and **F2** as provable-equivalence micro-fixes on
rev-1 modes 11/12/15.

Grounds, any one sufficient:

1. **Structural (decisive).** The canonical arm is `--parse=ratio` with both
   transforms off; `encode_ratio_block` (`:4614-4639`) contains no parser. 159 B
   of 46,446,995 B — 0.00034% — is attributable to ANVIL at all (§2.2).
2. **Measured total domination.** `brotli-q6` beats `anvil-mdl-rans` on bytes,
   encode (76.8x) and decode (3.78x) simultaneously (§1.2).
3. **Regression where it matters.** MDL is **8-18% worse than brotli q4** on
   `doc.md` and `generated.sqlite` and **50% worse** on `synth-arith.bin`
   (§1.1) — on a general-purpose codec's core data.
4. **Mechanism-class unrepayability.** Non-domination needs 83.2x (§1.2); the
   decode deficit is unreachable by any encoder-side mechanism; and the chartered
   cost model is **length-filtered before the objective sees distance** (§3.1-1)
   and **cannot grow its own symbol support** (§3.1-2).
5. **Provenance insufficiency.** In-sample sweeps, no paired CIs/CV/A/A
   null/affinity, no xz, no held-out corpus, and the shipped `auto` default is
   unmeasured (§3.1-4/5).

**Explicitly NOT recommended:** any prototype wired into production; any new or
changed default; any `--parse=auto` default change; any reuse of `anvil-mdl-rans`
rows as validated ratio evidence; any boundary-candidate work (`06 §D2` closed);
any SIMD match-extension novelty claim (decided prior art — LZMA SDK / zstd /
7-Zip all ship it); any re-proposal of the mode-17 transform portfolio (route to
track 16 / 13 / 14 — not track 17's mechanism); and any claim of Silesia/enwik8
impact for a parser change.

**Hand-off:** the only unexhausted lever on the canonical route is additional
cheap whole-block reversible transforms that change which backend wins, with a
decoder-cost proof and a byte pre-filter. That contains **no parser and no
asymptotic content**; it belongs to track 16 / tracks 13-14.

---

## 11. Open uncertainties

| # | uncertainty | decisive? | settlement |
|---|---|---|---|
| U1 | Is CRSS judged in-distinct from `06 §D2`? | **yes — decides whether the track retains anything** | coordinator ruling |
| U2 | Will token-LZ ever be promoted to a ratio backend (I1)? | **yes — decides whether rev-1 work is worth shipping at all** | coordinator product decision |
| U3 | Does `surprise=12` survive on held-out families? | yes, for the CRSS ratio bar | §8 run |
| U4 | Is the 13-file MDL byte-identity-to-greedy set (4/13) reproducible? | no claim rests on it | remote A1 arm |
| U5 | Has a peer changed the rev-1 parser since `b8eae11`? | duplicate-work risk | re-read `src/anvil.cpp` at dispatch |

---

## Appendix — the micro-fixes worth dispatching regardless

Both are **provably output-identical**, gated on a **hash-identity check**, not a
Pareto argument. Both are rev-1 only and touch **no** canonical frontier number.

| # | defect | location | fix | modes |
|---|---|---|---|---|
| **F1** | `uint8_t cpy = (dist > 0) ? src[j % dist] : src[j];` — a **runtime-divisor integer modulo per scanned byte** in the overlapping-copy inner loop; `scan_candidate` is called up to `kSparseChainMax = 32` times per position | `src/anvil.cpp:592` (callers `:700-708`, `:630-650`) | phase counter `++k; if (k == dist) k = 0;`, exactly reproducing `j % dist` | 11/12/13/14/15 |
| **F2** | `walk()` returns `std::vector<Match>` **by value** and `find()` does `insert / std::sort / resize(8)` => **1-2 heap allocations plus a sort per byte position**, with `find()` running at every byte under both `parse_mdl_pass` (`:3638`) and `parse_greedy` (`:743`) | `src/anvil.cpp:500-522`, `:536-555` | fixed-capacity `std::array<Match,8>` + count | 10/11/12 |

Novelty class: **none (D) / adopt-class**. Not gated as a mechanism. F1 is the one
cycle-level lever in this track not already closed by `06 §D3` (reciprocal-rANS:
measured slower than hardware divide). *Caveat retained from Fledge §4.2: F1 is a
`[P]` projection, plausibly mitigated by the `dead_band` early exit (`:613`) and
`kSparseBlockBudget` (`:591`); the hash-identity gate is what settles it.*

**F3 (dead `bhead_`/`bprev_`, 2 MiB/block/finder when `--boundary=off`) is
demoted to immaterial** per §3.4 — real waste, 0.15-0.45% of the encode budget.
Recorded so it is not re-elevated.