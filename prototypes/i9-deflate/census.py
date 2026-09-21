#!/usr/bin/env python3
"""P4.1 DEFLATE census -- enumerate embedded DEFLATE streams in a binary.

Scans for:
  - ZIP archives (EOCD -> central directory -> local file headers), method 8
    (deflate) and method 0 (stored) entries; supports zip64 and
    self-extracting prefixes via archive-base reconstruction.
  - gzip members (1f 8b 08), full header parse + CRC32/ISIZE verification.
  - PNG files (chunk walk, IDAT zlib streams).
  - Raw zlib streams over a whitelist of common CMF/FLG headers, validated by
    actual decompression + Adler-32 (random false positives cannot survive a
    multi-KB decompression; zlib validates Adler at stream end).
  - CAB (MSCF) MSZIP blocks ('CK' raw deflate blocks) -- census only.

Outputs a per-stream CSV plus a JSON summary with byte totals.

Usage:
  python census.py <file> --out-dir results
"""
import argparse
import csv
import json
import os
import struct
import sys
import zlib
from dataclasses import dataclass, asdict, field
from typing import Optional

# ---------------------------------------------------------------- data model

@dataclass
class Stream:
    src_file: str
    kind: str          # zip | gzip | png-zlib | raw-zlib | cab-mszip
    container: str     # archive name / container description
    entry: str         # entry name if available
    offset: int        # absolute offset of the COMPRESSED deflate payload
    clen: int          # compressed payload bytes
    ulen: int          # decompressed bytes (0 if unknown/not validated)
    method: str        # deflate / stored / other
    crc_ok: bool = False
    verified: bool = False
    note: str = ""
    level: int = 0     # recursion level (0 = raw file bytes)

# ---------------------------------------------------------------- helpers

def crc32(b: bytes) -> int:
    return zlib.crc32(b) & 0xFFFFFFFF


# ---------------------------------------------------------------- ZIP

def iter_eocds(data: bytes):
    """Yield (eocd_offset, info) for every plausible EOCD, scanning backwards
    from each candidate so concatenated archives are found."""
    pos = 0
    sig = b"PK\x05\x06"
    while True:
        off = data.find(sig, pos)
        if off < 0:
            return
        pos = off + 4
        if off + 22 > len(data):
            continue
        (disk, cd_disk, n_disk, n_total, cd_size, cd_off, comment_len) = \
            struct.unpack_from("<HHHHIIH", data, off + 4)
        # candidate sanity: cd_size/cd_off within file, cd signature present
        if cd_size == 0xFFFFFFFF or cd_off == 0xFFFFFFFF:
            # zip64 locator precedes EOCD
            base = off - 20
            if base >= 0 and data[base:base + 4] == b"PK\x06\x07":
                z64_off = struct.unpack_from("<Q", data, base + 8)[0]
                if z64_off + 56 <= len(data) and data[z64_off:z64_off + 4] == b"PK\x06\x06":
                    n_total = struct.unpack_from("<Q", data, z64_off + 32)[0]
                    cd_size = struct.unpack_from("<Q", data, z64_off + 40)[0]
                    cd_off = struct.unpack_from("<Q", data, z64_off + 48)[0]
        if n_total == 0:
            continue
        # archive base: EOCD is (usually) at base + cd_off + cd_size
        base = off - cd_off - cd_size
        yield off, base, n_total, cd_size, cd_off


