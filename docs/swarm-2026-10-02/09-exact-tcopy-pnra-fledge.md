# Track 09 — Exact Transformed Backreferences (TCOPY / PNRA)
## Independent adversarial audit — Fledge Alpha Free — **FINAL rev. 2**

**Rev. 2 changes:** (1) a 1006× arithmetic error is corrected and a retraction of my own
rev.1 reasoning is recorded (§0, §4.2); (2) Track 20's prior-art **kill of H2-as-novelty** is
adopted as binding (§4.4); (3) my remote-dispatch recommendation is **withdrawn** (§9).
Measured facts M1-M8 are unchanged and were independently re-derived.

**Read before use:** `09-exact-tcopy-pnra-space-bunny.md` (constructive),
`20-priorart-killteam-space-bunny.md` (kill team), `03-heldout-corpus-space-bunny.md`
(corpus protocol). **Cross-lane note:** I was asked to read *both* Track 03 reports; only
`03-heldout-corpus-space-bunny.md` exists — there is no `03-heldout-corpus-fledge.md`.

**Independence.** No Space Bunny *output* formed my measured facts. Every number in §2 is a
line-referenced read of `src/anvil.cpp`/`FORMAT.md`/`RESEARCH_LEDGER.md`, or arithmetic on
committed CSVs. No benchmark was run; the brief forbids local corpus measurement. **No
prototype created.**

---

## 0. Record correction — verified, and it invalidates my own rev.1 reasoning

**The correction is verified.** Track 20's critic reports that the parameter-axis prize is
633.75 B (0.036 %), not 0.63 B (0.000036 %). Independently recomputed:

```
5,070 phrases × 1 bit = 5,070 bits = 633.75 bytes
633.75 / 1,761,776   = 3.597e-4 = 0.035972 %  ≈ 0.036 %
error factor         = 633.75 / 0.63 = 1,006×
```

The origin is a bits→bytes conflation: 5,070 bits were divided by ~8,000 instead of 8. The
error is in the **constructive lane's** report §3.1 ("≈0.63 B out of 1,761,776 B — about
0.000036%").

**I adopt the correction and retract my rev.1 concession R2 in full.** In rev.1 I wrote:

> "an implicit-vs-transmitted test confined to rel32 cannot produce a resolvable difference…
> A1 ≥ A2 expected null… **NOT a kill condition** — unresolvable by construction."

That was **wrong in its premise**. Compressed byte counts in this project are deterministic
and exactly reproducible (`I10-FRONTIER-RECON-2026-09-24.md:24`, "Bytes are deterministic"), so
there is no statistical-power problem in a byte-count A/B at all. A 633.75 B delta is
trivially measurable. **The parameter axis is measurable; it is not material.** Both my
error and the constructive lane's error pointed the same direction (dismiss the axis), so the
*verdict* survives while the *reasoning* was wrong on both sides. Recording that explicitly
rather than quietly keeping a right answer for a wrong reason.

**Why the axis is nonetheless immaterial — and this is the decisive number of the audit.**
The 5,070-phrase figure belongs to a **Linux `.text` configuration the project has already
abandoned**. In the configuration that actually runs, the transform-field commit counts are
29 (`anvil.exe`) and 80 (`anvil_bench.exe`) (`RESEARCH_LEDGER.md:3294`, frozen γ=0.5):

| configuration | transformed phrases | parameter-axis prize (1 bit each) |
|---|---:|---:|
| Linux `.text` (abandoned) | 5,070 | **633.75 B** = 0.036 % of file |
| Windows legacy rule | 429 / 1,599 | 53.6 B / 199.9 B |
| **Windows frozen γ=0.5 (live)** | **29 / 80** | **3.6 B / 10.0 B** |

**In the live configuration the entire implicit-parameter advantage — the only separator
left to this family once Track 20's placement kill is adopted — is under 10 bytes.** On a
1.9 MB file. That is not a novelty separator; it is a rounding artefact.

---

## 1. Final recommendation

