# FORMAT lane — I9 close-out report (read-only investigation)

Lane: `format` (owns FORMAT.md + decoder strictness). No tracked file edited.
HEAD observed: `fc23d9a191d7ab488ebe05be598bfd93c1f840ca`.
Current `src/anvil.cpp` sha256: `BDC9047434D19106D1D2316DC094FD615F8B71E3EB0FCA98CAFCD076B53B8CE0`.
Current canonical `build\anvil.exe` sha256: `72D65150CBD65182C69BA85DAD3A8961C25DE48E794FD9823BCAB4ED20A36505`.

## (1) FORMAT.md COVERAGE GAP TABLE

Block-mode / transform namespace cross-checked against `src/anvil.cpp`.

| mode | in src? | in FORMAT.md? | line (FORMAT.md) |
|---|---|---|---|
| 0 raw | yes | yes | 37 |
| 1 arith o0 | yes | yes | 38 |
| 2 arith o1 | yes | yes | 39 |
| 3 arith g4 | yes | yes | 40 |
| 4 arith g8 | yes | yes | 41 |
| 5 arith g16 | yes | yes | 42 |
| 10 rANS | yes | yes | 43 |
| 11 SPARSE-REF | yes | yes | 44 |
| 12 SHAPE | yes | yes | 45 |
| 13 TOPOLOGY | yes (src:3445-3548) | yes | 46 |
| 14 TCOPY | yes | yes | 47 |
| 15 HOTOP | yes | yes | 48 |
| 16 ARI-REF | yes (src:2510-2580) | yes | 49 (explicitly "provisional/uncommitted at I9 registry pass") |
| 17 rev-2 ratio | yes | yes | 50 |

Stream-suite codec namespace (0-8) documented at FORMAT.md:530-540, incl. mode 7 RePair
(539) and mode 8 RLZ (540) with full specs at 580-676. Rev-2 transforms 0-3 documented
at 78-124; backend IDs at 133-136; postcoders 0-4 at 167-173.

**Gap assessment: NONE for modes 7/8 or I9-touched items.** Both RLZ (mode 8) and RePair
(mode 7) are documented with version discipline: wire layout (FORMAT.md:591-595), acyclic
rule constraint (613-616), amplification invariants (621-627), RLZ addend wrap guard
(643-646), the `at_end()` 7/8 explicit-branch requirement (663-667), and the recursion cap
(668-671). Mode-13's outer-dim change is recorded at FORMAT.md:862-865 ("stream codecs 7/8
additionally enforce…") and decoder-audit.md:185-186. Mode 16 carries an explicit
provisional-wire caveat (FORMAT.md:49), which is the correct discipline for uncommitted wire.

Residual (documentation-precision, not a gap): FORMAT.md's registry table (827-836) lists
stream-suite ids 7/8 as "selection-only; 8 observed, 7 not selected on the matrix" (835),
matching the fuzz `suite_unforceable=[4,7]`. FORMAT.md:842 says "ids 4 and 7 are not selected"
— consistent. No mode present in src is absent from FORMAT.md, and no mode documented in
FORMAT.md lacks a src implementation.

## (2) FUZZ HARNESS CAPABILITY

`tests/fuzz.py` (800 lines, 40,613 B). Real harness; every claimed counter is produced by
code in this file.

| counter | produced at | real? |
|---|---|---|
| `roundtrip_variants=` | fuzz.py:793 (`total+=1` at 793 inside double loop 768-770) | yes |
| `mutations=` | fuzz.py:792 (`mutated+=1` inside `mutate()` loop 786) | yes |
| `deterministic_rev2=` | fuzz.py:759 `adversarial_rev2`; count at 104 (9 cases) | yes |
| `deterministic_bwt=` | fuzz.py:760 `adversarial_bwt`; count at 363 (24 cases) | yes |
| `golden_bwt=` | fuzz.py:761 `golden_bwt`; count at 499 (4 inputs) | yes |
| `forced_postcoders=` | fuzz.py:765 `forced_postcoder_roundtrip`; 5 ids x 4 inputs = 20 (397-406) | yes |
| `registry_block_modes=` | fuzz.py:766; 14 cases at 574-589 | yes |
| `registry_transforms=` | fuzz.py:766; 5 cases at 598-616 | yes |
| `suite_modes=` / `suite_unforceable=` | fuzz.py:767 `stream_suite_coverage` (649-706), printed 798 | yes |

