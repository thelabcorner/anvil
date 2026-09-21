// bwt_lab.cpp — standalone BWT postcoder byte-accounting + EV pre-computation.
// bwt-theory, ANVIL-BLOCKSPLIT-EXP. LANE: prototypes/ (analysis only, no codec edits).
//
// Goal: decompose the BWT payload into (a) MTF rank coding, (b) zero-run coding,
// (c) run-length varint bytes, (d) header/model overhead; compute entropy floors
// (o0/o1/o2) for the MTF-token stream and the raw BWT stream to bound QLFC/LZP EV;
// measure LZP coverage. Byte-exact replication of anvil postcoder-0/-1/-2/-3 paths
// so the decomposition is trustworthy against build/anvil.exe.
//
// Build: clang-cl /O2 /std:c++20 /I third_party/libsais/include bwt_lab.cpp third_party/libsais/src/libsais.c /Fe:bwt_lab.exe
//
// Usage: bwt_lab.exe <file> [--block=N] [--lzp-minlen=M]
//   block N = process in N-byte blocks (router-regret handoff); 0 = whole file.

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

// ---- libsais ----
extern "C" {
#include "libsais.h"
}

// ============================================================================
// Verbatim copies of anvil primitives (src/anvil.cpp) — used for byte-exactness.
// ============================================================================
static constexpr uint32_t kHalf = 0x80000000u;
static constexpr uint32_t kFirstQtr = 0x40000000u;
static constexpr uint32_t kThirdQtr = 0xC0000000u;
static constexpr uint32_t kModelRescale = 32768;

struct BitWriter {
    std::vector<uint8_t> out;
    uint8_t cur = 0; uint8_t used = 0; uint64_t count = 0;
    void bit(uint32_t b){ ++count; cur=(uint8_t)((cur<<1)|(b&1)); if(++used==8){out.push_back(cur);cur=0;used=0;} }
    void finish(){ if(used){ cur<<=(uint8_t)(8-used); out.push_back(cur); cur=0; used=0; } }
    uint64_t bit_count() const { return count; }
};
struct BitReader {
    const uint8_t* p; size_t n; size_t byte=0; uint8_t bitpos=0;
    BitReader(const uint8_t* pp,size_t nn):p(pp),n(nn){}
    uint32_t bit(){ if(byte>=n) return 0; uint32_t v=(p[byte]>>(7-bitpos))&1u; if(++bitpos==8){bitpos=0;++byte;} return v; }
};

class ArithmeticEncoder {
    BitWriter bw_; uint32_t low_=0, high_=0xFFFFFFFFu; uint64_t pending_=0;
    void emit_plus_pending(uint32_t b){ bw_.bit(b); while(pending_){bw_.bit(b^1u);--pending_;} }
public:
    void encode(uint32_t lo,uint32_t hi,uint32_t total){
        if(!(lo<hi&&hi<=total&&total>0)) throw std::runtime_error("bad interval");
        uint64_t range=(uint64_t)high_-low_+1;
        high_=low_+(uint32_t)((range*hi)/total-1);
        low_ =low_+(uint32_t)((range*lo)/total);
        for(;;){ if(high_<kHalf) emit_plus_pending(0);
            else if(low_>=kHalf){ emit_plus_pending(1); low_-=kHalf; high_-=kHalf; }
            else if(low_>=kFirstQtr&&high_<kThirdQtr){++pending_; low_-=kFirstQtr; high_-=kFirstQtr;}
            else break; low_<<=1; high_=(high_<<1)|1u; }
    }
    std::vector<uint8_t> finish(){ ++pending_; if(low_<kFirstQtr) emit_plus_pending(0); else emit_plus_pending(1); bw_.finish(); return std::move(bw_.out); }
    uint64_t bit_count() const { return bw_.bit_count(); }
};
class ArithmeticDecoder {
    BitReader br_; uint32_t low_=0,high_=0xFFFFFFFFu,code_=0;
public:
    ArithmeticDecoder(const uint8_t* p,size_t n):br_(p,n){ for(int i=0;i<32;++i) code_=(code_<<1)|br_.bit(); }
    uint32_t scaled(uint32_t total) const { uint64_t range=(uint64_t)high_-low_+1; return (uint32_t)(((uint64_t)(code_-low_+1)*total-1)/range); }
    void consume(uint32_t lo,uint32_t hi,uint32_t total){
        uint64_t range=(uint64_t)high_-low_+1;
        high_=low_+(uint32_t)((range*hi)/total-1);
        low_ =low_+(uint32_t)((range*lo)/total);
        for(;;){ if(high_<kHalf){} else if(low_>=kHalf){code_-=kHalf;low_-=kHalf;high_-=kHalf;}
            else if(low_>=kFirstQtr&&high_<kThirdQtr){code_-=kFirstQtr;low_-=kFirstQtr;high_-=kThirdQtr;}
            else break; low_<<=1; high_=(high_<<1)|1u; code_=(code_<<1)|br_.bit(); }
    }
};

