// autoseg.cpp — prototypes/i9-pnra (pnra lane, iteration 9 follow-up)
//
// AUTO-DISCOVERY STUDY for the native per-region step reference. Copy of the
// FROZEN stride_ref.cpp (the PR-5 verified artifact) extended with:
//   * segment() variants: auto (greedy maximal span with a 3-consecutive-delta
//     window — the rule the PR-5 wire already used), forced run lens
//     (--force-lens=, oracle arm), and --min-step=N sensitivity;
//   * compact region table (--table=compact): drops the always-zero gap and
//     the decoder-derivable startup count; header reserved byte bit0;
//   * `stats` mode: prints the discovered region table and compares the
//     discovered boundaries against a known truth (--truth-lens=).
// stride_ref.cpp/exe remain FROZEN and unchanged as the PR-5 artifact.
//
// Mechanism (inside the reference):
//   For a discovered region R = [start, start+len) the decoder reconstructs
//   each word from the immediately preceding decoded word plus a per-region
//   integer step s and a bounded jitter residual e:
//
//       out[p+i] = out[p+i-1] + s + e[p+i],   e in {-1,0,1}
//
//   The region's first word is a literal; the residual stream carries the
//   entropy. Untransformed spans stay native (literal regions) — the transform
//   is scoped to the reference, never applied globally.
//
// Parameter provenance ablation (the PR-5 question):
//   param=0 transmitted : s sent per region (1 varint). G3-control arm.
//   param=1 derived     : s derived DECODER-SIDE from the reference's own
//                         decoded prefix, zero transmitted bits. Exact rule:
//                         the first delta fixes the alphabet position; the
//                         alphabet completes to 3 consecutive integers; the
//                         steady-state residual is exactly {-1,0,1}.
//   param=2 ddelta      : parameter-free control (second difference, al-5).
//
// Entropy-coder ablation (mechanism vs coder):
//   backend=0 pack3  : base-3 packing, 5 symbols/byte (1.6 bits/sym), fast.
//   backend=1 raw2   : 2 bits/symbol, NO entropy coder (mechanism only).
//   backend=2 brotli : symbols as bytes, brotli-compressed (external coder).
//   backend=3 pack5  : base-5 packing, 13/u32 (2.46 bits/sym) for ddelta.
//   backend=4 raw3   : 3 bits/symbol, NO entropy coder (ddelta control).
//
// EVERY parameter is charged in the wire: region gap, region length, region
// type, step (or startup count), first-word literals, first-delta literals,
// startup symbols, residual symbols, trailing bytes, header.
//
// Build (Windows host):
//   clang-cl /O2 /EHsc /std:c++20 /MD /nologo /DNDEBUG
//     /I ..\..\third_party\install\include /Festride_ref.exe stride_ref.cpp
//     /link /LIBPATH:..\..\third_party\install\lib
//     brotlienc.lib brotlidec.lib brotlicommon.lib
//
// Usage:
//   stride_ref selftest
//   stride_ref c <in> <out> [--backend=N] [--param=N] [--brotli-q=N] [--literal]
//   stride_ref d <in> <out>
//   stride_ref bench <in> [--reps=N] [--backend=N] [--param=N] [--brotli-q=N] [--literal]
//
// Output on encode/bench is one `SRESULT` line (machine-readable) plus a
// section accounting (AUDIT-7: every parameter charged).
#include <brotli/encode.h>
#include <brotli/decode.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using u8 = uint8_t; using u32 = uint32_t; using u64 = uint64_t; using i64 = int64_t;

// ---------------------------------------------------------------- io + varint
static std::vector<u8> rdfile(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) { std::fprintf(stderr, "cannot open %s\n", p.c_str()); std::exit(1); }
    return std::vector<u8>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
static void wrfile(const std::string& p, const std::vector<u8>& d) {
    std::ofstream f(p, std::ios::binary);
    if (!f) { std::fprintf(stderr, "cannot write %s\n", p.c_str()); std::exit(1); }
    f.write((const char*)d.data(), (std::streamsize)d.size());
}
static inline void put32(std::vector<u8>& o, u32 v) {
    o.push_back(u8(v)); o.push_back(u8(v >> 8)); o.push_back(u8(v >> 16)); o.push_back(u8(v >> 24));
}
static inline u32 get32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}
static void putvar(std::vector<u8>& o, u64 v) {
    while (v >= 0x80) { o.push_back(u8(v) | 0x80); v >>= 7; }
    o.push_back(u8(v));
}
static bool getvar(const u8* b, size_t n, size_t& pos, u64& v) {
    v = 0; int shift = 0;
    while (pos < n) {
        u8 c = b[pos++];
        v |= u64(c & 0x7F) << shift;
        if (!(c & 0x80)) return true;
        shift += 7;
        if (shift > 63) return false;
    }
    return false;
}
static inline u64 zz(i64 x) { return (u64(x) << 1) ^ u64(x >> 63); }
static inline i64 unzz(u64 v) { return i64(v >> 1) ^ -i64(v & 1); }

