#!/usr/bin/env python3
"""theory/i8 — iso-crossing curve: the byte budget as a FUNCTION of ANVIL
decode throughput.

THE POINT
---------
tools/pareto_front.py rules EXTENDS_FRONT iff no reference row q has
(q.ratio <= a.ratio AND q.mbps >= a.mbps). Rearranged: define

    R_ref(t) = min { q.ratio : q.decode_mbps >= t }        (inf if none)

Then an ANVIL row at (L, t) CROSSES iff  L/N < R_ref(t).

R_ref is a DECREASING STEP FUNCTION of t. Its steps are the reference rows.
So the byte budget L*(t) = N * R_ref(t) is also a step function, and it has
CLIFFS: at t = q.decode for some reference row q, stepping just above q's
throughput REMOVES q from the min, and the budget jumps to the next-best
ratio among the remaining (slower-or-equal... no: FASTER) references.

A cliff where the budget jumps by a LOT for a SMALL throughput increase is
the cheapest attack on the front. That is the whole deliverable: it converts
"we need -36.8% bytes" (which is the t -> 0 branch, i.e. beat q11 at any
speed) into a menu of (decode speed, byte budget) targets, and names the
cheapest one.

Units / conventions:
  ratio r = compressed/input (lower better)
  t in MB/s (higher better)
  N = input bytes
  L*(t) = N * R_ref(t), in bytes; the row must be STRICTLY under it.

Falsification: recompute from tests/benchmark-suite.csv; the curve is a pure
function of that file. If the CSV's reference set changes (new zstd levels,
new brotli levels) the curve changes — re-run, do not reuse.

Usage: python prototypes/i8-theory/iso_crossing.py
"""
import csv, sys
from collections import defaultdict

CSV = 'tests/benchmark-suite.csv'
REF = ('brotli-', 'zstd-')
SLACK = 0.02          # 2% throughput margin over the reference row's own MB/s
                      # (never claim a crossing by an exact-tie on throughput:
                      #  timing CV is 0.9-17% by codec -- see agenda §4.3)


def load():
    out = []
    with open(CSV, newline='') as f:
        for r in csv.DictReader(f):
            out.append(dict(
                file=r['file'].replace('tests\\corpus\\', '').replace('tests/corpus/', ''),
                n=int(r['input_bytes']), codec=r['codec'], b=int(r['compressed_bytes']),
                ratio=float(r['ratio']), enc=float(r['encode_MBps']),
                dec=float(r['decode_MBps'])))
    return out


def budget_curve(refs, n, key, slack_frac=SLACK):
    """Return the step function as a list of (t_threshold, ratio, codec) — the
    value of R_ref(t) for t in [t_i * (1+slack), t_{i+1} * (1+slack)).
    Sorted by increasing required throughput (cheapest first)."""
    # R_ref(t) = min ratio among refs with mbps >= t.
    # The function changes exactly at t = q.mbps for each q.
    # For t slightly above q.mbps*(1+slack), q is excluded.
    pts = sorted(refs, key=lambda r: r[key], reverse=True)  # fastest first
    curve = []
    # For t in (0, fastest.mbps]: all refs available -> global min ratio
    # Walk thresholds upward: at each step we drop the refs slower than t.
    thresholds = sorted(set(r[key] for r in refs))
    for i, t in enumerate(thresholds):
        t_req = t * (1.0 + slack_frac)
        avail = [r for r in refs if r[key] >= t_req]
        # the threshold at which the NEXT drop happens
        if not avail:
            curve.append((t_req, float('inf'), None))
            continue
        best = min(avail, key=lambda r: r['ratio'])
        curve.append((t_req, best['ratio'], best['codec']))
    return curve


def compact(curve):
    """Collapse consecutive equal-ratio entries; return list of
    (t_min, t_max, ratio, codec) segments."""
    segs = []
    for t, r, c in curve:
        if segs and segs[-1][2] == r and segs[-1][3] == c:
            segs[-1][1] = t
        else:
            segs.append([t, t, r, c])
    return segs


