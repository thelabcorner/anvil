## ANVIL I20 C2 — auxiliary-index walk-budget sweep (`--bwt-aux-walks=N`)

Worker **W2**. Additive, default-preserving, wire-compatible change plus the
measurement tooling that sweeps it. **No novelty claim** — this is
`{adopt-class}` / engineering.

### What changes

| file | change |
|---|---|
| `src/anvil.cpp` | new CLI knob `--bwt-aux-walks=N` (1..1048576, default **1024** = today's behaviour, byte-identical output); `bwt_aux_rate_for_size()` generalised to take the walk budget; `--bwt-subblock` additionally accepts the spaced form `--bwt-subblock <bytes>` |
| `tools/i20_aux_walk_sweep.py` | new: byte / RSS / trade-curve sweep tool (prepare / rss / summarize) |
| `.github/workflows/anvil-i20-aux-walk-sweep.yml` | new: manual-only (cap x walk budget) grid sweep on the canonical corpora |
| `FORMAT.md` | one paragraph recording that the knob is **encoder policy, not wire-visible** |
| `docs/I20-RESULTS-W2-aux-walk-sweep.md` | the results, the trade curve, the fitted LF-walk share `f`, and the verdict |

### Why the wire is unaffected

The rev-2 mode-17 auxiliary-v2 payload already transmits the sampling rate `r`
and `auxiliary_count = 1 + floor((n-1)/r)`. The decoder validates only that
`r` is a power of two in `[2, transformed_size]`, that `auxiliary_count`
matches `1 + floor((transformed_size-1)/r)`, that
`auxiliary_count <= min(transformed_size, 1<<20)`, and that every index lies in
`[1, transformed_size]`. **No decoder rule ties the checkpoint count to 1024**,
so every `r` the new knob can select is a value the format already permits.
The default path is provably unchanged and is verified byte-identical per file
against the frozen `i10-aux-unbwt-final-v1` binary.

### Evidence

See `docs/I20-RESULTS-W2-aux-walk-sweep.md` for the full evidence record
(run URL, job, workflow file, commit SHA, artifact name and corpus for every
number).

### Verification performed

- per-file byte identity against the frozen I10-1A binary for the auxiliary-on,
  auxiliary-off and Brotli representations;
- the repo's own fuzz / registry path (`tests/fuzz.py`, registry coverage,
  forced-postcoder coverage, auxiliary-BWT assertions) on the changed binary;
- direct CLI-contract assertions for the new knob (both spellings, and the
  rejection of out-of-range values with an explicit error rather than a
  corrupt file);
- byte-exact round-trip on every corpus file for every arm in the sweep.
