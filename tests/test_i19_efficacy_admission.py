"""Synthetic-only I19 efficacy admission contract tests; no network or codec execution."""
from __future__ import annotations
import copy
import datetime as dt
import importlib.util
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
spec = importlib.util.spec_from_file_location("i19_efficacy", ROOT / "tools/i19_validate_efficacy_admission.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def admission():
    sources = []
    archive, rights, typed = {}, {}, []
    for i, role in enumerate(("discovery", "validation", "validation", "heldout")):
        sid = f"origin_{i}"
        sha = f"{i + 1:064x}"
        sources.append({
            "id": sid, "origin_id": f"publisher_{i}",
            "dataset_family_id": f"family_{i}", "split": role,
            "publisher": f"Publisher {i}", "url": f"https://example.org/{i}/data",
            "release": "release-1", "retrieved_date": "2026-10-07",
            "license": "test-only synthetic", "sha256": sha, "source_bytes": 64,
            "representation": "raw-bytes", "preprocess": "identity",
            "original_source_reconstructible": True,
        })
        archive[sid] = {
            "status": "sha-attested", "backend": "durable-immutable-access-controlled",
            "sha256": sha, "bytes": 64, "evidence_url": f"https://example.org/{i}/attest",
            "custodian": "Test custodian",
        }
        rights[sid] = {
            "review": "cleared", "redistribution_allowed": True,
            "attribution": "Example", "evidence_url": f"https://example.org/{i}/rights",
        }
        if role != "heldout":
            typed.append({
                "source_id": sid, "source_sha256": sha,
                "dtype": "int32le", "bytes": 64,
                "sha256": f"{i+10:064x}", "path": f"typed/stream_{i}.bin",
                "original_source_reconstructible": False,
                "competition_class": "typed-projection-only",
            })
    return {
        "schema": "anvil.i19.efficacy-admission.v1",
        "source_manifest": {
            "schema": "anvil.i19.origin-locked.v1",
            "frozen": True, "sources": sources,
        },
        "archive": archive, "rights": rights,
        "preregistration": {
            "frozen": True, "sha256": "a" * 64, "git_commit": "c" * 40,
            "date": "2026-10-07", "controls": sorted(mod.REQUIRED_CONTROLS),
            "acceptance_criteria_locked": True, "timing_protocol_locked": True,
            "memory_protocol_locked": True, "negative_controls_locked": True,
            "discovery_only_tuning": True,
        },
        "heldout_isolation": {
            "origin_id": "publisher_3", "source_sha256": sources[-1]["sha256"],
            "independent_custodian": True, "publicly_exposed": False,
            "was_inspected_during_tuning": False, "unsealed": False,
            "sealed_date": "2026-10-06", "access_log_sha256": "b" * 64,
            "custody_attestation_url": "https://example.org/custody",
        },
        "typed_streams": typed,
    }


class EfficacyAdmissionTests(unittest.TestCase):
    def test_even_complete_synthetic_metadata_never_certifies_efficacy(self):
        report = mod.validate_admission(admission())
        self.assertEqual(report["status"], "ELIGIBLE_FOR_EXTERNAL_ATTESTATION_ONLY")
        self.assertFalse(report["external_archive_verified"])
        self.assertFalse(report["efficacy_authorized"])

    def test_old_publicly_exposed_uci_sha_cannot_be_sealed(self):
        a = admission()
        a["source_manifest"]["sources"][-1]["sha256"] = mod.PUBLIC_I19_UCI_SHA
        a["heldout_isolation"]["source_sha256"] = mod.PUBLIC_I19_UCI_SHA
        a["archive"]["origin_3"]["sha256"] = mod.PUBLIC_I19_UCI_SHA
        with self.assertRaisesRegex(ValueError, "known publicly exposed"):
            mod.validate_admission(a)

    def test_expiring_github_artifacts_are_not_immutable_archive(self):
        a = admission()
        a["archive"]["origin_0"]["backend"] = "github-actions-90-day"
        with self.assertRaisesRegex(ValueError, "durable immutable"):
            mod.validate_admission(a)

    def test_rights_pending_blocks(self):
        a = admission()
        a["rights"]["origin_1"]["review"] = "pending"
        with self.assertRaisesRegex(ValueError, "rights"):
            mod.validate_admission(a)

    def test_custody_public_exposure_blocks(self):
        a = admission()
        a["heldout_isolation"]["publicly_exposed"] = True
        with self.assertRaisesRegex(ValueError, "heldout isolation"):
            mod.validate_admission(a)

    def test_holdout_unsealed_blocks(self):
        a = admission()
        a["heldout_isolation"]["unsealed"] = True
        with self.assertRaisesRegex(ValueError, "heldout isolation"):
            mod.validate_admission(a)

    def test_missing_typed_comparator_blocks(self):
        a = admission()
        a["preregistration"]["controls"].remove("typed-codec")
        with self.assertRaisesRegex(ValueError, "reference codec"):
            mod.validate_admission(a)

    def test_insufficient_validation_origins_blocks(self):
        a = admission()
        a["source_manifest"]["sources"][2]["split"] = "discovery"
        with self.assertRaisesRegex(ValueError, "two independent validation"):
            mod.validate_admission(a)

    def test_unfrozen_manifest_blocks(self):
        a = admission()
        a["source_manifest"]["frozen"] = False
        with self.assertRaisesRegex(ValueError, "explicitly frozen"):
            mod.validate_admission(a)

    def test_heldout_stream_extraction_blocks(self):
        a = admission()
        a["typed_streams"].append({
            "source_id": "origin_3", "source_sha256": a["source_manifest"]["sources"][3]["sha256"],
            "dtype": "int32le", "bytes": 16,
            "sha256": "e" * 64, "path": "typed/heldout.bin",
            "original_source_reconstructible": False,
            "competition_class": "typed-projection-only",
        })
        with self.assertRaisesRegex(ValueError, "heldout stream exposed"):
            mod.validate_admission(a)

    def test_original_byte_source_misrepresentation_blocks(self):
        a = admission()
        a["typed_streams"][0]["original_source_reconstructible"] = True
        with self.assertRaisesRegex(ValueError, "byte identity misrepresented"):
            mod.validate_admission(a)


if __name__ == "__main__":
    unittest.main()