class AdaptiveModel {
    uint32_t alphabet_; std::vector<uint16_t> freq_; std::vector<uint32_t> tree_; uint32_t total_=0;
    void add_tree(uint32_t idx,uint32_t delta){ for(uint32_t i=idx+1;i<=alphabet_;i+=i&-i) tree_[i]+=delta; }
    void rebuild(){ std::fill(tree_.begin(),tree_.end(),0); total_=0; for(uint32_t i=0;i<alphabet_;++i){total_+=freq_[i];add_tree(i,freq_[i]);} }
    uint32_t prefix(uint32_t s) const { uint32_t r=0; for(uint32_t i=s;i;i-=i&-i) r+=tree_[i]; return r; }
    void update(uint32_t s){ if(total_>=kModelRescale){ for(auto& f:freq_) f=(uint16_t)std::max<uint16_t>(1,(f+1)>>1); rebuild(); } ++freq_[s]; ++total_; add_tree(s,1); }
public:
    explicit AdaptiveModel(uint32_t a=256):alphabet_(a),freq_(a,1),tree_(a+1,0){rebuild();}
    void encode(ArithmeticEncoder& ac,uint32_t s){ uint32_t lo=prefix(s),hi=lo+freq_[s]; ac.encode(lo,hi,total_); update(s); }
    uint32_t decode(ArithmeticDecoder& ad){ uint32_t target=ad.scaled(total_); uint32_t idx=0,sum=0; uint32_t bit=1u<<(31-std::countl_zero(alphabet_));
        for(;bit;bit>>=1){ uint32_t nx=idx+bit; if(nx<=alphabet_&&sum+tree_[nx]<=target){idx=nx;sum+=tree_[nx];} }
        if(idx>=alphabet_) throw std::runtime_error("sym oor"); uint32_t s=idx; uint32_t lo=sum,hi=lo+freq_[s]; ad.consume(lo,hi,total_); update(s); return s; }
};

static void put_uvar(std::vector<uint8_t>& out,uint64_t x){ do{ uint8_t b=(uint8_t)(x&0x7f); x>>=7; if(x)b|=0x80; out.push_back(b);}while(x); }
static void append_varint_bytes(std::vector<uint8_t>& out,uint64_t x){ do{ uint8_t b=(uint8_t)(x&0x7f); x>>=7; if(x)b|=0x80; out.push_back(b);}while(x); }
static uint64_t read_varint_bytes(const std::vector<uint8_t>& v,size_t& pos){ uint64_t x=0; int sh=0; for(int i=0;i<10;++i){ if(pos>=v.size()) throw std::runtime_error("varint trunc"); uint8_t b=v[pos++]; x|=(uint64_t)(b&0x7f)<<sh; if(!(b&0x80)) return x; sh+=7; } throw std::runtime_error("varint of"); }

// ---- stream suite (encode_stream_smallest only) ----
struct RansSpec { uint32_t scale_bits, tot, L; };
static constexpr RansSpec kRans4096{12,1u<<12,1u<<23};
static constexpr RansSpec kRans512 {9, 1u<<9, 1u<<17};
static constexpr RansSpec kRans256 {8, 1u<<8, 1u<<16};
struct RansModel { std::array<uint16_t,256> freq{},start{}; };

