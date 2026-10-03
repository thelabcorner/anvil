#!/usr/bin/env python3
"""Adversarial probe of the CURRENT on-disk Q1a constructive prototype.

Records the exact source digest at import time, then attacks the substantive
requirements. Read-only w.r.t. the constructive tree; writes nothing outside this
directory. No network, no codec, no archive decompression, no real corpus bytes.
"""
import hashlib
import importlib.util
import json
import os
import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
SRC = (HERE.parent.parent / "q1a-corpus-lock" / "space-bunny"
       / "q1a_corpus_lock.py")

FILE_SHA = hashlib.sha256(SRC.read_bytes()).hexdigest()
spec = importlib.util.spec_from_file_location("q1a", SRC)
q = importlib.util.module_from_spec(spec)
sys.modules["q1a"] = q
spec.loader.exec_module(q)

print("source file sha256      :", FILE_SHA)
print("source bytes / lines    :", SRC.stat().st_size, len(SRC.read_text().splitlines()))
print("module _tool_source_sha :", q._tool_source_sha256())
print("audit() signature       : byte_sources default =",
      q.audit.__defaults__)

LOCK = q.fixture_lock()
TOMB = q.fixture_tombstone()
SRC_TEXT = q._read_own_source()
SEALED = q.SEALED_ROLES
results = []


def check(name, ok, detail=""):
    results.append((name, bool(ok), detail))
    print(f"[{'PASS' if ok else 'FAIL'}] {name}" + (f"  -- {detail}" if detail else ""))


def fresh(**kw):
    return q.fixture_lock(**kw)


# ---------------------------------------------------------------- T1
print("\n== T1 sealed-byte fail-closed ==")
g = q.ExposureGuard()
held = [e for e in LOCK["entries"] if e["role"] in SEALED]
check("fixture contains sealed entries", bool(held),
      f"{[e['corpus_id'] for e in held]}")
try:
    g.open_bytes(held[0]["corpus_id"], held[0]["role"], b"SECRETPAYLOAD")
    check("guard.open_bytes on a sealed role raises", False, "bytes returned")
except q.ExposureBreach as exc:
    check("guard.open_bytes on a sealed role raises PB-06", exc.code == "PB-06", exc.code)
bs = q.fixture_byte_sources()
check("fixture byte sources contain no sealed entry",
      not (set(bs) & {e["corpus_id"] for e in held}),
      f"sealed leaked into byte_sources: {sorted(set(bs) & {e['corpus_id'] for e in held})}")
poison = dict(bs)
poison[held[0]["corpus_id"]] = b"SECRETPAYLOAD"
try:
    rep_poison = q.audit(fresh(), TOMB, poison, q.ExposureGuard(), SRC_TEXT)
    check("audit() refuses a sealed byte source", False, "no exception")
except q.ExposureBreach as exc:
    check("audit() refuses a sealed byte source", exc.code == "PB-06", exc.code)
except Exception as exc:
    check("audit() refuses a sealed byte source", False, repr(exc))
for fn, arg in (("network", "x"), ("codec", "anvil"), ("decompress", "arc")):
    try:
        getattr(q.ExposureGuard(), fn)(arg)
        check(f"guard.{fn}() raises", False, "no exception")
    except q.ExposureBreach as exc:
        check(f"guard.{fn}() raises", exc.code in ("PB-05", "PB-06"), exc.code)

# ---------------------------------------------------------------- T2
print("\n== T2 not_evaluated accounting ==")
rep_none = q.audit(fresh(), TOMB, None, q.ExposureGuard(), SRC_TEXT)
ne = rep_none.get("not_evaluated", [])
edges = rep_none.get("conflict_edges", [])
check("audit(byte_sources=None) still builds the graph",
      len(edges) > 0, f"edges={len(edges)}")
check("skipped criteria are COUNTED, not zero",
      len(ne) > 0, f"not_evaluated={len(ne)}")
