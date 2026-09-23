// inv_selector.cpp — CHEAP TRANSFORM SELECTOR for the invariant family.
//
// WHY THIS EXISTS
// ---------------
// theory's finding (deliverable/theory-i8-model-phrase-rate): the invariant
// family has a fork that costs 4x if you get it wrong.
//
//   difference-first  <=>  the process INTEGRATES its innovations (random walk)
//   residual-to-fit   <=>  innovations are iid around a stationary trend
//
// Both look like "arithmetic progression + noise". They demand OPPOSITE
// transforms. theory's instruction: put the discriminator IN the invariant
// selector, not downstream. The test: compare H(second difference) against
// H(residual to fitted line). One pass, cheap, decides the transform.
//
// THIS FILE implements and MEASURES that discriminator, plus the second
// selector axis I found: fixed-record stride (synth-timeseries.bin is 28-byte
// records; the right invariant there is x[p] - x[p-28], not a linear fit).
//
// Three candidate transforms are ranked per region:
//   A  RAW            — no transform
//   B  DIFF-1         — x[i] - x[i-1]            (integrating processes)
//   C  RESID-FIT      — x[i] - (sigma*i + base)  (stationary trend)
//   D  STRIDE-P       — x[i] - x[i-P]            (fixed-record period)
//
// Scored with a REAL coder (brotli) so the ranking is not a hand-rolled
// entropy estimate. Hand-rolling the coder is what made my first prototype
// report compression ~1.000; do not repeat that.
//
// Build (Windows, this host):
//   clang-cl /O2 /EHsc /std:c++20 /MD /DNDEBUG
//     /I ..\..\third_party\install\include /Feinv_selector.exe inv_selector.cpp
//     /link /LIBPATH:..\..\third_party\install\lib
//     brotlienc.lib brotlidec.lib brotlicommon.lib
// Usage: inv_selector <file> [q...]
#include <brotli/encode.h>
#include <brotli/decode.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

using u8 = uint8_t; using u32 = uint32_t; using i64 = int64_t;

static std::vector<u8> rdfile(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) { std::fprintf(stderr, "cannot open %s\n", p.c_str()); std::exit(1); }
    return std::vector<u8>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
static size_t bz(const std::vector<u8>& d, int q) {
    if (d.empty()) return 0;
    size_t cap = BrotliEncoderMaxCompressedSize(d.size()) + 64;
    std::vector<u8> out(cap); size_t n = cap;
    if (!BrotliEncoderCompress(q, BROTLI_DEFAULT_WINDOW, BROTLI_MODE_GENERIC, d.size(), d.data(), &n, out.data()))
        { std::fprintf(stderr, "brotli failed\n"); std::exit(1); }
    std::vector<u8> back(d.size()); size_t bl = d.size();
    if (BrotliDecoderDecompress(n, out.data(), &bl, back.data()) != BROTLI_DECODER_RESULT_SUCCESS
        || bl != d.size() || back != d) { std::fprintf(stderr, "roundtrip failed\n"); std::exit(1); }
    return n;
}
static double Hcount(const std::vector<i64>& v) {
    std::unordered_map<i64, u32> c; c.reserve(v.size() * 2 + 1);
    for (i64 x : v) ++c[x];
    double n = double(v.size()), H = 0;
    for (auto& kv : c) { double p = kv.second / n; H -= p * std::log2(p); }
    return H;
}
static void put32(std::vector<u8>& o, u32 v) {
    o.push_back(u8(v)); o.push_back(u8(v >> 8)); o.push_back(u8(v >> 16)); o.push_back(u8(v >> 24));
}

