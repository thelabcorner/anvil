# SYNTH-CORPUS-FLEDGE — Corpus Red Team Synthesis

**Agent:** Fledge Alpha Free (independent adversarial reviewer)
**Date:** 2026-10-02
**Track:** 03-heldout-corpus / second-pass synthesis
**Independence statement:** reconstructed from primary artifacts. `03-heldout-corpus-space-bunny.md` was read only *after* my own report was written, for reconciliation (§13B of my report). No Space Bunny number is used here as an independent fact.
**Provenance:** all **[M]** items below were measured by me from bytes in this worktree. **[R]** = repo-recorded. **[A]** = judgment.

**Mandate compliance:** this synthesis is a *new* file at the coordinator's instruction. My primary deliverable remains `03-heldout-corpus-fledge.md`. No existing file was modified by me. No commit/push/reset/clean/stash/restore/rebase. No corpus benchmark, sweep, or heavy fuzzing was run — my local work was hashing, generator reading, PE header parsing, and SQLite record extraction.

---

## 0. One-paragraph verdict

The corpus presents **24 files and roughly 4–5 independence units**. Every synthetic family collapses into **two** units because two scripts produced all 14 of them; the single real-world family is **six host-installed binaries from one OS vendor**, of which **one** is from a genuinely different producer, **all six** are untracked in git, and **none** can be honestly expressed in the protocol's own schema. Separately, two already-consumed families (`pe-*` S6-2, Sino-US DrugQA V1) are **permanently non-held-out** and must never be counted again. The consequence is that **the structured-data and numeric claims the project is actively pursuing have zero admissible held-out evidence behind them.**

---

## 1. Independence-unit recount — the number that decides everything

### 1.1 The arithmetic

**[M]** measured. 24 files, 21,127,883 B. All 24 SHA-256 values match `tests/corpus/CHECKSUMS.txt` — the corpus is *honest about its bytes*, which is worth saying because it is the one thing it gets right.

| Unit | Members | Counted as | Evidence |
|---|---|---|---|
| **U1** `make_smoke_corpus.py` lineage | `generated.json`, `generated.jsonl`, `generated.log`, `generated.repeat.jsonl`, `generated.sqlite`, `random.bin` | **1** | 6 files, one 32-line script **[M]** |
| **U2** `make_synth_corpus.py` lineage | 8 × `synth-*` | **1** | one script, one author, seeds 90004–90008 **[M]** |
| **U3** ANVIL self-reference | `anvil.exe`, `anvil_bench.exe`, `src.cpp` | **inadmissible** (§4) | instrument + its own source **[M]** |
| **U4** docs | `doc.md`, `README.md` | **0** (project prose, not a family) | repo-owned **[M]** |
| **U5** Microsoft PE family | `pe-winver`, `pe-where`, `pe-notepad`, `pe-python`, `pe-ninja` | **1** | one linker, one vendor, one OS build **[M]** |
| **U6** Git/MinGW PE | `pe-git.exe` | **1** | only non-MSVC producer **[R]** + linker field does not evidence it **[M]** |
| **U7** (collision merge) | `{pe-where, pe-winver}` | **already inside U5** | **1 shared exact 4 KiB block [M]** |

**Admissible units ≈ 4** (U1, U2, U5, U6). **Independent *real-world* units ≈ 2.** **Independent real-world units that are structured or numeric = 0.**

### 1.2 Why file count is the wrong currency — and it is being used

**The corpus presents 14 synthetic files that look like 14 families and are, by the protocol's own §5.1 rule 8, two.** A gate written as "N synthetic files" is satisfiable by one generator emitting N files. This is not hypothetical: `make_synth_corpus.py`'s own docstring shows the project *deliberately* adding files in response to mechanisms that "found nothing" **[M]** — the file count grew because mechanisms were being fed, not because evidence was being added.

### 1.3 The cross-container finding — the sharpest one

`tests/make_smoke_corpus.py:31` builds `generated.sqlite` by inserting `json.dumps(rows[i % len(rows)])` — the same `rows` serialized into `generated.json` at line 7.

