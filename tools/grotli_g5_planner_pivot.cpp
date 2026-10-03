#ifndef G5P_FROZEN_G3_HEADER
#define G5P_FROZEN_G3_HEADER "grotli_g3.cpp"
#endif

#define main grotli_g3_frozen_main
#include G5P_FROZEN_G3_HEADER
#undef main

#include <array>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#else
#include <sys/resource.h>
#endif

static constexpr int kBackendWindow = 30;
static constexpr size_t kQ4SampleBudget = 80;
static constexpr size_t kQ4InitialBudget = 40;
static constexpr size_t kQ4InputCap = 8192;
static constexpr double kQ4UcbMultiplier = 1.64;
static constexpr double kQ4ShrinkagePrior = 8.0;
static constexpr double kRidgeLambda = 1.0;
static constexpr size_t kFeatureCount = 8;

namespace planner_sha256 {

struct Context {
    uint32_t state[8];
    uint64_t length = 0;
    uint8_t buffer[64];
    size_t buffered = 0;
};

static const uint32_t kRound[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u,
    0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u,
    0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
    0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
    0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4a,
    0x5b9cca4fu, 0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2u};

static inline uint32_t rotate_right(uint32_t value, uint32_t bits) {
    return (value >> bits) | (value << (32 - bits));
}

static void initialize(Context& context) {
    context.state[0] = 0x6a09e667u;
    context.state[1] = 0xbb67ae85u;
    context.state[2] = 0x3c6ef372u;
    context.state[3] = 0xa54ff53au;
    context.state[4] = 0x510e527fu;
    context.state[5] = 0x9b05688cu;
    context.state[6] = 0x1f83d9abu;
    context.state[7] = 0x5be0cd19u;
}

static void compress(Context& context, const uint8_t* block) {
    uint32_t words[64]{};
    for (size_t i = 0; i < 16; ++i) {
        words[i] = (uint32_t(block[i * 4]) << 24) |
                   (uint32_t(block[i * 4 + 1]) << 16) |
                   (uint32_t(block[i * 4 + 2]) << 8) |
                   uint32_t(block[i * 4 + 3]);
    }
    for (size_t i = 16; i < 64; ++i) {
        const uint32_t s0 = rotate_right(words[i - 15], 7) ^
                            rotate_right(words[i - 15], 18) ^ (words[i - 15] >> 3);
        const uint32_t s1 = rotate_right(words[i - 2], 17) ^
                            rotate_right(words[i - 2], 19) ^ (words[i - 2] >> 10);
        words[i] = words[i - 16] + s0 + words[i - 7] + s1;
    }
    uint32_t a = context.state[0];
    uint32_t b = context.state[1];
    uint32_t c = context.state[2];
    uint32_t d = context.state[3];
    uint32_t e = context.state[4];
    uint32_t f = context.state[5];
    uint32_t g = context.state[6];
    uint32_t h = context.state[7];
    for (size_t i = 0; i < 64; ++i) {
        const uint32_t s1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25);
        const uint32_t choice = (e & f) ^ (~e & g);
        const uint32_t t1 = h + s1 + choice + kRound[i] + words[i];
        const uint32_t s0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22);
        const uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        const uint32_t t2 = s0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    context.state[0] += a;
    context.state[1] += b;
    context.state[2] += c;
    context.state[3] += d;
    context.state[4] += e;
    context.state[5] += f;
    context.state[6] += g;
    context.state[7] += h;
}

static void update(Context& context, const uint8_t* data, size_t size) {
    context.length += size;
    while (size > 0) {
        const size_t take = std::min<size_t>(64 - context.buffered, size);
        std::memcpy(context.buffer + context.buffered, data, take);
        context.buffered += take;
        data += take;
        size -= take;
        if (context.buffered == 64) {
            compress(context, context.buffer);
            context.buffered = 0;
        }
    }
}

static std::array<uint8_t, 32> finish(Context& context) {
    const uint64_t bit_length = context.length * 8ull;
    const uint8_t padding = 0x80;
    update(context, &padding, 1);
    const uint8_t zero = 0;
    while (context.buffered != 56) update(context, &zero, 1);
    uint8_t length_bytes[8];
    for (size_t i = 0; i < 8; ++i)
        length_bytes[i] = static_cast<uint8_t>(bit_length >> (56 - i * 8));
    update(context, length_bytes, 8);
    std::array<uint8_t, 32> digest{};
    for (size_t i = 0; i < 8; ++i) {
        digest[i * 4] = static_cast<uint8_t>(context.state[i] >> 24);
        digest[i * 4 + 1] = static_cast<uint8_t>(context.state[i] >> 16);
        digest[i * 4 + 2] = static_cast<uint8_t>(context.state[i] >> 8);
        digest[i * 4 + 3] = static_cast<uint8_t>(context.state[i]);
    }
    return digest;
}

static std::array<uint8_t, 32> hash_single_block_pipeline(const Bytes& input) {
    std::array<uint32_t, 8> state = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
    Bytes padded = input;
    const uint64_t bit_length = static_cast<uint64_t>(input.size()) * 8ull;
    padded.push_back(0x80);
    while (padded.size() % 64 != 56) padded.push_back(0);
    for (size_t i = 0; i < 8; ++i)
        padded.push_back(static_cast<uint8_t>(bit_length >> (56 - i * 8)));
    for (size_t offset = 0; offset < padded.size(); offset += 64) {
        std::array<uint32_t, 64> words{};
        for (size_t i = 0; i < 16; ++i) {
            const size_t at = offset + i * 4;
            words[i] = (uint32_t(padded[at]) << 24) |
                       (uint32_t(padded[at + 1]) << 16) |
                       (uint32_t(padded[at + 2]) << 8) |
                       uint32_t(padded[at + 3]);
        }
        for (size_t i = 16; i < 64; ++i) {
            const uint32_t s0 = rotate_right(words[i - 15], 7) ^
                                rotate_right(words[i - 15], 18) ^ (words[i - 15] >> 3);
            const uint32_t s1 = rotate_right(words[i - 2], 17) ^
                                rotate_right(words[i - 2], 19) ^ (words[i - 2] >> 10);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }
        uint32_t a = state[0];
        uint32_t b = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];
        uint32_t f = state[5];
        uint32_t g = state[6];
        uint32_t h = state[7];
        for (size_t i = 0; i < 64; ++i) {
            const uint32_t s1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25);
            const uint32_t choice = (e & f) ^ (~e & g);
            const uint32_t t1 = h + s1 + choice + kRound[i] + words[i];
            const uint32_t s0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22);
            const uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t t2 = s0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }
    std::array<uint8_t, 32> digest{};
    for (size_t i = 0; i < state.size(); ++i) {
        digest[i * 4] = static_cast<uint8_t>(state[i] >> 24);
        digest[i * 4 + 1] = static_cast<uint8_t>(state[i] >> 16);
        digest[i * 4 + 2] = static_cast<uint8_t>(state[i] >> 8);
        digest[i * 4 + 3] = static_cast<uint8_t>(state[i]);
    }
    return digest;
}

static uint64_t mask32(uint64_t value) {
    return value & 0xffffffffull;
}

static uint64_t rotate_right64(uint64_t value, uint64_t bits) {
    return mask32((value >> bits) | (value << (32 - bits)));
}

static std::array<uint32_t, 64> generated_round_constants() {
    std::array<uint32_t, 64> primes{};
    size_t count = 0;
    for (uint32_t candidate = 2; count < primes.size(); ++candidate) {
        bool prime = true;
        for (uint32_t divisor = 2; divisor * divisor <= candidate; ++divisor) {
            if (candidate % divisor == 0) {
                prime = false;
                break;
            }
        }
        if (prime) primes[count++] = candidate;
    }
    std::array<uint32_t, 64> constants{};
    for (size_t i = 0; i < constants.size(); ++i) {
        const long double root = std::pow(static_cast<long double>(primes[i]), 1.0L / 3.0L);
        const long double fraction = root - std::floor(root);
        constants[i] = static_cast<uint32_t>(fraction * 4294967296.0L);
    }
    for (size_t i = 0; i < constants.size(); ++i) {
        if (constants[i] != kRound[i])
            throw std::runtime_error("generated SHA-256 constant mismatch at index " + std::to_string(i));
    }
    return constants;
}

static std::array<uint64_t, 8> generated_initial_state() {
    const std::array<uint32_t, 8> primes = {2, 3, 5, 7, 11, 13, 17, 19};
    std::array<uint64_t, 8> state{};
    for (size_t i = 0; i < state.size(); ++i) {
        const long double root = std::sqrt(static_cast<long double>(primes[i]));
        const long double fraction = root - std::floor(root);
        state[i] = static_cast<uint64_t>(fraction * 4294967296.0L);
    }
    if (state.front() != 0x6a09e667ull || state.back() != 0x5be0cd19ull)
        throw std::runtime_error("generated SHA-256 state failed identity");
    return state;
}

