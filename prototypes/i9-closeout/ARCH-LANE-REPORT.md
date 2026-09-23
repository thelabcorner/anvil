# arch lane — I9 close-out investigation report

Investigation only. No tracked file edited, no build, no benchmark run.
All commands are PowerShell 7, run from repo root `C:\Users\<user>\Documents\ANVIL`.

---

## 1. SHA CHAIN VERDICT

Command:
```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath "src\anvil.cpp","build\anvil.exe",`
  "build\anvil_bench.exe","build\anvil_old.exe","build-i9-arch\anvil.exe",`
  "build-i9-arch\anvil_bench.exe","build-i9-format\anvil.exe",`
  "build-i9-format\anvil_bench.exe" |
  ForEach-Object { "{0}  {1}" -f $_.Hash.Substring(0,8).ToUpper(), $_.Path }
```

| claimed sha | artifact | computed sha256 (prefix) | match? |
|---|---|---|---|
| src `BDC90474` (frozen canonical, ALLOC+leg4) | `src\anvil.cpp` | `BDC9047434D19106...` | **MATCH (live)** |
| exe `72D65150` | `build\anvil.exe` | `72D65150...` | **MATCH (live)** |
| bench `379341D9` | `build\anvil_bench.exe` | `379341D9...` | **MATCH (live)** |
| leg-4 LANE exe `FE0CF4F1` | `build-i9-arch\anvil.exe` | `FE0CF4F1...` | **MATCH (lane artifact)** |
| superseded src `38409E26` | — | not present on disk | **STALE / unverifiable** |
| superseded exe `E8AA2E48` | — | no binary hashes to it | **STALE / unverifiable** |
| superseded src `62BC6631` | — | not present | **STALE / unverifiable** |
| superseded exe `0D1E130B` | — | not present (`build-i9-format\anvil.exe` = `BA641C67`) | **STALE / unverifiable** |
| (doc-only, §0 line 62) format-gate exe `8EAE1FB3` | `build-i9-format\anvil.exe` | `BA641C67` | **STALE** — does not match |

**Verdict:** the live canonical chain (v3) holds exactly: `BDC90474` src ↔ `72D65150`
exe ↔ `379341D9` bench, and `FE0CF4F1` is confirmed a *lane* binary (`build-i9-arch`),
not `build\`. Every *superseded* sha is simply absent from disk — expected for
superseded binaries, but note this means their claims are **unverifiable from
artifacts alone**; only the strategy doc's prose attests them. Two extras:
`build\anvil_old.exe` = `98BDECB6`, `build-i9-arch\anvil_bench.exe` = `76CB8BD4`
(both unclaimed).

---

## 2. WORKTREE DIFF SUMMARY

Command: `git diff --numstat HEAD -- src/anvil.cpp` → `1460  69  src/anvil.cpp`
(1460 insertions, 69 deletions, 1 file). Worktree sha256 `BDC90474...` ≠ HEAD blob
(`git rev-parse HEAD:src/anvil.cpp` = `f7aa949e...`; `git hash-object` = `f55c2fd4...`).

**So `src/anvil.cpp` is heavily modified vs HEAD (`fc23d9a`) — it is NOT byte-identical
to HEAD.** Yet its sha `BDC90474...` is the doc's *frozen canonical* sha: the frozen
canonical is a **worktree state**, not a commit. Note `fc23d9a`'s blob is `f55c2fd4`,
which is neither `BDC90474` (worktree) nor any claimed superseded src sha — consistent
with "no commit" (doc §0 line 82), and worth flagging: the frozen canonical is
uncommitted and would be lost by a hard reset.

Functional areas touched by the diff (from `git diff HEAD -- src/anvil.cpp`): CRC32
PCLMULQDQ fast path (new ~lines 300-433), RLZ/RePair stream codecs (modes 7/8, ~1656-1775),
hot-op stream budget (`--hotop-budget`), the mode-13 topology outer-dim 64→65 ASan fix,
`MatchFinder::find` hash4 `pos+4` guard, `decode_one_block`/`decode_one_block_into`
refactor + ALLOC leg 4, and new CLI flags. This is a *mega-commit's* worth of work
sitting uncommitted.

---

## 3. CRC / STORE-PATH CLAIM VERDICT

**(a) Wire-invisible — SUPPORTED (strong).** The CRC is a *checksum value*, not a wire
byte: `uint32_t sum=crc32(block.data(),block.size());` at `src/anvil.cpp:4695`, stored
verbatim (`put_u32le(out,sum)` 4697/4699) and verified on decode. Only the *implementation*
behind `crc32()` changed (dispatcher `src/anvil.cpp:428-433`; `crc32_pclmul` 374;
`crc32_slice8` fallback). Evidence of bit-identity: the AB log's `wire_fnv` is identical
across all three arms on both files (`store_ab_w-arch-storepath.log:7-36`,
`CCA22D2F497CC5B7` / `DF74CCD877E93DC3`), plus the ledger's 16,529 + 5,784 CRC checks
0-fail and 494/494 wire gate (`RESEARCH_LEDGER.md:5280-5281`). Claim holds.

**(b) 4.5x/3.4x speedup provenance — SUPPORTED as an artifact.** Present and complete
under `prototypes/i9-arch/`:
- `store_ab.ps1` (the runner; interleaved, reps=5, core18, threads=1, per-arm sha line 13)
- `storepath-rootcause.md` (root-cause writeup, 41 lines)
- `logs/store_ab_w-arch-storepath.log` (raw per-rep data + medians + CV)
Arithmetic checks out: random.bin enc 2.4258/0.5341 = **4.54x**; dec 2.7611/0.8221 =
**3.36x**; synth-arith enc 2.3898/0.4969 = 4.81x, dec 2.7508/0.8723 = 3.15x. The doc's
"4.5x enc / 3.4x dec" is the random.bin (best-case) pair, reasonable. **Caveat:** this is
a *standalone microbench* against three purpose-built arms (`st_bytewise.exe`,
`st_slice8.exe`, `st_pclmul.exe`), not a full-codec `anvil_bench` run — the numbers are
crc-only ns/B, not codec MB/s. The claimed end-to-end "1950-2184 MB/s enc" band is
*inferred* from these, not directly measured by the artifact.

**(c) A21 "closed" status — SUPPORTED by artifact, but the artifact is the same
microbench, not a bench-signed codec run.** A21 (`RESEARCH_LEDGER.md:5260-5292`) is
marked "bench-signed", yet the entire supporting evidence is `prototypes/i9-arch/`
(arch-produced) plus the AB log. No bench-owned artifact was found: search
`Get-ChildItem -Recurse -Path docs,deliverable,tests | Where-Object Name -match 'A21|storepath|47c26072'`
returned **nothing**; the only hits for A21/4.5x are prose in `docs/anvil-i9-findings.md`
and `RESEARCH_LEDGER.md`, and the strategy doc §0 line 97. So "bench-signed" means
*bench signed a window id*, not that bench reproduced the numbers on the canonical
binary. The claim is *artifact-backed but not independently reproduced by the bench lane*.

---

## 4. MODE REGISTRY + ASAN FIX PRESENCE

**Block-mode registry (decode dispatch `src/anvil.cpp:4713-4739`):**
0 raw · 1-5 legacy token modes · 10 rANS · 11 sparse · 12 shape · 13 topology ·
14 tcopy · 15 hotop · 16 ariref · **17 ratio (revision-2 only)**. Modes 6-9 are **not
block modes**: they are *stream* modes inside the entropy stream suite (mode 4 Huffman,
mode 5 exception, mode 6 context-rANS, **mode 7 RePair, mode 8 RLZ**), decoded by
`decode_stream` at `src/anvil.cpp:1777+` (`if(mode>=7 && depth>0) throw "nested
rlz-reap stream"` at 1781).

