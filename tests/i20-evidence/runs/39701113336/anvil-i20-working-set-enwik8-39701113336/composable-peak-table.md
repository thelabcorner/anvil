# I20-W4 decode working-set: composable peak-RSS table

MEASURED columns come from the Actions artifact (`rss-by-file.csv`, /usr/bin/time -f %M per decode process). DERIVED columns are exact arithmetic over the MEASURED compressed sizes and the live-set model.

Reference peaks (MEASURED, prior run 35927623136): xz -9e = 54.3 MiB, Brotli q11/lgwin30 = 124.5 MiB.

## uncapped (128 MiB default)

| corpus | file | n | c | arm | peak MiB (MEASURED) | floor baseline MiB (DERIVED) | floor +C4 levers MiB (DERIVED) | vs xz | vs Brotli |
|---|---|---:|---:|---|---:|---:|---:|---:|---:|
| enwik8 | enwik8 | 100,000,000 | 23,537,422 | baseline | 598.9 | 594.7 | 476.8 | 11.030 | 4.811 |
| enwik8 | enwik8 | 100,000,000 | 23,537,422 | candidate | 481.1 | 594.7 | 476.8 | 8.860 | 3.864 |

## cap 16 MiB

| corpus | file | n | c | arm | peak MiB (MEASURED) | floor baseline MiB (DERIVED) | floor +C4 levers MiB (DERIVED) | vs xz | vs Brotli |
|---|---|---:|---:|---|---:|---:|---:|---:|---:|
| enwik8 | enwik8 | 100,000,000 | 24,810,212 | baseline | 251.7 | 275.9 | 236.8 | 4.635 | 2.021 |
| enwik8 | enwik8 | 100,000,000 | 24,810,212 | candidate | 157.9 | 275.9 | 236.8 | 2.907 | 1.268 |

## cap 8 MiB

| corpus | file | n | c | arm | peak MiB (MEASURED) | floor baseline MiB (DERIVED) | floor +C4 levers MiB (DERIVED) | vs xz | vs Brotli |
|---|---|---:|---:|---|---:|---:|---:|---:|---:|
| enwik8 | enwik8 | 100,000,000 | 24,810,212 | baseline | 251.7 | 243.9 | 212.8 | 4.635 | 2.021 |
| enwik8 | enwik8 | 100,000,000 | 24,810,212 | candidate | 157.7 | 243.9 | 212.8 | 2.905 | 1.267 |

## cap 4 MiB

| corpus | file | n | c | arm | peak MiB (MEASURED) | floor baseline MiB (DERIVED) | floor +C4 levers MiB (DERIVED) | vs xz | vs Brotli |
|---|---|---:|---:|---|---:|---:|---:|---:|---:|
| enwik8 | enwik8 | 100,000,000 | 24,810,212 | baseline | 251.7 | 227.9 | 200.8 | 4.635 | 2.022 |
| enwik8 | enwik8 | 100,000,000 | 24,810,212 | candidate | 157.7 | 227.9 | 200.8 | 2.905 | 1.267 |

