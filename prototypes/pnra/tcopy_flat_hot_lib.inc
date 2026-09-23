#pragma once
// Minimal standalone flat-token codec + support types for the PNRA prototypes
// (tcopy_pnra.cpp / tcopy_pnra_event.cpp / tcopy_pnra_pair.cpp).
//
// This header does NOT exist in the repo's history (confirmed via
// `git log --all --diff-filter=A` — see RESEARCH_LEDGER.md Experiment U,
// finding 1) and was never ported from whatever Linux-side session produced
// the PNRA prototypes. Rather than guess at a byte-exact original, this is a
// fresh, self-contained reconstruction of exactly the surface the three
// prototype files reference (FParse/FTok/GPatch/FlatIndex/FlatGate,
// enc_flat/dec_cd, uvlen/load32, anvil::Match/kNoPos/kHashBits/kHashSize/
// match_length/read_file). Semantics (field/patch correction meaning) are
// derived directly from how PNRA::verify()/PNRAPair::verify() consume the
// fields/patches it produces — not guessed — so encode/decode round-trip
// correctly by construction.

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace anvil {

static constexpr uint32_t kHashBits = 18;
static constexpr uint32_t kHashSize = 1u << kHashBits;
static constexpr uint32_t kNoPos = 0xFFFFFFFFu;

struct Match { uint32_t len = 0, dist = 0; };

static inline uint32_t match_length(const uint8_t* a, const uint8_t* b, uint32_t maxlen) {
    uint32_t n = 0;
    while (n + 8 <= maxlen) {
        uint64_t x, y; std::memcpy(&x, a + n, 8); std::memcpy(&y, b + n, 8);
        uint64_t d = x ^ y;
        if (d) { n += uint32_t(std::countr_zero(d) >> 3); return std::min(n, maxlen); }
        n += 8;
    }
    while (n < maxlen && a[n] == b[n]) ++n;
    return n;
}

static std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + path);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// A single-byte literal-overwrite exception at a transformed-copy offset —
// used where a byte inside an otherwise-transform-consistent span matches
// neither the raw copy nor the 4-byte field-translation relation.
struct GPatch { uint32_t off = 0; uint8_t byte = 0; };

} // namespace anvil

static inline uint32_t load32(const uint8_t* p) { uint32_t x; std::memcpy(&x, p, 4); return x; }

static inline size_t uvlen(uint32_t v) { size_t n = 1; while (v >= 0x80) { v >>= 7; ++n; } return n; }
static inline void uvput(std::vector<uint8_t>& o, uint32_t v) {
    while (v >= 0x80) { o.push_back(uint8_t(v) | 0x80); v >>= 7; }
    o.push_back(uint8_t(v));
}
static inline uint32_t uvget(const uint8_t*& p) {
    uint32_t v = 0; int s = 0;
    for (;;) { uint8_t b = *p++; v |= uint32_t(b & 0x7f) << s; if (!(b & 0x80)) break; s += 7; }
    return v;
}

// Flat parse token. kind: 0=literal run (len raw bytes follow at pos in the
// source), 1=exact backward copy (len,dist), 2=transformed backward copy
// (len,dist plus nf field corrections + np byte patches, indices into
// FParse::fields/patches at [fo,fo+nf)/[po,po+np)).
struct FTok {
    uint8_t kind = 0;
    uint32_t pos = 0, len = 0, dist = 0;
    uint8_t nf = 0, np = 0;
    uint32_t fo = 0, po = 0;
};

struct FParse {
    std::vector<FTok> t;
    std::vector<uint32_t> fields;   // token-relative 4-byte-aligned field offsets
    std::vector<anvil::GPatch> patches; // token-relative single-byte patch offsets
};

