#!/usr/bin/env python3
"""I15 frozen discovery judge. Run only after remote compile, roundtrip and paired benchmark."""
import csv
import json
import math
import sys

NAMES = [
    "synth-arith.bin", "synth-timeseries.bin", "synth-jitter.bin",
    "random.bin", "generated.repeat.jsonl", "generated.jsonl", "src.cpp",
]

def read(path):
    with open(path, newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle, delimiter="\t"))
    if [r["file"] for r in rows] != NAMES:
        raise ValueError("discovery inputs not source-locked in fixed order")
    return rows

def main():
    i13, i15 = read(sys.argv[1]), read(sys.argv[2])
    report = {"schema": "anvil.i15.discovery/v1", "role": "discovery",
              "frontier_crossing": False, "promotion_authorized": False, "records": []}
    for a, b in zip(i13, i15):
        input_bytes = int(b["input_bytes"])
        wire = int(b["wire_bytes"])
        old = int(a["wire_bytes"])
        brotli = int(b["brotli_q11_bytes"])
        if int(a["input_bytes"]) != input_bytes or int(a["brotli_q11_bytes"]) != brotli:
            raise ValueError("unpaired controls")
        if int(b["raw_blocks"]) + int(b["model_blocks"]) + int(b["column_blocks"]) != (input_bytes + 4095)//4096:
            raise ValueError("I15 block count mismatch")
        if not (0 <= int(b["column_diff_models"]) <= int(b["modeled_columns"])):
            raise ValueError("I15 column-DIFF count invalid")
        if not (0 <= int(b["diff_blocks"]) <= int(b["model_blocks"])):
            raise ValueError("I15 block-DIFF count invalid")
        if wire > old:
            raise ValueError("retained I13 candidates regressed in serialized size")
        i15_decode = float(b["decode_MBps"])
        i13_decode = float(a["decode_MBps"])
        q11_decode = float(b["brotli_q11_decode_MBps"])
        e = float(b["encode_MBps"])
        if min(input_bytes, wire, old, brotli) <= 0 or not all(
            math.isfinite(x) and x > 0 for x in (i15_decode, i13_decode, q11_decode, e)
        ):
            raise ValueError("invalid size or rate")
        report["records"].append({
            "file": b["file"], "input_bytes": input_bytes,
            "i15_wire_bytes": wire, "i13_wire_bytes": old,
            "brotli_q11_wire_bytes": brotli, "i15_model_blocks": int(b["model_blocks"]),
            "i15_diff_blocks": int(b["diff_blocks"]),
            "i15_column_blocks": int(b["column_blocks"]),
            "i15_modeled_columns": int(b["modeled_columns"]),
            "i15_column_diff_models": int(b["column_diff_models"]),
            "i15_encode_MBps": e, "i15_decode_MBps": i15_decode,
            "i13_decode_MBps": i13_decode, "brotli_q11_decode_MBps": q11_decode,
        })
    ar, timeseries = report["records"][:2]
    gates = {
        "H1_column_diff_selected_timeseries": timeseries["i15_column_diff_models"] > 0,
        "H2_smaller_than_i13_timeseries": timeseries["i15_wire_bytes"] < timeseries["i13_wire_bytes"],
        "H2_smaller_than_brotli_q11_timeseries": timeseries["i15_wire_bytes"] < timeseries["brotli_q11_wire_bytes"],
        "H3_timeseries_decode_faster_than_brotli_q11":
            timeseries["i15_decode_MBps"] > timeseries["brotli_q11_decode_MBps"],
        "H4_arithmetic_byte_nonregression_vs_i14": ar["i15_wire_bytes"] <= 33196
    }
    if not gates["H4_arithmetic_byte_nonregression_vs_i14"]:
        raise ValueError("arithmetic I14 candidate unexpectedly regressed")
    verdict = (
        "KILL-STRIDED-DIFF" if not gates["H1_column_diff_selected_timeseries"] else
        "SELECTED-NO-INCREMENT" if not gates["H2_smaller_than_i13_timeseries"] else
        "STRUCTURE-ADVANCE-NO-BROTLI-CROSSING" if not gates["H2_smaller_than_brotli_q11_timeseries"] else
        "RATIO-ONLY-STRIDED-DISCOVERY" if not gates["H3_timeseries_decode_faster_than_brotli_q11"] else
        "SPECIALIZED-STRIDED-DISCOVERY-ADVANCE"
    )
    report["gates"], report["ruling"] = gates, verdict
    with open(sys.argv[3], "w", encoding="utf-8") as out:
        json.dump(report, out, indent=2, sort_keys=True)
        out.write("\n")
    print(json.dumps(report, indent=2, sort_keys=True))

if __name__ == "__main__":
    main()
