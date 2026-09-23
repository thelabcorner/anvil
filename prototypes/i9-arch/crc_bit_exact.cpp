// arch I9 / dec-crc LEG 1: independent bit-exactness harness for the three
// crc32 candidates (IEEE reflected poly 0xEDB88320, init/final 0xFFFFFFFF):
//   A = bytewise table         (HEAD fc23d9a ground truth)
//   B = slicing-by-8          (current dirty-tree implementation)
//   C = PCLMULQDQ 4-way fold  (candidate integration; basis prototypes/i8-decode/crc_pclmul_dropin.h)
//
// Purpose: prove A == B == C on a broad length/pattern sweep + the standard
// CRC-32 vector before either is trusted as wire-invisible. This file is a
// test-only copy; it is NOT included by src/anvil.cpp.
//
// Build (from repo root, after `. .\env.ps1`):
//   clang-cl /nologo /O2 /std:c++20 /EHsc prototypes\i9-arch\crc_bit_exact.cpp /Fe:prototypes\i9-arch\crc_bit_exact.exe
// Run: prototypes\i9-arch\crc_bit_exact.exe
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <array>
#include <vector>
#include <random>
#include <immintrin.h>
#include <wmmintrin.h>

// ---------------- A: bytewise (HEAD fc23d9a) ----------------
static uint32_t crc32_bytewise(const uint8_t* p, size_t n) {
    static std::array<uint32_t,256> table = []{
        std::array<uint32_t,256> t{};
        for(uint32_t i=0;i<256;++i){ uint32_t c=i; for(int k=0;k<8;++k)c=(c&1)?(0xEDB88320u^(c>>1)):(c>>1); t[i]=c; }
        return t;
    }();
    uint32_t c=0xFFFFFFFFu;
    for(size_t i=0;i<n;++i)c=table[(c^p[i])&0xFFu]^(c>>8);
    return c^0xFFFFFFFFu;
}

// ---------------- B: slicing-by-8 (current dirty tree) ----------------
static uint32_t crc32_slice8(const uint8_t* p, size_t n) {
    static std::array<std::array<uint32_t,256>,8> qtabs = []{
        std::array<std::array<uint32_t,256>,8> t{};
        for(uint32_t i=0;i<256;++i){ uint32_t c=i; for(int k=0;k<8;++k)c=(c&1)?(0xEDB88320u^(c>>1)):(c>>1); t[7][i]=c; }
        for(int s=0;s<8;++s){
            int steps=7-s;
            for(uint32_t i=0;i<256;++i){
                uint32_t c=t[7][i];
                for(int r=0;r<steps;++r) c=(t[7][c&0xFFu])^(c>>8);
                t[s][i]=c;
            }
        }
        return t;
    }();
    uint32_t c=0xFFFFFFFFu;
    while(n>=8){
        uint32_t one = c ^ ((uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24));
        uint32_t two = (uint32_t)p[4] | ((uint32_t)p[5]<<8) | ((uint32_t)p[6]<<16) | ((uint32_t)p[7]<<24);
        c = qtabs[0][one&0xFFu]
          ^ qtabs[1][(one>>8)&0xFFu]
          ^ qtabs[2][(one>>16)&0xFFu]
          ^ qtabs[3][one>>24]
          ^ qtabs[4][two&0xFFu]
          ^ qtabs[5][(two>>8)&0xFFu]
          ^ qtabs[6][(two>>16)&0xFFu]
          ^ qtabs[7][two>>24];
        p+=8; n-=8;
    }
    for(size_t i=0;i<n;++i)c=qtabs[7][(c^p[i])&0xFFu]^(c>>8);
    return c^0xFFFFFFFFu;
}

