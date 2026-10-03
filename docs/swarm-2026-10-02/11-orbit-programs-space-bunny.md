# Track 11 — Orbit/Program Synthesis Compression — Space Bunny Free

**Track:** 11-orbit-programs · **Role:** constructive inventor
**Date:** 2026-10-02 · **Tree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty
**Mandate:** bounded reversible micro-program ISA over prior output; MDL parser/search;
decoder resource ceilings; implicit parameters; exact correction semantics; smallest
defensible mechanism that is not grammar/LZ/VM prior art.
**Status of this document:** design + pre-registration. **FINAL VERDICT IS IN §16: KILL.**
Sections 1–15 are retained as the record of the constructive hypothesis; §16.0 lists exactly
which of their rows are void. **No measurement was performed by this agent.** No local benchmark,
no local fuzzing, no corpus run.

---

## 0. Verdict in one paragraph

The "bounded micro-program ISA" *species* is fully occupied by prior art and by this
project's own measured negatives: the bitstream-as-program framing is LZMA/Brotli/DEFLATE,
grammar-with-loops is RePair/XMill (and its decode leg was **measured negative here**),
predictor+residual is Gorilla/Stencil/ALP/xz `--delta`, transmitted parameters are
foreclosed by the I8 audit, and estimator-derived implicit parameters are **MATH-class
closed**. What is *not* occupied — and what this project's own measurements point at —
is the one axis nobody has shipped: **entropy-free execution of a repeated program
fragment whose entire per-iteration operand set is re-derived by an exact, encode-verified
recurrence from decoder-visible state.** I advance that as **FLI (Fused Loop Instruction)**
and I advance it *conditionally*: the binding unknown is not the mechanism, it is that **the
incumbent mode-15 decoder's per-token cost decomposition has never been measured**, and no
decoder-leg claim in this track can be honestly sized until it is. Recommendation:
**PILOT**, with P1 = cost decomposition (cheap, benefits tracks 05/06/11/18 alike) and
P2 = a byte-only representation oracle, both remote-only. Plus six explicit KILLs and one
HOLD recorded in §12.

---

## 1. Evidence map

### 1.1 Measured facts I rely on (all in-repo, located)

| # | Fact | Source |
|---|---|---|
| M1 | Zero-bit derived transform parameters are defensible **only where the transform is exact (zero residual)**; on statistical transforms the family has *no* defensible novelty position. Measured 0.646–0.691 bits/doubling residual growth over d=4..1024; least-squares reduces the intercept, not the slope. | `RESEARCH_LEDGER.md` PART XIII §7 (DNB-M2, G4-amended); restated `docs/gate-ruling-i9-pr5-position-derived.md` P-2 |
| M2 | A bounded **non-zero**-residual derived parameter is a *third category*: buildable as engineering/adopt, **not claimable**; nearest art is adaptive DPCM / delta-of-delta / JPEG-LS / xz `--delta`. AUDIT-7: every input to the derivation must be available to the decoder **before** it is used; look-ahead-derived parameters are transmitted in disguise. | `docs/gate-ruling-i9-pr5-position-derived.md` P-3.2, P-3.3, P-3.4 |
| M3 | Decisive: on the one file where both arms were measured, **transmitted step (12,921 B) < derived zero-bit step (12,936 B)**. The zero-bit arm was "unnecessary as well as non-exact". | `docs/gate-ruling-i9-pr5-position-derived.md` P-7 |
| M4 | Transmitted parameters = **prior art** (parametric dictionary; US 7,676,506; xz `--delta dist=1..256`). Audit's framing: "the project has twice built the expeditious form and called it progress." | `docs/gate-priorart-audit-i8.md` §4, §1 |
| M5 | Self-referential copy-with-edits = **VCDIFF/RFC 3284**; mask-only = **US 12,373,439 / US 2024/0211132**. The only "NOT FOUND" element in that family is **entropy-coding correction *topology***. | `docs/gate-priorart-audit-i8.md` §3 |
| M6 | ANVIL's correction masks are **~90% unique** (top-32 cover 7–12% of tokens); (k,slot) modal accuracy **~20%**, not 86.5%. Topology coding (mode 13) **lost +13.5%..+21.0%** on every record file. Diagnosis: greedy sparse parser lets correction positions drift. | `RESEARCH_LEDGER.md` Exp. I |
| M7 | Compiled hot-op book (mode 15): real decode win **1.17–1.34x** vs fused mode-12 at near-neutral ratio; the named remaining floor is **"opcode-stream entropy decode + copy throughput"**. Linux measured **75–77% token hot coverage** on `generated.log`. | `RESEARCH_LEDGER.md` Exp. R; `docs/CONTEXT.md`; `FORMAT.md` §HOTOP |
| M8 | Context-switched rANS (mode 6): **−8.3%..−16.2% bytes**, decode **−3%..−10%**. Ratio mechanism, not a throughput primitive. | `RESEARCH_LEDGER.md` Exp. S |
| M9 | RePair/RLZ applied to mode-15's book streams: **−4.15%..−10.76% bytes** but decode **−8.1%..−15.9%** on every primary record file, encode **48–64x** slower. Root cause measured: **eager whole-buffer materialization loses to fused per-byte pulls** on this host/build. | `RESEARCH_LEDGER.md` Exp. Y |
| M10 | Whole-codec J-selection with a raw side-stream budget: implementation landed, all controls green, **formal bench arbiter never ran**, no bar verdict issued. Frozen calibration table (median-7, real mode-15 content): raw **0.10 ns/B bulk / 2.59–2.66 ns/B byte**, rANS-4096 **5.93–6.62**, ctx-rANS **~7–8**. Frozen Windows baselines on `generated.log`: anvil-hotop-rans **175,550 B / 273.534 MB/s decode / 19.681 MB/s encode**; brotli-q9 **124,669 B / 619.508 MB/s / 29.533 MB/s**. | `RESEARCH_LEDGER.md` Exp. AA |
| M11 | Exp. AA calibration verdict on the incumbent constants: "**right ORDER, wrong GAPS**" — any new selector economics built on J inherits a mis-scaled cost constant. | `RESEARCH_LEDGER.md` Exp. AA |
| M12 | ANVIL parse is a bits-based greedy/MDL edge-cost test (token + len/dist varints + `len/8` mask + residual costs vs best exact-plus-literals alternative); `--parse=dp` is a slow forward DP; `--parse=auto` is research-only. | `FORMAT.md` §Parser |
| M13 | Decoder strictness and amplification guards already in force: `1 ≤ blen ≤ block_size`, `raw_n ≤ 16·out_len + 64` per stream, dist ∈ [1, out.size()], `popcount(mask) == residuals`, full substream consumption, CRC-32. RePair/RLZ additionally needed per-rule and cumulative **expansion** caps and a recursion-depth cap. | `FORMAT.md` §Integrity and malformed input |
| M14 | Project rule: **every decoder-visible registry ID must have a direct forced encode→decode test**; selection-only coverage rots silently. | `FORMAT.md` §Registry coverage |
| M15 | I10 already designated adjacent mechanisms: **H2** decoder-derived relocation transform copy (transform sites deterministically recoverable from the referenced phrase); **H3** restricted iterated span generation `ITER(n, body, state₀, update)`; H1 kill conditions include "dependency depth/source locality predicts decode materially worse than the rate surplus can buy back"; §6 makes dependency depth a first-class compression cost. | `docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H1/H2/H3, §6 |
| M16 | ORBIT direction list: #2 conditional-program references over a tiny fixed deterministic VM; #3 implicit/zero-bit transform parameters (`θ = F(d, p, x, previous output)`). 2026 citations (Brevis/Pcodec/OpenZL/AIT) are **operator-reported and unverified**. | `docs/ORBIT_PROGRAM_COMPRESSION.md` |
| M17 | Brotli q11 on delta+zigzag-varint bytes of `synth-arith.bin` = **16,313 B** vs 87,013 B raw — the synth cell's "frontier" is an artifact of Brotli declining a filter it ships elsewhere. Dual-bar rule binding on all synth numbers. | `docs/gate-priorart-audit-i8.md` §1; `RESEARCH_LEDGER.md` PART XIII §5b |
| M18 | A1–A4 provenance ablation (exact-LZ │ transmitted │ implicit │ global-filter) has been an outstanding gate condition since I2-5 and **has never once been executed**. | `docs/gate-priorart-audit-i8.md` §6 action 5 |
| M19 | 2026-10-02, this agent: two web searches ("superinstruction in lossless compression / cloning prior-reference parameters"; "Stencil operators") returned **no relevant hits**. Per the audit's own rule this is a **coverage gap, not a negative**. | this session; `docs/gate-priorart-audit-i8.md` §5 |
| M20 | GitHub Actions measurement classes: deterministic byte/ratio/hash = citation-grade; MB/s and RSS = **scout/ranking-grade**, paired same-job A/B only; "a small timing delta on a shared runner is inconclusive". Promotion questions 1–7 must be answered before integrating a decoder-visible mechanism. | `docs/GITHUB-ACTIONS-BENCHMARKING.md` §2, §3, §9 |

### 1.2 Projections / hypotheses (explicitly not facts)

| # | Statement | Status |
|---|---|---|
| H1 | Per-token symbol coding (entropy pull + dispatch) is a **material but unquantified** share of mode-15's decode cost. | **VOID — CORRECTED IN §16.1: IT IS MEASURED.** `RESEARCH_LEDGER.md:3911-3931` is an instrumented, interleaved, median-7, QPC+invariant-TSC decode-floor profile of the incumbent on this exact cell (`generated.log`; HEAD 392e937 payloads; 175,550 B == frozen baseline exactly; all containers round-trip byte-exact; instrumented decoder byte-identical to `decode_tokens_hotop_fused`): **crc32 44 %; macro-stream eager materialization 26 %; token loop beyond setup 11 % (of which opcode entropy pulls 0.3 %); concat/alloc/headers ≈15 %**, annotated "THE HOT PATH IS CLEAN; the floor is everything AROUND it". Within-run CV 0.6–10 %; drift ±2–4 %; **zero-byte-change control variants moved +2.3–8.1 %**. It does not identify dispatch or copy separately, so it neither passes nor falsifies G1 — but it yields a hard **1.124x** ceiling on any token-loop-only change (§16.2). |
| H2 | The production MDL gate rejects long periodic/same-geometry token runs that a loop instruction could express, because each repetition is costed individually. | **Unmeasured.** Consistent with M6 (drift), M6b/Exp. N/T (69% of greedy sparse matches far-distance; structural propagation never landed). |
| H3 | Opcode-symbol entropy `H_op` in mode-15's hot stream is in **0.3–1.5 bits/token** (0.04–0.19 B). | **Unmeasured, cheaply measurable byte-only** (§9 P1a). Sets the width of the byte-leg projection: −2%..−7%. |
| H4 | A material fraction of record-structured files decomposes into maximal runs of ≥8 consecutive equal-`(class,len,dist)` tokens. | **Unmeasured.** Oracle in §8 exists to test exactly this. |
| H5 | `λ·C_decode` in the shipped J objective is currently too small to move real byte-vs-cycle decisions. | **RESOLVED (adverse) from documented constants — `FORMAT.md` §"Stream selection and the whole-codec budget (S6-1)", implemented mechanics: `J = L + λ·C_us` with `C_us = raw_n · ns_per_byte / 1000` μs and λ = 0.01 B/μs.** Arithmetic: for `raw_n` = 20,000 B at rANS-4096 6.0 ns/B, `C_us` = 120 μs and `λ·C_us` = **1.2 bytes**; the rANS→raw swap saves 118 μs and is therefore worth **1.18 B**. The exchange rate is literally *1 byte = 100 μs*, so **J is effectively pure length with a ≈0.006% tie-break**, and the shipped S6-1 verdict — budget-on output size-identical to legacy, **NO raw-flips** — is exactly the predicted consequence. Consequences for this track: (i) FLI's loop gate must be **pure length** (which is the correct rule anyway); (ii) novelty separator **S4 is withdrawn** (§7); (iii) every track proposing a decode-cost-gated representation (05, 06, 11, 18) is currently gating on an objective whose decode term is ~1e-5 of the stream's byte count. λ = 0.01 is a frozen EXP. L pre-registered constant and **must not be changed inside this track** — re-deriving the exchange rate is a coordinator-level decision. |

---

## 2. Space map — what is already dead or occupied in this track

Do not rebuild any of these; each is cited, not re-argued.

**Occupied by prior art (never claimable):**
1. *Bitstream = a bounded program over prior output.* LZMA's 12-state machine + rep0–rep3; DEFLATE's block symbol stream; **Brotli's meta-block command stream** (`insert-and-copy` quadruples, `insert-length`/`copy-length` splits, distance cache, block-switch). "Compressed data is a program" is not a claim.
2. *Bytecode VM decompressors.* CRUSH (LodePNG), LZH (Limberg) and the whole class of small opcode interpreters.
3. *Grammar with loops/parameters.* RePair (Larsson–Moffat 1998), SEQUIT, Re-Pair-inside-LZ, **XMill / MillView parse-tree pattern compression**, parameterized grammars (JCAD/CGL — which additionally foreclose transmitted parameters).
4. *Predictor + bounded residual.* Gorilla, FPC, Chimp/PDE, ALP, xz `--delta`, and the operator-reported Stencil operator family (AP/PD/GP/RD/BV + per-code residual varint). Measured collision, `docs/gate-priorart-audit-i8.md` §1.
5. *Copy-with-edits / masking.* VCDIFF RFC 3284, bsdiff, Zdelta, Zucchini, WinRAR/7-Zip patch mode, US 12,373,439 and US 2024/0211132.
6. *Discovered-transform self-similarity.* Fractal/PIFS — lineage only (lossless + no contraction + variable references are the four dismissals).

**Closed by this project (do not re-burn):**
7. Statistical/estimated implicit parameters — DNB-M2, **MATH-class** (M1).
8. Bounded-nonzero-residual derived parameters — PR-5, buildable as engineering, **no claim** (M2), and *measured unnecessary* (M3).
9. Correction-topology modeling (mode 13) — **lost 13.5–21%**, masks ~90% unique (M6).
10. Grammar factorization of the book streams — ratio win, **decode-loss** (M9).
11. Global lane/field transposition, G4 direct planner proxy, G5B ordinal proxy, PORDER — killed (`MASTER-BRIEF.md` item 13).
12. Wire-invisible decode optimization — **exhausted** (`MASTER-BRIEF.md` item 12; ledger A9/A10/A12/A15).
13. Record period P as novelty — ruled a file-level framing constant; auto-detection is infrastructure (ledger PART XIII §8).
14. Transformed-reference **search** is already owned by PNRA/TCOPY (invariant indexing) and H2 (decoder-derived transform sites). Track 11 must not re-issue either.

**Therefore the only unoccupied slot in this track** is the *decoder-execution* axis of a program ISA: how a *repeated* program's per-iteration operands are supplied **without transmitting them and without decoding a symbol for them**, while keeping the expansion strictly bounded and non-recursive.

---

## 3. Mechanism — FLI (Fused Loop Instruction)

### 3.1 One-sentence statement

> Add one instruction class — a **counted loop over a single hot-op body class** — whose per-iteration operands (`len`, `dist`) are supplied **entirely by an exact arithmetic recurrence anchored once at loop entry**, so that a run of k token iterations costs **one entropy-coded opcode plus one varint n** on the wire and **zero entropy symbols and zero varint reads per iteration** in the decoder, with strictly bounded, non-recursive expansion.

### 3.2 Preconditions this mechanism inherits (do not redesign)

FLI is defined **relative to the existing mode-15 hot-op semantics** (`FORMAT.md` §HOTOP), which already provides:

* a compiled book of entries `(kind, len uvar, shape byte)`, `K ≤ 254`;
* per-shape last-displacement state `last[shape]` (`num_states ∈ {1, 28}`, shapes 0..27);
* hot `kind=1` semantics `dist = last[shape]`, hot `kind=0` literal run;
* a macro/rare escape path that carries the full mode-12 coding and **updates the same shape state**.

FLI changes **only** how a maximal run of consecutive hot iterations is *emitted and executed*. It does not touch the book, the shape state, the macro path, the literal path, or any stream codec.

### 3.3 ISA (complete and closed — 3 opcodes total, one of them a no-op)

New opcode values appended to mode-15's existing opcode alphabet (which already has `0..K` plus an escape):

| Opcode | Name | Payload | Semantics |
|---|---|---|---|
| `K+1` | `LOOP_EQ` | `uvarint n` (raw stream `S_n`) | execute the **hot body class** `c` for `n` iterations with `L_i = L_0` and `D_i = D_0 = last[shape_c]` for all i |
| `K+2` | `LOOP_ARITH` | `uvarint n` (raw `S_n`), `zigzag varint δL` (raw `S_dl`), `zigzag varint δD` (raw `S_dd`) | `L_i = L_0 + i·δL`, `D_i = D_0 + i·δD`, `D_0 = last[shape_c]` |
| `K+3` | `LOOP_END` | — | **not emitted**; present only so the decoder can fail closed if a malformed stream leaves a loop open. Cost: one unused alphabet symbol. |

`c` (the body class) is *not* a payload byte: it is the hot book's entry identity, so it is recovered from the opcode stream's `K+1`/`K+2` code **only if** the block declares exactly one loop body class in its header (`uvarint loop_class`, ≤ 255). This keeps the loop entry at 1 entropy symbol. A block needing two body classes uses two distinct escape codes (`K+4` → class+1) — bounded at 4 classes, adding ≤4 alphabet symbols.

`LOOP_EQ` is the identity-recurrence case and is the one I expect to dominate (record-structured data). `LOOP_ARITH` is included so that the *generality* claim (recurrence, not just repetition) is testable and so the ablation can separate "repeated identical token" from "arithmetic continuation".

### 3.4 Decoder semantics (normative)

```
state added by FLI:
  loop_n, loop_i, loop_L0, loop_dL, loop_D0, loop_dD, loop_class   -- 7 scalars
  S_n, S_dl, S_dd : three raw byte cursors over already-materialized streams

