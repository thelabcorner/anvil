#!/usr/bin/env python3
"""
block_oracle.py — E4 block-local backend routing oracle for ANVIL.

For each (file, block_size) we:
  * compress the file with ANVIL mode-17 BWT with --block=N. Because each block
    is an independent mode-17 payload, ONE subprocess yields every per-block BWT
    coded size. We parse the container to recover, per block:
        - blen (true block length)
        - bwt_coded_bytes (the raw BWT-backend payload, with no outer framing)
        - whether the block fell back to raw (mode byte 0 in the container)
  * independently compress each fixed block with direct Brotli q11/lw30 via
    build\\brotli_lw.exe (the canonical reference; py-brotli caps lgwin at 24 so
    it cannot reproduce large-window reference numbers). Brotli is deterministic,
    so results are cached on disk and the run is fully resumable. Each Brotli call
    is round-trip validated (decoded back and byte-compared to the input).
  * compute the oracle = sum over blocks of min(brotli, bwt) + *real* ANVIL
    rev-2 framing (not guessed), plus a fully honest variant that also allows
    storing the block raw.

Output: a resumable CSV (one row per block) at --out. Rows already present for a
(file, block_size) are reused; only missing Brotli blocks are recomputed.

Framing model (derived from src/anvil.cpp compress(), ratio-first envelope):
  container header : b'ANV0' + rev(2) + uvar(block_size) + uvar(total)
  per block (coded): 1 mode byte(17) + uvar(payload_len) + 4 CRC32
                     + payload,  payload = 1 transform(0) + 1 backend + uvar(blen) + coded
  per block (raw)  : 1 mode byte(0)  + uvar(blen)       + 4 CRC32 + blen

So, for a routed block using winner coded-bytes c:
  coded_block_total = 1 + uvar_len(payload_len) + 4 + (2 + uvar_len(blen) + c)
  raw_block_total   = 1 + uvar_len(blen)       + 4 + blen
The oracle per block = min(coded_block_total using brotli,
                            coded_block_total using bwt,
                            raw_block_total).

NOTE on --workers: this only parallelizes the Brotli subprocess calls. anvil.exe is
invoked once per (file, size). The pinned clang-cl build's anvil.exe has been
observed to crash (exit 0xFFFFFFFF) when many processes open container files
concurrently, so the default is 1. Raise it for the MSVC recovery build if desired.
"""
import argparse
import csv
import math
import os
import subprocess
import tempfile
import concurrent.futures as cf

KI = 1024
MI = 1024 * 1024


def uvar_len(x: int) -> int:
    n = 1
    x >>= 7
    while x:
        n += 1
        x >>= 7
    return n


def read_file(path: str) -> bytes:
    with open(path, "rb") as f:
        return f.read()


# --------------------------------------------------------------------------
# ANVIL mode-17 container parser: extract per-block BWT info.
# --------------------------------------------------------------------------
def parse_anvil_bwt_container(data: bytes):
    if data[:4] != b"ANV0" or data[4] != 2:
        raise RuntimeError("not a rev-2 ANVIL container")
    p = 4 + 1  # skip magic + revision

    def get_uvar():
        nonlocal p
        x = 0
        shift = 0
        for _ in range(10):
            b = data[p]; p += 1
            x |= (b & 0x7F) << shift
            if not (b & 0x80):
                return x
            shift += 7
        raise RuntimeError("varint overflow")

    block_size = get_uvar()
    total = get_uvar()
    blocks = []
    out_len = 0
    while out_len < total:
        blen = get_uvar()
        mode = data[p]; p += 1
        if mode == 17:
            payload_len = get_uvar()
            crc = int.from_bytes(data[p:p + 4], "little"); p += 4
            payload = data[p:p + payload_len]; p += payload_len
            transform = payload[0]; backend = payload[1]
            q = 2
            transformed = 0; shift = 0
            while True:
                b = payload[q]; q += 1
                transformed |= (b & 0x7F) << shift
                if not (b & 0x80):
                    break
                shift += 7
            coded = payload_len - q
            if backend != 2:
                raise RuntimeError(f"container block backend={backend}, expected BWT(2)")
            if transform != 0:
                raise RuntimeError(f"container block transform={transform}, expected direct(0)")
            blocks.append({"blen": blen, "bwt_coded": coded, "raw": False})
            out_len += blen
        elif mode == 0:
            rawsize = get_uvar()
            crc = int.from_bytes(data[p:p + 4], "little"); p += 4
            p += rawsize
            blocks.append({"blen": blen, "bwt_coded": None, "raw": True})
            out_len += blen
        else:
            raise RuntimeError(f"unexpected block mode {mode}")
    return block_size, total, blocks


