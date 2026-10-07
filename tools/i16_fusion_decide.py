#!/usr/bin/env python3
"""Frozen I16 paired three-arm source-attested discovery adjudicator."""
import csv
import json
import math
import sys

NAMES = ["synth-arith.bin", "synth-timeseries.bin", "synth-jitter.bin",
         "random.bin", "generated.repeat.jsonl", "generated.jsonl", "src.cpp"]

def read(path):
    with open(path, encoding="utf-8", newline="") as fp:
        rows = list(csv.DictReader(fp, delimiter="\t"))
    if [row.get("file") for row in rows] != NAMES:
        raise ValueError("missing/duplicated/reordered frozen discovery members")
    return rows

def integer(row,key):
    n=int(row[key])
    if n<0: raise ValueError("negative field "+key)
    return n

def positive(row,key):
    v=float(row[key])
    if not math.isfinite(v) or v<=0: raise ValueError("invalid measurement "+key)
    return v

def main():
    field, tiled, fused = [read(p) for p in sys.argv[1:4]]
    report={"schema":"anvil.i16.paired-discovery/v1","role":"consumed-discovery",
            "class_a_crossing":False,"promotion_authorized":False,"files":[]}
    for a,b,c in zip(field,tiled,fused):
        size=integer(c,"input_bytes")
        if not size or any(integer(row,"input_bytes")!=size for row in (a,b)):
            raise ValueError("inconsistent input size")
        for ref in ("brotli_q5_bytes","brotli_q11_bytes"):
            if not integer(c,ref) or len({integer(row,ref) for row in (a,b,c)})!=1:
                raise ValueError("unpaired reference size "+ref)
        ca,cb,cc=[integer(row,"wire_bytes") for row in (a,b,c)]
        if not cc or cc>ca or cc>cb:
            raise ValueError("I16 violates retained candidate byte nonregression: "+c["file"])
        blocks=(size+4095)//4096
        if sum(integer(c,k) for k in ("raw_blocks","model_blocks","column_blocks"))!=blocks:
            raise ValueError("invalid output block totals")
        cols=integer(c,"modeled_columns")
        diff=integer(c,"column_diff_models")
        afftile=integer(c,"tiled_affine_lanes")
        difftile=integer(c,"tiled_diff_lanes")
        if diff>cols or difftile>diff or afftile+diff>cols:
            raise ValueError("invalid tiled/model lane counts")
        record={"file":c["file"],"source_bytes":size,"i15_field_bytes":ca,
                "i15t_bytes":cb,"i16_bytes":cc,"q5_bytes":integer(c,"brotli_q5_bytes"),
                "q11_bytes":integer(c,"brotli_q11_bytes"),
                "i16_column_blocks":integer(c,"column_blocks"),"i16_modeled_lanes":cols,
                "i16_diff_lanes":diff,"i16_tiled_affine_lanes":afftile,
                "i16_tiled_diff_lanes":difftile,
                "i15_field_decode_MBps":positive(a,"decode_MBps"),
                "i15t_decode_MBps":positive(b,"decode_MBps"),
                "i16_decode_MBps":positive(c,"decode_MBps"),
                "i16_encode_MBps":positive(c,"encode_MBps"),
                "i15_field_encode_MBps":positive(a,"encode_MBps"),
                "i15t_encode_MBps":positive(b,"encode_MBps"),
                "q5_decode_MBps":positive(c,"brotli_q5_decode_MBps"),
                "q11_decode_MBps":positive(c,"brotli_q11_decode_MBps"),
                "q5_encode_MBps":positive(c,"brotli_q5_encode_MBps"),
                "q11_encode_MBps":positive(c,"brotli_q11_encode_MBps")}
        report["files"].append(record)
    arith,series,_,_,repeat,_,_=report["files"]
    gates={
        "H1_local_tiled_diff_selected":series["i16_tiled_diff_lanes"]>0,
        "H2_smaller_than_both_prior_pilots":
          series["i16_bytes"]<min(series["i15_field_bytes"],series["i15t_bytes"]),
        "H2q11":series["i16_bytes"]<series["q11_bytes"],
        "H2q5":series["i16_bytes"]<series["q5_bytes"],
        "H3_faster_decode_than_q5_and_q11":
          series["i16_decode_MBps"]>max(series["q5_decode_MBps"],series["q11_decode_MBps"]),
        "H4_nonregression_all_seven":True
    }
    if not gates["H1_local_tiled_diff_selected"]: verdict="NO-DIFF-TILE-SELECTION"
    elif not gates["H2_smaller_than_both_prior_pilots"]: verdict="SELECTED-NO-INCREMENT"
    elif not gates["H2q11"]: verdict="INCREMENT-NO-Q11-CROSSING"
    elif not gates["H2q5"]: verdict="SPECIALIZED-Q11-ONLY"
    elif not gates["H3_faster_decode_than_q5_and_q11"]: verdict="SPECIALIZED-Q5-RATIO-ONLY"
    else: verdict="SPECIALIZED-Q5-DECODER-DISCOVERY"
    report["gates"]=gates
    report["verdict"]=verdict
    report["timeseries_delta_vs_i15_field"]=series["i16_bytes"]-series["i15_field_bytes"]
    report["timeseries_delta_vs_i15t"]=series["i16_bytes"]-series["i15t_bytes"]
    report["timeseries_delta_vs_q5"]=series["i16_bytes"]-series["q5_bytes"]
    report["timeseries_delta_vs_q11"]=series["i16_bytes"]-series["q11_bytes"]
    report["note"]="Previously consumed synthetic fixtures; no general-purpose frontier certification."
    with open(sys.argv[4],"w",encoding="utf-8") as fp:
        json.dump(report,fp,indent=2,sort_keys=True)
        fp.write("\n")
    print(json.dumps(report,indent=2,sort_keys=True))

if __name__=="__main__":main()
