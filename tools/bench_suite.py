#!/usr/bin/env python3
from __future__ import annotations
import argparse, csv, io, subprocess
from pathlib import Path

def run_one(exe:Path,path:Path,reps:int):
    p=subprocess.run([exe,path,str(reps)],check=True,text=True,capture_output=True)
    lines=p.stdout.strip().splitlines(); input_bytes=int(lines[0].split(',')[1]); rows=list(csv.DictReader(io.StringIO('\n'.join(lines[1:]))))
    for r in rows:
        r['file']=str(path); r['input_bytes']=input_bytes
        for k in ('compressed_bytes',):r[k]=int(r[k])
        for k in ('ratio','encode_MBps','decode_MBps'):r[k]=float(r[k])
    return rows

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('files',nargs='+'); ap.add_argument('--bench',default=str(Path(__file__).parents[1]/'bench_native')); ap.add_argument('--reps',type=int,default=3); ap.add_argument('--out',default='benchmark-suite.csv'); a=ap.parse_args()
    rows=[]
    for f in a.files: rows+=run_one(Path(a.bench),Path(f),a.reps)
    fields=['file','input_bytes','codec','compressed_bytes','ratio','encode_MBps','decode_MBps','roundtrip']
    with open(a.out,'w',newline='') as fp:
        w=csv.DictWriter(fp,fieldnames=fields);w.writeheader();w.writerows(rows)
    by={}
    for r in rows: by.setdefault(r['codec'],[]).append(r)
    agg=[]
    for codec,rs in by.items():
        ib=sum(r['input_bytes'] for r in rs); cb=sum(r['compressed_bytes'] for r in rs)
        et=sum(r['input_bytes']/1e6/max(r['encode_MBps'],1e-12) for r in rs); dt=sum(r['input_bytes']/1e6/max(r['decode_MBps'],1e-12) for r in rs)
        agg.append({'codec':codec,'total_ratio':f'{cb/ib:.6f}','aggregate_encode_MBps':f'{ib/1e6/et:.3f}','aggregate_decode_MBps':f'{ib/1e6/dt:.3f}','roundtrip':"OK" if all(r["roundtrip"]=="OK" for r in rs) else "FAIL"})
    out_sum=Path(a.out).with_name('benchmark-summary.csv')
    with open(out_sum,'w',newline='') as fp:
        w=csv.DictWriter(fp,fieldnames=['codec','total_ratio','aggregate_encode_MBps','aggregate_decode_MBps','roundtrip']);w.writeheader();w.writerows(agg)
    print(f'# wrote {a.out} and {out_sum}')
    print('codec,total_ratio,aggregate_encode_MBps,aggregate_decode_MBps,roundtrip')
    for r in agg: print(f'{r["codec"]},{r["total_ratio"]},{r["aggregate_encode_MBps"]},{r["aggregate_decode_MBps"]},{r["roundtrip"]}')

if __name__=='__main__':main()
