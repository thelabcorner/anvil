#!/usr/bin/env python3
"""Frozen I14 discovery gate: source-paired, complete-wire numbers only."""
import csv
import json
import math
import sys

def load(path):
    with open(path, encoding="utf-8", newline="") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))

def main():
    old, new = load(sys.argv[1]), load(sys.argv[2])
    names = [
        "synth-arith.bin", "synth-timeseries.bin", "synth-jitter.bin",
        "random.bin", "generated.repeat.jsonl", "generated.jsonl", "src.cpp"
    ]
    if [r["file"] for r in old] != names or [r["file"] for r in new] != names:
        raise SystemExit("I14 discovery population mismatch")
    rows = []
    for a, b in zip(old, new):
        size = int(b["input_bytes"])
        wire = int(b["wire_bytes"])
        i12 = int(a["wire_bytes"])
        brotli = int(b["brotli_q11_bytes"])
        if size != int(a["input_bytes"]) or brotli != int(a["brotli_q11_bytes"]):
            raise SystemExit("paired control/input mismatch")
        if int(b["raw_blocks"]) + int(b["model_blocks"]) != (size + 4095)//4096:
            raise SystemExit("invalid block count")
        if not (0 <= int(b["diff_blocks"]) <= int(b["model_blocks"])):
            raise SystemExit("invalid DIFF accounting")
        if wire > i12:
            raise SystemExit("I14 full-wire regression against retained I12 candidates")
        decode = float(b["decode_MBps"])
        refdecode = float(b["brotli_q11_decode_MBps"])
        olddecode = float(a["decode_MBps"])
        encode = float(b["encode_MBps"])
        if min(size, wire, i12, brotli) <= 0 or not all(
            math.isfinite(v) and v > 0 for v in (decode, refdecode, olddecode, encode)
        ):
            raise SystemExit("invalid measurements")
        rows.append({
            "file": b["file"], "input_bytes": size, "i14_wire_bytes": wire,
            "i12_wire_bytes": i12, "brotli_q11_bytes": brotli,
            "i14_diff_blocks": int(b["diff_blocks"]), "i14_model_blocks": int(b["model_blocks"]),
            "i14_decode_MBps": decode, "i12_decode_MBps": olddecode,
            "brotli_q11_decode_MBps": refdecode, "i14_encode_MBps": encode
        })
    ar = rows[0]
    gates = {
        "H1_diff_selected": ar["i14_diff_blocks"] > 0,
        "H2_improves_i12_bytes": ar["i14_wire_bytes"] < ar["i12_wire_bytes"],
        "H3_beats_brotli_q11_bytes": ar["i14_wire_bytes"] < ar["brotli_q11_bytes"],
        "H4_beats_brotli_q11_decode": ar["i14_decode_MBps"] > ar["brotli_q11_decode_MBps"]
    }
    ruling = (
        "KILL-DIFF" if not gates["H1_diff_selected"]
        else "NO-INCREMENTAL-BYTE-WIN" if not gates["H2_improves_i12_bytes"]
        else "SPECIALIZED-TRADEOFF" if not (gates["H3_beats_brotli_q11_bytes"] and gates["H4_beats_brotli_q11_decode"])
        else "SPECIALIZED-DISCOVERY-ADVANCE"
    )
    report = {"schema": "anvil.i14.discovery/v1", "role": "discovery",
              "frontier_crossing": False, "promotion_authorized": False,
              "ruling": ruling, "gates": gates, "records": rows}
    with open(sys.argv[3], "w", encoding="utf-8") as stream:
        json.dump(report, stream, sort_keys=True, indent=2)
        stream.write("\n")
    print(json.dumps(report, sort_keys=True, indent=2))

if __name__ == "__main__":
    main()
