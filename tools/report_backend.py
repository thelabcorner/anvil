#!/usr/bin/env python3
"""Report which backend ANVIL's rev-2 ratio envelope selected, per compressed file.

ANVIL rev-2 container (src/anvil.cpp):
    "ANV0"           4-byte magic
    byte revision     (2 for ratio/transform envelope)
    uvarint block_size
    uvarint total_output_size
    repeated blocks:
        uvarint blen
        byte    mode        (0 raw, 17 = ratio envelope)
        uvarint plen
        u32le   crc32
        plen bytes payload
For mode 17 (ratio envelope), payload[0] = transform_id (0 direct, 1 ctx1-256,
2 line-columns) and payload[1] = backend_id (1 Brotli, 2 BWT).

Usage:
    tools/report_backend.py <file.anv> [<file2.anv> ...]
    tools/report_backend.py --dir <dir> --glob "*.anv"

Prints per file: transform_id, backend name, and a short note. Exit code is 0
when every file parses; 2 on any parse error. Intended to be called after
bench_ratio.py so a measured run can state the chosen backend per file.
"""
from __future__ import annotations

import sys
from pathlib import Path


def _read_uvar(data: bytes, pos: int):
    """Decode an unsigned LEB128 varint. Returns (value, new_pos)."""
    shift = 0
    result = 0
    while True:
        if pos >= len(data):
            raise ValueError("truncated varint")
        b = data[pos]
        result |= (b & 0x7F) << shift
        pos += 1
        if not (b & 0x80):
            break
        shift += 7
    return result, pos


def detect(path: Path) -> dict:
    data = path.read_bytes()
    if len(data) < 6 or data[:4] != b"ANV0":
        raise ValueError("not an ANVIL container (missing ANV0 magic)")
    revision = data[4]
    pos = 5
    _, pos = _read_uvar(data, pos)  # block_size
    _, pos = _read_uvar(data, pos)  # total output size
    backends = []
    transforms = []
    block_idx = 0
    while pos < len(data):
        blen, pos = _read_uvar(data, pos)
        if pos >= len(data):
            raise ValueError("truncated block header")
        mode = data[pos]
        pos += 1
        plen, pos = _read_uvar(data, pos)
        pos += 4  # crc32
        if pos + plen > len(data):
            raise ValueError("block payload beyond EOF")
        payload = data[pos:pos + plen]
        pos += plen
        if mode == 17:
            if len(payload) < 2:
                raise ValueError("ratio envelope payload too short")
            transforms.append(payload[0])
            backends.append(payload[1])
        block_idx += 1
    if not backends:
        return {"file": path.name, "revision": revision, "blocks": block_idx,
                "backend": "NONE", "transform": "NONE",
                "ok": False, "note": "no mode-17 ratio blocks found"}
    # Whole-file ratio uses a single block, but report the per-block choices.
    unique_backends = sorted(set(backends))
    unique_transforms = sorted(set(transforms))
    name = {1: "Brotli", 2: "BWT"}.get(unique_backends[0], f"id{unique_backends[0]}")
    if len(unique_backends) > 1:
        name = "+".join({1: "Brotli", 2: "BWT"}.get(b, f"id{b}") for b in unique_backends) + " (mixed)"
    tname = {0: "direct", 1: "ctx1-256", 2: "line-columns"}.get(unique_transforms[0], f"id{unique_transforms[0]}")
    if len(unique_transforms) > 1:
        tname = "mixed"
    return {"file": path.name, "revision": revision, "blocks": block_idx,
            "backend": name, "transform": tname, "ok": True,
            "backend_ids": unique_backends}


def main() -> int:
    args = sys.argv[1:]
    paths: list[Path] = []
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--dir":
            d = Path(args[i + 1])
            pat = args[i + 2] if i + 2 < len(args) and not args[i + 2].startswith("--") else "*.anv"
            paths += list(d.glob(pat))
            i += 3
        else:
            paths.append(Path(a))
            i += 1
    if not paths:
        print("usage: report_backend.py <file.anv> [..] [--dir DIR GLOB]", file=sys.stderr)
        return 2
    ok = True
    for p in paths:
        try:
            info = detect(p)
            print(f"{info['file']:18s} rev={info['revision']} blocks={info['blocks']:>2} "
                  f"backend={info['backend']:14s} transform={info.get('transform','?'):12s}")
            if not info["ok"]:
                ok = False
        except Exception as e:
            print(f"{p.name:18s} ERROR: {e}")
            ok = False
    return 0 if ok else 2


if __name__ == "__main__":
    raise SystemExit(main())
