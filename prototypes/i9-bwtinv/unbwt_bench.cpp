// ============================================================================
// ANVIL i9-bwtinv — libsais inverse-BWT throughput microbench
//
// Purpose: measure the true cost of the inverse BWT step that ANVIL's BWT
// backend calls (src/anvil.cpp:4144 libsais_unbwt) and of prototype
// alternatives, on real payloads, with byte-identity gates.
//
// Variants (all outputs verified byte-identical to the original input):
//   REF build (shipped libsais, no OpenMP):
//     plain_fresh   libsais_unbwt, fresh U+A allocations per rep (== ANVIL call)
//     plain_reuse   libsais_unbwt, buffers reused (isolates alloc/page-fault)
//     plain_freq    libsais_unbwt with precomputed freq (isolates histogram)
//     aux_r<r>      libsais_unbwt_aux with encoder-provided index I (r samples)
//   OMP build (same source, -DLIBSAIS_OPENMP / libsais's documented flag):
//     omp_rn_t<t>   libsais_unbwt_aux_omp with r=n (I={primary}) -> parallel init
//     omp_r<r>_t<t> libsais_unbwt_aux_omp with r<n -> parallel init + parallel walk
//
// Timing: QPC, interleaved reps, median + CV, optional affinity restriction.
// Every row prints ok only after a byte-exact comparison vs the raw input.
// ============================================================================
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <stdexcept>

#include "libsais.h"

static double g_qpc = 0.0;
static double now_s() { LARGE_INTEGER c; QueryPerformanceCounter(&c); return double(c.QuadPart) / g_qpc; }

struct Stats { double med = 0, mn = 0, mx = 0, cv = 0, mean = 0; int n = 0; };
static Stats stats_of(std::vector<double> v) {
    Stats s; s.n = (int)v.size(); if (v.empty()) return s;
    std::sort(v.begin(), v.end());
    s.med = v[v.size() / 2]; s.mn = v.front(); s.mx = v.back();
    double sum = 0; for (double x : v) sum += x; s.mean = sum / v.size();
    double var = 0; for (double x : v) var += (x - s.mean) * (x - s.mean); var /= v.size();
    s.cv = s.mean > 0 ? std::sqrt(var) / s.mean : 0; return s;
}

static std::vector<uint8_t> read_file(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) throw std::runtime_error("cannot open " + path);
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> d((size_t)n);
    if (n > 0 && fread(d.data(), 1, (size_t)n, f) != (size_t)n) { fclose(f); throw std::runtime_error("short read"); }
    fclose(f); return d;
}

static std::string hex16(uint64_t h) { char b[32]; snprintf(b, sizeof b, "%016llx", (unsigned long long)h); return b; }
static uint64_t fnv1a(const std::vector<uint8_t>& d) {
    uint64_t h = 1469598103934665603ull;
    for (uint8_t c : d) { h ^= c; h *= 1099511628211ull; }
    return h;
}

// shared, outlives every lambda
struct Buf {
    std::vector<uint8_t> U;
    std::vector<int32_t> A;
    std::vector<int32_t> I;
    int32_t r = 0;
};
using BufP = std::shared_ptr<Buf>;

struct Variant {
    std::string name;
    std::function<bool()> verify;   // byte-identity vs original
    std::function<void()> run;      // timed
    bool ok = false;
    int tcount = 1;
    std::string note;
};

int main(int argc, char** argv) {
    LARGE_INTEGER fq; QueryPerformanceFrequency(&fq); g_qpc = double(fq.QuadPart);
    setvbuf(stdout, nullptr, _IONBF, 0); setvbuf(stderr, nullptr, _IONBF, 0);

    int reps = 7, warmup = 1, threads = 0, affinityCores = 0;
    std::vector<std::string> files;
    std::string csv;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--reps=", 0) == 0) reps = atoi(a.c_str() + 7);
        else if (a.rfind("--warmup=", 0) == 0) warmup = atoi(a.c_str() + 9);
        else if (a.rfind("--threads=", 0) == 0) threads = atoi(a.c_str() + 10);
        else if (a.rfind("--affinity=", 0) == 0) affinityCores = atoi(a.c_str() + 11);
        else if (a.rfind("--csv=", 0) == 0) csv = a.substr(6);
        else files.push_back(a);
    }
    if (files.empty()) {
        fprintf(stderr, "usage: unbwt_bench [--reps=N] [--warmup=N] [--threads=N] [--affinity=N] [--csv=out.csv] <rawfile>...\n");
        return 2;
    }
    if (affinityCores > 0) {
        DWORD_PTR mask = (affinityCores >= 64) ? ~(DWORD_PTR)0 : (((DWORD_PTR)1 << affinityCores) - 1);
        if (!SetProcessAffinityMask(GetCurrentProcess(), mask)) fprintf(stderr, "warn: affinity mask failed\n");
    }
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#ifdef USE_OMP
    const char* build = "OMP";
