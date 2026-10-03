#ifndef G5D_FROZEN_G3_HEADER
#define G5D_FROZEN_G3_HEADER "grotli_g3.cpp"
#endif

#define main grotli_g3_embedded_main
#include G5D_FROZEN_G3_HEADER
#undef main

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

namespace sha256 {

struct Ctx {
    uint32_t h[8];
    uint64_t len = 0;
    uint8_t buf[64];
    size_t buf_len = 0;
};

static const uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u,
    0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u,
    0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
    0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
    0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au,
    0x5b9cca4fu, 0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

static inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

static void init(Ctx& c) {
    c.h[0] = 0x6a09e667u; c.h[1] = 0xbb67ae85u; c.h[2] = 0x3c6ef372u;
    c.h[3] = 0xa54ff53au; c.h[4] = 0x510e527fu; c.h[5] = 0x9b05688cu;
    c.h[6] = 0x1f83d9abu; c.h[7] = 0x5be0cd19u;
}

static void compress(Ctx& c, const uint8_t* p) {
    uint32_t w[64];
    for (int i = 0; i < 16; ++i)
        w[i] = (uint32_t(p[i * 4]) << 24) | (uint32_t(p[i * 4 + 1]) << 16) |
               (uint32_t(p[i * 4 + 2]) << 8) | uint32_t(p[i * 4 + 3]);
    for (int i = 16; i < 64; ++i) {
        const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = c.h[0], b = c.h[1], cc = c.h[2], d = c.h[3];
    uint32_t e = c.h[4], f = c.h[5], g = c.h[6], h = c.h[7];
    for (int i = 0; i < 64; ++i) {
        const uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const uint32_t ch = (e & f) ^ (~e & g);
        const uint32_t t1 = h + s1 + ch + K[i] + w[i];
        const uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const uint32_t maj = (a & b) ^ (a & cc) ^ (b & cc);
        const uint32_t t2 = s0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = cc; cc = b; b = a; a = t1 + t2;
    }
    c.h[0] += a; c.h[1] += b; c.h[2] += cc; c.h[3] += d;
    c.h[4] += e; c.h[5] += f; c.h[6] += g; c.h[7] += h;
}

static void update(Ctx& c, const uint8_t* p, size_t n) {
    c.len += n;
    while (n) {
        const size_t take = std::min<size_t>(64 - c.buf_len, n);
        std::memcpy(c.buf + c.buf_len, p, take);
        c.buf_len += take;
        p += take;
        n -= take;
        if (c.buf_len == 64) { compress(c, c.buf); c.buf_len = 0; }
    }
}

static std::array<uint8_t, 32> final(Ctx& c) {
    const uint64_t bitlen = c.len * 8ull;
    const uint8_t pad = 0x80;
    update(c, &pad, 1);
    const uint8_t zero = 0;
    while (c.buf_len != 56) update(c, &zero, 1);
    uint8_t lenb[8];
    for (int i = 0; i < 8; ++i) lenb[i] = uint8_t(bitlen >> (56 - i * 8));
    update(c, lenb, 8);
    std::array<uint8_t, 32> out{};
    for (int i = 0; i < 8; ++i) {
        out[i * 4] = uint8_t(c.h[i] >> 24);
        out[i * 4 + 1] = uint8_t(c.h[i] >> 16);
        out[i * 4 + 2] = uint8_t(c.h[i] >> 8);
        out[i * 4 + 3] = uint8_t(c.h[i]);
    }
    return out;
}

static std::array<uint8_t, 32> hash(const Bytes& in) {
    Ctx c;
    init(c);
    update(c, in.data(), in.size());
    return final(c);
}

static std::string hex(const std::array<uint8_t, 32>& h) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (uint8_t b : h) {
        out.push_back(digits[b >> 4]);
        out.push_back(digits[b & 15]);
    }
    return out;
}

static std::string hex(const Bytes& in) { return hex(hash(in)); }

}

