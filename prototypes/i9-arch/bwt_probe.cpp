// arch I9 probe v2: BWT backend roundtrip + primary-index diagnostics.
// Build: clang-cl /nologo /O2 /std:c++20 /EHsc /DANVIL_NO_MAIN /DANVIL_HAVE_LIBSAIS=1 /Ithird_party\libsais\include prototypes\i9-arch\bwt_probe.cpp build-i9-arch\anvil_libsais.lib /Fe:prototypes\i9-arch\bwt_probe.exe
#define ANVIL_NO_MAIN
#include "../../src/anvil.cpp"
#include <cstdio>
int main(int argc,char**argv){
    if(argc<2){ std::printf("usage: bwt_probe <file> [post...]\n"); return 2; }
    auto x=anvil::read_file(argv[1]);
    int32_t n=(int32_t)x.size();
    std::printf("input=%s bytes=%zu\n",argv[1],x.size());
    { int32_t distinct[256]={0}; for(uint8_t b:x) distinct[b]=1; int nd=0; for(int i=0;i<256;++i) nd+=distinct[i];
      std::printf("  distinct_bytes=%d\n",nd); }
    // Direct libsais roundtrip + primary semantics.
    {
        std::vector<uint8_t> b(n); std::vector<int32_t> a(n?n:1);
        int32_t p=libsais_bwt(x.data(),b.data(),a.data(),n,0,nullptr);
        std::vector<uint8_t> o1(n),o2(n),o3(n); std::vector<int32_t> t1(n+1),t2(n+1),t3(n+1);
        int32_t r1=libsais_unbwt(b.data(),o1.data(),t1.data(),n,nullptr,p==n?1:p);
        int32_t r2=libsais_unbwt(b.data(),o2.data(),t2.data(),n,nullptr,p);
        int32_t r3=libsais_unbwt(b.data(),o3.data(),t3.data(),n,nullptr,p==0?1:p);
        std::printf("  libsais_bwt primary=%d  unbwt(p==n?1:p)=%d match=%d | unbwt(p)=%d match=%d | unbwt(p==0?1:p)=%d match=%d\n",
            p,r1,(o1==x),r2,(o2==x),r3,(o3==x));
    }
    std::vector<int> posts;
    if(argc>2){ for(int i=2;i<argc;++i) posts.push_back(std::atoi(argv[i])); }
    else posts={-1,1,2,3};
    for(int post:posts){
        anvil::Options o; o.bwt_post=post; o.bwt_subblock=128u<<20;
        try{
            auto bw=anvil::bwt_backend_encode(x,o);
            uint8_t chosen = bw.empty()?0:bw[0];
            const uint8_t* q=bw.data(); const uint8_t* qe=bw.data()+bw.size();
            ++q; uint64_t pv=anvil::get_uvar(q,qe);
            try{
                auto y=anvil::bwt_backend_decode(bw.data(),bw.size(),x.size());
                bool ok=(y==x);
                std::printf("force=%-2d payload=%-8zu chosen=%u stored_primary=%llu roundtrip=%s\n",post,bw.size(),chosen,(unsigned long long)pv,ok?"OK":"MISMATCH");
                if(!ok){ size_t i=0; while(i<x.size()&&i<y.size()&&x[i]==y[i])++i; std::printf("  first_diff=%zu x=%02X y=%02X\n",i,i<x.size()?x[i]:0,i<y.size()?y[i]:0); }
            }catch(const std::exception&e){ std::printf("force=%-2d payload=%-8zu chosen=%u stored_primary=%llu DECODE_THROW %s\n",post,bw.size(),chosen,(unsigned long long)pv,e.what()); }
        }catch(const std::exception&e){ std::printf("force=%-2d ENCODE_THROW %s\n",post,e.what()); }
    }
    return 0;
}
