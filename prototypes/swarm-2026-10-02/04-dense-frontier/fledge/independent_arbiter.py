#!/usr/bin/env python3
"""Independent reproduction of the Q0 dense retained-artifact closure tuple.

Codec-free arithmetic over ONE retained CSV. No corpus, no codec, no timer.
Written by Fledge to independently check Q0-DENSE-RETAINED-RESULT.md, which was
produced by a different script (parity_sweep.py) that this audit has not read.

Implements the adopted doctrine of docs/gate-ruling-i8-pareto-win.md R-2 in the
adopted order:

    DOMINATED -> DEGENERATE -> FRONT-GAP -> FRONT-CROSSING

  dominated(p): exists a reference q with q.ratio <= p.ratio AND q.mbps >= p.mbps
                AND at least one strict.
  degenerate(p): non-dominated AND p.ratio >= 0.95.
  bracket(p):    exists reference q_lo, q_hi on the SAME file and SAME plane with
                 q_lo.ratio < q_hi.ratio AND q_lo.mbps < q_hi.mbps, with p strictly
                 between them on BOTH axes  ->  FRONT-GAP, else FRONT-CROSSING.

epsilon variant: a bracket counts only if its separation on BOTH axes is at least
                 eps (relative: q_hi.axis >= q_lo.axis * (1+eps) on mbps and
                 q_hi.ratio >= q_lo.ratio * (1+eps) on ratio). eps=0 reproduces the
                 literal adopted predicate.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path

REF_PREFIXES = ("brotli-", "zstd-", "xz-")
ANVIL_PREFIXES = ("anvil-",)
DEGENERATE_RATIO = 0.95


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def load(path: Path) -> list[dict]:
    with path.open(newline="", encoding="utf-8") as fh:
        return list(csv.DictReader(fh))


def is_ref(codec: str) -> bool:
    return codec.startswith(REF_PREFIXES)


def dominated(p: dict, refs: list[dict], mbps_key: str) -> dict | None:
    pr, pm = float(p["ratio"]), float(p[mbps_key])
    for q in refs:
        qr, qm = float(q["ratio"]), float(q[mbps_key])
        if qr <= pr and qm >= pm and (qr < pr or qm > pm):
            return q
    return None


def bracket(p: dict, refs: list[dict], mbps_key: str, eps: float) -> tuple | None:
    """Return (q_lo, q_hi) if a bracket exists under the minimum-separation eps."""
    pr, pm = float(p["ratio"]), float(p[mbps_key])
    for lo in refs:
        lr, lm = float(lo["ratio"]), float(lo[mbps_key])
        if not (lr < pr and lm < pm):
            continue
        for hi in refs:
            hr, hm = float(hi["ratio"]), float(hi[mbps_key])
            if not (hr > pr and hm > pm):
                continue
            if not (lr < hr and lm < hm):
                continue
            if hr < lr * (1.0 + eps):
                continue
            if lm <= 0 or hm < lm * (1.0 + eps):
                continue
            return (lo, hi)
    return None


def classify(rows: list[dict], mbps_key: str, eps: float) -> tuple[dict, list]:
    by_file: dict[str, list[dict]] = defaultdict(list)
    for r in rows:
        by_file[r["file"]].append(r)
    counts = {"DOMINATED": 0, "DEGENERATE": 0, "FRONT-GAP": 0, "FRONT-CROSSING": 0}
    detail = []
    for fname in sorted(by_file):
        group = by_file[fname]
        refs = [r for r in group if is_ref(r["codec"])]
        for p in [r for r in group if r["codec"].startswith(ANVIL_PREFIXES)]:
            dom = dominated(p, refs, mbps_key)
            if dom is not None:
                cls = "DOMINATED"
            elif float(p["ratio"]) >= DEGENERATE_RATIO:
                cls = "DEGENERATE"
            else:
                cls = "FRONT-GAP" if bracket(p, refs, mbps_key, eps) else "FRONT-CROSSING"
            counts[cls] += 1
            detail.append(
                {
                    "file": fname,
                    "plane": mbps_key,
                    "codec": p["codec"],
                    "ratio": float(p["ratio"]),
                    "mbps": float(p[mbps_key]),
                    "class": cls,
                    "dominated_by": dom["codec"] if dom else "",
                }
            )
    return counts, detail


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="?", default="tests/benchmark-suite.frozen-bdc90474.plus-xz.csv")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()
    path = Path(args.csv)
    rows = load(path)
    digest = sha256(path)

    anvil = sorted({r["codec"] for r in rows if r["codec"].startswith(ANVIL_PREFIXES)})
    refs = sorted({r["codec"] for r in rows if is_ref(r["codec"])})
    files = sorted({r["file"] for r in rows})

    report = {
        "csv": str(path),
        "csv_sha256": digest,
        "rows": len(rows),
        "anvil_configs": len(anvil),
        "reference_codecs": len(refs),
        "reference_class": refs,
        "files": len(files),
        "expected_cells": len(anvil) * len(files) * 2,
        "epsilon_sweep": {},
    }

    print(f"csv sha256 : {digest}")
    print(f"rows {len(rows)} = {len(files)} files x {len(anvil) + len(refs)} codecs")
    print(f"ANVIL configs {len(anvil)} | reference class {len(refs)}: {', '.join(refs)}")
    print(f"expected ANVIL row-plane cells = {len(anvil)}*{len(files)}*2 = {report['expected_cells']}")
    print()

    for eps in (0.0, 0.02, 0.05, 0.10, 0.25):
        totals = {"DOMINATED": 0, "DEGENERATE": 0, "FRONT-GAP": 0, "FRONT-CROSSING": 0}
        all_detail = []
        for plane in ("encode_MBps", "decode_MBps"):
            counts, detail = classify(rows, plane, eps)
            for k in totals:
                totals[k] += counts[k]
            all_detail.extend(detail)
        n = sum(totals.values())
        tup = (
            f"DOM {totals['DOMINATED']} / DEG {totals['DEGENERATE']} / "
            f"GAP {totals['FRONT-GAP']} / CROSS {totals['FRONT-CROSSING']}"
        )
        report["epsilon_sweep"][f"{eps}"] = {"totals": totals, "cells": n}
        print(f"eps={eps:<5} {tup}   total={n}")
        if eps == 0.0:
            report["detail_eps0"] = all_detail
            gaps = [d for d in all_detail if d["class"] == "FRONT-GAP"]
            degen = sorted({(d["file"], d["codec"]) for d in all_detail if d["class"] == "DEGENERATE"})
            crosses = [d for d in all_detail if d["class"] == "FRONT-CROSSING"]
            print(f"  FRONT-GAP cells ({len(gaps)}):")
            for d in gaps:
                print(
                    f"    {d['file']:<34} {d['codec']:<26} r={d['ratio']:<6} {d['mbps']:>9} MB/s"
                )
            print(f"  DEGENERATE distinct (file,codec) = {len(degen)}; files: "
                  f"{sorted({f for f, _ in degen})}")
            print(f"  FRONT-CROSSING cells = {len(crosses)}")

    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())