static RansModel build_rans_model(const std::vector<uint8_t>& src,uint32_t tot){
    RansModel m; if(src.empty()) return m;
    std::array<uint32_t,256> count{}; for(uint8_t b:src)++count[b];
    std::array<double,256> exact{}; uint32_t sum=0;
    for(int i=0;i<256;++i) if(count[i]){ exact[i]=double(count[i])*tot/src.size(); uint32_t f=(uint32_t)std::max<uint32_t>(1,(uint32_t)std::floor(exact[i])); m.freq[i]=(uint16_t)f; sum+=f; }
    while(sum<tot){ double best=-1e100; int bi=-1; for(int i=0;i<256;++i) if(count[i]){ double sc=exact[i]-m.freq[i]; if(sc>best){best=sc;bi=i;} } if(bi<0) throw std::runtime_error("rU"); ++m.freq[bi]; ++sum; }
    while(sum>tot){ double best=-1e100; int bi=-1; for(int i=0;i<256;++i) if(m.freq[i]>1){ double sc=m.freq[i]-exact[i]; if(sc>best){best=sc;bi=i;} } if(bi<0) throw std::runtime_error("rO"); --m.freq[bi]; --sum; }
    uint32_t st=0; for(int i=0;i<256;++i){ m.start[i]=(uint16_t)st; st+=m.freq[i]; }
    if(st!=tot) throw std::runtime_error("rS");
    return m;
}
static std::vector<uint8_t> rans_encode(const std::vector<uint8_t>& src,const RansModel& m,const RansSpec& sp){
    if(src.empty()) return {};
    uint32_t x=sp.L; std::vector<uint8_t> em; em.reserve(src.size()/2+16);
    for(size_t ii=src.size();ii-->0;){ uint8_t s=src[ii]; uint32_t f=m.freq[s],st=m.start[s]; uint32_t xmax=((sp.L>>sp.scale_bits)<<8)*f;
        while(x>=xmax){ em.push_back((uint8_t)x); x>>=8; } x=((x/f)<<sp.scale_bits)+(x%f)+st; }
    std::vector<uint8_t> out(4); out[0]=(uint8_t)x; out[1]=(uint8_t)(x>>8); out[2]=(uint8_t)(x>>16); out[3]=(uint8_t)(x>>24);
    out.reserve(4+em.size()); for(auto it=em.rbegin();it!=em.rend();++it) out.push_back(*it); return out;
}
static std::vector<uint8_t> rans_stream_bytes(const std::vector<uint8_t>& src,const RansSpec& sp,uint8_t mode){
    RansModel m=build_rans_model(src,sp.tot); auto rd=rans_encode(src,m,sp);
    std::vector<uint8_t> z; z.push_back(mode); put_uvar(z,src.size());
    uint32_t nz=0; for(auto f:m.freq) if(f) ++nz; put_uvar(z,nz);
    for(int i=0;i<256;++i) if(m.freq[i]){ z.push_back((uint8_t)i); put_uvar(z,m.freq[i]); }
    put_uvar(z,rd.size()); z.insert(z.end(),rd.begin(),rd.end()); return z;
}
static std::array<uint8_t,256> huffman_lengths(const std::vector<uint8_t>& src){
    std::array<uint32_t,256> cnt{}; for(uint8_t b:src) ++cnt[b];
    std::array<uint8_t,256> len{}; uint32_t alive=0; for(int i=0;i<256;++i) if(cnt[i]) ++alive;
    if(alive==0) return len; if(alive==1){ for(int i=0;i<256;++i) if(cnt[i]) len[i]=1; return len; }
    struct Node{int freq,l,r,sym;}; std::vector<Node> nodes; nodes.reserve(2*alive);
    for(int i=0;i<256;++i) if(cnt[i]) nodes.push_back({(int)cnt[i],-1,-1,i});
    std::vector<std::pair<int,int>> heap; for(size_t i=0;i<nodes.size();++i) heap.push_back({(int)nodes[i].freq,(int)i});
    auto lt=[](auto&a,auto&b){return a.first>b.first;}; std::make_heap(heap.begin(),heap.end(),lt);
    while(heap.size()>1){ std::pop_heap(heap.begin(),heap.end(),lt); auto a=heap.back(); heap.pop_back();
        std::pop_heap(heap.begin(),heap.end(),lt); auto b=heap.back(); heap.pop_back();
        int nn=(int)nodes.size(); nodes.push_back({a.first+b.first,a.second,b.second,-1}); heap.push_back({nodes[nn].freq,nn}); std::push_heap(heap.begin(),heap.end(),lt); }
    std::function<void(int,int)> walk=[&](int nd,int d){ if(nodes[nd].sym>=0){len[nodes[nd].sym]=(uint8_t)d;return;} walk(nodes[nd].l,d+1); walk(nodes[nd].r,d+1); };
    walk(heap[0].second,0); return len;
}
static std::vector<uint8_t> huffman_stream_bytes(const std::vector<uint8_t>& src,const std::array<uint8_t,256>& len){
    std::vector<uint8_t> z; z.push_back(4); put_uvar(z,src.size()); for(int i=0;i<256;++i) z.push_back(len[i]);
    // encode bits
    std::array<uint8_t,256> order{}; size_t cnt=0; for(int s=0;s<256;++s) if(len[s]) order[cnt++]=s;
    std::sort(order.begin(),order.begin()+cnt,[&](uint8_t a,uint8_t b){return len[a]!=len[b]?len[a]<len[b]:a<b;});
    std::array<uint32_t,256> code{}; uint32_t c=0,clen=0;
    for(size_t i=0;i<cnt;++i){ while(clen<len[order[i]]){c<<=1;++clen;} code[order[i]]=c++; }
    std::vector<uint8_t> out; out.reserve(src.size()+16); uint64_t acc=0; int nb=0;
    for(uint8_t s:src){ uint32_t cd=code[s]; int l=len[s]; for(int b=l-1;b>=0;--b){ acc=(acc<<1)|((cd>>b)&1u); if(++nb==64){ for(int k=7;k>=0;--k) out.push_back((uint8_t)(acc>>(8*k))); nb=0; acc=0; } } }
    if(nb){ acc<<=(64-nb); int bytes=(nb+7)/8; for(int k=0;k<bytes;++k) out.push_back((uint8_t)(acc>>(64-8*(k+1)))); }
    put_uvar(z,out.size()); z.insert(z.end(),out.begin(),out.end()); return z;
}
static std::vector<uint8_t> defexc_stream_bytes(const std::vector<uint8_t>& src,uint8_t def){
    std::vector<uint8_t> z; z.push_back(5); put_uvar(z,src.size()); z.push_back(def);
    std::vector<uint8_t> mask((src.size()+7)/8,0); std::vector<uint8_t> vals;
    for(size_t i=0;i<src.size();++i) if(src[i]!=def){ mask[i>>3]|=uint8_t(1u<<(i&7)); vals.push_back(src[i]); }
    put_uvar(z,vals.size()); z.insert(z.end(),mask.begin(),mask.end()); z.insert(z.end(),vals.begin(),vals.end()); return z;
}
// ctx model (Lloyd K=12) — verbatim from anvil (needed for encode_stream_smallest path)
struct CtxModel { uint8_t K=0; std::array<uint8_t,256> map{}; std::array<RansModel,12> m{}; };
static constexpr uint32_t kCtxK=12;
static CtxModel build_ctx_model(const std::vector<uint8_t>& src,uint32_t tot){
    CtxModel cm; cm.K=kCtxK;
    std::array<std::array<uint32_t,256>,256> cnt{}; std::array<uint32_t,256> ptot{};
    uint8_t prev=0; for(uint8_t b:src){ ++cnt[prev][b]; ++ptot[prev]; prev=b; }
    std::array<uint8_t,256> group{}; std::vector<uint32_t> seeds;
    { std::vector<std::pair<uint32_t,uint8_t>> order; for(int p=0;p<256;++p) if(ptot[p]) order.push_back({ptot[p],(uint8_t)p});
      std::sort(order.rbegin(),order.rend()); for(size_t i=0;i<order.size()&&seeds.size()<kCtxK;++i) seeds.push_back(order[i].second);
      for(int p=0;p<256;++p) group[p]=0; }
    if(seeds.empty()) return cm;
    std::array<std::array<double,256>,kCtxK> centroid{}; std::array<double,kCtxK> cnorm{};
    for(uint8_t g=0;g<kCtxK&&g<seeds.size();++g){ double n2=0; for(int s=0;s<256;++s) n2+=double(cnt[seeds[g]][s])*cnt[seeds[g]][s];
        double inv=n2>0?1.0/std::sqrt(n2):0; for(int s=0;s<256;++s) centroid[g][s]=double(cnt[seeds[g]][s])*inv; cnorm[g]=std::sqrt(cnorm[g]+0); for(int s=0;s<256;++s) cnorm[g]+=centroid[g][s]*centroid[g][s]; cnorm[g]=std::sqrt(cnorm[g]); }
    auto assign=[&](){ for(int p=0;p<256;++p){ if(ptot[p]==0){group[p]=0;continue;} double n2=0; for(int s=0;s<256;++s) n2+=double(cnt[p][s])*cnt[p][s]; double inv=n2>0?1.0/std::sqrt(n2):0; double best=1e300; uint8_t bg=0;
        for(uint8_t g=0;g<kCtxK;++g){ if(cnorm[g]<=0) continue; double ct=0; for(int s=0;s<256;++s) ct+=double(cnt[p][s])*inv*centroid[g][s]; double d=1.0-ct; if(d<best){best=d;bg=g;} } group[p]=bg; } };
    for(int it=0;it<3;++it){ assign(); std::array<std::array<double,256>,kCtxK> sm{}; std::array<double,kCtxK> c2{};
        for(int p=0;p<256;++p){ uint8_t g=group[p]; double n2=0; for(int s=0;s<256;++s) n2+=double(cnt[p][s])*cnt[p][s]; double inv=n2>0?1.0/std::sqrt(n2):0;
            for(int s=0;s<256;++s){ double v=double(cnt[p][s])*inv; sm[g][s]+=v; c2[g]+=v*v; } }
        for(uint8_t g=0;g<kCtxK;++g){ if(c2[g]>0){ double inv=1.0/std::sqrt(c2[g]); for(int s=0;s<256;++s) centroid[g][s]=sm[g][s]*inv; cnorm[g]=1.0; } else {for(int s=0;s<256;++s)centroid[g][s]=0; cnorm[g]=0; } } }
    assign();
    std::array<uint8_t,kCtxK> renum{}; uint8_t keff=0; std::array<std::array<uint32_t,256>,kCtxK> gcount{};
    for(int g=0;g<kCtxK;++g){ bool any=false; for(int p=0;p<256;++p) if(group[p]==g&&ptot[p]) any=true; if(!any) continue; renum[g]=keff++;
        for(int p=0;p<256;++p) if(group[p]==g) for(int s=0;s<256;++s) gcount[renum[g]][s]+=cnt[p][s]; }
    if(keff==0) keff=1; cm.K=keff; for(int p=0;p<256;++p) cm.map[p]=renum[group[p]];
    for(uint8_t g=0;g<keff;++g){ uint32_t tg=0; for(int s=0;s<256;++s) tg+=gcount[g][s]; if(tg==0) continue;
        std::vector<uint8_t> synth; synth.reserve(tg); for(int s=0;s<256;++s) for(uint32_t j=0;j<gcount[g][s];++j) synth.push_back((uint8_t)s);
        cm.m[g]=build_rans_model(synth,tot); }
    return cm;
}
static std::vector<uint8_t> ctx_rans_encode(const std::vector<uint8_t>& src,const CtxModel& cm,const RansSpec& sp){
    if(src.empty()) return {}; uint32_t x=sp.L; std::vector<uint8_t> em; em.reserve(src.size()/2+16);
    for(size_t ii=src.size();ii-->0;){ uint8_t s=src[ii]; uint8_t ctx=ii>0?cm.map[src[ii-1]]:cm.map[0]; const RansModel& m=cm.m[ctx];
        uint32_t f=m.freq[s],st=m.start[s]; uint32_t xmax=((sp.L>>sp.scale_bits)<<8)*f; while(x>=xmax){em.push_back((uint8_t)x);x>>=8;} x=((x/f)<<sp.scale_bits)+(x%f)+st; }
    std::vector<uint8_t> out(4); out[0]=(uint8_t)x; out[1]=(uint8_t)(x>>8); out[2]=(uint8_t)(x>>16); out[3]=(uint8_t)(x>>24);
    out.reserve(4+em.size()); for(auto it=em.rbegin();it!=em.rend();++it) out.push_back(*it); return out;
}
static std::vector<uint8_t> ctx_stream_bytes(const std::vector<uint8_t>& src,const RansSpec& sp){
    CtxModel cm=build_ctx_model(src,sp.tot); auto rd=ctx_rans_encode(src,cm,sp);
    std::vector<uint8_t> z; z.push_back(6); put_uvar(z,src.size()); z.push_back(cm.K);
    for(int i=0;i<256;++i) z.push_back(cm.map[i]);
    for(uint8_t g=0;g<cm.K;++g){ uint32_t nz=0; for(auto f:cm.m[g].freq) if(f) ++nz; put_uvar(z,nz);
        for(int i=0;i<256;++i) if(cm.m[g].freq[i]){ z.push_back((uint8_t)i); put_uvar(z,cm.m[g].freq[i]); } }
    put_uvar(z,rd.size()); z.insert(z.end(),rd.begin(),rd.end()); return z;
}
static std::vector<uint8_t> encode_stream_smallest(const std::vector<uint8_t>& src){
    std::vector<std::vector<uint8_t>> cands;
    std::vector<uint8_t> raw; raw.push_back(0); put_uvar(raw,src.size()); raw.insert(raw.end(),src.begin(),src.end()); cands.push_back(std::move(raw));
    if(src.size()>=16){
        cands.push_back(rans_stream_bytes(src,kRans4096,1));
        cands.push_back(rans_stream_bytes(src,kRans512,2));
        cands.push_back(rans_stream_bytes(src,kRans256,3));
        cands.push_back(huffman_stream_bytes(src,huffman_lengths(src)));
        std::array<uint32_t,256> cnt{}; for(uint8_t b:src) ++cnt[b]; uint8_t def=0; for(int i=1;i<256;++i) if(cnt[i]>cnt[def]) def=(uint8_t)i;
        if(cnt[def]>=src.size()/2) cands.push_back(defexc_stream_bytes(src,def));
        if(src.size()>=4096) cands.push_back(ctx_stream_bytes(src,kRans4096));
    }
    size_t best=0; for(size_t i=1;i<cands.size();++i) if(cands[i].size()<cands[best].size()) best=i;
    return std::move(cands[best]);
}

