# SYNTH-CORPUS — Track 03 Second-Pass Synthesis (Space Bunny Free)

**Agent:** Space Bunny Free · **Date:** 2026-10-02 · **Lane:** `03-heldout-corpus`
**Status:** synthesis + reconciliation + queue admissibility review. **No promotion
is claimed or authorized by this document.** No commit/push, no corpus fetch, no
codec invocation, no compression measurement, no fuzz campaign.
**Companion artifacts:** `prototypes/swarm-2026-10-02/03-heldout-corpus/space-bunny/`
(`seal.py`, `lock_emit.py`, `lockout/*.json`).

Evidence tags: **[M]** measured this session from primary bytes · **[R]** repo-recorded ·
**[A]** arithmetic · **[H]** hypothesis · **[P]** projection.
**Benchmark windows are never spliced** (§8).

---

## 0. Headline, in one line

> **24 files → 7 independence units → 0 admissible as evidence → 0 held-out units.**
> The protocol is now executable and it says, mechanically, that this project has
> no held-out corpus and never had one. Nothing else in the swarm can be read as
> promotion until that changes by acquisition, not by reinterpretation.

Measured by `lock_emit.py emit` (W0, this session):

```
lock_id      37c0476444cee44362ca4a9e14f301bb4f2362be2aa44b8bcc321d58e2ce7e6a
truth_root   b6d2c0cf89ff753256c9a7acb3cfdd454d9ee76a288030b6bb0f4d0c3e1b29dc
entries 24   splits {discovery:5, known_stress:19, heldout:0, ...}
units 7      admissible 0      heldout_units 0
audit 156 blocking pairs of 276 compared
violations 6  (all pe-*: source.kind=host-installed has no v1 representation)
```

---

## 1. Mandatory reconciliation with `03-heldout-corpus-fledge.md`

Every measured claim of the adversarial review, independently re-derived from
primary bytes. **Cited numbers that disagree are resolved in favour of the
measurement, not the average.**

