# GATE I9 — CLAIM-VERIFICATION REGIME (research-gate)

**Author:** `research-gate`. **Date:** 2026-09-12. **Swarm:** `anvil-i9-pareto`.
**Status: BINDING on this lane's rulings and on every number this gate admits to
the record.** Companion to `docs/gate-ruling-i9-frontier-tokens.md`,
`docs/gate-ruling-i9-pr5-position-derived.md`,
`docs/gate-ruling-i9-datastruct-ts.md`, and ledger PART XIV.

The trigger is the project's own failure history (DNB-M4): a number in a
document is a claim, not a fact. Two lanes propagated the unreachable "460/465";
the I8 tally itself was verified against an **uncommitted** artifact. The rule
below is the one that would have caught both.

---

## 1. The recompute rule (artifact + command + label)

Every published number must be:

1. **recomputable from a named artifact**, with the **exact command shown**;
2. **labelled** `measured` | `derived` | `projection` | `prototype wire`;
3. **provenanced**:
   - committed artifact: quote the **git blob id**
     (`git rev-parse <rev>:<path>`), and extract with `git show <rev>:<path>`;
   - uncommitted artifact: quote the **sha256** and write
     **"uncommitted worktree artifact, not admissible as citation of record"**;
   - binary: quote the **executable sha256** (e.g. `de9b4caf…` is a binary
     SHA-256 prefix, **not a git revision**).
4. A number copied from another document is a **claim**, not a fact. If the
   artifact cannot be produced, the number is **struck**, not footnoted.

Throughput additionally obeys `tests/pr-4-measurement-window-protocol.md` v1.1:
tool+binary sha, input sha, UTC window, reps >= 3 (median + CV), host-load
median/max, pinning, **thread counts**, label, and the thread-imbalance rule.
Byte counts and ratios are deterministic and safe; `roundtrip OK` is
deterministic but needs a green binary.

## 2. Green-binary gate (updated 2026-09-12 — PENDING-0 cleared)

The decode defect is fixed and independently verified (roundtrip byte-exact on
`build-i9-format\anvil.exe` sha256 `8EAE1FB3…`, src `62BC6631`, HEAD fc23d9a;
format gate green, zero skips).

**Canonical binaries (current, 2026-09-12, ALLOC-only source):**
`build\anvil.exe` sha256 **`E8AA2E48…`**, `build\anvil_bench.exe`
**`56092B9B…`** (src `38409E26`, HEAD fc23d9a + uncommitted ALLOC-only; gates
green 442/442 roundtrip, fuzz 50 PASS, encode identity 494/494).
The previous canonical pair `0D1E130B` / src `62BC6631` remains valid **for
results measured on it** (e.g. the A7 byte totals). **Compressed-byte results
carry across this move by the 494/494 encode-identity gate** (label the
originating sha); decode/timing results remain sha-specific. `format`
re-verifies before commit.

**Canonical (post-leg-4, 2026-09-12):** frozen src `BDC90474…` (ALLOC + leg 4
retained) -> `build\anvil.exe`
`72D65150CBD65182C69BA85DAD3A8961C25DE48E794FD9823BCAB4ED20A36505`,
`build\anvil_bench.exe` `379341D9…`. Naming: `FE0CF4F1` was the lane EXE sha of
the leg-4 study; the **src** sha is `BDC90474`. `E8AA2E48` / `38409E26` is
superseded for new runs; its byte results carry by the 494/494
encode-wire-identity gate (originating sha labelled). leg-4 gates fully green
on the lane build (roundtrip 442/442, fuzz 50 PASS, wire identity 494/494 same /
0 diff); `format` **independently re-verified** `72D65150`: fuzz PASS, zero
skips, registry identical to both prior freezes -> ALLOC + leg 4 confirmed
**wire-untouched** (`msg_d3930170`).

- Every codec result names the binary sha; citation-grade runs use the canonical
  build. Same-source lane builds carry their own sha + label (bench's current
  window uses `build-i9-arch\anvil_bench.exe` `D6A70ACD`, same src — fine).
- Any move of `src/anvil.cpp` invalidates these shas until re-verified; format
  re-verifies before commit.
- Roundtrip claims still require byte-exact decode on the named binary + fuzz.
- The canonical Silesia `46,446,995 B` / enwik8 `23,534,368 B` byte totals were
  **reproduced on the canonical binary** (`tests/ratio-reverify-i9.csv`, sha256
  `53880D50…`, 13/13 roundtrip OK); the older `de9b4caf` label applies only when
  citing the earlier artifacts.

