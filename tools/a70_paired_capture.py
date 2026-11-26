#!/usr/bin/env python3
"""Durable A70 observations around the byte-frozen paired timing driver.

This adapter never replaces the timing clock, schedule, estimator, or scientific
verdict. It journals only outside the original timed interval. An incomplete
capture is diagnostic evidence, never a partial speed or Pareto result.

Run only in the manual GitHub Actions protocol. The scout owns process-group
creation, deadline enforcement, descendant cleanup, and artifact upload.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import json
import math
import os
import random
import re
import signal
import sys
import tempfile
from pathlib import Path
from types import ModuleType
from typing import Any


PINNED_RUNNER_SHA256 = "9a42bf80f6a35bbec61f191a40daf416652e1ee9c35fd7461c5de13755209b08"


class CaptureInterrupted(RuntimeError):
    """A termination signal interrupted the experiment, not its hypothesis."""


def build_call_plan(control: list[str], candidate: list[str], warmups: int,
                    reps: int, seed: int) -> list[dict[str, Any]]:
    """Metadata-only copy of the verified driver's schedule, never a scheduler."""
    plan: list[dict[str, Any]] = []
    for warmup in range(warmups):
        order = ("control", "candidate") if warmup % 2 == 0 else ("candidate", "control")
        for position, arm in enumerate(order):
            plan.append({"call_index": len(plan), "phase": "warmup", "pair": None,
                         "warmup": warmup, "position": position, "arm": arm,
                         "order": "->".join(order),
                         "argv": list(control if arm == "control" else candidate)})
    # This is an independent RNG used only to check the actual driver's calls.
    rng = random.Random(seed)
    for pair in range(reps):
        order = ("control", "candidate") if rng.randrange(2) == 0 else ("candidate", "control")
        for position, arm in enumerate(order):
            plan.append({"call_index": len(plan), "phase": "timed", "pair": pair,
                         "warmup": None, "position": position, "arm": arm,
                         "order": "->".join(order),
                         "argv": list(control if arm == "control" else candidate)})
    return plan


def _parse_runner_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument("--control-json", required=True)
    parser.add_argument("--candidate-json", required=True)
    parser.add_argument("--control-observe", type=Path, required=True)
    parser.add_argument("--candidate-observe", type=Path, required=True)
    parser.add_argument("--expected-sha256", required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--summary", type=Path, required=True)
    parser.add_argument("--reps", type=int, default=9)
    parser.add_argument("--warmups", type=int, default=2)
    parser.add_argument("--seed", type=int, default=41246)
    parser.add_argument("--bootstrap", type=int, default=20000)
    parser.add_argument("--epsilon", type=float, default=0.02)
    parser.add_argument("--max-cv", type=float, default=0.15)
    parser.add_argument("--timeout-s", type=float, default=1800.0)
    parser.add_argument("--ambient-reps", type=int, default=5)
    parser.add_argument("--ambient-iterations", type=int, default=250000)
    parser.add_argument("--ambient-max-cv", type=float, default=0.10)
    args, _ = parser.parse_known_args(argv)
    for field in ("control_json", "candidate_json"):
        value = json.loads(getattr(args, field))
        if not isinstance(value, list) or not value or any(not isinstance(x, str) for x in value):
            raise ValueError(f"{field} must be a nonempty string argv array")
        setattr(args, field.removesuffix("_json"), value)
    frozen = {"warmups": 2, "seed": 41246, "bootstrap": 20000, "epsilon": 0.02,
              "max_cv": 0.15, "timeout_s": 1800.0, "ambient_reps": 5,
              "ambient_iterations": 250000, "ambient_max_cv": 0.10}
    if args.reps not in (9, 13):
        raise ValueError("A70 capture requires the preregistered 9 or 13 pairs")
    for field, expected in frozen.items():
        if getattr(args, field) != expected:
            raise ValueError(f"frozen A70 setting changed: {field}")
    if not re.fullmatch(r"[0-9a-fA-F]{64}", args.expected_sha256):
        raise ValueError("A70 expected output SHA-256 is mandatory")
    return args


def _atomic_json(path: Path, value: dict[str, Any]) -> None:
    """Replace a state snapshot only after its complete new bytes are durable."""
    temporary: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", newline="\n",
                                         dir=path.parent, prefix=path.name + ".tmp-",
                                         delete=False) as handle:
            temporary = Path(handle.name)
            json.dump(value, handle, indent=2, sort_keys=True, allow_nan=False)
            handle.write("\n")
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary, path)
        temporary = None
        if hasattr(os, "O_DIRECTORY"):
            directory_fd = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
            try:
                os.fsync(directory_fd)
            finally:
                os.close(directory_fd)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


