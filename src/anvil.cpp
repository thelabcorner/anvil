#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
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
    // Number of payload bytes touched by the bit reader (final partial byte
    // counts as one). Used to reject in-payload trailing garbage (F1).
    size_t consumed_bytes() const { return br_.byte + (br_.bitpos ? 1u : 0u); }
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

// ---- SPARSE-REF (block mode 11) ------------------------------------------
// Approximate self-reference: copy a prior phrase (no byte-identity required)
// and entropy-code a sparse correction mask (flat 32-bit words, 4 B per 32 B of
// phrase, LSB = byte at the window start) plus the residual bytes in mask order.
// Decoder stays copy + sparse stores. The mask is a first-class entropy-coded
// stream; len(residuals) == popcount(mask) is a strict decoder invariant.
static constexpr uint32_t kSparseScanMax = 2048;    // max phrase length considered
static constexpr uint32_t kSparseChainMax = 32;     // chain depth for sparse candidates
static constexpr uint32_t kSparsePosBudget = 16384; // per-position scanned-byte cap
static constexpr uint64_t kSparseBlockBudget = 64ull * 1024 * 1024; // per-block scanned-byte cap
static constexpr uint32_t kSparseMaxLen = 65536;    // hard decoder bound per sparse token

struct SparseMatch {
    uint32_t len = 0;
    uint32_t dist = 0;
    std::vector<uint32_t> off;  // correction offsets, strictly increasing, < len
    std::vector<uint8_t> val;   // replacement bytes in mask order
};

struct SparseToken {
    uint8_t type = 0;  // 0 literal run, 1 exact match, 2 sparse-corrected match
    uint32_t pos = 0, len = 0, dist = 0;
    std::vector<uint32_t> off;
    std::vector<uint8_t> val;
};

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
    // Boundary-aligned candidate index (Linux C5): only token-start positions
    // are inserted here, so chains are short and sources align with structure.
    std::vector<uint32_t> bhead_;
    std::vector<uint32_t> bprev_;
    bool use_boundary_;
    uint32_t max_chain_;
    uint32_t max_match_;
    static std::vector<Match> walk(const uint8_t* d, size_t n, uint32_t pos, uint32_t q,
                                   const std::vector<uint32_t>& prev, uint32_t max_chain, uint32_t max_match) {
        std::vector<Match> out;
        if (pos + 4 > n) return out;
        uint32_t remain = static_cast<uint32_t>(n - pos);
        uint32_t cap = std::min(remain, max_match);
        uint32_t best = 3;
        for (uint32_t depth = 0; q != kNoPos && depth < max_chain; ++depth, q = prev[q]) {
            if (q >= pos) break;
            uint32_t dist = pos - q;
            if (d[q] != d[pos] || d[q+1] != d[pos+1] || d[q+2] != d[pos+2] || d[q+3] != d[pos+3]) continue;
            uint32_t l = match_length(d + q, d + pos, cap);
            if (l >= 4) {
                if (l > best || out.size() < 3) { out.push_back({l, dist}); best = std::max(best, l); }
                if (l == cap) break;
            }
        }
        if (out.size() > 8) {
            std::sort(out.begin(), out.end(), [](auto&a, auto&b){ if(a.len!=b.len)return a.len>b.len; return a.dist<b.dist; });
            out.resize(8);
        }
        return out;
    }
