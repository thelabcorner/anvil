# BWT postcoder — measured stage profile + acceleration spec (decode-perf, I9 leg 3)

Status: **COMPLETE (ranking-grade).** Every number is measured on the windows
below; caps are arithmetic on measured medians (labelled). No crossing claims.
Task: `task_7e688712c7284c4fb3783c19d7bda15f`.

## 0. Provenance / method

- Containers: **canonical `build\anvil.exe` sha256 `0D1E130B…`** (src `62BC6631…`),
  `--parse=ratio --ratio-backend=bwt --ratio-context=off --ratio-lines=off
  --bwt-post={1,2,3}`, every one round-trip SHA-256 verified.
- Instrumented decoder: lane `post_prof_wt.exe` sha256 `8DD0B8B8…` (`/O2`, same
  src; replica of `bwt_arith_decode` verified **byte-identical** to the shipped
  function on every container — hard check, not a claim).
- Protocol: median-7 **interleaved**, pinned to LP23, HIGH priority, threads = 1,
  1 Hz total+own CPU sampler. Windows: dickens+webster 08:40:50–08:45:21Z (load
  median 19%, max 74%; own CPU 98.4% of one core); enwik8 08:46:12–08:56:16Z (load
  median 18%, max 100%; own 99.0%). `attest/i9_post_*.txt` + `_cpuload.csv`.
- Canonical CLI cross-check (id2): dickens 9.49 MB/s, webster 10.09, enwik8 9.85
  (whole-container, median-7, same windows).
- Harness note (reproducibility): the first harness build had a genuine
  stack-use-after-scope bug (variant lambdas captured block-local `o1`/`dummy`/
  raws by reference); ASan located it, fixed in `post_prof.cpp`. A report-label
  index bug (fast/model-build variants inserted before setup) was also fixed; the
  published numbers below were recomputed from the recorded per-variant values.

## 1. Measured stage decomposition — arith postcoder (id2 unless noted)

`token+uvar` = token symbol decode (arithmetic + Fenwick) + run-varint decode,
no MTF/no writes; `MTF` = sym-list update; `output` = materialization writes;
`model-init` = eager + lazy `AdaptiveModel` construction.

| file | post | e2e base ms | postcoder ms | **postcoder MB/s** | token+uvar | MTF | output | model-init |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| dickens | 2 | 775.88 | 404.07 | **25.22** | 92.5% | 4.9% | 2.6% | 0.40 ms |
| dickens | 1 | 789.11 | 400.88 | **25.43** | 92.1% | 4.2% | 3.7% | 0.32 ms |
| webster | 2 | 3177.08 | 1155.75 | **35.87** | 92.2% | 4.5% | 3.3% | 0.43 ms |
| webster | 1 | 3178.42 | 1141.11 | **36.33** | 90.8% | 4.7% | 4.5% | 0.30 ms |
| enwik8 | 2 | 9313.23 | 3520.45 | **28.41** | 91.4% | 5.9% | 2.7% | 0.44 ms |
| enwik8 | 1 | 9176.14 | 3454.38 | **28.95** | 91.5% | 4.2% | 4.3% | 0.47 ms |

- The postcoder is **52.1% / 36.4% / 37.8%** of whole-container decode
  (dickens / webster / enwik8) — it is the binding floor once unbwt is hidden.
- **The adaptive arithmetic + Fenwick token/run decode is 91–93% of the
  postcoder.** MTF (4–6%) and output materialization (3–4%) are small;
  model construction is ~0.4 ms total (0.03%). ID2 uses 121–224 lazy O1 models —
  irrelevant to time.
- Rank histogram (dickens id2, 5.58M tokens): rank1 1.49M, rank2 0.72M,
  rank3 0.46M, rank4 0.33M → 54% of tokens have rank ≤4, but MTF is only 4.9% of
  the postcoder, so a small-rank fast path cannot matter.

## 2. Candidate caps vs the 40 MB/s bar (bench anti-tie v2)

