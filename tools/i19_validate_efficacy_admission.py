#!/usr/bin/env python3
"""Independent I19 efficacy preflight. FAIL-CLOSED metadata gate, never certifies truth.

This is an additional gate beyond the legacy STRUCTURE-only manifest validator.
It refuses known leaked heldout bytes and requires purported external custody,
license evidence, pinned controls, source archive identities and preregistration.
A passing report is ELIGIBLE_FOR_EXTERNAL_ATTESTATION, not a validation of the
claimed custody/rights, nor permission to benchmark without artifact verification.
"""
from __future__ import annotations
import argparse
import datetime as dt
import json
from pathlib import Path
import re
from urllib.parse import urlsplit
from i19_validate_corpus_manifest import validate as structural_validate

SHA = re.compile(r"^[0-9a-f]{64}$")
GIT_SHA = re.compile(r"^[0-9a-f]{40}$")
# Original I19 UCI blob was placed in a downloadable public GitHub Actions
# artifact, so it MUST NOT be promoted as a certified sealed Phase-3 heldout.
PUBLIC_I19_UCI_SHA = "9f84b46ade8a2d8e1286ec4b2b6c2987a45a755c59f263be3b3b3d10dfbda3ff"
REQUIRED_CONTROLS = {
    "brotli-q5", "brotli-q11", "zstd-fast", "zstd-medium",
    "lz4-fast", "typed-codec", "shuffle-backend", "anvil-i18-frozen",
}
ALLOWED_DTYPE = {
    "int16le", "int32le", "uint16le", "uint32le", "int64le", "uint64le",
    "float32-bitcast-le", "float64-bitcast-le",
}


def require(value, message):
    if not value:
        raise ValueError(message)


def hash_check(value, label, regex=SHA):
    require(isinstance(value, str) and regex.fullmatch(value) is not None,
            f"invalid {label}")


def valid_https(url, field):
    require(isinstance(url, str), f"missing {field}")
    parsed = urlsplit(url)
    require(parsed.scheme == "https" and parsed.hostname
            and not parsed.username and not parsed.password and not parsed.fragment,
            f"{field} must be public credential-free HTTPS")


def date_before(date, other, field):
    try:
        parsed = dt.date.fromisoformat(date)
        require(parsed <= other, f"{field} later than preregistration")
    except (TypeError, ValueError) as e:
        raise ValueError(f"invalid {field}") from e


