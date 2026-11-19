"""A90 exact reciprocal mathematical falsifiers. Scalar-only; no codec build."""
import random
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from a90_exact_division_oracle import (
    MAX64, exact_floor_divide, exact_pair, reciprocal64, verify_cases,
)


class A90ExactDivisionTests(unittest.TestCase):
    def test_unit_divisor_special_case(self):
        for x in (0, 1, (1 << 32), MAX64):
            self.assertEqual(exact_floor_divide(x, 1), x)

    def test_zero_denominator_rejected(self):
        with self.assertRaises(ValueError):
            exact_floor_divide(3, 0)

    def test_out_of_range_rejected(self):
        with self.assertRaises(ValueError):
            exact_floor_divide(MAX64 + 1, 2)
        with self.assertRaises(ValueError):
            exact_floor_divide(1, MAX64 + 1)

    def test_one_off_correction_is_needed(self):
        seen = False
        for d in range(3, 50):
            m = reciprocal64(d)
            for x in range(3, 200):
                q0 = (x * m) >> 64
                if q0 < x // d:
                    seen = True
                    self.assertEqual(q0 + 1, exact_floor_divide(x, d))
                    break
            if seen:
                break
        self.assertTrue(seen, "pre-rounding correction must be covered")

    def test_special_denominators_and_extreme_dividends(self):
        for d in (2, 3, 7, 32768, 65535, 65536, MAX64):
            for x in (0, 1, d - 1, d, MAX64 - 1, MAX64):
                self.assertEqual(exact_floor_divide(x, d), x // d)

    def test_shared_arithmetic_denominator_pair(self):
        a, b = exact_pair(1 << 32, 0, 65536, 65536)
        self.assertEqual((a, b), (0, 1 << 32))
        for span, lo, hi, total in (
            (123456789, 3, 7, 13), (1 << 31, 1, 3, 3),
            (1 << 32, 65534, 65535, 65535),
        ):
            self.assertEqual(exact_pair(span, lo, hi, total),
                             (span * lo // total, span * hi // total))

    def test_invalid_arithmetic_cdf_rejected(self):
        for row in ((0, 1, 2, 2), (1, 1, 1, 2), ((1 << 32) + 1, 0, 1, 2)):
            with self.assertRaises(ValueError):
                exact_pair(*row)

    def test_seeded_scalar_oracle_is_deterministic(self):
        a = verify_cases(seed=123, random_per_denominator=2, realistic_states=40)
        b = verify_cases(seed=123, random_per_denominator=2, realistic_states=40)
        self.assertEqual(a, b)
        self.assertEqual(a["exact_division_failures"], 0)
        self.assertFalse(a["byte_identity_proven_in_real_codec"])

    def test_never_labels_as_measured_speedup(self):
        r = verify_cases(seed=456, random_per_denominator=0, realistic_states=1)
        self.assertFalse(r["encoding_or_decoding_speed_measured"])
        self.assertIn("NO_CODEC_OR_PERFORMANCE", r["status"])


if __name__ == "__main__":
    unittest.main()