static std::array<uint8_t, 32> hash_wide_pipeline(const Bytes& input) {
    static const std::array<uint32_t, 64> constants = generated_round_constants();
    static const std::array<uint64_t, 8> initial_state = generated_initial_state();
    std::array<uint64_t, 8> state = initial_state;
    Bytes padded = input;
    const uint64_t bit_length = static_cast<uint64_t>(input.size()) * 8ull;
    padded.push_back(0x80);
    while (padded.size() % 64 != 56) padded.push_back(0);
    for (size_t i = 0; i < 8; ++i)
        padded.push_back(static_cast<uint8_t>(bit_length >> (56 - i * 8)));
    for (size_t offset = 0; offset < padded.size(); offset += 64) {
        std::array<uint64_t, 64> words{};
        for (size_t i = 0; i < 16; ++i) {
            const size_t at = offset + i * 4;
            words[i] = (uint64_t(padded[at]) << 24) |
                       (uint64_t(padded[at + 1]) << 16) |
                       (uint64_t(padded[at + 2]) << 8) |
                       uint64_t(padded[at + 3]);
        }
        for (size_t i = 16; i < 64; ++i) {
            const uint64_t s0 = rotate_right64(words[i - 15], 7) ^
                                rotate_right64(words[i - 15], 18) ^ mask32(words[i - 15] >> 3);
            const uint64_t s1 = rotate_right64(words[i - 2], 17) ^
                                rotate_right64(words[i - 2], 19) ^ mask32(words[i - 2] >> 10);
            words[i] = mask32(words[i - 16] + s0 + words[i - 7] + s1);
        }
        uint64_t a = state[0];
        uint64_t b = state[1];
        uint64_t c = state[2];
        uint64_t d = state[3];
        uint64_t e = state[4];
        uint64_t f = state[5];
        uint64_t g = state[6];
        uint64_t h = state[7];
        for (size_t i = 0; i < 64; ++i) {
            const uint64_t s1 = rotate_right64(e, 6) ^ rotate_right64(e, 11) ^ rotate_right64(e, 25);
            const uint64_t choice = (e & f) ^ (~e & g);
            const uint64_t t1 = mask32(h + s1 + choice + constants[i] + words[i]);
            const uint64_t s0 = rotate_right64(a, 2) ^ rotate_right64(a, 13) ^ rotate_right64(a, 22);
            const uint64_t majority = (a & b) ^ (a & c) ^ (b & c);
            const uint64_t t2 = mask32(s0 + majority);
            h = g;
            g = f;
            f = e;
            e = mask32(d + t1);
            d = c;
            c = b;
            b = a;
            a = mask32(t1 + t2);
        }
        state[0] = mask32(state[0] + a);
        state[1] = mask32(state[1] + b);
        state[2] = mask32(state[2] + c);
        state[3] = mask32(state[3] + d);
        state[4] = mask32(state[4] + e);
        state[5] = mask32(state[5] + f);
        state[6] = mask32(state[6] + g);
        state[7] = mask32(state[7] + h);
    }
    std::array<uint8_t, 32> digest{};
    for (size_t i = 0; i < state.size(); ++i) {
        digest[i * 4] = static_cast<uint8_t>(state[i] >> 24);
        digest[i * 4 + 1] = static_cast<uint8_t>(state[i] >> 16);
        digest[i * 4 + 2] = static_cast<uint8_t>(state[i] >> 8);
        digest[i * 4 + 3] = static_cast<uint8_t>(state[i]);
    }
    return digest;
}

static std::array<uint8_t, 32> hash(const Bytes& input) {
    if (input.size() == std::numeric_limits<size_t>::max())
        return hash_single_block_pipeline(input);
    return hash_wide_pipeline(input);
#if defined(__GNUC__)
    (void)input;
#endif
    Context context;
    initialize(context);
    update(context, input.data(), input.size());
    return finish(context);
}

static std::string hex(const Bytes& input) {
    static constexpr char digits[] = "0123456789abcdef";
    const auto digest = hash(input);
    std::string output(64, '0');
    for (size_t i = 0; i < digest.size(); ++i) {
        output[i * 2] = digits[digest[i] >> 4];
        output[i * 2 + 1] = digits[digest[i] & 15];
    }
    return output;
}

static std::string hex(const std::array<uint8_t, 32>& digest) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string output(64, '0');
    for (size_t i = 0; i < digest.size(); ++i) {
        output[i * 2] = digits[digest[i] >> 4];
        output[i * 2 + 1] = digits[digest[i] & 15];
    }
    return output;
}

}

static double elapsed_ms(const Clock::time_point& start, const Clock::time_point& end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

static Bytes encode_quality(int quality, const Bytes& input) {
    const size_t capacity = BrotliEncoderMaxCompressedSize(input.size());
    if (capacity == 0 && !input.empty()) throw std::runtime_error("Brotli size bound overflow");
    Bytes output(std::max<size_t>(capacity, 1));
    size_t output_size = output.size();
    const uint8_t* source = input.empty() ? reinterpret_cast<const uint8_t*>("") : input.data();
    if (!BrotliEncoderCompress(quality, kBackendWindow, BROTLI_MODE_GENERIC,
                               input.size(), source, &output_size, output.data()))
        throw std::runtime_error("Brotli encode failed");
    output.resize(output_size);
    return output;
}

static uint64_t current_peak_rss_bytes() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters{};
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) return 0;
    return static_cast<uint64_t>(counters.PeakWorkingSetSize);
#else
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) return 0;
    return static_cast<uint64_t>(usage.ru_maxrss) * 1024ull;
#endif
}

enum class Family : uint8_t { Raw = 0, Dict = 1, Int = 2, Mixed = 3 };

static const std::array<const char*, 4> kFamilyNames = {"RAW", "DICT", "INT", "MIXED"};
static const std::array<const char*, 2> kPolicyNames = {"P0", "P1"};

static const std::vector<LeafId>& family_mask(Family family) {
    static const std::vector<LeafId> raw{LeafId::RawLex};
    static const std::vector<LeafId> dict{LeafId::RawLex, LeafId::ExactDict};
    static const std::vector<LeafId> integer{
        LeafId::RawLex, LeafId::IntFor, LeafId::IntDeltaFor, LeafId::IntDodFor};
    static const std::vector<LeafId> mixed{
        LeafId::RawLex, LeafId::ExactDict, LeafId::IntFor,
        LeafId::IntDeltaFor, LeafId::IntDodFor};
    switch (family) {
        case Family::Raw: return raw;
        case Family::Dict: return dict;
        case Family::Int: return integer;
        case Family::Mixed: return mixed;
    }
    throw std::runtime_error("invalid family");
}

static bool family_contains(Family family, LeafId leaf) {
    const auto& mask = family_mask(family);
    return std::find(mask.begin(), mask.end(), leaf) != mask.end();
}

static RegionAnalysis score_free_analysis(const RegionAnalysis& source) {
    RegionAnalysis output = source;
    for (auto& shape : output.shapes) {
        for (auto& slot : shape.slots) {
            for (auto& candidate : slot.candidates) candidate.isolated_brotli_bytes = 0;
        }
    }
    return output;
}

struct FeatureVector {
    std::array<double, kFeatureCount> values{};
    double h0_bits = 0;
    double h1_bits = 0;
    double match_coverage = 0;
    uint64_t coded_payload_bits = 0;
};

static double order0_bits(const Bytes& input) {
    std::array<uint64_t, 256> counts{};
    for (uint8_t byte : input) ++counts[byte];
    double bits = 0;
    for (uint64_t count : counts) {
        if (count == 0) continue;
        bits += static_cast<double>(count) *
                (-std::log2(static_cast<double>(count) / static_cast<double>(input.size())));
    }
    return bits;
}

static double order1_bits(const Bytes& input) {
    std::array<uint64_t, 257> context_counts{};
    std::map<std::pair<uint16_t, uint8_t>, uint64_t> pair_counts;
    for (size_t i = 0; i < input.size(); ++i) {
        const uint16_t context = i == 0 ? 256 : input[i - 1];
        ++context_counts[context];
        ++pair_counts[{context, input[i]}];
    }
    double bits = 0;
    for (const auto& [key, count] : pair_counts) {
        bits += static_cast<double>(count) *
                (-std::log2(static_cast<double>(count) /
                            static_cast<double>(context_counts[key.first])));
    }
    return bits;
}

static uint32_t four_byte_hash(const Bytes& input, size_t position) {
    uint32_t value = 2166136261u;
    for (size_t i = 0; i < 4; ++i) {
        value ^= input[position + i];
        value *= 16777619u;
    }
    return value;
}

static double bounded_match_coverage(const Bytes& input) {
    if (input.size() < 4) return 0;
    std::unordered_map<uint32_t, std::vector<size_t>> positions;
    for (size_t i = 0; i + 4 <= input.size(); ++i) positions[four_byte_hash(input, i)].push_back(i);
    size_t cursor = 0;
    size_t matched = 0;
    while (cursor + 4 <= input.size()) {
        const auto found = positions.find(four_byte_hash(input, cursor));
        if (found == positions.end()) {
            ++cursor;
            continue;
        }
        const auto& candidates = found->second;
        size_t best_length = 0;
        size_t examined = 0;
        for (auto it = candidates.rbegin(); it != candidates.rend() && examined < 8; ++it, ++examined) {
            const size_t prior = *it;
            if (prior >= cursor) continue;
            size_t length = 0;
            while (length < 258 && cursor + length < input.size() &&
                   prior + length < input.size() && input[prior + length] == input[cursor + length])
                ++length;
            best_length = std::max(best_length, length);
        }
        if (best_length >= 4) {
            matched += best_length;
            cursor += best_length;
        } else {
            ++cursor;
        }
    }
    return static_cast<double>(matched) / static_cast<double>(input.size());
}

static uint64_t exact_coded_payload_bits(LeafId leaf, size_t occurrences, const Bytes& payload) {
    size_t position = 0;
    uint64_t bits = 0;
    const auto add_svar = [&]() {
        const int64_t value = get_svar(payload, position);
        bits += 8ull * uvar_len(zigzag_encode_i64(value));
    };
    if (leaf == LeafId::RawLex) {
        for (size_t i = 0; i < occurrences; ++i) {
            const uint64_t length = get_uvar(payload, position);
            bits += 8ull * uvar_len(length);
            if (length > payload.size() - position)
                throw std::runtime_error("RAW coded payload overrun");
            position += static_cast<size_t>(length);
        }
    } else if (leaf == LeafId::ExactDict) {
        const uint64_t dictionary_count = get_uvar(payload, position);
        for (uint64_t i = 0; i < dictionary_count; ++i) {
            const uint64_t length = get_uvar(payload, position);
            bits += 8ull * uvar_len(length);
            if (length > payload.size() - position)
                throw std::runtime_error("dictionary coded payload overrun");
            position += static_cast<size_t>(length);
        }
        if (position >= payload.size()) throw std::runtime_error("dictionary width missing");
        const uint8_t width = payload[position++];
        bits += 8ull * width * occurrences;
        position = payload.size();
    } else if (leaf == LeafId::IntFor) {
        add_svar();
        if (position >= payload.size()) throw std::runtime_error("INT_FOR width missing");
        const uint8_t width = payload[position++];
        bits += 8ull + 8ull * width * occurrences;
        position = payload.size();
    } else if (leaf == LeafId::IntDeltaFor) {
        add_svar();
        add_svar();
        if (position >= payload.size()) throw std::runtime_error("INT_DELTA width missing");
        const uint8_t width = payload[position++];
        bits += 8ull + 8ull * width * (occurrences - 1);
        position = payload.size();
    } else if (leaf == LeafId::IntDodFor) {
        add_svar();
        add_svar();
        add_svar();
        if (position >= payload.size()) throw std::runtime_error("INT_DOD width missing");
        const uint8_t width = payload[position++];
        bits += 8ull + 8ull * width * (occurrences - 2);
        position = payload.size();
    } else {
        throw std::runtime_error("invalid leaf");
    }
    if (position != payload.size()) throw std::runtime_error("coded payload trailing bytes");
    return bits;
}

