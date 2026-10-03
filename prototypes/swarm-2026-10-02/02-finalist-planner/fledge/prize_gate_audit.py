#!/usr/bin/env python3
"""
Track 02 critic -- arithmetic audit of the published FLEDGE gate/prize ratios.

Reads NO corpus, touches NO binary, runs NO benchmark, performs NO fuzzing.
It recomputes, from constants that are quoted verbatim in
docs/I10-GROTLI-G4-RESULTS.md, docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md,
docs/I10-FRONTIER-RECON-2026-09-24.md and RESEARCH_LEDGER.md, the ratios cited
in docs/swarm-2026-10-02/02-finalist-planner-fledge.md sections 4.3 and 5.1.

Every constant below carries its source. Run:  python prize_gate_audit.py
"""

# ---------------------------------------------------------------- constants --
# [M] G3 D1-D4 routed portfolio total, complete bytes.
#     docs/I10-FRONTIER-RECON-2026-09-24.md:180
C_REF_AGG = 1_384_654

# [M] G5A arm totals, charged complete bytes, D1-D4.
#     RESEARCH_LEDGER.md:4756-4757 ; docs/I10-G5A-LOCALITY-CONTROLS-PREREG.md:20
A1 = 1_470_205
A3 = 1_380_245
A3_NET_SAVING = 89_960          # A1 - A3
COLUMN_COMPONENT = 72_138      # F4, column ordering share of the net
SHAPE_GROUPING = 17_822        # F4, shape grouping share of the net

# [M] G4 Q2 pairwise separability probe.
#     docs/I10-GROTLI-G4-RESULTS.md:101-102
MAX_PAIR_GAIN_PCT = 0.277632
AGG_PAIR_GAIN_PCT = 0.057144

# [M] Q2 geometry and denominator.
#     docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md:1106-1107, 1156-1170
#     pair_gain_pct denominator is C_indep_MIXED (complete carrier bytes);
#     max_pairs = C(4,2) = 6 per file; C_indep_MIXED(f) is REUSED across the
#     6 pairs of file f, so the aggregate denominator is inflated 6x.
PAIRS_PER_FILE = 6
DISCOVERY_FILES = 4

# [M] Frozen fidelity gates inherited from G4.
#     docs/I10-GROTLI-G4-RESULTS.md:58-65 ; pivot prereg 16.1
GATE_AGG = 0.0025
GATE_FILE = 0.0050
LADDER_GATE_AGG = 0.0010
LADDER_GATE_FILE = 0.0025

# [M] Discovery corpus source bytes. Pivot prereg 3, table at :177-180.
CORPUS = {
    "D1": 277_673,
    "D2": 615_350,
    "D3": 10_485_760,
    "D4": 3_585_053,
}

# [M] Class A grid aggregate encode rates, MB/s, harmonic input-byte aggregation.
#     RESEARCH_LEDGER.md:4862-4865
T_Q1_MBS = 432.106
T_Q11_MBS = 0.681
# [M] NOTE: no Class A encode rate is recorded for q4 or q6 anywhere in the
#     ledger. Every q4/q6 cost term below is therefore an INTERPOLATION across
#     a 634x range measured at only its two endpoints.

# [M] Frozen pivot call budget. Pivot prereg 14.1 / 14.3.
PIVOT_Q4_SAMPLES = 80
PIVOT_SAMPLE_CAP_BYTES = 8192
# [M] REGION_RAW is byte-identical across P0/P1/O11 (pivot prereg 15 gate 4), so
#     the eight logical finalists collapse to at most SEVEN distinct carriers.
LOGICAL_FINALISTS = 8
DISTINCT_CARRIERS_MAX = 7


def rule(title):
    print()
    print("=" * 74)
    print(title)
    print("=" * 74)