namespace {

using Bytes = std::vector<uint8_t>;

constexpr uint8_t kG5DVersion = 1;
constexpr std::array<uint8_t, 4> kG5DMagic = {'G', '5', 'A', 'O'};
constexpr uint8_t kG5DRawLexLeaf = 0;
constexpr uint8_t kG5DPagedDictLeaf = 5;
constexpr std::array<uint32_t, 3> kPagePolicies = {4096, 16384, 65536};
constexpr std::array<uint64_t, 8> kCapCandidates = {0, 16, 64, 256, 1024, 4096, 16384, 65536};

enum class G5DOrder : uint8_t {
    SourceOrder = 1,
    ShapeColumn = 3,
};

static const char* order_name(G5DOrder order) {
    switch (order) {
        case G5DOrder::SourceOrder: return "SOURCE_ORDER";
        case G5DOrder::ShapeColumn: return "SHAPE_COLUMN";
    }
    throw std::runtime_error("unknown G5D order");
}

static bool valid_order_selector(uint8_t value) {
    return value == static_cast<uint8_t>(G5DOrder::SourceOrder) ||
           value == static_cast<uint8_t>(G5DOrder::ShapeColumn);
}

static G5DOrder decode_order_selector(uint8_t value) {
    if (!valid_order_selector(value)) throw std::runtime_error("invalid G5D order selector");
    return static_cast<G5DOrder>(value);
}

static uint32_t page_policy(uint8_t id) {
    if (id >= kPagePolicies.size()) throw std::runtime_error("invalid G5D page policy id");
    return kPagePolicies[id];
}

static const std::string& source_token_multiset_sha256(const RegionAnalysis& analysis) {
    static thread_local std::string value;
    std::vector<Bytes> records;
    uint64_t count = 0;
    for (const ShapePlan& shape : analysis.shapes)
        for (const SlotPlan& slot : shape.slots) count += slot.tokens.size();
    if (count > std::vector<Bytes>().max_size()) throw std::runtime_error("token count too large");
    records.reserve(static_cast<size_t>(count));
    for (const ShapePlan& shape : analysis.shapes) {
        for (const SlotPlan& slot : shape.slots) {
            for (const Bytes& token : slot.tokens) {
                Bytes record;
                put_uvar(record, token.size());
                record.insert(record.end(), token.begin(), token.end());
                records.push_back(std::move(record));
            }
        }
    }
    std::sort(records.begin(), records.end());
    Bytes joined;
    for (const Bytes& record : records) joined.insert(joined.end(), record.begin(), record.end());
    value = sha256::hex(joined);
    return value;
}

struct TokenCoord {
    uint32_t shape = 0;
    uint32_t occurrence = 0;
    uint32_t slot = 0;
};

struct LeafCoord {
    size_t shape = 0;
    size_t slot = 0;
};

struct G5DPlan {
    RegionAnalysis analysis;
    Bytes prefix;
    uint64_t common_structure_metadata_bytes = 0;
    uint64_t residual_bytes = 0;
    std::vector<LeafCoord> leaves;
    std::vector<std::vector<size_t>> leaf_index;
    std::vector<TokenCoord> source_coordinates;
    std::vector<TokenCoord> shape_column_coordinates;
    std::string source_token_multiset_sha256;
};

static const Bytes& token_at(const G5DPlan& plan, const TokenCoord& coord) {
    if (coord.shape >= plan.analysis.shapes.size())
        throw std::runtime_error("G5D token shape out of range");
    const ShapePlan& shape = plan.analysis.shapes[coord.shape];
    if (coord.slot >= shape.slots.size())
        throw std::runtime_error("G5D token slot out of range");
    if (coord.occurrence >= shape.slots[coord.slot].tokens.size())
        throw std::runtime_error("G5D token occurrence out of range");
    return shape.slots[coord.slot].tokens[coord.occurrence];
}

static const std::vector<TokenCoord>& coordinates_for(
    const G5DPlan& plan, G5DOrder order)
{
    if (order == G5DOrder::SourceOrder) return plan.source_coordinates;
    if (order == G5DOrder::ShapeColumn) return plan.shape_column_coordinates;
    throw std::runtime_error("unknown G5D coordinate order");
}

static G5DPlan build_plan(const Bytes& source) {
    if (source.empty()) throw std::runtime_error("G5D source is empty");
    G5DPlan plan;
    plan.analysis = analyze_regions(source);
    const RegionAnalysis& analysis = plan.analysis;
    if (analysis.frames.empty()) throw std::runtime_error("G5D requires one frame");

    const bool has_raw = !analysis.raw_members.empty();
    const uint64_t group_count = analysis.shapes.size() + (has_raw ? 1u : 0u);
    if (group_count == 0 || group_count > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("G5D invalid group count");
    const uint32_t raw_group = has_raw ? 0u : std::numeric_limits<uint32_t>::max();
    const uint32_t structured_offset = has_raw ? 1u : 0u;

    Bytes& out = plan.prefix;
    out.insert(out.end(), kG5DMagic.begin(), kG5DMagic.end());
    out.push_back(kG5DVersion);
    put_uvar(out, source.size());
    put_uvar(out, analysis.frames.size());
    put_uvar(out, group_count);
    for (const ParsedRecord& record : analysis.records) {
        if (record.structured && record.shape_id >= analysis.shapes.size())
            throw std::runtime_error("G5D record shape out of range");
        const uint32_t group = record.structured
            ? structured_offset + record.shape_id
            : raw_group;
        put_uvar(out, group);
    }

    if (has_raw) {
        out.push_back(1);
        put_uvar(out, analysis.raw_members.size());
    }
    for (size_t sid = 0; sid < analysis.shapes.size(); ++sid) {
        const ShapePlan& shape = analysis.shapes[sid];
        const size_t slots = shape.parts.empty() ? 0 : shape.parts.size() - 1;
        if (shape.slots.size() != slots)
            throw std::runtime_error("G5D shape slot/template mismatch");
        out.push_back(0);
        put_uvar(out, shape.members.size());
        put_uvar(out, slots);
        for (const Bytes& part : shape.parts) {
            put_uvar(out, part.size());
            out.insert(out.end(), part.begin(), part.end());
        }
    }

    const size_t raw_start = out.size();
    if (has_raw) {
        for (uint32_t frame_id : analysis.raw_members) {
            if (frame_id >= analysis.frames.size())
                throw std::runtime_error("G5D raw frame id out of range");
            const Frame& frame = analysis.frames[frame_id];
            const uint64_t length = frame.hi - frame.lo;
            put_uvar(out, length);
            out.insert(out.end(), source.begin() + frame.lo, source.begin() + frame.hi);
        }
    }
    plan.common_structure_metadata_bytes = raw_start;
    plan.residual_bytes = out.size() - raw_start;

    plan.leaf_index.resize(analysis.shapes.size());
    for (size_t sid = 0; sid < analysis.shapes.size(); ++sid) {
        const ShapePlan& shape = analysis.shapes[sid];
        plan.leaf_index[sid].resize(shape.slots.size());
        for (size_t slot = 0; slot < shape.slots.size(); ++slot) {
            plan.leaf_index[sid][slot] = plan.leaves.size();
            plan.leaves.push_back({sid, slot});
        }
    }

    std::vector<size_t> next_occurrence(analysis.shapes.size(), 0);
    for (const ParsedRecord& record : analysis.records) {
        if (!record.structured) continue;
        const size_t sid = record.shape_id;
        if (sid >= analysis.shapes.size()) throw std::runtime_error("G5D source shape id");
        const ShapePlan& shape = analysis.shapes[sid];
        const size_t occurrence = next_occurrence[sid]++;
        if (occurrence >= shape.members.size())
            throw std::runtime_error("G5D source occurrence count");
        for (size_t slot = 0; slot < shape.slots.size(); ++slot) {
            if (occurrence >= shape.slots[slot].tokens.size())
                throw std::runtime_error("G5D missing source token");
            plan.source_coordinates.push_back({
                static_cast<uint32_t>(sid),
                static_cast<uint32_t>(occurrence),
                static_cast<uint32_t>(slot)
            });
        }
    }
    for (size_t sid = 0; sid < analysis.shapes.size(); ++sid) {
        if (next_occurrence[sid] != analysis.shapes[sid].members.size())
            throw std::runtime_error("G5D shape consumption mismatch");
        const ShapePlan& shape = analysis.shapes[sid];
        for (size_t slot = 0; slot < shape.slots.size(); ++slot) {
            for (size_t occurrence = 0; occurrence < shape.members.size(); ++occurrence) {
                plan.shape_column_coordinates.push_back({
                    static_cast<uint32_t>(sid),
                    static_cast<uint32_t>(occurrence),
                    static_cast<uint32_t>(slot)
                });
            }
        }
    }
    plan.source_token_multiset_sha256 = source_token_multiset_sha256(analysis);
    return plan;
}

static std::vector<std::pair<Bytes, uint64_t>> ranked_tokens(const std::vector<Bytes>& tokens) {
    std::map<Bytes, uint64_t> counts;
    for (const Bytes& token : tokens) {
        if (token.empty()) throw std::runtime_error("G5D empty token");
        uint64_t& count = counts[token];
        if (count == std::numeric_limits<uint64_t>::max())
            throw std::runtime_error("G5D token count overflow");
        ++count;
    }
    std::vector<std::pair<Bytes, uint64_t>> ranked;
    ranked.reserve(counts.size());
    for (auto& item : counts) ranked.push_back(std::move(item));
    std::sort(ranked.begin(), ranked.end(), [](const auto& left, const auto& right) {
        if (left.second != right.second) return left.second > right.second;
        return left.first < right.first;
    });
    return ranked;
}

struct PagePlan {
    size_t begin = 0;
    std::vector<Bytes> overlay;
    uint8_t code_width = 0;
    std::vector<uint64_t> ids;
    uint64_t escape_count = 0;
    uint64_t escape_bytes = 0;
};

struct LeafPlan {
    size_t index = 0;
    uint8_t policy_id = 0;
    uint32_t policy = 0;
    std::vector<Bytes> root;
    std::vector<PagePlan> pages;
    Bytes payload;
    uint64_t root_table_bytes = 0;
    uint64_t overlay_table_bytes = 0;
    uint64_t internal_detection_bytes = 0;
    uint64_t escape_count = 0;
};

static size_t exact_table_bytes(const std::vector<Bytes>& entries) {
    size_t bytes = uvar_len(entries.size());
    for (const Bytes& entry : entries) {
        if (entry.empty()) throw std::runtime_error("G5D empty table entry");
        bytes += uvar_len(entry.size());
        if (bytes > std::numeric_limits<size_t>::max() - entry.size())
            throw std::runtime_error("G5D table size overflow");
        bytes += entry.size();
    }
    return bytes;
}

static LeafPlan build_leaf_plan(
    const std::vector<Bytes>& tokens,
    size_t leaf_index,
    uint8_t policy_id,
    const std::optional<uint64_t>& forced_root_cap = std::nullopt)
{
    if (tokens.empty()) throw std::runtime_error("G5D empty dictionary leaf");
    if (policy_id >= kPagePolicies.size()) throw std::runtime_error("G5D page policy id");
    const uint32_t policy = kPagePolicies[policy_id];
    const std::vector<std::pair<Bytes, uint64_t>> ranked = ranked_tokens(tokens);

    std::vector<uint64_t> root_caps;
    if (forced_root_cap) {
        root_caps.push_back(std::min<uint64_t>(*forced_root_cap, ranked.size()));
    } else {
        for (uint64_t cap : kCapCandidates)
            root_caps.push_back(std::min<uint64_t>(cap, ranked.size()));
        std::sort(root_caps.begin(), root_caps.end());
        root_caps.erase(std::unique(root_caps.begin(), root_caps.end()), root_caps.end());
    }

    bool have_best = false;
    LeafPlan best;
    uint64_t best_cost = std::numeric_limits<uint64_t>::max();

    for (uint64_t root_cap : root_caps) {
        LeafPlan candidate;
        candidate.index = leaf_index;
        candidate.policy_id = policy_id;
        candidate.policy = policy;
        candidate.root.reserve(static_cast<size_t>(root_cap));
        for (size_t i = 0; i < static_cast<size_t>(root_cap); ++i)
            candidate.root.push_back(ranked[i].first);
        std::vector<Bytes> root_sorted = candidate.root;
        std::sort(root_sorted.begin(), root_sorted.end());

        const size_t page_count = (tokens.size() + policy - 1) / policy;
        candidate.pages.reserve(page_count);
        uint64_t local_cost = uvar_len(leaf_index) + 1 + exact_table_bytes(candidate.root);
        for (size_t page_id = 0; page_id < page_count; ++page_id) {
            const size_t begin = page_id * policy;
            const size_t end = std::min(tokens.size(), begin + policy);
            std::vector<Bytes> page_tokens(tokens.begin() + begin, tokens.begin() + end);
            std::vector<std::pair<Bytes, uint64_t>> page_ranked = ranked_tokens(page_tokens);
            std::vector<std::pair<Bytes, uint64_t>> local_ranked;
            for (auto& item : page_ranked) {
                if (!std::binary_search(root_sorted.begin(), root_sorted.end(), item.first))
                    local_ranked.push_back(std::move(item));
            }

            std::vector<uint64_t> overlay_caps;
            for (uint64_t cap : kCapCandidates)
                overlay_caps.push_back(std::min<uint64_t>(cap, local_ranked.size()));
            std::sort(overlay_caps.begin(), overlay_caps.end());
            overlay_caps.erase(std::unique(overlay_caps.begin(), overlay_caps.end()), overlay_caps.end());

            bool have_page = false;
            PagePlan best_page;
            uint64_t best_page_cost = std::numeric_limits<uint64_t>::max();
            for (uint64_t overlay_cap : overlay_caps) {
                PagePlan page;
                page.begin = begin;
                page.overlay.reserve(static_cast<size_t>(overlay_cap));
                for (size_t i = 0; i < static_cast<size_t>(overlay_cap); ++i)
                    page.overlay.push_back(local_ranked[i].first);
                std::vector<Bytes> overlay_sorted = page.overlay;
                std::sort(overlay_sorted.begin(), overlay_sorted.end());
                const uint64_t active = candidate.root.size() + page.overlay.size();
                page.code_width = bit_width_u64(active);
                page.ids.reserve(end - begin);
                for (const Bytes& token : page_tokens) {
                    const auto root_it = std::lower_bound(candidate.root.begin(), candidate.root.end(), token);
                    if (root_it != candidate.root.end() && *root_it == token) {
                        const size_t id = static_cast<size_t>(root_it - candidate.root.begin());
                        page.ids.push_back(id);
                    } else {
                        const auto overlay_it = std::lower_bound(page.overlay.begin(), page.overlay.end(), token);
                        if (overlay_it != page.overlay.end() && *overlay_it == token) {
                            const size_t local = static_cast<size_t>(overlay_it - page.overlay.begin());
                            page.ids.push_back(candidate.root.size() + local);
                        } else {
                            page.ids.push_back(active);
                            page.escape_count += 1;
                            page.escape_bytes += uvar_len(token.size()) + token.size();
                        }
                    }
                }
                const unsigned __int128 id_bits =
                    static_cast<unsigned __int128>(page.ids.size()) * page.code_width;
                const uint64_t id_bytes = static_cast<uint64_t>((id_bits + 7) / 8);
                const uint64_t table_bytes = exact_table_bytes(page.overlay) + 1;
                const unsigned __int128 page_cost128 =
                    static_cast<unsigned __int128>(id_bytes) + table_bytes + page.escape_bytes;
                if (page_cost128 > std::numeric_limits<uint64_t>::max())
                    throw std::runtime_error("G5D page cost overflow");
                const uint64_t page_cost = static_cast<uint64_t>(page_cost128);
                if (!have_page || page_cost < best_page_cost ||
                    (page_cost == best_page_cost && page.overlay.size() < best_page.overlay.size())) {
                    have_page = true;
                    best_page_cost = page_cost;
                    best_page = std::move(page);
                }
            }
            if (!have_page) throw std::runtime_error("G5D missing page candidate");
            const uint64_t page_table_bytes = exact_table_bytes(best_page.overlay) + 1;
            if (local_cost > std::numeric_limits<uint64_t>::max() - page_table_bytes - best_page.escape_bytes)
                throw std::runtime_error("G5D leaf cost overflow");
            local_cost += page_table_bytes + best_page.escape_bytes;
            const unsigned __int128 id_bits =
                static_cast<unsigned __int128>(best_page.ids.size()) * best_page.code_width;
            if (id_bits > std::numeric_limits<uint64_t>::max() - local_cost)
                throw std::runtime_error("G5D id cost overflow");
            local_cost += static_cast<uint64_t>((id_bits + 7) / 8);
            candidate.escape_count += best_page.escape_count;
            candidate.pages.push_back(std::move(best_page));
        }

        candidate.internal_detection_bytes = uvar_len(leaf_index) + 1;
        candidate.root_table_bytes = exact_table_bytes(candidate.root);
        for (const PagePlan& page : candidate.pages)
            candidate.overlay_table_bytes += exact_table_bytes(page.overlay) + 1;

        put_uvar(candidate.payload, leaf_index);
        candidate.payload.push_back(policy_id);
        put_uvar(candidate.payload, candidate.root.size());
        for (const Bytes& entry : candidate.root) {
            put_uvar(candidate.payload, entry.size());
            candidate.payload.insert(candidate.payload.end(), entry.begin(), entry.end());
        }
        for (const PagePlan& page : candidate.pages) {
            put_uvar(candidate.payload, page.overlay.size());
            for (const Bytes& entry : page.overlay) {
                put_uvar(candidate.payload, entry.size());
                candidate.payload.insert(candidate.payload.end(), entry.begin(), entry.end());
            }
            candidate.payload.push_back(page.code_width);
        }

        if (local_cost > std::numeric_limits<uint64_t>::max() - uvar_len(candidate.payload.size()))
            throw std::runtime_error("G5D descriptor cost overflow");
        const uint64_t total_cost = local_cost + uvar_len(candidate.payload.size());
        if (!have_best || total_cost < best_cost ||
            (total_cost == best_cost && candidate.root.size() < best.root.size())) {
            have_best = true;
            best_cost = total_cost;
            best = std::move(candidate);
        }
    }
    if (!have_best) throw std::runtime_error("G5D root selection failed");
    return best;
}

struct PolicyPlan {
    uint32_t policy = 0;
    std::vector<LeafPlan> leaves;
    std::string dictionary_plan_sha256;
    std::string table_content_sha256;
};

static PolicyPlan build_policy_plan(const G5DPlan& plan, uint8_t policy_id) {
    PolicyPlan result;
    result.policy = kPagePolicies[policy_id];
    result.leaves.reserve(plan.leaves.size());
    for (size_t leaf_id = 0; leaf_id < plan.leaves.size(); ++leaf_id) {
        const LeafCoord& leaf = plan.leaves[leaf_id];
        const ShapePlan& shape = plan.analysis.shapes[leaf.shape];
        result.leaves.push_back(build_leaf_plan(
            shape.slots[leaf.slot].tokens, leaf_id, policy_id));
    }
    Bytes plan_bytes;
    put_uvar(plan_bytes, result.leaves.size());
    Bytes table_bytes;
    for (const LeafPlan& leaf : result.leaves) {
        put_uvar(plan_bytes, leaf.policy_id);
        put_uvar(plan_bytes, leaf.root.size());
        for (const Bytes& entry : leaf.root) {
            put_uvar(plan_bytes, entry.size());
            plan_bytes.insert(plan_bytes.end(), entry.begin(), entry.end());
        }
        put_uvar(plan_bytes, leaf.pages.size());
        for (const PagePlan& page : leaf.pages) {
            put_uvar(plan_bytes, page.overlay.size());
            for (const Bytes& entry : page.overlay) {
                put_uvar(plan_bytes, entry.size());
                plan_bytes.insert(plan_bytes.end(), entry.begin(), entry.end());
            }
            plan_bytes.push_back(page.code_width);
        }
        table_bytes.insert(table_bytes.end(), leaf.payload.begin(), leaf.payload.end());
    }
    result.dictionary_plan_sha256 = sha256::hex(plan_bytes);
    result.table_content_sha256 = sha256::hex(table_bytes);
    return result;
}

class BitWriter {
public:
    void write(uint64_t value, uint8_t width) {
        if (width > 64) throw std::runtime_error("G5D bit width exceeds 64");
        if (width == 0) {
            if (value != 0) throw std::runtime_error("G5D nonzero width-zero code");
            return;
        }
        const uint64_t maximum = width == 64
            ? std::numeric_limits<uint64_t>::max()
            : (uint64_t{1} << width) - 1;
        if (value > maximum) throw std::runtime_error("G5D code exceeds width");
        unsigned __int128 accumulator = acc_;
        unsigned bits = bit_count_;
        accumulator |= static_cast<unsigned __int128>(value) << bits;
        bits += width;
        while (bits >= 8) {
            bytes_.push_back(static_cast<uint8_t>(accumulator & 0xffu));
            accumulator >>= 8;
            bits -= 8;
        }
        acc_ = accumulator;
        bit_count_ = bits;
    }

    void align_zero() {
        if (bit_count_ == 0) return;
        bytes_.push_back(static_cast<uint8_t>(acc_ & 0xffu));
        acc_ = 0;
        bit_count_ = 0;
    }

    size_t byte_count() const { return bytes_.size(); }

    const Bytes& data() const { return bytes_; }

private:
    Bytes bytes_;
    unsigned __int128 acc_ = 0;
    unsigned bit_count_ = 0;
};

class BitReader {
public:
    BitReader(const Bytes& bytes, size_t position) : bytes_(bytes), position_(position) {}

    uint64_t read(uint8_t width) {
        if (width > 64) throw std::runtime_error("G5D read width exceeds 64");
        if (width == 0) return 0;
        while (bit_count_ < width) {
            if (position_ >= bytes_.size())
                throw std::runtime_error("G5D truncated packed ids");
            acc_ |= static_cast<unsigned __int128>(bytes_[position_++]) << bit_count_;
            bit_count_ += 8;
        }
        const uint64_t mask = width == 64
            ? std::numeric_limits<uint64_t>::max()
            : (uint64_t{1} << width) - 1;
        const uint64_t value = static_cast<uint64_t>(acc_) & mask;
        acc_ >>= width;
        bit_count_ -= width;
        return value;
    }

    void align_zero() {
        if (bit_count_ == 0) return;
        const unsigned used = bit_count_;
        const uint64_t keep = (1u << used) - 1u;
        if ((static_cast<uint64_t>(acc_) & ~keep) != 0)
            throw std::runtime_error("G5D nonzero escape alignment bits");
        acc_ = 0;
        bit_count_ = 0;
    }

    size_t position() const { return position_; }

private:
    const Bytes& bytes_;
    size_t position_ = 0;
    unsigned __int128 acc_ = 0;
    unsigned bit_count_ = 0;
};

struct BodyStats {
    uint64_t common_structure_metadata_bytes = 0;
    uint64_t residual_bytes = 0;
    uint64_t leaf_descriptor_and_detection_bytes = 0;
    uint64_t root_table_bytes = 0;
    uint64_t overlay_table_bytes = 0;
    uint64_t entry_id_bytes = 0;
    uint64_t escape_bytes = 0;
    uint64_t other_charged_bytes = 0;
    uint64_t component_sum_bytes = 0;
    uint64_t leaf_count = 0;
    uint64_t root_entry_count = 0;
    uint64_t overlay_page_count = 0;
    uint64_t overlay_entry_count = 0;
    uint64_t escape_count = 0;

    uint64_t sum() const {
        return common_structure_metadata_bytes + residual_bytes +
               leaf_descriptor_and_detection_bytes + root_table_bytes +
               overlay_table_bytes + entry_id_bytes + escape_bytes +
               other_charged_bytes;
    }
};

struct BuiltBody {
    Bytes body;
    BodyStats stats;
    uint64_t outer_envelope_bytes = 0;
    uint32_t page_policy = 0;
    uint8_t leaf_family = 0;
    double build_ms = 0.0;
    std::string token_order_sha256;
    std::string token_multiset_sha256;
    std::string dictionary_plan_sha256;
    std::string table_content_sha256;
    std::vector<size_t> dictionary_id_offsets;
    std::vector<size_t> page_policy_offsets;
    std::vector<size_t> root_count_offsets;
    std::vector<size_t> root_entry_length_offsets;
    std::vector<size_t> root_entry_data_offsets;
    std::vector<size_t> root_entry_lengths;
    std::vector<size_t> page_width_offsets;
    size_t stream_offset = 0;
    uint8_t first_code_width = 0;
    size_t first_escape_length_offset = std::numeric_limits<size_t>::max();
};

static std::string token_order_hash(const G5DPlan& plan, G5DOrder order) {
    Bytes bytes;
    for (const TokenCoord& coord : coordinates_for(plan, order)) {
        const Bytes& token = token_at(plan, coord);
        put_uvar(bytes, token.size());
        bytes.insert(bytes.end(), token.begin(), token.end());
    }
    return sha256::hex(bytes);
}

static std::string ordered_multiset_sha256(const G5DPlan& plan, G5DOrder order) {
    std::vector<Bytes> records;
    const std::vector<TokenCoord>& coords = coordinates_for(plan, order);
    records.reserve(coords.size());
    for (const TokenCoord& coord : coords) {
        const Bytes& token = token_at(plan, coord);
        Bytes record;
        put_uvar(record, token.size());
        record.insert(record.end(), token.begin(), token.end());
        records.push_back(std::move(record));
    }
    std::sort(records.begin(), records.end());
    Bytes joined;
    for (const Bytes& record : records) joined.insert(joined.end(), record.begin(), record.end());
    return sha256::hex(joined);
}

static uint64_t source_length(const G5DPlan& plan) {
    uint64_t length = 0;
    for (const Frame& frame : plan.analysis.frames) {
        const uint64_t frame_length = frame.hi - frame.lo;
        if (length > std::numeric_limits<uint64_t>::max() - frame_length)
            throw std::runtime_error("G5D source length overflow");
        length += frame_length;
    }
    return length;
}

static BuiltBody build_rawlex_body(const G5DPlan& plan, G5DOrder order) {
    const auto build_start = std::chrono::steady_clock::now();
    BuiltBody built;
    built.body = plan.prefix;
    built.stats.common_structure_metadata_bytes = plan.common_structure_metadata_bytes;
    built.stats.residual_bytes = plan.residual_bytes;
    built.stats.leaf_count = plan.leaves.size();
    put_uvar(built.body, plan.leaves.size());
    built.stats.leaf_descriptor_and_detection_bytes += uvar_len(plan.leaves.size());
    for (size_t leaf_id = 0; leaf_id < plan.leaves.size(); ++leaf_id) {
        built.body.push_back(kG5DRawLexLeaf);
        put_uvar(built.body, 0);
        built.stats.leaf_descriptor_and_detection_bytes += 1 + uvar_len(0);
    }
    for (const TokenCoord& coord : coordinates_for(plan, order)) {
        const Bytes& token = token_at(plan, coord);
        put_uvar(built.body, token.size());
        built.body.insert(built.body.end(), token.begin(), token.end());
        built.stats.other_charged_bytes += uvar_len(token.size()) + token.size();
    }
    built.outer_envelope_bytes = 1 + uvar_len(source_length(plan));
    built.leaf_family = kG5DRawLexLeaf;
    built.token_order_sha256 = token_order_hash(plan, order);
    built.token_multiset_sha256 = ordered_multiset_sha256(plan, order);
    built.dictionary_plan_sha256 = sha256::hex(Bytes{});
    built.table_content_sha256 = sha256::hex(Bytes{});
    built.stats.component_sum_bytes = built.stats.sum();
    if (built.stats.component_sum_bytes != built.body.size())
        throw std::runtime_error("G5D rawlex component accounting failure");
    const auto build_end = std::chrono::steady_clock::now();
    built.build_ms = std::chrono::duration<double, std::milli>(build_end - build_start).count();
    return built;
}

static BuiltBody build_dictionary_body(
    const G5DPlan& plan,
    const PolicyPlan& policy_plan,
    G5DOrder order)
{
    const auto build_start = std::chrono::steady_clock::now();
    BuiltBody built;
    built.body = plan.prefix;
    built.stats.common_structure_metadata_bytes = plan.common_structure_metadata_bytes;
    built.stats.residual_bytes = plan.residual_bytes;
    built.stats.leaf_count = plan.leaves.size();
    built.dictionary_plan_sha256 = policy_plan.dictionary_plan_sha256;
    built.page_policy = policy_plan.policy;
    built.leaf_family = kG5DPagedDictLeaf;
    built.table_content_sha256 = policy_plan.table_content_sha256;
    put_uvar(built.body, plan.leaves.size());
    built.stats.leaf_descriptor_and_detection_bytes += uvar_len(plan.leaves.size());
    built.dictionary_id_offsets.reserve(plan.leaves.size());
    built.page_policy_offsets.reserve(plan.leaves.size());
    built.root_count_offsets.reserve(plan.leaves.size());

    for (const LeafPlan& leaf : policy_plan.leaves) {
        built.body.push_back(kG5DPagedDictLeaf);
        put_uvar(built.body, leaf.payload.size());
        built.stats.leaf_descriptor_and_detection_bytes += 1 + uvar_len(leaf.payload.size());
        const size_t payload_start = built.body.size();
        built.dictionary_id_offsets.push_back(payload_start);
        built.page_policy_offsets.push_back(payload_start + uvar_len(leaf.index));
        built.root_count_offsets.push_back(payload_start + uvar_len(leaf.index) + 1);
        built.body.insert(built.body.end(), leaf.payload.begin(), leaf.payload.end());
        built.stats.leaf_descriptor_and_detection_bytes += leaf.internal_detection_bytes;
        built.stats.root_table_bytes += leaf.root_table_bytes;
        built.stats.overlay_table_bytes += leaf.overlay_table_bytes;
        built.stats.root_entry_count += leaf.root.size();
        built.stats.overlay_page_count += leaf.pages.size();
        for (const PagePlan& page : leaf.pages)
            built.stats.overlay_entry_count += page.overlay.size();
        built.stats.escape_count += leaf.escape_count;
    }

    struct PositionedCoord {
        TokenCoord coord;
        size_t leaf_id;
    };
    std::vector<PositionedCoord> positioned;
    positioned.reserve(coordinates_for(plan, order).size());
    for (const TokenCoord& coord : coordinates_for(plan, order)) {
        if (coord.shape >= plan.leaf_index.size() || coord.slot >= plan.leaf_index[coord.shape].size())
            throw std::runtime_error("G5D coordinate leaf out of range");
        positioned.push_back({coord, plan.leaf_index[coord.shape][coord.slot]});
    }

    built.stream_offset = built.body.size();
    BitWriter writer;
    Bytes escape_region;
    for (size_t position = 0; position < positioned.size(); ++position) {
        const PositionedCoord& item = positioned[position];
        const LeafPlan& leaf = policy_plan.leaves[item.leaf_id];
        const size_t page_id = item.coord.occurrence / leaf.policy;
        if (page_id >= leaf.pages.size()) throw std::runtime_error("G5D page id out of range");
        const PagePlan& page = leaf.pages[page_id];
        const size_t page_offset = item.coord.occurrence - page.begin;
        if (page_offset >= page.ids.size()) throw std::runtime_error("G5D page occurrence out of range");
        const uint64_t code = page.ids[page_offset];
        if (position == 0) built.first_code_width = page.code_width;
        writer.write(code, page.code_width);
        const uint64_t active = leaf.root.size() + page.overlay.size();
        if (code == active) {
            const Bytes& token = token_at(plan, item.coord);
            put_uvar(escape_region, token.size());
            escape_region.insert(escape_region.end(), token.begin(), token.end());
        }
    }
    writer.align_zero();
    built.body.insert(built.body.end(), writer.data().begin(), writer.data().end());
    built.stats.entry_id_bytes = writer.byte_count();
    if (!positioned.empty()) {
        const PositionedCoord& first = positioned.front();
        const LeafPlan& first_leaf = policy_plan.leaves[first.leaf_id];
        const PagePlan& first_page = first_leaf.pages[first.coord.occurrence / first_leaf.policy];
        if (first_page.ids[first.coord.occurrence - first_page.begin] ==
            first_leaf.root.size() + first_page.overlay.size())
            built.first_escape_length_offset = built.body.size();
    }
    built.body.insert(built.body.end(), escape_region.begin(), escape_region.end());
    built.stats.escape_bytes = escape_region.size();
    built.token_order_sha256 = token_order_hash(plan, order);
    built.token_multiset_sha256 = ordered_multiset_sha256(plan, order);
    built.outer_envelope_bytes = 1 + uvar_len(source_length(plan));
    built.stats.component_sum_bytes = built.stats.sum();
    if (built.stats.component_sum_bytes != built.body.size())
        throw std::runtime_error("G5D dictionary component accounting failure");

    size_t scan = built.stream_offset;
    for (size_t i = 0; i < policy_plan.leaves.size(); ++i) {
        const LeafPlan& leaf = policy_plan.leaves[i];
        scan = built.dictionary_id_offsets[i] + uvar_len(leaf.index) + 1;
        const size_t root_count_offset = scan;
        (void)root_count_offset;
        scan = built.root_count_offsets[i];
        scan += uvar_len(leaf.root.size());
        for (const Bytes& entry : leaf.root) {
            built.root_entry_length_offsets.push_back(scan);
            built.root_entry_data_offsets.push_back(scan + uvar_len(entry.size()));
            built.root_entry_lengths.push_back(entry.size());
            scan += uvar_len(entry.size()) + entry.size();
        }
        for (const PagePlan& page : leaf.pages) {
            scan += uvar_len(page.overlay.size());
            for (const Bytes& entry : page.overlay)
                scan += uvar_len(entry.size()) + entry.size();
            built.page_width_offsets.push_back(scan);
            ++scan;
        }
    }
    const auto build_end = std::chrono::steady_clock::now();
    built.build_ms = std::chrono::duration<double, std::milli>(build_end - build_start).count();
    return built;
}

struct G5DGroup {
    bool raw = false;
    uint32_t members = 0;
    uint32_t slots = 0;
    std::vector<Bytes> parts;
    std::vector<std::vector<Bytes>> columns;
    std::vector<Bytes> raw_bodies;
};

struct LeafRuntime {
    bool dictionary = false;
    LeafPlan plan;
};

struct LeafLocation {
    size_t group = 0;
    uint32_t slot = 0;
};

static LeafPlan parse_leaf_payload(
    const Bytes& payload, size_t leaf_index, size_t occurrences)
{
    if (occurrences == 0 || occurrences > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("G5D invalid dictionary occurrence count");
    size_t position = 0;
    const uint64_t dictionary_id = get_uvar(payload, position);
    if (dictionary_id != leaf_index) throw std::runtime_error("G5D dictionary id mismatch");
    if (position >= payload.size()) throw std::runtime_error("G5D missing page policy");
    const uint8_t policy_id = payload[position++];
    const uint32_t policy = page_policy(policy_id);
    const uint64_t root_count = get_uvar(payload, position);
    if (root_count > occurrences || root_count > payload.size())
        throw std::runtime_error("G5D root count out of range");
    LeafPlan leaf;
    leaf.index = leaf_index;
    leaf.policy_id = policy_id;
    leaf.policy = policy;
    leaf.payload = payload;
    leaf.internal_detection_bytes = uvar_len(dictionary_id) + 1;
    const size_t root_start = position;
    leaf.root.reserve(static_cast<size_t>(root_count));
    for (uint64_t i = 0; i < root_count; ++i) {
        const uint64_t length = get_uvar(payload, position);
        if (length == 0 || length > payload.size() - position)
            throw std::runtime_error("G5D invalid root entry");
        leaf.root.emplace_back(payload.begin() + position, payload.begin() + position + static_cast<size_t>(length));
        position += static_cast<size_t>(length);
    }
    leaf.root_table_bytes = position - root_start;
    std::vector<Bytes> root_sorted = leaf.root;
    std::sort(root_sorted.begin(), root_sorted.end());
    if (std::adjacent_find(root_sorted.begin(), root_sorted.end()) != root_sorted.end())
        throw std::runtime_error("G5D duplicate root entry");
    const size_t page_count = (occurrences + policy - 1) / policy;
    leaf.pages.resize(page_count);
    const size_t overlay_start = position;
    for (size_t page_id = 0; page_id < page_count; ++page_id) {
        PagePlan& page = leaf.pages[page_id];
        page.begin = page_id * policy;
        const size_t page_occurrences = std::min(occurrences, page.begin + policy) - page.begin;
        const uint64_t overlay_count = get_uvar(payload, position);
        if (overlay_count > page_occurrences || overlay_count > payload.size() - position)
            throw std::runtime_error("G5D overlay count out of range");
        page.overlay.reserve(static_cast<size_t>(overlay_count));
        for (uint64_t i = 0; i < overlay_count; ++i) {
            const uint64_t length = get_uvar(payload, position);
            if (length == 0 || length > payload.size() - position)
                throw std::runtime_error("G5D invalid overlay entry");
            page.overlay.emplace_back(payload.begin() + position, payload.begin() + position + static_cast<size_t>(length));
            position += static_cast<size_t>(length);
            if (std::binary_search(root_sorted.begin(), root_sorted.end(), page.overlay.back()))
                throw std::runtime_error("G5D root-overlay duplicate");
        }
        std::vector<Bytes> overlay_sorted = page.overlay;
        std::sort(overlay_sorted.begin(), overlay_sorted.end());
        if (std::adjacent_find(overlay_sorted.begin(), overlay_sorted.end()) != overlay_sorted.end())
            throw std::runtime_error("G5D duplicate overlay entry");
        if (position >= payload.size()) throw std::runtime_error("G5D missing page width");
        page.code_width = payload[position++];
        const uint64_t active = root_count + overlay_count;
        if (page.code_width != bit_width_u64(active))
            throw std::runtime_error("G5D page code width mismatch");
    }
    leaf.overlay_table_bytes = position - overlay_start;
    if (position != payload.size()) throw std::runtime_error("G5D leaf payload trailing bytes");
    return leaf;
}

static void validate_runtime_presence(
    const std::vector<G5DGroup>& groups,
    const std::vector<LeafRuntime>& leaves,
    const std::vector<LeafLocation>& locations)
{
    if (leaves.size() != locations.size()) throw std::runtime_error("G5D leaf location mismatch");
    for (size_t leaf_id = 0; leaf_id < leaves.size(); ++leaf_id) {
        const LeafLocation& location = locations[leaf_id];
        if (location.group >= groups.size() || location.slot >= groups[location.group].columns.size())
            throw std::runtime_error("G5D leaf location out of range");
        const std::vector<Bytes>& tokens = groups[location.group].columns[location.slot];
        std::vector<Bytes> sorted_tokens = tokens;
        std::sort(sorted_tokens.begin(), sorted_tokens.end());
        const LeafPlan& leaf = leaves[leaf_id].plan;
        for (const Bytes& entry : leaf.root) {
            if (!std::binary_search(sorted_tokens.begin(), sorted_tokens.end(), entry))
                throw std::runtime_error("G5D root entry absent from leaf");
        }
        for (const PagePlan& page : leaf.pages) {
            std::vector<Bytes> sorted_overlay = page.overlay;
            std::sort(sorted_overlay.begin(), sorted_overlay.end());
            for (const Bytes& entry : sorted_overlay) {
                bool found = false;
                const size_t end = std::min(tokens.size(), page.begin + leaf.policy);
                for (size_t i = page.begin; i < end; ++i) {
                    if (tokens[i] == entry) { found = true; break; }
                }
                if (!found) throw std::runtime_error("G5D overlay entry absent from page");
            }
        }
    }
}

static Bytes decode_body(const Bytes& body, G5DOrder order, bool dictionary) {
    if (!valid_order_selector(static_cast<uint8_t>(order)))
        throw std::runtime_error("G5D invalid decode order");
    if (body.size() < 5 || !std::equal(kG5DMagic.begin(), kG5DMagic.end(), body.begin()))
        throw std::runtime_error("G5D bad magic");
    size_t position = 4;
    if (body[position++] != kG5DVersion) throw std::runtime_error("G5D bad version");
    const uint64_t decoded_length = get_uvar(body, position);
    if (decoded_length == 0 || decoded_length > kMaxDecoded ||
        decoded_length > std::numeric_limits<size_t>::max())
        throw std::runtime_error("G5D bad decoded length");
    const uint64_t frame_count = get_uvar_bounded(body, position, decoded_length);
    if (frame_count == 0 || frame_count > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("G5D bad frame count");
    const uint64_t group_count = get_uvar_bounded(body, position, frame_count);
    if (group_count == 0 || group_count > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("G5D bad group count");
    std::vector<uint32_t> frame_group;
    frame_group.reserve(static_cast<size_t>(frame_count));
    for (uint64_t i = 0; i < frame_count; ++i) {
        const uint64_t group = get_uvar_bounded(body, position, group_count - 1);
        frame_group.push_back(static_cast<uint32_t>(group));
    }

    std::vector<G5DGroup> groups(static_cast<size_t>(group_count));
    uint64_t member_sum = 0;
    bool saw_raw = false;
    for (size_t group_id = 0; group_id < groups.size(); ++group_id) {
        if (position >= body.size()) throw std::runtime_error("G5D truncated group kind");
        const uint8_t kind = body[position++];
        if (kind > 1) throw std::runtime_error("G5D unknown group kind");
        const uint64_t members = get_uvar_bounded(body, position, frame_count);
        if (members == 0 || member_sum > frame_count - members)
            throw std::runtime_error("G5D bad member count");
        member_sum += members;
        G5DGroup& group = groups[group_id];
        group.raw = kind == 1;
        group.members = static_cast<uint32_t>(members);
        if (group.raw) {
            if (group_id != 0 || saw_raw) throw std::runtime_error("G5D invalid raw group placement");
            saw_raw = true;
        } else {
            const uint64_t slots = get_uvar_bounded(body, position, decoded_length);
            if (slots > std::numeric_limits<uint32_t>::max() || slots + 1 > decoded_length)
                throw std::runtime_error("G5D slot count out of range");
            group.slots = static_cast<uint32_t>(slots);
            if (group.members > std::numeric_limits<size_t>::max() / std::max<size_t>(1, group.slots))
                throw std::runtime_error("G5D row allocation overflow");
            if (slots && group.members > decoded_length / slots)
                throw std::runtime_error("G5D scalar allocation exceeds source");
            group.parts.reserve(static_cast<size_t>(group.slots) + 1);
            for (size_t i = 0; i <= group.slots; ++i) {
                const uint64_t length = get_uvar_bounded(body, position, decoded_length);
                if (length > body.size() - position) throw std::runtime_error("G5D template span");
                group.parts.emplace_back(body.begin() + position, body.begin() + position + static_cast<size_t>(length));
                position += static_cast<size_t>(length);
            }
            group.columns.assign(group.slots, std::vector<Bytes>(group.members));
        }
    }
    if (member_sum != frame_count) throw std::runtime_error("G5D member sum mismatch");

    for (G5DGroup& group : groups) {
        if (!group.raw) continue;
        group.raw_bodies.reserve(group.members);
        for (uint32_t i = 0; i < group.members; ++i) {
            const uint64_t length = get_uvar_bounded(body, position, decoded_length);
            if (length == 0 || length > body.size() - position)
                throw std::runtime_error("G5D invalid raw residual");
            group.raw_bodies.emplace_back(body.begin() + position, body.begin() + position + static_cast<size_t>(length));
            position += static_cast<size_t>(length);
        }
    }

    uint64_t expected_leaves = 0;
    for (const G5DGroup& group : groups) {
        if (!group.raw) expected_leaves += group.slots;
    }
    const uint64_t leaf_count = get_uvar_bounded(body, position, expected_leaves);
    std::vector<LeafRuntime> leaves;
    leaves.reserve(static_cast<size_t>(leaf_count));
    std::vector<LeafLocation> locations;
    locations.reserve(static_cast<size_t>(leaf_count));
    std::vector<size_t> leaf_base(groups.size(), std::numeric_limits<size_t>::max());
    for (size_t group_id = 0; group_id < groups.size(); ++group_id) {
        const G5DGroup& group = groups[group_id];
        if (group.raw) continue;
        leaf_base[group_id] = leaves.size();
        for (uint32_t slot = 0; slot < group.slots; ++slot) {
            if (position >= body.size()) throw std::runtime_error("G5D truncated leaf id");
            const uint8_t leaf_id = body[position++];
            const uint64_t payload_length = get_uvar_bounded(body, position, body.size() - position);
            if (payload_length > body.size() - position)
                throw std::runtime_error("G5D leaf payload span");
            const Bytes payload(body.begin() + position, body.begin() + position + static_cast<size_t>(payload_length));
            position += static_cast<size_t>(payload_length);
            LeafRuntime runtime;
            if (dictionary) {
                if (leaf_id != kG5DPagedDictLeaf) throw std::runtime_error("G5D wrong dictionary leaf id");
                runtime.dictionary = true;
                runtime.plan = parse_leaf_payload(payload, leaves.size(), group.members);
            } else {
                if (leaf_id != kG5DRawLexLeaf || payload_length != 0)
                    throw std::runtime_error("G5D wrong rawlex leaf descriptor");
                runtime.dictionary = false;
            }
            leaves.push_back(std::move(runtime));
            locations.push_back({group_id, slot});
        }
    }
    if (leaves.size() != expected_leaves) throw std::runtime_error("G5D leaf count mismatch");

    struct Destination {
        size_t group = 0;
        uint32_t slot = 0;
        uint32_t occurrence = 0;
        size_t leaf = 0;
    };
    std::vector<Destination> destinations;
    uint64_t token_count = 0;
    for (const G5DGroup& group : groups)
        if (!group.raw) token_count += uint64_t(group.slots) * group.members;
    if (token_count > std::numeric_limits<size_t>::max())
        throw std::runtime_error("G5D token destination count overflow");
    destinations.reserve(static_cast<size_t>(token_count));

    if (order == G5DOrder::SourceOrder) {
        std::vector<uint32_t> cursor(groups.size(), 0);
        for (uint32_t group_id : frame_group) {
            G5DGroup& group = groups[group_id];
            if (group.raw) continue;
            const uint32_t occurrence = cursor[group_id]++;
            if (occurrence >= group.members)
                throw std::runtime_error("G5D source occurrence overflow");
            for (uint32_t slot = 0; slot < group.slots; ++slot) {
                const size_t leaf = leaf_base[group_id] + slot;
                destinations.push_back({group_id, slot, occurrence, leaf});
            }
        }
        for (size_t group_id = 0; group_id < groups.size(); ++group_id) {
            const G5DGroup& group = groups[group_id];
            if (!group.raw && cursor[group_id] != group.members)
                throw std::runtime_error("G5D source group consumption mismatch");
        }
    } else {
        for (size_t group_id = 0; group_id < groups.size(); ++group_id) {
            const G5DGroup& group = groups[group_id];
            if (group.raw) continue;
            for (uint32_t slot = 0; slot < group.slots; ++slot) {
                const size_t leaf = leaf_base[group_id] + slot;
                for (uint32_t occurrence = 0; occurrence < group.members; ++occurrence)
                    destinations.push_back({
                        static_cast<uint32_t>(group_id), slot, occurrence, leaf
                    });
            }
        }
    }

    if (!dictionary) {
        for (const LeafRuntime& leaf : leaves)
            if (leaf.dictionary) throw std::runtime_error("G5D mixed rawlex/dictionary leaves");
        for (const Destination& destination : destinations) {
            const uint64_t length = get_uvar_bounded(body, position, decoded_length);
            if (length == 0 || length > body.size() - position)
                throw std::runtime_error("G5D invalid rawlex token");
            groups[destination.group].columns[destination.slot][destination.occurrence].assign(
                body.begin() + position, body.begin() + position + static_cast<size_t>(length));
            position += static_cast<size_t>(length);
        }
        if (position != body.size()) throw std::runtime_error("G5D rawlex trailing bytes");
    } else {
        for (const LeafRuntime& leaf : leaves)
            if (!leaf.dictionary) throw std::runtime_error("G5D mixed dictionary/rawlex leaves");
        BitReader reader(body, position);
        std::vector<size_t> escape_destinations;
        for (size_t destination_id = 0; destination_id < destinations.size(); ++destination_id) {
            const Destination& destination = destinations[destination_id];
            const LeafPlan& leaf = leaves[destination.leaf].plan;
            const size_t page_id = destination.occurrence / leaf.policy;
            if (page_id >= leaf.pages.size())
                throw std::runtime_error("G5D decoded page out of range");
            const PagePlan& page = leaf.pages[page_id];
            const size_t page_offset = destination.occurrence - page.begin;
            const size_t page_occurrences =
                std::min<size_t>(leaf.policy, groups[destination.group].members - page.begin);
            if (page_offset >= page_occurrences)
                throw std::runtime_error("G5D decoded page occurrence out of range");
            const uint64_t code = reader.read(page.code_width);
            const uint64_t active = leaf.root.size() + page.overlay.size();
            if (code == active) {
                escape_destinations.push_back(destination_id);
            } else if (code < active) {
                Bytes value;
                if (code < leaf.root.size()) {
                    value = leaf.root[static_cast<size_t>(code)];
                } else {
                    const size_t overlay = static_cast<size_t>(code - leaf.root.size());
                    if (overlay >= page.overlay.size())
                        throw std::runtime_error("G5D overlay code out of range");
                    value = page.overlay[overlay];
                }
                if (value.empty()) throw std::runtime_error("G5D decoded empty dictionary value");
                groups[destination.group].columns[destination.slot][destination.occurrence] =
                    std::move(value);
            } else {
                throw std::runtime_error("G5D entry code exceeds active alphabet");
            }
        }
        reader.align_zero();
        position = reader.position();
        for (size_t destination_id : escape_destinations) {
            const uint64_t length = get_uvar_bounded(body, position, decoded_length);
            if (length == 0 || length > body.size() - position)
                throw std::runtime_error("G5D invalid escape token");
            const Destination& destination = destinations[destination_id];
            groups[destination.group].columns[destination.slot][destination.occurrence].assign(
                body.begin() + position, body.begin() + position + static_cast<size_t>(length));
            position += static_cast<size_t>(length);
        }
        if (position != body.size()) throw std::runtime_error("G5D dictionary trailing bytes");
        validate_runtime_presence(groups, leaves, locations);
    }

    std::vector<uint32_t> cursor(groups.size(), 0);
    Bytes output;
    output.reserve(static_cast<size_t>(decoded_length));
    for (uint32_t group_id : frame_group) {
        G5DGroup& group = groups[group_id];
        const uint32_t occurrence = cursor[group_id]++;
        if (occurrence >= group.members)
            throw std::runtime_error("G5D reconstruction occurrence overflow");
        if (group.raw) {
            const Bytes& residual = group.raw_bodies[occurrence];
            if (residual.size() > decoded_length - output.size())
                throw std::runtime_error("G5D residual reconstruction overflow");
            output.insert(output.end(), residual.begin(), residual.end());
        } else {
            if (group.parts.size() != size_t(group.slots) + 1)
                throw std::runtime_error("G5D reconstruction template count");
            for (uint32_t slot = 0; slot < group.slots; ++slot) {
                const Bytes& part = group.parts[slot];
                const Bytes& token = group.columns[slot][occurrence];
                if (part.size() > decoded_length - output.size() ||
                    token.size() > decoded_length - output.size() - part.size())
                    throw std::runtime_error("G5D structured reconstruction overflow");
                output.insert(output.end(), part.begin(), part.end());
                output.insert(output.end(), token.begin(), token.end());
            }
            const Bytes& tail = group.parts.back();
            if (tail.size() > decoded_length - output.size())
                throw std::runtime_error("G5D tail reconstruction overflow");
            output.insert(output.end(), tail.begin(), tail.end());
        }
    }
    for (size_t group_id = 0; group_id < groups.size(); ++group_id)
        if (cursor[group_id] != groups[group_id].members)
            throw std::runtime_error("G5D reconstruction group consumption");
    if (output.size() != decoded_length)
        throw std::runtime_error("G5D reconstructed length mismatch");
    return output;
}

static bool mode_pack_unpack(G5DOrder order, const Bytes& body) {
    Bytes packed;
    packed.push_back(static_cast<uint8_t>(order));
    packed.insert(packed.end(), body.begin(), body.end());
    if (packed.size() != body.size() + 1 || !valid_order_selector(packed.front())) return false;
    if (decode_order_selector(packed.front()) != order) return false;
    return Bytes(packed.begin() + 1, packed.end()) == body;
}

static uint64_t process_peak_rss_kib() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS info{};
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info)))
        throw std::runtime_error("G5D GetProcessMemoryInfo failed");
    return static_cast<uint64_t>(info.PeakWorkingSetSize) / 1024ull;
#elif defined(__APPLE__)
    struct rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0)
        throw std::runtime_error("G5D getrusage failed");
    return static_cast<uint64_t>(usage.ru_maxrss) / 1024ull;
#else
    struct rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0)
        throw std::runtime_error("G5D getrusage failed");
    return static_cast<uint64_t>(usage.ru_maxrss);
