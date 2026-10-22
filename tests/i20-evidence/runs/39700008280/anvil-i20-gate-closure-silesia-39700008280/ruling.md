# I10-1A reference-cost gate — silesia

**Ruling: FRONT-GAP_COST**

| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |
|---|---:|---:|---:|---:|
| ANVIL aux | 46,593,971 | 0.219846575 | 50.187 | 201.7 |
| xz -9e | 48,456,004 | 0.228632295 | 69.027 | 51.0 |
| Brotli q11/lw30 | 49,383,136 | 0.233006827 | 198.588 | 122.2 |

## Paired decode ratios — ANVIL / reference (lower is faster)

- vs xz: 1.3596, 95% CI [1.3531, 1.3920]
- vs Brotli: 3.9624, 95% CI [3.9035, 4.0979]
- A/A null CI: [0.9999, 1.0123] — PASS

## Existing cost-margin gates

- bytes < xz: PASS
- decode <= 2x xz (CI upper): PASS
- decode <= 2x Brotli (CI upper): FAIL
- peak RSS <= 2x xz: FAIL (3.956x)
- peak RSS <= 2x Brotli: PASS (1.651x)

This gate can falsify a FRONT-CROSSING on decode/memory cost. Passing it does not establish a crossing because encode and decoder-only binary-size axes are not ruled here.
