// =====================================================================================
// ANVIL Iteration 11 / candidate D2 — "the postcoder's share of BWT-path decode"
//
// A standalone probe that measures, INSIDE ONE PROCESS and ONE JOB:
//
//   (1) the postcoder stage alone   (MTF + zero-run RLE + adaptive arithmetic -> BWT)
//   (2) the inverse BWT alone       (libsais LF walk over that SAME BWT string)
//   (3) the whole BWT-path decode   (the container's mode-17 ratio block, decoded)
//   (4) the reference Brotli one-shot decode of the same bytes
//   (5) ANVIL's streaming Brotli decode (256 KiB output window) against (4)
//
// WHY A COPY AND NOT A MODIFICATION OF src/anvil.cpp
// -------------------------------------------------
// `src/anvil.cpp` is NOT modified by this candidate (D3 owns mechanism changes).  The
// probe therefore carries VERBATIM COPIES of the code it measures, so that the decode
// path it times is structurally identical to the shipped one (same functions, same
// call graph, same compiler flags) and can be instrumented with two clock reads.
//
// PROVENANCE (all copies below)
// ------------------------------
//   src/anvil.cpp @ 89e4b32 (and every ancestor):
//     lines   45-48   kHalf / kFirstQtr / kThirdQtr / kModelRescale
//     lines   69-86   struct BitReader
//     lines  127-160  class ArithmeticDecoder
//     lines  214-246  class AdaptiveDecModel
//     lines  280-289  template<class M> decode_uvar_m
//     lines  294-298  get_uvar
//     lines 4055-4061 bwt_mtf_emit_rank
//     lines 4098-4122 bwt_arith_decode
//     lines 4474-4560 bwt_backend_decode
//     lines 3945-3960 brotli_lw_decode_exact
//     lines 3972-3995 brotli_lw_decode_stream
//
// The ONLY edits to the copies are:
//   * [PROBE] two `std::chrono::steady_clock` reads inside `bwt_backend_decode`,
//     one pair around the postcoder branch and one pair around the libsais call;
//   * [PROBE] the three postcoder branches that this probe never selects
//     (postcoder 0 static-MTF, 3 raw-stream, 4 QLFC) throw instead of calling
//     `decode_stream` / `bwt_mtf_expand` / `bwt_qlfc_decode`.  The shipped encoder
//     never emits those ids on the canonical route (postcoder 2, arithmetic
//     order-1, is the canonical winner; ids 0 and 4 are rejected up front by
//     `bwt_backend_encode`), so the branches are unreachable in this measurement.
//     The probe ASSERTS the postcoder id it sees and fails loudly otherwise.
//
// The vendored `third_party/libsais/src/libsais.c` is NOT copied: the probe links
// the same static library target (`anvil_libsais`) that the `anvil` binary links,
// so the inverse-BWT code executed here is byte-identical to the shipped one.
//
// The probe NEVER measures anything locally in the repository sense: it is built and
// run only inside the GitHub Actions job
// `.github/workflows/anvil-i11-postcoder-share.yml`.
//
// Build (Linux, matching the `anvil` target's flags):
//   clang++-18 -std=c++20 -O3 -DNDEBUG -I third_party/libsais/include \
//       prototypes/i11-postcoder-share/stage_probe.cpp \
//       third_party/libsais/src/libsais.c -lbrotlidec -lbrotlicommon -o stage_probe
// =====================================================================================

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <brotli/decode.h>
#include <libsais.h>

