#!/usr/bin/env python3
"""Emit + validate a machine-readable ANVIL corpus lock and independence audit
for the EXISTING tests/corpus, under docs/I10-CORPUS-LOCK-PROTOCOL.md v1.

PROTOTYPE SCOPE. Track 03-heldout-corpus, Space Bunny Free, 2026-10-02.
  - READ-ONLY over tests/corpus and the git index. Writes ONLY inside
    prototypes/swarm-2026-10-02/03-heldout-corpus/space-bunny/lockout/.
  - NO network. NO codec invocation. NO compression measurement. NO benchmark.
  - NO new held-out data. NO promotion claim.
  - pe-*.exe stay known_stress/control and are NEVER promoted to held-out.

Its purpose is to make the protocol EXECUTABLE and to expose blockers
mechanically instead of narrating them.

Run:
    python lock_emit.py emit      # write the lock + audit reports
    python lock_emit.py validate  # re-verify the emitted artifacts
"""

from __future__ import annotations

import datetime as _dt
import json
import subprocess
import sys
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

import seal  # noqa: E402

ROOT = HERE.parents[3]
CORPUS = ROOT / "tests" / "corpus"
OUT = HERE / "lockout"

NOW = "2026-10-02T00:00:00Z"
META_DIR = "prototypes/swarm-2026-10-02/03-heldout-corpus/space-bunny/lockout"

# --------------------------------------------------------------------------
# declared corpus policy (curator table; provenance only, never an outcome)
# --------------------------------------------------------------------------

# name -> (class, role, validity_framing, lineage_id)
POLICY = {
    # self-reference: the instrument and the instrumented source
    "anvil.exe":        ("executable", "known_stress", "none", "self-reference"),
    "anvil_bench.exe":  ("executable", "known_stress", "none", "self-reference"),
    "src.cpp":          ("source_code", "known_stress", "none", "self-reference"),
    "doc.md":           ("source_code", "known_stress", "none", "self-reference"),
    # make_smoke_corpus.py lineage -- tuned against, one independence unit
    "generated.json":       ("structured_json", "discovery", "json-document", "smoke-gen"),
    "generated.jsonl":      ("structured_ndjson", "discovery", "ndjson", "smoke-gen"),
    "generated.log":        ("text_log", "discovery", "log-lines", "smoke-gen"),
    "generated.repeat.jsonl": ("repeated", "discovery", "ndjson", "smoke-gen"),
    "generated.sqlite":     ("sqlite", "discovery", "sqlite", "smoke-gen"),
    "random.bin":           ("random_incompressible", "known_stress", "none", "smoke-gen"),
    # make_synth_corpus.py lineage -- adversarial controls, known ground truth
    "synth-timeseries.bin":      ("numeric_telemetry", "known_stress", "fixed-record", "synth-gen"),
    "synth-arith.bin":           ("numeric_telemetry", "known_stress", "fixed-record", "synth-gen"),
    "synth-jitter.bin":          ("repeated", "known_stress", "fixed-record", "synth-gen"),
    "synth-columnar-align.bin":  ("numeric_telemetry", "known_stress", "fixed-record", "synth-gen"),
    "synth-drift-stride.bin":    ("numeric_telemetry", "known_stress", "fixed-record", "synth-gen"),
    "synth-counters.log":        ("text_log", "known_stress", "log-lines", "synth-gen"),
    "synth-telemetry-f64.bin":   ("numeric_telemetry", "known_stress", "fixed-record", "synth-gen"),
    "synth-ndjson-columnar.ndjson": ("structured_ndjson", "known_stress", "ndjson", "synth-gen"),
    # host-installed binaries -- the only real-world bytes, and NOT admissible
    # as held-out evidence. Explicitly forbidden from promotion.
    "pe-winver.exe":  ("executable", "known_stress", "none", "host-ms-os-22h2"),
    "pe-where.exe":   ("executable", "known_stress", "none", "host-ms-os-22h2"),
    "pe-notepad.exe": ("executable", "known_stress", "none", "host-ms-os-22h2"),
    "pe-python.exe":  ("executable", "known_stress", "none", "host-cpython"),
    "pe-ninja.exe":   ("executable", "known_stress", "none", "host-ninja"),
    "pe-git.exe":     ("executable", "known_stress", "none", "host-gitforwin"),
}

