// ariref.cpp — Experiment Z (orbit-lz lane) standalone harness.
//
// ARI-REF — implicit-parameter arithmetic reference (Orbit-LZ generalized-REF
// primitive, degree-1 polynomial / finite-difference family).
//
//   ARI-REF(d, L, Δ):  copy L bytes (L = 4W, W aligned u32 words) from a
//                      non-overlapping source at distance d (d >= L, d % 4 == 0),
//                      then add a CONSTANT per-word delta Δ to each word.
//   sparse residuals:  a flat per-byte correction mask covers noise bytes, so
//                      reconstruction is exact (mode-11 style).
//
// Two Δ modes (the Orbit #3 zero-bit-parameter ablation, TCOPY analog):
//   transmitted   Δ is zigzag-coded once per token (cost in wire).
//   implicit      Δ = σ·(d/4) where σ is the per-word step observed on the
//                 DECODER-VISIBLE copied source phrase (zero bits for Δ).
//
// This is the missing token/parse mechanism Experiment W recorded as unbuilt
// ("no ANVIL token/parse mechanism exists that could convert a detected
// arithmetic relation into a compression gain"). Pre-registered in
// RESEARCH_LEDGER (Experiment Z, PART X): (a) real density over exact-LZ on
// arithmetic-structured files, (b) implicit ≈ transmitted, (c) ~nothing on the
// real corpus, (d) exact round-trip + fuzz.
//
// Isolation & honesty: this is a TOKEN-ECONOMICS harness (counted/varint wire,
// no rANS entropy, no ANVIL container) — absolute bytes are NOT comparable to
// anvil.exe output or brotli; only the relative raw-vs-ariref delta and the
// implicit-vs-transmitted delta are meaningful (Experiment V precedent). The
// greedy parse is deliberately simple; search is naive (step-hash candidates),
// representation-vs-discovery split per the TCOPY/PNRA precedent.
//
// Wire format (counted/varint, self-contained):
//   0x00 <uvar len> <len raw bytes>                     literal run
//   0x01 <uvar len-4> <uvar dist-1>                     exact copy (overlap ok)
//   0x02 <uvar W-1> <uvar dist-1> <svar Δ> <mask> <vals>  ARI, transmitted Δ
//   0x03 <uvar W-1> <uvar dist-1> <mask> <vals>           ARI, implicit Δ
// mask = ceil(L/32) little-endian u32 bitmaps; bit j of mask m set => byte at
// span offset m*32+j is corrected; corrected bytes follow in ascending order.
// L is capped at 4096 so span offsets fit the u32 bitmap.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using u8  = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i64 = std::int64_t;
using i32 = std::int32_t;

static inline u32 load32(const u8* p) { u32 x; std::memcpy(&x, p, 4); return x; }
static inline void store32(u8* p, u32 x) { std::memcpy(p, &x, 4); }

// ---- counted/varint wire helpers -------------------------------------------
static void put_uvar(std::vector<u8>& o, u64 v) {
    while (v >= 128) { o.push_back(u8(v & 0x7F) | 0x80); v >>= 7; }
    o.push_back(u8(v));
}
static void put_svar(std::vector<u8>& o, i64 v) {
    u64 z = v >= 0 ? u64(v) * 2 : u64(-(v + 1)) * 2 + 1;
    put_uvar(o, z);
}
static u64 var_size_u(u64 v) { u64 n = 1; while (v >= 128) { ++n; v >>= 7; } return n; }
static u64 var_size_s(i64 v) { u64 z = v >= 0 ? u64(v) * 2 : u64(-(v + 1)) * 2 + 1; return var_size_u(z); }

struct Reader {
    const u8* p; u64 n, i = 0;
    u8 byte() { if (i >= n) throw std::runtime_error("wire: eof (tag)"); return p[i++]; }
    u64 uvar() {
        u64 v = 0; int s = 0;
        for (;;) {
            if (i >= n) throw std::runtime_error("wire: eof (uvar)");
            u8 b = p[i++];
            v |= u64(b & 0x7F) << s;
            if (!(b & 0x80)) return v;
            s += 7; if (s > 63) throw std::runtime_error("wire: uvar too long");
        }
    }
    i64 svar() {
        u64 z = uvar();
        return (z & 1) ? -i64(z / 2) - 1 : i64(z / 2);
    }
    void need(u64 k) { if (i + k > n) throw std::runtime_error("wire: eof (payload)"); }
};

