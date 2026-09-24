// ANVIL I10 GROTLI G3 regionized residual standalone prototype.
//
// Frozen semantics:
//   docs/I10-GROTLI-G3-REGION-PREREG.md  (freeze revision r2)
//   docs/I10-GROTLI-G2-RESULTS.md
//   docs/I10-GROTLI-G2-CORPUS-FREEZE.md
//   docs/I10-GROTLI-G2-TYPED-PREREG.md   (leaf + lexical semantics reused)
//
// Research tooling only. This does not change ANVIL's production wire.
//
// Build:
//   clang++ -O3 -DNDEBUG -std=c++20 tools/grotli_g3.cpp \
//     -lbrotlienc -lbrotlidec -lbrotlicommon -o grotli_g3
//
// Usage:
//   grotli_g3 measure INPUT [--g2-result ROW.json]   -- one JSON diagnostic row
//   grotli_g3 selftest                               -- adversarial self-tests
//
// G2_WHOLE CONTROL POLICY (r2):
//   This tool intentionally does NOT implement a "G2-compatible" whole-object
//   carrier. Per prereg, C1 (G2_WHOLE) must be the ACTUAL frozen G2
//   implementation output (public SHA c34b291f40db4037ae114a39dae181644260dfff),
//   produced by running the frozen tools/grotli_g2.cpp. This G3 tool owns only
//   the G3_REGION_* arms plus raw Brotli. The CI workflow builds/runs the frozen
//   G2 binary separately and passes its single-file measure row to G3 via
//   --g2-result so arbitration can be combined; G3 never fabricates G2 numbers.
//
// Frozen leaf basis (identical to G2; no leaf may be added here):
//   0 RAW_LEX
//   1 EXACT_DICT
//   2 INT_FOR
//   3 INT_DELTA_FOR
//   4 INT_DOD_FOR
//
// G3 arms (owned here):
//   RAW_BROTLI    -- Brotli(original bytes); mandatory fallback
//   G3_REGION_RAW -- regionized carrier, RAW_LEX structured columns only
//   G3_REGION_DICT
//   G3_REGION_INT
//   G3_REGION_MIXED
//
// Common prototype archive envelope for every arm:
//   arm:u8 | decoded_source_len:uvar | brotli_payload
//
// FRAMING (r2 section 6): identical to frozen G2. A frame ends only at byte
// 0x0A (LF) and INCLUDES that byte; CRLF is not normalized (CR and LF both stay
// inside the frame); if bytes remain after the final LF, the whole remainder is
// one final frame. The ENTIRE frame extent is passed to the frozen G2 lexical
// parser, which already treats CR/LF as JSON whitespace, so trailing bytes are
// retained exactly in the structural/template parts. There is NO line-terminator
// code, terminator enum, or terminator side stream in the carrier.
//
// The regionized carrier is a byte-framed container so arbitrary residual bytes
// need no escaping. Every decoder-visible byte is charged. Reconstruction order
// is carried by an explicit decoder-visible group-id stream; the decoder never
// parses JSON and never rediscovers structure.

#include <brotli/decode.h>
#include <brotli/encode.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using Bytes = std::vector<uint8_t>;
using Clock = std::chrono::steady_clock;

static constexpr uint8_t kRegionMagic[4] = {'G','3','R','G'};
static constexpr uint8_t kVersion = 1;
static constexpr uint64_t kMaxDecoded = 1ull << 34; // research-wire safety bound
static constexpr uint64_t kMaxCarrierSlack = 64ull << 20;

// Standalone arm envelope selector cost shared by every arm (G2 precedent).
static constexpr uint64_t kArmSelectorBytes = 1;

enum class LeafId : uint8_t {
    RawLex = 0,
    ExactDict = 1,
    IntFor = 2,
    IntDeltaFor = 3,
    IntDodFor = 4,
};

static const char* leaf_name(LeafId id) {
    switch (id) {
        case LeafId::RawLex: return "RAW_LEX";
        case LeafId::ExactDict: return "EXACT_DICT";
        case LeafId::IntFor: return "INT_FOR";
        case LeafId::IntDeltaFor: return "INT_DELTA_FOR";
        case LeafId::IntDodFor: return "INT_DOD_FOR";
    }
    throw std::runtime_error("unknown leaf id");
}

// ---------------------------------------------------------------------------
// Canonical varints
// ---------------------------------------------------------------------------

static void put_uvar(Bytes& out, uint64_t x) {
    while (x >= 0x80) {
        out.push_back(static_cast<uint8_t>((x & 0x7f) | 0x80));
        x >>= 7;
    }
    out.push_back(static_cast<uint8_t>(x));
}

static size_t uvar_len(uint64_t x) {
    size_t n = 1;
    while (x >= 0x80) { x >>= 7; ++n; }
    return n;
}

static uint64_t get_uvar(const Bytes& in, size_t& p) {
    const size_t begin = p;
    uint64_t x = 0;
    unsigned shift = 0;
    for (unsigned i = 0; i < 10; ++i) {
        if (p >= in.size()) throw std::runtime_error("truncated uvar");
        const uint8_t b = in[p++];
        if (i == 9 && (b & 0xfe)) throw std::runtime_error("uvar overflow");
        x |= uint64_t(b & 0x7f) << shift;
        if (!(b & 0x80)) {
            if (p - begin != uvar_len(x)) throw std::runtime_error("noncanonical uvar");
            return x;
        }
        shift += 7;
    }
    throw std::runtime_error("uvar overflow");
}

static uint64_t get_uvar_bounded(const Bytes& in, size_t& p, uint64_t max_value) {
    const uint64_t x = get_uvar(in, p);
    if (x > max_value) throw std::runtime_error("uvar exceeds declared bound");
    return x;
}

// ---------------------------------------------------------------------------
// File and Brotli helpers
// ---------------------------------------------------------------------------

static Bytes read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open input");
    f.seekg(0, std::ios::end);
    const auto end = f.tellg();
    if (end < 0) throw std::runtime_error("cannot size input");
    Bytes b(static_cast<size_t>(end));
    f.seekg(0);
    if (!b.empty()) f.read(reinterpret_cast<char*>(b.data()), static_cast<std::streamsize>(b.size()));
    if (!f && !b.empty()) throw std::runtime_error("cannot read input");
    return b;
}

static std::string read_file_text(const std::string& path) {
    const Bytes b = read_file(path);
    return std::string(b.begin(), b.end());
}

static Bytes brotli_encode(const Bytes& in) {
    const size_t cap = BrotliEncoderMaxCompressedSize(in.size());
    if (!cap && !in.empty()) throw std::runtime_error("brotli size bound overflow");
    Bytes out(std::max<size_t>(cap, 1));
    size_t n = out.size();
    const uint8_t* src = in.empty() ? reinterpret_cast<const uint8_t*>("") : in.data();
    if (!BrotliEncoderCompress(11, 30, BROTLI_MODE_GENERIC, in.size(), src, &n, out.data()))
        throw std::runtime_error("brotli q11/lgwin30 encode failed");
    out.resize(n);
    return out;
}

static Bytes brotli_decode_exact(const Bytes& in, size_t expected) {
    BrotliDecoderState* s = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    if (!s) throw std::runtime_error("brotli decoder allocation failed");
    struct Guard {
        BrotliDecoderState* s;
        ~Guard() { BrotliDecoderDestroyInstance(s); }
    } guard{s};
    if (!BrotliDecoderSetParameter(s, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1))
        throw std::runtime_error("brotli large-window enable failed");
    Bytes out(expected + 1);
    size_t avail_in = in.size();
    const uint8_t* next_in = in.data();
    size_t avail_out = out.size();
    uint8_t* next_out = out.data();
    const auto rc = BrotliDecoderDecompressStream(
        s, &avail_in, &next_in, &avail_out, &next_out, nullptr);
    if (rc != BROTLI_DECODER_RESULT_SUCCESS || avail_in != 0)
        throw std::runtime_error("brotli decode failed/trailing compressed bytes");
    const size_t produced = out.size() - avail_out;
    if (produced != expected) throw std::runtime_error("brotli decoded size mismatch");
    out.resize(expected);
    return out;
}

static Bytes brotli_decode_bounded(const Bytes& in, size_t max_out) {
    BrotliDecoderState* s = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    if (!s) throw std::runtime_error("brotli decoder allocation failed");
    struct Guard {
        BrotliDecoderState* s;
        ~Guard() { BrotliDecoderDestroyInstance(s); }
    } guard{s};
    if (!BrotliDecoderSetParameter(s, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1))
        throw std::runtime_error("brotli large-window enable failed");

    size_t avail_in = in.size();
    const uint8_t* next_in = in.data();
    Bytes out;
    out.reserve(std::min<size_t>(max_out, 1u << 20));
    std::array<uint8_t, 65536> chunk{};

    for (;;) {
        size_t avail_out = chunk.size();
        uint8_t* next_out = chunk.data();
        const auto rc = BrotliDecoderDecompressStream(
            s, &avail_in, &next_in, &avail_out, &next_out, nullptr);
        const size_t produced = chunk.size() - avail_out;
        if (produced > max_out - out.size()) throw std::runtime_error("brotli decoded carrier exceeds bound");
        out.insert(out.end(), chunk.data(), chunk.data() + produced);
        if (rc == BROTLI_DECODER_RESULT_SUCCESS) {
            if (avail_in != 0) throw std::runtime_error("brotli trailing compressed bytes");
            return out;
        }
        if (rc == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) continue;
        if (rc == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT && avail_in == 0)
            throw std::runtime_error("truncated brotli stream");
        if (rc == BROTLI_DECODER_RESULT_ERROR)
            throw std::runtime_error("brotli decode error");
    }
}

static size_t complete_bytes(size_t source_len, const Bytes& brotli) {
    return static_cast<size_t>(kArmSelectorBytes) + uvar_len(source_len) + brotli.size();
}

static double ms_since(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

static double delta_pct(size_t candidate, size_t control) {
    return control ? (double(candidate) / double(control) - 1.0) * 100.0 : 0.0;
}

// ---------------------------------------------------------------------------
// Frozen G2 framing (r2 section 6): frames end only at 0x0A and include it.
// ---------------------------------------------------------------------------

struct Frame {
    size_t lo = 0; // first byte of the complete frame
    size_t hi = 0; // one past last byte of the complete frame (includes trailing LF)
};

// Identical rule to frozen G2 split_records(): a frame ends at each LF byte and
// includes it; a trailing remainder with no final LF is one final frame.
static std::vector<Frame> split_frames(const Bytes& src) {
    std::vector<Frame> out;
    size_t begin = 0;
    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] == '\n') {
            out.push_back({begin, i + 1});
            begin = i + 1;
        }
    }
    if (begin < src.size()) out.push_back({begin, src.size()});
    return out;
}

// ---------------------------------------------------------------------------
// Frozen G2 exact lexical JSON parser (reused verbatim, no semantic change)
// ---------------------------------------------------------------------------

struct Span {
    size_t lo = 0;
    size_t hi = 0;
};

static bool is_ws(uint8_t c) {
    return c == 0x20 || c == 0x09 || c == 0x0a || c == 0x0d;
}

