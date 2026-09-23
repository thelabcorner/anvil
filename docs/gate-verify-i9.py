#!/usr/bin/env python3
"""gate-verify-i9.py - research-gate's claim-verification harness (I9).

Recomputes, from a benchmark-suite CSV and optionally a pareto-baseline CSV,
the ONLY admissible frontier tally for this swarm:

    tuple = (non-dominated | FRONT-GAP | FRONT-CROSSING | DEGENERATE)
    plus dominated-cell fraction, per-file and including AGGREGATE.

Semantics are the I8 gate rulings (docs/gate-ruling-i8-pareto-win.md R-2,
ledger PART XIII S2) with the PR-2 v4 amendment (the DEGENERATE test applies
to the ROW's own ratio, not the file's current best row).

Reference class: codecs prefixed 'brotli-' or 'zstd-' - identical to
tools/pareto_front.py, which remains the sole issuer of the raw
EXTENDS_FRONT boolean. This script does NOT re-issue that boolean; it reads
the same predicate and adds the gate's sufficiency classification.
That keeps the two-lane split intact: bench owns the tool, gate owns the
reading.

Classification (applied in this order to every ANVIL row on every plane):
  1. DOMINATED      - some reference row q has q.ratio <= p.ratio and
                      q.mbps >= p.mbps, one strict (pareto_front.py rule).
  2. DEGENERATE     - non-dominated AND p.ratio >= 0.95 (PR-2, row-level).
  3. FRONT-GAP      - non-dominated AND a gap bracket exists: two reference
                      rows q_lo, q_hi with q_lo.ratio < q_hi.ratio and
                      q_lo.mbps < q_hi.mbps and p inside [q_lo, q_hi] on
                      both axes (R-2 mechanical test).
  4. FRONT-CROSSING - non-dominated AND not DEGENERATE AND no bracket.

Usage:
  python docs/gate-verify-i9.py --suite tests/benchmark-suite.csv
  python docs/gate-verify-i9.py --suite S.csv --baseline B.csv   # + cross-check
Exit code 0 always unless --strict and an inconsistency is found.
"""
from __future__ import annotations
import argparse
import csv
import os
import sys
from collections import defaultdict

# Default reference class = bench v2 (I9-6: xz -9e adopted). Override with
# --refs ("brotli-,zstd-" reproduces grid v1).
REF_PREFIXES = ("brotli-", "zstd-", "xz-")
ANVIL_PREFIX = "anvil-"
DEGENERATE_RATIO = 0.95


def load(path: str) -> list[dict]:
    with open(path, newline="") as fp:
        return list(csv.DictReader(fp))


def is_ref(codec: str) -> bool:
    return codec.startswith(REF_PREFIXES)


def is_anvil(codec: str) -> bool:
    return codec.startswith(ANVIL_PREFIX)


def dominated_by(p: dict, refs: list[dict], mbps_key: str) -> dict | None:
    """pareto_front.py predicate, verbatim semantics."""
    pr, pm = float(p["ratio"]), float(p[mbps_key])
    for q in refs:
        qr, qm = float(q["ratio"]), float(q[mbps_key])
        if qr <= pr and qm >= pm and (qr < pr or qm > pm):
            return q
    return None


def gap_bracket(p: dict, refs: list[dict], mbps_key: str) -> list[tuple[str, str]]:
    """R-2 bracket test. Returns every witnessing (q_lo.codec, q_hi.codec)."""
    pr, pm = float(p["ratio"]), float(p[mbps_key])
    out = []
    for ql in refs:
        rl, ml = float(ql["ratio"]), float(ql[mbps_key])
        if not (rl <= pr and ml <= pm):
            continue
        for qh in refs:
            rh, mh = float(qh["ratio"]), float(qh[mbps_key])
            if rl < rh and ml < mh and pr <= rh and pm <= mh:
                out.append((ql["codec"], qh["codec"]))
    return out


