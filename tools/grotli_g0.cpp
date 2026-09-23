// GROTLI-ANVIL-0 frozen representation prototype.
//
// This is intentionally standalone research code. It does not change ANVIL's
// production wire. Semantics are frozen by docs/I10-GROTLI-G0-PREREG.md.
//
// Build (Linux):
//   clang++ -O3 -std=c++20 tools/grotli_g0.cpp -lbrotlienc -lbrotlidec -lbrotlicommon -o grotli_g0
//
// Usage:
//   grotli_g0 measure INPUT
//   grotli_g0 selftest
//
// The measure command emits one JSON object. Compression is Brotli q11/lgwin30
// in both causal arms. Every vXOR reconstruction byte is inside the compressed
// carrier. The common prototype envelope is:
//   arm:u8 | decoded_source_len:uvar | brotli_payload
// so its unequal cost versus standalone Brotli is fully visible.

#include <brotli/decode.h>
#include <brotli/encode.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

using Bytes = std::vector<uint8_t>;
using Clock = std::chrono::steady_clock;

static constexpr uint8_t kCarrierMagic[4] = {'G','0','V','X'};
static constexpr uint8_t kCarrierVersion = 1;
static constexpr uint8_t kArmRaw = 0;
static constexpr uint8_t kArmVxor = 1;
static constexpr uint64_t kMaxStride = 4ull << 20;
static constexpr uint64_t kMaxMatrix = 1ull << 34;

static void put_uvar(Bytes& out, uint64_t x) {
    while (x >= 0x80) {
        out.push_back(static_cast<uint8_t>((x & 0x7f) | 0x80));
        x >>= 7;
    }
    out.push_back(static_cast<uint8_t>(x));
}

static uint64_t get_uvar(const Bytes& in, size_t& p) {
    uint64_t x = 0;
    unsigned shift = 0;
    for (unsigned i = 0; i < 10; ++i) {
        if (p >= in.size()) throw std::runtime_error("truncated uvar");
        const uint8_t b = in[p++];
        if (i == 9 && (b & 0xfe)) throw std::runtime_error("uvar overflow");
        x |= uint64_t(b & 0x7f) << shift;
        if (!(b & 0x80)) return x;
        shift += 7;
    }
    throw std::runtime_error("uvar overflow");
}

static size_t uvar_len(uint64_t x) {
    size_t n = 1;
    while (x >= 0x80) { x >>= 7; ++n; }
    return n;
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

struct Records {
    std::vector<size_t> start;
    std::vector<size_t> len;
    size_t stride = 0;
};

static Records split_records(const Bytes& src) {
    Records r;
    size_t begin = 0;
    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] == 0x0a) {
            r.start.push_back(begin);
            r.len.push_back(i + 1 - begin);
            r.stride = std::max(r.stride, i + 1 - begin);
            begin = i + 1;
        }
    }
    if (begin < src.size()) {
        r.start.push_back(begin);
        r.len.push_back(src.size() - begin);
        r.stride = std::max(r.stride, src.size() - begin);
    }
    return r;
}

struct CarrierStats {
    size_t descriptor_bytes = 0;
    size_t length_bytes = 0;
    size_t matrix_bytes = 0;
    size_t padding_bytes = 0;
    size_t zeros = 0;
};

