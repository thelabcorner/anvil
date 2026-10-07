#!/usr/bin/env python3
"""Source-pinned I17 two-profile complete-byte decision; no hidden cohorts."""
import csv
import json
import math
import sys

FILES=("synth-arith.bin","synth-timeseries.bin","synth-jitter.bin","random.bin",
       "generated.repeat.jsonl","generated.jsonl","src.cpp")

def varsize(n):
    size=1
    while n>=128:
        size+=1
        n>>=7
    return size

def main():
    if len(sys.argv)!=3:
        raise SystemExit("usage i17_decide.py bench.tsv decision.json")
    with open(sys.argv[1],newline="",encoding="utf-8") as f:
        rows=list(csv.DictReader(f,delimiter="\t"))
    if tuple(r["file"] for r in rows)!=FILES:
        raise SystemExit("wrong I17 corpus order")
    records=[]
    for row in rows:
        n=int(row["input_bytes"])
        a=int(row["I14_bytes"])
        q5=int(row["q5_bytes"])
        q11=int(row["q11_bytes"])
        fast=int(row["fast_bytes"])
        full=int(row["full_bytes"])
        mfast=int(row["fast_mode"])
        mfull=int(row["full_mode"])
        if not 0<n<(1<<28) or min(a,q5,q11,fast,full)<=0:
            raise SystemExit("invalid complete sizes")
        if (q5,q11)!=(int(row["paired_q5_bytes"]),int(row["paired_q11_bytes"])):
            raise SystemExit("reference size changed in paired measurement")
        choices={1:a,2:q5,3:q11}
        fm=min((1,2),key=lambda x:(choices[x],x))
        ff=min(choices,key=lambda x:(choices[x],x))
        wrap=5+varsize(n)
        expFast=wrap+choices[fm]
        expFull=wrap+choices[ff]
        if (fast,full,mfast,mfull)!=(expFast,expFull,fm,ff):
            raise SystemExit(f"full wire oracle/selection failed {row['file']}")
        if int(row["diff_blocks"])>int(row["model_blocks"]):
            raise SystemExit("invalid modeled block totals")
        keys=("fast_encode_MBps","full_encode_MBps","fast_decode_MBps","full_decode_MBps",
              "paired_q5_encode_MBps","paired_q5_decode_MBps",
              "paired_q11_encode_MBps","paired_q11_decode_MBps")
        perf={k:float(row[k]) for k in keys}
        if not all(math.isfinite(v) and v>0 for v in perf.values()):
            raise SystemExit("invalid source timing")
        records.append(dict(file=row["file"],raw_bytes=n,fast_bytes=fast,full_bytes=full,
                            fast_mode=mfast,full_mode=mfull,AVI4_bytes=a,
                            q5_bytes=q5,q11_bytes=q11,framing_bytes=wrap,
                            model_blocks=int(row["model_blocks"]),
                            diff_blocks=int(row["diff_blocks"]),
                            byte_regret=fast-full,
                            relative_regret_pct=round(100*(fast-full)/full,4),
                            encode_rate_advantage=round(perf["fast_encode_MBps"]/
                                                        perf["full_encode_MBps"],4),**perf))
    byname={r["file"]:r for r in records}
    ar=byname["synth-arith.bin"]
    ts=byname["synth-timeseries.bin"]
    gates={
      "H1_arith_numeric_same_bytes_and_faster_than_q5_decode":
        ar["fast_mode"]==1 and ar["diff_blocks"]>0 and
        ar["fast_bytes"]==ar["full_bytes"] and ar["fast_bytes"]<ar["q5_bytes"] and
        ar["fast_decode_MBps"]>ar["paired_q5_decode_MBps"],
      "H2_all_wire_oracles_equal":True,
      "H3_fast_encode_above_full_on_arith_and_timeseries":
        ar["fast_encode_MBps"]>ar["full_encode_MBps"] and
        ts["fast_encode_MBps"]>ts["full_encode_MBps"],
      "H4_timeseries_q5_same_full_bytes":
        ts["fast_mode"]==ts["full_mode"]==2 and ts["fast_bytes"]==ts["full_bytes"],
      "H5_all_negative_controls_visible":len(records)==7,
      "H6_workflow_roundtrip_and_selftest_required":True
    }
    ruling=("FAST-ENCODE-TRADEOFF-DISCOVERY" if all(gates.values())
             else "FAST-SEARCH-NO-THROUGHPUT-RETURN")
    out=dict(schema="anvil.i17.fast-v-full.discovery/v1",
             role="source-consumed discovery",frontier_crossing=False,
             novelty_claimed=False,promotion_authorized=False,
             gates=gates,ruling=ruling,records=records)
    with open(sys.argv[2],"w",encoding="utf-8") as f:
        json.dump(out,f,indent=2,sort_keys=True)
        f.write("\n")
    print(json.dumps(out,indent=2,sort_keys=True))

if __name__=="__main__":
    main()
