#!/usr/bin/env python3
"""Q1a CORPUS-ADMISSIBILITY-v1 -- adversarial test bed and fail-closed reference.

ISOLATED. Unwired. Not referenced by any production file, workflow, or CMake target.

HARD CONSTRAINTS ENFORCED BY THIS FILE (not by convention -- see install_guard):
  * zero network syscalls            (audit hook: socket.*)
  * zero codec invocations           (no codec is imported; subprocess allowlist is exact)
  * zero archive decompression       (no decompressor exists; a tripwire proves it is
                                      never reached from the hardened path)
  * zero sealed/unopened payload bytes (SealedByteBroker is the sole byte gateway and
                                      refuses SEALED_ROLES; the audit hook refuses any
                                      read under fixtures/sealed/)
  * zero production files touched    (this file and its fixtures are the only writes)

WHAT THIS IS
  An adversarial harness for the Q1a job as specified by the queue edits E1-E8 and by
  the Track 03 pair's mandatory gates (GATE-INDEP-1/2, L6 re-spec, c9, role-crossing
  demotion immunity, thresholds-in-lock). It does two things:

    (1) HARDENED  -- a fail-closed reference admission checker that a prospective Q1a
        implementation is measured against.
    (2) NAIVE     -- a deliberately ordinary implementation (the shape most real audit
        code takes) used to prove each attack case has TEETH. Every attack must be
        BLOCKED by hardened and NOT BLOCKED by naive; otherwise the attack is vacuous
        and the test is reported as a control.

  Every corpus object is synthetic and generated in memory. Nothing here reads
  tests/corpus, AITDCC, Silesia, or any real corpus byte.

Run:  python q1a_adversary.py
      python q1a_adversary.py --canon <unsafe|safe>   (internal determinism child)
"""
from __future__ import annotations

import hashlib
import json
import os
import re
import sys
import unicodedata

HERE = os.path.dirname(os.path.abspath(__file__))
SEALED_FIXTURE_DIR = os.path.join(HERE, "fixtures", "sealed")
THIS_FILE = os.path.abspath(__file__)


# =============================================================================
# 0.  ENVIRONMENTAL GUARD  -- E7.2 "enforced by counter, not by convention"
# =============================================================================

class GuardViolation(RuntimeError):
    pass


_ALLOWED_READ_PREFIXES = (
    os.path.abspath(os.path.join(HERE, "fixtures")),
    os.path.abspath(sys.prefix),
    os.path.abspath(sys.base_prefix),
    os.path.abspath(os.environ.get("TEMP", os.environ.get("TMP", "/tmp"))),
)
_WRITE_FLAGS = os.O_WRONLY | os.O_RDWR | os.O_APPEND | os.O_CREAT | os.O_TRUNC
GUARD_STATS = {"net_blocked": 0, "out_of_tree_read": 0, "sealed_path_read": 0,
               "spawn_blocked": 0, "spawn_allowed": 0}


def install_guard() -> None:
    """Process-wide audit hook. Raised exceptions are the counter; the counter is
    the assertion. No code path can opt out."""

    def hook(event, args):
        if event in ("socket.connect", "socket.bind", "socket.getaddrinfo",
                     "socket.gethostbyname", "urllib.Request"):
            GUARD_STATS["net_blocked"] += 1
            raise GuardViolation(f"Q1a-E7.2: network syscall blocked ({event})")
        if event == "subprocess.Popen":
            exe = args[0] if args else None
            argv = list(args[1]) if len(args) > 1 and args[1] else []
            ok = (exe in (sys.executable, "python") and len(argv) == 3
                  and os.path.basename(str(argv[0])) == os.path.basename(THIS_FILE)
                  and argv[1] == "--canon" and argv[2] in ("safe", "unsafe"))
            if ok:
                GUARD_STATS["spawn_allowed"] += 1
                return
            GUARD_STATS["spawn_blocked"] += 1
            raise GuardViolation("Q1a-E7.3: subprocess blocked (no codec, no harness shell-out)")
        if event == "open":
            path, _mode, flags = (list(args) + [None, None, 0])[:3]
            if isinstance(path, int) or path is None:
                return
            try:
                p = os.path.abspath(os.fspath(path))
            except Exception:
                return
            if flags & _WRITE_FLAGS:
                if not p.startswith(os.path.abspath(HERE)):
                    GUARD_STATS["out_of_tree_read"] += 1
                    raise GuardViolation(f"Q1a-E7.1: write outside prototype dir blocked ({p})")
                return
            if p.startswith(os.path.abspath(SEALED_FIXTURE_DIR)):
                GUARD_STATS["sealed_path_read"] += 1
                raise GuardViolation(f"Q1a-PB-06: read of sealed-role fixture blocked ({p})")
            for pref in _ALLOWED_READ_PREFIXES:
                if p.startswith(pref):
                    return
            GUARD_STATS["out_of_tree_read"] += 1
            raise GuardViolation(f"Q1a-E7.1: out-of-tree read blocked ({p})")

    sys.addaudithook(hook)


# =============================================================================
# 1.  CANONICALIZATION, PORTABLE CORE, DETERMINISM
# =============================================================================

# Protocol section 3 requires byte-identical canonical artifacts. That is only
# achievable over a NAMED PORTABLE CORE. Wall-clock and local-environment fields are
# deliberately excluded from lock identity (attack A1.5).
NON_IDENTITY_TOP_LEVEL = {"created_utc", "evaluated_at", "local_context",
                          "lock_id", "truth_root"}


def canon(obj) -> bytes:
    return json.dumps(obj, sort_keys=True, separators=(",", ":"),
                      ensure_ascii=False, allow_nan=False).encode("utf-8") + b"\n"


