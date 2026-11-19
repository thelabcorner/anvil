"""A83 scalar correctness tests; no codec builds or throughput measurement."""
import hashlib
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from a83_prequential_oracle import CausalBitMixture, CONTEXT_ORDERS, Q, evaluate


def consume(model, data):
    for b in data:
        for shift in range(7, -1, -1):
            model.observe((b >> shift) & 1)
    return model


class A83PrequentialTests(unittest.TestCase):
    def test_initial_predictions_are_exactly_uniform(self):
        m = CausalBitMixture()
        self.assertEqual(m.predict(), [Q // 2] * len(CONTEXT_ORDERS))
        self.assertEqual(m.weights[0] + sum(m.weights[1:]), Q)

    def test_all_first_bit_losses_equal_one(self):
        m = CausalBitMixture()
        m.observe(1)
        self.assertEqual(m.bits, 1)
        for value in m.loss.values():
            self.assertAlmostEqual(value, 1.0, places=9)

    def test_nonzero_probabilities_and_normalized_weights(self):
        m = CausalBitMixture()
        for i in range(257):
            predictions = m.predict()
            self.assertTrue(all(1 <= p < Q for p in predictions))
            self.assertEqual(sum(m.weights), Q)
            self.assertTrue(all(w > 0 for w in m.weights))
            m.observe((i * 73 + 7) % 11 < 4 and 1 or 0)

    def test_prefix_causality_not_future_data(self):
        a = consume(CausalBitMixture(), b"ANVIL")
        b = consume(CausalBitMixture(), b"ANVIL")
        self.assertEqual(a.predict(), b.predict())
        self.assertEqual(a.weights, b.weights)
        self.assertEqual(a.context_counts, b.context_counts)
        _ = consume(b, b"FUTURE BYTES")
        self.assertNotEqual(a.bits, b.bits)

    def test_estimates_deterministic_and_hashed(self):
        result1 = evaluate(b"\x00\xffABC\x80")
        result2 = evaluate(b"\x00\xffABC\x80")
        self.assertEqual(result1, result2)
        self.assertEqual(result1["n_bits"], 48)
        self.assertEqual(result1["sample_sha256"], hashlib.sha256(b"\x00\xffABC\x80").hexdigest())
        self.assertIn("NO_CODESTREAM", result1["purpose"])
        self.assertTrue(all(x > 0 for x in result1["ideal_estimates_bpb"].values()))

    def test_repetitive_predictable_bytes_have_low_ideal_rate(self):
        r = evaluate(b"\x00" * 96)
        self.assertLess(r["ideal_estimates_bpb"]["kt_order_0"], 8)
        self.assertLess(r["ideal_estimates_bpb"]["bayes_mixture"], 8)

    def test_empty_sample_is_not_a_compression_claim(self):
        with self.assertRaises(ValueError):
            evaluate(b"")

    def test_invalid_bit_fails_closed(self):
        with self.assertRaises(ValueError):
            CausalBitMixture().observe(2)

    def test_evaluate_all_orders_and_experimental_mixture(self):
        r = evaluate(b"hello world")
        for k in CONTEXT_ORDERS:
            self.assertIn(f"kt_order_{k}", r["code_length_estimates_bits"])
        self.assertIn("bayes_mixture", r["ideal_estimates_bpb"])
        self.assertEqual(sum(r["final_posterior_integer_weights"]), Q)


if __name__ == "__main__":
    unittest.main()
