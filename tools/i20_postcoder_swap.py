#!/usr/bin/env python3
"""I20 candidate C5 — postcoder decode-cost swap: measurement driver.

Runs on a GitHub Actions runner only (see .github/workflows/anvil-i20-postcoder-swap.yml).
The driver is intentionally explicit about every number it emits, because the I20
shared evidence contract (docs/I20-PARETO-GATE-CLOSURE-PREREG-2026-10-21.md §4.8)
requires a run URL / job / workflow / commit SHA / artifact / corpus for every
claim.

Stages
  1. prepare  — compress every canonical file with the canonical ANVIL ratio
                command, with BWT forced, with Brotli forced and with xz -9e;
                verify the ANVIL round trip byte-exactly; determine which files
                the canonical router sends to the BWT backend; run the
                prototype byte sweep and the prototype stage profile.
  2. time     — same-job paired/interleaved timing with tools/paired_bench.py:
                  - A/A null over the whole portfolio (mandatory null pair)
                  - A/A null over the BWT-routed subset
                  - ANVIL vs Brotli over the whole portfolio (same-job reference)
                  - prototype-arith vs prototype-<candidate> over the BWT subset
                    (the postcoder swap effect, harness-matched)
                  - ANVIL vs prototype-<candidate> over the BWT subset
                    (the direct whole-codec comparison)
  3. estimate — the integration estimate in program currency: Silesia/enwik8
                portfolio byte delta (with the router effect), the implied
                whole-codec decode change, and the G2 / G4 / G6 outcomes.

Nothing here modifies the codec: this is a prototype measurement package.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import shlex
import shutil
import statistics
import subprocess
import sys
from pathlib import Path
from typing import Any

CANONICAL_SILESIA = [
    "dickens", "mozilla", "mr", "nci", "ooffice", "osdb",
    "reymont", "samba", "sao", "webster", "x-ray", "xml",
]

# The suite's documented per-STREAM decode cost units (src/anvil.cpp lines
# 1465-1470).  These are the PRIOR used to predict the decode-speed ranking.
SUITE_COST_UNITS = {
    "raw": 10.0, "rans4096": 40.0, "rans512": 35.0, "rans256": 30.0,
    "huffman": 22.0, "defexc": 20.0, "ctx-rans": 45.0,
}
STREAM_LAMBDA = 0.01  # the suite's binding J = L + lambda * C


def canonical_files(corpus: str, root: Path) -> list[Path]:
    if corpus == "enwik8":
        p = root / "enwik8" / "enwik8"
        if not p.is_file():
            raise RuntimeError(f"missing canonical enwik8: {p}")
        return [p]
    if corpus == "silesia":
        d = root / "silesia"
        files = [d / n for n in CANONICAL_SILESIA]
        missing = [str(p) for p in files if not p.is_file()]
        if missing:
            raise RuntimeError(f"missing canonical Silesia files: {missing}")
        return files
    raise RuntimeError(f"unsupported corpus: {corpus}")


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def run(argv: list[str], *, stdout_path: Path | None = None, check: bool = True) -> None:
    out: Any = subprocess.DEVNULL
    handle = None
    try:
        if stdout_path is not None:
            stdout_path.parent.mkdir(parents=True, exist_ok=True)
            handle = stdout_path.open("wb")
            out = handle
        p = subprocess.run(argv, stdout=out, stderr=subprocess.PIPE, check=False)
        if p.returncode and check:
            raise RuntimeError(
                f"command failed rc={p.returncode}: {argv!r}\n"
                + p.stderr.decode(errors="replace")[-4000:]
            )
    finally:
        if handle is not None:
            handle.close()


def write_script(path: Path, commands: list[list[str]]) -> None:
    lines = ["#!/usr/bin/env bash", "set -euo pipefail"]
    lines.extend(shlex.join(cmd) for cmd in commands)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    path.chmod(0o755)


def parse_anvil_container(path: Path) -> dict[str, Any]:
    """Minimal ANVIL revision-2 container parser (exact, no measurement).

    Wire (src/anvil.cpp `compress`/`decompress`, revision 2 / ratio route):
      "ANV0" | uvar revision_byte? no: 'A','N','V','0',0x02
      uvar block_size | uvar total_input
      per block: uvar blen | u8 mode | uvar plen | u32le crc | payload[plen]
    mode 17 payload: u8 transform | u8 backend | uvar transformed_size | backend bytes
    backend 2 (BWT) payload: 0xFE | u8 postcoder | uvar rate | uvar count | u32le*count | body
    """
    data = path.read_bytes()
    if data[:4] != b"ANV0":
        raise RuntimeError(f"not an ANVIL container: {path}")
    rev = data[4]
    pos = 5

    def uvar(p: int) -> tuple[int, int]:
        x = 0
        sh = 0
        while True:
            if p >= len(data):
                raise RuntimeError("truncated varint")
            b = data[p]
            p += 1
            x |= (b & 0x7F) << sh
            if not (b & 0x80):
                return x, p
            sh += 7

    def uvar_buf(buf: bytes, p: int) -> tuple[int, int]:
        x = 0
        sh = 0
        while True:
            if p >= len(buf):
                raise RuntimeError("truncated varint")
            b = buf[p]
            p += 1
            x |= (b & 0x7F) << sh
            if not (b & 0x80):
                return x, p
            sh += 7

    block_size, pos = uvar(pos)
    total, pos = uvar(pos)
    blocks = []
    while pos < len(data):
        blen, pos = uvar(pos)
        if pos >= len(data):
            raise RuntimeError("truncated block header")
        mode = data[pos]
        pos += 1
        plen, pos = uvar(pos)
        pos += 4  # crc32
        payload = data[pos:pos + plen]
        pos += plen
        blocks.append({"blen": blen, "mode": mode, "payload": payload})
    out: dict[str, Any] = {
        "revision": rev, "block_size": block_size, "total": total,
        "file_bytes": len(data), "blocks": len(blocks),
    }
    if len(blocks) == 1 and blocks[0]["mode"] == 17:
        pl = blocks[0]["payload"]
        transform = pl[0]
        backend = pl[1]
        # the xlen varint lives INSIDE the ratio payload, not in the file buffer
        xlen, p2 = uvar_buf(pl, 2)
        out.update({
            "transform": transform, "backend": backend, "transformed_size": xlen,
            "ratio_payload_bytes": len(pl),
        })
        out["backend_payload_bytes"] = len(pl) - p2
        out["container_overhead_bytes"] = len(data) - len(pl)
        if backend == 2:
            bp = pl[p2:]
            out["postcoder"] = bp[1] if bp and bp[0] == 0xFE else bp[0]
            out["aux_framed"] = bool(bp and bp[0] == 0xFE)
    return out


# ---------------------------------------------------------------------------
# stage 1: prepare
# ---------------------------------------------------------------------------
def stage_prepare(args: argparse.Namespace) -> dict[str, Any]:
    root = args.root.resolve()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    files = canonical_files(args.corpus, root)

    packed = out / "packed"
    decoded = out / "decoded"
    sweep_dir = out / "sweep"
    profile_dir = out / "profile"
    post_dir = out / "post"
    for d in (packed, decoded, sweep_dir, profile_dir, post_dir):
        d.mkdir(parents=True, exist_ok=True)

    rows: list[dict[str, Any]] = []
    for src in files:
        name = src.name
        n = src.stat().st_size
        digest = sha256(src)

        anv = packed / f"{name}.anv"
        run([
            str(args.anvil), "c", str(src), str(anv),
            "--parse=ratio", "--ratio-backend=auto",
            "--ratio-context=off", "--ratio-lines=off",
            "--bwt-aux=on", "--quiet",
        ])
        # correctness gate: the canonical ANVIL payload must decode byte-exactly
        dec = decoded / name
        run([str(args.anvil), "d", str(anv), str(dec), "--quiet"])
        if not dec.is_file():
            raise RuntimeError(f"decoder did not create output: {dec}")
        if dec.stat().st_size != n or sha256(dec) != digest:
            raise RuntimeError(f"canonical round-trip mismatch: {name}")
        dec.unlink()

        info = parse_anvil_container(anv)
        canon_sz = anv.stat().st_size
        # The canonical router picks the smaller COMPLETE payload; the backend id
        # in the ratio envelope records which one it picked.
        bwt_routed = (info.get("backend") == 2)

        br = packed / f"{name}.br"
        run([str(args.brotli), "c", str(src), str(br)])
        br_sz = br.stat().st_size
        xp = packed / f"{name}.xz"
        run([str(args.xz), "-9e", "-c", str(src)], stdout_path=xp)
        xz_sz = xp.stat().st_size

        # The Brotli-forced ANVIL size is needed only for the BWT-routed files
        # (it is the router's alternative for them).  For a Brotli-routed file
        # the canonical size already IS the Brotli-forced size.
        if bwt_routed:
            anv_br = packed / f"{name}.brt.anv"
            run([
                str(args.anvil), "c", str(src), str(anv_br),
                "--parse=ratio", "--ratio-backend=brotli",
                "--ratio-context=off", "--ratio-lines=off",
                "--bwt-aux=on", "--quiet",
            ])
            brt_sz = anv_br.stat().st_size
        else:
            brt_sz = canon_sz

        rows.append({
            "corpus": args.corpus,
            "file": name,
            "input_bytes": n,
            "source_sha256": digest,
            "anvil_canonical_bytes": canon_sz,
            "anvil_brotli_forced_bytes": brt_sz,
            "brotli_ref_bytes": br_sz,
            "xz9e_bytes": xz_sz,
            "bwt_routed": bwt_routed,
            "backend_id": info.get("backend"),
            "postcoder_id": info.get("postcoder"),
            "transform_id": info.get("transform"),
            "transformed_size": info.get("transformed_size"),
        })
        print(
            f"{name}: canonical={canon_sz:,} brotli_forced={brt_sz:,} "
            f"brotli_ref={br_sz:,} xz9e={xz_sz:,} backend={info.get('backend')} "
            f"transform={info.get('transform')} postcoder={info.get('postcoder')} "
            f"bwt_routed={bwt_routed}",
            flush=True,
        )

    with (out / "bytes.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)

    # ---- prototype sweep + stage profile on every BWT-routed file ----------
    sweep_rows: list[dict[str, Any]] = []
    for src, r in zip(files, rows):
        if not r["bwt_routed"]:
            continue
        name = r["file"]
        anv = packed / f"{name}.anv"
        sweep_out = sweep_dir / f"{name}.jsonl"
        run([
            str(args.proto), "sweep", f"--anvil={anv}", f"--out={sweep_out}",
            f"--outdir={post_dir}", "--full-grid",
        ])
        prof_out = profile_dir / f"{name}.json"
        run([
            str(args.proto), "profile", f"--anvil={anv}", "--reps=3",
        ], stdout_path=prof_out)
        with sweep_out.open("r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                obj = json.loads(line)
                obj["corpus"] = args.corpus
                sweep_rows.append(obj)
        print(f"swept {name}", flush=True)

    with (out / "sweep.jsonl").open("w", encoding="utf-8") as f:
        for obj in sweep_rows:
            f.write(json.dumps(obj, sort_keys=True) + "\n")

    summary = {
        "schema": 1,
        "corpus": args.corpus,
        "files": len(rows),
        "bwt_routed": [r["file"] for r in rows if r["bwt_routed"]],
        "brotli_routed": [r["file"] for r in rows if not r["bwt_routed"]],
        "totals": {
            "anvil_canonical": sum(r["anvil_canonical_bytes"] for r in rows),
            "anvil_brotli_forced": sum(r["anvil_brotli_forced_bytes"] for r in rows),
            "brotli_ref": sum(r["brotli_ref_bytes"] for r in rows),
            "xz9e": sum(r["xz9e_bytes"] for r in rows),
        },
        "roundtrip": "OK",
    }
    (out / "prepare-summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(summary, indent=2, sort_keys=True))
    return summary


# ---------------------------------------------------------------------------
# stage 2: paired timing
# ---------------------------------------------------------------------------
def measure_rss_kib(cmd: list[str], tmp: Path) -> int:
    time_bin = Path("/usr/bin/time")
    if not time_bin.is_file():
        raise RuntimeError("/usr/bin/time is required for RSS measurement")
    rss_file = tmp / "rss.txt"
    if rss_file.exists():
        rss_file.unlink()
    p = subprocess.run(
        [str(time_bin), "-f", "%M", "-o", str(rss_file), "--", *cmd],
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, check=False,
    )
    if p.returncode:
        raise RuntimeError(f"RSS command failed rc={p.returncode}: {cmd!r}\n"
                           + p.stderr.decode(errors="replace")[-2000:])
    return int(rss_file.read_text(encoding="utf-8").strip())


def stage_time(args: argparse.Namespace) -> dict[str, Any]:
    out = args.out.resolve()
    prep = json.loads((out / "prepare-summary.json").read_text(encoding="utf-8"))
    files = canonical_files(args.corpus, args.root.resolve())
    by_name = {p.name: p for p in files}
    packed = out / "packed"
    decoded = out / "decoded"
    decoded.mkdir(parents=True, exist_ok=True)

    bwt_names = prep["bwt_routed"]
    all_names = [p.name for p in files]

    # candidate selection: byte-feasible candidates, ranked by the suite J prior
    selection = select_candidates(out, prep)
    print("selected candidates:", json.dumps(selection, indent=2), flush=True)

    arms: dict[str, list[str]] = {}

    def anvil_cmd(name: str) -> list[str]:
        return [str(args.anvil), "d", str(packed / f"{name}.anv"), str(decoded / name), "--quiet"]

    def proto_cmd(name: str, post: str, postfile: Path | None) -> list[str]:
        if post == "arith":
            return [str(args.proto), "decode", f"--anvil={packed / f'{name}.anv'}",
                    f"--out={decoded / name}", "--post=arith"]
        if post == "shipped":
            # the repository's own postcoder-2 decode path (no token materialisation)
            return [str(args.proto), "decode", f"--anvil={packed / f'{name}.anv'}",
                    f"--out={decoded / name}", "--post=shipped"]
        assert postfile is not None
        return [str(args.proto), "decode", f"--anvil={packed / f'{name}.anv'}",
                f"--out={decoded / name}", "--post=candidate", f"--postfile={postfile}"]

    def brotli_cmd(name: str) -> list[str]:
        return [str(args.brotli), "d", str(packed / f"{name}.br"), str(decoded / name), str(by_name[name].stat().st_size)]

    def xz_cmd(name: str) -> list[str]:
        xp = packed / f"{name}.xz"
        return [str(args.xz), "-d", "-k", "-f", str(xp)]

    def make(name: str, cmds: list[list[str]]) -> str:
        p = out / f"arm-{name}.sh"
        write_script(p, cmds)
        return str(p)

    arms["anvil_all"] = make("anvil-all", [anvil_cmd(n) for n in all_names])
    arms["anvil_bwt"] = make("anvil-bwt", [anvil_cmd(n) for n in bwt_names])
    arms["brotli_all"] = make("brotli-all", [brotli_cmd(n) for n in all_names])
    arms["xz_all"] = make("xz-all", [xz_cmd(n) for n in all_names])
    arms["proto_arith"] = make("proto-arith", [proto_cmd(n, "arith", None) for n in bwt_names])
    arms["proto_shipped"] = make("proto-shipped", [proto_cmd(n, "shipped", None) for n in bwt_names])
    for cand in selection["timed"]:
        cmds = []
        for n in bwt_names:
            pf = out / "post" / f"{n}.{cand}.post"
            if not pf.is_file():
                raise RuntimeError(f"missing candidate payload: {pf}")
            cmds.append(proto_cmd(n, "candidate", pf))
        arms[f"proto_{cand}"] = make("proto-" + cand.replace(":", "_").replace("+", "_"), cmds)

    driver = Path("tools/paired_bench.py")
    if not driver.is_file():
        driver = Path(__file__).resolve().parent / "paired_bench.py"

    comparisons: list[tuple[str, str, str]] = [
        # (label, control arm, candidate arm)
        ("null-all", "anvil_all", "anvil_all"),
        ("null-bwt", "anvil_bwt", "anvil_bwt"),
        ("brotli", "brotli_all", "anvil_all"),
        ("xz", "xz_all", "anvil_all"),
        # M: the cost of materialising the MTF token stream (the shipped
        # postcoder path does not materialise it; every stream-suite decoder
        # necessarily does).
        ("harness", "proto_shipped", "proto_arith"),
    ]
    for cand in selection["timed"]:
        key = cand.replace(":", "_").replace("+", "_")
        # swap-shipped: the repository's own postcoder-2 decode path vs the
        # candidate path, both inside the SAME prototype harness.  This is the
        # estimator for the change in the SHIPPED codec's postcoder stage: the
        # shipped path does not materialise the token stream, the candidate path
        # necessarily does (a stream decoder returns a vector), and that
        # difference is part of the real cost of the swap.
        comparisons.append((f"swap-shipped-{key}", "proto_shipped", f"proto_{cand}"))
        # swap-arith: the harness-matched swap effect with the token
        # materialisation common to both arms (the difference between the two
        # estimators is exactly the materialisation cost M).
        comparisons.append((f"swap-arith-{key}", "proto_arith", f"proto_{cand}"))
        comparisons.append((f"whole-{key}", "anvil_bwt", f"proto_{cand}"))

    # ---- per-candidate stage profile (the trade-curve throughput column) ---
    for cand in selection["timed"]:
        for n in bwt_names:
            pf = out / "post" / f"{n}.{cand}.post"
            if not pf.is_file():
                continue
            prof_out = profile_dir / f"{n}.{cand}.json"
            run([
                str(args.proto), "profile", f"--anvil={packed / f'{n}.anv'}",
                f"--postfile={pf}", "--reps=3",
            ], stdout_path=prof_out)

    # ---- peak decode RSS of the ANVIL arm and of the prototype arm -------
    # C5 does not change the representation, so the peak-RSS axis is expected to
    # be unchanged; this measurement bounds the postcoder's own RSS share.
    rss_rows: list[dict[str, Any]] = []
    if bwt_names:
        import tempfile
        with tempfile.TemporaryDirectory(prefix="i20-rss-") as td:
            tmp = Path(td)
            for n in bwt_names:
                rss_rows.append({
                    "file": n,
                    "arm": "anvil",
                    "max_rss_kib": measure_rss_kib(anvil_cmd(n), tmp),
                })
                for cand in selection["timed"]:
                    pf = out / "post" / f"{n}.{cand}.post"
                    rss_rows.append({
                        "file": n,
                        "arm": f"proto:{cand}",
                        "max_rss_kib": measure_rss_kib(proto_cmd(n, "candidate", pf), tmp),
                    })
        with (out / "rss.csv").open("w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=["file", "arm", "max_rss_kib"])
            w.writeheader()
            w.writerows(rss_rows)

    results: dict[str, Any] = {}
    for label, control, candidate in comparisons:
        cmd = [
            sys.executable, str(driver),
            "--control-json", json.dumps([ "bash", arms[control] ]),
            "--candidate-json", json.dumps([ "bash", arms[candidate] ]),
            "--reps", str(args.reps),
            "--warmups", "2",
            "--seed", "41246",
            "--bootstrap", "20000",
            "--epsilon", "0.02",
            "--max-cv", "0.15",
            "--cpu", "0",
            "--ambient-reps", "5",
            "--ambient-iterations", "250000",
            "--ambient-max-cv", "0.10",
            "--out", str(out / f"paired-{label}.csv"),
            "--summary", str(out / f"paired-{label}.json"),
            "--stdout-dir", str(out / f"stdout-{label}"),
        ]
        print("RUN", label, flush=True)
        p = subprocess.run(cmd, check=False)
        if p.returncode:
            print(f"  paired_bench {label} failed rc={p.returncode}", flush=True)
            results[label] = {"error": f"paired_bench rc={p.returncode}"}
            continue
        results[label] = json.loads((out / f"paired-{label}.json").read_text(encoding="utf-8"))

    (out / "timing-summary.json").write_text(
        json.dumps(results, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    (out / "rss-summary.json").write_text(
        json.dumps(rss_rows, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return results


# ---------------------------------------------------------------------------
# candidate selection (byte feasibility first, then the suite J prior)
# ---------------------------------------------------------------------------
def select_candidates(out: Path, prep: dict[str, Any]) -> dict[str, Any]:
    """Rank the swept candidates.

    Byte feasibility is evaluated with the ROUTER EFFECT: if the new BWT
    payload of a file would exceed its Brotli-forced payload, the canonical
    router would send that file to Brotli instead, and the charged cost of
    that file becomes (brotli_forced - canonical) instead of the BWT delta.
    """
    rows = list(csv.DictReader((out / "bytes.csv").open("r", encoding="utf-8")))
    bwt_rows = {r["file"]: r for r in rows if r["bwt_routed"] == "True"}
    xz_total = int(prep["totals"]["xz9e"])
    canon_total = int(prep["totals"]["anvil_canonical"])

    sweep: dict[str, dict[str, Any]] = {}
    with (out / "sweep.jsonl").open("r", encoding="utf-8") as f:
        for line in f:
            obj = json.loads(line)
            if obj.get("field") == "header":
                continue
            sweep.setdefault(obj["candidate"], {})[obj["file"]] = obj

    ranked = []
    for cand, per_file in sweep.items():
        if cand == "arith":
            continue
        delta = 0
        feasible = True
        detail = {}
        for name, r in bwt_rows.items():
            o = per_file.get(name)
            if o is None or not o.get("ok"):
                feasible = False
                break
            new_bwt = int(o["new_file_bytes"])
            brt = int(r["anvil_brotli_forced_bytes"])
            canon = int(r["anvil_canonical_bytes"])
            d = new_bwt - canon
            if new_bwt > brt:
                d = brt - canon  # router flips the file to Brotli
            delta += d
            detail[name] = d
        if not feasible:
            continue
        new_total = canon_total + delta
        row0 = next(iter(per_file.values()))
        mtok = row0["mtok_name"]
        mrun = row0["mrun_name"]
        if cand.startswith("single:"):
            # one stream: the cost unit is that stream's unit only
            cost = SUITE_COST_UNITS.get(mtok, 0.0)
        else:
            # two streams: the cost units add
            cost = SUITE_COST_UNITS.get(mtok, 0.0) + SUITE_COST_UNITS.get(mrun, 0.0)
        j_prior = delta + STREAM_LAMBDA * cost
        ranked.append({
            "candidate": cand,
            "portfolio_delta_bytes": delta,
            "new_total_bytes": new_total,
            "xz_total_bytes": xz_total,
            "g2_pass": new_total < xz_total,
            "surplus_vs_xz": xz_total - new_total,
            "suite_cost_units": cost,
            "j_prior": j_prior,
            "per_file_delta": detail,
        })
    ranked.sort(key=lambda d: (d["portfolio_delta_bytes"], d["suite_cost_units"]))
    feasible = [d for d in ranked if d["g2_pass"]]
    # the J prior: smallest L first, then the cheapest decode cost unit
    j_order = sorted(feasible, key=lambda d: d["j_prior"])
    timed = []
    for d in j_order:
        if d["candidate"] not in timed:
            timed.append(d["candidate"])
        if len(timed) >= args_max_timed():
            break
    return {"ranked": ranked, "j_order": [d["candidate"] for d in j_order], "timed": timed}


def args_max_timed() -> int:
    return int(os.environ.get("I20_MAX_TIMED", "2"))


# ---------------------------------------------------------------------------
# stage 3: the integration estimate
# ---------------------------------------------------------------------------
def stage_estimate(args: argparse.Namespace) -> dict[str, Any]:
    out = args.out.resolve()
    prep = json.loads((out / "prepare-summary.json").read_text(encoding="utf-8"))
    timing_path = out / "timing-summary.json"
    timing = json.loads(timing_path.read_text(encoding="utf-8")) if timing_path.is_file() else {}
    selection = select_candidates(out, prep)

    # ---- stage-1 attribution (the prototype's own stage profile) ----------
    profiles = []
    for p in sorted((out / "profile").glob("*.json")):
        try:
            profiles.append(json.loads(p.read_text(encoding="utf-8")))
        except Exception:
            pass
    prof_summary = {
        "files": len(profiles),
        "entropy_share_mean": statistics.mean([p["entropy_share"] for p in profiles]) if profiles else None,
        "postcoder_share_mean": statistics.mean([p["postcoder_share"] for p in profiles]) if profiles else None,
        "rle_share_mean": statistics.mean([p["rle_share"] for p in profiles]) if profiles else None,
        "unbwt_share_mean": statistics.mean([p["unbwt_share"] for p in profiles]) if profiles else None,
        "per_file": [{"file": p.stem, **{k: p[k] for k in
                     ("transformed_size", "entropy_s", "mtf_s", "rle_s", "unbwt_s", "total_s",
                      "entropy_share", "postcoder_share", "unbwt_share")}} for p in profiles],
    }

    # ---- the whole-codec decode time (same job) --------------------------
    def med(label: str, side: str) -> float | None:
        d = timing.get(label)
        if not d or "error" in d:
            return None
        return float(d[side]["median_s"])

    anvil_all = med("null-all", "control")
    anvil_bwt = med("null-bwt", "control")
    brotli_pair = timing.get("brotli", {})
    xz_pair = timing.get("xz", {})

    per_candidate = []
    harness = timing.get("harness", {})
    for cand in selection["timed"]:
        key = cand.replace(":", "_").replace("+", "_")
        shipped_swap = timing.get(f"swap-shipped-{key}", {})
        arith_swap = timing.get(f"swap-arith-{key}", {})
        whole = timing.get(f"whole-{key}", {})

        def delta_of(pair: dict[str, Any]) -> float | None:
            c = (pair.get("control") or {}).get("median_s")
            k = (pair.get("candidate") or {}).get("median_s")
            if c is None or k is None:
                return None
            return float(k) - float(c)

        d_true = delta_of(shipped_swap)     # shipped postcoder path -> candidate path
        d_proto = delta_of(arith_swap)      # harness-matched swap effect
        d_materialise = delta_of(harness)   # M: token-stream materialisation cost
        entry = {
            "candidate": cand,
            "byte": next((d for d in selection["ranked"] if d["candidate"] == cand), None),
            "swap_shipped_pair": {
                "ratio_candidate_over_control": shipped_swap.get("paired_ratio_candidate_over_control"),
                "ci95": shipped_swap.get("paired_ratio_ci95"),
                "timing_valid": shipped_swap.get("timing_valid"),
                "status": shipped_swap.get("status"),
                "control_median_s": (shipped_swap.get("control") or {}).get("median_s"),
                "candidate_median_s": (shipped_swap.get("candidate") or {}).get("median_s"),
            },
            "swap_arith_pair": {
                "ratio_candidate_over_control": arith_swap.get("paired_ratio_candidate_over_control"),
                "ci95": arith_swap.get("paired_ratio_ci95"),
                "timing_valid": arith_swap.get("timing_valid"),
                "status": arith_swap.get("status"),
                "control_median_s": (arith_swap.get("control") or {}).get("median_s"),
                "candidate_median_s": (arith_swap.get("candidate") or {}).get("median_s"),
            },
            "whole_pair": {
                "ratio_candidate_over_control": whole.get("paired_ratio_candidate_over_control"),
                "ci95": whole.get("paired_ratio_ci95"),
                "timing_valid": whole.get("timing_valid"),
                "status": whole.get("status"),
                "control_median_s": (whole.get("control") or {}).get("median_s"),
                "candidate_median_s": (whole.get("candidate") or {}).get("median_s"),
            },
            "delta_shipped_postcoder_stage": d_true,
            "delta_swap_effect_harness_matched": d_proto,
            "delta_token_materialisation_s": d_materialise,
        }
        # implied whole-codec change on the whole portfolio
        if d_true is not None and anvil_all is not None:
            new_all = anvil_all + d_true
            entry["implied_whole_portfolio_decode_s"] = new_all
            entry["implied_whole_portfolio_speedup"] = anvil_all / new_all if new_all > 0 else None
            # G4: the same-job ANVIL-vs-Brotli ratio scaled by the decode change
            if brotli_pair.get("timing_valid"):
                base_ratio = float(brotli_pair["paired_ratio_candidate_over_control"])
                base_hi = float(brotli_pair["paired_ratio_ci95"][1])
                base_lo = float(brotli_pair["paired_ratio_ci95"][0])
                scale = new_all / anvil_all
                entry["g4_ratio_point"] = base_ratio * scale
                entry["g4_ci95_scaled"] = [base_lo * scale, base_hi * scale]
                entry["g4_pass"] = (base_hi * scale) <= 2.0
            if xz_pair.get("timing_valid"):
                base_ratio = float(xz_pair["paired_ratio_candidate_over_control"])
                base_hi = float(xz_pair["paired_ratio_ci95"][1])
                base_lo = float(xz_pair["paired_ratio_ci95"][0])
                scale = new_all / anvil_all
                entry["g3_ratio_point"] = base_ratio * scale
                entry["g3_ci95_scaled"] = [base_lo * scale, base_hi * scale]
                entry["g3_pass"] = (base_hi * scale) <= 2.0
        per_candidate.append(entry)

    # ---- the trade curve -------------------------------------------------
    trade_rows: list[dict[str, Any]] = []
    sweep_path = out / "sweep.jsonl"
    if sweep_path.is_file():
        with sweep_path.open("r", encoding="utf-8") as f:
            for line in f:
                obj = json.loads(line)
                if obj.get("field") == "header":
                    continue
                prof = out / "profile" / f"{obj['file']}.{obj['candidate']}.json"
                p = {}
                if prof.is_file():
                    try:
                        p = json.loads(prof.read_text(encoding="utf-8"))
                    except Exception:
                        p = {}
                trade_rows.append({
                    "corpus": args.corpus,
                    "file": obj["file"],
                    "candidate": obj["candidate"],
                    "mtok": obj.get("mtok"),
                    "mrun": obj.get("mrun"),
                    "old_post_body_bytes": obj.get("old_post_body_bytes"),
                    "new_post_body_bytes": obj.get("new_post_body_bytes"),
                    "new_file_bytes": obj.get("new_file_bytes"),
                    "delta_bytes": obj.get("delta_bytes"),
                    "ok": obj.get("ok"),
                    "note": obj.get("note", ""),
                    "entropy_s": p.get("entropy_s"),
                    "mtf_s": p.get("mtf_s"),
                    "rle_s": p.get("rle_s"),
                    "unbwt_s": p.get("unbwt_s"),
                    "total_s": p.get("total_s"),
                    "postcoder_share": p.get("postcoder_share"),
                    "entropy_mb_s": p.get("entropy_mb_s"),
                })
        with (out / "trade.csv").open("w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=list(trade_rows[0].keys()))
            w.writeheader()
            w.writerows(trade_rows)

    estimate = {
        "schema": 1,
        "corpus": args.corpus,
        "stage1_postcoder_attribution": prof_summary,
        "same_job_decode_s": {
            "anvil_portfolio_median_s": anvil_all,
            "anvil_bwt_subset_median_s": anvil_bwt,
            "brotli_pair": brotli_pair,
            "xz_pair": xz_pair,
        },
        "candidates": per_candidate,
        "selection": {
            "timed": selection["timed"],
            "j_order": selection["j_order"],
            "ranked": selection["ranked"],
        },
        "totals": prep["totals"],
        "bwt_routed": prep["bwt_routed"],
    }
    (out / "estimate.json").write_text(
        json.dumps(estimate, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(estimate, indent=2, sort_keys=True))
    return estimate


# ---------------------------------------------------------------------------
def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description="I20 C5 postcoder swap measurement driver")
    ap.add_argument("stage", choices=("prepare", "time", "estimate", "all"))
    ap.add_argument("--corpus", choices=("silesia", "enwik8"), required=True)
    ap.add_argument("--root", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--anvil", type=Path, required=True)
    ap.add_argument("--proto", type=Path, required=True)
    ap.add_argument("--brotli", type=Path, required=True)
    ap.add_argument("--xz", default="xz")
    ap.add_argument("--reps", type=int, default=9)
    return ap


def main() -> None:
    args = build_parser().parse_args()
    args.root = args.root.resolve()
    args.out = args.out.resolve()
    args.anvil = args.anvil.resolve()
    args.proto = args.proto.resolve()
    args.brotli = args.brotli.resolve()
    if args.stage in ("prepare", "all"):
        stage_prepare(args)
    if args.stage in ("time", "all"):
        stage_time(args)
    if args.stage in ("estimate", "all"):
        stage_estimate(args)


if __name__ == "__main__":
    main()
