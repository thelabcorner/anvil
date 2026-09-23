#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

import psutil


FIELDS = ["corpus", "file", "input_bytes", "codec", "compressed_bytes", "ratio",
          "compress_s", "decompress_s", "compress_peak_mib", "decompress_peak_mib", "roundtrip",
          "timing_status", "legacy_compress_s", "legacy_decompress_s",
          "legacy_compress_peak_mib", "legacy_decompress_peak_mib"]

TIMING_VALID = "VALID_100MS_NONRECURSIVE"
TIMING_LEGACY_INVALID = "INVALID_5MS_RECURSIVE_POLL"

SILESIA = {
    "dickens": (10192446, "88334708559f6db57d79096bc0aca07e"),
    "mozilla": (51220480, "c7789a2097f1ff944b0c737430a339b3"),
    "mr": (9970564, "38e623e3093b7bf2003ca4b1bbc19927"),
    "nci": (33553445, "31f85bc8706f3c921104e7c169e2e2e1"),
    "ooffice": (6152192, "573c4ae915e36631d8f2dcffb9b9b66d"),
    "osdb": (10085684, "e734b0c48e6a982adfb5802da3032ecd"),
    "reymont": (6627202, "d8f54d78105079775f32d76dc55fc671"),
    "samba": (21606400, "154eaea7ea70e89f6339ff0abf4112ca"),
    "sao": (7251944, "79e95a22e18cd82b7e42bf91b380d30b"),
    "webster": (41458703, "474931ad907ac27bf962c75ded46c069"),
    "x-ray": (8474240, "9baec32ad14ec3eff487d254382cb91c"),
    "xml": (5345280, "9b09c0c80104adb8aae910b7d7db003e"),
}
ENWIK8 = {"enwik8": (100000000, "a1fa5ffddb56f4953e226637dabbb36a")}


def digest(path: Path, algorithm: str) -> str:
    h = hashlib.new(algorithm)
    with path.open("rb") as f:
        while b := f.read(1 << 20):
            h.update(b)
    return h.hexdigest()


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            b = f.read(1 << 20)
            if not b:
                break
            h.update(b)
    return h.hexdigest()


def verify_manifest(corpus: str, files: list[Path]) -> None:
    manifest = SILESIA if corpus == "silesia" else ENWIK8 if corpus == "enwik8" else None
    if manifest is None:
        return
    got = {p.name: p for p in files}
    if set(got) != set(manifest):
        missing = sorted(set(manifest) - set(got))
        extra = sorted(set(got) - set(manifest))
        raise RuntimeError(f"{corpus} manifest mismatch: missing={missing} extra={extra}")
    total = 0
    for name, (size, md5) in manifest.items():
        p = got[name]
        actual_size = p.stat().st_size
        actual_md5 = digest(p, "md5")
        if actual_size != size or actual_md5.lower() != md5:
            raise RuntimeError(f"{corpus}/{name}: canonical manifest mismatch: size {actual_size}/{size}, md5 {actual_md5}/{md5}")
        total += actual_size
    if corpus == "silesia" and total != 211_938_580:
        raise RuntimeError(f"Silesia total mismatch: {total} != 211938580")
    print(f"manifest OK: {corpus}, files={len(files)}, total={total:,} B", flush=True)


def run_measured(cmd: list[str], *, stdout_path: Path | None = None) -> tuple[float, float]:
    """Return (wall_seconds, peak_RSS_MiB) with low-overhead sampling.

    Benchmark codec commands are direct processes, so recursively walking child
    processes every few milliseconds only contaminates wall-clock.  Sample the
    benchmark process itself at 100 ms.  On Windows psutil exposes PeakWorkingSet
    through peak_wset; use it when available so short-lived peaks between polls
    are retained by the OS accounting.
    """
    out = stdout_path.open("wb") if stdout_path else subprocess.DEVNULL
    t0 = time.perf_counter()
    p = subprocess.Popen(cmd, stdout=out, stderr=subprocess.PIPE)
    proc = psutil.Process(p.pid)
    peak = 0
    try:
        while p.poll() is None:
            try:
                mi = proc.memory_info()
                rss = getattr(mi, "peak_wset", mi.rss)
                peak = max(peak, rss)
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                pass
            time.sleep(0.100)
        stderr = p.stderr.read() if p.stderr else b""
        if p.returncode:
            raise RuntimeError(f"command failed ({p.returncode}): {' '.join(cmd)}\n{stderr.decode(errors='replace')}")
    finally:
        if stdout_path:
            out.close()
    return time.perf_counter() - t0, peak / (1024 * 1024)


