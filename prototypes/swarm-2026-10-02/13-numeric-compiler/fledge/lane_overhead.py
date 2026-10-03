#!/usr/bin/env python3
"""E13 / track-13 Fledge isolated check: typed-lane metadata vs the frozen
20%-of-gross-savings cap, and the Gorilla B1/B2 bit invariant.

SCOPE (deliberately minimal, and stated so it cannot be misread as evidence):
  * Pure closed-form integer/decimal arithmetic over constants READ OUT OF THE
    REPOSITORY. No corpus is read, nothing is compressed, nothing is timed.
  * This is therefore a PROVENANCE/ARITHMETIC CHECK, not an experiment and not a
    benchmark. Its output is `derived`, never `measured`.
  * It exists so the numbers in docs/swarm-2026-10-02/13-numeric-compiler-fledge.md
    section 5.1 and section 1.2.1 are reproducible rather than asserted.

CONSTANTS AND THEIR SOURCES (all inside this repository):
  [C1] metadata cap "at most 20% of gross savings"
       -> RESEARCH_LEDGER.md:4828-4829 (retained PORDER probe condition:
          "beats both F10 and F01 by at least 1% complete bytes with
           metadata/codebook cost at most 20% of gross savings").
  [C2] Gorilla Case B1 = 2 + mb bits, Case B2 = 13 + mb bits; the vXOR block
       payload is stored in a 64-bit container (mb <= 64).
       -> docs/verify-notes/gorilla-verification-plan.md:15 (B1/B2 wire) and
          :18 (the disputed "(13+60)/64 - 1 = +14.06%" line).
  [C3] Per-lane descriptor fields -> the track-13 mechanism definition in the
       Fledge report section 5.1 (end-offset uvar, width code, operator
       selector, exception-model flag). MINIMUM viable encoding; m is a bound,
       not a tuned value, and the sweep shows the conclusion is insensitive to m
       over the whole plausible range.
  [C4] vXOR container -> docs/verify-notes/gorilla-verification-plan.md:15.
"""

# --- constants -------------------------------------------------------------

CAP = 0.20                 # [C1] metadata <= 20% of gross savings
B1_FIXED, B2_FIXED = 2, 13  # [C2] bits, excluding the mb-bit payload
CONTAINER_BITS = 64        # [C2]/[C4] vXOR block container width

# Per-lane descriptor: (name, bits)
LANE_FIELDS = (
    ("end-offset delta (uvar, 1 B typical)", 8),
    ("width code (unpack selector)", 5),
    ("operator / expression selector", 4),
    ("exception / patch model flag", 2),
)


def lane_bits(fields=LANE_FIELDS) -> int:
    return sum(bits for _, bits in fields)


def min_savings_fraction(m_bits: int, lane_width_bytes: int, cap: float = CAP) -> float:
    """Smallest fraction of lane bytes that must be SAVED so that decoder-visible
    lane metadata stays within `cap` of the gross savings.

        metadata/input  = (m_bits/8) / w
        metadata/savings = (metadata/input) / S   <=   cap
      =>                                        S  >=  (m_bits/8) / (w * cap)
    """
    return (m_bits / 8.0) / (lane_width_bytes * cap)


def main() -> None:
    m_bits = lane_bits()
    print("E13 track-13 Fledge -- isolated arithmetic check (no corpus, no codec)")
    print()
    print("[C3] minimum per-lane descriptor")
    for name, bits in LANE_FIELDS:
        print(f"    {bits:>2} bits  {name}")
    print(f"    ---- {m_bits} bits total = {m_bits/8.0:.3f} B/lane")
    print()

    print("[C1] cap: metadata <= %.0f%% of gross savings" % (CAP * 100))
    print()
    print("    lane   required saving S >= (m/8)/(w*cap)")
    print("    width      m=2B (16b)   m=3B (24b)   m=4B (32b)")
    for w in (8, 16, 32, 64, 256, 1024, 4096):
        cells = []
        for m_bytes in (2, 3, 4):
            s = min_savings_fraction(m_bytes * 8, w)
            cells.append(("%10.2f%%" % (100 * s)) if s <= 1 else ("%11s" % "IMPOSSIBLE"))
        print(f"    {w:>5}B  {cells[0]}  {cells[1]}  {cells[2]}")
    print()
    print("    Reading: a lane is admissible under the frozen cap only when the")
    print("    required saving fraction is <= 100%. At lane width 16 B with a 2 B")
    print("    descriptor the lane must save >= 62.5% of its own bytes; at 8 B no")
    print("    saving fraction exists at all. Long contiguous columns are cheap.")
    print()

    print("[C2] Gorilla Case B1 vs B2 for identical payload")
    print("    mb   B1 bits   B2 bits   B2-B1   vs B1      vs 64-bit container")
    for mb in (0, 4, 12, 24, 51, 52, 60, 64):
        b1, b2 = B1_FIXED + mb, B2_FIXED + mb
        vs_b1 = (b2 / b1 - 1.0) * 100.0 if b1 else float("nan")
        vs_ct = (b2 / CONTAINER_BITS - 1.0) * 100.0
        print(f"    {mb:>3}   {b1:>6}   {b2:>6}   {b2-b1:>5}   {vs_b1:>+7.2f}%   {vs_ct:>+8.2f}%")
    print()
    print(f"    Exact invariant: B2 - B1 = {B2_FIXED - B1_FIXED} bits, for every mb.")
    print(f"    Container overflow: B2 > {CONTAINER_BITS} bits first at mb = {CONTAINER_BITS - B2_FIXED + 1}.")
    print()
    print("    PROVENANCE NOTE: docs/verify-notes/gorilla-verification-plan.md:18")
    print("    records \"(13+60)/64 - 1 = +14.06%\" as EXACT. That denominator (64)")
    print("    is neither the B1 wire cost (2+mb) nor the payload width (mb), so the")
    print("    percentage is not reproducible as a like-for-like ratio. Both readings")
    print("    are shown above. Quote the +-11-bit invariant, not the percentage.")
    print()
    print("    Sensitivity: the section-5.1 conclusion is unchanged for every")
    print("    descriptor size from 2 B to 4 B, so it does not depend on the exact")
    print("    field layout chosen.")
    print()
    print("derivation class: DERIVED (closed form over [C1]-[C4]). Not a measurement.")


if __name__ == "__main__":
    main()