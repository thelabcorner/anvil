#!/usr/bin/env python3
"""
datastruct (I9, follow-up) — AUTO-DISCOVERY of record period + field partition
inside the reference. No oracle: the encoder is given only the byte stream.

Reuses colref.py's coder + field classes (prev/delta/const/posmod/worddelta).
Adds:
  * period discovery  — match-fraction spectrum over a CHARGED search budget,
    then a coding-cost proxy (per-column residual entropy + model penalty) to
    pick P. The aliasing trap (a multiple scoring higher) is rejected by the
    cost proxy, not by a human.
  * partition discovery — dynamic programming over the record template using
    per-byte conditional/delta cost estimates + exact class tests
    (const / posmod) on candidate intervals, with per-field header cost.
  * a CHARGED discovery block in the wire: pmax searched, candidate periods
    actually evaluated by the cost proxy, search flags, DP width.

Wire (all bytes counted):
  'AC2' | mode u8 | P u32 | nrec u32 | pmax u16 | K u8 | K*u16 candidates |
  flags u8 | maxw u8 | nfields u8 | per field (off u16, width u8, class u8, params)
  | arithmetic body

Usage:
  python prototypes/i9-datastruct/auto_colref.py probe  <file> [--pmax N]
  python prototypes/i9-datastruct/auto_colref.py verify <file> <out> [--pmax N]
  python prototypes/i9-datastruct/auto_colref.py compare <file> [--pmax N]
"""
from __future__ import annotations
import argparse
import math
import os
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import colref  # noqa: E402
from colref import (C_CONST, C_POSMOD, C_PREV, C_DELTA, C_WORDDELTA,  # noqa: E402
                    C_NAMES, FieldCoder, RangeEncoder, RangeDecoder,
                    enc_byte, dec_byte, ByteTree, enc_sym, dec_sym, new_trees,
                    field_value, val_bytes, try_const, try_posmod,
                    entropy_bits)

MAGIC = b"AC2"
MAXW = 8
HDRF_BITS = 48          # per-field header cost charged in the DP proxy
LAM_BITS_PER_COL = 48   # model/adaptation penalty per record column


# ---------------------------------------------------------------------------
# Period discovery
# ---------------------------------------------------------------------------
def match_fractions(b: np.ndarray, pmax: int) -> np.ndarray:
    n = len(b)
    m = np.zeros(pmax + 1)
    for p in range(1, pmax + 1):
        m[p] = np.count_nonzero(b[p:] == b[:-p]) / (n - p)
    return m


def proxy_cost(b: np.ndarray, p: int) -> float:
    """Coding-cost proxy in BITS for a fixed-period reference copy at p.

    Per record column j: empirical H0 of the byte delta vs the previous record
    (order-0 model the coder could reach), times the number of records; plus a
    per-column model/adaptation penalty, plus a literal charge for the tail
    bytes (n mod p). Lower is better.
    """
    n = len(b)
    nrec = n // p
    if nrec < 4:
        return float("inf")
    trunc = nrec * p
    cols = b[:trunc].reshape(nrec, p).astype(np.int16)
    d = (cols[1:] - cols[:-1]) & 0xFF
    total = 0.0
    for j in range(p):
        cnt = np.bincount(d[:, j], minlength=256).astype(np.float64)
        nz = cnt[cnt > 0]
        h = float(-(nz * np.log2(nz / (nrec - 1))).sum())  # bits, all records
        total += h
    total += LAM_BITS_PER_COL * p
    total += (n - trunc) * 8.0  # tail literals
    return total


