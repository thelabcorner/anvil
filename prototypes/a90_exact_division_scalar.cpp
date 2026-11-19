// A90 scalar C++ proof harness; NOT linked into ANVIL.
// Compile and execute only on approved manual-only hosted GitHub Actions.
// The actual codec and wire remain unchanged by this file.
#include <array>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

#if !defined(__SIZEOF_INT128__)
#error "A90 reciprocal arithmetic requires unsigned __int128; never silently fall back"
#endif

using U64 = std::uint64_t;
using U128 = unsigned __int128;
constexpr U64 MAX64 = std::numeric_limits<U64>::max();

struct ExactSharedDivisor {
    U64 d = 0;
    U64 m = 0;
    explicit ExactSharedDivisor(U64 denominator) : d(denominator) {
        if (!d) throw std::invalid_argument("zero denominator");
        if (d == 1) return; // m=2^64 would not fit U64
        const U64 q = MAX64 / d;
        const U64 rem = MAX64 % d;
        m = q + (rem == d - 1 ? 1 : 0);
    }
    U64 quotient(U64 x) const {
        if (d == 1) return x;
        const U64 q0 = static_cast<U64>((static_cast<U128>(x) * m) >> 64);
        const U64 remainder = x - q0 * d; // q0*d <= x: mathematically guaranteed
        return q0 + static_cast<U64>(remainder >= d);
    }
};

static U64 next(U64& s) {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
}
static void verify(U64 x, U64 d, U64& checked) {
    const ExactSharedDivisor rec(d);
    const U64 expected = x / d;
    const U64 actual = rec.quotient(x);
    if (actual != expected) throw std::runtime_error("exact quotient mismatch");
    ++checked;
}
static void verify_pair(U64 range, U64 lo, U64 hi, U64 total, U64& checked) {
    if (!(range >= 1 && range <= (U64{1} << 32) && lo < hi && hi <= total &&
          total >= 1 && total <= 65536)) {
        throw std::invalid_argument("bad ANVIL CDF test interval");
    }
    const U64 xlo = range * lo, xhi = range * hi;
    const ExactSharedDivisor rec(total);
    if (rec.quotient(xlo) != xlo / total || rec.quotient(xhi) != xhi / total)
        throw std::runtime_error("CDF interval mismatch");
    ++checked;
}

int main() {
    try {
        U64 count = 0, pairs = 0, seed = 41246;
        constexpr std::array<U64, 9> special = {
            32767, 32768, 32769, 65535, 65536, 65537,
            (U64{1} << 32) - 1, (U64{1} << 32), MAX64
        };
        auto test_divisor = [&](U64 d) {
            for (U64 x : {U64{0}, U64{1}, d - 1, d, MAX64 - 1, MAX64, MAX64 / 2})
                verify(x, d, count);
            // The random sequence and source are frozen; no timed benchmark.
            for (int i = 0; i < 25; ++i) verify(next(seed), d, count);
        };
        for (U64 d = 1; d <= 2048; ++d) test_divisor(d);
        for (U64 d : special) test_divisor(d);
        for (int i = 0; i < 5000; ++i) {
            const U64 range = next(seed) % (U64{1} << 32) + 1;
            const U64 total = next(seed) % 65535 + 2;
            const U64 lo = next(seed) % total;
            const U64 hi = lo + 1 + (next(seed) % (total - lo));
            verify_pair(range, lo, hi, total, pairs);
        }
        std::cout << "SCALAR_CXX_IDENTITY_ONLY divisors=" << (2048 + special.size())
                  << " quotients=" << count << " cdf_pairs=" << pairs
                  << " failures=0 CODEC_NOT_TESTED PERF_NOT_TESTED\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "A90_FAIL " << error.what() << '\n';
        return 2;
    }
}
