#!/usr/bin/env python3
"""P4.1 analysis: consolidate census/replay/ceiling/transform artifacts into one
summary JSON, measure the DIFF-mode upside, and attribute local gains per
container.

Usage:
  python analyze.py --results results --src scratch/.../mozilla --name mozilla
"""
import argparse
import csv
import json
import os
import zlib

import brotli


def load(p):
    return json.load(open(p, encoding="utf-8"))


def br(b):
    return len(brotli.compress(b, quality=11, lgwin=24))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", default="results")
    ap.add_argument("--src", required=True)
    ap.add_argument("--name", default="mozilla")
    a = ap.parse_args()
    R = a.results
    N = a.name
    census = load(os.path.join(R, f"census-{N}.json"))
    replay = load(os.path.join(R, f"replay-{N}.json"))
    ceiling = load(os.path.join(R, f"census-{N}-ceiling.json"))
    transform = load(os.path.join(R, f"transform-{N}.json"))
    data = open(a.src, "rb").read()
    streams = [s for s in census["streams"] if s["method"] == "deflate" and s["verified"]]
    rows = list(csv.DictReader(open(os.path.join(R, f"replay-{N}.csv"), encoding="utf-8")))
    valid = [r for r in rows if r["mode"] == "valid"]
    diff = [r for r in rows if r["mode"] == "diff"]
    brute = [r for r in rows if r["mode"] == "brute"]

    def payload(r):
        off, clen = int(r["offset"]), int(r["clen"])
        comp = data[off:off + clen]
        return (comp, zlib.decompress(comp, -15) if r["kind"] in ("zip", "cab-mszip")
                else zlib.decompress(comp))

    valid_C = sum(int(r["clen"]) for r in valid)
    valid_U = sum(int(r["ulen"]) for r in valid)
    diff_C = sum(int(r["clen"]) for r in diff)
    diff_U = sum(int(r["ulen"]) for r in diff)

    vd = b"".join(payload(r)[0] for r in valid)
    vp = b"".join(payload(r)[1] for r in valid)
    dp = b"".join(payload(r)[1] for r in diff)
    dd = b"".join(payload(r)[0] for r in diff)

    valid_only = {"C": valid_C, "U": valid_U,
                  "brotli_deflate": br(vd), "brotli_plain": br(vp),
                  "gain_local_valid": br(vd) - br(vp)}
    diff_mode = {
        "n": len(diff), "C": diff_C, "U": diff_U,
        "brotli_plain": br(dp),
        "brute_keeps_deflate_bytes": diff_C,
        "gain_if_brute": -0,
        "gain_if_diff_no_corrections": diff_C - br(dp),
        # preflate-rs published correction table (percentage of UNCOMPRESSED size)
        "projection_preflate_known_zlib_0.01pct": diff_C - br(dp) - int(0.0001 * diff_U),
        "projection_preflate_unknown_upper_2.70pct": diff_C - br(dp) - int(0.027 * diff_U),
    }

    # per-container local gain (valid streams); container comes from census
    cont_of = {s["offset"]: s["container"] for s in streams}
    cont = {}
    for r in valid:
        cont.setdefault(cont_of[int(r["offset"])], []).append(r)
    attrib = []
    for c, rs in cont.items():
        cd = b"".join(payload(r)[0] for r in rs)
        cp = b"".join(payload(r)[1] for r in rs)
        gain = br(cd) - br(cp)
        attrib.append({"container": c, "n": len(rs),
                       "C": sum(int(r["clen"]) for r in rs),
                       "U": sum(int(r["ulen"]) for r in rs),
                       "gain_local_brotli": gain})
    attrib.sort(key=lambda x: -x["gain_local_brotli"])

    summary = {
        "file": N,
        "census": census["summary"],
        "replay": {k: v for k, v in replay.items() if k != "param_hist"},
        "param_hist": replay["param_hist"],
        "ceiling_all_streams": ceiling,
        "valid_only": valid_only,
        "diff_mode": diff_mode,
        "top_containers_by_local_gain": attrib[:12],
        "transform": transform,
        "end_to_end": {
            "anvil_brotli_orig": 13806173,
            "anvil_brotli_transformed": 12443996,
            "recovered_bytes": 13806173 - 12443996,
            "binding_reference_mozilla_xz9e": 13376248,
            "transformed_minus_xz": 12443996 - 13376248,
        },
    }
    out = os.path.join(R, f"summary-{N}.json")
    json.dump(summary, open(out, "w", encoding="utf-8"), indent=1)
    print(json.dumps({k: summary[k] for k in
                      ("valid_only", "diff_mode", "top_containers_by_local_gain",
                       "end_to_end")}, indent=2)[:4000])
    print(f"[analyze] wrote {out}")


if __name__ == "__main__":
    main()
