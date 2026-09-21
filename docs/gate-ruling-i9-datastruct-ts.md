# GATE RULING I9-3 — datastruct-ts: independent reproduction, novelty ruling, and the dual bar

**Author:** `research-gate` (novelty gate arbiter). **Date:** 2026-09-12.
**Swarm:** `anvil-i9-pareto`. **Task:** `gate-i9`.
**Requesters:** coordinator (`msg_e10175981f8d46a89eee3908d7a6f532`), datastruct
(`msg_b7d70941e4f647cc8637a7cafc6bbde7`), strategy PR-3-Q1
(`msg_6cf17f0a5c984018ae4e33efa5329419`).
**Status: REPRODUCED (all cells byte-identical on double-run); NOVELTY: NO —
capability/engineering + placement, not a mechanism-level claim; dual-bar
finding recorded.**

---

## 1. Independent reproduction (VERIFIED)

Artifacts: `prototypes/i9-datastruct/{colref.py, analyze_ts.py,
verify_artifacts.py, out/, RESULTS.md}` (untracked worktree). One command:

```powershell
python prototypes\i9-datastruct\verify_artifacts.py
```

Run twice by this gate (two separate process invocations); output
**byte-identical**. Transcribed (all `rt=OK`):

| variant | bytes | ratio | sha256[0:16] |
|---|---:|---:|---|
| ts.raw_o0 | 207,197 | 0.7400 | 5c18df58668be296 |
| ts.raw_o1 | 187,436 | 0.6694 | 4e363fd244bb55b1 |
| ts.ref_flat | 107,564 | 0.3842 | e6595818ea138e79 |
| ts.ref_col | 97,057 | 0.3466 | 2b89ccbeb43e64bf |
| **ts.ref_field** | **89,877** | **0.3210** | **56b60452d61b8416** |
| ca.ref_col | 69,748 | 0.2527 | 8b6303613128004a |
| ca.ref_field_true | 24,055 | 0.0872 | 66b5503aba4e1b1a |
| ca.ref_field_naive | 44,837 | 0.1625 | 63afe67f0999599e |
| ca.ref_field_p184 | 35,808 | 0.1297 | 442a5d6f24f7a70a |

Classes printed for `ts.ref_field` = `0:8:prev, 8:4:prev, 12:2:posmod`
(identical to the writeup). Ablation arithmetic verified from the same table:
coder-only `207,197 -> 187,436 = -19,761`; mechanism `187,436 -> 89,877 =
-97,559`. The `ref_field` wire is 30,792 B (25.5%) below raw brotli q6
(120,669) and 50,021 B below the committed-HEAD best ANVIL row (143,132).
**Label check: the writeup says "prototype wire, not an ANVIL row", "no
crossing claim", {synthetic}, period + partition supplied. That labelling is
correct and is part of this verification.** Throughput was not measured by
datastruct and none is cited here.

## 2. Structure erratum — independently VERIFIED from bytes alone

Method (data-only; no generator constants assumed for the determination): scan
candidate periods 1..512; a period qualifies iff a *record-local invariant*
holds for 100% of the blocks. Exact command:

```powershell
python docs\gate-verify-i9-corpus.py
```

Results:
- `synth-timeseries.bin` (280,000 B): invariant `u16@12 = k mod 1000` holds at
  **P = 14 only** in 1..512 (20,000 records; 28 = 2x14 alias fails 9,990/10,000).
  True fully-constant offsets = `{4,5,6,7}` = **4/14 = 28.6%** of bytes.