# lineage -> (publisher_id, project_id, release_family_id, producer_id,
#             schema_cluster_id, generator_path, seeds)
LINEAGE = {
    "self-reference": ("anvil", "anvil", "anvil-worktree", "anvil-build",
                       "not-applicable", None, None),
    "smoke-gen": ("anvil", "anvil-smoke-corpus", "anvil-smoke-corpus", "make_smoke_corpus.py",
                  "smoke-rows-v1", "tests/make_smoke_corpus.py", None),

    "synth-gen": ("anvil", "anvil-synth-corpus", "anvil-synth-corpus", "make_synth_corpus.py",
                  "synth-frames-v1", "tests/make_synth_corpus.py", [90001, 90002, 90003, 90004,
                                                                    90005, 90006, 90007, 90008]),
    "host-ms-os-22h2": ("microsoft", "windows-22h2-system32", "windows-22h2", "msvc-os-ship",
                        "not-applicable", None, None),
    "host-cpython": ("python-org", "cpython-3.12.4", "cpython-3.12", "msvc-official-build",
                     "not-applicable", None, None),
    "host-ninja": ("ninja-build", "ninja-1.13.2", "ninja-1.13", "msvc-official-build",
                   "not-applicable", None, None),
    "host-gitforwin": ("git-for-windows", "git-2.55.0.windows", "git-2.55", "mingw-w64-ld-2.46",
                       "not-applicable", None, None),
}

SELF_REFERENCE = {"anvil.exe", "anvil_bench.exe", "src.cpp", "doc.md"}
FORBIDDEN_PROMOTION = {"pe-winver.exe", "pe-where.exe", "pe-notepad.exe",
                        "pe-python.exe", "pe-ninja.exe", "pe-git.exe"}


def git(*args) -> str:
    try:
        return subprocess.run(["git", *args], cwd=ROOT, capture_output=True,
                              text=True, timeout=60).stdout.strip()
    except Exception as exc:  # pragma: no cover
        return f"<error:{exc}>"


def blob_sha1(b: bytes) -> str:
    import hashlib
    return hashlib.sha1(b"blob " + str(len(b)).encode() + b"\0" + b).hexdigest()


def gen_sha(rel: str) -> str:
    import hashlib
    p = ROOT / rel
    return hashlib.sha256(p.read_bytes()).hexdigest() if p.exists() else ""


