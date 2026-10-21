#!/usr/bin/env python3
"""Dense reference-grid control for ANVIL Iteration 20 (candidate C6).

CONTROL MEASUREMENT.  This tool does not select or alter codec behaviour, it
does not emit a frontier token, and it does not replace the frozen arbiter
``tools/reference_cost_gate.py``.  It answers exactly one pre-registered
question (I20 prereg section 5.1 C6, satisfying the repo's ``NQ-6`` control):

    Is the canonical ANVIL ratio arm non-dominated against a DENSE reference
    grid on the (bytes, decode throughput, peak RSS) axes?

Subcommands
-----------
prepare       Build every reference payload and the candidate arm, verify every
              round trip byte-exactly, write bytes.csv, per-arm command
              manifests and decode scripts.
rss           Per-process peak decode RSS for every arm (/usr/bin/time -f %M).
survey        One-shot same-job decode timing for every arm (ranking-grade).
plan-paired   Choose the paired comparisons that decide a gate, given survey.
summarize     Emit the dense reference front table and the non-dominance
              analysis (the deliverable).

Every measurement is produced inside a GitHub Actions job; this script never
measures locally.  The candidate arm is parameterised with the same
``--candidate-extra-args`` convention the coordinator's frozen
``tools/reference_cost_gate.py`` uses, so the two tools agree.

Re-run against any candidate arm (exact invocation, from the repo root):

    python3 tools/i20_dense_reference.py prepare \
        --corpus silesia --root "$CORPUS_ROOT" \
        --anvil build/anvil --brotli-lw build/brotli_lw \
        --brotli-dense build/brotli_dense \
        --brotli-dense-distro build/brotli_dense_distro \
        --xz "$(command -v xz)" --zstd "$(command -v zstd)" \
        --lz4 "$(command -v lz4)" \
        --candidate-extra-args --bwt-subblock 4194304 \
        --out "$OUT_DIR"
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import shlex
import statistics
import subprocess
import sys
import time
from pathlib import Path
from typing import Any

_TOOLS = Path(__file__).resolve().parent
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))

import pareto_front  # noqa: E402  repo arbiter: sole issuer of EXTENDS_FRONT

SCHEMA = 1
SILESIA_NAMES = (
    "dickens", "mozilla", "mr", "nci", "ooffice", "osdb",
    "reymont", "samba", "sao", "webster", "x-ray", "xml",
)
CANDIDATE_ARM = "anvil-ratio"
GATE_RATIO_LIMIT = 2.0
#: The repo's arbiter (tools/pareto_front.py) covers the (ratio, throughput)
#: plane.  The peak-RSS plane is the same predicate with the second axis
#: inverted (lower RSS is better), mirroring the cost gate's own "<= 2.0x RSS
#: ratio" form in tools/reference_cost_gate.py.  It is used only for the
#: control's third axis pair and is labelled as an extension everywhere.
RSS_AXIS = "peak_rss_kib"


# --------------------------------------------------------------------------
# small helpers (mirroring tools/reference_cost_gate.py so the two agree)
# --------------------------------------------------------------------------

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
        p = subprocess.run(argv, stdout=out, stderr=subprocess.PIPE, check=False)
        if p.returncode:
            raise RuntimeError(
                f"command failed rc={p.returncode}: {argv!r}\n"
                + p.stderr.decode(errors="replace")[-4000:]
            )
    finally:
        if handle is not None:
            handle.close()


def version_line(argv: list[str]) -> str:
    try:
        p = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           check=False)
        text = p.stdout.decode(errors="replace").strip().splitlines()
        return text[0].strip() if text else ""
    except OSError:
        return ""


def canonical_files(corpus: str, root: Path) -> list[Path]:
    if corpus == "enwik8":
        p = root / "enwik8" / "enwik8"
        if not p.is_file():
            raise RuntimeError(f"missing canonical enwik8: {p}")
        return [p]
    if corpus == "silesia":
        d = root / "silesia"
        files = [d / name for name in SILESIA_NAMES]
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


def split_extra_args(values: list[str]) -> list[str]:
    """Accept both ``--flag value`` token lists and one space-separated string."""
    out: list[str] = []
    for value in values or []:
        out.extend(value.split())
    return out


def set_affinity(cpu: int | None) -> list[int] | None:
    if cpu is None:
        return None
    if hasattr(os, "sched_setaffinity"):
        os.sched_setaffinity(0, {cpu})
        return sorted(os.sched_getaffinity(0))
    return None


def robust_cv(values: list[float]) -> float:
    if not values:
        return math.inf
    med = statistics.median(values)
    if med <= 0:
        return math.inf
    mad = statistics.median(abs(v - med) for v in values)
    return 1.4826 * mad / med


# --------------------------------------------------------------------------
# arm construction
# --------------------------------------------------------------------------

def build_arms(args: argparse.Namespace) -> list[dict[str, Any]]:
    """Return the dense-grid arm table.

    Every arm exposes:
      name       arm id (reference arms use the repo's REF_PREFIXES so that
                 tools/pareto_front.py classifies them as reference rows)
      kind       candidate | binding | dense | crosscheck
      family     anvil | brotli | xz | zstd | lz4
      producer   version/commit label of the exact producer used
      variants   ordered list of producer variants for this arm.  Each variant
                 carries its own `config` string, `encode` callable
                 ((src, dst) -> (argv, stdout_path|None)), `decode` callable
                 ((packed, out, n) -> argv) and `out_path` callable
                 ((packed, out) -> decoded output path).  The first variant
                 whose encode succeeds is the one that is measured; the
                 others are fallbacks for a producer whose full configuration
                 the runner image does not support.
    """
    arms: list[dict[str, Any]] = []

    # ---------------- candidate ----------------
    extra = split_extra_args(list(args.candidate_extra_args or []))

    def anvil_encode(src: Path, dst: Path):
        argv = [
            str(args.anvil), "c", str(src), str(dst),
            "--parse=ratio", "--ratio-backend=auto",
            "--ratio-context=off", "--ratio-lines=off",
            "--bwt-aux=on", "--quiet",
            *extra,
        ]
        return argv, None

    def anvil_decode(packed: Path, out: Path, n: int):
        return [str(args.anvil), "d", str(packed), str(out), "--quiet"]

    arms.append({
        "name": CANDIDATE_ARM,
        "kind": "candidate",
        "family": "anvil",
        "producer": args.anvil_producer or "frozen candidate binary",
        "variants": [{
            "config": "--parse=ratio --ratio-backend=auto --ratio-context=off "
                      "--ratio-lines=off --bwt-aux=on"
                      + ((" " + " ".join(extra)) if extra else ""),
            "encode": anvil_encode,
            "decode": anvil_decode,
            "out_path": lambda packed, out: out,
        }],
    })

    # ---------------- Brotli ----------------
    if args.brotli_lw:
        def blw_encode(src: Path, dst: Path):
            return [str(args.brotli_lw), "c", str(src), str(dst)], None

        def blw_decode(packed: Path, out: Path, n: int):
            return [str(args.brotli_lw), "d", str(packed), str(out), str(n)]

        arms.append({
            "name": "brotli-q11-lw30-gate",
            "kind": "binding",
            "family": "brotli",
            "producer": args.brotli_gate_producer
                        or "repo brotli_lw + distribution libbrotli-dev",
            "variants": [{
                "config": "q11 lgwin30 (repo tools/brotli_lw.cpp, the gate's arm)",
                "encode": blw_encode,
                "decode": blw_decode,
                "out_path": lambda packed, out: out,
            }],
        })

    if args.brotli_dense_distro:
        arms.append(_brotli_dense_arm(
            "brotli-q11-lw30-distro", "crosscheck", 11, 30,
            args.brotli_dense_distro,
            args.brotli_distro_producer or "distribution libbrotli-dev",
            "q11 lgwin30 (parameterised helper, distribution libbrotli-dev) "
            "== helper-equivalence control",
        ))

    if args.brotli_dense:
        for q, lw in ((11, 30), (11, 24), (11, 22), (9, 22),
                      (6, 22), (4, 22), (1, 22)):
            kind = "dense"
            arms.append(_brotli_dense_arm(
                f"brotli-q{q}-lw{lw}", kind, q, lw, args.brotli_dense,
                args.brotli_pinned_producer or "pinned google/brotli",
                f"q{q} lgwin{lw} (parameterised helper, pinned producer)",
            ))

    # ---------------- xz ----------------
    if args.xz:
        for level, kind in (("9e", "binding"), ("9", "dense"), ("6", "dense")):
            arms.append(_xz_arm(f"xz-{level}", kind, level, args.xz,
                                 args.xz_producer or "distribution xz"))
        if args.xz_pinned:
            arms.append(_xz_arm("xz-9e-pinned", "crosscheck", "9e",
                                 args.xz_pinned,
                                 args.xz_pinned_producer or "pinned xz source build"))

    # ---------------- zstd ----------------
    if args.zstd:
        for level in (1, 3, 9, 19):
            arms.append(_zstd_arm(f"zstd-{level}", level, args.zstd,
                                   args.zstd_producer or "distribution zstd"))
        arms.append(_zstd_arm("zstd-22", 22, args.zstd,
                               args.zstd_producer or "distribution zstd",
                               ultra=True, long_window=True))

    # ---------------- lz4 ----------------
    if args.lz4:
        for level in (1, 9):
            arms.append(_lz4_arm(f"lz4-{level}", level, args.lz4,
                                  args.lz4_producer or "distribution lz4"))

    if args.only:
        wanted = {s.strip() for s in args.only.split(",") if s.strip()}
        arms = [a for a in arms if a["name"] in wanted]
    return arms


def _brotli_dense_arm(name: str, kind: str, q: int, lw: int, binary: Path,
                       producer: str, config: str) -> dict[str, Any]:
    def encode(src: Path, dst: Path):
        return [str(binary), "c", str(q), str(lw), str(src), str(dst)], None

    def decode(packed: Path, out: Path, n: int):
        return [str(binary), "d", str(packed), str(out), str(n)]

    return {
        "name": name,
        "kind": kind,
        "family": "brotli",
        "producer": producer,
        "variants": [{
            "config": config,
            "encode": encode,
            "decode": decode,
            "out_path": lambda packed, out: out,
        }],
    }


def _xz_arm(name: str, kind: str, level: str, binary: Path,
            producer: str) -> dict[str, Any]:
    def encode(src: Path, dst: Path):
        return [str(binary), f"-{level}", "-c", str(src)], dst

    def decode(packed: Path, out: Path, n: int):
        # `xz -d -k -f` writes next to the compressed file, dropping `.xz`.
        return [str(binary), "-d", "-k", "-f", str(packed)]

    return {
        "name": name,
        "kind": kind,
        "family": "xz",
        "producer": producer,
        "variants": [{
            "config": f"-{level} (per-file, single-threaded)",
            "encode": encode,
            "decode": decode,
            # `xz -d -k -f` writes next to the compressed file, dropping `.xz`.
            "out_path": lambda packed, out: packed.with_suffix(""),
        }],
    }


def _zstd_arm(name: str, level: int, binary: Path, producer: str, *,
              ultra: bool = False, long_window: bool = False) -> dict[str, Any]:
    def make_variant(extra_encode: list[str], extra_decode: list[str],
                     config: str) -> dict[str, Any]:
        def encode(src: Path, dst: Path):
            argv = [str(binary), "-q", "-f"]
            if ultra:
                argv.append("--ultra")
            argv.append(f"-{level}")
            argv += list(extra_encode)
            argv += ["-o", str(dst), str(src)]
            return argv, None

        def decode(packed: Path, out: Path, n: int):
            argv = [str(binary), "-q", "-f", "-d"]
            argv += list(extra_decode)
            argv += ["-o", str(out), str(packed)]
            return argv

        return {
            "config": config,
            "encode": encode,
            "decode": decode,
            "out_path": lambda packed, out: out,
        }

    base = f"-{level}" + (" --ultra" if ultra else "")
    variants: list[dict[str, Any]] = []
    if long_window:
        # zstd's long mode needs a window larger than the 8 MiB default, and
        # the decode side must be told the same window log.  The memory cap
        # uses a plain count with a K/M/G suffix; "4096MB" overflows zstd's
        # 32-bit unsigned parse, and 1G is ample for a 128 MiB window.
        variants.append(make_variant(
            ["--long=27", "--memory=1G"], ["--long=27"],
            base + " --long=27 --memory=1G (per-file, single-threaded)"))
        variants.append(make_variant(
            ["--long=27"], ["--long=27"],
            base + " --long=27 (per-file, single-threaded)"))
        variants.append(make_variant(
            [], [],
            base + " (long mode unavailable on this build; per-file, single-threaded)"))
    else:
        variants.append(make_variant(
            [], [], base + " (per-file, single-threaded)"))
    return {
        "name": name,
        "kind": "dense",
        "family": "zstd",
        "producer": producer,
        "variants": variants,
    }


def _lz4_arm(name: str, level: int, binary: Path, producer: str) -> dict[str, Any]:
    def encode(src: Path, dst: Path):
        return [str(binary), "-q", "-f", f"-{level}", str(src), str(dst)], None

    def decode(packed: Path, out: Path, n: int):
        return [str(binary), "-q", "-f", "-d", str(packed), str(out)]

    return {
        "name": name,
        "kind": "dense",
        "family": "lz4",
        "producer": producer,
        "variants": [{
            "config": f"-{level} (per-file, single-threaded)",
            "encode": encode,
            "decode": decode,
            "out_path": lambda packed, out: out,
        }],
    }


# --------------------------------------------------------------------------
# prepare
# --------------------------------------------------------------------------

def cmd_prepare(args: argparse.Namespace) -> None:
    root = args.root.resolve()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    files = canonical_files(args.corpus, root)
    source_total = sum(p.stat().st_size for p in files)
    source_digest = {p.name: sha256(p) for p in files}

    arms = build_arms(args)
    pack_root = out / "packed"
    dec_root = out / "decoded"
    for arm in arms:
        (pack_root / arm["name"]).mkdir(parents=True, exist_ok=True)
        (dec_root / arm["name"]).mkdir(parents=True, exist_ok=True)

    byte_rows: list[dict[str, Any]] = []
    arm_records: list[dict[str, Any]] = []
    failures: list[dict[str, Any]] = []

    for arm in arms:
        name = arm["name"]
        commands: list[list[str]] = []
        outputs: list[Path] = []
        packed_paths: list[Path] = []
        arm_bytes = 0
        status = "OK"
        error = ""
        variant: dict[str, Any] | None = None
        variant_errors: list[str] = []

        # ---- encode (the first variant that works is the measured one) ----
        for src in files:
            ext = {"anvil": ".anv", "brotli": ".br", "xz": ".xz",
                   "zstd": ".zst", "lz4": ".lz4"}[arm["family"]]
            packed = pack_root / name / f"{src.name}{ext}"
            decoded = dec_root / name / src.name
            if variant is None:
                last_error = ""
                for candidate in arm["variants"]:
                    try:
                        argv, stdout_path = candidate["encode"](src, packed)
                        run(argv, stdout_path=stdout_path)
                    except Exception as exc:  # noqa: BLE001 - try the next variant
                        last_error = str(exc)[:2000]
                        variant_errors.append(f"{candidate['config']}: {last_error}")
                        if packed.is_file():
                            packed.unlink()
                        continue
                    if not packed.is_file():
                        last_error = "encode produced no file"
                        variant_errors.append(f"{candidate['config']}: {last_error}")
                        continue
                    variant = candidate
                    break
                if variant is None:
                    status = "FAILED"
                    error = f"encode {src.name}: {last_error}"
                    failures.append({"arm": name, "stage": "encode",
                                     "file": src.name, "error": error,
                                     "variant_errors": variant_errors})
                    break
            else:
                argv, stdout_path = variant["encode"](src, packed)
                try:
                    run(argv, stdout_path=stdout_path)
                except Exception as exc:  # noqa: BLE001
                    status = "FAILED"
                    error = f"encode {src.name}: {exc}"
                    failures.append({"arm": name, "stage": "encode",
                                     "file": src.name, "error": str(exc)[:2000]})
                    break
                if not packed.is_file():
                    status = "FAILED"
                    error = f"encode produced no file for {src.name}"
                    failures.append({"arm": name, "stage": "encode",
                                     "file": src.name, "error": error})
                    break
            argv = variant["decode"](packed, decoded, src.stat().st_size)
            commands.append([str(x) for x in argv])
            out_path = variant["out_path"](packed, decoded)
            outputs.append(out_path)
            packed_paths.append(packed)
            arm_bytes += packed.stat().st_size
            byte_rows.append({
                "corpus": args.corpus,
                "file": src.name,
                "input_bytes": src.stat().st_size,
                "source_sha256": source_digest[src.name],
                "codec": name,
                "arm_kind": arm["kind"],
                "compressed_bytes": packed.stat().st_size,
                "ratio": f"{packed.stat().st_size / src.stat().st_size:.9f}",
            })

        # ---- verify every round trip byte-exactly, before any timing ----
        if status == "OK":
            for src, argv, out_path in zip(files, commands, outputs):
                try:
                    run(argv)
                except Exception as exc:  # noqa: BLE001
                    status = "FAILED"
                    error = f"decode {src.name}: {exc}"
                    failures.append({"arm": name, "stage": "decode",
                                     "file": src.name, "error": str(exc)[:2000]})
                    break
                if not out_path.is_file():
                    status = "FAILED"
                    error = f"decoder produced no output for {src.name}"
                    failures.append({"arm": name, "stage": "decode",
                                     "file": src.name, "error": error})
                    break
                if (out_path.stat().st_size != src.stat().st_size
                        or sha256(out_path) != source_digest[src.name]):
                    status = "FAILED"
                    error = f"roundtrip mismatch for {src.name}"
                    failures.append({"arm": name, "stage": "verify",
                                     "file": src.name, "error": error})
                    break

        record = {
            "name": name,
            "kind": arm["kind"],
            "family": arm["family"],
            "config": (variant["config"] if variant is not None
                       else " | ".join(v["config"] for v in arm["variants"])),
            "producer": arm["producer"],
            "status": status,
            "error": error,
            "variant_errors": variant_errors,
            "compressed_bytes": arm_bytes if status == "OK" else 0,
            "commands_manifest": f"{name}-commands.json",
            "decode_script": f"decode-{name}.sh",
        }
        arm_records.append(record)

        if status != "OK":
            # A failed arm is a disclosed gap, not a silent omission.
            (out / f"{name}-commands.json").write_text(
                json.dumps({"schema": SCHEMA, "arm": name, "commands": [],
                            "outputs": []}, indent=2) + "\n", encoding="utf-8")
            continue

        (out / f"{name}-commands.json").write_text(
            json.dumps({
                "schema": SCHEMA,
                "arm": name,
                "kind": arm["kind"],
                "family": arm["family"],
                "config": record["config"],
                "producer": arm["producer"],
                "commands": commands,
                "outputs": [str(p) for p in outputs],
                "packed": [str(p) for p in packed_paths],
            }, indent=2) + "\n", encoding="utf-8")
        write_script(out / f"decode-{name}.sh", commands)

    # ---- bytes.csv ----
    fields = ["corpus", "file", "input_bytes", "source_sha256", "codec",
              "arm_kind", "compressed_bytes", "ratio"]
    with (out / "bytes.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(byte_rows)

    # ---- candidate byte-identity assertion ----
    if args.expected_candidate_total is not None:
        got = next((a["compressed_bytes"] for a in arm_records
                    if a["name"] == CANDIDATE_ARM), 0)
        if got != args.expected_candidate_total:
            raise RuntimeError(
                f"candidate byte identity failed: {got} != "
                f"{args.expected_candidate_total}")

    # ---- producer provenance (exact version strings, captured in-job) ----
    producers: dict[str, Any] = {
        "anvil_candidate": {
            "label": args.anvil_producer or "",
            "binary_sha256": sha256(args.anvil) if args.anvil and args.anvil.is_file() else "",
        },
    }
    for label, binary in (("brotli_gate", args.brotli_lw),
                          ("brotli_dense_pinned", args.brotli_dense),
                          ("brotli_dense_distro", args.brotli_dense_distro),
                          ("xz", args.xz), ("xz_pinned", args.xz_pinned),
                          ("zstd", args.zstd), ("lz4", args.lz4)):
        if not binary:
            continue
        binary = Path(binary)
        entry: dict[str, Any] = {
            "path_name": binary.name,
            "sha256": sha256(binary) if binary.is_file() else "",
        }
        if binary.is_file():
            entry["version_raw"] = version_line([str(binary), "--version"])
        producers[label] = entry

    summary = {
        "schema": SCHEMA,
        "corpus": args.corpus,
        "source_bytes": source_total,
        "file_count": len(files),
        "source_sha256": source_digest,
        "arms": arm_records,
        "compressed_bytes": {a["name"]: a["compressed_bytes"]
                             for a in arm_records if a["status"] == "OK"},
        "producers": producers,
        "candidate_extra_args": split_extra_args(list(args.candidate_extra_args or [])),
        "failures": failures,
        "roundtrip": "OK" if not failures else "PARTIAL",
    }
    (out / "prepare-summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(
        {"corpus": args.corpus, "source_bytes": source_total,
         "arms": [{"name": a["name"], "kind": a["kind"],
                   "status": a["status"], "compressed_bytes": a["compressed_bytes"]}
                  for a in arm_records],
         "failures": len(failures)}, indent=2, sort_keys=True))


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
    p = subprocess.run([str(time_bin), "-f", "%M", "-o", str(rss_file), "--", *cmd],
                       stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, check=False)
    if p.returncode:
        raise RuntimeError(
            f"RSS command failed rc={p.returncode}: {cmd!r}\n"
            + p.stderr.decode(errors="replace")[-4000:])
    return int(rss_file.read_text(encoding="utf-8").strip())


def cmd_rss(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    manifests: list[Path] = list(args.manifest or [])
    if args.prepare:
        prep = json.loads(args.prepare.read_text(encoding="utf-8"))
        base = args.prepare.parent
        for arm in prep["arms"]:
            if arm["status"] == "OK":
                manifests.append(base / arm["commands_manifest"])
    rows: list[dict[str, Any]] = []
    with_temp = out / ".rss-tmp"
    with_temp.mkdir(parents=True, exist_ok=True)
    for manifest in manifests:
        payload = json.loads(Path(manifest).read_text(encoding="utf-8"))
        arm = str(payload["arm"])
        commands = payload["commands"]
        samples: list[int] = []
        for i, cmd in enumerate(commands):
            kib = measure_one_rss([str(x) for x in cmd], with_temp)
            samples.append(kib)
            rows.append({"arm": arm, "index": i, "max_rss_kib": kib})
        if samples:
            print(f"{arm}: max_rss={max(samples):,} KiB files={len(samples)}")
    try:
        with_temp.rmdir()
    except OSError:
        pass

    with (out / "rss.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["arm", "index", "max_rss_kib"])
        w.writeheader()
        w.writerows(rows)
    grouped: dict[str, list[int]] = {}
    for row in rows:
        grouped.setdefault(str(row["arm"]), []).append(int(row["max_rss_kib"]))
    result = {
        "schema": SCHEMA,
        "max_rss_kib": {arm: max(vals) for arm, vals in grouped.items()},
        "samples": rows,
    }
    (out / "rss.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")


# --------------------------------------------------------------------------
# survey (ranking-grade, one-shot, same job)
# --------------------------------------------------------------------------

def cmd_survey(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    affinity = set_affinity(args.cpu)
    arms = [a for a in prep["arms"] if a["status"] == "OK"]
    if args.only:
        wanted = {s.strip() for s in args.only.split(",") if s.strip()}
        arms = [a for a in arms if a["name"] in wanted]

    results: dict[str, Any] = {}
    rows: list[dict[str, Any]] = []
    for arm in arms:
        script = out / arm["decode_script"]
        manifest = json.loads((out / arm["commands_manifest"]).read_text(encoding="utf-8"))
        outputs = [Path(p) for p in manifest["outputs"]]
        source_sha = prep["source_sha256"]
        times: list[float] = []
        for i in range(args.warmups + args.reps):
            t0 = time.perf_counter()
            p = subprocess.run(["bash", str(script)], stdout=subprocess.DEVNULL,
                               stderr=subprocess.PIPE, check=False)
            elapsed = time.perf_counter() - t0
            if p.returncode:
                raise RuntimeError(
                    f"survey decode failed rc={p.returncode} for {arm['name']}\n"
                    + p.stderr.decode(errors="replace")[-4000:])
            if i >= args.warmups:
                times.append(elapsed)
        # Force the decoded bytes through a checksum (the decode output is
        # materialised on disk and hashed here; it is never /dev/null).
        hashes_ok = True
        for out_path in outputs:
            if not out_path.is_file():
                hashes_ok = False
                break
            if sha256(out_path) != source_sha[out_path.name]:
                hashes_ok = False
                break
        median = statistics.median(times)
        mbps = (prep["source_bytes"] / 1e6) / median if median > 0 else 0.0
        results[arm["name"]] = {
            "reps_s": times,
            "median_s": median,
            "robust_cv": robust_cv(times),
            "decode_MBps": mbps,
            "output_sha256_ok": hashes_ok,
            "timing_grade": "SURVEY (ranking-grade, one-shot, same job)",
        }
        rows.append({
            "arm": arm["name"], "kind": arm["kind"], "family": arm["family"],
            "reps": len(times), "median_s": f"{median:.6f}",
            "decode_MBps": f"{mbps:.3f}", "robust_cv": f"{robust_cv(times):.4f}",
            "output_sha256_ok": str(hashes_ok).lower(),
        })
        print(f"survey {arm['name']}: median={median:.3f}s "
              f"{mbps:.2f} MB/s cv={robust_cv(times):.4f} ok={hashes_ok}")

    payload = {
        "schema": SCHEMA,
        "corpus": prep["corpus"],
        "affinity": affinity,
        "reps": args.reps,
        "warmups": args.warmups,
        "source_bytes": prep["source_bytes"],
        "arms": results,
    }
    (out / "survey-timing.json").write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    with (out / "survey-timing.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["arm", "kind", "family", "reps",
                                          "median_s", "decode_MBps", "robust_cv",
                                          "output_sha256_ok"])
        w.writeheader()
        w.writerows(rows)


# --------------------------------------------------------------------------
# plan-paired
# --------------------------------------------------------------------------

def _load_rows(prep: dict[str, Any], survey: dict[str, Any],
               rss: dict[str, Any]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for arm in prep["arms"]:
        if arm["status"] != "OK":
            continue
        s = survey.get("arms", {}).get(arm["name"], {})
        r = rss.get("max_rss_kib", {}).get(arm["name"])
        rows.append({
            "codec": arm["name"],
            "kind": arm["kind"],
            "family": arm["family"],
            "config": arm["config"],
            "producer": arm["producer"],
            "portfolio_bytes": int(arm["compressed_bytes"]),
            "ratio": int(arm["compressed_bytes"]) / int(prep["source_bytes"]),
            "decode_MBps": float(s.get("decode_MBps") or 0.0),
            "decode_median_s": float(s.get("median_s") or 0.0),
            "peak_rss_kib": int(r) if r is not None else None,
        })
    return rows


def cmd_plan_paired(args: argparse.Namespace) -> None:
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    survey = json.loads(args.survey.read_text(encoding="utf-8"))
    rss = json.loads(args.rss.read_text(encoding="utf-8")) if args.rss else {}
    rows = _load_rows(prep, survey, rss)
    cand = next(r for r in rows if r["codec"] == CANDIDATE_ARM)
    refs = [r for r in rows if r["codec"] != CANDIDATE_ARM]
    timed = [r for r in refs if r["decode_MBps"] > 0]
    byte_competitive = [r for r in timed
                        if r["portfolio_bytes"] <= cand["portfolio_bytes"]]

    def pressure(ref: dict[str, Any]) -> float:
        if ref["decode_MBps"] <= 0:
            return 0.0
        cand_s = prep["source_bytes"] / 1e6 / cand["decode_MBps"]
        ref_s = prep["source_bytes"] / 1e6 / ref["decode_MBps"]
        return cand_s / ref_s if ref_s > 0 else 0.0

    pool = byte_competitive or timed
    binding_decode = max(pool, key=pressure) if pool else None
    min_bytes = min(timed, key=lambda r: r["portfolio_bytes"]) if timed else None

    plan: list[dict[str, Any]] = []
    seen: set[str] = set()

    def add(name: str, kind: str, reason: str) -> None:
        if name in seen:
            return
        seen.add(name)
        plan.append({
            "name": name,
            "kind": kind,
            "control_script": f"decode-{name}.sh",
            "candidate_script": f"decode-{CANDIDATE_ARM}.sh",
            "reason": reason,
        })

    add(CANDIDATE_ARM, "null", "mandatory A/A null pair")
    for required in ("xz-9e", "brotli-q11-lw30-gate"):
        if any(r["codec"] == required for r in rows):
            add(required, "binding", "binding gate reference arm")
    if min_bytes:
        add(min_bytes["codec"], "min-bytes",
            "NQ-6: best reference point in the frozen dense grid "
            f"({min_bytes['portfolio_bytes']:,} B)")
    if binding_decode and binding_decode["codec"] != CANDIDATE_ARM:
        add(binding_decode["codec"], "binding-decode",
            "strictest decode bar among byte-competitive references "
            f"(survey pressure {pressure(binding_decode):.3f}x)")

    payload = {"schema": SCHEMA, "corpus": prep["corpus"], "pairs": plan}
    args.out.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n",
                        encoding="utf-8")
    print(json.dumps(payload, indent=2, sort_keys=True))


# --------------------------------------------------------------------------
# summarize (the deliverable)
# --------------------------------------------------------------------------

def dominated_rss(p: dict[str, Any], refs: list[dict[str, Any]]) -> dict[str, Any] | None:
    """RSS-plane dominance: the repo's predicate with the second axis inverted.

    `q` dominates `p` iff q.ratio <= p.ratio AND q.peak_rss_kib <= p.peak_rss_kib
    AND one of them is strict.  Lower peak RSS is better, mirroring the cost
    gate's own "candidate peak RSS <= 2.0x reference peak RSS" form.
    """
    if p.get("peak_rss_kib") is None:
        return None
    pr, pm = float(p["ratio"]), float(p["peak_rss_kib"])
    for q in refs:
        if q.get("peak_rss_kib") is None:
            continue
        qr, qm = float(q["ratio"]), float(q["peak_rss_kib"])
        if qr <= pr and qm <= pm and (qr < pr or qm < pm):
            return q
    return None


def cmd_summarize(args: argparse.Namespace) -> None:
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    survey = json.loads(args.survey.read_text(encoding="utf-8"))
    rss = json.loads(args.rss.read_text(encoding="utf-8")) if args.rss else {}
    paired: dict[str, Any] = {}
    for item in (args.paired or []):
        if "=" not in item:
            continue
        key, path = item.split("=", 1)
        paired[key] = json.loads(Path(path).read_text(encoding="utf-8"))

    rows = _load_rows(prep, survey, rss)
    cand = next(r for r in rows if r["codec"] == CANDIDATE_ARM)
    refs = [r for r in rows if r["codec"] != CANDIDATE_ARM]
    repo_refs = [r for r in refs if pareto_front.is_ref(r["codec"])]
    extended_refs = refs  # adds lz4-* (outside the repo's REF_PREFIXES)

    # --- reference front within the dense grid (the repo's arbiter) ---
    front_rows = []
    for r in refs:
        d = pareto_front.dominated(r, extended_refs, "decode_MBps")
        front_rows.append({
            "codec": r["codec"], "kind": r["kind"], "family": r["family"],
            "config": r["config"], "producer": r["producer"],
            "portfolio_bytes": r["portfolio_bytes"],
            "ratio": f"{r['ratio']:.9f}",
            "decode_MBps": f"{r['decode_MBps']:.3f}",
            "peak_rss_kib": r["peak_rss_kib"] if r["peak_rss_kib"] is not None else "",
            "peak_rss_MiB": (f"{r['peak_rss_kib'] / 1024:.1f}"
                             if r["peak_rss_kib"] is not None else ""),
            "ref_front": "FRONT" if d is None else "DOMINATED",
            "dominated_by": "" if d is None else d["codec"],
            "in_repo_ref_prefixes": str(pareto_front.is_ref(r["codec"])).lower(),
        })

    # --- dominance of the candidate ---
    def dominators(axis: str) -> list[dict[str, Any]]:
        out: list[dict[str, Any]] = []
        for q in extended_refs:
            if axis == "bytes_decode":
                if q["decode_MBps"] <= 0:
                    continue
                d = pareto_front.dominated(cand, [q], "decode_MBps")
            else:
                d = dominated_rss(cand, [q])
            if d is not None:
                out.append({
                    "reference": q["codec"],
                    "bytes_ratio_reference_over_candidate":
                        f"{q['portfolio_bytes'] / cand['portfolio_bytes']:.6f}",
                    "bytes_advantage_bytes":
                        cand["portfolio_bytes"] - q["portfolio_bytes"],
                    "decode_time_ratio_candidate_over_reference":
                        (f"{(cand['decode_median_s'] / q['decode_median_s']):.6f}"
                         if q["decode_median_s"] > 0 else ""),
                    "rss_ratio_candidate_over_reference":
                        (f"{cand['peak_rss_kib'] / q['peak_rss_kib']:.6f}"
                         if q.get("peak_rss_kib") and cand.get("peak_rss_kib")
                         else ""),
                })
        return out

    dom_bytes_decode = dominators("bytes_decode")
    dom_bytes_rss = dominators("bytes_rss")

    # --- binding references ---
    timed = [r for r in refs if r["decode_MBps"] > 0]
    byte_competitive = [r for r in timed
                        if r["portfolio_bytes"] <= cand["portfolio_bytes"]]

    def decode_pressure(r: dict[str, Any]) -> float:
        if r["decode_median_s"] <= 0:
            return 0.0
        return cand["decode_median_s"] / r["decode_median_s"]

    def rss_pressure(r: dict[str, Any]) -> float:
        if not r.get("peak_rss_kib") or not cand.get("peak_rss_kib"):
            return 0.0
        return cand["peak_rss_kib"] / r["peak_rss_kib"]

    pool = byte_competitive or timed
    binding_decode = max(pool, key=decode_pressure) if pool else None
    rss_pool = byte_competitive or [r for r in timed if r.get("peak_rss_kib")]
    binding_rss = max(rss_pool, key=rss_pressure) if rss_pool else None
    min_bytes = min(timed, key=lambda r: r["portfolio_bytes"]) if timed else None

    def paired_block(name: str | None) -> dict[str, Any] | None:
        if not name or name not in paired:
            return None
        p = paired[name]
        lo, hi = [float(x) for x in p["paired_ratio_ci95"]]
        return {
            "paired_name": name,
            "status": p.get("status"),
            "timing_valid": bool(p.get("timing_valid")),
            "ratio_candidate_over_reference": float(p["paired_ratio_candidate_over_control"]),
            "ci95": [lo, hi],
            "reference_median_s": float(p["control"]["median_s"]),
            "candidate_median_s": float(p["candidate"]["median_s"]),
            "reference_MBps": (prep["source_bytes"] / 1e6) / float(p["control"]["median_s"]),
            "candidate_MBps": (prep["source_bytes"] / 1e6) / float(p["candidate"]["median_s"]),
            "gate_2x_ci_upper": "PASS" if hi <= GATE_RATIO_LIMIT else "FAIL",
        }

    null = paired.get(CANDIDATE_ARM) or paired.get("null")
    null_block = None
    if null is not None:
        lo, hi = [float(x) for x in null["paired_ratio_ci95"]]
        null_block = {
            "ci95": [lo, hi],
            "spans_one": bool(lo <= 1.0 <= hi),
            "clears_speed_gate": bool(null.get("clears_speed_gate")),
            "timing_valid": bool(null.get("timing_valid")),
            "status": null.get("status"),
        }

    dense_byte_bar = None
    if min_bytes is not None:
        dense_byte_bar = {
            "reference": min_bytes["codec"],
            "portfolio_bytes": min_bytes["portfolio_bytes"],
            "candidate_bytes": cand["portfolio_bytes"],
            "candidate_minus_reference_bytes":
                cand["portfolio_bytes"] - min_bytes["portfolio_bytes"],
            "candidate_is_byte_worse": cand["portfolio_bytes"] > min_bytes["portfolio_bytes"],
            "note": ("NQ-6: the byte figure must be stated against the best "
                     "reference point in the frozen dense grid, not only "
                     "against brotli q11/lgwin30."),
        }

    def gate_line(ref: dict[str, Any] | None, block: dict[str, Any] | None,
                  axis: str) -> dict[str, Any] | None:
        if ref is None:
            return None
        entry: dict[str, Any] = {"reference": ref["codec"],
                                 "axis": axis,
                                 "reference_portfolio_bytes": ref["portfolio_bytes"]}
        if axis == "decode":
            entry["survey_decode_time_ratio"] = f"{decode_pressure(ref):.6f}"
        else:
            entry["survey_rss_ratio"] = f"{rss_pressure(ref):.6f}"
        if block:
            entry["paired"] = block
            entry["strictest_gate"] = block["gate_2x_ci_upper"]
        return entry

    binding = {
        "min_bytes_reference": dense_byte_bar,
        "decode": gate_line(binding_decode,
                            paired_block(binding_decode["codec"]) if binding_decode else None,
                            "decode"),
        "rss": gate_line(binding_rss, None, "peak_rss"),
    }

    dominated = bool(dom_bytes_decode or dom_bytes_rss)
    cand_survey = survey.get("arms", {}).get(CANDIDATE_ARM)
    if dominated:
        verdict = "NO-GO"
    elif not cand_survey or not cand_survey.get("output_sha256_ok"):
        verdict = "INCONCLUSIVE"
    elif cand.get("peak_rss_kib") is None:
        verdict = "INCONCLUSIVE"
    else:
        verdict = "WIN"

    analysis = {
        "schema": SCHEMA,
        "corpus": prep["corpus"],
        "tool": "tools/i20_dense_reference.py summarize",
        "class": "CONTROL MEASUREMENT ({adopt-class}/{engineering}; "
                 "no frontier token is issued by this tool)",
        "candidate": {
            "arm": cand["codec"],
            "config": cand["config"],
            "producer": cand["producer"],
            "portfolio_bytes": cand["portfolio_bytes"],
            "ratio": f"{cand['ratio']:.9f}",
            "decode_MBps_survey": f"{cand['decode_MBps']:.3f}",
            "decode_median_s_survey": f"{cand['decode_median_s']:.6f}",
            "peak_rss_kib": cand["peak_rss_kib"],
            "peak_rss_MiB": (f"{cand['peak_rss_kib'] / 1024:.1f}"
                             if cand["peak_rss_kib"] is not None else None),
        },
        "non_dominance": {
            "rule": "q dominates p iff q.ratio <= p.ratio AND q.mbps >= p.mbps "
                    "AND (q.ratio < p.ratio OR q.mbps > p.mbps) "
                    "(tools/pareto_front.py::dominated, sole issuer of "
                    "EXTENDS_FRONT); the peak-RSS plane uses the same "
                    "predicate with the second axis inverted.",
            "bytes_decode": {
                "status": "DOMINATED" if dom_bytes_decode else "EXTENDS_FRONT",
                "dominators": dom_bytes_decode,
            },
            "bytes_peak_rss": {
                "status": "DOMINATED" if dom_bytes_rss else "EXTENDS_FRONT",
                "dominators": dom_bytes_rss,
            },
        },
        "binding_reference": binding,
        "null_control": null_block,
        "dual_bar": {
            "statement": "The canonical --parse=ratio portfolio applies "
                         "transform_id 0 (direct) to every corpus file, so no "
                         "transform-enabled reference row applies to this cell "
                         "and the dual-bar qualifier is vacuous here. As in "
                         "docs/gate-ruling-i9-frontier-tokens.md, same-transform "
                         "controls (xz --delta/--x86) remain side-channel only "
                         "and are never arbiter front rows.",
            "transform_enabled_rows_applied": False,
        },
        "grid_label": "GRID-THIN: this grid is denser than the binding gate "
                      "grid but is still not exhaustive; see the gap list in "
                      "docs/I20-RESULTS-W6-dense-reference.md.",
        "verdict": verdict,
        "verdict_rule": "The final candidate is reportable only if it is "
                        "non-dominated against the dense grid on the same axes. "
                        "If a denser reference point dominates it, the program "
                        "closes NO-GO on the dense grid, and this is recorded "
                        "as the binding result.",
    }

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(analysis, indent=2, sort_keys=True) + "\n",
                        encoding="utf-8")
    (args.out.parent / "dense-front.csv").write_text("", encoding="utf-8")
    with (args.out.parent / "dense-front.csv").open("w", newline="",
                                                      encoding="utf-8") as f:
        fields = ["codec", "kind", "family", "config", "producer",
                  "portfolio_bytes", "ratio", "decode_MBps", "peak_rss_kib",
                  "peak_rss_MiB", "ref_front", "dominated_by",
                  "in_repo_ref_prefixes"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(front_rows)

    md: list[str] = []
    md.append(f"# I20 dense reference front - {prep['corpus']}")
    md.append("")
    md.append(f"**Control verdict: {verdict}** "
              f"(class: control measurement, no frontier token issued)")
    md.append("")
    md.append("| Arm | Kind | Portfolio bytes | Ratio | Decode MB/s (survey) "
              "| Peak RSS MiB | Ref front |")
    md.append("|---|---|---:|---:|---:|---:|---|")
    for r in sorted(front_rows, key=lambda x: x["portfolio_bytes"]):
        md.append(f"| {r['codec']} | {r['kind']} | {r['portfolio_bytes']:,} "
                  f"| {r['ratio']} | {r['decode_MBps']} | {r['peak_rss_MiB']} "
                  f"| {r['ref_front']} |")
    md.append("")
    md.append(f"- candidate: {cand['codec']} "
              f"{cand['portfolio_bytes']:,} B, ratio {cand['ratio']:.9f}, "
              f"peak RSS {cand['peak_rss_kib']} KiB")
    md.append(f"- (bytes, decode) status: {analysis['non_dominance']['bytes_decode']['status']}")
    md.append(f"- (bytes, peak RSS) status: {analysis['non_dominance']['bytes_peak_rss']['status']}")
    if dense_byte_bar:
        md.append(f"- dense byte bar (NQ-6): best reference point in the dense "
                  f"grid is {dense_byte_bar['reference']} at "
                  f"{dense_byte_bar['portfolio_bytes']:,} B "
                  f"(candidate is {dense_byte_bar['candidate_minus_reference_bytes']:+,} B)")
    md.append("")
    (args.out.parent / "dense-front.md").write_text("\n".join(md) + "\n",
                                                     encoding="utf-8")
    print("\n".join(md))


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(
        description="I20 dense reference-grid control (candidate C6)")
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("prepare")
    p.add_argument("--corpus", choices=("silesia", "enwik8"), required=True)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--brotli-lw", type=Path)
    p.add_argument("--brotli-dense", type=Path)
    p.add_argument("--brotli-dense-distro", type=Path)
    p.add_argument("--xz", type=Path)
    p.add_argument("--xz-pinned", type=Path)
    p.add_argument("--zstd", type=Path)
    p.add_argument("--lz4", type=Path)
    p.add_argument("--anvil-producer", default="")
    p.add_argument("--brotli-gate-producer", default="")
    p.add_argument("--brotli-pinned-producer", default="")
    p.add_argument("--brotli-distro-producer", default="")
    p.add_argument("--xz-producer", default="")
    p.add_argument("--xz-pinned-producer", default="")
    p.add_argument("--zstd-producer", default="")
    p.add_argument("--lz4-producer", default="")
    p.add_argument("--expected-candidate-total", type=int)
    p.add_argument("--only", default="")
    p.add_argument("--candidate-extra-args", nargs="*", default=[],
                   help="Extra ANVIL encoder-policy flags appended to the "
                        "candidate arm. Same convention as "
                        "tools/reference_cost_gate.py. Empty by default.")
    p.set_defaults(func=cmd_prepare)

    p = sub.add_parser("rss")
    p.add_argument("--manifest", type=Path, action="append")
    p.add_argument("--prepare", type=Path)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_rss)

    p = sub.add_parser("survey")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--reps", type=int, default=5)
    p.add_argument("--warmups", type=int, default=1)
    p.add_argument("--cpu", type=int, default=0)
    p.add_argument("--only", default="")
    p.set_defaults(func=cmd_survey)

    p = sub.add_parser("plan-paired")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--survey", type=Path, required=True)
    p.add_argument("--rss", type=Path)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_plan_paired)

    p = sub.add_parser("summarize")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--survey", type=Path, required=True)
    p.add_argument("--rss", type=Path)
    p.add_argument("--paired", action="append",
                   help="name=path to a paired_bench.py summary JSON")
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_summarize)
    return ap


def _known_option_strings(parser: argparse.ArgumentParser) -> set[str]:
    """Every option string this CLI understands, across all subcommands."""
    found: set[str] = set()
    stack: list[argparse.ArgumentParser] = [parser]
    while stack:
        current = stack.pop()
        for action in current._actions:  # noqa: SLF001 - argparse has no public API
            found.update(action.option_strings)
            choices = getattr(action, "choices", None)
            if isinstance(choices, dict):
                for sub in choices.values():
                    if isinstance(sub, argparse.ArgumentParser):
                        stack.append(sub)
    return found


def _preprocess_candidate_args(argv: list[str], known: set[str]) -> list[str]:
    """Make --candidate-extra-args accept ANVIL's native flag syntax.

    ANVIL's own CLI takes flags in the ``--flag=value`` form, and argparse
    refuses to bind a value that *looks* like an option to a ``nargs="*"``
    argument.  This rewrite collects every token that follows
    ``--candidate-extra-args`` up to the next known option of this CLI and
    hands them over as a single ``--candidate-extra-args=...`` value, so all of
    these work identically::

        --candidate-extra-args --bwt-subblock=4194304
        --candidate-extra-args "--bwt-subblock=4194304"
        --candidate-extra-args=--bwt-subblock=4194304
    """
    out: list[str] = []
    i = 0
    while i < len(argv):
        token = argv[i]
        if token == "--candidate-extra-args":
            values: list[str] = []
            j = i + 1
            while j < len(argv) and argv[j] not in known and argv[j] != "--":
                values.append(argv[j])
                j += 1
            out.append("--candidate-extra-args=" + " ".join(values))
            i = j
            continue
        out.append(token)
        i += 1
    return out


def main() -> None:
    parser = build_parser()
    known = _known_option_strings(parser)
    args = parser.parse_args(_preprocess_candidate_args(sys.argv[1:], known))
    for name in ("anvil", "brotli_lw", "brotli_dense", "brotli_dense_distro",
                 "xz", "xz_pinned", "zstd", "lz4"):
        value = getattr(args, name, None)
        if isinstance(value, Path):
            setattr(args, name, value.resolve())
    args.func(args)


if __name__ == "__main__":
    main()
