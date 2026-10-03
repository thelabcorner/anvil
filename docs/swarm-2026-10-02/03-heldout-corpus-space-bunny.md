# Track 03 — Held-Out Corpus Protocol — INTERIM CHECKPOINT

**Agent:** Space Bunny Free
**Date:** 2026-10-02
**Track:** `03-heldout-corpus` (MASTER-BRIEF.md line 40)
**Status:** **INTERIM CHECKPOINT.** Mechanism defined, bounded prototype
partially validated, one decisive remote-only experiment pre-registered. Not a
gate result. No commit, push, reset, corpus fetch, corpus measurement, or codec
execution was performed.

**Mandatory-output compliance:** this file is the only file created under
`docs/swarm-2026-10-02/`. One isolated, unwired prototype exists at
`prototypes/swarm-2026-10-02/03-heldout-corpus/space-bunny/seal.py` (21,607 B,
SHA-256 `00bc1a8f0662dc8090ca6e449e5c7015fbad4c41098ae2a739fddb31831609bd`).
It is not referenced by any production file, workflow, or CMake target.

---

## 1. Evidence map — measured facts only

Tags: **[M]** measured on this host in this session; **[R]** read from a named
repository artifact; **[A]** arithmetic derived from [M]/[R] inputs;
**[H]** hypothesis; **[P]** projection. No [M]/[R] number below comes from a
different host or benchmark window than the one named; incompatible windows are
never spliced (see §1.3).

### 1.1 The normative protocol exists and is entirely unimplemented

| # | Fact | Tag | Source |
|---|---|---|---|
| E1 | `docs/I10-CORPUS-LOCK-PROTOCOL.md` is 825 lines, protocol version 1.0, schema `anvil.corpus-lock/v1`, 28 promotion blockers PB-01…PB-28 | [R] | `docs/I10-CORPUS-LOCK-PROTOCOL.md` §1, §4.1, §11 |
| E2 | The protocol already mandates: SHA-256 as content authority (rule 3), order-sorted `entries`, canonical serialization, fail-closed acquisition (§6), an append-only `corpus-access-ledger.jsonl` with `previous_event_sha256`/`event_sha256` chaining (§7), the 8-criterion pairwise audit (§5.1), and a minimum new held-out portfolio (§13) | [R] | same, §1.3, §4.5, §6, §7, §5.1, §13 |
| E3 | **No `corpus-lock.json`, `corpus-lock.sha256`, `corpus-access-ledger.jsonl`, `dedup-report.json`, `publisher-split-report.json`, or `license-report.json` exists anywhere in the repository.** A recursive filename search for `corpus-lock*` over the whole tree returned zero hits | [M] | this session, `Get-ChildItem -Recurse -Filter corpus-lock*` |
| E4 | The required machine-readable audit artifacts (E3) therefore do not exist | [M]+[R] | §5.3 mandates them |

### 1.2 The existing corpus is not a lock, and has already failed once

| # | Fact | Tag | Source |
|---|---|---|---|
| E5 | `tests/corpus/` is governed by a flat `CHECKSUMS.txt` (SHA-256 + byte length + filename). It carries **no role, schema cluster, license, publisher, or independence field** | [M] | `tests/corpus/CHECKSUMS.txt` (header + 23 data rows) |
| E6 | **The manifest has already drifted and the integrity model already failed.** `anvil_bench.exe` on disk (1,929,216 B, `fcd30da5…`) did not match its 2026-08-13 manifest entry (1,866,752 B, `e5a0f8a7…`) *before* the 2026-08-21 expansion began; the file is git-ignored so the old bytes are unrecoverable | [R] | `tests/corpus/CHECKSUMS.txt` "DRIFT NOTE (2026-08-21)"; `RESEARCH_LEDGER.md:3467-3474` |
| E7 | 23 data files, ~19.8 MB total; "+7.54 MB added, under the 30 MB budget" — a 30 MB corpus budget is an already-exercised planning constraint | [M]+[R] | `tests/corpus/` listing; `RESEARCH_LEDGER.md:3490-3491` |
| E8 | The 6 `pe-*` binaries are labelled "held-out" in the historical ledger but are **not held-out under the protocol's own role rules**: all 6 have been locally measured and their orientation ratios published | [R] | `RESEARCH_LEDGER.md:3419-3497` (esp. 3481-3488); protocol §2.3: "the historical S6-2 PE set are open known/anchor data and MUST NOT be relabeled as unseen" |
| E9 | **The executable held-out family is therefore currently zero.** Same for structured: the only structured held-out ever opened, Sino-US DrugQA V1 (15,374,047 B), was opened by G3 and is now `known_stress`, not held-out evidence | [R] | `RESEARCH_LEDGER.md:4759-4760` ("V1 is `V1-COLUMN-ADVERSE` and remains known stress, not held-out evidence"); protocol §2.3 |
| E10 | The ledger states the blocker in one sentence: "No new held-out structured corpus is currently available. Promotion requires new locked independent structured, mixed-validity, executable, and numeric families." | [R] | `RESEARCH_LEDGER.md:4831-4833` |
| E11 | Protocol §13 minimum new held-out portfolio: **3** independently published structured JSON/NDJSON families, **2** mixed-validity/malformed families with substantial residual material, **3** executables spanning ≥2 producers and ≥2 toolchains, **2** real numeric/telemetry streams **not derived from ANVIL synthetic generators**, plus external anchors and AITDCC | [R] | protocol §13 |
| E12 | PB-26 makes the absence of a genuinely new held-out family a hard blocker; PB-10/PB-11 forbid duplicate conflicts and forbid waiving a duplicate to inflate the independent-family count | [R] | protocol §11 |

