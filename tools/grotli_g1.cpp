// ANVIL I10 GROTLI G1-CEILING standalone prototype.
//
// Frozen semantics: docs/I10-GROTLI-G1-CEILING-PREREG.md
//
// This is research tooling only. It does not change ANVIL's production wire.
//
// Build:
//   clang++ -O3 -DNDEBUG -std=c++20 tools/grotli_g1.cpp \
//     -lbrotlienc -lbrotlidec -lbrotlicommon -o grotli_g1
//
// Usage:
//   grotli_g1 measure INPUT
//   grotli_g1 selftest
//
// Causal arms:
//   RAW          -> Brotli q11/lgwin30(original)
//   SHAPE_ROW    -> Brotli q11/lgwin30(exact shape carrier, row-value order)
//   SHAPE_COLUMN -> Brotli q11/lgwin30(exact shape carrier, column-value order)
//
// Common prototype envelope cost for every arm:
//   arm:u8 | decoded_source_len:uvar | brotli_payload
//
// The structured carrier itself contains every byte needed to reconstruct the
// source. No schema or semantic JSON state is external to the archive.

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
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using Bytes = std::vector<uint8_t>;
using Clock = std::chrono::steady_clock;

static constexpr uint8_t kMagic[4] = {'G','1','S','R'};
static constexpr uint8_t kVersion = 1;
static constexpr uint8_t kLayoutRow = 1;
static constexpr uint8_t kLayoutColumn = 2;
static constexpr uint8_t kArmRaw = 0;
static constexpr uint8_t kArmRow = 1;
static constexpr uint8_t kArmColumn = 2;
static constexpr uint64_t kMaxDecoded = 1ull << 34; // research-wire safety bound
static constexpr uint64_t kMaxCarrierSlack = 64ull << 20;

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
    uint64_t value_length_bytes = 0;
    uint64_t raw_value_bytes = 0;
};

static Bytes make_carrier(const Bytes& src, const Analysis& a, uint8_t layout, CarrierStats& st) {
    if (layout != kLayoutRow && layout != kLayoutColumn)
        throw std::runtime_error("bad layout");

    Bytes out;
    out.insert(out.end(), std::begin(kMagic), std::end(kMagic));
    out.push_back(kVersion);
    out.push_back(layout);
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

    auto emit_value = [&](const Span& s) {
        const size_t n = s.hi - s.lo;
        const size_t before = out.size();
        put_uvar(out, n);
        st.value_length_bytes += out.size() - before;
        st.raw_value_bytes += n;
        out.insert(out.end(), src.begin() + s.lo, src.begin() + s.hi);
    };

    for (const Shape& sh : a.shapes) {
        const size_t slots = sh.parts.size() - 1;
        if (layout == kLayoutRow) {
            for (size_t rid : sh.members) {
                const ParsedRecord& rec = a.records[rid];
                if (rec.values.size() != slots) throw std::runtime_error("shape slot mismatch");
                for (const Span& s : rec.values) emit_value(s);
            }
        } else {
            for (size_t slot = 0; slot < slots; ++slot) {
                for (size_t rid : sh.members) {
                    const ParsedRecord& rec = a.records[rid];
                    if (rec.values.size() != slots) throw std::runtime_error("shape slot mismatch");
                    emit_value(rec.values[slot]);
                }
            }
        }
    }

    return out;
}

struct CSpan {
    uint32_t off = 0;
    uint32_t len = 0;
};

struct DShape {
    uint32_t occurrences = 0;
    uint32_t slots = 0;
    std::vector<CSpan> parts;
    std::vector<CSpan> values; // occurrence-major indexing
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
    if (carrier.size() < 6 || std::memcmp(carrier.data(), kMagic, 4) != 0)
        throw std::runtime_error("bad G1 carrier magic");
    size_t p = 4;
    if (carrier[p++] != kVersion) throw std::runtime_error("bad G1 carrier version");
    const uint8_t layout = carrier[p++];
    if (layout != kLayoutRow && layout != kLayoutColumn)
        throw std::runtime_error("bad G1 carrier layout");

    const uint64_t decoded_len = get_uvar(carrier, p);
    const uint64_t record_count = get_uvar(carrier, p);
    const uint64_t shape_count = get_uvar(carrier, p);