// ---- BWT + MTF/RLE + postcoders (verbatim logic from anvil 3718-3873) ----
struct BwtMtfRle { std::vector<uint8_t> tokens, runs; };
static BwtMtfRle bwt_mtf_rle(const std::vector<uint8_t>& bwt){
    std::array<uint8_t,256> sym{},pos{}; for(uint32_t i=0;i<256;++i){sym[i]=(uint8_t)i;pos[i]=(uint8_t)i;}
    BwtMtfRle r; r.tokens.reserve(bwt.size()/2+16); r.runs.reserve(bwt.size()/16+16);
    uint64_t zrun=0; auto flush=[&](){ if(zrun){ r.tokens.push_back(0); append_varint_bytes(r.runs,zrun-1); zrun=0; } };
    for(uint8_t b:bwt){ uint32_t rank=pos[b]; if(rank==0){++zrun;continue;} flush(); r.tokens.push_back((uint8_t)rank);
        for(uint32_t j=rank;j>0;--j){ sym[j]=sym[j-1]; pos[sym[j]]=(uint8_t)j; } sym[0]=b; pos[b]=0; }
    flush(); return r;
}
static std::pair<std::vector<uint8_t>,uint64_t> bwt_arith_encode(const BwtMtfRle& r,bool order1){
    ArithmeticEncoder ac; AdaptiveModel tok0(256),runm(256);
    std::array<std::unique_ptr<AdaptiveModel>,257> tok1;
    auto model1=[&](uint32_t ctx)->AdaptiveModel&{ if(!tok1[ctx]) tok1[ctx]=std::make_unique<AdaptiveModel>(256); return *tok1[ctx]; };
    size_t rp=0; uint32_t prev=256;
    for(uint8_t t:r.tokens){ if(order1) model1(prev).encode(ac,t); else tok0.encode(ac,t); prev=t;
        if(t==0){ for(;;){ if(rp>=r.runs.size()) throw std::runtime_error("run bug"); uint8_t b=r.runs[rp++]; runm.encode(ac,b); if(!(b&0x80)) break; } } }
    if(rp!=r.runs.size()) throw std::runtime_error("trailing");
    auto bits=ac.finish(); uint64_t nb=ac.bit_count(); return {std::move(bits),nb};
}
static int32_t do_bwt(const std::vector<uint8_t>& in,std::vector<uint8_t>& out){
    const int32_t n=(int32_t)in.size(); out.resize(in.size()); std::vector<int32_t> tmp(in.size());
    int32_t primary=libsais_bwt(in.data(),out.data(),tmp.data(),n,0,nullptr); return primary;
}
// postcoder 0 payload only (internals for accounting)
static void post0_internals(const BwtMtfRle& mr,size_t& tok_payload,size_t& run_payload,size_t& hdr_overhead, size_t& run_raw_varint){
    auto ts=encode_stream_smallest(mr.tokens), rs=encode_stream_smallest(mr.runs);
    // header overhead = postcoder id(1) + primary uvar(<=5) [not counted here; caller adds]
    // within: each substream = mode(1) + uvar(raw_n) + payload. overhead of substream framing:
    size_t tok_overhead = 1 + 1; // mode + uvar(raw_n) (raw_n < 256 for tokens? use generic: 1 + uvar)
    // uvar(raw_n): for n up to 41M takes 4-5 bytes. Approximate by measuring.
    tok_payload = ts.size(); run_payload = rs.size();
    run_raw_varint = mr.runs.size(); // number of varint bytes in the uncompressed runs vector
    // recompute framing precisely below in caller using put_uvar sizes.
    (void)tok_overhead; (void)hdr_overhead;
}
static size_t uvar_len(uint64_t x){ size_t n=1; while(x>=0x80){++n;x>>=7;} return n; }