### 1.3 Provenance windows — kept separate, never spliced

| Window | Host / source | Used for |
|---|---|---|
| W0 (this session) | this Windows x64 host, control plane only, **no codec, no corpus bytes, no network** | E3; prototype self-test |
| W1 (2026-08-13 / 2026-08-21 corpus expansion) | this repo, `tests/corpus` | E5, E6, E7 |
| W2 (I7 ledger, 2026-08-21) | this repo, `RESEARCH_LEDGER.md` | E6, E8, E9, E10, E11 |
| W3 (I10 reconciliation, 2026-09-24/25) | this repo, GitHub Actions runs | E10, §6 disconfirmers |
| W4 (Linux EPYC v2 history) | **different host — NOT used for any claim in this report** | none |

No Linux-host and Windows-host number is combined anywhere below.

### 1.4 Acquisition robustness is already a reproduced failure, not a hypothesis

| # | Fact | Tag | Source |
|---|---|---|---|
| E13 | Run `36010649558` is `INVALID-G5B-ORDINAL`: the downloader forwarded the GitHub bearer token across a **cross-host 302 redirect** to signed blob storage, HTTP 401, and the discovery step was skipped; corpus, replay, discovery, V1 and floor steps were all skipped | [R] | `RESEARCH_LEDGER.md:4769-4781` |
| E14 | The repository's canonical fetcher `tools/gha_fetch_corpus.py` verifies **byte length + MD5 only** (`SILESIA` table of `(bytes, md5)`; `verify()` raises on size or MD5). MD5 is not a content authority under protocol rule 3 | [M] | `tools/gha_fetch_corpus.py:36-48` (table), `:66-75` (`verify`) |
| E15 | The G0 held-out fetch is an inline heredoc inside the workflow that verifies **size + git blob SHA-1 only**, and writes the SHA-256 into `results/validation-provenance.json` *after* measurement | [M] | `.github/workflows/anvil-i10-grotli-g0.yml:142-170` |
| E16 | The held-out hash identity that IS pre-registered (git commit + path + blob SHA-1) is sufficient for *immutability*, but the SHA-256 content authority is recorded **post-hoc** and therefore cannot satisfy PB-04 as an acquisition-time gate | [M]+[R] | `docs/I10-GROTLI-G1-CEILING-CORPUS-FREEZE.md:110-141,145-165`; protocol §4.10, §11 PB-04 |
| E17 | The local build is **blocked**, independent of doctrine: `cmake … -DCMAKE_CXX_COMPILER=clang++.exe` failed with `No CMAKE_RC_COMPILER could be found` at `Platform/Windows-Clang.cmake:149`. The authoritative build gate is the pinned Ubuntu CI workflows | [R] | `RESEARCH_LEDGER.md:4866-4874` |

### 1.5 The synthetic substrate is real, deterministic, and already one lineage

| # | Fact | Tag | Source |
|---|---|---|---|
| E18 | `tests/make_synth_corpus.py` is 274 lines with **8 generators** at fixed seeds 90004–90008; integer-only math is asserted specifically so bytes are stable across Python versions/platforms | [M] | `tests/make_synth_corpus.py` (generator header comments at lines 43, 62, 82, 111, 149, 188, 227, 246) |
| E19 | 8 synthetic files exist in `tests/corpus` (`synth-arith`, `synth-columnar-align`, `synth-counters.log`, `synth-drift-stride`, `synth-jitter`, `synth-ndjson-columnar`, `synth-telemetry-f64`, `synth-timeseries`), 4,634,516 B total | [M] | `tests/corpus/CHECKSUMS.txt`, `tests/corpus/` listing |
| E20 | Determinism was verified by double-run hash comparison, and all pre-existing files were re-verified byte-identical | [R] | `RESEARCH_LEDGER.md:3450-3452, 3463-3464` |
| E21 | Every one of the 8 generators is from **one generator file, one owner, one release** — i.e. all 8 share `independence.generator_sha256` lineage under protocol §4.7/§5.1 criterion 8 | [A] | E18 + protocol §4.7, §5.1(8) |

---

## 2. Strongest mechanism — PROSPECTIVE SEAL + OFFLINE-DETECTABLE BURN

### 2.1 The one-sentence claim

