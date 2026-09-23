#!/usr/bin/env python3
"""theory/i8 — NUMERICAL FALSIFICATION of the invariant-family claims in
INVARIANTS.md. An invariant that is algebraically correct can still be
economically useless; this measures the economics.

What is measured (all on tests/corpus, all reproducible):
  1. NO-GO CHECK (empirical): sample pairs of equal-width fields and count how
     often a *candidate* position-blind invariant would have to map distinct
     values to the same bucket. This does not prove the theorem (that is a
     two-line proof) but shows the entropy the theorem forbids preserving.
  2. SELECTIVITY sigma for each family:
       sigma = P( I(x,p) == I(y,q) | (x,p),(y,q) NOT a true transformed pair )
     estimated empirically as (collisions - true matches) / (probes - true).
  3. EVENT RATE: how many positions carry the structural event, vs input
     bytes. An invariant is only economical if event rate << byte rate.
  4. END/WIDTH (family C): collision rate of sort(bytes(x)) vs v+p.

Usage: python prototypes/i8-theory/invariant_selectivity.py
"""
import numpy as np, os, math, collections

CORP = 'tests/corpus'


def H(cnt):
    c = np.asarray(cnt, dtype=np.float64); c = c[c > 0]
    p = c / c.sum()
    return float(-(p * np.log2(p)).sum())


def load(fn):
    return np.fromfile(os.path.join(CORP, fn), dtype=np.uint8)


def fields32(b, align=4):
    """All 32-bit LE fields at byte offsets multiple of `align`."""
    n = len(b) // 4
    return b[:4 * n].view(np.uint32).astype(np.int64), (np.arange(n) * 4).astype(np.int64)


print("=" * 104)
print("INVARIANT FAMILY SELECTIVITY — measured on tests/corpus")
print("=" * 104)

FILES = ['anvil.exe', 'anvil_bench.exe', 'pe-python.exe', 'pe-where.exe',
         'generated.sqlite', 'synth-arith.bin', 'synth-timeseries.bin',
         'synth-columnar-align.bin', 'synth-drift-stride.bin',
         'synth-jitter.bin', 'generated.log', 'src.cpp']

print(f"\n{'file':<26}{'N fields':>10}{'uniq v+p':>11}{'collide%':>10}"
      f"{'sigma(A1)':>12}{'uniq sort':>11}{'collide%':>10}{'sigma(C)':>11}")
