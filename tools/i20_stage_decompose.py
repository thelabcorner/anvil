#!/usr/bin/env python3
"""I20 / C1 - ANVIL BWT decode stage decomposition (CI-only, source-free).

This tool attributes the *whole-codec* decode cost of the ANVIL ratio route
(BWT backend, auxiliary-index v2 payload) into four pre-registered stages:

  (a) postcoder entropy decode      (adaptive arithmetic coder, postcoder 1/2)
  (b) MTF + zero-run RLE expansion
  (c) inverse BWT                    (libsais LF walk / checkpoint jumps)
  (d) residual                       (framing, allocation, CRC-32, file I/O)

It never modifies ``src/anvil.cpp``.  Stage attribution is done on the
*unmodified* source with three tools:

  * ``valgrind --tool=callgrind``  -> deterministic instruction counts (Ir)
  * ``gprof``                      -> sampled wall-clock self time
  * ``nm``/``c++filt``             -> exact symbol names (quoted in the output)

Three binaries are profiled, all built from the *same* unmodified source:

  anvil         the frozen reference build (Release, exactly as the
                reference-cost workflow builds it) - used for bytes, timing,
                RSS and the primary callgrind run.  The inverse-BWT code
                lives in a separate translation unit (``libsais.c``) and is
                therefore never inlined into ``anvil.cpp``, so the stage (c)
                share measured here is exact.
  anvil-attr    Release + ``-g -fno-inline`` - attribution build.  It changes
                no semantics (the compressed bytes are byte-identical, which
                the job asserts) and only stops the compiler from fusing the
                entropy decoder, the MTF expansion and the std::vector writes
                into one function, so stages (a) and (b) become separable.
  anvil-prof    Release + ``-g -pg`` - gprof build for wall-clock sampling.

Subcommands
-----------
  prepare   encode the canonical arm and the ``--bwt-aux=off`` control arm,
            verify byte-exact round trips, parse the mode-17 container.
  rss       per-process peak decode RSS per arm.
  time      per-file paired/interleaved timing (delegates to tools/paired_bench.py).
  profile   callgrind / gprof stage decomposition per BWT-routed file.
  summarize Amdahl arithmetic + the machine-readable summary.

Every number produced here is a CI artifact.  Nothing in this file is allowed
to produce a timing or byte number on a developer workstation.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import re
import shutil
import statistics
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path
from typing import Any

# --------------------------------------------------------------------------
# constants
# --------------------------------------------------------------------------

SILESIA_NAMES = [
    "dickens", "mozilla", "mr", "nci", "ooffice", "osdb",
    "reymont", "samba", "sao", "webster", "x-ray", "xml",
]

# Pre-registered baseline of record (prereg 1.1 / 1.2).  These are the totals
# the canonical arm MUST reproduce; a mismatch is a hard finding.
EXPECTED_CANDIDATE_TOTAL = {"silesia": 46_466_339, "enwik8": 23_537_422}
EXPECTED_XZ_TOTAL = {"silesia": 48_456_004, "enwik8": 24_831_648}
EXPECTED_BROTLI_TOTAL = {"silesia": 49_383_136, "enwik8": 24_810_180}

# prereg 1.1 baseline ruling numbers (run 35927623136) used for the
# "reachable / not reachable" arithmetic.  These are quoted, not measured.
BASELINE = {
    "silesia": {
        "decode_ratio_brotli_point": 3.4474,
        "decode_ratio_brotli_ci_hi": 3.5059,
        "decode_ratio_xz_point": 1.7226,
        "decode_ratio_xz_ci_hi": 1.8400,
        "rss_ratio_xz": 4.572,
        "rss_ratio_brotli": 1.995,
    },
    "enwik8": {
        "decode_ratio_brotli_point": 5.6642,
        "decode_ratio_brotli_ci_hi": 6.7596,
        "decode_ratio_xz_point": 3.9065,
        "decode_ratio_xz_ci_hi": 4.0088,
        "rss_ratio_xz": 9.043,
        "rss_ratio_brotli": 2.378,
    },
}

# Stage labels.  These strings are the only stage vocabulary used downstream.
STAGE_A = "a_postcoder_entropy"
STAGE_B = "b_mtf_rle"
STAGE_C = "c_inverse_bwt_lf_walk"
STAGE_D = "d_residual_framing_alloc_crc_io"
STAGE_AB = "ab_postcoder_stage_fused"   # Release build: entropy+MTF inlined together
STAGES = (STAGE_A, STAGE_B, STAGE_C, STAGE_D, STAGE_AB)


# --------------------------------------------------------------------------
# small helpers
# --------------------------------------------------------------------------

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def run(argv: list[str], *, stdout_path: Path | None = None,
        stderr_path: Path | None = None, cwd: Path | None = None,
        env: dict[str, str] | None = None, check: bool = True) -> int:
    out: Any = subprocess.DEVNULL
    err: Any = subprocess.PIPE
    fh_out = fh_err = None
    try:
        if stdout_path is not None:
            stdout_path.parent.mkdir(parents=True, exist_ok=True)
            fh_out = stdout_path.open("wb")
            out = fh_out
        if stderr_path is not None:
            stderr_path.parent.mkdir(parents=True, exist_ok=True)
            fh_err = stderr_path.open("wb")
            err = fh_err
        p = subprocess.run(argv, stdout=out, stderr=err, cwd=str(cwd) if cwd else None,
                           env=env, check=False)
        if check and p.returncode:
            raise RuntimeError(f"command failed rc={p.returncode}: {argv!r}")
        return p.returncode
    finally:
        if fh_out is not None:
            fh_out.close()
        if fh_err is not None:
            fh_err.close()


def canonical_files(corpus: str, root: Path) -> list[Path]:
    if corpus == "enwik8":
        p = root / "enwik8" / "enwik8"
        if not p.is_file():
            raise RuntimeError(f"missing canonical enwik8: {p}")
        return [p]
    if corpus == "silesia":
        d = root / "silesia"
        files = [d / n for n in SILESIA_NAMES]
        missing = [str(p) for p in files if not p.is_file()]
        if missing:
            raise RuntimeError(f"missing canonical Silesia files: {missing}")
        return files
    raise RuntimeError(f"unsupported corpus: {corpus}")


def read_uvar(buf: bytes, pos: int) -> tuple[int, int]:
    x = 0
    shift = 0
    for _ in range(10):
        if pos >= len(buf):
            raise ValueError("truncated uvarint")
        b = buf[pos]
        pos += 1
        x |= (b & 0x7F) << shift
        if not (b & 0x80):
            return x, pos
        shift += 7
    raise ValueError("uvarint overflow")


# --------------------------------------------------------------------------
# ANVIL mode-17 container parsing (revision 2 ratio envelope)
# --------------------------------------------------------------------------

def parse_bwt_bare(p: bytes) -> dict[str, Any]:
    """Parse one bare BWT backend payload (legacy v1 or auxiliary v2)."""
    if not p:
        raise ValueError("empty BWT payload")
    if p[0] == 0xFE:  # kBwtAuxTag - auxiliary-index v2 payload
        post = p[1]
        rate, off = read_uvar(p, 2)
        count, off = read_uvar(p, off)
        return {
            "aux": True,
            "postcoder_id": post,
            "aux_rate": rate,
            "aux_count": count,
            "aux_bytes": 4 * count,
        }
    post = p[0]
    primary, off = read_uvar(p, 1)
    return {"aux": False, "postcoder_id": post, "primary_index": primary}


def parse_bwt_payload(p: bytes) -> dict[str, Any]:
    if p and p[0] == 0xFF:  # BWT subblock framing
        nsub, off = read_uvar(p, 1)
        subs = []
        for _ in range(nsub):
            dlen, off = read_uvar(p, off)
            plen, off = read_uvar(p, off)
            subs.append({"decoded_len": dlen, "payload_len": plen,
                          **parse_bwt_bare(p[off:off + plen])})
            off += plen
        return {"framing": "subblock", "subblock_count": nsub, "subblocks": subs}
    return {"framing": "bare", **parse_bwt_bare(p)}


def parse_ratio_payload(p: bytes) -> dict[str, Any]:
    if len(p) < 3:
        raise ValueError("truncated ratio payload")
    transform = p[0]
    backend = p[1]
    xlen, off = read_uvar(p, 2)
    rec: dict[str, Any] = {
        "transform_id": transform,
        "backend_id": backend,
        "transformed_size": xlen,
        "backend_name": {1: "brotli", 2: "bwt"}.get(backend, f"backend{backend}"),
    }
    if backend == 2:
        rec["bwt"] = parse_bwt_payload(p[off:])
    return rec


def parse_container(data: bytes) -> dict[str, Any]:
    if data[:4] != b"ANV0":
        raise ValueError("not an ANVIL container")
    revision = data[4]
    pos = 5
    block_size, pos = read_uvar(data, pos)
    total, pos = read_uvar(data, pos)
    blocks: list[dict[str, Any]] = []
    while pos < len(data):
        blen, pos = read_uvar(data, pos)
        mode = data[pos]
        pos += 1
        plen, pos = read_uvar(data, pos)
        crc = int.from_bytes(data[pos:pos + 4], "little")
        pos += 4
        payload = data[pos:pos + plen]
        pos += plen
        rec: dict[str, Any] = {"decoded_len": blen, "mode": mode,
                               "payload_len": plen, "crc32": crc}
        if mode == 17:
            rec["ratio"] = parse_ratio_payload(payload)
        blocks.append(rec)
    return {"revision": revision, "block_size": block_size,
            "declared_total": total, "blocks": blocks}


def routing_of(container: dict[str, Any]) -> list[str]:
    """Return the distinct backend names used by the mode-17 blocks."""
    names: list[str] = []
    for b in container["blocks"]:
        if b.get("mode") == 17:
            n = b["ratio"]["backend_name"]
            if n not in names:
                names.append(n)
    return names


# --------------------------------------------------------------------------
# prepare
# --------------------------------------------------------------------------

def cmd_prepare(args: argparse.Namespace) -> None:
    root = args.root.resolve()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    anvil = args.anvil.resolve()
    brotli = args.brotli.resolve()
    xz = shutil.which("xz")
    if not anvil.is_file() or not brotli.is_file() or not xz:
        raise RuntimeError("missing ANVIL, brotli_lw, or xz")

    files = canonical_files(args.corpus, root)
    packed = out / "packed"
    decoded = out / "decoded"
    for d in (packed, decoded):
        d.mkdir(parents=True, exist_ok=True)

    canon_flags = ["--parse=ratio", "--ratio-backend=auto",
                   "--ratio-context=off", "--ratio-lines=off",
                   "--bwt-aux=on", "--quiet"]
    noaux_flags = ["--parse=ratio", "--ratio-backend=auto",
                   "--ratio-context=off", "--ratio-lines=off",
                   "--bwt-aux=off", "--quiet"]

    rows: list[dict[str, Any]] = []
    per_file: list[dict[str, Any]] = []

    for src in files:
        name = src.name
        n = src.stat().st_size
        digest = sha256(src)
        rec: dict[str, Any] = {
            "name": name,
            "input_bytes": n,
            "source_sha256": digest,
            "arms": {},
        }

        # ---- canonical arm (--bwt-aux=on) -------------------------------
        cand = packed / f"{name}.anv"
        run([str(anvil), "c", str(src), str(cand), *canon_flags])
        canon_digests = [sha256(cand)]
        for i in range(max(0, args.encode_reps - 1)):
            alt = packed / f"{name}.anv.rep{i}"
            run([str(anvil), "c", str(src), str(alt), *canon_flags])
            canon_digests.append(sha256(alt))
        canon_out = decoded / f"{name}.anvil"
        canon_cmd = [str(anvil), "d", str(cand), str(canon_out), "--quiet"]
        run(canon_cmd)
        ok = canon_out.is_file() and canon_out.stat().st_size == n and sha256(canon_out) == digest
        if not ok:
            raise RuntimeError(f"round-trip mismatch (anvil canonical): {name}")
        container = parse_container(cand.read_bytes())
        rec["arms"]["anvil"] = {
            "packed": str(cand), "cmd": canon_cmd, "observe": str(canon_out),
            "bytes": cand.stat().st_size, "sha256": canon_digests[0],
            "encode_digests": canon_digests,
            "encode_deterministic": len(set(canon_digests)) == 1,
            "roundtrip": "OK", "container": container,
            "routing": routing_of(container),
        }

        # ---- control arm (--bwt-aux=off) --------------------------------
        noaux = packed / f"{name}.noaux.anv"
        run([str(anvil), "c", str(src), str(noaux), *noaux_flags])
        noaux_digests = [sha256(noaux)]
        for i in range(max(0, args.encode_reps - 1)):
            alt = packed / f"{name}.noaux.anv.rep{i}"
            run([str(anvil), "c", str(src), str(alt), *noaux_flags])
            noaux_digests.append(sha256(alt))
        noaux_out = decoded / f"{name}.noaux"
        noaux_cmd = [str(anvil), "d", str(noaux), str(noaux_out), "--quiet"]
        run(noaux_cmd)
        ok = (noaux_out.is_file() and noaux_out.stat().st_size == n
              and sha256(noaux_out) == digest)
        if not ok:
            raise RuntimeError(f"round-trip mismatch (anvil noaux): {name}")
        ncontainer = parse_container(noaux.read_bytes())
        rec["arms"]["anvil-noaux"] = {
            "packed": str(noaux), "cmd": noaux_cmd, "observe": str(noaux_out),
            "bytes": noaux.stat().st_size, "sha256": noaux_digests[0],
            "encode_digests": noaux_digests,
            "encode_deterministic": len(set(noaux_digests)) == 1,
            "roundtrip": "OK", "container": ncontainer,
            "routing": routing_of(ncontainer),
        }

        # ---- brotli reference ------------------------------------------
        br = packed / f"{name}.br"
        run([str(brotli), "c", str(src), str(br)])
        br_out = decoded / f"{name}.brout"
        br_cmd = [str(brotli), "d", str(br), str(br_out), str(n)]
        run(br_cmd)
        ok = br_out.is_file() and br_out.stat().st_size == n and sha256(br_out) == digest
        if not ok:
            raise RuntimeError(f"round-trip mismatch (brotli): {name}")
        rec["arms"]["brotli"] = {
            "packed": str(br), "cmd": br_cmd, "observe": str(br_out),
            "bytes": br.stat().st_size, "sha256": sha256(br),
            "roundtrip": "OK",
        }

        # ---- xz reference ----------------------------------------------
        xp = packed / f"{name}.xz"
        run([str(xz), "-9e", "-c", str(src)], stdout_path=xp)
        xz_out = xp.with_suffix("")
        xz_cmd = [str(xz), "-d", "-k", "-f", str(xp)]
        run(xz_cmd)
        ok = xz_out.is_file() and xz_out.stat().st_size == n and sha256(xz_out) == digest
        if not ok:
            raise RuntimeError(f"round-trip mismatch (xz): {name}")
        rec["arms"]["xz"] = {
            "packed": str(xp), "cmd": xz_cmd, "observe": str(xz_out),
            "bytes": xp.stat().st_size, "sha256": sha256(xp),
            "roundtrip": "OK",
        }

        for arm, a in rec["arms"].items():
            rows.append({"corpus": args.corpus, "file": name,
                         "input_bytes": n, "source_sha256": digest,
                         "codec": arm, "compressed_bytes": a["bytes"],
                         "ratio": f"{a['bytes'] / n:.9f}"})
        per_file.append(rec)
        print(f"prepared {name}: anvil={rec['arms']['anvil']['bytes']} "
              f"noaux={rec['arms']['anvil-noaux']['bytes']} "
              f"brotli={rec['arms']['brotli']['bytes']} "
              f"xz={rec['arms']['xz']['bytes']} "
              f"routing={rec['arms']['anvil']['routing']}", flush=True)

    fields = ["corpus", "file", "input_bytes", "source_sha256", "codec",
              "compressed_bytes", "ratio"]
    with (out / "bytes.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)

    totals: dict[str, int] = defaultdict(int)
    for r in rows:
        totals[r["codec"]] += int(r["compressed_bytes"])
    totals = dict(totals)

    expected = EXPECTED_CANDIDATE_TOTAL.get(args.corpus)
    matches = (expected is None) or (totals["anvil"] == expected)
    if expected is not None and not matches:
        raise RuntimeError(
            f"candidate byte identity failed: {totals['anvil']} != {expected}")

    bwt_files = [r["name"] for r in per_file
                 if "bwt" in r["arms"]["anvil"]["routing"]]
    brotli_files = [r["name"] for r in per_file
                    if "brotli" in r["arms"]["anvil"]["routing"]]

    summary = {
        "schema": 1,
        "corpus": args.corpus,
        "source_bytes": sum(f.stat().st_size for f in files),
        "file_count": len(files),
        "compressed_bytes": totals,
        "expected_candidate_total": expected,
        "expected_xz_total": EXPECTED_XZ_TOTAL.get(args.corpus),
        "expected_brotli_total": EXPECTED_BROTLI_TOTAL.get(args.corpus),
        "candidate_total_matches_expected": matches,
        "xz_total_matches_expected": (
            EXPECTED_XZ_TOTAL.get(args.corpus) is None
            or totals["xz"] == EXPECTED_XZ_TOTAL[args.corpus]),
        "brotli_total_matches_expected": (
            EXPECTED_BROTLI_TOTAL.get(args.corpus) is None
            or totals["brotli"] == EXPECTED_BROTLI_TOTAL[args.corpus]),
        "canonical_flags": canon_flags,
        "control_flags": noaux_flags,
        "routing": {"bwt_files": bwt_files, "brotli_files": brotli_files},
        "files": per_file,
        "roundtrip": "OK",
    }
    (out / "prepare-summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in summary.items() if k != "files"},
                     indent=2, sort_keys=True))


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
        raise RuntimeError(f"RSS command failed rc={p.returncode}: {cmd!r}\n"
                           + p.stderr.decode(errors="replace")[-2000:])
    return int(rss_file.read_text(encoding="utf-8").strip())


def cmd_rss(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    rows: list[dict[str, Any]] = []
    with tempfile.TemporaryDirectory(prefix="anvil-rss-") as td0:
        tmp = Path(td0)
        for frec in prep["files"]:
            for arm, arec in frec["arms"].items():
                kib = measure_one_rss(list(arec["cmd"]), tmp)
                rows.append({"corpus": prep["corpus"], "file": frec["name"],
                             "arm": arm, "max_rss_kib": kib,
                             "input_bytes": frec["input_bytes"],
                             "compressed_bytes": arec["bytes"]})
                print(f"rss {frec['name']:10s} {arm:12s} {kib:>10,} KiB", flush=True)
    fields = ["corpus", "file", "arm", "max_rss_kib", "input_bytes", "compressed_bytes"]
    with (out / "rss.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)
    peak: dict[str, int] = {}
    for r in rows:
        arm = r["arm"]
        peak[arm] = max(peak.get(arm, 0), int(r["max_rss_kib"]))
    result = {"schema": 1, "corpus": prep["corpus"], "peak_rss_kib": peak,
              "samples": rows}
    (out / "rss.json").write_text(json.dumps(result, indent=2, sort_keys=True) + "\n",
                                  encoding="utf-8")
    print(json.dumps(peak, indent=2, sort_keys=True))


# --------------------------------------------------------------------------
# timing (per-file paired/interleaved, delegates to tools/paired_bench.py)
# --------------------------------------------------------------------------

def cmd_time(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    driver = Path(args.driver).resolve()
    if not driver.is_file():
        raise RuntimeError(f"missing timing driver: {driver}")

    comparisons = [("brotli", "brotli"), ("xz", "xz"), ("null", None)]
    results: list[dict[str, Any]] = []
    for frec in prep["files"]:
        name = frec["name"]
        cand = list(frec["arms"]["anvil"]["cmd"])
        cand_obs = frec["arms"]["anvil"]["observe"]
        for label, ctrl_arm in comparisons:
            if ctrl_arm is None:
                control = cand
                ctrl_obs = cand_obs
            else:
                control = list(frec["arms"][ctrl_arm]["cmd"])
                ctrl_obs = frec["arms"][ctrl_arm]["observe"]
            tag = f"{name}-{label}"
            csv_path = out / f"paired-{tag}.csv"
            json_path = out / f"paired-{tag}.json"
            cmd = [
                sys.executable, str(driver),
                "--control-json", json.dumps(control),
                "--candidate-json", json.dumps(cand),
                "--reps", str(args.reps),
                "--warmups", str(args.warmups),
                "--seed", "41246",
                "--bootstrap", "20000",
                "--epsilon", "0.02",
                "--max-cv", "0.15",
                "--cpu", str(args.cpu),
                "--ambient-reps", "5",
                "--ambient-iterations", "250000",
                "--ambient-max-cv", "0.10",
                "--out", str(csv_path),
                "--summary", str(json_path),
                "--control-observe", ctrl_obs,
                "--candidate-observe", cand_obs,
                "--expected-sha256", frec["source_sha256"],
            ]
            print(f"paired {tag}", flush=True)
            run(cmd)
            s = json.loads(json_path.read_text(encoding="utf-8"))
            results.append({
                "file": name,
                "comparison": label,
                "status": s["status"],
                "timing_valid": s["timing_valid"],
                "clears_speed_gate": s["clears_speed_gate"],
                "ratio": s["paired_ratio_candidate_over_control"],
                "ci95": s["paired_ratio_ci95"],
                "anvil_median_s": s["candidate"]["median_s"],
                "anvil_robust_cv": s["candidate"]["robust_cv"],
                "control_median_s": s["control"]["median_s"],
                "control_robust_cv": s["control"]["robust_cv"],
                "ambient_blocked": s["ambient"]["blocked"],
                "ambient_robust_cv": s["ambient"]["robust_cv"],
                "summary_path": str(json_path),
                "csv_path": str(csv_path),
            })
    payload = {"schema": 1, "corpus": prep["corpus"], "results": results}
    (out / "timing.json").write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n",
                                     encoding="utf-8")
    print(f"wrote {out / 'timing.json'}")


# --------------------------------------------------------------------------
# callgrind parsing
# --------------------------------------------------------------------------

class CallgrindData:
    def __init__(self) -> None:
        self.total = 0
        self.self_by_fn: dict[str, int] = defaultdict(int)
        self.self_by_fn_line: dict[tuple[str, str, int], int] = defaultdict(int)
        self.arcs: dict[tuple[str, str], int] = defaultdict(int)
        self.arc_sites: dict[tuple[str, str], dict[tuple[str, int], int]] = defaultdict(
            lambda: defaultdict(int))
        self.events: list[str] = []
        self.summary_total = 0


_RECORD_RE = re.compile(r"^(?P<key>[a-z]+)=(?:\((?P<id>[^)]*)\))?\s*(?P<rest>.*)$")
_SKIP_PREFIX = ("version:", "creator:", "pid:", "cmd:", "part:", "desc:",
                "totals:", "positions:", "jump=", "jcnd=")


def parse_callgrind(path: Path) -> CallgrindData:
    """Parse a callgrind.out file into per-function self costs and call arcs."""
    data = CallgrindData()
    fn_names: dict[str, str] = {}
    cfn_names: dict[str, str] = {}
    file_names: dict[str, str] = {}

    cur_fn: str | None = None
    cur_file: str | None = None
    file_stack: list[str] = []
    pending_cfn: str | None = None
    pending_call = False
    last_line = 0
    total = 0
    summary_total = 0

    for raw in path.read_text(errors="replace").splitlines():
        line = raw.strip()
        if not line:
            continue
        if line.startswith("events:"):
            data.events = line.split(":", 1)[1].split()
            continue
        if line.startswith("summary:"):
            try:
                summary_total = int(line.split(":", 1)[1].strip())
            except ValueError:
                pass
            continue
        if line.startswith(_SKIP_PREFIX):
            continue

        m = _RECORD_RE.match(line)
        if m and m.group("key") in ("fn", "cfn", "fl", "fi", "fe", "cfi", "ob", "cob"):
            key = m.group("key")
            ident = m.group("id")
            rest = m.group("rest").strip()
            if key == "fn":
                if ident is not None and rest:
                    fn_names[ident] = rest
                    name = rest
                elif ident is not None:
                    name = fn_names.get(ident, ident)
                else:
                    name = fn_names.get(rest, rest)
                cur_fn = name
                continue
            if key == "cfn":
                if ident is not None and rest:
                    cfn_names[ident] = rest
                continue
            if key in ("fl", "fi", "fe"):
                if ident is not None and rest:
                    file_names[ident] = rest
                    name = rest
                elif ident is not None:
                    name = file_names.get(ident, ident)
                else:
                    name = file_names.get(rest, rest)
                if key == "fe":
                    if file_stack:
                        file_stack.pop()
                    cur_file = file_stack[-1] if file_stack else None
                else:
                    file_stack.append(name)
                    cur_file = name
                continue
            if key == "cfi":
                if ident is not None and rest:
                    file_names[ident] = rest
                continue
            continue  # ob / cob

        if line.startswith("calls="):
            parts = line.split("=", 1)[1].split()
            target = parts[1] if len(parts) > 1 else ""
            pending_cfn = cfn_names.get(target, target)
            pending_call = True
            continue

        # ---- cost line: <position> <cost> ... ----
        parts = line.split()
        if len(parts) < 2:
            continue
        pos = parts[0]
        if pos == "*":
            lineno = last_line
        elif pos.startswith("+") and pos[1:].isdigit():
            lineno = last_line + int(pos[1:])
        elif pos.startswith("-") and pos[1:].isdigit():
            lineno = last_line - int(pos[1:])
        elif pos.isdigit():
            lineno = int(pos)
        else:
            continue
        last_line = lineno
        try:
            ir = int(parts[1])
        except ValueError:
            continue

        if pending_call:
            pending_call = False
            if cur_fn is not None and pending_cfn is not None:
                data.arcs[(cur_fn, pending_cfn)] += ir
                data.arc_sites[(cur_fn, pending_cfn)][(cur_file or "?", lineno)] += ir
            continue

        if cur_fn is None:
            continue
        data.self_by_fn[cur_fn] += ir
        data.self_by_fn_line[(cur_fn, cur_file or "?", lineno)] += ir
        total += ir

    data.total = total
    data.summary_total = summary_total
    return data


def demangle(names: list[str]) -> dict[str, str]:
    if not names:
        return {}
    filt = shutil.which("c++filt")
    if not filt:
        return {n: n for n in names}
    try:
        p = subprocess.run([filt], input="\n".join(names), capture_output=True,
                           text=True, check=False)
        if p.returncode != 0:
            return {n: n for n in names}
        out = p.stdout.splitlines()
        if len(out) != len(names):
            return {n: n for n in names}
        return dict(zip(names, out))
    except Exception:
        return {n: n for n in names}


# --------------------------------------------------------------------------
# stage classification
# --------------------------------------------------------------------------

_CONTAINER_PAT = re.compile(
    r"std::vector<|operator new|operator delete|_M_realloc_insert|_M_default_append"
    r"|::push_back|::emplace_back|::reserve\b|::resize|::insert<|memcpy|memmove"
    r"|std::allocator|::allocate|::deallocate|__cxa_|_Unwind|std::__cxx11"
    r"|::_M_erase|::_M_fill|::_M_range|::fill\b|std::swap|::_S_")

_ENTROPY_PAT = re.compile(
    r"AdaptiveDecModel|ArithmeticDecoder|BitReader|decode_uvar|AdaptiveModel")

_MTF_PAT = re.compile(r"bwt_mtf_emit_rank|bwt_mtf_expand")


def classify_name(name: str) -> str | None:
    """Stage of a function by symbol name, or None for container/allocator code."""
    if "libsais" in name:
        return STAGE_C
    if _ENTROPY_PAT.search(name):
        return STAGE_A
    if _MTF_PAT.search(name):
        return STAGE_B
    if "bwt_arith_decode" in name:
        return STAGE_AB
    if _CONTAINER_PAT.search(name):
        return None
    return STAGE_D


def find_func_range(lines: list[str], marker: str) -> tuple[int, int] | None:
    """1-based inclusive body line range of the function containing `marker`."""
    start = None
    for i, ln in enumerate(lines):
        if marker in ln:
            start = i
            break
    if start is None:
        return None
    depth = 0
    seen = False
    for j in range(start, len(lines)):
        for ch in lines[j]:
            if ch == "{":
                depth += 1
                seen = True
            elif ch == "}":
                depth -= 1
        if seen and depth == 0:
            return (start + 1, j + 1)
    return None


def build_source_map(src: Path) -> dict[str, Any]:
    """Locate the anvil.cpp source lines that belong to each postcoder stage."""
    text = src.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    arith_range = find_func_range(lines, "static std::vector<uint8_t> bwt_arith_decode(")
    mtf_range = find_func_range(lines, "static void bwt_mtf_emit_rank(")
    exp_range = find_func_range(lines, "static std::vector<uint8_t> bwt_mtf_expand(")
    arith_lines: list[int] = []
    mtf_lines: list[int] = []
    other_lines: list[int] = []
    if arith_range:
        for i in range(arith_range[0], arith_range[1] + 1):
            s = lines[i - 1]
            if "decode(ad)" in s or "decode_uvar_m" in s:
                arith_lines.append(i)
            elif ("bwt_mtf_emit_rank" in s or "out.insert" in s
                  or "out.push_back" in s or "sym[j]=sym[j-1]" in s
                  or "sym[0]=b" in s):
                mtf_lines.append(i)
            else:
                other_lines.append(i)
    return {
        "bwt_arith_decode_range": arith_range,
        "bwt_mtf_emit_rank_range": mtf_range,
        "bwt_mtf_expand_range": exp_range,
        "arith_lines": arith_lines,
        "mtf_lines": mtf_lines,
        "other_postcoder_lines": other_lines,
        "n_lines": len(lines),
    }


def stage_of_call_site(fname: str, ffile: str, lineno: int,
                       smap: dict[str, Any]) -> str:
    """Stage of a call site, using the fused postcoder function's line map."""
    base = os.path.basename(ffile or "")
    if base == "anvil.cpp" and lineno in set(smap["arith_lines"]):
        return STAGE_A
    if base == "anvil.cpp" and lineno in set(smap["mtf_lines"]):
        return STAGE_B
    named = classify_name(fname)
    if named is not None:
        return named
    return STAGE_AB


