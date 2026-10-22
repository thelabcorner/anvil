# I10-1A reference-cost gate — silesia

**Ruling: COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE**

| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |
|---|---:|---:|---:|---:|
| ANVIL aux | 48,281,716 | 0.227809944 | 144.313 | 78.1 |
| xz -9e | 48,456,004 | 0.228632295 | 73.857 | 54.1 |
| Brotli q11/lw30 | 49,383,136 | 0.233006827 | 276.321 | 122.3 |

## Paired decode ratios — ANVIL / reference (lower is faster)

- vs xz: 0.5117, 95% CI [0.5106, 0.5123]
- vs Brotli: 1.9142, 95% CI [1.9137, 1.9158]
- A/A null CI: [0.9678, 1.0013] — PASS

## Existing cost-margin gates

- bytes < xz: PASS
- decode <= 2x xz (CI upper): PASS
- decode <= 2x Brotli (CI upper): PASS
- peak RSS <= 2x xz: PASS (1.444x)
- peak RSS <= 2x Brotli: PASS (0.639x)

This gate can falsify a FRONT-CROSSING on decode/memory cost. Passing it does not establish a crossing because encode and decoder-only binary-size axes are not ruled here.
