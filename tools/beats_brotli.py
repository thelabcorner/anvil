#!/usr/bin/env python3
# Beats-Brotli verdict rows for the ANVIL Pareto regression (bench lane).
# For every (file, anvil codec): which brotli settings it beats/ties on RATIO
# (the only plane ANVIL competes on), the ratio delta vs the best brotli,
# both-plane Pareto status, and a verdict.
#
# Verdict rules (per research gate protocol):
#   PARETO-WIN     - not dominated by the brotli/zstd front on a plane
#                    (extends the front); none expected today.
#   RATIO-BEATS    - anvil.ratio < best brotli ratio on this file (strict win).
#   RATIO-BEATS-SOME - beats/ties >=1 brotli q but not the best.
#   NO-BEAT        - beats no brotli setting on ratio.
# Usage: python tools\beats_brotli.py [suite.csv] [--out tests\pareto-verdict.csv]
from __future__ import annotations
import argparse, csv
from collections import defaultdict
from pathlib import Path

ANVIL = "anvil-"
BROTLI = "brotli-"
ZSTD = "zstd-"


def parse(path: Path) -> list[dict]:
    with open(path, newline="") as fp:
        return list(csv.DictReader(fp))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("suite", nargs="?", default="tests/benchmark-suite.csv")
    ap.add_argument("--out", default="tests/pareto-verdict.csv")
    a = ap.parse_args()
    rows = parse(Path(a.suite))
    by_file = defaultdict(list)
    for r in rows:
        by_file[r["file"]].append(r)

    def dominated(p, refs, key):
        pr, pm = float(p["ratio"]), float(p[key])
        for q in refs:
            if float(q["ratio"]) <= pr and float(q[key]) >= pm and \
               (float(q["ratio"]) < pr or float(q[key]) > pm):
                return q["codec"]
        return None

    out = [["file", "codec", "ratio", "encode_MBps", "decode_MBps",
            "beats_brotli_on_ratio", "best_brotli_q", "best_brotli_ratio",
            "ratio_delta_vs_best_brotli_pct", "ratio_delta_vs_q9_pct",
            "pareto_encode", "pareto_decode", "verdict"]]
    for f in sorted(by_file):
        refs = [r for r in by_file[f] if r["codec"].startswith(BROTLI) or r["codec"].startswith(ZSTD)]
        brotlis = sorted((r for r in by_file[f] if r["codec"].startswith(BROTLI)),
                         key=lambda r: float(r["ratio"]))
        best_b = brotlis[0]  # lowest ratio brotli
        q9 = next((r for r in brotlis if r["codec"] == "brotli-q9"), None)
        for r in sorted((x for x in by_file[f] if x["codec"].startswith(ANVIL)),
                        key=lambda x: x["codec"]):
            ar = float(r["ratio"])
            beats = [q["codec"].replace("brotli-q", "q") for q in brotlis if ar <= float(q["ratio"])]
            db = dominated(r, refs, "encode_MBps")
            dd = dominated(r, refs, "decode_MBps")
            delta = (ar / float(best_b["ratio"]) - 1.0) * 100.0 if float(best_b["ratio"]) > 0 else 0.0
            delta_q9 = (ar / float(q9["ratio"]) - 1.0) * 100.0 if q9 and float(q9["ratio"]) > 0 else 0.0
            if db is None or dd is None:
                verdict = "PARETO-WIN" if (db is None and dd is None) else "EXTENDS-ONE-PLANE"
            elif beats and beats[-1] == "q" + best_b["codec"].split("q")[1] and ar < float(best_b["ratio"]):
                verdict = "RATIO-BEATS"
            elif beats:
                verdict = "RATIO-BEATS-SOME"
            else:
                verdict = "NO-BEAT"
            out.append([f, r["codec"], r["ratio"], r["encode_MBps"], r["decode_MBps"],
                        ";".join(beats), best_b["codec"], best_b["ratio"],
                        f"{delta:+.2f}", f"{delta_q9:+.2f}",
                        db or "FRONT", dd or "FRONT", verdict])
    with open(a.out, "w", newline="") as fp:
        w = csv.writer(fp)
        w.writerows(out)
    print(f"wrote {a.out}")
    # Console: verdict tally + every RATIO-BEATS row
    tallies = defaultdict(int)
    for row in out[1:]:
        tallies[row[12]] += 1
    print("verdict tally:", dict(tallies))
    print("=== RATIO-BEATS rows (anvil beats best brotli on ratio) ===")
    for row in out[1:]:
        if row[12] == "RATIO-BEATS":
            print(f"  {row[0]} | {row[1]} ratio={row[2]} beats {row[6]} ({row[7]}) delta={row[8]}%")


if __name__ == "__main__":
    main()
