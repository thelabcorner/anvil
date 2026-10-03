#!/usr/bin/env python3
"""Fledge static audit of the I10 dense-frontier measurement instrument.

Scope: this is a STATIC linter. It parses the workflow text, classifies each
arm's measurement instrument, and computes the dispatch invocation budget from
the frozen prereg table. It runs NO codec, NO corpus, NO benchmark, NO timing.

It cannot tell you whether a number is fast. It can tell you whether two arms
were measured with the same instrument, which is the precondition for comparing
them at all.

Usage: python instrument_audit.py <workflow.yml> [--prereg <prereg.md>]
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

# Instrument classes. The class is decided by how the process obtains its input
# and emits its output, because that -- not the codec -- sets the floor on
# measured wall time and the composition of measured peak RSS.
INSTRUMENT = {
    "buffered_harness": "process reads whole input into RAM and pre-sizes whole output buffer",
    "streaming_cli": "process streams input->output through internal fixed buffers",
    "shell_redirect": "process writes via shell stdout redirection inside an extra bash spawn",
}

# Source sizes from docs/I10-DENSE-FRONTIER-PREREG.md section 4.
PANEL_SILESIA = ["dickens", "mozilla", "nci", "webster", "xml"]
PANEL_ENWIK8 = ["enwik8"]
SILESIA_BYTES = {
    "dickens": 10_192_446,
    "mozilla": 51_220_480,
    "mr": 9_970_564,
    "nci": 33_553_445,
    "ooffice": 6_152_192,
    "osdb": 10_085_684,
    "reymont": 6_627_202,
    "samba": 21_606_400,
    "sao": 7_251_944,
    "webster": 41_458_703,
    "x-ray": 8_474_240,
    "xml": 5_345_280,
}
ENWIK8_BYTES = 100_000_000


def classify(command_fragment: str) -> str:
    if '"bash"' in command_fragment or "str(encode_script)" in command_fragment:
        return "shell_redirect"
    if '"-o", str(' in command_fragment or "-o\", str(" in command_fragment:
        return "streaming_cli"
    return "buffered_harness"


def extract_arms(text: str) -> dict[str, dict[str, str]]:
    """Pull the commands_for() branches out of the workflow text.

    Scoped to the commands_for() function body so that unrelated `if arm ==`
    statements elsewhere in the workflow cannot be mistaken for arm branches.
    """
    start = text.find("def commands_for(")
    end = text.find("cells = {}", start)
    if start < 0 or end < 0:
        return {}
    body_text = text[start:end]
    arms: dict[str, dict[str, str]] = {}
    guard_re = re.compile(r'^ {12}if arm (?:== (\S+)|startswith\((\S+)\)):', re.M)
    matches = list(guard_re.finditer(body_text))
    for index, match in enumerate(matches):
        stop = matches[index + 1].start() if index + 1 < len(matches) else len(body_text)
        chunk = body_text[match.start():stop]
        label = match.group(1) or f"{match.group(2).strip(chr(34))}<arg>"
        enc = re.search(r"return \(\[(.*?)\],\s*\[(.*?)\]\)", chunk, re.S)
        arms[label] = {
            "guard": match.group(0).strip(),
            "encode_fragment": (enc.group(1) if enc else "").strip(),
            "decode_fragment": (enc.group(2) if enc else "").strip(),
            "enc_instrument": classify(enc.group(1) if enc else ""),
            "dec_instrument": classify(enc.group(2) if enc else ""),
        }
    return arms


def extract_arm_list(text: str) -> list[str]:
    arms: list[str] = []
    literal = re.search(r'arms = \[([^\]]*)\]', text)
    if literal:
        arms += re.findall(r'"([^"]+)"', literal.group(1))
    brotli = re.search(r'arms \+= \[f"brotli-q\{q\}-lw30" for q in \(([^)]*)\)\]', text)
    zstd = re.search(r'arms \+= \[f"zstd-\{level\}" for level in \(([^)]*)\)\]', text)
    if brotli:
        arms += [f"brotli-q{q.strip()}-lw30" for q in brotli.group(1).split(",")]
    if zstd:
        arms += [f"zstd-{level.strip()}" for level in zstd.group(1).split(",")]
    long_arm = re.search(r'arms \+= \[([^\]]*?"zstd-22-long27"[^\]]*?)\]', text)
    if long_arm:
        arms += re.findall(r'"([^"]+)"', long_arm.group(1))
    return arms


def anchored(text: str, arm: str) -> bool:
    """Is this arm's byte total hard-gated in the prepare step?"""
    if re.search(rf'"{re.escape(arm)}"\s*:\s*\d+', text):
        return True
    if arm.startswith("anvil-"):
        return True  # gated by the per-file expected_anvil table
    return False


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("workflow")
    parser.add_argument("--prereg", default=None)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    text = Path(args.workflow).read_text(encoding="utf-8", errors="replace")
    arms = extract_arm_list(text)
    branches = extract_arms(text)
    panel = {"silesia": PANEL_SILESIA, "enwik8": PANEL_ENWIK8}
    sizes = {"silesia": SILESIA_BYTES, "enwik8": {"enwik8": ENWIK8_BYTES}}

    report: dict[str, object] = {
        "arms_declared": arms,
        "arm_count": len(arms),
        "branches_found": branches,
        "anchored_arms": [a for a in arms if anchored(text, a)],
        "unanchored_arms": [a for a in arms if not anchored(text, a)],
        "corpora": {},
    }

    for corpus, files in panel.items():
        reps = 7
        warmups = 2
        planes = 2
        invocations_per_arm = (warmups + reps) * planes
        panel_bytes = sum(sizes[corpus][f] for f in files)
        corpus_bytes = sum(sizes[corpus].values())
        report["corpora"][corpus] = {
            "panel": files,
            "panel_source_bytes": panel_bytes,
            "corpus_source_bytes": corpus_bytes,
            "panel_coverage_pct": round(100.0 * panel_bytes / corpus_bytes, 2),
            "timing_driver_invocations": len(arms) * len(files) * planes,
            "codec_invocations_per_arm": invocations_per_arm,
            "total_codec_invocations": len(arms) * len(files) * invocations_per_arm,
            "census_codec_invocations": len(arms) * len({"silesia": SILESIA_BYTES, "enwik8": {"enwik8": ENWIK8_BYTES}}[corpus]),
            "raw_observation_rows_expected": len(arms) * len(files) * planes * reps * 2,
        }

    # Instrument-class uniformity across arms.
    classes: dict[str, dict[str, str]] = {}
    for label, info in branches.items():
        classes[label] = {
            "encode": info["enc_instrument"],
            "decode": info["dec_instrument"],
        }
    report["instrument_classes"] = classes
    enc_classes = {info["encode"] for info in classes.values()}
    dec_classes = {info["decode"] for info in classes.values()}
    report["encode_instrument_uniform"] = len(enc_classes) == 1
    report["decode_instrument_uniform"] = len(dec_classes) == 1
    report["decode_instrument_classes_observed"] = sorted(dec_classes)

    # A shell-mediated arm pays extra bash spawns that no other arm pays.
    report["shell_mediated_arms"] = [
        label for label, info in classes.items() if "shell" in (info["encode"] + info["decode"])
    ]

    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(f"arms declared: {report['arm_count']}")
        print(f"anchored byte gates: {len(report['anchored_arms'])}")
        print(f"UNANCHORED (first-run bytes, no identity gate): {report['unanchored_arms']}")
        print(f"encode instrument uniform: {report['encode_instrument_uniform']}")
        print(f"decode instrument uniform: {report['decode_instrument_uniform']}")
        print(f"decode instrument classes observed: {report['decode_instrument_classes_observed']}")
        print(f"shell-mediated arms: {report['shell_mediated_arms']}")
        for corpus, info in report["corpora"].items():
            print(
                f"[{corpus}] panel coverage {info['panel_coverage_pct']}% of corpus bytes; "
                f"codec invocations {info['total_codec_invocations']} "
                f"(+{info['census_codec_invocations']} census); "
                f"raw timing rows {info['raw_observation_rows_expected']}"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())