def classify(p: dict, refs: list[dict], mbps_key: str,
             controls: dict[str, float] | None = None) -> dict:
    pr, pm = float(p["ratio"]), float(p[mbps_key])
    dom = dominated_by(p, refs, mbps_key)
    if dom is not None:
        return {"class": "DOMINATED", "ratio": pr, "mbps": pm,
                "by": dom["codec"], "brackets": [], "dual_bar": None}
    if pr >= DEGENERATE_RATIO:
        return {"class": "DEGENERATE", "ratio": pr, "mbps": pm,
                "by": "", "brackets": [], "dual_bar": None}
    ctrl = None
    if controls:
        key = os.path.basename(p.get("file", "")).lower()
        ctrl = controls.get(key)
    if ctrl is not None and ctrl < pr:
        return {"class": "FRONT-GAP", "ratio": pr, "mbps": pm, "by": "",
                "brackets": [], "dual_bar": ctrl}
    br = gap_bracket(p, refs, mbps_key)
    if br:
        return {"class": "FRONT-GAP", "ratio": pr, "mbps": pm,
                "by": "", "brackets": br, "dual_bar": None}
    return {"class": "FRONT-CROSSING", "ratio": pr, "mbps": pm,
            "by": "", "brackets": [], "dual_bar": None}


def aggregate_rows(rows: list[dict]) -> list[dict]:
    """Same aggregation as tools/pareto_front.py: byte-weighted ratio, and
    throughput = total_input / sum(per-row decode time) (harmonic)."""
    agg: dict[str, list[dict]] = defaultdict(list)
    for r in rows:
        agg[r["codec"]].append(r)
    out = []
    for codec, rs in agg.items():
        ib = sum(int(x["input_bytes"]) for x in rs)
        cb = sum(int(x["compressed_bytes"]) for x in rs)
        et = sum(int(x["input_bytes"]) / 1e6 / max(float(x["encode_MBps"]), 1e-12)
                 for x in rs)
        dt = sum(int(x["input_bytes"]) / 1e6 / max(float(x["decode_MBps"]), 1e-12)
                 for x in rs)
        out.append({"codec": codec, "ratio": f"{cb / ib:.6f}",
                    "encode_MBps": f"{ib / 1e6 / et:.3f}",
                    "decode_MBps": f"{ib / 1e6 / dt:.3f}",
                    "input_bytes": str(ib), "compressed_bytes": str(cb)})
    return out


def evaluate(rows: list[dict], label: str,
             controls: dict[str, float] | None = None):
    """Returns (counts, findings, total_cells) over both planes."""
    by_file: dict[str, list[dict]] = defaultdict(list)
    for r in rows:
        by_file[r.get("file", label)].append(r)
    counts = {"DOMINATED": 0, "DEGENERATE": 0, "FRONT-GAP": 0,
              "FRONT-CROSSING": 0}
    findings = []
    cells = 0
    for f in sorted(by_file):
        refs = [r for r in by_file[f] if is_ref(r["codec"])]
        for plane in ("encode_MBps", "decode_MBps"):
            for a in [r for r in by_file[f] if is_anvil(r["codec"])]:
                cells += 1
                c = classify(a, refs, plane, controls)
                counts[c["class"]] += 1
                if c["class"] != "DOMINATED":
                    findings.append((label, f, plane, a["codec"], c))
    return counts, findings, cells


def fmt_tuple(counts: dict, cells: int) -> str:
    nd = cells - counts["DOMINATED"]
    return (f"{nd} non-dominated | {counts['FRONT-GAP']} FRONT-GAP | "
            f"{counts['FRONT-CROSSING']} FRONT-CROSSING | "
            f"{counts['DEGENERATE']} DEGENERATE | "
            f"{counts['DOMINATED']}/{cells} dominated")