    if (decoded_len == 0 || decoded_len > kMaxDecoded ||
        decoded_len > std::numeric_limits<size_t>::max())
        throw std::runtime_error("bad G1 decoded length");
    if (record_count == 0 || record_count > decoded_len ||
        record_count > std::numeric_limits<uint32_t>::max() ||
        record_count > carrier.size())
        throw std::runtime_error("bad G1 record count");
    if (shape_count == 0 || shape_count > record_count ||
        shape_count > std::numeric_limits<uint32_t>::max() ||
        shape_count > carrier.size() / 3)
        throw std::runtime_error("bad G1 shape count");
    if (carrier.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("G1 carrier exceeds prototype offset width");

    std::vector<DShape> shapes(static_cast<size_t>(shape_count));
    uint64_t occurrence_sum = 0;
    uint64_t template_sum = 0;
    uint64_t scalar_count_bound = 0;

    for (DShape& sh : shapes) {
        const uint64_t occ = get_uvar(carrier, p);
        const uint64_t slots = get_uvar(carrier, p);
        if (occ == 0 || occ > record_count || slots > decoded_len)
            throw std::runtime_error("bad G1 shape dimensions");
        if (occurrence_sum > record_count - occ)
            throw std::runtime_error("G1 occurrence sum overflow");
        occurrence_sum += occ;
        if (slots && occ > decoded_len / slots)
            throw std::runtime_error("G1 scalar count bound");
        const uint64_t values = occ * slots;
        if (scalar_count_bound > decoded_len - values)
            throw std::runtime_error("too many G1 scalar values");
        scalar_count_bound += values;
        // Every scalar necessarily consumes at least one canonical length byte
        // plus one token byte. Every template part consumes at least its length
        // varint. Bound these counts against the actual decoded carrier before
        // allocating vectors from attacker-controlled metadata.
        if (scalar_count_bound > carrier.size() / 2)
            throw std::runtime_error("G1 scalar count exceeds carrier capacity");
        if (slots + 1 > carrier.size() - p)
            throw std::runtime_error("G1 part count exceeds carrier capacity");

        sh.occurrences = static_cast<uint32_t>(occ);
        sh.slots = static_cast<uint32_t>(slots);
        sh.parts.reserve(static_cast<size_t>(slots + 1));
        for (uint64_t i = 0; i <= slots; ++i) {
            const uint64_t n = get_uvar(carrier, p);
            if (template_sum > decoded_len - std::min<uint64_t>(n, decoded_len))
                throw std::runtime_error("G1 template sum overflow");
            if (n > decoded_len) throw std::runtime_error("G1 template part too large");
            template_sum += n;
            sh.parts.push_back(take_span(carrier, p, n, decoded_len));
        }
        sh.values.resize(static_cast<size_t>(values));
    }
    if (occurrence_sum != record_count) throw std::runtime_error("G1 occurrence count mismatch");
    if (template_sum > decoded_len) throw std::runtime_error("G1 unique template bytes exceed source");

    std::vector<uint32_t> shape_ids;
    shape_ids.reserve(static_cast<size_t>(record_count));
    std::vector<uint32_t> freq(static_cast<size_t>(shape_count), 0);
    for (uint64_t i = 0; i < record_count; ++i) {
        const uint64_t sid = get_uvar(carrier, p);
        if (sid >= shape_count) throw std::runtime_error("G1 shape id out of range");
        shape_ids.push_back(static_cast<uint32_t>(sid));
        if (freq[static_cast<size_t>(sid)] == std::numeric_limits<uint32_t>::max())
            throw std::runtime_error("G1 shape frequency overflow");
        ++freq[static_cast<size_t>(sid)];
    }
    for (size_t i = 0; i < shapes.size(); ++i)
        if (freq[i] != shapes[i].occurrences)
            throw std::runtime_error("G1 shape frequency mismatch");

    uint64_t value_bytes = 0;
    for (DShape& sh : shapes) {
        const size_t occ = sh.occurrences;
        const size_t slots = sh.slots;
        auto read_one = [&](size_t oi, size_t sj) {
            const uint64_t n = get_uvar(carrier, p);
            if (n == 0 || n > decoded_len) throw std::runtime_error("bad G1 scalar length");
            if (value_bytes > decoded_len - n) throw std::runtime_error("G1 scalar byte sum overflow");
            value_bytes += n;
            sh.values[oi * slots + sj] = take_span(carrier, p, n, decoded_len);
        };
        if (layout == kLayoutRow) {
            for (size_t oi = 0; oi < occ; ++oi)
                for (size_t sj = 0; sj < slots; ++sj)
                    read_one(oi, sj);
        } else {
            for (size_t sj = 0; sj < slots; ++sj)
                for (size_t oi = 0; oi < occ; ++oi)
                    read_one(oi, sj);
        }
    }

    if (p != carrier.size()) throw std::runtime_error("G1 carrier trailing bytes");

    Bytes out;
    out.reserve(static_cast<size_t>(decoded_len));
    std::vector<uint32_t> cursor(shapes.size(), 0);

    auto append = [&](CSpan s) {
        if (s.len > decoded_len - out.size()) throw std::runtime_error("G1 reconstructed size overflow");
        out.insert(out.end(), carrier.begin() + s.off, carrier.begin() + s.off + s.len);
    };

    for (uint32_t sid : shape_ids) {
        DShape& sh = shapes[sid];
        const uint32_t oi = cursor[sid]++;
        if (oi >= sh.occurrences) throw std::runtime_error("G1 shape cursor overflow");
        for (size_t sj = 0; sj < sh.slots; ++sj) {
            append(sh.parts[sj]);
            append(sh.values[size_t(oi) * sh.slots + sj]);
        }
        append(sh.parts[sh.slots]);
    }

    for (size_t i = 0; i < shapes.size(); ++i)
        if (cursor[i] != shapes[i].occurrences)
            throw std::runtime_error("G1 shape cursor mismatch");
    if (out.size() != decoded_len) throw std::runtime_error("G1 reconstructed length mismatch");
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

static int measure(const std::string& path) {
    const Bytes src = read_file(path);

    const auto raw_t0 = Clock::now();
    const Bytes raw_br = brotli_encode(src);
    const double raw_enc_ms = ms_since(raw_t0);
    const auto raw_d0 = Clock::now();
    const Bytes raw_dec = brotli_decode_exact(raw_br, src.size());
    const double raw_dec_ms = ms_since(raw_d0);
    if (raw_dec != src) throw std::runtime_error("raw Brotli roundtrip mismatch");

    bool eligible = true;
    std::string ineligible_reason;
    Analysis a;
    Bytes row_carrier, col_carrier, row_br, col_br;
    CarrierStats row_st, col_st;
    double analyze_ms = 0, row_build_ms = 0, col_build_ms = 0;
    double row_enc_ms = 0, col_enc_ms = 0, row_dec_ms = 0, col_dec_ms = 0;
    bool row_ok = false, col_ok = false;

    try {
        const auto t0 = Clock::now();
        a = analyze(src);
        analyze_ms = ms_since(t0);
    } catch (const std::exception& e) {
        eligible = false;
        ineligible_reason = e.what();
    }

    // Once the source is eligible, every later error is an implementation or
    // carrier-correctness failure and must fail the run. Never convert an
    // internal bug into an apparently harmless raw fallback.
    if (eligible) {
        auto b0 = Clock::now();
        row_carrier = make_carrier(src, a, kLayoutRow, row_st);
        row_build_ms = ms_since(b0);
        b0 = Clock::now();
        col_carrier = make_carrier(src, a, kLayoutColumn, col_st);
        col_build_ms = ms_since(b0);

        if (row_carrier.size() != col_carrier.size())
            throw std::runtime_error("row/column logical carrier size must match");

        auto e0 = Clock::now();
        row_br = brotli_encode(row_carrier);
        row_enc_ms = ms_since(e0);
        e0 = Clock::now();
        col_br = brotli_encode(col_carrier);
        col_enc_ms = ms_since(e0);

        const uint64_t bound64 = std::min<uint64_t>(
            std::numeric_limits<size_t>::max(),
            std::min<uint64_t>(kMaxDecoded, uint64_t(src.size()) * 4ull + kMaxCarrierSlack));

        auto d0 = Clock::now();
        const Bytes row_cd = brotli_decode_bounded(row_br, static_cast<size_t>(bound64));
        const Bytes row_decoded = decode_carrier(row_cd);
        row_dec_ms = ms_since(d0);
        row_ok = (row_decoded == src);

        d0 = Clock::now();
        const Bytes col_cd = brotli_decode_bounded(col_br, static_cast<size_t>(bound64));
        const Bytes col_decoded = decode_carrier(col_cd);
        col_dec_ms = ms_since(d0);
        col_ok = (col_decoded == src);
        if (!row_ok || !col_ok) throw std::runtime_error("structured roundtrip mismatch");
    }

    const size_t raw_complete = complete_bytes(src.size(), raw_br);
    size_t row_complete = std::numeric_limits<size_t>::max();
    size_t col_complete = std::numeric_limits<size_t>::max();
    std::string selected = "raw";
    size_t selected_bytes = raw_complete;
    if (eligible) {
        row_complete = complete_bytes(src.size(), row_br);
        col_complete = complete_bytes(src.size(), col_br);
        if (row_complete < selected_bytes) { selected_bytes = row_complete; selected = "row"; }
        if (col_complete < selected_bytes) { selected_bytes = col_complete; selected = "column"; }
    }

    std::cout << "{";
    std::cout << "\"schema\":1";
    std::cout << ",\"file\":\"" << json_escape(path) << "\"";
    std::cout << ",\"source_bytes\":" << src.size();
    std::cout << ",\"structured_eligible\":" << (eligible ? "true" : "false");
    if (!eligible)
        std::cout << ",\"ineligible_reason\":\"" << json_escape(ineligible_reason) << "\"";
    std::cout << ",\"raw_brotli_bytes\":" << raw_br.size();
    std::cout << ",\"raw_complete_bytes\":" << raw_complete;
    std::cout << ",\"raw_encode_ms\":" << std::fixed << std::setprecision(6) << raw_enc_ms;
    std::cout << ",\"raw_decode_ms\":" << raw_dec_ms;
    std::cout << ",\"raw_roundtrip\":true";

    if (eligible) {
        const ShapeCoverage cov = shape_coverage(a);
        std::cout << ",\"record_count\":" << a.records.size();
        std::cout << ",\"scalar_count\":" << a.scalar_count;
        std::cout << ",\"scalar_bytes\":" << a.scalar_bytes;
        std::cout << ",\"structural_bytes\":" << a.structural_bytes;
        std::cout << ",\"shape_count\":" << a.shapes.size();
        std::cout << ",\"singleton_shape_count\":" << cov.singleton;
        std::cout << ",\"top1_shape_rows\":" << cov.top1;
        std::cout << ",\"top5_shape_rows\":" << cov.top5;
        std::cout << ",\"top10_shape_rows\":" << cov.top10;
        std::cout << ",\"top100_shape_rows\":" << cov.top100;
        std::cout << ",\"shape_occ_median\":" << cov.median;
        std::cout << ",\"shape_occ_p95\":" << cov.p95;
        std::cout << ",\"shape_occ_max\":" << cov.max;
        std::cout << ",\"avg_placeholders_per_record\":"
                  << (a.records.empty() ? 0.0 : double(a.scalar_count) / double(a.records.size()));
        std::cout << ",\"unique_template_bytes\":" << a.unique_template_bytes;
        std::cout << ",\"gross_template_bytes_avoided\":" << a.gross_template_bytes_avoided;
        std::cout << ",\"analyze_ms\":" << analyze_ms;

        std::cout << ",\"carrier_bytes\":" << row_carrier.size();
        std::cout << ",\"carrier_header_bytes\":" << row_st.header_bytes;
        std::cout << ",\"shape_dictionary_bytes\":" << row_st.shape_dictionary_bytes;
        std::cout << ",\"template_bytes\":" << row_st.template_bytes;
        std::cout << ",\"shape_id_bytes\":" << row_st.shape_id_bytes;
        std::cout << ",\"value_length_bytes\":" << row_st.value_length_bytes;
        std::cout << ",\"raw_value_bytes\":" << row_st.raw_value_bytes;

        std::cout << ",\"row_brotli_bytes\":" << row_br.size();
        std::cout << ",\"row_complete_bytes\":" << row_complete;
        std::cout << ",\"row_delta_pct\":" << delta_pct(row_complete, raw_complete);
        std::cout << ",\"row_build_ms\":" << row_build_ms;
        std::cout << ",\"row_encode_ms\":" << row_enc_ms;
        std::cout << ",\"row_decode_ms\":" << row_dec_ms;
        std::cout << ",\"row_roundtrip\":" << (row_ok ? "true" : "false");

        std::cout << ",\"column_brotli_bytes\":" << col_br.size();
        std::cout << ",\"column_complete_bytes\":" << col_complete;
        std::cout << ",\"column_delta_pct\":" << delta_pct(col_complete, raw_complete);
        std::cout << ",\"column_build_ms\":" << col_build_ms;
        std::cout << ",\"column_encode_ms\":" << col_enc_ms;
        std::cout << ",\"column_decode_ms\":" << col_dec_ms;
        std::cout << ",\"column_roundtrip\":" << (col_ok ? "true" : "false");
    }

    std::cout << ",\"selected_arm\":\"" << selected << "\"";
    std::cout << ",\"selected_bytes\":" << selected_bytes;
    std::cout << ",\"selected_delta_pct\":" << delta_pct(selected_bytes, raw_complete);
    std::cout << "}\n";
    return 0;
}

static Bytes bytes(const std::string& s) {
    return Bytes(s.begin(), s.end());
}

template<class F>
static void require_throw(F&& fn, const char* label) {
    bool threw = false;
    try { fn(); } catch (...) { threw = true; }
    if (!threw) throw std::runtime_error(std::string("expected rejection: ") + label);
}

static void roundtrip_fixture(const std::string& text) {
    const Bytes src = bytes(text);
    const Analysis a = analyze(src);
    for (uint8_t layout : {kLayoutRow, kLayoutColumn}) {
        CarrierStats st;
        const Bytes c = make_carrier(src, a, layout, st);
        if (decode_carrier(c) != src) throw std::runtime_error("fixture carrier roundtrip");
        const Bytes br = brotli_encode(c);
        const size_t max_out = src.size() * 4 + kMaxCarrierSlack;
        const Bytes cd = brotli_decode_bounded(br, max_out);
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
}

static int selftest() {
    roundtrip_fixture(
        "{\"a\":1,\"b\":\"x\",\"n\":null}\n"
        "{\"a\":2,\"b\":\"y\",\"n\":null}\n"
        "{\"a\":-0.0e+2,\"b\":\"x\\\\nq\",\"n\":true}\n");
    roundtrip_fixture(
        "{\"k\":[1,2,{\"z\":\"a\"}],\"k\":3}\r\n"
        "{\"k\":[4,5,{\"z\":\"b\"}],\"k\":6}\r\n");
    roundtrip_fixture("[1,\"x\",true,null]\n[2,\"y\",false,null]");

    const std::array<std::string, 9> invalid = {
        "\n",
        "{\"a\":01}\n",
        "{\"a\":1,}\n",
        "{\"a\" 1}\n",
        "{\"a\":\"bad\\x\"}\n",
        "{\"a\":\"bad\n\"}\n",
        "[1,]\n",
        "true false\n",
        "{\"a\":1\n",
    };
    for (const auto& s : invalid)
        require_throw([&]{ (void)analyze(bytes(s)); }, "invalid JSON");

    Bytes noncanon{0x80, 0x00};
    size_t p = 0;
    require_throw([&]{ (void)get_uvar(noncanon, p); }, "noncanonical uvar");

    const Bytes raw = bytes("hello hello hello");
    const Bytes br = brotli_encode(raw);
    if (brotli_decode_exact(br, raw.size()) != raw) throw std::runtime_error("raw Brotli selftest");

    std::cout << "PASS grotli_g1 selftest\n";
    return 0;
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "selftest") return selftest();
        if (argc == 3 && std::string(argv[1]) == "measure") return measure(argv[2]);
        std::cerr << "usage: grotli_g1 selftest | grotli_g1 measure INPUT\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
