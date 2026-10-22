#!/usr/bin/env python3
"""Build the I20 gate-closure evidence manifest from the downloaded Actions artifacts.

This tool does NOT measure anything. It reads the artifacts that the
GitHub Actions runs uploaded and re-emits their raw fields, so that the
programme's evidence is independently inspectable inside the repository
rather than only asserted in prose.

Usage:
    python tools/i20_evidence_manifest.py tests/i20-evidence \
        --out tests/i20-evidence/MANIFEST.json --md tests/i20-evidence/MANIFEST.md
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
from typing import Any

# The frozen arbiter's gate thresholds, restated here so the manifest can be
# checked against the tool's own source (tools/reference_cost_gate.py).
GATE_RULES = {
    "bytes_below_xz": "compressed_bytes['anvil-aux'] < compressed_bytes['xz']",
    "decode_xz_2x": "decode.xz.ci_hi <= 2.0",
    "decode_brotli_2x": "decode.brotli.ci_hi <= 2.0",
    "memory_xz_2x": "memory_ratio['xz'] <= 2.0",
    "memory_brotli_2x": "memory_ratio['brotli'] <= 2.0",
}


def jload(p: Path) -> Any:
    try:
        return json.loads(p.read_text(encoding="utf-8"))
    except Exception:
        return None


def sha256(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def prov(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for line in text.splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            out[k.strip()] = v.strip()
    return out


def collect(run_dir: Path) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for d in sorted(p for p in run_dir.iterdir() if p.is_dir()):
        ruling = jload(d / "ruling.json")
        if not ruling:
            continue
        prep = jload(d / "prepare-summary.json") or {}
        pv = prov((d / "provenance.txt").read_text(encoding="utf-8", errors="replace")) if (d / "provenance.txt").is_file() else {}
        paired: dict[str, Any] = {}
        for tag in ("xz", "brotli", "null"):
            j = jload(d / f"paired-{tag}.json")
            if j:
                paired[tag] = {
                    "paired_ratio_candidate_over_control": j.get("paired_ratio_candidate_over_control"),
                    "paired_ratio_ci95": j.get("paired_ratio_ci95"),
                    "control_median_s": (j.get("control") or {}).get("median_s"),
                    "candidate_median_s": (j.get("candidate") or {}).get("median_s"),
                    "timing_valid": j.get("timing_valid"),
                    "clears_speed_gate": j.get("clears_speed_gate"),
                }
        rss = jload(d / "rss.json") or {}
        cb = ruling.get("compressed_bytes", {})
        dec = ruling.get("decode", {})
        mem = ruling.get("memory_ratio", {})
        gates = ruling.get("gates", {})
        null_ci = (paired.get("null") or {}).get("paired_ratio_ci95")
        null_spans = bool(null_ci and null_ci[0] <= 1.0 <= null_ci[1]) if null_ci else None
        checks = {
            "bytes_below_xz": bool(cb.get("anvil-aux") is not None and cb.get("xz") is not None
                                   and cb["anvil-aux"] < cb["xz"]),
            "decode_xz_2x": bool((dec.get("xz") or {}).get("ci_hi") is not None and dec["xz"]["ci_hi"] <= 2.0),
            "decode_brotli_2x": bool((dec.get("brotli") or {}).get("ci_hi") is not None and dec["brotli"]["ci_hi"] <= 2.0),
            "memory_xz_2x": bool(mem.get("xz") is not None and mem["xz"] <= 2.0),
            "memory_brotli_2x": bool(mem.get("brotli") is not None and mem["brotli"] <= 2.0),
            "a_a_null_ci_spans_one": null_spans,
        }
        rows.append({
            "run_id": run_dir.name,
            "artifact_dir": d.name,
            "artifact_files": sorted(p.name for p in d.iterdir() if p.is_file()),
            "ruling": ruling.get("ruling"),
            "ruling_gates": gates,
            "ruling_gates_recomputed_here": checks,
            "compressed_bytes": cb,
            "rss_kib": rss.get("max_rss_kib"),
            "memory_ratio": mem,
            "decode": {
                "xz": {k: (dec.get("xz") or {}).get(k) for k in ("ratio", "ci_lo", "ci_hi", "control_s", "candidate_s", "ref_mbps", "anvil_mbps")},
                "brotli": {k: (dec.get("brotli") or {}).get(k) for k in ("ratio", "ci_lo", "ci_hi", "control_s", "candidate_s", "ref_mbps", "anvil_mbps")},
            },
            "paired_series": paired,
            "prepare_summary": {k: prep.get(k) for k in ("corpus", "source_bytes", "file_count", "compressed_bytes", "roundtrip")},
            "provenance": pv,
            "candidate_ref": (d / "candidate-ref.txt").read_text(encoding="utf-8").strip() if (d / "candidate-ref.txt").is_file() else None,
            "candidate_extra_args": (d / "candidate-extra-args.txt").read_text(encoding="utf-8").strip() if (d / "candidate-extra-args.txt").is_file() else None,
            "ruling_json_sha256": sha256(d / "ruling.json"),
        })
    return rows


def md_table(rows: list[dict[str, Any]]) -> str:
    out = ["# I20 gate-closure evidence manifest (machine-generated)", "",
           "Generated by `tools/i20_evidence_manifest.py` from the GitHub Actions artifacts",
           "downloaded into `tests/i20-evidence/runs/<run_id>/`. **Nothing here is measured by this",
           "tool**; every number is re-emitted from the artifact that the run uploaded.", "",
           "Gate rules, restated from the frozen `tools/reference_cost_gate.py`:", ""]
    for k, v in GATE_RULES.items():
        out.append(f"- `{k}` — {v}")
    out += ["", "## Runs", "",
            "| run | artifact dir | corpus | candidate_ref | extra args | ruling | bytes anvil/xz/brotli | G3 CI-upper vs xz | G4 CI-upper vs Brotli | RSS vs xz / Brotli | A/A null CI | checks failed |",
            "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|"]
    for r in rows:
        cb = r["compressed_bytes"]
        dec = r["decode"]
        mem = r["memory_ratio"]
        null = (r["paired_series"].get("null") or {})
        nci = null.get("paired_ratio_ci95")
        checks = r["ruling_gates_recomputed_here"]
        failed = [k for k, v in checks.items() if v is not True]
        allok = not failed
        out.append(
            f"| `{r['run_id']}` | `{r['artifact_dir']}` | {(r['prepare_summary'] or {}).get('corpus')} | "
            f"`{r['candidate_ref']}` | `{r['candidate_extra_args']}` | **{r['ruling']}** | "
            f"{cb.get('anvil-aux')} / {cb.get('xz')} / {cb.get('brotli')} | "
            f"{dec['xz'].get('ci_hi')} | {dec['brotli'].get('ci_hi')} | "
            f"{mem.get('xz', 0):.4f} / {mem.get('brotli', 0):.4f} | "
            f"{nci} | {'**none - all six pass**' if allok else ', '.join(failed)} |")
    return "\n".join(out) + "\n"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("evidence_dir", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--md", type=Path, required=True)
    a = ap.parse_args()
    rows: list[dict[str, Any]] = []
    for run_dir in sorted(p for p in a.evidence_dir.joinpath("runs").iterdir() if p.is_dir()):
        rows.extend(collect(run_dir))
    payload = {
        "schema": 1,
        "gate_rules": GATE_RULES,
        "runs": rows,
    }
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    a.md.write_text(md_table(rows), encoding="utf-8")
    print(f"wrote {a.out} and {a.md} ({len(rows)} run records)")


if __name__ == "__main__":
    main()
