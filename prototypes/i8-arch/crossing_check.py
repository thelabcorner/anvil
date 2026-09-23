# -*- coding: utf-8 -*-
"""Recompute the synth-arith crossing condition using pareto_front.py's OWN
dominance function, against the EXACT rounding the bench harness emits.

Key facts verified in-source before writing this:
  - tools/bench_native.cpp:82-84  -> std::fixed << std::setprecision(3), so the
    ratio column in tests/benchmark-suite.csv carries only 3 decimals.
  - tools/pareto_front.py:dominated() -> q dominates p iff q.ratio <= p.ratio
    AND q.mbps >= p.mbps (one strict).  p EXTENDS_FRONT iff NO ref dominates.
So the CSV ratio is quantised, and the arbiter compares QUANTISED values.
"""
import sys, csv, io
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
import pareto_front as pf

N = 256000  # synth-arith.bin input bytes

# Read the committed reference rows for synth-arith straight out of the suite.
rows = pf.parse(Path("tests/benchmark-suite.csv"))
sa = [r for r in rows if r["file"].endswith("synth-arith.bin")]
refs = [r for r in sa if pf.is_ref(r["codec"])]

print("=== reference rows on synth-arith.bin (decode plane) ===")
for r in sorted(refs, key=lambda x: float(x["ratio"])):
    print(f"  {r['codec']:<12} bytes={int(r['compressed_bytes']):>7} "
          f"ratio={r['ratio']} dec={float(r['decode_MBps']):8.3f}")


def ratio_str(nbytes):
    """Exactly what bench_native prints: fixed, 3 decimals."""
    return f"{nbytes / N:.3f}"


def status(nbytes, dec_mbps):
    p = {"codec": "anvil-mode16", "ratio": ratio_str(nbytes),
         "decode_MBps": f"{dec_mbps:.3f}"}
    d = pf.dominated(p, refs, "decode_MBps")
    return ("EXTENDS_FRONT" if d is None else f"DOMINATED by {d['codec']}"), p["ratio"]


print("\n=== byte BUDGET sweep: largest byte count that still crosses ===")
print("    (smaller bytes = smaller ratio = harder to dominate, so the budget")
print("     is a MAXIMUM, not a minimum - an earlier version of this script")
print("     searched upward and reported the floor of the range. Corrected.)")
for dec in (150.0, 187.0, 187.6, 188.0, 200.0, 250.0, 326.0, 400.0, 886.0):
    hi = None
    for L in range(60000, 260000):
        st, _ = status(L, dec)
        if st == "EXTENDS_FRONT":
            hi = L
        else:
            break  # monotone: once dominated at L, dominated for all larger L
    print(f"  decode={dec:7.1f} MB/s -> max bytes that crosses = {hi}")

print("\n=== specific candidate rows ===")
cands = [
    (87013, 187.0, "brief's stated bar, just under"),
    (87013, 188.0, "brief's stated bar, over decode cliff"),
    (89363, 188.0, "harness wire, just over decode cliff"),
    (89363, 200.0, "harness wire @200"),
    (89363, 300.0, "harness wire @300"),
    (100000, 200.0, "100 KB @200"),
    (106000, 200.0, "106 KB @200 (just under zstd-19 ratio)"),
    (107000, 200.0, "107 KB @200 (over zstd-19 ratio)"),
    (256022, 389.7, "today's best row (mdl-rans)"),
]
for L, d, note in cands:
    st, r = status(L, d)
    print(f"  {L:>7} B  ratio={r}  dec={d:7.1f}  -> {st:<28} ({note})")
