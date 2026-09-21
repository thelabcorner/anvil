#!/usr/bin/env python3
"""theory/i8 — crossing check for arch's mode-16 candidate row.

Verdict computation uses EXACTLY the rule in tools/pareto_front.py:
  ref q dominates candidate a iff  q.ratio <= a.ratio AND q.mbps >= a.mbps
  (with at least one strict).

Two things are computed:
 1. STATUS of the candidate at a given decode speed — is it dominated?
 2. The ISO-CROSSING BUDGET L*(t) = N * R_ref(t), R_ref(t) = min ratio among
    reference rows with decode >= t. Blocks recomputed on EXACT integer byte
    counts (the CSV stores ratio rounded to 4dp; I use bytes/len to avoid
    propagating that rounding).

Also classifies GENUINE-CROSSING vs FRONT-GAP-FILL per
deliverable/theory-i8-iso-crossing: a row is a FRONT-GAP FILL if it lies
inside an axis-rectangle (r_lo, r_hi) x (t_lo, t_hi) whose corners are
mutually non-dominating reference rows and which contains no reference row.
A row STRICTLY OUTSIDE the reference ratio range is NOT in any such rectangle.

Usage: python prototypes/i8-theory/crossing_check.py [bytes] [decode_MBps]
"""
import csv, sys
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


def dominated(a_r, a_t, refs, key='dec'):
    """Return dominating ref, or None. EXACT pareto_front.py semantics."""
    for q in refs:
        qr, qm = q['b'] / q['n'], q[key]
        if qr <= a_r and qm >= a_t and (qr < a_r or qm > a_t):
            return q
    return None


def main():
    rows = load()
    byfile = defaultdict(list)
    for r in rows:
        byfile[r['file']].append(r)

    CAND = [
        ('synth-arith.bin', 75455, 'arch mode-16 ARI-REF (landed)'),
        ('synth-arith.bin', 89363, 'ARI-REF harness wire (EXP. Z, prior)'),
        ('synth-arith.bin', 12717, 'entropy floor (theory, derived)'),
        ('synth-arith.bin', 87013, 'brotli-q11 parity'),
    ]
    # decode speeds to probe; None = "any"
    SPEEDS = [0.0, 50.0, 100.0, 139.7, 163.7, 187.5, 188.8, 204.6,
              325.5, 362.5, 839.3]

    for fn in sorted(set(c[0] for c in CAND)):
        rs = byfile[fn]
        n = rs[0]['n']
        refs = [r for r in rs if r['codec'].startswith(REF)]
        print("=" * 100)
        print(f"{fn}  N={n:,}")
        print("  reference rows (ratio computed from EXACT bytes/N):")
        for q in sorted(refs, key=lambda x: x['dec']):
            print(f"    {q['codec']:<12}{q['b']:>8,} B  r={q['b']/q['n']:.6f}  "
                  f"dec={q['dec']:>7.1f}  enc={q['enc']:>7.2f}")
        min_ref_ratio = min(q['b'] / q['n'] for q in refs)
        print(f"  minimum reference ratio = {min_ref_ratio:.6f}")
        print()
        for cand_fn, L, note in CAND:
            if cand_fn != fn:
                continue
            r = L / n
            print(f"  CANDIDATE {note:<40} {L:,} B  ratio {r:.6f}")
            print(f"    {'decode MB/s':>14}  {'status':<22}{'dominated by':<16} "
                  f"{'budget L*(t)':>14}")
            for t in SPEEDS:
                d = dominated(r, t, refs)
                # budget at this speed
                avail = [q for q in refs if q['dec'] >= t]
                R = min((q['b'] / q['n'] for q in avail), default=float('inf'))
                bud = n * R
                st = 'DOMINATED' if d else '*** EXTENDS_FRONT ***'
                budstr = ('{:>14,}'.format(int(bud)) if bud != float('inf')
                          else 'CROSSES'.rjust(14))
                print(f"    {t:>14.1f}  {st:<22}{(d['codec'] if d else '-'):<16} "
                      f"{budstr:>14}")
            # classification
            below_all = r < min_ref_ratio
            print(f"    CLASSIFICATION: ", end='')
            if below_all:
                print(f"GENUINE-CROSSING — ratio {r:.6f} is STRICTLY BELOW every "
                      f"reference ratio")
                print(f"      (min ref {min_ref_ratio:.6f}). No reference row can "
                      f"dominate it on ratio at")
                print(f"      ANY decode speed, so it is outside every "
                      f"axis-rectangle gap by construction.")
            else:
                print("requires the decode axis; check per-speed table above.")
            print()


if __name__ == '__main__':
    main()
