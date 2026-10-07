#!/usr/bin/env python3
"""Frozen I15-T discovery adjudicator; no automatic frontier promotion."""
import csv
import json
import math
import sys

NAMES = ["synth-arith.bin", "synth-timeseries.bin", "synth-jitter.bin",
         "random.bin", "generated.repeat.jsonl", "generated.jsonl", "src.cpp"]

def load(path):
    with open(path, encoding="utf-8", newline="") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    if [row["file"] for row in rows] != NAMES:
        raise ValueError("unpaired, missing, or reordered discovery inputs")
    return rows

def number(row, key):
    value = float(row[key])
    if not math.isfinite(value) or value <= 0:
        raise ValueError(f"invalid rate: {key}")
    return value

def main():
    original, tiled = load(sys.argv[1]), load(sys.argv[2])
    report = {"schema": "anvil.i15t.discovery/v1", "role": "consumed-discovery",
              "frontier_crossing": False, "promotion_authorized": False,
              "records": []}
    for base, new in zip(original, tiled):
        size = int(new["input_bytes"])
        oldsize = int(base["wire_bytes"])
        now = int(new["wire_bytes"])
        q5 = int(new["brotli_q5_bytes"])
        q11 = int(new["brotli_q11_bytes"])
        if size <= 0 or min(oldsize, now, q5, q11) <= 0:
            raise ValueError("nonpositive measured size")
        if size != int(base["input_bytes"]):
            raise ValueError("source size mismatch")
        for ref in ("brotli_q5_bytes", "brotli_q11_bytes"):
            if int(base[ref]) != int(new[ref]):
                raise ValueError("reference bytes differ within runner")
        if now > oldsize:
            raise ValueError("retained I13 candidate regressed in complete bytes")
        if sum(int(new[k]) for k in ("raw_blocks", "model_blocks", "column_blocks")) != (size+4095)//4096:
            raise ValueError("block totals incorrect")
        lanes = int(new["tiled_lanes"])
        cols = int(new["modeled_columns"])
        if not (0 <= lanes <= cols):
            raise ValueError("invalid tile mode count")
        for key in ("encode_MBps", "decode_MBps", "brotli_q5_decode_MBps", "brotli_q11_decode_MBps"):
            number(new, key)
        report["records"].append({
            "file": new["file"], "source_bytes": size, "i13_bytes": oldsize,
            "i15t_bytes": now, "brotli_q5_bytes": q5, "brotli_q11_bytes": q11,
            "column_blocks": int(new["column_blocks"]), "modeled_columns": cols,
            "tiled_lanes": lanes,
            "i15t_encode_MBps": number(new, "encode_MBps"),
            "i15t_decode_MBps": number(new, "decode_MBps"),
            "brotli_q5_decode_MBps": number(new, "brotli_q5_decode_MBps"),
            "brotli_q11_decode_MBps": number(new, "brotli_q11_decode_MBps")
        })
    arith, series, _, _, repeated, _, _ = report["records"]
    gates = {
        "H1_tiles_selected_on_timeseries": series["tiled_lanes"] > 0,
        "H2_q11_bytes": series["i15t_bytes"] < series["brotli_q11_bytes"],
        "H2b_q5_bytes": series["i15t_bytes"] < series["brotli_q5_bytes"],
        "H3_decode_above_q5": series["i15t_decode_MBps"] > series["brotli_q5_decode_MBps"],
        "H3_decode_above_q11": series["i15t_decode_MBps"] > series["brotli_q11_decode_MBps"],
        "H4_arithmetic_nonregression": arith["i15t_bytes"] <= 62256,
        "H4_repeat_jsonl_nonregression": repeated["i15t_bytes"] <= 916835
    }
    # Absence of old options violates the code contract, not just an efficacy gate.
    if not gates["H4_arithmetic_nonregression"] or not gates["H4_repeat_jsonl_nonregression"]:
        raise ValueError("frozen old-option byte invariant broken")
    if not gates["H1_tiles_selected_on_timeseries"]:
        decision = "KILL-TILED16-NO-SELECTION"
    elif not gates["H2_q11_bytes"]:
        decision = "SELECTED-NO-Q11-BYTE-WIN"
    elif not (gates["H3_decode_above_q5"] and gates["H3_decode_above_q11"]):
        decision = "RATIO-ONLY-TILED-DISCOVERY"
    elif gates["H2b_q5_bytes"]:
        decision = "SPECIALIZED-Q5-BYTE-DISCOVERY"
    else:
        decision = "SPECIALIZED-Q11-TRADEOFF-DISCOVERY"
    report["gates"], report["decision"] = gates, decision
    report["delta_bytes_vs_i13_timeseries"] = series["i15t_bytes"]-series["i13_bytes"]
    report["delta_bytes_vs_q11_timeseries"] = series["i15t_bytes"]-series["brotli_q11_bytes"]
    report["delta_bytes_vs_q5_timeseries"] = series["i15t_bytes"]-series["brotli_q5_bytes"]
    with open(sys.argv[3], "w", encoding="utf-8") as stream:
        json.dump(report, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(json.dumps(report, indent=2, sort_keys=True))

if __name__ == "__main__":
    main()
