# I10-1A reference-cost gate — silesia

**Ruling: COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE**

| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |
|---|---:|---:|---:|---:|
| ANVIL aux | 48,096,796 | 0.226937427 | 127.073 | 76.3 |
| xz -9e | 48,456,004 | 0.228632295 | 85.782 | 50.9 |
| Brotli q11/lw30 | 49,383,136 | 0.233006827 | 154.740 | 122.4 |

## Paired decode ratios — ANVIL / reference (lower is faster)

- vs xz: 0.6751, 95% CI [0.6542, 0.6756]
- vs Brotli: 1.2177, 95% CI [0.9955, 1.3126]
- A/A null CI: [0.9999, 1.0300] — PASS

## Existing cost-margin gates

- bytes < xz: PASS
- decode <= 2x xz (CI upper): PASS
- decode <= 2x Brotli (CI upper): PASS
- peak RSS <= 2x xz: PASS (1.499x)
- peak RSS <= 2x Brotli: PASS (0.623x)

This gate can falsify a FRONT-CROSSING on decode/memory cost. Passing it does not establish a crossing because encode and decoder-only binary-size axes are not ruled here.
