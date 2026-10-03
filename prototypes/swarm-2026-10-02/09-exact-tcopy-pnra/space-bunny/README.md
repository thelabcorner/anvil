# SLX-REF prototype sketch — track 09, Space Bunny Free

**Isolated.** Not on any build target. Not wired into `src/anvil.cpp`. No production path depends
on this directory.

Report: `docs/swarm-2026-10-02/09-exact-tcopy-pnra-space-bunny.md`.

## What this is

A **sketch**, not a production candidate. It exists to make three things concrete and checkable
before anyone spends remote budget:

1. the span-local recognizer (`classify_sites`) — the only genuinely new decoder-side code;
2. the arm-parameterized serializer — one token multiset, five wire representations;
3. the decoder that **recomputes** the site set instead of reading it — the property the whole
   mechanism is about, and the one a wiring bug would silently destroy.

Everything else (entropy coding, real match finding, block framing, cost model, integration) is
deliberately absent.

## Verification state — read this first

| Check | Result |
|---|---|
| Compile-only syntax/warning check | **PASS**, `clang-cl /std:c++20 /c /O2 /EHsc /W4 /WX /D_CRT_SECURE_NO_WARNINGS` |
| Linked | **no** |
| Executed (`--selftest` or any corpus) | **no** |
| Corpus benchmark / sweep / fuzz | **no** — remote-only by mandate |

The `.obj` was written to a temp directory outside the repo. Nothing in this directory was executed.
Every number in the report is therefore `MEASURED`-from-ledger, `DERIVED`, `ESTIMATE`, or
`HYPOTHESIS` — never a number this sketch produced.

## Arms

`Arm` selects the representation of the **same** token multiset. This is the whole point: a density
difference between arms is attributable to representation alone, with no parse difference.

| enum value | `--arm=N` | Parameter | Topology | Report label |
|---:|---|---|---|---|
| 0 | 0 | — | — | `A0` exact-LZ only |
| 1 | 1 | transmitted | computed | `A1` |
| 2 | 2 | transmitted | transmitted | `A1'` |
| 3 | 3 | derived (±d) | transmitted | `A2` = today's mode 14 |
| 4 | 4 | derived (±d) | computed | `A3` = SLX-REF |

On a pure `C1 REL32` corpus, arms 1/2 and 3/4 differ by ≈1 bit per phrase (report §3.1). That is why
the primary pre-registered threshold is `A3` vs `A2` (topology axis) and `A3` vs `A1` is secondary
with a pre-declared expected null.

## Classes (frozen for v1)

| Class | Field | Sign | Recognizer |
|---|---|---|---|
| `C1 REL32` | 4 bytes at `marker+1` | −1 | `0xE8`/`0xE9` scan of the just-copied span |
| `C2 ABS32-P` | three 4-byte words per 12-byte triple | +1 | `L % 12 == 0` and every triple has `begin < end`, `unwind % 4 == 0` |

`C2` is evaluated first and claims its sites; `C1` does not re-claim a claimed site. Deterministic,
zero bits.

## Invariants asserted or enforced in code

- **I1 non-overlap.** No site satisfies `off + 4 > dist`. Enforced in `classify_sites`, re-checked in
  `deserialize`, and re-checked at apply time. Inherited from `FORMAT.md:491-492`; **must not be
  relaxed** — it is what makes overlapping transformed copies unambiguous.
- **I2 purity.** `classify_sites(span, L, dist)` depends on nothing but its arguments. No cross-token
  state. A production integration must add an explicit assert here.
- **I3 audit-7.** Every recognizer input is a byte the decoder has **already reconstructed**
  (`out[p-dist .. p-dist+L)`, produced by step 1) plus `dist` and `L`. No look-ahead past `p+L`, no
  transmitted alphabet, no transmitted class id. See
  `docs/gate-ruling-i9-pr5-position-derived.md:84-93`.
- **I4 exactness is encoder-side.** `xref_reconstructs` is the gate: the encoder refuses any token
  whose reconstruction is not bit-exact. The decoder performs no verification branch.
- **I5 topology disjointness.** Transmitted topologies are rejected by `sites_are_disjoint`.
- **I6 bounds / full consumption.** `deserialize` requires `at == w.size()` at the end, rejects
  `dist > p`, `len == 0`, `len > 65536`, sites past `len`, non-±1 signs, and overlap. Mirrors
  `FORMAT.md:500-503`.

## What a production integration must replace

- `parse()` is a 12-byte-prefix hash probe with a **single** chain step and **no** exact-match
  fallback. It is a candidate *source*, not a parser. The real parser must offer `XREF` through the
  shared cost model (`RESEARCH_LEDGER.md:2506-2510` records that the mode-14 cost-model-gated path
  already exists) — and must carry the frozen framing-proportional acceptance gate
  (`docs/s62-threshold-formula.md` §3, γ = 0.5). **Do not re-tune γ against held-out data.**
- `head` is a flat 1M-entry table with one slot and one chain link; a real temporal index needs the
  recency/channel structure and the locality controls.
- No entropy coding, no block framing, no CRC, no context modelling.
- `C2` needs a charged auto-detection story before it can be measured on any cell
  (`RESEARCH_LEDGER.md:5092-5096` records the +24…+48 B precedent for on-wire detection).

## Remote-only invocation (never run locally)

Byte-only tier R1 per `docs/I10-BREAKTHROUGH-PROGRAM.md:541-553`.

```
clang-cl /std:c++20 /O2 /EHsc /W4 /WX /D_CRT_SECURE_NO_WARNINGS \
  prototypes/swarm-2026-10-02/09-exact-tcopy-pnra/space-bunny/slxref.cpp /Fe:slxref.exe
./slxref --selftest
for f in tests/corpus/anvil.exe tests/corpus/anvil_bench.exe; do
  for a in 0 1 2 3 4; do ./slxref "$f" "/tmp/$f.arm$a" --arm=$a; done
done
for f in tests/corpus/pe-*.exe; do
  for a in 0 1 2 3 4; do ./slxref "$f" "/tmp/$f.arm$a" --arm=$a; done
done
```

`--selftest` is expected to report `PASS arm=<n> wire=<n> tokens=<n>` for all five arms plus
`PASS mutation sweep`. **Expected, not observed** — it has not been run.

## Corpus split (binding)

- Calibration: `anvil.exe`, `anvil_bench.exe` **only**.
- Verdict set: the six locked held-out PEs (`pe-winver`, `pe-where`, `pe-notepad`, `pe-python`,
  `pe-ninja`, `pe-git`; 5,540,960 B total).
- Any `{synthetic}` cell needs the dual bar and a `{synthetic}` tag
  (`RESEARCH_LEDGER.md:5346`, `:5117`).
- Any v2 citation co-lists xz-9e and carries `GRID-THIN`
  (`RESEARCH_LEDGER.md:5316-5326`).

## Known sketch limits

- `A4` (global BCJ + same backend) and `A5` (relocation-normalized control) are **not implemented
  here**. They are external bars and must be produced by the real harness; the prototype cannot
  discharge them.
- The prototype wire is counted/varint with no entropy coding. Absolute byte counts are **not**
  comparable to `anvil.exe` or brotli — only the within-harness arm-to-arm deltas are meaningful
  (the `prototypes/orbit_ariref` precedent, `RESEARCH_LEDGER.md:2933`).
- `head` is 8 MiB of encoder-only memory; this is not a charge against any decoder-side cost claim.