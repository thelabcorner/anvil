#!/usr/bin/env python3
"""Summarize I10-1A auxiliary-index routing/byte economics.

Consumes a bench_ratio.py CSV containing the control/aux cells and produces a
machine-readable + Markdown view of exact byte deltas and backend-routing changes.
No timing claim is made here.
"""

from __future__ import annotations

import argparse
import csv
import json
from collections import defaultdict
from pathlib import Path
from typing import Any

REQUIRED_CODECS = (
    "anvil-auto-direct",
    "anvil-auto-direct-aux",
    "anvil-bwt-direct",
    "anvil-bwt-direct-aux",
    "anvil-ratio-auto",
    "anvil-ratio-auto-aux",
)


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        raise RuntimeError(f"{path}: no rows")
    return rows


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", type=Path)
    ap.add_argument("--json-out", type=Path)
    ap.add_argument("--md-out", type=Path)
    args = ap.parse_args()

    rows = read_rows(args.csv)
    by_file: dict[tuple[str, str], dict[str, dict[str, str]]] = defaultdict(dict)
    for row in rows:
        if row.get("roundtrip") != "OK":
            raise RuntimeError(
                f"roundtrip failure: {row.get('corpus')}/{row.get('file')}/{row.get('codec')}"
            )
        by_file[(row["corpus"], row["file"])][row["codec"]] = row

    result: dict[str, Any] = {
        "schema": 1,
        "source": str(args.csv),
        "corpora": {},
    }

    md = [
        "# I10-1A auxiliary-index portfolio byte economics",
        "",
        "| Corpus | File | Input | Control auto | Aux auto | Auto delta | Control route | Aux route | Route changed | BWT aux delta |",
        "|---|---|---:|---:|---:|---:|---|---|---|---:|",
    ]

    corpus_agg: dict[str, dict[str, Any]] = defaultdict(
        lambda: {
            "input_bytes": 0,
            "control_auto_bytes": 0,
            "aux_auto_bytes": 0,
            "control_ratio_auto_bytes": 0,
            "aux_ratio_auto_bytes": 0,
            "bwt_selected_control_files": 0,
            "bwt_selected_aux_files": 0,
            "routing_changes": 0,
            "files": [],
        }
    )

    for (corpus, file), cells in sorted(by_file.items()):
        missing = [c for c in REQUIRED_CODECS if c not in cells]
        if missing:
            raise RuntimeError(f"{corpus}/{file}: missing codec cells {missing}")

        def b(codec: str) -> int:
            return int(cells[codec]["compressed_bytes"])

        input_bytes = int(cells["anvil-auto-direct"]["input_bytes"])
        control_auto = b("anvil-auto-direct")
        aux_auto = b("anvil-auto-direct-aux")
        control_bwt = b("anvil-bwt-direct")
        aux_bwt = b("anvil-bwt-direct-aux")
        control_ratio = b("anvil-ratio-auto")
        aux_ratio = b("anvil-ratio-auto-aux")

        control_route = "bwt" if control_auto == control_bwt else "non-bwt"
        aux_route = "bwt" if aux_auto == aux_bwt else "non-bwt"
        route_changed = control_route != aux_route

        item = {
            "file": file,
            "input_bytes": input_bytes,
            "control_auto_bytes": control_auto,
            "aux_auto_bytes": aux_auto,
            "auto_delta_bytes": aux_auto - control_auto,
            "auto_delta_source_ratio": (aux_auto - control_auto) / input_bytes,
            "control_route": control_route,
            "aux_route": aux_route,
            "route_changed": route_changed,
            "control_bwt_bytes": control_bwt,
            "aux_bwt_bytes": aux_bwt,
            "bwt_aux_delta_bytes": aux_bwt - control_bwt,
            "control_ratio_auto_bytes": control_ratio,
            "aux_ratio_auto_bytes": aux_ratio,
            "ratio_auto_delta_bytes": aux_ratio - control_ratio,
        }

        agg = corpus_agg[corpus]
        agg["files"].append(item)
        agg["input_bytes"] += input_bytes
        agg["control_auto_bytes"] += control_auto
        agg["aux_auto_bytes"] += aux_auto
        agg["control_ratio_auto_bytes"] += control_ratio
        agg["aux_ratio_auto_bytes"] += aux_ratio
        agg["bwt_selected_control_files"] += control_route == "bwt"
        agg["bwt_selected_aux_files"] += aux_route == "bwt"
        agg["routing_changes"] += route_changed

        md.append(
            f"| {corpus} | {file} | {input_bytes:,} | {control_auto:,} | "
            f"{aux_auto:,} | {aux_auto-control_auto:+,} | {control_route} | "
            f"{aux_route} | {'YES' if route_changed else 'no'} | "
            f"{aux_bwt-control_bwt:+,} |"
        )

    md.extend(["", "## Corpus totals", ""])
    md.append(
        "| Corpus | Input | Control auto | Aux auto | Auto delta | delta/source | "
        "BWT routes control->aux | Routing changes | Ratio-auto delta |"
    )
    md.append("|---|---:|---:|---:|---:|---:|---:|---:|---:|")

    for corpus, agg in sorted(corpus_agg.items()):
        auto_delta = agg["aux_auto_bytes"] - agg["control_auto_bytes"]
        ratio_delta = agg["aux_ratio_auto_bytes"] - agg["control_ratio_auto_bytes"]
        agg["auto_delta_bytes"] = auto_delta
        agg["auto_delta_source_ratio"] = auto_delta / agg["input_bytes"]
        agg["ratio_auto_delta_bytes"] = ratio_delta
        result["corpora"][corpus] = agg
        md.append(
            f"| {corpus} | {agg['input_bytes']:,} | {agg['control_auto_bytes']:,} | "
            f"{agg['aux_auto_bytes']:,} | {auto_delta:+,} | "
            f"{agg['auto_delta_source_ratio']:.8%} | "
            f"{agg['bwt_selected_control_files']}->{agg['bwt_selected_aux_files']} | "
            f"{agg['routing_changes']} | {ratio_delta:+,} |"
        )

    report = "\n".join(md) + "\n"
    print(report, end="")

    if args.json_out:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(
            json.dumps(result, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
    if args.md_out:
        args.md_out.parent.mkdir(parents=True, exist_ok=True)
        args.md_out.write_text(report, encoding="utf-8")


if __name__ == "__main__":
    main()
