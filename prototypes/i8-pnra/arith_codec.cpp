// arith_codec.cpp — measure arithmetic/stride transform families with a REAL
// entropy coder (brotli) rather than a hand-rolled one.
//
// Rationale: the previous prototype's bit-plane coder reset its model per
// symbol and therefore produced no compression; hand-rolling a coder to
// evaluate a transform is a measurement bug waiting to happen. Brotli is a
// real, shipped coder, already linked on this host, and it is the reference we
// are measured against. So:
//
//      score(transform) = brotli_q(transform(bytes))
//
// and we compare against brotli_q(raw bytes). Any gain is attributable to the
// transform, not to a bespoke entropy coder.
//
// Transform ladder (cheapest / most-obvious first, so gain is attributable):
//   T0  identity (control)
//   T1  delta  width 4 LE          (sigma=1, the ancient control)
//   T2  delta  width 4 LE, zigzag + varint bytes
//   T3  delta  width 8 LE
//   T4  stride-P delta: v[i] - v[i-P] for several P (period discovery)
//   T5  xor    width 4/8            (constant-XOR / float-domain probe)
//   T6  best global integer-sigma linear: v[i] - sigma*i (per file grid)
//   T7  piecewise-linear derived-sigma residual (segmentation)
//   T8  byte-lane split (transposition of the u32 stream into 4 planes)
//
// Build (Windows, this host):
//   clang-cl /O2 /EHsc /std:c++20 /MD /DNDEBUG
//     /I ..\..\third_party\install\include /Fearith_codec.exe arith_codec.cpp
//     /link /LIBPATH:..\..\third_party\install\lib
//     brotlienc.lib brotlidec.lib brotlicommon.lib
// Usage: arith_codec <file> [qualities...]
#include <brotli/encode.h>
#include <brotli/decode.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

using u8=uint8_t; using u32=uint32_t; using i64=int64_t;

