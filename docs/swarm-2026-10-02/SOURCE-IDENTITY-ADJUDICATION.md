# SOURCE-IDENTITY-ADJUDICATION — what must be published before any citation-grade Actions job

**Date:** 2026-10-02
**Type:** Read-only reconciliation. Additive.
**Authority: none.** This document authorizes no commit, no branch, no tag, no remote, no push,
no workflow edit, and no dispatch. It records findings and produces a prerequisite checklist.

## 0. Non-actions (deliberate, exhaustive)

No commit, no branch, no tag, no stash, no reset, no checkout, no clean, no fetch, no remote
configuration. No network call. No workflow dispatched or edited. No `.github/workflows/*`
file and no coordinator-owned file (`COORDINATOR-*.md`, `MASTER-BRIEF.md`,
`REMOTE-EXPERIMENT-QUEUE.md`, `CLOSEOUT-MATRIX-REDTEAM.md`) modified. No corpus byte
compressed, decompressed, hashed for measurement, or benchmarked; no timer, no build, no codec
invocation. Every hash below is a Git object id or a size/mtime read of a file already on disk.

**The only file written is this one.**

Operations performed were all reads: `rev-parse`, `log`, `cat-file`, `ls-tree`, `ls-files`,
`status --porcelain`, `check-ignore`, `hash-object`, `merge-base --is-ancestor`, `fsck
--unreachable`, `diff --name-status`, `diff --stat`, plus filesystem enumeration and
`Get-FileHash`.

---

## 1. The central finding: the two lineages share no ancestor

This is not drift. It is **two disjoint histories** in one object store.

```
local HEAD          b8eae11fa353bb5e3c88e8e757fe3ad41e4bbd24   branch i10-aux-unbwt   176 commits
  root              050135df30329861379144abd3f4cb32a6a7b7cc   "Import ANVIL v0.1 prototype"
connected main      b6a243c9657776058a37e5f0ee2eaae2ff925dff   (FETCH_HEAD, 2026-09-24)
  root              5b6122d5eebb58d7833cbec978b6ad36fc7fe666   "Initial public ANVIL research snapshot"   90 commits

git merge-base b8eae11 b6a243c   ->  exit 1, no output      NO COMMON ANCESTOR
git rev-list --max-parents=0     ->  two distinct roots
subject-line intersection        ->  1 commit subject ("fix: use portable CPUID path for clang on Linux"),
                                     and it exists twice as two separate commits:
                                     770eeb4 (local) and 95ce16a (connected), 5 seconds apart,
                                     same author, identical tree, different parents.
```

`FETCH_HEAD` is the **only** thing naming `b6a243`. There is no remote-tracking ref, no branch,
no tag. `git fsck --unreachable` reports `b6a243c…` as an **unreachable commit**;
`git rev-list --all` does not contain it. The connected history is one `git gc --prune` away
from being unrecoverable from this machine.

Consequence for every plan in this swarm: **"push local HEAD" is not a fast-forward.** Any
topology that assumes a linear progression from `b8eae11` to `b6a243`, or that counts "drift
commits" between them, is arithmetically void. `Q2-ADJUDICATION.md:151-159` and
`Q2-PARITY-PLAN-CRITIC.md:536-543` both reason over a commit-distance model that does not exist
here.

### 1.1 What the lineages agree on

| path | local HEAD | connected main | verdict |
|---|---|---|---|
| `src/` tree | `3ebadf9d6f07434269e2de965d6573845750dca7` | `3ebadf9d6f07434269e2de965d6573845750dca7` | **identical** |
| `src/anvil.cpp` blob | `755df76ae53795aea040f4c99784d1effb43b3ab` | `755df76ae53795aea040f4c99784d1effb43a` | **identical** |
| `tools/paired_bench.py` | `f847c50e38d3b39b61deeb6b21bcf12e9c132fd5` | `f847c50e38d3b39b61deeb6b21bcf12e9c132fd5` | **identical** |
| `tools/gha_fetch_corpus.py` | `c8f5cc71f9ade44db3b975d10aad39e901e26ebc` | same | identical |
| `tools/bench_native.cpp` | `4f6ceb5a103948f60d3bc73f7c9ac695f36f3232` | same | identical |
| `tools/brotli_lw.cpp` | `c9a55893b827f6f45e6855a79514a91e0787346d` | same | identical |
| `CMakeLists.txt` | `449ae00e0dd56cac5584e08fa725288870573667` | same | identical |
| `tools/` tree | `f765d19c29781e09fabe9e5295c689bece0e6a0c` | `d264d9b65134685b7b1528cfa8e3a78f776e8505` | **differs** |
| `tests/` tree | `69f30c0c5b3c872ca164bcbea9c63f6d5fcb5077` | `578d28ca09b3ee6f0a8e06b39b0f9ed6ba6a0cdf` | **differs** |
| `docs/` tree | `09251ba191b938c43691921da44e9873a88732a3` | `c4e58d2b138e88b327c96e26f4e11a718a28f8d7` | **differs** |
| `prototypes/` tree | `b17b4e8dab0ebe631cd08333aee771fddf50897a` | `ca38e3c544fc7a9be3c711d51252c2989fb9c01a` | **differs** |
| `.opencode/` | 3 tracked files | absent | **local-only** |