| # | Fledge claim | Verdict | My measurement / basis |
|---|---|---|---|
| 1 | 24 files, all SHA-256 match `CHECKSUMS.txt`, 21,127,883 B | **CONCEDED** | [M] 24/24 verified, 0 mismatch, 21,127,883 B |
| 2 | No lock / ledger / audit reports exist | **CONCEDED** | [M] recursive search: zero hits |
| 3 | 24 files → ~4–5 independence units | **CONCEDED, and made stricter** | [M] `independence-units.json`: **7 units**, of which **0 admissible as evidence** |
| 4 | 8 `synth-*` from one script/author/seed family | **CONCEDED** | [M] `make_synth_corpus.py`, seeds 90001–90008 |
| 5 | 6 `generated.*` from one 32-line script | **CONCEDED** | [M] `make_smoke_corpus.py` is 32 lines and emits all six incl. `random.bin` |
| 6 | 12,000/12,000 sqlite payloads verbatim from `generated.json` | **CONCEDED** | [M] 12,000/12,000 (100.00 %); payload set ⊆ json set; also proven at `make_smoke_corpus.py:31` vs `:7` |
| 7 | `pe-where` ↔ `pe-winver` share 1 exact 4 KiB block | **CONCEDED** | [M] exactly 1 collision in the whole 24-file sweep, and it is that pair |
| 8 | **All six PEs report linker 12.1; header cannot evidence toolchain diversity** | **REFUTED** | [M] PE optional-header `MajorLinkerVersion.MinorLinkerVersion` reads **2.46** (`pe-git`), **14.44**, **14.40**, **14.30**×3. MinGW `ld 2.46` *is* separable from MSVC 14.x. Fledge's 12.1 corresponds to the VS2013 linker and is a mis-parse. The README's toolchain claim is **corroborated**, not unevidenced. Producer *isolation* still fails (§5.2 needs a repository owner; System32 binaries have none) |
| 9 | 6 `pe-*` + 3 `synth-*` untracked in git | **CONCEDED, corrected count** | [M] **11** untracked: the 6 `pe-*`, `anvil.exe`, `anvil_bench.exe`, and 3 `synth-*` |
| 10 | `benchmark-suite.csv` has zero `pe-*` rows | **CONCEDED** | [M] 351 data rows, 13 distinct files, **0** `pe-` rows |
| 11 | Corpus `anvil_bench.exe` ≠ `build/anvil_bench.exe` | **CONCEDED** | [M] `fcd30da5…` 1,929,216 B vs `379341d9…` 2,038,272 B |
| 12 | `gha_fetch_corpus.py` verifies MD5 not SHA-256 | **CONCEDED** | [M] `SILESIA`/`ENWIK8` tables are `(bytes, md5)`; `verify()` compares md5 only |
| 13 | §13 portfolio fails 3 of 5 counts | **CONCEDED, worsened** | [M] fails **4 of 5** — structured 0, mixed-validity 0, numeric 0, executable 0 *admissible*; AITDCC unconfigured |
| 14 | Audit ≈ <1 s, ≈165 KB RAM | **CONCEDED** | [M] my prototype: 64 objects / 2,016 pairs in **326 ms**; projection for a real 60-object, 120 MB lock is <1 s verifier CPU |
| 15 | Redistribution terms of the host binaries unclear | **CONCEDED** | [M] now encoded as `unknown-no-redistribution` on all 6, which forces the §4.8 fetch-outside-repo path and blocks artifact upload |
| 16 | Sealed-generator protocol better than shipping binaries | **CONCEDED as direction**, with a limit | [H] I adopt it for *family construction* and reject it as a **substitute for real published corpora**; it cannot manufacture §13's "real numeric stream not from ANVIL generators" |
| 17 | 2026-08-21 manifest re-pin was "manifest-laundering" | **PARTIALLY CONCEDED** | [R] the drift and the re-pin are real. I concede the *class* is live (claim 11 proves it recurred). I do **not** concede "unrecoverable" as an excuse — the correct response was fail-closed acquisition, and re-pinning is what produced a corpus whose instrument ≠ its own instrument |
| 18 | *(new)* `generated.sqlite` is a **separate** independence unit in my first pass | **CORRECTED BY ME** | [M] first emit produced 8 units with sqlite separate; **wrong**. C6/C7 are lineage arguments, so they must merge like content conflicts. Final emit: 7 units, sqlite inside the `generated` unit |