ids = sorted(e["corpus_id"] for e in LOCK["entries"])
pairs = len(ids) * (len(ids) - 1) // 2
orient = lambda a, b: (a, b) if a <= b else (b, a)          # noqa: E731
decided = {orient(x["a"], x["b"]) for x in edges}
skipped = {orient(x["a"], x["b"]) for x in ne}
raw_decided = {(x["a"], x["b"]) for x in edges}
raw_skipped = {(x["a"], x["b"]) for x in ne}
check("every pair is either decided or explicitly not-evaluated",
      len(decided | skipped) == pairs,
      f"pairs={pairs} decided={len(decided)} skipped={len(skipped)} "
      f"unaccounted={pairs - len(decided | skipped)}")
check("edge and not_evaluated pairs share one canonical orientation",
      len(raw_decided) + len(raw_skipped - raw_decided) == len(raw_decided | raw_skipped),
      f"raw_decided={len(raw_decided)} raw_union={len(raw_decided | raw_skipped)} "
      f"both_orientations_for_same_pair="
      f"{len(raw_decided & raw_skipped ^ (raw_skipped & {(b, a) for a, b in raw_skipped})) > 0}")
crit_decided = {(orient(x["a"], x["b"]), x["criterion"]) for x in edges}
crit_skipped = {(orient(x["a"], x["b"]), x["criterion"]) for x in ne}
check("no (pair, criterion) is both decided and not-evaluated",
      not (crit_decided & crit_skipped),
      f"{sorted(crit_decided & crit_skipped)[:3]}")
check("independence_units is an int, not null",
      isinstance(rep_none["independence"].get("independence_units"), int),
      repr(rep_none["independence"].get("independence_units")))
check("report carries audit-complete semantics",
      any("not_evaluated" in k for k in rep_none), "")

# ---------------------------------------------------------------- T3
print("\n== T3 computed graph / union-find authority ==")
check("group ids declared as OUTPUTS",
      rep_none["independence"].get("group_ids_are_outputs") is True)
check("curator group fields are structurally forbidden",
      len(q.FORBIDDEN_CURATOR_FIELDS) >= 8, str(len(q.FORBIDDEN_CURATOR_FIELDS)))
bad = fresh()
for e in bad["entries"]:
    ind = e.get("independence") or {}
    ind["near_duplicate_group_ids"] = ["totally-unrelated-id"]
rep_bad = q.audit(bad, TOMB, None, q.ExposureGuard(), SRC_TEXT)
check("a false curator near-duplicate annotation is refused (PB-11)",
      any(f["code"] == "PB-11" for f in rep_bad["findings"]),
      f"{len(rep_bad['findings'])} findings")
bad2 = fresh()
bad2["deduplication"]["independence_units"] = 99
try:
    rep_bad2 = q.audit(bad2, TOMB, None, q.ExposureGuard(), SRC_TEXT)
    check("curator-supplied independence_units rejected",
          any(f["code"].startswith("schema") for f in rep_bad2["findings"]),
          f"{[f['code'] for f in rep_bad2['findings']][:4]}")
except Exception as exc:
    check("curator-supplied independence_units rejected", False, repr(exc))

# ---------------------------------------------------------------- T4
print("\n== T4 canonical ordering / determinism ==")
base_units = rep_none["independence"]["independence_units"]
perm = fresh()
perm["entries"] = list(reversed(perm["entries"]))
rep_perm = q.audit(perm, TOMB, None, q.ExposureGuard(), SRC_TEXT)
print("   report['lock'] keys:", sorted(rep_none["lock"]))
lk_key = "lock_sha256" if "lock_sha256" in rep_none["lock"] else None
check("entry array order is bound into lock identity",
      bool(lk_key) and bool(rep_perm["lock"].get(lk_key))
      and rep_perm["lock"][lk_key] != rep_none["lock"][lk_key],
      f"field={lk_key!r} perm={rep_perm['lock'].get(lk_key)!r} "
      f"orig={rep_none['lock'].get(lk_key)!r}")
shuf = json.loads(json.dumps(LOCK))
shuf = {k: shuf[k] for k in reversed(list(shuf))}
shuf["entries"] = [dict(reversed(list(e.items()))) for e in shuf["entries"]]
rep_shuf = q.audit(shuf, TOMB, None, q.ExposureGuard(), SRC_TEXT)
check("dict key order does NOT change lock identity",
      bool(lk_key) and rep_shuf["lock"][lk_key] == rep_none["lock"][lk_key],
      f"field={lk_key!r} shuf={rep_shuf['lock'].get(lk_key)!r}")