Arithmetic of the claimed PASS line: `--cases 50` (README.md:74, docs/CONTEXT.md:44) yields
`patterns()` = 10 fixed + 50 random = 60 inputs (fuzz.py:710-712); 60 x 8 combos
(fuzz.py:754-755) = **480 roundtrip_variants**; 480 x 6 mutations/file
(fuzz.py:752 default `--mutations 6`) = **2880 mutations**. So 480/2880 is exactly the
`--cases 50` run, not the default (`--cases 120` -> 1040/6240).

Skip analysis: **no corpus-skip path.** `adversarial_bwt` returns a `skipped` list
(fuzz.py:189) that is never appended to (always `[]`, fuzz.py:363), and the degenerate
1-byte/all-equal cases are now asserted round-trips (346-358). The only `continue` is
fuzz.py:625, which permits a *clean up-front rejection* of transform 3 (`--bwt-lzp=on`) —
a rejection, not a skipped assertion. So the "zero skips" claim is structurally sound.

Caveats on coverage (real but not defects):
- `suite_modes=[0,1,2,3,5,6,8]`, `unforceable=[4,7]` — a codec that stops being *selected*
  is detected only by absence (fuzz.py:657-659). Ids 4 (Huffman) and 7 (RePair) have no
  forced path, so a mode-7 decoder regression would NOT be caught by suite coverage alone.
  Mode-8 coverage is selection-dependent too.
- The mutation oracle (fuzz.py:789-791) accepts a mutation whose decoded output is
  *identical* to the original (CRC-collision tolerance); a mutation that is silently
  accepted with different output is the only failure — correct and strict.

## (3) HARDENING ITEM PRESENCE TABLE

All five items are PRESENT in current `src/anvil.cpp` (sha `BDC90474`).

| item | present/absent | file:line |
|---|---|---|
| (a) StreamPull `at_end()` explicit 7/8 branch | PRESENT | `src/anvil.cpp:2138` (`if (codec == 7 \|\| codec == 8) return rp_i >= rp_buf.size();`) |
| (b) mode-8 varint-wrap OOB reject (before `+1`) | PRESENT | `src/anvil.cpp:1880-1884` (`dv>=0xFFFFFFFFull\|\|lv>=0xFFFFFFFFull` -> "bad rlz match varint"; then `dist>out.size()`); macro/other paths also guarded at 2945,3222,3303,3497 |
| (c) RePair expansion amplification bound | PRESENT | `src/anvil.cpp:1851-1858` (per-rule `exp[i].size()>raw_n`; cumulative `> 2*raw_n+65536`) |
| (d) stream-recursion depth cap | PRESENT | `src/anvil.cpp:1777,1781` (`int depth=0`; `mode>=7 && depth>0` -> "nested rlz-reap stream"); depth passed at 1832,1870 |
| (e) mode-13 topology table outer dim 64->65, wire unchanged | PRESENT | `src/anvil.cpp:3453-3455` (`std::array<std::array<uint8_t,64>,65>` + matching `has_modal`); k validated `1..64` at 3463 |

Corroborating evidence: `docs/decoder-audit.md:191-242` records F1-F4 all `[CLOSED]`,
landed in `da01c54`, with crafted probes (`scratch/format/probes/P_F2_WRAP.anv` 59 B,
`P_F3_AMP.anv` 212 B, `P_F4_NEST.anv` 145,899 B) all rejecting cleanly in <10 ms.
ASan-found mode-13 dim fix recorded at decoder-audit.md:184-189 (`aea3023`).

