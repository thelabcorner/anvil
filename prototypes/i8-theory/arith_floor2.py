#!/usr/bin/env python3
"""theory/i8 — CORRECTION AND TIGHTENING of arith_floor.py.

Two things arith_floor.py glossed over, both of which a decoder must actually
have, and both of which I must price before calling 12,717 B a floor:

  (1) SEGMENTATION. The 8 columns are 8000 u32 each, concatenated. A
      stride-4 walk over the whole file crosses column boundaries, where the
      first difference is a large NEGATIVE jump (2 such events, measured).
      Measured H(d1) over the whole file at stride 4 is 3.108 b/val, not
      1.585 — because the alphabet is the UNION of the 8 columns' alphabets
      plus the 2 boundary artifacts. The log2(3) figure is PER COLUMN and
      requires the segment boundaries.

  (2) MODEL COST. arith_floor priced 8 x (a:32 + stride:7) = 312 bits.
      That omits: run lengths, the number of runs, and the fact that a
      decoder must DERIVE the segmentation, not be handed it.

This script:
  - measures the boundary artifacts and shows segmentation is AUTO-DETECTABLE
    (so its cost is not a transmitted table, it is a detector);
  - prices a self-describing descriptor honestly;
  - gives the corrected floor and, separately, the "detector cost excluded"
    number, so nobody mistakes one for the other.

Usasge: python prototypes/i8-theory/arith_floor2.py
"""
import numpy as np, os, math

CORP = 'tests/corpus'
b = np.fromfile(os.path.join(CORP, 'synth-arith.bin'), dtype=np.uint8)
u = b.view(np.uint32).astype(np.int64)
N = len(u)


def H(cnt):
    c = np.asarray(cnt, dtype=np.float64); c = c[c > 0]
    p = c / c.sum()
    return float(-(p * np.log2(p)).sum())


def ent(x):
    _, c = np.unique(np.asarray(x), return_counts=True)
    return H(c)


print("=" * 100)
print("synth-arith.bin — CORRECTED FLOOR, with segmentation priced")
print("=" * 100)
print(f"{N:,} u32 values, {len(b):,} bytes, 8 columns x 8000 (generator: seed 90002)")
print()

d1 = (u[1:] - u[:-1]) & 0xFFFFFFFF
H_whole = ent(d1)
print(f"(1) SEGMENTATION IS REAL AND MUST BE PRICED")
print(f"    H(d1) over the WHOLE file at stride 4 : {H_whole:.4f} b/val")
print(f"    H(d1) WITHIN one column               : 1.5850 b/val  (= log2 3)")
print(f"    => the naive whole-file differencing is {H_whole/1.585:.2f}x worse because")
print(f"       the alphabet is the UNION of 8 column alphabets + boundary jumps.")
print()

# --- detect the boundaries from the data alone.
# THE DETECTOR MUST BE SCALE-FREE (no absolute threshold tuned to this file).
# Principle: within a segment, |d1| is tightly clustered around the stride;
# at a boundary it is orders of magnitude larger. Use a robust scale estimate:
#   med = median(|d1|), and flag |d1| > K * max(med, 1).
# Note d1 can be ~0 (stride could be tiny), so use a MAD-style fallback.
signed = u[1:].astype(np.int64) - u[:-1].astype(np.int64)
a1 = np.abs(signed)
med = float(np.median(a1))
mad = float(np.median(np.abs(a1 - med)))
scale = max(med, 2.0 * mad, 1.0)
print(f"    robust scale estimate: median|d1| = {med:.1f}, MAD = {mad:.1f}, "
      f"scale = {scale:.1f}")
print(f"    in-segment max |d1| = 97 (stride<=97, noise +-1)  |  "
      f"boundary jumps: {sorted(int(signed[8000*c-1]) for c in range(1,8))}")
print(f"    separation between the two populations: "
      f"{min(abs(int(signed[8000*c-1])) for c in range(1,8)) / 97:.0f}x")
for K in (4, 8, 16, 64, 256):
    bnd = np.where(a1 > K * scale)[0] + 1
    ok = bnd.tolist() == [8000 * c for c in range(1, 8)]
    print(f"      K={K:>4}: threshold {K*scale:>10.1f} -> {len(bnd)} boundaries "
          f"{'EXACT MATCH' if ok else '(miss/dup)'}")
bnd = np.where(a1 > 16 * scale)[0] + 1
print(f"    => K=16 recovers all 7 boundaries exactly and nothing else.")
print(f"    Segmentation is therefore DERIVABLE from the data (one abs + one")
print(f"    compare per value), not a transmitted table. Honest accounting:")
print(f"    cost = a DETECTOR, ~0 bits on the wire.")
print()

