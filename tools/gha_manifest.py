#!/usr/bin/env python3
"""Create machine-readable provenance and deterministic byte summaries for ANVIL CI.

This script does not benchmark. It summarizes already-produced result CSVs and
captures enough runner/toolchain identity to make an Actions artifact self-
describing. Timing fields are retained as context but are never promoted here.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import platform
import re
import subprocess
from collections import defaultdict
from pathlib import Path
from typing import Any


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def command_text(argv: list[str]) -> str:
    try:
        p = subprocess.run(
            argv,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=10,
        )
        return p.stdout.strip()
    except Exception as exc:
        return f"<unavailable: {exc}>"


def first_line(text: str) -> str:
    return text.splitlines()[0].strip() if text else ""


def cpu_info() -> dict[str, Any]:
    text = command_text(["lscpu"])
    out: dict[str, Any] = {"raw": text}
    for line in text.splitlines():
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        key = key.strip().lower().replace(" ", "_").replace("(", "").replace(")", "")
        value = value.strip()
        if key in {
            "architecture",
            "cpu_s",
            "model_name",
            "vendor_id",
            "socket_s",
            "core_s_per_socket",
            "thread_s_per_core",
            "numa_node_s",
        }:
            out[key] = value
    return out


def package_version(name: str) -> str:
    return command_text(["dpkg-query", "-W", "-f=${Version}", name]).strip()


def result_files(out_dir: Path) -> list[Path]:
    return sorted(
        p
        for p in out_dir.rglob("*")
        if p.is_file() and p.name not in {"manifest.json", "bytes.csv"}
    )


def parse_ratio_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        fields = set(reader.fieldnames or [])
        required = {
            "corpus",
            "file",
            "input_bytes",
            "codec",
            "compressed_bytes",
            "ratio",
            "roundtrip",
        }
        if not required.issubset(fields):
            return []
        return list(reader)


def rep_index(path: Path) -> int:
    m = re.search(r"-rep(\d+)\.csv$", path.name)
    return int(m.group(1)) if m else 0


def aggregate_bytes(out_dir: Path) -> list[dict[str, Any]]:
    per_rep: dict[tuple[str, str, int], dict[str, Any]] = {}

    for path in sorted(out_dir.glob("*-rep*.csv")):
        rows = parse_ratio_csv(path)
        if not rows:
            continue
        rep = rep_index(path)
        bucket: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)
        for row in rows:
            bucket[(row["corpus"], row["codec"])].append(row)

        for (corpus, codec), group in bucket.items():
            names = [r["file"] for r in group]
            if len(names) != len(set(names)):
                raise RuntimeError(
                    f"{path}: duplicate file rows for {corpus}/{codec}: {names}"
                )
            input_bytes = sum(int(r["input_bytes"]) for r in group)
            compressed_bytes = sum(int(r["compressed_bytes"]) for r in group)
            roundtrip = all(r["roundtrip"] == "OK" for r in group)
            per_rep[(corpus, codec, rep)] = {
                "input_bytes": input_bytes,
                "compressed_bytes": compressed_bytes,
                "roundtrip": roundtrip,
                "files": len(group),
            }

    grouped: dict[tuple[str, str], list[tuple[int, dict[str, Any]]]] = defaultdict(list)
    for (corpus, codec, rep), payload in per_rep.items():
        grouped[(corpus, codec)].append((rep, payload))

    out: list[dict[str, Any]] = []
    for (corpus, codec), reps in sorted(grouped.items()):
        reps.sort(key=lambda item: item[0])
        signatures = {
            (
                v["input_bytes"],
                v["compressed_bytes"],
                bool(v["roundtrip"]),
                v["files"],
            )
            for _, v in reps
        }
        deterministic = len(signatures) == 1
        if not deterministic:
            raise RuntimeError(
                f"deterministic aggregate changed across reps for {corpus}/{codec}: {reps}"
            )
        _, value = reps[0]
        out.append(
            {
                "corpus": corpus,
                "codec": codec,
                "input_bytes": value["input_bytes"],
                "compressed_bytes": value["compressed_bytes"],
                "ratio": (
                    value["compressed_bytes"] / value["input_bytes"]
                    if value["input_bytes"]
                    else 0.0
                ),
                "roundtrip": "OK" if value["roundtrip"] else "FAIL",
                "files": value["files"],
                "repetitions": len(reps),
                "deterministic_across_reps": deterministic,
            }
        )
    return out


def write_bytes_csv(path: Path, rows: list[dict[str, Any]]) -> None:
    fields = [
        "corpus",
        "codec",
        "input_bytes",
        "compressed_bytes",
        "ratio",
        "roundtrip",
        "files",
        "repetitions",
        "deterministic_across_reps",
    ]
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for row in rows:
            item = dict(row)
            item["ratio"] = f"{float(item['ratio']):.9f}"
            w.writerow(item)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-dir", type=Path, required=True)
    ap.add_argument("--suite", required=True)
    ap.add_argument("--reps", type=int, required=True)
    ap.add_argument("--fuzz-cases", type=int, required=True)
    args = ap.parse_args()

    out_dir = args.out_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    byte_rows = aggregate_bytes(out_dir)
    write_bytes_csv(out_dir / "bytes.csv", byte_rows)

    files = result_files(out_dir)
    manifest = {
        "schema": 1,
        "project": "ANVIL",
        "benchmark_series": "linux-gha-i10",
        "timing_class": "shared-runner-scout",
        "suite": args.suite,
        "requested_repetitions": args.reps,
        "requested_fuzz_cases": args.fuzz_cases,
        "github": {
            "repository": os.getenv("GITHUB_REPOSITORY", ""),
            "sha": os.getenv("GITHUB_SHA", ""),
            "ref": os.getenv("GITHUB_REF", ""),
            "run_id": os.getenv("GITHUB_RUN_ID", ""),
            "run_attempt": os.getenv("GITHUB_RUN_ATTEMPT", ""),
            "job": os.getenv("GITHUB_JOB", ""),
        },
        "runner": {
            "os": os.getenv("RUNNER_OS", platform.system()),
            "arch": os.getenv("RUNNER_ARCH", platform.machine()),
            "image_os": os.getenv("ImageOS", ""),
            "image_version": os.getenv("ImageVersion", ""),
            "platform": platform.platform(),
            "cpu": cpu_info(),
        },
        "toolchain": {
            "clang": first_line(command_text(["clang-18", "--version"])),
            "clangxx": first_line(command_text(["clang++-18", "--version"])),
            "cmake": first_line(command_text(["cmake", "--version"])),
            "ninja": first_line(command_text(["ninja", "--version"])),
            "python": platform.python_version(),
            "zstd": first_line(command_text(["zstd", "--version"])),
            "xz": first_line(command_text(["xz", "--version"])),
            "brotli_dev": package_version("libbrotli-dev"),
            "zstd_dev": package_version("libzstd-dev"),
        },
        "deterministic_byte_summary": byte_rows,
        "artifacts": [
            {
                "path": str(p.relative_to(out_dir)).replace("\\", "/"),
                "bytes": p.stat().st_size,
                "sha256": sha256(p),
            }
            for p in files
        ],
    }

    path = out_dir / "manifest.json"
    path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"wrote {path}")
    print(f"wrote {out_dir / 'bytes.csv'}")
    for row in byte_rows:
        print(
            f"{row['corpus']}/{row['codec']}: "
            f"{row['compressed_bytes']:,}/{row['input_bytes']:,} B "
            f"r={row['ratio']:.6f} reps={row['repetitions']} "
            f"{row['roundtrip']}"
        )


if __name__ == "__main__":
    main()