using Clock = std::chrono::steady_clock;

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 45-48
// =====================================================================================
static constexpr uint32_t kHalf = 0x80000000u;
static constexpr uint32_t kFirstQtr = 0x40000000u;
static constexpr uint32_t kThirdQtr = 0xC0000000u;
static constexpr uint32_t kModelRescale = 32768;

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 69-86 (struct BitReader)
// =====================================================================================
struct BitReader {
    const uint8_t* p;
    size_t n;
    size_t byte = 0;
    uint64_t acc = 0;   // buffered byte (I9 leg 4: bit-at-a-time load removed)
    int have = 0;       // bits still available in acc
    uint32_t bit() {
        if (have == 0) {
            if (byte >= n) return 0; // arithmetic decoder pads with zeros
            acc = p[byte++];
            have = 8;
        }
        --have;
        return static_cast<uint32_t>(acc >> have) & 1u;
    }
    // Number of payload bytes touched (a partially-consumed buffered byte counts).
    size_t consumed_bytes() const { return byte; }
};

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 127-160 (class ArithmeticDecoder)
// =====================================================================================
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
    size_t consumed_bytes() const { return br_.consumed_bytes(); }
    uint32_t scaled(uint32_t total) const {
        uint64_t range = static_cast<uint64_t>(high_) - low_ + 1;
        return static_cast<uint32_t>(((static_cast<uint64_t>(code_ - low_) + 1) * total - 1) / range);
    }
    void consume(uint32_t cum_lo, uint32_t cum_hi, uint32_t total) {
        uint64_t range = static_cast<uint64_t>(high_) - low_ + 1;
        high_ = low_ + static_cast<uint32_t>((static_cast<uint64_t>(range) * cum_hi) / total - 1);
        low_  = low_ + static_cast<uint32_t>((static_cast<uint64_t>(range) * cum_lo) / total);
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

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 214-246 (class AdaptiveDecModel)
// =====================================================================================
class AdaptiveDecModel {
    static constexpr uint32_t kBlkShift = 4;              // 16 symbols/block
    static constexpr uint32_t kBlkSize = 1u << kBlkShift;
    uint32_t alphabet_;
    std::vector<uint16_t> freq_;
    std::vector<uint32_t> blk_;
    uint32_t total_ = 0;
    void rebuild() {
        total_ = 0;
        std::fill(blk_.begin(), blk_.end(), 0);
        for (uint32_t i = 0; i < alphabet_; ++i) { total_ += freq_[i]; blk_[i >> kBlkShift] += freq_[i]; }
    }
    void update(uint32_t sym) {
        if (total_ >= kModelRescale) {
            for (auto& f : freq_) f = static_cast<uint16_t>(std::max<uint16_t>(1, (f + 1) >> 1));
            rebuild();
        }
        ++freq_[sym]; ++total_; ++blk_[sym >> kBlkShift];
    }
public:
    explicit AdaptiveDecModel(uint32_t alphabet = 256) : alphabet_(alphabet), freq_(alphabet, 1), blk_((alphabet + kBlkSize - 1) >> kBlkShift, 0) { rebuild(); }
    uint32_t decode(ArithmeticDecoder& ad) {
        uint32_t target = ad.scaled(total_);
        uint32_t b = 0, sum = 0;
        // blk_ sums to total_ > target, so this terminates inside blk_.
        while (sum + blk_[b] <= target) { sum += blk_[b]; ++b; }
        uint32_t s = b << kBlkShift;
        while (sum + freq_[s] <= target) { sum += freq_[s]; ++s; }
        if (s >= alphabet_) throw std::runtime_error("arithmetic symbol out of range");
        uint32_t lo = sum, hi = lo + freq_[s];
        ad.consume(lo, hi, total_); update(s); return s;
    }
};

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 280-289 (decode_uvar_m)
// =====================================================================================
template<class M> static uint64_t decode_uvar_m(ArithmeticDecoder& ad, M& m) {
    uint64_t x=0; int shift=0;
    for (int i=0;i<10;++i) {
        uint8_t b=static_cast<uint8_t>(m.decode(ad));
        x |= static_cast<uint64_t>(b & 0x7Fu) << shift;
        if (!(b&0x80u)) return x;
        shift += 7;
    }
    throw std::runtime_error("varint overflow");
}

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 294-298 (get_uvar)
// =====================================================================================
static uint64_t get_uvar(const uint8_t*& p, const uint8_t* e) {
    uint64_t x=0; int shift=0;
    for(int i=0;i<10;++i) { if(p>=e) throw std::runtime_error("truncated varint"); uint8_t b=*p++; x|=uint64_t(b&0x7f)<<shift; if(!(b&0x80))return x; shift+=7; }
    throw std::runtime_error("varint overflow");
}

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 481-482 (get_u32le)
// =====================================================================================
static uint32_t get_u32le(const uint8_t*& p,const uint8_t* e){ if(e-p<4)throw std::runtime_error("truncated u32"); uint32_t x=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24); p+=4; return x; }

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp line 4022 (kBwtAuxMaxIndexes)
// =====================================================================================
static constexpr uint64_t kBwtAuxMaxIndexes = 1u << 20;

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 4055-4061 (bwt_mtf_emit_rank)
// =====================================================================================
static void bwt_mtf_emit_rank(std::array<uint8_t,256>& sym,uint8_t rank,std::vector<uint8_t>& out) {
    uint8_t b=sym[rank]; out.push_back(b);
    if(rank) {
        for(uint32_t j=rank;j>0;--j) sym[j]=sym[j-1];
        sym[0]=b;
    }
}

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 4098-4122 (bwt_arith_decode)
// =====================================================================================
static std::vector<uint8_t> bwt_arith_decode(const uint8_t* p,size_t n,size_t expected,bool order1,uint64_t bit_count) {
    if(bit_count==0 || bit_count>uint64_t(n)*8) throw std::runtime_error("bad BWT arithmetic bit count");
    uint64_t need=(bit_count+7)/8; if(need!=n) throw std::runtime_error("BWT arithmetic byte count mismatch");
    if((bit_count&7) && n) {
        uint32_t pad=8-static_cast<uint32_t>(bit_count&7); uint8_t mask=static_cast<uint8_t>((1u<<pad)-1u);
        if(p[n-1]&mask) throw std::runtime_error("nonzero BWT arithmetic padding");
    }
    ArithmeticDecoder ad(p,n); AdaptiveDecModel tok0(256), runm(256);
    std::array<std::unique_ptr<AdaptiveDecModel>,257> tok1;
    auto model1=[&](uint32_t ctx)->AdaptiveDecModel& {
        if(!tok1[ctx])tok1[ctx]=std::make_unique<AdaptiveDecModel>(256); return *tok1[ctx]; };
    std::array<uint8_t,256> sym{}; for(uint32_t i=0;i<256;++i)sym[i]=static_cast<uint8_t>(i);
    std::vector<uint8_t> out; out.reserve(expected); uint32_t prev=256;
    while(out.size()<expected) {
        uint8_t t=static_cast<uint8_t>(order1?model1(prev).decode(ad):tok0.decode(ad)); prev=t;
        if(t==0) {
            uint64_t rv=decode_uvar_m(ad,runm);
            if(rv>=expected-out.size()) throw std::runtime_error("BWT arithmetic zero run exceeds output");
            size_t len=static_cast<size_t>(rv)+1; out.insert(out.end(),len,sym[0]);
        } else bwt_mtf_emit_rank(sym,t,out);
    }
    // Arithmetic termination may leave a bounded suffix of termination bits;
    // bit_count fixes the exact payload byte extent and zero-padding above, so
    // there can be no hidden trailing bytes or attacker-driven allocation.
    return out;
}

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 3945-3960 (brotli_lw_decode_exact)
// =====================================================================================
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

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 3972-3995 (brotli_lw_decode_stream)
// The shipped version takes a `DecodeSink*`; the probe supplies its own sink with
// the same contract (`put(bytes,n)`), so the decode loop is byte-for-byte the
// shipped one.  ANVIL's DecodeSink also folds a CRC-32 over the bytes; the probe
// sink folds an identical-order FNV-1a instead (the CRC is a correctness check,
// not a timing cost, and the probe's output is verified by SHA-256 outside).
// =====================================================================================
struct ProbeSink {
    enum class Mode { Discard, Collect };
    Mode mode = Mode::Collect;
    std::vector<uint8_t> out;
    uint64_t fnv = 1469598103934665603ull;
    uint64_t written = 0;
    void put(const uint8_t* p, size_t n) {
        written += n;
        for (size_t i = 0; i < n; ++i) {
            fnv ^= p[i];
            fnv *= 1099511628211ull;
        }
        if (mode == Mode::Collect) out.insert(out.end(), p, p + n);
    }
};

