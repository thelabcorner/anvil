#!/usr/bin/env python3
"""
datastruct (I9) — COLREF: columnar reference-copy coder, counted wire.

MECHANISM (the open capability gap, per RESEARCH_LEDGER PART XIII S8)
----------------------------------------------------------------------
A record-period reference token: record i is reconstructed by COPYING record
i-P (distance = P bytes, the record period) and coding only the bytes that
change, where the correction model is conditioned BOTH on the byte column
(offset within the record) and on the reference byte at that column. The
record layout is never transposed: the decoder emits row-interleaved output,
so contiguous phrase structure is preserved (the retired global lane-transpose
is a different placement). P is supplied as a framing constant and CHARGED in
the counted header; P detection is infrastructure, not claimed.

Placement discipline: this is a PROTOTYPE wire, not an ANVIL row. The range
coder here is the same LZMA-style binary arithmetic coder as
prototypes/i8-datastruct/model_phrase.py.

MODES (the ablation ladder; all share ONE coder, so differences are mechanism)
  raw_o0      order-0 adaptive byte tree over the raw file        (no mechanism)
  raw_o1      order-1 adaptive byte tree (context = prev byte)     (no mechanism)
  ref_flat    record reference, SINGLE global model keyed only by
              the reference byte (no column identity)              (mechanism-lite)
  ref_col     record reference, per-COLUMN model keyed by ref byte (mechanism)
  ref_field   ref_col + per-field class selection:
              CONST / POSMOD / PREV_BYTES / DELTA_BYTES / WORDDELTA

Everything (header, framing constants, class parameters) is counted.

Usage:
  python prototypes/i9-datastruct/colref.py encode <in> <out> --mode MODE
  python prototypes/i9-datastruct/colref.py verify <in> <out> --mode MODE
  python prototypes/i9-datastruct/colref.py sweep  <in>            # all modes
"""
from __future__ import annotations
import argparse
import math
import struct
import sys
from collections import Counter

KBITS = 11
BIT_MODEL_TOTAL = 1 << KBITS
TOP = 1 << 24
MASK32 = 0xFFFFFFFF
INIT = BIT_MODEL_TOTAL >> 1


# ---------------------------------------------------------------------------
# Range coder (LZMA-style, carry-correct) — same as i8 model_phrase.py
# ---------------------------------------------------------------------------
class RangeEncoder:
    __slots__ = ("low", "rng", "cache", "cache_size", "out")

    def __init__(self):
        self.low = 0
        self.rng = MASK32
        self.cache = 0
        self.cache_size = 1
        self.out = bytearray()

    def _shift_low(self):
        if (self.low & MASK32) < 0xFF000000 or (self.low >> 32) != 0:
            temp = self.cache
            carry = (self.low >> 32) & 0xFF
            for _ in range(self.cache_size):
                self.out.append((temp + carry) & 0xFF)
                temp = 0xFF
            self.cache = (self.low >> 24) & 0xFF
            self.cache_size = 0
        self.cache_size += 1
        self.low = (self.low << 8) & MASK32

    def bit(self, state: int, b: int) -> int:
        bound = (self.rng >> KBITS) * state
        if b == 0:
            self.rng = bound
            state += (BIT_MODEL_TOTAL - state) >> 5
        else:
            self.low += bound
            self.rng -= bound
            state -= state >> 5
        while self.rng < TOP:
            self.rng = (self.rng << 8) & MASK32
            self._shift_low()
        return state

    def flush(self):
        for _ in range(5):
            self._shift_low()

    def nbytes(self) -> int:
        return len(self.out)


class RangeDecoder:
    __slots__ = ("low", "rng", "code", "data", "pos")

    def __init__(self, data: bytes):
        self.data = data
        self.pos = 0
        self.low = 0
        self.rng = MASK32
        self.code = 0
        for _ in range(5):
            self.code = ((self.code << 8) | self._nb()) & MASK32

    def _nb(self) -> int:
        if self.pos < len(self.data):
            b = self.data[self.pos]
            self.pos += 1
            return b
        return 0

    def bit(self, state: int):
        bound = (self.rng >> KBITS) * state
        if self.code < bound:
            self.rng = bound
            state += (BIT_MODEL_TOTAL - state) >> 5
            b = 0
        else:
            self.code -= bound
            self.low += bound
            self.rng -= bound
            state -= state >> 5
            b = 1
        while self.rng < TOP:
            self.rng = (self.rng << 8) & MASK32
            self.code = ((self.code << 8) | self._nb()) & MASK32
        return state, b