**The codec subject source is byte-identical across the split.** That is the single most
useful fact in this document: the subject-source identity does not need reconciling, because
it was never forked. What forked is the *tooling, evidence, and documentation* around it.

---

## 2. Subject-source identity vs control-plane/audit-tool identity

These are two different objects with two different admissibility rules. Conflating them is
the root cause of most of the swarm's source-identity confusion.

### 2.1 Subject source — what a scientific claim is about

`src/anvil.cpp`, `CMakeLists.txt`, `FORMAT.md`, the frozen tool blobs (`tools/bench_native.cpp`,
`tools/brotli_lw.cpp`, `tools/paired_bench.py`, `tools/gha_fetch_corpus.py`, `tools/bench_ratio.py`),
the frozen corpus, and the frozen result CSVs.

- Identity form: Git tree id + per-path blob SHA-1 + SHA-256 of the compiled artifact.
- Admissible when: every pinned object is reachable from a **ref that will exist on the remote**,
  and the ref is published.
- Current state: **substrate is already pinned correctly.** `anvil-i10-dense-frontier.yml`
  carries `TOOLING_SHA`, `CANDIDATE_SHA`, `CANDIDATE_SRC_BLOB`, `CMAKE_BLOB`, `BROTLI_LW_BLOB`,
  `TOOLING_PAIRED_BLOB`, `TOOLING_FETCH_BLOB`, `TOOLING_BENCH_BLOB` and asserts each with
  `test "$(git -C … rev-parse …)" = "…"`. All eight blobs resolve to objects that exist locally.
- The blocker for subject source is **not** pinning. It is that two of the pinned values
  (below) resolve to the *wrong lineage*, and the workflow file itself is untracked.

### 2.2 Control plane — agent/session machinery

`.opencode/swarms/swarms.chunkdb`, `-shm`, `-wal`. Three files, **tracked at HEAD**, and
**mutated by every swarm session**:

```
                                    at HEAD b8eae11        dirty in worktree
swarms.chunkdb        60,000 B      c352ed30…              200,000 B
swarms.chunkdb-shm    32,768 B      b5ed44ef…               32,768 B
swarms.chunkdb-wal  4,120,032 B      c7a21266…            4,124,152 B
                     --------      ---------------       -------------
total             4,213,240 B      tracked                all three modified
```

These are the connected-main `.gitignore` excludes (`.opencode/`, `scratch/`). The local
`.gitignore` at HEAD does **not** exclude them, which is why they are tracked at all.

They are **not subject source, not evidence, and not reproducible** — they are a mutable
SQLite-family database whose content is a function of session wall-clock, not of any commit.
A branch cut from local HEAD carries 4.2 MB of operator-session state into the published
repository and into every future `git status` diff. This is the clearest single instance of
"dirty local bytes being smuggled into a source identity" and it is currently *already inside
the committed tree*, not merely in the worktree.

**Ruling: `.opencode/swarms/*` must be untracked and gitignored before any branch is
published.** Not deleted — untracked. Deleting operator session state is not this
adjudication's call.

### 2.3 Audit tool — the corpus-admissibility scaffold

`prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/q1a_corpus_lock.py`. Untracked. It
has had **three distinct identities** in this swarm's own documents within hours:

| source document | bytes | SHA-256 |
|---|---|---|
| `GHA-SUBSTRATE-AUDIT.md:63-72` | 153,894 | `05e9b981b10b5c0a2ad91a336e2c6e8fdfda7232990e35b158a8a59de3ac2b72` |
| `Q1A-CORPUS-ADMISSIBILITY-CRITIC.md` (audited digest) | 185,060 | `a583fc1cb0556778035a046052d7c8de1d9f0113c315ae7bd7bf4ebf6171f288` |
| **actual file right now** | **188,698** | **`0662dd491f04d0fb1d2a78aad653418959ef10f3db52223f6d435a51e58b160f`** |

4,094 lines; mtime `2026-10-03T02:22:03Z`; git blob `21d96058718c5223b03083ba286d497905a3719b`.

Two of these three digests no longer exist on disk. Both prior audits are therefore auditing
objects that cannot be reproduced, and the critic's `127 checks, 0 failed` result is **not
transferable to the current file** without a re-run.