def codec_specs(src: Path, n: int, anvil: Path, brotli_lw: Path):
    zstd = shutil.which("zstd")
    xz = shutil.which("xz")
    if not zstd or not xz:
        raise RuntimeError("zstd and xz must be available on PATH")

    return [
            (
                "anvil-ratio",
                lambda packed: [str(anvil), "c", str(src), str(packed), "--parse=ratio", "--quiet"],
                lambda packed, dec: [str(anvil), "d", str(packed), str(dec), "--quiet"],
                False,
                False,
            ),
            (
                "anvil-bwt-direct",
                lambda packed: [str(anvil), "c", str(src), str(packed), "--parse=ratio", "--ratio-backend=bwt",
                                "--ratio-context=off", "--ratio-lines=off", "--quiet"],
                lambda packed, dec: [str(anvil), "d", str(packed), str(dec), "--quiet"],
                False,
                False,
            ),
            (
                "anvil-bwt-direct-aux",
                lambda packed: [str(anvil), "c", str(src), str(packed), "--parse=ratio", "--ratio-backend=bwt",
                                "--ratio-context=off", "--ratio-lines=off", "--bwt-aux=on", "--quiet"],
                lambda packed, dec: [str(anvil), "d", str(packed), str(dec), "--quiet"],
                False,
                False,
            ),
            (
                "anvil-bwt",
                lambda packed: [str(anvil), "c", str(src), str(packed), "--parse=ratio", "--ratio-backend=bwt", "--quiet"],
                lambda packed, dec: [str(anvil), "d", str(packed), str(dec), "--quiet"],
                False,
                False,
            ),
            (
                "anvil-ratio-auto",
                lambda packed: [str(anvil), "c", str(src), str(packed), "--parse=ratio", "--ratio-backend=auto", "--quiet"],
                lambda packed, dec: [str(anvil), "d", str(packed), str(dec), "--quiet"],
                False,
                False,
            ),
            (
                "anvil-ratio-auto-aux",
                lambda packed: [str(anvil), "c", str(src), str(packed), "--parse=ratio", "--ratio-backend=auto",
                                "--bwt-aux=on", "--quiet"],
                lambda packed, dec: [str(anvil), "d", str(packed), str(dec), "--quiet"],
                False,
                False,
            ),
            (
                "anvil-auto-direct",
                # Whole-file --ratio-backend=auto with transforms DISABLED, so the
                # result is directly comparable to the naked-Brotli / direct-BWT oracle.
                # This is the E2 / E7 measurement cell.
                lambda packed: [str(anvil), "c", str(src), str(packed), "--parse=ratio", "--ratio-backend=auto",
                                "--ratio-context=off", "--ratio-lines=off", "--quiet"],
                lambda packed, dec: [str(anvil), "d", str(packed), str(dec), "--quiet"],
                False,
                False,
            ),
            (
                "anvil-auto-direct-aux",
                # Same auto backend oracle, but every BWT candidate carries the
                # fully-charged I10 auxiliary-index representation.
                lambda packed: [str(anvil), "c", str(src), str(packed), "--parse=ratio", "--ratio-backend=auto",
                                "--ratio-context=off", "--ratio-lines=off", "--bwt-aux=on", "--quiet"],
                lambda packed, dec: [str(anvil), "d", str(packed), str(dec), "--quiet"],
                False,
                False,
            ),
            (
                "brotli-q11-lw30",
                lambda packed: [str(brotli_lw), "c", str(src), str(packed)],
                lambda packed, dec: [str(brotli_lw), "d", str(packed), str(dec), str(n)],
                False,
                False,
            ),
            (
                "zstd-ultra-22-long27",
                lambda packed: [zstd, "-q", "-f", "--ultra", "-22", "--long=27", str(src), "-o", str(packed)],
                lambda packed, dec: [zstd, "-q", "-f", "-d", "--long=27", str(packed), "-o", str(dec)],
                False,
                False,
            ),
            (
                "xz-9e",
                lambda packed: [xz, "-9e", "-c", str(src)],
                lambda packed, dec: [xz, "-d", "-c", str(packed)],
                True,
                True,
            ),
        ]


