#!/usr/bin/env python3
"""A48 measurement driver -- per-file paired grid for the frozen Pareto classifiers.

This is a NEW measurement tool (not one of the frozen arbiters). It produces a
per-file benchmark-suite-shaped CSV whose timing is measured on ONE ubuntu-24.04
VM with the canonical tools/paired_bench.py driver (symmetric warmup, AB/BA
seeded interleave, every raw repetition retained, bootstrap paired CI, A/A null).

The frozen arbiters (tools/pareto_front.py, tools/beats_brotli.py,
docs/gate-verify-i9.py) are then run UNMODIFIED on the emitted grid.

Arms per file:
  anvil-cap8 -- anvil c --parse=ratio --ratio-backend=auto --ratio-context=off
               --ratio-lines=off --bwt-aux=on --bwt-subblock=8388608 --quiet
  xz-9e      -- xz -9e -T1 -c / -dc
  brotli-q11-lw30 -- brotli_lw c / d

Reference class is deliberately THIN (xz-9e + brotli-q11/lw30) exactly as the
A48 prereg specifies; the frozen tools' GRID-THIN caveat applies to the ruling.
"""
from __future__ import annotations
import argparse, csv, hashlib, json, os, subprocess, sys
from pathlib import Path

FIELDS = ["file", "input_bytes", "codec", "compressed_bytes", "ratio",
          "encode_MBps", "decode_MBps", "roundtrip"]


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def run(argv: list[str]) -> None:
    p = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        raise RuntimeError(f"cmd failed rc={p.returncode}: {argv}\n"
                           + p.stderr.decode("utf-8", "replace")[-2000:])


def paired(tools: Path, control: list[str], candidate: list[str], reps: int,
           label: str, out: Path, seed: int = 41246) -> dict:
    argv = [sys.executable, str(tools / "paired_bench.py"),
            "--control-json", json.dumps(control),
            "--candidate-json", json.dumps(candidate),
            "--reps", str(reps), "--warmups", "2", "--seed", str(seed),
            "--bootstrap", "20000", "--epsilon", "0.02", "--max-cv", "0.15",
            "--cpu", "0",
            "--ambient-reps", "5", "--ambient-iterations", "250000",
            "--ambient-max-cv", "0.10",
            "--out", str(out / f"paired-{label}.csv"),
            "--summary", str(out / f"paired-{label}.json")]
    p = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if p.returncode != 0:
        raise RuntimeError(f"paired_bench failed for {label}:\n{p.stdout[-2000:]}")
    return json.loads((out / f"paired-{label}.json").read_text())


