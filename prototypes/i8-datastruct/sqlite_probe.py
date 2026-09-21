#!/usr/bin/env python3
"""
datastruct lane (I8) — SQLite page anatomy + field-aware patch mining.

Task 2 of my lane: mine SQLite's approximate-repeat pairs for reusable
mismatch structure the way ELF was mined for TCOPY, and define a field-aware
patch reference.

Prior art to beat / reference points:
  - Parquet/ORC: RLE + bit-packing + dictionary per column
  - delta + zigzag + varint: every columnar store
  - SQLite's OWN varint + serial-type encoding (the file format itself)

SQLite page layout (page_size=4096 here):
  - Page 1: 100-byte database header, then a b-tree leaf interior/leaf
  - Leaf table b-tree page: 8-byte header (type u8, freeblock u16, ncells u16,
    cellcontent u16, fragmented u8), then a cell POINTER ARRAY (u16 each,
    ascending? NO — cell pointer array is in cell order, content is packed
    from the end)
  - Cell: [payload_len varint][rowid varint][payload]
  - Payload: [header_len varint][serial types varint...][body]

The measurement I need: how much of the SQLite gap is REPRESENTATION (a
better reference/field model would capture it) vs ENTROPY (irreducible)?
Method: decompose the file into structural classes and measure each class's
compressible content under (a) raw bytes, (b) a field-aware model.

Usage: python sqlite_probe.py <file>
"""
from __future__ import annotations
import argparse, math, os, struct, sys
from collections import Counter, defaultdict
import numpy as np

# SQLite serial types
SERIAL_LEN = {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 6, 6: 8, 7: 8, 8: 0, 9: 0}
SERIAL_NAME = {0: "NULL", 1: "i8", 2: "i16", 3: "i24", 4: "i32", 5: "i48",
               6: "i64", 7: "f64", 8: "0", 9: "1"}


def rd_varint(buf, p):
    """SQLite Huffman-ish varint: 1-9 bytes. Returns (value, newpos)."""
    v = 0
    for i in range(8):
        if p + i >= len(buf):
            return None, p
        b = buf[p + i]
        if i == 7:
            v = (v << 8) | b
            return v, p + 8
        v = (v << 7) | (b & 0x7F)
        if not (b & 0x80):
            return v, p + i + 1
    return None, p


def entropy_bits(arr) -> tuple[float, int]:
    a = np.asarray(arr)
    if a.size == 0:
        return 0.0, 0
    u, c = np.unique(a, return_counts=True)
    p = c.astype(np.float64) / a.size
    return float(-(p * np.log2(p)).sum()) * a.size, len(u)