## 3. Dual bar for {synthetic} transform cells

A {synthetic} cell result must report **both** the raw reference score and the
**transform-enabled** reference score (same reference codec given the same
reversible transform; E1). A row non-dominated on the raw grid but beaten by a
transform-enabled reference row is **FRONT-GAP (dual-bar)**, not a crossing
(I9-1 R-2). Measured bars and commands: `docs/gate-verify-i9-transform-bar.py`
and `docs/gate-ruling-i9-datastruct-ts.md` §4. Every synth-cell number carries
`{synthetic}` with the generator cited.

## 4. Co-listing and grid provenance

Quote the frontier tuple whole, with its grid **and its reference class**:
`(non-dominated | FRONT-GAP | FRONT-CROSSING | DEGENERATE | dominated fraction)`.

| grid | ref class | tuple |
|---|---|---|
| committed HEAD fc23d9a | v1 brotli+zstd | `5 | 5 | 0 | 0 | 411/416` (`443/448`) |
| committed HEAD fc23d9a | **v2 +xz-9e (canonical)** | `4 | 4 | 0 | 0 | 412/416` (`444/448`) |
| worktree `C70179EA` (label) | v1 | `5 | 5 | 0 | 0 | 463/468` (`499/504`) |
| worktree `C70179EA` (label) | v2 +xz-9e | `1 | 1 | 0 | 0 | 467/468` (`503/504`) |

v1 may be cited ONLY as "brotli+zstd reference class (grid v1)"; v2 is the
canonical current citation; worktree always labeled; **GRID-THIN** accompanies
every v2 citation (no zstd 4-22 / brotli lw30 tiers). Never swap grids or
classes without the label; never quote a single figure alone. Verified command:
`python docs\gate-verify-i9.py --suite tests\benchmark-suite.head-fc23d9a.plus-xz.csv`
(add `--refs brotli-,zstd-` for v1, `--transform-controls tests\xz-transform-controls-i9.csv`
for the recon dual bar).

## 5. Vocabulary non-interchange

Frontier tokens (FRONT-GAP / FRONT-CROSSING / DEGENERATE / DOMINATED) and
decode-route arithmetic tokens (DECODE-GO / DECODE-TIE / DECODE-SHORT) are
**never interchangeable** (I9-1 R-3). A DECODE-GO is an arithmetic go-ahead,
never a frontier crossing. A crossing requires a landed measured row + PR-4
window + two hash-identical arbiter runs + bench sign-off + this gate's R-2
classification. Likewise the ratio-first lane's `FRONT-GAP (cost)` is a
different vocabulary; qualify cross-lane citations.

## 6. No verdict without lineage + an ablatable claim

Every novelty verdict requires (a) prior-art lineage (what exists, why it lost,
what the reference exploits) and (b) an ablatable claim separating the mechanism
from its coder and from the closest control. Absent either, the disposition is
"capability / engineering, no novelty claim", not silence and not approval.

## 7. Submission format and the one-line verdict

Submissions to this gate state: mechanism; provenance arms; exactness status
(zero residual? bounded? estimated?); the closest controls; the ablatable claim;
artifacts + hashes; the exact command; label. Verdicts are issued one line per
claim:

```
<VERIFY|REJECT|PARTIAL|CORRECT> <claim-id> — <one line> —
  artifact=<path@gitblob|sha256> cmd=<exact> label=<measured|derived|projection|prototype>
```

## 8. Dispositions ledger (everything sent to this gate this iteration)

