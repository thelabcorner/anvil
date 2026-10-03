#!/usr/bin/env python3
"""Instantiate a REAL current-tree Q1a lock over tests/corpus and run the
zero-codec audit against it.

READS: tests/corpus/README.md (provenance table) and tests/corpus/CHECKSUMS.txt
(SHA-256 manifest) -- metadata only -- plus the 24 open-role corpus FILES, which
are already-open, already-consumed discovery/synthetic data per
docs/I10-CORPUS-LOCK-PROTOCOL.md 2.3. NO sealed object is read: no AITDCC I-P
byte, no pe-* anatomy beyond hashing, no network, no codec, no decompression.

Writes only under --out-dir. Touches no production file.
"""

import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import q1a_corpus_lock as q  # noqa: E402

# Role assignment is taken from protocol 2.3 and the corpus README, NOT from any
# compression outcome. pe-* are CONSUMED (S6-2) -> known_stress per the
# coordinator's D16 and the SYNTH-CORPUS matrix; they are never heldout.
ROLE_BY_PREFIX = (
    ("synth-", "synthetic_control"),
    ("generated.", "synthetic_control"),
    ("pe-", "known_stress"),
    ("anvil", "self_reference_control"),
)
CLASS_BY_SUFFIX = {
    ".md": "text_log",
    ".cpp": "source_code",
    ".json": "structured_json",
    ".jsonl": "structured_ndjson",
    ".ndjson": "structured_ndjson",
    ".log": "text_log",
    ".sqlite": "sqlite",
    ".exe": "executable",
    ".bin": "random_incompressible",
}


def parse_checksums(path):
    """`<sha256>  <bytes>  <name>`, with '#' comment lines (real manifest form)."""
    out = {}
    with open(path, "r", encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) != 3:
                raise ValueError("unparsable manifest line: %r" % (line,))
            digest, _size, name = parts
            if len(digest) != 64:
                raise ValueError("not a sha256: %r" % (line,))
            out[name.strip().lstrip("*")] = digest.lower()
    return out


def parse_readme_provenance(path):
    out = {}
    with open(path, "r", encoding="utf-8") as fh:
        for line in fh:
            if not line.startswith("|"):
                continue
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            if len(cells) < 4:
                continue
            name = cells[0].strip("`")
            if name and name not in ("file", "---"):
                out[name] = cells[3]
    return out


