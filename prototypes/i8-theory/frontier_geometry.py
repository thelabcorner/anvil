#!/usr/bin/env python3
"""theory/i8 — frontier geometry from tests/benchmark-suite.csv.

Purpose: answer "what does ANVIL actually have to achieve to produce an
EXTENDS_FRONT row", per file and per plane, with the reference front
computed from the SAME csv (no re-measurement).

Method (standard Pareto): a candidate row (r, t) where r = ratio (lower
better) and t = throughput (higher better) EXTENDS the front of a set S iff
there is NO row in S with r' <= r AND t' >= t (with at least one strict).
We compute, for each ANVIL row, whether it is dominated by the union of all
reference (brotli/zstd) rows, and the exact coordinates of the reference
front "knee" it must beat.

Also computes the MDL REQUIRED BYTE BUDGET: for each ANVIL row, the maximum
compressed size at which it would still cross the front (holding its own
throughput), i.e. the largest size L* such that (L*/N, t_anvil) is
non-dominated. This is the number that matters for `theory` deliverable 2:
it converts "need -36.8% bytes" into "the byte budget you may spend".

Usage: python prototypes/i8-theory/frontier_geometry.py
"""
import csv, math, os, sys
from collections import defaultdict

CSV = 'tests/benchmark-suite.csv'
REF_PREFIX = ('brotli-', 'zstd-')


def load():
    rows = []
    with open(CSV, newline='') as f:
        for r in csv.DictReader(f):
            rows.append(dict(
                file=r['file'].replace('tests\\corpus\\', '').replace('tests/corpus/', ''),
                n=int(r['input_bytes']),
                codec=r['codec'],
                b=int(r['compressed_bytes']),
                ratio=float(r['ratio']),
                enc=float(r['encode_MBps']),
                dec=float(r['decode_MBps']),
                rt=r['roundtrip'],
            ))
    return rows


def dominates(a, b):
    """a dominates b on (ratio lower, throughput higher) - no, we pass explicit."""
    raise NotImplementedError


def dom(a_r, a_t, b_r, b_t):
    """row A dominates row B: A.ratio <= B.ratio and A.tput >= B.tput, one strict."""
    le = a_r <= b_r
    ge = a_t >= b_t
    return (le and ge) and (a_r < b_r or a_t > b_t)


def front(rows):
    """Pareto-maximal set (ratio min, tput max)."""
    out = []
    for i, a in enumerate(rows):
        if not any(dom(b[0], b[1], a[0], a[1]) for j, b in enumerate(rows) if j != i):
            out.append(a)
    return sorted(set(out))


def max_size_to_cross(ratio_tput_refs, n, t):
    """Largest compressed size L such that (L/n, t) is NOT dominated by any
    reference row. Search over the reference front: the binding constraint is
    the reference row with the smallest ratio among those with tput >= t
    (call it r_star). We need L/n < r_star, i.e. L < n*r_star. If no ref has
    tput >= t, the row crosses at any ratio (unbounded) -> return n (trivially
    crosses since ratio <= 1 <= ... but really means 'no ratio requirement').
    """
    cands = [(r, tt) for (r, tt) in ratio_tput_refs if tt >= t]
    if not cands:
        return float('inf'), None
    r_star, tt_star = min(cands)  # smallest ratio among faster-or-equal refs
    return n * r_star, (r_star, tt_star)


