// ANVIL I10 G5A ordering-attribution prototype.
//
// This experiment intentionally reuses the exact frozen G3 parser / frame / shape
// implementation in the same translation unit. G5A adds no leaf family and no
// planner. Its only causal variable is a permutation of identical exact lexical
// scalar chunks before the same Brotli backend.
//
// Preregistration:
//   docs/I10-GROTLI-G5A-ORDERING-ATTRIBUTION-PREREG.md
//
// IMPORTANT: D1-D4 / V1 measurements belong in GitHub Actions only. Local use is
// limited to compilation and tiny selftests.

#define main grotli_g3_embedded_main
#include "grotli_g3.cpp"
#undef main

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

enum class G5Order : uint8_t {
    SourceOrder = 0,
    ShapeRow = 1,
    ShapeColumn = 2,
};

static const char* order_name(G5Order m) {
    switch (m) {
        case G5Order::SourceOrder: return "SOURCE_ORDER";
        case G5Order::ShapeRow: return "SHAPE_ROW";
        case G5Order::ShapeColumn: return "SHAPE_COLUMN";
    }
    throw std::runtime_error("unknown G5 order");
}

static constexpr uint8_t kG5Version = 1;
static constexpr std::array<uint8_t, 4> kG5Magic = {'G','5','A','O'};

struct TokenCoord {
    uint32_t shape = 0;
    uint32_t occurrence = 0;
    uint32_t slot = 0;
};

struct G5Plan {
    RegionAnalysis analysis;
    Bytes prefix; // full common envelope + raw residuals; token stream begins after this.
    std::vector<TokenCoord> canonical; // canonical SOURCE_ORDER identity list.
    // [shape][occurrence][slot] -> canonical token identity.
    std::vector<std::vector<std::vector<size_t>>> canonical_id;
    uint64_t structured_token_bytes = 0;
};

static const Bytes& token_at(const G5Plan& p, const TokenCoord& c) {
    if (c.shape >= p.analysis.shapes.size())
        throw std::runtime_error("G5 token shape out of range");
    const ShapePlan& sh = p.analysis.shapes[c.shape];
    if (c.slot >= sh.slots.size())
        throw std::runtime_error("G5 token slot out of range");
    if (c.occurrence >= sh.slots[c.slot].tokens.size())
        throw std::runtime_error("G5 token occurrence out of range");
    return sh.slots[c.slot].tokens[c.occurrence];
}

static void append_token_chunk(Bytes& out, const Bytes& tok) {
    if (tok.empty()) throw std::runtime_error("G5 empty scalar token");
    put_uvar(out, tok.size());
    out.insert(out.end(), tok.begin(), tok.end());
}

static bool validate_permutation(const std::vector<size_t>& perm, size_t n) {
    if (perm.size() != n) return false;
    std::vector<uint8_t> seen(n, 0);
    for (size_t x : perm) {
        if (x >= n || seen[x]) return false;
        seen[x] = 1;
    }
    for (uint8_t x : seen) if (!x) return false;
    return true;
}