// ---------------------------------------------------------------- packers
static const u8* p3tab() {
    static u8 tab[243 * 5]; static bool init = false;
    if (!init) { for (int b = 0; b < 243; b++) { int v = b; for (int i = 0; i < 5; i++) { tab[b * 5 + i] = u8(v % 3); v /= 3; } } init = true; }
    return tab;
}
static std::vector<u8> pack3(const std::vector<u8>& s) {
    std::vector<u8> o; o.reserve((s.size() + 4) / 5);
    for (size_t i = 0; i < s.size(); i += 5) {
        u32 b = 0, pw = 1;
        for (size_t j = 0; j < 5; j++) { u32 v = (i + j < s.size()) ? s[i + j] : 0; b += v * pw; pw *= 3; }
        o.push_back(u8(b));
    }
    return o;
}
static std::vector<u8> raw2(const std::vector<u8>& s) {
    std::vector<u8> o((s.size() + 3) / 4, 0);
    for (size_t i = 0; i < s.size(); i++) o[i / 4] |= u8((s[i] & 3) << (2 * (i % 4)));
    return o;
}
static std::vector<u8> pack5(const std::vector<u8>& s) {
    std::vector<u8> o;
    for (size_t i = 0; i < s.size(); i += 13) {
        u64 b = 0, pw = 1;
        for (size_t j = 0; j < 13; j++) { u32 v = (i + j < s.size()) ? s[i + j] : 0; b += u64(v) * pw; pw *= 5; }
        put32(o, u32(b));
    }
    return o;
}
static std::vector<u8> raw3(const std::vector<u8>& s) {
    std::vector<u8> o; o.reserve(s.size());
    for (u8 v : s) o.push_back(v & 7);
    return o;
}
static std::vector<u8> brcomp(const std::vector<u8>& in, int q) {
    if (in.empty()) return {};
    size_t cap = BrotliEncoderMaxCompressedSize(in.size()) + 64;
    std::vector<u8> out(cap); size_t n = cap;
    if (!BrotliEncoderCompress(q, BROTLI_DEFAULT_WINDOW, BROTLI_MODE_GENERIC, in.size(), in.data(), &n, out.data())) {
        std::fprintf(stderr, "brotli compress failed\n"); std::exit(1);
    }
    out.resize(n); return out;
}
static std::vector<u8> brdecomp(const u8* d, size_t n, size_t raw) {
    std::vector<u8> out(raw ? raw : 1); size_t outn = raw;
    if (BrotliDecoderDecompress(n, d, &outn, out.data()) != BROTLI_DECODER_RESULT_SUCCESS || outn != raw) {
        std::fprintf(stderr, "brotli decompress failed\n"); std::exit(1);
    }
    out.resize(outn); return out;
}

// ---------------------------------------------------------------- regions
struct Region { u32 start = 0, len = 0; u8 type = 0; i64 pval = 0; };  // type 0 step, 1 literal
static const size_t kHeaderSize = 44;
static const u32 kMagic = 0x39524653u;  // 'S','R','F','9'

// maximal span of words [i, i+L) whose deltas fit a 3-consecutive window
static size_t step_span(const std::vector<u32>& w, size_t i) {
    size_t n = w.size();
    if (i + 1 >= n) return 1;
    i64 lo = i64(w[i + 1]) - i64(w[i]), hi = lo;
    size_t j = i + 2;
    while (j < n) {
        i64 d = i64(w[j]) - i64(w[j - 1]);
        i64 nlo = std::min(lo, d), nhi = std::max(hi, d);
        if (nhi - nlo <= 2) { lo = nlo; hi = nhi; ++j; } else break;
    }
    return j - i;
}
static std::vector<Region> segment(const std::vector<u32>& w, bool force_literal,
                                   const std::vector<u32>* force_lens, u32 min_step) {
    std::vector<Region> seg; size_t n = w.size();
    if (force_literal) { if (n) seg.push_back({ 0, (u32)n, 1, 0 }); return seg; }
    if (force_lens && !force_lens->empty()) {
        size_t pos = 0, k = 0;
        while (pos < n) {
            size_t L = (*force_lens)[std::min(k, force_lens->size() - 1)];
            if (L == 0) L = 1;
            if (L > n - pos) L = n - pos;
            seg.push_back({ (u32)pos, (u32)L, 0, 0 });
            pos += L; ++k;
        }
        return seg;
    }
    size_t i = 0, lit = 0;
    while (i < n) {
        size_t L = step_span(w, i);
        if (L >= min_step) {
            if (lit < i) seg.push_back({ (u32)lit, (u32)(i - lit), 1, 0 });
            seg.push_back({ (u32)i, (u32)L, 0, 0 });
            i += L; lit = i;
        } else ++i;
    }
    if (lit < n) seg.push_back({ (u32)lit, (u32)(n - lit), 1, 0 });
    return seg;
}

