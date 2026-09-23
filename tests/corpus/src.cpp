#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace anvil {

static constexpr uint32_t kHalf = 0x80000000u;
static constexpr uint32_t kFirstQtr = 0x40000000u;
static constexpr uint32_t kThirdQtr = 0xC0000000u;
static constexpr uint32_t kModelRescale = 32768;
static constexpr uint32_t kHashBits = 18;
static constexpr uint32_t kHashSize = 1u << kHashBits;
static constexpr uint32_t kNoPos = 0xFFFFFFFFu;

struct BitWriter {
    std::vector<uint8_t> out;
    uint8_t cur = 0;
    uint8_t used = 0;
    void bit(uint32_t b) {
        cur = static_cast<uint8_t>((cur << 1) | (b & 1));
        if (++used == 8) { out.push_back(cur); cur = 0; used = 0; }
    }
    void finish() {
        if (used) { cur <<= (8 - used); out.push_back(cur); cur = 0; used = 0; }
    }
};

struct BitReader {
    const uint8_t* p;
    size_t n;
    size_t byte = 0;
    uint8_t bitpos = 0;
    uint32_t bit() {
        if (byte >= n) return 0; // arithmetic decoder pads with zeros
        uint32_t v = (p[byte] >> (7 - bitpos)) & 1u;
        if (++bitpos == 8) { bitpos = 0; ++byte; }
        return v;
    }
};

class ArithmeticEncoder {
    BitWriter bw_;
    uint32_t low_ = 0;
    uint32_t high_ = 0xFFFFFFFFu;
    uint64_t pending_ = 0;

    void emit_plus_pending(uint32_t b) {
        bw_.bit(b);
        while (pending_) { bw_.bit(b ^ 1u); --pending_; }
    }
public:
    void encode(uint32_t cum_lo, uint32_t cum_hi, uint32_t total) {
        if (!(cum_lo < cum_hi && cum_hi <= total && total > 0)) throw std::runtime_error("bad arithmetic interval");
        uint64_t range = static_cast<uint64_t>(high_) - low_ + 1;
        high_ = low_ + static_cast<uint32_t>((range * cum_hi) / total - 1);
        low_  = low_ + static_cast<uint32_t>((range * cum_lo) / total);
        for (;;) {
            if (high_ < kHalf) {
                emit_plus_pending(0);
            } else if (low_ >= kHalf) {
                emit_plus_pending(1);
                low_ -= kHalf; high_ -= kHalf;
            } else if (low_ >= kFirstQtr && high_ < kThirdQtr) {
                ++pending_;
                low_ -= kFirstQtr; high_ -= kFirstQtr;
            } else break;
            low_ <<= 1;
            high_ = (high_ << 1) | 1u;
        }
    }
    std::vector<uint8_t> finish() {
        ++pending_;
        if (low_ < kFirstQtr) emit_plus_pending(0); else emit_plus_pending(1);
        bw_.finish();
        return std::move(bw_.out);
    }
};

class ArithmeticDecoder {
    BitReader br_;
    uint32_t low_ = 0;
    uint32_t high_ = 0xFFFFFFFFu;
    uint32_t code_ = 0;
public:
    ArithmeticDecoder(const uint8_t* p, size_t n) : br_{p,n} {
        for (int i=0;i<32;++i) code_ = (code_ << 1) | br_.bit();
    }
    uint32_t scaled(uint32_t total) const {
        uint64_t range = static_cast<uint64_t>(high_) - low_ + 1;
        return static_cast<uint32_t>(((static_cast<uint64_t>(code_ - low_) + 1) * total - 1) / range);
    }
    void consume(uint32_t cum_lo, uint32_t cum_hi, uint32_t total) {
        uint64_t range = static_cast<uint64_t>(high_) - low_ + 1;
        high_ = low_ + static_cast<uint32_t>((range * cum_hi) / total - 1);
        low_  = low_ + static_cast<uint32_t>((range * cum_lo) / total);
        for (;;) {
            if (high_ < kHalf) {
                // no offset
            } else if (low_ >= kHalf) {
                code_ -= kHalf; low_ -= kHalf; high_ -= kHalf;
            } else if (low_ >= kFirstQtr && high_ < kThirdQtr) {
                code_ -= kFirstQtr; low_ -= kFirstQtr; high_ -= kFirstQtr;
            } else break;
            low_ <<= 1;
            high_ = (high_ << 1) | 1u;
            code_ = (code_ << 1) | br_.bit();
        }
    }
};

class AdaptiveModel {
    uint32_t alphabet_;
    std::vector<uint16_t> freq_;
    std::vector<uint32_t> tree_;
    uint32_t total_ = 0;