def bench_cell(corpus: str, src: Path, codec: str, anvil: Path, brotli_lw: Path) -> dict[str, object]:
    n = src.stat().st_size
    src_hash = sha256(src)
    specs = {s[0]: s[1:] for s in codec_specs(src, n, anvil, brotli_lw)}
    if codec not in specs:
        raise RuntimeError(f"unknown codec {codec}; choices={sorted(specs)}")
    ccmd, dcmd, cstdout, dstdout = specs[codec]
    with tempfile.TemporaryDirectory(prefix=f"anvil-ratio-{src.name}-{codec}-") as td0:
        td = Path(td0)
        packed = td / (codec + ".bin")
        dec = td / (codec + ".out")
        ct, cm = run_measured(ccmd(packed), stdout_path=packed if cstdout else None)
        dt, dm = run_measured(dcmd(packed, dec), stdout_path=dec if dstdout else None)
        ok = dec.stat().st_size == n and sha256(dec) == src_hash
        cb = packed.stat().st_size
        row = {
            "corpus": corpus,
            "file": src.name,
            "input_bytes": n,
            "codec": codec,
            "compressed_bytes": cb,
            "ratio": f"{cb / n:.9f}" if n else "0",
            "compress_s": f"{ct:.6f}",
            "decompress_s": f"{dt:.6f}",
            "compress_peak_mib": f"{cm:.3f}",
            "decompress_peak_mib": f"{dm:.3f}",
            "roundtrip": "OK" if ok else "FAIL",
            "timing_status": TIMING_VALID,
            "legacy_compress_s": "",
            "legacy_decompress_s": "",
            "legacy_compress_peak_mib": "",
            "legacy_decompress_peak_mib": "",
        }
        print(f"{corpus}/{src.name}: {codec:22s} {cb:12,d} B  r={cb/n:.6f}  c={ct:.3f}s d={dt:.3f}s  peak={cm:.1f}/{dm:.1f} MiB  {'OK' if ok else 'FAIL'}", flush=True)
        if not ok:
            raise RuntimeError(f"roundtrip mismatch: {codec} {src}")
        return row


def load_completed(out: Path, *, require_valid_timing: bool = False) -> set[tuple[str, str, str]]:
    if not out.exists():
        return set()
    with out.open(newline="") as fp:
        return {(r["corpus"], r["file"], r["codec"]) for r in csv.DictReader(fp)
                if r.get("roundtrip") == "OK" and
                (not require_valid_timing or r.get("timing_status") == TIMING_VALID)}


def migrate_legacy_csv(out: Path) -> None:
    """Preserve pre-fix timing values, explicitly marking them invalid.

    Existing byte counts and round-trip results remain authoritative.  A later
    --refresh-timings run replaces only the active timing columns while keeping
    the contaminated measurements in legacy_* for auditability.
    """
    if not out.exists() or out.stat().st_size == 0:
        return
    with out.open(newline="") as fp:
        reader = csv.DictReader(fp)
        rows = list(reader)
        old_fields = reader.fieldnames or []
    if all(k in old_fields for k in FIELDS):
        return
    for r in rows:
        r["timing_status"] = TIMING_LEGACY_INVALID
        r["legacy_compress_s"] = r.get("compress_s", "")
        r["legacy_decompress_s"] = r.get("decompress_s", "")
        r["legacy_compress_peak_mib"] = r.get("compress_peak_mib", "")
        r["legacy_decompress_peak_mib"] = r.get("decompress_peak_mib", "")
    tmp = out.with_suffix(out.suffix + ".tmp")
    with tmp.open("w", newline="") as fp:
        w = csv.DictWriter(fp, fieldnames=FIELDS)
        w.writeheader()
        w.writerows(rows)
        fp.flush()
        os.fsync(fp.fileno())
    os.replace(tmp, out)