| Item | Verdict |
|---|---|
| **H2 / computed-topology as a NOVELTY** | **CLOSED** — Track 20 prior-art kill adopted (§4.4). No non-BCJ mechanism found (§4.5) |
| **Zero-bit Δ provenance as a NOVELTY** | **CLOSED BY DEFAULT**, pending Track 20 pair reconciliation (§10). Immaterial in the live configuration regardless (§0) |
| **PNRA as shipped** (`--pnra=on`) | **KILL** |
| **TCOPY implicit Δ=−d** (mode 14) | **HOLD as `{engineering}` / adopt-class.** Correctness survives intact; it is not novelty |
| **Remote A1-A5 matrix** | **DO NOT DISPATCH** — withdrawn (§9). The ≤0.02 % engineering prize cannot close a 20.5 % deficit |
| **Any promotion / generalization claim** | **BLOCKED-ON-CORPUS** — executable held-out family is currently **zero** |
| **Positive handoff** | The binding constraint is **parse quality**, not transform overhead → Track 02 / Track 17 (§9.3) |

---

## 2. Measured facts (re-derived by me from frozen artifacts)

**M1 — TCOPY's in-repo effect is 0.057 %.** `I10-FRONTIER-RECON-2026-09-24.md:42` gives
ANVIL TCOPY-rANS = 2,607,687 B; summing the 13 `anvil-tcopy-rans` rows in
`tests/benchmark-suite.frozen-bdc90474.csv` gives exactly 2,607,687 B. Against its own control
(sparse-RANS 2,609,164 B) the whole mechanism is **1,477 B = 0.0566 %**. Against a different
existing ANVIL mode (MDL-RANS 2,439,834 B) it is **6.88 % worse**.

**M2 — TCOPY is dominated by brotli q6 on all three axes simultaneously.** Bytes 2,607,687
vs 2,163,836 (**+20.5 %**); encode 10.374 vs 69.262 MB/s (**6.7× slower**); decode 226.017 vs
624.797 MB/s (**2.8× slower**). Source `I10-FRONTIER-RECON-2026-09-24.md:36-43` (Class A,
frozen `bdc90474`). That window self-labels **"ranking-grade, not citation-grade"** with **no
peak-RSS field**, so **no RSS claim about TCOPY is admissible from any existing artifact**.

**M3 — PNRA is a no-op on 11 of 13 frozen files and net-worse on the set.** From
`tests/benchmark-suite.frozen-bdc90474.csv`: anvil.exe 108,518→108,514 (−4 B);
anvil_bench.exe 845,675→846,050 (**+375 B**); other 11 bit-identical; total **+371 B (worse)**.
Independently reproduced at `RESEARCH_LEDGER.md:3291-3293` and by the constructive lane's E5.
On the PE recon corpus (`tests/benchmark-recon-i9.csv`, 6 executables): net −7,893 B
(−0.271 %), **of which −7,442 B is pe-git.exe alone**; 2 of 6 Pareto-positive, 2 mixed,
2 strictly dominated. Aggregate encode 3.028→**2.008 MB/s (−33.7 %)**. Best-ever frozen
configuration (γ=0.5): **−0.0110 % / −0.0021 %** (`RESEARCH_LEDGER.md:3293`).

**M4 — the A/B that was run used the wrong baseline.** `RESEARCH_LEDGER.md:971`: TCOPY loses
to MDL on those executables, 0.406 vs 0.402. Constructive E3, identical finding.

**M5 — the mechanism is code-exact and Δ is genuinely decoder-derived.** Verified in source:
encoder detection `src/anvil.cpp:594-604` (4-aligned, `j+4<=dist`, `t32 == s32 - dist`,
stores `j>>2`, **never writes Δ**); decoder `:2706-2735` (overlap-permitted copy `:2712`, then
`v -= dist` `:2728` using `dist` already read from the `ds` substream `:2708`). Bounds:
`dist==0||dist>out.size()||len>out_len-out.size()` `:2709`; `len>kSparseMaxLen` `:2710`; mask
truncation `:2716`; bits beyond window count `:2722`; `o+4 > start+dist` non-overlap
re-validation `:2726`; full substream-consumption assertion `:2761-2763`. Ledger: round-trip
10 corpus + 2 PEs, 540 fuzz cases PASS (`:946`). **Correctness is the one claim in this family
that no prior-art result can touch.**

**M6 — the PNRA invariant is BCJ's canonical form, by the repo's own comment.**
`src/anvil.cpp:847-848`: *"transformation-invariant index over x86 E8/E9 … I(v,p) = p+4+v
(the absolute branch target, i.e. rip-after-instruction + rel32) is INVARIANT"*; index keyed on
absolute branch target `:866`. That is the quantity a BCJ/E8-E9 filter computes when it
rewrites `rel32 → absolute`, stored in a side index instead of rewriting the stream.
Corroborated by `docs/CONTEXT.md:270`: BCJ-style normalisation improved the ELF payload by
**~117-130 KB** vs TCOPY's **~73 KB**, at 1.6 GB/s transform vs 16-60 MB/s search.

