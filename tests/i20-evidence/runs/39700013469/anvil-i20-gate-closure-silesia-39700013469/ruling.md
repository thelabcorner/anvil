# I10-1A reference-cost gate — silesia

**Ruling: TIMING_BLOCKED**

| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |
|---|---:|---:|---:|---:|
| ANVIL aux | 48,281,716 | 0.227809944 | 119.872 | 76.1 |
| xz -9e | 48,456,004 | 0.228632295 | 70.189 | 51.0 |
| Brotli q11/lw30 | 49,383,136 | 0.233006827 | 198.546 | 122.2 |

## Paired decode ratios — ANVIL / reference (lower is faster)

- vs xz: 0.5856, 95% CI [0.5853, 0.5859]
- vs Brotli: 1.6568, 95% CI [1.6539, 1.6578]
- A/A null CI: [0.9995, 1.0000] — BLOCKED

## Existing cost-margin gates

- bytes < xz: PASS
- decode <= 2x xz (CI upper): PASS
- decode <= 2x Brotli (CI upper): PASS
- peak RSS <= 2x xz: PASS (1.493x)
- peak RSS <= 2x Brotli: PASS (0.623x)

This gate can falsify a FRONT-CROSSING on decode/memory cost. Passing it does not establish a crossing because encode and decoder-only binary-size axes are not ruled here.