on LOOP_EQ(n):
    require n >= 2 and n <= KITER_MAX                        (header)
    L0  <- book[loop_class].len
    D0  <- last[shape(loop_class)]                           require set and D0 in [1, pos]
    for i in 1..n:
        require pos + L0 <= block_end
        emit_copy(D0, L0)                                   periodic if D0 < L0 (as mode 11-15)
        pos += L0
        last[shape] <- D0
    consume exactly one uvarint from S_n

on LOOP_ARITH(n, dL, dD):
    ... identical, with L0 += dL and D0 += dD after each iteration,
    each iteration requiring D0 in [1, pos] and pos + L0 <= block_end
```

**Exactness.** FLI has no correction mechanism at all. A loop may only span **exact** token iterations; any sparse-corrected token (mode 11/12/14/16 type 2/3/4) **breaks the run** by construction, because the encoder only forms runs over tokens it has already emitted as exact copies. Therefore:

* the residual is identically zero — **G4-amended is satisfied not by an estimator but by construction** (M1);
* DNB-M2 is **not applicable**: nothing is estimated. `δL`, `δD`, `L_0` are values the encoder *chose and verified*, and `D_0` is decoder state, so there is no error term and no `log2(d)` growth (this is the structural separator from M1/M2, and it is the reason the mechanism is expected to behave unlike ARI-STRIDE);
* the decoder performs **no verification of exactness** — exactness is a property of *which* tokens the encoder emitted, not a runtime check. This is what makes the decode leg cheap.

**Correction semantics are therefore untouched**: they live entirely in the macro/rare path and in type-2/3/4 tokens, exactly as mode 15 specifies today. FLI cannot be applied across a region whose explanation requires residuals — which also means FLI is structurally immune to the M6 mask-uniqueness problem.

### 3.5 Implicit parameters — AUDIT-7 line-by-line

| Parameter | Source | Transmitted bits |
|---|---|---|
| `loop_class` | block header | uvarint (once per block) |
| `n` | stream `S_n`, read at the loop entry | uvarint (~1–2 B) |
| `L_0` | book entry (already decoder-visible) | **0** |
| `D_0` | `last[shape]` (set by a previously executed instruction) | **0** |
| `δL`, `δD` | zero in `LOOP_EQ` | **0** |
| `shape` | book entry | **0** |

Causality, in execution order: the loop header is decoded first; `last[shape]` was written by an instruction that has already completed (never by a future or out-of-block instruction); `L_0`/`shape` come from the book, decoded once at block start. **No input is derived from a region's aggregate statistics, and no look-ahead is used anywhere.** This is the AUDIT-7 statement PR-5 P-3.4 requires, stated as a decoder algorithm.

### 3.6 Why this is not "grammar, LZ, or VM prior art" — and where I concede

**Conceded as lineage (not claims):** counted repetition is prior art in grammar coding (RePair rule expansion; XMill repeated parse patterns) and in Brevis's reported DSL `repeat/scan` primitives (operator-reported, unverified — M16). Superinstructions are a mature interpreter technique. Periodic byte loops are DEFLATE overlapping copies and LZMA rep-matches. Single-scalar parameter reuse is LZMA rep0–rep3, zstd repeat-offsets, and Brotli's distance cache. A hot-op book is Brotli's fused-command/aliasing lineage and this project's own Exp. R.

**Claimed (species-level, and it is thin — see §7):** the *interaction* of

1. an explicit counted loop in the **decode stream** (not a grammar rule, not an overlapping byte match),
2. whose per-iteration operand set is **fully re-derived by an exact recurrence** with **zero transmitted and zero entropy-decoded operands**,
3. executed **fused** (no materialization, no rule table, no recursion, no side table),
4. under the project's **existing J objective** so it is self-gating and cannot worsen a block under that objective,
5. with a **hard bounded, non-recursive expansion** that introduces no new amplification class.

Each ingredient is prior art or project-internal. **The combination is the claim, and I label it a systems/unification claim, in the same sense PR-5 P-4 item 2 does** ("what has never been shipped is a practical LZ-family compressor unifying multiple equivalence relations under one MDL parser"). That is the only slot the audit leaves open, and it is a weak slot; §7 states the risk plainly and hands track 20 four falsifiable separators.

---

## 4. Encoder + decoder state machine

### 4.1 Encoder (three passes; pass 1 is the existing parser, unchanged)

```
P1  existing parse  -> token trace T = [t_1 .. t_N]   (UNCHANGED; bits-based greedy/MDL gate)
    also emits, per token, the decoder-side effect it will have:
        kind_i in {LIT, HOT_EQ, HOT_LEN, MACRO_*}, len_i, shape_i, dist_i (or ⊥)

P2  recurrence scanner  -- O(N), <= 6 comparisons/token, 4 registers
    walk T; maintain a maximal run R of consecutive tokens with the SAME hot body class c
    and with all fields decoder-materializable in this block (i.e. dist_i derived from
    last[shape] rather than from an absolute varint -- the mode-12 flag-0 case ENDS a run)
    a run of k >= 2 is ADMISSIBLE iff
        (dL_i = len_{i+1} - len_i) is constant over the run   -> LOOP_EQ if dL=0, else LOOP_ARITH
        (dD_i = dist_{i+1} - dist_i) is constant over the run -> included in LOOP_EQ if 0
    runs are maximal and disjoint by construction

P3  J gate, per run  -- O(N) total, constant work per run
    J_loop     = H_op_loop + bytes(n) + (bytes(dL)+bytes(dD) if LOOP_ARITH)
    J_expanded = k * H_op_hot
    accept iff J_loop < J_expanded
    H_op_hot is supplied from the block's own measured opcode distribution (P1a), NOT tuned
    runs of k < 4 are rejected unconditionally: a recurrence over k tokens with 2 free
    parameters saves nothing below k = 3, so this bound is derived, not tuned

output token stream: the accepted runs collapse to one opcode; everything else is emitted
                    by the existing path, with the collapsed tokens' state updates
                    PROVEN equivalent (see 4.3) so `last[]` evolution is unchanged
```

**Bounded inference cost.** P2+P3 are `O(N)` with a constant under ~10 ops/token against a
parse whose own per-token cost is already ~25 ns/token at 39 MB/s (Exp. X measurement).
Projection: **+1.5%..+4% encoder parse time**, no change to asymptotic class, no new data
structures beyond 4 registers. FLI therefore respects the working agreement ("single-pass,
cache-resident parser"; no global DP).

### 4.2 Decoder (fused, no materialization)

```
block start:  book, last[] (existing), loop registers (7 scalars), 3 raw stream cursors
token loop:   switch(opcode):
                <= K   : existing hot op (unchanged)
                K+1    : LOOP_EQ     -> fused inner loop, no entropy reads inside
                K+2    : LOOP_ARITH  -> fused inner loop, no entropy reads inside
                K+3    : LOOP_END    -> malformed-path only; reject
              else    : existing macro/rare path (unchanged)