// ---------------- C: PCLMULQDQ (candidate) ----------------
#if defined(__clang__)
__attribute__((target("pclmul,sse4.1")))
#endif
static uint32_t crc32_pclmul(const uint8_t* src, size_t len) {
    if (len < 64) return crc32_bytewise(src, len);
    const __m128i f4  = _mm_set_epi32(0x00000001, 0x54442bd4, 0x00000001, 0xc6e41596);
    const __m128i k12 = _mm_set_epi32(0x00000001, 0x751997d0, 0x00000000, 0xccaa009e);
    const __m128i bk  = _mm_set_epi32(0x00000001, 0xdb710640, 0xb4e5b025, 0xf7011641);
    __m128i c0 = _mm_cvtsi32_si128(0x9db42487);
    __m128i c1 = _mm_setzero_si128();
    __m128i c2 = _mm_setzero_si128();
    __m128i c3 = _mm_setzero_si128();
    while (len >= 64) {
        __m128i t0 = _mm_loadu_si128((const __m128i*)(src));
        __m128i t1 = _mm_loadu_si128((const __m128i*)(src + 16));
        __m128i t2 = _mm_loadu_si128((const __m128i*)(src + 32));
        __m128i t3 = _mm_loadu_si128((const __m128i*)(src + 48));
        src += 64; len -= 64;
        __m128i l0 = _mm_clmulepi64_si128(c0, f4, 0x01), h0 = _mm_clmulepi64_si128(c0, f4, 0x10);
        __m128i l1 = _mm_clmulepi64_si128(c1, f4, 0x01), h1 = _mm_clmulepi64_si128(c1, f4, 0x10);
        __m128i l2 = _mm_clmulepi64_si128(c2, f4, 0x01), h2 = _mm_clmulepi64_si128(c2, f4, 0x10);
        __m128i l3 = _mm_clmulepi64_si128(c3, f4, 0x01), h3 = _mm_clmulepi64_si128(c3, f4, 0x10);
        c0 = _mm_xor_si128(_mm_xor_si128(l0, h0), t0);
        c1 = _mm_xor_si128(_mm_xor_si128(l1, h1), t1);
        c2 = _mm_xor_si128(_mm_xor_si128(l2, h2), t2);
        c3 = _mm_xor_si128(_mm_xor_si128(l3, h3), t3);
    }
    { __m128i lo=_mm_clmulepi64_si128(c0,k12,0x01), hi=_mm_clmulepi64_si128(c0,k12,0x10);
      c1=_mm_xor_si128(_mm_xor_si128(c1,lo),hi);
      lo=_mm_clmulepi64_si128(c1,k12,0x01); hi=_mm_clmulepi64_si128(c1,k12,0x10);
      c2=_mm_xor_si128(_mm_xor_si128(c2,lo),hi);
      lo=_mm_clmulepi64_si128(c2,k12,0x01); hi=_mm_clmulepi64_si128(c2,k12,0x10);
      c3=_mm_xor_si128(_mm_xor_si128(c3,lo),hi); }
    __m128i x0 = _mm_clmulepi64_si128(c3, bk, 0x00);
    __m128i x1 = _mm_clmulepi64_si128(x0, bk, 0x10);
    x1 = _mm_blend_epi16(x1, _mm_setzero_si128(), 0xcf);
    x0 = _mm_xor_si128(x1, c3);
    __m128i ra = _mm_clmulepi64_si128(x0, bk, 0x01);
    __m128i rb = _mm_clmulepi64_si128(ra, bk, 0x10);
    uint32_t reg = (uint32_t)_mm_extract_epi32(rb, 2);
    if (len) {
        static const std::array<uint32_t,256> T = []{
            std::array<uint32_t,256> t{}; for(uint32_t i=0;i<256;++i){uint32_t c=i;for(int k=0;k<8;++k)c=(c&1)?(0xEDB88320u^(c>>1)):(c>>1);t[i]=c;} return t; }();
        for (size_t i = 0; i < len; ++i) reg = T[(reg ^ src[i]) & 0xFFu] ^ (reg >> 8);
    }
    return ~reg;
}

static int failures = 0, checks = 0;
static void expect(const char* what, size_t n, uint32_t a, uint32_t b, uint32_t c) {
    ++checks;
    if (a != b || a != c) {
        ++failures;
        std::printf("FAIL %-28s n=%-8zu bytewise=%08X slice8=%08X pclmul=%08X\n", what, n, a, b, c);
    }
}

static void sweep(const char* what, const std::vector<uint8_t>& d) {
    expect(what, d.size(), crc32_bytewise(d.data(), d.size()), crc32_slice8(d.data(), d.size()),
           crc32_pclmul(d.data(), d.size()));
}

int main() {
    // 1) standard vector
    const char* v = "123456789";
    uint32_t c = crc32_bytewise((const uint8_t*)v, 9);
    std::printf("vector \"123456789\": bytewise=%08X slice8=%08X pclmul=%08X (expected CBF43926)\n",
                c, crc32_slice8((const uint8_t*)v, 9), crc32_pclmul((const uint8_t*)v, 9));
    expect("vector", 9, c, crc32_slice8((const uint8_t*)v, 9), crc32_pclmul((const uint8_t*)v, 9));
    if (c != 0xCBF43926u) { ++failures; std::printf("FAIL vector constant\n"); }

    // 2) length sweep 0..4113 over several deterministic patterns
    std::mt19937 rng(0xA11E5EED);
    std::vector<uint8_t> rnd(70000);
    for (auto& x : rnd) x = (uint8_t)rng();
    for (size_t n = 0; n <= 4113; ++n) {
        std::vector<uint8_t> a(n), b(n), z(n);
        for (size_t i=0;i<n;++i){ a[i]=(uint8_t)(i*131u+7u); b[i]=(uint8_t)(i*17u); z[i]=0xFF; }
        sweep("ramp131", a);
        sweep("ramp17", b);
        sweep("allFF", z);
        sweep("random", std::vector<uint8_t>(rnd.begin(), rnd.begin()+n));
    }
    // 3) large lengths (cross the 64-byte fold loop tail in every phase)
    for (size_t n : {(size_t)4096,(size_t)65536,(size_t)70000}) {
        sweep("large-random", std::vector<uint8_t>(rnd.begin(), rnd.begin()+n));
        std::vector<uint8_t> z(n,0); sweep("large-zero", z);
        std::vector<uint8_t> f(n,0xFF); sweep("large-FF", f);
    }
    // 4) unaligned starts
    for (size_t off=1; off<64; ++off) {
        expect("unaligned", 5000, crc32_bytewise(rnd.data()+off,5000), crc32_slice8(rnd.data()+off,5000), crc32_pclmul(rnd.data()+off,5000));
    }
    std::printf("%s: %d checks, %d failures\n", failures ? "FAIL" : "PASS", checks, failures);
    return failures ? 1 : 0;
}
