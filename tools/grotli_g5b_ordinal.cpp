// ANVIL I10 G5B-ORDINAL interstitial probe prototype.
//
// Sharp causal question: does the boundary between exact lexical shapes fragment
// positional same-ordinal slot locality?
//
// This experiment intentionally reuses the exact frozen G3 parser / frame / shape
// implementation in the same translation unit, exactly as frozen G5A did. It adds
// no leaf family, no planner, no new metadata, and no new representation family.
// Its only causal variable is the permutation of identical exact lexical scalar
// chunks before the same Brotli q11/lgwin30 backend.
//
// Preregistration:
//   docs/I10-GROTLI-G5B-ORDINAL-PREREG.md  (freeze revision r3)
//
// The carrier grammar is the EXACT frozen G5A common carrier body: magic "G5AO",
// version 1, and the same prefix grammar. B1/FLOOR is byte/accounting-identical to
// frozen G5A A3 INCLUDING the charged out-of-band selector byte 3 (the frozen G5A
// A3 SHAPE_COLUMN selector): the shared A3 bytes, and therefore the envelope
// SHA-256 and complete-byte totals, are identical. B0/B2 use new out-of-band
// charged selector bytes 4/5, disjoint from G5A's 0..3 mode space, while B1
// preserves the A3 selector 3; the prereg revision is non-serialized. Three arms
// only:
// There is deliberately NO B3 in this first freeze.
//
// FROZEN-INCLUSION (prereg section 2.1): CI compiles G5B-ORDINAL against the
// materialized pinned frozen G3 source, NOT the mutable working-tree file, by
// defining
//     -DG5B_FROZEN_G3_HEADER=\"frozen-grotli_g3.cpp\"
// Local builds with no define fall back to the working-tree tools/grotli_g3.cpp.
//
// IMPORTANT: D1-D4 / V1 measurements belong in GitHub Actions only. Local use is
// limited to compilation and tiny selftests. No D1-D4/V1 outcome was observed
// before the prereg freeze.

#ifndef G5B_FROZEN_G3_HEADER
#define G5B_FROZEN_G3_HEADER "grotli_g3.cpp"
#endif

#define main grotli_g3_embedded_main
#include G5B_FROZEN_G3_HEADER
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
// hashes). The frozen G3 TU provides none, so G5B-ORDINAL carries its own.
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

// The three frozen G5B-ORDINAL arms. Values are the charged out-of-band selector
// bytes. The carrier body now shares frozen G5A's "G5AO" magic, so selector values
// are chosen to AVOID semantic aliasing between the two grammars: B1/FLOOR is
// EXACTLY 3 so that it is byte-identical to frozen G5A A3 SHAPE_COLUMN (which used
// selector 3), while B0 NULL and B2 ORDINAL_BLOCKED take new, non-colliding values.
enum class G5BOrdinal : uint8_t {
    OrdinalFloor = 3,    // B1 EXACT frozen G5A A3 SHAPE_COLUMN (= G5A A3 selector 3)
    OrdinalNull = 4,     // B0 deterministic null (new, non-colliding with G5A 0..3)
    OrdinalBlocked = 5,  // B2 global-ordinal blocked, the single treatment
};

static const char* arm_name(G5BOrdinal m) {
    switch (m) {
        case G5BOrdinal::OrdinalNull: return "ORDINAL_NULL";
        case G5BOrdinal::OrdinalFloor: return "ORDINAL_FLOOR";
        case G5BOrdinal::OrdinalBlocked: return "ORDINAL_BLOCKED";
    }
    throw std::runtime_error("unknown G5B ordinal arm");
}

static constexpr uint8_t kG5BVersion = 1;
// EXACT frozen G5A carrier magic. B1/FLOOR must reproduce frozen G5A A3 byte-for-byte
// on the shared carrier body, which is only possible if the envelope bytes (including
// this magic) are the frozen G5A bytes rather than a distinct G5B magic.
static constexpr std::array<uint8_t, 4> kG5BMagic = {'G','5','A','O'};

// Single shared definition of the B0 null seed. BOTH the encoder
// (`permutation_for`) and the decoder (`decode_g5b_body`) hash exactly these
// bytes; keeping one definition here prevents the two sites from drifting.
// Changing these bytes changes the frozen B0 null and is forbidden post-freeze.
// This is a NEW literal, distinct from G5A's A0 seed.
static constexpr const char* kG5BNullSeed = "G5B-ORDINAL-NULL-SEED-v1";

// The three frozen arm selectors are exactly {3,4,5}. Any other selector byte is
// malformed and must be rejected deterministically (prereg I7). The set deliberately
// excludes G5A's 0/1/2 selectors so the shared "G5AO" carrier cannot be silently
// confused with a G5A A0/A1/A2 mode; selector 3 is reserved for the exact frozen
// G5A A3 floor.
static bool is_valid_g5b_selector(uint8_t s) {
    return s == static_cast<uint8_t>(G5BOrdinal::OrdinalFloor) ||
           s == static_cast<uint8_t>(G5BOrdinal::OrdinalNull) ||
           s == static_cast<uint8_t>(G5BOrdinal::OrdinalBlocked);
}

struct TokenCoord {
    uint32_t shape = 0;
    uint32_t occurrence = 0;
    uint32_t slot = 0;
};

struct G5BPlan {
    RegionAnalysis analysis;
    Bytes prefix; // full common envelope + raw residuals; token stream begins after this.
    std::vector<TokenCoord> canonical; // canonical SOURCE_ORDER identity list.
    // [shape][occurrence][slot] -> canonical token identity.
    std::vector<std::vector<std::vector<size_t>>> canonical_id;
    uint64_t structured_token_bytes = 0;
    // Frozen liveness metrics (prereg section 5). Computed once per file.
    uint64_t max_slots = 0;
    uint64_t shared_ordinal_slots = 0;
    // Byte-level invariant hashes (prereg I3/I4). Identical across all arms.
    std::string envelope_sha256;         // SHA-256 of `prefix`
    std::string multiset_sha256;         // SHA-256 of sorted (uvar(len)||bytes) records
};

static const Bytes& token_at(const G5BPlan& p, const TokenCoord& c);

