# ANVIL I14 — Exact AVI3 wire-cost findings

**Date:** 2026-10-07. **Run:** https://github.com/thelabcorner/anvil/actions/runs/37699298906 — all steps PASS. **Artifact:** https://github.com/thelabcorner/anvil/actions/runs/37699298906/artifacts/11516856472, SHA-256 4c8314e0e8d7f6a5696647c5f4e60329c8393c4f0a4b3af0e69c511422d2ca49.
**Source:** frozen I13 SHA 274be4c8a5de222b4bbb16c586fc04310dd1deadd455dacc7741110b8dfcb7c0, analyzer SHA f6a14a466fb85678274e3c037dbc96d6eaf63e7caff14f13b0b5d3b9987e7c57.
**Role:** deterministic byte-conservation diagnostic only, not an alternate codec, novelty or Class-A frontier claim.

## Actual cost of 139,387 AVI3 bytes on the 280 KB synthetic time-series

| Disjoint wire component | Bytes |
|---|---:|
| 69 block mode tags | 69 |
| 69 block-length varints | 138 |
| 69 stride values | 69 |
| 483 lane mode tags | 483 |
| 407 modeled bases | 1,628 |
| 407 modeled slopes | 1,628 |
| 407 modeled bit widths | 407 |
| Modeled-lane packed residuals | 90,390 |
| Literal RAW column bytes | 44,008 |
| Partial-record tails | 560 |
| Outer magic and length | 7 |
| **All bytes conserved** | **139,387** |

All three preregistered wires passed exact conservation and frozen length checks: time-series 139,387 B, arithmetic 62,256 B, repeated JSONL 916,835 B. I13 source roundtrips, I14 parsing invariants and artifact upload succeeded, entirely on GitHub Actions.

## Frozen microtile-width capacity (hypothesis, not encoded bytes)

For each already-modeled lane, residuals were regrouped mathematically into fixed-size microtiles, charging one full byte for each tile width and exact pad bytes. Signed delta if *all* fields tiled versus an adaptive choice of flat/tiled:

| Values per tile | All-fields signed delta | Adaptive positive-only capacity | Fields benefiting |
|---|---:|---:|---:|
| 16 | +6,859 B | **+7,704 B** | 289 |
| 32 | +6,755 B | +7,244 B | 286 |
| 64 | +5,051 B | +5,480 B | 235 |
| 128 | +1,220 B | +1,704 B | 89 |

The I13 global width byte disappears for tiled lanes if a new lane mode reuses the existing tag. This optional additional byte is *not included* in the 7,704-B projection. We must implement the exact AVI4 wire and decoder before claiming any saving or speed.

## Consequence

I13's 4,669-B deficit to Brotli q11 (134,718 B) appears recoverable with a conservative, well-understood 16-value grouping technique. However Brotli **q5 yielded 120,594 B**, so the more demanding compressed-byte target is an **18,793 B deficit** to q5. A projected 7,704 B cannot clear this larger gap on its own. Reaching the true frontier calls for subsequent raw-column and/or residual coding improvements, not merely claiming victory over the weaker reference setting.

**Decision:** proceed with separately frozen I15 AVI4 codec using COL lane mode 2: exact fixed 16-value tiles, no altered predictor, actual complete cost selection, fully functional low-allocation decoder, and matched q5/q11 reference on GitHub Actions. If the hypothesis fails, preserve I13 and I14 evidence unchanged.

**Scope:** seven already-consumed discovery fixtures; Gate B still blocked; no new novelty, heldout generalization, or certified general-purpose crossing.
