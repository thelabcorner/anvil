"""Independent A70 acceptance tests. EXECUTE ONLY on GitHub Actions.

The original paired_bench.py is the immutable statistical oracle. These tests
change its external clock/process dependencies, never its estimator or schedule.
No codec benchmark, build, or local execution is needed to author this file.
"""
from __future__ import annotations

import csv
import hashlib
import importlib.util
import json
import os
import random
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from contextlib import ExitStack
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch


if os.environ.get("GITHUB_ACTIONS") != "true":
    raise unittest.SkipTest("A70 protocol acceptance executes only on GitHub Actions")

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
RUNNER = TOOLS / "paired_bench.py"
FROZEN_RUNNER_SHA256 = "9a42bf80f6a35bbec61f191a40daf416652e1ee9c35fd7461c5de13755209b08"
PAYLOAD = b"A70 independent deterministic output\x00\xff\n"
OUTPUT_SHA256 = hashlib.sha256(PAYLOAD).hexdigest()


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load test subject {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


capture = load_module("a70_independent_capture_subject", TOOLS / "a70_paired_capture.py")
scout = load_module("a70_independent_scout_subject", TOOLS / "a70_version_scout.py")


def runner_arguments(work: Path, reps: int) -> list[str]:
    work.mkdir(parents=True, exist_ok=True)
    return [
        "--control-json", json.dumps(["control", str(work / "control.out")]),
        "--candidate-json", json.dumps(["candidate", str(work / "candidate.out")]),
        "--control-observe", str(work / "control.out"),
        "--candidate-observe", str(work / "candidate.out"),
        "--expected-sha256", OUTPUT_SHA256,
        "--reps", str(reps), "--warmups", "2",
        "--seed", "41246", "--bootstrap", "20000",
        "--epsilon", "0.02", "--max-cv", "0.15",
        "--timeout-s", "1800",
        "--ambient-reps", "5", "--ambient-iterations", "250000",
        "--ambient-max-cv", "0.10",
        "--out", str(work / "raw.csv"),
        "--summary", str(work / "summary.json"),
    ]


def emit_complete_capture_fixture(argv: list[str], source_hash: str, observed_bytes: int = 6):
    """Evaluator-owned coherent fixture for scout-consumer/affinity tests."""
    def argument(name):
        return argv[argv.index(name) + 1]

    reps = int(argument("--reps"))
    controls = {"control": json.loads(argument("--control-json")),
                "candidate": json.loads(argument("--candidate-json"))}
    observed = {"control": argument("--control-observe"),
                "candidate": argument("--candidate-observe")}
    events, raw, pairs = [], [], []

    def emit(event, **fields):
        events.append({"schema": 1, "seq": len(events) + 1, "event": event, **fields})

    emit("INIT", runner_sha256=FROZEN_RUNNER_SHA256, requested_reps=reps,
         expected_calls=4 + 2 * reps, expected_verified_arms=2 * reps)
    schedule = [("control", "candidate"), ("candidate", "control")]
    rng = random.Random(41246)
    schedule.extend(
        ("control", "candidate") if rng.randrange(2) == 0 else ("candidate", "control")
        for _ in range(reps))
    call_index = 0
    for round_index, order in enumerate(schedule):
        pair = round_index - 2
        phase = "warmup" if pair < 0 else "timed"
        for position, arm in enumerate(order):
            call = {"call_index": call_index, "phase": phase,
                    "pair": None if pair < 0 else pair,
                    "warmup": round_index if pair < 0 else None,
                    "position": position, "arm": arm,
                    "order": "->".join(order), "argv": controls[arm]}
            emit("START", **call, observation_path=observed[arm])
            emit("RETURN", **call, seconds=1.0)
            if pair >= 0:
                emit("VERIFIED", **call, seconds=1.0,
                     observation={"path": observed[arm], "bytes": observed_bytes,
                                  "sha256": source_hash})
                raw.append({"pair": pair, "position": position, "order": "->".join(order),
                            "arm": arm, "seconds": "1.000000000",
                            "observed_bytes": observed_bytes, "observed_sha256": source_hash})
            call_index += 1
        if pair >= 0:
            pairs.append({"pair": pair, "order": "->".join(order),
                          "control_s": 1.0, "candidate_s": 1.0,
                          "log_candidate_over_control": 0.0})
    emit("COMPLETE", verified_arms=2 * reps, complete_pairs=list(range(reps)),
         scientific_status="NO_SPEED_GATE", timing_valid=True)
    state = {
        "schema": 1, "status": "COMPLETE", "complete": True,
        "runner_sha256": FROZEN_RUNNER_SHA256, "requested_reps": reps,
        "expected_calls": 4 + 2 * reps, "expected_verified_arms": 2 * reps,
        "calls_started": 4 + 2 * reps, "calls_returned": 4 + 2 * reps,
        "verified_arms": 2 * reps, "complete_pairs": list(range(reps)),
        "current_call": None, "failure": None, "last_event_seq": len(events),
        "scientific_status": "NO_SPEED_GATE", "timing_valid": True,
    }
    summary = {
        "schema": 1, "status": "NO_SPEED_GATE", "timing_valid": True,
        "clears_speed_gate": False, "practical_epsilon": 0.02,
        "paired_ratio_candidate_over_control": 1.0, "paired_ratio_ci95": [1.0, 1.0],
        "candidate_speedup_point_estimate": 1.0, "pairs": pairs,
        "reps": reps, "warmups": 2, "seed": 41246, "bootstrap_samples": 20000,
        "observed_outputs": {
            arm: {"bytes": observed_bytes, "sha256": source_hash, "deterministic": True}
            for arm in ("control", "candidate")
        },
    }
    for arm in ("control", "candidate"):
        summary[arm] = {"argv": controls[arm], "median_s": 1.0,
                        "mad_s": 0.0, "robust_cv": 0.0}
    Path(argument("--summary")).write_text(json.dumps(summary), encoding="utf-8")
    Path(argument("--state")).write_text(json.dumps(state), encoding="utf-8")
    Path(argument("--journal")).write_text(
        "".join(json.dumps(event) + "\n" for event in events), encoding="utf-8")
    with Path(argument("--out")).open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(raw[0]))
        writer.writeheader()
        writer.writerows(raw)
    return summary, state, events


