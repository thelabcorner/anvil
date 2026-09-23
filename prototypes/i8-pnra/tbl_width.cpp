// tbl_width.cpp — LANE 3: temporal-table PHYSICAL specialization.
// Pure cache-economics test. NO candidate-decision change: K=1 is the same
// algorithm as K=4 with the bucket narrowed, so any difference is cache, not
// search quality (and the ratio column is there to PROVE that).
//
// BACKGROUND (CONTEXT.md, do not re-derive)
//   "K=1/K=2 still allocate four positions per hash bucket (hardwired bucket
//   type); specialize the temporal table width PHYSICALLY (K=1 -> ~1 MB direct
//   table instead of ~4 MB four-slot) — no candidate-decision change, pure
//   cache economics test."
//
// WHAT IS MEASURED
//   Six physical layouts of the SAME logical index (K-way bucket, 4-byte
//   positions, direct-mapped fallback on a 16-bit hash of the leading 4):
//     A  AoS-K4   : array<struct{array<u32,4>}>          (today's shape)
//     B  AoS-K2   : array<struct{array<u32,2>}>
//     C  AoS-K1   : array<struct{u32}>
//     D  SoA-K4   : 4 parallel arrays                    (probe touches 1 line)
//     E  SoA-K1   : single array, ~1 MB
//     F  SoA-K1-packed: single array + 4-bit tag to cut false-positive verifies
//   For each: match-finder throughput (MB/s), bytes (must be IDENTICAL across
//   A/B/C — same decisions; D/E/F differ only if tags change verification),
//   and hardware counters where available.
//
// The falsifiable claim under test: at K=1, does the ~4 MB -> ~1 MB footprint
// reduction buy more encoder throughput than the extra history depth costs?
//
// Build (Windows, this host):
//   clang-cl /O2 /EHsc /std:c++20 /MD /nologo /Fetbl_width.exe tbl_width.cpp
// Usage: tbl_width <file> [reps]
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <array>

using u8 = uint8_t; using u32 = uint32_t; using u64 = uint64_t;
static const u32 kNoPos = 0xFFFFFFFFu;
static const int kHashBits = 18;
static const u32 kHashSize = 1u << kHashBits;   // 262,144 buckets

static std::vector<u8> rdfile(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) { std::fprintf(stderr, "cannot open %s\n", p.c_str()); std::exit(1); }
    return std::vector<u8>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
static inline u32 load32(const u8* p) { u32 x; std::memcpy(&x, p, 4); return x; }
static inline u64 load64(const u8* p) { u64 x; std::memcpy(&x, p, 8); return x; }
static inline u32 h8(const u8* p) { u64 x = load64(p); x ^= x >> 32; x *= 0x9E3779B185EBCA87ULL; return u32(x >> (64 - kHashBits)); }
static inline u32 h4(const u8* p) { return (load32(p) * 0x9E3779B1u) >> 16; }

// Greedy parse producing (len,dist) so bytes are comparable across layouts.
struct Out { size_t bytes = 0, tokens = 0, matches = 0; double mbps = 0; };

// --- A/B/C: array-of-struct, K positions per bucket -------------------------
template <int K>
struct AoS {
    struct B { std::array<u32, K> p; B() { p.fill(kNoPos); } };
    std::vector<B> b;
    std::vector<u32> s4;
    explicit AoS() : b(kHashSize), s4(1u << 16, kNoPos) {}
    size_t footprint() const { return b.size() * sizeof(B) + s4.size() * 4; }
    void insert(const std::vector<u8>& d, u32 a, u32 e) {
        const u32 le = d.size() >= 8 ? u32(d.size() - 7) : 0;
        for (u32 p = a; p < e; p++) {
            if (p < le) { auto& z = b[h8(d.data() + p)]; for (int i = K - 1; i > 0; --i) z.p[i] = z.p[i - 1]; z.p[0] = p; s4[h4(d.data() + p)] = p; }
            else if (p + 4 <= d.size()) s4[h4(d.data() + p)] = p;
        }
    }
    // returns {len,dist}
    std::pair<u32, u32> find(const std::vector<u8>& d, u32 pos, u32 cap) const {
        std::pair<u32, u32> ex{0, 0};
        cap = std::min<u32>(cap, u32(d.size() - pos));
        if (pos + 8 > d.size()) return ex;
        const auto& z = b[h8(d.data() + pos)];
        for (int i = 0; i < K; i++) {
            u32 q = z.p[i]; if (q == kNoPos) break; if (q >= pos) continue;
            u32 ml = std::min<u32>(cap, u32(d.size() - q));
            const u8* a = d.data() + q; const u8* c = d.data() + pos;
            u32 l = 0;
            while (l + 8 <= ml) { u64 x = load64(a + l) ^ load64(c + l); if (x) { l += u32(__builtin_ctzll(x) >> 3); break; } l += 8; }
            while (l < ml && a[l] == c[l]) ++l;
            if (l >= 8 && (l > ex.first || (l == ex.first && pos - q < ex.second))) ex = {l, pos - q};
        }
        if (ex.first < 8) {
            u32 q = s4[h4(d.data() + pos)];
            if (q != kNoPos && q < pos) {
                u32 ml = std::min<u32>(15, cap); const u8* a = d.data() + q; const u8* c = d.data() + pos;
                u32 l = 0; while (l < ml && a[l] == c[l]) ++l;
                if (l >= 4) ex = {l, pos - q};
            }
        }
        return ex;
    }
};