**[M] Measured: 12,000 of 12,000 SQLite `payload` values are byte-verbatim `generated.json` rows.**

So `generated.sqlite` contains **100%** of `generated.json`'s information. The corpus presents a `structured_json` family and a `sqlite` family that are **one family in two containers**.

**And it is invisible to the cheap dedup rule.** My corpus-wide 4 KiB block scan found **zero** shared blocks between `generated.json` and `generated.sqlite` **[M]** — SQLite page framing interleaves the payload so no aligned block survives. A content-only rule passes this pair clean.

The same re-framing appears twice more **[M]**: `synth-ndjson-columnar.ndjson` and `synth-telemetry-f64.bin` both compute the identical value model `20.0 + 0.001*i + r.randint(-2,2)*0.0001` (`make_synth_corpus.py:240-267`); `generated.repeat.jsonl` shares `generated.jsonl`'s record template.

**Net: the corpus is substantially a re-encoding benchmark for two generators' output models.** A structured-data router will look excellent here for reasons unrelated to generalization.

---

## 2. Self-authored and synthetic overcounting

### 2.1 The instrument is inside the instrumented corpus

**[M]** `anvil.exe`, `anvil_bench.exe`, and `src.cpp` are corpus members:

- `src.cpp` is the codec source; `anvil.exe` is the codec built from it — **parent and child in the same measurement table.**
- `anvil_bench.exe` is the **measurement instrument**. Any harness change moves a corpus row.
- `build/anvil_bench.exe` (`379341d9…`) ≠ `tests/corpus/anvil_bench.exe` (`fcd30da5…`) **[M]** — the corpus copy is a **stale, different build** than the binary that runs measurements.
- `tests/corpus/README.md:177-194` documents a **prior** manifest-drift incident on exactly this file: silently re-pinned to on-disk bytes, old bytes **unrecoverable** because `.gitignore` matches it **[R]**.

**Severity: high, and already once realized.** The response at the time ("RESOLVED… no re-calibration needed") was defensible for one incident; the objection is that a **recurring class** was closed as an **incident**, and §6's fail-closed rule was not applied. The corpus copy being divergent from `build/` today proves the class is live.

### 2.2 Synthetic bias is directional, not just synthetic

Every generator writes from a small Python-level model — a handful of integers with arithmetic strides, or one f-string template **[M]**. Real corpora contain entropy no generator emits: natural-language variance, edit history, vendor formatting quirks, encoding mixtures, adversarial tails.

**So the synthetic families are systematically biased toward the mechanisms ANVIL is building.** `synth-columnar-align.bin` was built so that "a finite-difference / columnar mechanism that finds the true field offsets gets long near-constant-delta runs; one that scans wrong offsets gets nothing" **[M, `make_synth_corpus.py:126-127`]**. That is a fair **unit test** and a biased **benchmark**.

The project's own history shows the trap firing: the synth corpus was added because experiments found the real corpus's periodic/arithmetic signal "at the hash-noise floor" **[R, README:55-57]** — the real corpus was correctly judged signal-free and a substitute was built. Legitimate as a **mechanism-existence** test; illegitimate as **evidence of generalization**.

**Required reclassification (novel to this report):** every synthetic family moves from "evidence" to **"adversarial control with known ground truth."** Legitimate uses: prove a mechanism *can* fire; prove no regression; measure detection sensitivity. Illegitimate: any generalization claim, any aggregate ratio, any Pareto frontier point. The protocol's role enum (§2.1) has **no such role** — `synthetic_control` must be added.

### 2.3 Self-authored textual data is not a text family

`doc.md` and `README.md` are **the project's own prose** **[M, git-tracked, README:9-10 states "hand-written project doc (committed)"]**. Compressing the codec's own documentation is a self-referential measurement. It is a *sanity check*, not a `text_log` or `source_code` family.

---

## 3. Contamination already incurred — what can never be held out again

This is the part most likely to be quietly forgotten, and it is irreversible.

### 3.1 The S6-2 PE set is consumed