#else
    const char* build = "REF";
#endif
    printf("=== i9-bwtinv unbwt bench [%s build] reps=%d warmup=%d threads=%d affinity=%d host=Ryzen9-5900X ===\n",
           build, reps, warmup, threads, affinityCores);

    FILE* cfo = csv.empty() ? nullptr : fopen(csv.c_str(), "a");
    if (cfo) fprintf(cfo, "file,variant,build,n_bytes,reps,median_ms,mean_ms,min_ms,max_ms,cv_pct,MBps,ns_per_B,threads,note,reps_ms\n");

    for (const auto& path : files) {
        std::vector<uint8_t> in;
        try { in = read_file(path); } catch (std::exception& e) { printf("%s: SKIP (%s)\n", path.c_str(), e.what()); continue; }
        if (in.size() < 2 || in.size() > (size_t)INT32_MAX) { printf("%s: SKIP (size)\n", path.c_str()); continue; }
        const int32_t n = (int32_t)in.size();
        printf("\nFILE %s  n=%d  input_fnv=%s\n", path.c_str(), n, hex16(fnv1a(in)).c_str());

        // --- encoder-side setup (not timed) ---
        std::vector<uint8_t> bwt(n);
        std::vector<int32_t> sa(n);
        std::vector<int32_t> freq(256);
        int32_t primary = libsais_bwt(in.data(), bwt.data(), sa.data(), n, 0, freq.data());
        if (primary <= 0 || primary > n) { printf("  bwt failed rc=%d\n", primary); continue; }
        printf("  primary=%d bwt_fnv=%s\n", primary, hex16(fnv1a(bwt)).c_str());

        auto pow2_floor = [](int64_t x) { int64_t p = 1; while (p * 2 <= x) p *= 2; return p; };
        auto blocks_to_r = [&](int64_t blocks) { int64_t r = pow2_floor(((int64_t)n - 1) / blocks + 1); return (int32_t)(r < 2 ? 2 : r); };

        std::vector<int64_t> block_list;
#ifdef USE_OMP
        block_list = { 16, 64, 256 };
#else
        block_list = { 8, 32, 128, 1024 };
#endif
        struct Aux { int32_t r; bool bwt_match; };
        std::vector<Aux> auxes;
        std::vector<BufP> aux_bufs;
        for (int64_t bl : block_list) {
            int32_t r = blocks_to_r(bl);
            if (r == n) continue;
            bool dup = false; for (auto& a : auxes) if (a.r == r) dup = true;
            if (dup) continue;
            BufP B = std::make_shared<Buf>();
            B->r = r; B->I.assign((size_t)((n - 1) / r + 1), 0);
            B->U.resize(n); B->A.assign((size_t)n + 1, 0);
            std::vector<uint8_t> b2(n); std::vector<int32_t> sa2(n); std::vector<int32_t> fr2(256);
            int32_t rc = libsais_bwt_aux(in.data(), b2.data(), sa2.data(), n, 0, fr2.data(), r, B->I.data());
            bool match = (rc == 0 && b2 == bwt && B->I[0] == primary);
            auxes.push_back({ r, match });
            aux_bufs.push_back(B);
            printf("  aux r=%-9d I_entries=%-5zu I_bytes=%-6zu bwt_identity=%s\n",
                   r, B->I.size(), B->I.size() * sizeof(int32_t), match ? "YES" : "NO");
        }

        std::vector<Variant> vs;
        std::vector<BufP> keepalive;

        auto add_buf_variant = [&](const std::string& nm, BufP B, const int32_t* freqp, int32_t rr, const int32_t* Ip, int t) {
            keepalive.push_back(B);
            Variant v; v.name = nm; v.tcount = (t > 0 ? t : 1);
            v.verify = [&in, &bwt, B, freqp, rr, Ip, t, n]() -> bool {
#ifdef USE_OMP
                if (t > 0) return libsais_unbwt_aux_omp(bwt.data(), B->U.data(), B->A.data(), n, freqp, rr, Ip, t) == 0 && B->U == in;
#endif
                return libsais_unbwt_aux(bwt.data(), B->U.data(), B->A.data(), n, freqp, rr, Ip) == 0 && B->U == in;
            };
            v.run = [B, freqp, rr, Ip, t, &bwt, n]() {
#ifdef USE_OMP
                if (t > 0) libsais_unbwt_aux_omp(bwt.data(), B->U.data(), B->A.data(), n, freqp, rr, Ip, t);
                else
#endif
                    libsais_unbwt_aux(bwt.data(), B->U.data(), B->A.data(), n, freqp, rr, Ip);
                volatile uint8_t s = B->U[0]; (void)s;
            };
            vs.push_back(std::move(v));
        };

        // (1) plain_fresh — exactly ANVIL's call shape: fresh out + tmp per call
        vs.push_back({ "plain_fresh",
            [&] { std::vector<uint8_t> U(n); std::vector<int32_t> A((size_t)n + 1);
                  return libsais_unbwt(bwt.data(), U.data(), A.data(), n, nullptr, primary) == 0 && U == in; },
            [&] { std::vector<uint8_t> U(n); std::vector<int32_t> A((size_t)n + 1);
                  libsais_unbwt(bwt.data(), U.data(), A.data(), n, nullptr, primary);
                  volatile uint8_t s = U[0]; (void)s; } });

        // (2/3) reuse + freq
        { BufP B = std::make_shared<Buf>(); B->U.resize(n); B->A.assign((size_t)n + 1, 0);
          add_buf_variant("plain_reuse", B, nullptr, n, &primary, 0); }
        { BufP B = std::make_shared<Buf>(); B->U.resize(n); B->A.assign((size_t)n + 1, 0);
          add_buf_variant("plain_freq", B, freq.data(), n, &primary, 0); }

        // (4) aux single-thread
        for (size_t i = 0; i < auxes.size(); ++i) {
            if (!auxes[i].bwt_match) continue;
            add_buf_variant("aux_r" + std::to_string(auxes[i].r), aux_bufs[i], freq.data(), auxes[i].r, aux_bufs[i]->I.data(), 0);
        }
#ifdef USE_OMP
        // (5) OMP variants: plain index (r=n, parallel init only) + r<n (full parallel)
        for (int t : { 1, 4, 8, 16 }) {
            BufP B = std::make_shared<Buf>(); B->U.resize(n); B->A.assign((size_t)n + 1, 0);
            add_buf_variant("omp_rn_t" + std::to_string(t), B, freq.data(), n, &primary, t);
            for (size_t i = 0; i < auxes.size(); ++i) {
                if (!auxes[i].bwt_match) continue;
                int32_t r = auxes[i].r; int32_t blocks = 1 + (n - 1) / r;
                if (t > 1 && blocks < t) continue;      // no point: fewer blocks than threads
                add_buf_variant("omp_r" + std::to_string(r) + "_t" + std::to_string(t),
                                aux_bufs[i], freq.data(), r, aux_bufs[i]->I.data(), t);
            }
        }
#endif

        // --- verify all variants once ---
        fprintf(stderr, "[mark] variants built: %zu\n", vs.size());
        for (auto& v : vs) {
            try { v.ok = v.verify(); } catch (std::exception& e) { v.ok = false; v.note = e.what(); }
        }
        bool any_bad = false;
        for (auto& v : vs) if (!v.ok) { printf("  VERIFY FAIL: %s %s\n", v.name.c_str(), v.note.c_str()); any_bad = true; }
        if (any_bad) { printf("  aborting file: byte-identity gate failed\n"); continue; }

        // --- interleaved timing ---
        for (auto& v : vs) for (int i = 0; i < warmup; ++i) v.run();
        std::vector<std::vector<double>> S(vs.size());
        for (int r = 0; r < reps; ++r)
            for (size_t i = 0; i < vs.size(); ++i) {
                double t0 = now_s(); vs[i].run(); double t1 = now_s();
                S[i].push_back(t1 - t0);
            }

        printf("  %-26s %10s %10s %9s %8s %8s  %s\n", "variant", "median_ms", "MB/s", "ns/B", "mean_ms", "CV%", "verify");
        for (size_t i = 0; i < vs.size(); ++i) {
            Stats st = stats_of(S[i]);
            double MBps = (double)n / st.med / 1e6;
            double nsB = st.med * 1e9 / (double)n;
            printf("  %-26s %10.3f %10.1f %9.3f %8.3f %7.1f%%  ok\n",
                   vs[i].name.c_str(), st.med * 1e3, MBps, nsB, st.mean * 1e3, st.cv * 100.0);
            if (reps <= 12) {
                printf("      reps_ms=");
                for (double x : S[i]) printf("%.3f ", x * 1e3);
                printf("\n");
            }
            if (cfo) {
                const char* note = vs[i].name.rfind("omp_", 0) == 0 ? "omp_prototype"
                                 : (vs[i].name.rfind("aux_", 0) == 0 ? "aux_index_input" : "baseline");
                fprintf(cfo, "%s,%s,%s,%d,%d,%.4f,%.4f,%.4f,%.4f,%.2f,%.2f,%.4f,%d,%s,",
                        path.c_str(), vs[i].name.c_str(), build, n, reps,
                        st.med * 1e3, st.mean * 1e3, st.mn * 1e3, st.mx * 1e3, st.cv * 100.0,
                        MBps, nsB, vs[i].tcount, note);
                for (size_t r = 0; r < S[i].size(); ++r) fprintf(cfo, "%s%.4f", r ? ";" : "", S[i][r] * 1e3);
                fprintf(cfo, "\n");
            }
        }
        fflush(stdout);
    }
    if (cfo) fclose(cfo);
    return 0;
}
