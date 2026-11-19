"""Small scalar A70 orchestration checks; never build or execute the codec."""
from __future__ import annotations

import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import a70_version_scout as scout
from a70_version_scout import max_rss_kib, run, sha256


class A70ScalarTests(unittest.TestCase):
    def test_sha256_exact(self):
        with tempfile.TemporaryDirectory() as td:
            f = Path(td) / "raw.bin"
            payload = b"\x00\xffANVIL\x00\x0a"
            f.write_bytes(payload)
            self.assertEqual(sha256(f), hashlib.sha256(payload).hexdigest())

    def test_normal_process_success(self):
        run([sys.executable, "-c", "print('scalar')"])

    def test_normal_process_failure_is_hard_error(self):
        with self.assertRaises(RuntimeError):
            run([sys.executable, "-c", "import sys; sys.exit(7)"])

    def test_expected_failure_is_distinguished(self):
        run([sys.executable, "-c", "import sys;sys.exit(7)"],
            expect_failure=True)

    def test_truncated_input_unexpected_success_is_hard_error(self):
        with self.assertRaises(RuntimeError):
            run([sys.executable, "-c", "pass"], expect_failure=True)

    def test_selects_first_allowed_runner_cpu_not_literal_zero(self):
        source_hash = hashlib.sha256(b"source").hexdigest()
        captured = {}
        def fake_run(argv, timeout=0):
            captured["argv"] = argv
            with Path(argv[argv.index("--summary") + 1]).open("w", encoding="utf-8") as out:
                json.dump({
                    "observed_outputs": {
                        "control": {"sha256": source_hash, "deterministic": True},
                        "candidate": {"sha256": source_hash, "deterministic": True},
                    },
                    "pairs": [{} for _ in range(7)],
                }, out)
        with tempfile.TemporaryDirectory() as td:
            out = Path(td)
            with patch.object(scout.os, "sched_getaffinity", return_value={4, 7},
                              create=True), patch.object(scout, "run", side_effect=fake_run):
                result = scout.paired(
                    out, ["old"], ["new"], out / "old.out", out / "new.out",
                    source_hash, "tiny-scalar", out, 7)
        self.assertEqual(captured["argv"][captured["argv"].index("--cpu") + 1], "4")
        self.assertEqual(result["peer_affinity_preflight"]["selected_cpu"], 4)

    @unittest.skipUnless(Path("/usr/bin/time").is_file(),
                         "GNU time unavailable in this scalar test environment")
    def test_process_rss_not_timed_as_codec(self):
        with tempfile.TemporaryDirectory() as td:
            rss = max_rss_kib(
                [sys.executable, "-c", "a = bytearray(4096); assert len(a)==4096"],
                Path(td) / "maxrss.txt")
            self.assertGreater(rss, 0)


if __name__ == "__main__":
    unittest.main()
