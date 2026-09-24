// ANVIL I10 G5A ordering-attribution prototype.
//
// This experiment intentionally reuses the exact frozen G3 parser / frame / shape
// implementation in the same translation unit. G5A adds no leaf family and no
// planner. Its only causal variable is a permutation of identical exact lexical
// scalar chunks before the same Brotli backend.
//
// Preregistration:
//   docs/I10-GROTLI-G5A-ORDERING-ATTRIBUTION-PREREG.md  (freeze revision r6)
//
// FROZEN-INCLUSION (prereg section 2.1): CI compiles G5A against the materialized
// pinned frozen G3 source, NOT the mutable working-tree file, by defining
//     -DG5A_FROZEN_G3_HEADER=\"frozen-grotli_g3.cpp\"
// Local builds with no define fall back to the working-tree tools/grotli_g3.cpp.
//
// IMPORTANT: D1-D4 / V1 measurements belong in GitHub Actions only. Local use is
// limited to compilation and tiny selftests.

#ifndef G5A_FROZEN_G3_HEADER
#define G5A_FROZEN_G3_HEADER "grotli_g3.cpp"
#endif

#define main grotli_g3_embedded_main
#include G5A_FROZEN_G3_HEADER
#undef main

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Self-contained SHA-256 (prereg I3/I4 require byte-level multiset and envelope
// hashes). The frozen G3 TU provides none, so G5A carries its own.
// ---------------------------------------------------------------------------

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
        const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const uint32_t ch = (e & f) ^ (~e & g);
        const uint32_t t1 = h + S1 + ch + K[i] + w[i];
        const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const uint32_t maj = (a & b) ^ (a & cc) ^ (b & cc);
        const uint32_t t2 = S0 + maj;
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
    Ctx c; init(c); update(c, in.data(), in.size()); return final(c);
}

static std::string hex(const std::array<uint8_t, 32>& h) {
    static const char* d = "0123456789abcdef";
    std::string s;
    s.reserve(64);
    for (uint8_t b : h) { s += d[b >> 4]; s += d[b & 0xf]; }
    return s;
}

static std::string hex(const Bytes& in) { return hex(hash(in)); }

} // namespace sha256

enum class G5Order : uint8_t {
    RandomPermutation = 0,  // A0 deterministic null
    SourceOrder = 1,        // A1
    ShapeRow = 2,           // A2
    ShapeColumn = 3,        // A3
};

static const char* order_name(G5Order m) {
    switch (m) {
        case G5Order::RandomPermutation: return "RANDOM_PERMUTATION";
        case G5Order::SourceOrder: return "SOURCE_ORDER";
        case G5Order::ShapeRow: return "SHAPE_ROW";
        case G5Order::ShapeColumn: return "SHAPE_COLUMN";
    }
    throw std::runtime_error("unknown G5 order");
}

static constexpr uint8_t kG5Version = 1;
static constexpr std::array<uint8_t, 4> kG5Magic = {'G','5','A','O'};

// Single shared definition of the A0 null seed. BOTH the encoder
// (`permutation_for`) and the decoder (`decode_g5_body`) hash exactly these
// bytes; keeping one definition here prevents the two sites from drifting.
// Changing these bytes changes the frozen A0 null and is forbidden post-freeze.
static constexpr const char* kG5RandomPermutationSeed =
    "G5A-RANDOM-PERMUTATION-SEED-v1";

// The four frozen order selectors are exactly 0..3. Any other selector byte is
// malformed and must be rejected deterministically (prereg I7).
static bool is_valid_g5_selector(uint8_t s) {
    return s == static_cast<uint8_t>(G5Order::RandomPermutation) ||
           s == static_cast<uint8_t>(G5Order::SourceOrder) ||
           s == static_cast<uint8_t>(G5Order::ShapeRow) ||
           s == static_cast<uint8_t>(G5Order::ShapeColumn);
}

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
    // Byte-level invariant hashes (prereg I3/I4). Identical across all arms.
    std::string envelope_sha256;         // SHA-256 of `prefix`
    std::string multiset_sha256;         // SHA-256 of sorted (uvar(len)||bytes) records
};

