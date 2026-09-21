// post_prof.cpp -- decode-perf I9 leg 3: BWT postcoder (IDs 1/2/3) stage profile.
// READ-ONLY w.r.t. src/anvil.cpp (lane snapshot). Replicates bwt_arith_decode with
// ablation variants and verifies P_FULL byte-identical to the real decoder.
//
// Decomposition targets (postcoder is now the binding BWT decode floor after the
// aux-unbwt result):
//   SETUP       = ArithmeticDecoder init + eager AdaptiveModel construction
//   TOKEN+UAR   = token symbol decode (arith + Fenwick) + run-uvar decode, no MTF/no writes
//   MTF         = sym-list search/shift per rank token
//   OUT         = output materialization (inserts / pushes)
// For postcoder 3 (raw stream) the postcoder time is the stream-suite decode.
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
#ifndef ANVIL_SNAPSHOT_NAME
#define ANVIL_SNAPSHOT_NAME anvil_snapshot.cpp
#endif
#define ANVIL_STR2(x) #x
#define ANVIL_STR(x) ANVIL_STR2(x)
#include ANVIL_STR(ANVIL_SNAPSHOT_NAME)

using namespace anvil;

#ifdef PROF_HEAD_SNAPSHOT
static std::vector<uint8_t> call_decompress(const std::vector<uint8_t>& c, const Options&) { return decompress(c); }
#else
static std::vector<uint8_t> call_decompress(const std::vector<uint8_t>& c, const Options& o) { return decompress(c, o); }
#endif

static double g_qpc_freq = 0.0;
static double now_s() { LARGE_INTEGER c; QueryPerformanceCounter(&c); return double(c.QuadPart) / g_qpc_freq; }
static inline uint64_t rdtsc() { return __rdtsc(); }
static volatile uint64_t g_sink = 0;

struct Stats { double med = 0, mean = 0, cv = 0, mn = 0; int n = 0; };
static Stats stats_of(std::vector<double> v) {
    Stats s; s.n = (int)v.size();
    if (v.empty()) return s;
    std::sort(v.begin(), v.end());
    s.med = v[v.size() / 2]; s.mn = v.front();
    double sum = 0; for (double x : v) sum += x;
    s.mean = sum / v.size();
    double var = 0; for (double x : v) var += (x - s.mean) * (x - s.mean);
    var /= v.size();
    s.cv = s.mean > 0 ? std::sqrt(var) / s.mean : 0;
    return s;
}

struct Variant { std::string name; std::function<double()> run; };
static std::string g_rep_dir, g_rep_label, g_rep_tag;
static std::vector<Stats> bench_interleaved(std::vector<Variant>& vs, int warmup, int reps) {
    for (auto& v : vs) for (int i = 0; i < warmup; ++i) {
        try { (void)v.run(); }
        catch (const std::exception& ex) { std::fprintf(stderr, "WARMUP VARIANT '%s' THREW: %s\n", v.name.c_str(), ex.what()); std::exit(1); }
    }
    std::vector<std::vector<double>> samples(vs.size());
    for (int r = 0; r < reps; ++r)
        for (size_t i = 0; i < vs.size(); ++i) {
            try { samples[i].push_back(vs[i].run()); }
            catch (const std::exception& ex) { std::fprintf(stderr, "VARIANT '%s' THREW (rep %d): %s\n", vs[i].name.c_str(), r, ex.what()); std::exit(1); }
        }
    if (!g_rep_dir.empty()) {
        std::ofstream f(g_rep_dir + "/" + g_rep_label + "_reps_" + g_rep_tag + ".csv", std::ios::app);
        f << "variant,rep,seconds\n";
        for (size_t i = 0; i < samples.size(); ++i)
            for (size_t r = 0; r < samples[i].size(); ++r)
                f << "\"" << vs[i].name << "\"," << r << "," << std::fixed << samples[i][r] << "\n";
    }
    std::vector<Stats> out;
    for (auto& s : samples) out.push_back(stats_of(s));
    return out;
}