**[R]** `RESEARCH_LEDGER.md:3419-3497` records that all six `pe-*` binaries were locally measured and their orientation ratios published. Protocol §2.3 is explicit: "the historical S6-2 PE set are open known/anchor data and **MUST NOT be relabeled as unseen**."

**[M]** Corroborating structural evidence: the file's own `README.md:59-67` publishes each binary's SHA-256, byte length, **version, and exact filesystem origin** — `10.0.22621.1`, `3.12.4 (tags/v3.12.4)`, `1.13.2`, `2.55.0.windows.3`. That is anatomy disclosure, and it is committed.

**Therefore the executable held-out family is 0, not 6.**

### 3.2 Sino-US DrugQA V1 is consumed

**[R]** `RESEARCH_LEDGER.md:4759-4760`: V1 is `V1-COLUMN-ADVERSE` and remains known stress, not held-out evidence. It was legitimately opened by G3. **The only structured held-out corpus ever opened is gone.**

### 3.3 The ledger's own summary agrees

**[R]** `RESEARCH_LEDGER.md:4831-4833`: *"No new held-out structured corpus is currently available. Promotion requires new locked independent structured, mixed-validity, executable, and numeric families."*

**Independent confirmation: I reached this from corpus composition alone, before reading the ledger, via §1.1 and §13A.8.** The constructive report reached it from ledger citation. Convergent, from opposite directions.

### 3.4 Contamination inside this swarm

40 agents are reading these docs concurrently **[M: 38 files now exist in `docs/swarm-2026-10-02/`]**. If any agent measures a sealed object locally, PB-06 fires with **no rollback**. Discovery-stage agents must have **no network path** to sealed URLs.

---

## 4. Toolchain and publisher lineage

### 4.1 What the corpus can and cannot prove

**[M]** PE header parse of all six (`e_lfanew` → machine, TimeDateStamp, optional-header magic, linker version):

| File | Linker | Producer **[R]** |
|---|---|---|
| `pe-winver` | 12.1 | Microsoft (MSVC) |
| `pe-where` | 12.1 | Microsoft (MSVC) |
| `pe-notepad` | 12.1 | Microsoft (MSVC) |
| `pe-python` | 12.1 | Microsoft (MSVC) — python.org build |
| `pe-ninja` | 12.1 | Microsoft (MSVC) — ninja-build project |
| `pe-git` | 12.1 | Microsoft (MSVC per header) / **MinGW-w64/GCC** per `README.md:76` |

**All six report linker 12.1 [M].** The optional-header linker version **does not encode toolchain** — MinGW-w64 links via `ld` and still emits a 12.x-style header. So:

- **Producer diversity: 1 of 6 genuinely distinct.** Three of six are `%SystemRoot%\System32` from **one OS build (10.0.22621.1)** **[R]** — no repository owner exists for them at all, which makes protocol §5.2's "repository owner MUST be new" **unsatisfiable** for exactly the data the project needs.
- **Toolchain diversity: unevidenced.** The MinGW claim is **prose with no in-corpus artifact**. Protocol §13 demands "two unrelated toolchains"; the corpus cannot substantiate its own claim. **A prose claim is not an attestation** — `toolchain_attestation` with an evidence hash (build-id, `.comment`/Rich header, registry digest) is required.

### 4.2 Untracked = not reproducible, and invisible to the lock

**[M]** All six `pe-*.exe` **plus** `synth-columnar-align.bin`, `synth-drift-stride.bin`, `synth-telemetry-f64.bin` are **untracked** (`git status --porcelain` → `??`).

Consequences:
1. A fresh clone has **no PE set and three fewer synth files** — and `make_synth_corpus.py`'s reproducibility claim does not hold for a fresh checkout.
2. **No `git_blob_sha1` can be computed** for them. Protocol §4.10 requires git-blob identity for git-derived objects; for these, the *only* identity is a local hash that nothing in version control protects.
3. **Schema inexpressibility [A]:** §4.9's `source.kind` ∈ {`git-raw`, `https-file`, `archive-extract`, `generate`}. Host-installed binaries are **none of the four**. The project's only real-world family **cannot be honestly recorded** in its own protocol, while all 14 synthetic files record perfectly via `generate`. **The schema encodes synthetic bias; it does not merely fail to catch it.**
4. The lock binds *fetched bytes*, never *absence from version control* — so a smaller clone still verifies PASS.

