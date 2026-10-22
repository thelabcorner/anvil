# I10-1A reference-cost gate — silesia

**Ruling: FRONT-GAP_COST**

| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |
|---|---:|---:|---:|---:|
| ANVIL aux | 46,466,339 | 0.219244363 | 48.990 | 201.8 |
| xz -9e | 48,456,004 | 0.228632295 | 70.122 | 54.1 |
| Brotli q11/lw30 | 49,383,136 | 0.233006827 | 259.389 | 123.6 |

## Paired decode ratios — ANVIL / reference (lower is faster)

- vs xz: 1.4316, 95% CI [1.4152, 1.4484]
- vs Brotli: 5.5297, 95% CI [5.3643, 5.6412]
- A/A null CI: [0.9886, 1.0118] — PASS

## Existing cost-margin gates

- bytes < xz: PASS
- decode <= 2x xz (CI upper): PASS
- decode <= 2x Brotli (CI upper): FAIL
- peak RSS <= 2x xz: FAIL (3.730x)
- peak RSS <= 2x Brotli: PASS (1.633x)

This gate can falsify a FRONT-CROSSING on decode/memory cost. Passing it does not establish a crossing because encode and decoder-only binary-size axes are not ruled here.