# ---------------------------------------------------------------------------
# Byte bit-tree (8 levels, 255 states) and context containers
# ---------------------------------------------------------------------------
class ByteTree:
    __slots__ = ("p",)

    def __init__(self):
        self.p = [INIT] * 256


def enc_byte(enc: RangeEncoder, t: ByteTree, b: int) -> None:
    node = 1
    p = t.p
    for i in range(7, -1, -1):
        bit = (b >> i) & 1
        p[node] = enc.bit(p[node], bit)
        node = (node << 1) | bit


def dec_byte(dec: RangeDecoder, t: ByteTree) -> int:
    node = 1
    p = t.p
    for _ in range(8):
        p[node], bit = dec.bit(p[node])
        node = (node << 1) | bit
    return node & 0xFF


def enc_sym(enc, trees, ctx: int, b: int):
    enc_byte(enc, trees[ctx], b)


def dec_sym(dec, trees, ctx: int) -> int:
    return dec_byte(dec, trees[ctx])


def new_trees() -> list:
    return [ByteTree() for _ in range(256)]


# ---------------------------------------------------------------------------
# Field classes
# ---------------------------------------------------------------------------
C_CONST = 0
C_POSMOD = 1
C_PREV = 2
C_DELTA = 3
C_WORDDELTA = 4
C_NAMES = {C_CONST: "const", C_POSMOD: "posmod", C_PREV: "prev",
           C_DELTA: "delta", C_WORDDELTA: "worddelta"}


def field_value(d: bytes, P: int, i: int, off: int, width: int) -> int:
    return int.from_bytes(d[P * i + off:P * i + off + width], "little")


def val_bytes(v: int, width: int) -> bytes:
    return (v & ((1 << (8 * width)) - 1)).to_bytes(width, "little")


def try_const(d, P, n, off, width):
    v0 = field_value(d, P, 0, off, width)
    for i in range(1, n):
        if field_value(d, P, i, off, width) != v0:
            return None
    return (v0,)


def try_posmod(d, P, n, off, width):
    """v_i == (phase + i*step) mod m, exact. Returns (m, step, phase) or None.

    m is recovered from g = gcd_i( v_i - v0 - i*step ) (all i); the class is
    accepted only if it reproduces every record exactly. This expresses the
    position-derived cycle id=i%1000 as (m=1000, step=1, phase=0).
    """
    full = 1 << (8 * width)
    v0 = field_value(d, P, 0, off, width)
    v1 = field_value(d, P, 1, off, width) if n > 1 else v0
    step = v1 - v0
    if step == 0:
        return (full, 0, v0) if all(
            field_value(d, P, i, off, width) == v0 for i in range(n)) else None
    import math as _m
    g = 0
    for i in range(n):
        g = _m.gcd(g, abs(field_value(d, P, i, off, width) - v0 - i * step))
    if g == 0:
        # exact linear, no wrap: sentinel m=0 means "value = phase + i*step"
        return (0, step, v0)
    m = g
    if m <= 1 or m > full:
        return None
    s = step % m
    ph = v0 % m
    for i in range(n):
        if field_value(d, P, i, off, width) != (ph + i * s) % m:
            return None
    return (m, s, ph)


def entropy_bits(vals) -> float:
    c = Counter(vals)
    tot = sum(c.values())
    return -sum(v * math.log2(v / tot) for v in c.values()) if tot else 0.0


