#!/usr/bin/env python3
"""Conservative *secondary* all-axis Pareto claim audit for ANVIL.

NOT the frozen I9 2-D FRONT-CROSSING gate, and NEVER a universal-proof engine.
A valid report is scoped to one byte-identical input, named baselines, one
hosted run, and observed time/memory/software-distribution evidence. It cannot
certify general-purpose superiority or replace frozen independent evaluation.

Requires experiment manifest JSON described in docs/ALL-AXIS-PARETO-AUDIT.md.
Only local scalar parsing/math; no codec, benchmark, network or mutation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import statistics
import sys
from pathlib import Path

SHA256 = re.compile(r"^[0-9a-fA-F]{64}$")
PASS_GATES = ("roundtrip", "malformed_input", "wire_compatibility", "streaming")
TIME_AXES = ("encode", "decode")
MIN_RSS_REPS = 3
MIN_TIME_PAIRS = 7
VALID = {"OBSERVED_SCOPE_DOMINANCE", "OBSERVED_NONINFERIOR_NO_STRICT_GAIN"}


def positive_int(obj: dict, field: str) -> int:
    value = obj[field]
    if type(value) is not int or value <= 0:
        raise ValueError(f"{field}: positive integer required")
    return value


def ratio_ci(obj: dict, key: str, problems: list[str],
             run_id: str) -> tuple[float, float] | None:
    """CI bounds for candidate/reference TIME, never throughput inversion."""
    try:
        meta = obj[key]
        if str(meta["run_id"]) != run_id or not run_id:
            raise ValueError("pairing must be from one identical hosted run")
        if meta.get("timing_valid") is not True:
            raise ValueError("timing_valid not true")
        if positive_int(meta, "raw_pairs") < MIN_TIME_PAIRS:
            raise ValueError("too few paired raw repetitions")
        if not meta.get("raw_evidence_sha256") or not SHA256.fullmatch(
                meta["raw_evidence_sha256"]):
            raise ValueError("raw evidence artifact SHA-256 missing")
        lo, hi = (float(v) for v in meta["ci95"])
        if not (math.isfinite(lo) and math.isfinite(hi) and 0 < lo <= hi):
            raise ValueError("invalid time-ratio CI")
        for control in ("null_pre_ci95", "null_post_ci95"):
            a, b = (float(v) for v in meta[control])
            if not (math.isfinite(a) and math.isfinite(b)
                    and 0 < a <= 1 <= b):
                raise ValueError(f"{control}: null does not span 1.0")
        return lo, hi
    except (KeyError, TypeError, ValueError, OverflowError) as exc:
        problems.append(f"{key} timing not verified: {exc}")
        return None


def check_samples(obj: dict, problems: list[str], tag: str) -> list[int] | None:
    data = obj.get("peak_rss_samples_bytes")
    if (not isinstance(data, list) or len(data) < MIN_RSS_REPS
            or any(type(v) is not int or v <= 0 for v in data)):
        problems.append(f"{tag}: >=3 valid peak-RSS process observations required")
        return None
    return data


def check_input(scope: dict, candidate: dict, ref: dict,
                pair: dict) -> tuple[list[str], list[str]]:
    missing, hard = [], []
    try:
        if not SHA256.fullmatch(str(scope["input_sha256"])):
            raise ValueError("invalid input SHA256")
        if positive_int(scope, "input_bytes") <= 0:
            raise ValueError("empty input")
        if not re.fullmatch(r"[0-9a-fA-F]{40}", str(scope["source_commit"])):
            raise ValueError("source commit must be full 40-hex git SHA")
        if not re.fullmatch(r"[0-9]+", str(scope["run_id"])):
            raise ValueError("same-job GitHub Actions run ID missing")
        if not str(scope["runner_fingerprint"]).strip():
            raise ValueError("runner hardware class/fingerprint missing")
        if not SHA256.fullmatch(str(scope["benchmark_tool_sha256"])):
            raise ValueError("benchmark tool content SHA256 missing")
        if not str(scope["file"]).strip():
            raise ValueError("missing per-file scope")
    except (KeyError, ValueError, TypeError) as exc:
        missing.append(f"source identity: {exc}")
    for name, obj in (("candidate", candidate), ("reference", ref)):
        if obj.get("decoded_sha256") != scope.get("input_sha256"):
            hard.append(f"{name}: decoded SHA256 not the expected raw input")
        try:
            positive_int(obj, "complete_wire_bytes")
            positive_int(obj, "decoder_distribution_bytes")
        except (KeyError, ValueError, TypeError) as exc:
            missing.append(f"{name}: missing required footprint {exc}")
        if obj.get("decoder_footprint_includes_dependencies") is not True:
            missing.append(f"{name}: decoder dependency/model assets not charged")
        for artifact in ("binary_sha256", "rss_evidence_sha256"):
            if not SHA256.fullmatch(str(obj.get(artifact, ""))):
                missing.append(f"{name}: invalid or missing {artifact}")
        if not re.fullmatch(r"[0-9a-fA-F]{40}", str(obj.get("code_commit", ""))):
            missing.append(f"{name}: code_commit must be full 40-hex SHA")
        gates = obj.get("hard_gates")
        if not isinstance(gates, dict):
            missing.append(f"{name}: no hard-gate evidence object")
            gates = {}
        for gate in PASS_GATES:
            state = gates.get(gate)
            if state == "fail":
                hard.append(f"{name}: {gate} FAILED")
            elif state != "pass":
                missing.append(f"{name}: hard gate '{gate}' not yet verified")
    for feature in ("streaming", "random_access"):
        # Boolean capabilities measured over *same* input/format scope.
        c = candidate.get("capabilities", {}).get(feature)
        r = ref.get("capabilities", {}).get(feature)
        if type(c) is not bool or type(r) is not bool:
            missing.append(f"{feature}: capability evidence missing")
        elif r and not c:
            hard.append(f"{feature}: candidate loses reference capability")
    if pair.get("reference") != ref.get("codec"):
        missing.append("paired reference identity differs from ref codec")
    if candidate.get("codec") == ref.get("codec"):
        hard.append("candidate and reference codec names must differ")
    return missing, hard


def compare(manifest: dict, locked_references: tuple[str, ...] | None = None) -> dict:
    """Produce conservative witnessed status for each *named* baseline."""
    problems = []
    if manifest.get("schema") != 1:
        problems.append("expected schema 1")
    scope = manifest.get("scope", {})
    candidate = manifest.get("candidate", {})
    refs = manifest.get("references", [])
    pairs = manifest.get("paired", [])
    required = manifest.get("required_references", [])
    if not isinstance(scope, dict) or not isinstance(candidate, dict):
        return {"status": "INVALID_SCHEMA", "errors": ["scope/candidate must be objects"]}
    if not isinstance(refs, list) or not isinstance(pairs, list):
        return {"status": "INVALID_SCHEMA", "errors": ["references/paired must be lists"]}
    if not isinstance(required, list) or not required or any(
            not isinstance(x, str) or not x for x in required):
        problems.append("manifest has no valid required_references")
        required = []
    if len(required) != len(set(required)):
        problems.append("duplicate names in manifest required_references")
    if not locked_references or len(locked_references) != len(set(locked_references)):
        problems.append("INDEPENDENT immutable reference lock is mandatory")
    elif set(required) != set(locked_references):
        problems.append("manifest reference list differs from externally frozen reference lock")
    ref_index = {r.get("codec"): r for r in refs if isinstance(r, dict)}
    pair_index = {r.get("reference"): r for r in pairs if isinstance(r, dict)}
    if len(ref_index) != len(refs) or len(pair_index) != len(pairs):
        problems.append("duplicate/unnamed reference or pairing")
    if set(ref_index) != set(required) or set(pair_index) != set(required):
        problems.append("missing or extra reference/paired evidence for required set")

    findings = []
    for name in required:
        if name not in ref_index or name not in pair_index:
            findings.append({"reference": name, "status": "INCOMPLETE",
                             "issues": ["missing reference or paired arm"]})
            continue
        ref, pair = ref_index[name], pair_index[name]
        missing, hard = check_input(scope, candidate, ref, pair)
        # Exact source bytes and required decoder distribution costs are
        # necessary, but their numerical relation is checked independently.
        improvements = []
        regressed = []
        if not missing and not hard:
            for field in ("complete_wire_bytes", "decoder_distribution_bytes"):
                c, r = candidate[field], ref[field]
                if c > r:
                    regressed.append(f"{field}: {c} > {r}")
                elif c < r:
                    improvements.append(field)
        c_samples = check_samples(candidate, missing, "candidate")
        r_samples = check_samples(ref, missing, name)
        if c_samples and r_samples:
            # Strong *observed* separation. This is NOT a statistical
            # guarantee outside the sample/host.
            if min(c_samples) > max(r_samples):
                regressed.append("peak RSS: all candidate samples exceed all reference samples")
            elif max(c_samples) <= min(r_samples):
                if max(c_samples) < min(r_samples):
                    improvements.append("observed_peak_rss")
            else:
                missing.append("RSS samples overlap: no certified nonregression bound")
        time_results = {}
        for axis in TIME_AXES:
            ci = ratio_ci(pair, axis, missing, str(scope.get("run_id", "")))
            if ci is None:
                continue
            lo, hi = ci
            time_results[axis] = {"candidate_to_reference_ci95": [lo, hi]}
            if lo > 1:
                regressed.append(f"{axis} time: CI lower {lo} > 1")
            elif hi <= 1:
                if hi < 1:
                    improvements.append(f"{axis}_time")
            else:
                missing.append(f"{axis} time: confidence interval overlaps regression")
        if hard or regressed:
            status = "OBSERVED_REGRESSION_OR_HARD_FAIL"
        elif missing:
            status = "INCOMPLETE_OR_STATISTICALLY_INCONCLUSIVE"
        elif improvements:
            status = "OBSERVED_SCOPE_DOMINANCE"
        else:
            status = "OBSERVED_NONINFERIOR_NO_STRICT_GAIN"
        findings.append({
            "reference": name, "status": status, "improvements": improvements,
            "regressions": regressed + hard, "unproven": missing,
            "paired_time": time_results,
        })
    complete = bool(required) and all(
        f["status"] == "OBSERVED_SCOPE_DOMINANCE" for f in findings)
    return {
        "schema": 1,
        "scope": scope,
        "status": ("ALL_REQUIRED_REFERENCES_OBSERVED_SCOPE_DOMINANCE"
                   if complete and not problems
                   else "NOT_ALL_REQUIRED_REFERENCES_DOMINATED"),
        "per_reference": findings,
        "integrity_errors": problems,
        "strong_claim_prohibited": (
            "This tests one file on one runner against declared reference modes; "
            "it does not prove statistical RSS non-regression, all files, all "
            "common codecs, held-out generality, safety under arbitrary data "
            "or universal Pareto dominance. Retain frozen Class-A gate."
        ),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("manifest", type=Path, help="One-file, same-run all-axis evidence JSON")
    ap.add_argument("--reference-lock", type=Path, required=True,
                    help="Separate pre-registered reference-class JSON file")
    ap.add_argument("--reference-lock-sha256", required=True,
                    help="SHA256 pinned from the source-controlled preregistration")
    ap.add_argument("--out", type=Path)
    args = ap.parse_args()
    lock_bytes = args.reference_lock.read_bytes()
    actual = hashlib.sha256(lock_bytes).hexdigest()
    if not SHA256.fullmatch(args.reference_lock_sha256) or (
            actual != args.reference_lock_sha256.lower()):
        print("invalid external reference lock SHA256", file=sys.stderr)
        return 2
    lock = json.loads(lock_bytes)
    expected = lock.get("required_references")
    if lock.get("schema") != 1 or not isinstance(expected, list):
        print("invalid external reference-lock schema", file=sys.stderr)
        return 2
    result = compare(json.loads(args.manifest.read_text(encoding="utf-8")),
                     tuple(expected))
    data = json.dumps(result, indent=2, sort_keys=True) + "\n"
    print(data, end="")
    if args.out:
        args.out.write_text(data, encoding="utf-8")
    # Exit 0 means a *scoped* all-reference observation passed.
    # A regression, incomplete evidence, or unproven timing must never
    # silently present as CI success merely because JSON printed correctly.
    if result.get("integrity_errors") or result.get("status") == "INVALID_SCHEMA":
        return 2
    return 0 if result["status"] == "ALL_REQUIRED_REFERENCES_OBSERVED_SCOPE_DOMINANCE" else 1


if __name__ == "__main__":
    sys.exit(main())