static Bytes make_vxor_carrier(const Bytes& src, const Records& rec, CarrierStats& st) {
    if (rec.len.size() < 8) throw std::runtime_error("vXOR ineligible: record_count < 8");
    if (rec.stride > kMaxStride) throw std::runtime_error("vXOR ineligible: stride > 4 MiB");
    if (rec.stride && rec.len.size() > kMaxMatrix / rec.stride)
        throw std::runtime_error("vXOR ineligible: matrix resource bound");
    const uint64_t matrix_len = uint64_t(rec.len.size()) * rec.stride;

    Bytes out;
    const size_t header_begin = out.size();
    out.insert(out.end(), std::begin(kCarrierMagic), std::end(kCarrierMagic));
    out.push_back(kCarrierVersion);
    put_uvar(out, src.size());
    put_uvar(out, rec.len.size());
    put_uvar(out, rec.stride);
    st.descriptor_bytes = out.size() - header_begin;

    const size_t lens_begin = out.size();
    for (size_t n : rec.len) put_uvar(out, n);
    st.length_bytes = out.size() - lens_begin;
    put_uvar(out, matrix_len);
    st.descriptor_bytes += uvar_len(matrix_len);

    if (matrix_len > std::numeric_limits<size_t>::max() - out.size())
        throw std::runtime_error("carrier size overflow");
    out.reserve(out.size() + static_cast<size_t>(matrix_len));

    Bytes prev(rec.stride, 0);
    Bytes cur(rec.stride, 0);
    for (size_t r = 0; r < rec.len.size(); ++r) {
        std::fill(cur.begin(), cur.end(), 0);
        std::copy_n(src.data() + rec.start[r], rec.len[r], cur.data());
        for (size_t c = 0; c < rec.stride; ++c) {
            const uint8_t d = (r == 0) ? cur[c] : uint8_t(cur[c] ^ prev[c]);
            out.push_back(d);
            st.zeros += (d == 0);
        }
        prev.swap(cur);
    }
    st.matrix_bytes = static_cast<size_t>(matrix_len);
    st.padding_bytes = st.matrix_bytes - src.size();
    return out;
}

static Bytes decode_vxor_carrier(const Bytes& carrier) {
    size_t p = 0;
    if (carrier.size() < 5 || std::memcmp(carrier.data(), kCarrierMagic, 4) != 0)
        throw std::runtime_error("bad carrier magic");
    p += 4;
    if (carrier[p++] != kCarrierVersion) throw std::runtime_error("bad carrier version");
    const uint64_t decoded_len = get_uvar(carrier, p);
    const uint64_t count = get_uvar(carrier, p);
    const uint64_t stride = get_uvar(carrier, p);
    if (decoded_len > std::numeric_limits<size_t>::max() ||
        count > std::numeric_limits<size_t>::max() ||
        stride > std::numeric_limits<size_t>::max())
        throw std::runtime_error("carrier integer exceeds address space");
    if (count < 8) throw std::runtime_error("invalid record_count");
    if (stride > kMaxStride) throw std::runtime_error("invalid stride");
    if (stride && count > kMaxMatrix / stride) throw std::runtime_error("matrix bound");

    std::vector<size_t> lens;
    lens.reserve(static_cast<size_t>(count));
    uint64_t sum = 0, max_len = 0;
    for (uint64_t i = 0; i < count; ++i) {
        const uint64_t n = get_uvar(carrier, p);
        if (n > stride) throw std::runtime_error("record length exceeds stride");
        if (sum > decoded_len || n > decoded_len - sum)
            throw std::runtime_error("record length sum overflow");
        sum += n;
        max_len = std::max(max_len, n);
        lens.push_back(static_cast<size_t>(n));
    }
    if (sum != decoded_len) throw std::runtime_error("decoded length mismatch");
    if (max_len != stride) throw std::runtime_error("stride is not max(record_len)");

    const uint64_t matrix_len = get_uvar(carrier, p);
    if (matrix_len != count * stride) throw std::runtime_error("matrix length mismatch");
    if (matrix_len > carrier.size() - p) throw std::runtime_error("truncated matrix");
    if (matrix_len != carrier.size() - p) throw std::runtime_error("trailing carrier bytes");

    Bytes out;
    out.reserve(static_cast<size_t>(decoded_len));
    Bytes prev(static_cast<size_t>(stride), 0);
    Bytes cur(static_cast<size_t>(stride), 0);
    for (size_t r = 0; r < lens.size(); ++r) {
        for (size_t c = 0; c < static_cast<size_t>(stride); ++c) {
            const uint8_t d = carrier[p++];
            cur[c] = (r == 0) ? d : uint8_t(d ^ prev[c]);
        }
        out.insert(out.end(), cur.begin(), cur.begin() + lens[r]);
        prev.swap(cur);
    }
    if (out.size() != decoded_len || p != carrier.size())
        throw std::runtime_error("carrier consumption mismatch");
    return out;
}