loop inner:   for i in 1..n { bounds checks; periodic memcpy; last[shape] = D0; }
```

The inner loop contains **no entropy decoder call, no varint read, no dispatch on a symbol**.
That is the entire decode thesis, and it is the reason FLI escapes the M9 failure mode: the
loop is executed *where the tokens are consumed*, not expanded into a buffer.

### 4.3 Correctness argument (sketch; the prototype must test it)

*Lemma 1 (loop equivalence).* For an accepted run, the emitted `LOOP_EQ`/`LOOP_ARITH`
produces byte-identical output to the expanded token sequence, **and** leaves `last[]` in the
same state as the expanded sequence.
*Proof sketch.* P2 admits a run only when all `len_i` and `dist_i` follow the recurrence that
the loop body applies. The expanded sequence applies exactly the same `last[shape]` reads and
writes (all `dist_i` come from `last[shape]`, since flag-0/absolute tokens end a run), so the
state trace coincides induction on `i`. ∎

*Lemma 2 (state non-interference).* A loop never spans a token whose decoder-side effect is
not reproduced by the loop body. Macro tokens and literal tokens end runs (P2 filter). ∎

*Lemma 3 (boundedness).* `n ≤ KITER_MAX` and `len ≤ LEN_MAX` are checked at loop entry;
`pos + len ≤ block_end` is checked every iteration. Therefore a loop emits at most
`min(n, KITER_MAX) · LEN_MAX` bytes and the existing `1 ≤ blen ≤ block_size` /
`raw_n ≤ 16·out_len + 64` guards dominate. ∎

*Lemma 4 (no new amplification class).* FLI cannot recurse: there is no call, no rule table,
no indirect target. Contrast M13's RePair/RLZ, which required per-rule, cumulative-expansion,
and recursion-depth caps. FLI's worst case is *its own output length*, which every other
token already bounds. ∎

---

## 5. Cost model

### 5.1 Wire ledger per loop of `k` iterations

| Term | Bytes | Note |
|---|---|---|
| loop opcode | `H_op` | entropy-coded in the existing opcode stream; `H_op` = −Σ p log₂ p of the opcode alphabet, bits/8 |
| `uvarint n` (raw stream) | `1` for n≤127, `2` for n≤16383 | raw stream, ~2.6 ns/B (M10) |
| `δL`, `δD` | `0` in `LOOP_EQ`; ~1+1 in `LOOP_ARITH` | `LOOP_EQ` is the expected common case |
| new alphabet symbols | ≤ 4 codes ≈ **≤ 30 B per block** | charged pessimistically |
| expanded equivalent | `k · H_op` | what mode-15 pays today for the same k tokens |
| **Δ bytes per loop** | **`2 − (k−1)·H_op`** | |

Byte-neutrality threshold: `k ≥ 1 + 2/H_op` → `k ≥ 8` at `H_op=0.3 B`, `k ≥ 5` at 0.5,
`k ≥ 3` at 1.0, `k ≥ 3` at 1.5 (rounded up). Under H3's uncertainty band the projected
whole-file byte win on a record file is **0% .. −7%**; the width is entirely `H_op`, which is
**byte-only and deterministic** (P1a). I refuse to pick a number before measuring it.

For scale, using the record-file token density implied by M7 (75–77% hot coverage) and a
~40 B mean iteration, a 1.9 MB log has ~47 k match iterations; at `H_op = 0.5 B` and mean
admissible `k = 8`, that is ≈ −8.9 KB on a 452 KB output (−2.0%); at `H_op = 1.0 B`,
≈ −30 KB (−6.6%). **Projection, not a measurement.**

### 5.2 Decode cycle ledger per iteration

Using only the in-repo measured constants (M10) and a conventional short-memcpy model
`M(L) = max(2.0, 0.12·L)` ns (projection; needs confirmation):

| Path | Per-iteration cycles (ns) |
|---|---|
| expanded hot op | `c_pull + c_dispatch + c_state + M(L)` ≈ `6.0 + 2.0 + 1.0 + M(L)` |
| FLI loop | `2 adds + 1 store + M(L) + loop overhead` ≈ `3.0 + M(L)` |
| **saved** | **`≈ 8 ns/iteration`, independent of L** |

At L=40 that is 13.8 → 9.8 ns (1.41x); at L=8, 11.0 → 7.0 ns (1.57x). At L=256 the copy
dominates and the saving fraction falls to ~1.09x.

**Projection, and deliberately framed as a bound rather than a promise:** FLI removes exactly
`≈8 ns` of per-iteration work. Against a frozen whole-codec 273.534 MB/s (M10) that is
worth `8 ns / 40 B = 0.20 ns/B` of the 3.66 ns/B budget — i.e. **≤ ~5% whole-codec**, unless
H1 turns out to be wrong and per-token symbol coding is a much larger share than the
mode-15 description implies. **This is precisely why P1 must run first.** Sizing a
decoder-leg claim on H1 is the single easiest way for this track to fail honestly-but-wisely
after burning a CI cycle.

### 5.3 Memory / resource ceilings

| Resource | Cost | Notes |
|---|---|---|
| Decoder state | **+≤ 200 B, O(1)** | 7 loop registers + 3 cursors; **no** side table, **no** rule table, **no** program text |
| Output buffer | unchanged | FLI appends into the existing buffer; no temporary materialization (M9's failure mode) |
| Peak RSS | **unchanged** | FLI adds no persistent structure; the I10 aux-unbwt RSS legs (ledger PART XV item 2) are unaffected |
| Expansion | ≤ `KITER_MAX · LEN_MAX` per instruction, and ≤ declared `blen` | **no new amplification class** (Lemma 4) |
| Recursion | **impossible** | no depth cap needed (contrast M13) |
| Random access / locality | **unchanged** | FLI has **no reference chain**, so I10 §6's "dependency depth is a first-class compression cost" does not apply — this is the design choice that keeps FLI clear of H1's kill condition |
| Per-stream length cap | unchanged `raw_n ≤ 16·out_len + 64` | `S_n`, `S_dl`, `S_dd` are ≤ 3 varints per block, so these streams are trivially bounded; **charge their headers** (~3 mode bytes + 3 length varints ≈ 12 B/block) |

### 5.4 Asymptotics

* Encoder: `Θ(n)` before and after; P2+P3 add `Θ(N_tokens)`. No superlinear term, no new index.
* Decoder: `Θ(n)` before and after, with a smaller constant on covered iterations. Worst case
  (no admissible run) is the expanded path plus ≤ 4 unused alphabet symbols per block.
* Memory: `Θ(1)` added state; total decoder memory remains `Θ(output)`, unchanged.
* Ratio: bounded below by the J gate — **if and only if `λ·C_decode` is calibrated** (H5).
  **H5 is resolved and the "if and only if" fails**: the frozen λ makes `J` effectively
  pure-length, so FLI's gate degrades to a byte test. That is benign for FLI (accept every
  byte-negative loop, decline every byte-positive one) but it means the *cycle-aware*
  part of the original framing is **not implemented**, and a "~8 ns/iteration" saving
  cannot be traded for bytes by the current selector. FLI is therefore a **byte-negative**
  mechanism whose decode benefit is a *consequence* of accepting it, not a term in its
  objective. Any future candidate that genuinely needs a rate-vs-cycles trade must first
  force a coordinator-level re-derivation of the exchange rate.

---

## 6. What FLI is deliberately NOT

* **Not a VM.** No jumps, no indirect targets, no rule instantiation, no recursion. A general
  grammar VM is prior art and measured decode-negative here (M9).
* **Not a transform-parameter mechanism.** No Δ, no θ, no estimator. Those axes are closed
  (M1) or foreclosed (M4), and the profitable form is already held by TCOPY/PNRA/H2.
* **Not mask/topology coding.** Masks are ~90% unique (M6); FLI never crosses a masked token.
* **Not a search contribution.** Candidate generation is unchanged. FLI is a *representation +
  execution* mechanism over an existing parse, the same "representation before discovery"
  split that TCOPY/PNRA/ARI-REF used.
* **Not a decode-only trick.** It changes the wire, so it is not in the exhausted
  wire-invisible class (M-master-brief item 12); it must pay for itself in bytes, and §5.1
  says the byte price is negative (a win) if and only if admissible runs are long.

---

## 7. Novelty / prior-art risk — honest

**Risk: HIGH that the claim is judged "engineering".** The species is occupied five times
over (§2 items 1–6), the audit's genus ("encode a value relative to already-reconstructed
context") is unclaimable, and each FLI ingredient individually reads as an obvious extension
of something shipped: counted loops (grammar coding), zero-bit operand reuse (LZMA rep0–rep3
/ Brotli distance cache), superinstructions (interpreter literature), fused execution
(Brotli's command loop).

**What genuinely has no shipped instance** (my best case, and it is narrow):

* **S1** No shipped general-purpose codec has an instruction that **suppresses per-iteration
  entropy-coded symbols inside a loop body**. Brotli/LZMA/DEFLATE have no token loops;
  grammar coders suppress symbols but *materialize*.
* **S2** No shipped grammar/superinstruction coder executes its loop **fused** in the
  consumer rather than expanding to a buffer. Exp. Y measured exactly this distinction on
  this host: materializing RePair/RLZ cost 8–16% decode while shrinking bytes 4–11%.
* **S3** No shipped codec supplies a **multi-parameter operand vector** by an **exact,
  encode-verified recurrence** with a hard non-recursive ceiling. Zero-bit reuse in the
  literature is single-scalar (distances) or statistical (delta-of-delta, which is *not*
  zero-bit and is not exact).
* **S4 — WITHDRAWN (post-H5).** "No shipped codec gates a representation choice by a
  λ-weighted decode-cost objective at the token level" is *true but unreachable for ANVIL*:
  with the frozen λ the decode term is ~1e-5 of stream bytes, so no ANVIL mechanism can
  actually exercise such a gate. FLI must not claim S4, and G10 below is restated to match.
  The claim now rests on S1–S3 only.

**Handoff to track 20 (kill team):** S1–S4 above are the decisive novelty separators to
search. If S1 or S2 is anticipated by anything (including non-general-purpose systems), the
claim collapses to adopt-class and this report's recommendation must be downgraded to
**HOLD/KILL**. I have already searched twice this session and got nothing (M19) — which per
the audit's own standard is a gap, not a clearance.

**Ablation the gate should require before any claim is recorded** (extending the never-run
A1–A4 of M18 to this family):

| Arm | What it isolates |
|---|---|
| A0 | mode-15 baseline, unchanged |
| A1 | **+ raw operand side-streams, no loops** (isolates the Exp-AA raw budget alone) |
| A2 | **+ loops, `LOOP_EQ` only** (repetition claim) |
| A3 | **+ `LOOP_ARITH`** (recurrence claim; Δ = A3 − A2) |
| A4 | **grammar control**: RePair over the same opcode stream, fused-pull variant (the strongest prior-art control) |
| A5 | **materialization control**: FLI loop expanded into a buffer then byte-walked (reproduces M9's failure; expected negative) |
| A6 | **J-gate ablation**: λ=0 vs λ=0.01 (isolates the gating claim, S4) |

A claim is supportable only if A2 > A1 on bytes **and** A3 ≥ A2, **and** A2 ≥ A4 on decode.

---

## 8. Minimum prototype (representation-only, byte-only)

Location (new files only, nothing wired into production):

```
prototypes/swarm-2026-10-02/11-orbit-programs/space-bunny/
  README.md          what it computes, how to build REMOTE, thresholds, what it does NOT do
  loop_oracle.cpp    ~200 lines, no dependencies, no codec code
