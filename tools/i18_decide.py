#!/usr/bin/env python3
"""I18 frozen seven-input *consumed-discovery* gate adjudicator; not a frontier grader."""
import csv
import json
import sys
from pathlib import Path

NAMES = [
    "synth-arith.bin",
    "synth-timeseries.bin",
    "synth-jitter.bin",
    "random.bin",
    "generated.repeat.jsonl",
    "generated.jsonl",
    "src.cpp",
]

def decide(source: Path) -> dict:
    with source.open("r", encoding="utf-8", newline="") as f:
        rows = list(csv.DictReader(f, delimiter="\t"))
    if [r["file"] for r in rows] != NAMES:
        raise ValueError("discovery population or order changed")
    # Strict numeric validation: incomplete TSV and missing/NaN numbers invalidate.
    nums = [
        "input_bytes","budget_bytes","fast_bytes","full_bytes","q5_bytes","q11_bytes",
        "budget_mode","probed","regular","skipped_by_ratio","math_candidate_calls",
        "avi4_bytes","avi6_bytes","budget_encode_MBps","fast_encode_MBps",
        "full_encode_MBps","q5_encode_MBps","budget_decode_MBps","fast_decode_MBps",
        "full_decode_MBps","q5_decode_MBps","q11_decode_MBps"
    ]
    import math
    for r in rows:
        for k in nums:
            try:
                v = float(r[k])
            except (ValueError, KeyError) as exc:
                raise ValueError(f"{r['file']} invalid {k}") from exc
            if not math.isfinite(v) or v < 0:
                raise ValueError(f"{r['file']} invalid {k}: {v}")
        if int(r["budget_mode"]) not in (0,1,2,4):
            raise ValueError("unknown budget mode")
        if int(r["budget_bytes"]) < 5:
            raise ValueError("invalid container size")
        if int(r["budget_bytes"]) > int(r["input_bytes"])+16:
            raise ValueError("raw fallback exceeded")
    indexed={r["file"]:r for r in rows}
    a=indexed["synth-arith.bin"]
    t=indexed["synth-timeseries.bin"]
    negatives=[indexed[k] for k in NAMES[2:]]
    asfloat=lambda r,k:float(r[k])
    skip_count=sum(int(r["math_candidate_calls"])==0 for r in negatives)
    gates={
        "H1_arithmetic_avi6_selection_and_bytes": (
            int(a["budget_mode"])==4 and int(a["budget_bytes"])<33204
        ),
        "H2_timeseries_q5_and_no_byte_regression": (
            int(t["budget_mode"])==2 and int(t["budget_bytes"])<=120602
        ),
        "H3_negative_controls_no_math_in_majority": skip_count>=4,
        "H4_budget_encode_faster_than_full_both_numeric": (
            asfloat(a,"budget_encode_MBps")>asfloat(a,"full_encode_MBps") and
            asfloat(t,"budget_encode_MBps")>asfloat(t,"full_encode_MBps")
        ),
        "H5_all_byte_regrets_accounted": all(
            int(r["budget_bytes"])>=0 and int(r["q5_bytes"])>=0 and
            int(r["q11_bytes"])>=0 for r in rows
        ),
        "H6_arithmetic_avi6_decode_faster_than_q5_q11": (
            int(a["budget_mode"])==4 and
            asfloat(a,"budget_decode_MBps")>asfloat(a,"q5_decode_MBps") and
            asfloat(a,"budget_decode_MBps")>asfloat(a,"q11_decode_MBps")
        )
    }
    records=[]
    for r in rows:
        records.append({
            "file":r["file"],
            "input_bytes":int(r["input_bytes"]),
            "budget_bytes":int(r["budget_bytes"]),
            "fast_bytes":int(r["fast_bytes"]),
            "full_bytes":int(r["full_bytes"]),
            "q5_bytes":int(r["q5_bytes"]),
            "q11_bytes":int(r["q11_bytes"]),
            "budget_mode":int(r["budget_mode"]),
            "skipped_by_ratio":bool(int(r["skipped_by_ratio"])),
            "math_candidate_calls":int(r["math_candidate_calls"]),
            "byte_regret_vs_full":int(r["budget_bytes"])-int(r["full_bytes"]),
            "byte_delta_vs_fast":int(r["budget_bytes"])-int(r["fast_bytes"]),
            "byte_delta_vs_q5":int(r["budget_bytes"])-int(r["q5_bytes"]),
            "byte_delta_vs_q11":int(r["budget_bytes"])-int(r["q11_bytes"]),
            "encode_MBps":{k.replace("_encode_MBps",""):float(r[k]) for k in (
                "budget_encode_MBps","fast_encode_MBps",
                "full_encode_MBps","q5_encode_MBps"
            )},
            "decode_MBps":{k.replace("_decode_MBps",""):float(r[k]) for k in (
                "budget_decode_MBps","fast_decode_MBps","full_decode_MBps",
                "q5_decode_MBps","q11_decode_MBps"
            )}
        })
    return {
        "schema":"anvil.i18.consumed-discovery.v1",
        "source_tsv":str(source.name),
        "consumed_discovery":True,
        "promotion_authorized":False,
        "general_pareto_crossing":False,
        "novelty_claimed":False,
        "gates":gates,
        "negative_skips":skip_count,
        "records":records,
        "aggregate_bytes":{
            k:sum(int(r[k+"_bytes"]) for r in rows)
            for k in ("budget","fast","full","q5","q11")
        },
        "verdict":(
            "BUDGETED-SPECIALIZED-DISCOVERY" if all(gates.values()) else
            "EXPERIMENTAL-GATES-FAILED"
        )
    }

def main(argv: list[str]) -> None:
    if len(argv)!=3:
        raise SystemExit("usage: i18_decide.py BENCHMARK.tsv DECISION.json")
    decision=decide(Path(argv[1]))
    Path(argv[2]).write_text(json.dumps(decision,indent=2,sort_keys=True)+"\n",encoding="utf-8")
    print(json.dumps({"verdict":decision["verdict"],"gates":decision["gates"]},sort_keys=True))

if __name__=="__main__":
    main(sys.argv)