r1 = q.audit(fresh(), TOMB, None, q.ExposureGuard(), SRC_TEXT)
r2 = q.audit(fresh(), TOMB, None, q.ExposureGuard(), SRC_TEXT)
check("two audits of the same lock are byte-identical",
      q.canon_sha256(r1) == q.canon_sha256(r2))
check("declared clock fields are inputs, excluded from identity",
      "created_utc" in SRC_TEXT and "evaluated_at" in SRC_TEXT)

# ---------------------------------------------------------------- T5
print("\n== T5 threshold externalization ==")
for key in q.REQUIRED_AUDIT_PARAMETERS:
    if key in ("declared_by", "declared_sha256", "declared_utc",
               "containment_evidence_kind"):
        continue
    miss = fresh()
    del miss["audit_parameters"][key]
    try:
        rep_m = q.audit(miss, TOMB, None, q.ExposureGuard(), SRC_TEXT)
        ok = (q.threshold_lint(miss, SRC_TEXT)["status"] == "fail"
              and rep_m["verdict"]["status"] != "ADMISSIBLE_PILOT")
        check(f"missing threshold '{key}' fails closed without crashing", ok,
              f"verdict={rep_m['verdict']['status']}")
    except Exception as exc:
        check(f"missing threshold '{key}' fails closed without crashing", False,
              repr(exc))
moved = fresh()
moved["audit_parameters"]["minhash_threshold_num"] = 99
rep_mv = q.audit(moved, TOMB, q.fixture_byte_sources(), q.ExposureGuard(), SRC_TEXT)
c1 = len([e for e in rep_none["conflict_edges"] if e["criterion"] == "c5"])
check("moving a lock threshold changes the computed edge set",
      c1 != len([e for e in rep_mv["conflict_edges"] if e["criterion"] == "c5"]),
      f"c5={c1}")
extra = fresh()
extra["audit_parameters"]["hand_tuned"] = 1
check("undeclared parameter fails lint",
      q.threshold_lint(extra, SRC_TEXT)["status"] == "fail")

# ---------------------------------------------------------------- T6
print("\n== T6 tombstones and role laundering ==")
tomb_ids = {t.get("corpus_id") for t in TOMB.get("entries", [])}
check("tombstone is keyed by sha256",
      "sha256" in (q.Tombstone.__doc__ or "") or
      any("sha256" in str(k) for k in TOMB.keys()) or bool(tomb_ids),
      f"{sorted(tomb_ids)[:4]}")
relabel = fresh()
target = None
for e in relabel["entries"]:
    if e["role"] in SEALED:
        target = e
        break
if target is not None:
    tset = TOMB.get("entries", [])
    if tset:
        target["content"]["sha256"] = tset[0].get("sha256")
        rep_t = q.audit(relabel, TOMB, None, q.ExposureGuard(), SRC_TEXT)
        check("consumed bytes relabelled sealed are blocked",
              rep_t["verdict"]["status"] != "ADMISSIBLE_PILOT"
              or any("tomb" in f["code"].lower() or f["code"] == "PB-11"
                     for f in rep_t["findings"]),
              f"verdict={rep_t['verdict']['status']}")
launder = fresh()
for e in launder["entries"]:
    if e["role"] == "discovery" and (e.get("synthetic") or {}).get("generator_sha256"):
        e["role"] = "heldout"
        break
rep_l = q.audit(launder, TOMB, None, q.ExposureGuard(), SRC_TEXT)
check("a sealed object must map to the heldout split (role/split disagreement)",
      any(f["code"] in ("PB-03", "schema.role_split_disagreement",
                        "schema.missing_field", "PB-10")
          for f in rep_l["findings"])
      or rep_l["verdict"]["status"] != "ADMISSIBLE_PILOT",
      f"{[f['code'] for f in rep_l['findings']][:4]}")
selfref = fresh()
for e in selfref["entries"]:
    if e["role"] in SEALED and e.get("source", {}).get("kind") == "host-installed":
        continue
for e in selfref["entries"]:
    if e["role"] == "self_reference_control":
        e["role"] = "heldout"