    void add_tree(uint32_t idx, uint32_t delta) {
        for (uint32_t i = idx + 1; i <= alphabet_; i += i & -i) tree_[i] += delta;
    }
    void rebuild() {
        std::fill(tree_.begin(), tree_.end(), 0);
        total_ = 0;
        for (uint32_t i=0;i<alphabet_;++i) { total_ += freq_[i]; add_tree(i, freq_[i]); }
    }
    uint32_t prefix(uint32_t sym) const { // sum [0,sym)
        uint32_t s = 0;
        for (uint32_t i=sym; i; i-=i&-i) s += tree_[i];
        return s;
    }
    void update(uint32_t sym) {
        if (total_ >= kModelRescale) {
            for (auto& f : freq_) f = static_cast<uint16_t>(std::max<uint16_t>(1, (f + 1) >> 1));
            rebuild();
        }
        ++freq_[sym]; ++total_; add_tree(sym, 1);
    }
public:
    explicit AdaptiveModel(uint32_t alphabet=256) : alphabet_(alphabet), freq_(alphabet,1), tree_(alphabet+1,0) { rebuild(); }
    void encode(ArithmeticEncoder& ac, uint32_t sym) {
        uint32_t lo = prefix(sym), hi = lo + freq_[sym];
        ac.encode(lo, hi, total_); update(sym);
    }
    uint32_t decode(ArithmeticDecoder& ad) {
        uint32_t target = ad.scaled(total_);
        uint32_t idx = 0, sum = 0;
        uint32_t bit = 1u << (31 - std::countl_zero(alphabet_));
        for (; bit; bit >>= 1) {
            uint32_t next = idx + bit;
            if (next <= alphabet_ && sum + tree_[next] <= target) { idx = next; sum += tree_[next]; }
        }
        if (idx >= alphabet_) throw std::runtime_error("arithmetic symbol out of range");
        uint32_t sym = idx;
        uint32_t lo = sum, hi = lo + freq_[sym];
        ad.consume(lo, hi, total_); update(sym); return sym;
    }
};

struct CodecModels {
    AdaptiveModel token{2};
    AdaptiveModel lit_len{256};
    AdaptiveModel match_len{256};
    AdaptiveModel dist{256};
    std::array<std::unique_ptr<AdaptiveModel>,257> lit;
    AdaptiveModel& literal(uint32_t ctx) {
        if (!lit[ctx]) lit[ctx] = std::make_unique<AdaptiveModel>(256);
        return *lit[ctx];
    }
};

static void encode_uvar(ArithmeticEncoder& ac, AdaptiveModel& m, uint64_t x) {
    for (;;) {
        uint8_t b = static_cast<uint8_t>(x & 0x7Fu); x >>= 7;
        if (x) b |= 0x80u;
        m.encode(ac,b);
        if (!x) break;
    }
}
static uint64_t decode_uvar(ArithmeticDecoder& ad, AdaptiveModel& m) {
    uint64_t x=0; int shift=0;
    for (int i=0;i<10;++i) {
        uint8_t b = static_cast<uint8_t>(m.decode(ad));
        x |= static_cast<uint64_t>(b & 0x7Fu) << shift;
        if (!(b&0x80u)) return x;
        shift += 7;
    }
    throw std::runtime_error("varint overflow");
}

static void put_uvar(std::vector<uint8_t>& out, uint64_t x) {
    do { uint8_t b=static_cast<uint8_t>(x&0x7f); x>>=7; if(x)b|=0x80; out.push_back(b); } while(x);
}
static uint64_t get_uvar(const uint8_t*& p, const uint8_t* e) {
    uint64_t x=0; int shift=0;
    for(int i=0;i<10;++i) { if(p>=e) throw std::runtime_error("truncated varint"); uint8_t b=*p++; x|=uint64_t(b&0x7f)<<shift; if(!(b&0x80))return x; shift+=7; }
    throw std::runtime_error("varint overflow");
}

static uint32_t crc32(const uint8_t* p, size_t n) {
    static std::array<uint32_t,256> table = []{
        std::array<uint32_t,256> t{};
        for(uint32_t i=0;i<256;++i){ uint32_t c=i; for(int k=0;k<8;++k)c=(c&1)?(0xEDB88320u^(c>>1)):(c>>1); t[i]=c; }
        return t;
    }();
    uint32_t c=0xFFFFFFFFu;
    for(size_t i=0;i<n;++i)c=table[(c^p[i])&0xFFu]^(c>>8);
    return c^0xFFFFFFFFu;
}
static void put_u32le(std::vector<uint8_t>& out,uint32_t x){ for(int i=0;i<4;++i)out.push_back(static_cast<uint8_t>(x>>(8*i))); }
static uint32_t get_u32le(const uint8_t*& p,const uint8_t* e){ if(e-p<4)throw std::runtime_error("truncated u32"); uint32_t x=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24); p+=4; return x; }

struct Match { uint32_t len, dist; };

static inline uint32_t hash4(const uint8_t* p) {
    uint32_t x; std::memcpy(&x,p,4);
    return (x * 0x9E3779B1u) >> (32-kHashBits);
}

