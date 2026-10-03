#!/usr/bin/env python3
"""Track 04 reconciliation: retained-artifact parity sweep (T2, T3).

Answers the adversarial review's verdict-flipping objection:
  "your sweep varied the REFERENCE set while holding the CANDIDATE pinned at one
   configuration. A candidate that is handicapped loses every dominance comparison
   by construction, so mean_crossing = 0.000 would be true and irrelevant."

T2  bracket predicate WITH epsilon (the adopted R-2 predicate, materialised)
T3' candidate-side configuration as the swept variable, bracket predicate fixed
    -> literal T3 (candidate BLOCK SIZE) is NOT computable from a retained CSV;
       the computable analogue sweeps the candidate's own configuration ladder.

No codec. No timer. No network. Reads one frozen CSV.
"""
from __future__ import annotations

import argparse
import csv
import itertools
from pathlib import Path

REF_PREFIXES = ("brotli-", "zstd-", "xz-")
DEGENERATE_RATIO = 0.95
BROTLI_LADDER = ["brotli-q1", "brotli-q4", "brotli-q6", "brotli-q9", "brotli-q11"]
ZSTD_LADDER = ["zstd-1", "zstd-3", "zstd-9", "zstd-19"]
PLANES = ("encode", "decode")


def load(path: Path) -> dict[str, dict[str, dict]]:
    by_file: dict[str, dict[str, dict]] = {}
    with path.open(newline="", encoding="utf-8-sig") as handle:
        for row in csv.DictReader(handle):
            name = Path(row["file"].replace("\\", "/")).name
            by_file.setdefault(name, {})[row["codec"]] = {
                "codec": row["codec"],
                "ratio": float(row["ratio"]),
                "encode": float(row["encode_MBps"]),
                "decode": float(row["decode_MBps"]),
            }
    return by_file


def dom(a, b, plane) -> bool:
    return a["ratio"] <= b["ratio"] and a[plane] >= b[plane] and (
        a["ratio"] < b["ratio"] or a[plane] > b[plane]
    )


def bracket_gap(p, refs, plane, eps: float) -> bool:
    """Adopted R-2 bracket predicate, with a material-epsilon requirement so a
    0.1% gap cannot manufacture a FRONT-GAP label."""
    for lo, hi in itertools.combinations(refs, 2):
        if dom(lo, hi, plane) or dom(hi, lo, plane):
            continue
        if lo["ratio"] > hi["ratio"] or lo[plane] > hi[plane]:
            lo, hi = hi, lo
        if (
            p["ratio"] - lo["ratio"] >= eps
            and hi["ratio"] - p["ratio"] >= eps
            and p[plane] - lo[plane] >= eps
            and hi[plane] - p[plane] >= eps
        ):
            return True
    return False


def classify_cell(p, refs, plane, eps):
    """Adopted classification order (RESEARCH_LEDGER.md:4508):
    DOMINATED -> DEGENERATE -> FRONT-GAP -> FRONT-CROSSING.
    DEGENERATE is ratio >= 0.95 (gate-ruling-i8-pareto-win.md / brief strategy PR-2).
    Omitting it silently promotes incompressible rows to CROSSING."""
    if any(dom(q, p, plane) for q in refs):
        return "DOMINATED"
    if p["ratio"] >= DEGENERATE_RATIO:
        return "DEGENERATE"
    return "FRONT-GAP" if bracket_gap(p, refs, plane, eps) else "CROSSING"


def sweep_candidates(by_file, candidate_names, ref_names, eps):
    """For each candidate rung, tally every adopted class over the corpus.
    Reports EVERY rung; no cell is ranked, selected, or suppressed."""
    tally = {c: {"DOMINATED": 0, "DEGENERATE": 0, "FRONT-GAP": 0, "CROSSING": 0} for c in candidate_names}
    for codecs in by_file.values():
        refs = [codecs[c] for c in ref_names if c in codecs]
        for c in candidate_names:
            if c not in codecs:
                continue
            p = codecs[c]
            for plane in PLANES:
                tally[c][classify_cell(p, refs, plane, eps)] += 1
    return tally