def parse_zip(data: bytes, path: str, level: int = 0):
    """Parse all ZIP archives found; returns stream list."""
    out = []
    seen = set()
    for eocd, base, n_total, cd_size, cd_off in iter_eocds(data):
        if base < 0 or base + cd_off + cd_size > len(data):
            continue
        p = base + cd_off
        entries = 0
        while p + 46 <= len(data) and data[p:p + 4] == b"PK\x01\x02" and entries < n_total + 1:
            (ver_made, ver_need, flags, method, mtime, mdate, crc, csize, usize,
             nlen, elen, clen_, dstart, iattr, eattr, lho) = \
                struct.unpack_from("<HHHHHHIIIHHHHHII", data, p + 4)
            name = data[p + 46:p + 46 + nlen]
            extra = data[p + 46 + nlen:p + 46 + nlen + elen]
            name_s = name.decode("utf-8", "replace")
            p += 46 + nlen + elen + clen_
            entries += 1
            # zip64 extra
            if csize == 0xFFFFFFFF or usize == 0xFFFFFFFF or lho == 0xFFFFFFFF:
                q = 0
                while q + 4 <= len(extra):
                    hid, hsz = struct.unpack_from("<HH", extra, q)
                    if hid == 0x0001:
                        body = extra[q + 4:q + 4 + hsz]
                        r = 0
                        if usize == 0xFFFFFFFF and r + 8 <= len(body):
                            usize = struct.unpack_from("<Q", body, r)[0]; r += 8
                        if csize == 0xFFFFFFFF and r + 8 <= len(body):
                            csize = struct.unpack_from("<Q", body, r)[0]; r += 8
                        if lho == 0xFFFFFFFF and r + 8 <= len(body):
                            lho = struct.unpack_from("<Q", body, r)[0]; r += 8
                        break
                    q += 4 + hsz
            abso = base + lho
            if abso + 30 > len(data) or data[abso:abso + 4] != b"PK\x03\x04":
                # some archives have no usable local header; skip
                continue
            lnlen, lelen = struct.unpack_from("<HH", data, abso + 26)
            dstart_abs = abso + 30 + lnlen + lelen
            if csize == 0 or dstart_abs + csize > len(data):
                continue
            key = (dstart_abs, csize, name_s)
            if key in seen:
                continue
            seen.add(key)
            st = Stream(src_file=path, kind="zip",
                        container=f"zip@{base}" + (f"[rec{level}]" if level else ""),
                        entry=name_s, offset=dstart_abs, clen=csize, ulen=usize,
                        method={0: "stored", 8: "deflate"}.get(method, f"method{method}"),
                        level=level)
            if method == 8:
                comp = data[dstart_abs:dstart_abs + csize]
                try:
                    dec = zlib.decompress(comp, -15)
                    st.ulen = len(dec)
                    st.verified = True
                    if crc:
                        st.crc_ok = (crc32(dec) == crc)
                    else:
                        st.crc_ok = (len(dec) == usize)
                except Exception as e:  # noqa: BLE001
                    st.note = f"deflate error: {e.__class__.__name__}"
            elif method == 0:
                st.verified = True
                st.crc_ok = (crc32(data[dstart_abs:dstart_abs + csize]) == crc) if crc else True
            out.append(st)
    return out


# ---------------------------------------------------------------- gzip

def parse_gzip(data: bytes, path: str, level: int = 0):
    out = []
    pos = 0
    sig = b"\x1f\x8b\x08"
    while True:
        off = data.find(sig, pos)
        if off < 0:
            return out
        pos = off + 3
        try:
            d = zlib.decompressobj(16 + zlib.MAX_WBITS)
            dec = d.decompress(data[off:off + (1 << 31)])
            dec += d.flush()
            if not d.eof:
                continue
            consumed = len(data[off:off + (1 << 31)]) - len(d.unused_data)
            isize = struct.unpack_from("<I", data, off + consumed - 4)[0]
            crc = struct.unpack_from("<I", data, off + consumed - 8)[0]
            st = Stream(src_file=path, kind="gzip", container=f"gzip@{off}",
                        entry="", offset=off, clen=consumed, ulen=len(dec),
                        method="deflate", level=level)
            st.verified = True
            st.crc_ok = (crc32(dec) == crc) and (len(dec) & 0xFFFFFFFF) == isize
            out.append(st)
        except Exception:  # noqa: BLE001
            continue
    return out


# ---------------------------------------------------------------- PNG