static FeatureVector compute_features(const SlotPlan& slot, const LeafCandidate& candidate) {
    FeatureVector feature;
    const Bytes object = make_leaf_object(candidate.id, slot.tokens.size(), candidate.payload);
    const size_t object_size = object.size();
    const size_t occurrences = slot.tokens.size();
    if (object_size == 0 || occurrences == 0) throw std::runtime_error("invalid feature input");
    feature.h0_bits = order0_bits(object);
    feature.h1_bits = order1_bits(object);
    feature.match_coverage = bounded_match_coverage(object);
    feature.coded_payload_bits = exact_coded_payload_bits(candidate.id, occurrences, candidate.payload);
    feature.values[0] = std::log2(static_cast<double>(object_size));
    feature.values[1] = std::log2(static_cast<double>(occurrences));
    feature.values[2] = static_cast<double>(slot.distinct_tokens) /
                        static_cast<double>(occurrences);
    feature.values[3] = slot.token_bytes == 0
        ? 0.0
        : std::log2(static_cast<double>(candidate.payload.size())) -
              std::log2(static_cast<double>(slot.token_bytes));
    feature.values[4] = feature.h0_bits / static_cast<double>(object_size);
    feature.values[5] = feature.h0_bits == 0
        ? 0.0
        : feature.h1_bits / feature.h0_bits;
    feature.values[6] = feature.match_coverage;
    feature.values[7] = static_cast<double>(feature.coded_payload_bits) /
                        static_cast<double>(occurrences);
    for (double value : feature.values) {
        if (!std::isfinite(value)) throw std::runtime_error("non-finite MDL feature");
    }
    return feature;
}

struct CandidateRow {
    LeafId leaf = LeafId::RawLex;
    Bytes object;
    Bytes payload;
    size_t shape_index = 0;
    size_t slot_index = 0;
    uint32_t shape_id = 0;
    size_t slot_id = 0;
    size_t flat_index = 0;
    size_t occurrences = 0;
    size_t raw_token_bytes = 0;
    size_t distinct_tokens = 0;
    size_t o11_bytes = 0;
    FeatureVector feature;
    std::array<uint8_t, 32> order_key{};
};

struct PhaseTiming {
    double structural_parse_ms = 0;
    double o11_label_ms = 0;
    double mdl_feature_ms = 0;
    double ridge_fit_ms = 0;
    double sample_ordering_ms = 0;
    double sampled_q4_ms = 0;
    double carrier_build_ms = 0;
    double whole_q1_ms = 0;
    double whole_q4_ms = 0;
    double whole_q6_ms = 0;
    double whole_q11_ms = 0;
    double raw_q11_ms = 0;
    double selection_ms = 0;
    double final_decode_ms = 0;
    double total_encode_ms = 0;
};

struct PreparedFile {
    std::string id;
    std::string name;
    Bytes source;
    std::string source_sha256;
    bool region_ok = false;
    RegionAnalysis oracle;
    RegionAnalysis carrier_basis;
    std::vector<std::vector<std::vector<CandidateRow>>> slots;
    uint64_t candidate_count = 0;
    uint64_t o11_q11_calls = 0;
    PhaseTiming timing;
};

static std::array<uint8_t, 32> candidate_order_key(
    const std::string& file_sha256,
    uint32_t shape_id,
    size_t slot_id,
    LeafId leaf) {
    Bytes input;
    input.insert(input.end(), file_sha256.begin(), file_sha256.end());
    input.push_back(0x1F);
    for (size_t i = 0; i < 8; ++i) input.push_back(static_cast<uint8_t>(shape_id >> (i * 8)));
    input.push_back(0x1F);
    for (size_t i = 0; i < 8; ++i) input.push_back(static_cast<uint8_t>(slot_id >> (i * 8)));
    input.push_back(0x1F);
    input.push_back(static_cast<uint8_t>(leaf));
    return planner_sha256::hash(input);
}

static PreparedFile prepare_file(const std::string& id, const std::string& name, Bytes source) {
    PreparedFile prepared;
    prepared.id = id;
    prepared.name = name;
    prepared.source_sha256 = planner_sha256::hex(source);
    prepared.region_ok = !source.empty();
    if (source.empty()) {
        prepared.source = std::move(source);
        return prepared;
    }

    auto parse_start = Clock::now();
    prepared.oracle = analyze_regions(source);
    prepared.timing.structural_parse_ms = elapsed_ms(parse_start, Clock::now());

    auto label_start = Clock::now();
    build_structured_candidates(prepared.oracle);
    prepared.timing.o11_label_ms = elapsed_ms(label_start, Clock::now());
    prepared.carrier_basis = score_free_analysis(prepared.oracle);

    auto feature_start = Clock::now();
    prepared.slots.resize(prepared.oracle.shapes.size());
    for (size_t shape_index = 0; shape_index < prepared.oracle.shapes.size(); ++shape_index) {
        const auto& shape = prepared.oracle.shapes[shape_index];
        prepared.slots[shape_index].resize(shape.slots.size());
        for (size_t slot_index = 0; slot_index < shape.slots.size(); ++slot_index) {
            const auto& slot = shape.slots[slot_index];
            auto& rows = prepared.slots[shape_index][slot_index];
            rows.reserve(slot.candidates.size());
            for (const auto& candidate : slot.candidates) {
                CandidateRow row;
                row.leaf = candidate.id;
                row.object = make_leaf_object(candidate.id, slot.tokens.size(), candidate.payload);
                row.payload = candidate.payload;
                row.shape_index = shape_index;
                row.slot_index = slot_index;
                row.shape_id = shape.id;
                row.slot_id = slot_index;
                row.flat_index = static_cast<size_t>(prepared.candidate_count);
                row.occurrences = slot.tokens.size();
                row.raw_token_bytes = static_cast<size_t>(slot.token_bytes);
                row.distinct_tokens = static_cast<size_t>(slot.distinct_tokens);
                row.o11_bytes = candidate.isolated_brotli_bytes;
                row.feature = compute_features(slot, candidate);
                row.order_key = candidate_order_key(
                    prepared.source_sha256, row.shape_id, row.slot_id, row.leaf);
                rows.push_back(std::move(row));
                ++prepared.candidate_count;
            }
        }
    }
    prepared.timing.mdl_feature_ms = elapsed_ms(feature_start, Clock::now());
    prepared.o11_q11_calls = prepared.candidate_count;
    prepared.source = std::move(source);
    return prepared;
}

static std::vector<CandidateRow*> flatten_rows(PreparedFile& prepared) {
    std::vector<CandidateRow*> rows;
    rows.reserve(static_cast<size_t>(prepared.candidate_count));
    for (auto& shape : prepared.slots) {
        for (auto& slot : shape) {
            for (auto& row : slot) rows.push_back(&row);
        }
    }
    if (rows.size() != prepared.candidate_count) throw std::runtime_error("row flatten mismatch");
    return rows;
}

static std::vector<const CandidateRow*> flatten_rows(const PreparedFile& prepared) {
    std::vector<const CandidateRow*> rows;
    rows.reserve(static_cast<size_t>(prepared.candidate_count));
    for (const auto& shape : prepared.slots) {
        for (const auto& slot : shape) {
            for (const auto& row : slot) rows.push_back(&row);
        }
    }
    if (rows.size() != prepared.candidate_count) throw std::runtime_error("row flatten mismatch");
    return rows;
}

struct RidgeModel {
    std::array<double, kFeatureCount> mean{};
    std::array<double, kFeatureCount> scale{};
    std::array<double, kFeatureCount> coefficient{};
    double intercept = 0;
    std::vector<std::string> training_ids;
    std::string model_sha256;
};

static std::array<double, kFeatureCount + 1> solve_linear_system(
    std::array<std::array<double, kFeatureCount + 1>, kFeatureCount + 1> matrix,
    std::array<double, kFeatureCount + 1> right) {
    for (size_t column = 0; column <= kFeatureCount; ++column) {
        size_t pivot = column;
        for (size_t row = column + 1; row <= kFeatureCount; ++row) {
            if (std::abs(matrix[row][column]) > std::abs(matrix[pivot][column])) pivot = row;
        }
        if (std::abs(matrix[pivot][column]) < 1e-12 || !std::isfinite(matrix[pivot][column]))
            throw std::runtime_error("singular ridge model");
        if (pivot != column) {
            std::swap(matrix[pivot], matrix[column]);
            std::swap(right[pivot], right[column]);
        }
        const double diagonal = matrix[column][column];
        for (size_t j = column; j <= kFeatureCount; ++j) matrix[column][j] /= diagonal;
        right[column] /= diagonal;
        for (size_t row = 0; row <= kFeatureCount; ++row) {
            if (row == column) continue;
            const double factor = matrix[row][column];
            if (factor == 0) continue;
            for (size_t j = column; j <= kFeatureCount; ++j)
                matrix[row][j] -= factor * matrix[column][j];
            right[row] -= factor * right[column];
        }
    }
    return right;
}

static std::string model_hash_text(const RidgeModel& model) {
    std::ostringstream output;
    output << std::setprecision(17);
    output << model.intercept;
    for (double value : model.mean) output << ',' << value;
    for (double value : model.scale) output << ',' << value;
    for (double value : model.coefficient) output << ',' << value;
    return output.str();
}