struct BlockRec { uint64_t blen = 0; int mode = 0; size_t plen = 0; uint32_t crc = 0; const uint8_t* payload = nullptr; };
static std::vector<BlockRec> parse_blocks(const std::vector<uint8_t>& comp) {
    const uint8_t* p = comp.data(); const uint8_t* e = p + comp.size();
    if (comp.size() < 5 || std::memcmp(p, "ANV0", 4) != 0 || (p[4] != 1 && p[4] != 2)) throw std::runtime_error("not ANV0 rev1/rev2");
    p += 5;
    (void)get_uvar(p, e); uint64_t total = get_uvar(p, e);
    std::vector<BlockRec> blocks; size_t outsz = 0;
    while (outsz < total) {
        BlockRec br; br.blen = get_uvar(p, e); br.mode = *p++; br.plen = (size_t)get_uvar(p, e); br.crc = get_u32le(p, e); br.payload = p;
        blocks.push_back(br); p += br.plen; outsz += (size_t)br.blen;
    }
    return blocks;
}

struct Ctx { int transform = 0, backend = 0; size_t xlen = 0; int post = -1; int32_t primary = 0;
             uint64_t nbits = 0; const uint8_t* post_p = nullptr; const uint8_t* post_e = nullptr; };

// parse ratio header + postcoder header (does NOT run the postcoder)
static Ctx parse_ctx(const std::vector<uint8_t>& comp) {
    auto blocks = parse_blocks(comp);
    if (blocks.size() != 1 || blocks[0].mode != 17) throw std::runtime_error("post_prof needs exactly one mode-17 block");
    const BlockRec& br = blocks[0];
    const uint8_t* p = br.payload; const uint8_t* e = br.payload + br.plen;
    Ctx c; c.transform = *p++; c.backend = *p++;
    if (c.backend != kRatioBackendBwt) throw std::runtime_error("needs ratio backend 2");
    c.xlen = (size_t)get_uvar(p, e);
    c.post = *p++;
    uint64_t pv = get_uvar(p, e); if (pv >= c.xlen) throw std::runtime_error("bad primary");
    c.primary = (int32_t)pv;
    if (c.post == kBwtPostArithO0 || c.post == kBwtPostArithO1) { c.nbits = get_uvar(p, e); }
    c.post_p = p; c.post_e = e;
    return c;
}

// ---- instrumented copy of bwt_arith_decode ---------------------------------
enum PAb { P_FULL = 0, P_SETUP = 1, P_NO_EMIT = 2, P_NO_OUT = 3 };
struct PostCounters { uint64_t tokens = 0, runs = 0, run_bytes = 0, out_bytes = 0, model_builds = 0; uint64_t rank_hist[16] = {0}; };
static bool g_dbg = false;
static bool g_nofast = (std::getenv("POSTPROF_NOFAST") != nullptr);
static void dbg_sym(const char* who, uint64_t i, uint8_t t, long long rv) {
    if (g_dbg && i < 60) std::fprintf(stderr, "[%s] sym %llu: t=%u rv=%lld\n", who, (unsigned long long)i, (unsigned)t, rv);
}

