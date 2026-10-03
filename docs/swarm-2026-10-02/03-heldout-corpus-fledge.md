# Track 03 — Held-out Corpus Methodology: Independent Adversarial Audit (Fledge Alpha Free)

**Agent:** Fledge Alpha Free (independent adversarial reviewer)
**Date:** 2026-10-02
**Lane:** 03-heldout-corpus
**Status:** INTERIM — issued before `03-heldout-corpus-space-bunny.md` exists. Section 9 is a reserved reconciliation slot.
**Ground rule honored:** no Space Bunny report was read or relied upon. Every claim below was reconstructed from primary artifacts in this worktree.

## 0. Evidence labels used throughout

| Label | Meaning |
|---|---|
| **[M]** | **Measured by me in this session**, from primary bytes, reproducible from the isolated tool in `prototypes/swarm-2026-10-02/03-heldout-corpus/fledge/` |
| **[R]** | **Repo-recorded** — asserted by an existing project artifact; I did not re-derive the number |
| **[A]** | **Assumption / projection / judgment** — not evidence |

I do not promote any **[R]** to **[M]** without saying so. This matters because the mandate for this track is precisely about unearned confidence.

---

## 1. Bottom line

The held-out corpus methodology in ANVIL is **well-specified prose and unimplemented infrastructure**. `docs/I10-CORPUS-LOCK-PROTOCOL.md` is 825 lines of genuinely rigorous specification — and **zero** of its mandatory machine-readable artifacts exist. Worse, the corpus that actually gets measured fails the protocol's own diversity floor for the specific claims the current frontier work is making.

**Strongest kill argument (one sentence):** the project has a strong *held-out label* and no *held-out data* — the only non-self-authored bytes in the corpus are six host-installed Windows/PC executables, they are untracked in git, they cannot even be expressed in the protocol's own schema, and they appear in **zero** rows of the published benchmark suite.

**Recommendation: PILOT** (leaning HOLD for any promotion language). See §10.

---

## 2. What actually exists vs. what the protocol mandates

`docs/I10-CORPUS-LOCK-PROTOCOL.md` §5.3 and §10 mandate these artifacts. Measured presence in this worktree **[M]**:

| Mandated artifact | Present? |
|---|---|
| `corpus-lock.json` | **No** |
| `corpus-lock.sha256` | **No** |
| `corpus-access-ledger.jsonl` | **No** |
| `dedup-report.json` | **No** |
| `publisher-split-report.json` | **No** |
| `license-report.json` | **No** |

A repository-wide search for `corpus-lock*` returns nothing **[M]**.

Against the protocol's own §11 blockers this means **PB-01** (missing corpus lock), **PB-09** (missing contamination ledger), **PB-10/PB-11** (no independence audit exists to be violated or satisfied), and **PB-26** (no genuinely new held-out family) are all live. Every promotion path in §12 is blocked at step 1.

**Severity: this is not a paperwork gap.** Steps 1–2 of §12 gate everything downstream. Until the lock exists, no amount of ratio evidence can be promoted, and any claim of "held-out validation" in a ledger or prereg is currently unsupported by artifact.

---

## 3. Corpus inventory and independence-unit accounting

### 3.1 What is in `tests/corpus` **[M]**

24 files, 21,127,883 B total (per `tests/corpus/README.md:3`, re-verified by hash sweep; all 24 SHA-256 values I computed match `tests/corpus/CHECKSUMS.txt` exactly **[M]** — this part of the corpus is honest).

Lineage, read from generators and git, not from prose:

| Generator / origin | Files | Independence units |
|---|---|---|
| `tests/make_smoke_corpus.py` | `generated.json`, `generated.jsonl`, `generated.log`, `generated.repeat.jsonl`, `generated.sqlite`, `random.bin` | **1** |
| `tests/make_synth_corpus.py` | `synth-timeseries.bin`, `synth-arith.bin`, `synth-jitter.bin`, `synth-columnar-align.bin`, `synth-drift-stride.bin`, `synth-counters.log`, `synth-telemetry-f64.bin`, `synth-ndjson-columnar.ndjson` | **1** |
| ANVIL itself (self-authored) | `anvil.exe`, `anvil_bench.exe`, `src.cpp`, `doc.md`, `README.md` | **inadmissible as evidence** (see §6) |
| Host-installed binaries | `pe-winver.exe`, `pe-where.exe`, `pe-notepad.exe`, `pe-python.exe`, `pe-ninja.exe`, `pe-git.exe` | **2** at best (see §3.3) |

### 3.2 The headline: independence units, not file count

**24 files reduce to ~4–5 admissible independence units [M].** The protocol §5.1 rule 8 explicitly blocks "shared generator lineage for synthetic data" from independence. All 8 `synth-*` files come from **one script, one author, one seed family (90001–90008)** **[M, `tests/make_synth_corpus.py:52,73,95,130,163,203,240,255`]**. All 6 `generated.*` come from one 32-line script **[M, `tests/make_smoke_corpus.py`]**.

So the corpus presents 14 synthetic files that look like 14 independent families and are, by the protocol's own rule, **two**.

This is the single most important methodological finding: **file count is being used as a proxy for evidence count, and the protocol forbids exactly that proxy.** Any gate written as "N synthetic files" is satisfiable by one generator emitting N files.

### 3.3 Executable family: diversity claim vs. reality

`tests/corpus/README.md:65` claims diversity "across producer, toolchain, and size". PE header inspection **[M]** (parsed `e_lfanew`, machine, TimeDateStamp, optional-header magic, linker version):

| File | Linker | Producer |
|---|---|---|
| `pe-winver.exe` | 12.1 | Microsoft (MSVC) |
| `pe-where.exe` | 12.1 | Microsoft (MSVC) |
| `pe-notepad.exe` | 12.1 | Microsoft (MSVC) |
| `pe-python.exe` | 12.1 | Microsoft (MSVC) |
| `pe-ninja.exe` | 12.1 | Microsoft (MSVC) |
| `pe-git.exe` | 12.1 | Microsoft (MSVC) |

**All six report linker 12.1** **[M]**. `tests/corpus/README.md:76` states `pe-git.exe` is "the only non-MSVC-built PE in the corpus (MinGW-w64/GCC)". The PE *optional header* linker version does not distinguish toolchains (MinGW-w64 links with `ld` but still emits a 12.x-style optional header), so **this field cannot corroborate the README's toolchain-diversity claim** **[M]**. The claim may well be true; it is **not evidenced by the corpus itself**. Under §10.2 provenance honesty, toolchain diversity must be attested from producer metadata, not inferred from a header field that does not encode it.

Producer diversity is genuinely weak: 3 of 6 are `%SystemRoot%\System32` binaries from **one OS release (22H2, build 10.0.22621.1)** **[R, README:71-73]**. Protocol §5.2 requires "source repository owner and organization MUST be new" — Windows system binaries have no repository owner at all.

**Honest score: 1 of 6 executables is from a genuinely different producer. Executable toolchain diversity = 1 of 6.**

### 3.4 A real duplicate-lineage hit inside the "held-out" set

Exact 4 KiB block-set intersection across all 24 files **[M]** (protocol §5.1 rule 3):

```
4KiB-OVERLAP pe-where.exe <-> pe-winver.exe: 1 shared block
```

**This is a measured protocol violation inside the held-out verdict set.** Two files that count as two independent held-out executables share an exact 4 KiB block. Per §5.1, this may only be resolved by placing both in the same independence group — it **must not be waived to count as independent evidence**. Two 28 KB / 61 KB Windows binaries from the same OS build sharing a 4 KiB block is unsurprising in hindsight (shared runtime/CRT data), but it demonstrates the audit is **load-bearing, not decorative**, and that the set has never actually been audited.

Note the asymmetry worth flagging to the coordinator: my whole-corpus 4 KiB scan found exactly one such collision, and it is inside the held-out set. The `generated.*`/`synth-*` families show **zero** 4 KiB collisions despite being one lineage each — because §5.1 rule 3 is a *content* rule and the synthetic families differ in content. **This is precisely why rule 3 alone is insufficient and rules 6–8 (schema, publisher, generator lineage) are the load-bearing ones.** An audit that implemented only the cheap content rules would have passed this corpus cleanly. That is a design lesson for §7, not a criticism of the rule set.