def main() -> int:
    global REF_PREFIXES
    ap = argparse.ArgumentParser()
    ap.add_argument("--suite", default="tests/benchmark-suite.csv")
    ap.add_argument("--baseline", default=None)
    ap.add_argument("--refs", default=",".join(REF_PREFIXES),
                    help="comma-separated reference prefixes; default v2 "
                         "(brotli-,zstd-,xz-); use brotli-,zstd- for grid v1")
    ap.add_argument("--transform-controls", default=None,
                    help="CSV of same-transform controls (file,ratio) applied "
                         "as the I9-6 dual-bar qualifier")
    ap.add_argument("--strict", action="store_true")
    a = ap.parse_args()

    REF_PREFIXES = tuple(x.strip() for x in a.refs.split(",") if x.strip())
    controls = None
    if a.transform_controls:
        controls = {}
        for r in load(a.transform_controls):
            key = os.path.basename(r["file"]).lower()
            ratio = float(r["ratio"])
            controls[key] = min(controls.get(key, 1.0), ratio)
    print(f"ref class: {','.join(REF_PREFIXES)}" +
          (f"  | dual-bar controls: {len(controls)}" if controls else ""))

    rows = load(a.suite)
    per_counts, per_findings, per_cells = evaluate(rows, "per-file", controls)

    agg = aggregate_rows(rows)
    agg_counts, agg_findings, agg_cells = evaluate(agg, "AGGREGATE", None)

    print(f"suite: {a.suite}  ({len(rows)} rows)")
    print(f"per-file : {fmt_tuple(per_counts, per_cells)}")
    print(f"AGGREGATE: {fmt_tuple(agg_counts, agg_cells)}")
    total_cells = per_cells + agg_cells
    total_dom = per_counts["DOMINATED"] + agg_counts["DOMINATED"]
    nd = total_cells - total_dom
    print(f"combined : {nd} non-dominated | {total_dom}/{total_cells} dominated")
    print()
    print("non-dominated rows:")
    for scope, f, plane, codec, c in per_findings + agg_findings:
        br = ("  bracket=" + ",".join(f"{x}<->{y}" for x, y in c["brackets"])
              if c["brackets"] else "")
        db = ("  dual-bar: control ratio %.4f < row ratio -> FRONT-GAP, not a "
              "crossing" % c["dual_bar"]) if c.get("dual_bar") is not None else ""
        print(f"  {scope:9s} {f:40s} {plane:11s} {codec:24s} "
              f"ratio={c['ratio']:.3f} MBps={c['mbps']:.3f} -> {c['class']}{br}{db}")

    status = 0
    if a.baseline:
        base = load(a.baseline)
        # suite-derived status per (label, plane, codec) for anvil rows
        suite_by_key = {}
        by_file: dict[str, list[dict]] = defaultdict(list)
        for r in rows:
            by_file[r["file"]].append(r)
        for f in sorted(by_file):
            refs = [r for r in by_file[f] if is_ref(r["codec"])]
            for plane in ("encode_MBps", "decode_MBps"):
                for x in [r for r in by_file[f] if is_anvil(r["codec"])]:
                    suite_by_key[(f, plane, x["codec"])] = classify(x, refs, plane)["class"]
        for f in [None]:
            refs = [r for r in agg if is_ref(r["codec"])]
            for plane in ("encode_MBps", "decode_MBps"):
                for x in [r for r in agg if is_anvil(r["codec"])]:
                    suite_by_key[("AGGREGATE", plane, x["codec"])] = classify(x, refs, plane)["class"]
        mismatches = 0
        checked = 0
        for b in base:
            if not is_anvil(b["codec"]):
                continue
            key = (b["label"], b["plane"], b["codec"])
            if key not in suite_by_key:
                continue
            checked += 1
            want = {"DOMINATED": "DOMINATED", "EXTENDS_FRONT": None}.get(b["status"], b["status"])
            got = suite_by_key[key]
            if want == "DOMINATED" and got != "DOMINATED":
                print(f"BASELINE MISMATCH {key}: baseline=DOMINATED suite={got}")
                mismatches += 1
            elif want is None and got == "DOMINATED":
                print(f"BASELINE MISMATCH {key}: baseline=EXTENDS_FRONT suite=DOMINATED")
                mismatches += 1
        print()
        print(f"baseline cross-check: {checked} anvil cells, {mismatches} status mismatches")
        if mismatches and a.strict:
            status = 1
    return status


if __name__ == "__main__":
    sys.exit(main())
