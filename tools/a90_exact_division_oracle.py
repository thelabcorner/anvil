#!/usr/bin/env python3
"""A90 scalar oracle: EXACT 64-bit reciprocal division, not a compressor.

Portable Python *arithmetic* validation only. It does not compile ANVIL, benchmark
any codec, measure speed, or establish a compressed-wire or Pareto improvement.
"""
from __future__ import annotations

import argparse
import json
import random

MAX64 = (1 << 64) - 1
SPECIAL_DENOMINATORS = (
    32767, 32768, 32769, 65535, 65536, 65537,
    (1 << 32) - 1, 1 << 32, MAX64,
)


def reciprocal64(d: int) -> int:
    if not 1 < d <= MAX64:
        raise ValueError("reciprocal denominator must be in [2, 2^64-1]")
    q, remainder = divmod(MAX64, d)
    return q + int(remainder == d - 1)


def exact_floor_divide(x: int, d: int) -> int:
    """Use a shared denominator reciprocal, 128-bit high multiply and correction.

    A real C++ implementation must use unsigned 128-bit product and check
    architecture/compiler support; Python unlimited ints are ONLY the oracle.
    """
    if not 0 <= x <= MAX64:
        raise ValueError("numerator must fit unsigned 64 bits")
    if not 1 <= d <= MAX64:
        raise ValueError("denominator must fit unsigned 64 bits and be nonzero")
    if d == 1:
        return x
    m = reciprocal64(d)
    q0 = (x * m) >> 64
    # q0 is within one of the true quotient, and q0*d cannot exceed x.
    return q0 + int(x - q0 * d >= d)


def exact_pair(range_: int, cum_lo: int, cum_hi: int, total: int) -> tuple[int, int]:
    """Return the exact existing ANVIL arithmetic interval divisions.

    Both quotients share a single reciprocal; a one-unit error would change
    the bitstream and is unacceptable. Does NOT generate a compressed stream.
    """
    if not 1 <= range_ <= (1 << 32):
        raise ValueError("arithmetic coder range must fit 32-bit interval span")
    if not 0 <= cum_lo < cum_hi <= total <= 65536:
        raise ValueError("invalid test arithmetic CDF")
    return exact_floor_divide(range_ * cum_lo, total), exact_floor_divide(range_ * cum_hi, total)


def verify_cases(seed: int = 41246, random_per_denominator: int = 25,
                 realistic_states: int = 5000) -> dict:
    rng = random.Random(seed)
    checked = 0
    denominators = list(range(1, 2049)) + list(SPECIAL_DENOMINATORS)
    for d in denominators:
        boundary = (0, 1, d - 1, d, d + 1, MAX64 - 1, MAX64, MAX64 // 2)
        numerators = [x & MAX64 for x in boundary]
        numerators += [rng.getrandbits(64) for _ in range(random_per_denominator)]
        for x in numerators:
            expected = x // d
            actual = exact_floor_divide(x, d)
            if expected != actual:
                raise AssertionError(f"EXACT DIVIDE FAILURE x={x} d={d}: {actual} != {expected}")
            checked += 1
    pairs = 0
    for _ in range(realistic_states):
        span = rng.randint(1, 1 << 32)
        total = rng.randint(2, 65536)
        lo = rng.randrange(total)
        hi = rng.randrange(lo + 1, total + 1)
        xlo, xhi = span * lo, span * hi
        a, b = exact_pair(span, lo, hi, total)
        if (a, b) != (xlo // total, xhi // total):
            raise AssertionError("SHARED-DENOMINATOR PAIR FAILURE")
        pairs += 1
    return {
        "status": "SCALAR_IDENTITY_ONLY_NO_CODEC_OR_PERFORMANCE_CLAIM",
        "seed": seed,
        "denominators": len(denominators),
        "individual_division_cases": checked,
        "realistic_cdf_pair_cases": pairs,
        "exact_division_failures": 0,
        "byte_identity_proven_in_real_codec": False,
        "encoding_or_decoding_speed_measured": False,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--seed", type=int, default=41246)
    ap.add_argument("--random-per-denominator", type=int, default=25)
    ap.add_argument("--cdf-pairs", type=int, default=5000)
    args = ap.parse_args()
    if not (0 <= args.random_per_denominator <= 100 and 0 <= args.cdf_pairs <= 50000):
        ap.error("scalar-only bounds violated")
    print(json.dumps(verify_cases(args.seed, args.random_per_denominator, args.cdf_pairs),
                     indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