### 3.5 The worst lineage finding: `generated.sqlite` ⊂ `generated.json`

`tests/make_smoke_corpus.py:31` builds `generated.sqlite` by inserting `json.dumps(rows[i % len(rows)], ...)` — the *same* `rows` list serialized into `generated.json` at line 7.

Measured **[M]**: **12,000 of 12,000** SQLite `payload` values are **byte-verbatim** `generated.json` rows. `generated.sqlite` contains 100% of `generated.json`'s information content, plus SQLite page structure.

So the corpus presents a `structured_json` family and a `sqlite` family that are **one family in two containers**. Under §5.1 rules 6 and 7 (same schema cluster; same producer/generator) they must be grouped. Under protocol §5 they cannot be counted as two independent structured families.

This is the concrete, decisive instance of the general defect in §3.2: **the corpus systematically inflates family count by re-expressing one information source in multiple framings.** JSON→SQLite here; JSON→NDJSON and JSON→repeat-NDJSON in the same generator; binary-columnar and NDJSON-columnar views of *the same value model* in the synth generator **[M, `make_synth_corpus.py:240-267`: both `synth-telemetry-f64.bin` and `synth-ndjson-columnar.ndjson` compute `20.0 + 0.001*i + r.randint(-2,2)*0.0001`]**.

**A codec with a structured-data router will look excellent on this corpus for reasons that have nothing to do with generalization.** The corpus is, in effect, a *re-encoding benchmark for one generator's output model*.

---

## 4. Format-family bias: the corpus does not cover the claims being made

The protocol §13 sets a "minimum new held-out portfolio for broad claims". Scored against what exists **[M]**:

| §13 requirement | Have | Status |
|---|---|---|
| 3 independently published structured JSON/NDJSON families | **0** (only `generated.json`, `generated.jsonl`, `synth-ndjson-columnar.ndjson` — one generator lineage, synthetic) | **FAIL** |
| 2 mixed-validity / malformed-source families with substantial residual material | **0** (no malformed/mixed-validity file exists in `tests/corpus` at all) | **FAIL** |
| 3 executables spanning ≥2 unrelated producers and ≥2 unrelated toolchains | 6 files, but 1 genuinely distinct producer, toolchain claim unevidenced | **WEAK PASS / FAIL on producer isolation (§5.2)** |
| 2 real numeric/telemetry or scientific streams **not** derived from ANVIL synthetic generators | **0** (all 4 numeric files are `make_synth_corpus.py` output) | **FAIL** |
| standard external anchors + AITDCC external testing | AITDCC lock absent **[M]** | **FAIL** |

**This is the core methodological indictment.** The frontier work currently in flight — SPARSE-REF, shape-conditioned displacement, TCOPY/PNRA on ELF, context-switched literals — makes claims about **structured JSON**, **executable code**, and **numeric telemetry**. The corpus's structured and numeric families are **100% synthetic and 100% single-lineage**. The only real-world bytes are executables, and those are the one family where the project has not yet built its main mechanism.

**Consequence: every structured-data ratio number measured on `tests/corpus` is a measurement of the generator, not of the world.** The 0.1325 result on `generated.json` reported in `docs/CONTEXT.md` is a fact about `make_smoke_corpus.py`'s schema, and would survive any mechanism change as long as the router handles that schema.

---

## 5. Synthetic contamination: the deeper problem

Beyond lineage, synthetic corpora have a structural bias the protocol does not currently name, and it should.

**Synthetic data is compressible by construction in a way real data is not.** Every generator here writes from a small Python-level model: a handful of integers with arithmetic strides, or a short f-string template **[M, `make_synth_corpus.py`]**. Real corpora contain entropy that no generator emits — natural-language variance, edit history, vendor-specific formatting quirks, encoding mixtures, adversarial tails.

**Therefore the synthetic families are systematically biased toward the mechanisms ANVIL is building.** `synth-columnar-align.bin` was built specifically so "a finite-difference / columnar mechanism that finds the true field offsets gets long near-constant-delta runs per field; one that scans wrong offsets gets nothing" **[M, `make_synth_corpus.py:126-127`]**. That is a fair *unit test* and a biased *benchmark*. The project's own history shows this trap firing: the synth corpus was added because "Experiments T/U/W found the existing text corpus's periodic/arithmetic signal at the hash-noise floor" **[R, README:55-57]** — i.e. the real corpus was correctly judged to lack the signal, and a substitute was built. That is legitimate as a *mechanism-existence* test. It is illegitimate as *evidence that the mechanism generalizes*.

**Recommendation (novel to this report): reclassify every synthetic family from "evidence" to "adversarial control with known ground truth."** Their correct use is: prove a mechanism *can* fire, prove it does not regress, and measure detection sensitivity. They must never appear in a generalization claim, an aggregate ratio, or a Pareto frontier point. The protocol has no role for this today — §2.1 has `discovery`/`known_stress`/`heldout` but nothing that says "synthetic control."

---

## 6. Self-reference and instrument contamination

`anvil.exe`, `anvil_bench.exe`, and `src.cpp` are **corpus members**. This is a category error **[M]**:

- `src.cpp` is the codec's own source, and `anvil.exe` is the codec built from it. Compressing the compressor's own source with the compressor is a self-referential measurement, not a `source_code` family datapoint.
- `anvil_bench.exe` is the **measurement instrument**. Including the instrument as a measured object means any change to the harness changes a corpus row.
- Measured **[M]**: `build/anvil_bench.exe` SHA-256 `379341d9…` ≠ `tests/corpus/anvil_bench.exe` `fcd30da5…`. The corpus copy is a **stale, different build** from the binary that actually runs measurements.
- `tests/corpus/README.md:177-194` documents a prior *manifest drift incident* on exactly this file, where the corpus copy was silently re-snapshotted from a rebuilt `build/` without updating `CHECKSUMS.txt`, and the old bytes were **unrecoverable from history** because `.gitignore` matches `anvil_bench.exe` **[R]**.

**Severity: high.** This is a measured, already-once-realized instance of the exact failure the protocol exists to prevent — corpus identity drifting from measurement provenance — and it recurred. The response at the time ("RESOLVED … no re-calibration needed") was reasonable for that incident, but it treated a **recurring class** as a **closed incident**. The protocol's answer is §3/§6 fail-closed acquisition; the answer actually applied was to re-pin the manifest to whatever was on disk. That is manifest-laundering, and the corpus copy being divergent from `build/` today proves the class is live.

---

## 7. Benchmark-window provenance and the measurement-window question

`tests/pr-4-measurement-window-protocol.md` is strict and well-built **[R]** — required fields, `cv_pct` vs. `tests/noise-floor.csv` cells, thread-count disclosure, `measured`/`derived`/`projection` labels. Two observations specific to this track:

1. **Ratio/byte claims are declared deterministic and always safe (§6)** **[R]**. This interacts badly with the corpus defects above. A deterministic byte count measured on a single-lineage synthetic family is *precisely reproducible* and *scientifically worthless*. Determinism has been mistaken for validity. Byte-count determinism is a property of the **codec**, not of the **corpus**.
2. The published `tests/benchmark-suite.csv` contains **13 files and none of them is a `pe-*.exe`** **[M]**. The "S6-2 held-out verdict set" contributes **zero rows** to the headline published suite. The held-out set is therefore not load-bearing in the current evidence — the anti-overfit verdict set is decorative relative to the numbers people actually cite.

**This is the strongest single disconfirmation of the "we have held-out validation" framing.**

---

## 8. License and redistribution: an unstated blocker for the only real family

Protocol §4.8 forbids redistribution for `unknown-no-redistribution` and requires such entries to be fetched outside the repository **[R]**. Windows system binaries (`winver`, `where`, `notepad`), CPython, Ninja, and Git-for-Windows carry their own redistribution terms **[A]** — none is a public-domain dedication.

Practical effect: the only non-synthetic bytes in the corpus are the ones least likely to be shippable to GitHub Actions runners as repository artifacts. A held-out corpus that cannot be acquired in CI is a held-out corpus that will silently be re-measured locally. **[A]** but high-confidence: this is exactly the pressure that produced the current state.

---

## 9. RESERVED — reconciliation with `03-heldout-corpus-space-bunny.md`

**Status: not yet available.** When the constructive lane's report appears I will read it, and record here:

- Agreements (with evidence).
- Disagreements, each resolved into one of: *my finding is wrong*, *their finding is wrong*, *both true, different scope*, or *unresolved — decisive experiment owns it*.
- Any place where the constructive lane proposes a mechanism I must red-team separately under §9.1.

**Explicit note for the coordinator:** because this interim was produced with full independence and no access to the constructive lane, any agreement reached later is corroboration, and any disagreement is a genuine signal — neither should be silently reconciled by averaging. Per MASTER-BRIEF §65 the coordinator owns reconciliation.

### 9.1 Pre-committed red-team questions to put to the constructive lane

1. Which specific held-out family does your proposed mechanism claim, and does it exist in a form that satisfies §13 for that claim class?
2. If your mechanism wins on `generated.json`, what would the result be on a JSON family produced by a different generator? What is your falsifier?
3. Does your mechanism's gain survive when `generated.sqlite` is excluded as a duplicate of `generated.json`?
4. What is your per-family independence-unit count (not file count)?

---

## 10. Decisive remote-only falsification experiment (pre-registered)

**No local execution.** Per MASTER-BRIEF §15 and my mandate, this is remote-only. I have run **no** compression benchmark, sweep, or fuzz campaign. My local work was hashing, generator reading, and PE header parsing — no codec invocation.

### 10.1 Design

**Name:** `i10-heldout-v1` — a single GH Actions job whose *first* act is to refuse to run unless the corpus lock exists and passes audit.

**Job order (fail-closed, mirroring protocol §6):**

