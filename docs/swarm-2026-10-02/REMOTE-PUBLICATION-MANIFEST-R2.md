# Project Anvil — R2 Remote Publication Manifest

**Date:** 2026-10-02  
**Authority:** operational handoff for `ANVIL-CLOSEOUT-2026-10-02-R2`  
**Purpose:** make the next authorized Git publication/Actions phase mechanical and auditable.  
**This document does not authorize commit, push, merge, or dispatch by itself.**

---

## 1. Publication rule

The local research worktree is intentionally dirty and the swarm brief forbids this coordinator from
committing or pushing it. The publication actor must construct a **clean publication commit** containing
only the approved R2 bundle and workflow installs.

Never stage the whole dirty worktree.

For every file below:

1. copy/add exactly the intended bytes;
2. verify the Git blob ID after staging;
3. verify the working-file SHA-256 where listed;
4. if any byte changes, recompute every dependent preregistration anchor before dispatch;
5. do not preserve a stale expected hash merely to make a workflow green.

---

## 2. Frozen control-plane authority

| path | current SHA-256 | prospective Git blob |
|---|---|---|
| `docs/swarm-2026-10-02/FROZEN-CLOSEOUT-MATRIX-R2.md` | `70f7d47268f15c15d1651d5e4aab94ae7e49a2f8e16bdcbde9a916dd01d42941` | `29d08ddae0d4768ef855a20adb8206c6d356cf4a` |

Execution status comes from R2, not from the earlier R1/draft queue/40-agent brief where they differ.

---

## 3. Q1a / XRUN publication bundle

### 3.1 Source files

| source path | SHA-256 | prospective Git blob |
|---|---|---|
| `prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/q1a_corpus_lock.py` | `c0c027e15f2a572d2816396129a1d0ed950bdd86dbf84d3c3d1e6310ccb399a4` | `0d2467c719daef37d301fa221fb8b4b3d069934b` |
| `prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/q1a_xrun_fixture.py` | `255ec7d56633bc4fa970024d446586cdd18afc3d8d7d5930257c85dc953dc3fb` | `07720d4fbba9e4f77143e873fb70d8a05154da43` |
| `prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/anvil-q1a-xrun.DRAFT.yml` | `131cef5e4339d0bbe6509bbbe5bd163d09b43891661bf0fe9ea4f53a9658d74f` | `433c2ad05684fdf031ba38e0fe2d91453ae2e9d0` |

### 3.2 Workflow install mapping

Install the **exact workflow content** from:

`prototypes/swarm-2026-10-02/q1a-corpus-lock/space-bunny/anvil-q1a-xrun.DRAFT.yml`

to an approved tracked GitHub Actions path, recommended:

`.github/workflows/anvil-q1a-xrun.yml`

The destination path changes no blob identity because Git blobs are path-independent.

### 3.3 Frozen XRUN anchors

Current preregistered anchors:

- fixture lock SHA-256:
  `0c4090c84d9de954cbd1bbf3b9b239f4c31413f596c87ef6c55eae62575ce17f`
- artifact manifest SHA-256:
  `feaa7848967d8245dc16f5d531fcc1e5c9f2898e941b6f5a424182301c4fec5c`
- summary SHA-256:
  `112add224d0017e6bbb7fad083424d90c3818526416de546f3000e00216dcdaa`

Expected XRUN semantic result is deliberately:

- `CORPUS_BLOCKED`
- `audit_complete=true`
- `graph_verification=attestation_backed`
- promotion-authoritative independence = false
- 7 fixture units / 30 edges / 113 not-evaluated
- zero network / codec / archive decompression / sealed bytes.

**XRUN PASS is a reproducibility result, not a corpus-promotion result.**

### 3.4 Publication-time rebinding rule

If either the Q1a tool or fixture-emitter bytes change while publishing:

1. update its SHA-256 and Git blob assertion in the workflow;
2. regenerate the synthetic artifact set twice;
3. confirm both emissions are byte-identical;
4. freeze the new artifact-manifest and summary SHA-256;
5. update R2/publication manifest before dispatch.

---

## 4. Q2 Rev-8 publication bundle

### 4.1 Source identities

