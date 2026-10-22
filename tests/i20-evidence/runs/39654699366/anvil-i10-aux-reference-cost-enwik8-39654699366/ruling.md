# I10-1A reference-cost gate — enwik8

**Ruling: FRONT-GAP_COST**

| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |
|---|---:|---:|---:|---:|
| ANVIL aux | 23,537,422 | 0.235374220 | 20.300 | 598.9 |
| xz -9e | 24,831,648 | 0.248316480 | 70.506 | 66.1 |
| Brotli q11/lw30 | 24,810,180 | 0.248101800 | 162.185 | 250.7 |

## Paired decode ratios — ANVIL / reference (lower is faster)

- vs xz: 3.4733, 95% CI [3.4391, 3.5071]
- vs Brotli: 7.9859, 95% CI [7.3877, 7.9979]
- A/A null CI: [0.9900, 1.0103] — PASS

## Existing cost-margin gates

- bytes < xz: PASS
- decode <= 2x xz (CI upper): FAIL
- decode <= 2x Brotli (CI upper): FAIL
- peak RSS <= 2x xz: FAIL (9.059x)
- peak RSS <= 2x Brotli: FAIL (2.388x)

This gate can falsify a FRONT-CROSSING on decode/memory cost. Passing it does not establish a crossing because encode and decoder-only binary-size axes are not ruled here.