template <int AB>
static std::vector<uint8_t> prof_arith_decode(const uint8_t* p, size_t n, size_t expected, bool order1, uint64_t bit_count, PostCounters* pc) {
    if (bit_count == 0 || bit_count > uint64_t(n) * 8) throw std::runtime_error("bad BWT arithmetic bit count");
    uint64_t need = (bit_count + 7) / 8; if (need != n) throw std::runtime_error("BWT arithmetic byte count mismatch");
    if ((bit_count & 7) && n) {
        uint32_t pad = 8 - static_cast<uint32_t>(bit_count & 7); uint8_t mask = static_cast<uint8_t>((1u << pad) - 1u);
        if (p[n - 1] & mask) throw std::runtime_error("nonzero BWT arithmetic padding");
    }
    if (AB == P_SETUP) { // setup-only: construct the eager models + decoder (no token loop)
        ArithmeticDecoder ad(p, n); AdaptiveModel tok0(256), runm(256);
        g_sink += ad.consumed_bytes();
        return {};
    }
    ArithmeticDecoder ad(p, n); AdaptiveModel tok0(256), runm(256);
    std::array<std::unique_ptr<AdaptiveModel>, 257> tok1;
    auto model1 = [&](uint32_t ctx)->AdaptiveModel& {
        if (!tok1[ctx]) { tok1[ctx] = std::make_unique<AdaptiveModel>(256); if (pc) ++pc->model_builds; }
        return *tok1[ctx];
    };
    std::array<uint8_t, 256> sym{}; for (uint32_t i = 0; i < 256; ++i) sym[i] = static_cast<uint8_t>(i);
    std::vector<uint8_t> out; if (AB == P_FULL || AB == P_NO_OUT) out.reserve(expected);
    size_t produced = 0; uint32_t prev = 256;
    while (produced < expected) {
        uint8_t t = static_cast<uint8_t>(order1 ? model1(prev).decode(ad) : tok0.decode(ad)); prev = t;
        if (pc) { ++pc->tokens; if (t) ++pc->rank_hist[(t < 16) ? t : 15]; }
        if (t == 0) {
            uint64_t rv = decode_uvar(ad, runm);
            if (rv >= expected - produced) throw std::runtime_error("BWT arithmetic zero run exceeds output");
            size_t len = static_cast<size_t>(rv) + 1;
            if (pc) { ++pc->runs; pc->run_bytes += len; }
            if (pc) dbg_sym("ref", pc->tokens - 1, t, (long long)rv);
            if (AB == P_FULL) out.insert(out.end(), len, sym[0]);
            else if (AB == P_NO_OUT) { g_sink += sym[0]; }
            produced += len;
        } else {
            uint8_t b = sym[t];
            if (pc) dbg_sym("ref", pc->tokens - 1, t, -1);
            if (AB == P_FULL) out.push_back(b);
            else if (AB == P_NO_OUT) { g_sink += b; }
            if (AB != P_NO_EMIT) {
                if (t) { for (uint32_t j = t; j > 0; --j) sym[j] = sym[j - 1]; sym[0] = b; }
            }
            produced += 1;
        }
    }
    if (pc) pc->out_bytes = produced;
    if (AB == P_FULL) { if (produced != expected) throw std::runtime_error("postcoder size mismatch"); }
    if (AB == P_FULL) return out;
    return {};
}

// ---- prototype: buffered-bit decoder (same bit stream, branch cheaper) ----
// Semantics identical to ArithmeticDecoder: MSB-first bit order, zero padding past
// the payload end. Windowing removes the per-bit byte-boundary branch only.
class ArithmeticDecoderFast {
    const uint8_t* p_; size_t n_; size_t pos_ = 0;
    uint64_t win_ = 0; int winbits_ = 0;
    uint32_t low_ = 0, high_ = 0xFFFFFFFFu, code_ = 0;
    uint32_t next() {
        if (winbits_ == 0) {
            uint32_t v = 0;
            for (int i = 0; i < 4; ++i) { v <<= 8; if (pos_ < n_) v |= p_[pos_++]; }
            win_ = v; winbits_ = 32;
        }
        uint32_t b = static_cast<uint32_t>(win_ >> 31); win_ = (win_ << 1) & 0xFFFFFFFFull; --winbits_; return b;
    }
public:
    ArithmeticDecoderFast(const uint8_t* p, size_t n) : p_(p), n_(n) { for (int i = 0; i < 32; ++i) code_ = (code_ << 1) | next(); }
    uint32_t scaled(uint32_t total) const {
        uint64_t range = static_cast<uint64_t>(high_) - low_ + 1;
        return static_cast<uint32_t>(((static_cast<uint64_t>(code_ - low_) + 1) * total - 1) / range);
    }
    uint32_t next_bit() { return next(); }
    void consume(uint32_t cum_lo, uint32_t cum_hi, uint32_t total) {
        uint64_t range = static_cast<uint64_t>(high_) - low_ + 1;
        high_ = low_ + static_cast<uint32_t>((range * cum_hi) / total - 1);
        low_ = low_ + static_cast<uint32_t>((range * cum_lo) / total);
        for (;;) {
            if (high_ < kHalf) { }
            else if (low_ >= kHalf) { code_ -= kHalf; low_ -= kHalf; high_ -= kHalf; }
            else if (low_ >= kFirstQtr && high_ < kThirdQtr) { code_ -= kFirstQtr; low_ -= kFirstQtr; high_ -= kFirstQtr; }
            else break;
            low_ <<= 1; high_ = (high_ << 1) | 1u; code_ = (code_ << 1) | next();
        }
    }
};