def estimate_class(d, P, n, off, width, cls) -> float:
    """Entropy proxy for class selection (selector only; final wire is counted)."""
    if cls == C_CONST:
        return 0.0 if try_const(d, P, n, off, width) is not None else float("inf")
    if cls == C_POSMOD:
        return 0.0 if try_posmod(d, P, n, off, width) is not None else float("inf")
    if cls == C_PREV:
        tot = 0.0
        for k in range(width):
            pairs = []
            for i in range(1, n):
                ref = d[P * (i - 1) + off + k]
                cur = d[P * i + off + k]
                pairs.append(ref * 256 + cur)
            # H(cur | ref) via joint/ctx counts
            joint = Counter(pairs)
            ctx = Counter(p >> 8 for p in pairs)
            h = 0.0
            for (p, c) in joint.items():
                h += c * -math.log2(c / ctx[p >> 8])
            tot += h / max(1, len(pairs))
        return tot
    if cls == C_DELTA:
        tot = 0.0
        for k in range(width):
            dd = [(d[P * i + off + k] - d[P * (i - 1) + off + k]) & 0xFF
                  for i in range(1, n)]
            tot += entropy_bits(dd)
        return tot
    if cls == C_WORDDELTA:
        # per-byte H0 of the little-endian difference, MSB byte first
        mask = (1 << (8 * width)) - 1
        tot = 0.0
        for k in range(width - 1, -1, -1):
            dd = [((field_value(d, P, i, off, width)
                    - field_value(d, P, i - 1, off, width)) & mask) >> (8 * k) & 0xFF
                  for i in range(1, n)]
            tot += entropy_bits(dd)
        return tot
    return float("inf")


def select_class(d, P, n, off, width, allowed):
    best, best_bits = None, float("inf")
    # prefer exact classes (0 bits) in a fixed order
    for cls in (C_CONST, C_POSMOD, C_PREV, C_DELTA, C_WORDDELTA):
        if cls not in allowed:
            continue
        b = estimate_class(d, P, n, off, width, cls)
        if b < best_bits - 1e-9:
            best, best_bits = cls, b
    return best


class FieldCoder:
    """Encodes one field across all records with one class; shared enc/dec."""

    def __init__(self, off, width, cls, params):
        self.off = off
        self.width = width
        self.cls = cls
        self.params = params
        if cls == C_PREV:
            self.trees = [[ByteTree() for _ in range(256)] for _ in range(width)]
        elif cls == C_DELTA:
            self.trees = [ByteTree() for _ in range(width)]
        elif cls == C_WORDDELTA:
            self.trees = [ByteTree() for _ in range(width)]
        elif cls in (C_CONST, C_POSMOD):
            self.trees = None

    # ---- record 0 (no reference yet) -------------------------------------
    def enc_first(self, enc, d, P):
        self._ensure_pos0()
        if self.cls in (C_CONST, C_POSMOD):
            return
        v = field_value(d, P, 0, self.off, self.width)
        # order-0 per byte position, MSB first
        for k in range(self.width - 1, -1, -1):
            enc_byte(enc, self.trees_pos0[k], (v >> (8 * k)) & 0xFF)

    def dec_first(self, dec, out: bytearray, P):
        self._ensure_pos0()
        if self.cls == C_CONST:
            out.extend(val_bytes(self.params[0], self.width))
            return
        if self.cls == C_POSMOD:
            m, step, phase = self.params
            val = phase if m == 0 else phase % m
            out.extend(val_bytes(val, self.width))
            return
        v = 0
        for k in range(self.width - 1, -1, -1):
            v |= dec_byte(dec, self.trees_pos0[k]) << (8 * k)
        out.extend(val_bytes(v, self.width))

    def _ensure_pos0(self):
        if not hasattr(self, "trees_pos0"):
            self.trees_pos0 = [ByteTree() for _ in range(self.width)]

    # ---- record i>=1 -----------------------------------------------------
    def enc_record(self, enc, d, P, i):
        self._ensure_pos0()
        if self.cls == C_CONST:
            return
        if self.cls == C_POSMOD:
            return
        base = P * i + self.off
        rbase = P * (i - 1) + self.off
        if self.cls == C_PREV:
            for k in range(self.width):
                ctx = d[rbase + k]
                enc_sym(enc, self.trees[k], ctx, d[base + k])
        elif self.cls == C_DELTA:
            for k in range(self.width):
                enc_byte(enc, self.trees[k], (d[base + k] - d[rbase + k]) & 0xFF)
        elif self.cls == C_WORDDELTA:
            mask = (1 << (8 * self.width)) - 1
            cur = field_value(d, P, i, self.off, self.width)
            ref = field_value(d, P, i - 1, self.off, self.width)
            diff = (cur - ref) & mask
            for k in range(self.width - 1, -1, -1):
                enc_byte(enc, self.trees[k], (diff >> (8 * k)) & 0xFF)

    def dec_record(self, dec, out: bytearray, P, i):
        self._ensure_pos0()
        if self.cls == C_CONST:
            out.extend(val_bytes(self.params[0], self.width))
            return
        if self.cls == C_POSMOD:
            m, step, phase = self.params
            val = phase + i * step
            if m:
                val %= m
            out.extend(val_bytes(val, self.width))
            return
        rbase = P * (i - 1) + self.off
        if self.cls == C_PREV:
            for k in range(self.width):
                ctx = out[rbase + k]
                b = dec_sym(dec, self.trees[k], ctx)
                out.append(b)
        elif self.cls == C_DELTA:
            for k in range(self.width):
                b = dec_byte(dec, self.trees[k])
                out.append((out[rbase + k] + b) & 0xFF)
        elif self.cls == C_WORDDELTA:
            mask = (1 << (8 * self.width)) - 1
            ref = int.from_bytes(out[rbase:rbase + self.width], "little")
            diff = 0
            for k in range(self.width - 1, -1, -1):
                diff |= dec_byte(dec, self.trees[k]) << (8 * k)
            out.extend(val_bytes((ref + diff) & mask, self.width))