// ---------------------------------------------------------------- encoding
struct Encoded {
    std::vector<u8> bytes;
    size_t header = 0, lit = 0, par = 0, dfirst = 0, startup = 0, main = 0, tail = 0;
    u64 main_syms = 0, start_syms = 0, n_regions = 0, n_step_regions = 0;
};

static Encoded encode(const std::vector<u8>& raw, int backend, int pmode, int bq, bool force_literal,
                      const std::vector<u32>* force_lens = nullptr, u32 min_step = 8, bool compact = false) {
    size_t nw = raw.size() / 4;
    std::vector<u32> w(nw);
    if (nw) std::memcpy(w.data(), raw.data(), nw * 4);
    size_t tail_n = raw.size() - nw * 4;
    auto seg = segment(w, force_literal, force_lens, min_step);

    std::vector<u8> lit, par, dfirst, startup, msym;
    u64 main_syms = 0, start_syms = 0, nstep = 0;
    u32 prev_end = 0;
    for (auto& g : seg) {
        if (compact) { putvar(par, g.len); par.push_back(g.type); }
        else { putvar(par, u64(g.start) - prev_end); putvar(par, g.len); putvar(par, g.type); }
        prev_end = g.start + g.len;
        if (g.type == 1) {
            for (u32 k = g.start; k < g.start + g.len; k++) put32(lit, w[k]);
            continue;
        }
        ++nstep;
        put32(lit, w[g.start]);
        if (pmode == 0) {  // transmitted step
            i64 lo = i64(w[g.start + 1]) - i64(w[g.start]), hi = lo;
            for (u32 k = g.start + 2; k < g.start + g.len; k++) {
                i64 d = i64(w[k]) - i64(w[k - 1]);
                lo = std::min(lo, d); hi = std::max(hi, d);
            }
            i64 s = (lo + hi) / 2;
            g.pval = s;
            putvar(par, zz(s));
            for (u32 k = g.start + 1; k < g.start + g.len; k++) {
                i64 d = i64(w[k]) - i64(w[k - 1]);
                i64 e = d - s;
                if (e < -1 || e > 1) { std::fprintf(stderr, "transmitted symbol out of range\n"); std::exit(1); }
                msym.push_back(u8(e + 1)); ++main_syms;
            }
        } else if (pmode == 1) {  // derived step, zero transmitted bits
            i64 d0 = i64(w[g.start + 1]) - i64(w[g.start]);
            putvar(dfirst, zz(d0));
            i64 lo = d0, hi = d0; u64 sc = 0;
            for (u32 k = g.start + 2; k < g.start + g.len; k++) {
                i64 d = i64(w[k]) - i64(w[k - 1]);
                if (hi - lo == 2) { msym.push_back(u8(d - lo)); ++main_syms; }
                else {
                    u8 sym;
                    if (d == lo) sym = 0;
                    else if (d == lo + 1) sym = 1;
                    else if (d == lo - 1) sym = 2;
                    else if (d == lo + 2) sym = 3;
                    else if (d == lo - 2) sym = 4;
                    else { std::fprintf(stderr, "derived startup rule violated (lo=%lld hi=%lld d=%lld)\n", (long long)lo, (long long)hi, (long long)d); std::exit(1); }
                    startup.push_back(sym); ++sc; ++start_syms;
                    lo = std::min(lo, d); hi = std::max(hi, d);
                    if (hi - lo > 2) { std::fprintf(stderr, "segmentation invariant broken\n"); std::exit(1); }
                }
            }
            g.pval = (i64)sc;
            if (!compact) putvar(par, sc);
        } else {  // ddelta (parameter-free control)
            i64 d0 = i64(w[g.start + 1]) - i64(w[g.start]);
            putvar(dfirst, zz(d0));
            i64 dprev = d0;
            for (u32 k = g.start + 2; k < g.start + g.len; k++) {
                i64 dk = i64(w[k]) - i64(w[k - 1]);
                i64 c = dk - dprev;
                if (c < -2 || c > 2) { std::fprintf(stderr, "ddelta out of range\n"); std::exit(1); }
                msym.push_back(u8(c + 2)); ++main_syms;
                dprev = dk;
            }
            if (!compact) putvar(par, 0);
        }
    }
    // pack main symbols
    std::vector<u8> mainb; u32 main_raw = 0;
    int alpha = (pmode == 2) ? 5 : 3;
    switch (backend) {
        case 0:
            if (alpha != 3) { std::fprintf(stderr, "pack3 needs ternary symbols\n"); std::exit(1); }
            mainb = pack3(msym); break;
        case 1:
            if (alpha != 3) { std::fprintf(stderr, "raw2 needs ternary symbols\n"); std::exit(1); }
            mainb = raw2(msym); break;
        case 2:
            main_raw = (u32)msym.size(); mainb = brcomp(msym, bq); break;
        case 3:
            if (alpha != 5) { std::fprintf(stderr, "pack5 needs 5-ary symbols\n"); std::exit(1); }
            mainb = pack5(msym); break;
        case 4:
            if (alpha != 5) { std::fprintf(stderr, "raw3 needs 5-ary symbols\n"); std::exit(1); }
            mainb = raw3(msym); break;
        default: std::fprintf(stderr, "bad backend\n"); std::exit(1);
    }
    std::vector<u8> startb;
    for (size_t i = 0; i < startup.size(); i += 2) {
        u8 b = startup[i] & 0xF;
        if (i + 1 < startup.size()) b |= u8((startup[i + 1] & 0xF) << 4);
        startb.push_back(b);
    }
    // header + sections
    Encoded e;
    e.header = kHeaderSize;
    e.lit = lit.size(); e.par = par.size(); e.dfirst = dfirst.size();
    e.startup = startb.size(); e.main = mainb.size(); e.tail = tail_n;
    e.main_syms = main_syms; e.start_syms = start_syms; e.n_regions = seg.size(); e.n_step_regions = nstep;

    auto& o = e.bytes; o.reserve(kHeaderSize + lit.size() + par.size() + dfirst.size() + startb.size() + mainb.size() + tail_n);
    put32(o, kMagic); o.push_back(1); o.push_back(u8(backend)); o.push_back(u8(pmode)); o.push_back(compact ? 1 : 0);
    put32(o, (u32)nw); put32(o, (u32)seg.size());
    put32(o, (u32)lit.size()); put32(o, (u32)par.size()); put32(o, (u32)dfirst.size());
    put32(o, (u32)startb.size()); put32(o, (u32)mainb.size()); put32(o, main_raw); put32(o, (u32)tail_n);
    o.insert(o.end(), lit.begin(), lit.end());
    o.insert(o.end(), par.begin(), par.end());
    o.insert(o.end(), dfirst.begin(), dfirst.end());
    o.insert(o.end(), startb.begin(), startb.end());
    o.insert(o.end(), mainb.begin(), mainb.end());
    if (tail_n) o.insert(o.end(), raw.end() - tail_n, raw.end());
    return e;
}

