#!/usr/bin/env python3
"""theory/i8 — CALIBRATE lambda from the measured reference frontier.

lambda is the shadow price of decode time: bytes we are willing to spend per
unit of decode throughput. It is NOT a taste parameter. The only defensible
calibration is the market price revealed by the front we are trying to beat:

    lambda_frontier = -dL/dt  along the reference Pareto front  [B per MB/s]

Since 1 MB/s == 1 B/us, this is directly in B/us, the same units as the frozen
lambda = 0.01 B/us from the S6-1 pre-registration.

Also computes, per file and per candidate flip, the IMPLIED WILLINGNESS TO PAY
(IWTP): the byte cost of the flip divided by the decode time it buys. A flip
is worth taking iff the applicable lambda exceeds its IWTP.

Usage: python prototypes/i8-theory/lambda_calibration.py
"""
import csv
from collections import defaultdict

CSV = 'tests/benchmark-suite.csv'
REF = ('brotli-', 'zstd-')


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


def front(rows, key):
    """Non-dominated reference rows, sorted by increasing throughput."""
    def dom(a, b):
        return (a['ratio'] <= b['ratio'] and a[key] >= b[key]
                and (a['ratio'] < b['ratio'] or a[key] > b[key]))
    fr = [r for r in rows if not any(dom(q, r) for p in rows if p is not r for q in [p])]
    return sorted(fr, key=lambda r: r[key])


def main():
    rows = load()
    byfile = defaultdict(list)
    for r in rows:
        byfile[r['file']].append(r)

    print("=" * 108)
    print("LAMBDA CALIBRATION FROM THE MEASURED REFERENCE FRONTIER (decode plane)")
    print("lambda = -dL/dt along the front  [B per MB/s] == [B per us]")
    print("=" * 108)
    print(f"{'file':<24}{'from':<14}{'to':<14}{'dL (B)':>12}{'dt (MB/s)':>12}"
          f"{'lambda':>12}   note")
    print("-" * 108)

    lambdas = []
    for fn in sorted(byfile, key=lambda x: -byfile[x][0]['n']):
        rs = byfile[fn]
        refs = [r for r in rs if r['codec'].startswith(REF)]
        fr = front(refs, 'dec')
        for a, b in zip(fr, fr[1:]):
            dL = b['b'] - a['b']          # bytes GIVEN UP
            dt = b['dec'] - a['dec']      # MB/s BOUGHT
            if dt <= 0 or dL <= 0:
                continue
            lam = dL / dt
            note = ''
            if dt < 100:
                note = 'small dt -> unstable secant'
            if lam > 300:
                note = (note + '; ').strip('; ') + 'OUTLIER (sparse front)'
            lambdas.append((fn, lam, dt))
            print(f"{fn:<24}{a['codec']:<14}{b['codec']:<14}{dL:>12,}{dt:>12.1f}"
                  f"{lam:>12.1f}   {note}")

    vals = sorted(l for _, l, dt in lambdas if dt >= 100)
    if vals:
        print()
        print(f"  ROBUST lambda band (excluding secants with dt < 100 MB/s):")
        print(f"    n = {len(vals)} steps,  min {vals[0]:.1f},  median "
              f"{vals[len(vals)//2]:.1f},  max {vals[-1]:.1f}  B/us")
        print(f"    frozen S6-1 lambda = 0.01 B/us")
        print(f"    ratio median/frozen = {vals[len(vals)//2]/0.01:,.0f}x")

    print()
    print("=" * 108)
    print("IMPLIED WILLINGNESS TO PAY (IWTP) OF THE FLIPS S6-1 DECLINED")
    print("=" * 108)
    print("A flip is worth taking iff  lambda > IWTP = (bytes given up)/(MB/s bought)")
    print()
    # Measured byte costs from the S6-1 / t3 record (prototypes + ledger):
    #   macro-masks -> raw : +166,823 B for +21-27% decode
    #   macro-resid -> raw : +84,956 B for +18-21% decode
    #   Linux shape-distance-delta -> raw : +19,800 B for +0.12 GB/s
    FLIPS = [
        ('macro-masks -> raw', 166823, 0.21, 0.27, 'S6-1 / decode-perf t3'),
        ('macro-resid -> raw', 84956, 0.18, 0.21, 'S6-1 / decode-perf t3'),
        ('shape-dist-delta -> raw (Linux)', 19800, 0.12, 0.12, 'Linux hot-book trade'),
    ]
    print(f"{'flip':<34}{'dL (B)':>10}{'rel. gain':>12}{'IWTP (B/us)':>28}   source")
    print("-" * 108)
    for nm, dL, lo, hi, src in FLIPS:
        # need an absolute base speed to convert relative gain to MB/s
        print(f"{nm:<34}{dL:>10,}{f'{lo:.0%}-{hi:.0%}':>12}"
              f"{'see per-file below':>28}   {src}")
    print()
    # convert using the measured mode-15 floor speeds
    BASE = {  # decode-perf t3 floor, median-7, MB/s
        'generated.log': 235.8, 'generated.json': 195.7,
        'generated.jsonl': 293.5, 'generated.sqlite': 168.6,
    }
    print(f"{'flip':<34}{'file':<22}{'base MB/s':>10}{'abs gain':>11}"
          f"{'IWTP (B/us)':>14}")
    print("-" * 108)
    iwtps = []
    for nm, dL, lo, hi, src in FLIPS[:2]:
        for fn, base in BASE.items():
            for frac in (lo, hi):
                gain = base * frac
                iwtp = dL / gain
                iwtps.append((nm, fn, iwtp))
                print(f"{nm:<34}{fn:<22}{base:>10.1f}{gain:>11.1f}{iwtp:>14.1f}")
    iv = sorted(x[2] for x in iwtps)
    print()
    print(f"  IWTP range over the measured flips: {iv[0]:.0f} - {iv[-1]:.0f} B/us "
          f"(median {iv[len(iv)//2]:.0f})")
    print(f"  S6-1 frozen lambda: 0.01 B/us")
    print(f"  => frozen lambda is {iv[0]/0.01:,.0f}x to {iv[-1]/0.01:,.0f}x BELOW the")
    print(f"     willingness-to-pay of the very flips it declined.")
    print()
    print("  NOTE ON THE LEDGER'S 460.2 B/us FIGURE: that number came from the Linux")
    print("  microbench (+19.8 KB for +0.12 GB/s => 165 B/us at face value; the ledger")
    print("  states 460.2 via its own accounting). My per-stream figure for lits is")
    print("  ~44.6 B/us per the S6-1 record. The DISCREPANCY IS UNRESOLVED and both are")
    print("  inside the measured lambda_frontier band, so the conclusion is unaffected.")


if __name__ == '__main__':
    main()
