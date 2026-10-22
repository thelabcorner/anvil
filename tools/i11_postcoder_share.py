#!/usr/bin/env python3
"""I11 / D2 - the postcoder's share of BWT-path decode (same-job differential).

This tool measures, for a fixed always-BWT file, INSIDE ONE GitHub Actions job:

  (1) the postcoder stage alone  (MTF + zero-run RLE + adaptive arithmetic -> the
      BWT string), with the inverse BWT deliberately NOT run;
  (2) the inverse BWT alone, over that SAME BWT string;
  (3) the end-to-end ANVIL decode of the same payload (the shipped `anvil d`);
  (4) the reference `brotli_lw` one-shot decode of the same bytes;
  (5) ANVIL's streaming Brotli decode (`brotli_lw_decode_stream`, 256 KiB output
      window) against (4).

The stage times come from `prototypes/i11-postcoder-share/stage_probe.cpp`, a
standalone probe that carries VERBATIM COPIES of the repo's own postcoder code
and links the vendored `libsais` inverse.  `src/anvil.cpp` is NOT modified.

The end-to-end and the reference decodes are timed with `tools/paired_bench.py`
under the frozen parameters of the shared evidence contract, each with a
mandatory A/A null whose CI must span 1.0.

Every number this tool produces is a CI artefact.  Nothing here is allowed to
produce a timing or byte number on a developer workstation.

Subcommands
-----------
  prepare    encode the canonical arms, verify byte-exact round trips, assert
             the BWT route, and record the container anatomy.
  measure    run the stage probe and the paired timing series.
  summarize  combine the artefacts into the machine-readable postcoder share and
             evaluate the pre-registered decision rule.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import shutil
import statistics
import subprocess
import sys
from pathlib import Path
from typing import Any

# --------------------------------------------------------------------------
# the two pre-registered arms
# --------------------------------------------------------------------------
# "canonical" is the route the whole I20/I11 programme measures (transform 0, so
# the ratio envelope's backend payload is the raw BWT payload).
#
# "closed" is the I20 closed-arm operating point: the same route plus the
# `--ratio-bwt-min-gain-pct 3` routing threshold and the 2 MiB BWT sub-block cap
# that closed the external reference-cost gate.  It is the programme's live arm.
ARM_FLAGS: dict[str, list[str]] = {
    "canonical": [
        "--parse=ratio", "--ratio-backend=auto",
        "--ratio-context=off", "--ratio-lines=off",
        "--bwt-aux=on",
    ],
    "closed": [
        "--parse=ratio", "--ratio-backend=auto",
        "--ratio-context=off", "--ratio-lines=off",
        "--bwt-aux=on",
        "--ratio-bwt-min-gain-pct=3",
        "--bwt-subblock=2097152",
    ],
}

# Frozen paired-timing parameters (shared evidence contract 6).
FROZEN = {
    "reps": 9,
    "warmups": 2,
    "seed": 41246,
    "bootstrap": 20000,
    "epsilon": 0.02,
    "max_cv": 0.15,
    "cpu": 0,
    "ambient_reps": 5,
    "ambient_iterations": 250000,
    "ambient_max_cv": 0.10,
}

# The fixed decision rule (preregistration §5, candidate D2).  Thresholds are
# NOT re-weighted anywhere in this file.
POSTCODER_FUND_PCT = 35.0
POSTCODER_NOGO_PCT = 20.0
STREAM_WINDOW_MATERIAL_PCT = 10.0

SILESIA_DIR = "silesia"


# --------------------------------------------------------------------------
# helpers
# --------------------------------------------------------------------------

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def run(argv: list[str], *, check: bool = True, cwd: Path | None = None) -> subprocess.CompletedProcess:
    p = subprocess.run([str(a) for a in argv], capture_output=True,
                       cwd=str(cwd) if cwd else None)
    if check and p.returncode != 0:
        raise RuntimeError(
            f"command failed rc={p.returncode}: {argv!r}\n"
            + p.stdout.decode(errors="replace")[-4000:]
            + p.stderr.decode(errors="replace")[-4000:])
    return p


def read_uvar(buf: bytes, pos: int) -> tuple[int, int]:
    x = 0
    shift = 0
    for _ in range(10):
        if pos >= len(buf):
            raise ValueError("truncated uvarint")
        b = buf[pos]
        pos += 1
        x |= (b & 0x7F) << shift
        if not (b & 0x80):
            return x, pos
        shift += 7
    raise ValueError("uvarint overflow")


def parse_bwt_bare(p: bytes) -> dict[str, Any]:
    if not p:
        raise ValueError("empty BWT payload")
    if p[0] == 0xFE:  # kBwtAuxTag - auxiliary-index v2 payload
        post = p[1]
        rate, off = read_uvar(p, 2)
        count, off = read_uvar(p, off)
        return {"aux": True, "postcoder_id": post, "aux_rate": rate,
                "aux_count": count}
    post = p[0]
    primary, off = read_uvar(p, 1)
    return {"aux": False, "postcoder_id": post, "primary_index": primary}


def parse_bwt_payload(p: bytes) -> dict[str, Any]:
    if p and p[0] == 0xFF:  # BWT sub-block framing
        nsub, off = read_uvar(p, 1)
        subs = []
        for _ in range(nsub):
            dlen, off = read_uvar(p, off)
            plen, off = read_uvar(p, off)
            subs.append({"decoded_len": dlen, "payload_len": plen,
                          **parse_bwt_bare(p[off:off + plen])})
            off += plen
        return {"framing": "subblock", "subblock_count": nsub, "subblocks": subs}
    return {"framing": "bare", **parse_bwt_bare(p)}


def parse_ratio_payload(p: bytes) -> dict[str, Any]:
    if len(p) < 3:
        raise ValueError("truncated ratio payload")
    transform = p[0]
    backend = p[1]
    xlen, off = read_uvar(p, 2)
    rec: dict[str, Any] = {
        "transform_id": transform,
        "backend_id": backend,
        "transformed_size": xlen,
        "backend_name": {1: "brotli", 2: "bwt"}.get(backend, f"backend{backend}"),
    }
    if backend == 2:
        rec["bwt"] = parse_bwt_payload(p[off:])
    return rec


def parse_container(data: bytes) -> dict[str, Any]:
    if data[:4] != b"ANV0":
        raise ValueError("not an ANVIL container")
    revision = data[4]
    pos = 5
    block_size, pos = read_uvar(data, pos)
    total, pos = read_uvar(data, pos)
    blocks: list[dict[str, Any]] = []
    while pos < len(data):
        blen, pos = read_uvar(data, pos)
        mode = data[pos]
        pos += 1
        plen, pos = read_uvar(data, pos)
        crc = int.from_bytes(data[pos:pos + 4], "little")
        pos += 4
        payload = data[pos:pos + plen]
        pos += plen
        rec: dict[str, Any] = {"decoded_len": blen, "mode": mode,
                               "payload_len": plen, "crc32": crc}
        if mode == 17:
            rec["ratio"] = parse_ratio_payload(payload)
        blocks.append(rec)
    return {"revision": revision, "block_size": block_size,
            "declared_total": total, "blocks": blocks}


def bwt_anatomy(container: dict[str, Any]) -> dict[str, Any]:
    """Flatten the container into the BWT-path facts this candidate needs."""
    posts: list[int] = []
    backends: list[str] = []
    transforms: list[int] = []
    subblocks = 0
    aux_rates: list[int] = []
    for b in container["blocks"]:
        if b.get("mode") != 17:
            raise RuntimeError("non-mode-17 block in a BWT-path payload")
        r = b["ratio"]
        backends.append(r["backend_name"])
        transforms.append(r["transform_id"])
        if r["backend_name"] != "bwt":
            raise RuntimeError(f"file is not on the BWT route: {r['backend_name']}")
        bt = r["bwt"]
        if bt["framing"] == "subblock":
            subblocks += bt["subblock_count"]
            for s in bt["subblocks"]:
                posts.append(s["postcoder_id"])
                if s["aux"]:
                    aux_rates.append(s["aux_rate"])
        else:
            posts.append(bt["postcoder_id"])
            if bt["aux"]:
                aux_rates.append(bt["aux_rate"])
    return {
        "backends": sorted(set(backends)),
        "transforms": sorted(set(transforms)),
        "postcoder_ids": sorted(set(posts)),
        "subblock_count": subblocks,
        "aux_rates": sorted(set(aux_rates)),
        "block_count": len(container["blocks"]),
    }


# --------------------------------------------------------------------------
# prepare
# --------------------------------------------------------------------------

def cmd_prepare(args: argparse.Namespace) -> None:
    root = args.root.resolve()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    anvil = args.anvil.resolve()
    brotli = args.brotli.resolve()
    packed = out / "packed"
    decoded = out / "decoded"
    for d in (packed, decoded):
        d.mkdir(parents=True, exist_ok=True)

    names = [n.strip() for n in args.files.split(",") if n.strip()]
    files: list[dict[str, Any]] = []
    rows: list[dict[str, Any]] = []

    for name in names:
        src = root / SILESIA_DIR / name
        if not src.is_file():
            raise RuntimeError(f"missing corpus file: {src}")
        nbytes = src.stat().st_size
        digest = sha256(src)
        rec: dict[str, Any] = {
            "name": name, "input_bytes": nbytes, "source_sha256": digest,
            "arms": {},
        }

        # ---- the Brotli reference payload (same file, same bytes) -----------
        br = packed / f"{name}.br"
        run([str(brotli), "c", str(src), str(br)])
        br_out = decoded / f"{name}.brout"
        run([str(brotli), "d", str(br), str(br_out), str(nbytes)])
        ok = br_out.is_file() and br_out.stat().st_size == nbytes and sha256(br_out) == digest
        if not ok:
            raise RuntimeError(f"brotli_lw round-trip mismatch: {name}")
        rec["brotli"] = {
            "packed": str(br), "bytes": br.stat().st_size,
            "sha256": sha256(br),
            "cmd": [str(brotli), "d", str(br), str(br_out), str(nbytes)],
            "observe": str(br_out),
            "roundtrip": "OK",
        }
        rows.append({"file": name, "input_bytes": nbytes,
                     "source_sha256": digest, "codec": "brotli_lw",
                     "compressed_bytes": br.stat().st_size,
                     "ratio": f"{br.stat().st_size / nbytes:.9f}"})

        for arm, flags in ARM_FLAGS.items():
            cand = packed / f"{name}.{arm}.anv"
            run([str(anvil), "c", str(src), str(cand), *flags, "--quiet"])
            outp = decoded / f"{name}.{arm}.out"
            cmd = [str(anvil), "d", str(cand), str(outp), "--quiet"]
            run(cmd)
            ok = outp.is_file() and outp.stat().st_size == nbytes and sha256(outp) == digest
            if not ok:
                raise RuntimeError(f"anvil round-trip mismatch: {name}/{arm}")
            container = parse_container(cand.read_bytes())
            anatomy = bwt_anatomy(container)
            if anatomy["postcoder_ids"] not in ([1], [2]):
                raise RuntimeError(
                    f"{name}/{arm}: this probe measures the arithmetic postcoders only, "
                    f"found postcoder ids {anatomy['postcoder_ids']}")
            rec["arms"][arm] = {
                "packed": str(cand), "cmd": cmd, "observe": str(outp),
                "bytes": cand.stat().st_size, "sha256": sha256(cand),
                "flags": flags, "roundtrip": "OK",
                "container": container, "anatomy": anatomy,
            }
            rows.append({"file": name, "input_bytes": nbytes,
                         "source_sha256": digest, "codec": f"anvil-{arm}",
                         "compressed_bytes": cand.stat().st_size,
                         "ratio": f"{cand.stat().st_size / nbytes:.9f}"})
            print(f"prepared {name}/{arm}: {cand.stat().st_size:,} B "
                  f"post={anatomy['postcoder_ids']} subblocks={anatomy['subblock_count']} "
                  f"aux_rate={anatomy['aux_rates']}", flush=True)

        files.append(rec)

    fields = ["file", "input_bytes", "source_sha256", "codec",
              "compressed_bytes", "ratio"]
    with (out / "bytes.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)

    summary = {
        "schema": 1,
        "corpus": args.corpus,
        "source_bytes": sum(r["input_bytes"] for r in files),
        "file_count": len(files),
        "roundtrip": "OK",
        "arm_flags": ARM_FLAGS,
        "files": files,
    }
    cb: dict[str, int] = {}
    for r in rows:
        cb[r["codec"]] = cb.get(r["codec"], 0) + int(r["compressed_bytes"])
    summary["compressed_bytes"] = cb
    (out / "prepare-summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in summary.items() if k != "files"},
                     indent=2, sort_keys=True))


# --------------------------------------------------------------------------
# measure
# --------------------------------------------------------------------------

def paired(out: Path, tag: str, control: list[str], candidate: list[str],
           observe_control: Path | None = None,
           observe_candidate: Path | None = None,
           expected_sha: str | None = None,
           driver: Path | None = None) -> dict[str, Any]:
    """Run one paired/interleaved series with the frozen parameters."""
    driver = driver or Path(__file__).resolve().parent / "paired_bench.py"
    csv_path = out / f"paired-{tag}.csv"
    json_path = out / f"paired-{tag}.json"
    argv = [
        sys.executable, str(driver),
        "--control-json", json.dumps([str(x) for x in control]),
        "--candidate-json", json.dumps([str(x) for x in candidate]),
        "--out", str(csv_path), "--summary", str(json_path),
    ]
    for k, v in FROZEN.items():
        argv += [f"--{k.replace('_', '-')}", str(v)]
    if expected_sha:
        argv += ["--expected-sha256", expected_sha]
    if observe_control is not None:
        argv += ["--control-observe", str(observe_control)]
    if observe_candidate is not None:
        argv += ["--candidate-observe", str(observe_candidate)]
    print(f"paired {tag}", flush=True)
    run(argv)
    return json.loads(json_path.read_text(encoding="utf-8"))


def cmd_measure(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    prep = json.loads(Path(args.prepare).read_text(encoding="utf-8"))
    probe = args.probe.resolve()
    anvil = args.anvil.resolve()
    driver = Path(args.driver).resolve()

    results: dict[str, Any] = {"schema": 1, "corpus": prep["corpus"], "series": {}}

    for frec in prep["files"]:
        name = frec["name"]
        nbytes = frec["input_bytes"]
        digest = frec["source_sha256"]

        # ---- (1)+(2)+(3): the in-process stage decomposition ---------------
        for arm, arec in frec["arms"].items():
            tag = f"{name}-{arm}"
            probe_out = out / f"probe-{tag}.out"
            stages_path = out / f"stages-{tag}.json"
            run([str(probe), "bwt",
                 "--container", arec["packed"],
                 "--reps", str(args.probe_reps),
                 "--warmups", str(args.probe_warmups),
                 "--out-file", str(probe_out),
                 "--out", str(stages_path)])
            # correctness before timing: the probe's own reconstruction must be
            # byte-identical to the source.
            ok = probe_out.is_file() and probe_out.stat().st_size == nbytes \
                and sha256(probe_out) == digest
            if not ok:
                raise RuntimeError(f"probe reconstruction mismatch: {tag}")
            stages = json.loads(stages_path.read_text(encoding="utf-8"))
            stages["roundtrip"] = "OK"
            stages["source_sha256"] = digest
            stages_path.write_text(json.dumps(stages, indent=2, sort_keys=True) + "\n",
                                   encoding="utf-8")

            # ---- (3): end-to-end ANVIL decode, with its A/A null -----------
            e2e = paired(out, f"{tag}-null",
                         control=frec["arms"][arm]["cmd"],
                         candidate=frec["arms"][arm]["cmd"],
                         observe_control=Path(frec["arms"][arm]["observe"]),
                         observe_candidate=Path(frec["arms"][arm]["observe"]),
                         expected_sha=digest, driver=driver)
            results["series"][f"{tag}-e2e-null"] = e2e
            print(f"  e2e {tag}: median={e2e['control']['median_s']:.6f}s "
                  f"null_ci={e2e['paired_ratio_ci95']} status={e2e['status']}", flush=True)

        # ---- (4): the reference brotli_lw one-shot decode, with its null ---
        brnull = paired(out, f"{name}-brotli-null",
                        control=frec["brotli"]["cmd"],
                        candidate=frec["brotli"]["cmd"],
                        observe_control=Path(frec["brotli"]["observe"]),
                        observe_candidate=Path(frec["brotli"]["observe"]),
                        expected_sha=digest, driver=driver)
        results["series"][f"{name}-brotli-null"] = brnull
        print(f"  brotli null {name}: median={brnull['control']['median_s']:.6f}s "
              f"ci={brnull['paired_ratio_ci95']}", flush=True)

        # ---- (4)+(5): one-shot vs the 256 KiB streaming window -------------
        oneshot_stats_path = out / f"brotli-oneshot-{name}.json"
        stream_stats_path = out / f"brotli-stream-{name}.json"
        for variant, path in (("oneshot", oneshot_stats_path), ("stream", stream_stats_path)):
            run([str(probe), "brotli",
                 "--in", frec["brotli"]["packed"],
                 "--expected", str(nbytes),
                 "--mode", variant,
                 "--sink", "discard",
                 "--reps", str(args.probe_reps),
                 "--warmups", str(args.probe_warmups),
                 "--out", str(path)])
        results["series"][f"{name}-brotli-oneshot"] = json.loads(
            oneshot_stats_path.read_text(encoding="utf-8"))
        results["series"][f"{name}-brotli-stream"] = json.loads(
            stream_stats_path.read_text(encoding="utf-8"))

        one_out = out / f"brotli-oneshot-{name}.bin"
        str_out = out / f"brotli-stream-{name}.bin"
        win = paired(out, f"{name}-window",
                     control=[str(probe), "brotli", "--in", frec["brotli"]["packed"],
                              "--expected", str(nbytes), "--mode", "oneshot",
                              "--sink", "collect", "--out-file", str(one_out),
                              "--reps", "1", "--warmups", "0"],
                     candidate=[str(probe), "brotli", "--in", frec["brotli"]["packed"],
                                "--expected", str(nbytes), "--mode", "stream",
                                "--sink", "collect", "--out-file", str(str_out),
                                "--reps", "1", "--warmups", "0"],
                     observe_control=one_out, observe_candidate=str_out,
                     expected_sha=digest, driver=driver)
        results["series"][f"{name}-window"] = win
        print(f"  window {name}: ratio={win['paired_ratio_candidate_over_control']:.6f} "
              f"ci={win['paired_ratio_ci95']}", flush=True)

    (out / "measure.json").write_text(
        json.dumps(results, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    # The shared evidence tool looks for `paired-<tag>.json` with a small fixed
    # tag vocabulary.  Publish the two series that matter under those names so
    # that the manifest carries them verbatim; the full per-combination series
    # stays in measure.json and in the per-combination paired-*.json files.
    primary_tag = None
    for frec in prep["files"]:
        for arm in ("closed", "canonical"):
            if arm in frec["arms"]:
                primary_tag = f"{frec['name']}-{arm}"
                break
        if primary_tag:
            break
    if primary_tag:
        shutil.copyfile(out / f"paired-{primary_tag}-null.json", out / "paired-null.json")
        shutil.copyfile(out / f"paired-{primary_tag}-null.csv", out / "paired-null.csv")
    first = prep["files"][0]["name"]
    shutil.copyfile(out / f"paired-{first}-brotli-null.json", out / "paired-brotli.json")
    shutil.copyfile(out / f"paired-{first}-brotli-null.csv", out / "paired-brotli.csv")

    print(f"wrote {out / 'measure.json'}")


# --------------------------------------------------------------------------
# summarize
# --------------------------------------------------------------------------

def summarize_series(series: dict[str, Any]) -> dict[str, Any]:
    """Aggregate the stage decomposition into the decision-rule inputs."""
    out_rows: list[dict[str, Any]] = []
    for key, val in series.items():
        if not key.endswith("-e2e-null"):
            continue
        tag = key[: -len("-e2e-null")]
        name, arm = tag.rsplit("-", 1)
        stages = val.get("_stages", {})
        e2e_median = val["control"]["median_s"]
        t_post = stages.get("postcoder", {}).get("median_ns", 0) / 1e9
        t_inv = stages.get("inverse_bwt", {}).get("median_ns", 0) / 1e9
        t_probe = stages.get("total", {}).get("median_ns", 0) / 1e9
        stage_sum = t_post + t_inv
        residual = e2e_median - stage_sum
        shares = {
            "vs_stage_sum": (t_post / stage_sum) if stage_sum > 0 else None,
            "vs_probe_total": (t_post / t_probe) if t_probe > 0 else None,
            "vs_end_to_end": (t_post / e2e_median) if e2e_median > 0 else None,
        }
        out_rows.append({
            "file": name, "arm": arm,
            "input_bytes": val.get("input_bytes"),
            "compressed_bytes": val.get("compressed_bytes"),
            "postcoder_seconds": t_post,
            "inverse_bwt_seconds": t_inv,
            "stage_sum_seconds": stage_sum,
            "probe_decode_seconds": t_probe,
            "end_to_end_seconds": e2e_median,
            "residual_seconds": residual,
            "residual_share_of_end_to_end": (residual / e2e_median) if e2e_median > 0 else None,
            "postcoder_share_vs_stage_sum": shares["vs_stage_sum"],
            "postcoder_share_vs_probe_total": shares["vs_probe_total"],
            "postcoder_share_vs_end_to_end": shares["vs_end_to_end"],
            "postcoder_mb_per_s": (val.get("input_bytes", 0) / 1e6 / t_post) if t_post > 0 else None,
            "inverse_bwt_mb_per_s": (val.get("input_bytes", 0) / 1e6 / t_inv) if t_inv > 0 else None,
            "end_to_end_mb_per_s": (val.get("input_bytes", 0) / 1e6 / e2e_median) if e2e_median > 0 else None,
            "probe_null_spans_one": stages.get("null", {}).get("spans_one"),
            "e2e_null_ci95": val.get("paired_ratio_ci95"),
            "e2e_timing_valid": val.get("timing_valid"),
            "e2e_status": val.get("status"),
            "postcoder_share_ci95_vs_end_to_end": stages.get("share_vs_probe_total", {}).get("ci95"),
        })
    return {"rows": out_rows}


def cmd_summarize(args: argparse.Namespace) -> None:
    out = args.out.resolve()
    prep = json.loads(Path(args.prepare).read_text(encoding="utf-8"))
    measure = json.loads(Path(args.measure).read_text(encoding="utf-8"))
    series = measure["series"]

    # Attach the stage-probe output to each end-to-end series so the aggregator
    # can compute the decomposition.
    for frec in prep["files"]:
        for arm in frec["arms"]:
            tag = f"{frec['name']}-{arm}"
            sp = out / f"stages-{tag}.json"
            if not sp.is_file():
                raise RuntimeError(f"missing stage probe output: {sp}")
            stages = json.loads(sp.read_text(encoding="utf-8"))
            e2e = series[f"{tag}-e2e-null"]
            e2e["_stages"] = stages
            e2e["input_bytes"] = frec["input_bytes"]
            e2e["compressed_bytes"] = frec["arms"][arm]["bytes"]

    agg = summarize_series(series)
    rows = agg["rows"]
    if not rows:
        raise RuntimeError("no measurement rows")

    # ---- the streaming-window cost -----------------------------------------
    window_rows = []
    for frec in prep["files"]:
        name = frec["name"]
        one = series[f"{name}-brotli-oneshot"]
        stream = series[f"{name}-brotli-stream"]
        win = series[f"{name}-window"]
        inproc = (stream["median_s"] / one["median_s"]) if one["median_s"] > 0 else None
        proc = win["paired_ratio_candidate_over_control"]
        window_rows.append({
            "file": name,
            "reference_one_shot_median_s": one["median_s"],
            "streaming_256k_median_s": stream["median_s"],
            "window_cost_pct_in_process": (inproc - 1.0) * 100.0 if inproc is not None else None,
            "window_cost_pct_process_level": (proc - 1.0) * 100.0,
            "window_ratio_process_level": proc,
            "window_ratio_ci95_process_level": win["paired_ratio_ci95"],
            "window_timing_valid": win["timing_valid"],
            "reference_brotli_lw_median_s": series[f"{name}-brotli-null"]["control"]["median_s"],
            "reference_brotli_lw_null_ci95": series[f"{name}-brotli-null"]["paired_ratio_ci95"],
        })

    # ---- the fixed decision rule -------------------------------------------
    # Aggregation convention (declared here, before the numbers are read; it
    # does NOT re-weight the 35 % / 20 % thresholds):
    #   D4 is FUNDED  iff the postcoder share is >= 35 % under EVERY denominator;
    #   D4 is NO-GO  iff the postcoder share is <  20 % under EVERY denominator;
    #   otherwise the arms straddle the decision band and the verdict is
    #   INCONCLUSIVE, with the measured range reported.
    denom_keys = ("postcoder_share_vs_stage_sum",
                  "postcoder_share_vs_probe_total",
                  "postcoder_share_vs_end_to_end")
    per_row = []
    for r in rows:
        vals = [r[k] for k in denom_keys if r[k] is not None]
        if not vals:
            continue
        lo, hi = min(vals), max(vals)
        funded = all(v * 100.0 >= POSTCODER_FUND_PCT for v in vals)
        nogo = all(v * 100.0 < POSTCODER_NOGO_PCT for v in vals)
        verdict = "FUNDED" if funded else ("NO_GO" if nogo else "INCONCLUSIVE")
        per_row.append({**r,
                        "postcoder_share_min": lo,
                        "postcoder_share_max": hi,
                        "postcoder_share_pct_min": lo * 100.0,
                        "postcoder_share_pct_max": hi * 100.0,
                        "decision_rule_verdict": verdict})

    shares_all = [v for r in per_row for v in
                  (r["postcoder_share_min"], r["postcoder_share_max"])]
    programme_funded = bool(per_row) and all(
        r["decision_rule_verdict"] == "FUNDED" for r in per_row)
    programme_nogo = bool(per_row) and all(
        r["decision_rule_verdict"] == "NO_GO" for r in per_row)
    programme_verdict = ("D4_FUNDED" if programme_funded else
                         "D4_NO_GO" if programme_nogo else "D4_INCONCLUSIVE")

    window_costs = [w["window_cost_pct_in_process"] for w in window_rows
                    if w["window_cost_pct_in_process"] is not None]
    window_material = bool(window_costs) and all(
        c >= STREAM_WINDOW_MATERIAL_PCT for c in window_costs)

    headline = None
    if per_row:
        # the headline share is the primary definition (against the shipped
        # end-to-end decode) of the arm/file with the LOWEST share: the
        # conservative reading of the funding bar.
        worst = min(per_row, key=lambda r: r["postcoder_share_vs_end_to_end"]
                    if r["postcoder_share_vs_end_to_end"] is not None else 1.0)
        headline = worst["postcoder_share_vs_end_to_end"]

    payload = {
        "schema": 1,
        "corpus": prep["corpus"],
        "decision_rule_verbatim": (
            "If the postcoder is >= 35 % of BWT-path decode, D4 (the entropy-coder "
            "swap) is FUNDED and must be executed. If the postcoder is < 20 %, D4 is "
            "NO-GO on Amdahl grounds and the programme's speed effort pivots "
            "entirely to D3. If the 256 KiB streaming window costs >= 10 % against "
            "the one-shot reference decode, enlarging it is a mandatory part of any "
            "later arm."),
        "thresholds": {
            "postcoder_fund_pct": POSTCODER_FUND_PCT,
            "postcoder_nogo_pct": POSTCODER_NOGO_PCT,
            "streaming_window_material_pct": STREAM_WINDOW_MATERIAL_PCT,
        },
        "postcoder_share": headline,
        "postcoder_share_pct": (headline * 100.0) if headline is not None else None,
        "postcoder_share_range_pct": [
            (min(shares_all) * 100.0) if shares_all else None,
            (max(shares_all) * 100.0) if shares_all else None,
        ],
        "per_measurement": per_row,
        "streaming_window": window_rows,
        "streaming_window_material": window_material,
        "verdict": programme_verdict,
        "d4_funded": programme_funded,
        "aggregation_convention": (
            "FUNDED iff the postcoder share is >= 35 % under every denominator "
            "(stage sum, probe decode total, shipped end-to-end); NO-GO iff it is "
            "< 20 % under every denominator; otherwise INCONCLUSIVE. The fixed "
            "thresholds are not re-weighted."),
    }
    (out / "postcoder-share.json").write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    # A manifest-compatible ruling so that the shared evidence tool carries the
    # number verbatim in `ruling_gates`.
    gates = {
        "postcoder_share": headline,
        "postcoder_share_pct": (headline * 100.0) if headline is not None else None,
        "postcoder_share_min_pct": (min(shares_all) * 100.0) if shares_all else None,
        "postcoder_share_max_pct": (max(shares_all) * 100.0) if shares_all else None,
        "postcoder_share_threshold_fund_pct": POSTCODER_FUND_PCT,
        "postcoder_share_threshold_nogo_pct": POSTCODER_NOGO_PCT,
        "d4_funded": programme_funded,
        "d4_no_go": programme_nogo,
        "d4_verdict": programme_verdict,
        "streaming_window_cost_pct": (
            window_costs[0] if len(window_costs) == 1 else
            (max(window_costs) if window_costs else None)),
        "streaming_window_cost_pct_max": (max(window_costs) if window_costs else None),
        "streaming_window_threshold_pct": STREAM_WINDOW_MATERIAL_PCT,
        "streaming_window_material": window_material,
        "enlarging_the_256k_window_is_mandatory": window_material,
    }
    ruling = {
        "schema": 1,
        "ruling": programme_verdict,
        "gates": gates,
        "compressed_bytes": prep.get("compressed_bytes"),
        "decode": {
            "postcoder_share": headline,
            "postcoder_share_pct": (headline * 100.0) if headline is not None else None,
            "per_measurement": [
                {"file": r["file"], "arm": r["arm"],
                 "postcoder_seconds": r["postcoder_seconds"],
                 "inverse_bwt_seconds": r["inverse_bwt_seconds"],
                 "end_to_end_seconds": r["end_to_end_seconds"],
                 "postcoder_share_vs_end_to_end": r["postcoder_share_vs_end_to_end"],
                 "postcoder_share_vs_stage_sum": r["postcoder_share_vs_stage_sum"],
                 "postcoder_share_vs_probe_total": r["postcoder_share_vs_probe_total"]}
                for r in per_row],
        },
        "memory_ratio": None,
        "note": ("This is a D2 diagnostic run, not an arbiter ruling: the frozen "
                 "reference-cost gate is not evaluated here, so every gate check in "
                 "the shared manifest is reported as null (not evaluated) and is "
                 "never counted as a pass."),
    }
    (out / "ruling.json").write_text(
        json.dumps(ruling, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(json.dumps({
        "verdict": programme_verdict,
        "postcoder_share_pct": payload["postcoder_share_pct"],
        "postcoder_share_range_pct": payload["postcoder_share_range_pct"],
        "streaming_window_material": window_material,
        "streaming_window_cost_pct": window_costs,
    }, indent=2, sort_keys=True))


# --------------------------------------------------------------------------

def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("prepare")
    p.add_argument("--corpus", default="silesia")
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--brotli", type=Path, required=True)
    p.add_argument("--files", default="mr,x-ray")
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_prepare)

    p = sub.add_parser("measure")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--anvil", type=Path, required=True)
    p.add_argument("--probe", type=Path, required=True)
    p.add_argument("--driver", type=Path, required=True)
    p.add_argument("--probe-reps", type=int, default=9)
    p.add_argument("--probe-warmups", type=int, default=2)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_measure)

    p = sub.add_parser("summarize")
    p.add_argument("--prepare", type=Path, required=True)
    p.add_argument("--measure", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.set_defaults(func=cmd_summarize)

    args = ap.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