This is the second clearest instance of the same class: an audit tool whose identity is
asserted in prose and never bound to a commit. It is also the tool that the coordinator's
ruling (`GHA-SUBSTRATE-AUDIT.md:22-30`) makes the gate for every remote job.

**Ruling: the scaffold must be committed at a fixed blob before any Q1/Q1a claim is
citation-grade, and every citation must carry the blob id, not a prose digest.**

### 2.4 Coordinator and lane documents — also untracked

`docs/swarm-2026-10-02/` is **65 files, 3,088,203 B, entirely untracked**, including
`COORDINATOR-CLOSEOUT-v1.md`, `COORDINATOR-STATE.md`, `MASTER-BRIEF.md`,
`REMOTE-EXPERIMENT-QUEUE.md`, `CLOSEOUT-MATRIX-REDTEAM.md`, `Q2-ADJUDICATION.md`, and this
file. On the connected lineage the path does not exist at all (`git ls-tree b6a243c --
docs/swarm-2026-10-02` returns 0 entries).

The dispatch authority (`COORDINATOR-CLOSEOUT-v1.md:153`) is itself an unversioned local
file. That is acceptable as *policy* but not as *pre-registered preregistration*: a
preregistration that can be edited without leaving a diff is not a preregistration.

---

## 3. Dirty-tree inventory: 8 modified, 127 untracked entries

```
 M .gitignore                                     <- control plane / policy
 M .opencode/swarms/swarms.chunkdb                <- control plane, binary, 60 KB -> 200 KB
 M .opencode/swarms/swarms.chunkdb-shm            <- control plane, binary
 M .opencode/swarms/swarms.chunkdb-wal            <- control plane, binary, 4.1 MB
 M RESEARCH_LEDGER.md                             <- EVIDENCE, +269 lines
 M docs/I10-AUX-UNBWT-INTEGRATION-PLAN.md         <- evidence doc, +13/-…
 M docs/I10-GROTLI-FRONTIER-ARCHITECTURE.md        <- evidence doc, +73/…
 M tools/paired_bench.py                          <- SUBJECT-SOURCE TOOLING, +188/-…
```

`tools/paired_bench.py` is the only modified file that is subject-source tooling, and it is
**not** the pinned blob:

```
HEAD blob            f847c50e38d3b39b61deeb6b21bcf12e9c132fd5   <- pinned by dense-frontier as TOOLING_PAIRED_BLOB
worktree blob        bb2b2a7efcaa2959800e7ea27e7b5c6f65c1f2cd   <- 188 uncommitted lines
```

If the workflow were dispatched from the worktree instead of the pinned commit, the tool
identity assertion at `anvil-i10-dense-frontier.yml:129` would fail. It cannot be smuggled
past that gate — which is correct — but the uncommitted 188 lines are currently **unowned
work that no remote job can measure**.

Untracked, by class:

| class | entries | bytes on disk | reproducible from clean checkout? |
|---|---:|---:|---|
| `docs/swarm-2026-10-02/` | 1 dir (65 files) | 3,088,203 | no — the bytes exist only here |
| `prototypes/swarm-2026-10-02/` | 1 dir (75 files) | 1,906,975 | no — ditto |
| `.github/workflows/` (2 new) | 2 | — | no — **untracked ⇒ absent from any checkout** |
| `docs/I10-*-PREREG.md` (9) | 9 | — | no |
| `.github` workflows: `anvil-i10-dense-frontier.yml`, `anvil-i10-g5-paged-dictionary.yml` | 2 | — | **no — and this is the dispatch blocker** |
| `entropy-mix-wt/` | 1 dir | 69,978,017 | **must never be published — it is a registered linked worktree of this same repo** |
| `scratch/`, `build-*/` | 8 dirs | ~5.5 GB | no — build/scratch products |
| binary build products under `prototypes/` | 86 (`.exe`/`.obj`/`.pdb`/`.dll`/`.bin`) | — | no |
| `tests/benchmark-*.csv` (3 new), `tests/pareto-*.head-*.csv` (2) | 5 | — | no |
| `Clear-OrphanedPyInstallerTemp.ps1` | 1 | — | no |

`git status --porcelain` reports `entropy-mix-wt/` as a plain untracked directory. It is
registered in `.git/worktrees` (`git worktree list` shows it at `7b999c6`). Adding it would
nest a repository inside the published tree. Hard stop.

---

## 4. Untracked corpus objects and build products: reproducible or not

The frozen 13-file fidelity control set is exactly the first column of
`tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` (13 unique paths, confirmed). It splits
cleanly:

```
tests\corpus\anvil_bench.exe      <- NOT reproducible (gitignored build product)
tests\corpus\anvil.exe            <- NOT reproducible (gitignored build product)
tests\corpus\doc.md               <- tracked
tests\corpus\generated.json       <- tracked
tests\corpus\generated.jsonl      <- tracked
tests\corpus\generated.log        <- tracked
tests\corpus\generated.repeat.jsonl<- tracked
tests\corpus\generated.sqlite     <- tracked
tests\corpus\random.bin           <- tracked
tests\corpus\src.cpp              <- tracked
tests\corpus\synth-arith.bin      <- tracked
tests\corpus\synth-jitter.bin     <- tracked
tests\corpus\synth-timeseries.bin <- tracked
```

### 4.1 Cannot be reproduced from a clean checkout — and never will be

| object | bytes | sha256 | why |
|---|---:|---|---|
| `tests/corpus/anvil.exe` | 268,800 | `09b9b0cc15d96c2b0c4cb371a69b61e617de95d28c00c115b4bd9a252c45fe4c` | gitignored build product (`.gitignore:8`). No history — `tests/corpus/CHECKSUMS.txt` records the prior bytes as **unrecoverable** |
| `tests/corpus/anvil_bench.exe` | 1,929,216 | `fcd30da5f2745df439aaedcb3d84ec71451e0974a5c0f84da814a73dd9ef4ce0` | same; manifest carries an explicit DRIFT NOTE that the prior hash's bytes are lost |

These are **2 of the 13** control files. A clean pinned checkout can reconstruct 11 of 13. The
byte-equality fidelity gate that `Q2-ADJUDICATION.md:299` (P4) and the coordinator's ruling
require to fire *first* is therefore **structurally unevaluable on 2 of its 13 rows** without a
reproducible-build attestation. This is Q2 P3/P5, and it is confirmed rather than inherited:
`git check-ignore -v tests/corpus/anvil.exe` → `.gitignore:8:anvil.exe`.

Note the direction of the hazard: the manifest was already re-pinned once, to make the file
truthful, after the older bytes were lost. That is a one-way ratchet. Any further toolchain
change silently invalidates the retained Class-A rows with no way back.

### 4.2 Reproducible in principle, from a tracked generator — needs an attestation run

| object | bytes | sha256 | generator |
|---|---:|---|---|
| `tests/corpus/synth-columnar-align.bin` | 276,000 | `a42ea6b4bbba64720030646933199c8f35ef8145fcb8ab1995978b4361f0263b` | `tests/make_synth_corpus.py` (tracked), `random.Random(90004)` |
| `tests/corpus/synth-drift-stride.bin` | 615,376 | `ea580de5ea28f8bd3de9c7faa523c05914c7a41ca55b0482cbf9920baf6ba85f` | same, seed 90005 |
| `tests/corpus/synth-telemetry-f64.bin` | 320,000 | `ede91d55233736ab89bbc453f2a14dad95bd3971aa0ad0d570e37410179eec8a` | same, seed 90007 |

All three digests match `tests/corpus/CHECKSUMS.txt` exactly, and the generator is tracked and
declares fixed seeds and byte-for-byte determinism (`make_synth_corpus.py:13`). These are
**regenerable, not irrecoverable** — they should be *proved* regenerable by a CI-side
regeneration-and-compare step, not shipped as bytes. Shipping them as bytes would be correct
too, but the byte-add must be a deliberate decision, not a side effect of `git add`.

Also untracked but not part of the frozen 13: the six harvested Windows binaries
`pe-git.exe` (4,383,048 B), `pe-ninja.exe` (603,648), `pe-notepad.exe` (360,448),
`pe-python.exe` (103,704), `pe-where.exe` (61,440), `pe-winver.exe` (28,672). All six digests
match the manifest. **None is reproducible from any repository** — they are host system files,
and their only provenance is the manifest assertion. They are also the family under
Lineage scrutiny in `Q1A-CORPUS-ADMISSIBILITY-CRITIC.md` §B-1. Publishing them raises a
redistribution question that is out of scope here and must not be resolved by default.

### 4.3 Untracked, no generator, no provenance

| object | note |
|---|---|
| `tests/bwt-golden/g-tiny.bin` (5 B), `g-text.bin` (90,000), `g-random.bin` (20,000), `g-repetitive.bin` (20,000) | no generator anywhere in `tests/`; only the derived `.anv`/`.out` are ignored. Irreproducible |
| `prototypes/i9-pnra/var_00..09.bin`, `var_out*.bin`, `var_jitter2.bin`, `var_seamless.bin` | measurement scratch |
| `prototypes/i9-deflate/results/*.bin` (12) | measurement scratch |
| `prototypes/i8-pnra/text_*.bin` (6), `prototypes/i8-datastruct/tmp_*.bin` (2) | derived inputs |
| 86 `.exe`/`.obj`/`.pdb`/`.dll` under `prototypes/` | compiled products, including a vendored `clang_rt.asan_dynamic-x86_64.dll` |

