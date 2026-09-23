#!/usr/bin/env python3
"""
datastruct (I9) — reproduce the colref ACTRIFACT ledger in one command.

Encodes each variant, writes the wire to prototypes/i9-datastruct/out/,
decodes the written file back and byte-compares, and prints bytes / ratio /
sha256. Every published number in RESULTS.md is reproduced by running this.

Usage: python prototypes/i9-datastruct/verify_artifacts.py
"""
from __future__ import annotations
import hashlib
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import colref  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
TS = "tests/corpus/synth-timeseries.bin"
CA = "tests/corpus/synth-columnar-align.bin"

TRUE_CA = [(0, 2), (2, 4), (6, 8), (14, 4), (18, 2), (20, 1), (21, 1), (22, 1)]
NAIVE_CA = [(0, 4), (4, 4), (8, 4), (12, 4), (16, 4), (20, 3)]
# 8 rows of the true 23-B pattern = the P=184 brief framing
CA184 = [(23 * k + o, w) for k in range(8) for (o, w) in TRUE_CA]

VARIANTS = [
    ("ts.raw_o0",    TS, "raw_o0",    14, None),
    ("ts.raw_o1",    TS, "raw_o1",    14, None),
    ("ts.ref_flat",  TS, "ref_flat",  14, None),
    ("ts.ref_col",   TS, "ref_col",   14, None),
    ("ts.ref_field", TS, "ref_field", 14, [(0, 8), (8, 4), (12, 2)]),
    ("ca.ref_col",   CA, "ref_col",   23, None),
    ("ca.ref_field_true",  CA, "ref_field", 23, TRUE_CA),
    ("ca.ref_field_naive", CA, "ref_field", 23, NAIVE_CA),
    ("ca.ref_field_p184",  CA, "ref_field", 184, CA184),
]


def main():
    os.makedirs(OUT, exist_ok=True)
    print("variant                 bytes    ratio   sha256[0:16]  rt  classes")
    rows = []
    for (name, path, mode, P, layout) in VARIANTS:
        data = open(path, "rb").read()
        wire = colref.encode(data, mode, P=P, layout=layout)
        back = colref.decode(wire)
        ok = back == data
        wpath = os.path.join(OUT, name)
        open(wpath, "wb").write(wire)
        sha = hashlib.sha256(wire).hexdigest()[:16]
        desc = ""
        if mode == "ref_field":
            _, _, _, fields, _ = colref.parse_header(wire)
            desc = ",".join(f"{o}:{w}:{colref.C_NAMES[c]}" for (o, w, c, _) in fields)
        print(f"{name:22s} {len(wire):7d}  {len(wire)/len(data):.4f}  {sha}  "
              f"{'OK ' if ok else 'FAIL'} {desc}")
        rows.append((name, len(wire), len(data), ok, sha))
        if not ok:
            raise SystemExit("ROUNDTRIP FAIL " + name)
    print("\nall roundtrips OK")


if __name__ == "__main__":
    main()
