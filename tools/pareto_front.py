#!/usr/bin/env python3
# Pareto-frontier analysis for ANVIL (bench lane).
# Implements agenda §4.6: a Pareto claim = an ANVIL row that is NOT dominated
# by the Brotli/Zstd reference front on a (ratio, throughput) plane, per file
# and aggregate. Lower ratio is better, higher MB/s is better.
#
# Usage:
#   python tools\pareto_front.py [suite.csv] [--out tests\pareto-baseline.csv]
# Reads the row-level tests\benchmark-suite.csv produced by bench_suite.py.
from __future__ import annotations
import argparse, csv, sys
from collections import defaultdict
from pathlib import Path

# Reference class v2 (I9-6, research-gate adopted): brotli + zstd + xz.
# GRID-THIN: zstd tiers 4-22 and brotli lw30 are NOT measured; xz-9e rows may carry
# process-level decode (label them). Co-list the v1 (brotli/zstd-only) grid whenever
# the v2 grid is reported. Same-transform controls (xz --delta / --x86) are
# SIDE-CHANNEL only (tests/xz-transform-controls-i9.csv) and are applied by the gate
# as a dual-bar qualifier, never as arbiter front rows.
REF_PREFIXES = ("brotli-", "zstd-", "xz-")
ANVIL_PREFIXES = ("anvil-",)


def parse(path: Path) -> list[dict]:
    with open(path, newline="") as fp:
        return list(csv.DictReader(fp))


def is_ref(codec: str) -> bool:
    return codec.startswith(REF_PREFIXES)


def dominated(p: dict, refs: list[dict], mbps_key: str) -> dict | None:
    """Return a dominating reference point if p is dominated on the plane, else None.
    ref q dominates p iff q.ratio <= p.ratio AND q.mbps >= p.mbps (one strict)."""
    pr, pm = float(p["ratio"]), float(p[mbps_key])
    for q in refs:
        qr, qm = float(q["ratio"]), float(q[mbps_key])
        if qr <= pr and qm >= pm and (qr < pr or qm > pm):
            return q
    return None


def reference_front(refs: list[dict], mbps_key: str) -> list[dict]:
    """Non-dominated reference rows: the tradeoff frontier Brotli/Zstd own."""
    return [r for r in refs if dominated(r, refs, mbps_key) is None]


def analyze(rows: list[dict], label: str, mbps_key: str, out) -> None:
    refs = [r for r in rows if is_ref(r["codec"])]
    anvil = [r for r in rows if r["codec"].startswith(ANVIL_PREFIXES)]
    if not refs:
        return
    front = reference_front(refs, mbps_key)
    for a in anvil:
        d = dominated(a, refs, mbps_key)
        status = "DOMINATED" if d else "EXTENDS_FRONT"
        out.append([label, mbps_key, a["codec"], a["ratio"], a[mbps_key], status,
                    d["codec"] if d else ""])
    for r in front:
        out.append([label, mbps_key, r["codec"], r["ratio"], r[mbps_key], "FRONT", ""])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("suite", nargs="?", default="tests/benchmark-suite.csv")
    ap.add_argument("--out", default="tests/pareto-baseline.csv")
    a = ap.parse_args()
    rows = parse(Path(a.suite))
    by_file = defaultdict(list)
    for r in rows:
        by_file[r["file"]].append(r)
    out = [["label", "plane", "codec", "ratio", "MBps", "status", "dominated_by"]]
    for f in sorted(by_file):
        for plane in ("encode_MBps", "decode_MBps"):
            analyze(by_file[f], f, plane, out)
    # Aggregate: total compressed / total input per codec.
    agg = {}
    for r in rows:
        agg.setdefault(r["codec"], []).append(r)
    agg_rows = []
    for codec, rs in agg.items():
        ib = sum(int(x["input_bytes"]) for x in rs)
        cb = sum(int(x["compressed_bytes"]) for x in rs)
        et = sum(int(x["input_bytes"]) / 1e6 / max(float(x["encode_MBps"]), 1e-12) for x in rs)
        dt = sum(int(x["input_bytes"]) / 1e6 / max(float(x["decode_MBps"]), 1e-12) for x in rs)
        agg_rows.append({"codec": codec, "ratio": f"{cb/ib:.6f}",
                         "encode_MBps": f"{ib/1e6/et:.3f}", "decode_MBps": f"{ib/1e6/dt:.3f}"})
    for plane in ("encode_MBps", "decode_MBps"):
        analyze(agg_rows, "AGGREGATE", plane, out)
    with open(a.out, "w", newline="") as fp:
        w = csv.writer(fp)
        w.writerows(out)
    print(f"wrote {a.out}")
    # Console summary: which ANVIL rows are not dominated?
    print("=== ANVIL rows extending the reference front ===")
    found = False
    for row in out:
        if len(row) >= 6 and row[2].startswith(ANVIL_PREFIXES) and row[5] == "EXTENDS_FRONT":
            print(f"  {row[0]} | {row[1]} | {row[2]} ratio={row[3]} {row[4]} MBps")
            found = True
    if not found:
        print("  (none) — every ANVIL row is dominated by the Brotli/Zstd front. Baseline is not a Pareto win.")
    print("GRID-THIN: ref class = brotli q1-q11 + zstd 1/3/9/19 + xz-9e; zstd 4-22 and brotli lw30")
    print("           tiers unmeasured; xz-9e decode may be process-level; transform controls side-channel only.")


if __name__ == "__main__":
    main()