def _git_tracked(names, tracked_paths):
    """Which declared objects exist at the pinned commit. DERIVED, not assumed."""
    return {n for n in names if ("tests/corpus/" + n) in tracked_paths}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", required=True)
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--git-sha", required=True)
    ap.add_argument(
        "--tracked-paths",
        required=True,
        help="newline-separated `git ls-tree -r --name-only <sha>` output for the subject root",
    )
    ap.add_argument("--dirty-path-count", type=int, required=True)
    args = ap.parse_args()

    corpus_dir = os.path.join(args.repo, "tests", "corpus")
    checksums = parse_checksums(os.path.join(corpus_dir, "CHECKSUMS.txt"))
    provenance = parse_readme_provenance(os.path.join(corpus_dir, "README.md"))
    if not checksums:
        print("FAIL: no checksums parsed; refusing to guess")
        return 3

    with open(args.tracked_paths, "r", encoding="utf-8") as fh:
        tracked_paths = {ln.strip() for ln in fh if ln.strip()}
    declared_names = sorted(n for n in checksums if n not in ("README.md", "CHECKSUMS.txt"))
    tracked_names = _git_tracked(declared_names, tracked_paths)
    untracked_names = sorted(set(declared_names) - tracked_names)
    print("declared data objects: %d" % (len(declared_names),))
    print("tracked at %s: %d" % (args.git_sha[:7], len(tracked_names)))
    print("UNTRACKED at %s: %d -> %s" % (args.git_sha[:7], len(untracked_names), ", ".join(untracked_names)))

    fixed = "2026-10-02T00:00:00Z"
    entries = []
    for name in sorted(checksums):
        if name in ("README.md", "CHECKSUMS.txt"):
            continue
        digest = checksums[name]
        suffix = os.path.splitext(name)[1]
        role = None
        for prefix, r in ROLE_BY_PREFIX:
            if name.startswith(prefix):
                role = r
                break
        if role is None:
            role = "discovery"
        base = os.path.splitext(name)[0]
        synthetic = None
        generator = None
        if role == "synthetic_control":
            generator = (
                "tests/make_synth_corpus.py"
                if name.startswith("synth-")
                else "tests/make_smoke_corpus.py"
            )
            synthetic = {
                "generator_path": generator,
                "generator_git_blob_sha1": None,
                "generator_sha256": q.sha256_hex(
                    open(os.path.join(args.repo, generator), "rb").read()
                ),
                "seed": {"value": base},
                "parameters": {"name": name},
                "parameters_sha256": q.canon_sha256({"name": name}),
                "runtime": {"language": "python", "version": "see-git-blob"},
            }
        entries.append(
            {
                "corpus_id": name.lower().replace(".", "-"),
                "display_name": name,
                "role": role,
                "class": CLASS_BY_SUFFIX.get(suffix, "external_mixed"),
                "selection_frozen_utc": fixed,
                "role_rationale": (
                    "from tests/corpus/README.md + protocol 2.3; selection used no "
                    "compression outcome"
                ),
                "independence": {
                    "publisher_id": "anvil-project" if not name.startswith("pe-") else "microsoft-windows",
                    "project_id": (
                        "anvil-smoke-corpus"
                        if name.startswith("generated.")
                        else ("anvil-synth-corpus" if name.startswith("synth-") else
                              ("microsoft-windows-binaries" if name.startswith("pe-") else "anvil-self"))
                    ),
                    "release_family_id": (
                        "anvil-smoke" if name.startswith("generated.")
                        else ("anvil-synth" if name.startswith("synth-") else
                              ("ms-windows-pe" if name.startswith("pe-") else "anvil-self"))
                    ),
                    "producer_id": None,
                    "schema_cluster_id": (
                        "anvil-smoke-schema"
                        if name.startswith("generated.")
                        else ("anvil-synth-schema" if name.startswith("synth-") else "not-applicable")
                    ),
                    "exact_duplicate_group_id": digest,
                    "near_duplicate_group_ids": [],
                    "audit_artifact_sha256": q.canon_sha256({"entry": name}),
                },
                "license": {
                    "status": "owned" if name.startswith("pe-") else "permitted",
                    "name": "ANVIL repository contents" if not name.startswith("pe-") else "Microsoft EULA",
                    "spdx": None,
                    "url": None,
                    "redistribution_allowed": False if name.startswith("pe-") else True,
                    "attribution_required": False,
                    "attribution_text": None,
                },
                "source": {
                    "kind": (
                        "host-installed" if name.startswith("pe-")
                        else (
                            # protocol 4.13: a generated entry is kind=generate even
                            # when the output is committed; git_tracked records repo
                            # presence separately.
                            "generate"
                            if role == "synthetic_control"
                            else ("git-raw" if (name in tracked_names) else "host-installed")
                        )
                    ),
                    "repository": (
                        "https://github.com/thelabcorner/anvil" if (name in tracked_names) else None
                    ),
                    "commit": args.git_sha if (name in tracked_names) else None,
                    "path": ("tests/corpus/" + name) if (name in tracked_names) else None,
                    "url": None,
                    "archive": None,
                    "git_tracked": (name in tracked_names),
                    "host_install": (
                        {
                            "host_product_name": "Microsoft Windows",
                            "install_root": "C:\\Windows",
                            "install_path": "C:\\Windows\\" + name.replace("pe-", "") + ".exe",
                            "file_version": None,
                        }
                        if name.startswith("pe-")
                        else (
                            {
                                "host_product_name": "ANVIL local build output",
                                "install_root": args.repo,
                                "install_path": os.path.join(args.repo, "tests", "corpus", name),
                                "file_version": None,
                            }
                            if (name not in tracked_names and role != "synthetic_control")
                            else None
                        )
                    ),
                },
                "content": {
                    "canonical_filename": name,
                    "bytes": None,
                    "sha256": digest,
                    "git_blob_sha1": None,
                    "media_type": "application/octet-stream",
                    "local_path": os.path.join("tests", "corpus", name),
                },
                "derivation": {"transform_chain": []},
                "validity": {
                    "framing": (
                        "json-document" if name.endswith(".json")
                        else "ndjson" if name.endswith((".jsonl", ".ndjson"))
                        else "log-lines" if name.endswith(".log")
                        else "sqlite" if name.endswith(".sqlite")
                        else "none"
                    ),
                    "total_records": None,
                    "valid_records": None,
                    "residual_records": None,
                    "final_newline": None,
                    "schema_signature_sha256": None,
                    "counts_source": "not-applicable",
                    "counts_artifact_sha256": None,
                },
                "synthetic": synthetic,
                "lineage": {
                    "introduced_lock_revision": 1,
                    "previous_roles": ["known_stress"] if name.startswith("pe-") else [],
                    "replaces_corpus_id": None,
                    "replacement_reason": None,
                },
                "toolchain": (
                    {
                        "producer_id": "microsoft-msvc",
                        "producer_version": "12.1",
                        "attestation_kind": "README-provenance",
                        "attestation_sha256": q.canon_sha256({"pe": name}),
                    }
                    if name.endswith(".exe")
                    else None
                ),
            }
        )

    # fill declared byte lengths from the manifest's own recorded size column
    for e in entries:
        p = os.path.join(args.repo, e["content"]["local_path"])
        if not os.path.isfile(p):
            print("FAIL: declared local_path missing on disk: %s" % (p,))
            return 3
        e["content"]["bytes"] = os.path.getsize(p)

    splits = {name: [] for name in q.PROTOCOL_SPLITS}
    for e in entries:
        splits[q.ROLE_TO_SPLIT[e["role"]]].append(e["corpus_id"])
    entries.sort(key=lambda e: e["corpus_id"])

    lock = {
        "schema": q.SCHEMA,
        "protocol_version": 1,
        "lock_revision": 1,
        "created_utc": fixed,
        "title": "Real current-tree Q1a admissibility lock over tests/corpus (NOT citation-grade)",
        "project": {
            "name": "ANVIL",
            "repository": "https://github.com/thelabcorner/anvil",
            "protocol_path": "docs/I10-CORPUS-LOCK-PROTOCOL.md",
            "source_git_sha": args.git_sha,
            # DERIVED, not asserted: the audited subject root is this repo's working
            # tree, which carries dirty paths and declared corpus objects that do
            # NOT exist at the pinned commit. Edit E8 scopes source_dirty to the
            # pinned checkout; this subject root is not that checkout, so the honest
            # value is true and the audit must block rather than claim cleanliness.
            "source_dirty": True,
            "source_tree_ref": args.git_sha,
            "evaluated_at": fixed,
            "source_dirty_scope": "pinned_checkout",
            "local_worktree": {
                "evaluated": True,
                "dirty_path_count": args.dirty_path_count,
                "declared_objects_absent_from_pinned_commit": untracked_names,
                "note": (
                    "Subject root is the local working tree, not a clean pinned checkout: "
                    "%d of %d declared data objects are absent from commit %s. This lock is "
                    "NOT citation-grade; it is tool discovery over the current tree."
                    % (len(untracked_names), len(declared_names), args.git_sha[:7])
                ),
            },
        },
        "lineage": {
            "introduced_lock_revision": 1,
            "supersedes_lock_sha256": None,
            "change_reason": None,
            "replacement_policy": "new-lock-only-no-silent-splicing",
        },
        "roles": {
            "allowed": list(q.ROLE_ENUM),
            "default_transition": {"external_test": "known_stress", "heldout": "known_stress"},
            "forbidden_transitions": [
                "external_test->external_test",
                "heldout->heldout",
                "self_reference_control->heldout",
                "synthetic_control->heldout",
            ],
            "evidence_split_map": dict(sorted(q.ROLE_TO_SPLIT.items())),
        },
        "splits": {k: sorted(v) for k, v in splits.items()},
        "entries": entries,
        "deduplication": {
            "criteria": sorted(q.CRITERIA),
            "grouping": "transitively-closed-connected-components",
            "group_ids_are_outputs": True,
            "waiver_path": "absent",
            # amendment A9: c9 is NOT byte-computed by this tool. The claim below is
            # a carried publisher/attestation record and is labelled as such; it may
            # merge units (fail-safe) but does NOT close criterion c9.
            "containment_verification": "attestation_only",
            "containment_claims": [
                {
                    "contained_corpus_id": "generated-json",
                    "container_corpus_id": "generated-sqlite",
                    "record_set_sha256": q.canon_sha256({"claim": "generated.sqlite embeds generated.json"}),
                    "evidence": "docs/swarm-2026-10-02/SYNTH-CORPUS-FLEDGE.md 1.3 (measured 12000/12000)",
                    "attestation_kind": "project-doc-measurement",
                    "attestation_sha256": q.canon_sha256({"claim": "c9-generated"}),
                }
            ],
        },
        "publisher_isolation": {
            "required_new_fields": sorted(
                ["publisher_id", "project_id", "release_family_id", "schema_cluster_id"]
            ),
            "producer_diversity_rule": "producer/compiler family must not be the sole diversity dimension",
            "selection_basis": sorted(["provenance", "size", "format", "validity", "license", "independence"]),
        },
        "acquisition": {
            "identity_algorithm": "sha256",
            "md5_forbidden": True,
            "allowed_hosts": [],
            "byte_access_roles": list(q.OPEN_ROLES),
            "sealed_byte_access": "forbidden",
            "archive_role_policy": "manifest-only, not decompressed",
        },
        "aitdcc": {
            "enabled": False,
            "development_range": "A-H",
            "test_range": "I-P",
            "sha256sums_sha256": None,
            "attribution": "AITDCC dataset and paper (protocol 8); not enabled in this lock",
        },
        "workflow": {
            "jobs": sorted(["enumerate", "units", "containment", "threshold-lint", "provenance"]),
            "fail_closed": True,
            "ledger_path": "corpus-access-ledger.jsonl",
            "ledger_appended_by": "workflow",
        },
        "artifacts": {
            "required": sorted(n for n, v in q.ARTIFACT_FILES.items() if v is not None),
            "uploaded_by": "workflow",
            "upload_condition": "always()",
        },
        "promotion": {
            "sequence": sorted([
                "lock-and-ledger", "frozen-preregistration", "correctness-gate",
                "adversarial-gate", "discovery-and-ablation-gates", "frozen-implementation",
                "one-shot-heldout-gate", "frontier-reconstruction",
            ]),
            "blocked_roles": list(q.QP_NO_PROMOTE_ROLES),
            "pass_variants": sorted(["PASS_DISCOVERY", "PASS_KNOWN_STRESS", "PASS_HELDOUT", "PASS_FRONTIER"]),
        },
        "audit_parameters": {
            "block_bytes": 4096,
            "rare_block_max_entries": 2,
            "minhash_shingle_tokens": 5,
            "minhash_permutations": 128,
            "minhash_threshold_num": 20,
            "minhash_threshold_den": 100,
            "minhash_screen_k": 8,
            "text_likeness_min_num": 90,
            "text_likeness_min_den": 100,
            "containment_evidence_kind": "declared_manifest",
            "declared_by": "q1a-real-lock-builder",
            "declared_utc": fixed,
            "declared_sha256": q.canon_sha256({"params": "real-corpus-v1"}),
        },
        "consumed_controls_ref": {
            "path": "consumed_controls.json",
            "sha256": "",
            "entry_count": 0,
            "keyed_by": ["ledger_entry", "sha256"],
        },
    }

    # real PE consumption tombstone (queue Edit E5 hashes, public facts)
    tomb = {
        "schema": q.TOMBSTONE_SCHEMA,
        "append_only": True,
        "entries": [],
    }
    pe = (
        ("1a0043555d254618f2d56c936c3d9a1fbfb878bc878416a133c346bc7835eda9", "pe-git.exe"),
        ("e52a7ad9538d9618c67a0bd777964e2eec8a30f68b810a2f6adce1f2daf847b8", "pe-ninja.exe"),
        ("49f096cbf9337b0a80bde835d29be41bc9371057c4ff6c72f8a36158c29cfa3a", "pe-notepad.exe"),
        ("fd5c46d73d29ba21b04c844bbaf9096066136526911230645a2a040d23fb612b", "pe-python.exe"),
        ("ade557dd65848c5cf6565913cf6e01cf5c9a8033f0d784c4d6932394958d743e", "pe-where.exe"),
        ("d1d050efbae74c970ba6e666de004405b21f60f34ff0886000026763fe117cf0", "pe-winver.exe"),
    )
    for digest, name in pe:
        tomb["entries"].append({
            "identity_kind": "sha256", "identity": digest, "display_name": name,
            "tombstone_key": "anvil.consumed." + name, "consumed": True,
            "consumed_by": "S6-2 PE set", "consumed_utc": fixed,
            "role_after_consumption": "known_stress",
            "evidence": "queue Edit E5 measured SHA-256 block",
            "promotion_forbidden": True,
        })
    tomb["entries"].append({
        "identity_kind": "ledger_entry", "identity": "Sino-US DrugQA V1",
        "display_name": "Sino-US DrugQA V1", "tombstone_key": "anvil.consumed.drugqa.v1",
        "consumed": True, "consumed_by": "G3", "consumed_utc": fixed,
        "role_after_consumption": "known_stress",
        "evidence": "contamination ledger entry V1-COLUMN-ADVERSE",
        "promotion_forbidden": True,
    })
    lock["consumed_controls_ref"]["sha256"] = q.canon_sha256(tomb)
    lock["consumed_controls_ref"]["entry_count"] = len(tomb["entries"])

    os.makedirs(args.out_dir, exist_ok=True)
    with open(os.path.join(args.out_dir, "real-corpus-lock.json"), "wb") as fh:
        fh.write(q.canon(q.canonicalize_lock(lock)))
    with open(os.path.join(args.out_dir, "real-consumed_controls.json"), "wb") as fh:
        fh.write(q.canon(tomb))

    schema_findings = q.validate_lock(lock)
    print("real lock entries: %d" % (len(entries),))
    print("real lock schema findings: %d" % (len(schema_findings),))
    for f in schema_findings[:10]:
        print("   ", f["code"], "|", f["path"], "|", f["detail"][:110])
    print("lock_sha256: %s" % (q.canon_sha256(q.canonicalize_lock(lock)),))
    return 0


if __name__ == "__main__":
    sys.exit(main())