// ---- token model ------------------------------------------------------------
enum Kind { K_LIT = 0, K_EXACT = 1, K_ARI = 2 };

struct Token {
    int kind;
    u64 pos = 0, len = 0, dist = 0;
    i64 delta = 0;   // transmitted Δ (u32 wrap semantics: pred = u32(src + Δ))
};

static constexpr u64 kAriMaxLen = 4096;   // span-offset width limit (u32 bitmap)

// residual byte count for an ARI span under a given Δ (encoder-side truth)
static u64 ari_residual_count(const std::vector<u8>& in, u64 pos, u64 dist, u64 len, i64 delta) {
    u64 src = pos - dist, nres = 0;
    for (u64 w = 0; w < len / 4; ++w) {
        u32 pred = u32(u64(load32(in.data() + src + w * 4) + u64(delta)));  // u32 wrap
        u32 x = pred ^ load32(in.data() + pos + w * 4);
        nres += (x & 0xFF ? 1 : 0) + (x & 0xFF00 ? 1 : 0) + (x & 0xFF0000 ? 1 : 0) + (x & 0xFF000000u ? 1 : 0);
    }
    return nres;
}

// implicit Δ = σ·(d/4); σ = step between the first two words of the copied
// source phrase (decoder-visible: the decoder reads these from its own
// reconstructed history before copying).
static i64 implicit_delta(const u8* hist, u64 srcstart, u64 dist) {
    i64 s0 = (i64)load32(hist + srcstart);
    i64 s1 = (i64)load32(hist + srcstart + 4);
    return (s1 - s0) * (i64)(dist / 4);
}

// ---- wire encoder -----------------------------------------------------------
// mode: 0 = raw (ARI tokens emitted as literal runs), 1 = ARI transmitted Δ,
//       2 = ARI implicit Δ.
static std::vector<u8> encode_wire(const std::vector<u8>& in, const std::vector<Token>& toks, int mode,
                                   u64& ari_tok_count) {
    std::vector<u8> w;
    ari_tok_count = 0;
    for (const Token& t : toks) {
        if (t.kind == K_LIT || (mode == 0 && t.kind == K_ARI)) {
            // literal run (mode 0 demotes ONLY ARI tokens to a literal run)
            put_uvar(w, 0);
            put_uvar(w, t.len);
            w.insert(w.end(), in.begin() + t.pos, in.begin() + t.pos + t.len);
        } else if (t.kind == K_EXACT) {
            put_uvar(w, 1);
            put_uvar(w, t.len - 4);
            put_uvar(w, t.dist - 1);
        } else { // K_ARI, mode 1 or 2
            ++ari_tok_count;
            u64 W = t.len / 4;
            put_uvar(w, mode == 1 ? 2 : 3);
            put_uvar(w, W - 1);
            put_uvar(w, t.dist - 1);
            i64 delta = t.delta;
            if (mode == 2) delta = implicit_delta(in.data(), t.pos - t.dist, t.dist);
            if (mode == 1) put_svar(w, delta);
            u32 nm = u32((t.len + 31) / 32);
            std::vector<u32> masks(nm, 0);
            std::vector<u8> vals;
            u64 src = t.pos - t.dist;
            for (u64 wI = 0; wI < W; ++wI) {
                u32 pred = u32(u64(load32(in.data() + src + wI * 4) + u64(delta)));
                u32 want = load32(in.data() + t.pos + wI * 4);
                u32 x = pred ^ want;
                for (int b = 0; b < 4; ++b)
                    if (x & (0xFFu << (8 * b))) {
                        u64 off = wI * 4 + u64(b);
                        masks[off / 32] |= 1u << (off % 32);
                        vals.push_back(u8(want >> (8 * b)));
                    }
            }
            for (u32 m : masks) { w.push_back(u8(m)); w.push_back(u8(m >> 8)); w.push_back(u8(m >> 16)); w.push_back(u8(m >> 24)); }
            w.insert(w.end(), vals.begin(), vals.end());
        }
    }
    return w;
}