### 4.3 The MD5 acquisition inconsistency

**[M]** `tools/gha_fetch_corpus.py` verifies **byte length + MD5 only** (`SILESIA` table of `(bytes, md5)`; `verify()` compares MD5). Its docstring advertises it. This is the **remote** path — the one producing citation-grade held-out numbers — using a hash the protocol (§3: "SHA-256 is the content authority") has already rejected. Self-inflicted PB-04/PB-02-class inconsistency, trivially fixed.

---

## 5. Already-consumed data: the consolidated ledger

| Family | Origin | Units | Status | Can it ever be held out? |
|---|---|---|---|---|
| `pe-*` S6-2 set | host-installed, one vendor | 1 (+1 MinGW) | **`known_stress`** — measured, anatomy published | **No.** Irreversible |
| Sino-US DrugQA V1 | opened by G3 | 1 | **`known_stress`** — `V1-COLUMN-ADVERSE` | **No.** Irreversible |
| `generated.*` | `make_smoke_corpus.py` | **1** | discovery / synthetic control | **No** — never real |
| `synth-*` | `make_synth_corpus.py` | **1** | discovery / synthetic control | **No** — never real |
| `anvil.*`, `src.cpp` | ANVIL itself | — | **self-reference control** | **No** — inadmissible by construction |
| `doc.md`, `README.md` | ANVIL itself | — | **self-reference control** | **No** |
| Silesia / enwik8 | remote fetch | 1 | **`external_anchor`** (§2.3) — open anchor, **never** unseen | **No** — anchor by rule |
| AITDCC A–H | external | ≥1 | `external_development` | **No** |
| AITDCC I–P | external | ≥1 | `external_test` — the **only** unopened real family **[R]** | **Yes** — historical-split + procedural blinding only |

**Admissible held-out real families available today: AITDCC I–P, and nothing else.**

---

## 6. Hidden-cost and resource audit

### 6.1 Decoder cost: genuinely zero

**[M]** Corpus discipline adds **0 bytes** to the wire, **0** decoder states, **0** cycles, **0** RSS. Nothing is framed or selected by the codec. This is the right design and I found no leakage of corpus metadata into the bitstream. **Recorded explicitly because the mandate asks, and the honest answer is "none."**

### 6.2 Audit compute: a non-issue — so there is no resource defense of the status quo

**[M]** over the actual 21.1 MB corpus:

| Stage | Measured cost |
|---|---|
| SHA-256, all 24 files | **~14 ms** (SHA-NI, ~1.5 GB/s) |
| 4 KiB block set + full pairwise intersection | ~5,159 blocks; ~13.3M lookups; **< 1 s** |
| Block-hash resident set | ~5,159 × 32 B ≈ **165 KB** |
| MinHash 128-perm over ~6.9 MB text | ~145M ops ≈ **0.5–1.5 s** 1-thread |

**Verdict: the protocol is affordable by orders of magnitude.** Nobody can defend the current state on resource grounds.

### 6.3 The costs actually being paid invisibly

- **Scientific:** the 2026-08-21 manifest re-pin — unrecoverable bytes plus a re-baselining risk argued away rather than measured **[R]**.
- **Opportunity:** the strongest ratio claims rest on families with 1–2 independence units.
- **Reproducibility:** 9 untracked files; a fresh clone has no held-out set at all.
- **Licensing:** the only real-world family is Windows binaries + CPython (PSF) + Ninja (Apache-2.0) + Git (**GPLv2** — triggers `attribution_required`, forcing license text into every artifact). `PB-13` is currently **unenforceable**: nothing defines how `redistribution_allowed` is determined or **from what authority**.

---

## 7. Format-family coverage vs. what is actually being claimed