```

**What it is.** A self-contained oracle that produces a **greedy exact-LZ token trace** of an
input file and then measures, over that trace:

* the histogram of maximal runs of consecutive match tokens;
* the recurrence-family hit matrix for runs of k ≥ 2: `(δL=0,δD=0)`, `(δL=0,δD≠0)`,
  `(δL≠0,δD=0)`, `(δL≠0,δD≠0)`;
* **coverage**: match bytes inside admissible runs (k ≥ 4) / total match bytes, and / total bytes;
* **byte delta** under two explicitly-labelled baselines:
  * **B1 (raw-LZ)** — per-iteration `uvarint(len)` + zigzag `Δdist` + type ≈ 3.7 B. *This
    baseline trivially favours FLI and is reported only for completeness. It is not binding.*
  * **B2 (fused hot-op book)** — per-iteration cost `H_op` only, swept over
    `H_op ∈ {0.3, 0.5, 1.0, 1.5}` B. **B2 is binding.**
* **symbols-eliminated** `Σ(k−1)` over admissible runs — the timing-free quantity that a later
  remote timing run converts into MB/s via a single calibration constant.

**Honest framing (ARI-REF harness precedent, M17's caveat applies in full):** absolute byte
counts from this oracle are **not comparable** to `anvil.exe`, Brotli, or the frozen suite
baselines. Only the within-harness loop-vs-expanded delta and the coverage percentages are
meaningful. The oracle uses a 4-byte-hash, small-MRU, min-match-4 greedy matcher, which is
*not* ANVIL's MDL-gated parse; consequently **its coverage figure is an optimistic bound**
relative to what the production parse would admit.

**Status: specified and written, NOT built, NOT run.** Per the master brief no local corpus
benchmark may be executed; the oracle's first execution is remote (§9 P2).

---

## 9. Remote preregistered benchmark

Protocol per `docs/GITHUB-ACTIONS-BENCHMARKING.md` (tiers A/B/C, §5 `taskset` pin,
§8.1 `manifest.json` + `bytes.csv`, §9 promotion questions 1–7) and the I10 remote ladder
(R0 build/correctness → R1 byte-only oracle → R2 scout throughput → R3 promotion evidence).
Manual dispatch only. Corpus: dev corpus (smoke), Silesia (211,938,580 B), enwik8
(100,000,000 B), the held-out PE set `u3`, and the record files
`generated.{json,jsonl,log,sqlite}` — with the frozen `generated.log` pair as the
pre-registered primary cell (anvil-hotop-rans 175,550 B / 273.534 MB/s; brotli-q9 124,669 B /
619.508 MB/s, M10).

### P1 — cost decomposition of the incumbent (no mechanism). R0+R2.
* **P1a (byte-only, deterministic, citation-grade):** compute `H_op` (and the full opcode
  distribution) of the frozen mode-15 output for every corpus file. Answers H3.
* **P1b (byte-only):** compute per-token category counts and mask/literal/macro byte shares.
* **P1c (scout, paired same-job A/B):** instrumented mode-15 decode with **counters**, not
  timers, for: entropy-pull count, varint-read count, copy bytes, macro-path entries,
  literal bytes. Report `pulls per output byte`. Answers H1.
* **P1d (byte-only):** resolve H5 by reconciling `λ`, cost-unit `c`, and the Exp-AA ns/B
  table into one documented product, and publish the arithmetic before any FLI decision.

### P2 — representation oracle (byte-only). R1.
Run `loop_oracle` over Silesia + enwik8 + PE set + record files. Report coverage, hit matrix,
`Σ(k−1)`, and B2 byte delta at the four `H_op` points. **This decides whether FLI is worth
building at all** and fixes the GO/NO-GO of §10 G2/G3 *with data already in hand*.

### P3 — mechanism build + ablation. R2, then R3 if P2 passes.
Arms A0–A6 of §7 in a single job, same build, same flags except the arm selector,
`--fli=off` byte-identical to the freeze-HEAD binary (M10's chain requirement), round-trip and
fuzz **before** any byte counts, `anvil_bench` median-3, paired bootstrap CI on log-ratios, no
outlier deletion, no ambient-load violations.

### Mandatory controls (any FAIL ⇒ run VOID)
* `--fli=off` byte-identity, chained against the frozen binary.
* `repeat.jsonl` no-regression (must be ≤ same-build exact-LZ row).
* `random.bin` ≈ 262,166 B unchanged (encode-time negative gate).
* `synth-arith.bin` / `doc.md` / `src.cpp`: FLI must not fire (reported, not required).
* Dual-bar reporting on every synth cell (M17).
* Full round-trip on all corpus files + PEs in **both** arms, plus forced-registry coverage
  per M14 (new mode ID ⇒ new forced encode→decode test; **FLI must ship with a forced flag**,
  which is a hard condition from M14, not an option).
* Fuzz: single-bit mutations of FLI frames must be rejected or detected, never silently
  accepted. Specific probes required: truncated `S_n`; `n = 0`; `n = KITER_MAX+1`;
  `D_0 = 0`; `D_0 > pos`; loop spanning the block end; `loop_class` out of book range;
  `last[shape]` unset at loop entry.

---

## 10. GO / NO-GO thresholds (fixed **before** any FLI measurement)

| Gate | GO | NO-GO |
|---|---|---|
| **G0 — prior art** | Track 20 reports S1–S4 unanticipated | any of S1/S2 anticipated ⇒ **NO-GO** (downgrade to adopt-class) |
| **G1 — cost decomposition (P1c)** | measured per-iteration **entropy+dispatch overhead ≥ 25%** of measured per-iteration decode cost on the primary cell | < 25% ⇒ **NO-GO on the decode leg**; the byte leg may still proceed as adopt-class ratio work |
| **G2 — coverage (P2)** | ≥ **40%** of match bytes and ≥ **30%** of all bytes inside admissible runs (k ≥ 4) | < 40% match-byte coverage ⇒ **NO-GO** |
| **G3 — run length (P2)** | median admissible run length ≥ **8** on ≥ 3 of 4 record files | median < 8 ⇒ **NO-GO** |
| **G4 — byte leg (P2, B2)** | byte delta ≤ **−1.0%** on the primary cell at `H_op = 0.5 B`, and ≤ 0% at `H_op = 1.0 B` | positive at `H_op = 1.0 B` ⇒ **NO-GO** (byte cost not paid) |
| **G5 — implementation (P3, arms A2/A3)** | A2 bytes ≤ A1 bytes on ≥ 3/4 record files; A3 bytes ≥ A2 − 0.2% | otherwise ⇒ **NO-GO** |
| **G6 — decode leg (P3)** | A2 decode ≥ **1.08x** A1, paired CI excluding 1.00, with G1 satisfied | CI includes 1.00 ⇒ **DECODE-SHORT**: record the byte result as engineering, adopt-class, and do **not** record a frontier claim |
| **G7 — attribution (P3)** | A2 ≥ A4 on decode AND A3 ≥ A2 on bytes | A4 ≥ A2 on decode ⇒ the gain is the known grammar effect, not FLI ⇒ **NO-GO as claim** |
| **G8 — control (P3)** | A5 (materialized) measurably worse than A2 — reproduces M9 and shows the fusion is load-bearing | A5 ≥ A2 ⇒ fusion is not the mechanism ⇒ **NO-GO as claim** |
| **G9 — ceilings** | peak RSS ratio vs A1 within [0.98, 1.05]; no new amplification class; all fuzz probes clean | RSS growth or any silent accept ⇒ **HARD NO-GO** |
| **G10 — byte gate (P3)** | with a **pure-length** gate (λ forced to 0, per the resolved H5) the byte delta on the primary cell is ≤ **−1.0%** and `repeat.jsonl` / `random.bin` are byte-flat | any accepted loop increases the primary cell's bytes ⇒ the gate is not doing its job ⇒ **NO-GO** |

**Non-negotiable:** thresholds are not moved after seeing data (master brief item 5).
**Frontier language** requires the full R-3 ladder plus two hash-identical arbiter runs and
sign-off; a decode-leg win alone is not a crossing (`docs/gate-ruling-i9-pareto-win.md` R-2/R-3
semantics, per PR-5 C7).

---

## 11. Adversarial failure cases

| # | Attack / failure | Expected behaviour | Risk |
|---|---|---|---|
| A1 | Crafted stream with `n = KITER_MAX`, `len = LEN_MAX` in a tiny block | rejected at `pos+len > block_end`; existing `blen ≤ block_size` dominates (Lemma 4) | low |
| A2 | Crafted stream leaving a loop open (no terminator) | `LOOP_END` is not emitted by any encoder; decoder rejects on `out.pos != block_end` at block end | low — but **G14/M14 requires a forced test**, else the path rots |
| A3 | `S_n` truncated / varint wrap | uvarint canonical-form + length checks, mirroring the RLZ `dv/lv ≥ 0xFFFFFFFF` wrap guard that already exists (M13) | low |
| A4 | `loop_class` ≥ `K` (book empty) | reject before touching the book | low |
| A5 | `last[shape]` unset at loop entry | reject, same rule mode-15 hot kind-1 already enforces | low |
| A6 | **Hostile data engineered to maximise loops**: a file of `n`-token runs with `δL=0, δD=0` and mean `k` just above the byte-neutrality threshold → FLI wins bytes but the encoder cost is unchanged; conversely `k` just below → bytes grow. | J-gate declines the latter. **Residual risk: the adversary tunes `k` to sit at `k = 1+2/H_op` and harvest ~0.** Not a security issue; a *neutrality* nuisance. Recorded. |
| A7 | **Worst-case decode-time amplification relative to a same-size baseline**: FLI is never *slower* per iteration than the expanded path, so decode time is monotone-improving. The only regression path is J mis-calibration (H5). | bounded | low |
| A8 | **Encoder blowup**: P2/P3 are O(N) — but P2's run construction must be a single linear pass; a naive "for each start, extend" is O(N²). Prototype must assert the linear form (run-length counters), and the CI job must record encode wall-time to catch a regression to quadratic. | bounded if implemented linearly | **medium** — this is the classic way a "bounded search" becomes unbounded; add an explicit code review item |
| A9 | **Coverage inflation by the oracle**: min-match-4 greedy over-produces long runnable stretches relative to ANVIL's MDL gate. If P2's coverage is measured on this trace, G2 could pass while production admits far less. | mitigated by the explicit "optimistic bound" label; P3's A1/A2 arms are the real test | **medium-high** — treat P2 as a *screen*, G5 as the *decision* |
| A10 | **Interaction with `LOOP_ARITH` producing a periodic-copy alias**: `D_0 < L_0` means the copy is periodic; a *diagonal* period (D_0 not dividing L_0) is still exactly periodic under the existing rule, but a reviewer may question whether FLI can create a distance that aliases past the current position. | `D_0 ∈ [1, pos]` checked every iteration; unchanged from modes 11–15 | low |
| A11 | **Cross-block**: FLI never crosses a block boundary (blocks reset `last[]` and the cursor). A run that spans a boundary is simply split; the split costs two loop entries. | expected, cheap | low |
| A12 | **Mode-15 book exhaustion**: `K ≤ 254`; adding ≤4 loop codes. If a block already uses all 254 entries + escapes, the escape alphabet grows — bounded, charged in §5.1. | bounded | low |
| A13 | **Claim laundering**: a positive byte result gets reported as "program-synthesis compression beats Brotli". Pre-empted: G0/G7, dual-bar (M17), `{synthetic}`/`{engineering}` labelling, and PR-5 C1–C8 conditions apply verbatim. | procedural | **high if ignored** |
| A14 | **Redundancy with track 12 (encoder-only search) and track 18 (decoder architecture)**: FLI's P1c and its fused-loop shape are shared instruments. Coordinate before building; do not duplicate the harness. | procedural | medium |

---

## 12. Recommendation

### 12.1 Dispositions

| Item | Disposition | Why |
|---|---|---|
| General bounded micro-program **VM** / grammar VM over bytes | **KILL** | prior art (RePair/XMill/Breach/CRUSH/LZH) **and** measured decode-negative in this repo (M9). No amount of novelty review rescues a grammar that materializes. |
| Any **transmitted-parameter** program ISA | **KILL** | prior art, foreclosed twice (M4); the project's own twice-repeated error is named in the audit. |
| **Estimator-derived implicit parameters** (any width, any residual bound) | **KILL** | DNB-M2 MATH-class (M1); bounded-residual variants are engineering-only (M2) and measured unnecessary (M3). |
| **Mask / correction-topology cloning** | **KILL** | masks ~90% unique (M6); mode 13 lost 13.5–21%. |
| FLI as a **ratio-only** mechanism (byte win, no decode claim) | **HOLD as adopt-class** | the byte leg is plausible (G4) but a ratio-only win on record files was already shown insufficient by the project's history (M8, M9, and master-brief item 12's decode requirement). Worth keeping only if G6 fails while G4 passes. |
| **`LOOP_ARITH` (the recurrence generalization)** | **PILOT inside FLI, gated by A3 ≥ A2** | the *generality* half of the claim is untested and is where the audit's species argument lives; it must not ride in on the repetition half. |
| **FLI (Fused Loop Instruction)** | **PILOT** | see below |

### 12.2 Final: **PILOT** — not PROMOTE-TO-REMOTE, not HOLD

PILOT means: build and measure **P1 and P2 remotely now**; do **not** build the mechanism yet.

*Promote-to-REMOTE is not justified* because the decode leg's size is gated on an
**unmeasured** quantity (H1) whose plausible value spans "material" to "≈5% of whole-codec"
(§5.2). Funding a mechanism build before measuring it would be the project's characteristic
failure mode.

*HOLD is not justified* because (a) P1 is nearly free and **unblocks tracks 05, 06, 11 and 18
simultaneously** — the cost decomposition of the incumbent decoder is an instrument everyone
needs and nobody has; (b) P2 is byte-only, deterministic, cheap, and could return a decisive
**NO-GO** in one CI job, which is a good outcome; (c) the mechanism is a ~200-line encoder
change on top of an existing mode, with a decoder change that is one fused inner loop.

**Explicit condition attached to the PILOT:** if P1c shows the per-iteration
entropy+dispatch overhead is **< 25%** of per-iteration decode cost (G1), FLI's decode thesis
is dead on this architecture and the correct disposition is **KILL**, not "ratio-only adopt" —
because master brief item 12 already retires wire-invisible decode work, and a byte-only FLI
would be a fourth instance of the pattern this project has recorded three times.

### 12.3 Sequenced ask

1. ~~**P1d first, before P1a** — publish the `λ · c` arithmetic.~~ **DONE, and it is the
   highest-leverage finding in this report.** Resolved from `FORMAT.md` §S6-1's own documented
   mechanics (H5): `C_us = raw_n · ns_per_byte / 1000`, λ = 0.01 B/μs ⇒ **1 byte = 100 μs**
   ⇒ for a 20 KB stream the whole decode-cost term is **1.2 bytes**. **J is effectively
   pure length.** This predicts, and thereby explains, the already-measured S6-1 verdict
   (size-identical output, no raw-flips) — and it is a **cross-track blocker for tracks 05,
   06 and 18**, not just 11: any candidate that needs a rate-vs-cycles trade is currently
   unable to express one. Escalate the exchange rate to the coordinator as a standalone
   question. **Do not change λ inside this track** — it is a frozen EXP. L pre-registered
   constant and changing it after seeing outcomes is the forbidden move.
2. **P1c** — the incumbent cost decomposition. Cross-track instrument; coordinate with 18.
3. **P2** — the byte-only oracle screen (prototype already specified in §8, not built).
4. Only then: decide G2–G5, and only if they pass, fund P3 with arms A0–A6 and the forced
   registry test (M14).

### 12.4 Honest self-assessment

The most likely outcomes, in my own order of probability:
**(a) G1 fails** — decode cost is copy- and literal-dominated, FLI's decode leg is
DECODE-SHORT, the honest verdict is adopt-class ratio work (~−2%), and the report's value is
**(b) G2/G3 fail** — real parses do not decompose into long admissible runs, and the
mechanism is dead at the screen. **(c) G6 passes but G7/G8 fail** — the gain is the
already-known grammar effect, or the materialization confound, i.e. FLI is a relabelling.
**(d) All gates pass** — I regard this as least likely (~15%) and would treat it as a
mandate to re-open the novelty question with track 20 rather than as a basis for any claim
of my own.

What I am most likely to have wrong: I may be over-weighting `H_op`'s contribution to the
byte leg. If mode-15's opcode stream is already dominated by a single near-deterministic
symbol (`H_op` ≈ 0.05–0.15 B), then FLI's byte win collapses toward zero and G4 fails even
with excellent coverage. P1a settles this in one deterministic CI job and is the cheapest
high-information measurement available to this track.

---

## 13. Strongest disconfirming evidence (against FLI, gathered deliberately)

| # | Disconfirming fact | Location | Force |
|---|---|---|---|
| D1 | **Every prior ratio win in this project cost decode throughput.** Mode-6 context switching: −8.3..−16.2% bytes, decode −3..−10%. Grammar factoring of the book streams: −4..−11% bytes, decode −8..−16%. Neither adopted. | `RESEARCH_LEDGER.md` Exp. S, Exp. Y | The base rate says a *ratio-first* program-ISA mechanism is unlikely to survive. FLI's ratio claim alone would be the fourth instance of this pattern. |
| D2 | **Wire-invisible decode optimization is exhausted**; therefore any decode win must change the wire and must therefore be paid for in bytes — precisely the axis where FLI is weakest (G4 spans 0%..−7%). | `docs/swarm-2026-10-02/MASTER-BRIEF.md` item 12; ledger A9/A10/A12/A15 | Structural squeeze on the whole track. |
| D3 | **The zero-bit / implicit-parameter axis has already produced one measured dead end and one measured-unnecessary result.** ARI-STRIDE closed across two decades; PR-5 P-7 found transmitted beat derived by 15 B. | ledger PART XIII §7; `docs/gate-ruling-i9-pr5-position-derived.md` P-7 | Undermines the instinct that implicit parameters are where this track's win lives. My counter — FLI's parameters are *copied*, not *estimated* — is structurally sound but the base rate is against me. |
| D4 | **The two mechanisms closest to mine both failed on decode.** Exp. Y's grammar factorization attacked the *same* decode floor, in the *same* block, with explicit loop/rule structure, and lost 8–16% decode. | `RESEARCH_LEDGER.md` Exp. Y | The strongest specific prior *inside this repo* against any loop-structured book mechanism. |
| D5 | **Correction/token position structure does not recur.** 90% unique masks; structural-distance propagation never landed (Exp. N reinforcement failed; Exp. T SRR found span-like structure that "does not propagate"). If tokens do not recur positionally, runs do not form. | ledger Exp. I, N, T | Attacks G2/G3 at the root, not just the mask case. |
| D6 | **The block-type/distance code is already shape-conditioned** (mode 12: 28 shapes, `last[shape]`, first-absolute/reuse/delta flags) and the measured distance log-proxy is ≈9.4 bits vs ≈13 unconditional. Parameters in this architecture are already compressed by context modelling. | `FORMAT.md` §SHAPE; `docs/CONTEXT.md` distance analysis | If operands are already near-entropy-coded, removing their symbols has little left to remove. |
| D7 | **The mode-15 hot op already carries ZERO operand bits**: the book entry is `(kind, len, shape)` and `dist = last[shape]`. There are no per-iteration operand symbols on the hot path to remove — there are only *opcode* symbols. | `FORMAT.md` §HOTOP | **The most serious objection in this document, and a partial refutation of my own framing.** §5.1's byte ledger is arithmetically correct only because it charges `H_op` alone; the surrounding prose in §3 and §5 overstates what exists to remove. FLI is closer to *run-length coding of the decode stream* than to an *implicit operand algebra*. Corrected in §14 rather than quietly dropped. |
| D8 | **A same-transform reference erases apparent wins.** On `synth-arith.bin`, brotli q11 on pre-transformed bytes is 5.33x smaller than on raw bytes; that cell's apparent frontier was a control artifact. | `docs/gate-priorart-audit-i8.md` §1; ledger PART XIII §5b | Any FLI byte win measured only against mode-15/A0 rather than against a same-representation reference is void. Built into G5/A1 and the dual-bar rule. |

**How D7 changes the deliverable.** The honest hypothesis is narrower and sharper: *does a
counted loop over repeated hot-op iterations, with the operand vector re-derived by an exact
recurrence and zero entropy-coded symbols inside the body, beat (i) mode-15 expanded,
(ii) the raw side-stream budget alone, and (iii) a fused-pull grammar control, on bytes
**and** decode?* That is still worth one CI job. It is no longer a "zero-bit operand algebra",
and no downstream artifact may describe it as one.

---

## 14. Corrected claim statement (post-D7)

> **FLI** adds one instruction class to mode-15: a counted loop over repeated hot-op
> iterations whose per-iteration operand vector `(len, dist)` is re-derived by an **exact,
> encode-verified recurrence** anchored once at loop entry (`L_0` from the book, `D_0` from
> `last[shape]`, `δ_L`/`δ_D` as loop-header constants), so the loop body executes **without
> any entropy-decoded symbol and without any varint read**, with strictly bounded,
> non-recursive, allocation-free expansion and `O(1)` added decoder state.
>
> The claim is the **fusion** — fused execution + exact recurrence + entropy-free body +
> `O(1)` state + `J`-gated self-selection — **not** the counting, **not** the recurrence
> alone, and **not** the implicit operands. Implicit operands are prior art (LZMA `rep0–rep3`,
> Brotli distance cache, Brotli opcode aliasing) and, on the hot path, largely already absent
> (D7).

Sections 3–5 remain correct as written; only the *emphasis* of the claim changes.

---

## 15. INTERIM CHECKPOINT SUMMARY

### 15.1 Strongest mechanism / hypothesis

**FLI — Fused Loop Instruction.** A counted loop over repeated hot-op iterations whose operand
vector is re-derived by an exact, encode-verified recurrence, executing with zero
entropy-coded symbols and zero varint reads in the loop body; `O(1)` decoder state; hard
bounded non-recursive expansion; gated by the project's existing `J` objective.

**Falsifiable hypothesis (one sentence):** *on record-structured files the incumbent
bit-cost-gated parser emits long maximal runs of consecutive identical `(class, len, dist)`
hot-op iterations that no current instruction can express, and collapsing such runs into one
recurrence-anchored loop instruction is simultaneously byte-negative and decode-positive
relative to (i) mode-15 expanded, (ii) the raw side-stream budget alone, and (iii) a
fused-pull grammar control.*

### 15.2 Measured evidence, with exact source paths

| Claim | Exact path |
|---|---|
| G4-amended: zero-bit derived parameters only when EXACT; statistical family closed MATH-class, 0.646–0.691 bits/doubling over d = 4..1024 | `RESEARCH_LEDGER.md` PART XIII §7 (DNB-M2) |
| Bounded non-zero-residual derived parameter = buildable, not claimable; AUDIT-7 causality guard | `docs/gate-ruling-i9-pr5-position-derived.md` P-3.2, P-3.3, P-3.4 |
| Transmitted step 12,921 B < derived 12,936 B (decisive) | `docs/gate-ruling-i9-pr5-position-derived.md` P-7 |
| Transmitted parameters = prior art; "twice built the expeditious form and called it progress"; A1–A4 never executed | `docs/gate-priorart-audit-i8.md` §4, §6 action 5 |
| VCDIFF RFC 3284 / US 12,373,439 mask prior art; correction *topology* coding "NOT FOUND" | `docs/gate-priorart-audit-i8.md` §3 |
| Masks ≈90% unique; modal accuracy ≈20%; mode 13 lost +13.5..+21.0% | `RESEARCH_LEDGER.md` Exp. I |
| Mode 15 hot-op book: 1.17–1.34x decode vs fused mode-12; floor = "opcode-stream entropy decode + copy"; Linux hot coverage 75–77% | `RESEARCH_LEDGER.md` Exp. R; `FORMAT.md` §HOTOP; `docs/CONTEXT.md` |
| Mode-6 context switching: −8.3..−16.2% bytes, decode −3..−10% | `RESEARCH_LEDGER.md` Exp. S |
| RePair/RLZ on book streams: −4.15..−10.76% bytes, decode −8.1..−15.9%, encode 48–64x, eager materialization the measured root cause | `RESEARCH_LEDGER.md` Exp. Y |
| Frozen J baseline + ns/B calibration table; arbiter never ran; "right ORDER, wrong GAPS" | `RESEARCH_LEDGER.md` Exp. AA |
| Parse edge-cost model; `--parse=auto` is research-only | `FORMAT.md` §Parser |
| Decoder bounds, amplification guards, RePair/RLZ expansion caps | `FORMAT.md` §Integrity and malformed input |
| Forced registry test mandatory for every decoder-visible ID | `FORMAT.md` §Registry coverage |
| H2 decoder-derived transform sites; H3 `ITER(n, body, state)`; H1 kill conditions; depth is a first-class cost | `docs/I10-BREAKTHROUGH-PROGRAM.md` §4, §6 |
| Conditional-program references; implicit/zero-bit parameters; 2026 citations unverified | `docs/ORBIT_PROGRAM_COMPRESSION.md` |
| Synth dual bar: brotli q11 on transformed bytes 16,313 B vs 87,013 B raw | `docs/gate-priorart-audit-i8.md` §1; `RESEARCH_LEDGER.md` PART XIII §5b |
| CI measurement classes; promotion questions 1–7; reference-class policy | `docs/GITHUB-ACTIONS-BENCHMARKING.md` §2, §3, §7, §9 |
| Wire-invisible decode exhausted; G4/G5B/PORDER/lane-transpose killed | `docs/swarm-2026-10-02/MASTER-BRIEF.md` items 12, 13 |
| 2026-10-02 keyword searches: no relevant hits (coverage gap, **not** a negative) | this session; cf. `docs/gate-priorart-audit-i8.md` §5 |

**Provenance / no-splicing rule, recorded.** Two *different* measurement sessions are cited in
this document and **must not be merged into one window**:

1. the frozen Exp-AA **arbiter baseline** — `generated.log` 175,550 B / 273.534 MB/s decode /
   19.681 MB/s encode, brotli-q9 124,669 B / 619.508 MB/s / 29.533 MB/s, **median-3, bench
   arbiter**;
2. the Exp-AA **decode-cost calibration table** — median-7 ns/B, sourced from `decode-perf t3
   §6`, "real mode-15 stream content", a different session and a different statistic.

§5.2's "273.534 MB/s = 3.66 ns/B" is a unit restatement of (1) only; §5.2's "≈6.0 ns per rANS
pull" comes from (2). They are combined only to *size a projection*, never to state a measured
result. All Linux figures (75–77% hot coverage, 0.87–0.99 GB/s microbench, 452 KB, the
19.8 KB raw stream-budget trade) are **directional Linux-line numbers** and are never compared
against the frozen Windows row. The hold-out PE set `u3` is reported separately from the
discovery corpus, per master-brief item 6.

### 15.3 Complete cost model — bytes / cycles / RSS / decoder code + state

Per loop of `k` iterations, mean length `L`:

| Item | Cost | Class |
|---|---|---|
| wire, added | 1 entropy-coded opcode (`H_op`) + `uvarint n` (1–2 B) + `δ_L`/`δ_D` (0 B in `LOOP_EQ`, ≈2 B in `LOOP_ARITH`) + ≤4 new alphabet symbols (≤30 B **per block**) + 3 raw stream headers (≈12 B **per block**) | by construction |
| wire, removed | `k · H_op` — **opcode symbols only**; hot-path operand symbols already cost 0 B (D7) | by construction |
| wire, net | **`+2 B − (k−1)·H_op` per loop**; byte-neutral at `k = 1 + 2/H_op` → k ≥ 8 (H_op = 0.3 B), k ≥ 5 (0.5), k ≥ 3 (1.0); projected whole-file **0% .. −7%**, width = `H_op` | projection; becomes deterministic once P1a measures `H_op` |
| decoder cycles saved | **`≈8 ns/iteration`, L-independent** (rANS-4096 pull ≈6.0 ns from session (2) + ≈2 ns dispatch); `M(L) = max(2.0, 0.12·L)` ns copy unchanged | projection; an upper bound of the mechanism by construction |
| decoder cycles spent | loop header: 1 entropy pull + 3 varint reads + 3 bounds checks ≈20 ns **per loop**, <4 ns amortized at k = 8 | projection |
| decoder cycles, net (L=40) | 13.8 → 9.8 ns/iter (1.41x on covered iterations); at L=8, 1.57x; at L=256, 1.09x (copy-dominated) | projection |
| whole-codec decode | **HARD CEILING 1.124x** for *any* token-loop-only improvement, `1/(1−0.11)`; FLI's own projection **≤ ≈1.05x**; entropy-pull component alone **1.003x**; measured noise/instrumentation band **2.3–8.1 %** | **MEASURED ceiling + DERIVED projection — decisive, §16.2** |
| encoder cycles | `Θ(n)` before and after; P2+P3 add ≈6 comparisons and 4 registers per token; projected **+1.5%..+4%** parse time; no new index, no superlinear term | projection; must be a single linear pass (A8) |
| RSS / peak memory | **unchanged**: `+≤200 B` decoder state (7 loop registers + 3 cursors); **no** side table, **no** rule table, **no** program text, **no** temporary materialization, **no** reference chain | by construction |
| expansion bound | `≤ KITER_MAX · LEN_MAX`, additionally `≤ declared blen`; dominated by existing `1 ≤ blen ≤ block_size` and `raw_n ≤ 16·out_len + 64`. **No new amplification class**; **recursion impossible** | by construction; contrast RePair/RLZ's per-rule + cumulative + depth caps |
| decoder code size | 1 new opcode arm (≤4 escape values), 1 fused inner loop, 1 `switch` case; **no** new stream codec, **no** new entropy model, **no** new table. Estimated **+250..+450 B** Release x86-64 — **must be reported as an actual `.text` delta at R0**, never as this estimate | projection; measure at P3 |
| decoder-visible IDs added | ≤4 opcode alphabet values + 1 block-header field (`loop_class`). Triggers the mandatory forced registry test — a hard delivery condition | process |

### 15.4 The one decisive REMOTE-ONLY experiment

**R1/PR-11 — the FLI two-stage remote screen: `P1` (incumbent cost decomposition) then `P2`
(byte-only representation oracle).** GitHub Actions only, manual dispatch, per
`docs/GITHUB-ACTIONS-BENCHMARKING.md`; byte results are citation-grade deterministic, timing is
scout-grade paired same-job A/B. **No mechanism is built until both report.**

* **P1a / P1b (byte-only, deterministic):** opcode-symbol entropy `H_op` and the full decode
  category split (entropy pulls / varint reads / copy bytes / macro entries / literal bytes)
  of the frozen mode-15 output across Silesia, enwik8, PE `u3`, and the four record files.
  *Determines whether the byte leg exists at all (D7).*
* **P1c (counters, not timers):** instrumented mode-15 decode reporting entropy pulls per
  output byte and the per-iteration cost share. *Determines whether the decode leg exists (H1).*
* **P1d (byte-only):** publish the `λ · C_decode` arithmetic reconciling `FORMAT.md`
  §"Stream-codec selection objective" units with the Exp-AA ns/B table. *Determines whether the
  project's `J` gate can make a rate-vs-cycles decision at all (H5).*
* **P2 (byte-only):** run the §8 oracle; report admissible-run coverage, recurrence hit matrix,
  `Σ(k−1)`, and the B2 byte delta at `H_op ∈ {0.3, 0.5, 1.0, 1.5}`.
* **Primary cell:** the frozen `generated.log` pair from session (1). **Never** compared against
  the Linux 452 KB / 0.87–0.99 GB/s microbench figures.
* **Mandatory controls (any FAIL ⇒ run VOID):** `--fli=off` byte-identity chained to the
  freeze-HEAD binary; `repeat.jsonl` ≤ same-build exact-LZ row; `random.bin` ≈262,166 B
  unchanged; `synth-arith.bin`/`doc.md`/`src.cpp` FLI must not fire (reported, not required);
  dual-bar reporting with `{synthetic}` tags on every synth cell; full round-trip in both arms
  before any byte count; every §11 adversarial probe rejected or detected.
* **Build scratch**, when it happens, lives in `scratch/swarm-tmp/11-orbit-programs-space-bunny/`
  inside this repo. No external temp paths are requested or used.

**Preregistered KILL / PROMOTE thresholds, fixed now, before any FLI data:**

| Gate | KILL if | PROMOTE to P3 (mechanism build) if |
|---|---|---|
| G0 prior art (track 20) | any of separators S1/S2 anticipated | S1–S4 unanticipated |
| G1 decode leg sized | per-iteration entropy+dispatch < **25%** of per-iteration decode cost | ≥ 25% |
| G2 coverage | < **40%** of match bytes inside admissible runs (k ≥ 4) | ≥ 40% |
| G3 run length | median admissible run < **8** on < 3 of 4 record files | median ≥ 8 on ≥ 3 of 4 |
| G4 byte leg (B2) | byte delta positive at `H_op = 1.0 B` | ≤ **−1.0%** at `H_op = 0.5 B` and ≤ 0% at `H_op = 1.0 B` |
| G10 byte gate (λ forced to 0, per resolved H5) | primary-cell byte delta positive | byte delta ≤ −1.0% and repeat/random controls byte-flat |

G5–G9 (§10) govern P3 and cannot be waived. **A G1 FAIL is a KILL of FLI, not a downgrade to
"ratio-only"**: master-brief item 12 already retires wire-invisible decode work, and a
byte-only FLI would be the fourth instance of a pattern this project has recorded three times
(D1).

### 15.5 Leaning at checkpoint time: **PILOT** — **WITHDRAWN, SUPERSEDED BY §16.7 (KILL)**

*Retained for the record; do not act on this subsection. Its substantive errors, corrected in
§16: it called the decode cost unmeasured (it is measured — §16.1); it did not connect FLI's byte
projection to the recorded frontier gap (that connection is the kill — §16.4); it treated
`KITER_MAX` as available for ratio (it is a security parameter, pinned at 15 — §16.3); and it
rested a novelty claim on separators cross-lane review has since removed (§16.5).*

* **Not PROMOTE-TO-REMOTE.** The decode leg's size is gated on an unmeasured quantity (H1/D7)
  whose plausible range spans "material" to "≤5% whole-codec". Funding a mechanism build
  before measuring would repeat the project's characteristic failure mode.
* **Not HOLD.** P1 is nearly free and unblocks tracks 05, 06, 11 and 18 at once — the incumbent
  decoder cost decomposition is an instrument everyone needs and nobody has. P2 can return a
  decisive NO-GO in one CI job, which is a good outcome. The mechanism itself is a ~200-line
  encoder change plus one fused inner loop.
* **PILOT, with an explicit kill attached:** run P1 then P2 remotely now; build nothing yet. If
  G1 fails, disposition is **KILL**, not "adopt-class ratio work".
* **Confidence:** high (~0.8) that PILOT beats HOLD as the *process* call; **low (~0.4)** that
  FLI clears G4+G6+G7+G8, i.e. I do **not** expect this to become a promotable mechanism.
* **Post-checkpoint update (H5 resolved, adverse).** Confidence that FLI's *decode* leg is
  the binding unknown is **unchanged**; confidence in the **P1 decomposition as a
  cross-track deliverable is raised to ~0.9**, and the **H5 finding is now a confirmed
  blocker rather than a suspicion**: the project's decode-cost-weighted selection objective
  cannot, at λ = 0.01, trade even 0.01 % of a stream's bytes for decode time. That is
  mechanism-independent, needs no corpus, and is correctable only by a coordinator-level
  decision on the exchange rate. It should be reconciled alongside tracks 05/06/18 before
  any of them fund a decoder-cost-gated representation. FLI's own remaining verdict now
  rests entirely on **G2/G3 (coverage and run length)** and **G1 (per-iteration cost
  share)** — the byte leg is pure length and therefore unambiguous.

### 15.6 Explicit KILL / HOLD / PILOT ledger from this track

| Item | Disposition | Basis |
|---|---|---|
| General bounded micro-program **VM** / grammar VM over bytes | **KILL** | prior art (RePair, SEQUIT, XMill/MillView, CRUSH, LZH) **and** measured decode-negative here (M9) |
| Any **transmitted-parameter** program ISA | **KILL** | prior art; the project's own twice-repeated error (`docs/gate-priorart-audit-i8.md` §4) |
| **Estimator-derived implicit parameters** (any width, any residual bound) | **KILL** | DNB-M2 MATH-class; bounded-residual variants engineering-only and measured unnecessary (M2, M3) |
| **Mask / correction-topology cloning** | **KILL** | masks ≈90% unique; mode 13 lost 13.5–21% (M6) |
| **Zero-bit implicit operands as the novelty claim** | **KILL** | D7 + prior art (LZMA `rep0–rep3`, Brotli distance cache / opcode aliasing) |
| FLI as **ratio-only** (G4 passes, G6 fails) | **KILL — see §16.7** | after `KITER_MAX = 15` and the newly charged fixed per-block tax it closes only 3.4–19.3 % of the recorded 50,881 B gap to brotli q9 (§16.4); not a crossing instrument on any frozen cell |
| **`LOOP_ARITH`** recurrence generality | **KILL as a claim — see §16.5** | **S3 is ANTICIPATED at the abstract level** — ISLP (Jeż/Navarro/Olivares/Urbina, LATIN 2024) iteration rules with exponents linear in `i` are exactly a counted loop with an exact arithmetic operand recurrence, zero per-iteration symbols and bounded non-recursive expansion (`20-priorart-killteam-space-bunny.md` §9.2) |
| **FLI** | **KILL — see §16.7** | decode leg unresolvable against a measured 2.3–8.1 % noise band (§16.2); novelty reduced to an implementation property (§16.5) |

### 15.7 Files created by this agent (nothing else touched)

* `docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md` (this report)
* `prototypes/swarm-2026-10-02/11-orbit-programs/space-bunny/README.md`
* `prototypes/swarm-2026-10-02/11-orbit-programs/space-bunny/loop_oracle.cpp`

No existing file modified. No reset / clean / stash / restore / rebase. No commit or push. No
local benchmark, sweep, or fuzz campaign executed; the prototype is written but **not built and
not run** — its first execution is remote (§15.4 P2), per master-brief item 7.

---

## 16. FINAL RECONCILIATION — cross-lane review, security audit, corrected verdict

**This section controls and supersedes the rows named in the table at §16.0.** It reconciles
`11-orbit-programs-fledge.md` (paired adversarial audit),
`19-format-security-space-bunny.md` §10 (bounds audit),
`20-priorart-killteam-space-bunny.md` §9/§16 (prior art), and the coordinator's corrections to
both. Tags: `MEASURED` = in-repo artifact cited by path:line; `DERIVED` = arithmetic on MEASURED
inputs, shown inline; `PRIOR-ART` = literature claim inheriting its source's coverage honesty;
`SPEC-AUDIT` = specification defect, not a code defect (FLI has no code).

### 16.0 Supersession table — which earlier rows are void

| Earlier location | Status | Replaced by |
|---|---|---|
| §0 verdict paragraph ("the binding unknown is the per-token cost decomposition") | **VOID** | §16.1, §16.7 |
| §1.2 row **H1** ("per-token cost share **unmeasured**") | **VOID** | §16.1 (it is `MEASURED`, and what it measures is *not* G1's denominator) |
| §1.2 row **H5** (λ·C_decode, my own derivation) | **SUPERSEDED** | §16.8 — a stronger in-repo measurement already existed |
| §5.3 "RSS unchanged" / §5.4 ratio-bullet | **QUALIFIED** | §16.9 — guards change the ceiling's provenance, not the mechanism's fate |
| §7 separator **S4** (J-gated self-selection) | **WITHDRAWN** | §16.5 |
| §8 + §15.4 **P2** (run `loop_oracle`) | **VOID as a gate** | §16.6 — the instrument is knowingly optimistic; it must not certify G2/G3 |
| §15.3 row "whole-codec decode ≤ ≈5 % … not statable until P1c" | **VOID** | §16.2 — ceiling 1.124x, projection ≤ ≈1.05x, noise 2.3–8.1 % |
| §15.4 P1c (instrumented timing) | **VOID** | §16.2 — instrumentation perturbs 2.3–8.1 %; counters only |
| §15.5 "Current leaning: PILOT" | **WITHDRAWN** | §16.7 — KILL |
| §15.6 rows "FLI … PILOT", "LOOP_ARITH … PILOT inside FLI", "FLI ratio-only … HOLD" | **VOID** | §16.7 |
| §12.2 "Final: PILOT" | **WITHDRAWN** | §16.7 |
| §13 D7, §14 corrected claim, §15.1 hypothesis | **RETAINED** — still accurate and now load-bearing | — |

### 16.1 The evidence correction I owed, and the denominator discipline

The paired audit is **right** that H1 and §15.4 P1c were wrong to call unmeasured, and **wrong**
to read the profile as having already falsified my `G1`. Both errors were mine; the
coordinator's correction is the precise form. `MEASURED`, `RESEARCH_LEDGER.md:3911-3931`
(decode-perf t3 final; HEAD 392e937 payloads; `gen_log.anv` = 175,550 B == frozen gate baseline
exactly; QPC + invariant TSC; median-of-7 **interleaved**; warmup 2; all four containers
round-trip byte-exact; instrumented decoder verified byte-identical to
`decode_tokens_hotop_fused` on every block):

| share of end-to-end decode (`generated.log`) | value |
|---|---|
| crc32 | **44 %** (1.86–1.91 ns/B; 43.8 log / 36.4 json / 56.1 jsonl / 31.4 sqlite) |
| eager materialization of the 7 macro streams (setup **is** masks+resid) | **26 %** |
| **token loop beyond setup** | **11 %** |
| — of which **opcode entropy pulls** | **0.3 %** |
| concat / alloc / headers | ≈15 % |
| residual | ≈4 % |

Same entry, verbatim: **"THE HOT PATH IS CLEAN; the floor is everything AROUND it."** Also
`MEASURED` in the same entry: within-run CV 0.6–10 %; run-to-run drift ±2–4 %; **zero-byte-change
control variants measured +2.3–8.1 %** ("single-stream effects inside it NOT resolvable");
absolute level 5–15 % below the bench-suite protocol, **relative** deltas valid.

**Denominator discipline — what that profile does and does not identify.** The 0.3 % isolates
**entropy pulls**. It does **not** isolate per-token **dispatch**, **state update**, or the
**copy**. The 11 % is the **whole token loop**, and it includes both hot-op and macro-path
iterations. Therefore:

* `G1` as I wrote it — "entropy+dispatch ≥ 25 % of per-iteration decode cost" — is **not
  falsified** by this profile; it is **unidentifiable** from it. The 2.7 % the audit computes
  (0.3 / 11) is a ratio of *pull* to *whole loop*, which is not the ratio `G1` asks for.
  **My gate was badly posed.** `DERIVED`.
* The decision-relevant quantity needs no decomposition at all: **removing 100 % of the token
  loop — including the copy FLI preserves — yields 1/(1 − 0.11) = 1.1236x.** Any token-loop-only
  mechanism is capped at **≈1.124x**. `DERIVED`.
* A second bound cuts the other way and I record it for honesty: if hot-op iterations are ~75 %
  of tokens (`RESEARCH_LEDGER.md` Exp. R's 75–77 % hot coverage, a **stale-flagged Linux** figure
  and therefore admissible only as an order-of-magnitude hint) and ~95 % of the loop's iterations,
  FLI's true pool is ≈ **10 %** of decode, giving **1.111x** instead of 1.124x. Both are derived
  from `MEASURED` inputs; neither rescues `G6`.

### 16.2 Decode leg: the corrected arithmetic, and why the leg is dead

| Bound | Value | Basis |
|---|---|---|
| Entropy-pull-only upside (FLI's own named thesis) | **1.003x** | 0.3 % of decode, `MEASURED` |
| FLI's own projection (≈8 ns/iter of pull+dispatch, §5.2) | **≤ ≈1.05x** | `DERIVED` on §5.2/§5.3 |
| Hard ceiling if the *entire* token loop vanished (impossible — FLI keeps the copy) | **1.124x** | `DERIVED` from 11 % |
| `G6` requirement (≥ **1.08x**, paired CI excluding 1.00) | needs 1 − 1/1.08 = **7.41 %** of total decode = **67 % of the whole 11 % pool** | `DERIVED` |
| Measured instrument noise / instrumentation shift | **+2.3–8.1 %**; drift **±2–4 %** | `MEASURED`, same entry |

**Reading, on the corrected basis.** `G6` is not *arithmetically excluded* by the 11 % pool —
that is the audit's over-strong form and I decline it. It is **unresolvable**: FLI's attainable
range runs from 1.003x (pull-only, measured) to 1.124x (an upper bound requiring deletion of the
copy), and that range **straddles the instrument's noise band of 2.3–8.1 %**. The central
estimate (≈1.05x) sits **inside** it. A mechanism whose expected effect is smaller than the floor
of its own measurement protocol cannot be gated by that protocol, and no amount of remote
repetition fixes it: the noise is a property of the harness and the shared runner, not of the
sample size.

Two independent confirmations that the decode leg was never the right place to spend:

1. **The other 89 % is already owned elsewhere.** crc32 44 % has a wire-invisible fix that exists
   and was scoped *out* of S6-1 into a separate pre-registration (the ledger names a slice-by-8 or
   PCLMUL path). Macro-materialization 26 % and concat/alloc 15 % are the S6-1 / ALLOC lanes.
   **A mechanism that spends a track on 11 % while 89 % sits in three identified, separately
   owned buckets is misallocated by construction.**
2. **FLI does not remove the copy**, which is the co-equal named floor together with entropy
   decode (`RESEARCH_LEDGER.md:1452-1478`, Exp. R). FLI's true pool is therefore *strictly
   smaller* than 11 %, and the smaller it is, the deeper inside the noise band it sits.

**Track 18's methodological finding is adopted and folded in.** Zero-byte-change *instrumented*
controls move decode **+2.3–8.1 %** — the instrument is a larger perturbation than any effect
FLI could claim. Therefore any timing evidence in this neighbourhood must come from a **separate
uninstrumented paired build**, identical wire, with an **A/A null on that build**; instrumentation
is admissible **only** for counters and causal shares, never for timing. This supersedes §15.4
P1c as written.

### 16.3 Security audit: disposition of every Track 19 finding

`SPEC-AUDIT` throughout; FLI has no code, so "defect" means *the specification does not require
the guard*. Bounds are now **pinned from the amplification ceiling, before any run-length data is
seen** — the only doctrine-5-clean order.

| ID | Finding | Disposition |
|---|---|---|
| **MG-7** | `KITER_MAX`/`LEN_MAX` occur only as symbols, no numeric value anywhere; the track's own probes are unrunnable | **REPAIRED.** Pinned now, pre-data: **`KITER_MAX = 15`**, **`LEN_MAX = 65536`** (`kSparseMaxLen` — the only defensible value, since the book `len` is already validated to `[1, 65536]`, `src/anvil.cpp:3004`). Derivation: work-per-input-byte `= KITER_MAX × 65536 / 3`; at 15 that is **3.3×10⁵×**, on par with (not above) the existing RLZ path's ≈3.4×10⁵×. `KITER_MAX` is a **security parameter**, never a ratio knob. |
| **MG-1** | Addition-form guard `pos + L0 ≤ block_end` wraps exactly when `L0` drifts; the codebase uses the subtraction form throughout | **REPAIRED (spec).** Mandate **`L_i ≤ block_end − pos`**, every iteration, both opcodes. |
| **MG-2** | No typing/width/cap/post-check on `δL`/`δD`; the house zigzag applies **no cap** to `zz` (`src/anvil.cpp:2947`), so `zz = UINT64_MAX` wraps and the delta decodes as 0 | **REPAIRED (spec).** Cap the varint pre-decode (`≥ 0xFFFFFFFF` ⇒ reject); accumulate in **`int64_t`**; post-check **both** sides every iteration (`D_i ∈ [1, 0xFFFFFFFF]`, `L_i ∈ [1, LEN_MAX]`); re-validate before use. FLI must not rely on the house zigzag. |
| **MG-3** | `δL = −1` drives `L0` to 0, leaving `KITER_MAX − L0` dispatch-only iterations | **REPAIRED (spec).** `1 ≤ L_i ≤ LEN_MAX` **every** iteration. With the pin this is a 15-iteration bound, so it is no longer the primary DoS vector. |
| **MG-4** | Three new substreams not added to the block's full-consumption contract ⇒ **byte laundering** past the J accounting; breaks the format-evolution invariant | **REPAIRED (spec), and it would have been a wire-landing precondition.** `S_n`/`S_dl`/`S_dd` cursors must join the existing consumption tuple (`src/anvil.cpp:2979-2981`) **and** the `payload trailing bytes` check (`:2911`). A doctrine-2 breach, not merely a bug. |
| **MG-5** | `K` is a per-block uvarint (`:2994`) capped `kHotMaxOps = 254` (`:2797`) and the opcode stream is a **byte** stream (`:2915`) ⇒ `K+4` is unrepresentable for `K > 251` | **REPAIRED (spec), at a cost I had omitted.** Mandate: **reject any block declaring loop opcodes when `K > 251`**; do **not** lower `kHotMaxOps` (that shrinks *incumbent* book capacity, i.e. changes the baseline). **Newly priced:** FLI is then silently unavailable exactly in the largest-book blocks — those with the most hot classes and plausibly the longest runs. My A12 was **wrong**: "K ≤ 254", "adding ≤4 loop codes" and "bounded" cannot all be true simultaneously. |
| **MG-6** | Escape-class bound must apply **post-accumulation**, not per escape byte | **REPAIRED (spec)** plus a chained-escape probe. |
| **MG-8** | Canonical-varint checking "does not exist anywhere in ANVIL"; `get_uvar` accepts overlong encodings | **CORRECTED.** My A3 claimed a mitigation no implementation provides. Restate A3 as the checks that exist (`n ≥ 2`, `n ≤ 15`, `get_uvar`'s own `p < e` and 10-iteration bounds), or implement canonical form **and** pay for it in the header ledger. |
| **MG-9** | The overlap path must be re-evaluated per iteration (`L_i` drifts); **`memcpy` forbidden when `D0 < L_i`** (overlap ⇒ undefined behaviour) | **REPAIRED (spec).** Two-path split per iteration; index safety rests only on `D_i ≤ pos` being checked **first**. |
| **ρ row** | FLI must be metered in the resource-meter registry at the same rate as the tokens it replaces, or it becomes the format's cheapest amplification primitive | **ACCEPTED as a wire-landing precondition.** With the pin it is bounded at 15 × 65536 ≈ 0.98 MB per instruction — ~15× any incumbent token — so the row is still required. |

**Net:** all nine are specification repairs, cheap now and expensive later. **They are not what
kills FLI.** Repairs make a mechanism buildable; they do not create one. What kills it is
§16.2 (decode), §16.4 (bytes) and §16.5 (novelty).

### 16.4 Byte economics under `KITER_MAX = 15`, the fixed tax, and the MG-5 cliff

| Item | Value | Class |
|---|---|---|
| Byte-neutrality threshold `k ≥ 1 + 2/H_op` | k ∈ **[3, 8]** across the whole `H_op ∈ [0.3, 1.5]` band | `DERIVED` |
| Effect of the cap on the operative band | **none** — 15 > 8. Track 19's "largely dissolves the mechanism" is rhetoric; I side with the paired audit against Track 19 on this point | `DERIVED` |
| Truncation cost above 15 | a run of length `k` costs `⌈k/15⌉` loops, each seam ≈ `H_op + 3` B; at `k = 40`, 3 seams ≈ 10.5 B ⇒ **0.26 B/iter** against a 0.5 B/iter saving ⇒ ~50 % loss on long runs, **0 % on runs 8–15** | `DERIVED` |
| **Fixed per-block cost, newly charged** | 1,942,280 / 65,536 = 29.6 ⇒ **30 blocks**; stream headers 12–24 B + `loop_class` 1–2 B + alphabet ⇒ **≈0.7–1.7 KB on 175,550 B = 0.4–1.0 %**. My §5.1 charged per-loop terms only and stated the alphabet cost as "≤30 B/block" **without amortizing it**; under doctrine 2 that is a defect in my ledger | `DERIVED` |
| Net byte win, mid-band `H_op = 0.5` | −2.0 % ⇒ 3,511 B, **minus** 0.4–1.0 % tax ⇒ **net 1,755–2,809 B** | `DERIVED` |
| Net byte win, optimistic `H_op = 1.0` | −6.6 % ⇒ 11,586 B, minus 1,756 B ⇒ **net ≈ 9,830 B** | `DERIVED` |
| Recorded gap to brotli q9, frozen cell | 175,550 − 124,669 = **50,881 B** ("starts 41 % BEHIND q9") | `MEASURED`, `RESEARCH_LEDGER.md:3828-3830, 3845` |
| **Share of the gap FLI can close** | **3.4 % – 19.3 %** | `DERIVED` |
| Canonical frontier state | `33 non-dominated | 5 FRONT-GAP | 0 FRONT-CROSSING | 28 DEGENERATE | 435/468 dominated`, `GRID-THIN` binding | `MEASURED`, `docs/anvil-i9-findings.md:189-213` |

**Reading.** Even at the most optimistic point of its own hypothesis band, and after paying a
fixed tax I had not charged, FLI closes **under a fifth of the recorded deficit** on the cell it
was designed for. No cell in the frozen tuple makes a 3–19 % move decisive. **FLI is not a
crossing instrument**, so under R-2/R-3 semantics it cannot produce frontier language under any
outcome — which removes the only reason the project's gate structure exists.

### 16.5 Novelty: S3 removed, S4 withdrawn, S2 is an implementation property

`PRIOR-ART`, inheriting the coverage honesty of
`docs/swarm-2026-10-02/20-priorart-killteam-space-bunny.md` §9/§16:

* **S3 — ANTICIPATED. Removed from the claim entirely.** ISLP (Jeż, Navarro, Olivares, Urbina,
  *Iterated Straight-Line Programs*, LATIN 2024) iteration rules
  `A → ∏_{i=k1}^{k2} B_i^{c1}···B_t^{ct}` with exponents **linear in `i`** are a counted loop whose
  per-iteration operands come from an exact arithmetic recurrence, with zero per-iteration
  transmitted symbols and hard bounded non-recursive expansion. That is `LOOP_ARITH` stated as
  published grammar compression. The LZ-token-stream instantiation differentiates *engineering*,
  not mechanism.
* **S4 — WITHDRAWN**, twice over: my own H5 result, and then Brotli's bits-vs-decode-speed
  representation gating. Restated at most as "per-construct decode-cost gating is unclaimed" —
  and unreachable for ANVIL anyway, since `J` is a length objective (`MEASURED`,
  `RESEARCH_LEDGER.md:4002-4011`).
* **S1 — UNRESOLVED GAP.** Not anticipated in anything reached; but "not retrieved" is a gap, not
  a clearance (`docs/gate-priorart-audit-i8.md` §5), no patent database was queried, and the
  unsearched territory (proprietary packers, console/embedded crunchers, the publication
  blackout) is explicitly flagged unsearched.
* **S2 — the only survivor, and it is an implementation property, so it cannot carry a mechanism
  claim.** I accept the audit's argument, which is also the project's own standard:
  materialize-vs-fuse changes memory traffic, not mathematics — the same instruction stream
  decodes identically either way — and the repo's verdict on the one measured instance classifies
  exactly this effect as **"implementation-era … not math"** (`RESEARCH_LEDGER.md:3778-3781`).
  Two concessions I owe: (i) the coordinator's update that compressed-domain non-materializing
  grammar computation **is** published (GPU engines computing directly on compressed grammars;
  random access to grammar-compressed strings without materializing) and that streaming grammar
  compression was **explicitly posed as an open problem in 2014** — I should cite that
  voluntarily rather than omit an on-point prior open-problem statement; (ii) my own §7 S2 bullet
  cited **Exp. Y — this repo's negative — as proof that fusion matters**, which is evidence *for*
  adopt-class value and *against* mechanism novelty. The bullet was self-undermining as a
  separator.
* **Zero-bit operand inheritance is occupied regardless**: LZMA `rep0–rep3`, zstd repeat-offsets,
  Brotli's distance cache, and LZO's documented cross-instruction `state`.

**Net: adopt-class at best, and `G0` as I worded it ("Track 20 reports S1–S4 unanticipated")
cannot be recorded as passed.** A gate that cannot be passed is a reason not to fund, not a
reason to wait.

### 16.6 Gates re-tallied on existing data, and the smaller thing that survives

| Gate | Status on data already in hand |
|---|---|
| **G0** prior art | **CANNOT PASS.** S3 anticipated; S4 withdrawn; S1 a gap. |
| **G1** decode thesis sized | **INVALIDLY POSED** (unidentifiable denominator) and **moot**: G6 dominates. |
| **G6** decode ≥ 1.08x | **UNRESOLVABLE.** Attainable 1.003x–1.124x against a 2.3–8.1 % noise band; centre ≈1.05x inside it. |
| **G4** byte ≤ −1.0 % at `H_op = 0.5` | **Cannot reach significance.** Closes 3.4–19.3 % of the recorded gap after tax; not a crossing instrument on any frozen cell. |
| **G2/G3** coverage / run length | **INVALID AS WRITTEN.** The only specified instrument (`loop_oracle`, §8) is a greedy min-match-4 trace whose own README calls its coverage "an **optimistic bound** relative to what the production parse would admit". A threshold certified by a knowingly optimistic instrument is not a threshold. E6's 75–77 % *token* hot coverage is a different, non-commensurable quantity. |
| **G10** byte gate | **Unrunnable as a scientific test** — with `J` a length objective it can only confirm that a length gate is a length gate. |
| **Bounds** (MG-7) | **Now pinned pre-data** at `KITER_MAX = 15`, `LEN_MAX = 65536`. Repaired. |

**The smaller thing that survives, isolated as a different question.** FLI's byte win exists
because `k` consecutive tokens are identical, so their `k` opcode symbols are redundant — and **a
redundancy in a coded symbol stream is a job for the entropy model, not for an ISA.** The
zero-ISA alternative is an **order-1 context on the existing opcode stream** ("is this opcode
equal to its previous opcode?"): **0 new opcodes, 0 new substreams, 0 block-header change, 1
cached symbol of new decoder state, no amplification-class change, no format-evolution impact,
+0 decoder binary**, using stream mode 6's shipped context-switched machinery (Exp. S: the
project's largest single measured ratio win, −8.3 %…−16.2 %, decode −3 %…−10 %). It is
**adopt-class by construction and is not a novelty claim.**

I record it, name the one number that decides it, and **do not build it**: the number is
`H(same | same)` of the existing opcode stream (with `H(same | diff)` reported so the model's
loss is visible), obtainable byte-only and deterministically as a two-line extension of the
opcode-entropy pass; and it must be compared against FLI's byte delta *after* the §16.4 tax and
the 15-cap. Ownership: this is a **stream-coding** change, not an orbit/program change — **route
it to the stream-suite owner (track 06 / arch lane), not to track 11**, and do not let two lanes
build the same harness (audit FM-10). Timing, if it is ever measured, follows §16.2's
uninstrumented-build rule.

### 16.7 ONE FINAL VERDICT

> ## **KILL** — the FLI bounded reversible micro-program ISA.
>
> **KILL as a novelty-bearing mechanism.** S3 is anticipated; S4 is withdrawn; S1 is a gap; the
> surviving S2 is an implementation property, and the repo's own standard calls that
> "implementation-era, not math".
>
> **KILL as a decode-leg mechanism.** The hard ceiling for *any* token-loop-only change is
> 1.124x (≈1.111x once hot-op iterations are discounted from the 11 % pool); FLI's range is
> 1.003x–1.05x, inside a measured 2.3–8.1 % instrument noise band; and 89 % of decode sits in
> crc32 / macro-materialization / allocation, which are separately owned lanes.
>
> **KILL as a ratio-only adopt-class mechanism.** It closes 3.4–19.3 % of the recorded 50,881 B
> gap to brotli q9 after its own fixed per-block tax, and no cell in the frozen
> `33 | 5 | 0 | 28` tuple makes that decisive.
>
> **KILL as an orbit/program contribution.** All five family directions in this lane — bounded
> micro-program VM, transmitted-parameter ISA, estimator-derived implicit parameters, mask /
> topology cloning, grammar/Orbit search — are closed (§15.6).
>
> **Declining PILOT and PROMOTE-TO-REMOTE.** I withdraw my own §12.2/§15.5 PILOT. All nine
> security findings are recorded as specification repairs and the bounds are pinned pre-data
> (`KITER_MAX = 15`, `LEN_MAX = 65536`), so this track closes **with its guard design on the
> record** even though the mechanism does not survive — that guard list is reusable by any future
> token-loop or grammar-adjacent proposal.
>
> **One thing is isolated, not killed:** the zero-ISA **opcode order-1 context** question,
> **adopt-class, not a claim**, routed to the stream-coding owner (§16.6). Its single decisive
> number is `H(same | same)` on the existing opcode stream, byte-only and deterministic.

### 16.8 Correction to my own §15 H5 finding — it was already measured

My §15 H5 derivation (from `FORMAT.md` §S6-1's implemented mechanics, `C_us = raw_n ·
ns_per_byte / 1000`, λ = 0.01 B/μs ⇒ 1.2 B on a 20 KB stream) is **superseded by a stronger
in-repo measurement I should have found first**: `RESEARCH_LEDGER.md:4002-4011`, machine-verified
by `research-gate`. At λ = 0.01 B/μs with the t3 costs, the decode term spans **≈0.02–0.66 B on
10–20 KB streams** — it can decide only near-exact ties; the Linux trade's implied
willingness-to-pay was **460.2 B/μs, 4.66 orders of magnitude above λ**; the nearest candidate
(literals) is short by ≈4,462× and the first candidates appear only at **λ ≈ 44.6 B/μs**; and
**zero raw-flips were arithmetically certain**. My derivation agrees in direction and order of
magnitude and I record it as corroboration only.

**Standing, mechanism-independent, reusable across tracks 05, 06, 11 and 18:** the
decode-cost-weighted objective `J` is, at the frozen λ = 0.01 B/μs, a **length** objective. Any
lane proposing a "rate-vs-cycles" gate must first escalate the exchange rate as a
coordinator-level decision — it cannot be re-derived inside a track without a doctrine-5 breach.

### 16.9 Standing methodological corrections this track contributes

1. **Instrumentation is a larger perturbation than the effects being measured.** Zero-byte-change
   instrumented controls move decode **+2.3–8.1 %**; run-to-run drift is ±2–4 %; within-run CV
   0.6–10 % (`RESEARCH_LEDGER.md:3911-3931`). Timing evidence for these lanes must come from a
   **separate uninstrumented paired build** with an **A/A null on that build**; instrumentation is
   admissible for counters and causal shares only. (`docs/GITHUB-ACTIONS-BENCHMARKING.md` §2
   already classes MB/s as scout-grade; this quantifies the instrument's own bias.)
2. **The hot path is 11 %.** Decode is crc32 44 %, macro-stream materialization 26 %, token loop
   11 %, concat/alloc/headers 15 %, residual 4 %. Any future proposal aimed at the token loop is
   misallocated against three identified, separately owned buckets — CRC (a wire-invisible
   slice-by-8/PCLMUL path already exists and was scoped out of S6-1), macro-stream
   materialization, and the allocation path.
3. **Guard-list transfer.** The nine repaired specifications in §16.3 are reusable by any future
   token-loop, grammar-adjacent or iterated-span proposal without re-audit: subtraction-form
   block-end guard; capped zigzag with `int64_t` drift and two-sided post-checks; per-iteration
   `1 ≤ L_i ≤ LEN_MAX`; full-consumption extension for every new substream plus the trailing-byte
   check; byte-alphabet representability (`K > 251`); post-accumulation escape bound; periodic-vs-
   bulk split re-evaluated per iteration with `memcpy` forbidden on overlap; a `ρ` resource-meter
   row at parity with the tokens replaced.
4. **Do not certify a threshold with a knowingly optimistic instrument.** `loop_oracle` is retained
   but must not be used to certify coverage; any coverage claim requires a **production-parse-trace**
   instrument.