#endif
}

static const char* peak_rss_status() {
#if defined(_WIN32)
    return "MEASURED_WINDOWS_PEAK_WORKING_SET_AT_DECODE_COMPLETION";
#elif defined(__APPLE__)
    return "MEASURED_DARWIN_MAXRSS_AT_DECODE_COMPLETION";
#else
    return "MEASURED_LINUX_MAXRSS_AT_DECODE_COMPLETION";
#endif
}

struct MeasuredBody {
    BuiltBody built;
    Bytes brotli;
    uint64_t complete_bytes = 0;
    bool roundtrip = false;
    bool pack_unpack = false;
    double encode_ms = 0.0;
    double decode_ms = 0.0;
    uint64_t decode_peak_rss_kib = 0;
    std::string decode_peak_rss_status;
    uint32_t brotli_encoder_version = 0;
};

static MeasuredBody measure_body(
    const Bytes& source, G5DOrder order, bool dictionary, BuiltBody built)
{
    MeasuredBody measured;
    measured.built = std::move(built);
    const auto encode_start = std::chrono::steady_clock::now();
    measured.brotli = brotli_encode(measured.built.body);
    const auto encode_end = std::chrono::steady_clock::now();
    measured.encode_ms = std::chrono::duration<double, std::milli>(encode_end - encode_start).count();
    measured.complete_bytes = measured.built.outer_envelope_bytes + measured.brotli.size();
    measured.pack_unpack = mode_pack_unpack(order, measured.built.body);
    if (!measured.pack_unpack) throw std::runtime_error("G5D mode pack/unpack failure");
    const auto decode_start = std::chrono::steady_clock::now();
    const Bytes decoded_body = brotli_decode_exact(measured.brotli, measured.built.body.size());
    if (decoded_body != measured.built.body) throw std::runtime_error("G5D Brotli body mismatch");
    const Bytes decoded = decode_body(decoded_body, order, dictionary);
    const auto decode_end = std::chrono::steady_clock::now();
    measured.decode_ms = std::chrono::duration<double, std::milli>(decode_end - decode_start).count();
    measured.decode_peak_rss_kib = process_peak_rss_kib();
    measured.decode_peak_rss_status = peak_rss_status();
    measured.brotli_encoder_version = BrotliEncoderVersion();
    measured.roundtrip = decoded == source;
    if (!measured.roundtrip) throw std::runtime_error("G5D source roundtrip failure");
    return measured;
}

