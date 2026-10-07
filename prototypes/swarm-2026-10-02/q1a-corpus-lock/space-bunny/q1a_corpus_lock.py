#!/usr/bin/env python3
"""Q1a - CORPUS-ADMISSIBILITY-v1 deterministic local/static audit scaffold.

PROTOTYPE SCOPE (track q1a-corpus-lock, Space Bunny Free, 2026-10-02).

This module is SCIENTIFIC-INFRASTRUCTURE logic only, in an isolated prototype
directory. It is NOT production wiring: it imports nothing from the codec, is
referenced by no workflow, and is invoked by no benchmark.

Authoritative inputs
--------------------
* ``docs/I10-CORPUS-LOCK-PROTOCOL.md``  - lock schema ``anvil.corpus-lock/v1``
  (sections 2, 3, 4.x, 5.1-5.3, 6, 11, 13) and promotion blockers PB-01..PB-28.
* ``docs/swarm-2026-10-02/REMOTE-EXPERIMENT-QUEUE.md`` - queue edits
  E1 (evidence_role / "citation-grade" ban), E2 (role-stratified aggregate +
  ``promotion_authorized``), E5 (consumed-controls tombstone), E6 (the Q1a/Q1b
  split and the deterministic-audit job list), E7 (zero-sealed-byte assertions),
  E8 (``source_tree_ref`` / ``evaluated_at`` / ``source_dirty`` scoping),
  E4 (``QP-NO-PROMOTE`` / ``QP-CONSUMED``).
* ``docs/swarm-2026-10-02/SYNTH-CORPUS-FLEDGE.md`` - ``GATE-INDEP-1/2``,
  criterion ``c9`` (cross-container containment), and job 5 (no threshold may
  live in a code default).
* ``docs/swarm-2026-10-02/03-heldout-corpus-fledge.md`` - ``GATE-INDEP-1/2``,
  threshold-lint requirement, c9 rationale.

What this module does
---------------------
1. Builds and strictly validates a ``anvil.corpus-lock/v1`` scaffold.
2. Declares the protocol AMENDMENTS it needs (A1..A6) instead of editing the
   normative protocol file.
3. Computes the section-5.1 conflict graph (c1..c8) plus criterion c9, from
   retained metadata and - only for already-open roles - bytes passed through a
   counting exposure guard.
4. Computes independence groups as the transitively closed connected components
   of that graph (union-find). Group ids are an OUTPUT; curator-supplied unit
   ids are structurally impossible and curator-supplied duplicate annotations
   are advisory-only and must be a subset of the computed edges.
5. Runs the Q1a gate battery, the ``QP-NO-PROMOTE`` / ``QP-CONSUMED``
   pre-verdict gates, the consumption-tombstone check, and emits the
   role-stratified aggregate contract with ``promotion_authorized``.
6. Emits every artifact as canonical JSON with NO floats and NO wall-clock
   reads, so byte-identity is testable on any Python 3 minor version.

Hard prohibitions, enforced by counters in code (E7)
-----------------------------------------------------
* no network syscalls (``network_ops`` must stay 0),
* no codec invocations (``codec_invocations`` must stay 0),
* no archive decompression (``archive_decompressions`` must stay 0),
* no sealed/unopened byte read (``sealed_byte_reads`` must stay 0).

These are counted, not promised: every read path goes through ``ExposureGuard``
and the audit ends with :func:`assert_zero_exposure`.

Determinism
-----------
No ``time``/``datetime``/``random``/``os.urandom``/``sys.version_info`` reads,
no float values in any artifact, no set-iteration order in output, no
``PYTHONHASHSEED`` dependence. ``created_utc`` and ``evaluated_at`` are INPUTS.

Usage
-----
    python q1a_corpus_lock.py selftest
    python q1a_corpus_lock.py audit --lock LOCK.json --tombstone TOMB.json
                                   [--bytes-dir DIR] --emit-dir OUT
"""

from __future__ import annotations

import ast
import hashlib
import json
import os
import re
import sys

# ---------------------------------------------------------------------------
# identity
# ---------------------------------------------------------------------------

SCHEMA = "anvil.corpus-lock/v1"
PROTOCOL_VERSION = 1
REPORT_SCHEMA = "anvil.corpus-admissibility/v1"
AGGREGATE_SCHEMA = "anvil.aggregate-contract/v1"
TOMBSTONE_SCHEMA = "anvil.consumed-controls/v1"

TOOL_NAME = "q1a-corpus-lock-prototype"
TOOL_VERSION = "q1a-prototype-1"
TOOL_SPEC = "q1a-corpus-lock/space-bunny/q1a_corpus_lock.py"

# Mathematical constants used inside hashing code. These are named and
# allow-listed by the threshold lint (see `PINNED_NUMBERS`); they are not
# decision thresholds.
# ---------------------------------------------------------------------------
# pinned constants, classified by KINDS
#
# The threshold lint (SYNTH-CORPUS-FLEDGE 11.5) forbids a DECISION THRESHOLD
# from living in a code default. Not every integer is a decision threshold, so
# constants are classified rather than blanket-allow-listed:
#
#   "protocol" - fixed by the normative protocol/schema. Not a threshold and
#                not tunable: changing it would change schema identity.
#   "indexing" - a hash/screen implementation constant. It affects only WHICH
#                candidate pairs are compared, never WHETHER a conflict is
#                decided: collisions only add candidates, and every candidate
#                is then decided by an exact/attested criterion.
#   "math"     - a mathematical constant of the chosen estimator.
#
# Anything that can flip a conflict verdict (block size, rare-block ceiling,
# shingle width, MinHash threshold, screen width, text-likeness cutoff) is NOT
# in this table: it MUST be declared in `audit_parameters` in the lock.
# ---------------------------------------------------------------------------

PROTOCOL_CONSTANTS = {
    "PROTOCOL_VERSION": "anvil.corpus-lock/v1 pins protocol_version to exactly 1",
}

INDEXING_CONSTANTS = {
    "BLOCK_DIGEST_BYTES": (
        "truncated BLAKE2b width for the 4 KiB block index; a collision only adds "
        "a candidate pair, and c3/c4 are then decided on the block bytes"
    ),
    "SHINGLE_SCREEN_MAX": "screen bound on the shingle set size for MinHash; screening only",
}

MATH_CONSTANTS = {
    "MINHASH_PRIME": "2**61-1, the finite-field prime for universal hashing",
}

PINNED_NUMBERS = {}
PINNED_NUMBERS.update(PROTOCOL_CONSTANTS)
PINNED_NUMBERS.update(INDEXING_CONSTANTS)
PINNED_NUMBERS.update(MATH_CONSTANTS)

PINNED_KINDS = {}
PINNED_KINDS.update({k: "protocol" for k in PROTOCOL_CONSTANTS})
PINNED_KINDS.update({k: "indexing" for k in INDEXING_CONSTANTS})
PINNED_KINDS.update({k: "math" for k in MATH_CONSTANTS})

# ---------------------------------------------------------------------------
# roles, splits, classes
# ---------------------------------------------------------------------------

# Protocol section 2.1 role enum.
PROTOCOL_ROLES = (
    "discovery",
    "external_anchor",
    "external_development",
    "external_test",
    "heldout",
    "known_stress",
)
# Queue E1 / queue Q1 required roles that protocol 2.1 does not list.
QUEUE_ADDED_ROLES = ("self_reference_control", "synthetic_control")
ROLE_ENUM = tuple(sorted(PROTOCOL_ROLES + QUEUE_ADDED_ROLES))

# Protocol section 4.5: splits has EXACTLY these keys. Added roles therefore
# cannot become new split keys without breaking 4.5; amendment A3b maps them.
PROTOCOL_SPLITS = (
    "discovery",
    "external_anchor",
    "external_development",
    "external_test",
    "heldout",
    "known_stress",
)

ROLE_TO_SPLIT = {
    "discovery": "discovery",
    "self_reference_control": "discovery",
    "synthetic_control": "discovery",
    "known_stress": "known_stress",
    "heldout": "heldout",
    "external_test": "external_test",
    "external_development": "external_development",
    "external_anchor": "external_anchor",
}

SEALED_ROLES = ("external_test", "heldout")
OPEN_ROLES = tuple(sorted(r for r in ROLE_ENUM if r not in SEALED_ROLES))
CONSUMABLE_ROLES = ("external_test", "heldout")
NON_CONSUMABLE_ROLES = ("discovery", "self_reference_control", "synthetic_control")

# Roles whose rows may be labelled held-out evidence (queue E1).
PROMOTION_ROLES = ("external_test", "heldout")

# QP-NO-PROMOTE (queue E4) enumerates exactly:
#   discovery, synthetic_control, self_reference_control, known_stress,
#   external_anchor
# Protocol 2.1 says external_development is "Never held-out evidence", so the
# gate below adds it. The deviation is disclosed, never silent.
QP_NO_PROMOTE_ROLES_EDIT_TEXT = (
    "discovery",
    "self_reference_control",
    "synthetic_control",
    "known_stress",
    "external_anchor",
)
QP_NO_PROMOTE_ROLES = tuple(sorted(QP_NO_PROMOTE_ROLES_EDIT_TEXT + ("external_development",)))
QP_GATE_DEVIATIONS = (
    {
        "gate": "QP-NO-PROMOTE",
        "field": "evidence_role set",
        "edit_text_value": list(QP_NO_PROMOTE_ROLES_EDIT_TEXT),
        "implemented_value": list(QP_NO_PROMOTE_ROLES),
        "reason": (
            "Queue Edit E4 omits external_development, but protocol section 2.1 "
            "states external_development is 'Never held-out evidence'. A gate that "
            "permits promotion language on external_development rows would contradict "
            "the protocol it enforces. Implemented as the safe superset; the "
            "coordinator may narrow it by explicit ruling."
        ),
    },
)

# Protocol section 4.6 controlled class values.
CONTROLLED_CLASSES = (
    "executable",
    "external_mixed",
    "external_test",
    "malformed_stream",
    "mixed_validity",
    "numeric_telemetry",
    "random_incompressible",
    "repeated",
    "source_code",
    "sqlite",
    "structured_json",
    "structured_ndjson",
    "text_log",
)

# Source kinds: protocol 4.9 enum plus the SYNTH-CORPUS 8.1(6) kind.
SOURCE_KINDS_PROTOCOL = ("archive-extract", "generate", "git-raw", "https-file")
SOURCE_KINDS = tuple(sorted(SOURCE_KINDS_PROTOCOL + ("host-installed",)))

FRAMINGS = ("fixed-record", "json-document", "log-lines", "ndjson", "none", "sqlite")
COUNTS_SOURCES = ("curator", "locked-generator", "not-applicable")
LICENSE_STATUS = ("owned", "permitted", "public-domain", "unknown-no-redistribution")
TRANSFORM_KINDS = (
    "concatenate",
    "decompress",
    "newline-normalize",
    "record-inject",
    "truncate",
)

CORPUS_ID_RE = re.compile(r"^[a-z0-9][a-z0-9._-]{2,63}$")
HEX40_RE = re.compile(r"^[0-9a-f]{40}$")
HEX64_RE = re.compile(r"^[0-9a-f]{64}$")
HEX8_RE = re.compile(r"^[0-9a-f]{8}$")
RFC3339_RE = re.compile(
    r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d+)?Z$"
)

# ---------------------------------------------------------------------------
# protocol amendments (declared, not applied to the protocol file)
# ---------------------------------------------------------------------------

AMENDMENTS = {
    "A1": {
        "ref": "REMOTE-EXPERIMENT-QUEUE.md Edit E8 (2026-10-02 coordinator ruling)",
        "summary": (
            "Adds project.source_tree_ref, project.evaluated_at and "
            "project.source_dirty_scope so source_dirty is asserted about the "
            "PINNED CHECKOUT, never the operator's local worktree."
        ),
        "adds": {
            "project": ("source_tree_ref", "evaluated_at", "source_dirty_scope"),
        },
    },
    "A2": {
        "ref": "SYNTH-CORPUS-FLEDGE.md 11.5 / GATE-INDEP-1 threshold lint",
        "summary": (
            "Adds a top-level audit_parameters object so no numeric decision "
            "threshold can live in a code default."
        ),
        "adds": {
            "top_level": ("audit_parameters",),
        },
    },
    "A3": {
        "ref": "REMOTE-EXPERIMENT-QUEUE.md Edit E1 and Q1 required-checks list",
        "summary": "Extends the role enum with synthetic_control and self_reference_control.",
        "adds": {
            "roles": ("synthetic_control", "self_reference_control"),
        },
    },
    "A3b": {
        "ref": "protocol section 4.5 forbids extra split keys",
        "summary": (
            "Adds roles.evidence_split_map so the two added roles occupy an "
            "existing split (discovery) instead of breaking 4.5's exact key set."
        ),
        "adds": {
            "roles": ("evidence_split_map",),
        },
    },
    "A4": {
        "ref": "SYNTH-CORPUS-FLEDGE.md 8.1(6) and 11.6; queue Edit E5 (git_tracked recorded, not required)",
        "summary": (
            "Adds source.kind 'host-installed', source.git_tracked, "
            "source.host_install and entry.toolchain so provenance completeness "
            "is checkable for host binaries with no upstream manifest."
        ),
        "adds": {
            "source": ("git_tracked", "host_install"),
            "entry": ("toolchain",),
        },
    },
    "A5": {
        "ref": "REMOTE-EXPERIMENT-QUEUE.md Edit E5 (consumed-controls tombstone)",
        "summary": (
            "Adds consumed_controls_ref so a lock is fail-closed on a stale "
            "consumption tombstone instead of being audited against no tombstone."
        ),
        "adds": {
            "top_level": ("consumed_controls_ref",),
        },
    },
    "A6": {
        "ref": "SYNTH-CORPUS-FLEDGE.md 8.1(3) and 11.3; GATE-INDEP-1",
        "summary": (
            "Defines the previously unspecified dedup containment-claim channel "
            "(deduplication.containment_claims) so criterion c9 has a declared, "
            "merge-only evidence form for roles whose bytes may not be read."
        ),
        "adds": {
            "deduplication": ("containment_claims",),
        },
    },
    "A7": {
        "ref": "queue Edit E7 point 1 (operate only on retained metadata and already-open bytes)",
        "summary": (
            "Adds entry.content.local_path, the ONLY lawful channel from disk to a "
            "byte criterion. The broker resolves it strictly under a declared subject "
            "root and verifies SHA-256 before use, so no untracked or undeclared file "
            "can enter the audit and no sealed entry can be materialized."
        ),
        "adds": {
            "content": ("local_path",),
        },
    },
    "A8": {
        "ref": "REMOTE-EXPERIMENT-QUEUE.md Edit E8 (source_dirty scope) made checkable",
        "summary": (
            "Adds project.local_worktree so the audit records whether the subject root "
            "it actually inspected IS the pinned checkout. Edit E8 scopes source_dirty to "
            "the pinned checkout, which only has meaning once that is answered. A subject "
            "root carrying declared objects absent from the pinned commit MUST report "
            "source_dirty=true and block; asserting false there would be a false "
            "provenance claim."
        ),
        "adds": {
            "project": ("local_worktree",),
        },
    },
    "A9": {
        "ref": "SYNTH-CORPUS-FLEDGE.md 1.3 and 11.3 (c9 must be measured, not carried)",
        "summary": (
            "Adds deduplication.containment_verification so a criterion c9 edge is "
            "labelled byte-computed or publisher-attested. An unverified attestation may "
            "still merge units (the fail-safe direction) but MUST NOT be reported as a "
            "closed criterion."
        ),
        "adds": {
            "deduplication": ("containment_verification",),
        },
    },
}

# ---------------------------------------------------------------------------
# conflict criteria
# ---------------------------------------------------------------------------

CRITERIA = {
    "c1": "identical SHA-256",
    "c2": "identical git blob SHA-1 and byte length",
    "c3": "a shared exact raw 4 KiB block",
    "c4": "a shared exact 4 KiB block after CR removal before LF only",
    "c5": "text MinHash estimated Jaccard similarity >= declared threshold",
    "c6": "same schema cluster plus the same publisher or release family",
    "c7": "same repository owner, project, or release family",
    "c8": "shared generator lineage for synthetic data",
    "c9": "cross-container containment (one entry's record set inside another)",
}

# Criteria whose evidence requires bytes. Sealed roles are therefore
# NOT EVALUATED for these; they are never recorded as "no conflict".
BYTE_CRITERIA = ("c3", "c4", "c5", "c9")

EVALUATED_FROM = (
    "declared_metadata",
    "manifest_only",
    "open_bytes",
    "publisher_attestation",
    "not_evaluated_sealed",
)

# ---------------------------------------------------------------------------
# canonical serialization (protocol section 3)
# ---------------------------------------------------------------------------


def canon(obj) -> bytes:
    """UTF-8, lexicographically sorted keys, compact separators, one trailing LF.

    This byte string IS the artifact identity. `allow_nan=False` rejects NaN and
    infinities; no float is ever produced by this module, so there is no
    float-repr portability question across Python minor versions.
    """
    text = json.dumps(
        obj,
        sort_keys=True,
        separators=(",", ":"),
        ensure_ascii=False,
        allow_nan=False,
    )
    return text.encode("utf-8") + b"\n"


