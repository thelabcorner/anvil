#!/usr/bin/env python3
"""Prepare and rule ANVIL same-job reference-cost experiments.

This tool is for remote CI evaluation only. It does not select or alter codec
behavior. It prepares exact payloads, emits deterministic decode command sets,
measures per-process decode RSS on request, and combines paired timing evidence
into a conservative FRONT-GAP cost ruling.

A PASS here is intentionally *not* a FRONT-CROSSING: encode and decoder-only
binary-size axes remain separate gates.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def run(argv: list[str], *, stdout_path: Path | None = None) -> None:
    out: Any = subprocess.DEVNULL
    handle = None
    try:
        if stdout_path is not None:
            stdout_path.parent.mkdir(parents=True, exist_ok=True)
            handle = stdout_path.open("wb")
            out = handle
        p = subprocess.run(
            argv,
            stdout=out,
            stderr=subprocess.PIPE,
            check=False,
        )
        if p.returncode:
            raise RuntimeError(
                f"command failed rc={p.returncode}: {argv!r}\n"
                + p.stderr.decode(errors="replace")[-4000:]
            )
    finally:
        if handle is not None:
            handle.close()


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


def write_script(path: Path, commands: list[list[str]]) -> None:
    lines = ["#!/usr/bin/env bash", "set -euo pipefail"]
    lines.extend(shlex.join(cmd) for cmd in commands)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    path.chmod(0o755)


def verify_outputs(files: list[Path], commands: list[list[str]], outputs: list[Path]) -> None:
    if len(files) != len(commands) or len(files) != len(outputs):
        raise RuntimeError("verification manifest length mismatch")
    for src, cmd, out in zip(files, commands, outputs):
        run(cmd)
        if not out.is_file():
            raise RuntimeError(f"decoder did not create output: {out}")
        if out.stat().st_size != src.stat().st_size or sha256(out) != sha256(src):
            raise RuntimeError(f"roundtrip mismatch: {src.name} via {cmd[0]}")


def prepare(args: argparse.Namespace) -> None:
    root = args.root.resolve()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    anvil = args.anvil.resolve()
    brotli = args.brotli.resolve()
    xz = shutil.which("xz")
    if not anvil.is_file() or not brotli.is_file() or not xz:
        raise RuntimeError("missing ANVIL, brotli_lw, or xz")

    files = canonical_files(args.corpus, root)
    pack_root = out / "packed"
    dec_root = out / "decoded"
    for arm in ("anvil-aux", "brotli", "xz"):
        (pack_root / arm).mkdir(parents=True, exist_ok=True)
        (dec_root / arm).mkdir(parents=True, exist_ok=True)

    commands: dict[str, list[list[str]]] = {
        "anvil-aux": [],
        "brotli": [],
        "xz": [],
    }
    outputs: dict[str, list[Path]] = {
        "anvil-aux": [],
        "brotli": [],
        "xz": [],
    }
    byte_rows: list[dict[str, Any]] = []

    for src in files:
        name = src.name
        n = src.stat().st_size
        digest = sha256(src)

        cand = pack_root / "anvil-aux" / f"{name}.anv"
        run([
            str(anvil), "c", str(src), str(cand),
            "--parse=ratio", "--ratio-backend=auto",
            "--ratio-context=off", "--ratio-lines=off",
            "--bwt-aux=on", "--quiet",
        ])
        cand_out = dec_root / "anvil-aux" / name
        cand_cmd = [str(anvil), "d", str(cand), str(cand_out), "--quiet"]

        br = pack_root / "brotli" / f"{name}.br"
        run([str(brotli), "c", str(src), str(br)])
        br_out = dec_root / "brotli" / name
        br_cmd = [str(brotli), "d", str(br), str(br_out), str(n)]

        xp = pack_root / "xz" / f"{name}.xz"
        run([str(xz), "-9e", "-c", str(src)], stdout_path=xp)
        # xz -d -k -f writes next to the compressed file after removing .xz.
        xz_out = xp.with_suffix("")
        xz_cmd = [str(xz), "-d", "-k", "-f", str(xp)]

        for arm, packed, cmd, decoded in (
            ("anvil-aux", cand, cand_cmd, cand_out),
            ("brotli", br, br_cmd, br_out),
            ("xz", xp, xz_cmd, xz_out),
        ):
            commands[arm].append(cmd)
            outputs[arm].append(decoded)
            byte_rows.append({
                "corpus": args.corpus,
                "file": name,
                "input_bytes": n,
                "source_sha256": digest,
                "codec": arm,
                "compressed_bytes": packed.stat().st_size,
                "ratio": f"{packed.stat().st_size / n:.9f}",
            })

    # Correctness is established before any timed pair is consumed.
    for arm in ("anvil-aux", "brotli", "xz"):
        verify_outputs(files, commands[arm], outputs[arm])

    fields = [
        "corpus", "file", "input_bytes", "source_sha256",
        "codec", "compressed_bytes", "ratio",
    ]
    with (out / "bytes.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(byte_rows)

    total_by_codec: dict[str, int] = {}
    for row in byte_rows:
        total_by_codec[row["codec"]] = (
            total_by_codec.get(row["codec"], 0) + int(row["compressed_bytes"])
        )
    source_total = sum(p.stat().st_size for p in files)
    if args.expected_candidate_total is not None:
        got = total_by_codec["anvil-aux"]
        if got != args.expected_candidate_total:
            raise RuntimeError(
                f"candidate byte identity failed: {got} != {args.expected_candidate_total}"
            )

    manifests: dict[str, str] = {}
    scripts: dict[str, str] = {}
    for arm in ("anvil-aux", "brotli", "xz"):
        mp = out / f"{arm}-commands.json"
        mp.write_text(
            json.dumps(
                {
                    "schema": 1,
                    "arm": arm,
                    "commands": commands[arm],
                    "outputs": [str(p) for p in outputs[arm]],
                },
                indent=2,
            ) + "\n",
            encoding="utf-8",
        )
        sp = out / f"decode-{arm}.sh"
        write_script(sp, commands[arm])
        manifests[arm] = str(mp)
        scripts[arm] = str(sp)

    summary = {
        "schema": 1,
        "corpus": args.corpus,
        "source_bytes": source_total,
        "file_count": len(files),
        "compressed_bytes": total_by_codec,
        "scripts": scripts,
        "command_manifests": manifests,
        "roundtrip": "OK",
    }
    (out / "prepare-summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(json.dumps(summary, indent=2, sort_keys=True))


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


def rss(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    rows = []
    with tempfile.TemporaryDirectory(prefix="anvil-rss-") as td0:
        td = Path(td0)
        for manifest in args.manifest:
            payload = json.loads(manifest.read_text(encoding="utf-8"))
            arm = str(payload["arm"])
            commands = payload["commands"]
            samples = []
            for i, cmd in enumerate(commands):
                kib = measure_one_rss([str(x) for x in cmd], td)
                samples.append(kib)
                rows.append({"arm": arm, "index": i, "max_rss_kib": kib})
            print(f"{arm}: max_rss={max(samples):,} KiB files={len(samples)}")

    with (out / "rss.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["arm", "index", "max_rss_kib"])
        w.writeheader()
        w.writerows(rows)

    grouped: dict[str, list[int]] = {}
    for row in rows:
        grouped.setdefault(str(row["arm"]), []).append(int(row["max_rss_kib"]))
    result = {
        "schema": 1,
        "max_rss_kib": {arm: max(vals) for arm, vals in grouped.items()},
        "samples": rows,
    }
    (out / "rss.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def summarize(args: argparse.Namespace) -> None:
    prep = load_json(args.prepare)
    rss_data = load_json(args.rss)
    xz_pair = load_json(args.xz_pair)
    br_pair = load_json(args.brotli_pair)
    null_pair = load_json(args.null_pair)

    source = int(prep["source_bytes"])
    cb = {k: int(v) for k, v in prep["compressed_bytes"].items()}
    rss_kib = {k: int(v) for k, v in rss_data["max_rss_kib"].items()}

    def pair_metrics(d: dict[str, Any]) -> dict[str, float]:
        ratio = float(d["paired_ratio_candidate_over_control"])
        lo, hi = [float(x) for x in d["paired_ratio_ci95"]]
        ct = float(d["control"]["median_s"])
        at = float(d["candidate"]["median_s"])
        return {
            "ratio": ratio,
            "ci_lo": lo,
            "ci_hi": hi,
            "ref_mbps": source / 1e6 / ct,
            "anvil_mbps": source / 1e6 / at,
            "control_s": ct,
            "candidate_s": at,
        }

    xz = pair_metrics(xz_pair)
    br = pair_metrics(br_pair)
    null_lo, null_hi = [float(x) for x in null_pair["paired_ratio_ci95"]]
    null_ok = (
        bool(null_pair.get("timing_valid"))
        and null_lo <= 1.0 <= null_hi
        and not bool(null_pair.get("clears_speed_gate"))
    )

    timing_ok = (
        bool(xz_pair.get("timing_valid"))
        and bool(br_pair.get("timing_valid"))
        and null_ok
    )
    bytes_beat_xz = cb["anvil-aux"] < cb["xz"]
    bytes_beat_brotli = cb["anvil-aux"] < cb["brotli"]

    # Conservative use of the paired CI: the upper 95% timing-ratio bound must
    # remain within the historic 2x "acceptable" decode cost margin.
    decode_xz_2x = xz["ci_hi"] <= 2.0
    decode_br_2x = br["ci_hi"] <= 2.0

    mem_xz_ratio = rss_kib["anvil-aux"] / rss_kib["xz"]
    mem_br_ratio = rss_kib["anvil-aux"] / rss_kib["brotli"]
    memory_xz_2x = mem_xz_ratio <= 2.0
    memory_br_2x = mem_br_ratio <= 2.0

    if not timing_ok:
        ruling = "TIMING_BLOCKED"
    elif not bytes_beat_xz:
        ruling = "NO_BINDING_BYTE_WIN"
    elif not (decode_xz_2x and decode_br_2x and memory_xz_2x and memory_br_2x):
        ruling = "FRONT-GAP_COST"
    else:
        ruling = "COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE"

    result = {
        "schema": 1,
        "corpus": prep["corpus"],
        "ruling": ruling,
        "source_bytes": source,
        "compressed_bytes": cb,
        "bytes_beat_xz": bytes_beat_xz,
        "bytes_beat_brotli": bytes_beat_brotli,
        "decode": {"xz": xz, "brotli": br},
        "rss_kib": rss_kib,
        "memory_ratio": {"xz": mem_xz_ratio, "brotli": mem_br_ratio},
        "gates": {
            "timing_valid": timing_ok,
            "null_ci_spans_one": null_ok,
            "decode_within_2x_xz": decode_xz_2x,
            "decode_within_2x_brotli": decode_br_2x,
            "memory_within_2x_xz": memory_xz_2x,
            "memory_within_2x_brotli": memory_br_2x,
        },
        "scope_note": (
            "This gate can falsify a FRONT-CROSSING on decode/memory cost. "
            "Passing it does not establish a crossing because encode and "
            "decoder-only binary-size axes are not ruled here."
        ),
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    md = [
        f"# I10-1A reference-cost gate — {prep['corpus']}",
        "",
        f"**Ruling: {ruling}**",
        "",
        "| Codec | Compressed bytes | Ratio | Decode MB/s | Peak RSS MiB |",
        "|---|---:|---:|---:|---:|",
        f"| ANVIL aux | {cb['anvil-aux']:,} | {cb['anvil-aux']/source:.9f} | {xz['anvil_mbps']:.3f} | {rss_kib['anvil-aux']/1024:.1f} |",
        f"| xz -9e | {cb['xz']:,} | {cb['xz']/source:.9f} | {xz['ref_mbps']:.3f} | {rss_kib['xz']/1024:.1f} |",
        f"| Brotli q11/lw30 | {cb['brotli']:,} | {cb['brotli']/source:.9f} | {br['ref_mbps']:.3f} | {rss_kib['brotli']/1024:.1f} |",
        "",
        "## Paired decode ratios — ANVIL / reference (lower is faster)",
        "",
        f"- vs xz: {xz['ratio']:.4f}, 95% CI [{xz['ci_lo']:.4f}, {xz['ci_hi']:.4f}]",
        f"- vs Brotli: {br['ratio']:.4f}, 95% CI [{br['ci_lo']:.4f}, {br['ci_hi']:.4f}]",
        f"- A/A null CI: [{null_lo:.4f}, {null_hi:.4f}] — {'PASS' if null_ok else 'BLOCKED'}",
        "",
        "## Existing cost-margin gates",
        "",
        f"- bytes < xz: {'PASS' if bytes_beat_xz else 'FAIL'}",
        f"- decode <= 2x xz (CI upper): {'PASS' if decode_xz_2x else 'FAIL'}",
        f"- decode <= 2x Brotli (CI upper): {'PASS' if decode_br_2x else 'FAIL'}",
        f"- peak RSS <= 2x xz: {'PASS' if memory_xz_2x else 'FAIL'} ({mem_xz_ratio:.3f}x)",
        f"- peak RSS <= 2x Brotli: {'PASS' if memory_br_2x else 'FAIL'} ({mem_br_ratio:.3f}x)",
        "",
        result["scope_note"],
        "",
    ]
    md_path = args.out.with_suffix(".md")
    md_path.write_text("\n".join(md), encoding="utf-8")
    print("\n".join(md))


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("prepare")
    p.add_argument("--corpus", choices=("silesia", "enwik8"), required=True)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--brotli", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--expected-candidate-total", type=int)
    p.set_defaults(func=prepare)

    p = sub.add_parser("rss")
    p.add_argument("--manifest", type=Path, action="append", required=True)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=rss)

    p = sub.add_parser("summarize")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--rss", type=Path, required=True)
    p.add_argument("--xz-pair", type=Path, required=True)
    p.add_argument("--brotli-pair", type=Path, required=True)
    p.add_argument("--null-pair", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=summarize)
    return ap


def main() -> None:
    args = build_parser().parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