// ---- prototype: templated model copy so the buffered decoder can be measured ----
template <class Dec>
class TModel {
    uint32_t alphabet_;
    std::vector<uint16_t> freq_;
    std::vector<uint32_t> tree_;
    uint32_t total_ = 0;
    void add_tree(uint32_t idx, uint32_t delta) { for (uint32_t i = idx + 1; i <= alphabet_; i += i & -i) tree_[i] += delta; }
    void rebuild() { std::fill(tree_.begin(), tree_.end(), 0); total_ = 0; for (uint32_t i = 0; i < alphabet_; ++i) { total_ += freq_[i]; add_tree(i, freq_[i]); } }
    void update(uint32_t sym) {
        if (total_ >= kModelRescale) { for (auto& f : freq_) f = static_cast<uint16_t>(std::max<uint16_t>(1, (f + 1) >> 1)); rebuild(); }
        ++freq_[sym]; ++total_; add_tree(sym, 1);
    }
public:
    explicit TModel(uint32_t alphabet = 256) : alphabet_(alphabet), freq_(alphabet, 1), tree_(alphabet + 1, 0) { rebuild(); }
    uint32_t decode(Dec& ad) {
        uint32_t target = ad.scaled(total_);
        uint32_t idx = 0, sum = 0;
        uint32_t bit = 1u << (31 - std::countl_zero(alphabet_));
        for (; bit; bit >>= 1) { uint32_t next = idx + bit; if (next <= alphabet_ && sum + tree_[next] <= target) { idx = next; sum += tree_[next]; } }
        if (idx >= alphabet_) throw std::runtime_error("arithmetic symbol out of range");
        uint32_t sym = idx; uint32_t lo = sum, hi = lo + freq_[sym];
        ad.consume(lo, hi, total_); update(sym); return sym;
    }
};

template <class Dec>
static uint64_t decode_uvar_t(Dec& ad, TModel<Dec>& m) {
    uint64_t x = 0; int shift = 0;
    for (int i = 0; i < 10; ++i) {
        uint8_t b = static_cast<uint8_t>(m.decode(ad));
        x |= static_cast<uint64_t>(b & 0x7Fu) << shift;
        if (!(b & 0x80u)) return x;
        shift += 7;
    }
    throw std::runtime_error("varint overflow");
}

template <int AB>
static std::vector<uint8_t> prof_arith_decode_fast(const uint8_t* p, size_t n, size_t expected, bool order1, uint64_t bit_count, PostCounters* pc) {
    if (bit_count == 0 || bit_count > uint64_t(n) * 8) throw std::runtime_error("bad bit count");
    uint64_t need = (bit_count + 7) / 8; if (need != n) throw std::runtime_error("byte count mismatch");
    ArithmeticDecoderFast ad(p, n);
    TModel<ArithmeticDecoderFast> tok0(256), runm(256);
    std::array<std::unique_ptr<TModel<ArithmeticDecoderFast>>, 257> tok1;
    auto model1 = [&](uint32_t ctx)->TModel<ArithmeticDecoderFast>& { if (!tok1[ctx]) tok1[ctx] = std::make_unique<TModel<ArithmeticDecoderFast>>(256); return *tok1[ctx]; };
    std::array<uint8_t, 256> sym{}; for (uint32_t i = 0; i < 256; ++i) sym[i] = static_cast<uint8_t>(i);
    std::vector<uint8_t> out; out.reserve(expected);
    size_t produced = 0; uint32_t prev = 256;
    while (produced < expected) {
        uint8_t t = static_cast<uint8_t>(order1 ? model1(prev).decode(ad) : tok0.decode(ad)); prev = t;
        if (pc) { ++pc->tokens; if (t) ++pc->rank_hist[(t < 16) ? t : 15]; }
        if (t == 0) {
            uint64_t rv = decode_uvar_t(ad, runm);
            if (rv >= expected - produced) throw std::runtime_error("zero run exceeds output");
            size_t len = static_cast<size_t>(rv) + 1;
            if (pc) { ++pc->runs; pc->run_bytes += len; }
            if (pc) dbg_sym("fast", pc->tokens - 1, t, (long long)rv);
            out.insert(out.end(), len, sym[0]); produced += len;
        } else {
            uint8_t b = sym[t]; out.push_back(b);
            if (pc) dbg_sym("fast", pc->tokens - 1, t, -1);
            if (t) { for (uint32_t j = t; j > 0; --j) sym[j] = sym[j - 1]; sym[0] = b; }
            produced += 1;
        }
    }
    if (pc) pc->out_bytes = produced;
    return out;
}