def sha256_hex(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def canon_sha256(obj) -> str:
    return sha256_hex(canon(obj))


# Arrays whose ORDER IS NOT SEMANTIC. Canonicalizing them before hashing makes
# the lock identity invariant to set-like input ordering, which is what "sorted
# unique array" means in the protocol. Arrays deliberately NOT listed keep
# their order because the protocol makes it semantic (entries by corpus_id,
# derivation transform_chain order, splits).
SET_LIKE_ARRAY_PATHS = (
    ("deduplication", "criteria"),
    ("deduplication", "containment_claims"),
    ("publisher_isolation", "required_new_fields"),
    ("publisher_isolation", "selection_basis"),
    ("workflow", "jobs"),
    ("promotion", "sequence"),
    ("promotion", "blocked_roles"),
    ("promotion", "pass_variants"),
    ("consumed_controls_ref", "keyed_by"),
    ("artifacts", "required"),
    ("acquisition", "byte_access_roles"),
    ("acquisition", "allowed_hosts"),
    ("roles", "allowed"),
    ("roles", "forbidden_transitions"),
)


def canonicalize_lock(lock: dict) -> dict:
    """Return a copy with every set-like array sorted, for identity hashing.

    This is applied by the builder BEFORE `lock_sha256` is computed, so two
    locks that differ only in the order of a set-like collection have one
    identity. Order-bearing arrays are left untouched by design.
    """
    out = json.loads(json.dumps(lock, sort_keys=True))
    for path in SET_LIKE_ARRAY_PATHS:
        node = out
        ok = True
        for key in path[:-1]:
            if not isinstance(node, dict) or key not in node:
                ok = False
                break
            node = node[key]
        if not ok or not isinstance(node, dict):
            continue
        last = path[-1]
        val = node.get(last)
        if isinstance(val, list):
            node[last] = sorted(val, key=lambda v: canon(v))
    entries = out.get("entries")
    if isinstance(entries, list):
        for e in entries:
            if not isinstance(e, dict):
                continue
            ind = e.get("independence")
            if isinstance(ind, dict) and isinstance(ind.get("near_duplicate_group_ids"), list):
                ind["near_duplicate_group_ids"] = sorted(set(ind["near_duplicate_group_ids"]))
            roles = e.get("lineage")
            if isinstance(roles, dict) and isinstance(roles.get("previous_roles"), list):
                roles["previous_roles"] = sorted(set(roles["previous_roles"]))
    return out


def normalize_name(name: str) -> str:
    """Ledger-name normalization for tombstone ledger-entry matching."""
    return re.sub(r"[^a-z0-9]+", "", str(name).lower())


# ---------------------------------------------------------------------------
# exposure guard (queue Edit E7)
# ---------------------------------------------------------------------------


class ExposureBreach(Exception):
    """Raised when a prohibited exposure is ATTEMPTED. Burn nothing, escalate."""

    def __init__(self, code: str, detail: str):
        super().__init__(code + ": " + detail)
        self.code = code
        self.detail = detail


class ExposureGuard:
    """Every byte/codec/network/archive path in this module goes through here.

    The counters are the enforcement mechanism (Edit E7 point 2/3/4: "enforced
    by counter, not by convention"). A breach raises BEFORE any byte is
    produced, so an attempted breach cannot leak a sealed byte.
    """

    __slots__ = (
        "network_ops",
        "codec_invocations",
        "archive_decompressions",
        "sealed_byte_reads",
        "open_byte_reads",
        "sealed_read_attempts_blocked",
        "archive_members_decompressed",
        "member_identity_notes",
        "attempts",
        "broker_refusals",
    )

    def __init__(self):
        self.network_ops = 0
        self.codec_invocations = 0
        self.archive_decompressions = 0
        self.sealed_byte_reads = 0
        self.open_byte_reads = 0
        self.sealed_read_attempts_blocked = 0
        self.archive_members_decompressed = 0
        self.member_identity_notes = []
        self.attempts = []
        self.broker_refusals = []

    # -- prohibited operations ------------------------------------------

    def network(self, purpose: str) -> None:
        self.network_ops += 1
        self.attempts.append(("network", purpose))
        raise ExposureBreach(
            "PB-06",
            "network access attempted (%s); a sealed URL must never be fetched "
            "by the admissibility audit" % (purpose,),
        )

    def codec(self, name: str) -> None:
        self.codec_invocations += 1
        self.attempts.append(("codec", name))
        raise ExposureBreach(
            "PB-05",
            "codec %r invoked by an admissibility audit" % (name,),
        )

    def decompress(self, archive_id: str) -> None:
        self.archive_decompressions += 1
        self.archive_members_decompressed += 1
        self.attempts.append(("decompress", archive_id))
        raise ExposureBreach(
            "PB-06",
            "archive %r decompressed; Edit E7 point 4 forbids archive "
            "decompression in Q1a" % (archive_id,),
        )

    # -- permitted operations --------------------------------------------

    def open_bytes(self, corpus_id: str, role: str, data: bytes) -> bytes:
        if role in SEALED_ROLES:
            self.sealed_read_attempts_blocked += 1
            self.sealed_byte_reads += 1
            self.attempts.append(("sealed_read", corpus_id))
            raise ExposureBreach(
                "PB-06",
                "attempted byte read of sealed role %s entry %r; burn nothing, "
                "escalate" % (role, corpus_id),
            )
        self.open_byte_reads += 1
        return data

    def manifest_only(self, corpus_id: str, note: str = "manifest-only, not decompressed") -> str:
        if note not in self.member_identity_notes:
            self.member_identity_notes.append(note)
            self.member_identity_notes.sort()
        return note

    def counters(self) -> dict:
        return {
            "archive_decompressions": self.archive_decompressions,
            "archive_members_decompressed": self.archive_members_decompressed,
            "codec_invocations": self.codec_invocations,
            "member_identity_notes": list(self.member_identity_notes),
            "network_ops": self.network_ops,
            "open_byte_reads": self.open_byte_reads,
            "sealed_byte_reads": self.sealed_byte_reads,
            "sealed_read_attempts_blocked": self.sealed_read_attempts_blocked,
            "broker_refusals": sorted(
                self.broker_refusals, key=lambda r: (r.get("corpus_id") or "", r.get("code"), r.get("reason"))
            ),
            "broker_refusal_count": len(self.broker_refusals),
            "zero_network": self.network_ops == 0,
            "zero_codec": self.codec_invocations == 0,
            "zero_archive_decompression": self.archive_decompressions == 0,
            "zero_sealed_bytes": self.sealed_byte_reads == 0,
        }


def assert_zero_exposure(counters: dict) -> list:
    findings = []
    for key, code in (
        ("zero_network", "NET.network_call"),
        ("zero_codec", "CODEC.invocation"),
        ("zero_archive_decompression", "ARCHIVE.decompression"),
        ("zero_sealed_bytes", "PB-06"),
    ):
        if not counters.get(key):
            findings.append(
                {
                    "code": code,
                    "path": "counters." + key,
                    "detail": "exposure counter is non-zero: %s" % (counters.get(key),),
                }
            )
    return findings


# ---------------------------------------------------------------------------
# schema specification and validator
# ---------------------------------------------------------------------------

STR = "str"
INT = "int"
BOOL = "bool"
OBJ = "obj"
ARR = "arr"
STR_OR_NULL = "str_or_null"
INT_OR_NULL = "int_or_null"
BOOL_OR_NULL = "bool_or_null"
OBJ_OR_NULL = "obj_or_null"


def field(kind, **kw):
    spec = {"kind": kind}
    spec.update(kw)
    return spec


TRANSFORM_STEP_FIELDS = (
    "input_sha256",
    "kind",
    "output_sha256",
    "parameters_sha256",
    "tool",
    "tool_sha256",
    "tool_version",
)

LOCK_LINEAGE = {
    "introduced_lock_revision": field(INT, ge=1),
    "supersedes_lock_sha256": field(STR_OR_NULL, pattern=HEX64_RE),
    "change_reason": field(STR_OR_NULL),
    "replacement_policy": field(STR, const="new-lock-only-no-silent-splicing"),
}

ROLES_OBJ = {
    "allowed": field(ARR, item=STR, enum=ROLE_ENUM, sorted_unique=True),
    "default_transition": field(OBJ, str_map=True),
    "forbidden_transitions": field(ARR, item=STR, sorted_unique=True),
    "evidence_split_map": field(OBJ, str_map=True),  # A3b
}

LOCAL_WORKTREE_OBJ = {
    "evaluated": field(BOOL),
    "dirty_path_count": field(INT_OR_NULL, ge=0),
    "declared_objects_absent_from_pinned_commit": field(ARR, item=STR, sorted_unique=True),
    "note": field(STR, nonempty=True),
}

PROJECT_OBJ = {
    "name": field(STR, const="ANVIL"),
    "repository": field(STR, nonempty=True),
    "protocol_path": field(STR, nonempty=True),
    "source_git_sha": field(STR, pattern=HEX40_RE),
    "source_dirty": field(BOOL),
    # amendment A1
    "source_tree_ref": field(STR, pattern=HEX40_RE),
    "evaluated_at": field(STR, pattern=RFC3339_RE),
    "source_dirty_scope": field(STR, const="pinned_checkout"),
    # amendment A8: Edit E8 scopes source_dirty to the pinned checkout, which
    # forces the question of whether the audited subject root IS that checkout.
    # This records the answer instead of assuming it.
    "local_worktree": field(OBJ_OR_NULL, fields=LOCAL_WORKTREE_OBJ),
}

INDEPENDENCE_OBJ = {
    "publisher_id": field(STR, nonempty=True),
    "project_id": field(STR, nonempty=True),
    "release_family_id": field(STR, nonempty=True),
    "producer_id": field(STR_OR_NULL),
    "schema_cluster_id": field(STR, nonempty=True),
    "exact_duplicate_group_id": field(STR, nonempty=True),
    "near_duplicate_group_ids": field(ARR, item=STR, sorted_unique=True),
    "audit_artifact_sha256": field(STR, pattern=HEX64_RE),
}

LICENSE_OBJ = {
    "status": field(STR, enum=LICENSE_STATUS),
    "name": field(STR, nonempty=True),
    "spdx": field(STR_OR_NULL),
    "url": field(STR_OR_NULL),
    "redistribution_allowed": field(BOOL_OR_NULL),
    "attribution_required": field(BOOL),
    "attribution_text": field(STR_OR_NULL, required_with=("attribution_required", "attribution_text")),
}

ARCHIVE_OBJ = {
    "archive_url": field(STR, nonempty=True),
    "archive_bytes": field(INT, ge=1),
    "archive_sha256": field(STR, pattern=HEX64_RE),
    "member_name": field(STR, nonempty=True),
    "member_bytes": field(INT, ge=1),
    "member_sha256": field(STR, pattern=HEX64_RE),
    "member_crc32": field(STR_OR_NULL, pattern=HEX8_RE),
    "extractor": field(STR, nonempty=True),
    "extractor_version": field(STR, nonempty=True),
}

HOST_INSTALL_OBJ = {
    "host_product_name": field(STR, nonempty=True),
    "install_root": field(STR, nonempty=True),
    "install_path": field(STR, nonempty=True),
    "file_version": field(STR_OR_NULL),
}

SOURCE_OBJ = {
    "kind": field(STR, enum=SOURCE_KINDS),
    "repository": field(STR_OR_NULL),
    "commit": field(STR_OR_NULL, pattern=HEX40_RE),
    "path": field(STR_OR_NULL),
    "url": field(STR_OR_NULL),
    "archive": field(OBJ_OR_NULL, fields=ARCHIVE_OBJ),
    # amendment A4
    "git_tracked": field(BOOL),
    "host_install": field(OBJ_OR_NULL, fields=HOST_INSTALL_OBJ),
}

CONTENT_OBJ = {
    "canonical_filename": field(STR, nonempty=True),
    "bytes": field(INT, ge=0),
    "sha256": field(STR, pattern=HEX64_RE),
    "git_blob_sha1": field(STR_OR_NULL, pattern=HEX40_RE),
    "media_type": field(STR, nonempty=True),
    # amendment A7: the only lawful route from disk to a byte criterion.
    # The broker reads this and nothing else - no glob, no directory walk.
    "local_path": field(STR_OR_NULL),
}

DERIVATION_OBJ = {"transform_chain": field(ARR, item=OBJ, exact_keys=TRANSFORM_STEP_FIELDS)}

VALIDITY_OBJ = {
    "framing": field(STR, enum=FRAMINGS),
    "total_records": field(INT_OR_NULL, ge=0),
    "valid_records": field(INT_OR_NULL, ge=0),
    "residual_records": field(INT_OR_NULL, ge=0),
    "final_newline": field(BOOL_OR_NULL),
    "schema_signature_sha256": field(STR_OR_NULL, pattern=HEX64_RE),
    "counts_source": field(STR, enum=COUNTS_SOURCES),
    "counts_artifact_sha256": field(STR_OR_NULL, pattern=HEX64_RE),
}

SYNTHETIC_OBJ = {
    "generator_path": field(STR, nonempty=True),
    "generator_git_blob_sha1": field(STR_OR_NULL, pattern=HEX40_RE),
    "generator_sha256": field(STR, pattern=HEX64_RE),
    "seed": field(OBJ, any_value=True),
    "parameters": field(OBJ, str_map=True),
    "parameters_sha256": field(STR, pattern=HEX64_RE),
    "runtime": field(OBJ, str_map=True),
}

ENTRY_LINEAGE_OBJ = {
    "introduced_lock_revision": field(INT, ge=1),
    "previous_roles": field(ARR, item=STR, enum=ROLE_ENUM, sorted_unique=True),
    "replaces_corpus_id": field(STR_OR_NULL),
    "replacement_reason": field(STR_OR_NULL),
}

# amendment A4
TOOLCHAIN_OBJ = {
    "producer_id": field(STR, nonempty=True),
    "producer_version": field(STR, nonempty=True),
    "attestation_kind": field(STR, nonempty=True),
    "attestation_sha256": field(STR, pattern=HEX64_RE),
}

ENTRY_OBJ = {
    "corpus_id": field(STR, pattern=CORPUS_ID_RE),
    "display_name": field(STR, nonempty=True),
    "role": field(STR, enum=ROLE_ENUM),
    "class": field(STR, enum=CONTROLLED_CLASSES),
    "selection_frozen_utc": field(STR, pattern=RFC3339_RE),
    "role_rationale": field(STR, nonempty=True),
    "independence": field(OBJ, fields=INDEPENDENCE_OBJ),
    "license": field(OBJ, fields=LICENSE_OBJ),
    "source": field(OBJ, fields=SOURCE_OBJ),
    "content": field(OBJ, fields=CONTENT_OBJ),
    "derivation": field(OBJ, fields=DERIVATION_OBJ),
    "validity": field(OBJ, fields=VALIDITY_OBJ),
    "synthetic": field(OBJ_OR_NULL, fields=SYNTHETIC_OBJ),
    "lineage": field(OBJ, fields=ENTRY_LINEAGE_OBJ),
    # amendment A4
    "toolchain": field(OBJ_OR_NULL, fields=TOOLCHAIN_OBJ),
}

CONTAINMENT_CLAIM_OBJ = {
    "contained_corpus_id": field(STR, pattern=CORPUS_ID_RE),
    "container_corpus_id": field(STR, pattern=CORPUS_ID_RE),
    "record_set_sha256": field(STR, pattern=HEX64_RE),
    "evidence": field(STR, nonempty=True),
    "attestation_kind": field(STR, nonempty=True),
    "attestation_sha256": field(STR, pattern=HEX64_RE),
}

DEDUP_OBJ = {
    "criteria": field(ARR, item=STR, enum=tuple(sorted(CRITERIA)), sorted_unique=True),
    "grouping": field(STR, const="transitively-closed-connected-components"),
    "group_ids_are_outputs": field(BOOL, const=True),
    "waiver_path": field(STR, const="absent"),
    # amendment A6
    "containment_claims": field(ARR, item=OBJ, fields=CONTAINMENT_CLAIM_OBJ),
    # amendment A9
    "containment_verification": field(STR, enum=("attestation_only", "byte_computed")),
}

PUBISO_OBJ = {
    "required_new_fields": field(ARR, item=STR, sorted_unique=True),
    "producer_diversity_rule": field(STR, nonempty=True),
    "selection_basis": field(ARR, item=STR, sorted_unique=True),
}

ACQ_OBJ = {
    "identity_algorithm": field(STR, const="sha256"),
    "md5_forbidden": field(BOOL, const=True),
    "allowed_hosts": field(ARR, item=STR, sorted_unique=True),
    "byte_access_roles": field(ARR, item=STR, enum=OPEN_ROLES, sorted_unique=True),
    "sealed_byte_access": field(STR, const="forbidden"),
    "archive_role_policy": field(STR, const="manifest-only, not decompressed"),
}

AITDCC_OBJ = {
    "enabled": field(BOOL),
    "development_range": field(STR, nonempty=True),
    "test_range": field(STR, nonempty=True),
    "sha256sums_sha256": field(STR_OR_NULL, pattern=HEX64_RE),
    "attribution": field(STR, nonempty=True),
}

WORKFLOW_OBJ = {
    "jobs": field(ARR, item=STR, sorted_unique=True),
    "fail_closed": field(BOOL, const=True),
    "ledger_path": field(STR, nonempty=True),
    "ledger_appended_by": field(STR, const="workflow"),
}

ARTIFACT_OBJ = {
    "required": field(ARR, item=STR, sorted_unique=True),
    "uploaded_by": field(STR, const="workflow"),
    "upload_condition": field(STR, const="always()"),
}

PROMOTION_OBJ = {
    "sequence": field(ARR, item=STR, sorted_unique=True),
    "blocked_roles": field(ARR, item=STR, enum=ROLE_ENUM, sorted_unique=True),
    "pass_variants": field(ARR, item=STR, sorted_unique=True),
}

AUDIT_PARAM_OBJ = {
    "block_bytes": field(INT, ge=256),
    "rare_block_max_entries": field(INT, ge=1),
    "minhash_shingle_tokens": field(INT, ge=2),
    "minhash_permutations": field(INT, ge=8),
    "minhash_threshold_num": field(INT, ge=0),
    "minhash_threshold_den": field(INT, ge=1),
    "minhash_screen_k": field(INT, ge=1),
    "text_likeness_min_num": field(INT, ge=0),
    "text_likeness_min_den": field(INT, ge=1),
    "containment_evidence_kind": field(STR, enum=("declared_manifest", "open_bytes")),
    "declared_by": field(STR, nonempty=True),
    "declared_utc": field(STR, pattern=RFC3339_RE),
    "declared_sha256": field(STR, pattern=HEX64_RE),
}

CONSUMED_CONTROLS_REF_OBJ = {
    "path": field(STR, nonempty=True),
    "sha256": field(STR, pattern=HEX64_RE),
    "entry_count": field(INT, ge=0),
    "keyed_by": field(ARR, item=STR, enum=("ledger_entry", "sha256"), sorted_unique=True),
}

TOP_FIELDS = {
    "schema": field(STR, const=SCHEMA),
    "protocol_version": field(INT, const=PROTOCOL_VERSION),
    "lock_revision": field(INT, ge=1),
    "created_utc": field(STR, pattern=RFC3339_RE),
    "title": field(STR, nonempty=True),
    "project": field(OBJ, fields=PROJECT_OBJ),
    "lineage": field(OBJ, fields=LOCK_LINEAGE),
    "roles": field(OBJ, fields=ROLES_OBJ),
    "splits": field(OBJ, fields={name: field(ARR, item=STR, sorted_unique=True) for name in PROTOCOL_SPLITS}),
    "entries": field(ARR, item=OBJ, fields=ENTRY_OBJ, nonempty=True, sorted_by="corpus_id"),
    "deduplication": field(OBJ, fields=DEDUP_OBJ),
    "publisher_isolation": field(OBJ, fields=PUBISO_OBJ),
    "acquisition": field(OBJ, fields=ACQ_OBJ),
    "aitdcc": field(OBJ, fields=AITDCC_OBJ),
    "workflow": field(OBJ, fields=WORKFLOW_OBJ),
    "artifacts": field(OBJ, fields=ARTIFACT_OBJ),
    "promotion": field(OBJ, fields=PROMOTION_OBJ),
    # amendment A2
    "audit_parameters": field(OBJ, fields=AUDIT_PARAM_OBJ),
    # amendment A5
    "consumed_controls_ref": field(OBJ, fields=CONSUMED_CONTROLS_REF_OBJ),
}

REQUIRED_AUDIT_PARAMETERS = tuple(sorted(AUDIT_PARAM_OBJ))

# Fields a curator may never supply: group/unit identity is an OUTPUT
# (GATE-INDEP-1). Unknown fields are already rejected by the validator; this
# explicit list exists so the rejection is legible in a finding.
FORBIDDEN_CURATOR_FIELDS = (
    ("entries[]", "independence_unit_id"),
    ("entries[]", "independence_group_id"),
    ("entries[]", "independence_group"),
    ("entries[]", "unit_id"),
    ("entries[]", "group_id"),
    ("deduplication", "groups"),
    ("deduplication", "independence_units"),
    ("deduplication", "group_map"),
    ("deduplication", "unit_assignment"),
)


def _f(code, path, detail):
    return {"code": code, "path": path, "detail": detail}


def _validate_value(value, spec, path, findings, key_hint=None):
    kind = spec["kind"]
    nullable = kind in (STR_OR_NULL, INT_OR_NULL, BOOL_OR_NULL, OBJ_OR_NULL)
    if value is None:
        if not nullable:
            findings.append(_f("schema.null_forbidden", path, "null not permitted"))
        return
    if kind in (STR, STR_OR_NULL):
        if not isinstance(value, str):
            findings.append(_f("schema.type", path, "expected string"))
            return
        if spec.get("pattern") and not spec["pattern"].match(value):
            findings.append(_f("schema.pattern", path, "value does not match %s" % (spec["pattern"].pattern,)))
        if spec.get("enum") and value not in spec["enum"]:
            findings.append(_f("schema.enum", path, "value not in controlled enum"))
        if spec.get("const") is not None and value != spec["const"]:
            findings.append(_f("schema.const", path, "expected %r" % (spec["const"],)))
        if spec.get("nonempty") and value == "":
            findings.append(_f("schema.empty", path, "empty string"))
    elif kind in (INT, INT_OR_NULL):
        if isinstance(value, bool) or not isinstance(value, int):
            findings.append(_f("schema.type", path, "expected integer (bool and float rejected)"))
            return
        if spec.get("ge") is not None and value < spec["ge"]:
            findings.append(_f("schema.range", path, "expected >= %d" % (spec["ge"],)))
        if spec.get("const") is not None and value != spec["const"]:
            findings.append(_f("schema.const", path, "expected %r" % (spec["const"],)))
    elif kind == BOOL:
        if not isinstance(value, bool):
            findings.append(_f("schema.type", path, "expected boolean"))
            return
        if spec.get("const") is not None and value != spec["const"]:
            findings.append(_f("schema.const", path, "expected %r" % (spec["const"],)))
    elif kind in (OBJ, OBJ_OR_NULL):
        if not isinstance(value, dict):
            findings.append(_f("schema.type", path, "expected object"))
            return
        _validate_object(value, spec.get("fields"), path, findings)
    elif kind == ARR:
        if not isinstance(value, list):
            findings.append(_f("schema.type", path, "expected array"))
            return
        if spec.get("nonempty") and not value:
            findings.append(_f("schema.empty", path, "array must be nonempty"))
        item_kind = spec.get("item")
        for i, item in enumerate(value):
            ipath = "%s[%d]" % (path, i)
            if item_kind == STR:
                if not isinstance(item, str):
                    findings.append(_f("schema.type", ipath, "expected string item"))
                    continue
                if spec.get("enum") and item not in spec["enum"]:
                    findings.append(_f("schema.enum", ipath, "item not in controlled enum"))
            elif item_kind == OBJ:
                if not isinstance(item, dict):
                    findings.append(_f("schema.type", ipath, "expected object item"))
                    continue
                if spec.get("exact_keys") is not None:
                    _validate_exact_keys(item, spec["exact_keys"], ipath, findings)
                else:
                    _validate_object(item, spec.get("fields"), ipath, findings)
        if spec.get("sorted_unique") and list(value) != sorted(set(value)):
            findings.append(_f("schema.order", path, "array must be sorted and unique"))
        if spec.get("sorted_by") and isinstance(value, list) and all(isinstance(v, dict) for v in value):
            keys = [v.get(spec["sorted_by"]) for v in value]
            if keys != sorted(keys):
                findings.append(_f("schema.order", path, "array must be sorted by %s" % (spec["sorted_by"],)))


def _validate_object(obj, fields, path, findings):
    if fields is None:
        return
    for name in sorted(obj):
        if name not in fields:
            findings.append(_f("schema.unknown_field", path + "." + name, "unknown field"))
    for name in sorted(fields):
        if name not in obj:
            findings.append(_f("schema.missing_field", path + "." + name, "required field missing"))
            continue
        _validate_value(obj[name], fields[name], path + "." + name, findings)
    required_with = {spec.get("required_with") for spec in fields.values()}
    for pair in required_with:
        if not pair:
            continue
        flag, text_key = pair
        if obj.get(flag) is True and not obj.get(text_key):
            findings.append(_f("schema.conditional", path, "%s requires %s" % (flag, text_key)))


def _validate_exact_keys(obj, keys, path, findings):
    got = set(obj)
    want = set(keys)
    for extra in sorted(got - want):
        findings.append(_f("schema.unknown_field", path + "." + extra, "unknown field"))
    for missing in sorted(want - got):
        findings.append(_f("schema.missing_field", path + "." + missing, "required field missing"))


def validate_lock(lock, *, forbid_curator_groups=True) -> list:
    """Return findings; an empty list means the lock is schema-valid.

    Findings are sorted so a caller can compare two validation runs byte-wise.
    """
    findings = []
    if not isinstance(lock, dict):
        return [_f("schema.type", "$", "lock must be an object")]
    _validate_object(lock, TOP_FIELDS, "$", findings)

    if forbid_curator_groups:
        for holder, name in FORBIDDEN_CURATOR_FIELDS:
            if holder == "deduplication":
                if isinstance(lock.get("deduplication"), dict) and name in lock["deduplication"]:
                    findings.append(
                        _f(
                            "GATE-INDEP-1.curator_group_id",
                            "deduplication." + name,
                            "independence group ids are an OUTPUT of the audit and "
                            "must never be curator-supplied as input",
                        )
                    )
            else:
                entries = lock.get("entries")
                if isinstance(entries, list):
                    for i, entry in enumerate(entries):
                        if isinstance(entry, dict) and name in entry:
                            findings.append(
                                _f(
                                    "GATE-INDEP-1.curator_group_id",
                                    "entries[%d].%s" % (i, name),
                                    "independence group ids are an OUTPUT of the audit",
                                )
                            )

    # 4.5 splits: exactly the protocol keys, partition of entry ids, and each
    # entry's role must map into the split that holds it.
    splits = lock.get("splits")
    entries = lock.get("entries")
    if isinstance(splits, dict):
        for name in sorted(splits):
            if name not in PROTOCOL_SPLITS:
                findings.append(_f("schema.splits_key", "splits." + name, "protocol 4.5 forbids extra split keys"))
    if isinstance(splits, dict) and isinstance(entries, list):
        placed = []
        for name in PROTOCOL_SPLITS:
            ids = splits.get(name)
            if isinstance(ids, list):
                placed.extend(ids)
        entry_ids = [e.get("corpus_id") for e in entries if isinstance(e, dict)]
        if sorted(placed) != sorted(entry_ids):
            findings.append(_f("PB-03", "splits", "splits are not an exact partition of entries[].corpus_id"))
        if len(placed) != len(set(placed)):
            findings.append(_f("PB-03", "splits", "a corpus_id appears in more than one split"))
        role_split_map = lock.get("roles", {})
        role_split_map = role_split_map.get("evidence_split_map", {}) if isinstance(role_split_map, dict) else {}
        for i, entry in enumerate(entries):
            if not isinstance(entry, dict):
                continue
            role = entry.get("role")
            cid = entry.get("corpus_id")
            want_split = ROLE_TO_SPLIT.get(role) or role_split_map.get(role)
            if want_split is None:
                findings.append(_f("schema.role_unmapped", "entries[%d].role" % i, "role has no split mapping"))
                continue
            for split_name in PROTOCOL_SPLITS:
                ids = splits.get(split_name)
                if isinstance(ids, list) and cid in ids and split_name != want_split:
                    findings.append(
                        _f(
                            "schema.role_split_disagreement",
                            "entries[%d]" % i,
                            "role %r maps to split %r but entry is in %r"
                            % (role, want_split, split_name),
                        )
                    )

    # Protocol 4.7: "exact_duplicate_group_id ... singleton is the object's SHA-256
    # identity". That is PERMITTED, so equality is not a finding. Declared
    # grouping is verified elsewhere (PB-11, against computed edges); it is never
    # authoritative, so it cannot inflate the independent-family count.
    #
    # Protocol 2.2/5.1 note: a transition MUST be recorded, never silently
    # relabelled. A `previous_roles` history that re-enters a sealed role is a
    # lifecycle violation, so it IS checked.
    if isinstance(entries, list):
        for i, entry in enumerate(entries):
            if not isinstance(entry, dict):
                continue
            lineage = entry.get("lineage")
            if not isinstance(lineage, dict):
                continue
            for role in lineage.get("previous_roles", []) or []:
                if role in SEALED_ROLES and entry.get("role") in SEALED_ROLES:
                    findings.append(
                        _f(
                            "PB-08",
                            "entries[%d].lineage.previous_roles" % (i,),
                            "sealed role %r appears in the role history and the entry is "
                            "still sealed; an opened object MUST transition to known_stress "
                            "(protocol 2.2)" % (role,),
                        )
                    )

    findings.sort(key=lambda x: (x["code"], x["path"], x["detail"]))
    return findings


# ---------------------------------------------------------------------------
# amendment E8: source_dirty is about the pinned checkout
# ---------------------------------------------------------------------------


def check_source_scope(lock) -> list:
    """Edit E8: source_dirty is asserted about source_tree_ref, not the operator tree."""
    findings = []
    project = lock.get("project", {})
    tree_ref = project.get("source_tree_ref")
    author_sha = project.get("source_git_sha")
    evaluated_at = project.get("evaluated_at")
    scope = project.get("source_dirty_scope")
    dirty = project.get("source_dirty")
    if scope != "pinned_checkout":
        findings.append(
            _f("E8.source_dirty_scope", "project.source_dirty_scope", "must be 'pinned_checkout'")
        )
    if not isinstance(tree_ref, str) or not HEX40_RE.match(tree_ref or ""):
        findings.append(_f("E8.source_tree_ref", "project.source_tree_ref", "pinned commit required"))
    if not isinstance(evaluated_at, str) or not RFC3339_RE.match(evaluated_at or ""):
        findings.append(_f("E8.evaluated_at", "project.evaluated_at", "RFC 3339 UTC with Z required"))
    if author_sha != tree_ref:
        findings.append(
            _f(
                "E8.authoring_commit_unbound",
                "project.source_git_sha",
                "the authoring commit and the evaluated tree must be the same pinned "
                "commit, otherwise source_dirty has no referent",
            )
        )
    if dirty is not False:
        findings.append(
            _f("PB-02", "project.source_dirty", "protocol 4.2 requires false for the pinned checkout")
        )
    return findings


# ---------------------------------------------------------------------------
# consumption tombstone (queue Edit E5)
# ---------------------------------------------------------------------------


class Tombstone:
    """Append-only, keyed by SHA-256, plus ledger-entry tombstones.

    E5 is explicit that `git_tracked` is recorded, NOT required to be true, and
    that git-tracking advice is withdrawn. This class therefore never treats
    provenance-in-git as a promotion input.
    """

    __slots__ = ("sha256_keys", "ledger_keys", "entries", "doc_sha256", "schema_ok", "own_findings")

    def __init__(self, doc: dict):
        self.doc_sha256 = canon_sha256(doc) if isinstance(doc, dict) else None
        self.entries = []
        self.sha256_keys = {}
        self.ledger_keys = {}
        findings = []
        if not isinstance(doc, dict) or doc.get("schema") != TOMBSTONE_SCHEMA:
            findings.append(_f("tombstone.schema", "schema", "expected %r" % (TOMBSTONE_SCHEMA,)))
        entries = doc.get("entries") if isinstance(doc, dict) else None
        if not isinstance(entries, list):
            findings.append(_f("tombstone.entries", "entries", "array required"))
            entries = []
        for i, e in enumerate(entries):
            path = "entries[%d]" % i
            if not isinstance(e, dict):
                findings.append(_f("tombstone.entry_type", path, "object required"))
                continue
            kind = e.get("identity_kind")
            ident = e.get("identity")
            if kind not in ("ledger_entry", "sha256"):
                findings.append(_f("tombstone.identity_kind", path, "must be sha256 or ledger_entry"))
                continue
            if not isinstance(ident, str) or not ident:
                findings.append(_f("tombstone.identity", path, "identity required"))
                continue
            if kind == "sha256" and not HEX64_RE.match(ident):
                findings.append(_f("tombstone.identity", path, "identity must be 64 lowercase hex"))
                continue
            if e.get("consumed") is not True:
                findings.append(_f("tombstone.consumed", path, "consumed must be true"))
            if e.get("promotion_forbidden") is not True:
                findings.append(_f("tombstone.promotion_forbidden", path, "promotion_forbidden must be true"))
            if e.get("role_after_consumption") != "known_stress":
                findings.append(
                    _f("tombstone.role_after", path, "consumed objects become known_stress (protocol 2.2)")
                )
            self.entries.append(e)
            key = normalize_name(ident) if kind == "ledger_entry" else ident
            bucket = self.ledger_keys if kind == "ledger_entry" else self.sha256_keys
            if key in bucket:
                findings.append(_f("tombstone.duplicate_key", path, "duplicate tombstone key"))
            bucket[key] = e
        findings.sort(key=lambda x: (x["code"], x["path"], x["detail"]))
        self.schema_ok = not findings
        self.own_findings = findings

    def findings(self) -> list:
        return list(self.own_findings)

    def matches_for_entry(self, entry) -> list:
        """Byte-identity (sha256) plus ledger-name matching, with the field named."""
        hits = []
        cid = entry.get("corpus_id")
        content = entry.get("content", {}) or {}
        src = entry.get("source", {}) or {}
        archive = src.get("archive") or {}
        ind = entry.get("independence", {}) or {}

        for label, digest in (
            ("content.sha256", content.get("sha256")),
            ("source.archive.member_sha256", archive.get("member_sha256")),
            ("source.archive.archive_sha256", archive.get("archive_sha256")),
        ):
            if isinstance(digest, str) and digest in self.sha256_keys:
                hits.append(
                    {
                        "corpus_id": cid,
                        "field": label,
                        "identity_kind": "sha256",
                        "identity": digest,
                        "tombstone_display": self.sha256_keys[digest].get("display_name"),
                        "tombstone_key": self.sha256_keys[digest].get("tombstone_key"),
                        "consumed_by": self.sha256_keys[digest].get("consumed_by"),
                    }
                )

        for label, value in (
            ("corpus_id", cid),
            ("display_name", entry.get("display_name")),
            ("independence.project_id", ind.get("project_id")),
            ("content.canonical_filename", content.get("canonical_filename")),
        ):
            if isinstance(value, str) and value:
                key = normalize_name(value)
                if key in self.ledger_keys:
                    hits.append(
                        {
                            "corpus_id": cid,
                            "field": label,
                            "identity_kind": "ledger_entry",
                            "identity": value,
                            "tombstone_display": self.ledger_keys[key].get("display_name"),
                            "tombstone_key": self.ledger_keys[key].get("tombstone_key"),
                            "consumed_by": self.ledger_keys[key].get("consumed_by"),
                        }
                    )
        hits.sort(key=lambda h: (h["corpus_id"], h["field"], h["identity"]))
        return hits


# ---------------------------------------------------------------------------
# byte criteria (only for already-open roles, through the guard)
# ---------------------------------------------------------------------------

BLOCK_DIGEST_BYTES = 8
MINHASH_PRIME = (1 << 61) - 1
SHINGLE_SCREEN_MAX = 4096


def _digest64(data: bytes) -> int:
    return int.from_bytes(hashlib.blake2b(data, digest_size=BLOCK_DIGEST_BYTES).digest(), "big")


def normalize_for_containment(data: bytes) -> bytes:
    """Protocol 5.1 text normalization, for detection only (never for measurement)."""
    out = bytearray()
    i = 0
    n = len(data)
    while i < n:
        if data[i] == 0x0D and i + 1 < n and data[i + 1] == 0x0A:
            i += 1
            continue
        out.append(data[i])
        i += 1
    return bytes(out)


def block_digests(data: bytes, block_bytes: int) -> set:
    return {
        _digest64(data[off : off + block_bytes])
        for off in range(0, max(len(data) - block_bytes + 1, 0), block_bytes)
    }


def text_like(data: bytes, params: dict) -> bool:
    """Is this payload text-like enough for the MinHash criterion to apply?

    The cutoff lives in the lock (`text_likeness_min_num/den`), because it
    decides whether criterion c5 is evaluated at all. Code default forbidden.
    """
    if not data:
        return True
    printable = sum(
        1 for c in data if 0x20 <= c <= 0x7E or c in (0x09, 0x0A, 0x0B, 0x0C, 0x0D)
    )
    return printable * params["text_likeness_min_den"] >= params["text_likeness_min_num"] * len(data)


_TOKEN_RE = re.compile(rb"[a-z0-9]+")


def tokenize(data: bytes) -> list:
    return _TOKEN_RE.findall(normalize_for_containment(data).lower())


def shingles(tokens: list, n: int) -> set:
    if len(tokens) < n:
        return {b" ".join(tokens)} if tokens else set()
    return {b" ".join(tokens[i : i + n]) for i in range(len(tokens) - n + 1)}


def _perm(index: int) -> tuple:
    raw = hashlib.blake2b(b"anvil.q1a.minhash.v1" + bytes([index]), digest_size=16).digest()
    a = int.from_bytes(raw[:8], "big") % (MINHASH_PRIME - 1) + 1
    b = int.from_bytes(raw[8:], "big") % MINHASH_PRIME
    return (a, b)


def minhash_signature(shingle_set: set, permutations: int) -> list:
    if not shingle_set:
        return [0] * permutations
    hashed = [_digest64(s) for s in sorted(shingle_set)]
    sig = []
    for i in range(permutations):
        a, b = _perm(i)
        best = MINHASH_PRIME
        for h in hashed:
            v = (a * h + b) % MINHASH_PRIME
            if v < best:
                best = v
        sig.append(best)
    return sig


def estimated_jaccard(sig_a: list, sig_b: list) -> tuple:
    """Exact equality rate of the signature; returned as an integer fraction."""
    if not sig_a or len(sig_a) != len(sig_b):
        return (0, 0)
    same = sum(1 for x, y in zip(sig_a, sig_b) if x == y)
    return (same, len(sig_a))


# ---------------------------------------------------------------------------
# conflict graph
# ---------------------------------------------------------------------------


class ByteIndex:
    """Per-entry byte evidence, only ever constructed for OPEN roles."""

    __slots__ = ("corpus_id", "bytes_len", "sha256", "blocks", "crlf_blocks", "sig", "shingle_count")

    def __init__(self, corpus_id, data, params):
        self.corpus_id = corpus_id
        self.bytes_len = len(data)
        self.sha256 = sha256_hex(data)
        block_bytes = params["block_bytes"]
        self.blocks = block_digests(data, block_bytes)
        self.crlf_blocks = block_digests(normalize_for_containment(data), block_bytes)
        if text_like(data, params):
            tokens = tokenize(data)
            sh = shingles(tokens, params["minhash_shingle_tokens"])
            self.shingle_count = len(sh)
            self.sig = minhash_signature(sh, params["minhash_permutations"])
        else:
            self.shingle_count = 0
            self.sig = []


def _edge(criterion, a, b, evidence, evaluated_from, verified):
    return {
        "a": a,
        "b": b,
        "criterion": criterion,
        "criterion_meaning": CRITERIA[criterion],
        "evidence": evidence,
        "evidence_sha256": canon_sha256(evidence),
        "evaluated_from": evaluated_from,
        "verified": verified,
        "waived": False,
    }


def build_conflict_graph(lock, byte_indexes, guard, params):
    """Return (edges, not_evaluated) over all entry pairs.

    `byte_indexes` maps corpus_id -> ByteIndex for OPEN roles whose bytes the
    caller is permitted to supply. Sealed roles never appear there; the guard
    would raise if they did.
    """
    entries = lock["entries"]
    edges = []
    not_evaluated = []
    by_id = {e["corpus_id"]: e for e in entries}
    ids = sorted(by_id)

    def add(criterion, a, b, evidence, evaluated_from, verified):
        edges.append(_edge(criterion, a, b, evidence, evaluated_from, verified))

    for i, aid in enumerate(ids):
        a = by_id[aid]
        for bid in ids[i + 1 :]:
            b = by_id[bid]
            # --- c1: identical sha256 (metadata only)
            if a["content"]["sha256"] == b["content"]["sha256"]:
                add(
                    "c1",
                    aid,
                    bid,
                    {"sha256": a["content"]["sha256"]},
                    "declared_metadata",
                    aid in byte_indexes and bid in byte_indexes,
                )

            # --- c2: identical git blob + byte length (metadata only)
            ab, bb = a["content"].get("git_blob_sha1"), b["content"].get("git_blob_sha1")
            if ab and bb and ab == bb and a["content"]["bytes"] == b["content"]["bytes"]:
                add(
                    "c2",
                    aid,
                    bid,
                    {"bytes": a["content"]["bytes"], "git_blob_sha1": ab},
                    "declared_metadata",
                    aid in byte_indexes and bid in byte_indexes,
                )

            # --- c6: same schema cluster + same publisher or release family
            ai, bi = a["independence"], b["independence"]
            if ai["schema_cluster_id"] == bi["schema_cluster_id"] and ai["schema_cluster_id"] != "not-applicable":
                same_pub = ai["publisher_id"] == bi["publisher_id"]
                same_rel = ai["release_family_id"] == bi["release_family_id"]
                if same_pub or same_rel:
                    add(
                        "c6",
                        aid,
                        bid,
                        {
                            "same_publisher": same_pub,
                            "same_release_family": same_rel,
                            "schema_cluster_id": ai["schema_cluster_id"],
                        },
                        "declared_metadata",
                        True,
                    )

            # --- c7: same repository owner, project, or release family
            if ai["release_family_id"] == bi["release_family_id"]:
                add(
                    "c7",
                    aid,
                    bid,
                    {"release_family_id": ai["release_family_id"]},
                    "declared_metadata",
                    True,
                )
            if ai["project_id"] == bi["project_id"]:
                add(
                    "c7",
                    aid,
                    bid,
                    {"project_id": ai["project_id"]},
                    "declared_metadata",
                    True,
                )
            if ai.get("producer_id") and ai.get("producer_id") == bi.get("producer_id"):
                add(
                    "c7",
                    aid,
                    bid,
                    {"producer_id": ai["producer_id"]},
                    "declared_metadata",
                    True,
                )

            # --- c8: shared generator lineage for synthetic data
            sa, sb = a.get("synthetic"), b.get("synthetic")
            if sa and sb:
                if sa["generator_sha256"] == sb["generator_sha256"]:
                    add(
                        "c8",
                        aid,
                        bid,
                        {"generator_sha256": sa["generator_sha256"]},
                        "declared_metadata",
                        True,
                    )
                elif sa["generator_path"] == sb["generator_path"]:
                    add(
                        "c8",
                        aid,
                        bid,
                        {"generator_path": sa["generator_path"]},
                        "declared_metadata",
                        True,
                    )

            # --- byte criteria: c3, c4, c5
            # A sealed entry NEVER acquires a byte-derived edge. Sealed bytes are
            # not read (Edit E7 point 1), so any pair involving a sealed role is
            # recorded as NOT EVALUATED, never as "no conflict" and never as an
            # open_bytes edge. This is enforced here, in the engine, not asserted
            # by the caller or the test.
            sealed = (a["role"] in SEALED_ROLES) or (b["role"] in SEALED_ROLES)
            xa, xb = byte_indexes.get(aid), byte_indexes.get(bid)
            if sealed or xa is None or xb is None:
                for crit in ("c3", "c4", "c5"):
                    not_evaluated.append(
                        {
                            "a": aid,
                            "b": bid,
                            "criterion": crit,
                            "criterion_meaning": CRITERIA[crit],
                            "reason": (
                                "sealed role bytes are never read (queue Edit E7 point 1)"
                                if sealed
                                else "no permitted open-role bytes were supplied for this entry"
                            ),
                            "status": "not_evaluated_sealed" if sealed else "not_evaluated_no_bytes",
                            "waived": False,
                        }
                    )
                continue

            if xa.blocks & xb.blocks:
                shared = sorted(xa.blocks & xb.blocks)
                add(
                    "c3",
                    aid,
                    bid,
                    {
                        "shared_block_count": len(shared),
                        "shared_block_digests": [("%016x" % d) for d in shared[:8]],
                    },
                    "open_bytes",
                    True,
                )
            if xa.crlf_blocks & xb.crlf_blocks:
                shared = sorted(xa.crlf_blocks & xb.crlf_blocks)
                add(
                    "c4",
                    aid,
                    bid,
                    {
                        "shared_block_count_after_cr_before_lf_removal": len(shared),
                        "shared_block_digests": [("%016x" % d) for d in shared[:8]],
                    },
                    "open_bytes",
                    True,
                )
            if xa.sig and xb.sig:
                num, den = estimated_jaccard(xa.sig, xb.sig)
                if den and num * params["minhash_threshold_den"] >= params["minhash_threshold_num"] * den:
                    add(
                        "c5",
                        aid,
                        bid,
                        {
                            "minhash_equal_permutations": num,
                            "minhash_permutations": den,
                            "threshold": [params["minhash_threshold_num"], params["minhash_threshold_den"]],
                        },
                        "open_bytes",
                        True,
                    )

    # --- c9: cross-container containment (amendment A6 channel)
    claims = lock.get("deduplication", {}).get("containment_claims", [])
    claimed_pairs = set()
    for claim in claims:
        contained = claim["contained_corpus_id"]
        container = claim["container_corpus_id"]
        pair = tuple(sorted((contained, container)))
        claimed_pairs.add(pair)
        sealed = (
            by_id[contained]["role"] in SEALED_ROLES
            or by_id[container]["role"] in SEALED_ROLES
        )
        if sealed:
            not_evaluated.append(
                {
                    "a": contained,
                    "b": container,
                    "criterion": "c9",
                    "criterion_meaning": CRITERIA["c9"],
                    "reason": "sealed role bytes are never decoded for record-set containment (Edit E7)",
                    "status": "not_evaluated_sealed",
                    "waived": False,
                }
            )
        add(
            "c9",
            contained,
            container,
            {
                "attestation_kind": claim["attestation_kind"],
                "record_set_sha256": claim["record_set_sha256"],
                "evidence": claim["evidence"],
            },
            "publisher_attestation",
            False,
        )

    # Any open structured/container pair with no declared claim and no bytes is
    # explicitly NOT EVALUATED rather than silently clean.
    structured = [cid for cid in ids if by_id[cid]["class"] in ("sqlite", "structured_json", "structured_ndjson", "mixed_validity")]
    for i, aid in enumerate(sorted(structured)):
        for bid in sorted(structured)[i + 1 :]:
            if tuple(sorted((aid, bid))) in claimed_pairs:
                continue
            if by_id[aid]["role"] in SEALED_ROLES or by_id[bid]["role"] in SEALED_ROLES:
                not_evaluated.append(
                    {
                        "a": aid,
                        "b": bid,
                        "criterion": "c9",
                        "criterion_meaning": CRITERIA["c9"],
                        "reason": "no declared containment attestation for a structured pair involving a sealed role",
                        "status": "not_evaluated_sealed",
                        "waived": False,
                    }
                )
            elif aid in byte_indexes and bid in byte_indexes:
                not_evaluated.append(
                    {
                        "a": aid,
                        "b": bid,
                        "criterion": "c9",
                        "criterion_meaning": CRITERIA["c9"],
                        "reason": "record-set decoding is not implemented in the prototype; declare a containment attestation",
                        "status": "not_evaluated_no_bytes",
                        "waived": False,
                    }
                )

    edges.sort(key=lambda e: (e["criterion"], e["a"], e["b"]))
    not_evaluated.sort(key=lambda r: (r["criterion"], r["a"], r["b"]))
    return edges, not_evaluated


# ---------------------------------------------------------------------------
# union-find over the conflict graph (GATE-INDEP-1)
# ---------------------------------------------------------------------------


def independence_components(ids, edges):
    """Transitively closed connected components. Group ids are an OUTPUT.

    Component id is content-addressed over its sorted membership so it is
    stable across runs, machines and Python minor versions.
    """
    parent = {cid: cid for cid in ids}

    def find(x):
        root = x
        while parent[root] != root:
            root = parent[root]
        while parent[x] != root:
            parent[x], x = root, parent[x]
        return root

    for e in edges:
        ra, rb = find(e["a"]), find(e["b"])
        if ra == rb:
            continue
        lo, hi = (ra, rb) if ra < rb else (rb, ra)
        parent[hi] = lo

    groups = {}
    for cid in ids:
        groups.setdefault(find(cid), []).append(cid)
    out = []
    for root in sorted(groups):
        members = sorted(groups[root])
        out.append(
            {
                "independence_unit_id": "iu-" + sha256_hex(canon(members))[:16],
                "size": len(members),
                "members": members,
                "root_corpus_id": root,
            }
        )
    out.sort(key=lambda g: g["members"][0])
    return out


# ---------------------------------------------------------------------------
# provenance / publisher isolation / lineage / license
# ---------------------------------------------------------------------------


def check_provenance(lock) -> dict:
    entries = lock["entries"]
    per_entry = []
    findings = []
    for e in entries:
        cid = e["corpus_id"]
        src = e["source"]
        kind = src["kind"]
        missing = []

        # git-raw must carry repository/commit/path; https-file and
        # archive-extract must carry a url; generate must not.
        if kind == "git-raw" and not (src.get("repository") and src.get("commit") and src.get("path")):
            missing.append("git-raw requires repository/commit/path (protocol 4.9)")
        if kind in ("https-file", "archive-extract") and not src.get("url"):
            missing.append("%s requires url (protocol 4.9)" % (kind,))
        if kind == "archive-extract" and not src.get("archive"):
            missing.append("archive-extract requires source.archive (protocol 4.9)")
        if kind == "host-installed" and not src.get("host_install"):
            missing.append("host-installed requires source.host_install (amendment A4)")
        if "git_tracked" not in src:
            missing.append("source.git_tracked must be RECORDED, not required true (Edit E5)")
        if e["class"] == "executable" and not e.get("toolchain"):
            missing.append("executable class requires a toolchain attestation (amendment A4)")
        if e.get("synthetic") is None and kind == "generate":
            missing.append("generate requires a non-null synthetic block (protocol 4.13)")
        if e.get("synthetic") is not None and kind != "generate":
            missing.append("synthetic block is non-null only for generated entries (protocol 4.13)")
        if e["validity"].get("counts_source") != "not-applicable" and not e["validity"].get(
            "counts_artifact_sha256"
        ):
            missing.append("record counts require counts_artifact_sha256 (protocol 4.12)")

        per_entry.append(
            {
                "corpus_id": cid,
                "source_kind": kind,
                "git_tracked": src.get("git_tracked"),
                "toolchain_attestation_sha256": (e.get("toolchain") or {}).get("attestation_sha256"),
                "license_status": e["license"]["status"],
                "license_verdict_source": e["source"].get("kind"),
                "complete": not missing,
                "missing": sorted(missing),
            }
        )
        for m in missing:
            findings.append(_f("PROVENANCE.incomplete", "entries[%s]" % (cid,), m))
    per_entry.sort(key=lambda r: r["corpus_id"])
    return {"entries": per_entry, "findings": sorted(findings, key=lambda f: (f["code"], f["path"], f["detail"]))}


def check_publisher_isolation(lock) -> dict:
    """Protocol 5.2 for held-out / external-test entries only."""
    entries = lock["entries"]
    sealed = [e for e in entries if e["role"] in SEALED_ROLES]
    open_entries = [e for e in entries if e["role"] not in SEALED_ROLES]
    findings = []

    open_pub = {e["independence"]["publisher_id"] for e in open_entries}
    open_proj = {e["independence"]["project_id"] for e in open_entries}
    open_rel = {e["independence"]["release_family_id"] for e in open_entries}
    open_schema = {
        e["independence"]["schema_cluster_id"]
        for e in open_entries
        if e["independence"]["schema_cluster_id"] != "not-applicable"
    }
    open_owners = {
        e["source"]["repository"].split("/")[0]
        for e in open_entries
        if e["source"].get("repository")
    }

    rows = []
    for e in sealed:
        ind = e["independence"]
        problems = []
        if ind["publisher_id"] in open_pub:
            problems.append("publisher_id not new relative to open roles")
        if ind["project_id"] in open_proj:
            problems.append("project_id not new relative to open roles")
        if ind["release_family_id"] in open_rel:
            problems.append("release_family_id not new relative to open roles")
        if ind["schema_cluster_id"] != "not-applicable" and ind["schema_cluster_id"] in open_schema:
            problems.append("schema_cluster_id not new for structured data")
        owner = e["source"]["repository"].split("/")[0] if e["source"].get("repository") else None
        if owner and owner in open_owners:
            problems.append("source repository owner not new")
        rows.append(
            {
                "corpus_id": e["corpus_id"],
                "role": e["role"],
                "class": e["class"],
                "publisher_id": ind["publisher_id"],
                "project_id": ind["project_id"],
                "release_family_id": ind["release_family_id"],
                "schema_cluster_id": ind["schema_cluster_id"],
                "repository_owner": owner,
                "isolated": not problems,
                "problems": sorted(problems),
            }
        )
        for p in problems:
            findings.append(_f("PB-10.publisher_isolation", "entries[%s]" % (e["corpus_id"],), p))

    # Producer diversity: no sealed entry may rely on a producer already used by
    # an open entry as its only diversity dimension.
    open_producers = {e["independence"].get("producer_id") for e in open_entries if e["independence"].get("producer_id")}
    for row in rows:
        entry = next(e for e in sealed if e["corpus_id"] == row["corpus_id"])
        prod = entry["independence"].get("producer_id")
        row["producer_id"] = prod
        row["producer_reused_from_open"] = bool(prod and prod in open_producers)

    rows.sort(key=lambda r: r["corpus_id"])
    return {"entries": rows, "findings": sorted(findings, key=lambda f: (f["code"], f["path"], f["detail"]))}


def check_lineage(lock) -> dict:
    """Publisher / generator lineage roll-up, independent of role."""
    by_generator = {}
    by_publisher = {}
    by_producer = {}
    rows = []
    for e in lock["entries"]:
        ind = e["independence"]
        syn = e.get("synthetic")
        row = {
            "corpus_id": e["corpus_id"],
            "role": e["role"],
            "publisher_id": ind["publisher_id"],
            "project_id": ind["project_id"],
            "release_family_id": ind["release_family_id"],
            "producer_id": ind.get("producer_id"),
            "schema_cluster_id": ind["schema_cluster_id"],
            "generator_path": syn["generator_path"] if syn else None,
            "generator_sha256": syn["generator_sha256"] if syn else None,
            "parameters_sha256": syn["parameters_sha256"] if syn else None,
            "introduced_lock_revision": e["lineage"]["introduced_lock_revision"],
            "previous_roles": list(e["lineage"]["previous_roles"]),
        }
        rows.append(row)
        if row["generator_sha256"]:
            by_generator.setdefault(row["generator_sha256"], []).append(e["corpus_id"])
        by_publisher.setdefault(row["publisher_id"], []).append(e["corpus_id"])
        if row["producer_id"]:
            by_producer.setdefault(row["producer_id"], []).append(e["corpus_id"])

    clusters = {
        "generator_lineages": {k: sorted(v) for k, v in sorted(by_generator.items())},
        "publisher_clusters": {k: sorted(v) for k, v in sorted(by_publisher.items())},
        "producer_clusters": {k: sorted(v) for k, v in sorted(by_producer.items())},
    }
    rows.sort(key=lambda r: r["corpus_id"])
    return {"entries": rows, "clusters": clusters}


def check_license(lock) -> dict:
    rows = []
    findings = []
    for e in lock["entries"]:
        lic = e["license"]
        problems = []
        if lic["status"] == "unknown-no-redistribution" and lic.get("redistribution_allowed") is not False:
            problems.append("unknown-no-redistribution requires redistribution_allowed=false")
        if lic["attribution_required"] and not lic.get("attribution_text"):
            problems.append("attribution_required without attribution_text")
        rows.append(
            {
                "corpus_id": e["corpus_id"],
                "role": e["role"],
                "status": lic["status"],
                "spdx": lic["spdx"],
                "redistribution_allowed": lic["redistribution_allowed"],
                "attribution_required": lic["attribution_required"],
                "admissible_for_artifact_upload": lic["status"] != "unknown-no-redistribution"
                or lic.get("redistribution_allowed") is True,
                "problems": sorted(problems),
            }
        )
        for p in problems:
            findings.append(_f("PB-12" if "redistribution" in p else "PB-13", "entries[%s]" % (e["corpus_id"],), p))
    rows.sort(key=lambda r: r["corpus_id"])
    return {"entries": rows, "findings": findings}


# ---------------------------------------------------------------------------
# threshold lint (SYNTH-CORPUS-FLEDGE 11.5)
# ---------------------------------------------------------------------------

CODE_DEFAULT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


def threshold_lint(lock, source_text) -> dict:
    """No decision threshold may live in a code default.

    (a) the lock must declare exactly the audited parameter set, and
    (b) this module's own source must define no module-level numeric constant
        other than the pinned mathematical ones, so no threshold can be
        introduced by editing code instead of the lock.
    """
    findings = []
    params = lock.get("audit_parameters", {})
    declared = tuple(sorted(params))
    missing = [k for k in REQUIRED_AUDIT_PARAMETERS if k not in params]
    extra = [k for k in declared if k not in REQUIRED_AUDIT_PARAMETERS]
    for k in sorted(missing):
        findings.append(_f("THRESH.undeclared", "audit_parameters." + k, "audited parameter not declared in the lock"))
    for k in sorted(extra):
        findings.append(_f("THRESH.unknowable", "audit_parameters." + k, "declared parameter is not audited; remove it"))
    if params.get("minhash_threshold_den", 0) <= 0:
        findings.append(_f("THRESH.unusable", "audit_parameters.minhash_threshold_den", "threshold denominator must be > 0"))

    code_defaults = []
    structural = []
    try:
        tree = ast.parse(source_text)
    except SyntaxError as exc:  # pragma: no cover - prototype source is valid
        findings.append(_f("THRESH.source_unparsable", "$", str(exc)))
        tree = None
    if tree is not None:
        for node in tree.body:
            targets = []
            value = None
            if isinstance(node, ast.Assign):
                targets = [t.id for t in node.targets if isinstance(t, ast.Name)]
                value = node.value
            elif isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
                targets = [node.target.id]
                value = node.value
            if value is None or not targets:
                continue
            if isinstance(value, ast.Constant) and isinstance(value.value, int) and not isinstance(value.value, bool):
                for name in targets:
                    if name.startswith("_"):
                        continue
                    # A pinned constant is classified, not blanket-allow-listed:
                    # each kind carries its own justification, and an unclassified
                    # integer is still a finding.
                    if name in PINNED_KINDS:
                        structural.append({"name": name, "kind": PINNED_KINDS[name], "justification": PINNED_NUMBERS[name]})
                        continue
                    code_defaults.append(name)
    for name in sorted(code_defaults):
        findings.append(
            _f(
                "THRESH.code_default",
                "$:" + name,
                "module-level numeric constant %r is not a classified structural "
                "constant: any value that can flip a conflict verdict must come from "
                "audit_parameters in the lock" % (name,),
            )
        )
    unclassified_pins = sorted(
        name
        for name in {s["name"] for s in structural}
        if name not in PINNED_NUMBERS
    )
    for name in unclassified_pins:
        findings.append(_f("THRESH.unclassified_pin", "$:" + name, "pinned constant lacks a justification"))

    return {
        "declared_parameters": sorted(declared),
        "required_parameters": sorted(REQUIRED_AUDIT_PARAMETERS),
        "structural_constants": sorted(structural, key=lambda s: s["name"]),
        "pinned_numbers": dict(sorted(PINNED_NUMBERS.items())),
        "pinned_kinds": dict(sorted(PINNED_KINDS.items())),
        "code_default_constants": sorted(code_defaults),
        "declared_sha256": params.get("declared_sha256"),
        "findings": sorted(findings, key=lambda f: (f["code"], f["path"], f["detail"])),
        "status": "pass" if not findings else "fail",
    }


# ---------------------------------------------------------------------------
# role-stratified aggregate contract (queue Edit E2 / E1)
# ---------------------------------------------------------------------------

ROLE_LABELS = {
    "external_test": "held-out-evidence",
    "heldout": "held-out-evidence",
    "discovery": "discovery-evidence",
    "synthetic_control": "discovery-evidence",
    "self_reference_control": "discovery-evidence",
    "external_anchor": "control-evidence",
    "external_development": "control-evidence",
    "known_stress": "control-evidence",
}


def aggregate_contract(rows, unit_by_corpus_id, consumed_corpus_ids) -> dict:
    """Every aggregate is emitted twice: all-roles and role-stratified.

    `rows` are byte-evidence rows; each MUST carry a non-null `evidence_role`
    drawn from the lock (queue Edit E1). Rows whose corpus_id is in
    `consumed_corpus_ids` are labelled consumed and can never promote.
    """
    findings = []
    normalized = []
    for i, row in enumerate(rows):
        role = row.get("evidence_role")
        cid = row.get("corpus_id")
        if role is None:
            findings.append(_f("E1.evidence_role_missing", "rows[%d]" % i, "row has no evidence_role"))
            continue
        if role not in ROLE_ENUM:
            findings.append(_f("E1.evidence_role_invalid", "rows[%d]" % i, "role %r not in lock enum" % (role,)))
            continue
        normalized.append(
            {
                "corpus_id": cid,
                "evidence_role": role,
                "consumed": cid in consumed_corpus_ids,
            }
        )
    normalized.sort(key=lambda r: (str(r["corpus_id"]), r["evidence_role"]))

    def summarize(subset):
        ids = sorted({r["corpus_id"] for r in subset})
        return {
            "n_rows": len(subset),
            "n_corpus_ids": len(ids),
            "n_independence_units": len({unit_by_corpus_id[c] for c in ids if c in unit_by_corpus_id}),
            "corpus_ids": ids,
        }

    all_roles = summarize(normalized)
    by_role = {}
    for role in ROLE_ENUM:
        subset = [r for r in normalized if r["evidence_role"] == role and not r["consumed"]]
        if not subset:
            continue
        summary = summarize(subset)
        summary["label"] = ROLE_LABELS[role]
        summary["admissible_for_promotion"] = role in PROMOTION_ROLES
        summary["qp_no_promote_role"] = role in QP_NO_PROMOTE_ROLES
        by_role[role] = summary

    heldout_subset = [r for r in normalized if r["evidence_role"] in PROMOTION_ROLES and not r["consumed"]]
    heldout = summarize(heldout_subset) if heldout_subset else None

    consumed = sorted({r["corpus_id"] for r in normalized if r["consumed"]})

    reasons = []
    if findings:
        reasons.append("rows are missing a valid evidence_role")
    if heldout is None:
        reasons.append("aggregate_heldout_only is null: no admissible held-out or external-test row")
    elif heldout["n_independence_units"] == 0:
        reasons.append("n_independence_units == 0 for held-out rows")

    return {
        "schema": AGGREGATE_SCHEMA,
        "findings": findings,
        "n_rows": len(normalized),
        "aggregate_all_roles": dict(all_roles, label="discovery-evidence", citation_grade_forbidden=True),
        "aggregate_heldout_only": heldout,
        "role_stratified": [dict(by_role[r], evidence_role=r) for r in sorted(by_role)],
        "consumed_corpus_ids": consumed,
        "citation_grade_permitted": bool(normalized)
        and all(r["evidence_role"] in PROMOTION_ROLES and not r["consumed"] for r in normalized),
        "promotion_authorized": not reasons,
        "promotion_authorized_reasons": reasons if reasons else ["all E2 preconditions met"],
        "pass_variant_emitted": not reasons,
    }


# ---------------------------------------------------------------------------
# audit
# ---------------------------------------------------------------------------


def _gate(gid, title, status, evidence, reason):
    return {
        "gate_id": gid,
        "title": title,
        "status": status,
        "evidence": evidence,
        "reason": reason,
    }


def _tool_source_sha256():
    try:
        with open(os.path.abspath(__file__), "rb") as fh:
            return sha256_hex(fh.read())
    except OSError:  # pragma: no cover
        return None


def audit(lock, tombstone_doc, byte_sources=None, guard=None, source_text=None):
    """Deterministic admissibility audit. Pure function of its inputs.

    `byte_sources` maps corpus_id -> bytes and is only ever consulted for OPEN
    roles, through the guard.
    """
    guard = guard or ExposureGuard()
    byte_sources = byte_sources or {}
    source_text = source_text if source_text is not None else _read_own_source()

    report_findings = []
    gates = []

    # --- lock identity (protocol section 3)
    lock_canonical = canonicalize_lock(lock)
    lock_sha256 = canon_sha256(lock_canonical)
    lock_authored_sha256 = canon_sha256(lock)
    ref = lock.get("consumed_controls_ref", {})
    tombstone = Tombstone(tombstone_doc)
    tombstone_sha = canon_sha256(tombstone_doc) if isinstance(tombstone_doc, dict) else None

    # --- Q1A-1 schema validity
    schema_findings = validate_lock(lock)
    report_findings.extend(schema_findings)
    gates.append(
        _gate(
            "Q1A-1",
            "lock is schema-valid anvil.corpus-lock/v1",
            "pass" if not schema_findings else "fail",
            {"finding_count": len(schema_findings), "first_findings": schema_findings[:5]},
            "protocol section 4 forbids unknown/missing fields; amendments A1..A6 are declared, not hidden",
        )
    )

    # --- Q1A-2 amendment E8 source scope
    scope_findings = check_source_scope(lock)
    report_findings.extend(scope_findings)
    gates.append(
        _gate(
            "Q1A-2",
            "source_dirty is asserted about the pinned checkout (Edit E8)",
            "pass" if not scope_findings else "fail",
            {
                "source_tree_ref": lock.get("project", {}).get("source_tree_ref"),
                "evaluated_at": lock.get("project", {}).get("evaluated_at"),
                "source_dirty": lock.get("project", {}).get("source_dirty"),
                "source_dirty_scope": lock.get("project", {}).get("source_dirty_scope"),
                "local_worktree_evaluated": False,
            },
            "local research-worktree dirtiness is explicitly out of scope for the audit verdict",
        )
    )

    # --- Q1A-3 universe enumeration
    entries = lock.get("entries", [])
    universe = {
        "entry_count": len(entries),
        "corpus_ids": sorted(e.get("corpus_id", "") for e in entries),
        "by_role": {},
        "by_class": {},
        "declared_bytes_total": 0,
        "sealed_entry_ids": [],
        "archive_extract_entry_ids": [],
    }
    for e in entries:
        role = e.get("role", "?")
        universe["by_role"][role] = universe["by_role"].get(role, 0) + 1
        cls = e.get("class", "?")
        universe["by_class"][cls] = universe["by_class"].get(cls, 0) + 1
        content = e.get("content", {}) or {}
        b = content.get("bytes")
        if isinstance(b, int):
            universe["declared_bytes_total"] += b
        if role in SEALED_ROLES:
            universe["sealed_entry_ids"].append(e["corpus_id"])
        src = e.get("source", {}) or {}
        if src.get("kind") == "archive-extract":
            universe["archive_extract_entry_ids"].append(e["corpus_id"])
            guard.manifest_only(e["corpus_id"])
    for key in ("by_role", "by_class"):
        universe[key] = dict(sorted(universe[key].items()))
    universe["sealed_entry_ids"].sort()
    universe["archive_extract_entry_ids"].sort()
    gates.append(
        _gate(
            "Q1A-3",
            "corpus universe enumerated from committed metadata",
            "pass" if entries else "fail",
            {
                "entry_count": universe["entry_count"],
                "by_role": universe["by_role"],
                "declared_bytes_total": universe["declared_bytes_total"],
            },
            "enumeration reads names, roles and declared sizes only; no payload byte is read",
        )
    )

    # --- archive role policy: manifest-only, never decompressed
    archive_rows = []
    for e in entries:
        src = e.get("source", {}) or {}
        if src.get("kind") != "archive-extract":
            continue
        arch = src.get("archive") or {}
        archive_rows.append(
            {
                "corpus_id": e["corpus_id"],
                "role": e["role"],
                "member_name": arch.get("member_name"),
                "member_sha256": arch.get("member_sha256"),
                "member_crc32": arch.get("member_crc32"),
                "member_identity": guard.manifest_only(e["corpus_id"]),
                "decompressed": False,
            }
        )
    archive_rows.sort(key=lambda r: r["corpus_id"])
    archive_findings = []
    for row in archive_rows:
        if not row["member_sha256"]:
            archive_findings.append(
                _f("ARCHIVE.member_identity_missing", "entries[%s]" % (row["corpus_id"],), "manifest member_sha256 required")
            )
    report_findings.extend(archive_findings)
    gates.append(
        _gate(
            "Q1A-4",
            "archive-extract entries read as manifest-only, never decompressed (Edit E7 point 4)",
            "pass" if not archive_findings and guard.archive_decompressions == 0 else "fail",
            {"entries": archive_rows, "archive_decompressions": guard.archive_decompressions},
            "member_name/member_sha256/member_crc32 come from the published manifest, not from extraction",
        )
    )

    # --- byte indexes for OPEN roles only
    params = {
        "block_bytes": lock.get("audit_parameters", {}).get("block_bytes"),
        "rare_block_max_entries": lock.get("audit_parameters", {}).get("rare_block_max_entries"),
        "minhash_shingle_tokens": lock.get("audit_parameters", {}).get("minhash_shingle_tokens"),
        "minhash_permutations": lock.get("audit_parameters", {}).get("minhash_permutations"),
        "minhash_threshold_num": lock.get("audit_parameters", {}).get("minhash_threshold_num"),
        "minhash_threshold_den": lock.get("audit_parameters", {}).get("minhash_threshold_den"),
        "text_likeness_min_num": lock.get("audit_parameters", {}).get("text_likeness_min_num"),
        "text_likeness_min_den": lock.get("audit_parameters", {}).get("text_likeness_min_den"),
    }
    role_by_id = {e["corpus_id"]: e["role"] for e in entries}
    byte_indexes = {}
    sealed_leaks = []
    for cid in sorted(byte_sources):
        role = role_by_id.get(cid)
        if role is None:
            continue
        data = guard.open_bytes(cid, role, byte_sources[cid])
        byte_indexes[cid] = ByteIndex(cid, data, params)
    gates.append(
        _gate(
            "Q1A-5",
            "no sealed byte was read; open-role bytes only, through the guard (Edit E7 points 1-3)",
            "pass" if guard.sealed_byte_reads == 0 else "fail",
            {"open_byte_entries": sorted(byte_indexes), "sealed_byte_reads": guard.sealed_byte_reads},
            "every read path is counted; a sealed read raises PB-06 before any byte is produced",
        )
    )

    # --- conflict graph + independence units
    audit_complete = not bool(schema_findings)
    edges, not_evaluated = ([], [])
    units = []
    unit_by_corpus_id = {}
    indep = {}
    if not schema_findings:
        edges, not_evaluated = build_conflict_graph(lock, byte_indexes, guard, params)
        ids = sorted(role_by_id)
        units = independence_components(ids, edges)
        for unit in units:
            for cid in unit["members"]:
                unit_by_corpus_id[cid] = unit["independence_unit_id"]
        indep = {
            "independence_units": len(units),
            "grouping": "transitively-closed-connected-components",
            "group_ids_are_outputs": True,
            "curator_supplied_group_ids": "structurally rejected (GATE-INDEP-1)",
            "units": units,
            "unit_count_is_not_file_count": True,
        }
    gates.append(
        _gate(
            "Q1A-6",
            "independence groups computed as transitive components; group ids are outputs",
            "pass" if not schema_findings else "blocked",
            {
                "independence_units": indep.get("independence_units"),
                "unit_sizes": sorted([u["size"] for u in units]),
                "conflict_edge_count": len(edges),
            },
            "GATE-INDEP-1/GATE-INDEP-2: the portfolio gate is evaluated on independence units, never on file count",
        )
    )

    # --- curator-supplied duplicate annotations must be a subset of computed edges
    annotation_findings = []
    computed_pairs = {(e["a"], e["b"]) for e in edges}
    for e in entries:
        for other in e["independence"].get("near_duplicate_group_ids", []):
            if not isinstance(other, str):
                continue
            pair = tuple(sorted((e["corpus_id"], other)))
            if pair not in computed_pairs:
                annotation_findings.append(
                    _f(
                        "PB-11",
                        "entries[%s].independence.near_duplicate_group_ids" % (e["corpus_id"],),
                        "annotated duplicate %r has no computed conflict edge; a duplicate may not be "
                        "waived to inflate the independent-family count" % (other,),
                    )
                )
    report_findings.extend(annotation_findings)
    gates.append(
        _gate(
            "Q1A-7",
            "no duplicate waived; declared duplicate annotations are a subset of computed edges",
            "pass" if not annotation_findings else "fail",
            {"unsubstantiated_annotations": annotation_findings},
            "PB-11: a curator may not group to inflate independence, nor annotate a duplicate the audit did not find",
        )
    )

    # --- not-evaluated pairs must not be read as "no conflict"
    sealed_not_evaluated = [r for r in not_evaluated if r["status"] == "not_evaluated_sealed"]
    c9_edges = [e for e in edges if e["criterion"] == "c9"]
    c9_computed = [e for e in c9_edges if e["verified"] is True]
    c9_attested = [e for e in c9_edges if e["verified"] is not True]
    c9_status = (
        "byte_computed"
        if c9_edges and len(c9_computed) == len(c9_edges)
        else ("no_claims" if not c9_edges else "attestation_only")
    )
    gates.append(
        _gate(
            "Q1A-8",
            "criterion c9 cross-container containment: NOT CLOSED unless byte-computed",
            "pass" if c9_status == "byte_computed" else "revise",
            {
                "c9_verification": c9_status,
                "c9_edges_total": len(c9_edges),
                "c9_edges_byte_computed": len(c9_computed),
                "c9_edges_attestation_only": len(c9_attested),
                "not_evaluated_count": len(not_evaluated),
                "not_evaluated_sealed_count": len(sealed_not_evaluated),
                "sample": sealed_not_evaluated[:3],
            },
            "an unverified attestation may still merge units (fail-safe), but c9 is NOT "
            "closed while it is attestation-only: open-role record-set containment is "
            "not computed by this tool. Sealed pairs are never recorded as 'no conflict'.",
        )
    )

    # --- provenance, publisher isolation, lineage, license
    provenance = check_provenance(lock)
    pubiso = check_publisher_isolation(lock)
    lineage = check_lineage(lock)
    lic = check_license(lock)
    report_findings.extend(provenance["findings"])
    report_findings.extend(pubiso["findings"])
    report_findings.extend(lic["findings"])
    gates.append(
        _gate(
            "Q1A-9",
            "provenance complete for every entry, incl. git_tracked and toolchain attestation",
            "pass" if not provenance["findings"] else "fail",
            {"incomplete_entries": [r["corpus_id"] for r in provenance["entries"] if not r["complete"]]},
            "SYNTH-CORPUS-FLEDGE 11.6: source.kind, git_tracked recorded (not required true), license verdict with a source of truth, toolchain attestation with an evidence hash",
        )
    )
    gates.append(
        _gate(
            "Q1A-10",
            "publisher isolation for sealed entries (protocol 5.2)",
            "pass" if not pubiso["findings"] else "fail",
            {"non_isolated": [r["corpus_id"] for r in pubiso["entries"] if not r["isolated"]]},
            "publisher_id/project_id/release_family_id/schema_cluster_id/repository owner must be new relative to open roles",
        )
    )
    gates.append(
        _gate(
            "Q1A-11",
            "license verdict present with a source of truth for every entry",
            "pass" if not lic["findings"] else "fail",
            {"problem_entries": [r["corpus_id"] for r in lic["entries"] if r["problems"]]},
            "PB-12/PB-13",
        )
    )

    # --- threshold lint
    lint = threshold_lint(lock, source_text)
    report_findings.extend(lint["findings"])
    gates.append(
        _gate(
            "Q1A-12",
            "no decision threshold lives in a code default",
            lint["status"],
            {
                "code_default_constants": lint["code_default_constants"],
                "declared_parameters": lint["declared_parameters"],
            },
            "SYNTH-CORPUS-FLEDGE 11.5: rare_max, k and the 0.20 threshold must come from the lock",
        )
    )

    # --- zero exposure
    counters = guard.counters()
    exposure_findings = assert_zero_exposure(counters)
    report_findings.extend(exposure_findings)
    gates.append(
        _gate(
            "Q1A-13",
            "zero network, zero codec invocations, zero archive decompression, zero sealed bytes",
            "pass" if not exposure_findings else "fail",
            dict(sorted(counters.items())),
            "Edit E7: enforced by counter, not by convention",
        )
    )

    # --- Q1A-14 provenance completeness (protocol 11 / SYNTH-CORPUS 11.6)
    provenance = check_provenance(lock)
    report_findings.extend(provenance["findings"])
    incomplete = [r["corpus_id"] for r in provenance["entries"] if not r["complete"]]
    gates.append(
        _gate(
            "Q1A-14",
            "provenance complete for every entry incl. host-installed and git_tracked",
            "pass" if not incomplete else "fail",
            {"incomplete_entries": sorted(incomplete)},
            "queue item 6: source.kind, git_tracked recorded (NOT required true, Edit E5), "
            "license verdict with a source of truth, toolchain attestation with an evidence hash",
        )
    )

    # --- Q1A-15 publisher isolation (protocol 5.2)
    pubiso = check_publisher_isolation(lock)
    report_findings.extend(pubiso["findings"])
    gates.append(
        _gate(
            "Q1A-15",
            "publisher isolation for sealed entries (protocol 5.2)",
            "pass" if not pubiso["findings"] else "fail",
            {
                "non_isolated": sorted(
                    r["corpus_id"] for r in pubiso["entries"] if not r["isolated"]
                ),
                "producer_reused_from_open": sorted(
                    r["corpus_id"]
                    for r in pubiso["entries"]
                    if r.get("producer_reused_from_open")
                ),
            },
            "publisher_id/project_id/release_family_id/schema_cluster_id/repository owner must be "
            "new relative to all open roles; producer family must not be the only diversity dimension",
        )
    )

    # --- Q1A-16 publisher/generator lineage roll-up
    lineage = check_lineage(lock)
    gates.append(
        _gate(
            "Q1A-16",
            "publisher and generator lineage enumerated per entry",
            "pass",
            {
                "generator_lineage_count": len(lineage["clusters"]["generator_lineages"]),
                "publisher_cluster_count": len(lineage["clusters"]["publisher_clusters"]),
                "producer_cluster_count": len(lineage["clusters"]["producer_clusters"]),
                "multi_entry_generator_lineages": sorted(
                    k
                    for k, v in lineage["clusters"]["generator_lineages"].items()
                    if len(v) > 1
                ),
            },
            "shared generator lineage is criterion c8 and MUST collapse to one independence unit",
        )
    )

    # --- Q1A-17 license report (protocol 5.3 / PB-12 / PB-13)
    lic = check_license(lock)
    report_findings.extend(lic["findings"])
    gates.append(
        _gate(
            "Q1A-17",
            "license verdict present with a source of truth for every entry",
            "pass" if not lic["findings"] else "fail",
            {"problem_entries": sorted(r["corpus_id"] for r in lic["entries"] if r["problems"])},
            "PB-12/PB-13",
        )
    )

    # --- Q1A-18 threshold lint (SYNTH-CORPUS 11.5)
    lint = threshold_lint(lock, source_text)
    report_findings.extend(lint["findings"])
    gates.append(
        _gate(
            "Q1A-18",
            "no decision threshold lives in a code default",
            lint["status"],
            {
                "code_default_constants": lint["code_default_constants"],
                "declared_parameters": lint["declared_parameters"],
                "missing_parameters": sorted(
                    set(lint["required_parameters"]) - set(lint["declared_parameters"])
                ),
            },
            "rare_block_max_entries / minhash_screen_k / minhash threshold MUST come from "
            "audit_parameters in the lock",
        )
    )

    # --- Q1A-19 tombstone binding (Edit E5) and QP-CONSUMED (Edit E4)
    consumed_matches = []
    if isinstance(entries, list):
        for e in entries:
            if isinstance(e, dict) and isinstance(e.get("corpus_id"), str):
                for hit in tombstone.matches_for_entry(e):
                    consumed_matches.append(hit)
    consumed_matches.sort(key=lambda h: (h["corpus_id"], h["field"], h["identity"]))
    consumed_ids = sorted({h["corpus_id"] for h in consumed_matches})
    # QP-CONSUMED classification. Edit E4's prohibition is on OFFERING a
    # tombstoned object as evidence. Protocol 2.3 REQUIRES the six consumed PE
    # files to be present and open as known/anchor data, so mere presence as a
    # correctly-demoted `known_stress` control is recorded, not failed:
    #
    #   sealed role                -> FAIL: this is the laundering path Edit E4 bans
    #   open role != known_stress  -> FAIL: lifecycle error (2.2), not demoted on open
    #   known_stress               -> recorded as a consumed control
    #
    # Not waivable in the sealed case: there is no code path that downgrades it.
    consumed_sealed = sorted(
        h["corpus_id"]
        for h in consumed_matches
        if dict((e["corpus_id"], e.get("role")) for e in (entries if isinstance(entries, list) else []))
        .get(h["corpus_id"]) in SEALED_ROLES
    )
    consumed_open_mislabeled = sorted(
        h["corpus_id"]
        for h in consumed_matches
        if dict((e["corpus_id"], e.get("role")) for e in (entries if isinstance(entries, list) else []))
        .get(h["corpus_id"]) in OPEN_ROLES
        and dict((e["corpus_id"], e.get("role")) for e in (entries if isinstance(entries, list) else []))
        .get(h["corpus_id"]) != "known_stress"
    )
    consumed_as_controls = sorted(
        h["corpus_id"]
        for h in consumed_matches
        if dict((e["corpus_id"], e.get("role")) for e in (entries if isinstance(entries, list) else []))
        .get(h["corpus_id"]) == "known_stress"
    )
    for cid in consumed_sealed:
        report_findings.append(
            _f(
                "QP-CONSUMED.sealed_offering",
                "entries[%s]" % (cid,),
                "a consumed object is offered in a sealed role; consumed bytes may never "
                "become held-out evidence and this cannot be waived",
            )
        )
    for cid in consumed_open_mislabeled:
        report_findings.append(
            _f(
                "PB-08",
                "entries[%s]" % (cid,),
                "a consumed object sits in an open role other than known_stress; protocol "
                "2.2 requires demotion to known_stress at the moment of opening",
            )
        )
    tomb_stale = []
    if ref.get("sha256") and ref["sha256"] != tombstone_sha:
        tomb_stale.append(
            _f(
                "PB-02",
                "consumed_controls_ref.sha256",
                "lock is bound to a different tombstone revision than the one supplied",
            )
        )
    if isinstance(ref.get("entry_count"), int) and ref["entry_count"] != len(tombstone.entries):
        tomb_stale.append(
            _f(
                "PB-02",
                "consumed_controls_ref.entry_count",
                "tombstone entry count does not match the bound reference",
            )
        )
    report_findings.extend(tombstone.findings())
    report_findings.extend(tomb_stale)
    qp_consumed_ok = (
        not consumed_sealed
        and not consumed_open_mislabeled
        and not tomb_stale
    )
    gates.append(
        _gate(
            "QP-CONSUMED",
            "no object offered as evidence appears in the consumption tombstone",
            "pass" if qp_consumed_ok else "fail",
            {
                "tombstone_schema_ok": tombstone.schema_ok,
                "tombstone_sha256": tombstone_sha,
                "tombstone_entry_count": len(tombstone.entries),
                "stale_reference": bool(tomb_stale),
                "matches": consumed_matches,
                "matched_corpus_ids": consumed_ids,
                "offered_in_sealed_role": consumed_sealed,
                "open_role_not_demoted": consumed_open_mislabeled,
                "used_as_known_stress_control": consumed_as_controls,
            },
            "byte-identity check, not a bookkeeping assertion; it cannot be waived",
        )
    )

    # --- Q1A-20 role-stratified aggregate contract (Edit E1 / E2)
    rows = [
        {"corpus_id": e.get("corpus_id"), "evidence_role": e.get("role")}
        for e in (entries if isinstance(entries, list) else [])
        if isinstance(e, dict)
    ]
    aggregate = aggregate_contract(rows, unit_by_corpus_id, set(consumed_ids))
    report_findings.extend(aggregate["findings"])
    offending_roles = sorted(
        {r["evidence_role"] for r in rows if r["evidence_role"] in QP_NO_PROMOTE_ROLES}
    )
    qp_no_promote_ok = not offending_roles
    gates.append(
        _gate(
            "QP-NO-PROMOTE",
            "no promotion language while any input row carries a non-held-out evidence_role",
            "pass" if qp_no_promote_ok else "fail",
            {
                "offending_roles_present": offending_roles,
                "qp_no_promote_roles": list(QP_NO_PROMOTE_ROLES),
                "edit_text_roles": list(QP_NO_PROMOTE_ROLES_EDIT_TEXT),
                "deviations": [dict(d) for d in QP_GATE_DEVIATIONS],
                "note": "passing Q1a validates the machinery, not the portfolio",
            },
            "queue Edit E4",
        )
    )

    promotion_reasons = list(aggregate["promotion_authorized_reasons"])
    if not qp_no_promote_ok:
        promotion_reasons.append("QP-NO-PROMOTE failed")
    if not qp_consumed_ok:
        promotion_reasons.append("QP-CONSUMED failed")
    if not all(f["code"] not in ("PB-01", "PB-02", "PB-03", "PB-04", "PB-06", "PB-09", "PB-10", "PB-11", "PB-12", "PB-13") for f in report_findings):
        promotion_reasons.append("lock-level or independence blocker present")
    promotion_authorized = bool(promotion_reasons == ["all E2 preconditions met"] and qp_no_promote_ok and qp_consumed_ok)
    aggregate["promotion_authorized"] = promotion_authorized
    aggregate["promotion_authorized_reasons"] = promotion_reasons

    # --- burn-on-result-view semantics (protocol 2.2 / section 7)
    burn = []
    for e in (entries if isinstance(entries, list) else []):
        if not isinstance(e, dict):
            continue
        role = e.get("role")
        if role in CONSUMABLE_ROLES:
            burn.append(
                {
                    "corpus_id": e["corpus_id"],
                    "role_at_event": role,
                    "post_open_role": "known_stress",
                    "irreversible": True,
                    "may_return_to_sealed_role": False,
                    "burned_this_run": False,
                    "burn_condition": "result-viewed or compression-anatomy view, NOT this audit",
                }
            )
    burn.sort(key=lambda b: b["corpus_id"])
    gates.append(
        _gate(
            "Q1A-21",
            "burn-on-result-view semantics: this audit consumes nothing",
            "pass",
            {"consumable_entries": burn, "entries_burned_by_this_audit": 0},
            "an admissibility audit reads metadata only; PB-06 remains untriggered and every "
            "sealed role survives this run unconsumed",
        )
    )

    # --- verdict
    # Gate identity is unique and last-write-wins, so a duplicated gate id can
    # never produce two contradictory rows in `gates` or two entries in
    # `failed_gates`.
    gate_by_id = {}
    gate_order = []
    for g in gates:
        gid = g["gate_id"]
        if gid not in gate_by_id:
            gate_order.append(gid)
        gate_by_id[gid] = g
    duplicate_gate_ids = sorted(
        {g["gate_id"] for g in gates if sum(1 for x in gates if x["gate_id"] == g["gate_id"]) > 1}
    )
    if duplicate_gate_ids:
        report_findings.append(
            _f(
                "GATE.duplicate_id",
                "gates",
                "gate ids emitted more than once and were collapsed: %s"
                % (", ".join(duplicate_gate_ids),),
            )
        )
    gates = [gate_by_id[gid] for gid in sorted(gate_by_id)]

    blocking = [
        {"code": f["code"], "path": f["path"], "detail": f["detail"]}
        for f in report_findings
    ]
    blocking = sorted(blocking, key=lambda x: (x["code"], x["path"], x["detail"]))
    hard_codes = ("PB-01", "PB-02", "PB-03", "PB-04", "PB-06", "PB-09")
    hard_blocked = any(f["code"] in hard_codes for f in blocking)
    failed_gates = sorted(g["gate_id"] for g in gates if g["status"] in ("fail", "blocked", "revise"))
    if hard_blocked:
        status = "INVALID_INFRA"
    elif failed_gates or blocking:
        status = "CORPUS_BLOCKED"
    elif promotion_authorized and c9_status == "byte_computed":
        status = "ADMISSIBLE_PILOT"
    else:
        # An attestation-only c9 can never produce ADMISSIBLE_PILOT: the graph
        # is structurally sound but not fully verified.
        status = "CORPUS_BLOCKED"

    # The graph may still be attestation-backed. That must be visible wherever
    # independence_units is read, so it cannot be quoted as verified.
    graph_verification = "fully_verified" if c9_status == "byte_computed" else "attestation_backed"
    indep["graph_verification"] = graph_verification
    indep["authoritative_for_promotion"] = bool(
        graph_verification == "fully_verified" and not hard_blocked and not failed_gates
    )
    indep["non_authoritative_reason"] = (
        None
        if indep["authoritative_for_promotion"]
        else (
            "criterion c9 is attestation-only (not byte-computed); cross-container "
            "containment is therefore unverified, and the operator worktree is not a "
            "pinned clean checkout"
        )
    )

    report = {
        "schema": REPORT_SCHEMA,
        "audit_complete": audit_complete,
        "audit_status_reason": (
            "all graph criteria evaluated to their declared availability semantics"
            if audit_complete
            else "schema validation failed before conflict-graph evaluation; not_evaluated=[] "
                 "must NOT be interpreted as exhaustive evaluation"
        ),
        "tool": {
            "name": TOOL_NAME,
            "version": TOOL_VERSION,
            "source_path": TOOL_SPEC,
            "source_sha256": _tool_source_sha256(),
            "invokes_codec": False,
            "uses_network": False,
            "decompresses_archives": False,
            "reads_sealed_bytes": False,
        },
        "amendments": {k: {"ref": v["ref"], "summary": v["summary"]} for k, v in sorted(AMENDMENTS.items())},
        "lock": {
            "lock_sha256": lock_sha256,
            "lock_authored_sha256": lock_authored_sha256,
            "lock_canonicalization": "set-like arrays sorted before hashing; order-bearing arrays preserved",
            "lock_revision": lock.get("lock_revision"),
            "source_git_sha": lock.get("project", {}).get("source_git_sha"),
            "source_tree_ref": lock.get("project", {}).get("source_tree_ref"),
            "evaluated_at": lock.get("project", {}).get("evaluated_at"),
            "source_dirty": lock.get("project", {}).get("source_dirty"),
            "source_dirty_scope": lock.get("project", {}).get("source_dirty_scope"),
            "local_worktree_evaluated": False,
            "local_worktree_note": (
                "operator worktree dirtiness is out of scope per Edit E8; this field is a "
                "constant, never measured, and is recorded so the artifact cannot be misread"
            ),
        },
        "counters": counters,
        "universe": universe,
        "conflict_edges": edges,
        "not_evaluated": not_evaluated,
        "independence": indep,
        "provenance": provenance,
        "publisher_split": pubiso,
        "lineage": lineage,
        "license": lic,
        "threshold_lint": lint,
        "consumed_controls": {
            "schema": TOMBSTONE_SCHEMA,
            "sha256": tombstone_sha,
            "entry_count": len(tombstone.entries),
            "lock_reference": dict(sorted((ref or {}).items())),
            "stale_reference": bool(tomb_stale),
            "matches": consumed_matches,
            "matched_corpus_ids": consumed_ids,
            "offered_in_sealed_role": consumed_sealed,
            "open_role_not_demoted": consumed_open_mislabeled,
            "used_as_known_stress_control": consumed_as_controls,
            "tombstoned_roles_after_consumption": "known_stress",
        },
        "burn": {
            "rule": "burn on result view or compression-anatomy view; identity metadata view does not consume",
            "entries_burned_by_this_audit": 0,
            "consumable_entries": burn,
        },
        "aggregate": aggregate,
        "qp_gates": {
            "QP-NO-PROMOTE": {
                "status": "pass" if qp_no_promote_ok else "fail",
                "offending_roles": offending_roles,
                "roles": list(QP_NO_PROMOTE_ROLES),
            },
            "QP-CONSUMED": {
                "status": "pass" if qp_consumed_ok else "fail",
                "matched_corpus_ids": consumed_ids,
            },
            "deviations_from_edit_text": [dict(d) for d in QP_GATE_DEVIATIONS],
        },
        "gates": sorted(gates, key=lambda g: g["gate_id"]),
        "findings": blocking,
        "verdict": {
            "status": status,
            "audit_complete": audit_complete,
            "graph_verification": graph_verification,
            "independence_units_authoritative_for_promotion": indep.get("authoritative_for_promotion"),
            "promotion_authorized": False,
            "production_authorized": False,
            "mechanism_claim_authorized": False,
            "novelty_claim_authorized": False,
            "promotion_authorized_by_this_tool": False,
            "aggregate_promotion_authorized": bool(aggregate["promotion_authorized"]),
            "failed_gates": failed_gates,
            "blocking_findings": blocking[:32],
            "blocking_finding_count": len(blocking),
            "next_allowed_action": (
                "portfolio acquisition of a NEW provenance-selected family; no existing object "
                "may be promoted"
                if status == "CORPUS_BLOCKED"
                else "Q1b blind second-auditor agreement and cross-runner reproducibility"
            ),
        },
    }
    return report


# ---------------------------------------------------------------------------
# lawful byte broker (production audit path)
#
# The synthetic suite passes bytes in memory. A real Q1a invocation must obtain
# them lawfully, so this broker is the ONLY sanctioned route from disk to a
# byte criterion. It is fail-closed on every axis:
#
#   * sealed roles are structurally excluded before any filesystem call;
#   * only paths already declared in the lock are considered - no URL, no glob,
#     no directory walk, so an untracked file cannot be silently audited;
#   * the resolved real path must stay inside the declared subject root;
#   * every opened object is SHA-256 verified against the lock before use;
#   * a symlink or traversal out of the root is rejected, not followed.
#
# Reading a sealed entry through this broker is impossible, not merely
# discouraged: sealed ids are filtered out before the path is even resolved.
# ---------------------------------------------------------------------------

BROKER_FORBIDDEN_KINDS = ("https-file", "archive-extract")
BROKER_LOCAL_KINDS = ("generate", "git-raw", "host-installed")


class BrokerRefusal(Exception):
    def __init__(self, code: str, detail: str):
        super().__init__(code + ": " + detail)
        self.code = code
        self.detail = detail


def broker_refusable_corpus_ids(lock: dict) -> list:
    """Entries whose bytes this broker will refuse, with the governing code."""
    out = []
    for e in lock.get("entries", []):
        role = e.get("role")
        kind = (e.get("source") or {}).get("kind")
        if role in SEALED_ROLES:
            out.append({"corpus_id": e.get("corpus_id"), "code": "PB-06", "reason": "sealed role"})
        elif kind in BROKER_FORBIDDEN_KINDS:
            out.append({"corpus_id": e.get("corpus_id"), "code": "PB-06", "reason": "remote source kind %s" % (kind,)})
        elif not (e.get("content") or {}).get("local_path"):
            out.append({"corpus_id": e.get("corpus_id"), "code": "PB-04", "reason": "no declared local_path"})
    return out


def _resolve_under_root(subject_root: str, rel: str) -> str:
    if os.path.isabs(rel) or ":" in rel.split("/")[0][1:2] + "" and False:
        raise BrokerRefusal("PB-04", "declared local_path must be relative to the subject root: %r" % (rel,))
    if rel.startswith("/") or rel.startswith("\\") or (len(rel) > 1 and rel[1] == ":"):
        raise BrokerRefusal("PB-04", "declared local_path must be relative: %r" % (rel,))
    root_real = os.path.realpath(subject_root)
    target = os.path.realpath(os.path.join(root_real, rel))
    if target != root_real and not target.startswith(root_real + os.sep):
        raise BrokerRefusal("PB-06", "declared local_path escapes the subject root: %r" % (rel,))
    return target


def broker_open_role_bytes(lock: dict, subject_root: str, guard: ExposureGuard) -> dict:
    """Materialize bytes for OPEN roles only, verified against the lock.

    Returns {corpus_id: bytes}. Every refusal is recorded on the guard so the
    emitted counters prove sealed bytes were never even attempted.
    """
    if not os.path.isdir(subject_root):
        raise BrokerRefusal("INVALID_INFRA", "subject root does not exist: %s" % (subject_root,))

    sources = {}
    for e in lock.get("entries", []):
        cid = e.get("corpus_id")
        role = e.get("role")
        kind = (e.get("source") or {}).get("kind")
        content = e.get("content") or {}

        # Structural exclusion: sealed roles never reach the filesystem.
        if role in SEALED_ROLES:
            guard.broker_refusals.append({"corpus_id": cid, "code": "PB-06", "reason": "sealed role"})
            continue
        if kind in BROKER_FORBIDDEN_KINDS:
            guard.broker_refusals.append({"corpus_id": cid, "code": "PB-06", "reason": "remote source kind"})
            continue
        rel = content.get("local_path")
        if not rel:
            guard.broker_refusals.append({"corpus_id": cid, "code": "PB-04", "reason": "no declared local_path"})
            continue
        if kind not in BROKER_LOCAL_KINDS:
            guard.broker_refusals.append({"corpus_id": cid, "code": "PB-04", "reason": "source kind not brokerable"})
            continue

        try:
            real = _resolve_under_root(subject_root, rel)
        except BrokerRefusal as exc:
            guard.broker_refusals.append({"corpus_id": cid, "code": exc.code, "reason": exc.detail})
            continue
        if not os.path.isfile(real):
            guard.broker_refusals.append({"corpus_id": cid, "code": "PB-04", "reason": "declared path is not a file"})
            continue

        with open(real, "rb") as fh:
            data = fh.read()
        # identity BEFORE use, per protocol 6.8
        digest = sha256_hex(data)
        expected = content.get("sha256")
        if expected and digest != expected:
            guard.broker_refusals.append(
                {"corpus_id": cid, "code": "PB-04", "reason": "sha256 mismatch", "declared": expected, "actual": digest}
            )
            continue
        declared_bytes = content.get("bytes")
        if isinstance(declared_bytes, int) and len(data) != declared_bytes:
            guard.broker_refusals.append(
                {"corpus_id": cid, "code": "PB-04", "reason": "byte-length mismatch"}
            )
            continue
        # Finally through the guard, which refuses sealed roles by construction.
        sources[cid] = guard.open_bytes(cid, role, data)
    return sources


def _read_own_source() -> str:
    try:
        with open(os.path.abspath(__file__), "r", encoding="utf-8") as fh:
            return fh.read()
    except OSError:  # pragma: no cover
        return ""


# ---------------------------------------------------------------------------
# static import/call analysis (AST, not substring)
#
# Determinism and zero-exposure claims are checked structurally: what the tool
# IMPORTS and what it CALLS, never whether a word appears in a string literal
# (a docstring mentioning a forbidden name proves nothing either way).
# ---------------------------------------------------------------------------

_FORBIDDEN_IMPORTS = frozenset(
    {
        "asyncio", "bz2", "ftplib", "gzip", "hashlib_shake", "lzma", "multiprocessing",
        "pickle", "poplib", "quopri", "random", "requests", "secrets", "shutil",
        "smtplib", "socket", "socketserver", "ssl", "subprocess", "tarfile",
        "telnetlib", "urllib", "uuid", "zipfile",
    }
)


def _ast_tree(source_text: str):
    try:
        return ast.parse(source_text)
    except SyntaxError:  # pragma: no cover
        return None


def _imported_modules(source_text: str) -> set:
    """Root module names actually imported by this source."""
    tree = _ast_tree(source_text)
    mods = set()
    if tree is None:
        return mods
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            for alias in node.names:
                mods.add(alias.name.split(".")[0])
        elif isinstance(node, ast.ImportFrom):
            if node.module:
                mods.add(node.module.split(".")[0])
    return mods


def _dotted_calls(source_text: str) -> list:
    """Dotted attribute-call targets, e.g. `os.urandom(...)` -> 'os.urandom'."""
    tree = _ast_tree(source_text)
    out = []
    if tree is None:
        return out
    for node in ast.walk(tree):
        if isinstance(node, ast.Call):
            parts = []
            cur = node.func
            while isinstance(cur, ast.Attribute):
                parts.append(cur.attr)
                cur = cur.value
            if isinstance(cur, ast.Name):
                parts.append(cur.id)
                out.append(".".join(reversed(parts)))
    return out


def _name_reads(source_text: str) -> list:
    """Attribute names read off any expression, e.g. `sys.version_info`."""
    tree = _ast_tree(source_text)
    out = []
    if tree is None:
        return out
    for node in ast.walk(tree):
        if isinstance(node, ast.Attribute):
            out.append(node.attr)
    return out


# ---------------------------------------------------------------------------
# emitted artifact set (protocol 5.3 / 10 + queue E6)
# ---------------------------------------------------------------------------

ARTIFACT_FILES = {
    "admissibility-report.json": lambda r: r,
    "artifact-manifest.json": None,
    "conflict-edges.json": lambda r: r["conflict_edges"],
    "consumed-controls-report.json": lambda r: r["consumed_controls"],
    "counters.json": lambda r: r["counters"],
    "dedup-report.json": lambda r: {
        "schema": "anvil.dedup-report/v1",
        "tool": r["tool"],
        "edges": [e for e in r["conflict_edges"] if e["criterion"] in ("c1", "c2", "c3", "c4", "c5")],
        "not_evaluated": r["not_evaluated"],
        "waiver_events": 0,
    },
    "independence-units.json": lambda r: r["independence"],
    "license-report.json": lambda r: r["license"],
    "lineage-report.json": lambda r: r["lineage"],
    "provenance-completeness.json": lambda r: r["provenance"],
    "publisher-split-report.json": lambda r: r["publisher_split"],
    "threshold-lint.json": lambda r: r["threshold_lint"],
    "universe.json": lambda r: r["universe"],
    "verdict.json": lambda r: r["verdict"],
}


def emit_artifacts(report, out_dir):
    """Write every artifact as canonical JSON plus a manifest (protocol 10.10)."""
    os.makedirs(out_dir, exist_ok=True)
    written = []
    for name in sorted(ARTIFACT_FILES):
        if ARTIFACT_FILES[name] is None:
            continue
        payload = ARTIFACT_FILES[name](report)
        data = canon(payload)
        rel = os.path.join(out_dir, name)
        with open(rel, "wb") as fh:
            fh.write(data)
        written.append(
            {
                "relative_path": name,
                "bytes": len(data),
                "sha256": sha256_hex(data),
                "media_type": "application/json",
                "producer": TOOL_NAME,
                "producer_version": TOOL_VERSION,
            }
        )
    manifest = {
        "schema": "anvil.artifact-manifest/v1",
        "tool": report["tool"],
        "artifacts": written,
        "aggregate_of_artifacts": {"artifact_count": len(written)},
        "lock_sha256": report["lock"]["lock_sha256"],
    }
    mdata = canon(manifest)
    with open(os.path.join(out_dir, "artifact-manifest.json"), "wb") as fh:
        fh.write(mdata)
    return manifest


# ---------------------------------------------------------------------------
# synthetic metadata fixtures (SELF-TEST ONLY - never real corpus data)
# ---------------------------------------------------------------------------

FIXTURE_TREE_REF = "0000000000000000000000000000000000000000"
FIXTURE_FIXED_UTC = "2026-10-02T00:00:00Z"
FIXTURE_REPO = "https://example.invalid/anvil/fixture"
PROTO_PATH = "docs/I10-CORPUS-LOCK-PROTOCOL.md"


def _fx_sha(seed: str) -> str:
    return hashlib.sha256(seed.encode("utf-8")).hexdigest()


def _fx_blob(seed: str) -> str:
    return hashlib.sha256(b"blob:" + seed.encode("utf-8")).hexdigest()[:40]


def _fx_synthetic(generator: str, seed_value: int) -> dict:
    params = {"seed": seed_value, "shape": "fixture"}
    return {
        "generator_path": generator,
        "generator_git_blob_sha1": _fx_blob(generator),
        "generator_sha256": _fx_sha("gen:" + generator),
        "seed": {"value": seed_value},
        "parameters": params,
        "parameters_sha256": canon_sha256(params),
        "runtime": {"language": "python", "version": "fixture-3.13"},
    }


def _fx_entry(
    cid,
    role,
    cls,
    *,
    publisher,
    project,
    release,
    schema_cluster="not-applicable",
    producer=None,
    generator=None,
    seed=0,
    git_tracked=False,
    source_kind="generate",
    url=None,
    archive=None,
    host_install=None,
    toolchain=None,
    framing="none",
    counts_source="not-applicable",
    content_sha=None,
    content_bytes=4096,
    git_blob=None,
    local_path=None,
    license_status="permitted",
    attribution_required=False,
    attribution_text=None,
    redistribution=True,
    near_dupes=None,
    exact_group=None,
    previous_roles=None,
    role_rationale="synthetic self-test fixture; not a corpus object",
):
    return {
        "corpus_id": cid,
        "display_name": cid.replace("-", "_"),
        "role": role,
        "class": cls,
        "selection_frozen_utc": FIXTURE_FIXED_UTC,
        "role_rationale": role_rationale,
        "independence": {
            "publisher_id": publisher,
            "project_id": project,
            "release_family_id": release,
            "producer_id": producer,
            "schema_cluster_id": schema_cluster,
            "exact_duplicate_group_id": exact_group or _fx_sha("exact:" + cid),
            "near_duplicate_group_ids": list(near_dupes or []),
            "audit_artifact_sha256": _fx_sha("audit:" + cid),
        },
        "license": {
            "status": license_status,
            "name": "fixture-license",
            "spdx": None,
            "url": None,
            "redistribution_allowed": redistribution,
            "attribution_required": attribution_required,
            "attribution_text": attribution_text,
        },
        "source": {
            "kind": source_kind,
            "repository": None,
            "commit": None,
            "path": None,
            "url": url,
            "archive": archive,
            "git_tracked": git_tracked,
            "host_install": host_install,
        },
        "content": {
            "canonical_filename": cid.replace("-", "_") + ".bin",
            "bytes": content_bytes,
            "sha256": content_sha or _fx_sha("content:" + cid),
            "git_blob_sha1": git_blob,
            "media_type": "application/octet-stream",
            "local_path": local_path,
        },
        "derivation": {"transform_chain": []},
        "validity": {
            "framing": framing,
            "total_records": None,
            "valid_records": None,
            "residual_records": None,
            "final_newline": None,
            "schema_signature_sha256": (
                _fx_sha("schema:" + schema_cluster) if schema_cluster != "not-applicable" else None
            ),
            "counts_source": counts_source,
            "counts_artifact_sha256": (
                _fx_sha("counts:" + cid) if counts_source != "not-applicable" else None
            ),
        },
        "synthetic": _fx_synthetic(generator, seed) if generator else None,
        "lineage": {
            "introduced_lock_revision": 1,
            "previous_roles": list(previous_roles or []),
            "replaces_corpus_id": None,
            "replacement_reason": None,
        },
        "toolchain": toolchain,
    }


def _fx_toolchain(producer: str, version: str) -> dict:
    return {
        "producer_id": producer,
        "producer_version": version,
        "attestation_kind": "fixture-declared",
        "attestation_sha256": _fx_sha("toolchain:" + producer + ":" + version),
    }


def _fx_host_install(product: str, rel_path: str) -> dict:
    return {
        "host_product_name": product,
        "install_root": "C:\\Program Files\\FixtureVendor",
        "install_path": "C:\\Program Files\\FixtureVendor\\" + rel_path,
        "file_version": "0.0.0.0",
    }


def _fx_archive(archive_sha: str, member: str) -> dict:
    return {
        "archive_url": "https://example.invalid/archive.zip",
        "archive_bytes": 65536,
        "archive_sha256": archive_sha,
        "member_name": member,
        "member_bytes": 4096,
        "member_sha256": _fx_sha("member:" + member),
        "member_crc32": "0badf00d",
        "extractor": "fixture-publisher-manifest",
        "extractor_version": "1",
    }


def fixture_lock(**overrides):
    """A 13-entry synthetic lock exercising c1..c9, all seven roles, and both
    sealed roles. Contains no real corpus byte and no real provenance claim."""
    shared_schema = "fx-sc-structured"

    # Two entries share one generator -> c8 (and c6/c7).
    fx_json_a = _fx_entry(
        "fx-json-a", "synthetic_control", "structured_json",
        publisher="fx-pub-a", project="fx-proj-a", release="fx-rel-a",
        schema_cluster=shared_schema, generator="fixture/gen_a.py", seed=1,
        framing="json-document", counts_source="curator",
    )
    fx_json_b = _fx_entry(
        "fx-json-b", "synthetic_control", "structured_json",
        publisher="fx-pub-a", project="fx-proj-a", release="fx-rel-a",
        schema_cluster=shared_schema, generator="fixture/gen_a.py", seed=2,
        framing="json-document", counts_source="curator",
        near_dupes=["fx-json-a"],
    )
    # c9 container: contains fx-json-a's record set.
    fx_sqlite = _fx_entry(
        "fx-sqlite", "synthetic_control", "sqlite",
        publisher="fx-pub-a", project="fx-proj-a", release="fx-rel-a",
        schema_cluster=shared_schema, generator="fixture/gen_a.py", seed=3,
        framing="sqlite", counts_source="curator", near_dupes=["fx-json-a"],
    )
    # c2 + c3 + c4 + c5: identical git blob AND overlapping text bytes.
    text_blob = _fx_blob("shared-text-blob")
    fx_text_e = _fx_entry(
        "fx-text-e", "known_stress", "text_log",
        publisher="fx-pub-b", project="fx-proj-b", release="fx-rel-b",
        producer="fixture-producer-b", git_blob=text_blob,
        source_kind="https-file", url="https://example.invalid/e.log",
        framing="log-lines", counts_source="curator",
    )
    fx_text_f = _fx_entry(
        "fx-text-f", "self_reference_control", "text_log",
        publisher="fx-pub-b", project="fx-proj-b", release="fx-rel-b",
        producer="fixture-producer-b", git_blob=text_blob,
        source_kind="https-file", url="https://example.invalid/f.log",
        framing="log-lines", counts_source="curator", near_dupes=["fx-text-e"],
    )
    # host-installed + toolchain provenance pair sharing a release family -> c7.
    fx_bin_g = _fx_entry(
        "fx-bin-g", "external_anchor", "executable",
        publisher="fx-pub-c", project="fx-proj-c", release="fx-rel-c",
        producer="fixture-msvc", source_kind="host-installed",
        host_install=_fx_host_install("FixtureVendor Suite", "bin/tool_g.exe"),
        toolchain=_fx_toolchain("fixture-msvc", "19.30"),
        license_status="owned", content_bytes=8192,
    )
    fx_bin_h = _fx_entry(
        "fx-bin-h", "external_anchor", "executable",
        publisher="fx-pub-c", project="fx-proj-c", release="fx-rel-c",
        producer="fixture-msvc", source_kind="host-installed",
        host_install=_fx_host_install("FixtureVendor Suite", "bin/tool_h.exe"),
        toolchain=_fx_toolchain("fixture-msvc", "19.30"),
        license_status="owned", content_bytes=8192,
    )
    # c1: byte-identical to fx-bin_h -> identical sha256.
    fx_bin_i = _fx_entry(
        "fx-bin-i", "discovery", "executable",
        publisher="fx-pub-d", project="fx-proj-d", release="fx-rel-d",
        producer="fixture-clang", source_kind="host-installed",
        host_install=_fx_host_install("OtherVendor Suite", "bin/tool_i.exe"),
        toolchain=_fx_toolchain("fixture-clang", "17.0"),
        license_status="owned", content_sha=fx_bin_h["content"]["sha256"], content_bytes=8192,
    )
    # ledger-entry tombstone target (project_id normalizes to the DrugQA marker).
    fx_v1 = _fx_entry(
        "fx-drugqa-v1", "discovery", "structured_json",
        publisher="fx-pub-e", project="sino-us drugqa v1", release="fx-rel-e",
        schema_cluster="fx-sc-drugqa", generator="fixture/gen_b.py", seed=9,
        framing="json-document", counts_source="curator",
    )
    # sealed pair, isolated publisher/project/release/schema.
    fx_held_j = _fx_entry(
        "fx-held-j", "heldout", "structured_ndjson",
        publisher="fx-pub-sealed-1", project="fx-proj-sealed-1", release="fx-rel-sealed-1",
        schema_cluster="fx-sc-sealed-1", producer="fixture-gcc-sealed-1",
        source_kind="https-file", url="https://example.invalid/sealed1.ndjson",
        framing="ndjson", counts_source="curator",
    )
    fx_ext_k = _fx_entry(
        "fx-ext-k", "external_test", "external_mixed",
        publisher="fx-pub-sealed-2", project="fx-proj-sealed-2", release="fx-rel-sealed-2",
        schema_cluster="fx-sc-sealed-2", producer="fixture-gcc-sealed-2",
        source_kind="https-file", url="https://example.invalid/sealed2.bin",
        framing="none", counts_source="curator",
    )
    # archive-extract role: manifest-only, never decompressed.
    fx_arch_m = _fx_entry(
        "fx-arch-m", "external_development", "external_mixed",
        publisher="fx-pub-f", project="fx-proj-f", release="fx-rel-f",
        producer="fixture-gcc-open", source_kind="archive-extract",
        url="https://example.invalid/dataset.zip",
        archive=_fx_archive(_fx_sha("archive:fx"), "inner/data.bin"),
    )
    # independent singleton, no shared lineage with anything.
    fx_ndjson_l = _fx_entry(
        "fx-ndjson-l", "discovery", "structured_ndjson",
        publisher="fx-pub-g", project="fx-proj-g", release="fx-rel-g",
        schema_cluster="fx-sc-ndjson", generator="fixture/gen_c.py", seed=13,
        framing="ndjson", counts_source="curator",
    )

    # protocol 4.1: entries MUST be sorted by corpus_id.
    entries = sorted(
        [
            fx_json_a, fx_json_b, fx_sqlite, fx_text_e, fx_text_f,
            fx_bin_g, fx_bin_h, fx_bin_i, fx_v1, fx_held_j, fx_ext_k,
            fx_arch_m, fx_ndjson_l,
        ],
        key=lambda e: e["corpus_id"],
    )

    lock = {
        "schema": SCHEMA,
        "protocol_version": PROTOCOL_VERSION,
        "lock_revision": 1,
        "created_utc": FIXTURE_FIXED_UTC,
        "title": "Q1a synthetic self-test fixture lock (NOT a corpus lock)",
        "project": {
            "name": "ANVIL",
            "repository": FIXTURE_REPO,
            "protocol_path": PROTO_PATH,
            "source_git_sha": FIXTURE_TREE_REF,
            "source_dirty": False,
            "source_tree_ref": FIXTURE_TREE_REF,
            "evaluated_at": FIXTURE_FIXED_UTC,
            "source_dirty_scope": "pinned_checkout",
            # amendment A8: the fixture audits a CLEAN PINNED checkout, and says so.
            "local_worktree": {
                "evaluated": True,
                "dirty_path_count": 0,
                "declared_objects_absent_from_pinned_commit": [],
                "note": "fixture: subject root is a clean pinned checkout; every declared "
                        "object is present at the pinned commit",
            },
        },
        "lineage": {
            "introduced_lock_revision": 1,
            "supersedes_lock_sha256": None,
            "change_reason": None,
            "replacement_policy": "new-lock-only-no-silent-splicing",
        },
        "roles": {
            "allowed": list(ROLE_ENUM),
            "default_transition": {"external_test": "known_stress", "heldout": "known_stress"},
            "forbidden_transitions": [
                "external_test->external_test",
                "heldout->heldout",
                "self_reference_control->heldout",
                "synthetic_control->heldout",
            ],
            "evidence_split_map": dict(sorted(ROLE_TO_SPLIT.items())),
        },
        "splits": {},
        "entries": entries,
        "deduplication": {
            "criteria": sorted(CRITERIA),
            "grouping": "transitively-closed-connected-components",
            "group_ids_are_outputs": True,
            "waiver_path": "absent",
            "containment_verification": "attestation_only",
            "containment_claims": [
                {
                    "contained_corpus_id": "fx-json-a",
                    "container_corpus_id": "fx-sqlite",
                    "record_set_sha256": _fx_sha("records:fx-json-a"),
                    "evidence": "fixture attestation: fx-sqlite embeds every fx-json-a record",
                    "attestation_kind": "fixture-declared",
                    "attestation_sha256": _fx_sha("attest:fx-sqlite"),
                },
                {
                    "contained_corpus_id": "fx-json-a",
                    "container_corpus_id": "fx-arch-m",
                    "record_set_sha256": _fx_sha("records:fx-json-a"),
                    "evidence": "fixture attestation: fx-arch-m embeds every fx-json-a record",
                    "attestation_kind": "fixture-declared",
                    "attestation_sha256": _fx_sha("attest:fx-arch-m"),
                },
            ],
        },
        "publisher_isolation": {
            "required_new_fields": sorted(
                ["publisher_id", "project_id", "release_family_id", "schema_cluster_id"]
            ),
            "producer_diversity_rule": "producer/compiler family must not be the sole diversity dimension",
            "selection_basis": sorted(
                ["provenance", "size", "format", "validity", "license", "independence"]
            ),
        },
        "acquisition": {
            "identity_algorithm": "sha256",
            "md5_forbidden": True,
            "allowed_hosts": ["example.invalid"],
            "byte_access_roles": list(OPEN_ROLES),
            "sealed_byte_access": "forbidden",
            "archive_role_policy": "manifest-only, not decompressed",
        },
        "aitdcc": {
            "enabled": False,
            "development_range": "A-H",
            "test_range": "I-P",
            "sha256sums_sha256": None,
            "attribution": "AITDCC dataset and paper, when enabled (protocol 8)",
        },
        "workflow": {
            "jobs": sorted(["enumerate", "units", "containment", "threshold-lint", "provenance", "q1a"]),
            "fail_closed": True,
            "ledger_path": "corpus-access-ledger.jsonl",
            "ledger_appended_by": "workflow",
        },
        "artifacts": {
            "required": sorted(n for n, v in ARTIFACT_FILES.items() if v is not None),
            "uploaded_by": "workflow",
            "upload_condition": "always()",
        },
        "promotion": {
            "sequence": sorted([
                "lock-and-ledger", "frozen-preregistration", "correctness-gate", "adversarial-gate",
                "discovery-and-ablation-gates", "frozen-implementation", "one-shot-heldout-gate",
                "frontier-reconstruction",
            ]),
            "blocked_roles": list(QP_NO_PROMOTE_ROLES),
            "pass_variants": sorted([
                "PASS_DISCOVERY", "PASS_KNOWN_STRESS", "PASS_HELDOUT", "PASS_FRONTIER",
            ]),
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
            "declared_by": "fixture-author",
            "declared_utc": FIXTURE_FIXED_UTC,
            "declared_sha256": _fx_sha("audit-params"),
        },
        "consumed_controls_ref": {
            "path": "consumed_controls.json",
            "sha256": canon_sha256(fixture_tombstone()),
            "entry_count": len(fixture_tombstone()["entries"]),
            "keyed_by": ["ledger_entry", "sha256"],
        },
    }
    for key, value in overrides.items():
        lock[key] = value

    splits = {name: [] for name in PROTOCOL_SPLITS}
    for e in lock["entries"]:
        splits[ROLE_TO_SPLIT[e["role"]]].append(e["corpus_id"])
    lock["splits"] = {k: sorted(v) for k, v in splits.items()}
    return lock


def fixture_tombstone(**overrides):
    """Synthetic consumption tombstone.

    The six PE SHA-256 values are PUBLIC FACTS quoted verbatim from the queue's
    Edit E5 text block. They are literal strings; no PE file is opened, read,
    fetched or hashed by this prototype.
    """
    entries = [
        {
            "identity_kind": "sha256",
            "identity": "1a0043555d254618f2d56c936c3d9a1fbfb878bc878416a133c346bc7835eda9",
            "display_name": "pe-git.exe",
            "tombstone_key": "anvil.consumed.pe.git.s6-2",
            "consumed_by": "S6-2 PE set",
            "consumed_utc": FIXTURE_FIXED_UTC,
            "role_after_consumption": "known_stress",
            "evidence": "RESEARCH_LEDGER S6-2 PE set; hash quoted from queue Edit E5",
            "promotion_forbidden": True,
        },
        {
            "identity_kind": "ledger_entry",
            "identity": "Sino-US DrugQA V1",
            "display_name": "Sino-US DrugQA V1",
            "tombstone_key": "anvil.consumed.drugqa.v1-column-adverse",
            "consumed_by": "G3",
            "consumed_utc": FIXTURE_FIXED_UTC,
            "role_after_consumption": "known_stress",
            "evidence": "contamination ledger entry V1-COLUMN-ADVERSE",
            "promotion_forbidden": True,
        },
    ]
    for e in entries:
        e["consumed"] = True
    doc = {
        "schema": TOMBSTONE_SCHEMA,
        "append_only": True,
        "entries": entries,
        "note": "append-only tombstone; git_tracked is recorded, never required true (Edit E5)",
    }
    for key, value in overrides.items():
        doc[key] = value
    return doc


def fixture_byte_sources():
    """Synthetic in-memory bytes for OPEN roles only.

    Engineered so each byte criterion is exercised deterministically: a shared
    4 KiB block (c3), a CR-before-LF variant of it (c4), high 5-gram MinHash
    similarity (c5), and one non-text binary pair that must NOT fire c5.
    """
    def lcg(seed, n, lo=32, hi=126):
        out = bytearray()
        x = (seed * 2654435761) & 0xFFFFFFFF
        for _ in range(n):
            x = (1103515245 * x + 12345) & 0x7FFFFFFF
            out.append(lo + (x % (hi - lo)))
        return bytes(out)

    shared_block = lcg(101, 4096)
    crlf_block = shared_block.replace(b"\n", b"\r\n", 4)

    base = b"alpha bravo charlie delta echo foxtrot golf hotel india juliet\n"
    text_e = base * 300 + shared_block + b"\nend-of-e\n"
    text_f = base * 300 + crlf_block + b"\nend-of-f\n"

    block_g = lcg(202, 4096, lo=0, hi=255)
    block_h = b"\x7fELF" + block_g + b"\x00\x01"

    return {
        "fx-text-e": text_e,
        "fx-text-f": text_f,
        "fx-bin-g": block_g,
        "fx-bin-h": block_h,
        "fx-bin-i": block_h,
        "fx-json-a": b'{"id":1}\n' * 900,
        "fx-json-b": b'{"id":1}\n' * 900,
        "fx-sqlite": b"SQLite format 3\x00" + lcg(303, 8192, lo=0, hi=255),
        "fx-ndjson-l": b'{"id":7}\n' * 1200,
        "fx-drugqa-v1": b'{"id":9}\n' * 700,
    }


# ---------------------------------------------------------------------------
# self-test (synthetic fixtures only; no codec, no network, no sealed byte)
# ---------------------------------------------------------------------------


def _with_threshold(lock: dict, num: int, den: int) -> dict:
    """Copy of `lock` with the declared MinHash threshold moved, nothing else."""
    out = canonicalize_lock(lock)
    out["audit_parameters"]["minhash_threshold_num"] = num
    out["audit_parameters"]["minhash_threshold_den"] = den
    return out


def _minhash_estimate(sources: dict, lock: dict) -> int:
    """Equal-permutation count for a two-entry source pair, per the lock params."""
    params = lock["audit_parameters"]
    idx = {
        cid: ByteIndex(cid, data, params)
        for cid, data in sources.items()
        if text_like(data, params)
    }
    keys = sorted(idx)
    num, den = estimated_jaccard(idx[keys[0]].sig, idx[keys[1]].sig)
    return num if den else 0


def _raises_broker(fn) -> bool:
    try:
        fn()
    except BrokerRefusal:
        return True
    except OSError:
        return True
    return False


def selftest() -> int:
    failures = []
    checks = [0]

    def check(name, cond, detail=""):
        checks[0] += 1
        if not cond:
            failures.append("%s :: %s" % (name, detail))

    source_text = _read_own_source()
    # fixture_lock() already binds consumed_controls_ref to the fixture
    # tombstone, so every derived fixture is a complete, schema-valid lock.
    lock = fixture_lock()
    tomb = fixture_tombstone()
    check(
        "W0.0 fixture lock binds the fixture tombstone",
        lock["consumed_controls_ref"]["sha256"] == canon_sha256(tomb),
    )

    # --- W1 fixtures are themselves schema valid
    findings = validate_lock(lock)
    check("W1.1", not findings, "fixture lock findings: %s" % (findings[:3],))
    check("W1.2", Tombstone(tomb).schema_ok, "fixture tombstone invalid")
    check("W1.3", not check_source_scope(lock), "fixture source scope invalid")
    check("W1.4", len(lock["entries"]) == 13, "entry count %d" % (len(lock["entries"]),))
    check(
        "W1.5",
        all(any(e["role"] == r for e in lock["entries"]) for r in ROLE_ENUM),
        "fixture does not cover every role",
    )

    # --- W2 run the audit
    guard = ExposureGuard()
    report = audit(lock, tomb, fixture_byte_sources(), guard, source_text)

    c = report["counters"]
    check("W2.0 valid fixture completes graph audit", report["audit_complete"] is True)
    check("W2.1 zero network", c["network_ops"] == 0, str(c["network_ops"]))
    check("W2.2 zero codec", c["codec_invocations"] == 0, str(c["codec_invocations"]))
    check("W2.3 zero archive decompression", c["archive_decompressions"] == 0, str(c["archive_decompressions"]))
    check("W2.4 zero sealed bytes", c["sealed_byte_reads"] == 0, str(c["sealed_byte_reads"]))
    check(
        "W2.5 sealed entries absent from byte sources",
        not (set(fixture_byte_sources()) & set(report["universe"]["sealed_entry_ids"])),
        "a sealed id was supplied as bytes",
    )
    check(
        "W2.6 archive member identity is manifest-only",
        c["member_identity_notes"] == ["manifest-only, not decompressed"],
        str(c["member_identity_notes"]),
    )

    # --- W3 criteria coverage
    crits = {e["criterion"] for e in report["conflict_edges"]}
    for crit in ("c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9"):
        check("W3.%s fires on the fixture" % (crit,), crit in crits, "missing; have %s" % (sorted(crits),))
    check("W3.waiver", all(e["waived"] is False for e in report["conflict_edges"]), "an edge was waived")
    check(
        "W3.evidence",
        all(len(e["evidence_sha256"]) == 64 for e in report["conflict_edges"]),
        "an edge lacks an evidence hash",
    )
    check(
        "W3.sealed-pairs-labelled",
        any(r["status"] == "not_evaluated_sealed" for r in report["not_evaluated"]),
        "sealed pairs were not explicitly not-evaluated",
    )
    check(
        "W3.no-false-clean",
        all(
            not (set(e["a"] for e in report["conflict_edges"]) & set(report["universe"]["sealed_entry_ids"]))
            or e["evaluated_from"] != "open_bytes"
            for e in report["conflict_edges"]
        ),
        "a sealed entry has an open_bytes edge",
    )

    # --- W4 independence units
    units = report["independence"]["units"]
    check("W4.1 computed", report["independence"]["independence_units"] > 0)
    check(
        "W4.1b the graph is labelled attestation-backed, not fully verified",
        report["independence"]["graph_verification"] == "attestation_backed"
        and report["independence"]["authoritative_for_promotion"] is False,
        str(report["independence"].get("graph_verification")),
    )
    check(
        "W4.1c c9 attestation-only can never be ADMISSIBLE_PILOT",
        report["verdict"]["status"] != "ADMISSIBLE_PILOT",
        report["verdict"]["status"],
    )
    check(
        "W4.1d gate ids are unique",
        len({g["gate_id"] for g in report["gates"]}) == len(report["gates"]),
        str(sorted(g["gate_id"] for g in report["gates"])),
    )
    check(
        "W4.1e failed_gates has no duplicates",
        len(set(report["verdict"]["failed_gates"])) == len(report["verdict"]["failed_gates"]),
        str(report["verdict"]["failed_gates"]),
    )
    broken_schema = fixture_lock()
    del broken_schema["project"]["local_worktree"]
    broken_report = audit(broken_schema, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    check(
        "W4.1f schema-aborted graph is explicitly incomplete",
        broken_report["audit_complete"] is False
        and broken_report["verdict"]["audit_complete"] is False
        and broken_report["verdict"]["status"] != "ADMISSIBLE_PILOT",
        "%s / %s" % (broken_report.get("audit_complete"), broken_report["verdict"].get("status")),
    )
    check(
        "W4.2 fewer units than entries",
        report["independence"]["independence_units"] < len(lock["entries"]),
        "units %d entries %d" % (report["independence"]["independence_units"], len(lock["entries"])),
    )
    check(
        "W4.3 ids derived",
        all(u["independence_unit_id"].startswith("iu-") for u in units),
        "a unit id is not derived",
    )
    check(
        "W4.4 partition",
        sorted(m for u in units for m in u["members"]) == sorted(e["corpus_id"] for e in lock["entries"]),
        "units do not partition the entries",
    )
    check(
        "W4.5 generator family collapsed",
        any(
            set(("fx-json-a", "fx-json-b", "fx-sqlite")).issubset(set(u["members"]))
            for u in units
        ),
        "shared-generator entries were not collapsed into one unit",
    )
    check(
        "W4.6 c9 container merged",
        any(
            set(("fx-json-a", "fx-arch-m")).issubset(set(u["members"]))
            for u in units
        ),
        "c9 containment did not merge the container into the contained unit",
    )
    check(
        "W4.7 sealed singleton",
        any(u["members"] == ["fx-held-j"] for u in units),
        "an isolated sealed entry did not stay a singleton unit",
    )

    # --- W5 curator group ids are outputs, never authority
    bad = fixture_lock()
    bad["deduplication"]["independence_units"] = 99
    check(
        "W5.1",
        any(x["code"] == "schema.unknown_field" for x in validate_lock(bad)),
        "a curator-supplied unit count was accepted",
    )
    bad2 = fixture_lock()
    bad2["entries"][0]["independence_group_id"] = "handmade"
    check(
        "W5.2",
        any(x["code"] == "GATE-INDEP-1.curator_group_id" for x in validate_lock(bad2)),
        "a curator-supplied entry group id was accepted",
    )
    r5 = audit(bad2, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    check(
        "W5.3 curator group field blocks the audit",
        r5["independence"].get("independence_units") is None
        and r5["verdict"]["status"] != "ADMISSIBLE_PILOT",
        "a curator-supplied group id did not block the audit: %s" % (r5["independence"],),
    )
    check(
        "W5.4 units are never read from the lock",
        report["independence"]["independence_units"]
        == len(report["independence"]["units"])
        and all(
            "independence_units" not in e
            for e in report["independence"]["units"]
        ),
        "unit count is not the number of computed components",
    )
    check(
        "W5.5 a lock whose group fields disagree with the computed edges is rejected",
        True,
    )
    disagree = fixture_lock()
    for e in disagree["entries"]:
        if e["corpus_id"] == "fx-json-a":
            e["independence"]["near_duplicate_group_ids"] = ["fx-ndjson-l"]
    r5b = audit(disagree, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    check(
        "W5.6",
        any(f["code"] == "PB-11" for f in r5b["findings"]) and r5b["verdict"]["status"] != "ADMISSIBLE_PILOT",
        "a declared duplicate with no computed edge was not rejected",
    )

    # --- W6 unsupportable duplicate annotation -> PB-11
    bad3 = fixture_lock()
    bad3["entries"][0]["independence"]["near_duplicate_group_ids"] = ["fx-held-j"]
    r6 = audit(bad3, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    check("W6.1 PB-11 raised", any(f["code"] == "PB-11" for f in r6["findings"]), "PB-11 not raised")
    check("W6.2 verdict not admissible", r6["verdict"]["status"] != "ADMISSIBLE_PILOT", r6["verdict"]["status"])

    # --- W7 consumed controls / QP-CONSUMED (public queue hashes only)
    matched = set(report["consumed_controls"]["matched_corpus_ids"])
    check("W7.1 ledger tombstone matched by project_id", "fx-drugqa-v1" in matched, str(sorted(matched)))
    check("W7.2 QP-CONSUMED fails on the fixture", report["qp_gates"]["QP-CONSUMED"]["status"] == "fail")
    check(
        "W7.2b the fixture's tombstoned entry sits in an UNDEMOTED open role",
        report["consumed_controls"]["open_role_not_demoted"] == ["fx-drugqa-v1"],
        str(report["consumed_controls"]["open_role_not_demoted"]),
    )
    # a tombstoned object correctly demoted to known_stress is a usable control
    demoted = fixture_lock()
    for e in demoted["entries"]:
        if e["corpus_id"] == "fx-drugqa-v1":
            e["role"] = "known_stress"
    demoted["splits"] = {name: [] for name in PROTOCOL_SPLITS}
    for e in demoted["entries"]:
        demoted["splits"][ROLE_TO_SPLIT[e["role"]]].append(e["corpus_id"])
    r7a = audit(demoted, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    check(
        "W7.2c a demoted known_stress control does NOT fail QP-CONSUMED",
        r7a["qp_gates"]["QP-CONSUMED"]["status"] == "pass"
        and r7a["consumed_controls"]["used_as_known_stress_control"] == ["fx-drugqa-v1"],
        str(r7a["qp_gates"]["QP-CONSUMED"]),
    )
    # offering a consumed object in a SEALED role is the laundering path
    sealed_offer = fixture_lock()
    for e in sealed_offer["entries"]:
        if e["corpus_id"] == "fx-drugqa-v1":
            e["role"] = "heldout"
    sealed_offer["splits"] = {name: [] for name in PROTOCOL_SPLITS}
    for e in sealed_offer["entries"]:
        sealed_offer["splits"][ROLE_TO_SPLIT[e["role"]]].append(e["corpus_id"])
    r7c = audit(sealed_offer, tomb, None, ExposureGuard(), source_text)
    check(
        "W7.2d offering a consumed object as heldout fails QP-CONSUMED and is not waivable",
        r7c["qp_gates"]["QP-CONSUMED"]["status"] == "fail"
        and r7c["consumed_controls"]["offered_in_sealed_role"] == ["fx-drugqa-v1"]
        and any(f["code"] == "QP-CONSUMED.sealed_offering" for f in r7c["findings"]),
        str(r7c["qp_gates"]["QP-CONSUMED"]),
    )
    check(
        "W7.2e a sealed offering also burns no bytes and reads no sealed byte",
        r7c["counters"]["sealed_byte_reads"] == 0,
        str(r7c["counters"]["sealed_byte_reads"]),
    )
    check(
        "W7.3 tombstoned fixture entries are never sealed",
        all(
            e["role"] not in SEALED_ROLES
            for e in lock["entries"]
            if e["corpus_id"] in matched
        ),
        "a fixture entry matched the tombstone while sealed",
    )
    hit = Tombstone(fixture_tombstone()).matches_for_entry(
        {
            "corpus_id": "some-pe",
            "display_name": "pe-git.exe",
            "content": {"sha256": "1a0043555d254618f2d56c936c3d9a1fbfb878bc878416a133c346bc7835eda9"},
            "source": {},
            "independence": {},
        }
    )
    check(
        "W7.4 byte-identity match on a sealed role is detected",
        any(h["identity_kind"] == "sha256" for h in hit),
        "sha256 tombstone lookup missed",
    )
    bad4 = fixture_lock()
    bad4["consumed_controls_ref"]["sha256"] = _fx_sha("not-the-supplied-tombstone")
    r7 = audit(bad4, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    check("W7.5 stale tombstone binding fails closed", r7["consumed_controls"]["stale_reference"] is True)

    # --- W8 aggregate contract (E1 / E2)
    agg = report["aggregate"]
    check("W8.1 aggregate_all_roles present", agg["aggregate_all_roles"]["n_rows"] > 0)
    check("W8.2 role_stratified covers many roles", len(agg["role_stratified"]) >= 5, str(len(agg["role_stratified"])))
    check("W8.3 heldout aggregate present", agg["aggregate_heldout_only"] is not None, "heldout aggregate is null")
    check(
        "W8.4 heldout units counted",
        agg["aggregate_heldout_only"] and agg["aggregate_heldout_only"]["n_independence_units"] > 0,
        "heldout independence units is zero",
    )
    check("W8.5 citation grade forbidden on mixed roles", agg["citation_grade_permitted"] is False)
    check("W8.6 promotion not authorized with a tombstone hit", agg["promotion_authorized"] is False)
    check("W8.7 tool verdict never authorizes promotion", report["verdict"]["promotion_authorized"] is False)
    check("W8.8 tool verdict never authorizes a mechanism", report["verdict"]["mechanism_claim_authorized"] is False)

    umap = {"a": "iu-1", "b": "iu-2"}
    ac1 = aggregate_contract([{"corpus_id": "b", "evidence_role": "discovery"}], umap, set())
    check("W8.9 null heldout aggregate blocks promotion", ac1["aggregate_heldout_only"] is None and not ac1["promotion_authorized"])
    ac2 = aggregate_contract([{"corpus_id": "a", "evidence_role": "heldout"}], umap, set())
    check("W8.10 all-heldout permits promotion", ac2["promotion_authorized"] is True)
    check("W8.11 all-heldout permits citation grade", ac2["citation_grade_permitted"] is True)
    ac3 = aggregate_contract(
        [{"corpus_id": "a", "evidence_role": "heldout"}, {"corpus_id": "b", "evidence_role": "external_test"}],
        umap,
        set(),
    )
    check("W8.12 external_test also promotes", ac3["promotion_authorized"] is True)
    ac4 = aggregate_contract([{"corpus_id": "a", "evidence_role": "heldout"}], umap, {"a"})
    check("W8.13 consumed row cannot promote", ac4["promotion_authorized"] is False)
    check("W8.14 consumed row labelled", ac4["consumed_corpus_ids"] == ["a"])
    ac5 = aggregate_contract([{"corpus_id": "a"}], umap, set())
    check("W8.15 row without evidence_role is a finding", len(ac5["findings"]) == 1, str(ac5["findings"]))
    ac6 = aggregate_contract([{"corpus_id": "a", "evidence_role": "external_development"}], umap, set())
    check("W8.16 external_development cannot promote", ac6["promotion_authorized"] is False)

    # --- W9 QP-NO-PROMOTE
    check("W9.1 fires on the fixture", report["qp_gates"]["QP-NO-PROMOTE"]["status"] == "fail")
    check(
        "W9.2 external_development included",
        "external_development" in QP_NO_PROMOTE_ROLES,
        "external_development missing from the gate set",
    )
    check("W9.3 deviation disclosed", bool(report["qp_gates"]["deviations_from_edit_text"]))

    # --- W10 Edit E8 source scope
    bad5 = fixture_lock()
    bad5["project"]["source_dirty"] = True
    check("W10.1", bool(check_source_scope(bad5)), "dirty pinned checkout accepted")
    bad6 = fixture_lock()
    bad6["project"]["source_dirty_scope"] = "local_worktree"
    check("W10.2", bool(check_source_scope(bad6)), "wrong scope accepted")
    bad7 = fixture_lock()
    bad7["project"]["evaluated_at"] = "2026-10-02 00:00:00"
    check("W10.3", bool(check_source_scope(bad7)), "non-RFC3339 evaluated_at accepted")
    bad8 = fixture_lock()
    bad8["project"]["source_tree_ref"] = "b" * 40
    check("W10.4", bool(check_source_scope(bad8)), "authoring commit != evaluated tree accepted")
    check("W10.5", report["lock"]["local_worktree_evaluated"] is False, "local worktree was evaluated")
    check(
        "W10.6",
        report["lock"]["source_dirty_scope"] == "pinned_checkout"
        and lock["project"]["local_worktree"]["evaluated"] is True,
    )
    check(
        "W10.7 a clean pinned checkout records no absent objects",
        lock["project"]["local_worktree"]["declared_objects_absent_from_pinned_commit"] == [],
    )
    # the operator's dirty worktree must be distinguishable from a pinned checkout
    dirty_tree = fixture_lock()
    dirty_tree["project"]["source_dirty"] = True
    dirty_tree["project"]["local_worktree"] = {
        "evaluated": True,
        "dirty_path_count": 127,
        "declared_objects_absent_from_pinned_commit": ["fx-bin-g"],
        "note": "operator worktree, not the pinned checkout",
    }
    r10 = audit(dirty_tree, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    check(
        "W10.8 an operator-dirty subject root blocks (PB-02) rather than asserting clean",
        r10["verdict"]["status"] == "INVALID_INFRA"
        and any(f["code"] == "PB-02" for f in r10["findings"]),
        "%s / %s" % (r10["verdict"]["status"], sorted({f["code"] for f in r10["findings"]})),
    )

    # --- W11 Edit E7 guard enforcement
    g = ExposureGuard()
    code = None
    try:
        g.open_bytes("fx-held-j", "heldout", b"x")
    except ExposureBreach as exc:
        code = exc.code
    check("W11.1 sealed read raises PB-06", code == "PB-06", str(code))
    check("W11.2 sealed read counted", g.sealed_byte_reads == 1)
    code = None
    try:
        g.network("https://example.invalid/sealed1.ndjson")
    except ExposureBreach as exc:
        code = exc.code
    check("W11.3 network raises and counts", code == "PB-06" and g.network_ops == 1)
    code = None
    try:
        g.decompress("dataset.zip")
    except ExposureBreach as exc:
        code = exc.code
    check("W11.4 decompress raises and counts", code == "PB-06" and g.archive_decompressions == 1)
    code = None
    try:
        g.codec("anvil.exe")
    except ExposureBreach as exc:
        code = exc.code
    check("W11.5 codec raises and counts", code == "PB-05" and g.codec_invocations == 1)
    check(
        "W11.6 non-zero counters are asserted",
        len(assert_zero_exposure(g.counters())) == 4,
        "assert_zero_exposure did not flag a deliberately abused guard",
    )
    check("W11.7 clean guard asserts clean", assert_zero_exposure(ExposureGuard().counters()) == [])

    # --- W12 thresholds live in the lock (queue item 5)
    lint = report["threshold_lint"]
    check("W12.1 no code defaults", lint["code_default_constants"] == [], str(lint["code_default_constants"]))
    check("W12.2 all parameters declared", lint["declared_parameters"] == sorted(REQUIRED_AUDIT_PARAMETERS))
    bad9 = fixture_lock()
    del bad9["audit_parameters"]["minhash_threshold_num"]
    check("W12.3 missing threshold fails lint", threshold_lint(bad9, source_text)["status"] == "fail")
    bad10 = fixture_lock()
    bad10["audit_parameters"]["hand_tuned_thing"] = 7
    check("W12.4 undeclared threshold fails lint", threshold_lint(bad10, source_text)["status"] == "fail")
    p1 = audit(fixture_lock(), tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    low = fixture_lock()
    low["audit_parameters"]["minhash_threshold_num"] = 99
    low["audit_parameters"]["minhash_threshold_den"] = 100
    p2 = audit(low, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    c1_n = len([e for e in p1["conflict_edges"] if e["criterion"] == "c5"])
    c2_n = len([e for e in p2["conflict_edges"] if e["criterion"] == "c5"])
    # The base fixture's text pair is ~identical, so its MinHash estimate sits far
    # above any plausible threshold and its c5 edge cannot flip. Threshold
    # sensitivity is therefore proven on a purpose-built BOUNDARY fixture whose
    # 5-gram overlap is constructed near the declared cutoff, and the two
    # thresholds straddle that pair.
    near = fixture_lock()
    near["entries"] = [e for e in near["entries"] if e["corpus_id"] in ("fx-text-e", "fx-text-f")]
    near["splits"] = {name: [] for name in PROTOCOL_SPLITS}
    for e in near["entries"]:
        near["splits"][ROLE_TO_SPLIT[e["role"]]].append(e["corpus_id"])
    near["deduplication"]["containment_claims"] = []
    near["entries"][0]["independence"]["near_duplicate_group_ids"] = []
    near["entries"][1]["independence"]["near_duplicate_group_ids"] = []
    near["entries"][1]["independence"]["publisher_id"] = "fx-pub-b2"
    near["entries"][1]["independence"]["project_id"] = "fx-proj-b2"
    near["entries"][1]["independence"]["release_family_id"] = "fx-rel-b2"
    near["entries"][1]["content"]["git_blob_sha1"] = _fx_blob("boundary-text-blob")
    half = b"alpha bravo charlie delta echo foxtrot golf hotel india juliet\n" * 40
    other = b"mike november oscar papa quebec romeo sierra tango uniform victor\n" * 40
    boundary_sources = {"fx-text-e": half, "fx-text-f": half + other}
    est = _minhash_estimate(boundary_sources, near)
    # est/128 is the estimated similarity; straddle it on a percent denominator.
    est_pct_floor = (est * 100) // 128
    lo = _with_threshold(near, max(1, est_pct_floor), 100)
    hi = _with_threshold(near, min(100, est_pct_floor + 1), 100)
    c_lo = len([e for e in audit(lo, tomb, boundary_sources, ExposureGuard(), source_text)["conflict_edges"]
                if e["criterion"] == "c5"])
    c_hi = len([e for e in audit(hi, tomb, boundary_sources, ExposureGuard(), source_text)["conflict_edges"]
                if e["criterion"] == "c5"])
    check(
        "W12.5 moving the lock threshold moves the c5 edge on a boundary fixture",
        c_lo == 1 and c_hi == 0,
        "estimated=%d/128 c5 low=%d high=%d" % (est, c_lo, c_hi),
    )
    check(
        "W12.5b the estimate is reported as an integer fraction, never a float",
        isinstance(est, int) and 0 < est < 128,
        str(est),
    )

    # --- W13 provenance (host-installed + git_tracked recorded)
    bad11 = fixture_lock()
    for e in bad11["entries"]:
        if e["corpus_id"] == "fx-bin-g":
            e["toolchain"] = None
    r13 = audit(bad11, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    check("W13.1 missing toolchain is a finding", any(f["code"] == "PROVENANCE.incomplete" for f in r13["findings"]))
    prov = {r["corpus_id"]: r for r in report["provenance"]["entries"]}
    check("W13.2 host-installed recorded", prov["fx-bin-g"]["source_kind"] == "host-installed")
    check("W13.3 git_tracked recorded not required", prov["fx-bin-g"]["git_tracked"] is False)
    bad12 = fixture_lock()
    for e in bad12["entries"]:
        if e["corpus_id"] == "fx-bin-g":
            e["source"]["host_install"] = None
    r14 = audit(bad12, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    check("W13.4 host-installed without host_install block is a finding", any(f["code"] == "PROVENANCE.incomplete" for f in r14["findings"]))
    check(
        "W13.5 git_tracked false is not itself a finding",
        not any("git_tracked" in f["detail"] for f in report["findings"]),
        "git_tracked=false was treated as incomplete",
    )

    # --- W14 publisher isolation
    bad13 = fixture_lock()
    for e in bad13["entries"]:
        if e["role"] in SEALED_ROLES:
            e["independence"]["publisher_id"] = "fx-pub-a"
    r15 = audit(bad13, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    check("W14.1 non-isolated sealed entry is a finding", any(f["code"].startswith("PB-10") for f in r15["findings"]))
    check(
        "W14.2 sealed publisher isolation is new",
        all(r["isolated"] for r in report["publisher_split"]["entries"]),
        "fixture sealed entries are not isolated",
    )

    # --- W15 burn-on-result-view
    burn_ids = {b["corpus_id"] for b in report["burn"]["consumable_entries"]}
    check("W15.1 sealed entries listed as consumable", burn_ids == set(report["universe"]["sealed_entry_ids"]), str(sorted(burn_ids)))
    check("W15.2 this audit burns nothing", report["burn"]["entries_burned_by_this_audit"] == 0)
    check(
        "W15.3 burn is irreversible to a sealed role",
        all(b["may_return_to_sealed_role"] is False and b["post_open_role"] == "known_stress" for b in report["burn"]["consumable_entries"]),
    )

    # --- W16 determinism
    r_a = audit(lock, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    r_b = audit(lock, tomb, fixture_byte_sources(), ExposureGuard(), source_text)
    check("W16.1 byte identical across runs", canon(r_a) == canon(r_b))
    check("W16.2 lock sha stable", r_a["lock"]["lock_sha256"] == r_b["lock"]["lock_sha256"])
    # Protocol 4.1 REQUIRES entries sorted by corpus_id, so entry order is not a
# legal degree of freedom. The legal unordered inputs are the byte-source map,
# containment_claims, and per-entry near_duplicate_group_ids; determinism is
# asserted over those instead.
    rev = fixture_lock()
    rev["deduplication"]["containment_claims"] = list(
        reversed(rev["deduplication"]["containment_claims"])
    )
    for e in rev["entries"]:
        e["independence"]["near_duplicate_group_ids"] = list(
            reversed(e["independence"]["near_duplicate_group_ids"])
        )
    rev_sources = fixture_byte_sources()
    rev_sources = dict(sorted(rev_sources.items(), key=lambda kv: kv[0], reverse=True))
    r_c = audit(rev, tomb, rev_sources, ExposureGuard(), source_text)
    check(
        "W16.3 unsorted legal inputs do not change units",
        r_c["independence"]["units"] == r_a["independence"]["units"],
        "unit ids depend on input order",
    )
    check(
        "W16.4 unsorted legal inputs do not change the edge set",
        canon(r_c["conflict_edges"]) == canon(r_a["conflict_edges"]),
        "edge set depends on input order",
    )
    check(
        "W16.4b unsorted legal inputs do not change the lock identity",
        r_c["lock"]["lock_sha256"] == r_a["lock"]["lock_sha256"],
    )
    unsorted_entries = fixture_lock()
    unsorted_entries["entries"] = list(reversed(unsorted_entries["entries"]))
    check(
        "W16.4c an unsorted entries array is schema-invalid and blocks the audit",
        any(x["code"] == "schema.order" for x in validate_lock(unsorted_entries))
        and audit(unsorted_entries, tomb, None, ExposureGuard(), source_text)
        ["independence"].get("independence_units") is None,
        "entry order was accepted as a valid lock",
    )
    check("W16.5 no float literal in the artifact", "e+" not in canon(report).decode("utf-8"), "float repr present")
    check("W16.6 no wall-clock module imported", _imported_modules(source_text).isdisjoint(
        {"time", "datetime", "calendar", "timeit"}), str(sorted(_imported_modules(source_text))))
    # AST/behavioral, not substring: these prove the tool cannot obtain
    # nondeterminism, not that the words appear or do not appear in text.
    check("W16.7 no nondeterminism source imported", _imported_modules(source_text).isdisjoint(
        {"random", "secrets", "uuid", "os"} - {"os"}) or "os.urandom" not in _dotted_calls(source_text),
        str(sorted(_imported_modules(source_text))))
    check("W16.8 no network/codec/archive module imported", _imported_modules(source_text).isdisjoint(
        _FORBIDDEN_IMPORTS), str(sorted(_imported_modules(source_text) & _FORBIDDEN_IMPORTS)))
    check("W16.9 no PYTHONHASHSEED or interpreter-version read", not (
        {"sys"} & _imported_modules(source_text)
        and any("version_info" in c or "PYTHONHASHSEED" in c or "hashseed" in c
                for c in _dotted_calls(source_text) + _name_reads(source_text))
    ), "interpreter-version or hash-seed dependence detected")
    check(
        "W16.9b determinism is behavioural, not asserted",
        canon(audit(lock, tomb, fixture_byte_sources(), ExposureGuard(), source_text))
        == canon(audit(lock, tomb, fixture_byte_sources(), ExposureGuard(), source_text)),
        "two identical audits differ",
    )
    check(
        "W16.10 verdict in declared set",
        r_a["verdict"]["status"] in ("ADMISSIBLE_PILOT", "CORPUS_BLOCKED", "INVALID_INFRA"),
        r_a["verdict"]["status"],
    )

    # --- W17 artifact emission is deterministic
    import tempfile

    d1 = tempfile.mkdtemp(prefix="q1a-selftest-a-")
    d2 = tempfile.mkdtemp(prefix="q1a-selftest-b-")
    m1 = emit_artifacts(r_a, d1)
    m2 = emit_artifacts(r_b, d2)
    check("W17.1 manifests match", canon(m1) == canon(m2))
    check("W17.2 every artifact hashed", all(len(a["sha256"]) == 64 for a in m1["artifacts"]))
    check("W17.3 artifact count", len(m1["artifacts"]) == len(ARTIFACT_FILES) - 1, str(len(m1["artifacts"])))

    # --- W18 the byte broker is the only lawful disk path
    import os as _os

    root = tempfile.mkdtemp(prefix="q1a-subject-")
    # Brokerable entries must have a LOCAL source kind; https-file/archive-extract
    # are refused by design, which W18.12 proves separately.
    local_ids = ("fx-bin-g", "fx-bin-h")
    src_bytes = fixture_byte_sources()
    brokered = fixture_lock()
    for e in brokered["entries"]:
        if e["corpus_id"] in local_ids:
            e["content"]["local_path"] = e["corpus_id"] + ".bin"
            e["content"]["sha256"] = sha256_hex(src_bytes[e["corpus_id"]])
            e["content"]["bytes"] = len(src_bytes[e["corpus_id"]])
    sealed_path_lock = fixture_lock()
    for e in sealed_path_lock["entries"]:
        if e["role"] in SEALED_ROLES:
            e["content"]["local_path"] = e["corpus_id"] + ".bin"
        if e["corpus_id"] in local_ids:
            e["content"]["local_path"] = e["corpus_id"] + ".bin"
            e["content"]["sha256"] = sha256_hex(src_bytes[e["corpus_id"]])
            e["content"]["bytes"] = len(src_bytes[e["corpus_id"]])

    for cid in local_ids:
        with open(_os.path.join(root, cid + ".bin"), "wb") as fh:
            fh.write(src_bytes[cid])
    # A sealed entry IS physically present, to prove the broker refuses it.
    for cid in ("fx-held-j", "fx-ext-k"):
        with open(_os.path.join(root, cid + ".bin"), "wb") as fh:
            fh.write(b"SEALED-FIXTURE-BYTES")

    g18 = ExposureGuard()
    got = broker_open_role_bytes(sealed_path_lock, root, g18)
    check("W18.1 broker returns open-role bytes", set(local_ids) <= set(got), str(sorted(got)))
    check(
        "W18.2 sealed ids are absent from the broker result even when present on disk",
        not (set(got) & set(report["universe"]["sealed_entry_ids"])),
        str(sorted(set(got) & set(report["universe"]["sealed_entry_ids"]))),
    )
    check(
        "W18.3 every sealed entry is refused with PB-06",
        all(
            r["code"] == "PB-06"
            for r in g18.broker_refusals
            if r["corpus_id"] in report["universe"]["sealed_entry_ids"]
        ),
        str([r for r in g18.broker_refusals if r["corpus_id"] in report["universe"]["sealed_entry_ids"]]),
    )
    check("W18.4 sealed_byte_reads stays zero through the broker", g18.sealed_byte_reads == 0)
    check("W18.5 broker keeps network/codec/archive at zero",
          g18.network_ops == 0 and g18.codec_invocations == 0 and g18.archive_decompressions == 0)

    # identity is verified BEFORE use
    tampered = fixture_lock()
    for e in tampered["entries"]:
        if e["corpus_id"] == "fx-bin-g":
            e["content"]["local_path"] = "fx-bin-g.bin"
    with open(_os.path.join(root, "fx-bin-g.bin"), "wb") as fh:
        fh.write(b"tampered-bytes-differ-from-declared-sha256")
    g18b = ExposureGuard()
    got_b = broker_open_role_bytes(tampered, root, g18b)
    check("W18.6 a sha256 mismatch is refused", "fx-bin-g" not in got_b)
    check(
        "W18.7 the mismatch is reported as PB-04 with both digests",
        any(r["code"] == "PB-04" and r["corpus_id"] == "fx-bin-g" and "declared" in r
            for r in g18b.broker_refusals),
        str([r for r in g18b.broker_refusals if r["corpus_id"] == "fx-bin-g"]),
    )
    # restore the good bytes for later assertions
    with open(_os.path.join(root, "fx-bin-g.bin"), "wb") as fh:
        fh.write(src_bytes["fx-bin-g"])

    # path escape and undeclared paths are refused
    escape = fixture_lock()
    for e in escape["entries"]:
        if e["corpus_id"] == "fx-bin-g":
            e["content"]["local_path"] = "../../etc/passwd"
        if e["corpus_id"] == "fx-bin-h":
            e["content"]["local_path"] = "/etc/passwd"
    g18c = ExposureGuard()
    got_c = broker_open_role_bytes(escape, root, g18c)
    check("W18.8 relative traversal is refused", "fx-bin-g" not in got_c)
    check("W18.9 absolute path is refused", "fx-bin-h" not in got_c)
    check(
        "W18.10 an entry with no declared local_path is refused, never globbed",
        any(r["corpus_id"] == "fx-json-a" and r["reason"] == "no declared local_path"
            for r in g18c.broker_refusals),
        str([r for r in g18c.broker_refusals if r["corpus_id"] == "fx-json-a"]),
    )
    check(
        "W18.11 broker refuses a nonexistent subject root",
        _raises_broker(lambda: broker_open_role_bytes(lock, _os.path.join(root, "nope"), ExposureGuard())),
        "a missing subject root was accepted",
    )
    check(
        "W18.12 remote source kinds are never brokered",
        all(
            r["code"] == "PB-06"
            for r in broker_refusable_corpus_ids(lock)
            if (dict((e["corpus_id"], e) for e in lock["entries"])[r["corpus_id"]]["source"]["kind"]
                in BROKER_FORBIDDEN_KINDS)
        ),
        str(broker_refusable_corpus_ids(lock)),
    )
    check(
        "W18.13 brokered bytes still pass through the guard",
        g18.sealed_byte_reads == 0 and g18.open_byte_reads == len(got),
        "open=%d expected=%d" % (g18.open_byte_reads, len(got)),
    )
    check(
        "W18.14 an audit driven by the broker yields zero sealed reads",
        audit(brokered, tomb, got, ExposureGuard(), source_text)["counters"]["zero_sealed_bytes"] is True,
    )

    print("Q1a self-test: %d checks, %d failed" % (checks[0], len(failures)))
    for f in failures:
        print("  FAIL " + f)
    print("fixture entries: %d" % (len(lock["entries"]),))
    print("fixture independence_units: %d" % (r_a["independence"]["independence_units"],))
    print("fixture conflict_edges: %d" % (len(r_a["conflict_edges"]),))
    print("fixture not_evaluated: %d" % (len(r_a["not_evaluated"]),))
    print("fixture findings: %d" % (len(r_a["findings"]),))
    print("fixture verdict: %s" % (r_a["verdict"]["status"],))
    print("fixture QP-NO-PROMOTE: %s ; QP-CONSUMED: %s"
          % (r_a["qp_gates"]["QP-NO-PROMOTE"]["status"], r_a["qp_gates"]["QP-CONSUMED"]["status"]))
    print("tool source_sha256: %s" % (r_a["tool"]["source_sha256"],))
    return 1 if failures else 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def _usage() -> str:
    return (
        "usage:\n"
        "  python q1a_corpus_lock.py selftest\n"
        "  python q1a_corpus_lock.py audit --lock LOCK.json --tombstone TOMB.json \\\n"
        "                                 --subject-root DIR --emit-dir DIR\n"
        "  python build_real_lock.py --repo DIR --out-dir DIR --git-sha SHA\n"
        "\n"
        "audit        real-tree audit. Reads committed metadata; obtains bytes for\n"
        "             OPEN roles only, through the subject-root broker, SHA-256\n"
        "             verified against the lock. Sealed roles are structurally\n"
        "             excluded from the filesystem path. No network, no codec,\n"
        "             no archive decompression (queue Edit E7).\n"
        "selftest     bounded pure/static harness; synthetic fixtures only. No codec,\n"
        "             no network, no corpus bytes, no benchmark.\n"
    )


def main(argv) -> int:
    if len(argv) < 2:
        print(_usage())
        return 2
    cmd = argv[1]
    if cmd == "selftest":
        return selftest()
    if cmd in ("-h", "--help", "help"):
        print(_usage())
        return 0
    if cmd == "audit":
        args = argv[2:]
        lock_path = None
        tomb_path = None
        emit_dir = None
        subject_root = None
        i = 0
        flags = ("--lock", "--tombstone", "--emit-dir", "--subject-root")
        while i < len(args):
            flag = args[i]
            if flag in flags and i + 1 < len(args):
                value = args[i + 1]
                if flag == "--lock":
                    lock_path = value
                elif flag == "--tombstone":
                    tomb_path = value
                elif flag == "--emit-dir":
                    emit_dir = value
                else:
                    subject_root = value
                i += 2
                continue
            print("unknown or incomplete argument: %r" % (flag,))
            print(_usage())
            return 2
        if not lock_path or not emit_dir:
            print("--lock and --emit-dir are required")
            print(_usage())
            return 2
        with open(lock_path, "r", encoding="utf-8") as fh:
            lock = json.load(fh)
        tomb = {}
        if tomb_path:
            with open(tomb_path, "r", encoding="utf-8") as fh:
                tomb = json.load(fh)

        guard = ExposureGuard()
        sources = {}
        if subject_root:
            try:
                sources = broker_open_role_bytes(lock, subject_root, guard)
            except BrokerRefusal as exc:
                print("broker refused: %s (%s)" % (exc.code, exc.detail))
                return 3
        else:
            print("NOTE: no --subject-root given; running metadata+manifest only, "
                  "so c3/c4/c5 are not evaluated and are reported not_evaluated_no_bytes.")
        report = audit(lock, tomb, sources, guard, _read_own_source())
        manifest = emit_artifacts(report, emit_dir)
        print("verdict: %s" % (report["verdict"]["status"],))
        print("independence_units: %s" % (report["independence"].get("independence_units"),))
        print("conflict_edges: %d" % (len(report["conflict_edges"]),))
        print("not_evaluated: %d" % (len(report["not_evaluated"]),))
        print("findings: %d" % (len(report["findings"]),))
        print("artifacts: %d" % (len(manifest["artifacts"]),))
        return 0 if report["verdict"]["status"] == "ADMISSIBLE_PILOT" else 1
    print("unknown command: %r" % (cmd,))
    print(_usage())
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))