static void brotli_lw_decode_stream(const uint8_t* p,size_t n,size_t expected,ProbeSink* sink) {
    if(expected>(384ull<<20)) throw std::runtime_error("ratio transformed stream too large");
    BrotliDecoderState* s=BrotliDecoderCreateInstance(nullptr,nullptr,nullptr);
    if(!s) throw std::runtime_error("brotli decoder allocation failed");
    struct Guard { BrotliDecoderState* p; ~Guard(){ BrotliDecoderDestroyInstance(p); } } guard{s};
    if(!BrotliDecoderSetParameter(s,BROTLI_DECODER_PARAM_LARGE_WINDOW,1))
        throw std::runtime_error("brotli large-window decoder setup failed");
    std::vector<uint8_t> win(256u<<10);
    size_t ai=n, ao=win.size(), total_out=0;
    const uint8_t* ni=p; uint8_t* no=win.data();
    for(;;) {
        BrotliDecoderResult r=BrotliDecoderDecompressStream(s,&ai,&ni,&ao,&no,&total_out);
        const size_t produced=static_cast<size_t>(no-win.data());
        if(produced) sink->put(win.data(),produced);
        if(r==BROTLI_DECODER_RESULT_SUCCESS) break;
        if(r==BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) { no=win.data(); ao=win.size(); continue; }
        if(r==BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
            if(ai==0) throw std::runtime_error("brotli ratio payload decode mismatch"); // truncated
            continue;
        }
        throw std::runtime_error("brotli ratio payload decode mismatch");
    }
    if(ai!=0 || total_out!=expected) throw std::runtime_error("brotli ratio payload decode mismatch");
}

// =====================================================================================
// [PROBE] stage clocks
// =====================================================================================
struct StageAccum {
    uint64_t postcoder_ns = 0;   // postcoder branch of bwt_backend_decode
    uint64_t inverse_ns = 0;     // libsais inverse-BWT call
    uint64_t count_post = 0;
    uint64_t count_inv = 0;
    void reset() { postcoder_ns = inverse_ns = count_post = count_inv = 0; }
};
static StageAccum g_stage;

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 4474-4560 (bwt_backend_decode)
// with the two [PROBE] clock reads added.
// =====================================================================================
static constexpr uint8_t kBwtPostStaticMtf = 0;
static constexpr uint8_t kBwtPostArithO0   = 1;
static constexpr uint8_t kBwtPostArithO1   = 2;
static constexpr uint8_t kBwtPostRawStream = 3;
static constexpr uint8_t kBwtPostQlfc      = 4; // QLFC-like local frequency
static constexpr uint8_t kBwtAuxTag = 0xFE;

