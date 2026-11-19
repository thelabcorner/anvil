#!/usr/bin/env python3
"""A70 scout: pinned v2.10.4 vs v2.12.2 decoder on SAME old-encoded wire.

For manual-only GitHub Actions. NEVER treats the result as a portfolio ruling.
This file is orchestration, not an ANVIL benchmark to execute on the workstation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def run(argv: list[str], timeout: int = 1800, *, expect_failure: bool = False) -> None:
    p = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       timeout=timeout, check=False)
    failed = p.returncode != 0
    if failed != expect_failure:
        raise RuntimeError(f"unexpected return code {p.returncode}: {argv}\n"
                           + p.stderr.decode("utf-8", "replace")[-2000:])


def max_rss_kib(argv: list[str], output: Path) -> int:
    """One-shot process peak in KiB, separate from every timed comparison."""
    run(["/usr/bin/time", "-f", "%M", "-o", str(output), *argv])
    try:
        value = int(output.read_text(encoding="utf-8").strip())
    except (OSError, ValueError) as exc:
        raise RuntimeError(f"missing GNU time RSS observation: {output}") from exc
    if value <= 0:
        raise RuntimeError(f"nonsensical RSS observation {value} from {output}")
    return value


def paired(tools: Path, control: list[str], candidate: list[str],
           control_output: Path, candidate_output: Path,
           source_hash: str, label: str, out: Path, reps: int) -> dict:
    summary = out / f"paired-{label}.json"
    raw = out / f"paired-{label}.csv"
    argv = [
        sys.executable, str(tools / "paired_bench.py"),
        "--control-json", json.dumps(control),
        "--candidate-json", json.dumps(candidate),
        "--reps", str(reps), "--warmups", "2",
        "--bootstrap", "20000", "--seed", "41246",
        "--epsilon", "0.02", "--max-cv", "0.15",
        "--ambient-reps", "5", "--ambient-iterations", "250000",
        "--ambient-max-cv", "0.10", "--cpu", "0",
        "--control-observe", str(control_output),
        "--candidate-observe", str(candidate_output),
        "--expected-sha256", source_hash,
        "--out", str(raw), "--summary", str(summary),
    ]
    run(argv)
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
    return d


def run_scout(a: argparse.Namespace) -> dict:
    a.out.mkdir(parents=True, exist_ok=True)
    results = a.out / "results"
    results.mkdir(exist_ok=True)
    work = a.out / "work"
    work.mkdir(exist_ok=True)

    # The trusted ANVIL source manifest defines exact Silesia files and MD5s.
    sys.path.insert(0, str(a.tools.resolve()))
    from gha_fetch_corpus import SILESIA, SILESIA_BASE, download, extract_single, verify

    manifest = {
        "schema": 1,
        "scope": "A70 serial libsais version-only scout, NOT full Silesia/heldout Pareto",
        "old_binary_sha256": sha256(a.old),
        "new_binary_sha256": sha256(a.candidate),
        "old_binary_bytes": a.old.stat().st_size,
        "new_binary_bytes": a.candidate.stat().st_size,
        "binary_size_note": "FULL anvil CLI, NOT decoder-only binary footprint",
        "source": "Silesia canonical MD5 from ANVIL gha_fetch_corpus.py",
        "files": [],
    }
    for name in a.files:
        if name not in SILESIA:
            raise ValueError(f"non-canonical Silesia input: {name}")
        size, md5 = SILESIA[name]
        filedir = work / name
        filedir.mkdir(exist_ok=True)
        src = filedir / name
        archive = filedir / (name + ".zip")
        download(SILESIA_BASE.format(name=name), archive)
        extract_single(archive, name, src)
        verify(src, size, md5)
        archive.unlink()
        source_hash = sha256(src)

        # Critical experimental isolation: encode ONCE with OLD version.
        # Both decoder arms consume exactly the same immutable packed stream.
        packed = filedir / "old.anv"
        run([str(a.old), "c", str(src), str(packed),
             "--parse=ratio", "--ratio-backend=auto",
             "--ratio-context=off", "--ratio-lines=off", "--bwt-aux=on",
             "--bwt-subblock=8388608", "--quiet"])

        oldout, newout = filedir / "old.out", filedir / "new.out"
        oldcmd = [str(a.old), "d", str(packed), str(oldout), "--quiet"]
        newcmd = [str(a.candidate), "d", str(packed), str(newout), "--quiet"]
        run(oldcmd)
        run(newcmd)
        if sha256(oldout) != source_hash or sha256(newout) != source_hash:
            raise RuntimeError(f"old-wire decode compatibility FAIL: {name}")

        # Separate process-level peak-RSS readings OUTSIDE the paired clocks.
        # This is a one-shot scout metric, NOT the full reproducible RSS gate.
        old_rss_kib = max_rss_kib(oldcmd, results / f"{name}-old-rss.txt")
        new_rss_kib = max_rss_kib(newcmd, results / f"{name}-new-rss.txt")
        if sha256(oldout) != source_hash or sha256(newout) != source_hash:
            raise RuntimeError(f"RSS measured command produced corrupt bytes: {name}")

        # One bounded malformed-input guard, not a substitute for full fuzz.
        if a.test_truncation:
            truncated = filedir / "truncated.anv"
            with packed.open("rb") as f, truncated.open("wb") as g:
                n = packed.stat().st_size - 1
                while n > 0:
                    chunk = f.read(min(n, 1 << 20))
                    if not chunk:
                        raise RuntimeError("short packed read")
                    g.write(chunk)
                    n -= len(chunk)
            run([str(a.old), "d", str(truncated),
                 str(filedir / "old-trunc.out"), "--quiet"],
                timeout=120, expect_failure=True)
            run([str(a.candidate), "d", str(truncated),
                 str(filedir / "new-trunc.out"), "--quiet"],
                timeout=120, expect_failure=True)

        comparison = paired(a.tools, oldcmd, newcmd, oldout, newout,
                            source_hash, f"{name}-old-vs-new", results, a.reps)
        null = paired(a.tools, oldcmd, oldcmd, oldout, oldout,
                      source_hash, f"{name}-old-null", results, a.reps)

        null_ci = null.get("paired_ratio_ci95", [])
        null_valid = (len(null_ci) == 2 and float(null_ci[0]) <= 1.0 <= float(null_ci[1]))
        good = bool(comparison.get("timing_valid") and null.get("timing_valid") and null_valid)
        record = {
            "file": name, "input_bytes": size, "input_sha256": source_hash,
            "packed_bytes": packed.stat().st_size, "packed_sha256": sha256(packed),
            "old_decode_peak_rss_kib_one_shot": old_rss_kib,
            "new_decode_peak_rss_kib_one_shot": new_rss_kib,
            "rss_note": "one-shot per-process GNU time KiB outside paired clocks; NOT full RSS gate",
            "same_old_packed_sha_both_arms": True,
            "old_decode_sha256": source_hash, "new_decode_sha256": source_hash,
            "paired_candidate_over_control": comparison.get("paired_ratio_candidate_over_control"),
            "paired_ci95": comparison.get("paired_ratio_ci95"),
            "null_ci95": null_ci,
            "timing_valid": good,
            "status": "SCOUT_TIMING_VALID" if good else "INCONCLUSIVE_TIMING",
        }
        manifest["files"].append(record)
        # Store durable complete per-file evidence before the next input.
        (a.out / "scout-partial.json").write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps(record, sort_keys=True), flush=True)
        # Reproducible raw input and pack bytes are not needed in the artifact:
        # exact hashes, paths, full sizes and paired repetitions are preserved.
        for p in (src, packed, oldout, newout):
            p.unlink(missing_ok=True)
    manifest["research_decision"] = (
        "SCOUT_ONLY_NOT_FULL_PARETO_NO_VERSION_CLAIM"
        if all(r["timing_valid"] for r in manifest["files"])
        else "INCONCLUSIVE_NO_SPEED_CLAIM")
    (a.out / "scout.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return manifest


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--old", type=Path, required=True)
    ap.add_argument("--candidate", type=Path, required=True)
    ap.add_argument("--tools", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--files", nargs="+", default=["dickens", "mr", "mozilla"])
    ap.add_argument("--reps", type=int, choices=[7, 9, 13], default=9)
    ap.add_argument("--test-truncation", action="store_true")
    a = ap.parse_args()
    if len(a.files) != len(set(a.files)) or len(a.files) > 3 or not a.files:
        ap.error("scout file set must have 1-3 distinct Silesia filenames")
    result = run_scout(a)
    print(result["research_decision"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
