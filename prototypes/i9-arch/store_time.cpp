// arch I9 store-path microbench: in-process encode (store/raw block path) +
// decode timing for a CRC-path variant. Prints per-call median ns/B and the
// encoded container sha256 identity anchor.
// Build: clang-cl /nologo /O2 /std:c++20 /EHsc /DANVIL_NO_MAIN /DANVIL_HAVE_BROTLI=1 /DANVIL_HAVE_LIBSAIS=1 /Ithird_party\install\include /Ithird_party\libsais\include <wrapper>.cpp build-i9-arch\anvil_libsais.lib <brotli libs> /Fe:...
#ifndef ANVIL_SRC
#define ANVIL_SRC "../../src/anvil.cpp"
#endif
#define ANVIL_NO_MAIN
#include ANVIL_SRC
#include <cstdio>
#include <chrono>
#include <random>

static uint64_t fnv64(const std::vector<uint8_t>& d){ uint64_t h=1469598103934665603ull; for(auto b:d){h^=b;h*=1099511628211ull;} return h; }
static double med(std::vector<double> v,double& cv){
    std::sort(v.begin(),v.end()); double m=v[v.size()/2];
    double s=0; for(double x:v)s+=(x-m)*(x-m); s=std::sqrt(s/v.size());
    cv=m>0?100.0*s/m:0; return m;
}
int main(int argc,char**argv){
    if(argc<2){ std::printf("usage: store_time <file> [reps=5] [parse=mdl]\n"); return 2; }
    auto src=anvil::read_file(argv[1]);
    int reps=argc>2?std::atoi(argv[2]):5;
    std::string parse=argc>3?argv[3]:"mdl";
    anvil::Options o; o.parse=parse; o.literal="o0"; o.entropy="rans"; o.quiet=true;
    anvil::GlobalStats st; auto packed=anvil::compress(src,o,&st);
    std::vector<double> te,td;
    for(int r=0;r<reps;++r){
        auto a=std::chrono::steady_clock::now(); anvil::GlobalStats x; auto y=anvil::compress(src,o,&x); auto b=std::chrono::steady_clock::now();
        te.push_back(std::chrono::duration<double>(b-a).count());
        volatile size_t sink=y.size(); (void)sink;
    }
    for(int r=0;r<reps;++r){
        auto a=std::chrono::steady_clock::now(); auto y=anvil::decompress(packed,o); auto b=std::chrono::steady_clock::now();
        td.push_back(std::chrono::duration<double>(b-a).count());
        volatile size_t sink=y.size(); (void)sink;
    }
    double cve=0,cvd=0; double me=med(te,cve), md=med(td,cvd);
    std::printf("file=%s bytes=%zu wire=%zu wire_fnv=%016llX encode_ns_per_inB=%.4f encode_MBps=%.1f CV=%.2f%% decode_ns_per_outB=%.4f decode_MBps=%.1f CV=%.2f%% encode_blocks=%llu raw_blocks=%llu\n",
        argv[1], src.size(), packed.size(), (unsigned long long)fnv64(packed),
        me*1e9/src.size(), src.size()/1e6/me, cve,
        md*1e9/(src.size()?src.size():1), src.size()/1e6/md, cvd,
        (unsigned long long)st.blocks,(unsigned long long)st.raw_blocks);
    return 0;
}
