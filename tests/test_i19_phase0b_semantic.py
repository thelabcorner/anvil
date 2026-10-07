"""Synthetic tiny format checks only; real dataset processing is Actions-only."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("i19_semantic", ROOT / "tools/i19_phase0b_semantic.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class SourceSemanticsTests(unittest.TestCase):
    def test_noaa_correct_int16_selected_values(self):
        lines = []
        for year in (2000, 2010):
            for month in range(1, 13):
                for element in ("TMAX", "TMIN"):
                    field = f"{123 if element == 'TMAX' else -9999:5d}   "
                    line = f"USW00003947{year:04d}{month:02d}{element}" + field * 31
                    self.assertEqual(len(line), 269)
                    lines.append(line)
        result = module.decode_noaa(("\n".join(lines) + "\n").encode("ascii"))
        self.assertEqual(result["tmax"][1]["samples"], 744)
        self.assertEqual(result["tmin"][1]["missing_count"], 744)
        self.assertEqual(len(result["tmax"][0]), 1488)
        self.assertFalse(result["tmax"][1]["original_source_reconstructible"])

    def test_noaa_rejects_wrong_station(self):
        raw = (("USW00000000" + "200001TMAX" + "  123   " * 31 + "\n") * 30).encode()
        with self.assertRaisesRegex(ValueError, "station identity"):
            module.decode_noaa(raw)

    def test_usgs_exact_decimal_scaling_and_date(self):
        daily = [{"dateTime": f"2000-01-{i % 28 + 1:02d}T00:00:00.000", "value": "1.125"}
                 for i in range(365)]
        # Unique ascending days across two years are not required for this miniature parser case;
        # create strictly increasing ISO dates instead.
        from datetime import date, timedelta
        start = date(2000, 1, 1)
        for i, item in enumerate(daily):
            item["dateTime"] = (start + timedelta(days=i)).isoformat() + "T00:00:00.000"
        raw = {"value": {"timeSeries": [{"variable": {"variableCode": [{"value": "00060"}]},
                                        "sourceInfo": {"siteCode": [{"value": "01646500"}]},
                                        "values": [{"value": daily}]}]}}
        output = module.decode_usgs(json.dumps(raw).encode())
        self.assertEqual(output["discharge"][1]["samples"], 365)
        self.assertEqual(len(output["discharge"][0]), 1460)
        self.assertEqual(module.scaled_int32("1.125"), 1125)

    def test_fixed_point_precision_rejection(self):
        with self.assertRaisesRegex(ValueError, "precision"):
            module.scaled_int32("1.00001")
        with self.assertRaisesRegex(ValueError, "non-finite"):
            module.scaled_int32("NaN")

    def test_nasa_sorted_day_keys(self):
        from datetime import date, timedelta
        start = date(2000, 1, 1)
        series = {(start + timedelta(days=i)).strftime("%Y%m%d"): 12.34 for i in range(365)}
        result = module.decode_nasa(json.dumps({"properties": {"parameter": {"T2M": series}}}).encode())
        self.assertEqual(result["t2m"][1]["samples"], 365)
        self.assertEqual(len(result["t2m"][0]), 1460)

    def test_heldout_never_parsed_even_with_valid_snapshot_sha(self):
        import hashlib
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            root = base / "source"
            (root / "snapshot/raw").mkdir(parents=True)
            inputs = [
                ("noaa_kmci_daily", "discovery"), ("usgs_potomac_daily", "validation"),
                ("nasa_power_t2m_daily", "validation"), ("uci_household_electricity", "heldout"),
            ]
            lock = {"schema": "anvil.i19.source-byte-lock.v1",
                    "status": "SOURCE_BYTES_FROZEN_EFFICACY_NOT_AUTHORIZED",
                    "efficacy_authorized": False, "heldout_unsealed": True,
                    "origin_run_id": 1, "origin_commit": "f" * 40, "sources": []}
            candidate = {"schema": "anvil.i19.origin-locked.v1", "frozen": False, "sources": []}
            for name, split in inputs:
                data = b"opaque"
                path = root / "snapshot/raw" / (name + ".source")
                path.write_bytes(data)
                digest = hashlib.sha256(data).hexdigest()
                item = {"id": name, "split": split, "sha256": digest, "source_bytes": len(data)}
                lock["sources"].append(item)
                candidate["sources"].append(item)
            (root / "snapshot/candidate-manifest.UNFROZEN.json").write_text(json.dumps(candidate))
            with patch.dict(module.PARSERS, {
                "noaa_kmci_daily": lambda _: {"a": (b"\x01\x00", {"original_source_reconstructible": False})},
                "usgs_potomac_daily": lambda _: {"a": (b"\x01\x00", {"original_source_reconstructible": False})},
                "nasa_power_t2m_daily": lambda _: {"a": (b"\x01\x00", {"original_source_reconstructible": False})},
                "uci_household_electricity": lambda _: self.fail("heldout parser accessed"),
            }):
                report = module.validate_and_extract(lock, root, base / "out")
            self.assertEqual(report["status"], "THREE_ORIGIN_TYPED_SELECTIONS_READY_UNADMITTED")
            self.assertEqual(report["outputs"][-1]["status"], "OPAQUE_UNPARSED_SHA_ONLY")


if __name__ == "__main__":
    unittest.main()
