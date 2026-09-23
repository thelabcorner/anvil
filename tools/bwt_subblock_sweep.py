#!/usr/bin/env python3
"""Remote-only I10 BWT subblock bytes/decode/RSS Pareto sweep.

This tool does not alter codec behavior. It exercises the existing direct
Brotli and direct-BWT ratio representations with --bwt-aux=on and a grid of
--bwt-subblock caps, verifies roundtrips, records exact bytes and subblock
counts, emits selected-portfolio decode scripts, measures per-process RSS, and
summarizes paired timing evidence produced by tools/paired_bench.py.

The auto portfolio is reconstructed exactly from direct payloads because the
tested configuration disables ratio transforms and encode_ratio_block() selects
the strictly smallest backend payload (Brotli is the stable tie winner).
The frozen 128 MiB aggregate is checked against the known I10-1A byte total.
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
import tempfile
from pathlib import Path
from typing import Any


SILESIA = (
    "dickens", "mozilla", "mr", "nci", "ooffice", "osdb",
    "reymont", "samba", "sao", "webster", "x-ray", "xml",
)

EXPECTED_CONTROL_BYTES = {
    "silesia": 46_466_339,
    "enwik8": 23_537_422,
}

CONTROL_CAP_MIB = 128


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def read_uvar(blob: bytes, pos: int) -> tuple[int, int]:
    value = 0
    shift = 0
    for _ in range(10):
        if pos >= len(blob):
            raise ValueError("truncated uvar")
        b = blob[pos]
        pos += 1
        value |= (b & 0x7F) << shift
        if not (b & 0x80):
            return value, pos
        shift += 7
    raise ValueError("uvar overflow")


def ratio_info(path: Path) -> dict[str, Any]:
    """Inspect a single rev-2 mode-17 whole-file ratio payload."""
    blob = path.read_bytes()
    if len(blob) < 5 or blob[:4] != b"ANV0" or blob[4] != 2:
        raise ValueError(f"{path}: not ANVIL rev2")
    p = 5
    block_size, p = read_uvar(blob, p)
    total, p = read_uvar(blob, p)
    blen, p = read_uvar(blob, p)
    if p >= len(blob):
        raise ValueError(f"{path}: truncated block mode")
    mode = blob[p]
    p += 1
    plen, p = read_uvar(blob, p)
    if p + 4 > len(blob):
        raise ValueError(f"{path}: truncated block crc")
    p += 4
    payload = p
    if mode != 17 or payload + plen != len(blob):
        raise ValueError(f"{path}: expected one whole-file mode-17 block")
    if plen < 3:
        raise ValueError(f"{path}: short ratio payload")
    transform = blob[payload]
    backend = blob[payload + 1]
    xlen, inner = read_uvar(blob, payload + 2)
    if transform != 0:
        raise ValueError(f"{path}: expected direct transform, got {transform}")

    out: dict[str, Any] = {
        "block_size": block_size,
        "total": total,
        "block_len": blen,
        "payload_len": plen,
        "transform": transform,
        "backend": backend,
        "transformed_len": xlen,
        "subblocks": None,
    }
    if backend == 2:
        if inner >= len(blob):
            raise ValueError(f"{path}: missing BWT backend payload")
        if blob[inner] == 0xFF:
            nsub, _ = read_uvar(blob, inner + 1)
            out["subblocks"] = nsub
        else:
            out["subblocks"] = 1
    return out


def canonical_files(corpus: str, root: Path) -> list[Path]:
    if corpus == "enwik8":
        p = root / "enwik8" / "enwik8"
        if not p.is_file():
            raise RuntimeError(f"missing canonical enwik8: {p}")
        return [p]
    if corpus == "silesia":
        d = root / "silesia"
        files = [d / name for name in SILESIA]
        missing = [str(p) for p in files if not p.is_file()]
        if missing:
            raise RuntimeError(f"missing canonical Silesia files: {missing}")
        return files
    raise RuntimeError(f"unsupported corpus: {corpus}")


def parse_caps(text: str) -> list[int]:
    values = sorted({int(x.strip()) for x in text.split(",") if x.strip()})
    if not values or any(v <= 0 for v in values):
        raise argparse.ArgumentTypeError("caps must be positive comma-separated MiB values")
    if CONTROL_CAP_MIB not in values:
        raise argparse.ArgumentTypeError(f"caps must include {CONTROL_CAP_MIB} MiB control")
    return values


def run(argv: list[str], *, stdout_path: Path | None = None) -> None:
    handle = None
    out: Any = subprocess.DEVNULL
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


def run_time_rss(argv: list[str], *, stdout_path: Path | None = None) -> tuple[float, int]:
    """Run once under GNU time; return elapsed seconds and max RSS KiB."""
    time_bin = Path("/usr/bin/time")
    if not time_bin.is_file():
        raise RuntimeError("/usr/bin/time is required")
    with tempfile.TemporaryDirectory(prefix="anvil-subblock-time-") as td0:
        stamp = Path(td0) / "time.txt"
        handle = None
        out: Any = subprocess.DEVNULL
        try:
            if stdout_path is not None:
                stdout_path.parent.mkdir(parents=True, exist_ok=True)
                handle = stdout_path.open("wb")
                out = handle
            p = subprocess.run(
                [
                    str(time_bin), "-f", "%e %M", "-o", str(stamp),
                    "--", *argv,
                ],
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
        parts = stamp.read_text(encoding="utf-8").strip().split()
        if len(parts) != 2:
            raise RuntimeError(f"bad GNU time output: {parts!r}")
        return float(parts[0]), int(parts[1])


def write_script(path: Path, commands: list[list[str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = ["#!/usr/bin/env bash", "set -euo pipefail"]
    lines.extend(shlex.join(cmd) for cmd in commands)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    path.chmod(0o755)


def encode_direct(
    anvil: Path,
    src: Path,
    packed: Path,
    backend: str,
    *,
    cap_mib: int | None = None,
) -> tuple[float, int]:
    packed.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        str(anvil), "c", str(src), str(packed),
        "--parse=ratio",
        f"--ratio-backend={backend}",
        "--ratio-context=off",
        "--ratio-lines=off",
    ]
    if backend == "bwt":
        if cap_mib is None:
            raise RuntimeError("BWT encode requires cap")
        cmd += [
            "--bwt-aux=on",
            f"--bwt-subblock={cap_mib * 1024 * 1024}",
        ]
    cmd.append("--quiet")
    return run_time_rss(cmd)


def prepare(args: argparse.Namespace) -> None:
    root = args.root.resolve()
    anvil = args.anvil.resolve()
    out = args.out.resolve()
    caps = parse_caps(args.caps_mib)
    files = canonical_files(args.corpus, root)
    if not anvil.is_file():
        raise RuntimeError(f"missing ANVIL executable: {anvil}")

    out.mkdir(parents=True, exist_ok=True)
    packed_root = out / "packed"
    decoded_root = out / "verified"
    timed_root = out / "timed"
    for cap in caps:
        (timed_root / f"cap-{cap}").mkdir(parents=True, exist_ok=True)

    source_rows: list[dict[str, Any]] = []
    brotli_meta: dict[str, dict[str, Any]] = {}
    bwt_meta: dict[tuple[str, int], dict[str, Any]] = {}

    # Direct Brotli is cap-independent and is encoded once per file.
    for src in files:
        name = src.name
        src_digest = sha256(src)
        bp = packed_root / "brotli" / f"{name}.anv"
        enc_s, enc_rss = encode_direct(anvil, src, bp, "brotli")
        info = ratio_info(bp)
        if info["backend"] != 1:
            raise RuntimeError(f"{name}: direct Brotli emitted backend {info['backend']}")
        brotli_meta[name] = {
            "path": str(bp),
            "bytes": bp.stat().st_size,
            "sha256": sha256(bp),
            "encode_s": enc_s,
            "encode_rss_kib": enc_rss,
            "ratio_info": info,
        }
        source_rows.append({
            "file": name,
            "input_bytes": src.stat().st_size,
            "source_sha256": src_digest,
        })

    # For a cap >= file size, ratio_backend_encode() emits the same bare,
    # unsplit BWT payload. Encode that representation once and reuse it.
    for src in files:
        name = src.name
        n = src.stat().st_size
        unsplit_cap = next((cap for cap in caps if cap * 1024 * 1024 >= n), None)
        encoded_by_effective: dict[int | str, dict[str, Any]] = {}

        for cap in caps:
            split = cap * 1024 * 1024 < n
            key: int | str = cap if split else "unsplit"
            if key not in encoded_by_effective:
                bp = packed_root / "bwt" / f"cap-{cap}" / f"{name}.anv"
                enc_s, enc_rss = encode_direct(anvil, src, bp, "bwt", cap_mib=cap)
                info = ratio_info(bp)
                if info["backend"] != 2:
                    raise RuntimeError(f"{name}/cap-{cap}: direct BWT emitted backend {info['backend']}")
                expected_sub = (n + cap * 1024 * 1024 - 1) // (cap * 1024 * 1024) if split else 1
                if info["subblocks"] != expected_sub:
                    raise RuntimeError(
                        f"{name}/cap-{cap}: subblocks={info['subblocks']} expected={expected_sub}"
                    )
                encoded_by_effective[key] = {
                    "path": str(bp),
                    "bytes": bp.stat().st_size,
                    "sha256": sha256(bp),
                    "encode_s": enc_s,
                    "encode_rss_kib": enc_rss,
                    "subblocks": info["subblocks"],
                    "ratio_info": info,
                    "physical_cap_mib": cap,
                }
            meta = dict(encoded_by_effective[key])
            meta["requested_cap_mib"] = cap
            meta["reused_unsplit"] = key == "unsplit" and cap != meta["physical_cap_mib"]
            bwt_meta[(name, cap)] = meta

        if unsplit_cap is None:
            raise RuntimeError(f"{name}: no unsplit control cap in {caps}")

    rows: list[dict[str, Any]] = []
    commands_by_cap: dict[int, list[list[str]]] = {cap: [] for cap in caps}
    manifests: dict[str, str] = {}

    for cap in caps:
        for src in files:
            name = src.name
            bm = brotli_meta[name]
            wm = bwt_meta[(name, cap)]
            # encode_ratio_block considers Brotli first and only replaces on
            # strict size improvement, so Brotli is the stable tie winner.
            route = "bwt" if int(wm["bytes"]) < int(bm["bytes"]) else "brotli"
            selected = wm if route == "bwt" else bm
            dec = decoded_root / f"cap-{cap}" / name
            dec.parent.mkdir(parents=True, exist_ok=True)
            cmd = [str(anvil), "d", str(selected["path"]), str(dec), "--quiet"]
            run(cmd)
            if dec.stat().st_size != src.stat().st_size or sha256(dec) != sha256(src):
                raise RuntimeError(f"roundtrip mismatch: {name}/cap-{cap}/{route}")

            timed_dec = timed_root / f"cap-{cap}" / name
            commands_by_cap[cap].append(
                [str(anvil), "d", str(selected["path"]), str(timed_dec), "--quiet"]
            )
            rows.append({
                "corpus": args.corpus,
                "file": name,
                "input_bytes": src.stat().st_size,
                "cap_mib": cap,
                "route": route,
                "selected_bytes": selected["bytes"],
                "brotli_bytes": bm["bytes"],
                "bwt_bytes": wm["bytes"],
                "bwt_subblocks": wm["subblocks"],
                "bwt_encode_s": wm["encode_s"],
                "bwt_encode_rss_kib": wm["encode_rss_kib"],
                "brotli_encode_s": bm["encode_s"],
                "brotli_encode_rss_kib": bm["encode_rss_kib"],
                "selected_encode_s": selected["encode_s"],
                "selected_encode_rss_kib": selected["encode_rss_kib"],
                "bwt_reused_unsplit": wm["reused_unsplit"],
            })

        script = out / f"decode-cap-{cap}.sh"
        write_script(script, commands_by_cap[cap])
        manifest = out / f"decode-cap-{cap}.json"
        manifest.write_text(
            json.dumps(
                {
                    "schema": 1,
                    "corpus": args.corpus,
                    "cap_mib": cap,
                    "files": [p.name for p in files],
                    "commands": commands_by_cap[cap],
                },
                indent=2,
            ) + "\n",
            encoding="utf-8",
        )
        manifests[str(cap)] = str(manifest)

    by_cap: dict[int, dict[str, Any]] = {}
    for cap in caps:
        cr = [r for r in rows if r["cap_mib"] == cap]
        by_cap[cap] = {
            "cap_mib": cap,
            "selected_bytes": sum(int(r["selected_bytes"]) for r in cr),
            "bwt_direct_bytes": sum(int(r["bwt_bytes"]) for r in cr),
            "bwt_routes": sum(r["route"] == "bwt" for r in cr),
            "brotli_routes": sum(r["route"] == "brotli" for r in cr),
            "max_selected_encode_rss_kib": max(int(r["selected_encode_rss_kib"]) for r in cr),
            "files": len(cr),
        }

    control_total = by_cap[CONTROL_CAP_MIB]["selected_bytes"]
    expected = EXPECTED_CONTROL_BYTES[args.corpus]
    if control_total != expected:
        raise RuntimeError(
            f"128 MiB portfolio identity failed: {control_total} != {expected}"
        )

    fields = [
        "corpus", "file", "input_bytes", "cap_mib", "route",
        "selected_bytes", "brotli_bytes", "bwt_bytes", "bwt_subblocks",
        "bwt_encode_s", "bwt_encode_rss_kib",
        "brotli_encode_s", "brotli_encode_rss_kib",
        "selected_encode_s", "selected_encode_rss_kib",
        "bwt_reused_unsplit",
    ]
    with (out / "prepare.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)

    summary = {
        "schema": 1,
        "corpus": args.corpus,
        "caps_mib": caps,
        "control_cap_mib": CONTROL_CAP_MIB,
        "source_bytes": sum(p.stat().st_size for p in files),
        "file_count": len(files),
        "expected_control_bytes": expected,
        "control_identity": "OK",
        "by_cap": {str(k): v for k, v in by_cap.items()},
        "decode_manifests": manifests,
        "source": source_rows,
    }
    (out / "prepare.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(json.dumps(summary, indent=2, sort_keys=True))


def rss(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    prepare_json = json.loads(args.prepare.read_text(encoding="utf-8"))
    caps = [int(x) for x in prepare_json["caps_mib"]]
    rows: list[dict[str, Any]] = []
    by_cap: dict[str, Any] = {}

    for cap in caps:
        manifest_path = Path(prepare_json["decode_manifests"][str(cap)])
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        cap_rows = []
        if len(manifest["commands"]) != len(manifest["files"]):
            raise RuntimeError(f"cap-{cap}: command/file manifest mismatch")
        for i, (name, cmd) in enumerate(zip(manifest["files"], manifest["commands"])):
            elapsed, peak = run_time_rss(cmd)
            row = {
                "corpus": prepare_json["corpus"],
                "cap_mib": cap,
                "file_index": i,
                "file": name,
                "seconds": elapsed,
                "peak_rss_kib": peak,
            }
            rows.append(row)
            cap_rows.append(row)
        by_cap[str(cap)] = {
            "cap_mib": cap,
            "sum_seconds": sum(float(r["seconds"]) for r in cap_rows),
            "max_peak_rss_kib": max(int(r["peak_rss_kib"]) for r in cap_rows),
            "per_file": cap_rows,
        }

    with (out / "rss.csv").open("w", newline="", encoding="utf-8") as f:
        fields = ["corpus", "cap_mib", "file_index", "file", "seconds", "peak_rss_kib"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)

    result = {
        "schema": 1,
        "corpus": prepare_json["corpus"],
        "by_cap": by_cap,
    }
    (out / "rss.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(json.dumps(result, indent=2, sort_keys=True))


def is_dominated(point: dict[str, float], others: list[dict[str, float]]) -> bool:
    for other in others:
        if other is point:
            continue
        le_all = (
            other["bytes"] <= point["bytes"]
            and other["decode_s"] <= point["decode_s"]
            and other["rss_kib"] <= point["rss_kib"]
        )
        lt_any = (
            other["bytes"] < point["bytes"]
            or other["decode_s"] < point["decode_s"]
            or other["rss_kib"] < point["rss_kib"]
        )
        if le_all and lt_any:
            return True
    return False


def summarize(args: argparse.Namespace) -> None:
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    rss_data = json.loads(args.rss.read_text(encoding="utf-8"))
    out = args.out.resolve()
    caps = [int(x) for x in prep["caps_mib"]]
    timing_dir = args.timing_dir.resolve()

    null_path = timing_dir / "paired-null.json"
    null = json.loads(null_path.read_text(encoding="utf-8"))
    if not null.get("timing_valid"):
        timing_note = f"BLOCKED: null timing invalid ({null.get('status')})"
    else:
        lo, hi = null["paired_ratio_ci95"]
        timing_note = f"A/A null CI [{lo:.4f}, {hi:.4f}]"

    rows: list[dict[str, Any]] = []
    control_cap = int(prep["control_cap_mib"])

    # Use the A/A control median as the 128 MiB point.
    control_decode_s = (
        float(null["control"]["median_s"]) + float(null["candidate"]["median_s"])
    ) / 2.0

    for cap in caps:
        p = prep["by_cap"][str(cap)]
        r = rss_data["by_cap"][str(cap)]
        if cap == control_cap:
            decode_s = control_decode_s
            ratio = 1.0
            ci = [float(null["paired_ratio_ci95"][0]), float(null["paired_ratio_ci95"][1])]
            timing_valid = bool(null["timing_valid"])
        else:
            t = json.loads((timing_dir / f"paired-cap-{cap}.json").read_text(encoding="utf-8"))
            decode_s = float(t["candidate"]["median_s"])
            ratio = float(t["paired_ratio_candidate_over_control"])
            ci = [float(x) for x in t["paired_ratio_ci95"]]
            timing_valid = bool(t["timing_valid"])
        rows.append({
            "cap_mib": cap,
            "bytes": int(p["selected_bytes"]),
            "byte_delta_vs_128": int(p["selected_bytes"]) - int(prep["by_cap"][str(control_cap)]["selected_bytes"]),
            "bwt_routes": int(p["bwt_routes"]),
            "decode_s": decode_s,
            "decode_ratio_vs_128": ratio,
            "decode_ci95": ci,
            "timing_valid": timing_valid,
            "rss_kib": int(r["max_peak_rss_kib"]),
            "rss_ratio_vs_128": int(r["max_peak_rss_kib"]) / int(rss_data["by_cap"][str(control_cap)]["max_peak_rss_kib"]),
            "max_selected_encode_rss_kib": int(p["max_selected_encode_rss_kib"]),
        })

    all_timing_valid = bool(null.get("timing_valid")) and all(
        bool(r["timing_valid"]) for r in rows
    )
    points = [
        {
            "bytes": float(r["bytes"]),
            # Use the paired cap/128 ratio as the timing coordinate. Candidate
            # medians come from separate A/B series and are less comparable
            # across time than their paired ratios against the common control.
            "decode_s": float(r["decode_ratio_vs_128"]),
            "rss_kib": float(r["rss_kib"]),
        }
        for r in rows
    ]
    for row, point in zip(rows, points):
        row["pareto_nondominated"] = (
            not is_dominated(point, points) if all_timing_valid else None
        )

    md = [
        f"# I10 BWT subblock frontier — {prep['corpus']}",
        "",
        f"- source bytes: {prep['source_bytes']:,}",
        f"- frozen 128 MiB identity: {prep['control_identity']} ({prep['expected_control_bytes']:,} B)",
        f"- timing control: {timing_note}",
        "",
        "| Cap | Portfolio bytes | Δ vs 128 | BWT routes | Decode s | time / 128 | 95% CI | Peak RSS MiB | RSS / 128 | Pareto |",
        "|---:|---:|---:|---:|---:|---:|---|---:|---:|---|",
    ]
    for row in rows:
        md.append(
            f"| {row['cap_mib']} MiB | {row['bytes']:,} | {row['byte_delta_vs_128']:+,} | "
            f"{row['bwt_routes']} | {row['decode_s']:.4f} | {row['decode_ratio_vs_128']:.4f} | "
            f"[{row['decode_ci95'][0]:.4f}, {row['decode_ci95'][1]:.4f}] | "
            f"{row['rss_kib']/1024:.1f} | {row['rss_ratio_vs_128']:.3f} | "
            f"{'BLOCKED' if row['pareto_nondominated'] is None else 'KEEP' if row['pareto_nondominated'] else 'dominated'} |"
        )

    md.extend([
        "",
        "The Pareto flag is internal to this cap sweep only (bytes, paired decode ratio vs 128 MiB, max per-process decode RSS).",
        "If timing is blocked/noisy the Pareto classification is also BLOCKED rather than inferred from invalid timing.",
        "It is not an external FRONT-CROSSING ruling.",
        "",
    ])
    report = "\n".join(md)
    (out / "summary.md").write_text(report, encoding="utf-8")
    result = {
        "schema": 1,
        "corpus": prep["corpus"],
        "source_bytes": prep["source_bytes"],
        "control_cap_mib": control_cap,
        "timing_null": null,
        "rows": rows,
    }
    (out / "summary.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(report)


def main() -> None:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("prepare")
    p.add_argument("--corpus", choices=("silesia", "enwik8"), required=True)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--caps-mib", default="8,16,32,64,128")
    p.set_defaults(fn=prepare)

    p = sub.add_parser("rss")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(fn=rss)

    p = sub.add_parser("summarize")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--rss", type=Path, required=True)
    p.add_argument("--timing-dir", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(fn=summarize)

    args = ap.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