### 4.4 Net

Of the frozen control set: **11 tracked, 2 permanently irreproducible, 0 newly obtainable.**
The fidelity gate has no path to full coverage without either a reproducible-build attestation
for `anvil.exe`/`anvil_bench.exe` or a formally declared 11-row scope. Declaring the scope
after seeing which rows fail is the failure mode to avoid; the declaration must precede the
run.

---

## 5. Publishability: five failures, all confirmed

| # | failure | check performed | result |
|---|---|---|---|
| **P1** | No remote configured | `git remote -v`; `.git/remotes/`; `git config --local --list` | empty. No push target, no Actions dispatch destination |
| **P2** | No LICENSE, no CONTRIBUTING, at any root, on **either** lineage | `git ls-tree -r b6a243c \| grep -E '^(LICENSE\|CONTRIBUTING)'`; local `Get-ChildItem LICENSE*` | **0 matches on both** |
| **P3** | The two workflows with complete source-identity assertions are untracked | `git status --porcelain` shows `?? .github/workflows/anvil-i10-dense-frontier.yml`, `?? …-g5-paged-dictionary.yml` | 14 of 16 tracked at HEAD; the 2 strongest are the 2 unpublished |
| **P4** | No clean pinned candidate | `git status --porcelain` → 8 modified + 127 untracked | `COORDINATOR-CLOSEOUT-v1.md:153`: "Existing local dirty state is not authority for remote source identity" |
| **P5** | 2 of 13 frozen control files are gitignored build products | `git check-ignore -v tests/corpus/anvil.exe` | confirmed; §4.1 |

**Effective dispatchable workflow count: 0.** The two that would qualify (`GHA-SUBSTRATE-AUDIT.md:27-30`
independently reached this number) cannot be selected from a checkout that does not contain
them.

### 5.1 A sixth failure the prior audits did not record: the pinned SHAs straddle the split

`anvil-i10-dense-frontier.yml`:

```
TOOLING_SHA:   b6a243c9657776058a37e5f0ee2eaae2ff925dff   <- connected lineage, UNREACHABLE from any local ref
CANDIDATE_TAG: i10-aux-unbwt-final-v1
CANDIDATE_SHA: 0dce534e5df40945d518c9ef4a722f4a0ecfc0e6   <- connected lineage
```

`.github/workflows/anvil-i10-grotli-g5a.yml` (tracked, blob `6b8014d8`, clean in worktree):

```
FROZEN_IMPLEMENTATION_SHA: 0321e41c574b8651fce865cad1d7fc7d6789e35d   <- LOCAL lineage
FROZEN_G3_SHA:             1a3d18fed76adb6fb33264e1994f9c357306b3fa   <- CONNECTED lineage
```

`.github/workflows/anvil-i10-g5-paged-dictionary.yml` (untracked):

```
FROZEN_G2_SHA:  c34b291f40db4037ae114a39dae181644260dfff   <- CONNECTED lineage
FROZEN_G3_SHA:  1a3d18fed76adb6fb33264e1994f9c357306b3fa   <- CONNECTED lineage
```

Verified by `merge-base --is-ancestor`:

| commit | reachable from local `b8eae11` | reachable from connected `b6a243c` |
|---|---|---|
| `0321e41` (g5a r5 impl) | **yes** | **no** |
| `1a3d18f` (G3) | **no** | **yes** |
| `c34b291f` (G2) | **no** | **yes** |
| `0dce534` (aux-unbwt close) | **no** | **yes** |
| `ce964dc` (local `i10-aux-unbwt-final-v1`) | **yes** | **no** |
| `92dc5c1` (local `i10-aux-unbwt-measured-v1`) | **yes** | **no** |
| `770eeb4` (local `i10-baseline-codec-20260923`) | **yes** | **no** |

So the G5A workflow **cannot run from its own lineage**: it pins a G3 commit that exists only
on the other side. And `1a3d18f` is presently a locally *dangling* object — reachable from
`FETCH_HEAD` only via the connected history that no ref names.

### 5.2 A tag-name collision that would produce a false citation

```
local tag  i10-aux-unbwt-final-v1  ->  ce964dc5e2f0a6fd6dad3e73ab4b757f006e9be1  (local lineage)
workflow    CANDIDATE_TAG: i10-aux-unbwt-final-v1
           CANDIDATE_SHA: 0dce534e5df40945d518c9ef4a722f4a0ecfc0e6  (connected lineage)
git tag --points-at 0dce534  ->  (empty)
```

The same tag **name** denotes two different commits depending on which object store you ask.
Both carry the subject "docs: close I10-1A aux-unBWT experiment", 10 seconds apart, with the
**same** `src/anvil.cpp` blob but 17 differing paths (`.gitignore`, the three `.opencode`
chunkdb files, three benchmark CSVs, five `.ps1`/`.md` files, two prototypes files).