**A held-out corpus can be bound by identity before any of its bytes exist, and
a second measurement of an already-consumed object can be proven from artifacts
alone by an offline auditor with no trust in the actor** — replacing both
currently-procedural defenses (curator honesty; a self-reported append-only
ledger) with cryptographic ones.

### 2.2 Precise mechanism

Four components. M1/M4 are extensions of an existing normative spec; M2/M3 are
the parts I believe are genuinely load-bearing and not present in the spec.

**M1 — Prospective seal (byte-free identity commitment).**
Define `entry_seal(e) = SHA256(canon(e))` where `canon` is the protocol §3
canonical serialization and `e` is the entry's **provenance metadata only**
(publisher, project, release family, source kind, repository, commit, path,
declared byte length, archive digest, license, schema cluster, generator
digest). Define
`truth_root = Merkle(entry_seal(e_i) for i in declared array order)`
with odd-level duplication and a domain-separated internal hash
(`SHA256(0x01 || L || R)`), and `lock_id = SHA256(canon(lock))`.

Properties, each directly testable:
- `truth_root` is computable with **zero corpus bytes and zero network access**.
- Array order is part of the identity: reordering the corpus yields a different
  root. Reordering is a *scientific* change (it changes which arm sees which
  file first), not a cosmetic one.
- Mutating any single metadata byte changes `lock_id`.
- Because `truth_root` sits inside the preregistration hash, and the
  preregistration hash sits inside the gate binding, **selection-by-outcome is
  structurally impossible**: a curator cannot add, remove, or substitute an
  entry after seeing compression results without breaking the binding.

This closes E16: the SHA-256 content authority stops being a post-hoc artifact
field and becomes an acquisition-time assertion against a value that was
committed *before* the fetch.

**M2 — Chain-head commitment + evidence root (the offline double-use detector).**
Protocol §7 specifies an append-only hash chain but does **not** commit the
chain head anywhere outside the ledger, and does not bind the ledger to the
measurement output. Two additions:
1. `head_commitment = event_sha256(last_event)` is published in
   `gates.json`/`verdict.json` and is inside the artifact manifest.
2. Each measurement run emits `evidence_root = Merkle(row_digest[i] for i in
   rows.jsonl)` together with `implementation_git_sha`, `workflow_git_sha`,
   `binary_sha256`.

Consequence: a **second** run over a consumed object produces a well-formed
chain (the log is genuinely append-only, so nothing is malformed) but the
auditor can enumerate every `remote-measurement` / `local-measurement` /
`result-viewed` event per `corpus_id` and flag any `corpus_id` with count > 1,
and can detect tail truncation by comparing the local head to the published
head commitment. **The protocol as written cannot detect either.** Truncation is
the more serious of the two: an append-only chain that is simply *shorter* than
the truth is internally consistent, so "we only measured once" is currently
unfalsifiable.

**M3 — Screen-then-decide independence audit (two concrete defects in §5.1).**

*Defect A — the 0.20 MinHash threshold is not resolvable by the estimator the
protocol specifies.* Protocol §5.1 criterion 5 specifies "text MinHash estimated
Jaccard similarity `>= 0.20` using 128 BLAKE2b-derived permutations over
normalized 5-token shingles". The standard error of a Jaccard estimator with
k independent permutations is `sqrt(J(1-J)/k)`. [A] At J = 0.20, k = 128:
`sqrt(0.16/128) = 0.0354`, so ±2σ = **±0.071**. The decision boundary region
J ∈ [0.13, 0.27] is noise. An 8× increase to k = 1024 gives ±2σ = ±0.025 [A];
bottom-k with k = 256 gives ≈ ±0.055 [A]. The spec's own number is too coarse
for its own threshold.

*Defect B — criterion 3 ("shared exact raw 4 KiB block") will fire on
boilerplate and push curators into the forbidden waiver path.* Real corpora
share license headers, JSON preambles, `#!/usr/bin/env` lines, and standard
ELF/DOS stub blocks. If criterion 3 blocks a pair for one shared boilerplate
block, the curator's only relief is PB-11, which the protocol forbids
("MUST NOT be waived to count as independent evidence"). The protocol therefore
contains a built-in incentive to commit the one violation it names as a hard
blocker.

*Repair, both parts:*
- **Rare-block demotion.** Build the corpus-global block-hash multiset first;
  a shared block that appears in more than `T = 2` objects is **boilerplate**,
  is excluded from criterion 3/4, and is instead reported as a *graded* signal
  (`common_block_overlap_count`). A shared block that appears in ≤ 2 objects is
  **rare** and is genuine copy evidence. This preserves copy detection and
  removes the false-positive pressure.
- **Screen then decide.** bottom-k (k = 256) MinHash *proposes* candidate pairs
  (O(total_shingles), conservative by construction); an exact shared-8-gram
  containment test *decides* only the proposed pairs. A pair is blocked on
  criterion 5 only if it is both screened ≥ 0.20 **and** confirmed. The
  estimator's imprecision is converted from a decision input into a
  false-positive *generator*.