rep_s = q.audit(selfref, TOMB, None, q.ExposureGuard(), SRC_TEXT)
check("self-reference control promoted to heldout is blocked",
      rep_s["verdict"]["status"] != "ADMISSIBLE_PILOT",
      f"verdict={rep_s['verdict']['status']}")
check("no verdict ever authorizes a mechanism",
      rep_none["verdict"].get("mechanism_claim_authorized") is False
      and rep_none["verdict"].get("production_authorized") is False)

# ---------------------------------------------------------------- T7
print("\n== T7 checkout semantics ==")
check("source_dirty is schema-constrained to the pinned checkout",
      'const="pinned_checkout"' in SRC_TEXT or "pinned_checkout" in SRC_TEXT)
dirty = fresh()
dirty["project"]["local_worktree_dirty"] = True
rep_d = q.audit(dirty, TOMB, None, q.ExposureGuard(), SRC_TEXT)
check("local operator dirtiness does not change the verdict",
      rep_d["verdict"]["status"] == rep_none["verdict"]["status"],
      f"{rep_none['verdict']['status']} vs {rep_d['verdict']['status']}")
notree = fresh()
del notree["project"]["source_tree_ref"]
rep_n = q.audit(notree, TOMB, None, q.ExposureGuard(), SRC_TEXT)
check("missing source_tree_ref is a finding",
      any(f["code"].startswith("E8") for f in rep_n["findings"]),
      f"{[f['code'] for f in rep_n['findings']][:3]}")
check("audit tool self-pin is exposed in the report",
      "tool" in rep_none and isinstance(rep_none["tool"], dict),
      str(sorted(rep_none.get("tool", {}))[:4]))

# ---------------------------------------------------------------- T8
print("\n== T8 the CLI's lawful byte path ==")
has_subject_root = "--subject-root" in SRC_TEXT
check("CLI exposes a subject-root byte broker", has_subject_root)
print("   NOTE: with no --subject-root the CLI is metadata+manifest only and")
print("         reports c3/c4/c5 as not_evaluated_no_bytes (AC2 honoured).")

# ---------------------------------------------------------------- T9
print("\n== T9 is c9 COMPUTED from bytes, or ATTESTED? ==")
_by_role = {e["corpus_id"]: e["role"] for e in LOCK["entries"]}
src = SRC_TEXT
check("c9 has a byte-available branch",
      "aid in byte_indexes and bid in byte_indexes" in src,
      "no byte-available branch found for c9")
check("c9 record-set decoding is declared unimplemented",
      "record-set decoding is not implemented" in src)
rep_c9 = q.audit(fresh(), TOMB, q.fixture_byte_sources(), q.ExposureGuard(), SRC_TEXT)
c9_edges = [e for e in rep_c9["conflict_edges"] if e["criterion"] == "c9"]
c9_open = [e for e in c9_edges
           if _by_role.get(e["a"]) not in SEALED
           and _by_role.get(e["b"]) not in SEALED]
c9_ne_open = [r for r in rep_c9["not_evaluated"]
              if r["criterion"] == "c9" and r["status"] == "not_evaluated_no_bytes"]
print(f"   c9 edges={len(c9_edges)} both-open={len(c9_open)} "
      f"open-pairs-not-evaluated={len(c9_ne_open)}")
check("every c9 edge is attestation-sourced and unverified",
      bool(c9_edges) and all(e.get("verified") is False
                             and e.get("evaluated_from") == "publisher_attestation"
                             for e in c9_edges),
      f"{[(e.get('evaluated_from'), e.get('verified')) for e in c9_edges][:3]}")
check("open structured pairs WITH brokered bytes are still not_evaluated for c9",
      len(c9_ne_open) > 0, f"{len(c9_ne_open)} pairs")

print("\n== T10 remaining code-level claims ==")
check("dead 'and False' predicate still present in _resolve_under_root",
      'and False:' in src or "and False" in src)
check("lock does not commit an audit-tool digest",
      "audit_tool_sha256" not in src,
      "audit_tool_sha256 appears in the source")
check("c9 edges are emitted semantically ordered (contained, container)",
      'add(\n                    "c9",\n                    contained,\n                    container,'
      in src or '"c9",\n                    contained,\n                    container,' in src)

