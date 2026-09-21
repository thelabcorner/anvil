// ============================================================================
// cost_oracle.cpp — S6-3 MEASURED-COST CANDIDATE-REJECTION ORACLE (PROTOTYPE)
// swarm anvil-i7-cost / task u2 / lane: prototypes/cost_oracle/ (standalone)
//
// Pre-registration context (docs/swarm-i6-strategy.md §5 S6-3):
//   EXP. F proved measured-rANS-cost parsing is the best ratio mechanism but
//   DP-class slow. EXP. L proved measured per-stream cost is 100% faithful
//   (lambda=0.01). EXP. X failed because a fixed local heuristic
//   (`1.5+varint(len-4)+varint(dist-1)+len/8+0.18*log2(dist+1)`) over-committed
//   1,599 short far candidates on anvil_bench.exe whose REAL coded cost was
//   higher. THIS prototype: after ONE greedy/sparse parse, build the real
//   rANS streams, measure each committed candidate's actual coded size by
//   per-symbol byte attribution, and roll back ONLY the worst over-committed
//   candidates (measured keep-cost > measured drop-cost) in a SINGLE pass.
//   Explicitly NOT EXP. F: no iterative DP re-parse, greedy-class encode.
//
// Provenance: MatchFinder, scan_candidate, find_sparse/_at/_pnra_at,
//   parse_sparse, the rANS/context/Huffman/defexc coders, encode_stream/
//   decode_stream and encode/decode_tokens_{sparse,tcopy} are copied VERBATIM
//   from src/anvil.cpp (read-only reuse; NO src edits — arch owns that file).
//   New code (marked NEW below): per-symbol measured-cost attribution, the
//   single rejection pass, calibration statistics, harness timing/framing.
//
// Build (Experiment V standalone precedent):
//   clang-cl /std:c++20 /MD /O2 /EHsc /DNDEBUG cost_oracle.cpp /Fe:cost_oracle.exe
//
// Harness-relative labeling (binding): all ratio/throughput numbers here come
// from THIS harness's own encode/decode of the token streams with production
// stream codecs. They are NOT comparable to anvil.exe absolutes (no full
// container router, single parse family per run); they measure the ORACLE's
// delta against the SAME harness without the oracle pass.
// ============================================================================

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// ---- verbatim helpers (src/anvil.cpp lines 207-227, 1848-1849) -------------
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

static void append_varint_bytes(std::vector<uint8_t>& out,uint64_t x){do{uint8_t b=static_cast<uint8_t>(x&0x7f);x>>=7;if(x)b|=0x80;out.push_back(b);}while(x);}
static uint64_t read_varint_bytes(const std::vector<uint8_t>&v,size_t&pos){uint64_t x=0;int sh=0;for(int i=0;i<10;++i){if(pos>=v.size())throw std::runtime_error("stream varint truncated");uint8_t b=v[pos++];x|=uint64_t(b&0x7f)<<sh;if(!(b&0x80))return x;sh+=7;}throw std::runtime_error("stream varint overflow");}

// NEW: varint byte-length (for cost accounting only)
static uint32_t varint_len(uint64_t x){ uint32_t n=1; while(x>=128){x>>=7;++n;} return n; }
// verbatim (src/anvil.cpp line 544): parse-time varint cost estimate
static double varint_cost(uint64_t x) {
    int bytes=1; while(x>=128){x>>=7;++bytes;}
    return 3.5 + 5.25*bytes; // adaptive byte model tends to beat raw 8-bit bytes
}

static constexpr uint32_t kHashBits = 18;
static constexpr uint32_t kHashSize = 1u << kHashBits;
static constexpr uint32_t kNoPos = 0xFFFFFFFFu;

// ---- verbatim SPARSE-REF structures (src/anvil.cpp lines 229-275) ----------
struct Match { uint32_t len, dist; };
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
};

static inline uint32_t hash4(const uint8_t* p) {
    uint32_t x; std::memcpy(&x,p,4);
    return (x * 0x9E3779B1u) >> (32-kHashBits);
}

