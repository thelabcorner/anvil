// prof_i9.cpp -- decode-perf (I9): mode-15 decode time-share decomposition, t3 method.
// READ-ONLY w.r.t. src/anvil.cpp: this harness includes a SNAPSHOT COPY of it
// (anvil_snapshot.cpp, frozen by build_i9.ps1 with sha256 provenance).
//
// Method (reproduced from prototypes/profile_tmp/prof.cpp, I8 t3):
//   [1] baseline: median-of-7 INTERLEAVED full decompress() of an ANV0 container
//   [2] census: per-block mode + per-stream (9) codec / wire bytes / raw bytes
//   [3] stage ablation: instrumented copy of decode_tokens_hotop_fused with
//       variants full / no-lit-pull / no-copy / no-macro / setup-only /
//       opcode-pull-only. A_FULL is verified byte-identical to the real decoder
//       per block (hard failure otherwise).
//   [4] raw-stream counterfactuals: stream i stored raw, full decompress re-timed
//   [5] per-stream in-block pull + materialize cost (ns/B, cyc/B)
//   [6] fixed costs: crc32 slicing-by-8 (current tree) + crc32 bytewise
//       (the I8 t3 44% measurement base) + output memcpy.
//       residual = baseline - blockloop - crc - memcpy
//       (= block concat + output alloc + container header parsing)
// Timing: QPC seconds + invariant-TSC cycles, HIGH priority, pinned core,
// interleaved reps so host-load drift hits all variants equally. CV reported.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>
#include <map>
#include <filesystem>

#define ANVIL_NO_MAIN
// Snapshot header is macro-indirected so HEAD-based and worktree-based builds coexist.
#ifndef ANVIL_SNAPSHOT_NAME
#define ANVIL_SNAPSHOT_NAME anvil_snapshot.cpp
#endif
#define ANVIL_STR2(x) #x
#define ANVIL_STR(x) ANVIL_STR2(x)
#include ANVIL_STR(ANVIL_SNAPSHOT_NAME)

using namespace anvil;

// Snapshot API compatibility: HEAD fc23d9a decompress() takes only the container;
// the I9 worktree added an Options parameter (decode_threads).
#ifdef PROF_HEAD_SNAPSHOT
static std::vector<uint8_t> call_decompress(const std::vector<uint8_t>& c, const Options&) { return decompress(c); }
#else
static std::vector<uint8_t> call_decompress(const std::vector<uint8_t>& c, const Options& o) { return decompress(c, o); }
#endif

// ---------------------------------------------------------------- timing ----
static double g_qpc_freq = 0.0;
static double now_s() { LARGE_INTEGER c; QueryPerformanceCounter(&c); return double(c.QuadPart) / g_qpc_freq; }
static inline uint64_t rdtsc() { return __rdtsc(); }
static volatile uint64_t g_sink = 0;

struct Stats { double med = 0, mean = 0, cv = 0, mn = 0; int n = 0; };
static Stats stats_of(std::vector<double> v) {
    Stats s; s.n = (int)v.size();
    if (v.empty()) return s;
    std::sort(v.begin(), v.end());
    s.med = v[v.size() / 2];
    s.mn = v.front();
    double sum = 0; for (double x : v) sum += x;
    s.mean = sum / v.size();
    double var = 0; for (double x : v) var += (x - s.mean) * (x - s.mean);
    var /= v.size();
    s.cv = s.mean > 0 ? std::sqrt(var) / s.mean : 0;
    return s;
}

struct Variant { std::string name; std::function<double()> run; };
static std::string g_rep_dir, g_rep_label, g_rep_tag;   // raw per-rep dump (PR-4 field 4)
static std::vector<Stats> bench_interleaved(std::vector<Variant>& vs, int warmup, int reps) {
    for (auto& v : vs) for (int i = 0; i < warmup; ++i) (void)v.run();
    std::vector<std::vector<double>> samples(vs.size());
    for (int r = 0; r < reps; ++r)
        for (size_t i = 0; i < vs.size(); ++i)
            samples[i].push_back(vs[i].run());
    if (!g_rep_dir.empty()) {
        std::ofstream f(g_rep_dir + "/" + g_rep_label + "_reps_" + g_rep_tag + ".csv", std::ios::app);
        f << "variant,rep,seconds\n";
        for (size_t i = 0; i < samples.size(); ++i)
            for (size_t r = 0; r < samples[i].size(); ++r)
                f << vs[i].name << "," << r << "," << std::fixed << samples[i][r] << "\n";
    }
    std::vector<Stats> out;
    for (auto& s : samples) out.push_back(stats_of(s));
    return out;
}