def validate_admission(doc: dict) -> dict:
    require(isinstance(doc, dict) and doc.get("schema") == "anvil.i19.efficacy-admission.v1",
            "unsupported I19 efficacy admission schema")
    source_manifest = doc.get("source_manifest")
    structural_validate(source_manifest)
    sources = source_manifest["sources"]
    ids = {s["id"] for s in sources}
    split = {s["id"]: s["split"] for s in sources}
    originals = {s["id"]: s["sha256"] for s in sources}
    validation_origins = {s["origin_id"] for s in sources if s["split"] == "validation"}
    require(len(validation_origins) >= 2, "need two independent validation origins")
    require(sum(s["split"] == "heldout" for s in sources) == 1,
            "I19 phase-3 protocol requires exactly one sealed heldout origin")
    heldout = next(s for s in sources if s["split"] == "heldout")
    require(heldout["sha256"] != PUBLIC_I19_UCI_SHA,
            "known publicly exposed UCI artifact cannot be reclassified sealed")

    prereg = doc.get("preregistration")
    require(isinstance(prereg, dict) and prereg.get("frozen") is True,
            "missing frozen, independently reviewable preregistration")
    hash_check(prereg.get("sha256"), "preregistration SHA")
    hash_check(prereg.get("git_commit"), "preregistration source commit", GIT_SHA)
    date_before(prereg.get("date"), dt.datetime.now(dt.timezone.utc).date(), "preregistration date")
    prereg_date = dt.date.fromisoformat(prereg["date"])
    controls = prereg.get("controls")
    require(isinstance(controls, list) and all(isinstance(x, str) for x in controls)
            and set(controls) >= REQUIRED_CONTROLS,
            "missing mandatory matched reference codec arms")
    require(prereg.get("acceptance_criteria_locked") is True
            and prereg.get("timing_protocol_locked") is True
            and prereg.get("memory_protocol_locked") is True
            and prereg.get("negative_controls_locked") is True
            and prereg.get("discovery_only_tuning") is True,
            "experimental gates or origin isolation not frozen")

    archive = doc.get("archive")
    rights = doc.get("rights")
    require(isinstance(archive, dict) and set(archive) == ids,
            "incomplete authoritative immutable archive inventory")
    require(isinstance(rights, dict) and set(rights) == ids,
            "incomplete rights and attribution review")
    for src in sources:
        sid = src["id"]
        item = archive[sid]
        require(isinstance(item, dict) and item.get("status") == "sha-attested"
                and item.get("backend") == "durable-immutable-access-controlled",
                f"{sid}: durable immutable source not attested")
        hash_check(item.get("sha256"), f"{sid} archive SHA")
        require(item["sha256"] == src["sha256"]
                and type(item.get("bytes")) is int and item["bytes"] == src["source_bytes"],
                f"{sid}: immutable archive not byte-identical to source")
        valid_https(item.get("evidence_url"), f"{sid} archive attestation URL")
        require(isinstance(item.get("custodian"), str) and item["custodian"].strip(),
                f"{sid}: archive custodian missing")
        permissions = rights[sid]
        require(isinstance(permissions, dict) and permissions.get("review") == "cleared",
                f"{sid}: rights and publication review pending")
        require(permissions.get("redistribution_allowed") is True,
                f"{sid}: evidence redistribution not authorized")
        require(isinstance(permissions.get("attribution"), str)
                and permissions["attribution"].strip(), f"{sid}: attribution missing")
        valid_https(permissions.get("evidence_url"), f"{sid} rights decision URL")

    isolation = doc.get("heldout_isolation")
    require(isinstance(isolation, dict)
            and isolation.get("origin_id") == heldout["origin_id"]
            and isolation.get("source_sha256") == heldout["sha256"]
            and isolation.get("independent_custodian") is True
            and isolation.get("publicly_exposed") is False
            and isolation.get("was_inspected_during_tuning") is False
            and isolation.get("unsealed") is False,
            "heldout isolation not established")
    hash_check(isolation.get("access_log_sha256"), "heldout access audit digest")
    valid_https(isolation.get("custody_attestation_url"), "heldout custody attestation")
    date_before(isolation.get("sealed_date"), prereg_date, "holdout sealing date")

    typed = doc.get("typed_streams")
    require(isinstance(typed, list) and typed, "no typed numerical streams")
    observed_paths, typed_origins = set(), set()
    for stream in typed:
        require(isinstance(stream, dict), "invalid typed descriptor")
        sid = stream.get("source_id")
        require(sid in ids and split[sid] != "heldout", "heldout stream exposed to prereg")
        require(stream.get("source_sha256") == originals[sid],
                "typed stream source identity mismatch")
        require(stream.get("dtype") in ALLOWED_DTYPE, "unknown typed width")
        hash_check(stream.get("sha256"), "typed stream digest")
        require(type(stream.get("bytes")) is int and stream["bytes"] > 0,
                "empty typed stream")
        require(stream.get("original_source_reconstructible") is False
                and stream.get("competition_class") == "typed-projection-only",
                "original-file byte identity misrepresented")
        path = stream.get("path")
        require(isinstance(path, str) and path.startswith("typed/")
                and ".." not in path and "\\" not in path and path not in observed_paths,
                "unsafe or duplicate typed stream path")
        observed_paths.add(path)
        typed_origins.add(sid)
    require(any(split[s] == "discovery" for s in typed_origins)
            and sum(split[s] == "validation" for s in typed_origins) >= 2,
            "insufficient real nonheldout origin projection coverage")

    # This tool cannot audit filesystem bytes, legal rights, or custody itself.
    # A SEPARATE attested Actions job must check those claims before efficacy.
    return {
        "status": "ELIGIBLE_FOR_EXTERNAL_ATTESTATION_ONLY",
        "structural_manifest_passed": True, "external_archive_verified": False,
        "rights_independently_verified": False, "holdout_independently_sealed": False,
        "efficacy_authorized": False, "general_pareto_crossing": False,
        "sources": len(sources), "typed_streams": len(typed)
    }


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("admission", type=Path)
    args = cli.parse_args()
    doc = json.loads(args.admission.read_text("utf-8"))
    print(json.dumps(validate_admission(doc), sort_keys=True))


if __name__ == "__main__":
    main()