def run_anvil_bwt_container(anvil_exe, infile, block_size, workdir):
    out = os.path.join(workdir, os.path.basename(infile) + f".{block_size}.bwt.anv")
    if os.path.exists(out) and os.path.getsize(out) > 0:
        try:
            data = read_file(out)
            bs, total, blocks = parse_anvil_bwt_container(data)
            if bs == block_size and total == os.path.getsize(infile):
                return blocks
        except Exception:
            pass
    subprocess.run(
        [anvil_exe, "c", infile, out,
         "--parse=ratio", "--ratio-backend=bwt",
         "--ratio-context=off", "--ratio-lines=off",
         f"--block={block_size}", "--quiet"],
        check=True,
    )
    data = read_file(out)
    _, _, blocks = parse_anvil_bwt_container(data)
    return blocks


def brotli_block_size(brotli_exe, block: bytes, tmpdir, expected_len: int, retries=5) -> int:
    """Return Brotli q11/lw30 compressed size of `block` (deterministic).

    Under high worker counts build\\brotli_lw.exe occasionally returns exit 0 but
    writes no output file (a spawn/stdio artifact). We validate by round-trip
    decoding back to the exact input; only a correct, sized output is accepted.
    """
    for attempt in range(retries):
        with tempfile.NamedTemporaryFile(delete=False, dir=tmpdir, suffix=".bin") as tf:
            tf.write(block)
            src = tf.name
        dst = src + ".br"
        try:
            subprocess.run([brotli_exe, "c", src, dst],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
            if not os.path.exists(dst) or os.path.getsize(dst) == 0:
                continue  # broken run, retry
            dec = subprocess.run([brotli_exe, "d", dst, dst + ".dec", str(expected_len)],
                                 stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if dec.returncode != 0 or not os.path.exists(dst + ".dec"):
                continue
            if os.path.getsize(dst + ".dec") != expected_len:
                continue
            with open(dst + ".dec", "rb") as f:
                if f.read() != block:
                    continue
            return os.path.getsize(dst)
        except Exception:
            pass
        finally:
            for p in (src, dst, dst + ".dec"):
                try:
                    os.remove(p)
                except OSError:
                    pass
    raise RuntimeError(f"brotli_lw failed after {retries} attempts (block {expected_len} B)")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="scratch/block-oracle.csv")
    ap.add_argument("--corpus-dir", default="scratch/ratio-first/corpora/silesia")
    ap.add_argument("--anvil", default="build/anvil.exe")
    ap.add_argument("--brotli", default="build/brotli_lw.exe")
    ap.add_argument("--sizes", default="4194304,16777216",
                    help="comma list of block sizes in bytes")
    ap.add_argument("--files", default="mozilla,samba,ooffice,sao,webster",
                    help="comma list of corpus files; or 'all' for 12 Silesia files")
    ap.add_argument("--workers", type=int, default=1,
                    help="parallel Brotli subprocess workers (anvil.exe is 1/process). "
                         "Use 1 with the pinned clang-cl build; higher OK with MSVC recovery build.")
    ap.add_argument("--tmpdir", default="scratch/block-oracle/tmp")
    args = ap.parse_args()

    sizes = [int(s) for s in args.sizes.split(",")]
    if args.files == "all":
        files = ["dickens", "mozilla", "mr", "nci", "ooffice", "osdb",
                 "reymont", "samba", "sao", "webster", "x-ray", "xml"]
    else:
        files = [f for f in args.files.split(",") if f]

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    os.makedirs(args.tmpdir, exist_ok=True)

    done = {}  # (file, block_size, block_index) -> row dict
    fieldnames = ["file", "block_size", "block_index", "blen",
                  "brotli_bytes", "bwt_coded_bytes", "bwt_raw", "winner", "margin"]
    if os.path.exists(args.out):
        with open(args.out, newline="") as f:
            for r in csv.DictReader(f):
                done[(r["file"], int(r["block_size"]), int(r["block_index"]))] = r

    write_header = not os.path.exists(args.out)
    out_f = open(args.out, "a", newline="")
    w = csv.DictWriter(out_f, fieldnames=fieldnames)
    if write_header:
        w.writeheader()

    # whole-file reference (to compute warmup + whole-file regret)
    wholefile = {}
    whole_csv = os.path.join(os.path.dirname(args.out) or ".",
                             os.path.basename(args.out).replace(".csv", "-wholes.csv"))
    whole_rows = {}
    if os.path.exists(whole_csv):
        with open(whole_csv, newline="") as f:
            for r in csv.DictReader(f):
                whole_rows[r["file"]] = (int(r["brotli_bytes"]), int(r["bwt_coded_bytes"]))
    for fn in files:
        if fn in whole_rows:
            wholefile[fn] = whole_rows[fn]
            print(f"[whole-file cached] {fn}: brotli={wholefile[fn][0]} bwt={wholefile[fn][1]}",
                  flush=True)
            continue
        infile = os.path.join(args.corpus_dir, fn)
        data = read_file(infile)
        wb = brotli_block_size(args.brotli, data, args.tmpdir, len(data))
        wblocks = run_anvil_bwt_container(args.anvil, infile, len(data),
                                          os.path.dirname(args.out))
        assert len(wblocks) == 1 and wblocks[0]["blen"] == len(data)
        wbt = wblocks[0]["bwt_coded"]
        wholefile[fn] = (wb, wbt)
        whole_rows[fn] = (wb, wbt)
        print(f"[whole-file] {fn}: brotli={wb} bwt={wbt} winner={'bwt' if wbt < wb else 'brotli'}",
              flush=True)
    with open(whole_csv, "w", newline="") as f:
        ww = csv.writer(f)
        ww.writerow(["file", "brotli_bytes", "bwt_coded_bytes"])
        for fn in files:
            wb, wbt = wholefile[fn]
            ww.writerow([fn, wb, wbt])
    print("WROTE", whole_csv, flush=True)

    for fn in files:
        infile = os.path.join(args.corpus_dir, fn)
        data = read_file(infile)
        total = len(data)
        for bs in sizes:
            bblocks = run_anvil_bwt_container(args.anvil, infile, bs,
                                              os.path.dirname(args.out))
            nblocks = len(bblocks)
            jobs = []
            for i, b in enumerate(bblocks):
                if (fn, bs, i) not in done and not b["raw"]:
                    start = i * bs
                    blk = data[start:start + b["blen"]]
                    jobs.append((i, blk, b["blen"]))
            brotli_cache = {}
            if jobs:
                with cf.ProcessPoolExecutor(max_workers=args.workers) as ex:
                    fut = {ex.submit(brotli_block_size, args.brotli, blk, args.tmpdir, blen): i
                           for i, blk, blen in jobs}
                    for f in cf.as_completed(fut):
                        i = fut[f]
                        brotli_cache[i] = f.result()
            for i, b in enumerate(bblocks):
                key = (fn, bs, i)
                if key in done:
                    continue
                blen = b["blen"]
                if b["raw"]:
                    brotli_bytes = brotli_cache.get(i, None)
                    bwt_coded = None
                    winner = "raw"
                    margin = None
                else:
                    brotli_bytes = brotli_cache[i]
                    bwt_coded = b["bwt_coded"]
                    if brotli_bytes < bwt_coded:
                        winner = "brotli"; margin = bwt_coded - brotli_bytes
                    else:
                        winner = "bwt"; margin = brotli_bytes - bwt_coded
                row = {
                    "file": fn, "block_size": bs, "block_index": i, "blen": blen,
                    "brotli_bytes": "" if brotli_bytes is None else brotli_bytes,
                    "bwt_coded_bytes": "" if bwt_coded is None else bwt_coded,
                    "bwt_raw": int(b["raw"]), "winner": winner,
                    "margin": "" if margin is None else margin,
                }
                w.writerow(row)
                done[key] = row
            out_f.flush()
            print(f"[done] {fn} bs={bs}: {nblocks} blocks, {len(jobs)} brotli runs",
                  flush=True)

    out_f.close()
    print("WROTE", args.out, flush=True)


if __name__ == "__main__":
    main()