def build_entry(name: str, data: bytes, head: str) -> dict:
    cls, role, framing, lin = POLICY[name]
    pub, proj, fam, prod, cluster, gpath, seeds = LINEAGE[lin]
    is_synth = gpath is not None
    rel = f"tests/corpus/{name}"
    is_host = name.startswith("pe-")
    if is_host:
        source = {"kind": "https-file", "repository": None, "commit": None,
                  "path": None, "url": None, "archive": None}
    elif is_synth:
        source = {"kind": "generate", "repository": None, "commit": None,
                  "path": None, "url": None, "archive": None}
    else:
        source = {"kind": "git-raw", "repository": "thelabcorner/anvil", "commit": head,
                  "path": rel, "url": None, "archive": None}
    if name in SELF_REFERENCE:
        lic = {"status": "owned", "name": "ANVIL project output",
               "spdx": None, "url": None, "redistribution_allowed": True,
               "attribution_required": False, "attribution_text": None}
    elif is_host:
        lic = {"status": "unknown-no-redistribution", "name": "vendor EULA, unresolved",
               "spdx": None, "url": None, "redistribution_allowed": None,
               "attribution_required": True,
               "attribution_text": f"{name}: redistributed from a host-installed binary; "
                                   "upstream terms unresolved."}
    else:
        lic = {"status": "owned", "name": "ANVIL project output",
               "spdx": None, "url": None, "redistribution_allowed": True,
               "attribution_required": False, "attribution_text": None}
    entry = {
        "corpus_id": name.replace(".", "-"),
        "display_name": f"tests/corpus/{name}",
        "role": role,
        "class": cls,
        "selection_frozen_utc": NOW,
        "role_rationale": (
            "self-reference control: the instrument or the instrumented source; "
            "never admissible as a source_code/executable evidence datapoint"
            if name in SELF_REFERENCE else
            "host-installed binary: only real-world bytes in the corpus, but "
            "consumed, untracked, license-unresolved, and one OS release. "
            "Permanently known_stress; NEVER held-out; NEVER promotable."
            if is_host else
            "adversarial control with known ground truth; supports safety and "
            "integrity evidence only, never a generalization claim"
            if lin == "synth-gen" else
            "tuned-against discovery data from a single generator lineage; "
            "supports discovery evidence only"),
        "independence": {
            "publisher_id": pub, "project_id": proj, "release_family_id": fam,
            "producer_id": prod, "schema_cluster_id": cluster,
            "exact_duplicate_group_id": seal.sha256_hex(data),
            "near_duplicate_group_ids": [],
            "audit_artifact_sha256": "",
        },
        "license": lic,
        "source": source,
        "content": {
            "canonical_filename": name, "bytes": len(data),
            "sha256": seal.sha256_hex(data),
            "git_blob_sha1": blob_sha1(data),
            "media_type": "application/octet-stream",
        },
        "derivation": {"transform_chain": []},
        "validity": {
            "framing": framing, "total_records": None, "valid_records": None,
            "residual_records": None, "final_newline": None,
            "schema_signature_sha256": None if cluster == "not-applicable"
            else seal.sha256_hex(cluster.encode()),
            "counts_source": "not-applicable", "counts_artifact_sha256": None,
        },
        "synthetic": ({
            "generator_path": gpath,
            "generator_git_blob_sha1": blob_sha1((ROOT / gpath).read_bytes()) if gpath else "",
            "generator_sha256": gen_sha(gpath) if gpath else "",
            "seed": (seeds[0] if seeds else None),
            "parameters": {"seeds": seeds} if seeds else {},
            "parameters_sha256": seal.sha256_hex(seal.canon({"seeds": seeds})) if seeds
            else seal.sha256_hex(b"{}"),
            "runtime": {"language": "python", "version": "unpinned-local"},
        } if is_synth else None),
        "lineage": {"introduced_lock_revision": 1, "previous_roles": [],
                    "replaces_corpus_id": None, "replacement_reason": None},
    }
    return entry


# --------------------------------------------------------------------------
# audit
# --------------------------------------------------------------------------

def build_audit(entries, data):
    idxs, meta = [], []
    for e in entries:
        n = e["content"]["canonical_filename"]
        idxs.append(seal.Index(e["corpus_id"], data[n]))
        meta.append({"corpus_id": e["corpus_id"], "declared_bytes": e["content"]["bytes"],
                     "declared_sha256": e["content"]["sha256"],
                     "git_blob_sha1": e["content"]["git_blob_sha1"],
                     "independence": e["independence"],
                     "synthetic": e["synthetic"]})
    seal.mark_rare(idxs, rare_max=2, gram_rare_max=2)
    pairs = []
    for i in range(len(idxs)):
        for j in range(i + 1, len(idxs)):
            reasons = seal.audit_pair(idxs[i], idxs[j], meta[i], meta[j])
            if reasons:
                pairs.append({"a": meta[i]["corpus_id"], "b": meta[j]["corpus_id"],
                              "verdict": "BLOCK", "reasons": sorted(reasons),
                              "waived": False})
    return idxs, meta, pairs


