#!/usr/bin/env python3
"""Fledge Alpha Free - isolated integer-only check of Track 07 H-1 (walk-width saturation).

NO corpus, NO codec, NO timing loop, NO entropy coder. Pure integer arithmetic on
the dispatch structure of third_party/libsais/src/libsais.c.

Two questions, both decidable from source text alone:

Q1. Is the aux index logically necessary at a given checkpoint density r?
    libsais.c:8053 validates exactly `1 + (n-1)/r` seeds: `for (t = 0;
    t <= (n-1)/r; ++t) { if (I[t] <= 0 || I[t] > n) return -1; }`.
    libsais.c:7887-7939 dispatches blocks 8 at a time, block j seeded by I[j],
    block j covering output bytes [j*r, min((j+1)*r, n)). Therefore
    seeds == blocks exactly, with no slack and no sharing.

Q2. Which checkpoint counts are actually reachable, and what do they cost?
    libsais.c:8042 hard-requires r to be a power of two (unless r == n):
        ((r != n) && ((r < 2) || ((r & (r - 1)) != 0)))
    So the reachable block counts B = 1 + (n-1)//r are a SPARSE, n-dependent set.
    This script enumerates them and costs each with the exact dispatch structure.

Cost model (declared, falsifiable, integer only):
  * libsais_unbwt_decode_8 body (libsais.c:7867-7877) is 8 independent
    load->use chains `p = P[p]`, replicated 8x, advanced in lockstep for `k`
    iterations. An m-wide invocation with k iterations costs k/min(m,8) units,
    where 8 is the widest shipped kernel (libsais.c:7887 `while (blocks > 8)`).
    The 1-wide kernel (libsais.c:7897) is the same shape with one chain.
  * Each invocation's iteration count follows the strided decomposition visible in
    libsais.c:7894-7940: the last block holds `remainder` bytes; a tail invocation
    of width m first writes `remainder` bytes to each of m strided r-blocks, then a
    width-(m-1) invocation appends the remaining r-remainder bytes.
  * U0..U7 are spaced exactly r apart (libsais.c:7857-7863), confirming the strided
    layout above.
This model is a MODEL. Its only job is to rank reachable geometries; SBW-1 measures.
"""

from __future__ import annotations

import sys

W_MAX = 8  # widest shipped kernel: libsais.c:7854 decode_8, dispatched at :7887


def blocks_of(n: int, r: int) -> int:
    """libsais.c:7946  blocks = 1 + ((n - 1) / r)"""
    return 1 + (n - 1) // r


def remainder_of(n: int, r: int, b: int) -> int:
    """libsais.c:7947  remainder = n - r * (blocks - 1)"""
    return n - r * (b - 1)


def dispatch_cost(n: int, r: int) -> float:
    """Total cost units for one libsais_unbwt_decode() call (libsais.c:7882-7941).

    Replays the control flow literally:
      while blocks > 8: decode_8(k = r>>1); blocks -= 8; offset += 8r
      then exactly one tail arm selected by the residual block count.
    Cost of a width-m invocation of k iterations = k / min(m, W_MAX).
    """
    blocks = blocks_of(n, r)
    rem = remainder_of(n, r, blocks)
    total = 0.0
    # main loop, libsais.c:7887-7892
    while blocks > W_MAX:
        total += (r >> 1) / W_MAX  # decode_8, full width
        blocks -= W_MAX
    # tail arms, libsais.c:7894-7940
    if blocks == 1:
        total += (rem >> 1) / 1  # :7897
    else:
        # every arm runs decode_{blocks} at remainder>>1 then decode_{blocks-1}
        # at (r>>1)-(remainder>>1); the blocks==8 arm is the `else` at :7935.
        total += (rem >> 1) / blocks
        total += ((r >> 1) - (rem >> 1)) / (blocks - 1)
    return total


def per_byte_cost(n: int, r: int) -> float:
    return dispatch_cost(n, r) / n


def is_pow2(x: int) -> bool:
    return x > 0 and (x & (x - 1)) == 0


def pow2_at_least(x: int) -> int:
    p = 1
    while p < x:
        p <<= 1
    return p


def reachable(n: int):
    """All (r, B) pairs libsais will accept: r == n, or r a power of two in [2, n]."""
    out = []
    r = 2
    while r <= n:
        if is_pow2(r):
            out.append((r, blocks_of(n, r)))
        r <<= 1
    out.append((n, 1))  # r == n is explicitly allowed (libsais.c:8042)
    return sorted(set(out), key=lambda t: (t[1], t[0]))


