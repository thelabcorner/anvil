#!/usr/bin/env python3
"""P4.1 replay prototype: try to reproduce an embedded DEFLATE bitstream from
its plaintext alone, using zlib parameter search (precomp-style "valid" mode).

Modes per stream:
  valid : some (level, memLevel, strategy, wbits) reproduces the payload
          byte-for-byte.  Reconstruction cost = parameter id (~1 byte).
  diff  : no exact replay; we measure a naive correction proxy:
          first-diff byte position and xz-compressed XOR delta (UPPER bound on
          correction cost; preflate-class coders do better -- labelled).
  brute : fallback = keep original payload bytes (no transform).

Usage:
  python replay.py <census.json> --src <file> --out-dir results [--region <container>]
"""
import argparse
import csv
import json
import lzma
import os
import sys
import zlib

LEVELS = list(range(1, 10))
MEMLEVELS = list(range(1, 10))
STRATEGIES = [zlib.Z_DEFAULT_STRATEGY, zlib.Z_FILTERED, zlib.Z_HUFFMAN_ONLY,
              zlib.Z_RLE, zlib.Z_FIXED]

# Priority list: most common producers first (zlib default 6/8/0, then 9/8/0).
def priority_params():
    out = [(l, 8, 0) for l in (6, 9, 1, 2, 3, 4, 5, 7, 8)]
    out += [(6, 9, 0), (9, 9, 0), (6, 8, 1), (6, 8, 2), (6, 8, 3), (6, 8, 4)]
    out += [(9, 8, s) for s in (1, 2, 3, 4)]
    return out


PRIORITY = priority_params()


def deflate_with(plain, level, memlevel, strategy, wbits):
    c = zlib.compressobj(level, zlib.DEFLATED, wbits, memlevel, strategy)
    return c.compress(plain) + c.flush()


def search_exact(plain, target, kind):
    """Return list of (wbits, level, memlevel, strategy) that reproduces target."""
    hits = []
    if kind in ("zip", "cab-mszip"):
        wbits_list = [-15]
    else:
        # zlib stream: try the window from the CMF byte, then full 15
        cmf = target[0]
        cinfo = (cmf >> 4) & 0x0F
        w = cinfo + 8
        wbits_list = [w] if 9 <= w <= 15 else []
        if 15 not in wbits_list:
            wbits_list.append(15)
    tried = set()
    for wbits in wbits_list:
        for (l, m, s) in PRIORITY:
            if (wbits, l, m, s) in tried:
                continue
            tried.add((wbits, l, m, s))
            try:
                out = deflate_with(plain, l, m, s, wbits)
            except Exception:  # noqa: BLE001
                continue
            if out == target:
                hits.append((wbits, l, m, s))
                return hits
        # full grid for this wbits only if priority failed
        for l in LEVELS:
            for m in MEMLEVELS:
                for s in STRATEGIES:
                    if (wbits, l, m, s) in tried:
                        continue
                    tried.add((wbits, l, m, s))
                    try:
                        out = deflate_with(plain, l, m, s, wbits)
                    except Exception:  # noqa: BLE001
                        continue
                    if out == target:
                        hits.append((wbits, l, m, s))
                        return hits
    return hits


def correction_proxy(plain, target, kind, best_params):
    """Naive diff-mode cost proxy: re-deflate with best_params (or zlib 6/8/0),
    XOR against target, xz the XOR.  UPPER bound on corrections."""
    wbits, l, m, s = best_params
    try:
        replay = deflate_with(plain, l, m, s, wbits)
    except Exception:  # noqa: BLE001
        replay = b""
    # first divergence
    n = min(len(replay), len(target))
    fd = next((i for i in range(n) if replay[i] != target[i]), n)
    x = bytes(a ^ b for a, b in zip(replay, target)) if replay else target
    xz_size = len(lzma.compress(x, format=lzma.FORMAT_XZ,
                                preset=9 | lzma.PRESET_EXTREME)) if x else 0
    return fd, xz_size, len(replay)


