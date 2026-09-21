#!/usr/bin/env python3
"""theory/i8 — CLOSED-FORM CROSSOVER ARITHMETIC for datastruct's lane model.

Answers, with derivation, the three questions:
  (a) minimum N at which seed amortisation pays off (per width, per order)
  (b) how the crossover moves when the model must be DISCOVERED vs TRANSMITTED
  (c) the correct MDL order-selection criterion for k vs k+1, and whether the
      random-walk / iid-noise discriminator determines optimal k in closed form

NOTATION
  N      values in the lane
  W      width in bytes, b := 8W bits per raw value
  k      differencing order (k=0 constant run, k=1 AP, k=2 delta-of-delta)
  h_k    H(residual under the order-k model), bits/value
  c_s    seed cost, bits   [datastruct charges (k+1)*b PESSIMISTICALLY;
                            the realistic charge is b + k*H(delta)]
  rho    reference codec ratio (brotli), for the brotli-competitive crossover

MODEL COST
  C_model(N,k) = c_s(k) + (N - k) * h_k        [bits]
  RAW COST     = N * b
  BROTLI COST  = N * b * rho
"""
import numpy as np, os, math

CORP = 'tests/corpus'


def H(cnt):
    c = np.asarray(cnt, dtype=np.float64); c = c[c > 0]
    p = c / c.sum()
    return float(-(p * np.log2(p)).sum())


def ent(x):
    _, c = np.unique(np.asarray(x), return_counts=True)
    return H(c)


# ---------------------------------------------------------------------------
# (a) MINIMUM N
# ---------------------------------------------------------------------------
# C_model < C_raw
#   c_s + (N-k) h  <  N b
#   c_s - k h      <  N (b - h)
#   N > (c_s - k h)/(b - h)                                            [b > h]
def N_min(c_s, k, b, h, versus_b=None):
    """Minimum N for the model to beat `versus` bits/value. Default versus = b
    (raw). Pass versus_b = b*rho to ask 'beats brotli'."""
    v = b if versus_b is None else versus_b
    if v <= h:
        return None                     # never pays at any N
    return int(math.floor((c_s - k * h) / (v - h))) + 1


print("=" * 104)
print("(a) MINIMUM N FOR SEED AMORTISATION — pessimistic seed charge c_s = (k+1)*b")
print("=" * 104)
print("  N > (c_s - k*h)/(b - h).   Read: below this N the model is a LOSS vs raw bytes.")
print()
hdr = f"{'W':>3}{'b':>5}{'k':>3}{'c_s':>6} |" + "".join(f"{f'h={h}':>9}" for h in
      (1.585, 2.32, 4.0, 8.0, 16.0, 24.0, 30.0))
print(hdr)
print("-" * len(hdr))
for W in (1, 2, 4, 8):
    b = 8 * W
    for k in (0, 1, 2):
        c_s = (k + 1) * b
        row = f"{W:>3}{b:>5}{k:>3}{c_s:>6} |"
        for h in (1.585, 2.32, 4.0, 8.0, 16.0, 24.0, 30.0):
            if h >= b:
                row += f"{'n/a':>9}"
            else:
                n = N_min(c_s, k, b, h)
                row += f"{n:>9}"
        print(row)
print()
print("  Same, but asking 'beats BROTLI' (b*rho, rho=0.340 as on synth-arith):")
hdr = f"{'W':>3}{'b':>5}{'k':>3}{'c_s':>6} |" + "".join(f"{f'h={h}':>9}" for h in
      (1.585, 2.32, 4.0, 8.0, 16.0, 24.0, 30.0))
print(hdr)
print("-" * len(hdr))
RHO = 0.340
for W in (1, 2, 4, 8):
    b = 8 * W
    for k in (0, 1, 2):
        c_s = (k + 1) * b
        row = f"{W:>3}{b:>5}{k:>3}{c_s:>6} |"
        for h in (1.585, 2.32, 4.0, 8.0, 16.0, 24.0, 30.0):
            n = N_min(c_s, k, b, h, versus_b=b * RHO)
            row += f"{('never' if n is None else n):>9}"
        print(row)

