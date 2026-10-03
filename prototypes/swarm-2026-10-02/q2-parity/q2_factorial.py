#!/usr/bin/env python3
"""Q2 analysis: exact signed 2x2 factorial contrasts + retained byte/identity fidelity.

Isolated prototype. Not wired to production. NOT dispatchable.

Design authority: docs/swarm-2026-10-02/Q2-PARITY-PLAN-SPACE-BUNNY.md (Rev 5).

Q2 publishes measurements and attaches no verdict. Deliberate absences, all coordinator-mandated:

  * NO frontier predicate. The DOMINATED / DEGENERATE / FRONT-GAP / FRONT-CROSSING vocabulary is
    not computed, not replayed, and not emitted anywhere -- not even in a "diagnostic". The retained
    baseline check is BYTE AND IDENTITY FIDELITY ONLY: does this run's G0 geometry reproduce the
    retained Class-A byte counts for each configuration, and does the retained twin structure
    survive? That is a provenance question, not a classification question.
  * NO frontier class token in any artifact, at any grade. `assert_no_frontier_vocabulary` enforces it.
  * NO epsilon parameter. None exists.
  * NO materiality threshold. `materiality_threshold: null` everywhere.
  * NO label, void trigger, or classification minted from any published number. A non-zero
    interaction is a measured geometry x gate interaction; a larger-block byte increase is a
    measured geometry effect; neither invalidates anything. Interpretation is the coordinator's.
  * Timing, parallelism, RSS, binary size and reference-codec bytes are DESCRIPTIVE and enter no
    contrast.
"""
from __future__ import annotations

import argparse
import csv
import itertools
import json
import sys
from collections import defaultdict
from pathlib import Path
from typing import Iterable, Mapping

import q2_arms as _A  # retained twin groups (declared identity expectation)

# ---------------------------------------------------------------------------
# Frozen constants
# ---------------------------------------------------------------------------

CELLS = ("G0", "G1", "G2", "G3")
CONTRAST_IDS = ("geom_effect_gate_on", "geom_effect_gate_off",
               "gate_effect_at_small", "gate_effect_at_large")
CONTRAST_PAIRS = {
    "geom_effect_gate_on": ("G1", "G0"),
    "geom_effect_gate_off": ("G2", "G3"),
    "gate_effect_at_small": ("G0", "G3"),
    "gate_effect_at_large": ("G1", "G2"),
}

# Reporting-only annotation; never a threshold.
FRAMING_BYTES_PER_BLOCK = 11      # FORMAT.md:25-33

# Declared retained twin groups, imported from the arm registry. See _A for the basis.
RETAINED_TWIN_GROUPS = _A.RETAINED_TWIN_GROUPS
RETAINED_TWIN_BASIS = _A.RETAINED_TWIN_BASIS
RETAINED_ROW_BY_CONFIG = {c.config_id: c.frozen_row for c in _A.CANDIDATE_CONFIGS}

# The published token. It asserts that contrasts were measured and published. Nothing more.
DECISION_TOKEN = "PARITY-GEOMETRY-REPORTED"

# The complete frontier vocabulary Q2 is forbidden to emit. Includes the two the coordinator
# named explicitly plus the remaining frontier classes and the crossing-candidate stand-in.
FORBIDDEN_FRONTIER_TOKENS = (
    "FRONT-CROSSING",
    "FRONT-GAP",
    "BASELINE-CROSSING-CANDIDATE",
    "DOMINATED",
    "DEGENERATE",
)
_VOCAB_DECLARATION_MARKERS = (
    "FORBIDDEN_FRONTIER_TOKENS",   # the constant that declares the prohibition
    "forbidden frontier",           # prose declaring it
    "frontier vocabulary",
)


# ---------------------------------------------------------------------------
# Part 1 -- the factorial. Exact signed bytes + relative percentages. No thresholds.
# ---------------------------------------------------------------------------

def _pct(delta: int, denom: int) -> float | None:
    return None if denom == 0 else 100.0 * delta / denom


def _sign(delta: int) -> str:
    return "ZERO" if delta == 0 else ("NEGATIVE" if delta < 0 else "POSITIVE")


