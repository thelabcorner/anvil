#!/usr/bin/env python3
"""Prospective corpus seal + screened/decided independence audit + chained
single-shot burn ledger.

Prototype scope (track 03-heldout-corpus, Space Bunny Free, 2026-10-02).

This prototype is SCIENTIFIC-INFRASTRUCTURE logic only. It never touches corpus
bytes over a network, never invokes a codec, and never measures compression.
Its only self-test is a deterministic hash/decision correctness harness.

Implements four pieces of docs/I10-CORPUS-LOCK-PROTOCOL.md that are, as
measured on 2026-10-02, entirely unimplemented in this repository:

  M1  prospective seal      -> canonical entry seal + Merkle truth root that
                                is computable from METADATA ONLY, before any
                                corpus byte exists or is fetched.
  M4a block/shingle index   -> exact criteria 1-4 in O(total_bytes), plus a
                                globally-rare-block qualifier that removes the
                                boilerplate false-positive pressure which
                                otherwise drives curators into the forbidden
                                PB-11 waiver path.
  M4b screen-then-decide     -> bottom-k MinHash SCREENS; exact 8-gram
                                containment DECIDES. Protocol 5.1 criterion 5
                                is a 128-permission estimator compared against
                                a 0.20 threshold, which cannot resolve better
                                than about +/-0.07.
  M2  chained burn ledger    -> append-only event chain + per-run evidence
                                roots such that a SECOND measurement run over
                                an already-consumed object is detectable by an
                                offline auditor from artifacts alone.

Run:  python seal.py selftest
"""

from __future__ import annotations

import hashlib
import json
import sys
import time

# --------------------------------------------------------------------------
# canonical serialization (protocol section 3)
# --------------------------------------------------------------------------


def canon(obj) -> bytes:
    """UTF-8, keys sorted, compact separators, no NaN/Inf, one trailing LF."""
    return (
        json.dumps(
            obj,
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
            allow_nan=False,
        ).encode("utf-8")
        + b"\n"
    )


