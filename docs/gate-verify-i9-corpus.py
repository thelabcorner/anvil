#!/usr/bin/env python3
"""gate-verify-i9-corpus.py - record-structure verification for I9 (research-gate).

Independently determines the MINIMAL record period of the synthetic corpus
files from the bytes alone, then checks the two periods that were recorded in
I8 (28 and 184) as ALIASES (integer multiples of the true period).

Method (data-only, no generator constants assumed for the determination):
  For a candidate period P, walk the file in P-byte blocks and evaluate a
  record-local invariant that must hold for EVERY block at the true period.
    - synth-timeseries.bin : u16le at offset 12 == block_index % 1000
    - synth-columnar-align : byte at offset 22 == XOR(bytes 0..21)
  The minimal P with 100% satisfaction is the record period. Multiples of it
  also satisfy the invariant (block boundaries still fall on record starts),
  which is exactly the aliasing failure mode being corrected.

Also prints the fully-constant byte offsets per record and their fraction of
bytes, for the true period and for the two recorded aliases.

Read-only: never writes tests/corpus.

Usage: python docs/gate-verify-i9-corpus.py
"""
from __future__ import annotations
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CORPUS = os.path.join(ROOT, "tests", "corpus")


def invariant_timeseries(buf: bytes, P: int) -> tuple[int, int]:
    n = len(buf)
    blocks = 0
    ok = 0
    k = 0
    while P * k + 14 <= n:
        blocks += 1
        if int.from_bytes(buf[P * k + 12:P * k + 14], "little") == k % 1000:
            ok += 1
        k += 1
    return ok, blocks


def invariant_columnar(buf: bytes, P: int) -> tuple[int, int]:
    n = len(buf)
    blocks = 0
    ok = 0
    k = 0
    while P * k + 23 <= n:
        blocks += 1
        start = P * k
        par = buf[start + 22]
        x = 0
        for b in buf[start:start + 22]:
            x ^= b
        if par == x:
            ok += 1
        k += 1
    return ok, blocks


def minimal_period(buf: bytes, inv) -> tuple[int, list[int]]:
    perfect = []
    for P in range(1, 513):
        if len(buf) % P != 0:
            continue
        ok, blocks = inv(buf, P)
        if blocks > 0 and ok == blocks:
            perfect.append(P)
    return (perfect[0] if perfect else -1), perfect


def constant_offsets(buf: bytes, P: int) -> list[int]:
    n = len(buf)
    rows = n // P
    out = []
    for o in range(P):
        b0 = buf[o]
        if all(buf[r * P + o] == b0 for r in range(rows)):
            out.append(o)
    return out


def report(path: str, inv, expected_aliases: list[tuple[int, int]]):
    buf = open(path, "rb").read()
    name = os.path.basename(path)
    mp, perfect = minimal_period(buf, inv)
    print(f"== {name}  size={len(buf)} B")
    print(f"   minimal period (100% invariant satisfaction) = {mp}")
    print(f"   periods in 1..512 passing 100% = {perfect[:12]}"
          f"{' ...' if len(perfect) > 12 else ''}")
    for P, why in expected_aliases:
        ok, blocks = inv(buf, P)
        co = constant_offsets(buf, P)
        frac = len(co) / P
        print(f"   P={P:4d} ({why}): invariant {ok}/{blocks} = "
              f"{100.0*ok/max(blocks,1):.1f}%   constant offsets {len(co)}/{P} "
              f"= {100.0*frac:.1f}%  {co}")
    co = constant_offsets(buf, mp)
    print(f"   P={mp:4d} (true): constant offsets {len(co)}/{mp} = "
          f"{100.0*len(co)/mp:.1f}%  {co}")
    print()


def main() -> int:
    ts = os.path.join(CORPUS, "synth-timeseries.bin")
    ca = os.path.join(CORPUS, "synth-columnar-align.bin")
    ar = os.path.join(CORPUS, "synth-arith.bin")
    for p in (ts, ca):
        if not os.path.exists(p):
            print(f"MISSING {p}", file=sys.stderr)
            return 1
    report(ts, invariant_timeseries, [(28, "recorded I8 alias"), (56, "2x true")])
    report(ca, invariant_columnar, [(184, "recorded I8 alias"), (46, "2x true")])
    buf = open(ar, "rb").read()
    print(f"== synth-arith.bin  size={len(buf)} B  "
          f"(8 columns x 8000 u32 = {8*8000*4}; not a record file)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
