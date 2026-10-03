# Track 14 — Executable/Binary Representation Compiler — Fledge Alpha Free

**Date:** 2026-10-02
**Agent:** Fledge Alpha Free (independent adversarial reviewer; paired with Space Bunny Free, constructive)
**Track:** 14 — executable/binary representation compiler: relocations, instruction fields, sections, exact transforms without BCJ cloning.
**Status:** FINAL (rev. 2 — reconciled against Track 20's BCJ claim chart and Track 20's independent critic verification, and against `COORDINATOR-STATE.md` D15/D17).
**Worktree discipline:** no existing file modified; no reset/clean/stash/restore/rebase; no commit/push; no local compression or throughput benchmark; no fuzz campaign. `prototypes/swarm-2026-10-02/14-executable-compiler/fledge/` created and left **empty** (§11).

---

## 0. Verdict, and how it changed

**Rev. 1 → rev. 2 in one sentence:** my rev. 1 HOLD was correct on *logic* — the coverage scan does not bound the mechanism family — but the survivor that refutation protected has since been closed on prior-art grounds by Track 20. **The kill moved; it did not disappear.** Track 14 constructive's KILL was right, for a reason it did not have and I did not supply.

| object | rev. 1 | **rev. 2 (final)** | basis |
|---|---|---|---|
| **RSC** — contiguous, zero-residual, derived-set, zero-bit transform | KILL | **KILL** (agree) | measured 0.0107%; structurally a stricter special case of already-closed PNRA |
| **H2 / C1** — reference-local rel32 normalization, masked, per-phrase | HOLD | **KILL as novelty — CLOSED, adopt-class "localized BCJ"** | Track 20 claim chart M-8 (Axis 5 only) + `lzma_options_bcj::start_offset` shipped-parameter evidence + Track 20 critic K1/K8/K9 |
| **H2 / C2** — ABS32 `Δ=+d`, distance-anchored | HOLD | **CORPUS-BLOCKED** (Track 20 §3.1, §8): the only structurally unoccupied residue cannot be built — no locked held-out family, and it overlaps Courgette | Track 20 critic §3.1, §8; coordinator D17 |
| **Global BCJ / x86 `--filters=x86`** as an ANVIL-side adopt-class pre-transform | mandatory control | **RETAINED — the only executable work worth keeping** | in-repo measured 0.0–7.9% on the locked PE set (§4.5) |
| **Mask-granularity defect in shipped mode 14** (§5.1) | headline engineering finding | **RETAINED as `{engineering}` / attribution only** — no novelty attaches | derived from `src/anvil.cpp:2625`, `:2714` |

**Track 14 lane-level ruling: KILL** the executable-compiler mechanism line; **RETAIN one adopt-class engineering control** (§8).

---

## 1. Evidence map (independently reconstructed)

Legend: **[M]** measured by me from in-repo artifacts · **[L]** recorded project fact with citation · **[H]** projection.

### 1.1 [M] The rel32 field population is 100–1000× the scanned pool

Single linear byte pass over the locked corpus PEs (`tests/corpus/*.exe`), counting `0xE8`/`0xE9` opcodes. No compression, no timing, no held-out data.

| file | bytes | E8 | E9 | sum | % of file | rel32-field upper bound (5 B/site) |
|---|---:|---:|---:|---:|---:|---:|
| `anvil.exe` | 268,800 | 3,216 | 783 | 3,999 | 1.49% | ~7.4% |
| `anvil_bench.exe` | 1,929,216 | 10,641 | 8,209 | 18,850 | 0.98% | ~4.9% |
| `pe-git.exe` | 4,383,048 | 76,616 | 25,304 | 101,920 | 2.33% | ~11.6% |
| `pe-ninja.exe` | 603,648 | 9,086 | 2,443 | 11,529 | 1.91% | ~9.5% |
| `pe-notepad.exe` | 360,448 | 2,226 | 1,053 | 3,279 | 0.91% | ~5.2% |
| `pe-python.exe` | 103,704 | 373 | 159 | 532 | 0.51% | ~2.6% |
| `pe-where.exe` | 61,440 | 681 | 126 | 807 | 1.31% | ~6.6% |
| `pe-winver.exe` | 28,672 | 26 | 6 | 32 | 0.11% | ~0.6% |

**Conservative reading.** An **upper bound** — opcode bytes are also produced by displacements, immediates and data. But it is an upper bound in the safe direction, and it is two to three orders of magnitude above the constructive scan's 0.0107% / 0.0040%. **The fields exist; they are not the problem.**

