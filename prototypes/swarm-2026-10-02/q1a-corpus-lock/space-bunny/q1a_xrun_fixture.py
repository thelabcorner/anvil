#!/usr/bin/env python3
"""Emit the deterministic Q1a synthetic fixture artifact set for cross-runner attestation.

This is deliberately NOT a corpus audit:
- no network
- no codec
- no archive decompression
- no sealed bytes
- synthetic fixtures only

The artifact-manifest.json SHA-256 is the cross-runner reproducibility anchor.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import q1a_corpus_lock as q


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--summary", type=Path)
    args = ap.parse_args()

    lock = q.fixture_lock()
    tomb = q.fixture_tombstone()
    guard = q.ExposureGuard()
    report = q.audit(
        lock,
        tomb,
        q.fixture_byte_sources(),
        guard,
        q._read_own_source(),
    )
    manifest = q.emit_artifacts(report, str(args.out))
    manifest_path = args.out / "artifact-manifest.json"

    summary = {
        "schema": "anvil.q1a-xrun-fixture/v1",
        "tool_sha256": report["tool"]["source_sha256"],
        "lock_sha256": report["lock"]["lock_sha256"],
        "verdict": report["verdict"]["status"],
        "audit_complete": report["audit_complete"],
        "graph_verification": report["verdict"]["graph_verification"],
        "independence_units_authoritative_for_promotion": report["verdict"][
            "independence_units_authoritative_for_promotion"
        ],
        "independence_units": report["independence"].get("independence_units"),
        "conflict_edges": len(report["conflict_edges"]),
        "not_evaluated": len(report["not_evaluated"]),
        "manifest_artifact_count": manifest["aggregate_of_artifacts"]["artifact_count"],
        "artifact_manifest_sha256": sha256_file(manifest_path),
        "zero_network": report["counters"]["zero_network"],
        "zero_codec": report["counters"]["zero_codec"],
        "zero_archive_decompressions": report["counters"]["zero_archive_decompression"],
        "zero_sealed_bytes": report["counters"]["zero_sealed_bytes"],
    }
    data = q.canon(summary)
    if args.summary:
        args.summary.parent.mkdir(parents=True, exist_ok=True)
        args.summary.write_bytes(data)
    print(data.decode("utf-8"), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
