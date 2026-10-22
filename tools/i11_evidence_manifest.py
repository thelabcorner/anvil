#!/usr/bin/env python3
"""Build the I11 evidence manifest from the downloaded GitHub Actions artefacts.

This tool measures nothing. It reads the artefacts that the Actions runs
uploaded and re-emits their raw fields, and it **recomputes the gate checks
independently of the arbiter's own `gates` map**, so that a claim can be
re-derived from the raw files instead of being restated from a summary.

Usage:
    python tools/i11_evidence_manifest.py tests/i11-evidence \
        --out tests/i11-evidence/MANIFEST.json \
        --md  tests/i11-evidence/MANIFEST.md

Artefacts are expected under
    tests/i11-evidence/runs/<run_id>/<artifact_dir>/...
and the tool tolerates missing files: anything it cannot find is reported as
`null` and its check as `not_evaluated`, never as a pass.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

# The frozen arbiter's gate conditions, restated from
# tools/reference_cost_gate.py so that the manifest can be checked against the
# tool's own source.  No threshold here is derived from a measurement.
GATE_RULES = {
    "bytes_below_xz": "compressed_bytes['anvil-aux'] < compressed_bytes['xz']",
    "bytes_below_brotli": "compressed_bytes['anvil-aux'] < compressed_bytes['brotli']",
    "decode_xz_2x": "decode.xz.ci_hi <= 2.0",
    "decode_brotli_2x": "decode.brotli.ci_hi <= 2.0",
    "memory_xz_2x": "memory_ratio['xz'] <= 2.0",
    "memory_brotli_2x": "memory_ratio['brotli'] <= 2.0",
    "a_a_null_ci_spans_one": "null paired_ratio_ci95 contains 1.0",
    "timing_valid": "gates.timing_valid is true",
}

# Dominance axes added by I11 (the I20 arbiter's scope note excludes them).
DOMINANCE_RULES = {
    "encode_at_least_as_fast_as_every_reference": "encode.candidate_median_s <= min(encode.reference_median_s)",
    "decoder_binary_at_most_every_reference": "decoder_bytes.candidate <= min(decoder_bytes.references)",
}

# Files that may carry the per-reference front, depending on the workflow.
FRONT_CANDIDATES = (
    "dense-front.json",
    "dense-front.csv",
    "front.json",
    "dominance.json",
    "grid-summary.json",
    "pareto-verdict.csv",
    "pareto-baseline.csv",
    "tuple.json",
)


def jload(p: Path) -> Any:
    try:
        return json.loads(p.read_text(encoding="utf-8"))
    except Exception:
        return None


def cload(p: Path) -> Any:
    try:
        import csv
        with p.open(newline="", encoding="utf-8") as f:
            return list(csv.DictReader(f))
    except Exception:
        return None


def sha256(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def read_text(p: Path) -> str:
    try:
        return p.read_text(encoding="utf-8", errors="replace")
    except Exception:
        return ""


def prov(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for line in text.splitlines():
        if "=" in line and not line.startswith("="):
            k, v = line.split("=", 1)
            out[k.strip()] = v.strip()
    return out


def find(d: Path, *names: str) -> Path | None:
    for n in names:
        p = d / n
        if p.is_file():
            return p
    return None


def checks_for(ruling: dict[str, Any], paired: dict[str, Any]) -> dict[str, Any]:
    cb = ruling.get("compressed_bytes") or {}
    dec = ruling.get("decode") or {}
    mem = ruling.get("memory_ratio") or {}
    gates = ruling.get("gates") or {}
    null_ci = (paired.get("null") or {}).get("paired_ratio_ci95")
    null_spans = None
    if null_ci and len(null_ci) == 2:
        null_spans = bool(null_ci[0] <= 1.0 <= null_ci[1])

    def le(section: dict[str, Any], key: str) -> bool | None:
        v = (section.get(key) if isinstance(section, dict) else None)
        return None if v is None else bool(v <= 2.0)

    def ge_bytes(a: str, b: str) -> bool | None:
        x, y = cb.get(a), cb.get(b)
        return None if x is None or y is None else bool(x < y)

    return {
        "bytes_below_xz": ge_bytes("anvil-aux", "xz"),
        "bytes_below_brotli": ge_bytes("anvil-aux", "brotli"),
        "decode_xz_2x": le(dec.get("xz") or {}, "ci_hi"),
        "decode_brotli_2x": le(dec.get("brotli") or {}, "ci_hi"),
        "memory_xz_2x": None if mem.get("xz") is None else bool(mem["xz"] <= 2.0),
        "memory_brotli_2x": None if mem.get("brotli") is None else bool(mem["brotli"] <= 2.0),
        "a_a_null_ci_spans_one": null_spans,
        "timing_valid": gates.get("timing_valid"),
    }


def collect(run_dir: Path) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for d in sorted(p for p in run_dir.iterdir() if p.is_dir()):
        ruling = jload(d / "ruling.json") or {}
        prep = jload(d / "prepare-summary.json") or {}
        rss = jload(d / "rss.json") or {}
        paired: dict[str, Any] = {}
        for tag in ("xz", "brotli", "null", "encode", "changed"):
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
        front: dict[str, Any] = {}
        for name in FRONT_CANDIDATES:
            p = d / name
            if not p.is_file():
                continue
            payload = jload(p) if p.suffix == ".json" else cload(p)
            if payload:
                front[name] = payload
        dec_bin: dict[str, Any] = {}
        p = find(d, "decoder-bytes.json", "binary-sizes.json")
        if p:
            dec_bin = jload(p) or {}
        enc: dict[str, Any] = {}
        p = find(d, "encode.json", "encode-summary.json")
        if p:
            enc = jload(p) or {}
        checks = checks_for(ruling, paired) if ruling else {}
        rows.append({
            "run_id": run_dir.name,
            "artifact_dir": d.name,
            "artifact_files": sorted(p.name for p in d.iterdir() if p.is_file()),
            "ruling": ruling.get("ruling"),
            "ruling_gates": ruling.get("gates"),
            "checks_recomputed_from_raw_artefacts": checks,
            "compressed_bytes": ruling.get("compressed_bytes") or prep.get("compressed_bytes"),
            "rss_kib": rss.get("max_rss_kib"),
            "memory_ratio": ruling.get("memory_ratio"),
            "decode": ruling.get("decode"),
            "paired_series": paired,
            "encode_axis": enc,
            "decoder_bytes_axis": dec_bin,
            "front_artefacts": sorted(front.keys()),
            "prepare_summary": {k: prep.get(k) for k in ("corpus", "source_bytes", "file_count", "compressed_bytes", "roundtrip")},
            "provenance": prov(read_text(d / "provenance.txt")) if (d / "provenance.txt").is_file() else {},
            "candidate_ref": read_text(d / "candidate-ref.txt").strip() if (d / "candidate-ref.txt").is_file() else None,
            "candidate_extra_args": read_text(d / "candidate-extra-args.txt").strip() if (d / "candidate-extra-args.txt").is_file() else None,
            "ruling_json_sha256": sha256(d / "ruling.json") if (d / "ruling.json").is_file() else None,
        })
    return rows


def fmt_ci(series: dict[str, Any] | None, key: str) -> str:
    if not series:
        return "-"
    v = series.get(key)
    if isinstance(v, list) and len(v) == 2:
        return f"[{v[0]:.4f}, {v[1]:.4f}]"
    return "-" if v is None else str(v)


def md_table(rows: list[dict[str, Any]]) -> str:
    out = [
        "# I11 evidence manifest (machine-generated)",
        "",
        "Generated by `tools/i11_evidence_manifest.py` from the GitHub Actions artefacts",
        "downloaded into `tests/i11-evidence/runs/<run_id>/`. **Nothing here is measured by this",
        "tool**; every number is re-emitted from the artefact the run uploaded, and the gate",
        "checks are **recomputed here from the raw artefacts**, independently of the arbiter's own",
        "`gates` map.",
        "",
        "Gate conditions, restated from the frozen `tools/reference_cost_gate.py`:",
        "",
    ]
    for k, v in GATE_RULES.items():
        out.append(f"- `{k}` - {v}")
    out += ["", "Dominance axes added by I11 (not ruled by the I20 arbiter):", ""]
    for k, v in DOMINANCE_RULES.items():
        out.append(f"- `{k}` - {v}")
    out += [
        "",
        "A check that could not be evaluated from the artefacts is reported as `null` and is",
        "**never** counted as a pass.",
        "",
        "## Runs",
        "",
        "| run | artifact dir | corpus | candidate_ref | extra args | ruling | bytes anvil/xz/brotli | G3 CI-upper vs xz | G4 CI-upper vs Brotli | RSS vs xz / Brotli | A/A null CI | checks failed |",
        "|---|---|---|---|---|---|---|---|---|---|---|---|---|",
    ]
    for r in rows:
        cb = r["compressed_bytes"] or {}
        dec = r["decode"] or {}
        mem = r["memory_ratio"] or {}
        null = (r["paired_series"] or {}).get("null") or {}
        checks = r["checks_recomputed_from_raw_artefacts"] or {}
        failed = [k for k, v in checks.items() if v is not True]
        xs = mem.get("xz")
        bs = mem.get("brotli")
        out.append(
            f"| `{r['run_id']}` | `{r['artifact_dir']}` | {(r['prepare_summary'] or {}).get('corpus')} | "
            f"`{r['candidate_ref']}` | `{r['candidate_extra_args']}` | **{r['ruling']}** | "
            f"{cb.get('anvil-aux')} / {cb.get('xz')} / {cb.get('brotli')} | "
            f"{fmt_ci(dec.get('xz'), 'ci_hi')} | {fmt_ci(dec.get('brotli'), 'ci_hi')} | "
            f"{xs if xs is None else f'{xs:.4f}'} / {bs if bs is None else f'{bs:.4f}'} | "
            f"{fmt_ci(null, 'paired_ratio_ci95')} | "
            f"{'**none - all evaluated checks pass**' if not failed else ', '.join(failed)} |"
        )
    return "\n".join(out) + "\n"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("evidence_dir", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--md", type=Path, required=True)
    a = ap.parse_args()
    rows: list[dict[str, Any]] = []
    runs = a.evidence_dir / "runs"
    if runs.is_dir():
        for run_dir in sorted(p for p in runs.iterdir() if p.is_dir()):
            rows.extend(collect(run_dir))
    payload = {
        "schema": 1,
        "gate_rules": GATE_RULES,
        "dominance_rules": DOMINANCE_RULES,
        "note": "checks_recomputed_from_raw_artefacts is computed here; null means not evaluable.",
        "runs": rows,
    }
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    a.md.write_text(md_table(rows), encoding="utf-8")
    print(f"wrote {a.out} and {a.md} ({len(rows)} run records)")


if __name__ == "__main__":
    main()