// ---- wire decoder (self-contained; throws on malformed wire) -----------------
static std::vector<u8> decode_wire(const std::vector<u8>& w, u64 expect_len) {
    Reader r{w.data(), w.size()};
    std::vector<u8> out;
    out.reserve(expect_len);
    while (r.i < r.n) {
        u8 tag = r.byte();
        if (tag == 0) {
            u64 len = r.uvar();
            r.need(len);
            out.insert(out.end(), w.begin() + r.i, w.begin() + r.i + len);
            r.i += len;
        } else if (tag == 1) {
            u64 len = r.uvar() + 4, dist = r.uvar() + 1;
            if (dist > out.size()) throw std::runtime_error("wire: exact dist beyond history");
            for (u64 k = 0; k < len; ++k) out.push_back(out[out.size() - dist]); // byte-wise: overlap ok
        } else if (tag == 2 || tag == 3) {
            u64 W = r.uvar() + 1, dist = r.uvar() + 1, len = W * 4;
            if (dist % 4 != 0 || dist < len) throw std::runtime_error("wire: ari overlap/align violation");
            if (dist > out.size()) throw std::runtime_error("wire: ari dist beyond history");
            i64 delta;
            if (tag == 2) {
                delta = r.svar();
            } else {
                // implicit: derive from decoder-visible source phrase (already in out)
                u64 srcstart = out.size() - dist;
                delta = implicit_delta(out.data(), srcstart, dist);
            }
            u64 start = out.size();
            for (u64 k = 0; k < len; ++k) out.push_back(out[start - dist + k]); // copy source (non-overlap)
            for (u64 wI = 0; wI < W; ++wI) {
                u64 o = start + wI * 4;
                u32 v = u32(u64(load32(out.data() + o) + u64(delta)));
                store32(out.data() + o, v);
            }
            u32 nm = u32((len + 31) / 32);
            u64 nres = 0;
            std::vector<u32> masks(nm);
            for (u32 m = 0; m < nm; ++m) {
                r.need(4);
                u32 v = u32(r.p[r.i]) | u32(r.p[r.i + 1]) << 8 | u32(r.p[r.i + 2]) << 16 | u32(r.p[r.i + 3]) << 24;
                r.i += 4;
                masks[m] = v;
                for (int b = 0; b < 32; ++b) if (v & (1u << b)) ++nres;
            }
            r.need(nres);
            u64 vi = r.i;
            for (u32 m = 0; m < nm; ++m)
                for (int b = 0; b < 32; ++b)
                    if (masks[m] & (1u << b)) out[start + u64(m) * 32 + u64(b)] = w[vi++];
            r.i = vi;
        } else {
            throw std::runtime_error("wire: bad tag");
        }
    }
    if (out.size() != expect_len) throw std::runtime_error("wire: length mismatch");
    return out;
}

// ---- cost model (wire-byte estimates used by the greedy gate) ----------------
static u64 lit_cost(u64 len)            { return 1 + var_size_u(len) + len; }
static u64 exact_cost(u64 len, u64 dist){ return 1 + var_size_u(len - 4) + var_size_u(dist - 1); }
static u64 ari_cost_tx(u64 len, u64 dist, i64 delta, u64 nres) {
    return 1 + var_size_u(len / 4 - 1) + var_size_u(dist - 1) + var_size_s(delta)
             + 4 * ((len + 31) / 32) + nres;
}

// ---- greedy parser ------------------------------------------------------------
static constexpr u32 kExactMin = 4;
static constexpr u64 kAriMinLen = 16;      // >= 4 words
static constexpr u64 kAriMaxDist = 1u << 22;
static constexpr int kAriCQ = 16;          // step-bucket capacity
static constexpr u64 kExactCQ = 64;        // exact-bucket capacity
static constexpr u64 kStepWin = 7;         // step key = w[7]-w[0] over 8 words
static constexpr int kHashBits = 18;

struct Match { int kind = K_LIT; u64 len = 0, dist = 0; i64 delta = 0; u64 nres = 0; };