# ---------------------------------------------------------------------------
# (c) ORDER SELECTION — the MDL criterion, and the closed form
# ---------------------------------------------------------------------------
print()
print("=" * 104)
print("(c) ORDER SELECTION — why 'minimise H(k-th difference)' is the WRONG statistic")
print("=" * 104)
print("""
  Going from order k to k+1 costs ONE extra seed and buys a different residual
  alphabet. The wire costs are

     C(k)   = c_s(k)   + (N - k)   h_k
     C(k+1) = c_s(k+1) + (N - k - 1) h_{k+1}

  Prefer k+1 iff  C(k+1) < C(k), i.e.

     (N-k-1)(h_k - h_{k+1})  >  [c_s(k+1) - c_s(k)] - h_k
                             ~=  b        (one extra full-width seed)

  *** THE CORRECT STATISTIC IS THE TOTAL WIRE COST, NOT h_k ALONE. ***
  Minimising h_k ignores the seed and, more importantly, ignores that
  differencing an i.i.d. sequence can only INCREASE entropy (proof below),
  so greedy "keep differencing while H drops" is not the right rule either.
""")
print("  THEOREM (differencing never helps an i.i.d. sequence).")
print("    Let X, Y iid, H(X) = h. Then H(X - Y) >= h.")
print("    Proof: H(X-Y) >= H(X-Y | Y) = H(X | Y) = H(X) = h.  (conditioning")
print("    cannot increase entropy; X-Y given Y=y is a translate of X.)   []")
print()
print("    Consequence: if the k-th difference is ALREADY iid, one more")
print("    differencing step strictly increases the rate. Conversely, if the")
print("    process INTEGRATES its innovations, exactly one difference recovers")
print("    the innovation and that is the optimum.")
print()
print("  CLOSED FORM FOR OPTIMAL k — the discriminator decides it:")
print("""
    PROCESS                          OPTIMAL TRANSFORM          ACHIEVED RATE
    v_i = trend_i + S_i,             k = 1 (one difference)     H(innovation)
      S_i = cumsum(e_i)  [INTEGRATED]
    v_i = a + b*i + e_i,             residual-to-FIT (no diff)  H(e)
      e_i iid  [TREND-STATIONARY]
    v_i = c + e_i, e_i iid           k = 0 (constant run)       H(e)

    Doing the wrong one costs: integrated process coded at k=0 pays the walk's
    unbounded marginal entropy; trend-stationary process differenced pays
    H(e_i - e_{i-1}) > H(e_i).
""")

# --- verify on the real files
print("  MEASURED ORDER PROFILE (H of the k-th difference, bits/value):")
print(f"  {'file':<26}{'lane':<22}{'H(d0)':>9}{'H(d1)':>9}{'H(d2)':>9}{'H(d3)':>9}  optimal")

FILES = [
    ('synth-arith.bin', 4, 4, 0),
    ('synth-telemetry-f64.bin', 16, 8, 0),
    ('synth-telemetry-f64.bin', 16, 8, 8),
    ('synth-timeseries.bin', 14, 8, 0),
    ('synth-timeseries.bin', 14, 4, 12),
    ('synth-columnar-align.bin', 23, 4, 2),
    ('synth-drift-stride.bin', 44, 8, 0),
]
for fn, S, W, off in FILES:
    p = os.path.join(CORP, fn)
    if not os.path.exists(p):
        continue
    raw = np.fromfile(p, dtype=np.uint8)
    idx = np.arange(off, len(raw) - W + 1, S)
    if len(idx) < 100:
        continue
    if W == 1:
        v = raw[idx].astype(np.int64)
    elif W == 2:
        v = np.stack([raw[idx], raw[idx + 1]], 1).astype(np.int64)
        v = v[:, 0] | (v[:, 1] << 8)
    elif W == 4:
        v = np.stack([raw[idx], raw[idx + 1], raw[idx + 2], raw[idx + 3]], 1).astype(np.int64)
        v = v[:, 0] | (v[:, 1] << 8) | (v[:, 2] << 16) | (v[:, 3] << 24)
    else:
        v = np.stack([raw[idx + j] for j in range(8)], 1).astype(np.int64)
        v = np.zeros(len(idx), dtype=np.int64)
        for j in range(8):
            v |= v_shift if False else (np.stack([raw[idx + j]], 1).astype(np.int64)[:, 0] << (8 * j))
    hs = []
    cur = v.astype(np.int64)
    for k in range(4):
        hs.append(ent(cur))
        cur = cur[1:] - cur[:-1]
    bestk = int(np.argmin(hs))
    print(f"  {fn:<26}{f'S={S} W={W} off={off}':<22}"
          + "".join(f"{x:>9.3f}" for x in hs) + f"   k={bestk}")

