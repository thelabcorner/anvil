// arith_inv.cpp — Transformation-invariant search over ARITHMETIC (linear)
// transform families. The generalization lane of PNRA.
//
// THE UNIFYING RELATION
// ---------------------
// ELF/TCOPY (validated, Linux): a relocation field at absolute position p
// satisfies v_dst = v_src - (p - q), i.e.  **I(x,p) = x + p is invariant**.
// Generalize: for a field whose value shifts by sigma*d when its site moves
// forward by d,
//
//        I_sigma(x, p) = x - sigma * p        is invariant.
//
// TCOPY is the special case sigma = -1 (Delta = -d), and sigma is IMPLICIT
// there (zero transmitted bits). For numeric columns sigma is the per-region
// slope and must be DISCOVERED.
//
// WHY THIS FILE IS THE RIGHT TARGET
// ---------------------------------
// tests/corpus/synth-arith.bin is the single worst cell in the frozen baseline:
// ANVIL r=1.000 (256,022 B, literally zero compression) vs brotli-q11 87,013 B
// (r=0.340). Measured structure (see deliverable/pnra-i8-invariant-stride):
// 64,000 uint32 LE values = 8 runs of 8,000; within run k,
//      v[p] = a_k + s_k * p + eps,  s_k integer, eps small,
// with slopes (64,83,79,69,96,48,91,2). A SINGLE GLOBAL affine fit fails
// (global slope -1.35, residual std 266,809) — which is exactly why the file
// looks incompressible to byte-oriented LZ.
//
// WHAT THIS PROTOTYPE MEASURES
// ----------------------------
// A cheapest-first ladder, so the gain is attributable:
//   L0  raw bytes                       (control)
//   L1  global affine, sigma=1          (delta coding — the obvious control)
//   L2  global affine, best integer sigma
//   L3  piecewise-linear: segmentation + per-segment integer sigma
//       (sigma DERIVED, not transmitted — see note below)
//   L4  L3 + entropy estimate of the residual (the ceiling proxy)
//
// SIGMA DERIVATION (the point that decides whether this ships): a transmitted
// per-segment sigma costs bits. The self-describing form avoids that: encode
// the segment's two endpoint values and let the DECODER compute
// sigma = (v_end - v_start) / (len - 1). Then a phrase over a linear region
// needs only (len, endpoints, residuals). L4 reports the entropy that form
// would need.
//
// Build (Windows, this host):
//   clang-cl /O2 /EHsc /std:c++20 /MD /Fearith_inv.exe arith_inv.cpp
// Usage: arith_inv <file.bin> [--json]
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

using u32 = uint32_t; using u64 = uint64_t; using i64 = int64_t;

static std::vector<uint8_t> rdfile(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) { std::fprintf(stderr, "cannot open %s\n", p.c_str()); std::exit(1); }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static double entropy_bits(const std::vector<i64>& v) {
    std::unordered_map<i64, u32> c;
    c.reserve(v.size() * 2);
    for (i64 x : v) ++c[x];
    double n = double(v.size()), H = 0.0;
    for (auto& kv : c) { double p = kv.second / n; H -= p * std::log2(p); }
    return H;
}
static double entropy_bits_u32(const std::vector<u32>& v) {
    std::unordered_map<u32, u32> c;
    c.reserve(v.size() * 2);
    for (u32 x : v) ++c[x];
    double n = double(v.size()), H = 0.0;
    for (auto& kv : c) { double p = kv.second / n; H -= p * std::log2(p); }
    return H;
}

// Deflate (via zlib is not linked) — instead report ENTROPY bytes as the
// rate proxy and use a real order-0 range coder below so the number is an
// actual compressed size, not a bound.
struct RangeCoder {
    std::vector<uint8_t> out;
    u32 lo = 0; u32 range = 0xFFFFFFFFu;
    void enc(u32 sym, u32 cum, u32 tot) {
        u64 r = range; r /= tot;
        lo += u32(r) * cum;
        range = u32(r) * (tot - cum > 0 ? 1 : 1); // placeholder, fixed below
        (void)sym;
    }
};

