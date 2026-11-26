#!/usr/bin/env python3
"""A70 scout: pinned v2.10.4 vs v2.12.2 decoder on SAME old-encoded wire.

For manual-only GitHub Actions. NEVER treats the result as a portfolio ruling.
This file is orchestration, not an ANVIL benchmark to execute on the workstation.
"""
from __future__ import annotations

import argparse
import contextlib
import hashlib
import json
import os
import signal
import subprocess
import sys
import time
from pathlib import Path


# Independent historical A48 artifact 14480494484, run 44380412243,
# ZIP sha256 71b6b288b1a2d3f2ae0adf132287e0b1a790642e937ba9526018f8b678ea3eab
# The frozen build used source 2c5eeb9e and the IDENTICAL exact 7 flags
# supplied below. These are fingerprints of complete previous ANVIL files,
# not fabricated payload hashes or a new performance claim.
FROZEN_A48_OLD_WIRE = {
    "dickens": (2636635, "e2f188e5dc5879d622bc71278cdbb40e68baae73515379353a0fe498c0cb670d"),
    "mr": (2413023, "9cf875f7a51c910ba5772d31f7983400e8b89ee3d217f74bd1bc67101b5eed7c"),
    "mozilla": (13806173, "175c68610e213bb7af3275b7ef5cd51f008add01e2865e9a44020dc1a452443c"),
}

A70_REPS = (9, 13)
PAIRED_TIMEOUT_SECONDS = 600
CLEANUP_RESERVE_SECONDS = 15
PROCESS_TERM_GRACE_SECONDS = 3
PROCESS_KILL_GRACE_SECONDS = 3
PAIRED_RUNNER_SHA256 = "9a42bf80f6a35bbec61f191a40daf416652e1ee9c35fd7461c5de13755209b08"


def require_scout_reps(reps: int) -> None:
    if type(reps) is not int or reps not in A70_REPS:
        raise ValueError("A70 requires 9 or 13 paired repetitions")


