#!/usr/bin/env python3
"""theory/i8 — the DUAL of the iso-crossing curve.

Primal:  given decode throughput t, what byte budget L*(t) still crosses?
Dual:    given a candidate byte count L, what throughput t_crit(L) is needed?

    t_crit(L) = min { t : L < N * R_ref(t) }
              = min { q.mbps : q in refs, L < N * q.ratio }   * (1 + margin)

The second line is the whole point and it's almost tautological, which is why
it is trustworthy: to cross at size L you must be FASTER than every reference
row that is at least as small as you are. So:

    t_crit(L) = (1 + eps) * max { q.mbps : q.ratio <= L/N }

with tie handling: if some q has q.ratio <= L/N we must STRICTLY beat its
throughput. If no reference is as small as L, t_crit = 0 (you already win on
ratio alone, at any speed).

This is the number an implementer needs: "land X bytes, decode faster than Y
MB/s, and the arbiter must print EXTENDS_FRONT."

A SEPARATE, IMPORTANT DISTINCTION (see deliverable/theory-i8-iso-crossing):
  - GENUINE CROSSING: the binding reference q is beaten on ratio (L/N < q.ratio)
    AND q is faster. Neither dominates. Not inside any gap rectangle.
  - FRONT-GAP FILL: non-dominated only because two non-dominating reference
    rows bracket an empty rectangle.
We report which, per candidate.

Also computes the DECODE-SPEEDUP CEILING from the measured t3 floor profile:
  decode time shares: crc32 0.44, masks+resid materialization 0.26,
  concat/alloc 0.15, token loop 0.11 (of which entropy pulls 0.003).
  Eliminating fraction phi of decode time yields speedup 1/(1-phi).
Under three scenarios (conservative / middle / perfect) we compute the max
attainable speedup and compare it to the multiple each file needs.

Usage: python prototypes/i8-theory/critical_throughput.py
"""
import csv
from collections import defaultdict

CSV = 'tests/benchmark-suite.csv'
REF = ('brotli-', 'zstd-')
MARGIN = 0.02   # never claim a crossing on an exact throughput tie

# ---- measured t3 decode-floor profile (docs/CONTEXT.md, median-7 interleaved)
FLOOR = dict(crc32=0.44, materialize=0.26, concat=0.15, tokenloop=0.11)
ENTROPY_SHARE_OF_TOKENLOOP = 0.003 / 0.11   # opcode entropy pulls = 0.3% of total

SCENARIOS = {
    'conservative (CRC only)': dict(crc32=1.00, materialize=0.0, concat=0.0, tokenloop=0.0),
    'middle (CRC + lazy mat + half concat)': dict(crc32=1.00, materialize=0.50,
                                                  concat=0.50, tokenloop=0.0),
    'perfect (eliminate all three)': dict(crc32=1.00, materialize=1.00,
                                          concat=1.00, tokenloop=0.0),
}


def load():
    out = []
    with open(CSV, newline='') as f:
        for r in csv.DictReader(f):
            out.append(dict(
                file=r['file'].replace('tests\\corpus\\', '').replace('tests/corpus/', ''),
                n=int(r['input_bytes']), codec=r['codec'], b=int(r['compressed_bytes']),
                ratio=float(r['ratio']), enc=float(r['encode_MBps']),
                dec=float(r['decode_MBps'])))
    return out


def t_crit(refs, n, L, key='dec', margin=MARGIN):
    """Minimum throughput to cross at size L. Returns (t, binding_ref, kind)."""
    r = L / n
    as_small = [q for q in refs if q['b'] <= L]        # refs at least as small
    if not as_small:
        return 0.0, None, 'RATIO-WIN (no reference this small; crosses at any speed)'
    q = max(as_small, key=lambda x: x[key])            # fastest of them
    return q[key] * (1.0 + margin), q, 'GENUINE-CROSSING'


def speedup_ceiling(scen):
    phi = sum(FLOOR[k] * f for k, f in scen.items())
    return phi, 1.0 / (1.0 - phi)