static void write_text(const std::string& path, const std::string& s) { std::ofstream f(path, std::ios::binary); f << s; }

struct Rep {
    std::string name; size_t src_bytes = 0, comp_bytes = 0;
    Stats baseline; double mbps = 0;
    int post = -1; size_t xlen = 0; bool arith = false;
    PostCounters cnt;
    Stats pc_full, pc_setup, pc_noemit, pc_noout;
    size_t id3_codec = 0, id3_wire = 0, id3_raw = 0;
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
    std::printf("[host] logical processors=%lu pinned=0x%llx priority=HIGH profile=post_prof\n", (unsigned long)nproc, (unsigned long long)mask);

    std::vector<std::string> files; std::string outdir = ".", label = "post", bittest = "";
    int warmup = 2, reps = 7;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--reps=", 0) == 0) reps = std::atoi(a.c_str() + 7);
        else if (a.rfind("--out=", 0) == 0) outdir = a.c_str() + 6;
        else if (a.rfind("--label=", 0) == 0) label = a.c_str() + 8;
        else if (a.rfind("--dbg", 0) == 0) g_dbg = true;
        else if (a.rfind("--bittest=", 0) == 0) bittest = a.c_str() + 10;
        else files.push_back(a);
    }
    if (files.empty() && bittest.empty()) { std::fprintf(stderr, "usage: post_prof [--reps=N] [--out=DIR] [--label=L] <containers...>\n"); return 2; }
    g_rep_dir = outdir; g_rep_label = label;

    std::ostringstream report;
    report << "ANVIL BWT postcoder stage profile (decode-perf, I9 leg 3)\n";
    report << "  LABEL: " << label << "\n";
    report << "  reps=" << reps << " interleaved, median; CV=stdev/mean; threads=1\n\n";

    Options opt; opt.decode_threads = 1; opt.quiet = true;

    if (!bittest.empty()) {
        std::vector<uint8_t> comp = read_file(bittest);
        Ctx ctx = parse_ctx(comp);
        const uint8_t* p = ctx.post_p; size_t n = (size_t)(ctx.post_e - ctx.post_p);
        BitReader br{p, n};
        for (int i = 0; i < 32; ++i) (void)br.bit();
        ArithmeticDecoderFast fd(p, n);
        size_t bad = SIZE_MAX;
        for (int i = 0; i < 4096; ++i) {
            uint32_t x = br.bit(), y = fd.next_bit();
            if (i < 16) std::fprintf(stderr, "[bit] %d ref=%u fast=%u\n", i + 32, x, y);
            if (x != y) { bad = (size_t)i; break; }
        }
        std::string where = (bad == SIZE_MAX) ? std::string("none(4096)") : std::to_string(bad + 32);
        std::printf("[bittest] payload_bytes=%zu first_bit_mismatch_at=%s\n", n, where.c_str());
        return bad == SIZE_MAX ? 0 : 1;
    }

    try {
        for (auto& path : files) {
            std::vector<uint8_t> comp = read_file(path);
            std::vector<uint8_t> real = call_decompress(comp, opt);
            Ctx ctx = parse_ctx(comp);
            Rep rep; rep.name = path; rep.src_bytes = real.size(); rep.comp_bytes = comp.size();
            rep.post = ctx.post; rep.xlen = ctx.xlen;
            std::printf("[file] %s comp=%zu src=%zu post=%d xlen=%zu\n", path.c_str(), comp.size(), real.size(), ctx.post, ctx.xlen);

            // Hoisted so the variant lambdas (invoked later) never capture
            // block-local state by reference. (ASan: stack-use-after-scope.)
            bool o1 = false;
            PostCounters dummy;
            const uint8_t* raw_p = nullptr;
            const uint8_t* raw_e = nullptr;
            std::vector<Variant> vs;
            vs.push_back({"baseline_full_decompress", [&]() { double w0 = now_s(); auto o = call_decompress(comp, opt); double w1 = now_s(); g_sink += o[o.size() / 2]; return w1 - w0; }});

            if (ctx.post == kBwtPostArithO0 || ctx.post == kBwtPostArithO1) {
                rep.arith = true;
                o1 = (ctx.post == kBwtPostArithO1);
                // verify P_FULL == real postcoder bwt bytes
                {
                    PostCounters c;
                    std::vector<uint8_t> mine, truth;
                    { PostCounters cd; try { prof_arith_decode<P_FULL>(ctx.post_p, (size_t)(ctx.post_e - ctx.post_p), ctx.xlen, o1, ctx.nbits, &cd); } catch (const std::exception& ex) { std::fprintf(stderr, "VERIFY-WARMUP REPLICA THREW: %s\n", ex.what()); return 1; } }  // warmup
                    try { mine = prof_arith_decode<P_FULL>(ctx.post_p, (size_t)(ctx.post_e - ctx.post_p), ctx.xlen, o1, ctx.nbits, &c); }
                    catch (const std::exception& ex) { std::fprintf(stderr, "REPLICA THREW: %s\n", ex.what()); return 1; }
                    try { truth = bwt_arith_decode(ctx.post_p, (size_t)(ctx.post_e - ctx.post_p), ctx.xlen, o1, ctx.nbits); }
                    catch (const std::exception& ex) { std::fprintf(stderr, "SNAPSHOT bwt_arith_decode THREW: %s\n", ex.what()); return 1; }
                    if (mine != truth) { std::fprintf(stderr, "POSTCODER REPLICA MISMATCH on %s\n", path.c_str()); return 1; }
                    if (!g_nofast) {
                    auto fast = [&]() {
                        PostCounters c2;
                        try { return prof_arith_decode_fast<P_FULL>(ctx.post_p, (size_t)(ctx.post_e - ctx.post_p), ctx.xlen, o1, ctx.nbits, &c2); }
                        catch (const std::exception& ex) {
                            std::fprintf(stderr, "FAST THREW after tokens=%llu runs=%llu: %s\n", (unsigned long long)c2.tokens, (unsigned long long)c2.runs, ex.what());
                            std::exit(1);
                        }
                    }();
                    if (fast != truth) { std::fprintf(stderr, "FAST-RENORM PROTOTYPE MISMATCH on %s\n", path.c_str()); return 1; }
                    }
                    rep.cnt = c;
                }
                vs.push_back({"post_full", [&]() { PostCounters c; double w0 = now_s(); auto o = prof_arith_decode<P_FULL>(ctx.post_p, (size_t)(ctx.post_e - ctx.post_p), ctx.xlen, o1, ctx.nbits, &c); double w1 = now_s(); g_sink += o[o.size() / 2]; return w1 - w0; }});
                if (!g_nofast) vs.push_back({"post_fast_renorm", [&]() { double w0 = now_s(); auto o = prof_arith_decode_fast<P_FULL>(ctx.post_p, (size_t)(ctx.post_e - ctx.post_p), ctx.xlen, o1, ctx.nbits, &dummy); double w1 = now_s(); g_sink += o[o.size() / 2]; return w1 - w0; }});
                vs.push_back({"model_build_x257", [&]() { double w0 = now_s(); std::array<std::unique_ptr<AdaptiveModel>, 257> m; for (int i = 0; i < 257; ++i) m[i] = std::make_unique<AdaptiveModel>(256); double w1 = now_s(); g_sink += (uint64_t)(uintptr_t)m[256].get(); return w1 - w0; }});
                vs.push_back({"post_setup_only", [&]() { double w0 = now_s(); prof_arith_decode<P_SETUP>(ctx.post_p, (size_t)(ctx.post_e - ctx.post_p), ctx.xlen, o1, ctx.nbits, nullptr); double w1 = now_s(); return w1 - w0; }});
                vs.push_back({"post_no_emit(token+uvar only)", [&]() { double w0 = now_s(); prof_arith_decode<P_NO_EMIT>(ctx.post_p, (size_t)(ctx.post_e - ctx.post_p), ctx.xlen, o1, ctx.nbits, &dummy); double w1 = now_s(); return w1 - w0; }});
                vs.push_back({"post_no_out(mtf, no writes)", [&]() { double w0 = now_s(); prof_arith_decode<P_NO_OUT>(ctx.post_p, (size_t)(ctx.post_e - ctx.post_p), ctx.xlen, o1, ctx.nbits, &dummy); double w1 = now_s(); return w1 - w0; }});
            } else if (ctx.post == kBwtPostRawStream) {
                // raw-stream postcoder: byte + uvar len + stream-suite payload
                const uint8_t* p = ctx.post_p;
                const uint8_t* e = ctx.post_e;
                rep.id3_codec = (p < e) ? p[0] : 0;
                rep.id3_wire = (size_t)(e - p);
                const uint8_t* q = p;
                auto raw = decode_stream(q, e, ctx.xlen);
                if (q != e || raw.size() != ctx.xlen) { std::fprintf(stderr, "raw postcoder mismatch\n"); return 1; }
                rep.id3_raw = raw.size();
                raw_p = p; raw_e = e;
                vs.push_back({"post_full(raw stream)", [&]() { double w0 = now_s(); const uint8_t* qq = raw_p; auto o = decode_stream(qq, raw_e, ctx.xlen); double w1 = now_s(); g_sink += o[o.size() / 2]; return w1 - w0; }});
            } else {
                std::fprintf(stderr, "unsupported postcoder %d\n", ctx.post); return 1;
            }

            g_rep_tag = "post_" + std::filesystem::path(path).filename().string();
            { std::ofstream trunc(outdir + "/" + label + "_reps_" + g_rep_tag + ".csv", std::ios::trunc); }
            auto res = bench_interleaved(vs, warmup, reps);
            rep.baseline = res[0]; rep.mbps = double(rep.src_bytes) / res[0].med / 1e6;
            if (rep.arith) {
                // vs layout when the fast prototype is enabled:
                // [0]=baseline [1]=post_full [2]=post_fast_renorm [3]=model_build_x257
                // [4]=post_setup_only [5]=post_no_emit [6]=post_no_out
                rep.pc_full = res[1]; rep.pc_setup = res[4]; rep.pc_noemit = res[5]; rep.pc_noout = res[6];
            } else {
                rep.pc_full = res[1];
            }

            std::ostringstream o; char line[360];
            o << "=== " << path << " ===\n";
            std::snprintf(line, sizeof line, "src=%zu comp=%zu ratio=%.4f post=%d xlen=%zu%s\n",
                rep.src_bytes, rep.comp_bytes, double(rep.comp_bytes) / double(rep.src_bytes), rep.post, rep.xlen,
                rep.arith ? (rep.post == kBwtPostArithO1 ? " (arith O1)" : " (arith O0)") : " (raw stream)");
            o << line;
            std::snprintf(line, sizeof line, "BASELINE full decode: %.2f MB/s (med %.2f ms CV %.1f%% min %.2f ms)\n",
                rep.mbps, rep.baseline.med * 1e3, rep.baseline.cv * 100, rep.baseline.mn * 1e3);
            o << line;
            double base = rep.baseline.med;
            if (rep.arith) {
                double setup = rep.pc_setup.med, noemit = rep.pc_noemit.med, noout = rep.pc_noout.med, full = rep.pc_full.med;
                std::snprintf(line, sizeof line, "POSTCODER (arith) med=%.2f ms = %.1f%% of e2e (CV %.1f%%)\n", full * 1e3, 100.0 * full / base, rep.pc_full.cv * 100);
                o << line;
                std::snprintf(line, sizeof line, "  setup (ad init + eager models): %.2f ms = %.1f%% of e2e (%.1f%% of post)\n", setup * 1e3, 100.0 * setup / base, 100.0 * setup / full);
                o << line;
                std::snprintf(line, sizeof line, "  token+uvar decode (no MTF/no writes): %.2f ms = %.1f%% of e2e (%.1f%% of post)\n", noemit * 1e3, 100.0 * noemit / base, 100.0 * noemit / full);
                o << line;
                std::snprintf(line, sizeof line, "  MTF list update (no writes): %.2f ms = %.1f%% of e2e (%.1f%% of post)\n", (noout - noemit) * 1e3, 100.0 * (noout - noemit) / base, 100.0 * (noout - noemit) / full);
                o << line;
                std::snprintf(line, sizeof line, "  output materialization (writes): %.2f ms = %.1f%% of e2e (%.1f%% of post)\n", (full - noout) * 1e3, 100.0 * (full - noout) / base, 100.0 * (full - noout) / full);
                o << line;
                std::snprintf(line, sizeof line, "  counters: tokens=%llu runs=%llu run_bytes=%llu out_bytes=%llu O1_model_builds=%llu\n",
                    (unsigned long long)rep.cnt.tokens, (unsigned long long)rep.cnt.runs, (unsigned long long)rep.cnt.run_bytes,
                    (unsigned long long)rep.cnt.out_bytes, (unsigned long long)rep.cnt.model_builds);
                o << line;
                std::snprintf(line, sizeof line, "  PROTOTYPE fast_renorm: %.2f ms vs full %.2f ms -> %.3fx (candidate C=%.3fx if this share is removed)\n",
                    res[2].med * 1e3, full * 1e3, full / res[2].med, 1.0 / (1.0 - (full - res[2].med) / base));
                o << line;
                std::snprintf(line, sizeof line, "  model_build_x257: %.3f ms per full set (%.4f ms/model) -> lazy-build est share = %llu * per_model / post_full = %.1f%%\n",
                    res[3].med * 1e3, res[3].med * 1e3 / 257.0, (unsigned long long)rep.cnt.model_builds,
                    100.0 * (rep.cnt.model_builds * (res[3].med / 257.0)) / full);
                o << line;
                std::snprintf(line, sizeof line, "  rank hist (1..15+): %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
                    (unsigned long long)rep.cnt.rank_hist[1], (unsigned long long)rep.cnt.rank_hist[2], (unsigned long long)rep.cnt.rank_hist[3], (unsigned long long)rep.cnt.rank_hist[4],
                    (unsigned long long)rep.cnt.rank_hist[5], (unsigned long long)rep.cnt.rank_hist[6], (unsigned long long)rep.cnt.rank_hist[7], (unsigned long long)rep.cnt.rank_hist[8],
                    (unsigned long long)rep.cnt.rank_hist[9], (unsigned long long)rep.cnt.rank_hist[10], (unsigned long long)rep.cnt.rank_hist[11], (unsigned long long)rep.cnt.rank_hist[12],
                    (unsigned long long)rep.cnt.rank_hist[13], (unsigned long long)rep.cnt.rank_hist[14], (unsigned long long)rep.cnt.rank_hist[15], (unsigned long long)rep.cnt.rank_hist[0]);
                o << line;
            } else {
                std::snprintf(line, sizeof line, "POSTCODER (raw stream) med=%.2f ms = %.1f%% of e2e (CV %.1f%%)  codec=%zu wire=%zu raw=%zu\n",
                    rep.pc_full.med * 1e3, 100.0 * rep.pc_full.med / base, rep.pc_full.cv * 100, rep.id3_codec, rep.id3_wire, rep.id3_raw);
                o << line;
            }
            write_text(outdir + "/" + label + "_results_" + std::filesystem::path(path).filename().string() + ".txt", o.str());
            report << o.str() << "\n";
            std::printf("  -> %s\n", (outdir + "/" + label + "_results_" + std::filesystem::path(path).filename().string() + ".txt").c_str());
        }
        std::ofstream rf(outdir + "/" + label + "_summary.txt", std::ios::binary); rf << report.str();
        std::printf("[done] summary -> %s/%s_summary.txt\n", outdir.c_str(), label.c_str());
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[FATAL] exception: %s\n", ex.what());
        return 1;
    }
    return 0;
}