static uint32_t match_length(const uint8_t* a, const uint8_t* b, uint32_t maxlen) {
    uint32_t i=0;
    while (i+8<=maxlen) {
        uint64_t x,y; std::memcpy(&x,a+i,8); std::memcpy(&y,b+i,8);
        uint64_t d=x^y;
        if (d) return i + static_cast<uint32_t>(std::countr_zero(d)/8);
        i+=8;
    }
    while(i<maxlen && a[i]==b[i]) ++i;
    return i;
}

class MatchFinder {
    const std::vector<uint8_t>& d_;
    std::vector<uint32_t> head_;
    std::vector<uint32_t> prev_;
    uint32_t max_chain_;
    uint32_t max_match_;
public:
    MatchFinder(const std::vector<uint8_t>& d, uint32_t max_chain, uint32_t max_match)
      : d_(d), head_(kHashSize,kNoPos), prev_(d.size(),kNoPos), max_chain_(max_chain), max_match_(max_match) {}
    void insert(uint32_t pos) {
        if (pos+4>d_.size()) return;
        uint32_t h=hash4(d_.data()+pos); prev_[pos]=head_[h]; head_[h]=pos;
    }
    std::vector<Match> find(uint32_t pos) const {
        std::vector<Match> out;
        if (pos+4>d_.size()) return out;
        uint32_t h=hash4(d_.data()+pos), q=head_[h];
        uint32_t remain=static_cast<uint32_t>(d_.size()-pos);
        uint32_t cap=std::min(remain,max_match_);
        uint32_t best=3;
        for(uint32_t depth=0; q!=kNoPos && depth<max_chain_; ++depth, q=prev_[q]) {
            if (q>=pos) break;
            uint32_t dist=pos-q;
            if (d_[q]!=d_[pos] || d_[q+1]!=d_[pos+1] || d_[q+2]!=d_[pos+2] || d_[q+3]!=d_[pos+3]) continue;
            uint32_t l=match_length(d_.data()+q,d_.data()+pos,cap);
            if(l>=4) {
                if (l>best || out.size()<3) { out.push_back({l,dist}); best=std::max(best,l); }
                if(l==cap) break;
            }
        }
        if(out.size()>8) {
            std::sort(out.begin(),out.end(),[](auto&a,auto&b){ if(a.len!=b.len)return a.len>b.len; return a.dist<b.dist; });
            out.resize(8);
        }
        return out;
    }
};

struct Token {
    bool match=false;
    uint32_t pos=0;
    uint32_t len=0;
    uint32_t dist=0;
};
struct ParseStats { uint64_t literals=0, matches=0, matched_bytes=0, tokens=0; };

static std::vector<Token> merge_literals(std::vector<Token> in) {
    std::vector<Token> out;
    for(auto &t:in) {
        if(!t.match && !out.empty() && !out.back().match && out.back().pos+out.back().len==t.pos) out.back().len += t.len;
        else out.push_back(t);
    }
    return out;
}

static std::vector<Token> parse_greedy(const std::vector<uint8_t>& d, uint32_t max_chain, uint32_t max_match) {
    MatchFinder mf(d,max_chain,max_match);
    std::vector<Token> toks;
    uint32_t i=0;
    while(i<d.size()) {
        auto ms=mf.find(i);
        Match best{0,0};
        for(auto&m:ms) if(m.len>best.len || (m.len==best.len && m.dist<best.dist)) best=m;
        if(best.len>=4) {
            toks.push_back({true,i,best.len,best.dist});
            uint32_t end=i+best.len;
            for(uint32_t p=i;p<end;++p) mf.insert(p);
            i=end;
        } else {
            toks.push_back({false,i,1,0}); mf.insert(i); ++i;
        }
    }
    return merge_literals(std::move(toks));
}

static double varint_cost(uint64_t x) {
    int bytes=1; while(x>=128){x>>=7;++bytes;}
    return 3.5 + 5.25*bytes; // adaptive byte model tends to beat raw 8-bit bytes
}