static const Bytes& token_at(const G5Plan& p, const TokenCoord& c);

// Canonical token-multiset SHA-256 (prereg I3): sort the decode-visible framed
// records lexicographically ascending, then hash the concatenation with no
// separator. Order-independent, so it is identical for every permutation.
static std::string canonical_multiset_sha256(const G5Plan& p) {
    std::vector<Bytes> records;
    records.reserve(p.canonical.size());
    for (const TokenCoord& c : p.canonical) {
        Bytes rec;
        const Bytes& tok = token_at(p, c);
        put_uvar(rec, tok.size());
        rec.insert(rec.end(), tok.begin(), tok.end());
        records.push_back(std::move(rec));
    }
    std::sort(records.begin(), records.end());
    Bytes joined;
    for (const Bytes& r : records) joined.insert(joined.end(), r.begin(), r.end());
    return sha256::hex(joined);
}

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

    // Byte-level invariant hashes (prereg I3/I4), computed once per file.
    p.envelope_sha256 = sha256::hex(p.prefix);
    p.multiset_sha256 = canonical_multiset_sha256(p);

    return p;
}

static std::vector<size_t> permutation_for(const G5Plan& p, G5Order mode) {
    std::vector<size_t> perm;
    perm.reserve(p.canonical.size());

    if (mode == G5Order::RandomPermutation) {
        // A0 deterministic null (prereg section 4 A0 / 4.1): sort canonical
        // indices by SHA-256(seed || 0x1F || index_le_u64) as a big-endian
        // 256-bit integer, ties by lower index. No score, no content, no
        // observed byte influences this permutation.
        const char* const kSeed = kG5RandomPermutationSeed;
        std::vector<std::pair<std::array<uint8_t, 32>, size_t>> keyed;
        keyed.reserve(p.canonical.size());
        for (size_t i = 0; i < p.canonical.size(); ++i) {
            Bytes buf;
            for (const char* c = kSeed; *c; ++c) buf.push_back(uint8_t(*c));
            buf.push_back(0x1F);
            for (unsigned b = 0; b < 8; ++b)
                buf.push_back(uint8_t((uint64_t(i) >> (b * 8)) & 0xffu));
            keyed.emplace_back(sha256::hash(buf), i);
        }
        std::sort(keyed.begin(), keyed.end(),
                  [](const auto& x, const auto& y) {
                      if (x.first != y.first) return x.first < y.first;  // big-endian digest
                      return x.second < y.second;
                  });
        for (const auto& kv : keyed) perm.push_back(kv.second);
    } else if (mode == G5Order::SourceOrder) {
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
    // Byte-level invariant hashes (prereg I3/I4).
    std::string envelope_sha256;
    std::string multiset_sha256;
    // Strengthened I2 (r6): envelope and token-region lengths. Derived from the
    // existing exact prefix/body construction; body bytes are UNCHANGED.
    uint64_t envelope_len = 0;
    uint64_t token_region_len = 0;
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

    // Strengthened I2: envelope is exactly the shared prefix; the token region
    // is the remainder. These are pure derivations of the unchanged body.
    r.envelope_len = static_cast<uint64_t>(p.prefix.size());
    r.token_region_len = static_cast<uint64_t>(r.body.size()) - r.envelope_len;

    // Envelope hash is over the shared prefix; the multiset hash is over the
    // arm's own token region records (sorted), which must equal the plan hash.
    r.envelope_sha256 = sha256::hex(p.prefix);
    {
        std::vector<Bytes> records;
        records.reserve(r.permutation.size());
        for (size_t id : r.permutation) {
            Bytes rec;
            const Bytes& tok = token_at(p, p.canonical[id]);
            put_uvar(rec, tok.size());
            rec.insert(rec.end(), tok.begin(), tok.end());
            records.push_back(std::move(rec));
        }
        std::sort(records.begin(), records.end());
        Bytes joined;
        for (const Bytes& rec : records) joined.insert(joined.end(), rec.begin(), rec.end());
        r.multiset_sha256 = sha256::hex(joined);
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
    } else if (mode == G5Order::RandomPermutation) {
        // Reconstruct the deterministic A0 null (prereg section 4 A0) over the
        // decoder's own canonical coordinate list. The encoder's canonical order
        // is SOURCE-FRAME order: walk frame_group, and for each structured
        // record emit occ-then-slot. This mirrors build_g5_plan exactly.
        std::vector<size_t> occ(groups.size(), 0);
        struct Coord { size_t gid, occ, slot; };
        std::vector<Coord> canon;
        for (uint32_t gid : frame_group) {
            const G5DGroup& g = groups[gid];
            if (g.raw) continue;
            const size_t oi = occ[gid]++;
            if (oi >= g.members) throw std::runtime_error("G5 A0 occurrence overflow");
            for (size_t slot = 0; slot < g.slots; ++slot)
                canon.push_back(Coord{gid, oi, slot});
        }
        for (size_t gid = 0; gid < groups.size(); ++gid)
            if (!groups[gid].raw && occ[gid] != groups[gid].members)
                throw std::runtime_error("G5 A0 occurrence mismatch");

        const char* const kSeed = kG5RandomPermutationSeed;
        std::vector<std::pair<std::array<uint8_t, 32>, size_t>> keyed;
        keyed.reserve(canon.size());
        for (size_t i = 0; i < canon.size(); ++i) {
            Bytes buf;
            for (const char* ch = kSeed; *ch; ++ch) buf.push_back(uint8_t(*ch));
            buf.push_back(0x1F);
            for (unsigned b = 0; b < 8; ++b)
                buf.push_back(uint8_t((uint64_t(i) >> (b * 8)) & 0xffu));
            keyed.emplace_back(sha256::hash(buf), i);
        }
        std::sort(keyed.begin(), keyed.end(), [](const auto& x, const auto& y) {
            if (x.first != y.first) return x.first < y.first;
            return x.second < y.second;
        });
        for (const auto& kv : keyed) {
            const Coord& c = canon[kv.second];
            read_into(groups[c.gid], c.occ, c.slot);
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
    bool mode_pack_ok = false;
    double encode_ms = 0.0;
    double decode_ms = 0.0;
};

static bool mode_pack_unpack_roundtrip(G5Order mode, const Bytes& body);

static MeasuredArm measure_arm(const G5Plan& p, const Bytes& src, G5Order mode) {
    MeasuredArm m;
    m.built = build_arm(p, mode);

    const auto e0 = std::chrono::steady_clock::now();
    m.brotli = brotli_encode(m.built.body);
    const auto e1 = std::chrono::steady_clock::now();
    m.encode_ms = std::chrono::duration<double, std::milli>(e1 - e0).count();
    m.complete_bytes = 1 + m.brotli.size(); // one charged outer order-mode byte

    // Materialize the charged mode byte (prereg section 3.2).
    m.mode_pack_ok = mode_pack_unpack_roundtrip(mode, m.built.body);
    if (!m.mode_pack_ok) throw std::runtime_error("G5 mode byte pack/unpack mismatch");

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
              << ",\"mode_byte\":" << static_cast<unsigned>(m.built.mode)
              << ",\"body_bytes\":" << m.built.body.size()
              << ",\"envelope_len\":" << m.built.envelope_len
              << ",\"token_region_len\":" << m.built.token_region_len
              << ",\"brotli_bytes\":" << m.brotli.size()
              << ",\"complete_bytes\":" << m.complete_bytes
              << ",\"roundtrip\":" << (m.roundtrip ? "true" : "false")
              << ",\"mode_pack_unpack_ok\":" << (m.mode_pack_ok ? "true" : "false")
              << ",\"permutation_ok\":" << (m.built.permutation_ok ? "true" : "false")
              << ",\"envelope_sha256\":\"" << m.built.envelope_sha256 << "\""
              << ",\"canonical_token_multiset_sha256\":\"" << m.built.multiset_sha256 << "\""
              << ",\"build_ms\":" << m.built.build_ms
              << ",\"encode_ms\":" << m.encode_ms
              << ",\"decode_ms\":" << m.decode_ms
              << "}";
}

// Mode byte materialization (prereg section 3.2): pack the one-byte order mode
// with the body and unpack it, verifying the recovered mode is the packed one.
static bool mode_pack_unpack_roundtrip(G5Order mode, const Bytes& body) {
    Bytes packed;
    packed.push_back(static_cast<uint8_t>(mode));
    packed.insert(packed.end(), body.begin(), body.end());
    if (packed.size() != body.size() + 1) return false;
    // Selector-byte validation (prereg I7): the materialized out-of-band mode
    // byte must be one of the four frozen selectors before it is accepted.
    const uint8_t selector = packed.front();
    if (!is_valid_g5_selector(selector)) return false;
    const G5Order recovered = static_cast<G5Order>(selector);
    if (recovered != mode) return false;
    const Bytes unpacked(packed.begin() + 1, packed.end());
    return unpacked == body;
}

static std::string brotli_version_dotted() {
    const uint32_t v = BrotliEncoderVersion();
    return std::to_string((v >> 24) & 0xffu) + "." +
           std::to_string((v >> 12) & 0xfffu) + "." +
           std::to_string(v & 0xfffu);
}

static int measure_g5a(const std::string& path) {
    const Bytes src = read_file(path);
    const auto p0 = std::chrono::steady_clock::now();
    const G5Plan plan = build_g5_plan(src);
    const auto p1 = std::chrono::steady_clock::now();
    const double parse_plan_ms =
        std::chrono::duration<double, std::milli>(p1 - p0).count();

    const MeasuredArm a0 = measure_arm(plan, src, G5Order::RandomPermutation);
    const MeasuredArm a1 = measure_arm(plan, src, G5Order::SourceOrder);
    const MeasuredArm a2 = measure_arm(plan, src, G5Order::ShapeRow);
    const MeasuredArm a3 = measure_arm(plan, src, G5Order::ShapeColumn);

    const std::array<const MeasuredArm*, 4> arms = {&a0, &a1, &a2, &a3};

    const bool same_body_size =
        a0.built.body.size() == a1.built.body.size() &&
        a1.built.body.size() == a2.built.body.size() &&
        a2.built.body.size() == a3.built.body.size();
    if (!same_body_size) throw std::runtime_error("G5 body-size identity failure");

    auto has_common_prefix = [&](const Bytes& body) {
        return body.size() >= plan.prefix.size() &&
               std::equal(plan.prefix.begin(), plan.prefix.end(), body.begin());
    };
    const bool envelope_identity =
        has_common_prefix(a0.built.body) && has_common_prefix(a1.built.body) &&
        has_common_prefix(a2.built.body) && has_common_prefix(a3.built.body);
    if (!envelope_identity)
        throw std::runtime_error("G5 common-envelope identity failure");

    // Envelope SHA-256 (prereg I4) must be identical across arms and equal the
    // plan prefix hash; multiset SHA-256 (prereg I3) must be identical too.
    for (const MeasuredArm* m : arms) {
        if (m->built.envelope_sha256 != plan.envelope_sha256)
            throw std::runtime_error("G5 envelope sha256 mismatch");
        if (m->built.multiset_sha256 != plan.multiset_sha256)
            throw std::runtime_error("G5 multiset sha256 mismatch");
    }
    const bool hashes_identical =
        a0.built.envelope_sha256 == a1.built.envelope_sha256 &&
        a1.built.envelope_sha256 == a2.built.envelope_sha256 &&
        a2.built.envelope_sha256 == a3.built.envelope_sha256 &&
        a0.built.multiset_sha256 == a1.built.multiset_sha256 &&
        a1.built.multiset_sha256 == a2.built.multiset_sha256 &&
        a2.built.multiset_sha256 == a3.built.multiset_sha256;
    if (!hashes_identical)
        throw std::runtime_error("G5 invariant-hash identity failure");

    // Strengthened I2 (r6): expose and gate envelope-length and token-region
    // length identities across all four arms, in addition to total body length.
    // These are derived from the unchanged prefix/body construction.
    const bool envelope_len_identity =
        a0.built.envelope_len == a1.built.envelope_len &&
        a1.built.envelope_len == a2.built.envelope_len &&
        a2.built.envelope_len == a3.built.envelope_len;
    const bool token_region_len_identity =
        a0.built.token_region_len == a1.built.token_region_len &&
        a1.built.token_region_len == a2.built.token_region_len &&
        a2.built.token_region_len == a3.built.token_region_len;
    if (!envelope_len_identity)
        throw std::runtime_error("G5 envelope-length identity failure");
    if (!token_region_len_identity)
        throw std::runtime_error("G5 token-region-length identity failure");

    const bool all_perm =
        a0.built.permutation_ok && a1.built.permutation_ok &&
        a2.built.permutation_ok && a3.built.permutation_ok;
    const bool all_roundtrip =
        a0.roundtrip && a1.roundtrip && a2.roundtrip && a3.roundtrip;
    const bool all_mode_pack =
        a0.mode_pack_ok && a1.mode_pack_ok && a2.mode_pack_ok && a3.mode_pack_ok;
    if (!all_perm || !all_roundtrip)
        throw std::runtime_error("G5 hard invariant failure");
    if (!all_mode_pack)
        throw std::runtime_error("G5 mode byte pack/unpack failure");

    // Raw Brotli is context only and does not enter the A0/A1/A2/A3 causal gate.
    const Bytes raw_br = brotli_encode(src);
    // Frozen accounting: 1 (arm selector) + uvar_len(source_len) + payload.
    const size_t raw_complete_context =
        1ull + static_cast<size_t>(uvar_len(src.size())) + raw_br.size();

    const int64_t shape_bytes =
        static_cast<int64_t>(a1.complete_bytes) - static_cast<int64_t>(a2.complete_bytes);
    const int64_t column_bytes =
        static_cast<int64_t>(a2.complete_bytes) - static_cast<int64_t>(a3.complete_bytes);
    const int64_t total_bytes =
        static_cast<int64_t>(a1.complete_bytes) - static_cast<int64_t>(a3.complete_bytes);
    const int64_t random_null_bytes =
        static_cast<int64_t>(a0.complete_bytes) - static_cast<int64_t>(a3.complete_bytes);

    std::cout << "{"
              << "\"schema\":1"
              << ",\"file\":\"" << json_escape(path) << "\""
              << ",\"source_bytes\":" << src.size()
              << ",\"raw_brotli_bytes\":" << raw_br.size()
              << ",\"raw_complete_bytes_context\":" << raw_complete_context
              << ",\"brotli_encoder_version\":" << BrotliEncoderVersion()
              << ",\"brotli_encoder_version_dotted\":\"" << brotli_version_dotted() << "\""
              << ",\"carrier_quality\":11"
              << ",\"carrier_window\":30"
              << ",\"parse_plan_ms\":" << parse_plan_ms
              << ",\"frame_count\":" << plan.analysis.frames.size()
              << ",\"structured_frame_count\":" << plan.analysis.structured_frame_count
              << ",\"raw_frame_count\":" << plan.analysis.raw_frame_count
              << ",\"shape_count\":" << plan.analysis.shapes.size()
              << ",\"token_chunk_count\":" << plan.canonical.size()
              << ",\"structured_token_bytes\":" << plan.structured_token_bytes
              << ",\"common_prefix_bytes\":" << plan.prefix.size()
              << ",\"envelope_sha256\":\"" << plan.envelope_sha256 << "\""
              << ",\"canonical_token_multiset_sha256\":\"" << plan.multiset_sha256 << "\""
              << ",\"body_size_identity\":" << (same_body_size ? "true" : "false")
              << ",\"envelope_len_identity\":" << (envelope_len_identity ? "true" : "false")
              << ",\"token_region_len_identity\":" << (token_region_len_identity ? "true" : "false")
              << ",\"envelope_len\":" << a1.built.envelope_len
              << ",\"token_region_len\":" << a1.built.token_region_len
              << ",\"envelope_identity\":" << (envelope_identity ? "true" : "false")
              << ",\"invariant_hashes_identical\":" << (hashes_identical ? "true" : "false")
              << ",\"all_permutations_exact\":" << (all_perm ? "true" : "false")
              << ",\"all_mode_pack_unpack\":" << (all_mode_pack ? "true" : "false")
              << ",\"all_roundtrip\":" << (all_roundtrip ? "true" : "false");
    print_arm_json("a0_random_permutation", a0);
    print_arm_json("a1_source_order", a1);
    print_arm_json("a2_shape_row", a2);
    print_arm_json("a3_shape_column", a3);
    std::cout << ",\"shape_grouping_bytes\":" << shape_bytes
              << ",\"column_increment_bytes\":" << column_bytes
              << ",\"total_order_bytes\":" << total_bytes
              << ",\"random_null_bytes\":" << random_null_bytes
              << "}\n";
    return 0;
}

static void g5a_fixture(const std::string& s, const char* label) {
    const Bytes src = bytes(s);
    const G5Plan p = build_g5_plan(src);
    const BuiltArm a0 = build_arm(p, G5Order::RandomPermutation);
    const BuiltArm a1 = build_arm(p, G5Order::SourceOrder);
    const BuiltArm a2 = build_arm(p, G5Order::ShapeRow);
    const BuiltArm a3 = build_arm(p, G5Order::ShapeColumn);

    if (a0.body.size() != a1.body.size() || a1.body.size() != a2.body.size() ||
        a2.body.size() != a3.body.size())
        throw std::runtime_error(std::string(label) + ": body sizes differ");
    auto has_prefix = [&](const Bytes& body) {
        return body.size() >= p.prefix.size() &&
               std::equal(p.prefix.begin(), p.prefix.end(), body.begin());
    };
    if (!has_prefix(a0.body) || !has_prefix(a1.body) || !has_prefix(a2.body) ||
        !has_prefix(a3.body))
        throw std::runtime_error(std::string(label) + ": common envelope differs");
    for (const auto* a : {&a0, &a1, &a2, &a3}) {
        if (!a->permutation_ok)
            throw std::runtime_error(std::string(label) + ": permutation invalid");
        // Byte-level invariant hashes (prereg I3/I4).
        if (a->envelope_sha256 != p.envelope_sha256)
            throw std::runtime_error(std::string(label) + ": envelope hash mismatch");
        if (a->multiset_sha256 != p.multiset_sha256)
            throw std::runtime_error(std::string(label) + ": multiset hash mismatch");
        // Strengthened I2 (r6): envelope and token-region lengths are identical
        // across arms and derive exactly from prefix + body.
        if (a->envelope_len != p.prefix.size())
            throw std::runtime_error(std::string(label) + ": envelope length mismatch");
        if (a->token_region_len != a->body.size() - p.prefix.size())
            throw std::runtime_error(std::string(label) + ": token region length mismatch");
        // Mode byte materialization (prereg section 3.2).
        if (!mode_pack_unpack_roundtrip(a->mode, a->body))
            throw std::runtime_error(std::string(label) + ": mode pack/unpack");
    }
    if (a0.envelope_len != a1.envelope_len || a1.envelope_len != a2.envelope_len ||
        a2.envelope_len != a3.envelope_len)
        throw std::runtime_error(std::string(label) + ": envelope length not identical");
    if (a0.token_region_len != a1.token_region_len ||
        a1.token_region_len != a2.token_region_len ||
        a2.token_region_len != a3.token_region_len)
        throw std::runtime_error(std::string(label) + ": token region length not identical");
    if (decode_g5_body(a0.body, G5Order::RandomPermutation) != src)
        throw std::runtime_error(std::string(label) + ": random decode");
    if (decode_g5_body(a1.body, G5Order::SourceOrder) != src)
        throw std::runtime_error(std::string(label) + ": source decode");
    if (decode_g5_body(a2.body, G5Order::ShapeRow) != src)
        throw std::runtime_error(std::string(label) + ": shape-row decode");
    if (decode_g5_body(a3.body, G5Order::ShapeColumn) != src)
        throw std::runtime_error(std::string(label) + ": shape-column decode");

    const size_t expected = p.canonical.size();
    // A0/A1/A2/A3 must be exact permutations; A1 is the identity permutation.
    for (const auto* a : {&a0, &a1, &a2, &a3})
        if (!validate_permutation(a->permutation, expected))
            throw std::runtime_error(std::string(label) + ": exact coverage");
    for (size_t i = 0; i < expected; ++i)
        if (a1.permutation[i] != i)
            throw std::runtime_error(std::string(label) + ": source order is not identity");
    // The A0 random permutation must be deterministic: rebuilt identically.
    const BuiltArm a0b = build_arm(p, G5Order::RandomPermutation);
    if (a0.permutation != a0b.permutation || a0.body != a0b.body)
        throw std::runtime_error(std::string(label) + ": A0 not deterministic");
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

    // SHA-256 known-answer tests: the invariant hashes depend on this.
    if (sha256::hex(bytes("")) !=
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")
        throw std::runtime_error("G5 sha256 empty KAT");
    if (sha256::hex(bytes("abc")) !=
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
        throw std::runtime_error("G5 sha256 abc KAT");

    // A0 must actually reorder a multi-chunk fixture (otherwise it is a no-op
    // null and the audit-control claim is vacuous).
    {
        const std::string reorderable =
            "{\"a\":1,\"b\":\"x\"}\n"
            "{\"p\":9,\"q\":\"y\"}\n"
            "{\"a\":2,\"b\":\"z\"}\n"
            "{\"p\":8,\"q\":\"w\"}\n";
        const Bytes src = bytes(reorderable);
        const G5Plan p = build_g5_plan(src);
        if (p.canonical.size() < 4)
            throw std::runtime_error("G5 A0 fixture too small to reorder");
        const BuiltArm a0 = build_arm(p, G5Order::RandomPermutation);
        const BuiltArm a1 = build_arm(p, G5Order::SourceOrder);
        if (a0.permutation == a1.permutation)
            throw std::runtime_error("G5 A0 permutation equals identity (no null)");
    }

    // Selector-byte validation (prereg I7): only the four frozen G5Order values
    // are valid; any other byte must be rejected deterministically.
    for (uint8_t s : {static_cast<uint8_t>(0), static_cast<uint8_t>(1),
                      static_cast<uint8_t>(2), static_cast<uint8_t>(3)})
        if (!is_valid_g5_selector(s))
            throw std::runtime_error("G5 valid selector rejected");
    for (uint8_t s : {static_cast<uint8_t>(4), static_cast<uint8_t>(5),
                      static_cast<uint8_t>(0x7f), static_cast<uint8_t>(0xff)})
        if (is_valid_g5_selector(s))
            throw std::runtime_error("G5 invalid selector accepted");
    {
        Bytes body = bytes("payload");
        Bytes packed;
        packed.push_back(0xff);  // invalid selector
        packed.insert(packed.end(), body.begin(), body.end());
        if (is_valid_g5_selector(packed.front()))
            throw std::runtime_error("G5 invalid packed selector accepted");
    }

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
