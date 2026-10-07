#!/usr/bin/env python3
"""Frozen I16 same-run readout. No CPU-heavy benchmark here; Actions owns that."""
import csv
import json
import math
import sys

NAMES = ("synth-arith.bin", "synth-timeseries.bin", "synth-jitter.bin",
         "random.bin", "generated.repeat.jsonl", "generated.jsonl", "src.cpp")

def rows(path):
    with open(path, encoding="utf-8", newline="") as f:
        result = list(csv.DictReader(f, delimiter="\t"))
    if tuple(x["file"] for x in result) != NAMES:
        raise ValueError("wrong frozen I16 cohort")
    return result

def main():
    old, new = rows(sys.argv[1]), rows(sys.argv[2])
    results=[]
    for a,b in zip(old,new):
        n=int(b["input_bytes"])
        wire=int(b["wire_bytes"])
        prior=int(a["wire_bytes"])
        q5=int(b["brotli_q5_bytes"])
        q11=int(b["brotli_q11_bytes"])
        if n!=int(a["input_bytes"]) or q5!=int(a["brotli_q5_bytes"]) or q11!=int(a["brotli_q11_bytes"]):
            raise ValueError("unpaired reference/input data")
        models=int(b["model_blocks"])
        raws=int(b["raw_blocks"])
        cols=int(b["column_blocks"])
        frames=int(b["framed_blocks"])
        frameDiff=int(b["framed_diff_fields"])
        if raws+models+cols+frames!=(n+4095)//4096:
            raise ValueError("framed block accounting wrong")
        if frameDiff<frames or (frames==0 and frameDiff!=0):
            raise ValueError("framed field accounting wrong")
        if wire>prior:
            raise ValueError("I16 regressed retained I15 candidate bytes")
        rates=[float(b[x]) for x in ("encode_MBps","decode_MBps","brotli_q5_decode_MBps","brotli_q11_decode_MBps")]
        rates += [float(a[x]) for x in ("decode_MBps","encode_MBps")]
        if min(n,wire,prior,q5,q11)<=0 or not all(math.isfinite(r) and r>0 for r in rates):
            raise ValueError("nonfinite/invalid sizes and throughputs")
        results.append(dict(file=b["file"],input_bytes=n,
            i15_wire_bytes=prior,i16_wire_bytes=wire,brotli_q5_wire_bytes=q5,brotli_q11_wire_bytes=q11,
            i16_framed_blocks=frames,i16_framed_fields=frameDiff,
            i16_global_diff_blocks=int(b["diff_blocks"]),i16_column_diff_fields=int(b["column_diff_models"]),
            i15_encode_MBps=rates[5],i16_encode_MBps=rates[0],
            i15_decode_MBps=rates[4],i16_decode_MBps=rates[1],
            brotli_q5_decode_MBps=rates[2],brotli_q11_decode_MBps=rates[3]))
    ar=results[0]
    ts=results[1]
    gates={
        "H1_timeseries_frams_selected":ts["i16_framed_blocks"]>0 and ts["i16_framed_fields"]>0,
        "H2a_timeseries_smaller_than_i15":ts["i16_wire_bytes"]<ts["i15_wire_bytes"],
        "H2b_timeseries_smaller_than_brotli_q5":ts["i16_wire_bytes"]<ts["brotli_q5_wire_bytes"],
        "H3_timeseries_decode_faster_than_brotli_q5":ts["i16_decode_MBps"]>ts["brotli_q5_decode_MBps"],
        "H4_arithmetic_nonregression":ar["i16_wire_bytes"]<=33196,
    }
    if not gates["H4_arithmetic_nonregression"]:
        raise ValueError("I16 regressed known arithmetic source")
    if not gates["H1_timeseries_frams_selected"]:
        ruling="KILL-FRAME"
    elif not gates["H2a_timeseries_smaller_than_i15"]:
        ruling="STRUCTURE-SELECTED-NO-INCREMENT"
    elif not gates["H2b_timeseries_smaller_than_brotli_q5"]:
        ruling="FRAMED-BYTE-ADVANCE-Q5-ADVERSE"
    elif not gates["H3_timeseries_decode_faster_than_brotli_q5"]:
        ruling="FRAMED-RATIO-ONLY-DISCOVERY"
    else:
        ruling="FRAMED-SPECIALIZED-DISCOVERY-CANDIDATE"
    obj=dict(schema="anvil.i16.discovery/v1",role="discovery",
             frontier_crossing=False,promotion_authorized=False,
             gates=gates,ruling=ruling,records=results)
    with open(sys.argv[3],"w",encoding="utf-8") as f:
        json.dump(obj,f,indent=2,sort_keys=True);f.write("\n")
    print(json.dumps(obj,indent=2,sort_keys=True))

if __name__=="__main__":
    main()
