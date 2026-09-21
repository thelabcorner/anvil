#!/usr/bin/env python3
"""P4.1 transform prototype: build a reconstruction-valid transformed file and
prove its inverse is byte-identical to the source.

Transform (in situ):
  every VALID stream's compressed payload (clen bytes at absolute offset) is
  replaced by its decompressed plaintext (ulen bytes).  DIFF/BRUTE streams are
  left untouched (charged as fallback).
  A side table is appended:
      entries: varint(offset), varint(clen), varint(ulen), 2 param bytes
      footer : u32 entries_len, u32 n_streams, b"I9DFLT01"
  param byte0 = (level-1) | (strategy << 4)
  param byte1 = (memlevel-1) | (wbits==15 ? 0x10 : 0)

Inverse:
  walk entries in offset order with a running delta; replace the ulen
  plaintext bytes with replay(plaintext, params); then compare sha256 to the
  original.  Any mismatch aborts non-zero.

Outputs transformed-<file>.bin plus transform-<file>.json with sizes and the
side-table raw/brotli cost.

Usage:
  python transform.py <census.json> <replay.csv> --src <file> --out-dir results
"""
import argparse
import csv
import hashlib
import json
import os
import zlib

import brotli


def put_uvarint(buf, v):
    while True:
        b = v & 0x7F
        v >>= 7
        if v:
            buf.append(b | 0x80)
        else:
            buf.append(b)
            return


def get_uvarint(buf, p):
    shift = 0
    v = 0
    while True:
        b = buf[p]
        p += 1
        v |= (b & 0x7F) << shift
        if not (b & 0x80):
            return v, p
        shift += 7


def replay(plain, params):
    (level, memlevel, strategy, wbits) = params
    c = zlib.compressobj(level, zlib.DEFLATED, wbits, memlevel, strategy)
    return c.compress(plain) + c.flush()


def parse_param(s):
    wbits, level, memlevel, strategy = s.split("/")
    return (int(level), int(memlevel), int(strategy), int(wbits))