def main():
    rows = load()
    byfile = defaultdict(list)
    for r in rows:
        byfile[r['file']].append(r)

    order = sorted(byfile, key=lambda x: -byfile[x][0]['n'])
    print("=" * 118)
    print("ISO-CROSSING CURVE — decode plane.  L*(t) = max compressed bytes that still reads EXTENDS_FRONT")
    print(f"(throughput thresholds inflated {SLACK:.0%} — never cross on an exact tie; timing CV 0.9-17%)")
    print("=" * 118)

    cliffs = []
    for fn in order:
        rs = byfile[fn]
        n = rs[0]['n']
        refs = [r for r in rs if r['codec'].startswith(REF)]
        anvil = [r for r in rs if not r['codec'].startswith(REF)]
        best_a = min(anvil, key=lambda r: r['b'])
        segs = compact(budget_curve(refs, n, 'dec'))
        print(f"\n{fn}  N={n:,}   best ANVIL = {best_a['b']:,} B "
              f"({best_a['ratio']:.4f}) @ {best_a['dec']:.1f} MB/s dec  [{best_a['codec']}]")
        print(f"  {'need dec >= (MB/s)':>20} {'byte budget':>14} {'ratio':>9} "
              f"{'binding ref':>18} {'vs best ANVIL':>14}")
        prev = None
        for tlo, thi, r, c in segs:
            if r == float('inf'):
                print(f"  {tlo:>20.1f} {'CROSSES (no ref that fast)':>14} "
                      f"{'inf':>9} {'-':>18} {'+inf':>14}")
                prev = None
                continue
            L = n * r
            gap = L - best_a['b']
            print(f"  {tlo:>20.1f} {L:>14,.0f} {r:>9.4f} {c:>18} {gap:>+14,.0f}")
            if prev is not None and prev[2] > r:
                dt = tlo - prev[1]
                dL = (r - prev[2]) * n
                cliffs.append((fn, n, prev[1], tlo, dL, dt,
                               dL / max(dt, 1e-9), best_a['b'], prev[2], r))
            prev = (tlo, thi, r, c)

    print()
    print("=" * 118)
    print("CLIFFS — cheapest decode improvements, ranked by bytes-budget-bought per MB/s")
    print("=" * 118)
    print(f"{'file':<26}{'from MB/s':>10}{'to MB/s':>10}{'dB budget':>12}"
          f"{'d MB/s':>9}{'B per MB/s':>14}{'new ratio':>11}{'vs ANVIL':>12}")
    for c in sorted(cliffs, key=lambda x: -x[6]):
        fn, n, t0, t1, dL, dt, eff, ba, r0, r1 = c
        print(f"{fn:<26}{t0:>10.1f}{t1:>10.1f}{dL:>+12,.0f}{dt:>9.1f}"
              f"{eff:>14,.0f}{r1:>11.4f}{n*r1 - ba:>+12,.0f}")

    # ---- how much does the CURRENT best ANVIL row miss by, on the decode route
    print()
    print("=" * 118)
    print("TWO ROUTES TO A CROSSING — per file")
    print("=" * 118)
    print(f"{'file':<26}{'ROUTE A: bytes to find':>24}{'A %':>8}   "
          f"{'ROUTE B: MB/s to find':>24}{'B x':>8}")
    for fn in order:
        rs = byfile[fn]
        n = rs[0]['n']
        refs = [r for r in rs if r['codec'].startswith(REF)]
        anvil = [r for r in rs if not r['codec'].startswith(REF)]
        best_a = min(anvil, key=lambda r: r['b'])
        segs = compact(budget_curve(refs, n, 'dec'))
        r_now = min(s[2] for s in segs)                      # best-possible (t small)
        L_A = n * r_now
        # Route B: smallest t at which budget >= current ANVIL bytes
        routeB = None
        for tlo, thi, r, c in segs:
            if n * r > best_a['b']:
                routeB = tlo
                break
        print(f"{fn:<26}{best_a['b'] - L_A:>+24,.0f}"
              f"{(best_a['b'] - L_A) / best_a['b'] * 100:>7.1f}%   "
              f"{(('%8.1f -> %8.1f' % (best_a['dec'], routeB)) if routeB else 'IMPOSSIBLE'):>24}"
              f"{(('%.1fx' % (routeB / best_a['dec'])) if routeB else '   -'):>8}")


if __name__ == '__main__':
    main()