- `synth-columnar-align.bin` (276,000 B): invariant
  `byte@22 = XOR(bytes 0..21)` (the generator's parity field) holds at
  **P in {23,46,69,92,115,138,184,230,276,345,368,460}`** in 1..512; **minimal
  P = 23** (12,000 rows). True constant offsets = `{5,12,13,16,17}` =
  **5/23 = 21.7%**; the I8 `56/184 = 30.4%` is an 8x-decimation artifact (e.g.
  `ts_ns` advances +8,000,000 per 8-row group, and 8,000,000 = 0 mod 256, so
  the low ts byte *looks* constant).
- Byte-exact provenance: regenerating `tests/make_synth_corpus.py` into a temp
  directory reproduces `synth-timeseries.bin` sha256 `1B7D6182…7425` and
  `synth-columnar-align.bin` sha256 `A42EA6B4…263B` byte-for-byte (read-only;
  `tests/corpus` untouched).

**Supersession:** PART XIII §8's recorded facts (P=28 / 10,000 records; P=184 /
1,500 records; offsets {4-7,18-21}; 56 offsets) are **superseded**, as are
`docs/gate-verdict-i8-ari-stride.md` §4b/§5 and strategy §2.7c. The **conclusion
that P is infrastructure, not novelty, STANDS** — xz `--delta` documents
`dist` 1..256 and both 14 and 23 fall inside it. The methodology rule is
recorded: distinct-offset counting aliases at any multiple of the true period;
minimality must be validated (invariant scan), and fractions can change under
aliasing (timeseries 28.6% survived; columnar 30.4% -> 21.7% did not).

## 3. Reference rows (VERIFIED)

- committed HEAD fc23d9a, `synth-timeseries.bin`: best ANVIL row
  `anvil-sparse-rans` = **143,132 B** (0.511); raw brotli q6/q9 = **120,669 B**
  (0.431), brotli q11 = 134,718 (0.481), xz-9e class ~117,448.
- uncommitted worktree grid `C70179EA…`: best ANVIL row
  `anvil-hotop-rlzp-rans` = **140,898 B** (0.503, encode 0.035 MB/s, label it if
  cited).
- python-brotli 1.1.0 reproduces the harness rows exactly on this file
  (q6 = 120,669; q11 = 134,718), so python-brotli is a valid control for the
  delta-transformed rows below.

## 4. THE DUAL BAR (new finding — this cell is not crossing material)

Binding rule (PART XIII §5b, generalised in I9-1 R-2 and DNB/E1): a {synthetic}
transform-cell result must be reported against **both** the raw reference score
and the **transform-enabled reference score** (same reference codec given a
reversible transform). Measured controls (raw brotli rows match the harness;
delta is the xz `--delta` byte-wise subtraction at the true period):

```powershell
# transform-enabled reference bars
xz --delta=dist=14 --lzma2=preset=9e -c tests\corpus\synth-timeseries.bin     > ts.d14.xz
xz --delta=dist=23 --lzma2=preset=9e -c tests\corpus\synth-columnar-align.bin > ca.d23.xz
python docs\gate-verify-i9-transform-bar.py    # brotli/zstd controls + zero-fraction
```

| file | bar | bytes | ratio |
|---|---|---:|---:|
| synth-timeseries | raw best ref (brotli q6/q9) | 120,669 | 0.431 | 
| synth-timeseries | xz -9e raw | 117,456 | 0.419 |
| synth-timeseries | xz -9e --delta=dist=14 | **89,564** | 0.320 |
| synth-timeseries | brotli q11 on delta14 | **80,650** | 0.288 |
| synth-timeseries | brotli q6 on delta14 | 97,296 | 0.347 |
| synth-columnar-align | raw best ref (brotli q11) | 105,915 | 0.384 |
| synth-columnar-align | xz -9e --delta=dist=23 | **21,340** | 0.077 |
| synth-columnar-align | brotli q11 on delta23 | 27,326 | 0.099 |

**Consequences.**
1. `ts.ref_field` (89,877) is **above** both transform-enabled bars: by 313 B
   vs `xz -9e --delta=dist=14` (89,564) and by 9,227 B vs `brotli-q11 + delta14`
   (80,650). Its non-dominance against the raw grid (best raw ref ratio 0.431)
   is a **raw-bar artifact**, exactly the class PART XIII §5b was written to
   strike. It is a **FRONT-GAP (dual-bar)**, not a crossing — even if natively
   integrated and landed.
2. `ca.ref_field_true` (24,055) is likewise **above** `xz -9e --delta=dist=23`
   (21,340). Same classification.
3. The honest target on these cells is the transform-enabled bar, not the raw
   one. The prototype's mechanism gain over its own controls remains real and
   useful — as a capability, not as a frontier advance.
4. This is a reference-control result; the reference codec is not being
   handicapped (E1 satisfied). The `{synthetic}` tag and the "probe, not
   generalization" caveat apply to every number above.

## 5. NOVELTY RULING — record-period reference copy with per-column residual contexts

**RULING: NO mechanism-level novelty.** Classification:
**engineering/capability + placement (inside-the-reference)**. Grounds, against
each claimed element:

1. **Copy at distance = record period** is an LZ match at a distance that
   happens to equal a framing constant. Copy-with-edits is VCDIFF/bsdiff/Zdelta
   territory; self-reference is VCDIFF (RFC 3284); ANVIL's own SPARSE-REF is
   this shape (audit §3).
2. **Per-column residual contexts conditioned on the reference byte** are a
   columnar/delta predictor plus context modeling: xz `--delta` (fixed-distance
   subtraction, `dist` 1..256 — covers both measured periods), Gorilla/FPC
   (delta-of-delta on a column), Parquet RLE/bit-packing, Brotli RFC 7932 §7
   (a context map driving per-context literal models — context conditioning is
   established, as this project already conceded for the K≈8–12 quantizer).
3. **The period P is transmitted and is a framing constant.** PART XIII §8
   stands: it is G3-adjacent prior art realisable by the published filter; its
   **auto-detection is infrastructure** (same class as the G6 detector, the
   Gorilla-Pv stack, the context quantizer). Not gated, not claimed.
4. **Field classes (`const`/`posmod`/`prev`/`delta`)** are columnar codecs'
   stock-in-trade (dictionary/RLE/delta per column). `posmod` exactness makes
   them *correct*, not *new*.
5. **Placement INSIDE the reference** is real and useful — it preserves the
   row-interleaved layout (the lane-transpose negative) — but per verdict G1 it
   separates only from global filters and lands in copy-with-edits, where it is
   not a separator. It is a **placement constraint satisfied**, not a novel
   interaction.
6. **The one live novelty family nearby is correction TOPOLOGY coding**
   (SPARSE-REF element 2, the audit's strongest surviving element). This
   mechanism does **not** do it: it codes every column with a per-column model;
   there is no correction-position/topology stream. It therefore neither
   re-opens nor retires the R2 target (DNB-B1 stays as recorded).

**The unsolved gap, explicit:** auto-discovery of the period **and** the field
partition — inside the reference, no global transpose — is genuinely unsolved
(ca: true partition 24,055 vs naive 4-B words 44,837, +86%). It is an
**engineering** gap; solving it would be infrastructure-class capability unless
paired with a separately ablatable mechanism claim (e.g. topology coding). No
frontier claim may be built on it without that.

**Prior-art lineage for any future pre-registration:** xz `--delta` (dist
1..256); VCDIFF RFC 3284 (self-reference + ADD/RUN); bsdiff/Zdelta/Zucchini
(copy-with-edits); Gorilla/FPC (delta-of-delta); Parquet/ORC columnar encodings;
Brotli RFC 7932 §7 (context map); ANVIL SPARSE-REF/R2 record (audit §3,
DNB-B1). Cite once each; FTO not obtained.

## 6. Strategy PR-3-Q1 — answers (framing question, pre-registration)

**(1) Would a landed row at ~89,877 B on synth-timeseries be FRONT-CROSSING?**
Mechanically on the **raw** grid: yes — no raw reference row has ratio <= 0.321,
so the row is non-dominated with no bracket, and R-2's raw test alone would say
FRONT-CROSSING. **But it would be struck by the dual-bar qualifier** (I9-1
R-2 + PART XIII §5b): the transform-enabled references reach 89,564 B
(xz -9e --delta=14) and 80,650 B (brotli-q11 on delta14), so the row is
**FRONT-GAP (dual-bar)** and must not be reported as a crossing. The
{synthetic} + supplied-structure nature is carried as a mandatory qualifier on
the narrative and on every number, not as a separate token. Correct sequence,
confirmed: native integration -> byte-exact roundtrip -> bench arbiter (two
hash-identical runs) -> PR-4 window -> this gate's classification -> dual bar
reported last, not least.

**(2) Does "mechanism probe, not generalization" change the token?** It changes
the narrative/qualifier and the admissibility of generalization; the token
still comes from the mechanical test on the measured grid (including the dual
bar). A probe result may be recorded as a capability measurement; it may not be
cited as a crossing or as evidence for real columnar data.

**(3) Do projections alter the tuple?** No. The co-listing tuple counts
**landed, measured rows only** (arbiter over a measured suite). Projections and
prototype wires never alter it; they appear labelled `PROJECTION` /
`prototype wire` and are excluded from the count.

## 7. Dispositions — one line per claim

| # | claim | disposition |
|---|---|---|
| D-1 | ladder 207,197 / 187,436 / 107,564 / 97,057 / 89,877 | **VERIFY** — byte-identical double-run; sha prefixes match |
| D-2 | ca ladder 69,748 / 24,055 / 44,837 / 35,808 | **VERIFY** |
| D-3 | ablation coder-only 19,761 vs mechanism 97,559 | **VERIFY** (arithmetic from the verified table) |
| D-4 | P=14 / 20,000 records; P=23 / 12,000; 56/184 is aliasing | **VERIFY** (independent invariant scan + byte-exact regen) |
| D-5 | "prototype wire, not an ANVIL row; no crossing claim" | **VERIFY** (labelling correct) |
| D-6 | "25.5% below brotli q6 / 36.2% below worktree ANVIL row" | **VERIFY as raw-bar statement**; qualifier REQUIRED: it is above both transform-enabled bars (89,564 / 80,650) |
| D-7 | "mechanism needs P + partition supplied; auto-discovery unsolved" | **VERIFY** — engineering gap recorded |
| D-8 | mechanism novelty (record-reference copy + per-column contexts inside the reference) | **REJECT** — engineering/capability + placement; prior-art lineage in §5 |
| D-9 | P as novelty | **REJECT** — infrastructure (PART XIII §8 stands) |

## 8. What may be built/claimed

May be built and offered to `arch` as an adopt-class capability (columnar
residual contexts for copy tokens), Pareto-gated, with the native-cost caveat
(Python prototype wire; ANVIL rANS/token costs unmeasured) and C1–C7 of I9-2
P-4 applied mutatis mutandis. No novelty claim; no crossing language; every
synth number carries `{synthetic}` and both bars.

---

*Ruled by `research-gate`. Reproduction harnesses: `docs/gate-verify-i9-corpus.py`,
`docs/gate-verify-i9-transform-bar.py`; claim regime:
`docs/gate-verify-regime-i9.md`.*
