"""Small scalar A70 orchestration checks; never build or execute the codec."""
from __future__ import annotations

import hashlib
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
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
