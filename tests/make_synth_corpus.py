#!/usr/bin/env python3
"""Synthetic structural-regularity corpus (Iteration-5 prerequisite).

Experiment T (SRR probe) and Experiment U (finite-difference invariant) both
found near-zero periodic/arithmetic signal in tests/corpus/ — but that corpus
was never designed to have tight, low-noise periodic or arithmetic structure
(json/jsonl/log/sqlite are semi-structured text, not fixed-stride binary
records). This script generates three small, deterministic, purpose-built
files with the specific structure those mechanisms target, so a retest can
distinguish "mechanism doesn't work" from "corpus has nothing to find".

Usage: python tests/make_synth_corpus.py tests/corpus
All output is fully deterministic (fixed seeds) and reproducible byte-for-byte.

Iteration-7 additions (swarm-i7-cost, corpus-expand) — three MORE
structure-carrying files, stronger/different than the first three:
  synth-columnar-align.bin  columnar arithmetic table, row-interleaved, all
                            fields misaligned (prime 23 B row stride)
  synth-drift-stride.bin    fixed-width records whose stride DRIFTS
                            systematically (sawtooth 40..48 B), not randomly
  synth-counters.log        counter/timestamp-heavy TEXT log (monotonic
                            counters at several periods, page-stepping hex
                            addresses) — strong arithmetic signal in text
Post-I7 Gorilla/vXOR sweet-spot substrate (coordinator research briefing):
  synth-telemetry-f64.bin   smooth f64 sensor telemetry, exact cadence
                            (DoD '0' timestamps + Case A/B1 value XOR)
  synth-ndjson-columnar.ndjson mostly-stable-column NDJSON (vXOR trigger,
                            variable line length for honest frame detection)
The I7 files use only integer math + fixed seeds (no floats in output, no
dict iteration); the two Gorilla/vXOR files additionally use IEEE-754 basic
float ops and correctly-rounded formatting only — byte-stable across
platforms/Python versions (verified by double-run hash compare).
"""
from pathlib import Path
import argparse, random, struct

ap = argparse.ArgumentParser()
ap.add_argument('out', nargs='?', default='tests/corpus')
a = ap.parse_args()
root = Path(a.out)
root.mkdir(parents=True, exist_ok=True)

# ---------------------------------------------------------------------------
# synth-timeseries.bin: fixed-stride binary sensor-log records, 14 B each,
# N=20000 records (280,000 B). struct { u64 ts_ms (LE, +1000 +/- jitter per
# record); f32 value (LE, small random walk); u16 id (cyclic 0..999) }.
# This is exactly the "record-period + per-field small delta" shape the SRR
# probe (mode 13 / --channels=on) and the finite-difference invariant both
# target: tight fixed period (14 B, zero drift) and each field individually
# forms a near-arithmetic (ts) or near-constant-delta (value, id) sequence.
# ---------------------------------------------------------------------------
r = random.Random(90001)
ts = 1_700_000_000_000
val = 0.0
with (root / 'synth-timeseries.bin').open('wb') as f:
    for i in range(20000):
        ts += 1000 + r.randint(-50, 50)
        val += r.uniform(-0.1, 0.1)
        rec_id = i % 1000
        f.write(struct.pack('<QfH', ts, val, rec_id))

# ---------------------------------------------------------------------------
# synth-arith.bin: columnar arithmetic progressions with small additive
# noise, 8 interleaved columns x 8000 u32 values each (256,000 B), each
# column its own base + i*stride (+/- small noise), columns concatenated
# block-wise (not interleaved per-row) so the finite-difference invariant has
# a long, unbroken run of near-constant first differences per column to find
# — the direct analogue of counter/timestamp/row-id fields Experiment U's
# dense finite-diff family targets, deliberately made strong instead of
# incidental.
# ---------------------------------------------------------------------------
r = random.Random(90002)
with (root / 'synth-arith.bin').open('wb') as f:
    for col in range(8):
        base = r.randint(0, 1 << 20)
        stride = r.randint(1, 97)
        v = base
        for i in range(8000):
            v += stride + r.randint(-1, 1)  # near-constant first difference
            f.write(struct.pack('<I', v & 0xFFFFFFFF))