| Active mechanism | Family it needs | Admissible held-out units available | Verdict |
|---|---|---|---|
| SPARSE-REF | structured JSON/NDJSON | **0** | **unsupported** |
| Shape-conditioned displacement | structured JSON/log | **0** | **unsupported** |
| TCOPY / PNRA | executable `.text` | **0** (PE set consumed) | **unsupported** |
| Context-switched literals | mixed text/structured | **0** | **unsupported** |
| Numeric / Gorilla-class | real telemetry | **0** (all 4 numeric files are synthetic) | **unsupported** |

**[M]** `docs/CONTEXT.md`'s headline `generated.json` 0.1325 vs. brotli q9 0.137 result is a **fact about `make_smoke_corpus.py`'s schema**. It would survive almost any mechanism change so long as the router handles that schema. **It is not evidence about the world.**

---

## 8. THE MATRIX — may-discover / may-pilot / may-promote, per family

Legend: **✔** permitted · **~** permitted with stated constraint · **✘** forbidden · **—** impossible today (no admissible data exists)

| Family | Independence units | May **discover** (tune/ablate/profile) | May **pilot** (remote dry-run, frozen impl) | May **promote** (held-out/generalization claim) | Governing constraint |
|---|---:|---|---|---|---|
| `generated.*` (U1) | **1** | ✔ tune, ablate, profile | ~ only as **negative/positive control**, labeled `synthetic_control` | **✘** | Self-authored; `generated.sqlite` ≡ `generated.json` (12,000/12,000 **[M]**) |
| `synth-*` (U2) | **1** | ✔ mechanism-existence testing | ~ sensitivity calibration only, at **S0** sealed params | **✘** | One generator, one author, seeds 90004–90008 **[M]**; buys *sensitivity*, never *generalization* |
| `pe-*` MSVC (U5) | **1** | ✔ regression, failure anatomy | ✘ | **✘** | **Already consumed** (S6-2, anatomy published) → `known_stress` **[R]** |
| `pe-git` (U6) | **1** | ✔ regression | ✘ | **✘** | Same consumption **[R]**; toolchain claim unevidenced **[M]** |
| ANVIL self (`anvil.*`, `src.cpp`) | — | ✔ self-reference sanity | ✘ | **✘** | Instrument inside the instrumented corpus; `build/` ≠ corpus copy **[M]** |
| ANVIL prose (`doc.md`, `README.md`) | — | ✔ sanity | ✘ | **✘** | Project's own writing; not a text family |
| Sino-US DrugQA V1 | **1** | ✔ known-stress / adverse diagnosis | ~ **frontier reconstruction only**, after held-out authorization | **✘** | Opened by G3; `V1-COLUMN-ADVERSE` **[R]** |
| Silesia / enwik8 | 1 | ✔ open anchor, frontier context | ~ anchor regression | **✘** | `external_anchor` by §2.3 — **never** unseen; MD5 fetcher **[M]** |
| AITDCC A–H | ≥1 | ✔ external development | ✔ as development data | **✘** | `external_development` by §8 |
| **AITDCC I–P** | ≥1 | **✘** (sealed) | ✔ once lock + prereg frozen **[R]** | ~ **conditional** — see gates below | `external_test`; historical split + procedural blinding, **not cryptographic** (§8) |
| **NEW foreign structured ×2** | **0 — must acquire** | — | — | — | **The actual blocker [M]: 0 exist today** |
| **NEW real numeric/telemetry ×1** | **0 — must acquire** | — | — | — | **The actual blocker [M]: 0 exist today** |
| **NEW foreign executable family** | **0 — must acquire** | — | — | — | Must clear licensing + producer isolation (§4.1) |
| **NEW mixed-validity/malformed ×2** | **0 — must acquire** | — | — | — | **0 exist in `tests/corpus` [M]** |

### 8.1 Gates that must ALL pass before anything reaches the promote column