// ============================================================================
// Entropy helpers
// ============================================================================
static double shannon_o0(const std::vector<uint8_t>& v){
    if(v.empty()) return 0; std::array<uint64_t,256> c{}; for(uint8_t b:v) ++c[b];
    double H=0; for(int i=0;i<256;++i) if(c[i]){ double p=(double)c[i]/v.size(); H-=p*std::log2(p); } return H;
}
static double shannon_o1(const std::vector<uint8_t>& v){
    if(v.size()<2) return 0; std::array<uint64_t,256> ctx_tot{}; std::array<std::array<uint64_t,256>,256> cnt{};
    uint8_t prev=0; for(uint8_t b:v){ ++cnt[prev][b]; ++ctx_tot[prev]; prev=b; }
    double H=0; uint64_t tot=0;
    for(int c=0;c<256;++c) if(ctx_tot[c]){ double hc=0; for(int s=0;s<256;++s) if(cnt[c][s]){ double p=(double)cnt[c][s]/ctx_tot[c]; hc-=p*std::log2(p); }
        H+=ctx_tot[c]*hc; tot+=ctx_tot[c]; }
    return H/tot;
}
static double shannon_o2(const std::vector<uint8_t>& v){
    if(v.size()<3) return 0;
    // flat context table 65536 -> freq; use map to bound memory
    std::unordered_map<uint32_t,std::array<uint64_t,256>> ctxmap; std::array<uint64_t,65536> ctot{};
    uint8_t a=0,b=0; uint64_t tot=0; double H=0;
    for(uint8_t s:v){ uint32_t c=((uint32_t)a<<8)|b; auto it=ctxmap.find(c); if(it==ctxmap.end()){ ctxmap[c]={}; it=ctxmap.find(c);} ++it->second[s]; ++ctot[c]; a=b; b=s; }
    for(auto& kv:ctxmap){ uint32_t c=kv.first; uint64_t t=ctot[c]; double hc=0; for(int s=0;s<256;++s) if(kv.second[s]){ double p=(double)kv.second[s]/t; hc-=p*std::log2(p); } H+=t*hc; tot+=t; }
    return H/tot;
}

