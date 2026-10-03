#!/usr/bin/env python3
"""NAC - Numeric Anatomy Census (track 13, Space Bunny Free).

WHAT THIS IS
    A codec-free, entropy-free, timing-free *anatomy* pass. It does not compress,
    does not decompress, does not invoke any codec, does not time anything, and
    emits no ratio or throughput number of any kind.

WHY
    The typed-numeric-lane direction (CANL) is blocked by a closed-form metadata
    condition, not by an implementation gap. The project's own frozen PORDER-probe
    condition (RESEARCH_LEDGER.md:4828-4829) allows decoder-visible metadata at
    most 20% of gross savings. A per-lane descriptor costs m ~= 2-3 B, so a lane of
    input width w bytes with saving fraction S satisfies the cap only when

        (m / w) / S <= 0.20   <=>   S >= 5 * m / w.

    With m = 2..3 B that demands S >= 125% at w = 4, S >= 62.5% at w = 16, and is
    trivial at w = 1024 (a contiguous column interior). So the decisive unknown is
    not "how good is the predictor" but:

        Q-PREVALENCE: on REAL files, what is the distribution of typed-numeric lane
                      widths w, and what saving fraction S is actually available?
        Q-BAR:        what do transform-enabled published references (xz --delta,
                      Gorilla-class, ALP-class) achieve on those same files?

    This tool answers Q-PREVALENCE. Q-BAR needs reference codecs in the benchmark
    harness (out of scope here, remote-only).

EXECUTION POLICY
    --self-test runs pure arithmetic on in-memory patterns and touches no file.
    Census execution over corpus files is REMOTE-ONLY (GitHub Actions), per
    MASTER-BRIEF item 7. This tool never writes outside its --out path.

DERIVATION LABELS (per docs/gate-verify-regime-i9.md)
    [M] measured in-process (arithmetic over input bytes)   [A] derived arithmetic
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Frozen, non-tuned constants.
# ---------------------------------------------------------------------------

CANDIDATE_LO = 8              # smallest plausible record stride, bytes
CANDIDATE_HI = 64             # largest plausible record stride, bytes
PERIOD_PREFIX = 1 << 15       # bounded cheap pass for candidate voting
PERIOD_VOTE_KEEP = 6          # only these get the expensive scan
MIN_STRUCTURE_GAIN = 0.10     # bits: mean residue-class entropy deficit vs null
MIN_NULL_MARGIN = 0.05        # structure score must beat its own null control
MIN_LANE_SAVING = 0.25        # a lane must be worth this to be reported
MIN_LANE_VALUES = 8
PERIOD_TOLERANCE_BITS = 0.02  # minimality rule for period selection
DESCRIPTOR_BYTES = (2, 3)     # m: min..max per-lane metadata, bytes
METADATA_CAP_OF_GROSS = 0.20  # RESEARCH_LEDGER.md:4828-4829
FIELD_WIDTHS = (2, 4, 8)      # fine-grained interleaved lane widths
WIDE_WIDTHS = (16, 24, 32, 40, 48, 64)   # contiguous-column lane widths
MAX_SCAN = 1 << 20            # bounded bytes scanned per candidate stride


def _entropy(hist) -> float:
    n = sum(hist)
    if n == 0:
        return 0.0
    h = 0.0
    for c in hist:
        if c:
            p = c / n
            h -= p * math.log2(p)
    return h


# ---------------------------------------------------------------------------
# Stage 1 - data-derived period candidates, then null-calibrated structure
# ---------------------------------------------------------------------------

def _period_candidates(buf: bytes, keep: int = PERIOD_VOTE_KEEP) -> list:
    """Cheap, bounded, DATA-DERIVED stride candidates from a period vote. [M]

    A hand-listed stride set is a silent liability: an earlier revision of this
    tool listed (8,12,14,16,20,23,24,28,32,40,44,48,64) and therefore could not
    see the 18-byte record stride of its own reference pattern, reporting 48
    instead. Candidates must come from the data.

    Vote = fraction of positions in a bounded head prefix where byte i equals
    byte i-p. One bounded pass, no residue histograms.
    """
    head = buf[:PERIOD_PREFIX]
    n = len(head)
    if n <= CANDIDATE_HI + 1:
        return []
    votes = []
    for p in range(CANDIDATE_LO, CANDIDATE_HI + 1):
        v = 0
        for i in range(p, n):
            if head[i] == head[i - p]:
                v += 1
        votes.append((v / (n - p), p))
    votes.sort(key=lambda t: (-t[0], t[1]))
    kept = []
    for _, p in votes:
        # Primitivity rule: an exact multiple of a surviving candidate carries the
        # same field structure but fewer samples per residue class, which inflates
        # its apparent entropy deficit. Keep only primitive periods.
        if any(p % q == 0 for q in kept):
            continue
        kept.append(p)
        if len(kept) >= keep:
            break
    return kept


def _structure_score(buf: bytes, L: int):
    """Mean per-residue-class byte entropy deficit against the global entropy. [M]

    A typed-numeric lane is, by construction, a fixed-stride record field. When a
    file really is a packed array of fixed-width records, the byte stream splits
    into L residue classes and the classes belonging to low-entropy fields sit
    below the global byte entropy while mantissa classes sit near it. The mean
    deficit therefore rises exactly when record structure exists.

    This is a *structure* test, not a *predictability* test: deliberately weaker
    than any predictor a lane would use, so it cannot flatter the mechanism.
    """
    n = len(buf)
    if L <= 0 or n < 64 * L:
        return None
    span = min(n, MAX_SCAN)
    gh = [0] * 256
    for b in buf[:span]:
        gh[b] += 1
    g = _entropy(gh)
    rh = [[0] * 256 for _ in range(L)]
    counts = [0] * L
    for i in range(span):
        r = i % L
        rh[r][buf[i]] += 1
        counts[r] += 1
    if min(counts) < 256:           # guard the finite-sample entropy bias
        return None
    mean_h = sum(_entropy(h) for h in rh) / L
    return g - mean_h, g, mean_h


def discover_stride(buf: bytes, strides=None) -> dict:
    """Pick the minimal stride with the strongest null-calibrated structure gain.

    [A] Each candidate L is scored against its own null control at distance L+1,
    because a raw score is meaningless without the noise floor (same null-floor
    discipline the project already uses for detector hit rates,
    RESEARCH_LEDGER.md Experiment U/W: 0.14-1.1% hash-noise floor).

    Minimality: among strides statistically indistinguishable from the best, the
    SMALLEST is taken. Multiples of the true period also carry structure and
    reporting the largest would overstate the lane. The ledger's own lesson is
    that period minimality must be validated - the P=28 vs P=14 alias
    correction, RESEARCH_LEDGER.md:4636-4642.
    """
    if strides is None:
        strides = _period_candidates(buf)
    scored = []
    gh_best = rh_best = 0.0
    for L in strides:
        s = _structure_score(buf, L)
        if s is None:
            continue
        nxt = _structure_score(buf, L + 1)
        null = nxt[0] if nxt else 0.0
        scored.append((L, s[0], null, s[0] - null))
        gh_best, rh_best = s[1], s[2]
    if not scored:
        return {"stride": 0, "structure_gain": 0.0, "null_gain": 0.0,
                "margin": 0.0, "global_entropy": 0.0, "residue_entropy": 0.0,
                "candidates": []}
    top = max(x[3] for x in scored)
    L, gain, null, margin = min((x for x in scored
                                 if x[3] >= top - PERIOD_TOLERANCE_BITS),
                                key=lambda x: x[0])
    return {"stride": L, "structure_gain": round(gain, 5),
            "null_gain": round(null, 5), "margin": round(margin, 5),
            "global_entropy": round(gh_best, 5),
            "residue_entropy": round(rh_best, 5),
            "candidates": [x[0] for x in scored]}


# ---------------------------------------------------------------------------
# Stage 2 - lane anatomy
# ---------------------------------------------------------------------------

def _xor_residual_bits(words):
    """XOR-of-bit-patterns residual width over a sequence of unsigned words. [M]

    Returns (mean bit length of w_i XOR w_{i-1}, Case-A fraction), Case A being
    "the running window width suffices", i.e. the Gorilla cheap regime.

    Regime caveat carried from the adversarial review: a low mean co-occurs with a
    high Case-A fraction, and a lane is cheap only when BOTH hold; quoting one
    without the other is a regime cherry-pick.
    """
    if len(words) < 2:
        return 0.0, 0.0
    widths = [(words[i] ^ words[i - 1]).bit_length() for i in range(1, len(words))]
    cur = widths[0]
    case_a = 0
    for b in widths[1:]:
        if b <= cur:
            case_a += 1
        else:
            cur = b
    return sum(widths) / len(widths), case_a / len(widths)


def _deltas(words):
    """First differences of a word sequence. [M]"""
    if len(words) < 2:
        return []
    return [words[i + 1] - words[i] for i in range(len(words) - 1)]


def _saving(deltas, control_bits: float, w_bytes: int) -> float:
    """S = 1 - (mean payload bits + per-value control bits) / lane input bits."""
    if not deltas:
        return 0.0
    bits = [d.bit_length() if d else 0 for d in deltas]
    s = 1.0 - ((sum(bits) / len(bits) + control_bits) / (8.0 * w_bytes))
    return s if s > 0.0 else 0.0


def _cap(m: int, w: int, S: float) -> bool:
    return S >= 5.0 * m / w


def census_lanes(buf: bytes, stride: int, offsets) -> list:
    """Fine-grained INTERLEAVED lane anatomy for one record stride."""
    units = len(buf) // stride
    lanes = []
    for off in offsets:
        w = stride - off
        if w not in FIELD_WIDTHS:
            continue
        col = bytearray()
        for u in range(units):
            base = u * stride + off
            col += buf[base:base + w]
        raw = bytes(col)
        nvals = len(raw) // w
        if nvals < MIN_LANE_VALUES:
            continue
        ifmt = {2: "h", 4: "i", 8: "q"}[w]
        words = list(struct.unpack("<" + ifmt * nvals, raw))
        d = _deltas(words)
        S_int = _saving(d, 1.0, w)
        ufmt = {4: "I", 8: "Q"}[w] if w in (4, 8) else None
        if ufmt:
            iw = list(struct.unpack("<" + ufmt * nvals, raw))
            xbits, xcasea = _xor_residual_bits(iw)
            S_xor = _saving([(iw[i] ^ iw[i - 1]) for i in range(1, nvals)],
                            2.0, w)
        else:
            xbits = xcasea = 0.0
            S_xor = 0.0
        mode = "int-lane-delta" if S_int >= S_xor else "float-lane-xor"
        S = max(S_int, S_xor)
        lanes.append({
            "offset": off,
            "width_bytes": w,
            "values": nvals,
            "geometry": "interleaved",
            "float_xor_mean_bits": round(xbits, 3),
            "float_case_a_fraction": round(xcasea, 4),
            "int_mean_delta_bits": round(
                sum(x.bit_length() if x else 0 for x in d) / len(d), 3)
            if d else 0.0,
            "int_delta_zero_fraction": round(
                sum(1 for x in d if x == 0) / len(d), 4) if d else 0.0,
            "int_distinct_deltas": len(set(d)),
            "admitted_mode": mode,
            "saving_fraction_S": round(S, 4),
            "cap_feasible": {str(m): _cap(m, w, S) for m in DESCRIPTOR_BYTES},
        })
    return lanes


def census_wide_lane(buf: bytes, stride: int):
    """CONTIGUOUS-COLUMN lane anatomy: the whole record span is one lane. [M]

    This is the only lane geometry the 20%-of-gross-savings metadata cap can
    satisfy at all (w >= 15 B), and it is exactly the geometry where Parquet,
    ORC, ALP, FastLanes and BtrBlocks already operate and where the container
    format already carries a schema.
    """
    if stride not in WIDE_WIDTHS:
        return None
    units = len(buf) // stride
    if units < MIN_LANE_VALUES:
        return None
    col = buf[: units * stride]
    out = {"offset": 0, "width_bytes": stride, "values": units,
           "geometry": "contiguous-column"}
    best = None
    if stride % 8 == 0:
        nw = len(col) // 8
        iw = list(struct.unpack("<" + "Q" * nw, col))
        xbits, xcasea = _xor_residual_bits(iw)
        S = _saving([(iw[i] ^ iw[i - 1]) for i in range(1, nw)], 2.0, 8)
        best = {"mode": "xor-64", "S": S}
        out["xor_mean_bits"] = round(xbits, 3)
        out["case_a_fraction"] = round(xcasea, 4)
    if stride % 4 == 0:
        nw = len(col) // 4
        i32 = list(struct.unpack("<" + "I" * nw, col))
        d32 = _deltas(i32)
        S32 = _saving(d32, 1.0, 4)
        if best is None or S32 > best["S"]:
            best = {"mode": "delta-32", "S": S32}
        out["delta32_mean_bits"] = round(
            sum(x.bit_length() if x else 0 for x in d32) / len(d32), 3)
        out["delta32_zero_fraction"] = round(
            sum(1 for x in d32 if x == 0) / len(d32), 4)
    if best is None:
        return None
    out["admitted_mode"] = best["mode"]
    out["saving_fraction_S"] = round(best["S"], 4)
    out["cap_feasible"] = {str(m): _cap(m, stride, best["S"])
                           for m in DESCRIPTOR_BYTES}
    return out


def census_file(path: Path) -> dict:
    buf = Path(path).read_bytes()
    si = discover_stride(buf)
    res = {
        "file": str(path),
        "bytes": len(buf),
        "stride": si["stride"],
        "stride_candidates": si.get("candidates", []),
        "structure_gain_bits": si["structure_gain"],
        "null_gain_bits": si["null_gain"],
        "structure_margin": si["margin"],
        "global_byte_entropy_bits": si["global_entropy"],
        "mean_residue_entropy_bits": si["residue_entropy"],
        "interleaved_lanes": [],
        "wide_lane": None,
        "prevalence": 0.0,
        "cap_feasible_prevalence": {str(m): 0.0 for m in DESCRIPTOR_BYTES},
        "verdict": "NO_LANE_CANDIDATE",
        "labels": ["[M] in-process arithmetic over file bytes; no codec invoked"],
        "cap_condition": "S >= 5*m/w, m in {2,3} B (20% of gross savings)",
    }
    if (si["stride"] == 0 or si["margin"] < MIN_NULL_MARGIN
            or si["structure_gain"] < MIN_STRUCTURE_GAIN):
        res["verdict"] = "STRIDE_REFUSED"
        res["reason"] = (f"structure margin {si['margin']} / gain "
                         f"{si['structure_gain']} below floors "
                         f"{MIN_NULL_MARGIN} / {MIN_STRUCTURE_GAIN}")
        return res
    res["interleaved_lanes"] = census_lanes(buf, si["stride"],
                                             range(si["stride"]))
    res["wide_lane"] = census_wide_lane(buf, si["stride"])
    total = max(1, len(buf))
    counted = sum(l["width_bytes"] * l["values"]
                  for l in res["interleaved_lanes"])
    res["prevalence"] = round(counted / total, 4)
    for m in DESCRIPTOR_BYTES:
        ok = sum(l["width_bytes"] * l["values"] for l in res["interleaved_lanes"]
                 if l["cap_feasible"][str(m)])
        res["cap_feasible_prevalence"][str(m)] = round(ok / total, 4)
    any_feasible = (
        any(any(l["cap_feasible"].values()) for l in res["interleaved_lanes"])
        or (res["wide_lane"] and any(res["wide_lane"]["cap_feasible"].values())))
    any_real = (
        any(l["saving_fraction_S"] >= MIN_LANE_SAVING
            for l in res["interleaved_lanes"])
        or (res["wide_lane"]
            and res["wide_lane"]["saving_fraction_S"] >= MIN_LANE_SAVING))
    if not res["interleaved_lanes"] and not res["wide_lane"]:
        res["verdict"] = "NO_LANE_CANDIDATE"
    elif any_feasible:
        res["verdict"] = "CAP_FEASIBLE_LANE_EXISTS"
    elif any_real:
        res["verdict"] = "LANES_EXIST_BUT_CAP_BLOCKED"
    else:
        res["verdict"] = "LANES_INEXISTENTIAL"
    return res


# ---------------------------------------------------------------------------
# Self-test: pure arithmetic, in memory, no file access
# ---------------------------------------------------------------------------

def _ts_records(units: int = 4096):
    """{u64 ts @ exact cadence, f64 decimal-drift value, u16 cyclic id} - 18 B."""
    out = bytearray()
    ts = 1_700_000_000_000
    val = 20.0
    for i in range(units):
        ts += 1000
        val += 0.001
        out += struct.pack("<QdH", ts, val, i % 1000)
    return bytes(out)


def _ap_u32(units: int = 16384):
    return b"".join(struct.pack("<I", 1000 + 7 * i) for i in range(units))


def _exponent_ramp_f64(units: int = 4096):
    """Same mantissa, incrementing exponent: the genuinely XOR-friendly regime."""
    return b"".join(struct.pack("<d", 20.0 * (2.0 ** (i // 8)))
                    for i in range(units))


def _decimal_drift_f64(units: int = 2048):
    return b"".join(struct.pack("<d", 20.0 + 0.001 * i) for i in range(units))


def _nonrepeating_random(nbytes: int) -> bytes:
    """Deterministic, genuinely non-periodic bytes (SHA-512 counter mode)."""
    out = bytearray()
    c = 0
    while len(out) < nbytes:
        out += hashlib.sha512(b"nac-rng" + c.to_bytes(8, "little")).digest()
        c += 1
    return bytes(out[:nbytes])


def self_test() -> int:
    fails = []

    def check(name, cond, detail=""):
        if not cond:
            fails.append(f"{name}: {detail}")

    # --- 1. data-derived period discovery finds the true stride --------------
    si = discover_stride(_ts_records())
    check("stride_recovery", si["stride"] == 18,
          f"got {si['stride']} from candidates {si.get('candidates')} "
          f"margin {si['margin']:.4f}")
    check("structure_margin_positive", si["margin"] >= MIN_NULL_MARGIN,
          f"margin {si['margin']:.4f}")

    # --- 2. structure detection refuses genuinely random data ---------------
    si2 = discover_stride(_nonrepeating_random(1 << 18))
    check("structure_refuses_random",
          si2["stride"] == 0 or si2["margin"] < MIN_NULL_MARGIN,
          f"stride {si2['stride']} margin {si2['margin']:.4f}")

    # --- 3. the metadata cap closed form: the load-bearing arithmetic -------
    for w, expect_ok in ((4, False), (8, False), (16, True), (1024, True)):
        for m in DESCRIPTOR_BYTES:
            need = 5.0 * m / w
            check(f"cap_w{w}_m{m}", (need <= 1.0) == expect_ok,
                  f"need S >= {need:.4f}")
    # First width at which the cap is satisfiable at all, per m:
    #   m=2 -> S >= 10/10 = 1.00  -> w >= 10 B
    #   m=3 -> S >= 15/w          -> w >= 15 B
    for m, w_min in ((2, 10), (3, 15)):
        check(f"cap_min_width_m{m}",
              _cap(m, w_min, 1.0) and (w_min == 1 or not _cap(m, w_min - 1, 1.0)),
              f"m={m}: satisfiable first at w={w_min} B")

    # --- 4. integer AP lane: real S at w=4, but cap-BLOCKED -----------------
    ap = _ap_u32()
    d = _deltas(list(struct.unpack("<" + "I" * (len(ap) // 4), ap)))
    check("ap_constant_delta", len(set(d)) == 1 and d[0] == 7,
          f"distinct={len(set(d))} first={d[0]}")
    l0 = next((l for l in census_lanes(ap, 4, range(4))
               if l["width_bytes"] == 4), None)
    check("ap_lane_found_w4", l0 is not None, json.dumps(l0))
    check("ap_lane_real_S", l0 and l0["saving_fraction_S"] >= 0.85,
          json.dumps(l0))
    check("ap_lane_cap_blocked_at_w4", l0 and not l0["cap_feasible"]["2"],
          json.dumps(l0))

    # --- 5. the contiguous-column case IS cap-feasible at w=32 -------------
    wide = census_wide_lane(_ap_u32(), 32)
    check("wide_lane_built", wide is not None, json.dumps(wide))
    check("wide_lane_cap_feasible", wide and wide["cap_feasible"]["2"],
          json.dumps(wide))
    check("wide_lane_S_real", wide and wide["saving_fraction_S"] >= 0.80,
          json.dumps(wide))

    # --- 6. float XOR regime discrimination (measured, not assumed) ---------
    #    6a. decimal-origin drift: XOR is BAD. This is precisely why ALP needs a
    #        decimal-factor transform rather than XOR.
    xb_dec, ca_dec = _xor_residual_bits(
        list(struct.unpack("<" + "Q" * 2048, _decimal_drift_f64())))
    check("decimal_drift_xor_is_bad", xb_dec > 24.0,
          f"mean_xor_bits={xb_dec:.2f} case_a={ca_dec:.3f}")
    #    6b. exponent ramp: XOR is GOOD.
    xb_ramp, ca_ramp = _xor_residual_bits(
        list(struct.unpack("<" + "Q" * 4096, _exponent_ramp_f64())))
    check("exponent_ramp_xor_is_good", xb_ramp <= 16.0,
          f"mean_xor_bits={xb_ramp:.2f} case_a={ca_ramp:.3f}")

    if fails:
        for f in fails:
            print("FAIL " + f)
        return 1
    print("PASS NAC self-test (pure arithmetic, in-memory, no codec, no file)")
    print("  cap condition: (m/w)/S <= 0.20  <=>  S >= 5m/w")
    for w in (4, 8, 16, 32, 64, 1024):
        n2 = 5.0 * 2 / w
        print(f"    w={w:>5} B  S required: m=2 {n2:9.4%}  m=3 "
              f"{5.0 * 3 / w:9.4%}  satisfiable: "
              f"{'yes' if n2 <= 1.0 else 'NO'}")
    print("  measured float XOR regimes (mean XOR bits/value):")
    print(f"    decimal-origin drift 20.0+0.001*i : {xb_dec:6.2f}  "
          f"case_a={ca_dec:.3f}  (XOR BAD)")
    print(f"    exponent ramp 20.0*2^(i//8)      : {xb_ramp:6.2f}  "
          f"case_a={ca_ramp:.3f}  (XOR GOOD)")
    print("  the census reports a regime, never a single float figure")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="NAC numeric anatomy census")
    ap.add_argument("--self-test", action="store_true",
                    help="pure arithmetic self-test; touches no file")
    ap.add_argument("files", nargs="*", help="files to census (REMOTE-ONLY)")
    ap.add_argument("--out", help="write census JSON here")
    a = ap.parse_args(argv)
    if a.self_test:
        return self_test()
    if not a.files:
        ap.error("nothing to do: pass --self-test or one or more files")
    report = [census_file(Path(f)) for f in a.files]
    text = json.dumps(report, indent=2, sort_keys=True)
    if a.out:
        Path(a.out).write_text(text + "\n", encoding="utf-8", newline="\n")
    else:
        print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
