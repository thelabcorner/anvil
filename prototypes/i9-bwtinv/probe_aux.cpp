// Minimal libsais bwt_aux/unbwt_aux semantics probe (i9-bwtinv lane).
// Checks: (1) I[t] vs ISA[t*r], (2) unbwt_aux roundtrip with r<n.
#include "libsais.h"
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>

int main() {
    const int32_t n = 64;
    std::vector<uint8_t> T(n);
    for (int32_t i = 0; i < n; ++i) T[i] = (uint8_t)((i * 37 + 11) % 251);

    std::vector<int32_t> SA(n), A(n), freq(256);
    int32_t rc = libsais(T.data(), SA.data(), n, 0, freq.data());
    printf("libsais rc=%d\n", rc);
    std::vector<int32_t> ISA(n);
    for (int32_t i = 0; i < n; ++i) ISA[SA[i]] = i;

    for (int32_t r : { 2, 4, 8, 16, 32 }) {
        std::vector<uint8_t> U(n);
        std::vector<int32_t> I((size_t)((n - 1) / r + 1), 0);
        std::vector<int32_t> A2(n), f2(256);
        int32_t rc2 = libsais_bwt_aux(T.data(), U.data(), A2.data(), n, 0, f2.data(), r, I.data());
        printf("r=%2d bwt_aux rc=%d I=[", r, rc2);
        for (size_t t = 0; t < I.size(); ++t) printf("%d%s", I[t], t + 1 < I.size() ? "," : "");
        printf("]  ISA[t*r]=[");
        for (size_t t = 0; t < I.size(); ++t) printf("%d%s", ISA[t * r], t + 1 < I.size() ? "," : "");
        printf("]\n");

        std::vector<uint8_t> T2(n);
        int32_t rc3 = libsais_unbwt_aux(U.data(), T2.data(), A2.data(), n, f2.data(), r, I.data());
        bool ok = (rc3 == 0 && T2 == T);
        printf("      unbwt_aux rc=%d roundtrip=%s\n", rc3, ok ? "OK" : "FAIL");
        if (!ok) {
            printf("      got: "); for (int i = 0; i < n; ++i) printf("%02x", T2[i]);
            printf("\n      exp: "); for (int i = 0; i < n; ++i) printf("%02x", T[i]);
            printf("\n");
        }
    }
    // Harness-shaped test: BWT via libsais_bwt, decode via unbwt_aux with libsais_bwt's freq.
    for (int32_t r : { 2, 4, 8, 16, 32 }) {
        std::vector<uint8_t> U1(n), U2(n);
        std::vector<int32_t> A3(n), f3(256), A4(n), f4(256);
        int32_t prim = libsais_bwt(T.data(), U1.data(), A3.data(), n, 0, f3.data());
        std::vector<int32_t> I((size_t)((n - 1) / r + 1), 0);
        int32_t rc4 = libsais_bwt_aux(T.data(), U2.data(), A4.data(), n, 0, f4.data(), r, I.data());
        bool freq_same = (f3 == f4);
        std::vector<uint8_t> T2(n);
        int32_t rc3 = libsais_unbwt_aux(U1.data(), T2.data(), A3.data(), n, f3.data(), r, I.data());
        bool ok = (rc3 == 0 && T2 == T);
        printf("harness-shape r=%2d prim=%d rc4=%d freq_bwt==freq_aux:%d unbwt_aux=%s\n",
               r, prim, rc4, (int)freq_same, ok ? "OK" : "FAIL");
        if (!freq_same) {
            printf("  f3: "); for (int i = 0; i < 8; ++i) printf("%d ", f3[i]);
            printf("\n  f4: "); for (int i = 0; i < 8; ++i) printf("%d ", f4[i]); printf("\n");
        }
        // also try freq=NULL
        std::vector<uint8_t> T3(n);
        int32_t rc5 = libsais_unbwt_aux(U1.data(), T3.data(), A3.data(), n, nullptr, r, I.data());
        printf("           freq=NULL rc=%d %s\n", rc5, (rc5 == 0 && T3 == T) ? "OK" : "FAIL");
        (void)prim;
    }
    return 0;
}
