"""Scalar, filesystem-only tests. No codec builds, compression or network."""
import hashlib
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from corpus_lineage_gate import evaluate


def sha(payload):
    return hashlib.sha1(b"blob " + str(len(payload)).encode() + b"\0" + payload).hexdigest()


class LineageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        (self.root / "generator.py").write_text("print('source')\n")
        p = (self.root / "generator.py").read_bytes()
        (self.root / "generator2.py").write_text("print('independent-source')\n")
        self.names = ["discovery.json", "samefamily.jsonl", "unrelated.json"]
        self.manifest = {"schema": 1, "files": []}
        for name, family in zip(self.names, ["generator-A", "generator-A", "generator-B"]):
            data = ("abc:" + name).encode()
            (self.root / name).write_bytes(data)
            generator = "generator2.py" if family == "generator-B" else "generator.py"
            self.manifest["files"].append({"path": name, "generator_family": family,
                "git_blob_sha1": sha(data), "generator_path": generator,
                "generator_git_blob_sha1": sha((self.root / generator).read_bytes())})

    def tearDown(self):
        self.temp.cleanup()

    def run_gate(self, validation):
        return evaluate(self.manifest, self.root, [self.names[0]], [validation])

    def test_same_generator_cannot_be_called_independent_holdout(self):
        x = self.run_gate(self.names[1])
        self.assertEqual(x["decision"], "SAME_FAMILY_TRANSFER_NOT_INDEPENDENT_HOLDOUT")
        self.assertFalse(x["errors"])

    def test_different_declared_generator_not_claimed_independent(self):
        x = self.run_gate(self.names[2])
        self.assertEqual(x["decision"], "DISJOINT_DECLARED_LINEAGES_INDEPENDENCE_NOT_PROVEN")

    def test_forged_distinct_family_still_same_generator(self):
        self.manifest["files"][1]["generator_family"] = "fraudulent-independent-name"
        x = self.run_gate(self.names[1])
        self.assertEqual(x["decision"], "SAME_FAMILY_TRANSFER_NOT_INDEPENDENT_HOLDOUT")
        self.assertTrue(x["pairs"][0]["same_generator_source"])

    def test_mutated_corpus_fail_closed(self):
        (self.root / self.names[1]).write_text("changed")
        x = self.run_gate(self.names[1])
        self.assertEqual(x["decision"], "INVALID_PROVENANCE_OR_INPUTS")
        self.assertIn("content ID mismatch", repr(x["errors"]))

    def test_mutated_generator_fail_closed(self):
        (self.root / "generator.py").write_text("print('changed')\n")
        x = self.run_gate(self.names[1])
        self.assertEqual(x["decision"], "INVALID_PROVENANCE_OR_INPUTS")
        self.assertIn("generator content ID mismatch", repr(x["errors"]))

    def test_missing_validation_fail_closed(self):
        x = self.run_gate("missing.json")
        self.assertEqual(x["decision"], "INVALID_PROVENANCE_OR_INPUTS")

    def test_path_escape_fail_closed(self):
        self.manifest["files"][1]["path"] = "../escape.txt"
        x = self.run_gate("../escape.txt")
        self.assertEqual(x["decision"], "INVALID_PROVENANCE_OR_INPUTS")

    def test_reused_same_path_fail_closed(self):
        x = evaluate(self.manifest, self.root, [self.names[0]], [self.names[0]])
        self.assertIn("same file occurs", repr(x["errors"]))

    def test_duplicate_manifest_rejected(self):
        self.manifest["files"].append(self.manifest["files"][0].copy())
        x = self.run_gate(self.names[1])
        self.assertIn("duplicate path", repr(x["errors"]))


if __name__ == "__main__":
    unittest.main()
