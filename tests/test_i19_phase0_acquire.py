"""I19 Phase-0 provenance-only gates. No network or CPU-heavy benchmarking."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("i19_acquire", ROOT / "tools/i19_phase0_acquire.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def proposal():
    return json.loads((ROOT / "prototypes/i19-origin-lock/sources.json").read_text("utf-8"))


class AcquisitionContractTests(unittest.TestCase):
    def test_real_source_proposal_is_structurally_valid(self):
        result = module.validate_proposal(proposal())
        self.assertEqual(result["sources"], 4)
        self.assertEqual(result["split_counts"]["validation"], 2)
        self.assertFalse(result["promotion_authorized"])

    def test_duplicate_content_origin_rejected(self):
        data = proposal()
        data["sources"][2]["origin_id"] = data["sources"][1]["origin_id"]
        data["sources"][2]["split"] = "heldout"
        with self.assertRaisesRegex(ValueError, "origin leakage"):
            module.validate_proposal(data)

    def test_same_dataset_family_crossing_rejected(self):
        data = proposal()
        data["sources"][3]["dataset_family_id"] = data["sources"][0]["dataset_family_id"]
        with self.assertRaisesRegex(ValueError, "family leakage"):
            module.validate_proposal(data)

    def test_insecure_http_rejected(self):
        data = proposal()
        data["sources"][0]["url"] = "http://example.org/test"
        with self.assertRaisesRegex(ValueError, "credential-free HTTPS"):
            module.validate_proposal(data)

    def test_credentialed_url_rejected(self):
        data = proposal()
        data["sources"][0]["url"] = "https://user:secret@example.org/path"
        with self.assertRaisesRegex(ValueError, "credential-free HTTPS"):
            module.validate_proposal(data)

    def test_overlarge_source_rejected(self):
        data = proposal()
        data["sources"][0]["max_bytes"] = 60_000_000
        with self.assertRaisesRegex(ValueError, "source byte cap"):
            module.validate_proposal(data)

    def test_negative_control_not_passed_as_source(self):
        data = proposal()
        data["sources"][0]["representation"] = "uint32le"
        with self.assertRaisesRegex(ValueError, "byte-identity"):
            module.validate_proposal(data)

    def test_heldout_candidate_is_unfrozen_after_snapshot(self):
        data = proposal()
        with tempfile.TemporaryDirectory() as td:
            def fake_snapshot(url, target, cap):
                target.parent.mkdir(parents=True, exist_ok=True)
                body = url.encode("ascii")
                target.write_bytes(body)
                import hashlib
                return len(body), hashlib.sha256(body).hexdigest(), {"final_url": url}
            with patch.object(module, "snapshot", side_effect=fake_snapshot):
                report = module.acquire(data, Path(td) / "capture", "c" * 40)
            out = Path(td) / "capture"
            manifest = json.loads((out / "candidate-manifest.UNFROZEN.json").read_text("utf-8"))
            self.assertFalse(manifest["frozen"])
            self.assertEqual(len(manifest["sources"]), 4)
            self.assertEqual(len(report["sources"]), 4)
            self.assertFalse(report["codec_efficacy_measured"])
            self.assertTrue(report["heldout_unsealed"])
            for record in report["sources"]:
                blob = out / record["raw_artifact"]
                self.assertEqual(len(blob.read_bytes()), record["source_bytes"])

    def test_partial_failure_is_recorded_fail_closed(self):
        data = proposal()
        with tempfile.TemporaryDirectory() as td:
            with patch.object(module, "snapshot", side_effect=ValueError("rejected upstream")):
                with self.assertRaisesRegex(ValueError, "rejected upstream"):
                    module.acquire(data, Path(td) / "capture", "b" * 40)
            report = json.loads((Path(td) / "capture" / "acquisition-evidence.json").read_text("utf-8"))
            self.assertEqual(report["status"], "BLOCKED_ACQUISITION")
            self.assertFalse(report["promotion_authorized"])

    def test_cross_domain_redirect_is_rejected(self):
        handler = module.SameOriginOnly()
        import urllib.request
        request = urllib.request.Request("https://data.example.org/file")
        with self.assertRaisesRegex(ValueError, "unsafe cross-origin"):
            handler.redirect_request(request, None, 302, "Moved", {},
                                     "https://other.example.org/file")


if __name__ == "__main__":
    unittest.main()