An evidence artifact that cites `i10-aux-unbwt-final-v1` is therefore **ambiguous by
construction** and cannot be citation-grade until one of the two is renamed or the pair is
reconciled. Emitting the artifact name and the SHA in different lineages is precisely the
"citation-grade" failure this adjudication exists to prevent.

### 5.3 Action-reference pinning

Per `GHA-SUBSTRATE-AUDIT.md:99-100`: 40 floating `@vN` action references across 15 of 16
workflows; commit-pinned in exactly one, `anvil-i10-dense-frontier.yml`
(`actions/checkout@11d5960a…` ×3, `actions/upload-artifact@ea165f8d…`). Confirmed present on
disk. Both untracked workflows use `@v4`; only dense-frontier pins.

---

## 6. Minimal topology that preserves frozen identities without smuggling bytes

Constraints, all established above:

1. No merge between the lineages is admissible. `--allow-unrelated-histories` would fabricate a
   shared descent that never existed, and every commit-distance claim in the Q0–Q2 documents
   would become false rather than true.
2. Subject source is already identical across the split, so **nothing in `src/`, `CMakeLists.txt`,
   `FORMAT.md`, or the frozen tool blobs needs re-publishing for correctness** — only for
   reachability from a published ref.
3. Every SHA a workflow pins must be reachable from a ref that exists on the remote at dispatch
   time. `1a3d18f`, `c34b291f`, `0dce534`, `b6a243c` are currently on **no ref at all**.
4. Nothing that is a function of wall-clock, session state, or host tooling may enter a
   published identity: `.opencode/swarms/*` (4.2 MB tracked and dirty), `scratch/`, `build-*/`,
   the 86 compiled products, and the `entropy-mix-wt/` nested worktree.

Therefore:

```
A. Establish transport.  git remote add origin <github url>; git fetch --no-tags origin.
   This creates refs/remotes/origin/main naming b6a243c, converting the currently-unreachable
   connected lineage into a reachable one. Until this happens every connected-lineage pin in
   §5.1 is a dangling object and nothing can be dispatched.

B. Pin the local side too.  refs/heads/i10-aux-unbwt already names b8eae11 and already makes
   0321e41, ce964dc, 92dc5c1, 770eeb4 reachable. No new ref needed.

C. Publish BOTH lineages as separate branches.  Never merge.  Never rebase one onto the other.
   Minimal:  origin/main  -> b6a243c
             origin/i10-aux-unbwt -> b8eae11
   Each workflow then resolves its own pinned SHAs inside the lineage that contains them, with
   the single exception in §5.1 (G5A's cross-lineage G3 pin), which requires that commit to be
   fetchable by SHA — see stop condition S-4.

D. One new branch, from b8eae11, carrying ONLY an explicitly reviewed allowlist:
     - .gitignore with .opencode/, scratch/, build-*/, entropy-mix-wt/ excluded
     - .github/workflows/anvil-i10-dense-frontier.yml      (as-is; it is already commit-pinned)
     - .github/workflows/anvil-i10-g5-paged-dictionary.yml (as-is)
     - docs/swarm-2026-10-02/**                            (preregistration; version it)
     - prototypes/swarm-2026-10-02/**                      (audit tools; version them)
     - docs/I10-*-PREREG.md                                 (the 9 untracked preregs)
   Everything else stays out: no .exe, no .obj/.pdb/.dll, no var_*.bin, no g-*.bin,
   no pe-*.exe, no CSVs, no .ps1 scratch, no build-*/ tree.

E. Do NOT include, in that branch or any commit:
     - .opencode/swarms/* (tracked today — must be untracked first, by a separate explicit act)
     - entropy-mix-wt/      (nested linked worktree)
     - the 188 uncommitted lines of tools/paired_bench.py  (breaks TOOLING_PAIRED_BLOB)
     - any staged-by-accident binary. 127 untracked entries make `git add -A` catastrophic.

F. Resolve §5.2 before the branch is named in any artifact.  Either re-point
   CANDIDATE_SHA to ce964dc, or rename the local tag, or rename the workflow's tag label.
   Choose one; do not leave the collision.

G. LICENSE absent on both lineages is a publishability failure independent of topology.
   Selecting a license is an owner decision and is not made here.
```

