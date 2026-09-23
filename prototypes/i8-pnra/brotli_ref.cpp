// brotli_ref.cpp — minimal brotli/zstd size+time reference for a raw file.
// Used by the i8-pnra lane so prototypes get an honest reference column on
// THIS Windows host (the Linux PNRA numbers were EPYC/directional).
//
// Usage: brotli_ref <file> [qualities...]
//   qualities default: 1 4 6 9   (q11 deliberately out of inner loops)
// Prints CSV: file,size,q,bytes,ratio,enc_MBps,dec_MBps
#include <brotli/encode.h>
#include <brotli/decode.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

static std::vector<uint8_t> rdfile(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) { std::fprintf(stderr, "cannot open %s\n", p.c_str()); std::exit(1); }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: brotli_ref <file> [q...]\n"); return 2; }
    auto d = rdfile(argv[1]);
    std::vector<int> qs;
    if (argc > 2) for (int i = 2; i < argc; i++) qs.push_back(std::atoi(argv[i]));
    else qs = {1, 4, 6, 9};
    std::printf("file,input,q,bytes,ratio,enc_MBps,dec_MBps\n");
    for (int q : qs) {
        size_t cap = BrotliEncoderMaxCompressedSize(d.size()) + 64;
        std::vector<uint8_t> out(cap);
        size_t n = cap;
        using C = std::chrono::steady_clock;
        double best_e = 1e30, best_d = 1e30;
        size_t nb = 0;
        for (int rep = 0; rep < 3; rep++) {
            n = cap;
            auto A = C::now();
            if (!BrotliEncoderCompress(q, BROTLI_DEFAULT_WINDOW, BROTLI_MODE_GENERIC, d.size(), d.data(), &n, out.data())) {
                std::fprintf(stderr, "brotli encode failed\n"); return 1;
            }
            auto B = C::now();
            std::vector<uint8_t> back(d.size());
            size_t bl = d.size();
            if (BrotliDecoderDecompress(n, out.data(), &bl, back.data()) != BROTLI_DECODER_RESULT_SUCCESS || bl != d.size()) {
                std::fprintf(stderr, "brotli roundtrip failed\n"); return 1;
            }
            auto D = C::now();
            best_e = std::min(best_e, std::chrono::duration<double>(B - A).count());
            best_d = std::min(best_d, std::chrono::duration<double>(D - B).count());
            nb = n;
        }
        std::printf("%s,%zu,%d,%zu,%.4f,%.3f,%.3f\n", argv[1], d.size(), q, nb,
                    double(nb) / double(d.size()), d.size() / 1e6 / best_e, d.size() / 1e6 / best_d);
    }
    return 0;
}
