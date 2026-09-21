#!/usr/bin/env python3
"""Measure per-file compressed size for mode-14 (tcopy) with --pnra on/off
across the full corpus. Reports ratio vs plain tcopy (pnra=off) per file and
captures the g_pnra_* diagnostic counters from stderr."""
import subprocess, os, sys, re, glob

ANVIL = sys.argv[1] if len(sys.argv) > 1 else r".\build\anvil.exe"
CORPUS = r"tests\corpus"
OUT = r"C:\Users\SLOOSH~1\AppData\Local\Temp\opencode"

DATA_FILES = ["anvil.exe", "anvil_bench.exe", "doc.md", "generated.json",
              "generated.jsonl", "generated.log", "generated.repeat.jsonl",
              "generated.sqlite", "random.bin", "src.cpp",
              "synth-arith.bin", "synth-jitter.bin", "synth-timeseries.bin"]

def comp_size(path, pnra):
    outp = os.path.join(OUT, "_m.dat")
    cmd = [ANVIL, "c", path, outp, "--parse=tcopy",
           f"--pnra={'on' if pnra else 'off'}", "--quiet"]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(f"{path} pnra={pnra} failed: {r.stderr.strip()}")
    return os.path.getsize(outp)

def main():
    rows = []
    for f in DATA_FILES:
        p = os.path.join(CORPUS, f)
        if not os.path.exists(p):
            print(f"MISSING {p}"); continue
        orig = os.path.getsize(p)
        try:
            off = comp_size(p, False)
            on = comp_size(p, True)
        except RuntimeError as e:
            print(e); continue
        delta = 100.0 * (on - off) / off
        rows.append((f, orig, off, on, delta))
        print(f"{f:28s} orig={orig:>9d} off={off:>9d} on={on:>9d} "
              f"d(on-vs-off)= {delta:+.4f}%  r_off={off/orig:.6f} r_on={on/orig:.6f}")
    tot_o = sum(r[2] for r in rows); tot_n = sum(r[3] for r in rows)
    print("\nAGGREGATE  sum_off=%d sum_on=%d  d= %+.4f%%"
          % (tot_o, tot_n, 100.0*(tot_n-tot_o)/tot_o))

if __name__ == "__main__":
    main()
