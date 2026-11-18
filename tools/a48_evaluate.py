#!/usr/bin/env python3
"""A48 independent ruling evaluator (A49 gates, mechanical, no frozen-tool edit).

Consumes raw A48 artifacts and applies the A49 validity gates BEFORE any
crossing claim:
  G0 every per-file row roundtrip=OK and consistent input/byte counts
  G1 timing-validity.json: all_timing_valid true for every file
  G2 both A/A null CIs (decode + encode) span 1.0 for every file
Only if G0&G1&G2 hold do we recompute the frozen classifier counts, in TWO
SEPARATE scopes (per-file, AGGREGATE) with NO double counting. A positive is
labelled SCOPED/THIN-GRID (2-reference Silesia grid), never all-axis dominance.

Usage:
  python tools/a48_evaluate.py --art-dir DIR --grid DIR/benchmark-suite.csv \
      --gate docs/gate-verify-i9.py --out DIR/a48-ruling.json
"""
from __future__ import annotations
import argparse, csv, json, subprocess, sys
from collections import defaultdict
from pathlib import Path

FIELDS = ("file", "input_bytes", "codec", "compressed_bytes", "ratio",
          "encode_MBps", "decode_MBps", "roundtrip")


def load(path: Path) -> list[dict]:
    with path.open(newline="") as fp:
        return list(csv.DictReader(fp))


def span1(ci) -> bool:
    try:
        lo, hi = float(ci[0]), float(ci[1])
    except (TypeError, ValueError, IndexError):
        return False
    return lo <= 1.0 <= hi


def gate_validity(art: Path) -> dict:
    """Return {ok, reasons} for G1 (timing_valid) and G2 (null CIs span 1.0)."""
    reasons: list[str] = []
    tv_path = art / "timing-validity.json"
    if not tv_path.exists():
        return {"ok": False, "reasons": ["missing timing-validity.json"]}
    tv = json.loads(tv_path.read_text())
    if not isinstance(tv, list) or not tv:
        return {"ok": False, "reasons": ["empty timing-validity.json"]}
    for row in tv:
        f = row.get("file", "?")
        if not row.get("timing_valid_all", False):
            reasons.append(f"{f}: timing_valid_all false")
        if not row.get("decode_null_valid", False):
            reasons.append(f"{f}: decode null CI not spanning 1.0 / invalid")
        if not row.get("encode_null_valid", False):
            reasons.append(f"{f}: encode null CI not spanning 1.0 / invalid")
        dci = row.get("decode_null_ci")
        eci = row.get("encode_null_ci")
        if not span1(dci):
            reasons.append(f"{f}: decode null CI {dci} excludes 1.0")
        if not span1(eci):
            reasons.append(f"{f}: encode null CI {eci} excludes 1.0")
    return {"ok": not reasons, "reasons": reasons}


def gate_roundtrip_bytes(grid: list[dict]) -> dict:
    reasons: list[str] = []
    by_file: dict[str, dict[str, dict]] = defaultdict(dict)
    for r in grid:
        for k in FIELDS:
            if k not in r:
                reasons.append(f"missing column {k}")
                break
        if str(r.get("roundtrip", "")).upper() != "OK":
            reasons.append(f"{r.get('file')}/{r.get('codec')}: roundtrip != OK")
        by_file[r["file"]][r["codec"]] = r
    strict_min = []
    for f, arms in sorted(by_file.items()):
        ib = {c: int(x["input_bytes"]) for c, x in arms.items()}
        if len(set(ib.values())) != 1:
            reasons.append(f"{f}: inconsistent input_bytes {ib}")
        anvil = [x for c, x in arms.items() if c.startswith("anvil-")]
        refs = [x for c, x in arms.items() if c.startswith(("xz-", "brotli-", "zstd-"))]
        if not anvil or not refs:
            reasons.append(f"{f}: missing anvil or reference arms")
            continue
        amin = min(int(x["compressed_bytes"]) for x in anvil)
        if all(amin < int(x["compressed_bytes"]) for x in refs):
            strict_min.append(f)
    return {"ok": not reasons, "reasons": reasons, "strict_byte_min_files": strict_min}


def frozen_counts(gate_py: Path, grid: Path) -> dict:
    """Run frozen classifier unmodified; parse per-file and AGGREGATE tuples separately."""
    p = subprocess.run([sys.executable, str(gate_py), "--suite", str(grid),
                        "--refs", "brotli-,zstd-,xz-"],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    text = p.stdout
    scopes = {}
    for scope in ("per-file", "AGGREGATE"):
        line = next((ln for ln in text.splitlines()
                     if ln.strip().startswith(scope + ":") or ln.strip().startswith(scope + " :")), "")
        counts = {}
        import re
        for token in ("DEGENERATE", "FRONT-GAP", "FRONT-CROSSING"):
            m = re.search(r"(\d+)\s+" + re.escape(token), line)
            counts[token] = int(m.group(1)) if m else 0
        m = re.search(r"(\d+)\s*/\s*\d+\s+dominated", line)
        counts["DOMINATED"] = int(m.group(1)) if m else 0
        scopes[scope] = counts
    return {"scopes": scopes, "raw": text}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--art-dir", required=True)
    ap.add_argument("--grid", required=True)
    ap.add_argument("--gate", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    art, grid = Path(a.art_dir), Path(a.grid)
    rows = load(grid)
    rt = gate_roundtrip_bytes(rows)
    vd = gate_validity(art)
    valid = rt["ok"] and vd["ok"]
    ruling = {
        "schema": 1,
        "gates": {"roundtrip_and_bytes": rt, "timing_and_null": vd},
        "valid": valid,
        "classification": "VALID" if valid else "INVALID_OR_INCONCLUSIVE",
        "note": "A positive here is SCOPED/THIN-GRID (2-ref Silesia), NOT all-axis dominance.",
    }
    if valid:
        ruling["frozen_counts"] = frozen_counts(Path(a.gate), grid)
        ruling["scope_A_strict_byte_min_files"] = rt["strict_byte_min_files"]
    Path(a.out).write_text(json.dumps(ruling, indent=2) + "\n")
    print(json.dumps({"classification": ruling["classification"],
                      "valid": valid,
                      "g0": rt["ok"], "g1g2": vd["ok"]}, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
