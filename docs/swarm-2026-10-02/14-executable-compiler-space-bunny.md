# Track 14 — Executable/Binary Representation Compiler — Space Bunny Free

**Date:** 2026-10-02
**Agent:** Space Bunny Free (constructive inventor)
**Track:** 14 — executable/binary representation compiler.
**Document status:** FINAL for this track (revision 2; revision 1 over-killed and was corrected by the coordinator).
**Worktree discipline:** no existing file modified; no reset/clean/stash/restore/rebase; no commit/push; no local compression or throughput benchmark; no fuzz campaign. One new prototype directory, not wired into production.

**Peers read and reconciled:** `09-exact-tcopy-pnra-space-bunny.md`,
`09-exact-tcopy-pnra-fledge.md`, `20-priorart-killteam-space-bunny.md`,
`docs/swarm-2026-10-02/COORDINATOR-STATE.md`.
*Note: the paired Track 14 critic (`14-executable-compiler-fledge.md`) landed after
revision 1. **Revision 3 incorporates its §1.1/§1.2 methodological correction, which
overturned a claim in revision 2.** Its headline rulings (KILL-RSC / HOLD-sparse /
BCJ-as-mandatory-control / lane-level HOLD) agree with mine.*

---

## 0. Headline and final ruling

Four separable verdicts, deliberately kept apart because conflating them is what
revision 1 of this report got wrong:

| # | Object | Verdict | Basis |
|---|---|---|---|
| 1 | **RSC** — contiguous, all-shifted, zero-residual derived-run copy (built this session) | **KILL** | **Measured** coverage 0.0107% of corpus bytes. Three orders of magnitude short. Per the paired critic, this is *redundant rather than informative*: RSC is a stricter special case of PNRA, which is already closed (Exp. P/X). |
| 2 | **H2 / C1 novelty** — reference-local, decoder-derived Δ=−d transformed copy | **CLOSED by prior art.** Not defended, not reopened. | Track 20: xz's documented `lzma_options_bcj::start_offset` already ships the insight H2 rests on; Track 09 X7: the shipped PNRA invariant `I(v,p)=p+4+v` (`src/anvil.cpp:847-848`) **is** BCJ's canonical quantity. |
| 3 | **Global BCJ / E8-E9 / x86 normalization** | **Mandatory engineering control** for every executable claim. Not a differentiator, not a novelty position. | Track 09 arm `A3` (the decisive missing control); Track 20 failure mode `F1`. |
| 4 | **Sparse transformed-reference engineering** (TCOPY/PNRA as mask-carrying transforms) | **HOLD** as a narrow mask/framing economics question. Not killed by this report; not a novelty lane. | **Measured** aligned-probe addressable fraction 1.591% whole-file / 2.046% of `.text`; **measured** realized value 0.0566% (Track 09 X1). **Correction (rev. 3):** the aligned figure is a **lower** bound, not a family ceiling — see §1.3. |

**No broad executable novelty lane is left open by this report.** Items 2 and 3
close the novelty question; item 4 is explicitly an engineering-economics
remainder, not a mechanism claim.

---

## 1. Evidence map

**[M]** measured this session, provenance given · **[L]** already-recorded project
fact, cited to artifact · **[H]** hypothesis / projection.

### 1.1 [M] Prototype build and self-test

Artifact `prototypes/swarm-2026-10-02/14-executable-compiler/space-bunny/rsc_probe.cpp`,
built `clang-cl 22.1.8 /std:c++20 /O2 /W3 /WX /D_CRT_SECURE_NO_WARNINGS` (exit 0,
warning-clean). Logs: `coverage_run.txt`, `coverage_sysbin.txt`.

```
selftest shiftall/nonzero/short/mixed  PASS   (exact byte roundtrip)
selftest malformed                    PASS   (out-of-range token rejected)
selftest PASS (0 failures)
```

Four bugs found and fixed in construction. Two were **silent out-of-bounds writes**
(hash chain sized by slot count but indexed by file position; PE section stride 64
instead of 40). Recorded because it is the standing argument for remote-only fuzzing
before any claim.

### 1.2 [M] Arm R1 — contiguous derived-run coverage (the RSC mechanism itself)

Single linear structural pass. **No compression, no timing, no fuzzing.** Open
`known_stress` PE anchors only.

