#!/usr/bin/env python3
"""
datastruct (I9 follow-up) — reproduce the AUTO-DISCOVERY artifact ledger.

  python prototypes/i9-datastruct/verify_discovery.py            # ts + ca (fast)
  python prototypes/i9-datastruct/verify_discovery.py --heavy    # + jsonl (slow ~10 min)

Encodes with discovery (no P, no partition given), decodes the written wire,
byte-compares, prints bytes/ratio/P/partition/hash. All discovery state charged.
"""
from __future__ import annotations
import hashlib
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import auto_colref as A  # noqa: E402
from colref import C_NAMES  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
JOBS = [
    ("ts.auto", "tests/corpus/synth-timeseries.bin", 256, "0:8:prev,8:4:prev,12:2:posmod"),
    ("ca.auto", "tests/corpus/synth-columnar-align.bin", 256,
     "0:6:posmod,6:8:posmod,14:4:prev,18:3:posmod,21:2:prev"),
]
HEAVY = [("jsonl.auto", "tests/corpus/generated.jsonl", 1024, None)]


def run(name, path, pmax, expect_part):
    data = open(path, "rb").read()
    wire, P, fields = A.encode_auto(data, pmax=pmax)
    back = A.decode_auto(wire)
    ok = back == data
    part = ",".join(f"{o}:{w}:{C_NAMES[c]}" for (o, w, c, _) in fields)
    open(os.path.join(OUT, name), "wb").write(wire)
    sha = hashlib.sha256(wire).hexdigest()
    print(f"{name:10s} bytes={len(wire):8d} ratio={len(wire)/len(data):.4f} "
          f"P={P} roundtrip={'OK' if ok else 'FAIL'} sha256={sha}")
    print(f"           partition={part}")
    if expect_part and part != expect_part:
        print(f"           WARNING: partition changed (was {expect_part})")
    if not ok:
        raise SystemExit("ROUNDTRIP FAIL " + name)


def main():
    os.makedirs(OUT, exist_ok=True)
    for j in JOBS:
        run(*j)
    if "--heavy" in sys.argv:
        for j in HEAVY:
            run(*j)
    print("\nall roundtrips OK")


if __name__ == "__main__":
    main()
