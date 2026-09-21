# -*- coding: utf-8 -*-
"""pnra lane — LOCAL re-implementation of the arbiter dominance predicate for
classifying PROTOTYPE rows on synth-arith.bin.

IMPORTANT: this file does NOT run or import tools/pareto_front.py (bench lane).
It re-derives the documented rule from the published semantics:

    q dominates p  iff  q.ratio <= p.ratio  AND  q.mbps >= p.mbps  (one strict)

on the 3-decimal quantised ratio string that tools/bench_native.cpp prints
(std::fixed << setprecision(3)).  This is the same rule arch re-implemented in
prototypes/i8-arch/crossing_check.py; the spot checks below reproduce arch's
published numbers as a self-validation.

A prototype row is NOT an ANVIL row and is NOT in the suite. Any frontier
language requires integration into src/anvil.cpp (arch), a suite row, two
hash-identical arbiter runs and bench sign-off (PR-4). This script only
classifies for ranking and reports the byte budget at a given decode speed.

Usage:  python corridor_check.py
"""
import csv
from pathlib import Path

N = 256000  # synth-arith.bin input bytes
ROOT = Path(__file__).resolve().parents[2]
SUITE = ROOT / "tests/benchmark-suite.csv"


def ratio_str(nbytes: int) -> str:
    return f"{nbytes / N:.3f}"


def load_refs():
    rows = list(csv.DictReader(open(SUITE, newline="")))
    sa = [r for r in rows if r["file"].endswith("synth-arith.bin")]
    return [r for r in sa if not r["codec"].startswith("anvil")]


REFS = load_refs()


def dominated(p, plane="decode_MBps"):
    for q in REFS:
        if float(q["ratio"]) <= float(p["ratio"]) and float(q[plane]) >= float(p[plane]):
            if float(q["ratio"]) < float(p["ratio"]) or float(q[plane]) > float(p[plane]):
                return q
    return None


def status(nbytes: int, dec: float):
    p = {"codec": "pnra-prototype", "ratio": ratio_str(nbytes), "decode_MBps": f"{dec:.3f}"}
    d = dominated(p)
    return ("EXTENDS_FRONT" if d is None else f"DOMINATED by {d['codec']}"), p["ratio"], d


def max_crossing_bytes(dec: float) -> int:
    hi = None
    for L in range(60000, 260001):
        st, _, _ = status(L, dec)
        if st == "EXTENDS_FRONT":
            hi = L
        else:
            break  # monotone: dominated for all larger L
    return hi


def main():
    print("=== reference rows on synth-arith.bin (from committed tests/benchmark-suite.csv) ===")
    for r in sorted(REFS, key=lambda x: float(x["ratio"])):
        print(f"  {r['codec']:<10} bytes={int(r['compressed_bytes']):>7} ratio={r['ratio']:<6} dec={float(r['decode_MBps']):8.3f}")

    print("\n=== self-validation: arch's published spot checks ===")
    checks = [
        (87013, 187.0, "EF expected"),
        (87013, 188.0, "EF expected"),
        (89363, 188.0, "EF expected"),
        (106000, 200.0, "EF expected"),
        (107000, 200.0, "DOMINATED by zstd-19 expected"),
    ]
    for L, d, note in checks:
        st, r, _ = status(L, d)
        print(f"  {L:>7} B ratio={r} dec={d:>7.1f} -> {st:<26} ({note})")

    print("\n=== byte budget sweep: max bytes that still EXTENDS_FRONT ===")
    budgets = {}
    for dec in (150.0, 187.0, 187.6, 188.0, 200.0, 250.0, 326.0, 400.0, 462.0, 500.0, 886.0):
        b = max_crossing_bytes(dec)
        budgets[dec] = b
        print(f"  decode={dec:7.1f} MB/s -> max crossing bytes = {b}")

    print("\n=== measured PROTOTYPE rows (window w-pnra-20260912T062115Z) ===")
    rows = [
        ("pack3/transmitted", 12921, 462.511, 208.639, 472.936),
        ("pack3/derived", 12936, 462.344, 204.734, 463.265),
        ("raw2/derived", 16134, 460.597, 225.055, 466.133),
        ("brotli/derived", 13456, 223.835, 190.533, 229.741),
        ("pack5/ddelta", 19817, 407.903, 217.465, 408.815),
        ("raw3/ddelta", 64113, 486.970, 205.128, 489.203),
        ("literal-only (control)", 256049, 587.695, 333.507, 592.867),
    ]
    print(f"{'arm':<22} {'wire':>7} {'ratio':>7} {'dec_med':>9} {'status':<24} {'budget@dec':>10} {'DEGEN':>6}")
    out = []
    for name, L, d, dmin, dmax in rows:
        st, r, dq = status(L, d)
        # gate-ruling c1: DEGENERATE is reserved for NON-DOMINATED rows at ratio >= 0.95
        degen = "YES" if (st == "EXTENDS_FRONT" and float(r) >= 0.95) else "n/a"
        b = max_crossing_bytes(d)
        print(f"{name:<22} {L:>7} {r:>7} {d:>9.3f} {st:<24} {b:>10} {degen:>6}")
        out.append((name, L, r, d, dmin, dmax, st, b, degen))

    # FRONT-GAP bracket test (gate ruling R-2): exists q_lo (smaller AND slower)
    # and q_hi (larger AND faster) with p between them on BOTH axes?
    print("\n=== FRONT-GAP bracket test (mechanical, gate ruling R-2) ===")
    for name, L, r, d, *_ in out:
        bracket = None
        pr = float(r)
        for a in REFS:
            for b in REFS:
                if float(a["ratio"]) < pr and float(a["decode_MBps"]) < d \
                   and float(b["ratio"]) > pr and float(b["decode_MBps"]) > d:
                    bracket = (a["codec"], b["codec"])
        print(f"  {name:<22} bracket={'none -> not FRONT-GAP by this mechanical test' if bracket is None else bracket}")


if __name__ == "__main__":
    main()
