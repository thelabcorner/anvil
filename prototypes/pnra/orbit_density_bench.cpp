// orbit_density_bench.cpp — Experiment U (t4-orbit-diag) microbenchmark.
//
// Standalone, independently-buildable (no dependency on the missing
// tcopy_flat_hot_lib.inc that prototypes/pnra/tcopy_pnra*.cpp require).
//
// Tests the pre-registered falsifiable hypothesis for I4-4 (RESEARCH_LEDGER.md
// Experiment U): PNRA's O(1)-per-candidate cost is bought by SPARSE event
// generation (x86 E8/E9 opcode triggers, ~5% density), not by lookup
// cheapness alone. A second invariant family (finite-difference / stride)
// has no equivalent structural marker in general byte streams, so an
// equivalent event stream must be built at every (or every k-aligned)
// position — dense, not sparse. This benchmark measures whether that dense
// candidate generation costs materially more per output byte than the
// sparse one, i.e. whether adding an invariant family without a sparse
// trigger breaks the "O(1) parser cost per added invariant" target.
//
// Both invariant families use the SAME lookup structure shape PNRA uses
// (fixed-size direct-mapped hash table, K-way contiguous bucket, no pointer
// chasing) so the comparison isolates candidate-generation density, not
// lookup-structure differences (Experiment U already found PNRA's lookup
// structure is not the bottleneck — this benchmark does not re-litigate
// that; it holds the lookup structure constant and varies density).

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace diag {

static std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + path);
    f.seekg(0, std::ios::end);
    size_t n = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> d(n);
    if (n) f.read(reinterpret_cast<char*>(d.data()), static_cast<std::streamsize>(n));
    return d;
}

static inline uint32_t load32(const uint8_t* p) {
    uint32_t x; std::memcpy(&x, p, 4); return x;
}

// Same lookup-structure shape as PNRA::tab/PNRA::put/PNRA::find:
// fixed 1<<BITS buckets, K-way contiguous slot array, insert = shift,
// lookup = linear scan of K contiguous slots after one hash. No pointer
// chasing / no dependent-load chain (Experiment U finding: this shape is
// not the bottleneck; held constant here).
template <int BITS, int K>
struct FixedTable {
    struct Slot { uint32_t key = 0, pos = 0xFFFFFFFFu; };
    struct Bucket { std::array<Slot, K> s; };
    std::vector<Bucket> tab;
    FixedTable() : tab(size_t(1) << BITS) {}
    static inline uint32_t hh(uint32_t x) {
        x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
        return x & ((1u << BITS) - 1);
    }
    inline void put(uint32_t key, uint32_t pos) {
        auto& b = tab[hh(key)];
        for (int i = K - 1; i > 0; --i) b.s[i] = b.s[i - 1];
        b.s[0] = {key, pos};
    }
    // returns whether a matching key was found at an earlier position (the
    // "equivalence-class membership test" PNRA::find performs before the
    // expensive byte-level verify() step).
    inline bool probe(uint32_t key, uint32_t pos, uint32_t& hit_pos) const {
        const auto& b = tab[hh(key)];
        for (const auto& s : b.s) {
            if (s.pos == 0xFFFFFFFFu) break;
            if (s.key == key && s.pos < pos) { hit_pos = s.pos; return true; }
        }
        return false;
    }
};

struct Result {
    const char* label;
    uint64_t candidates = 0;
    uint64_t hits = 0;
    double density = 0.0;      // candidates / file size
    double ms = 0.0;
    double mbps = 0.0;         // FILE SIZE MB / time — throughput relative to input, comparable across families
    double cand_per_us = 0.0;  // candidate ops / microsecond — raw per-candidate rate
};

// Family A: translation invariant, PNRA's real sparse trigger — x86 E8/E9
// opcode bytes. key = value_at(f) + f (invariant under Δ=-dist translation).
static Result bench_sparse_translation(const std::vector<uint8_t>& d) {
    FixedTable<17, 4> tab;
    uint64_t cand = 0, hits = 0;
    auto t0 = std::chrono::steady_clock::now();
    for (uint32_t op = 0; op + 5 <= d.size(); ++op) {
        if (d[op] != 0xE8 && d[op] != 0xE9) continue;
        uint32_t f = op + 1;
        uint32_t key = load32(d.data() + f) + f;
        uint32_t hitpos;
        if (tab.probe(key, f, hitpos)) ++hits;
        tab.put(key, f);
        ++cand;
    }
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    Result r{"sparse-translation(E8/E9)", cand, hits, double(cand) / std::max<size_t>(1, d.size()), ms,
              d.size() / 1e6 / std::max(1e-9, ms / 1000.0), cand / std::max(1e-9, ms * 1000.0)};
    return r;
}

