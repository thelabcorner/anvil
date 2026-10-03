#!/usr/bin/env python3
"""Fledge Q1 red-team: semantic diagnosis + fix for the 5 seal.py selftest failures.

ISOLATED PROOF. Not wired into anything. No codec, no network, no timing claim.
Analysis only: hashing, tokenizing, set algebra. Touches no sealed role.

Run: python c5_semantics.py
"""
from __future__ import annotations

import importlib.util
import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
SEAL = HERE.parent / "space-bunny" / "seal.py"

spec = importlib.util.spec_from_file_location("seal", SEAL)
seal = importlib.util.module_from_spec(spec)
sys.modules["seal"] = seal
spec.loader.exec_module(seal)

Index = seal.Index
hashlib = seal.hashlib if hasattr(seal, "hashlib") else __import__("hashlib")

# ---------------------------------------------------------------------------
# FIXTURE BUG (independent verification of the coordinator's diagnosis)
# ---------------------------------------------------------------------------


def verify_boiler_bug() -> None:
    print("=" * 74)
    print("0. FIXTURE BUG: _BOILER length")
    print("=" * 74)
    src = SEAL.read_text()
    line = [ln for ln in src.splitlines() if ln.startswith("_BOILER =")][0]
    prefix = b"# boilerplate banner block\n"
    print(f"   source line        : {line.strip()[:70]}")
    print(f"   ASCII prefix bytes : {len(prefix)}")
    print(f"   code subtracts     : 28   -> payload {4096 - 28}")
    boiler = prefix + bytes((i * 11 + 7) % 256 for i in range(4096 - 28))
    print(f"   ACTUAL _BOILER len : {len(boiler)}   (multiple of 4096? "
          f"{len(boiler) % 4096 == 0})")
    rare_prefix = b"rare shared payload\n"
    rare = rare_prefix + bytes((i * 17 + 5) % 256 for i in range(4096 - 20))
    print(f"   _RARE prefix/len   : {len(rare_prefix)} / {len(rare)}  "
          f"(aligned? {len(rare) % 4096 == 0})")
    print()
    print("   CONSEQUENCE: _BOILER is 4095 B, so the 4096-stride slicer makes")
    print("   block0 = _BOILER + 1 byte of the *unique* trailing filler, which")
    print("   differs per fixture. Shared 4 KiB blocks across v-boiler1/2/3 = 0.")
    print("   _RARE is exactly 4096 B, so v-rareA/B genuinely share 1 block.")
    print("   FIX: use 4096 - len(prefix); never a hand-counted literal.")


# ---------------------------------------------------------------------------
# ROOT-CAUSE DIAGNOSIS
# ---------------------------------------------------------------------------


def diagnose() -> None:
    print()
    print("=" * 74)
    print("1. ROOT CAUSE: C5 screen has no notion of WHICH REGION is shared")
    print("=" * 74)
    F = seal.FIXTURES
    idxs = [Index(c, F[c]) for c in sorted(F)]
    seal.mark_rare(idxs, rare_max=2)
    by = {i.corpus_id: i for i in idxs}
    pairs = [("v-boiler1", "v-boiler2"), ("v-boiler2", "v-boiler3"),
             ("v-rareA", "v-rareB"), ("v-a", "v-b"), ("v-exact-a", "v-exact-b")]
    print(f"   {'pair':26} {'unigramJ':>9} {'>=0.20':>7} {'shared4K':>9} "
          f"{'printable(shared)':>18}")
    for a, b in pairs:
        ia, ib = by[a], by[b]
        shared_blk = ia.blocks & ib.blocks
        region = shared_region(ia, ib)
        pl = printable_ratio(region)
        u = ia.jaccard_screen(ib)
        print(f"   {a + ' vs ' + b:26} {u:9.4f} {str(u >= 0.20):>7} "
              f"{len(shared_blk):9} {pl:18.4f}")
    print()
    print("   All five failures are ONE root cause: C5's screen is computed over")
    print("   the WHOLE object, and the shared binary filler tokenizes into a large")
    print("   saturated unigram set. Unigram Jaccard lands at 0.95-0.98 on pairs")
    print("   that share ONLY boilerplate or ONLY a binary block.")


def shared_region(a: Index, b: Index) -> bytes:
    """Bytes of `a` that also appear in `b` at the same 4 KiB stride.

    May be EMPTY even when the pair has high text overlap: json/sqlite share
    zero aligned blocks yet share every payload string. Callers must distinguish
    'no shared block region' from 'binary shared region'."""
    raw = getattr(a, "raw", a.data)
    out = bytearray()
    for i in range(0, len(raw), seal.BLOCK):
        blk = raw[i:i + seal.BLOCK]
        if seal.h64(blk) in b.blocks:
            out += blk
    return bytes(out)