static std::vector<Token> parse_dp(const std::vector<uint8_t>& d, uint32_t max_chain, uint32_t max_match) {
    const uint32_t n=static_cast<uint32_t>(d.size());
    std::array<uint32_t,256> hist{}; for(auto b:d) ++hist[b];
    std::array<double,256> litcost{};
    for(int b=0;b<256;++b) {
        double p=(hist[b]+0.5)/(double(n)+128.0);
        litcost[b]=std::clamp(-std::log2(p),1.0,9.5);
    }
    struct Prev { uint32_t from=0, dist=0; bool match=false; };
    std::vector<double> dp(n+1,std::numeric_limits<double>::infinity());
    std::vector<Prev> prev(n+1);
    dp[0]=0;
    MatchFinder mf(d,max_chain,max_match);
    static constexpr uint32_t cuts[] = {4,5,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048,3072,4096,6144,8192,12288,16384,24576,32768,49152,65535};
    for(uint32_t i=0;i<n;++i) {
        double lc=dp[i]+litcost[d[i]]+0.10;
        if(lc<dp[i+1]) { dp[i+1]=lc; prev[i+1]={i,0,false}; }
        auto ms=mf.find(i);
        for(const auto&m:ms) {
            std::array<uint32_t,32> lens{}; size_t nl=0;
            for(uint32_t c:cuts) if(c<=m.len) lens[nl++]=c;
            if(nl==0 || lens[nl-1]!=m.len) lens[nl++]=m.len;
            for(size_t k=0;k<nl;++k) {
                uint32_t l=lens[k];
                double mc=dp[i]+1.0+varint_cost(l-4)+varint_cost(m.dist-1)+0.18*std::log2(double(m.dist)+1.0);
                uint32_t j=i+l;
                if(mc<dp[j]) { dp[j]=mc; prev[j]={i,m.dist,true}; }
            }
        }
        mf.insert(i);
    }
    std::vector<Token> rev;
    uint32_t cur=n;
    while(cur>0) {
        Prev p=prev[cur];
        if(p.from>=cur) throw std::runtime_error("DP parse reconstruction failed");
        rev.push_back({p.match,p.from,cur-p.from,p.dist}); cur=p.from;
    }
    std::reverse(rev.begin(),rev.end());
    return merge_literals(std::move(rev));
}

static ParseStats token_stats(const std::vector<Token>& t) {
    ParseStats s; s.tokens=t.size();
    for(auto&x:t) if(x.match){++s.matches;s.matched_bytes+=x.len;} else s.literals+=x.len;
    return s;
}

static constexpr uint32_t kRansScaleBits=12;
static constexpr uint32_t kRansTot=1u<<kRansScaleBits;
static constexpr uint32_t kRansL=1u<<23;

struct RansModel {
    std::array<uint16_t,256> freq{};
    std::array<uint16_t,256> start{};
};

static RansModel build_rans_model(const std::vector<uint8_t>& src) {
    RansModel m; if(src.empty()) return m;
    std::array<uint32_t,256> count{}; for(uint8_t b:src)++count[b];
    std::array<double,256> exact{}; uint32_t sum=0;
    for(int i=0;i<256;++i) if(count[i]) {
        exact[i]=double(count[i])*kRansTot/src.size();
        uint32_t f=std::max<uint32_t>(1,static_cast<uint32_t>(std::floor(exact[i])));
        m.freq[i]=static_cast<uint16_t>(f); sum+=f;
    }
    while(sum<kRansTot) {
        int best=-1; double score=-1e100;
        for(int i=0;i<256;++i) if(count[i]) { double sc=exact[i]-m.freq[i]; if(sc>score){score=sc;best=i;} }
        if(best<0) throw std::runtime_error("rANS normalization underflow");
        ++m.freq[best]; ++sum;
    }
    while(sum>kRansTot) {
        int best=-1; double score=-1e100;
        for(int i=0;i<256;++i) if(m.freq[i]>1) { double sc=m.freq[i]-exact[i]; if(sc>score){score=sc;best=i;} }
        if(best<0) throw std::runtime_error("rANS normalization overflow");
        --m.freq[best]; --sum;
    }
    uint32_t st=0; for(int i=0;i<256;++i){m.start[i]=static_cast<uint16_t>(st);st+=m.freq[i];}
    if(st!=kRansTot) throw std::runtime_error("rANS normalization sum");
    return m;
}

static std::vector<uint8_t> rans_encode(const std::vector<uint8_t>& src,const RansModel&m) {
    if(src.empty())return {};
    uint32_t x=kRansL; std::vector<uint8_t> emitted; emitted.reserve(src.size()/2+16);
    for(size_t ii=src.size();ii-->0;) {
        uint8_t sym=src[ii]; uint32_t f=m.freq[sym], st=m.start[sym];
        uint32_t x_max=((kRansL>>kRansScaleBits)<<8)*f;
        while(x>=x_max){emitted.push_back(static_cast<uint8_t>(x));x>>=8;}
        x=((x/f)<<kRansScaleBits)+(x%f)+st;
    }
    std::vector<uint8_t> out(4);
    out[0]=static_cast<uint8_t>(x); out[1]=static_cast<uint8_t>(x>>8); out[2]=static_cast<uint8_t>(x>>16); out[3]=static_cast<uint8_t>(x>>24);
    out.reserve(4+emitted.size());
    for(auto it=emitted.rbegin();it!=emitted.rend();++it)out.push_back(*it);
    return out;
}

static std::vector<uint8_t> rans_decode(const uint8_t* p,size_t n,size_t out_n,const RansModel&m) {
    if(out_n==0)return {};
    if(n<4)throw std::runtime_error("truncated rANS state");
    const uint8_t* q=p; const uint8_t* e=p+n; uint32_t x=get_u32le(q,e);
    std::array<uint8_t,kRansTot> symtab{};
    for(int s=0;s<256;++s) if(m.freq[s]) for(uint32_t j=0;j<m.freq[s];++j)symtab[m.start[s]+j]=static_cast<uint8_t>(s);
    std::vector<uint8_t> out(out_n);
    for(size_t i=0;i<out_n;++i) {
        uint32_t slot=x&(kRansTot-1); uint8_t sym=symtab[slot]; out[i]=sym;
        x=uint32_t(m.freq[sym])*(x>>kRansScaleBits)+slot-m.start[sym];
        while(x<kRansL){ if(q>=e)throw std::runtime_error("truncated rANS renorm"); x=(x<<8)|*q++; }
    }
    if(q!=e)throw std::runtime_error("trailing rANS bytes");
    return out;
}