// Family B: degree-1 finite-difference invariant (predecessor-encoding /
// counter family), evaluated at EVERY 4-byte-aligned position — there is
// no structural marker analogous to E8/E9 for this family in general byte
// streams, so candidate generation must be dense. key = value_at(p) - p
// (invariant under a constant per-step increment, i.e. y[p] - p is
// constant along an arithmetic progression with step 1; using -p directly
// tests the "no filter" dense case).
static Result bench_dense_finite_difference(const std::vector<uint8_t>& d) {
    FixedTable<17, 4> tab;
    uint64_t cand = 0, hits = 0;
    auto t0 = std::chrono::steady_clock::now();
    for (uint32_t p = 0; p + 4 <= d.size(); p += 4) {
        uint32_t key = load32(d.data() + p) - p;
        uint32_t hitpos;
        if (tab.probe(key, p, hitpos)) ++hits;
        tab.put(key, p);
        ++cand;
    }
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    Result r{"dense-finite-diff(every 4B)", cand, hits, double(cand) / std::max<size_t>(1, d.size()), ms,
              d.size() / 1e6 / std::max(1e-9, ms / 1000.0), cand / std::max(1e-9, ms * 1000.0)};
    return r;
}

// Family B': same finite-difference family but with a cheap PRE-FILTER
// before the expensive key+hash+probe step — only evaluate a candidate if
// it locally looks like it could be part of an arithmetic run (its value
// minus 4 bytes back's value equals a small constant). This tests whether
// a cheap dense-family filter (the primitive the falsifiable-hypothesis
// paragraph in Experiment U names as the fix, IF the hypothesis confirms)
// recovers most of the throughput while still visiting every position.
static Result bench_dense_prefiltered(const std::vector<uint8_t>& d) {
    FixedTable<17, 4> tab;
    uint64_t cand = 0, hits = 0, scanned = 0;
    auto t0 = std::chrono::steady_clock::now();
    for (uint32_t p = 0; p + 8 <= d.size(); p += 4) {
        ++scanned;
        uint32_t v0 = load32(d.data() + p);
        uint32_t v1 = load32(d.data() + p + 4);
        int32_t step = int32_t(v1 - v0);
        // cheap pre-test: only positions whose local step is "small" (looks
        // like a plausible counter/timestamp stride) pay for key+hash+probe.
        if (step < -256 || step > 256) continue;
        uint32_t key = v0 - p;
        uint32_t hitpos;
        if (tab.probe(key, p, hitpos)) ++hits;
        tab.put(key, p);
        ++cand;
    }
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    Result r{"dense-finite-diff+prefilter", cand, hits, double(cand) / std::max<size_t>(1, d.size()), ms,
              d.size() / 1e6 / std::max(1e-9, ms / 1000.0), cand / std::max(1e-9, ms * 1000.0)};
    (void)scanned;
    return r;
}

static void print_row(const Result& r) {
    std::cout << r.label << "," << r.candidates << "," << r.hits << ","
              << r.density << "," << r.ms << "," << r.mbps << "," << r.cand_per_us << "\n";
}

} // namespace diag

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: orbit_density_bench <file> [file...]\n";
        return 2;
    }
    std::cout << "file,family,candidates,hits,density,ms,input_MBps,cand_per_us\n";
    for (int i = 1; i < argc; ++i) {
        auto d = diag::read_file(argv[i]);
        constexpr int REPS = 5;
        diag::Result best_a, best_b, best_c;
        best_a.ms = best_b.ms = best_c.ms = 1e300;
        for (int z = 0; z < REPS; ++z) {
            auto a = diag::bench_sparse_translation(d);
            auto b = diag::bench_dense_finite_difference(d);
            auto c = diag::bench_dense_prefiltered(d);
            if (a.ms < best_a.ms) best_a = a;
            if (b.ms < best_b.ms) best_b = b;
            if (c.ms < best_c.ms) best_c = c;
        }
        std::cout << argv[i] << "," ; diag::print_row(best_a);
        std::cout << argv[i] << "," ; diag::print_row(best_b);
        std::cout << argv[i] << "," ; diag::print_row(best_c);
    }
    return 0;
}
