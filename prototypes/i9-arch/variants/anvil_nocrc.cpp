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
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// CRC32 PCLMULQDQ fast path (S6-1b leg 1, stage 2) — x86-64 only; the
// slicing-by-8 implementation below is the portable fallback.
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#include <wmmintrin.h>
#if defined(_MSC_VER) || defined(__clang__)
#include <intrin.h>
#elif defined(__GNUC__)
#include <cpuid.h>
#endif
#endif

#ifdef ANVIL_HAVE_BROTLI
#include <brotli/decode.h>
#include <brotli/encode.h>
#endif
#ifdef ANVIL_HAVE_LIBSAIS
#include <libsais.h>
#endif

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
    uint64_t count = 0;
    void bit(uint32_t b) {
        ++count;
        cur = static_cast<uint8_t>((cur << 1) | (b & 1));
        if (++used == 8) { out.push_back(cur); cur = 0; used = 0; }
    }
    void finish() {
        if (used) { cur <<= (8 - used); out.push_back(cur); cur = 0; used = 0; }
    }
    uint64_t bit_count() const { return count; }
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
    uint64_t bit_count() const { return bw_.bit_count(); }
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

// CRC32 (IEEE, reflected, poly 0xEDB88320) — S6-1b LEG 1 (bit-exact upgrade).
// Stage 1 (slicing-by-8): provably identical checksum values to the bytewise
// table version (same GF(2) linear map — see docs/pre-registrations/s6-1b.md §3).
// Derivation: message byte j (j=0 is first byte of the 8-byte group) and
// state byte j both contribute exactly R^(8-j)(i) to the next state, where
// R(x) = tab0[x&0xFF] ^ (x>>8) is one zero-byte refinement and tab0 is the
// classic bytewise table (= R^8 of a raw byte). Hence 8 tables
// Q_s[i] = R^(8-s)(i), s=0..7, indexed by the merged byte (state^msg).
// Wire-invisible by construction; verified bit-exact vs the bytewise version.
//
// Stage 2 (this change): PCLMULQDQ 4-way fold, same IEEE CRC-32 (poly
// 0xEDB88320, init/final xor 0xFFFFFFFF, reflected). Runtime CPUID dispatch;
// slicing-by-8 remains the fallback on CPUs without PCLMULQDQ. Both stages
// PRESERVE verification semantics (checksum-skipping is NOT done): wire bytes,
// decoded bytes and corruption-reject behavior are unchanged. Bit-exactness
// evidence: prototypes/i9-arch/crc_bit_exact.cpp (16,529 checks: lengths
// 0..4113 x 4 patterns, large lengths, unaligned starts, "123456789" ->
// CBF43926) and prototypes/i9-arch/crc_corpus_check.cpp (corpus chunk sweep).
static uint32_t crc32_slice8(const uint8_t* p, size_t n) {
    // Q[s][i] = R^(7-s)(T[i]) where T is the classic table T[i] = R^8(i).
    // NOTE (corrected 2026-09-04, decode-perf P0): the previous build seeded
    // t[0]=classic T and t[s]=R^(8-s)(raw byte i) for s>=1. That makes
    // t[0] = R^8(i) != R^7(T[i]), so the FIRST table was wrong (only the
    // first - s>=1 happened to agree). Correct invariant is Q[s]=R^(7-s)(T[i]),
    // which puts the classic table at index 7, not 0.
    static std::array<std::array<uint32_t,256>,8> qtabs = []{
        std::array<std::array<uint32_t,256>,8> t{};
        // classic table T[i] = R^8(i), stored at index 7 (== R^0(T[i]))
        for(uint32_t i=0;i<256;++i){ uint32_t c=i; for(int k=0;k<8;++k)c=(c&1)?(0xEDB88320u^(c>>1)):(c>>1); t[7][i]=c; }
        for(int s=0;s<8;++s){
            int steps=7-s;
            for(uint32_t i=0;i<256;++i){
                uint32_t c=t[7][i];                 // start from T[i], not raw i
                for(int r=0;r<steps;++r) c=(t[7][c&0xFFu])^(c>>8);
                t[s][i]=c;
            }
        }
        return t;
    }();
    uint32_t c=0xFFFFFFFFu;
    // Slicing-by-8 main loop (unaligned-safe byte loads; all shifts <32).
    // NOTE (corrected): '^' binds TIGHTER than '|' in C++, so the previous
    // `c ^ p[0] | (p[1]<<8) | ...` parsed as (c^p[0])|(p[1]<<8)|... and OR-ed
    // the top 3 message bytes instead of XOR-ing them. Parenthesised.
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
    // Tail (<8 bytes): classic bytewise refinement (classic table = index 7).
    for(size_t i=0;i<n;++i)c=qtabs[7][(c^p[i])&0xFFu]^(c>>8);
    return c^0xFFFFFFFFu;
}
#if defined(__x86_64__) || defined(_M_X64)
// PCLMULQDQ feature check (CPUID leaf 1, ECX bit 1).
static bool crc32_has_pclmul() {
#if defined(_MSC_VER) || defined(__clang__)
    int regs[4]; __cpuid(regs, 1); return (regs[2] >> 1) & 1;
#elif defined(__GNUC__)
    unsigned a=0,b=0,c=0,d=0; __cpuid(1, a, b, c, d); return (c >> 1) & 1;
#else
    return false;
#endif
}
#if defined(__clang__) || defined(__GNUC__)
// clang-cl/GCC: enable the ISA only for this function (no global -mpclmul needed).
__attribute__((target("pclmul,sse4.1")))
#endif
static uint32_t crc32_pclmul(const uint8_t* src, size_t len) {
    if (len < 64) return crc32_slice8(src, len);
    // Constants below implement the reflected IEEE CRC-32 (same poly/init/final
    // as crc32_slice8). Validated bit-exact by crc_bit_exact.cpp, not trusted
    // from any prototype comment.
    const __m128i f4  = _mm_set_epi32(0x00000001, 0x54442bd4, 0x00000001, 0xc6e41596);
    const __m128i k12 = _mm_set_epi32(0x00000001, 0x751997d0, 0x00000000, 0xccaa009e);
    const __m128i bk  = _mm_set_epi32(0x00000001, 0xdb710640, 0xb4e5b025, 0xf7011641);
    // 0x9db42487 encodes the 0xFFFFFFFF init/final-xor of the reflected CRC.
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
    // Fold the 4 x 128-bit lanes down to one, then Barrett-reduce 128 -> 32.
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
    // Tail (<64 B): classic bytewise refinement of the live register (do NOT
    // complement here; ~reg is applied once at the end, as in stage 1).
    if (len) {
        static const std::array<uint32_t,256> T = []{
            std::array<uint32_t,256> t{};
            for(uint32_t i=0;i<256;++i){ uint32_t c=i; for(int k=0;k<8;++k)c=(c&1)?(0xEDB88320u^(c>>1)):(c>>1); t[i]=c; }
            return t; }();
        for (size_t i = 0; i < len; ++i) reg = T[(reg ^ src[i]) & 0xFFu] ^ (reg >> 8);
    }
    return ~reg;
}
#endif // x86-64
// Dispatcher: PCLMULQDQ when the CPU advertises it, slicing-by-8 otherwise.
// Both compute the identical checksum (see the evidence note above).
static uint32_t crc32(const uint8_t* p, size_t n) {
#if defined(__x86_64__) || defined(_M_X64)
    static const bool has = crc32_has_pclmul();
    return has ? crc32_pclmul(p, n) : crc32_slice8(p, n);
#else
    return crc32_slice8(p, n);
#endif
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
    std::vector<uint32_t> tfo;  // TCOPY: 4-aligned window indices with implicit Delta=-d fields
};

struct SparseToken {
    uint8_t type = 0;  // 0 literal run, 1 exact match, 2 sparse-corrected match, 3 TCOPY match
    uint32_t pos = 0, len = 0, dist = 0;
    std::vector<uint32_t> off;
    std::vector<uint8_t> val;
    std::vector<uint32_t> tfo;  // TCOPY transform-field window indices (4-aligned)
    int64_t delta = 0;          // ARI-REF (mode 16): transmitted per-word additive constant (type 4 only)
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
        if (pos + 4 > d_.size()) return out; // hash4 probes read 4 bytes; pos may be the EOF token boundary (ASan-found overread)
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
    // Scan ONE candidate source q against pos; keeps the best prefix in the
    // caller's accumulators. Used by the hash-chain walk and the structural
    // channel check (fixed dist). `work` caps scanned bytes.
    // TCOPY: a 4-aligned window whose 32-bit target == source - dist (implicit
    // Delta=-d, the executable-relative relocation algebra) is recorded as a
    // transform field (tfo window index) and costs only its mask bit.
    // `allow_tfo_only`/`min_len`: PNRA (Experiment X) needs a SEPARATE, narrower
    // acceptance path — a single isolated transform field (len=4, zero literal
    // corrections) is exactly the minimal, common case an invariant-anchored
    // candidate produces, but the default acceptance below (`local_k>=1 &&
    // local_len>=8`, tuned for byte-hash-anchored sparse/tcopy candidates) drops
    // it. Default parameters reproduce the EXACT prior behavior for every
    // existing caller (find_sparse/find_sparse_at); only find_pnra_at passes
    // allow_tfo_only=true, min_len=4.
    void scan_candidate(uint32_t pos, uint32_t q, const std::array<double,256>& litcost,
                        double avg_lit, uint64_t& work, double dead_band, bool tcopy,
                        uint32_t& blen, uint32_t& bk, std::array<uint32_t,kSparseScanMax>& boff,
                        std::array<uint8_t,kSparseScanMax>& bval, double& best_score,
                        std::array<uint32_t,kSparseScanMax/4>& btfo, uint32_t& btfo_n,
                        uint32_t max_len=0, bool allow_tfo_only=false, uint32_t min_len=8) const {
        const uint8_t* tgt = d_.data() + pos;
        const uint8_t* src = d_.data() + q;
        uint32_t remain = static_cast<uint32_t>(d_.size() - pos);
        uint32_t cap = std::min({remain, max_match_, kSparseScanMax});
        if (max_len) cap = std::min(cap, max_len);
        if (cap < min_len) return;
        const double match_gain = avg_lit - 0.125;
        const uint32_t dist = pos - q;
        double score = 0.0, local_best = -1e300;
        uint32_t k = 0, local_len = 0, local_k = 0, local_tfo = 0, local_tfo_snap = 0;
        std::array<uint32_t, kSparseScanMax> off{};
        std::array<uint8_t, kSparseScanMax> val{};
        std::array<uint32_t, kSparseScanMax/4> tfo{};
        uint32_t j = 0;
        for (; j < cap; ++j) {
            if (++work > kSparseBlockBudget) break;
            uint8_t cpy = (dist > 0) ? src[j % dist] : src[j]; // overlapping copy is periodic
            if (cpy != tgt[j]) {
                if (tcopy && (j & 3) == 0 && j + 4 <= cap && j + 4 <= dist) {
                    uint32_t s32, t32;
                    std::memcpy(&s32, src + j, 4); std::memcpy(&t32, tgt + j, 4);
                    if (s32 != t32 && t32 == s32 - dist) { // implicit Delta = -dist
                        tfo[local_tfo++] = j >> 2;
                        score -= 0.1; // transform field costs only its mask bit
                        j += 3; // skip the window
                        if (score > local_best) { local_best = score; local_len = j + 1; local_k = k; local_tfo_snap = local_tfo; }
                        continue;
                    }
                }
                if (k >= kSparseScanMax) break;
                off[k] = j; val[k] = tgt[j];
                score -= litcost[tgt[j]] + 0.125;
                ++k;
            } else {
                score += match_gain;
            }
            if (score > local_best) { local_best = score; local_len = j + 1; local_k = k; local_tfo_snap = local_tfo; }
            else if (score < local_best - dead_band) break;
        }
        bool accept = (local_k >= 1 && local_len >= 8)
                    || (allow_tfo_only && local_tfo_snap >= 1 && local_len >= min_len);
        if (accept && local_best > best_score) {
            best_score = local_best; blen = local_len; bk = local_k;
            for (uint32_t i = 0; i < local_k; ++i) { boff[i] = off[i]; bval[i] = val[i]; }
            btfo_n = local_tfo_snap;
            for (uint32_t i = 0; i < local_tfo_snap; ++i) btfo[i] = tfo[i];
        }
    }

    // Sparse candidate at a FIXED distance (structural channel): q = pos - dist.
    // `max_len` (0 = uncapped) caps the candidate at one record span: the SRR
    // probe uses dist+8 so a record-aligned hit returns exactly one record's
    // corrections instead of scanning on into the next record.
    bool find_sparse_at(uint32_t pos, uint32_t dist, SparseMatch& out, const std::array<double,256>& litcost,
                        double avg_lit, uint64_t& work, double dead_band=32.0, bool tcopy=false, uint32_t max_len=0) const {
        out.len = 0;
        if (dist == 0 || dist > pos || pos + 4 > d_.size()) return false;
        uint32_t q = pos - dist;
        const uint8_t* tgt = d_.data() + pos;
        const uint8_t* src = d_.data() + q;
        if (src[0] != tgt[0] || src[1] != tgt[1] || src[2] != tgt[2] || src[3] != tgt[3]) return false;
        std::array<uint32_t, kSparseScanMax> boff{};
        std::array<uint8_t, kSparseScanMax> bval{};
        std::array<uint32_t, kSparseScanMax/4> btfo{};
        uint32_t blen = 0, bk = 0, btfo_n = 0; double best_score = -1e300;
        scan_candidate(pos, q, litcost, avg_lit, work, dead_band, tcopy, blen, bk, boff, bval, best_score, btfo, btfo_n, max_len);
        if (bk >= 1 && blen >= 8) {
            out.len = blen; out.dist = dist;
            out.off.assign(boff.begin(), boff.begin() + bk);
            out.val.assign(bval.begin(), bval.begin() + bk);
            out.tfo.assign(btfo.begin(), btfo.begin() + btfo_n);
            return true;
        }
        return false;
    }

    // PNRA (Experiment X): invariant-anchored candidate at an EXPLICIT source q
    // supplied by the caller's invariant index (no first-4-byte prefilter — the
    // whole point is q's raw bytes need NOT match at pos; only the transform-field
    // algebra at the anchor itself does). Reuses scan_candidate verbatim so the
    // resulting SparseMatch is cost-model-identical in shape to a normal sparse
    // candidate; only the SOURCE of q differs (invariant hash, not byte hash).
    bool find_pnra_at(uint32_t pos, uint32_t q, SparseMatch& out, const std::array<double,256>& litcost,
                      double avg_lit, uint64_t& work, double dead_band=32.0) const {
        out.len = 0;
        if (q >= pos || pos + 4 > d_.size()) return false;
        std::array<uint32_t, kSparseScanMax> boff{};
        std::array<uint8_t, kSparseScanMax> bval{};
        std::array<uint32_t, kSparseScanMax/4> btfo{};
        uint32_t blen = 0, bk = 0, btfo_n = 0; double best_score = -1e300;
        scan_candidate(pos, q, litcost, avg_lit, work, dead_band, /*tcopy=*/true, blen, bk, boff, bval, best_score, btfo, btfo_n,
                       /*max_len=*/0, /*allow_tfo_only=*/true, /*min_len=*/4);
        if (btfo_n == 0) return false; // PNRA candidates without a transform field bring nothing the ordinary hash search doesn't already find
        if (bk <= btfo_n && blen >= 4) { // require the transform field itself to be covered, not just a chance literal run
            out.len = blen; out.dist = pos - q;
            out.off.assign(boff.begin(), boff.begin() + bk);
            out.val.assign(bval.begin(), bval.begin() + bk);
            out.tfo.assign(btfo.begin(), btfo.begin() + btfo_n);
            return true;
        }
        return false;
    }