def contrasts_for_group(g0: int, g1: int, g2: int, g3: int) -> dict:
    """The frozen algebra, exactly as specified. No epsilon, no threshold, no rounding.

    Complete crossed 2x2: (G0 small/on, G1 large/on, G2 large/off, G3 small/off).
    Each main effect holds the other factor fixed exactly.
    """
    gate_on = g1 - g0      # geometry factor, gate held ON
    gate_off = g2 - g3     # geometry factor, gate held OFF
    at_small = g0 - g3     # gate factor, geometry held SMALL (identical block size/count)
    at_large = g1 - g2     # gate factor, geometry held LARGE (identical block size/count)
    interaction_a = at_large - at_small           # (G1 - G2) - (G0 - G3)
    interaction_b = gate_on - gate_off           # (G1 - G0) - (G2 - G3)
    return {
        "bytes": {
            "G0": g0, "G1": g1, "G2": g2, "G3": g3,
            "geom_effect_gate_on": gate_on,
            "geom_effect_gate_off": gate_off,
            "gate_effect_at_small": at_small,
            "gate_effect_at_large": at_large,
            "interaction": interaction_a,
        },
        # identity is asserted, not assumed
        "interaction_identity_holds": interaction_a == interaction_b,
        "interaction_identity_delta": interaction_a - interaction_b,
        "pct_of_g0_bytes": {
            "geom_effect_gate_on": _pct(gate_on, g0),
            "geom_effect_gate_off": _pct(gate_off, g0),
            "gate_effect_at_small": _pct(at_small, g0),
            "gate_effect_at_large": _pct(at_large, g0),
            "interaction": _pct(interaction_a, g0),
        },
        # threshold-free sign labels over the published integers
        "sign": {
            "geom_effect_gate_on": _sign(gate_on),
            "geom_effect_gate_off": _sign(gate_off),
            "gate_effect_at_small": _sign(at_small),
            "gate_effect_at_large": _sign(at_large),
            "interaction": _sign(interaction_a),
        },
    }