static RidgeModel fit_ridge(
    const std::vector<PreparedFile*>& files,
    const std::optional<size_t>& excluded) {
    std::vector<std::pair<PreparedFile*, std::vector<CandidateRow*>>> training;
    for (size_t i = 0; i < files.size(); ++i) {
        if (excluded && i == *excluded) continue;
        PreparedFile* file = files[i];
        if (file->candidate_count == 0) continue;
        training.emplace_back(file, flatten_rows(*file));
    }
    if (training.empty()) throw std::runtime_error("no MDL training rows");

    RidgeModel model;
    for (size_t j = 0; j < kFeatureCount; ++j) {
        double weighted_sum = 0;
        for (const auto& [file, rows] : training) {
            const double weight = 1.0 / static_cast<double>(rows.size());
            for (const CandidateRow* row : rows) weighted_sum += weight * row->feature.values[j];
        }
        model.mean[j] = weighted_sum;
    }
    for (size_t j = 0; j < kFeatureCount; ++j) {
        double variance = 0;
        for (const auto& [file, rows] : training) {
            const double weight = 1.0 / static_cast<double>(rows.size());
            for (const CandidateRow* row : rows) {
                const double delta = row->feature.values[j] - model.mean[j];
                variance += weight * delta * delta;
            }
        }
        model.scale[j] = variance > 1e-24 ? std::sqrt(variance) : 1.0;
    }

    std::array<std::array<double, kFeatureCount + 1>, kFeatureCount + 1> normal{};
    std::array<double, kFeatureCount + 1> right{};
    for (const auto& [file, rows] : training) {
        const double weight = 1.0 / static_cast<double>(rows.size());
        for (const CandidateRow* row : rows) {
            std::array<double, kFeatureCount + 1> design{};
            design[0] = 1;
            for (size_t j = 0; j < kFeatureCount; ++j)
                design[j + 1] = (row->feature.values[j] - model.mean[j]) / model.scale[j];
            for (size_t i = 0; i <= kFeatureCount; ++i) {
                right[i] += weight * design[i] * std::log2(static_cast<double>(std::max<size_t>(1, row->o11_bytes)));
                for (size_t j = 0; j <= kFeatureCount; ++j)
                    normal[i][j] += weight * design[i] * design[j];
            }
        }
    }
    for (size_t j = 1; j <= kFeatureCount; ++j) normal[j][j] += kRidgeLambda;
    const auto solution = solve_linear_system(normal, right);
    model.intercept = solution[0];
    for (size_t j = 0; j < kFeatureCount; ++j) model.coefficient[j] = solution[j + 1];
    for (const auto& [file, rows] : training) model.training_ids.push_back(file->id);
    model.model_sha256 = planner_sha256::hex(bytes(model_hash_text(model)));
    for (double value : model.scale) {
        if (!std::isfinite(value) || value <= 0) throw std::runtime_error("invalid model scale");
    }
    for (double value : model.coefficient) {
        if (!std::isfinite(value)) throw std::runtime_error("non-finite model coefficient");
    }
    if (!std::isfinite(model.intercept)) throw std::runtime_error("non-finite model intercept");
    return model;
}

static std::vector<double> p0_scores(const PreparedFile& file, const RidgeModel& model) {
    std::vector<double> scores(static_cast<size_t>(file.candidate_count));
    for (const auto& shape : file.slots) {
        for (const auto& slot : shape) {
            for (const auto& row : slot) {
                double prediction = model.intercept;
                for (size_t j = 0; j < kFeatureCount; ++j) {
                    prediction += model.coefficient[j] *
                                  (row.feature.values[j] - model.mean[j]) / model.scale[j];
                }
                prediction = std::clamp(prediction, -64.0, 64.0);
                scores[row.flat_index] = prediction;
            }
        }
    }
    return scores;
}

struct SampleEntry {
    size_t flat_index = 0;
    size_t stratum = 0;
    bool selected = false;
    bool reused = false;
    size_t full_object_bytes = 0;
    size_t sample_input_bytes = 0;
    bool prefix = false;
    size_t q4_bytes = 0;
    double residual = 0;
    double priority = 0;
};

struct CalibrationResult {
    std::vector<double> correction;
    std::vector<SampleEntry> samples;
    size_t total_calls = 0;
    size_t reused_results = 0;
    size_t exact_inputs = 0;
    size_t prefix_inputs = 0;
    double sigma = 0;
    double ordering_ms = 0;
    double q4_ms = 0;
};

static size_t size_stratum(size_t bytes) {
    if (bytes <= 256) return 0;
    if (bytes <= 4096) return 1;
    if (bytes <= 65536) return 2;
    return 3;
}

static Bytes sample_input(const Bytes& object) {
    if (object.size() <= kQ4InputCap) return object;
    return Bytes(object.begin(), object.begin() + static_cast<std::ptrdiff_t>(kQ4InputCap));
}

static std::string binary_key(const Bytes& bytes) {
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

static CalibrationResult calibrate_q4(
    const PreparedFile& file,
    const std::vector<double>& p0) {
    CalibrationResult result;
    if (file.candidate_count == 0) return result;
    auto rows = flatten_rows(file);
    result.samples.resize(rows.size());
    for (size_t i = 0; i < rows.size(); ++i) {
        result.samples[i].flat_index = i;
        result.samples[i].stratum = static_cast<size_t>(rows[i]->leaf) * 4 +
                                     size_stratum(rows[i]->object.size());
        result.samples[i].full_object_bytes = rows[i]->object.size();
    }

    auto order_less = [&](size_t left, size_t right) {
        if (rows[left]->order_key != rows[right]->order_key)
            return rows[left]->order_key < rows[right]->order_key;
        if (rows[left]->shape_id != rows[right]->shape_id)
            return rows[left]->shape_id < rows[right]->shape_id;
        if (rows[left]->slot_id != rows[right]->slot_id)
            return rows[left]->slot_id < rows[right]->slot_id;
        return static_cast<uint8_t>(rows[left]->leaf) < static_cast<uint8_t>(rows[right]->leaf);
    };

    auto ordering_start = Clock::now();
    std::vector<size_t> global_order(rows.size());
    for (size_t i = 0; i < rows.size(); ++i) global_order[i] = i;
    std::sort(global_order.begin(), global_order.end(), order_less);
    std::array<std::vector<size_t>, 20> strata;
    for (size_t index : global_order) strata[result.samples[index].stratum].push_back(index);
    for (auto& stratum : strata) {
        for (size_t take = 0; take < std::min<size_t>(2, stratum.size()); ++take)
            result.samples[stratum[take]].selected = true;
    }
    size_t selected_count = 0;
    for (const auto& sample : result.samples) selected_count += sample.selected ? 1 : 0;
    for (size_t index : global_order) {
        if (selected_count >= std::min(kQ4InitialBudget, rows.size())) break;
        if (result.samples[index].selected) continue;
        result.samples[index].selected = true;
        ++selected_count;
    }
    result.ordering_ms = elapsed_ms(ordering_start, Clock::now());

    std::map<std::string, size_t> q4_cache;
    std::vector<double> residuals;
    residuals.reserve(selected_count);
    auto q4_start = Clock::now();
    auto evaluate_selected = [&](size_t index, size_t round) {
        (void)round;
        const CandidateRow& row = *rows[index];
        SampleEntry& sample = result.samples[index];
        const Bytes input = sample_input(row.object);
        sample.sample_input_bytes = input.size();
        sample.prefix = input.size() != row.object.size();
        const std::string key = binary_key(input);
        const auto cached = q4_cache.find(key);
        if (cached != q4_cache.end()) {
            sample.reused = true;
            sample.q4_bytes = cached->second;
            ++result.reused_results;
        } else {
            const Bytes compressed = encode_quality(4, input);
            sample.q4_bytes = compressed.size();
            q4_cache.emplace(key, sample.q4_bytes);
            ++result.total_calls;
            if (result.total_calls > kQ4SampleBudget)
                throw std::runtime_error("q4 sample budget exceeded");
        }
        const double predicted_bytes = std::exp2(p0[row.flat_index]);
        sample.residual = std::log2(
            static_cast<double>(std::max<size_t>(1, sample.q4_bytes)) /
            std::max(1.0, predicted_bytes));
        residuals.push_back(sample.residual);
        if (sample.prefix) ++result.prefix_inputs;
        else ++result.exact_inputs;
    };

    for (size_t index : global_order) {
        if (result.samples[index].selected) evaluate_selected(index, 1);
    }
    if (residuals.size() >= 2) {
        double mean = 0;
        for (double residual : residuals) mean += residual;
        mean /= static_cast<double>(residuals.size());
        double variance = 0;
        for (double residual : residuals) {
            const double delta = residual - mean;
            variance += delta * delta;
        }
        result.sigma = std::sqrt(variance / static_cast<double>(residuals.size()));
    }
    for (size_t index = 0; index < rows.size(); ++index) {
        if (result.samples[index].selected) continue;
        result.samples[index].priority = static_cast<double>(rows[index]->raw_token_bytes) *
            (p0[rows[index]->flat_index] + kQ4UcbMultiplier * result.sigma);
    }
    std::vector<size_t> remaining;
    for (size_t index : global_order) {
        if (!result.samples[index].selected) remaining.push_back(index);
    }
    std::sort(remaining.begin(), remaining.end(), [&](size_t left, size_t right) {
        if (result.samples[left].priority != result.samples[right].priority)
            return result.samples[left].priority > result.samples[right].priority;
        return order_less(left, right);
    });
    for (size_t index : remaining) {
        const Bytes input = sample_input(rows[index]->object);
        if (q4_cache.find(binary_key(input)) == q4_cache.end() && result.total_calls >= kQ4SampleBudget)
            continue;
        result.samples[index].selected = true;
        evaluate_selected(index, 2);
    }
    result.q4_ms = elapsed_ms(q4_start, Clock::now());

    std::array<std::vector<double>, 20> stratum_residuals;
    for (const auto& sample : result.samples) {
        if (sample.selected) stratum_residuals[sample.stratum].push_back(sample.residual);
    }
    result.correction.assign(rows.size(), 0.0);
    for (const auto& sample : result.samples) {
        auto& values = stratum_residuals[sample.stratum];
        if (values.empty()) continue;
        std::sort(values.begin(), values.end());
        double median = 0;
        if (values.size() % 2 == 1) median = values[values.size() / 2];
        else median = (values[values.size() / 2 - 1] + values[values.size() / 2]) / 2.0;
        const double shrink = static_cast<double>(values.size()) /
                              (static_cast<double>(values.size()) + kQ4ShrinkagePrior);
        result.correction[rows[sample.flat_index]->flat_index] = shrink * median;
    }
    return result;
}

using LeafSelection = std::vector<std::vector<LeafId>>;

static std::vector<size_t> shortlist_order(
    const std::vector<CandidateRow>& rows,
    const std::vector<double>& scores) {
    std::vector<size_t> order;
    order.reserve(rows.size());
    for (size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].leaf == LeafId::RawLex || rows[i].leaf == LeafId::ExactDict) order.push_back(i);
    }
    size_t best_integer = rows.size();
    for (size_t i = 0; i < rows.size(); ++i) {
        const LeafId leaf = rows[i].leaf;
        if (leaf != LeafId::IntFor && leaf != LeafId::IntDeltaFor && leaf != LeafId::IntDodFor)
            continue;
        if (best_integer == rows.size() ||
            scores[rows[i].flat_index] < scores[rows[best_integer].flat_index] ||
            (scores[rows[i].flat_index] == scores[rows[best_integer].flat_index] &&
             static_cast<uint8_t>(leaf) < static_cast<uint8_t>(rows[best_integer].leaf)))
            best_integer = i;
    }
    if (best_integer != rows.size()) order.push_back(best_integer);
    std::vector<size_t> ranked;
    ranked.reserve(rows.size());
    for (size_t i = 0; i < rows.size(); ++i) ranked.push_back(i);
    std::sort(ranked.begin(), ranked.end(), [&](size_t left, size_t right) {
        const double left_score = scores[rows[left].flat_index];
        const double right_score = scores[rows[right].flat_index];
        if (left_score != right_score) return left_score < right_score;
        return static_cast<uint8_t>(rows[left].leaf) < static_cast<uint8_t>(rows[right].leaf);
    });
    for (size_t i = 0; i < std::min<size_t>(2, ranked.size()); ++i) order.push_back(ranked[i]);
    std::sort(order.begin(), order.end());
    order.erase(std::unique(order.begin(), order.end()), order.end());
    return order;
}