// ============================================================================
// LZP coverage (order-k hash, copy-match extension)
// ============================================================================
static double lzp_coverage(const std::vector<uint8_t>& v,int order,int minlen){
    if(v.size()<(size_t)(order+minlen)) return 0;
    std::vector<int64_t> tab(1<<20,-1); // 20-bit context hash
    uint64_t covered=0; size_t i=order;
    auto hashctx=[&](size_t p)->uint32_t{ uint32_t h=2166136261u; for(int k=order;k>0;--k){ h^=v[p-(size_t)k]; h*=16777619u; } return h & ((1u<<20)-1u); };
    while(i+minlen<=v.size()){
        uint32_t h=hashctx(i); int64_t pos=tab[h];
        if(pos>=0 && (size_t)pos+minlen<=v.size()){
            size_t l=0; while(l<minlen && v[pos+(int64_t)l]==v[i+l]) ++l; // ensure minlen
            if(l>=minlen){ // extend
                while(i+l<v.size() && (size_t)pos+l<v.size() && v[pos+(int64_t)l]==v[i+l]) ++l;
                covered+=l; i+=l; continue;
            }
        }
        tab[h]=(int64_t)i; ++i;
    }
    return (double)covered/v.size();
}

// ============================================================================
// Per-block analysis
// ============================================================================
struct BlockRes {
    std::string name;
    size_t input=0;
    int32_t primary=0;
    size_t tok_payload=0, run_payload=0, run_raw_varint=0, hdr_overhead=0, tok_hdr=0, run_hdr=0;
    size_t p0=0,p1=0,p2=0,p3=0, p0_best=0;
    size_t qlfc=0, qlfc_table=0, lzp=0, chosen=0;
    double H0_raw=0,H1_raw=0,H2_raw=0, H0_tok=0,H1_tok=0,H2_tok=0, H0_run=0,H1_run=0;
    double lzp4=0,lzp5=0;
    size_t n_tokens=0, n_zeroruns=0, total_zeros=0;
    double bwt_bytes_per_input=0;
};

// ---- Ablation ID 4: QLFC-like per-context static rANS on the MTF-token stream ----
struct QlfcRes { size_t total=0, table=0, stream=0; };
static QlfcRes qlfc_encode_tokens(const std::vector<uint8_t>& tokens){
    const uint32_t SCALE = 1u<<12;
    std::array<std::array<uint32_t,256>,257> cnt{}; std::array<uint32_t,257> ctot{};
    uint32_t prev=256; for(uint8_t t: tokens){ cnt[prev][t]++; ctot[prev]++; prev=t; }
    std::array<std::vector<uint8_t>,257> seg{}; prev=256;
    for(uint8_t t: tokens){ seg[prev].push_back(t); prev=t; }
    QlfcRes res; std::vector<uint8_t> hdr; uint32_t used=0;
    for(int c=0;c<257;++c) if(ctot[c]) ++used;
    put_uvar(hdr, used);
    for(int c=0;c<257;++c){ if(!ctot[c]) continue;
        put_uvar(hdr,(uint64_t)c);
        auto m=build_rans_model(seg[c],SCALE);
        uint32_t nz=0; for(auto f:m.freq) if(f) ++nz; put_uvar(hdr,nz);
        for(int s=0;s<256;++s) if(m.freq[s]){ hdr.push_back((uint8_t)s); put_uvar(hdr,m.freq[s]); }
    }
    res.table = hdr.size();
    std::vector<uint8_t> streams;
    for(int c=0;c<257;++c){ if(!ctot[c]) continue;
        auto m=build_rans_model(seg[c],SCALE);
        auto rd=rans_encode(seg[c],m,kRans4096);
        put_uvar(streams,rd.size()); streams.insert(streams.end(),rd.begin(),rd.end());
    }
    res.stream = streams.size();
    res.total = hdr.size()+streams.size();
    return res;
}