static std::string env_string(const char* name) {
#if defined(_WIN32)
    char* value = nullptr;
    size_t length = 0;
    if (_dupenv_s(&value, &length, name) != 0 || value == nullptr)
        return std::string();
    std::string result(value);
    std::free(value);
    return result;
#else
    const char* value = std::getenv(name);
    return value ? value : "";
#endif
}

static std::string brotli_version_dotted() {
    const uint32_t version = BrotliEncoderVersion();
    return std::to_string((version >> 24) & 0xffu) + "." +
           std::to_string((version >> 12) & 0xfffu) + "." +
           std::to_string(version & 0xfffu);
}

static void print_measured_arm(const MeasuredBody& measured) {
    const BuiltBody& built = measured.built;
    const BodyStats& stats = built.stats;
    std::cout << "{"
              << "\"page_policy\":" << built.page_policy
              << ",\"complete_bytes\":" << measured.complete_bytes
              << ",\"outer_envelope_bytes\":" << built.outer_envelope_bytes
              << ",\"carrier_body_bytes\":" << built.body.size()
              << ",\"brotli_payload_bytes\":" << measured.brotli.size()
              << ",\"common_structure_metadata_bytes\":" << stats.common_structure_metadata_bytes
              << ",\"residual_bytes\":" << stats.residual_bytes
              << ",\"leaf_descriptor_and_detection_bytes\":" << stats.leaf_descriptor_and_detection_bytes
              << ",\"root_table_bytes\":" << stats.root_table_bytes
              << ",\"overlay_table_bytes\":" << stats.overlay_table_bytes
              << ",\"entry_id_bytes\":" << stats.entry_id_bytes
              << ",\"escape_bytes\":" << stats.escape_bytes
              << ",\"other_charged_bytes\":" << stats.other_charged_bytes
              << ",\"component_sum_bytes\":" << stats.component_sum_bytes
              << ",\"leaf_count\":" << stats.leaf_count
              << ",\"root_entry_count\":" << stats.root_entry_count
              << ",\"overlay_page_count\":" << stats.overlay_page_count
              << ",\"overlay_entry_count\":" << stats.overlay_entry_count
              << ",\"escape_count\":" << stats.escape_count
              << ",\"dictionary_plan_sha256\":\"" << built.dictionary_plan_sha256 << "\""
              << ",\"table_content_sha256\":\"" << built.table_content_sha256 << "\""
              << ",\"token_order_sha256\":\"" << built.token_order_sha256 << "\""
              << ",\"build_ms\":" << built.build_ms
              << ",\"encode_ms\":" << measured.encode_ms
              << ",\"decode_ms\":" << measured.decode_ms
              << ",\"decode_peak_rss_kib\":" << measured.decode_peak_rss_kib
              << ",\"decode_peak_rss_kib_status\":\"" << measured.decode_peak_rss_status << "\""
              << ",\"roundtrip\":" << (measured.roundtrip ? "true" : "false")
              << ",\"pack_unpack_ok\":" << (measured.pack_unpack ? "true" : "false")
              << "}";
}