def summarise_callgrind(data: CallgrindData, smap: dict[str, Any]) -> dict[str, Any]:
    """Turn a parsed callgrind file into per-stage self-cost shares."""
    all_names = sorted(set(list(data.self_by_fn.keys())
                           + [c for (_a, c) in data.arcs.keys()]))
    dem = demangle(all_names)

    def d(n: str) -> str:
        return dem.get(n, n)

    # incoming arc costs per callee
    incoming: dict[str, int] = defaultdict(int)
    for (_caller, callee), v in data.arcs.items():
        incoming[callee] += v

    # stage of a caller function
    def caller_stage(caller: str, ffile: str, lineno: int) -> str:
        return stage_of_call_site(caller, ffile, lineno, smap)

    stage_self: dict[str, int] = defaultdict(int)
    fn_table: list[dict[str, Any]] = []
    for fn, cost in sorted(data.self_by_fn.items(), key=lambda kv: -kv[1]):
        name = d(fn)
        named = classify_name(name)
        if named is None:
            # container / allocator / memcpy: distribute the self cost over the
            # call sites that reached it, in proportion to the call-arc cost.
            tot = incoming.get(fn, 0)
            if not tot:
                stage_self[STAGE_D] += cost
                fn_table.append({"symbol": name, "self_ir": cost,
                                 "stage": STAGE_D, "distributed": []})
                continue
            dist: list[dict[str, Any]] = []
            for (caller, callee), arc in sorted(data.arcs.items(), key=lambda kv: -kv[1]):
                if callee != fn:
                    continue
                share = cost * (arc / tot)
                for (ffile, lineno), site_cost in data.arc_sites[(caller, callee)].items():
                    frac = site_cost / arc if arc else 0.0
                    st = caller_stage(caller, ffile, lineno)
                    stage_self[st] += share * frac
                    dist.append({"caller": d(caller), "caller_file": ffile,
                                 "caller_line": lineno, "stage": st,
                                 "ir": share * frac})
            fn_table.append({"symbol": name, "self_ir": cost,
                             "stage": "distributed", "distributed": dist[:40]})
            continue
        if named == STAGE_AB:
            # The Release build fuses the entropy decoder, the MTF expansion
            # and the std::vector writes into one function.  Split that
            # function's own self cost by the source line it was charged to.
            sub: dict[str, int] = defaultdict(int)
            for (f2, ffile, line), v in data.self_by_fn_line.items():
                if f2 != fn:
                    continue
                sub[stage_of_call_site(f2, ffile, line, smap)] += v
            for st, v in sub.items():
                stage_self[st] += v
            fn_table.append({"symbol": name, "self_ir": cost,
                             "stage": STAGE_AB, "stage_split_ir": dict(sub)})
            continue
        stage_self[named] += cost
        fn_table.append({"symbol": name, "self_ir": cost, "stage": named})

    total = sum(data.self_by_fn.values())
    a_plus_b = stage_self[STAGE_A] + stage_self[STAGE_B]
    postcoder = a_plus_b + stage_self[STAGE_AB]
    shares = {}
    for st in (STAGE_A, STAGE_B, STAGE_C, STAGE_D, STAGE_AB):
        shares[st] = (stage_self[st] / total) if total else 0.0
    return {
        "total_ir": total,
        "stage_ir": {st: stage_self[st]
                     for st in (STAGE_A, STAGE_B, STAGE_C, STAGE_D, STAGE_AB)},
        "stage_share": shares,
        "postcoder_stage_ir": postcoder,
        "postcoder_stage_share": (postcoder / total) if total else 0.0,
        "lf_walk_share": (stage_self[STAGE_C] / total) if total else 0.0,
        "functions": fn_table[:400],
    }