def compute_contrasts(rows: Iterable[Mapping], g_s: dict | None = None) -> dict:
    """rows: collector JSONL records with config_id, file, cell, complete_bytes.

    `g_s` is the result of the retained-twin PREFLIGHT. If it is supplied and did not pass, the
    geometry contrasts are computed but explicitly WITHHELD (`contrasts_withheld: true`, empty
    `records`), so no downstream surface can read a contrast off a drifted substrate.
    """
    rows = list(rows)
    g_s_pass = True if g_s is None else bool(g_s.get("contrasts_emitted"))
    groups: dict[tuple[str, str], dict[str, Mapping]] = defaultdict(dict)
    meta: dict[tuple[str, str], dict] = {}
    for r in rows:
        key = (str(r["config_id"]), str(r["file"]))
        cell = str(r["cell"])
        if cell not in CELLS:
            continue
        groups[key][cell] = r
        meta[key] = {
            "source_bytes": r.get("source_bytes"),
            "identity_role": r.get("identity_role"),
            "block_count_small": r.get("block_count"),
            "framing_bytes_descriptive": r.get("framing_bytes_descriptive"),
            "roundtrip_verified_all_cells": all(
                bool(groups[key].get(c, {}).get("roundtrip_verified")) for c in CELLS
            ),
        }

    records: list[dict] = []
    incomplete: list[dict] = []
    for key in sorted(groups):
        cells = groups[key]
        missing = [c for c in CELLS if c not in cells]
        if missing:
            incomplete.append({"config_id": key[0], "file": key[1], "missing_cells": missing})
            continue
        b = {c: int(cells[c]["complete_bytes"]) for c in CELLS}
        meta[key]["roundtrip_verified_all_cells"] = all(
            bool(cells[c].get("roundtrip_verified")) for c in CELLS
        )
        records.append({
            "config_id": key[0],
            "file": key[1],
            **meta[key],
            **contrasts_for_group(b["G0"], b["G1"], b["G2"], b["G3"]),
        })

    return {
        "schema": 5,
        "protocol": "Q2-PARITY-2X2-MULTICONFIG",
        "decision_token": DECISION_TOKEN,
        "token_meaning": "contrasts were measured and published; no grade, no verdict",
        "materiality_threshold": None,
        "epsilon": None,
        "pct_denominator": "bytes(G0) of the same configuration and file",
        "naming_note": "Complete crossed 2x2 in {geometry small|large} x {gate on|off}, so "
                       "every main effect holds the other factor fixed exactly. "
                       "geom_effect_gate_on/off hold the gate fixed; gate_effect_at_small/large "
                       "hold the geometry fixed (identical block size AND identical block count "
                       "within each pair). No contrast is confounded. gate_effect_at_small and "
                       "gate_effect_at_large are GATE main effects: never window effects, never "
                       "geometry effects, never geometry controls. Their difference IS the "
                       "interaction.",
        "rejected_claim": "Any assertion that G0 - G3 is a 'pure window effect' is rejected: G0 "
                          "and G3 share an identical block size, so that contrast does not vary "
                          "geometry at all.",
        "classifying_axes": ["complete_bytes"],
        "non_classifying_axes": ["encode_MBps", "decode_MBps", "peak_rss_kib",
                                 "segment_count", "max_parallel_decode_units", "binary_bytes",
                                 "reference_bytes"],
        "output_classes": {
            "raw_exact_contrasts": "the four contrasts plus the interaction, exact integers, "
                                   "with relative percentages against bytes(G0); published per "
                                   "file AND as role-stratified exact totals (A tracked, "
                                   "B build-output-under-test, C explicitly mixed)",
            "validity": ["roundtrip_verified", "complete_bytes", "compressed_sha256",
                         "roundtrip_sha256", "roundtrip_source_sha256", "source_sha256",
                         "evidence_role", "identity_role", "argv_encode"],
            "provenance": ["workflow_commit_sha", "base_parent_sha", "production blob ids",
                           "binary identity", "helper identity", "source_tree_ref"],
            "retained_twin_fidelity": "recorded identity check; decisional: false",
            "retained_byte_fidelity": "reported provenance fidelity across the full set; the "
                                      "PRECONDITION is the fail-closed G-S preflight over "
                                      "stratum A, run before any geometry arm",
        },
        "interpretation": "belongs to the coordinator after measurement; Q2 mints no label, no "
                          "void trigger, and no classification from any published number",
        "g_s_preflight": g_s,
        "contrasts_withheld": not g_s_pass,
        "withheld_reason": None if g_s_pass else
            "G-S retained-twin preflight did not pass; geometry contrasts are withheld pending "
            "substrate-integrity resolution (BASELINE-VOID)",
        "records": [] if not g_s_pass else records,
        "incomplete_groups": incomplete,
        "corpus_totals": _corpus_totals(records if g_s_pass else []),
        "retained_twin_fidelity": retained_twin_fidelity(records if g_s_pass else []),
        "config_contrast_equivalence": _config_equivalence(records if g_s_pass else []),
        "descriptive_only_reference_residual": _reference_residual(rows),
        "reference_producer_state": {
            "brotli_producer_pinned": _A.BROTLI_PRODUCER_PINNED,
            "hold": None if _A.BROTLI_PRODUCER_PINNED else _A.PRODUCER_HOLD,
            "residual_withheld": not _A.BROTLI_PRODUCER_PINNED,
            "reason": None if _A.BROTLI_PRODUCER_PINNED else _A.PRODUCER_HOLD_REASON,
            "identity": "R1/R3 are a same-run matched geometry-control pair. R1 is NOT a "
                        "reproduction of the retained Brotli rows, which used "
                        f"{_A.FROZEN_BROTLI_WINDOW}, while R1 uses explicit lgwin "
                        f"{_A.R1_EXPLICIT_LGWIN}. The pair is never compared to the frozen grid.",
        },
        "structural_capacity_basis": {
            "formula": "min(N, nblocks) / block_count",
            "source": "src/anvil.cpp:4908",
            "emitted": "symbolically; the pinned decode_threads=1 evaluation is reported only as "
                       "a special case and is not the structural capacity",
            "timing_implied": False,
        },
    }


def _stratum_totals(records: list[dict], label: str, note: str) -> dict:
    ids = list(CONTRAST_IDS) + ["interaction"]
    acc = {k: 0 for k in ids}
    g0_sum = 0
    files: set[str] = set()
    for rec in records:
        for k in ids:
            acc[k] += rec["bytes"][k]
        g0_sum += rec["bytes"]["G0"]
        files.add(str(rec["file"]))
    return {
        "stratum": label,
        "groups": len(records),
        "files": sorted(files),
        "sum_bytes_G0": g0_sum,
        "sum_signed_bytes": acc,
        "sum_pct_of_G0": {k: _pct(v, g0_sum) for k, v in acc.items()},
        "sign": {k: _sign(v) for k, v in acc.items()},
        "note": note,
    }