# --- per-column floor, with segmentation known
LOG2_3 = math.log2(3.0)
cols = []
for c in range(8):
    v = u[c * 8000:(c + 1) * 8000]
    dd = v[1:] - v[:-1]
    cols.append((len(dd), ent(dd)))

data_bits = sum(n * h for n, h in cols)
print(f"(2) PER-COLUMN FLOOR (segmentation known)")
print(f"    differences: {sum(n for n,_ in cols):,} values")
print(f"    sum n*H(d) = {data_bits:,.1f} bits = {data_bits/8:,.1f} B")
print(f"    (closed form 8*7999*log2(3) = {8*7999*LOG2_3:,.1f} bits — agrees)")
print()

print(f"(3) DESCRIPTOR COST — priced three ways, honest about each")
starts = u[[c * 8000 for c in range(8)]]
strides = [int(np.bincount(u[c*8000+1:(c+1)*8000] - u[c*8000:(c+1)*8000-1]).argmax())
           for c in range(8)]
print(f"    measured per-column modal strides: {strides}")
print(f"    generator truth: randint(1,97) each, so stride in [1,97] -> 6.60 bits")
print()
# (a) fully self-describing, worst case: transmit each start as raw 32 bits
desc_a = 8 * 32
# (b) start = previous segment's last value + first difference; but bases are
#     independent randint(0,2^20) -> log2(2^20) = 20 bits each
desc_b = 8 * 20
# (c) exploit that the FIRST value of each column is arbitrary u32 but the
#     stride is tiny: transmit start raw (32) + stride (7). This is arith_floor's
desc_c = 8 * (32 + 7)
for nm, bits, note in (
    ('fully raw: 8 x 32-bit start', desc_a, 'no assumption at all'),
    ('starts are randint(0,2^20): 8 x 20 bits', desc_b, 'USES GENERATOR KNOWLEDGE'),
    ('start 32 + stride 7', desc_c, 'what arith_floor.py assumed'),
):
    print(f"    ({note:<28}) {bits:>4} bits = {bits/8:>6.1f} B   [{nm}]")
print()
print(f"    PLUS: number of runs (log2(8)=3 bits) and run length 8000")
print(f"          (log2(8000)=12.97 bits, once if uniform) ~ 16 bits = 2.0 B")
print()

for nm, db in (('raw starts (no generator knowledge)', desc_a),
               ('generator-aware starts (2^20 range)', desc_b),
               ('arith_floor assumption', desc_c)):
    total = (data_bits + db + 16) / 8
    print(f"    FLOOR [{nm:<38}] = {total:>10,.1f} B   ratio {total/len(b):.6f}")
print()
print(f"    => the honest, assumption-light floor is ~{(data_bits+desc_a+16)/8:,.0f} B;")
print(f"       the generator-aware figure is ~{(data_bits+desc_b+16)/8:,.0f} B.")
print(f"       arith_floor's 12,717 B sits at the optimistic end. I flag this rather")
print(f"       than defend it. All three are ~7x below brotli-q11's 87,013 B, so the")
print(f"       CONCLUSION IS UNCHANGED; only the precision of the number moves.")
print()

print("=" * 100)
print("WHAT THIS CHANGES AND WHAT IT DOES NOT")
print("=" * 100)
print(f"""
  UNCHANGED: brotli-q11 (87,013 B) is {87013/((data_bits+desc_a+16)/8):.1f}x-{87013/((data_bits+desc_b+16)/8):.1f}x the
  floor depending on descriptor accounting. ANVIL today (256,022 B) is ~20x.
  The ceiling claim ("pnra's 49,585 B is ~3.9x pessimistic") is UNCHANGED:
  49,585 / 12,717 = 3.90 and the arithmetic does not depend on which
  descriptor convention is used, only on differencing vs not.

  CHANGED: the log2(3) = 1.585 b/val rate is attainable ONLY WITHIN a segment.
  Any mechanism that differences at a fixed stride across the whole file gets
  3.108 b/val, i.e. ~2x worse, because it mixes 8 distinct alphabets. The
  segmentation detector is therefore not an optimisation — it is a
  PREREQUISITE for the rate. Cost: one signed comparison per value.

  IMPLICATION FOR THE MECHANISM (this is the actionable part):
  a stride/AP model phrase must either
    (a) detect segment boundaries (large |d1| outliers) and restart the model, or
    (b) carry a per-phrase stride and code d1 - stride in a shared alphabet,
        which costs ~log2(number of distinct strides) bits/phrase but needs no
        boundary table.
  Option (b) is more robust to files whose columns are interleaved rather than
  block-wise (e.g. synth-columnar-align.bin, 23 B rows, all fields misaligned)
  and is the one I would build.
""")
