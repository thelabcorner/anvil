#!/usr/bin/env python3
"""I11 candidate D1 - BWT-path decode speed vs the BWT sub-block cap, routing held constant.

Preregistered in `docs/I11-PARETO-DOMINANCE-PREREG-2026-10-22.md` section 5 (candidate D1).
The decision rule is fixed in advance and is quoted verbatim in the module docstring of
`docs/I11-RESULTS-D1-cap-speed.md`; this tool implements it, it does not re-derive it.

Hypothesis under test
---------------------
The inverse BWT performs `n/2` dependent random accesses into a `4C`-byte LF (PSI) array when
the sub-block cap is `C`, so a smaller cap should make those accesses cache-resident and
therefore faster.  The I20 sweeps could not test this because changing the cap also changes the
*router's* backend choice, confounding portfolio speed-up with routing.

What this tool measures
-----------------------
For a single file that is always BWT-routed, the decode time at caps
{1,2,4,8,16,32,64,128} MiB with the backend and every other encoder setting held constant, so
that ONLY the LF-array working set changes.  The route (BWT vs Brotli backend) is recorded for
every (arm, file, cap) and the sweep is VOID if the route changes at any cap.

Arms
----
  bwt-forced     `--ratio-backend=bwt`  - the backend is held constant BY CONSTRUCTION, so the
                 route cannot change at any cap.  This is the primary "routing held constant" arm.
  auto-i20       `--ratio-backend=auto --ratio-bwt-min-gain-pct 3` - the I20 operating profile.
  auto-default   `--ratio-backend=auto` - the plain default profile (bytes-only router).

Every arm uses the canonical ratio route settings `--parse=ratio --ratio-context=off
--ratio-lines=off --bwt-aux=on` (the transform-0 route the I20 arbiter measures), so the only
thing that varies between the arms is the routing policy, and the only thing that varies between
caps inside an arm is the sub-block cap.

Measurement protocol
--------------------
Same-job paired/interleaved timing through `tools/paired_bench.py` with the frozen prereg
parameters, CPU affinity 0, every raw repetition retained, and a mandatory A/A null whose CI
must span 1.0.  The control arm of every cap comparison is the **uncapped default** payload
(cap 128 MiB, which for these files is a single unsplit BWT block), so the reported ratio is
`decode_time(cap) / decode_time(uncapped)` measured in the same job, interleaved, paired.

Usage
-----
    python tools/i11_cap_speed.py prepare  --corpus silesia --root <corpus-root> \
        --anvil <anvil> --out <dir> --caps-mib 1,2,4,8,16,32,64,128 --files mr,x-ray
    python tools/i11_cap_speed.py rss      --prepare <dir>/prepare.json --out <dir>
    python tools/i11_cap_speed.py timing   --prepare <dir>/prepare.json --out <dir> \
        --driver tools/paired_bench.py --reps 9 --warmups 2 --seed 41246 --bootstrap 20000 \
        --epsilon 0.02 --max-cv 0.15 --cpu 0 --ambient-reps 5 --ambient-iterations 250000 \
        --ambient-max-cv 0.10
    python tools/i11_cap_speed.py verify   --prepare <dir>/prepare.json --anvil <anvil> --out <dir>
    python tools/i11_cap_speed.py summarize --prepare <dir>/prepare.json --out <dir>
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import os
import re
import shutil
import statistics
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

MIB = 1024 * 1024

# The preregistered cap sweep (D1, prereg section 5).
DEFAULT_CAPS_MIB = [1, 2, 4, 8, 16, 32, 64, 128]

# The uncapped default: 128 MiB is the shipped default cap and is >= the size of
# every file in this sweep, so it is the "no split" reference point.
CONTROL_CAP_MIB = 128

# Arm name -> fixed encoder arguments.  `--bwt-subblock=<bytes>` is appended per cap.
ARMS: dict[str, list[str]] = {
    "bwt-forced": [
        "--parse=ratio",
        "--ratio-backend=bwt",
        "--ratio-context=off",
        "--ratio-lines=off",
        "--bwt-aux=on",
    ],
    "auto-i20": [
        "--parse=ratio",
        "--ratio-backend=auto",
        "--ratio-context=off",
        "--ratio-lines=off",
        "--bwt-aux=on",
        # SPACED form on purpose: the `=` form of this flag is broken in
        # src/anvil.cpp (the parser reads a.substr(27) of a 25-character prefix), so
        # `--ratio-bwt-min-gain-pct=3` throws "invalid string position" and a
        # three-digit value would be silently mis-parsed.  The spaced form is the one
        # the I20 closure runs and the preregistration name.  See the D1 results doc.
        "--ratio-bwt-min-gain-pct",
        "3",
    ],
    "auto-default": [
        "--parse=ratio",
        "--ratio-backend=auto",
        "--ratio-context=off",
        "--ratio-lines=off",
        "--bwt-aux=on",
    ],
}

# The preregistered decision rule, quoted verbatim from the prereg (section 5, D1).
DECISION_RULE = (
    "Report the curve, the fitted cache-locality gain, and the answer to one question: "
    "is the BWT-path decode time a function of the cap once routing is held constant? "
    "If the curve is flat within the run's own noise band (the A/A null's CV), the "
    "cache-locality hypothesis is DEAD, D3 must change the inverse algorithm rather than the "
    "cap, and the programme must not spend further arbiter minutes on cap tuning. "
    "If the curve is monotone and material (>= 1.5x across the sweep), the cap becomes a "
    "first-class locality lever and D3 must exploit it."
)

KILL_CONDITION = "Any cap sweep that changes the route is void."


# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------


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
    """Inspect a single rev-2 mode-17 whole-file ratio payload.

    Same wire walk as the I20 tool (`tools/i20_subblock_sweep.py::ratio_info`), re-implemented
    here so that D1 owns a self-contained tool.  backend id 1 = Brotli, 2 = BWT.
    """
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
        "backend_name": {1: "brotli", 2: "bwt"}.get(backend, f"unknown-{backend}"),
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


def run(argv: list[str], *, stdout_path: Path | None = None, cwd: Path | None = None) -> None:
    handle = None
    out: Any = subprocess.DEVNULL
    try:
        if stdout_path is not None:
            stdout_path.parent.mkdir(parents=True, exist_ok=True)
            handle = stdout_path.open("wb")
            out = handle
        p = subprocess.run(argv, stdout=out, stderr=subprocess.PIPE, cwd=cwd, check=False)
        if p.returncode:
            raise RuntimeError(
                f"command failed rc={p.returncode}: {argv!r}\n"
                + p.stderr.decode(errors="replace")[-4000:]
            )
    finally:
        if handle is not None:
            handle.close()


def run_time_rss(argv: list[str], *, timeout_s: float = 1800.0) -> tuple[float, int]:
    """Run once under GNU time; return (elapsed seconds, max RSS KiB)."""
    time_bin = Path("/usr/bin/time")
    if not time_bin.is_file():
        raise RuntimeError("/usr/bin/time is required")
    with tempfile.TemporaryDirectory(prefix="anvil-i11-cap-time-") as td0:
        stamp = Path(td0) / "time.txt"
        p = subprocess.run(
            [str(time_bin), "-f", "%e %M", "-o", str(stamp), "--", *argv],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            timeout=timeout_s,
            check=False,
        )
        if p.returncode:
            raise RuntimeError(
                f"command failed rc={p.returncode}: {argv!r}\n"
                + p.stderr.decode(errors="replace")[-4000:]
            )
        parts = stamp.read_text(encoding="utf-8").strip().split()
    if len(parts) != 2:
        raise RuntimeError(f"bad GNU time output: {parts!r}")
    return float(parts[0]), int(parts[1])


def parse_caps(text: str) -> list[int]:
    values = sorted({int(x.strip()) for x in text.split(",") if x.strip()})
    if not values or any(v <= 0 for v in values):
        raise argparse.ArgumentTypeError("caps must be positive comma-separated MiB values")
    if CONTROL_CAP_MIB not in values:
        raise argparse.ArgumentTypeError(
            f"caps must include the {CONTROL_CAP_MIB} MiB uncapped control"
        )
    return values


def parse_list(text: str) -> list[str]:
    return [x.strip() for x in text.split(",") if x.strip()]


def arm_args(arm: str, cap_bytes: int) -> list[str]:
    if arm not in ARMS:
        raise RuntimeError(f"unknown arm {arm!r}; known arms: {sorted(ARMS)}")
    return list(ARMS[arm]) + [f"--bwt-subblock={cap_bytes}"]


def encode_cmd(anvil: Path, src: Path, dst: Path, arm: str, cap_bytes: int) -> list[str]:
    return [str(anvil), "c", str(src), str(dst), *arm_args(arm, cap_bytes), "--quiet"]


def decode_cmd(anvil: Path, payload: Path, out: Path) -> list[str]:
    return [str(anvil), "d", str(payload), str(out), "--quiet"]


def payload_path(out: Path, arm: str, name: str, cap_mib: int) -> Path:
    return out / "work" / "payload" / arm / name / f"cap-{cap_mib}.anv"


def decode_out_path(out: Path, arm: str, name: str, cap_mib: int) -> Path:
    return out / "work" / "decode" / arm / name / f"cap-{cap_mib}.out"


# ---------------------------------------------------------------------------
# linear fits (no third-party dependency: the runner has plain python3)
# ---------------------------------------------------------------------------


def linear_fit(xs: list[float], ys: list[float]) -> dict[str, float]:
    n = len(xs)
    if n < 2:
        return {"slope": 0.0, "intercept": 0.0, "r2": 0.0, "sse": 0.0}
    mx = sum(xs) / n
    my = sum(ys) / n
    sxx = sum((x - mx) ** 2 for x in xs)
    sxy = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    slope = (sxy / sxx) if sxx > 0 else 0.0
    intercept = my - slope * mx
    sse = sum((y - (intercept + slope * x)) ** 2 for x, y in zip(xs, ys))
    sst = sum((y - my) ** 2 for y in ys)
    r2 = (1.0 - sse / sst) if sst > 0 else 0.0
    return {"slope": slope, "intercept": intercept, "r2": r2, "sse": sse}


# ---------------------------------------------------------------------------
# prepare
# ---------------------------------------------------------------------------


def corpus_files(corpus: str, root: Path, names: list[str]) -> list[Path]:
    if corpus == "silesia":
        base = root / "silesia"
    elif corpus == "enwik8":
        base = root / "enwik8"
    else:
        raise RuntimeError(f"unsupported corpus: {corpus}")
    files = []
    for name in names:
        p = base / name
        if not p.is_file():
            raise RuntimeError(f"missing corpus file: {p}")
        files.append(p)
    return files


def prepare(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    anvil = args.anvil.resolve()
    root = args.root.resolve()
    caps = parse_caps(args.caps_mib)
    files = corpus_files(args.corpus, root, args.files)
    out.mkdir(parents=True, exist_ok=True)

    combos: list[dict[str, Any]] = []
    for src in files:
        combos.append(
            {
                "file": src.name,
                "path": str(src),
                "input_bytes": src.stat().st_size,
                "source_sha256": sha256(src),
            }
        )

    payloads: dict[str, Any] = {}
    for arm in args.arms:
        for combo in combos:
            name = combo["file"]
            n = combo["input_bytes"]
            for cap in caps:
                cap_bytes = cap * MIB
                pp = payload_path(out, arm, name, cap)
                pp.parent.mkdir(parents=True, exist_ok=True)
                run(encode_cmd(anvil, Path(combo["path"]), pp, arm, cap_bytes))
                info = ratio_info(pp)
                expected_sub = ((n + cap_bytes - 1) // cap_bytes) if cap_bytes < n else 1
                subblocks = info["subblocks"]
                split = cap_bytes < n
                row = {
                    "arm": arm,
                    "file": name,
                    "cap_mib": cap,
                    "cap_bytes": cap_bytes,
                    "payload_path": str(pp),
                    "payload_bytes": pp.stat().st_size,
                    "payload_sha256": sha256(pp),
                    "route": info["backend_name"],
                    "backend_id": info["backend"],
                    "transform": info["transform"],
                    "subblocks": subblocks,
                    "expected_subblocks": expected_sub,
                    "split": split,
                    "transformed_len": info["transformed_len"],
                    # The LF (PSI) array the inverse BWT walks is 4 bytes per symbol of the
                    # sub-block, so the random-access working set is 4 * min(cap, n) bytes.
                    "lf_working_set_bytes": 4 * min(cap_bytes, n),
                    "subblock_bytes": min(cap_bytes, n),
                }
                if info["backend"] == 2:
                    if subblocks != expected_sub:
                        raise RuntimeError(
                            f"{name}/{arm}/cap-{cap}: subblocks={subblocks} expected={expected_sub}"
                        )
                payloads.setdefault(arm, {}).setdefault(name, {})[str(cap)] = row

    # Round-trip gate: every payload must decode byte-exactly to the source.
    roundtrip: list[dict[str, Any]] = []
    for arm in payloads:
        for name in payloads[arm]:
            for cap_s, row in payloads[arm][name].items():
                cap = int(cap_s)
                dp = decode_out_path(out, arm, name, cap)
                dp.parent.mkdir(parents=True, exist_ok=True)
                run(decode_cmd(anvil, Path(row["payload_path"]), dp))
                got = sha256(dp)
                src = combos_by_name(combos)[name]
                ok = dp.stat().st_size == src["input_bytes"] and got == src["source_sha256"]
                roundtrip.append(
                    {
                        "arm": arm,
                        "file": name,
                        "cap_mib": cap,
                        "route": row["route"],
                        "decoded_bytes": dp.stat().st_size,
                        "decoded_sha256": got,
                        "ok": ok,
                    }
                )
                if not ok:
                    raise RuntimeError(
                        f"roundtrip mismatch: {name}/{arm}/cap-{cap} route={row['route']}"
                    )
                dp.unlink()

    # Route constancy per (arm, file): the kill condition.
    route_report: list[dict[str, Any]] = []
    for arm in payloads:
        for name in payloads[arm]:
            routes = {c: payloads[arm][name][c]["route"] for c in payloads[arm][name]}
            distinct = sorted(set(routes.values()))
            route_report.append(
                {
                    "arm": arm,
                    "file": name,
                    "routes_per_cap": routes,
                    "distinct_routes": distinct,
                    "route_constant": len(distinct) == 1,
                    "route": distinct[0] if len(distinct) == 1 else None,
                    "sweep_valid": len(distinct) == 1,
                }
            )

    # Payload-identity cross-check: caps >= the file size must produce the identical
    # (unsplit) payload, and every payload must be byte-deterministic.
    identity_notes: list[dict[str, Any]] = []
    for arm in payloads:
        for name in payloads[arm]:
            n = combos_by_name(combos)[name]["input_bytes"]
            by_effective: dict[int, list[int]] = {}
            for cap_s, row in payloads[arm][name].items():
                by_effective.setdefault(int(row["subblock_bytes"]), []).append(int(cap_s))
            groups = {str(k): sorted(v) for k, v in sorted(by_effective.items())}
            identity_notes.append(
                {
                    "arm": arm,
                    "file": name,
                    "input_bytes": n,
                    "caps_grouped_by_effective_subblock_bytes": groups,
                    "note": "caps whose effective sub-block size is identical produce the "
                    "identical payload (the cap saturates at the file size)",
                }
            )

    payload_bytes: dict[str, Any] = {}
    for arm in payloads:
        payload_bytes[arm] = {
            f"{name}@{cap}": row["payload_bytes"]
            for name in payloads[arm]
            for cap, row in payloads[arm][name].items()
        }

    prepare_doc = {
        "schema": 1,
        "corpus": args.corpus,
        "caps_mib": caps,
        "control_cap_mib": CONTROL_CAP_MIB,
        "arms": args.arms,
        "files": combos,
        "payloads": payloads,
        "route_report": route_report,
        "roundtrip": roundtrip,
        "payload_bytes": payload_bytes,
        "identity_notes": identity_notes,
        "arms_definitions": ARMS,
        "decision_rule": DECISION_RULE,
        "kill_condition": KILL_CONDITION,
        "anvil_path": str(anvil),
    }
    (out / "prepare.json").write_text(
        json.dumps(prepare_doc, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    with (out / "prepare.csv").open("w", newline="", encoding="utf-8") as f:
        fields = [
            "arm", "file", "cap_mib", "route", "backend_id", "subblocks", "split",
            "payload_bytes", "payload_sha256", "lf_working_set_bytes", "input_bytes",
        ]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for arm in payloads:
            for name in payloads[arm]:
                for cap_s, row in payloads[arm][name].items():
                    w.writerow(
                        {
                            "arm": arm,
                            "file": name,
                            "cap_mib": row["cap_mib"],
                            "route": row["route"],
                            "backend_id": row["backend_id"],
                            "subblocks": row["subblocks"],
                            "split": row["split"],
                            "payload_bytes": row["payload_bytes"],
                            "payload_sha256": row["payload_sha256"],
                            "lf_working_set_bytes": row["lf_working_set_bytes"],
                            "input_bytes": combos_by_name(combos)[name]["input_bytes"],
                        }
                    )

    summary = {
        "schema": 1,
        "corpus": args.corpus,
        "source_bytes": sum(c["input_bytes"] for c in combos),
        "file_count": len(combos),
        "compressed_bytes": payload_bytes,
        "roundtrip": all(r["ok"] for r in roundtrip),
        "route_report": route_report,
        "caps_mib": caps,
        "arms": args.arms,
    }
    (out / "prepare-summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(
        f"prepared {len(payloads)} arms x {len(combos)} files x {len(caps)} caps; "
        f"roundtrip_ok={summary['roundtrip']}",
        flush=True,
    )


def combos_by_name(combos: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    return {c["file"]: c for c in combos}


# ---------------------------------------------------------------------------
# rss
# ---------------------------------------------------------------------------


def rss(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    anvil = args.anvil.resolve()
    prep = json.loads((out / "prepare.json").read_text(encoding="utf-8"))
    caps = prep["caps_mib"]
    rows: list[dict[str, Any]] = []
    for arm in prep["arms"]:
        for combo in prep["files"]:
            name = combo["file"]
            for cap in caps:
                p = prep["payloads"][arm][name][str(cap)]
                dp = decode_out_path(out, arm, name, cap)
                dp.parent.mkdir(parents=True, exist_ok=True)
                kibs: list[int] = []
                secs: list[float] = []
                for _ in range(args.reps):
                    el, kib = run_time_rss(decode_cmd(anvil, Path(p["payload_path"]), dp))
                    kibs.append(kib)
                    secs.append(el)
                rows.append(
                    {
                        "arm": arm,
                        "file": name,
                        "cap_mib": cap,
                        "route": p["route"],
                        "subblocks": p["subblocks"],
                        "lf_working_set_bytes": p["lf_working_set_bytes"],
                        "payload_bytes": p["payload_bytes"],
                        "input_bytes": combo["input_bytes"],
                        "rss_kib_samples": kibs,
                        "peak_rss_kib": max(kibs),
                        "peak_rss_kib_median": int(statistics.median(kibs)),
                        "wall_s_samples": secs,
                        "wall_s_median": statistics.median(secs),
                    }
                )
                print(
                    f"rss {arm}/{name}/cap-{cap}: route={p['route']} "
                    f"peak={max(kibs)} KiB",
                    flush=True,
                )
    (out / "rss.json").write_text(
        json.dumps(
            {
                "schema": 1,
                "rows": rows,
                "max_rss_kib": max(r["peak_rss_kib"] for r in rows) if rows else None,
                "note": "peak_rss_kib is /usr/bin/time -f %M of the per-process decode; "
                "the per-cap series is in rows[]",
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
    with (out / "rss.csv").open("w", newline="", encoding="utf-8") as f:
        fields = [
            "arm", "file", "cap_mib", "route", "subblocks", "lf_working_set_bytes",
            "payload_bytes", "input_bytes", "peak_rss_kib", "peak_rss_kib_median",
            "wall_s_median",
        ]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for r in rows:
            w.writerow({k: r[k] for k in fields})


# ---------------------------------------------------------------------------
# timing
# ---------------------------------------------------------------------------


def timing(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    anvil = args.anvil.resolve()
    driver = args.driver.resolve()
    prep = json.loads((out / "prepare.json").read_text(encoding="utf-8"))
    caps = prep["caps_mib"]
    tdir = out / "timing"
    tdir.mkdir(parents=True, exist_ok=True)

    for arm in prep["arms"]:
        for combo in prep["files"]:
            name = combo["file"]
            control = prep["payloads"][arm][name][str(CONTROL_CAP_MIB)]
            control_out = decode_out_path(out, arm, name, CONTROL_CAP_MIB)
            control_out.parent.mkdir(parents=True, exist_ok=True)

            # ---- mandatory A/A null: the control arm against itself ----
            null_csv = tdir / f"null-{arm}-{name}.csv"
            null_json = tdir / f"null-{arm}-{name}.json"
            run_driver(
                driver,
                decode_cmd(anvil, Path(control["payload_path"]), control_out),
                decode_cmd(anvil, Path(control["payload_path"]), control_out),
                control["payload_path"],
                control["payload_path"],
                null_csv,
                null_json,
                args,
            )

            # ---- one paired comparison per cap against the uncapped control ----
            for cap in caps:
                cand = prep["payloads"][arm][name][str(cap)]
                cand_out = decode_out_path(out, arm, name, cap)
                cand_out.parent.mkdir(parents=True, exist_ok=True)
                csv_path = tdir / f"cap-{arm}-{name}-{cap}.csv"
                json_path = tdir / f"cap-{arm}-{name}-{cap}.json"
                run_driver(
                    driver,
                    decode_cmd(anvil, Path(control["payload_path"]), control_out),
                    decode_cmd(anvil, Path(cand["payload_path"]), cand_out),
                    control["payload_path"],
                    cand["payload_path"],
                    csv_path,
                    json_path,
                    args,
                )
                print(
                    f"timed {arm}/{name}/cap-{cap}: route={cand['route']} "
                    f"subblocks={cand['subblocks']}",
                    flush=True,
                )


def run_driver(
    driver: Path,
    control_argv: list[str],
    candidate_argv: list[str],
    control_observe: str,
    candidate_observe: str,
    csv_path: Path,
    json_path: Path,
    args: argparse.Namespace,
) -> None:
    argv = [
        sys.executable,
        str(driver),
        "--control-json",
        json.dumps(control_argv),
        "--candidate-json",
        json.dumps(candidate_argv),
        "--reps",
        str(args.reps),
        "--warmups",
        str(args.warmups),
        "--seed",
        str(args.seed),
        "--bootstrap",
        str(args.bootstrap),
        "--epsilon",
        str(args.epsilon),
        "--max-cv",
        str(args.max_cv),
        "--cpu",
        str(args.cpu),
        "--ambient-reps",
        str(args.ambient_reps),
        "--ambient-iterations",
        str(args.ambient_iterations),
        "--ambient-max-cv",
        str(args.ambient_max_cv),
        "--control-observe",
        control_observe,
        "--candidate-observe",
        candidate_observe,
        "--out",
        str(csv_path),
        "--summary",
        str(json_path),
    ]
    p = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    sys.stdout.write(p.stdout.decode(errors="replace"))
    if p.returncode:
        raise RuntimeError(
            f"paired_bench failed rc={p.returncode}\n{p.stderr.decode(errors='replace')[-4000:]}"
        )


# ---------------------------------------------------------------------------
# verify
# ---------------------------------------------------------------------------


def verify(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    anvil = args.anvil.resolve()
    prep = json.loads((out / "prepare.json").read_text(encoding="utf-8"))
    rows: list[dict[str, Any]] = []
    for arm in prep["arms"]:
        for combo in prep["files"]:
            name = combo["file"]
            for cap_s, p in sorted(prep["payloads"][arm][name].items(), key=lambda kv: int(kv[0])):
                cap = int(cap_s)
                dp = decode_out_path(out, arm, name, cap)
                dp.parent.mkdir(parents=True, exist_ok=True)
                run(decode_cmd(anvil, Path(p["payload_path"]), dp))
                got = sha256(dp)
                payload_now = sha256(Path(p["payload_path"]))
                ok = (
                    dp.stat().st_size == combo["input_bytes"]
                    and got == combo["source_sha256"]
                    and payload_now == p["payload_sha256"]
                )
                rows.append(
                    {
                        "arm": arm,
                        "file": name,
                        "cap_mib": cap,
                        "route": p["route"],
                        "decoded_bytes": dp.stat().st_size,
                        "decoded_sha256": got,
                        "payload_sha256_after": payload_now,
                        "payload_bytes": p["payload_bytes"],
                        "ok": ok,
                    }
                )
                if not ok:
                    raise RuntimeError(f"verify failed: {name}/{arm}/cap-{cap}")
                dp.unlink()
    doc = {
        "schema": 1,
        "rows": rows,
        "all_ok": all(r["ok"] for r in rows),
        "note": "re-decoded after the timed pass: byte-exact round trip and unchanged payload",
    }
    (out / "verify.json").write_text(
        json.dumps(doc, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(f"verify: {len(rows)} payloads, all_ok={doc['all_ok']}", flush=True)


# ---------------------------------------------------------------------------
# summarize
# ---------------------------------------------------------------------------


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return None


def rss_span(combo: dict[str, Any], cap_a: int, cap_b: int) -> float | None:
    """Peak-RSS ratio between two caps of one combo (None when a cap is missing)."""
    vals: dict[int, float] = {}
    for r in combo.get("curve") or []:
        if r.get("peak_rss_kib") is not None:
            vals[int(r["cap_mib"])] = float(r["peak_rss_kib"])
    if cap_a not in vals or cap_b not in vals or vals[cap_a] <= 0:
        return None
    return vals[cap_b] / vals[cap_a]


def curve_stats(
    curve: list[dict[str, Any]],
    null_cv: float,
    only_caps: list[int] | None = None,
) -> tuple[float | None, bool | None, bool | None]:
    """Span (max/min), monotonicity and flatness of the relative decode-time curve.

    The noise band is the A/A null's robust CV: consecutive steps whose relative change
    is inside the band do not count as reversals, and a curve whose whole span is inside
    the band is "flat within the run's own noise band".
    """
    times = [
        (c["cap_mib"], c["relative_decode_s"])
        for c in curve
        if c["relative_decode_s"] is not None
        and (only_caps is None or c["cap_mib"] in only_caps)
    ]
    times.sort()
    if not times:
        return None, None, None
    vals = [v for _, v in times]
    span = max(vals) / min(vals) if min(vals) > 0 else None
    signs: list[int] = []
    for i in range(len(vals) - 1):
        d = vals[i + 1] - vals[i]
        if abs(d) <= null_cv * vals[i]:
            continue
        signs.append(1 if d > 0 else -1)
    monotone = bool(signs) and (all(s > 0 for s in signs) or all(s < 0 for s in signs))
    flat_in_noise = bool(span is not None and (span - 1.0) <= null_cv)
    return span, monotone, flat_in_noise


def summarise_combo(
    arm: str,
    name: str,
    prep: dict[str, Any],
    tdir: Path,
    rss_rows: list[dict[str, Any]],
) -> dict[str, Any]:
    caps = prep["caps_mib"]
    combo_bytes = {c["file"]: c["input_bytes"] for c in prep["files"]}
    control = prep["payloads"][arm][name][str(CONTROL_CAP_MIB)]
    null = load_json(tdir / f"null-{arm}-{name}.json") or {}
    null_cv = max(
        float((null.get("control") or {}).get("robust_cv") or 0.0),
        float((null.get("candidate") or {}).get("robust_cv") or 0.0),
    )
    null_ci = null.get("paired_ratio_ci95")
    null_spans = bool(null_ci and null_ci[0] <= 1.0 <= null_ci[1])

    curve: list[dict[str, Any]] = []
    for cap in caps:
        p = prep["payloads"][arm][name][str(cap)]
        s = load_json(tdir / f"cap-{arm}-{name}-{cap}.json") or {}
        r = next(
            (
                x
                for x in rss_rows
                if x["arm"] == arm and x["file"] == name and x["cap_mib"] == cap
            ),
            None,
        )
        ratio = s.get("paired_ratio_candidate_over_control")
        ctl_med = float((s.get("control") or {}).get("median_s") or 0.0)
        cand_med = float((s.get("candidate") or {}).get("median_s") or 0.0)
        t_rel = (ratio * ctl_med) if (ratio is not None and ctl_med > 0) else None
        curve.append(
            {
                "arm": arm,
                "file": name,
                "cap_mib": cap,
                "cap_bytes": p["cap_bytes"],
                "route": p["route"],
                "backend_id": p["backend_id"],
                "subblocks": p["subblocks"],
                "split": p["split"],
                "payload_bytes": p["payload_bytes"],
                "lf_working_set_bytes": p["lf_working_set_bytes"],
                "decode_s_median": cand_med,
                "control_decode_s_median": ctl_med,
                "decode_mb_s": (combo_bytes[name] / 1e6) / cand_med if cand_med > 0 else None,
                "paired_ratio_vs_uncapped": ratio,
                "paired_ratio_ci95": s.get("paired_ratio_ci95"),
                "paired_ci_excludes_one": (
                    bool(s.get("paired_ratio_ci95"))
                    and (s["paired_ratio_ci95"][0] > 1.0 or s["paired_ratio_ci95"][1] < 1.0)
                ),
                "paired_timing_valid": s.get("timing_valid"),
                "paired_status": s.get("status"),
                "control_robust_cv": (s.get("control") or {}).get("robust_cv"),
                "candidate_robust_cv": (s.get("candidate") or {}).get("robust_cv"),
                "relative_decode_s": t_rel,
                "peak_rss_kib": (r or {}).get("peak_rss_kib"),
                "peak_rss_kib_median": (r or {}).get("peak_rss_kib_median"),
                "rss_wall_s_median": (r or {}).get("wall_s_median"),
            }
        )

    route_set = sorted({c["route"] for c in curve})
    route_constant = len(route_set) == 1
    span, monotone, flat_in_noise = curve_stats(curve, null_cv)

    # Supporting (NOT the preregistered sweep) analysis over the caps that share the
    # dominant route, for the case where the router changes route at some cap and the
    # preregistered sweep is therefore void.  Reported as supporting evidence only.
    dominant_route = None
    if route_set:
        counts = {r: sum(1 for c in curve if c["route"] == r) for r in route_set}
        dominant_route = max(counts, key=lambda r: counts[r])
    subset_caps = [c["cap_mib"] for c in curve if c["route"] == dominant_route]
    sub_span, sub_monotone, sub_flat = curve_stats(curve, null_cv, subset_caps)

    # Fitted cache-locality model: t(cap) = a + b * W(cap), W = 4 * min(cap, n) bytes.
    xs = []
    ys = []
    xs_cap = []
    for c in curve:
        if c["relative_decode_s"] is None:
            continue
        xs.append(float(c["lf_working_set_bytes"]))
        ys.append(float(c["relative_decode_s"]))
        xs_cap.append(float(min(c["cap_bytes"], c["lf_working_set_bytes"] // 4)))
    fit_w = linear_fit(xs, ys)
    fit_cap = linear_fit(xs_cap, ys)
    flat_sse = sum((y - statistics.mean(ys)) ** 2 for y in ys) if ys else 0.0

    # RSS model: RSS(cap) = a + b * min(cap, n); the 4C hypothesis predicts b in [4, 5].
    rxs: list[float] = []
    rys: list[float] = []
    for c in curve:
        if c["peak_rss_kib"] is None:
            continue
        rxs.append(float(min(c["cap_bytes"], c["lf_working_set_bytes"] // 4)))
        rys.append(float(c["peak_rss_kib"]) * 1024.0)
    fit_rss = linear_fit(rxs, rys)

    if not route_constant:
        verdict = "VOID_ROUTE_CHANGED"
    elif flat_in_noise:
        verdict = "NO_GO_CACHE_LOCALITY_HYPOTHESIS_DEAD"
    elif monotone and span is not None and span >= 1.5:
        verdict = "WIN_CAP_IS_A_FIRST_CLASS_LOCALITY_LEVER"
    elif monotone:
        verdict = "INCONCLUSIVE_MONOTONE_BUT_NOT_MATERIAL"
    else:
        verdict = "INCONCLUSIVE_NOT_MONOTONE"

    return {
        "arm": arm,
        "file": name,
        "route_per_cap": {str(c["cap_mib"]): c["route"] for c in curve},
        "routes": route_set,
        "route_constant_across_caps": route_constant,
        "sweep_valid": route_constant,
        "null": {
            "paired_ratio_ci95": null_ci,
            "spans_one": null_spans,
            "robust_cv": null_cv,
            "timing_valid": null.get("timing_valid"),
            "status": null.get("status"),
            "control_median_s": (null.get("control") or {}).get("median_s"),
            "candidate_median_s": (null.get("candidate") or {}).get("median_s"),
        },
        "curve": curve,
        "span_max_over_min": span,
        "monotone": monotone,
        "flat_within_null_cv": flat_in_noise,
        "caps_with_ci_excluding_one": [
            c["cap_mib"] for c in curve if c.get("paired_ci_excludes_one")
        ],
        "material_lever": bool(monotone and span is not None and span >= 1.5),
        "supporting_dominant_route_subset": {
            "note": "supporting evidence only, NOT the preregistered sweep: the caps that "
            "share the dominant route, used only when the router changed route and the "
            "preregistered sweep is void under the kill condition",
            "dominant_route": dominant_route,
            "caps": subset_caps,
            "span_max_over_min": sub_span,
            "monotone": sub_monotone,
            "flat_within_null_cv": sub_flat,
        },
        "fit_locality_time_vs_lf_working_set": {
            **fit_w,
            "model": "t = a + b * (4 * min(cap, n))",
            "slope_s_per_byte": fit_w["slope"],
            "predicted_span_uncapped_over_min_cap": (
                (fit_w["intercept"] + fit_w["slope"] * max(xs)) / (fit_w["intercept"] + fit_w["slope"] * min(xs))
                if xs and (fit_w["intercept"] + fit_w["slope"] * min(xs)) > 0
                else None
            ),
        },
        "fit_time_vs_cap_bytes": {**fit_cap, "model": "t = a + b * min(cap, n)"},
        "flat_model_sse": flat_sse,
        "locality_model_sse": fit_w["sse"],
        "fit_rss_vs_cap_bytes": {
            **fit_rss,
            "model": "peak_rss_bytes = a + b * min(cap, n)",
            "predicted_slope_range_for_4C_hypothesis": [4.0, 5.0],
            "slope_bytes_of_rss_per_byte_of_cap": fit_rss["slope"],
        },
        "verdict": verdict,
    }


def summarize(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    prep = json.loads((out / "prepare.json").read_text(encoding="utf-8"))
    tdir = out / "timing"
    rss_doc = load_json(out / "rss.json") or {}
    rss_rows = rss_doc.get("rows") or []

    combos: list[dict[str, Any]] = []
    for arm in prep["arms"]:
        for combo in prep["files"]:
            combos.append(summarise_combo(arm, combo["file"], prep, tdir, rss_rows))

    primary = next(
        (c for c in combos if c["arm"] == "bwt-forced" and c["file"] == prep["files"][0]["file"]),
        combos[0] if combos else None,
    )
    verdicts = sorted({c["verdict"] for c in combos})
    if primary is None:
        programme_verdict = "INCONCLUSIVE_NO_DATA"
    else:
        programme_verdict = primary["verdict"]

    doc = {
        "schema": 1,
        "candidate": "D1",
        "title": "BWT-path decode speed vs sub-block cap, routing held constant",
        "corpus": prep["corpus"],
        "caps_mib": prep["caps_mib"],
        "arms": prep["arms"],
        "decision_rule": DECISION_RULE,
        "kill_condition": KILL_CONDITION,
        "primary_combo": {
            "arm": primary["arm"],
            "file": primary["file"],
        }
        if primary
        else None,
        "programme_verdict": programme_verdict,
        "distinct_verdicts": verdicts,
        "combos": combos,
        "rss_rows": rss_rows,
    }
    (out / "d1-cap-speed.json").write_text(
        json.dumps(doc, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    md = render_markdown(doc)
    (out / "d1-cap-speed.md").write_text(md, encoding="utf-8")

    # The shared evidence manifest (`tools/i11_evidence_manifest.py`) reads
    # `ruling.json` and `provenance.txt`; keep the D1 verdict in both.
    ruling = {
        "ruling": programme_verdict,
        "gates": {
            "timing_valid": all(
                (c["null"] or {}).get("timing_valid") is not False for c in combos
            ),
            "d1_route_constant_across_caps": all(
                c["route_constant_across_caps"] for c in combos
            ),
            "d1_a_a_null_ci_spans_one": all(
                (c["null"] or {}).get("spans_one") for c in combos
            ),
        },
        "scope_note": (
            "I11 candidate D1 is a DIAGNOSTIC, not an arbiter arm. The I20 reference-cost "
            "gate checks (bytes vs xz/Brotli, decode ratio vs xz/Brotli, RSS ratio vs "
            "xz/Brotli) are NOT EVALUABLE from this artefact and are reported as null by "
            "tools/i11_evidence_manifest.py; they are neither passed nor failed here. The "
            "D1 ruling answers only the preregistered D1 question: is the BWT-path decode "
            "time a function of the sub-block cap once routing is held constant?"
        ),
        "d1_cap_speed": {
            "primary_combo": doc["primary_combo"],
            "programme_verdict": programme_verdict,
            "combos": [
                {
                    "arm": c["arm"],
                    "file": c["file"],
                    "routes": c["routes"],
                    "route_constant_across_caps": c["route_constant_across_caps"],
                    "span_max_over_min": c["span_max_over_min"],
                    "monotone": c["monotone"],
                    "flat_within_null_cv": c["flat_within_null_cv"],
                    "null_cv": (c["null"] or {}).get("robust_cv"),
                    "null_ci95": (c["null"] or {}).get("paired_ratio_ci95"),
                    "verdict": c["verdict"],
                    "curve": [
                        {
                            "cap_mib": r["cap_mib"],
                            "route": r["route"],
                            "payload_bytes": r["payload_bytes"],
                            "decode_s_median": r["decode_s_median"],
                            "paired_ratio_vs_uncapped": r["paired_ratio_vs_uncapped"],
                            "paired_ratio_ci95": r["paired_ratio_ci95"],
                            "peak_rss_kib": r["peak_rss_kib"],
                        }
                        for r in c["curve"]
                    ],
                }
                for c in combos
            ],
        },
    }
    (out / "ruling.json").write_text(
        json.dumps(ruling, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    # The shared evidence manifest (tools/i11_evidence_manifest.py) reads
    # `paired-null.json` for the A/A null check.  Publish the primary combination's
    # null at the artefact root in the driver's own summary schema; every other
    # combination's null stays in timing/null-<arm>-<file>.json.
    if primary is not None:
        src_null = out / "timing" / f"null-{primary['arm']}-{primary['file']}.json"
        if src_null.is_file():
            (out / "paired-null.json").write_text(
                src_null.read_text(encoding="utf-8"), encoding="utf-8"
            )
            (out / "paired-null-arm-file.txt").write_text(
                f"{primary['arm']} {primary['file']}\n", encoding="utf-8"
            )

    # Append the D1 verdict lines to provenance.txt so the shared manifest carries them.
    # Any previous d1_ lines are dropped first so that re-running summarize is idempotent.
    prov = out / "provenance.txt"
    kept: list[str] = []
    if prov.is_file():
        kept = [ln for ln in prov.read_text(encoding="utf-8").splitlines() if not ln.startswith("d1_")]
    lines: list[str] = []
    for c in combos:
        tag = f"{c['arm']}_{c['file']}"
        lines.append(f"d1_{tag}_verdict={c['verdict']}")
        lines.append(f"d1_{tag}_route_constant={int(bool(c['route_constant_across_caps']))}")
        lines.append(f"d1_{tag}_routes={','.join(c['routes'])}")
        lines.append(f"d1_{tag}_span_max_over_min={c['span_max_over_min']}")
        lines.append(f"d1_{tag}_null_cv={(c['null'] or {}).get('robust_cv')}")
        lines.append(f"d1_{tag}_null_ci95={(c['null'] or {}).get('paired_ratio_ci95')}")
        lines.append(f"d1_{tag}_monotone={c['monotone']}")
        lines.append(f"d1_{tag}_flat_within_null_cv={c['flat_within_null_cv']}")
        lines.append(f"d1_{tag}_rss_span_1mib_vs_uncapped_kib={rss_span(c, 1, CONTROL_CAP_MIB)}")
        lines.append(f"d1_{tag}_locality_slope_s_per_byte={(c['fit_locality_time_vs_lf_working_set'] or {}).get('slope_s_per_byte')}")
        lines.append(f"d1_{tag}_rss_slope_bytes_per_cap_byte={(c['fit_rss_vs_cap_bytes'] or {}).get('slope_bytes_of_rss_per_byte_of_cap')}")
    lines.append(f"d1_programme_verdict={programme_verdict}")
    with prov.open("w", encoding="utf-8") as f:
        if kept:
            f.write("\n".join(kept) + "\n")
        f.write("\n".join(lines) + "\n")

    print(f"verdict={programme_verdict}", flush=True)
    print(md, flush=True)


def render_markdown(doc: dict[str, Any]) -> str:
    out: list[str] = []
    out.append("# I11 D1 - BWT-path decode speed vs sub-block cap (routing held constant)")
    out.append("")
    out.append(f"- corpus: `{doc['corpus']}`")
    out.append(f"- caps (MiB): `{doc['caps_mib']}`")
    out.append(f"- arms: `{doc['arms']}`")
    out.append(f"- **verdict: `{doc['programme_verdict']}`**")
    out.append("")
    out.append("## Decision rule (fixed in advance)")
    out.append("")
    out.append("> " + doc["decision_rule"])
    out.append("")
    out.append("Kill condition: " + doc["kill_condition"])
    out.append("")
    for c in doc["combos"]:
        out.append(f"## {c['arm']} / {c['file']} - verdict `{c['verdict']}`")
        out.append("")
        out.append(
            f"- route per cap: `{json.dumps(c['route_per_cap'], sort_keys=True)}` "
            f"(constant across caps: **{c['route_constant_across_caps']}**)"
        )
        n = c["null"] or {}
        out.append(
            f"- A/A null: CI95 `{n.get('paired_ratio_ci95')}` spans 1.0: "
            f"**{n.get('spans_one')}**; null robust CV (the noise band): "
            f"**{n.get('robust_cv')}**"
        )
        out.append(
            f"- span (max/min relative decode time) = **{c['span_max_over_min']}**; "
            f"monotone = **{c['monotone']}**; flat within the null CV = "
            f"**{c['flat_within_null_cv']}**"
        )
        fit = c["fit_locality_time_vs_lf_working_set"]
        out.append(
            f"- fitted locality model `t = a + b * (4*min(cap,n))`: b = "
            f"`{fit.get('slope_s_per_byte')}` s/byte, R2 = `{fit.get('r2')}`; "
            f"model SSE `{fit.get('sse')}` vs flat-model SSE `{c.get('flat_model_sse')}`"
        )
        rfit = c["fit_rss_vs_cap_bytes"]
        out.append(
            f"- fitted RSS model `peak_rss = a + b * min(cap,n)`: b = "
            f"`{rfit.get('slope_bytes_of_rss_per_byte_of_cap')}` bytes of RSS per byte of cap "
            f"(the 4C hypothesis predicts 4..5), R2 = `{rfit.get('r2')}`"
        )
        out.append("")
        sub = c.get("supporting_dominant_route_subset") or {}
        if sub:
            out.append(
                f"- supporting subset (dominant route `{sub.get('dominant_route')}`, caps "
                f"`{sub.get('caps')}`): span `{sub.get('span_max_over_min')}`, monotone "
                f"`{sub.get('monotone')}`, flat within the null CV "
                f"`{sub.get('flat_within_null_cv')}` - {sub.get('note')}"
            )
            out.append("")
        out.append(
            "| cap (MiB) | route | sub-blocks | payload B | decode s (median) | "
            "paired ratio vs uncapped (CI95) | peak RSS KiB |"
        )
        out.append("|---:|---|---:|---:|---|---|---:|")
        for r in c["curve"]:
            out.append(
                f"| {r['cap_mib']} | {r['route']} | {r['subblocks']} | {r['payload_bytes']} | "
                f"{r['decode_s_median']} | {r['paired_ratio_vs_uncapped']} "
                f"{r['paired_ratio_ci95']} | {r['peak_rss_kib']} |"
            )
        out.append("")
    return "\n".join(out) + "\n"


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description="I11 D1 cap-speed sweep")
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("prepare")
    p.add_argument("--corpus", default="silesia")
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--caps-mib", default=",".join(str(c) for c in DEFAULT_CAPS_MIB))
    p.add_argument("--files", default="mr,x-ray")
    p.add_argument("--arms", default=",".join(ARMS))
    p.set_defaults(func=prepare)

    p = sub.add_parser("rss")
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--reps", type=int, default=3)
    p.set_defaults(func=rss)

    p = sub.add_parser("timing")
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--driver", type=Path, required=True)
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
    p.set_defaults(func=timing)

    p = sub.add_parser("verify")
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.set_defaults(func=verify)

    p = sub.add_parser("summarize")
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=summarize)

    return ap


def main() -> None:
    ap = build_parser()
    args = ap.parse_args()
    if hasattr(args, "files") and isinstance(args.files, str):
        args.files = parse_list(args.files)
    if hasattr(args, "arms") and isinstance(args.arms, str):
        args.arms = parse_list(args.arms)
    args.func(args)


if __name__ == "__main__":
    main()
