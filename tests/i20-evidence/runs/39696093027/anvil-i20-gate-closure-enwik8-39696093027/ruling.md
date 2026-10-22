# I10-1A reference-cost gate — enwik8

**Ruling: FRONT-GAP_COST**

| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |
|---|---:|---:|---:|---:|
| ANVIL aux | 23,537,422 | 0.235374220 | 28.401 | 481.2 |
| xz -9e | 24,831,648 | 0.248316480 | 89.471 | 66.2 |
| Brotli q11/lw30 | 24,810,180 | 0.248101800 | 122.485 | 250.9 |

## Paired decode ratios — ANVIL / reference (lower is faster)

- vs xz: 3.1502, 95% CI [3.0148, 3.3013]
- vs Brotli: 4.3146, 95% CI [4.3077, 4.6013]
- A/A null CI: [0.9995, 1.0145] — PASS

## Existing cost-margin gates

- bytes < xz: PASS
- decode <= 2x xz (CI upper): FAIL
- decode <= 2x Brotli (CI upper): FAIL
- peak RSS <= 2x xz: FAIL (7.272x)
- peak RSS <= 2x Brotli: PASS (1.918x)

This gate can falsify a FRONT-CROSSING on decode/memory cost. Passing it does not establish a crossing because encode and decoder-only binary-size axes are not ruled here.