**Modes 7/8 wired + default-off — CONFIRMED.** Definitions 1659-1666; encoder candidates
`repair_stream_bytes`/`rlz_stream_bytes` tried only under `if (g_rlz_reap)` at
`src/anvil.cpp:2869-2875`; `g_rlz_reap` defaults `false` (1677) and is only set from
`opt.hotop_rlzp` (4569); `opt.hotop_rlzp=false` (3785); CLI `--hotop-rlzp=` defaults off
(4884). Smallest-wins selection against the existing suite ⇒ flag-off path is
byte-identical. Default-off: **yes**.

**ASan fix 1 — outer dim 64→65: PRESENT.** Encoder `std::array<std::array<uint8_t,64>,65>
modal{}` at `src/anvil.cpp:3378` and `has_modal` 3379; decoder the same at 3454/3455.
Comment at 3377 documents the prior `[64][j]` silent OOB. Guard at 3463
(`if (k == 0 || k > 64 || j >= k)`) and 3533 (`if (pc > 64 || ...)`) accept k==64.

**ASan fix 2 — MatchFinder::find hash4 `pos+4` guard: PRESENT.**
`if (pos + 4 > d_.size()) return out;` at `src/anvil.cpp:541`, with the inline comment
that the probe may sit on the EOF token boundary. Matches commit `aea3023` description
("walk() guarded, probes weren't" — cf. walk guards at 503/528/532).

