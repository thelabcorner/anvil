#!/usr/bin/env python3
"""analyze_post.py -- decode-perf I9 leg 3: turn results/<label>_summary.txt from
post_prof_wt.exe into stage-share and single-component-cap (C) tables.

Definitions (bench anti-tie v2):
  s        = removed time share of the end-to-end baseline (median)
  C        = 1/(1-s)  single-component cap
  R_40     = 40 / postcoder_only_MBps   (per-file requirement to reach the 06 F0.1
             40 MB/s BWT-only reopen bar assuming a free/unbwt-hidden inverse)
  R_xz     = 78.7 / postcoder_only_MBps (xz-match bar, context)
  pass iff C >= 1.02 * R
Labels: measured (ppm), derived (arithmetic on measured medians).
"""
import re, sys, pathlib

def parse(path):
    blocks = []
    cur = None
    for line in open(path, encoding="utf-8", errors="replace"):
        m = re.match(r"^=== (.+) ===\s*$", line)
        if m:
            cur = {"name": m.group(1)}
            blocks.append(cur)
            continue
        if cur is None:
            continue
        def g(pat, key, cast=float):
            mm = re.search(pat, line)
            if mm:
                cur[key] = cast(mm.group(1))
        g(r"src=(\d+)", "src", int)
        g(r"comp=(\d+)", "comp", int)
        g(r"post=(-?\d+)", "post", int)
        g(r"xlen=(\d+)", "xlen", int)
        g(r"BASELINE full decode: ([\d.]+) MB/s", "base_mbps")
        g(r"POSTCODER \(arith\) med=([\d.]+) ms = ([\d.]+)% of e2e", "post_ms")
        mm = re.search(r"POSTCODER \(arith\) med=([\d.]+) ms = ([\d.]+)% of e2e", line)
        if mm:
            cur["post_pct"] = float(mm.group(2))
        g(r"setup \(ad init \+ eager models\): ([\d.]+) ms = ([\d.]+)% of e2e", "setup_ms")
        g(r"token\+uvar decode \(no MTF/no writes\): ([\d.]+) ms = ([\d.]+)% of e2e", "token_ms")
        g(r"MTF list update \(no writes\): ([\d.]+) ms = ([\d.]+)% of e2e", "mtf_ms")
        g(r"output materialization \(writes\): ([\d.]+) ms = ([\d.]+)% of e2e", "out_ms")
        g(r"PROTOTYPE fast_renorm: ([\d.]+) ms vs full ([\d.]+) ms -> ([\d.]+)x", "fast_ms")
        g(r"model_build_x257: ([\d.]+) ms per full set \(([\d.]+) ms/model\) -> lazy-build est share = (\d+) \* per_model / post_full = ([\d.]+)%", "build_share_pct")
        g(r"counters: tokens=(\d+) runs=(\d+) run_bytes=(\d+) out_bytes=(\d+) O1_model_builds=(\d+)", "tokens", int)
        mm = re.search(r"counters: tokens=(\d+) runs=(\d+) run_bytes=(\d+) out_bytes=(\d+) O1_model_builds=(\d+)", line)
        if mm:
            cur["tokens"] = int(mm.group(1)); cur["runs"] = int(mm.group(2))
            cur["run_bytes"] = int(mm.group(3)); cur["out_bytes"] = int(mm.group(4))
            cur["model_builds"] = int(mm.group(5))
        g(r"POSTCODER \(raw stream\) med=([\d.]+) ms = ([\d.]+)% of e2e", "raw_ms")
        g(r"codec=(\d+) wire=(\d+) raw=(\d+)", "raw_wire", int)
        mm = re.search(r"codec=(\d+) wire=(\d+) raw=(\d+)", line)
        if mm:
            cur["raw_codec"] = int(mm.group(1)); cur["raw_wire"] = int(mm.group(2)); cur["raw_raw"] = int(mm.group(3))
    return blocks

def main():
    summary = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "results/i9_post_summary.txt")
    blocks = parse(summary)
    print("file | post | base_MBps | post%_e2e | pc_MBps | s_fast | C_fast | s_model | C_model | s_mtf | C_mtf | R_40 | R'_40 | fast_pass")
    for b in blocks:
        if not b.get("src"):
            continue
        base_ms = b["src"] / b["base_mbps"] / 1e3 if b.get("base_mbps") else None  # ms
        post_ms = b.get("post_ms") or b.get("raw_ms")
        post_pct = b.get("post_pct")
        pc_mbps = (b["xlen"] / 1e6) / (post_ms / 1e3) if post_ms else None
        def C(ms):
            return 1.0 / (1.0 - (ms / base_ms)) if ms is not None and base_ms and ms / base_ms < 1 else None
        s_fast = (b.get("post_ms", 0) - b.get("fast_ms", b.get("post_ms", 0))) / base_ms if b.get("fast_ms") else None
        C_fast = C(s_fast * base_ms) if s_fast is not None else None
        model_share = b.get("build_share_pct")
        C_model = 1.0 / (1.0 - model_share / 100.0) if model_share is not None and model_share < 100 else None
        C_mtf = C(b.get("mtf_ms", 0)) if b.get("mtf_ms") else None
        R40 = 40.0 / pc_mbps if pc_mbps else None
        R40p = 1.02 * R40 if R40 else None
        fp = "GO" if (C_fast and R40p and C_fast >= R40p) else ("SHORT" if C_fast else "-")
        nm = pathlib.Path(b["name"]).name
        print(f"{nm} | post={b.get('post')} | {b.get('base_mbps')} | {post_pct} | {pc_mbps:.2f} | "
              f"{s_fast if s_fast is None else round(s_fast,4)} | {C_fast if C_fast is None else round(C_fast,3)} | "
              f"{model_share} | {C_model if C_model is None else round(C_model,3)} | "
              f"{round(b.get('mtf_ms',0)/base_ms,4) if b.get('mtf_ms') else None} | {C_mtf if C_mtf is None else round(C_mtf,3)} | "
              f"{R40 if R40 is None else round(R40,3)} | {R40p if R40p is None else round(R40p,3)} | {fp}")

if __name__ == "__main__":
    main()
