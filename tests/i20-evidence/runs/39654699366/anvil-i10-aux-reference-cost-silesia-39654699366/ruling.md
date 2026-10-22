# I10-1A reference-cost gate — silesia

**Ruling: FRONT-GAP_COST**

| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |
|---|---:|---:|---:|---:|
| ANVIL aux | 46,466,339 | 0.219244363 | 48.452 | 248.3 |
| xz -9e | 48,456,004 | 0.228632295 | 71.344 | 54.1 |
| Brotli q11/lw30 | 49,383,136 | 0.233006827 | 244.517 | 122.3 |

## Paired decode ratios — ANVIL / reference (lower is faster)

- vs xz: 1.4726, 95% CI [1.4717, 1.4897]
- vs Brotli: 5.4547, 95% CI [5.1604, 5.6033]
- A/A null CI: [0.9891, 1.0112] — PASS

## Existing cost-margin gates

- bytes < xz: PASS
- decode <= 2x xz (CI upper): PASS
- decode <= 2x Brotli (CI upper): FAIL
- peak RSS <= 2x xz: FAIL (4.588x)
- peak RSS <= 2x Brotli: FAIL (2.030x)

This gate can falsify a FRONT-CROSSING on decode/memory cost. Passing it does not establish a crossing because encode and decoder-only binary-size axes are not ruled here.
