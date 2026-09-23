#!/usr/bin/env python3
"""GROTLI-ANVIL-0: exact line-record vertical-XOR representation scout.

This is intentionally a standalone research prototype. It does not define an
ANVIL production wire format. The carrier semantics are frozen in
docs/I10-GROTLI-G0-PREREG.md.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import pathlib
import statistics
import subprocess
import tempfile
import time
from dataclasses import asdict, dataclass
from typing import Iterable

MAGIC = b"AVG0"
VERSION = 1


def uvar_encode(x: int) -> bytes:
    if x < 0:
        raise ValueError("negative uvar")
    out = bytearray()
    while x >= 0x80:
        out.append((x & 0x7F) | 0x80)
        x >>= 7
    out.append(x)
    return bytes(out)


def uvar_decode(buf: bytes, pos: int) -> tuple[int, int]:
    value = 0
    shift = 0
    for _ in range(10):
        if pos >= len(buf):
            raise ValueError("truncated uvar")
        b = buf[pos]
        pos += 1
        value |= (b & 0x7F) << shift
        if not b & 0x80:
            # Canonical encoding only.
            if uvar_encode(value) != buf[pos - len(uvar_encode(value)):pos]:
                raise ValueError("non-canonical uvar")
            return value, pos
        shift += 7
    raise ValueError("uvar overflow")


def split_records(src: bytes) -> list[bytes]:
    records: list[bytes] = []
    start = 0
    for i, b in enumerate(src):
        if b == 0x0A:
            records.append(src[start:i + 1])
            start = i + 1
    if start < len(src):
        records.append(src[start:])
    return records


def h0_bits_per_byte(data: bytes) -> float:
    if not data:
        return 0.0
    counts = [0] * 256
    for b in data:
        counts[b] += 1
    n = len(data)
    return sum((-c * math.log2(c / n)) for c in counts if c) / n


def record_stats(lengths: list[int]) -> dict[str, float | int]:
    if not lengths:
        return {
            "record_count": 0,
            "record_len_min": 0,
            "record_len_median": 0.0,
            "record_len_p95": 0,
            "record_len_max": 0,
            "record_len_cv": 0.0,
        }
    ordered = sorted(lengths)
    p95 = ordered[min(len(ordered) - 1, math.ceil(0.95 * len(ordered)) - 1)]
    mean = statistics.fmean(lengths)
    cv = statistics.pstdev(lengths) / mean if mean else 0.0
    return {
        "record_count": len(lengths),
        "record_len_min": ordered[0],
        "record_len_median": statistics.median(ordered),
        "record_len_p95": p95,
        "record_len_max": ordered[-1],
        "record_len_cv": cv,
    }


@dataclass
class CarrierInfo:
    source_bytes: int
    record_count: int
    stride: int
    descriptor_bytes: int
    record_length_bytes: int
    xor_matrix_bytes: int
    padding_bytes: int
    carrier_bytes: int
    xor_zero_density: float
    raw_h0: float
    xor_h0: float


def encode_carrier(src: bytes) -> tuple[bytes, CarrierInfo]:
    records = split_records(src)
    lengths = [len(r) for r in records]
    stride = max(lengths, default=0)
    matrix_len = len(records) * stride
    if matrix_len > (1 << 34):
        raise ValueError("matrix exceeds frozen G0 bound")
    if stride > 4 * 1024 * 1024:
        raise ValueError("stride exceeds frozen G0 bound")

    header = bytearray(MAGIC)
    header.append(VERSION)
    header += uvar_encode(len(src))
    header += uvar_encode(len(records))
    header += uvar_encode(stride)

    length_stream = b"".join(uvar_encode(n) for n in lengths)
    header += length_stream
    header += uvar_encode(matrix_len)
    descriptor_bytes = len(header) - len(length_stream)

    out = bytearray(header)
    prev = bytearray(stride)
    zero_count = 0
    for rec in records:
        row = bytearray(stride)
        row[:len(rec)] = rec
        for i in range(stride):
            x = row[i] ^ prev[i]
            out.append(x)
            zero_count += x == 0
        prev = row

    matrix = bytes(out[-matrix_len:]) if matrix_len else b""
    padding = matrix_len - len(src)
    info = CarrierInfo(
        source_bytes=len(src),
        record_count=len(records),
        stride=stride,
        descriptor_bytes=descriptor_bytes,
        record_length_bytes=len(length_stream),
        xor_matrix_bytes=matrix_len,
        padding_bytes=padding,
        carrier_bytes=len(out),
        xor_zero_density=(zero_count / matrix_len if matrix_len else 0.0),
        raw_h0=h0_bits_per_byte(src),
        xor_h0=h0_bits_per_byte(matrix),
    )
    return bytes(out), info


def decode_carrier(carrier: bytes) -> bytes:
    if len(carrier) < len(MAGIC) + 1 or carrier[:4] != MAGIC:
        raise ValueError("bad G0 magic")
    if carrier[4] != VERSION:
        raise ValueError("unsupported G0 version")
    pos = 5
    source_len, pos = uvar_decode(carrier, pos)
    count, pos = uvar_decode(carrier, pos)
    stride, pos = uvar_decode(carrier, pos)
    if stride > 4 * 1024 * 1024:
        raise ValueError("stride bound")
    lengths: list[int] = []
    total = 0
    max_len = 0
    for _ in range(count):
        n, pos = uvar_decode(carrier, pos)
        if n > stride:
            raise ValueError("record length exceeds stride")
        total += n
        if total > source_len:
            raise ValueError("record lengths exceed source length")
        max_len = max(max_len, n)
        lengths.append(n)
    matrix_len, pos = uvar_decode(carrier, pos)
    if count and stride > (1 << 34) // count:
        raise ValueError("matrix multiplication overflow")
    if matrix_len != count * stride:
        raise ValueError("matrix length mismatch")
    if total != source_len:
        raise ValueError("source length mismatch")
    if count and max_len != stride:
        raise ValueError("stride mismatch")
    if not count and (source_len or stride or matrix_len):
        raise ValueError("invalid empty carrier")
    if len(carrier) - pos != matrix_len:
        raise ValueError("trailing/truncated carrier")

    result = bytearray()
    prev = bytearray(stride)
    for row_i, n in enumerate(lengths):
        base = pos + row_i * stride
        row = bytearray(stride)
        for i in range(stride):
            row[i] = carrier[base + i] ^ prev[i]
        result += row[:n]
        prev = row
    if len(result) != source_len:
        raise ValueError("decoded length mismatch")
    return bytes(result)


def brotli_compress(tool: pathlib.Path, path: pathlib.Path, out: pathlib.Path) -> tuple[float, int]:
    start = time.perf_counter()
    subprocess.run(
        [str(tool), "c", str(path), str(out)],
        check=True,
        stdout=subprocess.DEVNULL,
    )
    return time.perf_counter() - start, out.stat().st_size


def brotli_decompress(tool: pathlib.Path, path: pathlib.Path, out: pathlib.Path, expected: int) -> float:
    start = time.perf_counter()
    subprocess.run(
        [str(tool), "d", str(path), str(out), str(expected)],
        check=True,
        stdout=subprocess.DEVNULL,
    )
    return time.perf_counter() - start


def measure_file(path: pathlib.Path, work: pathlib.Path, brotli_tool: pathlib.Path) -> dict:
    src = path.read_bytes()
    carrier, info = encode_carrier(src)
    if decode_carrier(carrier) != src:
        raise RuntimeError("carrier roundtrip failed before Brotli")

    stem = hashlib.sha256(str(path).encode()).hexdigest()[:12]
    raw_path = work / f"{stem}.raw"
    car_path = work / f"{stem}.carrier"
    raw_br = work / f"{stem}.raw.br"
    car_br = work / f"{stem}.carrier.br"
    raw_dec = work / f"{stem}.raw.dec"
    car_dec = work / f"{stem}.carrier.dec"
    raw_path.write_bytes(src)
    car_path.write_bytes(carrier)

    raw_enc_s, raw_bytes = brotli_compress(brotli_tool, raw_path, raw_br)
    car_enc_s, car_bytes = brotli_compress(brotli_tool, car_path, car_br)
    raw_dec_s = brotli_decompress(brotli_tool, raw_br, raw_dec, len(src))
    car_br_dec_s = brotli_decompress(brotli_tool, car_br, car_dec, len(carrier))

    raw_roundtrip = raw_dec.read_bytes()
    decoded_carrier = car_dec.read_bytes()
    final = decode_carrier(decoded_carrier)
    sha = hashlib.sha256(src).hexdigest()
    if hashlib.sha256(raw_roundtrip).hexdigest() != sha or hashlib.sha256(final).hexdigest() != sha:
        raise RuntimeError("Brotli/final roundtrip hash mismatch")

    stats = record_stats([len(r) for r in split_records(src)])
    padding_expansion = (info.xor_matrix_bytes / len(src)) if src else 1.0
    delta = car_bytes - raw_bytes
    selected = "vxor" if car_bytes < raw_bytes else "raw"
    row = {
        "file": path.name,
        "source_bytes": len(src),
        "sha256": sha,
        **stats,
        "padding_expansion": padding_expansion,
        "raw_brotli_bytes": raw_bytes,
        "raw_encode_s": raw_enc_s,
        "raw_decode_s": raw_dec_s,
        **asdict(info),
        "vxor_brotli_bytes": car_bytes,
        "vxor_encode_s": car_enc_s,
        "vxor_brotli_decode_s": car_br_dec_s,
        "vxor_delta_bytes": delta,
        "vxor_delta_pct": (100.0 * delta / raw_bytes if raw_bytes else 0.0),
        "selected_arm": selected,
        "selected_bytes": min(raw_bytes, car_bytes),
        "roundtrip": True,
    }
    return row


def selftest() -> None:
    cases = [
        b"",
        b"\n",
        b"a\n",
        b"a\nb\n",
        b"a\nb",
        b"\n\n",
        bytes(range(1, 64)),
        b'{"x":1}\n{"x":2}\n{"x":3}\n',
        b"a\r\nb\r\n",
    ]
    for src in cases:
        car, _ = encode_carrier(src)
        assert decode_carrier(car) == src

    # Malformed/trailing-byte rejection.
    car, _ = encode_carrier(b"a\nb\n")
    for bad in (car[:-1], car + b"\x00", b"BAD!" + car[4:]):
        try:
            decode_carrier(bad)
        except ValueError:
            pass
        else:
            raise AssertionError("malformed carrier accepted")
    print("G0 carrier selftest: PASS")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*", type=pathlib.Path)
    ap.add_argument("--out", type=pathlib.Path)
    ap.add_argument("--brotli-tool", type=pathlib.Path, default=pathlib.Path("build/brotli_lw"))
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        selftest()
        if not args.files:
            return
    if not args.files:
        ap.error("provide files or --selftest")
    out = args.out or pathlib.Path("g0-results.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="anvil-g0-") as td:
        work = pathlib.Path(td)
        rows = [measure_file(p, work, args.brotli_tool) for p in args.files]
    result = {
        "schema": 1,
        "files": rows,
        "aggregate": {
            "source_bytes": sum(r["source_bytes"] for r in rows),
            "raw_brotli_bytes": sum(r["raw_brotli_bytes"] for r in rows),
            "vxor_brotli_bytes": sum(r["vxor_brotli_bytes"] for r in rows),
            "selected_bytes": sum(r["selected_bytes"] for r in rows),
        },
    }
    result["aggregate"]["selected_delta_bytes"] = (
        result["aggregate"]["selected_bytes"] - result["aggregate"]["raw_brotli_bytes"]
    )
    out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