static std::vector<uint8_t> encode_stream(const std::vector<uint8_t>& src) {
    std::vector<uint8_t> raw; raw.push_back(0); put_uvar(raw,src.size()); raw.insert(raw.end(),src.begin(),src.end());
    if(src.size()<16)return raw;
    RansModel m=build_rans_model(src); auto rd=rans_encode(src,m); std::vector<uint8_t> z; z.push_back(1); put_uvar(z,src.size());
    uint32_t nz=0;for(auto f:m.freq)if(f)++nz; put_uvar(z,nz);
    for(int i=0;i<256;++i)if(m.freq[i]){z.push_back(static_cast<uint8_t>(i));put_uvar(z,m.freq[i]);}
    put_uvar(z,rd.size()); z.insert(z.end(),rd.begin(),rd.end());
    return z.size()<raw.size()?z:raw;
}

static std::vector<uint8_t> decode_stream(const uint8_t*&p,const uint8_t*e) {
    if(p>=e) throw std::runtime_error("truncated stream header");
    uint8_t mode=*p++; uint64_t raw_n=get_uvar(p,e);
    if(raw_n>(1ull<<32))throw std::runtime_error("stream too large");
    if(mode==0){if(raw_n>uint64_t(e-p))throw std::runtime_error("truncated raw stream");std::vector<uint8_t>o(p,p+raw_n);p+=raw_n;return o;}
    if(mode!=1)throw std::runtime_error("unknown stream codec");
    uint64_t nz=get_uvar(p,e); if(nz>256)throw std::runtime_error("bad rANS model"); RansModel m; uint32_t sum=0;
    for(uint64_t k=0;k<nz;++k){if(p>=e)throw std::runtime_error("truncated rANS model");uint8_t sym=*p++;uint64_t f=get_uvar(p,e);if(f==0||f>kRansTot||m.freq[sym])throw std::runtime_error("bad rANS frequency");m.freq[sym]=static_cast<uint16_t>(f);sum+=f;}
    if(sum!=kRansTot) throw std::runtime_error("bad rANS total");
    uint32_t st=0;for(int i=0;i<256;++i){m.start[i]=static_cast<uint16_t>(st);st+=m.freq[i];}
    uint64_t dn=get_uvar(p,e);if(dn>uint64_t(e-p))throw std::runtime_error("truncated rANS stream");auto out=rans_decode(p,static_cast<size_t>(dn),static_cast<size_t>(raw_n),m);p+=dn;return out;
}

static void append_varint_bytes(std::vector<uint8_t>& out,uint64_t x){do{uint8_t b=static_cast<uint8_t>(x&0x7f);x>>=7;if(x)b|=0x80;out.push_back(b);}while(x);}
static uint64_t read_varint_bytes(const std::vector<uint8_t>&v,size_t&pos){uint64_t x=0;int sh=0;for(int i=0;i<10;++i){if(pos>=v.size())throw std::runtime_error("stream varint truncated");uint8_t b=v[pos++];x|=uint64_t(b&0x7f)<<sh;if(!(b&0x80))return x;sh+=7;}throw std::runtime_error("stream varint overflow");}

static std::vector<uint8_t> encode_tokens_rans(const std::vector<uint8_t>&d,const std::vector<Token>&toks){
    std::vector<uint8_t> types,ll,ml,ds,lits;types.reserve(toks.size());
    for(auto&t:toks){types.push_back(t.match?1:0);if(t.match){append_varint_bytes(ml,t.len-4);append_varint_bytes(ds,t.dist-1);}else{append_varint_bytes(ll,t.len-1);lits.insert(lits.end(),d.begin()+t.pos,d.begin()+t.pos+t.len);}}
    std::vector<uint8_t> out; for(const auto* v:{&types,&ll,&ml,&ds,&lits}){auto z=encode_stream(*v);put_uvar(out,z.size());out.insert(out.end(),z.begin(),z.end());} return out;
}

