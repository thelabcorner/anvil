#!/usr/bin/env python3
"""Track 04 bounded prototype: ENVELOPE arbiter recomputation (offline).

Reads ONE frozen suite CSV and recomputes frontier classification two ways:

  bracket  -- the adopted ruling (RESEARCH_LEDGER.md:4500-4508):
              FRONT-GAP = non-dominated AND exists a bracket of two mutually
              non-dominating reference rows q_lo (smaller+slower) and
              q_hi (larger+faster) with p between them on both axes.
  envelope -- proposed: FRONT-GAP iff non-dominated AND the non-dominance
              margin on the binding axis is smaller than eps_env times the
              local reference spacing at p.

Also measures whether bracket count is monotone in reference-grid size, which
is the property the dense-frontier vehicle silently violates.

This performs NO measurement. It is pure arithmetic over a retained artifact.
"""
from __future__ import annotations

import argparse
import csv
import itertools
import math
import random
from pathlib import Path

REF_PREFIXES = ("brotli-", "zstd-", "xz-")
DEGENERATE_RATIO = 0.95
PLANES = ("encode", "decode")


def load(path: Path) -> dict[str, dict[str, dict[str, float]]]:
    by_file: dict[str, dict[str, dict[str, float]]] = {}
    with path.open(newline="", encoding="utf-8-sig") as handle:
        for row in csv.DictReader(handle):
            name = Path(row["file"].replace("\\", "/")).name
            entry = {
                "codec": row["codec"],
                "ratio": float(row["ratio"]),
                "encode": float(row["encode_MBps"]),
                "decode": float(row["decode_MBps"]),
            }
            by_file.setdefault(name, {})[row["codec"]] = entry
    return by_file


def is_ref(codec: str) -> bool:
    return codec.startswith(REF_PREFIXES)


def dominates(a: dict[str, float], b: dict[str, float], plane: str) -> bool:
    return (
        a["ratio"] <= b["ratio"]
        and a[plane] >= b[plane]
        and (a["ratio"] < b["ratio"] or a[plane] > b[plane])
    )


def bracket_front_gap(p: dict[str, float], refs: list[dict[str, float]], plane: str) -> bool:
    """Adopted ruling: exists q_lo smaller+slower and q_hi larger+faster, mutually
    non-dominating, with p between them on both axes."""
    for lo, hi in itertools.combinations(refs, 2):
        if dominates(lo, hi, plane) or dominates(hi, lo, plane):
            continue
        if lo["ratio"] > hi["ratio"] or lo[plane] > hi[plane]:
            lo, hi = hi, lo
        if (
            lo["ratio"] <= p["ratio"] <= hi["ratio"]
            and lo[plane] <= p[plane] <= hi[plane]
        ):
            return True
    return False


def margins(p: dict[str, float], refs: list[dict[str, float]], plane: str) -> tuple[float, float]:
    """Non-dominance margin of p on each axis, positive where p is better."""
    # ratio margin: how much smaller p is than the fastest reference that beats p's speed
    ratio_m = min(
        (p["ratio"] - q["ratio"] for q in refs if q[plane] >= p[plane]),
        default=math.inf,
    )
    # speed margin: how much faster p is than the smallest reference that beats p's ratio
    speed_m = min(
        (p[plane] - q[plane] for q in refs if q["ratio"] <= p["ratio"]),
        default=math.inf,
    )
    return ratio_m, speed_m


def local_spacing(p: dict[str, float], refs: list[dict[str, float]], plane: str) -> float:
    """Smallest log-ratio/log-speed gap from p to any non-dominated reference row."""
    front = [q for q in refs if not any(dominates(r, q, plane) for r in refs if r is not q)]
    if not front:
        return math.inf
    distances = [
        math.hypot(math.log(p["ratio"] / q["ratio"]), math.log(p[plane] / q[plane]))
        for q in front
        if p["ratio"] > 0 and q["ratio"] > 0 and p[plane] > 0 and q[plane] > 0
    ]
    return min(distances) if distances else math.inf