static G5Plan build_g5_plan(const Bytes& src) {
    if (src.empty()) throw std::runtime_error("G5 source is empty");

    G5Plan p;
    p.analysis = analyze_regions(src);
    RegionAnalysis& a = p.analysis;
    if (a.frames.empty()) throw std::runtime_error("G5 requires at least one frame");

    const bool has_raw = !a.raw_members.empty();
    const uint64_t group_count = a.shapes.size() + (has_raw ? 1u : 0u);
    if (group_count == 0) throw std::runtime_error("G5 has no groups");
    const uint32_t raw_gid = has_raw ? 0u : std::numeric_limits<uint32_t>::max();
    const uint32_t structured_offset = has_raw ? 1u : 0u;

    Bytes& out = p.prefix;
    out.insert(out.end(), kG5Magic.begin(), kG5Magic.end());
    out.push_back(kG5Version);
    put_uvar(out, src.size());
    put_uvar(out, a.frames.size());
    put_uvar(out, group_count);

    // Exact source reconstruction map. Identical in all G5A arms.
    for (const ParsedRecord& r : a.records) {
        if (r.structured && r.shape_id >= a.shapes.size())
            throw std::runtime_error("G5 record shape id out of range");
        const uint32_t gid = r.structured
            ? structured_offset + r.shape_id
            : raw_gid;
        put_uvar(out, gid);
    }

    // Group descriptors. Raw group first iff present, then exact shapes in
    // frozen first-appearance order. This is common-envelope metadata, not the
    // experimental ordering variable.
    if (has_raw) {
        out.push_back(1); // raw
        put_uvar(out, a.raw_members.size());
    }
    for (size_t sid = 0; sid < a.shapes.size(); ++sid) {
        const ShapePlan& sh = a.shapes[sid];
        if (sh.id != sid) throw std::runtime_error("G5 shape id ordering bug");
        const size_t slots = sh.parts.empty() ? 0 : sh.parts.size() - 1;
        if (sh.slots.size() != slots && !sh.members.empty())
            throw std::runtime_error("G5 shape slot/part mismatch");
        out.push_back(0); // structured
        put_uvar(out, sh.members.size());
        put_uvar(out, slots);
        put_uvar(out, sh.parts.size());
        for (const Bytes& part : sh.parts) {
            put_uvar(out, part.size());
            out.insert(out.end(), part.begin(), part.end());
        }
    }

    // Raw residual bytes are common and remain in exact frozen raw-member order.
    if (has_raw) {
        for (uint32_t fi : a.raw_members) {
            if (fi >= a.frames.size()) throw std::runtime_error("G5 raw frame id out of range");
            const Frame& f = a.frames[fi];
            if (f.hi < f.lo || f.hi > src.size())
                throw std::runtime_error("G5 raw frame span invalid");
            const uint64_t n = f.hi - f.lo;
            put_uvar(out, n);
            out.insert(out.end(), src.begin() + f.lo, src.begin() + f.hi);
        }
    }

    // Build a canonical token identity table in SOURCE_ORDER. Every alternative
    // arm is required to be an exact permutation of these identities.
    p.canonical_id.resize(a.shapes.size());
    for (size_t sid = 0; sid < a.shapes.size(); ++sid) {
        const ShapePlan& sh = a.shapes[sid];
        p.canonical_id[sid].resize(sh.members.size());
        for (auto& row : p.canonical_id[sid])
            row.resize(sh.slots.size(), std::numeric_limits<size_t>::max());
    }

    std::vector<size_t> next_occ(a.shapes.size(), 0);
    for (size_t fi = 0; fi < a.records.size(); ++fi) {
        const ParsedRecord& r = a.records[fi];
        if (!r.structured) continue;
        const size_t sid = r.shape_id;
        if (sid >= a.shapes.size()) throw std::runtime_error("G5 canonical sid");
        const ShapePlan& sh = a.shapes[sid];
        const size_t occ = next_occ[sid]++;
        if (occ >= sh.members.size() || sh.members[occ] != fi)
            throw std::runtime_error("G5 shape member/source-order mismatch");
        for (size_t slot = 0; slot < sh.slots.size(); ++slot) {
            if (occ >= sh.slots[slot].tokens.size())
                throw std::runtime_error("G5 missing structured token");
            const size_t id = p.canonical.size();
            if (id == std::numeric_limits<size_t>::max())
                throw std::runtime_error("G5 token id overflow");
            p.canonical.push_back(TokenCoord{
                static_cast<uint32_t>(sid),
                static_cast<uint32_t>(occ),
                static_cast<uint32_t>(slot)
            });
            p.canonical_id[sid][occ][slot] = id;
            p.structured_token_bytes += sh.slots[slot].tokens[occ].size();
        }
    }

    for (size_t sid = 0; sid < a.shapes.size(); ++sid) {
        if (next_occ[sid] != a.shapes[sid].members.size())
            throw std::runtime_error("G5 shape occurrence count mismatch");
        for (const auto& row : p.canonical_id[sid])
            for (size_t id : row)
                if (id == std::numeric_limits<size_t>::max())
                    throw std::runtime_error("G5 unassigned canonical token");
    }

    return p;
}