static uint32_t Matchlength(const uint8_t* a, const uint8_t* b, uint32_t maxlen) {
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

// ---- verbatim MatchFinder (src/anvil.cpp lines 276-505) --------------------
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
    uint32_t max_Match;
    static std::vector<Match> walk(const uint8_t* d, size_t n, uint32_t pos, uint32_t q,
                                    const std::vector<uint32_t>& prev, uint32_t max_chain, uint32_t max_match);
public:
    MatchFinder(const std::vector<uint8_t>& d, uint32_t max_chain, uint32_t max_match, bool boundary=false)
      : d_(d), head_(kHashSize,kNoPos), prev_(d.size(),kNoPos), bhead_(kHashSize,kNoPos),
        bprev_(d.size(),kNoPos), use_boundary_(boundary), max_chain_(max_chain), max_Match(max_match) {}
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
        std::vector<Match> out;
        if (use_boundary_) {
            uint32_t h = hash4(d_.data() + pos);
            out = walk(d_.data(), d_.size(), pos, bhead_[h], bprev_, max_chain_, max_Match);
        }
        uint32_t h = hash4(d_.data() + pos);
        std::vector<Match> full = walk(d_.data(), d_.size(), pos, head_[h], prev_, max_chain_, max_Match);
        if (out.empty()) return full;
        out.insert(out.end(), full.begin(), full.end());
        if (out.size() > 8) {
            std::sort(out.begin(), out.end(), [](auto&a, auto&b){ if(a.len!=b.len)return a.len>b.len; return a.dist<b.dist; });
            out.resize(8);
        }
        return out;
    }
    void scan_candidate(uint32_t pos, uint32_t q, const std::array<double,256>& litcost,
                        double avg_lit, uint64_t& work, double dead_band, bool tcopy,
                        uint32_t& blen, uint32_t& bk, std::array<uint32_t,kSparseScanMax>& boff,
                        std::array<uint8_t,kSparseScanMax>& bval, double& best_score,
                        std::array<uint32_t,kSparseScanMax/4>& btfo, uint32_t& btfo_n,
                        uint32_t max_len=0, bool allow_tfo_only=false, uint32_t min_len=8) const {
        const uint8_t* tgt = d_.data() + pos;
        const uint8_t* src = d_.data() + q;
        uint32_t remain = static_cast<uint32_t>(d_.size() - pos);
        uint32_t cap = std::min({remain, max_Match, kSparseScanMax});
        if (max_len) cap = std::min(cap, max_len);
        if (cap < min_len) return;
        const double Matchgain = avg_lit - 0.125;
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
                score += Matchgain;
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
        if (btfo_n == 0) return false;
        if (bk <= btfo_n && blen >= 4) {
            out.len = blen; out.dist = pos - q;
            out.off.assign(boff.begin(), boff.begin() + bk);
            out.val.assign(bval.begin(), bval.begin() + bk);
            out.tfo.assign(btfo.begin(), btfo.begin() + btfo_n);
            return true;
        }
        return false;
    }

    bool find_sparse(uint32_t pos, SparseMatch& out, const std::array<double,256>& litcost,
                     double avg_lit, uint64_t& work, double dead_band=32.0, bool tcopy=false) const {
        out.len = 0;
        if (pos + 4 > d_.size()) return false;
        const uint8_t* tgt = d_.data() + pos;
        uint32_t h = hash4(tgt);
        uint32_t q = head_[h];
        uint32_t remain = static_cast<uint32_t>(d_.size() - pos);
        uint32_t cap = std::min({remain, max_Match, kSparseScanMax});
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
// walk is defined out-of-class below (verbatim body; Match declared above).
std::vector<Match> MatchFinder::walk(const uint8_t* d, size_t n, uint32_t pos, uint32_t q,
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
        uint32_t l = Matchlength(d + q, d + pos, cap);
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

// ---- verbatim parse_sparse + diagnostics globals (src/anvil.cpp 602-915) ---
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
    uint32_t last_span = 0;
    uint32_t phase = 0;          // next expected record-boundary position (0 = unknown)
    bool took_ch = false;
    auto note_span = [&](uint32_t dist, uint32_t len) {
        if (dist > 0 && dist <= 16384) { last_span = dist; phase = i + len; }
    };
    auto note_ch_emit = [&](uint32_t dist) {
        ++g_ch_chosen;
        if (last_span > 4 && dist >= last_span - 4 && dist <= last_span + 4) ++g_ch_span_emit;
    };
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
        SparseMatch ch_sm; bool has_ch=false; double ch_cost=0.0;
        bool ch_span=false;
        bool at_phase = (phase != 0 && i + 2 >= phase && i <= phase + 2);
        if (phase != 0 && i > phase + 2) { phase = 0; }
        bool discovery = (phase == 0 && i < 65536 && nchan == 0);
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
            if (has_ch && ch_sm.len >= ch_sm.dist && ch_sm.len <= ch_sm.dist + 8) {
                for (size_t c = 0; c < nchan; ++c)
                    if (chan[c].dist == ch_sm.dist && chan[c].score >= 96.0) { ch_span = true; break; }
            }
        }
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

// ---- verbatim precision/work-adaptive entropy (src/anvil.cpp 928-994) ------
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

// NEW: rANS encode with per-symbol renorm-byte attribution. Bytes flushed at
// the step that pushes symbol ii are charged to ii (the encoder walks symbols
// in reverse; each renorm flush belongs to exactly one push). Sum(attr)+4 ==
// payload size is asserted by the caller-side accounting check.
static std::vector<uint8_t> rans_encode_attr(const std::vector<uint8_t>& src,const RansModel&m,const RansSpec&sp,std::vector<double>& attr) {
    attr.assign(src.size(),0.0);
    if(src.empty())return {};
    uint32_t x=sp.L; std::vector<uint8_t> emitted; emitted.reserve(src.size()/2+16);
    for(size_t ii=src.size();ii-->0;) {
        uint8_t sym=src[ii]; uint32_t f=m.freq[sym], st=m.start[sym];
        uint32_t x_max=((sp.L>>sp.scale_bits)<<8)*f;
        uint32_t rb=0;
        while(x>=x_max){emitted.push_back(static_cast<uint8_t>(x));x>>=8;++rb;}
        attr[ii]=double(rb);
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

// ---- verbatim context-switched literal coder (src/anvil.cpp 1002-1136) -----
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
    std::array<double, kCtxK> cnorm{};
    for (uint8_t g = 0; g < kCtxK && g < seeds.size(); ++g) {
        double n2 = 0; for (int s = 0; s < 256; ++s) n2 += double(cnt[seeds[g]][s]) * cnt[seeds[g]][s];
        double inv = n2 > 0 ? 1.0 / std::sqrt(n2) : 0;
        for (int s = 0; s < 256; ++s) { centroid[g][s] = double(cnt[seeds[g]][s]) * inv; cnorm[g] += centroid[g][s] * centroid[g][s]; }
        cnorm[g] = std::sqrt(cnorm[g]);
    }
    auto assign = [&]() {
        for (int p = 0; p < 256; ++p) {
            if (prev_tot[p] == 0) { group[p] = 0; continue; }
            double n2 = 0; for (int s = 0; s < 256; ++s) n2 += double(cnt[p][s]) * cnt[p][s];
            double inv = 1.0 / std::sqrt(n2);
            double best = 1e300; uint8_t bg = 0;
            for (uint8_t g = 0; g < kCtxK; ++g) {
                if (cnorm[g] <= 0) continue;
                double ct = 0; for (int s = 0; s < 256; ++s) ct += double(cnt[p][s]) * inv * centroid[g][s];
                double d = 1.0 - ct;
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
    std::array<uint8_t, kCtxK> renum{};
    uint8_t keff = 0;
    std::array<std::array<uint32_t,256>, kCtxK> gcount{};
    for (int g = 0; g < kCtxK; ++g) {
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

// NEW: ctx-rANS encode with per-symbol renorm-byte attribution (same rule as
// rans_encode_attr; the context of symbol ii is decoded from src[ii-1], which
// the encoder can read directly since it walks the source in reverse).
static std::vector<uint8_t> ctx_rans_encode_attr(const std::vector<uint8_t>& src, const CtxModel& cm, const RansSpec& sp, std::vector<double>& attr) {
    attr.assign(src.size(), 0.0);
    if (src.empty()) return {};
    uint32_t x = sp.L; std::vector<uint8_t> emitted; emitted.reserve(src.size()/2 + 16);
    uint8_t prev = 0;
    for (size_t ii = src.size(); ii-- > 0;) {
        uint8_t sym = src[ii];
        uint8_t ctx = ii > 0 ? cm.map[src[ii-1]] : cm.map[prev];
        const RansModel& m = cm.m[ctx];
        uint32_t f = m.freq[sym], st = m.start[sym];
        uint32_t x_max = ((sp.L >> sp.scale_bits) << 8) * f;
        uint32_t rb = 0;
        while (x >= x_max) { emitted.push_back(static_cast<uint8_t>(x)); x >>= 8; ++rb; }
        attr[ii] = double(rb);
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

// ---- verbatim canonical Huffman + defexc (src/anvil.cpp 1139-1249) ---------
static std::vector<uint8_t> huffman_encode(const std::vector<uint8_t>& src, const std::array<uint8_t,256>& len) {
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
    std::array<uint16_t,256> first_code{};
    std::array<uint16_t,256> first_sym{};
    std::array<uint16_t,256> n_codes{};
    std::array<uint8_t,256> order{};
    uint8_t max_len=0, n_ord=0;
    std::array<uint16_t,4096> tbl{};
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
        for(size_t i=0;i<out_n;++i){
            refill(12);
            int win = have; if (win > 12) win = 12;
            uint32_t code=(uint32_t)((acc>>(have-win)) & ((1u<<win)-1));
            uint32_t idx = code << (12 - win);
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

// ---- verbatim stream-suite selection (src/anvil.cpp 1256-1367) -------------
static double g_stream_lambda = 0.01;
static double g_stream_mu = 0.0, g_stream_nu = 0.0;
static bool g_stream_suite = true;   // false = fixed rANS-4096 + raw (pre-suite behavior)
static bool g_stream_ctx = true;      // true = context-switched rANS (mode 6) enabled in the suite
static bool g_stream_log = false;    // --stream-log: record per-stream selection
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
        if(g_stream_ctx && src.size() >= 4096) add(ctx_stream_bytes(src,kRans4096), 45.0);
    }
    const Cand* best=&cands[0];
    for(auto& c:cands) if(c.J<best->J) best=&c;
    if(g_stream_suite && cands.size()>1) {
        size_t lw=0; for(size_t i=1;i<cands.size();++i) if(cands[i].bytes.size()<cands[lw].bytes.size()) lw=i;
        ++g_j_total;
        if(best->bytes.size() <= cands[lw].bytes.size()*101/100) ++g_j_agree;
        if(g_stream_log) g_stream_log_entries.push_back({uint32_t(best-&cands[0]+1), uint32_t(lw), best->bytes.size(), cands[lw].bytes.size()});
    }
    return best->bytes;
}

// ============================================================================
// NEW: measured-cost per-symbol attribution.
// build_stream_attributed runs the EXACT encode_stream candidate set and
// J-selection (byte-identical winner, asserted by --self-test against
// encode_stream), then attributes the winner's actual output bytes to input
// symbols:
//   raw     : 1 byte/symbol
//   rANS    : renorm bytes flushed at that symbol's push (+4-byte final state)
//   huffman : code length / 8 bytes per symbol (exact bit accounting)
//   defexc  : default -> 1/8 mask byte; exception -> 1 + 1/8 bytes
//   ctx-rANS: renorm bytes under the symbol's context model (+4-byte state)
// `fixed` = bytes.size() - sum(per_sym): headers, model tables, final state,
// padding — charged pro-rata by symbol count in keep-cost accounting.
// Accounting identity sum(per_sym)+fixed == bytes.size() is asserted.
// ============================================================================
struct StreamAttrib {
    std::vector<uint8_t> bytes;
    std::vector<double> per_sym;
    double fixed = 0.0;
};

static void attrib_check(const StreamAttrib& sa, const char* what) {
    double s=0; for(double v:sa.per_sym) s+=v;
    double total = s + sa.fixed;
    if (std::fabs(total - double(sa.bytes.size())) > 1e-6 || sa.fixed < -1e-9) {
        std::fprintf(stderr,"ATTR DBG %s: bytes=%zu nsym=%zu sum=%.6f fixed=%.6f total=%.6f first_mode=%u\n",
            what,sa.bytes.size(),sa.per_sym.size(),s,sa.fixed,total,(unsigned)(sa.bytes.empty()?255:sa.bytes[0]));
        throw std::runtime_error(std::string("attribution accounting mismatch in ") + what);
    }
}

static StreamAttrib build_stream_attributed(const std::vector<uint8_t>& src) {
    StreamAttrib sa;
    // candidate construction mirrors encode_stream EXACTLY (same order/J/tie-break)
    struct Cand {
        std::vector<uint8_t> bytes; double J; uint8_t codec;
        const RansSpec* spec=nullptr; RansModel model{};
        CtxModel cm{};
        std::array<uint8_t,256> hlen{};
        uint8_t def=0;
    };
    std::vector<Cand> cands;
    auto Jof=[&](const std::vector<uint8_t>& b, double cu){ return double(b.size()) + g_stream_lambda*cu + g_stream_mu*2.0 + g_stream_nu*cu; };
    {
        Cand c; c.codec=0;
        c.bytes.push_back(0); put_uvar(c.bytes,src.size()); c.bytes.insert(c.bytes.end(),src.begin(),src.end());
        c.J=Jof(c.bytes,10.0); cands.push_back(std::move(c));
    }
    if(src.size()>=16) {
        auto mk_rans=[&](const RansSpec& sp, uint8_t mode, double cu){
            Cand c; c.codec=mode; c.spec=&sp; c.model=build_rans_model(src,sp.tot);
            auto rd=rans_encode(src,c.model,sp);
            std::vector<uint8_t>& z=c.bytes; z.push_back(mode); put_uvar(z,src.size());
            uint32_t nz=0; for(auto f:c.model.freq) if(f) ++nz; put_uvar(z,nz);
            for(int i=0;i<256;++i) if(c.model.freq[i]) { z.push_back(uint8_t(i)); put_uvar(z,c.model.freq[i]); }
            put_uvar(z,rd.size()); z.insert(z.end(),rd.begin(),rd.end());
            c.J=Jof(z,cu); cands.push_back(std::move(c));
        };
        mk_rans(kRans4096,1,40.0);
        if(g_stream_suite) {
            mk_rans(kRans512,2,35.0);
            mk_rans(kRans256,3,30.0);
            {
                Cand c; c.codec=4; c.hlen=huffman_lengths(src);
                std::vector<uint8_t>& z=c.bytes; z.push_back(4); put_uvar(z,src.size());
                for(int i=0;i<256;++i) z.push_back(c.hlen[i]);
                auto bits=huffman_encode(src,c.hlen); put_uvar(z,bits.size()); z.insert(z.end(),bits.begin(),bits.end());
                c.J=Jof(z,22.0); cands.push_back(std::move(c));
            }
            {
                std::array<uint32_t,256> cnt{}; for(uint8_t b:src) ++cnt[b];
                uint8_t def=0; for(int i=1;i<256;++i) if(cnt[i]>cnt[def]) def=uint8_t(i);
                if(cnt[def]>=src.size()/2) {
                    Cand c; c.codec=5; c.def=def;
                    std::vector<uint8_t>& z=c.bytes; z.push_back(5); put_uvar(z,src.size()); z.push_back(def);
                    std::vector<uint8_t> mask((src.size()+7)/8, 0); std::vector<uint8_t> vals;
                    for(size_t i=0;i<src.size();++i) if(src[i]!=def){ mask[i>>3]|=uint8_t(1u<<(i&7)); vals.push_back(src[i]); }
                    put_uvar(z,vals.size()); z.insert(z.end(),mask.begin(),mask.end()); z.insert(z.end(),vals.begin(),vals.end());
                    c.J=Jof(z,20.0); cands.push_back(std::move(c));
                }
            }
            if(g_stream_ctx && src.size() >= 4096) {
                Cand c; c.codec=6; c.cm=build_ctx_model(src,kRans4096.tot);
                auto rd=ctx_rans_encode(src,c.cm,kRans4096);
                std::vector<uint8_t>& z=c.bytes; z.push_back(6); put_uvar(z,src.size());
                z.push_back(c.cm.K);
                for (int i = 0; i < 256; ++i) z.push_back(c.cm.map[i]);
                for (uint8_t g = 0; g < c.cm.K; ++g) {
                    uint32_t nz = 0; for (auto f : c.cm.m[g].freq) if (f) ++nz;
                    put_uvar(z, nz);
                    for (int i = 0; i < 256; ++i) if (c.cm.m[g].freq[i]) { z.push_back(uint8_t(i)); put_uvar(z, c.cm.m[g].freq[i]); }
                }
                put_uvar(z, rd.size()); z.insert(z.end(), rd.begin(), rd.end());
                c.J=Jof(z,45.0); cands.push_back(std::move(c));
            }
        }
    }
    const Cand* best=&cands[0];
    for(auto& c:cands) if(c.J<best->J) best=&c;
    sa.bytes=best->bytes;
    const size_t n=src.size();
    switch(best->codec) {
    case 0: { // raw
        sa.per_sym.assign(n,1.0);
        sa.fixed = double(sa.bytes.size()-n);
        break;
    }
    case 1: case 2: case 3: { // rANS
        sa.per_sym.assign(n,0.0);
        auto rd=rans_encode_attr(src,best->model,*best->spec,sa.per_sym);
        double s=0; for(double v:sa.per_sym) s+=v;
        if (s+4.0 != double(rd.size())) throw std::runtime_error("rANS attribution != payload");
        sa.fixed = double(sa.bytes.size()) - s; // headers + 4-byte final state
        break;
    }
    case 4: { // huffman: exact bit accounting
        sa.per_sym.assign(n,0.0);
        uint64_t total_bits=0;
        for(size_t i=0;i<n;++i){ sa.per_sym[i]=double(best->hlen[src[i]])/8.0; total_bits+=best->hlen[src[i]]; }
        double s=0; for(double v:sa.per_sym) s+=v;
        sa.fixed = double(sa.bytes.size()) - s;
        uint64_t dn=(total_bits+7)/8;
        double expect_fixed = double(1+varint_len(n)+256+varint_len(dn)) + (double(dn)-double(total_bits)/8.0);
        if (std::fabs(sa.fixed-expect_fixed)>1e-6) throw std::runtime_error("huffman attribution header mismatch");
        break;
    }
    case 5: { // defexc
        sa.per_sym.assign(n,0.0);
        uint64_t nexc=0;
        for(size_t i=0;i<n;++i){ if(src[i]!=best->def){ sa.per_sym[i]=1.125; ++nexc; } else sa.per_sym[i]=0.125; }
        double s=0; for(double v:sa.per_sym) s+=v;
        sa.fixed = double(sa.bytes.size()) - s;
        double expect_fixed = double(1+varint_len(n)+1+varint_len(nexc)) + (double((n+7)/8) - double(n)/8.0); // + mask padding
        if (std::fabs(sa.fixed-expect_fixed)>1e-6) throw std::runtime_error("defexc attribution header mismatch");
        break;
    }
    case 6: { // ctx-rANS
        sa.per_sym.assign(n,0.0);
        auto rd=ctx_rans_encode_attr(src,best->cm,kRans4096,sa.per_sym);
        double s=0; for(double v:sa.per_sym) s+=v;
        if (s+4.0 != double(rd.size())) throw std::runtime_error("ctx attribution != payload");
        sa.fixed = double(sa.bytes.size()) - s; // headers + 4-byte final state
        break;
    }
    default: throw std::runtime_error("unknown codec in attribution");
    }
    attrib_check(sa,"stream");
    return sa;
}

// ---- verbatim RLZ/RePair stream constants (src/anvil.cpp 1386-1391) --------
static constexpr uint32_t kRlzReapMaxRules = 768;   // RePair rule cap (bounds symbol ids 256..256+R-1)
static constexpr uint32_t kRlzWindow          = 64u*1024; // RLZ self-reference window
static constexpr uint32_t kRlzMinMatch        = 4;        // min RLZ match length

// ---- verbatim decode_stream (src/anvil.cpp 1486-1587) ----------------------
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
        auto body=decode_stream(q,qe,max_n);
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
        for(size_t i=0;i<R;++i){
            auto app=[&](uint32_t s,std::vector<uint8_t>&v){ if(s<256){v.push_back(static_cast<uint8_t>(s));} else { if((s-256)>=i) throw std::runtime_error("reap rule forward ref"); v.insert(v.end(),exp[s-256].begin(),exp[s-256].end());} };
            app(L[i],exp[i]); app(Rt[i],exp[i]);
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
        auto body=decode_stream(q,qe,max_n);
        if(q!=qe) throw std::runtime_error("rlz inner trailing bytes");
        p=qe;
        const uint8_t* bp=body.data(); const uint8_t* be=body.data()+body.size();
        uint64_t nops=get_uvar(bp,be);
        std::vector<uint8_t> out; out.reserve(static_cast<size_t>(raw_n));
        for(uint64_t k=0;k<nops;++k){
            if(bp>=be) throw std::runtime_error("truncated rlz op");
            uint8_t op=*bp++;
            if(op==0){ uint64_t len=get_uvar(bp,be); if(len>uint64_t(be-bp)||len>raw_n-out.size()) throw std::runtime_error("bad rlz literal"); out.insert(out.end(),bp,bp+len); bp+=len; }
            else if(op==1){ uint64_t dist=get_uvar(bp,be)+1, len=get_uvar(bp,be)+kRlzMinMatch; if(dist>out.size()||len>raw_n-out.size()) throw std::runtime_error("bad rlz match"); for(uint64_t t=0;t<len;++t) out.push_back(out[out.size()-dist]); }
            else throw std::runtime_error("bad rlz op type");
        }
        if(out.size()!=raw_n) throw std::runtime_error("rlz output-size mismatch");
        return out;
    }
    throw std::runtime_error("unknown stream codec");
}

// ---- verbatim SPARSE backend codec (src/anvil.cpp 1888-1989) ---------------
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

// ---- verbatim TCOPY backend codec (src/anvil.cpp 1998-2170) ----------------
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

// ============================================================================
// NEW: S6-3 measured-cost candidate-rejection oracle (the prototype's core)
//
// One pass, greedy-class cost, explicitly NOT EXP. F's iterative DP:
//   1. build the REAL token streams (types/ll/ml/ds/lits/masks/resid/tmask)
//      exactly as encode_tokens_{sparse,tcopy} would;
//   2. run the production per-stream codec suite (J = L + 0.01*C_decode) with
//      per-symbol byte attribution -> each committed candidate's MEASURED
//      coded cost (keep);
//   3. compute the measured cost of the alternative (drop): the covered span
//      re-emitted as literals under the block's measured order-0 literal /
//      ll / types models (add-1/2 smoothing);
//   4. reject iff keep > drop*(1+theta)  [theta=0 default], splice rejected
//      spans back as literal runs, re-encode ONCE with the untouched
//      production encoders (wire produced by verbatim code).
// Rollback is always decoder-safe: output bytes are identical either way
// (literals reproduce the same file bytes), so later tokens' back-references
// remain valid.
// ============================================================================

// stream ids: 0 types, 1 ll, 2 ml, 3 ds, 4 lits, 5 masks, 6 resid, 7 tmask
struct TokenStreams {
    std::vector<uint8_t> s[8];
    std::vector<std::array<uint32_t,8>> beg, en; // per-token symbol ranges
};

// Mirrors encode_tokens_tcopy's stream construction exactly, recording ranges.
static void build_token_streams(const std::vector<uint8_t>& d, const std::vector<SparseToken>& toks, TokenStreams& ts) {
    ts = TokenStreams{};
    ts.beg.reserve(toks.size()); ts.en.reserve(toks.size());
    std::array<uint32_t,(kSparseScanMax+31)/32> words{};
    std::array<uint32_t,(kSparseScanMax+31)/128> twords{};
    for(const auto& t : toks) {
        std::array<uint32_t,8> bg{}, ed{};
        for(int k=0;k<8;++k) bg[k]=ed[k]=uint32_t(ts.s[k].size());
        ts.s[0].push_back(t.type); ed[0]=uint32_t(ts.s[0].size());
        if(t.type==0) {
            append_varint_bytes(ts.s[1],t.len-1); ed[1]=uint32_t(ts.s[1].size());
            ts.s[4].insert(ts.s[4].end(),d.begin()+t.pos,d.begin()+t.pos+t.len); ed[4]=uint32_t(ts.s[4].size());
        } else if(t.type==1) {
            append_varint_bytes(ts.s[2],t.len-4); ed[2]=uint32_t(ts.s[2].size());
            append_varint_bytes(ts.s[3],t.dist-1); ed[3]=uint32_t(ts.s[3].size());
        } else { // 2 or 3
            append_varint_bytes(ts.s[2],t.len-4); ed[2]=uint32_t(ts.s[2].size());
            append_varint_bytes(ts.s[3],t.dist-1); ed[3]=uint32_t(ts.s[3].size());
            std::fill(words.begin(),words.end(),0u);
            for(size_t k=0;k<t.off.size();++k) {
                words[t.off[k]/32]|=(1u<<(t.off[k]%32));
                ts.s[6].push_back(t.val[k]);
            }
            uint32_t nwords=(t.len+31)/32;
            for(uint32_t w=0;w<nwords;++w) {
                uint32_t m=words[w];
                ts.s[5].push_back(static_cast<uint8_t>(m));
                ts.s[5].push_back(static_cast<uint8_t>(m>>8));
                ts.s[5].push_back(static_cast<uint8_t>(m>>16));
                ts.s[5].push_back(static_cast<uint8_t>(m>>24));
            }
            ed[5]=uint32_t(ts.s[5].size()); ed[6]=uint32_t(ts.s[6].size());
            if(t.type==3) {
                std::fill(twords.begin(),twords.end(),0u);
                for(size_t k=0;k<t.tfo.size();++k) twords[t.tfo[k]/32]|=(1u<<(t.tfo[k]%32));
                uint32_t nwin=t.len/4;
                uint32_t twn=(nwin+31)/32;
                for(uint32_t w=0;w<twn;++w) {
                    uint32_t m=twords[w];
                    ts.s[7].push_back(static_cast<uint8_t>(m));
                    ts.s[7].push_back(static_cast<uint8_t>(m>>8));
                    ts.s[7].push_back(static_cast<uint8_t>(m>>16));
                    ts.s[7].push_back(static_cast<uint8_t>(m>>24));
                }
                ed[7]=uint32_t(ts.s[7].size());
            }
        }
        ts.beg.push_back(bg); ts.en.push_back(ed);
    }
}

// Measured order-0 model with add-1/2 smoothing (drop-side counterfactual).
struct O0Model {
    std::array<uint32_t,256> cnt{};
    uint64_t n=0;
    void add(uint8_t b){ ++cnt[b]; ++n; }
    double cost(uint8_t b) const { return -std::log2((double(cnt[b])+0.5)/(double(n)+128.0)); }
};

struct CalibRec {
    uint8_t type; uint32_t len, dist;
    double pred_tok, true_tok, pred_alt, true_alt;
    uint8_t flip; // heuristic committed (pred<alt) but measurement says drop (keep>alt)
};

struct OracleStats {
    uint64_t considered=0, rejected=0, rej_t1=0, rej_t2=0, rej_t3=0, rej_shortfar=0;
    double sum_gain_rejected=0;   // (drop-keep) over R1; negative = over-commit magnitude
    double sum_flip_overcommit=0; // (keep-drop) over sign-flips: measured recoverable bytes
    uint64_t won0=0, won1=0, won2=0; // blocks won by keep-all / drop-R1 / drop-R1|R2
};

// The single measurement pass. Fills per-token rejection-class masks:
//   r1: measured keep > measured drop*(1+theta)   (individually over-committed
//       vs the order-0 literal counterfactual; rare on real data)
//   r2: type==3 && len<=8                          (EXP. X framing class, the
//       ledger's stated root cause: a <=8-byte transformed phrase saves at most
//       8 raw bytes but pays full per-token framing — type+ml+ds+mask+tmask
//       words — which the local heuristic underprices; STATED SHAPE FORMULA,
//       not tuned per file)
// The DECISION is made end-to-end by the caller: encode the block with
// {nothing, r1, r1|r2} removed and keep the smallest MEASURED payload — the
// wire itself arbitrates, so model-interaction externalities (which per-token
// margins cannot see, and which are exactly why EXP. X regressed in aggregate
// while every candidate looked locally cheap) are priced truthfully.
struct OracleMasks { std::vector<char> r1, r2; };

static void run_oracle(const std::vector<uint8_t>& d,
                       const std::vector<SparseToken>& toks,
                       double theta, size_t max_sample,
                       OracleStats* st, std::vector<CalibRec>* calib,
                       OracleMasks& masks) {
    const size_t T=toks.size();
    masks.r1.assign(T,0); masks.r2.assign(T,0);
    TokenStreams ts;
    build_token_streams(d,toks,ts);
    StreamAttrib sa[8];
    double symcount[8];
    for(int k=0;k<8;++k){ sa[k]=build_stream_attributed(ts.s[k]); symcount[k]=double(ts.s[k].size()); }
    O0Model m_lit, m_ll, m_ty;
    for(auto b:ts.s[4]) m_lit.add(b);
    for(auto b:ts.s[1]) m_ll.add(b);
    for(auto b:ts.s[0]) m_ty.add(b);
    // parse-time cost context (mirrors parse_sparse's own arrays; calibration only)
    const uint32_t n=static_cast<uint32_t>(d.size());
    std::array<double,256> litcost{};
    if(n){
        std::array<uint32_t,256> hist{};
        for(auto b:d) ++hist[b];
        for(int b=0;b<256;++b){ double p=(hist[b]+0.5)/(double(n)+128.0); litcost[b]=std::clamp(-std::log2(p),1.0,9.5); }
    }
    std::vector<double> pref(n+1,0.0);
    for(uint32_t i=0;i<n;++i) pref[i+1]=pref[i]+litcost[d[i]]+0.10;

    size_t matches=0; for(auto&t:toks) if(t.type!=0) ++matches;
    size_t stride = (max_sample && matches>max_sample) ? (matches+max_sample-1)/max_sample : 1;
    size_t midx=0;

    for(size_t t=0;t<T;++t) {
        const auto& tok=toks[t];
        if(tok.type==0) continue;
        ++midx;
        if(st) ++st->considered;
        // keep: measured attributed bytes + pro-rata share of each touched stream's fixed cost
        double keep=0.0;
        for(int k=0;k<8;++k) {
            uint32_t b=ts.beg[t][k], e=ts.en[t][k];
            if(e<=b) continue;
            for(uint32_t j=b;j<e;++j) keep+=sa[k].per_sym[j];
            keep += sa[k].fixed * double(e-b) / symcount[k];
        }
        // drop: span re-emitted as literals under the measured models
        double drop = m_ty.cost(0);
        { uint64_t v=tok.len-1; do { drop += m_ll.cost(uint8_t(v&0x7f)); v>>=7; } while(v); }
        for(uint32_t b=tok.pos;b<tok.pos+tok.len;++b) drop += m_lit.cost(d[b]);

        if(calib && ((midx-1)%stride)==0) {
            double pred_tok;
            if(tok.type==1) pred_tok = 0.6+varint_cost(tok.len-4)+varint_cost(tok.dist-1)+0.18*std::log2(double(tok.dist)+1.0);
            else {
                pred_tok = 1.5+varint_cost(tok.len-4)+varint_cost(tok.dist-1)+double(tok.len)/8.0
                         +0.18*std::log2(double(tok.dist)+1.0);
                for(size_t k=0;k<tok.val.size();++k) pred_tok += litcost[tok.val[k]];
            }
            double pred_alt = pref[tok.pos+tok.len]-pref[tok.pos];
            CalibRec r{tok.type,tok.len,tok.dist,pred_tok,keep,pred_alt,drop,
                       uint8_t((pred_tok<pred_alt && keep>drop)?1:0)};
            calib->push_back(r);
        }
        if(keep > drop*(1.0+theta)) {
            masks.r1[t]=1;
            if(st){
                ++st->rejected;
                if(tok.type==1) ++st->rej_t1; else if(tok.type==2) ++st->rej_t2; else ++st->rej_t3;
                st->sum_gain_rejected += (drop-keep);
            }
        }
        if(st && tok.type==3 && tok.len<=8) {
            ++st->rej_shortfar; // class-R2 population (EXP. X shape), decided by measured trial below
            st->sum_flip_overcommit += (keep>drop)?(keep-drop):0.0;
        }
    }
}

// Splice masked-out match tokens back as literal runs (merge adjacent).
static std::vector<SparseToken> apply_masks(const std::vector<SparseToken>& toks, const std::vector<char>& rej) {
    std::vector<SparseToken> out; out.reserve(toks.size());
    for(size_t t=0;t<toks.size();++t) {
        if(rej[t]) {
            if(!out.empty() && out.back().type==0 && out.back().pos+out.back().len==toks[t].pos) out.back().len+=toks[t].len;
            else out.push_back(SparseToken{0,toks[t].pos,toks[t].len,0,{},{}});
        } else out.push_back(toks[t]);
    }
    return out;
}

// ============================================================================
// NEW: harness (framing mirrors production compress(); single parse family
// per run; all numbers HARNESS-RELATIVE, not anvil.exe absolutes)
// ============================================================================
struct RunOpts {
    uint32_t mode=11;          // 11 = sparse backend (types 0-2), 14 = tcopy backend (types 0-3)
    bool pnra=false;
    uint32_t block_size=256*1024;
    uint32_t max_chain=48;
    uint32_t max_match=65535;
    uint32_t surprise=12;
    bool boundary=false;
    bool channels=false;
    int reps=3;
    double theta=0.0;
    size_t max_sample=20000;
    bool force_t3=false; // DIAGNOSTIC ONLY: drop all type-3 tokens (EXP. X attribution probe)
};

static uint64_t framed_size(uint32_t blen, size_t plen) {
    if(plen+1 < blen) return varint_len(blen)+1+varint_len(plen)+4+plen;
    return varint_len(blen)+1+varint_len(blen)+4+blen; // raw-block fallback (production rule)
}

struct CalibAgg {
    size_t n=0; double sum=0, sumsq=0;
    std::vector<double> relerr; double median=0,p90=0,p99=0,mx=0,mn=0;
    uint64_t flips=0; double flip_overcommit=0;
    void add(const CalibRec& r){
        // pred_tok is in BITS (the parse-time formula's native unit); true_tok
        // in BYTES (measured attributed wire bytes). Compare in bytes.
        double e=((r.pred_tok/8.0)-r.true_tok)/r.true_tok;
        relerr.push_back(e); sum+=e; sumsq+=e*e; ++n;
        if(r.flip){ ++flips; flip_overcommit += (r.true_tok-r.true_alt); }
    }
    void finish(){
        if(!n) return;
        std::sort(relerr.begin(),relerr.end());
        median=relerr[n/2]; p90=relerr[(size_t)(0.90*double(n-1))]; p99=relerr[(size_t)(0.99*double(n-1))];
        mx=relerr.back(); mn=relerr.front();
    }
};

struct FileResult {
    std::string name;
    uint64_t raw_bytes=0;
    uint64_t base_payload=0, orc_payload=0, base_framed=0, orc_framed=0, base_cont=0, orc_cont=0;
    uint64_t tokens=0, matches=0;
    OracleStats ost;
    CalibAgg cal;
    double enc_base_ms=0, enc_orc_ms=0, dec_base_ms=0, dec_orc_ms=0;
    bool rt_base=true, rt_orc=true;
    uint64_t pnra_commits=0;
};

static double median_of(std::vector<double>& v){ std::sort(v.begin(),v.end()); return v.empty()?0.0:v[v.size()/2]; }

static FileResult process_file(const std::string& path, const RunOpts& o) {
    FileResult R;
    R.name=path;
    std::ifstream f(path,std::ios::binary);
    if(!f) throw std::runtime_error("cannot open "+path);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
    R.raw_bytes=data.size();
    const bool tcopy=(o.mode==14);
    std::vector<double> t_enc_base, t_enc_orc, t_dec_base, t_dec_orc;
    uint64_t total=data.size();
    const uint64_t pnra_before=g_pnra_commit; // global accumulates across parses; per-rep count = delta/reps
    for(int rep=0;rep<o.reps;++rep) {
        double eb=0,eo=0,db=0,doo=0;
        uint64_t fp=0,op=0,ff=0,of=0;
        uint64_t toks=0,mtch=0;
        bool rtb=true,rto=true;
        OracleStats last_ost; std::vector<CalibRec> last_cal;
        for(size_t off=0;off<data.size()||off==0;off+=o.block_size) {
            const uint32_t blen=uint32_t(std::min<uint64_t>(o.block_size,data.size()-off));
            const uint8_t* bp=data.data()+off;
            std::vector<uint8_t> block(bp,bp+blen);
            // ---- baseline: parse + encode (greedy-class reference) ----
            auto t0=std::chrono::steady_clock::now();
            auto toks_b=parse_sparse(block,o.max_chain,o.max_match,o.surprise,o.boundary,o.channels,tcopy,o.pnra);
            auto payload_b=tcopy?encode_tokens_tcopy(block,toks_b):encode_tokens_sparse(block,toks_b);
            auto t1=std::chrono::steady_clock::now();
            eb+=std::chrono::duration<double,std::milli>(t1-t0).count();
            { auto dec=tcopy?decode_tokens_tcopy(payload_b.data(),payload_b.size(),blen)
                           :decode_tokens_sparse(payload_b.data(),payload_b.size(),blen);
              if(dec!=block) rtb=false; }
            auto t2=std::chrono::steady_clock::now();
            db+=std::chrono::duration<double,std::milli>(t2-t1).count();
            fp+=payload_b.size(); ff+=framed_size(blen,payload_b.size());
            toks+=toks_b.size(); for(auto&t:toks_b) if(t.type!=0) ++mtch;
            // ---- oracle: same parse + ONE measured-cost pass + measured variant sweep ----
            auto t3=std::chrono::steady_clock::now();
            auto toks_o=parse_sparse(block,o.max_chain,o.max_match,o.surprise,o.boundary,o.channels,tcopy,o.pnra);
            OracleStats ost; std::vector<CalibRec> cal; OracleMasks masks;
            run_oracle(block,toks_o,o.theta,rep==o.reps-1?o.max_sample:0,
                       rep==o.reps-1?&ost:nullptr, rep==o.reps-1?&cal:nullptr, masks);
            if(o.force_t3) for(size_t i=0;i<toks_o.size();++i) if(toks_o[i].type==3) masks.r2[i]=1; // DIAGNOSTIC
            auto t4=std::chrono::steady_clock::now();
            // decision by MEASUREMENT: try {keep-all, drop-R1, drop-R1|R2}, keep the
            // smallest actual encoded payload (the wire arbitrates model interactions)
            bool any_r1=false, any_r2=false;
            for(size_t i=0;i<masks.r1.size();++i){ any_r1|=(masks.r1[i]!=0); any_r2|=(masks.r2[i]!=0); }
            std::vector<uint8_t> payload_o=payload_b; // variant 0 = keep-all == baseline payload
            int won=0;
            if(any_r1) {
                auto toksA=apply_masks(toks_o,masks.r1);
                auto pA=tcopy?encode_tokens_tcopy(block,toksA):encode_tokens_sparse(block,toksA);
                if(pA.size()<payload_o.size()){ payload_o=std::move(pA); won=1; }
            }
            if(any_r2) {
                std::vector<char> rb(masks.r1.size());
                for(size_t i=0;i<rb.size();++i) rb[i]=masks.r1[i]|masks.r2[i];
                auto toksB=apply_masks(toks_o,rb);
                auto pB=tcopy?encode_tokens_tcopy(block,toksB):encode_tokens_sparse(block,toksB);
                if(pB.size()<payload_o.size()){ payload_o=std::move(pB); won=2; }
            }
            auto t5=std::chrono::steady_clock::now();
            eo+=std::chrono::duration<double,std::milli>(t4-t3).count()
               +std::chrono::duration<double,std::milli>(t5-t4).count();
            { auto dec=tcopy?decode_tokens_tcopy(payload_o.data(),payload_o.size(),blen)
                           :decode_tokens_sparse(payload_o.data(),payload_o.size(),blen);
              if(dec!=block) rto=false; }
            auto t6=std::chrono::steady_clock::now();
            doo+=std::chrono::duration<double,std::milli>(t6-t5).count();
            op+=payload_o.size(); of+=framed_size(blen,payload_o.size());
            if(rep==o.reps-1){ // accumulate stats across blocks (collected once, last rep)
                last_ost.considered+=ost.considered; last_ost.rejected+=ost.rejected;
                last_ost.rej_t1+=ost.rej_t1; last_ost.rej_t2+=ost.rej_t2; last_ost.rej_t3+=ost.rej_t3;
                last_ost.rej_shortfar+=ost.rej_shortfar;
                last_ost.sum_gain_rejected+=ost.sum_gain_rejected;
                last_ost.sum_flip_overcommit+=ost.sum_flip_overcommit;
                last_ost.won0+=(won==0); last_ost.won1+=(won==1); last_ost.won2+=(won==2);
                for(auto&r:cal) last_cal.push_back(r);
            }
        }
        t_enc_base.push_back(eb); t_enc_orc.push_back(eo);
        t_dec_base.push_back(db); t_dec_orc.push_back(doo);
        R.base_payload=fp; R.orc_payload=op; R.base_framed=ff; R.orc_framed=of;
        R.tokens=toks; R.matches=mtch; R.rt_base=rtb; R.rt_orc=rto;
        R.ost=last_ost;
        for(auto&r:last_cal) R.cal.add(r);
    }
    R.pnra_commits=(g_pnra_commit-pnra_before)/(o.reps>0?uint64_t(o.reps):1);
    R.cal.finish();
    R.enc_base_ms=median_of(t_enc_base); R.enc_orc_ms=median_of(t_enc_orc);
    R.dec_base_ms=median_of(t_dec_base); R.dec_orc_ms=median_of(t_dec_orc);
    R.base_cont = 5+varint_len(o.block_size)+varint_len(total)+R.base_framed;
    R.orc_cont  = 5+varint_len(o.block_size)+varint_len(total)+R.orc_framed;
    return R;
}

static void self_test() {
    uint64_t rng=0x9E3779B97F4A7C15ull;
    auto next=[&]{ rng^=rng<<13; rng^=rng>>7; rng^=rng<<17; return rng; };
    for(int trial=0;trial<24;++trial) {
        size_t sz;
        switch(trial%4){ case 0: sz=0; break; case 1: sz=size_t(trial)*3; break;
                         case 2: sz=100+size_t(trial)*97; break; default: sz=5000+size_t(trial)*613; }
        std::vector<uint8_t> buf(sz);
        uint8_t fam=uint8_t(trial/4);
        for(size_t i=0;i<sz;++i){
            switch(fam){
            case 0: buf[i]=uint8_t("the quick brown fox jumps over the lazy dog\n"[next()%43]); break;
            case 1: buf[i]=uint8_t(next()&0xFF); break;
            case 2: buf[i]=(next()%16)?uint8_t('A'):uint8_t(next()&0xFF); break;
            default: buf[i]=uint8_t((next()%3)?0x00:0xFF); break;
            }
        }
        auto a=encode_stream(buf);
        auto b=build_stream_attributed(buf);
        if(a!=b.bytes) throw std::runtime_error("self-test: attributed bytes != encode_stream bytes");
        attrib_check(b,"self-test");
    }
}

int main(int argc,char**argv) {
    RunOpts o;
    std::vector<std::string> files;
    std::string csv, calcsv;
    std::vector<std::string> record_files={"generated.log","generated.json","generated.jsonl"};
    for(int i=1;i<argc;++i) {
        std::string a=argv[i];
        auto val=[&](const char* pre)->std::string{
            if(a.rfind(pre,0)==0) return a.substr(std::strlen(pre));
            return std::string();
        };
        if(auto v=val("--mode=");!v.empty()) o.mode=uint32_t(std::stoul(v));
        else if(auto v=val("--pnra=");!v.empty()) o.pnra=(v!="off");
        else if(auto v=val("--block=");!v.empty()) o.block_size=std::stoul(v);
        else if(auto v=val("--chain=");!v.empty()) o.max_chain=std::stoul(v);
        else if(auto v=val("--max-match=");!v.empty()) o.max_match=std::stoul(v);
        else if(auto v=val("--surprise=");!v.empty()) o.surprise=std::stoul(v);
        else if(auto v=val("--reps=");!v.empty()) o.reps=std::stoi(v);
        else if(auto v=val("--theta=");!v.empty()) o.theta=std::stod(v);
        else if(auto v=val("--max-sample=");!v.empty()) o.max_sample=std::stoull(v);
        else if(auto v=val("--csv=");!v.empty()) csv=v;
        else if(auto v=val("--cal-csv=");!v.empty()) calcsv=v;
        else if(a=="--boundary") o.boundary=true;
        else if(a=="--channels") o.channels=true;
        else if(a=="--force-t3") o.force_t3=true;
        else if(!a.empty()&&a[0]=='-') { std::fprintf(stderr,"unknown arg %s\n",a.c_str()); return 2; }
        else files.push_back(a);
    }
    if(files.empty()){ std::fprintf(stderr,"usage: cost_oracle [--mode=11|14] [--pnra=on|off] [--reps=N] [--theta=X] [--csv=f] [--cal-csv=f] <files...>\n"); return 2; }
    try {
        self_test();
        std::printf("self-test: attributed stream bytes == encode_stream bytes on 24 synthetic buffers OK\n");
    } catch(const std::exception& e) {
        std::fprintf(stderr,"SELF-TEST FAILED: %s\n",e.what());
        return 1;
    }
    std::vector<FileResult> results;
    for(auto& fpath:files) {
        try {
            auto R=process_file(fpath,o);
            results.push_back(R);
            double base_ratio=double(R.base_cont)/double(R.raw_bytes);
            double orc_ratio=double(R.orc_cont)/double(R.raw_bytes);
            double delta=100.0*(double(R.orc_cont)-double(R.base_cont))/double(R.base_cont);
            double encx=R.enc_orc_ms/R.enc_base_ms;
            double decpen=100.0*(R.dec_orc_ms-R.dec_base_ms)/R.dec_base_ms;
            std::printf("%-28s raw=%llu  base=%llu (%.5f)  oracle=%llu (%.5f)  delta=%+.4f%%  R1=%llu R2pop=%llu (t1=%llu t2=%llu t3=%llu) won0/1/2=%llu/%llu/%llu  cal n=%zu med=%+.1f%% p90=%+.1f%% flips=%llu (%.2f%%)  enc=%.2fx  dec=%+.2f%%  rt=%s/%s\n",
                R.name.c_str(),(unsigned long long)R.raw_bytes,
                (unsigned long long)R.base_cont,base_ratio,
                (unsigned long long)R.orc_cont,orc_ratio,delta,
                (unsigned long long)R.ost.rejected,(unsigned long long)R.ost.rej_shortfar,
                (unsigned long long)R.ost.rej_t1,(unsigned long long)R.ost.rej_t2,(unsigned long long)R.ost.rej_t3,
                (unsigned long long)R.ost.won0,(unsigned long long)R.ost.won1,(unsigned long long)R.ost.won2,
                R.cal.n,100.0*R.cal.median,100.0*R.cal.p90,
                (unsigned long long)R.cal.flips,R.cal.n?100.0*double(R.cal.flips)/double(R.cal.n):0.0,
                encx,decpen,R.rt_base?"OK":"FAIL",R.rt_orc?"OK":"FAIL");
            std::fflush(stdout);
        } catch(const std::exception& e) {
            std::fprintf(stderr,"ERROR on %s: %s\n",fpath.c_str(),e.what());
            return 1;
        }
    }
    // aggregate over the pre-registered record files present in the run
    double ab=0,ao=0,ar=0; bool any_rec=false;
    for(auto&R:results){
        std::string base=R.name;
        auto pos=base.find_last_of("/\\");
        if(pos!=std::string::npos) base=base.substr(pos+1);
        bool isrec=false; for(auto&r:record_files) if(base==r) isrec=true;
        if(isrec){ any_rec=true; ab+=R.base_cont; ao+=R.orc_cont; ar+=R.raw_bytes; }
    }
    if(any_rec){
        double d=100.0*(ao-ab)/ab;
        std::printf("RECORD-FILE AGGREGATE: base=%.0f oracle=%.0f raw=%.0f  ratio base=%.6f oracle=%.6f  delta=%+.4f%%  [%s] (target <= 0)\n",
            ab,ao,ar,ab/ar,ao/ar,d,d<=0.0?"PASS":"FAIL");
    }
    if(!csv.empty()){
        std::ofstream out(csv);
        out<<"file,mode,pnra,raw_B,base_B,oracle_B,delta_pct,base_ratio,oracle_ratio,tokens,matches,"
              "considered,R1,R2pop,rej_t1,rej_t2,rej_t3,won0,won1,won2,cal_n,cal_median,cal_p90,cal_p99,flips,flip_rate,"
              "enc_base_ms,enc_orc_ms,enc_x,dec_base_ms,dec_orc_ms,dec_penalty_pct,rt_base,rt_orc,pnra_commits\n";
        for(auto&R:results){
            out<<R.name<<','<<o.mode<<','<<(o.pnra?"on":"off")<<','<<R.raw_bytes<<','<<R.base_cont<<','<<R.orc_cont<<','
               <<std::fixed<<(100.0*(double(R.orc_cont)-double(R.base_cont))/double(R.base_cont))<<','
               <<double(R.base_cont)/double(R.raw_bytes)<<','<<double(R.orc_cont)/double(R.raw_bytes)<<','
               <<R.tokens<<','<<R.matches<<','<<R.ost.considered<<','<<R.ost.rejected<<','<<R.ost.rej_shortfar<<','
               <<R.ost.rej_t1<<','<<R.ost.rej_t2<<','<<R.ost.rej_t3<<','
               <<R.ost.won0<<','<<R.ost.won1<<','<<R.ost.won2<<','
               <<R.cal.n<<','<<R.cal.median<<','<<R.cal.p90<<','<<R.cal.p99<<','
               <<R.cal.flips<<','<<(R.cal.n?double(R.cal.flips)/double(R.cal.n):0.0)<<','
               <<R.enc_base_ms<<','<<R.enc_orc_ms<<','<<(R.enc_orc_ms/R.enc_base_ms)<<','
               <<R.dec_base_ms<<','<<R.dec_orc_ms<<','<<(100.0*(R.dec_orc_ms-R.dec_base_ms)/R.dec_base_ms)<<','
               <<(R.rt_base?1:0)<<','<<(R.rt_orc?1:0)<<','<<R.pnra_commits<<'\n';
        }
        std::printf("csv written: %s\n",csv.c_str());
    }
    if(!calcsv.empty()){
        std::ofstream out(calcsv);
        out<<"file,type,len,dist,pred_tok,true_tok,pred_alt,true_alt,flip\n";
        // per-file calibration samples were aggregated into CalibAgg; re-run collection for the dump
        for(auto& fpath:files){
            std::ifstream f(fpath,std::ios::binary);
            if(!f) continue;
            std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
            const bool tcopy=(o.mode==14);
            for(size_t off=0;off<data.size()||off==0;off+=o.block_size){
                const uint32_t blen=uint32_t(std::min<uint64_t>(o.block_size,data.size()-off));
                std::vector<uint8_t> block(data.data()+off,data.data()+off+blen);
                auto toks=parse_sparse(block,o.max_chain,o.max_match,o.surprise,o.boundary,o.channels,tcopy,o.pnra);
                OracleMasks masks;
                std::vector<CalibRec> cal;
                run_oracle(block,toks,o.theta,o.max_sample,nullptr,&cal,masks);
                for(auto&r:cal)
                    out<<fpath<<','<<int(r.type)<<','<<r.len<<','<<r.dist<<','
                       <<r.pred_tok<<','<<r.true_tok<<','<<r.pred_alt<<','<<r.true_alt<<','<<int(r.flip)<<'\n';
            }
        }
        std::printf("calibration csv written: %s\n",calcsv.c_str());
    }
    return 0;
}