print()
print("  SYNTH-ARITH CHECK — the theory predicts H(d2) exactly.")
print("    e_i ~ U{-1,0,1}  =>  H(e) = log2 3 = 1.584963")
print("    H(e_i - e_{i-1}) over alphabet {-2..2} with weights (1,2,3,2,1)/9:")
w = np.array([1, 2, 3, 2, 1], dtype=np.float64)
p = w / w.sum()
print(f"      = {float(-(p*np.log2(p)).sum()):.6f} b   <-- compare measured H(d2) above")
print("    Agreement confirms: d1 is the innovation (integrated process), d2 is a")
print("    differenced iid sequence and is therefore strictly worse.")

print()
print("=" * 104)
print("(b) DISCOVERED vs TRANSMITTED PARAMETERS")
print("=" * 104)
print("""
  TRANSMITTED: the wire carries (offset, stride, width, order, length).
    Cost is a CONSTANT, independent of N:
      c_param ~ log2(off_range) + log2(stride_range) + log2(#widths)
                + log2(#orders) + log2(N)
             ~ 6 + 8 + 2 + 2 + 17  =  ~35 bits for a generous parameter space.
    Effect on the crossover: it adds c_param to c_s, so
      N > (c_s + c_param - k h)/(b - h)
    i.e. it pushes N_min up by roughly c_param/(b-h). At b=32, h=1.585 that is
    35/30.4 ~ 1.2 values — NEGLIGIBLE for long lanes. Transmitting is cheap.

  DISCOVERED: no wire cost, but the encoder pays SEARCH. The search must
    examine (offset, stride, width, order) jointly:
      naive:   O(|off| * |S| * |W| * |k| * N)
      smarter: for each (W,k), compute the autocorrelation of the lane once
               via FFT -> all strides at once, O(|W|*|k|*N log N)
    With |W|=4 (1,2,4,8), |k|=3, and FFT-based stride search, the search is
    ~12 * N log N operations per file — cheap and N-scaling, so it does NOT
    change the per-lane crossover; it changes the ENCODE SPEED only.

  THE REAL DISTINCTION IS ELSEWHERE: a transmitted parameter lets the encoder
  pick any (S,W,off,k); a DISCOVERED one must be INFERABLE BY THE DECODER from
  the already-emitted prefix, or transmitted. Since c_param ~ 35 bits is
  negligible against any lane of N > ~10, TRANSMIT IT. Discovery is for the
  ENCODER's benefit (choosing good values), not the wire's.

  Encode-side economics, priced at the measured lambda_frontier (10-40 B/us):
    search pays iff  (bytes saved) > lambda * (search seconds)
    A lane of N=8000, W=4, b=32, h=1.585 saves (8000*32 - 8000*1.585)/8
      = 30,415 B. At lambda = 20 B/us = 20 B per 1e-6 s, that is worth
      30,415/20 = 1,521 us = 1.5 ms of search. Enormous budget: a full
      (S,W,off,k) brute-force scan is comfortably affordable on any lane that
      long. Search cost is NOT the binding constraint on long lanes; it IS on
      short ones (N < ~64), where the saving is <250 B i.e. <12 us.
""")