// ---- Ablation ID 5: LZP prepass (order-k hash, minlen) BEFORE BWT ----
static std::vector<uint8_t> lzp_transform(const std::vector<uint8_t>& v,int order,int minlen){
    std::vector<int64_t> tab(1<<20,-1);
    std::vector<uint8_t> out; out.reserve(v.size());
    size_t i=order;
    auto hashctx=[&](size_t p){ uint32_t h=2166136261u; for(int k=order;k>0;--k){ h^=v[p-(size_t)k]; h*=16777619u; } return h & ((1u<<20)-1u); };
    while(i+(size_t)minlen <= v.size()){
        uint32_t h=hashctx(i); int64_t pos=tab[h];
        if(pos>=0 && (size_t)pos+(size_t)minlen <= v.size()){
            size_t l=0; while(l<(size_t)minlen && v[pos+(int64_t)l]==v[i+l]) ++l;
            if(l>=(size_t)minlen){ while(i+l<v.size() && (size_t)pos+l<v.size() && v[pos+(int64_t)l]==v[i+l]) ++l;
                for(size_t k=0;k<l;++k) out.push_back(0);
                i+=l; continue; }
        }
        tab[h]=(int64_t)i; out.push_back(v[i]); ++i;
    }
    while(i<v.size()){ out.push_back(v[i]); ++i; }
    return out;
}

static size_t bwt_post2_size(const std::vector<uint8_t>& in){
    std::vector<uint8_t> bwt; int32_t primary=do_bwt(in,bwt);
    BwtMtfRle mr=bwt_mtf_rle(bwt);
    auto [b2,n2]=bwt_arith_encode(mr,true);
    return 1 + uvar_len((uint64_t)primary) + uvar_len(n2) + b2.size();
}

static int g_bwt_post = -1;

static BlockRes analyze(const std::vector<uint8_t>& in){
    BlockRes r; r.input=in.size();
    std::vector<uint8_t> bwt; r.primary=do_bwt(in,bwt);
    BwtMtfRle mr=bwt_mtf_rle(bwt);
    // counts
    r.n_tokens=mr.tokens.size(); r.n_zeroruns=0; r.total_zeros=0;
    size_t tzero=0; for(uint8_t t:mr.tokens){ if(t==0){ r.n_zeroruns++; tzero++; } }
    { size_t rp=0; while(rp<mr.runs.size()){ uint64_t rv=read_varint_bytes(mr.runs,rp); r.total_zeros += (size_t)rv+1; } }
    // postcoder 0
    auto ts=encode_stream_smallest(mr.tokens), rs=encode_stream_smallest(mr.runs);
    r.tok_payload=ts.size(); r.run_payload=rs.size(); r.run_raw_varint=mr.runs.size();
    // substream framing overhead: mode(1)+uvar(raw_n) each; plus outer postcoder-id(1)+primary uvar
    r.tok_hdr = 1 + uvar_len(mr.tokens.size());
    r.run_hdr = 1 + uvar_len(mr.runs.size());
    r.hdr_overhead = 1 /*postcoder id*/ + uvar_len((uint64_t)r.primary); // minimal
    size_t p0 = 1 + uvar_len((uint64_t)r.primary) + ts.size() + rs.size();
    r.p0=p0;
    // postcoder 1/2 (adaptive arithmetic) and 3 (raw)
    { auto [b1,n1]=bwt_arith_encode(mr,false); r.p1 = 1 + uvar_len((uint64_t)r.primary) + uvar_len(n1) + b1.size(); }
    { auto [b2,n2]=bwt_arith_encode(mr,true);  r.p2 = 1 + uvar_len((uint64_t)r.primary) + uvar_len(n2) + b2.size(); }
    { auto s=encode_stream_smallest(bwt); r.p3 = 1 + uvar_len((uint64_t)r.primary) + s.size(); }
    r.p0_best = std::min({r.p0,r.p1,r.p2,r.p3});
    QlfcRes q = qlfc_encode_tokens(mr.tokens);
    auto qrs = encode_stream_smallest(mr.runs);
    r.qlfc = 1 + uvar_len((uint64_t)r.primary) + q.total + qrs.size();
    r.qlfc_table = q.table;
    std::vector<uint8_t> lzp_in = lzp_transform(in,4,32);
    r.lzp = bwt_post2_size(lzp_in);
    if(g_bwt_post==4) r.chosen=r.qlfc;
    else if(g_bwt_post==5) r.chosen=r.lzp;
    else if(g_bwt_post>=0 && g_bwt_post<=3) r.chosen=(size_t[]){r.p0,r.p1,r.p2,r.p3}[g_bwt_post];
    else r.chosen=r.p0_best;
    // entropy
    r.H0_raw=shannon_o0(bwt); r.H1_raw=shannon_o1(bwt); r.H2_raw=shannon_o2(bwt);
    r.H0_tok=shannon_o0(mr.tokens); r.H1_tok=shannon_o1(mr.tokens); r.H2_tok=shannon_o2(mr.tokens);
    r.H0_run=shannon_o0(mr.runs); r.H1_run=shannon_o1(mr.runs);
    r.lzp4=lzp_coverage(in,4,32); r.lzp5=lzp_coverage(in,5,32);
    r.bwt_bytes_per_input=(double)r.p0_best/in.size();
    return r;
}

