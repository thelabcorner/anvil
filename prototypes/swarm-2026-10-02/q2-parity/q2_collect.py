#!/usr/bin/env python3
"""Q2 collector: complete bytes + round-trip hashes for every arm, plus descriptive rows.

Isolated prototype. Not wired to production. NOT DISPATCHABLE and not executed by this
repository's contributors: heavy execution is GitHub Actions only (MASTER-BRIEF §7).

By default this script DOES NOTHING BUT PRINT THE PLAN. Every codec invocation requires an
explicit --execute, and --execute additionally refuses to run unless the environment
variable that could alter arm identity is absent.

Emitted per (arm, file):
  complete_bytes, compressed_sha256, roundtrip_sha256, roundtrip_verified (byte-compare,
  never exit code), payload_sum_bytes + framing_bytes for split reference arms, plus
  DESCRIPTIVE-ONLY in-process MB/s, router counters, peak RSS and segment-parallelism
  fields. Nothing descriptive feeds any contrast.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import q2_arms as A  # noqa: E402

RSS_TOOL = "/usr/bin/time"
_ANVIL_STATS = re.compile(
    r"ANVIL c .*?ratio=([0-9.eE+-]+) MB/s=([0-9.eE+-]+) blocks=(\d+) "
    r"compressed=(\d+) raw=(\d+)"
)


class Q2Error(RuntimeError):
    pass


def sha256_file(path: Path) -> str:
    import hashlib
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def child_env() -> dict:
    """Identity-determinant environment: ANVIL_STREAM_LAMBDA is removed, not inherited."""
    env = {k: v for k, v in os.environ.items() if k not in A.ENV_VARS_MUST_BE_UNSET}
    env.setdefault("LC_ALL", "C")
    return env


def preflight(binary: Path, helper: Path) -> None:
    problems = A.check_arg_strings()
    problems += A.check_env_identity()
    if not binary.is_file():
        problems.append(f"anvil binary not found: {binary}")
    if not helper.is_file():
        problems.append(f"reference helper not found: {helper}")
    if not Path(RSS_TOOL).exists():
        problems.append(f"{RSS_TOOL} not available; peak RSS is mandatory for scope matching")
    if problems:
        raise Q2Error("preflight failed:\n  " + "\n  ".join(problems))


def run(argv: list[str], env: dict, cwd: Path | None = None) -> subprocess.CompletedProcess:
    proc = subprocess.run(argv, capture_output=True, text=True, env=env, cwd=cwd)
    if proc.returncode != 0:
        raise Q2Error(f"command failed ({proc.returncode}): {' '.join(argv)}\n{proc.stderr}")
    return proc


def with_rss(argv: list[str], env: dict, rss_path: Path) -> tuple[str, str]:
    """Run under /usr/bin/time -v so peak RSS is measured by ONE identical mechanism for
    every arm (scope matching, Q0 rule 9). Returns (stdout, stderr)."""
    if not Path(RSS_TOOL).exists():
        proc = subprocess.run(argv, capture_output=True, text=True, env=env)
        return proc.stdout, proc.stderr
    wrapped = [RSS_TOOL, "-v", "-o", str(rss_path), *argv]
    proc = subprocess.run(wrapped, capture_output=True, text=True, env=env)
    if proc.returncode != 0:
        raise Q2Error(f"command failed ({proc.returncode}): {' '.join(argv)}\n{proc.stderr}")
    return proc.stdout, proc.stderr


def peak_rss_kib(rss_path: Path) -> int | None:
    if not rss_path.is_file():
        return None
    for line in rss_path.read_text(encoding="utf-8", errors="replace").splitlines():
        if "Maximum resident set size" in line:
            m = re.search(r":\s*(\d+)", line)
            if m:
                return int(m.group(1))
    return None


def parse_anvil_stats(stderr: str) -> dict:
    """In-process MB/s and the per-block router counters. src/anvil.cpp:5009,5011,5024.
    steady_clock wraps compress/decompress and write_file is OUTSIDE the timed region."""
    m = _ANVIL_STATS.search(stderr)
    out: dict = {"timing_grade": "descriptive-in-process",
                 "timing_classifying": False,
                 "router_counters_present": bool(m)}
    if m:
        out.update({
            "encode_ratio_reported": float(m.group(1)),
            "encode_MBps": float(m.group(2)),
            "blocks": int(m.group(3)),
            "compressed_blocks": int(m.group(4)),
            "raw_blocks": int(m.group(5)),
        })
    dm = re.search(r"ANVIL d out=(\d+) MB/s=([0-9.eE+-]+)", stderr)
    if dm:
        out["decode_output_bytes"] = int(dm.group(1))
        out["decode_MBps"] = float(dm.group(2))
    return out


def collect_candidate(arm: A.CandidateArm, src: Path, work: Path, env: dict,
                      binary: Path, cf: A.CorpusFile) -> dict:
    enc = work / f"{arm.cell_id}.{src.name}.anv"
    dec = work / f"{arm.cell_id}.{src.name}.out"
    rss_e, rss_d = work / f"{arm.cell_id}.{src.name}.enc.rss", work / f"{arm.cell_id}.{src.name}.dec.rss"
    argv = [str(binary), *arm.encode_argv(Path("SRC"), Path("OUT"))[1:]]
    argv = [str(binary), "c", str(src), str(enc), *arm.config_argv(),
            f"--block={arm.block_bytes}", f"--negate={arm.negate}",
            f"--decode-threads={A.PINNED_DECODE_THREADS}", "--bwt-aux=off"]
    _, enc_err = with_rss(argv, env, rss_e)

    dargv = [str(binary), "d", str(enc), str(dec),
             f"--decode-threads={A.PINNED_DECODE_THREADS}", "--bwt-aux=off"]
    _, dec_err = with_rss(dargv, env, rss_d)

    # Gate G-C: byte-compare, never exit code.
    same = src.is_file() and dec.is_file() and src.read_bytes() == dec.read_bytes()
    src_sha = sha256_file(src)
    dec_sha = sha256_file(dec) if dec.is_file() else None
    stats = parse_anvil_stats(enc_err + dec_err)
    row = {
        **arm.geometry(src.stat().st_size),
        "side": "candidate",
        "arm_id": arm.cell_id,
        "file": src.name,
        "source_bytes_manifest": cf.source_bytes,
        "source_bytes_actual": src.stat().st_size,
        "identity_role": cf.identity_role,
        "complete_bytes": enc.stat().st_size,
        "compressed_sha256": sha256_file(enc),
        "roundtrip_sha256": dec_sha,
        "roundtrip_source_sha256": src_sha,
        "roundtrip_verified": bool(same and dec_sha == src_sha),
        "argv_encode": argv,
        "argv_decode": dargv,
        "binary_bytes": binary.stat().st_size,
        "peak_rss_encode_kib": peak_rss_kib(rss_e),
        "peak_rss_decode_kib": peak_rss_kib(rss_d),
        "evidence_role": A.EVIDENCE_ROLE,
        **stats,
    }
    # Gate G-G: the router counters are a direct per-block measurement of the gate.
    row["router_counters_required"] = True
    if not stats.get("router_counters_present"):
        row["gate_failure"] = "G-G router counters absent (stderr not parsed)"
    if not row["roundtrip_verified"]:
        row["gate_failure"] = "G-C roundtrip byte-compare failed"
    enc.unlink(missing_ok=True)
    dec.unlink(missing_ok=True)
    return row


def collect_reference(arm: A.ReferenceArm, src: Path, work: Path, env: dict,
                      helper: Path, cf: A.CorpusFile) -> dict:
    enc = work / f"{arm.arm_id}.{src.name}.br"
    dec = work / f"{arm.arm_id}.{src.name}.out"
    rss_e, rss_d = work / f"{arm.arm_id}.{src.name}.enc.rss", work / f"{arm.arm_id}.{src.name}.dec.rss"
    streams = arm.streams_for(src.stat().st_size)

    # The helper writes NO payload side-file. Its stderr line
    #   payload_sum_bytes=<N> framing_overhead_bytes=<N>
    # is the single authoritative channel for payload sums, for both the whole-file and
    # the split path. See q2_brotli_geom.cpp usage.
    argv = [str(helper), "c", str(src), str(enc), f"--q=11",
            f"--lgwin={arm.brotli_lgwin}", f"--streams={streams}",
            f"--block={arm.block_bytes}"]
    _, enc_err = with_rss(argv, env, rss_e)

    dargv = [str(helper), "d", str(enc), str(dec), str(src.stat().st_size),
             f"--lgwin={arm.brotli_lgwin}"]
    with_rss(dargv, env, rss_d)

    same = src.is_file() and dec.is_file() and src.read_bytes() == dec.read_bytes()
    src_sha = sha256_file(src)
    dec_sha = sha256_file(dec) if dec.is_file() else None
    m = re.search(r"payload_sum_bytes=(\d+) framing_overhead_bytes=(\d+)", enc_err)
    payload_sum = int(m.group(1)) if m else None
    framing = int(m.group(2)) if m else None
    complete = enc.stat().st_size
    gate_failure = None
    if payload_sum is None:
        gate_failure = ("G-E helper stderr carried no payload_sum_bytes line; the "
                        "framing-free tax cannot be computed")
    elif framing is None or payload_sum + framing != complete:
        gate_failure = (f"G-E payload_sum_bytes+framing_overhead_bytes != complete_bytes "
                        f"({payload_sum}+{framing} != {complete})")
    return {
        **arm.geometry(src.stat().st_size),
        "side": "reference",
        "file": src.name,
        "source_bytes_manifest": cf.source_bytes,
        "source_bytes_actual": src.stat().st_size,
        "identity_role": cf.identity_role,
        "complete_bytes": complete,
        "compressed_sha256": sha256_file(enc),
        "roundtrip_sha256": dec_sha,
        "roundtrip_source_sha256": src_sha,
        "roundtrip_verified": bool(same and dec_sha == src_sha),
        "payload_sum_bytes": payload_sum,
        "framing_bytes": framing,
        "framing_policy": "declared separately from stderr payload_sum_bytes; our container "
                          "overhead is never charged to the codec (gate G-E)",
        "payload_sum_source": "helper stderr (single authoritative channel; no side-file)",
        "binary_bytes": helper.stat().st_size,
        "peak_rss_encode_kib": peak_rss_kib(rss_e),
        "peak_rss_decode_kib": peak_rss_kib(rss_d),
        "timing_grade": "descriptive-process-level",
        "timing_classifying": False,
        "comparability_note": "process-level MB/s is comparable to OTHER process-level values "
                              "only; never to ANVIL's in-process MB/s nor to the frozen CSV",
        "classifying": False,
        "evidence_role": A.EVIDENCE_ROLE,
        "argv_encode": argv,
        "argv_decode": dargv,
        **({"gate_failure": gate_failure} if gate_failure else {}),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description="Q2 collector (GitHub Actions only)")
    ap.add_argument("--corpus", type=Path, required=True)
    ap.add_argument("--frozen-suite", type=Path, required=True,
                    help="frozen Class-A suite CSV; the sole authority for the 13-file population. "
                         "CHECKSUMS.txt supplies identities but never the population.")
    ap.add_argument("--out", type=Path, required=True, help="collector JSONL")
    ap.add_argument("--manifest", type=Path, help="write the resolved arm manifest here")
    ap.add_argument("--anvil", default="build/anvil")
    ap.add_argument("--helper", default="q2_brotli_geom")
    ap.add_argument("--work", default="q2-work")
    ap.add_argument("--preflight-only", action="store_true",
                    help="execute ONLY the G0 cell on stratum-A tracked files, for the G-S "
                         "retained-twin preflight. Runs before any geometry arm so a drifted "
                         "substrate is caught before the expensive cells are spent.")
    ap.add_argument("--execute", action="store_true",
                    help="actually run the codecs. Without this flag nothing is executed.")
    args = ap.parse_args()

    corpus = A.load_corpus(args.corpus, args.frozen_suite)
    manifest = A.plan(corpus, helper=args.helper)
    manifest["population"] = A.population_report(args.frozen_suite, args.corpus)
    if args.manifest:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n",
                                 encoding="utf-8")

    print(f"configurations        : {len(A.CANDIDATE_CONFIGS)}")
    print(f"cells per config      : {len(A.CANDIDATE_CELLS)}")
    print(f"candidate cells       : {len(A.CANDIDATE_ARMS)}")
    print(f"reference arms        : {len(A.REFERENCE)}")
    pop = manifest["population"]
    print(f"population            : {pop['count']} files (from the frozen suite, NOT CHECKSUMS)")
    print(f"  tracked slots       : {len(pop['tracked_slots'])}")
    print(f"  build-output slots  : {pop['build_output_slots']}")
    print(f"  out of population   : {pop['checksums_entries_not_in_population']}")
    print(f"  population problems : {pop['problems'] or 'none'}")
    print(f"candidate rows        : {len(A.CANDIDATE_ARMS) * len(corpus)}")
    print(f"retained lambda span  : {sorted({c.stream_lambda for c in A.CANDIDATE_CONFIGS})}")
    print(f"env must be unset     : {', '.join(A.ENV_VARS_MUST_BE_UNSET)}")
    print(f"env violations        : {A.check_env_identity() or 'none'}")
    print(f"materiality threshold : {manifest['materiality_threshold']}")
    print(f"epsilon               : {manifest['epsilon']}")

    if not args.execute:
        print("\nPLAN ONLY. No codec was invoked. Pass --execute to run (GitHub Actions only).")
        return 0

    binary, helper = Path(args.anvil), Path(args.helper)
    work = Path(args.work)
    work.mkdir(parents=True, exist_ok=True)
    preflight(binary, helper)
    env = child_env()

    rows: list[dict] = []
    for cf in corpus:
        src = args.corpus / cf.name
        if not src.is_file():
            raise Q2Error(f"corpus file missing: {src}")
        actual = src.stat().st_size
        if cf.identity_role == "tracked" and actual != cf.source_bytes:
            raise Q2Error(f"{cf.name}: size {actual} != manifest {cf.source_bytes} (gate G-B)")
        if cf.identity_role == "tracked" and sha256_file(src) != cf.sha256:
            raise Q2Error(f"{cf.name}: SHA-256 mismatch against CHECKSUMS.txt (gate G-B)")
        for arm in A.CANDIDATE_ARMS:
            # G-S preflight stage: G0 only, stratum A only. Geometry arms are deliberately
            # excluded so a drifted substrate cannot consume them.
            if args.preflight_only and (arm.cell != "G0" or cf.identity_role != "tracked"):
                continue
            rows.append(collect_candidate(arm, src, work, env, binary, cf))
        if args.preflight_only:
            continue  # no reference arms during the preflight stage
        for arm in A.REFERENCE.values():
            rows.append(collect_reference(arm, src, work, env, helper, cf))

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", encoding="utf-8") as fh:
        for row in rows:
            fh.write(json.dumps(row, sort_keys=True) + "\n")
    failures = [r for r in rows if r.get("gate_failure") or not r.get("roundtrip_verified")]
    print(f"\nwrote {len(rows)} rows to {args.out}")
    print(f"gate failures         : {len(failures)}")
    for r in failures[:20]:
        print("  ", r.get("cell_id") or r.get("arm_id"), r.get("file"),
              r.get("gate_failure", "roundtrip"))
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