def main():
    rule("1. Q2 DENOMINATOR UN-AMBIGUATION  ->  prize ceiling of the selector")
    denom = PAIRS_PER_FILE * C_REF_AGG
    total_pair_prize = (AGG_PAIR_GAIN_PCT / 100.0) * denom
    ceiling_pct = 100.0 * total_pair_prize / C_REF_AGG
    print(f"aggregate-pair denominator (inflated {PAIRS_PER_FILE}x) : {denom:>12,} B")
    print(f"total prize over the {PAIRS_PER_FILE * DISCOVERY_FILES} sampled pairs   : "
          f"{total_pair_prize:>12,.0f} B")
    print(f"as a share of the aggregate carrier              : {ceiling_pct:>12.4f} %")
    print("(upper bound: exact pair optimisation is a superset of per-slot")
    print(" independent selection, so no selector can win more than this)")

    rule("2. GATE SCALABILITY  ->  can the gates discriminate a planner?")
    r_agg = 100.0 * GATE_AGG / ceiling_pct
    r_file = 100.0 * GATE_FILE / MAX_PAIR_GAIN_PCT
    r_ladd = 100.0 * LADDER_GATE_AGG / ceiling_pct
    r_laddf = 100.0 * LADDER_GATE_FILE / MAX_PAIR_GAIN_PCT
    print(f"aggregate gate  {GATE_AGG * 100:>6.3f} %  /  ceiling {ceiling_pct:.4f} %"
          f"  = {r_agg:>6.1f} % of the ENTIRE prize")
    print(f"per-file gate   {GATE_FILE * 100:>6.3f} %  /  max pair gain "
          f"{MAX_PAIR_GAIN_PCT:.6f} %  = {r_file:.3f}x")
    print(f"ladder agg gate {LADDER_GATE_AGG * 100:>6.3f} %  /  ceiling {ceiling_pct:.4f} %"
          f"  = {r_ladd:>6.1f} % of the ENTIRE prize")
    print(f"ladder file gate{LADDER_GATE_FILE * 100:>6.3f} %  /  max pair gain "
          f"{MAX_PAIR_GAIN_PCT:.6f} %  = {r_laddf:.3f}x")
    print()
    print(f"expected regret of an UNINFORMATIVE but non-anti-correlated selector")
    print(f"  ~ half the prize = {0.5 * ceiling_pct:.4f} %"
          f"  -> {'BELOW' if 0.5 * ceiling_pct < GATE_AGG * 100 else 'ABOVE'}"
          f" the {GATE_AGG * 100:.3f} % aggregate gate")

    rule("3. QUANTIZATION FLOOR  (stratum S0 objects, 1..256 B)")
    gate_bytes = GATE_AGG * C_REF_AGG
    print(f"aggregate gate expressed in bytes           : {gate_bytes:>12,.0f} B")
    print(f"tolerable S0 candidates @ 1.0 B/object     : {int(gate_bytes):>12,} ")
    print(f"tolerable S0 candidates @ 0.5 B/object     : {int(2 * gate_bytes):>12,}")
    print("n_candidates_by_stratum[f] is NOT reported by the frozen contract ->")
    print("this unmeasured count can decide feasibility by itself")

    rule("4. PRIZE COMPARISON  ->  selector axis vs representation axis")
    ratio = A3_NET_SAVING / total_pair_prize
    print(f"G5A A3 - A1 net ordering saving             : {A3_NET_SAVING:>12,} B "
          f"({100 * A3_NET_SAVING / A1:.4f} % of A1)")
    print(f"  of which column ordering                  : {COLUMN_COMPONENT:>12,} B "
          f"({100 * COLUMN_COMPONENT / A1:.4f} % of A1)")
    print(f"  of which shape grouping                  : {SHAPE_GROUPING:>12,} B")
    print(f"Q2-capped selector prize ceiling           : {total_pair_prize:>12,.0f} B")
    print(f"RATIO representation prize / selector prize : {ratio:>12.2f}x")

    rule("5. BACKEND-CALL ARITHMETIC")
    print(f"pivot sampled q4 backend INPUT volume      : "
          f"{PIVOT_Q4_SAMPLES * PIVOT_SAMPLE_CAP_BYTES:>12,} B  (<= cap)")
    print(f"logical finalists                          : {LOGICAL_FINALISTS:>12}")
    print(f"distinct carriers (REGION_RAW collapses)   : {DISTINCT_CARRIERS_MAX:>12}")
    print("-> the frozen table's 'at most eight q1 calls' is UNREACHABLE")

    rule("6. INCREMENTAL ENCODE COST vs RAW BROTLI q11  (D3, Class-A anchored)")
    d3 = CORPUS["D3"]
    raw_q11_ms = 1000.0 * d3 / (T_Q11_MBS * 1e6)
    q1_7_ms = 1000.0 * DISTINCT_CARRIERS_MAX * d3 / (T_Q1_MBS * 1e6)
    q11_2_ms = 2.0 * raw_q11_ms
    g2_ms = raw_q11_ms
    incr = q11_2_ms + g2_ms
    print(f"D3 source bytes                            : {d3:>12,} B")
    print(f"raw Brotli q11 encode (Class A anchor)     : {raw_q11_ms:>12,.0f} ms")
    print(f"R1: 7 distinct carriers at q1               : {q1_7_ms:>12,.0f} ms")
    print(f"R4: 2 finalists at q11                     : {q11_2_ms:>12,.0f} ms")
    print(f"C_prod G2 q11 baseline                     : {g2_ms:>12,.0f} ms")
    print(f"INCREMENTAL over 'ship raw q11'            : {incr:>12,.0f} ms"
          f"  = {incr / raw_q11_ms:.2f}x raw q11")
    print("(q4 and q6 ladder terms are OMITTED: no Class A rate exists for them,")
    print(" so this is an OPTIMISTIC lower bound on the incremental cost)")

    rule("7. STAGED-ORDER COST COMPARISON  (information gain per backend call)")
    stage1_calls = (DISTINCT_CARRIERS_MAX + 4 + 2 + 2) * DISCOVERY_FILES
    # per-file raw-q11 cost in MILLISECONDS, summed over the four files
    raw_q11_sum_ms = sum(1000.0 * CORPUS[f] / (T_Q11_MBS * 1e6) for f in CORPUS)
    stage1_s = raw_q11_sum_ms * (2 + 1) / 1000.0       # 2 finalist q11 + 1 raw q11
    oracle_l_calls = 32 * DISCOVERY_FILES
    oracle_l_s = 32.0 * raw_q11_sum_ms / 1000.0         # 32 q11 whole-carrier per file
    print(f"Stage 0 (zero-backend-call audit)          : {0:>12} calls"
          f"   {0:>10.1f} s  backend")
    print(f"Stage 1 (ladder rank stability, my R0.3)   : {stage1_calls:>12} calls"
          f"  {stage1_s:>10.1f} s  backend  [lower bound, q1/q4/q6 omitted]")
    print(f"LCPS-1 ORACLE-L control ALONE              : {oracle_l_calls:>12} calls"
          f"  {oracle_l_s:>10.1f} s  backend  [lower bound]")
    print()
    print(f"LCPS-1 ORACLE-L / (Stage 0 + Stage 1)      : "
          f"{oracle_l_calls / max(stage1_calls, 1):>11.1f}x calls, "
          f"{oracle_l_s / max(stage1_s, 1):.0f}x backend seconds")
    print("plus LCPS-1 must additionally rebuild C0's O11 isolated-q11")
    print("population, whose cardinality is UNMEASURED (pivot prereg 14.3).")

    rule("8. THE 634x ANCHOR BOTH TRACKS' ECONOMICS DEPEND ON")
    ratio_anchor = T_Q1_MBS / T_Q11_MBS
    print(f"Class A q1/q11 encode-rate ratio           : {ratio_anchor:>12.1f}x")
    print("[M] This is measured at EXACTLY TWO quality points (q1 and q11).")
    print("[M] No Class A q4 or q6 encode rate exists in the ledger.")
    print("[A] Cross-check against per-file Linux figures for a 827 KB JSON")
    print("    (docs/CONTEXT.md shape-frontier table): q4 ~86.6 MB/s, q9 ~22 MB/s")
    print("    -> the q9/q4 ratio there is ~0.25x, not ~634x. The anchor is")
    print("    CORPUS-DEPENDENT by orders of magnitude, and both tracks'")
    print("    'q1 is ~634x cheaper than q11' economics inherit that instability.")
    print()
    print("=" * 74)
    print("No threshold in this file is new. Every gate reproduced above is")
    print("frozen elsewhere; this script only recomputes ratios of measured")
    print("quantities so a reviewer can audit them without running anything.")
    print("=" * 74)


if __name__ == "__main__":
    main()