Both `aea3023` fixes are verbatim in the current source. `aea3023` diff stat confirms
`8 insertions, 4 deletions`, consistent with the two guards + two dim changes.

---

## 5. DISCREPANCIES (ranked)

**D-1 (HIGH) — the "frozen canonical" src sha is an uncommitted worktree, and the doc's
sha-citation rule obscures this.** `src/anvil.cpp` is dirty (+1460/-69 vs HEAD) and its
`BDC90474` sha is a *worktree* state; HEAD `fc23d9a`'s blob is `f55c2fd4`
(`git rev-parse HEAD:src/anvil.cpp`). Doc §0 (lines 76-82) presents `BDC90474` as "frozen
canonical" and then says "No commit" — true but easy to misread as committed. Any
`git checkout/stash` destroys the canonical. **Recommended:** commit or tag the frozen
state (or record `BDC90474` as an explicit worktree-only canonical + a `git stash`
guard) before close-out.

**D-2 (MEDIUM) — A21 "bench-signed" overstates independent verification.** Doc §0 line 97
and `RESEARCH_LEDGER.md:5260` label A21 bench-signed, but all evidence is arch-lane
(`prototypes/i9-arch/`, `store_ab.ps1` runner authored and run by arch). No bench-owned
artifact exists (searches found none). The microbench is *crc-ns/B on 3 hand-built arms*,
not a codec-level `anvil_bench` median-3 run on `72D65150`. **Doc claims a bench-signed
citable win; truth is an arch-run microbench with an anonymous "window id".**

**D-3 (MEDIUM) — "store throughput citable" rests on a microbench, not a codec
measurement.** `storepath-rootcause.md:38-41` and ledger A21 claim store-path throughput
"citable" at 4.5x/3.4x. The artifact measures only `crc32` cost (st_*.exe arms), so the
end-to-end codec band (1950-2184 MB/s) is an extrapolation. Citable as *a CRC-implementation
microbench*, not as *codec store-path throughput*.

**D-4 (LOW) — strategy doc §0 line 62's gate exe sha `8EAE1FB3` is stale.**
`build-i9-format\anvil.exe` now hashes `BA641C67`, not `8EAE1FB3`; likewise the doc's
`62BC6631` src pairing is absent. These are declared superseded elsewhere, so low impact,
but the §0 line 62-65 fuzz-gate PASS is pinned to shas that no longer exist on disk and
cannot be re-verified.

**D-5 (LOW) — superseded sha chain is prose-only.** `38409E26`/`E8AA2E48`,
`62BC6631`/`0D1E130B` cannot be recomputed (no artifacts). Not a correctness problem,
but the "chain" in the doc is partly unfalsifiable from the repo.

**No discrepancy found** in: mode 7/8 wiring + default-off; both `aea3023` ASan fixes;
the live v3 canonical sha trio; CRC wire-invisibility mechanism.

---

## 6. RECOMMENDED I9 CLOSE-OUT ACTIONS (arch lane)

1. **Freeze the canonical properly (D-1):** commit `src/anvil.cpp` @ `BDC90474` (or tag
   the worktree state) so the frozen canonical is durable, and record that `build\anvil.exe`
   `72D65150` / `anvil_bench` `379341D9` are the v3 binaries. This is the single highest-
   value close-out action — everything else is gated on the canonical surviving.
2. **Down-label A21 (D-2/D-3):** change "bench-signed" to "arch microbench, window id
   `w-arch-storepath-20260912T1300Z`, pending bench end-to-end confirmation"; or ask bench
   for one median-3 `anvil_bench` run on the store path with `72D65150` vs a no-CRC build
   to make the codec-level band real.
3. **Mark stale shas (D-4):** annotate strategy §0 lines 62-65/69-80 that `8EAE1FB3`,
   `62BC6631`, `0D1E130B`, `E8AA2E48`, `38409E26` are non-recomputable (no artifacts),
   so future lanes do not attempt to verify them.
4. **Confirm flags are committed as default-off** in the close-out note: `--hotop-rlzp`
   (3785/4884), `--hotop-budget` (3786/4885), `g_rlz_reap=false` (1677) — so the shipped
   default path is byte-identical to the frozen pre-flight wire.
5. **No code changes recommended** from this lane: the two ASan fixes and modes 7/8 are
   correct and gated. Do not re-build without re-pinning shas.
