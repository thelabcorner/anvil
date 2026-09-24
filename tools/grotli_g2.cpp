// ANVIL I10 GROTLI G2 typed-column standalone prototype.
//
// Frozen semantics:
//   docs/I10-GROTLI-G2-TYPED-PREREG.md
//   docs/I10-GROTLI-G2-CORPUS-FREEZE.md
//
// Research tooling only. This does not change ANVIL's production wire.
//
// Build:
//   clang++ -O3 -DNDEBUG -std=c++20 tools/grotli_g2.cpp \
//     -lbrotlienc -lbrotlidec -lbrotlicommon -o grotli_g2
//
// Usage:
//   grotli_g2 measure INPUT
//   grotli_g2 selftest
//
// Frozen leaf basis:
//   0 RAW_LEX
//   1 EXACT_DICT
//   2 INT_FOR
//   3 INT_DELTA_FOR
//   4 INT_DOD_FOR
//
// Final arms:
//   RAW_BROTLI, G1R, P-DICT, P-INT, P-MIXED, P-MARGINAL.
//
// Common prototype envelope cost for every archive arm:
//   arm:u8 | decoded_source_len:uvar | brotli_payload
//
// The structured carrier contains every decoder-visible byte required to
// reconstruct the source. Planner/search state is encoder-only and uncharged
// because the decoder never needs it.

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
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using Bytes = std::vector<uint8_t>;
using Clock = std::chrono::steady_clock;

static constexpr uint8_t kMagic[4] = {'G','2','S','R'};
static constexpr uint8_t kVersion = 1;
// Every archive arm is charged the same one-byte external arm selector.
// The research tool only needs the common byte cost, not materialized arm IDs.
static constexpr uint64_t kMaxDecoded = 1ull << 34; // research-wire safety bound
static constexpr uint64_t kMaxCarrierSlack = 64ull << 20;
static constexpr size_t kMarginalTopK = 24;
static constexpr size_t kMarginalMaxAccepted = 8;

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
    return "UNKNOWN";
}

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

struct Span {
    size_t lo = 0;
    size_t hi = 0;
};

struct RecordExtent {
    size_t lo = 0;
    size_t hi = 0;
};

