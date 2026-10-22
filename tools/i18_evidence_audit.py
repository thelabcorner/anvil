#!/usr/bin/env python3
"""Independent, cheap integrity/consistency audit of I18 Actions evidence.

Run against an extracted Actions artifact; does not execute codecs or
reconstruct unavailable timings. Strict failures preserve negative evidence.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import re
from pathlib import Path

FILES = (
    "synth-arith.bin", "synth-timeseries.bin", "synth-jitter.bin",
    "random.bin", "generated.repeat.jsonl", "generated.jsonl", "src.cpp",
)
MODES = {0: "RAW", 1: "AVI4", 2: "BROTLI_Q5", 4: "AVI6"}
SOURCES = {
    "prototypes/i18-budgeted/i18_budgeted.cpp": "e5fd1a2c44d697f7cfd49427e1ca3654b192cfff07168e67409007b6f97d65e2",
    "prototypes/i18-budgeted/i16_fusion_frozen.inc": "b214dcff0355293dd588bd762c7328dfce6e114749669f2b7a32d8c052f51dc3",
    "prototypes/i17-fast/i17_fast.cpp": "727469a41c5506d73f85d0fdc1405f067c1206ad5de94063f1a06605b15c9cf6",
    "prototypes/i17-fast/i17_fast_entry_renamed.inc": "dbd0b11cac966a7c5aedf603c3ba7d30b8a4f63b039deb48cebbadbc37d02615",
    "tools/i18_decide.py": "44cfe588fb94d7f05002289cfe1d273a34824722fb65dd440f31cc6a6fb39afe",
}
EXPECTED_COMMIT = "7ec50e8dadd1b3cc12d815c80007a9fcc2bfd490"
EXPECTED_BROTLI = "ed738e842d2fbdf2d6459e39267a633c4a9b2f5d"
RATE_KEYS = (
    "budget_encode_MBps", "fast_encode_MBps", "full_encode_MBps", "q5_encode_MBps",
    "budget_decode_MBps", "fast_decode_MBps", "full_decode_MBps",
    "q5_decode_MBps", "q11_decode_MBps",
)
NUMBER_KEYS = (
    "input_bytes", "budget_bytes", "fast_bytes", "full_bytes", "q5_bytes", "q11_bytes",
    "budget_mode", "probed", "regular", "skipped_by_ratio",
    "math_candidate_calls", "avi4_bytes", "avi6_bytes",
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def canonical_uleb_size(value: int) -> int:
    return max(1, (value.bit_length() + 6) // 7)


def audit(artifact: Path, source_root: Path) -> dict:
    require(artifact.is_dir(), f"artifact missing: {artifact}")
    for relative, expected in SOURCES.items():
        path = source_root / relative
        require(path.is_file() and sha256(path) == expected,
                f"frozen source digest mismatch: {relative}")
    provenance = (artifact / "provenance.txt").read_text(encoding="utf-8")
    require(f"dispatch_commit={EXPECTED_COMMIT}" in provenance, "unexpected dispatch commit")
    require(f"brotli_source_commit={EXPECTED_BROTLI}" in provenance, "Brotli provenance mismatch")
    require("role=preconsumed-discovery" in provenance, "missing consumed-discovery label")
    require(f"i18_source_sha={SOURCES['prototypes/i18-budgeted/i18_budgeted.cpp']}" in provenance,
            "source provenance mismatch")

    with (artifact / "benchmark.tsv").open(encoding="utf-8", newline="") as handle:
        rows = list(csv.DictReader(handle, delimiter="\t"))
    require([r["file"] for r in rows] == list(FILES), "benchmark file roster/order invalid")
    report = json.loads((artifact / "decision.json").read_text(encoding="utf-8"))
    require(report["schema"] == "anvil.i18.consumed-discovery.v1", "unknown report schema")
    require(report["consumed_discovery"] is True and
            report["promotion_authorized"] is False and
            report["general_pareto_crossing"] is False and
            report["novelty_claimed"] is False, "unsupported scientific promotion")
    require(report["verdict"] == "BUDGETED-SPECIALIZED-DISCOVERY", "unexpected recorded verdict")
    require(len(report["records"]) == len(rows), "decision report row count mismatch")

    totals = {key: 0 for key in ("budget", "fast", "full", "q5", "q11")}
    skipped_negatives = 0
    for index, (row, record) in enumerate(zip(rows, report["records"], strict=True)):
        name = FILES[index]
        require(record["file"] == name, f"report roster mismatch: {name}")
        vals = {}
        for key in NUMBER_KEYS:
            try:
                n = int(row[key])
            except (KeyError, TypeError, ValueError) as exc:
                raise ValueError(f"{name}: invalid integer field {key}") from exc
            require(n >= 0 and str(n) == row[key], f"{name}: noncanonical {key}")
            vals[key] = n
        for key in RATE_KEYS:
            try:
                n = float(row[key])
            except (KeyError, TypeError, ValueError) as exc:
                raise ValueError(f"{name}: invalid rate {key}") from exc
            require(math.isfinite(n) and n > 0, f"{name}: invalid positive rate {key}")
        mode = vals["budget_mode"]
        require(mode in MODES, f"{name}: unknown mode")
        require(vals["probed"] in (0, 1) and vals["regular"] in (0, 1)
                and vals["skipped_by_ratio"] in (0, 1), f"{name}: nonboolean flags")
        require(vals["math_candidate_calls"] in (0, 2), f"{name}: invalid candidate count")
        require((mode in (1, 4)) <= (vals["math_candidate_calls"] == 2),
                f"{name}: math selected without candidate")
        if vals["math_candidate_calls"] == 2:
            require(vals["probed"] == vals["regular"] == 1 and not vals["skipped_by_ratio"],
                    f"{name}: math executed despite rejection")
        if vals["skipped_by_ratio"]:
            require(vals["math_candidate_calls"] == 0, f"{name}: ratio skip leaked work")
        if vals["regular"] == 0:
            require(vals["math_candidate_calls"] == 0, f"{name}: negative probe leaked work")
        size = vals["input_bytes"]
        framing = 5 + canonical_uleb_size(size)
        payloads = {
            0: size, 1: vals["avi4_bytes"],
            2: vals["q5_bytes"], 4: vals["avi6_bytes"],
        }
        require(vals["budget_bytes"] == framing + payloads[mode],
                f"{name}: complete frame accounting mismatch")
        require(vals["budget_bytes"] <= size + framing, f"{name}: RAW bound exceeded")
        for tag in totals:
            key = tag + "_bytes"
            totals[tag] += vals[key]
            require(record[key] == vals[key], f"{name}: decision differs from TSV {key}")
        require(record["budget_mode"] == mode and
                record["math_candidate_calls"] == vals["math_candidate_calls"] and
                record["input_bytes"] == size, f"{name}: report control mismatch")
        require(record["byte_delta_vs_fast"] == vals["budget_bytes"] - vals["fast_bytes"]
                and record["byte_regret_vs_full"] == vals["budget_bytes"] - vals["full_bytes"]
                and record["byte_delta_vs_q5"] == vals["budget_bytes"] - vals["q5_bytes"]
                and record["byte_delta_vs_q11"] == vals["budget_bytes"] - vals["q11_bytes"],
                f"{name}: byte delta reporting mismatch")
        for phase in ("encode", "decode"):
            for key, amount in record[phase + "_MBps"].items():
                require(math.isclose(float(row[key + "_" + phase + "_MBps"]), amount,
                                     rel_tol=0, abs_tol=0), f"{name}: rate report mismatch")
        if index >= 2:
            skipped_negatives += vals["math_candidate_calls"] == 0
    require(totals == report["aggregate_bytes"], "incorrect aggregate byte totals")
    require(skipped_negatives == report["negative_skips"] == 5,
            "invalid negative-control accounting")

    roundtrip = (artifact / "roundtrips.txt").read_text(encoding="utf-8").splitlines()
    expected = [f"ROUNDTRIP_PASS {profile} tests/corpus/{name}"
                for name in FILES for profile in ("budget", "fast", "full")]
    require(roundtrip == expected, "incomplete or reordered 21-profile roundtrip proof")
    selftest = (artifact / "selftest.txt").read_text(encoding="utf-8")
    require("I18_SELFTEST PASS cases=14 invalid=5" in selftest and
            "FAST_SELFTEST PASS" in selftest, "selftest evidence missing")
    for name in ("synth-arith.bin", "synth-timeseries.bin", "generated.jsonl"):
        for phase in ("encode", "decode"):
            rss = (artifact / f"rss-{phase}-{name}.txt").read_text(encoding="utf-8")
            matched = re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)", rss)
            require(matched is not None and int(matched.group(1)) > 0,
                    f"missing RSS result: {phase} {name}")
            require(re.search(r"Exit status:\s*0", rss) is not None,
                    f"RSS process failed: {phase} {name}")
    binary_size = (artifact / "binary-sizes.txt").read_text(encoding="utf-8")
    require(binary_size.startswith("1062832 "), "binary-size attestation unexpected")
    return {"verdict": "AUDIT_PASS", "commit": EXPECTED_COMMIT,
            "source_attested": len(SOURCES), "cases": len(FILES),
            "roundtrips": len(expected), "negative_skips": skipped_negatives,
            "aggregate_bytes": totals, "scientific_status": "consumed-discovery-only"}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifact", type=Path)
    parser.add_argument("--source-root", type=Path,
                        default=Path(__file__).resolve().parent.parent)
    arguments = parser.parse_args()
    print(json.dumps(audit(arguments.artifact, arguments.source_root), sort_keys=True))


if __name__ == "__main__":
    main()