def run_gprof(binp: Path, packed: str, out: Path, gp_dir: Path, name: str,
              reps: int) -> dict[str, Any]:
    """Sample the decode with gprof and merge the flat profiles."""
    gprof_bin = shutil.which("gprof")
    if not gprof_bin:
        return {"available": False, "reason": "gprof binary not found"}
    merged: dict[str, float] = defaultdict(float)
    calls: dict[str, int] = defaultdict(int)
    for i in range(max(1, reps)):
        wd = gp_dir / f"{name}-r{i}"
        wd.mkdir(parents=True, exist_ok=True)
        dec = out / "decoded" / f"{name}.gprof.dec"
        dec.parent.mkdir(parents=True, exist_ok=True)
        rc = run([str(binp), "d", str(packed), str(dec), "--quiet"],
                 cwd=wd, stdout_path=wd / "stdout.log",
                 stderr_path=wd / "stderr.log", check=False)
        gmon = wd / "gmon.out"
        if rc != 0 or not gmon.is_file():
            continue
        pf = gp_dir / f"{name}-r{i}.flat.txt"
        run([gprof_bin, "-b", "--flat-profile", str(binp), str(gmon)],
            stdout_path=pf, stderr_path=pf, check=False)
        if not pf.is_file():
            continue
        for fn, sec, ncalls in parse_gprof_flat(pf):
            merged[fn] += sec
            calls[fn] += ncalls
    if not merged:
        return {"available": False, "reason": "no gmon.out produced"}
    total = sum(merged.values())
    rows = []
    for fn, sec in sorted(merged.items(), key=lambda kv: -kv[1]):
        rows.append({"symbol": fn, "self_seconds": sec,
                     "self_share": (sec / total) if total else 0.0,
                     "calls": calls[fn]})
    lf = sum(v for k, v in merged.items() if "libsais" in k)
    post = sum(v for k, v in merged.items() if _ENTROPY_PAT.search(k) or _MTF_PAT.search(k))
    fused = sum(v for k, v in merged.items() if "bwt_arith_decode" in k)
    return {
        "available": True,
        "reps": reps,
        "total_self_seconds": total,
        "lf_walk_self_seconds": lf,
        "lf_walk_share": (lf / total) if total else 0.0,
        "postcoder_stage_self_seconds": post + fused,
        "postcoder_stage_share": ((post + fused) / total) if total else 0.0,
        "functions": rows[:200],
    }