def png_idat_ranges(data, sig_off):
    """Return list of (data_off, data_len) for IDAT chunks of the PNG at sig_off."""
    p = sig_off + 8
    ranges = []
    while p + 12 <= len(data):
        ln = int.from_bytes(data[p:p + 4], "big")
        typ = data[p + 4:p + 8]
        if typ == b"IDAT":
            ranges.append((p + 8, ln))
        p += 12 + ln
        if typ == b"IEND":
            break
    return ranges


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("census_json")
    ap.add_argument("replay_csv")
    ap.add_argument("--src", required=True)
    ap.add_argument("--out-dir", default="results")
    a = ap.parse_args()
    os.makedirs(a.out_dir, exist_ok=True)
    data = bytearray(open(a.src, "rb").read())
    orig = bytes(data)
    cd = json.load(open(a.census_json))
    streams = {s["offset"]: s for s in cd["streams"]
               if s["method"] == "deflate" and s["verified"]}
    name = cd["summary"]["file"]

    entries = []
    skipped_png_multi = 0
    for r in csv.DictReader(open(a.replay_csv, encoding="utf-8")):
        if r["mode"] != "valid":
            continue
        off = int(r["offset"])
        s = streams.get(off)
        if s is None:
            continue
        clen = int(r["clen"])
        ulen = int(r["ulen"])
        kind = r["kind"]
        if kind == "png-zlib":
            # only safe when the IDAT payload is one contiguous range matching
            # the census slice (multi-IDAT chunk headers would be overwritten)
            sig = orig.rfind(b"\x89PNG\r\n\x1a\n", max(0, off - (1 << 20)), off)
            ranges = png_idat_ranges(orig, sig) if sig >= 0 else []
            if len(ranges) != 1 or ranges[0][0] != off or ranges[0][1] != clen:
                skipped_png_multi += 1
                continue
        params = parse_param(r["param"])
        entries.append((off, clen, ulen, params))
    entries.sort()

    # overlap / bounds check
    prev_end = -1
    for off, clen, ulen, _ in entries:
        assert off >= prev_end, f"overlap at {off}"
        assert off + clen <= len(data), f"oob at {off}"
        prev_end = off + clen

    # apply in-situ replacement (buffer grows by ulen-clen per stream)
    delta = 0
    for off, clen, ulen, params in entries:
        comp = bytes(orig[off:off + clen])
        plain = (zlib.decompress(comp, -15) if params[3] == -15
                 else zlib.decompress(comp))
        pos = off + delta
        data[pos:pos + clen] = plain
        delta += ulen - clen
    t = bytes(data)

    # side table
    ent = bytearray()
    for off, clen, ulen, (level, memlevel, strategy, wbits) in entries:
        put_uvarint(ent, off)
        put_uvarint(ent, clen)
        put_uvarint(ent, ulen)
        ent.append(((level - 1) & 0x0F) | ((strategy & 0x0F) << 4))
        ent.append(((memlevel - 1) & 0x0F) | (0x10 if wbits == 15 else 0))
    footer = bytearray()
    footer += len(ent).to_bytes(4, "little")
    footer += len(entries).to_bytes(4, "little")
    footer += b"I9DFLT01"
    t = t + bytes(ent) + bytes(footer)

    # ---- inverse verification
    buf = bytearray(t)
    ent_len = int.from_bytes(t[-16:-12], "little")
    n = int.from_bytes(t[-12:-8], "little")
    assert t[-8:] == b"I9DFLT01"
    assert n == len(entries)
    entries_off = len(t) - 16 - ent_len
    p = entries_off
    delta = 0
    plan = []
    for _ in range(n):
        off, p = get_uvarint(t, p)
        clen, p = get_uvarint(t, p)
        ulen, p = get_uvarint(t, p)
        b0 = t[p]; b1 = t[p + 1]; p += 2
        level = (b0 & 0x0F) + 1
        strategy = (b0 >> 4) & 0x0F
        memlevel = (b1 & 0x0F) + 1
        wbits = 15 if (b1 & 0x10) else -15
        plan.append((off + delta, clen, ulen, (level, memlevel, strategy, wbits)))
        delta += ulen - clen
    # T-coordinates computed; reconstruct from last to first so earlier
    # positions are unaffected by the shrink.
    for pos, clen, ulen, params in reversed(plan):
        plain = bytes(buf[pos:pos + ulen])
        rec = replay(plain, params)
        if len(rec) != clen:
            raise SystemExit(f"replay length mismatch at {pos}: {len(rec)} != {clen}")
        buf[pos:pos + ulen] = rec
    body_len = len(t) - 16 - ent_len  # transformed body length (pre-replay)
    buf = buf[:len(buf) - 16 - ent_len]  # drop side table + footer
    sha_t = hashlib.sha256(buf).hexdigest()
    sha_o = hashlib.sha256(orig).hexdigest()
    ok = sha_t == sha_o
    if not ok:
        print(f"[transform] len(buf)={len(buf)} len(orig)={len(orig)} "
              f"body_len={body_len} ent_len={ent_len} n={n}")
        mism = [i for i in range(min(len(buf), len(orig))) if buf[i] != orig[i]]
        print(f"[transform] n_mismatch={len(mism)} first={mism[:8]}")
        if mism:
            i = mism[0]
            print(f"  ctx orig {orig[max(0,i-8):i+8].hex()}")
            print(f"  ctx out  {buf[max(0,i-8):i+8].hex()}")
            for off, clen, ulen, params in entries:
                if off - 64 <= i <= off + max(clen, ulen) + 64:
                    print(f"  near entry off={off} clen={clen} ulen={ulen} params={params}")
    print(f"[transform] {len(entries)} streams, roundtrip identical={ok}")
    if not ok:
        raise SystemExit("INVERSE NOT BYTE-IDENTICAL")

    out_t = os.path.join(a.out_dir, f"transformed-{name}.bin")
    open(out_t, "wb").write(t)
    side_brotli = len(brotli.compress(bytes(ent), quality=11, lgwin=24))
    summary = {
        "file": name,
        "src_bytes": len(orig),
        "transformed_bytes": len(t),
        "n_transformed": len(entries),
        "skipped_png_multi_idat": skipped_png_multi,
        "side_table_raw": len(ent),
        "side_table_brotli_q11_lw24": side_brotli,
        "roundtrip_sha256_identical": ok,
        "src_sha256": sha_o,
        "transformed_sha256": sha_t,
        "replay_command": "prototypes/i9-deflate/transform.py <census.json> <replay.csv> --src <file>",
        "inverse_formula": "replace T[off+delta : off+delta+ulen] with zlib.replay(plain, level,memLevel,strategy,wbits)",
    }
    out_j = os.path.join(a.out_dir, f"transform-{name}.json")
    json.dump(summary, open(out_j, "w", encoding="utf-8"), indent=1)
    print(json.dumps(summary, indent=2))
    print(f"[transform] wrote {out_t} / {out_j}")


if __name__ == "__main__":
    main()
