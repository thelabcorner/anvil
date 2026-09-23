#!/usr/bin/env python3
"""Rule the I10 remote baseline against the frozen I9 deterministic byte record.

ANVIL's own deterministic auto-direct bytes are a hard regression contract.
External reference-codec byte differences are reported, but not automatically
treated as ANVIL regressions because tool/library versions may differ by series.
"""

from __future__ import annotations

import argparse
import csv
import json
from collections import defaultdict
from pathlib import Path
from typing import Iterable

SILESIA_INPUT = 211_938_580
ENWIK8_INPUT = 100_000_000

# Frozen I9 anvil-auto-direct bytes, by canonical file.
FROZEN_ANVIL = {
    "silesia": {
        "dickens": 2_571_873,
        "mozilla": 13_806_173,
        "mr": 2_382_322,
        "nci": 1_365_712,
        "ooffice": 2_478_889,
        "osdb": 2_584_657,
        "reymont": 1_144_032,
        "samba": 3_761_931,
        "sao": 4_586_126,
        "webster": 7_317_361,
        "x-ray": 4_017_320,
        "xml": 430_599,
    },
    "enwik8": {"enwik8": 23_534_368},
}

FROZEN_REFERENCE_TOTALS = {
    "silesia": {
        "brotli-q11-lw30": 49_383_136,
        "zstd-ultra-22-long27": 52_522_343,
        "xz-9e": 48_456_100,
    },
    "enwik8": {
        "brotli-q11-lw30": 24_810_180,
        "zstd-ultra-22-long27": 25_333_695,
        "xz-9e": 24_831_656,
    },
}

EXPECTED_INPUT = {"silesia": SILESIA_INPUT, "enwik8": ENWIK8_INPUT}


