#!/usr/bin/env python3
"""Regression harness for the I11 (D5) arbiter extension.

The I11 candidate D5 extends the frozen arbiter `tools/reference_cost_gate.py`
with two additional axes (whole-corpus encode wall time, decoder-only binary
size).  The extension must be a STRICT SUPERSET: no existing verdict may
change.  This harness proves that over the artefacts the I20 programme already
stored in the repository, WITHOUT any new compute:

  1. REPLAY.  For every stored I20 run the extended arbiter re-runs its
     `summarize` stage over the stored raw artefacts.  The produced ruling must
     reproduce the stored `ruling.json` exactly: same `ruling` string, same
     `gates` map (key for key, value for value), same `scope_note`, and the
     same bytes after newline normalisation.

  2. SUPERSET.  The same run is repeated with SYNTHETIC new-axis inputs.  The
     legacy `ruling` string, the legacy `gates` keys and the legacy
     `scope_note` must be untouched; only new keys may appear.

  3. INDEPENDENT RECOMPUTE.  The shared evidence machinery
     (`tools/i11_evidence_manifest.py`, the programme's own tool) recomputes
     the gate checks from the raw artefacts, independently of the arbiter's
     `gates` map.  The harness compares that independent recomputation with
     the arbiter's own `gates` map for every stored run.

The harness measures nothing: it replays already-measured artefacts.  The
authoritative execution of this harness is the GitHub Actions run whose
artefact is the evidence; a local execution is only a convenience check of the
same deterministic replay.

Usage:
    python tools/i11_replay_reference_gate.py tests/i20-evidence \
        --out-dir /tmp/regression-proof
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
import tempfile
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

import i11_evidence_manifest as manifest_tool  # noqa: E402  programme tool
import reference_cost_gate as arbiter  # noqa: E402  the frozen arbiter (+D5)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def normalized_sha256(path: Path) -> str:
    """sha256 of the file with line endings normalised to LF.

    The stored artefacts are committed with LF; a Windows replay writes CRLF.
    Normalising makes the comparison platform-independent while still proving
    byte-identity of the content.
    """
    data = path.read_bytes().replace(b"\r\n", b"\n")
    return sha256_bytes(data)


def legacy_gate_keys() -> list[str]:
    """The gate keys the frozen I20 arbiter emits (must never change)."""
    return [
        "timing_valid",
        "null_ci_spans_one",
        "decode_within_2x_xz",
        "decode_within_2x_brotli",
        "memory_within_2x_xz",
        "memory_within_2x_brotli",
    ]


def synthetic_axis_inputs() -> tuple[Path, Path]:
    """Synthetic new-axis inputs used only for the SUPERSET check.

    They are NOT measurements; they only prove that the extension adds keys
    and never rewrites an existing one.
    """
    tmp = Path(tempfile.mkdtemp(prefix="i11-superset-"))
    encode = tmp / "encode.json"
    decode = tmp / "decoder-bytes.json"
    encode.write_text(json.dumps({
        "schema": 1,
        "axis": {"status": "FAIL"},
        "counts": {"PASS": 0, "FAIL": 1, "INCONCLUSIVE": 0, "TIMING_BLOCKED": 0},
        "points": [],
    }, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    decode.write_text(json.dumps({
        "schema": 1,
        "axis": {"status": "FAIL"},
        "counts": {"PASS": 0, "FAIL": 1, "NOT_MEASURED": 0},
        "points": [],
    }, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return encode, decode


def replay_one(run_dir: Path, artifact: Path, work: Path) -> dict[str, Any]:
    name = f"{run_dir.name}/{artifact.name}"
    record: dict[str, Any] = {"run": name, "artifact_dir": artifact.name}
    stored_ruling_path = artifact / "ruling.json"
    if not stored_ruling_path.is_file():
        record["status"] = "NO_RULING"
        return record
    stored = json.loads(stored_ruling_path.read_text(encoding="utf-8"))

    required = ["prepare-summary.json", "rss.json", "paired-xz.json",
                "paired-brotli.json", "paired-null.json"]
    missing = [f for f in required if not (artifact / f).is_file()]
    if missing:
        record["status"] = "MISSING_INPUTS"
        record["missing"] = missing
        return record

    out = work / f"{run_dir.name}-{artifact.name}" / "ruling.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    args = argparse.Namespace(
        prepare=artifact / "prepare-summary.json",
        rss=artifact / "rss.json",
        xz_pair=artifact / "paired-xz.json",
        brotli_pair=artifact / "paired-brotli.json",
        null_pair=artifact / "paired-null.json",
        out=out,
        encode=None,
        decoder_bytes=None,
    )
    arbiter.summarize(args)
    replayed = json.loads(out.read_text(encoding="utf-8"))

    stored_gates = stored.get("gates") or {}
    replayed_gates = replayed.get("gates") or {}
    gates_equal = all(
        replayed_gates.get(k) == stored_gates.get(k) for k in legacy_gate_keys()
    ) and set(replayed_gates.keys()) == set(stored_gates.keys())

    record["stored_ruling"] = stored.get("ruling")
    record["replayed_ruling"] = replayed.get("ruling")
    record["ruling_identical"] = stored.get("ruling") == replayed.get("ruling")
    record["gates_identical"] = gates_equal
    record["scope_note_identical"] = stored.get("scope_note") == replayed.get("scope_note")
    record["stored_sha256"] = sha256_bytes(stored_ruling_path.read_bytes())
    record["replayed_sha256"] = sha256_bytes(out.read_bytes())
    record["stored_normalized_sha256"] = normalized_sha256(stored_ruling_path)
    record["replayed_normalized_sha256"] = normalized_sha256(out)
    record["byte_identical"] = (record["stored_sha256"] == record["replayed_sha256"])
    record["normalized_identical"] = (
        record["stored_normalized_sha256"] == record["replayed_normalized_sha256"])

    # ---- superset check: the extension must not rewrite anything legacy ----
    enc, dec = synthetic_axis_inputs()
    out2 = out.with_name("ruling-extended.json")
    args2 = argparse.Namespace(**{**vars(args), "out": out2,
                                   "encode": enc, "decoder_bytes": dec})
    arbiter.summarize(args2)
    extended = json.loads(out2.read_text(encoding="utf-8"))
    legacy_gates_after = {k: (extended.get("gates") or {}).get(k)
                          for k in legacy_gate_keys()}
    stored_gates_map = {k: stored_gates.get(k) for k in legacy_gate_keys()}
    record["superset_ruling_unchanged"] = extended.get("ruling") == stored.get("ruling")
    record["superset_legacy_gates_unchanged"] = legacy_gates_after == stored_gates_map
    record["superset_legacy_scope_note_unchanged"] = (
        extended.get("scope_note") == stored.get("scope_note"))
    record["superset_added_gates"] = sorted(
        set((extended.get("gates") or {}).keys()) - set(stored_gates.keys()))
    record["superset_extended_ruling"] = extended.get("extended_ruling")

    # ---- independent recomputation with the programme's shared tool ----
    paired: dict[str, Any] = {}
    for tag in ("xz", "brotli", "null"):
        j = json.loads((artifact / f"paired-{tag}.json").read_text(encoding="utf-8"))
        paired[tag] = {
            "paired_ratio_candidate_over_control": j.get("paired_ratio_candidate_over_control"),
            "paired_ratio_ci95": j.get("paired_ratio_ci95"),
            "control_median_s": (j.get("control") or {}).get("median_s"),
            "candidate_median_s": (j.get("candidate") or {}).get("median_s"),
            "timing_valid": j.get("timing_valid"),
            "clears_speed_gate": j.get("clears_speed_gate"),
        }
    recomputed = manifest_tool.checks_for(stored, paired)
    record["manifest_recomputed_checks"] = recomputed
    # The two byte conditions live in the arbiter's ruling as the top-level
    # fields `bytes_beat_xz` / `bytes_beat_brotli` (the `gates` map carries
    # the six cost-margin conditions); the shared manifest tool restates them
    # as `bytes_below_xz` / `bytes_below_brotli`.
    record["manifest_vs_arbiter_gates"] = {
        "bytes_below_xz": recomputed.get("bytes_below_xz") == stored.get("bytes_beat_xz"),
        "bytes_below_brotli": recomputed.get("bytes_below_brotli") == stored.get("bytes_beat_brotli"),
        "decode_xz_2x": recomputed.get("decode_xz_2x") == stored_gates.get("decode_within_2x_xz"),
        "decode_brotli_2x": recomputed.get("decode_brotli_2x") == stored_gates.get("decode_within_2x_brotli"),
        "memory_xz_2x": recomputed.get("memory_xz_2x") == stored_gates.get("memory_within_2x_xz"),
        "memory_brotli_2x": recomputed.get("memory_brotli_2x") == stored_gates.get("memory_within_2x_brotli"),
        "a_a_null_ci_spans_one": recomputed.get("a_a_null_ci_spans_one") == stored_gates.get("null_ci_spans_one"),
        "timing_valid": recomputed.get("timing_valid") == stored_gates.get("timing_valid"),
    }
    record["manifest_recompute_matches_arbiter_gates"] = all(
        record["manifest_vs_arbiter_gates"].values())

    failures = []
    for key in ("ruling_identical", "gates_identical", "scope_note_identical",
                "normalized_identical", "superset_ruling_unchanged",
                "superset_legacy_gates_unchanged",
                "superset_legacy_scope_note_unchanged"):
        if not record.get(key):
            failures.append(key)
    if not record["manifest_recompute_matches_arbiter_gates"]:
        failures.append("manifest_recompute_matches_arbiter_gates")
    record["failures"] = failures
    record["status"] = "PASS" if not failures else "FAIL"
    return record


def md_report(payload: dict[str, Any]) -> str:
    rows = payload["runs"]
    out = [
        "# I11 (D5) regression proof - the extended arbiter changes no I20 verdict",
        "",
        "Generated by `tools/i11_replay_reference_gate.py` by REPLAYING the stored I20",
        "artefacts under `tests/i20-evidence/runs/<run_id>/` with the extended",
        "`tools/reference_cost_gate.py`.  No measurement is performed here: the harness",
        "re-runs the arbiter's own `summarize` stage over raw artefacts that were",
        "already measured on the hosted runner.",
        "",
        "| run / artefact | stored ruling | replayed ruling | ruling identical | gates identical | scope_note identical | normalized bytes identical | superset: legacy ruling + gates + scope_note unchanged | shared-manifest recompute == arbiter gates | verdict |",
        "|---|---|---|---|---|---|---|---|---|---|---|",
    ]
    for r in rows:
        out.append(
            f"| `{r['run']}` | {r.get('stored_ruling')} | {r.get('replayed_ruling')} | "
            f"{r.get('ruling_identical')} | {r.get('gates_identical')} | "
            f"{r.get('scope_note_identical')} | {r.get('normalized_identical')} | "
            f"{r.get('superset_ruling_unchanged')} / {r.get('superset_legacy_gates_unchanged')} / "
            f"{r.get('superset_legacy_scope_note_unchanged')} | "
            f"{r.get('manifest_recompute_matches_arbiter_gates')} | **{r.get('status')}** |")
    out += [
        "",
        "## Gate keys that the extension added (must be additive only)",
        "",
    ]
    added = sorted({k for r in rows for k in (r.get("superset_added_gates") or [])})
    out += [f"- `{k}`" for k in added] or ["- (none)"]
    out += [
        "",
        "## Verdict",
        "",
        f"- runs replayed: **{payload['counts']['runs']}**",
        f"- runs reproduced exactly: **{payload['counts']['pass']}**",
        f"- runs failed: **{payload['counts']['fail']}**",
        f"- stored ruling strings reproduced: "
        f"{', '.join(sorted({str(r.get('stored_ruling')) for r in rows if r.get('stored_ruling')}))}",
        "",
    ]
    return "\n".join(out) + "\n"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("evidence_dir", type=Path,
                    help="directory holding runs/<run_id>/<artifact>/...")
    ap.add_argument("--out-dir", type=Path, required=True)
    ap.add_argument("--out", type=Path,
                    help="explicit path for the JSON proof (default <out-dir>/regression-proof.json)")
    ap.add_argument("--md", type=Path,
                    help="explicit path for the Markdown proof")
    args = ap.parse_args()

    runs = args.evidence_dir / "runs"
    if not runs.is_dir():
        raise SystemExit(f"no runs directory under {args.evidence_dir}")
    rows: list[dict[str, Any]] = []
    with tempfile.TemporaryDirectory(prefix="i11-replay-") as td:
        work = Path(td)
        for run_dir in sorted(p for p in runs.iterdir() if p.is_dir()):
            for artifact in sorted(p for p in run_dir.iterdir() if p.is_dir()):
                rows.append(replay_one(run_dir, artifact, work))

    counts = {
        "runs": len(rows),
        "pass": sum(1 for r in rows if r.get("status") == "PASS"),
        "fail": sum(1 for r in rows if r.get("status") == "FAIL"),
        "skipped": sum(1 for r in rows if r.get("status") not in ("PASS", "FAIL")),
    }
    payload = {
        "schema": 1,
        "tool": "tools/i11_replay_reference_gate.py",
        "arbiter": "tools/reference_cost_gate.py",
        "evidence_dir": str(args.evidence_dir),
        "legacy_gate_keys": legacy_gate_keys(),
        "counts": counts,
        "runs": rows,
    }
    args.out_dir.mkdir(parents=True, exist_ok=True)
    json_out = args.out or (args.out_dir / "regression-proof.json")
    md_out = args.md or (args.out_dir / "regression-proof.md")
    json_out.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n",
                        encoding="utf-8")
    md_out.write_text(md_report(payload), encoding="utf-8")
    print(md_report(payload))
    if counts["fail"]:
        raise SystemExit(f"REGRESSION FAILED: {counts['fail']} run(s) did not reproduce")
    print(f"REGRESSION PASS: {counts['pass']}/{counts['runs']} stored runs reproduced exactly")


if __name__ == "__main__":
    main()