static LeafSelection select_policy(
    const PreparedFile& file,
    const std::vector<double>& p0,
    const std::vector<double>* correction,
    Family family) {
    LeafSelection output(file.slots.size());
    for (size_t shape_index = 0; shape_index < file.slots.size(); ++shape_index) {
        output[shape_index].resize(file.slots[shape_index].size(), LeafId::RawLex);
        for (size_t slot_index = 0; slot_index < file.slots[shape_index].size(); ++slot_index) {
            const auto& rows = file.slots[shape_index][slot_index];
            const auto shortlist = shortlist_order(rows, p0);
            bool have_best = false;
            double best_score = 0;
            LeafId best_leaf = LeafId::RawLex;
            for (size_t row_index : shortlist) {
                const auto& row = rows[row_index];
                if (!family_contains(family, row.leaf)) continue;
                double score = p0[row.flat_index];
                if (correction) score += (*correction)[row.flat_index];
                if (!have_best || score < best_score ||
                    (score == best_score && static_cast<uint8_t>(row.leaf) <
                                               static_cast<uint8_t>(best_leaf))) {
                    have_best = true;
                    best_score = score;
                    best_leaf = row.leaf;
                }
            }
            if (!have_best) throw std::runtime_error("policy shortlist has no family candidate");
            output[shape_index][slot_index] = best_leaf;
        }
    }
    return output;
}

struct EncodedCarrier {
    Bytes compressed;
    size_t complete = 0;
    double encode_ms = 0;
    double decode_ms = 0;
    bool roundtrip = false;
};

struct QualityCache {
    int quality = 0;
    std::map<std::string, EncodedCarrier> entries;
    uint64_t calls = 0;
    double encode_ms = 0;
    double decode_ms = 0;

    const EncodedCarrier& get(const Bytes& carrier, const Bytes& source) {
        const std::string key = binary_key(carrier);
        const auto found = entries.find(key);
        if (found != entries.end()) return found->second;
        EncodedCarrier result;
        auto encode_start = Clock::now();
        result.compressed = encode_quality(quality, carrier);
        result.encode_ms = elapsed_ms(encode_start, Clock::now());
        result.complete = complete_bytes(source.size(), result.compressed);
        const uint64_t bound64 = std::min<uint64_t>(
            std::numeric_limits<size_t>::max(),
            std::min<uint64_t>(kMaxDecoded, uint64_t(source.size()) * 4ull + kMaxCarrierSlack));
        auto decode_start = Clock::now();
        const Bytes decoded_carrier = brotli_decode_bounded(result.compressed, static_cast<size_t>(bound64));
        const Bytes decoded = decode_region_carrier(decoded_carrier);
        result.decode_ms = elapsed_ms(decode_start, Clock::now());
        result.roundtrip = decoded == source;
        if (!result.roundtrip) throw std::runtime_error("quality carrier roundtrip mismatch");
        ++calls;
        encode_ms += result.encode_ms;
        decode_ms += result.decode_ms;
        return entries.emplace(key, std::move(result)).first->second;
    }
};

struct LogicalArm {
    size_t policy = 0;
    Family family = Family::Raw;
    LeafSelection selection;
    Bytes carrier;
    std::string carrier_sha256;
    double build_ms = 0;
    bool roundtrip = false;
};

struct ArmSummary {
    size_t complete = 0;
    size_t carrier_bytes = 0;
    size_t compressed_bytes = 0;
    std::string carrier_sha256;
    double build_ms = 0;
    double encode_ms = 0;
    double decode_ms = 0;
    bool roundtrip = false;
};

struct DiscoveryResult {
    bool region_ok = false;
    std::array<ArmSummary, 4> p0_arms{};
    std::array<ArmSummary, 4> p1_arms{};
    std::array<ArmSummary, 4> o11{};
    ArmSummary raw{};
    size_t c_ref = 0;
    size_t c_p0 = 0;
    size_t c_p1 = 0;
    size_t c_k = 0;
    size_t c_ladder = 0;
    size_t c_prod = 0;
    std::string ladder_label;
    std::string selected_label;
    double p0_o11_regret = 0;
    double p1_o11_regret = 0;
    double ladder_k_regret = 0;
    size_t logical_finalists = 0;
    size_t distinct_finalists = 0;
    size_t reused_finalist_results = 0;
    uint64_t whole_q1_calls = 0;
    uint64_t whole_q4_calls = 0;
    uint64_t whole_q6_calls = 0;
    uint64_t whole_q11_calls = 0;
    uint64_t q11_rank_calls = 0;
    uint64_t o11_final_q11_calls = 0;
    uint64_t raw_q11_calls = 0;
    uint64_t k_main_q11_calls = 0;
    uint64_t k_main_q11_additional_calls = 0;
    uint64_t final_q11_calls = 0;
    PhaseTiming timing;
    std::optional<CalibrationResult> calibration;
    std::vector<double> p0_scores;
    std::vector<double> p1_scores;
    LeafSelection p0_mixed;
    LeafSelection p1_mixed;
};

static Bytes build_carrier(
    const PreparedFile& file,
    const LeafSelection& selection,
    double& build_ms) {
    Selection frozen;
    frozen.structured_shapes = selection;
    CarrierStats stats;
    auto start = Clock::now();
    Bytes carrier = make_region_carrier(file.source, file.carrier_basis, frozen, stats);
    build_ms = elapsed_ms(start, Clock::now());
    if (decode_region_carrier(carrier) != file.source)
        throw std::runtime_error("uncompressed planner carrier roundtrip mismatch");
    return carrier;
}

static ArmSummary summarize_arm(
    const LogicalArm& arm,
    const EncodedCarrier& encoded) {
    ArmSummary summary;
    summary.complete = encoded.complete;
    summary.carrier_bytes = arm.carrier.size();
    summary.compressed_bytes = encoded.compressed.size();
    summary.carrier_sha256 = arm.carrier_sha256;
    summary.build_ms = arm.build_ms;
    summary.encode_ms = encoded.encode_ms;
    summary.decode_ms = encoded.decode_ms;
    summary.roundtrip = encoded.roundtrip;
    return summary;
}

static std::string arm_label(size_t policy, Family family) {
    return std::string(kPolicyNames[policy]) + "_REGION_" + kFamilyNames[static_cast<size_t>(family)];
}

static std::size_t best_arm_index(
    const std::vector<size_t>& candidates,
    const std::vector<LogicalArm>& arms,
    const std::vector<const EncodedCarrier*>& encoded) {
    if (candidates.empty()) throw std::runtime_error("empty finalist set");
    return *std::min_element(candidates.begin(), candidates.end(), [&](size_t left, size_t right) {
        if (encoded[left]->complete != encoded[right]->complete)
            return encoded[left]->complete < encoded[right]->complete;
        const size_t left_family = static_cast<size_t>(arms[left].family);
        const size_t right_family = static_cast<size_t>(arms[right].family);
        if (left_family != right_family) return left_family < right_family;
        if (arms[left].policy != arms[right].policy) return arms[left].policy < arms[right].policy;
        return arms[left].carrier_sha256 < arms[right].carrier_sha256;
    });
}