static std::vector<size_t> permutation_for(const G5Plan& p, G5Order mode) {
    std::vector<size_t> perm;
    perm.reserve(p.canonical.size());

    if (mode == G5Order::SourceOrder) {
        for (size_t i = 0; i < p.canonical.size(); ++i) perm.push_back(i);
    } else if (mode == G5Order::ShapeRow) {
        for (size_t sid = 0; sid < p.analysis.shapes.size(); ++sid) {
            const ShapePlan& sh = p.analysis.shapes[sid];
            for (size_t occ = 0; occ < sh.members.size(); ++occ)
                for (size_t slot = 0; slot < sh.slots.size(); ++slot)
                    perm.push_back(p.canonical_id[sid][occ][slot]);
        }
    } else if (mode == G5Order::ShapeColumn) {
        for (size_t sid = 0; sid < p.analysis.shapes.size(); ++sid) {
            const ShapePlan& sh = p.analysis.shapes[sid];
            for (size_t slot = 0; slot < sh.slots.size(); ++slot)
                for (size_t occ = 0; occ < sh.members.size(); ++occ)
                    perm.push_back(p.canonical_id[sid][occ][slot]);
        }
    } else {
        throw std::runtime_error("G5 unknown order");
    }

    if (!validate_permutation(perm, p.canonical.size()))
        throw std::runtime_error("G5 permutation coverage failure");
    return perm;
}

struct BuiltArm {
    G5Order mode = G5Order::SourceOrder;
    Bytes body;
    std::vector<size_t> permutation;
    bool permutation_ok = false;
    double build_ms = 0.0;
};

static BuiltArm build_arm(const G5Plan& p, G5Order mode) {
    const auto t0 = std::chrono::steady_clock::now();
    BuiltArm r;
    r.mode = mode;
    r.permutation = permutation_for(p, mode);
    r.permutation_ok = validate_permutation(r.permutation, p.canonical.size());
    if (!r.permutation_ok) throw std::runtime_error("G5 invalid permutation");

    r.body = p.prefix;
    for (size_t id : r.permutation) {
        const TokenCoord& c = p.canonical[id];
        append_token_chunk(r.body, token_at(p, c));
    }

    const auto t1 = std::chrono::steady_clock::now();
    r.build_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return r;
}

struct G5DGroup {
    bool raw = false;
    size_t members = 0;
    size_t slots = 0;
    std::vector<Bytes> parts;
    std::vector<Bytes> raw_bodies;
    std::vector<std::vector<Bytes>> rows; // [occurrence][slot]
};

static Bytes take_exact_chunk(
    const Bytes& body, size_t& pos, uint64_t decoded_len)
{
    const uint64_t n = get_uvar(body, pos);
    if (n == 0 || n > decoded_len || n > body.size() - pos)
        throw std::runtime_error("G5 bad structured token length");
    Bytes tok(body.begin() + pos, body.begin() + pos + static_cast<size_t>(n));
    pos += static_cast<size_t>(n);
    return tok;
}

