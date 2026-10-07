#!/usr/bin/env python3
"""I19 Phase-0 only: obtain exact independent source snapshots in GitHub Actions.

No preprocessing, codec invocation, efficacy, automatic manifest freezing, or
heldout inspection is permitted. The resulting candidate manifest is a DRAFT.
"""
from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import ssl
import sys
import urllib.error
import urllib.parse
import urllib.request

IDENT = re.compile(r"^[a-z][a-z0-9_-]{2,63}$")
SPLITS = {"discovery", "validation", "heldout"}
FORMATS = {"ghcnd-dly", "usgs-waterml-json", "nasa-power-json", "zip"}
MAX_BYTES_HARD = 50_000_000


def require(ok: bool, why: str) -> None:
    if not ok:
        raise ValueError(why)


def validate_proposal(doc: dict) -> dict:
    require(isinstance(doc, dict) and doc.get("schema") == "anvil.i19.acquisition-proposal.v1",
            "invalid acquisition proposal schema")
    require(doc.get("frozen_selection") is True, "source selection must be fixed before acquisition")
    sources = doc.get("sources")
    require(isinstance(sources, list) and len(sources) >= 4,
            "need four sources to support two validation origins and a heldout")
    ids, origins, families, urls, splits = set(), {}, {}, set(), {}
    for src in sources:
        require(isinstance(src, dict), "invalid source record")
        for field in ("id", "origin_id", "dataset_family_id"):
            require(isinstance(src.get(field), str) and IDENT.fullmatch(src[field]) is not None,
                    f"invalid {field}")
        require(src["id"] not in ids, "duplicate source id")
        ids.add(src["id"])
        split = src.get("split")
        require(split in SPLITS, "invalid split")
        require(src["origin_id"] not in origins or origins[src["origin_id"]] == split,
                "origin leakage")
        require(src["dataset_family_id"] not in families or families[src["dataset_family_id"]] == split,
                "family leakage")
        origins[src["origin_id"]] = split
        families[src["dataset_family_id"]] = split
        splits[split] = splits.get(split, 0) + 1
        for field in ("publisher", "release", "license", "license_url", "preprocess"):
            require(isinstance(src.get(field), str) and src[field].strip(),
                    f"missing {field}")
        require(src.get("format") in FORMATS, "unsupported file format")
        require(src.get("representation") == "raw-bytes"
                and src.get("original_source_reconstructible") is True,
                "Phase-0 admits only byte-identity raw source snapshots")
        require(type(src.get("max_bytes")) is int
                and 0 < src["max_bytes"] <= MAX_BYTES_HARD, "invalid source byte cap")
        parsed = urllib.parse.urlsplit(src.get("url", ""))
        rights = urllib.parse.urlsplit(src["license_url"])
        for value in (parsed, rights):
            require(value.scheme == "https" and bool(value.hostname)
                    and not value.username and not value.password and not value.fragment,
                    "source and rights URLs must be credential-free HTTPS")
        require(not parsed.hostname.endswith(".local") and parsed.port in (None, 443),
                "non-public source or custom port")
        require(src["url"] not in urls, "duplicate acquisition URL")
        urls.add(src["url"])
    require(splits.get("discovery", 0) >= 1
            and splits.get("validation", 0) >= 2
            and splits.get("heldout", 0) >= 1
            and len({o for o, s in origins.items() if s == "validation"}) >= 2,
            "insufficient independent split coverage")
    return {"sources": len(sources), "origins": len(origins), "split_counts": splits,
            "phase": "SOURCE_PROPOSAL_ONLY", "promotion_authorized": False}