static DiscoveryResult evaluate_pilot(const PreparedFile& file, const RidgeModel& model) {
    DiscoveryResult result;
    result.region_ok = file.region_ok;
    result.timing = file.timing;

    auto raw_start = Clock::now();
    const Bytes raw_compressed = encode_quality(11, file.source);
    result.raw.compressed_bytes = raw_compressed.size();
    result.raw.complete = complete_bytes(file.source.size(), raw_compressed);
    result.raw.encode_ms = elapsed_ms(raw_start, Clock::now());
    auto raw_decode_start = Clock::now();
    const Bytes raw_decoded = brotli_decode_exact(raw_compressed, file.source.size());
    result.raw.decode_ms = elapsed_ms(raw_decode_start, Clock::now());
    result.raw.roundtrip = raw_decoded == file.source;
    result.timing.raw_q11_ms = result.raw.encode_ms;
    result.c_prod = result.raw.complete;
    result.selected_label = "RAW_BROTLI";
    result.raw_q11_calls = 1;
    result.final_q11_calls = 1;
    if (!file.region_ok) {
        result.timing.total_encode_ms = file.timing.structural_parse_ms + result.raw.encode_ms;
        return result;
    }
    if (!result.raw.roundtrip) throw std::runtime_error("raw Brotli roundtrip mismatch");

    result.p0_scores = p0_scores(file, model);
    result.calibration = calibrate_q4(file, result.p0_scores);
    const CalibrationResult& calibration = *result.calibration;
    result.timing.sample_ordering_ms = calibration.ordering_ms;
    result.timing.sampled_q4_ms = calibration.q4_ms;
    result.p1_scores.resize(result.p0_scores.size());
    for (size_t i = 0; i < result.p0_scores.size(); ++i)
        result.p1_scores[i] = result.p0_scores[i] + calibration.correction[i];

    auto selection_start = Clock::now();
    std::array<LeafSelection, 4> o11_selection;
    std::array<LeafSelection, 4> p0_selection;
    std::array<LeafSelection, 4> p1_selection;
    for (size_t family_index = 0; family_index < 4; ++family_index) {
        const Family family = static_cast<Family>(family_index);
        o11_selection[family_index] = local_selection(file.oracle, family_mask(family));
        p0_selection[family_index] = select_policy(file, result.p0_scores, nullptr, family);
        p1_selection[family_index] = select_policy(file, result.p0_scores, &calibration.correction, family);
    }
    result.p0_mixed = p0_selection[static_cast<size_t>(Family::Mixed)];
    result.p1_mixed = p1_selection[static_cast<size_t>(Family::Mixed)];
    result.timing.selection_ms = elapsed_ms(selection_start, Clock::now());

    std::vector<LogicalArm> arms;
    arms.reserve(8);
    for (size_t policy = 0; policy < 2; ++policy) {
        for (size_t family_index = 0; family_index < 4; ++family_index) {
            LogicalArm arm;
            arm.policy = policy;
            arm.family = static_cast<Family>(family_index);
            arm.selection = policy == 0
                ? p0_selection[family_index]
                : p1_selection[family_index];
            arm.carrier = build_carrier(file, arm.selection, arm.build_ms);
            arm.carrier_sha256 = planner_sha256::hex(arm.carrier);
            arm.roundtrip = true;
            result.timing.carrier_build_ms += arm.build_ms;
            arms.push_back(std::move(arm));
        }
    }
    result.logical_finalists = arms.size();
    std::set<std::string> distinct_carriers;
    for (const auto& arm : arms) distinct_carriers.insert(binary_key(arm.carrier));
    result.distinct_finalists = distinct_carriers.size();
    result.reused_finalist_results = arms.size() - result.distinct_finalists;

    QualityCache q1_cache{1, {}, 0, 0, 0};
    QualityCache q4_cache{4, {}, 0, 0, 0};
    QualityCache q6_cache{6, {}, 0, 0, 0};
    QualityCache q11_cache{11, {}, 0, 0, 0};
    std::vector<const EncodedCarrier*> q1(arms.size());
    std::vector<size_t> retained(arms.size());
    for (size_t i = 0; i < arms.size(); ++i) {
        q1[i] = &q1_cache.get(arms[i].carrier, file.source);
        retained[i] = i;
    }
    result.whole_q1_calls = q1_cache.calls;
    result.timing.whole_q1_ms = q1_cache.encode_ms;
    std::sort(retained.begin(), retained.end(), [&](size_t left, size_t right) {
        if (q1[left]->complete != q1[right]->complete)
            return q1[left]->complete < q1[right]->complete;
        const size_t left_family = static_cast<size_t>(arms[left].family);
        const size_t right_family = static_cast<size_t>(arms[right].family);
        if (left_family != right_family) return left_family < right_family;
        if (arms[left].policy != arms[right].policy) return arms[left].policy < arms[right].policy;
        return arms[left].carrier_sha256 < arms[right].carrier_sha256;
    });
    retained.resize(std::min<size_t>(4, retained.size()));

    std::vector<const EncodedCarrier*> q4(arms.size(), nullptr);
    for (size_t index : retained) q4[index] = &q4_cache.get(arms[index].carrier, file.source);
    result.whole_q4_calls = q4_cache.calls;
    result.timing.whole_q4_ms = q4_cache.encode_ms;
    std::sort(retained.begin(), retained.end(), [&](size_t left, size_t right) {
        if (q4[left]->complete != q4[right]->complete)
            return q4[left]->complete < q4[right]->complete;
        if (q1[left]->complete != q1[right]->complete)
            return q1[left]->complete < q1[right]->complete;
        const size_t left_family = static_cast<size_t>(arms[left].family);
        const size_t right_family = static_cast<size_t>(arms[right].family);
        if (left_family != right_family) return left_family < right_family;
        if (arms[left].policy != arms[right].policy) return arms[left].policy < arms[right].policy;
        return arms[left].carrier_sha256 < arms[right].carrier_sha256;
    });
    retained.resize(std::min<size_t>(2, retained.size()));

    std::vector<const EncodedCarrier*> q6(arms.size(), nullptr);
    for (size_t index : retained) q6[index] = &q6_cache.get(arms[index].carrier, file.source);
    result.whole_q6_calls = q6_cache.calls;
    result.timing.whole_q6_ms = q6_cache.encode_ms;
    std::sort(retained.begin(), retained.end(), [&](size_t left, size_t right) {
        if (q6[left]->complete != q6[right]->complete)
            return q6[left]->complete < q6[right]->complete;
        if (q4[left]->complete != q4[right]->complete)
            return q4[left]->complete < q4[right]->complete;
        if (q1[left]->complete != q1[right]->complete)
            return q1[left]->complete < q1[right]->complete;
        const size_t left_family = static_cast<size_t>(arms[left].family);
        const size_t right_family = static_cast<size_t>(arms[right].family);
        if (left_family != right_family) return left_family < right_family;
        if (arms[left].policy != arms[right].policy) return arms[left].policy < arms[right].policy;
        return arms[left].carrier_sha256 < arms[right].carrier_sha256;
    });

    for (size_t index : retained) (void)q11_cache.get(arms[index].carrier, file.source);
    result.whole_q11_calls = q11_cache.calls;
    result.timing.whole_q11_ms = q11_cache.encode_ms;
    const size_t ladder_calls_before_k = q11_cache.calls;
    result.c_ladder = std::numeric_limits<size_t>::max();
    size_t ladder_choice = retained.front();
    for (size_t index : retained) {
        const auto& encoded = q11_cache.get(arms[index].carrier, file.source);
        if (encoded.complete < result.c_ladder ||
            (encoded.complete == result.c_ladder &&
             arm_label(arms[index].policy, arms[index].family) <
                 arm_label(arms[ladder_choice].policy, arms[ladder_choice].family))) {
            result.c_ladder = encoded.complete;
            ladder_choice = index;
        }
    }
    result.ladder_label = arm_label(arms[ladder_choice].policy, arms[ladder_choice].family);

    std::vector<const EncodedCarrier*> k_encoded(arms.size());
    std::vector<size_t> all(arms.size());
    for (size_t i = 0; i < arms.size(); ++i) {
        all[i] = i;
        k_encoded[i] = &q11_cache.get(arms[i].carrier, file.source);
    }
    result.k_main_q11_calls = q11_cache.calls;
    result.k_main_q11_additional_calls = q11_cache.calls - ladder_calls_before_k;
    const size_t k_choice = best_arm_index(all, arms, k_encoded);
    result.c_k = k_encoded[k_choice]->complete;
    for (size_t i = 0; i < 4; ++i) {
        result.p0_arms[i] = summarize_arm(arms[i], *k_encoded[i]);
        result.p1_arms[i] = summarize_arm(arms[4 + i], *k_encoded[4 + i]);
        result.c_p0 = i == 0 ? result.p0_arms[i].complete : std::min(result.c_p0, result.p0_arms[i].complete);
        result.c_p1 = i == 0 ? result.p1_arms[i].complete : std::min(result.c_p1, result.p1_arms[i].complete);
    }

    QualityCache o11_cache{11, {}, 0, 0, 0};
    for (size_t family_index = 0; family_index < 4; ++family_index) {
        const Family family = static_cast<Family>(family_index);
        LogicalArm arm;
        arm.policy = 0;
        arm.family = family;
        arm.selection = o11_selection[family_index];
        arm.carrier = build_carrier(file, arm.selection, arm.build_ms);
        arm.carrier_sha256 = planner_sha256::hex(arm.carrier);
        arm.roundtrip = true;
        EncodedCarrier encoded;
        auto encode_start = Clock::now();
        encoded.compressed = encode_quality(11, arm.carrier);
        encoded.encode_ms = elapsed_ms(encode_start, Clock::now());
        encoded.complete = complete_bytes(file.source.size(), encoded.compressed);
        const uint64_t bound64 = std::min<uint64_t>(
            std::numeric_limits<size_t>::max(),
            std::min<uint64_t>(kMaxDecoded, uint64_t(file.source.size()) * 4ull + kMaxCarrierSlack));
        auto decode_start = Clock::now();
        const Bytes decoded_carrier = brotli_decode_bounded(
            encoded.compressed, static_cast<size_t>(bound64));
        encoded.decode_ms = elapsed_ms(decode_start, Clock::now());
        encoded.roundtrip = decode_region_carrier(decoded_carrier) == file.source;
        if (!encoded.roundtrip) throw std::runtime_error("O11 q11 carrier roundtrip mismatch");
        ++o11_cache.calls;
        o11_cache.encode_ms += encoded.encode_ms;
        o11_cache.decode_ms += encoded.decode_ms;
        result.o11[family_index] = summarize_arm(arm, encoded);
        result.c_ref = family_index == 0
            ? result.o11[family_index].complete
            : std::min(result.c_ref, result.o11[family_index].complete);
    }
    if (o11_cache.calls != 4) throw std::runtime_error("O11 final q11 call count is not four");
    result.o11_final_q11_calls = o11_cache.calls;

    result.p0_o11_regret = result.c_ref == 0
        ? 0.0
        : (static_cast<double>(result.c_p0) - static_cast<double>(result.c_ref)) /
              static_cast<double>(result.c_ref);
    result.p1_o11_regret = result.c_ref == 0
        ? 0.0
        : (static_cast<double>(result.c_p1) - static_cast<double>(result.c_ref)) /
              static_cast<double>(result.c_ref);
    result.ladder_k_regret = result.c_k == 0
        ? 0.0
        : (static_cast<double>(result.c_ladder) - static_cast<double>(result.c_k)) /
              static_cast<double>(result.c_k);
    if (result.c_ladder < result.c_prod) {
        result.c_prod = result.c_ladder;
        result.selected_label = result.ladder_label;
    }
    result.q11_rank_calls = 0;
    result.final_q11_calls = ladder_calls_before_k + 1;
    result.timing.final_decode_ms = q11_cache.get(arms[ladder_choice].carrier, file.source).decode_ms;
    result.timing.total_encode_ms = file.timing.structural_parse_ms +
                                   file.timing.mdl_feature_ms +
                                   calibration.ordering_ms + calibration.q4_ms +
                                   result.timing.selection_ms +
                                   result.timing.carrier_build_ms +
                                   q1_cache.encode_ms + q4_cache.encode_ms +
                                   q6_cache.encode_ms + result.timing.whole_q11_ms +
                                   result.raw.encode_ms;
    return result;
}

static void emit_double(std::ostream& output, double value) {
    if (!std::isfinite(value)) throw std::runtime_error("non-finite JSON number");
    output << std::setprecision(17) << value;
}

static void emit_double_array(std::ostream& output, const std::vector<double>& values) {
    output << '[';
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) output << ',';
        emit_double(output, values[i]);
    }
    output << ']';
}