# ---------------------------------------------------------------------------
# synth-jitter.bin: near-duplicate 64-byte records at a JITTERED (not fixed)
# stride — Experiment T noted jsonl record periods drift +/-2 bytes; this
# isolates that specific condition in binary form so the SRR probe's
# synchronized drift-window logic (built, but never fairly exercised — the
# real corpus's drift was incidental, not a deliberately strong test) has a
# genuine periodic-with-jitter signal to lock onto. Record: a 64-byte mostly-
# identical skeleton (2 fixed marker bytes + 54 near-constant payload bytes
# that change in only a few positions) with 0-2 random pad bytes inserted
# before each record, so consecutive record starts drift by a small amount
# around a ~64-66 B mean period while byte-for-byte record content still
# recurs almost exactly.
# ---------------------------------------------------------------------------
r = random.Random(90003)
skeleton = bytearray(b'\xAB\xCD' + bytes(range(54)) + b'\x00\x00\x00\x00\x00\x00\x00\x00')
assert len(skeleton) == 64
with (root / 'synth-jitter.bin').open('wb') as f:
    for i in range(15000):
        pad = r.randint(0, 2)
        if pad:
            f.write(bytes(r.randrange(256) for _ in range(pad)))
        rec = bytearray(skeleton)
        # a handful of bytes vary per record (record-local counter + noise),
        # rest of the 64-byte skeleton recurs exactly like the real corpus's
        # mostly-identical jsonl records.
        rec[58:62] = struct.pack('<I', i)
        rec[62] = r.randrange(256)
        f.write(bytes(rec))

# ---------------------------------------------------------------------------
# synth-columnar-align.bin (Iteration-7): a columnar arithmetic table stored
# ROW-interleaved with every field misaligned — the realistic "sensor frame /
# table page" shape (synth-arith.bin stores columns block-wise, which is the
# easy case). Row = 23 bytes (prime stride => no field is naturally aligned):
#   off  0  u16 row_id     AP stride 1
#   off  2  u32 seq        AP stride 7
#   off  6  u64 ts_ns      AP stride 1000
#   off 14  u32 temp_x100  AP stride 3 + noise +/-1
#   off 18  u16 volt       AP stride 2
#   off 20  u8  status     cyclic 0..7
#   off 21  u8  flags      0xA5, flips to 0x5A on every 257th row
#   off 22  u8  parity     XOR of all preceding field bytes (cross-field
#                          linear structure: derivable, not independent)
# A finite-difference / columnar mechanism that finds the true field offsets
# gets long near-constant-delta runs per field; one that scans wrong offsets
# gets nothing. 12,000 rows x 23 B = 276,000 B.
# ---------------------------------------------------------------------------
r = random.Random(90004)
ts_ns = 1_700_000_000_000_000_000
temp = 215_000          # 21.50 degC x100
volt = 3_300            # centivolt
with (root / 'synth-columnar-align.bin').open('wb') as f:
    for i in range(12000):
        row_id = i & 0xFFFF
        seq = (7 * (i + 1)) & 0xFFFFFFFF
        ts_ns += 1_000_000                      # +1 us per row
        temp += 3 + r.randint(-1, 1)
        volt += 2
        status = i % 8
        flags = 0x5A if (i % 257) == 256 else 0xA5
        fields = struct.pack('<HIQIHB', row_id, seq, ts_ns & 0xFFFFFFFFFFFFFFFF,
                             temp & 0xFFFFFFFF, volt & 0xFFFF, status)
        parity = flags
        for b in fields:
            parity ^= b
        f.write(fields + bytes([flags, parity]))

# ---------------------------------------------------------------------------
# synth-drift-stride.bin (Iteration-7): fixed-width records whose STRIDE
# DRIFTS SYSTEMATICALLY over the file — a sawtooth ramp, not random jitter
# (synth-jitter.bin already isolates random 0-2 B pads). Pad before record i
# is (i // 96) % 9 bytes of a visible filler byte, so the record period ramps
# 40 -> 48 B and snaps back every 864 records; any fixed-period assumption
# breaks while content stays highly structured. Payload (40 B):
#   off  0  u64 ts_ms      AP stride 500
#   off  8  u32 counter    +1 per record
#   off 12  u16 channel    cyclic 0..63
#   off 14  24 B skeleton  near-constant pattern, two positions tick slowly
#   off 38  u16 crc16      deterministic checksum of the payload
# 14,000 records, mean stride 44 B => ~616,000 B.
# ---------------------------------------------------------------------------
r = random.Random(90005)
ts_ms = 1_700_000_000_000
skeleton = bytearray(bytes(range(24)))
def _crc16(buf):
    crc = 0xFFFF
    for b in buf:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc & 0xFFFF
