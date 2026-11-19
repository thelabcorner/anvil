#!/usr/bin/env python3
"""Read-only corpus provenance gate; no compression or benchmarking.

Same-generator corpora remain valid stress tests, but they are NOT independent
holdouts. Different declared families are not proof of independence either.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

SCHEMA = 1


def git_blob_sha1(path: Path) -> str:
    size = path.stat().st_size
    digest = hashlib.sha1(b"blob " + str(size).encode("ascii") + b"\0")
    with path.open("rb") as stream:
        for data in iter(lambda: stream.read(1 << 20), b""):
            digest.update(data)
    return digest.hexdigest()


def checked_path(root: Path, name: str) -> Path:
    """Read only repository-relative files, never outside the intended root."""
    if not name or Path(name).is_absolute():
        raise ValueError("absolute or empty provenance path is forbidden")
    resolved = (root / name).resolve(strict=True)
    try:
        resolved.relative_to(root.resolve(strict=True))
    except ValueError as exc:
        raise ValueError(f"path escapes repository root: {name}") from exc
    if not resolved.is_file():
        raise ValueError(f"not a regular file: {name}")
    return resolved


def evaluate(manifest: dict, root: Path, discovery: list[str], validation: list[str]) -> dict:
    errors: list[str] = []
    if manifest.get("schema") != SCHEMA:
        errors.append(f"provenance schema must be {SCHEMA}")
    records = manifest.get("files")
    if not isinstance(records, list):
        records = []
        errors.append("files must be a list")
    by_name = {}
    for rec in records:
        if not isinstance(rec, dict):
            errors.append("nonobject file record")
            continue
        name = rec.get("path", "")
        if not isinstance(name, str) or not name:
            errors.append("missing path in provenance record")
            continue
        if name in by_name:
            errors.append(f"duplicate path in manifest: {name}")
            continue
        by_name[name] = rec
        fam = rec.get("generator_family")
        if not isinstance(fam, str) or not fam.strip():
            errors.append(f"missing generator_family for {name}")
        if not isinstance(rec.get("git_blob_sha1"), str) or len(rec["git_blob_sha1"]) != 40:
            errors.append(f"missing/invalid git_blob_sha1 for {name}")
    wanted = list(dict.fromkeys(discovery + validation))
    if not discovery or not validation:
        errors.append("both discovery and validation file sets are required")
    if len(discovery) != len(set(discovery)) or len(validation) != len(set(validation)):
        errors.append("duplicate filenames in discovery/validation")
    if set(discovery) & set(validation):
        errors.append("same file occurs in discovery and validation")
    for name in wanted:
        rec = by_name.get(name)
        if rec is None:
            errors.append(f"path has no frozen provenance declaration: {name}")
            continue
        try:
            actual = git_blob_sha1(checked_path(root, name))
            if actual != rec.get("git_blob_sha1"):
                errors.append(f"content ID mismatch for {name}: {actual} != {rec.get('git_blob_sha1')}")
        except (OSError, ValueError) as exc:
            errors.append(f"cannot verify {name}: {exc}")
        gen = rec.get("generator_path")
        gen_sha = rec.get("generator_git_blob_sha1")
        if gen is not None or gen_sha is not None:
            if not isinstance(gen, str) or not isinstance(gen_sha, str):
                errors.append(f"incomplete generator attestation for {name}")
                continue
            try:
                actual = git_blob_sha1(checked_path(root, gen))
                if actual != gen_sha:
                    errors.append(f"generator content ID mismatch for {name}")
            except (OSError, ValueError) as exc:
                errors.append(f"cannot verify generator for {name}: {exc}")
    cross = []
    for d in discovery:
        for v in validation:
            if d not in by_name or v not in by_name:
                continue
            left = by_name[d].get("generator_family")
            right = by_name[v].get("generator_family")
            cross.append({"discovery": d, "validation": v,
                          "discovery_family": left, "validation_family": right,
                          "same_declared_family": left == right})
    if errors:
        decision = "INVALID_PROVENANCE_OR_INPUTS"
    elif any(x["same_declared_family"] for x in cross):
        decision = "SAME_FAMILY_TRANSFER_NOT_INDEPENDENT_HOLDOUT"
    else:
        decision = "DISJOINT_DECLARED_LINEAGES_INDEPENDENCE_NOT_PROVEN"
    return {"schema": SCHEMA, "decision": decision,
            "discovery": discovery, "validation": validation,
            "pairs": cross, "errors": errors,
            "claim_limits": [
                "Shared generator blocks independent-heldout claims; stress tests remain valid",
                "Distinct declared families are necessary screening, not proof of independence",
                "Promotion also requires source-authentic independent corpora, exact wire bytes, byte-exact roundtrip, decode/RSS/size gates",
            ]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--repo-root", type=Path, default=Path("."))
    parser.add_argument("--discovery", nargs="+", required=True)
    parser.add_argument("--validation", nargs="+", required=True)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--require-disjoint-lineage", action="store_true",
                        help="Fail if discovery and validation share a generator family")
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    result = evaluate(manifest, args.repo_root, args.discovery, args.validation)
    output = json.dumps(result, indent=2, sort_keys=True) + "\n"
    print(output, end="")
    if args.out:
        args.out.write_text(output, encoding="utf-8")
    if result["errors"]:
        return 2
    if args.require_disjoint_lineage and result["decision"].startswith("SAME_FAMILY"):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