Independent cross-check: the constructive's own favourable input and Experiment U's `g_pnra_gate` figure for `anvil.exe` is 3,999 — **identical to my E8+E9 count**. Two unrelated scans, same population, corpus intact.

### 1.2 [M] The scan's 4-aligned-slot rule is misaligned with the fields it purports to bound

The constructive probe searches 4-byte-aligned LE32 slots. Its `.text` ≈ 0 is structural, for two independent reasons:

1. **Alignment.** In x86-64 a `rel32` immediate sits at `(opcode address) + 1` (optionally behind prefixes). Its alignment mod 4 is a pseudorandom function of the opcode's own address; requiring 4-alignment *relative to phrase start* captures a field only on a matching residue.
2. **Contiguity.** A run requires *every* intervening aligned slot to satisfy the relation. With a `rel32` in roughly one in five instructions, runs of length ≥ 2 are close to arithmetically impossible.

The paired report says runs appear in `.rsrc` "almost exclusively" — so **the reported 0.0107% is a measurement about resource-directory tables, not about code.**

### 1.3 [L] The denominator that governs the mechanism is the one the scan did not use

`docs/CONTEXT.md` ELF anatomy (propagated to `research-agenda.md` I2-5): ~81% of sampled `.text` approximate-repeat candidates contain ≥2 32-bit fields differing by exactly −distance; those fields explain **~46% of mismatch bytes**; a single repeated 32-bit Δ explains ~59% of `.text` mismatch bytes.

Against the **mismatch-byte** denominator the mechanism's coverage is 46–59%. Against the **file-byte** denominator it collapses, because exact LZ already explains most of `.text`. Both are true; they are not interchangeable. Track 14 constructive quotes the first while scanning under the second.

**Rev. 2 note.** This observation is *why the mechanism deserved a real control rather than a scan-derived closure* — and it remains correct. It no longer supports a survivor, because §3 establishes the mechanism is occupied regardless of how large its pool is.

### 1.4 [M] Two frozen in-repo controls that size the lane

`tests/benchmark-suite.c70179ea.plus-xz.csv` + `tests/xz-reference-i9.csv`:

| codec | `anvil.exe` | ratio | `anvil_bench.exe` | ratio |
|---|---:|---:|---:|---:|
| anvil-mdl-rans (best ANVIL here) | 107,468 | 0.400 | 830,331 | 0.430 |
| anvil-tcopy-rans (mode 14) | 108,518 | 0.404 | 845,675 | 0.438 |
| anvil-tcopy-pnra-rans | 108,514 | 0.404 | 846,050 | 0.439 |
| brotli-q4 | 104,655 | 0.389 | 834,690 | 0.433 |
| brotli-q6 | 97,138 | 0.361 | 754,571 | 0.391 |
| brotli-q11 | 88,322 | 0.329 | 646,675 | 0.335 |
| zstd-19 | 93,403 | 0.347 | 705,242 | 0.366 |
| **xz-9e (BCJ-enabled)** | **86,652** | **0.3224** | **632,472** | **0.3278** |

- **TCOPY is outranked inside ANVIL**: mode 14 (`0.404`/`0.438`) is worse than the simpler mode-17/mdl parse (`0.400`/`0.430`). A mechanism losing to a simpler representation of the same file on the same backend has a representation problem independent of novelty.
- **The lane's real gap is 24.0% / 31.3%, not 0.6%.** Any executable proposal must be sized against that.

### 1.5 [M] Global x86 BCJ is worth 0.0–7.9% on this exact corpus — the adopt-class prize

`tests/xz-transform-controls-i9.csv` vs `tests/xz-reference-i9.csv`, same file, same xz build, `--filters=x86 lzma2:preset=9e` vs default chain:

| file | xz-9e default | + `--filters=x86` | Δ |
|---|---:|---:|---:|
| `pe-git.exe` | 1,778,240 (0.4057) | 1,637,188 (0.3735) | **−7.93%** |
| `pe-ninja.exe` | 247,312 (0.4097) | 230,696 (0.3822) | **−6.72%** |
| `pe-where.exe` | 19,696 (0.3206) | 18,660 (0.3037) | **−5.26%** |
| `pe-notepad.exe` | 183,756 (0.5098) | 181,724 (0.5042) | −1.10% |
| `pe-winver.exe` | 5,008 (0.1747) | 5,000 (0.1744) | −0.16% |
| `pe-python.exe` | 50,352 | 50,352 | 0.00% |