| path | SHA-256 | prospective Git blob |
|---|---|---|
| `prototypes/swarm-2026-10-02/q2-parity/q2_arms.py` | `f527ae7206327386918a05bb0c494edb5e6291c67bd5e2432d536241403f8bf8` | `d6a090731c4eb2495487771a1e3c5fe581c436e5` |
| `prototypes/swarm-2026-10-02/q2-parity/q2_factorial.py` | `841f861d96b03de894654d47425351cf6f3e6fbf944cdcd01a7d4ffede898f07` | `2d2a73062b80a337f79143e766c7936ba6fb5777` |
| `prototypes/swarm-2026-10-02/q2-parity/q2_collect.py` | `b9b1d2e93dc61eebcbe8066a12d7838c450cc72654f1900ea4f692d7c0e81dc7` | `2a0b58da023bf3b84f9e76b756e0bcd75bfa6b7f` |
| `prototypes/swarm-2026-10-02/q2-parity/q2_brotli_geom.cpp` | `6ff05fdf8ca5c3fe14997505edc2a5affa4b6a73cccf22d541c6960c41804cb1` | `65e05514415afb663b43bf9a9cafa35c7780d68a` |
| `prototypes/swarm-2026-10-02/q2-parity/anvil-q2-parity.DRAFT.yml` | `1fadf1bff755f434441a48de88d0d3d3583ab2d4815dd62734c2f6b6197c6503` | `8fd9c0d3a94ef3be63f2bfa8f46febd67be3eb18` |
| `prototypes/swarm-2026-10-02/q2-parity/README.md` | `d56b05949e34b4079bb6dcb583c04db668b85380d52c3c02e927fa9ff077179b` | `4c5a50fbaac54da31de91116cc94cd4d9a165b07` |
| `docs/swarm-2026-10-02/Q2-PARITY-PLAN-SPACE-BUNNY.md` | `247f41f177fdd27b1d4a72650a073fe3d710968074559f69d7f29eb0abee3dc3` | `db0a17dae25557492c772bdfd1427411d64bad8c` |

### 4.2 Workflow install mapping

Install byte-identical content from:

`prototypes/swarm-2026-10-02/q2-parity/anvil-q2-parity.DRAFT.yml`

to recommended tracked path:

`.github/workflows/anvil-q2-parity.yml`

The workflow is already:

- `workflow_dispatch` only;
- `contents: read`;
- secret-free;
- commit-pinned for checkout and upload-artifact;
- source/blob fail-closed;
- G-S-before-geometry;
- frontier-vocabulary clean;
- non-timing;
- role-stratified;
- pinned to Brotli v1.1.0 commit
  `ed738e842d2fbdf2d6459e39267a633c4a9b2f5d`.

### 4.3 Q2 runtime stop rules

Do not interpret or continue Q2 if:

- any measurement-critical source/evidence blob differs;
- `ANVIL_STREAM_LAMBDA` is set;
- G-S tracked-stratum G0 bytes differ from frozen per-file candidate bytes;
- pinned Brotli producer HEAD differs;
- roundtrip byte comparison fails;
- forbidden ratio/BWT arms enter the factorial;
- any frontier class leaks into emitted artifacts.

G-S failure is **BASELINE-VOID**, not a codec result.

---

## 5. Docs recommended in the publication commit

At minimum publish:

- `FROZEN-CLOSEOUT-MATRIX-R2.md`
- this publication manifest;
- Q1a constructive + critic closeouts;
- Q2 Rev-8 plan + adjudication;
- QBYTES ceiling/ruling;
- GHA substrate audit;
- `COORDINATOR-STATE.md` after R2 delta is appended.

Historical swarm reports may remain broader, but their execution authority is superseded by R2.

---

## 6. Remote repository state verified before this manifest

Connected GitHub inspection established:

- repository: `thelabcorner/anvil`
- default branch: `main`
- branch: `i10-aux-unbwt` exists
- at inspection time, the Q1a tool, Q2 workflow, and frozen closeout artifacts were **not present**
  on remote `i10-aux-unbwt`.

Therefore the next blocker is concrete publication/tracking, not local absence of a configured `git remote`.

A project-level `LICENSE` file is not a measurement-validity prerequisite. Corpus-level provenance
and license evidence remain a Gate-B requirement.

---

## 7. First authorized remote sequence after publication

1. Verify committed blob IDs against this manifest.
2. If publication changed Q1a bytes, rebind XRUN anchors before dispatch.
3. Dispatch **Q1a-XRUN**.
4. XRUN source/artifact mismatch ⇒ STOP remote program and repair publication.
5. XRUN PASS ⇒ Gate A (source/CI trust) is open.
6. Then:
   - Q8 may run;
   - Q2 Rev 8 may run with G-S first;
   - Q7/Q4 remain conditional on their own preregistered controls;
   - c9/new-family promotion work proceeds independently in parallel.
7. No promotion/generalization/novelty claim until Gate B separately passes.