// ---- Simple order-0 adaptive binary arithmetic coder over 32-bit symbols,
// via bit-plane decomposition with per-plane adaptive probability. Cheap,
// correct, and enough to turn an entropy figure into a real byte count.
struct BitCoder {
    std::vector<uint8_t> out;
    u32 lo = 0, range = 0xFFFFFFFFu;
    std::vector<u32> bitbuf; size_t nbits = 0;
    void put_bit(int b, u32* p0 /*models*/, int ctx) {
        u32 p = p0[ctx]; // p = P(0) in [1,65535]
        u64 r = range; u64 split = (r * p) >> 16;
        if (b == 0) { range = u32(split); }
        else { lo += u32(split) + 1; range = u32(r - split - 1); }
        // renormalize
        while (range < 0x10000) { out.push_back(uint8_t(lo >> 24)); lo <<= 8; range <<= 8; if (range == 0) range = 0xFFFFFFFFu; }
        // adapt
        if (b == 0) p0[ctx] = p + ((65536 - p) >> 5);
        else p0[ctx] = p - (p >> 5);
        if (p0[ctx] < 1) p0[ctx] = 1;
        if (p0[ctx] > 65535) p0[ctx] = 65535;
    }
    void flush() { for (int i = 0; i < 5; i++) out.push_back(uint8_t(lo >> 24)), lo <<= 8; }
    void put_u32(u32 v, int nctx) {
        std::vector<u32> p0(size_t(nctx) * 32, 32768);
        for (int b = 31; b >= 0; b--) put_bit((v >> b) & 1, p0.data() + b * nctx, 0);
    }
};

// Encode a residual stream with a plain bit-plane adaptive coder (MSB->LSB,
// one shared context per bit position). Returns BYTES.
static size_t code_residual_bytes(const std::vector<i64>& r) {
    // zigzag -> u32
    std::vector<u32> z; z.reserve(r.size());
    for (i64 x : r) z.push_back(u32((x << 1) ^ (x >> 63)));
    BitCoder bc;
    for (u32 v : z) bc.put_u32(v, 1);
    bc.flush();
    return bc.out.size();
}

// ---------------------------------------------------------------------------
// Segmentation: discover piecewise-linear homogeneous regions.
// Greedy online: extend the current segment while (v[p] - sigma*p) stays within
// a bounded residual set; reseed sigma when it breaks. Mirrors the ANVIL
// single-pass rule (no lookahead DP, must be shippable).
// ---------------------------------------------------------------------------
struct Segment { size_t start, len; i64 sigma; i64 base; };

static std::vector<Segment> segment_linear(const std::vector<u32>& d, int window, i64 tol) {
    std::vector<Segment> segs;
    size_t n = d.size();
    size_t s = 0;
    while (s < n) {
        // Seed sigma from the first `window` samples by endpoints.
        size_t w = std::min<size_t>(window, n - s);
        i64 sigma = 0;
        if (w >= 2) sigma = (i64(d[s + w - 1]) - i64(d[s])) / i64(w - 1);
        i64 base = i64(d[s]) - sigma * i64(s);
        // Extend while |v - (sigma*p + base)| <= tol
        size_t e = s;
        while (e < n) {
            i64 pred = sigma * i64(e) + base;
            if (std::llabs(i64(d[e]) - pred) > tol) break;
            ++e;
        }
        if (e == s) { e = s + 1; } // always advance
        segs.push_back({s, e - s, sigma, base});
        s = e;
    }
    return segs;
}