public:
    MatchFinder(const std::vector<uint8_t>& d, uint32_t max_chain, uint32_t max_match, bool boundary=false)
      : d_(d), head_(kHashSize,kNoPos), prev_(d.size(),kNoPos), bhead_(kHashSize,kNoPos),
        bprev_(d.size(),kNoPos), use_boundary_(boundary), max_chain_(max_chain), max_match_(max_match) {}
    void insert(uint32_t pos) {
        if (pos+4>d_.size()) return;
        uint32_t h=hash4(d_.data()+pos); prev_[pos]=head_[h]; head_[h]=pos;
    }
    void insert_boundary(uint32_t pos) {
        if (pos+4>d_.size()) return;
        uint32_t h=hash4(d_.data()+pos); bprev_[pos]=bhead_[h]; bhead_[h]=pos;
    }
    bool boundary_active() const { return use_boundary_; }
    std::vector<Match> find(uint32_t pos) const {
        // Boundary-aligned sources supplement the full hash when active (union:
        // strictly more candidates -> never a ratio regression, aligned sources win
        // via the cost model's near-distance preference).
        std::vector<Match> out;
        if (use_boundary_) {
            uint32_t h = hash4(d_.data() + pos);
            out = walk(d_.data(), d_.size(), pos, bhead_[h], bprev_, max_chain_, max_match_);
        }
        uint32_t h = hash4(d_.data() + pos);
        std::vector<Match> full = walk(d_.data(), d_.size(), pos, head_[h], prev_, max_chain_, max_match_);
        if (out.empty()) return full;
        out.insert(out.end(), full.begin(), full.end());
        if (out.size() > 8) {
            std::sort(out.begin(), out.end(), [](auto&a, auto&b){ if(a.len!=b.len)return a.len>b.len; return a.dist<b.dist; });
            out.resize(8);
        }
        return out;
    }
    // Approximate candidate: same first-4-byte hash bucket, then scan forward
    // allowing mismatches. Tracks the best prefix by an MDL-ish score
    //   score(prefix) = len*avg_lit - len/8 - sum(litcost[correction])
    // i.e. mask+residuals vs the all-literal fallback. Returns the best sparse
    // candidate with >=1 correction, or false. `work` caps scanned bytes.
    bool find_sparse(uint32_t pos, SparseMatch& out, const std::array<double,256>& litcost,
                     double avg_lit, uint64_t& work, double dead_band=32.0) const {
        out.len = 0;
        if (pos + 4 > d_.size()) return false;
        const uint8_t* tgt = d_.data() + pos;
        uint32_t h = hash4(tgt);
        uint32_t q = head_[h];
        uint32_t remain = static_cast<uint32_t>(d_.size() - pos);
        uint32_t cap = std::min({remain, max_match_, kSparseScanMax});
        if (cap < 8) return false;
        const double match_gain = avg_lit - 0.125;   // saved literal minus mask bit
        std::array<uint32_t, kSparseScanMax> boff{};
        std::array<uint8_t, kSparseScanMax> bval{};
        uint32_t blen = 0, bk = 0;
        double best_score = -1e300;
        for (uint32_t depth = 0; q != kNoPos && depth < kSparseChainMax; ++depth, q = prev_[q]) {
            if (q >= pos) break;
            const uint8_t* src = d_.data() + q;
            if (src[0] != tgt[0] || src[1] != tgt[1] || src[2] != tgt[2] || src[3] != tgt[3]) continue;
            const uint32_t dist = pos - q;
            double score = 0.0, local_best = -1e300;
            uint32_t k = 0, local_len = 0, local_k = 0;
            std::array<uint32_t, kSparseScanMax> off{};
            std::array<uint8_t, kSparseScanMax> val{};
            uint32_t j = 0;
            for (; j < cap; ++j) {
                if (++work > kSparseBlockBudget) break;
                // The decoder's overlapping copy produces src[j % dist]; corrections
                // must be computed against that, not against src[j], for dist < len.
                uint8_t cpy = (dist > 0) ? src[j % dist] : src[j];
                if (cpy != tgt[j]) {
                    if (k >= kSparseScanMax) break;
                    off[k] = j; val[k] = tgt[j];
                    score -= litcost[tgt[j]] + 0.125;
                    ++k;
                } else {
                    score += match_gain;
                }
                if (score > local_best) { local_best = score; local_len = j + 1; local_k = k; }
                else if (score < local_best - dead_band) break;  // surprise budget: tolerance to dips
            }
            if (local_k >= 1 && local_len >= 8 && local_best > best_score) {
                best_score = local_best; blen = local_len; bk = local_k;
                for (uint32_t i = 0; i < local_k; ++i) { boff[i] = off[i]; bval[i] = val[i]; }
                out.dist = pos - q;
            }
            if (work >= kSparseBlockBudget) break;
        }
        if (bk >= 1 && blen >= 8) {
            out.len = blen;
            out.off.assign(boff.begin(), boff.begin() + bk);
            out.val.assign(bval.begin(), bval.begin() + bk);
            return true;
        }
        return false;
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

// Single-pass greedy parse over literal / exact-match / sparse-corrected edges.
// At each position the sparse edge is accepted only if its estimated cost
// (token + len/dist varints + L/8 flat mask + residual litcosts) beats the
// best alternative covering the same span (exact edge + literals), per the
// mask-stream cost rule. Conservative by design: the block router arbitrates.
// `surprise` is the mismatch budget (entropy-control variable, swept): it
// scales find_sparse's dead band and the max corrections per sparse candidate.
static std::vector<SparseToken> parse_sparse(const std::vector<uint8_t>& d, uint32_t max_chain, uint32_t max_match,
                                             uint32_t surprise=6, bool boundary=false) {
    const uint32_t n=static_cast<uint32_t>(d.size());
    std::vector<SparseToken> toks;
    if(n==0) return toks;
    std::array<uint32_t,256> hist{}; for(auto b:d) ++hist[b];
    std::array<double,256> litcost{};
    double avg_lit=0.0;
    for(int b=0;b<256;++b) {
        double p=(hist[b]+0.5)/(double(n)+128.0);
        litcost[b]=std::clamp(-std::log2(p),1.0,9.5);
        avg_lit+=litcost[b]*hist[b];
    }
    avg_lit/=double(n);
    std::vector<double> pref(n+1,0.0); // prefix sums of per-byte literal cost
    for(uint32_t i=0;i<n;++i) pref[i+1]=pref[i]+litcost[d[i]]+0.10;

    const double dead_band=32.0*double(surprise)/6.0;
    MatchFinder mf(d,max_chain,max_match,boundary);
    uint64_t work=0;
    uint32_t i=0;
    while(i<n) {
        auto ms=mf.find(i);
        Match exact{0,0};
        for(auto&m:ms) if(m.len>exact.len || (m.len==exact.len && m.dist<exact.dist)) exact=m;
        double exact_c=std::numeric_limits<double>::infinity();
        if(exact.len>=4) exact_c=0.6+varint_cost(exact.len-4)+varint_cost(exact.dist-1)+0.18*std::log2(double(exact.dist)+1.0);
        if(exact.len<128 && work<kSparseBlockBudget) {
            SparseMatch sm;
            if(mf.find_sparse(i,sm,litcost,avg_lit,work,dead_band)) {
                double sparse_c=1.5+varint_cost(sm.len-4)+varint_cost(sm.dist-1)+double(sm.len)/8.0
                               +0.18*std::log2(double(sm.dist)+1.0);
                for(size_t k=0;k<sm.off.size();++k) sparse_c+=litcost[sm.val[k]];
                double alt_c=std::numeric_limits<double>::infinity();
                if(exact.len>=4) alt_c=exact_c+(pref[i+sm.len]-pref[i+std::min<uint32_t>(exact.len,sm.len)]);
                else alt_c=pref[i+sm.len]-pref[i];
                if(sparse_c<alt_c) {
                    SparseToken t; t.type=2; t.pos=i; t.len=sm.len; t.dist=sm.dist;
                    t.off=std::move(sm.off); t.val=std::move(sm.val);
                    toks.push_back(std::move(t));
                    if(boundary) mf.insert_boundary(i);
                    uint32_t end=i+sm.len;
                    for(uint32_t p=i;p<end;++p) mf.insert(p);
                    i=end;
                    continue;
                }
            }
        }
        // A match must beat the literal cost of the SAME span it covers, not one byte.
        if(exact.len>=4 && exact_c<(pref[i+exact.len]-pref[i])) {
            SparseToken t; t.type=1; t.pos=i; t.len=exact.len; t.dist=exact.dist;
            toks.push_back(std::move(t));
            if(boundary) mf.insert_boundary(i);
            uint32_t end=i+exact.len;
            for(uint32_t p=i;p<end;++p) mf.insert(p);
            i=end;
        } else {
            if(!toks.empty() && toks.back().type==0 && toks.back().pos+toks.back().len==i) ++toks.back().len;
            else { toks.push_back(SparseToken{0,i,1,0,{}, {}}); if(boundary) mf.insert_boundary(i); }
            mf.insert(i);
            ++i;
        }
    }
    return toks;
}

static ParseStats token_stats(const std::vector<Token>& t) {
    ParseStats s; s.tokens=t.size();
    for(auto&x:t) if(x.match){++s.matches;s.matched_bytes+=x.len;} else s.literals+=x.len;
    return s;
}

// ---- Precision/work-adaptive entropy (t2-entropy) ---------------------------
// Stream suite: each substream picks the cheapest codec under
//   J = L + lambda * C_decode * L     (C_decode = per-byte decode cost units)
// Codecs: 0 raw, 1 rANS-4096 (existing wire), 2 rANS-512, 3 rANS-256,
//         4 canonical Huffman, 5 default-with-exceptions.
struct RansSpec { uint32_t scale_bits; uint32_t tot; uint32_t L; };
static constexpr RansSpec kRans4096{12, 1u<<12, 1u<<23};
static constexpr RansSpec kRans512 {9,  1u<<9,  1u<<17};
static constexpr RansSpec kRans256 {8,  1u<<8,  1u<<16};

struct RansModel {
    std::array<uint16_t,256> freq{};
    std::array<uint16_t,256> start{};
};

static RansModel build_rans_model(const std::vector<uint8_t>& src, uint32_t tot) {
    RansModel m; if(src.empty()) return m;
    std::array<uint32_t,256> count{}; for(uint8_t b:src)++count[b];
    std::array<double,256> exact{}; uint32_t sum=0;
    for(int i=0;i<256;++i) if(count[i]) {
        exact[i]=double(count[i])*tot/src.size();
        uint32_t f=std::max<uint32_t>(1,static_cast<uint32_t>(std::floor(exact[i])));
        m.freq[i]=static_cast<uint16_t>(f); sum+=f;
    }
    while(sum<tot) {
        int best=-1; double score=-1e100;
        for(int i=0;i<256;++i) if(count[i]) { double sc=exact[i]-m.freq[i]; if(sc>score){score=sc;best=i;} }
        if(best<0) throw std::runtime_error("rANS normalization underflow");
        ++m.freq[best]; ++sum;
    }
    while(sum>tot) {
        int best=-1; double score=-1e100;
        for(int i=0;i<256;++i) if(m.freq[i]>1) { double sc=m.freq[i]-exact[i]; if(sc>score){score=sc;best=i;} }
        if(best<0) throw std::runtime_error("rANS normalization overflow");
        --m.freq[best]; --sum;
    }
    uint32_t st=0; for(int i=0;i<256;++i){m.start[i]=static_cast<uint16_t>(st);st+=m.freq[i];}
    if(st!=tot) throw std::runtime_error("rANS normalization sum");
    return m;
}

static std::vector<uint8_t> rans_encode(const std::vector<uint8_t>& src,const RansModel&m,const RansSpec&sp) {
    if(src.empty())return {};
    uint32_t x=sp.L; std::vector<uint8_t> emitted; emitted.reserve(src.size()/2+16);
    for(size_t ii=src.size();ii-->0;) {
        uint8_t sym=src[ii]; uint32_t f=m.freq[sym], st=m.start[sym];
        uint32_t x_max=((sp.L>>sp.scale_bits)<<8)*f;
        while(x>=x_max){emitted.push_back(static_cast<uint8_t>(x));x>>=8;}
        x=((x/f)<<sp.scale_bits)+(x%f)+st;
    }
    std::vector<uint8_t> out(4);
    out[0]=static_cast<uint8_t>(x); out[1]=static_cast<uint8_t>(x>>8); out[2]=static_cast<uint8_t>(x>>16); out[3]=static_cast<uint8_t>(x>>24);
    out.reserve(4+emitted.size());
    for(auto it=emitted.rbegin();it!=emitted.rend();++it)out.push_back(*it);
    return out;
}

static std::vector<uint8_t> rans_decode(const uint8_t* p,size_t n,size_t out_n,const RansModel&m,const RansSpec&sp) {
    if(out_n==0)return {};
    if(n<4)throw std::runtime_error("truncated rANS state");
    const uint8_t* q=p; const uint8_t* e=p+n; uint32_t x=get_u32le(q,e);
    std::vector<uint8_t> symtab(sp.tot);
    for(int s=0;s<256;++s) if(m.freq[s]) for(uint32_t j=0;j<m.freq[s];++j)symtab[m.start[s]+j]=static_cast<uint8_t>(s);
    std::vector<uint8_t> out(out_n);
    for(size_t i=0;i<out_n;++i) {
        uint32_t slot=x&(sp.tot-1); uint8_t sym=symtab[slot]; out[i]=sym;
        x=uint32_t(m.freq[sym])*(x>>sp.scale_bits)+slot-m.start[sym];
        while(x<sp.L){ if(q>=e)throw std::runtime_error("truncated rANS renorm"); x=(x<<8)|*q++; }
    }
    if(q!=e)throw std::runtime_error("trailing rANS bytes");
    return out;
}

// Canonical Huffman (stream mode 4). Code lengths transmitted as 256 bytes.
static std::vector<uint8_t> huffman_encode(const std::vector<uint8_t>& src, const std::array<uint8_t,256>& len) {
    // canonical codes: symbols sorted by (len, sym); code increments per symbol
    std::array<uint8_t,256> order{};
    size_t cnt=0;
    for(int s=0;s<256;++s) if(len[s]) order[cnt++]=uint8_t(s);
    std::sort(order.begin(), order.begin()+cnt, [&](uint8_t a, uint8_t b){ return len[a]!=len[b] ? len[a]<len[b] : a<b; });
    std::array<uint32_t,256> code{};
    uint32_t c=0, clen=0;
    for(size_t i=0;i<cnt;++i){ while(clen<len[order[i]]){ c<<=1; ++clen; } code[order[i]]=c++; }
    std::vector<uint8_t> out; out.reserve(src.size()+16);
    uint64_t acc=0; int nbits=0;
    for(uint8_t sym:src) {
        uint32_t cd=code[sym]; int l=len[sym];
        for(int b=l-1;b>=0;--b){ acc=(acc<<1)|((cd>>b)&1); if(++nbits==64){ for(int k=7;k>=0;--k)out.push_back(uint8_t(acc>>(8*k))); nbits=0; acc=0; } }
    }
    if(nbits){ acc<<=(64-nbits); int bytes=(nbits+7)/8; for(int k=0;k<bytes;++k)out.push_back(uint8_t(acc>>(64-8*(k+1)))); }
    return out;
}

struct HuffModel {
    std::array<uint8_t,256> len{};
    // canonical decode state
    std::array<uint16_t,256> first_code{}; // first code of each length (as a bit-reversed? no: canonical, read MSB-first)
    std::array<uint16_t,256> first_sym{};  // first symbol index of each length
    std::array<uint16_t,256> n_codes{};
    std::array<uint8_t,256> order{};
    uint8_t max_len=0, n_ord=0;
};

static HuffModel build_huff_model(const std::array<uint8_t,256>& len) {
    HuffModel h; h.len=len;
    std::array<uint8_t,256> syms{};
    uint32_t cnt=0;
    for(int s=0;s<256;++s) if(len[s]) syms[cnt++]=uint8_t(s);
    std::sort(syms.begin(), syms.begin()+cnt, [&](uint8_t a,uint8_t b){ return len[a]!=len[b] ? len[a]<len[b] : a<b; });
    h.n_ord=uint8_t(cnt); for(uint32_t i=0;i<cnt;++i) h.order[i]=syms[i];
    std::array<uint32_t,256> code{};
    uint32_t c=0, clen=0;
    for(uint32_t i=0;i<cnt;++i){ while(clen<len[syms[i]]){ c<<=1; ++clen; } code[syms[i]]=c++; if(len[syms[i]]>h.max_len) h.max_len=len[syms[i]]; }
    uint32_t cur=0;
    for(int l=1;l<=24;++l){
        while(cur<cnt && len[syms[cur]]<l) ++cur;
        if(cur<cnt && len[syms[cur]]==l){ h.first_code[l]=uint16_t(code[syms[cur]]); h.first_sym[l]=uint16_t(cur); uint32_t k=cur; while(k<cnt && len[syms[k]]==l) ++k; h.n_codes[l]=uint16_t(k-cur); }
    }
    return h;
}

static std::vector<uint8_t> huffman_decode(const uint8_t* p, size_t n, size_t out_n, const HuffModel& h) {
    std::vector<uint8_t> out(out_n);
    const uint8_t* e=p+n;
    uint64_t acc=0; int have=0;
    auto refill=[&](int need){ while(have<need && p<e){ acc=(acc<<8)|*p++; have+=8; } };
    for(size_t i=0;i<out_n;++i){
        if (h.max_len <= 12) {
            refill(12);
            int win = have; if (win > 12) win = 12;
            uint32_t code=(uint32_t)((acc>>(have-win)) & ((1u<<win)-1));
            uint8_t sym=0; int used=-1;
            for(int l=1;l<=win;++l){
                if(h.n_codes[l]){
                    uint32_t sh=win-l;
                    uint32_t cand=(code>>sh);
                    if(cand>=h.first_code[l] && cand<h.first_code[l]+h.n_codes[l]){ sym=h.order[h.first_sym[l]+(cand-h.first_code[l])]; used=l; break; }
                }
            }
            if(used<0) throw std::runtime_error("invalid huffman code");
            have-=used; out[i]=sym;
        } else {
            refill(1);
            uint32_t code=0; uint8_t sym=0; bool found=false;
            for(int l=1;l<=24;++l){
                if(have<1) throw std::runtime_error("truncated huffman bits");
                code=(code<<1)|((uint32_t)((acc>>(have-1))&1));
                --have; refill(1);
                if(h.n_codes[l] && code>=h.first_code[l] && code<h.first_code[l]+h.n_codes[l]){ sym=h.order[h.first_sym[l]+(code-h.first_code[l])]; found=true; break; }
            }
            if(!found) throw std::runtime_error("invalid huffman code");
            out[i]=sym;
        }
    }
    return out;
}

static std::vector<uint8_t> defexc_decode(const uint8_t* p, size_t n, size_t out_n, uint8_t def) {
    // format: mode 5, uvarint raw_n, byte default, uvarint nexc, ceil(n/8) mask bytes, nexc value bytes
    const uint8_t* e=p+n;
    if(p>=e) throw std::runtime_error("truncated defexc");
    uint64_t nexc=get_uvar(p,e);
    uint64_t mask_bytes=(out_n+7)/8;
    if(mask_bytes>uint64_t(e-p)) throw std::runtime_error("truncated defexc mask");
    if(nexc>uint64_t(e-p)-mask_bytes) throw std::runtime_error("truncated defexc values");
    const uint8_t* mask=p; p+=mask_bytes;
    std::vector<uint8_t> out(out_n);
    for(size_t i=0;i<out_n;++i){
        if((mask[i>>3]>>(i&7))&1){ out[i]=*p++; }
        else out[i]=def;
    }
    if(p!=e) throw std::runtime_error("trailing defexc bytes");
    return out;
}

// ---- stream-suite selection ------------------------------------------------
// J = L + lambda * C_decode * L with per-byte decode cost units:
//   raw 1, rans-4096 4, rans-512 3.5, rans-256 3, huffman 2.2, defexc 2.0
static double g_stream_lambda = 0.04; // ANVIL_STREAM_LAMBDA overrides (0 = pure length)
static bool g_stream_suite = true;   // false = fixed rANS-4096 + raw (pre-suite behavior)
static uint64_t g_j_agree = 0, g_j_total = 0; // J-selection vs pure-L agreement counters

static std::vector<uint8_t> rans_stream_bytes(const std::vector<uint8_t>& src, const RansSpec& sp, uint8_t mode) {
    RansModel m=build_rans_model(src,sp.tot); auto rd=rans_encode(src,m,sp);
    std::vector<uint8_t> z; z.push_back(mode); put_uvar(z,src.size());
    uint32_t nz=0; for(auto f:m.freq) if(f) ++nz; put_uvar(z,nz);
    for(int i=0;i<256;++i) if(m.freq[i]) { z.push_back(uint8_t(i)); put_uvar(z,m.freq[i]); }
    put_uvar(z,rd.size()); z.insert(z.end(),rd.begin(),rd.end());
    return z;
}

static std::vector<uint8_t> huffman_stream_bytes(const std::vector<uint8_t>& src, const std::array<uint8_t,256>& len) {
    std::vector<uint8_t> z; z.push_back(4); put_uvar(z,src.size());
    for(int i=0;i<256;++i) z.push_back(len[i]);
    auto bits=huffman_encode(src,len); put_uvar(z,bits.size()); z.insert(z.end(),bits.begin(),bits.end());
    return z;
}

static std::vector<uint8_t> defexc_stream_bytes(const std::vector<uint8_t>& src, uint8_t def) {
    std::vector<uint8_t> z; z.push_back(5); put_uvar(z,src.size()); z.push_back(def);
    std::vector<uint8_t> mask((src.size()+7)/8, 0); std::vector<uint8_t> vals;
    for(size_t i=0;i<src.size();++i) if(src[i]!=def){ mask[i>>3]|=uint8_t(1u<<(i&7)); vals.push_back(src[i]); }
    put_uvar(z,vals.size()); z.insert(z.end(),mask.begin(),mask.end()); z.insert(z.end(),vals.begin(),vals.end());
    return z;
}

// Build code lengths for canonical Huffman (bottom-up tree, O(256 log 256)).
static std::array<uint8_t,256> huffman_lengths(const std::vector<uint8_t>& src) {
    std::array<uint32_t,256> cnt{}; for(uint8_t b:src) ++cnt[b];
    std::array<uint8_t,256> len{};
    uint32_t alive=0; for(int i=0;i<256;++i) if(cnt[i]) ++alive;
    if(alive==0) return len;
    if(alive==1){ for(int i=0;i<256;++i) if(cnt[i]) len[i]=1; return len; }
    struct Node { uint32_t freq; int left, right; int sym; }; // sym >= 0 leaf
    std::vector<Node> nodes; nodes.reserve(2*alive);
    for(int i=0;i<256;++i) if(cnt[i]) nodes.push_back({cnt[i],-1,-1,i});
    std::vector<std::pair<int,int>> heap2; // (freq, node index)
    for(size_t i=0;i<nodes.size();++i) heap2.push_back({int(nodes[i].freq),int(i)});
    auto lt=[](auto&a,auto&b){ return a.first>b.first; };
    std::make_heap(heap2.begin(),heap2.end(),lt);
    while(heap2.size()>1){
        std::pop_heap(heap2.begin(),heap2.end(),lt); auto a=heap2.back(); heap2.pop_back();
        std::pop_heap(heap2.begin(),heap2.end(),lt); auto b=heap2.back(); heap2.pop_back();
        int nn=int(nodes.size()); nodes.push_back({uint32_t(a.first+b.first),a.second,b.second,-1});
        heap2.push_back({int(nodes[nn].freq),nn}); std::push_heap(heap2.begin(),heap2.end(),lt);
    }
    std::function<void(int,int)> walk=[&](int nd,int depth){
        if(nodes[nd].sym>=0){ len[nodes[nd].sym]=uint8_t(depth); return; }
        walk(nodes[nd].left,depth+1); walk(nodes[nd].right,depth+1);
    };
    walk(heap2[0].second,0);
    return len;
}

static std::vector<uint8_t> encode_stream(const std::vector<uint8_t>& src) {
    std::vector<uint8_t> raw; raw.push_back(0); put_uvar(raw,src.size()); raw.insert(raw.end(),src.begin(),src.end());
    if(src.size()<16) return raw;
    struct Cand { std::vector<uint8_t> bytes; double J; };
    std::vector<Cand> cands;
    auto add=[&](std::vector<uint8_t> b, double cu){ double L=double(b.size()); cands.push_back({std::move(b), L + g_stream_lambda*cu*L}); };
    add(rans_stream_bytes(src,kRans4096,1), 4.0);
    if(g_stream_suite) {
        add(rans_stream_bytes(src,kRans512,2), 3.5);
        add(rans_stream_bytes(src,kRans256,3), 3.0);
        auto hlen=huffman_lengths(src);
        add(huffman_stream_bytes(src,hlen), 2.2);
        {
            std::array<uint32_t,256> cnt{}; for(uint8_t b:src) ++cnt[b];
            uint8_t def=0; for(int i=1;i<256;++i) if(cnt[i]>cnt[def]) def=uint8_t(i);
            if(cnt[def]>=src.size()/2) add(defexc_stream_bytes(src,def), 2.0);
        }
    }
    const Cand* best=&cands[0];
    for(auto& c:cands) if(c.J<best->J) best=&c;
    // J-prediction accounting: ratio-faithfulness = J-winner's length within 1% of
    // the smallest-length codec (the decode-cost term must not mis-pick badly).
    if(g_stream_suite && cands.size()>1) {
        size_t lw=0; for(size_t i=1;i<cands.size();++i) if(cands[i].bytes.size()<cands[lw].bytes.size()) lw=i;
        ++g_j_total;
        if(best->bytes.size() <= cands[lw].bytes.size()*101/100) ++g_j_agree;
    }
    return best->bytes;
}

static std::vector<uint8_t> decode_stream(const uint8_t*&p,const uint8_t*e, size_t max_n) {
    if(p>=e) throw std::runtime_error("truncated stream header");
    uint8_t mode=*p++; uint64_t raw_n=get_uvar(p,e);
    if(raw_n>max_n)throw std::runtime_error("stream too large"); // DoS guard: bound by block out_len
    if(mode==0){if(raw_n>uint64_t(e-p))throw std::runtime_error("truncated raw stream");std::vector<uint8_t>o(p,p+raw_n);p+=raw_n;return o;}
    if(mode>=1 && mode<=3) {
        const RansSpec* sp = mode==1 ? &kRans4096 : mode==2 ? &kRans512 : &kRans256;
        uint64_t nz=get_uvar(p,e); if(nz>256)throw std::runtime_error("bad rANS model"); RansModel m; uint32_t sum=0;
        for(uint64_t k=0;k<nz;++k){if(p>=e)throw std::runtime_error("truncated rANS model");uint8_t sym=*p++;uint64_t f=get_uvar(p,e);if(f==0||f>sp->tot||m.freq[sym])throw std::runtime_error("bad rANS frequency");m.freq[sym]=static_cast<uint16_t>(f);sum+=f;}
        if(sum!=sp->tot) throw std::runtime_error("bad rANS total");
        uint32_t st=0;for(int i=0;i<256;++i){m.start[i]=static_cast<uint16_t>(st);st+=m.freq[i];}
        uint64_t dn=get_uvar(p,e);if(dn>uint64_t(e-p))throw std::runtime_error("truncated rANS stream");auto out=rans_decode(p,static_cast<size_t>(dn),static_cast<size_t>(raw_n),m,*sp);p+=dn;return out;
    }
    if(mode==4) {
        if(uint64_t(e-p)<256) throw std::runtime_error("truncated huffman lengths");
        std::array<uint8_t,256> len{}; for(int i=0;i<256;++i) len[i]=*p++;
        // validate Kraft inequality
        uint64_t kraft=0; for(int i=0;i<256;++i) if(len[i]) { if(len[i]>24) throw std::runtime_error("bad huffman length"); kraft += 1ull<<(24-len[i]); }
        if(kraft> (1ull<<24)) throw std::runtime_error("huffman overfull");
        HuffModel h=build_huff_model(len);
        uint64_t dn=get_uvar(p,e); if(dn>uint64_t(e-p)) throw std::runtime_error("truncated huffman stream");
        auto out=huffman_decode(p,static_cast<size_t>(dn),static_cast<size_t>(raw_n),h); p+=dn; return out;
    }
    if(mode==5) {
        if(p>=e) throw std::runtime_error("truncated defexc default");
        uint8_t def=*p++;
        auto out=defexc_decode(p,uint64_t(e-p),static_cast<size_t>(raw_n),def);
        p=e; return out;
    }
    throw std::runtime_error("unknown stream codec");
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
    const size_t max_sub=16*out_len+64; // provable per-substream bound: varints(<=10B)*tokens(<=out_len) + literals
    for(int i=0;i<5;++i){uint64_t zn=get_uvar(p,e);if(zn>uint64_t(e-p))throw std::runtime_error("truncated substream");const uint8_t*q=p;const uint8_t*qe=p+zn;s[i]=decode_stream(q,qe,max_sub);if(q!=qe)throw std::runtime_error("substream trailing bytes");p+=zn;}
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

// ---- SPARSE-REF backend (mode 11) -----------------------------------------
// Seven separated streams, each serialized via encode_stream (raw or static
// order-0 rANS, chosen per stream):
//   S0 token types: 0 literal run, 1 exact match, 2 sparse-corrected match
//   S1 literal-run length, uvarint(len-1)
//   S2 match length, uvarint(len-4)         [types 1,2]
//   S3 match distance, uvarint(dist-1)      [types 1,2]
//   S4 literal bytes
//   S5 correction masks: per sparse token, ceil(len/32) little-endian 32-bit
//      words (bit j of word w covers byte 32w+j of the phrase)
//   S6 residual bytes, popcount(mask) per sparse token, in mask order
// Decode of a sparse token: base = out.size()-dist; copy len bytes from base
// (overlap allowed) THEN apply residual bytes at base+offset for each set mask
// bit. len(residuals) == popcount(mask) is enforced strictly.

static std::vector<uint8_t> encode_tokens_sparse(const std::vector<uint8_t>& d, const std::vector<SparseToken>& toks) {
    std::vector<uint8_t> types, ll, ml, ds, lits, masks, resid;
    types.reserve(toks.size());
    std::array<uint32_t,(kSparseScanMax+31)/32> words{};
    for(auto&t:toks) {
        types.push_back(t.type);
        if(t.type==0) {
            append_varint_bytes(ll,t.len-1);
            lits.insert(lits.end(),d.begin()+t.pos,d.begin()+t.pos+t.len);
        } else if(t.type==1) {
            append_varint_bytes(ml,t.len-4);
            append_varint_bytes(ds,t.dist-1);
        } else {
            append_varint_bytes(ml,t.len-4);
            append_varint_bytes(ds,t.dist-1);
            std::fill(words.begin(),words.end(),0u);
            for(size_t k=0;k<t.off.size();++k) {
                words[t.off[k]/32]|=(1u<<(t.off[k]%32));
                resid.push_back(t.val[k]);
            }
            uint32_t nwords=(t.len+31)/32;
            for(uint32_t w=0;w<nwords;++w) {
                uint32_t m=words[w];
                masks.push_back(static_cast<uint8_t>(m));
                masks.push_back(static_cast<uint8_t>(m>>8));
                masks.push_back(static_cast<uint8_t>(m>>16));
                masks.push_back(static_cast<uint8_t>(m>>24));
            }
        }
    }
    std::vector<uint8_t> out;
    for(const auto* v:{&types,&ll,&ml,&ds,&lits,&masks,&resid}) {
        auto z=encode_stream(*v);
        put_uvar(out,z.size());
        out.insert(out.end(),z.begin(),z.end());
    }
    return out;
}

static std::vector<uint8_t> decode_tokens_sparse(const uint8_t* p, size_t n, size_t out_len) {
    const uint8_t* e=p+n;
    std::array<std::vector<uint8_t>,7> s;
    const size_t max_sub=16*out_len+64; // masks <= out_len/8+4/token, residuals <= out_len, varints <= 10/token
    for(int i=0;i<7;++i) {
        uint64_t zn=get_uvar(p,e);
        if(zn>uint64_t(e-p)) throw std::runtime_error("truncated substream");
        const uint8_t* q=p; const uint8_t* qe=p+zn;
        s[i]=decode_stream(q,qe,max_sub);
        if(q!=qe) throw std::runtime_error("substream trailing bytes");
        p+=zn;
    }
    if(p!=e) throw std::runtime_error("payload trailing bytes");
    size_t ip_ll=0,ip_ml=0,ip_ds=0,ip_lit=0,ip_mask=0,ip_res=0;
    std::vector<uint8_t> out; out.reserve(out_len);
    for(uint8_t type:s[0]) {
        if(out.size()>=out_len) throw std::runtime_error("too many tokens");
        if(type==0) {
            uint64_t len=read_varint_bytes(s[1],ip_ll)+1;
            if(len>out_len-out.size()||len>s[4].size()-ip_lit) throw std::runtime_error("bad literal run");
            out.insert(out.end(),s[4].begin()+ip_lit,s[4].begin()+ip_lit+len);
            ip_lit+=len;
        } else if(type==1) {
            uint64_t len=read_varint_bytes(s[2],ip_ml)+4;
            uint64_t dist=read_varint_bytes(s[3],ip_ds)+1;
            if(dist==0||dist>out.size()||len>out_len-out.size()) throw std::runtime_error("bad exact match");
            for(uint64_t k=0;k<len;++k) out.push_back(out[out.size()-dist]);
        } else if(type==2) {
            uint64_t len=read_varint_bytes(s[2],ip_ml)+4;
            uint64_t dist=read_varint_bytes(s[3],ip_ds)+1;
            if(dist==0||dist>out.size()||len>out_len-out.size()) throw std::runtime_error("bad sparse match");
            if(len>kSparseMaxLen) throw std::runtime_error("sparse match too long");
            uint64_t nwords=(len+31)/32;
            if(nwords*4>s[5].size()-ip_mask) throw std::runtime_error("truncated mask stream");
            std::array<uint32_t,(kSparseMaxLen+31)/32> words{};
            uint32_t pc=0;
            for(uint64_t w=0;w<nwords;++w) {
                uint32_t m=uint32_t(s[5][ip_mask])|(uint32_t(s[5][ip_mask+1])<<8)
                          |(uint32_t(s[5][ip_mask+2])<<16)|(uint32_t(s[5][ip_mask+3])<<24);
                ip_mask+=4;
                uint32_t first=uint32_t(w*32);
                if(first+32>len) { uint32_t over=first+32-len; if((m>>(32-over))!=0) throw std::runtime_error("mask bits beyond copy length"); }
                words[w]=m;
                pc+=std::popcount(m);
            }
            if(pc>s[6].size()-ip_res) throw std::runtime_error("truncated residual stream");
            size_t start=out.size(); // copy destination start (base+dist)
            for(uint64_t k=0;k<len;++k) out.push_back(out[out.size()-dist]); // phrase copy (overlap allowed)
            for(uint64_t w=0;w<nwords;++w) {
                uint32_t m=words[w];
                while(m) {
                    uint32_t b=std::countr_zero(m);
                    out[start+uint32_t(w*32)+b]=s[6][ip_res++];
                    m&=m-1;
                }
            }
        } else throw std::runtime_error("unknown sparse token type");
    }
    if(out.size()!=out_len||ip_ll!=s[1].size()||ip_ml!=s[2].size()||ip_ds!=s[3].size()
       ||ip_lit!=s[4].size()||ip_mask!=s[5].size()||ip_res!=s[6].size())
        throw std::runtime_error("substream consumption mismatch");
    return out;
}

// ---- SHAPE backend (mode 12): shape-book + per-shape displacement ----------
// Semantic shape vocabulary (kind x len-class) compiled into a decoder-side
// instruction book: each match token's shape selects a per-shape displacement
// state, and the distance is coded against that state (first = absolute, then
// reuse-last or signed delta, zigzag). Decoder = stream lookups + a tiny state
// table (LZ-class). num_states is transmitted (1 = generic single state, the
// FLAG-D control; 28 = per-shape (type x 14 len-classes)).
static constexpr uint32_t kShapeClasses = 14;

static inline uint32_t len_class(uint32_t len) { // len >= 4 -> 0..13 (doubling)
    uint32_t cl = 0, v = 4;
    while (len >= v * 2 && cl < kShapeClasses - 1) { v *= 2; ++cl; }
    return cl;
}

static inline uint32_t shape_index(uint8_t type, uint32_t len, uint32_t num_states) {
    if (num_states <= 1) return 0;
    return (type - 1) * kShapeClasses + len_class(len); // types 1,2 -> 0..27
}

static std::vector<uint8_t> encode_tokens_shape(const std::vector<uint8_t>& d, const std::vector<SparseToken>& toks,
                                                uint32_t num_states) {
    std::vector<uint8_t> types, ll, ml, dflags, dvar, lits, masks, resid;
    types.reserve(toks.size());
    std::array<uint32_t, 2 * kShapeClasses> last{};
    std::array<uint32_t,(kSparseScanMax+31)/32> words{};
    for (auto& t : toks) {
        types.push_back(t.type);
        if (t.type == 0) {
            append_varint_bytes(ll, t.len - 1);
            lits.insert(lits.end(), d.begin() + t.pos, d.begin() + t.pos + t.len);
        } else {
            append_varint_bytes(ml, t.len - 4);
            uint32_t shape = shape_index(t.type, t.len, num_states);
            uint32_t& lastd = last[shape];
            if (lastd == 0) {
                dflags.push_back(0);
                append_varint_bytes(dvar, t.dist - 1);
                lastd = t.dist;
            } else if (t.dist == lastd) {
                dflags.push_back(1);
            } else {
                dflags.push_back(2);
                int64_t dlt = int64_t(t.dist) - int64_t(lastd);
                uint64_t zz = dlt >= 0 ? uint64_t(dlt) * 2 : uint64_t(-dlt) * 2 - 1;
                append_varint_bytes(dvar, zz);
                lastd = t.dist;
            }
            if (t.type == 2) {
                std::fill(words.begin(), words.end(), 0u);
                for (size_t k = 0; k < t.off.size(); ++k) {
                    words[t.off[k] / 32] |= (1u << (t.off[k] % 32));
                    resid.push_back(t.val[k]);
                }
                uint32_t nwords = (t.len + 31) / 32;
                for (uint32_t w = 0; w < nwords; ++w) {
                    uint32_t m = words[w];
                    masks.push_back(static_cast<uint8_t>(m));
                    masks.push_back(static_cast<uint8_t>(m >> 8));
                    masks.push_back(static_cast<uint8_t>(m >> 16));
                    masks.push_back(static_cast<uint8_t>(m >> 24));
                }
            }
        }
    }
    std::vector<uint8_t> out;
    out.push_back(static_cast<uint8_t>(num_states));
    for (const auto* v : {&types, &ll, &ml, &dflags, &dvar, &lits, &masks, &resid}) {
        auto z = encode_stream(*v);
        put_uvar(out, z.size());
        out.insert(out.end(), z.begin(), z.end());
    }
    return out;
}

static std::vector<uint8_t> decode_tokens_shape(const uint8_t* p, size_t n, size_t out_len) {
    const uint8_t* e = p + n;
    if (p >= e) throw std::runtime_error("truncated shape header");
    uint32_t num_states = *p++;
    if (num_states != 1 && num_states != 2 * kShapeClasses) throw std::runtime_error("bad shape state count");
    std::array<std::vector<uint8_t>, 8> s;
    const size_t max_sub = 16 * out_len + 64;
    for (int i = 0; i < 8; ++i) {
        uint64_t zn = get_uvar(p, e);
        if (zn > uint64_t(e - p)) throw std::runtime_error("truncated substream");
        const uint8_t* q = p; const uint8_t* qe = p + zn;
        s[i] = decode_stream(q, qe, max_sub);
        if (q != qe) throw std::runtime_error("substream trailing bytes");
        p += zn;
    }
    if (p != e) throw std::runtime_error("payload trailing bytes");
    size_t ip_ll = 0, ip_ml = 0, ip_df = 0, ip_dv = 0, ip_lit = 0, ip_mask = 0, ip_res = 0;
    std::array<uint32_t, 2 * kShapeClasses> last{};
    std::vector<uint8_t> out; out.reserve(out_len);
    for (uint8_t type : s[0]) {
        if (out.size() >= out_len) throw std::runtime_error("too many tokens");
        if (type == 0) {
            uint64_t len = read_varint_bytes(s[1], ip_ll) + 1;
            if (len > out_len - out.size() || len > s[5].size() - ip_lit) throw std::runtime_error("bad literal run");
            out.insert(out.end(), s[5].begin() + ip_lit, s[5].begin() + ip_lit + len);
            ip_lit += len;
        } else if (type == 1 || type == 2) {
            uint64_t len = read_varint_bytes(s[2], ip_ml) + 4;
            if (len > kSparseMaxLen || len > out_len - out.size()) throw std::runtime_error("bad shape match");
            if (ip_df >= s[3].size()) throw std::runtime_error("truncated dist flags");
            uint8_t flag = s[3][ip_df++];
            uint32_t shape = shape_index(type, static_cast<uint32_t>(len), num_states);
            uint32_t& lastd = last[shape];
            uint32_t dist;
            if (flag == 0) {
                uint64_t dv = read_varint_bytes(s[4], ip_dv);
                if (dv >= 0xFFFFFFFFull) throw std::runtime_error("bad absolute distance");
                dist = static_cast<uint32_t>(dv) + 1; // dv = dist-1, dist in [1, 2^32)
            } else if (flag == 1) {
                if (lastd == 0) throw std::runtime_error("dist reuse before first absolute");
                dist = lastd;
            } else if (flag == 2) {
                if (lastd == 0) throw std::runtime_error("dist delta before first absolute");
                uint64_t zz = read_varint_bytes(s[4], ip_dv);
                int64_t dlt = (zz & 1) ? -int64_t((zz + 1) >> 1) : int64_t(zz >> 1);
                int64_t dd = int64_t(lastd) + dlt;
                if (dd <= 0 || dd > 0xFFFFFFFFll) throw std::runtime_error("bad distance delta");
                dist = static_cast<uint32_t>(dd);
            } else throw std::runtime_error("bad dist flag");
            if (dist == 0 || dist > out.size()) throw std::runtime_error("invalid shape distance");
            lastd = dist;
            for (uint64_t k = 0; k < len; ++k) out.push_back(out[out.size() - dist]); // copy (overlap allowed)
            if (type == 2) {
                uint64_t nwords = (len + 31) / 32;
                if (nwords * 4 > s[6].size() - ip_mask) throw std::runtime_error("truncated mask stream");
                std::array<uint32_t,(kSparseMaxLen+31)/32> words{};
                uint32_t pc = 0;
                for (uint64_t w = 0; w < nwords; ++w) {
                    uint32_t m = uint32_t(s[6][ip_mask]) | (uint32_t(s[6][ip_mask+1]) << 8)
                              | (uint32_t(s[6][ip_mask+2]) << 16) | (uint32_t(s[6][ip_mask+3]) << 24);
                    ip_mask += 4;
                    uint32_t first = uint32_t(w * 32);
                    if (first + 32 > len) { uint32_t over = first + 32 - len; if ((m >> (32 - over)) != 0) throw std::runtime_error("mask bits beyond copy length"); }
                    words[w] = m;
                    pc += std::popcount(m);
                }
                if (pc > s[7].size() - ip_res) throw std::runtime_error("truncated residual stream");
                size_t start = out.size() - len; // copy destination start
                for (uint64_t w = 0; w < nwords; ++w) {
                    uint32_t m = words[w];
                    while (m) {
                        uint32_t b = std::countr_zero(m);
                        out[start + uint32_t(w * 32) + b] = s[7][ip_res++];
                        m &= m - 1;
                    }
                }
            }
        } else throw std::runtime_error("unknown shape token type");
    }
    if (out.size() != out_len || ip_ll != s[1].size() || ip_ml != s[2].size() || ip_df != s[3].size()
       || ip_dv != s[4].size() || ip_lit != s[5].size() || ip_mask != s[6].size() || ip_res != s[7].size())
        throw std::runtime_error("substream consumption mismatch");
    return out;
}

// ---- TOPOLOGY backend (mode 13): per-slot modal residual + exception mask -----
// R2 correction-topology coding. Same token/dist coding as mode 12 (shape
// per-shape displacement), but sparse residuals are coded against a per-slot
// modal value: context = (k, slot) where k = correction count and slot = the
// correction's index within the token's mask. The modal table (context ->
// most common value, for contexts with >= 2 observations) is transmitted once.
// Per type-2 token, an exception mask (ceil(k/8) bytes, bit j = exception for
// correction j) marks which residuals differ from the modal; only exceptions
// are coded in the residual stream. Corrections are identical, so topology
// coding is pure ratio win when residuals recur (Linux: modal accuracy 86.5%).
static std::vector<uint8_t> encode_tokens_topology(const std::vector<uint8_t>& d, const std::vector<SparseToken>& toks,
                                                   uint32_t num_states) {
    // Pass 1: count (k, slot) -> value to find modal residuals.
    std::map<std::pair<uint8_t,uint8_t>, std::array<uint32_t,256>> hist; // (k, slot) -> value counts
    std::array<std::array<uint8_t,64>,64> modal{};
    std::array<std::array<bool,64>,64> has_modal{};
    for (auto& t : toks) {
        if (t.type == 2 && t.off.size() <= 64)
            for (uint32_t j = 0; j < t.off.size(); ++j) ++hist[{uint8_t(t.off.size()), uint8_t(j)}][t.val[j]];
    }
    for (auto& [ks, counts] : hist) {
        uint32_t tot = 0; uint8_t best = 0; uint32_t bcnt = 0;
        for (int v = 0; v < 256; ++v) { tot += counts[v]; if (counts[v] > bcnt) { bcnt = counts[v]; best = uint8_t(v); } }
        if (tot >= 2) { modal[ks.first][ks.second] = best; has_modal[ks.first][ks.second] = true; }
    }
    // Serialize the modal table.
    std::vector<uint8_t> mtab;
    uint32_t mcount = 0;
    for (auto& [ks, c] : hist) if (has_modal[ks.first][ks.second]) ++mcount;
    append_varint_bytes(mtab, mcount);
    for (auto& [ks, c] : hist)
        if (has_modal[ks.first][ks.second]) { mtab.push_back(ks.first); mtab.push_back(ks.second); mtab.push_back(modal[ks.first][ks.second]); }

    // Pass 2: encode tokens.
    std::vector<uint8_t> types, ll, ml, dflags, dvar, lits, masks, exc, resid;
    types.reserve(toks.size());
    std::array<uint32_t, 2 * kShapeClasses> last{};
    std::array<uint32_t,(kSparseScanMax+31)/32> words{};
    for (auto& t : toks) {
        types.push_back(t.type);
        if (t.type == 0) {
            append_varint_bytes(ll, t.len - 1);
            lits.insert(lits.end(), d.begin() + t.pos, d.begin() + t.pos + t.len);
        } else {
            append_varint_bytes(ml, t.len - 4);
            uint32_t shape = shape_index(t.type, t.len, num_states);
            uint32_t& lastd = last[shape];
            if (lastd == 0) { dflags.push_back(0); append_varint_bytes(dvar, t.dist - 1); lastd = t.dist; }
            else if (t.dist == lastd) dflags.push_back(1);
            else { dflags.push_back(2); int64_t dlt = int64_t(t.dist) - int64_t(lastd); uint64_t zz = dlt >= 0 ? uint64_t(dlt) * 2 : uint64_t(-dlt) * 2 - 1; append_varint_bytes(dvar, zz); lastd = t.dist; }
            if (t.type == 2) {
                std::fill(words.begin(), words.end(), 0u);
                std::vector<uint8_t> excbits((t.off.size() + 7) / 8, 0);
                for (size_t k = 0; k < t.off.size(); ++k) {
                    words[t.off[k] / 32] |= (1u << (t.off[k] % 32));
                    bool m = (t.off.size() <= 64 && has_modal[t.off.size()][k] && modal[t.off.size()][k] == t.val[k]);
                    if (!m) { excbits[k / 8] |= uint8_t(1u << (k % 8)); resid.push_back(t.val[k]); }
                }
                exc.insert(exc.end(), excbits.begin(), excbits.end());
                uint32_t nwords = (t.len + 31) / 32;
                for (uint32_t w = 0; w < nwords; ++w) {
                    uint32_t m = words[w];
                    masks.push_back(static_cast<uint8_t>(m));
                    masks.push_back(static_cast<uint8_t>(m >> 8));
                    masks.push_back(static_cast<uint8_t>(m >> 16));
                    masks.push_back(static_cast<uint8_t>(m >> 24));
                }
            }
        }
    }
    std::vector<uint8_t> out;
    out.push_back(static_cast<uint8_t>(num_states));
    put_uvar(out, mtab.size()); out.insert(out.end(), mtab.begin(), mtab.end());
    for (const auto* v : {&types, &ll, &ml, &dflags, &dvar, &lits, &masks, &exc, &resid}) {
        auto z = encode_stream(*v);
        put_uvar(out, z.size());
        out.insert(out.end(), z.begin(), z.end());
    }
    return out;
}

static std::vector<uint8_t> decode_tokens_topology(const uint8_t* p, size_t n, size_t out_len) {
    const uint8_t* e = p + n;
    if (p >= e) throw std::runtime_error("truncated topology header");
    uint32_t num_states = *p++;
    if (num_states != 1 && num_states != 2 * kShapeClasses) throw std::runtime_error("bad shape state count");
    uint64_t mtlen = get_uvar(p, e);
    if (mtlen > uint64_t(e - p)) throw std::runtime_error("truncated modal table");
    const uint8_t* mt = p; p += mtlen;
    std::array<std::array<uint8_t,64>,64> modal{};
    std::array<std::array<bool,64>,64> has_modal{};
    {
        const uint8_t* q = mt; const uint8_t* qe = mt + mtlen;
        uint64_t mcount = get_uvar(q, qe);
        if (mcount > 4096) throw std::runtime_error("bad modal count");
        for (uint64_t i = 0; i < mcount; ++i) {
            if (qe - q < 3) throw std::runtime_error("truncated modal entry");
            uint8_t k = *q++, j = *q++, v = *q++;
            if (k == 0 || k > 64 || j >= k) throw std::runtime_error("bad modal context");
            modal[k][j] = v; has_modal[k][j] = true;
        }
        if (q != qe) throw std::runtime_error("modal table trailing bytes");
    }
    std::array<std::vector<uint8_t>, 9> s;
    const size_t max_sub = 16 * out_len + 64;
    for (int i = 0; i < 9; ++i) {
        uint64_t zn = get_uvar(p, e);
        if (zn > uint64_t(e - p)) throw std::runtime_error("truncated substream");
        const uint8_t* q = p; const uint8_t* qe = p + zn;
        s[i] = decode_stream(q, qe, max_sub);
        if (q != qe) throw std::runtime_error("substream trailing bytes");
        p += zn;
    }
    if (p != e) throw std::runtime_error("payload trailing bytes");
    size_t ip_ll = 0, ip_ml = 0, ip_df = 0, ip_dv = 0, ip_lit = 0, ip_mask = 0, ip_exc = 0, ip_res = 0;
    std::array<uint32_t, 2 * kShapeClasses> last{};
    std::vector<uint8_t> out; out.reserve(out_len);
    for (uint8_t type : s[0]) {
        if (out.size() >= out_len) throw std::runtime_error("too many tokens");
        if (type == 0) {
            uint64_t len = read_varint_bytes(s[1], ip_ll) + 1;
            if (len > out_len - out.size() || len > s[5].size() - ip_lit) throw std::runtime_error("bad literal run");
            out.insert(out.end(), s[5].begin() + ip_lit, s[5].begin() + ip_lit + len);
            ip_lit += len;
        } else if (type == 1 || type == 2) {
            uint64_t len = read_varint_bytes(s[2], ip_ml) + 4;
            if (len > kSparseMaxLen || len > out_len - out.size()) throw std::runtime_error("bad topology match");
            if (ip_df >= s[3].size()) throw std::runtime_error("truncated dist flags");
            uint8_t flag = s[3][ip_df++];
            uint32_t shape = shape_index(type, static_cast<uint32_t>(len), num_states);
            uint32_t& lastd = last[shape];
            uint32_t dist;
            if (flag == 0) { uint64_t dv = read_varint_bytes(s[4], ip_dv); if (dv >= 0xFFFFFFFFull) throw std::runtime_error("bad absolute distance"); dist = static_cast<uint32_t>(dv) + 1; }
            else if (flag == 1) { if (lastd == 0) throw std::runtime_error("dist reuse before first absolute"); dist = lastd; }
            else if (flag == 2) { if (lastd == 0) throw std::runtime_error("dist delta before first absolute"); uint64_t zz = read_varint_bytes(s[4], ip_dv); int64_t dlt = (zz & 1) ? -int64_t((zz + 1) >> 1) : int64_t(zz >> 1); int64_t dd = int64_t(lastd) + dlt; if (dd <= 0 || dd > 0xFFFFFFFFll) throw std::runtime_error("bad distance delta"); dist = static_cast<uint32_t>(dd); }
            else throw std::runtime_error("bad dist flag");
            if (dist == 0 || dist > out.size()) throw std::runtime_error("invalid topology distance");
            lastd = dist;
            size_t start = out.size();
            for (uint64_t k = 0; k < len; ++k) out.push_back(out[out.size() - dist]);
            if (type == 2) {
                uint64_t nwords = (len + 31) / 32;
                if (nwords * 4 > s[6].size() - ip_mask) throw std::runtime_error("truncated mask stream");
                std::array<uint32_t,(kSparseMaxLen+31)/32> words{};
                uint32_t pc = 0;
                for (uint64_t w = 0; w < nwords; ++w) {
                    uint32_t m = uint32_t(s[6][ip_mask]) | (uint32_t(s[6][ip_mask+1]) << 8)
                              | (uint32_t(s[6][ip_mask+2]) << 16) | (uint32_t(s[6][ip_mask+3]) << 24);
                    ip_mask += 4;
                    uint32_t first = uint32_t(w * 32);
                    if (first + 32 > len) { uint32_t over = first + 32 - len; if ((m >> (32 - over)) != 0) throw std::runtime_error("mask bits beyond copy length"); }
                    words[w] = m;
                    pc += std::popcount(m);
                }
                uint64_t excb = (pc + 7) / 8;
                if (excb > s[7].size() - ip_exc) throw std::runtime_error("truncated exception mask");
                uint32_t ex = 0;
                for (uint64_t w = 0; w < nwords; ++w) {
                    uint32_t m = words[w];
                    while (m) {
                        uint32_t b = std::countr_zero(m);
                        uint32_t j = ex++;
                        uint32_t val;
                        uint32_t byte = s[7][ip_exc + j / 8];
                        if ((byte >> (j % 8)) & 1) {
                            if (ip_res >= s[8].size()) throw std::runtime_error("truncated exception residual");
                            val = s[8][ip_res++];
                        } else {
                            if (pc > 64 || j >= 64 || !has_modal[pc][j]) throw std::runtime_error("missing modal residual");
                            val = modal[pc][j];
                        }
                        out[start + uint32_t(w * 32) + b] = uint8_t(val);
                        m &= m - 1;
                    }
                }
                ip_exc += excb;
            }
        } else throw std::runtime_error("unknown topology token type");
    }
    if (out.size() != out_len || ip_ll != s[1].size() || ip_ml != s[2].size() || ip_df != s[3].size()
       || ip_dv != s[4].size() || ip_lit != s[5].size() || ip_mask != s[6].size() || ip_exc != s[7].size() || ip_res != s[8].size())
        throw std::runtime_error("substream consumption mismatch");
    return out;
}

// ---- R3: measured-cost single-pass MDL parser (--parse=mdl) ---------------
// Replaces the global DP's heuristic costs with costs MEASURED from the actual
// downstream rANS stream construction: after each single-pass greedy parse we
// build the five separated streams (types/lit-len/match-len/dist/literals) and
// measure per-symbol empirical entropies; the next pass re-parses with those
// measured costs. All state is per-block and cache-resident; iterative
// refinement converges in a few passes at greedy-class speed (the DP is O(block)
// with global lookahead, this is O(block) with bounded single-edge lookahead).

struct MdlCosts {
    std::array<double,256> lit{};   // measured cost per literal byte value
    std::array<double,256> ttype{}; // measured cost per token type symbol
    std::array<double,256> ll{};    // measured cost per lit-run varint byte value
    std::array<double,256> ml{};    // measured cost per match-len varint byte value
    std::array<double,256> ds{};    // measured cost per dist varint byte value
};

static double varint_cost_ms(uint64_t x, const std::array<double,256>& m) {
    double c=0.0;
    for(;;) {
        uint8_t b=static_cast<uint8_t>(x&0x7F); x>>=7;
        if(x) b|=0x80;
        c+=m[b];
        if(!x) break;
    }
    return c;
}

static void measure_stream_costs(const std::vector<uint8_t>& v, std::array<double,256>& out) {
    if(v.empty()) { std::fill(out.begin(),out.end(),8.0); return; }
    std::array<uint64_t,256> cnt{};
    for(auto b:v) ++cnt[b];
    double tot=double(v.size());
    for(int i=0;i<256;++i)
        out[i]=cnt[i]?std::clamp(-std::log2(double(cnt[i])/tot),0.1,16.0):20.0; // absent syms never chosen
}

// Build the five mode-10 streams from a token sequence; return the measured
// per-stream costs plus the total ENCODED size (encode_stream picks raw-or-rANS
// per stream — the true downstream rANS cost, the MDL objective).
static std::pair<MdlCosts,size_t> measure_parse(const std::vector<uint8_t>& d, const std::vector<Token>& toks) {
    std::vector<uint8_t> types,ll,ml,ds,lits;
    types.reserve(toks.size());
    for(auto&t:toks) {
        types.push_back(t.match?1:0);
        if(t.match){ append_varint_bytes(ml,t.len-4); append_varint_bytes(ds,t.dist-1); }
        else { append_varint_bytes(ll,t.len-1); lits.insert(lits.end(),d.begin()+t.pos,d.begin()+t.pos+t.len); }
    }
    MdlCosts c;
    measure_stream_costs(types,c.ttype);
    measure_stream_costs(ll,c.ll);
    measure_stream_costs(ml,c.ml);
    measure_stream_costs(ds,c.ds);
    measure_stream_costs(lits,c.lit);
    size_t total=0;
    for(const auto* v:{&types,&ll,&ml,&ds,&lits}) total+=encode_stream(*v).size();
    return {c,total};
}

// One pass: windowed forward DP with measured costs. Each cache-resident window
// (16 KiB) is a full DP over literal edges and sampled match-length edges, so
// the parser makes the same GLOBAL edge choices as the old whole-block DP (near
// distances win because their measured ds cost is cheap) while processing each
// position once per pass. Match lengths are clamped at the window edge; the next
// window re-parses from there. Cost model comes from the actual rANS streams.
static std::vector<Token> parse_mdl_pass(const std::vector<uint8_t>& d, uint32_t max_chain, uint32_t max_match, const MdlCosts& c,
                                         bool use_boundary=false, const std::vector<uint8_t>& bseed={}) {
    const uint32_t n=static_cast<uint32_t>(d.size());
    std::vector<Token> toks;
    if(n==0) return toks;
    static constexpr uint32_t kWin = 16384;
    static constexpr uint32_t cuts[] = {4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65535};
    struct Prev { uint32_t from=0, dist=0; bool match=false; };
    MatchFinder mf(d,std::min(max_chain,16u),max_match,use_boundary); // shallow chains: near distances dominate measured ds cost
    if(use_boundary && bseed.size()==n) for(uint32_t p=0;p<n;++p) if(bseed[p]) mf.insert_boundary(p);
    std::vector<double> dp(kWin+1);
    std::vector<Prev> prev(kWin+1);
    uint32_t s=0;
    while(s<n) {
        uint32_t e=std::min(n,s+kWin);
        uint32_t wlen=e-s+1;
        std::fill(dp.begin(),dp.begin()+wlen,std::numeric_limits<double>::infinity());
        std::fill(prev.begin(),prev.begin()+wlen,Prev{});
        dp[0]=0.0;
        for(uint32_t i=s;i<e;++i) {
            uint32_t w=i-s;
            double lc=dp[w]+c.lit[d[i]]+0.10;
            if(lc<dp[w+1]){ dp[w+1]=lc; prev[w+1]={i,0,false}; }
            auto ms=mf.find(i);
            uint32_t win_remain=e-i;
            size_t ncand=std::min<size_t>(ms.size(),4);
            for(size_t ci=0;ci<ncand;++ci) {
                const auto&m=ms[ci];
                uint32_t cap=std::min(m.len,win_remain);
                std::array<uint32_t,16> lens{}; size_t nl=0;
                for(uint32_t ct:cuts) if(ct<=cap) lens[nl++]=ct;
                if(nl==0||lens[nl-1]!=cap) lens[nl++]=cap;
                double mcost=varint_cost_ms(m.dist-1,c.ds);
                for(size_t k=0;k<nl;++k) {
                    uint32_t l=lens[k];
                    double mc=dp[w]+c.ttype[1]+varint_cost_ms(l-4,c.ml)+mcost;
                    uint32_t wj=w+l;
                    if(mc<dp[wj]){ dp[wj]=mc; prev[wj]={i,m.dist,true}; }
                }
            }
            mf.insert(i);
        }
        std::vector<Token> rev;
        uint32_t cur=e;
        while(cur>s) {
            Prev p=prev[cur-s];
            if(p.from>=cur) throw std::runtime_error("window DP reconstruction failed");
            rev.push_back({p.match,p.from,cur-p.from,p.dist});
            cur=p.from;
        }
        for(auto it=rev.rbegin();it!=rev.rend();++it) toks.push_back(*it);
        s=e;
    }
    return toks;
}

static std::vector<Token> parse_mdl(const std::vector<uint8_t>& d, uint32_t max_chain, uint32_t max_match, uint32_t iters=3,
                                    bool boundary=false) {
    const uint32_t n=static_cast<uint32_t>(d.size());
    if(n==0) return {};
    // Seed: cheap single-pass greedy (longest match), measured costs from it.
    auto toks=parse_greedy(d,max_chain,max_match);
    auto [c,total]=measure_parse(d,toks);
    std::vector<Token> best=std::move(toks);
    size_t best_total=total;
    // Boundary-aligned candidate seeding (Linux C5): token starts of the previous
    // pass become the boundary-indexed source set for the next pass.
    std::vector<uint8_t> bseed(n,0);
    // Linux C5: 66-92% of LZ sources start within +-8 B of a prior token start.
    // Dilate each token start by +-8 so the boundary index covers aligned sources.
    auto mark_starts=[&](const std::vector<Token>& t){
        for(auto&x:t) {
            uint32_t lo = x.pos>8 ? x.pos-8 : 0;
            uint32_t hi = std::min<uint32_t>(n-1, x.pos+8);
            for(uint32_t p=lo;p<=hi;++p) bseed[p]=1;
        }
    };
    mark_starts(best);
    // Refine: windowed-DP passes with measured costs; stop when no gain.
    for(uint32_t it=1;it<iters;++it) {
        auto cand=parse_mdl_pass(d,max_chain,max_match,c,boundary,bseed);
        auto [cm,t2]=measure_parse(d,cand);
        if(t2<best_total){ best_total=t2; best=std::move(cand); }
        mark_starts(best);
        if(it>1 && t2+1>=best_total) break; // converged (1-byte slack)
        c=cm;
        total=t2;
    }
    return best;
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
    // F1 strictness: the arithmetic coder is self-terminating; reject if a full
    // trailing payload byte was never consumed (only the final partial byte's
    // zero padding is allowed). Modes 10/11 already enforce full consumption.
    if (n >= ad.consumed_bytes() + 2) throw std::runtime_error("trailing arithmetic bytes");
    return out;
}

struct Options {
    uint32_t block_size=256*1024;
    uint32_t max_chain=48;
    uint32_t max_match=65535;
    std::string parse="auto";
    std::string literal="auto"; // auto|o0|o1|g4|g8|g16
    std::string entropy="auto"; // auto|arith|rans
    uint32_t surprise=12;  // parser mismatch/surprise budget (entropy-control var; swept -> 12 beats 6 on corpus)
    uint32_t shape_states=28; // mode-12 per-shape displacement states (28 = per-shape, 1 = generic/FLAG-D control)
    bool boundary=false;   // boundary-aligned candidate generation (C5; measured neutral on corpus)
    bool negate=true;      // difference-cover negative gate for incompressible blocks (C5)
    bool stream_suite=true; // stream codec suite (huffman/defexc/256-512 rANS); off = fixed rANS-4096+raw
    bool quiet=false;
};
struct GlobalStats { uint64_t in=0,out=0,blocks=0,raw_blocks=0,compressed_blocks=0,literals=0,matches=0,matched_bytes=0,tokens=0; };

// Negative gate (Linux C5): content-hash probe. Samples ~1024 positions; if the
// sampled 4-byte windows are (nearly) all distinct, the block has no exploitable
// repetition -> incompressible -> emit raw without running any parser (big encode
// win on random data; random.bin was ~0.5 MB/s in auto because all 5 parses ran).
// Conservative: any repeated window keeps the block on the normal path, and a raw
// block is always valid, so this can never break correctness — worst case it skips
// a compression opportunity.
static bool probe_incompressible(const std::vector<uint8_t>& d) {
    const size_t n = d.size();
    if (n < 512) return false;
    const size_t S = 1024;
    size_t stride = std::max<size_t>(1, n / S);
    std::vector<uint32_t> setv(4096, 0xFFFFFFFFu); // open-addressing set of 18-bit hashes
    size_t uniq = 0, dup = 0;
    size_t off = 0;
    for (size_t i = 0; i < S && off + 4 <= n; ++i, off += stride) {
        uint32_t h = hash4(d.data() + off);
        uint32_t slot = h & 4095;
        bool found = false;
        while (setv[slot] != 0xFFFFFFFFu) {
            if (setv[slot] == h) { found = true; break; }
            slot = (slot + 1) & 4095;
        }
        if (found) ++dup; else { setv[slot] = h; ++uniq; }
    }
    // Random data: expected ~0.2% duplicate 18-bit hashes over 1024 samples.
    return dup * 100 <= S; // <=1% repeats across the sample -> incompressible
}

static std::vector<uint8_t> compress(const std::vector<uint8_t>& input, const Options& opt, GlobalStats* gs) {
    g_stream_suite = opt.stream_suite;
    g_j_agree = 0; g_j_total = 0;
    std::vector<uint8_t> out={'A','N','V','0',1};
    put_uvar(out,opt.block_size); put_uvar(out,input.size());
    GlobalStats st; st.in=input.size();
    for(size_t off=0; off<input.size();) {
        size_t blen=std::min<size_t>(opt.block_size,input.size()-off);
        std::vector<uint8_t> block(input.begin()+off,input.begin()+off+blen);

        // Negative gate: incompressible blocks go straight to raw (no parser runs).
        if(opt.negate && probe_incompressible(block)) {
            ++st.blocks; ++st.raw_blocks; st.literals+=blen;
            put_uvar(out,blen);
            uint32_t sum=crc32(block.data(),block.size());
            out.push_back(0); put_uvar(out,block.size()); put_u32le(out,sum);
            out.insert(out.end(),block.begin(),block.end());
            off+=blen; continue;
        }

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
        std::vector<Token> mdl_toks; bool have_mdl=false;
        if(opt.parse=="auto" || opt.parse=="mdl") { mdl_toks=parse_mdl(block,opt.max_chain,opt.max_match,3,opt.boundary); have_mdl=true; consider_parse(mdl_toks); }
        std::vector<SparseToken> sp_toks; bool have_sp=false;
        if(opt.parse=="auto" || opt.parse=="sparse") {
            sp_toks=parse_sparse(block,opt.max_chain,opt.max_match,opt.surprise,opt.boundary); have_sp=true;
            auto payload=encode_tokens_sparse(block,sp_toks);
            std::vector<Token> t; t.reserve(sp_toks.size());
            for(auto&s:sp_toks) t.push_back({s.type!=0,s.pos,s.len,s.dist});
            if(best.mode==0 || payload.size()<best.payload.size()) best={std::move(payload),std::move(t),11};
        }
        // Mode 12 (SHAPE): per-shape displacement prediction over either the
        // sparse parse (types 0/1/2) or the mdl parse (exact-only).
        if(opt.parse=="auto" || opt.parse=="shape") {
            if(!have_sp) { sp_toks=parse_sparse(block,opt.max_chain,opt.max_match,opt.surprise,opt.boundary); have_sp=true; }
            if(!have_mdl) { mdl_toks=parse_mdl(block,opt.max_chain,opt.max_match,3,opt.boundary); have_mdl=true; }
            auto try_shape=[&](const std::vector<SparseToken>& st){
                auto payload=encode_tokens_shape(block,st,opt.shape_states);
                std::vector<Token> t; t.reserve(st.size());
                for(auto&s:st) t.push_back({s.type!=0,s.pos,s.len,s.dist});
                if(best.mode==0 || payload.size()<best.payload.size()) best={std::move(payload),std::move(t),12};
            };
            try_shape(sp_toks);
            std::vector<SparseToken> st2; st2.reserve(mdl_toks.size());
            for(auto&x:mdl_toks) st2.push_back({static_cast<uint8_t>(x.match?1:0), x.pos, x.len, x.dist, {}, {}});
            try_shape(st2);
        }
        // Mode 13 (TOPOLOGY): per-slot modal residual + exception mask over the
        // sparse parse (types 0/1/2). R2 correction-topology coding.
        if(opt.parse=="auto" || opt.parse=="topology") {
            if(!have_sp) { sp_toks=parse_sparse(block,opt.max_chain,opt.max_match,opt.surprise,opt.boundary); have_sp=true; }
            auto payload=encode_tokens_topology(block,sp_toks,opt.shape_states);
            std::vector<Token> t; t.reserve(sp_toks.size());
            for(auto&s:sp_toks) t.push_back({s.type!=0,s.pos,s.len,s.dist});
            if(best.mode==0 || payload.size()<best.payload.size()) best={std::move(payload),std::move(t),13};
        }
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
    // DoS guard: output cannot legitimately exceed (max blocks) * (max block size);
    // each block needs >= 7 header bytes, block_size is capped at 64 MiB.
    if(total > ((uint64_t)in.size()/7 + 2) * (1ull<<26)) throw std::runtime_error("declared size exceeds amplification bound");
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
        } else if(mode==11) {
            b=decode_tokens_sparse(p,static_cast<size_t>(plen),static_cast<size_t>(blen));
        } else if(mode==12) {
            b=decode_tokens_shape(p,static_cast<size_t>(plen),static_cast<size_t>(blen));
        } else if(mode==13) {
            b=decode_tokens_topology(p,static_cast<size_t>(plen),static_cast<size_t>(blen));
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
              << "  anvil c <input> <output> [--parse=auto|dp|greedy|sparse|mdl|shape|topology] [--literal=auto|o0|o1|g4|g8|g16] [--entropy=auto|arith|rans|sparse] [--block=N] [--chain=N] [--max-match=N] [--surprise=N] [--shape-states=1|28] [--boundary=on|off] [--negate=on|off] [--quiet]\n"
              << "  anvil d <input> <output> [--quiet]\n"
              << "  anvil verify <input> [--parse=auto|dp|greedy|sparse|mdl|shape|topology] [--literal=auto|o0|o1|g4|g8|g16] [--entropy=auto|arith|rans|sparse]\n"
              << "  note: sparse->mode 11, shape->mode 12 (per-shape displacement), topology->mode 13 (modal residuals); --shape-states=1 is the FLAG-D control\n"
              << "  note: --surprise=N is the sparse-parser mismatch budget (default 12); --boundary/--negate are C5 adopts\n";
}

} // namespace anvil

#ifndef ANVIL_NO_MAIN
int main(int argc,char**argv) {
    using namespace anvil;
    try {
        if(const char* env=getenv("ANVIL_STREAM_LAMBDA")) g_stream_lambda=std::atof(env);
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
            else if(a.rfind("--surprise=",0)==0)opt.surprise=std::stoul(a.substr(11));
            else if(a.rfind("--shape-states=",0)==0)opt.shape_states=std::stoul(a.substr(15));
            else if(a.rfind("--boundary=",0)==0)opt.boundary=(a.substr(11)!="off");
            else if(a.rfind("--negate=",0)==0)opt.negate=(a.substr(9)!="off");
            else if(a.rfind("--stream-suite=",0)==0)opt.stream_suite=(a.substr(15)!="off");
            else if(a=="--quiet")opt.quiet=true;
            else throw std::runtime_error("unknown option: "+a);
        }
        if(opt.parse!="auto"&&opt.parse!="dp"&&opt.parse!="greedy"&&opt.parse!="sparse"&&opt.parse!="mdl"&&opt.parse!="shape"&&opt.parse!="topology")throw std::runtime_error("parse must be auto, dp, greedy, sparse, mdl, shape or topology");
        if(opt.literal!="auto"&&opt.literal!="o0"&&opt.literal!="o1"&&opt.literal!="g4"&&opt.literal!="g8"&&opt.literal!="g16")throw std::runtime_error("literal must be auto, o0, o1, g4, g8 or g16");
        if(opt.entropy!="auto"&&opt.entropy!="arith"&&opt.entropy!="rans"&&opt.entropy!="sparse")throw std::runtime_error("entropy must be auto, arith, rans or sparse");
        if(opt.shape_states!=1 && opt.shape_states!=28)throw std::runtime_error("shape-states must be 1 or 28");
        if(cmd=="c") {
            if(argc<4){usage();return 2;} auto in=read_file(argv[2]); GlobalStats st;
            auto t0=std::chrono::steady_clock::now(); auto out=compress(in,opt,&st); auto t1=std::chrono::steady_clock::now(); write_file(argv[3],out);
            if(!opt.quiet){double sec=std::chrono::duration<double>(t1-t0).count(); std::cerr<<"ANVIL c parse="<<opt.parse<<" literal="<<opt.literal<<" entropy="<<opt.entropy<<" in="<<st.in<<" out="<<st.out<<" ratio="<<(st.in?double(st.out)/st.in:0)<<" MB/s="<<(sec?st.in/1e6/sec:0)<<" blocks="<<st.blocks<<" compressed="<<st.compressed_blocks<<" raw="<<st.raw_blocks<<" literals="<<st.literals<<" matches="<<st.matches<<" matched_bytes="<<st.matched_bytes<<" j_agree="<<g_j_agree<<"/"<<g_j_total<<"\n";}
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