def best_effort(plain, target, kind):
    """Parameter set minimizing XOR divergence (used for diff-mode proxy)."""
    if kind in ("zip", "cab-mszip"):
        wbits_list = [-15]
    else:
        cmf = target[0]
        c = (cmf >> 4) & 0x0F
        wbits_list = [c + 8] if 9 <= c + 8 <= 15 else [15]
    best = None
    best_score = None
    for wbits in wbits_list:
        for (l, m, s) in [(6, 8, 0), (9, 8, 0), (1, 8, 0), (6, 9, 0), (9, 9, 0),
                          (6, 8, 1), (6, 8, 2), (6, 8, 3), (6, 8, 4),
                          (9, 8, 4), (1, 9, 0), (2, 8, 0), (3, 8, 0), (4, 8, 0),
                          (5, 8, 0), (7, 8, 0), (8, 8, 0)]:
            try:
                out = deflate_with(plain, l, m, s, wbits)
            except Exception:  # noqa: BLE001
                continue
            n = min(len(out), len(target))
            fd = next((i for i in range(n) if out[i] != target[i]), n)
            score = (fd + abs(len(out) - len(target)))
            if best_score is None or score > best_score:
                best_score = score
                best = (wbits, l, m, s)
    return best


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("census_json")
    ap.add_argument("--src", required=True)
    ap.add_argument("--out-dir", default="results")
    ap.add_argument("--region", default=None,
                    help="container value to restrict to (bounded region)")
    ap.add_argument("--limit", type=int, default=0)
    a = ap.parse_args()
    os.makedirs(a.out_dir, exist_ok=True)
    data = open(a.src, "rb").read()
    cd = json.load(open(a.census_json))
    streams = [s for s in cd["streams"]
               if s["method"] == "deflate" and s["verified"]]
    name = cd["summary"]["file"]
    if a.region:
        streams = [s for s in streams if s["container"] == a.region]
    if a.limit:
        streams = streams[:a.limit]
    streams.sort(key=lambda s: s["offset"])
    print(f"[replay] {len(streams)} streams, region={a.region or 'ALL'}", flush=True)

    rows = []
    tally = {"valid": 0, "diff": 0, "brute": 0}
    bytes_tally = {"valid": 0, "diff": 0, "brute": 0}
    valid_bytes = 0
    diff_corr_proxy = 0
    param_hist = {}
    for i, s in enumerate(streams):
        comp = data[s["offset"]:s["offset"] + s["clen"]]
        if s["kind"] in ("zip", "cab-mszip"):
            plain = zlib.decompress(comp, -15)
        else:
            plain = zlib.decompress(comp)
        hits = search_exact(plain, comp, s["kind"])
        if hits:
            mode = "valid"
            params = hits[0]
            corr = 0
            fd = len(comp)
            replay_len = len(comp)
        else:
            mode = "diff" if len(plain) >= 256 else "brute"
            params = best_effort(plain, comp, s["kind"])
            fd, corr, replay_len = correction_proxy(plain, comp, s["kind"], params)
            if mode == "brute":
                mode = "brute"
        tally[mode] += 1
        bytes_tally[mode] += s["clen"]
        if mode == "valid":
            valid_bytes += s["clen"]
            key = f"w{params[0]}_l{params[1]}_m{params[2]}_s{params[3]}"
            param_hist[key] = param_hist.get(key, 0) + 1
        elif mode == "diff":
            diff_corr_proxy += corr
        rows.append({
            "offset": s["offset"], "clen": s["clen"], "ulen": len(plain),
            "kind": s["kind"], "entry": s["entry"],
            "mode": mode, "param": f"{params[0]}/{params[1]}/{params[2]}/{params[3]}",
            "first_diff": fd if mode != "valid" else "",
            "corr_proxy_xz": corr if mode == "diff" else "",
        })
        if (i + 1) % 250 == 0:
            print(f"[replay] {i+1}/{len(streams)} valid={tally['valid']} "
                  f"diff={tally['diff']} brute={tally['brute']}", flush=True)

    suffix = f"-{a.region.replace('@','').replace('/','_')}" if a.region else ""
    out_csv = os.path.join(a.out_dir, f"replay-{name}{suffix}.csv")
    with open(out_csv, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)

    C = sum(s["clen"] for s in streams)
    summary = {
        "file": name, "region": a.region or "ALL", "n_streams": len(streams),
        "C_deflate": C,
        "tally": tally,
        "bytes_tally": bytes_tally,
        "valid_fraction_of_streams": tally["valid"] / len(streams) if streams else 0,
        "valid_fraction_of_bytes": valid_bytes / C if C else 0,
        "diff_corr_proxy_xz_bytes": diff_corr_proxy,
        "param_hist": param_hist,
        "zlib_version": zlib.ZLIB_VERSION,
    }
    out_json = os.path.join(a.out_dir, f"replay-{name}{suffix}.json")
    json.dump(summary, open(out_json, "w", encoding="utf-8"), indent=1)
    print(json.dumps(summary, indent=2))
    print(f"[replay] wrote {out_csv} / {out_json}")


if __name__ == "__main__":
    main()