**M7 — the section-only/generalization splice is live in the brief.** `docs/CONTEXT.md:314`
headlines "DENSITY LEG CROSSED … 1,761,776 B vs brotli q4 1,781,130 B (−1.1 %)";
`MASTER-BRIEF.md:23` carries "exact TCOPY/PNRA-style transforms remain one defensible novelty
neighborhood" forward. The committed Class A number is **20.5 % worse than q6** (M2). **Action:**
relabel the TCOPY block in `docs/CONTEXT.md` first line
`[SECTION-ONLY · LINUX · DIRECTIONAL · DID NOT TRANSFER · NOVELTY CLOSED PER TRACK 20]`.

**M8 — the anchor corpus is consumed, not held-out.** Track 03 E8/E9: the 6 `pe-*` binaries
are "open known/anchor data and MUST NOT be relabeled as unseen"; the **executable held-out
family is zero**; ledger `:4831-4833` names corpus availability as the promotion blocker. γ=0.5
was tuned on `anvil.exe` + `anvil_bench.exe` (`:3265-3279`). **Every Track 09 PE datum is
calibration-or-known class.**

---

## 3. Projection vs. fact

**P1** — Linux `.text` legs (1,761,776 / 1,730,689 vs q4; ~5,070 phrases; ~16,500 corrections):
single host, single section, crude 10-stream prototype with ~100 KB of known syntax
inefficiency. Existence evidence of headroom only.

**P2** — anatomy (~81 % of *sampled anchor-matched candidates* carry ≥2 −distance fields
explaining ~46 % of mismatch bytes): candidate-selected sample, selection bias unquantified.

**P3** — **μ (covered fraction) is never measured anywhere.** Every mask-cost figure is
therefore per-covered-byte, not per-file. Binding rule, independently reached by both lanes:
**no per-file mask-cost claim is admissible until a run reports μ.**

---

## 4. Reconciliation

### 4.1 Agreements with the constructive lane
M3=E5, M4=E3, E4 (explicit-Δ control unbuilt), the framing-cost diagnosis, the
section-only/generalization contradiction, and "G1 more likely to fail than pass". No
disagreement on any measured number.

### 4.2 R2 — RETRACTED (see §0)

**Wrong premise:** I treated "≈1 bit per phrase" as making the parameter axis *unmeasurable*.
Byte counts are deterministic; the axis is measurable at 633.75 B / 0.036 % on the abandoned
Linux configuration and **3.6-10 B on the live one**. The correct verdict is *immaterial*, not
*unresolvable*. Consequence: A1 is a legitimate deterministic arm; its expected sign is
**A1 ≥ A2 by ~4-10 B**, and that magnitude is itself the finding.

### 4.3 Where I maintain disagreement with the constructive lane

**D1 — G1 is arithmetically unreachable and its failure mode is mis-specified.** G1 requires
`A3 ≤ 0.995 × A2` (≥0.5 % better than mode 14). The mechanism's entire measured effect on this
family, best-ever, is **0.0110 %** (M3) — G1 demands **45-250×** that. The constructive lane
pre-registers N1 = *"A3 > A2 ⇒ computed topology does not pay for itself ⇒ KILL on rate."*
**I contest N1's wording:** a G1 fail driven by low phrase population does not show topology
is worthless; it shows the mechanism barely fires. Recording that as a topology null is a
**false negative**. N1 must split: harness-internal A3-vs-A2 non-positive ⇒ `TOPOLOGY-NULL`;
harness-internal positive but end-to-end non-positive ⇒ `INCONCLUSIVE-ON-COVERAGE`, routed to
the corpus blocker, not to KILL. *(Now moot for dispatch purposes — §9 — but the wording must
not enter the ledger as-is.)*

**D2 — stock BCJ is the wrong placement control.** Both the constructive lane (§7.1 `A4`) and
Track 20 (F1) use "global BCJ + same backend". Stock xz BCJ uses a crude E8/E9 heuristic that
does not disambiguate instruction boundaries; a global pass using the *identical recognizer R*
would dominate it, so a loss to stock BCJ says nothing about placement.

