// ===========================================================================
// I20 C5 — postcoder decode-cost swap prototype (ANVIL Iteration 20).
//
// PURPOSE
//   Measure what happens to the ANVIL BWT backend (backend id 2) when its
//   postcoder-2 payload (MTF token stream coded with an ADAPTIVE order-1
//   arithmetic coder) is re-coded with cheaper members of ANVIL's own static
//   stream suite (rANS-256/512/4096, canonical Huffman, default-with-
//   exceptions, raw, context-switched rANS) — charging the COMPLETE new
//   payload bytes, and measuring decode throughput.
//
// PROVENANCE / SCOPE
//   This prototype does NOT contain a copy of ANVIL's codec.  It COMPILES the
//   repository's own `src/anvil.cpp` (unmodified) with `-DANVIL_NO_MAIN`, so
//   every arithmetic-coder, stream-suite, MTF and container code path used
//   here is byte-identical to the shipped codec.  See
//   prototypes/i20-postcoder-swap/README.md for the full provenance note and
//   the exact source commit.
//
//   The only *copied* code in this directory is `huffman_fixed.hpp`, a
//   corrected copy of the repository's canonical-Huffman decoder (the repo's
//   `HuffModel::first_code` is uint16_t and truncates canonical codes of
//   length >= 17, which is the documented root cause of the postcoder-0
//   "invalid huffman code" encoder/decoder mismatch).
//
//   NOTHING in `src/anvil.cpp` is modified.  This is a research prototype:
//   the "swapped" postcoder wire it produces is a PROTOTYPE wire, not a
//   proposed ANVIL format change.
//
// COMMANDS
//   proto info    --anvil F.anv
//       Dump the container / ratio-envelope / BWT-backend structure as JSON.
//
//   proto sweep   --anvil F.anv --out out.jsonl [--outdir DIR]
//       Byte-only sweep: extract the MTF token stream from the shipped
//       postcoder-2 payload, re-code it with every candidate, charge the
//       COMPLETE new payload bytes, and verify each candidate end-to-end
//       (container -> BWT -> inverse BWT -> original bytes, byte-exact).
//
//   proto decode  --anvil F.anv --out FILE (--post shipped|arith | --postfile FILE)
//       Decode the container with a selected postcoder path and write the
//       output.  `shipped` is the repository's own postcoder-2 decode;
//       `arith` is the same arithmetic decode but materialising the token
//       stream (the harness-matched baseline for the swap arms);
//       `--postfile` decodes a prototype candidate payload produced by `sweep`.
//
//   proto profile --anvil F.anv
//       Stage-1 attribution: per-stage decode timings of the shipped path.
// ===========================================================================

#define ANVIL_NO_MAIN 1
#include "../../src/anvil.cpp"  // the repository's own codec, unmodified

#include "huffman_fixed.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace anvil;

// ---------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------
static double i20_now() {
    return std::chrono::duration<double>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

static std::string i20_json_escape(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') { o.push_back('\\'); o.push_back(c); }
        else if (c == '\n') o += "\\n";
        else o.push_back(c);
    }
    return o;
}

static std::string i20_u64(uint64_t v) { return std::to_string(v); }

// ---------------------------------------------------------------------------
// container / ratio-envelope / BWT-backend parsing
// ---------------------------------------------------------------------------
struct I20Block {
    uint64_t blen = 0;
    uint8_t mode = 0;
    uint64_t plen = 0;
    size_t payload_off = 0;
    std::vector<uint8_t> payload;
};

struct I20Container {
    uint8_t revision = 0;
    uint64_t block_size = 0;
    uint64_t total = 0;
    size_t header_len = 0;
    std::vector<I20Block> blocks;
};

static I20Container i20_parse_container(const std::vector<uint8_t>& in) {
    I20Container c;
    if (in.size() < 5 || std::memcmp(in.data(), "ANV0", 4) != 0)
        throw std::runtime_error("not an ANVIL container");
    c.revision = in[4];
    if (c.revision != 2) throw std::runtime_error("prototype expects revision 2 (ratio route)");
    const uint8_t* p = in.data() + 5;
    const uint8_t* e = in.data() + in.size();
    c.block_size = get_uvar(p, e);
    c.total = get_uvar(p, e);
    c.header_len = static_cast<size_t>(p - in.data());
    while (static_cast<size_t>(p - in.data()) < in.size()) {
        I20Block b;
        b.blen = get_uvar(p, e);
        if (p >= e) throw std::runtime_error("truncated block header");
        b.mode = *p++;
        b.plen = get_uvar(p, e);
        (void)get_u32le(p, e);  // crc32 — the prototype does not re-verify it
        if (b.plen > static_cast<uint64_t>(e - p)) throw std::runtime_error("truncated block payload");
        b.payload_off = static_cast<size_t>(p - in.data());
        b.payload.assign(p, p + b.plen);
        p += b.plen;
        c.blocks.push_back(std::move(b));
    }
    return c;
}

// ratio envelope payload: [transform][backend][uvar xlen][body]
struct I20Ratio {
    uint8_t transform = 0;
    uint8_t backend = 0;
    uint64_t xlen = 0;
    const uint8_t* body = nullptr;
    size_t body_len = 0;
};

