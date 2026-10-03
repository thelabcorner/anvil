# Q2 parity pilot — isolated prototype (NOT DISPATCHABLE)

**Status: prepared, not dispatched. No codec has been run. `.github/` is untouched.**

Design authority: `docs/swarm-2026-10-02/Q2-PARITY-PLAN-SPACE-BUNNY.md` (Rev 3).

## What this directory is

Bounded, isolated tooling for the Q2 measurement-parity job. Nothing here is wired into
`src/`, `tools/`, `tests/`, or `.github/`. Heavy execution is GitHub Actions only
(`MASTER-BRIEF.md` §7).

## What Q2 measures

Bytes. **Only bytes classify.** The published quantities are:

The design is a **complete, crossed 2×2** in two binary factors — geometry
{small, large} × incompressibility gate {on, off} — so **every main effect holds the other
factor exactly fixed**:

| contrast | factor varied | factor held fixed |
|---|---|---|
| `geom_effect_gate_on  = G1 − G0` | geometry | gate **ON** |
| `geom_effect_gate_off = G2 − G3` | geometry | gate **OFF** |
| `gate_effect_at_small  = G0 − G3` | gate | geometry **small** |
| `gate_effect_at_large  = G1 − G2` | gate | geometry **large** |
| `interaction = (G1 − G2) − (G0 − G3) == (G1 − G0) − (G2 − G3)` | — | tests whether the **geometry effect depends on gate state** |

**No contrast is confounded.** Within `G0 − G3` and within `G1 − G2` the block size — and
therefore the block count — is *identical by construction* (`G0`/`G3` are both 262,144 B;
`G1`/`G2` are both 67,108,864 B). Within `G1 − G0` and within `G2 − G3` the gate state is
identical by construction. Each contrast is a clean main effect of one factor at a fixed
level of the other.

**Naming discipline (binding).** `G0 − G3` and `G1 − G2` are **gate** effects. They are never
described as window effects, geometry effects, or geometry controls: the factor they vary is
the incompressibility router (`src/anvil.cpp:4685`), not block geometry. They are the same
factor estimated at two levels of geometry, and **their difference is the interaction**.

**Rejected claim.** Any later red-team assertion that `G0 − G3` is a *"pure window effect"* —
or any window, reach, or geometry effect — is **rejected on the design's face**: `G0` and `G3`
share an identical block size, so that contrast does not vary geometry at all. Equally
rejected is the converse error, that only the interaction holds a factor fixed; `G1 − G0` and
`G2 − G3` each already hold the gate fixed.

## Outputs

1. **Raw exact contrasts** — the four contrasts plus the interaction, exact bytes, per
   (configuration, file) and as corpus sums, plus relative percentages against `bytes(G0)` and
   threshold-free sign labels.
2. **Round-trip / provenance validity** — `roundtrip_verified` by byte-compare (never exit
   code), `complete_bytes`, `compressed_sha256`, `roundtrip_sha256` vs `source_sha256`,
   `evidence_role`, `identity_role`, the exact `argv_encode` per cell.
3. **Retained-twin fidelity** — the retained grid has exactly two distinct byte values across
   all five configurations on every file (the three `anvil-mdl-rans*` rows are byte-identical
   to each other; the two `anvil-shape-rans*` rows likewise). Whether that identity survives at
   each geometry is measured and reported with `decisional: false`.

Q2 attaches **no verdict** to any of them.

## What Q2 must never do

* **Never emit any frontier class** — `FRONT-CROSSING`, `FRONT-GAP`, `DOMINATED`, `DEGENERATE`,
  `BASELINE-CROSSING-CANDIDATE`, or anything else in that vocabulary. The frontier predicate is
  **not run at all**: no dominance test, no bracket test, no classification order.
  `q2_factorial.py --assert-no-frontier-vocabulary` fails the job if any frontier token appears in
  any artifact.
* **Never apply an epsilon.** No `eps` parameter exists — there is no bracket predicate to tune.
* **Never apply a materiality threshold.** `materiality_threshold: null` is carried in every
  manifest. The 11 B/block framing quantum appears only as a descriptive annotation of
  block-count change.
* **Never mint a label, a void trigger, or a classification from a published number.** A
  non-zero interaction is a *measured* geometry × gate interaction, not an invalidation. Opposite
  `D_anvil`/`D_ref` signs are a *descriptive divergence*. With only two geometry levels, a
  larger-block byte **increase** is still a valid measured geometry effect and voids nothing.
  **Interpretation belongs to the coordinator after measurement.**
* **Never pool Linux build outputs as retained bytes.** `corpus_totals` is role-stratified
  (A tracked / B build-output-under-test / C explicitly mixed); the two build-output slots are
  fresh artifacts, not the historical Windows objects the retained grid measured.