static int measure_file(const std::string& path) {
    const Bytes source = read_file(path);
    const auto plan_start = std::chrono::steady_clock::now();
    const G5DPlan plan = build_plan(source);
    const auto plan_end = std::chrono::steady_clock::now();
    const double plan_ms = std::chrono::duration<double, std::milli>(plan_end - plan_start).count();

    std::array<PolicyPlan, 3> policies;
    for (uint8_t i = 0; i < kPagePolicies.size(); ++i) policies[i] = build_policy_plan(plan, i);

    const std::array<G5DOrder, 2> orders = {G5DOrder::SourceOrder, G5DOrder::ShapeColumn};
    std::map<std::string, MeasuredBody> rawlex;
    std::map<std::string, MeasuredBody> dictionary;
    for (G5DOrder order : orders) {
        const std::string order_key = order_name(order);
        rawlex[order_key] = measure_body(source, order, false, build_rawlex_body(plan, order));
        for (size_t i = 0; i < policies.size(); ++i) {
            dictionary[order_key + ":" + std::to_string(policies[i].policy)] =
                measure_body(source, order, true, build_dictionary_body(plan, policies[i], order));
        }
    }

    std::set<std::string> token_multisets;
    std::set<uint32_t> backend_versions;
    bool all_accounting = true;
    bool allowed_families = true;
    auto accumulate_arm = [&](const MeasuredBody& measured) {
        token_multisets.insert(measured.built.token_multiset_sha256);
        backend_versions.insert(measured.brotli_encoder_version);
        all_accounting = all_accounting &&
            measured.built.stats.component_sum_bytes == measured.built.body.size();
        allowed_families = allowed_families &&
            (measured.built.leaf_family == kG5DRawLexLeaf ||
             measured.built.leaf_family == kG5DPagedDictLeaf);
    };
    for (const auto& entry : rawlex) accumulate_arm(entry.second);
    for (const auto& entry : dictionary) accumulate_arm(entry.second);
    const bool same_token_multiset = token_multisets.size() == 1;
    const bool fixed_backend = backend_versions.size() == 1 &&
        *backend_versions.begin() == BrotliEncoderVersion();
    const bool no_excluded_family = allowed_families;
    const bool no_production_authorization =
        env_string("G5D_PRODUCTION_TRANSFORM_ID").empty();

    bool same_plan = true;
    bool same_table = true;
    bool all_roundtrip = true;
    bool all_pack = true;
    for (G5DOrder order : orders) {
        const std::string order_key = order_name(order);
        for (size_t i = 0; i < policies.size(); ++i) {
            const MeasuredBody& left = dictionary.at(order_key + ":" + std::to_string(policies[i].policy));
            const G5DOrder other = order == G5DOrder::SourceOrder
                ? G5DOrder::ShapeColumn : G5DOrder::SourceOrder;
            const MeasuredBody& right = dictionary.at(std::string(order_name(other)) + ":" +
                                                std::to_string(policies[i].policy));
            same_plan = same_plan && left.built.dictionary_plan_sha256 == right.built.dictionary_plan_sha256;
            same_table = same_table && left.built.table_content_sha256 == right.built.table_content_sha256;
            all_roundtrip = all_roundtrip && left.roundtrip;
            all_pack = all_pack && left.pack_unpack;
        }
        all_roundtrip = all_roundtrip && rawlex.at(order_key).roundtrip;
        all_pack = all_pack && rawlex.at(order_key).pack_unpack;
    }

    const Bytes raw_brotli = brotli_encode(source);
    const uint64_t raw_complete = 1 + uvar_len(source.size()) + raw_brotli.size();
    const std::string filename = path.substr(path.find_last_of("/\\") + 1);
    const std::string role = filename.rfind("V1", 0) == 0
        ? "known-stress-not-held-out" : "discovery";

    std::cout << "{"
              << "\"schema\":1"
              << ",\"experiment\":\"G5D-PAGED-DICTIONARY\""
              << ",\"file\":\"" << json_escape(path) << "\""
              << ",\"file_role\":\"" << role << "\""
              << ",\"source_bytes\":" << source.size()
              << ",\"source_sha256\":\"" << sha256::hex(source) << "\""
              << ",\"implementation_sha\":\"" << json_escape(env_string("FROZEN_IMPLEMENTATION_SHA")) << "\""
              << ",\"frozen_g3_sha\":\"1a3d18fed76adb6fb33264e1994f9c357306b3fa\""
              << ",\"g5a_results_sha\":\"" << json_escape(env_string("G5A_RESULTS_SHA")) << "\""
              << ",\"g5a_results_sha256\":\"" << json_escape(env_string("G5A_RESULTS_SHA256")) << "\""
              << ",\"raw_brotli_bytes\":" << raw_brotli.size()
              << ",\"raw_brotli_complete_bytes\":" << raw_complete
              << ",\"carrier_quality\":11"
              << ",\"carrier_window\":30"
              << ",\"brotli_encoder_version\":" << BrotliEncoderVersion()
              << ",\"brotli_encoder_version_dotted\":\"" << brotli_version_dotted() << "\""
              << ",\"plan_ms\":" << plan_ms
              << ",\"frame_count\":" << plan.analysis.frames.size()
              << ",\"structured_frame_count\":" << plan.analysis.structured_frame_count
              << ",\"raw_frame_count\":" << plan.analysis.raw_frame_count
              << ",\"shape_count\":" << plan.analysis.shapes.size()
              << ",\"leaf_count\":" << plan.leaves.size()
              << ",\"source_token_multiset_sha256\":\"" << plan.source_token_multiset_sha256 << "\""
              << ",\"page_policies\":[4096,16384,65536]"
              << ",\"invariants\":{"
              << "\"exact_roundtrip_all\":" << (all_roundtrip ? "true" : "false")
              << ",\"same_source_token_multiset\":" << (same_token_multiset ? "true" : "false")
              << ",\"same_dictionary_plan_across_orders\":" << (same_plan ? "true" : "false")
              << ",\"same_table_content_across_orders\":" << (same_table ? "true" : "false")
              << ",\"exact_reconstruction_order\":" << (all_roundtrip ? "true" : "false")
              << ",\"complete_component_accounting\":" << (all_accounting ? "true" : "false")
              << ",\"fixed_backend\":" << (fixed_backend ? "true" : "false")
              << ",\"mode_pack_unpack\":" << (all_pack ? "true" : "false")
              << ",\"no_excluded_family\":" << (no_excluded_family ? "true" : "false")
              << ",\"no_production_authorization\":" << (no_production_authorization ? "true" : "false")
              << "},\"orders\":{";

    bool first_order = true;
    for (G5DOrder order : orders) {
        if (!first_order) std::cout << ",";
        first_order = false;
        const std::string order_key = order_name(order);
        std::cout << "\"" << order_key << "\":{"
                  << "\"mode_byte\":" << static_cast<unsigned>(order)
                  << ",\"rawlex_control\":";
        print_measured_arm(rawlex.at(order_key));
        std::cout << ",\"arms\":{";
        for (size_t i = 0; i < policies.size(); ++i) {
            if (i) std::cout << ",";
            const std::string policy_key = std::to_string(policies[i].policy);
            std::cout << "\"" << policy_key << "\":";
            print_measured_arm(dictionary.at(order_key + ":" + policy_key));
        }
        std::cout << "}}";
    }
    std::cout << "}}\n";
    return 0;
}