| id | claim / submission | disposition |
|---|---|---|
| G-1 | pnra PR-5 original submission (`msg_3d53d393…`) | **RULED** — I9-2 P-1..P-6; no novelty for arm (B) |
| G-2 | pnra measured supplement (`msg_899c910d…`): transmitted 12,921 < derived 12,936 | **VERIFY** — decisive: derived arm unnecessary; corrections c1–c3 (I9-2 P-7) |
| G-3 | datastruct-ts ladder + erratum + "prototype wire" | **VERIFY** ladder/erratum; **REJECT** novelty; **NEW** dual-bar finding (I9-3) |
| G-4 | coordinator: synth-timeseries 20,000x14 (P=14); columnar 12,000x23 (P=23) | **VERIFY** — byte-exact regen + minimal-period invariant scan; supersession recorded |
| G-5 | strategy: "matches committed pareto-baseline 468/468 / tuple 463/468" | **PARTIAL REJECT** — recomputable only from the uncommitted worktree grid; committed HEAD is 416 cells / 411 dominated; the 5-row classification is verified in both |
| G-6 | strategy C-4: ratio-first decode multiple 20.3–21.0x vs carried 11.2x | **MIXED** — see §9; 11.2x recomputes exactly for Silesia; enwik8 sub-claim corrected to 20.96x; the 20.33x Silesia replacement is from a different window |
| G-7 | strategy PR-3-Q1 (synth-timeseries contingency token) | **ANSWERED** — I9-3 §6 |
| G-8 | format: dirty-build decode blocker | **REGISTERED/ENFORCED** — §2; roundtrip claims from that binary rejected |
| G-9 | bench: p0-arbiter (HEAD 411/416; worktree 463/468; fresh CSV hashes; 0 DEGENERATE) | **VERIFY** — this gate's independent recompute agrees cell-for-cell (448/448, 0 mismatches) |
| G-10 | bench: anti-tie convention v2 + PR-4 v1.1 | **RECORDED** — I9-1 R-3 (gate records, bench owns) |
| G-11 | format: fuzz test-set change (registry coverage, postcoder-id asserts, adversarial rev2) | **REGISTERED** — no numeric claim; re-run when green |
| G-12 | coordinator: frontier-tuple citation rule (HEAD 411/416 vs worktree 463/468) | **RECORDED** — matches I9-1 R-4 |

## 9. G-6 resolution — the ratio-first decode multiple (C-4), recomputed

Artifacts: `tests/bwt-backend-standard.csv`, `tests/ratio-first-standard.csv`,
`tests/auto-routing.csv`, `tests/enwik8-bwt.csv` — **all untracked**
(`git ls-files` returns empty; `git status --porcelain` shows `??`).
`de9b4caf…` is the **executable sha256** of `build\anvil.exe` (1,481,728 B),
not a commit. So every figure below is an **uncommitted artifact**; label it.

Routed files (BWT-wins, per the ratio-first record): webster, x-ray, mr, osdb,
dickens, reymont, nci.

```powershell
python - @'
import csv
def rows(p): return list(csv.DictReader(open(p,newline='')))
ROUTED=['webster','x-ray','mr','osdb','dickens','reymont','nci']
bwt=rows('tests/bwt-backend-standard.csv'); rfs=rows('tests/ratio-first-standard.csv')
def agg(rs,codec,files):
    ib=sum(int(r['input_bytes']) for r in rs if r['corpus']=='silesia' and r['file'] in files and r['codec']==codec)
    dt=sum(float(r['decompress_s']) for r in rs if r['corpus']=='silesia' and r['file'] in files and r['codec']==codec)
    return ib,dt,ib/dt/1e6
ib,dt,bm=agg(bwt,'anvil-bwt-direct',ROUTED); _,_,br=agg(rfs,'brotli-q11-lw30',ROUTED); _,_,xz=agg(rfs,'xz-9e',ROUTED)
print('7-file Silesia: in=%d dec_s=%.6f bwt=%.3f MB/s brotli=%.3f (%.3fx) xz=%.3f (%.3fx)'%(ib,dt,bm,br,br/bm,xz,xz/bm))
ew=rows('tests/enwik8-bwt.csv'); eb=[r for r in ew if r['codec']=='anvil-bwt-direct'][0]
ib=100000000; bm=ib/float(eb['decompress_s'])/1e6
b=[r for r in rfs if r['corpus']=='enwik8' and r['codec']=='brotli-q11-lw30'][0]
x=[r for r in rfs if r['corpus']=='enwik8' and r['codec']=='xz-9e'][0]
br=ib/float(b['decompress_s'])/1e6; xz=ib/float(x['decompress_s'])/1e6
print('enwik8: bwt=%.3f MB/s brotli=%.3f (%.3fx) xz=%.3f (%.3fx)'%(bm,br,br/bm,xz,xz/bm))
'@
```

Output:
- **7-file Silesia:** `in=120,362,284 dec_s=9.248178 bwt=13.015 MB/s
  brotli=146.329 (11.243x) xz=78.698 (6.047x)`.