def incumbent_r(n: int, walks: int = 1024) -> int:
    """src/anvil.cpp:4247-4256 policy: smallest pow2 >= ceil(n / walks), min 2."""
    r = pow2_at_least(-(-n // walks))
    return max(r, 2)


def analyse(n: int, label: str) -> dict:
    r_inc = incumbent_r(n)
    b_inc = blocks_of(n, r_inc)
    c_inc = per_byte_cost(n, r_inc)

    rows = []
    for r, b in reachable(n):
        if r > n:
            continue
        rows.append({
            "r": r,
            "blocks": b,
            "index_bytes": 4 * b,
            "per_byte": per_byte_cost(n, r),
            "rel_cost": per_byte_cost(n, r) / c_inc,
            "full_width": (b % W_MAX == 0),
        })

    # best reachable reduction that keeps predicted cost within +2% (the project's
    # own practical-speed threshold) of the incumbent
    ok = [x for x in rows if x["rel_cost"] <= 1.02]
    best_ok = min(ok, key=lambda x: x["index_bytes"]) if ok else None
    best_any = min(rows, key=lambda x: x["index_bytes"])

    return {
        "label": label,
        "n": n,
        "incumbent_r": r_inc,
        "incumbent_blocks": b_inc,
        "incumbent_index_bytes": 4 * b_inc,
        "best_ok": best_ok,
        "best_any": best_any,
        "full_width_options": [x for x in rows if x["full_width"]],
    }


def main() -> int:
    targets = [
        ("dickens", 10_192_446),
        ("webster", 41_458_703),
        ("enwik8", 100_000_000),
        # geometry probes: does the reachable set depend on n's binary digits?
        ("probe-2^27", 1 << 27),
        ("probe-2^27+2^20", (1 << 27) + (1 << 20)),
    ]
    print("Q1  seeds == blocks, exactly. libsais.c:8053 validates 1+(n-1)/r entries;")
    print("    libsais.c:7887-7939 dispatches one chain per block seeded by I[j].")
    print("    => index_bytes = 4 * blocks, with no slack. Density reduction is")
    print("       possible ONLY by lengthening r, i.e. by REDUCING CHAIN COUNT.\n")

    print("Q2  reachable block counts under the pow2 hard constraint (libsais.c:8042)")
    hdr = (f"{'target':<18}{'n':>13}{'B_inc':>7}{'idx_inc':>9}"
           f"{'B@<=+2%':>9}{'idx':>7}{'cut':>7}{'B@min':>7}{'idx':>7}{'cost':>8}")
    print(hdr)
    print("-" * len(hdr))
    for label, n in targets:
        a = analyse(n, label)
        ok, anyb = a["best_ok"], a["best_any"]
        assert ok is not None, label
        cut = 1.0 - ok["index_bytes"] / a["incumbent_index_bytes"]
        print(f"{label:<18}{n:>13,}{a['incumbent_blocks']:>7}"
              f"{a['incumbent_index_bytes']:>9,}{ok['blocks']:>9}"
              f"{ok['index_bytes']:>7,}{cut:>7.1%}{anyb['blocks']:>7}"
              f"{anyb['index_bytes']:>7,}{anyb['rel_cost']:>8.3f}")

    print("\nfull-width (B mod 8 == 0) reachable options, smallest index first:")
    for label, n in targets[:3]:
        a = analyse(n, label)
        fw = sorted(a["full_width_options"], key=lambda x: x["index_bytes"])[:4]
        s = ", ".join(f"B={x['blocks']}/r={x['r']:,}/{x['index_bytes']}B" for x in fw)
        print(f"  {label:<10} {s}")

    print("\nproposed arm A5 (enwik8, 16 walks) and the 8-chain floor:")
    for label, walks in (("A5=16 walks", 16), ("A4=32 walks", 32), ("A3=128 walks", 128)):
        n = 100_000_000
        r = max(pow2_at_least(-(-n // walks)), 2)
        b = blocks_of(n, r)
        inc = analyse(n, "enwik8")
        print(f"  {label:<12} r={r:>10,}  B={b:>4}  index={4*b:>5} B"
              f"  full_width={b % W_MAX == 0}  predicted_rel_cost="
              f"{per_byte_cost(n, r) / per_byte_cost(n, inc['incumbent_r']):.3f}")
    print(f"  theoretical floor 8 chains (B=8) => 32 B, but needs pow2 r in")
    print(f"  ((n-1)/8, (n-1)/7]; for n=1e8 that is (12499999.875, 14285714.0] -> no pow2.")
    return 0


if __name__ == "__main__":
    sys.exit(main())