**Net: 15 conceded, 1 refuted (Fledge's PE linker parse), 2 corrected.** The
substantive indictment stands and is now mechanically enforced rather than argued.

---

## 2. Reconciliation of the Q1 self-test failures (root cause + exact rule change)

The coordinator reproduced 5 failures in `seal.py selftest`. Both causes are now
fixed **in the isolated prototype only**, with regression tests added. Neither
fix relabels a curator expectation to force green; both change *semantics*, and
both were derived from data outside the fixtures.

### 2.1 Root cause A — fixture length bug (confirms the coordinator's diagnosis)

`_BOILER` was built as `b"# boilerplate banner block\n" + bytes(range(4096-28))`.
The prefix is **27** bytes, not 28, so the block was **4095 B**. Block 0 therefore
absorbed one fixture-specific byte and could never be common, which is why
"shared by 3 objects" failed while "demoted to common" passed **vacuously** on an
empty intersection.

**Fix:** `_pad_to_block(prefix)` derives the length from `len(prefix)` and
`assert`s `len(out) == BLOCK`. No magic constant remains.

### 2.2 Root cause B — C5 was under-specified in two independent ways

The old rule was *"unigram Jaccard ≥ 0.20 AND any shared 5-gram."* Two defects:

1. **Not text-gated.** `normalize_text` maps arbitrary binary bytes to
   tokenizable replacement text, so a binary pair that shares a block produced
   "shared text" and C5 fired — **double-counting evidence C3/C4 already
   explained**.
2. **Not prevalence-aware.** A shared *licence banner* is common text. It
   confirmed overlaps just as readily as a genuinely unusual phrase.

**Exact semantic rule change** (`audit_pair`, C5 arm):

```
C5 fires  iff
     text_like(a) >= 0.90  AND  text_like(b) >= 0.90          # (i) text-only
 AND unigram_Jaccard(a,b) >= 0.20                             # (ii) propose
 AND residual_rare_grams(a,b) is non-empty                     # (iii) confirm
```
where `residual_rare_grams(a,b)` =
`grams_excluding_blocks(a, a.blocks ∩ b.blocks) ∩ grams_excluding_blocks(b, same) ∩ a.rare_grams ∩ b.rare_grams`

- **`TEXT_LIKENESS_MIN = 0.90`** is *not* fitted to a fixture. It is the
  midpoint-ish semantic definition (≥90 % of bytes printable-or-whitespace)
  and it sits inside the **measured natural gap** in the real corpus:
  all-text files 0.9989–1.0000 · `generated.sqlite` 0.8752 · all binary
  0.1495–0.4418. Largest gap = **0.4418 … 0.8752 (width 0.4335)**.
  Source: `scratch/swarm-tmp/derive_text_threshold.py`.
- **Prevalence** mirrors C3/C4 exactly: corpus-wide document frequency ≤ 2 ⇒ rare.
- **Non-double-counting** is the new part: confirmation must come from *outside*
  any block the pair already shares.

### 2.3 Regression tests added (all now pass)

| Test | Asserts |
|---|---|
| genuine near-duplicate structured **text**, no shared block (`v-textdupA/B`) | C5 **fires** |
| same template, **different publisher** (`v-textdupA/C`) | C5 **fires** — it is the *only* lane that sees this; C6/C7 metadata preconditions are not met |
| shared **common text boilerplate** (`v-boiler1/2/3`) | C5 **silent**, no criterion fires |
| **binary-ish rare 4 KiB** pair (`v-rareA/B`) | C3+C4 fire, C5 **silent** |
| banner hashes identically in 3 objects | `len(shared) >= 1` **and** disjoint from `rare_blocks` **and** ⊆ `common_blocks` |
| `common ∪ rare == blocks`, disjoint | partition exactness for every index |
| byte-identical pair (`v-exact-a/b`) | C1 fires, C3/C4 fire, C5 **silent** (evidence attributed once) |
| protocol's own 5-gram estimator | stays **blind** (regression guard against "fixing" it back) |

**Result: `seal.py selftest` → 64/64 PASS** (W0, deterministic, stdlib only).

Two expectations I had to change, and why that is *not* green-washing:
- `v-exact-a/b` no longer lists C5 — under non-double-counting, a byte-identical
  pair's evidence is attributed to C1/C3/C4. Reporting it three times is noise.
- `v-textdupA/C` **gains** C5. The metadata lanes correctly decline; the content
  lane is what catches a same-template/different-publisher pair. That is a
  load-bearing property, not a loosened threshold.

**Recorded limitation:** C5 cannot separate "same schema template, different
data" from "genuinely near-duplicate text" — both are C5. Disambiguating needs
the **schema-signature lane** (computed field-path signature from a hash-pinned
script). **Not implemented.** It is the one place where a curator-supplied
`schema_cluster_id` is still load-bearing, which is exactly the subjectivity
Fledge §3.4 warns about.

### 2.4 A third defect the fix exposed, and a correction to my own earlier design

My original unit-merge rule unioned on content criteria only. That produced
**8 units with `generated.sqlite` separate from `generated.json`** — wrong, given
12,000/12,000 containment. Corrected rule:

- union on **any** criterion **conflict** (C1/C3/C4/C5);
- union on rule 8: same `producer_id` **and** (same project or release family);
- union on rule 6: same schema cluster **and** same publisher/release;
- **do NOT** union on rule 7's bare publisher equality.

The last clause matters: every ANVIL-authored file shares publisher `"anvil"`,
so a bare publisher test collapses all 24 files into **one** unit (measured: an
earlier draft did exactly that, 18 files in one bucket). Rule 7 is aimed at
*cross-publisher* held-out independence, not at intra-project lineage.

---

## 3. Track → required held-out family map (all 20 tracks)

Legend — **need**: the family class without which the track's *broad* claim
cannot be licensed. **Now**: `✓` usable on current data · `~` usable only as a
control/diagnostic · `✗` blocked. **Ceiling**: what current data can license.

| # | Track | Family class it needs | Now | Ceiling on current data |
|---|---|---|:-:|---|
| 01 | G5D paged base+overlay dictionary | mixed-validity / large structured + numeric tables | ~ | discovery ablation only; zero G5D corpus bytes ever measured |
| 02 | G5A-informed finalist planner | structured + mixed-validity (routing substrate) | ~ | planner regret on discovery only |
| **03** | **held-out corpus protocol** | **the substrate itself** | **✗** | **admissibility audit only — cannot license anything** |
| 04 | Dense Pareto measurement | *external anchors* (Silesia/enwik8) + held-out | ~ | anchors are `external_anchor`, never generalization |
| 05 | Backend frontier beyond Brotli | executable + large mixed text | ~ | byte-only backend cross-product on discovery/anchors |
| 06 | Entropy backend co-design | structured + numeric streams | ~ | byte-only selector arbiter; λ·c uncalibrated |
| 07 | BWT subblocking | large text/executable | ~ | adopt-class decomposition only |
| 08 | DEFLATE reconstruction | precompressed real streams (real-world formats) | ✗ | **no real precompressed family exists at all** |
| 09 | Exact transformed backreferences (TCOPY/PNRA) | **real x64/PE from ≥2 producers** | ✗ | novelty closed by NQ-10; PE set consumed |
| 10 | Correction topology coding | structured sparse-residual | ~ | byte-only mask ceiling; mechanism already killed |
| 11 | Orbit/program synthesis | structured/repetitive text | ~ | paper + arithmetic only (Q0b, Q0c) |
| 12 | Encoder-only search/learning | any (search-side) | ~ | byte-only censuses; H0 withdrawn |
| 13 | Numeric/floating representation compiler | **≥2 real numeric/telemetry streams, non-ANVIL** | ✗ | **hard blocked**; no prototype may start |
| 14 | Executable representation compiler | real executables, ≥2 producers | ✗ | PE set consumed; schema cannot even record it |
| 15 | Long-range / cross-block memory | large single-file mixed | ~ | memory/decode tradeoff on discovery |
| 16 | Segmentation / routing theory | structured + mixed-validity | ~ | regret bounds on discovery |
| 17 | Parser / candidate generation | structured + numeric | ~ | byte-only MDL cost; never the crossing route |
| 18 | New decoder/backend architecture | format-agnostic | ~ | architecture only; needs a format to matter |
| 19 | Format / security / fuzz | synthetic + adversarial | ✓ | **the only track fully served by current data** |
| 20 | Novelty / prior-art kill team | *literature*, not corpus | ✓ | literature mapping; corpus-independent |

**Structural observation.** The three families the frontier work is actually
built around — **structured**, **executable**, **numeric** — are exactly the three
that have **zero** admissible units. The one track that *is* fully served (19,
security/fuzz) is the one with no frontier ambition. That is the whole gating
problem in one table.

---

## 4. Current admissible discovery / control data (classified)

From `lockout/corpus-lock.json` + `lockout/independence-units.json`:

| Unit | Files | Class | Role | Admissible as **evidence**? | Legal use today |
|---|---:|---|---|:-:|---|
| `unit-anvil*` | 4 | self-reference | `known_stress` | **NO** | instrument sanity, self-referential round-trip |
| `generated.*` | 6 | 1 lineage | `discovery` (5) / `known_stress` (1) | **NO** | discovery, anatomy, negative controls, regression |
| `synth-*` | 8 | 1 lineage | `known_stress` | **NO** | detector-sensitivity, must-fail controls, decode-cost accounting, fuzz |
| `pe-*` (4 units) | 6 | host-installed | `known_stress` | **NO — permanently** | executable anatomy; **never held-out, never promotable** |
| **held-out** | **0** | — | — | — | **nothing** |

`pe-*.exe` status, per coordinator ruling: **`known_stress`/control, never
held-out.** They are recorded in `lockout/` and in the tombstone (§6), **not**
git-added. Version control would manufacture a curation history that never
occurred; absent history is more honest than fabricated history.

**Synthetic-control grade lattice** (`lockout/synthetic-grade.json`):
protocol v1 has no `synthetic_control` role, so synthetic families are carried as
`discovery`/`known_stress` and their control semantics live in a sibling artifact
so they can never be mistaken for evidence. Both existing families are **S2** (in
the tuning ancestry): usable for correctness, negative control, cost accounting
and oracle-anatomy calibration; **barred** from blind sensitivity verdicts,
generalization claims, and any reported ratio row without a real counterpart.

---

## 5. Promotion blockers (mechanically enumerated)

`lockout/blocking-gates.json`:

| Blocker | Status | Evidence |
|---|---|---|
| **PB-01** missing lock | **RESOLVED by this scaffold** | `lockout/corpus-lock.json` + `corpus-lock.sha256` now exist |
| **PB-09** missing contamination ledger | **LIVE** | no `corpus-access-ledger.jsonl`; chain-head commitment not yet bound |
| **PB-10** independence conflict | **RESOLVED by this scaffold** | `lockout/dedup-report.json`, 156 blocking pairs, `waiver_path_present: false` |
| **PB-26** no new held-out family | **LIVE — binding** | `heldout_units = 0`; §13 fails **4 of 5** counts |
| **PB-04** identity gap (schema) | **LIVE** | 6 violations: `source.kind` ∈ {git-raw, https-file, archive-extract, generate} has **no representation for a host-installed binary** |
| **PB-11** waiver pressure | **guarded** | no waiver code path in the tool; asserted by grep + tool hash |
| **PB-12/13** license | **LIVE for the 6 PEs** | all 6 `unknown-no-redistribution` → may not be uploaded as artifacts |

**Schema gap, stated as a required amendment (§4.9):** a `source.kind` value for
**host-snapshot** entries (`host`, `host_path`, `host_sha256_pinned`,
`redistribution: unknown`). I did **not** modify
`docs/I10-CORPUS-LOCK-PROTOCOL.md`; instead the emitter emits `https-file` with
`url: null` so `schema-validation.json` **fails loudly** on exactly those six
entries. A silent workaround would have hidden the only thing the lock can still
teach us.

---

## 6. Consumption tombstone (`QP-CONSUMED`)

Append-only, keyed by SHA-256. Any future lock offering one of these as
`heldout`/`external_test` MUST be rejected; this is a byte-identity check and
cannot be waived. Emitted by `lockout/license-report.json` + the ledger.

```
1a0043555d254618f2d56c936c3d9a1fbfb878bc878416a133c346bc7835eda9  pe-git.exe
e52a7ad9538d9618c67a0bd777964e2eec8a30f68b810a2f6adce1f2daf847b8  pe-ninja.exe
49f096cbf9337b0a80bde835d29be41bc9371057c4ff6c72f8a36158c29cfa3a  pe-notepad.exe
fd5c46d73d29ba21b04c844bbaf9096066136526911230645a2a040d23fb612b  pe-python.exe
ade557dd65848c5cf6565913cf6e01cf5c9a8033f0d784c4d6932394958d743e  pe-where.exe
d1d050efbae74c970ba6e666de004405b21f60f34ff0886000026763fe117cf0  pe-winver.exe
```
Plus, by ledger record rather than byte identity: **Sino-US DrugQA V1**
(`cd026a2…`, 15,374,047 B, opened by G3, `V1-COLUMN-ADVERSE`) and
**GH-Archive-10 MiB** (`59d1d008…`, G0's original V1).

---

## 7. Checkout cleanliness — remote vs local (coordinator ruling encoded)

`lockout/checkout-cleanliness.json`. **The rule:** cleanliness is a property of
the **remote measurement checkout at a pinned commit**, never of the
coordinator's intentionally dirty research worktree (MASTER-BRIEF §8/§16).

| Object | Value |
|---|---|
| `measurement_checkout.pinned_commit` | `<HEAD 40-hex>` |
| `measurement_checkout.expected_dirty` | `false` |
| must verify | `git rev-parse HEAD == pinned`; `git status --porcelain == empty`; `git diff --stat == empty`; `git stash list == empty` |
| failure class | **`INVALID_INFRA`**, never a scientific loss |
| `coordinator_worktree.authoritative` | **`false`** |
| `coordinator_worktree.expected_dirty` | `true` |

`project.source_dirty` in the lock stays `false` **and means the pinned
measurement checkout**. Local swarm artifacts — including this document, the
prototypes, and `scratch/` — are **not** part of the measured source and must
never influence a clean-checkout assertion. This implements queue edit **E8**
(`source_tree_ref` + `evaluated_at` + scoped `source_dirty`).

---

## 8. Provenance and measurement windows (never spliced)

| Window | Host | Used for |
|---|---|---|
| **W0** | this Windows host, control plane only; **no codec, no corpus bytes, no network** | all `[M]` here; `seal.py selftest` 64/64; `lock_emit.py emit`; every reconciliation row |
| W1 | `tests/corpus` on disk, 2026-08-13/08-21 manifest | [R] manifest identity |
| W2 | `RESEARCH_LEDGER.md` I7/I10 entries | [R] V1/PE consumption, budgets |
| W3 | GitHub Actions runs (I10 reconciliation) | [R] G3/G5A/G5B rulings |
| **W4** | **Linux EPYC v2 history** | **UNUSED — no claim in this document** |

**No Windows and no Linux number is combined anywhere above.** The corpus
byte counts (21,127,883 B) are file-system facts, host-independent; the audit
timings are W0-only and are labelled as prototype-scale, not portfolio-scale.

---

## 9. What byte-only work is still legitimate before new corpora exist

Admissible now, **without** any held-out byte, because it either needs no corpus
or only uses data as a *control*:

1. **Admissibility work itself (Q1a/Q1b).** The lock, the audit, the tombstone,
   the schema amendment, the ledger. Highest information gain per CPU-second in
   the entire program, and it can legitimately return a decisive negative about
   the *evidence base* rather than about a codec.
2. **Arithmetic and proof (Q0b, Q0c).** Paper-only. `Q0c`'s λ·c publication is
   zero-cost and currently blocks the whole Tier-1 selector lane.
3. **Static/byte-only mechanism falsification** (Q3 Stage 1, Q5, Q9, Q4a). Valid
   as *engineering* results, provided every row carries `evidence_role` and
   every aggregate is emitted role-stratified. These can kill mechanisms; they
   cannot license them.
4. **Correctness, malformed-input, fuzz, and security** (track 19). The one
   track fully served by current data — synthetic families are *ideal* here.
5. **Sensitivity/oracle calibration on S2 synthetics.** Legitimate and valuable:
   measuring whether a detector fires at a known injected SNR is exactly what
   synthetic data is good for. It yields **sensitivity**, never **generalization**.
6. **Anatomy and negative controls** on `known_stress`, provided every resulting
   number is labelled `discovery-evidence` and never quoted as a frontier point.

**Not legitimate, ever, on current data:** any Pareto crossing, any
generalization claim, any "beats Brotli on a new family" statement, any
promotion, and any ratio aggregate not accompanied by
`aggregate_heldout_only: null` + `promotion_authorized: false`.

---

## 10. Queue review — evidence class and ceiling per job

Classes used: **OPEN-retained** (already-consumed/retained artifacts, discovery
role) · **SYN** (synthetic control) · **KS** (known stress) · **ANCHOR**
(external anchor) · **SEALED** (real held-out — currently empty).

Ceiling: **DISCOVER** (may only discover) · **PILOT** (may validate a frozen
mechanism on discovery data) · **PROMOTE** (may produce promotion language).
**No job currently reaches PROMOTE**, because `heldout_units = 0`.

| Job | Legal evidence class **now** | Ceiling | Admissibility flag |
|---|---|---|---|
| **Q0** dense-frontier correction | OPEN-retained + ANCHOR | DISCOVER | Sound. Must emit `aggregate_heldout_only: null` + `promotion_authorized: false`. |
| **Q0b** safe-search theorem | none needed (paper) | DISCOVER | Clean. No corpus. |
| **Q0c** λ·c calibration *(missing, NQ-1)* | none needed (paper) | DISCOVER | Clean, and **blocks Q3**. Adopt now. |
| **Q1a** deterministic artifact audit | OPEN-retained bytes; **metadata-only for sealed**; **zero archive decompression** | DISCOVER | Clean and decisive. This is the job I recommend first. |
| **Q1b** CI gate (blind 2nd auditor, fault injection) | same + cross-runner | DISCOVER | Clean. |
| **Q2** block-window parity pilot | OPEN-retained + ANCHOR | PILOT | Block-size finding is **configuration**, not a mechanism (NQ-8). |
| **Q3** selector arbiter | OPEN-retained | PILOT | **Adopt-class.** Blocked by Q0c. `lambda=0.01` uninterpretable until calibrated. Bars novelty/wire/frontier language (NQ-2). |
| **Q4a/Q4b** BWT leverage | OPEN-retained + ANCHOR | PILOT | `{adopt-class}`/`{engineering}`. Amdahl ceiling applies literally to the *implementation*. |
| **Q5** MASK-CEILING | SYN + OPEN-retained | DISCOVER | ⚠ **Flag: intended conclusion exceeds admissibility.** An "ideal replacement ceiling" measured on single-lineage synthetic data reads as headroom. Must emit `mask_uniqueness_p50`, `mask_modal_accuracy`, `ceiling_basis_role: synthetic_control`, and `authorizes_prototype: false` unconditionally (NQ-3). |
| **Q6** G5D census ablation | OPEN-retained + SYN | DISCOVER | `{engineering}`, `novelty_claim_authorized: false` (NQ-5). |
| **Q7** DEFLATE reconstruction | **none admissible** | **DISCOVER only** | ⚠ **Flag: hardest blocker.** There is **no real precompressed family in existence here** — not a held-out one, not a control one. §4.9 has no `source.kind` for a precompressed artifact. Job cannot start. |
| **Q8** declared-total amplification | SYN | DISCOVER | Clean; security infrastructure. |
| **Q9** aux-index W1024 vs W16 | OPEN-retained | PILOT | `{adopt-class}`; small absolute prize; byte/correctness first. |
| **B1** numeric dual-bar | **BLOCKED — SEALED required** | none | ⚠ Correctly blocked. Zero admissible numeric families; prototype may not start. |
| **B2** planner-routing preflight | OPEN-retained | DISCOVER | Correctly held. Do not fund two overlapping pilots. |
| **B3** POOL-1 / selector census | OPEN-retained, byte-only | DISCOVER | Correctly held post-H0. |
| **B4** CAM model prior | n/a | none | Novelty closed; the NQ-7 vacuity finding (static-table rANS: the transmitted model **is** the prior; rANS already carries coder state across blocks free) means no third trigger should exist. |

### Jobs whose intended conclusion exceeds current corpus admissibility

1. **Q5** — a positive *ceiling* is not headroom (NQ-3). Highest laundering risk.
2. **Q7** — needs a real precompressed family that does not exist and cannot be
   expressed in the schema. Effectively unfundable today.
3. **Q2** — any framing of its block-size finding as adaptive blocking is
   occupied prior art (NQ-8).
4. **Q3** — comparing two objectives under an uncalibrated shared constant makes
   the A/B *uninterpretable*, not merely imprecise (QP-NOVELTY-2).
5. **B1** — correct to block; recording so no one reopens it "just for diagnostics"
   on synthetic numeric data, which would produce exactly the kind of number that
   gets quoted.

### Queue-wide gates I endorse and the scaffold supports mechanically

- **QP-NO-PROMOTE** — enforceable now via `blocking-gates.json.any_entry_promotable: false`.
- **QP-CONSUMED** — enforceable now via §6 tombstone + SHA-256 comparison.
- **E1 `evidence_role` per row; "citation-grade" banned for non-heldout.**
- **E2 dual aggregate** (`aggregate_all_roles` **and** `aggregate_heldout_only`).
- **E7 zero sealed-byte exposure** — Q1a must assert *zero archive
  decompression*; computing an AITDCC I-P member hash "to be thorough" would burn
  the project's only unopened real family. This is the single most dangerous
  implementation trap in the whole queue.
- **E8** — implemented in §7.

---

## 11. Lock/audit scaffold reference

`prototypes/swarm-2026-10-02/03-heldout-corpus/space-bunny/`

| File | Purpose |
|---|---|
| `seal.py` | prospective seal (byte-free, order-binding Merkle truth root); screened/decided independence audit; chained single-shot burn ledger with committed head; evidence root; synthetic grade lattice. `python seal.py selftest` → **64/64 PASS**. Stdlib only. |
| `lock_emit.py` | `emit` / `validate`. Read-only over `tests/corpus` + git index. Writes only under `lockout/`. |
| `lockout/corpus-lock.json` (54,204 B) + `corpus-lock.sha256` | the lock; `lock_id 37c0476…` |
| `lockout/dedup-report.json` | 276 pairs compared, 156 blocking, `waiver_path_present: false` |
| `lockout/publisher-split-report.json` | publisher/release/schema clusters; `isolation_ok: false` with the §5.2 reason |
| `lockout/license-report.json` | per-entry status; 6 unresolved |
| `lockout/schema-validation.json` | 6 violations, all the `source.kind` gap |
| `lockout/independence-units.json` | 24 files → 7 units → **0 admissible**, **0 held-out** |
| `lockout/prospective-seal.json` | `truth_root b6d2c0…`, computable with zero corpus bytes |
| `lockout/checkout-cleanliness.json` | §7 remote-vs-local distinction |
| `lockout/synthetic-grade.json` | grade lattice + admissible uses |
| `lockout/blocking-gates.json` | per-family and per-track blockers |
| `lockout/acquisition-checklist.json` | §13 candidate requirements, **no candidate URLs** (none verified by me; verification is a network-enabled control-plane step) |

Cost of the whole scaffold: **[M]** W0, stdlib only, no network, no codec.
`seal.py selftest` 326 ms at 64 objects/2,016 pairs. Projected for a real
60-entry / 120 MB portfolio: verifier <1 s CPU, **peak RSS ≈ 75–90 MiB**
(4 KiB block index 246 KB + unigram screens ~10 MB + 64-bit decide arrays for one
pair ≈ 40 MB + interpreter), lock document ~84 KB. Ceiling pre-registered as
**≤120 s and ≤256 MiB** — deliberately *not* tightened after measuring.

Decoder-visible cost of this entire track: **0 bytes on the wire, 0 codec state,
0 decoder cycles.** By construction — nothing here is framed, tabulated or
selected by the codec.

---

## 12. Recommendation

**PILOT**, unchanged, with the scope now mechanically demonstrated rather than
asserted.

- **Achieved this pass:** protocol made executable; blockers exposed
  mechanically; 15/17 critic claims conceded with 1 refutation; two real defects
  in my own design found and fixed; `heldout_units = 0` proven, not estimated.
- **Not claimed:** any novelty (this is adopt-class scientific infrastructure —
  the project's own precedent, `RESEARCH_LEDGER.md` Experiment J, downgraded
  context clustering the same way); any promotion; any portfolio existence.
- **Blocking dependency that no amount of engineering removes:** the §13
  portfolio. Until it exists, the correct global posture is *HOLD on all
  promotion language*, which matches both the critic's uncomfortable clause and
  `tests/corpus/README.md:3`, which already describes the corpus honestly.
- **Next single highest-value action:** acquire **one** real, independently
  published structured JSON/NDJSON family and **one** real numeric/telemetry
  stream, per `acquisition-checklist.json`. One real family each converts two
  tracks (13 and 14/09) from `✗` to `~`, and no code change does that.
- **Kill trigger for this track:** if a provenance audit shows §13 is
  unsatisfiable with license-compatible sources, formally abandon the *broad
  general-purpose held-out claim* in the ledger rather than quietly narrowing it,
  and retain the scaffold as narrow infrastructure.