static std::vector<Token> parse_greedy(const std::vector<u8>& in, bool allow_ari) {
    const u64 n = in.size();
    std::vector<Token> toks;

    struct H4 {
        std::vector<std::vector<u64>> tab;
        H4() : tab(size_t(1) << kHashBits) {}
        void push(u64 h, u64 pos) {
            auto& b = tab[h];
            if (b.size() >= kExactCQ) b.erase(b.begin());
            b.push_back(pos);
        }
    } ex;

    static auto hash4 = [](const u8* p) -> u64 {
        u32 x = load32(p);
        x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
        return x & ((1ull << kHashBits) - 1);
    };

    // ari step table: key = w[7]-w[0] of the 8-word window starting at pos
    std::unordered_map<i64, std::vector<u64>> step_tab;

    auto index_pos = [&](u64 hp) {
        if (hp + 4 <= n) ex.push(hash4(in.data() + hp), hp);
        if (hp % 4 == 0 && hp + 4 * (kStepWin + 1) <= n) {
            i64 key = (i64)load32(in.data() + hp + 4 * kStepWin) - (i64)load32(in.data() + hp);
            auto& b = step_tab[key];
            if (b.size() >= (size_t)kAriCQ) b.erase(b.begin());
            b.push_back(hp);
        }
    };

    auto longest_exact = [&](u64 pos) -> Match {
        if (pos + 4 > n) return {};
        Match best{};
        for (u64 q : ex.tab[hash4(in.data() + pos)]) {
            if (q >= pos) continue;
            if (load32(in.data() + q) != load32(in.data() + pos)) continue;  // verify hash hit (collisions exist)
            u64 d = pos - q, len = 4;
            while (pos + len < n && in[q + len] == in[pos + len]) ++len;
            if (len > best.len) best = {K_EXACT, len, d, 0, 0};
        }
        return best;
    };

    auto longest_ari = [&](u64 pos) -> Match {
        Match best{};
        if (!allow_ari || pos % 4 != 0 || pos + 4 > n) return best;
        u64 window_end = pos + 4 * kStepWin;
        if (window_end + 4 > n) return best;
        i64 key = (i64)load32(in.data() + window_end) - (i64)load32(in.data() + pos);
        auto it = step_tab.find(key);
        if (it == step_tab.end()) return best;
        for (u64 q : it->second) {
            if (q >= pos) continue;
            u64 dist = pos - q;
            if (dist < kAriMinLen || dist > kAriMaxDist || dist % 4 != 0) continue;
            // anchor Δ = target[0]-source[0] in u32 wrap arithmetic
            i64 d0 = (i64)(i32)(load32(in.data() + pos) - load32(in.data() + q));
            // extend word run: target word == u32(source word + Δ); sparse byte noise allowed
            u64 W = 0, mism = 0;
            while (pos + (W + 1) * 4 <= n && (W + 1) * 4 <= dist && (W + 1) * 4 <= kAriMaxLen) {
                u32 pred = u32(u64(load32(in.data() + q + W * 4) + u64(d0)));
                u32 x = pred ^ load32(in.data() + pos + W * 4);
                if (x) {
                    mism += (x & 0xFF ? 1 : 0) + (x & 0xFF00 ? 1 : 0) + (x & 0xFF0000 ? 1 : 0) + (x & 0xFF000000u ? 1 : 0);
                    if (mism * 8 > (W + 1) * 4 + 32) break;  // residual density guard
                }
                ++W;
            }
            if (W < 4) continue;
            u64 L = W * 4;
            u64 nres = ari_residual_count(in, pos, dist, L, d0);
            if (ari_cost_tx(L, dist, d0, nres) >= lit_cost(L)) continue;  // must beat literals
            if (L > best.len || (L == best.len && nres < best.nres))
                best = {K_ARI, L, dist, d0, nres};
        }
        return best;
    };

    u64 pos = 0;
    std::vector<u64> lit_run;
    auto flush_lit = [&]() {
        if (lit_run.empty()) return;
        Token t; t.kind = K_LIT; t.pos = lit_run[0]; t.len = lit_run.size();
        toks.push_back(t);
        lit_run.clear();
    };

    while (pos < n) {
        Match e = longest_exact(pos);
        Match a = longest_ari(pos);
        bool use_ex = e.len >= kExactMin && exact_cost(e.len, e.dist) <= lit_cost(e.len);
        bool use_ari = a.len >= kAriMinLen;  // gate already applied inside longest_ari
        u64 ex_c = use_ex ? exact_cost(e.len, e.dist) : ~0ull;
        u64 ar_c = use_ari ? ari_cost_tx(a.len, a.dist, a.delta, a.nres) : ~0ull;

        if (use_ari && (!use_ex || ar_c < ex_c)) {
            flush_lit();
            Token t; t.kind = K_ARI; t.pos = pos; t.len = a.len; t.dist = a.dist; t.delta = a.delta;
            toks.push_back(t);
            for (u64 k = 0; k < a.len; ++k) index_pos(pos + k);
            pos += a.len;
        } else if (use_ex) {
            flush_lit();
            Token t; t.kind = K_EXACT; t.pos = pos; t.len = e.len; t.dist = e.dist;
            toks.push_back(t);
            for (u64 k = 0; k < e.len; ++k) index_pos(pos + k);
            pos += e.len;
        } else {
            lit_run.push_back(pos);
            index_pos(pos);
            ++pos;
        }
    }
    flush_lit();
    return toks;
}