def write_json_atomic(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    with temporary.open("w", encoding="utf-8") as f:
        json.dump(value, f, indent=2, sort_keys=True)
        f.write("\n")
        f.flush()
        os.fsync(f.fileno())
    temporary.replace(path)


class MeasurementBudgetExceeded(RuntimeError):
    def __init__(self, message: str, *, not_started: bool = False):
        super().__init__(message)
        self.not_started = not_started


class MeasurementDeadline:
    """One absolute Linux measurement deadline; cleanup has a separate reserve."""

    def __init__(self, seconds: float):
        if not 0 < seconds <= 1200:
            raise ValueError("A70 budget must be greater than zero and at most 1200 seconds")
        self.seconds = float(seconds)
        self.started = time.monotonic()
        self.expires = self.started + self.seconds
        self._armed = False
        self._previous_alarm = None

    def remaining(self) -> float:
        return self.expires - time.monotonic()

    def require(self, seconds: float, phase: str) -> None:
        remaining = self.remaining()
        if remaining < seconds + CLEANUP_RESERVE_SECONDS:
            raise MeasurementBudgetExceeded(
                f"A70 cannot start {phase}: {remaining:.3f}s remain; "
                f"requires {seconds}s plus {CLEANUP_RESERVE_SECONDS}s cleanup",
                not_started=True,
            )

    def bounded_timeout(self, maximum: float, phase: str) -> float:
        available = self.remaining() - CLEANUP_RESERVE_SECONDS
        if available <= 0:
            raise MeasurementBudgetExceeded(
                f"A70 measurement budget exhausted before {phase}", not_started=True)
        return min(float(maximum), available)

    def _expire(self, signum, frame) -> None:
        raise MeasurementBudgetExceeded("A70 absolute measurement deadline expired")

    def arm(self) -> None:
        if os.name != "posix" or not hasattr(signal, "setitimer"):
            raise RuntimeError("A70 measurement requires the hosted Linux signal deadline")
        if any(signal.getitimer(signal.ITIMER_REAL)):
            raise RuntimeError("A70 refuses to replace an existing process alarm")
        self._previous_alarm = signal.getsignal(signal.SIGALRM)
        signal.signal(signal.SIGALRM, self._expire)
        self._armed = True
        signal.setitimer(signal.ITIMER_REAL, max(self.remaining(), 0.001))

    def disarm(self) -> None:
        if self._armed:
            signal.setitimer(signal.ITIMER_REAL, 0)
            signal.signal(signal.SIGALRM, self._previous_alarm)
            self._armed = False


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def _stop_process_group(p: subprocess.Popen) -> tuple[bytes, bytes, dict]:
    cleanup = {"process_group": p.pid, "term_sent": False, "kill_sent": False}
    try:
        os.killpg(p.pid, signal.SIGTERM)
        cleanup["term_sent"] = True
    except ProcessLookupError:
        pass
    completed = False
    stdout, stderr = b"", b""
    try:
        stdout, stderr = p.communicate(timeout=PROCESS_TERM_GRACE_SECONDS)
        completed = True
    except subprocess.TimeoutExpired:
        pass
    # The leader can exit while a descendant ignores TERM. Always clean the
    # remaining group, even if communicate() already reaped the leader.
    try:
        os.killpg(p.pid, signal.SIGKILL)
        cleanup["kill_sent"] = True
    except ProcessLookupError:
        pass
    if not completed:
        stdout, stderr = p.communicate(timeout=PROCESS_KILL_GRACE_SECONDS)
    else:
        p.wait(timeout=PROCESS_KILL_GRACE_SECONDS)
    cleanup["leader_returncode"] = p.returncode
    return stdout, stderr, cleanup


def _process_evidence(prefix: Path | None, record: dict, stdout: bytes,
                      stderr: bytes) -> None:
    if prefix is None:
        return
    prefix.parent.mkdir(parents=True, exist_ok=True)
    stdout_path = Path(str(prefix) + ".stdout")
    stderr_path = Path(str(prefix) + ".stderr")
    if not record.get("streamed_logs"):
        stdout_path.write_bytes(stdout)
        stderr_path.write_bytes(stderr)
    record.update({"stdout_path": str(stdout_path), "stderr_path": str(stderr_path),
                   "stdout_bytes": len(stdout), "stderr_bytes": len(stderr)})
    write_json_atomic(Path(str(prefix) + ".process.json"), record)


def run(argv: list[str], timeout: float = 1800, *, expect_failure: bool = False,
        evidence_prefix: Path | None = None) -> subprocess.CompletedProcess[bytes]:
    """Managed direct argv; expected failure means the codec's controlled exit 1."""
    if os.name != "posix":
        raise RuntimeError("A70 managed commands require the hosted Linux process group")
    expected_returncode = 1 if expect_failure else 0
    record = {"schema": 1, "argv": argv, "timeout_seconds": timeout,
              "expected_returncode": expected_returncode, "returncode": None}
    started = time.monotonic()
    with contextlib.ExitStack() as handles:
        stdout_target = stderr_target = subprocess.PIPE
        stdout_path = stderr_path = None
        if evidence_prefix is not None:
            evidence_prefix.parent.mkdir(parents=True, exist_ok=True)
            stdout_path = Path(str(evidence_prefix) + ".stdout")
            stderr_path = Path(str(evidence_prefix) + ".stderr")
            stdout_target = handles.enter_context(stdout_path.open("wb"))
            stderr_target = handles.enter_context(stderr_path.open("wb"))
            record["streamed_logs"] = True

        def collected(stdout, stderr) -> tuple[bytes, bytes]:
            if stdout_path is not None:
                stdout_target.flush()
                stderr_target.flush()
                return stdout_path.read_bytes(), stderr_path.read_bytes()
            return stdout or b"", stderr or b""

        p = None
        try:
            p = subprocess.Popen(argv, stdout=stdout_target, stderr=stderr_target,
                                 start_new_session=True)
            record["process_group"] = p.pid
            stdout, stderr = p.communicate(timeout=timeout)
        except BaseException as exc:
            stdout = getattr(exc, "stdout", None) or b""
            stderr = getattr(exc, "stderr", None) or b""
            cleanup = None
            if p is not None:
                try:
                    stdout, stderr, cleanup = _stop_process_group(p)
                except BaseException as cleanup_exc:
                    cleanup = {"error": repr(cleanup_exc), "process_group": p.pid}
            stdout, stderr = collected(stdout, stderr)
            record.update({
                "outcome": ("SPAWN_ERROR" if p is None else "TIMEOUT"
                            if isinstance(exc, subprocess.TimeoutExpired) else "INTERRUPTED"),
                "returncode": p.returncode if p is not None else None,
                "error": repr(exc), "cleanup": cleanup,
                "elapsed_seconds": time.monotonic() - started,
            })
            _process_evidence(evidence_prefix, record, stdout, stderr)
            if isinstance(exc, subprocess.TimeoutExpired):
                raise RuntimeError(f"command timed out after {timeout}s: {argv}") from exc
            if p is None and isinstance(exc, OSError):
                raise RuntimeError(f"could not start command: {argv}") from exc
            raise
        if p.returncode != expected_returncode:
            # An abnormal leader exit must not leave a surviving descendant.
            _, _, record["cleanup"] = _stop_process_group(p)
        stdout, stderr = collected(stdout, stderr)
        record.update({"outcome": "PROCESS_EXITED", "returncode": p.returncode,
                       "elapsed_seconds": time.monotonic() - started,
                       "expected_returncode_match": p.returncode == expected_returncode})
        _process_evidence(evidence_prefix, record, stdout, stderr)
        if p.returncode != expected_returncode:
            raise RuntimeError(f"unexpected return code {p.returncode}; expected {expected_returncode}: {argv}\n"
                               + stderr.decode("utf-8", "replace")[-2000:])
        return subprocess.CompletedProcess(argv, p.returncode, stdout, stderr)


def assert_outer_truncation_guard(argv: list[str], output: Path, evidence_prefix: Path,
                                  *, timeout: float = 120,
                                  fixture: dict | None = None) -> dict:
    """Guard ANVIL's outer frame only; this does not exercise hostile BWT payloads."""
    partial = Path(str(output) + ".anvil-part")
    record = {"schema": 1, "argv": argv, "fixture": fixture,
              "expected_returncode": 1,
              "expected_stderr": "anvil: truncated block payload",
              "scope": "OUTER_FRAME_TRUNCATION_ONLY", "status": "INCOMPLETE"}
    try:
        if output.exists() or output.is_symlink() or partial.exists() or partial.is_symlink():
            record["status"] = "DIRTY_PRECONDITION"
            raise RuntimeError("outer truncation guard requires clean output paths")
        completed = run(argv, timeout=timeout, expect_failure=True,
                        evidence_prefix=evidence_prefix)
        record["returncode"] = completed.returncode
        record["stderr"] = completed.stderr.decode("utf-8", "replace")
        # Recheck here too: the guard verdict must not depend on a helper silently
        # weakening the exact return-code contract in a future refactor.
        if completed.returncode != 1:
            record["status"] = "UNEXPECTED_EXIT"
            raise RuntimeError("outer truncation guard did not observe controlled exit 1")
        diagnostic = completed.stderr
        if diagnostic.endswith(b"\r\n"):
            diagnostic = diagnostic[:-2]
        elif diagnostic.endswith(b"\n"):
            diagnostic = diagnostic[:-1]
        if diagnostic != b"anvil: truncated block payload":
            record["status"] = "WRONG_DIAGNOSTIC"
            raise RuntimeError("unexpected truncated-input rejection reason")
        if output.exists() or output.is_symlink() or partial.exists() or partial.is_symlink():
            record["status"] = "OUTPUT_LEAK"
            raise RuntimeError("truncated-input rejection left output behind")
        record["status"] = "PASS_CONTROLLED_OUTER_FRAME_REJECTION"
        return record
    except BaseException as exc:
        if record["status"] == "INCOMPLETE":
            record["status"] = "PROCESS_FAILURE"
        record["error"] = repr(exc)
        if isinstance(exc, (KeyboardInterrupt, MeasurementBudgetExceeded)):
            raise
        raise RuntimeError(f"outer truncation guard failed: {record['status']}") from exc
    finally:
        record["final_output_exists"] = output.exists() or output.is_symlink()
        record["temporary_output_exists"] = partial.exists() or partial.is_symlink()
        write_json_atomic(Path(str(evidence_prefix) + ".guard.json"), record)


def max_rss_kib(argv: list[str], output: Path, *, timeout: float = 1800,
                evidence_prefix: Path | None = None) -> int:
    """One-shot process peak in KiB, separate from every timed comparison."""
    run(["/usr/bin/time", "-f", "%M", "-o", str(output), *argv],
        timeout=timeout, evidence_prefix=evidence_prefix)
    try:
        value = int(output.read_text(encoding="utf-8").strip())
    except (OSError, ValueError) as exc:
        raise RuntimeError(f"missing GNU time RSS observation: {output}") from exc
    if value <= 0:
        raise RuntimeError(f"nonsensical RSS observation {value} from {output}")
    return value


def require_safe_observation_paths(outputs: list[Path], protected: list[Path]) -> None:
    resolved = [path.resolve() for path in outputs]
    if len(resolved) != len(set(resolved)):
        raise ValueError("decoder observation outputs must be distinct")
    protected_paths = {path.resolve() for path in protected}
    for path, absolute in zip(outputs, resolved):
        if absolute in protected_paths or path.is_symlink() or path.is_dir():
            raise ValueError("decoder observation output collides with protected input or source")


def paired(tools: Path, control: list[str], candidate: list[str],
           control_output: Path, candidate_output: Path,
           source_hash: str, label: str, out: Path, reps: int,
           deadline: MeasurementDeadline | None = None) -> dict:
    require_scout_reps(reps)
    if deadline is not None:
        deadline.require(PAIRED_TIMEOUT_SECONDS, label)
    summary = out / f"paired-{label}.json"
    raw = out / f"paired-{label}.csv"
    journal = out / f"paired-{label}.events.jsonl"
    state = out / f"paired-{label}.state.json"
    # Hosted runners may expose an affinity mask NOT containing logical CPU 0.
    # Choose the first process-allowed CPU, then pin every paired arm equally.
    if not hasattr(os, "sched_getaffinity"):
        raise RuntimeError("A70 hosted paired scout requires Linux affinity support")
    allowed = sorted(os.sched_getaffinity(0))
    if not allowed:
        raise RuntimeError("A70 runner has no permitted CPU affinity set")
    selected_cpu = allowed[0]
    argv = [
        sys.executable, "-u", str(tools / "a70_paired_capture.py"),
        "--runner", str(tools / "paired_bench.py"),
        "--runner-sha256", PAIRED_RUNNER_SHA256,
        "--journal", str(journal), "--state", str(state), "--",
        "--control-json", json.dumps(control),
        "--candidate-json", json.dumps(candidate),
        "--reps", str(reps), "--warmups", "2",
        "--bootstrap", "20000", "--seed", "41246",
        "--epsilon", "0.02", "--max-cv", "0.15",
        "--ambient-reps", "5", "--ambient-iterations", "250000",
        "--ambient-max-cv", "0.10", "--cpu", str(selected_cpu),
        "--control-observe", str(control_output),
        "--candidate-observe", str(candidate_output),
        "--expected-sha256", source_hash,
        "--out", str(raw), "--summary", str(summary),
    ]
    run(argv, timeout=PAIRED_TIMEOUT_SECONDS,
        evidence_prefix=out / f"paired-{label}-runner")
    capture = json.loads(state.read_text(encoding="utf-8"))
    if capture.get("status") != "COMPLETE" or capture.get("complete") is not True:
        raise RuntimeError(f"paired capture did not complete: {label}")
    with journal.open(encoding="utf-8") as f:
        events = [json.loads(line) for line in f if line.strip()]
    if (not events or events[0].get("event") != "INIT"
            or any(event.get("seq") != i + 1 for i, event in enumerate(events))):
        raise RuntimeError(f"paired capture journal is incomplete: {label}")
    terminal = events[-1]
    verified = [event for event in events if event.get("event") == "VERIFIED"]
    if (terminal.get("event") != "COMPLETE"
            or terminal.get("seq") != capture.get("last_event_seq")
            or capture.get("runner_sha256") != PAIRED_RUNNER_SHA256
            or capture.get("requested_reps") != reps
            or capture.get("expected_verified_arms") != 2 * reps
            or capture.get("verified_arms") != 2 * reps
            or capture.get("complete_pairs") != list(range(reps))
            or terminal.get("verified_arms") != 2 * reps
            or terminal.get("complete_pairs") != list(range(reps))
            or len(verified) != 2 * reps
            or {(event.get("pair"), event.get("arm")) for event in verified}
               != {(i, arm) for i in range(reps) for arm in ("control", "candidate")}):
        raise RuntimeError(f"paired capture completion identities disagree: {label}")
    d = json.loads(summary.read_text(encoding="utf-8"))
    # Fail closed if the timed runs were not actually observed with the
    # canonical source hash AFTER their subprocess clock stopped.
    for arm in ("control", "candidate"):
        observation = d.get("observed_outputs", {}).get(arm)
        if not isinstance(observation, dict):
            raise RuntimeError(f"no timed-output observations for {label}/{arm}")
        if observation.get("sha256") != source_hash or not observation.get("deterministic"):
            raise RuntimeError(f"not byte-exact in {label}/{arm}")
    if len(d.get("pairs", [])) != reps:
        raise RuntimeError(f"raw paired repetition coverage incomplete: {label}")
    d["peer_affinity_preflight"] = {
        "selected_cpu": selected_cpu,
        "parent_allowed_cpus": allowed,
    }
    d["capture_evidence"] = {"journal": str(journal), "state": str(state)}
    return d


def inspect_ratio_wire(path: Path, expected_source_bytes: int) -> dict:
    """Read only frame headers and routed backend tags from ANVIL revision 2.

    Source contract: src/anvil.cpp compress() writes
      ANV0, revision=2, uvar(block_capacity), uvar(original_bytes),
      repeated [uvar(block_bytes), mode, uvar(payload_bytes), crc32_le,
                payload];
    ratio mode 17 payload begins [transform_id, backend_id,
                                  uvar(transformed_bytes), encoded_stream].
    No entropy decoding or codec execution is performed here.
    """
    size = path.stat().st_size
    if size < 8:
        raise RuntimeError("A70 invalid ANVIL frame: too short")

    with path.open("rb") as f:
        def read_exact(n: int) -> bytes:
            b = f.read(n)
            if len(b) != n:
                raise RuntimeError("A70 truncated ANVIL header")
            return b

        def uvar(bound_end: int | None = None) -> int:
            value = 0
            for i in range(10):
                if bound_end is not None and f.tell() >= bound_end:
                    raise RuntimeError("A70 ratio varint crosses declared payload boundary")
                b = read_exact(1)[0]
                if i == 9 and b > 1:
                    raise RuntimeError("A70 invalid overflowing uvar")
                value |= (b & 127) << (i * 7)
                if not (b & 128):
                    if i and not (b & 127):
                        raise RuntimeError("A70 noncanonical uvar from old encoder")
                    return value
            raise RuntimeError("A70 invalid 10-byte uvar")

        if read_exact(5) != b"ANV0\x02":
            raise RuntimeError("A70 expected canonical revision-2 ANVIL wire")
        block_capacity = uvar()
        declared_source_bytes = uvar()
        if not (0 < block_capacity <= 128 << 20):
            raise RuntimeError("A70 invalid revision-2 block capacity")
        if declared_source_bytes != expected_source_bytes:
            raise RuntimeError("A70 packed source length disagrees with frozen corpus")
        blocks: list[dict] = []
        consumed = 0
        while consumed < declared_source_bytes:
            block_len = uvar()
            if block_len == 0 or block_len > block_capacity:
                raise RuntimeError("A70 invalid wire block length")
            if consumed + block_len > declared_source_bytes:
                raise RuntimeError("A70 block exceeds original source")
            mode = read_exact(1)[0]
            payload_len = uvar()
            read_exact(4)  # existing per-block CRC32, decoder verifies it
            start = f.tell()
            end = start + payload_len
            if end > size:
                raise RuntimeError("A70 payload exceeds compressed file")
            item = {"source_bytes": block_len, "payload_bytes": payload_len,
                    "mode": mode}
            if mode == 17:
                if payload_len < 3:
                    raise RuntimeError("A70 truncated ratio-mode payload")
                transform, backend = read_exact(2)
                if transform > 2 or backend not in (1, 2):
                    raise RuntimeError("A70 invalid transform/backend identifier")
                transformed = uvar(bound_end=end)
                if transformed > 2 * block_len + 4096:
                    raise RuntimeError("A70 invalid transformed payload length")
                item.update({
                    "transform": transform,
                    "backend": "brotli" if backend == 1 else "bwt",
                    "transformed_bytes": transformed,
                })
            elif mode == 0:
                if payload_len != block_len:
                    raise RuntimeError("A70 invalid raw block payload length")
                item["backend"] = "raw"
            else:
                raise RuntimeError(f"A70 unexpected revision-2 block mode {mode}")
            f.seek(end)
            consumed += block_len
            blocks.append(item)
        if f.tell() != size:
            raise RuntimeError("A70 trailing bytes after complete original frame")
        counts = {"brotli": 0, "bwt": 0, "raw": 0}
        for b in blocks:
            counts[b["backend"]] += 1
        return {
            "revision": 2, "declared_source_bytes": declared_source_bytes,
            "block_capacity": block_capacity, "block_count": len(blocks),
            "backend_blocks": counts, "blocks": blocks,
        }


def audit_frozen_old_wire(path: Path, name: str) -> dict:
    """Cross-run provenance only; same-job causal old/new decoding is primary.

    A historical SHA mismatch is NOT a libsais H0 or a hard gate when the
    control and candidate decode the identical CURRENT old-encoded file.
    The current same-job route and output hashes are separate hard gates.
    """
    if name not in FROZEN_A48_OLD_WIRE:
        raise ValueError(f"no frozen A48 old-wire fingerprint for {name}")
    expected_bytes, expected_sha = FROZEN_A48_OLD_WIRE[name]
    actual_bytes, actual_sha = path.stat().st_size, sha256(path)
    matched = (actual_bytes == expected_bytes and actual_sha == expected_sha)
    if not matched:
        print(
            f"A70 HISTORICAL_WIRE_DRIFT {name}: new old-encoder wire "
            f"{actual_bytes} bytes sha256={actual_sha}, historical A48 "
            f"{expected_bytes} bytes sha256={expected_sha}. "
            f"Old/new decoders still compare the SAME new wire; do NOT "
            f"attribute cross-run changes or claim A48-replicated bytes.",
            file=sys.stderr,
            flush=True,
        )
    return {
        "run_id": 44380412243,
        "artifact_id": 14480494484,
        "expected_bytes": expected_bytes,
        "expected_sha256": expected_sha,
        "actual_bytes": actual_bytes,
        "actual_sha256": actual_sha,
        "matches_byte_for_byte": matched,
        "cross_run_comparability": (
            "EXACT_HISTORICAL_REPLAY" if matched
            else "DRIFT_DIAGNOSTIC_NO_CROSS_RUN_COMPARISON"
        ),
    }


def require_preregistered_route(wire: dict, name: str) -> str:
    expected_routes = {"dickens": "bwt", "mr": "bwt", "mozilla": "brotli"}
    if name not in expected_routes:
        raise ValueError(
            f"A70 scout only preregisters {sorted(expected_routes)}; "
            f"{name} requires a new source-backed route hypothesis"
        )
    expected = expected_routes[name]
    observed = set(b["backend"] for b in wire["blocks"])
    if observed != {expected}:
        raise RuntimeError(
            f"A70 ROUTING_DRIFT {name}: expected only {expected}, "
            f"actual={sorted(observed)}. No libsais speed claim is valid."
        )
    return expected


def run_scout(a: argparse.Namespace) -> dict:
    require_scout_reps(a.reps)
    if (not a.files or len(a.files) != len(set(a.files)) or len(a.files) > 3
            or not set(a.files) <= {"dickens", "mr", "mozilla"}):
        raise ValueError("A70 requires 1-3 distinct preregistered scout inputs")
    if not a.test_truncation:
        raise ValueError("A70 requires the outer-frame --test-truncation guard")
    deadline = MeasurementDeadline(a.budget_seconds)
    a.out.mkdir(parents=True, exist_ok=True)
    for name in ("scout.json", "scout-partial.json", "scout-failure.json"):
        path = a.out / name
        if path.exists() or path.is_symlink():
            raise RuntimeError(f"A70 requires fresh measurement state: {path}")
    manifest = {
        "schema": 2, "status": "RUNNING", "timing_valid": False,
        "scope": "A70 serial libsais version-only scout, NOT full Silesia/heldout Pareto",
        "binary_size_note": "FULL anvil CLI, NOT decoder-only binary footprint",
        "source": "Silesia canonical MD5 from ANVIL gha_fetch_corpus.py",
        "requested_files": list(a.files), "requested_reps": a.reps,
        "budget_seconds": deadline.seconds,
        "research_decision": "INCOMPLETE_NO_SPEED_CLAIM",
        "current_file": None, "current_phase": "initializing", "active_file": None,
        "files": [],
    }

    def checkpoint(phase: str) -> None:
        manifest["current_phase"] = phase
        manifest["elapsed_seconds"] = time.monotonic() - deadline.started
        manifest["remaining_seconds"] = deadline.remaining()
        write_json_atomic(a.out / "scout-partial.json", manifest)

    try:
        deadline.arm()
        checkpoint("binary_provenance")
        manifest.update({
            "old_binary_sha256": sha256(a.old), "new_binary_sha256": sha256(a.candidate),
            "old_binary_bytes": a.old.stat().st_size,
            "new_binary_bytes": a.candidate.stat().st_size,
        })
        _run_scout_files(a, deadline, manifest, checkpoint)
        if len(manifest["files"]) != len(a.files):
            raise RuntimeError("A70 cannot complete with missing requested inputs")
        manifest["status"] = "COMPLETE"
        manifest["timing_valid"] = all(r["timing_valid"] for r in manifest["files"])
        manifest["research_decision"] = (
            "SCOUT_ONLY_NOT_FULL_PARETO_NO_VERSION_CLAIM"
            if manifest["timing_valid"] else "INCONCLUSIVE_NO_SPEED_CLAIM")
        manifest["current_file"] = None
        manifest["active_file"] = None
        checkpoint("complete")
        write_json_atomic(a.out / "scout.json", manifest)
        return manifest
    except BaseException as exc:
        # Stop the alarm before writing failure evidence; cleanup/upload owns
        # the external reserve, never additional measurements or partial claims.
        deadline.disarm()
        failure = {"type": type(exc).__name__, "message": str(exc),
                   "file": manifest["current_file"], "phase": manifest["current_phase"]}
        if isinstance(exc, MeasurementBudgetExceeded):
            failure["classification"] = (
                "NOT_STARTED_BUDGET" if exc.not_started else "MEASUREMENT_BUDGET_EXCEEDED")
        manifest.update(status="INCOMPLETE", timing_valid=False,
                        research_decision="INCOMPLETE_NO_SPEED_CLAIM", failure=failure)
        checkpoint(manifest["current_phase"])
        write_json_atomic(a.out / "scout-failure.json", manifest)
        raise
    finally:
        deadline.disarm()


def _run_scout_files(a: argparse.Namespace, deadline: MeasurementDeadline,
                     manifest: dict, checkpoint) -> None:
    results = a.out / "results"
    results.mkdir(exist_ok=True)
    work = a.out / "work"
    work.mkdir(exist_ok=True)

    # The trusted ANVIL source manifest defines exact Silesia files and MD5s.
    sys.path.insert(0, str(a.tools.resolve()))
    from gha_fetch_corpus import SILESIA, SILESIA_BASE, download, extract_single, verify

    for name in a.files:
        manifest["current_file"] = name
        active = {"file": name, "completed_sessions": {}}
        manifest["active_file"] = active
        if name not in SILESIA:
            raise ValueError(f"non-canonical Silesia input: {name}")
        size, md5 = SILESIA[name]
        filedir = work / name
        filedir.mkdir(exist_ok=True)
        src = filedir / name
        archive = filedir / (name + ".zip")
        packed = filedir / "old.anv"
        oldout, newout = filedir / "old.out", filedir / "new.out"
        require_safe_observation_paths([oldout, newout], [src, packed, a.old, a.candidate])
        checkpoint("corpus_fetch")
        download(SILESIA_BASE.format(name=name), archive)
        extract_single(archive, name, src)
        verify(src, size, md5)
        archive.unlink()
        source_hash = sha256(src)
        active.update(input_bytes=size, input_sha256=source_hash)

        # Critical experimental isolation: encode ONCE with OLD version.
        # Both decoder arms consume exactly the same immutable packed stream.
        checkpoint("old_encode")
        run([str(a.old), "c", str(src), str(packed),
             "--parse=ratio", "--ratio-backend=auto",
             "--ratio-context=off", "--ratio-lines=off", "--bwt-aux=on",
             "--bwt-subblock=8388608", "--quiet"],
            timeout=deadline.bounded_timeout(600, "old_encode"),
            evidence_prefix=results / f"{name}-old-encode")

        checkpoint("wire_provenance")
        wire = inspect_ratio_wire(packed, size)
        # Independent A48 cross-run fingerprint: diagnostic, NOT a false
        # causal hard gate. The actual source and backend are still checked,
        # and BOTH decoders consume exactly the same freshly encoded wire.
        replay_evidence = audit_frozen_old_wire(packed, name)
        # Route provenance MUST precede any speed observation. A libsais
        # version-only test on a Brotli-routed file is a negative control,
        # never evidence that the inverse-BWT implementation got faster.
        expected_backend = require_preregistered_route(wire, name)
        packed_hash = sha256(packed)
        active.update(packed_bytes=packed.stat().st_size, packed_sha256=packed_hash,
                      old_wire_backend=expected_backend, old_wire_route_provenance=wire,
                      frozen_a48_replay=replay_evidence)

        oldcmd = [str(a.old), "d", str(packed), str(oldout), "--quiet"]
        newcmd = [str(a.candidate), "d", str(packed), str(newout), "--quiet"]
        checkpoint("old_decode_compatibility")
        run(oldcmd, timeout=deadline.bounded_timeout(1800, "old_decode_compatibility"),
            evidence_prefix=results / f"{name}-old-compatibility")
        checkpoint("new_decode_compatibility")
        run(newcmd, timeout=deadline.bounded_timeout(1800, "new_decode_compatibility"),
            evidence_prefix=results / f"{name}-new-compatibility")
        if sha256(oldout) != source_hash or sha256(newout) != source_hash:
            raise RuntimeError(f"old-wire decode compatibility FAIL: {name}")

        # Separate process-level peak-RSS readings OUTSIDE the paired clocks.
        # This is a one-shot scout metric, NOT the full reproducible RSS gate.
        checkpoint("old_decode_rss")
        old_rss_kib = max_rss_kib(
            oldcmd, results / f"{name}-old-rss.txt",
            timeout=deadline.bounded_timeout(1800, "old_decode_rss"),
            evidence_prefix=results / f"{name}-old-rss-runner")
        active["old_decode_peak_rss_kib_one_shot"] = old_rss_kib
        checkpoint("new_decode_rss")
        new_rss_kib = max_rss_kib(
            newcmd, results / f"{name}-new-rss.txt",
            timeout=deadline.bounded_timeout(1800, "new_decode_rss"),
            evidence_prefix=results / f"{name}-new-rss-runner")
        active["new_decode_peak_rss_kib_one_shot"] = new_rss_kib
        if sha256(oldout) != source_hash or sha256(newout) != source_hash:
            raise RuntimeError(f"RSS measured command produced corrupt bytes: {name}")

        # One bounded OUTER-FRAME guard; this is not hostile libsais input/fuzz.
        guards = {}
        if a.test_truncation:
            checkpoint("outer_truncation_fixture")
            truncated = filedir / "truncated.anv"
            with packed.open("rb") as f, truncated.open("wb") as g:
                n = packed.stat().st_size - 1
                while n > 0:
                    chunk = f.read(min(n, 1 << 20))
                    if not chunk:
                        raise RuntimeError("short packed read")
                    g.write(chunk)
                    n -= len(chunk)
            fixture = {"mutation": "remove_exactly_one_final_byte",
                       "original_bytes": packed.stat().st_size, "original_sha256": packed_hash,
                       "malformed_bytes": truncated.stat().st_size,
                       "malformed_sha256": sha256(truncated)}
            active["outer_truncation_fixture"] = fixture
            for arm, binary in (("old", a.old), ("new", a.candidate)):
                checkpoint(f"{arm}_outer_truncation_guard")
                output = filedir / f"{arm}-trunc.out"
                guards[arm] = assert_outer_truncation_guard(
                    [str(binary), "d", str(truncated), str(output), "--quiet"],
                    output, results / f"{name}-{arm}-truncation",
                    timeout=deadline.bounded_timeout(120, f"{arm}_outer_truncation_guard"),
                    fixture=fixture)
                active["outer_truncation_guards"] = guards

        # Bracket the old-vs-new paired experiment with TWO A/A null sessions.
        # A later-only null misses drift in the setup interval before comparison.
        sessions = {}
        for phase, control, candidate, control_output, candidate_output in (
            ("old-null-pre", oldcmd, oldcmd, oldout, oldout),
            ("old-vs-new", oldcmd, newcmd, oldout, newout),
            ("old-null-post", oldcmd, oldcmd, oldout, oldout),
        ):
            checkpoint(phase)
            label = f"{name}-{phase}"
            sessions[phase] = paired(
                a.tools, control, candidate, control_output, candidate_output,
                source_hash, label, results, a.reps, deadline=deadline)
            active["completed_sessions"][phase] = {
                "summary": str(results / f"paired-{label}.json"),
                "capture": sessions[phase]["capture_evidence"],
                "timing_valid": sessions[phase].get("timing_valid"),
            }
            checkpoint(phase + "-complete")
        null_pre = sessions["old-null-pre"]
        comparison = sessions["old-vs-new"]
        null_post = sessions["old-null-post"]
        if sha256(packed) != packed_hash:
            raise RuntimeError(f"old compressed stream changed during measurement: {name}")

        def valid_null(d: dict) -> bool:
            ci = d.get("paired_ratio_ci95", [])
            return (len(ci) == 2 and
                    float(ci[0]) <= 1.0 <= float(ci[1]) and
                    d.get("timing_valid") is True)

        null_valid = valid_null(null_pre) and valid_null(null_post)
        good = bool(comparison.get("timing_valid") and null_valid)
        record = {
            "file": name, "input_bytes": size, "input_sha256": source_hash,
            "packed_bytes": packed.stat().st_size, "packed_sha256": packed_hash,
            "old_wire_backend": expected_backend,
            "old_wire_route_provenance": wire,
            "frozen_a48_replay": replay_evidence,
            "outer_truncation_guards": guards,
            "completed_sessions": active["completed_sessions"],
            "old_decode_peak_rss_kib_one_shot": old_rss_kib,
            "new_decode_peak_rss_kib_one_shot": new_rss_kib,
            "rss_note": "one-shot per-process GNU time KiB outside paired clocks; NOT full RSS gate",
            "same_old_packed_sha_both_arms": True,
            "old_decode_sha256": source_hash, "new_decode_sha256": source_hash,
            "paired_candidate_over_control": comparison.get("paired_ratio_candidate_over_control"),
            "paired_ci95": comparison.get("paired_ratio_ci95"),
            "null_pre_ci95": null_pre.get("paired_ratio_ci95"),
            "null_post_ci95": null_post.get("paired_ratio_ci95"),
            "timing_valid": good,
            "status": "SCOUT_TIMING_VALID" if good else "INCONCLUSIVE_TIMING",
        }
        manifest["files"].append(record)
        # Store durable complete per-file evidence before the next input.
        checkpoint("file_complete")
        print(json.dumps(record, sort_keys=True), flush=True)
        # Retain old compressed stream as immutable replay evidence (~19MB total
        # for the three scout inputs); it is small compared with A48's 427MB
        # regenerable-work artifact. Originals are source-pinned by MD5/SHA.
        for p in (src, oldout, newout):
            p.unlink(missing_ok=True)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--old", type=Path, required=True)
    ap.add_argument("--candidate", type=Path, required=True)
    ap.add_argument("--tools", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--files", nargs="+", default=["dickens", "mr", "mozilla"])
    ap.add_argument("--reps", type=int, choices=A70_REPS, default=9)
    ap.add_argument("--budget-seconds", type=float, default=1200)
    ap.add_argument("--test-truncation", action="store_true")
    a = ap.parse_args()
    if len(a.files) != len(set(a.files)) or len(a.files) > 3 or not a.files:
        ap.error("scout file set must have 1-3 distinct Silesia filenames")
    if not 0 < a.budget_seconds <= 1200:
        ap.error("measurement budget must be greater than zero and at most 1200 seconds")

    def interrupted(signum, frame):
        raise RuntimeError(f"A70 interrupted by signal {signum}")

    previous_term = signal.signal(signal.SIGTERM, interrupted)
    try:
        result = run_scout(a)
        print(result["research_decision"])
        return 0
    finally:
        signal.signal(signal.SIGTERM, previous_term)


if __name__ == "__main__":
    raise SystemExit(main())