def read_rows(paths: Iterable[Path]) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for path in paths:
        with path.open(newline="", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            required = {
                "corpus",
                "file",
                "input_bytes",
                "codec",
                "compressed_bytes",
                "roundtrip",
            }
            missing = required - set(reader.fieldnames or [])
            if missing:
                raise RuntimeError(f"{path}: missing fields {sorted(missing)}")
            for row in reader:
                row["_source"] = str(path)
                rows.append(row)
    return rows


def aggregate(rows: list[dict[str, str]]) -> dict[tuple[str, str], dict[str, object]]:
    out: dict[tuple[str, str], dict[str, object]] = {}
    grouped: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        grouped[(row["corpus"], row["codec"])].append(row)

    for key, group in grouped.items():
        by_file: dict[str, set[tuple[int, int, str]]] = defaultdict(set)
        for row in group:
            by_file[row["file"]].add(
                (
                    int(row["input_bytes"]),
                    int(row["compressed_bytes"]),
                    row["roundtrip"],
                )
            )
        unstable = {name: vals for name, vals in by_file.items() if len(vals) != 1}
        if unstable:
            raise RuntimeError(f"{key}: deterministic rows disagree across repetitions: {unstable}")

        values = {name: next(iter(vals)) for name, vals in by_file.items()}
        out[key] = {
            "files": values,
            "input_bytes": sum(v[0] for v in values.values()),
            "compressed_bytes": sum(v[1] for v in values.values()),
            "roundtrip": all(v[2] == "OK" for v in values.values()),
            "repetitions": max(
                sum(1 for row in group if row["file"] == name) for name in values
            ),
        }
    return out


def check_anvil(
    corpus: str,
    payload: dict[str, object],
    hard_failures: list[str],
) -> list[str]:
    lines: list[str] = []
    files = payload["files"]
    assert isinstance(files, dict)
    expected = FROZEN_ANVIL[corpus]

    got_names = set(files)
    expected_names = set(expected)
    if got_names != expected_names:
        hard_failures.append(
            f"{corpus}/anvil-auto-direct file set mismatch: "
            f"missing={sorted(expected_names-got_names)} extra={sorted(got_names-expected_names)}"
        )

    mismatch_count = 0
    for name in sorted(expected_names & got_names):
        got = files[name]
        assert isinstance(got, tuple)
        got_bytes = int(got[1])
        want = expected[name]
        if got_bytes != want:
            mismatch_count += 1
            hard_failures.append(
                f"{corpus}/{name}/anvil-auto-direct bytes changed: {got_bytes} != {want}"
            )

    total = int(payload["compressed_bytes"])
    frozen_total = sum(expected.values())
    if total != frozen_total:
        hard_failures.append(
            f"{corpus}/anvil-auto-direct aggregate changed: {total} != {frozen_total}"
        )

    if not bool(payload["roundtrip"]):
        hard_failures.append(f"{corpus}/anvil-auto-direct roundtrip failure")

    lines.append(
        f"| {corpus} | anvil-auto-direct | {total:,} | {frozen_total:,} | "
        f"{total-frozen_total:+,} | {'IDENTICAL' if mismatch_count == 0 and total == frozen_total else 'REGRESSION/CHANGE'} |"
    )
    return lines


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="+", type=Path, help="remote benchmark CSV(s)")
    ap.add_argument("--out", type=Path, help="write Markdown ruling")
    ap.add_argument(
        "--json-out",
        type=Path,
        help="write machine-readable verdict JSON",
    )
    args = ap.parse_args()

    rows = read_rows(args.csv)
    agg = aggregate(rows)
    hard_failures: list[str] = []
    notes: list[str] = []

    lines = [
        "# I10 remote baseline closure",
        "",
        "| Corpus | Codec | Remote bytes | Frozen I9 bytes | Delta | Classification |",
        "|---|---|---:|---:|---:|---|",
    ]

    for corpus in ("silesia", "enwik8"):
        key = (corpus, "anvil-auto-direct")
        if key not in agg:
            hard_failures.append(f"missing {corpus}/anvil-auto-direct")
            continue

        p = agg[key]
        if int(p["input_bytes"]) != EXPECTED_INPUT[corpus]:
            hard_failures.append(
                f"{corpus} input aggregate changed: {p['input_bytes']} != {EXPECTED_INPUT[corpus]}"
            )
        lines.extend(check_anvil(corpus, p, hard_failures))

        for codec, frozen in FROZEN_REFERENCE_TOTALS[corpus].items():
            k = (corpus, codec)
            if k not in agg:
                notes.append(f"missing external reference row {corpus}/{codec}")
                continue
            p = agg[k]
            got = int(p["compressed_bytes"])
            if not bool(p["roundtrip"]):
                hard_failures.append(f"{corpus}/{codec} roundtrip failure")
            classification = "IDENTICAL" if got == frozen else "SERIES/TOOLCHAIN DIFFERENCE"
            lines.append(
                f"| {corpus} | {codec} | {got:,} | {frozen:,} | "
                f"{got-frozen:+,} | {classification} |"
            )

        auto = agg.get((corpus, "anvil-ratio-auto"))
        if auto is not None:
            got = int(auto["compressed_bytes"])
            direct = int(agg[(corpus, "anvil-auto-direct")]["compressed_bytes"])
            lines.append(
                f"| {corpus} | anvil-ratio-auto | {got:,} | n/a | "
                f"{got-direct:+,} vs auto-direct | NEW I10 CONTEXT ROW |"
            )
            if not bool(auto["roundtrip"]):
                hard_failures.append(f"{corpus}/anvil-ratio-auto roundtrip failure")

    lines.extend(["", "## Ruling", ""])
    if hard_failures:
        lines.append("**STOP — remote baseline is not closed.**")
        lines.append("")
        for item in hard_failures:
            lines.append(f"- HARD: {item}")
    else:
        lines.append(
            "**PASS — ANVIL deterministic baseline is byte-identical to frozen I9 for the "
            "canonical auto-direct rows, and all observed rows roundtrip.**"
        )
        lines.append("")
        lines.append(
            "External reference-codec differences, if any, are series/toolchain observations, "
            "not ANVIL byte regressions; preserve their exact tool versions with the artifact."
        )

    if notes:
        lines.extend(["", "## Notes", ""])
        for item in notes:
            lines.append(f"- {item}")

    report = "\n".join(lines) + "\n"
    print(report, end="")

    verdict = {
        "schema": 1,
        "pass": not hard_failures,
        "hard_failures": hard_failures,
        "notes": notes,
        "frozen_anvil_totals": {
            corpus: sum(files.values()) for corpus, files in FROZEN_ANVIL.items()
        },
    }

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(report, encoding="utf-8")
    if args.json_out:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(
            json.dumps(verdict, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )

    if hard_failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
