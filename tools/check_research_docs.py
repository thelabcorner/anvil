#!/usr/bin/env python3
"""Lightweight, stdlib-only documentation link and evidence-language checks.

Usage: python tools/check_research_docs.py README.md docs/*.md
Designed for local *inexpensive* static checks (not a compression workload).
Does not claim to validate externally hosted URLs, scientific assertions,
artifact authenticity, or benchmark admissibility.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit

_LINK = re.compile(r"(?<!!)\[[^\]]+\]\(([^)]+)\)")


def check_file(path: Path) -> list[str]:
    problems = []
    try:
        content = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as exc:
        return [f"{path}: cannot read: {exc}"]
    for match in _LINK.finditer(content):
        url = match.group(1).strip()
        if not url or url.startswith(("#", "mailto:", "data:")):
            continue
        split = urlsplit(url)
        if split.scheme or split.netloc:
            continue
        path_part = unquote(split.path)
        if not path_part or " " in path_part:
            continue  # URLs with optional titles are not handled by this small checker
        target = (path.parent / path_part)
        if not target.exists():
            line = content.count("\n", 0, match.start()) + 1
            problems.append(f"{path}:{line}: missing relative link {url!r}")
    return problems


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="+", type=Path)
    args = parser.parse_args(argv)
    files = sorted(set(args.files))
    problems = [item for path in files for item in check_file(path)]
    for problem in problems:
        print(problem)
    print(f"DOC_LINT files={len(files)} missing_links={len(problems)}")
    return int(bool(problems))


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