def parse_png(data: bytes, path: str, level: int = 0):
    out = []
    sig = b"\x89PNG\r\n\x1a\n"
    pos = 0
    while True:
        off = data.find(sig, pos)
        if off < 0:
            return out
        pos = off + 8
        p = off + 8
        idat_parts = []
        idat_start = None
        try:
            while p + 12 <= len(data):
                (ln,) = struct.unpack_from(">I", data, p)
                typ = data[p + 4:p + 8]
                if ln > (1 << 31) or p + 12 + ln > len(data):
                    raise ValueError
                if typ == b"IDAT":
                    if idat_start is None:
                        idat_start = p + 8
                    idat_parts.append(data[p + 8:p + 8 + ln])
                p += 12 + ln
                if typ == b"IEND":
                    break
            if idat_start is None:
                continue
            comp = b"".join(idat_parts)
            dec = zlib.decompress(comp)
            st = Stream(src_file=path, kind="png-zlib", container=f"png@{off}",
                        entry="IDAT", offset=idat_start, clen=len(comp), ulen=len(dec),
                        method="deflate", level=level)
            st.verified = True
            st.crc_ok = True  # zlib stream had valid Adler-32
            out.append(st)
        except Exception:  # noqa: BLE001
            continue
    return out


# ---------------------------------------------------------------- raw zlib

# CM=8 (deflate), FDICT=0, FLEVEL in {0,1,2,3}: 4 canonical FLG per CMF.
ZLIB_HEADS = []
for _cmf in (0x78, 0x68, 0x58, 0x48, 0x38, 0x28, 0x18, 0x08):
    for _flevel in range(4):
        for _flg in range(256):
            if (_flg & 0x20) == 0 and (_flg >> 6) == _flevel \
                    and ((_cmf << 8) | _flg) % 31 == 0:
                ZLIB_HEADS.append(bytes([_cmf, _flg]))
                break
assert all((h[0] & 0x0F) == 8 for h in ZLIB_HEADS), ZLIB_HEADS


def parse_raw_zlib(data: bytes, path: str, exclude_ranges, level: int = 0):
    """Whitelist-header scan.  Validated by real decompression (Adler-32 makes
    a >4KB false positive effectively impossible)."""
    out = []
    ex = sorted(exclude_ranges)
    import bisect
    starts = [e[0] for e in ex]

    def excluded(off):
        i = bisect.bisect_right(starts, off) - 1
        if i >= 0 and ex[i][0] <= off < ex[i][1]:
            return True
        return False

    n_attempt = 0
    for h in ZLIB_HEADS:
        pos = 0
        while True:
            off = data.find(h, pos)
            if off < 0:
                break
            pos = off + 2
            if excluded(off):
                continue
            n_attempt += 1
            try:
                cap = min(len(data) - off, 64 * 1024 * 1024)
                mv = memoryview(data)[off:off + cap]
                d = zlib.decompressobj()
                dec = d.decompress(mv)
                if not d.eof or len(dec) < 256:
                    continue
                # consumed = input length - unused_data - unconsumed_tail
                consumed = cap - len(d.unused_data) - len(d.unconsumed_tail)
                st = Stream(src_file=path, kind="raw-zlib",
                            container=f"zlib@{off}", entry="",
                            offset=off, clen=consumed, ulen=len(dec),
                            method="deflate", level=level)
                st.verified = True
                st.crc_ok = True
                st.note = f"head={h.hex()}"
                out.append(st)
            except Exception:  # noqa: BLE001
                continue
    return out, n_attempt


# ---------------------------------------------------------------- CAB MSZIP