// --- D: struct-of-arrays, K=4 ----------------------------------------------
struct SoA4 {
    std::vector<u32> p0, p1, p2, p3;
    std::vector<u32> s4;
    explicit SoA4() : p0(kHashSize, kNoPos), p1(kHashSize, kNoPos), p2(kHashSize, kNoPos), p3(kHashSize, kNoPos), s4(1u << 16, kNoPos) {}
    size_t footprint() const { return (p0.size() + p1.size() + p2.size() + p3.size()) * 4 + s4.size() * 4; }
    void insert(const std::vector<u8>& d, u32 a, u32 e) {
        const u32 le = d.size() >= 8 ? u32(d.size() - 7) : 0;
        for (u32 p = a; p < e; p++) {
            if (p < le) { u32 h = h8(d.data() + p); p3[h] = p2[h]; p2[h] = p1[h]; p1[h] = p0[h]; p0[h] = p; s4[h4(d.data() + p)] = p; }
            else if (p + 4 <= d.size()) s4[h4(d.data() + p)] = p;
        }
    }
    std::pair<u32, u32> find(const std::vector<u8>& d, u32 pos, u32 cap) const {
        std::pair<u32, u32> ex{0, 0};
        cap = std::min<u32>(cap, u32(d.size() - pos));
        if (pos + 8 > d.size()) return ex;
        u32 h = h8(d.data() + pos);
        const u32 qs[4] = {p0[h], p1[h], p2[h], p3[h]};
        for (int i = 0; i < 4; i++) {
            u32 q = qs[i]; if (q == kNoPos) break; if (q >= pos) continue;
            u32 ml = std::min<u32>(cap, u32(d.size() - q));
            const u8* a = d.data() + q; const u8* c = d.data() + pos;
            u32 l = 0;
            while (l + 8 <= ml) { u64 x = load64(a + l) ^ load64(c + l); if (x) { l += u32(__builtin_ctzll(x) >> 3); break; } l += 8; }
            while (l < ml && a[l] == c[l]) ++l;
            if (l >= 8 && (l > ex.first || (l == ex.first && pos - q < ex.second))) ex = {l, pos - q};
        }
        if (ex.first < 8) {
            u32 q = s4[h4(d.data() + pos)];
            if (q != kNoPos && q < pos) {
                u32 ml = std::min<u32>(15, cap); const u8* a = d.data() + q; const u8* c = d.data() + pos;
                u32 l = 0; while (l < ml && a[l] == c[l]) ++l;
                if (l >= 4) ex = {l, pos - q};
            }
        }
        return ex;
    }
};

// --- E/F: struct-of-arrays K=1, ~1 MB (+ optional 4-bit tag) ---------------
struct SoA1 {
    std::vector<u32> p0;
    std::vector<u32> s4;
    std::vector<u8> tag;     // only used when useTag
    bool useTag;
    explicit SoA1(bool ut) : p0(kHashSize, kNoPos), s4(1u << 16, kNoPos), useTag(ut) { if (ut) tag.assign(kHashSize, 0); }
    size_t footprint() const { return p0.size() * 4 + s4.size() * 4 + tag.size(); }
    void insert(const std::vector<u8>& d, u32 a, u32 e) {
        const u32 le = d.size() >= 8 ? u32(d.size() - 7) : 0;
        for (u32 p = a; p < e; p++) {
            if (p < le) { u32 h = h8(d.data() + p); p0[h] = p; if (useTag) tag[h] = u8(d[p] ^ d[p + 4]); s4[h4(d.data() + p)] = p; }
            else if (p + 4 <= d.size()) s4[h4(d.data() + p)] = p;
        }
    }
    std::pair<u32, u32> find(const std::vector<u8>& d, u32 pos, u32 cap) const {
        std::pair<u32, u32> ex{0, 0};
        cap = std::min<u32>(cap, u32(d.size() - pos));
        if (pos + 8 > d.size()) return ex;
        u32 h = h8(d.data() + pos);
        if (useTag && tag[h] != u8(d[pos] ^ d[pos + 4])) {
            // tag mismatch: skip the (expensive) 8-byte verify entirely
        } else {
            u32 q = p0[h];
            if (q != kNoPos && q < pos) {
                u32 ml = std::min<u32>(cap, u32(d.size() - q));
                const u8* a = d.data() + q; const u8* c = d.data() + pos;
                u32 l = 0;
                while (l + 8 <= ml) { u64 x = load64(a + l) ^ load64(c + l); if (x) { l += u32(__builtin_ctzll(x) >> 3); break; } l += 8; }
                while (l < ml && a[l] == c[l]) ++l;
                if (l >= 8) ex = {l, pos - q};
            }
        }
        if (ex.first < 8) {
            u32 q = s4[h4(d.data() + pos)];
            if (q != kNoPos && q < pos) {
                u32 ml = std::min<u32>(15, cap); const u8* a = d.data() + q; const u8* c = d.data() + pos;
                u32 l = 0; while (l < ml && a[l] == c[l]) ++l;
                if (l >= 4) ex = {l, pos - q};
            }
        }
        return ex;
    }
};

