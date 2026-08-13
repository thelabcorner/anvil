#!/usr/bin/env python3
# Run-to-run variance (noise floor) for ANVIL benchmarks (bench lane).
# Agenda §4.3: a mechanism's Delta must exceed run-to-run variance on the same
# file; bench owns that estimate. Runs bench_native R times per file and
# reports mean / stddev / CV% per codec for encode_MBps, decode_MBps, ratio.
#
# Usage: python tools\noise_floor.py <file...> --bench build\anvil_bench.exe
#        --runs 3 --reps 3 --out tests\noise-floor.csv
from __future__ import annotations
import argparse, csv, io, statistics, subprocess, sys
from pathlib import Path


def run_one(exe: Path, path: Path, reps: int) -> list[dict]:
    p = subprocess.run([str(exe), str(path), str(reps)], check=True,
                       text=True, capture_output=True)
    lines = p.stdout.strip().splitlines()
    rows = list(csv.DictReader(io.StringIO("\n".join(lines[1:]))))
    for r in rows:
        r["compressed_bytes"] = int(r["compressed_bytes"])
        r["ratio"] = float(r["ratio"])
        r["encode_MBps"] = float(r["encode_MBps"])
        r["decode_MBps"] = float(r["decode_MBps"])
    return rows


def stats(vals: list[float]) -> tuple[float, float, float]:
    m = statistics.mean(vals)
    sd = statistics.stdev(vals) if len(vals) > 1 else 0.0
    cv = 100.0 * sd / m if m else 0.0
    return m, sd, cv


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--bench", default=str(Path(__file__).parents[1] / "build" / "anvil_bench.exe"))
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--reps", type=int, default=3)
    ap.add_argument("--out", default="tests/noise-floor.csv")
    a = ap.parse_args()
    out = [["file", "codec", "metric", "mean", "stddev", "cv_pct"]]
    for f in a.files:
        path = Path(f)
        samples: dict[str, list[dict]] = {}
        for run in range(a.runs):
            for r in run_one(Path(a.bench), path, a.reps):
                samples.setdefault(r["codec"], []).append(r)
        for codec, rs in sorted(samples.items()):
            for metric in ("ratio", "encode_MBps", "decode_MBps"):
                vals = [r[metric] for r in rs]
                m, sd, cv = stats(vals)
                out.append([str(path), codec, metric, f"{m:.6f}", f"{sd:.6f}", f"{cv:.3f}"])
    with open(a.out, "w", newline="") as fp:
        w = csv.writer(fp)
        w.writerows(out)
    print(f"wrote {a.out} ({a.runs} runs x {a.reps} reps per file)")
    worst = sorted((r for r in out[1:] if r[2] != "ratio"), key=lambda r: -float(r[5]))[:5]
    print("worst CV% (timing):")
    for r in worst:
        print(f"  {r[0]} {r[1]} {r[2]} cv={r[5]}%")


if __name__ == "__main__":
    main()
