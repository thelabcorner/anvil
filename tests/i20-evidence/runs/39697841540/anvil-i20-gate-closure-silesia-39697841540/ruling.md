# I10-1A reference-cost gate — silesia

**Ruling: COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE**

| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |
|---|---:|---:|---:|---:|
| ANVIL aux | 48,281,716 | 0.227809944 | 135.107 | 78.0 |
| xz -9e | 48,456,004 | 0.228632295 | 70.129 | 54.1 |
| Brotli q11/lw30 | 49,383,136 | 0.233006827 | 259.218 | 122.3 |

## Paired decode ratios — ANVIL / reference (lower is faster)

- vs xz: 0.5191, 95% CI [0.5189, 0.5198]
- vs Brotli: 1.9188, 95% CI [1.9156, 1.9210]
- A/A null CI: [0.9694, 1.0001] — PASS

## Existing cost-margin gates

- bytes < xz: PASS
- decode <= 2x xz (CI upper): PASS
- decode <= 2x Brotli (CI upper): PASS
- peak RSS <= 2x xz: PASS (1.441x)
- peak RSS <= 2x Brotli: PASS (0.638x)

This gate can falsify a FRONT-CROSSING on decode/memory cost. Passing it does not establish a crossing because encode and decoder-only binary-size axes are not ruled here.
