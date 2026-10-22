# I10-1A reference-cost gate — silesia

**Ruling: COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE**

| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |
|---|---:|---:|---:|---:|
| ANVIL aux | 48,281,716 | 0.227809944 | 131.056 | 76.3 |
| xz -9e | 48,456,004 | 0.228632295 | 79.393 | 51.2 |
| Brotli q11/lw30 | 49,383,136 | 0.233006827 | 149.266 | 122.4 |

## Paired decode ratios — ANVIL / reference (lower is faster)

- vs xz: 0.6058, 95% CI [0.5948, 0.6059]
- vs Brotli: 1.1390, 95% CI [1.0989, 1.1808]
- A/A null CI: [0.9997, 1.0003] — PASS

## Existing cost-margin gates

- bytes < xz: PASS
- decode <= 2x xz (CI upper): PASS
- decode <= 2x Brotli (CI upper): PASS
- peak RSS <= 2x xz: PASS (1.490x)
- peak RSS <= 2x Brotli: PASS (0.623x)

This gate can falsify a FRONT-CROSSING on decode/memory cost. Passing it does not establish a crossing because encode and decoder-only binary-size axes are not ruled here.