def _corpus_totals(records: list[dict]) -> dict:
    """ROLE-STRATIFIED exact totals.

    Stratum A is the tracked, Q0-compatible files. Stratum B is the two build-output slots,
    whose bytes on a hosted Linux run are FRESH BUILDS and are not the historical Windows objects
    the retained grid measured -- so they are never presented as such. Stratum C is the
    explicitly mixed 13-file discovery aggregate. Every per-file contrast is published unchanged
    regardless of stratum.
    """
    tracked = [r for r in records if r.get("identity_role") != "build-output-under-test"]
    build_out = [r for r in records if r.get("identity_role") == "build-output-under-test"]
    return {
        "stratified": True,
        "A_tracked_q0_compatible": _stratum_totals(
            tracked, "A_tracked_q0_compatible",
            "tracked files that are byte-identical to the retained Class-A corpus objects"),
        "B_build_output_under_test": _stratum_totals(
            build_out, "B_build_output_under_test",
            "anvil.exe / anvil_bench.exe are THIS BUILD's outputs under test. Their bytes are "
            "fresh Linux build artifacts, NOT the historical Windows objects the retained grid "
            "measured. Never cite them as retained bytes and never pool them silently."),
        "C_all_13_mixed_discovery_aggregate": _stratum_totals(
            records, "C_all_13_mixed_discovery_aggregate",
            "EXPLICITLY MIXED: pools stratum A and stratum B. Discovery aggregate only. Because "
            "stratum B is not the retained object, this aggregate is not comparable to any "
            "retained-grid total."),
        "pooling_warning": "Stratum C mixes tracked retained-equivalent files with fresh build "
                           "outputs. Read strata A and B separately; do not treat C as a "
                           "retained-grid figure.",
        "interpretation": "coordinator-only; Q2 attaches no label to any stratum",
    }


def _config_equivalence(records: list[dict]) -> dict:
    """Exact-equality classes over the configurations' contrast vectors. Reported, not enforced."""
    by_cfg: dict[str, set[tuple]] = defaultdict(set)
    for rec in records:
        by_cfg[str(rec["config_id"])].add(
            tuple(rec["bytes"][k] for k in (*CONTRAST_IDS, "interaction"))
        )
    identical_pairs = [
        {"config_a": a, "config_b": b, "contrast_vectors_identical": True}
        for a, b in itertools.combinations(sorted(by_cfg), 2) if by_cfg[a] == by_cfg[b]
    ]
    return {
        "distinct_contrast_vectors_per_config": {k: len(v) for k, v in sorted(by_cfg.items())},
        "config_pairs_with_identical_contrast_vectors": identical_pairs,
        "decisional": False,
        "note": "exact integer equality; no tolerance, no threshold; reported, not enforced",
    }


def retained_twin_fidelity(records: list[dict]) -> dict:
    """Does the retained byte-identity of the twin configurations survive at each geometry?

    RECORDED, NOT ENFORCED. A divergence is an observation about configuration identity; it does
    not invalidate the run, void any contrast, trigger a label, or license a conclusion.
    """
    by_file: dict[str, dict[str, dict]] = defaultdict(dict)
    for rec in records:
        by_file[str(rec["file"])][str(rec["config_id"])] = rec["bytes"]
    rows: list[dict] = []
    for fname in sorted(by_file):
        b = by_file[fname]
        for gid, members in sorted(RETAINED_TWIN_GROUPS.items()):
            present = [m for m in members if m in b]
            if len(present) < 2:
                rows.append({"file": fname, "twin_group": gid, "members": present,
                             "status": "INCOMPLETE", "decisional": False})
                continue
            per_cell: dict[str, dict] = {}
            for cell in CELLS:
                vals = sorted({b[m][cell] for m in present})
                per_cell[cell] = {"identical": len(vals) == 1, "complete_bytes": vals}
            rows.append({
                "file": fname,
                "twin_group": gid,
                "members": present,
                "basis": RETAINED_TWIN_BASIS,
                "identical_at_G0_frozen_geometry": per_cell["G0"]["identical"],
                "per_cell": per_cell,
                "divergent_cells": [c for c, v in per_cell.items() if not v["identical"]],
                "decisional": False,
                "note": "observation only; does not invalidate, void, or label anything",
            })
    return {
        "role": "provenance / identity fidelity",
        "decisional": False,
        "twin_groups": {k: list(v) for k, v in RETAINED_TWIN_GROUPS.items()},
        "basis": RETAINED_TWIN_BASIS,
        "files_with_any_divergence": sorted({r["file"] for r in rows if r.get("divergent_cells")}),
        "rows": rows,
    }