def parse_cab(data: bytes, path: str, level: int = 0):
    """Census only: MSCF cabinet, walk CFFOLDER/CFFILE for MSZIP blocks.
    MSSZIP block payloads start with 'CK' followed by raw deflate."""
    out = []
    pos = 0
    while True:
        off = data.find(b"MSCF", pos)
        if off < 0:
            return out
        pos = off + 4
        if off + 36 > len(data):
            continue
        try:
            (cbCabinet, coffFiles) = struct.unpack_from("<II", data, off + 8)
            if cbCabinet == 0 or off + cbCabinet > len(data):
                continue
            p = off + coffFiles
            while p + 16 <= off + cbCabinet and data[p:p + 4] == b"CK":
                (csize, usize, uoff) = struct.unpack_from("<HHI", data, p + 4)
                if p + 8 + csize > len(data):
                    break
                st = Stream(src_file=path, kind="cab-mszip", container=f"cab@{off}",
                            entry=f"CFDATA@{p}", offset=p + 8, clen=csize, ulen=usize,
                            method="deflate", level=level)
                try:
                    dec = zlib.decompress(data[p + 8:p + 8 + csize], -15)
                    st.verified = True
                    st.ulen = len(dec)
                    st.crc_ok = (len(dec) == usize)
                except Exception as e:  # noqa: BLE001
                    st.note = f"MSZIP error: {e.__class__.__name__}"
                out.append(st)
                p += 8 + csize
        except Exception:  # noqa: BLE001
            continue
    return out


# ---------------------------------------------------------------- main

def census(path: str, out_dir: str):
    data = open(path, "rb").read()
    name = os.path.basename(path)
    streams = []
    streams += parse_png(data, name)
    streams += parse_gzip(data, name)
    streams += parse_zip(data, name)
    streams += parse_cab(data, name)

    # raw zlib: exclude known container payload ranges to avoid double counting
    ranges = []
    for s in streams:
        if s.verified:
            ranges.append((s.offset, s.offset + s.clen))
    raw, n_attempt = parse_raw_zlib(data, name, ranges)
    streams += raw

    # de-dup identical (offset, clen) across detectors
    dedup = {}
    for s in streams:
        key = (s.offset, s.clen, s.kind)
        if key not in dedup:
            dedup[key] = s
    streams = sorted(dedup.values(), key=lambda s: s.offset)

    os.makedirs(out_dir, exist_ok=True)
    csv_path = os.path.join(out_dir, f"census-{name}.csv")
    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["kind", "container", "entry", "offset", "clen", "ulen",
                    "method", "verified", "crc_ok", "note"])
        for s in streams:
            w.writerow([s.kind, s.container, s.entry, s.offset, s.clen, s.ulen,
                        s.method, int(s.verified), int(s.crc_ok), s.note])

    def total(pred):
        return sum(s.clen for s in streams if pred(s))

    def total_u(pred):
        return sum(s.ulen for s in streams if pred(s))

    is_deflate = lambda s: s.method == "deflate"
    summary = {
        "file": name,
        "size": len(data),
        "n_streams": len(streams),
        "n_verified": sum(1 for s in streams if s.verified),
        "n_crc_ok": sum(1 for s in streams if s.crc_ok),
        "deflate_clen": total(is_deflate),
        "deflate_ulen": total_u(is_deflate),
        "deflate_clen_verified": total(lambda s: is_deflate(s) and s.verified),
        "deflate_ulen_verified": total_u(lambda s: is_deflate(s) and s.verified),
        "stored_clen": total(lambda s: s.method == "stored"),
        "by_kind": {},
        "n_raw_zlib_attempts": n_attempt,
    }
    for s in streams:
        k = summary["by_kind"].setdefault(s.kind, {"n": 0, "clen": 0, "ulen": 0, "verified": 0})
        k["n"] += 1
        k["clen"] += s.clen
        k["ulen"] += s.ulen
        k["verified"] += int(s.verified)

    json_path = os.path.join(out_dir, f"census-{name}.json")
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump({"summary": summary,
                   "streams": [asdict(s) for s in streams]}, f, indent=1)
    print(json.dumps(summary, indent=2))
    print(f"[census] wrote {csv_path} and {json_path}")
    return summary


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("--out-dir", default="results")
    a = ap.parse_args()
    census(a.file, a.out_dir)
