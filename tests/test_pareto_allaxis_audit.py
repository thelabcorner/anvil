"""Independent scalar adversarial checks for optional all-axis claim audit.

No compressor build, benchmark, fuzz or network. This gate is NOT frozen I9.
"""
import copy
import hashlib
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from pareto_allaxis_audit import compare

H = hashlib.sha256(b"source-data").hexdigest()
ART = hashlib.sha256(b"raw-pair-evidence").hexdigest()


def record(codec, wire, size, rss):
    return {
        "codec": codec, "complete_wire_bytes": wire,
        "decoder_distribution_bytes": size,
        "decoder_footprint_includes_dependencies": True,
        "decoded_sha256": H,
        "peak_rss_samples_bytes": rss,
        "hard_gates": {
            "roundtrip": "pass", "malformed_input": "pass",
            "wire_compatibility": "pass", "streaming": "pass",
        },
        "capabilities": {"streaming": True, "random_access": False},
    }


def arm(hi=0.95, lo=0.90, valid=True):
    return {
        "run_id": "44370017321",
        "timing_valid": valid,
        "raw_pairs": 9,
        "raw_evidence_sha256": ART,
        "ci95": [lo, hi],
        "null_pre_ci95": [0.98, 1.01],
        "null_post_ci95": [0.99, 1.02],
    }


def valid_manifest():
    return {
        "schema": 1,
        "scope": {
            "file": "dickens",
            "input_bytes": 10192446,
            "input_sha256": H,
            "run_id": "44370017321",
            "source_commit": "2c5eeb9e570b1e1162cbff98d29e0a978edeca92",
        },
        "candidate": record("anvil", 200, 10000, [500, 510, 520]),
        "required_references": ["brotli-q11", "xz-9e"],
        "references": [
            record("brotli-q11", 220, 11000, [600, 610, 620]),
            record("xz-9e", 250, 12000, [700, 710, 720]),
        ],
        "paired": [
            {"reference": "brotli-q11", "encode": arm(), "decode": arm()},
            {"reference": "xz-9e", "encode": arm(), "decode": arm()},
        ],
    }


class ParetoAllAxisTests(unittest.TestCase):
    def check(self, change=None):
        m = valid_manifest()
        if change:
            change(m)
        return compare(m)

    def test_genuine_observed_scope_dominance(self):
        r = self.check()
        self.assertEqual(r["status"], "ALL_REQUIRED_REFERENCES_OBSERVED_SCOPE_DOMINANCE")
        self.assertEqual(len(r["per_reference"]), 2)
        self.assertTrue(all(x["status"] == "OBSERVED_SCOPE_DOMINANCE"
                            for x in r["per_reference"]))

    def test_faster_but_more_bytes_is_regression(self):
        def change(m):
            m["candidate"]["complete_wire_bytes"] = 9999
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "OBSERVED_REGRESSION_OR_HARD_FAIL")
        self.assertNotEqual(r["status"], "ALL_REQUIRED_REFERENCES_OBSERVED_SCOPE_DOMINANCE")

    def test_byte_win_but_decode_time_regression(self):
        def change(m):
            m["paired"][0]["decode"]["ci95"] = [1.02, 1.09]
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "OBSERVED_REGRESSION_OR_HARD_FAIL")

    def test_crossing_time_ci_is_not_certified_nonregression(self):
        def change(m):
            m["paired"][0]["decode"]["ci95"] = [0.94, 1.03]
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "INCOMPLETE_OR_STATISTICALLY_INCONCLUSIVE")

    def test_bad_null_is_inconclusive(self):
        def change(m):
            m["paired"][1]["encode"]["null_post_ci95"] = [1.01, 1.05]
        r = self.check(change)
        self.assertEqual(r["per_reference"][1]["status"], "INCOMPLETE_OR_STATISTICALLY_INCONCLUSIVE")

    def test_one_shot_rss_insufficient(self):
        def change(m):
            m["candidate"]["peak_rss_samples_bytes"] = [500]
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "INCOMPLETE_OR_STATISTICALLY_INCONCLUSIVE")

    def test_overlapping_rss_samples_inconclusive(self):
        def change(m):
            m["candidate"]["peak_rss_samples_bytes"] = [500, 650, 700]
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "INCOMPLETE_OR_STATISTICALLY_INCONCLUSIVE")

    def test_all_rss_worse_is_regression(self):
        def change(m):
            m["candidate"]["peak_rss_samples_bytes"] = [900, 910, 920]
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "OBSERVED_REGRESSION_OR_HARD_FAIL")

    def test_binary_assets_not_charged_rejected(self):
        def change(m):
            m["candidate"]["decoder_footprint_includes_dependencies"] = False
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "INCOMPLETE_OR_STATISTICALLY_INCONCLUSIVE")

    def test_capability_regression_hard_fail(self):
        def change(m):
            m["references"][0]["capabilities"]["random_access"] = True
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "OBSERVED_REGRESSION_OR_HARD_FAIL")

    def test_failed_roundtrip_hard_gate(self):
        def change(m):
            m["candidate"]["hard_gates"]["roundtrip"] = "fail"
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "INCOMPLETE_OR_STATISTICALLY_INCONCLUSIVE")

    def test_required_reference_coverage(self):
        def change(m):
            m["required_references"].append("zstd")
        r = self.check(change)
        self.assertIn("missing or extra reference", repr(r["integrity_errors"]))
        self.assertEqual(r["per_reference"][-1]["status"], "INCOMPLETE")

    def test_same_job_provenance_required(self):
        def change(m):
            m["paired"][0]["decode"]["run_id"] = "another-run"
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "INCOMPLETE_OR_STATISTICALLY_INCONCLUSIVE")

    def test_reject_missing_raw_hash(self):
        def change(m):
            del m["paired"][0]["decode"]["raw_evidence_sha256"]
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "INCOMPLETE_OR_STATISTICALLY_INCONCLUSIVE")

    def test_malformed_input_rejected(self):
        def change(m):
            m["candidate"]["decoded_sha256"] = "bad"
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "OBSERVED_REGRESSION_OR_HARD_FAIL")

    def test_not_all_references_win(self):
        def change(m):
            m["references"][1]["complete_wire_bytes"] = 150
        r = self.check(change)
        self.assertEqual(r["per_reference"][0]["status"], "OBSERVED_SCOPE_DOMINANCE")
        self.assertEqual(r["per_reference"][1]["status"], "OBSERVED_REGRESSION_OR_HARD_FAIL")
        self.assertNotEqual(r["status"], "ALL_REQUIRED_REFERENCES_OBSERVED_SCOPE_DOMINANCE")


if __name__ == "__main__":
    unittest.main()