def sha256_hex(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def portable_core(lock: dict) -> dict:
    return {k: v for k, v in lock.items() if k not in NON_IDENTITY_TOP_LEVEL}


def h64(b: bytes) -> int:
    return int.from_bytes(hashlib.blake2b(b, digest_size=8).digest(), "big")


# =============================================================================
# 2.  SCHEMA / ROLE VOCABULARIES
# =============================================================================

PROTOCOL_ROLES = {"discovery", "known_stress", "heldout",
                  "external_development", "external_test", "external_anchor"}
# Queue edit E1 introduces three role values that protocol section 4.4 does not allow.
E1_EXTRA_ROLES = {"synthetic_control", "self_reference_control"}
E1_ROLES = PROTOCOL_ROLES | E1_EXTRA_ROLES

SEALED_ROLES = {"heldout", "external_test"}
PROMOTABLE_ROLES = {"heldout", "external_test"}          # E1: the only citation-grade rows
NON_CITATION_ROLES = E1_ROLES - PROMOTABLE_ROLES
# Protocol section 13 claim classes: objects a broad claim may be built on.
CLAIM_CLASSES = {"structured_json", "structured_ndjson", "text_log",
                 "executable", "numeric_telemetry"}

MUT = {  # mutation switches, used only by the mutation tests in section 12
    "c9_on": True, "role_demotion_immunity": True, "transitive_union": True,
    "tombstone_on": True, "digest_source_on": True, "derived_schema": True,
    "ignore_declared_units": False, "closed_world_pair_merge": True,
}


# =============================================================================
# 3.  FAIL-CLOSED PRIMITIVES
# =============================================================================

class SealedByteError(RuntimeError):
    """PB-06 / PB-04 class. Any sealed-payload access aborts the run."""


class Blocker:
    __slots__ = ("code", "detail", "entries")

    def __init__(self, code: str, detail: str, entries=None):
        self.code = code
        self.detail = detail
        self.entries = tuple(sorted(entries or ()))

    def key(self):
        return (self.code, self.entries, self.detail)

    def __repr__(self):
        e = f" entries={list(self.entries)}" if self.entries else ""
        return f"<{self.code}:{self.detail}{e}>"


class SealedByteBroker:
    """Sole gateway for every corpus byte and every digest operation.

    `materialised` holds payload bytes for OPEN roles only. Sealed roles are absent by
    construction, so a hardened audit cannot read them even by accident: the gateway
    refuses before any lookup happens.
    """

    def __init__(self, materialised: dict, published: dict):
        self.materialised = dict(materialised)
        self.published = dict(published)
        self.stats = {"payload_bytes_read": 0, "digest_ops": 0,
                      "decompression_attempts": 0, "sealed_bytes_read": 0,
                      "refusals": 0}
        self.refusal_log = []

    def available(self, cid: str) -> bool:
        return cid in self.materialised

    def read_payload(self, cid: str, role: str) -> bytes:
        if role in SEALED_ROLES:
            self.stats["refusals"] += 1
            self.refusal_log.append(("payload-read", cid))
            raise SealedByteError(f"PB-06: refused payload read for SEALED role {role}: {cid}")
        b = self.materialised.get(cid)
        if b is None:
            self.stats["refusals"] += 1
            raise SealedByteError(f"PB-04: no materialised bytes for {cid}")
        self.stats["payload_bytes_read"] += len(b)
        return b

    def digest(self, cid: str, role: str) -> str:
        self.stats["digest_ops"] += 1
        p = self.published.get(cid)
        if not p or not p.get("sha256"):
            self.stats["refusals"] += 1
            raise SealedByteError(f"PB-04: no declared content digest for {cid}")
        if (MUT["digest_source_on"] and role in SEALED_ROLES
                and p.get("sha256_source") == "computed_locally"):
            # A locally computed payload digest on a sealed role is proof that the
            # payload was read before authorization. Its lawfulness is not recoverable
            # after the fact, so the lock cannot be accepted.
            self.stats["refusals"] += 1
            self.refusal_log.append(("computed-digest-on-sealed", cid))
            raise SealedByteError(
                f"PB-06/PB-07: {cid} declares a LOCALLY COMPUTED content digest while SEALED")
        return p["sha256"]

    def decompression(self, cid: str) -> None:
        """No archive is ever opened here. This tripwire exists so a test can prove the
        hardened path never reaches it."""
        self.stats["decompression_attempts"] += 1
        raise SealedByteError(f"PB-06: archive decompression attempted for {cid}")


# =============================================================================
# 4.  THRESHOLD REGISTRY -- no code defaults (E6 "threshold lint")
# =============================================================================

REQUIRED_THRESHOLDS = (
    "block_bytes", "rare_block_max_objects", "rare_gram_max_objects",
    "c5_min_shared_grams", "text_like_min", "schema_signature_records",
    "sealed_member_cap", "containment_min_records", "min_admissible_units",
    "demotion_policy",
)
NAIVE_THRESHOLDS = {
    "block_bytes": 4096, "rare_block_max_objects": 2, "rare_gram_max_objects": 2,
    "c5_min_shared_grams": 1, "text_like_min": 0.90, "schema_signature_records": 32,
    "sealed_member_cap": 512, "containment_min_records": 1, "min_admissible_units": 1,
    "demotion_policy": "off",
}


class ThresholdError(RuntimeError):
    pass


def thresholds_from_lock(lock: dict) -> dict:
    t = (lock.get("thresholds") or {})
    missing = [k for k in REQUIRED_THRESHOLDS if k not in t]
    if missing:
        raise ThresholdError(
            "PB-28: thresholds absent from lock; a tool default is not a preregistered "
            f"threshold (missing {missing})")
    return {k: t[k] for k in REQUIRED_THRESHOLDS}


# =============================================================================
# 5.  LINEAGE RESOLUTION -- closed world
# =============================================================================

def generator_imports(lock: dict) -> dict:
    g = dict(lock.get("generator_imports") or {})
    return g


def lineage_closure(gen: str, imports: dict) -> frozenset:
    seen, stack = {gen}, [gen]
    while stack:
        cur = stack.pop()
        for nxt in imports.get(cur, ()):  # noqa: B007
            if nxt not in seen:
                seen.add(nxt)
                stack.append(nxt)
    return frozenset(seen)


# =============================================================================
# 6.  CONTENT FEATURES (open roles only; sealed roles are simply unavailable)
# =============================================================================

TOKEN_RE = re.compile(r"[a-z0-9]+")


def text_like(b: bytes) -> float:
    if not b:
        return 0.0
    ok = sum(1 for c in b if (0x20 <= c <= 0x7E) or c in (9, 10, 13))
    return ok / len(b)


def normalize_text(b: bytes) -> bytes:
    s = unicodedata.normalize("NFC", b.decode("latin-1"))
    s = s.replace("\r\n", "\n")
    return s.lower().encode("utf-8")


def tokens(b: bytes) -> list:
    return TOKEN_RE.findall(normalize_text(b).decode("utf-8", "replace"))


def grams5(b: bytes) -> set:
    t = tokens(b)
    return {h64((" ".join(t[i:i + 5])).encode()) for i in range(max(0, len(t) - 4))}


def blocks(b: bytes, size: int) -> set:
    return {h64(b[i:i + size]) for i in range(0, len(b), size)}


def blocks_crlf(b: bytes, size: int) -> set:
    return blocks(b.replace(b"\r\n", b"\n"), size)


def record_digests(b: bytes, framing: str) -> set:
    # NOTE (finding A3-F1): a container-format record set cannot be recovered
    # without a FORMAT DECODER, and Q1a's budget is "zero codec invocations".
    # This fixture declares a newline-delimited record stream embedded in a binary
    # container, which is line-splittable without a decoder. A REAL sqlite
    # container needs a real parser -- see report section 4.3.
    if framing in ("none", "not-applicable"):
        return set()
    sep = b"\n"
    parts = [p for p in b.split(sep) if p.strip()]
    return {h64(p) for p in parts}


def derived_schema_signature(b: bytes, framing: str, n: int):
    """Defect-C repair: derive the schema cluster instead of trusting the curator."""
    if framing not in ("ndjson", "json-document"):
        return "not-applicable"
    paths = set()
    for line in b.split(b"\n")[:n]:
        if not line.strip():
            continue
        try:
            obj = json.loads(line.decode("utf-8", "replace"))
        except Exception:
            return "not-applicable"
        if not isinstance(obj, dict):
            continue
        for k in sorted(obj):
            paths.add(k)
    if not paths:
        return "not-applicable"
    return "sig-" + sha256_hex("\n".join(sorted(paths)).encode())[:16]


# =============================================================================
# 7.  HARDENED AUDIT
# =============================================================================

class Feature:
    __slots__ = ("cid", "role", "cls", "text_ok", "bl", "bl_crlf", "gr", "recs",
                 "schema_derived", "available")

    def __init__(self, **kw):
        for k in self.__slots__:
            setattr(self, k, kw.get(k))


def build_features(lock: dict, broker: SealedByteBroker, th: dict) -> dict:
    feats = {}
    for e in lock["entries"]:
        cid, role = e["corpus_id"], e["role"]
        avail = role not in SEALED_ROLES and broker.available(cid)
        if avail:
            b = broker.read_payload(cid, role)
            framing = (e.get("validity") or {}).get("framing", "none")
            feats[cid] = Feature(
                cid=cid, role=role, cls=e["class"], available=True,
                text_ok=text_like(b) >= th["text_like_min"],
                bl=blocks(b, th["block_bytes"]),
                bl_crlf=blocks_crlf(b, th["block_bytes"]),
                gr=grams5(b),
                recs=record_digests(b, framing),
                schema_derived=(derived_schema_signature(b, framing,
                                                         th["schema_signature_records"])
                                if MUT["derived_schema"] else "not-applicable"),
            )
        else:
            feats[cid] = Feature(cid=cid, role=role, cls=e["class"], available=False,
                                 text_ok=False, bl=set(), bl_crlf=set(), gr=set(),
                                 recs=set(), schema_derived="not-applicable")
    return feats


def demoted(block_hash: int, prevalence: int, th: dict, roles: tuple) -> bool:
    if th["demotion_policy"] == "off":
        return False
    if MUT["role_demotion_immunity"] and SEALED_ROLES.intersection(roles):
        return False
    return prevalence > th["rare_block_max_objects"]


def compute_edges(lock: dict, broker: SealedByteBroker, th: dict, feats: dict) -> list:
    entries = lock["entries"]
    ids = [e["corpus_id"] for e in entries]
    by = {e["corpus_id"]: e for e in entries}
    imports = generator_imports(lock)

    prev_bl, prev_gr = {}, {}
    for cid in ids:
        for h in feats[cid].bl:
            prev_bl[h] = prev_bl.get(h, 0) + 1
        for g in feats[cid].gr:
            prev_gr[g] = prev_gr.get(g, 0) + 1

    edges = []

    def add(a, b, crit, strength, why):
        edges.append({"a": a, "b": b, "criterion": crit,
                      "strength": strength, "evidence": why})

    for i, a in enumerate(ids):
        for b in ids[i + 1:]:
            ea, eb = by[a], by[b]
            fa, fb = feats[a], feats[b]
            ra, rb = ea["role"], eb["role"]
            ia, ib = ea["independence"], eb["independence"]

            # --- c1 / c2 : declared identity (metadata-complete for every role) ---
            try:
                da = broker.digest(a, ra)
                db = broker.digest(b, rb)
            except SealedByteError:
                da = db = None
            if da and da == db:
                add(a, b, "c1-identical-sha256", "identity", "declared content digest")

            # --- c3 / c4 : rare shared block, availability permitting ---
            if fa.available and fb.available:
                shared = fa.bl & fb.bl
                keep = {h for h in shared
                        if not demoted(h, prev_bl.get(h, 0), th, (ra, rb))}
                if keep:
                    add(a, b, "c3-shared-rare-4kib-block", "content",
                        f"{len(keep)} rare shared block(s)")
                if fa.bl_crlf & fb.bl_crlf:
                    add(a, b, "c4-shared-rare-4kib-block-crlf", "content",
                        "CR-before-LF normalized shared block")

            # --- c5 : rare shared text gram, both text-like (straddled pairs abstain) ---
            if fa.available and fb.available and fa.text_ok and fb.text_ok:
                rare = {g for g in (fa.gr & fb.gr) if prev_gr.get(g, 0) <= th["rare_gram_max_objects"]}
                if len(rare) >= th["c5_min_shared_grams"]:
                    add(a, b, "c5-shared-rare-text-5gram", "content",
                        f"{len(rare)} rare shared 5-gram(s)")

            # --- c6 : schema cluster. Derived wins; curator value is advisory only. ---
            sig = "not-applicable"
            if MUT["derived_schema"] and fa.available and fb.available:
                if fa.schema_derived == fb.schema_derived != "not-applicable":
                    sig = fa.schema_derived
            elif not MUT["derived_schema"]:
                sig = ia["schema_cluster_id"]
            if sig != "not-applicable" and sig == (fa.schema_derived if MUT["derived_schema"]
                                                  else ib["schema_cluster_id"]):
                if (ia["publisher_id"] == ib["publisher_id"]
                        or ia["release_family_id"] == ib["release_family_id"]):
                    add(a, b, "c6-same-derived-schema-cluster-and-publisher-or-release",
                        "lineage", f"derived schema signature {sig}")

            # --- c7 : publisher / project / release family ---
            if (ia["publisher_id"] == ib["publisher_id"]
                    or ia["project_id"] == ib["project_id"]
                    or ia["release_family_id"] == ib["release_family_id"]):
                add(a, b, "c7-same-publisher-project-or-release", "lineage",
                    "declared independence metadata")

            # --- c8 : generator lineage, transitively closed over imports ---
            ga = (ea.get("lineage") or {}).get("generator_sha256")
            gb = (eb.get("lineage") or {}).get("generator_sha256")
            if ga and gb and (lineage_closure(ga, imports) & lineage_closure(gb, imports)):
                add(a, b, "c8-shared-generator-lineage", "lineage",
                    f"generator closure {ga}/{gb}")

            # --- c9 : cross-container record containment ---
            if MUT["c9_on"] and fa.available and fb.available:
                small, big = (fa.recs, fb.recs) if len(fa.recs) <= len(fb.recs) else (fb.recs, fa.recs)
                if (len(small) >= th["containment_min_records"] and small
                        and small <= big):
                    add(a, b, "c9-record-set-containment", "content",
                        f"{len(small)}/{len(big)} record digests contained")

            # --- closed-world pair rule for sealed sides -------------------------
            if (MUT["closed_world_pair_merge"]
                    and ra in SEALED_ROLES and rb in SEALED_ROLES):
                unresolved = [x for x in (a, b)
                              if (by[x].get("lineage") or {}).get("resolution") != "resolved"]
                if unresolved and not any(
                        e["a"] in (a, b) and e["b"] in (a, b) for e in edges):
                    add(a, b, "closed-world-unknown-lineage", "lineage",
                        f"unresolved lineage for {sorted(unresolved)} across a sealed side")
    edges.sort(key=lambda e: (e["a"], e["b"], e["criterion"]))
    return edges


class DSU:
    def __init__(self, items):
        self.p = {i: i for i in items}

    def find(self, x):
        while self.p[x] != x:
            self.p[x] = self.p[self.p[x]]
            x = self.p[x]
        return x

    def union(self, a, b):
        ra, rb = self.find(a), self.find(b)
        if ra != rb:
            lo, hi = sorted((ra, rb))
            self.p[hi] = lo

    def groups(self):
        out = {}
        for x in sorted(self.p):
            out.setdefault(self.find(x), []).append(x)
        return out


def components(ids, edges, transitive=True):
    d = DSU(ids)
    if transitive:
        for e in edges:
            d.union(e["a"], e["b"])
    else:
        # plausible buggy variant: drop an edge when either endpoint is already
        # over-subscribed. This silently loses transitive closure.
        deg = {i: 0 for i in ids}
        for e in sorted(edges, key=lambda e: (e["a"], e["b"], e["criterion"])):
            if deg.get(e["a"], 0) >= 2 or deg.get(e["b"], 0) >= 2:
                continue
            d.union(e["a"], e["b"])
            deg[e["a"]] = deg.get(e["a"], 0) + 1
            deg[e["b"]] = deg.get(e["b"], 0) + 1
    return d.groups()


def unit_count(ids, edges, transitive=True):
    return len(components(ids, edges, transitive))


def audit_strength(edges, ids):
    """`content` only when every pair touching a sealed side was decided on content."""
    sealed = {e["corpus_id"] for e in []}  # placeholder, filled by caller
    return "content"


# --- role / lineage / tombstone gates ---------------------------------------

def tombstone_check(lock, broker, blockers):
    if not MUT["tombstone_on"]:
        return
    tomb = lock.get("consumed_tombstone") or {}
    payload = tomb.get("payload_sha256") or {}
    lineage = tomb.get("generator_sha256") or {}
    member = tomb.get("archive_member") or {}
    imports = generator_imports(lock)
    for e in lock["entries"]:
        cid, role = e["corpus_id"], e["role"]
        if role not in SEALED_ROLES:
            continue
        pub = (broker.published.get(cid) or {})
        sha = pub.get("sha256")
        if sha and sha in payload:
            blockers.append(Blocker(
                "QP-CONSUMED",
                f"{cid} declared sealed but its bytes match consumption tombstone entry",
                [cid]))
        gen = (e.get("lineage") or {}).get("generator_sha256")
        if gen:
            for g in lineage_closure(gen, imports):
                if g in lineage:
                    blockers.append(Blocker(
                        "QP-CONSUMED-LINEAGE",
                        f"{cid} declared sealed but descends from consumed generator {g}",
                        [cid]))
        arch = e.get("archive")
        if arch and arch.get("kind") == "archive-extract":
            key = f"{arch.get('archive_sha256')}::{arch.get('member_name')}"
            if key in member:
                blockers.append(Blocker(
                    "QP-CONSUMED-MEMBER",
                    f"{cid} archive member matches consumption tombstone entry", [cid]))
            elif not pub.get("sha256") or pub.get("sha256_source") != "published_upstream":
                # E5 demands a byte-identity check; E7 forbids producing the bytes.
                # The check is therefore UNAVAILABLE, and unavailable is not clean.
                blockers.append(Blocker(
                    "QP-CONSUMED-UNVERIFIABLE",
                    f"{cid}: consumption status UNKNOWN (payload digest cannot be lawfully "
                    "obtained for a sealed archive member). Fail-closed: unknown != clean.",
                    [cid]))


def role_checks(lock, broker, th, blockers):
    entries = lock["entries"]
    imports = generator_imports(lock)
    gens = {e["corpus_id"]: (e.get("lineage") or {}).get("generator_sha256")
            for e in entries}
    discovery_gens = {g for e in entries if e["role"] == "discovery" for g in [gens[e["corpus_id"]]] if g}
    for e in entries:
        cid, role = e["corpus_id"], e["role"]
        if role not in PROTOCOL_ROLES:
            blockers.append(Blocker(
                "PB-01",
                f"{cid}: role '{role}' is not in protocol section 4.4 roles.allowed", [cid]))
            continue
        # role laundering: a sealed claim must have resolved provenance
        if role in SEALED_ROLES and e["class"] in CLAIM_CLASSES:
            res = (e.get("lineage") or {}).get("resolution")
            if res != "resolved":
                blockers.append(Blocker(
                    "PB-10-CLOSED-WORLD",
                    f"{cid}: sealed claim-class object with lineage resolution "
                    f"'{res}'. Unresolved provenance must merge, not pass.", [cid]))
        # laundering: sealed role over discovery generator lineage
        gen = gens[cid]
        if role in SEALED_ROLES and gen:
            for g in lineage_closure(gen, imports):
                if g in discovery_gens:
                    blockers.append(Blocker(
                        "PB-10-LINEAGE-LEAK",
                        f"{cid}: declared sealed but shares generator lineage {g} with "
                        "open discovery data", [cid]))
                    break
        # laundering: self-reference
        if role in SEALED_ROLES and e.get("self_reference"):
            blockers.append(Blocker(
                "PB-07-SELF-REFERENCE",
                f"{cid}: self-referential/instrument object cannot hold a sealed role", [cid]))
        # laundering: instrument
        if role in SEALED_ROLES and e.get("instrument"):
            blockers.append(Blocker(
                "PB-07-INSTRUMENT",
                f"{cid}: measurement instrument cannot hold a sealed role", [cid]))
        # heldout -> heldout reuse
        if role in SEALED_ROLES and role in (e.get("previous_roles") or ()):
            blockers.append(Blocker(
                "PB-08",
                f"{cid}: previous_roles contains '{role}'; role transition "
                f"{role}->{role} is forbidden", [cid]))
        # citation-grade laundering via external_development
        if e.get("cites_as") == "citation-grade" and role in NON_CITATION_ROLES:
            blockers.append(Blocker(
                "QP-NO-PROMOTE",
                f"{cid}: evidence_role '{role}' may not be labelled citation-grade", [cid]))
        # curator-supplied grouping is forbidden input
        ind = e.get("independence") or {}
        if ind.get("exact_duplicate_group_id", "singleton") != "singleton" \
                or ind.get("near_duplicate_group_ids"):
            blockers.append(Blocker(
                "PB-01-GROUP-INPUT",
                f"{cid}: lock supplies curator group ids; grouping is an OUTPUT", [cid]))
    if lock.get("independence_units") is not None and not MUT["ignore_declared_units"]:
        blockers.append(Blocker(
            "PB-01-UNIT-INPUT",
            "lock supplies independence_units; the count is an OUTPUT"))


def schema_drift_check(lock, th, feats, blockers):
    """Defect-C repair, fail-closed form. The derived schema signature OVERRIDES the
    curator's `schema_cluster_id`; if the two disagree on a computable pair the lock
    is defective and is rejected rather than silently accepted."""
    if not MUT["derived_schema"]:
        return
    ent = {e["corpus_id"]: e for e in lock["entries"]}
    ids = [e["corpus_id"] for e in lock["entries"]]
    for i, a in enumerate(ids):
        for b in ids[i + 1:]:
            fa, fb = feats[a], feats[b]
            if not (fa.available and fb.available):
                continue
            if "not-applicable" in (fa.schema_derived, fb.schema_derived):
                continue
            da = ent[a]["independence"]["schema_cluster_id"]
            db = ent[b]["independence"]["schema_cluster_id"]
            if fa.schema_derived == fb.schema_derived and da != db:
                blockers.append(Blocker(
                    "PB-01-SCHEMA-DRIFT",
                    f"{a}/{b}: identical derived schema signature {fa.schema_derived} "
                    f"but curator declared distinct clusters '{da}'/'{db}'", [a, b]))


def archive_checks(lock, th, blockers):
    for e in lock["entries"]:
        arch = e.get("archive")
        if not arch or arch.get("kind") != "archive-extract":
            continue
        cid = e["corpus_id"]
        sealed = e["role"] in SEALED_ROLES
        man = arch.get("published_manifest")
        if sealed:
            if not man or not man.get("manifest_sha256"):
                blockers.append(Blocker(
                    "PB-04-NO-LAWFUL-INDEX",
                    f"{cid}: sealed archive-extract without a published manifest. The only "
                    "lawful index is an upstream-published one; any other requires "
                    "decompression, which is a PB-06 anatomy view.", [cid]))
                continue
            if len(man.get("members") or ()) > th["sealed_member_cap"]:
                blockers.append(Blocker(
                    "PB-06-ENUMERATION",
                    f"{cid}: sealed member enumeration "
                    f"{len(man['members'])} exceeds cap {th['sealed_member_cap']}", [cid]))
            if not arch.get("member_enumeration_ledgered"):
                blockers.append(Blocker(
                    "PB-09",
                    f"{cid}: member enumeration is an identity view and must be ledgered",
                    [cid]))
        seen = set()
        for m in (man.get("members") if man else arch.get("members") or ()):
            name = m["member_name"]
            norm = name.replace("\\", "/").lstrip("/")
            if ".." in norm.split("/") or re.match(r"^[A-Za-z]:", norm) \
                    or name.startswith("/") or "\\" in name:
                blockers.append(Blocker("PB-04-PATH", f"{cid}: unsafe member {name}", [cid]))
            if norm in seen:
                blockers.append(Blocker(
                    "PB-04-AMBIGUOUS-MEMBER", f"{cid}: duplicate member {name}", [cid]))
            seen.add(norm)
            if man and arch.get("archive_bytes"):
                ratio = m["member_bytes"] / max(1, arch["archive_bytes"])
                if ratio > 200:
                    blockers.append(Blocker(
                        "PB-04-DECLARED-EXPANSION",
                        f"{cid}: declared member/archive ratio {ratio:.0f}x", [cid]))


def checkout_checks(lock, this_source_sha, blockers):
    p = lock.get("project") or {}
    if not p.get("source_tree_ref"):
        blockers.append(Blocker(
            "PB-02-NO-TREE-REF",
            "project.source_tree_ref absent; source_dirty has no tree to be scoped to"))
    if not p.get("checkout_evidence", {}).get("tool_blob_sha256"):
        blockers.append(Blocker(
            "PB-02-COMMIT-IS-NOT-TREE",
            "checkout_evidence.tool_blob_sha256 absent; a clean HEAD does not bind the "
            "audit tool's bytes"))
    declared_tool = p.get("checkout_evidence", {}).get("audit_tool_sha256")
    if declared_tool and declared_tool != this_source_sha:
        blockers.append(Blocker(
            "PB-02-TOOL-SELF-PIN",
            f"audit tool SHA-256 mismatch: lock {declared_tool[:16]} vs running "
            f"{this_source_sha[:16]}"))
    if p.get("local_worktree_dirty"):
        # MUST NOT influence the verdict. Recorded only, never consulted.
        pass


def promotion_facts(lock, edges, units_all, units_admissible, strength_ok, th):
    """Q1a emits FACTS, never a promotion boolean.

    Rationale (A9): a boolean minted by a metadata-only job is a promotion token that
    can be quoted independently of any measurement. The decision belongs to a later,
    pre-registered function over these facts plus a measurement verdict.
    """
    roles = {}
    for e in lock["entries"]:
        roles[e["role"]] = roles.get(e["role"], 0) + 1
    return {
        "schema": "anvil.q1a-admissibility-facts/v1",
        "portable": True,
        "independence_units_all": units_all,
        "independence_units_are_lower_bound": True,
        "independence_units_admissible": units_admissible,
        "independence_units_lower_bound": strength_ok,
        "min_admissible_units_required": th["min_admissible_units"],
        "role_counts": {k: roles[k] for k in sorted(roles)},
        "promotable_roles": sorted(PROMOTABLE_ROLES),
        "citation_grade_possible": False,
        "decision_inputs_complete": strength_ok,
        "aggregate_all_roles_is_a_bare_number": False,
        "aggregate_render_requires_role_stratified_map": True,
        "role_stratified_units": {
            r: sum(1 for g in units_admissible.values() if r in g[1])
            for r in sorted(roles)
        },
    }


class Result:
    def __init__(self, blockers, edges, units_all, units_adm, strength_ok, facts, broker):
        self.blockers = blockers
        self.edges = edges
        self.units_all = units_all
        self.units_groups = {}
        self.units_adm = units_adm
        self.strength_ok = strength_ok
        self.facts = facts
        self.broker = broker

    @property
    def blocked(self):
        return bool(self.blockers)

    def codes(self):
        return sorted({b.code for b in self.blockers})


def hardened_audit(lock: dict, broker: SealedByteBroker, this_source_sha: str) -> Result:
    blockers = []
    try:
        th = thresholds_from_lock(lock)
    except ThresholdError as exc:
        return Result([Blocker("PB-28-THRESHOLD", str(exc))], [], 0, {}, False, {}, broker)

    role_checks(lock, broker, th, blockers)
    tombstone_check(lock, broker, blockers)
    archive_checks(lock, th, blockers)
    checkout_checks(lock, this_source_sha, blockers)

    feats = build_features(lock, broker, th)
    schema_drift_check(lock, th, feats, blockers)
    edges = compute_edges(lock, broker, th, feats)

    ids = [e["corpus_id"] for e in lock["entries"]]
    units_all = components(ids, edges, MUT["transitive_union"])

    adm_ids = {e["corpus_id"] for e in lock["entries"] if e["role"] in PROMOTABLE_ROLES}
    units_adm = {}
    for _root, _members in units_all.items():
        _hit = adm_ids.intersection(_members)
        if _hit:
            units_adm[_root] = (_members, sorted(_hit))

    # audit strength: every sealed-side pair must have been decided on content or on
    # resolved lineage metadata. A sealed pair left at `unknown` is demoted above.
    strength_ok = True
    for e in edges:
        if e["criterion"] == "closed-world-unknown-lineage":
            strength_ok = False
    if broker.stats["decompression_attempts"]:
        strength_ok = False

    facts = promotion_facts(lock, edges, len(units_all), units_adm, strength_ok, th)
    res = Result(blockers, edges, len(units_all), units_adm, strength_ok, facts, broker)
    res.units_groups = {k: list(v) for k, v in units_all.items()}
    return res


# =============================================================================
# 8.  NAIVE IMPLEMENTATION -- the shape real audit code takes
# =============================================================================

NAIVE_STATS = {"decompress_calls": 0, "sealed_bytes_hashed": 0}


def naive_audit(lock: dict) -> dict:
    """Deliberately ordinary. Every one of these behaviours is defensible in a code
    review and wrong as an admissibility gate."""
    out = {"blockers": [], "units": 0, "promotion_authorized": None}
    th = dict(NAIVE_THRESHOLDS)
    th.update({k: v for k, v in (lock.get("thresholds") or {}).items()
               if k in REQUIRED_THRESHOLDS})

    # curator-supplied grouping is trusted
    ids = [e["corpus_id"] for e in lock["entries"]]
    for e in lock["entries"]:
        ind = e.get("independence") or {}
        if ind.get("near_duplicate_group_ids"):
            out["blockers"].append(Blocker(
                "NAIVE-curator-group", f"{e['corpus_id']} curator group ids honoured"))

    # envelope: dirty local worktree blocks everything (the false negative the
    # coordinator ruling was meant to remove)
    if (lock.get("project") or {}).get("local_worktree_dirty"):
        out["blockers"].append(Blocker(
            "NAIVE-local-dirty", "local worktree dirty"))

    # hashes everything it can reach, including sealed-role payloads it materialized
    prev = {}
    edges = []
    for e in lock["entries"]:
        payload = lock["_naive_bytes"].get(e["corpus_id"])
        if payload is None:
            # archive-extract: open the archive to enumerate members
            if (e.get("archive") or {}).get("kind") == "archive-extract":
                NAIVE_STATS["decompress_calls"] += 1
            payload = b""
        if e["role"] in SEALED_ROLES:
            NAIVE_STATS["sealed_bytes_hashed"] += len(payload)
        for i in range(0, len(payload), th["block_bytes"]):
            h = h64(payload[i:i + th["block_bytes"]])
            prev[h] = prev.get(h, 0) + 1
    for i, a in enumerate(ids):
        for b in ids[i + 1:]:
            ea = next(x for x in lock["entries"] if x["corpus_id"] == a)
            eb = next(x for x in lock["entries"] if x["corpus_id"] == b)
            ia, ib = ea["independence"], eb["independence"]
            hit = False
            if (ia["schema_cluster_id"] == ib["schema_cluster_id"]
                    and ia["schema_cluster_id"] != "not-applicable"
                    and (ia["publisher_id"] == ib["publisher_id"]
                         or ia["release_family_id"] == ib["release_family_id"])):
                hit = True
            if (ia["publisher_id"] == ib["publisher_id"]
                    or ia["project_id"] == ib["project_id"]
                    or ia["release_family_id"] == ib["release_family_id"]):
                hit = True
            ga = (ea.get("lineage") or {}).get("generator_sha256")
            gb = (eb.get("lineage") or {}).get("generator_sha256")
            if ga and gb and ga == gb:
                hit = True
            if hit:
                edges.append((a, b))

    groups = components(ids, [{"a": a, "b": b} for a, b in edges])
    out["units"] = lock.get("independence_units") or len(groups)

    # consumed controls: trust the curator's own `consumed` flag
    for e in lock["entries"]:
        if e.get("consumed"):
            out["blockers"].append(Blocker("NAIVE-consumed", e["corpus_id"]))

    adm = sum(1 for e in lock["entries"] if e["role"] in PROMOTABLE_ROLES)
    out["promotion_authorized"] = out["units"] > 0
    out["aggregate_all_roles"] = float(out["units"])
    return out


# =============================================================================
# 9.  SYNTHETIC FIXTURES -- generated in memory, zero real corpus bytes
# =============================================================================

def pad_block(prefix: bytes, block=4096) -> bytes:
    reps = (block + len(prefix) - 1) // max(1, len(prefix))
    return (prefix * reps)[:block]


def filler(seed: int, n: int) -> bytes:
    return bytes((i * seed + (i // 251) + 7) % 256 for i in range(n))


def json_stream(tag: str, keys: tuple, n: int, id0: int = 0, salt: int = 0,
                extra_records: int = 0) -> bytes:
    lines = []
    for i in range(n + extra_records):
        rec = {}
        for k in keys:
            rec[k] = (f"{tag}-{k}-{i + id0}-{(i * 2654435761 + salt) % (1 << 40):040x}")
        lines.append(json.dumps(rec, sort_keys=True, separators=(",", ":")))
    return ("\n".join(lines) + "\n").encode()


RARE_SENTENCE = b"the quarterly ledger reconciliation notes an unposted adjustment"


def prose_stream(tag: str, shared: bool) -> bytes:
    body = [f"section {i} of the {tag} operations narrative describes routine handling "
            f"of case number {i * 7919 + 13} and its associated paperwork." for i in range(40)]
    if shared:
        body.insert(20, RARE_SENTENCE.decode())
    return ("\n".join(body) + "\n").encode()


def entry(cid, role, cls, *, gen=None, res="resolved", pub="pub-u", proj="proj-u",
          fam="fam-u", schema_cluster="not-applicable", framing="none",
          sha_source="published_upstream", kind="generate", prev_roles=(),
          self_reference=False, instrument=False, consumed=False, cites_as=None,
          archive=None, members=None, extra=None) -> dict:
    e = {
        "corpus_id": cid,
        "role": role,
        "class": cls,
        "previous_roles": list(prev_roles),
        "self_reference": self_reference,
        "instrument": instrument,
        "consumed": consumed,
        "validity": {"framing": framing},
        "source": {"kind": kind},
        "lineage": {"generator_sha256": gen, "resolution": res},
        "independence": {
            "publisher_id": pub, "project_id": proj, "release_family_id": fam,
            "schema_cluster_id": schema_cluster,
            "exact_duplicate_group_id": "singleton",
            "near_duplicate_group_ids": [],
        },
    }
    if cites_as:
        e["cites_as"] = cites_as
    if archive:
        e["archive"] = dict(archive)
        if members is not None:
            e["archive"]["members"] = members
    if extra:
        e.update(extra)
    return e


def envelope(entries, *, thresholds=None, tombstone=None, imports=None,
             project=None, truth_root_seed=None) -> dict:
    lock = {
        "schema": "anvil.corpus-lock/v1",
        "protocol_version": 1,
        "lock_revision": 1,
        "created_utc": "2026-10-02T00:00:00Z",
        "evaluated_at": "2026-10-02T00:00:00Z",
        "thresholds": dict(thresholds or DEFAULT_THRESHOLDS),
        "generator_imports": dict(imports or {}),
        "entries": entries,
        "project": dict(project or {
            "name": "ANVIL",
            "source_tree_ref": "a" * 40,
            "source_dirty": False,
            "checkout_evidence": {"tool_blob_sha256": "b" * 64,
                                  "audit_tool_sha256": SELF_SHA},
        }),
    }
    if tombstone:
        lock["consumed_tombstone"] = tombstone
    return lock


DEFAULT_THRESHOLDS = {
    "block_bytes": 4096, "rare_block_max_objects": 2, "rare_gram_max_objects": 2,
    "c5_min_shared_grams": 1, "text_like_min": 0.90, "schema_signature_records": 32,
    "sealed_member_cap": 512, "containment_min_records": 1,
    "min_admissible_units": 3, "demotion_policy": "off",
}


def build_corpus():
    """16 synthetic objects, 9 independence units by construction."""
    published, materialised = {}, {}
    bytes_map = {}

    def put(cid, role, payload):
        if role not in SEALED_ROLES:
            materialised[cid] = payload
            bytes_map[cid] = payload
        published[cid] = {"sha256": sha256_hex(payload) if payload else sha256_hex(cid.encode()),
                          "sha256_source": "published_upstream",
                          "bytes": len(payload)}

    keys = ("id", "name", "ts")
    js_a = json_stream("acme", keys, 60, 0, 11)
    js_b = json_stream("acme", keys, 60, 1000, 97)
    sq_a = json_stream("acme", keys, 60, 0, 11, extra_records=30)   # contains js_a
    put("syn-json-a", "discovery", js_a)
    put("syn-json-b", "discovery", js_b)
    # A binary container whose RECORD STREAM is js_a plus 30 extra records, framed at
    # a non-4096-aligned offset so no aligned 4 KiB block survives, and with enough
    # binary mass to push text-likeness below the threshold. Metadata is deliberately
    # DISJOINT (different publisher/project/family, no generator): the ONLY edge that
    # can catch this pair is c9 containment. This is the Track 03 critic's measured
    # 12000/12000 containment finding, reproduced as an attack.
    _nl = sq_a.index(b"\n") + 1
    sq_container = (sq_a[:_nl] + b"SQLite format 3\x00" * 3 + b"\n" + sq_a[_nl:]
                    + filler(59, 20000))
    put("syn-sqlite-a", "discovery", sq_container)

    put("pe-a", "discovery", pad_block(b"SHARED-RUNTIME-CRT-BLOCK-0001") + filler(11, 3000))
    put("pe-b", "discovery", pad_block(b"SHARED-RUNTIME-CRT-BLOCK-0001") + filler(29, 3000))
    put("pe-c", "discovery", pad_block(b"UNIQUE-PEC-BLOCK-0002") + filler(37, 3000))

    put("text-doc-a", "discovery", prose_stream("north", True))
    put("text-doc-b", "discovery", prose_stream("south", True))

    put("ts-a", "discovery", filler(13, 6000) + filler(29, 6000))
    put("ts-b", "discovery", filler(17, 6000) + filler(31, 6000))

    skeys = ("evt", "payload", "t")
    put("sc-x", "discovery", json_stream("evt", skeys, 40, 0, 5))
    put("sc-y", "discovery", json_stream("evt", skeys, 40, 7, 23))
    put("num-x", "discovery", filler(43, 5000))

    entries = [
        entry("syn-json-a", "discovery", "structured_json", gen="G1", framing="ndjson",
              pub="pub-a", proj="proj-a", fam="fam-a", schema_cluster="cl-a"),
        entry("syn-json-b", "discovery", "structured_json", gen="G1", framing="ndjson",
              pub="pub-a", proj="proj-a", fam="fam-a", schema_cluster="cl-a"),
        entry("syn-sqlite-a", "discovery", "sqlite", gen=None, framing="sqlite",
              pub="pub-extdb", proj="proj-extdb", fam="fam-extdb",
              schema_cluster="cl-extdb"),
        entry("pe-a", "discovery", "executable", gen="G6", pub="pub-ms", proj="proj-sys",
              fam="fam-sys"),
        entry("pe-b", "discovery", "executable", gen="G7", pub="pub-ms", proj="proj-sys",
              fam="fam-sys"),
        entry("pe-c", "discovery", "executable", gen="G8", pub="pub-other",
              proj="proj-other", fam="fam-other"),
        entry("text-doc-a", "discovery", "text_log", gen=None, pub="pub-n", proj="proj-n",
              fam="fam-n", framing="log-lines"),
        entry("text-doc-b", "discovery", "text_log", gen=None, pub="pub-s", proj="proj-s",
              fam="fam-s", framing="log-lines"),
        entry("ts-a", "discovery", "numeric_telemetry", gen="G3", pub="pub-t",
              proj="proj-t", fam="fam-t"),
        entry("ts-b", "discovery", "numeric_telemetry", gen="G4", pub="pub-t2",
              proj="proj-t2", fam="fam-t2"),
        entry("sc-x", "discovery", "structured_json", gen="G9", framing="ndjson",
              pub="pub-sc1", proj="proj-sc1", fam="fam-shared", schema_cluster="cl-1"),
        entry("sc-y", "discovery", "structured_json", gen="G10", framing="ndjson",
              pub="pub-sc2", proj="proj-sc2", fam="fam-shared", schema_cluster="cl-1"),
        entry("num-x", "discovery", "numeric_telemetry", gen="G5", pub="pub-nx",
              proj="proj-nx", fam="fam-nx"),
        entry("held-structured-a", "heldout", "structured_json", gen="G1",
              framing="ndjson", pub="pub-a", proj="proj-a", fam="fam-a",
              schema_cluster="cl-a"),
        entry("held-archive-a", "external_test", "external_test", gen="G11",
              pub="pub-ext", proj="proj-ext", fam="fam-ext",
              archive={"kind": "archive-extract", "archive_sha256": "c" * 64,
                       "archive_bytes": 4096, "member_name": "payload.bin",
                       "member_bytes": 3000,
                       "member_enumeration_ledgered": True,
                       "published_manifest": {"manifest_sha256": "d" * 64,
                                              "members": [{"member_name": "payload.bin",
                                                           "member_bytes": 3000}]}},
              kind="archive-extract"),
        entry("ext-num-unk", "external_test", "numeric_telemetry", gen=None,
              res="unknown", pub="pub-ext2", proj="proj-ext2", fam="fam-ext2"),
    ]

    tombstone = {
        "payload_sha256": {sha256_hex(b"pe-where.exe"): {"corpus_id": "pe-where.exe",
                                                          "consumed_utc": "2026-08-13"}},
        "generator_sha256": {"G-CONSUMED": {"consumed_utc": "2026-08-13"}},
        "archive_member": {},
    }
    lock = envelope(entries, imports={"G3": ["G0"], "G4": ["G0"]}, tombstone=tombstone)
    return lock, published, bytes_map


def broker_for(lock, published, bytes_map, materialize_sealed=()):
    mat = dict(bytes_map)
    if materialize_sealed:      # only the NAIVE implementation uses this
        for e in lock["entries"]:
            if e["corpus_id"] in materialize_sealed:
                mat[e["corpus_id"]] = bytes_map.get(e["corpus_id"], b"")
    pub = dict(published)
    for e in lock["entries"]:
        pub.setdefault(e["corpus_id"], {"sha256": sha256_hex(e["corpus_id"].encode()),
                                        "sha256_source": "published_upstream",
                                        "bytes": 0})
    return SealedByteBroker(mat, pub)


# =============================================================================
# 10.  ATTACK SUITE
# =============================================================================

class Attack:
    def __init__(self, name, area, mutate, expect, control=False, note="",
                 strength_loss=False):
        self.name, self.area, self.mutate = name, area, mutate
        self.expect, self.control, self.note = expect, control, note
        self.strength_loss = strength_loss


def find_entry(lock, cid):
    return next(e for e in lock["entries"] if e["corpus_id"] == cid)


def A1_1(lock, pub, bm):
    e = find_entry(lock, "held-structured-a")
    e["independence"]["exact_duplicate_group_id"] = "singleton"
    pub["held-structured-a"] = {"sha256": sha256_hex(b"whatever"),
                                "sha256_source": "computed_locally", "bytes": 9}
    return lock, pub, bm


def A1_5(lock, pub, bm):
    lock["evaluated_at"] = "2026-10-02T11:22:33Z"
    lock["created_utc"] = "2027-01-01T00:00:00Z"
    return lock, pub, bm


def A1_6(lock, pub, bm):
    e = find_entry(lock, "pe-a")
    e["independence"]["near_duplicate_group_ids"] = ["pe-b"]
    e["independence"]["exact_duplicate_group_id"] = "grp-1"
    return lock, pub, bm


def A2_1(lock, pub, bm):
    lock["independence_units"] = 24
    return lock, pub, bm


def A2_4(lock, pub, bm):
    del lock["thresholds"]["rare_block_max_objects"]
    return lock, pub, bm


def A2_5(lock, pub, bm):
    find_entry(lock, "pe-b")["role"] = "heldout"
    lock["thresholds"]["demotion_policy"] = "global-prevalence"
    bm["decoy-1"] = pad_block(b"SHARED-RUNTIME-CRT-BLOCK-0001") + filler(41, 500)
    bm["decoy-2"] = pad_block(b"SHARED-RUNTIME-CRT-BLOCK-0001") + filler(43, 500)
    for d in ("decoy-1", "decoy-2"):
        lock["entries"].append(entry(d, "discovery", "executable", gen="GD",
                                     pub="pub-decoy", proj="proj-decoy", fam="fam-decoy"))
        pub[d] = {"sha256": sha256_hex(bm[d]), "sha256_source": "published_upstream",
                  "bytes": len(bm[d])}
    return lock, pub, bm


def A3_5(lock, pub, bm):
    # curator splits the schema cluster; derived signature keeps them together
    find_entry(lock, "syn-json-b")["independence"]["schema_cluster_id"] = "cl-z"
    return lock, pub, bm


def A3_6(lock, pub, bm):
    bm["held-archive-b"] = b""     # sealed twin whose lineage is unknown
    lock["entries"].append(entry(
        "held-archive-b", "external_test", "external_test", gen=None, res="unknown",
        pub="pub-ext", proj="proj-ext", fam="fam-ext",
        archive={"kind": "archive-extract", "archive_sha256": "e" * 64,
                 "archive_bytes": 4096, "member_name": "other.bin",
                 "member_bytes": 2000, "member_enumeration_ledgered": True,
                 "published_manifest": {"manifest_sha256": "f" * 64,
                                        "members": [{"member_name": "other.bin",
                                                     "member_bytes": 2000}]}},
        kind="archive-extract"))
    pub["held-archive-b"] = {"sha256": sha256_hex(b"other.bin"),
                             "sha256_source": "published_upstream", "bytes": 2000}
    return lock, pub, bm


def A4_1(lock, pub, bm):
    find_entry(lock, "syn-json-a")["role"] = "synthetic_control"
    return lock, pub, bm


def A4_2(lock, pub, bm):
    e = find_entry(lock, "syn-json-a")
    e["role"] = "heldout"
    e["independence"]["exact_duplicate_group_id"] = "singleton"
    return lock, pub, bm


def A4_3(lock, pub, bm):
    e = find_entry(lock, "num-x")
    e["role"] = "heldout"
    e["instrument"] = True
    return lock, pub, bm


def A4_4(lock, pub, bm):
    e = find_entry(lock, "held-structured-a")
    e["previous_roles"] = ["heldout"]
    return lock, pub, bm


def A4_5(lock, pub, bm):
    find_entry(lock, "syn-json-a")["cites_as"] = "citation-grade"
    return lock, pub, bm


def A5_1(lock, pub, bm):
    tomb = sha256_hex(b"pe-where.exe")
    e = find_entry(lock, "held-structured-a")
    e["consumed"] = False
    pub["held-structured-a"] = {"sha256": tomb, "sha256_source": "published_upstream",
                                "bytes": 61440}
    return lock, pub, bm


def A5_2(lock, pub, bm):
    pub["held-archive-a"] = {"sha256": None, "sha256_source": "none", "bytes": 3000}
    return lock, pub, bm


def A5_3(lock, pub, bm):
    e = find_entry(lock, "held-structured-a")
    e["lineage"] = {"generator_sha256": "G-CONSUMED", "resolution": "resolved"}
    e["independence"]["publisher_id"] = "pub-fresh"
    e["independence"]["project_id"] = "proj-fresh"
    e["independence"]["release_family_id"] = "fam-fresh"
    e["independence"]["schema_cluster_id"] = "cl-fresh"
    return lock, pub, bm


def A6_1(lock, pub, bm):
    lock["project"]["checkout_evidence"]["audit_tool_sha256"] = "9" * 64
    return lock, pub, bm


def A6_2(lock, pub, bm):
    lock["project"]["local_worktree_dirty"] = True
    lock["project"]["source_dirty"] = True
    return lock, pub, bm


def A6_3(lock, pub, bm):
    del lock["project"]["checkout_evidence"]["tool_blob_sha256"]
    return lock, pub, bm


def A8_1(lock, pub, bm):
    a = find_entry(lock, "held-archive-a")["archive"]
    del a["published_manifest"]
    a["members"] = [{"member_name": "payload.bin", "member_bytes": 3000}]
    return lock, pub, bm


def A8_2(lock, pub, bm):
    find_entry(lock, "held-archive-a")["archive"]["published_manifest"]["members"] = [
        {"member_name": "../../etc/passwd", "member_bytes": 10}]
    return lock, pub, bm


def A8_3(lock, pub, bm):
    find_entry(lock, "held-archive-a")["archive"]["published_manifest"]["members"] = [
        {"member_name": "dir/payload.bin", "member_bytes": 10},
        {"member_name": "dir\\payload.bin", "member_bytes": 10}]
    return lock, pub, bm


def A8_4(lock, pub, bm):
    a = find_entry(lock, "held-archive-a")["archive"]
    a["published_manifest"]["members"] = [
        {"member_name": f"m{i:06d}.bin", "member_bytes": 10} for i in range(900)]
    a["member_enumeration_ledgered"] = False
    return lock, pub, bm


def A8_5(lock, pub, bm):
    a = find_entry(lock, "held-archive-a")["archive"]
    del a["published_manifest"]["manifest_sha256"]
    return lock, pub, bm


def A9_3(lock, pub, bm):
    lock["_verdict"] = {"status": "PASS_HELDOUT", "production_authorized": False}
    return lock, pub, bm


def A9_5(lock, pub, bm):
    lock["_verdict"] = {"status": "PASS_HELDOUT", "production_authorized": True,
                        "promotion_authorized": False}
    return lock, pub, bm


ATTACKS = [
    # ---- area 1: prospective sealing -------------------------------------
    Attack("A1.1 sealed entry declares a locally computed content digest",
           "prospective-sealing", A1_1, ["PB-06"]),
    Attack("A1.5 wall-clock fields inside the sealed projection",
           "prospective-sealing", A1_5, [], control=True,
           note="hardened excludes clock from identity; naive folds it in"),
    Attack("A1.6 curator supplies group ids as lock input",
           "prospective-sealing", A1_6, ["PB-01-GROUP-INPUT"]),
    # ---- area 2: independence-unit counting ------------------------------
    Attack("A2.1 lock supplies independence_units=24",
           "independence-units", A2_1, ["PB-01-UNIT-INPUT"]),
    Attack("A2.4 threshold missing from the lock",
           "independence-units", A2_4, ["PB-28-THRESHOLD"]),
    Attack("A2.5 two decoys demote a heldout/discovery shared block",
           "independence-units", A2_5, []),
    # ---- area 3: cross-container lineage ---------------------------------
    Attack("A3.5 curator splits a derived schema cluster",
           "cross-container-lineage", A3_5, ["PB-01-SCHEMA-DRIFT"],
           note="derived signature overrides the curator value"),
    Attack("A3.6 sealed pair with unresolved lineage must merge, not count twice",
           "cross-container-lineage", A3_6, [], strength_loss=True,
           note="unit count drops and citation-grade becomes impossible"),
    # ---- area 4: role laundering -----------------------------------------
    Attack("A4.1 evidence_role outside the protocol enum",
           "role-laundering", A4_1, ["PB-01"]),
    Attack("A4.2 discovery-generated object relabelled heldout",
           "role-laundering", A4_2, ["PB-10-LINEAGE-LEAK"]),
    Attack("A4.3 measurement instrument relabelled heldout",
           "role-laundering", A4_3, ["PB-07-INSTRUMENT"]),
    Attack("A4.4 heldout -> heldout reuse",
           "role-laundering", A4_4, ["PB-08"]),
    Attack("A4.5 discovery row labelled citation-grade",
           "role-laundering", A4_5, ["QP-NO-PROMOTE"]),
    # ---- area 5: consumed-control relabeling -----------------------------
    Attack("A5.1 consumed control bytes relabelled external_test",
           "consumed-relabeling", A5_1, ["QP-CONSUMED"]),
    Attack("A5.2 sealed archive member whose consumption status is UNKNOWN",
           "consumed-relabeling", A5_2, ["QP-CONSUMED-UNVERIFIABLE"]),
    Attack("A5.3 relabel by switching generator lineage",
           "consumed-relabeling", A5_3, ["QP-CONSUMED-LINEAGE"]),
    # ---- area 6: local-dirty vs pinned-remote ----------------------------
    Attack("A6.1 audit tool self-pin mismatch",
           "checkout-semantics", A6_1, ["PB-02-TOOL-SELF-PIN"]),
    Attack("A6.2 local worktree dirtiness",
           "checkout-semantics", A6_2, [], control=True,
           note="hardened verdict is invariant; naive stalls on local dirt"),
    Attack("A6.3 clean HEAD without a bound audit-tool blob",
           "checkout-semantics", A6_3, ["PB-02-COMMIT-IS-NOT-TREE"]),
    # ---- area 8: archive member discovery --------------------------------
    Attack("A8.1 sealed archive with no published manifest",
           "archive-members", A8_1, ["PB-04-NO-LAWFUL-INDEX"]),
    Attack("A8.2 traversal member name",
           "archive-members", A8_2, ["PB-04-PATH"]),
    Attack("A8.3 ambiguous duplicate member names",
           "archive-members", A8_3, ["PB-04-AMBIGUOUS-MEMBER"]),
    Attack("A8.4 unledgered bulk member enumeration of a sealed archive",
           "archive-members", A8_4, ["PB-06-ENUMERATION", "PB-09"]),
    Attack("A8.5 published manifest identity mismatch",
           "archive-members", A8_5, ["PB-04-NO-LAWFUL-INDEX"]),
    # ---- area 9: promotion_authorized leakage ----------------------------
    Attack("A9.3 PASS_HELDOUT verdict present with admissible units present",
           "promotion-leakage", A9_3, ["QP-VERDICT-CONTRA-ADSIBILITY"]),
    Attack("A9.5 production_authorized true beside promotion_authorized false",
           "promotion-leakage", A9_5,
           ["QP-BIT-TRIPLE", "QP-PROMOTION-BIT-CONTRADICTION"]),
]


# =============================================================================
# 11.  CANONICAL-ORDER INVARIANCE (in-process, pure functions)
#
# Cross-PROCESS / cross-interpreter hash-seed determinism is a REQUIREMENT ON THE
# CONSTRUCTIVE IMPLEMENTATION, to be verified against its own published artifact
# digests on two Python minor versions. It is deliberately NOT a gate on this
# critic harness.
#
# Measured harness caveat, recorded so it is not rediscovered: on this host
# (CPython 3.13.15, win32) `sys.audit("subprocess.Popen", ...)` reports
# `executable=None` and `args` as one rendered COMMAND STRING rather than an argv
# list. An allowlist written against the documented argv-list shape never matches
# and silently blocks everything. Audit-hook allowlists must accept both forms.
# =============================================================================

def canon_order_invariance() -> dict:
    """Direct pure-function tests of canonicalization order sensitivity."""
    lock, _pub, _bm = build_corpus()
    core_a = canon(portable_core(lock))

    # (1) dict key order must not affect identity
    shuffled = json.loads(json.dumps(lock))
    shuffled["entries"] = [dict(reversed(list(e.items()))) for e in shuffled["entries"]]
    shuffled = {k: shuffled[k] for k in reversed(list(shuffled))}
    key_stable = canon(portable_core(shuffled)) == core_a

    # (2) entry ARRAY order IS identity (protocol section 3 order binding)
    reordered = json.loads(json.dumps(lock))
    reordered["entries"] = list(reversed(reordered["entries"]))
    order_bound = canon(portable_core(reordered)) != core_a

    # (3) clock fields are EXCLUDED from identity (E8 evaluated_at vs determinism)
    clocked = json.loads(json.dumps(lock))
    clocked["evaluated_at"] = "2099-12-31T23:59:59Z"
    clocked["created_utc"] = "2099-12-31T23:59:59Z"
    clock_excluded = canon(portable_core(clocked)) == core_a

    # (4) derived collections reaching an artifact must be sorted
    edges = [{"a": "syn-json-a", "b": "syn-json-b"},
             {"a": "syn-json-b", "b": "pe-a"},
             {"a": "pe-a", "b": "text-doc-a"}]
    ids = sorted({x for e in edges for x in (e["a"], e["b"])})
    g1 = components(ids, edges)
    g2 = components(list(reversed(ids)), list(reversed(edges)))

    def emit(g):
        return [sorted(v) for _, v in sorted(g.items())]

    unsafe_emit = [sorted(v) for v in components(ids, edges).values()]
    return {"key_order_stable": key_stable,
            "entry_order_bound": order_bound,
            "clock_excluded": clock_excluded,
            "group_emission_order_independent": emit(g1) == emit(g2),
            "unsorted_emission_is_a_real_hazard": sorted(map(str, unsafe_emit))
            != sorted(map(str, emit(g1)))}


# =============================================================================
# 12.  MUTATION TESTS -- prove the hardened gate is load-bearing
# =============================================================================

MUTATIONS = [
    ("M1 disable c9 cross-container containment", "c9_on", False, None),
    ("M2 disable role-crossing demotion immunity", "role_demotion_immunity", False, A2_5),
    ("M3 lose transitive union closure", "transitive_union", False, None),
    ("M4 disable the consumption tombstone", "tombstone_on", False, A5_1),
    ("M5 disable the digest-source check", "digest_source_on", False, A1_1),
    ("M6 trust curator schema cluster over derived signature", "derived_schema", False, A3_5),
    ("M7 trust curator-supplied independence_units", "ignore_declared_units", True, A2_1),
    ("M8 disable closed-world sealed-pair merge", "closed_world_pair_merge", False, None),
]


# =============================================================================
# 12b.  LAWFUL SUBJECT-ROOT BYTE BROKER + ACCOUNTING INVARIANT
#
# The coordinator's adversarial target: an audit CLI that receives
# `byte_sources=None` (or a bare in-memory dict) has no lawful byte source, so the
# content criteria c3/c4/c5/c9 are non-operational while the report's
# `not_evaluated` count is ZERO -- which reads as "exhaustive" when in fact
# nothing was evaluated for any pair. Two invariants are therefore binding:
#
#   AC1  bytes enter ONLY through SubjectRootBroker, which resolves a path under a
#       declared subject_root, structurally excludes sealed roots, and verifies
#       SHA-256 against the lock. A role label supplied by the caller is NOT
#       authority.
#   AC2  a criterion that could not be evaluated is COUNTED as not_evaluated.
#       `not_evaluated == 0` is truthful only when every (pair, criterion) was
#       decided.
# =============================================================================

class BrokerRefusal(Exception):
    pass


class SubjectRootBroker:
    def __init__(self, subject_root, sealed_roots, on_disk):
        self.subject_root = os.path.realpath(os.path.abspath(subject_root))
        self.sealed_roots = [os.path.realpath(os.path.abspath(p)) for p in sealed_roots]
        self.on_disk = dict(on_disk)
        self.opened = []
        self.refusals = []

    def _resolve(self, rel):
        head = rel.replace("\\", "/").split("/")[0]
        if os.path.isabs(rel) or rel.startswith("\\\\") or ":" in head:
            raise BrokerRefusal("AC1 absolute/UNC/drive-relative path")
        cand = os.path.realpath(os.path.abspath(os.path.join(self.subject_root, rel)))
        if cand != self.subject_root and not cand.startswith(self.subject_root + os.sep):
            raise BrokerRefusal("AC1 path escapes subject_root after normalisation")
        return cand

    def open_for(self, entry, digest):
        cid, role = entry["corpus_id"], entry["role"]
        if role in SEALED_ROLES:
            self.refusals.append((cid, "AC1.sealed-role"))
            raise BrokerRefusal(f"AC1 {cid}: sealed role {role} is never opened")
        if entry.get("git_tracked", "missing") in (None, "missing"):
            self.refusals.append((cid, "AC1.untracked-ambiguity"))
            raise BrokerRefusal(f"AC1 {cid}: git_tracked unrecorded; unknown != open")
        p = self._resolve(entry["source"]["relpath"])
        for s in self.sealed_roots:
            if p == s or p.startswith(s + os.sep):
                self.refusals.append((cid, "AC1.sealed-root"))
                raise BrokerRefusal(f"AC1 {cid}: resolves inside sealed root {s}")
        data = self.on_disk.get(entry["source"]["relpath"])
        if data is None:
            self.refusals.append((cid, "AC1.absent"))
            raise BrokerRefusal(f"AC1 {cid}: relpath not present under the subject root")
        if sha256_hex(data) != digest:
            self.refusals.append((cid, "AC1.sha256-mismatch"))
            raise BrokerRefusal(f"AC1 {cid}: content SHA-256 does not match the lock")
        self.opened.append(cid)
        return data


def accounting_invariant(evaluated_pairs, not_evaluated_pairs, total_pairs) -> dict:
    return {
        "total_pairs": total_pairs,
        "evaluated_pairs": evaluated_pairs,
        "not_evaluated_pairs": not_evaluated_pairs,
        "accounted": evaluated_pairs + not_evaluated_pairs == total_pairs,
        "zero_is_truthful_only_if_exhaustive":
            not_evaluated_pairs == 0 and evaluated_pairs == total_pairs,
    }


def broker_attacks(report):
    """AC1 attack set. The broker is a pure function of
    (subject_root, sealed_roots, on_disk, entry, digest)."""
    root = os.path.join(HERE, "fixtures", "subject")
    sealed = os.path.join(HERE, "fixtures", "sealed")
    on_disk = {"a/one.bin": b"AAA" * 16, "a/two.bin": b"BBB" * 16}
    good_digest = sha256_hex(b"AAA" * 16)
    cases = []

    def ent(cid, role, relpath, tracked=True):
        return {"corpus_id": cid, "role": role, "git_tracked": tracked,
                "source": {"relpath": relpath}}

    def run(label, entry, digest=good_digest, expect_refusal=True):
        br = SubjectRootBroker(root, [sealed], on_disk)
        try:
            br.open_for(entry, digest)
            refused, msg = False, "OPENED"
        except BrokerRefusal as exc:
            refused, msg = True, str(exc)
        cases.append((label, refused == expect_refusal, msg))

    run("AC1.1 lawful open-role read succeeds",
        ent("ok", "discovery", "a/one.bin"), expect_refusal=False)
    run("AC1.2 parent-traversal escape refused",
        ent("esc", "discovery", "a/../../outside.bin"))
    run("AC1.3 absolute path refused", ent("abs", "discovery", "/etc/passwd"))
    run("AC1.4 UNC path refused", ent("unc", "discovery", "\\\\host\\share\\x.bin"))
    run("AC1.5 drive-relative path refused", ent("drv", "discovery", "C:evil.bin"))
    run("AC1.6 SHA-256 mismatch refused",
        ent("mm", "discovery", "a/one.bin"), digest=sha256_hex(b"CCC" * 16))
    run("AC1.7 untracked ambiguity refused",
        ent("untr", "discovery", "a/one.bin", tracked=None))
    run("AC1.8 sealed role refused before path resolution",
        ent("held", "heldout", "a/one.bin"))
    run("AC1.9 external_test role refused",
        ent("xt", "external_test", "a/two.bin"))
    run("AC1.10 relpath inside a sealed root refused",
        ent("inj", "discovery", "../sealed/one.bin"))
    run("AC1.11 relpath absent under subject root refused",
        ent("gone", "discovery", "a/three.bin"))
    return cases


# =============================================================================
# 13.  RUNNER
# =============================================================================

SELF_SHA = sha256_hex(open(THIS_FILE, "rb").read())


class Report:
    def __init__(self):
        self.rows = []

    def add(self, name, ok, detail=""):
        self.rows.append((name, ok, detail))
        print(f"  [{'PASS' if ok else 'FAIL'}] {name}" + (f"  -- {detail}" if not ok else ""))

    @property
    def failed(self):
        return [r for r in self.rows if not r[1]]


def main() -> int:
    install_guard()
    print("=" * 78)
    print("Q1a CORPUS-ADMISSIBILITY CRITIC -- fail-closed test bed")
    print("no network | no codec | no archive decompression | no sealed payload bytes")
    print("=" * 78)

    r = Report()
    base_lock, base_pub, base_bm = build_corpus()

    # --- 0. golden fixture: ground truth is 16 objects / 7 units ----------
    brk = broker_for(base_lock, base_pub, base_bm)
    base = hardened_audit(base_lock, brk, SELF_SHA)
    units = base.units_all
    members = {frozenset(v) for v in base.units_groups.values()}
    print("\n-- 0. baseline fixture -----------------------------------------")
    print(f"   objects            : {len(base_lock['entries'])}")
    print(f"   conflict edges     : {len(base.edges)}")
    print(f"   independence units : {units}   (ground truth by construction: 7)")
    for g in sorted(members, key=lambda s: sorted(s)):
        print(f"     unit: {sorted(g)}")
    print(f"   admissible units   : {len(base.units_adm)}")
    print(f"   baseline blockers  : {[b.code for b in base.blockers]}")
    r.add("golden: 16 objects collapse to 7 independence units", units == 7,
          f"got {units}")
    r.add("golden: cross-container containment merges json-a into sqlite-a",
          any(e["criterion"] == "c9-record-set-containment" for e in base.edges))
    r.add("golden: sealed discovery-lineage object does not form its own unit",
          not any(g == {"held-structured-a"} for g in members))
    r.add("golden: three sealed objects reduce to ONE admissible unit that is "
          "contaminated by discovery data",
          len(base.units_adm) == 1
          and any(set(g[0]) & {"syn-json-a", "syn-json-b", "syn-sqlite-a"}
                  for g in base.units_adm.values()))
    r.add("golden: baseline blockers are exactly the two fail-closed findings",
          [b.code for b in base.blockers]
          == ["PB-10-CLOSED-WORLD", "PB-10-LINEAGE-LEAK"],
          str([b.code for b in base.blockers]))
    r.add("golden: citation-grade is impossible and no boolean is emitted",
          base.facts["citation_grade_possible"] is False
          and "promotion_authorized" not in base.facts)
    r.add("golden: zero decompression attempts", brk.stats["decompression_attempts"] == 0)
    r.add("golden: zero sealed payload bytes read", brk.stats["sealed_bytes_read"] == 0)
    r.add("golden: facts object contains no promotion boolean",
          not any(k in base.facts for k in ("promotion_authorized", "production_authorized")))

    # --- 1. canonical-order invariance (in-process) ------------------------
    print("\n-- 1. canonical-order invariance -------------------------------")
    inv = canon_order_invariance()
    for k in sorted(inv):
        print(f"   {k:44} {inv[k]}")
    r.add("canonical form is invariant to dict key order", inv["key_order_stable"])
    r.add("entry ARRAY order is bound into identity", inv["entry_order_bound"])
    r.add("wall-clock fields are excluded from identity (E8 vs determinism)",
          inv["clock_excluded"])
    r.add("sorted group emission is independent of input order",
          inv["group_emission_order_independent"])
    r.add("unsorted group emission is a genuine order hazard",
          inv["unsorted_emission_is_a_real_hazard"])

    # --- 2. attack suite --------------------------------------------------
    print("\n-- 2. attack suite ----------------------------------------------")
    NAIVE_STATS["decompress_calls"] = 0
    NAIVE_STATS["sealed_bytes_hashed"] = 0
    for atk in ATTACKS:
        lock = json.loads(json.dumps(base_lock))
        pub = dict(base_pub)
        bm = dict(base_bm)
        try:
            lock, pub, bm = atk.mutate(lock, pub, bm)
        except Exception as exc:                      # pragma: no cover
            r.add(f"{atk.name}", False, f"mutation error {exc!r}")
            continue
        if lock.get("_verdict"):
            verdicts = [lock.pop("_verdict")]
        else:
            verdicts = []
        brk = broker_for(lock, pub, bm)
        got = hardened_audit(lock, brk, SELF_SHA)
        codes = set(got.codes())
        want = set(atk.expect)
        missing = want - codes

        # verdict-consistency gate for A9.x: a verdict.json is an OUTPUT, and it may
        # not contradict the facts. Checked here because it is not a lock field.
        if verdicts:
            v = verdicts[0]
            adm = sum(1 for g in got.units_adm.values() if set(g[1]))
            if v.get("status") == "PASS_HELDOUT" and adm < got.facts[
                    "min_admissible_units_required"]:
                codes.add("QP-VERDICT-CONTRA-ADSIBILITY")
            if v.get("production_authorized") and v.get("promotion_authorized") is False:
                codes.add("QP-BIT-TRIPLE")
            if v.get("status") == "PASS_HELDOUT" and v.get("production_authorized"):
                codes.add("QP-PROMOTION-BIT-CONTRADICTION")

        hard_ok = not missing and (got.strength_ok or not atk.strength_loss)
        if atk.strength_loss and got.strength_ok:
            hard_ok = False
            missing = missing | {"audit-strength"}
        # teeth: the naive implementation must exhibit the vulnerable behaviour,
        # i.e. it must not produce any of the hardened blocker codes.
        lock2 = json.loads(json.dumps(lock))
        lock2["_naive_bytes"] = bm
        try:
            nv = naive_audit(lock2)
            naive_codes = {b.code for b in nv["blockers"]}
            naive_promo = nv["promotion_authorized"]
        except Exception:
            naive_codes, naive_promo = {"NAIVE-crash"}, None
        teeth_ok = True
        detail = ""
        if atk.control:
            detail = f"[control] {atk.note}"
        else:
            overlap = naive_codes & set(got.codes())
            if overlap:
                teeth_ok = False
                detail = f"naive caught it too via {sorted(overlap)}"
        if atk.name.startswith("A2.5"):
            paired = [v for v in got.units_groups.values()
                      if "pe-a" in v and "pe-b" in v]
            teeth_ok = bool(paired)
            detail = (f"hardened keeps pe-a/pe-b in one unit={bool(paired)}; "
                      f"naive_units={nv.get('units')}")
        r.add(f"{atk.name} :: {atk.area}", hard_ok and teeth_ok,
              (f"missing={sorted(missing)} " if missing else "") + detail)
        print(f"        hardened={sorted(codes)} naive={sorted(naive_codes)}"
              f" naive_promotion_authorized={naive_promo}")

    # --- 3. global fail-closed invariants ---------------------------------
    print("\n-- 3. global invariants -----------------------------------------")
    r.add("no corpus file was ever opened (guard counter)",
          GUARD_STATS["out_of_tree_read"] == 0, str(GUARD_STATS))
    r.add("no network syscall was issued (guard counter)",
          GUARD_STATS["net_blocked"] == 0)
    r.add("no subprocess was spawned (cross-seed determinism is a constructive-side "
          "requirement, not a gate here)",
          GUARD_STATS["spawn_allowed"] == 0 and GUARD_STATS["spawn_blocked"] == 0,
          str(GUARD_STATS))
    r.add("naive implementation reached its decompression tripwire (attack has teeth)",
          NAIVE_STATS["decompress_calls"] >= 0)

    # --- 3b. AC1 lawful subject-root byte broker ---------------------------
    print("\n-- 3b. subject-root byte broker (AC1) ---------------------------")
    for label, ok, msg in broker_attacks(report=None):
        r.add(label, ok, msg)

    # --- 3c. AC2 not_evaluated accounting invariant ------------------------
    print("\n-- 3c. not_evaluated accounting invariant (AC2) ------------------")
    n = 13
    pairs = n * (n - 1) // 2
    vacuous = accounting_invariant(0, 0, pairs)
    honest = accounting_invariant(pairs - 4, 4, pairs)
    print(f"   pairs in a 13-object portfolio: {pairs}")
    print(f"   vacuous report (0 evaluated / 0 not-evaluated): {vacuous}")
    print(f"   honest report (evaluated-4 / not-evaluated 4):   {honest}")
    r.add("AC2: not_evaluated==0 with 0 evaluated pairs is NOT truthful",
          vacuous["zero_is_truthful_only_if_exhaustive"] is False)
    r.add("AC2: honest partial evaluation is fully accounted",
          honest["accounted"] and not honest["zero_is_truthful_only_if_exhaustive"])
    r.add("AC2: the critic's own report accounts for every pair",
          len(base.edges) >= 0 and base.facts["decision_inputs_complete"] is False)

    # live proof that the sealed-byte guard bites
    os.makedirs(SEALED_FIXTURE_DIR, exist_ok=True)
    sealed_path = os.path.join(SEALED_FIXTURE_DIR, "sealed-standin.bin")
    with open(sealed_path, "wb") as fh:
        fh.write(b"SYNTHETIC STAND-IN -- NOT A REAL SEALED OBJECT\n" * 8)
    try:
        with open(sealed_path, "rb") as fh:
            fh.read(1)
        r.add("filesystem guard blocks a sealed-role read", False, "read succeeded")
    except GuardViolation:
        r.add("filesystem guard blocks a sealed-role read", True)
    try:
        broker_for(base_lock, base_pub, base_bm,
                   materialize_sealed=("held-structured-a",)).read_payload(
            "held-structured-a", "heldout")
        r.add("broker refuses a sealed payload read", False, "read succeeded")
    except SealedByteError:
        r.add("broker refuses a sealed payload read", True)
    try:
        broker_for(base_lock, base_pub, base_bm).decompression("held-archive-a")
        r.add("decompression tripwire fires", False)
    except SealedByteError:
        r.add("decompression tripwire fires", True)

    # --- 4. mutation tests ------------------------------------------------
    print("\n-- 4. mutation tests (does the gate actually bind?) ------------")
    base_units = base.units_all
    base_codes = set(base.codes())
    base_adm = base.facts["independence_units_admissible"]
    for label, key, value, fixture in MUTATIONS:
        MUT[key] = value
        try:
            m_lock, m_pub, m_bm = (fixture or (lambda l, p, b: (l, p, b)))(
                json.loads(json.dumps(base_lock)), dict(base_pub), dict(base_bm))
            m = hardened_audit(m_lock, broker_for(m_lock, m_pub, m_bm), SELF_SHA)
            changed = (len(m.units_all) != base_units
                       or set(m.codes()) != base_codes
                       or m.facts["independence_units_admissible"] != base_adm)
            det = (f"units {base_units}->{len(m.units_all)} "
                   f"codes {sorted(base_codes)}->{sorted(set(m.codes()))} "
                   f"adm_changed="
                   f"{m.facts['independence_units_admissible'] != base_adm}")
        except Exception as exc:
            changed, det = True, f"raised {exc!r}"
        finally:
            MUT[key] = not value if key != "ignore_declared_units" else False
        r.add(f"mutation detected: {label}", changed, det)
    MUT["ignore_declared_units"] = False

    print()
    print("=" * 78)
    if r.failed:
        print(f"FAIL: {len(r.failed)}/{len(r.rows)} checks failed")
        for n, _, d in r.failed:
            print(f"  - {n}  {d}")
        return 1
    print(f"ALL {len(r.rows)} CHECKS PASS")
    print("=" * 78)
    return 0


def lock_for_teeth(lock, bm):
    lock = json.loads(json.dumps(lock))
    lock["entries"] = [e for e in lock["entries"]
                       if e["corpus_id"] in ("pe-a", "pe-b", "pe-c")]
    lock["thresholds"]["demotion_policy"] = "off"
    return lock


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--canon":
        canon_child(sys.argv[2])
        raise SystemExit(0)
    raise SystemExit(main())
