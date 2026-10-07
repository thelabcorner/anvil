#!/usr/bin/env python3
"""Frozen I16 complete-wire, same-job hybrid selection adjudication."""
import csv
import json
import math
import sys

FILES = (
    "synth-arith.bin", "synth-timeseries.bin", "synth-jitter.bin",
    "random.bin", "generated.repeat.jsonl", "generated.jsonl", "src.cpp",
)

def vsize(value):
    count = 1
    while value >= 128:
        count += 1
        value >>= 7
    return count

def main():
    if len(sys.argv) != 3:
        raise SystemExit("Usage: i16_decide.py BENCH.tsv DECISION.json")
    with open(sys.argv[1], encoding="utf-8", newline="") as f:
        measured = list(csv.DictReader(f, delimiter="\t"))
    if tuple(r["file"] for r in measured) != FILES:
        raise SystemExit("I16 discovery population/order mismatch")
    report = {
        "schema": "anvil.i16.hybrid.discovery/v1",
        "role": "adopted_fallback_scoping_only",
        "promotion_authorized": False,
        "novelty_claimed": False,
        "frontier_crossing": False,
        "records": [],
    }
    for row in measured:
        n = int(row["input_bytes"])
        wire = int(row["wire_bytes"])
        mode = int(row["selected_mode"])
        i14 = int(row["AVI4_bytes"])
        q5 = int(row["Brotli_q5_bytes"])
        q11 = int(row["Brotli_q11_bytes"])
        refq5 = int(row["paired_Brotli_q5_bytes"])
        refq11 = int(row["paired_Brotli_q11_bytes"])
        if not 0 <= n <= (1 << 28) or min(wire, i14) <= 0:
            raise SystemExit("invalid lengths")
        if (q5,q11)!=(refq5,refq11):
            raise SystemExit("reference deterministic bytes disagree in same job")
        if n and min(q5, q11) <= 0:
            raise SystemExit("missing reference source")
        cost = {1:i14}
        if n:
            cost.update({2:q5,3:q11})
        winning = min(cost, key=lambda m:(cost[m], m))
        full_expected = cost[winning] + 5 + vsize(n)
        if mode != winning or wire != full_expected:
            raise SystemExit(f"size-oracle failure {row['file']}: {mode=} {wire=} {winning=} {full_expected=}")
        if int(row["I14_diff_blocks"])>int(row["I14_model_blocks"]):
            raise SystemExit("invalid model stats")
        perf_names=("encode_MBps","decode_MBps",
                    "paired_Brotli_q5_encode_MBps","paired_Brotli_q5_decode_MBps",
                    "paired_Brotli_q11_encode_MBps","paired_Brotli_q11_decode_MBps")
        measured_speeds={k:float(row[k]) for k in perf_names}
        if not all(math.isfinite(x) and x > 0 for x in measured_speeds.values()):
            raise SystemExit("invalid complete-file throughput")
        report["records"].append({
            "file":row["file"], "raw_bytes":n,
            "hybrid_full_bytes":wire, "selected_mode":mode,
            "fixed_envelope_overhead":5+vsize(n),
            "frozen_avi4_bytes":i14, "brotli_q5_bytes":q5,
            "brotli_q11_bytes":q11,
            "modeled_blocks_if_I14":int(row["I14_model_blocks"]),
            "diff_blocks_if_I14":int(row["I14_diff_blocks"]),
            **measured_speeds,
        })
    byname={r["file"]:r for r in report["records"]}
    a=byname["synth-arith.bin"]
    t=byname["synth-timeseries.bin"]
    repeat=byname["generated.repeat.jsonl"]
    gates={
        "H1_model_selected_arithmetic":a["selected_mode"]==1 and a["diff_blocks_if_I14"]>0,
        "H2_dictionary_selected_repeated_jsonl":repeat["selected_mode"] in (2,3),
        "H3_q5_selected_timeseries":t["selected_mode"]==2,
        "H4_arithmetic_byte_win_vs_q11":a["hybrid_full_bytes"]<a["brotli_q11_bytes"],
        "H5_arithmetic_decode_scout_above_q11":
           a["decode_MBps"]>a["paired_Brotli_q11_decode_MBps"],
        "H6_complete_size_oracle_valid":True,
    }
    report["gates"]=gates
    report["ruling"]=(
        "INVALID_ENVELOPE_SELECTION" if not all((gates["H1_model_selected_arithmetic"],
                                                gates["H2_dictionary_selected_repeated_jsonl"],
                                                gates["H6_complete_size_oracle_valid"]))
        else "HYBRID_ARCHITECTURE_VALIDATED_DISCOVERY" if all(gates.values())
        else "HYBRID_ARCHITECTURE_PARTIAL_DISCOVERY"
    )
    with open(sys.argv[2], "w", encoding="utf-8") as f:
        json.dump(report,f,sort_keys=True,indent=2)
        f.write("\n")
    print(json.dumps(report,sort_keys=True,indent=2))
    if report["ruling"]=="INVALID_ENVELOPE_SELECTION":
        raise SystemExit("hybrid invariant gate failed")

if __name__ == "__main__":
    main()