**This is the one large, cheap, already-prior-arted number in the whole lane.** It is adopt-class, not novelty — and it is toolchain-heterogeneous (7.9% on MinGW/GCC, 0% on `pe-python.exe`), so any adoption decision must declare the spread rather than average it.

### 1.6 [L] Provenance defects in the executable evidence chain

1. **The Linux ELF density leg is not reproducible here.** `docs/CONTEXT.md`'s `1,761,776 B vs q4 1,781,130 B` comes from a different tree/host/corpus (ELF `.text` only). Experiment U finding 1 (`RESEARCH_LEDGER.md:1676–1696`) records the PNRA prototypes `#include "tcopy_flat_hot_lib.inc"`, a header present in **no commit of this repository**. The lane's most favourable number cannot currently be re-derived. Historical only; never a gate input.
2. **The constructive scan's aggregate omits 3 of 8 corpus PEs** — `pe-git.exe` (4.38 MB, the only MinGW/GCC binary), `pe-ninja.exe`, `anvil_bench.exe`. 823,264 of ~7.6 MB, with no stated selection rule. The largest and most structurally distinct file was never scanned.
3. **Statistical power is negligible.** 14 runs across nine files. "Median run 2 slots" is a median of 14.
4. **Coverage and ratio must not be cited adjacently** — different hosts, harnesses, corpora, and different populations.

---

## 2. Attack on the paired report's structural claim ("the granularity trap")

Track 14 constructive §3:

> Exactness (zero residual) forces contiguity… Consequently **no exact, zero-bit, derived transform is ever the correct granularity** for position-relative relocation fields. Any mechanism in this family must either transmit a sparse field set, or adopt a dense rule that does not hold.

**Correct as a cost statement; it is not a coverage result, and it was over-extended.**