static Bytes decode_g5_body(const Bytes& body, G5Order mode) {
    if (body.size() < 5 || !std::equal(kG5Magic.begin(), kG5Magic.end(), body.begin()))
        throw std::runtime_error("G5 bad magic");
    size_t pos = 4;
    if (body[pos++] != kG5Version) throw std::runtime_error("G5 bad version");

    const uint64_t decoded_len = get_uvar(body, pos);
    if (decoded_len == 0 || decoded_len > kMaxDecoded ||
        decoded_len > std::numeric_limits<size_t>::max())
        throw std::runtime_error("G5 bad decoded length");

    const uint64_t frame_count = get_uvar_bounded(body, pos, decoded_len);
    if (frame_count == 0 || frame_count > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("G5 bad frame count");

    const uint64_t group_count = get_uvar_bounded(body, pos, frame_count);
    if (group_count == 0 || group_count > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("G5 bad group count");

    std::vector<uint32_t> frame_group;
    frame_group.reserve(static_cast<size_t>(frame_count));
    for (uint64_t i = 0; i < frame_count; ++i) {
        const uint64_t gid = get_uvar_bounded(body, pos, group_count - 1);
        frame_group.push_back(static_cast<uint32_t>(gid));
    }

    std::vector<G5DGroup> groups(static_cast<size_t>(group_count));
    uint64_t member_sum = 0;
    size_t raw_groups = 0;
    for (size_t gid = 0; gid < groups.size(); ++gid) {
        if (pos >= body.size()) throw std::runtime_error("G5 truncated group kind");
        const uint8_t kind = body[pos++];
        if (kind > 1) throw std::runtime_error("G5 unknown group kind");

        const uint64_t members = get_uvar_bounded(body, pos, frame_count);
        if (members == 0) throw std::runtime_error("G5 zero-member group");
        if (member_sum > frame_count - members)
            throw std::runtime_error("G5 group member sum overflow");
        member_sum += members;

        G5DGroup& g = groups[gid];
        g.raw = kind == 1;
        g.members = static_cast<size_t>(members);
        if (g.raw) {
            ++raw_groups;
            if (raw_groups > 1) throw std::runtime_error("G5 multiple raw groups");
            continue;
        }

        const uint64_t slots = get_uvar_bounded(body, pos, decoded_len);
        const uint64_t part_count = get_uvar_bounded(body, pos, decoded_len);
        if (part_count != slots + 1)
            throw std::runtime_error("G5 template part count mismatch");
        if (slots > std::numeric_limits<size_t>::max() ||
            part_count > std::numeric_limits<size_t>::max())
            throw std::runtime_error("G5 template count width");

        g.slots = static_cast<size_t>(slots);
        g.parts.reserve(static_cast<size_t>(part_count));
        for (uint64_t i = 0; i < part_count; ++i) {
            const uint64_t n = get_uvar_bounded(body, pos, decoded_len);
            if (n > body.size() - pos) throw std::runtime_error("G5 template span");
            g.parts.emplace_back(
                body.begin() + pos,
                body.begin() + pos + static_cast<size_t>(n));
            pos += static_cast<size_t>(n);
        }
        if (g.members > std::numeric_limits<size_t>::max() / std::max<size_t>(1, g.slots))
            throw std::runtime_error("G5 row allocation overflow");
        g.rows.assign(g.members, std::vector<Bytes>(g.slots));
    }
    if (member_sum != frame_count)
        throw std::runtime_error("G5 group member sum mismatch");

    // Raw residual section is fixed/common and precedes the experimental stream.
    for (G5DGroup& g : groups) {
        if (!g.raw) continue;
        g.raw_bodies.reserve(g.members);
        for (size_t i = 0; i < g.members; ++i) {
            const uint64_t n = get_uvar_bounded(body, pos, decoded_len);
            if (n > body.size() - pos) throw std::runtime_error("G5 raw residual span");
            g.raw_bodies.emplace_back(
                body.begin() + pos,
                body.begin() + pos + static_cast<size_t>(n));
            pos += static_cast<size_t>(n);
        }
    }

    auto read_into = [&](G5DGroup& g, size_t occ, size_t slot) {
        if (g.raw || occ >= g.members || slot >= g.slots)
            throw std::runtime_error("G5 token destination out of range");
        g.rows[occ][slot] = take_exact_chunk(body, pos, decoded_len);
    };

    if (mode == G5Order::SourceOrder) {
        std::vector<size_t> occ(groups.size(), 0);
        for (uint32_t gid : frame_group) {
            G5DGroup& g = groups[gid];
            if (g.raw) continue;
            const size_t oi = occ[gid]++;
            if (oi >= g.members) throw std::runtime_error("G5 source order occurrence overflow");
            for (size_t slot = 0; slot < g.slots; ++slot) read_into(g, oi, slot);
        }
        for (size_t gid = 0; gid < groups.size(); ++gid)
            if (!groups[gid].raw && occ[gid] != groups[gid].members)
                throw std::runtime_error("G5 source order occurrence mismatch");
    } else if (mode == G5Order::ShapeRow) {
        for (G5DGroup& g : groups) {
            if (g.raw) continue;
            for (size_t occ = 0; occ < g.members; ++occ)
                for (size_t slot = 0; slot < g.slots; ++slot)
                    read_into(g, occ, slot);
        }
    } else if (mode == G5Order::ShapeColumn) {
        for (G5DGroup& g : groups) {
            if (g.raw) continue;
            for (size_t slot = 0; slot < g.slots; ++slot)
                for (size_t occ = 0; occ < g.members; ++occ)
                    read_into(g, occ, slot);
        }
    } else {
        throw std::runtime_error("G5 invalid decode order");
    }

    if (pos != body.size()) throw std::runtime_error("G5 trailing carrier bytes");

    Bytes out;
    out.reserve(static_cast<size_t>(decoded_len));
    std::vector<size_t> occ(groups.size(), 0);
    for (uint32_t gid : frame_group) {
        if (gid >= groups.size()) throw std::runtime_error("G5 reconstruction gid");
        G5DGroup& g = groups[gid];
        const size_t oi = occ[gid]++;
        if (oi >= g.members) throw std::runtime_error("G5 reconstruction occurrence");
        if (g.raw) {
            const Bytes& b = g.raw_bodies[oi];
            if (b.size() > decoded_len - out.size())
                throw std::runtime_error("G5 raw reconstruction exceeds bound");
            out.insert(out.end(), b.begin(), b.end());
        } else {
            if (g.parts.size() != g.slots + 1)
                throw std::runtime_error("G5 reconstruction template mismatch");
            for (size_t slot = 0; slot < g.slots; ++slot) {
                const Bytes& part = g.parts[slot];
                const Bytes& tok = g.rows[oi][slot];
                if (part.size() > decoded_len - out.size())
                    throw std::runtime_error("G5 template reconstruction exceeds bound");
                out.insert(out.end(), part.begin(), part.end());
                if (tok.size() > decoded_len - out.size())
                    throw std::runtime_error("G5 token reconstruction exceeds bound");
                out.insert(out.end(), tok.begin(), tok.end());
            }
            const Bytes& tail = g.parts.back();
            if (tail.size() > decoded_len - out.size())
                throw std::runtime_error("G5 tail reconstruction exceeds bound");
            out.insert(out.end(), tail.begin(), tail.end());
        }
    }

    if (out.size() != decoded_len) throw std::runtime_error("G5 decoded length mismatch");
    for (size_t gid = 0; gid < groups.size(); ++gid)
        if (occ[gid] != groups[gid].members)
            throw std::runtime_error("G5 reconstruction member mismatch");
    return out;
}

struct MeasuredArm {
    BuiltArm built;
    Bytes brotli;
    size_t complete_bytes = 0;
    bool roundtrip = false;
    double encode_ms = 0.0;
    double decode_ms = 0.0;
};

static MeasuredArm measure_arm(const G5Plan& p, const Bytes& src, G5Order mode) {
    MeasuredArm m;
    m.built = build_arm(p, mode);

    const auto e0 = std::chrono::steady_clock::now();
    m.brotli = brotli_encode(m.built.body);
    const auto e1 = std::chrono::steady_clock::now();
    m.encode_ms = std::chrono::duration<double, std::milli>(e1 - e0).count();
    m.complete_bytes = 1 + m.brotli.size(); // one charged outer order-mode byte

    const auto d0 = std::chrono::steady_clock::now();
    const Bytes decoded_body = brotli_decode_exact(m.brotli, m.built.body.size());
    if (decoded_body != m.built.body)
        throw std::runtime_error("G5 Brotli body roundtrip mismatch");
    const Bytes decoded_src = decode_g5_body(decoded_body, mode);
    const auto d1 = std::chrono::steady_clock::now();
    m.decode_ms = std::chrono::duration<double, std::milli>(d1 - d0).count();
    m.roundtrip = decoded_src == src;
    if (!m.roundtrip) throw std::runtime_error("G5 source roundtrip mismatch");
    return m;
}

static void print_arm_json(const char* key, const MeasuredArm& m) {
    std::cout << ",\"" << key << "\":{"
              << "\"mode\":\"" << order_name(m.built.mode) << "\""
              << ",\"body_bytes\":" << m.built.body.size()
              << ",\"brotli_bytes\":" << m.brotli.size()
              << ",\"complete_bytes\":" << m.complete_bytes
              << ",\"roundtrip\":" << (m.roundtrip ? "true" : "false")
              << ",\"permutation_ok\":" << (m.built.permutation_ok ? "true" : "false")
              << ",\"build_ms\":" << m.built.build_ms
              << ",\"encode_ms\":" << m.encode_ms
              << ",\"decode_ms\":" << m.decode_ms
              << "}";
}

static int measure_g5a(const std::string& path) {
    const Bytes src = read_file(path);
    const auto p0 = std::chrono::steady_clock::now();
    const G5Plan plan = build_g5_plan(src);
    const auto p1 = std::chrono::steady_clock::now();
    const double parse_plan_ms =
        std::chrono::duration<double, std::milli>(p1 - p0).count();

    const MeasuredArm a1 = measure_arm(plan, src, G5Order::SourceOrder);
    const MeasuredArm a2 = measure_arm(plan, src, G5Order::ShapeRow);
    const MeasuredArm a3 = measure_arm(plan, src, G5Order::ShapeColumn);

    const bool same_body_size =
        a1.built.body.size() == a2.built.body.size() &&
        a2.built.body.size() == a3.built.body.size();
    if (!same_body_size) throw std::runtime_error("G5 body-size identity failure");

    auto has_common_prefix = [&](const Bytes& body) {
        return body.size() >= plan.prefix.size() &&
               std::equal(plan.prefix.begin(), plan.prefix.end(), body.begin());
    };
    const bool envelope_identity =
        has_common_prefix(a1.built.body) &&
        has_common_prefix(a2.built.body) &&
        has_common_prefix(a3.built.body);
    if (!envelope_identity)
        throw std::runtime_error("G5 common-envelope identity failure");

    const bool all_perm =
        a1.built.permutation_ok && a2.built.permutation_ok && a3.built.permutation_ok;
    const bool all_roundtrip = a1.roundtrip && a2.roundtrip && a3.roundtrip;
    if (!all_perm || !all_roundtrip)
        throw std::runtime_error("G5 hard invariant failure");

    // Raw Brotli is context only and does not enter the A1/A2/A3 causal gate.
    const Bytes raw_br = brotli_encode(src);

    const int64_t shape_bytes =
        static_cast<int64_t>(a1.complete_bytes) - static_cast<int64_t>(a2.complete_bytes);
    const int64_t column_bytes =
        static_cast<int64_t>(a2.complete_bytes) - static_cast<int64_t>(a3.complete_bytes);
    const int64_t total_bytes =
        static_cast<int64_t>(a1.complete_bytes) - static_cast<int64_t>(a3.complete_bytes);

    std::cout << "{"
              << "\"schema\":1"
              << ",\"file\":\"" << json_escape(path) << "\""
              << ",\"source_bytes\":" << src.size()
              << ",\"raw_brotli_bytes\":" << raw_br.size()
              << ",\"raw_complete_bytes_context\":" << (raw_br.size() + 5)
              << ",\"parse_plan_ms\":" << parse_plan_ms
              << ",\"frame_count\":" << plan.analysis.frames.size()
              << ",\"structured_frame_count\":" << plan.analysis.structured_frame_count
              << ",\"raw_frame_count\":" << plan.analysis.raw_frame_count
              << ",\"shape_count\":" << plan.analysis.shapes.size()
              << ",\"token_chunk_count\":" << plan.canonical.size()
              << ",\"structured_token_bytes\":" << plan.structured_token_bytes
              << ",\"common_prefix_bytes\":" << plan.prefix.size()
              << ",\"body_size_identity\":" << (same_body_size ? "true" : "false")
              << ",\"envelope_identity\":" << (envelope_identity ? "true" : "false")
              << ",\"all_permutations_exact\":" << (all_perm ? "true" : "false")
              << ",\"all_roundtrip\":" << (all_roundtrip ? "true" : "false");
    print_arm_json("a1_source_order", a1);
    print_arm_json("a2_shape_row", a2);
    print_arm_json("a3_shape_column", a3);
    std::cout << ",\"shape_grouping_bytes\":" << shape_bytes
              << ",\"column_increment_bytes\":" << column_bytes
              << ",\"total_order_bytes\":" << total_bytes
              << "}\n";
    return 0;
}

static void g5a_fixture(const std::string& s, const char* label) {
    const Bytes src = bytes(s);
    const G5Plan p = build_g5_plan(src);
    const BuiltArm a1 = build_arm(p, G5Order::SourceOrder);
    const BuiltArm a2 = build_arm(p, G5Order::ShapeRow);
    const BuiltArm a3 = build_arm(p, G5Order::ShapeColumn);

    if (a1.body.size() != a2.body.size() || a2.body.size() != a3.body.size())
        throw std::runtime_error(std::string(label) + ": body sizes differ");
    auto has_prefix = [&](const Bytes& body) {
        return body.size() >= p.prefix.size() &&
               std::equal(p.prefix.begin(), p.prefix.end(), body.begin());
    };
    if (!has_prefix(a1.body) || !has_prefix(a2.body) || !has_prefix(a3.body))
        throw std::runtime_error(std::string(label) + ": common envelope differs");
    if (!a1.permutation_ok || !a2.permutation_ok || !a3.permutation_ok)
        throw std::runtime_error(std::string(label) + ": permutation invalid");
    if (decode_g5_body(a1.body, G5Order::SourceOrder) != src)
        throw std::runtime_error(std::string(label) + ": source decode");
    if (decode_g5_body(a2.body, G5Order::ShapeRow) != src)
        throw std::runtime_error(std::string(label) + ": shape-row decode");
    if (decode_g5_body(a3.body, G5Order::ShapeColumn) != src)
        throw std::runtime_error(std::string(label) + ": shape-column decode");

    const size_t expected = p.canonical.size();
    for (const auto* a : {&a1, &a2, &a3})
        if (!validate_permutation(a->permutation, expected))
            throw std::runtime_error(std::string(label) + ": exact coverage");
}

static void g5a_selftest() {
    // Re-run the complete frozen G3 selftest first. This is intentionally small
    // correctness work and ensures imported parser/shape semantics remain valid.
    selftest();

    g5a_fixture(
        "{\"a\":1,\"b\":\"x\"}\n"
        "{\"a\":2,\"b\":\"y\"}\n"
        "garbage\n"
        "{\"x\":true}\n"
        "{\"a\":3,\"b\":\"z\"}\n",
        "mixed shapes + residual");

    g5a_fixture(
        "{}\n[]\n{}\n"
        "{\"n\":1}\r\n"
        "{\"n\":2}\r\n",
        "zero scalar + CRLF");

    g5a_fixture(
        "{\"k\":\"same\",\"v\":100}\n"
        "{\"k\":\"same\",\"v\":101}\n"
        "{\"k\":\"same\",\"v\":102}\n",
        "single shape");

    g5a_fixture(
        "not-json\n"
        "still-not-json\n",
        "raw only");

    // Exact permutation validator must reject both duplicates and omissions.
    if (validate_permutation({0, 0}, 2))
        throw std::runtime_error("G5 permutation duplicate accepted");
    if (validate_permutation({0}, 2))
        throw std::runtime_error("G5 permutation omission accepted");
    if (!validate_permutation({}, 0))
        throw std::runtime_error("G5 empty permutation rejected");

    std::cout << "PASS grotli_g5a_ordering selftest\n";
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "selftest") {
            g5a_selftest();
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "measure") {
            return measure_g5a(argv[2]);
        }
        std::cerr << "usage: grotli_g5a_ordering selftest\n"
                     "       grotli_g5a_ordering measure INPUT\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
