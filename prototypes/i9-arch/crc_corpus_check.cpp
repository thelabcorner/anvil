// arch I9 / dec-crc LEG 1: validates the ACTUAL src/anvil.cpp crc32 dispatcher
// (after the PCLMULQDQ integration) against an independent slicing-by-8
// reference over real corpus bytes, chunked exactly like the codec does
// (block_size defaults to 256 KiB; also tests 64 KiB and unaligned sub-spans).
//
// Build (repo root, after `. .\env.ps1`):
//   clang-cl /nologo /O2 /std:c++20 /EHsc /DANVIL_NO_MAIN prototypes\i9-arch\crc_corpus_check.cpp /Fe:prototypes\i9-arch\crc_corpus_check.exe
// Run: prototypes\i9-arch\crc_corpus_check.exe [corpus_dir]
#define ANVIL_NO_MAIN
#include "../../src/anvil.cpp"
#include <filesystem>
#include <cstdio>
#include <random>

// Independent reference: slicing-by-8 (copied from HEAD fc23d9a equivalent).
static uint32_t ref_crc32_slice8(const uint8_t* p, size_t n) {
    static std::array<std::array<uint32_t,256>,8> qtabs = []{
        std::array<std::array<uint32_t,256>,8> t{};
        for(uint32_t i=0;i<256;++i){ uint32_t c=i; for(int k=0;k<8;++k)c=(c&1)?(0xEDB88320u^(c>>1)):(c>>1); t[7][i]=c; }
        for(int s=0;s<8;++s){ int steps=7-s;
            for(uint32_t i=0;i<256;++i){ uint32_t c=t[7][i]; for(int r=0;r<steps;++r)c=(t[7][c&0xFFu])^(c>>8); t[s][i]=c; } }
        return t; }();
    uint32_t c=0xFFFFFFFFu;
    while(n>=8){
        uint32_t one=c^((uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24));
        uint32_t two=(uint32_t)p[4]|((uint32_t)p[5]<<8)|((uint32_t)p[6]<<16)|((uint32_t)p[7]<<24);
        c = qtabs[0][one&0xFFu]^qtabs[1][(one>>8)&0xFFu]^qtabs[2][(one>>16)&0xFFu]^qtabs[3][one>>24]
          ^ qtabs[4][two&0xFFu]^qtabs[5][(two>>8)&0xFFu]^qtabs[6][(two>>16)&0xFFu]^qtabs[7][two>>24];
        p+=8; n-=8;
    }
    for(size_t i=0;i<n;++i)c=qtabs[7][(c^p[i])&0xFFu]^(c>>8);
    return c^0xFFFFFFFFu;
}
// Bytewise reference (HEAD fc23d9a ground truth).
static uint32_t ref_crc32_bytewise(const uint8_t* p, size_t n) {
    static std::array<uint32_t,256> T=[]{ std::array<uint32_t,256> t{};
        for(uint32_t i=0;i<256;++i){uint32_t c=i;for(int k=0;k<8;++k)c=(c&1)?(0xEDB88320u^(c>>1)):(c>>1);t[i]=c;} return t; }();
    uint32_t c=0xFFFFFFFFu; for(size_t i=0;i<n;++i)c=T[(c^p[i])&0xFFu]^(c>>8); return c^0xFFFFFFFFu;
}

static uint64_t checks=0, fails=0;
static void cmp(const char* what, const char* file, size_t off, size_t len, const uint8_t* p) {
    uint32_t got = anvil::crc32(p, len);
    uint32_t ref = ref_crc32_slice8(p, len);
    uint32_t bz  = ref_crc32_bytewise(p, len);
    ++checks;
    if (got != ref || got != bz) { ++fails; std::printf("FAIL %s %s off=%zu len=%zu got=%08X slice8=%08X bytewise=%08X\n", what, file, off, len, got, ref, bz); }
}

int main(int argc, char** argv) {
    std::printf("dispatch: PCLMULQDQ=%s\n", anvil::crc32_has_pclmul() ? "YES" : "no (slice8 fallback)");
    const char* dir = argc>1 ? argv[1] : "tests/corpus";
    std::vector<std::filesystem::path> files;
    for (auto& e : std::filesystem::directory_iterator(dir))
        if (e.is_regular_file()) files.push_back(e.path());
    std::sort(files.begin(), files.end());
    // edge lengths on real bytes (first file prefix) + random
    std::mt19937 rng(0xC0FFEE);
    std::vector<uint8_t> rnd(1<<20); for(auto&x:rnd)x=(uint8_t)rng();
    for (size_t n : {0u,1u,2u,3u,7u,15u,31u,63u,64u,65u,127u,128u,129u,255u,256u,257u,4095u,4096u,65535u,65536u}) cmp("edge", "random", 0, n, rnd.data());
    for (size_t off=1; off<80; ++off) cmp("unaligned", "random", off, 65536+7, rnd.data()+off);

    for (auto& f : files) {
        auto d = anvil::read_file(f.string());
        if (d.empty()) continue;
        std::string name = f.filename().string();
        for (size_t bs : {(size_t)256*1024, (size_t)64*1024, (size_t)4096}) {
            for (size_t off=0; off<d.size(); off+=bs) {
                size_t n = std::min(bs, d.size()-off);
                cmp("chunk", name.c_str(), off, n, d.data()+off);
            }
        }
        // whole-file + hash-adjacent spans
        cmp("whole", name.c_str(), 0, d.size(), d.data());
        if (d.size()>3) { cmp("suffix", name.c_str(), d.size()-3, 3, d.data()+d.size()-3); cmp("prefix1", name.c_str(), 0, 1, d.data()); }
        std::printf("  %-28s %10zu B  ok\n", name.c_str(), d.size());
    }
    std::printf("%s: %llu checks, %llu failures\n", fails?"FAIL":"PASS", (unsigned long long)checks, (unsigned long long)fails);
    return fails?1:0;
}
