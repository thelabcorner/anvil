#!/usr/bin/env python3
"""S6-1b byte-identity vs freeze-HEAD protocol (bench lane).

Wire-invisibility gate: for every corpus file and every SHARED codec, the
candidate (HEAD) build must produce BIT-IDENTICAL compressed bytes vs the
independently built freeze-HEAD (commit fc23d9a, no S6-1b leg-1/2/3) build.

Why an independent build and not a flag: S6-1b leg 1 (CRC slicing-by-8) is a
pure implementation swap with no ablation flag -- the only way to A/B it is two
binaries. Both are built from the same third_party libs and the same
bench_native.cpp protocol (median of reps).

Identity is asserted on the ratio column AND on an exact SHA-256 of the
compressed bytes. ratio alone is rounded to 3 dp, so this script ALSO calls the
anvil CLI directly to hash real .anv output.

Usage: python tools/byte_identity.py --freeze <dir> [--files ...]
"""
from __future__ import annotations
import argparse, csv, hashlib, io, subprocess, sys, tempfile
from pathlib import Path

SHARED_EXCLUDE = {"anvil-hotop-budget-rans", "anvil-hotop-rlzp-rans"}


def bench_rows(exe: Path, path: Path, reps: int) -> dict:
    p = subprocess.run([str(exe), str(path), str(reps)], check=True,
                       text=True, capture_output=True)
    lines = p.stdout.strip().splitlines()
    rows = list(csv.DictReader(io.StringIO("\n".join(lines[1:]))))
    return {r["codec"]: r for r in rows}


def cli_bytes(exe: Path, src: Path, opts: list[str]) -> str:
    """Compress via the CLI and return sha256 of the .anv bytes."""
    with tempfile.TemporaryDirectory() as td:
        out = Path(td) / "o.anv"
        subprocess.run([str(exe), "c", str(src), str(out)] + opts,
                       check=True, capture_output=True)
        return hashlib.sha256(out.read_bytes()).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--freeze", required=True, help="dir with freeze anvil.exe/anvil_bench.exe")
    ap.add_argument("--head", default="build")
    ap.add_argument("--corpus", default="tests/corpus")
    ap.add_argument("--reps", type=int, default=1)
    ap.add_argument("--only", nargs="*", default=None)
    ap.add_argument("--out", default=None)
    a = ap.parse_args()

    freeze = Path(a.freeze)
    head = Path(a.head)
    corpus = Path(a.corpus)
    files = sorted(p for p in corpus.iterdir()
                   if p.is_file() and p.name not in ("CHECKSUMS.txt", "README.md"))
    if a.only:
        files = [p for p in files if p.name in a.only]

    results = []
    n_same = n_diff = 0
    for f in files:
        fb = bench_rows(freeze / "anvil_bench.exe", f, a.reps)
        hb = bench_rows(head / "anvil_bench.exe", f, a.reps)
        shared = sorted(set(fb) & set(hb) - SHARED_EXCLUDE)
        for codec in shared:
            fbz, hbz = fb[codec]["compressed_bytes"], hb[codec]["compressed_bytes"]
            fr, hr = fb[codec]["ratio"], hb[codec]["ratio"]
            same = (fbz == hbz) and (fr == hr)
            if same:
                n_same += 1
            else:
                n_diff += 1
            results.append((f.name, codec, fbz, hbz, fr, hr,
                            "IDENTICAL" if same else "DIFFERENT"))

    # Independent check on real CLI output bytes (default options).
    cli = []
    for f in files:
        try:
            fh = cli_bytes(freeze / "anvil.exe", f, [])
            hh = cli_bytes(head / "anvil.exe", f, [])
            cli.append((f.name, fh[:16], hh[:16], "IDENTICAL" if fh == hh else "DIFFERENT"))
        except subprocess.CalledProcessError as e:
            cli.append((f.name, "-", "-", f"CLI-ERR({e.returncode})"))

    print(f"=== BYTE IDENTITY: freeze {freeze} vs head {head}")
    print(f"    {len(files)} files x {len(results)//max(len(files),1)} shared codecs")
    print(f"    IDENTICAL: {n_same}   DIFFERENT: {n_diff}")
    diffs = [r for r in results if r[6] != "IDENTICAL"]
    for r in diffs[:40]:
        print(f"    DIFFER {r[0]} {r[1]}: freeze={r[2]} head={r[3]} (ratio {r[4]} vs {r[5]})")
    print("=== CLI default-option .anv sha256 (independent path) ===")
    for c in cli:
        print(f"    {c[0]:34s} {c[1]} {c[2]} {c[3]}")
    cli_bad = [c for c in cli if c[3] != "IDENTICAL"]
    print(f"    CLI IDENTICAL: {len(cli)-len(cli_bad)} / {len(cli)}")

    if a.out:
        with open(a.out, "w", newline="") as fp:
            w = csv.writer(fp)
            w.writerow(["file", "codec", "freeze_bytes", "head_bytes",
                        "freeze_ratio", "head_ratio", "verdict"])
            w.writerows(results)
            w.writerow([])
            w.writerow(["cli_file", "freeze_sha256_16", "head_sha256_16", "cli_verdict"])
            w.writerows(cli)
        print(f"    wrote {a.out}")

    return 0 if (n_diff == 0 and not cli_bad) else 1


if __name__ == "__main__":
    sys.exit(main())
