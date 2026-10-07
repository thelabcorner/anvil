#!/usr/bin/env python3
"""Validate origin separation and provenance of a PRE-MEASUREMENT I19 JSON corpus manifest.

The validator intentionally never fetches data and cannot establish that a
source exists or that its declared SHA-256 is correct. Those are separately
attested by GitHub Actions before using this manifest.
"""
import argparse
import datetime
import json
import re
from collections import defaultdict
from pathlib import Path
from urllib.parse import urlsplit

SHA256 = re.compile(r"^[0-9a-f]{64}$")
IDENT = re.compile(r"^[a-z][a-z0-9_-]{2,63}$")
SPLITS = {"discovery", "validation", "heldout"}
KINDS = {"raw-bytes", "uint8", "uint16le", "uint32le", "uint64le",
         "int16le", "int32le", "int64le", "float32-bitcast-le", "float64-bitcast-le"}


def validate(doc: dict) -> dict:
    if not isinstance(doc, dict) or doc.get("schema") != "anvil.i19.origin-locked.v1":
        raise ValueError("unsupported manifest schema")
    sources = doc.get("sources")
    if not isinstance(sources, list) or not sources:
        raise ValueError("no sources")
    ids, digests, origin_splits, family_splits = set(), set(), defaultdict(set), defaultdict(set)
    split_counts = defaultdict(int)
    for i, src in enumerate(sources):
        label = f"sources[{i}]"
        if not isinstance(src, dict):
            raise ValueError(f"{label}: invalid source record")
        required = ("id", "origin_id", "dataset_family_id", "split", "publisher",
                    "url", "release", "retrieved_date", "license",
                    "sha256", "source_bytes", "representation", "preprocess",
                    "original_source_reconstructible")
        if any(key not in src for key in required):
            raise ValueError(f"{label}: missing required provenance field")
        for field in ("id", "origin_id", "dataset_family_id"):
            if not isinstance(src[field], str) or not IDENT.fullmatch(src[field]):
                raise ValueError(f"{label}: invalid {field}")
        if src["id"] in ids:
            raise ValueError(f"{label}: duplicate id")
        ids.add(src["id"])
        if src["split"] not in SPLITS:
            raise ValueError(f"{label}: invalid split")
        split_counts[src["split"]] += 1
        origin_splits[src["origin_id"]].add(src["split"])
        family_splits[src["dataset_family_id"]].add(src["split"])
        if not isinstance(src["sha256"], str) or not SHA256.fullmatch(src["sha256"]):
            raise ValueError(f"{label}: SHA-256 must be lowercase hex")
        if src["sha256"] in digests:
            raise ValueError(f"{label}: duplicate source content across records/splits")
        digests.add(src["sha256"])
        if type(src["source_bytes"]) is not int or src["source_bytes"] <= 0:
            raise ValueError(f"{label}: invalid source size")
        parsed = urlsplit(src["url"])
        if parsed.scheme != "https" or not parsed.hostname or parsed.username or parsed.password or parsed.fragment:
            raise ValueError(f"{label}: source URL must be credential-free HTTPS")
        for field in ("publisher", "release", "license", "preprocess"):
            if not isinstance(src[field], str) or not src[field].strip():
                raise ValueError(f"{label}: {field} must be nonempty")
        if src["representation"] not in KINDS:
            raise ValueError(f"{label}: unknown numerical representation")
        if type(src["original_source_reconstructible"]) is not bool:
            raise ValueError(f"{label}: reconstructibility must be explicit")
        try:
            date = datetime.date.fromisoformat(src["retrieved_date"])
        except (ValueError, TypeError) as exc:
            raise ValueError(f"{label}: invalid retrieval date") from exc
        if date > datetime.date.today():
            raise ValueError(f"{label}: retrieval date in future")
        if src["representation"] != "raw-bytes" and src["original_source_reconstructible"]:
            if not isinstance(src.get("reconstruction_recipe"), str) or not src["reconstruction_recipe"].strip():
                raise ValueError(f"{label}: claimed original reconstruction lacks recipe")
    if any(len(v) != 1 for v in origin_splits.values()):
        raise ValueError("origin leakage between splits")
    if any(len(v) != 1 for v in family_splits.values()):
        raise ValueError("dataset family leakage between splits")
    if len(origin_splits) < 3 or any(not split_counts[s] for s in SPLITS):
        raise ValueError("need at least three origins and nonempty discovery, validation, heldout")
    if doc.get("frozen") is not True:
        raise ValueError("manifest must be explicitly frozen; drafts cannot enter CI efficacy")
    return {"status": "MANIFEST_STRUCTURE_PASS", "sources": len(sources),
            "origins": len(origin_splits), "split_counts": dict(split_counts),
            "content_integrity_attested": False, "heldout_unsealed": True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    options = parser.parse_args()
    doc = json.loads(options.manifest.read_text(encoding="utf-8"))
    print(json.dumps(validate(doc), sort_keys=True))


if __name__ == "__main__":
    main()