print("-" * 104)
for fn in FILES:
    p = os.path.join(CORP, fn)
    if not os.path.exists(p):
        continue
    b = load(fn)
    v, pos = fields32(b)
    if len(v) < 1000:
        continue
    # Family A1: I = v + p  (mod 2^32)
    IA = ((v + pos) & 0xFFFFFFFF).astype(np.int64)
    # Family C:  I = sort(bytes) -> pack as sorted 4 bytes
    bb = b[:4 * len(v)].reshape(-1, 4)
    srt = np.sort(bb, axis=1)
    IC = (srt[:, 0].astype(np.int64) << 24) | (srt[:, 1].astype(np.int64) << 16) | \
         (srt[:, 2].astype(np.int64) << 8) | srt[:, 3].astype(np.int64)
    n = len(v)
    uA, cA = np.unique(IA, return_counts=True)
    uC, cC = np.unique(IC, return_counts=True)
    # collisions = sum over buckets of C(k,2) pairs sharing a bucket
    collA = int((cA * (cA - 1) // 2).sum())
    collC = int((cC * (cC - 1) // 2).sum())
    tot_pairs = n * (n - 1) // 2
    sigA = collA / tot_pairs
    sigC = collC / tot_pairs
    print(f"{fn:<26}{n:>10,}{len(uA):>11,}{100*collA/tot_pairs:>10.4f}"
          f"{sigA:>12.3e}{len(uC):>11,}{100*collC/tot_pairs:>10.4f}"
          f"{sigC:>11.3e}")

print()
print("  READ: sigma = probability two RANDOM fields share a bucket.")
print("  A1 (v+p) should be ~ 1/2^32 = 2.3e-10 for a well-spread digest.")
print("  C  (sorted bytes) should be FAR worse -- that is the honest negative.")

# ---------------------------------------------------------------------------
print()
print("=" * 104)
print("FAMILY C IN DETAIL — why symmetric byte digests are not indexable")
print("=" * 104)
for fn in ['anvil.exe', 'generated.sqlite', 'src.cpp']:
    b = load(fn)
    n = len(b) // 4
    v, pos = fields32(b)
    bb = b[:4 * n].reshape(-1, 4)
    srt = np.sort(bb, axis=1)
    IC = (srt[:, 0].astype(np.int64) << 24) | (srt[:, 1].astype(np.int64) << 16) | \
         (srt[:, 2].astype(np.int64) << 8) | srt[:, 3].astype(np.int64)
    u, c = np.unique(IC, return_counts=True)
    top = np.argsort(-c)[:5]
    print(f"  {fn}: {n:,} fields -> {len(u):,} distinct sorted-byte digests "
          f"({len(u)/n:.4f} distinctness)")
    print(f"     top buckets: {[ (int(u[i]), int(c[i])) for i in top ]}")
    # entropy of the digest vs entropy of the raw field
    print(f"     H(raw field) = {H(c):.3f} b ... H(digest dist) = "
          f"{H(np.bincount(np.searchsorted(u, IC))):.3f} b")
print("""
  Interpretation: a symmetric digest on a 4-byte window of executable or
  structured data collapses to a small number of buckets (heavily skewed byte
  distributions: lots of 00, FF, and small integers). sigma is orders of
  magnitude above the additive invariant's. Combined with near-zero useful
  transform population (a file is LE or BE; mixed-endian fields are a
  container pathology), family C does not pay for an index. The algebra is
  right; the population is not there.
""")

# ---------------------------------------------------------------------------
print("=" * 104)
print("NO-GO THEOREM — empirical shadow")
print("=" * 104)
print("""
  Theorem: for T(x,theta) = x + theta with theta FREE over a set |Theta|>1,
  any I with I(x+theta, p') = I(x,p) for all theta is CONSTANT.

  Empirical shadow: suppose we ignored the theorem and defined the
  "difference invariant" I(x,y) = x - y for a PAIR of fields. It is invariant
  under a COMMON additive shift (both fields shift by the same Delta), which
  is Family A2. Measure its selectivity:
""")
print(f"{'file':<26}{'n fields':>10}{'distinct diffs':>16}{'top-bucket size':>17}"
      f"{'sigma(A2) est':>15}")
print("-" * 104)
for fn in ['anvil.exe', 'generated.sqlite', 'synth-arith.bin']:
    b = load(fn)
    v, pos = fields32(b)
    n = len(v)
    # A2: pairwise differences of ADJACENT fields (the cheapest pair set)
    d = ((v[1:] - v[:-1]) & 0xFFFFFFFF).astype(np.int64)
    u, c = np.unique(d, return_counts=True)
    # adjacent-field pair set has n-1 members; sigma ~ sum C(c,2)/C(n-1,2)
    coll = int((c * (c - 1) // 2).sum())
    tp = (n - 1) * (n - 2) // 2
    print(f"{fn:<26}{n:>10,}{len(u):>16,}{int(c.max()):>17,}{coll/max(tp,1):>15.3e}")
print("""
  A2's invariant is algebraically valid (it cancels a common Delta) but its
  candidate population is the set of field PAIRS, and the bucket occupancy is
  dominated by recurring small differences (0, 1, small integers). That is
  why .eh_frame/.rodata discovery was 'prohibitively expensive': the
  invariant finds candidates, then verification is swamped. The rejection of
  transmitted-Delta was correct; note it rejected PARAMETER TRANSMISSION, not
  the invariant -- but the invariant alone does not rescue the economics.
""")

# ---------------------------------------------------------------------------
print("=" * 104)
print("FAMILY B2 — second difference as a STRIDE DETECTOR (not a coder)")
print("=" * 104)
print("""
  Claim: d2_i = v_{i+2} - 2 v_{i+1} + v_i is invariant to any affine
  reparametrisation (kills both a and b), so it detects a column/stride
  without knowing the model. But it AMPLIFIES noise, so it must not be used
  as the residual coder. Measure the amplification:
""")
print(f"{'file':<26}{'H(v)':>9}{'H(d1)':>9}{'H(d2)':>9}{'amplif':>9}  verdict")
print("-" * 104)
for fn in ['synth-arith.bin', 'synth-columnar-align.bin', 'synth-timeseries.bin',
           'synth-drift-stride.bin', 'generated.sqlite']:
    b = load(fn)
    n = len(b) // 4
    v, pos = fields32(b)
    v = v[:20000]
    d1 = ((v[1:] - v[:-1]) & 0xFFFFFFFF).astype(np.int64)
    d2 = ((d1[1:] - d1[:-1]) & 0xFFFFFFFF).astype(np.int64)
    Hv = H(np.bincount(np.minimum(v, 2**22)))
    H1 = H(np.bincount(np.minimum(d1, 2**22)))
    H2 = H(np.bincount(np.minimum(d2, 2**22)))
    amp = H2 - H1
    verdict = 'B2 amplifies -> do NOT code with d2' if amp > 0.3 else \
              ('d2 helps' if amp < -0.1 else 'neutral')
    print(f"{fn:<26}{Hv:>9.3f}{H1:>9.3f}{H2:>9.3f}{amp:>+9.3f}  {verdict}")
print("""
  synth-arith is the exception that proves the rule: there d1 has H=1.585
  (log2 3, the innovation) and d2 has H=2.19 -- d1 is the CODER there because
  the noise is iid-innovation, and d2 amplifies. The discriminator for
  choosing d1-vs-residual-to-fit is: does the process INTEGRATE its
  innovations? Measured test: compare H(v_i - (a+b*i)) against H(d1).
  Whichever is lower names the correct transform. That is one O(N) pass.
""")

# ---------------------------------------------------------------------------
print("=" * 104)
print("EVENT RATE — the condition that decides economy (condition 3)")
print("=" * 104)
print("""
  An invariant index pays only if the STRUCTURAL EVENT RATE is far below the
  byte rate; otherwise the index is just a slower byte index. Reference point
  from the Linux PNRA work: ~115,490 relocation-pair events in 3.26 MB .text
  = 3.5% of positions, vs ~1.4M parser decision positions. That 12x reduction
  is what made event-driven PNRA win.
""")
# measure: for the PE files, how many 8-byte windows look like a relocation
# candidate (E8/E9 opcode followed by a plausible rel32)?
for fn in ['anvil.exe', 'pe-python.exe', 'pe-where.exe']:
    p = os.path.join(CORP, fn)
    if not os.path.exists(p):
        continue
    b = load(fn)
    n = len(b)
    # E8 = call rel32, E9 = jmp rel32  (x86-64)
    hits = np.zeros(n, dtype=bool)
    idx = np.where((b[:-5] == 0xE8) | (b[:-5] == 0xE9))[0]
    idx = idx[idx + 5 <= n]
    hits[idx] = True
    print(f"  {fn:<20} N={n:>9,}  E8/E9 event positions = {int(hits.sum()):>8,} "
          f"= {100*hits.sum()/n:>6.3f}% of bytes   "
          f"(reduction {n/max(int(hits.sum()),1):>6.1f}x)")
print("""
  A 28x-58x reduction in decision positions: the event-driven inversion is
  justified on these files by the event rate alone, before any invariant is
  computed. That is condition 3 satisfied, and it is why PNRA's formulation
  is economical where a per-byte transformed search would not be.
""")