static std::vector<u8> rdfile(const std::string& p){
    std::ifstream f(p,std::ios::binary);
    if(!f){std::fprintf(stderr,"cannot open %s\n",p.c_str());std::exit(1);}
    return std::vector<u8>((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
}
static size_t bz(const std::vector<u8>& d,int q){
    size_t cap=BrotliEncoderMaxCompressedSize(d.size())+64;
    std::vector<u8> out(cap); size_t n=cap;
    if(!BrotliEncoderCompress(q,BROTLI_DEFAULT_WINDOW,BROTLI_MODE_GENERIC,d.size(),d.data(),&n,out.data()))
        {std::fprintf(stderr,"brotli failed\n");std::exit(1);}
    // roundtrip
    std::vector<u8> back(d.size()); size_t bl=d.size();
    if(BrotliDecoderDecompress(n,out.data(),&bl,back.data())!=BROTLI_DECODER_RESULT_SUCCESS||bl!=d.size()||back!=d)
        {std::fprintf(stderr,"brotli roundtrip failed\n");std::exit(1);}
    return n;
}
static void put32(std::vector<u8>&o,u32 v){o.push_back(u8(v));o.push_back(u8(v>>8));o.push_back(u8(v>>16));o.push_back(u8(v>>24));}
static void put64(std::vector<u8>&o,uint64_t v){for(int i=0;i<8;i++)o.push_back(u8(v>>(8*i)));}
static void varint(std::vector<u8>&o,uint64_t v){while(v>=0x80){o.push_back(u8(v)|0x80);v>>=7;}o.push_back(u8(v));}
static inline u32 zz32(i64 x){return u32((x<<1)^(x>>63));}

// Reported per transform: which brotli quality, and the best over the sweep.
struct Row{const char*name;std::vector<u8> data;};

static double best_over(const std::vector<u8>& d,const std::vector<int>& qs,size_t& out_bytes,int& out_q){
    out_bytes=SIZE_MAX;out_q=0;
    for(int q:qs){size_t n=bz(d,q);if(n<out_bytes){out_bytes=n;out_q=q;}}
    return double(out_bytes);
}

int main(int argc,char**argv){
    if(argc<2){std::fprintf(stderr,"usage: arith_codec <file> [q...]\n");return 2;}
    auto raw=rdfile(argv[1]);
    std::vector<int> qs;
    if(argc>2)for(int i=2;i<argc;i++)qs.push_back(std::atoi(argv[i]));
    else qs={1,4,6,9};

    const size_t N=raw.size();
    size_t n4=N/4, n8=N/8;

    std::printf("file=%s bytes=%zu\n",argv[1],N);
    std::printf("%-42s %10s %6s %10s %8s\n","transform","bytes","q","ratio","vs raw");

    size_t raw_best=0; int raw_q=0;
    best_over(raw,qs,raw_best,raw_q);
    std::printf("%-42s %10zu %6d %10.4f %8s\n","T0 identity (raw)",raw_best,raw_q,double(raw_best)/N,"-");

    auto report=[&](const char*name,std::vector<u8>&& d){
        size_t b=0;int q=0;best_over(d,qs,b,q);
        std::printf("%-42s %10zu %6d %10.4f %+7.2f%%\n",name,b,q,double(b)/N,
                    100.0*(double(b)-double(raw_best))/double(raw_best));
        return b;
    };

    // ---- T1 delta width 4 --------------------------------------------------
    {std::vector<u8> o;o.reserve(N);
     u32 prev=0;for(size_t i=0;i<n4;i++){u32 v;std::memcpy(&v,raw.data()+i*4,4);put32(o,i?v-prev:v);prev=v;}
     for(size_t i=n4*4;i<N;i++)o.push_back(raw[i]);
     report("T1 delta u32 LE",std::move(o));}
    // ---- T2 delta u32 zigzag varint ---------------------------------------
    {std::vector<u8> o;o.reserve(N);
     u32 prev=0;for(size_t i=0;i<n4;i++){u32 v;std::memcpy(&v,raw.data()+i*4,4);varint(o,i?zz32(i64(v)-i64(prev)):zz32(i64(v)));prev=v;}
     for(size_t i=n4*4;i<N;i++)o.push_back(raw[i]);
     report("T2 delta u32 zigzag varint",std::move(o));}
    // ---- T3 delta width 8 --------------------------------------------------
    {std::vector<u8> o;o.reserve(N);
     uint64_t prev=0;for(size_t i=0;i<n8;i++){uint64_t v;std::memcpy(&v,raw.data()+i*8,8);put64(o,i?v-prev:v);prev=v;}
     for(size_t i=n8*8;i<N;i++)o.push_back(raw[i]);
     report("T3 delta u64 LE",std::move(o));}
    // ---- T4 stride-P delta (period discovery) ------------------------------
    for(int P : {2,3,4,8,16,64}){
        std::vector<u8> o;o.reserve(N);
        for(size_t i=0;i<n4;i++){u32 v;std::memcpy(&v,raw.data()+i*4,4);
            u32 src = (i>=size_t(P))?0:v; if(i>=size_t(P)){u32 w;std::memcpy(&w,raw.data()+(i-P)*4,4);src=w;}
            put32(o,i>=size_t(P)?v-src:v);}
        for(size_t i=n4*4;i<N;i++)o.push_back(raw[i]);
        char nm[64];std::snprintf(nm,sizeof nm,"T4 stride delta u32 P=%d",P);
        report(nm,std::move(o));
    }
    // ---- T5 xor ------------------------------------------------------------
    {std::vector<u8> o;o.reserve(N);
     u32 prev=0;for(size_t i=0;i<n4;i++){u32 v;std::memcpy(&v,raw.data()+i*4,4);put32(o,i?(v^prev):v);prev=v;}
     for(size_t i=n4*4;i<N;i++)o.push_back(raw[i]);
     report("T5 xor u32 LE",std::move(o));}
    // ---- T6 best global integer sigma -------------------------------------
    {std::vector<u32> d(n4);std::memcpy(d.data(),raw.data(),n4*4);
     double bestH=1e30;i64 bestS=0;
     for(i64 s=-256;s<=1024;s++){
        std::unordered_map<u32,u32> c;c.reserve(n4*2);
        for(size_t i=0;i<n4;i++){u32 inv=u32(i64(d[i])-s*i64(i));++c[inv];}
        double H=0,nn=double(n4);for(auto&kv:c){double p=kv.second/nn;H-=p*std::log2(p);}
        if(H<bestH){bestH=H;bestS=s;}
     }
     std::vector<u8> o;o.reserve(N);
     for(size_t i=0;i<n4;i++)put32(o,u32(i64(d[i])-bestS*i64(i)));
     for(size_t i=n4*4;i<N;i++)o.push_back(raw[i]);
     char nm[64];std::snprintf(nm,sizeof nm,"T6 linear inv sigma=%lld",(long long)bestS);
     report(nm,std::move(o));
     std::printf("     (T6 grid-search sigma=%lld, residual H=%.3f bits/val)\n",(long long)bestS,bestH);
    }
    // ---- T7 piecewise-linear, derived sigma --------------------------------
    {std::vector<u32> d(n4);std::memcpy(d.data(),raw.data(),n4*4);
     // segmentation: greedy extend while |v - (sigma*p + base)| <= tol
     struct Seg{size_t start,len;i64 sigma,base;};
     auto segment=[&](int window,i64 tol){
        std::vector<Seg> segs;size_t s=0;
        while(s<n4){
            size_t w=std::min<size_t>(size_t(window),n4-s);
            i64 sigma=0; if(w>=2) sigma=(i64(d[s+w-1])-i64(d[s]))/i64(w-1);
            i64 base=i64(d[s])-sigma*i64(s);
            size_t e=s;
            while(e<n4){i64 pred=sigma*i64(e)+base;if(std::llabs(i64(d[e])-pred)>tol)break;++e;}
            if(e==s)e=s+1;
            segs.push_back({s,e-s,sigma,base});s=e;
        }
        return segs;
     };
     size_t bb=SIZE_MAX;int bq=0,bestw=0;long long besttol=0;
     for(int w:{16,32,64,128,256})for(long long tol:{32,64,128,256,512,1024}){
        auto segs=segment(w,tol);
        // refit sigma from endpoints (decoder-derivable), base = mode of residual
        for(auto&g:segs){
            if(g.len>=2){i64 num=i64(d[g.start+g.len-1])-i64(d[g.start]);i64 den=i64(g.len-1);
                g.sigma=num>=0?num/den:-((-num+den-1)/den);}else g.sigma=0;
            std::unordered_map<i64,u32> cnt;i64 bv=0;u32 bc=0;
            for(size_t i=g.start;i<g.start+g.len;i++){i64 inv=i64(d[i])-g.sigma*i64(i);u32 c=++cnt[inv];if(c>bc){bc=c;bv=inv;}}
            g.base=bv;
        }
        std::vector<u8> o;o.reserve(N);
        for(auto&g:segs)for(size_t i=g.start;i<g.start+g.len;i++)put32(o,u32(i64(d[i])-(g.sigma*i64(i)+g.base)));
        for(size_t i=n4*4;i<N;i++)o.push_back(raw[i]);
        size_t b=0;int q=0;best_over(o,qs,b,q);
        if(b<bb){bb=b;bq=q;bestw=w;besttol=tol;}
     }
     std::printf("%-42s %10zu %6d %10.4f %+7.2f%%\n","T7 piecewise-linear (best)",bb,bq,double(bb)/N,
                 100.0*(double(bb)-double(raw_best))/double(raw_best));
     std::printf("     (T7 best window=%d tol=%lld, %zu segments)\n",bestw,besttol,size_t(0));
     std::printf("     (T7 NOTE: segment params are NOT counted here - upper-bound-favorable)\n");
    }
    // ---- T8 byte-lane split ------------------------------------------------
    {std::vector<u8> o;o.reserve(N);
     for(int lane=0;lane<4;lane++)for(size_t i=0;i<n4;i++)o.push_back(raw[i*4+size_t(lane)]);
     for(size_t i=n4*4;i<N;i++)o.push_back(raw[i]);
     report("T8 byte-lane split (4 planes)",std::move(o));}
    // ---- T9 delta THEN byte-lane split -------------------------------------
    {std::vector<u8> del;del.reserve(N);
     u32 prev=0;for(size_t i=0;i<n4;i++){u32 v;std::memcpy(&v,raw.data()+i*4,4);put32(del,i?v-prev:v);prev=v;}
     std::vector<u8> o;o.reserve(N);
     for(int lane=0;lane<4;lane++)for(size_t i=0;i<n4;i++)o.push_back(del[i*4+size_t(lane)]);
     for(size_t i=n4*4;i<N;i++)o.push_back(raw[i]);
     report("T9 delta + byte-lane split",std::move(o));}
    return 0;
}