static Bytes envelope(uint8_t arm, size_t decoded_size, const Bytes& payload) {
    Bytes out;
    out.push_back(arm);
    put_uvar(out, decoded_size);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

static double h0(const Bytes& b) {
    if (b.empty()) return 0.0;
    uint64_t f[256] = {};
    for (uint8_t x : b) ++f[x];
    double bits = 0.0;
    for (uint64_t n : f) if (n) {
        const double p = double(n) / double(b.size());
        bits -= double(n) * std::log2(p);
    }
    return bits / 8.0; // idealized bytes, diagnostic only
}

static double seconds(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double>(b - a).count();
}

static void json_str(const std::string& s) {
    std::cout << '"';
    for (char c : s) {
        if (c == '"' || c == '\\') std::cout << '\\' << c;
        else if (static_cast<unsigned char>(c) < 0x20) std::cout << '?';
        else std::cout << c;
    }
    std::cout << '"';
}

static int measure(const std::string& path) {
    const Bytes src = read_file(path);
    const Records rec = split_records(src);

    const auto r0 = Clock::now();
    const Bytes raw_br = brotli_encode(src);
    const auto r1 = Clock::now();
    const Bytes raw_dec = brotli_decode_exact(raw_br, src.size());
    const auto r2 = Clock::now();
    if (raw_dec != src) throw std::runtime_error("raw Brotli roundtrip mismatch");
    const Bytes raw_env = envelope(kArmRaw, src.size(), raw_br);

    bool eligible = rec.len.size() >= 8 && rec.stride <= kMaxStride &&
                    (!rec.stride || rec.len.size() <= kMaxMatrix / rec.stride);

    CarrierStats st;
    Bytes carrier, vx_br, vx_env;
    double vx_enc_s = 0, vx_dec_s = 0;
    bool vx_roundtrip = false;
    double raw_h0 = h0(src), vx_h0 = 0;

    if (eligible) {
        const auto v0 = Clock::now();
        carrier = make_vxor_carrier(src, rec, st);
        vx_h0 = h0(carrier);
        vx_br = brotli_encode(carrier);
        const auto v1 = Clock::now();
        const Bytes carrier_dec = brotli_decode_exact(vx_br, carrier.size());
        const Bytes vx_dec = decode_vxor_carrier(carrier_dec);
        const auto v2 = Clock::now();
        vx_roundtrip = (vx_dec == src);
        if (!vx_roundtrip) throw std::runtime_error("vXOR roundtrip mismatch");
        vx_enc_s = seconds(v0, v1);
        vx_dec_s = seconds(v1, v2);
        vx_env = envelope(kArmVxor, src.size(), vx_br);
    }

    std::vector<size_t> sorted = rec.len;
    std::sort(sorted.begin(), sorted.end());
    auto quant = [&](double q) -> size_t {
        if (sorted.empty()) return 0;
        const size_t i = std::min(sorted.size()-1, static_cast<size_t>(q * (sorted.size()-1)));
        return sorted[i];
    };
    double mean = 0, cv = 0;
    if (!rec.len.empty()) {
        mean = double(src.size()) / rec.len.size();
        double ss = 0;
        for (size_t n : rec.len) { const double d = double(n)-mean; ss += d*d; }
        cv = mean ? std::sqrt(ss / rec.len.size()) / mean : 0;
    }

    const bool choose_vx = eligible && vx_env.size() < raw_env.size();

    std::cout << std::setprecision(12) << "{";
    std::cout << "\"file\":"; json_str(path);
    std::cout << ",\"source_bytes\":" << src.size();
    std::cout << ",\"record_count\":" << rec.len.size();
    std::cout << ",\"record_len_min\":" << (sorted.empty()?0:sorted.front());
    std::cout << ",\"record_len_median\":" << quant(0.5);
    std::cout << ",\"record_len_p95\":" << quant(0.95);
    std::cout << ",\"record_len_max\":" << rec.stride;
    std::cout << ",\"record_len_cv\":" << cv;
    std::cout << ",\"padding_expansion\":" << (src.empty()?0.0:double(rec.len.size())*rec.stride/src.size());
    std::cout << ",\"raw_h0_ideal_bytes\":" << raw_h0;
    std::cout << ",\"raw_brotli_bytes\":" << raw_br.size();
    std::cout << ",\"raw_envelope_bytes\":" << raw_env.size();
    std::cout << ",\"raw_encode_s\":" << seconds(r0,r1);
    std::cout << ",\"raw_decode_s\":" << seconds(r1,r2);
    std::cout << ",\"vxor_eligible\":" << (eligible?"true":"false");
    if (eligible) {
        std::cout << ",\"carrier_bytes\":" << carrier.size();
        std::cout << ",\"descriptor_bytes\":" << st.descriptor_bytes;
        std::cout << ",\"record_length_bytes\":" << st.length_bytes;
        std::cout << ",\"xor_matrix_bytes\":" << st.matrix_bytes;
        std::cout << ",\"padding_bytes\":" << st.padding_bytes;
        std::cout << ",\"vxor_zero_density\":" << (st.matrix_bytes?double(st.zeros)/st.matrix_bytes:0.0);
        std::cout << ",\"vxor_h0_ideal_bytes\":" << vx_h0;
        std::cout << ",\"vxor_brotli_bytes\":" << vx_br.size();
        std::cout << ",\"vxor_envelope_bytes\":" << vx_env.size();
        std::cout << ",\"vxor_delta_bytes\":" << (int64_t(vx_env.size())-int64_t(raw_env.size()));
        std::cout << ",\"vxor_delta_pct\":" << (raw_env.empty()?0.0:100.0*(double(vx_env.size())/raw_env.size()-1.0));
        std::cout << ",\"vxor_encode_s\":" << vx_enc_s;
        std::cout << ",\"vxor_decode_s\":" << vx_dec_s;
        std::cout << ",\"vxor_roundtrip\":" << (vx_roundtrip?"true":"false");
    }
    std::cout << ",\"selected_arm\":\"" << (choose_vx?"vxor":"raw") << "\"";
    std::cout << ",\"selected_bytes\":" << (choose_vx?vx_env.size():raw_env.size());
    std::cout << "}\n";
    return 0;
}

static void expect_bad(Bytes b) {
    bool bad = false;
    try { (void)decode_vxor_carrier(b); } catch (...) { bad = true; }
    if (!bad) throw std::runtime_error("malformed carrier unexpectedly accepted");
}

static int selftest() {
    Bytes src;
    const char* rows[] = {
        "{\"id\":1,\"v\":\"alpha\"}\n", "{\"id\":2,\"v\":\"alpha\"}\n",
        "{\"id\":3,\"v\":\"beta\"}\n",  "{\"id\":4,\"v\":\"alpha\"}\n",
        "{\"id\":5,\"v\":\"gamma\"}\n", "{\"id\":6,\"v\":\"alpha\"}\n",
        "{\"id\":7,\"v\":\"beta\"}\n",  "{\"id\":8,\"v\":\"alpha\"}",
    };
    for (const char* s : rows) src.insert(src.end(), s, s + std::strlen(s));
    const Records rec = split_records(src);
    CarrierStats st;
    const Bytes c = make_vxor_carrier(src, rec, st);
    if (decode_vxor_carrier(c) != src) throw std::runtime_error("selftest roundtrip");

    Bytes b = c; b.push_back(0); expect_bad(b);                // trailing byte
    b = c; b.resize(c.size()-1); expect_bad(b);                // truncation
    b = c; b[4] = 99; expect_bad(b);                           // version
    b = c; b[0] ^= 1; expect_bad(b);                           // magic

    const Bytes br = brotli_encode(c);
    if (brotli_decode_exact(br, c.size()) != c) throw std::runtime_error("brotli selftest");
    std::cout << "GROTLI-G0 selftest: PASS\n";
    return 0;
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "selftest") return selftest();
        if (argc == 3 && std::string(argv[1]) == "measure") return measure(argv[2]);
        std::cerr << "usage: grotli_g0 selftest | grotli_g0 measure INPUT\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "grotli_g0: " << e.what() << "\n";
        return 1;
    }
}
