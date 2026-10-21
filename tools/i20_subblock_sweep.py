#!/usr/bin/env python3
"""I20 / C3 — BWT subblock-cap sweep for the external reference-cost gate.

This tool does not alter codec behaviour. It exercises the existing revision-2
mode-17 ratio envelope (BWT + auxiliary index) with a grid of ``--bwt-subblock``
caps, reconstructs the canonical ``--ratio-backend=auto`` portfolio exactly the
way ``encode_ratio_block()`` does (Brotli is considered first and is replaced
only on a strict size improvement), verifies byte-exact round trips, measures
exact charged bytes, per-file peak decode RSS and whole-codec decode time, and
drives the repo's own arbiter (``tools/reference_cost_gate.py summarize``) for
every timed cap so the ruling token is produced by the repo, not re-derived.

The binding references (``xz -9e`` and Brotli q11/lgwin30 via the pinned
``brotli_lw`` helper) are prepared and measured **in the same job**, which the
I20 evidence contract requires for the G3/G4/G5/G6 gates.

Sub-commands
-------------
prepare    encode every arm, verify round trips, emit decode scripts + manifests
rss        per-file peak decode RSS for every cap and both references
verify     re-decode every arm and re-hash every payload (identity across reps)
timing     same-job paired/interleaved timing through tools/paired_bench.py
summarize  build the per-cap table, gate verdicts and arbiter rulings

Every measurement is expected to run on GitHub Actions; the tool itself is
location agnostic and performs no arithmetic on corpus bytes.
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


SILESIA = (
    "dickens", "mozilla", "mr", "nci", "ooffice", "osdb",
    "reymont", "samba", "sao", "webster", "x-ray", "xml",
)

CONTROL_CAP_MIB = 128

# Frozen canonical totals (GitHub Actions run 35927623136) used as hard
# byte-identity gates for the 128 MiB control and as cross-checks for the
# two binding references.
EXPECTED_CONTROL_BYTES = {
    "silesia": 46_466_339,
    "enwik8": 23_537_422,
}
EXPECTED_REFERENCE_BYTES = {
    "silesia": {"xz": 48_456_004, "brotli": 49_383_136},
    "enwik8": {"xz": 24_831_648, "brotli": 24_810_180},
}

MIB = 1024 * 1024


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
        raise argparse.ArgumentTypeError(f"caps must include the {CONTROL_CAP_MIB} MiB control")
    return values


def parse_cap_list(text: str) -> list[int]:
    """Optional cap list (MiB). Empty means 'none'."""
    if not text.strip():
        return []
    return sorted({int(x.strip()) for x in text.split(",") if x.strip()})


def run(argv: list[str], *, stdout_path: Path | None = None) -> None:
    handle = None
    out: Any = subprocess.DEVNULL
    try:
        if stdout_path is not None:
            stdout_path.parent.mkdir(parents=True, exist_ok=True)
            handle = stdout_path.open("wb")
            out = handle
        p = subprocess.run(argv, stdout=out, stderr=subprocess.PIPE, check=False)
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
    with tempfile.TemporaryDirectory(prefix="anvil-i20-subblock-time-") as td0:
        stamp = Path(td0) / "time.txt"
        handle = None
        out: Any = subprocess.DEVNULL
        try:
            if stdout_path is not None:
                stdout_path.parent.mkdir(parents=True, exist_ok=True)
                handle = stdout_path.open("wb")
                out = handle
            p = subprocess.run(
                [str(time_bin), "-f", "%e %M", "-o", str(stamp), "--", *argv],
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


def anvil_brotli_cmd(anvil: Path, src: Path, dst: Path) -> list[str]:
    # Mirrors the canonical ratio route with the Brotli backend forced. The
    # Brotli payload does not depend on the BWT options, so this is also the
    # exact payload the auto router would consider first.
    return [
        str(anvil), "c", str(src), str(dst),
        "--parse=ratio",
        "--ratio-backend=brotli",
        "--ratio-context=off",
        "--ratio-lines=off",
        "--quiet",
    ]


def anvil_bwt_cmd(anvil: Path, src: Path, dst: Path, cap_bytes: int) -> list[str]:
    return [
        str(anvil), "c", str(src), str(dst),
        "--parse=ratio",
        "--ratio-backend=bwt",
        "--ratio-context=off",
        "--ratio-lines=off",
        "--bwt-aux=on",
        f"--bwt-subblock={cap_bytes}",
        "--quiet",
    ]


# ---------------------------------------------------------------------------
# prepare
# ---------------------------------------------------------------------------


def prepare(args: argparse.Namespace) -> None:
    root = args.root.resolve()
    anvil = args.anvil.resolve()
    brotli = args.brotli.resolve()
    out = args.out.resolve()
    caps = parse_caps(args.caps_mib)
    files = canonical_files(args.corpus, root)
    if not anvil.is_file():
        raise RuntimeError(f"missing ANVIL executable: {anvil}")
    if not brotli.is_file():
        raise RuntimeError(f"missing brotli_lw helper: {brotli}")
    xz = shutil.which("xz")
    if not xz:
        raise RuntimeError("xz is required")

    out.mkdir(parents=True, exist_ok=True)
    packed = out / "packed"
    timed = out / "timed"
    for sub in ("anvil-brotli", "anvil-bwt", "xz", "brotli"):
        (packed / sub).mkdir(parents=True, exist_ok=True)
    for cap in caps:
        (packed / "anvil-bwt" / f"cap-{cap}").mkdir(parents=True, exist_ok=True)
        (timed / f"cap-{cap}").mkdir(parents=True, exist_ok=True)
    (timed / "xz").mkdir(parents=True, exist_ok=True)
    (timed / "brotli").mkdir(parents=True, exist_ok=True)

    source_rows: list[dict[str, Any]] = []
    brotli_meta: dict[str, dict[str, Any]] = {}
    bwt_meta: dict[tuple[str, int], dict[str, Any]] = {}
    ref_meta: dict[str, dict[str, Any]] = {}

    # ---- ANVIL Brotli-backend payload (cap independent, encoded once) ----
    for src in files:
        name = src.name
        bp = packed / "anvil-brotli" / f"{name}.anv"
        run(anvil_brotli_cmd(anvil, src, bp))
        info = ratio_info(bp)
        if info["backend"] != 1:
            raise RuntimeError(f"{name}: direct Brotli emitted backend {info['backend']}")
        brotli_meta[name] = {
            "path": str(bp),
            "bytes": bp.stat().st_size,
            "sha256": sha256(bp),
            "ratio_info": info,
        }

    # ---- ANVIL BWT-backend payload per cap ----
    # A cap >= file size produces the identical unsplit payload, so that
    # payload is encoded once and reused (as the frozen sibling tool does).
    for src in files:
        name = src.name
        n = src.stat().st_size
        encoded_by_effective: dict[int | str, dict[str, Any]] = {}
        for cap in caps:
            cap_bytes = cap * MIB
            split = cap_bytes < n
            key: int | str = cap if split else "unsplit"
            if key not in encoded_by_effective:
                bp = packed / "anvil-bwt" / f"cap-{cap}" / f"{name}.anv"
                run(anvil_bwt_cmd(anvil, src, bp, cap_bytes))
                info = ratio_info(bp)
                if info["backend"] != 2:
                    raise RuntimeError(f"{name}/cap-{cap}: direct BWT emitted backend {info['backend']}")
                expected_sub = ((n + cap_bytes - 1) // cap_bytes) if split else 1
                if info["subblocks"] != expected_sub:
                    raise RuntimeError(
                        f"{name}/cap-{cap}: subblocks={info['subblocks']} expected={expected_sub}"
                    )
                encoded_by_effective[key] = {
                    "path": str(bp),
                    "bytes": bp.stat().st_size,
                    "sha256": sha256(bp),
                    "subblocks": info["subblocks"],
                    "split": split,
                    "physical_cap_mib": cap,
                    "ratio_info": info,
                }
            meta = dict(encoded_by_effective[key])
            meta["requested_cap_mib"] = cap
            meta["reused_unsplit"] = key == "unsplit" and cap != meta["physical_cap_mib"]
            bwt_meta[(name, cap)] = meta

    # ---- binding references: xz -9e and Brotli q11/lgwin30 ----
    for src in files:
        name = src.name
        n = src.stat().st_size
        digest = sha256(src)
        source_rows.append({
            "file": name,
            "input_bytes": n,
            "source_sha256": digest,
        })

        xp = packed / "xz" / f"{name}.xz"
        run([xz, "-9e", "-c", str(src)], stdout_path=xp)
        xz_timed = timed / "xz" / f"{name}.xz"
        shutil.copyfile(xp, xz_timed)
        xz_out = timed / "xz" / name
        xz_cmd = [xz, "-d", "-k", "-f", str(xz_timed)]

        bp = packed / "brotli" / f"{name}.br"
        run([str(brotli), "c", str(src), str(bp)])
        br_timed = timed / "brotli" / f"{name}.br"
        shutil.copyfile(bp, br_timed)
        br_out = timed / "brotli" / name
        br_cmd = [str(brotli), "d", str(br_timed), str(br_out), str(n)]

        ref_meta[name] = {
            "xz": {"path": str(xp), "bytes": xp.stat().st_size, "sha256": sha256(xp),
                   "decode_cmd": xz_cmd, "output": str(xz_out)},
            "brotli": {"path": str(bp), "bytes": bp.stat().st_size, "sha256": sha256(bp),
                       "decode_cmd": br_cmd, "output": str(br_out)},
        }

    # ---- auto-router reconstruction per cap + round-trip verification ----
    rows: list[dict[str, Any]] = []
    commands_by_cap: dict[int, list[list[str]]] = {cap: [] for cap in caps}
    manifests: dict[str, str] = {}

    for cap in caps:
        for src in files:
            name = src.name
            bm = brotli_meta[name]
            wm = bwt_meta[(name, cap)]
            # encode_ratio_block() considers Brotli first and replaces it only
            # on a strict size improvement: Brotli wins ties.
            route = "bwt" if int(wm["bytes"]) < int(bm["bytes"]) else "brotli"
            selected = wm if route == "bwt" else bm
            dec = timed / f"cap-{cap}" / name
            cmd = [str(anvil), "d", str(selected["path"]), str(dec), "--quiet"]
            run(cmd)
            if dec.stat().st_size != src.stat().st_size or sha256(dec) != sha256(src):
                raise RuntimeError(f"roundtrip mismatch: {name}/cap-{cap}/{route}")
            commands_by_cap[cap].append(cmd)
            rows.append({
                "corpus": args.corpus,
                "file": name,
                "input_bytes": src.stat().st_size,
                "cap_mib": cap,
                "route": route,
                "selected_bytes": int(selected["bytes"]),
                "selected_sha256": selected["sha256"],
                "anvil_brotli_bytes": int(bm["bytes"]),
                "anvil_bwt_bytes": int(wm["bytes"]),
                "bwt_subblocks": int(wm["subblocks"]),
                "bwt_split": bool(wm["split"]),
                "bwt_reused_unsplit": bool(wm["reused_unsplit"]),
                "xz_bytes": int(ref_meta[name]["xz"]["bytes"]),
                "brotli_ref_bytes": int(ref_meta[name]["brotli"]["bytes"]),
            })

        script = out / f"decode-cap-{cap}.sh"
        write_script(script, commands_by_cap[cap])
        manifest = out / f"decode-cap-{cap}.json"
        manifest.write_text(
            json.dumps({
                "schema": 1,
                "corpus": args.corpus,
                "cap_mib": cap,
                "files": [p.name for p in files],
                "commands": commands_by_cap[cap],
            }, indent=2) + "\n",
            encoding="utf-8",
        )
        manifests[str(cap)] = str(manifest)

    # ---- reference decode scripts (same job as the candidate arms) ----
    for arm in ("xz", "brotli"):
        cmds = [ref_meta[p.name][arm]["decode_cmd"] for p in files]
        for p in files:
            out_path = Path(ref_meta[p.name][arm]["output"])
            run(ref_meta[p.name][arm]["decode_cmd"])
            if out_path.stat().st_size != p.stat().st_size or sha256(out_path) != sha256(p):
                raise RuntimeError(f"reference roundtrip mismatch: {p.name}/{arm}")
        script = out / f"decode-{arm}.sh"
        write_script(script, cmds)
        manifest = out / f"decode-{arm}.json"
        manifest.write_text(
            json.dumps({
                "schema": 1,
                "corpus": args.corpus,
                "arm": arm,
                "files": [p.name for p in files],
                "commands": cmds,
            }, indent=2) + "\n",
            encoding="utf-8",
        )
        manifests[arm] = str(manifest)

    # ---- per-cap aggregates ----
    by_cap: dict[int, dict[str, Any]] = {}
    for cap in caps:
        cr = [r for r in rows if r["cap_mib"] == cap]
        by_cap[cap] = {
            "cap_mib": cap,
            "cap_bytes": cap * MIB,
            "selected_bytes": sum(int(r["selected_bytes"]) for r in cr),
            "bwt_direct_bytes": sum(int(r["anvil_bwt_bytes"]) for r in cr),
            "bwt_routes": sum(1 for r in cr if r["route"] == "bwt"),
            "brotli_routes": sum(1 for r in cr if r["route"] == "brotli"),
            "files": len(cr),
        }

    control_total = by_cap[CONTROL_CAP_MIB]["selected_bytes"]
    expected = EXPECTED_CONTROL_BYTES[args.corpus]
    if control_total != expected:
        raise RuntimeError(
            f"128 MiB portfolio byte identity failed: {control_total} != {expected}"
        )

    xz_total = sum(int(r["xz_bytes"]) for r in rows if r["cap_mib"] == CONTROL_CAP_MIB)
    br_total = sum(int(r["brotli_ref_bytes"]) for r in rows if r["cap_mib"] == CONTROL_CAP_MIB)
    exp_ref = EXPECTED_REFERENCE_BYTES[args.corpus]
    reference_check = {
        "xz_bytes": xz_total,
        "xz_expected": exp_ref["xz"],
        "xz_matches_baseline": xz_total == exp_ref["xz"],
        "brotli_bytes": br_total,
        "brotli_expected": exp_ref["brotli"],
        "brotli_matches_baseline": br_total == exp_ref["brotli"],
    }

    fields = [
        "corpus", "file", "input_bytes", "cap_mib", "route",
        "selected_bytes", "selected_sha256", "anvil_brotli_bytes", "anvil_bwt_bytes",
        "bwt_subblocks", "bwt_split", "bwt_reused_unsplit",
        "xz_bytes", "brotli_ref_bytes",
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
        "reference_bytes": reference_check,
        "by_cap": {str(k): v for k, v in by_cap.items()},
        "decode_manifests": manifests,
        "decode_scripts": {str(cap): str(out / f"decode-cap-{cap}.sh") for cap in caps},
        "reference_scripts": {"xz": str(out / "decode-xz.sh"), "brotli": str(out / "decode-brotli.sh")},
        "source": source_rows,
        "payload_sha256": {
            "anvil_brotli": {k: v["sha256"] for k, v in brotli_meta.items()},
            "anvil_bwt": {f"{name}@{cap}": v["sha256"] for (name, cap), v in bwt_meta.items()},
            "xz": {k: v["xz"]["sha256"] for k, v in ref_meta.items()},
            "brotli": {k: v["brotli"]["sha256"] for k, v in ref_meta.items()},
        },
        "payload_bytes": {
            "anvil_brotli": {k: v["bytes"] for k, v in brotli_meta.items()},
            "anvil_bwt": {f"{name}@{cap}": v["bytes"] for (name, cap), v in bwt_meta.items()},
            "xz": {k: v["xz"]["bytes"] for k, v in ref_meta.items()},
            "brotli": {k: v["brotli"]["bytes"] for k, v in ref_meta.items()},
        },
        # Physical on-disk path of every payload. A cap larger than a file
        # produces the identical unsplit payload, so several keys share one
        # physical file; this map records where each payload actually lives.
        "payload_paths": {
            "anvil_brotli": {k: v["path"] for k, v in brotli_meta.items()},
            "anvil_bwt": {f"{name}@{cap}": v["path"] for (name, cap), v in bwt_meta.items()},
            "xz": {k: v["xz"]["path"] for k, v in ref_meta.items()},
            "brotli": {k: v["brotli"]["path"] for k, v in ref_meta.items()},
        },
    }
    (out / "prepare.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps({
        "corpus": args.corpus,
        "caps_mib": caps,
        "control_identity": "OK",
        "control_bytes": control_total,
        "reference_bytes": reference_check,
        "by_cap": summary["by_cap"],
    }, indent=2, sort_keys=True))


# ---------------------------------------------------------------------------
# rss
# ---------------------------------------------------------------------------


def rss(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    caps = [int(x) for x in prep["caps_mib"]]
    corpus = str(prep["corpus"])

    rows: list[dict[str, Any]] = []
    by_arm: dict[str, Any] = {}

    def measure(arm: str, label: str, commands: list[list[str]], files: list[str]) -> None:
        samples = []
        for i, (name, cmd) in enumerate(zip(files, commands)):
            elapsed, peak = run_time_rss(cmd)
            row = {
                "corpus": corpus,
                "arm": arm,
                "cap_mib": label,
                "file_index": i,
                "file": name,
                "seconds": elapsed,
                "peak_rss_kib": peak,
            }
            rows.append(row)
            samples.append(row)
        by_arm[arm] = {
            "arm": arm,
            "cap_mib": label,
            "sum_seconds": sum(float(r["seconds"]) for r in samples),
            "max_peak_rss_kib": max(int(r["peak_rss_kib"]) for r in samples),
            "per_file": samples,
        }

    for cap in caps:
        manifest = json.loads(Path(prep["decode_manifests"][str(cap)]).read_text(encoding="utf-8"))
        if len(manifest["commands"]) != len(manifest["files"]):
            raise RuntimeError(f"cap-{cap}: command/file manifest mismatch")
        measure(f"cap-{cap}", cap, manifest["commands"], manifest["files"])

    for arm in ("xz", "brotli"):
        manifest = json.loads(Path(prep["decode_manifests"][arm]).read_text(encoding="utf-8"))
        measure(arm, arm, manifest["commands"], manifest["files"])

    with (out / "rss.csv").open("w", newline="", encoding="utf-8") as f:
        fields = ["corpus", "arm", "cap_mib", "file_index", "file", "seconds", "peak_rss_kib"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)

    result = {
        "schema": 1,
        "corpus": corpus,
        "by_arm": by_arm,
        "max_rss_kib": {arm: int(v["max_peak_rss_kib"]) for arm, v in by_arm.items()},
    }
    (out / "rss.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(result["max_rss_kib"], indent=2, sort_keys=True))


# ---------------------------------------------------------------------------
# verify
# ---------------------------------------------------------------------------


def verify(args: argparse.Namespace) -> None:
    """Re-decode every arm and re-hash every payload after the timed pass."""
    out = args.out.resolve()
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    corpus = str(prep["corpus"])
    caps = [int(x) for x in prep["caps_mib"]]
    files = prep["source"]
    anvil = Path(args.anvil).resolve()
    if not anvil.is_file():
        raise RuntimeError(f"missing ANVIL executable: {anvil}")

    results: list[dict[str, Any]] = []
    all_ok = True

    for cap in caps:
        manifest = json.loads(Path(prep["decode_manifests"][str(cap)]).read_text(encoding="utf-8"))
        for name, cmd in zip(manifest["files"], manifest["commands"]):
            dec = Path(cmd[3])
            run(cmd)
            src_hash = next(f["source_sha256"] for f in files if f["file"] == name)
            digest = sha256(dec)
            ok = digest == src_hash
            all_ok = all_ok and ok
            results.append({
                "corpus": corpus, "arm": f"cap-{cap}", "file": name,
                "decoded_sha256": digest, "expected_sha256": src_hash, "match": ok,
            })

    for arm in ("xz", "brotli"):
        manifest = json.loads(Path(prep["decode_manifests"][arm]).read_text(encoding="utf-8"))
        for name, cmd in zip(manifest["files"], manifest["commands"]):
            out_path = Path(cmd[3] if arm == "brotli" else str(Path(cmd[-1]).with_suffix("")))
            run(cmd)
            src_hash = next(f["source_sha256"] for f in files if f["file"] == name)
            digest = sha256(out_path)
            ok = digest == src_hash
            all_ok = all_ok and ok
            results.append({
                "corpus": corpus, "arm": arm, "file": name,
                "decoded_sha256": digest, "expected_sha256": src_hash, "match": ok,
            })

    # Payload identity: the compressed bytes must be unchanged by the timed
    # repetitions (the timed arms decode these exact files).
    payload_check: dict[str, Any] = {}
    rehashed: dict[str, dict[str, str]] = {}
    paths = prep.get("payload_paths", {})
    for group, entries in prep["payload_sha256"].items():
        mismatches: list[str] = []
        group_paths = paths.get(group, {})
        for key, digest in entries.items():
            if key in group_paths:
                path = Path(group_paths[key])
            elif group == "anvil_brotli":
                path = out / "packed" / "anvil-brotli" / f"{key}.anv"
            elif group == "anvil_bwt":
                name, cap = key.split("@")
                path = out / "packed" / "anvil-bwt" / f"cap-{cap}" / f"{name}.anv"
            elif group == "xz":
                path = out / "packed" / "xz" / f"{key}.xz"
            else:
                path = out / "packed" / "brotli" / f"{key}.br"
            if not path.is_file():
                all_ok = False
                mismatches.append(key)
                continue
            got = sha256(path)
            rehashed.setdefault(group, {})[key] = got
            if got != digest:
                all_ok = False
                mismatches.append(key)
        payload_check[group] = {"entries": len(entries), "mismatches": mismatches}

    outp = {
        "schema": 1,
        "corpus": corpus,
        "roundtrip_all_match": all_ok,
        "roundtrip_rows": results,
        "payload_sha256_rehashed": rehashed,
        "payload_check": payload_check,
    }
    (out / "verify.json").write_text(
        json.dumps(outp, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps({
        "corpus": corpus,
        "roundtrip_all_match": all_ok,
        "payload_groups": sorted(payload_check),
        "payload_mismatches": {k: v.get("mismatches", []) for k, v in payload_check.items()},
    }, indent=2, sort_keys=True))
    if not all_ok:
        raise RuntimeError("verification failed")


# ---------------------------------------------------------------------------
# timing
# ---------------------------------------------------------------------------


def paired(
    driver: Path,
    out_dir: Path,
    label: str,
    control: list[str],
    candidate: list[str],
    *,
    reps: int,
    warmups: int,
    seed: int,
    bootstrap: int,
    epsilon: float,
    max_cv: float,
    cpu: int,
    ambient_reps: int,
    ambient_iterations: int,
    ambient_max_cv: float,
    timeout_s: float,
) -> None:
    cmd = [
        sys.executable, str(driver),
        "--control-json", json.dumps(control),
        "--candidate-json", json.dumps(candidate),
        "--reps", str(reps),
        "--warmups", str(warmups),
        "--seed", str(seed),
        "--bootstrap", str(bootstrap),
        "--epsilon", str(epsilon),
        "--max-cv", str(max_cv),
        "--cpu", str(cpu),
        "--ambient-reps", str(ambient_reps),
        "--ambient-iterations", str(ambient_iterations),
        "--ambient-max-cv", str(ambient_max_cv),
        "--timeout-s", str(timeout_s),
        "--out", str(out_dir / f"paired-{label}.csv"),
        "--summary", str(out_dir / f"paired-{label}.json"),
        "--stdout-dir", str(out_dir / f"stdout-{label}"),
    ]
    print(f"RUN {label}", flush=True)
    print("  " + " ".join(shlex.quote(c) for c in cmd), flush=True)
    subprocess.run(cmd, check=True)


def timing(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    driver = Path(args.driver).resolve()
    if not driver.is_file():
        raise RuntimeError(f"missing paired_bench driver: {driver}")
    control_cap = int(prep["control_cap_mib"])
    timed_caps = parse_cap_list(args.timed_caps)
    for cap in timed_caps:
        if cap not in prep["caps_mib"]:
            raise RuntimeError(f"timed cap {cap} is not in the prepared cap set")

    def script(cap: int) -> list[str]:
        return ["bash", str(out / f"decode-cap-{cap}.sh")]

    def ref(arm: str) -> list[str]:
        return ["bash", str(out / f"decode-{arm}.sh")]

    common = dict(
        reps=args.reps,
        warmups=args.warmups,
        seed=args.seed,
        bootstrap=args.bootstrap,
        epsilon=args.epsilon,
        max_cv=args.max_cv,
        cpu=args.cpu,
        ambient_reps=args.ambient_reps,
        ambient_iterations=args.ambient_iterations,
        ambient_max_cv=args.ambient_max_cv,
        timeout_s=args.timeout_s,
    )

    # A/A null: the control portfolio against itself. Mandatory in any job that
    # produces a claim-bearing timing number.
    paired(driver, out, "null", script(control_cap), script(control_cap), **common)

    # The control arm against both binding references (in-job baseline).
    paired(driver, out, f"cap-{control_cap}-vs-xz", ref("xz"), script(control_cap), **common)
    paired(driver, out, f"cap-{control_cap}-vs-brotli", ref("brotli"), script(control_cap), **common)

    for cap in timed_caps:
        paired(driver, out, f"cap-{cap}-vs-control", script(control_cap), script(cap), **common)
        paired(driver, out, f"cap-{cap}-vs-xz", ref("xz"), script(cap), **common)
        paired(driver, out, f"cap-{cap}-vs-brotli", ref("brotli"), script(cap), **common)

    index = {
        "schema": 1,
        "corpus": prep["corpus"],
        "control_cap_mib": control_cap,
        "timed_caps_mib": timed_caps,
        "driver": str(driver),
        "driver_sha256": sha256(driver),
        "parameters": {
            "reps": args.reps, "warmups": args.warmups, "seed": args.seed,
            "bootstrap": args.bootstrap, "epsilon": args.epsilon, "max_cv": args.max_cv,
            "cpu": args.cpu, "ambient_reps": args.ambient_reps,
            "ambient_iterations": args.ambient_iterations,
            "ambient_max_cv": args.ambient_max_cv, "timeout_s": args.timeout_s,
        },
    }
    (out / "timing-index.json").write_text(
        json.dumps(index, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(index, indent=2, sort_keys=True))


# ---------------------------------------------------------------------------
# summarize
# ---------------------------------------------------------------------------


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def pair_or_none(path: Path) -> dict[str, Any] | None:
    if not path.is_file():
        return None
    try:
        return load_json(path)
    except json.JSONDecodeError:
        return None


def summarize(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    prep = load_json(args.prepare)
    rss_data = load_json(args.rss)
    timing_dir = args.timing_dir.resolve()
    caps = [int(x) for x in prep["caps_mib"]]
    control_cap = int(prep["control_cap_mib"])
    corpus = str(prep["corpus"])

    xz_bytes = int(prep["reference_bytes"]["xz_bytes"])
    br_bytes = int(prep["reference_bytes"]["brotli_bytes"])
    control_bytes = int(prep["by_cap"][str(control_cap)]["selected_bytes"])
    surplus_vs_xz = xz_bytes - control_bytes

    rss_by_arm = rss_data["max_rss_kib"]
    xz_rss = int(rss_by_arm["xz"])
    br_rss = int(rss_by_arm["brotli"])

    null = pair_or_none(timing_dir / "paired-null.json")
    if null is None:
        null_note = "NO NULL PAIR"
    elif not null.get("timing_valid"):
        null_note = f"BLOCKED: null timing invalid ({null.get('status')})"
    else:
        lo, hi = null["paired_ratio_ci95"]
        null_note = f"A/A null CI [{float(lo):.4f}, {float(hi):.4f}]"

    rows: list[dict[str, Any]] = []
    for cap in caps:
        p = prep["by_cap"][str(cap)]
        arm = f"cap-{cap}"
        peak = int(rss_by_arm[arm])
        portfolio = int(p["selected_bytes"])

        vs_control = pair_or_none(timing_dir / f"paired-cap-{cap}-vs-control.json")
        vs_xz = pair_or_none(timing_dir / f"paired-cap-{cap}-vs-xz.json")
        vs_br = pair_or_none(timing_dir / f"paired-cap-{cap}-vs-brotli.json")

        def ci(pair: dict[str, Any] | None) -> list[float] | None:
            if pair is None:
                return None
            return [float(x) for x in pair["paired_ratio_ci95"]]

        xz_ci = ci(vs_xz)
        br_ci = ci(vs_br)
        ctrl_ci = ci(vs_control)

        xz_pair_valid = bool(vs_xz.get("timing_valid")) if vs_xz else False
        br_pair_valid = bool(vs_br.get("timing_valid")) if vs_br else False

        def gate_decode(pair_ci: list[float] | None, valid: bool) -> str:
            if pair_ci is None:
                return "NOT_MEASURED"
            if not valid:
                return "INCONCLUSIVE"
            return "PASS" if pair_ci[1] <= 2.0 else "FAIL"

        row = {
            "cap_mib": cap,
            "cap_bytes": cap * MIB,
            "portfolio_bytes": portfolio,
            "delta_bytes_vs_control": portfolio - control_bytes,
            "delta_bytes_vs_xz": portfolio - xz_bytes,
            "surplus_vs_xz_remaining": xz_bytes - portfolio,
            "surplus_consumed_vs_xz": surplus_vs_xz - (xz_bytes - portfolio),
            "bwt_routes": int(p["bwt_routes"]),
            "brotli_routes": int(p["brotli_routes"]),
            "peak_rss_kib": peak,
            "peak_rss_mib": peak / 1024.0,
            "rss_ratio_vs_xz": peak / xz_rss if xz_rss else None,
            "rss_ratio_vs_brotli": peak / br_rss if br_rss else None,
            "decode_ratio_vs_control": (float(vs_control["paired_ratio_candidate_over_control"])
                                        if vs_control else None),
            "decode_ci95_vs_control": ctrl_ci,
            "decode_ratio_vs_xz": (float(vs_xz["paired_ratio_candidate_over_control"]) if vs_xz else None),
            "decode_ci95_vs_xz": xz_ci,
            "decode_ratio_vs_brotli": (float(vs_br["paired_ratio_candidate_over_control"]) if vs_br else None),
            "decode_ci95_vs_brotli": br_ci,
            "timing_valid_xz": xz_pair_valid,
            "timing_valid_brotli": br_pair_valid,
            "gate_g2_bytes_below_xz": "PASS" if portfolio < xz_bytes else "FAIL",
            "gate_g3_decode_vs_xz": gate_decode(xz_ci, xz_pair_valid),
            "gate_g4_decode_vs_brotli": gate_decode(br_ci, br_pair_valid),
            "gate_g5_rss_vs_xz": "PASS" if peak <= 2.0 * xz_rss else "FAIL",
            "gate_g6_rss_vs_brotli": "PASS" if peak <= 2.0 * br_rss else "FAIL",
        }
        rows.append(row)

    # Routing changes relative to the control.
    rows_csv = []
    with (args.prepare.parent / "prepare.csv").open(newline="", encoding="utf-8") as f:
        rows_csv = list(csv.DictReader(f))
    control_route = {r["file"]: r["route"] for r in rows_csv if int(r["cap_mib"]) == control_cap}
    for row in rows:
        changed = []
        for r in rows_csv:
            if int(r["cap_mib"]) != row["cap_mib"]:
                continue
            if control_route.get(r["file"]) != r["route"]:
                changed.append(f"{r['file']}:{control_route.get(r['file'])}->{r['route']}")
        row["route_changes_vs_control"] = changed

    # Per-file detail: charged bytes, router choice and peak decode RSS.
    rss_rows: list[dict[str, Any]] = []
    with (args.rss.parent / "rss.csv").open(newline="", encoding="utf-8") as f:
        rss_rows = list(csv.DictReader(f))
    rss_lookup = {
        (str(r["arm"]), str(r["file"])): int(r["peak_rss_kib"]) for r in rss_rows
    }
    per_file: list[dict[str, Any]] = []
    for r in rows_csv:
        cap = int(r["cap_mib"])
        arm = f"cap-{cap}"
        kib = rss_lookup.get((arm, str(r["file"])))
        per_file.append({
            "file": str(r["file"]),
            "input_bytes": int(r["input_bytes"]),
            "cap_mib": cap,
            "route": str(r["route"]),
            "selected_bytes": int(r["selected_bytes"]),
            "bwt_subblocks": int(r["bwt_subblocks"]),
            "bwt_split": str(r.get("bwt_split", "False")) == "True",
            "peak_rss_kib": kib,
            "peak_rss_mib": (kib / 1024.0) if kib is not None else None,
            "rss_ratio_vs_xz": (kib / xz_rss) if kib is not None and xz_rss else None,
            "rss_ratio_vs_brotli": (kib / br_rss) if kib is not None and br_rss else None,
        })
    for arm in ("xz", "brotli"):
        for r in rss_rows:
            if str(r["arm"]) != arm:
                continue
            per_file.append({
                "file": str(r["file"]),
                "input_bytes": next(
                    (int(x["input_bytes"]) for x in prep["source"] if x["file"] == r["file"]), 0),
                "cap_mib": arm,
                "route": arm,
                "selected_bytes": int(
                    (xz_bytes if arm == "xz" else br_bytes) / max(1, int(prep["file_count"]))),
                "bwt_subblocks": 0,
                "bwt_split": False,
                "peak_rss_kib": int(r["peak_rss_kib"]),
                "peak_rss_mib": int(r["peak_rss_kib"]) / 1024.0,
                "rss_ratio_vs_xz": (int(r["peak_rss_kib"]) / xz_rss) if xz_rss else None,
                "rss_ratio_vs_brotli": (int(r["peak_rss_kib"]) / br_rss) if br_rss else None,
            })

    # Arbiter rulings (repo's own tool) for every timed cap.
    arbiter = Path(args.arbiter).resolve() if args.arbiter else None
    for row in rows:
        cap = row["cap_mib"]
        pair_xz = timing_dir / f"paired-cap-{cap}-vs-xz.json"
        pair_br = timing_dir / f"paired-cap-{cap}-vs-brotli.json"
        if arbiter is None or not arbiter.is_file() or not pair_xz.is_file() or not pair_br.is_file():
            row["arbiter_ruling"] = "NOT_AVAILABLE"
            continue
        adir = out / f"arbiter-cap-{cap}"
        adir.mkdir(parents=True, exist_ok=True)
        (adir / "prepare-summary.json").write_text(json.dumps({
            "schema": 1,
            "corpus": corpus,
            "source_bytes": int(prep["source_bytes"]),
            "file_count": int(prep["file_count"]),
            "compressed_bytes": {
                "anvil-aux": int(row["portfolio_bytes"]),
                "xz": xz_bytes,
                "brotli": br_bytes,
            },
        }, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        (adir / "rss.json").write_text(json.dumps({
            "schema": 1,
            "max_rss_kib": {
                "anvil-aux": int(row["peak_rss_kib"]),
                "xz": xz_rss,
                "brotli": br_rss,
            },
            "samples": [],
        }, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        cmd = [
            sys.executable, str(arbiter), "summarize",
            "--prepare", str(adir / "prepare-summary.json"),
            "--rss", str(adir / "rss.json"),
            "--xz-pair", str(pair_xz),
            "--brotli-pair", str(pair_br),
            "--null-pair", str(timing_dir / "paired-null.json"),
            "--out", str(adir / "ruling.json"),
        ]
        try:
            subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
            ruling = load_json(adir / "ruling.json")
            row["arbiter_ruling"] = ruling["ruling"]
            row["arbiter_gates"] = ruling["gates"]
        except Exception as exc:  # noqa: BLE001 - arbiter failure is reported, not hidden
            row["arbiter_ruling"] = f"ARBITER_ERROR: {exc}"

    md = [
        f"# I20 C3 — BWT subblock-cap sweep ({corpus})",
        "",
        f"- source bytes: {int(prep['source_bytes']):,}",
        f"- control cap: {control_cap} MiB, byte identity {prep['control_identity']} "
        f"({int(prep['expected_control_bytes']):,} B)",
        f"- xz -9e reference: {xz_bytes:,} B (baseline match: "
        f"{prep['reference_bytes']['xz_matches_baseline']}), peak decode RSS {xz_rss/1024:.1f} MiB",
        f"- Brotli q11/lw30 reference: {br_bytes:,} B (baseline match: "
        f"{prep['reference_bytes']['brotli_matches_baseline']}), peak decode RSS {br_rss/1024:.1f} MiB",
        f"- byte surplus vs xz at the control: {surplus_vs_xz:,} B",
        f"- timing control: {null_note}",
        "",
        "## Bytes / routing / remaining surplus",
        "",
        "| Cap | Portfolio bytes | delta vs 128 MiB | delta vs xz | Surplus left vs xz | BWT routes | Brotli routes | Routing changes vs 128 MiB |",
        "|---:|---:|---:|---:|---:|---:|---:|---|",
    ]
    for row in rows:
        changes = ", ".join(row["route_changes_vs_control"]) or "none"
        md.append(
            f"| {row['cap_mib']} MiB | {row['portfolio_bytes']:,} | "
            f"{row['delta_bytes_vs_control']:+,} | {row['delta_bytes_vs_xz']:+,} | "
            f"{row['surplus_vs_xz_remaining']:,} | {row['bwt_routes']} | {row['brotli_routes']} | {changes} |"
        )

    md.extend([
        "",
        "## Peak decode RSS and decode-time ratios (same job, paired)",
        "",
        "| Cap | Peak RSS MiB | RSS / xz | RSS / Brotli | decode CI-upper vs xz | decode CI-upper vs Brotli | G2 | G3 | G4 | G5 | G6 | Arbiter ruling |",
        "|---:|---:|---:|---:|---|---|---|---|---|---|---|---|",
    ])
    for row in rows:
        xz_ci = row["decode_ci95_vs_xz"]
        br_ci = row["decode_ci95_vs_brotli"]
        xz_s = f"[{xz_ci[0]:.4f}, {xz_ci[1]:.4f}]" if xz_ci else "not measured"
        br_s = f"[{br_ci[0]:.4f}, {br_ci[1]:.4f}]" if br_ci else "not measured"
        md.append(
            f"| {row['cap_mib']} MiB | {row['peak_rss_mib']:.1f} | "
            f"{row['rss_ratio_vs_xz']:.3f} | {row['rss_ratio_vs_brotli']:.3f} | "
            f"{xz_s} | {br_s} | {row['gate_g2_bytes_below_xz']} | {row['gate_g3_decode_vs_xz']} | "
            f"{row['gate_g4_decode_vs_brotli']} | {row['gate_g5_rss_vs_xz']} | "
            f"{row['gate_g6_rss_vs_brotli']} | {row['arbiter_ruling']} |"
        )

    md.extend([
        "",
        "## Per-file detail (charged bytes, router choice, peak decode RSS)",
        "",
        "| File | Input B | Arm | Route | Selected B | BWT subblocks | Peak RSS MiB | RSS / xz | RSS / Brotli |",
        "|---|---:|---|---|---:|---:|---:|---:|---:|",
    ])
    for pf in per_file:
        arm = pf["cap_mib"] if isinstance(pf["cap_mib"], str) else f"{pf['cap_mib']} MiB"
        md.append(
            f"| {pf['file']} | {pf['input_bytes']:,} | {arm} | {pf['route']} | "
            f"{pf['selected_bytes']:,} | {pf['bwt_subblocks']} | "
            f"{(pf['peak_rss_mib'] if pf['peak_rss_mib'] is not None else 0):.1f} | "
            f"{(pf['rss_ratio_vs_xz'] if pf['rss_ratio_vs_xz'] is not None else 0):.3f} | "
            f"{(pf['rss_ratio_vs_brotli'] if pf['rss_ratio_vs_brotli'] is not None else 0):.3f} |"
        )

    md.extend([
        "",
        "G2 = portfolio bytes strictly below xz -9e; G3/G4 = paired decode-time ratio CI-upper <= 2.0 "
        "vs xz / Brotli; G5/G6 = peak decode RSS ratio <= 2.0 vs xz / Brotli.",
        "A decode gate is INCONCLUSIVE when its paired series is not timing-valid "
        "(ambient block or excess CV); a blocked series is never reinterpreted as a failure.",
        "",
    ])
    report = "\n".join(md)
    (out / "summary.md").write_text(report, encoding="utf-8")
    result = {
        "schema": 1,
        "corpus": corpus,
        "control_cap_mib": control_cap,
        "reference_bytes": {"xz": xz_bytes, "brotli": br_bytes},
        "reference_peak_rss_kib": {"xz": xz_rss, "brotli": br_rss},
        "surplus_vs_xz_at_control": surplus_vs_xz,
        "timing_null": null,
        "rows": rows,
        "per_file": per_file,
    }
    (out / "summary.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(report)


# ---------------------------------------------------------------------------


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("prepare")
    p.add_argument("--corpus", choices=("silesia", "enwik8"), required=True)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--brotli", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--caps-mib", default="1,2,4,8,16,32,64,128")
    p.set_defaults(func=prepare)

    p = sub.add_parser("rss")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=rss)

    p = sub.add_parser("verify")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=verify)

    p = sub.add_parser("timing")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--driver", type=Path, required=True)
    p.add_argument("--timed-caps", default="")
    p.add_argument("--reps", type=int, default=9)
    p.add_argument("--warmups", type=int, default=2)
    p.add_argument("--seed", type=int, default=41246)
    p.add_argument("--bootstrap", type=int, default=20000)
    p.add_argument("--epsilon", type=float, default=0.02)
    p.add_argument("--max-cv", type=float, default=0.15)
    p.add_argument("--cpu", type=int, default=0)
    p.add_argument("--ambient-reps", type=int, default=5)
    p.add_argument("--ambient-iterations", type=int, default=250000)
    p.add_argument("--ambient-max-cv", type=float, default=0.10)
    p.add_argument("--timeout-s", type=float, default=3600.0)
    p.set_defaults(func=timing)

    p = sub.add_parser("summarize")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--rss", type=Path, required=True)
    p.add_argument("--timing-dir", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--arbiter", type=Path, default=Path("tools/reference_cost_gate.py"))
    p.set_defaults(func=summarize)

    return ap


def main() -> None:
    args = build_parser().parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