What is deliberately **absent** from the topology: no `.gitignore` blanket that would also
silently drop `docs/swarm-2026-10-02/` (the pending local edit adds `scratch/` and the
`third_party/libsais` exception — it does **not** yet exclude `.opencode/`, `.build-*`,
`__pycache__/`, or `*.exe` generally; the connected lineage's `.gitignore` does). Reconciling
the two `.gitignore` files is a prerequisite, and it is a semantic decision, not a merge.

---

## 7. Prerequisite checklist — exact, ordered, mechanically checkable

Ordered. Each item is a precondition for the next. **No branch is created by this document.**

**Group 0 — preservation (do first; nothing else is safe until these hold)**

| # | prerequisite | check | status |
|---|---|---|---|
| 0.1 | Local Git metadata is backed up before any ref operation | object store + `.git` copy exists off-machine | **NOT DONE — operator act** |
| 0.2 | The currently-unreachable connected lineage is made reachable | `git fsck --unreachable` no longer lists `b6a243c` after 0.3 | **UNMET** — needs 0.3 |
| 0.3 | A remote is configured | `git remote -v` non-empty | **UNMET** (P1) |
| 0.4 | Connected `main` is fetched to a real ref | `git rev-parse refs/remotes/origin/main` = `b6a243c…` | **UNMET** — blocked by 0.3 |
| 0.5 | Both roots are recorded in writing before any publish | `050135d…` and `5b6122d…` recorded as deliberately unrelated | **DONE here** (§1) |

**Group 1 — publishability, no history rewriting**

| # | prerequisite | check | status |
|---|---|---|---|
| 1.1 | A LICENSE exists at the root of the lineage to be published | `git ls-tree -r <ref> \| grep ^LICENSE` | **UNMET** (P2), both lineages — owner decision |
| 1.2 | `.opencode/swarms/*` is untracked and gitignored | `git ls-files .opencode` empty; `git check-ignore .opencode/swarms/swarms.chunkdb` matches | **UNMET** — 3 files, 4,213,240 B tracked and dirty |
| 1.3 | `.gitignore` on the published lineage excludes `.opencode/`, `scratch/`, `build-*/`, `entropy-mix-wt/`, `__pycache__/`, `*.exe`, `*.obj`, `*.pdb`, `*.dll` **and does not exclude** `docs/swarm-2026-10-02/`, `prototypes/swarm-2026-10-02/`, `third_party/libsais/` | `git check-ignore -v` on 10 named probes, each result asserted individually | **PARTIAL** — local edit adds `scratch/` + libsais exception only; connected `.gitignore` is broader and lacks `docs/swarm-…` exclusions |
| 1.4 | `entropy-mix-wt/` is never staged | absent from any `git add`; absent from `git status --porcelain` after staging | **UNMET** — currently shows as untracked dir; nested linked worktree |
| 1.5 | The 2 identity-complete workflows are tracked | `git ls-files .github/workflows` contains both | **UNMET** (P3) |
| 1.6 | The Q1a scaffold is committed at a fixed blob | `git rev-parse HEAD:<path>` exists; artifacts cite the blob, not a prose SHA-256 | **UNMET** — 3 identities recorded, 2 already unreproducible (§2.3) |
| 1.7 | Coordinator + lane documents are versioned | `git ls-tree -r <ref> docs/swarm-2026-10-02` non-empty | **UNMET** — 65 files, entirely untracked |
| 1.8 | The §5.2 tag collision is resolved by name | one tag name → exactly one commit in the published ref set | **UNMET** |
| 1.9 | Frozen control-set scope is declared **before** any run | 13-row scope, or 11-row scope with the 2 exclusions declared pre-run | **UNMET** (P3/P5) |

**Group 2 — subject-source reachability**

| # | prerequisite | check | status |
|---|---|---|---|
| 2.1 | Every SHA pinned by a to-be-dispatched workflow resolves from a published ref | `git merge-base --is-ancestor <pin> <published-ref>` per pin | **UNMET** for `b6a243c`, `0dce534`, `1a3d18f`, `c34b291f` — none on any ref today |
| 2.2 | G5A's cross-lineage G3 pin is made fetchable without a merge | `git fetch origin 1a3d18f…` succeeds, or the workflow's pin is re-pointed | **UNMET** (§5.1) |
| 2.3 | Every pinned blob resolves to an object a fresh clone will possess | `git cat-file -t <blob>` post-clone simulation | **MET** — all 8 dense-frontier blobs resolve locally |
| 2.4 | `source_dirty` is asserted about the pinned checkout, never the operator worktree | `REMOTE-EXPERIMENT-QUEUE.md:316-324` schema | **UNMET** — no pinned checkout exists to assert about |
| 2.5 | Action refs in the dispatched workflow are commit-pinned | no `@vN` in that file | **MET for dense-frontier; UNMET for g5-paged-dictionary** |

**Group 3 — evidence admissibility (independent of Git)**

| # | prerequisite | status |
|---|---|---|
| 3.1 | Fidelity gate defined as per-file byte equality, firing before any geometry is interpreted | **UNMET** — absent from the Q2 plan (R3-2) |
| 3.2 | Fidelity gate is evaluable on its admitted scope | **PARTIAL** — 11/13 rows; 2 need a build attestation |
| 3.3 | Reference identity is per-file exact, Brotli library pinned | **UNMET** (D5) |
| 3.4 | Informative (7) / near-control (2) / strict-control (4) subsets predeclared | **UNMET** (P9) |
| 3.5 | No `FRONT-GAP` / `FRONT-CROSSING` token emitted by any artifact | **UNMET** (R3-5) |

---

## 8. Stop conditions

Any one of these halts work and escalates to the coordinator. None is a judgement call.

| id | condition | consequence |
|---|---|---|
| **S-1** | Anyone creates a merge (or `--allow-unrelated-histories` merge, or a rebase of one lineage onto the other) between `b8eae11` and `b6a243c` | **fatal.** Fabricates shared descent; invalidates every commit-count and drift claim in Q0–Q2. Rewind; re-derive this document |
| **S-2** | Any commit includes `.opencode/swarms/*`, `entropy-mix-wt/`, `scratch/`, `build-*/`, or any of the 86 compiled products | **fatal.** Operator state masquerading as source identity. History rewrite required before publication |
| **S-3** | A `git add -A` / `git add .` is run in the current worktree | **stop.** 127 untracked entries, 4.2 MB of tracked-and-dirty chunkdb, 3.4 KB of tracked corpora-plus 5.5 GB scratch all in reach. Audit the index before continuing |
| **S-4** | G5A is dispatched without `1a3d18f` verified fetchable by SHA from the remote | the job's own assertion `test "$(git rev-parse HEAD)" = "$FROZEN_…"` cannot be satisfied; it fails at setup, or worse, resolves to a different object and produces a false green |
| **S-5** | Any artifact cites `i10-aux-unbwt-final-v1` while the §5.2 collision stands | **false citation.** The name denotes `ce964dc` locally and `0dce534` on the connected lineage |
| **S-6** | Any Q1/Q1a claim cites the `05e9b981…` or `a583fc1c…` scaffold digest | **unreproducible.** Neither matches any file on disk; the 127-check result does not transfer to `0662dd49…` |
| **S-7** | Any fidelity gate reports on fewer rows than were declared pre-run | scope was chosen after seeing results. Null, not a result |
| **S-8** | `anvil.exe` / `anvil_bench.exe` bytes are re-pinned again to make a gate pass | second irreversible ratchet; prior bytes are already unrecoverable per `CHECKSUMS.txt` |
| **S-9** | A license is added, changed, or inferred by a worker | licensing is an owner decision; a worker-selected license propagates to every downstream citation |
| **S-10** | Any job is dispatched before Group 0 **and** Group 1 both read satisfied | `COORDINATOR-CLOSEOUT-v1.md:153` — dirty local state is not authority for remote source identity |

---

## 9. Verdict

> **`HOLD`, on substrate. Not `CANCEL`.**
>
> The science is unaffected. `src/anvil.cpp` is **byte-identical** across the lineage split
> (`755df76a…` on both sides), and `anvil-i10-dense-frontier.yml` already carries a complete,
> commit-pinned, eight-blob source-identity assertion chain — the strongest in the repository.
> That is real and it is preserved.
>
> Four findings are new relative to `Q2-ADJUDICATION.md` §4 and `GHA-SUBSTRATE-AUDIT.md`, and
> three of them change the required work rather than merely confirming it:
>
> 1. **The lineages are unrelated, not drifted.** No merge-base; two roots. Every
>    commit-distance argument in the Q0–Q2 chain is void, and the connected lineage is
>    currently an **unreachable commit** held only by `FETCH_HEAD` — one `git gc --prune` from
>    loss. This is the highest-priority item in the entire document.
> 2. **The two identity-complete workflows pin SHAs from both sides.** G5A's own `FROZEN_G3_SHA`
>    is not reachable from its own lineage. There is no single lineage from which every
>    currently-designed workflow can be dispatched.
> 3. **`i10-aux-unbwt-final-v1` denotes two different commits** in the two lineages, and the
>    dense-frontier workflow pairs that name with the connected-lineage SHA. Citations naming
>    the tag are ambiguous today.
> 4. **4,213,240 B of tracked, session-mutated control-plane database is already inside the
>    committed tree** and must be untracked — not merely left dirty — before publication.
>
> Effective dispatchable job count remains **0**, reached now by a different and shorter route
> than the prior audits took: it is not primarily that the workflows are untracked and the
> controls irreproducible, it is that **the object store contains two histories and neither is
> reachable from a published ref.**
>
> The remedy is small and does not touch any research artifact: configure the remote, fetch
> `main` to a real ref, de-track `.opencode/swarms/*`, publish both lineages as separate
> branches without merging, resolve the tag collision, and declare the control-set scope
> before the run. **No branch is created by this document.**