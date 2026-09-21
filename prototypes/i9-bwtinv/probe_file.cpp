// i9-bwtinv probe: libsais bwt_aux / unbwt_aux semantics on a real file.
// usage: probe_file <rawfile> [r1 r2 ...]
#include "libsais.h"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <algorithm>

static std::vector<uint8_t> read_file(const char* path) {
    FILE* f = fopen(path, "rb"); if (!f) { fprintf(stderr, "cannot open %s\n", path); exit(2); }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> d((size_t)n);
    if (n > 0 && fread(d.data(), 1, (size_t)n, f) != (size_t)n) { fprintf(stderr, "short read\n"); exit(2); }
    fclose(f); return d;
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: probe_file <rawfile> [r...]\n"); return 2; }
    std::vector<uint8_t> T = read_file(argv[1]);
    const int32_t n = (int32_t)T.size();
    printf("n=%d\n", n);

    std::vector<uint8_t> bwt(n), bwt2(n);
    std::vector<int32_t> A(n), A2(n), freq(256), freq2(256);
    int32_t primary = libsais_bwt(T.data(), bwt.data(), A.data(), n, 0, freq.data());
    printf("libsais_bwt primary=%d\n", primary);

    std::vector<int32_t> rlist;
    if (argc > 2) for (int i = 2; i < argc; ++i) rlist.push_back(atoi(argv[i]));
    else rlist = { 4096, 32768, 131072, 524288 };

    for (int32_t r : rlist) {
        std::vector<int32_t> I((size_t)((n - 1) / r + 1), 0);
        int32_t rc = libsais_bwt_aux(T.data(), bwt2.data(), A2.data(), n, 0, freq2.data(), r, I.data());
        bool bwt_same = (rc == 0) && (bwt2 == bwt);
        // spot-check I[t] == ISA[t*r]+1 needs SA; instead spot-check ordering only
        printf("r=%-8d rc=%d I_n=%zu I0=%d primary=%d bwt_same=%d freq_same=%d\n",
               r, rc, I.size(), I.size() ? I[0] : -1, primary, (int)bwt_same, (int)(freq == freq2));
        if (!bwt_same) continue;
        // decode with libsais_bwt's freq
        std::vector<uint8_t> T2(n);
        int32_t rc2 = libsais_unbwt_aux(bwt.data(), T2.data(), A.data(), n, freq.data(), r, I.data());
        bool ok2 = (rc2 == 0 && T2 == T);
        // decode with NULL freq
        std::vector<uint8_t> T3(n);
        int32_t rc3 = libsais_unbwt_aux(bwt.data(), T3.data(), A.data(), n, nullptr, r, I.data());
        bool ok3 = (rc3 == 0 && T3 == T);
        // first mismatch position for the freq case
        size_t mm = 0; while (mm < (size_t)n && T2[mm] == T[mm]) ++mm;
        printf("      unbwt_aux freq=%s rc=%d | nullfreq=%s rc=%d | first_mismatch=%zu\n",
               ok2 ? "OK" : "FAIL", rc2, ok3 ? "OK" : "FAIL", rc3, mm);
        // also decode a single block boundary using libsais_unbwt_aux_style plain? no.
    }
    return 0;
}
