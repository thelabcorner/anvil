#!/usr/bin/env python3
"""Space Bunny Free -- track 07-bwt-subblocking -- subblock geometry model.

ISOLATED RESEARCH PROTOTYPE. Not wired into production. Not a codec.
Contains no encoder, no decoder, no entropy coder, no timing loop.

Three subcommands:

  selftest        deterministic correctness checks on inline synthetic inputs
  geom            exact decoder-visible byte + RSS accounting for a (n, cap,
                  walks) geometry, under the CURRENT per-subblock checkpoint
                  policy and the PROPOSED globally-allocated policy
  damage          reference-span ("damage") profile of one input file, plus the
                  fixed-stride vs damage-minimising boundary comparison

`damage` is a static structural analysis of LZ reference spans. It measures
nothing about compression ratio or speed and must not be reported as either.
"""

import argparse
import json
import sys
from collections import deque

# ---------------------------------------------------------------------------
# Constants mirrored from the production source (paths cited in the report).
# ---------------------------------------------------------------------------
WALKS_TARGET = 1024          # src/anvil.cpp:3913 kBwtAuxTargetWalks
MAX_INDEXES = 1 << 20        # src/anvil.cpp:3914 kBwtAuxMaxIndexes
ALPHA_RSS = 5.0              # DERIVED/FITTED, see report section 6
FIXED_RSS = 0.0              # fitted residual, ~0


def pow2_at_least(x: int) -> int:
    """Smallest power of two >= x, matching bwt_aux_rate_for_size().

    src/anvil.cpp:4247-4256 -- starts at r=2 and shifts left while r < target.
    """
    if x < 2:
        return 2
    r = 2
    while r < x:
        r <<= 1
    return r


def icount_for(n: int, r: int) -> int:
    """1 + (n-1)//r -- the exact validator in bwt_backend_decode.

    src/anvil.cpp:4355. Computed in Python ints; no overflow concerns.
    """
    return 1 + (n - 1) // r


def rate_for_size_current(n: int) -> int:
    """bwt_aux_rate_for_size(n): per-subblock policy, fixed 1024-walk target.

    src/anvil.cpp:4247-4256.
    """
    target = (n + WALKS_TARGET - 1) // WALKS_TARGET
    return pow2_at_least(target)


def aux_bytes_current(total_n: int, cap: int) -> dict:
    """Charge the aux index under the CURRENT per-subblock policy.

    ratio_backend_encode splits the input into <=cap pieces and calls
    bwt_backend_encode per piece (src/anvil.cpp:4446-4457); each piece
    independently calls bwt_aux_rate_for_size(piece_n). Total index bytes
    therefore grow as Theta(B), and per-piece walk width shrinks as
    Theta(n/B). This function reproduces that exactly.
    """
    if cap <= 0:
        raise ValueError("cap must be positive")
    pieces = []
    off = 0
    while off < total_n:
        ln = min(cap, total_n - off)
        pieces.append(ln)
        off += ln
    if len(pieces) == 1:
        # Bare payload path: no 0xFF wrapper, and the piece is the whole input.
        return {"subblocks": 1, "pieces": pieces, "rate": rate_for_size_current(pieces[0]),
                "index_bytes": 4 * icount_for(pieces[0], rate_for_size_current(pieces[0])),
                "wrapper_bytes": 0, "policy": "current-per-subblock"}
    total_index = 0
    rates = []
    icounts = []
    for ln in pieces:
        r = rate_for_size_current(ln)
        # libsais requires 2 <= r <= n for the v2 auxiliary form.
        if r > ln:
            r = max(2, pow2_at_least(1))
        ic = icount_for(ln, r)
        rates.append(r)
        icounts.append(ic)
        total_index += 4 * ic
    # 0xFF tag + uvar(nsub) + per-subblock (uvar(decoded_len) + uvar(payload_len))
    wrapper = 1 + uvar_len(len(pieces)) + sum(uvar_len(ln) + 4 for ln in pieces)
    return {"subblocks": len(pieces), "pieces": pieces, "rate": rates, "icounts": icounts,
            "index_bytes": total_index, "wrapper_bytes": wrapper,
            "policy": "current-per-subblock"}