static std::vector<uint8_t> bwt_backend_decode(const uint8_t* p,size_t n,size_t expected,std::vector<uint8_t>* in_owner) {
    if(expected==0 || expected>static_cast<size_t>(std::numeric_limits<int32_t>::max())) throw std::runtime_error("bad BWT output size");
    const uint8_t* e=p+n;
    const uint8_t* const payload_end=e; // one past this backend payload

    bool use_aux=false;
    uint8_t post=0;
    int32_t primary=0;
    int32_t aux_rate=0;
    std::vector<int32_t> aux_indexes;

    if(*p==kBwtAuxTag) {
        use_aux=true; ++p;
        if(p>=e) throw std::runtime_error("truncated BWT auxiliary postcoder");
        post=*p++;
        if(expected<=1) throw std::runtime_error("BWT auxiliary payload invalid for tiny output");
        uint64_t rv=get_uvar(p,e);
        uint64_t icount=get_uvar(p,e);
        if(rv<2 || rv>static_cast<uint64_t>(expected) ||
           rv>static_cast<uint64_t>(std::numeric_limits<int32_t>::max()) || (rv&(rv-1))!=0)
            throw std::runtime_error("bad BWT auxiliary sampling rate");
        const uint64_t want=1+(static_cast<uint64_t>(expected)-1)/rv;
        if(icount==0 || icount!=want || icount>kBwtAuxMaxIndexes || icount>expected)
            throw std::runtime_error("bad BWT auxiliary index count");
        if(icount>static_cast<uint64_t>(e-p)/4u) throw std::runtime_error("truncated BWT auxiliary index array");
        aux_indexes.resize(static_cast<size_t>(icount));
        for(size_t i=0;i<aux_indexes.size();++i) {
            uint32_t idx=get_u32le(p,e);
            if(idx<1 || idx>expected) throw std::runtime_error("bad BWT auxiliary index");
            aux_indexes[i]=static_cast<int32_t>(idx);
        }
        aux_rate=static_cast<int32_t>(rv);
        primary=aux_indexes[0];
    } else {
        post=*p++;
        // libsais primary is 1-based in [1,n]; n is a VALID index (do not remap).
        uint64_t pv=get_uvar(p,e);
        if(pv<1 || pv>expected) throw std::runtime_error("bad BWT primary index");
        primary=static_cast<int32_t>(pv);
    }

    std::vector<uint8_t> bwt;
    // [PROBE] stage clock: start of the postcoder stage
    const auto t_post0 = Clock::now();
    if(post==kBwtPostStaticMtf) {
        throw std::runtime_error("probe: postcoder 0 (static MTF) is not measured by this probe");
    } else if(post==kBwtPostArithO0 || post==kBwtPostArithO1) {
        uint64_t nbits=get_uvar(p,e); bwt=bwt_arith_decode(p,static_cast<size_t>(e-p),expected,post==kBwtPostArithO1,nbits); p=e;
    } else if(post==kBwtPostRawStream) {
        throw std::runtime_error("probe: postcoder 3 (raw stream) is not measured by this probe");
    } else if(post==kBwtPostQlfc) {
        throw std::runtime_error("probe: postcoder 4 (QLFC) is not measured by this probe");
    } else throw std::runtime_error("unknown BWT postcoder id");
    // [PROBE] stage clock: end of the postcoder stage
    const auto t_post1 = Clock::now();
    g_stage.postcoder_ns += static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t_post1 - t_post0).count());
    ++g_stage.count_post;
    if(bwt.size()!=expected) throw std::runtime_error("BWT string size mismatch");

    // I20-W4 lever L1 (release the compressed input): the block payload has
    // been fully consumed, so the compressed input that held it is dead. When
    // this payload ends at the end of that buffer (the final block of the
    // file) the whole buffer is released HERE, before the 4n libsais work
    // array is allocated: the decode peak drops by the compressed size c at
    // exactly zero wire cost.
    if(in_owner&&payload_end==in_owner->data()+in_owner->size()) {
        std::vector<uint8_t>().swap(*in_owner); in_owner=nullptr;
    }

    // I20-W4 lever L2 (in-place inverse BWT): the libsais LF walk reads only
    // its own PSI array and the bigram bucket tables -- never the BWT string
    // T -- so the reconstructed string can be written over T itself. That
    // removes a whole n-byte buffer from the decode working set at exactly
    // zero wire cost. libsais terminates with U[n-1] = T[0]; because T and U
    // are restrict-qualified a compiler may reload T[0] after the U writes, so
    // the first byte is captured before the call and re-applied after it. The
    // result is byte-identical to the separate-buffer path under any
    // reordering the compiler chooses.
    const uint8_t bwt_first=bwt[0];
    std::vector<int32_t> tmp(expected+1);
    // [PROBE] stage clock: start of the inverse BWT
    const auto t_inv0 = Clock::now();
    int32_t rc=use_aux
        ? libsais_unbwt_aux(bwt.data(),bwt.data(),tmp.data(),static_cast<int32_t>(expected),nullptr,aux_rate,aux_indexes.data())
        : libsais_unbwt(bwt.data(),bwt.data(),tmp.data(),static_cast<int32_t>(expected),nullptr,primary);
    // [PROBE] stage clock: end of the inverse BWT
    const auto t_inv1 = Clock::now();
    g_stage.inverse_ns += static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t_inv1 - t_inv0).count());
    ++g_stage.count_inv;
    if(rc!=0) throw std::runtime_error("libsais inverse BWT failed");
    bwt[expected-1]=bwt_first;
    return bwt;
}

