# I10-1A reference-cost gate — silesia

**Ruling: COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE**

| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |
|---|---:|---:|---:|---:|
| ANVIL aux | 48,281,716 | 0.227809944 | 160.927 | 76.2 |
| xz -9e | 48,456,004 | 0.228632295 | 93.382 | 51.2 |
| Brotli q11/lw30 | 49,383,136 | 0.233006827 | 149.469 | 122.4 |

## Paired decode ratios — ANVIL / reference (lower is faster)

- vs xz: 0.5808, 95% CI [0.5463, 0.5933]
- vs Brotli: 0.9642, 95% CI [0.8387, 1.0366]
- A/A null CI: [0.8672, 1.0378] — PASS

## Existing cost-margin gates

- bytes < xz: PASS
- decode <= 2x xz (CI upper): PASS
- decode <= 2x Brotli (CI upper): PASS
- peak RSS <= 2x xz: PASS (1.489x)
- peak RSS <= 2x Brotli: PASS (0.623x)

This gate can falsify a FRONT-CROSSING on decode/memory cost. Passing it does not establish a crossing because encode and decoder-only binary-size axes are not ruled here.
