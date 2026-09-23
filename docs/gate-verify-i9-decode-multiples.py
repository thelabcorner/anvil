#!/usr/bin/env python3
"""gate-verify-i9-decode-multiples.py - definitive ratio-first decode multiples.

Recomputes every live ratio-first decode figure from the four CSV artifacts so
that exactly one citation of record exists per object. Artifacts (all
UNCOMMITTED worktree files - label them):
  tests/bwt-backend-standard.csv   (anvil-bwt-direct, per-file decompress_s)
  tests/ratio-first-standard.csv   (brotli-q11-lw30 / xz-9e / zstd-ultra-22-long27)
  tests/auto-routing.csv           (anvil-auto-direct / anvil-bwt-direct)
  tests/enwik8-bwt.csv             (anvil-auto-direct / anvil-bwt-direct)

Objects:
  A  7 BWT-routed Silesia files, forced BWT direct
  B  12-file Silesia, forced BWT direct
  C  12-file Silesia, auto portfolio  -> DEFINITIVE Silesia
  D  7 BWT-routed files in the auto run's window (window artifact)
  E  enwik8 auto (== BWT direct bytes) -> DEFINITIVE enwik8
     E' enwik8 BWT-direct row (same bytes, second window)

Usage: python docs/gate-verify-i9-decode-multiples.py
"""
from __future__ import annotations
import csv
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FILES = [
    "tests/bwt-backend-standard.csv",
    "tests/ratio-first-standard.csv",
    "tests/auto-routing.csv",
    "tests/enwik8-bwt.csv",
]
ROUTED = ["webster", "x-ray", "mr", "osdb", "dickens", "reymont", "nci"]


def rows(rel: str) -> list[dict]:
    with open(os.path.join(ROOT, rel), newline="") as fp:
        return list(csv.DictReader(fp))


def agg(rs, corpus, codec, files=None):
    sel = [r for r in rs if r["corpus"] == corpus and r["codec"] == codec
           and (files is None or r["file"] in files)]
    ib = sum(int(r["input_bytes"]) for r in sel)
    dt = sum(float(r["decompress_s"]) for r in sel)
    return len(sel), ib, dt, (ib / dt / 1e6 if dt else 0.0)


def show(tag, n, ib, dt, anvil, br, xz):
    print(f"{tag}: files={n} input={ib} dec_s={dt:.6f} anvil={anvil:.3f} MB/s "
          f"| brotli={br:.3f} -> {br / anvil:.3f}x | xz={xz:.3f} -> {xz / anvil:.3f}x")


def main() -> int:
    for rel in FILES:
        if not os.path.exists(os.path.join(ROOT, rel)):
            print(f"MISSING {rel}", file=sys.stderr)
            return 1
    rfs = rows(FILES[1])
    bwt = rows(FILES[0])
    auto = rows(FILES[2])
    ew = rows(FILES[3])

    def refs(corpus, files=None):
        _, _, _, br = agg(rfs, corpus, "brotli-q11-lw30", files)
        _, _, _, xz = agg(rfs, corpus, "xz-9e", files)
        return br, xz

    br, xz = refs("silesia", ROUTED)
    n, ib, dt, a = agg(bwt, "silesia", "anvil-bwt-direct", ROUTED)
    show("A 7-routed forced BWT", n, ib, dt, a, br, xz)

    br, xz = refs("silesia")
    n, ib, dt, a = agg(bwt, "silesia", "anvil-bwt-direct")
    show("B 12-file forced BWT", n, ib, dt, a, br, xz)

    n, ib, dt, a = agg(auto, "silesia", "anvil-auto-direct")
    show("C 12-file AUTO (DEFINITIVE Silesia)", n, ib, dt, a, br, xz)

    br7, xz7 = refs("silesia", ROUTED)
    n, ib, dt, a = agg(auto, "silesia", "anvil-bwt-direct", ROUTED)
    show("D 7-routed auto window (artifact)", n, ib, dt, a, br7, xz7)

    br, xz = refs("enwik8")
    for codec, tag in (("anvil-auto-direct", "E enwik8 AUTO (DEFINITIVE)"),
                       ("anvil-bwt-direct", "E' enwik8 BWT-direct row (variant)")):
        r = [x for x in ew if x["corpus"] == "enwik8" and x["codec"] == codec][0]
        ib = int(r["input_bytes"])
        dt = float(r["decompress_s"])
        show(tag, 1, ib, dt, ib / dt / 1e6, br, xz)

    print("\nProvenance: all four CSVs are UNCOMMITTED worktree artifacts; binary")
    print("sha256 prefix de9b4caf (build\\anvil.exe, 1,481,728 B) per handoff docs;")
    print("threads NOT recorded in the CSV schema; tools/bench_ratio.py contains no")
    print("threading code (provisional 1t; PENDING bench PR-4 attestation).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