static std::vector<RecordExtent> split_records(const Bytes& src) {
    std::vector<RecordExtent> out;
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
        : src_(src), lo_(lo), hi_(hi), p_(lo) {}

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
    size_t lo_, hi_, p_;
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

struct ParsedRecord {
    RecordExtent extent;
    std::vector<Span> values;
    uint32_t shape_id = 0;
};

struct Shape {
    std::vector<Bytes> parts;
    std::vector<size_t> members; // record indexes, in source order
};

struct Analysis {
    std::vector<ParsedRecord> records;
    std::vector<Shape> shapes;
    uint64_t scalar_count = 0;
    uint64_t scalar_bytes = 0;
    uint64_t structural_bytes = 0;
    uint64_t unique_template_bytes = 0;
    uint64_t gross_template_bytes_avoided = 0;
};

static std::string shape_key(const std::vector<Bytes>& parts) {
    Bytes key;
    put_uvar(key, parts.size());
    for (const auto& part : parts) {
        put_uvar(key, part.size());
        key.insert(key.end(), part.begin(), part.end());
    }
    return std::string(reinterpret_cast<const char*>(key.data()), key.size());
}

static Analysis analyze(const Bytes& src) {
    if (src.empty()) throw std::runtime_error("structured candidate unavailable: empty input");
    const auto extents = split_records(src);
    if (extents.empty()) throw std::runtime_error("structured candidate unavailable: no records");

    Analysis a;
    a.records.reserve(extents.size());
    std::unordered_map<std::string, uint32_t> ids;
    ids.reserve(extents.size());

    for (const auto& ex : extents) {
        JsonLexParser parser(src, ex.lo, ex.hi);
        auto values = parser.parse();
        ParsedRecord rec{ex, std::move(values), 0};

        std::vector<Bytes> parts;
        parts.reserve(rec.values.size() + 1);
        size_t cursor = ex.lo;
        uint64_t rec_scalar = 0;
        for (const Span& s : rec.values) {
            if (s.lo < cursor || s.hi < s.lo || s.hi > ex.hi)
                throw std::runtime_error("scalar span ordering bug");
            parts.emplace_back(src.begin() + cursor, src.begin() + s.lo);
            rec_scalar += s.hi - s.lo;
            cursor = s.hi;
        }
        parts.emplace_back(src.begin() + cursor, src.begin() + ex.hi);

        const std::string key = shape_key(parts);
        auto it = ids.find(key);
        uint32_t sid;
        if (it == ids.end()) {
            if (a.shapes.size() >= std::numeric_limits<uint32_t>::max())
                throw std::runtime_error("too many shapes");
            sid = static_cast<uint32_t>(a.shapes.size());
            ids.emplace(key, sid);
            Shape sh;
            sh.parts = std::move(parts);
            a.shapes.push_back(std::move(sh));
        } else {
            sid = it->second;
        }

        rec.shape_id = sid;
        const size_t rid = a.records.size();
        a.shapes[sid].members.push_back(rid);
        a.scalar_count += rec.values.size();
        a.scalar_bytes += rec_scalar;
        a.records.push_back(std::move(rec));
    }

    if (a.scalar_bytes > src.size()) throw std::runtime_error("scalar accounting overflow");
    a.structural_bytes = src.size() - a.scalar_bytes;
    for (const Shape& sh : a.shapes)
        for (const Bytes& part : sh.parts)
            a.unique_template_bytes += part.size();
    if (a.unique_template_bytes > a.structural_bytes)
        throw std::runtime_error("template accounting bug");
    a.gross_template_bytes_avoided = a.structural_bytes - a.unique_template_bytes;
    return a;
}

struct CarrierStats {
    uint64_t header_bytes = 0;
    uint64_t shape_dictionary_bytes = 0;
    uint64_t template_bytes = 0;
    uint64_t shape_id_bytes = 0;
    uint64_t leaf_descriptor_bytes = 0;
    uint64_t leaf_payload_bytes = 0;
    uint64_t raw_leaf_count = 0;
    uint64_t dict_leaf_count = 0;
    uint64_t int_for_leaf_count = 0;
    uint64_t int_delta_leaf_count = 0;
    uint64_t int_dod_leaf_count = 0;
};

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

struct LeafCandidate {
    LeafId id = LeafId::RawLex;
    Bytes payload;
    size_t isolated_brotli_bytes = 0;
};

struct ColumnPlan {
    uint32_t shape_id = 0;
    uint32_t slot_id = 0;
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

static const LeafCandidate* candidate_by_id(const ColumnPlan& c, LeafId id) {
    for (const auto& x : c.candidates) if (x.id == id) return &x;
    return nullptr;
}

static std::vector<ColumnPlan> build_columns(const Bytes& src, const Analysis& a) {
    std::vector<ColumnPlan> cols;
    size_t total_cols = 0;
    for (const Shape& sh : a.shapes) total_cols += sh.parts.size() - 1;
    cols.reserve(total_cols);
    for (size_t sid = 0; sid < a.shapes.size(); ++sid) {
        const Shape& sh = a.shapes[sid];
        const size_t slots = sh.parts.size() - 1;
        for (size_t slot = 0; slot < slots; ++slot) {
            ColumnPlan c;
            c.shape_id = static_cast<uint32_t>(sid);
            c.slot_id = static_cast<uint32_t>(slot);
            c.tokens.reserve(sh.members.size());
            std::unordered_map<std::string, uint8_t> distinct;
            distinct.reserve(sh.members.size());
            std::vector<int64_t> ints;
            ints.reserve(sh.members.size());
            bool all_int = true;
            for (size_t rid : sh.members) {
                const Span s = a.records[rid].values[slot];
                Bytes tok(src.begin() + s.lo, src.begin() + s.hi);
                c.token_bytes += tok.size();
                distinct.emplace(std::string(reinterpret_cast<const char*>(tok.data()), tok.size()), 0);
                int64_t v = 0;
                if (parse_canonical_int64(tok, v)) ints.push_back(v);
                else all_int = false;
                c.tokens.push_back(std::move(tok));
            }
            c.distinct_tokens = distinct.size();
            auto add = [&](LeafId id, Bytes payload) {
                LeafCandidate x;
                x.id = id;
                x.payload = std::move(payload);
                x.isolated_brotli_bytes =
                    brotli_encode(make_leaf_object(id, c.tokens.size(), x.payload)).size();
                c.candidates.push_back(std::move(x));
            };
            add(LeafId::RawLex, make_raw_payload(c.tokens));
            add(LeafId::ExactDict, make_dict_payload(c.tokens));
            c.canonical_int = all_int && ints.size() == c.tokens.size();
            if (c.canonical_int) {
                const auto mm = std::minmax_element(ints.begin(), ints.end());
                c.int_min = *mm.first;
                c.int_max = *mm.second;
                if (auto p = make_int_for_payload(ints)) add(LeafId::IntFor, std::move(*p));
                int64_t dmin = 0, dmax = 0;
                if (auto p = make_int_delta_payload(ints, &dmin, &dmax)) {
                    c.delta_eligible = true; c.delta_min = dmin; c.delta_max = dmax;
                    add(LeafId::IntDeltaFor, std::move(*p));
                }
                int64_t ddmin = 0, ddmax = 0;
                if (auto p = make_int_dod_payload(ints, &ddmin, &ddmax)) {
                    c.dod_eligible = true; c.dod_min = ddmin; c.dod_max = ddmax;
                    add(LeafId::IntDodFor, std::move(*p));
                }
            }
            cols.push_back(std::move(c));
        }
    }
    return cols;
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
    throw std::runtime_error("unknown G2 leaf id");
}

static Bytes make_carrier(
    const Bytes& src, const Analysis& a, const std::vector<ColumnPlan>& cols,
    const std::vector<LeafId>& selected, CarrierStats& st)
{
    if (cols.size() != selected.size()) throw std::runtime_error("G2 selection size mismatch");
    Bytes out;
    out.insert(out.end(), std::begin(kMagic), std::end(kMagic));
    out.push_back(kVersion);
    put_uvar(out, src.size());
    put_uvar(out, a.records.size());
    put_uvar(out, a.shapes.size());
    st.header_bytes = out.size();
    const size_t dict_begin = out.size();
    for (const Shape& sh : a.shapes) {
        put_uvar(out, sh.members.size());
        const size_t slots = sh.parts.empty() ? 0 : sh.parts.size() - 1;
        put_uvar(out, slots);
        for (const Bytes& part : sh.parts) {
            put_uvar(out, part.size());
            st.template_bytes += part.size();
            out.insert(out.end(), part.begin(), part.end());
        }
    }
    st.shape_dictionary_bytes = out.size() - dict_begin;
    const size_t sid_begin = out.size();
    for (const ParsedRecord& r : a.records) put_uvar(out, r.shape_id);
    st.shape_id_bytes = out.size() - sid_begin;

    size_t ci = 0;
    for (size_t sid = 0; sid < a.shapes.size(); ++sid) {
        const size_t slots = a.shapes[sid].parts.size() - 1;
        for (size_t slot = 0; slot < slots; ++slot, ++ci) {
            if (ci >= cols.size() || cols[ci].shape_id != sid || cols[ci].slot_id != slot)
                throw std::runtime_error("G2 column ordering mismatch");
            const LeafCandidate* cand = candidate_by_id(cols[ci], selected[ci]);
            if (!cand) throw std::runtime_error("selected ineligible G2 leaf");
            const size_t before = out.size();
            out.push_back(static_cast<uint8_t>(cand->id));
            put_uvar(out, cand->payload.size());
            st.leaf_descriptor_bytes += out.size() - before;
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
    if (ci != cols.size()) throw std::runtime_error("G2 unused columns");
    return out;
}

struct CSpan { uint32_t off = 0; uint32_t len = 0; };
struct DShape {
    uint32_t occurrences = 0;
    uint32_t slots = 0;
    std::vector<CSpan> parts;
    std::vector<std::vector<Bytes>> columns;
};

static CSpan take_span(const Bytes& carrier, size_t& p, uint64_t n, uint64_t max_n) {
    if (n > max_n || n > carrier.size() - p) throw std::runtime_error("carrier span out of range");
    if (p > std::numeric_limits<uint32_t>::max() || n > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("carrier span exceeds prototype offset width");
    CSpan s{static_cast<uint32_t>(p), static_cast<uint32_t>(n)};
    p += static_cast<size_t>(n);
    return s;
}

static Bytes decode_carrier(const Bytes& carrier) {
    if (carrier.size() < 5 || std::memcmp(carrier.data(), kMagic, 4) != 0)
        throw std::runtime_error("bad G2 carrier magic");
    size_t p = 4;
    if (carrier[p++] != kVersion) throw std::runtime_error("bad G2 carrier version");
    const uint64_t decoded_len = get_uvar(carrier, p);
    const uint64_t record_count = get_uvar(carrier, p);
    const uint64_t shape_count = get_uvar(carrier, p);
    if (decoded_len == 0 || decoded_len > kMaxDecoded || decoded_len > std::numeric_limits<size_t>::max())
        throw std::runtime_error("bad G2 decoded length");
    if (record_count == 0 || record_count > decoded_len || record_count > std::numeric_limits<uint32_t>::max()
        || record_count > carrier.size())
        throw std::runtime_error("bad G2 record count");
    if (shape_count == 0 || shape_count > record_count || shape_count > std::numeric_limits<uint32_t>::max()
        || shape_count > carrier.size() / 3)
        throw std::runtime_error("bad G2 shape count");
    if (carrier.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("G2 carrier exceeds prototype offset width");

    std::vector<DShape> shapes(static_cast<size_t>(shape_count));
    uint64_t occurrence_sum = 0, template_sum = 0, scalar_count = 0;
    for (DShape& sh : shapes) {
        const uint64_t occ = get_uvar(carrier, p);
        const uint64_t slots = get_uvar(carrier, p);
        if (occ == 0 || occ > record_count || slots > decoded_len)
            throw std::runtime_error("bad G2 shape dimensions");
        if (occurrence_sum > record_count - occ) throw std::runtime_error("G2 occurrence sum overflow");
        occurrence_sum += occ;
        if (slots && occ > decoded_len / slots) throw std::runtime_error("G2 scalar count bound");
        const uint64_t values = occ * slots;
        if (scalar_count > decoded_len - values) throw std::runtime_error("too many G2 scalar values");
        scalar_count += values;
        if (scalar_count > carrier.size()) throw std::runtime_error("G2 scalar count exceeds carrier capacity");
        if (slots + 1 > carrier.size() - p) throw std::runtime_error("G2 part count exceeds carrier capacity");
        sh.occurrences = static_cast<uint32_t>(occ);
        sh.slots = static_cast<uint32_t>(slots);
        sh.parts.reserve(static_cast<size_t>(slots + 1));
        for (uint64_t i = 0; i <= slots; ++i) {
            const uint64_t n = get_uvar(carrier, p);
            if (n > decoded_len) throw std::runtime_error("G2 template part too large");
            if (template_sum > decoded_len - n) throw std::runtime_error("G2 template sum overflow");
            template_sum += n;
            sh.parts.push_back(take_span(carrier, p, n, decoded_len));
        }
        sh.columns.resize(static_cast<size_t>(slots));
    }
    if (occurrence_sum != record_count) throw std::runtime_error("G2 occurrence count mismatch");

    std::vector<uint32_t> shape_ids;
    shape_ids.reserve(static_cast<size_t>(record_count));
    std::vector<uint32_t> freq(static_cast<size_t>(shape_count), 0);
    for (uint64_t i = 0; i < record_count; ++i) {
        const uint64_t sid = get_uvar(carrier, p);
        if (sid >= shape_count) throw std::runtime_error("G2 shape id out of range");
        shape_ids.push_back(static_cast<uint32_t>(sid));
        ++freq[static_cast<size_t>(sid)];
    }
    for (size_t i = 0; i < shapes.size(); ++i)
        if (freq[i] != shapes[i].occurrences) throw std::runtime_error("G2 shape frequency mismatch");

    for (DShape& sh : shapes) {
        for (size_t slot = 0; slot < sh.slots; ++slot) {
            if (p >= carrier.size()) throw std::runtime_error("truncated G2 leaf id");
            const uint8_t raw_id = carrier[p++];
            if (raw_id > static_cast<uint8_t>(LeafId::IntDodFor))
                throw std::runtime_error("unknown G2 leaf id");
            const uint64_t n = get_uvar(carrier, p);
            if (n > carrier.size() - p) throw std::runtime_error("G2 leaf payload truncated");
            Bytes payload(carrier.begin() + p, carrier.begin() + p + static_cast<size_t>(n));
            p += static_cast<size_t>(n);
            sh.columns[slot] = decode_leaf_payload(
                static_cast<LeafId>(raw_id), payload, sh.occurrences, decoded_len);
        }
    }
    if (p != carrier.size()) throw std::runtime_error("G2 carrier trailing bytes");

    Bytes out;
    out.reserve(static_cast<size_t>(decoded_len));
    std::vector<uint32_t> cursor(shapes.size(), 0);
    auto append_span = [&](CSpan s) {
        if (s.len > decoded_len - out.size()) throw std::runtime_error("G2 reconstructed size overflow");
        out.insert(out.end(), carrier.begin() + s.off, carrier.begin() + s.off + s.len);
    };
    auto append_tok = [&](const Bytes& tok) {
        if (tok.size() > decoded_len - out.size()) throw std::runtime_error("G2 reconstructed token overflow");
        out.insert(out.end(), tok.begin(), tok.end());
    };
    for (uint32_t sid : shape_ids) {
        DShape& sh = shapes[sid];
        const uint32_t oi = cursor[sid]++;
        if (oi >= sh.occurrences) throw std::runtime_error("G2 shape cursor overflow");
        for (size_t slot = 0; slot < sh.slots; ++slot) {
            append_span(sh.parts[slot]);
            append_tok(sh.columns[slot][oi]);
        }
        append_span(sh.parts[sh.slots]);
    }
    if (out.size() != decoded_len) throw std::runtime_error("G2 reconstructed length mismatch");
    return out;
}
static double ms_since(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
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

static double delta_pct(size_t candidate, size_t control) {
    return control ? (double(candidate) / double(control) - 1.0) * 100.0 : 0.0;
}

struct ShapeCoverage {
    uint64_t singleton = 0;
    uint64_t top1 = 0, top5 = 0, top10 = 0, top100 = 0;
    uint64_t median = 0, p95 = 0, max = 0;
};

static ShapeCoverage shape_coverage(const Analysis& a) {
    std::vector<uint64_t> counts;
    counts.reserve(a.shapes.size());
    for (const Shape& sh : a.shapes) counts.push_back(sh.members.size());
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

static size_t complete_bytes(size_t source_len, const Bytes& brotli) {
    return 1 + uvar_len(source_len) + brotli.size();
}

static std::vector<LeafId> all_raw_selection(const std::vector<ColumnPlan>& cols) {
    return std::vector<LeafId>(cols.size(), LeafId::RawLex);
}

static bool leaf_allowed(LeafId id, const std::vector<LeafId>& allowed) {
    return std::find(allowed.begin(), allowed.end(), id) != allowed.end();
}

static std::vector<LeafId> local_selection(
    const std::vector<ColumnPlan>& cols,
    const std::vector<LeafId>& allowed)
{
    std::vector<LeafId> out(cols.size(), LeafId::RawLex);
    for (size_t i = 0; i < cols.size(); ++i) {
        const LeafCandidate* raw = candidate_by_id(cols[i], LeafId::RawLex);
        if (!raw) throw std::runtime_error("column missing RAW_LEX");
        const LeafCandidate* best = raw;
        for (const auto& c : cols[i].candidates) {
            if (!leaf_allowed(c.id, allowed)) continue;
            if (c.isolated_brotli_bytes < best->isolated_brotli_bytes) best = &c;
        }
        out[i] = best->id; // RAW wins exact ties because best starts at RAW and only strict < replaces.
    }
    return out;
}

struct ArmResult {
    std::string name;
    std::vector<LeafId> selection;
    Bytes carrier;
    Bytes brotli;
    CarrierStats stats;
    size_t complete = std::numeric_limits<size_t>::max();
    double build_ms = 0;
    double encode_ms = 0;
    double decode_ms = 0;
    bool roundtrip = false;
};

static ArmResult evaluate_arm(
    const std::string& name,
    const Bytes& src,
    const Analysis& a,
    const std::vector<ColumnPlan>& cols,
    const std::vector<LeafId>& selection)
{
    ArmResult r;
    r.name = name;
    r.selection = selection;
    auto t0 = Clock::now();
    r.carrier = make_carrier(src, a, cols, selection, r.stats);
    r.build_ms = ms_since(t0);

    t0 = Clock::now();
    r.brotli = brotli_encode(r.carrier);
    r.encode_ms = ms_since(t0);
    r.complete = complete_bytes(src.size(), r.brotli);

    const uint64_t bound64 = std::min<uint64_t>(
        std::numeric_limits<size_t>::max(),
        std::min<uint64_t>(
            kMaxDecoded,
            uint64_t(src.size()) * 4ull + kMaxCarrierSlack));

    t0 = Clock::now();
    const Bytes carrier_dec =
        brotli_decode_bounded(r.brotli, static_cast<size_t>(bound64));
    const Bytes decoded = decode_carrier(carrier_dec);
    r.decode_ms = ms_since(t0);
    r.roundtrip = decoded == src;
    if (!r.roundtrip) throw std::runtime_error(name + " roundtrip mismatch");
    return r;
}

struct Substitution {
    size_t column = 0;
    LeafId leaf = LeafId::RawLex;
    int64_t isolated_delta = 0;
    uint32_t shape_id = 0;
    uint32_t slot_id = 0;
};

struct MarginalResult {
    ArmResult arm;
    uint64_t candidate_substitutions = 0;
    uint64_t marginal_candidates_evaluated = 0;
    uint64_t exact_whole_carrier_evaluations = 0;
    std::vector<std::pair<size_t, LeafId>> accepted;
    std::vector<size_t> byte_trace;
    double search_ms = 0;
};

static MarginalResult marginal_search(
    const Bytes& src,
    const Analysis& a,
    const std::vector<ColumnPlan>& cols)
{
    const auto start = Clock::now();
    MarginalResult result;
    std::vector<LeafId> current = all_raw_selection(cols);

    // Search trials need exact whole-carrier q11 bytes, but do not need to
    // decompress every candidate that will be discarded. Final-arm correctness
    // is still verified once through evaluate_arm().
    auto exact_score = [&](const std::vector<LeafId>& selection) {
        CarrierStats st;
        const Bytes carrier = make_carrier(src, a, cols, selection, st);
        const Bytes br = brotli_encode(carrier);
        ++result.exact_whole_carrier_evaluations;
        return complete_bytes(src.size(), br);
    };

    size_t current_complete = exact_score(current);
    result.byte_trace.push_back(current_complete);

    std::vector<Substitution> substitutions;
    for (size_t ci = 0; ci < cols.size(); ++ci) {
        const LeafCandidate* raw = candidate_by_id(cols[ci], LeafId::RawLex);
        if (!raw) throw std::runtime_error("marginal column missing RAW");
        for (const auto& c : cols[ci].candidates) {
            if (c.id == LeafId::RawLex) continue;
            const __int128 d = static_cast<__int128>(c.isolated_brotli_bytes)
                - static_cast<__int128>(raw->isolated_brotli_bytes);
            if (d < std::numeric_limits<int64_t>::min()
                || d > std::numeric_limits<int64_t>::max())
                throw std::runtime_error("isolated delta overflow");
            substitutions.push_back({
                ci, c.id, static_cast<int64_t>(d),
                cols[ci].shape_id, cols[ci].slot_id
            });
        }
    }
    result.candidate_substitutions = substitutions.size();

    auto order = [](const Substitution& x, const Substitution& y) {
        if (x.isolated_delta != y.isolated_delta) return x.isolated_delta < y.isolated_delta;
        if (x.shape_id != y.shape_id) return x.shape_id < y.shape_id;
        if (x.slot_id != y.slot_id) return x.slot_id < y.slot_id;
        return static_cast<uint8_t>(x.leaf) < static_cast<uint8_t>(y.leaf);
    };
    std::sort(substitutions.begin(), substitutions.end(), order);

    for (size_t round = 0; round < kMarginalMaxAccepted; ++round) {
        std::vector<const Substitution*> eligible;
        eligible.reserve(kMarginalTopK);
        for (const auto& s : substitutions) {
            if (current[s.column] != LeafId::RawLex) continue;
            eligible.push_back(&s);
            if (eligible.size() == kMarginalTopK) break;
        }
        if (eligible.empty()) break;

        size_t best_complete = current_complete;
        const Substitution* best_sub = nullptr;

        // eligible is already in the frozen deterministic rank order; strict
        // byte improvement therefore makes that order the exact-tie breaker.
        for (const Substitution* s : eligible) {
            std::vector<LeafId> trial = current;
            trial[s->column] = s->leaf;
            const size_t trial_complete = exact_score(trial);
            ++result.marginal_candidates_evaluated;
            if (trial_complete < best_complete) {
                best_complete = trial_complete;
                best_sub = s;
            }
        }

        if (best_sub == nullptr) break;
        current[best_sub->column] = best_sub->leaf;
        current_complete = best_complete;
        result.accepted.emplace_back(best_sub->column, best_sub->leaf);
        result.byte_trace.push_back(current_complete);
    }

    result.search_ms = ms_since(start);
    result.arm = evaluate_arm("P-MARGINAL", src, a, cols, current);
    if (result.arm.complete != current_complete)
        throw std::runtime_error("P-MARGINAL deterministic score mismatch");
    return result;
}

static std::string leaf_json_name(LeafId id) {
    return leaf_name(id);
}

static void emit_selection_json(
    const char* key,
    const std::vector<ColumnPlan>& cols,
    const std::vector<LeafId>& selection)
{
    std::cout << ",\"" << key << "\":[";
    for (size_t i = 0; i < selection.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "{\"shape_id\":" << cols[i].shape_id
                  << ",\"slot_id\":" << cols[i].slot_id
                  << ",\"leaf\":\"" << leaf_json_name(selection[i]) << "\"}";
    }
    std::cout << "]";
}

static void emit_arm_json(const char* prefix, const ArmResult& r, size_t raw_complete) {
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

static int measure(const std::string& path) {
    const Bytes src = read_file(path);

    const auto raw_t0 = Clock::now();
    const Bytes raw_br = brotli_encode(src);
    const double raw_enc_ms = ms_since(raw_t0);
    const auto raw_d0 = Clock::now();
    const Bytes raw_dec = brotli_decode_exact(raw_br, src.size());
    const double raw_dec_ms = ms_since(raw_d0);
    if (raw_dec != src) throw std::runtime_error("raw Brotli roundtrip mismatch");
    const size_t raw_complete = complete_bytes(src.size(), raw_br);

    bool eligible = true;
    std::string ineligible_reason;
    Analysis a;
    double analyze_ms = 0;
    try {
        const auto t0 = Clock::now();
        a = analyze(src);
        analyze_ms = ms_since(t0);
    } catch (const std::exception& e) {
        eligible = false;
        ineligible_reason = e.what();
    }

    if (!eligible) {
        std::cout << "{"
                  << "\"schema\":2"
                  << ",\"file\":\"" << json_escape(path) << "\""
                  << ",\"source_bytes\":" << src.size()
                  << ",\"structured_eligible\":false"
                  << ",\"ineligible_reason\":\"" << json_escape(ineligible_reason) << "\""
                  << ",\"raw_brotli_bytes\":" << raw_br.size()
                  << ",\"raw_complete_bytes\":" << raw_complete
                  << ",\"raw_encode_ms\":" << std::fixed << std::setprecision(6) << raw_enc_ms
                  << ",\"raw_decode_ms\":" << raw_dec_ms
                  << ",\"raw_roundtrip\":true"
                  << ",\"selected_arm\":\"RAW_BROTLI\""
                  << ",\"selected_bytes\":" << raw_complete
                  << ",\"selected_delta_pct\":0.0"
                  << "}\n";
        return 0;
    }

    const auto planner_t0 = Clock::now();
    std::vector<ColumnPlan> cols = build_columns(src, a);
    const double candidate_build_local_score_ms = ms_since(planner_t0);

    const auto raw_sel = all_raw_selection(cols);
    const auto dict_sel = local_selection(cols, {LeafId::RawLex, LeafId::ExactDict});
    const auto int_sel = local_selection(
        cols, {LeafId::RawLex, LeafId::IntFor, LeafId::IntDeltaFor, LeafId::IntDodFor});
    const auto mixed_sel = local_selection(
        cols, {LeafId::RawLex, LeafId::ExactDict, LeafId::IntFor,
               LeafId::IntDeltaFor, LeafId::IntDodFor});

    ArmResult g1r = evaluate_arm("G1R", src, a, cols, raw_sel);
    ArmResult pdict = evaluate_arm("P-DICT", src, a, cols, dict_sel);
    ArmResult pint = evaluate_arm("P-INT", src, a, cols, int_sel);
    ArmResult pmixed = evaluate_arm("P-MIXED", src, a, cols, mixed_sel);
    MarginalResult marginal = marginal_search(src, a, cols);

    struct Choice { const char* name; size_t bytes; };
    Choice selected{"RAW_BROTLI", raw_complete};
    auto choose = [&](const char* name, size_t n) {
        if (n < selected.bytes) selected = {name, n};
    };
    choose("G1R", g1r.complete);
    choose("P-DICT", pdict.complete);
    choose("P-INT", pint.complete);
    choose("P-MIXED", pmixed.complete);
    choose("P-MARGINAL", marginal.arm.complete);

    uint64_t typed_cols = 0;
    uint64_t isolated_q11_evals = 0;
    for (const auto& c : cols) {
        isolated_q11_evals += c.candidates.size();
        if (c.canonical_int || c.distinct_tokens < c.tokens.size()) ++typed_cols;
    }

    const ShapeCoverage cov = shape_coverage(a);

    std::cout << "{";
    std::cout << "\"schema\":2";
    std::cout << ",\"file\":\"" << json_escape(path) << "\"";
    std::cout << ",\"source_bytes\":" << src.size();
    std::cout << ",\"structured_eligible\":true";
    std::cout << ",\"raw_brotli_bytes\":" << raw_br.size();
    std::cout << ",\"raw_complete_bytes\":" << raw_complete;
    std::cout << ",\"raw_encode_ms\":" << std::fixed << std::setprecision(6) << raw_enc_ms;
    std::cout << ",\"raw_decode_ms\":" << raw_dec_ms;
    std::cout << ",\"raw_roundtrip\":true";
    std::cout << ",\"analyze_ms\":" << analyze_ms;
    std::cout << ",\"candidate_build_local_score_ms\":" << candidate_build_local_score_ms;

    std::cout << ",\"record_count\":" << a.records.size();
    std::cout << ",\"shape_count\":" << a.shapes.size();
    std::cout << ",\"scalar_count\":" << a.scalar_count;
    std::cout << ",\"scalar_bytes\":" << a.scalar_bytes;
    std::cout << ",\"structural_bytes\":" << a.structural_bytes;
    std::cout << ",\"singleton_shape_count\":" << cov.singleton;
    std::cout << ",\"top1_shape_rows\":" << cov.top1;
    std::cout << ",\"top5_shape_rows\":" << cov.top5;
    std::cout << ",\"top10_shape_rows\":" << cov.top10;
    std::cout << ",\"unique_template_bytes\":" << a.unique_template_bytes;
    std::cout << ",\"gross_template_bytes_avoided\":" << a.gross_template_bytes_avoided;
    std::cout << ",\"total_columns\":" << cols.size();
    std::cout << ",\"typed_eligible_columns\":" << typed_cols;
    std::cout << ",\"isolated_q11_leaf_evaluations\":" << isolated_q11_evals;

    emit_arm_json("g1r", g1r, raw_complete);
    emit_arm_json("p_dict", pdict, raw_complete);
    emit_arm_json("p_int", pint, raw_complete);
    emit_arm_json("p_mixed", pmixed, raw_complete);
    emit_arm_json("p_marginal", marginal.arm, raw_complete);

    std::cout << ",\"candidate_substitutions\":" << marginal.candidate_substitutions;
    std::cout << ",\"marginal_candidates_evaluated\":" << marginal.marginal_candidates_evaluated;
    std::cout << ",\"accepted_marginal_substitutions\":" << marginal.accepted.size();
    std::cout << ",\"exact_whole_carrier_evaluations\":"
              << marginal.exact_whole_carrier_evaluations;
    std::cout << ",\"marginal_search_ms\":" << marginal.search_ms;
    std::cout << ",\"marginal_byte_trace\":[";
    for (size_t i = 0; i < marginal.byte_trace.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << marginal.byte_trace[i];
    }
    std::cout << "]";
    std::cout << ",\"marginal_accepted\":[";
    for (size_t i = 0; i < marginal.accepted.size(); ++i) {
        if (i) std::cout << ",";
        const auto [ci, leaf] = marginal.accepted[i];
        std::cout << "{\"shape_id\":" << cols[ci].shape_id
                  << ",\"slot_id\":" << cols[ci].slot_id
                  << ",\"leaf\":\"" << leaf_json_name(leaf) << "\"}";
    }
    std::cout << "]";

    emit_selection_json("p_dict_selection", cols, pdict.selection);
    emit_selection_json("p_int_selection", cols, pint.selection);
    emit_selection_json("p_mixed_selection", cols, pmixed.selection);
    emit_selection_json("p_marginal_selection", cols, marginal.arm.selection);

    std::cout << ",\"columns\":[";
    for (size_t i = 0; i < cols.size(); ++i) {
        if (i) std::cout << ",";
        const auto& c = cols[i];
        std::cout << "{\"shape_id\":" << c.shape_id
                  << ",\"slot_id\":" << c.slot_id
                  << ",\"occurrences\":" << c.tokens.size()
                  << ",\"token_bytes\":" << c.token_bytes
                  << ",\"distinct_tokens\":" << c.distinct_tokens
                  << ",\"canonical_int\":" << (c.canonical_int ? "true" : "false");
        if (c.canonical_int) {
            std::cout << ",\"int_min\":" << c.int_min
                      << ",\"int_max\":" << c.int_max
                      << ",\"delta_eligible\":" << (c.delta_eligible ? "true" : "false")
                      << ",\"dod_eligible\":" << (c.dod_eligible ? "true" : "false");
            if (c.delta_eligible)
                std::cout << ",\"delta_min\":" << c.delta_min << ",\"delta_max\":" << c.delta_max;
            if (c.dod_eligible)
                std::cout << ",\"dod_min\":" << c.dod_min << ",\"dod_max\":" << c.dod_max;
        }
        std::cout << ",\"candidates\":[";
        for (size_t j = 0; j < c.candidates.size(); ++j) {
            if (j) std::cout << ",";
            const auto& x = c.candidates[j];
            std::cout << "{\"leaf\":\"" << leaf_json_name(x.id)
                      << "\",\"payload_bytes\":" << x.payload.size()
                      << ",\"isolated_brotli_bytes\":" << x.isolated_brotli_bytes << "}";
        }
        std::cout << "]"
                  << ",\"p_dict_leaf\":\"" << leaf_json_name(pdict.selection[i]) << "\""
                  << ",\"p_int_leaf\":\"" << leaf_json_name(pint.selection[i]) << "\""
                  << ",\"p_mixed_leaf\":\"" << leaf_json_name(pmixed.selection[i]) << "\""
                  << ",\"p_marginal_leaf\":\"" << leaf_json_name(marginal.arm.selection[i]) << "\""
                  << "}";
    }
    std::cout << "]";

    std::cout << ",\"selected_arm\":\"" << selected.name << "\"";
    std::cout << ",\"selected_bytes\":" << selected.bytes;
    std::cout << ",\"selected_delta_pct\":" << delta_pct(selected.bytes, raw_complete);
    std::cout << "}\n";
    return 0;
}

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
        const std::vector<int64_t> v{
            0, std::numeric_limits<int64_t>::min(), -1};
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
        // dict_count=3 => width=2. ID 3 is out of range while unused
        // high bits stay zero, so this exercises ID range rather than padding.
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

static void roundtrip_fixture(const std::string& text) {
    const Bytes src = bytes(text);
    const Analysis a = analyze(src);
    const auto cols = build_columns(src, a);

    std::vector<LeafId> sel = all_raw_selection(cols);
    for (size_t i = 0; i < cols.size(); ++i) {
        if (candidate_by_id(cols[i], LeafId::IntDodFor)) sel[i] = LeafId::IntDodFor;
        else if (candidate_by_id(cols[i], LeafId::IntDeltaFor)) sel[i] = LeafId::IntDeltaFor;
        else if (candidate_by_id(cols[i], LeafId::IntFor)) sel[i] = LeafId::IntFor;
        else if (candidate_by_id(cols[i], LeafId::ExactDict)) sel[i] = LeafId::ExactDict;
    }

    CarrierStats st;
    Bytes c = make_carrier(src, a, cols, sel, st);
    if (decode_carrier(c) != src) throw std::runtime_error("fixture carrier roundtrip");
    const Bytes br = brotli_encode(c);
    const Bytes cd = brotli_decode_bounded(br, src.size() * 4 + kMaxCarrierSlack);
    if (decode_carrier(cd) != src) throw std::runtime_error("fixture Brotli+carrier roundtrip");

    Bytes bad = c;
    bad.pop_back();
    require_throw([&]{ (void)decode_carrier(bad); }, "truncated carrier");
    bad = c;
    bad.push_back(0);
    require_throw([&]{ (void)decode_carrier(bad); }, "carrier trailing byte");
    bad = c;
    bad[0] ^= 1;
    require_throw([&]{ (void)decode_carrier(bad); }, "carrier magic");
}

static int selftest() {
    leaf_selftests();
    roundtrip_fixture(
        "{\"a\":1,\"b\":\"x\",\"n\":null}\n"
        "{\"a\":2,\"b\":\"y\",\"n\":null}\n"
        "{\"a\":3,\"b\":\"x\",\"n\":null}\n"
        "{\"a\":4,\"b\":\"y\",\"n\":null}\n");
    roundtrip_fixture(
        "{\"k\":[1,2,{\"z\":\"a\"}],\"k\":3}\r\n"
        "{\"k\":[4,5,{\"z\":\"b\"}],\"k\":6}\r\n"
        "{\"k\":[7,8,{\"z\":\"a\"}],\"k\":9}\r\n");

    const std::array<std::string, 9> invalid = {
        "\n", "{\"a\":01}\n", "{\"a\":1,}\n", "{\"a\" 1}\n",
        "{\"a\":\"bad\\x\"}\n", "{\"a\":\"bad\n\"}\n", "[1,]\n",
        "true false\n", "{\"a\":1\n",
    };
    for (const auto& s : invalid)
        require_throw([&]{ (void)analyze(bytes(s)); }, "invalid JSON");

    Bytes noncanon{0x80, 0x00};
    size_t p = 0;
    require_throw([&]{ (void)get_uvar(noncanon, p); }, "noncanonical uvar");

    const Bytes raw = bytes("hello hello hello");
    const Bytes br = brotli_encode(raw);
    if (brotli_decode_exact(br, raw.size()) != raw)
        throw std::runtime_error("raw Brotli selftest");

    std::cout << "PASS grotli_g2 selftest\n";
    return 0;
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "selftest") return selftest();
        if (argc == 3 && std::string(argv[1]) == "measure") return measure(argv[2]);
        std::cerr << "usage: grotli_g2 selftest | grotli_g2 measure INPUT\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
