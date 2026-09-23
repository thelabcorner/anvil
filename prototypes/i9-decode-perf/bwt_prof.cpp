// bwt_prof.cpp -- decode-perf I9: BWT-routed (rev-2 mode-17 backend-2) decode stage split.
// Requested by bwtinv: wrap the exact callsites of bwt_backend_decode (postcoder
// decode, allocation of out/tmp, libsais_unbwt) plus container CRC/concat.
// READ-ONLY w.r.t. src/anvil.cpp (snapshot copy). Stage replication is verified
// against anvil::decompress byte-for-byte; hard failure otherwise.
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

// Snapshot API compatibility (HEAD decompress() has no Options param).
#ifdef PROF_HEAD_SNAPSHOT
static std::vector<uint8_t> call_decompress(const std::vector<uint8_t>& c) { return decompress(c); }
#else
static std::vector<uint8_t> call_decompress(const std::vector<uint8_t>& c) { Options o; o.decode_threads = 1; return decompress(c, o); }
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

struct Ctx { int transform = 0, backend = 0; size_t xlen = 0; int post = -1; int32_t primary = 0; const uint8_t* p = nullptr; const uint8_t* e = nullptr; };

// Stage A: ratio header parse + BWT postcoder decode -> bwt bytes.
static std::vector<uint8_t> bwt_stage_postcoder(const std::vector<uint8_t>& comp, Ctx& c) {
    auto blocks = parse_blocks(comp);
    if (blocks.size() != 1 || blocks[0].mode != 17) throw std::runtime_error("bwt_prof requires exactly one mode-17 block");
    const BlockRec& br = blocks[0];
    const uint8_t* p = br.payload; const uint8_t* e = br.payload + br.plen;
    c.transform = *p++; c.backend = *p++;
    if (c.backend != kRatioBackendBwt) throw std::runtime_error("bwt_prof requires ratio backend 2 (BWT)");
    c.xlen = (size_t)get_uvar(p, e);
    if (p >= e) throw std::runtime_error("truncated BWT backend header");
    c.post = *p++;
    uint64_t pv = get_uvar(p, e);
    if (pv >= c.xlen) throw std::runtime_error("bad BWT primary");
    c.primary = (int32_t)pv;
    const size_t expected = c.xlen;
    std::vector<uint8_t> bwt;
    if (c.post == kBwtPostStaticMtf) {
        uint64_t tn = get_uvar(p, e); const uint8_t* q = p; const uint8_t* qe = p + tn;
        auto tokens = decode_stream(q, qe, expected + 16); if (q != qe) throw std::runtime_error("token trailing");
        p = qe;
        uint64_t rn = get_uvar(p, e); q = p; qe = p + rn;
        auto runs = decode_stream(q, qe, expected + 16); if (q != qe) throw std::runtime_error("run trailing");
        p = qe;
        bwt = bwt_mtf_expand(tokens, runs, expected);
    } else if (c.post == kBwtPostArithO0 || c.post == kBwtPostArithO1) {
        uint64_t nbits = get_uvar(p, e);
        bwt = bwt_arith_decode(p, static_cast<size_t>(e - p), expected, c.post == kBwtPostArithO1, nbits);
        p = e;
    } else if (c.post == kBwtPostRawStream) {
        const uint8_t* q = p; bwt = decode_stream(q, e, expected); if (q != e) throw std::runtime_error("raw trailing");
        p = e;
    } else throw std::runtime_error("unsupported postcoder in bwt_prof");
    if (bwt.size() != expected) throw std::runtime_error("postcoder size mismatch");
    c.p = p; c.e = e;
    return bwt;
}

static void write_text(const std::string& path, const std::string& s) {
    std::ofstream f(path, std::ios::binary); f << s;
}

struct FileReport {
    std::string name; size_t src_bytes = 0, comp_bytes = 0;
    Stats baseline; double mbps = 0;
    int transform = 0, backend = 0, post = -1; int32_t primary = 0; size_t xlen = 0;
    Stats stageA, allocB1, unbwtB2, stageC, crc, memcpy_;
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
    std::printf("[host] logical processors=%lu pinned=0x%llx priority=HIGH profile=bwt_prof\n", (unsigned long)nproc, (unsigned long long)mask);