static I20Ratio i20_parse_ratio(const std::vector<uint8_t>& payload) {
    if (payload.size() < 3) throw std::runtime_error("truncated ratio payload");
    const uint8_t* p = payload.data();
    const uint8_t* e = payload.data() + payload.size();
    I20Ratio r;
    r.transform = *p++;
    r.backend = *p++;
    r.xlen = get_uvar(p, e);
    r.body = p;
    r.body_len = static_cast<size_t>(e - p);
    return r;
}

// BWT backend payload (auxiliary v2 framing): [0xFE][post][uvar rate][uvar icount][u32le*icount][post body]
struct I20Bwt {
    uint8_t post = 0;
    uint32_t aux_rate = 0;
    std::vector<int32_t> aux_indexes;
    const uint8_t* body = nullptr;   // the postcoder-specific payload
    size_t body_len = 0;
    size_t header_len = 0;           // bytes consumed before `body`
    bool aux = false;
};

static I20Bwt i20_parse_bwt(const uint8_t* p, size_t n) {
    I20Bwt b;
    if (n < 1) throw std::runtime_error("empty BWT backend payload");
    const uint8_t* start = p;
    const uint8_t* e = p + n;
    if (*p == kBwtAuxTag) {
        b.aux = true;
        ++p;
        if (p >= e) throw std::runtime_error("truncated BWT auxiliary postcoder");
        b.post = *p++;
        uint64_t rv = get_uvar(p, e);
        uint64_t icount = get_uvar(p, e);
        if (rv < 2 || (rv & (rv - 1)) != 0) throw std::runtime_error("bad aux sampling rate");
        if (icount == 0 || icount > kBwtAuxMaxIndexes) throw std::runtime_error("bad aux index count");
        if (icount > static_cast<uint64_t>(e - p) / 4u) throw std::runtime_error("truncated aux index array");
        b.aux_indexes.resize(static_cast<size_t>(icount));
        for (size_t i = 0; i < b.aux_indexes.size(); ++i) {
            uint32_t idx = get_u32le(p, e);
            b.aux_indexes[i] = static_cast<int32_t>(idx);
        }
        b.aux_rate = static_cast<uint32_t>(rv);
    } else {
        b.post = *p++;
        uint64_t pv = get_uvar(p, e);
        (void)pv;  // legacy primary index (not used by the aux route)
    }
    b.header_len = static_cast<size_t>(p - start);
    b.body = p;
    b.body_len = static_cast<size_t>(e - p);
    return b;
}

// ---------------------------------------------------------------------------
// MTF token stream extraction from the shipped postcoder-2 arithmetic payload
// ---------------------------------------------------------------------------
struct I20TokenStream {
    std::vector<uint8_t> tokens;      // MTF ranks; 0 = "a run of rank-0 symbols"
    std::vector<uint8_t> runs;        // concatenated uvarint(run_len-1) bytes
    std::vector<uint8_t> interleaved; // tokens with the run bytes spliced in
    uint64_t produced = 0;            // BWT bytes accounted for
};

// Mirrors src/anvil.cpp `bwt_arith_decode` (lines 3982-4006) but captures the
// MTF token stream instead of expanding it, so the same arithmetic payload can
// be re-coded with a different entropy coder.
static I20TokenStream i20_arith_token_stream(const uint8_t* p, size_t n, size_t expected,
                                              bool order1, uint64_t bit_count) {
    if (bit_count == 0 || bit_count > static_cast<uint64_t>(n) * 8)
        throw std::runtime_error("bad BWT arithmetic bit count");
    uint64_t need = (bit_count + 7) / 8;
    if (need != n) throw std::runtime_error("BWT arithmetic byte count mismatch");
    if ((bit_count & 7) && n) {
        uint32_t pad = 8 - static_cast<uint32_t>(bit_count & 7);
        uint8_t mask = static_cast<uint8_t>((1u << pad) - 1u);
        if (p[n - 1] & mask) throw std::runtime_error("nonzero BWT arithmetic padding");
    }
    ArithmeticDecoder ad(p, n);
    AdaptiveDecModel tok0(256), runm(256);
    std::array<std::unique_ptr<AdaptiveDecModel>, 257> tok1;
    auto model1 = [&](uint32_t ctx) -> AdaptiveDecModel& {
        if (!tok1[ctx]) tok1[ctx] = std::make_unique<AdaptiveDecModel>(256);
        return *tok1[ctx];
    };
    I20TokenStream ts;
    ts.tokens.reserve(expected / 2 + 16);
    ts.runs.reserve(expected / 16 + 16);
    ts.interleaved.reserve(expected / 2 + 16);
    uint32_t prev = 256;
    while (ts.produced < expected) {
        uint8_t t = static_cast<uint8_t>(order1 ? model1(prev).decode(ad) : tok0.decode(ad));
        prev = t;
        ts.tokens.push_back(t);
        ts.interleaved.push_back(t);
        if (t == 0) {
            uint64_t rv = decode_uvar_m(ad, runm);
            if (rv >= expected - ts.produced) throw std::runtime_error("BWT arithmetic zero run exceeds output");
            size_t before = ts.runs.size();
            append_varint_bytes(ts.runs, rv);
            for (size_t i = before; i < ts.runs.size(); ++i) ts.interleaved.push_back(ts.runs[i]);
            ts.produced += rv + 1;
        } else {
            ts.produced += 1;
        }
    }
    return ts;
}