class _Journal:
    def __init__(self, path: Path, state_path: Path, state: dict[str, Any]):
        self.handle = path.open("x", encoding="utf-8", newline="\n")
        self.state_path = state_path
        self.state = state
        self.sequence = 0
        self.write_failed = False

    def emit(self, event: str, **fields: Any) -> None:
        if self.write_failed:
            raise RuntimeError("capture journal already failed; refusing more observations")
        self.sequence += 1
        record = {"schema": 1, "seq": self.sequence, "event": event, **fields}
        try:
            self.handle.write(json.dumps(record, sort_keys=True, allow_nan=False) + "\n")
            self.handle.flush()
            os.fsync(self.handle.fileno())
            self.state["last_event_seq"] = self.sequence
            _atomic_json(self.state_path, self.state)
        except BaseException:
            self.write_failed = True
            raise
        # Acknowledgment follows both durable writes, never precedes them.
        print(f"A70_CAPTURE event={event} seq={self.sequence}", flush=True)

    def close(self) -> None:
        self.handle.close()


def _load_runner(path: Path) -> ModuleType:
    spec = importlib.util.spec_from_file_location("a70_frozen_paired_driver", path)
    if spec is None or spec.loader is None:
        raise ValueError("cannot load verified paired timing driver")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _verify_completion(args: argparse.Namespace, verified: list[dict[str, Any]]) -> dict:
    """Check coverage and identity only; never recompute or replace statistics."""
    summary = json.loads(args.summary.read_text(encoding="utf-8"))
    for key, expected in (("reps", args.reps), ("warmups", args.warmups),
                          ("seed", args.seed), ("bootstrap_samples", args.bootstrap)):
        if summary.get(key) != expected:
            raise RuntimeError(f"frozen summary coverage/identity mismatch: {key}")
    pairs = summary.get("pairs")
    if not isinstance(pairs, list) or len(pairs) != args.reps:
        raise RuntimeError("incomplete original paired summary")
    for arm in ("control", "candidate"):
        if summary.get(arm, {}).get("argv") != getattr(args, arm):
            raise RuntimeError(f"summary command identity mismatch: {arm}")
        observation = summary.get("observed_outputs", {}).get(arm, {})
        if (not isinstance(observation, dict) or observation.get("deterministic") is not True
                or str(observation.get("sha256", "")).lower() != args.expected_sha256.lower()):
            raise RuntimeError(f"summary output identity missing: {arm}")
    for index, pair in enumerate(pairs):
        records = [row for row in verified if row["pair"] == index]
        if (pair.get("pair") != index or len(records) != 2
                or {row["arm"] for row in records} != {"control", "candidate"}
                or pair.get("order") != records[0]["order"]):
            raise RuntimeError("summary pair coverage/order mismatch")
        for row in records:
            if pair.get(row["arm"] + "_s") != row["seconds"]:
                raise RuntimeError("summary differs from captured original elapsed time")
    with args.out.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    if len(rows) != len(verified):
        raise RuntimeError("original raw CSV has incomplete coverage")
    for row, record in zip(rows, verified):
        expected = {"pair": str(record["pair"]), "position": str(record["position"]),
                    "order": record["order"], "arm": record["arm"],
                    "seconds": f"{record['seconds']:.9f}",
                    "observed_bytes": str(record["observation"]["bytes"]),
                    "observed_sha256": record["observation"]["sha256"]}
        if row != expected:
            raise RuntimeError("original raw CSV differs from verified capture")
    return summary