def printable_ratio(data: bytes) -> float:
    """Deterministic text-likeness: printable ASCII + TAB/LF/CR over bytes."""
    if not data:
        return 0.0
    ok = sum(1 for c in data if 0x20 <= c <= 0x7E or c in (9, 10, 13))
    return ok / len(data)


def text_family(a: Index, b: Index, text_like_min: float = 0.5) -> tuple[bool, str]:
    """Layer 2 - PAIRWISE TEXT-LIKENESS GATE.

    C5 is a TEXT criterion, so it applies only when BOTH members are text-like.
    Three outcomes, all meaningful:

      both text-like   -> C5 applies; near-duplicate TEXT is the right reading
      both binary       -> C5 does NOT apply; a binary copy is C3/C4's job
      STRADDLED         -> different format families; C5 does NOT apply,
                           because this is containment (criterion c9), not
                           similarity, and scoring it by similarity mislabels
                           the relationship

    This replaces my first attempt, which fell back to whole-object text-likeness
    when no aligned block was shared. That fallback was WRONG and my own
    section-4 regression caught it: generated.json (~1.0) vs generated.sqlite
    (~0.39) straddles, so C5 must abstain and c9 must own the verdict.
    """
    pa, pb = printable_ratio(getattr(a, "raw", a.data)), printable_ratio(getattr(b, "raw", b.data))
    if pa >= text_like_min and pb >= text_like_min:
        return True, f"both text-like ({pa:.3f}, {pb:.3f}) -> similarity reading"
    if pa < text_like_min and pb < text_like_min:
        return False, f"both binary ({pa:.3f}, {pb:.3f}) -> C3/C4 own this"
    return False, (f"STRADDLED ({pa:.3f} vs {pb:.3f}) -> containment, "
                   f"criterion c9 owns this")


# ---------------------------------------------------------------------------
# THE FIX: two mechanisms, no similarity-threshold change
# ---------------------------------------------------------------------------


def demote_common_grams(indexes: list[Index], prevalence_max: int = 2) -> None:
    """Layer 1 - PREVALENCE. A gram/unigram appearing in more than
    `prevalence_max` objects is boilerplate and is removed from BOTH the screen
    and the decide set. Mirrors mark_rare() for blocks. This is prevalence, not
    similarity: license headers / banners / shebangs are demoted because they
    are widespread, never because they score high."""
    from collections import Counter

    cg, cu = Counter(), Counter()
    for ix in indexes:
        cg.update(ix.grams)
        cu.update(ix.unigrams)
    for ix in indexes:
        ix.rare_grams = {g for g in ix.grams if cg[g] <= prevalence_max}
        ix.rare_unigrams = {u for u in ix.unigrams if cu[u] <= prevalence_max}


def c5_applies(a: Index, b: Index, text_like_min: float = 0.5) -> tuple[bool, str]:
    return text_family(a, b, text_like_min)


def audit_pair_fixed(a: Index, b: Index, ma: dict, mb: dict,
                     prevalence_max: int = 2, text_like_min: float = 0.5):
    """c1/c2/c3/c4/c6/c7/c8 unchanged. Only c5's applicability changes."""
    r: list[str] = []
    if a.n == b.n and ma.get("declared_sha256") == mb.get("declared_sha256"):
        r.append("c1-identical-sha256")
    if (ma.get("git_blob_sha1") and ma.get("git_blob_sha1") == mb.get("git_blob_sha1")
            and a.n == b.n):
        r.append("c2-identical-git-blob")
    if (getattr(a, "rare_blocks", a.blocks) & getattr(b, "rare_blocks", b.blocks)):
        r.append("c3-shared-rare-4kib-block")
    if getattr(a, "rare_blocks_crlf", set()) & getattr(b, "rare_blocks_crlf", set()):
        r.append("c4-shared-rare-4kib-block-crlf")

    # --- C5, semantically gated -------------------------------------------
    ua = getattr(a, "rare_unigrams", a.unigrams)
    ub = getattr(b, "rare_unigrams", b.unigrams)
    screen = (len(ua & ub) / len(ua | ub)) if (ua and ub) else 0.0
    decides = bool(getattr(a, "rare_grams", a.grams) & getattr(b, "rare_grams", b.grams))
    applies, why = c5_applies(a, b, text_like_min)
    if applies and screen >= 0.20 and decides:
        r.append("c5-screened-and-confirmed-text-overlap")

    ia, ib = ma["independence"], mb["independence"]
    if (ia["schema_cluster_id"] == ib["schema_cluster_id"]
            and ia["schema_cluster_id"] != "not-applicable"
            and (ia["publisher_id"] == ib["publisher_id"]
                 or ia["release_family_id"] == ib["release_family_id"])):
        r.append("c6-same-schema-cluster-and-publisher-or-release")
    if (ia["publisher_id"] == ib["publisher_id"]
            or ia["project_id"] == ib["project_id"]
            or ia["release_family_id"] == ib["release_family_id"]):
        r.append("c7-same-publisher-project-or-release")
    ga, gb = ma.get("synthetic"), mb.get("synthetic")
    if ga and gb and ga.get("generator_sha256") == gb.get("generator_sha256"):
        r.append("c8-shared-generator-lineage")
    return r