**D3 — `A5` is double-labeled.** The constructive lane lists "relocation-normalized control"
beside `A0…A4` as if it were an external bar; it is a *global-placement* arm. Only the
identity-class control (`A6`) is a correctness invariant.

**D4 — framing cost: measured beats my raw figure.** `RESEARCH_LEDGER.md:3257-3259` records
measured coded/raw ratios (masks 0.435, tmask 0.153); the constructive lane's recomputation
gives **0.473 coded bits per covered byte** (7.57 B / 128 covered B), ~2.6× below my raw
1.25 bits/byte. **I adopt the measured figure**; my structural rates remain exact as
wire-format facts, my economic inference from them is downgraded to a hypothesis.

### 4.4 Track 20's prior-art kill — ADOPTED AS BINDING

Track 20's constructive lane independently killed H2-as-novelty on **BCJ
`start_offset`/`pos` semantics**: H2's decoder kernel is, statement for statement, a BCJ/E8-E9
normalization pass restricted to a source span, and Track 20 rates F1 **"fatal to the whole
surviving space."** Track 20's critic reaches a related placement kill.

I had already reached the same conclusion from an independent direction — M6, that the shipped
PNRA invariant *is* BCJ's canonical quantity, identified in the repo's own source comment — and
I had additionally flagged that BCJ out-gains the mechanism on the project's own numbers
(~117-130 KB vs ~73 KB). **I therefore adopt the kill rather than contesting it.**

Two consequences, stated separately because they are different kinds of statement:

