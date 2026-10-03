# Q0 — Dense retained-artifact correction

**Date:** 2026-10-02  
**Coordinator ruling:** **Q2 REQUIRED**  
**Measurement class:** retained-artifact arithmetic only; no codec execution, timing, corpus benchmark, build, or network access.

## Inputs

- Frozen suite: `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv`
  - SHA-256: `AF44D9D3D40FB85C7E35758FD7936ADB562C8DB6FF455B30371AB355EBE7B65E`
  - 364 rows = 13 files × 28 codecs.
- Static audit script: `prototypes/swarm-2026-10-02/04-dense-frontier/space-bunny/parity_sweep.py`
  - SHA-256 at coordinator execution: `2117B5C2998F99F9908590AA4DB12EAF33AC4DF9BBCB5B88A14A192AE65FDBE6`

The coordinator executed the script directly after code inspection.

## Decisive retained-grid result

At the frozen ANVIL geometry represented by this CSV (default 256 KiB independent blocks), using the recorded classification order:

`DOMINATED -> DEGENERATE -> FRONT-GAP -> FRONT-CROSSING`

the full ANVIL ladder gives:

| class | cells |
|---|---:|
| DOMINATED | **435** |
| DEGENERATE | **28** |
| FRONT-GAP | **5** |
| FRONT-CROSSING | **0** |
| total | **468** |

The 468 cells close exactly: 18 ANVIL configurations × 13 files × 2 planes.

**0 / 18 ANVIL configurations produces even one FRONT-CROSSING cell.**

The five FRONT-GAP cells are:
- `anvil-mdl-rans`: 1
- `anvil-mdl-rans-l0`: 1
- `anvil-mdl-rans-l001`: 1
- `anvil-shape-rans`: 1
- `anvil-shape-rans-l0`: 1

This means the existing zero-crossing result is **not merely an artifact of choosing one token-lane ANVIL configuration inside the retained Class-A grid**.

## Why Q2 is still REQUIRED

The retained suite contains **no candidate block-size sweep**. The script itself correctly states that literal T3 (candidate block geometry) is not computable from the retained CSV:

- ANVIL Class-A rows were generated at the default `block_size = 256 KiB`.
- the retained reference rows use materially larger/whole-stream horizons;
- prior E4 measurements show independent-block restart/model-warmup penalties on the order of **5–17%** as blocks are reduced.

Therefore the retained artifact cannot answer the actual parity question:

> Would the ANVIL/reference frontier classification change when block/window geometry is made comparable?

That is precisely what Q2 must measure.

**Q2 is not an RSS/subblocking tuning experiment.** Its primary purpose is fair byte/frontier classification under explicit block/window geometry. RSS may be recorded descriptively, but it is not the reason Q2 survives.

## Non-decisive diagnostics deliberately excluded from the ruling

The audit script also treats Brotli, zstd, and xz ladder rows as candidate families against subsets of the other reference codecs and observes many "crossings." Those rows do **not** answer the Project Anvil question and are not used to justify Q2. They are reference-front diagnostics only.

Likewise, adding a material epsilon to the bracket predicate can relabel the five retained FRONT-GAP cells as "CROSSING" at epsilon >= 0.02. That is a **classification-definition sensitivity**, not newly measured frontier evidence. Q2 must use the project's frozen adopted predicate and may not manufacture a crossing by changing the semantic definition after seeing this sweep.

## Q2 preconditions established by Q0

A corrected Q2 must:

1. vary **candidate block/window geometry explicitly** and record it in the arm identity;
2. preserve the frozen reference/candidate source identities;
3. make complete bytes + round-trip hashes the citation-grade primary data;
4. predeclare the classification predicate, including DEGENERATE and FRONT-GAP handling;
5. report **all** cells; no best-cell selection;
6. treat hosted-runner throughput as scout/ranking-grade only;
7. never let a single timing cell emit a FRONT-CROSSING token;
8. use preselected timing endpoints or a family-level null procedure if timing is run;
9. keep RSS/binary-size claims out of cross-family dominance unless scopes are actually comparable.

## Final Q0 ruling

> **Q2 REQUIRED.**
>
> The retained 256 KiB grid remains **0 FRONT-CROSSING** across every available ANVIL configuration, but the grid cannot adjudicate the measured block/window asymmetry. The only unresolved substrate question is geometry parity, so Q2 should be a small, corrected parity experiment—not a rerun of the old 20-arm dense vehicle.
