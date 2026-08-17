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

for name in ('synth-timeseries.bin', 'synth-arith.bin', 'synth-jitter.bin'):
    p = root / name
    print(p, p.stat().st_size)