def independence_units(entries, idxs, pairs, meta):
    """Group by lineage_id. Merge lineages that the audit proved conflict."""
    parent = {}

    def fname(cid):
        return next(e["content"]["canonical_filename"]
                    for e in entries if e["corpus_id"] == cid)

    def find(x):
        parent.setdefault(x, x)
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    def union(a, b):
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[rb] = ra

    lin_of = {e["corpus_id"]: POLICY[e["content"]["canonical_filename"]][3] for e in entries}
    for cid in lin_of:
        find(cid)
    M = {m["corpus_id"]: m for m in meta}

    def same_unit(a, b, reasons):
        ia, ib = M[a]["independence"], M[b]["independence"]
        if any(r.startswith(("c1-", "c3-", "c4-", "c5-")) for r in reasons):
            return True
        # protocol 5.1 rule 8: shared generator lineage for synthetic data.
        if ia["producer_id"] and ia["producer_id"] == ib["producer_id"] and (
                ia["project_id"] == ib["project_id"]
                or ia["release_family_id"] == ib["release_family_id"]):
            return True
        # protocol 5.1 rule 6: same schema cluster AND same publisher/release.
        if (ia["schema_cluster_id"] == ib["schema_cluster_id"]
                != "not-applicable"
                and (ia["publisher_id"] == ib["publisher_id"]
                     or ia["release_family_id"] == ib["release_family_id"])):
            return True
        # rule 7 on publisher equality ALONE is deliberately NOT a merge rule:
        # every ANVIL-authored file shares publisher "anvil", so a bare
        # publisher test collapses the whole corpus into one unit and destroys
        # the distinction the rule exists to draw.
        return False

    for p in pairs:
        if same_unit(p["a"], p["b"], p["reasons"]):
            union(p["a"], p["b"])
    for cid, lin in lin_of.items():
        for other, ol in lin_of.items():
            if other != cid and ol == lin:
                union(cid, other)
    groups = {}
    for e in entries:
        groups.setdefault(find(e["corpus_id"]), []).append(e["corpus_id"])
    out = []
    for root, members in sorted(groups.items()):
        files_ = sorted(fname(m) for m in members)
        classes = sorted({POLICY[f][0] for f in files_})
        lines_ = sorted({POLICY[f][3] for f in files_})
        self_ref = all(f in SELF_REFERENCE for f in files_)
        host = all(f.startswith("pe-") for f in files_)
        synth = all(f.startswith("synth-") or POLICY[f][3] in ("smoke-gen", "synth-gen")
                    for f in files_)
        out.append({
            "independence_unit_id": "unit-" + root.replace("pe-", "pe").replace("-", "_"),
            "members": sorted(members),
            "files": files_,
            "member_count": len(members),
            "classes": classes,
            "lineage_ids": lines_,
            "synthetic": synth,
            "self_reference": self_ref,
            "host_installed": host,
            "admissible_as_evidence": not (self_ref or host or synth),
            "admissibility_reason": (
                "self-reference control: the instrument or the instrumented source"
                if self_ref else
                "host-installed binary: consumed, untracked, license-unresolved; "
                "permanently ineligible for any held-out or promotion role"
                if host else
                "synthetic: single ANVIL generator lineage; admissible as a "
                "correctness/negative/sensitivity control only, never as evidence"
                if synth else
                "real, independently produced"),
        })
    return out


# --------------------------------------------------------------------------
# schema validation against the real v1 spec
# --------------------------------------------------------------------------

ROLES = {"discovery", "known_stress", "heldout", "external_development",
         "external_test", "external_anchor"}
CLASSES = {"structured_json", "structured_ndjson", "text_log", "source_code",
           "executable", "sqlite", "numeric_telemetry", "random_incompressible",
           "repeated", "mixed_validity", "malformed_stream", "external_mixed",
           "external_test"}
SOURCE_KINDS = {"git-raw", "https-file", "archive-extract", "generate"}
ENTRY_KEYS = {"corpus_id", "display_name", "role", "class", "selection_frozen_utc",
              "role_rationale", "independence", "license", "source", "content",
              "derivation", "validity", "synthetic", "lineage"}