// =====================================================================================
// VERBATIM COPY: src/anvil.cpp lines 4636-4683, the BWT branch of
// `ratio_backend_decode` (sub-block framing + the bare-payload path).
// =====================================================================================
static std::vector<uint8_t> ratio_backend_decode_bwt(const uint8_t* p,size_t n,size_t expected,std::vector<uint8_t>* in_owner) {
    const uint8_t* e=p+n;
    // Subblock framing uses a 0xFF tag as its first byte; a bare inner payload
    // starts with a legacy postcoder id in 0..4 or auxiliary-v2 tag 0xFE,
    // so the outer tag cannot collide.
    if(p<e && *p==0xFF) {
        ++p; uint64_t nsub=get_uvar(p,e);
        if(nsub==0 || nsub>expected || nsub>static_cast<uint64_t>(e-p)/2u)
            throw std::runtime_error("bad BWT subblock count");
        std::vector<uint8_t> out;
        out.reserve(expected);
        size_t written=0;
        for(uint64_t i=0;i<nsub;++i) {
            uint64_t dlen=get_uvar(p,e); uint64_t plen=get_uvar(p,e);
            if(dlen==0 || dlen>expected-written)
                throw std::runtime_error("bad BWT subblock decoded length");
            if(plen>uint64_t(e-p)) throw std::runtime_error("truncated BWT subblock");
            const uint8_t* const sub_end=p+static_cast<size_t>(plen);
            if(i+1==nsub && sub_end!=e) throw std::runtime_error("BWT subblock trailing bytes");
            auto sub=bwt_backend_decode(p,static_cast<size_t>(plen),static_cast<size_t>(dlen),in_owner);
            p=sub_end;
            if(sub.size()!=static_cast<size_t>(dlen)) throw std::runtime_error("BWT subblock size mismatch");
            out.insert(out.end(),sub.begin(),sub.end());
            written+=static_cast<size_t>(dlen);
        }
        if(written!=expected) throw std::runtime_error("BWT subblock reconstruction size mismatch");
        return out;
    }
    return bwt_backend_decode(p,n,expected,in_owner);
}

// =====================================================================================
// Container walk (the mode-17 ratio-envelope decode path of `decompress()`,
// src/anvil.cpp lines 5062-5140, reduced to what this probe measures).
// The per-block CRC-32 check is NOT replicated here; the probe's reconstructed
// output is verified by SHA-256 in the driver tool, which is strictly stronger.
// =====================================================================================
struct BlockInfo {
    uint64_t decoded_len = 0;
    uint8_t mode = 0;
    uint64_t payload_len = 0;
    uint8_t transform = 0;
    uint8_t backend = 0;
    uint64_t transformed_size = 0;
    size_t payload_off = 0;
};

static std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open input: " + path);
    f.seekg(0, std::ios::end); auto n = f.tellg(); f.seekg(0);
    std::vector<uint8_t> d(static_cast<size_t>(n));
    if (n > 0) f.read(reinterpret_cast<char*>(d.data()), n);
    return d;
}

static void write_file(const std::string& path, const std::vector<uint8_t>& d) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open output: " + path);
    if (!d.empty()) f.write(reinterpret_cast<const char*>(d.data()), static_cast<std::streamsize>(d.size()));
}