// ---------------------------------------------------------------- decoding
static std::vector<u8> decode(const std::vector<u8>& b) {
    if (b.size() < kHeaderSize || get32(b.data()) != kMagic) {
        std::fprintf(stderr, "bad header\n"); std::exit(1);
    }
    const u8* h = b.data();
    u8 backend = h[5], pmode = h[6], flags = h[7];
    bool compact = (flags & 1) != 0;
    u32 nw = get32(h + 8), nr = get32(h + 12);
    u32 lit_len = get32(h + 16), par_len = get32(h + 20), df_len = get32(h + 24);
    u32 st_len = get32(h + 28), mn_len = get32(h + 32), mn_raw = get32(h + 36), tl_len = get32(h + 40);
    size_t need = kHeaderSize + (size_t)lit_len + par_len + df_len + st_len + mn_len + tl_len;
    if (b.size() != need) { std::fprintf(stderr, "size mismatch %zu vs %zu\n", b.size(), need); std::exit(1); }
    const u8* lit = b.data() + kHeaderSize;
    const u8* par = lit + lit_len;
    const u8* df = par + par_len;
    const u8* st = df + df_len;
    const u8* mn = st + st_len;
    const u8* tl = mn + mn_len;

    std::vector<Region> seg; seg.reserve(nr);
    size_t ppos = 0; u64 prev_end = 0;
    for (u32 r = 0; r < nr; r++) {
        u64 gap = 0, len, type;
        if (compact) {
            if (!getvar(par, par_len, ppos, len) || ppos >= par_len) { std::fprintf(stderr, "params corrupt\n"); std::exit(1); }
            type = par[ppos++];
        } else if (!getvar(par, par_len, ppos, gap) || !getvar(par, par_len, ppos, len) || !getvar(par, par_len, ppos, type)) {
            std::fprintf(stderr, "params corrupt\n"); std::exit(1);
        }
        Region g; g.start = compact ? (u32)prev_end : (u32)(prev_end + gap); g.len = (u32)len; g.type = (u8)type;
        if (g.type == 0) {
            if (!compact || pmode == 0) {
                u64 pv;
                if (!getvar(par, par_len, ppos, pv)) { std::fprintf(stderr, "param corrupt\n"); std::exit(1); }
                g.pval = (pmode == 0) ? unzz(pv) : i64(pv);
            } else g.pval = 0;
        }
        prev_end = u64(g.start) + g.len;
        seg.push_back(g);
    }
    std::vector<u8> mnbuf;
    if (backend == 2 && mn_raw) mnbuf = brdecomp(mn, mn_len, mn_raw);

    const u8* T = p3tab();
    size_t litpos = 0, dfpos = 0, stpos = 0; u64 midx = 0;
    auto read_lit32 = [&]() -> u32 { if (litpos + 4 > lit_len) { std::fprintf(stderr, "lit overrun\n"); std::exit(1); } u32 v = get32(lit + litpos); litpos += 4; return v; };
    auto read_df = [&]() -> i64 { u64 v; if (!getvar(df, df_len, dfpos, v)) { std::fprintf(stderr, "dfirst overrun\n"); std::exit(1); } return unzz(v); };
    auto next_main = [&]() -> u8 {
        u8 v = 0;
        switch (backend) {
            case 0: { size_t byte = (size_t)(midx / 5); if (byte >= mn_len) { std::fprintf(stderr, "main overrun\n"); std::exit(1); } v = T[(size_t)mn[byte] * 5 + (midx % 5)]; break; }
            case 1: { size_t byte = (size_t)(midx / 4); if (byte >= mn_len) { std::fprintf(stderr, "main overrun\n"); std::exit(1); } v = (mn[byte] >> (2 * (midx % 4))) & 3; break; }
            case 2: { if (midx >= mnbuf.size()) { std::fprintf(stderr, "main overrun\n"); std::exit(1); } v = mnbuf[midx]; break; }
            case 3: { size_t word = (size_t)(midx / 13); if (word * 4 + 4 > mn_len) { std::fprintf(stderr, "main overrun\n"); std::exit(1); } u32 x = get32(mn + word * 4); size_t j = (size_t)(midx % 13); for (size_t i = 0; i < j; i++) x /= 5; v = u8(x % 5); break; }
            case 4: { size_t byte = (size_t)midx; if (byte >= mn_len) { std::fprintf(stderr, "main overrun\n"); std::exit(1); } v = mn[byte] & 7; break; }
        }
        ++midx; return v;
    };
    auto next_start = [&]() -> u8 {
        size_t byte = stpos / 2;
        if (byte >= st_len) { std::fprintf(stderr, "startup overrun\n"); std::exit(1); }
        u8 v = (stpos % 2) ? (st[byte] >> 4) : (st[byte] & 0xF);
        ++stpos; return v;
    };

    std::vector<u32> out; out.reserve(nw);
    for (auto& g : seg) {
        if (g.type == 1) {
            for (u32 k = 0; k < g.len; k++) out.push_back(read_lit32());
            continue;
        }
        u32 v0 = read_lit32();
        out.push_back(v0);
        if (g.len == 1) continue;
        if (pmode == 0) {
            i64 s = g.pval;
            for (u32 k = 1; k < g.len; k++) {
                u8 sym = next_main();
                if (sym > 2) { std::fprintf(stderr, "bad ternary symbol sym=%u idx=%llu backend=%d pmode=%d rstart=%u k=%u\n", sym, (unsigned long long)(midx - 1), backend, pmode, g.start, k); std::exit(1); }
                i64 d = s + (i64(sym) - 1);
                out.push_back(u32(i64(out.back()) + d));
            }
        } else if (pmode == 1) {
            i64 d0 = read_df();
            out.push_back(u32(i64(out.back()) + d0));
            i64 lo = d0, hi = d0;
            for (u32 k = 2; k < g.len; k++) {
                i64 d;
                if (hi - lo == 2) {
                    u8 sym = next_main();
                    if (sym > 2) { std::fprintf(stderr, "bad ternary symbol(derived) sym=%u idx=%llu backend=%d rstart=%u k=%u\n", sym, (unsigned long long)(midx - 1), backend, g.start, k); std::exit(1); }
                    d = lo + sym;
                } else {
                    u8 sym = next_start();
                    if (sym == 0) d = lo;
                    else if (sym == 1) d = lo + 1;
                    else if (sym == 2) d = lo - 1;
                    else if (sym == 3) d = lo + 2;
                    else d = lo - 2;
                    lo = std::min(lo, d); hi = std::max(hi, d);
                }
                out.push_back(u32(i64(out.back()) + d));
            }
        } else {
            i64 dprev = read_df();
            out.push_back(u32(i64(out.back()) + dprev));
            for (u32 k = 2; k < g.len; k++) {
                u8 sym = next_main();
                if (sym > 4) { std::fprintf(stderr, "bad 5-ary symbol\n"); std::exit(1); }
                dprev += i64(sym) - 2;
                out.push_back(u32(i64(out.back()) + dprev));
            }
        }
    }
    if (out.size() != nw) { std::fprintf(stderr, "word count mismatch\n"); std::exit(1); }
    std::vector<u8> outb; outb.reserve(out.size() * 4 + tl_len);
    for (u32 v : out) put32(outb, v);
    outb.insert(outb.end(), tl, tl + tl_len);
    return outb;
}