def main():
    rows = load()
    byfile = defaultdict(list)
    for x in rows:
        byfile[x['file']].append(x)

    print("=" * 112)
    print("CRITICAL THROUGHPUT  t_crit(L) = the decode speed a candidate of L bytes MUST exceed to cross")
    print("=" * 112)

    # --- named candidates (measured artifacts from the ledger/brief)
    CANDIDATES = [
        ('synth-arith.bin', 89363, 'mode-16 ARI-REF validated harness wire (EXP. Z, u1)'),
        ('synth-timeseries.bin', 120680, 'brotli-q6 parity (120,669 B) - Gorilla/vXOR target'),
        ('synth-arith.bin', 87013, 'brotli-q11 parity (87,013 B)'),
        ('generated.json', 79172, 'brotli-q11 parity (79,172 B)'),
        ('generated.log', 81946, 'brotli-q11 parity (81,946 B)'),
        ('generated.sqlite', 185702, 'brotli-q11 parity (185,702 B)'),
        ('generated.jsonl', 150619, 'brotli-q11 parity (150,619 B)'),
        ('synth-jitter.bin', 62717, 'brotli-q11 parity (62,717 B)'),
    ]
    print(f"\n{'candidate':<28}{'file':<22}{'L (B)':>10}{'ratio':>8}"
          f"{'t_crit (MB/s)':>15}{'binding ref':>16}  kind")
    print("-" * 112)
    for fn, L, note in CANDIDATES:
        rs = byfile.get(fn)
        if not rs:
            print(f"{note:<28}{fn:<22}{L:>10}  -- file not in CSV --")
            continue
        n = rs[0]['n']
        refs = [r for r in rs if r['codec'].startswith(REF)]
        t, q, kind = t_crit(refs, n, L)
        print(f"{note[:27]:<28}{fn:<22}{L:>10,}{L/n:>8.4f}{t:>15.1f}"
              f"{(q['codec'] if q else '-'):>16}  {kind}")

    # --- what the CURRENT best ANVIL row needs
    print()
    print("=" * 112)
    print("CURRENT BEST ANVIL ROW: how far from a crossing, on both axes")
    print("=" * 112)
    print(f"{'file':<24}{'best ANVIL B':>13}{'r':>8}{'dec':>8} | "
          f"{'t_crit @ that size':>18}{'multiple':>10} | {'bytes to q11':>14}")
    for fn in sorted(byfile, key=lambda x: -byfile[x][0]['n']):
        rs = byfile[fn]
        n = rs[0]['n']
        refs = [r for r in rs if r['codec'].startswith(REF)]
        anvil = [r for r in rs if not r['codec'].startswith(REF)]
        a = min(anvil, key=lambda r: r['b'])
        t, q, kind = t_crit(refs, n, a['b'])
        mult = t / a['dec'] if t > 0 else float('inf')
        q11 = next((r for r in refs if r['codec'] == 'brotli-q11'), None)
        print(f"{fn:<24}{a['b']:>13,}{a['b']/n:>8.4f}{a['dec']:>8.1f} | "
              f"{t:>18.1f}{mult:>9.2f}x | "
              f"{(('{:+,}'.format(a['b'] - q11['b'])) if q11 else '-'):>14}")

    # --- decode-speedup ceiling vs required multiple
    print()
    print("=" * 112)
    print("DECODE-SPEEDUP CEILING vs REQUIRED MULTIPLE  (is the speed route even open?)")
    print("=" * 112)
    for name, scen in SCENARIOS.items():
        phi, sp = speedup_ceiling(scen)
        print(f"  scenario {name:<44} removes {phi:5.1%} of decode time "
              f"-> ceiling {sp:5.2f}x")
    print()
    mid_phi, MID = speedup_ceiling(SCENARIOS['middle (CRC + lazy mat + half concat)'])
    perf_phi, PERF = speedup_ceiling(SCENARIOS['perfect (eliminate all three)'])
    print(f"{'file':<24}{'required multiple':>18}{'vs MID ceiling':>16}"
          f"{'vs PERFECT ceiling':>21}  verdict")
    print("-" * 112)
    for fn in sorted(byfile, key=lambda x: -byfile[x][0]['n']):
        rs = byfile[fn]
        n = rs[0]['n']
        refs = [r for r in rs if r['codec'].startswith(REF)]
        anvil = [r for r in rs if not r['codec'].startswith(REF)]
        a = min(anvil, key=lambda r: r['b'])
        t, q, kind = t_crit(refs, n, a['b'])
        req = t / a['dec'] if t > 0 else float('inf')
        if req == float('inf'):
            v = 'SPEED ROUTE CLOSED (no finite multiple)'
        elif req <= MID:
            v = 'SPEED ROUTE OPEN (middle scenario suffices)'
        elif req <= PERF:
            v = 'MARGINAL (needs the perfect-decoder ceiling)'
        else:
            v = 'SPEED ROUTE CLOSED (beyond even a perfect decoder)'
        rs_ = f"{req:.2f}x" if req != float('inf') else 'inf'
        print(f"{fn:<24}{rs_:>18}{'ok' if req <= MID else 'NO':>16}"
              f"{'ok' if req <= PERF else 'NO':>21}  {v}")

    print()
    print("NOTE on the ceiling: the CRC leg is wire-invisible (byte-identical output).")
    print("The materialization and concat legs are NOT free -- lazy materialization")
    print("defers work, it does not delete it. The 'middle' scenario assumes half of")
    print("each is genuinely eliminable; that is an ASSUMPTION, stated as such.")
    print(f"Entropy-codec selection lives inside the token loop ({FLOOR['tokenloop']:.0%} of")
    print(f"decode), of which the entropy pulls themselves are 0.3% => the maximum")
    print(f"speedup obtainable by per-stream codec choice is ~{1/(1-0.003):.3f}x. That is")
    print("why S6-1 measured zero decode effect from budget selection: it is not a")
    print("tuning failure, it is the size of the target.")


if __name__ == '__main__':
    main()