def classify(by_file: dict, eps_env: float, refs_filter=None) -> dict:
    out = {}
    for fname, codecs in sorted(by_file.items()):
        for plane in PLANES:
            refs = [c for k, c in codecs.items() if is_ref(k) and (refs_filter is None or refs_filter(k))]
            anvil = [c for k, c in codecs.items() if not is_ref(k)]
            cells = []
            for p in anvil:
                if p["ratio"] >= DEGENERATE_RATIO:
                    cells.append((p["codec"], "DEGENERATE", None, None, True))
                    continue
                nd = not any(dominates(q, p, plane) for q in refs)
                if not nd:
                    cells.append((p["codec"], "DOMINATED", None, None, False))
                    continue
                bg = bracket_front_gap(p, refs, plane)
                r_m, s_m = margins(p, refs, plane)
                sp = local_spacing(p, refs, plane)
                binding = min(r_m, s_m)
                env_gap = binding < eps_env * sp if math.isfinite(sp) else True
                cells.append(
                    (p["codec"], "NON_DOMINATED", bg, (binding, sp), env_gap)
                )
            out[(fname, plane)] = cells
    return out


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("suite", type=Path)
    ap.add_argument("--eps", type=float, nargs="*", default=[0.05, 0.10, 0.25, 0.50, 1.00])
    ap.add_argument("--subset-trials", type=int, default=2000)
    args = ap.parse_args()

    by_file = load(args.suite)
    n_files = len(by_file)
    n_refs = len({k for c in by_file.values() for k in c if is_ref(k)})
    print(f"suite            : {args.suite}")
    print(f"files            : {n_files}")
    print(f"reference codecs : {sorted({k for c in by_file.values() for k in c if is_ref(k)})}")
    print(f"distinct refs    : {n_refs}")

    # ---- U4: do bracket and envelope disagree, and at what eps_env? ----
    print("\n=== U4  bracket-FRONT-GAP vs ENVELOPE-GAP over non-dominated cells ===")
    base = classify(by_file, 0.0)
    nd_total = 0
    bg_total = 0
    nd_cells = []
    for key, cells in base.items():
        for codec, cls, bg, margin, _env in cells:
            if cls != "NON_DOMINATED":
                continue
            nd_total += 1
            if bg:
                bg_total += 1
            nd_cells.append((key, codec, bg, margin))
    print(f"non-dominated ANVIL cells (plane-split) : {nd_total}")
    print(f"  bracket FRONT-GAP                     : {bg_total}")
    print(f"  would be CROSSING under bracket rule   : {nd_total - bg_total}")
    print(f"{'eps_env':>8} {'ENVELOPE_GAP':>12} {'CROSSING':>9} {'disagree_vs_bracket':>19}")
    for eps in args.eps:
        res = classify(by_file, eps)
        env_gap = 0
        cross = 0
        for key, cells in res.items():
            for codec, cls, bg, margin, eg in cells:
                if cls != "NON_DOMINATED":
                    continue
                if eg:
                    env_gap += 1
                else:
                    cross += 1
        # disagreement: bracket says GAP, envelope says CROSS (or vice versa)
        dis = []
        for key, cells in res.items():
            for codec, cls, bg, _m, eg in cells:
                if cls == "NON_DOMINATED" and bool(bg) != bool(eg):
                    dis.append((key, codec, bool(bg), bool(eg)))
        print(f"{eps:8.2f} {env_gap:12d} {cross:9d} {len(dis):19d}")
        for key, codec, bg, eg in dis:
            print(f"{'':8} -> {key[0]} / {key[1]} / {codec}: bracket={'GAP' if bg else 'CROSS'} envelope={'GAP' if eg else 'CROSS'}")

    # ---- monotonicity of bracket count in reference-grid size ----
    print(f"\n=== density monotonicity ({args.subset_trials} random reference subsets per size) ===")
    ref_names = sorted({k for c in by_file.values() for k in c if is_ref(k)})
    rng = random.Random(41246)
    print(f"{'k_refs':>7} {'mean_bracket_gap':>17} {'mean_crossing':>14}")
    for k in range(2, len(ref_names) + 1):
        tot_bg = 0.0
        tot_cr = 0.0
        for _ in range(args.subset_trials):
            keep = set(rng.sample(ref_names, k))
            res = classify(by_file, 0.0, refs_filter=lambda c: c in keep)
            bg = sum(1 for cl in res.values() for _c, cls, b, _m, _e in cl if cls == "NON_DOMINATED" and b)
            cr = sum(1 for cl in res.values() for _c, cls, b, _m, _e in cl if cls == "NON_DOMINATED" and not b)
            tot_bg += bg
            tot_cr += cr
        print(f"{k:7d} {tot_bg / args.subset_trials:17.3f} {tot_cr / args.subset_trials:14.3f}")

    # ---- F3: joint-plane verdict, which the vehicle cannot express ----
    print("\n=== F3  joint (both-plane) non-dominance under the bracket rule ===")
    base2 = classify(by_file, 0.0)
    per_codec: dict[tuple[str, str], dict[str, str]] = {}
    for (fname, plane), cells in base2.items():
        for codec, cls, bg, _m, _e in cells:
            per_codec.setdefault((fname, codec), {})[plane] = "NON_DOMINATED" if cls == "NON_DOMINATED" else cls
    both = [k for k, v in per_codec.items() if v.get("encode") == "NON_DOMINATED" and v.get("decode") == "NON_DOMINATED"]
    one = [k for k, v in per_codec.items() if ("NON_DOMINATED" in v.values()) and v.get("encode") != v.get("decode")]
    print(f"non-dominated on BOTH planes : {len(both)}  -> {[f'{f}:{c}' for f, c in both][:8]}")
    print(f"non-dominated on ONE  plane  : {len(one)}")
    for fname, codec in both[:10]:
        e = dict((c, (cl, bg)) for c, cl, bg, _m, _e in next(cells for k, cells in base2.items() if k == (fname, 'encode')))
        d = dict((c, (cl, bg)) for c, cl, bg, _m, _e in next(cells for k, cells in base2.items() if k == (fname, 'decode')))
        print(f"   {fname:<28} {codec:<28} enc={'GAP' if e[codec][1] else 'CROSS'} dec={'GAP' if d[codec][1] else 'CROSS'}")


if __name__ == "__main__":
    main()
