// I20-W4 prototype: isolated proof that the IN-PLACE inverse BWT is
// byte-identical to the separate-buffer path, on REAL data, for BOTH inverse
// paths (legacy libsais_unbwt and auxiliary libsais_unbwt_aux).
//
// The C4 lever L2 writes the reconstructed string over the BWT string itself,
// so libsais_unbwt_aux() receives the SAME pointer as T (the BWT string) and
// U (the output). libsais' LF walk reads only its own PSI array and the bigram
// bucket tables, never T -- but T and U are restrict-qualified, so a compiler
// is allowed to reload T[0] after the U writes have begun. The ANVIL decoder
// therefore captures bwt[0] before the call and re-applies it to bwt[n-1]
// afterwards.
//
// This prototype exercises that mechanism directly, with no codec involved:
//
//   1. build the BWT of the input with libsais_bwt / libsais_bwt_aux;
//   2. inverse it with SEPARATE buffers (reference);
//   3. inverse it IN PLACE, capturing/restoring the first byte exactly the way
//      src/anvil.cpp does;
//   4. compare the two results byte for byte, and compare the in-place result
//      against the ORIGINAL input (a full round-trip);
//   5. repeat for a range of input sizes (degenerate / odd / even / multi-piece
//      aux paths) and for every file given on the command line at FULL size.
//
// Build (from the repo root):
//   clang -O2 -DNDEBUG -std=c11 -I third_party/libsais/include \
//         -c third_party/libsais/src/libsais.c -o libsais.o
//   clang++ -O2 -std=c++17 -I third_party/libsais/include \
//           prototypes/i20-working-set/inplace_unbwt.cpp libsais.o \
//           -o inplace_unbwt
//
// Run:
//   ./inplace_unbwt <file> [more files...]
//
// With no file arguments a deterministic synthetic corpus is used.
// Exit status 0 = every case matched byte for byte.

#include <libsais.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int g_fail = 0;
static int g_cases = 0;
static int g_bytes = 0;

static std::vector<uint8_t> read_all(const char* path) {
    std::FILE* f = std::fopen(path, "rb");
    if (!f) { std::fprintf(stderr, "cannot open %s\n", path); std::exit(2); }
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> d(static_cast<size_t>(n < 0 ? 0 : n));
    if (!d.empty() && std::fread(d.data(), 1, d.size(), f) != d.size()) { std::fclose(f); std::exit(2); }
    std::fclose(f);
    return d;
}

// The encoder's auxiliary sampling policy (src/anvil.cpp bwt_aux_rate_for_size):
// r = smallest power of two with r * 1024 >= n.
static int32_t aux_rate_for_size(int32_t n) {
    uint64_t target = (static_cast<uint64_t>(n) + 1023) / 1024;
    uint64_t r = 2;
    while (r < target) r <<= 1;
    return static_cast<int32_t>(r);
}

// Reference path: separate input and output buffers.
static std::vector<uint8_t> inverse_separate(const std::vector<uint8_t>& bwt,
                                             const std::vector<int32_t>& aux,
                                             int32_t aux_rate,
                                             int32_t primary) {
    const int32_t n = static_cast<int32_t>(bwt.size());
    std::vector<uint8_t> out(static_cast<size_t>(n));
    std::vector<int32_t> tmp(static_cast<size_t>(n) + 1);
    int32_t rc = aux_rate > 0
        ? libsais_unbwt_aux(bwt.data(), out.data(), tmp.data(), n, nullptr, aux_rate, aux.data())
        : libsais_unbwt(bwt.data(), out.data(), tmp.data(), n, nullptr, primary);
    if (rc != 0) { std::fprintf(stderr, "libsais inverse failed rc=%d\n", rc); std::exit(2); }
    return out;
}

// Candidate path: the output is written over the BWT string, with the first
// byte captured before the call and re-applied afterwards (src/anvil.cpp).
static std::vector<uint8_t> inverse_inplace(std::vector<uint8_t> bwt,
                                            const std::vector<int32_t>& aux,
                                            int32_t aux_rate,
                                            int32_t primary) {
    const int32_t n = static_cast<int32_t>(bwt.size());
    const uint8_t first = bwt[0];
    std::vector<int32_t> tmp(static_cast<size_t>(n) + 1);
    int32_t rc = aux_rate > 0
        ? libsais_unbwt_aux(bwt.data(), bwt.data(), tmp.data(), n, nullptr, aux_rate, aux.data())
        : libsais_unbwt(bwt.data(), bwt.data(), tmp.data(), n, nullptr, primary);
    if (rc != 0) { std::fprintf(stderr, "libsais inverse failed rc=%d\n", rc); std::exit(2); }
    bwt[static_cast<size_t>(n) - 1] = first;
    return bwt;
}

