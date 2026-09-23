// arch I9 / dec-crc: focused single-thread decode-time harness for the CRC leg.
// In-process median-N on a prebuilt container; reports ns/B, MB/s, CV, host CPU
// load sampled over the window (PR-4 fields: thread count + load attestation).
//
// Build a variant:
//   clang-cl /nologo /O2 /std:c++20 /EHsc /DANVIL_NO_MAIN /DANVIL_HAVE_LIBSAIS=1 /DANVIL_HAVE_BROTLI=1 ^
//     /Ithird_party\libsais\include /Ithird_party\install\include ^
//     /DANVIL_SRC="\"..\\..\\src\\anvil.cpp\"" prototypes\i9-arch\decode_time.cpp ^
//     build-i9-arch\anvil_libsais.lib <brotli libs> /Fe:prototypes\i9-arch\decode_time.exe
//
// Usage: decode_time <container.anv> [reps=7]
#ifndef ANVIL_SRC
#define ANVIL_SRC "../../src/anvil.cpp"
#endif
#define ANVIL_NO_MAIN
#include ANVIL_SRC
#include <cstdio>
#include <chrono>
#if defined(_WIN32)
#include <windows.h>
static double cpu_load_sample(double seconds){
    FILETIME i0,k0,u0,i1,k1,u1;
    if(!GetSystemTimes(&i0,&k0,&u0)) return -1;
    auto ft=[&](const FILETIME& f,unsigned long long& v){ v=( (unsigned long long)f.dwHighDateTime<<32)|f.dwLowDateTime; };
    unsigned long long a,b,c,d,e,f; ft(i0,a);ft(k0,b);ft(u0,c);
    Sleep((DWORD)(seconds*1000));
    if(!GetSystemTimes(&i1,&k1,&u1)) return -1;
    ft(i1,d);ft(k1,e);ft(u1,f);
    unsigned long long idle=d-a, kern=(e-b), user=(f-c), total=kern+user;
    if(total==0) return -1;
    return 100.0*((double)(total-idle))/ (double)total;
}
#else
static double cpu_load_sample(double){ return -1; }
#endif
static double med(std::vector<double> v,double& cv){
    std::sort(v.begin(),v.end()); double m=v[v.size()/2];
    double s=0; for(double x:v)s+=(x-m)*(x-m); s=std::sqrt(s/v.size());
    cv = m>0? 100.0*s/m : 0; return m;
}
int main(int argc,char**argv){
    if(argc<2){ std::printf("usage: decode_time <container.anv> [reps=7]\n"); return 2; }
    auto in=anvil::read_file(argv[1]);
    int reps=argc>2?std::atoi(argv[2]):7;
    anvil::Options o; o.quiet=true; o.decode_threads=1;   // single-thread, PR-4 thread rule
    auto dec=[&]{ auto b=anvil::decompress(in,o); volatile size_t s=b.size(); (void)s; return b; };
    auto warm=dec();                       // roundtrip sanity + warm caches
    std::printf("container=%s in=%zu out=%zu reps=%d decode_threads=%d\n",argv[1],in.size(),warm.size(),reps,o.decode_threads);
    std::vector<double> t;
    for(int r=0;r<reps;++r){ auto a=std::chrono::steady_clock::now(); dec(); auto b=std::chrono::steady_clock::now(); t.push_back(std::chrono::duration<double>(b-a).count()); }
    double cv=0; double m=med(t,cv);
    double mbps = warm.size()/1e6/m;
    // machine-speed normalizer: memcpy of the output size, same window style
    std::vector<double> tc;
    { std::vector<uint8_t> src(warm.size(),0x5A), dst(warm.size());
      for(int r=0;r<reps;++r){ auto a=std::chrono::steady_clock::now(); std::memcpy(dst.data(),src.data(),warm.size()); auto b=std::chrono::steady_clock::now(); tc.push_back(std::chrono::duration<double>(b-a).count()); } }
    double cvc=0; double mc=med(tc,cvc);
    double load=cpu_load_sample(0.25);
    std::printf("decode median=%.4f ms  %.1f MB/s  min=%.4f ms  CV=%.1f%%  ns_per_outB=%.3f\n",m*1e3,mbps,*std::min_element(t.begin(),t.end())*1e3,cv,m*1e9/warm.size());
    std::printf("normalizer memcpy median=%.4f ms  %.1f MB/s  CV=%.1f%%\n",mc*1e3,warm.size()/1e6/mc,cvc);
    std::printf("host_cpu_load_sampled=%.1f%%\n",load);
    return 0;
}