// ---- bytewise CRC32: the I8 t3 measurement base (classic table) ------------
static uint32_t crc32_bytewise(const uint8_t* p, size_t n) {
    static std::array<uint32_t, 256> t = [] {
        std::array<uint32_t, 256> a{};
        for (uint32_t i = 0; i < 256; ++i) { uint32_t c = i; for (int k = 0; k < 8; ++k) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1); a[i] = c; }
        return a;
    }();
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) c = t[(c ^ p[i]) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

// ---- slicing-by-8 CRC32 (the current worktree implementation; included here so
// BOTH CRC bases can be timed on the same data regardless of snapshot state).
// Bit-exactness vs crc32_bytewise is checked and reported.
static uint32_t crc32_slice8_local(const uint8_t* p, size_t n) {
    static std::array<std::array<uint32_t, 256>, 8> qtabs = [] {
        std::array<std::array<uint32_t, 256>, 8> t{};
        for (uint32_t i = 0; i < 256; ++i) { uint32_t c = i; for (int k = 0; k < 8; ++k) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1); t[7][i] = c; }
        for (int s = 0; s < 8; ++s) {
            int steps = 7 - s;
            for (uint32_t i = 0; i < 256; ++i) {
                uint32_t c = t[7][i];
                for (int r = 0; r < steps; ++r) c = (t[7][c & 0xFFu]) ^ (c >> 8);
                t[s][i] = c;
            }
        }
        return t;
    }();
    uint32_t c = 0xFFFFFFFFu;
    while (n >= 8) {
        uint32_t one = c ^ ((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
        uint32_t two = (uint32_t)p[4] | ((uint32_t)p[5] << 8) | ((uint32_t)p[6] << 16) | ((uint32_t)p[7] << 24);
        c = qtabs[0][one & 0xFFu] ^ qtabs[1][(one >> 8) & 0xFFu] ^ qtabs[2][(one >> 16) & 0xFFu] ^ qtabs[3][one >> 24]
          ^ qtabs[4][two & 0xFFu] ^ qtabs[5][(two >> 8) & 0xFFu] ^ qtabs[6][(two >> 16) & 0xFFu] ^ qtabs[7][two >> 24];
        p += 8; n -= 8;
    }
    for (size_t i = 0; i < n; ++i) c = qtabs[7][(c ^ p[i]) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

// ------------------------------------------------------- container parsing ----
struct BlockRec { uint64_t blen = 0; int mode = 0; size_t plen = 0; uint32_t crc = 0; const uint8_t* payload = nullptr; };
static std::vector<BlockRec> parse_blocks(const std::vector<uint8_t>& comp) {
    const uint8_t* p = comp.data(); const uint8_t* e = p + comp.size();
    if (comp.size() < 5 || std::memcmp(p, "ANV0", 4) != 0 || (p[4] != 1 && p[4] != 2)) throw std::runtime_error("not ANV0 rev1/rev2");
    p += 5;
    uint64_t block_size = get_uvar(p, e);
    uint64_t total = get_uvar(p, e);
    (void)block_size; (void)total;
    std::vector<BlockRec> blocks;
    size_t outsz = 0;
    while (outsz < total) {
        BlockRec br;
        br.blen = get_uvar(p, e);
        br.mode = *p++;
        br.plen = (size_t)get_uvar(p, e);
        br.crc = get_u32le(p, e);
        br.payload = p;
        blocks.push_back(br);
        p += br.plen;
        outsz += (size_t)br.blen;
    }
    return blocks;
}

// Split a mode-15 payload: book header + 9 substream spans (+ their raw decode).
struct Split15 {
    uint32_t num_states = 0; uint64_t K = 0;
    std::vector<uint8_t> bookhdr;
    std::array<std::vector<uint8_t>, 9> wire;
    std::array<std::vector<uint8_t>, 9> raw;
    std::array<int, 9> codec{};
};
static Split15 split15(const uint8_t* p, size_t n, size_t out_len) {
    const uint8_t* e = p + n;
    Split15 s;
    const uint8_t* p0 = p;
    if (p >= e) throw std::runtime_error("truncated hotop header");
    s.num_states = *p++;
    if (s.num_states != 1 && s.num_states != 2 * kShapeClasses) throw std::runtime_error("bad hotop state count");
    s.K = get_uvar(p, e);
    if (s.K > kHotMaxOps) throw std::runtime_error("bad hotop book size");
    for (uint64_t i = 0; i < s.K; ++i) {
        if (p >= e) throw std::runtime_error("truncated hotop book");
        (void)*p++;
        (void)get_uvar(p, e);
        if (p >= e) throw std::runtime_error("truncated hotop book");
        (void)*p++;
    }
    s.bookhdr.assign(p0, p);
    const size_t max_sub = 16 * out_len + 64;
    for (int i = 0; i < 9; ++i) {
        uint64_t zn = get_uvar(p, e);
        if (zn > uint64_t(e - p)) throw std::runtime_error("truncated substream");
        s.wire[i].assign(p, p + zn);
        const uint8_t* q = p; const uint8_t* qe = p + zn;
        s.codec[i] = (zn > 0) ? q[0] : -1;
        s.raw[i] = decode_stream(q, qe, max_sub);
        if (q != qe) throw std::runtime_error("substream trailing bytes");
        p += zn;
    }
    return s;
}

static const char* STREAM_NAMES[9] = {
    "opcodes", "macro-types", "macro-ll", "macro-ml", "macro-dflags",
    "macro-dvar", "literals", "macro-masks", "macro-resid"
};
static const char* codec_name(int c) {
    switch (c) {
    case 0: return "raw"; case 1: return "rans4096"; case 2: return "rans512";
    case 3: return "rans256"; case 4: return "huffman"; case 5: return "defexc";
    case 6: return "ctx-rans"; case 7: return "repair"; case 8: return "rlz";
    default: return "?";
    }
}

// Rebuild comp with the given mode-15 streams (bit i of mask) stored raw.
static size_t uvar_len(uint64_t x) { size_t n = 1; while (x >= 0x80) { x >>= 7; ++n; } return n; }
static std::vector<uint8_t> rebuild_with_raw(const std::vector<uint8_t>& comp, uint32_t mask) {
    std::vector<BlockRec> blocks = parse_blocks(comp);
    std::vector<uint8_t> out;
    out.reserve(comp.size() + comp.size() / 4);
    out.insert(out.end(), comp.begin(), comp.begin() + 5);
    {
        const uint8_t* p = comp.data() + 5; const uint8_t* e = comp.data() + comp.size();
        uint64_t block_size = get_uvar(p, e);
        uint64_t total = get_uvar(p, e);
        put_uvar(out, block_size); put_uvar(out, total);
    }
    for (auto& br : blocks) {
        put_uvar(out, br.blen);
        out.push_back((uint8_t)br.mode);
        if (br.mode != 15 || mask == 0) {
            put_uvar(out, br.plen); put_u32le(out, br.crc);
            out.insert(out.end(), br.payload, br.payload + br.plen);
            continue;
        }
        Split15 s = split15(br.payload, br.plen, (size_t)br.blen);
        std::vector<uint8_t> np;
        np.insert(np.end(), s.bookhdr.begin(), s.bookhdr.end());
        for (int i = 0; i < 9; ++i) {
            if (mask & (1u << i)) {
                put_uvar(np, s.raw[i].size() + 1 + uvar_len(s.raw[i].size()));
                np.push_back(0); put_uvar(np, s.raw[i].size());
                np.insert(np.end(), s.raw[i].begin(), s.raw[i].end());
            } else {
                put_uvar(np, s.wire[i].size());
                np.insert(np.end(), s.wire[i].begin(), s.wire[i].end());
            }
        }
        put_uvar(out, np.size()); put_u32le(out, br.crc);
        out.insert(out.end(), np.begin(), np.end());
    }
    return out;
}

// -------------------------------------------- instrumented fused decode ----
struct HotCounters {
    uint64_t hot_lit = 0, hot_match = 0, mac_lit = 0, mac_m1 = 0, mac_m2 = 0;
    uint64_t lit_bytes = 0, copy_bytes = 0, opcode_bytes = 0;
    uint64_t tokens = 0;
};
enum Abl { A_FULL = 0, A_NO_LIT, A_NO_COPY, A_NO_MACRO, A_SETUP_ONLY, A_OPCODE_ONLY };
static const char* abl_name(int a) {
    switch (a) {
    case A_FULL: return "full"; case A_NO_LIT: return "no-lit-pull";
    case A_NO_COPY: return "no-copy"; case A_NO_MACRO: return "no-macro";
    case A_SETUP_ONLY: return "setup-only"; case A_OPCODE_ONLY: return "opcode-pull-only";
    }
    return "?";
}

template <int AB>
static std::vector<uint8_t> prof_fused(const uint8_t* p, size_t n, size_t out_len, HotCounters& c) {
    const uint8_t* e = p + n;
    if (p >= e) throw std::runtime_error("truncated hotop header");
    uint32_t num_states = *p++;
    if (num_states != 1 && num_states != 2 * kShapeClasses) throw std::runtime_error("bad hotop state count");
    uint64_t K = get_uvar(p, e);
    if (K > kHotMaxOps) throw std::runtime_error("bad hotop book size");
    std::vector<HotOp> book(K);
    for (uint64_t i = 0; i < K; ++i) {
        if (p >= e) throw std::runtime_error("truncated hotop book");
        uint8_t kind = *p++;
        if (kind > 1) throw std::runtime_error("bad hotop kind");
        uint64_t len = get_uvar(p, e);
        if (p >= e) throw std::runtime_error("truncated hotop book");
        uint8_t shape = *p++;
        if (len == 0 || len > kSparseMaxLen) throw std::runtime_error("bad hotop len");
        if (kind == 1 && shape >= 2 * kShapeClasses) throw std::runtime_error("bad hotop shape");
        book[i] = {kind, static_cast<uint32_t>(len), shape};
    }
    const size_t max_sub = 16 * out_len + 64;
    StreamPull pop, plit;
    std::array<StreamPull, 7> mp;
    std::array<std::vector<uint8_t>, 7> mv;
    auto parse_stream = [&](StreamPull& sp, std::vector<uint8_t>& storage, bool eager) {
        uint64_t zn = get_uvar(p, e);
        if (zn > uint64_t(e - p)) throw std::runtime_error("truncated substream");
        const uint8_t* q = p; const uint8_t* qe = p + zn;
        if (eager) { storage = decode_stream(q, qe, max_sub); if (q != qe) throw std::runtime_error("substream trailing bytes"); }
        else { sp.parse(q, qe, max_sub); if (q != qe) throw std::runtime_error("substream trailing bytes"); }
        p += zn;
    };
    parse_stream(pop, mv[0], false);    // 0 opcodes (pull)
    parse_stream(mp[0], mv[0], true);   // 1 macro types
    parse_stream(mp[1], mv[1], true);   // 2 macro ll
    parse_stream(mp[2], mv[2], true);   // 3 macro ml
    parse_stream(mp[3], mv[3], true);   // 4 macro dflags
    parse_stream(mp[4], mv[4], true);   // 5 macro dvar
    parse_stream(plit, mv[5], false);   // 6 literals (pull)
    parse_stream(mp[5], mv[5], true);   // 7 macro masks
    parse_stream(mp[6], mv[6], true);   // 8 macro resid
    if (p != e) throw std::runtime_error("payload trailing bytes");
    if (AB == A_SETUP_ONLY) { g_sink += pop.total + plit.total; return {}; }
    if (AB == A_OPCODE_ONLY) {
        uint8_t op;
        while (pop.next_byte(op)) { ++c.opcode_bytes; g_sink += op; }
        return {};
    }
    size_t ip_mt = 0, ip_mll = 0, ip_mml = 0, ip_mdf = 0, ip_mdv = 0, ip_mask = 0, ip_res = 0;
    std::array<uint32_t, 2 * kShapeClasses> last{};
    std::vector<uint8_t> out(out_len);
    size_t pos = 0;
    uint8_t op;
    while (pop.next_byte(op)) {
        ++c.opcode_bytes; ++c.tokens;
        if (pos >= out_len) throw std::runtime_error("too many tokens");
        if (op < K) {
            const HotOp& b = book[op];
            if (b.kind == 0) {
                if (b.len > out_len - pos) throw std::runtime_error("bad hotop literal run");
                if (AB != A_NO_LIT) {
                    if (!plit.pull_bytes(out.data() + pos, b.len)) throw std::runtime_error("truncated hotop literals");
                }
                ++c.hot_lit; c.lit_bytes += b.len;
                pos += b.len;
            } else {
                uint32_t dist = last[b.shape];
                if (dist == 0 || dist > pos || b.len > out_len - pos) throw std::runtime_error("bad hotop match");
                if (AB != A_NO_COPY) {
                    uint8_t* o = out.data();
                    if (dist >= b.len) { std::memcpy(o + pos, o + pos - dist, b.len); }
                    else for (uint64_t k = 0; k < b.len; ++k) o[pos + k] = o[pos + k - dist];
                }
                ++c.hot_match; c.copy_bytes += b.len;
                pos += b.len;
            }
        } else if (op == K) {
            if (ip_mt >= mv[0].size()) throw std::runtime_error("truncated macro types");
            uint8_t type = mv[0][ip_mt++];
            const bool no_write = (AB == A_NO_MACRO);
            if (type == 0) {
                uint64_t len = read_varint_bytes(mv[1], ip_mll) + 1;
                if (len > out_len - pos) throw std::runtime_error("bad macro literal");
                if (!no_write && AB != A_NO_LIT) {
                    if (!plit.pull_bytes(out.data() + pos, static_cast<size_t>(len))) throw std::runtime_error("truncated macro literals");
                }
                ++c.mac_lit; c.lit_bytes += len;
                pos += static_cast<size_t>(len);
            } else if (type == 1 || type == 2) {
                uint64_t len = read_varint_bytes(mv[2], ip_mml) + 4;
                if (len > kSparseMaxLen || len > out_len - pos) throw std::runtime_error("bad macro match");
                if (ip_mdf >= mv[3].size()) throw std::runtime_error("truncated macro flags");
                uint8_t flag = mv[3][ip_mdf++];
                uint32_t shape = shape_index(type, static_cast<uint32_t>(len), num_states);
                uint32_t& ld = last[shape];
                uint32_t dist;
                if (flag == 0) { uint64_t dv = read_varint_bytes(mv[4], ip_mdv); if (dv >= 0xFFFFFFFFull) throw std::runtime_error("bad macro abs dist"); dist = static_cast<uint32_t>(dv) + 1; }
                else if (flag == 1) { if (ld == 0) throw std::runtime_error("macro reuse before absolute"); dist = ld; }
                else if (flag == 2) { if (ld == 0) throw std::runtime_error("macro delta before absolute"); uint64_t zz = read_varint_bytes(mv[4], ip_mdv); int64_t dlt = (zz & 1) ? -int64_t((zz + 1) >> 1) : int64_t(zz >> 1); int64_t dd = int64_t(ld) + dlt; if (dd <= 0 || dd > 0xFFFFFFFFll) throw std::runtime_error("bad macro delta"); dist = static_cast<uint32_t>(dd); }
                else throw std::runtime_error("bad macro flag");
                if (dist == 0 || dist > pos) throw std::runtime_error("invalid macro dist");
                ld = dist;
                if (!no_write && AB != A_NO_COPY) {
                    uint8_t* o = out.data();
                    for (uint64_t k = 0; k < len; ++k) o[pos + k] = o[pos + k - dist];
                }
                if (type == 1) ++c.mac_m1; else ++c.mac_m2;
                c.copy_bytes += len;
                pos += static_cast<size_t>(len);
                if (type == 2) {
                    uint64_t nwords = (len + 31) / 32;
                    if (nwords * 4 > mv[5].size() - ip_mask) throw std::runtime_error("truncated macro mask");
                    std::array<uint32_t, (kSparseMaxLen + 31) / 32> words{};
                    uint32_t pc = 0;
                    for (uint64_t w = 0; w < nwords; ++w) {
                        uint32_t m = uint32_t(mv[5][ip_mask]) | (uint32_t(mv[5][ip_mask + 1]) << 8) | (uint32_t(mv[5][ip_mask + 2]) << 16) | (uint32_t(mv[5][ip_mask + 3]) << 24);
                        ip_mask += 4;
                        uint32_t first = uint32_t(w * 32);
                        if (first + 32 > len) { uint32_t over = first + 32 - len; if ((m >> (32 - over)) != 0) throw std::runtime_error("mask bits beyond copy length"); }
                        words[w] = m;
                        pc += std::popcount(m);
                    }
                    if (pc > mv[6].size() - ip_res) throw std::runtime_error("truncated macro residual");
                    if (!no_write) {
                        size_t start = pos - static_cast<size_t>(len);
                        for (uint64_t w = 0; w < nwords; ++w) {
                            uint32_t m = words[w];
                            while (m) {
                                uint32_t b = std::countr_zero(m);
                                out[start + uint32_t(w * 32) + b] = mv[6][ip_res++];
                                m &= m - 1;
                            }
                        }
                    } else {
                        ip_res += pc;
                    }
                }
            } else throw std::runtime_error("bad macro type");
        } else throw std::runtime_error("bad hotop opcode");
    }
    if (AB == A_FULL) {
        if (!pop.at_end() || !plit.at_end()) throw std::runtime_error("substream consumption mismatch");
        if (pos != out_len || ip_mt != mv[0].size() || ip_mll != mv[1].size() || ip_mml != mv[2].size()
            || ip_mdf != mv[3].size() || ip_mdv != mv[4].size() || ip_mask != mv[5].size() || ip_res != mv[6].size())
            throw std::runtime_error("substream consumption mismatch");
    }
    g_sink += out[pos > 0 ? pos - 1 : 0];
    return out;
}

// -------------------------------------- mode-10 (rANS token) instrumented ----
// Mirrors decode_tokens_rans with ablations: full / setup-only (5 eager stream
// materializations) / no-lit-pull / no-copy. Uses a preallocated output buffer
// for all variants; R_FULL is verified byte-identical to decode_tokens_rans.
enum Abl10 { R_FULL = 0, R_SETUP = 1, R_NO_LIT = 2, R_NO_COPY = 3 };
template <int AB>
static std::vector<uint8_t> prof_rans10(const uint8_t* p, size_t n, size_t out_len) {
    const uint8_t* e = p + n; std::array<std::vector<uint8_t>, 5> s;
    const size_t max_sub = 16 * out_len + 64;
    for (int i = 0; i < 5; ++i) {
        uint64_t zn = get_uvar(p, e);
        if (zn > uint64_t(e - p)) throw std::runtime_error("truncated substream");
        const uint8_t* q = p; const uint8_t* qe = p + zn;
        s[i] = decode_stream(q, qe, max_sub);
        if (q != qe) throw std::runtime_error("substream trailing bytes");
        p += zn;
    }
    if (p != e) throw std::runtime_error("payload trailing bytes");
    if (AB == R_SETUP) { g_sink += s[0].size() + s[1].size() + s[2].size() + s[3].size() + s[4].size(); return {}; }
    size_t ip_ll = 0, ip_ml = 0, ip_ds = 0, ip_lit = 0;
    std::vector<uint8_t> out(out_len);
    size_t pos = 0;
    for (uint8_t type : s[0]) {
        if (pos >= out_len) throw std::runtime_error("too many tokens");
        if (type == 0) {
            uint64_t len = read_varint_bytes(s[1], ip_ll) + 1;
            if (len > out_len - pos || len > s[4].size() - ip_lit) throw std::runtime_error("bad literal run");
            if (AB != R_NO_LIT) std::memcpy(out.data() + pos, s[4].data() + ip_lit, static_cast<size_t>(len));
            ip_lit += static_cast<size_t>(len); pos += static_cast<size_t>(len);
        } else if (type == 1) {
            uint64_t len = read_varint_bytes(s[2], ip_ml) + 4, dist = read_varint_bytes(s[3], ip_ds) + 1;
            if (dist > pos || len > out_len - pos) throw std::runtime_error("bad rANS match");
            if (AB != R_NO_COPY) {
                uint8_t* o = out.data();
                if (dist >= len) std::memcpy(o + pos, o + pos - dist, static_cast<size_t>(len));
                else for (uint64_t k = 0; k < len; ++k) o[pos + k] = o[pos + k - dist];
            }
            pos += static_cast<size_t>(len);
        } else throw std::runtime_error("bad token type");
    }
    if (AB == R_FULL) {
        if (pos != out_len || ip_ll != s[1].size() || ip_ml != s[2].size() || ip_ds != s[3].size() || ip_lit != s[4].size())
            throw std::runtime_error("substream consumption mismatch");
    }
    g_sink += out[pos > 0 ? pos - 1 : 0];
    return out;
}

// ------------------------------------------------- per-stream in-block cost ----
struct StreamCost { double pull_s = 0, pull_cyc = 0, mat_s = 0, mat_cyc = 0; uint64_t raw_bytes = 0, wire_bytes = 0; int instances = 0; };
static StreamCost time_stream_instances(const std::vector<uint8_t>& comp, int si, int warmup, int reps) {
    std::vector<BlockRec> blocks = parse_blocks(comp);
    struct Inst { std::vector<uint8_t> wire; size_t rawn; };
    std::vector<Inst> insts;
    uint64_t rawb = 0, wireb = 0;
    for (auto& br : blocks) {
        if (br.mode != 15) continue;
        Split15 s = split15(br.payload, br.plen, (size_t)br.blen);
        insts.push_back({s.wire[si], s.raw[si].size()});
        rawb += s.raw[si].size(); wireb += s.wire[si].size();
    }
    StreamCost sc; sc.raw_bytes = rawb; sc.wire_bytes = wireb; sc.instances = (int)insts.size();
    if (insts.empty() || rawb == 0) return sc;
    auto pull_all = [&]() {
        uint64_t t0 = rdtsc(); double w0 = now_s();
        uint64_t snk = 0;
        for (auto& in : insts) {
            StreamPull sp;
            const uint8_t* q = in.wire.data(); const uint8_t* qe = q + in.wire.size();
            sp.parse(q, qe, 1 << 26);
            std::vector<uint8_t> buf(in.rawn);
            if (in.rawn) sp.pull_bytes(buf.data(), buf.size());
            snk += buf.empty() ? 0 : buf[0];
        }
        double w1 = now_s(); uint64_t t1 = rdtsc();
        g_sink += snk;
        sc.pull_s = w1 - w0; sc.pull_cyc = double(t1 - t0);
        return w1 - w0;
    };
    auto mat_all = [&]() {
        uint64_t t0 = rdtsc(); double w0 = now_s();
        uint64_t snk = 0;
        for (auto& in : insts) {
            const uint8_t* q = in.wire.data(); const uint8_t* qe = q + in.wire.size();
            auto v = decode_stream(q, qe, 1 << 26);
            snk += v.empty() ? 0 : v[0];
        }
        double w1 = now_s(); uint64_t t1 = rdtsc();
        g_sink += snk;
        sc.mat_s = w1 - w0; sc.mat_cyc = double(t1 - t0);
        return w1 - w0;
    };
    for (int i = 0; i < warmup; ++i) { pull_all(); mat_all(); }
    std::vector<double> ps, ms, pc, mc;
    for (int r = 0; r < reps; ++r) {
        pull_all(); ps.push_back(sc.pull_s); pc.push_back(sc.pull_cyc);
        mat_all(); ms.push_back(sc.mat_s); mc.push_back(sc.mat_cyc);
    }
    sc.pull_s = stats_of(ps).med; sc.pull_cyc = stats_of(pc).med;
    sc.mat_s = stats_of(ms).med; sc.mat_cyc = stats_of(mc).med;
    return sc;
}

// ---------------------------------------------------------------- main ----
static void write_text(const std::string& path, const std::string& s) {
    std::ofstream f(path, std::ios::binary); f << s;
}

struct FileReport {
    std::string name;
    size_t src_bytes = 0, comp_bytes = 0;
    Stats baseline;
    double mbps = 0;
    int nblocks = 0; std::map<int, int> mode_census;
    std::array<uint64_t, 9> wire_bytes{}, raw_bytes{};
    std::array<std::map<int, int>, 9> codec_hist;
    HotCounters cnt;
    std::vector<std::pair<std::string, Stats>> abl;
    std::vector<std::pair<std::string, Stats>> abl10;
    int n10 = 0;
    std::vector<std::pair<std::string, Stats>> cf;
    std::vector<std::pair<std::string, StreamCost>> scost;
    double crc_s = 0, crcbw_s = 0, memcpy_s = 0;
    double crc_dispatch_s = 0;
    double crc_snap_s = 0, crc_other_s = 0;
    std::string crc_snap_name;
    bool crc_equiv = false;
    bool snap_is_slice8 = false, snap_is_bytewise = false;
};

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    LARGE_INTEGER f; QueryPerformanceFrequency(&f); g_qpc_freq = double(f.QuadPart);

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SYSTEM_INFO si; GetSystemInfo(&si);
    DWORD nproc = si.dwNumberOfProcessors;
    DWORD_PTR mask = (nproc >= 1) ? (DWORD_PTR(1) << (nproc - 1)) : 1;
    SetProcessAffinityMask(GetCurrentProcess(), mask);
    std::printf("[host] logical processors=%lu pinned=0x%llx priority=HIGH profile=prof_i9\n",
        (unsigned long)nproc, (unsigned long long)mask);

    std::vector<std::string> files;
    std::string outdir = ".", label = "run", expect = "";
    int warmup = 2, reps = 7;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--reps=", 0) == 0) reps = std::atoi(a.c_str() + 7);
        else if (a.rfind("--out=", 0) == 0) outdir = a.c_str() + 6;
        else if (a.rfind("--label=", 0) == 0) label = a.c_str() + 8;
        else if (a.rfind("--expect=", 0) == 0) expect = a.c_str() + 9;
        else files.push_back(a);
    }
    if (files.empty()) { std::fprintf(stderr, "usage: prof_i9 [--reps=N] [--out=DIR] [--label=L] [--expect=SRC] <containers...>\n"); return 2; }

    Options opt;
#ifndef PROF_HEAD_SNAPSHOT
    opt.decode_threads = 1;   // single-thread comparability (coordinator directive #3)
#endif
    opt.quiet = true;

    std::ostringstream report;
    report << "ANVIL decode time-share profile (decode-perf, I9 / t3 method)\n";
    report << "  LEG LABEL: " << label << "\n";
    report << "  reps=" << reps << " interleaved, median reported; CV=stdev/mean; decoder threads=1\n\n";

    std::vector<uint8_t> expect_src;
    if (!expect.empty()) expect_src = read_file(expect);
    g_rep_dir = outdir; g_rep_label = label;

    try {
        for (auto& path : files) {
            std::vector<uint8_t> comp = read_file(path);
            std::vector<uint8_t> back = call_decompress(comp, opt);
            if (!expect_src.empty() && back != expect_src) { std::fprintf(stderr, "ROUND-TRIP FAILED for %s\n", path.c_str()); return 1; }
            size_t src_bytes = back.size();
            std::printf("[file] %s container=%zu B decoded=%zu B\n", path.c_str(), comp.size(), src_bytes);

            FileReport rep; rep.name = path; rep.src_bytes = src_bytes; rep.comp_bytes = comp.size();

            // ---- [1] baseline + counterfactuals, interleaved ----
            std::vector<Variant> vs;
            vs.push_back({"baseline", [&]() { double w0 = now_s(); auto o = call_decompress(comp, opt); double w1 = now_s(); g_sink += o[o.size() / 2]; return w1 - w0; }});
            std::vector<std::vector<uint8_t>> cfbufs;
            std::vector<std::string> cfnames;
            {
                for (int i = 0; i < 9; ++i) {
                    auto b = rebuild_with_raw(comp, 1u << i);
                    cfbufs.push_back(std::move(b));
                    cfnames.push_back(std::string("raw:") + STREAM_NAMES[i]);
                }
                cfbufs.push_back(rebuild_with_raw(comp, (1u << 0) | (1u << 6)));
                cfnames.push_back("raw:opcodes+literals");
                cfbufs.push_back(rebuild_with_raw(comp, 0x1FF));
                cfnames.push_back("raw:ALL9");
            }
            for (size_t i = 0; i < cfbufs.size(); ++i) {
                std::vector<uint8_t>* b = &cfbufs[i];
                vs.push_back({cfnames[i], [b, &opt]() { double w0 = now_s(); auto o = call_decompress(*b, opt); double w1 = now_s(); g_sink += o[o.size() / 2]; return w1 - w0; }});
            }
            g_rep_tag = "stage_" + std::filesystem::path(path).filename().string();
            { std::ofstream trunc(outdir + "/" + label + "_reps_" + g_rep_tag + ".csv", std::ios::trunc); }
            auto res = bench_interleaved(vs, warmup, reps);
            rep.baseline = res[0];
            rep.mbps = double(src_bytes) / res[0].med / 1e6;
            for (size_t i = 0; i < cfnames.size(); ++i) rep.cf.push_back({cfnames[i], res[1 + i]});

            // ---- [2] census ----
            std::vector<BlockRec> blocks = parse_blocks(comp);
            rep.nblocks = (int)blocks.size();
            for (auto& br : blocks) {
                ++rep.mode_census[br.mode];
                if (br.mode != 15) continue;
                Split15 s = split15(br.payload, br.plen, (size_t)br.blen);
                for (int i = 0; i < 9; ++i) {
                    rep.wire_bytes[i] += s.wire[i].size();
                    rep.raw_bytes[i] += s.raw[i].size();
                    ++rep.codec_hist[i][s.codec[i]];
                }
            }

            // ---- [3] instrumented ablations (mode-15 blocks only) ----
            HotCounters vc;
            bool have15 = false;
            for (auto& br : blocks) {
                if (br.mode != 15) continue;
                have15 = true;
                auto mine = prof_fused<A_FULL>(br.payload, br.plen, (size_t)br.blen, vc);
                auto real = decode_tokens_hotop_fused(br.payload, br.plen, (size_t)br.blen);
                if (mine != real) { std::fprintf(stderr, "INSTRUMENTED DECODER MISMATCH on %s\n", path.c_str()); return 1; }
            }
            rep.cnt = vc;
            if (have15) {
                auto make_abl = [&](Abl ab) {
                    return [&, ab]() {
                        double w0 = now_s();
                        HotCounters c;
                        for (auto& br : blocks) {
                            if (br.mode != 15) continue;
                            switch (ab) {
                            case A_FULL: { auto o = prof_fused<A_FULL>(br.payload, br.plen, (size_t)br.blen, c); g_sink += o[o.size() / 2]; break; }
                            case A_NO_LIT: { auto o = prof_fused<A_NO_LIT>(br.payload, br.plen, (size_t)br.blen, c); g_sink += o.size(); break; }
                            case A_NO_COPY: { auto o = prof_fused<A_NO_COPY>(br.payload, br.plen, (size_t)br.blen, c); g_sink += o.size(); break; }
                            case A_NO_MACRO: { auto o = prof_fused<A_NO_MACRO>(br.payload, br.plen, (size_t)br.blen, c); g_sink += o.size(); break; }
                            case A_SETUP_ONLY: { prof_fused<A_SETUP_ONLY>(br.payload, br.plen, (size_t)br.blen, c); break; }
                            case A_OPCODE_ONLY: { prof_fused<A_OPCODE_ONLY>(br.payload, br.plen, (size_t)br.blen, c); break; }
                            }
                        }
                        double w1 = now_s();
                        return w1 - w0;
                    };
                };
                std::vector<Variant> av;
                std::vector<Abl> abls = {A_FULL, A_NO_LIT, A_NO_COPY, A_NO_MACRO, A_SETUP_ONLY, A_OPCODE_ONLY};
                for (Abl ab : abls) av.push_back({abl_name(ab), make_abl(ab)});
                g_rep_tag = "ablation_" + std::filesystem::path(path).filename().string();
                { std::ofstream trunc(outdir + "/" + label + "_reps_" + g_rep_tag + ".csv", std::ios::trunc); }
                auto ares = bench_interleaved(av, warmup, reps);
                for (size_t i = 0; i < av.size(); ++i) rep.abl.push_back({av[i].name, ares[i]});
            }

            // ---- [3b] mode-10 instrumented setup/token split ----
            for (auto& br : blocks) if (br.mode == 10) ++rep.n10;
            if (rep.n10 > 0) {
                for (auto& br : blocks) {
                    if (br.mode != 10) continue;
                    auto mine = prof_rans10<R_FULL>(br.payload, br.plen, (size_t)br.blen);
                    auto real = decode_tokens_rans(br.payload, br.plen, (size_t)br.blen);
                    if (mine != real) { std::fprintf(stderr, "INSTRUMENTED MODE-10 DECODER MISMATCH on %s\n", path.c_str()); return 1; }
                }
                auto make_abl10 = [&](int ab) {
                    return [&, ab]() {
                        double w0 = now_s();
                        for (auto& br : blocks) {
                            if (br.mode != 10) continue;
                            switch (ab) {
                            case R_FULL: { auto o = prof_rans10<R_FULL>(br.payload, br.plen, (size_t)br.blen); g_sink += o[o.size() / 2]; break; }
                            case R_SETUP: { prof_rans10<R_SETUP>(br.payload, br.plen, (size_t)br.blen); break; }
                            case R_NO_LIT: { auto o = prof_rans10<R_NO_LIT>(br.payload, br.plen, (size_t)br.blen); g_sink += o.size(); break; }
                            case R_NO_COPY: { auto o = prof_rans10<R_NO_COPY>(br.payload, br.plen, (size_t)br.blen); g_sink += o.size(); break; }
                            }
                        }
                        return now_s() - w0;
                    };
                };
                const char* n10[4] = {"full10", "setup-only10", "no-lit-pull10", "no-copy10"};
                std::vector<Variant> av; std::vector<int> abls = {R_FULL, R_SETUP, R_NO_LIT, R_NO_COPY};
                for (int ab : abls) av.push_back({n10[ab], make_abl10(ab)});
                g_rep_tag = "ablation10_" + std::filesystem::path(path).filename().string();
                { std::ofstream trunc(outdir + "/" + label + "_reps_" + g_rep_tag + ".csv", std::ios::trunc); }
                auto ares = bench_interleaved(av, warmup, reps);
                for (size_t i = 0; i < av.size(); ++i) rep.abl10.push_back({av[i].name, ares[i]});
            }

            // ---- [5] per-stream in-block cost ----
            for (int i = 0; i < 9; ++i) {
                StreamCost sc = time_stream_instances(comp, i, warmup, reps);
                rep.scost.push_back({STREAM_NAMES[i], sc});
            }

            // ---- [6] fixed costs ----
            {
                auto crc8run = [&]() { double w0 = now_s(); uint32_t c = crc32_slice8_local(back.data(), back.size()); double w1 = now_s(); g_sink += c; return w1 - w0; };
                auto crcbwrun = [&]() { double w0 = now_s(); uint32_t c = crc32_bytewise(back.data(), back.size()); double w1 = now_s(); g_sink += c; return w1 - w0; };
                auto crcsnaprun = [&]() { double w0 = now_s(); uint32_t c = crc32(back.data(), back.size()); double w1 = now_s(); g_sink += c; return w1 - w0; };  // snapshot's own dispatch (bytewise/slice8/pclmul)
                auto mcrun = [&]() {
                    double w0 = now_s();
                    std::vector<uint8_t> tmp(back.size());
                    std::memcpy(tmp.data(), back.data(), back.size());
                    double w1 = now_s();
                    g_sink += tmp[tmp.size() / 2];
                    return w1 - w0;
                };
                rep.crc_equiv = (crc32_slice8_local(back.data(), back.size()) == crc32_bytewise(back.data(), back.size()));
                rep.snap_is_slice8 = (crc32(back.data(), back.size()) == crc32_slice8_local(back.data(), back.size()));
                rep.snap_is_bytewise = (crc32(back.data(), back.size()) == crc32_bytewise(back.data(), back.size()));
                for (int i = 0; i < warmup; ++i) { crc8run(); crcbwrun(); crcsnaprun(); mcrun(); }
                std::vector<double> a, b, sp, m;
                for (int r = 0; r < reps; ++r) { a.push_back(crc8run()); b.push_back(crcbwrun()); sp.push_back(crcsnaprun()); m.push_back(mcrun()); }
                rep.crc_s = stats_of(a).med; rep.crcbw_s = stats_of(b).med; rep.crc_dispatch_s = stats_of(sp).med; rep.memcpy_s = stats_of(m).med;
                // The baseline's CRC implementation is the snapshot's dispatch function.
                rep.crc_snap_name = "snapshot_dispatch"; rep.crc_snap_s = rep.crc_dispatch_s; rep.crc_other_s = rep.crcbw_s;
            }

            // ---- emit ----
            std::string base = outdir + "/" + label + "_results_" + path.substr(path.find_last_of("/\\") + 1) + ".txt";
            std::filesystem::create_directories(outdir);
            std::ostringstream o;
            char line[320];
            o << "=== " << path << " ===\n";
            std::snprintf(line, sizeof line, "src=%zu B  comp=%zu B  ratio=%.4f  blocks=%d\n", rep.src_bytes, rep.comp_bytes, double(rep.comp_bytes) / double(rep.src_bytes), rep.nblocks);
            o << line;
            o << "mode census:";
            for (auto& [m, k] : rep.mode_census) { std::snprintf(line, sizeof line, " m%d=%d", m, k); o << line; }
            o << "\n";
            std::snprintf(line, sizeof line, "BASELINE full decode: %.1f MB/s  (med %.3f ms, CV %.1f%%, min %.3f ms)\n",
                rep.mbps, rep.baseline.med * 1e3, rep.baseline.cv * 100, rep.baseline.mn * 1e3);
            o << line;
            if (have15) {
                o << "\n-- token census (instrumented, verified == real decoder) --\n";
                std::snprintf(line, sizeof line, "tokens=%llu  hot_lit=%llu hot_match=%llu  mac_lit=%llu mac_m1=%llu mac_m2=%llu\n",
                    (unsigned long long)rep.cnt.tokens, (unsigned long long)rep.cnt.hot_lit, (unsigned long long)rep.cnt.hot_match,
                    (unsigned long long)rep.cnt.mac_lit, (unsigned long long)rep.cnt.mac_m1, (unsigned long long)rep.cnt.mac_m2);
                o << line;
                std::snprintf(line, sizeof line, "lit_bytes=%llu (%.1f%% of src)  copy_bytes=%llu (%.1f%%)  opcode_bytes=%llu\n",
                    (unsigned long long)rep.cnt.lit_bytes, 100.0 * rep.cnt.lit_bytes / rep.src_bytes,
                    (unsigned long long)rep.cnt.copy_bytes, 100.0 * rep.cnt.copy_bytes / rep.src_bytes,
                    (unsigned long long)rep.cnt.opcode_bytes);
                o << line;
                o << "\n-- stage ablation (mode-15 block loop only, seconds; delta vs full = stage cost) --\n";
                double full_s = 0; for (auto& [n, s] : rep.abl) if (n == "full") full_s = s.med;
                for (auto& [n, s] : rep.abl) {
                    std::snprintf(line, sizeof line, "  %-18s med=%.3f ms  CV=%.1f%%  delta_vs_full=%+.3f ms\n",
                        n.c_str(), s.med * 1e3, s.cv * 100, (s.med - full_s) * 1e3);
                    o << line;
                }
                o << "\n-- per-stream census + in-block decode cost --\n";
                o << "  stream          codec              wire(B)  raw(B)   pull(ms) cyc/B   mat(ms) cyc/B\n";
                for (int i = 0; i < 9; ++i) {
                    auto& sc = rep.scost[i].second;
                    std::string codecs;
                    for (auto& [c, k] : rep.codec_hist[i]) { codecs += codec_name(c); codecs += "x" + std::to_string(k) + " "; }
                    if (codecs.empty()) codecs = "-";
                    std::snprintf(line, sizeof line, "  %-14s %-18s %8llu %8llu  %7.3f %6.2f  %7.3f %6.2f\n",
                        STREAM_NAMES[i], codecs.c_str(),
                        (unsigned long long)rep.wire_bytes[i], (unsigned long long)rep.raw_bytes[i],
                        sc.pull_s * 1e3, sc.raw_bytes ? sc.pull_cyc / double(sc.raw_bytes) : 0.0,
                        sc.mat_s * 1e3, sc.raw_bytes ? sc.mat_cyc / double(sc.raw_bytes) : 0.0);
                    o << line;
                }
                o << "\n-- raw-stream counterfactuals (full decompress; delta vs baseline = projected gain) --\n";
                for (size_t i = 0; i < rep.cf.size(); ++i) {
                    auto& [n, s] = rep.cf[i];
                    double mb = double(rep.src_bytes) / s.med / 1e6;
                    std::snprintf(line, sizeof line, "  %-22s %7.1f MB/s (%+.1f%%)  med=%.3f ms CV=%.1f%%  size=%zu B (%+lld)\n",
                        n.c_str(), mb, (rep.mbps > 0 ? (mb / rep.mbps - 1.0) * 100.0 : 0.0),
                        s.med * 1e3, s.cv * 100, cfbufs[i].size(), (long long)cfbufs[i].size() - (long long)rep.comp_bytes);
                    o << line;
                }
            }
            if (rep.n10 > 0) {
                o << "\n-- MODE-10 stage ablation (rANS token streams; block loop only, seconds) --\n";
                double full10 = 0; for (auto& [n, s] : rep.abl10) if (n == "full10") full10 = s.med;
                for (auto& [n, s] : rep.abl10) {
                    std::snprintf(line, sizeof line, "  %-18s med=%.3f ms  CV=%.1f%%  delta_vs_full=%+.3f ms\n",
                        n.c_str(), s.med * 1e3, s.cv * 100, (s.med - full10) * 1e3);
                    o << line;
                }
            }
            if (!have15 && rep.n10 == 0) {
                o << "\n(no mode-15 / mode-10 blocks: stage ablation not applicable)\n";
            }
            o << "\nfixed costs (file-level):\n";
            std::snprintf(line, sizeof line, "  crc32_snapshot_dispatch=%.3f ms (%.3f ns/B, %.1f MB/s)   crc32_slice8_local=%.3f ms (%.3f ns/B)   crc32_bytewise(I8 base)=%.3f ms (%.3f ns/B, %.1f MB/s)   slice8==bytewise=%s\n",
                rep.crc_dispatch_s * 1e3, rep.crc_dispatch_s * 1e9 / rep.src_bytes, double(rep.src_bytes) / rep.crc_dispatch_s / 1e6,
                rep.crc_s * 1e3, rep.crc_s * 1e9 / rep.src_bytes,
                rep.crcbw_s * 1e3, rep.crcbw_s * 1e9 / rep.src_bytes, double(rep.src_bytes) / rep.crcbw_s / 1e6,
                rep.crc_equiv ? "YES" : "NO");
            o << line;
            std::snprintf(line, sizeof line, "  memcpy-out=%.3f ms (%.3f ns/B)\n", rep.memcpy_s * 1e3, rep.memcpy_s * 1e9 / rep.src_bytes);
            o << line;
            {
                double base_t = rep.baseline.med;
                double blocks = 0; for (auto& [n, s] : rep.abl) if (n == "full") blocks = s.med;
                double setup = 0; for (auto& [n, s] : rep.abl) if (n == "setup-only") setup = s.med;
                if (blocks == 0) { for (auto& [n, s] : rep.abl10) if (n == "full10") blocks = s.med; }
                if (setup == 0) { for (auto& [n, s] : rep.abl10) if (n == "setup-only10") setup = s.med; }
                const double crc_snap = rep.crc_snap_s, crc_other = rep.crc_other_s;
                const char* other_name = (rep.crc_snap_name == "bytewise") ? "slice8" : "bytewise";
                double resid = base_t - blocks - crc_snap - rep.memcpy_s;
                double other_t = base_t - crc_snap + crc_other;
                std::snprintf(line, sizeof line, "\nSHARES of end-to-end baseline (snapshot CRC=%s, measured baseline): crc_snapshot=%.2f%%  crc_%s_counterfactual=%.2f%%  memcpy=%.2f%%  block_loop=%.2f%%  residual_concat_alloc_hdr=%.2f%%\n",
                    rep.crc_snap_name.c_str(), 100.0 * crc_snap / base_t, other_name, 100.0 * crc_other / other_t,
                    100.0 * rep.memcpy_s / base_t, 100.0 * blocks / base_t, 100.0 * resid / base_t);
                o << line;
                if (have15) {
                    std::snprintf(line, sizeof line, "BLOCK-LOOP SPLIT: setup(eager mat + header)=%.2f%% of e2e (%.2f%% of block loop)  token_loop_beyond_setup=%.2f%% of e2e\n",
                        100.0 * setup / base_t, 100.0 * setup / (blocks > 0 ? blocks : 1),
                        100.0 * (blocks - setup) / base_t);
                    o << line;
                }
                double slice8_share = rep.crc_s / base_t, bw_share = rep.crcbw_s / (base_t - crc_snap + rep.crcbw_s);
                std::snprintf(line, sizeof line, "CRC-ONLY LIFT (full CRC-time removal): snapshot_dispatch=%.3fx (share %.4f)   bytewise_I8base=%.3fx (share %.4f)   slice8_local=%.3fx (share %.4f)\n",
                    1.0 / (1.0 - crc_snap / base_t), crc_snap / base_t,
                    1.0 / (1.0 - bw_share), bw_share,
                    1.0 / (1.0 - slice8_share), slice8_share);
                o << line;
                std::snprintf(line, sizeof line, "COUNTERFACTUAL decode speed with bytewise CRC = %.1f MB/s; with slice8 CRC = %.1f MB/s; measured snapshot (%s) = %.1f MB/s\n",
                    double(rep.src_bytes) / (base_t - crc_snap + rep.crcbw_s) / 1e6,
                    double(rep.src_bytes) / (base_t - crc_snap + rep.crc_s) / 1e6,
                    rep.crc_snap_name.c_str(), rep.mbps);
                o << line;
            }
            write_text(base, o.str());
            report << o.str() << "\n";
            std::printf("  -> %s\n", base.c_str());
        }
        std::string full = report.str();
        std::filesystem::create_directories(outdir);
        write_text(outdir + "/" + label + "_summary.txt", full);
        std::printf("[done] summary -> %s/%s_summary.txt\n", outdir.c_str(), label.c_str());
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[FATAL] exception: %s\n", ex.what());
        return 1;
    }
    return 0;
}