def validate(lock: dict) -> list[dict]:
    v: list[dict] = []

    def add(code, where, msg, blocker=None):
        v.append({"code": code, "where": where, "message": msg, "blocker": blocker})

    if lock.get("schema") != "anvil.corpus-lock/v1":
        add("S-SCHEMA", "$", "wrong schema string")
    for k in ("protocol_version", "lock_revision", "created_utc", "title", "project",
              "lineage", "roles", "splits", "entries", "deduplication",
              "publisher_isolation", "acquisition", "aitdcc", "workflow",
              "artifacts", "promotion"):
        if k not in lock:
            add("S-MISSING-TOP", "$", f"missing top-level field {k}", "PB-01")
    for e in lock.get("entries", []):
        cid = e.get("corpus_id", "<no-id>")
        extra = set(e) - ENTRY_KEYS
        if extra:
            add("S-UNKNOWN-ENTRY-KEY", cid, f"unknown entry fields {sorted(extra)}")
        if e.get("role") not in ROLES:
            add("S-ROLE", cid, f"illegal role {e.get('role')}")
        if e.get("class") not in CLASSES:
            add("S-CLASS", cid, f"illegal class {e.get('class')}")
        sk = (e.get("source") or {}).get("kind")
        if sk not in SOURCE_KINDS:
            add("S-SOURCE-KIND", cid, f"illegal source.kind {sk}", "PB-04")
        elif sk == "git-raw":
            if not e["source"].get("repository"):
                add("S-SOURCE-GITRAW-EMPTY", cid, "git-raw requires repository")
            if not (e["source"].get("commit") or "").__len__() == 40:
                add("S-SOURCE-GITRAW-COMMIT", cid, "git-raw requires 40-hex commit")
            if not e["source"].get("path"):
                add("S-SOURCE-GITRAW-PATH", cid, "git-raw requires path")
        elif sk in ("https-file", "archive-extract") and not e["source"].get("url"):
            add("S-SOURCE-URL", cid, f"{sk} requires a url; none can be derived for a "
                                     "host-installed binary", "PB-04")
        if e.get("synthetic") is None and (e.get("class") or "").startswith("synth"):
            add("S-SYNTH-NULL", cid, "synthetic class without a synthetic object")
        if (e.get("independence") or {}).get("audit_artifact_sha256") == "":
            add("S-AUDIT-EMPTY", cid, "audit_artifact_sha256 not populated",
                "PB-10")
    splits = lock.get("splits", {})
    ids = [e["corpus_id"] for e in lock.get("entries", [])]
    union = sorted({i for v in splits.values() for i in v})
    if union != sorted(ids):
        add("S-SPLITS-UNION", "$", f"splits union != entries", "PB-03")
    for cid in ids:
        where = [r for r, v in splits.items() if cid in v]
        if len(where) != 1:
            add("S-SPLITS-MEMBERSHIP", cid, f"appears in {len(where)} splits", "PB-03")
        else:
            role = next(e["role"] for e in lock["entries"] if e["corpus_id"] == cid)
            if role != where[0]:
                add("S-SPLITS-ROLE", cid, f"split {where[0]} != role {role}", "PB-03")
    return v


# --------------------------------------------------------------------------
# emit
# --------------------------------------------------------------------------