// ---------------------------------------------------------------- helpers
static double now_us() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double, std::micro>(clock::now().time_since_epoch()).count();
}
static double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}
static const char* backend_name(int b) {
    switch (b) { case 0: return "pack3"; case 1: return "raw2"; case 2: return "brotli"; case 3: return "pack5"; case 4: return "raw3"; }
    return "?";
}
static const char* param_name(int p) {
    switch (p) { case 0: return "transmitted"; case 1: return "derived"; case 2: return "ddelta"; }
    return "?";
}
struct Opts {
    int backend = 0, pmode = 1, bq = 11, reps = 7;
    bool literal = false, compact = false;
    u32 min_step = 8;
    std::vector<u32> force_lens, truth_lens;
    std::string in, out;
};

static std::vector<u32> parse_lens(const std::string& s) {
    std::vector<u32> v; size_t p = 0;
    while (p < s.size()) {
        size_t q = s.find(',', p);
        if (q == std::string::npos) q = s.size();
        std::string tok = s.substr(p, q - p);
        if (!tok.empty()) v.push_back((u32)std::strtoul(tok.c_str(), nullptr, 10));
        p = q + 1;
    }
    return v;
}

static void parse_opts(int argc, char** argv, int start, Opts& o) {
    for (int i = start; i < argc; i++) {
        std::string a = argv[i];
        if (a.rfind("--backend=", 0) == 0) o.backend = std::atoi(a.c_str() + 10);
        else if (a.rfind("--param=", 0) == 0) o.pmode = std::atoi(a.c_str() + 8);
        else if (a.rfind("--brotli-q=", 0) == 0) o.bq = std::atoi(a.c_str() + 11);
        else if (a.rfind("--reps=", 0) == 0) o.reps = std::atoi(a.c_str() + 7);
        else if (a.rfind("--min-step=", 0) == 0) o.min_step = (u32)std::atoi(a.c_str() + 11);
        else if (a.rfind("--force-lens=", 0) == 0) o.force_lens = parse_lens(a.substr(13));
        else if (a.rfind("--truth-lens=", 0) == 0) o.truth_lens = parse_lens(a.substr(13));
        else if (a.rfind("--table=", 0) == 0) o.compact = (a.substr(8) == "compact");
        else if (a == "--literal") o.literal = true;
        else if (o.in.empty()) o.in = a;
        else o.out = a;
    }
}
static void print_sections(const Encoded& e, size_t in_bytes) {
    std::printf("  sections: header=%zu literals=%zu params=%zu dfirst=%zu startup=%zu main=%zu tail=%zu\n",
                e.header, e.lit, e.par, e.dfirst, e.startup, e.main, e.tail);
    std::printf("  regions=%llu step_regions=%llu main_syms=%llu startup_syms=%llu wire=%zu ratio=%.6f\n",
                (unsigned long long)e.n_regions, (unsigned long long)e.n_step_regions,
                (unsigned long long)e.main_syms, (unsigned long long)e.start_syms,
                e.bytes.size(), in_bytes ? double(e.bytes.size()) / double(in_bytes) : 0.0);
}