def page_class(data: bytes, page_size: int, pageno: int) -> str:
    off = pageno * page_size
    if off >= len(data):
        return "OOB"
    if pageno == 0:
        return "HDR"
    h = data[off]
    return {2: "INDEX_INTERIOR", 5: "TABLE_INTERIOR",
            10: "INDEX_LEAF", 13: "TABLE_LEAF"}.get(h, f"OTHER({h})")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--page-size", type=int, default=4096)
    a = ap.parse_args()

    for path in a.files:
        data = open(path, "rb").read()
        n = len(data)
        ps = a.page_size
        npages = n // ps
        print(f"=== {os.path.basename(path)}  {n:,} B  "
              f"{npages} pages x {ps} B ===")

        # --- page type histogram -------------------------------------------
        pc = Counter(page_class(data, ps, i) for i in range(npages))
        print("\n  page types:")
        for k, v in pc.most_common():
            print(f"    {k:<18} {v:>5} pages  {v*ps:>10,} B  "
                  f"({100.0*v*ps/n:5.1f}%)")

        # --- byte-lane entropy per page type -------------------------------
        print("\n  per-page-type raw byte entropy (o0) and o1:")
        for ptype in [k for k, _ in pc.most_common()]:
            pages = [i for i in range(npages)
                     if page_class(data, ps, i) == ptype]
            if not pages:
                continue
            idx = np.concatenate([np.arange(i * ps, (i + 1) * ps)
                                  for i in pages[:200]])
            idx = idx[idx < n]
            b = np.frombuffer(data, dtype=np.uint8)[idx]
            h0, _ = entropy_bits(b)
            o1 = b[:-1].astype(np.int64) * 256 + b[1:].astype(np.int64)
            h1, _ = entropy_bits(o1)
            print(f"    {ptype:<18} o0={h0/8/idx.size:.4f} B/B  "
                  f"o1={h1/8/(idx.size-1):.4f} B/B   "
                  f"({len(pages)} pages, {idx.size:,} B sampled)")

        # --- cell-pointer arrays: aligned u16 fields -----------------------
        # On a table leaf, the cell pointer array starts at off+8 (page 0 has
        # the 100-byte file header) and holds ncells u16 BE values.
        print("\n  cell pointer array (aligned u16, BE) analysis:")
        ptr_vals = []
        ptr_deltas = []
        for i in range(npages):
            off = i * ps + (100 if i == 0 else 0)
            ptype = data[off]
            if ptype != 13:
                continue
            ncells = struct.unpack_from(">H", data, off + 3)[0]
            base = off + 8
            for c in range(ncells):
                p = base + 2 * c
                if p + 2 > n:
                    break
                ptr_vals.append(struct.unpack_from(">H", data, p)[0])
            for c in range(1, ncells):
                p = base + 2 * c
                if p + 2 > n:
                    break
                a0 = struct.unpack_from(">H", data, p - 2)[0]
                a1 = struct.unpack_from(">H", data, p)[0]
                ptr_deltas.append(a1 - a0)
        if ptr_vals:
            hv, nu = entropy_bits(np.array(ptr_vals))
            hd, nd = entropy_bits(np.array(ptr_deltas))
            print(f"    {len(ptr_vals):,} pointers: H(raw)={hv/len(ptr_vals):.3f} b "
                  f"({nu} distinct)")
            print(f"    {len(ptr_deltas):,} deltas  : H(delta)={hd/len(ptr_deltas):.3f} b "
                  f"({nd} distinct)")
            c = Counter(ptr_deltas)
            print(f"    top-8 delta values: {c.most_common(8)}")

        # --- rowid varint stream ------------------------------------------
        print("\n  rowid stream (SQLite varint, per cell):")
        rowids = []
        for i in range(npages):
            off = i * ps + (100 if i == 0 else 0)
            if off + 8 > n or data[off] != 13:
                continue
            ncells = struct.unpack_from(">H", data, off + 3)[0]
            base = off + 8
            for c in range(ncells):
                p = base + 2 * c
                if p + 2 > n:
                    break
                cell = struct.unpack_from(">H", data, p)[0]
                if cell < 1 or cell >= ps:
                    continue
                cp = i * ps + cell
                if cp + 2 > n:
                    continue
                plen, q = rd_varint(data, cp)
                if plen is None:
                    continue
                rowid, q2 = rd_varint(data, q)
                if rowid is None:
                    continue
                rowids.append(rowid)
        if rowids:
            hr, _ = entropy_bits(np.array(rowids))
            rd = [rowids[i + 1] - rowids[i] for i in range(len(rowids) - 1)]
            hdr_, nud = entropy_bits(np.array(rd))
            print(f"    {len(rowids):,} rowids: H(raw)={hr/len(rowids):.3f} b")
            print(f"    {len(rd):,} deltas : H(delta)={hdr_/len(rd):.3f} b "
                  f"({nud} distinct)")
            c = Counter(rd)
            print(f"    top-6 rowid deltas: {c.most_common(6)}")

        # --- THE KEY MEASUREMENT: representation vs entropy ---------------
        # Compare: raw bytes vs a "record-aligned" differencing. Take each
        # 4096 B page and difference it against the PREVIOUS page at the same
        # offset (page-level vertical XOR), and against the previous page
        # overall. This is the columnar/Pv idea applied to pages.
        print("\n  page-level vertical structure (representation probe):")
        P = np.frombuffer(data, dtype=np.uint8)[:npages * ps]
        P = P.reshape(npages, ps)
        for lag in (1, 2, 4):
            if npages <= lag:
                continue
            X = P[lag:].astype(np.int16)
            Y = P[:-lag].astype(np.int16)
            # vertical XOR (vXOR across pages) and vertical delta
            vx = np.bitwise_xor(P[lag:], P[:-lag])
            vd = (X - Y) % 256
            hxr, nux = entropy_bits(vx.ravel())
            hdr2, nud2 = entropy_bits(vd.ravel())
            hraw, _ = entropy_bits(P[lag:].ravel())
            print(f"    lag={lag}: raw={hraw/8/X.size:,.0f} B  "
                  f"vXOR={hxr/8/X.size:,.0f} B  vDelta={hdr2/8/X.size:,.0f} B")
        print()


if __name__ == "__main__":
    main()