// Shared candidate-verification scan: given a copy source q and destination p
// (q<p, dist=p-q), extends a match byte-by-byte, opportunistically absorbing
// 4-byte-aligned "translation-invariant" mismatches (raw - dist == target,
// i.e. a relocation-style field that shifts by exactly -dist when the site
// moves forward by dist) as free field corrections, and isolated one-byte
// mismatches as patches, tracking the running (len - encode-cost) gain.
// Cand must have members: pos,len,dist (uint32_t), nf,np (uint8_t),
// f (array of uint32_t, >=32), p (array of anvil::GPatch, >=8), gain (double).
template <class Cand>
static Cand flat_verify(const std::vector<uint8_t>& d, uint32_t p, uint32_t q, uint32_t cap) {
    Cand best;
    if (q >= p) return best;
    uint32_t dist = p - q;
    if (dist < 32) return best;
    uint32_t maxl = std::min<uint32_t>({cap, uint32_t(d.size() - p), uint32_t(d.size() - q), dist, 65535u});
    if (maxl < 32) return best;
    std::array<uint32_t, 32> fields{};
    std::array<anvil::GPatch, 8> patches{};
    uint8_t nf = 0, np = 0;
    uint32_t j = 0;
    auto consider = [&](uint32_t L) {
        if (L < 32 || nf < 2 || np > 8) return;
        size_t cmd = 1 + uvlen(L - 8) + uvlen(dist - 1) + uvlen(nf) + uvlen(np);
        uint32_t pf = 0; for (uint8_t k = 0; k < nf; k++) { cmd += uvlen(fields[k] - pf); pf = fields[k] + 4; }
        uint32_t pp = 0; for (uint8_t k = 0; k < np; k++) { cmd += uvlen(patches[k].off - pp) + 1; pp = patches[k].off + 1; }
        double gain = double(L) - double(cmd) - .35 * np;
        if (gain > best.gain) {
            best.pos = q; best.len = L; best.dist = dist; best.nf = nf; best.np = np; best.gain = gain;
            std::copy_n(fields.data(), nf, best.f.data());
            std::copy_n(patches.data(), np, best.p.data());
        }
    };
    while (j < maxl) {
        bool found = false;
        while (j + 8 <= maxl) {
            uint64_t a, b; std::memcpy(&a, d.data() + q + j, 8); std::memcpy(&b, d.data() + p + j, 8);
            uint64_t x = a ^ b;
            if (!x) { j += 8; continue; }
            j += uint32_t(std::countr_zero(x) >> 3); found = true; break;
        }
        if (!found) { while (j < maxl && d[q + j] == d[p + j]) ++j; if (j >= maxl) break; }
        consider(j);
        if (j + 4 <= maxl && nf < fields.size()) {
            uint32_t a = load32(d.data() + q + j), b = load32(d.data() + p + j);
            if (uint32_t(a - dist) == b) { fields[nf++] = j; j += 4; continue; }
        }
        if (np >= 8) break;
        patches[np++] = {j, d[p + j]};
        ++j;
    }
    consider(j);
    return best;
}