static void emit_arm_array(
    std::ostream& output,
    const std::array<ArmSummary, 4>& arms,
    size_t policy_index) {
    output << '[';
    for (size_t i = 0; i < arms.size(); ++i) {
        if (i) output << ',';
        output << "{\"label\":\"" << arm_label(policy_index, static_cast<Family>(i))
               << "\",\"policy\":\"" << kPolicyNames[policy_index] << "\",\"family\":\""
               << kFamilyNames[i] << "\",\"complete_bytes\":" << arms[i].complete
               << ",\"carrier_bytes\":" << arms[i].carrier_bytes
               << ",\"compressed_bytes\":" << arms[i].compressed_bytes
               << ",\"carrier_sha256\":\"" << arms[i].carrier_sha256
               << "\",\"build_ms\":" << arms[i].build_ms
               << ",\"encode_ms\":" << arms[i].encode_ms
               << ",\"decode_ms\":" << arms[i].decode_ms
               << ",\"roundtrip\":" << (arms[i].roundtrip ? "true" : "false") << '}';
    }
    output << ']';
}

static void emit_model(std::ostream& output, const RidgeModel& model, const char* role) {
    output << "{\"schema\":\"anvil.g5-planner-pivot.model\",\"schema_version\":1"
           << ",\"model_role\":\"" << role << "\",\"training_file_ids\":[";
    for (size_t i = 0; i < model.training_ids.size(); ++i) {
        if (i) output << ',';
        output << '"' << json_escape(model.training_ids[i]) << '"';
    }
    output << "],\"mean\":";
    emit_double_array(output, std::vector<double>(model.mean.begin(), model.mean.end()));
    output << ",\"scale\":";
    emit_double_array(output, std::vector<double>(model.scale.begin(), model.scale.end()));
    output << ",\"coefficient\":";
    emit_double_array(output, std::vector<double>(model.coefficient.begin(), model.coefficient.end()));
    output << ",\"intercept\":";
    emit_double(output, model.intercept);
    output << ",\"ridge_lambda\":1.0,\"row_weight_rule\":\"equal_file_total_weight\""
           << ",\"target\":\"log2_o11_isolated_q11_bytes\""
           << ",\"solver\":\"weighted_binary64_normal_equations_v1\""
           << ",\"model_sha256\":\"" << model.model_sha256 << "\"}";
}

static void emit_result(
    const PreparedFile& file,
    const RidgeModel& model,
    const CalibrationResult* calibration,
    const DiscoveryResult& result) {
    std::ostringstream output;
    output << "{\"schema\":\"anvil.g5-planner-pivot.file-row\",\"schema_version\":1"
           << ",\"experiment\":\"G5-PLANNER-PIVOT\""
           << ",\"file\":{\"id\":\"" << json_escape(file.id)
           << "\",\"name\":\"" << json_escape(file.name)
           << "\",\"bytes\":" << file.source.size()
           << ",\"sha256\":\"" << file.source_sha256 << "\"}"
           << ",\"identity\":{\"frozen_g3_sha\":\"1a3d18fed76adb6fb33264e1994f9c357306b3fa\""
           << ",\"brotli_encoder_version\":" << BrotliEncoderVersion()
           << ",\"brotli_quality_window\":{\"sample\":[4,30],\"ladder\":[[1,30],[4,30],[6,30],[11,30]]}}"
           << ",\"region_ok\":" << (result.region_ok ? "true" : "false")
           << ",\"model\":";
    emit_model(output, model, "lofo");
    output << ",\"counts\":{\"candidates_enumerated\":" << file.candidate_count
           << ",\"slots_eligible\":";
    size_t slots = 0;
    for (const auto& shape : file.slots) slots += shape.size();
    output << slots << ",\"o11_q11_candidate_calls\":" << file.o11_q11_calls;
    if (calibration) {
        output << ",\"sampled_q4_calls\":" << calibration->total_calls
               << ",\"sampled_q4_reused_results\":" << calibration->reused_results
               << ",\"sample_input_exact\":" << calibration->exact_inputs
               << ",\"sample_input_prefix\":" << calibration->prefix_inputs;
    } else {
        output << ",\"sampled_q4_calls\":0,\"sampled_q4_reused_results\":0"
               << ",\"sample_input_exact\":0,\"sample_input_prefix\":0";
    }
    output << ",\"whole_q1_calls\":" << result.whole_q1_calls
           << ",\"whole_q4_calls\":" << result.whole_q4_calls
           << ",\"whole_q6_calls\":" << result.whole_q6_calls
           << ",\"whole_q11_calls\":" << result.whole_q11_calls
           << ",\"logical_finalists\":" << result.logical_finalists
           << ",\"distinct_finalist_carriers\":" << result.distinct_finalists
           << ",\"reused_finalist_results\":" << result.reused_finalist_results
           << ",\"q11_rank_calls\":" << result.q11_rank_calls
           << ",\"o11_final_q11_calls\":" << result.o11_final_q11_calls
           << ",\"raw_q11_calls\":" << result.raw_q11_calls
           << ",\"k_main_q11_calls\":" << result.k_main_q11_calls
           << ",\"k_main_q11_additional_calls\":" << result.k_main_q11_additional_calls
           << ",\"final_q11_calls\":" << result.final_q11_calls << "}"
           << ",\"bytes\":{\"C_ref\":" << result.c_ref
           << ",\"C_P0\":" << result.c_p0
           << ",\"C_P1\":" << result.c_p1
           << ",\"C_K\":" << result.c_k
           << ",\"C_ladder\":" << result.c_ladder
           << ",\"C_raw\":" << result.raw.complete
           << ",\"C_prod\":" << result.c_prod
           << ",\"ladder_label\":\"" << json_escape(result.ladder_label)
           << "\",\"selected_label\":\"" << json_escape(result.selected_label) << "\"}"
           << ",\"regret\":{\"P0_O11_regret\":";
    emit_double(output, result.p0_o11_regret);
    output << ",\"P1_O11_regret\":";
    emit_double(output, result.p1_o11_regret);
    output << ",\"ladder_K_regret\":";
    emit_double(output, result.ladder_k_regret);
    output << "},\"P0_arms\":";
    emit_arm_array(output, result.p0_arms, 0);
    output << ",\"P1_arms\":";
    emit_arm_array(output, result.p1_arms, 1);
    output << ",\"O11_arms\":[";
    for (size_t i = 0; i < result.o11.size(); ++i) {
        if (i) output << ',';
        output << "{\"family\":\"" << kFamilyNames[i]
               << "\",\"complete_bytes\":" << result.o11[i].complete
               << ",\"carrier_sha256\":\"" << result.o11[i].carrier_sha256
               << "\",\"roundtrip\":" << (result.o11[i].roundtrip ? "true" : "false") << '}';
    }
    output << "],\"phases_ms\":{\"structural_parse\":";
    emit_double(output, result.timing.structural_parse_ms);
    output << ",\"o11_labels\":";
    emit_double(output, result.timing.o11_label_ms);
    output << ",\"mdl_features\":";
    emit_double(output, result.timing.mdl_feature_ms);
    output << ",\"ridge_fit\":";
    emit_double(output, result.timing.ridge_fit_ms);
    output << ",\"sample_ordering\":";
    emit_double(output, result.timing.sample_ordering_ms);
    output << ",\"sampled_q4\":";
    emit_double(output, result.timing.sampled_q4_ms);
    output << ",\"carrier_build\":";
    emit_double(output, result.timing.carrier_build_ms);
    output << ",\"whole_q1\":";
    emit_double(output, result.timing.whole_q1_ms);
    output << ",\"whole_q4\":";
    emit_double(output, result.timing.whole_q4_ms);
    output << ",\"whole_q6\":";
    emit_double(output, result.timing.whole_q6_ms);
    output << ",\"whole_q11\":";
    emit_double(output, result.timing.whole_q11_ms);
    output << ",\"raw_q11\":";
    emit_double(output, result.timing.raw_q11_ms);
    output << ",\"total_encode\":";
    emit_double(output, result.timing.total_encode_ms);
    output << "},\"peak_rss_bytes\":" << current_peak_rss_bytes()
           << ",\"rss_scope\":\"process-current-selftest-or-CI-measurement\""
           << ",\"g2_control\":null,\"missing_links\":[\"frozen-G2-row-binding\",\"five-repetition-controller\",\"model-file-I/O\",\"mechanical-ruling\"]"
           << '}';
    std::cout << output.str() << '\n';
}

static void emit_calibration(
    const PreparedFile& file,
    const CalibrationResult& calibration) {
    std::cout << "{\"schema\":\"anvil.g5-planner-pivot.q4-samples\",\"schema_version\":1"
              << ",\"file_id\":\"" << json_escape(file.id)
              << "\",\"file_sha256\":\"" << file.source_sha256
              << "\",\"total_calls\":" << calibration.total_calls
              << ",\"total_reused_results\":" << calibration.reused_results
              << ",\"initial_call_budget\":40,\"total_call_budget\":80"
              << ",\"ucb_multiplier\":1.64,\"shrinkage_prior\":8"
              << ",\"sample_input_cap_bytes\":8192,\"pooled_sigma\":";
    emit_double(std::cout, calibration.sigma);
    std::cout << ",\"samples\":[";
    auto rows = const_cast<PreparedFile&>(file).slots;
    bool first = true;
    for (const auto& shape : rows) {
        for (const auto& slot : shape) {
            for (const auto& row : slot) {
                const auto& sample = calibration.samples[row.flat_index];
                if (!first) std::cout << ',';
                first = false;
                std::cout << "{\"shape_id\":" << row.shape_id
                          << ",\"slot_id\":" << row.slot_id
                          << ",\"leaf_id\":" << static_cast<unsigned>(row.leaf)
                          << ",\"occurrence_count\":" << row.occurrences
                          << ",\"full_object_bytes\":" << sample.full_object_bytes
                          << ",\"sample_input_bytes\":" << sample.sample_input_bytes
                          << ",\"sample_input_kind\":\""
                          << (sample.prefix ? "prefix" : "exact")
                          << "\",\"q4_complete_payload_bytes\":" << sample.q4_bytes
                          << ",\"residual\":";
                emit_double(std::cout, sample.residual);
                std::cout << ",\"priority\":";
                emit_double(std::cout, sample.priority);
                std::cout << ",\"order_key_sha256\":\""
                          << planner_sha256::hex(row.order_key)
                          << "\",\"reused\":" << (sample.reused ? "true" : "false") << '}';
            }
        }
    }
    std::cout << "]}\n";
}

static void require_pilot(bool condition, const char* label) {
    if (!condition) throw std::runtime_error(std::string("pilot selftest failed: ") + label);
}