// De-interleave a single interleaved symbol stream back into (tokens, runs).
static void i20_deinterleave(const std::vector<uint8_t>& in,
                              std::vector<uint8_t>& tokens, std::vector<uint8_t>& runs) {
    size_t i = 0;
    while (i < in.size()) {
        uint8_t t = in[i++];
        tokens.push_back(t);
        if (t == 0) {
            for (;;) {
                if (i >= in.size()) throw std::runtime_error("truncated zero-run in interleaved stream");
                uint8_t b = in[i++];
                runs.push_back(b);
                if (!(b & 0x80)) break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// stream-suite members (ANVIL's own suite, src/anvil.cpp lines 1140-1830)
// ---------------------------------------------------------------------------
enum I20SuiteMode {
    I20_RAW = 0, I20_RANS4096 = 1, I20_RANS512 = 2, I20_RANS256 = 3,
    I20_HUFF = 4, I20_DEFE = 5, I20_CTX = 6
};
static const int kI20SuiteModes = 7;

static const char* i20_mode_name(int m) {
    switch (m) {
        case I20_RAW: return "raw";
        case I20_RANS4096: return "rans4096";
        case I20_RANS512: return "rans512";
        case I20_RANS256: return "rans256";
        case I20_HUFF: return "huffman";
        case I20_DEFE: return "defexc";
        case I20_CTX: return "ctx-rans";
        default: return "?";
    }
}

// The suite's own documented decode-cost unit (src/anvil.cpp lines 1465-1470):
// raw 10, rans-4096 40, rans-512 35, rans-256 30, huffman 22, defexc 20, ctx 45.
static double i20_mode_cost_unit(int m) {
    switch (m) {
        case I20_RAW: return 10.0;
        case I20_RANS4096: return 40.0;
        case I20_RANS512: return 35.0;
        case I20_RANS256: return 30.0;
        case I20_HUFF: return 22.0;
        case I20_DEFE: return 20.0;
        case I20_CTX: return 45.0;
        default: return 999.0;
    }
}

static uint8_t i20_most_common(const std::vector<uint8_t>& src) {
    std::array<uint32_t, 256> cnt{};
    for (uint8_t b : src) ++cnt[b];
    uint8_t def = 0;
    for (int i = 1; i < 256; ++i) if (cnt[i] > cnt[def]) def = static_cast<uint8_t>(i);
    return def;
}

static std::vector<uint8_t> i20_suite_encode(const std::vector<uint8_t>& src, int mode) {
    switch (mode) {
        case I20_RAW: {
            std::vector<uint8_t> z; z.push_back(0); put_uvar(z, src.size());
            z.insert(z.end(), src.begin(), src.end()); return z;
        }
        case I20_RANS4096: return rans_stream_bytes(src, kRans4096, 1);
        case I20_RANS512:  return rans_stream_bytes(src, kRans512, 2);
        case I20_RANS256:  return rans_stream_bytes(src, kRans256, 3);
        case I20_HUFF:     return huffman_stream_bytes(src, huffman_lengths(src));
        case I20_DEFE:     return defexc_stream_bytes(src, i20_most_common(src));
        case I20_CTX:      return ctx_stream_bytes(src, kRans4096);
        default: throw std::runtime_error("unknown suite mode");
    }
}

// Decode a suite stream.  Identical to src/anvil.cpp `decode_stream` except
// that mode 4 (canonical Huffman) uses the corrected copy in huffman_fixed.hpp.
static std::vector<uint8_t> i20_suite_decode(const uint8_t*& p, const uint8_t* e, size_t max_n) {
    if (p >= e) throw std::runtime_error("truncated stream header");
    if (*p != I20_HUFF) return decode_stream(p, e, max_n);
    ++p;  // mode
    uint64_t raw_n = get_uvar(p, e);
    if (raw_n > max_n) throw std::runtime_error("stream too large");
    if (static_cast<uint64_t>(e - p) < 256) throw std::runtime_error("truncated huffman lengths");
    std::array<uint8_t, 256> len{};
    for (int i = 0; i < 256; ++i) len[i] = *p++;
    uint64_t kraft = 0;
    for (int i = 0; i < 256; ++i) if (len[i]) {
        if (len[i] > 32) throw std::runtime_error("bad huffman length (>32)");
        kraft += 1ull << (32 - len[i]);
    }
    if (kraft > (1ull << 32)) throw std::runtime_error("huffman overfull");
    i20::HuffModelFixed h = i20::build_huff_model_fixed(len);
    uint64_t dn = get_uvar(p, e);
    if (dn > static_cast<uint64_t>(e - p)) throw std::runtime_error("truncated huffman stream");
    auto out = i20::huffman_decode_fixed(p, static_cast<size_t>(dn), static_cast<size_t>(raw_n), h);
    p += dn;
    return out;
}

// ---------------------------------------------------------------------------
// prototype candidate postcoders
// ---------------------------------------------------------------------------
struct I20Candidate {
    std::string name;
    int kind = 0;        // 1 = single interleaved stream, 2 = split (tokens|runs)
    int mtok = 0, mrun = 0;
};

// Prototype post-body wire (NOT an ANVIL format proposal):
//   kind 1: [u8 kind=1][uvar len][stream bytes]
//   kind 2: [u8 kind=2][uvar len_a][stream a][uvar len_b][stream b]
static std::vector<uint8_t> i20_build_candidate_body(const I20Candidate& c, const I20TokenStream& ts) {
    std::vector<uint8_t> z;
    if (c.kind == 1) {
        auto s = i20_suite_encode(ts.interleaved, c.mtok);
        z.push_back(1); put_uvar(z, s.size()); z.insert(z.end(), s.begin(), s.end());
    } else {
        auto a = i20_suite_encode(ts.tokens, c.mtok);
        auto b = i20_suite_encode(ts.runs, c.mrun);
        z.push_back(2);
        put_uvar(z, a.size()); z.insert(z.end(), a.begin(), a.end());
        put_uvar(z, b.size()); z.insert(z.end(), b.begin(), b.end());
    }
    return z;
}

static I20TokenStream i20_decode_candidate_body(const uint8_t* p, size_t n) {
    const uint8_t* e = p + n;
    if (n < 1) throw std::runtime_error("empty candidate body");
    uint8_t kind = *p++;
    I20TokenStream ts;
    if (kind == 1) {
        uint64_t len = get_uvar(p, e);
        if (len > static_cast<uint64_t>(e - p)) throw std::runtime_error("truncated candidate stream");
        const uint8_t* q = p; const uint8_t* qe = p + len;
        auto inter = i20_suite_decode(q, qe, 1ull << 32);
        if (q != qe) throw std::runtime_error("candidate stream trailing bytes");
        p += len;
        i20_deinterleave(inter, ts.tokens, ts.runs);
        ts.interleaved = std::move(inter);
    } else if (kind == 2) {
        uint64_t la = get_uvar(p, e);
        if (la > static_cast<uint64_t>(e - p)) throw std::runtime_error("truncated candidate stream a");
        const uint8_t* q = p; const uint8_t* qe = p + la;
        ts.tokens = i20_suite_decode(q, qe, 1ull << 32);
        if (q != qe) throw std::runtime_error("candidate stream a trailing bytes");
        p += la;
        uint64_t lb = get_uvar(p, e);
        if (lb > static_cast<uint64_t>(e - p)) throw std::runtime_error("truncated candidate stream b");
        q = p; qe = p + lb;
        ts.runs = i20_suite_decode(q, qe, 1ull << 32);
        if (q != qe) throw std::runtime_error("candidate stream b trailing bytes");
        p += lb;
        if (p != e) throw std::runtime_error("candidate body trailing bytes");
        // rebuild the interleaved view by re-walking the token stream and the run bytes
        ts.interleaved.clear();
        ts.interleaved.reserve(ts.tokens.size() + ts.runs.size());
        size_t rp = 0;
        for (uint8_t t : ts.tokens) {
            ts.interleaved.push_back(t);
            if (t == 0) {
                for (;;) {
                    if (rp >= ts.runs.size()) throw std::runtime_error("run underflow");
                    uint8_t b = ts.runs[rp++];
                    ts.interleaved.push_back(b);
                    if (!(b & 0x80)) break;
                }
            }
        }
        if (rp != ts.runs.size()) throw std::runtime_error("run stream trailing bytes");
    } else {
        throw std::runtime_error("unknown candidate body kind");
    }
    return ts;
}

// ---------------------------------------------------------------------------
// the single BWT block of a ratio-route container
// ---------------------------------------------------------------------------
struct I20Target {
    I20Container cont;
    size_t block_index = 0;
    I20Ratio ratio;
    I20Bwt bwt;
    uint64_t xlen = 0;
    const uint8_t* arith_body = nullptr;  // [uvar nbits][arith bytes] for post 2
    size_t arith_body_len = 0;
    uint64_t nbits = 0;
    size_t old_file_bytes = 0;
    size_t old_plen = 0;
    size_t old_post_body_len = 0;
};

static I20Target i20_open_target(const std::vector<uint8_t>& anv) {
    I20Target t;
    t.cont = i20_parse_container(anv);
    if (t.cont.blocks.size() != 1)
        throw std::runtime_error("prototype expects exactly one container block");
    t.block_index = 0;
    const I20Block& b = t.cont.blocks[0];
    if (b.mode != 17) throw std::runtime_error("prototype expects block mode 17 (ratio envelope)");
    t.old_plen = b.payload.size();
    t.ratio = i20_parse_ratio(b.payload);
    if (t.ratio.transform != 0) throw std::runtime_error("prototype expects ratio transform 0");
    if (t.ratio.backend != kRatioBackendBwt) throw std::runtime_error("prototype expects ratio backend 2 (BWT)");
    t.xlen = t.ratio.xlen;
    t.bwt = i20_parse_bwt(t.ratio.body, t.ratio.body_len);
    if (!t.bwt.aux) throw std::runtime_error("prototype expects the auxiliary (0xFE) BWT framing");
    if (t.bwt.post != kBwtPostArithO1 && t.bwt.post != kBwtPostArithO0)
        throw std::runtime_error("prototype expects postcoder 1 or 2 (adaptive arithmetic)");
    const uint8_t* p = t.bwt.body;
    const uint8_t* e = t.bwt.body + t.bwt.body_len;
    t.nbits = get_uvar(p, e);
    t.arith_body = p;
    t.arith_body_len = static_cast<size_t>(e - p);
    t.old_post_body_len = t.bwt.body_len;
    t.old_file_bytes = anv.size();
    return t;
}

// New container size when the postcoder body is replaced by `new_body`.
static size_t i20_new_file_bytes(const I20Target& t, size_t new_body_len) {
    // container: [hdr][uvar blen][mode][uvar plen][u32 crc][payload]
    // payload  : [transform][backend][uvar xlen][bwt backend]
    // bwt      : [0xFE][post][uvar rate][uvar icount][u32*icount][post body]
    // The transformed size (xlen) and the aux framing are unchanged; only the
    // post body length changes, so only `plen` (and its uvar length) change.
    size_t new_plen = t.old_plen - t.old_post_body_len + new_body_len;
    size_t old_uv = 0, new_uv = 0;
    for (uint64_t v = t.old_plen; v; v >>= 7) ++old_uv;
    if (!old_uv) old_uv = 1;
    for (uint64_t v = new_plen; v; v >>= 7) ++new_uv;
    if (!new_uv) new_uv = 1;
    return t.old_file_bytes - t.old_plen + new_plen + (new_uv - old_uv);
}

// ---------------------------------------------------------------------------
// decode paths
// ---------------------------------------------------------------------------
enum I20DecodeMode { I20_DEC_SHIPPED, I20_DEC_ARITH, I20_DEC_CANDIDATE };

// Decode the BWT bytes for the target with the selected postcoder path.
static std::vector<uint8_t> i20_decode_bwt(const I20Target& t, I20DecodeMode mode,
                                           const std::vector<uint8_t>* cand_body) {
    const bool order1 = (t.bwt.post == kBwtPostArithO1);
    if (mode == I20_DEC_SHIPPED) {
        // exactly the repository's shipped postcoder-2 decode
        return bwt_arith_decode(t.arith_body, t.arith_body_len, static_cast<size_t>(t.xlen),
                                 order1, t.nbits);
    }
    I20TokenStream ts;
    if (mode == I20_DEC_ARITH) {
        ts = i20_arith_token_stream(t.arith_body, t.arith_body_len, static_cast<size_t>(t.xlen),
                                    order1, t.nbits);
    } else {
        if (!cand_body) throw std::runtime_error("candidate body missing");
        ts = i20_decode_candidate_body(cand_body->data(), cand_body->size());
    }
    return bwt_mtf_expand(ts.tokens, ts.runs, static_cast<size_t>(t.xlen));
}

static std::vector<uint8_t> i20_inverse_bwt(const I20Target& t, const std::vector<uint8_t>& bwt) {
    std::vector<uint8_t> out(t.xlen);
    std::vector<int32_t> tmp(t.xlen + 1);
    int32_t rc = libsais_unbwt_aux(bwt.data(), out.data(), tmp.data(),
                                   static_cast<int32_t>(t.xlen), nullptr,
                                   static_cast<int32_t>(t.bwt.aux_rate), t.bwt.aux_indexes.data());
    if (rc != 0) throw std::runtime_error("libsais inverse BWT failed");
    return out;
}

// ---------------------------------------------------------------------------
// command: info
// ---------------------------------------------------------------------------
static int i20_cmd_info(int argc, char** argv) {
    std::string anv_path;
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--anvil=", 0) == 0) anv_path = a.substr(7);
        else throw std::runtime_error("unknown option: " + a);
    }
    if (anv_path.empty()) throw std::runtime_error("--anvil is required");
    auto anv = read_file(anv_path);
    I20Target t = i20_open_target(anv);
    std::printf("{\n");
    std::printf("  \"file_bytes\": %zu,\n", t.old_file_bytes);
    std::printf("  \"revision\": %u,\n", (unsigned)t.cont.revision);
    std::printf("  \"block_size\": %llu,\n", (unsigned long long)t.cont.block_size);
    std::printf("  \"total_in\": %llu,\n", (unsigned long long)t.cont.total);
    std::printf("  \"blocks\": %zu,\n", t.cont.blocks.size());
    std::printf("  \"block_mode\": %u,\n", (unsigned)t.cont.blocks[0].mode);
    std::printf("  \"payload_len\": %zu,\n", t.old_plen);
    std::printf("  \"transform\": %u,\n", (unsigned)t.ratio.transform);
    std::printf("  \"backend\": %u,\n", (unsigned)t.ratio.backend);
    std::printf("  \"transformed_size\": %llu,\n", (unsigned long long)t.xlen);
    std::printf("  \"aux\": true,\n");
    std::printf("  \"aux_rate\": %u,\n", (unsigned)t.bwt.aux_rate);
    std::printf("  \"aux_count\": %zu,\n", t.bwt.aux_indexes.size());
    std::printf("  \"postcoder\": %u,\n", (unsigned)t.bwt.post);
    std::printf("  \"post_body_bytes\": %zu,\n", t.old_post_body_len);
    std::printf("  \"arith_nbits\": %llu,\n", (unsigned long long)t.nbits);
    std::printf("  \"arith_bytes\": %zu\n", t.arith_body_len);
    std::printf("}\n");
    return 0;
}

// ---------------------------------------------------------------------------
// command: sweep  (byte-only, all candidates, end-to-end verified)
// ---------------------------------------------------------------------------
static void i20_candidate_list(std::vector<I20Candidate>& out, bool full) {
    out.clear();
    I20Candidate base; base.name = "arith"; base.kind = 0; out.push_back(base);
    for (int m = 0; m < kI20SuiteModes; ++m) {
        I20Candidate c; c.name = std::string("single:") + i20_mode_name(m); c.kind = 1; c.mtok = m;
        out.push_back(c);
    }
    if (full) {
        for (int a = 0; a < kI20SuiteModes; ++a) {
            for (int b = 0; b < kI20SuiteModes; ++b) {
                I20Candidate c;
                c.name = std::string("split:") + i20_mode_name(a) + "+" + i20_mode_name(b);
                c.kind = 2; c.mtok = a; c.mrun = b;
                out.push_back(c);
            }
        }
    } else {
        const int pairs[][2] = {
            {I20_RANS4096, I20_RANS256}, {I20_RANS256, I20_RANS256}, {I20_RANS512, I20_RANS256},
            {I20_CTX, I20_RANS256}, {I20_CTX, I20_RANS4096}, {I20_HUFF, I20_RANS256},
            {I20_DEFE, I20_RANS256}, {I20_RAW, I20_RANS256}, {I20_RANS4096, I20_RAW},
        };
        for (auto& pr : pairs) {
            I20Candidate c;
            c.name = std::string("split:") + i20_mode_name(pr[0]) + "+" + i20_mode_name(pr[1]);
            c.kind = 2; c.mtok = pr[0]; c.mrun = pr[1];
            out.push_back(c);
        }
    }
}

// The maximum canonical Huffman code length over a stream.  The repository's
// shipped decoder rejects lengths > 24, so this is the integration constraint
// for any Huffman-coded candidate.
static uint8_t i20_huff_max_len(const std::vector<uint8_t>& src) {
    auto len = huffman_lengths(src);
    uint8_t m = 0;
    for (int i = 0; i < 256; ++i) if (len[i] > m) m = len[i];
    return m;
}

static int i20_cmd_sweep(int argc, char** argv) {
    std::string anv_path, out_path, outdir;
    bool full = false;
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--anvil=", 0) == 0) anv_path = a.substr(7);
        else if (a.rfind("--out=", 0) == 0) out_path = a.substr(6);
        else if (a.rfind("--outdir=", 0) == 0) outdir = a.substr(9);
        else if (a == "--full-grid") full = true;
        else throw std::runtime_error("unknown option: " + a);
    }
    if (anv_path.empty() || out_path.empty()) throw std::runtime_error("--anvil and --out are required");
    auto anv = read_file(anv_path);
    I20Target t = i20_open_target(anv);

    // Baseline: the shipped decode of this container (reference bytes).
    double d0 = i20_now();
    auto bwt_ship = i20_decode_bwt(t, I20_DEC_SHIPPED, nullptr);
    auto reference = i20_inverse_bwt(t, bwt_ship);
    double d1 = i20_now();
    if (reference.size() != t.xlen) throw std::runtime_error("reference decode size mismatch");

    // The MTF token stream carried by the shipped postcoder-2 payload.
    auto ts = i20_arith_token_stream(t.arith_body, t.arith_body_len, static_cast<size_t>(t.xlen),
                                     t.bwt.post == kBwtPostArithO1, t.nbits);
    if (ts.tokens.size() != ts.interleaved.size() - ts.runs.size())
        throw std::runtime_error("token/run accounting mismatch");

    std::vector<I20Candidate> cands;
    i20_candidate_list(cands, full);

    std::string base = anv_path;
    size_t slash = base.find_last_of('/');
    std::string stem = (slash == std::string::npos) ? base : base.substr(slash + 1);
    size_t dot = stem.find_last_of('.');
    if (dot != std::string::npos) stem = stem.substr(0, dot);

    std::ofstream fo(out_path, std::ios::binary);
    if (!fo) throw std::runtime_error("cannot open output: " + out_path);
    auto emit = [&](const std::string& s) { fo << s << "\n"; };

    {
        std::ostringstream o;
        o << "{\"file\": \"" << i20_json_escape(stem) << "\", \"field\": \"header\", "
          << "\"file_bytes\": " << t.old_file_bytes
          << ", \"payload_len\": " << t.old_plen
          << ", \"transformed_size\": " << (unsigned long long)t.xlen
          << ", \"postcoder\": " << (unsigned)t.bwt.post
          << ", \"aux_rate\": " << (unsigned)t.bwt.aux_rate
          << ", \"aux_count\": " << t.bwt.aux_indexes.size()
          << ", \"post_body_bytes\": " << t.old_post_body_len
          << ", \"arith_bytes\": " << t.arith_body_len
          << ", \"tokens\": " << ts.tokens.size()
          << ", \"runs\": " << ts.runs.size()
          << ", \"huff_max_len_tokens\": " << (unsigned)i20_huff_max_len(ts.tokens)
          << ", \"huff_max_len_runs\": " << (unsigned)i20_huff_max_len(ts.runs)
          << ", \"reference_decode_s\": " << std::fixed << std::setprecision(6) << (d1 - d0)
          << ", \"reference_fnv\": " << (unsigned long long)fnv1a(reference) << "}";
        emit(o.str());
    }

    for (const auto& c : cands) {
        if (c.kind == 0) {
            // baseline: identical bytes, no change
            std::ostringstream o;
            o << "{\"file\": \"" << i20_json_escape(stem) << "\", \"candidate\": \"arith\", "
              << "\"kind\": \"baseline\", \"mtok\": -1, \"mrun\": -1, "
              << "\"old_post_body_bytes\": " << t.old_post_body_len
              << ", \"new_post_body_bytes\": " << t.old_post_body_len
              << ", \"new_file_bytes\": " << t.old_file_bytes
              << ", \"delta_bytes\": 0, \"ok\": true, "
              << "\"note\": \"shipped postcoder 2 (adaptive order-1 arithmetic)\"}";
            emit(o.str());
            continue;
        }
        std::string note;
        bool ok = false;
        size_t new_body_len = 0, new_file_bytes = 0;
        std::vector<uint8_t> body;
        try {
            body = i20_build_candidate_body(c, ts);
            new_body_len = body.size();
            new_file_bytes = i20_new_file_bytes(t, new_body_len);
            // end-to-end verification
            auto cand_bwt = i20_decode_bwt(t, I20_DEC_CANDIDATE, &body);
            auto out = i20_inverse_bwt(t, cand_bwt);
            ok = (out == reference);
            if (!ok) note = "round-trip mismatch";
        } catch (const std::exception& ex) {
            note = std::string("error: ") + ex.what();
        }
        if (!outdir.empty() && ok) {
            std::string p = outdir + "/" + stem + "." + c.name + ".post";
            std::ofstream pf(p, std::ios::binary);
            if (pf) pf.write(reinterpret_cast<const char*>(body.data()),
                             static_cast<std::streamsize>(body.size()));
        }
        std::ostringstream o;
        o << "{\"file\": \"" << i20_json_escape(stem) << "\", \"candidate\": \""
          << i20_json_escape(c.name) << "\", \"kind\": " << c.kind
          << ", \"mtok\": " << c.mtok << ", \"mrun\": " << c.mrun
          << ", \"mtok_name\": \"" << i20_mode_name(c.mtok) << "\""
          << ", \"mrun_name\": \"" << i20_mode_name(c.mrun) << "\""
          << ", \"old_post_body_bytes\": " << t.old_post_body_len
          << ", \"new_post_body_bytes\": " << new_body_len
          << ", \"new_file_bytes\": " << new_file_bytes
          << ", \"delta_bytes\": " << ((long long)new_file_bytes - (long long)t.old_file_bytes)
          << ", \"ok\": " << (ok ? "true" : "false")
          << ", \"note\": \"" << i20_json_escape(note) << "\"}";
        emit(o.str());
    }
    fo.close();
    return 0;
}

// ---------------------------------------------------------------------------
// command: decode
// ---------------------------------------------------------------------------
static int i20_cmd_decode(int argc, char** argv) {
    std::string anv_path, out_path, post = "shipped", postfile;
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--anvil=", 0) == 0) anv_path = a.substr(7);
        else if (a.rfind("--out=", 0) == 0) out_path = a.substr(6);
        else if (a.rfind("--post=", 0) == 0) post = a.substr(7);
        else if (a.rfind("--postfile=", 0) == 0) postfile = a.substr(10);
        else throw std::runtime_error("unknown option: " + a);
    }
    if (anv_path.empty() || out_path.empty()) throw std::runtime_error("--anvil and --out are required");
    auto anv = read_file(anv_path);
    I20Target t = i20_open_target(anv);

    I20DecodeMode mode = I20_DEC_SHIPPED;
    std::vector<uint8_t> body;
    if (post == "shipped") mode = I20_DEC_SHIPPED;
    else if (post == "arith") mode = I20_DEC_ARITH;
    else if (post == "candidate") {
        if (postfile.empty()) throw std::runtime_error("--postfile is required for --post=candidate");
        body = read_file(postfile);
        mode = I20_DEC_CANDIDATE;
    } else throw std::runtime_error("unknown --post value: " + post);

    double t0 = i20_now();
    auto bwt = i20_decode_bwt(t, mode, mode == I20_DEC_CANDIDATE ? &body : nullptr);
    auto out = i20_inverse_bwt(t, bwt);
    double t1 = i20_now();
    uint64_t ck = fnv1a(out);
    write_file(out_path, out);
    // The checksum forces the decoded bytes to be consumed; it is printed so a
    // harness can confirm the two arms produced identical content.
    std::printf("{\"post\": \"%s\", \"out_bytes\": %zu, \"fnv\": %llu, \"decode_s\": %.6f}\n",
                i20_json_escape(post).c_str(), out.size(), (unsigned long long)ck, t1 - t0);
    return 0;
}

// ---------------------------------------------------------------------------
// command: profile  (stage-1 attribution of the shipped decode path)
// ---------------------------------------------------------------------------
// Raw-buffer copy of src/anvil.cpp `bwt_mtf_expand` (lines 3947-3961) and an
// ablation that SKIPS the zero-run expansion.  The buffer is allocated by the
// caller (outside the timed region) so the two variants differ ONLY in the
// zero-run RLE expansion work: the real variant memsets `len` bytes of
// sym[0], the ablation only advances the output position.
static void i20_mtf_expand_raw(const std::vector<uint8_t>& tokens, const std::vector<uint8_t>& runs,
                               size_t expected, uint8_t* out, bool do_rle) {
    std::array<uint8_t, 256> sym{};
    for (uint32_t i = 0; i < 256; ++i) sym[i] = static_cast<uint8_t>(i);
    size_t pos = 0, rp = 0;
    for (uint8_t t : tokens) {
        if (pos >= expected) throw std::runtime_error("BWT MTF token overflow");
        if (t == 0) {
            uint64_t rv = read_varint_bytes(runs, rp);
            if (rv >= expected - pos) throw std::runtime_error("BWT zero run exceeds output");
            size_t len = static_cast<size_t>(rv) + 1;
            if (do_rle) std::memset(out + pos, sym[0], len);
            pos += len;
        } else {
            uint8_t b = sym[t];
            out[pos++] = b;
            for (uint32_t j = t; j > 0; --j) sym[j] = sym[j - 1];
            sym[0] = b;
        }
    }
    if (pos != expected || rp != runs.size()) throw std::runtime_error("BWT MTF stream consumption mismatch");
}

static int i20_cmd_profile(int argc, char** argv) {
    std::string anv_path;
    int reps = 3;
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--anvil=", 0) == 0) anv_path = a.substr(7);
        else if (a.rfind("--reps=", 0) == 0) reps = std::atoi(a.substr(7).c_str());
        else throw std::runtime_error("unknown option: " + a);
    }
    if (anv_path.empty()) throw std::runtime_error("--anvil is required");
    if (reps < 1) reps = 1;
    auto anv = read_file(anv_path);

    double s_parse = 0, s_entropy = 0, s_mtf = 0, s_mtf_norle = 0, s_unbwt = 0, s_total = 0;
    uint64_t ck = 0;
    uint64_t xlen = 0;
    for (int r = 0; r < reps; ++r) {
        double a0 = i20_now();
        I20Target t = i20_open_target(anv);
        xlen = t.xlen;
        double a1 = i20_now();

        // (1) entropy decode -> MTF token stream
        auto ts = i20_arith_token_stream(t.arith_body, t.arith_body_len, static_cast<size_t>(t.xlen),
                                         t.bwt.post == kBwtPostArithO1, t.nbits);
        double a2 = i20_now();

        // (2) MTF expansion, with and without the zero-run RLE expansion.
        //     The output buffer is allocated OUTSIDE the timed region.
        std::vector<uint8_t> scratch(t.xlen);
        uint8_t* sp = scratch.data();
        double a2b = i20_now();
        i20_mtf_expand_raw(ts.tokens, ts.runs, static_cast<size_t>(t.xlen), sp, true);
        double a3 = i20_now();
        i20_mtf_expand_raw(ts.tokens, ts.runs, static_cast<size_t>(t.xlen), sp, false);
        double a3b = i20_now();
        (void)a2b;

        // (3) inverse BWT
        std::vector<uint8_t> out(t.xlen);
        std::vector<int32_t> tmp(t.xlen + 1);
        int32_t rc = libsais_unbwt_aux(sp, out.data(), tmp.data(),
                                       static_cast<int32_t>(t.xlen), nullptr,
                                       static_cast<int32_t>(t.bwt.aux_rate), t.bwt.aux_indexes.data());
        if (rc != 0) throw std::runtime_error("libsais inverse BWT failed");
        double a4 = i20_now();
        ck ^= fnv1a(out);
        s_parse += a1 - a0;
        s_entropy += a2 - a1;
        s_mtf += a3 - a2;
        s_mtf_norle += a3b - a3;
        s_unbwt += a4 - a3b;
        s_total += a4 - a0;
    }
    const double n = (double)reps;
    const double p_parse = s_parse / n, p_entropy = s_entropy / n, p_mtf = s_mtf / n,
                 p_mtf_nr = s_mtf_norle / n, p_unbwt = s_unbwt / n, p_total = s_total / n;
    const double p_rle = p_mtf - p_mtf_nr;
    std::printf("{\"reps\": %d, \"transformed_size\": %llu, "
                "\"parse_s\": %.6f, \"entropy_s\": %.6f, \"mtf_s\": %.6f, \"rle_s\": %.6f, "
                "\"mtf_norle_s\": %.6f, \"unbwt_s\": %.6f, \"total_s\": %.6f, "
                "\"entropy_share\": %.6f, \"postcoder_share\": %.6f, \"rle_share\": %.6f, "
                "\"unbwt_share\": %.6f, \"fnv\": %llu}\n",
                reps, (unsigned long long)xlen,
                p_parse, p_entropy, p_mtf, p_rle, p_mtf_nr, p_unbwt, p_total,
                p_total > 0 ? p_entropy / p_total : 0.0,
                p_total > 0 ? (p_entropy + p_mtf) / p_total : 0.0,
                p_total > 0 ? p_rle / p_total : 0.0,
                p_total > 0 ? p_unbwt / p_total : 0.0,
                (unsigned long long)ck);
    return 0;
}

// ---------------------------------------------------------------------------
static void i20_usage() {
    std::cerr <<
        "proto (I20 C5 postcoder swap prototype)\n"
        "  proto info    --anvil F.anv\n"
        "  proto sweep   --anvil F.anv --out out.jsonl [--outdir DIR] [--full-grid]\n"
        "  proto decode  --anvil F.anv --out FILE (--post shipped|arith|candidate --postfile FILE)\n"
        "  proto profile --anvil F.anv [--reps N]\n";
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) { i20_usage(); return 2; }
        std::string cmd = argv[1];
        if (cmd == "info") return i20_cmd_info(argc, argv);
        if (cmd == "sweep") return i20_cmd_sweep(argc, argv);
        if (cmd == "decode") return i20_cmd_decode(argc, argv);
        if (cmd == "profile") return i20_cmd_profile(argc, argv);
        i20_usage();
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "proto: " << e.what() << "\n";
        return 1;
    }
}
