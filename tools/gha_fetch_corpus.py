#!/usr/bin/env python3
"""Fetch canonical ANVIL benchmark corpora for CI.

This helper is intentionally separate from benchmark execution:
- downloads only into a caller-selected directory;
- verifies every extracted file against ANVIL's pinned size + MD5 manifest;
- never modifies tests/corpus or the repository;
- uses public sources so GitHub Actions needs no secrets.

Silesia source mirror:
  https://github.com/MiloszKrajewski/SilesiaCorpus
enwik8 source:
  https://mattmahoney.net/dc/textdata.html
"""

from __future__ import annotations

import argparse
import hashlib
import shutil
import tempfile
import urllib.request
import zipfile
from pathlib import Path


SILESIA = {
    "dickens": (10_192_446, "88334708559f6db57d79096bc0aca07e"),
    "mozilla": (51_220_480, "c7789a2097f1ff944b0c737430a339b3"),
    "mr": (9_970_564, "38e623e3093b7bf2003ca4b1bbc19927"),
    "nci": (33_553_445, "31f85bc8706f3c921104e7c169e2e2e1"),
    "ooffice": (6_152_192, "573c4ae915e36631d8f2dcffb9b9b66d"),
    "osdb": (10_085_684, "e734b0c48e6a982adfb5802da3032ecd"),
    "reymont": (6_627_202, "d8f54d78105079775f32d76dc55fc671"),
    "samba": (21_606_400, "154eaea7ea70e89f6339ff0abf4112ca"),
    "sao": (7_251_944, "79e95a22e18cd82b7e42bf91b380d30b"),
    "webster": (41_458_703, "474931ad907ac27bf962c75ded46c069"),
    "x-ray": (8_474_240, "9baec32ad14ec3eff487d254382cb91c"),
    "xml": (5_345_280, "9b09c0c80104adb8aae910b7d7db003e"),
}
ENWIK8 = {"enwik8": (100_000_000, "a1fa5ffddb56f4953e226637dabbb36a")}

SILESIA_BASE = (
    "https://raw.githubusercontent.com/"
    "MiloszKrajewski/SilesiaCorpus/master/{name}.zip"
)
ENWIK8_URL = "https://mattmahoney.net/dc/enwik8.zip"


def md5(path: Path) -> str:
    h = hashlib.md5()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def verify(path: Path, size: int, expected_md5: str) -> None:
    actual_size = path.stat().st_size
    if actual_size != size:
        raise RuntimeError(
            f"{path.name}: size mismatch {actual_size:,} != {size:,}"
        )
    actual_md5 = md5(path)
    if actual_md5.lower() != expected_md5.lower():
        raise RuntimeError(
            f"{path.name}: MD5 mismatch {actual_md5} != {expected_md5}"
        )


def download(url: str, dst: Path) -> None:
    req = urllib.request.Request(
        url,
        headers={"User-Agent": "Project-ANVIL-CI/1"},
    )
    with urllib.request.urlopen(req, timeout=120) as src, dst.open("wb") as out:
        shutil.copyfileobj(src, out, length=1 << 20)


def extract_single(zip_path: Path, expected_name: str, dst: Path) -> None:
    with zipfile.ZipFile(zip_path) as zf:
        candidates = [
            info
            for info in zf.infolist()
            if not info.is_dir() and Path(info.filename).name == expected_name
        ]
        if len(candidates) != 1:
            names = [i.filename for i in zf.infolist() if not i.is_dir()]
            raise RuntimeError(
                f"{zip_path.name}: expected exactly one {expected_name!r}; "
                f"archive entries={names}"
            )
        with zf.open(candidates[0]) as src, dst.open("wb") as out:
            shutil.copyfileobj(src, out, length=1 << 20)


def fetch_silesia(root: Path) -> list[Path]:
    out = root / "silesia"
    out.mkdir(parents=True, exist_ok=True)
    fetched: list[Path] = []
    with tempfile.TemporaryDirectory(prefix="anvil-silesia-") as td:
        temp = Path(td)
        for name, (size, checksum) in SILESIA.items():
            dst = out / name
            if not dst.exists():
                archive = temp / f"{name}.zip"
                print(f"download silesia/{name}", flush=True)
                download(SILESIA_BASE.format(name=name), archive)
                extract_single(archive, name, dst)
            verify(dst, size, checksum)
            fetched.append(dst)
            print(f"verified silesia/{name}: {size:,} B {checksum}", flush=True)
    total = sum(p.stat().st_size for p in fetched)
    if total != 211_938_580:
        raise RuntimeError(f"Silesia total mismatch: {total:,}")
    return fetched


def fetch_enwik8(root: Path) -> list[Path]:
    out = root / "enwik8"
    out.mkdir(parents=True, exist_ok=True)
    dst = out / "enwik8"
    if not dst.exists():
        with tempfile.TemporaryDirectory(prefix="anvil-enwik8-") as td:
            archive = Path(td) / "enwik8.zip"
            print("download enwik8", flush=True)
            download(ENWIK8_URL, archive)
            extract_single(archive, "enwik8", dst)
    size, checksum = ENWIK8["enwik8"]
    verify(dst, size, checksum)
    print(f"verified enwik8: {size:,} B {checksum}", flush=True)
    return [dst]


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "corpus",
        choices=("silesia", "enwik8", "all"),
        help="canonical corpus to fetch",
    )
    ap.add_argument(
        "--out",
        type=Path,
        required=True,
        help="destination directory outside the repository is recommended",
    )
    args = ap.parse_args()
    root = args.out.resolve()
    root.mkdir(parents=True, exist_ok=True)

    files: list[Path] = []
    if args.corpus in ("silesia", "all"):
        files.extend(fetch_silesia(root))
    if args.corpus in ("enwik8", "all"):
        files.extend(fetch_enwik8(root))

    print(f"ready: {len(files)} files under {root}", flush=True)


if __name__ == "__main__":
    main()