* **Never classify on timing, parallelism, or a reference codec.** Those axes are recorded,
  labelled `*_classifying: false`, and enter no contrast.
* **Never collapse the five retained configurations.** The retained FRONT-GAP set spans
  `stream_lambda` ∈ {0.04, 0.00, 0.01} across {mdl, shape}; `q2_arms.py` asserts the span and
  refuses a duplicate `(parse, lambda)` pair.
* **Never let the environment define identity.** `ANVIL_STREAM_LAMBDA` is read at
  `src/anvil.cpp:4959`; Q2 asserts it is **unset** (`q2_arms.py --check-env`) and the collector
  strips it from the child environment, so identity is argv-determined.
* **Never load the whole `CHECKSUMS.txt` as the population.** The 13-file population is derived
  from the frozen Class-A CSV and verified fail-closed against `CHECKSUMS.txt`
  (`q2_arms.py --population-report`), which supplies identities only.
* **Never build or invoke a codec on unverified production blobs.** The workflow asserts six
  measurement-critical git blob ids — including `tests/corpus/CHECKSUMS.txt`, on which the
  population and hashes depend — against the coordinator-validated base **before** the build;
  mismatch ⇒ `INVALID_INFRA` with no codec invocation.
* **Never add a push trigger to work around manual dispatch.** The workflow is
  `workflow_dispatch` only. An undispatchable staging workflow is a **HOLD prerequisite**, not a
  reason to auto-trigger.

## Files

| file | role |
|---|---|
| `q2_arms.py` | arm registry: 5 retained configurations × 4 cells + 2 reference arms; frozen contrast algebra; the machine-readable `retained_config_id → argv` map; identity gates |
| `q2_brotli_geom.cpp` | reference helper: explicit `lgwin`, whole-file (R1) or N independent streams (R3); declared container framing |
| `q2_collect.py` | runs every arm, records complete bytes + compressed/round-trip SHA-256 + descriptive rows. **Plan-only unless `--execute`** |
| `q2_factorial.py` | contrast computation, reference residual, and the G0 baseline diagnostic (adopted predicate, capped vocabulary) |
| `anvil-q2-parity.DRAFT.yml` | workflow **draft**. `workflow_dispatch` only, `contents: read`, no push trigger. Lives here on purpose; installing it is a coordinator action, and dispatch stays on HOLD until the path is tracked/approved |

## Bounded local validation (no codec, no corpus compression, no timing)

```bash
python q2_arms.py --list-arms            # registry, contrast algebra, identity gates
python q2_arms.py --verify-identity      # interaction identity, exact integers
python q2_arms.py --emit-retained-map    # retained_config_id -> argv, machine-readable
python q2_arms.py --check-env            # ANVIL_STREAM_LAMBDA must be unset
python q2_arms.py --population-report --plan <corpus-dir> --frozen-suite <suite.csv>
python q2_factorial.py --self-test       # algebra + fidelity unit tests, fixtures only
python q2_collect.py --corpus <dir> --frozen-suite <csv> --out rows.jsonl   # PLAN ONLY
python q2_factorial.py --assert-no-frontier-vocabulary rows.jsonl contrasts.json
```

`--population-report` derives the 13-file population from the frozen suite and verifies it against
`CHECKSUMS.txt`; it exits non-zero on any mismatch. `--frozen-suite` is **required** wherever the
population is needed — `CHECKSUMS.txt` supplies identities, never the population.

`q2_factorial.py` performs pure arithmetic over the collector's rows and the retained CSV. That is the
same measurement class the coordinator's own Q0 ruling permits ("retained-artifact arithmetic only; no
codec execution, timing, corpus benchmark, build, or network access").

**Never run `q2_collect.py --execute` locally.** That is the one line in this directory that
invokes a codec, and it is reserved for GitHub Actions.

## Configuration identity — the load-bearing detail

The retained Class-A rows were produced by `tools/bench_native.cpp`, which sets
`anvil::g_stream_lambda` **directly** (`:24`, `:30`) rather than through the CLI, and whose
harness default is **0.04** while the CLI default is **0.01** (`src/anvil.cpp:3783`).
`src/anvil.cpp:4662` then propagates `opt.stream_lambda` into the `g_stream_lambda` global
(`:1471`) that actually weights the stream suite.

Consequences, all enforced in code:

* each of the five configurations passes **its own** lambda explicitly;
* the bench-time defaults the harness leaves unset (`--boundary`, `--channels`, `--pnra`,
  `--hotop-rlzp`, `--hotop-budget`, `--stream-suite`, `--chain`, `--max-match`, `--surprise`)
  are pinned explicitly so a future default change cannot silently alter identity;
* `ANVIL_STREAM_LAMBDA` must be absent.