def run_capture(runner: Path, runner_sha256: str, journal: Path, state: Path,
                runner_args: list[str], *, module: ModuleType | None = None) -> dict[str, Any]:
    """Run the pinned driver with capture; module injection is for GHA fixtures.

    Production CLI always imports the SHA-verified file. Tests can inject the
    same module with fake subprocess/clock dependencies while retaining its
    original main, schedule, and estimator. Exceptions are recorded and reraised.
    """
    runner, journal, state = Path(runner), Path(journal), Path(state)
    if runner_sha256.lower() != PINNED_RUNNER_SHA256:
        raise ValueError("requested runner SHA is not the frozen A70 runner")
    actual_sha = hashlib.sha256(runner.read_bytes()).hexdigest()
    if actual_sha != PINNED_RUNNER_SHA256:
        raise ValueError("frozen paired runner source SHA mismatch")
    args = _parse_runner_args(list(runner_args))
    artifacts = [journal, state, args.out, args.summary]
    artifact_paths = [path.resolve() for path in artifacts]
    if len(set(artifact_paths)) != len(artifact_paths):
        raise ValueError("capture journal/state/raw/summary paths must be distinct")
    protected = set(artifact_paths) | {runner.resolve()}
    for path in (args.control_observe, args.candidate_observe):
        if path.resolve() in protected:
            raise ValueError("observation output collides with protected capture/source path")
        if path.is_dir():
            raise ValueError("observation output must be a file")
    for path in artifacts:
        if path.exists() or path.is_symlink():
            raise FileExistsError(f"capture requires fresh output paths: {path}")
    for path in artifacts:
        path.parent.mkdir(parents=True, exist_ok=True)
    plan = build_call_plan(args.control, args.candidate, args.warmups, args.reps, args.seed)
    current: dict[str, Any] = {
        "schema": 1, "status": "RUNNING", "complete": False,
        "runner_sha256": actual_sha, "runner_path": str(runner.resolve()),
        "capture_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "runner_arguments": list(runner_args),
        "expected_output_sha256": args.expected_sha256,
        "requested_reps": args.reps, "expected_calls": len(plan),
        "expected_verified_arms": 2 * args.reps, "calls_started": 0,
        "calls_returned": 0, "verified_arms": 0, "complete_pairs": [],
        "current_call": None, "failure": None, "scientific_status": None,
        "timing_valid": False, "partial_scientific_claim_prohibited": True,
        "github_run_id": os.environ.get("GITHUB_RUN_ID"),
        "github_run_attempt": os.environ.get("GITHUB_RUN_ATTEMPT"),
        "workflow_sha": os.environ.get("GITHUB_SHA"),
    }
    sink = _Journal(journal, state, current)
    verified: list[dict[str, Any]] = []
    pending: dict[str, Any] | None = None
    previous_argv = sys.argv
    previous_term = None
    driver = None
    original_run = original_observation = None

    def interrupted(signum: int, _frame: Any) -> None:
        raise CaptureInterrupted(f"capture interrupted by signal {signum}")

    def captured_run(argv: list[str], timeout_s: float, env: dict[str, str],
                     stdout_path: Path | None, stderr_path: Path | None) -> float:
        nonlocal pending
        index = current["calls_started"]
        if pending is not None:
            raise RuntimeError("missing post-run observation before next command")
        if index >= len(plan):
            raise RuntimeError("unexpected extra driver command")
        call = plan[index]
        current["current_call"] = dict(call)
        if list(argv) != call["argv"] or timeout_s != args.timeout_s:
            raise RuntimeError("driver call differs from frozen schedule/timeout")
        observe = args.control_observe if call["arm"] == "control" else args.candidate_observe
        # Freshness is outside the frozen clock, including on A/A and warmups.
        # A no-op command must not inherit a previously correct decoded file.
        observe.unlink(missing_ok=True)
        current["calls_started"] += 1
        sink.emit("START", **call, observation_path=str(observe))
        assert original_run is not None
        elapsed = original_run(argv, timeout_s, env, stdout_path, stderr_path)
        if not isinstance(elapsed, (int, float)) or not math.isfinite(elapsed) or elapsed <= 0:
            raise RuntimeError("original driver returned invalid elapsed time")
        current["calls_returned"] += 1
        if call["phase"] == "timed":
            pending = {**call, "seconds": elapsed}
        sink.emit("RETURN", **call, seconds=elapsed)
        return elapsed

    def captured_observation(path: Path | None, expected_sha256: str | None) -> dict[str, Any]:
        nonlocal pending
        if pending is None:
            raise RuntimeError("duplicate or unscheduled post-run observation")
        observe = args.control_observe if pending["arm"] == "control" else args.candidate_observe
        if (path is None or Path(path).resolve() != observe.resolve()
                or expected_sha256 != args.expected_sha256):
            raise RuntimeError("observation differs from scheduled output/SHA")
        assert original_observation is not None
        observation = original_observation(path, expected_sha256)
        if (not isinstance(observation, dict)
                or str(observation.get("sha256", "")).lower() != args.expected_sha256.lower()
                or type(observation.get("bytes")) is not int or observation["bytes"] < 0):
            raise RuntimeError("original observation did not verify expected output")
        record = {**pending, "observation": dict(observation)}
        verified.append(record)
        current["verified_arms"] = len(verified)
        pair = pending["pair"]
        pair_rows = [row for row in verified if row["pair"] == pair]
        if len(pair_rows) == 2:
            if {row["arm"] for row in pair_rows} != {"control", "candidate"}:
                raise RuntimeError("duplicate arm in captured pair")
            current["complete_pairs"].append(pair)
        sink.emit("VERIFIED", **record)
        pending = None
        return observation

    try:
        sink.emit("INIT", runner_sha256=actual_sha, capture_sha256=current["capture_sha256"],
                  requested_reps=args.reps, expected_calls=len(plan),
                  expected_verified_arms=2 * args.reps, call_plan=plan,
                  runner_arguments=list(runner_args), expected_output_sha256=args.expected_sha256,
                  github_run_id=current["github_run_id"],
                  github_run_attempt=current["github_run_attempt"],
                  workflow_sha=current["workflow_sha"])
        previous_term = signal.signal(signal.SIGTERM, interrupted)
        driver = module if module is not None else _load_runner(runner)
        original_run, original_observation = driver.run_once, driver.post_run_observation
        driver.run_once, driver.post_run_observation = captured_run, captured_observation
        sys.argv = [str(runner), *runner_args]
        driver.main()
        if (pending is not None or current["calls_started"] != len(plan)
                or current["calls_returned"] != len(plan)
                or len(verified) != 2 * args.reps
                or current["complete_pairs"] != list(range(args.reps))):
            raise RuntimeError("driver returned with incomplete capture coverage")
        summary = _verify_completion(args, verified)
        current.update(status="COMPLETE", complete=True, current_call=None,
                       scientific_status=summary.get("status"),
                       timing_valid=summary.get("timing_valid") is True)
        sink.emit("COMPLETE", verified_arms=len(verified), complete_pairs=list(range(args.reps)),
                  scientific_status=current["scientific_status"],
                  timing_valid=current["timing_valid"])
        return current
    except BaseException as exc:
        current.update(status="INCOMPLETE", complete=False, scientific_status=None,
                       timing_valid=False,
                       failure={"type": type(exc).__name__, "message": str(exc)})
        try:
            sink.emit("FAILED", failure=current["failure"], current_call=current["current_call"],
                      verified_arms=len(verified), partial_scientific_claim_prohibited=True)
        except BaseException:
            # A broken journal is itself fatal. A separate atomic snapshot may
            # still preserve the reason; never continue with uncaptured timing.
            try:
                _atomic_json(state, current)
            except BaseException:
                pass
        raise
    finally:
        if driver is not None and original_run is not None:
            driver.run_once = original_run
            driver.post_run_observation = original_observation
        sys.argv = previous_argv
        if previous_term is not None:
            signal.signal(signal.SIGTERM, previous_term)
        sink.close()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--runner-sha256", required=True)
    parser.add_argument("--journal", type=Path, required=True)
    parser.add_argument("--state", type=Path, required=True)
    parser.add_argument("runner_args", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    forwarded = args.runner_args
    if forwarded and forwarded[0] == "--":
        forwarded = forwarded[1:]
    if not forwarded:
        parser.error("unchanged paired_bench arguments are required after --")
    run_capture(args.runner, args.runner_sha256, args.journal, args.state, forwarded)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