def _reference_residual(rows: Iterable[Mapping]) -> dict:
    """Descriptive only. Reported, never compared against a cut value, never labelled."""
    by: dict[tuple[str, str], dict[str, int]] = defaultdict(dict)
    meta: dict[tuple[str, str], dict] = {}
    for r in rows:
        arm = r.get("arm_id") or str(r.get("cell_id", "@")).split("@")[-1]
        if arm not in ("R1", "R3"):
            continue
        key = (str(r["file"]), str(r["config_id"]))
        by[key][arm] = int(r["complete_bytes"])
        meta[key] = {
            "source_bytes": r.get("source_bytes"),
            "identity_role": r.get("identity_role"),
            "block_count_small": r.get("block_count"),
            "framing_bytes_descriptive": r.get("framing_bytes_descriptive"),
        }
    out = []
    for key in sorted(by):
        arms = by[key]
        if "R1" not in arms or "R3" not in arms:
            continue
        payload = meta[key].get("payload_sum_bytes")
        entry = {
            "file": key[0], "config_id": key[1],
            "R1_bytes": arms["R1"], "R3_bytes": arms["R3"],
            "R3_payload_sum_bytes": payload,
            "framing_bytes": meta[key].get("framing_bytes"),
            "independent_streams": meta[key].get("independent_streams"),
        }
        if payload is not None:
            entry["D_ref_framing_free_bytes"] = payload - arms["R1"]
            entry["D_ref_framing_free_pct_of_R1"] = _pct(payload - arms["R1"], arms["R1"])
            entry["D_ref_sign"] = _sign(payload - arms["R1"])
        entry["D_ref_incl_framing_bytes"] = arms["R3"] - arms["R1"]
        entry["D_ref_incl_framing_sign"] = _sign(arms["R3"] - arms["R1"])
        out.append(entry)
    return {
        "role": "descriptive / non-classifying",
        "reference_producer_pinned": False,
        "note": "same-job Brotli q11/lgwin30 fragmentation delta. The reference producer is "
                "environment-recorded but not version-pinned, so this is diagnostic only: no "
                "frozen-grid equality claim, no parity ruling, no band, threshold, label, or void.",
        "rows": out,
    }


# ---------------------------------------------------------------------------
# Part 2 -- retained byte/provenance fidelity. BYTES AND IDENTITY ONLY.
#
# No frontier predicate. No dominance test. No bracket test. No class of any kind.
# This answers exactly one provenance question: does this run's G0 geometry reproduce the
# retained Class-A byte count for the same configuration and file?
# ---------------------------------------------------------------------------

def load_retained_bytes(suite: Path) -> dict[tuple[str, str], int]:
    """(codec, file) -> compressed_bytes from the retained Class-A CSV. Pure CSV arithmetic."""
    out: dict[tuple[str, str], int] = {}
    with suite.open(newline="", encoding="utf-8-sig") as handle:
        for row in csv.DictReader(handle):
            name = Path(row["file"].replace("\\", "/")).name
            out[(row["codec"], name)] = int(row["compressed_bytes"])
    return out


def g_s_preflight(rows: Iterable[Mapping], suite: Path | None) -> dict:
    """G-S retained-twin PREFLIGHT -- the first-stage gate.

    For every STRATUM-A (tracked) file and every retained configuration, the G0 bytes
    (262,144 B, gate ON -- the frozen Class-A geometry) MUST be byte-identical to the frozen
    per-file candidate bytes in the retained suite.

    Why first, and why fail-closed: if G0 does not reproduce the retained grid, then the build,
    the corpus bytes, or the configuration identity has drifted, and every G1/G2/G3 contrast
    would be measured against a moving substrate while burning the most expensive arms. So this
    runs on G0 alone, before any geometry arm, and a mismatch withholds the contrasts entirely.

    Mismatch outcome is ``BASELINE-VOID`` -- build/corpus drift. It is a substrate outcome, not a
    codec result, and it carries no interpretation.
    """
    if suite is None:
        return {"gate": "G-S", "status": "NOT-RUN", "fail_closed": False,
                "contrasts_emitted": False,
                "reason": "no retained suite supplied"}
    retained = load_retained_bytes(suite)
    pairs: list[dict] = []
    for r in rows:
        if str(r.get("cell")) != "G0":
            continue
        if r.get("identity_role") != "tracked":
            continue  # stratum A only; build-output slots are excluded by construction
        cfg = str(r["config_id"])
        row_name = RETAINED_ROW_BY_CONFIG.get(cfg)
        if row_name is None:
            continue
        want = retained.get((row_name, str(r["file"])))
        if want is None:
            pairs.append({"config_id": cfg, "file": r["file"], "status": "NOT-IN-RETAINED"})
            continue
        got = int(r["complete_bytes"])
        pairs.append({
            "config_id": cfg,
            "retained_row": row_name,
            "file": r["file"],
            "retained_bytes": want,
            "observed_g0_bytes": got,
            "byte_identical": got == want,
            "delta_bytes": got - want,
        })
    compared = [p for p in pairs if "byte_identical" in p]
    mismatched = [p for p in compared if not p["byte_identical"]]
    ok = bool(compared) and not mismatched
    return {
        "gate": "G-S",
        "name": "retained-twin preflight: G0 on stratum A vs frozen per-file candidate bytes",
        "status": "G-S-PASS" if ok else "BASELINE-VOID",
        "fail_closed": True,
        "contrasts_emitted": ok,
        "compared": len(compared),
        "identical": len(compared) - len(mismatched),
        "mismatched": len(mismatched),
        "mismatch_detail": mismatched,
        "stratum": "A_tracked_only",
        "geometry_checked": "G0 (262144 B, gate ON) -- the frozen Class-A geometry",
        "void_reason": None if ok else
            "G0 bytes do not reproduce the retained per-file candidate bytes on stratum A. Build, "
            "corpus bytes, or configuration identity has drifted; geometry contrasts are "
            "WITHHELD. Substrate-drift outcome, not a codec result.",
        "interpretation": "none; G-S is a substrate-integrity gate, not a measurement",
    }


