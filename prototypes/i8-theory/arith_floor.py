#!/usr/bin/env python3
"""theory/i8 — EXACT ENTROPY FLOOR of synth-arith.bin under the true
generative model, plus a correction to my own model_phrase_rate.py.

SELF-CORRECTION (important, recorded before any number is published):
model_phrase_rate.py framed synth-arith as "AP with iid innovation, so code
the residual against the fitted line (regime 2), NOT the difference".
THAT IS WRONG FOR THIS FILE, and the measurement says so.

Read tests/make_synth_corpus.py:

    v = base
    for i in range(8000):
        v += stride + r.randint(-1, 1)
        f.write(struct.pack('<I', v & 0xFFFFFFFF))

The noise is INSIDE the accumulator. Therefore

    v_i = base + (i+1)*stride + SUM_{j=1}^{i+1} e_j ,  e_j iid U{-1,0,1}

The residual against the fitted line is the CUMULATIVE SUM S_i = sum e_j — a
RANDOM WALK — whose marginal entropy grows without bound (measured 5.7-6.4
b/val). The FIRST DIFFERENCE is

    d_i = v_i - v_{i-1} = stride + e_i

which takes exactly 3 values, each with probability 1/3:
    H(d) = log2(3) = 1.5849625007 bits/value  EXACTLY.

So synth-arith is the canonical case where FINITE DIFFERENCING IS THE CORRECT
transform — not because differencing is generically right, but because the
generative process INTEGRATES its innovations. The general rule:

    Difference  <=>  the process integrates its innovations (random walk).
    Residual-to-fit <=>  the innovations are iid (stationary around a trend).

Both are "arithmetic progression + noise"; they are DIFFERENT processes and
demand OPPOSITE transforms. Getting this backwards costs 4x on this file.

This script computes the exact floor and cross-checks it three ways:
  (a) closed form: n*log2(3) + model bits
  (b) measured empirical entropy of the first differences
  (c) actual arithmetic-coded / Huffman-coded byte count on the 3-symbol stream
"""
import numpy as np, os, math, struct

CORP = 'tests/corpus'
b = np.fromfile(os.path.join(CORP, 'synth-arith.bin'), dtype=np.uint8)
NCOL, NROW = 8, 8000
u = b.view(np.uint32).astype(np.int64)   # little-endian u32, len 64000
assert len(u) == NCOL * NROW, len(u)
u = u.reshape(NCOL, NROW)

print("=" * 100)
print("synth-arith.bin — EXACT ENTROPY FLOOR UNDER THE TRUE GENERATIVE MODEL")
print("=" * 100)
print("generator (tests/make_synth_corpus.py, seed 90002):")
print("   v = base;  for i: v += stride + randint(-1,1);  emit u32 LE v")
print("=> v_i = base + (i+1)*stride + S_i,  S_i = cumsum of iid U{-1,0,1}")
print()

LOG2_3 = math.log2(3.0)
print(f"  (a) CLOSED FORM.  d_i = v_i - v_(i-1) = stride + e_i,  e_i ~ U{{-1,0,1}}")
print(f"      H(d) = log2(3) = {LOG2_3:.10f} bits/value  (exactly, 3 equiprobable symbols)")
print(f"      differences per column: {NROW-1}  (first value must be transmitted raw)")
print(f"      data bits  = {NCOL}*{NROW-1}*log2(3) = {NCOL*(NROW-1)*LOG2_3:,.1f} bits"
      f" = {NCOL*(NROW-1)*LOG2_3/8:,.1f} B")
print(f"      model bits = {NCOL} x (a: 32 bits) + {NCOL} x (stride: log2(97)={math.log2(97):.2f},"
      f" say 7) = {NCOL*(32+7)} bits = {NCOL*(32+7)/8:.1f} B")
floor = NCOL * (NROW - 1) * LOG2_3 / 8 + NCOL * (32 + 7) / 8
print(f"      FLOOR      = {floor:,.1f} B   ratio {floor/len(b):.6f}")
print()

print(f"  (b) MEASURED empirical entropy of the first differences, per column:")
tot = 0.0
for c in range(NCOL):
    d = np.diff(u[c])
    vals, cnt = np.unique(d, return_counts=True)
    p = cnt / cnt.sum()
    H = float(-(p * np.log2(p)).sum())
    tot += H * len(d)
    print(f"      col {c}: alphabet {len(vals)} {list(vals)}  counts {list(cnt)}  "
          f"H = {H:.6f} b/val   (log2 3 = {LOG2_3:.6f}, dev {H-LOG2_3:+.2e})")