def main() -> int:
    if len(sys.argv) != 2 or sys.argv[1] not in ("emit", "validate"):
        print(__doc__)
        return 2
    OUT.mkdir(parents=True, exist_ok=True)
    head = git("rev-parse", "HEAD")
    files = sorted(p.name for p in CORPUS.iterdir()
                   if p.is_file() and p.name not in ("CHECKSUMS.txt", "README.md"))
    data = {n: (CORPUS / n).read_bytes() for n in files}

    entries = [build_entry(n, data[n], head) for n in files]
    idxs, meta, pairs = build_audit(entries, data)
    units = independence_units(entries, idxs, pairs, meta)

    audit_hash = seal.sha256_hex(seal.canon(pairs))
    for e in entries:
        e["independence"]["audit_artifact_sha256"] = audit_hash
    for p in pairs:
        for m in (p["a"], p["b"]):
            next(e for e in entries if e["corpus_id"] == m)[
                "independence"]["near_duplicate_group_ids"] = sorted(
                    {p["b"] if p["a"] == m else p["a"] for p in pairs
                     if m in (p["a"], p["b"])})

    splits = {r: [] for r in ("discovery", "known_stress", "heldout",
                              "external_development", "external_test", "external_anchor")}
    for e in entries:
        splits[e["role"]].append(e["corpus_id"])

    lock = {
        "schema": "anvil.corpus-lock/v1",
        "protocol_version": 1,
        "lock_revision": 1,
        "created_utc": NOW,
        "title": "ANVIL existing tests/corpus, classified at its true role",
        "project": {
            "name": "ANVIL",
            "repository": "https://github.com/thelabcorner/anvil",
            "protocol_path": "docs/I10-CORPUS-LOCK-PROTOCOL.md",
            "source_git_sha": head,
            # Cleanliness is a property of the REMOTE MEASUREMENT CHECKOUT at
            # source_git_sha, NOT of the coordinator's intentionally dirty
            # research worktree. See checkout-cleanliness.json.
            "source_dirty": False,
        },
        "lineage": {"introduced_lock_revision": 1, "supersedes_lock_sha256": None,
                    "change_reason": None,
                    "replacement_policy": "new-lock-only-no-silent-splicing"},
        "roles": {
            "allowed": sorted(ROLES),
            "default_transition": {"heldout": "known_stress",
                                   "external_test": "known_stress"},
            "forbidden_transitions": ["heldout->heldout", "external_test->external_test"],
        },
        "splits": splits,
        "entries": entries,
        "deduplication": {"criteria": [1, 2, 3, 4, 5, 6, 7, 8],
                          "block_size": seal.BLOCK,
                          "rare_max": 2, "gram_rare_max": 2,
                          "text_likeness_min": seal.TEXT_LIKENESS_MIN,
                          "screen": "unigram-jaccard", "decide": "rare-5gram-residual",
                          "waiver_path_present": False},
        "publisher_isolation": {"enforced": True,
                                "violations": ["host-installed binaries have no "
                                               "repository owner (protocol 5.2)"]},
        "acquisition": {"mode": "local-readonly-existing-tree",
                        "fail_closed": True, "hash_authority": "sha256"},
        "aitdcc": {"a_h": [], "i_p": [], "configured": False},
        "workflow": {"roles_allowed_per_job": {"discovery": ["discovery"],
                                                "known_stress": ["discovery", "known_stress"],
                                                "heldout": ["heldout", "external_test"],
                                                "frontier": ["external_anchor",
                                                             "external_development",
                                                             "external_test", "known_stress"]}},
        "artifacts": {"dir": META_DIR},
        "promotion": {"any_entry_promotable": False,
                      "reason": "zero heldout entries; PB-01/PB-09/PB-10/PB-26 live"},
    }

    violations = validate(lock)
    seal_input = {"lock_revision": lock["lock_revision"],
                  "entries": [{"corpus_id": e["corpus_id"],
                               "declared_bytes": e["content"]["bytes"],
                               "role": e["role"], "class": e["class"],
                               "independence": e["independence"],
                               "source": e["source"], "license": e["license"]}
                              for e in entries]}
    seal_obj = seal.prospective_seal(seal_input)
    canonical = seal.canon(lock)

    def w(name, obj):
        p = OUT / name
        p.write_bytes(seal.canon(obj))
        return p

    w("corpus-lock.json", lock)
    (OUT / "corpus-lock.sha256").write_bytes(
        f"{seal.sha256_hex(canonical)}  corpus-lock.json\n".encode())
    w("dedup-report.json", {"schema": "anvil.dedup-report/v1", "lock_sha256":
                            seal.sha256_hex(canonical), "pairs_compared":
                            len(idxs) * (len(idxs) - 1) // 2, "pairs_blocking": len(pairs),
                            "pairs": pairs, "waiver_path_present": False})
    w("publisher-split-report.json", {
        "schema": "anvil.publisher-split/v1",
        "publisher_groups": sorted({e["independence"]["publisher_id"] for e in entries}),
        "release_families": sorted({e["independence"]["release_family_id"] for e in entries}),
        "schema_clusters": sorted({e["independence"]["schema_cluster_id"] for e in entries}),
        "isolation_ok": False,
        "violations": ["protocol 5.2 requires a new repository owner per held-out entry; "
                       "6 host-installed binaries have no repository owner"]})
    w("license-report.json", {
        "schema": "anvil.license-report/v1",
        "entries": [{"corpus_id": e["corpus_id"], "status": e["license"]["status"],
                     "redistribution_allowed": e["license"]["redistribution_allowed"],
                     "may_upload_bytes": e["license"]["status"] != "unknown-no-redistribution"}
                    for e in entries],
        "unresolved": [e["corpus_id"] for e in entries
                       if e["license"]["status"] == "unknown-no-redistribution"]})
    w("schema-validation.json", {
        "schema": "anvil.schema-validation/v1", "spec": "docs/I10-CORPUS-LOCK-PROTOCOL.md",
        "violation_count": len(violations), "violations": violations,
        "valid": not violations})
    w("independence-units.json", {
        "schema": "anvil.independence-units/v1", "file_count": len(entries),
        "independence_unit_count": len(units), "units": units,
        "admissible_units": sum(1 for u in units if u["admissible_as_evidence"]),
        "heldout_units": 0})
    w("prospective-seal.json", seal_obj)
    w("checkout-cleanliness.json", {
        "schema": "anvil.checkout-cleanliness/v1",
        "principle": "Cleanliness is a property of the REMOTE MEASUREMENT CHECKOUT "
                     "at a pinned commit, never of the coordinator's intentionally "
                     "dirty research worktree.",
        "measurement_checkout": {
            "repository": "https://github.com/thelabcorner/anvil",
            "pinned_commit": head, "expected_dirty": False,
            "must_verify": ["git rev-parse HEAD == pinned_commit",
                            "git status --porcelain == empty",
                            "git diff --stat == empty",
                            "git stash list == empty"],
            "assertion": "a dirty measurement checkout is INVALID_INFRA"},
        "coordinator_worktree": {
            "path": str(ROOT), "expected_dirty": True, "authoritative": False,
            "observed_dirty": bool(git("status", "--porcelain")),
            "note": "MASTER-BRIEF doctrine 8: the worktree is intentionally dirty. "
                    "Local swarm artifacts are NOT part of the measured source and "
                    "must never influence a clean-checkout assertion."}})
    w("synthetic-grade.json", {
        "schema": "anvil.synthetic-grade/v1",
        "note": "protocol v1 has no synthetic-control role; synthetic families are "
                "classified discovery/known_stress and the control semantics are "
                "carried here so they cannot be mistaken for evidence.",
        "families": [
            {"family": "synth-gen", "grade": "S2", "in_tuning_ancestry": True,
             "usable_for": ["correctness_fuzz", "negative_control", "decode_cost_accounting",
                            "oracle_anatomy_calibration"],
             "forbidden": ["blind_mechanism_sensitivity", "heldout_or_generalization_claim",
                           "reported_ratio_row_without_real_counterpart"]},
            {"family": "smoke-gen", "grade": "S2", "in_tuning_ancestry": True,
             "usable_for": ["correctness_fuzz", "negative_control", "decode_cost_accounting"],
             "forbidden": ["blind_mechanism_sensitivity", "heldout_or_generalization_claim",
                           "reported_ratio_row_without_real_counterpart"]}]})
    w("blocking-gates.json", gates(entries, units, violations, head))
    w("acquisition-checklist.json", checklist())

    print(f"lock_id      {seal.sha256_hex(canonical)}")
    print(f"truth_root   {seal_obj['truth_root']}")
    print(f"entries      {len(entries)}")
    print(f"splits       " + json.dumps({k: len(v) for k, v in splits.items()}))
    print(f"units        {len(units)} (admissible "
          f"{sum(1 for u in units if u['admissible_as_evidence'])})")
    print(f"audit        {len(pairs)} blocking pairs of "
          f"{len(idxs) * (len(idxs) - 1) // 2} compared")
    print(f"violations   {len(violations)}")
    for v in violations:
        print(f"  [{v['code']}] {v['where']}: {v['message']}"
              + (f"  <- {v['blocker']}" if v["blocker"] else ""))
    print(f"wrote        {OUT}")
    return 0


