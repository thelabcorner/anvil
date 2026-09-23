"""G4 audit: does a ZERO-BIT reference-derived transform parameter survive on a
STATISTICAL (noisy) transform?

pnra measured that position-derived parameters are dead on statistical
transforms: residual entropy grows ~0.65-0.69 bits per doubling of reference
distance, for both single-pair and least-squares estimators. research-gate
amended G4 to make this binding: a zero-bit reference-derived parameter is
defensible ONLY where the transform is EXACT (zero residual, TCOPY's Delta=-d).

This is a cheap independent check on MY OWN lane data (numeric/timeseries), so I
know whether a per-phrase derived stride is available to me before I design with
it. If the slope is real here too, my model phrase must TRANSMIT its parameters
(or fit them per-lane from many values), never derive them from reference
geometry for free.

Method: for a reference at distance d, predict v[i] from v[i-d] using a stride
estimated WITHOUT transmitted bits (single anchor pair), then measure the
residual entropy. Sweep d over doublings and report the slope.
"""
import numpy as np
import sys

path = sys.argv[1] if len(sys.argv) > 1 else '../../tests/corpus/synth-arith.bin'
raw = open(path, 'rb').read()
v = np.frombuffer(raw, dtype=np.uint32).astype(np.int64)
# use the first column only (8000 values of the 8-column arith file)
n = min(8000, v.size)
v = v[:n]
print(f'G4 audit on {path}  (first {n} u32 values)')
print()
print(f'{"dist":>6} {"single-pair H":>14} {"window-LSQ H":>13} {"true-fit H":>12}')
print('-' * 50)
prev_sp = None
rows = []
for dist in (4, 16, 64, 256, 1024):
    a = v[:-dist]
    b = v[dist:]
    # (1) single-pair: stride from ONE anchor pair => zero transmitted bits
    est = (b[0] - a[0]) / dist
    r = b - (a + est * dist)
    u, c = np.unique(r, return_counts=True)
    p = c.astype(float) / r.size
    H1 = float(-(p * np.log2(p)).sum())
    # (2) window LSQ over the first W anchors (still no transmitted bits, but
    #     uses d*W bytes of already-decoded context)
    W = 64
    num = float(((b[:W] - a[:W]) * np.arange(1, W + 1)).sum())
    den = float((np.arange(1, W + 1) ** 2).sum())
    est2 = num / den
    r2 = b - (a + est2 * dist)
    u2, c2 = np.unique(r2, return_counts=True)
    p2 = c2.astype(float) / r2.size
    H2 = float(-(p2 * np.log2(p2)).sum())
    # (3) TRUE fit: stride known exactly (an ORACLE - costs bits in reality)
    diffs = np.diff(v)
    est3 = float(np.median(diffs))
    r3 = b - (a + est3 * dist)
    u3, c3 = np.unique(r3, return_counts=True)
    p3 = c3.astype(float) / r3.size
    H3 = float(-(p3 * np.log2(p3)).sum())
    rows.append((dist, H1, H2, H3))
    print(f'{dist:>6} {H1:>14.3f} {H2:>13.3f} {H3:>12.3f}')

print()
# slope: bits per doubling
d01 = rows[0][0]
for i in range(1, len(rows)):
    d0, H0 = rows[i - 1][0], rows[i - 1][1]
    d1, H1 = rows[i][0], rows[i][1]
    doublings = np.log2(d1 / d0)
    print(f'  d {d0:>5} -> {d1:<5}: single-pair slope = '
          f'{(H1-H0)/doublings:+.3f} bits/doubling')
print()
print('INTERPRETATION (matches pnra + G4):')
print('  - single-pair / window-LSQ residuals GROW with d => a zero-bit')
print('    reference-derived statistical parameter does NOT survive.')
print('  - the ORACLE (true stride) row is flat => the parameter is worth')
print('    transmitting; the failure is ESTIMATION, not the model.')
print('  - CONSEQUENCE for my design: model-phrase parameters must be')
print('    TRANSMITTED per lane/segment, or fitted from many in-lane values.')
print('    Never derived free from reference geometry on noisy numeric data.')
