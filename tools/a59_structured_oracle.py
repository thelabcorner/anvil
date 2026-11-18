#!/usr/bin/env python3
"""A59 structured-ratio anatomy oracle - field derivability census (NO codec).

Read-only, deterministic analysis of frozen structured corpus files. For each
JSON record field, test whether it is exactly derivable from the record index
`id` (and a small fixed rule), which would mean a general reconstruct-from-index
representation has headroom below brotli q11. Reports per-field derivability and
a cross-file OVERFIT guard: a rule is general only if it holds across ALL files.
This is an ORACLE, NOT a crossing claim.

Usage:
  python tools/a59_structured_oracle.py --files f1.json f2.jsonl ... --out oracle.json
"""
from __future__ import annotations
import argparse, json
from pathlib import Path


def load_records(path: Path) -> tuple[list[dict], str]:
    text = path.read_text(encoding="utf-8", errors="replace")
    if text.lstrip().startswith("["):
        return json.loads(text), "json_array"
    recs = [json.loads(ln) for ln in text.splitlines() if ln.strip()]
    return recs, "jsonl"


def analyze_file(path: Path) -> dict:
    recs, kind = load_records(path)
    if not recs or not isinstance(recs[0], dict):
        return {"file": str(path), "error": "no object records"}
    fields = sorted(set().union(*(set(r.keys()) for r in recs)))
    out = {"file": str(path), "kind": kind, "n_records": len(recs),
           "n_bytes": path.stat().st_size, "fields": {}}
    for fld in fields:
        vals = [r.get(fld) for r in recs]
        present = sum(1 for v in vals if v is not None)
        id_eq = sum(1 for i, v in enumerate(vals) if v == i)
        id_str = sum(1 for i, v in enumerate(vals)
                     if isinstance(v, str) and v.isdigit() and int(v) == i)
        const_step = None
        numeric = vals[0] is not None and isinstance(vals[0], (int, float)) and not isinstance(vals[0], bool)
        if numeric and all(isinstance(v, (int, float)) and not isinstance(v, bool) for v in vals):
            d = vals[1] - vals[0] if len(vals) > 1 else 0
            if all(abs((vals[i] - vals[0]) - i * d) < 1e-9 for i in range(len(vals))):
                const_step = "arith_progression"
        else:
            for step in range(1, 5):
                if all(isinstance(v, str) and v == f"t{i % step}" for i, v in enumerate(vals)):
                    const_step = f"cyc{step}"
                    break
        if id_eq / max(1, len(vals)) >= 0.999 or id_str / max(1, len(vals)) >= 0.999:
            rule = "identity"
        elif const_step:
            rule = const_step
        else:
            rule = "not_derivable"
        out["fields"][fld] = {"present": present, "rule": rule}
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--files", nargs="+", required=True)
    ap.add_argument("--q11-first-bytes", type=int, default=79172)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    per_file = [analyze_file(Path(f)) for f in a.files]
    common = None
    for r in per_file:
        if "fields" not in r:
            continue
        s = set(r["fields"])
        common = s if common is None else common & s
    general = {}
    for fld in sorted(common or []):
        rules = {r["fields"][fld]["rule"] for r in per_file if "fields" in r}
        stable = len(rules) == 1 and next(iter(rules)) != "not_derivable"
        general[fld] = {"stable_across_files": stable, "rules": sorted(rules)}
    result = {
        "schema": 1,
        "q11_first_file_bar_bytes": a.q11_first_bytes,
        "per_file": per_file,
        "cross_file_general_derivability": general,
        "note": "ORACLE ONLY. General rule must be stable across ALL files; "
                "single-file rules are OVERFIT and cannot support a crossing.",
    }
    Path(a.out).write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"files": len(a.files), "common_fields": sorted(common or []),
                      "general_stable": sum(1 for v in general.values() if v["stable_across_files"]),
                      "general": general}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