def replace_timing_row(out: Path, fresh: dict[str, object]) -> None:
    key = (str(fresh["corpus"]), str(fresh["file"]), str(fresh["codec"]))
    with out.open(newline="") as fp:
        rows = list(csv.DictReader(fp))
    found = False
    for r in rows:
        if (r.get("corpus"), r.get("file"), r.get("codec")) != key:
            continue
        if (r.get("compressed_bytes") != str(fresh["compressed_bytes"]) or
                r.get("roundtrip") != "OK" or fresh.get("roundtrip") != "OK"):
            raise RuntimeError(f"timing refresh changed authoritative result for {key}")
        if not r.get("legacy_compress_s"):
            r["legacy_compress_s"] = r.get("compress_s", "")
            r["legacy_decompress_s"] = r.get("decompress_s", "")
            r["legacy_compress_peak_mib"] = r.get("compress_peak_mib", "")
            r["legacy_decompress_peak_mib"] = r.get("decompress_peak_mib", "")
        for k in ("compress_s", "decompress_s", "compress_peak_mib", "decompress_peak_mib", "timing_status"):
            r[k] = str(fresh[k])
        found = True
        break
    if not found:
        raise RuntimeError(f"cannot refresh missing benchmark cell {key}")
    tmp = out.with_suffix(out.suffix + ".tmp")
    with tmp.open("w", newline="") as fp:
        w = csv.DictWriter(fp, fieldnames=FIELDS)
        w.writeheader(); w.writerows(rows); fp.flush(); os.fsync(fp.fileno())
    os.replace(tmp, out)


def append_row(out: Path, row: dict[str, object]) -> None:
    out.parent.mkdir(parents=True, exist_ok=True)
    new = not out.exists() or out.stat().st_size == 0
    with out.open("a", newline="") as fp:
        w = csv.DictWriter(fp, fieldnames=FIELDS)
        if new:
            w.writeheader()
        w.writerow(row)
        fp.flush()
        os.fsync(fp.fileno())
def main() -> None:
    ap = argparse.ArgumentParser(description="Ratio-first ANVIL benchmark: exact bytes + wall time + peak RSS + roundtrip")
    ap.add_argument("files", nargs="+", type=Path)
    ap.add_argument("--corpus", required=True)
    ap.add_argument("--anvil", type=Path, default=Path("build/anvil.exe"))
    ap.add_argument("--brotli", type=Path, default=Path("build/brotli_lw.exe"))
    ap.add_argument("--out", type=Path, default=Path("tests/ratio-first-benchmark.csv"))
    ap.add_argument("--codecs", default="anvil-ratio,brotli-q11-lw30,zstd-ultra-22-long27,xz-9e",
                    help="comma-separated codec cells to run")
    ap.add_argument("--no-resume", action="store_true", help="rerun cells already present as OK")
    ap.add_argument("--refresh-timings", action="store_true",
                    help="rerun only cells lacking valid timing, preserving old measurements as legacy_* fields")
    ap.add_argument("--verify-manifest", action="store_true", help="require canonical Silesia/enwik8 manifest")
    args = ap.parse_args()

    files = [f.resolve() for f in args.files]
    if args.verify_manifest:
        verify_manifest(args.corpus, files)
    migrate_legacy_csv(args.out)
    completed = set() if args.no_resume else load_completed(args.out, require_valid_timing=args.refresh_timings)
    codecs = [x.strip() for x in args.codecs.split(",") if x.strip()]
    ran = skipped = 0
    for f in files:
        for codec in codecs:
            key = (args.corpus, f.name, codec)
            if key in completed:
                print(f"SKIP completed {args.corpus}/{f.name}/{codec}", flush=True)
                skipped += 1
                continue
            row = bench_cell(args.corpus, f, codec, args.anvil.resolve(), args.brotli.resolve())
            if args.refresh_timings:
                replace_timing_row(args.out, row)
            else:
                append_row(args.out, row)
            completed.add(key)
            ran += 1
    print(f"checkpoint complete: ran={ran}, skipped={skipped}, out={args.out}")


if __name__ == "__main__":
    main()