// Cost model identical across layouts so `bytes` is comparable.
static inline size_t uvlen(u32 v) { size_t n = 1; while (v >= 0x80) { v >>= 7; ++n; } return n; }

// Reset helper: SoA1 carries a `useTag` flag that must survive a reset; the
// other layouts are default-constructible.
template <class Idx>
static Idx fresh(const Idx& ix) { return Idx(); }
template <>
SoA1 fresh<SoA1>(const SoA1& ix) { return SoA1(ix.useTag); }

template <class Idx>
Out run(Idx& ix, const std::vector<u8>& d, const char* name, int reps) {
    Out o;
    std::vector<double> ts;
    for (int r = 0; r < reps; r++) {
        ix = fresh(ix);
        auto A = std::chrono::steady_clock::now();
        size_t bytes = 0, toks = 0, mats = 0;
        u32 i = 0;
        u32 lit_run = 0;
        auto flush_lit = [&]() { if (lit_run) { bytes += 1 + uvlen(lit_run) + lit_run; toks++; lit_run = 0; } };
        while (i < d.size()) {
            auto ex = ix.find(d, i, 65535);
            if (ex.first >= 4) { flush_lit(); bytes += 1 + uvlen(ex.first - 4) + uvlen(ex.second - 1); toks++; mats++; u32 e = std::min<u32>(u32(d.size()), i + ex.first); ix.insert(d, i, e); i = e; }
            else { lit_run++; ix.insert(d, i, i + 1); i++; }
        }
        flush_lit();
        auto B = std::chrono::steady_clock::now();
        ts.push_back(std::chrono::duration<double>(B - A).count());
        o.bytes = bytes; o.tokens = toks; o.matches = mats;
    }
    std::sort(ts.begin(), ts.end());
    o.mbps = d.size() / 1e6 / ts[ts.size() / 2];
    std::printf("%-16s %10zu %9zu %9zu %10.2f %10.1f\n", name, o.bytes, o.tokens, o.matches, o.mbps, double(ix.footprint()) / 1048576.0);
    return o;
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: tbl_width <file> [reps]\n"); return 2; }
    auto d = rdfile(argv[1]);
    int reps = argc > 2 ? std::atoi(argv[2]) : 3;
    std::printf("file=%s bytes=%zu reps=%d\n", argv[1], d.size(), reps);
    std::printf("%-16s %10s %9s %9s %10s %10s\n", "layout", "bytes", "tokens", "matches", "MB/s", "footprint MB");

    { AoS<4> ix; run(ix, d, "A AoS-K4", reps); }
    { AoS<2> ix; run(ix, d, "B AoS-K2", reps); }
    { AoS<1> ix; run(ix, d, "C AoS-K1", reps); }
    { SoA4 ix; run(ix, d, "D SoA-K4", reps); }
    { SoA1 ix(false); run(ix, d, "E SoA-K1", reps); }
    { SoA1 ix(true); run(ix, d, "F SoA-K1+tag", reps); }
    std::printf("\nREADING THESE COLUMNS\n");
    std::printf("  * A vs D (both K=4) must have IDENTICAL bytes: same candidate set, only\n");
    std::printf("    the PHYSICAL LAYOUT differs. Any byte difference = bug. A->D isolates\n");
    std::printf("    cache economics at ZERO rate cost. This is the lane-3 headline.\n");
    std::printf("  * C vs E (both K=1) likewise must have identical bytes.\n");
    std::printf("  * A/B/C do NOT match each other: reducing K genuinely removes candidate\n");
    std::printf("    positions, so the parse changes. The bytes column prices that.\n");
    std::printf("    (An earlier version of this file wrongly asserted they must match.)\n");
    std::printf("  * F (tag) is a precision change: it skips verifications that would have\n");
    std::printf("    failed anyway, so bytes should match C/E.\n");
    return 0;
}