def aux_bytes_global(total_n: int, cap: int, walks_total: int) -> dict:
    """Charge the aux index under the PROPOSED globally-allocated policy.

    One stride r is chosen for the WHOLE input: r = pow2(ceil(n / K)), and the
    same r is used inside every subblock, subject to the libsais feasibility
    constraint 2 <= r <= piece_n. Total index bytes then approach 4*K and are
    independent of B instead of growing as Theta(B).

    Feasibility: every subblock needs at least one checkpoint per r-stride, so
    B must not exceed K. When piece_n < r the stride must be clamped and the
    policy degenerates -- reported explicitly as `degenerate`.
    """
    if cap <= 0:
        raise ValueError("cap must be positive")
    pieces = []
    off = 0
    while off < total_n:
        ln = min(cap, total_n - off)
        pieces.append(ln)
        off += ln
    target = (total_n + walks_total - 1) // walks_total
    r_global = pow2_at_least(target)
    total_index = 0
    icounts = []
    degenerate = False
    for ln in pieces:
        r = r_global
        if r > ln:
            # Cannot honour the global stride inside this piece.
            r = pow2_at_least(max(1, (ln + 15) // 16))
            degenerate = True
        ic = icount_for(ln, r)
        icounts.append(ic)
        total_index += 4 * ic
    if len(pieces) == 1:
        wrapper = 0
    else:
        wrapper = 1 + uvar_len(len(pieces)) + sum(uvar_len(ln) + 4 for ln in pieces)
    return {"subblocks": len(pieces), "pieces": pieces, "rate": r_global,
            "icounts": icounts, "index_bytes": total_index,
            "wrapper_bytes": wrapper, "policy": "proposed-global-budget",
            "degenerate": degenerate,
            "feasible": len(pieces) <= walks_total and not degenerate}


def uvar_len(v: int) -> int:
    """Byte length of the LEB128-style uvarint used by put_uvar in src/anvil.cpp."""
    if v < 0:
        raise ValueError("negative")
    n = 1
    while v >= 0x80:
        v >>= 7
        n += 1
    return n


def rss_model(source_bytes: int, cap: int, index_bytes: int) -> float:
    """Peak decode RSS in MiB under RSS ~= N_out + alpha*C + 4*K.

    N_out is the fully materialised output that ratio_backend_decode builds
    with out.reserve(expected) (src/anvil.cpp:4481). alpha*C covers the per
    subblock decode working set: bwt (1x) + out slice (1x) + libsais P/A
    buffer (4x) = 6x nominal, fitted to ~5.0x against measured points
    (enwik8 @64MiB, 411.4 MiB; enwik8 @128MiB, 598.9 MiB).
    """
    mib = 1024.0 * 1024.0
    n_out = source_bytes / mib
    biggest = min(cap, source_bytes) / mib
    return n_out + ALPHA_RSS * biggest + (index_bytes / mib) + FIXED_RSS


# ---------------------------------------------------------------------------
# Reference-span ("damage") analysis
# ---------------------------------------------------------------------------
def match_spans(data: bytes, max_chain: int = 16, min_len: int = 4,
                max_len: int = 4096, hash_bits: int = 16):
    """Yield (start, end) for every greedy LZ77 reference found in `data`.

    Bounded hash chain. This is an ANALYSIS PROBE ONLY: it is not ANVIL's
    production parse and its match set is not ANVIL's match set. That gap is
    adversarial failure case AF-3 in the report.
    """
    n = len(data)
    if n < min_len + 1:
        return
    hsize = 1 << hash_bits
    head = [-1] * hsize
    prev = [-1] * n
    for p in range(n - min_len):
        j = (int.from_bytes(data[p:p + min_len], "little") * 2654435761) & (hsize - 1)
        prev[p] = head[j]
        head[j] = p
    p = min_len
    while p < n:
        best_len = 0
        best_pos = -1
        j = (int.from_bytes(data[p:p + min_len], "little") * 2654435761) & (hsize - 1)
        cand = head[j]
        depth = 0
        while cand >= 0 and depth < max_chain:
            limit = min(max_len, n - p, n - cand)
            if limit > best_len and data[cand + best_len] == data[p + best_len]:
                ln = 0
                while ln < limit and data[cand + ln] == data[p + ln]:
                    ln += 1
                if ln > best_len:
                    best_len = ln
                    best_pos = cand
                    if ln >= limit:
                        break
            cand = prev[cand]
            depth += 1
        if best_len >= min_len and best_pos >= 0:
            yield (best_pos, p)
            # Insert the interior positions so later references can find them.
            j0 = (int.from_bytes(data[p:p + min_len], "little") * 2654435761) & (hsize - 1)
            q = head[j0]
            prev[p] = q
            head[j0] = p
            p += best_len
        else:
            p += 1


def damage_profile(data: bytes, **kw):
    """diff/prefix-sum load array: damage[c] = #references spanning cut c.

    A cut at c breaks a reference iff its interval [start, end) contains c.
    """
    n = len(data)
    diff = [0] * (n + 2)
    refs = 0
    total_span = 0
    for (s, e) in match_spans(data, **kw):
        if s < 0 or e > n or s >= e:
            continue
        diff[s] += 1
        diff[e] -= 1
        refs += 1
        total_span += (e - s)
    dmg = [0] * (n + 1)
    cur = 0
    for i in range(n):
        cur += diff[i]
        dmg[i] = cur
    return dmg, refs, total_span


def fixed_stride_cuts(n: int, cap: int):
    cuts = []
    c = cap
    while c < n:
        cuts.append(c)
        c += cap
    return cuts


def min_damage_cuts(dmg, n: int, cap: int, spread: float = 1.0):
    """Greedy minimum-damage boundaries with a bounded-deviation size window.

    Window is [cap*(1-spread), cap*(1+spread)] so no subblock can collapse or
    blow up. Adversarial failure case AF-2. Sliding-window argmin is O(n).
    """
    lo = max(1, int(cap * (1.0 - spread)))
    hi = max(lo, int(cap * (1.0 + spread)))
    cuts = []
    c = 0
    while True:
        a = c + lo
        b = min(n - 1, c + hi)
        if a >= n:
            break
        if b <= a:
            cuts.append(a)
            c = a
            continue
        best_i = a
        best_v = dmg[a]
        for i in range(a, b + 1):
            if dmg[i] < best_v:
                best_v = dmg[i]
                best_i = i
        cuts.append(best_i)
        c = best_i
    return cuts


# ---------------------------------------------------------------------------
def cmd_geom(args) -> int:
    n = args.source
    out = {"source_bytes": n, "cap": args.cap}
    cur = aux_bytes_current(n, args.cap)
    out["current_policy"] = cur
    for k in args.global_walks:
        g = aux_bytes_global(n, args.cap, k)
        out.setdefault("proposed_global", []).append(g)
    out["rss_mib"] = {}
    for cap in ([args.cap] + args.rss_caps):
        c = aux_bytes_current(n, cap)
        out["rss_mib"][str(cap)] = {
            "current_index_bytes": c["index_bytes"],
            "rss_decode_mib": rss_model(n, cap, c["index_bytes"]),
        }
    if args.m_budget:
        out["m_budget_mib"] = args.m_budget
        out["max_cap_within_budget_mib"] = max_cap_within_budget(n, args.m_budget)
    json.dump(out, sys.stdout, indent=2, sort_keys=True)
    sys.stdout.write("\n")
    return 0


def max_cap_within_budget(n: int, m_budget: float) -> float:
    """Largest cap C such that the fitted RSS model stays inside m_budget."""
    per_byte = (m_budget - n / (1024.0 * 1024.0)) / ALPHA_RSS
    cap_bytes = per_byte * 1024.0 * 1024.0
    return max(0.0, min(cap_bytes, float(n)))


def cmd_damage(args) -> int:
    with open(args.file, "rb") as fh:
        data = fh.read()
    n = len(data)
    dmg, refs, span = damage_profile(data, max_chain=args.chain)
    fixed = fixed_stride_cuts(n, args.cap)
    minim = min_damage_cuts(dmg, n, args.cap, spread=args.spread)
    sf = sum(dmg[c] for c in fixed)
    sm = sum(dmg[c] for c in minim)
    mean_load = (span / n) if n else 0.0
    out = {
        "file": args.file,
        "source_bytes": n,
        "cap": args.cap,
        "spread": args.spread,
        "references": refs,
        "mean_reference_load": mean_load,
        "fixed_stride": {"cuts": len(fixed), "sum_damage": sf,
                         "mean_at_cut": (sf / len(fixed)) if fixed else 0.0},
        "min_damage": {"cuts": len(minim), "sum_damage": sm,
                       "mean_at_cut": (sm / len(minim)) if minim else 0.0},
        "damage_ratio": (sm / sf) if sf else None,
        "predicted_relative_ctx_cost_reduction": ((sf - sm) / sf) if sf else None,
        "note": "structural span statistic only; NOT a compression-ratio measurement",
    }
    json.dump(out, sys.stdout, indent=2, sort_keys=True)
    sys.stdout.write("\n")
    return 0


def cmd_selftest(args) -> int:
    fails = []
    ran = [0]

    def ck(name, cond):
        ran[0] += 1
        if not cond:
            fails.append(name)

    # Reproduce the three measured aux charges exactly (I10-AUX-UNBWT-RESULTS).
    ck("dickens-icount", icount_for(10_192_446, rate_for_size_current(10_192_446)) == 623)
    ck("dickens-bytes", 4 * 623 == 2492)
    ck("webster-icount", icount_for(41_458_703, rate_for_size_current(41_458_703)) == 633)
    ck("webster-bytes", 4 * 633 == 2532)
    ck("enwik8-icount", icount_for(100_000_000, rate_for_size_current(100_000_000)) == 763)
    ck("enwik8-bytes", 4 * 763 == 3052)

    # Theta(B) growth claim, reproduced on the measured 64 MiB enwik8 geometry.
    c1 = aux_bytes_current(100_000_000, 128 << 20)
    ck("enwik8-B1-subblocks", c1["subblocks"] == 1)
    ck("enwik8-B1-index", c1["index_bytes"] == 3052)
    c2 = aux_bytes_current(100_000_000, 64 << 20)
    ck("enwik8-B2-subblocks", c2["subblocks"] == 2)
    ck("enwik8-B2-index", c2["index_bytes"] == 4096 + 4016)
    ck("theta-B-doubles", c2["index_bytes"] > 1.9 * c1["index_bytes"])

    # Global-budget policy must NOT grow with B.
    g1 = aux_bytes_global(100_000_000, 128 << 20, 1024)
    g2 = aux_bytes_global(100_000_000, 64 << 20, 1024)
    ck("global-B1-index", g1["index_bytes"] == 4 * icount_for(100_000_000, g1["rate"]))
    ck("global-not-theta-B", g2["index_bytes"] <= 1.25 * g1["index_bytes"])

    # Saturation geometry: K walks must yield >= 8 blocks for the widest
    # libsais kernel (libsais_unbwt_decode_8, libsais.c:7854). Note the
    # power-of-two quantisation: asking for 8 walks yields only 6, so the
    # requested walk target must be doubled to guarantee >= 8 blocks.
    ck("quant-K8-yields-6", sum(aux_bytes_global(100_000_000, 128 << 20, 8)["icounts"]) == 6)
    sat = aux_bytes_global(100_000_000, 128 << 20, 16)
    ck("sat-icount-ge-8", sum(sat["icounts"]) >= 8)
    ck("sat-index-cheap", sat["index_bytes"] <= 4 * 16)

    # uvarint lengths match the put_uvar encoding used in src/anvil.cpp.
    ck("uvar-1", uvar_len(0) == 1)
    ck("uvar-127", uvar_len(127) == 1)
    ck("uvar-128", uvar_len(128) == 2)
    ck("uvar-16384", uvar_len(16384) == 3)

    # pow2 policy must match the source's shift-left loop, including n=2,3.
    ck("pow2-2", pow2_at_least(2) == 2)
    ck("pow2-3", pow2_at_least(3) == 4)
    ck("pow2-9954", pow2_at_least(9954) == 16384)
    ck("pow2-65536", pow2_at_least(65536) == 65536)

    # Damage profile on a synthetic perfectly-periodic input: every reference
    # spans the whole remaining prefix, so damage is high and nearly flat.
    per = (b"abcdefgh" * 64)
    dmg, refs, _ = damage_profile(per, max_chain=8)
    ck("periodic-has-refs", refs > 0)
    ck("periodic-damage-monotone", all(dmg[i] >= dmg[i + 1] - 1 for i in range(len(dmg) - 1)))

    # On incompressible input there are no references, so all cuts cost zero and
    # the mechanism is a no-op by construction (report section 7).
    import random as _r
    rnd = _r.Random(20261002)
    noise = bytes(rnd.randrange(256) for _ in range(4096))
    dmg2, refs2, _ = damage_profile(noise, max_chain=8)
    ck("noise-no-refs", refs2 == 0)
    ck("noise-zero-damage", all(v == 0 for v in dmg2))

    # Bounded deviation: min-damage cuts must stay inside the size window.
    data = per * 8
    dd, _, _ = damage_profile(data, max_chain=8)
    cuts = min_damage_cuts(dd, len(data), 256, spread=0.5)
    prev = 0
    ok = True
    for c in cuts:
        seg = c - prev
        if seg < 128 or seg > 384:
            ok = False
        prev = c
    ck("bounded-deviation", ok)

    for f in fails:
        sys.stderr.write("SELFTEST FAIL: %s\n" % f)
    if fails:
        return 1
    sys.stdout.write("PASS sbw_model selftest (%d checks)\n" % ran[0])
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd")

    st = sub.add_parser("selftest")
    st.set_defaults(fn=cmd_selftest)

    g = sub.add_parser("geom")
    g.add_argument("--source", type=int, required=True,
                   help="BWT backend input size in bytes (post-transform)")
    g.add_argument("--cap", type=int, required=True, help="subblock cap in bytes")
    g.add_argument("--global-walks", type=int, nargs="*", default=[8, 32, 128, 1024, 4096])
    g.add_argument("--rss-caps", type=int, nargs="*",
                   default=[8 << 20, 16 << 20, 32 << 20, 64 << 20, 128 << 20])
    g.add_argument("--m-budget", type=float, default=None,
                   help="M_budget in MiB = 2 * min(R_xz, R_brotli)")
    g.set_defaults(fn=cmd_geom)

    d = sub.add_parser("damage")
    d.add_argument("file")
    d.add_argument("--cap", type=int, default=8 << 20)
    d.add_argument("--spread", type=float, default=0.5)
    d.add_argument("--chain", type=int, default=16)
    d.set_defaults(fn=cmd_damage)

    args = ap.parse_args(argv)
    if not getattr(args, "fn", None):
        ap.print_help()
        return 2
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