def retained_byte_fidelity(rows: Iterable[Mapping], suite: Path | None) -> dict:
    """Compare observed G0 bytes against the retained Class-A byte counts.

    This is an INFRASTRUCTURE PRECONDITION, not a frontier classifier.  Only stratum-A
    tracked rows are eligible: the two build-output-under-test slots are intentionally
    excluded because their Linux bytes are not the retained Windows objects.  Any missing
    or byte-different tracked G0 row voids the retained baseline and geometry contrasts
    must be withheld.
    """
    if suite is None:
        return {"status": "NOT-RUN", "decisional": True, "passed": False,
                "note": "no retained suite supplied; baseline fidelity cannot be established"}
    retained = load_retained_bytes(suite)
    out: list[dict] = []
    for r in rows:
        if str(r.get("cell")) != "G0":
            continue
        if str(r.get("identity_role", "")) == "build-output-under-test":
            continue
        cfg = str(r["config_id"])
        row_name = RETAINED_ROW_BY_CONFIG.get(cfg)
        if row_name is None:
            continue
        want = retained.get((row_name, str(r["file"])))
        if want is None:
            out.append({"config_id": cfg, "retained_row": row_name, "file": r["file"],
                        "status": "NOT-IN-RETAINED-SUITE", "decisional": True,
                        "byte_identical": False})
            continue
        got = int(r["complete_bytes"])
        out.append({
            "config_id": cfg,
            "retained_row": row_name,
            "file": r["file"],
            "retained_bytes": want,
            "observed_g0_bytes": got,
            "byte_identical": got == want,
            "delta_bytes": got - want,
            "delta_pct_of_retained": _pct(got - want, want),
            "decisional": True,
        })
    identical = sum(1 for o in out if o.get("byte_identical"))
    compared = len(out)
    differing = compared - identical
    return {
        "role": "retained byte / provenance INFRASTRUCTURE PRECONDITION",
        "decisional": True,
        "passed": compared > 0 and differing == 0,
        "status": "BASELINE-REPRODUCED" if compared > 0 and differing == 0
                  else "BASELINE-VOID (build drift)",
        "retained_suite": str(suite),
        "compared": compared,
        "byte_identical": identical,
        "byte_differing": differing,
        "interpretation_note": "This gate establishes whether retained Class-A tracked rows are "
                               "a valid baseline. It emits no frontier class. Failure withholds "
                               "all geometry contrasts and is reported as build drift.",
        "rows": out,
    }


# ---------------------------------------------------------------------------
# Frontier-vocabulary scanner
# ---------------------------------------------------------------------------

def assert_no_frontier_vocabulary(paths: Iterable[Path]) -> int:
    """Fail if any artifact emits a frontier class token.

    Q2 publishes bytes. It does not classify, so no frontier vocabulary may appear in any
    emitted artifact. A line that DECLARES the prohibition is not an emission and is skipped;
    without that exemption this gate would fail on the constant that defines the forbidden set.
    """
    exempt_files = {"q2_factorial.py"}
    bad: list[str] = []
    count = 0
    for p in paths:
        count += 1
        if p.name in exempt_files:
            continue
        try:
            text = p.read_text(encoding="utf-8", errors="replace")
        except OSError as exc:
            bad.append(f"{p}: unreadable ({exc})")
            continue
        for i, line in enumerate(text.splitlines(), 1):
            if any(marker in line for marker in _VOCAB_DECLARATION_MARKERS):
                continue
            for token in FORBIDDEN_FRONTIER_TOKENS:
                if token in line:
                    bad.append(f"{p}:{i}: emits frontier vocabulary")
                    break
    for b in bad:
        print(b, file=sys.stderr)
    if not bad:
        print(f"frontier-vocabulary scan: clean ({count} artifact(s))")
    return 1 if bad else 0