1. `corpus-lock.json` + `corpus-lock.sha256` exist and verify (§2 **[M]**: none exist).
2. **Independence units are computed, not asserted** — transitive union-find over published conflict edges; the tool must have no curator-supplied group ids.
3. `conflict_edges[]` published as a first-class artifact, including the **measured** `pe-where`/`pe-winver` 4 KiB edge **[M]** and the **measured** `generated.sqlite`⊂`generated.json` containment **[M]** — neither waived.
4. Cross-container containment (criterion c9) implemented — the fixed-4 KiB rule **provably cannot** see it **[M]**.
5. `synthetic_control` role exists; no synthetic file backs any promote-column cell.
6. `host-installed` source kind + `git_tracked` per entry + toolchain attestation with an evidence hash (§4.2).
7. Acquisition verifies **SHA-256**, never MD5 **[M]**.
8. Burn rule specified: **burn on result view only**; infra-fail-without-view retryable under a new workflow SHA, pre-registered cap.
9. Coordinator ruling on `source_dirty:false` (§4.2 blocker B2 in my main report) — a **decision**, not work.
10. Ledger appended by the **workflow**, not the human.

**Items 1–9 currently fail. Item 10 is absent.** Therefore **no family in this table may be promoted today.**

---

## 9. Strongest falsification case against my own position

I must state what would move me.

**My position is "0 admissible held-out units exist for structured and numeric claims."** It would be wrong if any of these is true:

1. **The PE set is genuinely held out.** Falsifier: a ledger entry showing the six `pe-*` ratios were never published. I found the opposite **[R]**, plus committed anatomy in `README.md:59-82`.
2. **V1 is still held out.** Falsifier: it is absent from the ledger as opened/`V1-COLUMN-ADVERSE`. Found the opposite **[R]**.
3. **The `generated.*`/`synth-*` files have more than two lineages.** Falsifier: a second generator script for either family. I read both generators end-to-end **[M]**.
4. **`generated.sqlite` does not contain `generated.json`'s rows.** Falsifier: any non-verbatim payload. Measured 12,000/12,000 verbatim **[M]**.
5. **Something real and structured already exists outside `tests/corpus`.** Falsifier: a foreign structured corpus in the tree. **This is my weakest point** — I searched `tests/corpus` and `docs/`, not every path. **Recommend the coordinator confirm no other corpus is in the tree before acting on the 0-count.**

**Falsification of the *contructive* position, for symmetry:** LOCK-V1 could be claimed to succeed on all nine gates while `independence_units` remains uncomputable and grouping stays manual — because no gate tests either. **If `GATE-INDEP-1/2` are not added, a passing LOCK-V1 is not evidence that the corpus is clean.**

---

## 10. Prior-art map (methodology — brief; full map in my main report §13)

The protocol is **not novel, and does not need to be** — it is a correct instance of standard practice: ML hidden-test-set discipline (Kaggle sealed finals; Dwork et al. DP board-vs-public), JPEG 2000 Part 4 train/test splits, CALIC/BOSS evaluation windows, Broder shingling/MinHash, W3C PROV / SPDX / SLSA / Sigstore attestation, and dataforge-style dedup (whose **content-defined chunking** remedy the protocol omits). **There is no scientific prize here** — the rational move is to implement the minimum that makes evidence trustworthy and spend the research budget on mechanisms.

**One genuine improvement over prior art, and it is the direct fix for §1, §3, §4.2 and §6.3 simultaneously: seal a generator specification + seed rather than (or alongside) sealing data bytes.** Gives reproducibility without redistribution, auditable lineage **by construction**, license avoidance, and CI acquirability without shipping binaries. **Honest caveat [A]:** a sealed generator is genuine held-out only if its *design* was not tuned against observed outcomes — sealing the artifact does not launder design-time leakage. Use it for **family construction**, never as a substitute for real published corpora.

---

## 11. Decisive remote-only experiment (pre-registered, zero corpus bytes)

**Name:** `CORPUS-ADMISSIBILITY-v1` — GH Actions only. **Corpus bytes fetched: 0. Codec invocations: 0. Compression outcomes: 0.** Nothing sealed is touched, so nothing is burned.

It is decisive in **both** directions: it can confirm the corpus is inadmissible (§8 matrix → no promotion), and it can detect a false negative in *my own* audit.

Jobs, each fail-closed:

1. **`enumerate`** — compute the true universe of corpus objects in the tree and in `external_anchor`/AITDCC config; assert it equals what §1.1 assumed. **This is the falsifier for my weakest point (§9.5).**
2. **`units`** — transitive union-find over the §5.1 conflict graph; emit `conflict_edges[]` + `independence_units`. Assert `independence_units ≤ 5` for the current corpus **[M]** — a higher number falsifies my §1 recount.
3. **`containment`** — criterion c9: for each structured/SQLite pair, test whether one entry's decoded record set is contained in another. Assert `generated.sqlite` ⊇ `generated.json` **[M]**. A negative result falsifies §1.3.
4. **`criterion5-rewrite`** — validate the constructive lane's measured finding (5-gram Jaccard ≤ 0.0074 on 28 real overlapping pairs) and their unigram screen; assert the rewritten rule fires on ≥90% of known-overlapping pairs and ≤5% of known-independent pairs.
5. **`threshold-lint`** — assert no threshold lives in a code default: `rare_max`, `k`, `0.20` must come from the lock. **[M]** three currently do not.
6. **`provenance`** — assert every entry has `source.kind`, `git_tracked`, license verdict **with a source of truth**, and toolchain attestation with an evidence hash.
7. **`blind`** — a second, independently authored auditor reaches the same `independence_units` and the same edge set from committed artifacts only. Disagreement blocks.

### Pre-registered thresholds

**PROMOTE the protocol to remote dispatch iff ALL:** L1 universe enumerated and matches assumption · L2 `independence_units ≤ 5` reproduced · L3 containment edge present for sqlite⊃json · L4 rewritten rule 5: ≥90% recall on known-overlapping, ≤5% FP on known-independent · L5 zero code-default thresholds · L6 provenance complete for every entry · L7 blind auditor agrees 100%.

**KILL / NO-GO iff ANY:** K1 `independence_units ≥ 8` (**my §1 recount is wrong** — re-derive everything) · K2 containment edge absent (**sqlite/json are genuinely independent** — drop §1.3 and B7) · K3 rule-5 rewrite cannot reach ≥90% recall on structured pairs (**§5.1 rule 5 is unfixable; content-only dedup is the only option**) · K4 `independence_units` cannot be computed deterministically across two runners (**the whole approach is not portable**) · K5 any job needs network access to a sealed URL (**PB-06 shape; kill the run, burn nothing**).

**PILOT continues, PROMOTE withheld iff:** K1–K4 pass and only K5 is untriggered, or rule-5 recall lands in [70%, 90%).

**Explicitly pre-registered as NOT a kill:** `CORPUS-ADMISSIBILITY-v1` **cannot** establish that a §13-satisfying portfolio is *acquirable*. That is the **portfolio gate** — separate, network-enabled, provenance/license only. Run it; if it fails, formally abandon the **broad general-purpose held-out claim** and say so in the ledger rather than quietly narrowing it.

**Never run locally:** any corpus benchmark, sweep, or heavy fuzz campaign.

---

## 12. Recommendation

### **PILOT** — with the promote column of §8 frozen shut

**PILOT** because the decisive question is not "is ANVIL good?" but **"does the corpus support any claim at all?"** — and that is answerable for ~1 s of CI and ~165 KB of RAM per §6.2, with zero bytes burned. A cheap experiment that can return a decisive negative about the *evidence base* is worth running.

**The clause that matters:** §8's promote column is empty for every family except AITDCC I–P, and even that is conditional on 10 gates, **9 of which currently fail**. Until `corpus-lock.json` exists and independence units are *computed*, the correct description of `tests/corpus` is the one its own README already uses: **a smoke/development corpus** **[R]** — and other artifacts have drifted past that wording.

**NOT PROMOTE-TO-REMOTE.** There is no mechanism to promote; the deliverable is a protocol and an experiment.

**NOT KILL.** The methodology is not unfixable — it is unimplemented, and unimplemented is cheap. But **NOT HOLD either**, because HOLD would imply waiting on a decision, and the two cheapest blockers (**B1** relabel the corpus, **B2** rule on `source_dirty`) are available to the coordinator right now and unblock everything else.

### Immediate actions, cheapest first

