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

    @staticmethod
    def _uvar(n):
        out = bytearray()
        while True:
            b = n & 0x7f
            n >>= 7
            out.append(b | (0x80 if n else 0))
            if not n:
                return bytes(out)

    def _frame(self, routes, source_len=128, tail=b""):
        """Source-accurate ANV0 revision-2 framing; fake payloads are NOT decoded."""
        contents = bytearray(b"ANV0\x02")
        contents.extend(self._uvar(128 << 20))
        contents.extend(self._uvar(source_len))
        for n, backend in routes:
            payload = bytes((0, backend)) + self._uvar(n) + b"x"
            contents.extend(self._uvar(n))
            contents.append(17)
            contents.extend(self._uvar(len(payload)))
            contents.extend(b"\x00" * 4)
            contents.extend(payload)
        contents.extend(tail)
        return bytes(contents)

    def _parse_synthetic_wire(self, payload, raw_bytes):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "old.anv"
            path.write_bytes(payload)
            return scout.inspect_ratio_wire(path, raw_bytes)

    def test_bwt_frame_routes_correctly_before_timing(self):
        obj = self._parse_synthetic_wire(self._frame([(80, 2), (48, 2)]), 128)
        self.assertEqual(obj["backend_blocks"], {"brotli": 0, "bwt": 2, "raw": 0})
        self.assertEqual(obj["declared_source_bytes"], 128)
        self.assertEqual(scout.require_preregistered_route(obj, "dickens"), "bwt")
        self.assertEqual(scout.require_preregistered_route(obj, "mr"), "bwt")

    def test_brotli_control_route_is_not_mistaken_for_bwt(self):
        obj = self._parse_synthetic_wire(self._frame([(128, 1)]), 128)
        self.assertEqual(scout.require_preregistered_route(obj, "mozilla"), "brotli")
        with self.assertRaisesRegex(RuntimeError, "ROUTING_DRIFT"):
            scout.require_preregistered_route(obj, "dickens")

    def test_mixed_backend_file_cannot_claim_decoding_isolates_libsais(self):
        obj = self._parse_synthetic_wire(self._frame([(80, 2), (48, 1)]), 128)
        with self.assertRaisesRegex(RuntimeError, "ROUTING_DRIFT"):
            scout.require_preregistered_route(obj, "mr")
        with self.assertRaisesRegex(RuntimeError, "ROUTING_DRIFT"):
            scout.require_preregistered_route(obj, "mozilla")

    def test_unfrozen_file_cannot_be_assigned_arbitrary_bwt_route(self):
        obj = self._parse_synthetic_wire(self._frame([(128, 2)]), 128)
        with self.assertRaisesRegex(ValueError, "only preregisters"):
            scout.require_preregistered_route(obj, "nci")

    def test_truncated_ratio_payload_header_is_hard_failure(self):
        with self.assertRaises(RuntimeError):
            self._parse_synthetic_wire(self._frame([(128, 2)])[:-2], 128)

    def test_trailing_payload_is_hard_failure(self):
        with self.assertRaisesRegex(RuntimeError, "trailing bytes"):
            self._parse_synthetic_wire(self._frame([(128, 2)], tail=b"\x00"), 128)

    def test_wrong_declared_source_length_is_hard_failure(self):
        with self.assertRaisesRegex(RuntimeError, "disagrees"):
            self._parse_synthetic_wire(self._frame([(128, 2)]), 127)

    def test_unknown_backend_id_is_hard_failure(self):
        with self.assertRaisesRegex(RuntimeError, "invalid transform/backend"):
            self._parse_synthetic_wire(self._frame([(128, 3)]), 128)

    def test_noncanonical_header_is_hard_failure(self):
        wire = self._frame([(128, 2)])
        # The 128MiB canonical block-cap varint must be preserved.
        k = len(b"ANV0\x02")
        malformed = wire[:k] + b"\x80\x00" + wire[k + len(self._uvar(128 << 20)):]
        with self.assertRaisesRegex(RuntimeError, "noncanonical"):
            self._parse_synthetic_wire(malformed, 128)

    def test_frozen_previous_run_fingerprint_is_reported(self):
        # Synthetic scalar fixture: the real 3 expected digests come from
        # immutable A48 artifact 14480494484, not from this fake fixture.
        payload = b"ANVIL old source, fixed test-only wire"
        digest = hashlib.sha256(payload).hexdigest()
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "old.anv"
            path.write_bytes(payload)
            with patch.dict(scout.FROZEN_A48_OLD_WIRE,
                            {"dickens": (len(payload), digest)}):
                result = scout.audit_frozen_old_wire(path, "dickens")
                self.assertTrue(result["matches_byte_for_byte"])
                self.assertEqual(result["cross_run_comparability"], "EXACT_HISTORICAL_REPLAY")
                self.assertEqual(result["artifact_id"], 14480494484)

    def test_changed_wire_same_size_marks_historical_drift_not_decoder_h0(self):
        payload = b"wire-golden"
        digest = hashlib.sha256(payload).hexdigest()
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "old.anv"
            path.write_bytes(b"wire-BROKEN")
            with patch.dict(scout.FROZEN_A48_OLD_WIRE,
                            {"dickens": (len(payload), digest)}):
                result = scout.audit_frozen_old_wire(path, "dickens")
                self.assertFalse(result["matches_byte_for_byte"])
                self.assertEqual(result["cross_run_comparability"],
                                 "DRIFT_DIAGNOSTIC_NO_CROSS_RUN_COMPARISON")

    def test_changed_wire_length_marks_historical_drift_not_decoder_h0(self):
        payload = b"wire-golden"
        digest = hashlib.sha256(payload).hexdigest()
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "old.anv"
            path.write_bytes(payload + b"\x00")
            with patch.dict(scout.FROZEN_A48_OLD_WIRE,
                            {"mr": (len(payload), digest)}):
                result = scout.audit_frozen_old_wire(path, "mr")
                self.assertFalse(result["matches_byte_for_byte"])
                self.assertNotEqual(result["actual_bytes"], result["expected_bytes"])

    def test_ratio_varint_cannot_cross_declared_payload_boundary(self):
        # Ratio header is [transform=0, backend=2, xlen]. The lone 0x80
        # is an UNTERMINATED xlen and must not consume another block's bytes.
        head = b"ANV0\x02" + self._uvar(128 << 20) + self._uvar(128)
        block = (self._uvar(128) + b"\x11" + self._uvar(3) +
                 b"\x00" * 4 + bytes((0, 2, 0x80)))
        with self.assertRaisesRegex(RuntimeError, "crosses declared payload boundary"):
            self._parse_synthetic_wire(head + block, 128)

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