# ---------------------------------------------------------------------------
# Self-test on hand-written fixture rows. No codec, no corpus, no timing.
# ---------------------------------------------------------------------------

def _self_test() -> int:
    failures: list[str] = []

    def check(name: str, got, want) -> None:
        if got != want:
            failures.append(f"{name}: got {got!r}, want {want!r}")

    # --- factorial algebra, two independent fixtures ------------------------
    r = contrasts_for_group(1000, 900, 880, 995)
    b = r["bytes"]
    check("gate_on", b["geom_effect_gate_on"], -100)
    check("gate_off", b["geom_effect_gate_off"], -115)
    check("at_small", b["gate_effect_at_small"], 5)
    check("at_large", b["gate_effect_at_large"], 20)
    check("interaction_a", b["interaction"], 15)
    check("interaction_identity", r["interaction_identity_holds"], True)
    check("identity_delta", r["interaction_identity_delta"], 0)
    check("interaction_pct", round(r["pct_of_g0_bytes"]["interaction"], 6), 1.5)
    check("interaction_sign", r["sign"]["interaction"], "POSITIVE")

    # all-equal cells -> every contrast exactly zero
    z = contrasts_for_group(500, 500, 500, 500)["bytes"]
    check("zero_gate_on", z["geom_effect_gate_on"], 0)
    check("zero_interaction", z["interaction"], 0)
    check("zero_sign", contrasts_for_group(500, 500, 500, 500)["sign"]["interaction"], "ZERO")

    # a larger-block INCREASE is a valid measurement, not an error, and is not labelled
    up = contrasts_for_group(1000, 1100, 1105, 1002)
    check("increase_geom_gate_on", up["bytes"]["geom_effect_gate_on"], 100)
    check("increase_sign", up["sign"]["geom_effect_gate_on"], "POSITIVE")

    # --- incomplete 2x2 must be reported, never defaulted --------------------
    rows = [
        {"config_id": "mdl-l0.04", "file": "f", "cell": "G0", "complete_bytes": 10,
         "roundtrip_verified": True, "source_bytes": 100},
        {"config_id": "mdl-l0.04", "file": "f", "cell": "G1", "complete_bytes": 9,
         "roundtrip_verified": True, "source_bytes": 100},
        {"config_id": "mdl-l0.04", "file": "f", "cell": "G2", "complete_bytes": 8,
         "roundtrip_verified": True, "source_bytes": 100},
    ]
    out = compute_contrasts(rows)
    check("incomplete_detected", len(out["incomplete_groups"]), 1)
    check("incomplete_no_records", len(out["records"]), 0)

    # --- full group + exact-equality class + twin fidelity ------------------
    rows = []
    for cfg, vals in (("mdl-l0.04", (1000, 900, 880, 995)),
                      ("mdl-l0.00", (1000, 900, 880, 995)),
                      ("shape-l0.04", (1000, 950, 940, 995))):
        for cell, v in zip(CELLS, vals):
            rows.append({"config_id": cfg, "file": "f", "cell": cell, "complete_bytes": v,
                         "roundtrip_verified": True, "source_bytes": 100})
    out = compute_contrasts(rows)
    check("records", len(out["records"]), 3)
    check("totals_stratified", out["corpus_totals"]["stratified"], True)
    for k in ("A_tracked_q0_compatible", "B_build_output_under_test",
              "C_all_13_mixed_discovery_aggregate"):
        check(f"stratum_present:{k}", k in out["corpus_totals"], True)
    check("stratum_A_groups", out["corpus_totals"]["A_tracked_q0_compatible"]["groups"], 3)
    check("stratum_B_groups", out["corpus_totals"]["B_build_output_under_test"]["groups"], 0)
    check("stratum_C_groups", out["corpus_totals"]["C_all_13_mixed_discovery_aggregate"]["groups"], 3)
    eq = out["config_contrast_equivalence"]
    check("identical_vectors", len(eq["config_pairs_with_identical_contrast_vectors"]), 1)
    check("threshold_is_none", out["materiality_threshold"], None)
    check("epsilon_is_none", out["epsilon"], None)
    check("token", out["decision_token"], DECISION_TOKEN)

    twin = out["retained_twin_fidelity"]
    check("twin_group_count", len(twin["twin_groups"]), 2)
    check("twin_rows", len(twin["rows"]), 2)          # two groups for one file
    check("twin_non_decisional", twin["decisional"], False)
    t_mdl = next(r for r in twin["rows"] if r["twin_group"] == "T-MDL")
    check("twin_T_MDL_members", t_mdl["members"], ["mdl-l0.04", "mdl-l0.00"])
    check("twin_T_MDL_identical_G0", t_mdl["per_cell"]["G0"]["identical"], True)
    t_shape = next(r for r in twin["rows"] if r["twin_group"] == "T-SHAPE")
    check("twin_T_SHAPE_incomplete", t_shape.get("status"), "INCOMPLETE")

    # --- fidelity with no retained suite is explicitly NOT-RUN ---------------
    check("fidelity_not_run", retained_byte_fidelity(rows, None)["status"], "NOT-RUN")

    # --- G-S preflight: PASS path, VOID path, and stratum-A-only scoping ------
    gs_rows = [
        {"config_id": "mdl-l0.04", "file": "f", "cell": "G0", "complete_bytes": 1000,
         "identity_role": "tracked"},
        {"config_id": "mdl-l0.04", "file": "g", "cell": "G0", "complete_bytes": 2000,
         "identity_role": "build-output-under-test"},
    ]
    gs = g_s_preflight(gs_rows, None)
    check("gs_no_suite_not_run", gs["status"], "NOT-RUN")
    check("gs_no_suite_blocks", gs["contrasts_emitted"], False)

    # withhold path: G-S supplied and failing -> records emptied
    failing = {"contrasts_emitted": False, "status": "BASELINE-VOID"}
    out_fail = compute_contrasts(rows, g_s=failing)
    check("withheld_flag", out_fail["contrasts_withheld"], True)
    check("withheld_records_empty", out_fail["records"], [])
    check("withheld_reason_set", bool(out_fail["withheld_reason"]), True)

    # pass path: G-S supplied and passing -> records retained
    passing = {"contrasts_emitted": True, "status": "G-S-PASS"}
    out_pass = compute_contrasts(rows, g_s=passing)
    check("not_withheld", out_pass["contrasts_withheld"], False)
    check("records_retained", len(out_pass["records"]), 3)

    # --- producer hold + symbolic capacity surfaces are present ---------------
    check("producer_state_present", "reference_producer_state" in out_pass, True)
    check("capacity_basis_present", "structural_capacity_basis" in out_pass, True)
    check("capacity_formula", out_pass["structural_capacity_basis"]["formula"],
          "min(N, nblocks) / block_count")

    # --- the published vocabulary contains no frontier class -----------------
    for token in FORBIDDEN_FRONTIER_TOKENS:
        check(f"absent:{token}", any(token in json.dumps(out) for _ in [0]), False)

    for f in failures:
        print("FAIL:", f, file=sys.stderr)
    print(f"self-test: {len(failures)} failure(s)")
    return 1 if failures else 0