def main():
    rows = load()
    byfile = defaultdict(list)
    for r in rows:
        byfile[r['file']].append(r)

    print("=" * 100)
    print("FRONTIER GEOMETRY — per file, both planes")
    print("=" * 100)
    print(f"{'file':<26}{'N':>10} | {'best ANVIL bytes':>16} {'r':>7} {'dec':>7} "
          f"| {'ref pivot (r,dec)':>22} | {'byte budget @ANVIL dec':>24} {'need':>8}")

    summary = []
    for fn in sorted(byfile, key=lambda x: -byfile[x][0]['n']):
        rs = byfile[fn]
        n = rs[0]['n']
        anvil = [r for r in rs if not r['codec'].startswith(REF_PREFIX)]
        ref = [r for r in rs if r['codec'].startswith(REF_PREFIX)]

        # DECODE PLANE
        best_a = min(anvil, key=lambda r: r['b'])            # best-ratio ANVIL
        fastest_a = max(anvil, key=lambda r: r['dec'])        # best-decode ANVIL
        ref_dec = [(r['ratio'], r['dec']) for r in ref]
        ref_enc = [(r['ratio'], r['enc']) for r in ref]

        L_star, pivot = max_size_to_cross(ref_dec, n, best_a['dec'])
        need = (best_a['b'] - L_star) / best_a['b'] * 100 if L_star < float('inf') else float('nan')

        print(f"{fn:<26}{n:>10} | {best_a['b']:>16} {best_a['ratio']:>7.4f} "
              f"{best_a['dec']:>7.1f} | "
              f"{('%.4f @ %.1f' % pivot) if pivot else 'CROSSES':>22} | "
              f"{('%.0f' % L_star) if L_star < float('inf') else 'inf':>24} "
              f"{need:>7.1f}%")

        summary.append(dict(file=fn, n=n,
                            best_b=best_a['b'], best_codec=best_a['codec'],
                            best_ratio=best_a['ratio'], best_dec=best_a['dec'],
                            fastest_a=fastest_a['codec'],
                            fastest_dec=fastest_a['dec'],
                            fastest_b=fastest_a['b'],
                            L_star=L_star, pivot=pivot,
                            need_pct=need))
    print()
    print("=" * 100)
    print("DECODE-PLANE CROSSING AT ANVIL'S *FASTEST* DECODE (the realistic attack)")
    print("=" * 100)
    print(f"{'file':<26}{'fastest ANVIL':>28}{'r':>8}{'dec':>8} | "
          f"{'byte budget':>12} {'vs best-ratio ANVIL':>20} {'slack':>10}")
    for s in summary:
        fn = s['file']
        rs = byfile[fn]
        n = s['n']
        ref = [r for r in rs if r['codec'].startswith(REF_PREFIX)]
        ref_dec = [(r['ratio'], r['dec']) for r in ref]
        fa = max((r for r in rs if not r['codec'].startswith(REF_PREFIX)),
                 key=lambda r: r['dec'])
        L, piv = max_size_to_cross(ref_dec, n, fa['dec'])
        slack = L - s['best_b']
        print(f"{fn:<26}{fa['codec']:>28}{fa['ratio']:>8.4f}{fa['dec']:>8.1f} | "
              f"{('%.0f' % L) if L < float('inf') else 'inf':>12} "
              f"{s['best_b']:>20} "
              f"{('%+.0f' % slack) if L < float('inf') else 'n/a':>10}")

    print()
    print("=" * 100)
    print("ENCODE PLANE (ANVIL's one winning axis)")
    print("=" * 100)
    print(f"{'file':<26}{'ANVIL row':>24}{'r':>8}{'enc':>8} | "
          f"{'byte budget @ANVIL enc':>22}")
    for s in summary:
        fn = s['file']
        rs = byfile[fn]
        n = s['n']
        ref = [r for r in rs if r['codec'].startswith(REF_PREFIX)]
        ref_enc = [(r['ratio'], r['enc']) for r in ref]
        best_a = min((r for r in rs if not r['codec'].startswith(REF_PREFIX)),
                     key=lambda r: r['b'])
        L, piv = max_size_to_cross(ref_enc, n, best_a['enc'])
        print(f"{fn:<26}{best_a['codec']:>24}{best_a['ratio']:>8.4f}{best_a['enc']:>8.2f} | "
              f"{('%.0f' % L) + ('  (pivot %s)' % (piv[0] if piv else '')) if L < float('inf') else 'CROSSES':>22}")


if __name__ == '__main__':
    main()