| # | Action | Cost |
|---|---|---|
| 1 | Stop calling `tests/corpus` held-out anywhere; restore the README's own wording | minutes |
| 2 | **Rule on `source_dirty:false` vs. the dirty-worktree doctrine** — a decision that blocks §12 step 1 permanently | **decision** |
| 3 | Remove `anvil_bench.exe` from the corpus; class `anvil.exe`/`src.cpp`/prose as `self_reference_control` | minutes |
| 4 | `gha_fetch_corpus.py`: MD5 → **SHA-256**, both pinned | minutes |
| 5 | Track the 9 untracked files, or relocate them to fail-closed CI acquisition | minutes |
| 6 | Add `synthetic_control` role, `host-installed` kind, `git_tracked` per entry | hours |
| 7 | Add criterion **c9** (cross-container containment) and demotion immunity for cross-role pairs | hours |
| 8 | Compute independence units mechanically; group ids become **derived**; L6 compares **components** | hours |
| 9 | Acquire **2 foreign structured + 1 real numeric** families — *the actual blocker*, and the only item here that decides whether the project's structured and numeric evidence means anything | days + licensing |

**Items 1–5 are minutes to hours and remove essentially all present interpretive risk. Do them before any further remote dispatch.**

---

## 13. Measured vs. assumed — full ledger for this synthesis

| # | Claim | Label |
|---|---|---|
| 1 | All 24 corpus SHA-256 match `CHECKSUMS.txt` | **[M]** |
| 2 | No `corpus-lock.json`, ledger, or audit reports exist anywhere | **[M]** |
| 3 | 24 files → **~4 admissible units** (~2 real-world) | **[M]** |
| 4 | All 8 `synth-*` from one script/author/seeds 90004–90008 | **[M]** |
| 5 | All 6 `generated.*` from one 32-line script | **[M]** |
| 6 | **12,000/12,000** sqlite payloads verbatim from `generated.json` | **[M]** |
| 7 | `generated.json` ↔ `generated.sqlite` share **zero** 4 KiB blocks | **[M]** |
| 8 | Corpus-wide: **exactly 1** 4 KiB collision, `pe-where`↔`pe-winver` | **[M]** |
| 9 | All 6 PEs report linker 12.1 → toolchain unevidenced | **[M]** |
| 10 | 9 corpus files untracked, incl. all 6 PEs | **[M]** |
| 11 | `benchmark-suite.csv` has **zero** `pe-*.exe` rows | **[M]** |
| 12 | `build/anvil_bench.exe` ≠ corpus copy | **[M]** |
| 13 | `gha_fetch_corpus.py` verifies MD5, not SHA-256 | **[M]** |
| 14 | Audit cost < 1 s, ~165 KB RAM | **[M/M-estimated]** |
| 15 | Decoder-visible cost of corpus discipline = **0** | **[M]** |
| 16 | 38 report files now exist in the swarm dir | **[M]** |
| 17 | §8 coverage matrix: structured and numeric admissible units = **0** | **[M]** counted |
| 18 | PE set consumed (measured + anatomy published) | **[R]** |
| 19 | Sino-US DrugQA V1 consumed by G3, `V1-COLUMN-ADVERSE` | **[R]** |
| 20 | AITDCC I–P is the only unopened real family | **[R]** |
| 21 | `host-installed` binaries inexpressible in §4.9's four `source.kind` values | **[M]** kinds + provenance read; **[A]** the conclusion |
| 22 | §5.2 publisher isolation unsatisfiable for System32 binaries | **[A]** |
| 23 | Sealed-generator sealing is better than shipping binaries | **[A]** — justified, but a judgment |
| 24 | 2026-08-21 manifest re-pin was "manifest-laundering" | **[A]** — strong inference from [M]+[R]; the one-off reasoning was defensible, the objection is to the class |
| 25 | **No other corpus exists elsewhere in the tree** | **[UNVERIFIED]** — my weakest point; §9.5 falsifier, and job 1 of the experiment |

---

**Final: PILOT** — run `CORPUS-ADMISSIBILITY-v1` with §11's thresholds frozen and §9's falsifiers registered; **freeze §8's promote column shut** until gates 1–10 pass; escalate **B1/B2** to the coordinator immediately.