| file | bytes | runs | run bytes | coverage |
|---|---:|---:|---:|---:|
| `pe-where.exe` | 61,440 | 0 | 0 | 0.0000% |
| `pe-winver.exe` | 28,672 | 0 | 0 | 0.0000% |
| `pe-python.exe` | 103,704 | 1 | 28 | 0.0270% |
| `anvil.exe` | 268,800 | 0 | 0 | 0.0000% |
| `pe-notepad.exe` | 360,448 | 4 | 60 | 0.0166% |
| **aggregate** | **823,264** | **5** | **88** | **0.0107%** |

Anatomy: median run 2 slots, max 7 slots in 2 of 5 files. Runs land almost entirely
in `.rsrc`. `.text`, `.rdata`, `.data`, `.pdata`, `.reloc` contribute ≤1 run each.

### 1.3 [M] Arm R2 — sparse addressable fraction under a 4-byte-aligned probe

**CORRECTION (rev. 3, from the paired critic §1.1–1.2): revision 2 wrongly called
this the mask-free ceiling for the family. It is not.** R2 probes only **4-byte-aligned
file positions**. `rel32` immediates sit at instruction-determined offsets, which in
optimized x86 are generally **not** 4-aligned; the critic measures the true field
population at **1–2.3% opcode density, ~100–1000× the pool this scan can see**.
Therefore:

> **R2's 1.591% is a LOWER bound on the addressable fraction for aligned probes. It is
> NOT an upper bound on what any mask-transmitting mechanism — including TCOPY —
> could reference. This scan supplies no usable ceiling for the sparse family.**

The consequence is stated plainly because it cuts against my own revision 2: **the
headroom of the sparse family remains genuinely unmeasured**, which is a further
reason to HOLD it rather than close it. The R1/R2 pair still does what §3 says it
does — it brackets the *contiguous derived-set* position at zero — but it cannot
bound the masked position.

Same pass, counting **every** slot satisfying `v_p + A(p) = v_q + A(q)` at the best
distance `d`, contiguous or not, ignoring mask cost. Chain cap 8, slot cap 64,
same-section only.

| file | bytes | sparse bytes | whole-file | `.text` sparse | max slots |
|---|---:|---:|---:|---:|---:|
| `anvil.exe` | 268,800 | 4,648 | 1.7292% | 1.938% | **1** |
| `pe-where.exe` | 61,440 | 912 | 1.4844% | 2.185% | **1** |
| `pe-notepad.exe` | 360,448 | 5,372 | 1.4904% | 2.351% | 7 |
| `pe-python.exe` | 103,704 | 620 | 0.5979% | 0.391% | 7 |
| `pe-winver.exe` | 28,672 | 152 | 0.5301% | 0.781% | **1** |
| `pe-ninja.exe` | 603,648 | 10,996 | 1.8216% | 2.008% | 2 |
| **aggregate** | **1,426,712** | **22,700** | **1.5912%** | **2.046%** (17,900/874,984) | — |

Cross-section pairs (which would require granting the decoder the section table, a
format coupling) add only **4,272 B ≈ 0.30 pp**. The §5 charge for that coupling is
therefore small but real, and is not claimed.

**`maxSlots = 1` on 3 of 6 files.** This is the single most important number in the
report: within the aligned probe, the sparse hits are dominated by **isolated single
4-byte fields**, not by extended transform runs — which is also why the aligned figure
understates the true field population.

### 1.4 [L] The realized value, and the resulting 28× tax

Track 09 X1 (**`XL-MEASURED`, critic lane, re-derived from the frozen Class A suite
`bdc90474`**): TCOPY's entire in-repo effect is **1,477 B = 0.0566%** of corpus bytes
against its own control (sparse-rANS 2,609,164 B), and it is **6.88% worse** than
MDL-rANS (2,439,834 B); it is **DOMINATED by brotli q6 on bytes, encode AND decode
simultaneously**. Track 09 X7: PNRA-current is net-worse with **−33.7% aggregate
encode**.

Reconciling 1.3 with 1.4 — these are *not* spliced; they are two different quantities
on the same object, and 1.3 is now known to be a lower bound:

> **Addressable under an aligned probe (mine, measured): 1.591% of bytes.**
> **True addressable fraction: UNMEASURED, and ≥ the above by a large factor**
> (critic: field population 1–2.3% opcode density, ~100–1000× this pool).
> **Realized (Track 09, measured): 0.0566% of bytes survive mask + framing costs.**
> **Mask/framing tax: ≥ 28×, and unbounded above until the field population is
> measured at the *correct* (instruction-aligned) probe positions.**