static bool is_hex(uint8_t c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

class JsonLexParser {
public:
    JsonLexParser(const Bytes& src, size_t lo, size_t hi)
        : src_(src), hi_(hi), p_(lo) {}

    std::vector<Span> parse() {
        skip_ws();
        if (p_ >= hi_) throw std::runtime_error("blank JSON record");
        parse_value(true);
        skip_ws();
        if (p_ != hi_) throw std::runtime_error("trailing bytes after JSON value");
        return scalars_;
    }

private:
    const Bytes& src_;
    size_t hi_, p_;
    std::vector<Span> scalars_;

    void skip_ws() {
        while (p_ < hi_ && is_ws(src_[p_])) ++p_;
    }

    void parse_value(bool capture_scalar) {
        skip_ws();
        if (p_ >= hi_) throw std::runtime_error("missing JSON value");
        const uint8_t c = src_[p_];
        if (c == '{') { parse_object(); return; }
        if (c == '[') { parse_array(); return; }
        if (c == '"') { parse_string(capture_scalar); return; }
        if (c == 't') { parse_literal("true", capture_scalar); return; }
        if (c == 'f') { parse_literal("false", capture_scalar); return; }
        if (c == 'n') { parse_literal("null", capture_scalar); return; }
        if (c == '-' || (c >= '0' && c <= '9')) { parse_number(capture_scalar); return; }
        throw std::runtime_error("invalid JSON value");
    }

    void parse_object() {
        if (src_[p_++] != '{') throw std::runtime_error("object parser bug");
        skip_ws();
        if (p_ < hi_ && src_[p_] == '}') { ++p_; return; }
        for (;;) {
            skip_ws();
            if (p_ >= hi_ || src_[p_] != '"') throw std::runtime_error("object key must be string");
            parse_string(false); // object keys are structural bytes
            skip_ws();
            if (p_ >= hi_ || src_[p_++] != ':') throw std::runtime_error("missing object colon");
            parse_value(true);
            skip_ws();
            if (p_ >= hi_) throw std::runtime_error("unterminated object");
            if (src_[p_] == '}') { ++p_; return; }
            if (src_[p_] != ',') throw std::runtime_error("missing object comma");
            ++p_;
        }
    }

    void parse_array() {
        if (src_[p_++] != '[') throw std::runtime_error("array parser bug");
        skip_ws();
        if (p_ < hi_ && src_[p_] == ']') { ++p_; return; }
        for (;;) {
            parse_value(true);
            skip_ws();
            if (p_ >= hi_) throw std::runtime_error("unterminated array");
            if (src_[p_] == ']') { ++p_; return; }
            if (src_[p_] != ',') throw std::runtime_error("missing array comma");
            ++p_;
        }
    }

    void parse_string(bool capture) {
        const size_t begin = p_;
        if (p_ >= hi_ || src_[p_++] != '"') throw std::runtime_error("string parser bug");
        bool closed = false;
        while (p_ < hi_) {
            const uint8_t c = src_[p_++];
            if (c == '"') { closed = true; break; }
            if (c < 0x20) throw std::runtime_error("unescaped JSON control character");
            if (c == '\\') {
                if (p_ >= hi_) throw std::runtime_error("truncated JSON escape");
                const uint8_t e = src_[p_++];
                switch (e) {
                    case '"': case '\\': case '/':
                    case 'b': case 'f': case 'n': case 'r': case 't':
                        break;
                    case 'u':
                        for (int i = 0; i < 4; ++i) {
                            if (p_ >= hi_ || !is_hex(src_[p_++]))
                                throw std::runtime_error("bad JSON unicode escape");
                        }
                        break;
                    default:
                        throw std::runtime_error("bad JSON escape");
                }
            }
        }
        if (!closed) throw std::runtime_error("unterminated JSON string");
        if (capture) scalars_.push_back({begin, p_});
    }

    void parse_literal(const char* lit, bool capture) {
        const size_t begin = p_;
        for (const char* q = lit; *q; ++q) {
            if (p_ >= hi_ || src_[p_++] != static_cast<uint8_t>(*q))
                throw std::runtime_error("bad JSON literal");
        }
        if (capture) scalars_.push_back({begin, p_});
    }

    void parse_number(bool capture) {
        const size_t begin = p_;
        if (src_[p_] == '-') {
            ++p_;
            if (p_ >= hi_) throw std::runtime_error("truncated JSON number");
        }
        if (src_[p_] == '0') {
            ++p_;
            if (p_ < hi_ && src_[p_] >= '0' && src_[p_] <= '9')
                throw std::runtime_error("JSON number leading zero");
        } else if (src_[p_] >= '1' && src_[p_] <= '9') {
            do { ++p_; } while (p_ < hi_ && src_[p_] >= '0' && src_[p_] <= '9');
        } else {
            throw std::runtime_error("bad JSON integer part");
        }
        if (p_ < hi_ && src_[p_] == '.') {
            ++p_;
            const size_t d0 = p_;
            while (p_ < hi_ && src_[p_] >= '0' && src_[p_] <= '9') ++p_;
            if (p_ == d0) throw std::runtime_error("JSON fraction missing digits");
        }
        if (p_ < hi_ && (src_[p_] == 'e' || src_[p_] == 'E')) {
            ++p_;
            if (p_ < hi_ && (src_[p_] == '+' || src_[p_] == '-')) ++p_;
            const size_t d0 = p_;
            while (p_ < hi_ && src_[p_] >= '0' && src_[p_] <= '9') ++p_;
            if (p_ == d0) throw std::runtime_error("JSON exponent missing digits");
        }
        if (capture) scalars_.push_back({begin, p_});
    }
};

// ---------------------------------------------------------------------------
// Frozen G2 leaf payload semantics (reused verbatim)
// ---------------------------------------------------------------------------

static uint64_t zigzag_encode_i64(int64_t x) {
    if (x >= 0) return uint64_t(x) << 1;
    const uint64_t mag = uint64_t(-(x + 1)) + 1;
    return (mag << 1) - 1;
}

static int64_t zigzag_decode_i64(uint64_t u) {
    if ((u & 1u) == 0) {
        const uint64_t mag = u >> 1;
        if (mag > uint64_t(std::numeric_limits<int64_t>::max()))
            throw std::runtime_error("zigzag positive overflow");
        return static_cast<int64_t>(mag);
    }
    const uint64_t mag = (u >> 1) + 1;
    if (mag == (uint64_t{1} << 63)) return std::numeric_limits<int64_t>::min();
    if (mag > uint64_t(std::numeric_limits<int64_t>::max()))
        throw std::runtime_error("zigzag negative overflow");
    return -static_cast<int64_t>(mag);
}

static void put_svar(Bytes& out, int64_t x) { put_uvar(out, zigzag_encode_i64(x)); }
static int64_t get_svar(const Bytes& in, size_t& p) { return zigzag_decode_i64(get_uvar(in, p)); }

static uint8_t bit_width_u64(uint64_t x) {
    uint8_t w = 0;
    while (x) { ++w; x >>= 1; }
    return w;
}

static size_t packed_bytes_for(size_t count, uint8_t width) {
    if (width > 64) throw std::runtime_error("bit width > 64");
    const unsigned __int128 bits = static_cast<unsigned __int128>(count) * width;
    const unsigned __int128 n = (bits + 7) / 8;
    if (n > std::numeric_limits<size_t>::max())
        throw std::runtime_error("packed byte count overflow");
    return static_cast<size_t>(n);
}

static Bytes pack_fixed(const std::vector<uint64_t>& values, uint8_t width) {
    if (width > 64) throw std::runtime_error("bit width > 64");
    if (width == 0) {
        for (uint64_t v : values)
            if (v != 0) throw std::runtime_error("nonzero value at width 0");
        return {};
    }
    const uint64_t maxv = width == 64
        ? std::numeric_limits<uint64_t>::max()
        : ((uint64_t{1} << width) - 1);
    Bytes out;
    out.reserve(packed_bytes_for(values.size(), width));
    unsigned __int128 acc = 0;
    unsigned bits = 0;
    for (uint64_t v : values) {
        if (v > maxv) throw std::runtime_error("bitpack value exceeds width");
        acc |= static_cast<unsigned __int128>(v) << bits;
        bits += width;
        while (bits >= 8) {
            out.push_back(static_cast<uint8_t>(acc & 0xffu));
            acc >>= 8;
            bits -= 8;
        }
    }
    if (bits) out.push_back(static_cast<uint8_t>(acc & 0xffu));
    if (out.size() != packed_bytes_for(values.size(), width))
        throw std::runtime_error("bitpack length bug");
    return out;
}

static std::vector<uint64_t> unpack_fixed(
    const Bytes& in, size_t& p, size_t count, uint8_t width, size_t end)
{
    if (width > 64) throw std::runtime_error("bit width > 64");
    if (end < p || end > in.size()) throw std::runtime_error("bad bitpack extent");
    const size_t need = packed_bytes_for(count, width);
    if (need != end - p) throw std::runtime_error("packed stream extent mismatch");
    if (width == 0) return std::vector<uint64_t>(count, 0);
    const unsigned __int128 total_bits =
        static_cast<unsigned __int128>(count) * width;
    if (need && total_bits % 8) {
        const unsigned used = static_cast<unsigned>(total_bits % 8);
        const uint8_t keep = static_cast<uint8_t>((uint16_t{1} << used) - 1u);
        if ((in[end - 1] & static_cast<uint8_t>(~keep)) != 0)
            throw std::runtime_error("nonzero unused bitpack high bits");
    }
    const uint64_t mask = width == 64
        ? std::numeric_limits<uint64_t>::max()
        : ((uint64_t{1} << width) - 1);
    std::vector<uint64_t> out;
    out.reserve(count);
    unsigned __int128 acc = 0;
    unsigned bits = 0;
    for (size_t i = 0; i < count; ++i) {
        while (bits < width) {
            if (p >= end) throw std::runtime_error("truncated bitpack");
            acc |= static_cast<unsigned __int128>(in[p++]) << bits;
            bits += 8;
        }
        out.push_back(static_cast<uint64_t>(acc) & mask);
        acc >>= width;
        bits -= width;
    }
    if (p != end) throw std::runtime_error("bitpack trailing bytes");
    return out;
}

static bool parse_canonical_int64(const Bytes& tok, int64_t& out) {
    if (tok.empty()) return false;
    size_t p = 0;
    bool neg = false;
    if (tok[p] == '-') {
        neg = true;
        if (++p == tok.size()) return false;
    }
    if (tok[p] == '0') {
        if (p + 1 != tok.size()) return false;
    } else {
        if (tok[p] < '1' || tok[p] > '9') return false;
        for (size_t i = p + 1; i < tok.size(); ++i)
            if (tok[i] < '0' || tok[i] > '9') return false;
    }
    if (neg && tok.size() == 2 && tok[1] == '0') return false;
    const uint64_t limit = neg
        ? (uint64_t{1} << 63)
        : uint64_t(std::numeric_limits<int64_t>::max());
    uint64_t mag = 0;
    for (; p < tok.size(); ++p) {
        const unsigned d = unsigned(tok[p] - '0');
        if (mag > (limit - d) / 10u) return false;
        mag = mag * 10u + d;
    }
    if (neg) {
        if (mag == (uint64_t{1} << 63)) out = std::numeric_limits<int64_t>::min();
        else out = -static_cast<int64_t>(mag);
    } else {
        out = static_cast<int64_t>(mag);
    }
    const std::string canon = std::to_string(out);
    return canon.size() == tok.size()
        && std::memcmp(canon.data(), tok.data(), tok.size()) == 0;
}

static Bytes token_from_int64(int64_t v) {
    const std::string s = std::to_string(v);
    return Bytes(s.begin(), s.end());
}

static Bytes make_raw_payload(const std::vector<Bytes>& tokens) {
    Bytes out;
    for (const Bytes& tok : tokens) {
        if (tok.empty()) throw std::runtime_error("empty scalar token");
        put_uvar(out, tok.size());
        out.insert(out.end(), tok.begin(), tok.end());
    }
    return out;
}

static Bytes make_dict_payload(const std::vector<Bytes>& tokens) {
    if (tokens.empty()) throw std::runtime_error("dictionary leaf needs values");
    std::unordered_map<std::string, uint64_t> ids;
    ids.reserve(tokens.size());
    std::vector<Bytes> dict;
    std::vector<uint64_t> stream;
    stream.reserve(tokens.size());
    for (const Bytes& tok : tokens) {
        const std::string key(reinterpret_cast<const char*>(tok.data()), tok.size());
        auto it = ids.find(key);
        uint64_t id;
        if (it == ids.end()) {
            id = dict.size();
            ids.emplace(key, id);
            dict.push_back(tok);
        } else {
            id = it->second;
        }
        stream.push_back(id);
    }
    const uint8_t width = dict.size() <= 1
        ? 0
        : bit_width_u64(static_cast<uint64_t>(dict.size() - 1));
    Bytes out;
    put_uvar(out, dict.size());
    for (const Bytes& tok : dict) {
        put_uvar(out, tok.size());
        out.insert(out.end(), tok.begin(), tok.end());
    }
    out.push_back(width);
    const Bytes packed = pack_fixed(stream, width);
    out.insert(out.end(), packed.begin(), packed.end());
    return out;
}

static std::optional<Bytes> make_int_for_payload(const std::vector<int64_t>& values) {
    if (values.empty()) return std::nullopt;
    const auto mm = std::minmax_element(values.begin(), values.end());
    const int64_t base = *mm.first;
    std::vector<uint64_t> residuals;
    residuals.reserve(values.size());
    uint64_t maxr = 0;
    for (int64_t v : values) {
        const __int128 d = static_cast<__int128>(v) - static_cast<__int128>(base);
        if (d < 0 || d > static_cast<__int128>(std::numeric_limits<uint64_t>::max()))
            throw std::runtime_error("FOR residual bug");
        const uint64_t r = static_cast<uint64_t>(d);
        residuals.push_back(r);
        maxr = std::max(maxr, r);
    }
    Bytes out;
    put_svar(out, base);
    const uint8_t width = bit_width_u64(maxr);
    out.push_back(width);
    const Bytes packed = pack_fixed(residuals, width);
    out.insert(out.end(), packed.begin(), packed.end());
    return out;
}

static std::optional<Bytes> make_int_delta_payload(
    const std::vector<int64_t>& values, int64_t* out_min = nullptr, int64_t* out_max = nullptr)
{
    if (values.size() < 2) return std::nullopt;
    std::vector<int64_t> delta;
    delta.reserve(values.size() - 1);
    for (size_t i = 1; i < values.size(); ++i) {
        const __int128 d = static_cast<__int128>(values[i]) - values[i - 1];
        if (d < std::numeric_limits<int64_t>::min()
            || d > std::numeric_limits<int64_t>::max()) return std::nullopt;
        delta.push_back(static_cast<int64_t>(d));
    }
    const auto mm = std::minmax_element(delta.begin(), delta.end());
    if (out_min) *out_min = *mm.first;
    if (out_max) *out_max = *mm.second;
    const int64_t base = *mm.first;
    std::vector<uint64_t> residuals;
    residuals.reserve(delta.size());
    uint64_t maxr = 0;
    for (int64_t d : delta) {
        const __int128 rr = static_cast<__int128>(d) - base;
        if (rr < 0 || rr > static_cast<__int128>(std::numeric_limits<uint64_t>::max()))
            throw std::runtime_error("delta FOR residual bug");
        const uint64_t r = static_cast<uint64_t>(rr);
        residuals.push_back(r);
        maxr = std::max(maxr, r);
    }
    Bytes out;
    put_svar(out, values.front());
    put_svar(out, base);
    const uint8_t width = bit_width_u64(maxr);
    out.push_back(width);
    const Bytes packed = pack_fixed(residuals, width);
    out.insert(out.end(), packed.begin(), packed.end());
    return out;
}

static std::optional<Bytes> make_int_dod_payload(
    const std::vector<int64_t>& values, int64_t* out_min = nullptr, int64_t* out_max = nullptr)
{
    if (values.size() < 3) return std::nullopt;
    std::vector<int64_t> delta;
    delta.reserve(values.size() - 1);
    for (size_t i = 1; i < values.size(); ++i) {
        const __int128 d = static_cast<__int128>(values[i]) - values[i - 1];
        if (d < std::numeric_limits<int64_t>::min()
            || d > std::numeric_limits<int64_t>::max()) return std::nullopt;
        delta.push_back(static_cast<int64_t>(d));
    }
    std::vector<int64_t> dod;
    dod.reserve(delta.size() - 1);
    for (size_t i = 1; i < delta.size(); ++i) {
        const __int128 d = static_cast<__int128>(delta[i]) - delta[i - 1];
        if (d < std::numeric_limits<int64_t>::min()
            || d > std::numeric_limits<int64_t>::max()) return std::nullopt;
        dod.push_back(static_cast<int64_t>(d));
    }
    const auto mm = std::minmax_element(dod.begin(), dod.end());
    if (out_min) *out_min = *mm.first;
    if (out_max) *out_max = *mm.second;
    const int64_t base = *mm.first;
    std::vector<uint64_t> residuals;
    residuals.reserve(dod.size());
    uint64_t maxr = 0;
    for (int64_t d : dod) {
        const __int128 rr = static_cast<__int128>(d) - base;
        if (rr < 0 || rr > static_cast<__int128>(std::numeric_limits<uint64_t>::max()))
            throw std::runtime_error("DoD FOR residual bug");
        const uint64_t r = static_cast<uint64_t>(rr);
        residuals.push_back(r);
        maxr = std::max(maxr, r);
    }
    Bytes out;
    put_svar(out, values.front());
    put_svar(out, delta.front());
    put_svar(out, base);
    const uint8_t width = bit_width_u64(maxr);
    out.push_back(width);
    const Bytes packed = pack_fixed(residuals, width);
    out.insert(out.end(), packed.begin(), packed.end());
    return out;
}

static Bytes make_leaf_object(LeafId id, size_t occurrences, const Bytes& payload) {
    Bytes out;
    out.push_back(static_cast<uint8_t>(id));
    put_uvar(out, occurrences);
    put_uvar(out, payload.size());
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

static std::vector<Bytes> decode_leaf_payload(
    LeafId id, const Bytes& payload, size_t occurrences, uint64_t decoded_len)
{
    if (occurrences == 0 || occurrences > decoded_len)
        throw std::runtime_error("bad leaf occurrence count");
    size_t p = 0;
    std::vector<Bytes> out;
    out.reserve(occurrences);
    uint64_t emitted_bytes = 0;
    auto push_token = [&](Bytes tok) {
        if (tok.empty() || tok.size() > decoded_len - emitted_bytes)
            throw std::runtime_error("leaf reconstructed bytes exceed source bound");
        emitted_bytes += tok.size();
        out.push_back(std::move(tok));
    };
    if (id == LeafId::RawLex) {
        for (size_t i = 0; i < occurrences; ++i) {
            const uint64_t n = get_uvar(payload, p);
            if (n == 0 || n > decoded_len || n > payload.size() - p)
                throw std::runtime_error("bad RAW_LEX token length");
            push_token(Bytes(payload.begin() + p, payload.begin() + p + static_cast<size_t>(n)));
            p += static_cast<size_t>(n);
        }
        if (p != payload.size()) throw std::runtime_error("RAW_LEX trailing bytes");
        return out;
    }
    if (id == LeafId::ExactDict) {
        const uint64_t dict_count = get_uvar(payload, p);
        if (dict_count == 0 || dict_count > occurrences || dict_count > decoded_len)
            throw std::runtime_error("bad dictionary count");
        std::vector<Bytes> dict;
        dict.reserve(static_cast<size_t>(dict_count));
        for (uint64_t i = 0; i < dict_count; ++i) {
            const uint64_t n = get_uvar(payload, p);
            if (n == 0 || n > decoded_len || n > payload.size() - p)
                throw std::runtime_error("bad dictionary token length");
            dict.emplace_back(payload.begin() + p, payload.begin() + p + static_cast<size_t>(n));
            p += static_cast<size_t>(n);
        }
        if (p >= payload.size()) throw std::runtime_error("missing dictionary width");
        const uint8_t width = payload[p++];
        const uint8_t expected = dict_count <= 1 ? 0 : bit_width_u64(dict_count - 1);
        if (width != expected) throw std::runtime_error("dictionary bit width mismatch");
        const auto ids = unpack_fixed(payload, p, occurrences, width, payload.size());
        for (uint64_t x : ids) {
            if (x >= dict_count) throw std::runtime_error("dictionary id out of range");
            push_token(dict[static_cast<size_t>(x)]);
        }
        return out;
    }
    if (id == LeafId::IntFor) {
        const int64_t base = get_svar(payload, p);
        if (p >= payload.size()) throw std::runtime_error("missing INT_FOR width");
        const uint8_t width = payload[p++];
        const auto residuals = unpack_fixed(payload, p, occurrences, width, payload.size());
        for (uint64_t r : residuals) {
            const __int128 v = static_cast<__int128>(base) + static_cast<__int128>(r);
            if (v < std::numeric_limits<int64_t>::min() || v > std::numeric_limits<int64_t>::max())
                throw std::runtime_error("INT_FOR reconstruction overflow");
            push_token(token_from_int64(static_cast<int64_t>(v)));
        }
        return out;
    }
    if (id == LeafId::IntDeltaFor) {
        if (occurrences < 2) throw std::runtime_error("INT_DELTA_FOR occurrence count");
        int64_t cur = get_svar(payload, p);
        const int64_t min_delta = get_svar(payload, p);
        if (p >= payload.size()) throw std::runtime_error("missing INT_DELTA_FOR width");
        const uint8_t width = payload[p++];
        const auto residuals = unpack_fixed(payload, p, occurrences - 1, width, payload.size());
        push_token(token_from_int64(cur));
        for (uint64_t r : residuals) {
            const __int128 d = static_cast<__int128>(min_delta) + static_cast<__int128>(r);
            if (d < std::numeric_limits<int64_t>::min() || d > std::numeric_limits<int64_t>::max())
                throw std::runtime_error("INT_DELTA_FOR delta overflow");
            const __int128 v = static_cast<__int128>(cur) + d;
            if (v < std::numeric_limits<int64_t>::min() || v > std::numeric_limits<int64_t>::max())
                throw std::runtime_error("INT_DELTA_FOR value overflow");
            cur = static_cast<int64_t>(v);
            push_token(token_from_int64(cur));
        }
        return out;
    }
    if (id == LeafId::IntDodFor) {
        if (occurrences < 3) throw std::runtime_error("INT_DOD_FOR occurrence count");
        int64_t cur = get_svar(payload, p);
        int64_t delta = get_svar(payload, p);
        const int64_t min_dod = get_svar(payload, p);
        if (p >= payload.size()) throw std::runtime_error("missing INT_DOD_FOR width");
        const uint8_t width = payload[p++];
        const auto residuals = unpack_fixed(payload, p, occurrences - 2, width, payload.size());
        push_token(token_from_int64(cur));
        const __int128 second = static_cast<__int128>(cur) + delta;
        if (second < std::numeric_limits<int64_t>::min() || second > std::numeric_limits<int64_t>::max())
            throw std::runtime_error("INT_DOD_FOR first delta overflow");
        cur = static_cast<int64_t>(second);
        push_token(token_from_int64(cur));
        for (uint64_t r : residuals) {
            const __int128 dd = static_cast<__int128>(min_dod) + static_cast<__int128>(r);
            if (dd < std::numeric_limits<int64_t>::min() || dd > std::numeric_limits<int64_t>::max())
                throw std::runtime_error("INT_DOD_FOR DoD overflow");
            const __int128 nd = static_cast<__int128>(delta) + dd;
            if (nd < std::numeric_limits<int64_t>::min() || nd > std::numeric_limits<int64_t>::max())
                throw std::runtime_error("INT_DOD_FOR delta overflow");
            delta = static_cast<int64_t>(nd);
            const __int128 v = static_cast<__int128>(cur) + delta;
            if (v < std::numeric_limits<int64_t>::min() || v > std::numeric_limits<int64_t>::max())
                throw std::runtime_error("INT_DOD_FOR value overflow");
            cur = static_cast<int64_t>(v);
            push_token(token_from_int64(cur));
        }
        return out;
    }
    throw std::runtime_error("unknown leaf id");
}

// ---------------------------------------------------------------------------
// Shape identity (frozen G2 definition: byte-identical inter-value parts)
// ---------------------------------------------------------------------------

static std::string shape_key(const std::vector<Bytes>& parts) {
    Bytes key;
    put_uvar(key, parts.size());
    for (const auto& part : parts) {
        put_uvar(key, part.size());
        key.insert(key.end(), part.begin(), part.end());
    }
    return std::string(reinterpret_cast<const char*>(key.data()), key.size());
}

struct LeafCandidate {
    LeafId id = LeafId::RawLex;
    Bytes payload;
    // Frozen G2 planner semantics: the isolated standalone-leaf Brotli q11 byte
    // score is computed ONCE when the candidate is built, exactly as G2 caches
    // it in build_columns(). The local selector then reads this cached score
    // instead of recompressing the same leaf object per arm.
    size_t isolated_brotli_bytes = 0;
};

struct SlotPlan {
    std::vector<Bytes> tokens;
    uint64_t token_bytes = 0;
    uint64_t distinct_tokens = 0;
    bool canonical_int = false;
    int64_t int_min = 0;
    int64_t int_max = 0;
    bool delta_eligible = false;
    int64_t delta_min = 0;
    int64_t delta_max = 0;
    bool dod_eligible = false;
    int64_t dod_min = 0;
    int64_t dod_max = 0;
    std::vector<LeafCandidate> candidates;
};

struct ShapePlan {
    uint32_t id = 0;
    std::vector<Bytes> parts;      // template parts (slots + 1); retain LF/CRLF exactly
    std::vector<uint32_t> members; // frame indexes, in source order
    std::vector<SlotPlan> slots;
    uint64_t source_bytes = 0;
};

struct ParsedRecord {
    uint32_t shape_id = 0; // structured: shape id; raw: unused (0)
    bool structured = false;
};

struct RegionAnalysis {
    std::vector<Frame> frames;
    std::vector<ParsedRecord> records;
    // Encoded group layout: there is at most one raw-residual group, placed at
    // group id 0 whenever any raw frame exists. Structured shapes follow in
    // first-appearance order; with no raw frames, group 0 is the first
    // structured shape. A frame's kind is carried by its explicit group kind
    // byte, never inferred from slot count.
    std::vector<ShapePlan> shapes;
    std::vector<uint32_t> raw_members;
    uint64_t raw_frame_count = 0;
    uint64_t raw_source_bytes = 0;
    uint64_t structured_frame_count = 0;
    uint64_t structured_source_bytes = 0;
    // Encoder-only diagnostics (never transmitted).
    uint64_t lf_frames = 0, crlf_frames = 0, unterminated_frames = 0;
};

// Per-record parse + exact G2 shape identity. The ENTIRE frame extent (including
// its trailing LF/CRLF, when present) is passed to the frozen G2 lexical parser.
// A frame that does not parse becomes a raw residual frame and never disables
// any other record.
static RegionAnalysis analyze_regions(const Bytes& src) {
    RegionAnalysis a;
    a.frames = split_frames(src);

    std::unordered_map<std::string, uint32_t> ids;
    ids.reserve(a.frames.size());

    for (size_t fi = 0; fi < a.frames.size(); ++fi) {
        const Frame& f = a.frames[fi];
        const uint64_t frame_len = f.hi - f.lo;

        // Encoder-only terminator diagnostics.
        if (frame_len >= 1 && src[f.hi - 1] == '\n') {
            if (frame_len >= 2 && src[f.hi - 2] == '\r') ++a.crlf_frames;
            else ++a.lf_frames;
        } else {
            ++a.unterminated_frames;
        }

        std::vector<Span> values;
        bool structured = false;
        if (frame_len > 0) {
            try {
                JsonLexParser parser(src, f.lo, f.hi);
                values = parser.parse();
                structured = true;
            } catch (const std::exception&) {
                structured = false;
            }
        }

        if (!structured) {
            a.raw_members.push_back(static_cast<uint32_t>(fi));
            a.records.push_back({0, false});
            a.raw_frame_count += 1;
            a.raw_source_bytes += frame_len;
            continue;
        }

        std::vector<Bytes> parts;
        parts.reserve(values.size() + 1);
        size_t cursor = f.lo;
        for (const Span& s : values) {
            if (s.lo < cursor || s.hi < s.lo || s.hi > f.hi)
                throw std::runtime_error("scalar span ordering bug");
            parts.emplace_back(src.begin() + cursor, src.begin() + s.lo);
            cursor = s.hi;
        }
        parts.emplace_back(src.begin() + cursor, src.begin() + f.hi);

        const std::string key = shape_key(parts);
        auto it = ids.find(key);
        uint32_t sid;
        if (it == ids.end()) {
            if (a.shapes.size() >= std::numeric_limits<uint32_t>::max() - 1)
                throw std::runtime_error("too many shapes");
            sid = static_cast<uint32_t>(a.shapes.size());
            ids.emplace(key, sid);
            ShapePlan sh;
            sh.id = sid;
            sh.parts = parts;
            a.shapes.push_back(std::move(sh));
        } else {
            sid = it->second;
        }
        ShapePlan& sh = a.shapes[sid];
        sh.members.push_back(static_cast<uint32_t>(fi));
        for (size_t slot = 0; slot < values.size(); ++slot) {
            if (sh.slots.size() <= slot) sh.slots.emplace_back();
            sh.slots[slot].tokens.emplace_back(
                src.begin() + values[slot].lo, src.begin() + values[slot].hi);
        }
        sh.source_bytes += frame_len;

        a.records.push_back({sid, true});
        a.structured_frame_count += 1;
        a.structured_source_bytes += frame_len;
    }
    return a;
}

// Builds per-slot leaf candidate payloads for every accepted structured shape.
// The isolated q11 leaf-object score is computed once here (frozen G2 planner
// semantics) and reused by every local-selection arm.
static void build_structured_candidates(RegionAnalysis& a) {
    for (ShapePlan& sh : a.shapes) {
        for (SlotPlan& slot : sh.slots) {
            slot.token_bytes = 0;
            std::unordered_map<std::string, uint8_t> distinct;
            distinct.reserve(slot.tokens.size());
            std::vector<int64_t> ints;
            ints.reserve(slot.tokens.size());
            bool all_int = true;
            for (const Bytes& tok : slot.tokens) {
                slot.token_bytes += tok.size();
                distinct.emplace(std::string(reinterpret_cast<const char*>(tok.data()), tok.size()), 0);
                int64_t v = 0;
                if (parse_canonical_int64(tok, v)) ints.push_back(v);
                else all_int = false;
            }
            slot.distinct_tokens = distinct.size();
            auto add = [&](LeafId id, Bytes payload) {
                LeafCandidate c;
                c.id = id;
                c.payload = std::move(payload);
                c.isolated_brotli_bytes =
                    brotli_encode(make_leaf_object(id, slot.tokens.size(), c.payload)).size();
                slot.candidates.push_back(std::move(c));
            };
            add(LeafId::RawLex, make_raw_payload(slot.tokens));
            add(LeafId::ExactDict, make_dict_payload(slot.tokens));
            slot.canonical_int = all_int && ints.size() == slot.tokens.size();
            if (slot.canonical_int) {
                const auto mm = std::minmax_element(ints.begin(), ints.end());
                slot.int_min = *mm.first;
                slot.int_max = *mm.second;
                if (auto p = make_int_for_payload(ints))
                    add(LeafId::IntFor, std::move(*p));
                int64_t dmin = 0, dmax = 0;
                if (auto p = make_int_delta_payload(ints, &dmin, &dmax)) {
                    slot.delta_eligible = true;
                    slot.delta_min = dmin;
                    slot.delta_max = dmax;
                    add(LeafId::IntDeltaFor, std::move(*p));
                }
                int64_t ddmin = 0, ddmax = 0;
                if (auto p = make_int_dod_payload(ints, &ddmin, &ddmax)) {
                    slot.dod_eligible = true;
                    slot.dod_min = ddmin;
                    slot.dod_max = ddmax;
                    add(LeafId::IntDodFor, std::move(*p));
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Regionized G3 carrier
//
// Reconstruction order is explicit and decoder-visible. There is at most one
// raw-residual group (group 0, present iff any raw frame exists) plus one
// structured group per exact shape in first-appearance order. When no raw frame
// exists there is no raw group and group 0 is the first structured shape. A
// frame's kind is read from its group's explicit kind byte, never inferred from
// slot count, so zero-scalar structured shapes ({} / []) remain structured.
//
// Wire (all counts/lengths canonical uvar unless noted):
//   magic 'G','3','R','G'
//   version u8 = 1
//   source_len
//   frame_count
//   group_count
//   frame_group[frame_count]      uvar each (< group_count)
//   group_descriptors[group_count], in group-id order:
//       group_kind u8 (0 = structured shape, 1 = raw residual)
//       member_count
//       structured only: slot_count, then (slot_count + 1) template parts
//                as part_len, part_bytes
//   structured_payloads[structured groups], in group-id order:
//       for each slot: leaf_id u8, payload_len, payload
//   raw_bodies[the single raw group], if present:
//       for each member: frame_len, frame_bytes
//
// Group kind is an explicit charged byte, so a valid zero-scalar structured
// shape (e.g. "{}" or "[]", slot_count == 0) is never conflated with a raw
// residual group. When no raw frame exists there is no raw group and group 0 is
// the first structured shape.
//
// Every byte is decoder-visible; no encoder-only state is assumed.
// ---------------------------------------------------------------------------

enum class GroupKind : uint8_t {
    Structured = 0,
    Raw = 1,
};

struct CarrierStats {
    uint64_t header_bytes = 0;
    uint64_t order_bytes = 0;
    uint64_t group_table_bytes = 0;
    uint64_t template_bytes = 0;
    uint64_t residual_bytes = 0;
    uint64_t leaf_descriptor_bytes = 0;
    uint64_t leaf_payload_bytes = 0;
    uint64_t raw_leaf_count = 0;
    uint64_t dict_leaf_count = 0;
    uint64_t int_for_leaf_count = 0;
    uint64_t int_delta_leaf_count = 0;
    uint64_t int_dod_leaf_count = 0;
    uint64_t shape_count = 0;
    uint64_t raw_group_members = 0;
};

struct Selection {
    std::vector<std::vector<LeafId>> structured_shapes;
};

static const LeafCandidate* candidate_for(
    const SlotPlan& slot, LeafId id)
{
    for (const auto& c : slot.candidates) if (c.id == id) return &c;
    return nullptr;
}

// Emits the regionized container for the current analysis. All four G3 arms use
// identical carrier semantics; only the per-column selected leaves differ.
static Bytes make_region_carrier(
    const Bytes& src, const RegionAnalysis& a, const Selection& sel, CarrierStats& st)
{
    if (sel.structured_shapes.size() != a.shapes.size())
        throw std::runtime_error("selection shape count mismatch");
    if (a.frames.empty()) throw std::runtime_error("region carrier needs a frame");
    const bool has_raw = !a.raw_members.empty();
    const uint64_t group_count = a.shapes.size() + (has_raw ? 1 : 0);
    const uint32_t raw_group_id = has_raw ? 0u : std::numeric_limits<uint32_t>::max();
    // Structured shape sid maps to encoded group id (has_raw ? 1 + sid : sid).
    const uint32_t structured_offset = has_raw ? 1u : 0u;

    Bytes out;
    out.insert(out.end(), std::begin(kRegionMagic), std::end(kRegionMagic));
    out.push_back(kVersion);
    put_uvar(out, src.size());
    put_uvar(out, a.frames.size());
    put_uvar(out, group_count);
    st.header_bytes = out.size();

    for (const ParsedRecord& r : a.records) {
        if (r.structured && r.shape_id >= a.shapes.size())
            throw std::runtime_error("record shape id out of range");
        const uint32_t gid = r.structured ? (structured_offset + r.shape_id) : raw_group_id;
        put_uvar(out, gid);
    }
    st.order_bytes = out.size() - st.header_bytes;

    // Group descriptors in group-id order.
    if (has_raw) {
        const size_t before = out.size();
        out.push_back(static_cast<uint8_t>(GroupKind::Raw));
        put_uvar(out, a.raw_members.size());
        st.group_table_bytes += out.size() - before;
        st.raw_group_members = a.raw_members.size();
    }
    st.shape_count = a.shapes.size();
    for (size_t sid = 0; sid < a.shapes.size(); ++sid) {
        const ShapePlan& sh = a.shapes[sid];
        if (sh.id != sid) throw std::runtime_error("shape id ordering bug");
        const size_t before = out.size();
        out.push_back(static_cast<uint8_t>(GroupKind::Structured));
        put_uvar(out, sh.members.size());
        const size_t slots = sh.parts.empty() ? 0 : sh.parts.size() - 1;
        put_uvar(out, slots);
        for (const Bytes& part : sh.parts) {
            put_uvar(out, part.size());
            st.template_bytes += part.size();
            out.insert(out.end(), part.begin(), part.end());
        }
        st.group_table_bytes += out.size() - before;
    }

    // Structured leaf payloads in group-id order.
    for (size_t sid = 0; sid < a.shapes.size(); ++sid) {
        const ShapePlan& sh = a.shapes[sid];
        const size_t slots = sh.parts.empty() ? 0 : sh.parts.size() - 1;
        const std::vector<LeafId>& leaves = sel.structured_shapes[sid];
        if (leaves.size() != slots) throw std::runtime_error("selection slot count mismatch");
        for (size_t slot = 0; slot < slots; ++slot) {
            const auto* cand = candidate_for(sh.slots[slot], leaves[slot]);
            if (!cand) throw std::runtime_error("selected ineligible leaf");
            const size_t dbefore = out.size();
            out.push_back(static_cast<uint8_t>(cand->id));
            put_uvar(out, cand->payload.size());
            st.leaf_descriptor_bytes += out.size() - dbefore;
            st.leaf_payload_bytes += cand->payload.size();
            out.insert(out.end(), cand->payload.begin(), cand->payload.end());
            switch (cand->id) {
                case LeafId::RawLex: ++st.raw_leaf_count; break;
                case LeafId::ExactDict: ++st.dict_leaf_count; break;
                case LeafId::IntFor: ++st.int_for_leaf_count; break;
                case LeafId::IntDeltaFor: ++st.int_delta_leaf_count; break;
                case LeafId::IntDodFor: ++st.int_dod_leaf_count; break;
            }
        }
    }

    // Raw residual bodies (group 0), last.
    if (has_raw) {
        for (uint32_t fi : a.raw_members) {
            const Frame& f = a.frames[fi];
            const uint64_t len = f.hi - f.lo;
            put_uvar(out, len);
            out.insert(out.end(), src.begin() + f.lo, src.begin() + f.hi);
            st.residual_bytes += len;
        }
    }
    return out;
}

struct DGroup {
    bool raw = false;
    uint32_t occurrences = 0;
    uint32_t slots = 0;
    std::vector<uint64_t> parts;
    std::vector<uint64_t> part_len;
    std::vector<std::vector<Bytes>> columns;
};

static void take_span(
    const Bytes& carrier, size_t& p, uint64_t n, uint64_t max_n,
    uint64_t& out_off, uint64_t& out_len)
{
    if (n > max_n || n > carrier.size() - p) throw std::runtime_error("carrier span out of range");
    out_off = static_cast<uint64_t>(p);
    out_len = n;
    p += static_cast<size_t>(n);
}

static Bytes decode_region_carrier(const Bytes& carrier) {
    if (carrier.size() < 5 || std::memcmp(carrier.data(), kRegionMagic, 4) != 0)
        throw std::runtime_error("bad G3 carrier magic");
    size_t p = 4;
    if (carrier[p++] != kVersion) throw std::runtime_error("bad G3 carrier version");
    const uint64_t decoded_len = get_uvar(carrier, p);
    if (decoded_len == 0 || decoded_len > kMaxDecoded || decoded_len > std::numeric_limits<size_t>::max())
        throw std::runtime_error("bad G3 decoded length");
    const uint64_t frame_count = get_uvar_bounded(carrier, p, decoded_len);
    if (frame_count == 0) throw std::runtime_error("bad G3 frame count");
    if (frame_count > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("G3 frame count exceeds width");
    const uint64_t group_count = get_uvar_bounded(carrier, p, frame_count);
    if (group_count == 0) throw std::runtime_error("bad G3 group count");

    // Reconstruction-order metadata: explicit per-frame group ids.
    std::vector<uint64_t> frame_group;
    frame_group.reserve(static_cast<size_t>(frame_count));
    for (uint64_t i = 0; i < frame_count; ++i) {
        const uint64_t g = get_uvar_bounded(carrier, p, group_count - 1);
        frame_group.push_back(g);
    }

    // Group descriptors with explicit, charged group kind.
    std::vector<DGroup> groups;
    groups.reserve(static_cast<size_t>(group_count));
    uint64_t record_sum = 0;
    uint64_t scalar_sum = 0;
    bool saw_raw = false;
    for (uint64_t gi = 0; gi < group_count; ++gi) {
        if (p >= carrier.size()) throw std::runtime_error("truncated G3 group kind");
        const uint8_t kind = carrier[p++];
        if (kind > static_cast<uint8_t>(GroupKind::Raw))
            throw std::runtime_error("unknown G3 group kind");
        const uint64_t members = get_uvar_bounded(carrier, p, frame_count);
        if (members == 0) throw std::runtime_error("bad G3 member count");
        if (record_sum > frame_count - members)
            throw std::runtime_error("G3 member sum overflow");
        record_sum += members;
        DGroup g;
        g.raw = (kind == static_cast<uint8_t>(GroupKind::Raw));
        if (g.raw) {
            if (gi != 0) throw std::runtime_error("G3 raw group not first");
            if (saw_raw) throw std::runtime_error("G3 duplicate raw group");
            saw_raw = true;
        }
        g.occurrences = static_cast<uint32_t>(members);
        if (!g.raw) {
            const uint64_t slots = get_uvar_bounded(carrier, p, decoded_len);
            if (slots > std::numeric_limits<uint32_t>::max())
                throw std::runtime_error("G3 slot count exceeds width");
            g.slots = static_cast<uint32_t>(slots);
            if (slots + 1 > carrier.size() - p)
                throw std::runtime_error("G3 part count exceeds carrier capacity");
            if (slots && members > decoded_len / slots)
                throw std::runtime_error("G3 scalar count bound");
            const uint64_t values = slots ? slots * members : 0;
            if (scalar_sum > decoded_len - values)
                throw std::runtime_error("too many G3 scalar values");
            scalar_sum += values;
            if (scalar_sum > carrier.size()) throw std::runtime_error("G3 scalar count exceeds carrier capacity");
            g.parts.resize(static_cast<size_t>(slots) + 1);
            g.part_len.resize(static_cast<size_t>(slots) + 1);
            uint64_t template_sum = 0;
            for (uint64_t i = 0; i <= slots; ++i) {
                const uint64_t n = get_uvar_bounded(carrier, p, decoded_len);
                if (template_sum > decoded_len - n)
                    throw std::runtime_error("G3 template sum overflow");
                template_sum += n;
                const size_t idx = static_cast<size_t>(i);
                take_span(carrier, p, n, decoded_len, g.parts[idx], g.part_len[idx]);
            }
            g.columns.resize(static_cast<size_t>(slots));
        }
        groups.push_back(std::move(g));
    }
    if (record_sum != frame_count) throw std::runtime_error("G3 frame count mismatch");
    for (uint64_t g : frame_group) {
        if (g >= groups.size()) throw std::runtime_error("G3 frame group out of range");
    }

    // Structured leaf payloads.
    for (uint64_t gi = 0; gi < group_count; ++gi) {
        DGroup& g = groups[static_cast<size_t>(gi)];
        if (g.raw) continue;
        for (uint32_t slot = 0; slot < g.slots; ++slot) {
            if (p >= carrier.size()) throw std::runtime_error("truncated G3 leaf id");
            const uint8_t raw_id = carrier[p++];
            if (raw_id > static_cast<uint8_t>(LeafId::IntDodFor))
                throw std::runtime_error("unknown G3 leaf id");
            const uint64_t n = get_uvar_bounded(carrier, p, carrier.size() - p);
            Bytes payload(carrier.begin() + p, carrier.begin() + p + static_cast<size_t>(n));
            p += static_cast<size_t>(n);
            g.columns[slot] = decode_leaf_payload(
                static_cast<LeafId>(raw_id), payload, g.occurrences, decoded_len);
        }
    }

    // Raw residual bodies: exactly the single raw group, if one was declared.
    DGroup* raw_group = nullptr;
    for (DGroup& g : groups) if (g.raw) { raw_group = &g; break; }
    std::vector<uint64_t> raw_off, raw_len;
    if (raw_group) {
        raw_off.resize(raw_group->occurrences);
        raw_len.resize(raw_group->occurrences);
        for (uint32_t i = 0; i < raw_group->occurrences; ++i) {
            const uint64_t n = get_uvar_bounded(carrier, p, decoded_len);
            if (n == 0) throw std::runtime_error("G3 empty residual frame");
            take_span(carrier, p, n, decoded_len, raw_off[i], raw_len[i]);
        }
    }
    if (p != carrier.size()) throw std::runtime_error("G3 carrier trailing bytes");

    // Reconstruct frames in source order using the explicit group-id stream.
    std::vector<uint32_t> cursor(groups.size(), 0);
    Bytes out;
    out.reserve(static_cast<size_t>(decoded_len));
    for (uint64_t fi = 0; fi < frame_count; ++fi) {
        const uint64_t gid = frame_group[static_cast<size_t>(fi)];
        DGroup& g = groups[static_cast<size_t>(gid)];
        const uint32_t oi = cursor[static_cast<size_t>(gid)];
        if (oi >= g.occurrences) throw std::runtime_error("G3 group cursor overflow");
        if (g.raw) {
            if (gid != 0) throw std::runtime_error("G3 raw group not first");
            if (raw_len[oi] > decoded_len - out.size())
                throw std::runtime_error("G3 reconstructed size overflow");
            out.insert(out.end(), carrier.begin() + static_cast<size_t>(raw_off[oi]),
                       carrier.begin() + static_cast<size_t>(raw_off[oi] + raw_len[oi]));
        } else {
            for (uint32_t slot = 0; slot < g.slots; ++slot) {
                const uint64_t off = g.parts[slot];
                const uint64_t len = g.part_len[slot];
                if (len > decoded_len - out.size())
                    throw std::runtime_error("G3 reconstructed size overflow");
                out.insert(out.end(), carrier.begin() + static_cast<size_t>(off),
                           carrier.begin() + static_cast<size_t>(off + len));
                const Bytes& tok = g.columns[slot][oi];
                if (tok.size() > decoded_len - out.size())
                    throw std::runtime_error("G3 reconstructed token overflow");
                out.insert(out.end(), tok.begin(), tok.end());
            }
            const uint64_t off = g.parts[g.slots];
            const uint64_t len = g.part_len[g.slots];
            if (len > decoded_len - out.size())
                throw std::runtime_error("G3 reconstructed size overflow");
            out.insert(out.end(), carrier.begin() + static_cast<size_t>(off),
                       carrier.begin() + static_cast<size_t>(off + len));
        }
        cursor[static_cast<size_t>(gid)] = oi + 1;
    }
    // Every declared group must be fully consumed exactly once.
    for (size_t gi = 0; gi < groups.size(); ++gi) {
        if (cursor[gi] != groups[gi].occurrences)
            throw std::runtime_error("G3 group consumption mismatch");
    }
    if (out.size() != decoded_len) throw std::runtime_error("G3 reconstructed length mismatch");
    return out;
}

// ---------------------------------------------------------------------------
// Planner (frozen local selector; RAW_LEX wins exact ties)
// ---------------------------------------------------------------------------

static bool leaf_allowed(LeafId id, const std::vector<LeafId>& allowed) {
    return std::find(allowed.begin(), allowed.end(), id) != allowed.end();
}

static std::vector<std::vector<LeafId>> local_selection(
    const RegionAnalysis& a, const std::vector<LeafId>& allowed)
{
    std::vector<std::vector<LeafId>> out(a.shapes.size());
    for (size_t sid = 0; sid < a.shapes.size(); ++sid) {
        const ShapePlan& sh = a.shapes[sid];
        out[sid].assign(sh.slots.size(), LeafId::RawLex);
        for (size_t slot = 0; slot < sh.slots.size(); ++slot) {
            const SlotPlan& sp = sh.slots[slot];
            const LeafCandidate* best = candidate_for(sp, LeafId::RawLex);
            if (!best) throw std::runtime_error("slot missing RAW_LEX");
            // RAW starts as best, so RAW wins exact ties (frozen G2 behavior).
            size_t best_bytes = best->isolated_brotli_bytes;
            for (const auto& c : sp.candidates) {
                if (!leaf_allowed(c.id, allowed)) continue;
                if (c.isolated_brotli_bytes < best_bytes) {
                    best_bytes = c.isolated_brotli_bytes;
                    best = &c;
                }
            }
            out[sid][slot] = best->id;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Arm evaluation
// ---------------------------------------------------------------------------

struct ArmResult {
    std::string name;
    bool available = true;
    Bytes carrier;
    Bytes brotli;
    CarrierStats stats;
    size_t complete = std::numeric_limits<size_t>::max();
    double build_ms = 0;
    double encode_ms = 0;
    double decode_ms = 0;
    bool roundtrip = false;
};

static ArmResult evaluate_region_arm(
    const std::string& name,
    const Bytes& src,
    const RegionAnalysis& a,
    const std::vector<std::vector<LeafId>>& selection)
{
    ArmResult r;
    r.name = name;
    Selection sel;
    sel.structured_shapes = selection;

    auto t0 = Clock::now();
    r.carrier = make_region_carrier(src, a, sel, r.stats);
    r.build_ms = ms_since(t0);

    t0 = Clock::now();
    r.brotli = brotli_encode(r.carrier);
    r.encode_ms = ms_since(t0);
    r.complete = complete_bytes(src.size(), r.brotli);

    const uint64_t bound64 = std::min<uint64_t>(
        std::numeric_limits<size_t>::max(),
        std::min<uint64_t>(kMaxDecoded, uint64_t(src.size()) * 4ull + kMaxCarrierSlack));

    t0 = Clock::now();
    const Bytes carrier_dec = brotli_decode_bounded(r.brotli, static_cast<size_t>(bound64));
    const Bytes decoded = decode_region_carrier(carrier_dec);
    r.decode_ms = ms_since(t0);
    r.roundtrip = decoded == src;
    if (!r.roundtrip) throw std::runtime_error(name + " roundtrip mismatch");
    return r;
}

static std::string json_escape(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (c < 0x20) {
                    static const char* hex = "0123456789abcdef";
                    o += "\\u00";
                    o += hex[c >> 4];
                    o += hex[c & 15];
                } else o += char(c);
        }
    }
    return o;
}

struct ShapeCoverage {
    uint64_t singleton = 0;
    uint64_t top1 = 0, top5 = 0, top10 = 0, top100 = 0;
    uint64_t median = 0, p95 = 0, max = 0;
};

static ShapeCoverage shape_coverage(const RegionAnalysis& a) {
    std::vector<uint64_t> counts;
    counts.reserve(a.shapes.size());
    for (const ShapePlan& sh : a.shapes) counts.push_back(sh.members.size());
    std::sort(counts.begin(), counts.end(), std::greater<uint64_t>());
    ShapeCoverage s;
    if (counts.empty()) return s;
    for (uint64_t x : counts) s.singleton += (x == 1);
    auto top = [&](size_t n) {
        return std::accumulate(counts.begin(), counts.begin() + std::min(n, counts.size()), uint64_t{0});
    };
    s.top1 = top(1); s.top5 = top(5); s.top10 = top(10); s.top100 = top(100);
    std::vector<uint64_t> asc = counts;
    std::sort(asc.begin(), asc.end());
    s.median = asc[(asc.size() - 1) / 2];
    s.p95 = asc[static_cast<size_t>(std::floor(0.95 * double(asc.size() - 1)))];
    s.max = counts.front();
    return s;
}

static void emit_arm_json(const char* prefix, const ArmResult& r, size_t raw_complete) {
    std::cout << ",\"" << prefix << "_available\":" << (r.available ? "true" : "false");
    if (!r.available) return;
    std::cout << ",\"" << prefix << "_carrier_bytes\":" << r.carrier.size();
    std::cout << ",\"" << prefix << "_brotli_bytes\":" << r.brotli.size();
    std::cout << ",\"" << prefix << "_complete_bytes\":" << r.complete;
    std::cout << ",\"" << prefix << "_delta_pct\":" << delta_pct(r.complete, raw_complete);
    std::cout << ",\"" << prefix << "_roundtrip\":" << (r.roundtrip ? "true" : "false");
    std::cout << ",\"" << prefix << "_build_ms\":" << r.build_ms;
    std::cout << ",\"" << prefix << "_encode_ms\":" << r.encode_ms;
    std::cout << ",\"" << prefix << "_decode_ms\":" << r.decode_ms;
    std::cout << ",\"" << prefix << "_raw_leaf_count\":" << r.stats.raw_leaf_count;
    std::cout << ",\"" << prefix << "_dict_leaf_count\":" << r.stats.dict_leaf_count;
    std::cout << ",\"" << prefix << "_int_for_leaf_count\":" << r.stats.int_for_leaf_count;
    std::cout << ",\"" << prefix << "_int_delta_leaf_count\":" << r.stats.int_delta_leaf_count;
    std::cout << ",\"" << prefix << "_int_dod_leaf_count\":" << r.stats.int_dod_leaf_count;
}

static void count_families(
    const RegionAnalysis& a,
    const std::vector<std::vector<LeafId>>& sel,
    uint64_t& raw, uint64_t& dict, uint64_t& ints)
{
    raw = dict = ints = 0;
    for (size_t sid = 0; sid < a.shapes.size(); ++sid) {
        for (LeafId id : sel[sid]) {
            switch (id) {
                case LeafId::RawLex: ++raw; break;
                case LeafId::ExactDict: ++dict; break;
                case LeafId::IntFor:
                case LeafId::IntDeltaFor:
                case LeafId::IntDodFor: ++ints; break;
            }
        }
    }
}

// The frozen G2 control is NOT reproduced here. The workflow runs the frozen
// tools/grotli_g2.cpp binary and passes its single-file `measure` JSON row via
// --g2-result. We extract a strictly-validated control binding so the G3 row can
// report the combined portfolio without fabricating any G2 number.
struct G2Control {
    bool provided = false;
    bool eligible = false;
    size_t source_bytes = 0;
    size_t raw_complete_bytes = 0;
    size_t selected_bytes = 0;
    std::string selected_arm;
};

static bool find_json_uint(const std::string& text, const std::string& key, size_t& out) {
    const std::string pat = "\"" + key + "\":";
    const size_t at = text.find(pat);
    if (at == std::string::npos) return false;
    size_t p = at + pat.size();
    while (p < text.size() && text[p] == ' ') ++p;
    size_t q = p;
    while (q < text.size() && text[q] >= '0' && text[q] <= '9') ++q;
    if (q == p) return false;
    out = static_cast<size_t>(std::stoull(text.substr(p, q - p)));
    return true;
}

static bool find_json_bool(const std::string& text, const std::string& key, bool& out) {
    const std::string pat = "\"" + key + "\":";
    const size_t at = text.find(pat);
    if (at == std::string::npos) return false;
    const std::string rest = text.substr(at + pat.size());
    if (rest.rfind("true", 0) == 0) { out = true; return true; }
    if (rest.rfind("false", 0) == 0) { out = false; return true; }
    return false;
}

static bool find_json_string(
    const std::string& text, const std::string& key, std::string& out)
{
    const std::string pat = "\"" + key + "\":\"";
    const size_t at = text.find(pat);
    if (at == std::string::npos) return false;
    const size_t b = at + pat.size();
    const size_t e = text.find('"', b);
    if (e == std::string::npos) return false;
    out = text.substr(b, e - b);
    return true;
}

// Frozen G2 selected_arm vocabulary (grotli_g2.cpp emits exactly these).
static bool is_frozen_g2_arm(const std::string& arm) {
    return arm == "RAW_BROTLI" || arm == "G1R" || arm == "P-DICT" ||
           arm == "P-INT" || arm == "P-MIXED" || arm == "P-MARGINAL";
}

// Strictly validated extraction from the frozen G2 row. When --g2-result is
// supplied explicitly, a malformed/incomplete row is a hard error: it must not
// silently vanish and be treated as "no control". We require the frozen schema
// tag and every field arbitration depends on, and reject inconsistent values.
static G2Control parse_g2_control(const std::string& text) {
    G2Control g;
    size_t schema = 0;
    if (!find_json_uint(text, "schema", schema))
        throw std::runtime_error("G2 control row missing schema");
    if (schema != 2)
        throw std::runtime_error("G2 control row has unexpected schema (need 2)");
    if (!find_json_uint(text, "source_bytes", g.source_bytes))
        throw std::runtime_error("G2 control row missing source_bytes");
    if (!find_json_uint(text, "raw_complete_bytes", g.raw_complete_bytes))
        throw std::runtime_error("G2 control row missing raw_complete_bytes");
    if (!find_json_uint(text, "selected_bytes", g.selected_bytes))
        throw std::runtime_error("G2 control row missing selected_bytes");
    if (!find_json_string(text, "selected_arm", g.selected_arm))
        throw std::runtime_error("G2 control row missing selected_arm");
    if (!is_frozen_g2_arm(g.selected_arm))
        throw std::runtime_error("G2 control row has unknown selected_arm: " + g.selected_arm);
    if (g.selected_bytes > g.raw_complete_bytes)
        throw std::runtime_error("G2 control selected_bytes exceeds raw_complete_bytes");
    bool eligible = false;
    if (!find_json_bool(text, "structured_eligible", eligible))
        throw std::runtime_error("G2 control row missing/invalid structured_eligible");
    // Structured eligibility only says the expert is available; RAW_BROTLI may
    // still legitimately win final arbitration. Conversely, an unavailable
    // structured expert must route exactly to the raw candidate.
    if (!eligible && g.selected_arm != "RAW_BROTLI")
        throw std::runtime_error("ineligible G2 control selected a non-raw arm");
    if (!eligible && g.selected_bytes != g.raw_complete_bytes)
        throw std::runtime_error("ineligible G2 control did not fall back exactly to raw");
    g.eligible = eligible;
    g.provided = true;
    return g;
}

// smallest of the four G3 regionized arms (used by the held-out gate).
static size_t region_best_bytes(
    const ArmResult& raw, const ArmResult& dict,
    const ArmResult& ints, const ArmResult& mixed)
{
    size_t best = raw.complete;
    best = std::min(best, dict.complete);
    best = std::min(best, ints.complete);
    best = std::min(best, mixed.complete);
    return best;
}

static int measure(const std::string& path, const G2Control& g2) {
    const Bytes src = read_file(path);

    const auto raw_t0 = Clock::now();
    const Bytes raw_br = brotli_encode(src);
    const double raw_enc_ms = ms_since(raw_t0);
    const auto raw_d0 = Clock::now();
    const Bytes raw_dec = brotli_decode_exact(raw_br, src.size());
    const double raw_dec_ms = ms_since(raw_d0);
    if (raw_dec != src) throw std::runtime_error("raw Brotli roundtrip mismatch");
    const size_t raw_complete = complete_bytes(src.size(), raw_br);

    bool region_ok = true;
    std::string region_reason;
    RegionAnalysis a;
    double parse_ms = 0;
    double candidate_ms = 0;
    if (src.empty()) {
        region_ok = false;
        region_reason = "empty input has no frames";
    } else {
        try {
            const auto t0 = Clock::now();
            a = analyze_regions(src);
            parse_ms = ms_since(t0);
            const auto t1 = Clock::now();
            build_structured_candidates(a);
            candidate_ms = ms_since(t1);
        } catch (const std::exception& e) {
            region_ok = false;
            region_reason = e.what();
        }
    }

    if (!region_ok) {
        std::cout << "{"
                  << "\"schema\":3"
                  << ",\"file\":\"" << json_escape(path) << "\""
                  << ",\"source_bytes\":" << src.size()
                  << ",\"region_ok\":false"
                  << ",\"region_reason\":\"" << json_escape(region_reason) << "\""
                  << ",\"raw_brotli_bytes\":" << raw_br.size()
                  << ",\"raw_complete_bytes\":" << raw_complete
                  << ",\"raw_roundtrip\":true"
                  << ",\"selected_arm\":\"RAW_BROTLI\""
                  << ",\"selected_bytes\":" << raw_complete
                  << ",\"selected_delta_pct\":0.0"
                  << "}\n";
        return 0;
    }

    const auto planner_start = Clock::now();
    const auto sel_raw = local_selection(a, {LeafId::RawLex});
    const auto sel_dict = local_selection(a, {LeafId::RawLex, LeafId::ExactDict});
    const auto sel_int = local_selection(
        a, {LeafId::RawLex, LeafId::IntFor, LeafId::IntDeltaFor, LeafId::IntDodFor});
    const auto sel_mixed = local_selection(
        a, {LeafId::RawLex, LeafId::ExactDict, LeafId::IntFor,
            LeafId::IntDeltaFor, LeafId::IntDodFor});
    const double local_score_ms = ms_since(planner_start);

    ArmResult region_raw = evaluate_region_arm("G3_REGION_RAW", src, a, sel_raw);
    ArmResult region_dict = evaluate_region_arm("G3_REGION_DICT", src, a, sel_dict);
    ArmResult region_int = evaluate_region_arm("G3_REGION_INT", src, a, sel_int);
    ArmResult region_mixed = evaluate_region_arm("G3_REGION_MIXED", src, a, sel_mixed);

    // The frozen G2 control (if supplied) must bind to THIS source object.
    // Cross-file control rows are rejected so arbitration can never mix objects.
    bool g2_bound = false;
    if (g2.provided) {
        if (g2.source_bytes != src.size())
            throw std::runtime_error("G2 control source_bytes mismatch");
        if (g2.raw_complete_bytes != raw_complete)
            throw std::runtime_error("G2 control raw_complete_bytes mismatch");
        g2_bound = true;
    }

    // G2_WHOLE is the frozen G2 control, supplied externally by the workflow.
    // This tool never fabricates it and never substitutes an approximated
    // carrier for it (r2).
    const size_t region_best = region_best_bytes(
        region_raw, region_dict, region_int, region_mixed);

    struct Choice { std::string name; size_t bytes; };
    Choice selected{"RAW_BROTLI", raw_complete};
    auto choose = [&](const std::string& name, bool available, size_t bytes) {
        if (available && bytes < selected.bytes) selected = {name, bytes};
    };
    if (g2_bound && g2.eligible)
        choose("G2_WHOLE", true, g2.selected_bytes);
    choose("G3_REGION_RAW", true, region_raw.complete);
    choose("G3_REGION_DICT", true, region_dict.complete);
    choose("G3_REGION_INT", true, region_int.complete);
    choose("G3_REGION_MIXED", true, region_mixed.complete);

    const ShapeCoverage cov = shape_coverage(a);

    std::cout << "{";
    std::cout << "\"schema\":3";
    std::cout << ",\"file\":\"" << json_escape(path) << "\"";
    std::cout << ",\"source_bytes\":" << src.size();
    std::cout << ",\"region_ok\":true";
    std::cout << ",\"raw_brotli_bytes\":" << raw_br.size();
    std::cout << ",\"raw_complete_bytes\":" << raw_complete;
    std::cout << ",\"raw_encode_ms\":" << std::fixed << std::setprecision(6) << raw_enc_ms;
    std::cout << ",\"raw_decode_ms\":" << raw_dec_ms;
    std::cout << ",\"raw_roundtrip\":true";
    std::cout << ",\"parse_framing_ms\":" << parse_ms;
    std::cout << ",\"candidate_build_local_score_ms\":" << candidate_ms;
    std::cout << ",\"local_leaf_scoring_ms\":" << local_score_ms;

    // Regionization diagnostics (r2 section 21).
    std::cout << ",\"frame_count\":" << a.frames.size();
    std::cout << ",\"structured_frame_count\":" << a.structured_frame_count;
    std::cout << ",\"raw_frame_count\":" << a.raw_frame_count;
    std::cout << ",\"structured_source_bytes\":" << a.structured_source_bytes;
    std::cout << ",\"raw_source_bytes\":" << a.raw_source_bytes;
    std::cout << ",\"structured_coverage_fraction\":"
              << (src.empty() ? 0.0 : double(a.structured_source_bytes) / double(src.size()));
    std::cout << ",\"raw_residual_fraction\":"
              << (src.empty() ? 0.0 : double(a.raw_source_bytes) / double(src.size()));
    std::cout << ",\"encoder_lf_frames\":" << a.lf_frames;
    std::cout << ",\"encoder_crlf_frames\":" << a.crlf_frames;
    std::cout << ",\"encoder_unterminated_frames\":" << a.unterminated_frames;
    std::cout << ",\"shape_count\":" << a.shapes.size();
    std::cout << ",\"singleton_shape_count\":" << cov.singleton;
    std::cout << ",\"top1_shape_rows\":" << cov.top1;
    std::cout << ",\"top5_shape_rows\":" << cov.top5;
    std::cout << ",\"top10_shape_rows\":" << cov.top10;
    std::cout << ",\"top100_shape_rows\":" << cov.top100;

    // Frozen G2 control is reported as an externally supplied value only, and
    // only when it bound to this exact source object.
    std::cout << ",\"g2_control_provided\":" << (g2.provided ? "true" : "false");
    std::cout << ",\"g2_control_bound\":" << (g2_bound ? "true" : "false");
    std::cout << ",\"g2_control_eligible\":" << (g2_bound && g2.eligible ? "true" : "false");
    if (g2_bound) {
        std::cout << ",\"g2_whole_selected_arm\":\"" << json_escape(g2.selected_arm) << "\"";
        std::cout << ",\"g2_whole_complete_bytes\":" << g2.selected_bytes;
        std::cout << ",\"g2_whole_delta_pct\":" << delta_pct(g2.selected_bytes, raw_complete);
    }

    emit_arm_json("g3_region_raw", region_raw, raw_complete);
    emit_arm_json("g3_region_dict", region_dict, raw_complete);
    emit_arm_json("g3_region_int", region_int, raw_complete);
    emit_arm_json("g3_region_mixed", region_mixed, raw_complete);

    uint64_t f_raw = 0, f_dict = 0, f_int = 0;
    count_families(a, sel_mixed, f_raw, f_dict, f_int);
    std::cout << ",\"mixed_selected_raw_lex\":" << f_raw;
    std::cout << ",\"mixed_selected_exact_dict\":" << f_dict;
    std::cout << ",\"mixed_selected_int\":" << f_int;

    uint64_t isolated_q11_evals = 0;
    for (const ShapePlan& sh : a.shapes)
        for (const SlotPlan& slot : sh.slots)
            isolated_q11_evals += slot.candidates.size();
    std::cout << ",\"isolated_q11_leaf_evaluations\":" << isolated_q11_evals;

    // Preserve the G2 per-column attribution diagnostics so a G3 outcome can be
    // separated into region-coverage, operator, and planner effects.
    std::cout << ",\"columns\":[";
    bool first_column = true;
    for (size_t sid = 0; sid < a.shapes.size(); ++sid) {
        const ShapePlan& sh = a.shapes[sid];
        for (size_t slot_id = 0; slot_id < sh.slots.size(); ++slot_id) {
            const SlotPlan& slot = sh.slots[slot_id];
            if (!first_column) std::cout << ",";
            first_column = false;
            std::cout << "{\"shape_id\":" << sid
                      << ",\"slot_id\":" << slot_id
                      << ",\"occurrences\":" << slot.tokens.size()
                      << ",\"token_bytes\":" << slot.token_bytes
                      << ",\"distinct_tokens\":" << slot.distinct_tokens
                      << ",\"dictionary_cardinality_ratio\":"
                      << (slot.tokens.empty()
                              ? 0.0
                              : double(slot.distinct_tokens) / double(slot.tokens.size()))
                      << ",\"canonical_int\":"
                      << (slot.canonical_int ? "true" : "false");
            if (slot.canonical_int) {
                std::cout << ",\"int_min\":" << slot.int_min
                          << ",\"int_max\":" << slot.int_max
                          << ",\"delta_eligible\":"
                          << (slot.delta_eligible ? "true" : "false")
                          << ",\"dod_eligible\":"
                          << (slot.dod_eligible ? "true" : "false");
                if (slot.delta_eligible)
                    std::cout << ",\"delta_min\":" << slot.delta_min
                              << ",\"delta_max\":" << slot.delta_max;
                if (slot.dod_eligible)
                    std::cout << ",\"dod_min\":" << slot.dod_min
                              << ",\"dod_max\":" << slot.dod_max;
            }
            std::cout << ",\"candidates\":[";
            for (size_t ci = 0; ci < slot.candidates.size(); ++ci) {
                if (ci) std::cout << ",";
                const LeafCandidate& c = slot.candidates[ci];
                std::cout << "{\"leaf\":\"" << leaf_name(c.id)
                          << "\",\"payload_bytes\":" << c.payload.size()
                          << ",\"isolated_brotli_bytes\":"
                          << c.isolated_brotli_bytes << "}";
            }
            std::cout << "]"
                      << ",\"region_raw_leaf\":\""
                      << leaf_name(sel_raw[sid][slot_id]) << "\""
                      << ",\"region_dict_leaf\":\""
                      << leaf_name(sel_dict[sid][slot_id]) << "\""
                      << ",\"region_int_leaf\":\""
                      << leaf_name(sel_int[sid][slot_id]) << "\""
                      << ",\"region_mixed_leaf\":\""
                      << leaf_name(sel_mixed[sid][slot_id]) << "\""
                      << "}";
        }
    }
    std::cout << "]";

    std::cout << ",\"g3_region_best_complete_bytes\":" << region_best;
    std::cout << ",\"g3_region_best_delta_pct\":" << delta_pct(region_best, raw_complete);
    std::cout << ",\"selected_arm\":\"" << selected.name << "\"";
    std::cout << ",\"selected_bytes\":" << selected.bytes;
    std::cout << ",\"selected_delta_pct\":" << delta_pct(selected.bytes, raw_complete);
    std::cout << "}\n";
    return 0;
}

// ---------------------------------------------------------------------------
// Self-tests (mandatory adversarial set; r2 section 15)
// ---------------------------------------------------------------------------

static Bytes bytes(const std::string& s) { return Bytes(s.begin(), s.end()); }

template<class F>
static void require_throw(F&& fn, const char* label) {
    bool threw = false;
    try { fn(); } catch (...) { threw = true; }
    if (!threw) throw std::runtime_error(std::string("expected rejection: ") + label);
}

static void require_tokens(
    const std::vector<Bytes>& got,
    const std::vector<std::string>& want,
    const char* label)
{
    if (got.size() != want.size()) throw std::runtime_error(std::string(label) + " count");
    for (size_t i = 0; i < want.size(); ++i)
        if (std::string(got[i].begin(), got[i].end()) != want[i])
            throw std::runtime_error(std::string(label) + " token");
}

static void leaf_selftests() {
    {
        std::vector<Bytes> t{bytes("\"x\""), bytes("true"), bytes("null")};
        require_tokens(
            decode_leaf_payload(LeafId::RawLex, make_raw_payload(t), t.size(), 100),
            {"\"x\"", "true", "null"}, "RAW");
    }
    {
        std::vector<Bytes> t{bytes("\"a\""), bytes("\"b\""), bytes("\"a\""), bytes("\"a\"")};
        require_tokens(
            decode_leaf_payload(LeafId::ExactDict, make_dict_payload(t), t.size(), 100),
            {"\"a\"", "\"b\"", "\"a\"", "\"a\""}, "DICT");
    }
    {
        std::vector<int64_t> v{
            std::numeric_limits<int64_t>::min(), -1, 0, 1,
            std::numeric_limits<int64_t>::max()};
        auto p = make_int_for_payload(v);
        if (!p) throw std::runtime_error("FOR selftest unavailable");
        require_tokens(
            decode_leaf_payload(LeafId::IntFor, *p, v.size(), 256),
            {std::to_string(v[0]), "-1", "0", "1", std::to_string(v[4])}, "FOR");
    }
    {
        std::vector<int64_t> v{100, 102, 105, 109};
        auto p = make_int_delta_payload(v);
        if (!p) throw std::runtime_error("DELTA selftest unavailable");
        require_tokens(
            decode_leaf_payload(LeafId::IntDeltaFor, *p, v.size(), 100),
            {"100", "102", "105", "109"}, "DELTA");
    }
    {
        std::vector<int64_t> v{1000, 1010, 1021, 1033, 1046};
        auto p = make_int_dod_payload(v);
        if (!p) throw std::runtime_error("DOD selftest unavailable");
        require_tokens(
            decode_leaf_payload(LeafId::IntDodFor, *p, v.size(), 100),
            {"1000", "1010", "1021", "1033", "1046"}, "DOD");
    }
    {
        const std::vector<int64_t> v{
            std::numeric_limits<int64_t>::min(),
            std::numeric_limits<int64_t>::max()};
        if (make_int_delta_payload(v))
            throw std::runtime_error("delta overflow should be ineligible");
    }
    {
        const std::vector<int64_t> v{0, std::numeric_limits<int64_t>::min(), -1};
        if (!make_int_delta_payload(v))
            throw std::runtime_error("delta overflow control unexpectedly ineligible");
        if (make_int_dod_payload(v))
            throw std::runtime_error("DoD overflow should be ineligible");
    }

    for (const std::string& bad : {"-0", "00", "01", "+1", "1.0", "1e2",
                                   "9223372036854775808", "-9223372036854775809"}) {
        int64_t x = 0;
        if (parse_canonical_int64(bytes(bad), x))
            throw std::runtime_error("noncanonical integer accepted: " + bad);
    }
    for (const std::string& good : {"0", "-1", "1", "9223372036854775807",
                                    "-9223372036854775808"}) {
        int64_t x = 0;
        if (!parse_canonical_int64(bytes(good), x) || std::to_string(x) != good)
            throw std::runtime_error("canonical integer rejected: " + good);
    }

    {
        // dict_count=3 => width=2. ID 3 is out of range while unused high bits
        // stay zero, exercising ID range rather than padding.
        Bytes bad{3, 1, 'a', 1, 'b', 1, 'c', 2, 3};
        require_throw([&]{ (void)decode_leaf_payload(LeafId::ExactDict, bad, 1, 100); },
                      "dictionary bad id");
    }
    {
        Bytes bad{2, 1, 'a', 1, 'b', 2, 0};
        require_throw([&]{ (void)decode_leaf_payload(LeafId::ExactDict, bad, 1, 100); },
                      "dictionary bad width");
    }
    {
        Bytes packed = pack_fixed({1, 0, 1}, 1);
        packed.back() |= 0x80;
        size_t p = 0;
        require_throw([&]{ (void)unpack_fixed(packed, p, 3, 1, packed.size()); },
                      "bitpack high bits");
    }
    {
        std::vector<Bytes> t{bytes("1"), bytes("2")};
        Bytes p = make_raw_payload(t);
        p.push_back(0);
        require_throw([&]{ (void)decode_leaf_payload(LeafId::RawLex, p, 2, 20); },
                      "leaf trailing");
        p = make_raw_payload(t);
        p.pop_back();
        require_throw([&]{ (void)decode_leaf_payload(LeafId::RawLex, p, 2, 20); },
                      "leaf truncated");
    }
}

// Builds all four regionized selections for a fixture, verifies every arm
// round-trips exactly, then exercises malformed-carrier rejections.
static void region_fixture(const std::string& text, const char* label) {
    const Bytes src = bytes(text);
    RegionAnalysis a = analyze_regions(src);
    build_structured_candidates(a);

    const auto sel_raw = local_selection(a, {LeafId::RawLex});
    const auto sel_dict = local_selection(a, {LeafId::RawLex, LeafId::ExactDict});
    const auto sel_int = local_selection(
        a, {LeafId::RawLex, LeafId::IntFor, LeafId::IntDeltaFor, LeafId::IntDodFor});
    const auto sel_mixed = local_selection(
        a, {LeafId::RawLex, LeafId::ExactDict, LeafId::IntFor,
            LeafId::IntDeltaFor, LeafId::IntDodFor});

    const std::array<const std::vector<std::vector<LeafId>>*, 4> sels = {
        &sel_raw, &sel_dict, &sel_int, &sel_mixed};
    for (const auto* sel : sels) {
        Selection s;
        s.structured_shapes = *sel;
        CarrierStats st;
        const Bytes c = make_region_carrier(src, a, s, st);
        if (decode_region_carrier(c) != src)
            throw std::runtime_error(std::string(label) + " carrier roundtrip");
        const Bytes br = brotli_encode(c);
        const Bytes cd = brotli_decode_bounded(br, src.size() * 4 + kMaxCarrierSlack);
        if (decode_region_carrier(cd) != src)
            throw std::runtime_error(std::string(label) + " brotli+carrier roundtrip");
    }

    Selection s;
    s.structured_shapes = sel_raw;
    CarrierStats st;
    const Bytes c = make_region_carrier(src, a, s, st);
    Bytes bad = c;
    bad.pop_back();
    require_throw([&]{ (void)decode_region_carrier(bad); }, "truncated region carrier");
    bad = c;
    bad.push_back(0);
    require_throw([&]{ (void)decode_region_carrier(bad); }, "region carrier trailing byte");
    bad = c;
    bad[0] ^= 1;
    require_throw([&]{ (void)decode_region_carrier(bad); }, "region carrier magic");
}

// Sentinel for "offset not found" in the carrier walk used by tests.
static constexpr size_t kNoOffset = std::numeric_limits<size_t>::max();

// Walks a well-formed carrier to the first structured group's first leaf-id byte.
// Used only by malformed-carrier self-tests; not part of the decoder.
static size_t first_structured_leaf_id_offset(const Bytes& carrier) {
    if (carrier.size() < 5 || std::memcmp(carrier.data(), kRegionMagic, 4) != 0)
        return kNoOffset;
    size_t p = 4;
    if (carrier[p++] != kVersion) return kNoOffset;
    (void)get_uvar(carrier, p); // source_len
    const uint64_t fc = get_uvar(carrier, p);
    const uint64_t gc = get_uvar(carrier, p);
    for (uint64_t i = 0; i < fc; ++i) (void)get_uvar(carrier, p); // frame_group
    for (uint64_t gi = 0; gi < gc; ++gi) {
        if (p >= carrier.size()) return kNoOffset;
        const uint8_t kind = carrier[p++];
        (void)get_uvar(carrier, p); // member_count
        if (kind == static_cast<uint8_t>(GroupKind::Raw)) continue;
        const uint64_t slots = get_uvar(carrier, p);
        for (uint64_t i = 0; i <= slots; ++i) {
            const uint64_t n = get_uvar(carrier, p);
            p += static_cast<size_t>(n);
        }
        if (slots > 0) return p; // leaf ids begin here
    }
    return kNoOffset;
}

static void selftest() {
    leaf_selftests();
    region_fixture(
        "{\"a\":1,\"b\":\"x\",\"n\":null}\n"
        "{\"a\":2,\"b\":\"y\",\"n\":null}\n"
        "{\"a\":3,\"b\":\"x\",\"n\":null}\n"
        "{\"a\":4,\"b\":\"y\",\"n\":null}\n",
        "all-valid LF");

    // 2. all-valid CRLF NDJSON (CRLF stays inside the frame)
    region_fixture(
        "{\"a\":1,\"b\":\"x\"}\r\n"
        "{\"a\":2,\"b\":\"y\"}\r\n"
        "{\"a\":3,\"b\":\"x\"}\r\n",
        "all-valid CRLF");

    // 3. valid + malformed middle line + valid later records
    region_fixture(
        "{\"a\":1}\n"
        "{\"a\":02}\n"
        "{\"a\":3}\n",
        "malformed middle");

    // 4. valid + incomplete final fragment (no terminator)
    region_fixture(
        "{\"a\":1}\n"
        "{\"a\":2}\n"
        "{\"a\":",
        "incomplete final");

    // 5. blank lines between records (blank frames are raw residuals)
    region_fixture(
        "{\"a\":1}\n"
        "\n"
        "{\"a\":2}\n"
        "\n",
        "blank lines");

    // 6. all-invalid / raw input
    region_fixture(
        "not json at all\n"
        "still not json\n",
        "all-invalid");

    // 7. one structured record only, no trailing newline
    region_fixture("{\"only\":true}", "single record EOF");

    // 8. mixed shapes
    region_fixture(
        "{\"a\":1,\"b\":\"x\"}\n"
        "{\"k\":[1,2,{\"z\":\"a\"}]}\n"
        "{\"a\":9,\"b\":\"y\"}\n",
        "mixed shapes");

    // 9. every G2 leaf exercised via forced constructibility
    {
        const Bytes src = bytes(
            "{\"i\":1,\"f\":100,\"d\":[100,102,105],\"s\":\"a\"}\n"
            "{\"i\":2,\"f\":200,\"d\":[200,202,205],\"s\":\"a\"}\n"
            "{\"i\":3,\"f\":300,\"d\":[300,302,305],\"s\":\"b\"}\n");
        RegionAnalysis a = analyze_regions(src);
        build_structured_candidates(a);
        uint64_t seen = 0;
        for (const ShapePlan& sh : a.shapes)
            for (const SlotPlan& sp : sh.slots)
                for (const auto& cand : sp.candidates)
                    seen |= (uint64_t{1} << static_cast<uint8_t>(cand.id));
        if ((seen & 0x1fu) != 0x1fu)
            throw std::runtime_error("not every G2 leaf was constructible in fixture");
    }

    // 10. INT64 extremes through a full region carrier
    region_fixture(
        "{\"v\":-9223372036854775808}\n"
        "{\"v\":9223372036854775807}\n",
        "int64 extremes");

    // 11. delta/DoD overflow ineligibility is covered in leaf_selftests; verify
    // through a region carrier that such a column still round-trips (falls back
    // to a safe leaf).
    region_fixture(
        "{\"v\":[0,-9223372036854775808,-1]}\n"
        "{\"v\":[0,-9223372036854775808,-1]}\n",
        "delta overflow column");

    // 12. malformed / truncated residual payload
    {
        const Bytes src = bytes("bad\n{\"a\":1}\n");
        RegionAnalysis a = analyze_regions(src);
        build_structured_candidates(a);
        const auto sel = local_selection(a, {LeafId::RawLex});
        Selection s;
        s.structured_shapes = sel;
        CarrierStats st;
        const Bytes c = make_region_carrier(src, a, s, st);
        Bytes bad = c;
        bad.resize(bad.size() - 1);
        require_throw([&]{ (void)decode_region_carrier(bad); }, "truncated residual");
    }

    // 13. malformed reconstruction-order stream (out-of-range group id)
    {
        const Bytes src = bytes("{\"a\":1}\n{\"a\":2}\n");
        RegionAnalysis a = analyze_regions(src);
        build_structured_candidates(a);
        const auto sel = local_selection(a, {LeafId::RawLex});
        Selection s;
        s.structured_shapes = sel;
        CarrierStats st;
        const Bytes c = make_region_carrier(src, a, s, st);
        Bytes bad = c;
        size_t q = 4;
        if (bad[q++] != kVersion) throw std::runtime_error("fixture version");
        (void)get_uvar(bad, q); // source_len
        (void)get_uvar(bad, q); // frame_count
        const uint64_t gc = get_uvar(bad, q); // group_count
        if (gc < 2) bad[q] = 0x7f; // frame_group[0] out of range
        else bad[q] = 0x7e;
        require_throw([&]{ (void)decode_region_carrier(bad); }, "bad frame group id");
    }

    // 14. impossible structured/residual counts (group_count > frame_count)
    {
        const Bytes src = bytes("{\"a\":1}\n");
        RegionAnalysis a = analyze_regions(src);
        build_structured_candidates(a);
        const auto sel = local_selection(a, {LeafId::RawLex});
        Selection s;
        s.structured_shapes = sel;
        CarrierStats st;
        const Bytes c = make_region_carrier(src, a, s, st);
        Bytes bad = c;
        size_t q = 4;
        ++q; // version
        (void)get_uvar(bad, q); // source_len
        (void)get_uvar(bad, q); // frame_count
        const size_t gpos = q;
        (void)get_uvar(bad, q); // group_count
        bad[gpos] = 0x7f; // group_count = 127 > frame_count 1
        require_throw([&]{ (void)decode_region_carrier(bad); }, "impossible group count");
    }

    // 15. bad frame/group length metadata (template part length exceeds source)
    {
        const Bytes src = bytes("{\"a\":1}\n{\"a\":2}\n");
        RegionAnalysis a = analyze_regions(src);
        build_structured_candidates(a);
        const auto sel = local_selection(a, {LeafId::RawLex});
        Selection s;
        s.structured_shapes = sel;
        CarrierStats st;
        const Bytes c = make_region_carrier(src, a, s, st);
        Bytes bad = c;
        size_t q = 4;
        ++q; // version
        (void)get_uvar(bad, q); // source_len
        (void)get_uvar(bad, q); // frame_count
        (void)get_uvar(bad, q); // group_count
        // frame_group[0], frame_group[1]
        (void)get_uvar(bad, q);
        (void)get_uvar(bad, q);
        // structured group kind, member_count, slot_count, then first part_len
        if (q >= bad.size()
            || bad[q++] != static_cast<uint8_t>(GroupKind::Structured))
            throw std::runtime_error("fixture structured group kind");
        (void)get_uvar(bad, q); // member_count
        (void)get_uvar(bad, q); // slot_count
        const size_t part_len_pos = q;
        (void)get_uvar(bad, q); // first part_len
        // Force an oversized part length by rewriting the first byte to 0x7f.
        bad[part_len_pos] = 0x7f;
        require_throw([&]{ (void)decode_region_carrier(bad); }, "bad part length");
    }

    // 16. bad dictionary ID/width covered by leaf tests; also bad leaf id here.
    {
        const Bytes src = bytes("{\"a\":1}\n{\"a\":2}\n");
        RegionAnalysis a = analyze_regions(src);
        build_structured_candidates(a);
        const auto sel = local_selection(a, {LeafId::RawLex});
        Selection s;
        s.structured_shapes = sel;
        CarrierStats st;
        const Bytes c = make_region_carrier(src, a, s, st);
        // Locate the first structured leaf-id byte by walking the wire.
        const size_t leaf_id_pos = first_structured_leaf_id_offset(c);
        if (leaf_id_pos == kNoOffset)
            throw std::runtime_error("no structured leaf id located");
        Bytes bad = c;
        bad[leaf_id_pos] = 5; // invalid leaf id
        require_throw([&]{ (void)decode_region_carrier(bad); }, "bad leaf id");
    }

    // 17. bad bitpack high bits covered by leaf test.
    // 18. noncanonical varints
    {
        Bytes noncanon{0x80, 0x00};
        size_t p = 0;
        require_throw([&]{ (void)get_uvar(noncanon, p); }, "noncanonical uvar");
    }
    // 19. trailing carrier bytes
    {
        const Bytes src = bytes("{\"a\":1}\n{\"a\":2}\n");
        RegionAnalysis a = analyze_regions(src);
        build_structured_candidates(a);
        const auto sel = local_selection(a, {LeafId::RawLex});
        Selection s;
        s.structured_shapes = sel;
        CarrierStats st;
        const Bytes c = make_region_carrier(src, a, s, st);
        Bytes bad = c;
        bad.push_back(0x00);
        require_throw([&]{ (void)decode_region_carrier(bad); }, "trailing carrier bytes");
    }

    // 20. Carrier-bug regression fixtures (audited).
    // 20a. fully-valid single structured shape, NO raw group (group 0 is the
    //      first structured shape; order stream must not assume a raw group).
    {
        const Bytes src = bytes(
            "{\"a\":1}\n"
            "{\"a\":2}\n"
            "{\"a\":3}\n");
        RegionAnalysis a = analyze_regions(src);
        build_structured_candidates(a);
        if (a.raw_members.size() != 0) throw std::runtime_error("20a: unexpected raw frames");
        if (a.shapes.size() != 1) throw std::runtime_error("20a: expected one shape");
        const auto sel = local_selection(a, {LeafId::RawLex});
        Selection s;
        s.structured_shapes = sel;
        CarrierStats st;
        const Bytes c = make_region_carrier(src, a, s, st);
        if (decode_region_carrier(c) != src) throw std::runtime_error("20a: no-raw roundtrip");
    }
    // 20b. mixed raw + structured (raw frames both before and after valid ones).
    region_fixture(
        "not json\n"
        "{\"a\":1}\n"
        "{\"a\":2}\n"
        "also not json\n"
        "{\"a\":3}\n",
        "mixed raw+structured");
    // 20c. valid zero-scalar {} frame, no raw residuals.
    {
        const Bytes src = bytes("{}\n{}\n");
        RegionAnalysis a = analyze_regions(src);
        build_structured_candidates(a);
        if (a.raw_members.size() != 0) throw std::runtime_error("20c: {} wrongly raw");
        if (a.shapes.size() != 1 || a.shapes[0].slots.size() != 0)
            throw std::runtime_error("20c: {} zero-scalar shape expected");
        const auto sel = local_selection(a, {LeafId::RawLex});
        Selection s;
        s.structured_shapes = sel;
        CarrierStats st;
        const Bytes c = make_region_carrier(src, a, s, st);
        if (decode_region_carrier(c) != src) throw std::runtime_error("20c: {} roundtrip");
    }
    // 20d. valid zero-scalar [] frame, no raw residuals.
    {
        const Bytes src = bytes("[]\n[]\n[]\n");
        RegionAnalysis a = analyze_regions(src);
        build_structured_candidates(a);
        if (a.raw_members.size() != 0) throw std::runtime_error("20d: [] wrongly raw");
        const auto sel = local_selection(a, {LeafId::RawLex});
        Selection s;
        s.structured_shapes = sel;
        CarrierStats st;
        const Bytes c = make_region_carrier(src, a, s, st);
        if (decode_region_carrier(c) != src) throw std::runtime_error("20d: [] roundtrip");
    }
    // 20e. zero-scalar {} / [] mixed WITH raw residual frames.
    region_fixture(
        "{}\n"
        "garbage\n"
        "[]\n"
        "{}\n",
        "zero-scalar + raw");
    // 20f. raw-only input (single raw group, one shape count of zero).
    {
        const Bytes src = bytes("nope\nnope2\n");
        RegionAnalysis a = analyze_regions(src);
        build_structured_candidates(a);
        if (a.shapes.size() != 0) throw std::runtime_error("20f: expected no shapes");
        if (a.raw_members.size() != 2) throw std::runtime_error("20f: expected two raw frames");
        Selection s; // no structured shapes
        CarrierStats st;
        const Bytes c = make_region_carrier(src, a, s, st);
        if (decode_region_carrier(c) != src) throw std::runtime_error("20f: raw-only roundtrip");
    }

    // 21. G2 control parsing is fail-closed (no silent fallback).
    {
        const std::string good =
            "{\"schema\":2,\"source_bytes\":29,\"raw_complete_bytes\":37,"
            "\"selected_bytes\":30,\"selected_arm\":\"P-DICT\",\"structured_eligible\":true}";
        const G2Control g = parse_g2_control(good);
        if (!g.provided || !g.eligible || g.selected_bytes != 30 || g.selected_arm != "P-DICT")
            throw std::runtime_error("21: valid control misparsed");

        // Wrong schema, missing field, unknown arm, and selected>raw all reject.
        require_throw([&]{
            (void)parse_g2_control(
                "{\"schema\":1,\"source_bytes\":1,\"raw_complete_bytes\":1,"
                "\"selected_bytes\":1,\"selected_arm\":\"RAW_BROTLI\"}");
        }, "control wrong schema");
        require_throw([&]{
            (void)parse_g2_control(
                "{\"schema\":2,\"source_bytes\":1,\"raw_complete_bytes\":1,"
                "\"selected_arm\":\"RAW_BROTLI\"}");
        }, "control missing selected_bytes");
        require_throw([&]{
            (void)parse_g2_control(
                "{\"schema\":2,\"source_bytes\":1,\"raw_complete_bytes\":1,"
                "\"selected_bytes\":1,\"selected_arm\":\"P-FLOAT\"}");
        }, "control unknown arm");
        require_throw([&]{
            (void)parse_g2_control(
                "{\"schema\":2,\"source_bytes\":1,\"raw_complete_bytes\":5,"
                "\"selected_bytes\":9,\"selected_arm\":\"P-DICT\",\"structured_eligible\":true}");
        }, "control selected exceeds raw");
        // Eligibility means the structured expert was available; raw may still
        // legitimately win final G2 arbitration.
        const G2Control raw_wins = parse_g2_control(
            "{\"schema\":2,\"source_bytes\":1,\"raw_complete_bytes\":5,"
            "\"selected_bytes\":5,\"selected_arm\":\"RAW_BROTLI\",\"structured_eligible\":true}");
        if (!raw_wins.provided || !raw_wins.eligible || raw_wins.selected_arm != "RAW_BROTLI")
            throw std::runtime_error("21: eligible raw-winning control misparsed");
        require_throw([&]{
            (void)parse_g2_control(
                "{\"schema\":2,\"source_bytes\":1,\"raw_complete_bytes\":5,"
                "\"selected_bytes\":5,\"selected_arm\":\"RAW_BROTLI\"}");
        }, "control missing structured_eligible");
        require_throw([&]{
            (void)parse_g2_control(
                "{\"schema\":2,\"source_bytes\":1,\"raw_complete_bytes\":5,"
                "\"selected_bytes\":4,\"selected_arm\":\"P-DICT\",\"structured_eligible\":false}");
        }, "ineligible control selected non-raw");
    }

    // Raw Brotli sanity.
    const Bytes raw = bytes("hello hello hello");
    const Bytes br = brotli_encode(raw);
    if (brotli_decode_exact(br, raw.size()) != raw)
        throw std::runtime_error("raw Brotli selftest");

    std::cout << "PASS grotli_g3 selftest\n";
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "selftest") { selftest(); return 0; }
        if (argc >= 3 && std::string(argv[1]) == "measure") {
            const std::string path = argv[2];
            G2Control g2;
            for (int i = 3; i < argc; ++i) {
                const std::string arg = argv[i];
                if (arg == "--g2-result") {
                    if (i + 1 >= argc) throw std::runtime_error("--g2-result needs a path");
                    g2 = parse_g2_control(read_file_text(argv[++i]));
                } else {
                    throw std::runtime_error("unknown argument: " + arg);
                }
            }
            return measure(path, g2);
        }
        std::cerr << "usage: grotli_g3 selftest\n"
                     "       grotli_g3 measure INPUT [--g2-result ROW.json]\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