def anvil_ladder(by_file, eps):
    anvil = sorted(
        {k for codecs in by_file.values() for k in codecs if not k.startswith(REF_PREFIXES)}
    )
    refs_all = sorted({k for codecs in by_file.values() for k in codecs if k.startswith(REF_PREFIXES)})
    return sweep_candidates(by_file, anvil, refs_all, eps), anvil, refs_all


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("suite", type=Path)
    ap.add_argument("--eps", type=float, nargs="*", default=[0.0, 0.005, 0.01, 0.02, 0.05])
    args = ap.parse_args()
    by_file = load(args.suite)
    all_refs = sorted({k for c in by_file.values() for k in c if k.startswith(REF_PREFIXES)})
    print(f"suite      : {args.suite}")
    print(f"files      : {len(by_file)}")
    print(f"references : {all_refs}")
    print("\nNOTE: literal T3 (sweep candidate BLOCK SIZE) is NOT computable from a retained CSV.")
    print("      The frozen suite holds exactly one ANVIL configuration per codec, all at")
    print("      256 KiB (bench_native.cpp:25 never assigns o.block_size; anvil.cpp:3769 default).")
    print("      T3' below sweeps the candidate's own CONFIGURATION LADDER instead, which tests")
    print("      the identical logical question: is 0-crossing a property of the FRONT, or of the")
    print("      candidate being pinned at one rung?\n")

    # ---------------- T2: bracket predicate with epsilon ----------------
    print("=== T2  adopted bracket predicate, material-epsilon sweep, full ANVIL ladder ===")
    print(f"{'eps':>7} {'DOMINATED':>10} {'FRONT-GAP':>10} {'CROSSING':>9}   verdict")
    lad, anvil_names, _ = anvil_ladder(by_file, 0.0)
    for eps in args.eps:
        lad_e, _, _ = anvil_ladder(by_file, eps)
        d = sum(v["DOMINATED"] for v in lad_e.values())
        g = sum(v["FRONT-GAP"] for v in lad_e.values())
        c = sum(v["CROSSING"] for v in lad_e.values())
        verdict = "KILL 'bracket already satisfied'" if c == 0 else "CHANGES CLASSIFICATION"
        print(f"{eps:7.3f} {d:10d} {g:10d} {c:9d}   {verdict}")

    # ---------------- T3': candidate ladder, disjoint splits ----------------
    splits = [
        ("A  candidate=brotli ladder, refs={zstd*,xz}",
         BROTLI_LADDER, [c for c in all_refs if not c.startswith("brotli-")]),
        ("B  candidate=zstd ladder,  refs={brotli*,xz}",
         ZSTD_LADDER, [c for c in all_refs if not c.startswith("zstd-")]),
        ("C  candidate=xz only,       refs={brotli*,zstd*}",
         ["xz-9e"], [c for c in all_refs if c != "xz-9e"]),
    ]
    for eps in (0.0, 0.01):
        print(f"\n=== T3' DIAGNOSTIC ONLY (not decisive for the ANVIL question) ===")
        print(f"    candidate-side configuration as the swept variable, eps={eps}")
        for label, cands, refs in splits:
            print(f"-- {label}")
            tally = sweep_candidates(by_file, cands, refs, eps)
            print(f"{'candidate rung':>18} {'DOM':>5} {'DEGEN':>6} {'GAP':>5} {'CROSS':>6}")
            for c in cands:
                v = tally[c]
                print(f"{c:>18} {v['DOMINATED']:5d} {v['DEGENERATE']:6d} {v['FRONT-GAP']:5d} {v['CROSSING']:6d}")
            tot = {k: sum(v[k] for v in tally.values()) for k in ("DOMINATED", "DEGENERATE", "FRONT-GAP", "CROSSING")}
            anycross = tot["CROSSING"] > 0
            print(f"{'TOTAL':>18} {tot['DOMINATED']:5d} {tot['DEGENERATE']:6d} {tot['FRONT-GAP']:5d} {tot['CROSSING']:6d}"
                  f"   -> reference-front diagnostic only; does NOT adjudicate ANVIL")

    # ---------------- candidate-configuration sensitivity at fixed window ----------------
    print("\n=== ALL ANVIL configs at the frozen 256 KiB window — FULL listing, no best-cell selection ===")
    print("(one-bright-cell rule: every tested cell is reported; nothing is ranked or cherry-picked)")
    lad0, anvil_names, _ = anvil_ladder(by_file, 0.0)
    print(f"{'anvil config':>30} {'DOM':>5} {'DEGEN':>6} {'GAP':>5} {'CROSS':>6}")
    for name in anvil_names:
        v = lad0[name]
        print(f"{name:>30} {v['DOMINATED']:5d} {v['DEGENERATE']:6d} {v['FRONT-GAP']:5d} {v['CROSSING']:6d}")
    tot = {k: sum(v[k] for v in lad0.values()) for k in ("DOMINATED", "DEGENERATE", "FRONT-GAP", "CROSSING")}
    print(f"{'TOTAL (' + str(len(anvil_names)) + ' configs)':>30} {tot['DOMINATED']:5d} {tot['DEGENERATE']:6d} {tot['FRONT-GAP']:5d} {tot['CROSSING']:6d}")
    cells = sum(lad0[n]["DOMINATED"] + lad0[n]["DEGENERATE"] + lad0[n]["FRONT-GAP"] + lad0[n]["CROSSING"] for n in anvil_names)
    print(f"total cells swept: {cells}  (13 files x 2 planes x 18 configs = {13*2*18})")
    print(f"configs producing >=1 CROSSING: {sum(1 for n in anvil_names if lad0[n]['CROSSING'] > 0)} of {len(anvil_names)}")


if __name__ == "__main__":
    main()