print(f"      total over all differences: {tot/8:,.1f} B  (+ model "
      f"{NCOL*(32+7)/8:.1f} B) = {tot/8 + NCOL*(32+7)/8:,.1f} B")
print()

print(f"  (c) ACTUAL CODED SIZE on the 3-symbol alphabet (order-0, the natural code):")
# concatenate all difference streams, map to symbols 0/1/2, arithmetic-code
syms = []
for c in range(NCOL):
    d = np.diff(u[c])
    base = int(np.median(d))
    # symbol = d - stride_hat ; stride_hat could be transmitted, but a decoder
    # can recover it as the mode. Encode d - mode(d) in {-1,0,1}.
    sd = d - base
    syms.append(sd + 1)
syms = np.concatenate(syms)
vals, cnt = np.unique(syms, return_counts=True)
print(f"      symbol distribution: {dict(zip(vals.tolist(), cnt.tolist()))}")
p = cnt / cnt.sum()
Hm = float(-(p * np.log2(p)).sum())
print(f"      H(symbol stream) = {Hm:.6f} b/sym over {len(syms):,} symbols "
      f"= {len(syms)*Hm/8:,.1f} B")
# Huffman block code on 3 symbols: optimal = one symbol 1 bit, two symbols 2 bits
# Actually for 3 equiprobable symbols Huffman gives (1,2,2) = 5/3 = 1.667 b/sym.
# Better: group pairs -> 9 symbols, Huffman ~ 3.17 bits/pair = 1.585 b/sym.
# Arithmetic/rANS achieves ~H + epsilon. Report both.
print(f"      Huffman on single symbols (1,2,2): {len(syms)*5/3/8:,.1f} B")
# pair coding
pairs = syms[:len(syms)//2*2].reshape(-1, 2)
pv = pairs[:, 0] * 3 + pairs[:, 1]
v2, c2 = np.unique(pv, return_counts=True)
p2 = c2 / c2.sum()
H2 = float(-(p2 * np.log2(p2)).sum())
print(f"      rANS/arith on PAIRS (9 symbols): {len(pairs)*H2/8:,.1f} B  "
      f"(H_pair = {H2:.6f} b/pair = {H2/2:.6f} b/sym)")
print(f"      rANS/arith on single symbols:    {len(syms)*Hm/8:,.1f} B  <-- near-exactly H")
coded = len(syms) * Hm / 8 + NCOL * (32 + 7) / 8
print(f"      + model = {coded:,.1f} B    ratio {coded/len(b):.6f}")
print()

print("=" * 100)
print("COMPARISON — the gap that is actually on the table")
print("=" * 100)
rows = [
    ("TRUE ENTROPY FLOOR (this derivation)", floor, 'derived'),
    ("pnra modelled ceiling (strategy msg)", 49585, 'peer-reported'),
    ("brotli-q11 (measured)", 87013, 'measured'),
    ("mode-16 ARI-REF harness wire (measured, EXP. Z)", 89363, 'measured'),
    ("zstd-19 (measured)", 106614, 'measured'),
    ("ANVIL today (measured)", 256022, 'measured'),
]
for nm, v, k in rows:
    print(f"  {nm:<52}{v:>10,} B   ratio {v/len(b):.4f}   [{k}]")
print()
print(f"  brotli-q11 is {87013/floor:.2f}x the floor.  ANVIL today is "
      f"{256022/floor:.1f}x the floor.")
print()
print("  WHY pnra's 49,585 B is ~3.9x the floor (hypothesis, testable):")
print("    64,000 residues at ~6.2 b/val = 49,600 B. That is the cost of coding")
print("    the residual S_i (a RANDOM WALK) at its marginal entropy, instead of")
print("    differencing first. Coding a random walk at its marginal entropy is")
print("    never optimal: the walk's ENTROPY RATE is H(innovation) = log2(3),")
print("    but its per-sample marginal entropy is unbounded. The fix is one")
print("    line: difference BEFORE coding. Test: apply diff then the same")
print("    residual coder; expect ~12.7 KB, not ~49.6 KB.")
print()
print("  CROSSOVER (when does a model phrase stop paying — synth-arith edition):")
for N in (16, 64, 256, 1024, 8000):
    M_over_N = (32 + 7) / N
    print(f"    run length N={N:>5}: model amortises to {M_over_N:7.3f} b/val; "
          f"break-even innovation entropy = 32*h_raw/8 - {M_over_N:.3f} "
          f"(h_raw=0.340 -> {32*0.340/8-M_over_N:6.3f} b/val, "
          f"h_raw=1.0 -> {4.0-M_over_N:6.3f} b/val)")
print(f"    measured innovation entropy here = {LOG2_3:.3f} b/val — pays at every N shown.")