// Decode the whole container.  Mirrors `decompress()`'s serial path for the
// mode-17 blocks: the final block's payload owns the compressed input buffer
// (I20-W4 lever L1), so the buffer is released inside `bwt_backend_decode`.
static std::vector<uint8_t> decode_container(std::vector<uint8_t> in,
                                             std::vector<BlockInfo>* blocks_out) {
    if (in.size() < 5 || std::memcmp(in.data(), "ANV0", 4) != 0 || (in[4] != 1 && in[4] != 2))
        throw std::runtime_error("not a supported ANVIL revision");
    const uint8_t revision = in[4];
    const uint8_t* p = in.data() + 5;
    const uint8_t* e = in.data() + in.size();
    uint64_t block_size = get_uvar(p, e);
    const uint64_t max_block = revision == 1 ? (64ull << 20) : (128ull << 20);
    if (block_size == 0 || block_size > max_block) throw std::runtime_error("invalid block size");
    uint64_t total = get_uvar(p, e);
    std::vector<uint8_t> out;
    out.reserve(static_cast<size_t>(total));
    uint64_t produced = 0;
    std::vector<BlockInfo> blocks;
    while (produced < total) {
        uint64_t blen = get_uvar(p, e);
        if (blen == 0 || blen > block_size) throw std::runtime_error("invalid block length");
        if (p >= e) throw std::runtime_error("truncated block header");
        uint8_t mode = *p++;
        uint64_t plen = get_uvar(p, e);
        p += 4;  // per-block CRC-32 (not replicated in the probe)
        if (plen > uint64_t(e - p)) throw std::runtime_error("truncated block payload");
        if (blen > total - produced) throw std::runtime_error("block exceeds declared output");
        BlockInfo bi;
        bi.decoded_len = blen; bi.mode = mode; bi.payload_len = plen;
        bi.payload_off = static_cast<size_t>(p - in.data());
        if (mode != 17) throw std::runtime_error("probe: only mode-17 ratio blocks are measured");
        const uint8_t* q = p;
        const uint8_t* qe = p + plen;
        if (plen < 3) throw std::runtime_error("truncated ratio payload");
        bi.transform = *q++;
        bi.backend = *q++;
        uint64_t xlen = get_uvar(q, qe);
        bi.transformed_size = xlen;
        if (bi.transform != 0) throw std::runtime_error("probe: only ratio transform 0 is measured");
        if (bi.backend != 2) throw std::runtime_error("probe: only the BWT backend (id 2) is measured");
        const uint8_t* backend_payload = q;
        size_t backend_len = static_cast<size_t>(qe - q);
        const bool final_block = (produced + blen == total);
        std::vector<uint8_t>* in_owner = nullptr;
        if (final_block) {
            if (p + plen != e) throw std::runtime_error("trailing bytes after final block");
            in_owner = &in;
        }
        auto b = ratio_backend_decode_bwt(backend_payload, backend_len,
                                          static_cast<size_t>(xlen), in_owner);
        if (b.size() != static_cast<size_t>(blen))
            throw std::runtime_error("ratio direct size mismatch");
        out.insert(out.end(), b.begin(), b.end());
        produced += blen;
        blocks.push_back(bi);
        if (final_block) break;
        p += plen;
    }
    if (out.size() != static_cast<size_t>(total)) throw std::runtime_error("container size mismatch");
    if (blocks_out) *blocks_out = blocks;
    return out;
}

// =====================================================================================
// small statistics helpers (median, bootstrap CI of a median, percentile bootstrap)
// =====================================================================================
static double median_of(std::vector<double> v) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    if (n % 2) return v[n / 2];
    return 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

static std::pair<double, double> bootstrap_ci_median(const std::vector<double>& v,
                                                      unsigned seed, int samples) {
    if (v.size() < 2) return {0.0, 0.0};
    std::mt19937 rng(seed);
    std::vector<double> boots;
    boots.reserve(samples);
    for (int i = 0; i < samples; ++i) {
        std::vector<double> s(v.size());
        for (size_t j = 0; j < v.size(); ++j) s[j] = v[rng() % v.size()];
        boots.push_back(median_of(s));
    }
    std::sort(boots.begin(), boots.end());
    size_t lo = static_cast<size_t>(0.025 * samples);
    size_t hi = static_cast<size_t>(0.975 * samples);
    if (hi >= boots.size()) hi = boots.size() - 1;
    return {boots[lo], boots[hi]};
}

static std::string json_escape(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; o += c; }
        else if (c == '\n') o += "\\n";
        else if (c == '\r') o += "\\r";
        else if (c == '\t') o += "\\t";
        else o += c;
    }
    return o;
}

static std::string fmt(double v, int prec = 9) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", prec, v);
    return std::string(buf);
}

