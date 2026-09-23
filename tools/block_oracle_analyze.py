#!/usr/bin/env python3
"""
block_oracle_analyze.py — E4 oracle + warmup/regret tables from block_oracle.py.

Reads scratch/block-oracle.csv (per-block rows) and companion -wholes.csv.
Computes per (file, block_size):
  * oracle_coded: sum over blocks min(framed brotli, framed bwt)   [route-oracle]
  * oracle_any : min(.., raw per block)                            [honest]
  * wholefile_framed: winner whole-file + rev-2 framing
  * wf_regret  : oracle_coded - wholefile_framed   (NEGATIVE = block wins)
  * warmup_bwt : sum framed-bwt blocks - whole-file framed-bwt (same backend)
  * warmup_br  : sum framed-br  blocks - whole-file framed-br
  * vs_xz      : oracle_coded - 48,456,100
"""
import argparse
import csv
import math
import os
import statistics as st


def uvar_len(x: int) -> int:
    n = 1
    x >>= 7
    while x:
        n += 1
        x >>= 7
    return n


def coded_block_total(blen: int, coded: int) -> int:
    payload_len = 2 + uvar_len(blen) + coded
    return 1 + uvar_len(payload_len) + 4 + payload_len


def raw_block_total(blen: int) -> int:
    return 1 + uvar_len(blen) + 4 + blen


def summarize(rows, wf):
    wb, wbt = wf
    n_brotli = n_bwt = n_raw = 0
    margins = []
    oracle_coded = 0
    oracle_any = 0
    sum_brotli_fb = 0   # framed brotli sum
    sum_bwt_fb = 0      # framed bwt sum
    for r in rows:
        blen = int(r["blen"]); raw = int(r["bwt_raw"])
        if raw:
            br = int(r["brotli_bytes"]); cb = coded_block_total(blen, br)
            rb = raw_block_total(blen); n_raw += 1
            margins.append(abs(rb - cb))
            oracle_coded += cb; oracle_any += min(cb, rb)
            sum_brotli_fb += cb
            continue
        br = int(r["brotli_bytes"]); bwt = int(r["bwt_coded_bytes"])
        cbb = coded_block_total(blen, br); cbbw = coded_block_total(blen, bwt)
        sum_brotli_fb += cbb; sum_bwt_fb += cbbw
        if br < bwt:
            n_brotli += 1; margin = bwt - br; ob = cbb
        else:
            n_bwt += 1; margin = br - bwt; ob = cbbw
        rb = raw_block_total(blen); margins.append(margin)
        oracle_coded += ob; oracle_any += min(ob, rb)
    tot = max(1, n_brotli + n_bwt + n_raw)
    med = st.median(margins) if margins else 0
    p95 = sorted(margins)[min(len(margins) - 1, int(math.ceil(0.95 * len(margins))) - 1)] if margins else 0
    # whole-file framed for the SAME backend (so warmup compares like-for-like)
    wf_b = coded_block_total(len(rows) and int(rows[0]["blen"]) * 0 + (wb if wb < wbt else wbt),
                             min(wb, wbt))
    # warmup per backend: how much the independent blocks of that backend lose
    if wbt <= wb:
        warmup = sum_bwt_fb - coded_block_total(len(rows) and int(rows[0]["blen"]) * 0 + wbt, wbt)
    else:
        warmup = sum_brotli_fb - coded_block_total(len(rows) and int(rows[0]["blen"]) * 0 + wb, wb)
    return dict(blocks=len(rows), n_brotli=n_brotli, n_bwt=n_bwt, n_raw=n_raw,
                frac_bwt=n_bwt / tot, frac_brotli=n_brotli / tot, frac_raw=n_raw / tot,
                median_margin=int(med), p95_margin=int(p95),
                oracle_coded=oracle_coded, oracle_any=oracle_any,
                sum_brotli_fb=sum_brotli_fb, sum_bwt_fb=sum_bwt_fb, warmup=warmup)


def whole_framed(wb, wbt):
    wf = min(wb, wbt)
    return 1 + uvar_len(2 + uvar_len(wf) + wf) + 4 + (2 + uvar_len(wf) + wf)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--in", dest="inp", default="scratch/block-oracle.csv")
    ap.add_argument("--xz", type=int, default=48456100)
    ap.add_argument("--file-oracle", type=int, default=46446836)
    args = ap.parse_args()
    base = os.path.dirname(args.inp) or "."
    whole_csv = os.path.join(base, os.path.basename(args.inp).replace(".csv", "-wholes.csv"))
    wholefile = {}
    with open(whole_csv, newline="") as f:
        for r in csv.DictReader(f):
            wholefile[r["file"]] = (int(r["brotli_bytes"]), int(r["bwt_coded_bytes"]))
    data = {}
    with open(args.inp, newline="") as f:
        for r in csv.DictReader(f):
            data.setdefault((r["file"], int(r["block_size"])), []).append(r)

    print(f"{'file':10} {'bs':>9} {'blk':>4} {'BWT%':>6} {'Br%':>6} {'medM':>7} {'p95M':>8} "
          f"{'oracle':>10} {'wfFramed':>10} {'wfRegret':>9} {'warmup':>8} {'vs_xz':>10}")
    agg = {}
    for (fn, bs), rows in sorted(data.items()):
        s = summarize(rows, wholefile[fn])
        wf_fr = whole_framed(*wholefile[fn])
        wf_reg = s["oracle_coded"] - wf_fr
        print(f"{fn:10} {bs:>9} {s['blocks']:>4} {100*s['frac_bwt']:>5.1f}% {100*s['frac_brotli']:>5.1f}% "
              f"{s['median_margin']:>7} {s['p95_margin']:>8} {s['oracle_coded']:>10} {wf_fr:>10} "
              f"{wf_reg:>+9} {s['warmup']:>8} {s['oracle_coded']-args.xz:>+10}")
        agg.setdefault(fn, {})[bs] = (s, wf_reg, wf_fr)
    print("\n=== AGGREGATE per block-size (sum over 5 files) ===")
    for bs in sorted({b for (_, b) in data}):
        o = sum(agg[fn][bs][0]["oracle_coded"] for fn in agg if bs in agg[fn])
        w = sum(agg[fn][bs][1] for fn in agg if bs in agg[fn])
        wf = sum(agg[fn][bs][2] for fn in agg if bs in agg[fn])
        warm = sum(agg[fn][bs][0].get("warmup", 0) for fn in agg if bs in agg[fn])
        print(f"bs={bs:>9}: oracle={o:>10} wf_framed={wf:>10} wf_regret={w:>+10} "
              f"warmup_tax={warm:>+10} vs_xz={o-args.xz:>+10}")
    print(f"\nvs xz -9e = {args.xz}; file-oracle (s5) = {args.file_oracle}; "
          f"measured auto (bench-normalize) = 46,446,995")


if __name__ == "__main__":
    main()