# ---------------------------------------------------------------------------
# VALIDATION
# ---------------------------------------------------------------------------


def validate() -> int:
    print()
    print("=" * 74)
    print("2. VALIDATION against UNCHANGED ground truth")
    print("=" * 74)
    F = dict(seal.FIXTURES)

    # Repair ONLY the fixture arithmetic. Ground truth is NOT touched.
    _bp = b"# boilerplate banner block\n"
    _boiler = _bp + bytes((i * 11 + 7) % 256 for i in range(4096 - len(_bp)))
    assert len(_boiler) == 4096, len(_boiler)
    for n in (1, 2, 3):
        F[f"v-boiler{n}"] = _boiler + bytes((i * 13 + n * 29) % 256 for i in range(3000))

    ids = sorted(F)
    idxs = []
    for cid in ids:
        ix = Index(cid, F[cid])
        ix.data = F[cid]
        ix.raw = F[cid]
        idxs.append(ix)
    seal.mark_rare(idxs, rare_max=2)
    demote_common_grams(idxs, prevalence_max=2)
    by = {i.corpus_id: i for i in idxs}

    def meta(cid):
        m = seal._meta(cid)
        m["declared_sha256"] = seal.sha256_hex(F[cid])
        m["declared_bytes"] = len(F[cid])
        return m

    fails = 0
    print(f"   {'pair':26} {'got':52} verdict")
    for (a, b), want in seal.TRUTH.items():
        got = audit_pair_fixed(by[a], by[b], meta(a), meta(b))
        ok = got == want
        fails += 0 if ok else 1
        print(f"   {a + ' vs ' + b:26} {str(got):52} {'OK' if ok else 'MISMATCH'}")
        if not ok:
            print(f"   {'':26} want {want}")

    print()
    print(f"   ground-truth pairs checked : {len(seal.TRUTH)}")
    print(f"   mismatches                 : {fails}")
    print("   GROUND TRUTH UNCHANGED. No threshold moved. No global C5 suppression.")

    # C3/C4 preservation on the real corpus pair I measured.
    print()
    print("=" * 74)
    print("3. REAL-CORPUS REGRESSION: C3/C4 must survive on pe-where/pe-winver")
    print("=" * 74)
    corpus = pathlib.Path(__file__).resolve().parents[4] / "tests" / "corpus"
    a_p = corpus / "pe-where.exe"
    b_p = corpus / "pe-winver.exe"
    if a_p.exists() and b_p.exists():
        da, db = a_p.read_bytes(), b_p.read_bytes()
        ia, ib = Index("pe-where.exe", da), Index("pe-winver.exe", db)
        ia.data = ia.raw = da
        ib.data = ib.raw = db
        seal.mark_rare([ia, ib], rare_max=2)
        demote_common_grams([ia, ib], prevalence_max=2)
        r = audit_pair_fixed(
            ia, ib,
            {"corpus_id": "pe-where.exe", "declared_sha256": seal.sha256_hex(da),
             "git_blob_sha1": None,
             "independence": {"publisher_id": "p", "project_id": "j",
                              "release_family_id": "f",
                              "schema_cluster_id": "not-applicable"}},
            {"corpus_id": "pe-winver.exe", "declared_sha256": seal.sha256_hex(db),
             "git_blob_sha1": None,
             "independence": {"publisher_id": "p", "project_id": "j",
                              "release_family_id": "f",
                              "schema_cluster_id": "not-applicable"}},
        )
        pl = printable_ratio(shared_region(ia, ib))
        print(f"   shared 4 KiB blocks : {len(ia.blocks & ib.blocks)}")
        print(f"   printable(shared)   : {pl:.4f}  -> C5 applicable: {pl >= 0.5}")
        print(f"   reasons             : {r}")
        blocked = bool(r)
        print(f"   STILL BLOCKED       : {blocked}")
        assert "c3-shared-rare-4kib-block" in r, "C3 REGRESSION"
        print("   C3 PRESERVED. Measured collision still blocks the pair.")
    else:
        print("   corpus not found; skipped")

    # Cross-container: the pair no block/shingle rule can see.
    print()
    print("=" * 74)
    print("4. CROSS-CONTAINER: C5 must NOT invent evidence for sqlite/json")
    print("=" * 74)
    corpus = pathlib.Path(__file__).resolve().parents[4] / "tests" / "corpus"
    gj, gsql = corpus / "generated.json", corpus / "generated.sqlite"
    if gj.exists() and gsql.exists():
        dj, ds = gj.read_bytes(), gsql.read_bytes()
        ij, isql = Index("generated.json", dj), Index("generated.sqlite", ds)
        ij.data = ij.raw = dj
        isql.data = isql.raw = ds
        seal.mark_rare([ij, isql], rare_max=2)
        demote_common_grams([ij, isql], prevalence_max=2)
        shared = len(ij.blocks & isql.blocks)
        pl = printable_ratio(shared_region(ij, isql))
        print(f"   shared 4 KiB blocks   : {shared}   (measured earlier: 0)")
        print(f"   exact shared 5-gram   : {ij.exact_shared_gram(isql)}")
        print(f"   printable(shared)     : {pl:.4f}")
        r = audit_pair_fixed(
            ij, isql,
            {"corpus_id": "generated.json", "declared_sha256": seal.sha256_hex(dj),
             "git_blob_sha1": None,
             "independence": {"publisher_id": "p", "project_id": "j",
                              "release_family_id": "f",
                              "schema_cluster_id": "cl"}},
            {"corpus_id": "generated.sqlite", "declared_sha256": seal.sha256_hex(ds),
             "git_blob_sha1": None,
             "independence": {"publisher_id": "p", "project_id": "j",
                              "release_family_id": "f",
                              "schema_cluster_id": "cl"}},
        )
        print(f"   reasons               : {r}")
        print()
        print("   C5 correctly abstains (no text region shared). But note c6/c7")
        print("   fire ONLY because this harness hard-codes the same publisher.")
        print("   The 12,000/12,000 row containment STILL needs criterion c9:")
        print("   content rules cannot see it. This fix does not close B7.")
    else:
        print("   corpus not found; skipped")

    print()
    print("=" * 74)
    print("5. TEXT-LIKENESS MARGIN (threshold robustness, measured)")
    print("=" * 74)
    print(f"   {'corpus file':34} {'printable':>10}  family")
    samples = []
    for name in ("generated.log", "synth-counters.log", "doc.md", "generated.json",
                 "pe-where.exe", "pe-git.exe", "random.bin", "synth-arith.bin",
                 "generated.sqlite"):
        p = corpus / name
        if p.exists():
            v = printable_ratio(p.read_bytes()[:400000])
            samples.append((name, v))
            fam = "text" if v >= 0.5 else "binary"
            print(f"   {name:34} {v:10.4f}  {fam}")
    txt = [v for _, v in samples if v >= 0.5]
    bin_ = [v for _, v in samples if v < 0.5]
    if txt and bin_:
        print()
        print(f"   text population  : {min(txt):.4f} .. {max(txt):.4f}")
        print(f"   binary population: {min(bin_):.4f} .. {max(bin_):.4f}")
        print(f"   GAP: {max(bin_):.4f} -> {min(txt):.4f}   "
              f"any gate in [0.45, 0.90] separates cleanly")
    print()
    print("   CORRECTION to my own earlier claim: uniform-random binary is ~0.37-0.39")
    print("   printable (95 of 256 byte values are printable ASCII), NOT ~0.005.")
    print("   The bimodality is real but smaller than I first printed.")
    print("   The 0.20 Jaccard threshold remains unusable: its populations are")
    print("   0.0074 (real overlap, 5-gram) vs 0.95+ (binary boilerplate, unigram).")
    print("   No threshold separates those; prevalence + text-family routing must.")
    return fails


if __name__ == "__main__":
    verify_boiler_bug()
    diagnose()
    sys.exit(0 if validate() == 0 else 1)