*Defect C — `schema_cluster_id` is curator-assigned, i.e. subjective, and is
the only criterion that catches "same schema, different values, low token
overlap".* Repair: derive it from a pinned canonical field-path signature (the
sorted set of JSON key paths of the first N records, hashed) computed by a
script whose SHA-256 is in the lock. That converts a subjective field into a
computed one and makes criterion 6 reproducible rather than arguable.

**M4 — Synthetic-control grade lattice (the mandate's contamination question).**
A formal admissible-use matrix. Synthetic objects are graded:

| Grade | Definition |
|---|---|
| **S0** | `parameters_sealed = true`; `parameters_sha256` is committed in the lock and the parameter *values* are never revealed until after the frozen run; generator lineage disjoint from every tuned generator |
| **S1** | lineage-disjoint from the tuned set, parameters known |
| **S2** | in the tuning ancestry (tuned against) |
| **S3** | neither of the above |

| Use | S0 | S1 | S2 | S3 |
|---|:-:|:-:|:-:|:-:|
| correctness / fuzzing / roundtrip | ✔ | ✔ | ✔ | ✔ |
| negative control (must-fail case) | ✔ | ✔ | ✔ | ✔ |
| decoder cost accounting | ✔ | ✔ | ✔ | ✔ |
| oracle-anatomy calibration (does the detector fire at the injected SNR?) | ✔ | ✔ | ✔ | ✘ |
| threshold **preregistration** | ✔ | ✔ | ✘ | ✘ |
| blind mechanism-sensitivity verdict | ✔ | ✘ | ✘ | ✘ |
| **held-out or generalization claim** | ✘ | ✘ | ✘ | ✘ |
| **reported ratio row with no real-data counterpart** | ✘ | ✘ | ✘ | ✘ |

Contamination proof obligation, stated as an operational test rather than an
admonition:

> Claim **C** is uncontaminated by synthetic family **S** iff **S** does not
> appear in the causal ancestry of any *decision* in **C**. Decisions are:
> thresholds, arms, representation semantics, candidate-generation parameters,
> backend selection, router rules. Each decision carries a
> `decision_provenance` row naming every input object and its role at decision
> time. Any S2/S3 ancestor forces the corresponding claim to be relabeled
> "tuned on synthetic structure", regardless of outcome.

The key asymmetry, stated so it cannot be quietly dropped: an **S0 blind
synthetic family supports the sentence** *"the mechanism's sensitivity at a
pre-registered, unobserved parameter draw was X"* — and **never** the sentence
*"the mechanism generalizes to real data."* Synthetic data buys *sensitivity*
(with known ground-truth structure); it cannot buy *generalization* (no real
data). Its legitimate role is to make threshold and detector calibration
honest, and to supply must-fail controls — not evidence.

Note E21: all 8 current synthetic files are one generator lineage, so **they
are all grade S2/S3 and none may pre-register a threshold.** A new S0/S1 pair
requires a second, independently-authored generator with disjoint lineage — a
real, unbudgeted piece of work.

### 2.3 Why this is the strongest mechanism for this track

It is the only formulation that makes the *binding* blocker attackable without
opening a single sealed byte. E10 says the blocker is corpus availability. But
availability is not directly testable without an unsealed fetch, and an unsealed
fetch burns the object (protocol §7: "A forbidden local held-out measurement
burns the object even if the run crashes"). Splitting the gate into
(a) *does the sealing machinery work, with no corpus bytes at all* and (b)
*does an acceptable portfolio exist, with no measurement of any compression
outcome* makes (a) runnable today and unspoiling. That is the pilot.

---

## 3. Cost model — bytes, cycles, RSS, and decoder state

### 3.1 Decoder-visible cost of the mechanism itself

| Quantity | Value | Basis |
|---|---|---|
| Decoder-visible bytes added to any ANVIL stream | **0** | the mechanism is entirely outside the compressed representation; nothing is framed, tabulated, or selected by the codec |
| Codec state added (encoder or decoder) | **0 bytes, 0 states** | no new opcode, no new block mode, no new model, no new table; no `FORMAT.md` change |
| Decoder binary delta | **0 bytes** | `seal.py` is never linked into `anvil` or `anvil_bench`; measured: no CMake target or workflow references it |
| Decode-cycle delta | **0 cycles** | same |

This is the mechanism's one unambiguous Pareto-gate advantage, and it is
*structural*, not measured: under the binding doctrine "charge every
decoder-visible byte, table, model, dictionary, framing field, selector,
planner cost, and reversible carrier" (MASTER-BRIEF.md doctrine 2), this track
adds nothing to be charged. It is *infrastructure*, and it must be reported as
such — it cannot be used to claim a Pareto win.

### 3.2 The executable that actually gates everything: the verifier

The only new executable state in this track is the offline verifier/sealer.
Measured, W0 (this session):

| Quantity | Value | Basis |
|---|---|---|
| Prototype source | 21,607 B, 535 lines | [M] `Get-Item` on `prototypes/…/seal.py` |
| Prototype SHA-256 | `00bc1a8f0662dc8090ca6e449e5c7015fbad4c41098ae2a739fddb31831609bd` | [M] `Get-FileHash -Algorithm SHA256` |
| Runtime dependencies | stdlib only: `hashlib`, `json`, `time` | [M] imports |
| Network / codec / corpus access | none | [M] no socket/subprocess import |

Projected verifier cost for the §13-minimum portfolio. **Inputs are
assumptions, not measurements**, and are stated so they can be attacked:
n = 60 entries, B_total = 120 MB, mean object 2 MB, largest object 15 MB,
gram = 5 normalized whitespace tokens, mean token 6 B.

| Stage | Cycles / time | Peak RSS | Notes |
|---|---|---|---|
| Author `corpus-lock.json` | ~60 × 1.4 KB = 84 KB of JSON; canonicalize + hash ≈ 1.3 × 10⁵ SHA-256 of ≤2 KB blocks | < 1 MiB working set | [A] from §4.1–§4.14 field list × 60 |
| `truth_root` | 60 leaf hashes + 59 internal hashes ≈ 119 SHA-256 of 65 B | negligible | [A] |
| Acquisition: SHA-256 + git-blob SHA-1 + block-hash + shingle, **one fused pass** | Θ(B_total); at 1.5 GB/s SHA-256-bound → **≈ 80 ms for 120 MB** | streaming O(4 KiB) + per-object sketch | [A]; SHA-NI assumed, unverified on the CI runner |
| Block index | B/4096 = 30,720 blocks × 8 B = **246 KB resident** (all entries) | 246 KB | [A] |
| Shingle index | tokens ≈ B/6 = 20 M; 5-gram hashes ≈ 20 M; **32-bit screen array = 4 B × 20 M = 80 MB if all-resident**; **64-bit = 160 MB if all-resident** | see §3.3 | [A] |
| Pairwise audit, all criteria 1–4 + 6–8 | n²/2 = 1,770 pairs × O(|set|) with early exit ≈ **10⁵ ops** | O(smallest pair set) | [A] |
| Criterion 5 screen | 20 M gram hashes total. Pure-Python BLAKE2b ≈ 1.2 µs each → **≈ 24 s**; vectorized 64-bit mixer → **≈ 0.5 s** | as §3.3 | [P] — the 24 s figure is a *projection from per-call cost*, not a measurement; **this is the single largest verifier cost and the prototype is currently on the slow path** |
| Criterion 5 decide | exact 8-gram intersection on screened pairs only. If P_s = fraction screened, cost = P_s × 2 × O(gram count) | two 64-bit arrays in flight | [A] |
| Ledger | E ≈ 60 × 5 + 20 = 320 events × ~520 B = **166 KB**; 320 SHA-256 | negligible | [A] |
| Evidence root | rows = 60 × 8 arms × 5 reps = 2,400; 2,400 SHA-256 of 32 B ≈ 1.7 MB `rows.jsonl` | streaming | [A] |

### 3.3 RSS ceiling — and a correction I had to make

First estimate I wrote was **64 MiB**. Working the numbers, that was wrong, and
the reason is instructive:

- grams ≈ B/6 per object, so a 64-bit gram array is **1.33 × object bytes**.
- Decide needs two objects' 64-bit arrays simultaneously: for two 15 MB
  objects that is 2 × 20 MB = **40 MB**, plus ~25 MB interpreter.

**Correction:** the honest RSS ceiling is **128 MiB**, not 64 MiB. The 64 MiB
figure assumed a hash set sized like the corpus, which is a bytes-vs-objects
category error.

Refinement that recovers most of it: **screen at 32 bits, decide at 64 bits.**
Screening only needs to be conservative (false *positives* are fine, false
*negatives* are not, and a 32-bit screen with 20 M grams has ≈ 46,000 spurious
shared grams — harmless when its only job is to *propose*). Deciding requires
64 bits (a 32-bit decide would produce ~46,000 spurious "duplicates" and be
worthless). Resident screen state becomes 0.67 × object bytes, and 64-bit
arrays are materialized only for proposed pairs. Peak RSS: **~64 MiB**, but
now for a stated reason rather than by wishful sizing.

---

## 4. Minimum prototype — status: BUILT, SELF-TEST FAILING, DEFECTS FOUND

`prototypes/swarm-2026-10-02/03-heldout-corpus/space-bunny/seal.py`, run as
`python seal.py selftest`. **Not wired into anything.**

The harness is doing its job: on its first two runs it produced **6 failing
checks out of 33**, and two of those failures are *findings*, not bugs.

### 4.1 What passed (W0, this session, deterministic)

- Prospective seal is byte-identical across re-serialization (lock_id, truth_root).
- Reordering entries changes the truth root (order binding works).
- A one-byte metadata mutation changes the lock identity.
- Duplicate `corpus_id` is rejected.
- Two distinct boilerplate-looking license fixtures audit **clean** — the
  rare-block demotion does not fire on unrelated files.
- Unrelated pairs (`v-a` vs `v-unique`, `v-license` vs `v-unique`) audit clean.
- Screen correctly rejects an unrelated pair; decide confirms a real shared
  n-gram pair.
- Clean ledger chain verifies; a **field mutation** in event 2 is detected; a
  **reorder** is detected.
- Re-measurement of a burned object is flagged by `double_burn`, **and the
  forged second run still produces a fully valid chain** — which is precisely
  the point of M2: the lie is well-formed, only the double-use is visible.
- Evidence root is order-binding and a single forged row changes it.
- Grade lattice rejects every synthetic-grade → held-out/generalization edge and
  every synthetic-grade → bare-ratio-row edge; allows correctness, negative
  control, and cost accounting at every grade.
- Audit of 64 objects × 2,016 pairs completed in 95 ms at prototype scale.

### 4.2 The six failures, classified

| # | Check | Diagnosis | Class |
|---|---|---|---|
| 1 | `audit v-exact-a vs v-exact-b` returned 7 reasons, expected 2 | **My declared ground truth was wrong**, not the code. The two fixtures share publisher/project/release family and schema cluster, so c6/c7 correctly fire; being byte-identical, c3/c4/c5 correctly fire. Ground truth corrected in the next revision | harness bug (mine) |
| 2 | `boilerplate blocks are demoted to common` failed | **Fixture design flaw.** `rare_max = 2` means a block in 1 object *is* rare, and my two license fixtures never shared a block. The rare-block rule is therefore untested, not broken. Needs ≥3 fixtures sharing one common block | harness bug (mine) |
| 3 | `screen proposes near-dup pair` failed (jaccard < 0.20 for near-identical 40-line fixtures) | **Genuine unexplained defect, unresolved.** Two fixtures differing only in `id`/`name`/`ts` should screen ≫ 0.20. Not yet root-caused. This is the largest open technical uncertainty in the prototype and it bears directly on M4's Defect A/B repair, so it is on the post-checkpoint work list | **real defect** |
| 4 | `audit v-a vs v-b` missing c5 | Downstream of #3 | **real defect** |
| 5 | `truncation detected` failed | **Genuine protocol finding, as predicted in §2.2.** `verify_chain()` on a *prefix* of a valid chain returns no errors — a shortened append-only log is internally consistent. Truncation is undetectable without an external head commitment. This is exactly the M2 gap, discovered by the harness rather than assumed. Fix = add `committed_head` parameter | **protocol defect (real finding)** |
| 6 | `consuming event recorded` failed | **Self-contradictory assertion I wrote** — I asserted `"v-a" in double_burn(ch)` and `double_burn(ch) == []` in consecutive checks. With one measurement, `double_burn` correctly returns `[]`. Bad assertion, correct code | harness bug (mine) |

Net: **3 harness bugs of mine, 1 real protocol defect found (valuable), 1 real
implementation defect unresolved (must fix before anything is claimed).**

---

## 5. Strongest disconfirming evidence

Ordered by how much it should move my leaning.

**D1 — Novelty: the mechanism is ~70% already written down.**
`docs/I10-CORPUS-LOCK-PROTOCOL.md` already specifies canonical serialization,
order-sorted entries, SHA-256 as content authority, the 8-criterion audit with
a 128-permutation MinHash threshold, the append-only chained ledger with
`previous_event_sha256`/`event_sha256`, fail-closed acquisition, and the §13
portfolio. M1 is "compute the identity the spec already implies, earlier and
without bytes". M4 Defect B is a refinement of criterion 3. Per this project's
own precedent — context clustering was explicitly downgraded to "not ANVIL
novelty, enabling infrastructure" (`RESEARCH_LEDGER.md` Experiment J, :625-660) —
this track must be classified **adopt-class {scientific infrastructure}**, and
I will not claim mechanism novelty for it. The additions I would defend as
non-trivial are narrow: **chain-head commitment** (M2) and the **rare-block
demotion** (M4 Defect B), because each closes a specific, named hole that makes
the protocol's own hard blockers achievable without committing the other hard
blocker it names.

**D2 — This does not unblock the actual blocker.**
E10's blocker is corpus *existence*. A perfect sealing protocol applied to an
empty portfolio seals nothing. §13 requires 4 numeric/telemetry streams
**not** from ANVIL generators, and the only acquisition evidence I have is a
reproduced HTTP-401 cross-host-redirect failure (E13) plus an MD5-only fetcher
(E14). I cannot estimate the probability that §13 is satisfiable with
license-compatible, independently published, non-ANVIL-provenance sources
without a network-enabled provenance audit, and doctrine 7 + PB-06 forbid my
doing that audit here.

**D3 — Acquisition robustness is untouched by this mechanism.**
M1–M4 say nothing about authorization headers, redirect policy, host
allow-lists, or archive availability. E13 shows that is exactly where the last
failed run died, and it cost a complete run (corpus, replay, discovery, V1 and
floor all skipped). A new protocol that is beautiful and a fetcher that 401s is
zero held-out data.

**D4 — The corpus cannot currently be *timed* reliably even if it existed.**
Ledger :4800-4801: enwik8 was ruled `TIMING_BLOCKED` because the Brotli
control's robust CV was **0.1756**. A locked corpus does not fix the arbiter. A
held-out ratio verdict obtained on a runner class with 17.6% control CV is not
a Pareto verdict. Track 04 owns this; but it is a live disconfirmer of the claim
"lock the corpus and held-out validation becomes available".

**D5 — Burning on crash makes preflight correctness load-bearing.**
Protocol §7: "A forbidden local held-out measurement burns the object even if
the run crashes before producing a verdict." So an infrastructure flake in a
4-job pipeline can permanently consume a family that took weeks to source. My
reading — that `identity-fetch`/`identity-inspected` are explicitly non-consuming
(protocol §7: "Viewing identity metadata does not consume a held-out role") and
that a preflight that never invokes a codec is therefore non-consuming — is an
*interpretation*, and it must be written into the workflow before the first
fetch or the first flake burns the portfolio.

**D6 — Contamination is now also inside this swarm.**
40 agents are reading these docs concurrently. If any agent measures a sealed
object locally, PB-06 fires and there is no rollback. The contamination ledger
records `actor`; the mechanism must additionally require that discovery-stage
agents have **no** network path to sealed URLs.

**D7 — My own prototype does not yet pass.**
6 of 33 checks fail, one of them an unexplained defect in the very screen
mechanism M4 depends on (§4.2 #3). Nothing in this checkpoint is validated.

---

## 6. The one decisive REMOTE-ONLY experiment (pre-registered, zero corpus bytes)

**Name:** `LOCK-V1`. **Host:** GitHub Actions only. **Corpus bytes fetched:
0. Codec invocations: 0. Compression outcomes observed: 0. Nothing is burned,
because nothing sealed is touched.**

This is the decisive experiment precisely because it is the one experiment that
*cannot* leak: its input is committed metadata, not data.

Jobs, in order, each fail-closed:

1. `seal` — rebuild `lock_id` and `truth_root` from the committed lock document
   on the pinned runner; assert both equal the values recorded in the
   preregistration; assert `entry_count` and `declared_bytes` match; assert no
   duplicate `corpus_id`.
2. `audit` — run the §5.1 audit over the portfolio metadata + fixture bytes;
   emit `dedup-report.json`, `publisher-split-report.json`, `license-report.json`
   per §5.3; assert tool SHA-256 equals the lock's `audit_tool_sha256`;
   assert **zero** waiver events (there is no waiver code path in the tool).
3. `fault-injection` — the acquisition fail-closed battery of §6, with **no
   network**: synthesize N faulted acquisition cases (wrong byte length, wrong
   SHA-256, wrong git blob, off-allow-list redirect target, missing archive
   member, tampered transform step, oversized body) and assert every one yields
   `INVALID_INFRA` with **zero** codec invocations.
4. `ledger` — the M2 battery: clean chain verifies; single-field mutation
   detected; reorder detected; **tail truncation detected via committed head**;
   double-measurement flagged; forged evidence root detected; a *well-formed*
   double-run is still flagged.
5. `seal-perturbation` — for each of: 1 byte changed in one metadata field;
   entries reordered; one entry removed; one entry added; role changed; license
   changed; schema cluster changed → assert `lock_id` **and** `truth_root`
   change in every case.
6. `adversarial-audit` — a **second, independently authored** auditor checks the
   committed artifacts only (no corpus access, no network) and must reach the
   same verdict as job 2. Disagreement blocks.

### 6.1 Pre-registered thresholds — fixed now, before any run

**PROMOTE to a real held-out dispatch iff ALL of:**

| Gate | Threshold |
|---|---|
| L1 seal determinism | `lock_id` and `truth_root` reproduced identically on **3** pinned runners with different Python minor versions |
| L2 seal sensitivity | **7/7** perturbation classes change both `lock_id` and `truth_root` |
| L3 fail-closed | **100%** of the fault-injection battery → `INVALID_INFRA`, with codec-invocation counter **= 0** on every case |
| L4 chain integrity | **100%** detection across all 6 tamper classes, **including tail truncation** |
| L5 double-use | a second well-formed measurement run over any consumed object is flagged by **both** auditors |
| L6 audit agreement | independent auditor verdict == job-2 verdict on **100%** of pairs |
| L7 no waiver | **0** waiver events; tool has no waiver code path (verified by grep, hash-pinned) |
| L8 cost | verifier wall time ≤ **120 s** and peak RSS ≤ **256 MiB** at n = 60, B_total = 120 MB on the pinned runner. (RSS ceiling raised from my earlier 64 MiB, per §3.3.) |
| L9 audit scale | full pairwise audit ≤ **1/20** of acquisition wall time at B_total = 120 MB |

**NO-GO / KILL the mechanism iff ANY of:**

| Gate | Kill condition |
|---|---|
| K1 | L1 fails on ≥2 of 3 runners → the canonicalization is not portable; the lock is unreproducible; **KILL** |
| K2 | L2 fails on any class → selection-by-outcome remains possible for that mutation; **KILL** |
| K3 | L3 fails on any case → acquisition is not fail-closed; **KILL** immediately, and record as an adversarial-format defect for track 19 |
| K4 | L4 fails on tail truncation specifically → the double-use detector is defeated by deletion rather than duplication; **KILL M2**, downgrade the ledger to bookkeeping, and record the limitation explicitly |
| K5 | L5 fails → same as K4 |
| K6 | L6 disagrees on any pair → the audit is curator-controllable; **KILL** M4 |
| K7 | L8 exceeded → the verifier is a load-bearing cost that a general-purpose codec cannot justify; **KILL**, and record the measured cost |
| K8 | Any job requires network access to a sealed URL | PB-06 shape; **KILL the run**, burn nothing, escalate |

**PILOT continues, PROMOTE withheld, iff:** K1–K6 pass, K7 fails only on RSS
(and the RSS fix is identified), or K8 not triggered. In that state the correct
next step is still *portfolio acquisition*, not held-out dispatch.

**Explicitly pre-registered as NOT a kill:** `LOCK-V1` cannot succeed at
establishing that a §13-satisfying portfolio exists. That is the *portfolio
gate*, run separately, also with zero compression measurement — provenance,
license, publisher-isolation and independence only. If the portfolio gate
passes, `LOCK-V1` becomes dispatchable; if it fails, broad general-purpose
claims are formally abandoned and only the narrow claims the actual portfolio
supports may proceed.

**Not run locally, ever:** any corpus benchmark, sweep, or heavy fuzz campaign.
The prototype self-test in §4 is hash/decision logic only — no corpus bytes, no
codec, no timing claim — and is the sole local execution in this track.

---

## 7. Current leaning: **PILOT**

Not HOLD — the blocker is concrete (E3: the protocol is 100% unimplemented;
E10: the corpus is the named promotion blocker) and this track is the only
thing that makes it attackable without leaking.

Not PROMOTE-TO-REMOTE — three independent reasons, any one sufficient:
1. The prototype fails 6 of 33 self-test checks, including an unexplained
   defect in the screening mechanism the design depends on (§4.2 #3).
2. No `corpus-lock.json` exists, so there is nothing to seal and no portfolio to
   audit (E3, E10). Dispatch would be guaranteed-empty.
3. Remote dispatch is blocked upstream regardless: the ledger records that the
   dense-frontier and G5D workflows are uncommitted and cannot be dispatched
   without explicitly authorized commit/push (`RESEARCH_LEDGER.md:4946-4964`),
   and I am instructed not to commit or push.

**PILOT means, concretely and boundedly:**
1. Fix §4.2 #1, #2, #5, #6 (my harness bugs) and root-cause §4.2 #3. Re-run to
   33/33. Still logic-only, still local, still no corpus.
2. Author `corpus-lock.json` v1 **metadata-only** for a candidate §13 portfolio
   — as a new isolated file under the prototype directory, never at the repo
   root, never wired in.
3. Freeze §6's thresholds as text. They are frozen as of this checkpoint.
4. Hand `LOCK-V1` to track 04 (dense Pareto) and track 19 (format/security)
   for cross-review: L8's RSS ceiling and K4's truncation case are their
   territory, not mine.

**What would move me to KILL:** K2 or K3 or K6 firing; or the portfolio gate
establishing that §13 is unsatisfiable with license-compatible sources — in
which case I recommend formally abandoning the *broad general-purpose held-out
claim* while retaining M1/M2/M4 as narrow infrastructure, and saying so in the
ledger rather than quietly narrowing the claim.

**What would move me to PROMOTE-TO-REMOTE:** 33/33 self-test, a locked v1 with
a satisfied portfolio gate, and authorized commit/push of the prereg + workflow.

---

## 8. Open uncertainties (post-checkpoint work list, ranked by verdict impact)

1. **Root-cause §4.2 #3** — why two near-identical fixtures screen below 0.20.
   If the screen is under-sensitive, M4's Defect A repair does not work and the
   near-duplicate defense for low-overlap same-schema pairs collapses. Highest
   impact on the verdict.
2. **Portfolio satisfiability** — can §13 be met (esp. the 2 non-ANVIL numeric
   streams) with license-compatible, independently published sources? Not
   answerable offline; belongs to a network-enabled control-plane step.
3. **Verifier RSS at scale** — the 32-bit-screen / 64-bit-decide refinement is
   a design, not a measurement. L8 decides it.
4. **Transport/authorization robustness** — out of my track's scope but the
   measured cause of the last invalid run (D3); must be handed to track 19.
5. **Timing determinism on the CI runner class** — D4; track 04's, not mine.