Note (b) is *slightly weaker than the audit text*: the source rejects at
`>= 0xFFFFFFFF` (not `== 0xFFFFFFFF`/`UINT64_MAX`). This is the macro-path precedent
style and is safe — `dv = 0xFFFFFFFF-1` gives `dist = 0xFFFFFFFF`, which then fails
`dist > out.size()` (out.size() is bounded by the 64/128 MiB block cap), so no OOB.
FORMAT.md:643-646 documents the `>= 0xFFFFFFFF` contract exactly.

## (4) MOST RECENT GREEN FUZZ RESULT + its src sha, and current-canonical coverage

**On-disk artifacts (all under `prototypes/i9-arch/logs/`):**

| log | mtime | binary pinned | src pinned | result |
|---|---|---|---|---|
| `fuzz50_mat.log` | 2026-09-12 02:51 | (mat build) | `62BC6631`-era | PASS (identical line) |
| `fuzz50.log` | 2026-09-12 01:56 | `8EAE1FB3` (claimed) | `62BC6631` | PASS |
| `fuzz50_alloc.log` | 2026-09-12 04:22 | alloc build | `38409E26`-era | PASS |
| `fuzz50_alloconly.log` | 2026-09-12 04:56 | alloc-only | `38409E26`-era | PASS |
| `fuzz50_leg4.log` | 2026-09-12 06:09 | leg-4 lane (`FE0CF4F1`) | `BDC90474`-era | PASS |

Every log carries the identical PASS line: `roundtrip_variants=480 mutations=2880
deterministic_rev2=9 deterministic_bwt=24 golden_bwt=4 forced_postcoders=20
registry_block_modes=14 registry_transforms=5 suite_modes=[0,1,2,3,5,6,8]
suite_unforceable=[4,7]`.

**`tests/fuzz-result.txt` is STALE** — 3 lines, mtime 2026-08-13, counters
`650/5200`, `1050/10500`, `800/6400`. It is NOT the I9 gate evidence and cannot be
matched to any I9 sha. `tests/fuzz-result.txt` quoted as the I9 gate would be wrong.

**Sha ↔ binary problem.** The strategy doc (swarm-i9-strategy.md:62) pins the PASS to
`build-i9-format\anvil.exe` sha256 `8EAE1FB3...` (src `62BC6631`). No file on disk hashes
to `8EAE1FB3`: `build-i9-format\anvil.exe` currently hashes `BA641C676745B8CA...`
(mtime 2026-09-12 06:00). `8EAE1FB3` survives only as a *citation* (FORMAT.md:106,
RESEARCH_LEDGER.md:4751/4752/5162) — there is no on-disk `8EAE1FB3` binary to re-verify.
The strongest on-disk leg-4 log (`fuzz50_leg4.log`, 06:09) is also *after*
`build-i9-format\anvil.exe`'s mtime, so even the leg-4 log does not cleanly bind to the
`8EAE1FB3` artifact.

**Does a GREEN fuzz result exist for the CURRENT canonical src sha `BDC90474`?**

- Direct log evidence: **NO on-disk log names `BDC90474` or `72D65150`.**
- Documented evidence: **YES, by ledger testimony.** RESEARCH_LEDGER.md A17 (lines
  5164-5173): `format` gate `b-format-gate-20260912T115333Z` on `72D65150` (frozen src
  `BDC90474`): "PASS, zero skips (roundtrip_variants 480, mutations 2880, … registry
  identical to both prior freezes)". So a canonical-sha PASS is *recorded*, but the
  raw log it cites is not present in `prototypes/i9-arch/logs/`.

**Verdict:** the *strategy doc's* PASS line is pinned to a superseded sha (`8EAE1FB3` /
`62BC6631`) and to an artifact that no longer exists on disk. A canonical-sha PASS
(`BDC90474`/`72D65150`) is claimed in the ledger (A17) but is **not reproducible from
on-disk logs** — this is exactly the "relying on a stale PASS" risk. The correct
reading: a PASS exists for the current src, but only as ledger assertion, not as
retained machine output.

