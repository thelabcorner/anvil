"""Cheap pure-Python manifest gate tests; heavy codec work is CI-only."""
import copy
import importlib.util
import pathlib
import unittest

TOOL = pathlib.Path(__file__).resolve().parents[1] / "tools/i19_validate_corpus_manifest.py"
spec = importlib.util.spec_from_file_location("i19_manifest", TOOL)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def sample():
    sources = []
    for i, split in enumerate(("discovery", "validation", "heldout")):
        sources.append({
            "id": f"sample_{i}", "origin_id": f"agency_{i}",
            "dataset_family_id": f"dataset_{i}", "split": split,
            "publisher": "Testing agency", "url": f"https://example.org/{i}",
            "release": "r1", "retrieved_date": "2026-10-07",
            "license": "Open test fixture", "sha256": f"{i + 1:064x}",
            "source_bytes": 64, "representation": "uint32le",
            "preprocess": "identity on typed 32-bit payload",
            "original_source_reconstructible": False,
        })
    return {"schema": "anvil.i19.origin-locked.v1", "frozen": True, "sources": sources}


class ManifestGateTests(unittest.TestCase):
    def test_three_independent_origins_pass(self):
        result = module.validate(sample())
        self.assertEqual(result["status"], "MANIFEST_STRUCTURE_PASS")
        self.assertFalse(result["content_integrity_attested"])

    def test_origin_leakage_fails(self):
        manifest = sample()
        manifest["sources"][1]["origin_id"] = "agency_0"
        with self.assertRaisesRegex(ValueError, "origin leakage"):
            module.validate(manifest)

    def test_family_leakage_fails(self):
        manifest = sample()
        manifest["sources"][2]["dataset_family_id"] = "dataset_0"
        with self.assertRaisesRegex(ValueError, "family leakage"):
            module.validate(manifest)

    def test_duplicate_byte_identity_fails(self):
        manifest = sample()
        manifest["sources"][1]["sha256"] = manifest["sources"][0]["sha256"]
        with self.assertRaisesRegex(ValueError, "duplicate source content"):
            module.validate(manifest)

    def test_unfrozen_cannot_measure(self):
        manifest = sample()
        manifest["frozen"] = False
        with self.assertRaisesRegex(ValueError, "explicitly frozen"):
            module.validate(manifest)

    def test_claimed_source_reconstruction_requires_recipe(self):
        manifest = sample()
        manifest["sources"][0]["original_source_reconstructible"] = True
        with self.assertRaisesRegex(ValueError, "lacks recipe"):
            module.validate(manifest)

    def test_embedded_credentials_disallowed(self):
        manifest = sample()
        manifest["sources"][0]["url"] = "https://secret:token@example.org/test"
        with self.assertRaisesRegex(ValueError, "credential-free"):
            module.validate(manifest)

    def test_insufficient_origins_fails(self):
        manifest = sample()
        manifest["sources"] = manifest["sources"][:2]
        with self.assertRaisesRegex(ValueError, "three origins"):
            module.validate(manifest)


if __name__ == "__main__":
    unittest.main()
