#!/usr/bin/env python3
"""I20-W4 decode working-set attribution, measurement and composable peak model.

This tool does NOT change codec behaviour and does NOT select a representation.
It produces the C4 evidence set:

  manifest  build per-arm (baseline / candidate) decode command manifests for a
            corpus, so both arms decode the SAME payloads in the SAME job
  rss       measure per-FILE peak decode RSS with /usr/bin/time -f %M (the same
            measurement the repo's arbiter tools/reference_cost_gate.py uses)
  model     theoretical peak-RSS floor per file and per BWT sub-block cap, from
            the live-set model
  report    join the measured RSS with the model into the composable table
            (baseline vs candidate, per cap) that the I20 coordinator needs to
            combine C4 (zero-byte working-set levers) with C3 (sub-block cap)

Live-set model (whole-file BWT, backend 2, ratio mode 17)
---------------------------------------------------------
The canonical ratio route uses one 128 MiB ratio block, so a file of n decoded
bytes is ONE whole-file BWT. At the inverse-BWT instant the decoder holds:

    c        compressed input buffer (read_file loads the whole .anv file)
    n        BWT string produced by the postcoder
    4n       libsais int32 PSI/work array  (P[bucket2[w]++] = i)
    n        reconstructed-output buffer written by libsais_unbwt(_aux)
    ---------------------------------------------
    6n + c   peak

Levers this tool can price (all at EXACTLY zero wire cost):
  L2_inplace         the reconstructed output is written over the BWT string
                     (the libsais LF walk never reads T)            -> -n
  L1_release_input   the compressed input is released once the block payload
                     has been consumed                              -> -c
  L2stream_output    the decoded bytes are streamed to the output file
                     instead of being accumulated in RAM            -> -(2n)
                                                                  in the capped
                                                                  regime

With a BWT sub-block cap C the outer 0xFF frame splits the block into
ceil(n/C) pieces. The peak then occurs in the LAST piece, where the two output
accumulators (the decompress-level accumulator and the ratio-level
sub-block accumulator) already hold n - piece bytes each:

    c + 2*(n - piece) + 6*piece        (piece = n - (K-1)*C)

Evidence classes: every number this tool prints is either MEASURED (read from
an artifact produced on a GitHub Actions run) or DERIVED (exact arithmetic over
such numbers). It never invents a number.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import os
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

MIB = 1024.0 * 1024.0

# Live-set coefficients (bytes of resident memory per unit of n).
BWT_STRING = 1      # the BWT string produced by the postcoder
LIBSAIS_PSI = 4     # libsais int32 work array (n+1 entries)
BLOCK_OUTPUT = 1    # the buffer libsais writes the reconstructed string into


def canonical_files(corpus: str, root: Path) -> list[Path]:
    if corpus == "enwik8":
        p = root / "enwik8" / "enwik8"
        if not p.is_file():
            raise RuntimeError(f"missing canonical enwik8: {p}")
        return [p]
    if corpus == "silesia":
        d = root / "silesia"
        names = [
            "dickens", "mozilla", "mr", "nci", "ooffice", "osdb",
            "reymont", "samba", "sao", "webster", "x-ray", "xml",
        ]
        files = [d / name for name in names]
        missing = [str(p) for p in files if not p.is_file()]
        if missing:
            raise RuntimeError(f"missing canonical Silesia files: {missing}")
        return files
    raise RuntimeError(f"unsupported corpus: {corpus}")


def parse_cap(text: str) -> int | None:
    text = text.strip().lower()
    if text in ("", "none", "0"):
        return None
    mult = 1
    if text.endswith("k"):
        mult = 1024
        text = text[:-1]
    elif text.endswith("m"):
        mult = 1024 * 1024
        text = text[:-1]
    elif text.endswith("g"):
        mult = 1024 * 1024 * 1024
        text = text[:-1]
    return int(text) * mult


def lever_set(text: str) -> set[str]:
    out: set[str] = set()
    for part in text.split(","):
        part = part.strip()
        if not part:
            continue
        if part not in ("L1_release_input", "L2_inplace", "L2stream_output"):
            raise RuntimeError(f"unknown lever: {part}")
        out.add(part)
    return out


def live_set(n: int, c: int, cap: int | None, levers: set[str]) -> dict[str, Any]:
    """Exact arithmetic model of the decoder live set at the inverse-BWT peak."""
    if cap is None or cap >= n:
        piece = n
        acc_dec = 0          # decompress-level accumulator resident at the peak
        acc_ratio = 0        # ratio-level sub-block accumulator resident at the peak
    else:
        k = 1 + (n - 1) // cap
        piece = n - (k - 1) * cap
        acc_dec = n - piece
        acc_ratio = n - piece
    terms: dict[str, int] = {}
    terms["compressed_input"] = 0 if "L1_release_input" in levers else c
    terms["bwt_string"] = BWT_STRING * piece
    terms["libsais_psi"] = LIBSAIS_PSI * piece
    terms["block_output"] = 0 if "L2_inplace" in levers else BLOCK_OUTPUT * piece
    terms["accumulator_decompress"] = 0 if "L2stream_output" in levers else acc_dec
    terms["accumulator_ratio"] = 0 if "L2stream_output" in levers else acc_ratio
    total = sum(terms.values())
    return {
        "n": n,
        "c": c,
        "cap": cap,
        "pieces": 1 if (cap is None or cap >= n) else 1 + (n - 1) // cap,
        "piece": piece,
        "terms": terms,
        "total": total,
    }


# --------------------------------------------------------------------------
# manifest
# --------------------------------------------------------------------------

def cmd_manifest(args: argparse.Namespace) -> None:
    root = Path(args.root).resolve()
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    files = canonical_files(args.corpus, root)
    arms: dict[str, Any] = {}
    for arm, exe in (("baseline", args.baseline), ("candidate", args.candidate)):
        if not exe:
            continue
        exe = str(Path(exe).resolve())
        commands: list[list[str]] = []
        outputs: list[str] = []
        names: list[str] = []
        for src in files:
            # The payloads are arm-independent on purpose: the change under test is
            # DECODE-ONLY, so the frozen wire bytes are identical for both arms and
            # both binaries must decode exactly the same payload files.
            payload = Path(args.payload_dir) / f"{src.name}.anv"
            if not payload.is_file():
                raise RuntimeError(f"missing payload: {payload}")
            dec = out / f"decoded-{arm}" / src.name
            dec.parent.mkdir(parents=True, exist_ok=True)
            commands.append([exe, "d", str(payload), str(dec), "--quiet"])
            outputs.append(str(dec))
            names.append(src.name)
        mp = out / f"{args.corpus}-{arm}-commands.json"
        mp.write_text(
            json.dumps(
                {
                    "schema": 1,
                    "arm": arm,
                    "corpus": args.corpus,
                    "files": names,
                    "commands": commands,
                    "outputs": outputs,
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
        # A single shell script that decodes every file: the paired timing
        # driver runs one argv per arm, not one per file.
        script = out / f"decode-{arm}.sh"
        lines = ["#!/usr/bin/env bash", "set -euo pipefail"]
        for cmd in commands:
            lines.append(shlex.join(cmd))
        script.write_text("\n".join(lines) + "\n", encoding="utf-8")
        if os.name != "nt":
            script.chmod(0o755)
        arms[arm] = {"manifest": str(mp), "script": str(script)}
    print(json.dumps({"corpus": args.corpus, "arms": arms}, indent=2, sort_keys=True))


# --------------------------------------------------------------------------
# rss
# --------------------------------------------------------------------------

def measure_one_rss(cmd: list[str], tmp: Path) -> int:
    time_bin = Path("/usr/bin/time")
    if not time_bin.is_file():
        raise RuntimeError("/usr/bin/time is required for RSS measurement")
    rss_file = tmp / "rss.txt"
    if rss_file.exists():
        rss_file.unlink()
    p = subprocess.run(
        [str(time_bin), "-f", "%M", "-o", str(rss_file), "--", *cmd],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        check=False,
    )
    if p.returncode:
        raise RuntimeError(
            f"RSS command failed rc={p.returncode}: {cmd!r}\n"
            + p.stderr.decode(errors="replace")[-4000:]
        )
    return int(rss_file.read_text(encoding="utf-8").strip())


def cmd_rss(args: argparse.Namespace) -> None:
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    rows: list[dict[str, Any]] = []
    with tempfile.TemporaryDirectory(prefix="i20-w4-rss-") as td0:
        tmp = Path(td0)
        for manifest in args.manifest:
            payload = json.loads(Path(manifest).read_text(encoding="utf-8"))
            arm = str(payload["arm"])
            files = payload.get("files") or [str(i) for i in range(len(payload["commands"]))]
            for i, cmd in enumerate(payload["commands"]):
                kib = measure_one_rss([str(x) for x in cmd], tmp)
                rows.append(
                    {
                        "arm": arm,
                        "corpus": payload.get("corpus", ""),
                        "file": files[i],
                        "index": i,
                        "max_rss_kib": kib,
                    }
                )
                print(f"{arm} {files[i]} max_rss={kib} KiB", flush=True)
    fields = ["arm", "corpus", "file", "index", "max_rss_kib"]
    with (out / "rss-by-file.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)
    grouped: dict[str, int] = {}
    for r in rows:
        key = f"{r['arm']}/{r['corpus']}"
        grouped[key] = max(grouped.get(key, 0), int(r["max_rss_kib"]))
    (out / "rss-by-arm.json").write_text(
        json.dumps({"schema": 1, "max_rss_kib": grouped}, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(json.dumps(grouped, indent=2, sort_keys=True))


# --------------------------------------------------------------------------
# model
# --------------------------------------------------------------------------

def read_bytes(path: Path | None, codec: str, want_aux: str | None = None) -> dict[tuple[str, str], dict[str, int]]:
    """(corpus,file) -> {input_bytes, compressed_bytes} for one codec/aux selection."""
    out: dict[tuple[str, str], dict[str, int]] = {}
    if path is None or not Path(path).is_file():
        return out
    for row in csv.DictReader(Path(path).open(encoding="utf-8")):
        if "codec" in row and row["codec"] != codec:
            continue
        if want_aux is not None and row.get("bwt_aux", want_aux) != want_aux:
            continue
        out[(row["corpus"], row["file"])] = {
            "input_bytes": int(row["input_bytes"]),
            "compressed_bytes": int(row["compressed_bytes"]),
        }
    return out


def cmd_model(args: argparse.Namespace) -> None:
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    caps: list[int | None] = [parse_cap(raw) for raw in args.caps.split(",")]
    lever_sets = {
        "baseline": set(),
        "L2": {"L2_inplace"},
        "L1": {"L1_release_input"},
        "L2+L1": {"L2_inplace", "L1_release_input"},
        "L2+L1+stream": {"L2_inplace", "L1_release_input", "L2stream_output"},
    }
    base = read_bytes(Path(args.bytes_csv), args.codec, "on")
    capped = read_bytes(Path(args.cap_bytes_csv) if args.cap_bytes_csv else None, args.codec)
    rows: list[dict[str, Any]] = []
    for (corpus, name), info in sorted(base.items()):
        n = info["input_bytes"]
        for cap in caps:
            c = info["compressed_bytes"]
            if cap is not None:
                c = capped.get((corpus, name), {}).get("compressed_bytes", c)
            for vname, levers in lever_sets.items():
                m = live_set(n, c, cap, levers)
                rows.append(
                    {
                        "corpus": corpus,
                        "file": name,
                        "input_bytes": n,
                        "compressed_bytes": c,
                        "cap": "uncapped" if cap is None else cap,
                        "variant": vname,
                        "levers": "|".join(sorted(levers)),
                        "pieces": m["pieces"],
                        "piece_bytes": m["piece"],
                        "floor_bytes": m["total"],
                        "floor_mib": round(m["total"] / MIB, 3),
                        **{f"term_{k}": v for k, v in m["terms"].items()},
                    }
                )
    fields = list(rows[0].keys()) if rows else []
    with (out / "peak-floor-model.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)
    print(f"wrote {out / 'peak-floor-model.csv'} rows={len(rows)}")


# --------------------------------------------------------------------------
# report
# --------------------------------------------------------------------------

def cmd_report(args: argparse.Namespace) -> None:
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    sizes = read_bytes(Path(args.bytes_csv), args.codec, "on")
    capped = read_bytes(Path(args.cap_bytes_csv) if args.cap_bytes_csv else None, args.codec)
    measured: dict[tuple[str, str], int] = {}
    for csv_path in args.rss_csv:
        for r in csv.DictReader(Path(csv_path).open(encoding="utf-8")):
            measured[(f"{r['arm']}/{r['corpus']}", r["file"])] = int(r["max_rss_kib"])
    caps: list[int | None] = [parse_cap(x) for x in args.caps.split(",")]
    lever_sets = {
        "baseline": set(),
        "L2": {"L2_inplace"},
        "L2+L1": {"L2_inplace", "L1_release_input"},
    }
    lines: list[str] = []
    lines.append("# I20-W4 decode working-set: composable peak-RSS table")
    lines.append("")
    lines.append(
        "MEASURED columns come from the Actions artifact (`rss-by-file.csv`, "
        "/usr/bin/time -f %M per decode process). DERIVED columns are exact "
        "arithmetic over the MEASURED compressed sizes and the live-set model."
    )
    lines.append("")
    ref_xz = float(args.xz_mib)
    ref_br = float(args.brotli_mib)
    lines.append(
        f"Reference peaks (MEASURED, prior run 35927623136): xz -9e = {ref_xz:.1f} MiB, "
        f"Brotli q11/lgwin30 = {ref_br:.1f} MiB."
    )
    lines.append("")
    for cap in caps:
        capname = "uncapped (128 MiB default)" if cap is None else f"cap {cap // (1024*1024)} MiB"
        lines.append(f"## {capname}")
        lines.append("")
        lines.append(
            "| corpus | file | n | c | arm | peak MiB (MEASURED) | floor baseline MiB (DERIVED) "
            "| floor +C4 levers MiB (DERIVED) | vs xz | vs Brotli |"
        )
        lines.append("|---|---|---:|---:|---|---:|---:|---:|---:|---:|")
        for (corpus, name), info in sorted(sizes.items()):
            n = info["input_bytes"]
            c = info["compressed_bytes"]
            if cap is not None:
                c = capped.get((corpus, name), {}).get("compressed_bytes", c)
            for arm in ("baseline", "candidate"):
                suffix = "" if cap is None else f"-cap{cap}"
                key = (f"{arm}{suffix}/{corpus}", name)
                if key not in measured:
                    continue
                mib = measured[key] / 1024.0
                fb = live_set(n, c, cap, lever_sets["baseline"])["total"] / MIB
                fc = live_set(n, c, cap, lever_sets["L2+L1"])["total"] / MIB
                lines.append(
                    f"| {corpus} | {name} | {n:,} | {c:,} | {arm} | {mib:.1f} | {fb:.1f} | {fc:.1f} "
                    f"| {mib/ref_xz:.3f} | {mib/ref_br:.3f} |"
                )
        lines.append("")
    (out / "composable-peak-table.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("manifest")
    p.add_argument("--corpus", choices=("silesia", "enwik8"), required=True)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--payload-dir", type=Path, required=True)
    p.add_argument("--baseline", default="")
    p.add_argument("--candidate", default="")
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_manifest)

    p = sub.add_parser("rss")
    p.add_argument("--manifest", type=Path, action="append", required=True)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_rss)

    p = sub.add_parser("model")
    p.add_argument("--bytes-csv", type=Path, required=True)
    p.add_argument("--cap-bytes-csv", type=Path)
    p.add_argument("--codec", default="anvil-aux")
    p.add_argument("--caps", default="none,16M,4M")
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_model)

    p = sub.add_parser("report")
    p.add_argument("--bytes-csv", type=Path, required=True)
    p.add_argument("--cap-bytes-csv", type=Path)
    p.add_argument("--rss-csv", type=Path, action="append", required=True)
    p.add_argument("--codec", default="anvil-aux")
    p.add_argument("--caps", default="none,16M,4M")
    p.add_argument("--xz-mib", type=float, default=54.3)
    p.add_argument("--brotli-mib", type=float, default=124.5)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_report)
    return ap


def main() -> None:
    args = build_parser().parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
