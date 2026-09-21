#!/usr/bin/env python3
"""gate-verify-i9-transform-bar.py - the dual bar for {synthetic} transform cells.

PART XIII S5b / DNB-E1: whenever ANVIL's candidate applies a reversible
transform, the reference codecs get the SAME transform opportunity before any
frontier claim. This computes the transform-enabled reference bars for the two
synth record files, next to the raw bars.

Delta definition: byte-wise mod-256 subtraction at the true record period P
(out[i] = in[i] - in[i-P], first P bytes pass through). This is exactly the
xz --delta filter's behaviour; the canonical CLI controls are:

  xz --delta=dist=14 --lzma2=preset=9e -c tests\\corpus\\synth-timeseries.bin     > ts.d14.xz
  xz --delta=dist=23 --lzma2=preset=9e -c tests\\corpus\\synth-columnar-align.bin > ca.d23.xz

Brotli has no shipped delta filter; the project standard (verdict I8 S4) is
brotli on the transformed bytes via the library - python-brotli 1.1.0 reproduces
the harness rows on these files exactly (q6=120,669 / q11=134,718 on
synth-timeseries). Rows are labelled measured-control.

Read-only. Usage: python docs/gate-verify-i9-transform-bar.py
"""
from __future__ import annotations
import hashlib
import lzma
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def delta(buf: bytes, p: int) -> bytes:
    out = bytearray(len(buf))
    for i in range(len(buf)):
        out[i] = (buf[i] - (buf[i - p] if i >= p else 0)) & 0xFF
    return bytes(out)


def main() -> int:
    try:
        import brotli
        bver = getattr(brotli, "__version__", "?")
    except Exception as e:  # pragma: no cover
        brotli = None
        bver = f"UNAVAILABLE ({e})"
    try:
        import zstandard as zstd
    except Exception:
        zstd = None
    print(f"python-brotli: {bver}   python-zstandard: {'yes' if zstd else 'no'}"
          f"   python-lzma/xz-compatible presets: 9e")
    pairs = [
        ("tests/corpus/synth-timeseries.bin", 14),
        ("tests/corpus/synth-columnar-align.bin", 23),
    ]
    for rel, p in pairs:
        path = os.path.join(ROOT, rel)
        data = open(path, "rb").read()
        print(f"\n== {rel}  {len(data)} B  sha256={hashlib.sha256(data).hexdigest()[:16].upper()}")
        d = delta(data, p)
        print(f"   delta P={p}: zero fraction {100.0 * d.count(0) / len(d):.1f}%")
        if brotli:
            for q in (6, 9, 11):
                c = brotli.compress(data, quality=q)
                print(f"   raw      brotli q{q:<2d}: {len(c):7d} B  ratio {len(c)/len(data):.4f}")
            for q in (6, 9, 11):
                c = brotli.compress(d, quality=q)
                print(f"   delta{p:<3d} brotli q{q:<2d}: {len(c):7d} B  ratio {len(c)/len(data):.4f}")
        x = lzma.compress(data, format=lzma.FORMAT_XZ, preset=9 | lzma.PRESET_EXTREME)
        print(f"   raw      xz -9e     : {len(x):7d} B  ratio {len(x)/len(data):.4f}")
        xd = lzma.compress(d, format=lzma.FORMAT_XZ, preset=9 | lzma.PRESET_EXTREME)
        print(f"   delta{p:<3d} xz -9e     : {len(xd):7d} B  ratio {len(xd)/len(data):.4f}")
        if zstd:
            z = zstd.ZstdCompressor(level=19).compress(d)
            print(f"   delta{p:<3d} zstd-19    : {len(z):7d} B  ratio {len(z)/len(data):.4f}")
    print("\nDual bar rule: a row that is non-dominated on the RAW grid but is beaten by a")
    print("transform-enabled reference row is FRONT-GAP (dual-bar), not FRONT-CROSSING.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