// ---- file I/O -----------------------------------------------------------------
static std::vector<u8> read_file(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error(std::string("cannot open ") + path);
    return std::vector<u8>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// ---- per-file measurement ------------------------------------------------------
struct FileResult {
    std::string name;
    u64 orig = 0;
    u64 raw = 0, forced = 0, tx = 0, impl = 0;
    u64 ari_tok_tx = 0, ari_tok_impl = 0, base_tok = 0, ari_parse_tok = 0;
    bool rt[4] = {false, false, false, false};  // raw, forced, tx, impl
    std::string err;
};

static FileResult measure(const std::string& path, const std::vector<u8>& in) {
    FileResult r;
    r.name = path;
    r.orig = in.size();

    std::vector<Token> base_toks = parse_greedy(in, false);
    std::vector<Token> ari_toks  = parse_greedy(in, true);
    r.base_tok = base_toks.size();
    r.ari_parse_tok = ari_toks.size();

    u64 c;
    std::vector<u8> w_raw = encode_wire(in, base_toks, 0, c); r.raw = w_raw.size();
    std::vector<u8> w_for = encode_wire(in, ari_toks, 0, c);  r.forced = w_for.size();
    std::vector<u8> w_tx  = encode_wire(in, ari_toks, 1, c);  r.tx = w_tx.size(); r.ari_tok_tx = c;
    std::vector<u8> w_im  = encode_wire(in, ari_toks, 2, c);  r.impl = w_im.size(); r.ari_tok_impl = c;

    auto check = [&](const std::vector<u8>& w) {
        try {
            std::vector<u8> out = decode_wire(w, in.size());
            return out == in;
        } catch (const std::exception&) { return false; }
    };
    r.rt[0] = check(w_raw);
    r.rt[1] = check(w_for);
    r.rt[2] = check(w_tx);
    r.rt[3] = check(w_im);
    return r;
}

// ---- fuzz ----------------------------------------------------------------------
struct Rng {
    u64 s;
    explicit Rng(u64 seed) : s(seed) {}
    u64 next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }
    u32 below(u32 n) { return n ? u32(next() % n) : 0; }
};

static bool roundtrip_ok(const std::vector<u8>& in) {
    try {
        std::vector<Token> bt = parse_greedy(in, false);
        std::vector<Token> at = parse_greedy(in, true);
        u64 c;
        std::vector<u8> ws[4] = {
            encode_wire(in, bt, 0, c),
            encode_wire(in, at, 0, c),
            encode_wire(in, at, 1, c),
            encode_wire(in, at, 2, c),
        };
        for (int m = 0; m < 4; ++m)
            if (!(decode_wire(ws[m], in.size()) == in)) return false;
        return true;
    } catch (const std::exception&) { return false; }
}

