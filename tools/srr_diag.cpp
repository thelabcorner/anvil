// SRR alignment diagnostic (t4-srr): measures whether the greedy sparse parse
// discovers record alignment. Reports, with channels on/off:
//   - type-2 token distance histogram (near/far split, record-period window)
//   - mask recurrence: coverage of the top-N masks (pre-registered I4-2 metric b)
//   - modal-residual accuracy on (k,slot) contexts (R2 topology raw material)
// Build: clang-cl -O2 -Ithird_party/install/include tools/srr_diag.cpp -o build/srr_diag.exe
#define ANVIL_NO_MAIN
#include "../src/anvil.cpp"
#include <cstdio>
#include <map>
#include <algorithm>

struct Diag {
    uint64_t t2 = 0;                 // type-2 sparse tokens
    uint64_t near_d = 0;             // dist <= 16384
    uint64_t far_d = 0;              // dist > 16384
    uint64_t period_hit = 0;         // dist in [220,260] (jsonl record ~235 B)
    uint64_t t2_bytes = 0;
    uint64_t span_like = 0;          // len ~= dist (one-record tokens)
    std::map<uint32_t, uint32_t> dist_hist;
    std::map<uint64_t, uint32_t> mask_hist;      // 64-bit fingerprint of first 8 words
    std::map<std::pair<uint8_t,uint8_t>, std::map<uint8_t,uint32_t>> ks_hist; // (k,slot)->val
    std::map<std::pair<uint8_t,uint8_t>, std::map<uint8_t,uint32_t>> ks_hist_span; // span-restricted
    std::array<uint32_t, 256> dist_buckets{};    // log2 buckets
};

static Diag run(const std::vector<uint8_t>& d, bool channels, uint32_t plo, uint32_t phi) {
    Diag di;
    // options: max_chain=48 matches CLI default (block-size 256KB splits apply
    // in the real codec; diag parses whole file — directional)
    auto toks = anvil::parse_sparse(d, 48, 1u<<20, 12, false, channels, false);
    for (auto& t : toks) {
        if (t.type != 2) continue;
        ++di.t2; di.t2_bytes += t.len;
        if (t.dist <= 16384) ++di.near_d; else ++di.far_d;
        if (t.dist >= plo && t.dist <= phi) ++di.period_hit;
        bool sl = t.len >= t.dist && t.len <= t.dist + 8;
        if (sl) ++di.span_like;
        int b = 0; uint32_t x = t.dist; while (x >>= 1) ++b;
        di.dist_buckets[std::min(31,b)] += t.len;
        // mask fingerprint: up to 8 correction offsets (kSparseScanMax/32 words)
        uint64_t fp = 0;
        for (size_t j = 0; j < t.off.size() && j < 8; ++j) fp = fp * 1099511628211ull + t.off[j];
        ++di.mask_hist[fp];
        for (size_t j = 0; j < t.off.size(); ++j) {
            ++di.ks_hist[{uint8_t(t.off.size()), uint8_t(j)}][t.val[j]];
            if (sl) ++di.ks_hist_span[{uint8_t(t.off.size()), uint8_t(j)}][t.val[j]];
        }
    }
    return di;
}

static void report(const char* label, const Diag& d) {
    uint64_t top = 0;
    std::vector<std::pair<uint32_t,uint64_t>> bycnt;
    for (auto& [fp, c] : d.mask_hist) bycnt.push_back({c, fp});
    std::sort(bycnt.begin(), bycnt.end(), [](auto&a, auto&b){ return a.first > b.first; });
    for (size_t i = 0; i < bycnt.size() && i < 32; ++i) top += bycnt[i].first;
    // modal accuracy over (k,slot) contexts with >= 2 observations
    uint64_t modal_hits = 0, modal_tot = 0;
    for (auto& [ks, vals] : d.ks_hist) {
        uint32_t tot = 0, bcnt = 0;
        for (auto& [v, c] : vals) { tot += c; bcnt = std::max(bcnt, c); }
        if (tot >= 2) { modal_tot += tot; modal_hits += bcnt; }
    }
    double modal = modal_tot ? 100.0 * double(modal_hits) / double(modal_tot) : 0.0;
    // span-restricted modal accuracy
    uint64_t sm_hits = 0, sm_tot = 0;
    for (auto& [ks, vals] : d.ks_hist_span) {
        uint32_t tot = 0, bcnt = 0;
        for (auto& [v, c] : vals) { tot += c; bcnt = std::max(bcnt, c); }
        if (tot >= 2) { sm_tot += tot; sm_hits += bcnt; }
    }
    double smodal = sm_tot ? 100.0 * double(sm_hits) / double(sm_tot) : 0.0;
    double t2 = d.t2 ? 100.0 : 0.0;
    printf("%s: t2=%llu t2bytes=%llu\n", label, (unsigned long long)d.t2, (unsigned long long)d.t2_bytes);
    printf("  dist: near(<=16384)=%llu (%.1f%%) far=%llu (%.1f%%) period[lo,hi]=%llu (%.1f%%)\n",
        (unsigned long long)d.near_d, t2 ? 100.0*d.near_d/d.t2 : 0, (unsigned long long)d.far_d,
        t2 ? 100.0*d.far_d/d.t2 : 0, (unsigned long long)d.period_hit, t2 ? 100.0*d.period_hit/d.t2 : 0);
    printf("  masks: unique=%zu top32_coverage=%.1f%% (of %llu)\n", d.mask_hist.size(),
        d.t2 ? 100.0*double(top)/double(d.t2) : 0.0, (unsigned long long)d.t2);
    printf("  (k,slot) modal accuracy (>=2 obs): %.1f%% all | %.1f%% span-only (n=%llu span tokens)\n", modal, smodal, (unsigned long long)d.span_like);
    printf("  dist-byte bucket: ");
    for (int i = 4; i <= 24; ++i) if (d.dist_buckets[i]) printf("[2^%d]=%llu ", i, (unsigned long long)d.dist_buckets[i]);
    printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: srr_diag <file> [period_lo period_hi]\n"); return 2; }
    auto d = anvil::read_file(argv[1]);
    // default window matches the original jsonl-record-period target (Experiment T);
    // pass explicit bounds for corpora with a different record stride.
    uint32_t plo = argc >= 4 ? (uint32_t)atoi(argv[2]) : 220;
    uint32_t phi = argc >= 4 ? (uint32_t)atoi(argv[3]) : 260;
    auto off = run(d, false, plo, phi);
    auto on  = run(d, true, plo, phi);
    report("channels=OFF", off);
    report("channels=ON ", on);
    return 0;
}