# ---------------------------------------------------------------------------
# Wire header (raw bytes, counted)
# ---------------------------------------------------------------------------
MAGIC = b"CR1"


def pack_header(mode: int, P: int, n: int, fields) -> bytes:
    out = bytearray(MAGIC)
    out.append(mode)
    out += struct.pack("<II", P, n)
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
    P, n = struct.unpack_from("<II", wire, pos); pos += 8
    nf = wire[pos]; pos += 1
    fields = []
    for _ in range(nf):
        off, width, cls = struct.unpack_from("<HBB", wire, pos); pos += 4
        params = None
        if cls == C_CONST:
            params = (int.from_bytes(wire[pos:pos + width], "little"),)
            pos += width
        elif cls == C_POSMOD:
            params = struct.unpack_from("<QqQ", wire, pos); pos += 24
        fields.append((off, width, cls, params))
    return mode, P, n, fields, pos


# ---------------------------------------------------------------------------
# Codecs
# ---------------------------------------------------------------------------
def encode(data: bytes, mode: str, P: int = 14, layout=None):
    n = len(data)
    if mode == "raw_o0":
        fields = []
        hdr = pack_header(0, 0, n, fields)
        enc = RangeEncoder()
        t = ByteTree()
        for b in data:
            enc_byte(enc, t, b)
        enc.flush()
        return hdr + bytes(enc.out)
    if mode == "raw_o1":
        fields = []
        hdr = pack_header(1, 0, n, fields)
        enc = RangeEncoder()
        trees = new_trees()
        prev = 0
        for b in data:
            enc_sym(enc, trees, prev, b)
            prev = b
        enc.flush()
        return hdr + bytes(enc.out)
    # ---- record-reference modes -----------------------------------------
    assert n % P == 0, "file not a whole number of records"
    nrec = n // P
    if mode == "ref_flat":
        hdr = pack_header(2, P, nrec, [])
        enc = RangeEncoder()
        trees = new_trees()          # global, keyed by reference byte only
        pos0 = ByteTree()
        for b in data[:P]:
            enc_byte(enc, pos0, b)
        for i in range(1, nrec):
            base, rbase = P * i, P * (i - 1)
            for k in range(P):
                enc_sym(enc, trees, data[rbase + k], data[base + k])
        enc.flush()
        return hdr + bytes(enc.out)
    if mode == "ref_col":
        layout = [(j, 1) for j in range(P)]
        fields = [(off, w, C_PREV, None) for (off, w) in layout]
        hdr = pack_header(3, P, nrec, fields)
        coders = [FieldCoder(off, w, C_PREV, None) for (off, w) in layout]
        enc = RangeEncoder()
        for c in coders:
            c.enc_first(enc, data, P)
        for i in range(1, nrec):
            for c in coders:
                c.enc_record(enc, data, P, i)
        enc.flush()
        return hdr + bytes(enc.out)
    if mode == "ref_field":
        layout = layout or parse_layout_arg()
        flds = []
        for (off, w) in layout:
            cls = select_class(data, P, nrec, off, w,
                               (C_CONST, C_POSMOD, C_PREV, C_DELTA, C_WORDDELTA))
            if cls == C_CONST:
                params = (field_value(data, P, 0, off, w),)
            elif cls == C_POSMOD:
                params = try_posmod(data, P, nrec, off, w)
            else:
                params = None
            flds.append((off, w, cls, params))
        hdr = pack_header(4, P, nrec, flds)
        coders = [FieldCoder(off, w, cls, params) for (off, w, cls, params) in flds]
        enc = RangeEncoder()
        for c in coders:
            c.enc_first(enc, data, P)
        for i in range(1, nrec):
            for c in coders:
                c.enc_record(enc, data, P, i)
        enc.flush()
        return hdr + bytes(enc.out)
    raise SystemExit("unknown mode " + mode)