// Plain exact-match hash index (8-byte hash w/ K-way bucket + 4-byte
// fallback table — same shape as PNRA's own bucket) that ALSO evaluates the
// transformed-copy candidate (via flat_verify) at each bucket hit, seeded
// from ordinary byte-content hashing rather than PNRA's sparse relocation
// events. This is the "raw"/baseline discovery path the PNRA prototypes
// compare against.
struct FlatIndex {
    struct C { uint32_t pos = 0, len = 0, dist = 0; uint8_t nf = 0, np = 0; std::array<uint32_t, 32> f{}; std::array<anvil::GPatch, 8> p{}; double gain = -1e30; };
    static constexpr int K = 4;
    const std::vector<uint8_t>& d;
    struct B { std::array<uint32_t, K> p; B() { p.fill(anvil::kNoPos); } };
    std::vector<B> b;
    std::vector<uint32_t> s4;
    static inline void hashes(const uint8_t* p, uint32_t& h8, uint32_t& h4) {
        uint64_t x; std::memcpy(&x, p, 8); uint32_t y = uint32_t(x);
        h4 = (y * 0x9E3779B1u) >> 16;
        x ^= x >> 32; x *= 0x9E3779B185EBCA87ULL;
        h8 = uint32_t(x >> (64 - anvil::kHashBits));
    }
    explicit FlatIndex(const std::vector<uint8_t>& x) : d(x), b(anvil::kHashSize), s4(1u << 16, anvil::kNoPos) {}
    void insert_range(uint32_t a, uint32_t e) {
        const uint32_t le = d.size() >= 8 ? uint32_t(d.size() - 7) : 0;
        for (uint32_t p = a; p < e; p++) {
            if (p < le) {
                uint32_t h8, h4; hashes(d.data() + p, h8, h4); auto& z = b[h8];
                for (int kk = K - 1; kk > 0; --kk) z.p[kk] = z.p[kk - 1];
                z.p[0] = p; s4[h4] = p;
            } else if (p + 4 <= d.size()) {
                uint32_t y; std::memcpy(&y, d.data() + p, 4); s4[(y * 0x9E3779B1u) >> 16] = p;
            }
        }
    }
    std::pair<anvil::Match, C> find(uint32_t pos, uint32_t cap) const {
        anvil::Match ex{0, 0}; C best;
        uint32_t mcap = std::min<uint32_t>(cap, uint32_t(d.size() - pos));
        if (pos + 8 <= d.size()) {
            uint32_t h8, h4; hashes(d.data() + pos, h8, h4); const auto& z = b[h8];
            for (uint32_t k = 0; k < K; k++) {
                uint32_t q = z.p[k];
                if (q == anvil::kNoPos) break;
                if (q >= pos) continue;
                uint32_t ml = std::min<uint32_t>(mcap, uint32_t(d.size() - q));
                uint32_t l = anvil::match_length(d.data() + q, d.data() + pos, ml);
                if (l >= 8 && (l > ex.len || (l == ex.len && pos - q < ex.dist))) ex = {l, pos - q};
                auto c = flat_verify<C>(d, pos, q, cap);
                if (c.gain > best.gain) best = c;
            }
            if (ex.len < 8) {
                uint32_t q = s4[h4];
                if (q != anvil::kNoPos && q < pos) {
                    uint32_t l = anvil::match_length(d.data() + q, d.data() + pos, std::min<uint32_t>(15, mcap));
                    if (l >= 4) ex = {l, pos - q};
                }
            }
        } else if (pos + 4 <= d.size()) {
            uint32_t y; std::memcpy(&y, d.data() + pos, 4); uint32_t q = s4[(y * 0x9E3779B1u) >> 16];
            if (q != anvil::kNoPos && q < pos) {
                uint32_t l = anvil::match_length(d.data() + q, d.data() + pos, std::min<uint32_t>(15, mcap));
                if (l >= 4) ex = {l, pos - q};
            }
        }
        return {ex, best};
    }
};

// Cheap local plausibility pre-filter ahead of the (more expensive) exact
// index lookup — a 2-byte popularity sketch, in the spirit of the pre-filter
// pattern validated in Experiment U (orbit_density_bench): reject positions
// whose leading 2 bytes have never been seen as a match anchor before doing
// the full 8-byte hash + bucket walk.
struct FlatGate {
    const std::vector<uint8_t>& d;
    std::vector<uint8_t> seen; // 1<<16 popcount-style seen bitmap over leading 2 bytes
    explicit FlatGate(const std::vector<uint8_t>& x) : d(x), seen(1u << 16, 0) {}
    bool plausible(uint32_t pos) const {
        if (pos + 2 > d.size()) return true;
        uint16_t k; std::memcpy(&k, d.data() + pos, 2);
        return seen[k] != 0;
    }
    void insert_range(uint32_t a, uint32_t e) {
        for (uint32_t p = a; p < e; p++) {
            if (p + 2 > d.size()) break;
            uint16_t k; std::memcpy(&k, d.data() + p, 2);
            seen[k] = 1;
        }
    }
};