static void fixture(const std::string& text, const char* label, bool compress) {
    const Bytes source = bytes(text);
    const G5DPlan plan = build_plan(source);
    std::array<PolicyPlan, 3> policies;
    for (uint8_t i = 0; i < kPagePolicies.size(); ++i) policies[i] = build_policy_plan(plan, i);
    for (G5DOrder order : {G5DOrder::SourceOrder, G5DOrder::ShapeColumn}) {
        const BuiltBody rawlex = build_rawlex_body(plan, order);
        if (decode_body(rawlex.body, order, false) != source)
            throw std::runtime_error(std::string(label) + ": rawlex roundtrip");
        if (!mode_pack_unpack(order, rawlex.body))
            throw std::runtime_error(std::string(label) + ": rawlex mode pack");
        for (const PolicyPlan& policy : policies) {
            const BuiltBody body = build_dictionary_body(plan, policy, order);
            if (decode_body(body.body, order, true) != source)
                throw std::runtime_error(std::string(label) + ": dictionary roundtrip");
            if (!mode_pack_unpack(order, body.body))
                throw std::runtime_error(std::string(label) + ": dictionary mode pack");
            if (body.stats.component_sum_bytes != body.body.size())
                throw std::runtime_error(std::string(label) + ": component accounting");
            if (compress) {
                const MeasuredBody measured = measure_body(source, order, true, body);
                if (!measured.roundtrip) throw std::runtime_error(std::string(label) + ": compressed roundtrip");
            }
        }
    }
}