`C = 1/(1−s)` on the e2e baseline; `R' = 1.02 × (40 / postcoder_MB/s)` (the
06 §F0.1 40 MB/s BWT-only reopen bar assuming a free/aux-hidden inverse).
`DECODE-GO` iff `C ≥ R'`; else `DECODE-SHORT`.
**Free-unbwt view only:** see "Requirement CORRECTION" below for the stage-correct
requirement (`k_p` 3.9–6.6×) that governs; this table's `R'` values must not be
cited as the requirement without that section.

| file | C: buffered-renorm (measured prototype) | C: MTF (cap) | C: output (cap) | C: all token time (cap) | R' | verdict |
|---|---:|---:|---:|---:|---:|---|
| dickens id2 | **1.021** | 1.026 | 1.014 | 1.93 | 1.617 | **DECODE-SHORT** |
| webster id2 | **1.014** | 1.020 | 1.012 | 1.50 | 1.137 | **DECODE-SHORT** |
| enwik8 id2 | **1.013** | 1.020 | 1.010 | 1.53 | 1.436 | **DECODE-SHORT** |

Required **token-decode speedup** if only that component shrinks, to hit 40 MB/s
postcoder-only: **dickens 1.66×, webster 1.13×, enwik8 1.46×** (portfolio ≈1.38×).
The measured buffered-renorm prototype delivers only **1.04×** → the cheap
wire-invisible route is **FALSIFIED as sufficient**.

## 3. Postcoder-ID trade (id1 / id2 / id3)

| file | id1 bytes | id2 bytes | id3 bytes | id3 postcoder ms (vs id2) | id3 byte cost |
|---|---:|---:|---:|---:|---:|
| dickens | 2,643,577 | **2,571,873** | 3,901,580 | 60.88 ms (6.6×) | +51.7% |
| webster | 7,557,402 | **7,317,361** | 14,711,551 | 227.31 ms (5.1×) | +101.0% |
| enwik8 | 24,181,513 | **23,534,368** | 44,100,996 | 587.65 ms (6.0×) | +87.4% |

id1 (order-0) is 2.7–3.3% larger than id2 at the same token cost → **id2 is the
correct default**. id3 (stream-suite raw, codec 6 = ctx-rANS) is 5–6.6× faster on
the postcoder but costs +52–101% bytes — **not a viable default**; usable only as
an explicit byte-budget selection (the encoder already constructs it).

## 4. Spec for arch — the one leg that can clear the bar

**Target: the adaptive arithmetic + Fenwick token/run decode inside
`bwt_arith_decode`.** Required: ≥1.4× aggregate (1.66× dickens) decoder-side,
**wire-identical**.

Callsites (lane-copy line refs; same shape in `src/anvil.cpp` at `62BC6631`):

| function | role | line |
|---|---|---:|
| `bwt_arith_decode` | token/run loop + MTF expand | 3910–3934 |
| `AdaptiveModel::decode` / `update` | Fenwick search + rescale | 154–199 |
| `ArithmeticDecoder::consume` / `BitReader::bit` | range update + renorm | 119–152 / 67–78 |
| `bwt_mtf_emit_rank` | MTF list update | 3867–3873 |

Cheapest wire-invisible route (byte-identity by construction — the model's
frequencies and the arithmetic intervals are **unchanged**, so the bitstream and
symbols are identical):
1. **Two-level cumulative table** replacing the 8-step Fenwick search
   (block prefix sums of 16 + 16-wide scan): removes ~7 dependent memory touches
   per symbol.
2. **Buffered-bit renorm** (measured here at 1.04×) folded into the same pass.
3. Keep `update()`/rescale semantics identical (or rebuild the block table on
   rescale); a change to *model values* would alter the wire and is out of scope.

Measured ceiling for (1)+(2) is not yet established; the arithmetic says the
combination must reach 1.66×/1.13×/1.46× (dickens/webster/enwik8) or ~1.38×
aggregate. If a prototype cannot reach ~1.4× aggregate, the remaining route is a
new postcoder ID with transmitted static tables (format change, format lane) and
a pre-registered byte budget — **not** a wire-invisible tweak.

Byte-identity gate for any landing: the instrumented replica here already asserts
equality with the shipped decoder per container; `tests/fuzz.py`
(`forced_postcoder_roundtrip`, `golden_bwt`) must stay green, and any citation
needs a bench-attested window (contract v1.2). ID3 remains selection-only.

## 5. Falsification on record

A purely decoder-side, wire-invisible acceleration of the **current** adaptive
arithmetic+Fenwick postcoder (all cheap levers: buffered renorm 1.04×, MTF fast
path ≤1.03×, output materialization ≤1.02×, model-init ≈1.00×) **does not reach
the 40 MB/s BWT-only reopen bar** on any tested file (dickens/enwik8 need
1.46–1.66×; webster 1.13×). The postcoder must be attacked at the symbol-decode
algorithm itself (two-level tables / a different coder), not at its edges.


---

## Addendum (after arch's materialization A/B)

arch's paired A/B falsifies the eager-materialization leg as removable (lazy
`StreamPull` is 1.4% slower on generated.json mode-10 and 27.6% slower on
synth-timeseries hotop-rlzp). Consequence for this spec: on the ratio-cell
decoders, the **only** remaining wire-invisible levers are the token arithmetic
decode (this spec) and alloc/concat (derived caps 1.05–1.39×, below their bars) —
so leg-4's token-decode target is the last live decode program, and the §5
falsification of the cheap-edge route is reinforced. If a two-level-table +
buffered-renorm prototype cannot reach ~1.4× aggregate, the fallback (new
postcoder ID with transmitted static tables) is the only remaining route.
`[arch A/B: measured; caps: derived]`


---

## Outcome (leg-4 landed, 2026-09-12) — spec target FALSIFIED by implementation

arch implemented exactly this spec (two-level cumulative tables + buffered renorm
inside `bwt_arith_decode`, model values unchanged) on lane exe `FE0CF4F1` (frozen
src `BDC90474…`; canonical `build\anvil.exe 72D65150…`, superseding `E8AA2E48`)
and gated it (roundtrip 442/442, fuzz `--cases 50` PASS incl. deterministic_bwt + golden_bwt,
targeted encode-wire identity SAME). Attested paired A/B (interleaved, reps=7,
core18, 1t, wire-identical):

| file | before | after | speedup |
|---|---:|---:|---:|
| dickens | 89.121 ns/outB | 73.353 ns/outB | **1.215×** |
| webster | 74.980 ns/outB | 64.135 ns/outB | **1.169×** |
| **bytes-weighted aggregate** | — | — | **1.179×** |

So the two named levers are real but bounded, and the **~1.4× aggregate target is
FALSIFIED** (1.179× < 1.38× required; 1.215× < 1.66× dickens; 1.169× ≈ 1.13×
webster — only webster would be at the bar). Consistent with §5: the residual
work is inside the per-symbol range-coder arithmetic + model maintenance, not in
the search/renorm edges this spec bounded.

**Terminal decode picture for I9:** CRC spent (A3/A8), materialization falsified
(A9), ALLOC-only retained but insufficient (1.352×/1.074×), postcoder leg-4
retained wire-identical but insufficient (1.179× aggregate). **All authorized
wire-invisible decode routes are now closed.** The only remaining route is a NEW
postcoder ID carrying transmitted static tables (format change; needs format +
research-gate sign-off and a pre-registered byte budget) — not authorized here.
Labels: arch's A/B = measured/attested (their window); restatement of targets =
derived. No crossing claims.


---

## Requirement CORRECTION (bwtinv cross-leg arithmetic, derived) — k_p is 3.9–6.6×, not 1.13–1.66×

My §2/§4 requirement numbers assumed a **free (hidden) unbwt**. The aux unbwt is
real but 3.4–3.5×, so the postcoder stage must be faster by much more. With the
shares from `i9_bwt_wt_v2` (62BC6631: dickens s_p=0.439, s_u=0.502; webster
s_p=0.342, s_u=0.610; enwik8 s_p=0.378, s_u≈0.582 est.) and k_u = 3.40/3.49:

`k_p = s_p / (R_base/40 − s_u/k_u − s_other)`

| file | R_base MB/s | **k_p required for 40 MB/s BWT-only** | stage k_p implied by arch's leg-4 whole-decode A/B | aux+leg4 end-to-end | resulting BWT-only |
|---|---:|---:|---:|---:|---:|
| dickens | 11.15 | **6.05×** | 1.674× | 2.134× | 23.80 MB/s |
| webster | 12.46 | **3.86×** | 1.733× | 2.379× | 29.65 MB/s |
| enwik8 (est) | 10.74 | **6.60×** (est, s_u assumed) | 1.671× | 2.286× | 24.55 MB/s |

Bytes-weighted portfolio with aux+leg4 ≈ **25.7 MB/s** (vs 40 target). So the
arithmetic+Fenwick symbol decode would need **~4–7×**, not ~1.4× — the leg-4
falsification is stronger than §5 stated, and the static-table postcoder ID is
the only apparent route that could provide it. `[derived: bwtinv formula, my
splits; whole-decode A/B from arch]`. A direct stage-level A/B (postcoder-only
before/after leg-4) would remove the whole-vs-stage conversion; not measured here.