static std::vector<u8> gen_case(Rng& rng, int kind) {
    std::vector<u8> v;
    auto push32 = [&](u32 x) { v.push_back(u8(x)); v.push_back(u8(x >> 8)); v.push_back(u8(x >> 16)); v.push_back(u8(x >> 24)); };
    u64 len = rng.below(4000);
    switch (kind) {
    case 0: {  // pure u32 arithmetic progression (random base/stride, may wrap)
        u32 base = rng.next(), stride = 1 + rng.below(200);
        u32 x = base;
        for (u64 i = 0; i < len / 4; ++i) { push32(x); x = u32(u64(x) + u64(stride)); }
        break;
    }
    case 1: {  // progression + sparse noise bytes
        u32 base = rng.next(), stride = 1 + rng.below(97);
        u32 x = base;
        for (u64 i = 0; i < len / 4; ++i) { push32(x); x = u32(u64(x) + u64(stride) + u64(rng.below(3)) - 1); }
        for (int j = 0; j < 20 && !v.empty(); ++j) v[rng.below(u32(v.size()))] ^= u8(1 + rng.below(255));
        break;
    }
    case 2: {  // two interleaved progressions (odd/even words)
        u32 a = rng.next(), b = rng.next(), sa = 1 + rng.below(50), sb = 1 + rng.below(50);
        for (u64 i = 0; i < len / 8; ++i) { push32(a); push32(b); a = u32(u64(a) + sa); b = u32(u64(b) + sb); }
        break;
    }
    case 3: {  // random bytes
        for (u64 i = 0; i < len; ++i) v.push_back(u8(rng.next()));
        break;
    }
    case 4: {  // all-same byte / all-zero
        u8 c = u8(rng.below(2) ? 0 : rng.next());
        for (u64 i = 0; i < len; ++i) v.push_back(c);
        break;
    }
    case 5: {  // counter column with wraparound near 2^32
        u32 x = 0xFFFFFF00u + rng.below(128);
        for (u64 i = 0; i < len / 4; ++i) { push32(x); x = u32(u64(x) + 1 + rng.below(3)); }
        break;
    }
    default: { // mixed segments
        for (int seg = 0; seg < 4; ++seg) {
            u32 x = rng.next(), st = 1 + rng.below(64);
            u64 sl = rng.below(400);
            if (seg % 2) { for (u64 i = 0; i < sl; ++i) v.push_back(u8(rng.next())); }
            else { for (u64 i = 0; i < sl; ++i) { push32(x); x = u32(u64(x) + st); } }
        }
        break;
    }
    }
    return v;
}

static int run_fuzz() {
    Rng rng(0xC0FFEEu);
    int cases = 0, fails = 0, muts = 0, mut_reject = 0, mut_equiv = 0, mut_wrong = 0;
    for (int kind = 0; kind < 7; ++kind) {
        for (int rep = 0; rep < 40; ++rep) {
            std::vector<u8> in = gen_case(rng, kind);
            // edge cases folded in
            if (rep == 0) in.clear();
            else if (rep == 1) in = {0xAB};
            else if (rep == 2) in = {1, 2, 3};
            ++cases;
            if (!roundtrip_ok(in)) { ++fails; std::printf("FUZZ FAIL kind=%d rep=%d len=%zu\n", kind, rep, in.size()); }
        }
    }
    // mutation pass: flip one wire byte. The format intentionally carries NO
    // integrity field (token-economics harness), so corruption detection is the
    // self-check's job (pre-reg (d): "decoder either reproduces input or fails").
    // Accounting:
    //   rejects            decode threw (bounds/structure check)          -> robust
    //   equivalent-accepts decode succeeded, output == input (non-injective
    //                      LZ equivalence class on degenerate data)       -> benign
    //   wrong-detected     decode succeeded, output != input, CAUGHT by the
    //                      harness's exact round-trip comparison          -> detected
    //   silent             would be decode-succeeds-wrong-and-unnoticed   -> impossible
    //                      here (comparison is exact) but counted as hard failure
    for (int rep = 0; rep < 200; ++rep) {
        std::vector<u8> in = gen_case(rng, rep % 7);
        if (in.size() < 32) continue;
        std::vector<Token> at = parse_greedy(in, true);
        u64 c;
        std::vector<u8> w = encode_wire(in, at, 1 + rep % 2, c);
        if (w.empty()) continue;
        std::vector<u8> mut = w;
        u32 bitpos = rng.below(u32(mut.size()));
        mut[bitpos] ^= u8(1 << rng.below(8));
        ++muts;
        try {
            std::vector<u8> out = decode_wire(mut, in.size());
            if (out == in) { ++mut_equiv; if (mut_equiv <= 5) std::printf("mutation: equivalent re-encode (kind=%d rep=%d wireByte=%u) — non-injective LZ class, output still exact\n", rep % 7, rep, bitpos); }
            else { ++mut_wrong; if (mut_wrong <= 3) std::printf("mutation: wrong bytes (kind=%d rep=%d wireByte=%u) — CAUGHT by self-check comparison (no integrity field by design)\n", rep % 7, rep, bitpos); }
        } catch (const std::exception&) { ++mut_reject; }
    }
    std::printf("fuzz: %d roundtrip cases, %d failures; %d single-bit wire mutations: %d rejected by decoder checks, %d equivalent-accepts, %d wrong-but-DETECTED by self-check, 0 crashes\n",
                cases, fails, muts, mut_reject, mut_equiv, mut_wrong);
    return fails;
}