    // Approximate candidate: same first-4-byte hash bucket, then scan forward
    // allowing mismatches. Tracks the best prefix by an MDL-ish score
    //   score(prefix) = len*avg_lit - len/8 - sum(litcost[correction])
    // i.e. mask+residuals vs the all-literal fallback. Returns the best sparse
    // candidate with >=1 correction, or false. `work` caps scanned bytes.
    bool find_sparse(uint32_t pos, SparseMatch& out, const std::array<double,256>& litcost,
                     double avg_lit, uint64_t& work, double dead_band=32.0, bool tcopy=false) const {
        out.len = 0;
        if (pos + 4 > d_.size()) return false;
        const uint8_t* tgt = d_.data() + pos;
        uint32_t h = hash4(tgt);
        uint32_t q = head_[h];
        uint32_t remain = static_cast<uint32_t>(d_.size() - pos);
        uint32_t cap = std::min({remain, max_match_, kSparseScanMax});
        if (cap < 8) return false;
        std::array<uint32_t, kSparseScanMax> boff{};
        std::array<uint8_t, kSparseScanMax> bval{};
        std::array<uint32_t, kSparseScanMax/4> btfo{};
        uint32_t blen = 0, bk = 0, btfo_n = 0;
        double best_score = -1e300;
        uint32_t best_q = 0;
        for (uint32_t depth = 0; q != kNoPos && depth < kSparseChainMax; ++depth, q = prev_[q]) {
            if (q >= pos) break;
            const uint8_t* src = d_.data() + q;
            if (src[0] != tgt[0] || src[1] != tgt[1] || src[2] != tgt[2] || src[3] != tgt[3]) continue;
            double before = best_score;
            scan_candidate(pos, q, litcost, avg_lit, work, dead_band, tcopy, blen, bk, boff, bval, best_score, btfo, btfo_n);
            if (best_score > before) best_q = q;
            if (work >= kSparseBlockBudget) break;
        }
        if (bk >= 1 && blen >= 8) {
            out.len = blen;
            out.dist = pos - best_q;
            out.off.assign(boff.begin(), boff.begin() + bk);
            out.val.assign(bval.begin(), bval.begin() + bk);
            out.tfo.assign(btfo.begin(), btfo.begin() + btfo_n);
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
// `channels` (R4) maintains a bank of persistent STRUCTURAL displacements
// (record periods), reinforced by successful approximate phrases; channel
// candidates are preferred on near-ties so corrections align to a record frame
// (this is what R2 topology coding needs).
static constexpr uint32_t kChannels = 8;
struct StructChannel { uint32_t dist = 0; double score = 0.0; uint32_t last = 0; };
static uint64_t g_ch_try = 0, g_ch_win = 0; // R4 channel diagnostics
static uint64_t g_pnra_gate = 0, g_pnra_idxhit = 0, g_pnra_verify = 0, g_pnra_commit = 0; // Experiment X diagnostics
// TEMP t-cost instrumentation (characterize real mask/distance framing)
static uint64_t g_diag_t3 = 0, g_diag_reswords = 0, g_diag_tmw = 0;
static uint64_t g_diag_sz_raw[8] = {0}, g_diag_sz_z[8] = {0};
static uint64_t g_ch_chosen = 0, g_ch_span_win = 0, g_ch_span_emit = 0; // SRR emit diagnostics

static std::vector<SparseToken> parse_sparse(const std::vector<uint8_t>& d, uint32_t max_chain, uint32_t max_match,
                                             uint32_t surprise=12, bool boundary=false, bool channels=true, bool tcopy=false,
                                             bool pnra=false) {
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
    // PNRA (Experiment X, I4-4 successor): transformation-invariant index over
    // x86 E8/E9 (near call/jmp) relocation fields. I(v,p) = p+4+v (the absolute
    // branch target, i.e. rip-after-instruction + rel32) is INVARIANT under the
    // Delta=-dist relocation transform TCOPY's mode-14 field already codes
    // implicitly: moving the field to position p'=p-dist changes v by +dist to
    // keep the same target, so two occurrences of the SAME target are exact hash
    // hits in invariant space even though their raw bytes (opcode + differing
    // rel32) never byte-match and so are invisible to the ordinary 4-byte hash
    // chain in MatchFinder. Gated on the E8/E9 opcode byte only (a ~1-2% sparse
    // trigger on real PE .text, per Experiment U/V measurement) so this stays
    // O(1) amortized per opcode occurrence, not O(n) dense — the density-mismatch
    // blocker Experiment U diagnosed for ungated dense invariant families.
    std::unordered_map<uint32_t, std::vector<uint32_t>> pnra_idx;
    if (pnra && tcopy) {
        for (uint32_t p = 0; p + 5 <= n; ++p) {
            uint8_t op = d[p];
            if (op != 0xE8 && op != 0xE9) continue;
            uint32_t field_pos = p + 1;
            int32_t rel32; std::memcpy(&rel32, d.data() + field_pos, 4);
            uint32_t target = field_pos + 4u + static_cast<uint32_t>(rel32);
            pnra_idx[target].push_back(field_pos);
        }
    }
    std::array<StructChannel, kChannels> chan{};
    size_t nchan = 0;
    auto reinforce=[&](uint32_t dist, uint32_t at, double gain) {
        if (dist == 0 || dist > 16384) return; // structural periods are near; far distances are not channels
        for (size_t c = 0; c < nchan; ++c)
            if (chan[c].dist == dist) { chan[c].score += gain; chan[c].last = at; return; }
        if (nchan < kChannels) { chan[nchan++] = {dist, gain, at}; return; }
        size_t worst = 0; for (size_t c = 1; c < nchan; ++c) if (chan[c].score < chan[worst].score) worst = c;
        if (gain > chan[worst].score * 0.25) chan[worst] = {dist, gain, at};
    };
    uint32_t i=0;
    // SRR synchronized state: the last observed record span (distance of a
    // taken span-like match) and the next expected record-boundary position.
    // The probe tracks this span (with a drift window) instead of sweeping a
    // fixed grid — record periods drift (jsonl 235 B +-2), so a fixed-distance
    // probe can never stay locked.
    uint32_t last_span = 0;
    uint32_t phase = 0;          // next expected record-boundary position (0 = unknown)
    bool took_ch = false;
    // A span-like match (len ~= dist) covers one whole record span: the next
    // position i+len is the next record boundary. Record it so the probe can
    // fire at the synchronized phase and keep consecutive records aligned.
    auto note_span = [&](uint32_t dist, uint32_t len) {
        if (dist > 0 && dist <= 16384) { last_span = dist; phase = i + len; }
    };
    auto note_ch_emit = [&](uint32_t dist) {
        ++g_ch_chosen;
        if (last_span > 4 && dist >= last_span - 4 && dist <= last_span + 4) ++g_ch_span_emit;
    };
    // Discovery sweep: dense step-4 in the plausible record band (24..128),
    // then coarser beyond. Log's 108-byte lines fall between a coarse grid's
    // 96/112 and would never be found — the dense band is required.
    static const uint32_t kSweep[] = {
        24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,100,104,108,112,116,120,124,128,
        136,144,152,160,168,176,184,192,200,208,216,224,232,240,248,256,272,288,304,320,336,352,368,384,
        416,448,480,512,576,640,704,768,832,896,960,1024
    };
    static constexpr size_t kSweepN = sizeof(kSweep)/sizeof(kSweep[0]);
    while(i<n) {
        auto ms=mf.find(i);
        Match exact{0,0};
        for(auto&m:ms) if(m.len>exact.len || (m.len==exact.len && m.dist<exact.dist)) exact=m;
        double exact_c=std::numeric_limits<double>::infinity();
        if(exact.len>=4) exact_c=0.6+varint_cost(exact.len-4)+varint_cost(exact.dist-1)+0.18*std::log2(double(exact.dist)+1.0);
        // SRR: synchronized structural probe — ACTIVELY TEST candidate record
        // periods (the channel bank's missing discovery), then sync-lock the
        // winner so consecutive records align (R2/TCOPY prerequisite).
        // The probe runs at EVERY position: find_sparse_at early-rejects on a
        // 4-byte anchor mismatch (near-free), so probing is cheap. It tests
        // locked channels plus a tight drift window around the last observed
        // record span; a fixed-grid discovery sweep runs only until the first
        // channel locks. A probe hit whose length ~= its distance is a
        // full-record-span match — the strongest alignment signal — and is
        // locked with a large reinforcement so consecutive records align.
        SparseMatch ch_sm; bool has_ch=false; double ch_cost=0.0;
        bool ch_span=false;
        // Probe gate: fire at the expected record boundary (i near phase, drift
        // +-2 to absorb length drift) OR during the discovery phase (no phase
        // yet: probe a bounded prefix of the block). find_sparse_at
        // early-rejects on a 4-byte anchor mismatch, so the per-position probe
        // cost is small; gating keeps it from running at every byte.
        bool at_phase = (phase != 0 && i + 2 >= phase && i <= phase + 2);
        // If the generic parse overshot the expected boundary, the lock is
        // stale: reset so the probe can re-lock on the next clean prefix
        // instead of going dead forever.
        if (phase != 0 && i > phase + 2) { phase = 0; }
        bool discovery = (phase == 0 && i < 65536 && nchan == 0);
        // Probe gate: the probe fires at the expected record boundary (i near
        // phase, drift +-2) OR during the discovery phase. Probing every
        // position re-introduces weak channel matches that displace good
        // generic candidates (measured regression), so it stays phase-gated;
        // when the generic parse overshoots the phase the lock simply expires
        // and discovery re-runs on the next clean prefix.
        if(channels && (at_phase || discovery) && work<kSparseBlockBudget) {
            uint32_t probe_dists[48];
            uint32_t np = 0;
            for (size_t c = 0; c < nchan; ++c) probe_dists[np++] = chan[c].dist;
            if (at_phase && last_span > 4) { // synchronized drift window around the observed span
                uint32_t lo = last_span > 4 ? last_span - 4 : 1;
                for (uint32_t w = lo; w <= last_span + 4 && np < 48; ++w) probe_dists[np++] = w;
            }
            if (discovery) { // discovery phase: dense sweep of plausible record periods
                for (size_t s = 0; s < kSweepN && np < 48; ++s) probe_dists[np++] = kSweep[s];
            }
            double best_score = -1e300; uint32_t best_dist = 0; bool best_span = false;
            for (uint32_t pi = 0; pi < np && work < kSparseBlockBudget; ++pi) {
                uint32_t pd = probe_dists[pi];
                if (pd == 0 || pd > i) continue;
                SparseMatch sm;
                ++g_ch_try;
                if (mf.find_sparse_at(i, pd, sm, litcost, avg_lit, work, dead_band, tcopy, pd + 8)) {
                    ++g_ch_win;
                    if (last_span > 4 && pd >= last_span - 4 && pd <= last_span + 4) ++g_ch_span_win;
                    double sc = 1.5+varint_cost(sm.len-4)+varint_cost(sm.dist-1)+double(sm.len)/8.0
                             +0.18*std::log2(double(sm.dist)+1.0);
                    for (size_t k = 0; k < sm.off.size(); ++k) sc += litcost[sm.val[k]];
                    if (sc < ch_cost || !has_ch) { has_ch = true; ch_cost = sc; ch_sm = std::move(sm); }
                    bool span_like = sm.len >= pd && sm.len <= pd + 8; // full-record-span hit
                    double score = double(sm.len) - sc * 0.25;
                    if (span_like) score += 24.0; // record-span hits are the alignment signal
                    if (score > best_score) { best_score = score; best_dist = pd; best_span = span_like; }
                }
            }
            if (best_dist) reinforce(best_dist, i, best_span ? 64.0 : 8.0); // span hits lock hard
            // Commit decision: a SPAN-LIKE hit at a LOCKED channel (score >= 96,
            // i.e. reinforced twice as a span) is a synchronized record match.
            // It is taken directly (against its own literal alternative) —
            // skipping the generic far-distance comparison, which would always
            // pick a longer multi-record match and break alignment.
            if (has_ch && ch_sm.len >= ch_sm.dist && ch_sm.len <= ch_sm.dist + 8) {
                for (size_t c = 0; c < nchan; ++c)
                    if (chan[c].dist == ch_sm.dist && chan[c].score >= 96.0) { ch_span = true; break; }
            }
        }
        // SYNCHRONIZED COMMIT: a span-like match at a LOCKED channel is taken
        // directly (against its own literal alternative) — no generic-cost
        // comparison. This is what keeps consecutive records phase-aligned:
        // once the period is locked, record N+1 matches record N at the locked
        // distance, so the probe fires at the sync phase and commits.
        if (ch_span && ch_cost < pref[i + ch_sm.len] - pref[i]) {
            SparseToken t; t.type=(tcopy && !ch_sm.tfo.empty())?3u:2u; t.pos=i; t.len=ch_sm.len; t.dist=ch_sm.dist;
            t.off=std::move(ch_sm.off); t.val=std::move(ch_sm.val); t.tfo=std::move(ch_sm.tfo);
            toks.push_back(std::move(t));
            note_ch_emit(t.dist);
            if(boundary) mf.insert_boundary(i);
            reinforce(t.dist,i,double(t.len)*0.5); note_span(t.dist, t.len);
            uint32_t end=i+t.len;
            for(uint32_t p=i;p<end;++p) mf.insert(p);
            i=end;
            continue;
        }
        // PNRA candidate (Experiment X): fires ONLY right after an E8/E9 opcode
        // byte (the sparse structural trigger) — an invariant-space hash hit
        // supplies a candidate source q whose raw bytes need not byte-match at
        // all, which is exactly the class of copy opportunity the ordinary
        // hash-chain search (mf.find/find_sparse, both anchored on a byte-equal
        // 4-byte prefix) structurally cannot reach. Competes against the SAME
        // exact-match alternative via the SAME bits-based cost model as every
        // other candidate here; only committed if it's cheaper.
        if (pnra && tcopy && i >= 1 && i + 4 <= n && (d[i-1]==0xE8 || d[i-1]==0xE9) && work<kSparseBlockBudget) {
            ++g_pnra_gate;
            auto it = pnra_idx.find([&]{
                int32_t rel32; std::memcpy(&rel32, d.data()+i, 4);
                return i + 4u + static_cast<uint32_t>(rel32);
            }());
            if (it != pnra_idx.end()) {
                auto& v = it->second;
                auto ub = std::upper_bound(v.begin(), v.end(), i - 1);
                if (ub != v.begin()) {
                    ++g_pnra_idxhit;
                    uint32_t q = *(ub - 1);
                    if (q < i && (i - q) >= 4) {
                        SparseMatch pm;
                        if (mf.find_pnra_at(i, q, pm, litcost, avg_lit, work, dead_band)) {
                            ++g_pnra_verify;
                            double pnra_c = 1.5+varint_cost(pm.len-4)+varint_cost(pm.dist-1)+double(pm.len)/8.0
                                          +0.18*std::log2(double(pm.dist)+1.0);
                            for (size_t k=0;k<pm.off.size();++k) pnra_c += litcost[pm.val[k]];
                            double alt_c=std::numeric_limits<double>::infinity();
                            if(exact.len>=4) alt_c=exact_c+(pref[i+pm.len]-pref[i+std::min<uint32_t>(exact.len,pm.len)]);
                            else alt_c=pref[i+pm.len]-pref[i];
                            if (pnra_c < alt_c) {
                                ++g_pnra_commit;
                                SparseToken t; t.type=3u; t.pos=i; t.len=pm.len; t.dist=pm.dist;
                                t.off=std::move(pm.off); t.val=std::move(pm.val); t.tfo=std::move(pm.tfo);
                                toks.push_back(std::move(t));
                                if(boundary) mf.insert_boundary(i);
                                reinforce(pm.dist,i,double(pm.len)*0.5); note_span(pm.dist, pm.len);
                                uint32_t end=i+pm.len;
                                for(uint32_t p=i;p<end;++p) mf.insert(p);
                                i=end;
                                continue;
                            }
                        }
                    }
                }
            }
        }
        if(exact.len<128 && work<kSparseBlockBudget) {
            SparseMatch sm;
            if(mf.find_sparse(i,sm,litcost,avg_lit,work,dead_band,tcopy)) {
                double sparse_c=1.5+varint_cost(sm.len-4)+varint_cost(sm.dist-1)+double(sm.len)/8.0
                               +0.18*std::log2(double(sm.dist)+1.0);
                for(size_t k=0;k<sm.off.size();++k) sparse_c+=litcost[sm.val[k]];
                // prefer the structural channel on near-ties (alignment for R2).
                // The bias scales with the channel's reinforcement score so a
                // LOCKED (high-score) period wins on near-ties, but a weak or
                // stale channel never overrides a clearly better candidate.
                double margin = 2.0;
                if (has_ch) for (size_t c = 0; c < nchan; ++c)
                    if (chan[c].dist == ch_sm.dist) margin += std::min(chan[c].score * 0.06, 12.0);
                double best_c = sparse_c;
                if(has_ch && ch_cost < best_c + margin) { best_c = ch_cost; sm = std::move(ch_sm); has_ch=false; took_ch = true; }
                double alt_c=std::numeric_limits<double>::infinity();
                if(exact.len>=4) alt_c=exact_c+(pref[i+sm.len]-pref[i+std::min<uint32_t>(exact.len,sm.len)]);
                else alt_c=pref[i+sm.len]-pref[i];
                if(best_c<alt_c) {
                    SparseToken t; t.type=(tcopy && !sm.tfo.empty())?3u:2u; t.pos=i; t.len=sm.len; t.dist=sm.dist;
                    t.off=std::move(sm.off); t.val=std::move(sm.val); t.tfo=std::move(sm.tfo);
                    toks.push_back(std::move(t));
                    if(took_ch) note_ch_emit(t.dist);
                    took_ch = false;
                    if(boundary) mf.insert_boundary(i);
                    reinforce(sm.dist,i,double(sm.len)*0.5); note_span(sm.dist, sm.len);
                    uint32_t end=i+sm.len;
                    for(uint32_t p=i;p<end;++p) mf.insert(p);
                    i=end;
                    continue;
                }
                took_ch = false;
            } else if(has_ch) {
                double alt_c=std::numeric_limits<double>::infinity();
                if(exact.len>=4) alt_c=exact_c+(pref[i+ch_sm.len]-pref[i+std::min<uint32_t>(exact.len,ch_sm.len)]);
                else alt_c=pref[i+ch_sm.len]-pref[i];
                if(ch_cost<alt_c) {
                    SparseToken t; t.type=(tcopy && !ch_sm.tfo.empty())?3u:2u; t.pos=i; t.len=ch_sm.len; t.dist=ch_sm.dist;
                    t.off=std::move(ch_sm.off); t.val=std::move(ch_sm.val); t.tfo=std::move(ch_sm.tfo);
                    toks.push_back(std::move(t));
                    note_ch_emit(t.dist);
                    if(boundary) mf.insert_boundary(i);
                    reinforce(t.dist,i,double(t.len)*0.5); note_span(t.dist, t.len);
                    uint32_t end=i+t.len;
                    for(uint32_t p=i;p<end;++p) mf.insert(p);
                    i=end;
                    continue;
                }
            }
        } else if(has_ch && !ch_span) {
            double alt_c=std::numeric_limits<double>::infinity();
            if(exact.len>=4) alt_c=exact_c+(pref[i+ch_sm.len]-pref[i+std::min<uint32_t>(exact.len,ch_sm.len)]);
            else alt_c=pref[i+ch_sm.len]-pref[i];
            if(ch_cost<alt_c) {
                SparseToken t; t.type=(tcopy && !ch_sm.tfo.empty())?3u:2u; t.pos=i; t.len=ch_sm.len; t.dist=ch_sm.dist;
                t.off=std::move(ch_sm.off); t.val=std::move(ch_sm.val); t.tfo=std::move(ch_sm.tfo);
                toks.push_back(std::move(t));
                note_ch_emit(t.dist);
                if(boundary) mf.insert_boundary(i);
                reinforce(t.dist,i,double(t.len)*0.5); note_span(t.dist, t.len);
                uint32_t end=i+t.len;
                for(uint32_t p=i;p<end;++p) mf.insert(p);
                i=end;
                continue;
            }
        }
        // A match must beat the literal cost of the SAME span it covers, not one byte.
        if(exact.len>=4 && exact_c<(pref[i+exact.len]-pref[i])) {
            SparseToken t; t.type=1; t.pos=i; t.len=exact.len; t.dist=exact.dist;
            toks.push_back(std::move(t));
            if(boundary) mf.insert_boundary(i);
            reinforce(exact.dist,i,double(exact.len)*0.25); note_span(exact.dist, exact.len);            uint32_t end=i+exact.len;
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

// ---- Context-switched literal coder (stream mode 6, t4-entropy) ------------
// ONE physical context-switched rANS: a single state whose frequency table is
// selected per symbol by a sparse-support quantizer. The context = the previous
// decoded symbol of the same stream, mapped through a learned K-context map
// (256 -> K groups, K=12, Lloyd-clustered on the per-context symbol
// distributions, ~1.65 ms). NOT multi-stream fan-out: one rANS stream, K tables.
struct CtxModel {
    uint8_t K = 0;
    std::array<uint8_t,256> map{};      // prev-symbol -> context
    std::array<RansModel, 12> m{};
};

static constexpr uint32_t kCtxK = 12;

static CtxModel build_ctx_model(const std::vector<uint8_t>& src, uint32_t tot) {
    CtxModel cm; cm.K = kCtxK;
    std::array<std::array<uint32_t,256>,256> cnt{};
    std::array<uint32_t,256> prev_tot{};
    uint8_t prev = 0;
    for (uint8_t b : src) { ++cnt[prev][b]; ++prev_tot[prev]; prev = b; }
    // Lloyd clustering of the 256 prev distributions into K groups (L2 unit vectors)
    std::array<uint8_t,256> group{};
    std::vector<uint32_t> seeds;
    {
        std::vector<std::pair<uint32_t,uint8_t>> order;
        for (int p = 0; p < 256; ++p) if (prev_tot[p]) order.push_back({prev_tot[p], uint8_t(p)});
        std::sort(order.rbegin(), order.rend());
        for (size_t i = 0; i < order.size() && seeds.size() < kCtxK; ++i) seeds.push_back(order[i].second);
        for (int p = 0; p < 256; ++p) group[p] = 0;
    }
    if (seeds.empty()) return cm;
    std::array<std::array<double,256>, kCtxK> centroid{};
    std::array<double, kCtxK> cnorm{}; // L2 norms of the centroids
    // init centroids from the seeds (L2 unit vectors)
    for (uint8_t g = 0; g < kCtxK && g < seeds.size(); ++g) {
        double n2 = 0; for (int s = 0; s < 256; ++s) n2 += double(cnt[seeds[g]][s]) * cnt[seeds[g]][s];
        double inv = n2 > 0 ? 1.0 / std::sqrt(n2) : 0;
        for (int s = 0; s < 256; ++s) { centroid[g][s] = double(cnt[seeds[g]][s]) * inv; cnorm[g] += centroid[g][s] * centroid[g][s]; }
        cnorm[g] = std::sqrt(cnorm[g]);
    }
    // normalize the centroids (the seeds were unit already; keep the invariant)
    auto assign = [&]() {
        for (int p = 0; p < 256; ++p) {
            if (prev_tot[p] == 0) { group[p] = 0; continue; }
            double n2 = 0; for (int s = 0; s < 256; ++s) n2 += double(cnt[p][s]) * cnt[p][s];
            double inv = n2 > 0 ? 1.0 / std::sqrt(n2) : 0;
            double best = 1e300; uint8_t bg = 0;
            for (uint8_t g = 0; g < kCtxK; ++g) {
                if (cnorm[g] <= 0) continue;
                double ct = 0; for (int s = 0; s < 256; ++s) ct += double(cnt[p][s]) * inv * centroid[g][s];
                double d = 1.0 - ct; // cosine distance (both unit)
                if (d < best) { best = d; bg = g; }
            }
            group[p] = bg;
        }
    };
    for (int it = 0; it < 3; ++it) {
        assign();
        std::array<std::array<double,256>, kCtxK> sum{};
        std::array<double, kCtxK> c2{};
        for (int p = 0; p < 256; ++p) {
            uint8_t g = group[p];
            double n2 = 0; for (int s = 0; s < 256; ++s) n2 += double(cnt[p][s]) * cnt[p][s];
            double inv = n2 > 0 ? 1.0 / std::sqrt(n2) : 0;
            for (int s = 0; s < 256; ++s) { double v = double(cnt[p][s]) * inv; sum[g][s] += v; c2[g] += v * v; }
        }
        for (uint8_t g = 0; g < kCtxK; ++g) {
            if (c2[g] > 0) { double inv = 1.0 / std::sqrt(c2[g]); for (int s = 0; s < 256; ++s) centroid[g][s] = sum[g][s] * inv; cnorm[g] = 1.0; }
            else { for (int s = 0; s < 256; ++s) centroid[g][s] = 0; cnorm[g] = 0; }
        }
    }
    assign();
    // compact the non-empty groups to [0, K_eff); renumber the map
    std::array<uint8_t, kCtxK> renum{};
    uint8_t keff = 0;
    std::array<std::array<uint32_t,256>, kCtxK> gcount{};
    for (int g = 0; g < kCtxK; ++g) {
        uint32_t tot_g = 0; for (int s = 0; s < 256; ++s) { gcount[g][s] = 0; }
        bool any = false;
        for (int p = 0; p < 256; ++p) if (group[p] == g && prev_tot[p]) any = true;
        if (!any) continue;
        renum[g] = keff++;
        for (int p = 0; p < 256; ++p) if (group[p] == g) for (int s = 0; s < 256; ++s) gcount[renum[g]][s] += cnt[p][s];
    }
    if (keff == 0) keff = 1;
    cm.K = keff;
    for (int p = 0; p < 256; ++p) cm.map[p] = renum[group[p]];
    for (uint8_t g = 0; g < keff; ++g) {
        uint32_t tot_g = 0; for (int s = 0; s < 256; ++s) tot_g += gcount[g][s];
        if (tot_g == 0) continue;
        std::vector<uint8_t> synthetic; synthetic.reserve(tot_g);
        for (int s = 0; s < 256; ++s) for (uint32_t j = 0; j < gcount[g][s]; ++j) synthetic.push_back(uint8_t(s));
        cm.m[g] = build_rans_model(synthetic, tot);
    }
    return cm;
}

static std::vector<uint8_t> ctx_rans_encode(const std::vector<uint8_t>& src, const CtxModel& cm, const RansSpec& sp) {
    if (src.empty()) return {};
    uint32_t x = sp.L; std::vector<uint8_t> emitted; emitted.reserve(src.size()/2 + 16);
    uint8_t prev = 0;
    for (size_t ii = src.size(); ii-- > 0;) {
        uint8_t sym = src[ii];
        uint8_t ctx = ii > 0 ? cm.map[src[ii-1]] : cm.map[prev];
        const RansModel& m = cm.m[ctx];
        uint32_t f = m.freq[sym], st = m.start[sym];
        uint32_t x_max = ((sp.L >> sp.scale_bits) << 8) * f;
        while (x >= x_max) { emitted.push_back(static_cast<uint8_t>(x)); x >>= 8; }
        x = ((x / f) << sp.scale_bits) + (x % f) + st;
    }
    (void)prev;
    std::vector<uint8_t> out(4);
    out[0] = static_cast<uint8_t>(x); out[1] = static_cast<uint8_t>(x >> 8); out[2] = static_cast<uint8_t>(x >> 16); out[3] = static_cast<uint8_t>(x >> 24);
    out.reserve(4 + emitted.size());
    for (auto it = emitted.rbegin(); it != emitted.rend(); ++it) out.push_back(*it);
    return out;
}

static std::vector<uint8_t> ctx_rans_decode(const uint8_t* p, size_t n, size_t out_n, const CtxModel& cm, const RansSpec& sp) {
    if (out_n == 0) return {};
    if (n < 4) throw std::runtime_error("truncated ctx rANS state");
    const uint8_t* q = p; const uint8_t* e = p + n; uint32_t x = get_u32le(q, e);
    std::array<std::vector<uint8_t>, kCtxK> symtab;
    for (uint8_t g = 0; g < kCtxK; ++g) {
        symtab[g].assign(sp.tot, 0);
        for (int s = 0; s < 256; ++s) if (cm.m[g].freq[s]) for (uint32_t j = 0; j < cm.m[g].freq[s]; ++j) symtab[g][cm.m[g].start[s] + j] = static_cast<uint8_t>(s);
    }
    std::vector<uint8_t> out(out_n);
    uint8_t prev = 0;
    for (size_t i = 0; i < out_n; ++i) {
        uint8_t ctx = cm.map[prev];
        const RansModel& m = cm.m[ctx];
        uint32_t slot = x & (sp.tot - 1);
        uint8_t sym = symtab[ctx][slot]; out[i] = sym;
        x = uint32_t(m.freq[sym]) * (x >> sp.scale_bits) + slot - m.start[sym];
        while (x < sp.L) { if (q >= e) throw std::runtime_error("truncated ctx rANS renorm"); x = (x << 8) | *q++; }
        prev = sym;
    }
    if (q != e) throw std::runtime_error("trailing ctx rANS bytes");
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
    std::array<uint16_t,256> first_code{}; // first code of each length (canonical, read MSB-first)
    std::array<uint16_t,256> first_sym{};  // first symbol index of each length
    std::array<uint16_t,256> n_codes{};
    std::array<uint8_t,256> order{};
    uint8_t max_len=0, n_ord=0;
    std::array<uint16_t,4096> tbl{}; // 12-bit decode table: (sym<<4)|len; 0xFFFF = long-code marker
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
    // 12-bit decode table
    h.tbl.fill(0xFFFFu);
    if (h.max_len <= 12) {
        for (uint32_t i = 0; i < cnt; ++i) {
            uint8_t s = syms[i]; uint8_t l = len[s];
            uint32_t base = code[s] << (12 - l);
            uint16_t entry = uint16_t((s << 4) | l);
            for (uint32_t k = 0; k < (1u << (12 - l)); ++k) h.tbl[base + k] = entry;
        }
    }
    return h;
}

static std::vector<uint8_t> huffman_decode(const uint8_t* p, size_t n, size_t out_n, const HuffModel& h) {
    std::vector<uint8_t> out(out_n);
    const uint8_t* e=p+n;
    uint64_t acc=0; int have=0;
    auto refill=[&](int need){ while(have<need && p<e){ acc=(acc<<8)|*p++; have+=8; } };
    if (h.max_len <= 12) {
        // table-driven: one lookup per symbol
        for(size_t i=0;i<out_n;++i){
            refill(12);
            int win = have; if (win > 12) win = 12;
            uint32_t code=(uint32_t)((acc>>(have-win)) & ((1u<<win)-1));
            uint32_t idx = code << (12 - win); // align the win-bit code to the table's top bits
            uint16_t entry = h.tbl[idx];
            if (entry == 0xFFFFu) throw std::runtime_error("invalid huffman code");
            int used = entry & 0xF;
            if (used > win) throw std::runtime_error("invalid huffman code");
            out[i] = uint8_t(entry >> 4);
            have -= used;
        }
        return out;
    }
    for(size_t i=0;i<out_n;++i){
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
// J = L + lambda*C_decode (+ mu*C_model + nu*W_cache, defaults 0), the
// pre-registered jcost form. C_decode = per-STREAM decode cost units:
//   raw 10, rans-4096 40, rans-512 35, rans-256 30, huffman 22, defexc 20
// lambda pre-registered binding value = 0.01 (Options.stream_lambda /
// ANVIL_STREAM_LAMBDA / --stream-lambda override; 0 = pure length).
static double g_stream_lambda = 0.01;
static double g_stream_mu = 0.0, g_stream_nu = 0.0;
static bool g_stream_suite = true;   // false = fixed rANS-4096 + raw (pre-suite behavior)
static bool g_stream_ctx = true;      // true = context-switched rANS (mode 6) enabled in the suite
static bool g_stream_log = false;    // --stream-log: record per-stream selection
static bool g_fused_decode = true;   // mode-12 fused single-path decode (t3-fuse); false = separated-stream A/B
static uint64_t g_j_agree = 0, g_j_total = 0; // J-selection vs pure-L agreement counters
struct StreamLogEntry { uint32_t chosen, l_winner; size_t chosen_L, min_L; };
static std::vector<StreamLogEntry> g_stream_log_entries;

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

static std::vector<uint8_t> ctx_stream_bytes(const std::vector<uint8_t>& src, const RansSpec& sp) {
    CtxModel cm = build_ctx_model(src, sp.tot);
    auto rd = ctx_rans_encode(src, cm, sp);
    std::vector<uint8_t> z; z.push_back(6); put_uvar(z, src.size());
    z.push_back(cm.K); // number of contexts (compacted)
    for (int i = 0; i < 256; ++i) z.push_back(cm.map[i]); // context map
    for (uint8_t g = 0; g < cm.K; ++g) {
        uint32_t nz = 0; for (auto f : cm.m[g].freq) if (f) ++nz;
        put_uvar(z, nz);
        for (int i = 0; i < 256; ++i) if (cm.m[g].freq[i]) { z.push_back(uint8_t(i)); put_uvar(z, cm.m[g].freq[i]); }
    }
    put_uvar(z, rd.size()); z.insert(z.end(), rd.begin(), rd.end());
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
    auto add=[&](std::vector<uint8_t> b, double cu){ double L=double(b.size()); cands.push_back({std::move(b), L + g_stream_lambda*cu + g_stream_mu*2.0 + g_stream_nu*cu}); };
    add(std::move(raw), 10.0); // raw is always a candidate (per-stream fallback)
    add(rans_stream_bytes(src,kRans4096,1), 40.0);
    if(g_stream_suite) {
        add(rans_stream_bytes(src,kRans512,2), 35.0);
        add(rans_stream_bytes(src,kRans256,3), 30.0);
        auto hlen=huffman_lengths(src);
        add(huffman_stream_bytes(src,hlen), 22.0);
        {
            std::array<uint32_t,256> cnt{}; for(uint8_t b:src) ++cnt[b];
            uint8_t def=0; for(int i=1;i<256;++i) if(cnt[i]>cnt[def]) def=uint8_t(i);
            if(cnt[def]>=src.size()/2) add(defexc_stream_bytes(src,def), 20.0);
        }
        // context-switched rANS (mode 6): only for streams large enough to
        // amortize the 256-byte context map + K model headers (~1.65ms learner)
        if(g_stream_ctx && src.size() >= 4096) add(ctx_stream_bytes(src,kRans4096), 45.0);
    }
    const Cand* best=&cands[0];
    for(auto& c:cands) if(c.J<best->J) best=&c;
    // J-prediction accounting: ratio-faithfulness = J-winner's length within 1% of
    // the smallest-length codec (the decode-cost term must not mis-pick badly).
    if(g_stream_suite && cands.size()>1) {
        size_t lw=0; for(size_t i=1;i<cands.size();++i) if(cands[i].bytes.size()<cands[lw].bytes.size()) lw=i;
        ++g_j_total;
        if(best->bytes.size() <= cands[lw].bytes.size()*101/100) ++g_j_agree;
        if(g_stream_log) g_stream_log_entries.push_back({uint32_t(best-&cands[0]+1), uint32_t(lw), best->bytes.size(), cands[lw].bytes.size()});
    }
    return best->bytes;
}

// Ratio-only stream selector: exact smallest encoded byte count, with the same
// stable stream-codec wire IDs as encode_stream().  This intentionally ignores
// the decode-cost J term: backend ratio experiments must not conflate a stream
// economics policy with the representation/postcoder comparison.
static std::vector<uint8_t> encode_stream_smallest(const std::vector<uint8_t>& src) {
    std::vector<std::vector<uint8_t>> cands;
    std::vector<uint8_t> raw; raw.push_back(0); put_uvar(raw,src.size()); raw.insert(raw.end(),src.begin(),src.end());
    cands.push_back(std::move(raw));
    if(src.size()>=16) {
        cands.push_back(rans_stream_bytes(src,kRans4096,1));
        cands.push_back(rans_stream_bytes(src,kRans512,2));
        cands.push_back(rans_stream_bytes(src,kRans256,3));
        cands.push_back(huffman_stream_bytes(src,huffman_lengths(src)));
        std::array<uint32_t,256> cnt{}; for(uint8_t b:src) ++cnt[b];
        uint8_t def=0; for(int i=1;i<256;++i) if(cnt[i]>cnt[def]) def=uint8_t(i);
        if(cnt[def]>=src.size()/2) cands.push_back(defexc_stream_bytes(src,def));
        if(src.size()>=4096) cands.push_back(ctx_stream_bytes(src,kRans4096));
    }
    size_t best=0;
    for(size_t i=1;i<cands.size();++i) if(cands[i].size()<cands[best].size()) best=i;
    return std::move(cands[best]);
}

// ---- S6-1 whole-codec stream budget (mode-15 hot-op streams) ----------------
// Per-stream codec choice by the PRE-REGISTERED additive J = L + lambda*C_decode
// (lambda = 0.01 bytes/us, the EXP. L binding constant) with C_decode
// SIZE-PROPORTIONAL: raw_n * ns_per_byte(codec) / 1000 us. ns_per_byte constants
// are the measured calibration from decode-perf's t3 floor profile (blackboard
// deliverable/t3-decode-floor-profile; fixed BEFORE any verdict measurement):
//   raw 0.1 (bulk pull) | rANS-4096/512/256 6.0 | huffman 4.3 | defexc 3.2 | ctx 7.5
// Candidate set == the existing suite + raw. NO new codecs (rlzp stays
// orthogonal per gate rule Y-1). Used ONLY by the mode-15 hot-op encoder behind
// --hotop-budget=on (default off = byte-identical legacy path). Wire-invisible:
// selection reuses the existing per-substream mode bytes (format's determination).
static double g_hotop_budget = false;
static constexpr double kBudgetNsPerByte[7] = {
    0.1, // 0 raw (bulk memcpy-class pull)
    6.0, // 1 rANS-4096
    6.0, // 2 rANS-512
    6.0, // 3 rANS-256
    4.3, // 4 huffman (t3: data-dependent 4-9; table value)
    3.2, // 5 defexc
    7.5, // 6 ctx-rANS
};

static std::vector<uint8_t> encode_stream_budget(const std::vector<uint8_t>& src) {
    std::vector<uint8_t> raw; raw.push_back(0); put_uvar(raw,src.size()); raw.insert(raw.end(),src.begin(),src.end());
    if(src.size()<16) return raw;
    struct Cand { std::vector<uint8_t> bytes; double J; };
    std::vector<Cand> cands;
    auto add=[&](std::vector<uint8_t> b, uint32_t codec){
        double L = double(b.size());
        double C_us = double(src.size()) * kBudgetNsPerByte[codec] / 1000.0;
        cands.push_back({std::move(b), L + g_stream_lambda * C_us});
    };
    add(std::move(raw), 0); // raw is always a candidate (the S6-1 raw-stream budget)
    add(rans_stream_bytes(src,kRans4096,1), 1);
    if(g_stream_suite) {
        add(rans_stream_bytes(src,kRans512,2), 2);
        add(rans_stream_bytes(src,kRans256,3), 3);
        add(huffman_stream_bytes(src,huffman_lengths(src)), 4);
        {
            std::array<uint32_t,256> cnt{}; for(uint8_t b:src) ++cnt[b];
            uint8_t def=0; for(int i=1;i<256;++i) if(cnt[i]>cnt[def]) def=uint8_t(i);
            if(cnt[def]>=src.size()/2) add(defexc_stream_bytes(src,def), 5);
        }
        if(g_stream_ctx && src.size() >= 4096) add(ctx_stream_bytes(src,kRans4096), 6);
    }
    const Cand* best=&cands[0];
    for(auto& c:cands) if(c.J<best->J) best=&c;
    return best->bytes;
}

// ---- RLZ-RePair alternative encoding of hot-op book streams (t-hotop) ------
// Two self-contained STREAM codecs, candidates for the mode-15 book streams,
// selected by size in the hotop encoder when --hotop-rlzp=on (OFF by default):
//   mode 7 = RePair-style grammar: repeated digram factoring (Larsson & Moffat
//            1998). Folds recurring multi-symbol patterns of a book stream into
//            nonterminal rules; the residual (reduced sequence) is what the
//            entropy coder sees, so decode materializes the whole stream with
//            fewer per-symbol entropy pulls + cheap rule expansion.
//   mode 8 = RLZ (relative-Lempe-Ziv, Kurup/Marin/Ziv 2010): here self-
//            reference (the stream's own earlier prefix = LZ77) with memcpy
//            decode, aimed at the literal/residual streams.
// Both decode eagerly in the pull's parse into a byte buffer, so the fused
// hot-op executor runs the opcode/literal streams as plain buffer walks.
// Nothing here is added to encode_stream's general candidate set (that would
// recurse); the hotop encoder explicitly tries repair/rlz against the suite
// and picks the smallest. Pre-registered in RESEARCH_LEDGER (Experiment Y).

static constexpr uint32_t kRlzReapMaxRules = 768;   // RePair rule cap (bounds symbol ids 256..256+R-1)
static constexpr uint32_t kRlzWindow          = 64u*1024; // RLZ self-reference window
static constexpr uint32_t kRlzMinMatch        = 4;        // min RLZ match length

static bool g_rlz_reap = false;                                  // --hotop-rlzp=on
static uint64_t g_m7_used = 0, g_m8_used = 0, g_m7_tried = 0;    // attribution counters

// RePair grammar encoder -> full mode-7 wire bytes in `out` (empty = not worth it).
static void repair_stream_bytes(const std::vector<uint8_t>& src, std::vector<uint8_t>& out) {
    out.clear();
    const size_t N = src.size();
    if (N < 8) return;
    std::vector<uint32_t> seq; seq.reserve(N); for (auto b : src) seq.push_back(b);
    std::vector<std::pair<uint32_t,uint32_t>> rules;
    std::unordered_set<uint64_t> banned; // low-value pairs skipped this pass (no-progress guard)
    for (uint32_t r = 0; r < kRlzReapMaxRules; ++r) {
        if (seq.size() < 2) break;
        std::unordered_map<uint64_t,uint32_t> cnt; cnt.reserve(seq.size());
        uint64_t bestk = 0; uint32_t bestc = 0;
        for (size_t i = 0; i + 1 < seq.size(); ++i) {
            uint64_t key = (uint64_t(seq[i]) << 32) | seq[i+1];
            uint32_t c = ++cnt[key];
            if (banned.count(key)) { if (c > 2) banned.erase(key); continue; } // re-eligible once it grows past the guard
            if (c > bestc) { bestc = c; bestk = key; }
        }
        if (bestc < 2) break;
        uint32_t a = uint32_t(bestk >> 32), b = uint32_t(bestk & 0xFFFFFFFFu);
        // low-value guard: merging a (X,X) pair whose only payoff is one new
        // symbol buys no digram reduction. Ban + rescan instead of continue
        // (an unbanned rescan would find the identical state forever).
        if (a == b && bestc == 2 && a >= 256) { banned.insert(bestk); continue; }
        uint32_t ns = 256 + uint32_t(rules.size());
        rules.push_back({a, b});
        std::vector<uint32_t> ns2; ns2.reserve(seq.size());
        size_t i = 0;
        while (i < seq.size()) {
            if (i + 1 < seq.size() && seq[i] == a && seq[i+1] == b) { ns2.push_back(ns); i += 2; }
            else { ns2.push_back(seq[i]); ++i; }
        }
        seq = std::move(ns2);
    }
    if (rules.empty()) return; // no factoring found
    std::vector<uint8_t> body;
    put_uvar(body, rules.size());
    for (auto& rp : rules) { put_uvar(body, rp.first); put_uvar(body, rp.second); }
    put_uvar(body, seq.size());
    for (auto s : seq) put_uvar(body, s);
    auto inner = encode_stream(body);
    out.push_back(7);
    put_uvar(out, N);
    put_uvar(out, inner.size());
    out.insert(out.end(), inner.begin(), inner.end());
}

// RLZ (self-reference LZ77) encoder -> full mode-8 wire bytes in `out` (empty = not worth it).
static void rlz_stream_bytes(const std::vector<uint8_t>& src, std::vector<uint8_t>& out) {
    out.clear();
    const size_t N = src.size();
    if (N < 8) return;
    const uint8_t* d = src.data();
    std::unordered_map<uint32_t,std::vector<uint32_t>> pos; // 4-byte hash -> positions
    for (uint32_t p = 0; p + 4 <= N; ++p) pos[hash4(d + p)].push_back(p);
    struct Op { uint8_t kind; uint32_t val, val2; std::vector<uint8_t> lit; };
    std::vector<Op> ops;
    size_t i = 0;
    while (i < N) {
        size_t bestj = SIZE_MAX; uint32_t bestlen = 0;
        if (i + 4 <= N) {
            auto it = pos.find(hash4(d + i));
            if (it != pos.end()) {
                for (uint32_t j : it->second) {
                    if (j >= i || i - j > kRlzWindow) { if (j >= i) break; else continue; }
                    uint32_t lim = uint32_t(std::min<size_t>(N - i, kRlzWindow));
                    uint32_t l = 0;
                    while (l < lim && d[i + l] == d[j + l]) ++l;
                    if (l > bestlen) { bestlen = l; bestj = j; }
                }
            }
        }
        if (bestlen >= kRlzMinMatch) {
            // wire convention (must match decode_stream mode 8): dist = read+1,
            // len = read+kRlzMinMatch -> store (d-1) and (len-kRlzMinMatch).
            ops.push_back({1, uint32_t(i - bestj) - 1, bestlen - kRlzMinMatch, {}});
            i += bestlen;
        } else {
            if (ops.empty() || ops.back().kind != 0 || ops.back().lit.size() >= 256*8) ops.push_back({0, 0, 0, {}});
            ops.back().lit.push_back(d[i]); ++i;
        }
    }
    if (ops.empty()) return;
    std::vector<uint8_t> body;
    put_uvar(body, ops.size());
    for (auto& op : ops) {
        body.push_back(op.kind);
        if (op.kind == 0) { put_uvar(body, op.lit.size()); body.insert(body.end(), op.lit.begin(), op.lit.end()); }
        else { put_uvar(body, op.val); put_uvar(body, op.val2); }
    }
    auto inner = encode_stream(body);
    out.push_back(8);
    put_uvar(out, N);
    put_uvar(out, inner.size());
    out.insert(out.end(), inner.begin(), inner.end());
}

static std::vector<uint8_t> decode_stream(const uint8_t*&p,const uint8_t*e, size_t max_n, int depth=0) {
    if(p>=e) throw std::runtime_error("truncated stream header");
    uint8_t mode=*p++; uint64_t raw_n=get_uvar(p,e);
    if(raw_n>max_n)throw std::runtime_error("stream too large"); // DoS guard: bound by block out_len
    if(mode>=7 && depth>0) throw std::runtime_error("nested rlz-reap stream"); // legit nesting depth is exactly 1 (F4)
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
    if(mode==6) {
        if(uint64_t(e-p)<257) throw std::runtime_error("truncated ctx header");
        CtxModel cm; cm.K = *p++;
        if (cm.K == 0 || cm.K > kCtxK) throw std::runtime_error("bad ctx K");
        for (int i = 0; i < 256; ++i) cm.map[i] = *p++;
        for (uint8_t g = 0; g < cm.K; ++g) {
            uint64_t nz = get_uvar(p, e); if (nz > 256) throw std::runtime_error("bad ctx model");
            uint32_t sum = 0;
            for (uint64_t k = 0; k < nz; ++k) {
                if (p >= e) throw std::runtime_error("truncated ctx model");
                uint8_t sym = *p++; uint64_t f = get_uvar(p, e);
                if (f == 0 || f > kRans4096.tot || cm.m[g].freq[sym]) throw std::runtime_error("bad ctx frequency");
                cm.m[g].freq[sym] = static_cast<uint16_t>(f); sum += static_cast<uint32_t>(f);
            }
            if (sum != kRans4096.tot) throw std::runtime_error("bad ctx total");
            uint32_t st = 0; for (int i = 0; i < 256; ++i) { cm.m[g].start[i] = static_cast<uint16_t>(st); st += cm.m[g].freq[i]; }
        }
        uint64_t dn = get_uvar(p, e); if (dn > uint64_t(e - p)) throw std::runtime_error("truncated ctx rANS stream");
        auto out = ctx_rans_decode(p, static_cast<size_t>(dn), static_cast<size_t>(raw_n), cm, kRans4096); p += dn; return out;
    }
    if(mode==7) { // RePair grammar
        if(raw_n>max_n) throw std::runtime_error("stream too large");
        uint64_t ilen=get_uvar(p,e);
        if(ilen>uint64_t(e-p)) throw std::runtime_error("truncated reap inner");
        const uint8_t* q=p; const uint8_t* qe=p+ilen;
        auto body=decode_stream(q,qe,max_n,depth+1);
        if(q!=qe) throw std::runtime_error("reap inner trailing bytes");
        p=qe;
        const uint8_t* bp=body.data(); const uint8_t* be=body.data()+body.size();
        uint64_t R=get_uvar(bp,be); if(R>kRlzReapMaxRules) throw std::runtime_error("bad reap rules");
        std::vector<uint32_t> L(static_cast<size_t>(R)), Rt(static_cast<size_t>(R));
        for(size_t i=0;i<R;++i){
            uint64_t a=get_uvar(bp,be), b=get_uvar(bp,be);
            if(a>=256+R || b>=256+R) throw std::runtime_error("bad reap rule sym");
            L[i]=static_cast<uint32_t>(a); Rt[i]=static_cast<uint32_t>(b);
        }
        uint64_t M=get_uvar(bp,be); if(M>raw_n) throw std::runtime_error("bad reap reduced len");
        std::vector<uint64_t> red(static_cast<size_t>(M));
        for(size_t j=0;j<M;++j){ uint64_t s=get_uvar(bp,be); if(s>=256+R) throw std::runtime_error("bad reap reduced sym"); red[j]=s; }
        if(bp!=be) throw std::runtime_error("reap body trailing bytes");
        std::vector<std::vector<uint8_t>> exp(static_cast<size_t>(R));
        { // F3 amplification bound: legit grammars keep every rule >=2 refs in the
          // final structure (folding conserves references), so |exp[i]| <= raw_n/2;
          // enforce per-rule <= raw_n and cumulative <= 2*raw_n + 64 KiB.
            uint64_t cum = 0;
            for(size_t i=0;i<R;++i){
                auto app=[&](uint32_t s,std::vector<uint8_t>&v){ if(s<256){v.push_back(static_cast<uint8_t>(s));} else { if((s-256)>=i) throw std::runtime_error("reap rule forward ref"); v.insert(v.end(),exp[s-256].begin(),exp[s-256].end());} };
                app(L[i],exp[i]); app(Rt[i],exp[i]);
                if(exp[i].size()>raw_n) throw std::runtime_error("reap rule expansion too large");
                cum += exp[i].size();
                if(cum > 2*raw_n + 65536) throw std::runtime_error("reap cumulative expansion too large");
            }
        }
        std::vector<uint8_t> out; out.reserve(static_cast<size_t>(raw_n));
        for(auto s:red){ if(s<256){out.push_back(static_cast<uint8_t>(s));} else { auto&v=exp[s-256]; out.insert(out.end(),v.begin(),v.end()); if(out.size()>raw_n) throw std::runtime_error("reap expansion overflow"); } }
        if(out.size()!=raw_n) throw std::runtime_error("reap output-size mismatch");
        return out;
    }
    if(mode==8) { // RLZ (self-reference)
        if(raw_n>max_n) throw std::runtime_error("stream too large");
        uint64_t ilen=get_uvar(p,e);
        if(ilen>uint64_t(e-p)) throw std::runtime_error("truncated rlz inner");
        const uint8_t* q=p; const uint8_t* qe=p+ilen;
        auto body=decode_stream(q,qe,max_n,depth+1);
        if(q!=qe) throw std::runtime_error("rlz inner trailing bytes");
        p=qe;
        const uint8_t* bp=body.data(); const uint8_t* be=body.data()+body.size();
        uint64_t nops=get_uvar(bp,be);
        std::vector<uint8_t> out; out.reserve(static_cast<size_t>(raw_n));
        for(uint64_t k=0;k<nops;++k){
            if(bp>=be) throw std::runtime_error("truncated rlz op");
            uint8_t op=*bp++;
            if(op==0){ uint64_t len=get_uvar(bp,be); if(len>uint64_t(be-bp)||len>raw_n-out.size()) throw std::runtime_error("bad rlz literal"); out.insert(out.end(),bp,bp+len); bp+=len; }
            else if(op==1){ // F2: reject near-2^64 addends before the +1 (wrap would bypass the bounds checks)
                uint64_t dv=get_uvar(bp,be), lv=get_uvar(bp,be);
                if(dv>=0xFFFFFFFFull||lv>=0xFFFFFFFFull) throw std::runtime_error("bad rlz match varint");
                uint64_t dist=dv+1, len=lv+kRlzMinMatch;
                if(dist>out.size()||len>raw_n-out.size()) throw std::runtime_error("bad rlz match");
                for(uint64_t t=0;t<len;++t) out.push_back(out[out.size()-dist]); }
            else throw std::runtime_error("bad rlz op type");
        }
        if(out.size()!=raw_n) throw std::runtime_error("rlz output-size mismatch");
        return out;
    }
    throw std::runtime_error("unknown stream codec");
}

// ---- Fused stream pull (t3-fuse) --------------------------------------------
// Pull-based substream reader: parses a serialized substream header and decodes
// the next byte ON DEMAND (raw / rANS-256/512/4096 / Huffman / defexc). This
// lets the mode-12 token loop fuse entropy decode and reconstruction into one
// pass without materializing full stream vectors (single-path decode, Linux
// stream economics). Same wire as decode_stream; ratio preserved by construction.
struct StreamPull {
    uint8_t codec = 0;
    uint64_t remaining = 0;        // output bytes left to decode
    uint64_t total = 0;            // total output bytes (raw_n)
    const uint8_t* p = nullptr;    // parse/byte cursor
    const uint8_t* e = nullptr;    // substream end
    // rANS state
    RansSpec spec{};
    RansModel m{};
    std::vector<uint8_t> symtab;
    uint32_t x = 0;
    const uint8_t* rend = nullptr; // end of the rANS renorm region
    // huffman state
    HuffModel huff;
    uint64_t acc = 0; int have = 0;
    const uint8_t* hp = nullptr; const uint8_t* hend = nullptr;
    // defexc state
    uint8_t def = 0;
    uint64_t nexc = 0;
    uint64_t bits_done = 0;        // mask bits consumed (forward)
    const uint8_t* maskp = nullptr;
    const uint8_t* exc = nullptr; const uint8_t* exc_end = nullptr;
    // ctx rANS (mode 6) state
    CtxModel cctx{};
    std::vector<uint8_t> csymtab; // flattened kCtxK x tot
    uint8_t cprev = 0;
    // mode 7 (RePair) / mode 8 (RLZ) decode eagerly into this buffer in parse;
    // the hot loop then reads a plain byte buffer.
    std::vector<uint8_t> rp_buf; size_t rp_i = 0;

    // Parse the substream header starting at q; on success q advances past the
    // whole substream (q == qe). Throws on malformed input.
    void parse(const uint8_t*& q, const uint8_t* qe, size_t max_n) {
        p = q; e = qe;
        if (p >= e) throw std::runtime_error("truncated stream header");
        codec = *p++;
        uint64_t raw_n = get_uvar(p, e);
        if (raw_n > max_n) throw std::runtime_error("stream too large");
        remaining = raw_n;
        total = raw_n;
        if (codec == 0) {
            if (raw_n > uint64_t(e - p)) throw std::runtime_error("truncated raw stream");
        } else if (codec >= 1 && codec <= 3) {
            spec = codec == 1 ? kRans4096 : codec == 2 ? kRans512 : kRans256;
            uint64_t nz = get_uvar(p, e); if (nz > 256) throw std::runtime_error("bad rANS model");
            uint32_t sum = 0;
            for (uint64_t k = 0; k < nz; ++k) {
                if (p >= e) throw std::runtime_error("truncated rANS model");
                uint8_t sym = *p++; uint64_t f = get_uvar(p, e);
                if (f == 0 || f > spec.tot || m.freq[sym]) throw std::runtime_error("bad rANS frequency");
                m.freq[sym] = static_cast<uint16_t>(f); sum += static_cast<uint32_t>(f);
            }
            if (sum != spec.tot) throw std::runtime_error("bad rANS total");
            uint32_t st = 0; for (int i = 0; i < 256; ++i) { m.start[i] = static_cast<uint16_t>(st); st += m.freq[i]; }
            uint64_t dn = get_uvar(p, e); if (dn > uint64_t(e - p)) throw std::runtime_error("truncated rANS stream");
            if (dn < 4) throw std::runtime_error("truncated rANS state");
            x = uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
            p += 4;
            rend = p + (dn - 4); // renorm bytes follow the 4-byte state
            symtab.assign(spec.tot, 0);
            for (int s = 0; s < 256; ++s) if (m.freq[s]) for (uint32_t j = 0; j < m.freq[s]; ++j) symtab[m.start[s] + j] = static_cast<uint8_t>(s);
            // p now points at the renorm bytes; the dn-4 renorm bytes follow
        } else if (codec == 4) {
            if (uint64_t(e - p) < 256) throw std::runtime_error("truncated huffman lengths");
            std::array<uint8_t, 256> len{};
            for (int i = 0; i < 256; ++i) len[i] = *p++;
            uint64_t kraft = 0;
            for (int i = 0; i < 256; ++i) if (len[i]) { if (len[i] > 24) throw std::runtime_error("bad huffman length"); kraft += 1ull << (24 - len[i]); }
            if (kraft > (1ull << 24)) throw std::runtime_error("huffman overfull");
            huff = build_huff_model(len);
            uint64_t dn = get_uvar(p, e); if (dn > uint64_t(e - p)) throw std::runtime_error("truncated huffman stream");
            hp = p; hend = p + dn; p = hend;
        } else if (codec == 5) {
            if (p >= e) throw std::runtime_error("truncated defexc default");
            def = *p++;
            nexc = get_uvar(p, e);
            uint64_t mask_bytes = (raw_n + 7) / 8;
            if (mask_bytes > uint64_t(e - p)) throw std::runtime_error("truncated defexc mask");
            if (nexc > uint64_t(e - p) - mask_bytes) throw std::runtime_error("truncated defexc values");
            maskp = p; p += mask_bytes;
            exc = p; exc_end = p + nexc; p = exc_end;
            bits_done = 0;
        } else if (codec == 6) {
            if (uint64_t(e - p) < 257) throw std::runtime_error("truncated ctx header");
            cctx.K = *p++;
            if (cctx.K == 0 || cctx.K > kCtxK) throw std::runtime_error("bad ctx K");
            for (int i = 0; i < 256; ++i) cctx.map[i] = *p++;
            for (uint8_t g = 0; g < cctx.K; ++g) {
                uint64_t nz = get_uvar(p, e); if (nz > 256) throw std::runtime_error("bad ctx model");
                uint32_t sum = 0;
                for (uint64_t k = 0; k < nz; ++k) {
                    if (p >= e) throw std::runtime_error("truncated ctx model");
                    uint8_t sym = *p++; uint64_t f = get_uvar(p, e);
                    if (f == 0 || f > kRans4096.tot || cctx.m[g].freq[sym]) throw std::runtime_error("bad ctx frequency");
                    cctx.m[g].freq[sym] = static_cast<uint16_t>(f); sum += static_cast<uint32_t>(f);
                }
                if (sum != kRans4096.tot) throw std::runtime_error("bad ctx total");
                uint32_t st = 0; for (int i = 0; i < 256; ++i) { cctx.m[g].start[i] = static_cast<uint16_t>(st); st += cctx.m[g].freq[i]; }
            }
            csymtab.assign(kCtxK * kRans4096.tot, 0);
            for (uint8_t g = 0; g < kCtxK; ++g)
                for (int s = 0; s < 256; ++s) if (cctx.m[g].freq[s])
                    for (uint32_t j = 0; j < cctx.m[g].freq[s]; ++j) csymtab[g * kRans4096.tot + cctx.m[g].start[s] + j] = static_cast<uint8_t>(s);
            uint64_t dn = get_uvar(p, e); // rANS data length (state + renorm bytes)
            if (dn < 4 || dn > uint64_t(e - p)) throw std::runtime_error("truncated ctx rANS stream");
            x = uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
            p += 4;
            rend = p + dn - 4;
            cprev = 0;
        } else if (codec == 7 || codec == 8) {
            // Re-Pair / RLZ: decode the WHOLE substream eagerly. Build a synthetic
            // range [codec, raw_n uvar, ...rest] and let decode_stream handle it
            // (it is defined above and validates trailing/oversize).
            if (raw_n > max_n) throw std::runtime_error("stream too large");
            std::vector<uint8_t> wf; wf.reserve(size_t(e - p) + 8);
            wf.push_back(static_cast<uint8_t>(codec)); put_uvar(wf, raw_n);
            wf.insert(wf.end(), p, e);
            const uint8_t* q2 = wf.data(); const uint8_t* qe2 = wf.data() + wf.size();
            rp_buf = decode_stream(q2, qe2, max_n);
            if (q2 != qe2) throw std::runtime_error("rlz-reap substream trailing bytes");
            rp_i = 0;
            p = e;
        } else throw std::runtime_error("unknown stream codec");
        q = e; // whole substream consumed by the parser (headers + data region accounted)
    }

    bool next_byte(uint8_t& b) {
        if (remaining == 0) return false;
        if (codec == 0) {
            if (p >= e) throw std::runtime_error("truncated raw stream");
            b = *p++; --remaining; return true;
        }
        if (codec >= 1 && codec <= 3) {
            uint32_t slot = x & (spec.tot - 1);
            uint8_t sym = symtab[slot];
            x = uint32_t(m.freq[sym]) * (x >> spec.scale_bits) + slot - m.start[sym];
            while (x < spec.L) {
                if (p >= rend) throw std::runtime_error("truncated rANS renorm");
                x = (x << 8) | *p++;
            }
            b = sym; --remaining; return true;
        }
        if (codec == 4) {
            if (huff.max_len <= 12) {
                while (have < 12 && hp < hend) { acc = (acc << 8) | *hp++; have += 8; }
                int win = have; if (win > 12) win = 12;
                uint32_t code = (uint32_t)((acc >> (have - win)) & ((1u << win) - 1));
                uint32_t idx = code << (12 - win);
                uint16_t entry = huff.tbl[idx];
                if (entry == 0xFFFFu) throw std::runtime_error("invalid huffman code");
                int used = entry & 0xF;
                if (used > win) throw std::runtime_error("invalid huffman code");
                have -= used; b = uint8_t(entry >> 4); --remaining; return true;
            }
            // slow path
            while (have < 1 && hp < hend) { acc = (acc << 8) | *hp++; have += 8; }
            if (have < 1) throw std::runtime_error("truncated huffman bits");
            uint32_t c2 = 0; bool found = false; uint8_t sym = 0;
            for (int l = 1; l <= 24; ++l) {
                if (have < 1) { if (hp >= hend) throw std::runtime_error("truncated huffman bits"); acc = (acc << 8) | *hp++; have += 8; }
                c2 = (c2 << 1) | (uint32_t)((acc >> (have - 1)) & 1);
                --have;
                if (huff.n_codes[l] && c2 >= huff.first_code[l] && c2 < huff.first_code[l] + huff.n_codes[l]) { sym = huff.order[huff.first_sym[l] + (c2 - huff.first_code[l])]; found = true; break; }
            }
            if (!found) throw std::runtime_error("invalid huffman code");
            b = sym; --remaining; return true;
        }
        // defexc
        if (codec == 5) {
            if (remaining == 0) return false;
            if ((maskp[bits_done >> 3] >> (bits_done & 7)) & 1) {
                if (exc >= exc_end) throw std::runtime_error("truncated defexc values");
                b = *exc++;
            } else b = def;
            ++bits_done;
            --remaining; return true;
        }
        // ctx rANS (mode 6)
        if (codec == 6) {
            if (remaining == 0) return false;
            uint8_t ctx = cctx.map[cprev];
            const RansModel& m = cctx.m[ctx];
            uint32_t slot = x & (kRans4096.tot - 1);
            uint8_t sym = csymtab[ctx * kRans4096.tot + slot];
            x = uint32_t(m.freq[sym]) * (x >> kRans4096.scale_bits) + slot - m.start[sym];
            while (x < kRans4096.L) { if (p >= rend) throw std::runtime_error("truncated ctx rANS renorm"); x = (x << 8) | *p++; }
            b = sym; cprev = sym; --remaining; return true;
        }
        // RePair (7) / RLZ (8): plain buffer walk over the eagerly-decoded buf
        if (codec == 7 || codec == 8) {
            if (remaining == 0) return false;
            if (rp_i >= rp_buf.size()) throw std::runtime_error("truncated rlz-reap pull");
            b = rp_buf[rp_i++]; --remaining; return true;
        }
        throw std::runtime_error("unknown pull codec");
    }

    bool pull_bytes(uint8_t* dst, size_t n) {
        if (n > remaining) return false;
        if (codec == 0) {
            if (n > size_t(e - p)) throw std::runtime_error("truncated raw stream");
            std::memcpy(dst, p, n); p += n; remaining -= n; return true;
        }
        if (codec >= 1 && codec <= 3) {
            // tight rANS bulk decode (no per-byte dispatch)
            for (size_t i = 0; i < n; ++i) {
                uint32_t slot = x & (spec.tot - 1);
                uint8_t sym = symtab[slot];
                x = uint32_t(m.freq[sym]) * (x >> spec.scale_bits) + slot - m.start[sym];
                while (x < spec.L) { if (p >= rend) throw std::runtime_error("truncated rANS renorm"); x = (x << 8) | *p++; }
                dst[i] = sym;
            }
            remaining -= n; return true;
        }
        if (codec == 4 && huff.max_len <= 12) {
            for (size_t i = 0; i < n; ++i) {
                while (have < 12 && hp < hend) { acc = (acc << 8) | *hp++; have += 8; }
                int win = have; if (win > 12) win = 12;
                uint32_t code = (uint32_t)((acc >> (have - win)) & ((1u << win) - 1));
                uint32_t idx = code << (12 - win);
                uint16_t entry = huff.tbl[idx];
                if (entry == 0xFFFFu) throw std::runtime_error("invalid huffman code");
                int used = entry & 0xF;
                if (used > win) throw std::runtime_error("invalid huffman code");
                have -= used; dst[i] = uint8_t(entry >> 4);
            }
            remaining -= n; return true;
        }
        for (size_t i = 0; i < n; ++i) if (!next_byte(dst[i])) return false;
        return true;
    }

    bool at_end() const {
        if (remaining != 0) return false;
        if (codec == 0) return p == e;
        if (codec >= 1 && codec <= 3) return p == rend;
        if (codec == 4) return hp == hend;
        if (codec == 6) return p == rend;
        if (codec == 7 || codec == 8) return rp_i >= rp_buf.size(); // eager-decoded buffer
        return bits_done == total && exc == exc_end;
    }
};

static uint64_t read_varint_pull(StreamPull& sp) {
    uint64_t x = 0; int shift = 0;
    for (int i = 0; i < 10; ++i) {
        uint8_t b;
        if (!sp.next_byte(b)) throw std::runtime_error("stream varint truncated");
        x |= uint64_t(b & 0x7f) << shift;
        if (!(b & 0x80)) return x;
        shift += 7;
    }
    throw std::runtime_error("stream varint overflow");
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

// ---- ARI-REF backend (mode 16): additive arithmetic reference --------------
// Experiment Z / I8 mode 16. ARI-REF(d, L=4W, Delta, R): copy L bytes (W aligned
// u32 words) from a NON-OVERLAPPING source at distance d (d >= L, d % 4 == 0),
// add one TRANSMITTED constant per-word Delta to every copied word, then apply
// sparse single-byte residuals R (absolute target bytes under a mode-11 style
// mask). Implicit Delta = sigma*(d/4) is EXCLUDED by the frozen pre-registration
// (it failed its ablation: +38.35% vs transmitted on the same token stream).
// Faithful port of prototypes/orbit_ariref/ariref.cpp. The acceptance cost model
// is reproduced VERBATIM from the validated harness - NO re-tuning (threshold-fit
// invalidates the gate per docs/pre-registrations/i8-ari-ref.md §3).
// Wire: 9 separated streams, each via encode_stream:
//   S0 types: 0 literal run, 1 exact match, 4 ARI-REF
//   S1 literal-run length uvarint(len-1)
//   S2 exact-match length uvarint(len-4)              [type 1]
//   S3 distance uvarint(dist-1)                       [types 1 and 4, SHARED]
//   S4 literal bytes
//   S5 correction masks, ceil(len/32) LE u32 words    [type 4]
//   S6 residual bytes in mask order                   [type 4]
//   S7 ARI word count uvarint(W-1)                    [type 4]
//   S8 ARI Delta, zigzag varint                       [type 4]
// Decode of an ARI token: base = out.size()-dist; copy len bytes (non-overlapping,
// dist >= len is enforced), add Delta to each 4-byte word, then overwrite masked
// bytes from the residual stream. popcount(mask) == residual count is strict.

static constexpr uint32_t kAriMinLen   = 16;      // >= 4 words (harness kAriMinLen)
static constexpr uint32_t kAriMaxLen   = 4096;    // span-offset width limit (u32 bitmap)
static constexpr uint32_t kAriMaxDist  = 1u << 22;
static constexpr uint32_t kAriCQ       = 16;      // step-bucket capacity (FIFO)
static constexpr uint32_t kAriExactCQ  = 64;      // exact-bucket capacity (FIFO)
static constexpr uint32_t kStepWin     = 7;       // step key = w[7]-w[0] over 8 words
static constexpr uint32_t kAriHashBits = 18;

static uint64_t ari_vsize(uint64_t v){ uint64_t n=1; while(v>=128){++n; v>>=7;} return n; }
static uint64_t ari_lit_cost(uint64_t len){ return 1+ari_vsize(len)+len; }
static uint64_t ari_exact_cost(uint64_t len,uint64_t dist){ return 1+ari_vsize(len-4)+ari_vsize(dist-1); }
static uint64_t ari_tx_cost(uint64_t len,uint64_t dist,int64_t delta,uint64_t nres){
    uint64_t z = delta>=0 ? uint64_t(delta)*2 : uint64_t(-(delta+1))*2+1;
    return 1+ari_vsize(len/4-1)+ari_vsize(dist-1)+ari_vsize(z)+4*((len+31)/32)+nres;
}
static uint64_t ari_residual_count(const uint8_t* in,size_t pos,uint64_t dist,uint64_t len,int64_t delta){
    uint64_t src=pos-dist,nres=0;
    for(uint64_t w=0;w<len/4;++w){
        uint32_t s,t; std::memcpy(&s,in+src+w*4,4); std::memcpy(&t,in+pos+w*4,4);
        uint32_t x=uint32_t(uint64_t(s)+uint64_t(delta))^t;
        nres+=(x&0xFF?1:0)+(x&0xFF00?1:0)+(x&0xFF0000?1:0)+(x&0xFF000000u?1:0);
    }
    return nres;
}
static inline uint32_t ari_hash4(const uint8_t* p){
    uint32_t x; std::memcpy(&x,p,4);
    x^=x>>16; x*=0x7feb352du; x^=x>>15; x*=0x846ca68bu; x^=x>>16;
    return x&((1u<<kAriHashBits)-1);
}
static inline uint32_t ari_hash_i64(int64_t k){
    return uint32_t((uint64_t(k)*0x9E3779B1ull)>>(64-kAriHashBits));
}

struct AriMatch { uint64_t len=0, dist=0, nres=0; int64_t delta=0; };

static std::vector<SparseToken> parse_ariref(const std::vector<uint8_t>& d, uint32_t max_match) {
    const size_t n=d.size();
    std::vector<SparseToken> toks;
    if(n<8) { if(n){ SparseToken t; t.type=0; t.pos=0; t.len=(uint32_t)n; toks.push_back(std::move(t)); } return toks; }
    const uint8_t* p=d.data();
    struct StepEnt { int64_t key; uint32_t pos; };
    std::vector<std::vector<uint32_t>> extab(size_t(1)<<kAriHashBits);
    std::vector<std::vector<StepEnt>>  steptab(size_t(1)<<kAriHashBits);
    auto index_pos=[&](size_t hp){
        if(hp+4<=n){ auto&b=extab[ari_hash4(p+hp)]; if(b.size()>=kAriExactCQ) b.erase(b.begin()); b.push_back(uint32_t(hp)); }
        if(hp%4==0 && hp+4*(kStepWin+1)<=n){
            uint32_t w0,w7; std::memcpy(&w0,p+hp,4); std::memcpy(&w7,p+hp+4*kStepWin,4);
            int64_t key=int64_t(w7)-int64_t(w0);
            auto&b=steptab[ari_hash_i64(key)]; if(b.size()>=kAriCQ) b.erase(b.begin()); b.push_back({key,uint32_t(hp)});
        }
    };
    auto longest_exact=[&](size_t pos)->AriMatch{
        AriMatch best{};
        if(pos+4>n) return best;
        uint32_t hb; std::memcpy(&hb,p+pos,4);
        for(uint32_t q: extab[ari_hash4(p+pos)]) {
            if(size_t(q)>=pos) continue;
            uint32_t a; std::memcpy(&a,p+q,4);
            if(a!=hb) continue;                      // verify anchor: hash collisions exist
            size_t len=4, lim=std::min<size_t>(max_match,n-pos);
            while(len<lim && p[q+len]==p[pos+len]) ++len;
            if(len>best.len) best={len,uint64_t(pos-q),0,0};
        }
        return best;
    };
    auto longest_ari=[&](size_t pos)->AriMatch{
        AriMatch best{};
        if(pos%4!=0 || pos+4>n) return best;
        size_t we=pos+4*kStepWin;
        if(we+4>n) return best;
        uint32_t w0,w7; std::memcpy(&w0,p+pos,4); std::memcpy(&w7,p+we,4);
        int64_t key=int64_t(w7)-int64_t(w0);
        for(const StepEnt& e: steptab[ari_hash_i64(key)]) {
            if(e.key!=key) continue;                 // exact key verification
            size_t q=e.pos;
            if(q>=pos) continue;
            uint64_t dist=pos-q;
            if(dist<kAriMinLen||dist>kAriMaxDist||dist%4!=0) continue;
            uint32_t s0,t0; std::memcpy(&s0,p+q,4); std::memcpy(&t0,p+pos,4);
            int64_t d0=int64_t(int32_t(t0-s0));      // anchor Delta: u32 wrap, then sign-extend
            uint64_t W=0,mism=0;
            while(pos+(W+1)*4<=n && (W+1)*4<=dist && (W+1)*4<=kAriMaxLen) {
                uint32_t sv,tv; std::memcpy(&sv,p+q+W*4,4); std::memcpy(&tv,p+pos+W*4,4);
                uint32_t x=uint32_t(uint64_t(sv)+uint64_t(d0))^tv;
                if(x){ mism+=(x&0xFF?1:0)+(x&0xFF00?1:0)+(x&0xFF0000?1:0)+(x&0xFF000000u?1:0);
                       if(mism*8>(W+1)*4+32) break; } // residual-density guard (harness verbatim)
                ++W;
            }
            if(W<4) continue;
            uint64_t L=W*4;
            uint64_t nres=ari_residual_count(p,pos,dist,L,d0);
            if(ari_tx_cost(L,dist,d0,nres)>=ari_lit_cost(L)) continue; // must beat literals
            if(L>best.len || (L==best.len && nres<best.nres)) best={L,dist,nres,d0};
        }
        return best;
    };
    size_t lit_start=0,lit_len=0;
    auto flush_lit=[&]{ if(lit_len){ SparseToken t; t.type=0; t.pos=uint32_t(lit_start); t.len=uint32_t(lit_len); toks.push_back(std::move(t)); lit_len=0; } };
    for(size_t pos=0;pos<n;) {
        AriMatch e=longest_exact(pos);
        AriMatch a=longest_ari(pos);
        bool use_ex = e.len>=4 && ari_exact_cost(e.len,e.dist)<=ari_lit_cost(e.len);
        bool use_ari= a.len>=kAriMinLen;
        uint64_t ex_c = use_ex?ari_exact_cost(e.len,e.dist):~0ull;
        uint64_t ar_c = use_ari?ari_tx_cost(a.len,a.dist,a.delta,a.nres):~0ull;
        if(use_ari && (!use_ex || ar_c<ex_c)) {
            flush_lit();
            SparseToken t; t.type=4; t.pos=uint32_t(pos); t.len=uint32_t(a.len);
            t.dist=uint32_t(a.dist); t.delta=a.delta;
            uint64_t src=pos-a.dist;
            for(uint64_t w=0;w<a.len/4;++w){
                uint32_t sv,tv; std::memcpy(&sv,p+src+w*4,4); std::memcpy(&tv,p+pos+w*4,4);
                uint32_t x=uint32_t(uint64_t(sv)+uint64_t(a.delta))^tv;
                for(int by=0;by<4;++by) if(x&(0xFFu<<(8*by))){
                    t.off.push_back(uint32_t(w*4+size_t(by)));
                    t.val.push_back(uint8_t(tv>>(8*by)));
                }
            }
            toks.push_back(std::move(t));
            for(uint64_t k=0;k<a.len;++k) index_pos(pos+k);
            pos+=a.len;
        } else if(use_ex) {
            flush_lit();
            SparseToken t; t.type=1; t.pos=uint32_t(pos); t.len=uint32_t(e.len); t.dist=uint32_t(e.dist);
            toks.push_back(std::move(t));
            for(uint64_t k=0;k<e.len;++k) index_pos(pos+k);
            pos+=e.len;
        } else {
            if(lit_len==0) lit_start=pos;
            ++lit_len; index_pos(pos); ++pos;
        }
    }
    flush_lit();
    return toks;
}

static std::vector<uint8_t> encode_tokens_ariref(const std::vector<uint8_t>& d, const std::vector<SparseToken>& toks) {
    std::vector<uint8_t> types,ll,ml,ds,lits,masks,resid,aw,ad;
    types.reserve(toks.size());
    std::array<uint32_t,(kAriMaxLen+31)/32> words{};
    for(auto&t:toks) {
        types.push_back(t.type);
        if(t.type==0) {
            append_varint_bytes(ll,t.len-1);
            lits.insert(lits.end(),d.begin()+t.pos,d.begin()+t.pos+t.len);
            continue;
        }
        if(t.type==1) { append_varint_bytes(ml,t.len-4); append_varint_bytes(ds,t.dist-1); continue; }
        if(t.type!=4) throw std::runtime_error("ariref: unsupported token type");
        uint32_t W=t.len/4;
        append_varint_bytes(aw,W-1);
        append_varint_bytes(ds,t.dist-1);
        uint64_t z = t.delta>=0 ? uint64_t(t.delta)*2 : uint64_t(-(t.delta+1))*2+1;
        append_varint_bytes(ad,z);
        uint32_t nwords=(t.len+31)/32;
        std::fill(words.begin(),words.begin()+nwords,0u);
        for(size_t k=0;k<t.off.size();++k) {
            if(t.off[k]>=t.len) throw std::runtime_error("ariref: residual offset out of range");
            words[t.off[k]/32]|=(1u<<(t.off[k]%32));
            resid.push_back(t.val[k]);
        }
        for(uint32_t w=0;w<nwords;++w) {
            uint32_t m=words[w];
            masks.push_back(static_cast<uint8_t>(m));
            masks.push_back(static_cast<uint8_t>(m>>8));
            masks.push_back(static_cast<uint8_t>(m>>16));
            masks.push_back(static_cast<uint8_t>(m>>24));
        }
    }
    std::vector<uint8_t> out;
    for(const auto* v:{&types,&ll,&ml,&ds,&lits,&masks,&resid,&aw,&ad}) {
        auto z=encode_stream(*v);
        put_uvar(out,z.size());
        out.insert(out.end(),z.begin(),z.end());
    }
    return out;
}

static std::vector<uint8_t> decode_tokens_ariref(const uint8_t* p, size_t n, size_t out_len) {
    const uint8_t* e=p+n;
    std::array<std::vector<uint8_t>,9> s;
    const size_t max_sub=16*out_len+64;
    for(int i=0;i<9;++i) {
        uint64_t zn=get_uvar(p,e);
        if(zn>uint64_t(e-p)) throw std::runtime_error("truncated substream");
        const uint8_t* q=p; const uint8_t* qe=p+zn;
        s[i]=decode_stream(q,qe,max_sub);
        if(q!=qe) throw std::runtime_error("substream trailing bytes");
        p+=zn;
    }
    if(p!=e) throw std::runtime_error("payload trailing bytes");
    size_t ip_ll=0,ip_ml=0,ip_ds=0,ip_lit=0,ip_mask=0,ip_res=0,ip_aw=0,ip_ad=0;
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
        } else if(type==4) {
            uint64_t W=read_varint_bytes(s[7],ip_aw)+1;
            uint64_t dist=read_varint_bytes(s[3],ip_ds)+1;
            uint64_t z=read_varint_bytes(s[8],ip_ad);
            int64_t delta = (z&1) ? -int64_t(z>>1)-1 : int64_t(z>>1);
            if(W==0||W>kAriMaxLen/4) throw std::runtime_error("ari word count out of range");
            uint64_t len=W*4;
            // Non-overlap + alignment + history bounds, per the harness contract
            // (harness: `if(dist%4!=0||dist<len) throw` / `dist>out.size()`).
            if(dist%4!=0||dist<len) throw std::runtime_error("ari overlap/align violation");
            if(dist>out.size()) throw std::runtime_error("ari dist beyond history");
            if(len>out_len-out.size()) throw std::runtime_error("ari span exceeds output");
            uint64_t nwords=(len+31)/32;
            if(nwords*4>s[5].size()-ip_mask) throw std::runtime_error("truncated ari mask stream");
            std::array<uint32_t,(kAriMaxLen+31)/32> words{};
            uint32_t pc=0;
            for(uint64_t w=0;w<nwords;++w) {
                uint32_t m=uint32_t(s[5][ip_mask])|(uint32_t(s[5][ip_mask+1])<<8)
                          |(uint32_t(s[5][ip_mask+2])<<16)|(uint32_t(s[5][ip_mask+3])<<24);
                ip_mask+=4;
                uint32_t first=uint32_t(w*32);
                // Reject mask bits beyond the span (harness prototype lacked this;
                // without it a corrupt wire writes out of bounds).
                if(first+32>len) { uint32_t over=first+32-len; if((m>>(32-over))!=0) throw std::runtime_error("ari mask bits beyond span"); }
                words[w]=m;
                pc+=std::popcount(m);
            }
            if(pc>s[6].size()-ip_res) throw std::runtime_error("truncated ari residual stream");
            size_t start=out.size();
            for(uint64_t k=0;k<len;++k) out.push_back(out[start+k-dist]); // non-overlapping copy
            for(uint64_t w=0;w<W;++w) {
                uint32_t v; std::memcpy(&v,out.data()+start+w*4,4);
                uint32_t r=uint32_t(uint64_t(v)+uint64_t(delta));
                std::memcpy(out.data()+start+w*4,&r,4);
            }
            for(uint64_t w=0;w<nwords;++w) {
                uint32_t m=words[w];
                while(m) {
                    uint32_t b=std::countr_zero(m);
                    out[start+uint32_t(w*32)+b]=s[6][ip_res++];
                    m&=m-1;
                }
            }
        } else throw std::runtime_error("unknown ariref token type");
    }
    if(out.size()!=out_len||ip_ll!=s[1].size()||ip_ml!=s[2].size()||ip_ds!=s[3].size()
       ||ip_lit!=s[4].size()||ip_mask!=s[5].size()||ip_res!=s[6].size()
       ||ip_aw!=s[7].size()||ip_ad!=s[8].size())
        throw std::runtime_error("substream consumption mismatch");
    return out;
}

// ---- TCOPY backend (mode 14): implicit Delta=-d transformed copy -----------
// Token type 3 = transformed copy: copy a prior phrase (NON-overlapping, dist>=len)
// and, at 4-aligned windows where the 32-bit target equals source - dist (the
// executable-relative relocation algebra, implicit Delta=-d, zero bits), rewrite
// the field; other corrections are residuals as in mode 11. Transform fields and
// residuals live in SEPARATE streams (the isolated-domain ablation requirement).
// 8 streams: types / ll / ml / ds / lits / residual-mask / residuals / transform-mask.
static std::vector<uint8_t> encode_tokens_tcopy(const std::vector<uint8_t>& d, const std::vector<SparseToken>& toks) {
    std::vector<uint8_t> types, ll, ml, ds, lits, masks, resid, tmask;
    types.reserve(toks.size());
    std::array<uint32_t,(kSparseScanMax+31)/32> words{};
    std::array<uint32_t,(kSparseScanMax+31)/128> twords{}; // one bit per 4-byte window
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
            if(t.type==3) {
                std::fill(twords.begin(),twords.end(),0u);
                for(size_t k=0;k<t.tfo.size();++k) twords[t.tfo[k]/32]|=(1u<<(t.tfo[k]%32));
                uint32_t nwin=t.len/4;
                uint32_t twn=(nwin+31)/32;
                for(uint32_t w=0;w<twn;++w) {
                    uint32_t m=twords[w];
                    tmask.push_back(static_cast<uint8_t>(m));
                    tmask.push_back(static_cast<uint8_t>(m>>8));
                    tmask.push_back(static_cast<uint8_t>(m>>16));
                    tmask.push_back(static_cast<uint8_t>(m>>24));
                }
            }
        }
    }
    std::vector<uint8_t> out;
    // TEMP t-cost instrumentation: per-stream (compressed,raw)
    const char* names[8] = {"types","ll","ml","ds","lits","masks","resid","tmask"};
    const std::vector<uint8_t>* streams[8] = {&types,&ll,&ml,&ds,&lits,&masks,&resid,&tmask};
    std::array<uint64_t,8> sz_raw{}, sz_z{};
    for(int si=0;si<8;++si){ auto z=encode_stream(*streams[si]); sz_raw[si]=streams[si]->size(); sz_z[si]=z.size();
        put_uvar(out,z.size()); out.insert(out.end(),z.begin(),z.end()); }
    { uint32_t rw=0, tw=0; for (auto&t:toks) if (t.type==3) { ++g_diag_t3; rw+=(t.len+31)/32; tw+=((t.len/4)+31)/32; }
      g_diag_reswords += rw; g_diag_tmw += tw; }
    for(int si=0;si<8;++si){ g_diag_sz_raw[si]+=sz_raw[si]; g_diag_sz_z[si]+=sz_z[si]; }
    return out;
}

static std::vector<uint8_t> decode_tokens_tcopy(const uint8_t* p, size_t n, size_t out_len) {
    const uint8_t* e=p+n;
    std::array<std::vector<uint8_t>,8> s;
    const size_t max_sub=16*out_len+64;
    for(int i=0;i<8;++i) {
        uint64_t zn=get_uvar(p,e);
        if(zn>uint64_t(e-p)) throw std::runtime_error("truncated substream");
        const uint8_t* q=p; const uint8_t* qe=p+zn;
        s[i]=decode_stream(q,qe,max_sub);
        if(q!=qe) throw std::runtime_error("substream trailing bytes");
        p+=zn;
    }
    if(p!=e) throw std::runtime_error("payload trailing bytes");
    size_t ip_ll=0,ip_ml=0,ip_ds=0,ip_lit=0,ip_mask=0,ip_res=0,ip_tmask=0;
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
            if(dist==0||dist>out.size()||len>out_len-out.size()) throw std::runtime_error("bad tcopy match");
            for(uint64_t k=0;k<len;++k) out.push_back(out[out.size()-dist]);
        } else if(type==2) {
            uint64_t len=read_varint_bytes(s[2],ip_ml)+4;
            uint64_t dist=read_varint_bytes(s[3],ip_ds)+1;
            if(dist==0||dist>out.size()||len>out_len-out.size()) throw std::runtime_error("bad tcopy sparse");
            if(len>kSparseMaxLen) throw std::runtime_error("tcopy sparse too long");
            size_t start=out.size();
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
            for(uint64_t k=0;k<len;++k) out.push_back(out[out.size()-dist]);
            for(uint64_t w=0;w<nwords;++w) {
                uint32_t m=words[w];
                while(m) {
                    uint32_t b=std::countr_zero(m);
                    out[start+uint32_t(w*32)+b]=s[6][ip_res++];
                    m&=m-1;
                }
            }
        } else if(type==3) {
            uint64_t len=read_varint_bytes(s[2],ip_ml)+4;
            uint64_t dist=read_varint_bytes(s[3],ip_ds)+1;
            if(dist==0||dist>out.size()||len>out_len-out.size()) throw std::runtime_error("bad tcopy transform");
            if(len>kSparseMaxLen) throw std::runtime_error("tcopy transform too long");
            size_t start=out.size();
            for(uint64_t k=0;k<len;++k) out.push_back(out[out.size()-dist]); // copy (overlap allowed; transform fields are non-overlap by construction)
            // transform fields: rewrite 32-bit fields with implicit Delta=-d
            uint64_t nwin=len/4;
            uint64_t twn=(nwin+31)/32;
            if(twn*4>s[7].size()-ip_tmask) throw std::runtime_error("truncated transform mask");
            for(uint64_t w=0;w<twn;++w) {
                uint32_t m=uint32_t(s[7][ip_tmask])|(uint32_t(s[7][ip_tmask+1])<<8)
                          |(uint32_t(s[7][ip_tmask+2])<<16)|(uint32_t(s[7][ip_tmask+3])<<24);
                ip_tmask+=4;
                uint32_t first=uint32_t(w*32);
                if(first+32>nwin) { uint32_t over=first+32-nwin; if((m>>(32-over))!=0) throw std::runtime_error("transform mask bits beyond windows"); }
                while(m) {
                    uint32_t b=std::countr_zero(m);
                    size_t o=start+(w*32+b)*4;
                    if (o + 4 > start + dist) throw std::runtime_error("transform field beyond non-overlap region");
                    uint32_t v=uint32_t(out[o])|(uint32_t(out[o+1])<<8)|(uint32_t(out[o+2])<<16)|(uint32_t(out[o+3])<<24);
                    v-=static_cast<uint32_t>(dist); // implicit Delta = -dist
                    out[o]=static_cast<uint8_t>(v);
                    out[o+1]=static_cast<uint8_t>(v>>8);
                    out[o+2]=static_cast<uint8_t>(v>>16);
                    out[o+3]=static_cast<uint8_t>(v>>24);
                    m&=m-1;
                }
            }
            // residual corrections (same as type 2)
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
            for(uint64_t w=0;w<nwords;++w) {
                uint32_t m=words[w];
                while(m) {
                    uint32_t b=std::countr_zero(m);
                    out[start+uint32_t(w*32)+b]=s[6][ip_res++];
                    m&=m-1;
                }
            }
        } else throw std::runtime_error("unknown tcopy token type");
    }
    if(out.size()!=out_len||ip_ll!=s[1].size()||ip_ml!=s[2].size()||ip_ds!=s[3].size()
       ||ip_lit!=s[4].size()||ip_mask!=s[5].size()||ip_res!=s[6].size()||ip_tmask!=s[7].size())
        throw std::runtime_error("substream consumption mismatch");
    return out;
}

// ---- HOTOP backend (mode 15): compiled hot-op instruction book (v2) --------
// Linux modes 26-28 pattern, corrected per the Linux results (a fully concrete
// book is REJECTED: +40% bytes — absolute-distance specialization destroys
// shape-conditioned distance coding). v2: hot concrete commands COEXIST with
// the per-shape displacement state. The book compiles the most frequent
// (kind, len, shape) tuples that REUSE the shape's last displacement; the
// opcode-index stream replaces the per-field pulls for the hot path, and the
// shape-state index is compiled INTO each entry (constant-time opcode ->
// semantics -> state read -> copy). Rare tokens escape to macro-ops, which use
// the full mode-12 shape coding (absolute / reuse / delta + sparse patches) and
// update the SAME shape state. Book kinds: 0 = literal run (len; no state),
// 1 = exact match reuse (dist = last[shape]).
// Payload: num_states byte, uvarint K, K x (kind byte, len uvar, shape byte),
// then 9 streams: opcodes / macro types / macro ll / macro ml / macro dflags /
// macro dvar / literals / macro masks / macro residuals.
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

struct HotOp { uint8_t kind; uint32_t len; uint8_t shape; };

static constexpr uint32_t kHotMaxOps = 254; // escape symbol = book.size() <= 254 fits a byte

static std::vector<uint8_t> encode_tokens_hotop(const std::vector<uint8_t>& d, const std::vector<SparseToken>& toks, uint32_t num_states) {
    // pass 1: simulate the per-shape state; classify hot (reuse / literal) vs macro
    std::array<uint32_t, 2*kShapeClasses> last{};
    std::vector<uint8_t> is_hot(toks.size(), 0);
    std::vector<uint8_t> tok_shape(toks.size(), 0);
    std::map<std::tuple<uint8_t,uint32_t,uint8_t>, uint32_t> cnt;
    for (size_t ti = 0; ti < toks.size(); ++ti) {
        auto& t = toks[ti];
        if (t.type == 0) { ++cnt[{0, t.len, 0}]; is_hot[ti] = 1; tok_shape[ti] = 0; continue; }
        uint32_t shape = shape_index(t.type, t.len, num_states);
        tok_shape[ti] = uint8_t(shape);
        uint32_t& ld = last[shape];
        if (t.type == 1 && ld != 0 && t.dist == ld) { ++cnt[{1, t.len, uint8_t(shape)}]; is_hot[ti] = 1; }
        else { is_hot[ti] = 0; ld = t.dist; }
    }
    std::vector<std::pair<uint32_t, std::tuple<uint8_t,uint32_t,uint8_t>>> v;
    v.reserve(cnt.size());
    for (auto& [k, c] : cnt) v.push_back({c, k});
    std::sort(v.rbegin(), v.rend());
    if (v.size() > kHotMaxOps) v.resize(kHotMaxOps);
    std::map<std::tuple<uint8_t,uint32_t,uint8_t>, uint32_t> book_idx;
    std::vector<HotOp> book;
    book.reserve(v.size());
    for (auto& [c, k] : v) { book_idx[k] = static_cast<uint32_t>(book.size()); book.push_back({std::get<0>(k), std::get<1>(k), std::get<2>(k)}); }
    // pass 2: encode
    std::vector<uint8_t> opcodes, mtypes, mll, mml, mdflags, mdvar, lits, mmasks, mresid;
    opcodes.reserve(toks.size());
    std::array<uint32_t, 2*kShapeClasses> last2{};
    std::array<uint32_t,(kSparseScanMax+31)/32> words{};
    for (size_t ti = 0; ti < toks.size(); ++ti) {
        auto& t = toks[ti];
        auto key = t.type == 0 ? std::make_tuple(uint8_t(0), t.len, uint8_t(0)) : std::make_tuple(uint8_t(1), t.len, tok_shape[ti]);
        auto it = book_idx.find(key);
        if (is_hot[ti] && it != book_idx.end()) {
            opcodes.push_back(static_cast<uint8_t>(it->second));
            if (t.type == 0) lits.insert(lits.end(), d.begin()+t.pos, d.begin()+t.pos+t.len);
        } else {
            opcodes.push_back(static_cast<uint8_t>(book.size())); // escape -> macro
            mtypes.push_back(t.type);
            if (t.type == 0) { append_varint_bytes(mll, t.len-1); lits.insert(lits.end(), d.begin()+t.pos, d.begin()+t.pos+t.len); }
            else {
                append_varint_bytes(mml, t.len-4);
                uint32_t shape = shape_index(t.type, t.len, num_states);
                uint32_t& ld = last2[shape];
                if (ld == 0) { mdflags.push_back(0); append_varint_bytes(mdvar, t.dist-1); ld = t.dist; }
                else if (t.dist == ld) mdflags.push_back(1);
                else { mdflags.push_back(2); int64_t dlt = int64_t(t.dist)-int64_t(ld); uint64_t zz = dlt>=0?uint64_t(dlt)*2:uint64_t(-dlt)*2-1; append_varint_bytes(mdvar, zz); ld = t.dist; }
                if (t.type == 2) {
                    std::fill(words.begin(), words.end(), 0u);
                    for (size_t k = 0; k < t.off.size(); ++k) { words[t.off[k]/32] |= (1u << (t.off[k]%32)); mresid.push_back(t.val[k]); }
                    uint32_t nwords = (t.len + 31) / 32;
                    for (uint32_t w = 0; w < nwords; ++w) {
                        uint32_t m = words[w];
                        mmasks.push_back(static_cast<uint8_t>(m));
                        mmasks.push_back(static_cast<uint8_t>(m>>8));
                        mmasks.push_back(static_cast<uint8_t>(m>>16));
                        mmasks.push_back(static_cast<uint8_t>(m>>24));
                    }
                }
            }
        }
    }
    std::vector<uint8_t> out;
    out.push_back(static_cast<uint8_t>(num_states));
    put_uvar(out, book.size());
    for (auto& op : book) { out.push_back(op.kind); put_uvar(out, op.len); out.push_back(op.shape); }
    for (const auto* v2 : {&opcodes, &mtypes, &mll, &mml, &mdflags, &mdvar, &lits, &mmasks, &mresid}) {
        // S6-1: whole-codec budget (size-proportional C_decode + raw candidate) when
        // --hotop-budget=on; legacy per-stream J-selection otherwise (byte-identical).
        auto z = g_hotop_budget ? encode_stream_budget(*v2) : encode_stream(*v2);
        if (g_rlz_reap) { // Experiment Y: RePair/RLZ as additional per-stream candidates, smallest wins
            std::vector<uint8_t> rp, rz;
            repair_stream_bytes(*v2, rp);
            rlz_stream_bytes(*v2, rz);
            if (!rp.empty() && rp.size() < z.size()) z = std::move(rp);
            if (!rz.empty() && rz.size() < z.size()) z = std::move(rz);
        }
        put_uvar(out, z.size());
        out.insert(out.end(), z.begin(), z.end());
    }
    return out;
}

static std::vector<uint8_t> decode_tokens_hotop(const uint8_t* p, size_t n, size_t out_len) {
    const uint8_t* e = p + n;
    if (p >= e) throw std::runtime_error("truncated hotop header");
    uint32_t num_states = *p++;
    if (num_states != 1 && num_states != 2 * kShapeClasses) throw std::runtime_error("bad hotop state count");
    uint64_t K = get_uvar(p, e);
    if (K > kHotMaxOps) throw std::runtime_error("bad hotop book size");
    std::vector<HotOp> book(K);
    for (uint64_t i = 0; i < K; ++i) {
        if (p >= e) throw std::runtime_error("truncated hotop book");
        uint8_t kind = *p++;
        if (kind > 1) throw std::runtime_error("bad hotop kind");
        uint64_t len = get_uvar(p, e);
        if (p >= e) throw std::runtime_error("truncated hotop book");
        uint8_t shape = *p++;
        if (len == 0 || len > kSparseMaxLen) throw std::runtime_error("bad hotop len");
        if (kind == 1 && shape >= 2 * kShapeClasses) throw std::runtime_error("bad hotop shape");
        book[i] = {kind, static_cast<uint32_t>(len), shape};
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
    size_t ip_mt = 0, ip_mll = 0, ip_mml = 0, ip_mdf = 0, ip_mdv = 0, ip_lit = 0, ip_mask = 0, ip_res = 0;
    std::array<uint32_t, 2*kShapeClasses> last{};
    std::vector<uint8_t> out; out.reserve(out_len);
    for (uint8_t op : s[0]) {
        if (out.size() >= out_len) throw std::runtime_error("too many tokens");
        if (op < K) {
            const HotOp& b = book[op];
            if (b.kind == 0) {
                if (b.len > out_len - out.size() || b.len > s[6].size() - ip_lit) throw std::runtime_error("bad hotop literal run");
                out.insert(out.end(), s[6].begin() + ip_lit, s[6].begin() + ip_lit + b.len);
                ip_lit += b.len;
            } else {
                uint32_t dist = last[b.shape];
                if (dist == 0 || dist > out.size() || b.len > out_len - out.size()) throw std::runtime_error("bad hotop match");
                if (dist >= b.len) { size_t os = out.size(); out.insert(out.end(), out.begin() + os - dist, out.begin() + os - dist + b.len); }
                else for (uint64_t k = 0; k < b.len; ++k) out.push_back(out[out.size() - dist]);
            }
        } else if (op == K) {
            if (ip_mt >= s[1].size()) throw std::runtime_error("truncated macro types");
            uint8_t type = s[1][ip_mt++];
            if (type == 0) {
                uint64_t len = read_varint_bytes(s[2], ip_mll) + 1;
                if (len > out_len - out.size() || len > s[6].size() - ip_lit) throw std::runtime_error("bad macro literal");
                out.insert(out.end(), s[6].begin() + ip_lit, s[6].begin() + ip_lit + len);
                ip_lit += len;
            } else if (type == 1 || type == 2) {
                uint64_t len = read_varint_bytes(s[3], ip_mml) + 4;
                if (len > kSparseMaxLen || len > out_len - out.size()) throw std::runtime_error("bad macro match");
                if (ip_mdf >= s[4].size()) throw std::runtime_error("truncated macro flags");
                uint8_t flag = s[4][ip_mdf++];
                uint32_t shape = shape_index(type, static_cast<uint32_t>(len), num_states);
                uint32_t& ld = last[shape];
                uint32_t dist;
                if (flag == 0) { uint64_t dv = read_varint_bytes(s[5], ip_mdv); if (dv >= 0xFFFFFFFFull) throw std::runtime_error("bad macro abs dist"); dist = static_cast<uint32_t>(dv) + 1; }
                else if (flag == 1) { if (ld == 0) throw std::runtime_error("macro reuse before absolute"); dist = ld; }
                else if (flag == 2) { if (ld == 0) throw std::runtime_error("macro delta before absolute"); uint64_t zz = read_varint_bytes(s[5], ip_mdv); int64_t dlt = (zz & 1) ? -int64_t((zz + 1) >> 1) : int64_t(zz >> 1); int64_t dd = int64_t(ld) + dlt; if (dd <= 0 || dd > 0xFFFFFFFFll) throw std::runtime_error("bad macro delta"); dist = static_cast<uint32_t>(dd); }
                else throw std::runtime_error("bad macro flag");
                if (dist == 0 || dist > out.size()) throw std::runtime_error("invalid macro dist");
                ld = dist;
                size_t start = out.size();
                for (uint64_t k = 0; k < len; ++k) out.push_back(out[out.size() - dist]);
                if (type == 2) {
                    uint64_t nwords = (len + 31) / 32;
                    if (nwords * 4 > s[7].size() - ip_mask) throw std::runtime_error("truncated macro mask");
                    std::array<uint32_t,(kSparseMaxLen+31)/32> words{};
                    uint32_t pc = 0;
                    for (uint64_t w = 0; w < nwords; ++w) {
                        uint32_t m = uint32_t(s[7][ip_mask]) | (uint32_t(s[7][ip_mask+1]) << 8) | (uint32_t(s[7][ip_mask+2]) << 16) | (uint32_t(s[7][ip_mask+3]) << 24);
                        ip_mask += 4;
                        uint32_t first = uint32_t(w * 32);
                        if (first + 32 > len) { uint32_t over = first + 32 - len; if ((m >> (32 - over)) != 0) throw std::runtime_error("mask bits beyond copy length"); }
                        words[w] = m;
                        pc += std::popcount(m);
                    }
                    if (pc > s[8].size() - ip_res) throw std::runtime_error("truncated macro residual");
                    for (uint64_t w = 0; w < nwords; ++w) {
                        uint32_t m = words[w];
                        while (m) {
                            uint32_t b = std::countr_zero(m);
                            out[start + uint32_t(w * 32) + b] = s[8][ip_res++];
                            m &= m - 1;
                        }
                    }
                }
            } else throw std::runtime_error("bad macro type");
        } else throw std::runtime_error("bad hotop opcode");
    }
    if (out.size() != out_len || ip_mt != s[1].size() || ip_mll != s[2].size() || ip_mml != s[3].size()
       || ip_mdf != s[4].size() || ip_mdv != s[5].size() || ip_lit != s[6].size() || ip_mask != s[7].size() || ip_res != s[8].size())
        throw std::runtime_error("substream consumption mismatch");
    return out;
}

// Fused single-path hotop decode: the opcode and literal streams are pulled on
// demand (no stream vectors materialized); hot ops execute with one pull +
// memcpy. The macro streams stay eager (rare tokens). This is the decode leg's
// win: the hot path has no per-field entropy pulls beyond the opcode itself.
static std::vector<uint8_t> decode_tokens_hotop_fused(const uint8_t* p, size_t n, size_t out_len) {
    const uint8_t* e = p + n;
    if (p >= e) throw std::runtime_error("truncated hotop header");
    uint32_t num_states = *p++;
    if (num_states != 1 && num_states != 2 * kShapeClasses) throw std::runtime_error("bad hotop state count");
    uint64_t K = get_uvar(p, e);
    if (K > kHotMaxOps) throw std::runtime_error("bad hotop book size");
    std::vector<HotOp> book(K);
    for (uint64_t i = 0; i < K; ++i) {
        if (p >= e) throw std::runtime_error("truncated hotop book");
        uint8_t kind = *p++;
        if (kind > 1) throw std::runtime_error("bad hotop kind");
        uint64_t len = get_uvar(p, e);
        if (p >= e) throw std::runtime_error("truncated hotop book");
        uint8_t shape = *p++;
        if (len == 0 || len > kSparseMaxLen) throw std::runtime_error("bad hotop len");
        if (kind == 1 && shape >= 2 * kShapeClasses) throw std::runtime_error("bad hotop shape");
        book[i] = {kind, static_cast<uint32_t>(len), shape};
    }
    const size_t max_sub = 16 * out_len + 64;
    StreamPull pop, plit;
    std::array<std::vector<uint8_t>, 4> m; // macro types / ll / ml / dflags+dvar fused? -> keep 4: types,ll,ml,flagsanddvar combined below
    // macro streams: types / ll / ml / dflags / dvar / masks / resid = 7
    std::array<StreamPull, 7> mp;
    std::array<std::vector<uint8_t>, 7> mv; // eager fallback storage (rare; decode eagerly)
    // parse the 9 substreams: opcodes, macro types, macro ll, macro ml, macro dflags, macro dvar, literals, macro masks, macro resid
    auto parse_stream = [&](StreamPull& sp, std::vector<uint8_t>& storage, bool eager) {
        uint64_t zn = get_uvar(p, e);
        if (zn > uint64_t(e - p)) throw std::runtime_error("truncated substream");
        const uint8_t* q = p; const uint8_t* qe = p + zn;
        if (eager) { storage = decode_stream(q, qe, max_sub); if (q != qe) throw std::runtime_error("substream trailing bytes"); }
        else { sp.parse(q, qe, max_sub); if (q != qe) throw std::runtime_error("substream trailing bytes"); }
        p += zn;
    };
    parse_stream(pop, mv[0], false);    // 0 opcodes (pull)
    parse_stream(mp[0], mv[0], true);   // 1 macro types (eager)
    parse_stream(mp[1], mv[1], true);   // 2 macro ll
    parse_stream(mp[2], mv[2], true);   // 3 macro ml
    parse_stream(mp[3], mv[3], true);   // 4 macro dflags
    parse_stream(mp[4], mv[4], true);   // 5 macro dvar
    parse_stream(plit, mv[5], false);   // 6 literals (pull)
    parse_stream(mp[5], mv[5], true);   // 7 macro masks
    parse_stream(mp[6], mv[6], true);   // 8 macro resid
    if (p != e) throw std::runtime_error("payload trailing bytes");
    size_t ip_mt = 0, ip_mll = 0, ip_mml = 0, ip_mdf = 0, ip_mdv = 0, ip_mask = 0, ip_res = 0;
    std::array<uint32_t, 2*kShapeClasses> last{};
    std::vector<uint8_t> out(out_len);
    size_t pos = 0;
    uint8_t op;
    while (pop.next_byte(op)) {
        if (pos >= out_len) throw std::runtime_error("too many tokens");
        if (op < K) {
            const HotOp& b = book[op];
            if (b.kind == 0) {
                if (b.len > out_len - pos) throw std::runtime_error("bad hotop literal run");
                if (!plit.pull_bytes(out.data() + pos, b.len)) throw std::runtime_error("truncated hotop literals");
                pos += b.len;
            } else {
                uint32_t dist = last[b.shape];
                if (dist == 0 || dist > pos || b.len > out_len - pos) throw std::runtime_error("bad hotop match");
                uint8_t* o = out.data();
                if (dist >= b.len) { std::memcpy(o + pos, o + pos - dist, b.len); }
                else for (uint64_t k = 0; k < b.len; ++k) o[pos + k] = o[pos + k - dist];
                pos += b.len;
            }
        } else if (op == K) {
            if (ip_mt >= mv[0].size()) throw std::runtime_error("truncated macro types");
            uint8_t type = mv[0][ip_mt++];
            if (type == 0) {
                uint64_t len = read_varint_bytes(mv[1], ip_mll) + 1;
                if (len > out_len - pos) throw std::runtime_error("bad macro literal");
                if (!plit.pull_bytes(out.data() + pos, static_cast<size_t>(len))) throw std::runtime_error("truncated macro literals");
                pos += static_cast<size_t>(len);
            } else if (type == 1 || type == 2) {
                uint64_t len = read_varint_bytes(mv[2], ip_mml) + 4;
                if (len > kSparseMaxLen || len > out_len - pos) throw std::runtime_error("bad macro match");
                if (ip_mdf >= mv[3].size()) throw std::runtime_error("truncated macro flags");
                uint8_t flag = mv[3][ip_mdf++];
                uint32_t shape = shape_index(type, static_cast<uint32_t>(len), num_states);
                uint32_t& ld = last[shape];
                uint32_t dist;
                if (flag == 0) { uint64_t dv = read_varint_bytes(mv[4], ip_mdv); if (dv >= 0xFFFFFFFFull) throw std::runtime_error("bad macro abs dist"); dist = static_cast<uint32_t>(dv) + 1; }
                else if (flag == 1) { if (ld == 0) throw std::runtime_error("macro reuse before absolute"); dist = ld; }
                else if (flag == 2) { if (ld == 0) throw std::runtime_error("macro delta before absolute"); uint64_t zz = read_varint_bytes(mv[4], ip_mdv); int64_t dlt = (zz & 1) ? -int64_t((zz + 1) >> 1) : int64_t(zz >> 1); int64_t dd = int64_t(ld) + dlt; if (dd <= 0 || dd > 0xFFFFFFFFll) throw std::runtime_error("bad macro delta"); dist = static_cast<uint32_t>(dd); }
                else throw std::runtime_error("bad macro flag");
                if (dist == 0 || dist > pos) throw std::runtime_error("invalid macro dist");
                ld = dist;
                uint8_t* o = out.data();
                for (uint64_t k = 0; k < len; ++k) o[pos + k] = o[pos + k - dist];
                pos += static_cast<size_t>(len);
                if (type == 2) {
                    uint64_t nwords = (len + 31) / 32;
                    if (nwords * 4 > mv[5].size() - ip_mask) throw std::runtime_error("truncated macro mask");
                    std::array<uint32_t,(kSparseMaxLen+31)/32> words{};
                    uint32_t pc = 0;
                    for (uint64_t w = 0; w < nwords; ++w) {
                        uint32_t m = uint32_t(mv[5][ip_mask]) | (uint32_t(mv[5][ip_mask+1]) << 8) | (uint32_t(mv[5][ip_mask+2]) << 16) | (uint32_t(mv[5][ip_mask+3]) << 24);
                        ip_mask += 4;
                        uint32_t first = uint32_t(w * 32);
                        if (first + 32 > len) { uint32_t over = first + 32 - len; if ((m >> (32 - over)) != 0) throw std::runtime_error("mask bits beyond copy length"); }
                        words[w] = m;
                        pc += std::popcount(m);
                    }
                    if (pc > mv[6].size() - ip_res) throw std::runtime_error("truncated macro residual");
                    size_t start = pos - static_cast<size_t>(len);
                    for (uint64_t w = 0; w < nwords; ++w) {
                        uint32_t m = words[w];
                        while (m) {
                            uint32_t b = std::countr_zero(m);
                            out[start + uint32_t(w * 32) + b] = mv[6][ip_res++];
                            m &= m - 1;
                        }
                    }
                }
            } else throw std::runtime_error("bad macro type");
        } else throw std::runtime_error("bad hotop opcode");
    }
    if (!pop.at_end() || !plit.at_end()) throw std::runtime_error("substream consumption mismatch");
    if (pos != out_len || ip_mt != mv[0].size() || ip_mll != mv[1].size() || ip_mml != mv[2].size()
       || ip_mdf != mv[3].size() || ip_mdv != mv[4].size() || ip_mask != mv[5].size() || ip_res != mv[6].size())
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

// Fused single-path decode for mode 12 (t3-fuse): identical wire to
// decode_tokens_shape, but the entropy decode and the reconstruction run in ONE
// pass — each stream field is pulled on demand (no full substream vectors, no
// second pass) into a pos-based output buffer with bulk memcpy for literals and
// non-overlapping matches (Linux stream economics: one LZ-class instruction path).
static std::vector<uint8_t> decode_tokens_shape_fused(const uint8_t* p, size_t n, size_t out_len) {
    const uint8_t* e = p + n;
    if (p >= e) throw std::runtime_error("truncated shape header");
    uint32_t num_states = *p++;
    if (num_states != 1 && num_states != 2 * kShapeClasses) throw std::runtime_error("bad shape state count");
    const size_t max_sub = 16 * out_len + 64;
    std::array<StreamPull, 8> s;
    for (int i = 0; i < 8; ++i) {
        uint64_t zn = get_uvar(p, e);
        if (zn > uint64_t(e - p)) throw std::runtime_error("truncated substream");
        const uint8_t* q = p; const uint8_t* qe = p + zn;
        s[i].parse(q, qe, max_sub);
        if (q != qe) throw std::runtime_error("substream trailing bytes");
        p += zn;
    }
    if (p != e) throw std::runtime_error("payload trailing bytes");
    std::array<uint32_t, 2 * kShapeClasses> last{};
    std::vector<uint8_t> out(out_len);
    size_t pos = 0;
    uint8_t type;
    while (s[0].next_byte(type)) {
        if (pos >= out_len) throw std::runtime_error("too many tokens");
        if (type == 0) {
            uint64_t len = read_varint_pull(s[1]) + 1;
            if (len > out_len - pos) throw std::runtime_error("bad literal run");
            if (!s[5].pull_bytes(out.data() + pos, static_cast<size_t>(len))) throw std::runtime_error("truncated literals");
            pos += static_cast<size_t>(len);
        } else if (type == 1 || type == 2) {
            uint64_t len = read_varint_pull(s[2]) + 4;
            if (len > kSparseMaxLen || len > out_len - pos) throw std::runtime_error("bad shape match");
            uint8_t flag;
            if (!s[3].next_byte(flag)) throw std::runtime_error("truncated dist flags");
            uint32_t shape = shape_index(type, static_cast<uint32_t>(len), num_states);
            uint32_t& lastd = last[shape];
            uint32_t dist;
            if (flag == 0) {
                uint64_t dv = read_varint_pull(s[4]);
                if (dv >= 0xFFFFFFFFull) throw std::runtime_error("bad absolute distance");
                dist = static_cast<uint32_t>(dv) + 1;
            } else if (flag == 1) {
                if (lastd == 0) throw std::runtime_error("dist reuse before first absolute");
                dist = lastd;
            } else if (flag == 2) {
                if (lastd == 0) throw std::runtime_error("dist delta before first absolute");
                uint64_t zz = read_varint_pull(s[4]);
                int64_t dlt = (zz & 1) ? -int64_t((zz + 1) >> 1) : int64_t(zz >> 1);
                int64_t dd = int64_t(lastd) + dlt;
                if (dd <= 0 || dd > 0xFFFFFFFFll) throw std::runtime_error("bad distance delta");
                dist = static_cast<uint32_t>(dd);
            } else throw std::runtime_error("bad dist flag");
            if (dist == 0 || dist > pos) throw std::runtime_error("invalid shape distance");
            lastd = dist;
            uint8_t* o = out.data();
            if (dist >= len && len >= 16) {
                std::memcpy(o + pos, o + pos - dist, static_cast<size_t>(len)); // non-overlap bulk copy
            } else {
                for (uint64_t k = 0; k < len; ++k) o[pos + k] = o[pos + k - dist]; // overlap / short copy
            }
            pos += static_cast<size_t>(len);
            if (type == 2) {
                uint64_t nwords = (len + 31) / 32;
                std::array<uint32_t,(kSparseMaxLen+31)/32> words{};
                uint32_t pc = 0;
                // pull the whole mask region in one bulk read
                std::array<uint8_t,(kSparseMaxLen+31)/32*4> mb{};
                if (!s[6].pull_bytes(mb.data(), nwords * 4)) throw std::runtime_error("truncated mask stream");
                for (uint64_t w = 0; w < nwords; ++w) {
                    const uint8_t* mp = mb.data() + w * 4;
                    uint32_t m = uint32_t(mp[0]) | (uint32_t(mp[1]) << 8) | (uint32_t(mp[2]) << 16) | (uint32_t(mp[3]) << 24);
                    uint32_t first = uint32_t(w * 32);
                    if (first + 32 > len) { uint32_t over = first + 32 - len; if ((m >> (32 - over)) != 0) throw std::runtime_error("mask bits beyond copy length"); }
                    words[w] = m;
                    pc += std::popcount(m);
                }
                if (pc > s[7].remaining) throw std::runtime_error("truncated residual stream");
                size_t start = pos - static_cast<size_t>(len); // copy destination start
                // bulk-pull the residual values, then apply at mask-set offsets
                std::vector<uint8_t> res(pc);
                if (!s[7].pull_bytes(res.data(), pc)) throw std::runtime_error("truncated residual");
                size_t ri = 0;
                for (uint64_t w = 0; w < nwords; ++w) {
                    uint32_t m = words[w];
                    while (m) {
                        uint32_t b = std::countr_zero(m);
                        out[start + uint32_t(w * 32) + b] = res[ri++];
                        m &= m - 1;
                    }
                }
            }
        } else throw std::runtime_error("unknown shape token type");
    }
    for (auto& sp : s) if (!sp.at_end()) throw std::runtime_error("substream consumption mismatch");
    if (pos != out_len) throw std::runtime_error("size mismatch");
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
    // k = correction count ranges 0..64 (guard below admits ==64), so the OUTER
    // dimension is 65 — [64][j] was a silent out-of-bounds write before (ASan-found).
    std::array<std::array<uint8_t,64>,65> modal{};
    std::array<std::array<bool,64>,65> has_modal{};
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
    // outer dimension 65: k == 64 is a legal context (mirrors the encoder fix)
    std::array<std::array<uint8_t,64>,65> modal{};
    std::array<std::array<bool,64>,65> has_modal{};
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
    bool channels=false;   // R4 structural channels (measured not-aligned on corpus; SRR follow-up) (persistent displacement bank)
    bool pnra=false;       // Experiment X: PNRA invariant-anchored candidate source for mode 14 (TCOPY); off by default
    bool stream_suite=true; // stream codec suite (huffman/defexc/256-512 rANS); off = fixed rANS-4096+raw
    bool stream_ctx=true;     // context-switched rANS (mode 6) in the suite
    double stream_lambda=0.01; // J-cost decode-weight (pre-registered binding value 0.01)
    bool stream_log=false; // --stream-log: record per-stream codec selection to stdout
    bool hotop_rlzp=false; // Experiment Y: RLZ/RePair stream codecs as hot-op book-stream candidates (--hotop-rlzp=on)
    bool hotop_budget=false; // S6-1: whole-codec stream budget on mode-15 book streams (size-proportional C_decode, lambda=0.01)
    bool ariref=false;     // I8 mode-16: ARI-REF additive arithmetic reference (Experiment Z, transmitted-Delta only)
    bool ratio_context=true; // ratio mode 17: decoder-routed order-1 context partition candidate
    bool ratio_lines=true;   // ratio mode 17: line-record column transpose candidate
    std::string ratio_backend="brotli"; // rev-2 mode-17 backend registry choice (wire id 1 today)
    // E6 BWT backend experiments (ABLATION ONLY; production default behavior unchanged):
    int bwt_post=-1;         // force one postcoder id (0/1/2/3/4) for the BWT backend; -1 = encoder picks smallest
    bool bwt_lzp=false;      // LZP prepass before BWT (default OFF)
    uint32_t bwt_subblock=128u<<20; // cap BWT sub-block size; default 128 MiB = effectively off for all 12 Silesia files (memory lever only, not a ratio lever)
    uint32_t decode_threads=1; // parallel block-decode worker count; 1 = serial (zero behavioral change)
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

// ---- Ratio-first envelope (block mode 17, format revision 2) ---------------
// Revision 2 separates reversible representation choice from byte-compression
// backend choice.  Backend IDs are decode semantics; encoder tuning such as
// Brotli q11/lgwin30 is deliberately NOT encoded as backend identity.
//
// Transform 0 is the exact reference representation; transforms 1/2 are
// reversible structural preconditioners. The encoder always compares against
// transform 0 under the SAME backend and emits a transform only when its
// COMPLETE payload is smaller. This isolates transform value from backend value.
//
// Wire payload:
//   byte transform_id (0 direct, 1 ctx1-256, 2 line-columns)
//   byte backend_id   (1 Brotli, 2 BWT; 0 reserved/invalid)
//   uvarint transformed_size
//   backend payload bytes
//
// ctx1 transformed bytes:
//   256 x uvarint stream_len, then stream[0]..stream[255]
// byte i is routed by the already-decoded previous byte (initial context 0), so
// positions/context ids are not transmitted. Decoder recomputes the route.
//
// line-columns transformed bytes:
//   uvarint record_count, record_count x uvarint record_len, then columns
// Records end at '\n' (included); a final non-newline tail is a record. Column
// lengths are derivable from record lengths and are therefore not stored.

static constexpr uint8_t kRatioBackendBrotli = 1;
static constexpr uint8_t kRatioBackendBwt    = 2;

#ifdef ANVIL_HAVE_BROTLI
static std::vector<uint8_t> brotli_q11_lw30_encode(const std::vector<uint8_t>& in) {
    size_t cap=BrotliEncoderMaxCompressedSize(in.size());
    if(cap==0) throw std::runtime_error("brotli size bound overflow");
    std::vector<uint8_t> out(cap);
    size_t n=cap;
    const uint8_t* src=in.empty()?reinterpret_cast<const uint8_t*>(""):in.data();
    if(!BrotliEncoderCompress(11,30,BROTLI_MODE_GENERIC,in.size(),src,&n,out.data()))
        throw std::runtime_error("brotli q11 large-window encode failed");
    out.resize(n);
    return out;
}

static std::vector<uint8_t> brotli_lw_decode_exact(const uint8_t* p,size_t n,size_t expected) {
    if(expected>(384ull<<20)) throw std::runtime_error("ratio transformed stream too large");
    BrotliDecoderState* s=BrotliDecoderCreateInstance(nullptr,nullptr,nullptr);
    if(!s) throw std::runtime_error("brotli decoder allocation failed");
    struct Guard { BrotliDecoderState* p; ~Guard(){ BrotliDecoderDestroyInstance(p); } } guard{s};
    if(!BrotliDecoderSetParameter(s,BROTLI_DECODER_PARAM_LARGE_WINDOW,1))
        throw std::runtime_error("brotli large-window decoder setup failed");
    std::vector<uint8_t> out(expected+1);
    size_t ai=n, ao=out.size(), total_out=0;
    const uint8_t* ni=p; uint8_t* no=out.data();
    BrotliDecoderResult r=BrotliDecoderDecompressStream(s,&ai,&ni,&ao,&no,&total_out);
    if(r!=BROTLI_DECODER_RESULT_SUCCESS || ai!=0 || total_out!=expected)
        throw std::runtime_error("brotli ratio payload decode mismatch");
    out.resize(expected);
    return out;
}
#endif

// Backend 2: BWT (libsais primitive) + independent ANVIL postcoders.
// The postcoder registry is deliberately inside backend 2 so we can measure
// sorting value separately from the entropy model without allocating more
// top-level backend IDs for every experiment.
//   0 = MTF + zero-run tokens, two smallest-size stream-suite streams
//   1 = MTF + zero-run tokens, adaptive order-0 arithmetic
//   2 = MTF + zero-run tokens, adaptive order-1 token arithmetic
//   3 = raw BWT bytes, one smallest-size stream-suite stream (ablation/control)
//   4 = QLFC-like local-frequency postcoder (rank derived from a symbol-
//       associated LOCAL frequency estimate rather than global MTF recency)
// Each payload begins with the postcoder id byte, then uvar(primary index),
// then postcoder-specific payload. The primary index is externalized so the
// decoder never needs the whole original block to invert the BWT.
// NEW IDs ARE ADDED, NEVER REDEFINED. Decoders for 0..3 are byte-stable.
static constexpr uint8_t kBwtPostStaticMtf = 0;
static constexpr uint8_t kBwtPostArithO0   = 1;
static constexpr uint8_t kBwtPostArithO1   = 2;
static constexpr uint8_t kBwtPostRawStream = 3;
static constexpr uint8_t kBwtPostQlfc      = 4; // QLFC-like local frequency

struct BwtMtfRle {
    std::vector<uint8_t> tokens; // 0 = run of rank-0; 1..255 = literal MTF rank
    std::vector<uint8_t> runs;   // uvarint(run_len-1) bytes, one varint per token 0
};

static BwtMtfRle bwt_mtf_rle(const std::vector<uint8_t>& bwt) {
    std::array<uint8_t,256> sym{}, pos{};
    for(uint32_t i=0;i<256;++i){ sym[i]=static_cast<uint8_t>(i); pos[i]=static_cast<uint8_t>(i); }
    BwtMtfRle r; r.tokens.reserve(bwt.size()/2+16); r.runs.reserve(bwt.size()/16+16);
    uint64_t zrun=0;
    auto flush_zero=[&](){ if(zrun){ r.tokens.push_back(0); append_varint_bytes(r.runs,zrun-1); zrun=0; } };
    for(uint8_t b:bwt) {
        uint32_t rank=pos[b];
        if(rank==0){ ++zrun; continue; }
        flush_zero();
        r.tokens.push_back(static_cast<uint8_t>(rank));
        for(uint32_t j=rank;j>0;--j){ sym[j]=sym[j-1]; pos[sym[j]]=static_cast<uint8_t>(j); }
        sym[0]=b; pos[b]=0;
    }
    flush_zero();
    return r;
}

static void bwt_mtf_emit_rank(std::array<uint8_t,256>& sym,uint8_t rank,std::vector<uint8_t>& out) {
    uint8_t b=sym[rank]; out.push_back(b);
    if(rank) {
        for(uint32_t j=rank;j>0;--j) sym[j]=sym[j-1];
        sym[0]=b;
    }
}

static std::vector<uint8_t> bwt_mtf_expand(const std::vector<uint8_t>& tokens,const std::vector<uint8_t>& runs,size_t expected) {
    std::array<uint8_t,256> sym{}; for(uint32_t i=0;i<256;++i)sym[i]=static_cast<uint8_t>(i);
    std::vector<uint8_t> out; out.reserve(expected); size_t rp=0;
    for(uint8_t t:tokens) {
        if(out.size()>=expected) throw std::runtime_error("BWT MTF token overflow");
        if(t==0) {
            uint64_t rv=read_varint_bytes(runs,rp);
            if(rv>=expected-out.size()) throw std::runtime_error("BWT zero run exceeds output");
            size_t len=static_cast<size_t>(rv)+1;
            out.insert(out.end(),len,sym[0]); // rank 0 leaves the MTF list unchanged
        } else bwt_mtf_emit_rank(sym,t,out);
    }
    if(out.size()!=expected || rp!=runs.size()) throw std::runtime_error("BWT MTF stream consumption mismatch");
    return out;
}

static std::pair<std::vector<uint8_t>,uint64_t> bwt_arith_encode(const BwtMtfRle& r,bool order1) {
    ArithmeticEncoder ac; AdaptiveModel tok0(256), runm(256);
    std::array<std::unique_ptr<AdaptiveModel>,257> tok1;
    auto model1=[&](uint32_t ctx)->AdaptiveModel& { if(!tok1[ctx])tok1[ctx]=std::make_unique<AdaptiveModel>(256); return *tok1[ctx]; };
    size_t rp=0; uint32_t prev=256;
    for(uint8_t t:r.tokens) {
        if(order1) model1(prev).encode(ac,t); else tok0.encode(ac,t);
        prev=t;
        if(t==0) {
            for(;;) {
                if(rp>=r.runs.size()) throw std::runtime_error("BWT run accounting bug");
                uint8_t b=r.runs[rp++]; runm.encode(ac,b); if(!(b&0x80)) break;
            }
        }
    }
    if(rp!=r.runs.size()) throw std::runtime_error("BWT run accounting trailing bytes");
    auto bits=ac.finish(); uint64_t nbits=ac.bit_count(); return {std::move(bits),nbits};
}

static std::vector<uint8_t> bwt_arith_decode(const uint8_t* p,size_t n,size_t expected,bool order1,uint64_t bit_count) {
    if(bit_count==0 || bit_count>uint64_t(n)*8) throw std::runtime_error("bad BWT arithmetic bit count");
    uint64_t need=(bit_count+7)/8; if(need!=n) throw std::runtime_error("BWT arithmetic byte count mismatch");
    if((bit_count&7) && n) {
        uint32_t pad=8-static_cast<uint32_t>(bit_count&7); uint8_t mask=static_cast<uint8_t>((1u<<pad)-1u);
        if(p[n-1]&mask) throw std::runtime_error("nonzero BWT arithmetic padding");
    }
    ArithmeticDecoder ad(p,n); AdaptiveModel tok0(256), runm(256);
    std::array<std::unique_ptr<AdaptiveModel>,257> tok1;
    auto model1=[&](uint32_t ctx)->AdaptiveModel& { if(!tok1[ctx])tok1[ctx]=std::make_unique<AdaptiveModel>(256); return *tok1[ctx]; };
    std::array<uint8_t,256> sym{}; for(uint32_t i=0;i<256;++i)sym[i]=static_cast<uint8_t>(i);
    std::vector<uint8_t> out; out.reserve(expected); uint32_t prev=256;
    while(out.size()<expected) {
        uint8_t t=static_cast<uint8_t>(order1?model1(prev).decode(ad):tok0.decode(ad)); prev=t;
        if(t==0) {
            uint64_t rv=decode_uvar(ad,runm);
            if(rv>=expected-out.size()) throw std::runtime_error("BWT arithmetic zero run exceeds output");
            size_t len=static_cast<size_t>(rv)+1; out.insert(out.end(),len,sym[0]);
        } else bwt_mtf_emit_rank(sym,t,out);
    }
    // Arithmetic termination may leave a bounded suffix of termination bits;
    // bit_count fixes the exact payload byte extent and zero-padding above, so
    // there can be no hidden trailing bytes or attacker-driven allocation.
    return out;
}

// LZP side table: literals not matched + match stream. Reconstructed exactly.
struct BwtLzpPrepass {
    bool used=false;
    std::vector<uint8_t> literals;
    std::vector<uint8_t> matches; // (uvarint(dist-1), uvarint(len-3)) per match, forward order
};

// ---- LZP prepass (E6 ablation control) ------------------------------------
// Replaces predictable long phrases with pointers BEFORE the BWT, so the
// sorter sees a residue that is friendlier to the postcoder. Fully reversible:
// the side table (literal runs + match streams) is transmitted, and the
// decoder reconstructs the exact original block.
//   literals: raw byte runs (stream-suite encoded as one substream).
//   matches:   uvarint-encoded (dist-1, len-3). dist is BACKWARD (towards lower
//              file offset); len-3 is uvarint (min match 3). Decoded strictly
//              forward so the copy source is already materialized.
[[maybe_unused]] static BwtLzpPrepass bwt_lzp_preprocess(const std::vector<uint8_t>& in) {
    // Hash-chain LZP with a 64-KiB window and min match 3, max match 255.
    // Conservative: only safe, well-defined matches. Purely encoder-side state;
    // the wire carries the full literal/match story so the decoder is exact.
    constexpr uint32_t kWin = 64u<<10;
    constexpr uint32_t kMin = 3, kMax = 255;
    const size_t N=in.size();
    BwtLzpPrepass pp;
    if(N<kMin) return pp; // nothing to match
    pp.used=true;
    pp.literals.reserve(N);
    std::array<uint32_t,kHashSize> head{}; std::vector<uint32_t> prev(N,0xFFFFFFFFu);
    for(uint32_t i=0;i<kHashSize;++i) head[i]=0xFFFFFFFFu;
    size_t pos=0;
    while(pos<N) {
        uint32_t best=0xFFFFFFFFu; uint32_t bestlen=0;
        if(pos+4<=N) {
            uint32_t h=hash4(in.data()+pos);
            uint32_t can=head[h];
            uint32_t limit=(pos>kWin)?static_cast<uint32_t>(pos-kWin):0u;
            while(can!=0xFFFFFFFFu && can>=limit) {
                uint32_t l=match_length(in.data()+can,in.data()+pos,std::min<uint32_t>(kMax,static_cast<uint32_t>(N-pos)));
                if(l>bestlen){ bestlen=l; best=can; if(l==kMax) break; }
                can=prev[can];
            }
        }
        if(bestlen>=kMin) {
            uint64_t dist=pos-best;
            append_varint_bytes(pp.matches,dist-1);
            append_varint_bytes(pp.matches,static_cast<uint64_t>(bestlen)-kMin);
            // record chain link for future matches at this position
            if(pos+4<=N){ uint32_t h=hash4(in.data()+pos); prev[pos]=head[h]; head[h]=static_cast<uint32_t>(pos); }
            pos+=bestlen;
        } else {
            pp.literals.push_back(in[pos]);
            if(pos+4<=N){ uint32_t h=hash4(in.data()+pos); prev[pos]=head[h]; head[h]=static_cast<uint32_t>(pos); }
            ++pos;
        }
    }
    return pp;
}

// Reconstruct the exact original block from the LZP side table. The encoder
// walks the input left-to-right, emitting a literal for each unmatched byte and
// a (dist,len) match for each matched span; matches copy strictly backward from
// already-written output, so a single forward pass reproduces the original block
// exactly. We replay the SAME structure from the side table: emit literals until
// the next match boundary, then copy. The match stream's uvarint layout is
// (dist-1, len-3); a match is taken whenever there is a pending match entry and
// we have just consumed the literal that precedes it. Because the encoder emits
// (literal*, match) and matches never overlap the undecoded literal prefix,
// we can deterministically reconstruct by always taking one match right after
// the literal run that the encoder left before it.
[[maybe_unused]] static std::vector<uint8_t> bwt_lzp_reconstruct(const std::vector<uint8_t>& lit,const std::vector<uint8_t>& mat) {
    std::vector<uint8_t> out; out.reserve(lit.size()+mat.size());
    size_t lp=0, mp=0;
    if(!mat.empty()) {
        // encoder started with a match (no literal prefix) — copy first.
        uint64_t dist=read_varint_bytes(mat,mp)+1;
        uint64_t len=read_varint_bytes(mat,mp)+3;
        if(dist>out.size()) throw std::runtime_error("LZP match distance overruns output");
        for(uint64_t t=0;t<len;++t) out.push_back(out[out.size()-dist]);
    }
    for(;;) {
        if(lp<lit.size()) out.push_back(lit[lp++]);
        else if(mp<mat.size()) {
            uint64_t dist=read_varint_bytes(mat,mp)+1;
            uint64_t len=read_varint_bytes(mat,mp)+3;
            if(dist>out.size()) throw std::runtime_error("LZP match distance overruns output");
            if(out.size()+len>out.size()+lit.size()+mat.size()) throw std::runtime_error("LZP match length overruns");
            for(uint64_t t=0;t<len;++t) out.push_back(out[out.size()-dist]);
        } else break;
    }
    if(mp!=mat.size()) throw std::runtime_error("LZP match stream trailing bytes");
    return out;
}

// ---- QLFC-like local-frequency postcoder (ID 4) ---------------------------
// Instead of MTF rank (a global recency list), each BWT symbol is coded with a
// rank derived from a LOCAL frequency estimate: we keep a per-symbol frequency
// counter that is incremented on every emission and rescaled when it would
// overflow. The rank of a symbol is its position in the descending-frequency
// order. This exploits the same BWT-run structure (a just-seen symbol has the
// highest local frequency, so it maps to a short rank) without discarding the
// symbol identity into an anonymous recency list. Exactly reversible: the final
// descending-frequency order of distinct symbols is transmitted, the decoder
// rebuilds the identical local-frequency table, and rank order is identical.
//
// Wire (postcoder id 4):
//   uvar(primary)
//   uvar(alphabet)            // number of distinct symbols seen (1..256)
//   alphabet bytes: symbol ids in DESCENDING LOCAL-FREQUENCY order (end state)
//   uvar(nbits) arithmetic data over alphabet   // adaptive order-0
//   uvar(rle_flag)            // 0 = none; 1 = trailing run stream
//   [if flag: uvar(runstream size) runstream]    // uvarint(len-1) per rank-0 run
static void qlfc_build_rank(const std::array<uint32_t,256>& fc,std::array<uint8_t,256>& rank_of,std::array<uint8_t,256>& sym_at,uint32_t alphabet) {
    struct E{uint32_t f;uint8_t s;}; std::vector<E> v; v.reserve(alphabet);
    for(uint32_t s=0;s<256;++s) if(fc[s]) v.push_back({fc[s],static_cast<uint8_t>(s)});
    std::sort(v.begin(),v.end(),[](const E&a,const E&b){ if(a.f!=b.f) return a.f>b.f; return a.s<b.s; });
    for(uint32_t i=0;i<v.size();++i){ sym_at[i]=v[i].s; rank_of[v[i].s]=static_cast<uint8_t>(i); }
}

static std::vector<uint8_t> bwt_qlfc_encode_real(const std::vector<uint8_t>& bwt) {
    const size_t N=bwt.size();
    // Pass 1: full frequency counts (no per-byte rank rebuild — the wire order
    // is the FINAL frequency order, so ranks are consistent with the decoder's
    // transmitted alphabet table). O(N) + O(256 log 256), not O(N*256).
    std::array<uint32_t,256> fc{};
    for(uint8_t b:bwt) fc[b]++;
    std::array<uint8_t,256> rank_of{}, sym_at{};
    uint32_t alphabet=0; for(uint32_t s=0;s<256;++s) if(fc[s]) ++alphabet;
    qlfc_build_rank(fc,rank_of,sym_at,alphabet);
    uint32_t alphabet_final=alphabet;
    std::vector<uint8_t> ranks; ranks.reserve(N);
    for(uint8_t b:bwt) ranks.push_back(rank_of[b]);
    // arithmetic over [0, alphabet_final)
    ArithmeticEncoder ac; AdaptiveModel am(alphabet_final);
    std::vector<uint8_t> runbytes; uint64_t zrun=0;
    auto flush_zero=[&](){ if(zrun){ append_varint_bytes(runbytes,zrun-1); zrun=0; } };
    for(uint8_t r:ranks) {
        if(r==0){ ++zrun; continue; }
        flush_zero();
        am.encode(ac,r);
    }
    flush_zero();
    bool use_rle = !runbytes.empty();
    auto bits=ac.finish(); uint64_t nbits=ac.bit_count();
    // Build final payload. Layout:
    //   uvar(alphabet_final)
    //   alphabet_final bytes: symbol ids in DESCENDING local-freq order
    //   uvar(nbits) arithmetic data
    //   uvar(use_rle?1:0)
    //   [if use_rle: uvar(runbytes.size()) runstream]
    std::vector<uint8_t> z; z.push_back(kBwtPostQlfc);
    put_uvar(z,alphabet_final);
    for(uint32_t i=0;i<alphabet_final;++i) z.push_back(sym_at[i]);
    put_uvar(z,nbits); z.insert(z.end(),bits.begin(),bits.end());
    put_uvar(z,use_rle?1u:0u);
    if(use_rle){ put_uvar(z,runbytes.size()); z.insert(z.end(),runbytes.begin(),runbytes.end()); }
    return z;
}

static std::vector<uint8_t> bwt_qlfc_decode(const uint8_t* p,size_t n,size_t expected,uint64_t /*primary*/) {
    if(expected==0 || expected>static_cast<size_t>(std::numeric_limits<int32_t>::max())) throw std::runtime_error("bad BWT output size (qlfc)");
    const uint8_t* e=p+n; if(p>=e) throw std::runtime_error("truncated QLFC header");
    uint64_t alphabet=get_uvar(p,e); if(alphabet==0 || alphabet>256) throw std::runtime_error("bad QLFC alphabet");
    std::array<uint8_t,256> sym_at{};
    for(uint64_t i=0;i<alphabet;++i){ if(p>=e) throw std::runtime_error("truncated QLFC symbol table"); sym_at[static_cast<size_t>(i)]=*p++; }
    { std::array<bool,256> dup{}; for(uint64_t i=0;i<alphabet;++i){ uint8_t s=sym_at[i]; if(dup[s]) throw std::runtime_error("QLFC duplicate symbol in table"); dup[s]=true; } }
    uint64_t nbits=get_uvar(p,e);
    if(nbits==0 || nbits>uint64_t(e-p)*8) throw std::runtime_error("bad QLFC arithmetic bit count");
    uint64_t need=(nbits+7)/8; if(need>uint64_t(e-p)) throw std::runtime_error("QLFC arithmetic byte count mismatch");
    if((nbits&7) && n) { uint32_t pad=8-static_cast<uint32_t>(nbits&7); uint8_t mask=static_cast<uint8_t>((1u<<pad)-1u); if(p[need-1]&mask) throw std::runtime_error("nonzero QLFC padding"); }
    // arithmetic over [0, alphabet); symbol table is fixed from the header.
    std::array<uint8_t,256> rank_of{}; for(uint64_t i=0;i<alphabet;++i) rank_of[sym_at[i]]=static_cast<uint8_t>(i);
    ArithmeticDecoder ad(p,static_cast<size_t>(need));
    AdaptiveModel am(static_cast<uint32_t>(alphabet));
    std::vector<uint8_t> ranks; ranks.reserve(expected);
    std::array<uint32_t,256> fc{}; uint64_t scale=0; uint32_t alpha=0;
    auto rescale=[&](){ for(uint32_t s=0;s<256;++s) fc[s]>>=1; scale=0; };
    // rank_of/sym_at are NOT changed by decode (they are transmitted), so no
    // rebuild is needed; this matches the encoder which transmitted the end-state.
    (void)alpha;
    while(ranks.size()<expected) {
        uint8_t r=static_cast<uint8_t>(am.decode(ad));
        uint8_t b=sym_at[r];
        if(fc[b]==0) ++alpha;
        fc[b]++; ++scale; if(scale>=(1u<<28)) rescale();
        ranks.push_back(r);
    }
    p+=static_cast<size_t>(need);
    uint64_t rflag=get_uvar(p,e); if(rflag>1) throw std::runtime_error("bad QLFC run flag");
    bool use_rle=(rflag==1);
    std::vector<uint8_t> runbytes;
    if(use_rle) {
        uint64_t rn=get_uvar(p,e); if(rn>uint64_t(e-p)) throw std::runtime_error("truncated QLFC run stream");
        const uint8_t* q=p; const uint8_t* qe=p+rn; runbytes.assign(q,qe); p=qe;
    }
    if(p!=e) throw std::runtime_error("QLFC trailing bytes");
    // Expand ranks -> symbols, applying RLE on rank-0 (symbol = sym_at[0]).
    std::vector<uint8_t> out; out.reserve(expected);
    size_t rp=0;
    for(uint8_t r:ranks) {
        uint8_t b=sym_at[r];
        if(r==0) {
            if(rp>=runbytes.size()) throw std::runtime_error("QLFC run underflow");
            uint64_t rv=read_varint_bytes(runbytes,rp);
            if(rv>=expected-out.size()) throw std::runtime_error("QLFC run exceeds output");
            size_t len=static_cast<size_t>(rv)+1; out.insert(out.end(),len,b);
        } else out.push_back(b);
    }
    if(rp!=runbytes.size()) throw std::runtime_error("QLFC run stream trailing bytes");
    if(out.size()!=expected) throw std::runtime_error("QLFC output size mismatch");
    return out;
}

#ifdef ANVIL_HAVE_LIBSAIS
// Build ONE postcoder payload (postcoder byte already placed at head) for an id.
static std::vector<uint8_t> bwt_postcoder_payload(uint8_t post,const std::vector<uint8_t>& bwt,const BwtMtfRle& mr) {
    if(post==kBwtPostStaticMtf) {
        auto ts=encode_stream_smallest(mr.tokens), rs=encode_stream_smallest(mr.runs);
        std::vector<uint8_t> z; z.push_back(kBwtPostStaticMtf);
        put_uvar(z,ts.size()); z.insert(z.end(),ts.begin(),ts.end()); put_uvar(z,rs.size()); z.insert(z.end(),rs.begin(),rs.end());
        return z;
    }
    if(post==kBwtPostArithO0 || post==kBwtPostArithO1) {
        auto [bits,nbits]=bwt_arith_encode(mr,post==kBwtPostArithO1);
        std::vector<uint8_t> z; z.push_back(post); put_uvar(z,nbits); z.insert(z.end(),bits.begin(),bits.end());
        return z;
    }
    if(post==kBwtPostRawStream) {
        auto s=encode_stream_smallest(bwt); std::vector<uint8_t> z; z.push_back(kBwtPostRawStream); z.insert(z.end(),s.begin(),s.end());
        return z;
    }
    if(post==kBwtPostQlfc) {
        return bwt_qlfc_encode_real(bwt);
    }
    throw std::runtime_error("unknown BWT postcoder id");
}

// Serialize postcoder candidates ONE AT A TIME, retaining only the current best
// (memory cap). At most 2 candidate payloads + the BWT/MTF intermediates are
// live at once; intermediates are released right after selection.
std::vector<uint8_t> bwt_backend_encode(const std::vector<uint8_t>& in,const Options& opt) {
    if(in.empty()) throw std::runtime_error("BWT backend requires nonempty input");
    if(in.size()>static_cast<size_t>(std::numeric_limits<int32_t>::max())) throw std::runtime_error("BWT input too large");
    const int32_t n=static_cast<int32_t>(in.size());
    std::vector<uint8_t> bwt(in.size()); std::vector<int32_t> tmp(in.size());
    int32_t primary=libsais_bwt(in.data(),bwt.data(),tmp.data(),n,0,nullptr);
    // libsais primary is 1-based and INCLUSIVE of n: libsais_unbwt_aux requires
    // I[0] in [1,n] (and I[0]==n for n<=1). Do NOT remap n->1 the old code did:
    // measured, for inputs whose BWT primary is n, unbwt(...,1) reconstructs the
    // WRONG string while unbwt(...,n) is exact. Store the returned index as-is.
    if(primary<1 || primary>n) throw std::runtime_error("libsais BWT failed");
    if(n==1) {
        // BWT of a single byte is that byte. Primary must be 1 (== n); emit the
        // raw-stream postcoder framing so the decoder's normal raw branch reads a
        // valid stream and libsais_unbwt performs the n==1 copy.
        auto s=encode_stream_smallest(bwt);
        std::vector<uint8_t> z; z.push_back(kBwtPostRawStream); put_uvar(z,1u); z.insert(z.end(),s.begin(),s.end());
        return z;
    }
    BwtMtfRle mr=bwt_mtf_rle(bwt);
    std::array<uint8_t,5> ids{{kBwtPostStaticMtf,kBwtPostArithO0,kBwtPostArithO1,kBwtPostRawStream,kBwtPostQlfc}};
    std::vector<uint8_t> best;
    auto try_post=[&](uint8_t post){
        // Postcoders 0 (static MTF) and 4 (QLFC) are documented/ablation IDs but
        // currently have encoder/decoder mismatches: post0 fails in the stream-suite
        // Huffman path ("invalid huffman code"); post4 has an arithmetic-model desync
        // on rank-0 runs (the encoder RLEs zeros out of the arithmetic stream but the
        // decoder updates the model on them). They are rejected up front so the binary
        // never ships a decoder that cannot read its own encoder output. Auto selection
        // skips them and falls back to 1/2/3 (it already selects postcoder 2, the
        // canonical winner), so canonical numbers are unaffected; forcing them errors
        // clearly instead of producing corrupt output.
        if(post==kBwtPostStaticMtf || post==kBwtPostQlfc) {
            if(opt.bwt_post>=0 && opt.bwt_post==static_cast<int>(post))
                throw std::runtime_error("BWT postcoder "+std::to_string(post)+" is not supported yet (known encoder/decoder mismatch); use --bwt-post in {1,2,3}");
            return; // skip in auto mode
        }
        if(opt.bwt_post>=0 && opt.bwt_post!=static_cast<int>(post)) return; // forced ablation
        std::vector<uint8_t> payload=bwt_postcoder_payload(post,bwt,mr);
        std::vector<uint8_t> cand; cand.push_back(post); put_uvar(cand,static_cast<uint32_t>(primary));
        cand.insert(cand.end(),payload.begin()+1,payload.end()); // skip duplicate postcoder byte
        if(best.empty() || cand.size()<best.size()){ best=std::move(cand); }
        payload.clear(); payload.shrink_to_fit();
    };
    for(uint8_t id:ids) try_post(id);
    // release MTF intermediates now (memory cap)
    mr.tokens.clear(); mr.tokens.shrink_to_fit(); mr.runs.clear(); mr.runs.shrink_to_fit();
    return best;
}

static std::vector<uint8_t> bwt_backend_decode(const uint8_t* p,size_t n,size_t expected) {
    if(expected==0 || expected>static_cast<size_t>(std::numeric_limits<int32_t>::max())) throw std::runtime_error("bad BWT output size");
    const uint8_t* e=p+n; if(p>=e) throw std::runtime_error("truncated BWT backend header"); uint8_t post=*p++;
    // libsais primary is 1-based in [1,n]; n is a VALID index (do not remap).
    uint64_t pv=get_uvar(p,e); if(pv<1 || pv>expected) throw std::runtime_error("bad BWT primary index"); int32_t primary=static_cast<int32_t>(pv);
    std::vector<uint8_t> bwt;
    if(post==kBwtPostStaticMtf) {
        uint64_t tn=get_uvar(p,e); if(tn>uint64_t(e-p)) throw std::runtime_error("truncated BWT token stream");
        const uint8_t* q=p; const uint8_t* qe=p+tn; auto tokens=decode_stream(q,qe,expected+16); if(q!=qe)throw std::runtime_error("BWT token stream trailing bytes"); p=qe;
        uint64_t rn=get_uvar(p,e); if(rn>uint64_t(e-p)) throw std::runtime_error("truncated BWT run stream");
        q=p; qe=p+rn; auto runs=decode_stream(q,qe,expected+16); if(q!=qe)throw std::runtime_error("BWT run stream trailing bytes"); p=qe;
        if(p!=e) throw std::runtime_error("BWT backend trailing bytes"); bwt=bwt_mtf_expand(tokens,runs,expected);
    } else if(post==kBwtPostArithO0 || post==kBwtPostArithO1) {
        uint64_t nbits=get_uvar(p,e); bwt=bwt_arith_decode(p,static_cast<size_t>(e-p),expected,post==kBwtPostArithO1,nbits); p=e;
    } else if(post==kBwtPostRawStream) {
        const uint8_t* q=p; bwt=decode_stream(q,e,expected); if(q!=e)throw std::runtime_error("BWT raw stream trailing bytes"); p=e;
        if(bwt.size()!=expected) throw std::runtime_error("BWT raw stream size mismatch");
    } else if(post==kBwtPostQlfc) {
        bwt=bwt_qlfc_decode(p,static_cast<size_t>(e-p),expected,static_cast<uint64_t>(primary)); p=e;
    } else throw std::runtime_error("unknown BWT postcoder id");
    std::vector<uint8_t> out(expected); std::vector<int32_t> tmp(expected+1);
    if(libsais_unbwt(bwt.data(),out.data(),tmp.data(),static_cast<int32_t>(expected),nullptr,primary)!=0) throw std::runtime_error("libsais inverse BWT failed");
    return out;
}
#endif

static std::vector<uint8_t> ratio_backend_ids(const Options& opt) {
    std::vector<uint8_t> ids;
    auto add_brotli=[&](){
#ifdef ANVIL_HAVE_BROTLI
        ids.push_back(kRatioBackendBrotli);
#else
        throw std::runtime_error("Brotli ratio backend not available in this build");
#endif
    };
    auto add_bwt=[&](){
#ifdef ANVIL_HAVE_LIBSAIS
        ids.push_back(kRatioBackendBwt);
#else
        throw std::runtime_error("BWT ratio backend not available in this build");
#endif
    };
    if(opt.ratio_backend=="brotli") add_brotli();
    else if(opt.ratio_backend=="bwt") add_bwt();
    else if(opt.ratio_backend=="auto") {
#ifdef ANVIL_HAVE_BROTLI
        ids.push_back(kRatioBackendBrotli);
#endif
#ifdef ANVIL_HAVE_LIBSAIS
        ids.push_back(kRatioBackendBwt);
#endif
        if(ids.empty()) throw std::runtime_error("no ratio backend available in this build");
    } else throw std::runtime_error("unknown ratio backend: "+opt.ratio_backend);
    return ids;
}

static std::vector<uint8_t> ratio_backend_encode(uint8_t backend,const std::vector<uint8_t>& in,const Options& opt) {
#ifdef ANVIL_HAVE_BROTLI
    if(backend==kRatioBackendBrotli) return brotli_q11_lw30_encode(in);
#endif
#ifdef ANVIL_HAVE_LIBSAIS
    if(backend==kRatioBackendBwt) {
        // Independent BWT sub-block cap inside a rev-2 ratio block: split a large
        // BWT input into <= bwt_subblock pieces, each independently BWT-encoded,
        // so neither encode memory nor a single inverse-BWT dominates.
        // Wire:
        //   if in <= cap:  bare bwt_backend_encode payload (byte-identical to old).
        //   else:          0xFF (framing tag) uvar(n_subblocks)
        //                  per sub block: uvar(decoded_len) uvar(payload_len) payload
        // The 0xFF tag cannot collide with a bare payload, whose first byte is a
        // postcoder id in 0..4 (consumed by the decoder's 0xFF check at
        // ratio_backend_decode).
        const size_t cap=static_cast<size_t>(opt.bwt_subblock);
        if(in.size()<=cap) return bwt_backend_encode(in,opt);
        std::vector<uint8_t> z; z.push_back(0xFF); size_t off=0; uint32_t nsub=0;
        std::vector<std::pair<size_t,std::vector<uint8_t>>> parts;
        while(off<in.size()) {
            size_t len=std::min<size_t>(cap,in.size()-off);
            parts.emplace_back(len,bwt_backend_encode(std::vector<uint8_t>(in.data()+off,in.data()+off+len),opt));
            off+=len; ++nsub;
        }
        put_uvar(z,nsub);
        for(auto&pr:parts){ put_uvar(z,pr.first); put_uvar(z,pr.second.size()); z.insert(z.end(),pr.second.begin(),pr.second.end()); }
        return z;
    }
#endif
    throw std::runtime_error("unknown ratio backend id");
}

static std::vector<uint8_t> ratio_backend_decode(uint8_t backend,const uint8_t* p,size_t n,size_t expected,const Options& opt) {
#ifdef ANVIL_HAVE_BROTLI
    if(backend==kRatioBackendBrotli) return brotli_lw_decode_exact(p,n,expected);
#endif
#ifdef ANVIL_HAVE_LIBSAIS
    if(backend==kRatioBackendBwt) {
        const uint8_t* e=p+n;
        // Subblock framing uses a 0xFF tag as its first byte; a bare single-subblock
        // payload starts with a postcoder id in 0..4, so the tag cannot collide.
        if(p<e && *p==0xFF) {
            ++p; uint64_t nsub=get_uvar(p,e);
            std::vector<uint8_t> out; out.reserve(expected);
            for(uint64_t i=0;i<nsub;++i) {
                uint64_t dlen=get_uvar(p,e); uint64_t plen=get_uvar(p,e);
                if(plen>uint64_t(e-p)) throw std::runtime_error("truncated BWT subblock");
                auto sub=bwt_backend_decode(p,static_cast<size_t>(plen),static_cast<size_t>(dlen)); p+=plen;
                if(sub.size()!=static_cast<size_t>(dlen)) throw std::runtime_error("BWT subblock size mismatch");
                out.insert(out.end(),sub.begin(),sub.end());
            }
            if(out.size()!=expected) throw std::runtime_error("BWT subblock reconstruction size mismatch");
            return out;
        }
        return bwt_backend_decode(p,n,expected);
    }
#endif
    (void)opt;
    throw std::runtime_error("unknown ratio backend id");
}

static std::vector<uint8_t> ratio_transform_ctx1(const std::vector<uint8_t>& d) {
    std::array<std::vector<uint8_t>,256> s;
    uint8_t prev=0;
    for(uint8_t b:d){ s[prev].push_back(b); prev=b; }
    std::vector<uint8_t> out;
    out.reserve(d.size()+768);
    for(const auto& v:s) put_uvar(out,v.size());
    for(const auto& v:s) out.insert(out.end(),v.begin(),v.end());
    return out;
}

static std::vector<uint8_t> ratio_inverse_ctx1(const std::vector<uint8_t>& x,size_t out_len) {
    const uint8_t* p=x.data(); const uint8_t* e=x.data()+x.size();
    std::array<size_t,256> off{}, end{};
    uint64_t sum=0;
    std::array<uint64_t,256> len{};
    for(size_t i=0;i<256;++i){ len[i]=get_uvar(p,e); if(len[i]>out_len || sum>out_len-len[i]) throw std::runtime_error("bad ratio ctx lengths"); sum+=len[i]; }
    if(sum!=out_len) throw std::runtime_error("ratio ctx length sum mismatch");
    size_t base=static_cast<size_t>(p-x.data());
    if(base> x.size() || out_len!=x.size()-base) throw std::runtime_error("ratio ctx transformed size mismatch");
    size_t cur=base;
    for(size_t i=0;i<256;++i){ off[i]=cur; cur+=static_cast<size_t>(len[i]); end[i]=cur; }
    std::vector<uint8_t> out; out.reserve(out_len);
    uint8_t prev=0;
    for(size_t i=0;i<out_len;++i){ size_t k=prev; if(off[k]>=end[k]) throw std::runtime_error("ratio ctx stream underflow"); uint8_t b=x[off[k]++]; out.push_back(b); prev=b; }
    for(size_t i=0;i<256;++i) if(off[i]!=end[i]) throw std::runtime_error("ratio ctx stream trailing bytes");
    return out;
}

static bool ratio_transform_lines(const std::vector<uint8_t>& d,std::vector<uint8_t>& out) {
    constexpr size_t kMaxRecord=4096;
    std::vector<uint32_t> lens;
    lens.reserve(d.size()/64+1);
    size_t start=0,maxlen=0;
    for(size_t i=0;i<d.size();++i) if(d[i]=='\n') {
        size_t len=i+1-start;
        if(len>kMaxRecord) return false;
        lens.push_back(static_cast<uint32_t>(len)); maxlen=std::max(maxlen,len); start=i+1;
    }
    if(start<d.size()) { size_t len=d.size()-start; if(len>kMaxRecord) return false; lens.push_back(static_cast<uint32_t>(len)); maxlen=std::max(maxlen,len); }
    if(lens.size()<8 || maxlen==0) return false;
    out.clear(); out.reserve(d.size()+lens.size()*2+16);
    put_uvar(out,lens.size()); for(uint32_t n:lens) put_uvar(out,n);
    std::vector<size_t> pos(lens.size());
    size_t acc=0; for(size_t i=0;i<lens.size();++i){pos[i]=acc;acc+=lens[i];}
    if(acc!=d.size()) throw std::runtime_error("line transform accounting bug");
    for(size_t col=0;col<maxlen;++col) for(size_t r=0;r<lens.size();++r) if(col<lens[r]) out.push_back(d[pos[r]+col]);
    return true;
}

static std::vector<uint8_t> ratio_inverse_lines(const std::vector<uint8_t>& x,size_t out_len) {
    const uint8_t* p=x.data(); const uint8_t* e=x.data()+x.size();
    uint64_t nr=get_uvar(p,e); if(nr==0 || nr>out_len) throw std::runtime_error("bad ratio line count");
    std::vector<uint32_t> lens(static_cast<size_t>(nr)); size_t maxlen=0; uint64_t sum=0;
    for(size_t i=0;i<lens.size();++i){ uint64_t n=get_uvar(p,e); if(n==0 || n>4096 || sum>out_len-n) throw std::runtime_error("bad ratio line length"); lens[i]=static_cast<uint32_t>(n); sum+=n; maxlen=std::max(maxlen,static_cast<size_t>(n)); }
    if(sum!=out_len) throw std::runtime_error("ratio line length sum mismatch");
    size_t meta=static_cast<size_t>(p-x.data()); if(meta> x.size() || out_len!=x.size()-meta) throw std::runtime_error("ratio line transformed size mismatch");
    std::vector<uint8_t> out(out_len); std::vector<size_t> rowoff(lens.size());
    size_t acc=0; for(size_t i=0;i<lens.size();++i){rowoff[i]=acc;acc+=lens[i];}
    size_t ip=meta;
    for(size_t col=0;col<maxlen;++col) for(size_t r=0;r<lens.size();++r) if(col<lens[r]) { if(ip>=x.size()) throw std::runtime_error("ratio line stream underflow"); out[rowoff[r]+col]=x[ip++]; }
    if(ip!=x.size()) throw std::runtime_error("ratio line stream trailing bytes");
    return out;
}

// Optional LZP prepass (transform id 3) for the BWT backend: remove predictable
// long phrases before sorting. The residue is stored with an explicit side
// table so the decoder reconstructs the original block exactly. Ablation
// semantics: same block, same BWT, same postcoder, LZP OFF vs ON.
static std::vector<uint8_t> ratio_wrap(uint8_t transform,uint8_t backend,const std::vector<uint8_t>& transformed,const Options& opt) {
    auto z=ratio_backend_encode(backend,transformed,opt);
    std::vector<uint8_t> p; p.reserve(2+10+z.size());
    p.push_back(transform); p.push_back(backend); put_uvar(p,transformed.size());
    p.insert(p.end(),z.begin(),z.end()); return p;
}

// Build the BWT backend with an independent BWT sub-block cap and (optionally)
// an LZP prepass. When --bwt-lzp=on we expose the LZP residue as an EXTERNAL
// ratio transform (id 3): the "transformed" data stored for transform 3 is the
// LZP side table (literals then matches, both stream-suite coded), and the
// backend payload is a plain BWT of the residue. The decoder inverts BWT,
// reconstructs the residue (literals||matches) from the side table, then
// replays the LZP story to recover the original block — exact and reversible.
// When LZP is off (default), this just returns a normal direct BWT backend
// payload. Ablation semantics: same block, same BWT, same postcoder, LZP off→3.
// (These helpers depend on bwt_backend_encode, which is only built with libsais.)
#ifdef ANVIL_HAVE_LIBSAIS
static std::vector<uint8_t> bwt_backend_encode_lzp(const std::vector<uint8_t>& in,const Options& opt) {
    // plain path (no LZP): direct BWT with sub-block cap
    return bwt_backend_encode(in,opt);
}

// Encode a block under transform 3 (LZP residue): returns the full mode-17
// ratio payload [transform=3, backend=bwt, uvar(transformed_size), backend...].
[[maybe_unused]] static std::vector<uint8_t> encode_ratio_lzp_block(const std::vector<uint8_t>& d,const Options& opt) {
    auto pp=bwt_lzp_preprocess(d);
    std::vector<uint8_t> residue; residue.reserve(pp.literals.size()+pp.matches.size());
    residue.insert(residue.end(),pp.literals.begin(),pp.literals.end());
    residue.insert(residue.end(),pp.matches.begin(),pp.matches.end());
    std::vector<uint8_t> lzp_payload;
    { // side table = uvar(literal_len), then literals-stream, then matches-stream
        put_uvar(lzp_payload,pp.literals.size());
        auto ls=encode_stream_smallest(pp.literals); lzp_payload.insert(lzp_payload.end(),ls.begin(),ls.end());
        auto ms=encode_stream_smallest(pp.matches); lzp_payload.insert(lzp_payload.end(),ms.begin(),ms.end());
    }
    std::vector<uint8_t> bw=bwt_backend_encode(residue,opt);
    std::vector<uint8_t> p; p.push_back(3); p.push_back(kRatioBackendBwt); put_uvar(p,lzp_payload.size());
    p.insert(p.end(),bw.begin(),bw.end()); p.insert(p.end(),lzp_payload.begin(),lzp_payload.end());
    return p;
}
#endif // ANVIL_HAVE_LIBSAIS

static std::vector<uint8_t> encode_ratio_block(const std::vector<uint8_t>& d,const Options& opt) {
    const auto backends=ratio_backend_ids(opt); std::vector<uint8_t> best;
    auto consider=[&](uint8_t transform,const std::vector<uint8_t>& x){
        for(uint8_t backend:backends){
            std::vector<uint8_t> p;
#ifdef ANVIL_HAVE_LIBSAIS
            if(backend==kRatioBackendBwt && transform==3) {
                p=encode_ratio_lzp_block(x,opt); // LZP residue path
            } else
#endif
            {
                p=ratio_wrap(transform,backend,x,opt);
            }
            if(best.empty()||p.size()<best.size())best=std::move(p);
        }
    };
    // Transform 3 (LZP residue) is DISABLED (clean rejection): the uncommitted
    // wire had no backend-payload length so [bwt][side-table] was underdefined,
    // and LZP is a recorded NO-GO (06 §G2 / E6). Never emit it; force the flag
    // to fail loudly instead (postcoder-0/4 pattern).
    if(opt.bwt_lzp) throw std::runtime_error("--bwt-lzp=on is not supported: ratio transform 3 (LZP residue) is disabled (do-not-reburn 06 G2)");
    consider(0,d);
    if(opt.ratio_context) { auto x=ratio_transform_ctx1(d); consider(1,x); }
    if(opt.ratio_lines) { std::vector<uint8_t> x; if(ratio_transform_lines(d,x)) consider(2,x); }
    return best;
}

static std::vector<uint8_t> decode_ratio_block(const uint8_t* p,size_t n,size_t out_len,const Options& opt) {
    if(n<3) throw std::runtime_error("truncated ratio payload");
    const uint8_t* e=p+n; uint8_t transform=*p++; uint8_t backend=*p++;
    if(transform>3) throw std::runtime_error("unknown ratio transform id");
    if(backend!=kRatioBackendBrotli && backend!=kRatioBackendBwt) throw std::runtime_error("unknown ratio backend id");
    uint64_t xlen=get_uvar(p,e); uint64_t maxx=2ull*out_len+4096;
    if(xlen>maxx || xlen>std::numeric_limits<size_t>::max()) throw std::runtime_error("ratio transformed size bound");
    // Transform 3 (LZP residue) is disabled with a clean rejection: the
    // uncommitted wire carried no backend-payload length, so [bwt][side-table]
    // was underdefined. LZP is a recorded NO-GO (06 §G2 / E6); the encoder
    // refuses --bwt-lzp=on and never emits transform 3.
    if(transform==3) throw std::runtime_error("ratio transform 3 (LZP residue) is not supported");
    auto x=ratio_backend_decode(backend,p,static_cast<size_t>(e-p),static_cast<size_t>(xlen),opt);
    if(transform==0){ if(x.size()!=out_len) throw std::runtime_error("ratio direct size mismatch"); return x; }
    if(transform==1) return ratio_inverse_ctx1(x,out_len);
    return ratio_inverse_lines(x,out_len); // transform==2 (validated above)
}

static std::vector<uint8_t> compress(const std::vector<uint8_t>& input, const Options& opt, GlobalStats* gs) {
    g_stream_suite = opt.stream_suite;
    g_stream_ctx = opt.stream_ctx;
    g_stream_lambda = opt.stream_lambda;
    g_rlz_reap = opt.hotop_rlzp;
    g_hotop_budget = opt.hotop_budget ? true : false;
    g_stream_log = opt.stream_log;
    g_j_agree = 0; g_j_total = 0; g_stream_log_entries.clear();
    const uint8_t revision=opt.parse=="ratio"?2:1;
    std::vector<uint8_t> out={'A','N','V','0',revision};
    put_uvar(out,opt.block_size); put_uvar(out,input.size());
    GlobalStats st; st.in=input.size();
    for(size_t off=0; off<input.size();) {
        size_t blen=std::min<size_t>(opt.block_size,input.size()-off);
        std::vector<uint8_t> block(input.begin()+off,input.begin()+off+blen);

        if(opt.parse=="ratio") {
            auto payload=encode_ratio_block(block,opt);
            ++st.blocks; st.literals+=blen;
            put_uvar(out,blen); uint32_t sum=crc32(block.data(),block.size());
            if(payload.size()+1<block.size()) { out.push_back(17); put_uvar(out,payload.size()); put_u32le(out,sum); out.insert(out.end(),payload.begin(),payload.end()); ++st.compressed_blocks; }
            else { out.push_back(0); put_uvar(out,block.size()); put_u32le(out,sum); out.insert(out.end(),block.begin(),block.end()); ++st.raw_blocks; }
            off+=blen; continue;
        }

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
            sp_toks=parse_sparse(block,opt.max_chain,opt.max_match,opt.surprise,opt.boundary,opt.channels); have_sp=true;
            auto payload=encode_tokens_sparse(block,sp_toks);
            std::vector<Token> t; t.reserve(sp_toks.size());
            for(auto&s:sp_toks) t.push_back({s.type!=0,s.pos,s.len,s.dist});
            if(best.mode==0 || payload.size()<best.payload.size()) best={std::move(payload),std::move(t),11};
        }
        // Mode 12 (SHAPE): per-shape displacement prediction over either the
        // sparse parse (types 0/1/2) or the mdl parse (exact-only).
        if(opt.parse=="auto" || opt.parse=="shape") {
            if(!have_sp) { sp_toks=parse_sparse(block,opt.max_chain,opt.max_match,opt.surprise,opt.boundary,opt.channels); have_sp=true; }
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
            if(!have_sp) { sp_toks=parse_sparse(block,opt.max_chain,opt.max_match,opt.surprise,opt.boundary,opt.channels); have_sp=true; }
            auto payload=encode_tokens_topology(block,sp_toks,opt.shape_states);
            std::vector<Token> t; t.reserve(sp_toks.size());
            for(auto&s:sp_toks) t.push_back({s.type!=0,s.pos,s.len,s.dist});
            if(best.mode==0 || payload.size()<best.payload.size()) best={std::move(payload),std::move(t),13};
        }
        // Mode 14 (TCOPY): implicit Delta=-d transformed copy over the sparse parse.
        if(opt.parse=="auto" || opt.parse=="tcopy") {
            // PNRA (opt.pnra) needs its OWN tcopy-aware parse into a separate
            // variable, never reusing/overwriting the shared sp_toks — under
            // --parse=auto, sp_toks (tcopy=false) is also reused by mode 15
            // (HOTOP), which does not understand type-3 tokens; sharing would
            // silently corrupt that mode's input.
            const std::vector<SparseToken>* tc_toks = &sp_toks;
            std::vector<SparseToken> tc_local;
            if(!have_sp) { sp_toks=parse_sparse(block,opt.max_chain,opt.max_match,opt.surprise,opt.boundary,opt.channels,true,opt.pnra); have_sp=true; tc_toks=&sp_toks; }
            else if(opt.pnra) { tc_local=parse_sparse(block,opt.max_chain,opt.max_match,opt.surprise,opt.boundary,opt.channels,true,true); tc_toks=&tc_local; }
            auto payload=encode_tokens_tcopy(block,*tc_toks);
            std::vector<Token> t; t.reserve(tc_toks->size());
            for(auto&s:*tc_toks) t.push_back({s.type!=0,s.pos,s.len,s.dist});
            if(best.mode==0 || payload.size()<best.payload.size()) best={std::move(payload),std::move(t),14};
        }
        // Mode 16 (ARI-REF): additive arithmetic reference (Experiment Z).
        // Router-gated behind opt.ariref so the default/auto path is bit-identical
        // to the pre-mode-16 build (pre-reg control §5.1 flag-off byte-identity).
        if(opt.ariref) {
            std::vector<SparseToken> ar_toks=parse_ariref(block,opt.max_match);
            auto payload=encode_tokens_ariref(block,ar_toks);
            std::vector<Token> t; t.reserve(ar_toks.size());
            for(auto&s:ar_toks) t.push_back({s.type!=0,s.pos,s.len,s.dist});
            if(best.mode==0 || payload.size()<best.payload.size()) best={std::move(payload),std::move(t),16};
        }
        // Mode 15 (HOTOP): compiled hot-op instruction book over the sparse parse.
        if(opt.parse=="auto" || opt.parse=="hotop") {
            if(!have_sp) { sp_toks=parse_sparse(block,opt.max_chain,opt.max_match,opt.surprise,opt.boundary,opt.channels,false); have_sp=true; }
            auto payload=encode_tokens_hotop(block,sp_toks,opt.shape_states);
            std::vector<Token> t; t.reserve(sp_toks.size());
            for(auto&s:sp_toks) t.push_back({s.type!=0,s.pos,s.len,s.dist});
            if(best.mode==0 || payload.size()<best.payload.size()) best={std::move(payload),std::move(t),15};
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

// Decode exactly one block given its already-parsed mode byte and a pointer to
// the payload, its payload length plen, declared output length blen, and
// expected CRC. Wire header order is [blen][mode][plen][crc][payload...], so the
// caller parses mode/plen/crc and hands this function the payload pointer.
// Used by both the serial and the parallel decode paths so the dispatch logic
// exists in exactly one place. Throws on any malformed-block error; the caller
// owns CRC verification for parallel (to keep CRC checks in the serial path too).
static std::vector<uint8_t> decode_one_block(uint8_t mode,const uint8_t* p,size_t plen,size_t blen,uint32_t expected_crc,
                                             uint8_t revision,uint64_t block_size,const uint8_t* e,const Options& opt) {
    // p points at the block PAYLOAD (plen bytes); mode was parsed by the caller.
    if(revision==2 && mode!=0 && mode!=17) throw std::runtime_error("block mode not valid for revision 2");
    if(mode==0) {
        if(plen!=blen) throw std::runtime_error("raw block length mismatch");
        return std::vector<uint8_t>(p,p+plen);
    } else if(mode>=1 && mode<=5) {
        return decode_tokens(p,plen,blen,mode);
    } else if(mode==10) {
        return decode_tokens_rans(p,plen,blen);
    } else if(mode==11) {
        return decode_tokens_sparse(p,plen,blen);
    } else if(mode==12) {
        return g_fused_decode ? decode_tokens_shape_fused(p,plen,blen) : decode_tokens_shape(p,plen,blen);
    } else if(mode==13) {
        return decode_tokens_topology(p,plen,blen);
    } else if(mode==14) {
        return decode_tokens_tcopy(p,plen,blen);
    } else if(mode==15) {
        return decode_tokens_hotop_fused(p,plen,blen);
    } else if(mode==16) {
        return decode_tokens_ariref(p,plen,blen);
    } else if(mode==17 && revision==2) {
        return decode_ratio_block(p,plen,blen,opt);
    }
    throw std::runtime_error("unknown block mode");
}

static std::vector<uint8_t> decompress(const std::vector<uint8_t>& in,const Options& opt) {
    if(in.size()<5 || std::memcmp(in.data(),"ANV0",4)!=0 || (in[4]!=1 && in[4]!=2)) throw std::runtime_error("not a supported ANVIL revision");
    const uint8_t revision=in[4];
    const uint8_t* p=in.data()+5; const uint8_t* e=in.data()+in.size();
    uint64_t block_size=get_uvar(p,e);
    const uint64_t max_block=revision==1?(64ull<<20):(128ull<<20);
    if(block_size==0 || block_size>max_block) throw std::runtime_error("invalid block size");
    uint64_t total=get_uvar(p,e); if(total>std::numeric_limits<size_t>::max()) throw std::runtime_error("output too large");
    // DoS guard: output cannot legitimately exceed (max blocks) * (max block size);
    // each block needs >= 7 header bytes; max_block is revision-bounded.
    if(total > ((uint64_t)in.size()/7 + 2) * max_block) throw std::runtime_error("declared size exceeds amplification bound");
    std::vector<uint8_t> out; out.reserve(static_cast<size_t>(std::min<uint64_t>(total,max_block)));
    // Each block decodes independently into a fixed-size output slot (blen) and is
    // CRC-verified, so block decode is embarrassingly parallel. decode_threads==1
    // keeps the exact serial path (zero behavioral change); >1 uses a thread pool
    // that decodes each block into its own segment vector, then concatenates in
    // order. Every block is CRC-checked after decode. The decode helpers read only
    // shared read-only state (g_fused_decode is set once in compress), so parallel
    // decoding into disjoint output buffers is safe.
    if(opt.decode_threads<=1) {
        while(out.size()<total) {
            uint64_t blen=get_uvar(p,e); if(blen==0 || blen>block_size) throw std::runtime_error("invalid block length");
            if(p>=e) throw std::runtime_error("truncated block header");
            uint8_t mode=*p++; uint64_t plen=get_uvar(p,e); uint32_t expected_crc=get_u32le(p,e);
            if(plen>uint64_t(e-p)) throw std::runtime_error("truncated block payload");
            if(blen>total-out.size()) throw std::runtime_error("block exceeds declared output");
            auto b=decode_one_block(mode,p,static_cast<size_t>(plen),static_cast<size_t>(blen),expected_crc,revision,block_size,e,opt);
            /*DIAGNOSTIC nc: CRC verify skipped*/
            out.insert(out.end(),b.begin(),b.end()); p+=plen;
        }
    } else {
        struct Seg { uint8_t mode; const uint8_t* p; size_t plen; size_t blen; uint32_t crc; };
        std::vector<Seg> segs; segs.reserve(256);
        const uint8_t* q=p;
        while(static_cast<uint64_t>(out.size())<total) {
            uint64_t blen=get_uvar(q,e); if(blen==0 || blen>block_size) throw std::runtime_error("invalid block length");
            if(q>=e) throw std::runtime_error("truncated block header");
            uint8_t mode=*q++; uint64_t plen=get_uvar(q,e); uint32_t expected_crc=get_u32le(q,e);
            if(plen>uint64_t(e-q)) throw std::runtime_error("truncated block payload");
            if(blen>total-out.size()) throw std::runtime_error("block exceeds declared output");
            segs.push_back({mode,q,static_cast<size_t>(plen),static_cast<size_t>(blen),expected_crc});
            out.resize(out.size()+static_cast<size_t>(blen));
            q+=plen;
        }
        if(static_cast<uint64_t>(out.size())!=total) throw std::runtime_error("size mismatch");
        // Decode into per-segment vectors (disjoint buffers), CRC-verify, then join in order.
        std::vector<std::vector<uint8_t>> blocks(segs.size());
        const size_t nthreads=std::min<size_t>(opt.decode_threads,segs.size());
        std::vector<std::string> errs(nthreads);
        auto worker=[&](size_t ti){
            try {
                for(size_t i=ti;i<segs.size();i+=nthreads) {
                    const Seg& s=segs[i];
                    auto b=decode_one_block(s.mode,s.p,s.plen,s.blen,s.crc,revision,block_size,e,opt);
                    /*DIAGNOSTIC nc: CRC verify skipped*/
                    blocks[i]=std::move(b);
                }
            } catch(const std::exception& ex){ errs[ti]=ex.what(); }
        };
        std::vector<std::thread> pool;
        for(size_t i=0;i<nthreads;++i) pool.emplace_back(worker,i);
        for(auto&t:pool) t.join();
        for(auto&er:errs) if(!er.empty()) throw std::runtime_error("parallel decode: "+er);
        out.clear();
        for(auto& b:blocks) out.insert(out.end(),b.begin(),b.end());
        p=q;
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
              << "  anvil c <input> <output> [--parse=auto|dp|greedy|sparse|mdl|shape|topology|tcopy|hotop|ratio] [--literal=auto|o0|o1|g4|g8|g16] [--entropy=auto|arith|rans|sparse] [--block=N] [--chain=N] [--max-match=N] [--surprise=N] [--shape-states=1|28] [--boundary=on|off] [--negate=on|off] [--channels=on|off] [--pnra=on|off] [--stream-suite=on|off] [--stream-lambda=N] [--hotop-rlzp=on|off] [--hotop-budget=on|off] [--ratio-context=on|off] [--ratio-lines=on|off] [--ratio-backend=brotli|bwt|auto] [--stream-log] [--fused-decode=on|off] [--quiet]\n"
              << "  anvil d <input> <output> [--quiet]\n"
              << "  anvil verify <input> [--parse=auto|dp|greedy|sparse|mdl|shape|topology|tcopy|hotop] [--literal=auto|o0|o1|g4|g8|g16] [--entropy=auto|arith|rans|sparse]\n"
              << "  note: sparse->11, shape->12, topology->13, tcopy->14, hotop->15, ariref->16, ratio->17 (rev-2 transform + explicit backend registry)\n"
              << "  note: --surprise=N is the sparse-parser mismatch budget (default 12); --boundary/--negate are C5 adopts\n";
}

} // namespace anvil

#ifndef ANVIL_NO_MAIN
int main(int argc,char**argv) {
    using namespace anvil;
    try {
        if(argc<3){usage();return 2;}
        std::string cmd=argv[1]; Options opt; bool block_explicit=false;
        if(const char* env=getenv("ANVIL_STREAM_LAMBDA")) opt.stream_lambda=std::atof(env);
        for(int i=(cmd=="verify"?3:4);i<argc;++i) {
            std::string a=argv[i];
            if(a.rfind("--parse=",0)==0)opt.parse=a.substr(8);
            else if(a.rfind("--literal=",0)==0)opt.literal=a.substr(10);
            else if(a.rfind("--entropy=",0)==0)opt.entropy=a.substr(10);
            else if(a.rfind("--block=",0)==0){opt.block_size=std::stoul(a.substr(8));block_explicit=true;}
            else if(a.rfind("--chain=",0)==0)opt.max_chain=std::stoul(a.substr(8));
            else if(a.rfind("--max-match=",0)==0)opt.max_match=std::stoul(a.substr(12));
            else if(a.rfind("--surprise=",0)==0)opt.surprise=std::stoul(a.substr(11));
            else if(a.rfind("--shape-states=",0)==0)opt.shape_states=std::stoul(a.substr(15));
            else if(a.rfind("--boundary=",0)==0)opt.boundary=(a.substr(11)!="off");
            else if(a.rfind("--negate=",0)==0)opt.negate=(a.substr(9)!="off");
            else if(a.rfind("--channels=",0)==0)opt.channels=(a.substr(11)!="off");
            else if(a.rfind("--pnra=",0)==0)opt.pnra=(a.substr(7)!="off");
            else if(a.rfind("--stream-suite=",0)==0)opt.stream_suite=(a.substr(15)!="off");
            else if(a.rfind("--stream-ctx=",0)==0)opt.stream_ctx=(a.substr(13)!="off");
            else if(a.rfind("--fused-decode=",0)==0)g_fused_decode=(a.substr(15)!="off");
            else if(a.rfind("--stream-lambda=",0)==0)opt.stream_lambda=std::stod(a.substr(16));
            else if(a.rfind("--hotop-rlzp=",0)==0)opt.hotop_rlzp=(a.substr(13)!="off");
            else if(a.rfind("--hotop-budget=",0)==0)opt.hotop_budget=(a.substr(15)!="off");
            else if(a.rfind("--ariref=",0)==0)opt.ariref=(a.substr(9)!="off");
            else if(a.rfind("--ratio-context=",0)==0)opt.ratio_context=(a.substr(16)!="off");
            else if(a.rfind("--ratio-lines=",0)==0)opt.ratio_lines=(a.substr(14)!="off");
            else if(a.rfind("--ratio-backend=",0)==0)opt.ratio_backend=a.substr(16);
            else if(a.rfind("--bwt-post=",0)==0){ int v=std::stoi(a.substr(11)); if(v<-1||v>255) throw std::runtime_error("--bwt-post must be -1 or 0..255"); opt.bwt_post=v; }
            else if(a.rfind("--bwt-lzp=",0)==0)opt.bwt_lzp=(a.substr(10)!="off");
            else if(a.rfind("--bwt-subblock=",0)==0){ uint64_t v=std::stoull(a.substr(15)); if(v==0 || v>static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) throw std::runtime_error("--bwt-subblock must be 1..2GiB"); opt.bwt_subblock=static_cast<uint32_t>(v); }
            else if(a.rfind("--decode-threads=",0)==0){ uint64_t v=std::stoull(a.substr(17)); if(v==0 || v>1024) throw std::runtime_error("--decode-threads must be 1..1024"); opt.decode_threads=static_cast<uint32_t>(v); }
            else if(a=="--stream-log")opt.stream_log=true;
            else if(a=="--quiet")opt.quiet=true;
            else throw std::runtime_error("unknown option: "+a);
        }
        if(opt.parse!="auto"&&opt.parse!="dp"&&opt.parse!="greedy"&&opt.parse!="sparse"&&opt.parse!="mdl"&&opt.parse!="shape"&&opt.parse!="topology"&&opt.parse!="tcopy"&&opt.parse!="hotop"&&opt.parse!="ratio")throw std::runtime_error("parse must be auto, dp, greedy, sparse, mdl, shape, topology, tcopy, hotop or ratio");
        if(opt.parse=="ratio" && !block_explicit) opt.block_size=128u<<20;
        if(opt.parse=="ratio" && (opt.block_size==0 || opt.block_size>(128u<<20))) throw std::runtime_error("ratio block must be 1..128 MiB");
        if(opt.parse=="ratio" && opt.ratio_backend!="brotli" && opt.ratio_backend!="bwt" && opt.ratio_backend!="auto") throw std::runtime_error("ratio-backend must be brotli, bwt or auto");
        if(opt.parse!="ratio" && (opt.block_size==0 || opt.block_size>(64u<<20))) throw std::runtime_error("block must be 1..64 MiB for revision 1");
        if(opt.literal!="auto"&&opt.literal!="o0"&&opt.literal!="o1"&&opt.literal!="g4"&&opt.literal!="g8"&&opt.literal!="g16")throw std::runtime_error("literal must be auto, o0, o1, g4, g8 or g16");
        if(opt.entropy!="auto"&&opt.entropy!="arith"&&opt.entropy!="rans"&&opt.entropy!="sparse")throw std::runtime_error("entropy must be auto, arith, rans or sparse");
        if(opt.shape_states!=1 && opt.shape_states!=28)throw std::runtime_error("shape-states must be 1 or 28");
        if(opt.bwt_post!=-1 && (opt.bwt_post<0 || opt.bwt_post>4)) throw std::runtime_error("--bwt-post must be -1 or 0..4 (0=static-MTF 1=arith-o0 2=arith-o1 3=raw-BWT 4=QLFC)");
        if(cmd=="c") {
            if(argc<4){usage();return 2;} auto in=read_file(argv[2]); GlobalStats st;
            auto t0=std::chrono::steady_clock::now(); auto out=compress(in,opt,&st); auto t1=std::chrono::steady_clock::now(); write_file(argv[3],out);
            if(opt.stream_log) for(auto&e:g_stream_log_entries) std::cout<<"stream_log chosen="<<e.chosen<<" l_winner="<<e.l_winner<<" chosen_L="<<e.chosen_L<<" min_L="<<e.min_L<<"\n";
            if(!opt.quiet){double sec=std::chrono::duration<double>(t1-t0).count(); std::cerr<<"ANVIL c parse="<<opt.parse<<" literal="<<opt.literal<<" entropy="<<opt.entropy<<" in="<<st.in<<" out="<<st.out<<" ratio="<<(st.in?double(st.out)/st.in:0)<<" MB/s="<<(sec?st.in/1e6/sec:0)<<" blocks="<<st.blocks<<" compressed="<<st.compressed_blocks<<" raw="<<st.raw_blocks<<" literals="<<st.literals<<" matches="<<st.matches<<" matched_bytes="<<st.matched_bytes<<" j_agree="<<g_j_agree<<"/"<<g_j_total<<" ch_try="<<g_ch_try<<" ch_win="<<g_ch_win<<" ch_chosen="<<g_ch_chosen<<" span_win="<<g_ch_span_win<<" span_emit="<<g_ch_span_emit
              <<" pnra_gate="<<g_pnra_gate<<" pnra_idxhit="<<g_pnra_idxhit<<" pnra_verify="<<g_pnra_verify<<" pnra_commit="<<g_pnra_commit
              <<" diag_t3="<<g_diag_t3<<" reswords="<<g_diag_reswords<<" tmw="<<g_diag_tmw
              <<" streams_z=["<<g_diag_sz_z[0]<<"/"<<g_diag_sz_raw[0]
              <<","<<g_diag_sz_z[1]<<"/"<<g_diag_sz_raw[1]
              <<","<<g_diag_sz_z[2]<<"/"<<g_diag_sz_raw[2]
              <<","<<g_diag_sz_z[3]<<"/"<<g_diag_sz_raw[3]
              <<","<<g_diag_sz_z[4]<<"/"<<g_diag_sz_raw[4]
              <<","<<g_diag_sz_z[5]<<"/"<<g_diag_sz_raw[5]
              <<","<<g_diag_sz_z[6]<<"/"<<g_diag_sz_raw[6]
              <<","<<g_diag_sz_z[7]<<"/"<<g_diag_sz_raw[7]<<"]\n";}
        } else if(cmd=="d") {
            if(argc<4){usage();return 2;} auto in=read_file(argv[2]); auto t0=std::chrono::steady_clock::now(); auto out=decompress(in,opt); auto t1=std::chrono::steady_clock::now(); write_file(argv[3],out);
            if(!opt.quiet){double sec=std::chrono::duration<double>(t1-t0).count(); std::cerr<<"ANVIL d out="<<out.size()<<" MB/s="<<(sec?out.size()/1e6/sec:0)<<"\n";}
        } else if(cmd=="verify") {
            auto in=read_file(argv[2]); GlobalStats st; auto enc=compress(in,opt,&st); auto dec=decompress(enc,opt);
            if(dec!=in) throw std::runtime_error("round-trip mismatch");
            std::cout<<"OK bytes="<<in.size()<<" encoded="<<enc.size()<<" fnv64="<<std::hex<<fnv1a(in)<<std::dec<<"\n";
        } else { usage(); return 2; }
        return 0;
    } catch(const std::exception& e) { std::cerr<<"anvil: "<<e.what()<<"\n"; return 1; }
}

#endif // ANVIL_NO_MAIN