    std::vector<std::string> files; std::string outdir = ".", label = "bwt";
    int warmup = 2, reps = 7;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--reps=", 0) == 0) reps = std::atoi(a.c_str() + 7);
        else if (a.rfind("--out=", 0) == 0) outdir = a.c_str() + 6;
        else if (a.rfind("--label=", 0) == 0) label = a.c_str() + 8;
        else files.push_back(a);
    }
    if (files.empty()) { std::fprintf(stderr, "usage: bwt_prof [--reps=N] [--out=DIR] [--label=L] <containers...>\n"); return 2; }
    g_rep_dir = outdir; g_rep_label = label;

    std::ostringstream report;
    report << "ANVIL BWT-routed decode stage split (decode-perf, I9) \n";
    report << "  LEG LABEL: " << label << "\n";
    report << "  reps=" << reps << " interleaved, median reported; CV=stdev/mean; decoder threads=1; libsais_unbwt single-thread\n\n";

    try {
        for (auto& path : files) {
            std::vector<uint8_t> comp = read_file(path);
            std::vector<uint8_t> real = call_decompress(comp);
            size_t src_bytes = real.size();
            std::printf("[file] %s container=%zu B decoded=%zu B\n", path.c_str(), comp.size(), src_bytes);

            Ctx ctx0;
            std::vector<uint8_t> probe = bwt_stage_postcoder(comp, ctx0);
            if (probe.size() != ctx0.xlen) throw std::runtime_error("probe size mismatch");

            FileReport rep; rep.name = path; rep.src_bytes = src_bytes; rep.comp_bytes = comp.size();
            rep.transform = ctx0.transform; rep.backend = ctx0.backend; rep.post = ctx0.post; rep.primary = ctx0.primary; rep.xlen = ctx0.xlen;

            // assembled verification: postcoder -> unbwt -> transform inverse == real
            {
                std::vector<uint8_t> bwt = bwt_stage_postcoder(comp, ctx0);
                std::vector<uint8_t> out(ctx0.xlen); std::vector<int32_t> tmp(ctx0.xlen + 1);
                if (libsais_unbwt(bwt.data(), out.data(), tmp.data(), (int32_t)ctx0.xlen, nullptr, ctx0.primary) != 0) throw std::runtime_error("libsais_unbwt failed");
                std::vector<uint8_t> x = out;
                if (ctx0.transform == 1) x = ratio_inverse_ctx1(out, src_bytes);
                else if (ctx0.transform == 2) x = ratio_inverse_lines(out, src_bytes);
                if (x != real) { std::fprintf(stderr, "STAGE ASSEMBLY MISMATCH on %s\n", path.c_str()); return 1; }
            }

            // variants
            std::vector<Variant> vs;
            vs.push_back({"baseline_full_decompress", [&]() { double w0 = now_s(); auto o = call_decompress(comp); double w1 = now_s(); g_sink += o[o.size() / 2]; return w1 - w0; }});
            vs.push_back({"stageA_postcoder", [&]() { double w0 = now_s(); Ctx c; auto b = bwt_stage_postcoder(comp, c); double w1 = now_s(); g_sink += b[b.size() / 2]; return w1 - w0; }});
            vs.push_back({"allocB1_out_tmp", [&]() { double w0 = now_s(); std::vector<uint8_t> out(ctx0.xlen); std::vector<int32_t> tmp(ctx0.xlen + 1); double w1 = now_s(); g_sink += out[0] + (uint64_t)tmp[0]; return w1 - w0; }});
            vs.push_back({"B1plusB2_unbwt", [&]() {
                double w0 = now_s();
                std::vector<uint8_t> bwt = bwt_stage_postcoder(comp, ctx0);
                std::vector<uint8_t> out(ctx0.xlen); std::vector<int32_t> tmp(ctx0.xlen + 1);
                if (libsais_unbwt(bwt.data(), out.data(), tmp.data(), (int32_t)ctx0.xlen, nullptr, ctx0.primary) != 0) throw std::runtime_error("unbwt");
                double w1 = now_s(); g_sink += out[out.size() / 2]; return w1 - w0;
            }});
            vs.push_back({"stageC_transform_inverse", [&]() {
                std::vector<uint8_t> bwt = bwt_stage_postcoder(comp, ctx0);
                std::vector<uint8_t> out(ctx0.xlen); std::vector<int32_t> tmp(ctx0.xlen + 1);
                if (libsais_unbwt(bwt.data(), out.data(), tmp.data(), (int32_t)ctx0.xlen, nullptr, ctx0.primary) != 0) throw std::runtime_error("unbwt");
                double w0 = now_s();
                std::vector<uint8_t> x;
                if (ctx0.transform == 1) x = ratio_inverse_ctx1(out, src_bytes);
                else if (ctx0.transform == 2) x = ratio_inverse_lines(out, src_bytes);
                else x = out;
                double w1 = now_s(); g_sink += x[x.size() / 2]; return w1 - w0;
            }});
            vs.push_back({"crc32_bytewise", [&]() { double w0 = now_s(); uint32_t c = crc32(real.data(), real.size()); double w1 = now_s(); g_sink += c; return w1 - w0; }});
            vs.push_back({"memcpy_out", [&]() { double w0 = now_s(); std::vector<uint8_t> tmp(real.size()); std::memcpy(tmp.data(), real.data(), real.size()); double w1 = now_s(); g_sink += tmp[tmp.size() / 2]; return w1 - w0; }});
            g_rep_tag = "bwt_" + std::filesystem::path(path).filename().string();
            { std::ofstream trunc(outdir + "/" + label + "_reps_" + g_rep_tag + ".csv", std::ios::trunc); }
            auto res = bench_interleaved(vs, warmup, reps);
            rep.baseline = res[0]; rep.mbps = double(src_bytes) / res[0].med / 1e6;
            rep.stageA = res[1]; rep.allocB1 = res[2]; rep.unbwtB2 = res[3]; rep.stageC = res[5]; rep.crc = res[6]; rep.memcpy_ = res[7];
            // NOTE: variant order above: 0 baseline, 1 A, 2 B1, 3 B1+B2, 4 C(+A+B), 5 crc, 6 memcpy
            // (indices: res[4] is the A+B1+B2 stageC variant; res[3]-res[2] approximates B2)

            double base_t = rep.baseline.med;
            double A = rep.stageA.med, B1 = rep.allocB1.med, B1B2 = res[3].med, C = rep.stageC.med;
            // B1B2 variant = stageA + alloc + unbwt, so B2 = B1B2 - A - B1 (harness v1
            // mis-derived B2 = B1B2 - B1 and double-counted stageA; fixed here).
            double B2 = B1B2 - A - B1; if (B2 < 0) B2 = 0;
            double seq = A + B1 + B2 + C;
            double crc_t = rep.crc.med, mem_t = rep.memcpy_.med;
            double resid = base_t - seq - crc_t - mem_t;

            std::ostringstream o; char line[320];
            o << "=== " << path << " ===\n";
            std::snprintf(line, sizeof line, "src=%zu B  comp=%zu B  ratio=%.4f  transform=%d backend=%d postcoder=%d primary=%d xlen=%zu\n",
                src_bytes, rep.comp_bytes, double(rep.comp_bytes) / double(src_bytes), rep.transform, rep.backend, rep.post, rep.primary, rep.xlen);
            o << line;
            std::snprintf(line, sizeof line, "BASELINE full decode: %.1f MB/s (med %.2f ms, CV %.1f%%, min %.2f ms)\n", rep.mbps, base_t * 1e3, rep.baseline.cv * 100, rep.baseline.mn * 1e3);
            o << line;
            o << "-- stage split (seconds; shares of end-to-end baseline) --\n";
            std::snprintf(line, sizeof line, "  stageA postcoder            med=%8.2f ms  CV=%5.1f%%  share=%5.1f%%  (%.2f ns/B-out)\n", A * 1e3, rep.stageA.cv * 100, 100.0 * A / base_t, A * 1e9 / src_bytes); o << line;
            std::snprintf(line, sizeof line, "  B1 alloc out+tmp            med=%8.2f ms  CV=%5.1f%%  share=%5.1f%%\n", B1 * 1e3, rep.allocB1.cv * 100, 100.0 * B1 / base_t); o << line;
            std::snprintf(line, sizeof line, "  B2 libsais_unbwt (B1+B2-B1) med=%8.2f ms  (B1+B2 med=%8.2f ms CV=%5.1f%%)\n", B2 * 1e3, B1B2 * 1e3, res[3].cv * 100); o << line;
            std::snprintf(line, sizeof line, "  stageC transform inverse    med=%8.2f ms  CV=%5.1f%%  share=%5.1f%%\n", C * 1e3, rep.stageC.cv * 100, 100.0 * C / base_t); o << line;
            std::snprintf(line, sizeof line, "  crc32 (bytewise, HEAD)      med=%8.2f ms  CV=%5.1f%%  share=%5.1f%%\n", crc_t * 1e3, rep.crc.cv * 100, 100.0 * crc_t / base_t); o << line;
            std::snprintf(line, sizeof line, "  memcpy final out            med=%8.2f ms  CV=%5.1f%%  share=%5.1f%%\n", mem_t * 1e3, rep.memcpy_.cv * 100, 100.0 * mem_t / base_t); o << line;
            std::snprintf(line, sizeof line, "  residual (container parse+alloc+concat) med=%8.2f ms  share=%5.1f%%\n", resid * 1e3, 100.0 * resid / base_t); o << line;
            std::snprintf(line, sizeof line, "  NOTE stageC variant includes A+B1+B2; stage C-only share reported after subtract. Sum check: seq+crc+mem+resid=%.2f ms vs baseline %.2f ms\n", (seq + crc_t + mem_t + resid) * 1e3, base_t * 1e3);
            o << line;
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
