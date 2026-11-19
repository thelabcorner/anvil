#!/usr/bin/env python3
"""Independent, read-only integrity audit of ANVIL's A48 Silesia thin-reference grid.

Never substitutes for the project's frozen gate. No benchmarking or building.
It intentionally labels every positive as SCOPED/PROVISIONAL, not Pareto dominance.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import random
import statistics
import sys
from collections import Counter, defaultdict
from pathlib import Path

REFS = ("brotli-", "zstd-", "xz-")
PLANES = ("encode_MBps", "decode_MBps")


def load_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def classify(p: dict, refs: list[dict], key: str) -> str:
    """Independent recomputation of frozen I9 R-2 predicates on supplied rows."""
    pr, pm = float(p["ratio"]), float(p[key])
    if any(float(q["ratio"]) <= pr and float(q[key]) >= pm
           and (float(q["ratio"]) < pr or float(q[key]) > pm) for q in refs):
        return "DOMINATED"
    if pr >= 0.95:
        return "DEGENERATE"
    if any(float(lo["ratio"]) <= pr <= float(hi["ratio"])
           and float(lo[key]) <= pm <= float(hi[key])
           and float(lo["ratio"]) < float(hi["ratio"])
           and float(lo[key]) < float(hi[key])
           for lo in refs for hi in refs):
        return "FRONT-GAP"
    return "FRONT-CROSSING"


def aggregate(rows: list[dict]) -> list[dict]:
    groups = defaultdict(list)
    for r in rows:
        groups[r["codec"]].append(r)
    out = []
    for name, rs in groups.items():
        ib = sum(int(r["input_bytes"]) for r in rs)
        cb = sum(int(r["compressed_bytes"]) for r in rs)
        # Reproduce the FROZEN gate-verify-i9.py aggregate_rows() quantization
        # EXACTLY. Full-precision aggregates can change dominance on near-ties,
        # even when the source bytes and timings are identical.
        row = {"codec": name, "ratio": f"{cb / ib:.6f}"}
        for key in PLANES:
            elapsed_s = sum(int(r["input_bytes"]) / 1e6 /
                            max(float(r[key]), 1e-12) for r in rs)
            row[key] = f"{ib / 1e6 / elapsed_s:.3f}"
        out.append(row)
    return out


def recompute_paired_ci(pairs: list[dict], seed: int, samples: int) -> tuple[float, float, float]:
    """Independently check the frozen paired bootstrap against ALL raw pairs.

    This reproduces the frozen resampling contract but does not trust its
    published point estimate or confidence interval. It performs scalar-only
    arithmetic; no codec run, build or benchmark.
    """
    logs = [math.log(float(p["candidate_s"]) / float(p["control_s"]))
            for p in pairs]
    n = len(logs)
    ratio = math.exp(statistics.median(logs))
    rng = random.Random(seed ^ 0xA5A5A5A5)
    boot = []
    for _ in range(samples):
        boot.append(math.exp(statistics.median(
            [logs[rng.randrange(n)] for _ in range(n)])))
    boot.sort()
    return (ratio, boot[max(0, int(0.025 * samples))],
            boot[min(samples - 1, int(0.975 * samples))])


def audit(grid: Path, timing: Path, expected: Path, paired_dir: Path | None) -> dict:
    errors = []
    rows = load_csv(grid)
    files = [s.strip() for s in expected.read_text(encoding="utf-8").splitlines() if s.strip()]
    if not files or len(files) != len(set(files)):
        errors.append("files.txt missing entries or contains duplicate file names")
    if len(files) != 12:
        errors.append(f"A48 expected 12 Silesia files, files.txt contains {len(files)}")
    expected_codecs = {"anvil-cap8", "xz-9e", "brotli-q11-lw30"}
    byfile = defaultdict(list)
    seen = set()
    for r in rows:
        try:
            f, c = r["file"], r["codec"]
            k = (f, c)
            if k in seen:
                errors.append(f"duplicate grid key: {k}")
            seen.add(k)
            ib, cb = int(r["input_bytes"]), int(r["compressed_bytes"])
            ratio = float(r["ratio"])
            if ib <= 0 or cb <= 0 or not math.isfinite(ratio) or abs(ratio - cb / ib) > 1e-9:
                errors.append(f"inconsistent wire byte accounting: {k}")
            if any(not math.isfinite(float(r[key])) or float(r[key]) <= 0 for key in PLANES):
                errors.append(f"invalid throughput: {k}")
            if r["roundtrip"] != "OK":
                errors.append(f"roundtrip not OK: {k}")
            byfile[f].append(r)
        except (KeyError, TypeError, ValueError, ZeroDivisionError) as exc:
            errors.append(f"malformed row: {exc}")
    if set(byfile) != set(files):
        errors.append(f"file-set mismatch: missing={sorted(set(files)-set(byfile))} extra={sorted(set(byfile)-set(files))}")
    for f in files:
        rs = byfile[f]
        if {r.get("codec") for r in rs} != expected_codecs or len(rs) != len(expected_codecs):
            errors.append(f"incomplete/wrong reference grid: {f} expected={sorted(expected_codecs)}")
        if len({int(r["input_bytes"]) for r in rs if r.get("input_bytes", "").isdigit()}) > 1:
            errors.append(f"reference input-size mismatch: {f}")
    try:
        valid_rows = json.loads(timing.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        errors.append(f"cannot read timing-validity.json: {exc}")
        valid_rows = []
    validity = {r.get("file"): r for r in valid_rows if isinstance(r, dict)}
    if len(valid_rows) != len(validity) or set(validity) != set(files):
        errors.append("timing-validity record set is incomplete or duplicated")
    noise = []
    paired = {}
    for f in files:
        v = validity.get(f, {})
        for k in ("encode_null_ci", "decode_null_ci"):
            ci = v.get(k)
            if not (isinstance(ci, list) and len(ci) == 2 and
                    all(isinstance(x, (float, int)) and math.isfinite(x) for x in ci) and
                    ci[0] <= 1.0 <= ci[1]):
                noise.append(f"{f}: null CI does not enclose 1.0 ({k})")
        if v.get("timing_valid_all") is not True:
            noise.append(f"{f}: timing_valid_all != true")
        if paired_dir is not None:
            for phase in ("dec", "enc"):
                for opponent in ("xz", "brotli", "null"):
                    path = paired_dir / f"paired-{f}-{phase}-{opponent}.json"
                    if not path.is_file():
                        errors.append(f"missing paired evidence {path.name}")
                        continue
                    try:
                        summary = json.loads(path.read_text(encoding="utf-8"))
                        if summary.get("timing_valid") is not True:
                            noise.append(f"{f}: paired {phase}-{opponent} timing invalid")
                        if opponent == "null":
                            ci = summary.get("paired_ratio_ci95", [])
                            if len(ci) != 2 or not (ci[0] <= 1.0 <= ci[1]):
                                noise.append(f"{f}: paired {phase} null CI misses 1.0")
                        paired[(f, phase, opponent)] = summary
                        pairs = summary.get("pairs", [])
                        expected_reps = 9 if phase == "dec" else 7
                        if not isinstance(pairs, list) or len(pairs) != expected_reps:
                            errors.append(f"{f}: wrong paired count {phase}-{opponent} ({len(pairs) if isinstance(pairs, list) else 'malformed'})")
                        else:
                            try:
                                for arm in ("control", "candidate"):
                                    sample = [float(p[f"{arm}_s"]) for p in pairs]
                                    if not all(math.isfinite(v) and v > 0 for v in sample):
                                        errors.append(f"{f}: invalid raw {arm} times {phase}-{opponent}")
                                        continue
                                    actual_median = statistics.median(sample)
                                    claimed_median = float(summary[arm]["median_s"])
                                    if not math.isclose(actual_median, claimed_median, rel_tol=1e-8, abs_tol=1e-8):
                                        errors.append(f"{f}: paired median mismatch {phase}-{opponent} {arm}")
                            except (KeyError, ValueError, TypeError) as exc:
                                errors.append(f"{f}: invalid raw repetition {phase}-{opponent}: {exc}")
                        # Recompute the paired bootstrap independently from
                        # original per-repetition times. Medians alone cannot
                        # validate a claimed 95% CI or A/A null.
                        if isinstance(pairs, list) and len(pairs) == expected_reps:
                            try:
                                if sorted(int(p["pair"]) for p in pairs) != list(range(expected_reps)):
                                    errors.append(f"{f}: missing/duplicate raw pair index {phase}-{opponent}")
                                seed = int(summary["seed"])
                                count = int(summary["bootstrap_samples"])
                                if count < 1000 or count > 100000:
                                    raise ValueError(f"unsafe bootstrap count {count}")
                                point, low, high = recompute_paired_ci(pairs, seed, count)
                                claimed = [float(summary["paired_ratio_candidate_over_control"]),
                                           *(float(v) for v in summary["paired_ratio_ci95"])]
                                if len(claimed) != 3 or not all(math.isfinite(v) for v in claimed):
                                    raise ValueError("invalid published ratio/CI")
                                if any(not math.isclose(v, c, rel_tol=1e-9, abs_tol=1e-9)
                                       for v, c in zip((point, low, high), claimed)):
                                    errors.append(f"{f}: paired ratio/CI mismatch {phase}-{opponent}")
                            except (KeyError, TypeError, ValueError, ZeroDivisionError, OverflowError) as exc:
                                errors.append(f"{f}: invalid paired bootstrap evidence {phase}-{opponent}: {exc}")
                        ctrl = summary.get("control", {}).get("argv", [])
                        cand = summary.get("candidate", {}).get("argv", [])
                        if not isinstance(ctrl, list) or not isinstance(cand, list) or not ctrl or not cand:
                            errors.append(f"{f}: missing command argv {phase}-{opponent}")
                        else:
                            if "anvil" not in Path(cand[0]).name.lower():
                                errors.append(f"{f}: candidate is not ANVIL {phase}-{opponent}")
                            expected_ctrl = {"xz": "xz", "brotli": "brotli", "null": "anvil"}[opponent]
                            if expected_ctrl not in Path(ctrl[0]).name.lower():
                                errors.append(f"{f}: reference argv mismatch {phase}-{opponent}")
                            if opponent == "null" and ctrl != cand:
                                errors.append(f"{f}: A/A control and candidate argv differ {phase}")
                    except (OSError, ValueError, TypeError, KeyError) as exc:
                        errors.append(f"invalid paired summary {path.name}: {exc}")
    if paired_dir is not None:
        for f in files:
            rs = {r.get("codec"): r for r in byfile[f]}
            if len(rs) != 3 or any((f, phase, opponent) not in paired
                                   for phase in ("dec", "enc")
                                   for opponent in ("xz", "brotli", "null")):
                continue
            try:
                for phase, key in (("dec", "decode_MBps"), ("enc", "encode_MBps")):
                    n = int(rs["anvil-cap8"]["input_bytes"])
                    sx = paired[f, phase, "xz"]
                    sb = paired[f, phase, "brotli"]
                    expected_s = {
                        "anvil-cap8": (sx["candidate"]["median_s"] + sb["candidate"]["median_s"]) / 2,
                        "xz-9e": sx["control"]["median_s"],
                        "brotli-q11-lw30": sb["control"]["median_s"],
                    }
                    for codec, elapsed in expected_s.items():
                        measured = float(rs[codec][key])
                        recomputed = n / (1e6 * float(elapsed))
                        if not math.isclose(measured, recomputed, abs_tol=0.00051):
                            errors.append(f"{f}: grid-vs-paired throughput mismatch ({codec}, {key}): {measured} vs {recomputed}")
            except (KeyError, TypeError, ValueError, ZeroDivisionError) as exc:
                errors.append(f"{f}: malformed paired throughput source: {exc}")
    result = {"schema": 1, "scope": "A48 Silesia 12-file, THIN reference grid; NOT the frozen Class-A frontier",
              "files": len(files), "grid_rows": len(rows), "errors": errors, "timing_uncertainties": noise,
              "per_file": [], "per_file_counts": {}, "aggregate_counts": {}, "decision": None,
              "limitations": [
                  "Only supplied reference rows are classified; the ratio-only robustness scan does NOT add timing rows",
                  "Crossing in either 2-D plane is NOT multi-axis Pareto dominance (RSS, decoder size, random-access untested)",
                  "Frozen Class-A 468-cell tallies must not be conflated with this 12-file Silesia experiment",
                  "Full-wire bytes and exact source/artifact hashes remain separate provenance gates",
              ]}
    if not errors:
        counts = Counter()
        for f in files:
            rs = byfile[f]
            p = next(r for r in rs if r["codec"].startswith("anvil-"))
            refs = [r for r in rs if r["codec"].startswith(REFS)]
            classes = {plane: classify(p, refs, plane) for plane in PLANES}
            counts.update(classes.values())
            result["per_file"].append({"file": f, **classes})
        result["per_file_counts"] = dict(sorted(counts.items()))
        aggr = aggregate(rows)
        p = next(r for r in aggr if r["codec"].startswith("anvil-"))
        refs = [r for r in aggr if r["codec"].startswith(REFS)]
        result["aggregate_counts"] = dict(Counter(classify(p, refs, plane) for plane in PLANES))
    if errors:
        result["decision"] = "INVALID_INCOMPLETE_OR_FAILED_CORRECTNESS"
    elif noise:
        result["decision"] = "INCONCLUSIVE_TIMING_NOT_H0"
    elif result["per_file_counts"].get("FRONT-CROSSING", 0):
        result["decision"] = "PROVISIONAL_SCOPED_THIN_FRONT_CROSSING_ONLY"
    else:
        result["decision"] = "NO_CROSSING_OBSERVED_ON_VALID_THIN_GRID"
    return result


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--grid", type=Path, required=True)
    ap.add_argument("--timing", type=Path, required=True)
    ap.add_argument("--files", type=Path, required=True)
    ap.add_argument("--paired-dir", type=Path, help="Optional folder of six paired JSON summaries per file")
    ap.add_argument("--out", type=Path, help="Write JSON report here as well as stdout")
    args = ap.parse_args()
    report = audit(args.grid, args.timing, args.files, args.paired_dir)
    data = json.dumps(report, indent=2, sort_keys=True) + "\n"
    print(data, end="")
    if args.out:
        args.out.write_text(data, encoding="utf-8")
    return 2 if report["errors"] else 0


if __name__ == "__main__":
    sys.exit(main())