static std::vector<uint8_t> decode_tokens_rans(const uint8_t*p,size_t n,size_t out_len){
    const uint8_t*e=p+n;std::array<std::vector<uint8_t>,5>s;
    for(int i=0;i<5;++i){uint64_t zn=get_uvar(p,e);if(zn>uint64_t(e-p))throw std::runtime_error("truncated substream");const uint8_t*q=p;const uint8_t*qe=p+zn;s[i]=decode_stream(q,qe);if(q!=qe)throw std::runtime_error("substream trailing bytes");p+=zn;}
    if(p!=e)throw std::runtime_error("payload trailing bytes");
    size_t ip_ll=0,ip_ml=0,ip_ds=0,ip_lit=0;std::vector<uint8_t>out;out.reserve(out_len);
    for(uint8_t type:s[0]){
        if(out.size()>=out_len)throw std::runtime_error("too many tokens");
        if(type==0){uint64_t len=read_varint_bytes(s[1],ip_ll)+1;if(len>out_len-out.size()||len>s[4].size()-ip_lit)throw std::runtime_error("bad literal run");out.insert(out.end(),s[4].begin()+ip_lit,s[4].begin()+ip_lit+len);ip_lit+=len;}
        else if(type==1){uint64_t len=read_varint_bytes(s[2],ip_ml)+4,dist=read_varint_bytes(s[3],ip_ds)+1;if(dist>out.size()||len>out_len-out.size())throw std::runtime_error("bad rANS match");for(uint64_t k=0;k<len;++k)out.push_back(out[out.size()-dist]);}
        else throw std::runtime_error("bad token type");
    }
    if(out.size()!=out_len||ip_ll!=s[1].size()||ip_ml!=s[2].size()||ip_ds!=s[3].size()||ip_lit!=s[4].size()) throw std::runtime_error("substream consumption mismatch");
    return out;
}

static uint32_t gate_threshold(uint8_t lit_mode) {
    if(lit_mode==3) return 4;
    if(lit_mode==4) return 8;
    if(lit_mode==5) return 16;
    return 0;
}

static std::vector<uint8_t> encode_tokens(const std::vector<uint8_t>& d, const std::vector<Token>& toks, uint8_t lit_mode) {
    ArithmeticEncoder ac; CodecModels m; std::array<uint32_t,256> ctx_seen{};
    const uint32_t threshold=gate_threshold(lit_mode);
    for(const auto&t:toks) {
        m.token.encode(ac,t.match?1:0);
        if(!t.match) {
            encode_uvar(ac,m.lit_len,t.len-1);
            for(uint32_t k=0;k<t.len;++k) {
                uint32_t pos=t.pos+k;
                uint32_t rawctx = pos ? d[pos-1] : 256u;
                uint32_t modelctx=256u;
                if(lit_mode==2) modelctx=rawctx;
                else if(threshold && rawctx<256 && ctx_seen[rawctx]>=threshold) modelctx=rawctx;
                m.literal(modelctx).encode(ac,d[pos]);
                if(rawctx<256) ++ctx_seen[rawctx];
            }
        } else {
            encode_uvar(ac,m.match_len,t.len-4);
            encode_uvar(ac,m.dist,t.dist-1);
        }
    }
    return ac.finish();
}

static std::vector<uint8_t> decode_tokens(const uint8_t* p, size_t n, size_t out_len, uint8_t lit_mode) {
    ArithmeticDecoder ad(p,n); CodecModels m; std::array<uint32_t,256> ctx_seen{}; std::vector<uint8_t> out; out.reserve(out_len);
    const uint32_t threshold=gate_threshold(lit_mode);
    while(out.size()<out_len) {
        uint32_t is_match=m.token.decode(ad);
        if(!is_match) {
            uint64_t len=decode_uvar(ad,m.lit_len)+1;
            if(len>out_len-out.size()) throw std::runtime_error("literal run exceeds block");
            for(uint64_t k=0;k<len;++k) {
                uint32_t rawctx=out.empty()?256u:static_cast<uint32_t>(out.back());
                uint32_t modelctx=256u;
                if(lit_mode==2) modelctx=rawctx;
                else if(threshold && rawctx<256 && ctx_seen[rawctx]>=threshold) modelctx=rawctx;
                uint8_t b=static_cast<uint8_t>(m.literal(modelctx).decode(ad)); out.push_back(b);
                if(rawctx<256) ++ctx_seen[rawctx];
            }
        } else {
            uint64_t len=decode_uvar(ad,m.match_len)+4;
            uint64_t dist=decode_uvar(ad,m.dist)+1;
            if(dist==0 || dist>out.size()) throw std::runtime_error("invalid match distance");
            if(len>out_len-out.size()) throw std::runtime_error("match exceeds block");
            for(uint64_t k=0;k<len;++k) out.push_back(out[out.size()-dist]);
        }
    }
    return out;
}

struct Options {
    uint32_t block_size=256*1024;
    uint32_t max_chain=48;
    uint32_t max_match=65535;
    std::string parse="auto";
    std::string literal="auto"; // auto|o0|o1|g4|g8|g16
    std::string entropy="auto"; // auto|arith|rans
    bool quiet=false;
};
struct GlobalStats { uint64_t in=0,out=0,blocks=0,raw_blocks=0,compressed_blocks=0,literals=0,matches=0,matched_bytes=0,tokens=0; };