def parse_gprof_flat(path: Path) -> list[tuple[str, float, int]]:
    out: list[tuple[str, float, int]] = []
    started = False
    for raw in path.read_text(errors="replace").splitlines():
        if raw.startswith("Flat profile"):
            started = True
            continue
        if not started:
            continue
        if raw.startswith("Call graph"):
            break
        parts = raw.split(None, 6)
        if len(parts) < 7:
            continue
        if not parts[0].replace(".", "", 1).isdigit():
            continue
        try:
            self_s = float(parts[2])
            ncalls = int(parts[3])
        except ValueError:
            continue
        name = parts[6].strip()
        if name:
            out.append((name, self_s, ncalls))
    return out


# --------------------------------------------------------------------------
# profile (callgrind + gprof)
# --------------------------------------------------------------------------

def cmd_profile(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    smap = build_source_map(Path(args.source))

    cg_dir = out / "callgrind"
    cg_dir.mkdir(parents=True, exist_ok=True)
    gp_dir = out / "gprof"
    gp_dir.mkdir(parents=True, exist_ok=True)

    targets = [f for f in prep["files"] if "bwt" in f["arms"]["anvil"]["routing"]]
    if not targets:
        print("no BWT-routed files in this corpus; nothing to decompose")
        (out / "profile.json").write_text(
            json.dumps({"schema": 1, "corpus": prep["corpus"], "bwt_files": [],
                        "source_line_map": smap,
                        "note": "no BWT-routed file in this corpus"},
                       indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return

    results: list[dict[str, Any]] = []
    for frec in targets:
        name = frec["name"]
        rec: dict[str, Any] = {"file": name,
                               "input_bytes": frec["input_bytes"],
                               "source_sha256": frec["source_sha256"]}
        for arm, binpath in (("anvil", args.anvil),
                              ("anvil-attr", args.anvil_attr),
                              ("anvil-noaux", args.anvil)):
            if not binpath:
                continue
            binp = Path(binpath)
            if not binp.is_file():
                continue
            packed = (frec["arms"]["anvil-noaux"]["packed"] if arm == "anvil-noaux"
                      else frec["arms"]["anvil"]["packed"])
            outfile = cg_dir / f"{name}-{arm}.out"
            outbin = out / "decoded" / f"{name}.{arm}.dec"
            outbin.parent.mkdir(parents=True, exist_ok=True)
            vg = ["valgrind", "--tool=callgrind",
                  "--callgrind-out-file=" + str(outfile),
                  "--dump-line=yes", "--dump-instr=no", "--collect-jumps=no",
                  str(binp), "d", str(packed), str(outbin), "--quiet"]
            log = cg_dir / f"{name}-{arm}.valgrind.log"
            print(f"callgrind {name} [{arm}]", flush=True)
            rc = run(vg, stdout_path=log, stderr_path=log, check=False)
            if rc != 0 or not outfile.is_file():
                rec.setdefault("errors", []).append(
                    {"arm": arm, "stage": "callgrind", "rc": rc})
                continue
            parsed = parse_callgrind(outfile)
            rec[arm] = summarise_callgrind(parsed, smap)
            rec[arm]["callgrind_out"] = str(outfile)
            rec[arm]["valgrind_log"] = str(log)
        if args.anvil_prof:
            rec["gprof"] = run_gprof(Path(args.anvil_prof),
                                     frec["arms"]["anvil"]["packed"],
                                     out, gp_dir, name, args.gprof_reps)
        results.append(rec)

    payload = {
        "schema": 1,
        "corpus": prep["corpus"],
        "bwt_files": [f["name"] for f in targets],
        "source_line_map": smap,
        "results": results,
    }
    (out / "profile.json").write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    ok = any(any(k in r for k in ("anvil", "anvil-attr", "anvil-noaux"))
             for r in results)
    if not ok:
        raise RuntimeError("no callgrind profile could be produced for any BWT-routed file")
    print(f"wrote {out / 'profile.json'}")


# --------------------------------------------------------------------------
# summarize
# --------------------------------------------------------------------------

def cmd_summarize(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    rss = json.loads(args.rss.read_text(encoding="utf-8"))
    timing = json.loads(args.timing.read_text(encoding="utf-8"))
    profile: dict[str, Any] = {}
    if args.profile and Path(args.profile).is_file():
        profile = json.loads(Path(args.profile).read_text(encoding="utf-8"))

    corpus = prep["corpus"]
    base = BASELINE[corpus]

    rss_by: dict[tuple[str, str], int] = {}
    for s in rss["samples"]:
        rss_by[(s["file"], s["arm"])] = int(s["max_rss_kib"])

    t_by: dict[tuple[str, str], dict[str, Any]] = {}
    for r in timing["results"]:
        t_by[(r["file"], r["comparison"])] = r

    prof_by = {r["file"]: r for r in profile.get("results", [])}

    rows: list[dict[str, Any]] = []
    for frec in prep["files"]:
        name = frec["name"]
        arms = frec["arms"]
        row: dict[str, Any] = {
            "file": name,
            "input_bytes": frec["input_bytes"],
            "source_sha256": frec["source_sha256"],
            "routing": arms["anvil"]["routing"],
            "bytes": {k: arms[k]["bytes"] for k in arms},
            "roundtrip": {k: arms[k]["roundtrip"] for k in arms},
            "encode_deterministic": {k: arms[k].get("encode_deterministic")
                                     for k in arms},
            "peak_rss_kib": {k: rss_by.get((name, k)) for k in arms},
        }
        container = arms["anvil"]["container"]
        row["container"] = {
            "revision": container["revision"],
            "block_size": container["block_size"],
            "block_count": len(container["blocks"]),
            "blocks": [{k: v for k, v in b.items() if k != "crc32"}
                       for b in container["blocks"]],
        }
        for cmp_ in ("brotli", "xz", "null"):
            t = t_by.get((name, cmp_))
            if t:
                row[f"timing_{cmp_}"] = {
                    "status": t["status"],
                    "timing_valid": t["timing_valid"],
                    "ratio": t["ratio"],
                    "ci95": t["ci95"],
                    "anvil_median_s": t["anvil_median_s"],
                    "control_median_s": t["control_median_s"],
                    "ambient_blocked": t["ambient_blocked"],
                }
        p = prof_by.get(name)
        if p:
            for key in ("anvil", "anvil-attr", "anvil-noaux"):
                if key in p:
                    q = p[key]
                    row[f"profile_{key}"] = {
                        "total_ir": q["total_ir"],
                        "stage_ir": q["stage_ir"],
                        "stage_share": q["stage_share"],
                        "postcoder_stage_share": q["postcoder_stage_share"],
                        "lf_walk_share": q["lf_walk_share"],
                    }
            if "gprof" in p:
                g = p["gprof"]
                row["profile_gprof"] = {k: v for k, v in g.items() if k != "functions"}
        rows.append(row)

    # ---------------- portfolio aggregation ----------------------------
    t_anvil = 0.0
    t_brotli = 0.0
    t_xz = 0.0
    t_anvil_xz = 0.0
    valid_files = 0
    for frec in prep["files"]:
        nm = frec["name"]
        tb = t_by.get((nm, "brotli"))
        tx = t_by.get((nm, "xz"))
        if tb:
            t_anvil += float(tb["anvil_median_s"])
            t_brotli += float(tb["control_median_s"])
        if tx:
            t_anvil_xz += float(tx["anvil_median_s"])
            t_xz += float(tx["control_median_s"])
        if tb and tb["timing_valid"]:
            valid_files += 1

    bwt_names = prep["routing"]["bwt_files"]
    t_bwt = 0.0
    for nm in bwt_names:
        t = t_by.get((nm, "brotli"))
        if t:
            t_bwt += float(t["anvil_median_s"])
    phi = (t_bwt / t_anvil) if t_anvil else 0.0

    # LF-walk share, aggregated over the BWT-routed files
    lf_ir: list[tuple[float, str]] = []
    lf_gp: list[tuple[float, str]] = []
    per_file_lf: dict[str, float] = {}
    per_file_lf_gprof: dict[str, float] = {}
    for nm in bwt_names:
        p = prof_by.get(nm)
        if not p:
            continue
        if "anvil" in p:
            lf_ir.append((p["anvil"]["lf_walk_share"], nm))
            per_file_lf[nm] = p["anvil"]["lf_walk_share"]
        g = p.get("gprof")
        if g and g.get("available"):
            lf_gp.append((g["lf_walk_share"], nm))
            per_file_lf_gprof[nm] = g["lf_walk_share"]

    def time_weighted(pairs: list[tuple[float, str]]) -> float | None:
        if not pairs:
            return None
        num = 0.0
        den = 0.0
        for share, nm in pairs:
            t = t_by.get((nm, "brotli"))
            w = float(t["anvil_median_s"]) if t else 0.0
            num += share * w
            den += w
        return (num / den) if den else None

    f_ir = time_weighted(lf_ir)
    f_gp = time_weighted(lf_gp)

    # ---------------- Amdahl arithmetic --------------------------------
    amdahl: dict[str, Any] = {
        "t_anvil_portfolio_s": t_anvil,
        "t_anvil_portfolio_s_xz_pair": t_anvil_xz,
        "t_brotli_portfolio_s": t_brotli,
        "t_xz_portfolio_s": t_xz,
        "files_with_valid_timing": valid_files,
        "bwt_files": bwt_names,
        "t_bwt_routed_s": t_bwt,
        "phi_bwt_share_of_anvil_decode_time": phi,
        "f_lf_walk_share_instruction_counts": f_ir,
        "f_lf_walk_share_wall_clock_gprof": f_gp,
        "per_file_lf_walk_share_instruction_counts": per_file_lf,
        "per_file_lf_walk_share_wall_clock_gprof": per_file_lf_gprof,
        "baseline": base,
    }
    for label, f in (("instruction_counts", f_ir), ("wall_clock_gprof", f_gp)):
        if f is None or not t_anvil:
            amdahl[f"amdahl_{label}"] = None
            continue
        saved = t_bwt * f
        t_new = t_anvil - saved
        speedup = (t_anvil / t_new) if t_new > 0 else float("inf")
        entry: dict[str, Any] = {
            "f": f,
            "t_saved_s": saved,
            "t_anvil_after_s": t_new,
            "max_portfolio_speedup": speedup,
            "per_file_max_speedup": 1.0 / (1.0 - f) if f < 1 else float("inf"),
        }
        if t_brotli:
            entry["projected_ratio_vs_brotli"] = t_new / t_brotli
        if t_xz:
            entry["projected_ratio_vs_xz"] = t_new / t_xz
        amdahl[f"amdahl_{label}"] = entry

    # reachable? compare projected point ratio against the required CI-upper
    reach: dict[str, Any] = {}
    for ref, key in (("brotli", "decode_ratio_brotli"), ("xz", "decode_ratio_xz")):
        b = base[key]
        ci_hi, point = b["ci_hi"], b["point"]
        factor = ci_hi / point
        need_point = 2.0 / factor
        need_speedup = point / need_point
        reach[ref] = {
            "baseline_point": point,
            "baseline_ci_hi": ci_hi,
            "ci_over_point_factor": factor,
            "required_point_ratio_for_ci_hi_le_2": need_point,
            "required_speedup": need_speedup,
        }
        for label in ("instruction_counts", "wall_clock_gprof"):
            e = amdahl.get(f"amdahl_{label}")
            if not e:
                continue
            pr = e.get(f"projected_ratio_vs_{ref}")
            if pr is None:
                continue
            reach[ref][label] = {
                "projected_point_ratio": pr,
                "projected_ci_hi_using_baseline_factor": pr * factor,
                "clears_gate": (pr * factor) <= 2.0,
            }
    amdahl["reachability"] = reach

    # ---------------- memory attribution (source-level) ----------------
    mem: list[dict[str, Any]] = []
    for frec in prep["files"]:
        n = frec["input_bytes"]
        a = frec["arms"]["anvil"]
        container = a["container"]
        blk = container["blocks"][0] if container["blocks"] else {}
        xlen = blk.get("ratio", {}).get("transformed_size", n)
        bwt = blk.get("ratio", {}).get("bwt", {})
        aux = bwt.get("aux_count") if bwt.get("aux") else 0
        pred = {
            "bwt_string_bytes": xlen,
            "inverse_bwt_A_array_bytes": 4 * (xlen + 1),
            "inverse_bwt_U_output_bytes": xlen,
            "container_output_buffer_bytes": n,
            "compressed_input_bytes": a["bytes"],
            "aux_index_bytes": 4 * aux,
            "predicted_peak_live_bytes": 4 * (xlen + 1) + 3 * xlen + a["bytes"] + 4 * aux,
        }
        mem.append({"file": frec["name"], "input_bytes": n,
                    "measured_peak_rss_kib": rss_by.get((frec["name"], "anvil")),
                    "measured_peak_rss_xz_kib": rss_by.get((frec["name"], "xz")),
                    "measured_peak_rss_brotli_kib": rss_by.get((frec["name"], "brotli")),
                    "prediction": pred})
    amdahl["memory_attribution"] = mem

    result = {
        "schema": 1,
        "corpus": corpus,
        "compressed_bytes": prep["compressed_bytes"],
        "expected_candidate_total": prep["expected_candidate_total"],
        "candidate_total_matches_expected": prep["candidate_total_matches_expected"],
        "xz_total_matches_expected": prep["xz_total_matches_expected"],
        "brotli_total_matches_expected": prep["brotli_total_matches_expected"],
        "routing": prep["routing"],
        "peak_rss_kib": rss["peak_rss_kib"],
        "files": rows,
        "amdahl": amdahl,
    }
    (out / "summary.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    (out / "summary.md").write_text(render_markdown(result), encoding="utf-8")
    print(render_markdown(result))


def render_markdown(result: dict[str, Any]) -> str:
    a = result["amdahl"]
    L: list[str] = []
    L.append(f"# I20 C1 stage decomposition - {result['corpus']}")
    L.append("")
    L.append("| item | value |")
    L.append("|---|---:|")
    for k in ("compressed_bytes", "expected_candidate_total",
              "candidate_total_matches_expected", "xz_total_matches_expected",
              "brotli_total_matches_expected"):
        L.append(f"| {k} | {result.get(k)} |")
    L.append(f"| peak_rss_kib | {result['peak_rss_kib']} |")
    L.append(f"| bwt_files | {result['routing']['bwt_files']} |")
    L.append("")
    L.append("## per-file")
    L.append("")
    L.append("| file | bytes anvil | routing | t_anvil s | t_brotli s | t_xz s | "
             "rss anvil KiB | rss xz KiB | LF share Ir | postcoder share Ir |")
    L.append("|---|---:|---|---:|---:|---:|---:|---:|---:|---:|")
    for r in result["files"]:
        tb = r.get("timing_brotli", {})
        tx = r.get("timing_xz", {})
        pa = r.get("profile_anvil", {})
        paa = r.get("profile_anvil-attr", {})
        L.append(
            f"| {r['file']} | {r['bytes']['anvil']:,} | {','.join(r['routing'])} | "
            f"{tb.get('anvil_median_s', float('nan')):.4f} | "
            f"{tb.get('control_median_s', float('nan')):.4f} | "
            f"{tx.get('control_median_s', float('nan')):.4f} | "
            f"{r['peak_rss_kib']['anvil']} | {r['peak_rss_kib']['xz']} | "
            f"{pa.get('lf_walk_share', float('nan')):.4f} | "
            f"{paa.get('postcoder_stage_share', float('nan')):.4f} |")
    L.append("")
    L.append("## amdahl")
    L.append("")
    L.append("```json")
    L.append(json.dumps(a, indent=2, sort_keys=True))
    L.append("```")
    L.append("")
    return "\n".join(L)


# --------------------------------------------------------------------------
# cli
# --------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("prepare")
    p.add_argument("--corpus", choices=("silesia", "enwik8"), required=True)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--brotli", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--encode-reps", type=int, default=3)
    p.set_defaults(func=cmd_prepare)

    p = sub.add_parser("rss")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_rss)

    p = sub.add_parser("time")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--driver", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--reps", type=int, default=9)
    p.add_argument("--warmups", type=int, default=2)
    p.add_argument("--cpu", type=int, default=0)
    p.set_defaults(func=cmd_time)

    p = sub.add_parser("profile")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--source", type=Path, required=True)
    p.add_argument("--anvil", type=Path, default="")
    p.add_argument("--anvil-attr", type=Path, default="")
    p.add_argument("--anvil-prof", type=Path, default="")
    p.add_argument("--gprof-reps", type=int, default=3)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_profile)

    p = sub.add_parser("summarize")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--rss", type=Path, required=True)
    p.add_argument("--timing", type=Path, required=True)
    p.add_argument("--profile", type=Path, default="")
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_summarize)
    return ap


def main() -> None:
    args = build_parser().parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