static void report(const char* label, const std::vector<uint8_t>& data, int32_t aux_rate,
                   const std::vector<uint8_t>& ref, const std::vector<uint8_t>& got) {
    ++g_cases;
    g_bytes += static_cast<int>(data.size());
    if (ref.size() != got.size() || std::memcmp(ref.data(), got.data(), ref.size()) != 0) {
        ++g_fail;
        std::fprintf(stderr, "MISMATCH %s aux_rate=%d (separate != in-place)\n", label, aux_rate);
        return;
    }
    if (std::memcmp(got.data(), data.data(), data.size()) != 0) {
        ++g_fail;
        std::fprintf(stderr, "ROUND-TRIP MISMATCH %s aux_rate=%d\n", label, aux_rate);
        return;
    }
    std::printf("ok %-28s aux_rate=%-9d bytes=%zu\n", label, aux_rate, got.size());
}

static void check_case(const std::vector<uint8_t>& data, int32_t aux_rate, const char* label) {
    const int32_t n = static_cast<int32_t>(data.size());
    if (n <= 0) return;
    std::vector<uint8_t> bwt(static_cast<size_t>(n));
    std::vector<int32_t> tmp(static_cast<size_t>(n) + 1);
    std::vector<int32_t> aux;
    int32_t primary = 0;
    if (aux_rate > 0) {
        const uint64_t count = 1 + (static_cast<uint64_t>(n) - 1) / static_cast<uint64_t>(aux_rate);
        aux.resize(static_cast<size_t>(count));
        if (libsais_bwt_aux(data.data(), bwt.data(), tmp.data(), n, 0, nullptr, aux_rate, aux.data()) != 0) {
            std::fprintf(stderr, "libsais_bwt_aux failed n=%d r=%d\n", n, aux_rate);
            std::exit(2);
        }
        primary = aux[0];
    } else {
        primary = libsais_bwt(data.data(), bwt.data(), tmp.data(), n, 0, nullptr);
        if (primary < 1 || primary > n) { std::fprintf(stderr, "libsais_bwt failed n=%d\n", n); std::exit(2); }
    }
    std::vector<uint8_t> ref = inverse_separate(bwt, aux, aux_rate, primary);
    std::vector<uint8_t> got = inverse_inplace(bwt, aux, aux_rate, primary);
    report(label, data, aux_rate, ref, got);
}

int main(int argc, char** argv) {
    // 1) Full-size, real-data cases: every file given on the command line, at
    //    both the legacy inverse (r == n) and the encoder's auxiliary policy.
    for (int i = 1; i < argc; ++i) {
        std::vector<uint8_t> d = read_all(argv[i]);
        if (d.empty()) continue;
        const int32_t n = static_cast<int32_t>(d.size());
        std::string label = argv[i];
        if (label.size() > 24) label = label.substr(label.size() - 24);
        check_case(d, 0, label.c_str());                                  // legacy inverse
        check_case(d, aux_rate_for_size(n), (label + " [aux]").c_str());   // auxiliary inverse
    }

    // 2) Synthetic size sweep: degenerate, odd/even and multi-piece aux paths.
    std::vector<uint8_t> corpus;
    for (int i = 0; i < 8192; ++i) corpus.push_back(static_cast<uint8_t>((i * 37 + (i >> 3)) & 0xFF));
    const int32_t sizes[] = {1, 2, 3, 7, 16, 17, 63, 64, 65, 255, 256, 257, 1023, 1024, 1025, 4096, 4097, 65535, 65536, 65537};
    for (int32_t s : sizes) {
        if (static_cast<size_t>(s) > corpus.size()) continue;
        std::vector<uint8_t> d(corpus.begin(), corpus.begin() + s);
        check_case(d, 0, "synthetic");
        check_case(d, 2, "synthetic");
        check_case(d, 64, "synthetic");
        check_case(d, 4096, "synthetic");
    }
    std::printf("cases=%d failures=%d bytes=%d\n", g_cases, g_fail, g_bytes);
    return g_fail ? 1 : 0;
}
