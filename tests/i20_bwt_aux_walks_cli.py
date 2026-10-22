#!/usr/bin/env python3
"""Corpus-free CLI contract test for the I20 C2 --bwt-aux-walks knob.

The knob is an encoder policy: it selects the auxiliary-index sampling rate r
that the rev-2 mode-17 wire already transmits.  This test pins the *CLI
contract* so an off-by-one in the option parsing cannot silently regress:

  * both spellings are accepted and are equivalent:
        --bwt-aux-walks 4096      --bwt-aux-walks=4096
  * the '=' spelling really carries the value (an off-by-one in the substring
    offset yields "=4096", which must be rejected rather than accepted with
    a wrong value or a corrupted payload);
  * the default (no flag) is byte-identical to the explicit default
    "--bwt-aux-walks 1024";
  * out-of-range and non-numeric values are rejected with an explicit error
    and no output file is produced (never a corrupt file);
  * --bwt-subblock accepts both spellings as well;
  * every accepted arm decodes byte-exactly back to the source.

The test needs no corpus: it encodes a small synthetic payload.

usage: tests/i20_bwt_aux_walks_cli.py --exe <path-to-anvil>
"""

from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
import tempfile
from pathlib import Path


FLAGS = [
    "--parse=ratio", "--ratio-context=off", "--ratio-lines=off",
    "--ratio-backend=bwt", "--bwt-aux=on", "--quiet",
]


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    h.update(path.read_bytes())
    return h.hexdigest()


def run(exe: Path, args: list[str]) -> subprocess.CompletedProcess:
    return subprocess.run([str(exe), *args], capture_output=True, text=True, check=False)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", required=True)
    args = ap.parse_args()
    exe = Path(args.exe).resolve()
    if not exe.is_file():
        print(f"missing anvil executable: {exe}")
        return 1

    failures: list[str] = []

    def check(cond: bool, msg: str) -> None:
        if cond:
            print(f"PASS {msg}")
        else:
            print(f"FAIL {msg}")
            failures.append(msg)

    with tempfile.TemporaryDirectory(prefix="anvil-walks-cli-") as td0:
        td = Path(td0)
        src = td / "src.bin"
        # A payload that compresses well enough to take the BWT backend but is
        # small enough to keep the test fast.
        src.write_bytes((b"ANVIL I20 C2 walk-budget CLI contract payload. " * 4096))
        src_sha = sha256(src)

        def encode(out: Path, extra: list[str]) -> subprocess.CompletedProcess:
            return run(exe, ["c", str(src), str(out), *FLAGS, *extra])

        def decode(packed: Path, out: Path) -> subprocess.CompletedProcess:
            return run(exe, ["d", str(packed), str(out), "--quiet"])

        outs: dict[str, Path] = {}
        for label, extra in (
            ("spaced4096", ["--bwt-aux-walks", "4096"]),
            ("equals4096", ["--bwt-aux-walks=4096"]),
            ("spaced1024", ["--bwt-aux-walks", "1024"]),
            ("equals1024", ["--bwt-aux-walks=1024"]),
            ("default", []),
            ("spaced16384", ["--bwt-aux-walks", "16384"]),
            ("equals16384", ["--bwt-aux-walks=16384"]),
        ):
            out = td / f"{label}.anv"
            r = encode(out, extra)
            check(r.returncode == 0,
                  f"{label}: encode succeeded ({r.stderr.strip()[:160]})")
            if r.returncode == 0:
                outs[label] = out
                dec = td / f"{label}.out"
                rd = decode(out, dec)
                check(rd.returncode == 0 and dec.is_file() and sha256(dec) == src_sha,
                      f"{label}: round-trips byte-exactly")

        if "spaced4096" in outs and "equals4096" in outs:
            check(sha256(outs["spaced4096"]) == sha256(outs["equals4096"]),
                  "spaced and '=' spellings of --bwt-aux-walks produce identical bytes")
        if "spaced1024" in outs and "equals1024" in outs:
            check(sha256(outs["spaced1024"]) == sha256(outs["equals1024"]),
                  "'--bwt-aux-walks 1024' and '--bwt-aux-walks=1024' agree")
        if "spaced1024" in outs and "default" in outs:
            check(sha256(outs["spaced1024"]) == sha256(outs["default"]),
                  "the default is byte-identical to the explicit '--bwt-aux-walks 1024'")
        if "spaced4096" in outs and "default" in outs:
            check(sha256(outs["spaced4096"]) != sha256(outs["default"]),
                  "a non-default walk budget actually changes the payload")
        if "spaced16384" in outs and "equals16384" in outs:
            check(sha256(outs["spaced16384"]) == sha256(outs["equals16384"]),
                  "'--bwt-aux-walks 16384' and '--bwt-aux-walks=16384' agree")

        # Rejections: out-of-range, non-numeric, and a missing value.  Each
        # must fail with an explicit error and must not leave an output file.
        for label, extra in (
            ("zero", ["--bwt-aux-walks=0"]),
            ("zero_spaced", ["--bwt-aux-walks", "0"]),
            ("too_large", ["--bwt-aux-walks=1048577"]),
            ("too_large_spaced", ["--bwt-aux-walks", "1048577"]),
            ("huge", ["--bwt-aux-walks=4294967296"]),
            ("negative", ["--bwt-aux-walks=-1"]),
            ("non_numeric", ["--bwt-aux-walks=abc"]),
            ("equals_with_equals", ["--bwt-aux-walks==4096"]),
        ):
            out = td / f"bad-{label}.anv"
            if out.exists():
                out.unlink()
            r = encode(out, extra)
            check(r.returncode != 0, f"{label}: rejected with a non-zero exit code")
            check("--bwt-aux-walks must be an integer in 1..1048576" in r.stderr,
                  f"{label}: rejected with the explicit range error")
            check(not out.exists(), f"{label}: no output file was produced")

        # A missing value must be rejected too.
        r = encode(td / "missing.anv", ["--bwt-aux-walks"])
        check(r.returncode != 0, "missing value: rejected with a non-zero exit code")
        check("--bwt-aux-walks requires a value" in r.stderr,
              "missing value: rejected with the explicit missing-value error")

        # --bwt-subblock must accept both spellings.
        s1 = td / "s1.anv"
        s2 = td / "s2.anv"
        r1 = encode(s1, ["--bwt-subblock", "8388608"])
        r2 = encode(s2, ["--bwt-subblock=8388608"])
        check(r1.returncode == 0 and r2.returncode == 0,
              "--bwt-subblock accepts both spellings")
        if r1.returncode == 0 and r2.returncode == 0:
            check(sha256(s1) == sha256(s2),
                  "spaced and '=' spellings of --bwt-subblock produce identical bytes")

    if failures:
        print(f"\nFAILED {len(failures)} check(s):")
        for f in failures:
            print(f"  - {f}")
        return 1
    print("\nOK: --bwt-aux-walks / --bwt-subblock CLI contract verified")
    return 0


if __name__ == "__main__":
    sys.exit(main())