// Encode an FParse to a flat byte stream. Tag byte per token: 0=literal
// (uvlen(len) + raw bytes), 1=exact copy (uvlen(len-4)+uvlen(dist-1)),
// 2=transformed copy (uvlen(len-8)+uvlen(dist-1)+uvlen(nf)+uvlen(np) then
// delta-coded field offsets, then delta-coded patch offsets+literal byte).
// Format is self-contained (carries token stream + literal bytes); it is a
// measurement harness output, not a production ANVIL container.
static std::vector<uint8_t> enc_flat(const std::vector<uint8_t>& d, const FParse& r) {
    std::vector<uint8_t> o;
    uvput(o, uint32_t(d.size()));
    uvput(o, uint32_t(r.t.size()));
    for (const auto& x : r.t) {
        if (x.kind == 0) {
            o.push_back(0); uvput(o, x.len);
            o.insert(o.end(), d.begin() + x.pos, d.begin() + x.pos + x.len);
        } else if (x.kind == 1) {
            o.push_back(1); uvput(o, x.len - 4); uvput(o, x.dist - 1);
        } else {
            o.push_back(2);
            uvput(o, x.len - 8); uvput(o, x.dist - 1); uvput(o, x.nf); uvput(o, x.np);
            uint32_t pf = 0;
            for (uint32_t k = 0; k < x.nf; k++) { uint32_t off = r.fields[x.fo + k]; uvput(o, off - pf); pf = off + 4; }
            uint32_t pp = 0;
            for (uint32_t k = 0; k < x.np; k++) { const auto& pt = r.patches[x.po + k]; uvput(o, pt.off - pp); pp = pt.off + 1; o.push_back(pt.byte); }
        }
    }
    return o;
}

#ifdef TCOPY_FLAT_WITH_BROTLI
// Only pulled in by the two mains (tcopy_pnra_event.cpp / tcopy_pnra_pair.cpp)
// that report a brotli-q4 reference column; requires linking brotlienc.
#include <brotli/encode.h>
static size_t brotli_size(const std::vector<uint8_t>& d, int quality) {
    size_t cap = BrotliEncoderMaxCompressedSize(d.size()) + 64;
    std::vector<uint8_t> out(cap);
    size_t n = cap;
    if (!BrotliEncoderCompress(quality, BROTLI_DEFAULT_WINDOW, BROTLI_MODE_GENERIC, d.size(), d.data(), &n, out.data()))
        throw std::runtime_error("brotli encode failed");
    return n;
}
#endif

static std::vector<uint8_t> dec_cd(const uint8_t* p, size_t n, size_t /*orig_hint*/) {
    const uint8_t* cur = p; const uint8_t* end = p + n;
    uint32_t total = uvget(cur);
    uint32_t ntok = uvget(cur);
    std::vector<uint8_t> out; out.reserve(total);
    for (uint32_t i = 0; i < ntok && cur < end; i++) {
        uint8_t tag = *cur++;
        if (tag == 0) {
            uint32_t len = uvget(cur);
            out.insert(out.end(), cur, cur + len);
            cur += len;
        } else if (tag == 1) {
            uint32_t len = uvget(cur) + 4;
            uint32_t dist = uvget(cur) + 1;
            uint32_t start = uint32_t(out.size()) - dist;
            for (uint32_t k = 0; k < len; k++) out.push_back(out[start + k]);
        } else {
            uint32_t len = uvget(cur) + 8;
            uint32_t dist = uvget(cur) + 1;
            uint32_t nf = uvget(cur);
            uint32_t np = uvget(cur);
            uint32_t base = uint32_t(out.size());
            uint32_t start = base - dist;
            for (uint32_t k = 0; k < len; k++) out.push_back(out[start + k]);
            uint32_t pf = 0;
            for (uint32_t k = 0; k < nf; k++) {
                uint32_t off = uvget(cur) + pf; pf = off + 4;
                uint32_t raw; std::memcpy(&raw, out.data() + base + off, 4);
                uint32_t corrected = raw - dist;
                std::memcpy(out.data() + base + off, &corrected, 4);
            }
            uint32_t pp = 0;
            for (uint32_t k = 0; k < np; k++) {
                uint32_t off = uvget(cur) + pp; pp = off + 1;
                out[base + off] = *cur++;
            }
        }
    }
    return out;
}