class FakeProcesses:
    """External process/clock fixture; paired-driver mathematics stays intact."""

    CONTROL = [
        1.0, 1.0625, 0.9375, 1.03125, 0.96875, 1.015625, 0.984375,
        1.046875, 0.953125, 1.0078125, 0.9921875, 1.0234375, 0.9765625,
    ]
    RATIOS = [
        0.875, 0.90625, 0.84375, 0.890625, 0.859375, 0.921875, 0.828125,
        0.875, 0.90625, 0.84375, 0.890625, 0.859375, 0.875,
    ]

    def __init__(self, *, fail_call=None, failure="returncode", ratio=None,
                 pause_call=None, marker=None):
        self.now = 0.0
        self.calls = []
        self.fail_call = fail_call
        self.failure = failure
        self.ratio = ratio
        self.pause_call = pause_call
        self.marker = marker

    def perf_counter(self):
        return self.now

    def run(self, argv, **kwargs):
        index = len(self.calls)
        self.calls.append(list(argv))
        if self.pause_call == index:
            Path(self.marker).write_text("paused", encoding="utf-8")
            time.sleep(60)
        arm = argv[0]
        if arm not in ("control", "candidate"):
            raise AssertionError(f"unexpected fixture command {argv!r}")
        pair = max(0, (index - 4) // 2)
        if index < 4:
            seconds = 0.125
        else:
            control = self.CONTROL[pair]
            ratio = self.RATIOS[pair] if self.ratio is None else self.ratio
            seconds = control if arm == "control" else control * ratio
        self.now += seconds
        if index == self.fail_call:
            if self.failure == "returncode":
                return subprocess.CompletedProcess(argv, 7, stdout=b"", stderr=b"injected failure")
            if self.failure == "timeout":
                raise subprocess.TimeoutExpired(argv, kwargs.get("timeout", 1800))
            if self.failure == "launch":
                raise OSError("injected launch failure")
            if self.failure == "missing":
                return subprocess.CompletedProcess(argv, 0, stdout=b"", stderr=b"")
            if self.failure == "wrong_hash":
                Path(argv[1]).write_bytes(b"wrong output")
                return subprocess.CompletedProcess(argv, 0, stdout=b"", stderr=b"")
            raise AssertionError(self.failure)
        Path(argv[1]).write_bytes(PAYLOAD)
        return subprocess.CompletedProcess(argv, 0, stdout=b"", stderr=b"")


def fixture_environment(module, fake: FakeProcesses):
    stack = ExitStack()
    original_observation = module.post_run_observation

    def deliberately_expensive_observation(path, expected):
        # This cost MUST remain outside the original subprocess clock.
        fake.now += 1024.0
        return original_observation(path, expected)

    stack.enter_context(patch.object(module, "time",
                                    SimpleNamespace(perf_counter=fake.perf_counter)))
    stack.enter_context(patch.object(module, "subprocess",
                                    SimpleNamespace(run=fake.run, PIPE=subprocess.PIPE,
                                                    DEVNULL=subprocess.DEVNULL)))
    stack.enter_context(patch.object(module, "ambient_probe", return_value={
        "seconds": [1.0] * 5, "median_s": 1.0, "robust_cv": 0.0, "checksum": 17,
    }))
    stack.enter_context(patch.object(module, "set_affinity", return_value=[4]))
    stack.enter_context(patch.object(module, "post_run_observation",
                                    side_effect=deliberately_expensive_observation))
    return stack


def invoke_fixture(work: Path, reps: int, *, wrapped: bool, fake=None):
    fake = fake or FakeProcesses()
    module = load_module(f"a70_frozen_fixture_{os.getpid()}_{id(fake)}", RUNNER)
    arguments = runner_arguments(work, reps)
    with fixture_environment(module, fake):
        if wrapped:
            result = capture.run_capture(
                RUNNER, FROZEN_RUNNER_SHA256, work / "events.jsonl",
                work / "state.json", arguments, module=module)
        else:
            with patch.object(sys, "argv", [str(RUNNER), *arguments]):
                module.main()
            result = None
    return fake, result


def read_events(work: Path):
    return [json.loads(line) for line in
            (work / "events.jsonl").read_text(encoding="utf-8").splitlines()]


def timed_events(work: Path, event: str):
    return [row for row in read_events(work)
            if row["event"] == event and row.get("phase") == "timed"]


def child_pause_capture(work: str, marker: str):
    """Called only by an isolated GitHub Actions acceptance subprocess."""
    invoke_fixture(Path(work), 9, wrapped=True,
                   fake=FakeProcesses(pause_call=7, marker=Path(marker)))


def process_is_live(pid: int) -> bool:
    path = Path(f"/proc/{pid}/stat")
    try:
        # A killed child may briefly remain as a zombie awaiting init's reap.
        state = path.read_text(encoding="utf-8").rpartition(") ")[2].split()[0]
        return state not in ("Z", "X")
    except FileNotFoundError:
        return False


class FrozenEstimatorAcceptance(unittest.TestCase):
    def test_runner_source_is_the_independently_frozen_oracle(self):
        self.assertEqual(hashlib.sha256(RUNNER.read_bytes()).hexdigest(),
                         FROZEN_RUNNER_SHA256)
        self.assertEqual(capture.PINNED_RUNNER_SHA256, FROZEN_RUNNER_SHA256)

    def assert_same_statistics(self, old, new):
        for key in (
            "status", "timing_valid", "clears_speed_gate", "practical_epsilon",
            "paired_ratio_candidate_over_control", "paired_ratio_ci95",
            "candidate_speedup_point_estimate", "pairs", "ambient", "affinity",
            "seed", "warmups", "reps", "bootstrap_samples",
        ):
            self.assertEqual(old[key], new[key], key)
        for arm in ("control", "candidate"):
            for key in ("median_s", "mad_s", "robust_cv"):
                self.assertEqual(old[arm][key], new[arm][key], f"{arm}/{key}")
            self.assertEqual(old["observed_outputs"][arm],
                             new["observed_outputs"][arm])

    def test_capture_preserves_original_schedule_clock_and_estimator_at_9_and_13(self):
        for reps in (9, 13):
            for ratio in (None, 0.8, 1.0):
                with self.subTest(reps=reps, ratio=ratio), tempfile.TemporaryDirectory() as td:
                    base, revised = Path(td) / "baseline", Path(td) / "capture"
                    base_fake = FakeProcesses(ratio=ratio)
                    new_fake = FakeProcesses(ratio=ratio)
                    invoke_fixture(base, reps, wrapped=False, fake=base_fake)
                    _, state = invoke_fixture(revised, reps, wrapped=True, fake=new_fake)
                    original = json.loads((base / "summary.json").read_text())
                    wrapped = json.loads((revised / "summary.json").read_text())
                    self.assert_same_statistics(original, wrapped)
                    self.assertEqual([a[0] for a in base_fake.calls],
                                     [a[0] for a in new_fake.calls])
                    self.assertEqual((base / "raw.csv").read_bytes(),
                                     (revised / "raw.csv").read_bytes())
                    self.assertEqual(state["status"], "COMPLETE")
                    self.assertTrue(state["complete"])
                    self.assertEqual(state["calls_started"], 4 + 2 * reps)
                    self.assertEqual(state["calls_returned"], 4 + 2 * reps)
                    self.assertEqual(state["verified_arms"], 2 * reps)
                    self.assertEqual(state["complete_pairs"], list(range(reps)))
                    verified = timed_events(revised, "VERIFIED")
                    self.assertEqual(len(verified), 2 * reps)
                    self.assertEqual(
                        {(r["pair"], r["arm"]) for r in verified},
                        {(p, a) for p in range(reps) for a in ("control", "candidate")})
                    if ratio is not None:
                        # The exact baseline comparison above is primary. The
                        # ideal ratio tolerates clock-subtraction rounding after
                        # deliberately large outside-clock observation costs.
                        self.assertAlmostEqual(wrapped["paired_ratio_candidate_over_control"],
                                               ratio, delta=1e-10)
                        for endpoint in wrapped["paired_ratio_ci95"]:
                            self.assertAlmostEqual(endpoint, ratio, delta=1e-10)

    def test_capture_rejects_changed_frozen_configuration_before_invocation(self):
        changes = {
            "--reps": ("7", "8", "10"),
            "--warmups": ("1",),
            "--seed": ("41247",),
            "--bootstrap": ("1000",),
            "--epsilon": ("0.01",),
            "--max-cv": ("0.16",),
            "--timeout-s": ("600",),
            "--ambient-reps": ("3",),
            "--ambient-iterations": ("10",),
            "--ambient-max-cv": ("0.11",),
        }
        for option, values in changes.items():
            for value in values:
                with self.subTest(option=option, value=value), tempfile.TemporaryDirectory() as td:
                    work = Path(td)
                    arguments = runner_arguments(work, 9)
                    arguments[arguments.index(option) + 1] = value
                    module = SimpleNamespace(main=lambda: self.fail("decoder work was invoked"))
                    with self.assertRaises((ValueError, SystemExit)):
                        capture.run_capture(RUNNER, FROZEN_RUNNER_SHA256,
                                            work / "events.jsonl", work / "state.json",
                                            arguments, module=module)
                    self.assertFalse((work / "summary.json").exists())

    def test_modified_runner_cannot_use_the_frozen_hash(self):
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            altered = work / "altered_runner.py"
            altered.write_bytes(RUNNER.read_bytes() + b"\n# unreviewed change\n")
            with self.assertRaises(ValueError):
                capture.run_capture(altered, FROZEN_RUNNER_SHA256,
                                    work / "events.jsonl", work / "state.json",
                                    runner_arguments(work, 9))


class CaptureFailureAcceptance(unittest.TestCase):
    @unittest.skipUnless(Path("/proc/self/fd").is_dir(), "requires Linux descriptor inspection")
    def test_journal_fsync_failure_stops_before_another_process(self):
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            journal = work / "events.jsonl"
            original_fsync = os.fsync
            injected = False
            fake = FakeProcesses()

            def fail_journal_fsync(fd):
                nonlocal injected
                try:
                    target = Path(os.readlink(f"/proc/self/fd/{fd}"))
                except OSError:
                    target = None
                if (not injected and target == journal and journal.exists()
                        and '"event": "VERIFIED"' in journal.read_text(encoding="utf-8")):
                    injected = True
                    raise OSError("injected journal durability failure")
                return original_fsync(fd)

            with patch.object(capture.os, "fsync", side_effect=fail_journal_fsync):
                with self.assertRaises(OSError):
                    invoke_fixture(work, 9, wrapped=True, fake=fake)
            self.assertTrue(injected)
            self.assertEqual(len(fake.calls), 5, "work continued after unacknowledged journal write")
            state = json.loads((work / "state.json").read_text())
            self.assertEqual(state["status"], "INCOMPLETE")
            self.assertFalse(state["complete"])
            self.assertFalse(state["timing_valid"])
            self.assertFalse((work / "summary.json").exists())

    def test_missing_or_duplicate_observation_hook_cannot_complete(self):
        for fault in ("missing", "duplicate"):
            with self.subTest(fault=fault), tempfile.TemporaryDirectory() as td:
                work = Path(td)
                arguments = runner_arguments(work, 9)
                module = load_module(f"a70_bad_hook_fixture_{fault}", RUNNER)
                fake = FakeProcesses()
                commands = {"control": ["control", str(work / "control.out")],
                            "candidate": ["candidate", str(work / "candidate.out")]}

                def deliberately_broken_driver():
                    for arm in ("control", "candidate", "candidate", "control"):
                        module.run_once(commands[arm], 1800.0, {}, None, None)
                    order = (("control", "candidate") if random.Random(41246).randrange(2) == 0
                             else ("candidate", "control"))
                    arm = order[0]
                    module.run_once(commands[arm], 1800.0, {}, None, None)
                    if fault == "missing":
                        module.run_once(commands[order[1]], 1800.0, {}, None, None)
                    else:
                        output = Path(commands[arm][1])
                        module.post_run_observation(output, OUTPUT_SHA256)
                        module.post_run_observation(output, OUTPUT_SHA256)

                with fixture_environment(module, fake), \
                        patch.object(module, "main", side_effect=deliberately_broken_driver):
                    with self.assertRaises(RuntimeError):
                        capture.run_capture(
                            RUNNER, FROZEN_RUNNER_SHA256, work / "events.jsonl",
                            work / "state.json", arguments, module=module)
                self.assertEqual(len(fake.calls), 5)
                state = json.loads((work / "state.json").read_text())
                self.assertEqual(state["status"], "INCOMPLETE")
                self.assertEqual(state["verified_arms"], 0 if fault == "missing" else 1)
                self.assertFalse((work / "summary.json").exists())

    def test_complete_event_without_committed_complete_state_is_not_success(self):
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            original = capture._atomic_json
            failed = False

            def fail_one_complete_state_write(path, value):
                nonlocal failed
                if value.get("status") == "COMPLETE" and not failed:
                    failed = True
                    raise OSError("injected failure between COMPLETE event and state commit")
                return original(path, value)

            with patch.object(capture, "_atomic_json", side_effect=fail_one_complete_state_write):
                with self.assertRaises(OSError):
                    invoke_fixture(work, 9, wrapped=True)
            self.assertTrue(failed)
            self.assertEqual(read_events(work)[-1]["event"], "COMPLETE")
            state = json.loads((work / "state.json").read_text())
            self.assertEqual(state["status"], "INCOMPLETE")
            self.assertFalse(state["complete"])
            self.assertFalse(state["timing_valid"])

    def test_failure_on_sixth_measured_invocation_retains_five_observations(self):
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            with self.assertRaises(RuntimeError):
                invoke_fixture(work, 9, wrapped=True, fake=FakeProcesses(fail_call=9))
            state = json.loads((work / "state.json").read_text())
            self.assertEqual(state["status"], "INCOMPLETE")
            self.assertFalse(state["complete"])
            self.assertFalse(state["timing_valid"])
            self.assertEqual(state["verified_arms"], 5)
            self.assertEqual(state["complete_pairs"], [0, 1])
            self.assertEqual(state["current_call"]["pair"], 2)
            self.assertEqual(state["current_call"]["position"], 1)
            self.assertEqual(len(timed_events(work, "RETURN")), 5)
            self.assertEqual(len(timed_events(work, "VERIFIED")), 5)
            self.assertEqual(read_events(work)[-1]["event"], "FAILED")
            self.assertFalse((work / "summary.json").exists())

    def test_timeout_and_launch_failure_are_incomplete_not_measurements(self):
        for failure, exception in (("timeout", subprocess.TimeoutExpired), ("launch", OSError)):
            with self.subTest(failure=failure), tempfile.TemporaryDirectory() as td:
                work = Path(td)
                with self.assertRaises(exception):
                    invoke_fixture(work, 9, wrapped=True,
                                   fake=FakeProcesses(fail_call=4, failure=failure))
                state = json.loads((work / "state.json").read_text())
                self.assertEqual(state["status"], "INCOMPLETE")
                self.assertEqual(state["verified_arms"], 0)
                self.assertEqual(state["complete_pairs"], [])
                self.assertEqual(len(timed_events(work, "RETURN")), 0)
                self.assertFalse((work / "summary.json").exists())

    def test_missing_or_wrong_output_retains_elapsed_but_never_verifies(self):
        for failure in ("missing", "wrong_hash"):
            with self.subTest(failure=failure), tempfile.TemporaryDirectory() as td:
                work = Path(td)
                work.mkdir(exist_ok=True)
                # Correct stale bytes must not turn the no-write case into success.
                (work / "control.out").write_bytes(PAYLOAD)
                (work / "candidate.out").write_bytes(PAYLOAD)
                with self.assertRaises(RuntimeError):
                    invoke_fixture(work, 9, wrapped=True,
                                   fake=FakeProcesses(fail_call=4, failure=failure))
                state = json.loads((work / "state.json").read_text())
                self.assertEqual(state["status"], "INCOMPLETE")
                self.assertEqual(len(timed_events(work, "RETURN")), 1)
                self.assertEqual(len(timed_events(work, "VERIFIED")), 0)
                self.assertFalse((work / "summary.json").exists())

    def test_reused_success_artifacts_are_refused_and_preserved_as_prior_evidence(self):
        for name in ("events.jsonl", "state.json", "raw.csv", "summary.json"):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as td:
                work = Path(td)
                arguments = runner_arguments(work, 9)
                previous = b'{"status":"COMPLETE","complete":true,"prior_run":true}\n'
                artifact = work / name
                artifact.write_bytes(previous)
                module = SimpleNamespace(main=lambda: self.fail("reused run was invoked"))
                with self.assertRaises(FileExistsError):
                    capture.run_capture(RUNNER, FROZEN_RUNNER_SHA256,
                                        work / "events.jsonl", work / "state.json",
                                        arguments, module=module)
                self.assertEqual(artifact.read_bytes(), previous)

    def test_capture_refuses_observation_aliasing_its_protected_evidence(self):
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            arguments = runner_arguments(work, 9)
            arguments[arguments.index("--control-observe") + 1] = str(work / "summary.json")
            with self.assertRaises(ValueError):
                capture.run_capture(RUNNER, FROZEN_RUNNER_SHA256,
                                    work / "events.jsonl", work / "state.json", arguments)

    def test_real_interruption_keeps_flushed_prefix_without_a_scientific_claim(self):
        for signum in (signal.SIGTERM, signal.SIGKILL):
            with self.subTest(signal=signum), tempfile.TemporaryDirectory() as td:
                work = Path(td)
                marker = work / "paused"
                code = (
                    "import importlib.util,sys;"
                    "s=importlib.util.spec_from_file_location('acceptance_child',sys.argv[1]);"
                    "m=importlib.util.module_from_spec(s);s.loader.exec_module(m);"
                    "m.child_pause_capture(sys.argv[2],sys.argv[3])"
                )
                with (work / "child.log").open("wb") as log:
                    child = subprocess.Popen(
                        [sys.executable, "-u", "-c", code, str(Path(__file__).resolve()),
                         str(work / "capture"), str(marker)],
                        stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
                    try:
                        deadline = time.monotonic() + 15
                        while not marker.exists() and child.poll() is None and time.monotonic() < deadline:
                            time.sleep(0.02)
                        self.assertTrue(marker.exists(), (work / "child.log").read_text())
                        os.killpg(child.pid, signum)
                        child.wait(timeout=10)
                    finally:
                        if child.poll() is None:
                            os.killpg(child.pid, signal.SIGKILL)
                            child.wait(timeout=10)
                evidence = work / "capture"
                self.assertEqual(len(timed_events(evidence, "VERIFIED")), 3)
                state = json.loads((evidence / "state.json").read_text())
                self.assertFalse(state["complete"])
                self.assertFalse(state["timing_valid"])
                self.assertNotEqual(state["status"], "COMPLETE")
                self.assertFalse((evidence / "summary.json").exists())
                if signum == signal.SIGTERM:
                    self.assertEqual(state["status"], "INCOMPLETE")


class NativeRejectionAcceptance(unittest.TestCase):
    DIAGNOSTIC = b"anvil: truncated block payload"

    def guard(self, work, result=None, side_effect=None):
        argv = ["pinned-anvil", "d", str(work / "truncated.anv"),
                str(work / "decoded.out"), "--quiet"]
        with patch.object(scout, "run", return_value=result, side_effect=side_effect):
            return scout.assert_outer_truncation_guard(
                argv, work / "decoded.out", work / "guard", timeout=1)

    def test_only_controlled_native_rejection_is_accepted(self):
        valid = ((1, self.DIAGNOSTIC), (1, self.DIAGNOSTIC + b"\n"),
                 (1, self.DIAGNOSTIC + b"\r\n"))
        invalid_codes = (0, 2, 7, -6, -9, -11, 124, 126, 127, 134, 137, 139)
        invalid = [(code, self.DIAGNOSTIC + b"\n") for code in invalid_codes]
        invalid += [(1, b""), (1, b"anvil: bad checksum\n"),
                    (1, b"prefix " + self.DIAGNOSTIC + b"\n"),
                    (1, self.DIAGNOSTIC + b"\nsecond diagnostic\n"),
                    (1, self.DIAGNOSTIC + b"\n\n")]
        for code, stderr in valid:
            with self.subTest(valid=(code, stderr)), tempfile.TemporaryDirectory() as td:
                work = Path(td)
                self.guard(work, subprocess.CompletedProcess([], code, b"", stderr))
                self.assertTrue((work / "guard.guard.json").exists())
        for code, stderr in invalid:
            with self.subTest(invalid=(code, stderr)), tempfile.TemporaryDirectory() as td:
                work = Path(td)
                with self.assertRaises(RuntimeError):
                    self.guard(work, subprocess.CompletedProcess([], code, b"", stderr))
                self.assertTrue((work / "guard.guard.json").exists())

    def test_rejection_cannot_leave_success_output_or_partial_output(self):
        for suffix in ("", ".anvil-part"):
            with self.subTest(suffix=suffix), tempfile.TemporaryDirectory() as td:
                work = Path(td)
                path = Path(str(work / "decoded.out") + suffix)

                def creates_output(*args, **kwargs):
                    path.write_bytes(PAYLOAD)
                    return subprocess.CompletedProcess([], 1, b"", self.DIAGNOSTIC + b"\n")

                with self.assertRaises(RuntimeError):
                    self.guard(work, side_effect=creates_output)
                self.assertTrue((work / "guard.guard.json").exists())

    def test_dirty_truncation_output_is_refused_before_decoder_invocation(self):
        for suffix in ("", ".anvil-part"):
            with self.subTest(suffix=suffix), tempfile.TemporaryDirectory() as td:
                work = Path(td)
                path = Path(str(work / "decoded.out") + suffix)
                path.write_bytes(b"prior evidence")
                with patch.object(scout, "run") as mocked:
                    with self.assertRaises(RuntimeError):
                        scout.assert_outer_truncation_guard(
                            ["decoder"], work / "decoded.out", work / "guard")
                    mocked.assert_not_called()
                self.assertEqual(path.read_bytes(), b"prior evidence")

    def test_timeout_or_launch_exception_cannot_become_expected_rejection(self):
        for failure in (subprocess.TimeoutExpired(["decoder"], 1), OSError("missing executable")):
            with self.subTest(failure=type(failure).__name__), tempfile.TemporaryDirectory() as td:
                work = Path(td)
                with self.assertRaises((RuntimeError, subprocess.TimeoutExpired, OSError)):
                    self.guard(work, side_effect=failure)
                self.assertTrue((work / "guard.guard.json").exists())

    def test_scout_repetition_boundary_is_enforced_before_work(self):
        for reps in (9, 13):
            scout.require_scout_reps(reps)
        for reps in (-1, 0, 1, 7, 8, 10, 12, 14):
            with self.subTest(reps=reps), self.assertRaises((ValueError, RuntimeError)):
                scout.require_scout_reps(reps)
        with patch.object(scout, "sha256", side_effect=AssertionError("binary work reached")):
            with self.assertRaises((ValueError, RuntimeError)):
                scout.run_scout(SimpleNamespace(reps=7))

    def test_invalid_repetitions_are_refused_by_cli_and_direct_paired_entry(self):
        for reps in (7, 8):
            with self.subTest(reps=reps), tempfile.TemporaryDirectory() as td:
                work = Path(td)
                arguments = [
                    "a70_version_scout.py", "--old", "old", "--candidate", "new",
                    "--tools", str(work), "--out", str(work), "--reps", str(reps),
                ]
                with patch.object(sys, "argv", arguments), patch.object(scout, "run_scout") as invoked:
                    with self.assertRaises(SystemExit) as stopped:
                        scout.main()
                    self.assertNotEqual(stopped.exception.code, 0)
                    invoked.assert_not_called()
                with patch.object(scout, "run") as invoked:
                    with self.assertRaises((ValueError, RuntimeError)):
                        scout.paired(work, ["old"], ["new"], work / "old.out", work / "new.out",
                                     OUTPUT_SHA256, "invalid-count", work, reps)
                    invoked.assert_not_called()

    @unittest.skipUnless(os.name == "posix", "requires Linux managed process execution")
    def test_real_managed_return_codes_do_not_accept_arbitrary_nonzero(self):
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            accepted = scout.run(
                [sys.executable, "-c", "import sys;sys.exit(1)"],
                expect_failure=True, evidence_prefix=work / "controlled")
            self.assertEqual(accepted.returncode, 1)
            for code in (0, 2, 7, 137, 139):
                with self.subTest(code=code):
                    with self.assertRaises(RuntimeError):
                        scout.run(
                            [sys.executable, "-c", f"import sys;sys.exit({code})"],
                            expect_failure=True, evidence_prefix=work / f"exit-{code}")
                    evidence = json.loads((work / f"exit-{code}.process.json").read_text())
                    self.assertEqual(evidence["returncode"], code)

    @unittest.skipUnless(os.name == "posix", "requires Linux signal return-code semantics")
    def test_real_signals_are_abnormal_not_expected_decoder_rejections(self):
        child = (
            "import os,resource,signal,sys;"
            "resource.setrlimit(resource.RLIMIT_CORE,(0,0));"
            "os.kill(os.getpid(),int(sys.argv[1]))"
        )
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            for signum in (signal.SIGTERM, signal.SIGSEGV):
                with self.subTest(signal=signum):
                    prefix = work / f"signal-{int(signum)}"
                    with self.assertRaises(RuntimeError):
                        scout.run([sys.executable, "-c", child, str(int(signum))],
                                  expect_failure=True, evidence_prefix=prefix)
                    evidence = json.loads(Path(str(prefix) + ".process.json").read_text())
                    self.assertEqual(evidence["returncode"], -int(signum))

    def test_scout_consumer_refuses_uncommitted_or_inconsistent_capture(self):
        mutations = ("incomplete_state", "terminal_failure", "wrong_terminal_count",
                     "sequence_gap", "uncommitted_terminal")
        for mutation in mutations:
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as td:
                work = Path(td)

                def fake_run(argv, **kwargs):
                    _, state, events = emit_complete_capture_fixture(argv, OUTPUT_SHA256)
                    state_path = Path(argv[argv.index("--state") + 1])
                    journal_path = Path(argv[argv.index("--journal") + 1])
                    if mutation == "incomplete_state":
                        state.update(status="INCOMPLETE", complete=False)
                    elif mutation == "terminal_failure":
                        events[-1]["event"] = "FAILED"
                    elif mutation == "wrong_terminal_count":
                        events[-1]["verified_arms"] = 17
                    elif mutation == "sequence_gap":
                        events[1]["seq"] += 1
                    elif mutation == "uncommitted_terminal":
                        state["last_event_seq"] -= 1
                    state_path.write_text(json.dumps(state), encoding="utf-8")
                    journal_path.write_text(
                        "".join(json.dumps(event) + "\n" for event in events), encoding="utf-8")
                    return subprocess.CompletedProcess(argv, 0, b"", b"")

                with patch.object(scout.os, "sched_getaffinity", return_value={4, 7}, create=True), \
                        patch.object(scout, "run", side_effect=fake_run):
                    with self.assertRaises((RuntimeError, ValueError, OSError)):
                        scout.paired(work, ["old"], ["new"], work / "old.out", work / "new.out",
                                     OUTPUT_SHA256, "independent-consumer", work, 9)

    @unittest.skipUnless(os.name == "posix" and Path("/proc").is_dir(),
                         "requires the Linux GitHub Actions runner")
    def test_managed_timeout_kills_parent_and_term_ignoring_grandchild(self):
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            marker = work / "owned-processes.json"
            descendant = (
                "import signal,time;"
                "signal.signal(signal.SIGTERM,signal.SIG_IGN);"
                "time.sleep(60)"
            )
            parent = (
                "import json,os,pathlib,signal,subprocess,sys,time;"
                "signal.signal(signal.SIGTERM,signal.SIG_IGN);"
                "p=subprocess.Popen([sys.executable,'-c',sys.argv[2]]);"
                "pathlib.Path(sys.argv[1]).write_text(json.dumps("
                "{'parent':os.getpid(),'child':p.pid}));"
                "time.sleep(60)"
            )
            owned = None
            try:
                with self.assertRaises(RuntimeError):
                    scout.run(
                        [sys.executable, "-u", "-c", parent, str(marker), descendant],
                        timeout=1.0, evidence_prefix=work / "managed-timeout")
                self.assertTrue(marker.exists(), "fixture never started its owned process group")
                owned = json.loads(marker.read_text())
                deadline = time.monotonic() + 5
                while any(process_is_live(pid) for pid in owned.values()) and time.monotonic() < deadline:
                    time.sleep(0.02)
                self.assertFalse(process_is_live(owned["parent"]))
                self.assertFalse(process_is_live(owned["child"]))
                self.assertTrue((work / "managed-timeout.process.json").exists())
            finally:
                if owned is None and marker.exists():
                    owned = json.loads(marker.read_text())
                if owned and any(process_is_live(pid) for pid in owned.values()):
                    try:
                        os.killpg(owned["parent"], signal.SIGKILL)
                    except ProcessLookupError:
                        pass


if __name__ == "__main__":
    unittest.main()