def mbs(nbytes: int, median_s: float) -> float:
    return (nbytes / 1e6 / median_s) if median_s and median_s > 0 else 0.0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--files", nargs="+", required=True)
    ap.add_argument("--anvil", required=True)
    ap.add_argument("--brotli", required=True)
    ap.add_argument("--tools", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--decode-reps", type=int, default=9)
    ap.add_argument("--encode-reps", type=int, default=7)
    a = ap.parse_args()

    tools = Path(a.tools)
    out = Path(a.out); out.mkdir(parents=True, exist_ok=True)
    anvil = str(Path(a.anvil).resolve())
    brotli = str(Path(a.brotli).resolve())
    xz = "xz"

    rows: list[dict] = []
    validity: list[dict] = []
    robustness: list[dict] = []

    def ratio_scan(src: Path, name: str, n: int) -> dict:
        """Deterministic ratio reference scan across the FULL frozen class.

        A FRONT-CROSSING (R-2) needs a gap bracket q_lo with ratio <= p.ratio and
        mbps <= p.mbps. If NO reference tier in the full class has ratio <=
        p.ratio, no q_lo can exist at ANY throughput -> the crossing is robust to
        grid thickness. Ratios need only bytes (deterministic), not timing.
        """
        ratios: dict[str, float] = {}
        # brotli CLI tiers (q1/q3/q5/q9/q11, default lgwin) if available
        if subprocess.run(["which", "brotli"], stdout=subprocess.DEVNULL,
                          stderr=subprocess.DEVNULL).returncode == 0:
            for q in (1, 3, 5, 9, 11):
                p = out / "work" / name / f"ref-brotli-q{q}.br"
                r = subprocess.run(["brotli", "-q", str(q), "-c", str(src)],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                if r.returncode == 0:
                    p.write_bytes(r.stdout)
                    ratios[f"brotli-q{q}"] = len(r.stdout) / n
        # zstd CLI tiers if available
        if subprocess.run(["which", "zstd"], stdout=subprocess.DEVNULL,
                          stderr=subprocess.DEVNULL).returncode == 0:
            for lvl in (1, 3, 9, 19):
                r = subprocess.run(["zstd", f"-{lvl}", "-q", "-c", str(src)],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                if r.returncode == 0:
                    ratios[f"zstd-{lvl}"] = len(r.stdout) / n
        ratios["xz-9e"] = cb_xz / n
        ratios["brotli-q11-lw30"] = cb_br / n
        return ratios

    # placeholders assigned per-file below
    cb_xz = cb_br = 0
    for f in a.files:
        src = Path(f).resolve()
        name = src.name
        n = src.stat().st_size
        src_sha = sha256(src)
        work = out / "work" / name; work.mkdir(parents=True, exist_ok=True)

        # --- encode once per arm; bytes are the citable deterministic quantity ---
        a_pack = work / "anvil.cap8"
        run([anvil, "c", str(src), str(a_pack), "--parse=ratio",
             "--ratio-backend=auto", "--ratio-context=off", "--ratio-lines=off",
             "--bwt-aux=on", "--bwt-subblock=8388608", "--quiet"])
        x_pack = work / "xz-9e.xz"
        with x_pack.open("wb") as fh:
            subprocess.run([xz, "-9e", "-T1", "-c", str(src)], stdout=fh,
                           stderr=subprocess.PIPE, check=True)
        b_pack = work / "brotli-q11.br"
        run([brotli, "c", str(src), str(b_pack)])

        cb = {"anvil": a_pack.stat().st_size,
              "xz": x_pack.stat().st_size,
              "brotli": b_pack.stat().st_size}
        cb_xz, cb_br = cb["xz"], cb["brotli"]
        full_ratios = ratio_scan(src, name, n)
        anvil_ratio = cb["anvil"] / n
        lower = {k: v for k, v in full_ratios.items() if v <= anvil_ratio + 1e-12}
        robustness.append({"file": name, "anvil_ratio": anvil_ratio,
                           "full_reference_ratios": full_ratios,
                           "refs_with_ratio_le_anvil": lower,
                           "bracket_anchor_impossible": len(lower) == 0})

        # --- roundtrip (deterministic hash) ---
        a_out = work / "anvil.out"; run([anvil, "d", str(a_pack), str(a_out), "--quiet"])
        x_out = work / "xz.out"
        with x_out.open("wb") as fh:
            subprocess.run([xz, "-dc", str(x_pack)], stdout=fh,
                           stderr=subprocess.PIPE, check=True)
        b_out = work / "brotli.out"
        run([brotli, "d", str(b_pack), str(b_out), str(n)])
        rt = all(sha256(p) == src_sha for p in (a_out, x_out, b_out))
        if not rt:
            raise RuntimeError(f"{name}: one or more encoded streams failed bit-exact roundtrip")

        # --- timing arms (argv JSON) ---
        A_ENC = [anvil, "c", str(src), str(work / "enc.anvil"), "--parse=ratio",
                 "--ratio-backend=auto", "--ratio-context=off", "--ratio-lines=off",
                 "--bwt-aux=on", "--bwt-subblock=8388608", "--quiet"]
        X_ENC = [xz, "-9e", "-T1", "-c", str(src)]
        B_ENC = [brotli, "c", str(src), str(work / "enc.br")]
        A_DEC = [anvil, "d", str(a_pack), str(work / "dec.anvil"), "--quiet"]
        X_DEC = [xz, "-dc", str(x_pack)]
        B_DEC = [brotli, "d", str(b_pack), str(work / "dec.br"), str(n)]

        # decode plane: ANVIL vs each ref + A/A null
        d_ax = paired(tools, X_DEC, A_DEC, a.decode_reps, f"{name}-dec-xz", out)
        d_ab = paired(tools, B_DEC, A_DEC, a.decode_reps, f"{name}-dec-brotli", out)
        d_nn = paired(tools, A_DEC, A_DEC, a.decode_reps, f"{name}-dec-null", out)
        # encode plane: ANVIL vs each ref + A/A null
        e_ax = paired(tools, X_ENC, A_ENC, a.encode_reps, f"{name}-enc-xz", out)
        e_ab = paired(tools, B_ENC, A_ENC, a.encode_reps, f"{name}-enc-brotli", out)
        e_nn = paired(tools, A_ENC, A_ENC, a.encode_reps, f"{name}-enc-null", out)

        def span1(s: dict) -> bool:
            lo, hi = s["paired_ratio_ci95"]
            return lo <= 1.0 <= hi

        valid = all(s["timing_valid"] for s in (d_ax, d_ab, d_nn, e_ax, e_ab, e_nn)) \
            and span1(d_nn) and span1(e_nn)
        validity.append({"file": name, "timing_valid_all": valid,
                         "decode_null_ci": d_nn["paired_ratio_ci95"],
                         "encode_null_ci": e_nn["paired_ratio_ci95"],
                         "decode_null_valid": d_nn["timing_valid"] and span1(d_nn),
                         "encode_null_valid": e_nn["timing_valid"] and span1(e_nn)})

        anvil_dec = mbs(n, (d_ax["candidate"]["median_s"] + d_ab["candidate"]["median_s"]) / 2)
        xz_dec = mbs(n, d_ax["control"]["median_s"])
        br_dec = mbs(n, d_ab["control"]["median_s"])
        anvil_enc = mbs(n, (e_ax["candidate"]["median_s"] + e_ab["candidate"]["median_s"]) / 2)
        xz_enc = mbs(n, e_ax["control"]["median_s"])
        br_enc = mbs(n, e_ab["control"]["median_s"])

        def row(codec: str, cbv: int, enc: float, dec: float) -> dict:
            return {"file": name, "input_bytes": n, "codec": codec,
                    "compressed_bytes": cbv, "ratio": cbv / n,
                    "encode_MBps": f"{enc:.3f}", "decode_MBps": f"{dec:.3f}",
                    "roundtrip": "OK" if rt else "FAIL"}
        rows += [row("anvil-cap8", cb["anvil"], anvil_enc, anvil_dec),
                 row("xz-9e", cb["xz"], xz_enc, xz_dec),
                 row("brotli-q11-lw30", cb["brotli"], br_enc, br_dec)]
        print(f"{name}: bytes anvil/xz/brotli={cb['anvil']}/{cb['xz']}/{cb['brotli']} "
              f"valid={valid} rt={rt}", flush=True)

        # Durable per-file checkpoint: the original 12-file experiment can exceed
        # the hosted job's runtime limit. This is *diagnostic evidence only*;
        # a partial grid is NEVER eligible for an H1/H0 conclusion.
        # Atomic replacement avoids exposing a half-written CSV/JSON snapshot.
        def checkpoint_text(filename: str, contents: str) -> None:
            dest = out / filename
            temporary = out / (filename + ".tmp")
            temporary.write_text(contents, encoding="utf-8")
            os.replace(temporary, dest)

        import io
        buffer = io.StringIO()
        writer = csv.DictWriter(buffer, fieldnames=FIELDS)
        writer.writeheader()
        writer.writerows(rows)
        checkpoint_text("grid-partial.csv", buffer.getvalue())
        checkpoint_text("timing-validity-partial.json", json.dumps(validity, indent=2) + "\n")
        checkpoint_text("bracket-robustness-partial.json", json.dumps(robustness, indent=2) + "\n")
        checkpoint_text("progress.json", json.dumps({
            "completed_files": [entry["file"] for entry in validity],
            "completed_count": len(validity),
            "total_count": len(a.files),
            "eligible_for_frontier_claim": False,
        }, indent=2) + "\n")

    grid = out / "grid.csv"
    with grid.open("w", newline="") as fp:
        w = csv.DictWriter(fp, fieldnames=FIELDS); w.writeheader(); w.writerows(rows)
    (out / "timing-validity.json").write_text(json.dumps(validity, indent=2) + "\n")
    (out / "bracket-robustness.json").write_text(json.dumps(robustness, indent=2) + "\n")
    print(f"wrote {grid} ({len(rows)} rows); all_valid="
          f"{all(v['timing_valid_all'] for v in validity)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