static std::vector<uint8_t> compress(const std::vector<uint8_t>& input, const Options& opt, GlobalStats* gs) {
    std::vector<uint8_t> out={'A','N','V','0',1};
    put_uvar(out,opt.block_size); put_uvar(out,input.size());
    GlobalStats st; st.in=input.size();
    for(size_t off=0; off<input.size();) {
        size_t blen=std::min<size_t>(opt.block_size,input.size()-off);
        std::vector<uint8_t> block(input.begin()+off,input.begin()+off+blen);

        struct Candidate { std::vector<uint8_t> payload; std::vector<Token> toks; uint8_t mode=0; };
        Candidate best;
        auto consider_parse = [&](std::vector<Token> toks) {
            auto try_lit = [&](uint8_t mode) {
                auto payload=encode_tokens(block,toks,mode);
                if(best.mode==0 || payload.size()<best.payload.size()) best={std::move(payload),toks,mode};
            };
            if(opt.entropy=="auto" || opt.entropy=="arith") {
                if(opt.literal=="auto" || opt.literal=="o0") try_lit(1);
                if(opt.literal=="auto" || opt.literal=="o1") try_lit(2);
                if(opt.literal=="auto" || opt.literal=="g4") try_lit(3);
                if(opt.literal=="auto" || opt.literal=="g8") try_lit(4);
                if(opt.literal=="auto" || opt.literal=="g16") try_lit(5);
            }
            if(opt.entropy=="auto" || opt.entropy=="rans") {
                auto payload=encode_tokens_rans(block,toks);
                if(best.mode==0 || payload.size()<best.payload.size()) best={std::move(payload),toks,10};
            }
        };
        if(opt.parse=="auto" || opt.parse=="greedy") consider_parse(parse_greedy(block,opt.max_chain,opt.max_match));
        if(opt.parse=="auto" || opt.parse=="dp") consider_parse(parse_dp(block,opt.max_chain,opt.max_match));
        if(best.mode==0) throw std::runtime_error("no encoder candidate");

        auto ps=token_stats(best.toks);
        ++st.blocks; st.literals+=ps.literals; st.matches+=ps.matches; st.matched_bytes+=ps.matched_bytes; st.tokens+=ps.tokens;
        put_uvar(out,blen);
        uint32_t sum=crc32(block.data(),block.size());
        if(best.payload.size()+1 < block.size()) {
            out.push_back(best.mode); put_uvar(out,best.payload.size()); put_u32le(out,sum); out.insert(out.end(),best.payload.begin(),best.payload.end()); ++st.compressed_blocks;
        } else {
            out.push_back(0); put_uvar(out,block.size()); put_u32le(out,sum); out.insert(out.end(),block.begin(),block.end()); ++st.raw_blocks;
        }
        off+=blen;
    }
    st.out=out.size(); if(gs)*gs=st; return out;
}

static std::vector<uint8_t> decompress(const std::vector<uint8_t>& in) {
    if(in.size()<5 || std::memcmp(in.data(),"ANV0",4)!=0 || in[4]!=1) throw std::runtime_error("not ANVIL v0.1");
    const uint8_t* p=in.data()+5; const uint8_t* e=in.data()+in.size();
    uint64_t block_size=get_uvar(p,e);
    if(block_size==0 || block_size>(64ull<<20)) throw std::runtime_error("invalid block size");
    uint64_t total=get_uvar(p,e); if(total>std::numeric_limits<size_t>::max()) throw std::runtime_error("output too large");
    std::vector<uint8_t> out; out.reserve(static_cast<size_t>(std::min<uint64_t>(total,64ull<<20)));
    while(out.size()<total) {
        uint64_t blen=get_uvar(p,e); if(blen==0 || blen>block_size) throw std::runtime_error("invalid block length");
        if(p>=e) throw std::runtime_error("truncated block header");
        uint8_t mode=*p++; uint64_t plen=get_uvar(p,e); uint32_t expected_crc=get_u32le(p,e);
        if(plen>uint64_t(e-p)) throw std::runtime_error("truncated block payload");
        if(blen>total-out.size()) throw std::runtime_error("block exceeds declared output");
        std::vector<uint8_t> b;
        if(mode==0) {
            if(plen!=blen) throw std::runtime_error("raw block length mismatch");
            b.assign(p,p+plen);
        } else if(mode>=1 && mode<=5) {
            b=decode_tokens(p,static_cast<size_t>(plen),static_cast<size_t>(blen),mode);
        } else if(mode==10) {
            b=decode_tokens_rans(p,static_cast<size_t>(plen),static_cast<size_t>(blen));
        } else throw std::runtime_error("unknown block mode");
        if(crc32(b.data(),b.size())!=expected_crc) throw std::runtime_error("block checksum mismatch");
        out.insert(out.end(),b.begin(),b.end()); p+=plen;
    }
    if(out.size()!=total) throw std::runtime_error("size mismatch");
    if(p!=e) throw std::runtime_error("trailing bytes after final block");
    return out;
}

