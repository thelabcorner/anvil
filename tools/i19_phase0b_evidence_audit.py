#!/usr/bin/env python3
"""Independent bounded audit of I19 Phase-0b CI evidence. No codec work.

Re-hashes all four nonheldout typed streams, compares against source-locked
expected values, and proves the artifact contains no heldout payload.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path

SOURCE_RUN = 37704571734
SOURCE_COMMIT = "c7e423e24ab94e1156f1245c1a4a06397ef15a70"
SEMANTIC_COMMIT = "c09375511cc56f582c5147204bdaa2a2450e3e4f"
CASES = {
    "noaa_kmci_daily__tmax.bin": (20119, 40238, "int16le", "discovery",
        "5fa00acbcc1d7e6f7341158e778dc1859cfb9aa838b7ab5617d214b37e968674"),
    "noaa_kmci_daily__tmin.bin": (20119, 40238, "int16le", "discovery",
        "74186bb2e58580be7b011a00e6bde9694a215d8e31d6d9ea679804137dcadf1f"),
    "usgs_potomac_daily__discharge.bin": (4018, 16072, "int32le", "validation",
        "43216110c8a695493117902e066b8c42ce05c1b252d8e304fcc22ef875217590"),
    "nasa_power_t2m_daily__t2m.bin": (4018, 16072, "int32le", "validation",
        "83d9d384e4d2063724769b078cbac51b23400a0a8ed10c40d3e3a23870684b49"),
}
HELDOUT_SHA = "9f84b46ade8a2d8e1286ec4b2b6c2987a45a755c59f263be3b3b3d10dfbda3ff"

def check(flag, why):
    if not flag:
        raise ValueError(why)

def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def audit(folder: Path) -> dict:
    check(folder.is_dir(), "missing artifact")
    report_path = folder / "results/semantic-report.json"
    report = json.loads(report_path.read_text(encoding="utf-8"))
    check(report["schema"] == "anvil.i19.typed-preparation.v1", "unknown schema")
    check(report["status"] == "THREE_ORIGIN_TYPED_SELECTIONS_READY_UNADMITTED",
          "typed source semantics not passed")
    check(report["origin_run_id"] == SOURCE_RUN and report["origin_commit"] == SOURCE_COMMIT,
          "source revision mismatch")
    check(report["codec_efficacy_measured"] is False and report["heldout_unsealed"] is True
          and report["promotion_authorized"] is False, "invalid scientific promotion flags")
    provenance = (folder / "provenance.txt").read_text("utf-8")
    for text in (
        f"semantic_commit={SEMANTIC_COMMIT}",
        f"locked_acquisition_run={SOURCE_RUN}",
        f"locked_acquisition_commit={SOURCE_COMMIT}",
        "phase=semantics-only", "efficacy_measured=false", "heldout_opaque=true",
    ):
        check(text in provenance.splitlines(), f"missing provenance {text}")
    check(len(report["outputs"]) == len(CASES) + 1, "unexpected output count")
    check((folder / "semantic-result.json").is_file()
          and "THREE_ORIGIN_TYPED_SELECTIONS_READY_UNADMITTED"
          in (folder / "semantic-result.json").read_text("utf-8"),
          "incomplete completion sentinel")
    checked = set()
    for item in report["outputs"]:
        if item["split"] == "heldout":
            check(item["source_id"] == "uci_household_electricity"
                  and item["status"] == "OPAQUE_UNPARSED_SHA_ONLY"
                  and item["source_sha256"] == HELDOUT_SHA
                  and item["source_bytes"] == 20640916
                  and "typed_path" not in item, "heldout data was parsed")
            continue
        check(item["status"] == "TYPED_SELECTION_ONLY"
              and item["original_source_reconstructible"] is False,
              "incomplete typed-only label")
        relative = item["typed_path"]
        check(relative.startswith("typed/") and "/" not in relative[6:]
              and "\\" not in relative, "unsafe path")
        name = relative[6:]
        check(name in CASES and name not in checked, "unrecognized or duplicated numeric stream")
        checked.add(name)
        samples, size, dtype, split, expected = CASES[name]
        p = folder / "results" / relative
        check(p.is_file() and p.stat().st_size == size, "typed file byte count mismatch")
        check(sha256(p) == expected == item["typed_sha256"], "typed digest mismatch")
        check(item["samples"] == samples and item["typed_bytes"] == size
              and item["dtype"] == dtype and item["split"] == split,
              "typed descriptor mismatch")
    check(checked == set(CASES), "missing typed stream")
    allowed = {"contracts.txt", "provenance.txt", "semantic-result.json",
               "results/semantic-report.json"} | {
                   "results/typed/" + name for name in CASES
               }
    actual = {str(p.relative_to(folder)).replace("\\", "/")
              for p in folder.rglob("*") if p.is_file()}
    check(actual == allowed, f"unexpected files or heldout payload in artifact: {actual - allowed}")
    return {"verdict": "PHASE0B_EVIDENCE_AUDIT_PASS", "source_run": SOURCE_RUN,
            "semantic_commit": SEMANTIC_COMMIT, "typed_streams": len(CASES),
            "typed_total_bytes": sum(x[1] for x in CASES.values()),
            "heldout_status": "SHA_ONLY_UNSEALED", "efficacy_claim": False}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifact", type=Path)
    args = parser.parse_args()
    print(json.dumps(audit(args.artifact), sort_keys=True))

if __name__ == "__main__":
    main()