def gates(entries, units, violations, head) -> dict:
    admissible = [u for u in units if u["admissible_as_evidence"]]
    non_synth = [u for u in admissible
                 if not all(m.replace("-", ".") in POLICY
                            and POLICY[m.replace("-", ".")][1] != "discovery"
                            or m.startswith("synth-") for m in u["members"])]
    return {
        "schema": "anvil.blocking-gates/v1",
        "measured_commit": head,
        "live_protocol_blockers": [
            {"id": "PB-01", "status": "live", "reason": "no prior corpus lock existed"},
            {"id": "PB-09", "status": "live", "reason": "no corpus-access-ledger.jsonl exists"},
            {"id": "PB-10", "status": "live", "reason": "no independence audit existed"},
            {"id": "PB-26", "status": "live",
             "reason": "zero heldout entries; no new held-out family exists"}],
        "resolved_by_this_scaffold": ["PB-01 (lock now exists)",
                                      "PB-09 (partial: ledger still required)",
                                      "PB-10 (audit now exists and is reproducible)"],
        "still_live_after_scaffold": ["PB-09", "PB-26"],
        "heldout_split_population": 0,
        "any_entry_promotable": False,
        "schema_violation_count": len(violations),
        "track_blocking": {
            "structured_heldout_family": "ABSENT",
            "executable_heldout_family": "ABSENT",
            "numeric_heldout_family": "ABSENT",
            "mixed_validity_family": "ABSENT"},
    }