with (root / 'synth-drift-stride.bin').open('wb') as f:
    for i in range(14000):
        pad_len = (i // 96) % 9
        if pad_len:
            f.write(bytes([0x80 | (i & 0x7F)]) * pad_len)
        ts_ms += 500
        rec = bytearray(40)
        struct.pack_into('<QIH', rec, 0, ts_ms & 0xFFFFFFFFFFFFFFFF, i, i % 64)
        sk = bytearray(skeleton)
        sk[10] = (i // 64) & 0xFF               # slow ticks inside skeleton
        sk[20] = (i // 512) & 0xFF
        rec[14:38] = sk
        struct.pack_into('<H', rec, 38, _crc16(rec[:38]))
        f.write(bytes(rec))

# ---------------------------------------------------------------------------
# synth-counters.log (Iteration-7): counter/timestamp-heavy TEXT log — the
# text analogue of the binary structural files. Experiments T/U/W found the
# existing text corpus's periodic/arithmetic signal at the hash-noise floor;
# this line format carries SEVERAL simultaneous monotonic sequences so an
# arithmetic/periodic mechanism has real signal in text form:
#   - ISO-8601 timestamp ticking +37 ms per line (integer ms math only)
#   - seq (+1/line), tick (+1000/line)
#   - addr: hex pointer stepping +0x40 within each of 8 modules (page walk)
#   - lat: seeded bounded random walk (integers only)
#   - qd: cyclic 0..31 ; crc: deterministic mix of seq/tick
# All formatting is integer-only (no floats) => byte-stable across platforms.
# 11,000 lines x ~110 B ~= 1.2 MB.
# ---------------------------------------------------------------------------
import datetime
r = random.Random(90006)
base_secs = 1_786_615_200               # 2026-08-13T10:00:00Z
lat = 850
lines = []
for i in range(11000):
    cur_ms = base_secs * 1000 + 37 * i
    secs, ms = divmod(cur_ms, 1000)
    dt = datetime.datetime.fromtimestamp(secs, tz=datetime.timezone.utc)
    mod = i % 8
    addr = 0x7FF600000000 + mod * 0x100000 + (i // 8) * 0x40
    lat += r.randint(-40, 60)
    if lat < 1:
        lat = 1
    elif lat > 9999:
        lat = 9999
    qd = (i * 7) % 32
    crc = ((i + 1) * 2654435761 ^ (1000 * (i + 1))) & 0xFFFF
    lines.append(
        f'{dt:%Y-%m-%dT%H:%M:%S}.{ms:03d}Z INFO mod{mod} seq={i + 1} '
        f'tick={1000 * (i + 1)} addr=0x{addr:012X} lat={lat} qd={qd} '
        f'crc={crc:04X}')
(root / 'synth-counters.log').write_text('\n'.join(lines) + '\n',
                                         encoding='ascii', newline='\n')

# ---------------------------------------------------------------------------
# synth-telemetry-f64.bin (Gorilla sweet-spot substrate, added post-I7 per
# coordinator research briefing): interleaved sensor telemetry { u64 ts_ms,
# f64 value } at an EXACT 1000 ms cadence so the delta-of-delta timestamp
# stream is '0' for every record after the second (Gorilla's 1-bit tier),
# and values drift a smooth 0.001 deg/sample with sub-noise perturbation so
# consecutive-value XOR stays in Case A/B1 (leading-zero reuse) territory.
# Float math is restricted to IEEE-754 basic ops (int*const, add) — no libm
# transcendentals — so the bytes are stable across platforms.
# 20,000 records x 16 B = 320,000 B. Seed 90007.
# ---------------------------------------------------------------------------
r = random.Random(90007)
ts_ms = 1_700_000_000_000
with (root / 'synth-telemetry-f64.bin').open('wb') as f:
    for i in range(20000):
        v = 20.0 + 0.001 * i + r.randint(-2, 2) * 0.0001
        f.write(struct.pack('<Qd', ts_ms, v))
        ts_ms += 1000

# ---------------------------------------------------------------------------
# synth-ndjson-columnar.ndjson (vXOR columnar substrate, added post-I7):
# natural NDJSON with MOSTLY-STABLE COLUMNS — sensor/unit/status constant
# over long runs, ts at exact cadence, one smoothly-drifting numeric column
# (reading), rare status flips. High zero-density under record-aligned
# column XOR is the vXOR trigger; deliberately VARIABLE line length (natural
# formatting, no padding) so frame detection is tested honestly rather than
# handed a fixed stride. 11,000 lines ~= 1.05 MB. Seed 90008.
# ---------------------------------------------------------------------------
r = random.Random(90008)
ts_ms = 1_700_000_000_000
lines = []
for i in range(11000):
    v = 20.0 + 0.001 * i + r.randint(-2, 2) * 0.0001
    status = 'warn' if (i // 1000) % 3 == 2 else 'ok'
    lines.append(
        '{{"seq":{},"ts":{},"sensor":"s-07","unit":"C",'
        '"reading":{:.3f},"status":"{}"}}'.format(i + 1, ts_ms, v, status))
    ts_ms += 1000
(root / 'synth-ndjson-columnar.ndjson').write_text('\n'.join(lines) + '\n',
                                                   encoding='ascii',
                                                   newline='\n')

for name in ('synth-timeseries.bin', 'synth-arith.bin', 'synth-jitter.bin',
             'synth-columnar-align.bin', 'synth-drift-stride.bin',
             'synth-counters.log', 'synth-telemetry-f64.bin',
             'synth-ndjson-columnar.ndjson'):
    p = root / name
    print(p, p.stat().st_size)