# ---------------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser(description="Q2 factorial contrasts + retained fidelity")
    ap.add_argument("--self-test", action="store_true")
    ap.add_argument("--rows", type=Path, help="collector JSONL (complete_bytes per cell)")
    ap.add_argument("--retained-suite", type=Path,
                    help="retained Class-A suite CSV, for BYTE/IDENTITY fidelity only")
    ap.add_argument("--out", type=Path)
    ap.add_argument("--preflight-only", action="store_true",
                    help="run ONLY the G-S retained-twin preflight and report pass/fail. "
                         "Intended for the first workflow stage, before any geometry arm.")
    ap.add_argument("--assert-no-frontier-vocabulary", type=Path, nargs="*", default=None)
    args = ap.parse_args()

    if args.self_test:
        return _self_test()

    if args.assert_no_frontier_vocabulary is not None:
        return assert_no_frontier_vocabulary(args.assert_no_frontier_vocabulary)

    if not args.rows:
        ap.error("nothing to do: pass --self-test, --rows, or --assert-no-frontier-vocabulary")

    with args.rows.open(encoding="utf-8") as handle:
        rows = [json.loads(line) for line in handle if line.strip()]
    fidelity = retained_byte_fidelity(rows, args.retained_suite)
    g_s = g_s_preflight(rows, args.retained_suite)
    result = compute_contrasts(rows, g_s=g_s)
    result["retained_byte_fidelity"] = fidelity
    result["g_s_preflight"] = g_s
    text = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)
    if args.preflight_only:
        # First-stage gate: the workflow runs this on G0 alone, before any geometry arm.
        if not g_s.get("contrasts_emitted"):
            print(f"G-S: {g_s.get('status')} -- geometry contrasts withheld", file=sys.stderr)
            return 2
        print(f"G-S: {g_s.get('status')} "
              f"({g_s.get('identical')}/{g_s.get('compared')} identical)", file=sys.stderr)
        return 0
    # Non-zero when the contrasts are withheld, so a caller cannot silently proceed.
    return 0 if not result["contrasts_withheld"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