static std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path,std::ios::binary); if(!f) throw std::runtime_error("cannot open input: "+path);
    f.seekg(0,std::ios::end); auto n=f.tellg(); f.seekg(0); std::vector<uint8_t> d(static_cast<size_t>(n)); if(n>0)f.read(reinterpret_cast<char*>(d.data()),n); return d;
}
static void write_file(const std::string& path,const std::vector<uint8_t>& d) {
    std::ofstream f(path,std::ios::binary); if(!f) throw std::runtime_error("cannot open output: "+path); if(!d.empty())f.write(reinterpret_cast<const char*>(d.data()),d.size());
}
static uint64_t fnv1a(const std::vector<uint8_t>& d) { uint64_t h=1469598103934665603ull; for(auto b:d){h^=b;h*=1099511628211ull;} return h; }

static void usage() {
    std::cerr << "ANVIL v0 research codec\n"
              << "  anvil c <input> <output> [--parse=auto|dp|greedy] [--literal=auto|o0|o1|g4|g8|g16] [--entropy=auto|arith|rans] [--block=N] [--chain=N] [--max-match=N] [--quiet]\n"
              << "  anvil d <input> <output> [--quiet]\n"
              << "  anvil verify <input> [--parse=auto|dp|greedy] [--literal=auto|o0|o1|g4|g8|g16] [--entropy=auto|arith|rans]\n";
}

} // namespace anvil

#ifndef ANVIL_NO_MAIN
int main(int argc,char**argv) {
    using namespace anvil;
    try {
        if(argc<3){usage();return 2;}
        std::string cmd=argv[1]; Options opt;
        for(int i=(cmd=="verify"?3:4);i<argc;++i) {
            std::string a=argv[i];
            if(a.rfind("--parse=",0)==0)opt.parse=a.substr(8);
            else if(a.rfind("--literal=",0)==0)opt.literal=a.substr(10);
            else if(a.rfind("--entropy=",0)==0)opt.entropy=a.substr(10);
            else if(a.rfind("--block=",0)==0)opt.block_size=std::stoul(a.substr(8));
            else if(a.rfind("--chain=",0)==0)opt.max_chain=std::stoul(a.substr(8));
            else if(a.rfind("--max-match=",0)==0)opt.max_match=std::stoul(a.substr(12));
            else if(a=="--quiet")opt.quiet=true;
            else throw std::runtime_error("unknown option: "+a);
        }
        if(opt.parse!="auto"&&opt.parse!="dp"&&opt.parse!="greedy")throw std::runtime_error("parse must be auto, dp or greedy");
        if(opt.literal!="auto"&&opt.literal!="o0"&&opt.literal!="o1"&&opt.literal!="g4"&&opt.literal!="g8"&&opt.literal!="g16")throw std::runtime_error("literal must be auto, o0, o1, g4, g8 or g16");
        if(opt.entropy!="auto"&&opt.entropy!="arith"&&opt.entropy!="rans")throw std::runtime_error("entropy must be auto, arith or rans");
        if(cmd=="c") {
            if(argc<4){usage();return 2;} auto in=read_file(argv[2]); GlobalStats st;
            auto t0=std::chrono::steady_clock::now(); auto out=compress(in,opt,&st); auto t1=std::chrono::steady_clock::now(); write_file(argv[3],out);
            if(!opt.quiet){double sec=std::chrono::duration<double>(t1-t0).count(); std::cerr<<"ANVIL c parse="<<opt.parse<<" literal="<<opt.literal<<" entropy="<<opt.entropy<<" in="<<st.in<<" out="<<st.out<<" ratio="<<(st.in?double(st.out)/st.in:0)<<" MB/s="<<(sec?st.in/1e6/sec:0)<<" blocks="<<st.blocks<<" compressed="<<st.compressed_blocks<<" raw="<<st.raw_blocks<<" literals="<<st.literals<<" matches="<<st.matches<<" matched_bytes="<<st.matched_bytes<<"\n";}
        } else if(cmd=="d") {
            if(argc<4){usage();return 2;} auto in=read_file(argv[2]); auto t0=std::chrono::steady_clock::now(); auto out=decompress(in); auto t1=std::chrono::steady_clock::now(); write_file(argv[3],out);
            if(!opt.quiet){double sec=std::chrono::duration<double>(t1-t0).count(); std::cerr<<"ANVIL d out="<<out.size()<<" MB/s="<<(sec?out.size()/1e6/sec:0)<<"\n";}
        } else if(cmd=="verify") {
            auto in=read_file(argv[2]); GlobalStats st; auto enc=compress(in,opt,&st); auto dec=decompress(enc);
            if(dec!=in) throw std::runtime_error("round-trip mismatch");
            std::cout<<"OK bytes="<<in.size()<<" encoded="<<enc.size()<<" fnv64="<<std::hex<<fnv1a(in)<<std::dec<<"\n";
        } else { usage(); return 2; }
        return 0;
    } catch(const std::exception& e) { std::cerr<<"anvil: "<<e.what()<<"\n"; return 1; }
}

#endif // ANVIL_NO_MAIN
