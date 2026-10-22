# I10-1A reference-cost gate — silesia

**Ruling: COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE**

| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |
|---|---:|---:|---:|---:|
| ANVIL aux | 48,281,716 | 0.227809944 | 174.139 | 78.3 |
| xz -9e | 48,456,004 | 0.228632295 | 93.385 | 54.2 |
| Brotli q11/lw30 | 49,383,136 | 0.233006827 | 173.873 | 123.8 |

## Paired decode ratios — ANVIL / reference (lower is faster)

- vs xz: 0.5246, 95% CI [0.5140, 0.5365]
- vs Brotli: 1.0408, 95% CI [1.0375, 1.0772]
- A/A null CI: [0.9597, 1.0428] — PASS

## Existing cost-margin gates

- bytes < xz: PASS
- decode <= 2x xz (CI upper): PASS
- decode <= 2x Brotli (CI upper): PASS
- peak RSS <= 2x xz: PASS (1.445x)
- peak RSS <= 2x Brotli: PASS (0.632x)

This gate can falsify a FRONT-CROSSING on decode/memory cost. Passing it does not establish a crossing because encode and decoder-only binary-size axes are not ruled here.
