#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <vector>

namespace {

using u64 = std::uint64_t;

struct Entry {
    u64 count;
    u64 length;
};

u64 bit_width_u64(u64 x) {
    u64 w = 0;
    while (x) { ++w; x >>= 1; }
    return w;
}

u64 uvar_len(u64 x) {
    u64 n = 1;
    while (x >= 0x80) { x >>= 7; ++n; }
    return n;
}

u64 naive_cost(const std::vector<Entry>& ranked, u64 page_occurrences, u64 cap) {
    u64 bytes = uvar_len(cap);
    for (u64 i = 0; i < cap; ++i) bytes += uvar_len(ranked[i].length) + ranked[i].length;
    bytes += (page_occurrences * bit_width_u64(cap) + 7) / 8;
    for (u64 i = cap; i < ranked.size(); ++i)
        bytes += ranked[i].count * (uvar_len(ranked[i].length) + ranked[i].length);
    return bytes;
}

struct Fast {
    u64 best_cap = 0;
    u64 best_cost = 0;
    std::vector<u64> cost_of;
};

Fast exact_optimize(const std::vector<Entry>& ranked, u64 page_occurrences) {
    Fast out;
    out.cost_of.assign(ranked.size() + 1, 0);
    u64 total_escape = 0;
    for (const Entry& e : ranked) total_escape += e.count * (1 + e.length);
    u64 table_prefix = 0;
    u64 saving_prefix = 0;
    (void)table_prefix;
    for (u64 c = 0; c <= ranked.size(); ++c) {
        if (c > 0) {
            table_prefix += 1 + ranked[c - 1].length;
            saving_prefix += (1 + ranked[c - 1].length) * (ranked[c - 1].count - 1);
        }
        out.cost_of[c] = total_escape - saving_prefix + uvar_len(c)
            + (page_occurrences * bit_width_u64(c) + 7) / 8;
    }
    out.best_cap = 0;
    out.best_cost = out.cost_of[0];
    for (u64 c = 1; c <= ranked.size(); ++c) {
        if (out.cost_of[c] < out.best_cost) { out.best_cost = out.cost_of[c]; out.best_cap = c; }
    }
    return out;
}

const u64 kLadder[] = {0, 16, 64, 256, 1024, 4096, 16384, 65536};

u64 ladder_choice(std::vector<Entry>& ranked, const Fast& exact) {
    u64 best = 0;
    u64 best_cost = exact.cost_of[ranked.size() < 0 ? 0 : 0];
    bool have = false;
    for (u64 raw : kLadder) {
        const u64 cap = raw < ranked.size() ? raw : ranked.size();
        const u64 c = exact.cost_of[cap];
        if (!have || c < best_cost || (c == best_cost && cap < best)) {
            best_cost = c; best = cap; have = true;
        }
    }
    return best;
}

}  // namespace

int main(int argc, char** argv) {
    const u64 trials = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 200000;
    const u64 seed = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 0x5eed1234u;
    std::mt19937_64 rng(seed);

    u64 evaluated = 0;
    u64 fast_vs_naive_mismatch = 0;
    u64 ladder_strictly_worse = 0;
    u64 ladder_tied = 0;
    u64 ladder_zero = 0;
    u64 total_regret = 0;
    u64 total_optimal = 0;
    u64 worst_regret = 0;
    u64 worst_regret_distinct = 0;
    u64 worst_regret_occ = 0;
    u64 worst_regret_cap = 0;
    u64 regret_over_1pct_pages = 0;
    u64 regret_over_5pct_pages = 0;

    for (u64 t = 0; t < trials; ++t) {
        const u64 distinct = 1 + (rng() % 2000);
        const u64 page_occurrences = distinct + (rng() % 30000);
        const u64 max_count = 1 + (rng() % 5000);
        std::vector<Entry> ranked;
        ranked.reserve(distinct);
        u64 heavy = 1 + (rng() % (distinct < 40 ? distinct : 40));
        for (u64 i = 0; i < distinct; ++i) {
            Entry e;
            e.count = i < heavy ? 1 + (rng() % max_count) : 1;
            e.length = 1 + (rng() % 60);
            ranked.push_back(e);
        }
        std::sort(ranked.begin(), ranked.end(), [](const Entry& a, const Entry& b) {
            if (a.count != b.count) return a.count > b.count;
            return a.length < b.length;
        });

        u64 occ_total = 0;
        for (const Entry& e : ranked) occ_total += e.count;
        if (occ_total < page_occurrences) continue;
        ++evaluated;

        const Fast exact = exact_optimize(ranked, page_occurrences);
        for (u64 c = 0; c <= ranked.size(); ++c) {
            if (naive_cost(ranked, page_occurrences, c) != exact.cost_of[c]) {
                ++fast_vs_naive_mismatch;
                break;
            }
        }

        const u64 lad = ladder_choice(ranked, exact);
        const u64 cost_ladder = exact.cost_of[lad];
        if (lad == 0) ++ladder_zero;
        if (cost_ladder > exact.best_cost) {
            ++ladder_strictly_worse;
            const u64 regret = cost_ladder - exact.best_cost;
            total_regret += regret;
            if (regret * 100 >= exact.best_cost) ++regret_over_1pct_pages;
            if (regret * 20 >= exact.best_cost) ++regret_over_5pct_pages;
            if (regret > worst_regret) {
                worst_regret = regret;
                worst_regret_distinct = distinct;
                worst_regret_occ = page_occurrences;
                worst_regret_cap = lad;
            }
        } else if (cost_ladder == exact.best_cost) {
            ++ladder_tied;
        }
        total_optimal += exact.best_cost;
    }

    std::printf("seed                         0x%llx\n", (unsigned long long)seed);
    std::printf("pages_evaluated              %llu\n", (unsigned long long)evaluated);
    std::printf("fast_vs_naive_mismatches     %llu\n", (unsigned long long)fast_vs_naive_mismatch);
    std::printf("ladder_strictly_worse        %llu  (%.2f%%)\n",
                (unsigned long long)ladder_strictly_worse,
                evaluated ? 100.0 * (double)ladder_strictly_worse / (double)evaluated : 0.0);
    std::printf("ladder_tied_optimal          %llu  (%.2f%%)\n",
                (unsigned long long)ladder_tied,
                evaluated ? 100.0 * (double)ladder_tied / (double)evaluated : 0.0);
    std::printf("ladder_selected_cap_zero     %llu\n", (unsigned long long)ladder_zero);
    std::printf("regret_ge_1pct_of_optimal    %llu  (%.2f%%)\n",
                (unsigned long long)regret_over_1pct_pages,
                evaluated ? 100.0 * (double)regret_over_1pct_pages / (double)evaluated : 0.0);
    std::printf("regret_ge_5pct_of_optimal    %llu  (%.2f%%)\n",
                (unsigned long long)regret_over_5pct_pages,
                evaluated ? 100.0 * (double)regret_over_5pct_pages / (double)evaluated : 0.0);
    std::printf("ladder_regret_over_optimal   %.6f\n",
                total_optimal ? (double)total_regret / (double)total_optimal : 0.0);
    std::printf("worst_ladder_regret_bytes    %llu  (distinct=%llu occ=%llu ladder_cap=%llu)\n",
                (unsigned long long)worst_regret,
                (unsigned long long)worst_regret_distinct,
                (unsigned long long)worst_regret_occ,
                (unsigned long long)worst_regret_cap);
    return fast_vs_naive_mismatch == 0 ? 0 : 1;
}