So the revised reading is weaker in one direction and stronger in another. It is
**stronger** that the tax is real and large — Experiment X measured the framing loss
and Track 20 projects H2's prize at ~0.02%, so the surviving value of any masked
mechanism is small on both available estimates. It is **weaker** as a ceiling claim:
I can no longer say the pool is bounded at 1.6%, so I cannot close the masked family
on measurement. **The tax is what decides this family, and the honest statement of the
tax requires an instruction-aligned field-population measurement that nobody in this
project has run.** That missing measurement — not a codec build — is the real
remainder.

### 1.5 [L] Prior-art state of the novelty question

- **Track 20:** xz's documented `lzma_options_bcj::start_offset` **already ships the
  exact insight H2 rests on**; H2 was downgraded from "PILOT a byte-only screen" to
  **"HOLD as a claim"**. Failure mode `F1` ("H2 = BCJ restricted to a source span")
  is rated **fatal to the whole surviving space**, survivable only by proving the
  *placement* difference is load-bearing and the parameter is geometry-derived.
- **Track 20 `N2`:** H2 requires a real x86 length-disassembler, which would make it
  **Courgette**, whose patent position was never audited; the claim must be
  withdrawn as "normalized-representation reconstruction".
- **Track 09 X7:** the shipped PNRA invariant is BCJ's canonical form.
- **Track 09 arm `A3`** (global BCJ/E8-E9 + same backend) is named **the decisive
  missing control**; `COLLAPSE-C1` (A4 ≥ A5 → novelty → ADOPT-CLASS) and
  `COLLAPSE-C2` (→ novelty **NONE**) are pre-declared.
- **Patent gate (ledger Experiment K + Intel Pass 5):** the relocation algebra and
  transformed-copy primitive are prior art (US7676506B2, 2003, two-file); the Intel
  μop-address family is non-anticipating; the single-file implicit-Δ position was
  left *conditionally* unclaimed.

### 1.6 [M] Provenance hygiene

- `tests/corpus/pe-notepad.exe` and `C:\Windows\System32\notepad.exe` share SHA-256
  `49F096CBF9337B0A80BDE835D29BE41BC9371057C4FF6C72F8A36158C29CFA3A` — the corpus PE is
  an untruncated verbatim copy, so the near-zero R1 result is a property of real
  linked images, not of clipped fixtures.
- **Corpus status:** per `09-...-space-bunny.md` §corpus and
  `docs/I10-CORPUS-LOCK-PROTOCOL.md`, the six `pe-*` files plus `anvil.exe` are
  **consumed `known_stress` anchors** (5,540,960 B). Every number in §1.2–1.4 is
  **discovery / control evidence only** and is **never** promotion evidence.
- **System32 / SysWOW64 scans are anatomy only.** They rule out a truncation
  artifact. They are **not** held-out, **not** promotion evidence, and are **not
  pooled** with any corpus figure. No figure in this report mixes the two strata.

---

## 2. Mechanism as built (RSC)

Token `RSC(d, L, mode)` at output position `p`, source `q = p − d`
(same-section, non-overlapping, `q + L ≤ p`):

```
for j in 0 .. L/4-1:
    s = LE32(out[q + 4j])
    mode 0 (SHIFT_ALL):        out[p+4j ..] = s - d
    mode 1 (SHIFT_NONZERO_SRC): out[p+4j ..] = (s == 0) ? 0 : s - d
```

Properties, all structural: **exact, zero residual**; **zero transmitted transform
parameter** (`d` is the ordinary match distance); **AUDIT-7 causal** (uses only the
token and bytes already reconstructed at `q < p`); **G4-amended compliant**
(`RESEARCH_LEDGER.md:4559-4563` permits a zero-bit reference-derived parameter only
where the transform is exact); **decoder state zero bytes** — no index, no sidecar,
no auxiliary table, no section table, RSS delta = output buffer only.

Discovery: position invariant `J(p) = LE32(out[p]) + A(p)`,
`A(p) = section_rva + (p − section_raw)`, open-addressed table, chain cap 8, forward
run growth, greedy disjoint cover, minimum 2 slots. Same-section restriction makes
`A` cancel, so the derived constant is exactly `−d` and the decoder needs no image
model.

**Hypothesis, stated before numbers were seen:** *a whole-run shift needs no
transmitted field mask, so it removes the per-token framing cost that killed TCOPY
and PNRA, and relocates the executable win from `.text` into the `.data`/`.rodata`
pointer planes.*

