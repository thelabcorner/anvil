#!/usr/bin/env python3
"""Remote-only I20 C2 auxiliary-index walk-budget sweep.

The auxiliary-index inverse-BWT representation stores a checkpoint index array
whose stride `r` is an ENCODER POLICY: the wire already transmits `r` and
`auxiliary_count = 1 + floor((n-1)/r)`, and the decoder only requires that

  * `r` is a power of two,
  * 2 <= r <= transformed_size,
  * auxiliary_count == 1 + floor((transformed_size-1)/r),
  * auxiliary_count <= min(transformed_size, 1<<20),
  * every index lies in [1, transformed_size].

No decoder rule ties the checkpoint count to 1024, so this sweep changes no
wire semantics: it only selects which permitted `r` the encoder emits.  The
default walk budget (1024) reproduces the frozen I10-1A bytes exactly.

Subcommands
  prepare    encode every arm, verify byte-exact round-trips, attribute the
             charged index bytes explicitly, prove byte identity against the
             frozen I10-1A binary per file, and emit the decode command sets
             for the candidate and both binding references.
  rss        measure per-process decode peak RSS for every arm.
  summarize  combine bytes + RSS + paired timing into the byte/speed trade
             curve and the G2/G4/G5/G6 gate table.

This tool never runs a local codec measurement; it is CI-only by contract.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


SILESIA = (
    "dickens", "mozilla", "mr", "nci", "ooffice", "osdb",
    "reymont", "samba", "sao", "webster", "x-ray", "xml",
)

# Frozen I10-1A portfolio totals (docs/I10-AUX-UNBWT-RESULTS.md, run 35927623136
# for the auxiliary v2 arm and the frozen I9 bytes for the legacy arm).
FROZEN_AUX_ON_TOTAL = {"silesia": 46_466_339, "enwik8": 23_537_422}
FROZEN_AUX_OFF_TOTAL = {"silesia": 46_446_995, "enwik8": 23_534_368}

# Binding reference totals from the same frozen run.
FROZEN_XZ_TOTAL = {"silesia": 48_456_004, "enwik8": 24_831_648}
FROZEN_BROTLI_TOTAL = {"silesia": 49_383_136, "enwik8": 24_810_180}

CONTROL_WALKS = 1024

CANONICAL_FLAGS = ["--parse=ratio", "--ratio-context=off", "--ratio-lines=off"]


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def run(argv: list[str], *, stdout_path: Path | None = None) -> None:
    handle = None
    out: Any = subprocess.DEVNULL
    try:
        if stdout_path is not None:
            stdout_path.parent.mkdir(parents=True, exist_ok=True)
            handle = stdout_path.open("wb")
            out = handle
        p = subprocess.run(
            argv,
            stdout=out,
            stderr=subprocess.PIPE,
            check=False,
        )
        if p.returncode:
            raise RuntimeError(
                f"command failed rc={p.returncode}: {argv!r}\n"
                + p.stderr.decode(errors="replace")[-4000:]
            )
    finally:
        if handle is not None:
            handle.close()


def read_uvar(blob: bytes, pos: int) -> tuple[int, int]:
    value = 0
    shift = 0
    for _ in range(10):
        if pos >= len(blob):
            raise ValueError("truncated uvar")
        b = blob[pos]
        pos += 1
        value |= (b & 0x7F) << shift
        if not (b & 0x80):
            return value, pos
        shift += 7
    raise ValueError("uvar overflow")


def write_script(path: Path, commands: list[list[str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = ["#!/usr/bin/env bash", "set -euo pipefail"]
    lines.extend(shlex.join(cmd) for cmd in commands)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    path.chmod(0o755)


def canonical_files(corpus: str, root: Path) -> list[Path]:
    if corpus == "enwik8":
        p = root / "enwik8" / "enwik8"
        if not p.is_file():
            raise RuntimeError(f"missing canonical enwik8: {p}")
        return [p]
    if corpus == "silesia":
        d = root / "silesia"
        files = [d / name for name in SILESIA]
        missing = [str(p) for p in files if not p.is_file()]
        if missing:
            raise RuntimeError(f"missing canonical Silesia files: {missing}")
        return files
    raise RuntimeError(f"unsupported corpus: {corpus}")


def parse_walks(text: str) -> list[int]:
    values = sorted({int(x.strip()) for x in text.split(",") if x.strip()})
    if not values or any(v < 1 or v > (1 << 20) for v in values):
        raise argparse.ArgumentTypeError("walks must be integers in 1..1048576")
    if CONTROL_WALKS not in values:
        raise argparse.ArgumentTypeError(f"walks must include the {CONTROL_WALKS} control")
    return values


# ---------------------------------------------------------------------------
# mode-17 payload inspection
# ---------------------------------------------------------------------------


def parse_container(blob: bytes) -> dict[str, Any]:
    """Parse one whole-file rev-2 mode-17 ANVIL container."""
    if len(blob) < 5 or blob[:4] != b"ANV0" or blob[4] != 2:
        raise ValueError("not an ANVIL rev2 container")
    p = 5
    block_size, p = read_uvar(blob, p)
    total, p = read_uvar(blob, p)
    blen, p = read_uvar(blob, p)
    if p >= len(blob):
        raise ValueError("truncated block header")
    mode = blob[p]
    p += 1
    plen, p = read_uvar(blob, p)
    if p + 4 > len(blob):
        raise ValueError("truncated block crc")
    crc = int.from_bytes(blob[p:p + 4], "little")
    p += 4
    if mode != 17:
        raise ValueError(f"expected one whole-file mode-17 block, got mode {mode}")
    if p + plen != len(blob):
        raise ValueError("mode-17 payload length mismatch")
    payload = blob[p:]
    if plen < 3:
        raise ValueError("short ratio payload")
    transform = payload[0]
    backend = payload[1]
    xlen, inner = read_uvar(payload, 2)
    if transform != 0:
        raise ValueError(f"expected direct transform 0, got {transform}")
    return {
        "block_size": block_size,
        "total": total,
        "block_len": blen,
        "payload_len": plen,
        "crc32": crc,
        "transform": transform,
        "backend": backend,
        "transformed_len": xlen,
        "backend_payload": payload[inner:],
    }


def parse_aux_v2(backend_payload: bytes) -> dict[str, Any] | None:
    """Parse the additive v2 auxiliary framing (0xFE tag) of a BWT payload.

    Returns None for a legacy v1 BWT payload (no auxiliary framing).
    """
    if not backend_payload or backend_payload[0] != 0xFE:
        return None
    p = 1
    if p >= len(backend_payload):
        raise ValueError("truncated aux tag")
    post = backend_payload[p]
    p += 1
    rate, p = read_uvar(backend_payload, p)
    count, p = read_uvar(backend_payload, p)
    if (rate & (rate - 1)) != 0 or rate < 2:
        raise ValueError(f"aux rate {rate} is not a power of two >= 2")
    index_bytes = 4 * count
    if p + index_bytes > len(backend_payload):
        raise ValueError("truncated aux index array")
    return {
        "postcoder": post,
        "aux_rate": rate,
        "aux_count": count,
        "index_bytes": index_bytes,
        "index_offset": p,
    }


def parse_bwt_backend(backend_payload: bytes) -> dict[str, Any]:
    """Describe a BWT backend payload (bare or 0xFF subblock framed)."""
    if not backend_payload:
        raise ValueError("empty BWT backend payload")
    if backend_payload[0] == 0xFF:
        p = 1
        nsub, p = read_uvar(backend_payload, p)
        subblocks = []
        for _ in range(nsub):
            dlen, p = read_uvar(backend_payload, p)
            plen, p = read_uvar(backend_payload, p)
            sub = backend_payload[p:p + plen]
            p += plen
            subblocks.append({
                "decoded_len": dlen,
                "payload_len": plen,
                "aux": parse_aux_v2(sub),
            })
        if p != len(backend_payload):
            raise ValueError("BWT subblock trailing bytes")
        return {"subblocks": subblocks, "framed": True}
    return {"subblocks": [{"decoded_len": None, "payload_len": len(backend_payload),
                           "aux": parse_aux_v2(backend_payload)}], "framed": False}


def inspect_payload(path: Path) -> dict[str, Any]:
    info = parse_container(path.read_bytes())
    out: dict[str, Any] = {
        "backend": info["backend"],
        "transformed_len": info["transformed_len"],
        "block_len": info["block_len"],
    }
    if info["backend"] == 2:
        bwt = parse_bwt_backend(info["backend_payload"])
        aux_counts = [s["aux"] for s in bwt["subblocks"] if s["aux"]]
        if aux_counts:
            out["aux_rate"] = aux_counts[0]["aux_rate"]
            out["aux_count"] = sum(a["aux_count"] for a in aux_counts)
            out["index_bytes"] = sum(a["index_bytes"] for a in aux_counts)
            out["postcoder"] = aux_counts[0]["postcoder"]
            out["aux_subblocks"] = len(aux_counts)
        else:
            out["aux_rate"] = None
            out["aux_count"] = 0
            out["index_bytes"] = 0
            out["aux_subblocks"] = 0
        out["bwt_subblocks"] = len(bwt["subblocks"])
    return out


# ---------------------------------------------------------------------------
# encoding helpers
# ---------------------------------------------------------------------------


def encode_anvil(
    anvil: Path,
    src: Path,
    packed: Path,
    *,
    backend: str,
    aux: str,
    walks: int | None = None,
) -> None:
    packed.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        str(anvil), "c", str(src), str(packed),
        *CANONICAL_FLAGS,
        f"--ratio-backend={backend}",
        f"--bwt-aux={aux}",
    ]
    if walks is not None:
        cmd += ["--bwt-aux-walks", str(walks)]
    cmd.append("--quiet")
    run(cmd)


def verify_roundtrip(cmd: list[str], out: Path, src: Path) -> None:
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.exists():
        out.unlink()
    run(cmd)
    if not out.is_file():
        raise RuntimeError(f"decoder did not create output: {out}")
    if out.stat().st_size != src.stat().st_size or sha256(out) != sha256(src):
        raise RuntimeError(f"roundtrip mismatch: {src.name} via {cmd[0]}")


# ---------------------------------------------------------------------------
# prepare
# ---------------------------------------------------------------------------


def prepare(args: argparse.Namespace) -> None:
    root = args.root.resolve()
    out = args.out.resolve()
    anvil = args.anvil.resolve()
    brotli = args.brotli.resolve()
    frozen = args.frozen_anvil.resolve() if args.frozen_anvil else None
    walks = parse_walks(args.walks)
    files = canonical_files(args.corpus, root)
    xz_bin = shutil.which("xz")
    if not anvil.is_file() or not brotli.is_file() or not xz_bin:
        raise RuntimeError("missing ANVIL, brotli_lw or xz")

    out.mkdir(parents=True, exist_ok=True)
    packed_root = out / "packed"
    decoded_root = out / "decoded"
    decoded_root.mkdir(parents=True, exist_ok=True)

    source_rows: list[dict[str, Any]] = []
    brotli_meta: dict[str, dict[str, Any]] = {}
    bwt_meta: dict[tuple[str, int], dict[str, Any]] = {}
    bwt_off_meta: dict[str, dict[str, Any]] = {}

    for src in files:
        name = src.name
        n = src.stat().st_size
        digest = sha256(src)
        source_rows.append({
            "file": name,
            "input_bytes": n,
            "source_sha256": digest,
        })

        # Brotli direct payload: independent of the walk budget.
        bp = packed_root / "brotli" / f"{name}.anv"
        encode_anvil(anvil, src, bp, backend="brotli", aux="off")
        info = inspect_payload(bp)
        if info["backend"] != 1:
            raise RuntimeError(f"{name}: direct Brotli emitted backend {info['backend']}")
        brotli_meta[name] = {
            "path": str(bp),
            "bytes": bp.stat().st_size,
            "sha256": sha256(bp),
        }

        # BWT direct payload with auxiliary indexes, once per walk budget.
        for w in walks:
            wp = packed_root / "bwt" / f"walks-{w}" / f"{name}.anv"
            encode_anvil(anvil, src, wp, backend="bwt", aux="on", walks=w)
            winfo = inspect_payload(wp)
            if winfo["backend"] != 2:
                raise RuntimeError(f"{name}/W={w}: direct BWT emitted backend {winfo['backend']}")
            if winfo.get("aux_rate") is None:
                raise RuntimeError(f"{name}/W={w}: no auxiliary v2 framing found")
            bwt_meta[(name, w)] = {
                "path": str(wp),
                "bytes": wp.stat().st_size,
                "sha256": sha256(wp),
                "aux_rate": winfo["aux_rate"],
                "aux_count": winfo["aux_count"],
                "index_bytes": winfo["index_bytes"],
                "postcoder": winfo.get("postcoder"),
                "bwt_subblocks": winfo.get("bwt_subblocks"),
            }

        # Legacy (auxiliary-off) BWT payload: must stay byte-identical.
        op = packed_root / "bwt-off" / f"{name}.anv"
        encode_anvil(anvil, src, op, backend="bwt", aux="off")
        oinfo = inspect_payload(op)
        if oinfo["backend"] != 2:
            raise RuntimeError(f"{name}: direct BWT (aux off) emitted backend {oinfo['backend']}")
        if oinfo.get("aux_rate") is not None:
            raise RuntimeError(f"{name}: auxiliary framing present with --bwt-aux=off")
        bwt_off_meta[name] = {
            "path": str(op),
            "bytes": op.stat().st_size,
            "sha256": sha256(op),
        }

    # ------------------------------------------------------------------
    # Reconstruct the auto portfolio and verify every arm round-trips.
    # encode_ratio_block() considers Brotli first and replaces it only on a
    # strict size improvement, so the auto route is
    #   bwt   if bwt_bytes < brotli_bytes
    #   brotli otherwise
    # and the auto container is byte-identical to the direct container of the
    # winning backend (same transform/backend/xlen/payload framing).
    # ------------------------------------------------------------------
    rows: list[dict[str, Any]] = []
    decode_commands: dict[int, list[list[str]]] = {w: [] for w in walks}
    decode_files: dict[int, list[str]] = {w: [] for w in walks}

    for w in walks:
        (out / "timed" / f"walks-{w}").mkdir(parents=True, exist_ok=True)
        for src in files:
            name = src.name
            bm = brotli_meta[name]
            wm = bwt_meta[(name, w)]
            route = "bwt" if int(wm["bytes"]) < int(bm["bytes"]) else "brotli"
            selected = wm if route == "bwt" else bm
            dec = decoded_root / f"walks-{w}" / name
            cmd = [str(anvil), "d", str(selected["path"]), str(dec), "--quiet"]
            verify_roundtrip(cmd, dec, src)
            dec.unlink()
            timed_dec = out / "timed" / f"walks-{w}" / name
            decode_commands[w].append(
                [str(anvil), "d", str(selected["path"]), str(timed_dec), "--quiet"]
            )
            decode_files[w].append(name)
            rows.append({
                "corpus": args.corpus,
                "file": name,
                "walks": w,
                "route": route,
                "selected_bytes": selected["bytes"],
                "selected_sha256": selected["sha256"],
                "brotli_bytes": bm["bytes"],
                "bwt_bytes": wm["bytes"],
                "bwt_sha256": wm["sha256"],
                "aux_rate": wm["aux_rate"],
                "aux_count": wm["aux_count"],
                "index_bytes": wm["index_bytes"],
                "bwt_postcoder": wm["postcoder"],
            })

    # Legacy (auxiliary-off) portfolio: byte identity against the frozen I9 bytes.
    legacy_rows: list[dict[str, Any]] = []
    legacy_total = 0
    for src in files:
        name = src.name
        bm = brotli_meta[name]
        om = bwt_off_meta[name]
        route = "bwt" if int(om["bytes"]) < int(bm["bytes"]) else "brotli"
        selected = om if route == "bwt" else bm
        dec = decoded_root / "legacy" / name
        cmd = [str(anvil), "d", str(selected["path"]), str(dec), "--quiet"]
        verify_roundtrip(cmd, dec, src)
        dec.unlink()
        legacy_total += int(selected["bytes"])
        legacy_rows.append({
            "corpus": args.corpus,
            "file": name,
            "route": route,
            "selected_bytes": selected["bytes"],
            "selected_sha256": selected["sha256"],
        })

    # ------------------------------------------------------------------
    # Byte identity against the frozen I10-1A binary (Adopt-Gate section 6.1).
    # ------------------------------------------------------------------
    identity: dict[str, Any] = {"frozen_anvil": str(frozen) if frozen else None}
    if frozen is not None:
        ident_rows: list[dict[str, Any]] = []
        for src in files:
            name = src.name
            entry: dict[str, Any] = {"file": name}
            for label, backend, aux, walks in (
                ("brotli", "brotli", "off", None),
                ("bwt_aux_on", "bwt", "on", CONTROL_WALKS),
                ("bwt_aux_off", "bwt", "off", None),
            ):
                fp = packed_root / "frozen" / label / f"{name}.anv"
                encode_anvil(frozen, src, fp, backend=backend, aux=aux, walks=walks)
                f_sha = sha256(fp)
                f_bytes = fp.stat().st_size
                if label == "brotli":
                    new_sha, new_bytes = brotli_meta[name]["sha256"], brotli_meta[name]["bytes"]
                elif label == "bwt_aux_on":
                    new_sha = bwt_meta[(name, CONTROL_WALKS)]["sha256"]
                    new_bytes = bwt_meta[(name, CONTROL_WALKS)]["bytes"]
                else:
                    new_sha = bwt_off_meta[name]["sha256"]
                    new_bytes = bwt_off_meta[name]["bytes"]
                entry[label] = {
                    "frozen_bytes": f_bytes,
                    "frozen_sha256": f_sha,
                    "new_bytes": new_bytes,
                    "new_sha256": new_sha,
                    "identical": (f_sha == new_sha and f_bytes == new_bytes),
                }
                if not entry[label]["identical"]:
                    raise RuntimeError(
                        f"byte identity failed for {name}/{label}: "
                        f"frozen {f_sha} != new {new_sha}"
                    )
            ident_rows.append(entry)
        identity["rows"] = ident_rows
        identity["all_identical"] = all(
            r[k]["identical"] for r in ident_rows
            for k in ("brotli", "bwt_aux_on", "bwt_aux_off")
        )

    # ------------------------------------------------------------------
    # Binding reference arms (measured in the same job as the candidate).
    # ------------------------------------------------------------------
    brotli_commands: list[list[str]] = []
    brotli_outputs: list[str] = []
    xz_commands: list[list[str]] = []
    xz_outputs: list[str] = []
    ref_rows: list[dict[str, Any]] = []
    for src in files:
        name = src.name
        n = src.stat().st_size
        br = packed_root / "ref-brotli" / f"{name}.br"
        run([str(brotli), "c", str(src), str(br)])
        br_out = decoded_root / "ref-brotli" / name
        br_cmd = [str(brotli), "d", str(br), str(br_out), str(n)]
        verify_roundtrip(br_cmd, br_out, src)
        br_out.unlink()
        brotli_commands.append(br_cmd)
        brotli_outputs.append(str(br_out))

        xp = packed_root / "ref-xz" / f"{name}.xz"
        run([xz_bin, "-9e", "-c", str(src)], stdout_path=xp)
        xz_out = xp.with_suffix("")
        xz_cmd = [xz_bin, "-d", "-k", "-f", str(xp)]
        verify_roundtrip(xz_cmd, xz_out, src)
        xz_out.unlink()
        xz_commands.append(xz_cmd)
        xz_outputs.append(str(xz_out))

        ref_rows.append({
            "corpus": args.corpus,
            "file": name,
            "input_bytes": n,
            "brotli_bytes": br.stat().st_size,
            "xz_bytes": xp.stat().st_size,
        })

    # ------------------------------------------------------------------
    # Totals, control identity, and the explicit index-byte attribution.
    # ------------------------------------------------------------------
    by_walks: dict[str, Any] = {}
    for w in walks:
        wr = [r for r in rows if r["walks"] == w]
        total = sum(int(r["selected_bytes"]) for r in wr)
        index_total = sum(int(r["index_bytes"]) for r in wr)
        bwt_routes = [r["file"] for r in wr if r["route"] == "bwt"]
        by_walks[str(w)] = {
            "walks": w,
            "portfolio_bytes": total,
            "bwt_routes": len(bwt_routes),
            "bwt_routed_files": bwt_routes,
            "index_bytes_total": index_total,
            "per_file": wr,
        }

    control_total = by_walks[str(CONTROL_WALKS)]["portfolio_bytes"]
    expected_total = FROZEN_AUX_ON_TOTAL[args.corpus]
    if control_total != expected_total:
        raise RuntimeError(
            f"control portfolio identity failed: {control_total} != {expected_total}"
        )
    expected_legacy = FROZEN_AUX_OFF_TOTAL[args.corpus]
    if legacy_total != expected_legacy:
        raise RuntimeError(
            f"legacy (aux-off) portfolio identity failed: "
            f"{legacy_total} != {expected_legacy}"
        )

    xz_total = sum(int(r["xz_bytes"]) for r in ref_rows)
    brotli_total = sum(int(r["brotli_bytes"]) for r in ref_rows)
    if xz_total != FROZEN_XZ_TOTAL[args.corpus]:
        raise RuntimeError(
            f"xz reference total differs from the frozen baseline: "
            f"{xz_total} != {FROZEN_XZ_TOTAL[args.corpus]}"
        )
    if brotli_total != FROZEN_BROTLI_TOTAL[args.corpus]:
        raise RuntimeError(
            f"brotli reference total differs from the frozen baseline: "
            f"{brotli_total} != {FROZEN_BROTLI_TOTAL[args.corpus]}"
        )

    # The charged index cost must be exactly attributable: the BWT payload
    # delta between two walk budgets is the index-array delta plus at most a
    # few framing bytes (the uvar-encoded rate/count fields).
    attribution: list[dict[str, Any]] = []
    for w in walks:
        if w == CONTROL_WALKS:
            continue
        for src in files:
            name = src.name
            a = bwt_meta[(name, CONTROL_WALKS)]
            b = bwt_meta[(name, w)]
            if a["bwt_subblocks"] != b["bwt_subblocks"]:
                raise RuntimeError(f"{name}: subblock count changed between walk budgets")
            payload_delta = int(b["bytes"]) - int(a["bytes"])
            index_delta = int(b["index_bytes"]) - int(a["index_bytes"])
            framing_delta = payload_delta - index_delta
            if abs(framing_delta) > 8:
                raise RuntimeError(
                    f"{name}/W={w}: charged bytes do not reconcile with the index "
                    f"array (payload delta {payload_delta}, index delta {index_delta}, "
                    f"framing delta {framing_delta}); the postcoder selection must "
                    f"have changed"
                )
            attribution.append({
                "file": name,
                "walks": w,
                "aux_rate_before": a["aux_rate"],
                "aux_rate_after": b["aux_rate"],
                "rate_reduced": b["aux_rate"] < a["aux_rate"],
                "aux_count_before": a["aux_count"],
                "aux_count_after": b["aux_count"],
                "index_bytes_before": a["index_bytes"],
                "index_bytes_after": b["index_bytes"],
                "index_bytes_delta": index_delta,
                "bwt_payload_delta": payload_delta,
                "framing_bytes_delta": framing_delta,
                "postcoder_before": a["postcoder"],
                "postcoder_after": b["postcoder"],
                "postcoder_unchanged": a["postcoder"] == b["postcoder"],
            })

    # Emit decode command sets.
    manifests: dict[str, str] = {}
    for w in walks:
        sp = out / f"decode-walks-{w}.sh"
        write_script(sp, decode_commands[w])
        mp = out / f"decode-walks-{w}.json"
        mp.write_text(json.dumps({
            "schema": 1,
            "corpus": args.corpus,
            "walks": w,
            "files": decode_files[w],
            "commands": decode_commands[w],
        }, indent=2) + "\n", encoding="utf-8")
        manifests[f"walks-{w}"] = str(mp)

    write_script(out / "decode-brotli.sh", brotli_commands)
    (out / "brotli-commands.json").write_text(json.dumps({
        "schema": 1, "arm": "brotli", "commands": brotli_commands, "outputs": brotli_outputs,
    }, indent=2) + "\n", encoding="utf-8")
    write_script(out / "decode-xz.sh", xz_commands)
    (out / "xz-commands.json").write_text(json.dumps({
        "schema": 1, "arm": "xz", "commands": xz_commands, "outputs": xz_outputs,
    }, indent=2) + "\n", encoding="utf-8")
    manifests["brotli"] = str(out / "brotli-commands.json")
    manifests["xz"] = str(out / "xz-commands.json")

    fields = [
        "corpus", "file", "walks", "route", "selected_bytes", "selected_sha256",
        "brotli_bytes", "bwt_bytes", "bwt_sha256", "aux_rate", "aux_count",
        "index_bytes", "bwt_postcoder",
    ]
    with (out / "prepare.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)

    with (out / "attribution.csv").open("w", newline="", encoding="utf-8") as f:
        afields = list(attribution[0].keys()) if attribution else []
        w = csv.DictWriter(f, fieldnames=afields)
        w.writeheader()
        w.writerows(attribution)

    summary = {
        "schema": 1,
        "corpus": args.corpus,
        "source_bytes": sum(p.stat().st_size for p in files),
        "file_count": len(files),
        "walks": walks,
        "control_walks": CONTROL_WALKS,
        "expected_control_bytes": expected_total,
        "control_identity": "OK",
        "expected_legacy_bytes": expected_legacy,
        "legacy_identity": "OK",
        "legacy_total": legacy_total,
        "legacy_per_file": legacy_rows,
        "xz_total": xz_total,
        "brotli_total": brotli_total,
        "xz_surplus": FROZEN_XZ_TOTAL[args.corpus] - expected_total,
        "by_walks": by_walks,
        "attribution": attribution,
        "identity": identity,
        "reference_rows": ref_rows,
        "decode_manifests": manifests,
        "source": source_rows,
    }
    (out / "prepare.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps({
        "corpus": args.corpus,
        "control_identity": "OK",
        "legacy_identity": "OK",
        "xz_total": xz_total,
        "brotli_total": brotli_total,
        "by_walks": {
            k: {
                "portfolio_bytes": v["portfolio_bytes"],
                "delta_vs_control": v["portfolio_bytes"] - control_total,
                "index_bytes_total": v["index_bytes_total"],
                "bwt_routes": v["bwt_routes"],
            } for k, v in by_walks.items()
        },
    }, indent=2, sort_keys=True))


# ---------------------------------------------------------------------------
# rss
# ---------------------------------------------------------------------------


def measure_one_rss(cmd: list[str], tmp: Path) -> tuple[float, int]:
    time_bin = Path("/usr/bin/time")
    if not time_bin.is_file():
        raise RuntimeError("/usr/bin/time is required for RSS measurement")
    stamp = tmp / "time.txt"
    if stamp.exists():
        stamp.unlink()
    p = subprocess.run(
        [str(time_bin), "-f", "%e %M", "-o", str(stamp), "--", *cmd],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        check=False,
    )
    if p.returncode:
        raise RuntimeError(
            f"RSS command failed rc={p.returncode}: {cmd!r}\n"
            + p.stderr.decode(errors="replace")[-4000:]
        )
    parts = stamp.read_text(encoding="utf-8").strip().split()
    if len(parts) != 2:
        raise RuntimeError(f"bad GNU time output: {parts!r}")
    return float(parts[0]), int(parts[1])


def rss(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    rows: list[dict[str, Any]] = []
    by_arm: dict[str, Any] = {}

    arms: list[tuple[str, str]] = []
    for w in prep["walks"]:
        arms.append((f"walks-{w}", prep["decode_manifests"][f"walks-{w}"]))
    arms.append(("brotli", prep["decode_manifests"]["brotli"]))
    arms.append(("xz", prep["decode_manifests"]["xz"]))

    with tempfile.TemporaryDirectory(prefix="anvil-walk-rss-") as td0:
        tmp = Path(td0)
        for arm, manifest_path in arms:
            manifest = json.loads(Path(manifest_path).read_text(encoding="utf-8"))
            commands = manifest["commands"]
            names = manifest.get("files") or [f"index-{i}" for i in range(len(commands))]
            samples = []
            for i, cmd in enumerate(commands):
                elapsed, peak = measure_one_rss([str(x) for x in cmd], tmp)
                samples.append(peak)
                rows.append({
                    "corpus": prep["corpus"],
                    "arm": arm,
                    "file_index": i,
                    "file": names[i],
                    "seconds": elapsed,
                    "peak_rss_kib": peak,
                })
            by_arm[arm] = {
                "max_peak_rss_kib": max(samples),
                "sum_seconds": sum(
                    float(r["seconds"]) for r in rows if r["arm"] == arm
                ),
                "per_file": [r for r in rows if r["arm"] == arm],
            }

    with (out / "rss.csv").open("w", newline="", encoding="utf-8") as f:
        fields = ["corpus", "arm", "file_index", "file", "seconds", "peak_rss_kib"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)

    result = {
        "schema": 1,
        "corpus": prep["corpus"],
        "by_arm": by_arm,
    }
    (out / "rss.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps({k: v["max_peak_rss_kib"] for k, v in by_arm.items()}, indent=2))


# ---------------------------------------------------------------------------
# summarize
# ---------------------------------------------------------------------------


def load_pair(path: Path) -> dict[str, Any]:
    d = json.loads(path.read_text(encoding="utf-8"))
    return {
        "status": d.get("status"),
        "timing_valid": bool(d.get("timing_valid")),
        "ratio": float(d["paired_ratio_candidate_over_control"]),
        "ci95": [float(x) for x in d["paired_ratio_ci95"]],
        "control_median_s": float(d["control"]["median_s"]),
        "candidate_median_s": float(d["candidate"]["median_s"]),
        "control_cv": float(d["control"]["robust_cv"]),
        "candidate_cv": float(d["candidate"]["robust_cv"]),
    }


def summarize(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    prep = json.loads(args.prepare.read_text(encoding="utf-8"))
    timing_dir = args.timing_dir.resolve()
    rss_path = Path(args.rss) if args.rss else None
    rss_data = json.loads(rss_path.read_text(encoding="utf-8")) if rss_path else None

    control = int(prep["by_walks"][str(CONTROL_WALKS)]["portfolio_bytes"])
    xz_total = int(prep["xz_total"])
    brotli_total = int(prep["brotli_total"])

    rows: list[dict[str, Any]] = []
    for w in prep["walks"]:
        bw = prep["by_walks"][str(w)]
        total = int(bw["portfolio_bytes"])
        entry: dict[str, Any] = {
            "walks": w,
            "portfolio_bytes": total,
            "delta_vs_control": total - control,
            "delta_vs_xz": total - xz_total,
            "index_bytes_total": int(bw["index_bytes_total"]),
            "bwt_routes": int(bw["bwt_routes"]),
            "aux_rate_min": min(
                (int(r["aux_rate"]) for r in bw["per_file"] if r["route"] == "bwt"),
                default=None,
            ),
            "aux_rate_max": max(
                (int(r["aux_rate"]) for r in bw["per_file"] if r["route"] == "bwt"),
                default=None,
            ),
        }
        vs_xz = timing_dir / f"paired-walks-{w}-vs-xz.json"
        vs_br = timing_dir / f"paired-walks-{w}-vs-brotli.json"
        null = timing_dir / f"paired-null-walks-{w}.json"
        if vs_xz.is_file():
            entry["vs_xz"] = load_pair(vs_xz)
        if vs_br.is_file():
            entry["vs_brotli"] = load_pair(vs_br)
        if null.is_file():
            entry["null"] = load_pair(null)
        if rss_data is not None:
            arm = f"walks-{w}"
            if arm in rss_data["by_arm"]:
                entry["rss_kib"] = int(rss_data["by_arm"][arm]["max_peak_rss_kib"])
        rows.append(entry)

    if rss_data is not None:
        rss_xz = int(rss_data["by_arm"]["xz"]["max_peak_rss_kib"])
        rss_br = int(rss_data["by_arm"]["brotli"]["max_peak_rss_kib"])
        for entry in rows:
            if "rss_kib" in entry:
                entry["rss_ratio_vs_xz"] = entry["rss_kib"] / rss_xz
                entry["rss_ratio_vs_brotli"] = entry["rss_kib"] / rss_br
    else:
        rss_xz = rss_br = 0

    for entry in rows:
        entry["g2_bytes_below_xz"] = entry["portfolio_bytes"] < xz_total
        if "vs_xz" in entry:
            entry["g3_decode_ci_upper_le_2x_xz"] = entry["vs_xz"]["ci95"][1] <= 2.0
        if "vs_brotli" in entry:
            entry["g4_decode_ci_upper_le_2x_brotli"] = entry["vs_brotli"]["ci95"][1] <= 2.0
        if "rss_ratio_vs_xz" in entry:
            entry["g5_rss_ratio_le_2x_xz"] = entry["rss_ratio_vs_xz"] <= 2.0
            entry["g6_rss_ratio_le_2x_brotli"] = entry["rss_ratio_vs_brotli"] <= 2.0
        gates = [entry["g2_bytes_below_xz"]]
        for k in ("g3_decode_ci_upper_le_2x_xz", "g4_decode_ci_upper_le_2x_brotli",
                  "g5_rss_ratio_le_2x_xz", "g6_rss_ratio_le_2x_brotli"):
            if k in entry:
                gates.append(bool(entry[k]))
        entry["all_gates_pass"] = all(gates)
        null = entry.get("null")
        entry["null_valid"] = bool(null and null["timing_valid"]
                                   and null["ci95"][0] <= 1.0 <= null["ci95"][1]
                                   and not null.get("clears_speed_gate"))

    md = [
        f"# I20 C2 aux walk-budget sweep — {prep['corpus']}",
        "",
        f"- source bytes: {prep['source_bytes']:,}",
        f"- frozen control identity (W={CONTROL_WALKS}): {prep['control_identity']} "
        f"({prep['expected_control_bytes']:,} B)",
        f"- frozen legacy (aux off) identity: {prep['legacy_identity']} "
        f"({prep['expected_legacy_bytes']:,} B)",
        f"- xz -9e total: {xz_total:,} B   Brotli q11/lw30 total: {brotli_total:,} B",
        f"- xz surplus over the frozen control: {xz_total - control:,} B",
        "",
        "| W | portfolio bytes | Δ vs W=1024 | Δ vs xz | index bytes | aux r (min..max) | "
        "decode ratio vs xz (CI95) | decode ratio vs Brotli (CI95) | peak RSS MiB | "
        "RSS / xz | RSS / Brotli | G2 | G4 | G5 | G6 |",
        "|---:|---:|---:|---:|---:|---|---|---|---:|---:|---:|---|---|---|---|",
    ]
    for e in rows:
        vx = e.get("vs_xz")
        vb = e.get("vs_brotli")
        md.append(
            f"| {e['walks']} | {e['portfolio_bytes']:,} | {e['delta_vs_control']:+,} | "
            f"{e['delta_vs_xz']:+,} | {e['index_bytes_total']:,} | "
            f"{e['aux_rate_min']}..{e['aux_rate_max']} | "
            + (f"{vx['ratio']:.4f} [{vx['ci95'][0]:.4f}, {vx['ci95'][1]:.4f}] | " if vx else "n/a | ")
            + (f"{vb['ratio']:.4f} [{vb['ci95'][0]:.4f}, {vb['ci95'][1]:.4f}] | " if vb else "n/a | ")
            + (f"{e['rss_kib']/1024:.1f} | {e.get('rss_ratio_vs_xz', 0):.3f} | "
               f"{e.get('rss_ratio_vs_brotli', 0):.3f} | " if "rss_kib" in e else "n/a | n/a | n/a | ")
            + f"{'PASS' if e['g2_bytes_below_xz'] else 'FAIL'} | "
            + (f"{'PASS' if e.get('g4_decode_ci_upper_le_2x_brotli') else 'FAIL'} | " if vb else "n/a | ")
            + (f"{'PASS' if e.get('g5_rss_ratio_le_2x_xz') else 'FAIL'} | " if "rss_ratio_vs_xz" in e else "n/a | ")
            + (f"{'PASS' if e.get('g6_rss_ratio_le_2x_brotli') else 'FAIL'} | " if "rss_ratio_vs_brotli" in e else "n/a | ")
            + ("YES" if e["all_gates_pass"] else "no")
            + "\n"
        )
    md.extend([
        "",
        "G2 = total bytes strictly below xz -9e. G4 = paired decode ratio CI-upper vs "
        "Brotli q11/lw30 <= 2.0. G5/G6 = peak decode RSS ratio <= 2.0 vs xz / Brotli.",
        "G3 (decode CI-upper vs xz) is in the table as the first ratio column.",
        "",
    ])
    report = "\n".join(md)
    (out / "summary.md").write_text(report, encoding="utf-8")
    result = {
        "schema": 1,
        "corpus": prep["corpus"],
        "control_walks": CONTROL_WALKS,
        "control_portfolio_bytes": control,
        "xz_total": xz_total,
        "brotli_total": brotli_total,
        "rows": rows,
    }
    (out / "summary.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(report)


def main() -> None:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("prepare")
    p.add_argument("--corpus", choices=("silesia", "enwik8"), required=True)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--frozen-anvil", type=Path)
    p.add_argument("--brotli", type=Path, required=True)
    p.add_argument("--walks", default="1024,4096,16384,65536,262144")
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(fn=prepare)

    p = sub.add_parser("rss")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(fn=rss)

    p = sub.add_parser("summarize")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--rss", type=Path)
    p.add_argument("--timing-dir", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(fn=summarize)

    args = ap.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
