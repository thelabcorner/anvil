#!/usr/bin/env python3
"""Paired/interleaved timing driver for ANVIL remote experiments.

This is intentionally generic: each arm is an argv JSON array. Both arms run
inside the same process/job/VM, warm symmetrically, alternate AB/BA by a seeded
schedule, retain every raw repetition, and bootstrap the paired log-time ratio.

The driver never drops timing outliers. A noisy ambient probe blocks timing
interpretation instead of deleting inconvenient samples.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import os
import random
import statistics
import subprocess
import sys
import time
from pathlib import Path
from typing import Any


def parse_argv(text: str, name: str) -> list[str]:
    try:
        value = json.loads(text)
    except json.JSONDecodeError as exc:
        raise SystemExit(f"{name}: invalid JSON argv: {exc}") from exc
    if not isinstance(value, list) or not value or not all(isinstance(x, str) for x in value):
        raise SystemExit(f"{name}: expected a non-empty JSON array of strings")
    return value


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def median_abs_deviation(values: list[float]) -> float:
    med = statistics.median(values)
    return statistics.median(abs(x - med) for x in values)


def robust_cv(values: list[float]) -> float:
    med = statistics.median(values)
    if med <= 0:
        return math.inf
    # 1.4826 makes MAD consistent with sigma for a normal distribution.
    return 1.4826 * median_abs_deviation(values) / med


def ambient_probe(iterations: int, reps: int) -> dict[str, Any]:
    """Small single-core integer probe used only to detect gross host jitter."""
    samples: list[float] = []
    checksum = 0
    for rep in range(reps):
        x = 0x9E3779B97F4A7C15 ^ rep
        t0 = time.perf_counter()
        for i in range(iterations):
            x ^= (x << 13) & 0xFFFFFFFFFFFFFFFF
            x ^= x >> 7
            x ^= (x << 17) & 0xFFFFFFFFFFFFFFFF
            x = (x + i + 0xD1B54A32D192ED03) & 0xFFFFFFFFFFFFFFFF
        samples.append(time.perf_counter() - t0)
        checksum ^= x
    return {
        "seconds": samples,
        "median_s": statistics.median(samples),
        "robust_cv": robust_cv(samples),
        "checksum": checksum,
    }


def set_affinity(cpu: int | None) -> list[int] | None:
    if cpu is None:
        return None
    if hasattr(os, "sched_setaffinity"):
        os.sched_setaffinity(0, {cpu})
        return sorted(os.sched_getaffinity(0))
    return None


def run_once(
    argv: list[str],
    timeout_s: float,
    env: dict[str, str],
    stdout_path: Path | None,
    stderr_path: Path | None,
) -> float:
    stdout = subprocess.DEVNULL
    stderr = subprocess.PIPE
    out_handle = err_handle = None
    try:
        if stdout_path is not None:
            stdout_path.parent.mkdir(parents=True, exist_ok=True)
            out_handle = stdout_path.open("ab")
            stdout = out_handle
        if stderr_path is not None:
            stderr_path.parent.mkdir(parents=True, exist_ok=True)
            err_handle = stderr_path.open("ab")
            stderr = err_handle

        t0 = time.perf_counter()
        p = subprocess.run(
            argv,
            stdout=stdout,
            stderr=stderr,
            env=env,
            timeout=timeout_s,
            check=False,
        )
        elapsed = time.perf_counter() - t0
        if p.returncode != 0:
            detail = ""
            if stderr == subprocess.PIPE and p.stderr:
                detail = p.stderr.decode(errors="replace")[-4000:]
            raise RuntimeError(
                f"command failed rc={p.returncode}: {argv!r}"
                + (f"\n{detail}" if detail else "")
            )
        return elapsed
    finally:
        if out_handle is not None:
            out_handle.close()
        if err_handle is not None:
            err_handle.close()


def post_run_observation(
    path: Path | None,
    expected_sha256: str | None,
) -> dict[str, Any]:
    if path is None:
        return {}
    if not path.exists():
        raise RuntimeError(f"observation path does not exist after command: {path}")
    digest = sha256(path)
    if expected_sha256 and digest.lower() != expected_sha256.lower():
        raise RuntimeError(
            f"hash mismatch for {path}: {digest} != expected {expected_sha256}"
        )
    return {
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": digest,
    }


def bootstrap_paired_ratio(
    log_ratios: list[float],
    seed: int,
    samples: int,
) -> tuple[float, float, float]:
    rng = random.Random(seed)
    n = len(log_ratios)
    observed = math.exp(statistics.median(log_ratios))
    boots: list[float] = []
    for _ in range(samples):
        sample = [log_ratios[rng.randrange(n)] for _ in range(n)]
        boots.append(math.exp(statistics.median(sample)))
    boots.sort()
    lo_i = max(0, int(0.025 * samples))
    hi_i = min(samples - 1, int(0.975 * samples))
    return observed, boots[lo_i], boots[hi_i]


def main() -> None:
    ap = argparse.ArgumentParser(
        description="same-job paired/interleaved A/B timing with raw repetitions"
    )
    ap.add_argument("--control-json", required=True, help="control argv as JSON string array")
    ap.add_argument("--candidate-json", required=True, help="candidate argv as JSON string array")
    ap.add_argument("--reps", type=int, default=9)
    ap.add_argument("--warmups", type=int, default=2)
    ap.add_argument("--seed", type=int, default=41246)
    ap.add_argument("--bootstrap", type=int, default=20_000)
    ap.add_argument("--epsilon", type=float, default=0.02, help="practical speedup threshold")
    ap.add_argument("--max-cv", type=float, default=0.15)
    ap.add_argument("--timeout-s", type=float, default=1800.0)
    ap.add_argument("--cpu", type=int)
    ap.add_argument("--ambient-reps", type=int, default=5)
    ap.add_argument("--ambient-iterations", type=int, default=250_000)
    ap.add_argument("--ambient-max-cv", type=float, default=0.10)
    ap.add_argument("--control-observe", type=Path)
    ap.add_argument("--candidate-observe", type=Path)
    ap.add_argument("--expected-sha256")
    ap.add_argument("--out", type=Path, required=True, help="raw repetition CSV")
    ap.add_argument("--summary", type=Path, required=True, help="summary JSON")
    ap.add_argument("--stdout-dir", type=Path)
    ap.add_argument("--env-json", default="{}", help="environment overrides as JSON object")
    args = ap.parse_args()

    if args.reps < 7:
        raise SystemExit("--reps must be >= 7 for scout-grade paired timing")
    if args.warmups < 1:
        raise SystemExit("--warmups must be >= 1")
    if args.bootstrap < 1000:
        raise SystemExit("--bootstrap must be >= 1000")
    if not 0 <= args.epsilon < 1:
        raise SystemExit("--epsilon must be in [0,1)")
    if args.ambient_reps < 3:
        raise SystemExit("--ambient-reps must be >= 3")

    control = parse_argv(args.control_json, "--control-json")
    candidate = parse_argv(args.candidate_json, "--candidate-json")

    try:
        env_delta = json.loads(args.env_json)
    except json.JSONDecodeError as exc:
        raise SystemExit(f"--env-json invalid JSON: {exc}") from exc
    if not isinstance(env_delta, dict) or not all(
        isinstance(k, str) and isinstance(v, str) for k, v in env_delta.items()
    ):
        raise SystemExit("--env-json must be a JSON object of string:string values")
    env = os.environ.copy()
    env.update(env_delta)

    affinity = set_affinity(args.cpu)
    ambient = ambient_probe(args.ambient_iterations, args.ambient_reps)
    blocked_ambient = ambient["robust_cv"] > args.ambient_max_cv

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.summary.parent.mkdir(parents=True, exist_ok=True)
    if args.stdout_dir:
        args.stdout_dir.mkdir(parents=True, exist_ok=True)

    def log_paths(arm: str) -> tuple[Path | None, Path | None]:
        if not args.stdout_dir:
            return None, None
        return (
            args.stdout_dir / f"{arm}.stdout.log",
            args.stdout_dir / f"{arm}.stderr.log",
        )

    # Symmetric warmup. Alternate which arm warms first.
    for w in range(args.warmups):
        order = ("control", "candidate") if w % 2 == 0 else ("candidate", "control")
        for arm in order:
            argv = control if arm == "control" else candidate
            so, se = log_paths(arm)
            run_once(argv, args.timeout_s, env, so, se)

    rng = random.Random(args.seed)
    pair_orders = []
    for _ in range(args.reps):
        pair_orders.append(("control", "candidate") if rng.randrange(2) == 0 else ("candidate", "control"))

    raw: list[dict[str, Any]] = []
    pair_times: list[dict[str, float]] = []
    observations: dict[str, list[dict[str, Any]]] = {"control": [], "candidate": []}

    for pair_index, order in enumerate(pair_orders):
        times: dict[str, float] = {}
        for position, arm in enumerate(order):
            argv = control if arm == "control" else candidate
            observe = args.control_observe if arm == "control" else args.candidate_observe
            so, se = log_paths(arm)
            elapsed = run_once(argv, args.timeout_s, env, so, se)
            obs = post_run_observation(observe, args.expected_sha256)
            observations[arm].append(obs)
            times[arm] = elapsed
            raw.append(
                {
                    "pair": pair_index,
                    "position": position,
                    "order": "->".join(order),
                    "arm": arm,
                    "seconds": f"{elapsed:.9f}",
                    "observed_bytes": obs.get("bytes", ""),
                    "observed_sha256": obs.get("sha256", ""),
                }
            )
        pair_times.append(times)

    with args.out.open("w", newline="", encoding="utf-8") as f:
        fields = [
            "pair",
            "position",
            "order",
            "arm",
            "seconds",
            "observed_bytes",
            "observed_sha256",
        ]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(raw)

    control_times = [p["control"] for p in pair_times]
    candidate_times = [p["candidate"] for p in pair_times]
    log_ratios = [math.log(p["candidate"] / p["control"]) for p in pair_times]
    ratio, ci_lo, ci_hi = bootstrap_paired_ratio(
        log_ratios,
        seed=args.seed ^ 0xA5A5A5A5,
        samples=args.bootstrap,
    )

    control_cv = robust_cv(control_times)
    candidate_cv = robust_cv(candidate_times)
    timing_valid = (
        not blocked_ambient
        and control_cv <= args.max_cv
        and candidate_cv <= args.max_cv
    )
    clears_speed_gate = timing_valid and ci_hi < (1.0 - args.epsilon)

    obs_summary: dict[str, Any] = {}
    for arm, entries in observations.items():
        nonempty = [e for e in entries if e]
        if not nonempty:
            obs_summary[arm] = None
            continue
        sizes = sorted({int(e["bytes"]) for e in nonempty})
        hashes = sorted({str(e["sha256"]) for e in nonempty})
        if len(sizes) != 1 or len(hashes) != 1:
            raise RuntimeError(
                f"{arm}: observed output is not deterministic: sizes={sizes}, hashes={hashes}"
            )
        obs_summary[arm] = {
            "bytes": sizes[0],
            "sha256": hashes[0],
            "deterministic": True,
        }

    summary = {
        "schema": 1,
        "status": (
            "BLOCKED_AMBIENT"
            if blocked_ambient
            else "BLOCKED_NOISE"
            if not timing_valid
            else "PASS_SPEED_GATE"
            if clears_speed_gate
            else "NO_SPEED_GATE"
        ),
        "timing_valid": timing_valid,
        "clears_speed_gate": clears_speed_gate,
        "practical_epsilon": args.epsilon,
        "paired_ratio_candidate_over_control": ratio,
        "paired_ratio_ci95": [ci_lo, ci_hi],
        "candidate_speedup_point_estimate": (1.0 / ratio if ratio > 0 else math.inf),
        "control": {
            "argv": control,
            "median_s": statistics.median(control_times),
            "mad_s": median_abs_deviation(control_times),
            "robust_cv": control_cv,
        },
        "candidate": {
            "argv": candidate,
            "median_s": statistics.median(candidate_times),
            "mad_s": median_abs_deviation(candidate_times),
            "robust_cv": candidate_cv,
        },
        "pairs": [
            {
                "pair": i,
                "order": "->".join(pair_orders[i]),
                "control_s": pair_times[i]["control"],
                "candidate_s": pair_times[i]["candidate"],
                "log_candidate_over_control": log_ratios[i],
            }
            for i in range(args.reps)
        ],
        "ambient": {
            **ambient,
            "max_cv": args.ambient_max_cv,
            "blocked": blocked_ambient,
        },
        "affinity": affinity,
        "seed": args.seed,
        "warmups": args.warmups,
        "reps": args.reps,
        "bootstrap_samples": args.bootstrap,
        "environment_overrides": env_delta,
        "observed_outputs": obs_summary,
        "python": sys.version,
    }
    args.summary.write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    print(
        f"status={summary['status']} "
        f"ratio={ratio:.6f} ci95=[{ci_lo:.6f},{ci_hi:.6f}] "
        f"speedup={summary['candidate_speedup_point_estimate']:.4f}x "
        f"cv={control_cv:.4f}/{candidate_cv:.4f} "
        f"ambient_cv={ambient['robust_cv']:.4f}"
    )

    # Blocked/no-gate is a scientific verdict, not a script/infrastructure error.
    # Only command/correctness failures raise nonzero before this point.


if __name__ == "__main__":
    main()