**Verdict: REFUTED** — R1 coverage 0.0107%. §3 explains why, structurally.

---

## 3. Why RSC failed — the granularity trap

A 32-bit field in a linked image satisfies `v_p + A(p) = v_q + A(q)` in exactly two
regimes:

**(A) Stored image-absolute.** After relocation, `.rdata`/`.data` pointers hold
`imagebase + RVA`, so two copies are **byte-identical** (8 bytes each on x64).
Ordinary LZ already copies them at `varint(d) + varint(L)` with no transform and no
residual. A shift reference is **strictly dominated** — same token bytes plus
`L/4` subtracts for zero information. This explains why `.rdata`/`.rodata` showed
zero useful TCOPY phrases (ledger Experiment K) and why my R1 found nothing there.

**(B) Stored position-relative.** `rel32` immediates, `.pdata` RVA triples,
`.reloc` page RVAs — genuinely shift-derived, but **non-contiguous**: in optimized
x86 a `rel32` immediate occurs roughly once per instruction and only in a fraction of
instructions, so consecutive useful fields are 4–15 bytes apart.

> **The granularity trap.** A zero-bit derived rule must be a **complete rule over a
> contiguous slot block**, because G4-amended requires zero residual — if one slot
> fails, a mask must be transmitted, which is exactly what the mechanism existed to
> eliminate. Exactness forces contiguity; the fields carrying the information are
> not contiguous. **The intersection is empty.**

This is why RSC found nothing while R2 (aligned probe) found 1.591%: R2 relaxes
contiguity — accepting that the field set must then be transmitted — and consequently
*can* see part of the information. The arms bracket the **contiguous derived-set
position** exactly, and nothing wider:

- **Contiguous derived-set (zero bits) → 0.0107%. DEAD.**
- **Sparse under an aligned probe → 1.591%, a lower bound on the true pool.**
- **Realized (mask + framing paid) → 0.0566%.**