static void print_block(const BlockRes& r,const char* tag){
    printf("\n=== %s (input %zu) ===\n", tag, r.input);
    printf("  BWT payload chosen postcoder  : %zu B  (%.4f b/B)\n", r.p0_best, 8.0*r.p0_best/r.input);
    printf("  postcoder split: p0(static)=%zu p1(o0arith)=%zu p2(o1arith)=%zu p3(raw)=%zu\n", r.p0,r.p1,r.p2,r.p3);
    if(g_bwt_post==4 || g_bwt_post<0){
      printf("  ABLATION QLFC (ID4): total=%zu B  (postcoder2=%zu B)  table=%zu stream=%zu delta=%+zd B\n",
             r.qlfc, r.p2, r.qlfc_table, r.qlfc-r.qlfc_table, (ptrdiff_t)r.qlfc-(ptrdiff_t)r.p2);
    }
    if(g_bwt_post==5 || g_bwt_post<0){
      printf("  ABLATION LZP  (ID5): total=%zu B  (baseline best=%zu B)  delta=%+zd B  cov o4/32=%.2f%%\n",
             r.lzp, r.p0_best, (ptrdiff_t)r.lzp-(ptrdiff_t)r.p0_best, r.lzp4*100);
    }
    printf("  CHOSEN postcoder (gate): %zu B\n", r.chosen);
    printf("  MTF/RLE token stream : %zu tokens, %zu B coded (%.4f b/B)  | token-stream frame hdr %zu B\n", r.n_tokens, r.tok_payload, 8.0*r.tok_payload/r.input, r.tok_hdr);
    printf("  zero-run stream      : %zu runs covering %zu zeros, %zu B coded (%.4f b/B)  | run frame hdr %zu B, raw varint %zu B\n",
           r.n_zeroruns, r.total_zeros, r.run_payload, 8.0*r.run_payload/r.input, r.run_hdr, r.run_raw_varint);
    printf("  header/model overhead (pc id+primary idx): %zu B\n", r.hdr_overhead);
    printf("  -- entropy floors (bits/byte), input-normalized --\n");
    printf("  raw BWT  stream : H0=%.4f H1=%.4f H2=%.4f  (total bits H1=%.0f H2=%.0f)\n", r.H0_raw,r.H1_raw,r.H2_raw, r.H1_raw*r.input, r.H2_raw*r.input);
    printf("  MTF tokens     : H0=%.4f H1=%.4f H2=%.4f  (total bits H1=%.0f H2=%.0f)\n", r.H0_tok,r.H1_tok,r.H2_tok, r.H1_tok*r.input, r.H2_tok*r.input);
    printf("  run varints    : H0=%.4f H1=%.4f\n", r.H0_run,r.H1_run);
    // MTF+RLE representation floor = tok H1 + run H1 (approx; streams independent)
    double mtfrep_floor_bits = r.H1_tok*r.n_tokens + r.H1_run*r.run_raw_varint;
    double mtfrep_floor_B = mtfrep_floor_bits/8.0;
    printf("  MTF+RLE rep floor (H1tok+H1run): %.0f bits = %.0f B  vs current best postcoder %zu B  (headroom %.0f B)\n",
           mtfrep_floor_bits, mtfrep_floor_B, r.p0_best, (double)r.p0_best-mtfrep_floor_B);
    printf("  LZP coverage   : order4/32 = %.2f%%  order5/32 = %.2f%%\n", r.lzp4*100, r.lzp5*100);
}

int main(int argc,char** argv){
    if(argc<2){ printf("usage: %s <file> [--block=N]\n",argv[0]); return 1; }
    std::string path=argv[1]; size_t block=0;
    for(int i=2;i<argc;++i){ std::string a=argv[i];
        if(a.rfind("--block=",0)==0) block=std::stoull(a.substr(8));
        else if(a.rfind("--bwt-post=",0)==0) g_bwt_post=(int)std::stoi(a.substr(11)); }
    std::ifstream f(path,std::ios::binary); if(!f){ printf("cannot open %s\n",path.c_str()); return 1; }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
    printf("# bwt_lab analysis of %s (%zu bytes)\n",path.c_str(),data.size());
    if(block==0 || block>=data.size()){
        BlockRes r=analyze(data); print_block(r, path.c_str());
    } else {
        size_t nblocks=(data.size()+block-1)/block; double sumBest=0,sumIn=0;
        for(size_t b=0;b<nblocks;++b){ size_t s=b*block, e=std::min(s+block,data.size());
            BlockRes r=analyze(std::vector<uint8_t>(data.begin()+s,data.begin()+e));
            printf("  block %zu [%zu..%zu] best=%zu b/B=%.4f\n", b,s,e,r.p0_best,8.0*r.p0_best/(e-s));
            sumBest+=r.p0_best; sumIn+=(e-s);
        }
        printf("  BLOCKED TOTAL: %zu -> %zu B (%.4f b/B)\n", (size_t)sumIn,(size_t)sumBest, 8.0*sumBest/sumIn);
    }
    return 0;
}