// ---------------------------------------------------------------- selftest
static bool roundtrip_check(const std::vector<u8>& raw, int backend, int pmode, bool literal, const char* tag, bool compact = false) {
    Encoded e = encode(raw, backend, pmode, 11, literal, nullptr, 8, compact);
    std::vector<u8> back = decode(e.bytes);
    if (back != raw) { std::fprintf(stderr, "SELFTEST FAIL %s backend=%d param=%d lit=%d\n", tag, backend, pmode, (int)literal); return false; }
    return true;
}
static int selftest() {
    std::vector<std::vector<u8>> cases;
    // arithmetic runs (the synth-arith shape)
    { std::vector<u8> v; u32 x = 12345; for (int r = 0; r < 5; r++) { u32 s = 1 + (r * 17) % 97; for (int i = 0; i < 500; i++) { x += s + ((int(x) % 3) - 1); put32(v, x); } } cases.push_back(v); }
    // constant words
    { std::vector<u8> v; for (int i = 0; i < 1000; i++) put32(v, 777); cases.push_back(v); }
    // random words
    { std::vector<u8> v; u32 x = 0x12345678; for (int i = 0; i < 1000; i++) { x ^= x << 13; x ^= x >> 17; x ^= x << 5; put32(v, x); } cases.push_back(v); }
    // mixed runs + random
    { std::vector<u8> v; u32 x = 999; for (int i = 0; i < 300; i++) { x += 7 + ((int(x) % 3) - 1); put32(v, x); } u32 y = 0xdeadbeef; for (int i = 0; i < 300; i++) { y ^= y << 13; y ^= y >> 17; y ^= y << 5; put32(v, y); } x = 5; for (int i = 0; i < 300; i++) { x += 3 + ((int(x) % 3) - 1); put32(v, x); } cases.push_back(v); }
    // non-multiple-of-4 size (only when the corpus file is reachable)
    { std::vector<u8> v; for (const char* p : { "tests/corpus/synth-arith.bin", "../../tests/corpus/synth-arith.bin" }) { std::ifstream f(p, std::ios::binary); if (f) { v.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()); break; } } if (!v.empty()) { v.resize(v.size() - 3); cases.push_back(v); } }
    // empty
    { cases.push_back({}); }
    bool ok = true;
    for (size_t c = 0; c < cases.size(); c++) {
        char tag[64]; std::snprintf(tag, sizeof tag, "case%zu", c);
        for (int b : {0, 1, 2}) for (int p : {0, 1}) ok &= roundtrip_check(cases[c], b, p, false, tag);
        for (int b : {2, 3, 4}) ok &= roundtrip_check(cases[c], b, 2, false, tag);
        for (int b : {0, 1, 2}) ok &= roundtrip_check(cases[c], b, 1, true, tag);
        for (int b : {0, 1, 2}) ok &= roundtrip_check(cases[c], b, 1, false, tag, true);  // compact table
    }
    std::printf("selftest %s (%zu cases x arms)\n", ok ? "PASS" : "FAIL", cases.size());
    return ok ? 0 : 1;
}