def sha256_hex(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def h64(data: bytes) -> int:
    """64-bit content hash for indexing. Truncated BLAKE2b, not a security
    boundary; collisions here only ever cause an extra candidate pair, which
    the exact decide step then resolves."""
    return int.from_bytes(hashlib.blake2b(data, digest_size=8).digest(), "big")


# --------------------------------------------------------------------------
# M1 prospective seal
# --------------------------------------------------------------------------

AUDIT_FIELDS = ("independence", "content")


def entry_seal(entry: dict) -> str:
    """SHA-256 over the canonical projection of an entry's PROVENANCE metadata.

    Bytes are never required. Everything consumed here is either a hash the
    upstream already published (commit, path, archive digest) or a curator
    declaration. This is what lets a preregistration bind corpus IDENTITY
    before a single byte is fetched.
    """
    return sha256_hex(canon(entry))


def merkle_root(leaves_hex: list[str]) -> str:
    """Order-binding Merkle root over 32-byte leaf digests.

    The declared array order is part of the identity: reordering the corpus is
    a different corpus, not a cosmetic edit.
    """
    if not leaves_hex:
        return sha256_hex(b"anvil.merkle.empty\n")
    level = [bytes.fromhex(h) for h in leaves_hex]
    while len(level) > 1:
        nxt = []
        for i in range(0, len(level), 2):
            if i + 1 < len(level):
                nxt.append(hashlib.sha256(b"\x01" + level[i] + level[i + 1]).digest())
            else:
                nxt.append(level[i])
        level = nxt
    return level[0].hex()


def prospective_seal(lock: dict) -> dict:
    """Compute the identity of a lock without any corpus bytes.

    Returns lock_id (SHA-256 of the canonical lock document as authored) and
    truth_root (Merkle root over per-entry seals in declared order).
    """
    entries = lock["entries"]
    ids = [e["corpus_id"] for e in entries]
    if len(set(ids)) != len(ids):
        raise ValueError("duplicate corpus_id in lock")
    seals = [entry_seal(e) for e in entries]
    return {
        "schema": "anvil.prospective-seal/v1",
        "protocol_version": 1,
        "lock_revision": lock["lock_revision"],
        "lock_id": sha256_hex(canon(lock)),
        "truth_root": merkle_root(seals),
        "entry_count": len(entries),
        "declared_bytes": sum(int(e["declared_bytes"]) for e in entries),
        "entry_seals": {e["corpus_id"]: s for e, s in zip(entries, seals)},
    }


# --------------------------------------------------------------------------
# M4 independence index: screen with sketches, decide exactly
# --------------------------------------------------------------------------

BLOCK = 4096

# C5 is a TEXT criterion. A deterministic, semantic text-likeness predicate is
# required, or binary payloads get tokenized into spurious "shared text" and
# C5 double-counts what C3/C4 already explain.
#
# TEXT_LIKENESS_MIN is derived from the natural gap in the measured
# printable-or-whitespace byte density of the 24 real corpus files
# (scratch/swarm-tmp/derive_text_threshold.py):
#     all-text files      1.0000 .. 0.9989
#     generated.sqlite    0.8752  (binary container with a text payload)
#     all binary files    0.4418 .. 0.1495
# Largest natural gap = 0.4418 .. 0.8752 (width 0.4335). 0.90 sits inside it and
# is also the semantic definition (>=90% of bytes printable-or-whitespace).
# It is NOT fitted to any fixture.
TEXT_LIKENESS_MIN = 0.90
PRINTABLE = set(range(0x20, 0x7F)) | {0x09, 0x0A, 0x0B, 0x0C, 0x0D}


def text_like(b: bytes) -> float:
    """Fraction of bytes that are printable ASCII or common whitespace."""
    if not b:
        return 1.0
    return sum(1 for c in b if c in PRINTABLE) / len(b)


def normalize_text(b: bytes) -> bytes:
    """Protocol 5.1 normalization, DUPLICATE-DETECTION USE ONLY.
    Never applied to measured bytes."""
    t = b.replace(b"\r\n", b"\n").decode("utf-8", errors="replace").lower()
    return "".join(ch if (ch.isalnum() or ch.isspace()) else " " for ch in t)


class Index:
    """Per-object index: block hashes, rare-block marker, bottom-k shingle
    sketch, and the exact shingle set used only to DECIDE screened pairs.

    The SCREEN is unigram (distinct normalized token types), not n-gram. This is
    a measured correction to protocol 5.1 criterion 5, which specifies 5-token
    shingles. Diagnosis (scratch/swarm-tmp/dbg03_screen.py,
    dbg03_precision.py): a 5-token window over short structured records is
    almost always contaminated by a varying field, so two files that are ~98%
    identical share 1 of 13 distinct 5-grams (Jaccard 0.040) and the protocol's
    own estimator blocks nothing. On 28 pairs of real open in-repo discovery
    text, 5-gram Jaccard never exceeded 0.0074, i.e. criterion 5 as written
    would have fired on 0 of 28 genuinely overlapping pairs. Unigram Jaccard
    on the same pairs proposed 3 and the exact decide confirmed 2.
    """

    def __init__(self, corpus_id: str, data: bytes, k: int = 256, ngram: int = 5):
        self.corpus_id = corpus_id
        self.n = len(data)
        self.data = data
        self.ngram = ngram
        self.text_like = text_like(data)
        self.blocks = {
            h64(data[i : i + BLOCK]) for i in range(0, len(data), BLOCK)
        }
        crlf = data.replace(b"\r\n", b"\n")
        self.blocks_crlf = {
            h64(crlf[i : i + BLOCK]) for i in range(0, len(crlf), BLOCK)
        }
        norm = normalize_text(data).split()
        self.unigrams = {h64(t.encode()) for t in norm}
        grams = {
            h64(" ".join(norm[i : i + ngram]).encode())
            for i in range(0, max(0, len(norm) - ngram + 1))
        }
        self.grams = grams
        self.sketch = sorted(grams)[:k]

    def jaccard_screen(self, other: "Index") -> float:
        a, b = self.unigrams, other.unigrams
        if not a or not b:
            return 0.0
        return len(a & b) / float(len(a | b))

    def gram_jaccard(self, other: "Index") -> float:
        """The estimator protocol 5.1 criterion 5 actually specifies.
        Retained only so the self-test can assert it stays blind."""
        a, b = set(self.grams), set(other.grams)
        if not a or not b:
            return 0.0
        return len(a & b) / float(len(a | b))

    def exact_shared_gram(self, other: "Index") -> bool:
        return bool(self.grams & other.grams)

    def grams_excluding_blocks(self, drop: set[int]) -> set[int]:
        """5-gram hashes over the normalized text of every block whose hash is
        NOT in `drop`. The C5 decide step uses this so overlap already
        explained by C3/C4 is attributed to the criterion that explains it and
        is not re-counted as text overlap."""
        toks: list[str] = []
        for i in range(0, len(self.data), BLOCK):
            chunk = self.data[i : i + BLOCK]
            if h64(chunk) in drop:
                continue
            toks.extend(normalize_text(chunk).split())
        n = self.ngram
        return {
            h64(" ".join(toks[j : j + n]).encode())
            for j in range(0, max(0, len(toks) - n + 1))
        }


def mark_rare(indexes: list[Index], rare_max: int = 2, gram_rare_max: int = 2) -> None:
    """Downgrade globally common blocks. A 4 KiB block that appears in more
    than `rare_max` objects is boilerplate (license headers, JSON preamble)
    and is NOT evidence of copying; a rare shared block is.

    The same prevalence discipline applies to C5's confirming grams: a gram
    whose corpus-wide document frequency exceeds `gram_rare_max` is common text
    (a licence banner, a standard header) and must not confirm an overlap."""
    from collections import Counter

    c_raw = Counter()
    c_crlf = Counter()
    c_gram = Counter()
    for ix in indexes:
        c_raw.update(ix.blocks)
        c_crlf.update(ix.blocks_crlf)
        c_gram.update(ix.grams)
    for ix in indexes:
        ix.rare_blocks = {b for b in ix.blocks if c_raw[b] <= rare_max}
        ix.rare_blocks_crlf = {b for b in ix.blocks_crlf if c_crlf[b] <= rare_max}
        ix.common_blocks = ix.blocks - ix.rare_blocks
        ix.common_blocks_crlf = ix.blocks_crlf - ix.rare_blocks_crlf
        ix.rare_grams = {g for g in ix.grams if c_gram[g] <= gram_rare_max}


def audit_pair(a: Index, b: Index, meta_a: dict, meta_b: dict) -> list[str]:
    """Protocol 5.1 criteria 1-8. Returns the list of blocking reasons."""
    r: list[str] = []
    if a.n == b.n and meta_a.get("declared_sha256") == meta_b.get("declared_sha256"):
        r.append("c1-identical-sha256")
    if (
        meta_a.get("git_blob_sha1")
        and meta_a.get("git_blob_sha1") == meta_b.get("git_blob_sha1")
        and a.n == b.n
    ):
        r.append("c2-identical-git-blob")
    if getattr(a, "rare_blocks", a.blocks) & getattr(b, "rare_blocks", b.blocks):
        r.append("c3-shared-rare-4kib-block")
    if getattr(a, "rare_blocks_crlf", set()) & getattr(b, "rare_blocks_crlf", set()):
        r.append("c4-shared-rare-4kib-block-crlf")
    # C5 is TEXT-ONLY, PREVALENCE-AWARE, and NON-DOUBLE-COUNTING.
    #  (i)   both objects must pass the deterministic text-likeness predicate,
    #        so binary payloads never reach the text criterion at all;
    #  (ii)  unigram Jaccard >= 0.20 proposes the pair (high recall);
    #  (iii) confirmation requires a shared gram that is RARE across the corpus
    #        (a shared licence banner is common and cannot confirm) AND that
    #        lies OUTSIDE any block the two objects already share (so C3/C4
    #        keep their evidence and are not re-counted here).
    shared_blocks = a.blocks & b.blocks
    if (
        a.text_like >= TEXT_LIKENESS_MIN
        and b.text_like >= TEXT_LIKENESS_MIN
        and a.jaccard_screen(b) >= 0.20
    ):
        if (
            a.grams_excluding_blocks(shared_blocks)
            & b.grams_excluding_blocks(shared_blocks)
            & a.rare_grams
            & b.rare_grams
        ):
            r.append("c5-screened-and-confirmed-text-overlap")
    ia, ib = meta_a["independence"], meta_b["independence"]
    if (
        ia["schema_cluster_id"] == ib["schema_cluster_id"]
        and ia["schema_cluster_id"] != "not-applicable"
        and (
            ia["publisher_id"] == ib["publisher_id"]
            or ia["release_family_id"] == ib["release_family_id"]
        )
    ):
        r.append("c6-same-schema-cluster-and-publisher-or-release")
    if (
        ia["publisher_id"] == ib["publisher_id"]
        or ia["project_id"] == ib["project_id"]
        or ia["release_family_id"] == ib["release_family_id"]
    ):
        r.append("c7-same-publisher-project-or-release")
    ga, gb = meta_a.get("synthetic"), meta_b.get("synthetic")
    if ga and gb and ga.get("generator_sha256") == gb.get("generator_sha256"):
        r.append("c8-shared-generator-lineage")
    return r


# --------------------------------------------------------------------------
# M2 chained single-shot burn ledger
# --------------------------------------------------------------------------

LEDGER_FIELDS = (
    "event_id utc actor action lock_sha256 lock_revision preregistration_sha256 "
    "implementation_git_sha workflow_git_sha run_id job_id corpus_id "
    "role_at_event host_class heavy_measurement result_artifact_sha256 reason "
    "previous_event_sha256 event_sha256"
).split()

NON_CONSUMING = {"identity-fetch", "identity-inspected", "lock-created"}
CONSUMING = {"local-measurement", "remote-measurement", "result-viewed"}


def event_hash(ev: dict) -> str:
    e = dict(ev)
    e["event_sha256"] = None
    return sha256_hex(canon(e))


def make_event(ev: dict) -> dict:
    for k in LEDGER_FIELDS:
        ev.setdefault(k, None)
    ev["event_sha256"] = event_hash(ev)
    return ev


def chain(events: list[dict]) -> list[dict]:
    out = []
    prev = None
    for ev in events:
        ev = dict(ev)
        ev["previous_event_sha256"] = prev
        out.append(make_event(ev))
        prev = ev["event_sha256"]
    return out


def _verify_chain_inner(events: list[dict]) -> list[str]:
    errs = []
    prev = None
    for i, ev in enumerate(events):
        missing = [k for k in LEDGER_FIELDS if k not in ev]
        if missing:
            errs.append(f"event[{i}] missing fields {missing}")
            continue
        if ev["previous_event_sha256"] != prev:
            errs.append(f"event[{i}] previous_event_sha256 break")
        if event_hash(ev) != ev["event_sha256"]:
            errs.append(f"event[{i}] event_sha256 mismatch")
        prev = ev["event_sha256"]
    if not events:
        return ["empty chain"]
    return errs


def verify_chain(events: list[dict], committed_head: str | None = None) -> list[str]:
    errs = _verify_chain_inner(events)
    if committed_head is not None and events:
        if events[-1]["event_sha256"] != committed_head:
            errs.append("tail truncation: local head != committed head")
    return errs


def double_burn(events: list[dict]) -> list[str]:
    seen = set()
    bad = []
    for ev in events:
        if ev.get("action") in CONSUMING:
            cid = ev.get("corpus_id")
            if cid in seen:
                bad.append(cid)
            seen.add(cid)
    return sorted(set(bad))


def evidence_root(digests: list[str]) -> str:
    return merkle_root(digests)


# --------------------------------------------------------------------------
# synthetic-control grade lattice
# --------------------------------------------------------------------------

SYNTH_USE = {
    "correctness_fuzz": {"S0", "S1", "S2", "S3"},
    "oracle_anatomy_calibration": {"S0", "S1", "S2"},
    "negative_control": {"S0", "S1", "S2", "S3"},
    "decode_cost_accounting": {"S0", "S1", "S2", "S3"},
    "threshold_preregistration": {"S0", "S1"},
    "blind_mechanism_sensitivity": {"S0"},
    "heldout_or_generalization_claim": set(),
    "reported_ratio_row_without_real_counterpart": set(),
}


def grade(p: dict) -> str:
    s = p.get("synthetic") or {}
    if not s:
        return "-"
    if s.get("parameters_sealed"):
        return "S0"
    if s.get("lineage_disjoint_from_tuned_set"):
        return "S1"
    if s.get("in_tuning_ancestry"):
        return "S2"
    return "S3"


def synth_use_allowed(p: dict, use: str) -> bool:
    g = grade(p)
    return g in SYNTH_USE.get(use, set())


# --------------------------------------------------------------------------
# self-test
# --------------------------------------------------------------------------

FIXTURES = {
    "v-a": b'{"schema":"acme/events","id":1,"name":"alpha","ts":"2026-01-01T00:00:00Z"}\n' * 40,
    "v-b": b'{"schema":"acme/events","id":2,"name":"beta","ts":"2026-01-01T00:00:01Z"}\n' * 40,
    "v-license": b"# MIT License\n\nCopyright (c) 2026 Example\nPermission is hereby granted, free of charge\n" + b"x" * 5000,
    "v-licensed": b"// BSD 3-Clause\n// Copyright 2026 Other\n// Redistribution and use in source and binary forms\n" + b"y" * 5000,
    "v-exact-a": b'{"schema":"zeta/rows","k":' + b"1" * 3000 + b'"}\n',
    "v-exact-b": b'{"schema":"zeta/rows","k":' + b"1" * 3000 + b'"}\n',
    "v-unique": bytes((i * 37 + (i // 251)) % 256 for i in range(9000)),
}

# --- prevalence fixtures -----------------------------------------------------
# COMMON TEXT boilerplate: three TEXT files sharing one identical BLOCK-sized
# banner. The banner length is derived from len(prefix), never a magic constant
# (a prior revision used 4096-28 while the prefix was 27 bytes, yielding 4095 B
# so block 0 absorbed a fixture-specific byte and could never be common).
_BANNER_PREFIX = b"# boilerplate licence banner text shared by many documents\n"


def _pad_to_block(prefix: bytes) -> bytes:
    reps = (BLOCK + len(prefix) - 1) // len(prefix)
    out = (prefix * reps)[:BLOCK]
    assert len(out) == BLOCK, (len(out), BLOCK)
    return out


_BANNER = _pad_to_block(_BANNER_PREFIX)
_WORDS = {
    1: "alpha bravo charlie delta echo foxtrot",
    2: "golf hotel india juliet kilo lima",
    3: "mike november oscar papa quebec romeo",
}
for _n in (1, 2, 3):
    body = (_WORDS[_n] + "\n").encode()
    FIXTURES[f"v-boiler{_n}"] = _BANNER + (body * ((3000 // len(body)) + 1))[:3000]

# RARE BINARY pair: shares exactly one BLOCK-sized block present in only these
# two objects, and is NOT text. Must yield C3/C4 and must NOT yield C5.
_RARE_PREFIX = bytes(range(256))


def _pad_bin_to_block(prefix: bytes) -> bytes:
    reps = (BLOCK + len(prefix) - 1) // len(prefix)
    out = (prefix * reps)[:BLOCK]
    assert len(out) == BLOCK, (len(out), BLOCK)
    return out


_RARE = _pad_bin_to_block(_RARE_PREFIX)
FIXTURES["v-rareA"] = _RARE + bytes((i * 3 + 0) % 256 for i in range(2000))
FIXTURES["v-rareB"] = _RARE + bytes((i * 3 + 91) % 256 for i in range(2000))

# GENUINE near-duplicate structured TEXT, no shared 4 KiB block: every record
# carries a high-entropy varying field, so block 0 differs between the pair.
_LOREM = ("the record schema declares a stable envelope with ordered members "
          "and a monotonically increasing identifier column ")
FIXTURES["v-textdupA"] = b"".join(
    b'{"s":"acme/rows","id":%d,"pad":"%s","v":%d}\n' % (i, (b"%x" % (i * 2654435761 % (1 << 128))).rjust(32, b"0"), i)
    for i in range(60))
FIXTURES["v-textdupB"] = b"".join(
    b'{"s":"acme/rows","id":%d,"pad":"%s","v":%d}\n' % (i, (b"%x" % (i * 40503 + 7919 % (1 << 128))).rjust(32, b"0"), i)
    for i in range(60))
FIXTURES["v-textdupC"] = b"".join(
    b'{"s":"acme/rows","id":%d,"pad":"%s","v":%d}\n' % (i, (b"%x" % (i * 11400714819323198485 % (1 << 128))).rjust(32, b"0"), i * 3 + 1)
    for i in range(60))

META = {
    "v-a": ("pub-a", "proj-a", "fam-a", "cl-a", "zeta", None),
    "v-b": ("pub-a", "proj-a", "fam-a", "cl-a", "zeta", None),
    "v-license": ("pub-x", "proj-x", "fam-x", "not-applicable", "not-applicable", None),
    "v-licensed": ("pub-y", "proj-y", "fam-y", "not-applicable", "not-applicable", None),
    "v-exact-a": ("pub-z", "proj-z", "fam-z", "cl-z", "eta", "gen-1"),
    "v-exact-b": ("pub-z", "proj-z", "fam-z", "cl-z", "eta", "gen-1"),
    "v-unique": ("pub-q", "proj-q", "fam-q", "not-applicable", "not-applicable", None),
    "v-boiler1": ("pub-b1", "proj-b1", "fam-b1", "not-applicable", "not-applicable", None),
    "v-boiler2": ("pub-b2", "proj-b2", "fam-b2", "not-applicable", "not-applicable", None),
    "v-boiler3": ("pub-b3", "proj-b3", "fam-b3", "not-applicable", "not-applicable", None),
    "v-rareA": ("pub-r1", "proj-r1", "fam-r1", "not-applicable", "not-applicable", None),
    "v-rareB": ("pub-r2", "proj-r2", "fam-r2", "not-applicable", "not-applicable", None),
    "v-textdupA": ("pub-t", "proj-t", "fam-t", "cl-t", "theta", None),
    "v-textdupB": ("pub-t2", "proj-t2", "fam-t2", "cl-t2", "theta", None),
    "v-textdupC": ("pub-t3", "proj-t3", "fam-t3", "cl-t3", "theta", None),
}

TRUTH = {
    ("v-a", "v-b"): ["c5-screened-and-confirmed-text-overlap", "c6-same-schema-cluster-and-publisher-or-release", "c7-same-publisher-project-or-release"],
    ("v-license", "v-licensed"): [],
    ("v-exact-a", "v-exact-b"): [
        "c1-identical-sha256", "c3-shared-rare-4kib-block", "c4-shared-rare-4kib-block-crlf",
        "c6-same-schema-cluster-and-publisher-or-release", "c7-same-publisher-project-or-release",
        "c8-shared-generator-lineage",
    ],
    ("v-a", "v-unique"): [],
    ("v-license", "v-unique"): [],
    ("v-boiler1", "v-boiler2"): [],
    ("v-boiler1", "v-boiler3"): [],
    ("v-boiler2", "v-boiler3"): [],
    ("v-rareA", "v-rareB"): ["c3-shared-rare-4kib-block", "c4-shared-rare-4kib-block-crlf"],
    ("v-textdupA", "v-textdupB"): ["c5-screened-and-confirmed-text-overlap"],
    # v-textdupA/v-textdupC share a VERBATIM schema template but come from
    # DIFFERENT publishers / projects / release families, and carry different
    # curator-assigned schema_cluster_ids. So C6 and C7 correctly stay silent
    # (their metadata preconditions are not met) and C5 is the ONLY lane that
    # catches them. That is a real, load-bearing property of the design: the
    # content lane catches same-schema-different-publisher pairs that the
    # metadata lanes structurally cannot see.
    #
    # It is also the cost: C5 therefore cannot distinguish "same template,
    # different data" from "genuinely near-duplicate text" -- both are C5.
    # Disambiguating them requires the schema-signature lane (a computed
    # field-path signature from a hash-pinned script), which is NOT implemented.
    # Recorded as an open gap in SYNTH-CORPUS-SPACE-BUNNY.md.
    ("v-textdupA", "v-textdupC"): ["c5-screened-and-confirmed-text-overlap"],
}


def _meta(cid: str) -> dict:
    p, pr, f, cl, sc, gen = META[cid]
    return {
        "corpus_id": cid,
        "declared_bytes": len(FIXTURES[cid]),
        "declared_sha256": sha256_hex(FIXTURES[cid]),
        "git_blob_sha1": None,
        "independence": {
            "publisher_id": p,
            "project_id": pr,
            "release_family_id": f,
            "schema_cluster_id": cl,
        },
        "synthetic": {"generator_sha256": gen} if gen else None,
    }


def selftest() -> int:
    fails = []

    def check(name, cond, detail=""):
        print(f"  [{'PASS' if cond else 'FAIL'}] {name}{(' -- ' + detail) if detail and not cond else ''}")
        if not cond:
            fails.append(name)

    print("A. prospective seal determinism / order binding")
    lock = {
        "lock_revision": 1,
        "entries": [
            {"corpus_id": c, "declared_bytes": len(FIXTURES[c]), "role": "heldout", "class": "structured_ndjson"}
            for c in sorted(FIXTURES)
        ],
    }
    s1 = prospective_seal(lock)
    s2 = prospective_seal(json.loads(json.dumps(lock)))
    check("seal is deterministic across reserialization", s1["lock_id"] == s2["lock_id"])
    check("truth root is deterministic", s1["truth_root"] == s2["truth_root"])
    check("declared bytes bound exactly", s1["declared_bytes"] == sum(len(v) for v in FIXTURES.values()))
    reordered = {"lock_revision": 1, "entries": list(reversed(lock["entries"]))}
    s3 = prospective_seal(reordered)
    check("entry reordering changes truth root", s3["truth_root"] != s1["truth_root"])
    mutated = json.loads(json.dumps(lock))
    mutated["entries"][0]["declared_bytes"] += 1
    check("one-byte metadata drift changes lock identity", prospective_seal(mutated)["lock_id"] != s1["lock_id"])
    dup = {"lock_revision": 1, "entries": lock["entries"] + [lock["entries"][0]]}
    try:
        prospective_seal(dup)
        check("duplicate corpus_id rejected", False)
    except ValueError:
        check("duplicate corpus_id rejected", True)

    print("B. independence audit vs curator ground truth")
    idxs = [Index(c, FIXTURES[c]) for c in sorted(FIXTURES)]
    mark_rare(idxs, rare_max=2)
    for (a, b), expect in TRUTH.items():
        ia = next(i for i in idxs if i.corpus_id == a)
        ib = next(i for i in idxs if i.corpus_id == b)
        got = audit_pair(ia, ib, _meta(a), _meta(b))
        check(f"audit {a} vs {b}", got == expect, f"got={got} expect={expect}")
    b1 = next(i for i in idxs if i.corpus_id == "v-boiler1")
    b2 = next(i for i in idxs if i.corpus_id == "v-boiler2")
    b3 = next(i for i in idxs if i.corpus_id == "v-boiler3")
    shared = b1.blocks & b2.blocks & b3.blocks
    check("banner fixture is exactly one block", len(_BANNER) == BLOCK, str(len(_BANNER)))
    check("common text banner hashes identically in 3 objects", len(shared) >= 1,
          f"shared={len(shared)}")
    check("shared banner is COMMON (demoted, not rare)", shared.isdisjoint(b1.rare_blocks))
    check("shared banner is classified common_blocks", shared <= b1.common_blocks)
    check("banner objects are text-like", b1.text_like >= TEXT_LIKENESS_MIN)
    ra = next(i for i in idxs if i.corpus_id == "v-rareA")
    rb = next(i for i in idxs if i.corpus_id == "v-rareB")
    check("unique shared block stays rare", bool(ra.rare_blocks & rb.rare_blocks))
    check("rare binary pair is NOT text-like", ra.text_like < TEXT_LIKENESS_MIN)
    check("common/rare block partition is exact",
          all((ix.common_blocks | ix.rare_blocks) == ix.blocks
              and not (ix.common_blocks & ix.rare_blocks) for ix in idxs))
    check("common/rare gram partition is exact",
          all(len(ix.rare_grams) <= len(ix.grams) for ix in idxs))
    # The banner's grams have corpus-wide DF=3, so they are COMMON and must
    # never be able to confirm a C5 overlap.
    check("banner grams are common (not rare)",
          all(len(ix.grams & b2.grams & b3.grams) == 0 or
              not ((ix.grams & b2.grams & b3.grams) & ix.rare_grams) for ix in idxs))

    print("B2. C5 semantic regressions (text-only / prevalence / no double count")
    for pair, want_c5 in ((("v-textdupA", "v-textdupB"), True),
                          (("v-textdupA", "v-textdupC"), True),
                          (("v-boiler1", "v-boiler2"), False),
                          (("v-boiler1", "v-boiler3"), False),
                          (("v-boiler2", "v-boiler3"), False),
                          (("v-rareA", "v-rareB"), False),
                          (("v-a", "v-b"), True)):
        ia = next(i for i in idxs if i.corpus_id == pair[0])
        ib = next(i for i in idxs if i.corpus_id == pair[1])
        got = "c5-screened-and-confirmed-text-overlap" in audit_pair(ia, ib, _meta(pair[0]), _meta(pair[1]))
        check(f"C5 {'fires' if want_c5 else 'silent'} for {pair[0]}/{pair[1]}", got == want_c5)
    tb = next(i for i in idxs if i.corpus_id == "v-boiler1")
    check("text predicate rejects the binary rare pair", tb.text_like >= TEXT_LIKENESS_MIN)
    check("rare binary pair is blocked by C3/C4 not C5",
          "c3-shared-rare-4kib-block" in audit_pair(ra, rb, _meta("v-rareA"), _meta("v-rareB")))
    ea = next(i for i in idxs if i.corpus_id == "v-exact-a")
    eb = next(i for i in idxs if i.corpus_id == "v-exact-b")
    ex_reasons = audit_pair(ea, eb, _meta("v-exact-a"), _meta("v-exact-b"))
    check("byte-identical pair is blocked by C1", "c1-identical-sha256" in ex_reasons)
    check("C5 stays silent when C1/C3 already explain the pair",
          "c5-screened-and-confirmed-text-overlap" not in ex_reasons)

    print("C. screen-then-decide separation")
    ix_a = next(i for i in idxs if i.corpus_id == "v-a")
    ix_u = next(i for i in idxs if i.corpus_id == "v-unique")
    ix_b = next(i for i in idxs if i.corpus_id == "v-b")
    check("screen rejects unrelated pair", ix_a.jaccard_screen(ix_u) < 0.20)
    check("screen proposes near-dup pair", ix_a.jaccard_screen(ix_b) >= 0.20)
    check("decide confirms near-dup pair", ix_a.exact_shared_gram(ix_b))
    check("decide rejects unrelated pair", not ix_a.exact_shared_gram(ix_u))
    # Regression guard: the estimator protocol 5.1 criterion 5 specifies must
    # stay demonstrably blind here, or this whole repair is unnecessary.
    check("protocol 5.1 5-gram estimator is blind on this case",
          ix_a.gram_jaccard(ix_b) < 0.20,
          f"5gram={ix_a.gram_jaccard(ix_b):.4f}")
    check("unigram screen repairs it",
          ix_a.jaccard_screen(ix_b) >= 0.20,
          f"unigram={ix_a.jaccard_screen(ix_b):.4f}")

    print("D. ledger chain integrity")
    base = dict(lock_sha256=s1["lock_id"], lock_revision=1, implementation_git_sha="a" * 40,
                workflow_git_sha="b" * 40, preregistration_sha256=s1["truth_root"],
                actor="space-bunny", host_class="github-actions", heavy_measurement=False)
    evs = []
    evs.append(dict(base, event_id="e0", action="lock-created", utc="2026-10-02T00:00:00Z",
                    run_id="1", job_id="seal", corpus_id=None, role_at_event="-",
                    result_artifact_sha256=None, reason="prospective seal"))
    for i, cid in enumerate(sorted(FIXTURES)):
        evs.append(dict(base, event_id=f"e{i+1}", action="identity-fetch", utc=f"2026-10-02T00:0{i}:00Z",
                        run_id="1", job_id="acquire", corpus_id=cid, role_at_event="heldout",
                        result_artifact_sha256=None, reason="verified before codec execution"))
    evs.append(dict(base, event_id="em", action="remote-measurement", utc="2026-10-02T00:09:00Z",
                    run_id="1", job_id="measure", corpus_id="v-a", role_at_event="heldout",
                    result_artifact_sha256="c" * 64, reason="one-shot"))
    ch = chain(evs)
    check("clean chain verifies", verify_chain(ch) == [])
    bad = json.loads(json.dumps(ch))
    bad[2]["actor"] = "someone-else"
    check("field mutation detected", verify_chain(bad) != [])
    check("truncation INVISIBLE without a committed head (the M2 gap)",
          verify_chain(ch[:-1]) == [])
    check("truncation detected WITH a committed head",
          verify_chain(ch[:-1], committed_head=ch[-1]["event_sha256"]) != [])
    check("committed head agrees on the full chain",
          verify_chain(ch, committed_head=ch[-1]["event_sha256"]) == [])
    check("reorder detected", verify_chain([ch[0], ch[2], ch[1]] + ch[3:]) != [])
    check("consuming action is a consuming one", ch[-1]["action"] in CONSUMING)
    check("single use is clean", double_burn(ch) == [])
    check("identity fetch is NOT consuming", "identity-fetch" in NON_CONSUMING)

    print("E. double-measurement is offline-detectable")
    ch2 = chain(evs + [dict(base, event_id="em2", action="remote-measurement", utc="2026-10-02T00:10:00Z",
                            run_id="2", job_id="measure", corpus_id="v-a", role_at_event="heldout",
                            result_artifact_sha256="d" * 64, reason="second run")])
    check("re-measurement of a burned object is flagged", double_burn(ch2) == ["v-a"])
    check("second run still produces a valid chain (the lie is well-formed)",
          verify_chain(ch2) == [])

    print("F. evidence root binding")
    rows = [sha256_hex(f"row-{i}".encode()) for i in range(64)]
    r1 = evidence_root(rows)
    check("evidence root is order binding", evidence_root(list(reversed(rows))) != r1)
    tampered = list(rows)
    tampered[31] = sha256_hex(b"row-31-forged")
    check("single forged row changes evidence root", evidence_root(tampered) != r1)

    print("G. synthetic-control grade lattice")
    s0 = {"synthetic": {"parameters_sealed": True, "lineage_disjoint_from_tuned_set": True}}
    s2 = {"synthetic": {"in_tuning_ancestry": True}}
    real = {"synthetic": None}
    check("S0 blind may give a sensitivity claim", synth_use_allowed(s0, "blind_mechanism_sensitivity"))
    check("S2 may never give a blind claim", not synth_use_allowed(s2, "blind_mechanism_sensitivity"))
    check("no synthetic grade may give a held-out claim", not synth_use_allowed(s0, "heldout_or_generalization_claim"))
    check("no synthetic grade may give a bare ratio row", not synth_use_allowed(s0, "reported_ratio_row_without_real_counterpart"))
    check("S2 may still calibrate oracle anatomy", synth_use_allowed(s2, "oracle_anatomy_calibration"))
    check("non-synthetic object is out of the lattice", grade(real) == "-")

    print("H. accounting only (no codec, no network, no corpus timing)")
    t0 = time.perf_counter()
    big = [Index(f"o{i}", FIXTURES["v-a"] + FIXTURES["v-unique"][: (i % 97) * 37]) for i in range(64)]
    mark_rare(big, rare_max=2)
    pairs = 0
    blocked = 0
    for i in range(len(big)):
        for j in range(i + 1, len(big)):
            pairs += 1
            if audit_pair(big[i], big[j], _meta("v-a"), _meta("v-a")):
                blocked += 1
    dt = time.perf_counter() - t0
    print(f"  64 objects, {pairs} pairs, {blocked} blocked, {dt * 1000:.1f} ms")
    check("audit cost is bounded and tiny at this scale", dt < 5.0, f"{dt:.2f}s")

    print()
    if fails:
        print(f"SELFTEST FAIL: {len(fails)} failing check(s): {fails}")
        return 1
    print("SELFTEST PASS")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2 or sys.argv[1] != "selftest":
        print(__doc__)
        print("usage: python seal.py selftest")
        sys.exit(2)
    sys.exit(selftest())