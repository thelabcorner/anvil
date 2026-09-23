#!/usr/bin/env python3
"""
datastruct lane (I8) — FAST lane/field structure analysis.

Pure analysis (no coding): for a candidate (lane_offset, lane_stride, width),
measure the residual entropy of the order-k finite-difference model over the
whole lane. Vectorised with numpy so a full sweep over stride x width x order
is seconds, not minutes.

This answers: "Is there a generative model here, and what is its rate?"
before any wire is written. The counted-wire coder (model_phrase.py) then
measures the real cost.

Usage: python lane_probe.py <files...> [--top N]
"""
from __future__ import annotations
import argparse, math, os, sys
import numpy as np
from collections import Counter


def load(path):
    return np.frombuffer(open(path, "rb").read(), dtype=np.uint8)


def lane_values(data: bytes, off: int, count: int, width: int, stride: int) -> np.ndarray:
    """Values read at off + i*stride, `width` bytes little-endian."""
    n = len(data)
    end = off + (count - 1) * stride + width
    if end > n:
        count = (n - off - width) // stride + 1
        if count <= 0:
            return np.empty(0, dtype=np.int64)
    dt = {(1, False): np.uint8, (2, False): np.uint16, (4, False): np.uint32,
          (8, False): np.uint64}[(width, False)]
    arr = np.frombuffer(data, dtype=dt, count=(n - width) // width + 1, offset=0)
    # byte offset off -> element index requires off % width == 0 for view-cast
    # general path: build index explicitly
    idx = off + stride * np.arange(count, dtype=np.int64)
    # gather bytes and compose LE
    b = np.frombuffer(data, dtype=np.uint8)
    valid = (idx + width) <= n
    idx = idx[valid]
    if idx.size == 0:
        return np.empty(0, dtype=np.int64)
    # gather width bytes per lane slot
    g = b[idx[:, None] + np.arange(width)[None, :]]
    w = (1 << (8 * np.arange(width))).astype(np.int64)
    return (g.astype(np.int64) * w).sum(axis=1)


def entropy_bits(vals: np.ndarray) -> tuple[float, int]:
    """Empirical entropy of the value sequence, in BITS TOTAL (not per symbol)."""
    if vals.size == 0:
        return 0.0, 0
    u, c = np.unique(vals, return_counts=True)
    p = c.astype(np.float64) / vals.size
    H = float(-(p * np.log2(p)).sum())
    return H * vals.size, len(u)


def kth_diff(vals: np.ndarray, k: int) -> np.ndarray:
    d = vals.astype(np.int64)
    for _ in range(k):
        d = d[1:] - d[:-1]
    return d


def probe(data: bytes, max_stride=64, widths=(1, 2, 4, 8), orders=(0, 1, 2),
          top=8, min_count=64):
    n = len(data)
    results = []
    for width in widths:
        for off in range(width):          # phase within the byte stride
            if off >= max_stride:
                break
            for stride in range(width, max_stride + 1):
                count = (n - off - width) // stride + 1
                if count < min_count:
                    continue
                v = lane_values(data, off, count, width, stride)
                if v.size < min_count:
                    continue
                for k in orders:
                    if v.size <= k + 1:
                        continue
                    d = kth_diff(v, k)
                    bits, nun = entropy_bits(d)
                    # rate: bits per COVERED BYTE
                    covered = v.size * stride
                    results.append((bits / covered if covered else 9e9,
                                    width, off, stride, k, bits, nun, v.size,
                                    covered))
    results.sort(key=lambda r: r[0])
    return results[:top]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--max-stride", type=int, default=64)
    ap.add_argument("--top", type=int, default=8)
    ap.add_argument("--widths", default="1,2,4,8")
    ap.add_argument("--orders", default="0,1,2")
    a = ap.parse_args()
    widths = tuple(int(x) for x in a.widths.split(","))
    orders = tuple(int(x) for x in a.orders.split(","))

    for path in a.files:
        data = open(path, "rb").read()
        n = len(data)
        print(f"=== {os.path.basename(path)}  {n:,} B ===")
        b = np.frombuffer(data, dtype=np.uint8)
        # raw baselines
        bits0, _ = entropy_bits(b.astype(np.int64))
        # order-1 byte entropy
        o1 = b[:-1].astype(np.int64) * 256 + b[1:].astype(np.int64)
        bits1, _ = entropy_bits(o1)
        print(f"  raw byte order-0 : {bits0/8:>10,.0f} B  (r={bits0/8/n:.4f})")
        print(f"  raw byte order-1 : {bits1/8:>10,.0f} B  (r={bits1/8/n:.4f})")
        print()
        res = probe(data, a.max_stride, widths, orders, a.top)
        print(f"  {'bits/B':>7} {'w':>2} {'off':>3} {'stride':>6} {'k':>2} "
              f"{'bits':>12} {'#sym':>6} {'nvals':>8} {'covB':>9}")
        for (rate, w, off, S, k, bits, nun, nv, cov) in res:
            print(f"  {rate:>7.4f} {w:>2} {off:>3} {S:>6} {k:>2} "
                  f"{bits:>12,.0f} {nun:>6} {nv:>8} {cov:>9,}")
        if res:
            r0 = res[0]
            best_bytes = r0[5] / 8.0
            print()
            print(f"  BEST single-lane model: width={r0[1]} off={r0[2]} "
                  f"stride={r0[3]} order={r0[4]}")
            print(f"    residual stream alone = {best_bytes:,.0f} B "
                  f"(covers {r0[8]:,} B of {n:,} B = {100.0*r0[8]/n:.1f}%)")
            print(f"    NOTE: excludes uncovered bytes + all model parameters.")
        print()


if __name__ == "__main__":
    main()