1. **Reference-local placement is NOT a defensible novelty separator.** The decode action is
   identical to BCJ's; only the trigger and the scope of the pass differ. The project's own
   ruling already said this (`gate-priorart-audit-i8.md:28-31`: per-reference placement "lands
   in the crowded copy-with-edits bucket… where placement alone is not novelty"). Track 20 has
   now supplied the claim-level reason.
2. **TCOPY correctness is untouched by this.** M5 stands: exact, bounded, strictly validated,
   zero-parameter. A correct adopt-class capability is not a novelty and was never one.

### 4.5 My search for a concrete non-BCJ mechanism — NEGATIVE, reported

Per the coordinator's condition ("treat novelty as closed **unless** you find a concrete
non-BCJ mechanism"), I searched and **did not find one**. Candidates examined and rejected:

| candidate | why it is not a non-BCJ mechanism |
|---|---|
| **ABS32 co-translating `Δ=+d`** (constructive `C2`) | still position-derived (`d = p−q`), hence BCJ-shaped; and the 12-byte-triple rule is an unsupported structural guess with **zero** evidence — it is blocked on a corpus that does not exist and on an unbuilt Courgette claim-level audit |
| **Periodic / overlapping transformed references** (`dist < len`) | genuinely non-BCJ in that BCJ has no self-reference or iteration, but it is the *same additive transform applied iteratively*, not a new mechanism. Currently excluded by the hard non-overlap invariant (`FORMAT.md:491-492`); relaxing it is a real correctness surface. **Zero measurement exists.** Not promotable on evidence |
| **Field referencing a decoded location outside the window** | requires a side structure ⇒ belongs to the paged-dictionary / cross-block lanes (Track 01 / 15), not TCOPY |
| **Content-derived (non-positional) parameter** | e.g. a field whose delta is fixed between two template occurrences | collapses into encoding the residual, which is the ordinary LZ path |

**Conclusion: the novelty search returns empty.** Under the coordinator's stated rule,
novelty is therefore **CLOSED**.

---

## 5. The remote matrix — demoted, retained on the record only

Retained because its *design* is sound and may be executed under a future corpus; **not
recommended for dispatch** (§9).

**Name:** `tcopy-fixed-multiset-2x2`. GitHub Actions, R1 byte-only.
One token multiset **T**, five representations, one parse, one candidate set, replayed.

| Arm | Parameter | Topology | Placement |
|---|---|---|---|
| **A0** | — | — | exact-LZ + literals only (transformed tokens refused) |
| **A1** | **transmitted** explicit Δ | transmitted | reference-local — isolates the parameter axis (now **powered**, §0) |
| **A2** | derived Δ=−d | transmitted | reference-local — mode 14's wire, verbatim |
| **A3** | derived Δ=−d | **computed by R** | reference-local — H2 |
| **A4b** | n/a | computed by the **identical R**, whole-file pre-pass | **global — the placement isolator** (D2) |
| **A4s** | n/a | stock xz/7-Zip BCJ | global — adopt-class reference only |
| **A5** | identity-class R | computed | must be **byte-identical to A0** |

**Identical parser/search conditions (machine-checked):** one parse, one candidate
enumeration, one acceptance-decision log replayed into A0-A3; no arm re-runs search; **no
post-parse candidate dropping** (E7: dropping always enlarges the block); identical block size,
backend, entropy coder, model selection, router; **R is one hash-pinned implementation shared
by A3 and A4b** or the placement verdict is void; non-overlap `i+4≤d` hard at recognition,
deserialization, and apply.

**Complete charged wire — every arm publishes:** token-type stream; length varints; distance
varints; literals; residual mask (coded); transform mask (coded, zero-length in A3); transform
parameter bytes (zero in A2/A3, ~1 bit/phrase in A1); **per-substream length varint AND
entropy model header for every declared substream including empty ones** (a removed stream is
not free if the format still declares it — A3 must shorten the declared stream list and pay for
the list length, or pay for all 8); block-header/framing delta; auto-detection bytes (AUTOSEG
precedent +24…+48 B/file); per-block integrity field; and **μ, token count, site count,
phrase-length histogram** (P3). Encoder costs published separately: PNRA's uncapped per-block
`unordered_map` (`src/anvil.cpp:858-868`) is ≈60-70 B per relocation field, zero decoder
benefit, measured −39.5 % encode on pe-git.exe.

**Declared asymmetry:** A4b/A4s cannot share **T** — a global pre-pass changes the bytes before
the parser runs. The placement comparison is necessarily whole-pipeline and carries an
irreducible trajectory confound (E7 measured counter deltas of −9/−17 beyond direct flips on
429/1,599 commits). Required: measure trajectory sensitivity in-run; if it is ≥50 % of
|A3−A4b|, record `INCONCLUSIVE-ON-TRAJECTORY`.

**Corpus (Track 03, binding):** the 6 `pe-*` files and the `anvil*` pair are **KNOWN-ANCHOR /
calibration class** — valid for attribution, **never for promotion** (M8). Validation requires
a **new locked independent executable family** (protocol §13: ≥3 executables, ≥2 producers,
≥2 toolchains). Executable held-out family: **zero**.

---

## 6. Hidden-cost audit (final)

| Item | Charge | Basis |
|---|---|---|
| Transform parameter Δ | **0 bits** in A2/A3; **~1 bit/phrase** in A1 = 3.6-10 B in the live config | `src/anvil.cpp:2728`; §0 |
| Transform topology mask | 0.25 raw bits/byte of phrase; coded 0.153 of raw | `:2622-2634`; measured `RESEARCH_LEDGER.md:3257-3259` |
| Residual mask | 1.00 raw bits/byte; coded 0.435 of raw | same |
| Combined coded framing | **≈0.473 coded bits per covered byte** (7.57 B / 128 covered B) | constructive §3.2 — **adopted over my raw figure** (D4) |
| Per-token descriptor | **0 in the intra-block score**; charged only at block routing `:4764` | code read |
| Decoder state / RSS for A3 | 0 B; +1.5-3 KB `.text` ESTIMATE | constructive §3.4 |
| Decoder worst-case transient alloc | +14.3 % (7→8 substreams ⇒ 112×→128× `out_len`) `:2592,2653` | **inherited**, amplified; not a new vuln |
| Encoder index (PNRA) | ≈60-70 B/relocation field, uncapped, per block; −39.5 % encode on pe-git.exe | `:858-868`; recon CSV |
| Peak decode RSS | **not recorded in any artifact** | M2 window caveat |

**Uncalibrated objective (retained finding).** `scan_candidate` (`:590-614`) charges 0.1 per
transform field, `litcost[v]+0.125` per residual, `avg_lit−0.125` per matched byte, and
**nothing** for the per-token descriptor or the mask bit rate. With hundreds of thousands of
tokens the uncharged descriptor dominates the objective. So M1's 0.057 % is an artifact of a
mispriced objective, not a bound. *Now moot for dispatch (§9) but it is the correct
explanation of why the family underperforms MDL (M4):* the family is losing on **parse
selection**, not on transform economics.

**Base rate.** Implicit-derived parameters have lost every head-to-head in this project:
ARI-REF transmitted-Δ −65.25 % vs implicit-Δ +38.35 % (`gate-priorart-audit-i8.md:195-199`);
PR-5 transmitted-step 12,921 B vs derived 12,936 B. **0 for 2.**

---

## 7. Strongest falsification case

> **Three independent lines now converge.** (i) Track 20's claim-level kill: H2's kernel is a
> BCJ `start_offset`/`pos` normalization restricted to a span, so placement is not a separator.
> (ii) My M6: the shipped PNRA already computes BCJ's canonical quantity, named in the repo's
> own source comment. (iii) On the project's own numbers the global public-domain filter
> out-gains the novel mechanism on its target data (~117-130 KB vs ~73 KB).
>
> And the residue is not merely unnovel — it is immaterial. The implicit-parameter advantage
> that survived the kill is **3.6-10 B in the live configuration** (§0). PNRA is bit-identical
> on 11 of 13 frozen files and **net +371 B worse**; its best-ever configuration is −0.011 %.
> TCOPY fires on 0.057 % of corpus bytes and is **dominated by brotli q6 on bytes, encode, and
> decode simultaneously** (M2). The one real positive datum — pe-git.exe −7,442 B — is a single
> file, on a consumed anchor, bought at −39.5 % encode.
>
> There is no non-BCJ mechanism (§4.5). There is no validation corpus (M8). The density leg
> that justified the family is section-only and did not transfer (M7).

**Strongest surviving case, at equal strength.** The representation is genuinely exact,
strictly validated and zero-parameter — verified in source, not asserted (M5), with 540 fuzz
cases passing. It produces real, reproducible, non-zero improvements on exactly the family it
was designed for: pe-git.exe −7,442 B; pe-ninja.exe and pe-where.exe positive on all three
axes. That is not noise. It is also, now: not novel (Track 20), not material (≤10 B on the
parameter axis), 20.5 % behind q6 (M2), on consumed anchors (M8), and losing to an existing
non-novel ANVIL mode (M4). **A correct, cheap, adopt-class capability — and nothing more.**

---

## 8. Failure modes of my own argument

- **I was wrong in rev.1 for a wrong reason** (§0/§4.2) and I retract it rather than preserve a
  correct conclusion on a false premise. A reader should assume my arithmetic warrants
  re-verification; §0 shows how.
- **If Track 20's pair reconciliation restores the provenance axis**, my `CLOSED BY DEFAULT`
  becomes contested — but §0's materiality finding stands regardless, so the *recommendation*
  would not change. Only the label would.
- **pe-git.exe may be unrepresentative.** If the effect is git-specific the label is
  `{narrow capability}` — still not a frontier contribution.
- **`FORMAT.md`'s mode-14 spec was not verified against source**; M5 is source-verified only.
- **Track 03 findings are single-sourced** — no fledge report exists for that track.
- **I did not run the matrix**, so every claim about what it would return is analytic from
  measured ratios, not measured.

---

## 9. Recommendation and the withdrawal of my remote-dispatch recommendation

### 9.1 The ≤0.02 % prize cannot close a 20.5 % deficit

The coordinator asked me to reconcile the ≤0.02 % projected end-to-end prize against any
remaining PILOT recommendation. It does not survive contact with the frontier numbers:

```
TCOPY − q6 byte deficit           = 443,851 B
upper bound on ALL removable overhead (0.02 % of TCOPY) = 521.5 B
521.5 / 443,851 = 0.1175 %  of the deficit closed by removing 100 % of the overhead
decode gap: 226.0 vs 624.8 MB/s = 2.76×, untouched by a byte-only job
```

**Optimising a 0.02 % overhead inside a configuration that is 20.5 % behind is noise
reduction on an already-losing configuration.** No ordering of the A1-A5 arms changes that.
Further, the one component the matrix would actually measure — topology cost — is bounded
analytically from the *measured* coded ratios (0.473 coded bits per covered byte, §6), so the
job would confirm an arithmetic bound rather than discover a quantity.

### 9.2 Revised recommendation

| Item | Verdict |
|---|---|
| Dispatch `tcopy-fixed-multiset-2x2` | **WITHDRAWN — DO NOT DISPATCH.** Supersedes my rev.1 `PROMOTE-TO-REMOTE`. Reasons: (i) novelty is closed by Track 20 (§4.4) and my own search returns empty (§4.5); (ii) the prize is ≤0.02 % against a 20.5 % deficit, and the deficit is decode-side (§9.1); (iii) the binding project blocker is corpus, not this experiment (M8). |
| **A1 (transmitted-Δ) arm only** | **DEFERRED, not dispatched.** It is the one arm that would convert "never executed" into a documented negative, and at ~50 lines it is cheap — but it yields a ≤10 B datum on consumed anchors. Execute only if a future new executable family exists and the question is worth 10 B there. |
| Record the closure in `RESEARCH_LEDGER.md` | **RECOMMENDED**, with the D1 wording fix so no false `TOPOLOGY-NULL` enters the record. |
| PNRA `--pnra=on` | **KILL**; keep default-off (`src/anvil.cpp:3780`); retain the framework text only. |
| TCOPY mode 14 | **HOLD as `{engineering}` / adopt-class.** Keep the capability; claim nothing. Preserve the hard non-overlap invariant. |
| Cost-model calibration of `scan_candidate` | **NOT WARRANTED** — see §9.3. |
| Any generalization/promotion | **BLOCKED-ON-CORPUS** — executable held-out family is zero. |

### 9.3 Positive handoff (the one thing worth keeping)

M4 and §6 together locate the real constraint: TCOPY loses to plain MDL (0.406 vs 0.402)
because of **parse selection**, not transform economics. That axis belongs to **Track 02
(G5A-informed finalist planner)** and **Track 17 (parser/candidate-generation algorithms)**,
not to this family. Closing TCOPY/PNRA with an explicit, measured reason — *correct,
non-novel, immaterial, and losing on parse quality* — is a better outcome than spending a
remote slot confirming a bound we can already compute.

### 9.4 Actions filed (none executed)

1. Relabel the TCOPY block in `docs/CONTEXT.md` `[SECTION-ONLY · LINUX · DIRECTIONAL ·
   DID NOT TRANSFER · NOVELTY CLOSED PER TRACK 20]` (M7).
2. Relabel the constructive lane's §7.2 "six locked held-out PEs" as `KNOWN-ANCHOR stratum`
   (M8).
3. Amend constructive N1 into `TOPOLOGY-NULL` vs `INCONCLUSIVE-ON-COVERAGE` (D1).
4. Correct the constructive lane's §3.1 arithmetic: 633.75 B / 0.036 %, not 0.63 B / 0.000036 %
   (§0) — and record that its "underpowered" conclusion must be restated as *immaterial*,
   with the live-configuration figure of 3.6-10 B.
5. Do **not** enter "H2/SLX-REF" in `research-agenda.md` as a live novelty family.
6. Carry the ≤0.02 %-vs-20.5 % arithmetic into the ledger entry so the family is not
   re-litigated from the section-only density numbers.

---

## 10. Standing condition on the final novelty verdict

Per the coordinator, the **final** novelty verdict waits on Track 20's pair reconciliation,
because its two lanes disagree on whether **zero-bit Δ provenance itself** remains defensible
after the BCJ kill. My position, stated so it can be falsified:

- **Default: CLOSED.** BCJ already occupies the derived-parameter, zero-transmitted-bit,
  exact, decoder-computable-from-reconstructed-state position for this exact field class. A
  genus that old (predictor-relative coding: DPCM, FPC, Gorilla, BCJ) cannot be claimed at the
  species level by an instance of it — and the project's own audit says as much
  (`gate-priorart-audit-i8.md:187-193`, `:218`).
- **I could not construct a non-BCJ mechanism** (§4.5), which is the condition the coordinator
  set for keeping novelty open.
- **Independently of that reconciliation, the recommendation does not change**, because §0
  shows the surviving separator is worth **under 10 bytes** in the live configuration. Even if
  the provenance axis were restored, it would restore a ≤10 B engineering margin on consumed
  anchors — `{engineering}`, never `{novelty}`.

*Prepared by Fledge Alpha Free, track 09 critic. Measured facts re-derived from frozen
artifacts; projections labelled. No existing file modified; no reset/clean/stash/restore/
rebase/commit/push; no local benchmark, sweep, or fuzz campaign; all measurement
GitHub-Actions-only. Only `docs/swarm-2026-10-02/09-exact-tcopy-pnra-fledge.md` was created.*