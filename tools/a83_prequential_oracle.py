#!/usr/bin/env python3
"""A83 bounded prequential bit-prediction oracle. NOT a compression codec.

Counts only ideal log-loss of causal integer-valued probability predictors.
NO output bitstream, byte-perfect decoder, model assets, wire overhead, or
performance inference; the output must not be reported as compressed bytes.

Run on frozen, appropriately sourced examples (default <=64 KiB/file).
Never select the model on a heldout file and then call it heldout evidence.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path

Q = 1 << 16
CONTEXT_ORDERS = (0, 1, 2, 4, 8)
MAX_SAMPLE_BYTES = 1 << 18
HALF_COUNT_AT = 1 << 15


class CausalBitMixture:
    """Small fixed-point smoothed KT experts and online Bayesian mixture."""

    def __init__(self) -> None:
        self.context_counts: dict[int, dict[int, list[int]]] = {
            k: {} for k in CONTEXT_ORDERS
        }
        self.history = 0
        self.weights = [Q // len(CONTEXT_ORDERS)] * len(CONTEXT_ORDERS)
        self.weights[0] += Q - sum(self.weights)
        self.loss = {f"kt_order_{k}": 0.0 for k in CONTEXT_ORDERS}
        self.loss["bayes_mixture"] = 0.0
        self.bits = 0

    def predict(self) -> list[int]:
        """Q16 P(1), computed BEFORE the current unknown bit is observed."""
        result = []
        for k in CONTEXT_ORDERS:
            key = self.history & ((1 << k) - 1)
            c0, c1 = self.context_counts[k].get(key, (0, 0))
            denominator = 2 * (c0 + c1 + 1)
            value = (Q * (2 * c1 + 1) + denominator // 2) // denominator
            result.append(min(Q - 1, max(1, value)))
        return result

    def observe(self, bit: int) -> tuple[list[int], int]:
        if bit not in (0, 1):
            raise ValueError("bit must be exactly 0 or 1")
        predictions = self.predict()
        mixed = sum(w * p for w, p in zip(self.weights, predictions))
        mixed = min(Q - 1, max(1, (mixed + Q // 2) // Q))
        for k, p in zip(CONTEXT_ORDERS, predictions):
            correct = p if bit else Q - p
            self.loss[f"kt_order_{k}"] -= math.log2(correct / Q)
        mixture_correct = mixed if bit else Q - mixed
        self.loss["bayes_mixture"] -= math.log2(mixture_correct / Q)

        # Update mixture *AFTER* scoring this bit. Integer-only posterior
        # update; the same calculation can be reproduced from decoded bits.
        numerators = [
            w * (p if bit else Q - p)
            for w, p in zip(self.weights, predictions)
        ]
        denom = sum(numerators)
        if denom <= 0:
            raise AssertionError("a nonzero-probability mixture cannot die")
        updated = [max(1, (x * Q) // denom) for x in numerators]
        difference = Q - sum(updated)
        j = max(range(len(updated)), key=lambda i: updated[i])
        updated[j] += difference
        if min(updated) < 1 or sum(updated) != Q:
            raise AssertionError("posterior normalization invalid")
        self.weights = updated

        for k in CONTEXT_ORDERS:
            key = self.history & ((1 << k) - 1)
            counts = self.context_counts[k].setdefault(key, [0, 0])
            if sum(counts) >= HALF_COUNT_AT:
                counts[0] = (counts[0] + 1) // 2
                counts[1] = (counts[1] + 1) // 2
            counts[bit] += 1
        self.history = ((self.history << 1) | bit) & 255
        self.bits += 1
        return predictions, mixed


def evaluate(data: bytes) -> dict:
    if not data:
        raise ValueError("empty oracle sample; cannot estimate bpb")
    model = CausalBitMixture()
    for b in data:
        for shift in range(7, -1, -1):
            model.observe((b >> shift) & 1)
    if model.bits != 8 * len(data):
        raise AssertionError("bit coverage failed")
    return {
        "schema": 1, "purpose": "IDEAL_PREQUENTIAL_LOGLOSS_ONLY_NO_CODESTREAM",
        "source_bytes_in_sample": len(data),
        "sample_sha256": hashlib.sha256(data).hexdigest(),
        "p_cdf_denominator": Q,
        "n_bits": model.bits,
        "code_length_estimates_bits": {k: round(v, 7) for k, v in model.loss.items()},
        "ideal_estimates_bpb": {k: round(v / len(data), 7) for k, v in model.loss.items()},
        "ideal_estimates_bytes": {k: round(v / 8, 3) for k, v in model.loss.items()},
        "model_prior": "equal five-order experts; posterior from causal Q16 likelihoods",
        "counts_halve_at": HALF_COUNT_AT,
        "final_posterior_integer_weights": model.weights,
        "decoder_state_contexts": {
            str(k): len(model.context_counts[k]) for k in CONTEXT_ORDERS
        },
        "scientific_limit": (
            "The reported lengths are IDEAL LOG-LOSS estimates, NOT archive size; "
            "no entropy-coder termination, framing, model/program bytes, decoder "
            "memory/time or codec correctness has been measured."
        ),
    }


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("files", nargs="+", type=Path)
    p.add_argument("--max-bytes", type=int, default=65536)
    p.add_argument("--offset", type=int, default=0)
    p.add_argument("--out", type=Path)
    args = p.parse_args()
    if not 1 <= args.max_bytes <= MAX_SAMPLE_BYTES:
        p.error(f"--max-bytes must be 1..{MAX_SAMPLE_BYTES}")
    if not 0 <= args.offset:
        p.error("--offset must be nonnegative")
    if not 1 <= len(args.files) <= 10:
        p.error("oracle allows 1..10 source files per invocation")
    output = {"schema": 1, "status": "EXPLORATORY_NO_WIRE_NO_PERF",
              "offset": args.offset, "sample_limit_bytes": args.max_bytes,
              "files": []}
    for path in args.files:
        with path.open("rb") as stream:
            stream.seek(args.offset)
            data = stream.read(args.max_bytes)
        r = evaluate(data)
        r["path"] = str(path)
        r["sample_offset"] = args.offset
        r["note"] = "SOURCE FILENAME AND SHA MUST BE PREREGISTERED SEPARATELY"
        output["files"].append(r)
    s = json.dumps(output, indent=2, sort_keys=True) + "\n"
    print(s, end="")
    if args.out:
        args.out.write_text(s, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