# ---------------------------------------------------------------- T11
print("\n== T11 can an attestation-only c9 reach ADMISSIBLE_PILOT? ==")
# Build the most promotion-friendly lock the schema permits: pinned-clean checkout,
# no consumed rows, sealed roles present, attestation-only c9. If the tool can still
# emit ADMISSIBLE_PILOT here, the c9 honesty claim is false.
nice = fresh()
for e in nice["entries"]:
    e["previous_roles"] = []
nice["project"]["source_dirty"] = False
nice["consumed_controls_ref"] = dict(LOCK.get("consumed_controls_ref") or {})
nice["consumed_controls_ref"]["entry_count"] = 0
if nice.get("consumed_tombstone"):
    nice["consumed_tombstone"] = {"payload_sha256": {}, "generator_sha256": {},
                                  "archive_member": {}}
rep_nice = q.audit(nice, {"entries": []}, q.fixture_byte_sources(),
                   q.ExposureGuard(), SRC_TEXT)
st_nice = rep_nice["verdict"]["status"]
check("attestation-only c9 does NOT reach ADMISSIBLE_PILOT",
      st_nice != "ADMISSIBLE_PILOT", f"status={st_nice}")
check("and is explicitly non-authoritative for promotion",
      rep_nice["verdict"].get("independence_units_authoritative_for_promotion") is False
      or rep_nice["independence"].get("authoritative_for_promotion") is False,
      f"verdict={rep_nice['verdict'].get('independence_units_authoritative_for_promotion')} "
      f"indep={rep_nice['independence'].get('authoritative_for_promotion')}")
check("graph is labelled attestation_backed",
      rep_nice["verdict"].get("graph_verification") == "attestation_backed"
      or rep_nice["independence"].get("graph_verification") == "attestation_backed",
      f"{rep_nice['verdict'].get('graph_verification')} / "
      f"{rep_nice['independence'].get('graph_verification')}")
check("a non_authoritative_reason is published",
      bool(rep_nice["independence"].get("non_authoritative_reason")),
      str(rep_nice["independence"].get("non_authoritative_reason"))[:90])
c9g = [g for g in rep_nice["gates"] if g["gate_id"] == "Q1A-8"]
check("Q1A-8 is not 'pass' while c9 is attestation-only",
      bool(c9g) and c9g[0]["status"] != "pass",
      f"status={c9g[0]['status'] if c9g else 'ABSENT'}")

# ---------------------------------------------------------------- T12
print("\n== T12 can a schema bypass produce a SILENT ZERO graph? ==")
broken = fresh()
del broken["entries"][0]["content"]          # force a schema finding
rep_broken = q.audit(broken, TOMB, None, q.ExposureGuard(), SRC_TEXT)
sf = len(rep_broken["findings"])
ne_b = len(rep_broken.get("not_evaluated") or [])
edges_b = len(rep_broken["conflict_edges"])
ind_b = rep_broken["independence"]
n_pairs = 13 * 12 // 2
silent_zero = (edges_b == 0 and ne_b == 0)
check("forcing a schema finding still BLOCKS (not a clean audit)",
      rep_broken["verdict"]["status"] != "ADMISSIBLE_PILOT",
      f"status={rep_broken['verdict']['status']} findings={sf}")
check("a schema bypass does NOT yield edges=0 AND not_evaluated=0",
      not silent_zero,
      f"edges={edges_b} not_evaluated={ne_b} pairs={n_pairs} "
      f"audit_complete_flag_present="
      f"{any('audit_complete' in k for k in ind_b)}")
check("or it declares itself incomplete",
      (ne_b > 0) or ("audit_complete" in ind_b) or bool(ind_b.get("non_authoritative_reason")),
      f"ind={sorted(ind_b)}")
ids_b = [g["gate_id"] for g in rep_broken["gates"]]
check("gate ids remain unique under a broken lock",
      len(ids_b) == len(set(ids_b)), f"{len(ids_b)} rows / {len(set(ids_b))} unique")
failed_b = rep_broken["verdict"].get("failed_gates") or []
check("failed_gates has no duplicates",
      len(failed_b) == len(set(failed_b)), str(failed_b))

print("\n" + "=" * 70)
bad_n = sum(1 for _n, ok, _d in results if not ok)
print(f"{len(results) - bad_n}/{len(results)} probe checks passed")
print("=" * 70)
sys.exit(0)