## (5) STRICTNESS CONCERNS

The decoder is broadly strict (FORMAT.md:844-880; decoder-audit.md), but four residual
concerns for the lane:

1. **StreamPull marks the whole substream consumed unconditionally (src:2023 `q = e;`).**
   For pull-based codecs the parser sets the cursor to the substream end after decoding
   the header region, so a mode-1/2/3/4/6 substream with *trailing bytes after the
   declared data region* is not rejected by `q == qe`; only `at_end()`'s `p == rend` /
   `hp == hend` protects it (2135-2137). For rANS, `rend = p + dn - 4` and renorm bytes
   beyond `rend` are simply never read — trailing bytes within the declared `dn` region
   are structurally invisible. Contrast the eager `decode_stream` path, which rejects
   trailing bytes at 2170. This is a decode-strictness asymmetry between the fused
   (mode-12/15) and eager (mode-10/11) paths; it is not memory-unsafe, but a
   "trailing bytes" mutation on a fused substream may be *silently accepted* rather than
   rejected (it still cannot change output, so CRC holds). Worth a targeted probe.
2. **`at_end()` defexc fallthrough (src:2139)** remains the final `else` for any codec not
   enumerated (notably codec 5). A future codec added without an `at_end()` branch would
   inherit the defexc predicate — the exact F1 defect class. A `default`-arm throw would
   harden this permanently.
3. **Mutation oracle tolerates identical-output acceptance (fuzz.py:789-791).** Correct
   per its comment, but it means "mutation rejected" is not actually asserted — only
   "mutation not accepted with different output". Corpus coverage of *strict rejection*
   is therefore weaker than the PASS line suggests.
4. **`read_varint_pull` (src:2143-2152) vs `read_varint_bytes` (2156)** both cap at 10
   bytes and throw on overflow — good. Mode-13 modal table validated `1..64`/`<k` (3463)
   and `mcount<=4096` (3459) — good. No silent-accept found there.

No memory-unsafety found by read; the mode-8 wrap (F2), RePair amplification (F3), and
nesting (F4) are all correctly guarded in the current source.

## (6) Recommended I9 close-out actions (format lane)

1. **Re-run and RETAIN a canonical PASS log.** Run
   `python tests\fuzz.py --exe build\anvil.exe --cases 50` against `72D65150` and save
   the raw log to a named, tracked-adjacent path (e.g.
   `prototypes/i9-closeout/fuzz-72d65150.log`) so the gate evidence binds to the
   current src sha (`BDC90474`) rather than to the vanished `8EAE1FB3` artifact. This is
   the single blocking item: no on-disk machine output proves a PASS for the canonical src.
2. **Restore `tests/fuzz-result.txt`** (or add a dated replacement) — it is stale
   (2026-08-13, 650/5200-class counters) and misleading if read as I9 evidence.
3. **Resolve the `8EAE1FB3` citation.** Either rebuild the artifact and confirm the sha,
   or annotate FORMAT.md:106 / LEDGER:4751-4752 / strategy:62 as "artifact not retained;
   superseded by A17 on `72D65150`".
4. **Harden `StreamPull`:** add a `default: throw` arm to `at_end()` (src:2132-2140) so a
   future codec cannot inherit the defexc predicate (F1 regression class), and add a
   trailing-bytes probe for the fused path (concern 5.1) to confirm whether trailing
   bytes after the declared data region are rejected or silently ignored.
5. **Document the mode-7/8 selection-only gap** in FORMAT.md is already present (835/842);
   consider adding a note that a mode-7 regression is not screenable by `--cases 50`
   (`suite_unforceable=[4,7]`) so future readers do not over-read the PASS.
6. **Pin the FORMAT.md verification block (822-825)** — it is already correct at
   `72D65150`/`BDC90474`; keep it as the citation of record and update the strategy doc
   §0 to stop quoting `8EAE1FB3`.