// ---------------------------------------------------------------- main
int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: autoseg selftest | stats <in> [--truth-lens=..] | c <in> <out> [opts] | d <in> <out> | bench <in> [opts]\n"
                             "  opts: --backend=N --param=N --brotli-q=N --reps=N --literal --table=compact --min-step=N --force-lens=a,b,c\n");
        return 2;
    }
    std::string mode = argv[1];
    if (mode == "selftest") return selftest();
    if (mode == "stats") {
        Opts o; parse_opts(argc, argv, 2, o);
        if (o.in.empty()) { std::fprintf(stderr, "stats needs <in>\n"); return 2; }
        auto raw = rdfile(o.in);
        size_t nw = raw.size() / 4;
        std::vector<u32> w(nw);
        if (nw) std::memcpy(w.data(), raw.data(), nw * 4);
        auto seg = segment(w, o.literal, (o.force_lens.empty() ? nullptr : &o.force_lens), o.min_step);
        u64 step_regions = 0, lit_regions = 0, step_words = 0, lit_words = 0;
        for (auto& g : seg) { if (g.type == 0) { ++step_regions; step_words += g.len; } else { ++lit_regions; lit_words += g.len; } }
        std::printf("SEGSTATS file=%s bytes=%zu words=%zu tail=%zu regions=%zu step_regions=%llu literal_regions=%llu step_words=%llu literal_words=%llu min_step=%u\n",
                    o.in.c_str(), raw.size(), nw, raw.size() - nw * 4, seg.size(),
                    (unsigned long long)step_regions, (unsigned long long)lit_regions,
                    (unsigned long long)step_words, (unsigned long long)lit_words, o.min_step);
        size_t shown = 0;
        for (size_t ri = 0; ri < seg.size() && shown < 30; ri++) {
            auto& g = seg[ri];
            if (g.type != 0) continue;
            i64 dmin = 0, dmax = 0; bool first = true;
            std::vector<i64> ds; ds.reserve(std::min<size_t>(g.len, 4096));
            for (u32 k = g.start + 1; k < g.start + g.len && ds.size() < 4096; k++) {
                i64 d = i64(w[k]) - i64(w[k - 1]);
                if (first) { dmin = dmax = d; first = false; }
                dmin = std::min(dmin, d); dmax = std::max(dmax, d);
                ds.push_back(d);
            }
            std::sort(ds.begin(), ds.end());
            size_t distinct = (size_t)(std::unique(ds.begin(), ds.end()) - ds.begin());
            std::printf("REGION idx=%zu start=%u len=%u type=step dmin=%lld dmax=%lld width=%lld distinct=%zu\n",
                        ri, g.start, g.len, (long long)dmin, (long long)dmax, (long long)(dmax - dmin), distinct);
            ++shown;
        }
        if (!o.truth_lens.empty()) {
            std::vector<u64> tb; u64 acc = 0;
            for (size_t i = 0; i + 1 < o.truth_lens.size(); i++) { acc += o.truth_lens[i]; tb.push_back(acc); }
            std::vector<u64> db;
            for (auto& g : seg) if (g.start > 0) db.push_back(g.start);
            std::sort(db.begin(), db.end()); db.erase(std::unique(db.begin(), db.end()), db.end());
            size_t exact = 0, missing = 0, spurious = 0;
            for (u64 b : tb) { auto it = std::lower_bound(db.begin(), db.end(), b); if (it != db.end() && *it == b) ++exact; else ++missing; }
            for (u64 b : db) { auto it = std::lower_bound(tb.begin(), tb.end(), b); if (it == tb.end() || *it != b) ++spurious; }
            size_t split_runs = 0;
            u64 acc2 = 0;
            for (size_t i = 0; i < o.truth_lens.size(); i++) {
                u64 a = acc2; acc2 += o.truth_lens[i]; u64 b = std::min<u64>(acc2, nw);
                size_t sr = 0;
                for (auto& g : seg) if (g.type == 0 && g.start < b && u64(g.start) + g.len > a) ++sr;
                if (sr > 1) ++split_runs;
            }
            std::printf("TRUTH boundaries=%zu discovered=%zu exact=%zu missing=%zu spurious=%zu truth_runs=%zu split_runs=%zu\n",
                        tb.size(), db.size(), exact, missing, spurious, o.truth_lens.size(), split_runs);
        }
        return 0;
    }
    if (mode == "c") {
        Opts o; parse_opts(argc, argv, 2, o);
        if (o.in.empty() || o.out.empty()) { std::fprintf(stderr, "c needs <in> <out>\n"); return 2; }
        auto raw = rdfile(o.in);
        auto t0 = now_us();
        Encoded e = encode(raw, o.backend, o.pmode, o.bq, o.literal, o.force_lens.empty() ? nullptr : &o.force_lens, o.min_step, o.compact);
        auto t1 = now_us();
        wrfile(o.out, e.bytes);
        auto back = decode(e.bytes);
        bool identical = (back == raw);
        std::printf("SRESULT op=encode file=%s backend=%s param=%s literal=%d in_bytes=%zu wire_bytes=%zu ratio=%.6f enc_ms=%.3f roundtrip=%s\n",
                    o.in.c_str(), backend_name(o.backend), param_name(o.pmode), (int)o.literal,
                    raw.size(), e.bytes.size(), raw.size() ? double(e.bytes.size()) / raw.size() : 0.0,
                    (t1 - t0) / 1000.0, identical ? "PASS" : "FAIL");
        print_sections(e, raw.size());
        return identical ? 0 : 1;
    }
    if (mode == "d") {
        Opts o; parse_opts(argc, argv, 2, o);
        if (o.in.empty() || o.out.empty()) { std::fprintf(stderr, "d needs <in> <out>\n"); return 2; }
        auto b = rdfile(o.in);
        auto t0 = now_us();
        auto outb = decode(b);
        auto t1 = now_us();
        wrfile(o.out, outb);
        std::printf("SRESULT op=decode in=%s out_bytes=%zu dec_ms=%.3f\n", o.in.c_str(), outb.size(), (t1 - t0) / 1000.0);
        return 0;
    }
    if (mode == "bench") {
        Opts o; parse_opts(argc, argv, 2, o);
        if (o.in.empty()) { std::fprintf(stderr, "bench needs <in>\n"); return 2; }
        auto raw = rdfile(o.in);
        Encoded e = encode(raw, o.backend, o.pmode, o.bq, o.literal, o.force_lens.empty() ? nullptr : &o.force_lens, o.min_step, o.compact);
        auto back = decode(e.bytes);
        bool identical = (back == raw);
        std::vector<double> enc_ms, dec_ms;
        for (int i = 0; i < o.reps; i++) {
            auto t0 = now_us(); Encoded e2 = encode(raw, o.backend, o.pmode, o.bq, o.literal, o.force_lens.empty() ? nullptr : &o.force_lens, o.min_step, o.compact); auto t1 = now_us();
            double ms = (t1 - t0) / 1000.0;
            enc_ms.push_back(ms);
            (void)e2;
            std::printf("SREP enc i=%d ms=%.4f\n", i, ms);
        }
        for (int i = 0; i < o.reps; i++) {
            auto t0 = now_us(); auto outb = decode(e.bytes); auto t1 = now_us();
            double ms = (t1 - t0) / 1000.0;
            dec_ms.push_back(ms);
            std::printf("SREP dec i=%d ms=%.4f\n", i, ms);
            if (outb != raw) { std::fprintf(stderr, "bench roundtrip fail\n"); return 1; }
        }
        double em = median(enc_ms), dm = median(dec_ms);
        auto cv = [](const std::vector<double>& v) {
            double m = 0; for (double x : v) m += x; m /= v.size();
            double s = 0; for (double x : v) s += (x - m) * (x - m);
            s = std::sqrt(s / v.size());
            return m > 0 ? 100.0 * s / m : 0.0;
        };
        double mb = double(raw.size()) / 1e6;
        double dminms = *std::min_element(dec_ms.begin(), dec_ms.end());
        double dmaxms = *std::max_element(dec_ms.begin(), dec_ms.end());
        double eminms = *std::min_element(enc_ms.begin(), enc_ms.end());
        double emaxms = *std::max_element(enc_ms.begin(), enc_ms.end());
        std::printf("SRESULT op=bench file=%s backend=%s param=%s literal=%d in_bytes=%zu wire_bytes=%zu ratio=%.6f reps=%d enc_MBps_median=%.3f dec_MBps_median=%.3f dec_MBps_min=%.3f dec_MBps_max=%.3f enc_cv_pct=%.2f dec_cv_pct=%.2f roundtrip=%s\n",
                    o.in.c_str(), backend_name(o.backend), param_name(o.pmode), (int)o.literal,
                    raw.size(), e.bytes.size(), raw.size() ? double(e.bytes.size()) / raw.size() : 0.0,
                    o.reps, mb / (em / 1e3), mb / (dm / 1e3),
                    mb / (dmaxms / 1e3), mb / (dminms / 1e3), cv(enc_ms), cv(dec_ms),
                    identical ? "PASS" : "FAIL");
        std::printf("SDETAIL enc_MBps_min=%.3f enc_MBps_max=%.3f\n",
                    mb / (emaxms / 1e3), mb / (eminms / 1e3));
        print_sections(e, raw.size());
        return identical ? 0 : 1;
    }
    std::fprintf(stderr, "unknown mode %s\n", mode.c_str());
    return 2;
}
