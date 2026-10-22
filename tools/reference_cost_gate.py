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
    # I20: one shlex-quoted string, split here, so that a value beginning with
    # '--' is not mistaken for an argparse option terminator.
    candidate_extra_args = shlex.split(args.candidate_extra_args or "")
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
        # I20 addition (coordinator-owned, additive, default-preserving):
        # --candidate-extra-args appends encoder-policy flags to the ANVIL arm
        # so a candidate arm can be ruled by this same frozen arbiter. With the
        # default (empty) the command is byte-identical to the pre-I20 form.
        run([
            str(anvil), "c", str(src), str(cand),
            "--parse=ratio", "--ratio-backend=auto",
            "--ratio-context=off", "--ratio-lines=off",
            "--bwt-aux=on", "--quiet",
            *candidate_extra_args,
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

    # ------------------------------------------------------------------
    # I11 extension (candidate D5): ADDITIVE ONLY.  The legacy `ruling`,
    # the legacy `gates` values and the legacy `scope_note` are untouched
    # and are reproduced byte-for-byte when the new inputs are absent.
    # ------------------------------------------------------------------
    if args.encode or args.decoder_bytes:
        encode_axis = load_json(args.encode) if args.encode else None
        dec_axis = load_json(args.decoder_bytes) if args.decoder_bytes else None

        def _axis_status(axis):
            if not axis:
                return None
            return str((axis.get("axis") or {}).get("status") or "") or None

        enc_status = _axis_status(encode_axis)
        dec_status = _axis_status(dec_axis)

        def _gate_value(status):
            if status is None:
                return None
            if status == "PASS":
                return True
            if status == "FAIL":
                return False
            return None

        result["gates"]["encode_at_least_as_fast_as_every_reference"] = _gate_value(enc_status)
        result["gates"]["decoder_binary_at_most_every_reference"] = _gate_value(dec_status)
        result["dominance"] = {
            "encode": encode_axis or {},
            "decoder_bytes": dec_axis or {},
        }
        if ruling == "COST_GAP_CLOSED_PENDING_ENCODE_DECSIZE":
            if enc_status == "PASS" and dec_status == "PASS":
                extended = "COST_GAP_CLOSED_ENCODE_DECSIZE_RULED_PASS"
            elif enc_status == "FAIL" or dec_status == "FAIL":
                extended = "COST_GAP_CLOSED_ENCODE_DECSIZE_RULED_FAIL"
            else:
                extended = "COST_GAP_CLOSED_ENCODE_DECSIZE_RULED_INCONCLUSIVE"
        else:
            extended = ruling
        result["extended_ruling"] = extended
        result["scope_note_i11"] = (
            "I11 extension (D5): this arbiter now ALSO rules the encode "
            "wall-time axis and the decoder-only binary-size axis, which the "
            "frozen I20 scope_note above excludes.  The I20 scope_note is kept "
            "verbatim for regression and is superseded by this note whenever "
            "the two new axes are present.  A dominance or FRONT-CROSSING "
            "claim is permitted ONLY where extended_ruling is "
            "COST_GAP_CLOSED_ENCODE_DECSIZE_RULED_PASS, and only over the "
            "front points actually measured: every front point that was not "
            "measured is reported as NOT_MEASURED / TIMING_BLOCKED and is "
            "never counted as a pass."
        )

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
    if args.encode or args.decoder_bytes:
        enc = result["dominance"]["encode"]
        dec = result["dominance"]["decoder_bytes"]
        md += [
            "## I11 extension - the two axes the frozen I20 scope note excluded",
            "",
            f"- extended ruling: **{result['extended_ruling']}**",
            f"- encode axis: **{(enc.get('axis') or {}).get('status')}** "
            f"({(enc.get('counts') or {}).get('PASS', 0)} PASS / "
            f"{(enc.get('counts') or {}).get('FAIL', 0)} FAIL / "
            f"{(enc.get('counts') or {}).get('INCONCLUSIVE', 0)} INCONCLUSIVE / "
            f"{(enc.get('counts') or {}).get('TIMING_BLOCKED', 0)} TIMING_BLOCKED)",
            f"- decoder-only binary-size axis: **{(dec.get('axis') or {}).get('status')}** "
            f"({(dec.get('counts') or {}).get('PASS', 0)} PASS / "
            f"{(dec.get('counts') or {}).get('FAIL', 0)} FAIL / "
            f"{(dec.get('counts') or {}).get('NOT_MEASURED', 0)} NOT_MEASURED)",
            "",
            "| reference point | encode ratio (CI95) | encode status | candidate bytes | reference bytes | decoder bytes (cand/ref) | decoder status |",
            "|---|---|---|---:|---:|---|---|",
        ]
        enc_points = {p["name"]: p for p in (enc.get("points") or [])}
        dec_points = {p["name"]: p for p in (dec.get("points") or [])}
        for point in FRONT_POINTS:
            e = enc_points.get(point["name"], {})
            d = dec_points.get(point["name"], {})
            ci = e.get("ci95") or []
            ci_txt = f"[{ci[0]:.4f}, {ci[1]:.4f}]" if len(ci) == 2 else "-"
            md.append(
                f"| {point['name']} | {ci_txt} | {e.get('status', 'NOT_MEASURED')} | "
                f"{e.get('candidate_bytes', '-')} | {e.get('reference_bytes', '-')} | "
                f"{d.get('candidate_bytes', '-')} / {d.get('reference_bytes', '-')} | "
                f"{d.get('status', 'NOT_MEASURED')} |")
        md += ["", result["scope_note_i11"], ""]
    md_path = args.out.with_suffix(".md")
    md_path.write_text("\n".join(md), encoding="utf-8")
    print("\n".join(md))


# ===========================================================================
# I11 EXTENSION (candidate D5) -- the encode axis and the decoder-only
# binary-size axis.
#
# The I20 arbiter's own scope note says that passing it "does not establish a
# crossing because encode and decoder-only binary-size axes are not ruled
# here".  This extension is ADDITIVE ONLY:
#
#   * no existing threshold is relaxed, widened, re-weighted or reinterpreted;
#   * every existing verdict is reproduced byte-for-byte when the new inputs
#     are absent (the regression harness proves this over the stored I20
#     artefacts);
#   * the new axes only ever ADD conditions.
#
# The two new axes are:
#
#   ENCODE AXIS
#     Whole-corpus encode wall time of the candidate against every reference
#     point of the I11 dominance front, measured inside one job with the
#     repository's frozen paired protocol (tools/paired_bench.py with the
#     frozen parameters) and a mandatory A/A null whose CI must span 1.0.
#     Rule (fixed in advance): for every reference point r the 95% CI upper
#     bound of the paired candidate/reference ratio must be <= 1.0
#     (PASS); if the CI lower bound is > 1.0 the candidate is provably
#     slower (FAIL); a CI that straddles 1.0 is INCONCLUSIVE; a series whose
#     A/A null does not span 1.0, or whose timing is invalid, is
#     TIMING_BLOCKED and counts for nothing.
#
#   DECODER-ONLY BINARY-SIZE AXIS
#     The size of the decoder-providing artifact of the candidate against the
#     decoder-providing artifact of every reference point, with the exact
#     per-producer definition recorded (see DECODER_SIZE_DEFINITION).
#     Rule (fixed in advance): the candidate's decoder size must be <= every
#     reference point's decoder size.
# ===========================================================================

CANDIDATE_POINT = "anvil-aux"

#: The 17 reference points of the I11 dominance front (prereg 1.1): Brotli
#: q1/q4/q6/q9/q11 at window 22 plus q11 at windows 24 and 30, zstd
#: 1/3/9/19/22, xz -6/-9/-9e, lz4 1/9.
FRONT_POINTS: list[dict[str, Any]] = [
    {"name": "brotli-q1-w22", "family": "brotli", "config": "q1 lgwin22",
     "quality": 1, "lgwin": 22},
    {"name": "brotli-q4-w22", "family": "brotli", "config": "q4 lgwin22",
     "quality": 4, "lgwin": 22},
    {"name": "brotli-q6-w22", "family": "brotli", "config": "q6 lgwin22",
     "quality": 6, "lgwin": 22},
    {"name": "brotli-q9-w22", "family": "brotli", "config": "q9 lgwin22",
     "quality": 9, "lgwin": 22},
    {"name": "brotli-q11-w22", "family": "brotli", "config": "q11 lgwin22",
     "quality": 11, "lgwin": 22},
    {"name": "brotli-q11-w24", "family": "brotli", "config": "q11 lgwin24",
     "quality": 11, "lgwin": 24},
    {"name": "brotli-q11-w30", "family": "brotli", "config": "q11 lgwin30",
     "quality": 11, "lgwin": 30},
    {"name": "zstd-1", "family": "zstd", "config": "-1", "level": 1},
    {"name": "zstd-3", "family": "zstd", "config": "-3", "level": 3},
    {"name": "zstd-9", "family": "zstd", "config": "-9", "level": 9},
    {"name": "zstd-19", "family": "zstd", "config": "-19", "level": 19},
    {"name": "zstd-22", "family": "zstd",
     "config": "-22 --ultra --long=27 --memory=1G", "level": 22,
     "ultra": True, "long_window": True},
    {"name": "xz-6", "family": "xz", "config": "-6", "level": "6"},
    {"name": "xz-9", "family": "xz", "config": "-9", "level": "9"},
    {"name": "xz-9e", "family": "xz", "config": "-9e", "level": "9e"},
    {"name": "lz4-1", "family": "lz4", "config": "-1", "level": 1},
    {"name": "lz4-9", "family": "lz4", "config": "-9", "level": 9},
]

FRONT_BY_NAME = {p["name"]: p for p in FRONT_POINTS}

#: The exact definition of "decoder-only binary size" used by the
#: decoder-bytes axis.  It is stated here (in the tool) as well as in the
#: results document, and it is emitted verbatim into decoder-bytes.json.
DECODER_SIZE_DEFINITION = (
    "decoder-only binary size = the size in bytes of the single shipped file "
    "that provides a COMPLETE decoder for the producer's compressed format, as "
    "resolved on the measurement runner, with the file path, SHA-256, producer "
    "version and the per-producer definition recorded.  Per producer: "
    "ANVIL (candidate) = the `anvil` executable built from the candidate ref; "
    "the codec is ONE translation unit (src/anvil.cpp) that contains the "
    "encoder, the decoder and the CLI, so no decoder-only object set exists "
    "without an src/ refactor owned by another lane -- the measured value is "
    "therefore an UPPER BOUND on ANVIL's decoder-only size.  "
    "Brotli = the decoder-only shared library `libbrotlidec.so.<ver>` that the "
    "measured decode actually links against; upstream ships the decoder "
    "separately from the encoder, so this is the EXACT decoder-only size.  "
    "zstd = `libzstd.so.<ver>`, xz = `liblzma.so.<ver>`, lz4 = "
    "`liblz4.so.<ver>`: each distribution ships ONE combined library that "
    "contains both the encoder and the decoder, so each measured value is an "
    "UPPER BOUND on that producer's decoder-only size.  Where a producer "
    "ships a separable decoder-only library, that library's size is recorded "
    "as `decoder_only_refined`; where it does not, or where the candidate's "
    "decoder cannot be separated at object level, the refined value is "
    "NOT_MEASURED with the reason recorded -- never guessed."
)

#: Per-producer definition strings (emitted into decoder-bytes.json).
DECODER_PRODUCER_DEFINITIONS = {
    "anvil": {
        "artifact": "the `anvil` executable",
        "decoder_only": False,
        "note": (
            "ONE translation unit (src/anvil.cpp) holds the encoder, the "
            "decoder and the CLI, so the executable size is an UPPER BOUND on "
            "the decoder-only size."
        ),
    },
    "brotli": {
        "artifact": "libbrotlidec.so.<ver>",
        "decoder_only": True,
        "note": (
            "upstream ships the Brotli decoder separately from the encoder; "
            "the measured shared object is the EXACT decoder-only size."
        ),
    },
    "zstd": {
        "artifact": "libzstd.so.<ver>",
        "decoder_only": False,
        "note": (
            "the distribution ships one combined zstd library containing the "
            "encoder and the decoder: UPPER BOUND."
        ),
    },
    "xz": {
        "artifact": "liblzma.so.<ver>",
        "decoder_only": False,
        "note": (
            "the distribution ships one combined liblzma containing the "
            "encoder and the decoder: UPPER BOUND."
        ),
    },
    "lz4": {
        "artifact": "liblz4.so.<ver>",
        "decoder_only": False,
        "note": (
            "the distribution ships one combined liblz4 containing the "
            "encoder and the decoder: UPPER BOUND."
        ),
    },
}


def write_script_lines(path: Path, lines: list[str]) -> None:
    body = ["#!/usr/bin/env bash", "set -euo pipefail"]
    body.extend(lines)
    path.write_text("\n".join(body) + "\n", encoding="utf-8")
    path.chmod(0o755)


def script_line(argv: list[str], stdout_path: Path | None = None) -> str:
    if stdout_path is None:
        return shlex.join(argv)
    return shlex.join(argv) + " > " + shlex.quote(str(stdout_path))


def front_point_commands(
    point: dict[str, Any],
    args: argparse.Namespace,
) -> tuple[Any, Any]:
    """Return the (encode, decode) command builders for one front point."""
    family = point["family"]
    if family == "brotli":
        helper = args.brotli_dense

        def encode(src: Path, packed: Path):
            return ([str(helper), "c", str(point["quality"]), str(point["lgwin"]),
                     str(src), str(packed)], None)

        def decode(packed: Path, decoded: Path, n: int):
            return ([str(helper), "d", str(packed), str(decoded), str(n)], decoded)
    elif family == "zstd":
        binary = args.zstd
        ultra = bool(point.get("ultra"))
        long_window = bool(point.get("long_window"))

        def encode(src: Path, packed: Path):
            argv = [str(binary), "-q", "-f"]
            if ultra:
                argv.append("--ultra")
            argv.append(f"-{point['level']}")
            if long_window:
                argv += ["--long=27", "--memory=1G"]
            argv += ["-o", str(packed), str(src)]
            return argv, None

        def decode(packed: Path, decoded: Path, n: int):
            argv = [str(binary), "-q", "-f", "-d"]
            if long_window:
                argv.append("--long=27")
            argv += ["-o", str(decoded), str(packed)]
            return argv, decoded
    elif family == "xz":
        binary = args.xz
        level = point["level"]

        def encode(src: Path, packed: Path):
            return ([str(binary), f"-{level}", "-c", str(src)], packed)

        def decode(packed: Path, decoded: Path, n: int):
            return ([str(binary), "-d", "-k", "-f", str(packed)],
                    packed.with_suffix(""))
    elif family == "lz4":
        binary = args.lz4

        def encode(src: Path, packed: Path):
            return ([str(binary), "-q", "-f", f"-{point['level']}",
                     str(src), str(packed)], None)

        def decode(packed: Path, decoded: Path, n: int):
            return ([str(binary), "-q", "-f", "-d", str(packed)],
                    packed.with_suffix(""))
    else:
        raise RuntimeError(f"unknown front family: {family}")
    return encode, decode


def prepare_front(args: argparse.Namespace) -> None:
    """Build the I11 dominance front: payloads, whole-corpus encode scripts,
    decode scripts, byte totals and the A/A-null scripts.

    Correctness (byte-exact round trip) is established before any timing is
    consumed, exactly as the frozen `prepare` stage does.
    """
    root = args.root.resolve()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    files = canonical_files(args.corpus, root)
    extra = shlex.split(args.candidate_extra_args or "")
    anvil = args.anvil.resolve()
    if not anvil.is_file():
        raise RuntimeError(f"missing candidate binary: {anvil}")

    wanted = [p.strip() for p in (args.points or "").split(",") if p.strip()]
    points: list[dict[str, Any]] = []
    for name in wanted:
        if name not in FRONT_BY_NAME:
            raise RuntimeError(
                f"unknown front point: {name} (known: {', '.join(FRONT_BY_NAME)})")
        points.append(FRONT_BY_NAME[name])

    source_total = sum(p.stat().st_size for p in files)
    source_digest = {p.name: sha256(p) for p in files}
    biggest = max(files, key=lambda p: p.stat().st_size)

    pack_root = out / "packed"
    dec_root = out / "decoded"
    (pack_root / CANDIDATE_POINT).mkdir(parents=True, exist_ok=True)
    (dec_root / CANDIDATE_POINT).mkdir(parents=True, exist_ok=True)

    byte_rows: list[dict[str, Any]] = []
    arms: list[dict[str, Any]] = []

    def build_arm(
        name: str,
        family: str,
        config: str,
        producer: str,
        encode_builder: Any,
        decode_builder: Any,
        ext: str,
    ) -> dict[str, Any]:
        pack_dir = pack_root / name
        dec_dir = dec_root / name
        pack_dir.mkdir(parents=True, exist_ok=True)
        dec_dir.mkdir(parents=True, exist_ok=True)
        encode_lines: list[str] = []
        decode_cmds: list[list[str]] = []
        outputs: list[Path] = []
        packed_paths: list[Path] = []
        total = 0
        for src in files:
            packed = pack_dir / f"{src.name}{ext}"
            argv, stdout_path = encode_builder(src, packed)
            run(argv, stdout_path=stdout_path)
            if not packed.is_file():
                raise RuntimeError(f"{name}: encoder produced no file for {src.name}")
            encode_lines.append(script_line([str(x) for x in argv], stdout_path))
            dargv, out_path = decode_builder(packed, dec_dir / src.name, src.stat().st_size)
            decode_cmds.append([str(x) for x in dargv])
            outputs.append(out_path)
            packed_paths.append(packed)
            total += packed.stat().st_size
            byte_rows.append({
                "corpus": args.corpus,
                "file": src.name,
                "input_bytes": src.stat().st_size,
                "source_sha256": source_digest[src.name],
                "codec": name,
                "compressed_bytes": packed.stat().st_size,
                "ratio": f"{packed.stat().st_size / src.stat().st_size:.9f}",
            })
        # Correctness before timing: every payload must round-trip byte-exactly.
        for src, cmd, out_path in zip(files, decode_cmds, outputs):
            run(cmd)
            if not out_path.is_file():
                raise RuntimeError(f"{name}: decoder produced no output for {src.name}")
            if (out_path.stat().st_size != src.stat().st_size
                    or sha256(out_path) != source_digest[src.name]):
                raise RuntimeError(f"{name}: roundtrip mismatch for {src.name}")
        write_script_lines(out / f"encode-{name}.sh", encode_lines)
        write_script(out / f"decode-{name}.sh", decode_cmds)
        observed = pack_dir / f"{biggest.name}{ext}"
        return {
            "name": name,
            "family": family,
            "config": config,
            "producer": producer,
            "status": "OK",
            "compressed_bytes": total,
            "encode_script": f"encode-{name}.sh",
            "decode_script": f"decode-{name}.sh",
            "observed_file": str(observed),
            "observed_file_bytes": observed.stat().st_size,
        }

    # ---- candidate arm ----
    def cand_encode(src: Path, packed: Path):
        return ([str(anvil), "c", str(src), str(packed),
                 "--parse=ratio", "--ratio-backend=auto",
                 "--ratio-context=off", "--ratio-lines=off",
                 "--bwt-aux=on", "--quiet", *extra], None)

    def cand_decode(packed: Path, decoded: Path, n: int):
        return ([str(anvil), "d", str(packed), str(decoded), "--quiet"], decoded)

    cand_arm = build_arm(
        CANDIDATE_POINT, "anvil",
        "--parse=ratio --ratio-backend=auto --ratio-context=off "
        "--ratio-lines=off --bwt-aux=on"
        + ((" " + " ".join(extra)) if extra else ""),
        f"candidate binary {anvil.name} (sha256 {sha256(anvil)})",
        cand_encode, cand_decode, ".anv",
    )
    arms.append(cand_arm)

    # ---- A/A null scripts ----
    # whole-corpus null: identical candidate encodes into a separate tree.
    null_dir = out / "packed-null"
    null_dir.mkdir(parents=True, exist_ok=True)
    null_lines: list[str] = []
    for src in files:
        packed = null_dir / f"{src.name}.anv"
        argv, stdout_path = cand_encode(src, packed)
        null_lines.append(script_line([str(x) for x in argv], stdout_path))
    write_script_lines(out / f"encode-{CANDIDATE_POINT}-null.sh", null_lines)

    # reduced null: the three smallest corpus files (wall-clock budget).
    small = sorted(files, key=lambda p: p.stat().st_size)[:3]
    small_dir = out / "packed-null-small"
    small_dir.mkdir(parents=True, exist_ok=True)
    small_lines: list[str] = []
    for src in small:
        packed = small_dir / f"{src.name}.anv"
        argv, stdout_path = cand_encode(src, packed)
        small_lines.append(script_line([str(x) for x in argv], stdout_path))
    write_script_lines(out / "encode-null-small.sh", small_lines)
    small_observed = small_dir / f"{max(small, key=lambda p: p.stat().st_size).name}.anv"

    # ---- reference points ----
    for point in points:
        encode_builder, decode_builder = front_point_commands(point, args)
        ext = {"brotli": ".br", "zstd": ".zst", "xz": ".xz", "lz4": ".lz4"}[point["family"]]
        producer = {
            "brotli": f"brotli_dense helper (brotli_lw-equivalent API, q/lgwin parameterised)",
            "zstd": "distribution zstd",
            "xz": "distribution xz",
            "lz4": "distribution lz4",
        }[point["family"]]
        arms.append(build_arm(point["name"], point["family"], point["config"],
                              producer, encode_builder, decode_builder, ext))

    # ---- bytes.csv ----
    fields = ["corpus", "file", "input_bytes", "source_sha256", "codec",
              "compressed_bytes", "ratio"]
    with (out / "front-bytes.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(byte_rows)

    total_by_arm = {a["name"]: a["compressed_bytes"] for a in arms}
    if args.expected_candidate_total is not None:
        got = total_by_arm[CANDIDATE_POINT]
        if got != args.expected_candidate_total:
            raise RuntimeError(
                f"candidate byte identity failed: {got} != {args.expected_candidate_total}")

    summary = {
        "schema": 1,
        "corpus": args.corpus,
        "source_bytes": source_total,
        "file_count": len(files),
        "source_sha256": source_digest,
        "candidate": cand_arm,
        "arms": arms,
        "compressed_bytes": total_by_arm,
        "candidate_extra_args": extra,
        "roundtrip": "OK",
        "null_scripts": {
            "whole_corpus": f"encode-{CANDIDATE_POINT}-null.sh",
            "whole_corpus_observed": str(pack_root / CANDIDATE_POINT / f"{biggest.name}.anv"),
            "reduced": "encode-null-small.sh",
            "reduced_files": [p.name for p in small],
            "reduced_bytes": sum(p.stat().st_size for p in small),
            "reduced_observed": str(small_observed),
        },
        "observed_files": {a["name"]: a["observed_file"] for a in arms},
    }
    (out / "front-summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({
        "corpus": args.corpus,
        "source_bytes": source_total,
        "arms": [{"name": a["name"], "family": a["family"],
                  "compressed_bytes": a["compressed_bytes"]} for a in arms],
    }, indent=2, sort_keys=True))


def _paired_point_status(
    pair: dict[str, Any],
    null_ok: bool,
) -> tuple[str, dict[str, Any]]:
    """Apply the frozen encode-axis rule to one paired series."""
    ci = [float(x) for x in pair.get("paired_ratio_ci95", [])]
    ratio = float(pair.get("paired_ratio_candidate_over_control", float("nan")))
    detail = {
        "ratio": ratio,
        "ci95": ci,
        "reference_median_s": (pair.get("control") or {}).get("median_s"),
        "candidate_median_s": (pair.get("candidate") or {}).get("median_s"),
        "timing_valid": bool(pair.get("timing_valid")),
    }
    if not null_ok or not pair.get("timing_valid") or len(ci) != 2:
        detail["status"] = "TIMING_BLOCKED"
        return "TIMING_BLOCKED", detail
    if ci[1] <= 1.0:
        detail["status"] = "PASS"
        return "PASS", detail
    if ci[0] > 1.0:
        detail["status"] = "FAIL"
        return "FAIL", detail
    detail["status"] = "INCONCLUSIVE"
    return "INCONCLUSIVE", detail


def encode_axis(args: argparse.Namespace) -> None:
    """Rule the encode axis for the reference points measured in this job."""
    front = load_json(args.front)
    null_pair = load_json(args.null_pair) if args.null_pair else {}
    null_ci = [float(x) for x in null_pair.get("paired_ratio_ci95", [])]
    null_spans = bool(len(null_ci) == 2 and null_ci[0] <= 1.0 <= null_ci[1])
    null_ok = bool(null_pair.get("timing_valid")) and null_spans

    points: list[dict[str, Any]] = []
    for item in args.paired:
        if "=" not in item:
            raise RuntimeError(f"--paired expects name=path, got {item!r}")
        name, path = item.split("=", 1)
        pair = load_json(Path(path))
        status, detail = _paired_point_status(pair, null_ok)
        meta = next((a for a in front.get("arms", []) if a["name"] == name), None)
        record = {
            "name": name,
            "family": (meta or {}).get("family"),
            "config": (meta or {}).get("config"),
            "producer": (meta or {}).get("producer"),
            "reference_bytes": (meta or {}).get("compressed_bytes"),
            "candidate_bytes": (front.get("candidate") or {}).get("compressed_bytes"),
            "status": status,
        }
        record.update(detail)
        if record["candidate_bytes"] is not None and record["reference_bytes"] is not None:
            record["candidate_bytes_below_reference"] = bool(
                record["candidate_bytes"] < record["reference_bytes"])
        points.append(record)

    counts = {s: sum(1 for p in points if p["status"] == s)
              for s in ("PASS", "FAIL", "INCONCLUSIVE", "TIMING_BLOCKED")}
    if counts["FAIL"]:
        axis_status = "FAIL"
    elif counts["INCONCLUSIVE"] or counts["TIMING_BLOCKED"] or not points:
        axis_status = "INCONCLUSIVE"
    else:
        axis_status = "PASS"

    result = {
        "schema": 1,
        "corpus": front.get("corpus"),
        "cell": args.cell,
        "rule": (
            "for every reference point r: the 95% CI upper bound of the paired "
            "candidate/reference whole-corpus encode-time ratio must be <= 1.0 "
            "(PASS); CI lower bound > 1.0 is FAIL (provably slower); a CI that "
            "straddles 1.0 is INCONCLUSIVE; a series whose A/A null CI does not "
            "span 1.0 or whose timing is invalid is TIMING_BLOCKED."
        ),
        "null": {
            "ci95": null_ci,
            "spans_one": null_spans,
            "timing_valid": bool(null_pair.get("timing_valid")),
            "workload": args.null_workload or "",
            "source": str(args.null_pair) if args.null_pair else "",
        },
        "candidate": {
            "name": CANDIDATE_POINT,
            "config": (front.get("candidate") or {}).get("config"),
            "compressed_bytes": (front.get("candidate") or {}).get("compressed_bytes"),
        },
        "front_arms": front.get("arms", []),
        "observed_files": front.get("observed_files", {}),
        "null_scripts": front.get("null_scripts", {}),
        "points": points,
        "counts": counts,
        "axis": {"status": axis_status},
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n",
                         encoding="utf-8")
    print(json.dumps({"cell": args.cell, "axis": axis_status, "counts": counts},
                     indent=2, sort_keys=True))


def encode_merge(args: argparse.Namespace) -> None:
    """Merge the per-cell encode-axis rulings into the whole-front ruling."""
    merged_points: list[dict[str, Any]] = []
    nulls: list[dict[str, Any]] = []
    front_arms: dict[str, Any] = {}
    candidate: dict[str, Any] = {}
    corpus = None
    for cell_path in args.cell:
        cell = load_json(Path(cell_path))
        corpus = corpus or cell.get("corpus")
        merged_points.extend(cell.get("points") or [])
        nulls.append({"cell": cell.get("cell"), "source": cell_path,
                      **(cell.get("null") or {})})
        candidate = candidate or (cell.get("candidate") or {})
        for arm in cell.get("front_arms") or []:
            front_arms[arm["name"]] = arm
    front = load_json(args.front) if args.front else {}
    for arm in front.get("arms", []):
        front_arms[arm["name"]] = arm

    counts = {s: sum(1 for p in merged_points if p.get("status") == s)
              for s in ("PASS", "FAIL", "INCONCLUSIVE", "TIMING_BLOCKED")}
    if counts["FAIL"]:
        axis_status = "FAIL"
    elif counts["INCONCLUSIVE"] or counts["TIMING_BLOCKED"] or not merged_points:
        axis_status = "INCONCLUSIVE"
    else:
        axis_status = "PASS"
    measured = {p["name"] for p in merged_points}
    missing = [name for name in FRONT_BY_NAME if name not in measured]

    result = {
        "schema": 1,
        "corpus": corpus,
        "rule": (
            "for every reference point r of the I11 dominance front: the 95% CI "
            "upper bound of the paired candidate/reference whole-corpus "
            "encode-time ratio must be <= 1.0 (PASS); CI lower bound > 1.0 is "
            "FAIL (provably slower); a CI that straddles 1.0 is INCONCLUSIVE; a "
            "series whose A/A null CI does not span 1.0 or whose timing is "
            "invalid is TIMING_BLOCKED and counts for nothing."
        ),
        "candidate": candidate,
        "points": merged_points,
        "counts": counts,
        "front_points_defined": [p["name"] for p in FRONT_POINTS],
        "front_points_not_measured": missing,
        "nulls": nulls,
        "front_arms": front_arms,
        "axis": {"status": axis_status},
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n",
                         encoding="utf-8")
    print(json.dumps({"axis": axis_status, "counts": counts,
                      "not_measured": missing}, indent=2, sort_keys=True))


def decoder_bytes(args: argparse.Namespace) -> None:
    """Rule the decoder-only binary-size axis."""
    manifest = load_json(args.manifest)
    cand_entry = manifest.get("candidate") or {}
    cand_path = Path(cand_entry.get("path", ""))
    cand_measured = cand_path.is_file()
    cand_bytes = cand_path.stat().st_size if cand_measured else None

    producers = manifest.get("producers") or {}
    refs: dict[str, Any] = {}
    for family, entry in producers.items():
        path = Path(entry.get("path", ""))
        measured = path.is_file()
        definition = DECODER_PRODUCER_DEFINITIONS.get(family, {})
        record = {
            "family": family,
            "artifact": definition.get("artifact", ""),
            "decoder_only": bool(definition.get("decoder_only", False)),
            "note": definition.get("note", ""),
            "path": str(path),
            "measured": measured,
            "bytes": path.stat().st_size if measured else None,
            "sha256": sha256(path) if measured else None,
            "version": entry.get("version", ""),
            "points": entry.get("points", []),
        }
        refined = manifest.get("decoder_only_refined", {}).get(family)
        if refined:
            rpath = Path(refined.get("path", ""))
            rmeasured = rpath.is_file()
            record["decoder_only_refined"] = {
                "path": str(rpath),
                "measured": rmeasured,
                "bytes": rpath.stat().st_size if rmeasured else None,
                "sha256": sha256(rpath) if rmeasured else None,
                "note": refined.get("note", ""),
            }
        else:
            record["decoder_only_refined"] = {
                "measured": False,
                "bytes": None,
                "note": refined_note(family),
            }
        refs[family] = record

    points: list[dict[str, Any]] = []
    for point in FRONT_POINTS:
        family = point["family"]
        ref = refs.get(family)
        ref_bytes = (ref or {}).get("bytes")
        if not cand_measured or ref is None or ref_bytes is None:
            status = "NOT_MEASURED"
        elif cand_bytes <= ref_bytes:
            status = "PASS"
        else:
            status = "FAIL"
        comparison = "NOT_MEASURED"
        if status != "NOT_MEASURED":
            cand_exact = bool(DECODER_PRODUCER_DEFINITIONS.get("anvil", {}).get("decoder_only"))
            ref_exact = bool((ref or {}).get("decoder_only"))
            comparison = ("EXACT_VS_EXACT" if cand_exact and ref_exact
                          else "UPPER_BOUND_VS_EXACT" if ref_exact
                          else "UPPER_BOUND_VS_UPPER_BOUND")
        points.append({
            "name": point["name"],
            "family": family,
            "config": point["config"],
            "candidate_bytes": cand_bytes,
            "reference_bytes": ref_bytes,
            "status": status,
            "comparison": comparison,
        })

    counts = {s: sum(1 for p in points if p["status"] == s)
              for s in ("PASS", "FAIL", "NOT_MEASURED")}
    if counts["FAIL"]:
        axis_status = "FAIL"
    elif counts["NOT_MEASURED"] or not points:
        axis_status = "INCONCLUSIVE"
    else:
        axis_status = "PASS"

    result = {
        "schema": 1,
        "definition": DECODER_SIZE_DEFINITION,
        "producer_definitions": DECODER_PRODUCER_DEFINITIONS,
        "rule": (
            "the candidate's decoder-providing artifact size must be <= every "
            "reference point's decoder-providing artifact size, under the "
            "definition above; an unmeasurable reference is NOT_MEASURED and is "
            "never counted as a pass."
        ),
        "candidate": {
            "name": cand_entry.get("name", CANDIDATE_POINT),
            "path": str(cand_path),
            "measured": cand_measured,
            "bytes": cand_bytes,
            "sha256": sha256(cand_path) if cand_measured else None,
            "decoder_only": bool(DECODER_PRODUCER_DEFINITIONS["anvil"]["decoder_only"]),
            "note": DECODER_PRODUCER_DEFINITIONS["anvil"]["note"],
            "version": cand_entry.get("version", ""),
        },
        "references": refs,
        "points": points,
        "counts": counts,
        "axis": {"status": axis_status},
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n",
                         encoding="utf-8")
    print(json.dumps({"axis": axis_status, "counts": counts}, indent=2, sort_keys=True))


def refined_note(family: str) -> str:
    notes = {
        "anvil": ("NOT_MEASURED: the candidate's decoder cannot be separated at "
                  "object level (one translation unit, src/anvil.cpp); the "
                  "`anvil` executable is recorded as the upper bound."),
        "brotli": ("NOT_MEASURED: no decoder-only static archive was built in "
                   "this job (the shared object libbrotlidec.so is the exact "
                   "decoder-only size)."),
        "zstd": ("NOT_MEASURED: the distribution ships one combined libzstd; a "
                 "decoder-only source-subset archive was not built in this job."),
        "xz": ("NOT_MEASURED: the distribution ships one combined liblzma; a "
               "decoder-only source-subset archive was not built in this job."),
        "lz4": ("NOT_MEASURED: upstream ships no separable decoder-only library "
                "for lz4 (the decoder shares a translation unit with the "
                "encoder)."),
    }
    return notes.get(family, "NOT_MEASURED: no decoder-only refinement available.")


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
    p.add_argument(
        "--candidate-extra-args",
        default="",
        help=(
            "Space-separated (shlex-quoted) extra ANVIL encoder-policy flags "
            "appended to the candidate arm, e.g. "
            "'--bwt-aux-walks 16384 --bwt-subblock 4194304'. Empty by default, "
            "which reproduces the pre-I20 command byte for byte. A single "
            "string is used rather than nargs='*' because argparse would "
            "otherwise treat a leading '--flag' as an option terminator."
        ),
    )
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
    # I11 (D5) additions.  Both are OPTIONAL: when they are absent the
    # ruling is byte-for-byte the frozen I20 ruling.
    p.add_argument("--encode", type=Path,
                   help="I11 encode-axis ruling (encode.json)")
    p.add_argument("--decoder-bytes", type=Path,
                   help="I11 decoder-only binary-size ruling (decoder-bytes.json)")
    p.set_defaults(func=summarize)

    # ---------------- I11 (D5) extension ----------------
    p = sub.add_parser("prepare-front")
    p.add_argument("--corpus", choices=("silesia", "enwik8"), required=True)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--brotli-dense", type=Path, required=True)
    p.add_argument("--xz", type=Path, required=True)
    p.add_argument("--zstd", type=Path, required=True)
    p.add_argument("--lz4", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--points", default="",
                   help="comma-separated front point names to build "
                        "(default: all 17)")
    p.add_argument("--candidate-extra-args", default="")
    p.add_argument("--expected-candidate-total", type=int)
    p.set_defaults(func=prepare_front)

    p = sub.add_parser("encode-axis")
    p.add_argument("--front", type=Path, required=True)
    p.add_argument("--paired", action="append", default=[],
                   help="name=path of a paired encode series")
    p.add_argument("--null-pair", type=Path)
    p.add_argument("--null-workload", default="")
    p.add_argument("--cell", default="")
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=encode_axis)

    p = sub.add_parser("encode-merge")
    p.add_argument("--cell", type=Path, action="append", default=[])
    p.add_argument("--front", type=Path)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=encode_merge)

    p = sub.add_parser("decoder-bytes")
    p.add_argument("--manifest", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=decoder_bytes)
    return ap


def main() -> None:
    args = build_parser().parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