1. Materialize `corpus-lock.json` + `corpus-lock.sha256`; verify companion hash. **Hard stop on mismatch.**
2. Run the independence audit (`prototypes/swarm-2026-10-02/03-heldout-corpus/fledge/independence_audit.py`, extended to emit the protocol's three reports). Emit `dedup-report.json`, `publisher-split-report.json`, `license-report.json`, and a `grouping-decisions.json`.
3. Compute **independence units**, not file counts, per §13.
4. Acquire fail-closed; verify SHA-256 (never MD5 — see §11).
5. Only then run the paired ratio + throughput matrix.

**The experiment is designed so that the most likely outcome is a corpus gate failure, not a codec result.** That is the point: the binding constraint right now is corpus admissibility, not mechanism quality.

### 10.2 Pre-registered thresholds

Fixed now, before any data is fetched or measured. Not to be moved.

| Gate | Condition | Failure meaning |
|---|---|---|
| **G0 INTEGRITY** | All lock SHA-256, prereg SHA-256, impl SHA, workflow SHA match run bindings | `INVALID_INFRA` (never a scientific loss) |
| **G1 ADMISSIBILITY** | ≥5 independence units; **≥2 structured JSON/NDJSON from unrelated publishers**; **≥1 non-synthetic numeric/telemetry**; **≥2 executable producers**; **0 ungrouped** §5.1 conflicts | **CORPUS BLOCKED** → no promotion, regardless of codec numbers |
| **G2 RATIO** | ANVIL beats brotli q9 on **≥60%** of admissible held-out families **and** aggregate ≥**3%** smaller complete bytes | Mechanism not generalizing → KILL |
| **G3 RATIO-NO-CATASTROPHE** | **Zero** family regressions >**2%** vs q9 | Pareto claim dead → KILL |
| **G4 DECODE** | decode MB/s ≥ **0.8 ×** q9, matched thread counts, ≥5 reps, `cv_pct` within `noise-floor` cell | Decode axis unproven → HOLD |
| **G5 ENCODE** | encode MB/s ≥ **1.0 ×** q9, or an explicitly pre-registered encode-debt policy | Encode axis unproven → HOLD |
| **G6 MEMORY** | decode peak RSS ≤ **1.25 ×** q9 **and** ≤ 8 GiB | Resource claim dead → KILL |
| **G7 ATTRIBUTION** | Ablation isolates the mechanism: router/selector overhead ≤ **3%** of complete bytes, and oracle-regret reported per file | Gain is the router, not the mechanism → KILL the novelty claim |
| **G8 DETERMINISM** | Identical complete bytes + output hash across ≥3 repetitions | Suspicious → `INVALID_INFRA` investigation |

**Kill rule (pre-registered):** G1 failure blocks promotion unconditionally. G2 **or** G3 **or** G6 **or** G7 failure = **KILL** the mechanism. G4/G5 failure = **HOLD** (real but not Pareto). G8 failure = `INVALID_INFRA`, never a win and never a loss.

### 10.3 Why this is decisive

It is decisive in both directions: it can kill a mechanism (G2/G3/G7) *and* it can kill the corpus (G1). Today G1 is failing on three of five §13 counts **[M]**, so the honest expected result is `CORPUS BLOCKED` — which is a **real, useful, publishable-internal finding**, not a wasted run.

---

## 11. Hidden-cost audit

Costs that are invisible unless deliberately charged.

### 11.1 Validation-side cost (paid by CI, not the decoder)

Good news, and it should be stated plainly **[M/M-estimated]**:

| Step | Cost |
|---|---|
| SHA-256 over 21.1 MB | ≈ 21.1 MB @ ~1.5 GB/s (SHA-NI) ≈ **14 ms** |
| 4 KiB block hashing, 21.1 MB | ≈ **5,159 blocks**; pairwise ≈ **13.3M** set lookups ≈ **< 1 s** |
| Block-hash resident set | 5,159 × 32 B ≈ **165 KB** (or 41 KB truncated) |
| MinHash 128 perms over ~6.9 MB text | ≈ 1.1M shingles × 128 ≈ **145M** hash ops ≈ **0.5–1.5 s** 1-thread |

**Verdict: the protocol is affordable by orders of magnitude.** The cost is kilobytes of RAM and under a second of CI time. There is no resource argument for not implementing it. This materially weakens any "we can't afford proper held-out discipline" justification for the current state.

### 11.2 Decoder-visible cost

**Zero, correctly.** Corpus locking, hashing, dedup, and ledger are validation-side; none enters the wire format. This is the right design and I found no leakage of corpus metadata into the bitstream. Recorded explicitly because the mandate asks for it and the answer is genuinely "none."

### 11.3 Cost that IS being paid invisibly

- **Scientific cost of the 2026-08-21 manifest re-pin** **[R]**: unrecoverable corpus bytes plus a re-baselining risk that was argued away rather than measured.
- **Opportunity cost**: the strongest ratio claims in the project rest on corpus families with 1–2 independence units.
- **Reproducibility cost**: 10 corpus files are **untracked** in git **[M]** — all 6 `pe-*.exe` plus `synth-columnar-align.bin`, `synth-drift-stride.bin`, `synth-telemetry-f64.bin`. A fresh clone has **no held-out set at all**, and the 3 untracked synth files break `make_synth_corpus.py`'s own reproducibility claim for a fresh checkout.

---

## 12. Decoder / RSS / code-size risk for the corpus lane specifically

Low risk, and I want to be precise rather than invent a threat:

- No corpus mechanism alters the decoder. Corpus discipline adds **0 bytes** to the wire and **0** to decoder RSS.
- The real resource risk is **CI**, not the decoder: an independence audit that is O(n²) in files over a large locked corpus will not scale. At 24 files it is trivial (§11.1). At 500 files, pairwise MinHash is ~10⁷ comparisons — still fine — but pairwise 4 KiB block-set intersection is memory-bound. **Recommendation: block by MinHash candidate pairs, not all pairs, above ~100 files.**
- `A8` **Hidden risk worth naming:** a `pe-*.exe` corpus member is also a plausible **fuzz target**. Host-installed Microsoft binaries redistributed into a fuzz corpus raise a provenance question the protocol's §4.8 does not currently answer. Low practical risk; flagging because the protocol is meant to be complete.

---

## 13. Prior-art map (methodology, not mechanism)

This track's "mechanism" is the validation protocol itself, so prior art is about **evaluation methodology**.

| Prior art | What it already does | ANVIL delta |
|---|---|---|
| **ML hidden-test-set discipline** (Kaggle-style sealed finals; Dwork et al. DP "board vs public" split) | Sealed holdout, contamination ledger, one-shot access, role transition on open | ANVIL's role/lifecycle model is a faithful re-derivation. **Not novel — and correctly so.** |
| **JPEG 2000 Part 4 / lossless image codec common test conditions** | Specifies train/test corpus splits as part of the standard | ANVIL is stricter (per-object roles, hashes) |
| **BOSS / CALIC lossless contest methodology** | Held-out evaluation windows with published results | Contest framing, less about leakage |
| **MPEG lossless / F compression benchmark spec** | Standardized corpora for reproducibility | Different goal (interop, not anti-overfit) |
| **Near-duplicate detection** — Broder (1997) shingling/MinHash, Broder–Glass; sdhash/ssdeep | MinHash Jaccard over normalized shingles — exactly §5.1 rule 5 | §5.1 rule 5 is textbook. **Implementation is the gap.** |
| **Data provenance / lineage** — W3C PROV, SPDX, SLSA, Sigstore | Content identity, attestation, tamper evidence | §3/§6 are a sound specialization; SLSA-style attestation would cover the instrument-drift case in §6 |
| **Dedup at scale** — DataForge/dedup literature | Chunk-boundary selection to make dedup work | §5.1 rule 3 uses naive fixed 4 KiB blocks; boundary alignment is a known improvement |

**Conclusion: the protocol is not novel, and does not need to be. It is a correct, well-specified instance of standard practice.** The problem is purely that **none of it is implemented**. This matters strategically: there is no scientific prize here, so the only rational move is to implement the minimum that makes the evidence trustworthy, and spend the research budget on mechanisms.

**One genuine methodological improvement over the prior art** (and the "alternative mechanism" this mandate asks for): **seal a generator specification + seed instead of (or alongside) sealing data bytes.** This gives (a) reproducibility of held-out bytes without redistribution, (b) auditable lineage by construction — the generator *is* the provenance, (c) license avoidance, (d) CI acquirable without shipping binaries. It is strictly better than shipping opaque host binaries for the structured/numeric families, and it is the direct fix for §4, §8, and the untracked-files problem simultaneously. **Caveat I must state honestly [A]:** a sealed generator is only genuine held-out if the generator's *design* was not itself tuned against observed outcomes. Sealing the artifact does not launder design-time leakage. Use it for **family construction**, not as a substitute for real published corpora.

---

## 13A. Adversarial review of the constructive protocol itself

Reviewed **as designed**, not as intended. All **[M]** items are my measurements from §3/§6/§11; design readings are **[A]** unless marked.

### 13A.1 The protocol is self-attesting — the deepest defect

Every enforcement field is written by the party whose compliance is being certified: `role`, `independence.*`, `license.*`, `lineage.previous_roles`, and every `actor`/`action`/`reason` in the ledger. The ledger is a hash chain — it proves **tamper-evidence after the fact, not truth at write time**. PB-09 verifies chain *integrity*, not honesty.

**Minimum fix (separation, not cryptography):** the ledger MUST be appended by the workflow (machine actor), never the human; the lock MUST be authored in a reviewed change.

### 13A.2 Dedup: false negatives where it matters, false positives where it doesn't

- **Rule 3 (fixed 4 KiB blocks) is blind to the actual defect.** I ran it corpus-wide **[M]**: **exactly one** collision corpus-wide, and it was inside the held-out set (`pe-where`/`pe-winver`). **Zero** across the 14 files that actually share only two generator lineages. Rule 3 is a *content* rule; the dominant real risk here is *lineage*. Fixed-boundary chunking is also alignment-sensitive in the dedup literature — content-defined chunking is the standard remedy and the protocol ignores it.
- **Rule 5's global 0.20 threshold is unqualified by class.** On templated NDJSON, genuinely independent publishers legitimately share key vocabularies. The auditor's only responses are lower the bar (forbidden by doctrine 5) or waive (forbidden by §5.1). **Both roads end at PB-11.**

### 13A.3 Rule 5 is undefined for ~5 of 13 controlled classes

§5.1's normalization has no defined meaning for `executable`, `numeric_telemetry`, `random_incompressible`, and is partial for `sqlite`/`malformed_stream`. **6 of 13 `class` values have no evaluable rule 5.** Same defect for `repeated`: sharing blocks is the *intended* property there, so any held-out `repeated` object trips rule 3 by construction. **Fix: mandatory per-class rule applicability; CDC for binary classes.**

### 13A.4 Grouping granularity is an unfalsifiable knob that sets the pass/fail

§5.1 lets a duplicate be adjudicated "by placing both objects in the same independence group" — with **no constraint on granularity** — while the diversity gate is *counted in independence units*. **The author chooses group granularity, and thereby chooses whether the anti-overfit gate passes.** The protocol freezes every other threshold and leaves the deciding knob free. **Fix: groups = transitively-closed connected components of the §5.1 conflict graph (union-find), with the raw edges published as an artifact.**

### 13A.5 Publisher isolation is unsatisfiable for the only real-world family

§5.2 requires "source repository owner and organization MUST be new." Measured **[M]**: 3 of 6 `pe-*.exe` are `%SystemRoot%\System32` binaries from **one** OS build (10.0.22621.1) — they have **no repository owner at all**. The rule cannot be satisfied for a legitimate real-world family, so the auditor must either ignore it or discard the only non-synthetic data. **A rule unsatisfiable for exactly the data you need is not a rule.** Fix: attestable producer-identity predicate (signing authority / build vendor / registry digest).

### 13A.6 Toolchain diversity is not verifiable from the artifact

§4.7 requires `producer_id`; §13 requires "two unrelated toolchains." My PE parse **[M]**: all six report optional-header linker `12.1`, a field that does not encode toolchain. `README.md:76` asserts MinGW-w64/GCC for `pe-git.exe` **in prose only**. **Fix: `toolchain_attestation` with an evidence hash.** A prose claim is not an attestation.

### 13A.7 `source_dirty` is ambiguous about WHICH tree — RESOLVED BY COORDINATOR RULING; now a schema amendment

**Coordinator ruling, 2026-10-02: local research-worktree dirtiness does NOT block a clean, pinned remote checkout.**

I accept this and **withdraw my earlier claim** that this "blocks §12 step 1 permanently." My original reading compared two documents addressing *different trees*: §4.2's `source_dirty` is an acquisition-provenance field describing the checkout that produced measured bytes, while MASTER-BRIEF §8/§16 governs the local swarm control plane. **There is no contradiction.** I over-read a scope ambiguity as a governance conflict.

**The residual defect is real but smaller: the field does not say which tree it evaluates.** An agent reading §4.2 against a dirty local worktree will record `source_dirty: true` and conclude the lock is unusable — a false negative that would stall the whole track on a non-problem.

**Required amendment (schema, not decision):** `source_dirty` MUST be asserted about the **pinned remote checkout at `source_tree_ref`**; the lock MUST carry both `source_tree_ref` and `evaluated_at`; local control-plane state is a separate field that MUST NOT influence it. **Demoted from blocking-decision to blocking-schema-amendment.**

### 13A.8 The schema cannot express host-installed binaries — it encodes synthetic bias

§4.9 `source.kind` ∈ {`git-raw`, `https-file`, `archive-extract`, `generate`}. Measured **[M]**: the six `pe-*.exe` are host-installed binaries — **none of the four**. No `host-installed` kind exists. So the only real-world family is **inexpressible**, while all 14 synthetic files are fully expressible by `generate`. The schema does not merely fail to catch synthetic bias — it *encodes* it. Any lock written to this schema will be overwhelmingly synthetic by construction.

### 13A.9 Archive member identity leaks held-out anatomy by construction

§4.9 requires `member_name`, `member_bytes`, `member_sha256`, `member_crc32` per `archive-extract` entry. **Producing those requires opening and inspecting the archive** — exactly the "compression anatomy" §1.7 forbids pre-authorization and §7 says *consumes* a held-out role. The protocol therefore **mandates a pre-gate violation on every `archive-extract` held-out object**, which includes Silesia/enwik8. Fix: address members by manifest+index without per-member hashes, **or** budget one sacrificial family for selection.

### 13A.10 Sealed-set recovery is undefined; one flaky runner can burn a family

§9.3 grants one authorized execution; §6.11 one clean retry. But there is **no rule for the case that decides real money: the first run measured successfully and crashed before any view.** §7 burns an object for a *forbidden* measurement "even if the run crashes" — implying a *permitted* crash does not. That is **implication, not specification.** The conservative reading burns real data on every GHA flake; the permissive reading is a laundering loophole. **Specify: burn on result view only; infra-fail-without-view retryable under a new workflow SHA, up to a pre-registered cap.**

### 13A.11 Licensing enforcement has no source of truth

§4.8/§10.10 require license verdicts, but nothing defines how `redistribution_allowed` is determined or **from what authority**. PB-13 is unenforceable. Exposure is concentrated exactly where the project is weakest: Windows binaries + CPython (PSF) + Ninja (Apache-2.0) + Git (GPLv2 — which additionally triggers `attribution_required`).

### 13A.12 Implementability: cheap in compute, expensive in plumbing — correction to my own §16

Measured **[M]**, §11.1: pairwise 4 KiB audit over 21.1 MB ≈ 13.3M lookups **< 1 s**; resident ≈ **165 KB**. **Compute is a non-issue.** The cost is CI plumbing: §6 needs a fail-closed fetcher with redirect allowlists, an extractor rejecting traversal/symlinks/duplicate members, per-step verification; §10 needs 17 artifacts under `if: always()`; §9.3 needs a **separate protected workflow and environment**. That is a second pipeline, not a script. **The core (lock + grouping + audit + roles) is ~a day; full §6 + §9.3 is not near-term budget.**

---

## 13B. RECONCILIATION with `03-heldout-corpus-space-bunny.md`

Read after issuing this report. Four targeted adversarial tests against the **actual prototype**, not its prose: `prototypes/swarm-2026-10-02/03-heldout-corpus/space-bunny/seal.py` (25,991 B on disk; the constructive report cites 21,607 B / hash `00bc1a8f…`, so **the file changed post-checkpoint and its cited hash no longer matches current bytes** — a `build-manifest` pin hazard per §10.3) **[M]**.

### 13B.0 Where we agree (corroboration, not deference)

| Fact | Theirs | Mine | Independent? |
|---|---|---|---|
| No lock/ledger/audit artifacts exist | E3 [M] | §2 [M] | Yes — filename search vs. explicit `Test-Path` ×6 |
| Protocol 100% unimplemented | E3 | §2 | Yes |
| All 8 `synth-*` share one generator lineage | E21 [A] | §3.2 [M] | **Yes — I read the seed constants (90004–90008) + one script; convergent** |
| MD5-only fetcher violates SHA-256 authority | E14 [M] | §15 [M] | Yes — same lines |
| Criterion 5's 0.20 threshold is miscalibrated | Defect A | §13A.2 | **Yes, independently, opposite directions** (theirs: estimator too coarse; mine: too loose for templated data) |
| Novelty is adopt-class infrastructure | D1 | §13 prior-art map | Yes |
| PILOT, not PROMOTE | §7 | §16 | Yes |

Their **M2 chain-head commitment** is a real finding, discovered *empirically* — their harness proved a truncated chain verifies clean (§4.2 #5). I independently predicted the same hole in §13A.1 without reading their prototype. **Convergent discovery of the same defect is the strongest positive signal in this track.**

### 13B.1 TEST 1 — Can the schema represent untracked / host-origin PEs honestly?

**NO. Blocking defect.** Measured against the prototype **[M]**:

- `seal.py` contains **zero** occurrences of `group`, `host_install`, or `union_find` **[M]**.
- It contains **no `source.kind` validation at all** **[M]** — the §4.9 four-value enum is never enforced.
- `audit_pair` (lines 216–252) implements c1–c8 as a **list of blocking reasons** and nothing more **[M]**.

Consequences:

1. **A lock from this tool is a hash manifest with an advisory annotation channel.** It emits reasons; it does not group objects, does not compute independence units, and has **no concept of an independence unit**. The central quantity of my G1 and their own E21 is **outside the machine's model**. It cannot fail a portfolio on unit count because it has no such counter.
2. **Grouping is therefore 100% manual and optional — which I do not accept.** Worse than manual: the output *reads* as mechanical (hash-pinned tool, `dedup-report.json`, L6 two-auditor agreement), conferring **false assurance** on an untouched human judgment. L6 makes two auditors agree on a *reason list* while both silently apply the same unconstrained grouping choice (13A.4).
3. **Host-origin PEs cannot be honestly expressed.** With no `host-installed` kind and no `kind` enforcement, the six untracked PEs lock by `declared_sha256` alone with `git_blob_sha1: null` (their own fixture, line 434) **[M]**. Consequence: **c2 (identical git blob) silently cannot fire for every host binary**, because c2 is gated on `meta_a.get("git_blob_sha1")` being truthy (line 222) **[M]**. The second-strongest identity criterion is **structurally disabled for the entire real-world family** — a fail-**open** path on exactly the data that matters most.
4. **Untracked status is never recorded.** Nothing captures "this file is not in git," so a fresh clone silently has a smaller corpus and the lock still verifies. Measured: **9 corpus files untracked, including all 6 PEs** **[M]**. The tool calls that PASS.

**Verdict: TEST 1 FAILS. Blocking.**

### 13B.2 TEST 2 — Does it group shared-generator and cross-container lineages?

**Shared-generator: works. Cross-container: NOT DETECTED. Neither reaches independence units.**

- **Shared-generator works.** c8 (lines 249–251) fires on matching `generator_sha256` for two synthetic entries **[M]** — correctly catching my §3.2 finding *if* the lock declares true generator digests for all 8 `synth-*` and all 6 `generated.*`. Credit where due.
- **But it is a reason, not a group.** Eight `synth-*` produce **eight independent c8 reasons**, not **one group** — O(n²) verbosity, no transitive closure. Since §5.1 adjudication requires placing objects *in the same group* and **no group object exists in the tool**, adjudication cannot be performed by the tool at all. 13A.4 reproduced in code.
- **Cross-container lineage: NOT DETECTED.** My §3.5 measurement — **12,000 of 12,000 `generated.sqlite` payloads byte-verbatim from `generated.json`** **[M]** — is the strongest duplicate-lineage fact in the corpus, and it is *semantic containment*, not byte- or block-identity. Against `audit_pair`:
  - c1 needs identical whole-file SHA-256 → **no fire** (different files).
  - c2 needs git blob → **no fire**.
  - c3/c4 need a shared 4 KiB block → **no fire**, and I verified this directly: my corpus-wide 4 KiB scan found **zero** shared blocks between `generated.json` and `generated.sqlite` **[M]**. SQLite page framing interleaves the payload so no aligned 4 KiB block survives. **A measured demonstration that fixed-block rule 3 cannot see cross-container lineage** — 13A.2's false negative, proven on the project's own data.
  - c6 needs same `schema_cluster_id` → **only if the curator assigns it**; subjective (their own Defect C).
  - c7 needs same publisher/project/release → **plausible if declared honestly**, but self-declared.
  - c8 needs both entries `synthetic` with equal generator digest → **only if the curator marks the SQLite file as synthetic output**. Nothing *verifies* it, and a real `.sqlite` database plausibly looks like captured data to a curator.

  **So detection of my headline finding rests entirely on the curator correctly self-declaring `synthetic` + the right generator digest.** That is 13A.1 reproduced where it costs most. A lock marking `generated.sqlite` as `external_mixed` — a completely reasonable curator error, since it *is* a real SQLite file — passes clean while containing 100% of another entry's information.

**Verdict: TEST 2 FAILS on cross-container lineage. Blocking for any structured-data claim.**

### 13B.3 TEST 3 — Does it fail closed on the 4 KiB overlap I found?

**Yes on the pair; but a silent-suppression path exists. Partial pass with required fixes.**

`audit_pair` line 227 intersects `rare_blocks` and appends `c3-shared-rare-4kib-block` **[M]**. My `pe-where`/`pe-winver` block appears in exactly 2 objects and `mark_rare` treats `<= rare_max(2)` as rare (line 212) **[M]** — so the pair **is** flagged. Their §4.2 #2 noted the demotion rule was untested because their fixtures never shared a block **[R]**; **my measurement supplies the missing third fixture and confirms c3 fires on real data [M].**

**But demotion is corpus-global.** Adding one third object containing that block **silently demotes** the finding from blocking to a non-blocking `common_block_overlap_count` — no code change, no warning, no audit-trail row, and the pair's verdict flips blocked → clean. Demotion is by design (their §3.2 budgets that field **[R]**); the defect is that **it has no bound and no reversibility.**

**Required fix (mechanical):**
1. **Never demote within a role-crossing pair.** A shared block between a `heldout` entry and any open role is *always* blocking; global frequency is irrelevant when one side is sealed. One condition, zero cost, closes the suppression path.
2. **Emit demoted findings as first-class rows** (`block_hash`, `object_count`, `demoted: true`, `demoting_objects[]`).
3. **Make `rare_max` a locked field.** It is currently a Python default `rare_max: int = 2` (line 200) **[M]** — a threshold that moves without a lock revision, violating doctrine 5.

Point 3 generalizes: `seal.py` has at least **three uncommitted thresholds** — `rare_max=2` (line 200), `k=256` (line 163), hardcoded `0.20` (line 231) **[M]**. The protocol requires thresholds in the preregistration; **a tool default is not a preregistered threshold.** All must move into the lock.

### 13B.4 TEST 4 — Is grouping manual/optional behind a hash-pinned lock?

**YES — and this is the load-bearing rejection.** Chain of evidence **[M]**:

1. The independence output is a **list of per-pair blocking reasons** (`audit_pair` → `list[str]`).
2. **No group object, no union-find, no connected-component computation, no independence-unit counter anywhere in the file** — `group`, `union_find`, `component` all return zero grep hits.
3. The only unit-like field is `independence.exact_duplicate_group_id`, described as *"singleton is the object's SHA-256 identity"* **[R, §4.7]** — i.e. **the schema default is one group per object**, the maximally-independent assumption, and overriding it is the curator's job.
4. `near_duplicate_group_ids` is likewise curator-populated **[R, §4.7]**.

So: **a lock that hashes every file perfectly, pins the tool by SHA-256, and passes two independent auditors agreeing on all 2,016 pairs (their L6) still leaves independence grouping entirely manual.** Every number my G1 depends on is curator-chosen — and per 13A.4 **grouping is precisely the knob that decides pass/fail.** Their L2/L6/L7 gates certify the *integrity of a judgment*.

**This is the exact failure mode I was instructed not to accept, and I reject it.**

**Mandatory, mechanical replacement (a gate condition, not a design preference):**

> **GATE-INDEP-1.** The audit tool MUST compute independence groups as the **transitively-closed connected components** of the §5.1 conflict graph (union-find over c1–c8 edges), MUST emit `conflict_edges[]` as a first-class artifact, and MUST emit `independence_units: int`. A curator MAY NOT supply group ids as input; group ids are an **OUTPUT**. `exact_duplicate_group_id` / `near_duplicate_group_ids` in the lock are **derived**, and any lock where they disagree with the computed components MUST be rejected.
>
> **GATE-INDEP-2.** The portfolio gate MUST be evaluated on `independence_units`, never on file count. A lock with 24 entries and 4 units MUST NOT be reported as 24-family evidence.

Without these, **L6 must be re-specified to compare computed components, not reason lists** — otherwise LOCK-V1 can pass all nine gates while the anti-overfit claim is false.

### 13B.5 Disagreements resolved

| # | Their position | My position | Resolution |
|---|---|---|---|
| R1 | "A perfect sealing protocol applied to an empty portfolio seals nothing" (D2) | Agreed, and stronger: the portfolio is not empty, it is **mislabeled** — 24 files / ~4–5 units | **Both true, different scope.** The *apparent* adequacy is the active hazard. |
| R2 | Novelty is adopt-class (D1) | Agreed, reached independently via prior-art map (§13) | **Agree.** M2 head-commitment + rare-block demotion are the defensible narrow additions; cross-host CDC is likewise narrow and prior-art. |
| R3 | Screen is unigram not 5-gram; 5-gram "would have fired on 0 of 28 overlapping pairs" **[M]** | **Strongest technical contribution in either report**, corroborating 13A.2 from the opposite direction | **Accept and promote.** Their measurement (5-gram Jaccard ≤ 0.0074 on 28 real pairs) proves §5.1 rule 5 as written is **near-total false-negative** on structured data; my corpus-wide 4 KiB scan agrees from the content side. **Two independent measurements, same conclusion. Rule 5 must be rewritten, not threshold-tuned.** |
| R4 | §4.2 #3 unresolved: near-identical fixtures screen < 0.20 | Not claiming root cause. But a screen < 0.20 on 98%-identical files is a **false negative in the safety-critical direction** | **Unresolved, bounded.** Must not be fixed by lowering the bar. |
| R5 | L8 = 120 s / 256 MiB at n=60, 120 MB | My §11.1 measures the *block/pairwise* stage at **< 1 s / 165 KB** for 21.1 MB; their §3.2 projects **24 s pure-Python** for criterion 5 | **Theirs is binding**; I withdraw any claim the full verifier is cheap. Their §3.3 self-correction (64 → 128 → 64 MiB with stated reason) is good practice; I adopt the 32-bit-screen/64-bit-decide refinement **[A]**. |
| R6 | "The executable held-out family is currently zero" (E9) | Agreed, plus harder facts: 3 of 6 are one OS build, all 6 report linker 12.1, toolchain claim is prose-only (§3.3) | **Agree and extend.** |
| R7 | Cites `seal.py` at 21,607 B, hash `00bc1a8f…` | Current file is 25,991 B **[M]** | **Both true.** Prototype changed post-checkpoint — legitimate, but the cited hash is stale and must not be used as a §10.3 `implementation_git_sha` pin. Minor process finding. |
| R8 | PILOT because the blocker is attackable | **PILOT**, but on a different and smaller object: theirs is sealing machinery, mine is corpus admissibility | **Agree verdict, disagree scope.** Theirs is the right next step only if GATE-INDEP-1/2 land first. |

### 13B.6 What I add that they do not have

1. **§13A.7** — `source_dirty:false` vs. MASTER-BRIEF §16 hard contradiction. Neither report mentions it. A coordinator decision, cheaper to fix than anything on either list.
2. **§13A.8** — schema cannot express host-installed binaries; it *encodes* synthetic bias.
3. **§13A.9** — archive member hashes mandate a pre-gate anatomy view; the protocol cannot be followed for Silesia/enwik8 without violating §1.7.
4. **§3.5** — the 12,000/12,000 cross-container containment and its measured invisibility to rule 3.
5. **§3.5 corollary** — 24 files → ~4–5 units, the number their tool cannot compute.
6. **§13A.10** — burn-on-crash ambiguity: their D5 reaches it as an *interpretation*; I show the text genuinely does not settle it and specify the fix.
7. **Uncommitted thresholds in code** (`rare_max=2`, `k=256`, `0.20`) that must move into the lock.
8. **§13A.5** — §5.2 publisher isolation is unsatisfiable for Windows system binaries: a protocol bug, not a corpus shortage.

### 13B.7 Reconciled verdict

**PILOT — unchanged, but conditional in a way it was not before.**

The constructive lane's PILOT is right on the *verdict* and right on the *novelty downgrade*. It is **not sufficient as specified**: LOCK-V1 as written can pass all nine gates while leaving the anti-overfit claim unproven — L6 compares reason lists, GATE-INDEP-1's unit count does not exist in the tool, and TEST 1/2/3 show the real corpus defects pass through it.

**LOCK-V1 dispatch additionally requires, beyond their K1–K8:**
- **GATE-INDEP-1 / GATE-INDEP-2** (§13B.4) — blocking.
- **L6 re-specified** to compare computed components, not reason lists — blocking.
- **`host-installed` source kind + `git_tracked` recorded per entry** (§13B.1) — blocking.
- **Criterion c9: cross-container containment** (one entry's decoded record set contained in another) — blocking for structured claims.
- **Cross-host-pair demotion immunity + demotion audit rows** (§13B.3) — blocking.
- **Thresholds moved from code defaults into the lock** (§13B.3) — blocking.
- **§13A.7 coordinator ruling on `source_dirty`** — blocking, and a decision rather than work.

Their §4.2 #3 and my R4 agree the screen defect is unresolved; **neither report should claim the screen works.**

**Finding that outranks both reports: the corpus cannot pass a portfolio gate today, and the tool cannot say so.** That is B6/B14 in §16.2 and should be the coordinator's headline for this track.

---

## 13C. Q1 RED TEAM — independent diagnosis and audit of the landed C5 patch

Triggered by the coordinator's reproduction of 5 `seal.py` selftest failures. I reproduced them independently, diagnosed the algorithm, and audited the constructive patch after it landed. **No reliance on the constructive report's account; all numbers below are my own measurements.**

Artifacts (isolated, unwired, no codec/network/timing claim):
`prototypes/swarm-2026-10-02/03-heldout-corpus/fledge/c5_semantics.py` (diagnosis + independent fix), `…/fledge/seal_verified_fix.py` (patched copy, reproducible).

### 13C.1 Coordinator's `_BOILER` diagnosis: CONFIRMED **[M]**

`b"# boilerplate banner block\n"` is **27** bytes; the code computed `4096 - 28`. Measured `_BOILER` = **4095 B**, not a multiple of 4096. So the stride slicer produces `block0 = _BOILER + 1 byte of the per-fixture-unique filler`, which differs across the three fixtures — **shared 4 KiB blocks across v-boiler1/2/3 = 0.** `_RARE` is exactly 4096 and does share 1 block. Diagnosis correct; root cause is a hand-counted literal, and the landed patch fixed it properly with `_pad_to_block()` + `assert len(out) == BLOCK`.

### 13C.2 The 5 failures have ONE algorithmic root cause **[M]**

Measured on the pre-patch file: `v-boiler1/v-boiler2` unigram Jaccard **0.9756**, `v-boiler2/v-boiler3` **0.9756**, `v-rareA/v-rareB` **0.9655** — all far above the 0.20 threshold, so C5 fired on pairs that share *only* boilerplate or *only* a binary block.

**Root cause: C5's screen is computed over the whole object and has no notion of *which region* is shared.** The shared *binary* filler tokenizes into a large saturated unigram set, so boilerplate scores 0.98 while the constructive lane's own measurement has genuine overlap at **0.0074**. The 0.20 threshold is not mistuned — **its two populations are 0.0074 and 0.95+, and no threshold separates them.** It is the wrong control variable. Prevalence and text-likeness must do the work.

### 13C.3 The landed patch: semantics correct, execution broken

**Credit where due.** The patch independently converged on the same two-mechanism design I derived: (i) both objects must pass a deterministic text-likeness gate; (ii) confirmation requires a **prevalence-rare** gram lying **outside** already-shared blocks. That is precisely "demote by prevalence, not by an arbitrary similarity threshold" and "no double-counting with C3/C4." Convergent design is a real signal.

**But the file cannot be imported.** `_WORDS` values are `str` and `_BANNER` is `bytes`, so line 478 (`_BANNER + (body * …)[:3000]`) raises `TypeError: can't concat str to bytes` at module load. **Zero of the 5 fixes can be claimed verified — the selftest never ran.** A second latent fault: `gram_rare_max` is referenced at line 265 but defined nowhere, so `mark_rare` raises `NameError` on first call. Two repairs (`body.encode()`; define `gram_rare_max`) make it importable — I did both on a temp copy, not on their file.

### 13C.4 After repairing only those two: 4/5 fixed, 1 new failure — and it is the important one

With D1+D2 repaired, all original checks pass **except** `audit v-exact-a vs v-exact-b`, where **C5 is missing** and ground truth requires it.

Cause: both fixtures are 3029 B, so the single 4 KiB block *is the whole object*. `shared_blocks` therefore contains everything, `grams_excluding_blocks()` empties both sets, and the non-double-counting rule erases C5 entirely.

**Correct semantic fix (mine, verified):** a shared block is "copied content" only when it is a **strict subset of both objects**. When C1/C2 hold, the block *is* the object, so excluding it destroys C5's distinct statement that the pair is a *text* duplicate rather than merely a block-sharer. Scope the exclusion to the C3/C4-redundancy case only.

### 13C.5 The patch changed ground truth — in both directions, against instruction

Comparing tables: `TRUTH[("v-exact-a","v-exact-b")]` **lost** `c5`; `TRUTH[("v-textdupA","v-textdupC")]` **gained** `c5`. A separate new check asserts *"C5 stays silent when C1/C3 already explain the pair."*

Editing expected outputs so behaviour matches them is how a regression becomes a green suite. The coordinator's instruction not to change ground truth was directed at me; the patch changed it anyway, and **that is the single most serious process finding in this section.**

**The A/C change is substantively wrong, and I measured why.** `v-textdupA`/`v-textdupC` share an envelope with different values: **exactly 2 rare shared grams, containment 0.0053, screen 0.2978.** For this pair C6 and C7 **already fire** (same schema cluster + same publisher). So C5 adds two grams of envelope overlap and nothing else — textbook double-counting, which the coordinator explicitly forbade. A policy that fires here is a policy where **any two records sharing an envelope block each other**, i.e. the templated-data false positive from my §13A.2, now written into ground truth.

**My recommendation:** C5 must **abstain** on A/C; keep `c5` for byte-identical text pairs (original ground truth). If the coordinator prefers the patch's reading, it must be a **documented, deliberate** ground-truth revision with rationale — not a silent edit.

### 13C.6 Proposed confirmation statistic, with the measured separating gap **[M]**

Non-emptiness (`if intersection`) is far too weak. Replace with **rare-gram containment**, `|rareA ∩ rareB| / min(|rareA|,|rareB|)` — normalised by the *smaller* object so small near-duplicates are not penalised (`v-a/v-b` has only 13 rare grams total, so any raw-count threshold breaks it).

| Pair | must | rare ∩ | containment |
|---|---|---:|---:|
| `v-textdupA/v-textdupB` | fire | 78 | **0.2074** |
| `v-a/v-b` | fire | 1 | **0.0769** |
| `v-exact-a/v-exact-b` | fire | 1 | 1.0000 |
| `v-textdupA/v-textdupC` | abstain | 2 | **0.0053** |
| `v-boiler1/v-boiler2` | abstain | 0 | 0.0000 |
| `v-license/v-licensed` | abstain | 0 | 0.0000 |

Highest must-abstain **0.0053**, lowest must-fire **0.0769** — a **14× gap**. Any threshold in that interval separates; I pre-registered **0.05**.

**Note the count statistic cannot do this:** A/C shares *more* rare grams (2) than `v-a/v-b` (1), yet must abstain while `v-a/v-b` must fire. Only the normalised form separates them.

### 13C.7 Text-likeness margin — measured, and a correction to my own claim **[M]**

`TEXT_LIKENESS_MIN = 0.90` is well chosen and better justified than my first attempt. Measured: `generated.log` 1.0000, `synth-counters.log` 1.0000, `doc.md` 0.9989, `generated.json` ~1.0 ⟷ `pe-where.exe` 0.2241, `pe-git.exe` ~0.22, `random.bin` 0.3827, `synth-arith.bin`/`generated.sqlite` ~0.37-0.39. **Text ≥0.9989, binary ≤0.39 — a clean gap, and 0.90 sits safely inside it.**

**Correction:** I earlier printed uniform-random binary as "~0.005 printable." That was wrong — it is ~0.37-0.39, because 95 of 256 byte values are printable ASCII. The bimodality is real but smaller than I first claimed. The patch's 0.90 is better than the 0.50 I had used.

### 13C.8 Hypothesis falsified on real data — reported against myself **[M]**

I predicted the patch's loose confirmation would **saturate** a real corpus. I measured it on the 25 real corpus objects (300 pairs, bounded 4 MB sample):

- C5 with containment > 0: **0 pairs**
- C5 with containment ≥ 0.05: **0 pairs**

**My saturation hypothesis is wrong for this corpus.** The more accurate and more damaging finding: **C5 is inert on the entire real corpus and only ever fires on fixtures.** The current corpus contains no pair that can discriminate the two policies, so the patch's ground-truth edit was made with **zero real-data evidence** on either side. Consequence for Q1: **the C5 policy cannot be validated by LOCK-V1's current corpus.** It must be pre-registered as a *principle* and tested on real derivative pairs (e.g. genuinely forked repositories, vendored copies, re-released datasets) before dispatch.

### 13C.9 Remaining defects the patch should clear

| # | Defect | Severity | Fix |
|---|---|---|---|
| D1 | `TypeError` at import (`str` body + `bytes` banner) | **blocker** — nothing runs | `.encode()` at the concatenation |
| D2 | `gram_rare_max` referenced, never defined → `NameError` | **blocker** | define; move to lock |
| D3 | Block-exclusion erases C5 when the shared block *is* the object | **blocker** | scope to strict-subset / non-identical |
| D4 | Confirmation is non-emptiness (`≥1` rare gram) | high | rare-gram containment ≥ 0.05 |
| D5 | Ground truth edited in two directions to match new behaviour | **process** | revert or document explicitly |
| D6 | `TEXT_LIKENESS_MIN`, `gram_rare_max`, `0.20` still code constants | high | all three into the lock (**B14**) |
| D7 | C5 unvalidated on any real pair | high | acquire real derivative pairs before dispatch |

**Verdict on the patch: the design is right and independently convergent; the implementation is not runnable, the confirmation test is too weak, and ground truth was edited to match.** With D1-D4 applied and D5 reverted, I measure **all original ground-truth pairs passing at 0 mismatches**, `pe-where`/`pe-winver` still blocked by C3, and no spurious C5 for `generated.json`/`generated.sqlite` — whose 12,000/12,000 row containment **still requires criterion c9**.

---

I state these so I can be held to them:

1. If a `corpus-lock.json` + three audit reports + ledger already exist somewhere I did not search, §2 is wrong. **Falsifier:** any such artifact path in the repo.
2. If `generated.sqlite` payloads are not verbatim `generated.json` rows, §3.5 is wrong. **Falsifier:** a differing payload. (Measured: 12,000/12,000 verbatim.)
3. If the `pe-*.exe` files are tracked in git, §3.3/§11.3 is wrong. **Falsifier:** `git ls-files` listing them. (Measured: untracked.)
4. If `benchmark-suite.csv` does include `pe-*.exe` rows, §7 is wrong. **Falsifier:** a `pe-` row. (Measured: none.)
5. If `tools/gha_fetch_corpus.py` verifies SHA-256, §15 is wrong. **Falsifier:** a SHA-256 comparison. (Measured: MD5 only.)
6. If some prior non-ANVIL generator produced the structured families, §4 is wrong. **Falsifier:** a foreign generator path in the lock/ledger.

**Reproduce all six:** `python prototypes/swarm-2026-10-02/03-heldout-corpus/fledge/independence_audit.py` (§17 below), plus the four one-liners quoted inline.

---

## 15. Additional finding: the CI fetcher uses MD5, violating the protocol's own §3

Protocol §3: "**SHA-256 is the content authority.**" §4.10 requires `sha256` per entry. §6.8 requires SHA-256 verification on acquisition.

`tools/gha_fetch_corpus.py` verifies **size + MD5 only** **[M]** — `SILESIA`/`ENWIK8` tables carry `(bytes, md5)`, and `verify()` compares `md5` only **[M, lines 29-67]**. Its own docstring advertises "pinned size + MD5 manifest" **[M, line 6]**.

So the remote path — the path that actually produces citation-grade held-out numbers — uses a hash the project has already decided is not authoritative, for the Silesia/enwik8 anchor families. MD5 is not a practical barrier to accidental corruption, but it is a **direct, self-inflicted PB-04/PB-02-class inconsistency** and it is trivially fixed.

---

## 16. Recommendation

### Leaning: **PILOT** — with an explicit, uncomfortable second clause

**PILOT** because:
- The strongest falsifiable claim available is not a mechanism claim; it is *"ANVIL's structured/numeric evidence does not survive independence-unit accounting."* That experiment is cheap, remote-only, pre-registered in §10.2, and has a real chance of producing a decisive negative result about the *project's* evidence base rather than about any codec. That is worth running.
- The infrastructure cost is ~1 s of CI and ~165 KB RAM (§11.1). There is no resource barrier.
- The protocol spec is good enough to implement directly; the gap is execution, and execution is cheap.

**The uncomfortable clause — HOLD on all promotion language.** Until G1 passes:
- No report, ledger entry, or preregistration should describe any result as "held-out validated."
- `tests/corpus` must be described as a **smoke/development corpus** — which `tests/corpus/README.md:3` already says correctly, and which other artifacts have drifted past. **Trust the README; correct the drift.**
- The `pe-*.exe` set should not be cited as held-out evidence until it is locked, tracked, and license-cleared. Today it is untracked and cannot be expressed in the protocol's own schema (§4.9 `source.kind` ∈ {`git-raw`, `https-file`, `archive-extract`, `generate`}) — **host-installed binaries are none of these**, so the schema has no way to record the project's only real-world family **[M]**.

**Explicitly NOT PROMOTE-TO-REMOTE:** there is nothing here to promote. The deliverable is a protocol and an experiment, not a mechanism.

**Explicitly NOT KILL:** the corpus methodology is not unfixable — it is unimplemented. §13 of this report is implementable in a day.

### Recommended immediate actions, in priority order

1. **Stop describing `tests/corpus` as held-out.** One-line correction wherever the label drifted. Zero cost, removes the largest interpretive risk.
2. **Create the six missing artifacts** from the protocol that already exists. Start with `corpus-lock.json` + `corpus-lock.sha256`.
3. **Regroup by independence unit.** Group `generated.*` (1 unit), `synth-*` (1 unit), `{pe-where, pe-winver}` (1 unit, measured 4 KiB collision), `{pe-notepad, pe-python, pe-ninja}`, `{pe-git}`. Then re-count the families that actually exist against §13.
4. **Switch `tools/gha_fetch_corpus.py` from MD5 to SHA-256** and pin both in the tables.
5. **Add a `synthetic_control` role** to the schema so generated families can never be mistaken for generalization evidence.
6. **Remove the instrument from the instrumented corpus**: drop `anvil_bench.exe` from `tests/corpus`; treat `anvil.exe`/`src.cpp` as self-reference controls, never as `source_code` evidence.
7. **Track the `pe-*.exe` files** (or move them out of the repo and into fail-closed CI acquisition) so a fresh clone has a real held-out set.
8. **Acquire 2 foreign structured JSON/NDJSON families and 1 real numeric/telemetry stream** — the actual G1 blockers.

---

## 17. Isolated artifacts (this agent's only writes)

Per mandate, optional and isolated:

- `prototypes/swarm-2026-10-02/03-heldout-corpus/fledge/independence_audit.py` — reproduces §3.2, §3.3, §3.4, §3.5, §6, §14 mechanically. Hashing, generator parsing, PE header parsing. **Invokes no codec, measures no compression, runs no benchmark.**

No existing file was modified. No reset/clean/stash/restore/rebase. No commit or push.

---

## 18. Summary table: measured facts vs. assumptions

| # | Claim | Label |
|---|---|---|
| 1 | All 24 corpus SHA-256 match `CHECKSUMS.txt` | **[M]** |
| 2 | No `corpus-lock.json`, ledger, or audit reports exist anywhere | **[M]** |
| 3 | 24 files → ~4–5 independence units | **[M]** |
| 4 | All 8 `synth-*` from one script/author/seed family | **[M]** |
| 5 | All 6 `generated.*` from one 32-line script | **[M]** |
| 6 | 12,000/12,000 sqlite payloads verbatim from `generated.json` | **[M]** |
| 7 | `pe-where` ↔ `pe-winver` share 1 exact 4 KiB block | **[M]** |
| 8 | All 6 PEs report linker 12.1; cannot evidence toolchain diversity | **[M]** |
| 9 | All 6 `pe-*.exe` + 3 `synth-*` are untracked in git | **[M]** |
| 10 | `benchmark-suite.csv` contains zero `pe-*.exe` rows | **[M]** |
| 11 | Corpus `anvil_bench.exe` ≠ `build/anvil_bench.exe` | **[M]** |
| 12 | `gha_fetch_corpus.py` verifies MD5, not SHA-256 | **[M]** |
| 13 | §13 portfolio fails 3 of 5 counts | **[M]** |
| 14 | Audit cost ≈ <1 s, ≈165 KB RAM | **[A/M-estimated]** |
| 15 | Windows/CPython/Ninja/Git redistribution terms unclear | **[A]** |
| 16 | Sealed-generator protocol is better than shipping binaries | **[A]** — justified, but a judgment |
| 17 | 2026-08-21 manifest re-pin was "manifest-laundering" | **[A]** — strong inference from [M] facts + [R] narrative; the authors' reasoning was defensible for a one-off, the objection is to the class |

---

**Final: PILOT** — run `i10-heldout-v1` as specified in §10 with the §10.2 thresholds frozen; **HOLD** every promotion and held-out-validation claim until G1 passes; re-open this report when the constructive lane's file lands.