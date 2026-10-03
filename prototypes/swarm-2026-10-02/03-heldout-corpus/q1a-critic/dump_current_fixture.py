#!/usr/bin/env python3
"""Dump the CURRENT constructive fixture lock + tombstone to disk for CLI audit.

Isolated read-only probe. Imports the constructive prototype, writes two JSON files
into THIS directory, and nothing else. No network, no codec, no corpus bytes.
"""
import importlib.util
import json
import os
import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
SRC = HERE.parent.parent / "q1a-corpus-lock" / "space-bunny" / "q1a_corpus_lock.py"

spec = importlib.util.spec_from_file_location("q1a_corpus_lock", SRC)
mod = importlib.util.module_from_spec(spec)
sys.modules["q1a_corpus_lock"] = mod
spec.loader.exec_module(mod)

lock = mod.fixture_lock()
tomb = mod.fixture_tombstone()

(HERE / "current-fixture-lock.json").write_text(
    json.dumps(lock, sort_keys=True, indent=1), encoding="utf-8")
(HERE / "current-fixture-tombstone.json").write_text(
    json.dumps(tomb, sort_keys=True, indent=1), encoding="utf-8")

ap = lock.get("audit_parameters", {})
print("entries                    :", len(lock["entries"]))
print("audit_parameters declared  :", sorted(ap))
missing = [k for k in mod.REQUIRED_AUDIT_PARAMETERS if k not in ap]
print("REQUIRED but UNDECLARED    :", missing)
print("roles present              :", sorted({e["role"] for e in lock["entries"]}))
print("source kinds               :",
      sorted({e["source"]["kind"] for e in lock["entries"]}))
print("tool sha256 in lock        :", (lock.get("project") or {}).get("audit_tool_sha256"))
print("current script sha256      :", mod._tool_source_sha256())
print("byte_source ids            :", len(mod.fixture_byte_sources()))
print("fixture lock sha256        :", mod.canon_sha256(lock))
print("wrote:", HERE / "current-fixture-lock.json")
print("wrote:", HERE / "current-fixture-tombstone.json")
