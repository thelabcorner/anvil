import math

def order0_bits(counts):
    n = sum(counts.values())
    return -sum(c * math.log2(c / n) for c in counts.values() if c > 0)

# --- Counterexample 1: payload = b"AB" * 512 = 1024 bytes, trivial LZ case
c1 = {0x41: 512, 0x42: 512}
n1 = sum(c1.values())
b1 = order0_bits(c1)
print("P = b'AB'*512")
print("  n =", n1)
print("  H0 total = %.1f bits = %.2f bytes" % (b1, b1 / 8))
print("  H0 per symbol = %.3f bits (uniform over 2 symbols => log2(2)=1.0)" % (b1 / n1))

# --- Counterexample 2: degenerate marginal -> H0 = 0, bound vacuous but SAFE
c2 = {0x00: 512}
print("\nP = 0x00*512")
print("  H0 total = %.1f bits (bound vacuous, never over-estimates here)" % order0_bits(c2))

# --- Counterexample 3: spread marginal + strong higher-order structure.
# A 4-symbol uniform marginal over a 1024-byte payload, but generated so that
# order-k modelling collapses it. Long-range repeat of a 4-byte motif is the
# cheapest legal construction: uniform over {A,B,C,D}, perfectly periodic.
c3 = {0x41: 256, 0x42: 256, 0x43: 256, 0x44: 256}
b3 = order0_bits(c3)
print("\nP = (b'ABCD'*256) -- uniform 4-symbol marginal, perfectly periodic")
print("  H0 total = %.1f bits = %.2f bytes" % (b3, b3 / 8))
print("  LZ description: one literal 'A' + match(dist=4, len=1023)")
print("  Brotli q11 cost of that body is order 10-20 bits, not %.0f bits" % b3)

# --- Defect (ii): ideal code length is an EXPECTATION over the distribution,
# not a per-instance bound. Kraft forces avg >= H, never per-instance >= H.
p = {0x41: 0.5, 0x42: 0.25, 0x43: 0.25}
H = -sum(q * math.log2(q) for q in p.values())
print("\nPer-instance vs expectation, order-0 over p=(1/2,1/4,1/4)")
print("  H = %.3f bits/symbol (expected length, Kraft lower bound on AVERAGE)" % H)
print("  an all-'A' instance realizes 1.000 bits/symbol  -> instance length < H")
print("  => no order-0 ideal-length number lower-bounds EVERY instance length")

# --- Error magnitude vs the decision it gates.
print("\nOverestimate scale vs candidate-spread scale (the decision being pruned)")
for L in (64, 235, 1024, 4096):
    max_over = L * 8 / 8.0  # H0 <= 8 bits/symbol => overestimate <= L bytes
    spread = max(1, L // 200)  # realistic per-record candidate spread
    print("  L=%5d  H0 overestimate <= %5.0f B   vs   spread ~ %d B   ratio >= %5.0fx"
          % (L, max_over, spread, max_over / spread))