// Canonical token-multiset SHA-256 (prereg I3): sort the decode-visible framed
// records lexicographically ascending, then hash the concatenation with no
// separator. Order-independent, so it is identical for every permutation.
static std::string canonical_multiset_sha256(const G5BPlan& p) {
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

static const Bytes& token_at(const G5BPlan& p, const TokenCoord& c) {
    if (c.shape >= p.analysis.shapes.size())
        throw std::runtime_error("G5B token shape out of range");
    const ShapePlan& sh = p.analysis.shapes[c.shape];
    if (c.slot >= sh.slots.size())
        throw std::runtime_error("G5B token slot out of range");
    if (c.occurrence >= sh.slots[c.slot].tokens.size())
        throw std::runtime_error("G5B token occurrence out of range");
    return sh.slots[c.slot].tokens[c.occurrence];
}

static void append_token_chunk(Bytes& out, const Bytes& tok) {
    if (tok.empty()) throw std::runtime_error("G5B empty scalar token");
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

static G5BPlan build_g5b_plan(const Bytes& src) {
    if (src.empty()) throw std::runtime_error("G5B source is empty");

    G5BPlan p;
    p.analysis = analyze_regions(src);
    RegionAnalysis& a = p.analysis;
    if (a.frames.empty()) throw std::runtime_error("G5B requires at least one frame");

    const bool has_raw = !a.raw_members.empty();
    const uint64_t group_count = a.shapes.size() + (has_raw ? 1u : 0u);
    if (group_count == 0) throw std::runtime_error("G5B has no groups");
    const uint32_t raw_gid = has_raw ? 0u : std::numeric_limits<uint32_t>::max();
    const uint32_t structured_offset = has_raw ? 1u : 0u;

    Bytes& out = p.prefix;
    out.insert(out.end(), kG5BMagic.begin(), kG5BMagic.end());
    out.push_back(kG5BVersion);
    put_uvar(out, src.size());
    put_uvar(out, a.frames.size());
    put_uvar(out, group_count);

    // Exact source reconstruction map. Identical in all G5B-ORDINAL arms.
    for (const ParsedRecord& r : a.records) {
        if (r.structured && r.shape_id >= a.shapes.size())
            throw std::runtime_error("G5B record shape id out of range");
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
        if (sh.id != sid) throw std::runtime_error("G5B shape id ordering bug");
        const size_t slots = sh.parts.empty() ? 0 : sh.parts.size() - 1;
        if (sh.slots.size() != slots && !sh.members.empty())
            throw std::runtime_error("G5B shape slot/part mismatch");
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
            if (fi >= a.frames.size()) throw std::runtime_error("G5B raw frame id out of range");
            const Frame& f = a.frames[fi];
            if (f.hi < f.lo || f.hi > src.size())
                throw std::runtime_error("G5B raw frame span invalid");
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
        if (sid >= a.shapes.size()) throw std::runtime_error("G5B canonical sid");
        const ShapePlan& sh = a.shapes[sid];
        const size_t occ = next_occ[sid]++;
        if (occ >= sh.members.size() || sh.members[occ] != fi)
            throw std::runtime_error("G5B shape member/source-order mismatch");
        for (size_t slot = 0; slot < sh.slots.size(); ++slot) {
            if (occ >= sh.slots[slot].tokens.size())
                throw std::runtime_error("G5B missing structured token");
            const size_t id = p.canonical.size();
            if (id == std::numeric_limits<size_t>::max())
                throw std::runtime_error("G5B token id overflow");
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
            throw std::runtime_error("G5B shape occurrence count mismatch");
        for (const auto& row : p.canonical_id[sid])
            for (size_t id : row)
                if (id == std::numeric_limits<size_t>::max())
                    throw std::runtime_error("G5B unassigned canonical token");
    }

    // Frozen liveness metrics (prereg section 5), computed once per file.
    for (size_t sid = 0; sid < a.shapes.size(); ++sid)
        p.max_slots = std::max<uint64_t>(p.max_slots, a.shapes[sid].slots.size());
    for (uint64_t j = 0; j < p.max_slots; ++j) {
        uint64_t present = 0;
        for (size_t sid = 0; sid < a.shapes.size(); ++sid)
            if (j < a.shapes[sid].slots.size()) ++present;
        if (present >= 2) ++p.shared_ordinal_slots;
    }
    // Fail-closed liveness sanity (prereg I11 / section 5.1, corrected): shared
    // ordinals are each counted once, no file can share more ordinals than it has
    // slots, and a single shape can never share an ordinal with itself
    // (shared_ordinal_slots == 0). NOTE (r3 correction): b1_eq_b2 does NOT imply
    // shared_ordinal_slots == 0. Two DISTINCT one-slot shapes are a legitimate
    // multi-shape DEGENERATE case (shared_ordinal_slots == 1, b1_eq_b2 == true);
    // no invariant may reject it.
    if (p.shared_ordinal_slots > p.max_slots)
        throw std::runtime_error("G5B shared_ordinal_slots exceeds max_slots");
    if (a.shapes.size() < 2 && p.shared_ordinal_slots != 0)
        throw std::runtime_error("G5B single shape cannot share an ordinal");


    // Byte-level invariant hashes (prereg I3/I4), computed once per file.
    p.envelope_sha256 = sha256::hex(p.prefix);
    p.multiset_sha256 = canonical_multiset_sha256(p);

    return p;
}

static std::vector<size_t> permutation_for(const G5BPlan& p, G5BOrdinal mode) {
    std::vector<size_t> perm;
    perm.reserve(p.canonical.size());

    if (mode == G5BOrdinal::OrdinalNull) {
        // B0 deterministic null (prereg section 3): sort canonical indices by
        // SHA-256(seed || 0x1F || index_le_u64) as a big-endian 256-bit integer,
        // ties by lower index. No score, no content, no observed byte influences
        // this permutation. One draw, never a p-test.
        const char* const kSeed = kG5BNullSeed;
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
    } else if (mode == G5BOrdinal::OrdinalFloor) {
        // B1 EXACT frozen G5A A3 SHAPE_COLUMN: for sid first-appearance order,
        // for slot j increasing, for occurrence increasing, emit id[sid][occ][j].
        for (size_t sid = 0; sid < p.analysis.shapes.size(); ++sid) {
            const ShapePlan& sh = p.analysis.shapes[sid];
            for (size_t slot = 0; slot < sh.slots.size(); ++slot)
                for (size_t occ = 0; occ < sh.members.size(); ++occ)
                    perm.push_back(p.canonical_id[sid][occ][slot]);
        }
    } else if (mode == G5BOrdinal::OrdinalBlocked) {
        // B2 ORDINAL_BLOCKED: for j = 0..D-1, for sid first-appearance order with
        // j < slots(sid), for occurrence increasing, emit id[sid][occ][j]. The
        // positional ordinal j is held constant globally across all shapes.
        const size_t D = static_cast<size_t>(p.max_slots);
        for (size_t j = 0; j < D; ++j) {
            for (size_t sid = 0; sid < p.analysis.shapes.size(); ++sid) {
                const ShapePlan& sh = p.analysis.shapes[sid];
                if (j >= sh.slots.size()) continue;
                for (size_t occ = 0; occ < sh.members.size(); ++occ)
                    perm.push_back(p.canonical_id[sid][occ][j]);
            }
        }
    } else {
        throw std::runtime_error("G5B unknown ordinal arm");
    }

    if (!validate_permutation(perm, p.canonical.size()))
        throw std::runtime_error("G5B permutation coverage failure");
    return perm;
}

struct BuiltArm {
    G5BOrdinal mode = G5BOrdinal::OrdinalFloor;  // selector 3
    Bytes body;
    std::vector<size_t> permutation;
    bool permutation_ok = false;
    double build_ms = 0.0;
    // Byte-level invariant hashes (prereg I3/I4).
    std::string envelope_sha256;
    std::string multiset_sha256;
    // I2: envelope and token-region lengths (derived; body bytes UNCHANGED).
    uint64_t envelope_len = 0;
    uint64_t token_region_len = 0;
    // I11 liveness (per arm; meaningful for B2).
    uint64_t cross_shape_ordinal_tokens = 0;
};

static BuiltArm build_arm(const G5BPlan& p, G5BOrdinal mode) {
    const auto t0 = std::chrono::steady_clock::now();
    BuiltArm r;
    r.mode = mode;
    r.permutation = permutation_for(p, mode);
    r.permutation_ok = validate_permutation(r.permutation, p.canonical.size());
    if (!r.permutation_ok) throw std::runtime_error("G5B invalid permutation");

    // Liveness (prereg section 5): count B2 tokens whose immediately-preceding
    // token in this arm's stream belongs to a DIFFERENT shape at the same ordinal.
    for (size_t i = 1; i < r.permutation.size(); ++i) {
        const TokenCoord& prev = p.canonical[r.permutation[i - 1]];
        const TokenCoord& cur = p.canonical[r.permutation[i]];
        if (prev.slot == cur.slot && prev.shape != cur.shape)
            ++r.cross_shape_ordinal_tokens;
    }

    r.body = p.prefix;
    for (size_t id : r.permutation) {
        const TokenCoord& c = p.canonical[id];
        append_token_chunk(r.body, token_at(p, c));
    }

    // I2: envelope is exactly the shared prefix; the token region is the
    // remainder. These are pure derivations of the unchanged body.
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

struct G5BDGroup {
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
        throw std::runtime_error("G5B bad structured token length");
    Bytes tok(body.begin() + pos, body.begin() + pos + static_cast<size_t>(n));
    pos += static_cast<size_t>(n);
    return tok;
}

static Bytes decode_g5b_body(const Bytes& body, G5BOrdinal mode) {
    if (body.size() < 5 || !std::equal(kG5BMagic.begin(), kG5BMagic.end(), body.begin()))
        throw std::runtime_error("G5B bad magic");
    size_t pos = 4;
    if (body[pos++] != kG5BVersion) throw std::runtime_error("G5B bad version");

    const uint64_t decoded_len = get_uvar(body, pos);
    if (decoded_len == 0 || decoded_len > kMaxDecoded ||
        decoded_len > std::numeric_limits<size_t>::max())
        throw std::runtime_error("G5B bad decoded length");

    const uint64_t frame_count = get_uvar_bounded(body, pos, decoded_len);
    if (frame_count == 0 || frame_count > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("G5B bad frame count");

    const uint64_t group_count = get_uvar_bounded(body, pos, frame_count);
    if (group_count == 0 || group_count > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("G5B bad group count");

    std::vector<uint32_t> frame_group;
    frame_group.reserve(static_cast<size_t>(frame_count));
    for (uint64_t i = 0; i < frame_count; ++i) {
        const uint64_t gid = get_uvar_bounded(body, pos, group_count - 1);
        frame_group.push_back(static_cast<uint32_t>(gid));
    }

    std::vector<G5BDGroup> groups(static_cast<size_t>(group_count));
    uint64_t member_sum = 0;
    size_t raw_groups = 0;
    for (size_t gid = 0; gid < groups.size(); ++gid) {
        if (pos >= body.size()) throw std::runtime_error("G5B truncated group kind");
        const uint8_t kind = body[pos++];
        if (kind > 1) throw std::runtime_error("G5B unknown group kind");

        const uint64_t members = get_uvar_bounded(body, pos, frame_count);
        if (members == 0) throw std::runtime_error("G5B zero-member group");
        if (member_sum > frame_count - members)
            throw std::runtime_error("G5B group member sum overflow");
        member_sum += members;

        G5BDGroup& g = groups[gid];
        g.raw = kind == 1;
        g.members = static_cast<size_t>(members);
        if (g.raw) {
            ++raw_groups;
            if (raw_groups > 1) throw std::runtime_error("G5B multiple raw groups");
            continue;
        }

        const uint64_t slots = get_uvar_bounded(body, pos, decoded_len);
        const uint64_t part_count = get_uvar_bounded(body, pos, decoded_len);
        if (part_count != slots + 1)
            throw std::runtime_error("G5B template part count mismatch");
        if (slots > std::numeric_limits<size_t>::max() ||
            part_count > std::numeric_limits<size_t>::max())
            throw std::runtime_error("G5B template count width");

        g.slots = static_cast<size_t>(slots);
        g.parts.reserve(static_cast<size_t>(part_count));
        for (uint64_t i = 0; i < part_count; ++i) {
            const uint64_t n = get_uvar_bounded(body, pos, decoded_len);
            if (n > body.size() - pos) throw std::runtime_error("G5B template span");
            g.parts.emplace_back(
                body.begin() + pos,
                body.begin() + pos + static_cast<size_t>(n));
            pos += static_cast<size_t>(n);
        }
        g.rows.assign(g.members, std::vector<Bytes>(g.slots));
    }

    if (member_sum != frame_count)
        throw std::runtime_error("G5B group member sum mismatch");

    for (G5BDGroup& g : groups) {
        if (!g.raw) continue;
        g.raw_bodies.reserve(g.members);
        for (size_t i = 0; i < g.members; ++i) {
            const uint64_t n = get_uvar_bounded(body, pos, decoded_len);
            if (n > body.size() - pos) throw std::runtime_error("G5B raw residual span");
            g.raw_bodies.emplace_back(
                body.begin() + pos,
                body.begin() + pos + static_cast<size_t>(n));
            pos += static_cast<size_t>(n);
        }
    }

    auto read_into = [&](G5BDGroup& g, size_t occ, size_t slot) {
        if (g.raw || occ >= g.members || slot >= g.slots)
            throw std::runtime_error("G5B token destination out of range");
        g.rows[occ][slot] = take_exact_chunk(body, pos, decoded_len);
    };

    // Reconstruct the decoder's own canonical coordinate list. The encoder's
    // canonical order is SOURCE-FRAME order: walk frame_group, and for each
    // structured record emit occ-then-slot. This mirrors build_g5b_plan exactly.
    std::vector<size_t> occ(groups.size(), 0);
    struct Coord { size_t gid, occ, slot; };
    std::vector<Coord> canon;
    for (uint32_t gid : frame_group) {
        const G5BDGroup& g = groups[gid];
        if (g.raw) continue;
        const size_t oi = occ[gid]++;
        if (oi >= g.members) throw std::runtime_error("G5B canonical occurrence overflow");
        for (size_t slot = 0; slot < g.slots; ++slot)
            canon.push_back(Coord{gid, oi, slot});
    }
    for (size_t gid = 0; gid < groups.size(); ++gid)
        if (!groups[gid].raw && occ[gid] != groups[gid].members)
            throw std::runtime_error("G5B canonical occurrence mismatch");

    if (mode == G5BOrdinal::OrdinalFloor) {
        // B1 ORDINAL_FLOOR: exact frozen G5A A3 SHAPE_COLUMN over the decoder's
        // group list (groups are the shapes in first-appearance order, plus an
        // optional raw group).
        for (G5BDGroup& g : groups) {
            if (g.raw) continue;
            for (size_t slot = 0; slot < g.slots; ++slot)
                for (size_t occ2 = 0; occ2 < g.members; ++occ2)
                    read_into(g, occ2, slot);
        }
    } else if (mode == G5BOrdinal::OrdinalBlocked) {
        // B2 ORDINAL_BLOCKED: for j = 0..D-1, for shape first-appearance order
        // with j < slots, for occurrence increasing.
        size_t D = 0;
        for (const G5BDGroup& g : groups)
            if (!g.raw) D = std::max(D, g.slots);
        for (size_t j = 0; j < D; ++j) {
            for (G5BDGroup& g : groups) {
                if (g.raw || j >= g.slots) continue;
                for (size_t occ2 = 0; occ2 < g.members; ++occ2)
                    read_into(g, occ2, j);
            }
        }
    } else if (mode == G5BOrdinal::OrdinalNull) {
        // B0 deterministic null over the decoder's own canonical coordinate list.
        const char* const kSeed = kG5BNullSeed;
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
        throw std::runtime_error("G5B invalid decode arm");
    }

    if (pos != body.size()) throw std::runtime_error("G5B trailing carrier bytes");

    Bytes out;
    out.reserve(static_cast<size_t>(decoded_len));
    std::vector<size_t> occ2(groups.size(), 0);
    for (uint32_t gid : frame_group) {
        if (gid >= groups.size()) throw std::runtime_error("G5B reconstruction gid");
        G5BDGroup& g = groups[gid];
        const size_t oi = occ2[gid]++;
        if (oi >= g.members) throw std::runtime_error("G5B reconstruction occurrence");
        if (g.raw) {
            const Bytes& b = g.raw_bodies[oi];
            if (b.size() > decoded_len - out.size())
                throw std::runtime_error("G5B raw reconstruction exceeds bound");
            out.insert(out.end(), b.begin(), b.end());
        } else {
            if (g.parts.size() != g.slots + 1)
                throw std::runtime_error("G5B reconstruction template mismatch");
            for (size_t slot = 0; slot < g.slots; ++slot) {
                const Bytes& part = g.parts[slot];
                const Bytes& tok = g.rows[oi][slot];
                if (part.size() > decoded_len - out.size())
                    throw std::runtime_error("G5B template reconstruction exceeds bound");
                out.insert(out.end(), part.begin(), part.end());
                if (tok.size() > decoded_len - out.size())
                    throw std::runtime_error("G5B token reconstruction exceeds bound");
                out.insert(out.end(), tok.begin(), tok.end());
            }
            const Bytes& tail = g.parts.back();
            if (tail.size() > decoded_len - out.size())
                throw std::runtime_error("G5B tail reconstruction exceeds bound");
            out.insert(out.end(), tail.begin(), tail.end());
        }
    }

    if (out.size() != decoded_len) throw std::runtime_error("G5B decoded length mismatch");
    for (size_t gid = 0; gid < groups.size(); ++gid)
        if (occ2[gid] != groups[gid].members)
            throw std::runtime_error("G5B reconstruction member mismatch");
    return out;
}

static bool mode_pack_unpack_roundtrip(G5BOrdinal mode, const Bytes& body);

struct MeasuredArm {
    BuiltArm built;
    Bytes brotli;
    size_t complete_bytes = 0;
    bool roundtrip = false;
    bool mode_pack_ok = false;
    double encode_ms = 0.0;
    double decode_ms = 0.0;
};

static MeasuredArm measure_arm(const G5BPlan& p, const Bytes& src, G5BOrdinal mode) {
    MeasuredArm m;
    m.built = build_arm(p, mode);

    const auto e0 = std::chrono::steady_clock::now();
    m.brotli = brotli_encode(m.built.body);
    const auto e1 = std::chrono::steady_clock::now();
    m.encode_ms = std::chrono::duration<double, std::milli>(e1 - e0).count();
    m.complete_bytes = 1 + m.brotli.size(); // one charged outer ordinal-mode byte

    // Materialize the charged mode byte (prereg section 2.3).
    m.mode_pack_ok = mode_pack_unpack_roundtrip(mode, m.built.body);
    if (!m.mode_pack_ok) throw std::runtime_error("G5B mode byte pack/unpack mismatch");

    const auto d0 = std::chrono::steady_clock::now();
    const Bytes decoded_body = brotli_decode_exact(m.brotli, m.built.body.size());
    if (decoded_body != m.built.body)
        throw std::runtime_error("G5B Brotli body roundtrip mismatch");
    const Bytes decoded_src = decode_g5b_body(decoded_body, mode);
    const auto d1 = std::chrono::steady_clock::now();
    m.decode_ms = std::chrono::duration<double, std::milli>(d1 - d0).count();
    m.roundtrip = decoded_src == src;
    if (!m.roundtrip) throw std::runtime_error("G5B source roundtrip mismatch");
    return m;
}

static void print_arm_json(const char* key, const MeasuredArm& m) {
    std::cout << ",\"" << key << "\":{"
              << "\"arm\":\"" << arm_name(m.built.mode) << "\""
              << ",\"mode_byte\":" << static_cast<unsigned>(m.built.mode)
              << ",\"body_bytes\":" << m.built.body.size()
              << ",\"envelope_len\":" << m.built.envelope_len
              << ",\"token_region_len\":" << m.built.token_region_len
              << ",\"brotli_bytes\":" << m.brotli.size()
              << ",\"complete_bytes\":" << m.complete_bytes
              << ",\"roundtrip\":" << (m.roundtrip ? "true" : "false")
              << ",\"mode_pack_unpack_ok\":" << (m.mode_pack_ok ? "true" : "false")
              << ",\"permutation_ok\":" << (m.built.permutation_ok ? "true" : "false")
              << ",\"cross_shape_ordinal_tokens\":"
              << m.built.cross_shape_ordinal_tokens
              << ",\"envelope_sha256\":\"" << m.built.envelope_sha256 << "\""
              << ",\"canonical_token_multiset_sha256\":\"" << m.built.multiset_sha256 << "\""
              << ",\"build_ms\":" << m.built.build_ms
              << ",\"encode_ms\":" << m.encode_ms
              << ",\"decode_ms\":" << m.decode_ms
              << "}";
}

// Mode byte materialization (prereg section 2.3): pack the one-byte arm mode
// with the body and unpack it, verifying the recovered mode is the packed one.
static bool mode_pack_unpack_roundtrip(G5BOrdinal mode, const Bytes& body) {
    Bytes packed;
    packed.push_back(static_cast<uint8_t>(mode));
    packed.insert(packed.end(), body.begin(), body.end());
    if (packed.size() != body.size() + 1) return false;
    // Selector-byte validation (prereg I7): the materialized out-of-band mode
    // byte must be one of the three frozen selectors before it is accepted.
    const uint8_t selector = packed.front();
    if (!is_valid_g5b_selector(selector)) return false;
    const G5BOrdinal recovered = static_cast<G5BOrdinal>(selector);
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

// Number of canonical indices whose position differs between two permutations.
static uint64_t moved_token_count(const std::vector<size_t>& a,
                                  const std::vector<size_t>& b) {
    if (a.size() != b.size()) throw std::runtime_error("G5B moved-count size mismatch");
    uint64_t moved = 0;
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i] != b[i]) ++moved;
    return moved;
}

static int measure_g5b(const std::string& path) {
    const Bytes src = read_file(path);
    const auto p0 = std::chrono::steady_clock::now();
    const G5BPlan plan = build_g5b_plan(src);
    const auto p1 = std::chrono::steady_clock::now();
    const double parse_plan_ms =
        std::chrono::duration<double, std::milli>(p1 - p0).count();

    const MeasuredArm b0 = measure_arm(plan, src, G5BOrdinal::OrdinalNull);
    const MeasuredArm b1 = measure_arm(plan, src, G5BOrdinal::OrdinalFloor);
    const MeasuredArm b2 = measure_arm(plan, src, G5BOrdinal::OrdinalBlocked);

    const std::array<const MeasuredArm*, 3> arms = {&b0, &b1, &b2};

    const bool same_body_size =
        b0.built.body.size() == b1.built.body.size() &&
        b1.built.body.size() == b2.built.body.size();
    if (!same_body_size) throw std::runtime_error("G5B body-size identity failure");

    auto has_common_prefix = [&](const Bytes& body) {
        return body.size() >= plan.prefix.size() &&
               std::equal(plan.prefix.begin(), plan.prefix.end(), body.begin());
    };
    const bool envelope_identity =
        has_common_prefix(b0.built.body) && has_common_prefix(b1.built.body) &&
        has_common_prefix(b2.built.body);
    if (!envelope_identity)
        throw std::runtime_error("G5B common-envelope identity failure");

    // Envelope SHA-256 (prereg I4) must be identical across arms and equal the
    // plan prefix hash; multiset SHA-256 (prereg I3) must be identical too.
    for (const MeasuredArm* m : arms) {
        if (m->built.envelope_sha256 != plan.envelope_sha256)
            throw std::runtime_error("G5B envelope sha256 mismatch");
        if (m->built.multiset_sha256 != plan.multiset_sha256)
            throw std::runtime_error("G5B multiset sha256 mismatch");
    }
    const bool hashes_identical =
        b0.built.envelope_sha256 == b1.built.envelope_sha256 &&
        b1.built.envelope_sha256 == b2.built.envelope_sha256 &&
        b0.built.multiset_sha256 == b1.built.multiset_sha256 &&
        b1.built.multiset_sha256 == b2.built.multiset_sha256;
    if (!hashes_identical)
        throw std::runtime_error("G5B invariant-hash identity failure");

    // I2: expose and gate envelope-length and token-region length identities
    // across all three arms, in addition to total body length.
    const bool envelope_len_identity =
        b0.built.envelope_len == b1.built.envelope_len &&
        b1.built.envelope_len == b2.built.envelope_len;
    const bool token_region_len_identity =
        b0.built.token_region_len == b1.built.token_region_len &&
        b1.built.token_region_len == b2.built.token_region_len;
    if (!envelope_len_identity)
        throw std::runtime_error("G5B envelope-length identity failure");
    if (!token_region_len_identity)
        throw std::runtime_error("G5B token-region-length identity failure");

    const bool all_perm =
        b0.built.permutation_ok && b1.built.permutation_ok && b2.built.permutation_ok;
    const bool all_roundtrip = b0.roundtrip && b1.roundtrip && b2.roundtrip;
    const bool all_mode_pack =
        b0.mode_pack_ok && b1.mode_pack_ok && b2.mode_pack_ok;
    if (!all_perm || !all_roundtrip)
        throw std::runtime_error("G5B hard invariant failure");
    if (!all_mode_pack)
        throw std::runtime_error("G5B mode byte pack/unpack failure");

    // I11 liveness (prereg section 5).
    const bool b1_eq_b2 = (b1.built.permutation == b2.built.permutation);
    const uint64_t b2_moved_token_count =
        moved_token_count(b1.built.permutation, b2.built.permutation);
    // Fail closed on the impossible/contradictory combination (prereg I11): when the
    // B1 and B2 permutations are identical the moved-token count is exactly zero, and
    // when they are distinct it is strictly positive. A measurement violating this is
    // an implementation bug and must never fall through to a favorable classification.
    if (b1_eq_b2 != (b2_moved_token_count == 0))
        throw std::runtime_error("G5B b1_eq_b2 / b2_moved_token_count contradiction");
    // b1_eq_b2 means the treatment changed nothing on this file (DEGENERATE,
    // section 5.1). It does NOT imply shared_ordinal_slots == 0 (r3 correction):
    // two distinct one-slot shapes legitimately share ordinal 0 while B1 and B2
    // coincide. The valid liveness invariants (shared <= max; single shape =>
    // shared == 0) are enforced once per file in build_g5b_plan.

    // Raw Brotli is context only and does not enter the B0/B1/B2 causal gate.
    const Bytes raw_br = brotli_encode(src);
    // Frozen accounting: 1 (arm selector) + uvar_len(source_len) + payload.
    const size_t raw_complete_context =
        1ull + static_cast<size_t>(uvar_len(src.size())) + raw_br.size();

    const int64_t b2_vs_b1_bytes =
        static_cast<int64_t>(b1.complete_bytes) - static_cast<int64_t>(b2.complete_bytes);
    const int64_t b2_vs_b0_bytes =
        static_cast<int64_t>(b0.complete_bytes) - static_cast<int64_t>(b2.complete_bytes);
    const int64_t b1_vs_b0_bytes =
        static_cast<int64_t>(b0.complete_bytes) - static_cast<int64_t>(b1.complete_bytes);

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
              << ",\"max_slots\":" << plan.max_slots
              << ",\"shared_ordinal_slots\":" << plan.shared_ordinal_slots
              << ",\"token_chunk_count\":" << plan.canonical.size()
              << ",\"structured_token_bytes\":" << plan.structured_token_bytes
              << ",\"common_prefix_bytes\":" << plan.prefix.size()
              << ",\"envelope_sha256\":\"" << plan.envelope_sha256 << "\""
              << ",\"canonical_token_multiset_sha256\":\"" << plan.multiset_sha256 << "\""
              << ",\"body_size_identity\":" << (same_body_size ? "true" : "false")
              << ",\"envelope_len_identity\":" << (envelope_len_identity ? "true" : "false")
              << ",\"token_region_len_identity\":" << (token_region_len_identity ? "true" : "false")
              << ",\"envelope_len\":" << b1.built.envelope_len
              << ",\"token_region_len\":" << b1.built.token_region_len
              << ",\"envelope_identity\":" << (envelope_identity ? "true" : "false")
              << ",\"invariant_hashes_identical\":" << (hashes_identical ? "true" : "false")
              << ",\"all_permutations_exact\":" << (all_perm ? "true" : "false")
              << ",\"all_mode_pack_unpack\":" << (all_mode_pack ? "true" : "false")
              << ",\"all_roundtrip\":" << (all_roundtrip ? "true" : "false")
              << ",\"b1_eq_b2\":" << (b1_eq_b2 ? "true" : "false")
              << ",\"b2_moved_token_count\":" << b2_moved_token_count;
    print_arm_json("b0_ordinal_null", b0);
    print_arm_json("b1_ordinal_floor", b1);
    print_arm_json("b2_ordinal_blocked", b2);
    std::cout << ",\"b2_vs_b1_bytes\":" << b2_vs_b1_bytes
              << ",\"b2_vs_b0_bytes\":" << b2_vs_b0_bytes
              << ",\"b1_vs_b0_bytes\":" << b1_vs_b0_bytes
              << "}\n";
    return 0;
}

static void g5b_fixture(const std::string& s, const char* label) {
    const Bytes src = bytes(s);
    const G5BPlan p = build_g5b_plan(src);
    const BuiltArm b0 = build_arm(p, G5BOrdinal::OrdinalNull);
    const BuiltArm b1 = build_arm(p, G5BOrdinal::OrdinalFloor);
    const BuiltArm b2 = build_arm(p, G5BOrdinal::OrdinalBlocked);

    // Carrier-envelope contract: every arm body must begin with the EXACT frozen G5A
    // carrier magic "G5AO" and version 1 (byte-compat requirement). B0/B2 share it
    // because the arm selector is out-of-band.
    static const std::array<uint8_t, 4> kExpectedMagic = {'G','5','A','O'};
    for (const auto* a : {&b0, &b1, &b2}) {
        if (a->body.size() < 5 ||
            !std::equal(kExpectedMagic.begin(), kExpectedMagic.end(), a->body.begin()) ||
            a->body[4] != kG5BVersion)
            throw std::runtime_error(std::string(label) +
                ": carrier body is not exact frozen G5A G5AO version 1");
    }
    // Charge/pack round-trip: the materialized selector byte must be B1==3 and must
    // not collide with G5A's 0..3 selector space for the non-floor arms.
    for (const auto* a : {&b0, &b1, &b2}) {
        Bytes packed;
        packed.push_back(static_cast<uint8_t>(a->mode));
        packed.insert(packed.end(), a->body.begin(), a->body.end());
        if (packed.size() != a->body.size() + 1)
            throw std::runtime_error(std::string(label) + ": selector packing size");
        if (packed.front() != static_cast<uint8_t>(a->mode) ||
            !is_valid_g5b_selector(packed.front()))
            throw std::runtime_error(std::string(label) + ": selector not charged/valid");
        const Bytes unpacked(packed.begin() + 1, packed.end());
        if (unpacked != a->body)
            throw std::runtime_error(std::string(label) + ": selector pack/unpack body drift");
    }
    if (static_cast<uint8_t>(b1.mode) != 3)
        throw std::runtime_error(std::string(label) + ": B1/FLOOR selector is not 3");
    if (static_cast<uint8_t>(b0.mode) == 3 || static_cast<uint8_t>(b2.mode) == 3)
        throw std::runtime_error(std::string(label) + ": B0/B2 selector aliases A3 (3)");

    if (b0.body.size() != b1.body.size() || b1.body.size() != b2.body.size())
        throw std::runtime_error(std::string(label) + ": body sizes differ");
    auto has_prefix = [&](const Bytes& body) {
        return body.size() >= p.prefix.size() &&
               std::equal(p.prefix.begin(), p.prefix.end(), body.begin());
    };
    if (!has_prefix(b0.body) || !has_prefix(b1.body) || !has_prefix(b2.body))
        throw std::runtime_error(std::string(label) + ": common envelope differs");
    for (const auto* a : {&b0, &b1, &b2}) {
        if (!a->permutation_ok)
            throw std::runtime_error(std::string(label) + ": permutation invalid");
        // Byte-level invariant hashes (prereg I3/I4).
        if (a->envelope_sha256 != p.envelope_sha256)
            throw std::runtime_error(std::string(label) + ": envelope hash mismatch");
        if (a->multiset_sha256 != p.multiset_sha256)
            throw std::runtime_error(std::string(label) + ": multiset hash mismatch");
        // I2: envelope and token-region lengths derive exactly from prefix + body.
        if (a->envelope_len != p.prefix.size())
            throw std::runtime_error(std::string(label) + ": envelope length mismatch");
        if (a->token_region_len != a->body.size() - p.prefix.size())
            throw std::runtime_error(std::string(label) + ": token region length mismatch");
        // Mode byte materialization (prereg section 2.3).
        if (!mode_pack_unpack_roundtrip(a->mode, a->body))
            throw std::runtime_error(std::string(label) + ": mode pack/unpack");
    }
    if (b0.envelope_len != b1.envelope_len || b1.envelope_len != b2.envelope_len)
        throw std::runtime_error(std::string(label) + ": envelope length not identical");
    if (b0.token_region_len != b1.token_region_len ||
        b1.token_region_len != b2.token_region_len)
        throw std::runtime_error(std::string(label) + ": token region length not identical");
    if (decode_g5b_body(b0.body, G5BOrdinal::OrdinalNull) != src)
        throw std::runtime_error(std::string(label) + ": null decode");
    if (decode_g5b_body(b1.body, G5BOrdinal::OrdinalFloor) != src)
        throw std::runtime_error(std::string(label) + ": floor decode");
    if (decode_g5b_body(b2.body, G5BOrdinal::OrdinalBlocked) != src)
        throw std::runtime_error(std::string(label) + ": blocked decode");

    const size_t expected = p.canonical.size();
    for (const auto* a : {&b0, &b1, &b2})
        if (!validate_permutation(a->permutation, expected))
            throw std::runtime_error(std::string(label) + ": exact coverage");
    // The B0 null must be deterministic: rebuilt identically.
    const BuiltArm b0b = build_arm(p, G5BOrdinal::OrdinalNull);
    if (b0.permutation != b0b.permutation || b0.body != b0b.body)
        throw std::runtime_error(std::string(label) + ": B0 not deterministic");
}

static void g5b_selftest() {
    // Re-run the complete frozen G3 selftest first. This is intentionally small
    // correctness work and ensures imported parser/shape semantics remain valid.
    selftest();

    g5b_fixture(
        "{\"a\":1,\"b\":\"x\"}\n"
        "{\"a\":2,\"b\":\"y\"}\n"
        "garbage\n"
        "{\"x\":true}\n"
        "{\"a\":3,\"b\":\"z\"}\n",
        "mixed shapes + residual");

    g5b_fixture(
        "{}\n[]\n{}\n"
        "{\"n\":1}\r\n"
        "{\"n\":2}\r\n",
        "zero scalar + CRLF");

    g5b_fixture(
        "{\"k\":\"same\",\"v\":100}\n"
        "{\"k\":\"same\",\"v\":101}\n"
        "{\"k\":\"same\",\"v\":102}\n",
        "single shape");

    g5b_fixture(
        "not-json\n"
        "still-not-json\n",
        "raw only");

    // Multi-shape, differing slot counts: B2 must genuinely lift ordinals across
    // exact shapes here (the treatment is non-vacuous on a nontrivial fixture).
    {
        const std::string multi =
            "{\"a\":1,\"b\":\"x\"}\n"
            "{\"p\":9,\"q\":\"y\",\"r\":7}\n"
            "{\"a\":2,\"b\":\"z\"}\n"
            "{\"p\":8,\"q\":\"w\",\"r\":6}\n";
        g5b_fixture(multi, "two shapes, differing slot counts");
        const Bytes src = bytes(multi);
        const G5BPlan p = build_g5b_plan(src);
        if (p.analysis.shapes.size() < 2)
            throw std::runtime_error("G5B multi fixture did not produce two shapes");
        if (p.shared_ordinal_slots < 1)
            throw std::runtime_error("G5B multi fixture has no shared ordinal slot");
        const BuiltArm b1 = build_arm(p, G5BOrdinal::OrdinalFloor);
        const BuiltArm b2 = build_arm(p, G5BOrdinal::OrdinalBlocked);
        const uint64_t moved = moved_token_count(b1.permutation, b2.permutation);
        if (moved == 0)
            throw std::runtime_error("G5B B2 equals B1 on a multi-shape fixture");
        if (b2.cross_shape_ordinal_tokens == 0)
            throw std::runtime_error("G5B B2 created no cross-shape ordinal adjacency");
    }

    // Degeneracy (prereg section 5.1): a single-shape file must be DEGENERATE
    // (B1 == B2), because every ordinal is shared by only one shape.
    {
        const std::string single =
            "{\"a\":1,\"b\":\"x\"}\n"
            "{\"a\":2,\"b\":\"y\"}\n"
            "{\"a\":3,\"b\":\"z\"}\n";
        const Bytes src = bytes(single);
        const G5BPlan p = build_g5b_plan(src);
        if (p.shared_ordinal_slots != 0)
            throw std::runtime_error("G5B single-shape fixture has shared ordinals");
        const BuiltArm b1 = build_arm(p, G5BOrdinal::OrdinalFloor);
        const BuiltArm b2 = build_arm(p, G5BOrdinal::OrdinalBlocked);
        if (b1.permutation != b2.permutation)
            throw std::runtime_error("G5B single-shape file not degenerate (B1 != B2)");
        if (b2.cross_shape_ordinal_tokens != 0)
            throw std::runtime_error("G5B degenerate file created cross-shape crossings");
    }

    // Legitimate MULTI-SHAPE degeneracy (r3 correction): two DISTINCT one-slot
    // shapes share ordinal 0 (shared_ordinal_slots == 1 > 0), yet B1 (shape-major)
    // and B2 (ordinal-major) emit the same order, so b1_eq_b2 == true and
    // b2_moved_token_count == 0. This must be VALID, never a contradiction.
    {
        const std::string two_one_slot =
            "{\"a\":1}\n"
            "{\"b\":2}\n";
        g5b_fixture(two_one_slot, "two distinct one-slot shapes");
        const Bytes src = bytes(two_one_slot);
        const G5BPlan p = build_g5b_plan(src);
        if (p.analysis.shapes.size() != 2)
            throw std::runtime_error("G5B two-one-slot fixture did not produce two shapes");
        if (p.max_slots != 1 || p.shared_ordinal_slots != 1)
            throw std::runtime_error("G5B two-one-slot fixture liveness unexpected");
        const BuiltArm b1 = build_arm(p, G5BOrdinal::OrdinalFloor);
        const BuiltArm b2 = build_arm(p, G5BOrdinal::OrdinalBlocked);
        if (b1.permutation != b2.permutation)
            throw std::runtime_error("G5B two-one-slot fixture is not degenerate");
        if (moved_token_count(b1.permutation, b2.permutation) != 0)
            throw std::runtime_error("G5B two-one-slot degenerate file moved tokens");
        if (b2.cross_shape_ordinal_tokens != 1)
            throw std::runtime_error("G5B two-one-slot fixture cross-shape count unexpected");
    }

    // Exact permutation validator must reject both duplicates and omissions.
    if (validate_permutation({0, 0}, 2))
        throw std::runtime_error("G5B permutation duplicate accepted");
    if (validate_permutation({0}, 2))
        throw std::runtime_error("G5B permutation omission accepted");
    if (!validate_permutation({}, 0))
        throw std::runtime_error("G5B empty permutation rejected");

    // SHA-256 known-answer tests: the invariant hashes depend on this.
    if (sha256::hex(bytes("")) !=
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")
        throw std::runtime_error("G5B sha256 empty KAT");
    if (sha256::hex(bytes("abc")) !=
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
        throw std::runtime_error("G5B sha256 abc KAT");

    // B0 must actually reorder a multi-chunk fixture (prereg I9).
    {
        const std::string reorderable =
            "{\"a\":1,\"b\":\"x\"}\n"
            "{\"p\":9,\"q\":\"y\"}\n"
            "{\"a\":2,\"b\":\"z\"}\n"
            "{\"p\":8,\"q\":\"w\"}\n";
        const Bytes src = bytes(reorderable);
        const G5BPlan p = build_g5b_plan(src);
        if (p.canonical.size() < 4)
            throw std::runtime_error("G5B B0 fixture too small to reorder");
        const BuiltArm b0 = build_arm(p, G5BOrdinal::OrdinalNull);
        const BuiltArm b1 = build_arm(p, G5BOrdinal::OrdinalFloor);
        if (b0.permutation == b1.permutation)
            throw std::runtime_error("G5B B0 permutation equals floor (no null)");
        const uint64_t moved = moved_token_count(b1.permutation, b0.permutation);
        if (moved == 0)
            throw std::runtime_error("G5B B0 does not reorder any token");
    }

    // Selector-byte validation (prereg I7): only the three frozen arm values
    // are valid; any other byte must be rejected deterministically.
    for (uint8_t s : {static_cast<uint8_t>(3), static_cast<uint8_t>(4),
                      static_cast<uint8_t>(5)})
        if (!is_valid_g5b_selector(s))
            throw std::runtime_error("G5B valid selector rejected");
    for (uint8_t s : {static_cast<uint8_t>(0), static_cast<uint8_t>(1),
                      static_cast<uint8_t>(2), static_cast<uint8_t>(6),
                      static_cast<uint8_t>(0x7f), static_cast<uint8_t>(0xff)})
        if (is_valid_g5b_selector(s))
            throw std::runtime_error("G5B invalid selector accepted");
    // The carrier now shares frozen G5A's "G5AO" magic. Assert the frozen G5A A3
    // selector 3 is exactly the B1/FLOOR selector, so "G5AO" + selector 3 is a
    // byte-identical G5A A3 carrier, and assert the B0/B2 selectors do not alias
    // G5A's 0..3 selector space.
    if (static_cast<uint8_t>(G5BOrdinal::OrdinalFloor) != 3)
        throw std::runtime_error("G5B B1/FLOOR selector is not the frozen G5A A3 value 3");
    if (static_cast<uint8_t>(G5BOrdinal::OrdinalNull) <= 3 ||
        static_cast<uint8_t>(G5BOrdinal::OrdinalBlocked) <= 3)
        throw std::runtime_error("G5B B0/B2 selector aliases the G5A 0..3 selector space");
    if (kG5BMagic != std::array<uint8_t, 4>{'G','5','A','O'})
        throw std::runtime_error("G5B carrier magic is not frozen G5A \"G5AO\"");
    {
        Bytes body = bytes("payload");
        Bytes packed;
        packed.push_back(0xff);  // invalid selector
        packed.insert(packed.end(), body.begin(), body.end());
        if (is_valid_g5b_selector(packed.front()))
            throw std::runtime_error("G5B invalid packed selector accepted");
    }

    std::cout << "PASS grotli_g5b_ordinal selftest\n";
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "selftest") {
            g5b_selftest();
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "measure") {
            return measure_g5b(argv[2]);
        }
        std::cerr << "usage: grotli_g5b_ordinal selftest\n"
                     "       grotli_g5b_ordinal measure INPUT\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