static void page_boundary_fixture() {
    std::string text;
    text.reserve(80000);
    for (size_t i = 0; i <= 4096; ++i) {
        text += "{\"v\":\"unique-";
        text += std::to_string(i);
        text += "\"}\n";
    }
    const Bytes source = bytes(text);
    const G5DPlan plan = build_plan(source);
    if (plan.leaves.size() != 1) throw std::runtime_error("page fixture leaf count");
    const std::vector<Bytes>& tokens = plan.analysis.shapes[0].slots[0].tokens;
    const LeafPlan forced = build_leaf_plan(tokens, 0, 0, 0);
    if (forced.pages.size() != 2) throw std::runtime_error("page boundary count");
    const PolicyPlan policy = build_policy_plan(plan, 0);
    if (policy.leaves.at(0).pages.size() != 2) throw std::runtime_error("P4K page policy");
    for (G5DOrder order : {G5DOrder::SourceOrder, G5DOrder::ShapeColumn}) {
        const BuiltBody body = build_dictionary_body(plan, policy, order);
        if (decode_body(body.body, order, true) != source)
            throw std::runtime_error("page boundary roundtrip");
    }
}

static void overlay_fixture() {
    std::string text;
    text.reserve(80000);
    for (size_t symbol = 0; symbol < 20; ++symbol) {
        for (size_t occurrence = 0; occurrence < 100; ++occurrence) {
            text += "{\"v\":\"local-";
            text += std::to_string(symbol);
            text += "\"}\n";
        }
    }
    for (size_t i = 2000; i <= 4096; ++i) {
        text += "{\"v\":\"unique-";
        text += std::to_string(i);
        text += "\"}\n";
    }
    const Bytes source = bytes(text);
    const G5DPlan plan = build_plan(source);
    const std::vector<Bytes>& tokens = plan.analysis.shapes[0].slots[0].tokens;
    const LeafPlan leaf = build_leaf_plan(tokens, 0, 0, 0);
    bool found_overlay = false;
    for (const PagePlan& page : leaf.pages) found_overlay = found_overlay || !page.overlay.empty();
    if (!found_overlay) throw std::runtime_error("forced overlay fixture selected no overlay");
}