class SameOriginOnly(urllib.request.HTTPRedirectHandler):
    """Redirects are allowed only within exact HTTPS origin (no credential leakage)."""

    def redirect_request(self, req, fp, code, msg, headers, newurl):
        old, new = urllib.parse.urlsplit(req.full_url), urllib.parse.urlsplit(newurl)
        require(new.scheme == "https" and old.hostname == new.hostname
                and (new.port or 443) == (old.port or 443)
                and not new.username and not new.password,
                "unsafe cross-origin or non-HTTPS redirect")
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def snapshot(url: str, target: Path, cap: int) -> tuple[int, str, dict]:
    target.parent.mkdir(parents=True, exist_ok=True)
    require(not target.exists(), "refuse overwriting existing snapshot")
    part = target.with_name(target.name + ".partial")
    require(not part.exists(), "refuse overwriting prior incomplete snapshot")
    req = urllib.request.Request(url, headers={
        "Accept-Encoding": "identity",
        "Accept": "application/octet-stream,application/json,text/plain,*/*",
        "User-Agent": "ANVIL-I19-Origin-Attestation/1.0 (research snapshot)",
    })
    opener = urllib.request.build_opener(
        urllib.request.HTTPSHandler(context=ssl.create_default_context()),
        SameOriginOnly(),
    )
    digest = hashlib.sha256()
    size = 0
    try:
        with opener.open(req, timeout=60) as response:
            require(response.status == 200, f"non-200 HTTP status: {response.status}")
            clen = response.headers.get("Content-Length")
            if clen and clen.isdecimal():
                require(int(clen) <= cap, "declared size over source cap")
            require(response.headers.get("Content-Encoding", "identity").lower() == "identity",
                    "unexpected transport content encoding")
            headers = {
                "content_type": response.headers.get("Content-Type", ""),
                "content_length": clen,
                "etag": response.headers.get("ETag"),
                "last_modified": response.headers.get("Last-Modified"),
                "final_url": response.url,
            }
            require(urllib.parse.urlsplit(response.url).hostname
                    == urllib.parse.urlsplit(url).hostname, "final origin mismatch")
            with part.open("xb") as sink:
                while chunk := response.read(1 << 16):
                    size += len(chunk)
                    require(size <= cap, "source exceeded byte cap")
                    if size == len(chunk):
                        lead = chunk[:256].lstrip().lower()
                        require(not lead.startswith((b"<!doctype html", b"<html")),
                                "source returned HTML error/interstitial")
                    digest.update(chunk)
                    sink.write(chunk)
        require(size > 0, "empty source download")
        part.rename(target)
        return size, digest.hexdigest(), headers
    except BaseException:
        part.unlink(missing_ok=True)
        raise


def proposal_to_manifest(src: dict, sha: str, size: int, date: str) -> dict:
    return {
        "id": src["id"], "origin_id": src["origin_id"],
        "dataset_family_id": src["dataset_family_id"], "split": src["split"],
        "publisher": src["publisher"], "url": src["url"], "release": src["release"],
        "retrieved_date": date, "license": src["license"], "sha256": sha,
        "source_bytes": size, "representation": "raw-bytes",
        "preprocess": "identity: raw HTTP response entity bytes preserved byte-for-byte",
        "original_source_reconstructible": True,
    }


def acquire(proposal: dict, out: Path, revision: str) -> dict:
    info = validate_proposal(proposal)
    require(re.fullmatch(r"[0-9a-f]{40}", revision) is not None, "invalid source revision")
    require(not out.exists() or not any(out.iterdir()), "output must be empty")
    out.mkdir(parents=True, exist_ok=True)
    date = dt.datetime.now(dt.timezone.utc).date().isoformat()
    report = {
        "schema": "anvil.i19.acquisition-evidence.v1", "source_commit": revision,
        "acquisition_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "proposal_sha256": hashlib.sha256(
            (json.dumps(proposal, sort_keys=True, separators=(",", ":")) + "\n").encode()
        ).hexdigest(),
        "phase": "PHASE0_ONLY", "codec_efficacy_measured": False,
        "heldout_unsealed": True, "promotion_authorized": False,
        "status": "IN_PROGRESS", "sources": [], "source_selection": info,
    }
    results = []
    manifest = {"schema": "anvil.i19.origin-locked.v1", "frozen": False, "sources": []}
    try:
        for src in proposal["sources"]:
            label = src["id"]
            dest = out / "raw" / (label + ".source")
            size, sha, headers = snapshot(src["url"], dest, src["max_bytes"])
            report["sources"].append({
                "id": label, "status": "SNAPSHOT_OK", "sha256": sha,
                "source_bytes": size, "content": headers,
                "split": src["split"], "raw_artifact": f"raw/{label}.source",
                "publisher": src["publisher"], "license_url": src["license_url"],
                "format": src["format"], "rights_review_status": "PENDING",
            })
            manifest["sources"].append(proposal_to_manifest(src, sha, size, date))
            results.append(f"{sha}  raw/{label}.source")
        report["status"] = "RAW_SNAPSHOTS_COMPLETE_NOT_ADMITTED"
    except Exception as exc:
        report["status"] = "BLOCKED_ACQUISITION"
        report["error"] = f"{type(exc).__name__}: {exc}"
        raise
    finally:
        (out / "acquisition-evidence.json").write_text(
            json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        (out / "candidate-manifest.UNFROZEN.json").write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        (out / "raw-sha256.txt").write_text("\n".join(results) + "\n", encoding="utf-8")
    return report


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--proposal", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--source-revision", required=True)
    args = p.parse_args()
    proposal = json.loads(args.proposal.read_text(encoding="utf-8"))
    report = acquire(proposal, args.out, args.source_revision)
    print(json.dumps({"status": report["status"], "sources": len(report["sources"]),
                      "heldout_unsealed": True, "efficacy_measured": False}))


if __name__ == "__main__":
    main()
