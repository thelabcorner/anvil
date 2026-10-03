# Track 11 / Space Bunny Free — FLI representation oracle

Isolated, un-wired prototype for the FLI (Fused Loop Instruction) mechanism described in
`docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md`. Nothing here is referenced by
`src/anvil.cpp`, `FORMAT.md`, or any production or test path. Default-off by construction:
there is no default because nothing is linked.

## What it computes

`loop_oracle` builds a **greedy exact-LZ token trace** of an input file and then measures, over
that trace, the quantities the FLI byte leg depends on:

1. histogram of maximal runs of consecutive match tokens, by run length, capped at `--runcap`;
2. recurrence hit matrix for runs of `k >= 2`: `(dL=0,dD=0)`, `(dL=0,dD!=0)`, `(dL!=0,dD=0)`,
   `(dL!=0,dD!=0)`, plus a degenerate bucket for `k < 4`;
3. **coverage** = match bytes inside *admissible* runs (`k >= 4`, constant `dL`, constant `dD`),
   as a fraction of match bytes and of total input bytes;
4. **symbols eliminated** = `sum(k-1)` over admissible runs — the timing-free quantity that a
   later remote timing run converts into MB/s using a single calibration constant;
5. **byte delta** under two explicitly separated baselines:
   * **B2 (binding)** — the fused hot-op book model: an expanded iteration costs one opcode
     symbol `H_op`; a loop costs one opcode symbol plus `uvarint n`. Swept over
     `--hop=0.3,0.5,1.0,1.5` bytes.
   * **B1 (reported, NOT binding)** — raw LZ model: an expanded iteration costs
     `1 + uvarint(len-4) + uvarint(zigzag(dD))`; a loop costs `1 + uvarint(n)`. B1 flatters FLI
     by construction and exists only so the two readings cannot be confused.
6. Runs with `k < 4` are **never** admissible: a two-parameter recurrence over `k` tokens saves
   nothing below `k = 3`, so that bound is derived, not tuned.

## Honest framing (mandatory when quoting any number from this tool)

* Absolute byte counts are **not comparable** to `anvil.exe`, to Brotli, or to the frozen suite
  baselines. Only the within-harness loop-vs-expanded deltas and the coverage percentages are
  meaningful. The synth-cell dual-bar rule applies.
* The matcher is a 4-byte-hash, 4-entry-MRU, min-match-4 greedy matcher. It is **not** ANVIL's
  bits-based MDL-gated parse (`FORMAT.md` §Parser), which rejects cost-increasing tokens.
  Coverage measured here is therefore an **optimistic bound**: the production parse would admit
  a subset of these runs. Treat this tool as a **screen**, not a decision. The decision is the
  P3 A1/A2 arms.
* This tool is **not** a benchmark. It reports counts and deltas only: no timing, no MB/s.

## Build (remote only)

```
clang-cl /std:c++20 /MD /O2 /EHsc /DNDEBUG loop_oracle.cpp /Fe:loop_oracle.exe /Fo:<objdir>/
```

or

```
g++ -std=c++20 -O2 -o loop_oracle loop_oracle.cpp
```

Build scratch, when it happens, belongs in `scratch/swarm-tmp/11-orbit-programs-space-bunny/`
inside the repository. No external temp paths are requested or used.

## Run

```
./loop_oracle <file> [<file> ...] [--hop=0.3,0.5,1.0,1.5] [--runcap=64]
```

Records emitted on stdout, one per line, comma separated:

* `RUNSUM,<file>,<input_bytes>,<tokens>,<literal_bytes>,<match_bytes>,<runs_ge2>,<runs_adm>,<match_bytes_in_adm_runs>,<cov_match_pct>,<cov_input_pct>,<symbols_eliminated>`
* `HITM,<file>,<LL_DD>,<LL_DN>,<LN_DD>,<LN_DN>,<degenerate_k_lt4>`
* `HIST,<file>,<k>,<count>` for `1 <= k <= runcap`, then `HIST,<file>,cap+,<count>`
* `DELTA,<file>,B2,<hop_bytes>,<delta_bytes>,<delta_pct_input>,<delta_pct_expanded_symbols>`
* `DELTA,<file>,B1,NA,<delta_bytes>,<delta_pct_input>,NA`

Exit status 0 on success, 2 on bad arguments or unreadable input. The tool never writes files.

## Execution status

**NOT BUILT AND NOT RUN by the authoring agent** (compile-checked only: `clang-cl /c` exit 0,
one benign MSVC `fopen` deprecation warning). Per `docs/swarm-2026-10-02/MASTER-BRIEF.md`
item 7, all measurement is GitHub-Actions-only.

**SUPERSEDED — DO NOT RUN THIS TO CERTIFY ANY GATE.** The mechanism it was written to screen
(FLI) is **KILL** in `docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md` §16.7, and this
instrument is itself the reason two of its gates are void:

* §16.6 records that `G2`/`G3` (coverage, run length) are **invalid as written**, because they
  were to be certified by this tool, and this tool's own header states its coverage is "an
  **optimistic bound** relative to what the production parse would admit". A threshold certified
  by a knowingly optimistic instrument is not a threshold.
* Any future coverage claim requires a **production-parse-trace** instrument, not this
  greedy-trace proxy.

**Permitted residual use.** If a later lane wants the *production-parse* admissible-run
histogram for a different question, this file is a starting sketch only and **must be rewritten
to consume the real mode-15 parse trace** — not run as-is. Any number it produces must be
labelled with its instrument's optimism, reported under the provenance rule in §15.2 of the
report (never spliced with the Linux-line figures), and charged against the frozen Windows cell
rather than the Linux one.

## Gate relevance

None remaining for FLI. The gates it was built for are void or KILL (§16.6, §16.7). The repaired
guard list in §16.3 — subtraction-form block-end guard, capped zigzag with `int64_t` drift and
two-sided post-checks, per-iteration `1 ≤ L_i ≤ LEN_MAX`, full-consumption extension for every
new substream, byte-alphabet representability (`K > 251`), post-accumulation escape bound,
periodic-vs-bulk split re-evaluated per iteration with `memcpy` forbidden on overlap, and a `ρ`
resource-meter row at parity with the tokens replaced — is the reusable artifact this prototype
directory leaves behind.