// Record-period detector: for candidate record size R, score the mean over
// byte offsets of (most-common-byte frequency). Constant fields (offsets 4-7
// and 18-21 in synth-timeseries) pin R exactly and cheaply.
static void detect_period(const std::vector<u8>& d, int& bestR, double& bestScore) {
    bestR = 0; bestScore = -1;
    const size_t N = d.size();
    const size_t probe = std::min<size_t>(N, 65536);   // sample; detector must be cheap
    const int maxR = 256;
    for (int R = 4; R <= maxR; R++) {
        size_t n = probe / R;
        if (n < 16) break;
        double sc = 0;
        for (int off = 0; off < R; off++) {
            std::unordered_map<u8, u32> c;
            for (size_t i = 0; i < n; i++) ++c[d[i * size_t(R) + size_t(off)]];
            u32 top = 0; for (auto& kv : c) top = std::max(top, kv.second);
            sc += double(top) / double(n);
        }
        sc /= R;
        if (sc > bestScore) { bestScore = sc; bestR = R; }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: inv_selector <file> [q...]\n"); return 2; }
    auto raw = rdfile(argv[1]);
    std::vector<int> qs;
    if (argc > 2) for (int i = 2; i < argc; i++) qs.push_back(std::atoi(argv[i]));
    else qs = {9};

    const size_t N = raw.size();
    const size_t n4 = N / 4;
    std::vector<u32> w(n4);
    std::memcpy(w.data(), raw.data(), n4 * 4);

    std::printf("file=%s bytes=%zu words(u32)=%zu\n", argv[1], N, n4);
    auto bestq = [&](const std::vector<u8>& d, size_t& out) {
        out = SIZE_MAX;
        for (int q : qs) out = std::min(out, bz(d, q));
    };

    size_t b_raw = 0; bestq(raw, b_raw);
    std::printf("  A raw          : %10zu  (%.4f)\n", b_raw, double(b_raw) / N);

    // ---- B: first difference ------------------------------------------------
    size_t b_diff = SIZE_MAX;
    {
        std::vector<u8> o; o.reserve(N);
        u32 prev = 0;
        for (size_t i = 0; i < n4; i++) { u32 v = w[i]; put32(o, i ? v - prev : v); prev = v; }
        for (size_t i = n4 * 4; i < N; i++) o.push_back(raw[i]);
        bestq(o, b_diff);
    }
    // ---- C: residual to fitted line ----------------------------------------
    size_t b_fit = SIZE_MAX;
    {
        std::vector<u8> o; o.reserve(N);
        for (size_t i = 0; i < n4; i++) {
            // fit over a trailing local window so sigma can drift (piecewise)
            const size_t W = 64;
            size_t a = (i >= W) ? i - W + 1 : 0;
            i64 sigma = 0;
            if (i > a) sigma = (i64(w[i]) - i64(w[a])) / i64(i - a);
            put32(o, u32(i64(w[i]) - sigma * i64(i)));
        }
        for (size_t i = n4 * 4; i < N; i++) o.push_back(raw[i]);
        bestq(o, b_fit);
    }
    // ---- D: stride-P difference ---------------------------------------------
    int R = 0; double rsc = 0;
    detect_period(raw, R, rsc);
    std::printf("  record-period detector: R=%d mean-offset-constancy=%.4f\n", R, rsc);

    size_t b_stride = SIZE_MAX; int bestP = 0;
    {
        // candidate periods: the detected record size (in words) plus small ints
        std::vector<int> cands;
        if (R >= 4) cands.push_back(std::max(1, R / 4));
        for (int P : {1, 2, 3, 4, 7, 8, 16}) cands.push_back(P);
        std::sort(cands.begin(), cands.end());
        cands.erase(std::unique(cands.begin(), cands.end()), cands.end());
        for (int P : cands) {
            std::vector<u8> o; o.reserve(N);
            for (size_t i = 0; i < n4; i++)
                put32(o, (i >= size_t(P)) ? w[i] - w[i - P] : w[i]);
            for (size_t i = n4 * 4; i < N; i++) o.push_back(raw[i]);
            size_t b = SIZE_MAX; bestq(o, b);
            if (b < b_stride) { b_stride = b; bestP = P; }
        }
    }

    // ---- theory's discriminator, measured ------------------------------------
    std::vector<i64> d1, d2, res;
    d1.reserve(n4); d2.reserve(n4); res.reserve(n4);
    for (size_t i = 1; i < n4; i++) d1.push_back(i64(w[i]) - i64(w[i - 1]));
    for (size_t i = 2; i < n4; i++) d2.push_back(d1[i - 1] - d1[i - 2]);
    for (size_t i = 0; i < n4; i++) {
        const size_t W = 64; size_t a = (i >= W) ? i - W + 1 : 0;
        i64 sigma = (i > a) ? (i64(w[i]) - i64(w[a])) / i64(i - a) : 0;
        res.push_back(i64(w[i]) - sigma * i64(i));
    }
    double h1 = Hcount(d1), h2 = Hcount(d2), hr = Hcount(res);
    std::printf("\n  theory discriminator (bits/val):\n");
    std::printf("    H(diff1)          = %.4f\n", h1);
    std::printf("    H(diff2)          = %.4f\n", h2);
    std::printf("    H(resid-to-fit)   = %.4f\n", hr);
    const char* verdict = (h1 < hr) ? "INTEGRATING -> difference-first"
                                    : "STATIONARY  -> residual-to-fit";
    std::printf("    verdict           : %s\n", verdict);
    std::printf("    (agreement check: does the H-ordering match the brotli ordering?)\n");

    std::printf("\n  brotli-scored ranking (real coder, round-trip verified):\n");
    struct Rk { const char* n; size_t b; };
    std::vector<Rk> rk = {{"A raw", b_raw}, {"B diff-1", b_diff}, {"C resid-fit", b_fit}};
    if (bestP) { static char nm[64]; std::snprintf(nm, sizeof nm, "D stride-P P=%d", bestP); rk.push_back({nm, b_stride}); }
    std::sort(rk.begin(), rk.end(), [](const Rk& a, const Rk& b) { return a.b < b.b; });
    for (size_t i = 0; i < rk.size(); i++)
        std::printf("    %zu. %-20s %10zu  (%.4f)  %+7.2f%% vs raw\n",
                    i + 1, rk[i].n, rk[i].b, double(rk[i].b) / N,
                    100.0 * (double(rk[i].b) - double(b_raw)) / double(b_raw));

    // agreement check
    bool h_says_diff = h1 < hr;
    bool b_says_diff = b_diff < b_fit;
    std::printf("\n  DISCRIMINATOR AGREEMENT: entropy-test says %s ; brotli says %s -> %s\n",
                h_says_diff ? "diff" : "fit", b_says_diff ? "diff" : "fit",
                (h_says_diff == b_says_diff) ? "AGREE" : "*** DISAGREE ***");
    return 0;
}
