# arch I9 — store-path anomaly root cause (CRC fast path)

Bench flagged the frozen canonical (src BDC90474 / build\anvil.exe 72D65150) store
path measuring 5-6x faster than the prior grid (325-380 -> 1950-2184 MB/s encode)
with byte-identical output. Research-gate asked: owning change vs protocol
artifact. Resolved by a 3-way isolation window (w-arch-storepath-20260912T1300Z).

## 3-way isolation (identical build recipe; ONLY crc32 implementation differs)

Interleaved, reps=5, core18, threads=1, per-rep ns/B; `wire_fnv` identical across
arms on each file (byte-identity anchor). Log
`prototypes/i9-arch/logs/store_ab_w-arch-storepath.log`.

| arm | random.bin enc ns/B (MB/s) | random.bin dec ns/B (MB/s) | synth-arith enc | synth-arith dec |
|---|---|---|---|---|
| bytewise (51C6A939) | 2.4258 (412) | 2.7611 (362) | 2.3898 (418) | 2.7508 (364) |
| slicing-by-8 (B743061C) | 0.8652 (1156) | 1.1261 (888) | 0.9020 (1109) | 1.2301 (813) |
| PCLMUL (4F00627B) | 0.5341 (1872) | 0.8221 (1216) | 0.4969 (2012) | 0.8723 (1147) |

- The bytewise arm reproduces the PRIOR grid's store-path band (~410 enc / ~370
  dec MB/s); the PCLMUL arm reproduces the FROZEN grid's band. So the jump is the
  CRC implementation, not the measurement protocol.
- Owning hunks: S6-1b leg 1 (bytewise -> slicing-by-8, src comment ~229-258) and
  the stage-2 PCLMUL dispatcher (`crc32_slice8`, `crc32_has_pclmul`,
  `crc32_pclmul`, `crc32`) added in the dec-crc leg.
- Mechanism: store/raw block write = `crc32(block)` + copy (src 4584-4586,
  4591-4596, 4695-4699); decode verifies `crc32(block)` after reconstructing
  (src ~4720/4760). On a store block the checksum dominated both planes
  (bytewise ~1.95-2.8 ns/B measured vs PCLMUL ~0.5-0.9).
- Byte identity: the IEEE CRC-32 VALUE is unchanged (16,529 unit + 5,784 corpus
  checks, 0 fail; 494/494 wire gate), so `wire_fnv` and every container byte match.
- Separately, reference codec rows (brotli/zstd) also moved between the old and
  frozen grids (e.g. zstd-9 random decode 1,990 -> 25,206 MB/s); that is a
  harness/load effect of the older grid and is NOT part of this claim.

## Verdict

Store-path throughput is citable as a wire-invisible encode+decode speed win of
the CRC leg: 4.5x encode / 3.4x decode (PCLMUL vs bytewise) on the store path,
2.8x / 2.5x (slicing-by-8 vs bytewise). No ratio change, no wire change, no new
mode. The 28 DEGENERATE ratio=1.000 rows' throughput axis is explained.