- **(a)** "You must transmit a sparse field set" is a claim about **bits**, not about whether the fields exist. §1.1 shows 1–2.3% opcode density, 100–1000× the scanned pool.
- **(b)** TCOPY is **not** zero-residual. The shipped decoder (`src/anvil.cpp:2706–2758`) copies → rewrites transform fields → applies residual mask → applies residual bytes. Residuals are fully supported and dominant. Exactness-with-zero-residual is a property RSC *chose*.
- **(c)** **Rev. 2 supersession.** The claim's own nominated survivor — a decoder-side instruction-length-derived sparse mask — is *precisely* what Track 20 records as Courgette's disassembler plus BCJ, and what `docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H2 already forbids claiming. **The "non-empty" branch of the argument leads into occupied art.** So the trap argument no longer needs to carry the lane: the lane is closed on prior art, and the trap remains a useful transferable rule for *other* zero-bit-transform candidates.

**Verdict: valid, portable, over-extended.** Record as *"zero-bit exact transforms over position-relative relocation fields have a near-zero ceiling at contiguous granularity — check the derived-set density before building."* Do not record as a family closure.

---

## 3. Track 20's BCJ claim chart, and what it settles

### 3.1 What the chart establishes (Track 20 constructive, `20-priorart-killteam-space-bunny.md`)

- **M-8 (H2 decoder-derived relocation transform copy)** maps to **Axis 5 only** — *"the kernel is a restricted BCJ pass."* Status `LIVE-BUT-UNTESTED` at the time of writing.
- **F1 is rated "fatal to the whole surviving space":** *"The decode action is identical; only the trigger and the parameter source differ."*
- **The decisive document is liblzma's own field documentation.** Verbatim (`lzma_options_bcj::start_offset`): *"This setting is useful only when the same filter is used separately for multiple sections of the same executable file… it is beneficial to set the start offset of the non-first sections so that the relative addresses of the cross-section branch/call/jump instructions will use the same absolute addresses as in the first section."* Plus the decoder kernel source: BCJ's state is `pos`, documented as absolute position, with `x86_prev_mask` and the per-ISA alignment/look-ahead table.
- **The derivation:** applying BCJ at `pos = source_position` to the destination is `bcj` evaluated at a different position — the same arithmetic with a different available register. The "geometry-derived Δ=−d" is **BCJ composed with a position substitution**, and `start_offset` shows position-substitution is a *shipped, documented parameter of the public-domain filter*.
- The same chart marks **M-13 (TCOPY/PNRA, exact implicit Δ, zero residual)** `LIVE-NARROW`, gated on an additional independent full-text pass.

### 3.2 What Track 20's independent critic adds

- **K1 (BCJ relative→absolute rewrite): OCCUPIED.** Public domain since 2008.
- **K8 (computed, non-transmitted transform topology): "occupied in effect by K1"** — BCJ's topology is content-determined and computed, never transmitted. Only the anchor differs.
- **K9 (per-phrase localization of a global pre-LZ filter): ENGINEERING — "locality vs whole-file… engineering placement, not a new genus."**
- **§3.1:** global BCJ *already* achieves the exact rate property H2 claims — copied instruction templates match with no correction mask and no parameter bits — with a simpler decoder, **zero** charged side information, and no dependence on the match parser finding the phrase. *"The only thing SLX-REF adds is the parser-locality of the anchor. That is the entire novelty residue, and it is a relocation, not a new mechanism."*
- **C2 (ABS32, Δ=+d) is the only structurally unreachable case**, because its anchor is the reference distance, which no whole-file pass can see — but C2 is **blocked on a locked held-out corpus that does not exist** and overlaps Courgette's ABS32 handling. *"So C2 — the only defensible residue — is currently unbuildable, and the promotable scope is exactly the part that is occupied."*
- **T1/T2 thresholds:** `μ < 8.4%` on ≥3/6 held-out PEs ⇒ KILL on rate; `C2`-eligible bytes `= 0` on all ⇒ **novelty KILL, keep the engineering measurement.**

### 3.3 My independent confirmation, and one place I go further

I concur with the closure. Three of my own findings *reinforce* it rather than rest on it:

1. **The shipped mode 14 is ISA-blind** (`src/anvil.cpp:594–604`): the field test is `t32 == s32 - dist` on any 4-aligned window. No opcode check, no section check, no instruction-boundary check. **The artefact is "copy, subtract `d` at marked 4-byte windows"** — describable with zero x86 content. Whatever mechanism it is, it is not "executable-relative relocation algebra" in any enforced sense. This is consistent with Track 20's option-(c) analysis: an unconditional scan with encoder-side admission is a *parser restriction*, not a decoder derivation — and calling it "derived" launders a transmitted decision into a zero-bit claim (Track 20's own N3, which this audit independently reaches).
2. **The field set is a property of the parse, not the file** (`src/anvil.cpp:594`: `(j & 3) == 0` with `j` = offset from phrase start). The same physical `rel32` is a transform field or a residual depending on where the match starts. So the mechanism does not "expose executable structure"; it exposes *phrase-alignment coincidences*, and its rate is coupled to parse choices. This is the strongest reason the mechanism could not have been a defensible claim even if its pool were large.
3. **I go one step further on the placement axis.** Track 20 F1 argues the *decode action* is identical and the difference is the trigger/parameter source. `start_offset` closes the placement axis too: **restricted-span BCJ is a documented shipped configuration of the public-domain filter**, so "apply BCJ where the reference is" is not an unoccupied mechanism — it is a call to the existing filter with a different window. The residue reduces to *which window*, which is a parser policy, not a compression mechanism.

**Consequence: no novelty ambiguity around C1/H2 remains.** The promotable scope is occupied (localized BCJ, adopt-class, K9 = engineering); the unoccupied residue (C2, distance-anchored ABS32) is corpus-blocked and Courgette-adjacent. **Track 14's executable-transform line is closed on prior art, not on the coverage scan.**

---

## 4. Remaining H2 novelty language in the paired report — challenged

Track 14 constructive §6 states that RSC *"is the only single-file self-referential copy whose transform parameter is derived from the match distance at zero bits. That distinction was real and worth building."* §13 nominates the instruction-length-derived sparse mask as the one admissible successor. **Both must be struck**, for the record:

| constructive text | disposition |
|---|---|
| §6 "only single-file self-referential copy whose transform parameter is derived from the match distance at zero bits" | **Struck.** "Derived from reference geometry" is BCJ-with-position-substitution (`start_offset`, Track 20 §16.2 E/F). The distinguishing phrase is an artefact of describing the same arithmetic with a different available register. |
| §6 table row "mechanism-novelty gates cannot rescue a lane whose measured information content is three orders of magnitude below the noise floor" | **Retained but inverted.** The *information content* argument was the wrong one (§1.1, §1.3). The lane dies because the mechanism is **occupied**, and it would have died at 46% coverage too. |
| §13 "the only sub-family I would permit… is a decoder-side instruction-length-derived sparse mask" | **Struck as a pre-registration candidate.** That is Courgette's disassembler + BCJ; `docs/I10-BREAKTHROUGH-PROGRAM.md` §4 H2 already forbids claiming instruction discovery / normalized-representation reconstruction; and it inherits the encoder/decoder length-agreement correctness class (Track 20 N2, high if discovered after a build). Track 14 constructive is right to decline it, and it should not be recorded as a permitted successor. |
| §3 "Any mechanism in this family must either transmit a sparse field set, or adopt a dense rule that does not hold" | **Retained.** Portable rule; see §2. |
| §9 RSEC remote plan (`R1/R2 ≥ 50%` pilot band) | **Struck as specified.** `R1` (contiguous, 4-aligned) is ~0 by construction and `R2` (sparse) is percent-level, so `R1/R2 ≥ 50%` is unreachable — the band silently converts any real survivor into a NO-GO. Also: **no data from a byte-only scan can upgrade a claim class** (Track 20 critic §10.2; PR-3 forbids a frontier crossing from a bytes-only oracle). |
| §5/§11.6 "the dual bar" | **Retained and promoted.** It is the only rule in the lane that still does work (§8). |

---

## 5. Engineering findings that survive the novelty closure (labelled `{engineering}`)

### 5.1 The shipped transform mask is structurally over-expensive — 32× too fine

From `src/anvil.cpp:2714–2716` (decode) and `:2625–2633` (encode):

```
nwin = len/4 ;  twn = (nwin+31)/32 ;  mask bytes = 4*twn ≈ len/32
```

Every type-3 token spends **~1 bit per 4 copied bytes — 3.125% of the copied length** on the transform mask *before entropy coding*, regardless of how many fields are present. The residual mask adds `≈ len/8` = 12.5%.

Worked consequences (arithmetic, not speculation):

- **`len = 4`, one isolated field (PNRA's modal shape):** mask cost = 4 B (transform) + 4 B (residual) = **8 B, to encode a 4-byte copy with zero residual bytes.** Strictly worse than literals. This is precisely Experiment X's recorded root cause (`RESEARCH_LEDGER.md:2566–2586`), now derived from the wire format. **Exp. X's regression is structural, not a tuning artifact** — Track 14 constructive is right about that and I confirm it from source.
- **`len = 64`, three fields:** pays 2 B + 8 B = 10 B of mask to save ~3 residual bytes. Still negative.
- **Break-even: ≈ 12–16 transform fields per reference** at current granularity. The mask is indexed by 4-byte window; the fields are instruction-anchored and ~1-in-5 sparse. **The mask spends ~32× more bits than the information it carries.**

**Why this survives the closure, and why it must not be dressed up:** it is a *cost* defect in an adopt-class artefact. It is worth recording because (a) it explains Exp. X's sign from source rather than from a heuristic, and (b) it tells the format lane that if anyone ever ships mode 14, the mask must be replaced by a sparse `(offset,len)` run list or by decoder-derived site marking. **It creates no novelty and no frontier claim.** It is `{engineering}` + `{attribution}`.

### 5.2 Ari-REF Experiment Z(b) is the strongest in-repo narrowing, and nobody used it as a kill argument

`RESEARCH_LEDGER.md` Exp. Z built **both** arms (`ari-tx` tag 0x02, `ari-impl` tag 0x03) on a shared token stream — the explicit-Δ control Experiment O records as *"NOT implemented"*. On `synth-arith`'s 2,160 tokens: **implicit costs +38.35% vs transmitted**; on an exact progression implicit beats transmitted by −15.38%, but with ±1 per-word jitter it degrades to **+40.55%**, because σ from one word-pair is multiplied by `d/4 ≥ 4` into residuals. On the corpus PE set, `ari-tx` deltas are −0.02%…−1.42%; `ari-impl` penalties up to **+3.10%** (jsonl).

Read together: **transmitted Δ carries the gain; implicit Δ is the weak form.** That is a *measured* narrowing of the implicit-parameter claim in exactly the genus Track 20's `start_offset` evidence occupies — **two independent reasons, one from documents and one from bytes, both pointing the same way.**

### 5.3 [M] Declared decode allocation envelope is 128× output, not 16×

`src/anvil.cpp:2653`: `const size_t max_sub = 16*out_len + 64;` is applied **per substream** inside an 8-substream loop (`:2654`). Aggregate declared ceiling = **8 × 16 = 128 × output length**. Declared, not exercised — a robustness property, not a live bug. Hand to the format/security lane (track 19); **not** a track-14 blocker.

### 5.4 [M] Positive correctness finding

Mode 14's non-overlap invariant is tight and correctly mirrored: `j + 4 <= dist` at encode (`src/anvil.cpp:594`), re-validated at decode (`o + 4 > start + dist` → reject, `:2726`). I checked the obvious hazard — a transform write landing inside the overlap window and aliasing a not-yet-copied byte — and it is correctly excluded. **The shipped mode 14 is sound.** Its problem is economics and attribution, never correctness.

---

## 6. Byte / cycle / memory accounting

| quantity | value | provenance |
|---|---:|---|
| RSC corpus coverage | 0.0107% (88 B / 823,264 B) | **[M]** constructive §1.2 — accepted; population = contiguous aligned runs in `.rsrc` |
| RSC best-case net over scanned corpus | ≤ **+66 B on 823,264** | **[M]** constructive §7.1 |
| rel32 opcode density, corpus PEs | 0.11–2.33% of file bytes | **[M]** mine, §1.1 |
| rel32-field upper bound (5 B/site) | ~0.6–11.6% | **[M]** mine, §1.1 |
| TCOPY vs sparse (Windows PEs) | −0.6% / −0.1% | **[L]** Exp. O, re-confirmed in frozen CSV |
| PNRA wired end-to-end | −0.0037% / **+0.0443%** | **[L]** Exp. X |
| PNRA wiring aggregate throughput cost | encode 9.679→9.223 MB/s; decode 172.0→166.0 MB/s | **[L]** Exp. X — paid per block regardless of commits |
| Mode-14 transform-mask cost | **≈ `len/32` B per type-3 token** (+`len/8` residual mask) | **[M]** from `src/anvil.cpp:2625`, `:2714` |
| Break-even fields per reference | **≈ 12–16** | **[M]** arithmetic |
| Mode-14 substreams | 8 vs mode 11's 7 | **[M]** `src/anvil.cpp:2592` |
| Ari-REF implicit vs transmitted Δ | **+38.35%** exact-progression / **+40.55%** jittered | **[L]** Exp. Z(b) |
| x86 BCJ filter gain, corpus | **0.0–7.9%** (7.93% on `pe-git.exe`) | **[M]** `tests/xz-transform-controls-i9.csv` |
| ANVIL best vs xz-9e | **+24.0%** / **+31.3%** | **[M]** §1.4 |
| Declared decode allocation envelope | **128 × output length** | **[M]** §5.3 |

**Cycles.** No timing run, by mandate. **[M]** structural: mode-14's copy is byte-at-a-time `out.push_back(out[out.size()-dist])` (`src/anvil.cpp:2712`) with no `memcpy` fast path. **[H]** ~4–6 cycles per transform field plus one 32-bit mask word per 128 phrase bytes. **Refutation of a likely objection:** constructive §7.2 credits RSC with ~0.20 cycles/byte of covered output as "the design's real virtue" — at 0.0107% coverage the total added decode work is under 100 slot operations. **Decode speed was never the constraint; no decode-side argument should carry weight in this lane.**

---

## 7. Decoder / resource risks

1. **Alignment coupling (§3.3.2)** — field set depends on phrase start; rate unpredictable across parser changes.
2. **ISA-blind marking (§3.3.1)** — the decoder applies `v -= d` on its own authority; exact by construction, semantically unverified. Corpus round-trip + fuzz exist (`RESEARCH_LEDGER.md:2517–2524`: 910 variants / 5,460 mutations) but there is **no targeted malformed-transform-mask campaign**. Remote-only, and only if the mode is ever revisited.
3. **128× allocation ceiling (§5.3)** — declared; needs a `FORMAT.md` statement.
4. **Type-3 non-overlap invariant (§5.4)** — sound.
5. **8 substreams vs 7** — one extra header + model init per block; inside the measured −3.5%.

---

## 8. The one executable remote experiment worth keeping — engineering only

Per `COORDINATOR-STATE.md` D17 and the coordinator instruction to keep the sparse control **only as closure/attribution evidence**, I drop my rev. 1 `H2-CTRL` proposal (arms C1–C6, including a mask-granularity build) in its entirety. Those arms would produce data about an **occupied** mechanism; under Track 20's G1/G2/G3 the outcome cannot change any claim class. Building a mask-granularity variant to chase an adopt-class artefact is not a use of a remote runner.

**What remains is the control the lane has never run, and it is adopt-class engineering:**

### **BCJ-CTRL — global x86 BCJ/E8-E9 pre-transform on ANVIL's own backend, dual-bar**

GitHub Actions only. No held-out data (`tests/corpus/pe-*.exe`, `anvil.exe`, `anvil_bench.exe` are open known/control data per `docs/I10-CORPUS-LOCK-PROTOCOL.md`; the frozen held-out PE set may be used as **controls only, never as promotion evidence**, per `COORDINATOR-STATE.md` D4).

| arm | what it isolates |
|---|---|
| **B1** `anvil-sparse-rans` / `anvil-mdl-rans`, raw input | ANVIL baseline, no executable transform |
| **B2** same backend on **x86-BCJ-filtered** input (`E8`/`E9` + `0F 8x` relative→absolute, inverse on decode) | the adopt-class route — the real bar |
| **B3** `anvil-tcopy-rans` on **BCJ-filtered** input | attribution only: does in-reference add anything *on top of* the filter? (expected: no) |
| **B4** *control:* `xz --filters=x86` vs plain `xz-9e` on the same files, reproduced in-job | validates the filter's in-job gain against §1.5's frozen numbers |

**Why B2 is the only thing worth the runner:** §1.5 shows the filter is worth up to **7.9%** on this corpus and §1.4 shows ANVIL's executable gap to a filter-enabled reference is **24–31%**. That is the only large, cheap, already-prior-arted number in the lane, and it has never been measured on ANVIL's backend. B1/B2 is also the **minimum prerequisite** for any future statement about executables at all: the project's own `tests/pareto-verdict.csv` has never had a transform-enabled reference on this class.

**Frozen thresholds (fixed before measurement):**

| outcome | condition | ruling |
|---|---|---|
| **INVALID** | any row missing its per-file output hash; or B1 not reproducing the frozen `tests/benchmark-suite.csv` byte counts **within 0 B**; or any non-PE file changing bytes; or B4 not reproducing §1.5 within 0.5 pp on ≥5/6 PEs | no interpretation; re-run |
| **ADOPT-CLASS WIN** | B2 ≤ B1 − **1.0%** on ≥ **4/6** PEs with decode ≥ **0.90×** B1 | `{engineering}`/`{adopt-class}`: propose BCJ as an ANVIL pre-transform. **No novelty claim, no EXTENDS_FRONT language** |
| **NEUTRAL** | B2 within ±1.0% of B1 on ≥4/6 | record; the executable-class bar is brotli q4 after all, and `xz-9e` remains the external frontier |
| **ADVERSE** | B2 > B1 + 1.0% on ≥3/6 | the filter perturbs ANVIL's literal/phrase contexts (the lane-transpose failure mode, `docs/anvil-i9-findings.md:379`); record as a measured negative and stop executable work entirely |

**Mandatory reporting:** per-file rows, all four arms, all six PEs; both bars on every row; no frontier language from a byte-plus-timing job without two hash-identical arbiter runs; `tools/pareto_front.py` remains the sole issuer of any `EXTENDS_FRONT` boolean.

**Attribution value, stated explicitly so it is not mistaken for a mechanism result:** B3's only purpose is to let the ledger attribute mode-14's bytes between "the transform" and "the parse." Exp. O already records TCOPY losing to mode 11; B3 closes whether TCOPY adds anything once the transform is applied globally for free. **Expected result: it does not.** If B3 unexpectedly beats B2 by ≥1%, that is a *frontier gap to investigate*, not a novelty resurrection — the claim class remains `{engineering}` regardless.

---

## 9. Prototype status

**None built.** `prototypes/swarm-2026-10-02/14-executable-compiler/fledge/` is intentionally **empty**. The only cheap independent measurement this audit could make without touching production was the `E8`/`E9` opcode-density scan (§1.1) — a single linear byte pass over the locked corpus, executed inline, needing no artefact. A standalone reimplementation of the paired report's probe would have duplicated it without adding an independent instrument, and every remaining decisive measurement belongs to §8's remote job.

---

## 10. Falsification criteria for my own conclusions

| my claim | falsified if |
|---|---|
| H2/C1 novelty is closed (localized BCJ, adopt-class) | A shipped instance is found where a per-reference, distance-anchored relative→absolute rewrite is (a) not `bcj(pos=…)` with a substituted position, and (b) not reachable by `lzma_options_bcj::start_offset` or an equivalent filter window. **Track 20's K10 (anchor-multiplexed classes) is the only candidate and it is `{engineering}` by its author's own pre-classification** |
| C2 is corpus-blocked, not refuted | A locked held-out family with ABS32/pointer-pool structure is admitted under `docs/I10-CORPUS-LOCK-PROTOCOL.md`, and `C2`-eligible bytes `> 0` on ≥2/6 PEs |
| The coverage scan does not bound the mechanism family | An alignment-free, contiguity-free sparse scan returns ≤1.0% of `.text` and ≤0.5% of whole-file bytes on ≥6/8 files — **which would make the constructive's ruling survive its own instrument. It still would not restore novelty** |
| Implicit Δ is the weak form in-repo | Exp. Z(b)'s +38.35% does not reproduce on the frozen files |
| The shipped transform mask is ~32× too fine | Byte-only instrumentation shows the removed/added mask ratio ≥ **2.0** on ≥2 executable cells (Track 20's G1 GO bar). **Note: even a GO here yields `{engineering}` only** |
| TCOPY is outranked inside ANVIL | mode 14 (`0.404`/`0.438`) is re-measured at or below mode 17/mdl (`0.400`/`0.430`) — i.e. the frozen CSV rows are stale |
| The lane's gap is ~24–31% | `tools/pareto_front.py` reports `anvil-mdl-rans` non-dominated by `xz-9e` on the PE class — it does not, because `pareto_front.py` does not ingest xz rows (below) |

---

## 11. Cross-cutting items for the coordinator

1. **`tools/pareto_front.py` does not ingest the xz rows.** The frozen verdict files carry `xz-9e` and the xz transform controls, but no Pareto issuer consumes them. On the executable class the frontier is `xz-9e` (BCJ-enabled), not Brotli. **Every "ANVIL loses on executables" statement in this project has been made against the wrong bar.** This is a project-level measurement defect larger than any single track's mechanism, and it is the direct cause of Track 14's lane being sized at 0.6% instead of 24%.
2. **The dual-bar rule has no enforcement.** It exists as prose (`RESEARCH_LEDGER.md` PART XIII §5b; cited by Tracks 09, 14, 20) but no script checks it. Suggest a `pareto_front.py` flag requiring a transform-enabled reference row for any executable-class verdict.
3. **Two closures in this lane were mislabelled.** PNRA is *"blocked on a missing harness header"* (`RESEARCH_LEDGER.md:1676–1696`), not "mechanism failed"; and H2 is now *"prior-art closed"*, not "measured too small." Both distinctions matter if either is ever reopened.
4. **Cross-lane consistency check.** Track 09's H2 and Track 14's RSC reach compatible verdicts by different routes (09: occupied via BCJ equivalence; 14: measured near-zero coverage plus BCJ equivalence). Per `COORDINATOR-STATE.md` D15/D17 both now close. **No contradiction remains to reconcile** — my rev. 1 HOLD was the outlier and is withdrawn on the novelty axis only; its engineering findings (§5.1, §5.3, §1.5) stand.

---

## 12. Recommendation

# KILL

Track-level, split precisely. **This is a change from rev. 1 and I state why in one line: my rev. 1 logic was right — the coverage scan does not bound the family — but Track 20's `start_offset` evidence closes the family on prior art regardless of coverage, so the survivor my refutation protected no longer exists.**

**KILL — the executable-compiler mechanism line (H2 / C1 / RSC / TCOPY / PNRA / Ari-REF implicit-Δ), on prior art.** Reference-local rel32 normalization is BCJ with a substituted position; `lzma_options_bcj::start_offset` shows restricted-span application is a *shipped, documented* parameter of the public-domain filter. Mechanism-novelty gates cannot rescue it. Independently and additionally, Exp. Z(b) shows the implicit-parameter form is the *weak* form in this repo. No remote run can change either fact.

**CORPUS-BLOCKED, not killed — C2 (ABS32, `Δ=+d`).** The only structurally unoccupied residue, because its anchor is the reference distance and no whole-file pass can see it. Blocked on a locked held-out family that does not exist, and adjacent to Courgette's ABS32 handling. Recorded so it is not lost; not actionable now.

**ADOPT-CLASS, RETAINED — global x86 BCJ as an ANVIL-side pre-transform.** Worth 0.0–7.9% on the locked corpus (§1.5), against a 24–31% lane gap (§1.4), and never yet measured on ANVIL's backend. This is where the lane's engineering value is.

**`{engineering}` RETAINED — §5.1's mask-granularity finding.** The shipped mode-14 transform mask is ~32× finer than the field population, forcing ≈12–16 transform fields per reference to break even, and fully explaining Experiment X's measured sign from the wire format. Worth recording for the format lane; **no novelty and no frontier claim attaches to it.**

**The one experiment worth a remote runner: `BCJ-CTRL` (§8).** Four arms, dual-bar, adopt-class engineering. It buys the measurement the entire executable class has been missing — ANVIL's own backend against a transform-enabled reference — and it is the only proposed run in this track whose outcome changes a *project-level* decision rather than an already-closed claim. Everything else in this lane is documentation.

---

*Independent critic. No Space Bunny verdict inherited; §2, §3, §4, §5 and §8 are constructed against the paired report's claims and against Track 20's chart. No production file modified. No local benchmark run. Scratch directory left empty.*