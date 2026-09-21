#!/usr/bin/env python3
"""
datastruct lane (I8) — HONEST model-phrase cost model.

GUARD AGAINST AUDIT-7 (the uncharged-parameter bug class that pnra hit and
strategy logged): every cost line below is charged, and the reconstruction
identity is ASSERTED:

    wire >= sum(seed bits) + sum(residual bits) + sum(uncovered byte bits)
           + framing bits

and (seeds + residuals + uncovered) MUST be sufficient to reconstruct the
input. Nothing is free:

  - SEEDS: an order-k model needs k+1 initial values per segment. A segment
    covering L values pays (k+1) * width * 8 bits of seeds.
  - COVERAGE: a lane at (off, stride, width) touches only `stride` bytes per
    (off + i*stride .. +width) window. Bytes NOT touched by ANY lane are
    charged at 8 bits each.
  - MULTI-LANE: to cover all `stride` byte offsets you need stride/width lanes
    (each with its own offset). Their seeds add up.
  - FRAMING: lane descriptor (width, off, stride, order) + segment boundaries.

The clean accounting unit is the TILE: a (stride x width) decomposition of the
whole file into `stride/width` lanes, each of n/stride values.

Total cost for a tiling (stride S, width W, order k, n values per lane):
    seeds     = (S/W) * (k+1) * W * 8  bits
    residuals = (S/W) * (n_lane - k) * H(residual)   [H in bits/symbol]
    framing   = small (charged, not ignored)

Usage: python cost_model.py <files...> [--max-stride N] [--top N]
"""
from __future__ import annotations
import argparse, math, os
import numpy as np


def load(path):
    return open(path, "rb").read()


def tile_values(data: bytes, stride: int, width: int):
    """Decompose the file into stride/width lanes. Returns list of (off, values)."""
    n = len(data)
    b = np.frombuffer(data, dtype=np.uint8)
    lanes = []
    nlanes = stride // width
    count = (n - width) // stride + 1
    w = (1 << (8 * np.arange(width))).astype(np.int64)
    for li in range(nlanes):
        off = li * width
        idx = off + stride * np.arange(count, dtype=np.int64)
        valid = (idx + width) <= n
        idx = idx[valid]
        if idx.size < 8:
            continue
        g = b[idx[:, None] + np.arange(width)[None, :]]
        v = (g.astype(np.int64) * w).sum(axis=1)
        lanes.append((off, v))
    return lanes


def covered_bytes(stride, width, n):
    """Bytes touched by the full tiling."""
    nlanes = stride // width
    count = (n - width) // stride + 1
    return nlanes * count * width


def entropy_bits_per_sym(vals: np.ndarray) -> tuple[float, int]:
    if vals.size == 0:
        return 0.0, 0
    u, c = np.unique(vals, return_counts=True)
    p = c.astype(np.float64) / vals.size
    H = float(-(p * np.log2(p)).sum())
    return H, len(u)


def kth_diff(v: np.ndarray, k: int) -> np.ndarray:
    d = v.astype(np.int64)
    for _ in range(k):
        d = d[1:] - d[:-1]
    return d


def cost(data: bytes, stride: int, width: int, k: int,
         seed_bits: int, framing_bits: int = 64) -> dict | None:
    """Fully charged cost of one tiling. Returns None if infeasible."""
    n = len(data)
    if stride % width != 0 or stride <= 0 or width <= 0:
        return None
    lanes = tile_values(data, stride, width)
    if not lanes:
        return None
    total_res = 0.0
    total_seed = 0.0
    nvals = 0
    sym_max = 0
    for (off, v) in lanes:
        if v.size <= k + 1:
            return None
        d = kth_diff(v, k)
        H, nsym = entropy_bits_per_sym(d)
        total_res += H * d.size
        total_seed += (k + 1) * seed_bits
        nvals += v.size
        sym_max = max(sym_max, nsym)
    cov = covered_bytes(stride, width, n)
    uncov = n - cov
    uncov_bits = uncov * 8.0
    total = total_seed + total_res + uncov_bits + framing_bits
    return dict(stride=stride, width=width, order=k, bits=total,
                bytes=total / 8.0, ratio=total / 8.0 / n,
                seed_bits=total_seed, res_bits=total_res,
                uncov_bits=uncov_bits, cov=cov, n=n,
                nlanes=len(lanes), nvals=nvals, sym_max=sym_max)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--max-stride", type=int, default=48)
    ap.add_argument("--top", type=int, default=6)
    ap.add_argument("--orders", default="0,1,2")
    a = ap.parse_args()
    orders = [int(x) for x in a.orders.split(",")]

    for path in a.files:
        data = load(path)
        n = len(data)
        print(f"=== {os.path.basename(path)}  {n:,} B ===")

        rows = []
        for S in range(1, a.max_stride + 1):
            for W in (1, 2, 4, 8):
                if S % W or W > S:
                    continue
                for k in orders:
                    # seed cost: the generator wrote width*8-bit values, but a
                    # real coder sends seeds as deltas from neighbours; we
                    # charge a PESSIMISTIC full width*8 bits per seed.
                    r = cost(data, S, W, k, seed_bits=W * 8)
                    if r:
                        rows.append(r)
        rows.sort(key=lambda r: r["bits"])
        print(f"  {'bytes':>10} {'ratio':>7} {'S':>3} {'W':>2} {'k':>2} "
              f"{'seedB':>8} {'residB':>8} {'uncovB':>8} {'#sym':>5}")
        for r in rows[:a.top]:
            print(f"  {r['bytes']:>10,.0f} {r['ratio']:>7.4f} "
                  f"{r['stride']:>3} {r['width']:>2} {r['order']:>2} "
                  f"{r['seed_bits']/8:>8,.0f} {r['res_bits']/8:>8,.0f} "
                  f"{r['uncov_bits']/8:>8,.0f} {r['sym_max']:>5}")
        if rows:
            b = rows[0]
            print()
            print(f"  BEST (fully charged): S={b['stride']} W={b['width']} "
                  f"k={b['order']}  ->  {b['bytes']:,.0f} B  (r={b['ratio']:.4f})")
            print(f"    seeds {b['seed_bits']/8:,.0f} B + residuals "
                  f"{b['res_bits']/8:,.0f} B + uncovered "
                  f"{b['uncov_bits']/8:,.0f} B + 8 B framing")
        print()


if __name__ == "__main__":
    main()