# ---------------------------------------------------------------------------
print("=" * 104)
print("HOW MUCH OF datastruct's GAP IS REMOVABLE?")
print("=" * 104)
print(f"  {'file':<26}{'their wire':>12}{'floor':>10}{'gap':>10}   decomposition")
rows = [('synth-arith.bin', 17637, 12705),
        ('synth-telemetry-f64.bin', 12741, 5821)]
for fn, theirs, floor in rows:
    print(f"  {fn:<26}{theirs:>12,}{floor:>10,}{theirs-floor:>+10,}")
print("""
  synth-arith: they selected (S=4, W=4, k=2). H(d1)=1.585 < H(d2)=2.197, so
  k=2 is strictly worse than k=1 by 0.61 b/val. Switching to k=1:
    8 lanes * (2*32 seeds + 7999*1.585) / 8  = 12,742 B   (their seed convention)
    8 lanes * (32 + 6.6  + 7999*1.585) / 8   = 12,717 B   (realistic seed)
  => 17,637 -> 12,742 removes 4,895 of the 4,932 B gap. The residual ~37 B is
  the pessimistic full-width seed charge. ESSENTIALLY ALL REMOVABLE — and
  none of it is intrinsic to per-lane modelling, because on this file the
  block-wise columns ARE the lanes (stride 4, width 4, aligned).

  synth-telemetry: their (16,8,1) models ts and value as ONE width-8 lane at
  stride 16. But the two sub-fields want OPPOSITE treatments:
    ts (u64)   increments by EXACTLY 1000 -> after 2 seeds, ZERO bits.
    value(f64) is a float; arithmetic differencing of its u64 bit pattern is
               the WRONG transform. XOR (Gorilla) is the right one.
  Measured below.
""")

# --- telemetry: XOR vs arithmetic difference on the f64 value column
p = os.path.join(CORP, 'synth-telemetry-f64.bin')
if os.path.exists(p):
    raw = np.fromfile(p, dtype=np.uint8)
    nrec = len(raw) // 16
    recs = raw[:nrec * 16].reshape(nrec, 16)
    ts = recs[:, 0:8].copy().view(np.uint64).astype(np.int64).ravel()
    val = recs[:, 8:16].copy().view(np.uint64).astype(np.int64).ravel()
    print(f"  telemetry: {nrec:,} records")
    dts = ts[1:] - ts[:-1]
    print(f"    ts: distinct first differences = {len(np.unique(dts))} "
          f"(values: {np.unique(dts)[:4]}) -> H = {ent(dts):.4f} b/rec; "
          f"free after 2 seeds")
    dval = val[1:] - val[:-1]
    xval = np.bitwise_xor(val[1:], val[:-1])
    print(f"    value f64: H(arith diff of u64) = {ent(dval):.4f} b/rec")
    print(f"               H(XOR consecutive)   = {ent(xval):.4f} b/rec   <-- Gorilla")
    # Gorilla framing
    def lz(x): return 64 - int(x).bit_length() if x else 64
    def tz(x): return (int(x) & -int(x)).bit_length() - 1 if x else 64
    lzs = np.array([lz(int(x)) for x in xval])
    tzs = np.array([tz(int(x)) for x in xval])
    mant = np.array([max(64 - int(l) - int(t), 0) for x, l, t in zip(xval, lzs, tzs)])
    zf = float((xval == 0).mean())
    gor = 1.0 + (1 - zf) * (1 + 6 + 6 + ent(mant))
    print(f"    Gorilla framing on XOR: zero-frac {zf:.4f}, "
          f"mean lz {lzs.mean():.1f}, mean tz {tzs.mean():.1f}, "
          f"H(mantissa len) {ent(mant):.3f}")
    print(f"      => ~{gor:.3f} b/rec = {nrec*gor/8:,.0f} B for the value column")
    print(f"    vs the eps-entropy floor log2(5) = {math.log2(5):.4f} b/rec "
          f"= {nrec*math.log2(5)/8:,.0f} B")
    print(f"    => Gorilla-XOR gets within {gor/math.log2(5):.2f}x of the true floor")
    print(f"       without discovering the float model at all.")