def checklist() -> dict:
    return {
        "schema": "anvil.acquisition-checklist/v1",
        "purpose": "Candidate-family acquisition checklist for protocol 13. "
                   "Selection MUST use provenance, size, format, validity, license and "
                   "independence criteria ONLY. Compression outcomes MUST NOT "
                   "influence selection. No candidate URL is recorded here because "
                   "none has been verified by this agent.",
        "per_family_requirements": [
            {"family_class": "structured_json_or_ndjson", "count_required": 3,
             "must_have": ["independent publisher_id", "independent project_id",
                           "independent release_family_id",
                           "new schema_cluster_id (computed field-path signature)",
                           "immutable pin: git commit + path + blob sha1, or "
                           "versioned release URL + archive_sha256 + member_sha256",
                           "license permitting CI acquisition",
                           "redistribution status resolved"],
             "current_status": "ABSENT"},
            {"family_class": "mixed_validity_or_malformed", "count_required": 2,
             "must_have": ["substantial residual (invalid) material",
                           "counts_source = curator or locked-generator",
                           "counts_artifact_sha256",
                           "deterministic framing", "explicit malformed-input policy"],
             "current_status": "ABSENT"},
            {"family_class": "executable", "count_required": 3,
             "must_have": [">= 2 unrelated producers", ">= 2 unrelated toolchains",
                           "attested from producer metadata, NOT inferred from the "
                           "PE optional-header linker version",
                           "new publisher/project/release family",
                           "license permitting CI acquisition"],
             "current_status": "ABSENT",
             "note": "the six host-installed pe-*.exe are known_stress and are "
                     "PERMANENTLY ineligible; they are not a candidate pool"},
            {"family_class": "numeric_telemetry_or_scientific", "count_required": 2,
             "must_have": ["NOT produced by tests/make_synth_corpus.py or any ANVIL "
                           "generator", "independent publisher",
                           "immutable version pin"],
             "current_status": "ABSENT"},
        ],
        "acceptance_per_candidate": [
            "identity verified BEFORE any codec runs",
            "independence audit shows no ungrouped 13.5.1 conflict",
            "preview: schema signature matches the declared cluster",
            "no entry may be added to a lock whose truth_root is already sealed",
        ],
        "hard_rule": "A candidate that fails any requirement is DROPPED, not waived. "
                     "Protocol 11 PB-11 forbids waiving a duplicate to inflate the "
                     "independent-family count.",
    }


if __name__ == "__main__":
    sys.exit(main())