- **enwik8:** `bwt=6.730 MB/s brotli=141.050 (20.960x) xz=82.831 (12.308x)`.

Rulings:
1. **The 11.2x / 13.0 MB/s pair IS recomputable** from the current
   `tests/bwt-backend-standard.csv` (11.243x; 13.015 MB/s). Strategy's C-4
   statement "not recomputable from the current CSVs" is **REJECTED**.
2. **The enwik8 sub-claim is a conflation and is corrected:** the audit's
   "6.7 MB/s = 11.2x" applied the Silesia multiple to enwik8. Correct:
   **20.96x vs Brotli / 12.31x vs xz**. The FRONT-GAP verdict **deepens**, it
   does not improve.
3. **Strategy's Silesia replacement (16.724858 s, 7.20 MB/s, 20.33x) is from a
   different artifact/window:** that sum is the `anvil-bwt-direct` rows for the
   same 7 files in `tests/auto-routing.csv` (~1.81x slower than
   `bwt-backend-standard.csv`; e.g. webster 6.769 s vs 3.516 s). It is
   **REJECTED** as the Silesia figure absent a PR-4 window attestation; both
   windows are flagged to `bench`/PR-4 to decide which is canonical.
4. **Citation of record:** see ruling **I9-4**
   (`docs/gate-ruling-i9-decode-multiples.md`) — Silesia 12-file auto portfolio
   **9.435 MB/s = 14.597x vs brotli / 7.885x vs xz**; enwik8 auto row
   **5.890 MB/s = 23.946x / 14.062x**. The **11.24x** figure survives only as the
   7-file *forced-BWT subset*; the earlier "enwik8 20.96x" is the same-bytes
   BWT-direct window variant, not the portfolio row. All artifacts
   **PENDING-COMMIT**; threads/window PENDING (PR-4). This §9 text is superseded
   by I9-4 where it differs.

## 10. Harnesses (this lane)

- `docs/gate-verify-i9.py` — frontier tuple + R-1/R-2 classification +
  baseline cross-check (does not replace `tools/pareto_front.py`).
- `docs/gate-verify-i9-corpus.py` — minimal record period + constant-offset
  fractions from bytes alone.
- `docs/gate-verify-i9-transform-bar.py` — raw vs transform-enabled reference
  bars for the two synth record files.

## 11. Standing orders (restated, binding)

1. No "Pareto win" / "crossed the frontier" without a FRONT-CROSSING
   classification from this gate (I9-1 R-2), and no crossing from a prototype
   wire or a projection.
2. No ratio claim without roundtrip verify + fuzz on a green binary. No
   throughput claim without median >= 3 + PR-4 fields + thread counts.
3. `tools/pareto_front.py` is the sole issuer of `EXTENDS_FRONT`; this gate is
   the sole classifier.
4. Every tally is recomputed at the moment of writing with the command shown.
5. Record, do not celebrate.

## 12. Measurement contract v1.2 (bench, binding — recorded 2026-09-12)

Coordinator-ratified (`msg_07044781be1f40fbb1f52817f3e6a268`); refines PR-4 v1.1.
This gate records it; `bench` owns it.

1. **Announced windows are exclusive.** During any `w-*` window, **all builds,
   gate runs, and fuzz are timing jobs and must hold** (a live arch gate test
   corrupted bwtinv's window). Byte-only CSV/arbiter work is exempt.
2. **Acceptance ladder:**
   - **RANKING-GRADE** = required fields present except affinity, **effect
     >= 5x arm CV**, external load <= 20pp over ambient.
   - **CITATION-GRADE** = all fields + affinity set + pinned-core mean < 5%
     pre-window + same-window paired arms + **reps >= 7 for a headline** +
     effect clears the requirement by `>= max(2%, CV_target, CV_ref)` + external
     load <= 5pp over ambient.
3. **Consequences in force:** bwtinv's 3.40-3.55x direction is
   ranking-grade-accepted; its absolute MB/s are **not citable** until a
   core-gated rerun. The aux I-array wire charge (~2-5 KB per whole-file block,
   ~40 KB across the Silesia portfolio) **must be co-listed with any ratio/net
   claim**. Binds `arch`'s materialization leg.
4. A consolidated citation-grade window will be scheduled once `src/anvil.cpp`
   is frozen; hold builds/gates/fuzz during it.

---

*Issued by `research-gate`. This regime binds this gate's own text first.*