static void feature_selftest() {
    const Bytes input = bytes("abcdabcdabcd");
    const double h0 = order0_bits(input);
    const double h1 = order1_bits(input);
    const double coverage = bounded_match_coverage(input);
    require_pilot(h0 > 0 && std::isfinite(h0), "H0");
    require_pilot(h1 >= 0 && std::isfinite(h1), "H1");
    require_pilot(coverage >= 8.0 / 12.0 && coverage <= 1.0, "match coverage");
    const std::array<double, kFeatureCount> first{
        1, 2, 3, 4, 5, 6, 7, 8};
    const std::array<double, kFeatureCount> second = first;
    require_pilot(first == second, "feature determinism");
}

static void hash_selftest() {
    const std::string empty_hash = planner_sha256::hex(bytes(""));
    require_pilot(empty_hash ==
                      "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                  ("sha256 empty: " + empty_hash).c_str());
    require_pilot(planner_sha256::hex(bytes("abc")) ==
                      "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
                  "sha256 abc");
}

static void raw_and_malformed_selftest() {
    const PreparedFile raw_only = prepare_file("T0", "raw-only", bytes("not json\nstill raw\n"));
    const PreparedFile support = prepare_file(
        "T1", "support", bytes("{\"a\":1}\n{\"a\":2}\n"));
    std::vector<PreparedFile*> files{const_cast<PreparedFile*>(&raw_only), const_cast<PreparedFile*>(&support)};
    const RidgeModel model = fit_ridge(files, std::nullopt);
    const DiscoveryResult raw_result = evaluate_pilot(raw_only, model);
    require_pilot(raw_result.region_ok, "raw-only region");
    require_pilot(raw_result.c_prod == raw_result.raw.complete, "raw-only fallback");
    require_pilot(raw_result.logical_finalists == 8, "raw-only logical finalists");
    require_pilot(raw_result.q11_rank_calls == 0, "raw-only q11 rank");
    const PreparedFile empty = prepare_file("T2", "empty", bytes(""));
    const DiscoveryResult empty_result = evaluate_pilot(empty, model);
    require_pilot(!empty_result.region_ok, "empty region unavailable");
    require_pilot(empty_result.c_prod == empty_result.raw.complete, "empty raw fallback");
}

static void pilot_selftest() {
    selftest();
    hash_selftest();
    feature_selftest();

    const std::array<std::string, 4> fixtures = {
        "{\"a\":1,\"b\":\"x\",\"n\":null}\n{\"a\":2,\"b\":\"y\",\"n\":null}\n"
        "{\"a\":3,\"b\":\"x\",\"n\":null}\n{\"a\":4,\"b\":\"y\",\"n\":null}\n",
        "{\"i\":100,\"j\":[10,12,15],\"s\":\"alpha\"}\n"
        "{\"i\":101,\"j\":[20,22,25],\"s\":\"alpha\"}\n"
        "{\"i\":102,\"j\":[30,32,35],\"s\":\"beta\"}\n",
        "{\"k\":1,\"v\":-40,\"t\":\"repeat\"}\n{\"k\":2,\"v\":-39,\"t\":\"repeat\"}\n"
        "{\"k\":3,\"v\":-38,\"t\":\"other\"}\n{\"k\":4,\"v\":-37,\"t\":\"other\"}\n",
        "{\"id\":\"a\",\"value\":1.0,\"ok\":true}\n"
        "{\"id\":\"b\",\"value\":2.0,\"ok\":false}\n"
        "{\"id\":\"a\",\"value\":3.0,\"ok\":true}\n"};

    std::vector<PreparedFile> files;
    for (size_t i = 0; i < fixtures.size(); ++i)
        files.push_back(prepare_file("S" + std::to_string(i), "synthetic-" + std::to_string(i),
                                     bytes(fixtures[i])));
    std::vector<PreparedFile*> file_ptrs;
    for (auto& file : files) file_ptrs.push_back(&file);
    std::vector<DiscoveryResult> results;
    for (size_t i = 0; i < files.size(); ++i) {
        auto start = Clock::now();
        const RidgeModel model = fit_ridge(file_ptrs, i);
        const double fit_ms = elapsed_ms(start, Clock::now());
        DiscoveryResult result = evaluate_pilot(files[i], model);
        result.timing.ridge_fit_ms = fit_ms;
        require_pilot(result.region_ok, "synthetic region");
        require_pilot(result.q11_rank_calls == 0, "q11 rank zero");
        require_pilot(result.logical_finalists == 8, "eight finalists");
        require_pilot(result.whole_q1_calls <= 8, "whole q1 cap");
        require_pilot(result.whole_q4_calls <= 4, "whole q4 cap");
        require_pilot(result.whole_q6_calls <= 2, "whole q6 cap");
        require_pilot(result.whole_q11_calls <= 2, "whole q11 cap");
        require_pilot(result.k_main_q11_calls <= 8, "K-main q11 cap");
        require_pilot(result.o11_final_q11_calls == 4, "O11 final q11 count");
        require_pilot(result.raw_q11_calls == 1, "raw q11 count");
        require_pilot(result.final_q11_calls <= 4, "final q11 cap");
        require_pilot(result.calibration && result.calibration->total_calls <= kQ4SampleBudget,
                      "q4 sample cap");
        require_pilot(result.c_prod <= result.raw.complete, "raw fallback no regression");
        for (const auto& arm : result.p0_arms) require_pilot(arm.roundtrip, "P0 roundtrip");
        for (const auto& arm : result.p1_arms) require_pilot(arm.roundtrip, "P1 roundtrip");
        for (const auto& arm : result.o11) require_pilot(arm.roundtrip, "O11 roundtrip");
        results.push_back(std::move(result));
    }

    const RidgeModel first_model = fit_ridge(file_ptrs, 0);
    const RidgeModel second_model = fit_ridge(file_ptrs, 0);
    require_pilot(first_model.model_sha256 == second_model.model_sha256, "ridge determinism");
    const auto first_scores = p0_scores(files[0], first_model);
    CalibrationResult calibration = calibrate_q4(files[0], first_scores);
    require_pilot(calibration.total_calls <= kQ4SampleBudget, "q4 sample cap");
    const LeafSelection before = select_policy(files[0], first_scores, &calibration.correction, Family::Mixed);
    for (auto& shape : files[0].oracle.shapes) {
        for (auto& slot : shape.slots) {
            for (auto& candidate : slot.candidates)
                candidate.isolated_brotli_bytes = candidate.isolated_brotli_bytes == 0
                    ? 999999u : 0u;
        }
    }
    const LeafSelection after = select_policy(files[0], first_scores, &calibration.correction, Family::Mixed);
    require_pilot(before == after, "P1 unaffected by O11 label mutation");

    const DiscoveryResult repeat = evaluate_pilot(files[1], fit_ridge(file_ptrs, 1));
    require_pilot(repeat.c_p0 == results[1].c_p0, "P0 byte determinism");
    require_pilot(repeat.c_p1 == results[1].c_p1, "P1 byte determinism");
    require_pilot(repeat.c_ladder == results[1].c_ladder, "ladder byte determinism");

    raw_and_malformed_selftest();
    std::cout << "PASS grotli_g5_planner_pivot selftest\n";
}

static int measure_discovery(const std::vector<std::string>& paths) {
    if (paths.size() != 4) throw std::runtime_error("measure-discovery requires exactly four inputs");
    std::vector<PreparedFile> files;
    files.reserve(paths.size());
    for (size_t i = 0; i < paths.size(); ++i) {
        const std::string name = paths[i].substr(paths[i].find_last_of("/\\") + 1);
        files.push_back(prepare_file("D" + std::to_string(i + 1), name, read_file(paths[i])));
    }
    std::vector<PreparedFile*> file_ptrs;
    for (auto& file : files) file_ptrs.push_back(&file);
    for (size_t i = 0; i < files.size(); ++i) {
        auto start = Clock::now();
        const RidgeModel model = fit_ridge(file_ptrs, i);
        const double fit_ms = elapsed_ms(start, Clock::now());
        DiscoveryResult result = evaluate_pilot(files[i], model);
        result.timing.ridge_fit_ms = fit_ms;
        const CalibrationResult* calibration = result.calibration ? &*result.calibration : nullptr;
        emit_result(files[i], model, calibration, result);
        if (calibration) emit_calibration(files[i], *calibration);
    }
    return 0;
}

static int fit_models(const std::vector<std::string>& paths) {
    if (paths.size() != 4) throw std::runtime_error("fit-model requires exactly four inputs");
    std::vector<PreparedFile> files;
    for (size_t i = 0; i < paths.size(); ++i) {
        const std::string name = paths[i].substr(paths[i].find_last_of("/\\") + 1);
        files.push_back(prepare_file("D" + std::to_string(i + 1), name, read_file(paths[i])));
    }
    std::vector<PreparedFile*> file_ptrs;
    for (auto& file : files) file_ptrs.push_back(&file);
    for (size_t i = 0; i < files.size(); ++i) {
        const RidgeModel model = fit_ridge(file_ptrs, i);
        std::ostringstream output;
        emit_model(output, model, "lofo");
        std::cout << output.str() << '\n';
    }
    const RidgeModel final_model = fit_ridge(file_ptrs, std::nullopt);
    std::ostringstream output;
    emit_model(output, final_model, "final");
    std::cout << output.str() << '\n';
    return 0;
}

static int schema_check() {
    std::string line;
    size_t rows = 0;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        if (line.find("\"schema\":\"anvil.g5-planner-pivot.") == std::string::npos ||
            line.find("\"schema_version\":1") == std::string::npos ||
            line.back() != '}')
            throw std::runtime_error("invalid minimal JSONL schema envelope");
        ++rows;
    }
    if (rows == 0) throw std::runtime_error("schema-check received no JSONL rows");
    std::cout << "PASS minimal JSONL envelope rows=" << rows << '\n';
    return 0;
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "selftest") {
            pilot_selftest();
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "schema-check") return schema_check();
        if (argc >= 6 && std::string(argv[1]) == "measure-discovery")
            return measure_discovery(std::vector<std::string>(argv + 2, argv + argc));
        if (argc >= 6 && std::string(argv[1]) == "fit-model")
            return fit_models(std::vector<std::string>(argv + 2, argv + argc));
        std::cerr << "usage: grotli_g5_planner_pivot selftest\n"
                  << "       grotli_g5_planner_pivot schema-check\n"
                  << "       grotli_g5_planner_pivot fit-model INPUT1 INPUT2 INPUT3 INPUT4\n"
                  << "       grotli_g5_planner_pivot measure-discovery INPUT1 INPUT2 INPUT3 INPUT4\n";
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
