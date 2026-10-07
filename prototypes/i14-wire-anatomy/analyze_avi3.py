#!/usr/bin/env python3
"""Exact byte-accounting for frozen AVI3 wires. CPU-heavy work: GitHub Actions ONLY.

Advisory microtile capacity does NOT create an alternative bitstream or codec.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

MAX_OUTPUT = 1 << 28
BLOCK = 4096
TILES = (16, 32, 64, 128)

class InvalidWire(ValueError):
    pass

def need(ok: bool, why: str) -> None:
    if not ok:
        raise InvalidWire(why)

class Parser:
    def __init__(self, data: bytes):
        self.data = data
        self.pos = 0
        self.cost = Counter()
        self.strides = Counter()
        self.widths = Counter()
        self.block_modes = Counter()
        self.column_modes = Counter()
        self.microflat = 0
        self.micro = Counter()
        self.micro_adaptive = Counter()
        self.micro_positive_fields = Counter()
        self.total_records = 0
        self.raw_constant_columns = 0
        self.columns = 0
        self.modeled_columns = 0
        self.col_model_details = []

    def take(self, n: int, category: str) -> bytes:
        need(n >= 0 and self.pos <= len(self.data) and n <= len(self.data) - self.pos,
             "truncated " + category)
        v = self.data[self.pos:self.pos + n]
        self.pos += n
        self.cost[category] += n
        return v

    def uvar(self, category: str) -> int:
        v = 0
        for i in range(10):
            digit = self.take(1, category)[0]
            need(not (i == 9 and (digit & 0x7f) > 1), "varint overflow")
            v |= (digit & 127) << (7 * i)
            if not (digit & 128):
                need(i == 0 or (digit & 127) != 0, "noncanonical varint")
                return v
        raise InvalidWire("overlong varint")

    def packed(self, n: int, bits: int, category: str) -> list[int]:
        need(0 <= bits <= 56 and n <= BLOCK, "invalid packed dimension")
        length = (n * bits + 7) // 8
        data = self.take(length, category)
        if bits == 0:
            return [0] * n
        num = int.from_bytes(data, "little")
        need(num >> (n * bits) == 0, "nonzero packed padding bits")
        mask = (1 << bits) - 1
        return [(num >> (i * bits)) & mask for i in range(n)]

    def column(self, records: int, idx: int, block: int, stride: int) -> None:
        mode = self.take(1, "col_lane_tag")[0]
        self.column_modes[str(mode)] += 1
        self.columns += 1
        if mode == 0:
            raw = self.take(records * 4, "col_raw_data")
            if records and all(raw[i:i+4] == raw[0:4]
                               for i in range(4, len(raw), 4)):
                self.raw_constant_columns += 1
            return
        need(mode == 1, "unknown COL lane mode")
        self.modeled_columns += 1
        base = int.from_bytes(self.take(4, "col_base"), "little")
        step = int.from_bytes(self.take(4, "col_step"), "little")
        bits = self.take(1, "col_bitwidth")[0]
        need(bits < 32, "illegal COL bitwidth")
        residuals = self.packed(records, bits, "col_packed_innovations")
        flat = (records * bits + 7) // 8
        self.microflat += flat
        self.widths[str(bits)] += 1
        field = {"block": block, "lane": idx, "stride": stride, "records": records,
                 "bits": bits, "base": base, "step": step, "flat_payload": flat}
        for tile in TILES:
            groups = [residuals[j:j + tile] for j in range(0, records, tile)]
            alternative = len(groups) + sum((len(group) * max(x.bit_length() for x in group) + 7) // 8
                                            for group in groups)
            delta = flat - alternative
            self.micro[str(tile)] += delta
            self.micro_adaptive[str(tile)] += max(0, delta)
            self.micro_positive_fields[str(tile)] += delta > 0
            field["tile_" + str(tile) + "_delta"] = delta
        self.col_model_details.append(field)

    def parse(self, name: str) -> dict:
        need(self.take(4, "outer_magic") == b"AVI3", "invalid AVI3 magic")
        length = self.uvar("outer_length")
        need(length <= MAX_OUTPUT, "oversize declared output")
        decoded = 0
        block = 0
        while decoded < length:
            tag = self.take(1, "block_tag")[0]
            if tag == 0:
                n = self.uvar("raw_count")
                need(0 < n <= BLOCK and decoded + n <= length, "invalid raw bytes")
                self.take(n, "raw_data")
                decoded += n
                self.block_modes["raw"] += 1
            elif tag == 3:
                width = self.take(1, "full_width")[0]
                need(width in (1, 2, 4, 8), "invalid full width")
                count = self.uvar("full_count")
                need(3 <= count <= BLOCK // width and decoded + count * width <= length,
                     "invalid full count")
                self.take(width, "full_base")
                self.take(width, "full_step")
                bits = self.take(1, "full_bitwidth")[0]
                need(bits < width * 8 and bits <= 56, "invalid full bitwidth")
                self.packed(count, bits, "full_packed_innovations")
                decoded += count * width
                self.block_modes["full"] += 1
            elif tag == 4:
                n = self.uvar("col_count")
                need(24 <= n <= BLOCK and decoded + n <= length, "invalid COL bytes")
                stride = self.take(1, "col_stride")[0]
                need(2 <= stride <= 12, "invalid stride")
                records = n // (stride * 4)
                need(records >= 3, "invalid record count")
                self.strides[str(stride)] += 1
                self.total_records += records
                for j in range(stride):
                    self.column(records, j, block, stride)
                tail = n - records * stride * 4
                self.take(tail, "col_literal_tail")
                decoded += n
                self.block_modes["column"] += 1
            else:
                raise InvalidWire("unknown tag " + str(tag))
            block += 1
            need(block <= (length + 1) // 2 + 1, "excess block work")
        need(self.pos == len(self.data), "trailing wire bytes")
        need(decoded == length, "wrong decoded length")
        charge = sum(self.cost.values())
        need(charge == len(self.data), "incomplete byte accounting")
        modeled_header = (self.cost["col_lane_tag"] - self.cost["col_raw_data"] * 0)
        modeled_10 = self.modeled_columns * 10
        need((self.cost["col_base"] + self.cost["col_step"] + self.cost["col_bitwidth"])
             == self.modeled_columns * 9, "modeled header discrepancy")
        return {
            "schema": "anvil.i14.wire-anatomy/v1",
            "evidence_role": "discovery",
            "promotion_authorized": False,
            "frontier_crossing": False,
            "file": name,
            "wire_bytes": len(self.data),
            "source_bytes": length,
            "sha256_wire": hashlib.sha256(self.data).hexdigest(),
            "conserved_bytes": charge,
            "cost_partition": dict(sorted(self.cost.items())),
            "block_modes": dict(sorted(self.block_modes.items())),
            "strides": dict(sorted(self.strides.items())),
            "column_modes": dict(sorted(self.column_modes.items())),
            "width_histogram": dict(sorted(self.widths.items(), key=lambda x: int(x[0]))),
            "selected_columns": self.columns,
            "modeled_columns": self.modeled_columns,
            "model_lane_headers_bytes": modeled_10,
            "optimistic_unrealizable_max_model_parameter_elision_bytes": self.modeled_columns * 9,
            "raw_constant_columns": self.raw_constant_columns,
            "model_flat_payload_bytes": self.microflat,
            "tile_potential_signed_payload_delta_bytes": dict(sorted(self.micro.items())),
            "tile_potential_adaptively_nonnegative_bytes": dict(sorted(self.micro_adaptive.items())),
            "tile_positive_fields": dict(sorted(self.micro_positive_fields.items())),
            "per_modeled_column": self.col_model_details,
        }

def selftest() -> None:
    # Tiny exact RAW-only legal file and known malicious variants.
    wire = b"AVI3" + b"\x10" + b"\x00\x10" + bytes(range(16))
    parsed = Parser(wire).parse("raw")
    assert parsed["conserved_bytes"] == len(wire)
    for bad in (wire[:-1], wire + b"\x00", b"NOPE" + wire[4:],
                b"AVI3\x80\x00" + wire[5:]):
        try:
            Parser(bad).parse("bad")
        except InvalidWire:
            pass
        else:
            raise AssertionError("invalid wire was accepted")
    # Three records of two columns, first affine zero innovation, second raw.
    col = (b"\x04\x18\x02" +
           b"\x01" + (17).to_bytes(4,"little") + (1).to_bytes(4,"little") + b"\x00" +
           b"\x00" + (7).to_bytes(4,"little") * 3)
    framed = b"AVI3\x18" + col
    result = Parser(framed).parse("col")
    assert result["modeled_columns"] == 1 and result["selected_columns"] == 2
    assert result["conserved_bytes"] == len(framed)
    print("SELFTEST PASS legal/raw/model/truncated/padding/canonical")

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--wire", type=Path)
    ap.add_argument("--name")
    ap.add_argument("--expected-bytes", type=int)
    ap.add_argument("--out", type=Path)
    args = ap.parse_args()
    if args.selftest:
        selftest()
        return
    if None in (args.wire, args.name, args.expected_bytes, args.out):
        ap.error("all --wire --name --expected-bytes --out required")
    data = args.wire.read_bytes()
    need(len(data) == args.expected_bytes, "frozen I13 wire length mismatch")
    output = Parser(data).parse(args.name)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(output, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in output.items() if k != "per_modeled_column"},
                     sort_keys=True, indent=2))

if __name__ == "__main__":
    main()
