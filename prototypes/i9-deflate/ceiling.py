#!/usr/bin/env python3
"""P4.1 ceiling: measure the DEFLATE-reconstruction byte economics on a file.

For a census JSON produced by census.py:
  - extract every verified DEFLATE payload and its plaintext;
  - write concatenated artifacts:
      plain.bin    (concatenated decompressed plaintext, stream order)
      deflate.bin  (concatenated original compressed payloads, same order)
      index.csv    (offset, clen, ulen, kind, entry)
  - compress each artifact with the strong reference codecs
    (brotli q11/lgwin24, xz -9e via lzma, zstd 22) and report:
      gain_local = cost(deflate.bin) - cost(plain.bin)
    This is the REGION-LOCAL ceiling: it assumes payload regions are stored
    standalone and that replay is bit-exact.  Reconstruction overhead
    (side table, corrections) is NOT charged here -- see transform_end2end.py.

Usage:
  python ceiling.py results/census-<file>.json --src <file> --out-dir results
"""
import argparse
import csv
import json
import lzma
import os
import zlib

import brotli
import zstandard as zstd


def extract_streams(src_path, census_json):
    data = open(src_path, "rb").read()
    cd = json.load(open(census_json))
    streams = []
    for s in cd["streams"]:
        if s["method"] != "deflate" or not s["verified"]:
            continue
        off, clen = s["offset"], s["clen"]
        comp = data[off:off + clen]
        try:
            if s["kind"] in ("zip", "cab-mszip"):
                dec = zlib.decompress(comp, -15)
            else:
                dec = zlib.decompress(comp)
        except Exception as e:  # noqa: BLE001
            s["error"] = str(e)
            continue
        s["plain"] = dec
        s["comp"] = comp
        streams.append(s)
    streams.sort(key=lambda s: s["offset"])
    return streams


def codec_sizes(payload: bytes, tag: str):
    out = {}
    out["brotli_q11_lgwin24"] = len(brotli.compress(payload, quality=11, lgwin=24))
    out["xz_9e"] = len(lzma.compress(payload, format=lzma.FORMAT_XZ,
                                     preset=9 | lzma.PRESET_EXTREME))
    out["zstd_22"] = len(zstd.ZstdCompressor(level=22).compress(payload))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("census_json")
    ap.add_argument("--src", required=True)
    ap.add_argument("--out-dir", default="results")
    ap.add_argument("--skip-codecs", action="store_true")
    a = ap.parse_args()
    os.makedirs(a.out_dir, exist_ok=True)
    base = os.path.splitext(os.path.basename(a.census_json))[0]
    streams = extract_streams(a.src, a.census_json)
    if not streams:
        print("no verified deflate streams")
        return
    plain = b"".join(s["plain"] for s in streams)
    comp = b"".join(s["comp"] for s in streams)
    open(os.path.join(a.out_dir, f"{base}-plain.bin"), "wb").write(plain)
    open(os.path.join(a.out_dir, f"{base}-deflate.bin"), "wb").write(comp)
    with open(os.path.join(a.out_dir, f"{base}-index.csv"), "w", newline="",
              encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["offset", "clen", "ulen", "kind", "entry"])
        for s in streams:
            w.writerow([s["offset"], s["clen"], s["ulen"], s["kind"], s["entry"]])
    C = sum(s["clen"] for s in streams)
    U = sum(len(s["plain"]) for s in streams)
    summary = {
        "n_streams": len(streams),
        "deflate_bytes_C": C,
        "plain_bytes_U": U,
        "deflate_ratio": C / U if U else 0.0,
        "codecs": {},
    }
    if not a.skip_codecs:
        for tag, payload in (("deflate.bin", comp), ("plain.bin", plain)):
            summary["codecs"][tag] = codec_sizes(payload, tag)
        for codec in summary["codecs"]["deflate.bin"]:
            cd = summary["codecs"]["deflate.bin"][codec]
            cp = summary["codecs"]["plain.bin"][codec]
            summary.setdefault("gain_local", {})[codec] = {
                "cost_deflate": cd,
                "cost_plain": cp,
                "gain": cd - cp,
                "gain_pct_of_deflate": 100.0 * (cd - cp) / cd if cd else 0.0,
            }
    out = os.path.join(a.out_dir, f"{base}-ceiling.json")
    json.dump(summary, open(out, "w", encoding="utf-8"), indent=1)
    print(json.dumps(summary, indent=2))
    print(f"[ceiling] wrote {out}, {base}-plain.bin, {base}-deflate.bin")


if __name__ == "__main__":
    main()