// =====================================================================================
// main
// =====================================================================================
static void usage() {
    std::cerr << "usage:\n"
              << "  stage_probe bwt --container <f> --reps N [--warmups W] [--out-file <f>] [--out <json>]\n"
              << "  stage_probe brotli --in <f> --expected N --mode oneshot|stream [--sink file|discard]\n"
              << "                 [--out-file <f>] [--reps N] [--warmups W] [--out <json>]\n";
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) { usage(); return 2; }
        const std::string mode = argv[1];
        std::string container, in_path, out_file, out_json;
        std::string variant = "oneshot", sink_mode = "discard";
        long long expected = -1;
        int reps = 9, warmups = 2;
        for (int i = 2; i < argc; ++i) {
            const std::string a = argv[i];
            auto need = [&](const char* name) -> std::string {
                if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + name);
                return std::string(argv[++i]);
            };
            if (a == "--container") container = need("--container");
            else if (a == "--in") in_path = need("--in");
            else if (a == "--out-file") out_file = need("--out-file");
            else if (a == "--out") out_json = need("--out");
            else if (a == "--mode") variant = need("--mode");
            else if (a == "--sink") sink_mode = need("--sink");
            else if (a == "--expected") expected = std::stoll(need("--expected"));
            else if (a == "--reps") reps = std::stoi(need("--reps"));
            else if (a == "--warmups") warmups = std::stoi(need("--warmups"));
            else throw std::runtime_error("unknown argument: " + a);
        }
        if (reps < 1 || warmups < 0) throw std::runtime_error("bad reps/warmups");

        std::ostringstream js;

        if (mode == "bwt") {
            if (container.empty()) throw std::runtime_error("--container is required");
            struct Sample { uint64_t post = 0, inv = 0, total = 0; };
            // Every round runs the IDENTICAL decode twice (an A/A pair) so that the
            // in-process timing series carries its own null: the ratio of the two
            // halves' total decode times must span 1.0, otherwise the series is
            // TIMING_BLOCKED and counts for nothing (shared evidence contract 6).
            std::vector<Sample> samples;      // one entry per half (A and B)
            std::vector<double> null_ratios;  // B_total / A_total per round
            std::vector<BlockInfo> blocks;
            std::vector<uint8_t> last_out;
            const int total_rounds = warmups + reps;
            for (int r = 0; r < total_rounds; ++r) {
                Sample halves[2];
                for (int half = 0; half < 2; ++half) {
                    std::vector<uint8_t> in = read_file(container);
                    g_stage.reset();
                    const auto t0 = Clock::now();
                    std::vector<BlockInfo> bl;
                    auto out = decode_container(std::move(in), &bl);
                    const auto t1 = Clock::now();
                    halves[half].post = g_stage.postcoder_ns;
                    halves[half].inv = g_stage.inverse_ns;
                    halves[half].total = static_cast<uint64_t>(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
                    last_out = std::move(out);
                    blocks = bl;
                }
                if (r < warmups) continue;
                // Alternating order (A first on even rounds, B first on odd rounds),
                // as in tools/paired_bench.py.
                const Sample& A = ((r % 2) == 0) ? halves[0] : halves[1];
                const Sample& B = ((r % 2) == 0) ? halves[1] : halves[0];
                samples.push_back(A);
                samples.push_back(B);
                if (A.total > 0)
                    null_ratios.push_back(static_cast<double>(B.total) / static_cast<double>(A.total));
            }
            std::vector<double> post_s, inv_s, tot_s, share_sum, share_tot;
            for (const auto& s : samples) {
                post_s.push_back(static_cast<double>(s.post));
                inv_s.push_back(static_cast<double>(s.inv));
                tot_s.push_back(static_cast<double>(s.total));
                const double sum = static_cast<double>(s.post + s.inv);
                share_sum.push_back(sum > 0 ? static_cast<double>(s.post) / sum : 0.0);
                share_tot.push_back(s.total > 0 ? static_cast<double>(s.post) / static_cast<double>(s.total) : 0.0);
            }
            const double m_post = median_of(post_s);
            const double m_inv = median_of(inv_s);
            const double m_tot = median_of(tot_s);
            auto ci_post = bootstrap_ci_median(post_s, 41246u, 20000);
            auto ci_inv = bootstrap_ci_median(inv_s, 41246u, 20000);
            auto ci_tot = bootstrap_ci_median(tot_s, 41246u, 20000);
            auto ci_share_sum = bootstrap_ci_median(share_sum, 41246u, 20000);
            auto ci_share_tot = bootstrap_ci_median(share_tot, 41246u, 20000);
            double null_pt = 0.0;
            if (!null_ratios.empty()) {
                std::vector<double> nr;
                for (double v : null_ratios) if (v > 0) nr.push_back(v);
                null_pt = median_of(nr);
            }
            auto ci_null = bootstrap_ci_median(null_ratios, 41246u, 20000);
            const bool null_spans = !null_ratios.empty() && ci_null.first <= 1.0 && ci_null.second >= 1.0;

            js << "{\n";
            js << "  \"schema\": 1,\n";
            js << "  \"mode\": \"bwt\",\n";
            js << "  \"container\": \"" << json_escape(container) << "\",\n";
            js << "  \"warmups\": " << warmups << ",\n";
            js << "  \"reps\": " << reps << ",\n";
            js << "  \"block_count\": " << blocks.size() << ",\n";
            js << "  \"blocks\": [";
            for (size_t i = 0; i < blocks.size(); ++i) {
                if (i) js << ", ";
                js << "{\"decoded_len\": " << blocks[i].decoded_len
                   << ", \"transform\": " << static_cast<int>(blocks[i].transform)
                   << ", \"backend\": " << static_cast<int>(blocks[i].backend)
                   << ", \"transformed_size\": " << blocks[i].transformed_size << "}";
            }
            js << "],\n";
            js << "  \"postcoder\": {\"median_ns\": " << fmt(m_post)
               << ", \"ci95_ns\": [" << fmt(ci_post.first) << ", " << fmt(ci_post.second) << "]"
               << ", \"min_ns\": " << fmt(*std::min_element(post_s.begin(), post_s.end()))
               << ", \"max_ns\": " << fmt(*std::max_element(post_s.begin(), post_s.end())) << "},\n";
            js << "  \"inverse_bwt\": {\"median_ns\": " << fmt(m_inv)
               << ", \"ci95_ns\": [" << fmt(ci_inv.first) << ", " << fmt(ci_inv.second) << "]"
               << ", \"min_ns\": " << fmt(*std::min_element(inv_s.begin(), inv_s.end()))
               << ", \"max_ns\": " << fmt(*std::max_element(inv_s.begin(), inv_s.end())) << "},\n";
            js << "  \"total\": {\"median_ns\": " << fmt(m_tot)
               << ", \"ci95_ns\": [" << fmt(ci_tot.first) << ", " << fmt(ci_tot.second) << "]"
               << ", \"min_ns\": " << fmt(*std::min_element(tot_s.begin(), tot_s.end()))
               << ", \"max_ns\": " << fmt(*std::max_element(tot_s.begin(), tot_s.end())) << "},\n";
            js << "  \"share_vs_stage_sum\": {\"median\": " << fmt(median_of(share_sum), 6)
               << ", \"ci95\": [" << fmt(ci_share_sum.first, 6) << ", " << fmt(ci_share_sum.second, 6) << "]},\n";
            js << "  \"share_vs_probe_total\": {\"median\": " << fmt(median_of(share_tot), 6)
               << ", \"ci95\": [" << fmt(ci_share_tot.first, 6) << ", " << fmt(ci_share_tot.second, 6) << "]},\n";
            js << "  \"null\": {\"ratio_b_over_a\": " << fmt(null_pt, 6)
               << ", \"ci95\": [" << fmt(ci_null.first, 6) << ", " << fmt(ci_null.second, 6) << "]"
               << ", \"spans_one\": " << (null_spans ? "true" : "false") << "},\n";
            js << "  \"samples_postcoder_ns\": [";
            for (size_t i = 0; i < post_s.size(); ++i) { if (i) js << ", "; js << fmt(post_s[i], 0); }
            js << "],\n";
            js << "  \"samples_inverse_ns\": [";
            for (size_t i = 0; i < inv_s.size(); ++i) { if (i) js << ", "; js << fmt(inv_s[i], 0); }
            js << "],\n";
            js << "  \"samples_total_ns\": [";
            for (size_t i = 0; i < tot_s.size(); ++i) { if (i) js << ", "; js << fmt(tot_s[i], 0); }
            js << "]\n";
            if (!out_file.empty()) write_file(out_file, last_out);
            js << "}\n";
        } else if (mode == "brotli") {
            if (in_path.empty() || expected < 0) throw std::runtime_error("--in and --expected are required");
            const auto in = read_file(in_path);
            std::vector<double> times;
            std::vector<uint8_t> last_out;
            for (int r = 0; r < warmups + reps; ++r) {
                ProbeSink sink;
                sink.mode = (sink_mode == "collect") ? ProbeSink::Mode::Collect : ProbeSink::Mode::Discard;
                const auto t0 = Clock::now();
                if (variant == "oneshot") {
                    auto out = brotli_lw_decode_exact(in.data(), in.size(), static_cast<size_t>(expected));
                    if (sink_mode == "collect") { sink.put(out.data(), out.size()); last_out = std::move(out); }
                } else if (variant == "stream") {
                    brotli_lw_decode_stream(in.data(), in.size(), static_cast<size_t>(expected), &sink);
                    if (sink_mode == "collect") last_out = std::move(sink.out);
                } else {
                    throw std::runtime_error("bad --mode: " + variant);
                }
                const auto t1 = Clock::now();
                if (r >= warmups)
                    times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() / 1e9);
            }
            auto ci = bootstrap_ci_median(times, 41246u, 20000);
            js << "{\n";
            js << "  \"schema\": 1,\n";
            js << "  \"mode\": \"brotli\",\n";
            js << "  \"variant\": \"" << variant << "\",\n";
            js << "  \"sink\": \"" << sink_mode << "\",\n";
            js << "  \"input_bytes\": " << in.size() << ",\n";
            js << "  \"expected_bytes\": " << expected << ",\n";
            js << "  \"warmups\": " << warmups << ",\n";
            js << "  \"reps\": " << reps << ",\n";
            js << "  \"median_s\": " << fmt(median_of(times)) << ",\n";
            js << "  \"ci95_s\": [" << fmt(ci.first) << ", " << fmt(ci.second) << "],\n";
            js << "  \"min_s\": " << fmt(*std::min_element(times.begin(), times.end())) << ",\n";
            js << "  \"max_s\": " << fmt(*std::max_element(times.begin(), times.end())) << ",\n";
            js << "  \"samples_s\": [";
            for (size_t i = 0; i < times.size(); ++i) { if (i) js << ", "; js << fmt(times[i]); }
            js << "]\n";
            js << "}\n";
            if (!out_file.empty() && sink_mode == "collect") write_file(out_file, last_out);
        } else {
            usage();
            return 2;
        }

        const std::string text = js.str();
        if (!out_json.empty()) {
            std::ofstream f(out_json, std::ios::binary);
            if (!f) throw std::runtime_error("cannot open json output: " + out_json);
            f << text;
        }
        std::cout << text;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "stage_probe: " << ex.what() << "\n";
        return 1;
    }
}