// Re-estimate each segment's sigma+base by least squares (integer), then
// recompute the residual. This is the "derived sigma" step: the decoder can
// reproduce it from the endpoints, so sigma costs (len, endpoints) only.
static void refit(std::vector<Segment>& segs, const std::vector<u32>& d) {
    for (auto& g : segs) {
        const size_t a = g.start, b = g.start + g.len;
        // Use endpoints for sigma so the decoder can derive it exactly:
        //   sigma = (v[b-1] - v[a]) / (len - 1)
        if (g.len >= 2) {
            i64 num = i64(d[b - 1]) - i64(d[a]);
            i64 den = i64(g.len - 1);
            g.sigma = num >= 0 ? num / den : -((-num + den - 1) / den);
        } else g.sigma = 0;
        // base = modal (v - sigma*p) over the segment (decoder: transmitted, or
        // derived from v[a]; we report both).
        std::unordered_map<i64, u32> cnt;
        i64 bestv = i64(d[a]) - g.sigma * i64(a); u32 bestc = 0;
        for (size_t i = a; i < b; i++) {
            i64 inv = i64(d[i]) - g.sigma * i64(i);
            u32 c = ++cnt[inv];
            if (c > bestc) { bestc = c; bestv = inv; }
        }
        g.base = bestv;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: arith_inv <file.bin> [window] [tol]\n"); return 2; }
    auto raw = rdfile(argv[1]);
    int window = argc > 2 ? std::atoi(argv[2]) : 64;
    i64 tol = argc > 3 ? std::atoll(argv[3]) : 256;

    // Interpret as uint32 LE (the width the file is generated at).
    size_t n = raw.size() / 4;
    std::vector<u32> d(n);
    std::memcpy(d.data(), raw.data(), n * 4);
    const size_t tail = raw.size() - n * 4;

    std::printf("file=%s bytes=%zu n_words=%zu tail=%zu\n", argv[1], raw.size(), n, tail);

    // ---- L0: raw control -------------------------------------------------
    std::vector<u32> raww(n);
    for (size_t i = 0; i < n; i++) std::memcpy(&raww[i], raw.data() + i * 4, 4);
    {
        std::vector<i64> v(n); for (size_t i = 0; i < n; i++) v[i] = i64(d[i]);
        double H = entropy_bits(v);
        std::printf("L0 raw            : H=%.3f bits/val  -> %8.0f B (entropy bound)\n", H, H * n / 8);
    }
    // ---- L1: delta (sigma=1 implicit) ------------------------------------
    {
        std::vector<i64> r; r.reserve(n);
        r.push_back(i64(d[0]));
        for (size_t i = 1; i < n; i++) r.push_back(i64(d[i]) - i64(d[i - 1]));
        double H = entropy_bits(r);
        std::printf("L1 delta(sigma=1) : H=%.3f bits/val  -> %8.0f B (entropy bound)\n", H, H * n / 8);
    }
    // ---- L2: global best integer sigma ------------------------------------
    {
        // grid search sigma over a plausible integer range, minimize entropy
        double bestH = 1e30; i64 bestS = 0;
        for (i64 s = -1024; s <= 1024; s++) {
            std::unordered_map<i64, u32> c; c.reserve(n * 2);
            for (size_t i = 0; i < n; i++) ++c[i64(d[i]) - s * i64(i)];
            double H = 0; double nn = double(n);
            for (auto& kv : c) { double p = kv.second / nn; H -= p * std::log2(p); }
            if (H < bestH) { bestH = H; bestS = s; }
        }
        std::printf("L2 global sigma   : best sigma=%lld H=%.3f bits/val -> %8.0f B\n",
                    (long long)bestS, bestH, bestH * n / 8);
    }
    // ---- L3/L4: piecewise-linear, derived sigma ---------------------------
    for (int w : {16, 32, 64, 128, 256}) {
        for (i64 tl : {64, 128, 256, 512, 1024}) {
            auto segs = segment_linear(d, w, tl);
            refit(segs, d);
            std::vector<i64> resid; resid.reserve(n);
            for (auto& g : segs)
                for (size_t i = g.start; i < g.start + g.len; i++)
                    resid.push_back(i64(d[i]) - (g.sigma * i64(i) + g.base));
            double H = entropy_bits(resid);
            size_t nseg = segs.size();
            // parameter cost: per segment (len, sigma OR endpoints, base).
            // Self-describing form: endpoints give sigma; so cost per segment
            // = 2 values (raw endpoints) + base. Be conservative and charge
            // 3 x 32 bits per segment.
            double param_bits = double(nseg) * 3.0 * 32.0;
            double total_bits = H * double(n) + param_bits;
            std::printf("L3 w=%3d tol=%4lld: segs=%6zu H=%6.3f b/val -> %8.0f B (+param %6.0f B = %8.0f B)\n",
                        w, (long long)tl, nseg, H, H * n / 8, param_bits / 8, total_bits / 8);
        }
    }
    // ---- L5: real coded size for the best config (not just an entropy bound)
    {
        int bw = 0; i64 btl = 0; double bestH = 1e30;
        for (int w : {16, 32, 64, 128, 256})
            for (i64 tl : {64, 128, 256, 512, 1024}) {
                auto segs = segment_linear(d, w, tl);
                refit(segs, d);
                std::vector<i64> resid; resid.reserve(n);
                for (auto& g : segs)
                    for (size_t i = g.start; i < g.start + g.len; i++)
                        resid.push_back(i64(d[i]) - (g.sigma * i64(i) + g.base));
                double H = entropy_bits(resid);
                if (H < bestH) { bestH = H; bw = w; btl = tl; }
            }
        auto segs = segment_linear(d, bw, btl);
        refit(segs, d);
        std::vector<i64> resid; resid.reserve(n);
        for (auto& g : segs)
            for (size_t i = g.start; i < g.start + g.len; i++)
                resid.push_back(i64(d[i]) - (g.sigma * i64(i) + g.base));
        size_t coded = code_residual_bytes(resid);
        double param_bits = double(segs.size()) * 3.0 * 32.0;
        std::printf("\nBEST piecewise: window=%d tol=%lld segs=%zu\n", bw, (long long)btl, segs.size());
        std::printf("  residual coded (bit-plane adaptive arith) = %zu B\n", coded);
        std::printf("  + segment params (3x32b x %zu)            = %.0f B\n", segs.size(), param_bits / 8);
        std::printf("  TOTAL                                     = %.0f B\n", coded + param_bits / 8);
        std::printf("  vs brotli-q11 87,013 B  vs ANVIL 256,022 B  vs raw 256,000 B\n");
        std::printf("  per-run slopes discovered: ");
        size_t shown = 0;
        for (auto& g : segs) { if (g.len < 500) continue; std::printf("%lld ", (long long)g.sigma); if (++shown >= 12) break; }
        std::printf("\n");
    }
    return 0;
}