def decode(wire: bytes):
    mode, P, n, fields, hpos = parse_header(wire)
    body = wire[hpos:]
    dec = RangeDecoder(body)
    if mode == 0:
        out = bytearray()
        t = ByteTree()
        for _ in range(n):
            out.append(dec_byte(dec, t))
        return bytes(out)
    if mode == 1:
        out = bytearray()
        trees = new_trees()
        prev = 0
        for _ in range(n):
            b = dec_sym(dec, trees, prev)
            out.append(b)
            prev = b
        return bytes(out)
    if mode == 2:
        out = bytearray()
        trees = new_trees()
        pos0 = ByteTree()
        for _ in range(P):
            out.append(dec_byte(dec, pos0))
        for i in range(1, n):
            rbase = P * (i - 1)
            for k in range(P):
                ctx = out[rbase + k]
                out.append(dec_sym(dec, trees, ctx))
        return bytes(out)
    if mode in (3, 4):
        coders = [FieldCoder(off, w, cls, params) for (off, w, cls, params) in fields]
        out = bytearray()
        for c in coders:
            c.dec_first(dec, out, P)
        for i in range(1, n):
            for c in coders:
                c.dec_record(dec, out, P, i)
        return bytes(out)
    raise SystemExit("unknown wire mode %d" % mode)


DEFAULT_LAYOUT = "0:8,8:4,12:2"


def parse_layout_str(s: str):
    out = []
    for part in s.split(","):
        off, w = part.split(":")
        out.append((int(off), int(w)))
    return out


def parse_layout_arg():
    s = DEFAULT_LAYOUT
    if "--layout" in sys.argv:
        s = sys.argv[sys.argv.index("--layout") + 1]
    return parse_layout_str(s)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd", choices=["encode", "verify", "sweep", "layout"])
    ap.add_argument("inp", nargs="?")
    ap.add_argument("out", nargs="?")
    ap.add_argument("--mode", default="ref_field")
    ap.add_argument("--period", type=int, default=14)
    ap.add_argument("--layout", default=DEFAULT_LAYOUT)
    ap.add_argument("--all-modes", action="store_true")
    a = ap.parse_args()

    if a.cmd == "layout":
        print(DEFAULT_LAYOUT)
        return

    data = open(a.inp, "rb").read()
    layout = parse_layout_str(a.layout)
    modes = (["raw_o0", "raw_o1", "ref_flat", "ref_col", "ref_field"]
             if a.cmd == "sweep" or a.all_modes else [a.mode])
    for m in modes:
        wire = encode(data, m, P=a.period, layout=layout)
        if m in ("ref_field",):
            _, P, n, fields, _ = parse_header(wire)
            desc = ",".join(f"{off}:{w}:{C_NAMES[cls]}" for (off, w, cls, _) in fields)
        else:
            desc = ""
        if a.cmd == "verify":
            back = decode(wire)
            ok = back == data
            print(f"{m}: bytes={len(wire)} roundtrip={'OK' if ok else 'FAIL'} "
                  f"ratio={len(wire)/len(data):.4f} {desc}")
            if not ok:
                raise SystemExit(1)
        else:
            print(f"{m}: bytes={len(wire)} ratio={len(wire)/len(data):.4f} {desc}")
        if a.out:
            open(a.out if len(modes) == 1 else f"{a.out}.{m}", "wb").write(wire)


if __name__ == "__main__":
    main()