def discover_period(b: np.ndarray, pmax: int = 1024, K: int = 24, pmin: int = 2):
    """Return (P, candidates, costs, pmax). Candidates are charged in the wire."""
    m = match_fractions(b, pmax)
    order = [int(p) for p in (np.argsort(-m[1:]) + 1)]
    cand, seen = [], set()

    def add(p):
        if pmin <= p <= pmax and p not in seen:
            seen.add(p)
            cand.append(p)

    for p in order[: K // 2]:          # top peaks by match fraction
        add(p)
    for p in list(cand)[:4]:           # aliasing guard: their divisors too
        for q in range(pmin, p + 1):
            if p % q == 0 and len(cand) < K:
                add(q)
    costs = {p: proxy_cost(b, p) for p in cand}
    best = min(costs, key=lambda p: (costs[p], p))
    return best, sorted(cand), costs, pmax


# ---------------------------------------------------------------------------
# Partition discovery
# ---------------------------------------------------------------------------
def per_position_costs(b: np.ndarray, p: int):
    """cost_prev[j], cost_delta[j] in bits per record (empirical H0 proxies)."""
    n = len(b)
    nrec = n // p
    trunc = nrec * p
    cols = b[:trunc].reshape(nrec, p).astype(np.int16)
    cp = np.zeros(p)
    cd = np.zeros(p)
    for j in range(p):
        col = cols[:, j]
        # H0 of the byte-delta
        dd = (col[1:] - col[:-1]) & 0xFF
        cnt = np.bincount(dd, minlength=256).astype(np.float64)
        nz = cnt[cnt > 0]
        cd[j] = float(-(nz * np.log2(nz / (nrec - 1))).sum())
        # conditional entropy H(x_i | x_{i-1}) per column
        joint = np.bincount(col[1:].astype(int) * 256 + col[:-1].astype(int),
                            minlength=65536).reshape(256, 256)
        rs = joint.sum(axis=1, keepdims=True)
        with np.errstate(divide="ignore", invalid="ignore"):
            pj = np.where(joint > 0, joint / np.where(rs == 0, 1, rs), 0)
        cp[j] = float(-(joint * np.log2(np.where(pj > 0, pj, 1))).sum() / (nrec - 1))
    return cp, cd


def _fast_const(b: np.ndarray, p: int, nrec: int, off: int, w: int) -> bool:
    arr = b[:nrec * p].reshape(nrec, p)[:, off:off + w]
    return bool((arr == arr[0]).all())


def _prefix_posmod_ok(data: bytes, p: int, nrec: int, off: int, w: int,
                      sample: int = 128) -> bool:
    """Cheap prefix test for the posmod class (full verify happens after)."""
    n = min(sample, nrec)
    if n < 3:
        return False
    v0 = field_value(data, p, 0, off, w)
    v1 = field_value(data, p, 1, off, w)
    step = v1 - v0
    if step == 0:
        return False
    g = 0
    for i in range(n):
        g = math.gcd(g, abs(field_value(data, p, i, off, w) - v0 - i * step))
        if g == 1:
            return False
    return g != 1  # g==0 (exact linear prefix) or g>1 (periodic prefix)


def discover_partition(data: bytes, p: int, maxw: int = MAXW):
    """DP segmentation of [0,p) into fields with classes const/posmod/prev/delta.

    Body-cost proxy: additively per byte (prev/delta), 0 for exact classes;
    plus HDRF_BITS per field. Exact classes are pre-filtered on a 128-record
    prefix and then FULLY verified, so the returned classes are exact.
    """
    b = np.frombuffer(data, np.uint8)
    n = len(b)
    nrec = n // p
    cp, cd = per_position_costs(b, p)
    per_byte_body = [min(cp[j], cd[j]) * nrec for j in range(p)]  # bits
    INF = float("inf")
    dp = [INF] * (p + 1)
    back = [None] * (p + 1)
    dp[0] = 0.0
    for e in range(1, p + 1):
        for w in range(1, min(maxw, e) + 1):
            s = e - w
            if dp[s] == INF:
                continue
            off, width = s, w
            cls_best = None
            cost_best = INF
            if _fast_const(b, p, nrec, off, width):
                cls_best, cost_best = C_CONST, dp[s] + HDRF_BITS + 8 * width
            elif _prefix_posmod_ok(data, p, nrec, off, width):
                params = try_posmod(data, p, nrec, off, width)
                if params is not None:
                    cls_best, cost_best = C_POSMOD, dp[s] + HDRF_BITS + 24 * 8
            if cls_best is None:
                body = sum(per_byte_body[s:e])
                for cls in (C_PREV, C_DELTA):
                    c = dp[s] + HDRF_BITS + body
                    if c < cost_best:
                        cls_best, cost_best = cls, c
            if cost_best < dp[e]:
                dp[e] = cost_best
                back[e] = (s, cls_best)
    fields = []
    e = p
    while e > 0:
        s, cls = back[e]
        width = e - s
        if cls == C_CONST:
            params = (field_value(data, p, 0, s, width),)
        elif cls == C_POSMOD:
            params = try_posmod(data, p, nrec, s, width)
        else:
            params = None
        fields.append((s, width, cls, params))
        e = s
    fields.reverse()
    return fields


# ---------------------------------------------------------------------------
# Wire
# ---------------------------------------------------------------------------
def pack_header(P, nrec, total_n, pmax, cand, maxw, fields):
    out = bytearray(MAGIC)
    out.append(0)                                   # mode 0 = fixed-period
    out += struct.pack("<III", P, nrec, total_n)
    out += struct.pack("<HB", pmax, len(cand))
    for p in cand:
        out += struct.pack("<H", p)
    out.append(0)                                   # flags (reserved)
    out.append(maxw)
    out.append(len(fields))
    for (off, width, cls, params) in fields:
        out += struct.pack("<HBB", off, width, cls)
        if cls == C_CONST:
            out += val_bytes(params[0], width)
        elif cls == C_POSMOD:
            out += struct.pack("<QqQ", params[0], params[1], params[2])
    return bytes(out)


def parse_header(wire: bytes):
    assert wire[:3] == MAGIC, "bad magic"
    pos = 3
    mode = wire[pos]; pos += 1
    P, nrec, total_n = struct.unpack_from("<III", wire, pos); pos += 12
    pmax, K = struct.unpack_from("<HB", wire, pos); pos += 3
    cand = list(struct.unpack_from("<%dH" % K, wire, pos)) if K else []
    pos += 2 * K
    flags = wire[pos]; pos += 1
    maxw = wire[pos]; pos += 1
    nf = wire[pos]; pos += 1
    fields = []
    for _ in range(nf):
        off, width, cls = struct.unpack_from("<HBB", wire, pos); pos += 4
        params = None
        if cls == C_CONST:
            params = (int.from_bytes(wire[pos:pos + width], "little"),); pos += width
        elif cls == C_POSMOD:
            params = struct.unpack_from("<QqQ", wire, pos); pos += 24
        fields.append((off, width, cls, params))
    return mode, P, nrec, total_n, pmax, cand, flags, maxw, fields, pos


def encode_auto(data: bytes, pmax: int = 1024, K: int = 16):
    b = np.frombuffer(data, np.uint8)
    P, cand, costs, pmax = discover_period(b, pmax=pmax, K=K)
    nrec = len(data) // P
    fields = discover_partition(data, P)
    hdr = pack_header(P, nrec, len(data), pmax, cand, MAXW, fields)
    enc = RangeEncoder()
    coders = [FieldCoder(off, w, cls, params) for (off, w, cls, params) in fields]
    for c in coders:
        c.enc_first(enc, data, P)
    for i in range(1, nrec):
        for c in coders:
            c.enc_record(enc, data, P, i)
    tail = data[nrec * P:]
    if tail:
        t = ByteTree()
        for byte in tail:
            enc_byte(enc, t, byte)
    enc.flush()
    return hdr + bytes(enc.out), P, fields


def decode_auto(wire: bytes):
    (mode, P, nrec, total_n, pmax, cand, flags, maxw, fields,
     hpos) = parse_header(wire)
    dec = RangeDecoder(wire[hpos:])
    coders = [FieldCoder(off, w, cls, params) for (off, w, cls, params) in fields]
    out = bytearray()
    for c in coders:
        c.dec_first(dec, out, P)
    for i in range(1, nrec):
        for c in coders:
            c.dec_record(dec, out, P, i)
    tail_len = total_n - nrec * P
    if tail_len:
        t = ByteTree()
        for _ in range(tail_len):
            out.append(dec_byte(dec, t))
    return bytes(out)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd", choices=["probe", "verify", "compare"])
    ap.add_argument("inp")
    ap.add_argument("out", nargs="?")
    ap.add_argument("--pmax", type=int, default=1024)
    a = ap.parse_args()
    data = open(a.inp, "rb").read()
    b = np.frombuffer(data, np.uint8)
    if a.cmd == "probe":
        P, cand, costs, pmax = discover_period(b, pmax=a.pmax)
        print(f"{os.path.basename(a.inp)} n={len(data)} pmax={pmax} K={len(cand)}")
        for p in cand:
            print(f"  p={p:6d} proxy={costs[p]:14.0f} bits "
                  f"({costs[p]/8:12.0f} B) match={np.count_nonzero(b[p:]==b[:-p])/(len(b)-p):.4f}")
        print(f"  -> chosen P={P}")
        fields = discover_partition(data, P)
        print("  partition:", ",".join(f"{o}:{w}:{C_NAMES[c]}" for (o, w, c, _) in fields))
        return
    if a.cmd == "verify":
        wire, P, fields = encode_auto(data, pmax=a.pmax)
        back = decode_auto(wire)
        ok = back == data
        if a.out:
            open(a.out, "wb").write(wire)
        desc = ",".join(f"{o}:{w}:{C_NAMES[c]}" for (o, w, c, _) in fields)
        print(f"{os.path.basename(a.inp)}: bytes={len(wire)} ratio={len(wire)/len(data):.4f} "
              f"P={P} roundtrip={'OK' if ok else 'FAIL'}")
        print(f"  partition: {desc}")
        if not ok:
            raise SystemExit(1)
        return
    if a.cmd == "compare":
        P, cand, costs, pmax = discover_period(b, pmax=a.pmax)
        nrec = len(data) // P
        trunc = nrec * P
        tail = len(data) - trunc
        fields = discover_partition(data, P)
        part = [(o, w) for (o, w, c, _) in fields]
        rows = []
        wire0 = colref.encode(data, "raw_o0")
        rows.append(("raw_o0 (no mechanism)", len(wire0), ""))
        wire1 = colref.encode(data, "raw_o1")
        rows.append(("raw_o1 (no mechanism)", len(wire1), ""))
        # supplied ladder: P supplied, byte-column partition (naive) / auto partition
        pre = data[:trunc]
        wcol = colref.encode(pre, "ref_col", P=P)
        rows.append((f"supplied P={P}, byte-columns", len(wcol) + tail,
                     f"(trunc {trunc}+raw tail {tail})"))
        wfld = colref.encode(pre, "ref_field", P=P, layout=part)
        rows.append((f"supplied P={P}, auto partition", len(wfld) + tail,
                     f"(trunc {trunc}+raw tail {tail})"))
        wire_auto, _, _ = encode_auto(data, pmax=a.pmax)
        rows.append(("AUTO (charged discovery, full file)", len(wire_auto), ""))
        print(f"{os.path.basename(a.inp)} n={len(data)} discovered P={P} "
              f"K={len(cand)} pmax={pmax} nrec={nrec} tail={tail}")
        print(f"{'variant':46s} {'bytes':>9s} {'ratio':>7s}  note")
        for name, nb, note in rows:
            print(f"{name:46s} {nb:9d} {nb/len(data):7.4f}  {note}")
        print("partition:", ",".join(f"{o}:{w}:{C_NAMES[c]}" for (o, w, c, _) in fields))
        return
    raise SystemExit("bad cmd")


if __name__ == "__main__":
    main()