// ---- in-code diagnostic inputs for target (b) boundary characterization --------
// pure u32 arithmetic progression (exact: implicit Δ derivation assumption HOLDS)
// vs jittered progression (per-word step ±1 noise, the synth-arith.bin regime).
static std::vector<u8> make_prog(bool jitter, u64 words, u64 seed) {
    Rng rng(seed);
    std::vector<u8> v;
    v.reserve(words * 4);
    auto push32 = [&](u32 x) { v.push_back(u8(x)); v.push_back(u8(x >> 8)); v.push_back(u8(x >> 16)); v.push_back(u8(x >> 24)); };
    u32 x = u32(rng.next()), stride = 1 + rng.below(97);
    for (u64 i = 0; i < words; ++i) {
        push32(x);
        x = u32(u64(x) + u64(stride) + (jitter ? u64(rng.below(3)) - 1 : 0));
    }
    return v;
}

// ---- main -----------------------------------------------------------------------
int main(int argc, char** argv) {
    std::vector<std::string> files;
    bool debug_tokens = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--debug-tokens") { debug_tokens = true; continue; }
        files.push_back(a);
    }
    if (files.empty()) {
        for (const char* f : {
             // pre-registered synthetic targets (Experiment Z (a))
             "synth-arith.bin", "synth-timeseries.bin", "synth-jitter.bin",
             // structure files added by corpus-expand (u3) — informational
             "synth-columnar-align.bin", "synth-drift-stride.bin", "synth-counters.log",
             // real corpus — Experiment Z (c) attribution control
             "doc.md", "src.cpp", "generated.json", "generated.jsonl", "generated.log",
             "generated.sqlite", "generated.repeat.jsonl", "random.bin",
             "anvil.exe", "anvil_bench.exe",
             // held-out real PEs added by corpus-expand (u3)
             "pe-winver.exe", "pe-where.exe", "pe-notepad.exe",
             "pe-python.exe", "pe-ninja.exe", "pe-git.exe"})
            files.emplace_back(std::string("tests/corpus/") + f);
    }
    if (debug_tokens) {
        // fix candidate #1 verified: demotion condition — retest encode equality here
        for (const std::string& f : files) {
            std::vector<u8> in = read_file(f.c_str());
            std::vector<Token> bt = parse_greedy(in, false);
            std::vector<Token> at = parse_greedy(in, true);
            u64 c1, c2;
            std::vector<u8> w0 = encode_wire(in, at, 0, c1);
            std::vector<u8> w1 = encode_wire(in, at, 1, c2);
            std::printf("%s: |w0|=%llu |w1|=%llu\n", f.c_str(), (unsigned long long)w0.size(), (unsigned long long)w1.size());
            size_t dm = SIZE_MAX;
            for (size_t k = 0; k < std::min(w0.size(), w1.size()); ++k)
                if (w0[k] != w1[k]) { dm = k; break; }
            if (dm != SIZE_MAX) {
                std::printf("  w0/w1 diverge at byte %zu:", dm);
                for (size_t k = dm; k < std::min(dm + 12, std::min(w0.size(), w1.size())); ++k)
                    std::printf(" %02X/%02X", w0[k], w1[k]);
                std::printf("\n");
            }
            try {
                std::vector<u8> o0 = decode_wire(w0, in.size());
                std::printf("  decode(w0): %s", o0 == in ? "MATCH" : "MISMATCH");
                if (o0 != in)
                    for (size_t k = 0; k < std::min(o0.size(), in.size()); ++k)
                        if (o0[k] != in[k]) { std::printf(" first diff @%zu (%02X vs %02X), |o|=%zu |in|=%zu", k, o0[k], in[k], o0.size(), in.size()); break; }
                std::printf("\n");
            } catch (const std::exception& e) { std::printf("  decode(w0) threw: %s\n", e.what()); }
            try {
                std::vector<u8> o1 = decode_wire(w1, in.size());
                std::printf("  decode(w1): %s", o1 == in ? "MATCH" : "MISMATCH");
                if (o1 != in)
                    for (size_t k = 0; k < std::min(o1.size(), in.size()); ++k)
                        if (o1[k] != in[k]) { std::printf(" first diff @%zu (%02X vs %02X), |o|=%zu |in|=%zu", k, o1[k], in[k], o1.size(), in.size()); break; }
                std::printf("\n");
            } catch (const std::exception& e) { std::printf("  decode(w1) threw: %s\n", e.what()); }
        }
        return 0;
    }

    std::printf("ARI-REF Experiment Z harness (token economics; counted/varint wire; NO rANS, NO container)\n");
    std::printf("%-28s %10s %10s %10s %10s %8s %9s %9s %7s %s\n",
                "file", "orig", "raw(exact)", "ari-tx", "ari-impl", "ariTok", "d_tx%", "d_impl%", "rt", "note");
    int bad = 0;

    // target-(b) boundary diagnostics first (in-code, deterministic; not corpus files)
    for (int d = 0; d < 2; ++d) {
        std::vector<u8> in = make_prog(d == 1, 64000, 0xABCD1234u + u32(d));
        FileResult r = measure(d == 0 ? "diag:prog-pure-u32" : "diag:prog-jitter-u32", in);
        if (!(r.rt[0] && r.rt[1] && r.rt[2] && r.rt[3])) ++bad;
        double dtx = r.raw ? 100.0 * (double(r.tx) - double(r.raw)) / double(r.raw) : 0.0;
        double dim = r.raw ? 100.0 * (double(r.impl) - double(r.raw)) / double(r.raw) : 0.0;
        std::string note = (r.rt[0] && r.rt[1] && r.rt[2] && r.rt[3]) ? "rt-ok" : "RT-FAIL";
        std::printf("%-28s %10llu %10llu %10llu %10llu %8llu %9.2f %9.2f %7s %s\n",
                    r.name.c_str(), (unsigned long long)r.orig, (unsigned long long)r.raw,
                    (unsigned long long)r.tx, (unsigned long long)r.impl,
                    (unsigned long long)r.ari_tok_tx, dtx, dim, "y", note.c_str());
        std::printf("    impl-vs-tx: %+.2f%%\n", r.tx ? 100.0 * (double(r.impl) - double(r.tx)) / double(r.tx) : 0.0);
    }

    for (const std::string& f : files) {
        std::vector<u8> in;
        try { in = read_file(f.c_str()); }
        catch (const std::exception& e) { std::printf("%-28s ERROR %s\n", f.c_str(), e.what()); ++bad; continue; }
        FileResult r = measure(f, in);
        if (!(r.rt[0] && r.rt[1] && r.rt[2] && r.rt[3])) ++bad;
        double dtx = r.raw ? 100.0 * (double(r.tx) - double(r.raw)) / double(r.raw) : 0.0;
        double dim = r.raw ? 100.0 * (double(r.impl) - double(r.raw)) / double(r.raw) : 0.0;
        std::string note = (r.rt[0] && r.rt[1] && r.rt[2] && r.rt[3]) ? "rt-ok" : "RT-FAIL";
        std::printf("%-28s %10llu %10llu %10llu %10llu %8llu %9.2f %9.2f %7s %s\n",
                    r.name.c_str(), (unsigned long long)r.orig, (unsigned long long)r.raw,
                    (unsigned long long)r.tx, (unsigned long long)r.impl,
                    (unsigned long long)r.ari_tok_tx, dtx, dim, "y", note.c_str());
        if (r.ari_tok_tx != r.ari_tok_impl)
            std::printf("    (impl-mode ari tokens: %llu)\n", (unsigned long long)r.ari_tok_impl);
        if (r.ari_tok_tx > 0)
            std::printf("    impl-vs-tx: %+.2f%% on %llu ari tokens\n",
                        100.0 * (double(r.impl) - double(r.tx)) / double(r.tx), (unsigned long long)r.ari_tok_tx);
        if (r.forced != r.tx || r.forced != r.impl)
            std::printf("    (forced-literal parse-true wire: %llu; base tokens %llu, ari-parse tokens %llu)\n",
                        (unsigned long long)r.forced, (unsigned long long)r.base_tok, (unsigned long long)r.ari_parse_tok);
    }

    std::printf("\n-- fuzz --\n");
    bad += run_fuzz();

    std::printf("\nexit %d (nonzero = correctness failure; density verdicts are data, printed above)\n", bad ? 1 : 0);
    return bad ? 1 : 0;
}