static void malformed_fixture() {
    const std::string repeated_x(64, 'x');
    const std::string repeated_y(64, 'y');
    std::string malformed_text;
    for (size_t i = 0; i < 4; ++i) {
        malformed_text += "{\"v\":\"";
        malformed_text += (i & 1) == 0 ? repeated_x : repeated_y;
        malformed_text += "\"}\n";
    }
    const Bytes source = bytes(malformed_text);
    const G5DPlan plan = build_plan(source);
    PolicyPlan policy;
    policy.policy = kPagePolicies[0];
    for (size_t leaf_id = 0; leaf_id < plan.leaves.size(); ++leaf_id) {
        const LeafCoord& leaf = plan.leaves[leaf_id];
        policy.leaves.push_back(build_leaf_plan(
            plan.analysis.shapes[leaf.shape].slots[leaf.slot].tokens,
            leaf_id, 0, 2));
    }
    const BuiltBody original = build_dictionary_body(plan, policy, G5DOrder::SourceOrder);
    if (decode_body(original.body, G5DOrder::SourceOrder, true) != source)
        throw std::runtime_error("malformed fixture base roundtrip");

    auto rejects = [&](Bytes bad, const char* reason) {
        require_throw([&] { (void)decode_body(bad, G5DOrder::SourceOrder, true); }, reason);
    };
    Bytes bad = original.body;
    bad[0] ^= 1;
    rejects(std::move(bad), "bad magic");
    bad = original.body;
    bad[4] ^= 1;
    rejects(std::move(bad), "bad version");
    bad = original.body;
    bad.pop_back();
    rejects(std::move(bad), "truncated body");
    bad = original.body;
    bad.push_back(0);
    rejects(std::move(bad), "trailing body");
    bad = original.body;
    if (original.dictionary_id_offsets.empty()) throw std::runtime_error("missing dict offset");
    bad[original.dictionary_id_offsets[0]] = 1;
    rejects(std::move(bad), "dictionary id mismatch");
    bad = original.body;
    if (original.page_policy_offsets.empty()) throw std::runtime_error("missing policy offset");
    bad[original.page_policy_offsets[0]] = 3;
    rejects(std::move(bad), "page policy id");
    bad = original.body;
    if (original.page_width_offsets.empty()) throw std::runtime_error("missing width offset");
    bad[original.page_width_offsets[0]] = 65;
    rejects(std::move(bad), "page width");
    bad = original.body;
    if (original.root_count_offsets.empty()) throw std::runtime_error("missing root count offset");
    bad[original.root_count_offsets[0]] = 127;
    rejects(std::move(bad), "root count");
    bad = original.body;
    if (original.root_entry_length_offsets.empty()) throw std::runtime_error("missing root length offset");
    bad[original.root_entry_length_offsets[0]] = 0;
    rejects(std::move(bad), "empty root entry");
    if (original.root_entry_data_offsets.size() >= 2 &&
        original.root_entry_lengths[0] == original.root_entry_lengths[1]) {
        bad = original.body;
        const size_t source_offset = original.root_entry_data_offsets[0];
        const size_t target_offset = original.root_entry_data_offsets[1];
        std::copy_n(bad.begin() + source_offset, original.root_entry_lengths[0],
                    bad.begin() + target_offset);
        rejects(std::move(bad), "duplicate root entry");
    }
    if (original.first_code_width >= 2 && original.stream_offset < bad.size()) {
        bad = original.body;
        const uint8_t mask = static_cast<uint8_t>((1u << original.first_code_width) - 1u);
        bad[original.stream_offset] = static_cast<uint8_t>(bad[original.stream_offset] | mask);
        rejects(std::move(bad), "out-of-range code");
    }
    if (valid_order_selector(2))
        throw std::runtime_error("invalid order selector accepted");

    const Bytes distinct = bytes(
        "{\"v\":\"a\"}\n"
        "{\"v\":\"b\"}\n"
        "{\"v\":\"c\"}\n");
    const G5DPlan distinct_plan = build_plan(distinct);
    PolicyPlan distinct_policy;
    distinct_policy.policy = kPagePolicies[0];
    for (size_t leaf_id = 0; leaf_id < distinct_plan.leaves.size(); ++leaf_id) {
        const LeafCoord& leaf = distinct_plan.leaves[leaf_id];
        distinct_policy.leaves.push_back(build_leaf_plan(
            distinct_plan.analysis.shapes[leaf.shape].slots[leaf.slot].tokens,
            leaf_id, 0, 0));
    }
    const BuiltBody distinct_body = build_dictionary_body(
        distinct_plan, distinct_policy, G5DOrder::SourceOrder);
    if (distinct_body.first_escape_length_offset != std::numeric_limits<size_t>::max()) {
        bad = distinct_body.body;
        bad[distinct_body.first_escape_length_offset] = 0;
        rejects(std::move(bad), "zero escape length");
    }
}

static void selftest_g5d() {
    selftest();
    fixture(
        "{\"a\":1,\"b\":\"x\"}\n"
        "{\"a\":2,\"b\":\"y\"}\n"
        "{\"a\":1,\"b\":\"x\"}\n",
        "valid LF", true);
    fixture(
        "{\"a\":1,\"b\":\"x\"}\r\n"
        "{\"a\":2,\"b\":\"y\"}\r\n",
        "valid CRLF", true);
    fixture(
        "{\"a\":1}\n"
        "bad\n"
        "{\"a\":2}\n",
        "raw residual", true);
    fixture("{}\n[]\n{}\n", "zero slots", true);
    fixture("not-json\nstill-not-json\n", "raw only", true);
    page_boundary_fixture();
    overlay_fixture();
    malformed_fixture();
    if (sha256::hex(bytes("")) !=
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")
        throw std::runtime_error("SHA-256 empty KAT");
    if (sha256::hex(bytes("abc")) !=
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
        throw std::runtime_error("SHA-256 abc KAT");
    std::cout << "PASS grotli_g5_paged_dictionary selftest\n";
}

}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "selftest") {
            selftest_g5d();
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "measure") {
            return measure_file(argv[2]);
        }
        std::cerr << "usage: grotli_g5_paged_dictionary selftest\n"
                  << "       grotli_g5_paged_dictionary measure INPUT\n";
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << "\n";
        return 1;
    }
}