**Per the paired critic (§2), the trap argument is valid and portable but was
over-extended in revision 2.** It proves a **cost** claim ("you must transmit a
sparse field set"), not a **coverage** claim ("the fields do not exist"). It
correctly predicts RSC's death; it does **not** predict TCOPY's death. It should be
recorded as the transferable rule *"zero-bit exact transforms over position-relative
relocation fields have a hard ceiling near zero at contiguous granularity — check
this before building"*, and **not** as a family closure. I adopt that reading here.

**A stronger, already-recorded kill argument exists and the critic is right that I
should have used it.** `RESEARCH_LEDGER.md` **Experiment Z** (ARI-REF) found that
implicit derivation from `d` is exact only when the relation holds *exactly*; on real
binaries it usually does not, so a residual error amplified by `d/4` must be paid as
residuals. That is the **identical failure mode** Experiment X measured as PNRA's
per-token framing loss. **Two in-repo experiments, two granularities, one diagnosis:
the gain that survives an implicit-Δ transform is precisely the one that pays bits
for Δ.** This is a better-grounded argument than my coverage scan — it is in-repo,
reproducible and already recorded — and it is a *cost* argument that applies to
masked mechanisms without claiming they are absent.

**Scope of the claim.** Stated for contiguous, whole-block, zero-residual, 4-byte-slot
transforms over instruction streams and linked-image pointer planes. **Not** claimed
for non-contiguous decoder-disassembled masks (Track 20 `N2` → Courgette), 8-byte
slots (untested; R2's 8-byte arm is the natural next measurement), or cross-section
grants (measured worth only ~0.30 pp, and a format coupling).

---

## 4. Exact invariants vs heuristic preprocessing

The exact half carries all the information; the heuristic half carries all the bit
cost; and the exact half cannot reach the information without the heuristic half.

**Exact (decoder-derivable, zero bits, no encoder/decoder disagreement risk):**
(1) position-invariance `v + A(p)`; (2) within-section bias cancellation, so the
constant is `−d` with no image model; (3) the `rel32` algebra `v_dst = v_src − d`;
(4) equal absolute-target runs ⇒ constant additive shift.

**Heuristic (must be transmitted, verified, or paid for):**
(1) **the field set** — the entire cost centre; TCOPY transmits it as a mask, RSC
tries to derive it by rule and gets an empty set; (2) section classification;
(3) instruction boundaries/lengths — the only route to a *sparse* derived set, and
the one that makes the mechanism Courgette; (4) opcode-pattern triggers (`E8`/`E9`),
which also force x86-specificity.

The 28× tax in §1.4 is exactly the price of crossing from the exact half to the
heuristic half. It is not a tuning target.

---

## 5. Relocation and control metadata, charged

| metadata | measured size | charge here |
|---|---|---|
| PE section table | 6–12 × 40 B | **0 bits** same-section (`A` cancels). Cross-section would require the decoder to have parsed the header — a **format coupling**, measured worth only ~0.30 pp (§1.3), **not claimed** |
| `.reloc` (loader reloc blocks) | 4,096–20,480 B | R1 **0 runs**; R2 sparse ≤0.78% of section, mostly 4 B absolute |
| `.pdata` (exception RVA triples) | 4,096–24,576 B | R1 **0 runs**; R2 sparse 0.49–3.47% of section — the purest RVA-valued region, and the *only* region where the sparse arm is materially non-zero |
| `.rsrc` resource tables | up to 126,976 B | R1's only real source; R2 0.44–1.37% |
| transform mask / field set | — | **0 bits**, which bought nothing, because the derived set is empty |

**Dual bar is mandatory.** This lane's entire comparison class is BCJ-enabled, so any
win visible only against raw Brotli is struck per `RESEARCH_LEDGER.md` PART XIII §5b.

---

## 6. Prior art: global BCJ / x86 normalization is the control, not a rival

The operative axis is **not** "is my transform reference-local or file-local". It is
**"is reference-local normalization distinguishable from BCJ normalization restricted
to a span a copy already selected"** — and on that axis the answer is no.

| family | object transformed | self-ref single file? | parameter source | standing |
|---|---|---|---|---|
| **BCJ / E8-E9 / xz filter chain** | whole stream, pre-LZ filter layer | filter | fixed opcode table + **`start_offset` (xz, documented)** | **THE mandatory control.** Track 09 arm `A3`, unbuilt. |
| **H2 / C1 (reference-local derived Δ=−d)** | span of one copy | yes | derived from `d` | **NOVELTY CLOSED** (Track 20: `start_offset` ships the insight; X7: shipped invariant **is** BCJ's quantity) |
| **RSC (this report)** | contiguous slot run inside one copy | yes | derived from `d`, zero bits | **KILL on coverage**; and on the operative axis it is BCJ's quantity restricted to a span |
| Courgette / Zucchini | two-file patch, disassembler-derived | no | transmitted / synthesized | two-file; and any disassembly route *is* Courgette (Track 20 `N2`) |
| VCDIFF / bsdiff / Zdelta | two-file or self-ref copy + `ADD`/`RUN` | VCDIFF yes | **transmitted** | copy-with-edits; separator is the mask-as-stream, not self-reference |
| Prelinking; global reloc normalization (IBM US6564314; MS US6466999) | global, needs external `.rel` / two-file | no | transmitted hint tables | excluded by mandate |
| Apple dyld chained fixups | loader rebase **metadata** | single-file | chained lists | closest single-file pointer-metadata compaction; not an LZ reference |
| `xz --delta`, Gorilla/FPC | statistical stream transform | n/a | transmitted stride | ledger classifies the transmitted per-region step reference as **engineering/adopt** |
| SIA / Wallace et al., EXEDUMP, COMPAX | opcode+operand stream split | single-file | transmitted opcodes | dense prior art; any operand-plane split is adopt-class |
| Intel US 7,111,148 / 7,010,665 | runtime μop address compaction | no LZ layer | stored head IP + transmitted correction | gate CLOSED, non-anticipating |

**Conclusion.** On the operative axis, the "reference-local vs global" distinction is
a **placement** difference, not a mechanism difference — and Track 20 `F1` rates it
fatal to the surviving space. RSC was additionally killed on its own coverage
measurement. **No executable novelty lane is left open here.**

---

## 7. Complete cost model

Bytes and coverage **measured**; cycles **projected from op counts [H]** — no timing
was run, by mandate.

**Bytes.** Per RSC token: 2 bits mode + `varint(d−1)` + `varint(L−4)`; model cost 0.
Measured on the corpus PE set: **22 bytes total token cost against 88 bytes gross
coverage** — net ≤ +66 B on 823,264, and strictly negative on 3 of 5 files.

**Cycles, decoder.** Per slot: 1 load, 1 conditional test (mode 1), 1 subtract,
1 store; AVX2 handles 8 slots per 256-bit iteration ⇒ `L/32` iterations, ~6 cycles
each. **Projected ~0.20 cycles/byte** vs ~0.04 for `memcpy`. The design's real virtue
— and irrelevant at 0.0107% coverage.

**Cycles, encoder.** 4-byte-stride pass: 1 load, 1 add, a 3-multiply mix, 1 insert
per position; 1 chain step + 1 slot of run growth per hit, cap 8.
**Projected 0.5–2.0 cycles/byte** (~1.5–6 GB/s). Encoder speed was never binding.

**Memory / RSS.** Decoder **RSS delta = 0 bytes** beyond the output buffer — the only
candidate in this lane with that property, and the reason RSC was built rather than a
sidecar design (the lane-transpose rejection rules out every design needing a second
interleaved stream). Encoder ~2.5 B per input byte at 2× load factor; encoder-only,
never shipped.

**Asymptotics.** `RSC = Θ(C)` slot ops, `O(1)` decoder state. Encoder `Θ(n)` dense
pass + `O(chaincap × runs)`. Crucially the mechanism does not fail at large `n` — it
fails identically at every `n`, because the ratio `C/n ≈ 10⁻⁴` is a property of the
instruction stream, not of file size.

---

## 8. Strongest disconfirming evidence

1. **R1 = 0.0107%** on open PE anchors; **0.0040%** on untruncated system binaries
   (anatomy only). Three orders of magnitude short. Not a near miss.
2. **Run-length anatomy refutes the premise:** median 2 slots; the relation breaks at
   the first non-relative slot (§3).
3. **The purest test region failed:** `.pdata`/`.reloc` are nothing but RVA fields
   and yielded **0** R1 runs — their RVAs are distinct, not repeated.
4. **Independent reproduction of the ledger boundary:** my probe reproduced TCOPY's
   `.rodata`/`.eh_frame` zero without reference to TCOPY.
5. **The mechanism's selling point was self-defeating:** zero bits bought by demanding
   a dense set is worthless when the dense set is empty; the information is in the
   sparse set, which must be transmitted. Framing is forced, not tuned.
6. **The one thing that could rescue R1 also kills it as novelty:** the only way to
   derive a *sparse* field set is decoder-side disassembly, which is Courgette
   (Track 20 `N2`).
7. **[M, critic §1.1–1.2] My own scan is misaligned with the fields it claims to
   bound.** `rel32` immediates sit at instruction-determined, generally non-4-aligned
   offsets; critic measures the field population at 1–2.3% opcode density,
   ~100–1000× my probeable pool. My R2 figure is a **lower** bound and rev. 2's
   family-ceiling claim is retracted.
8. **[M, critic §1.5] The lane is missing its bar.** The x86 BCJ filter is measured
   in-repo at **0.0–7.9%** on corpus PEs (**7.93%** on `pe-git.exe`,
   `tests/xz-transform-controls-i9.csv`). A +0.6% measured against raw Brotli on a
   file where `--filters=x86` is worth +7.9% is not a result. **Without the BCJ
   control, no executable-lane number in this project should be quoted.**
9. **[L, critic §4] The real threat to H2 is not the BCJ boundary.** It is
   **US7676506B2** (Reinsch lineage), which teaches `Δ = position difference for
   relative fields` — exactly TCOPY's algebra — in a two-file form. Novelty was
   already narrowed against it (ledger Experiment K); the critic is right that I
   over-weighted the BCJ axis.

---

## 9. The one decisive REMOTE-ONLY experiment

This is **not** a new mechanism test. It is the closure ablation for the *engineering*
remainder, and it is byte-only.

**Name:** `TCOPY-PROVENANCE-BCJ` closure run — Track 09's `A3`/`A4`/`A5` design,
executed once. GitHub Actions only. **Byte-only.** No timing, no throughput, no fuzz,
no held-out data. No mechanism build.

**Arms (complete charged wire, same backend):**

| arm | definition |
|---|---|
| `A0` | raw input, control |
| `A1` | exact-LZ control |
| `A2` | implicit `Δ=−d` with **transmitted** transform mask (mode 14, exists) — control of record |
| `A3` | **global BCJ/E8-E9 + same backend — THE decisive missing control** |
| `A4` | transmitted-Δ control (the ablation ledger Experiment O records as never built) |
| `A5` | reference-local derived-mask / derived topology — the H2 target |

**Controls that VOID the run:** identity-class `A4` with recognizer disabled must
reproduce `A0` byte-for-byte; per-file output hashes recorded for every arm; no-regex
controls (`random.bin`, `generated.repeat.jsonl`, every non-PE file byte-identical
across arms) present and reported.

**Data.** Open `known_stress` PE anchors only, reported per-file and as a stratum,
**never** pooled with any future locked family. Terminal outcomes are **KILL** or
**HOLD-pending-lock**; this run cannot produce a promotion.

**Second, cheaper measurement that should precede it (byte-only, and the true
remainder):** an **instruction-aligned** field-population count — the density of
`rel32`/`E8`/`E9` sites at their *actual* offsets rather than at 4-byte-aligned
positions. §1.3 shows nobody in this project has measured it, and without it the
mask/framing tax in §1.4 cannot be bounded from above. It is one linear pass, needs
no codec, and it is the only thing standing between this lane and an honest closure.
I am **not** running it locally, per the no-local-corpus-benchmark rule; it belongs in
the same Actions run.

**Pre-registered thresholds** (Track 09's, adopted unchanged):

| id | promote if | kill if |
|---|---|---|
| G1 | removed/added mask-byte ratio ≥ **2.0** on ≥2 executable cells **and** projected end-to-end prize ≥ **0.10%** | ratio < 1.5 on every cell → NO-GO as a byte mechanism; keep BCJ as the adopt-class route |
| G2 | `A5` ≤ `A4` − 1.0% complete bytes on ≥2 cells, `A5` decode ≤ `A4` × 1.02 | `A5` ≥ `A4` on both cells → **COLLAPSE-C1**: report as BCJ-restricted-to-span, adopt-class |
| G2b | — | `A5` ≥ `A3` → **COLLAPSE-C2**: novelty **NONE** |
| G4 | `A5` ≤ `A1` − 0.5% on ≥1 cell, decode within 1.10× | `A5` ≥ `A1` → no value proposition → KILL |

**My §1.3 measurement additionally pre-registers a hard prior:** R1/R2 = 0.0107/1.591 =
**0.67%**. The zero-bit derived form captures **0.67%** of what an aligned probe can
address, and less of the true population.
Any arm set that cannot close a ≥28× mask/framing tax (§1.4) cannot reach a
meaningful prize, and G1's `≥0.10%` projected end-to-end is the number that decides it.

---

## 10. Adversarial failure cases

1. **Relocatable/pre-stripped objects** (`.o`, unlinked ELF): `A(p) = p` everywhere,
   so RSC degenerates to the already-closed PNRA invariant. Duplicate work.
2. **ISA drift.** ARM64 `ADRP`+`ADD`, Thumb IT blocks, x86 length-changing prefixes:
   any instruction-aware derived mask must agree bit-for-bit encoder/decoder or it is
   a **correctness** bug, not a ratio loss. RSC avoided this surface by never
   disassembling; every successor that adopts a length decoder inherits it.
3. **Both failure directions are structural, not corpus-specific.** A file where every
   slot satisfies the relation gives enormous runs and a worthless win; a file with one
   non-relative slot per 16 bytes gives zero. Neither is visible from a small sample —
   which is why the closure run must cover the whole stratum.
4. **Cross-section grants.** Measured worth only ~0.30 pp and a format coupling. If a
   successor claims it, it must be charged, never presented as free.
5. **Memory safety.** Every token must validate `d ≥ 1`, `q = p − d ≥ 0`, `q + L ≤ p`,
   `p + L ≤ n`, `L ≥ 4`, `L ≡ 0 (mod 4)`, `mode ∈ {0,1}`. My self-test exercises
   **one** malformed case; two of my four bugs were silent out-of-bounds writes.
   **Remote-only fuzzing is mandatory before any claim.**
6. **Bar laundering.** A win visible only against raw Brotli in this lane is struck
   (`RESEARCH_LEDGER.md` §5b). Mandatory dual bar.
7. **Splice prohibition.** §1.2/§1.3 (corpus anchors) and the System32 anatomy come
   from different strata and are **not** combined into any single figure. Neither are
   they combined with Track 09's `XL-MEASURED` ratios, which come from a different
   harness, host and frozen suite.
8. **Contamination.** The `pe-*` anchors are consumed `known_stress`. Every number here
   is discovery/control evidence. No promotion claim may cite them.

---

## 11. Minimum prototype — status

Built, self-tested, measured: `prototypes/swarm-2026-10-02/14-executable-compiler/space-bunny/`
— `rsc_probe.cpp` (PE section parser, invariant hash index, run growth, greedy cover,
exact reference encoder + decoder, 5-case self-test), plus `rsc_probe.exe`,
`coverage_run.txt`, `coverage_sysbin.txt`.

**Proves:** decoder semantics exact/total/state-free; the transform is genuinely
zero-bit, exact and AUDIT-7/G4-amended-compliant; contiguous coverage ~0.01%; and
— via the R2 arm — a **three-point bracket on the contiguous position**: derived-set
0.0107%, aligned-probe sparse 1.591%, realized (masked) 0.0566%. It does **not**
bound the sparse family, for the alignment reason in §1.3.

**Does not prove:** anything about compressed ratio or throughput. No compression and
no timing were run, by mandate. This is a byte-only feasibility measurement.

**Deliberately not built:** any codec, parser integration, `src/anvil.cpp` change,
`FORMAT.md` entry, or bench row. Nothing wired into production.

---

## 12. Recommendation

# KILL (RSC) · CLOSED (H2/C1 novelty) · CONTROL (global BCJ) · HOLD (sparse engineering)
# Track 14 lane-level ruling: HOLD — agreeing with the paired critic.

1. **KILL RSC.** Measured 0.0107% contiguous coverage; killed structurally by the
   granularity trap (§3). Per the critic this is *redundant rather than informative* —
   RSC is a stricter special case of already-closed PNRA — but it is unambiguous. No
   codec, no parser pilot, no further variants of the contiguous derived-run form.
2. **H2 / C1 novelty is CLOSED and is not reopened here.** Per Track 20, xz's
   documented `lzma_options_bcj::start_offset` ships the insight; per Track 09 X7, the
   shipped invariant **is** BCJ's canonical quantity; per the critic, the deeper threat
   is **US7676506B2**, which teaches the same algebra two-file. This report does not
   preserve the novelty.
3. **Global BCJ / E8-E9 is the mandatory engineering control** on every executable
   arm, not a novelty position and not a rival. Any executable claim without an `A3`
   arm is VOID — and the control is not cosmetic: it is measured in-repo at
   **0.0–7.9%** on corpus PEs, so it dominates every unfiltered executable number in
   this project's history.
4. **HOLD the sparse transformed-reference remainder** as a narrow mask/framing
   economics question with **no novelty claim**, pending the byte-only closure run of
   §9 plus the instruction-aligned field-population count. Its terminal outcomes are
   KILL or HOLD-pending-lock.
5. **A broad executable novelty lane is NOT recommended and is not left open.**

**Transferable results for the coordinator's reconciliation** (the durable value of
this track):

- **Check derived-set density before building a derived-set mechanism.** An exact,
  zero-bit, decoder-derived transform is worth roughly
  `density(derived set) × bytes saved per element`. Where information lives in a
  *sparse* set, derived-set mechanisms have a hard ceiling near zero — here 0.0107%.
- **Probe at the field's real alignment, or do not claim a ceiling.** My own scan is
  the cautionary case: a 4-byte-aligned probe under-saw the `rel32` population by
  100–1000× and I initially mislabelled a lower bound as a family ceiling. A coverage
  argument is only as aligned as the probe that produced it.
- **Use the already-recorded Exp. Z diagnosis, not a new scan, to argue cost.**
  Implicit-Δ derivation is exact only when the relation holds exactly; on real binaries
  it usually does not, so the surviving gain is the one that pays bits for Δ. Exp. Z
  and Exp. X are the same failure at two granularities.
- **Bracket a transformed-reference position with both arms before building a codec.**
  R1 (derived) and R2 (relaxed) cost one linear pass each and bracket the *contiguous
  derived-set* question at zero. They should have been run before the first TCOPY
  prototype. Neither arm bounds the masked family — only an instruction-aligned probe
  can, and nobody has run one.
- **Measure addressable fraction and realized value separately, and never splice
  them.** Here they differ by ≥28×, and that ratio — not either endpoint — is the
  deciding fact
  for any mask-carrying transformed reference in this project.
- **Do not launch the decoder-disassembler route.** It is Courgette (Track 20 `N2`),
  it adds an unforgiving correctness surface, and it needs a cached decoder-side side
  structure that destroys the one property (zero decoder state) that made RSC worth
  building.