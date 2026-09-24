// ANVIL I10 GROTLI G4 planner-fidelity standalone research prototype.
//
// Frozen semantics:
//   docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md   freeze revision r5
//     (internal prereg commit 27a0dc8; public prereg commit d78244c)
//   docs/I10-GROTLI-FRONTIER-ARCHITECTURE.md        layers/invariants
//   docs/I10-GROTLI-G3-REGION-PREREG.md             frozen G3 carrier
//   docs/I10-GROTLI-G2-CORPUS-FREEZE.md             D1-D4 identities
//   tools/grotli_g3.cpp                             FROZEN G3 translation unit
//
// Research tooling only. This does not change ANVIL's production wire, and it
// does not touch src/anvil.cpp.
//
// Build (matches the frozen G3 tool build line; warning-clean -Wall -Wextra):
//   clang++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic \
//     tools/grotli_g4_planner.cpp \
//     -lbrotlienc -lbrotlidec -lbrotlicommon -o grotli_g4_planner
//
// Usage:
//   grotli_g4_planner selftest
//   grotli_g4_planner measure  INPUT [--g2-result ROW.json]
//   grotli_g4_planner probe    INPUT [--g2-result ROW.json]   # CI-only (Q2)
//   grotli_g4_planner selftest_synthetic            # local synthetic identity
//
// `measure` emits one JSON row per file carrying every P x F (16 rows) plus the
// O11 oracle rows, the per-surface accounting, and the r5 provenance fields.
// `probe` emits one JSON object per file carrying the bounded outcome-blind
// pairwise separability probe (r5 §10). No D1-D4/V1 work is done locally; heavy
// corpus work is GitHub Actions only.
//
// ===========================================================================
// SEMANTIC-IDENTITY STRATEGY (r5 §2, §14; task A.1)
// ===========================================================================
// This tool does NOT copy G3 code. It includes the frozen G3 translation unit
// with its `main` renamed, so every frozen semantic is compiled from
// tools/grotli_g3.cpp:
//   * leaf IDs / payload grammars (RAW_LEX / EXACT_DICT / INT_FOR /
//     INT_DELTA_FOR / INT_DOD_FOR);
//   * the lexical parser, framing, shape identity, shape dictionary, records;
//   * the regionized carrier grammar (`make_region_carrier`) and the strict
//     decoder (`decode_region_carrier`);
//   * the isolated q11 leaf oracle (`build_structured_candidates`) and the
//     q11/lgwin30 whole-carrier backend.
// The two translation units cannot drift: there is exactly one definition of
// every frozen semantic. The `main` rename is the only edit applied to the
// included TU.
//
// ===========================================================================
// PHYSICAL SEPARATION OF PRODUCTION MATERIALIZATION FROM THE ORACLE (task A.2)
// ===========================================================================
// Frozen G3's `build_structured_candidates` performs one isolated Brotli q11
// evaluation per (slot x eligible leaf) WHILE it constructs candidates. That
// cached q11 score is the RESEARCH ORACLE, and it must NOT be the production
// proxy materialization path.
//
// G4 therefore keeps TWO independent `RegionAnalysis` objects for every file:
//
//   (1) PRODUCTION analysis (`prod_a`):
//        analyze_regions(...)  -- frozen, no scoring
//        build_candidates_no_score(...) -- EXACT frozen G3 payload constructors
//        and EXACT frozen G3 eligibility semantics, but ZERO compression calls
//        of any quality. It never reads `isolated_brotli_bytes` (there is none
//        on its candidate type).
//
//   (2) ORACLE analysis (`orac_a`):
//        analyze_regions(...)  -- frozen, no scoring
//        build_structured_candidates(...) -- the FROZEN G3 builder, which
//        computes the isolated q11 labels. Used ONLY for O11 labels/scoring.
//
// A hard structural-identity assertion runs BEFORE any measurement: the two
// analyses and the score-free production structure must be identical in frame
// layout, shape/slot membership, candidate IDs/order, payload bytes, and every
// eligibility/carrier-relevant metric field. Any difference is a fatal defect.
//
// ===========================================================================
// RANKING SURFACES (r5 §4; task B)
// ===========================================================================
// O11 -- reference: frozen isolated q11 byte score of the complete serialized
//        isolated leaf object; RAW-first scalar tie-break (r5 §6.1).
// S0  -- exact byte length of that complete serialized isolated leaf object.
// S1  -- exact integer-only H0 tuple over the SAME complete serialized object:
//          H0_scaled = sum_b 256*count[b]*(ceil_log2(N) - floor_log2(count[b]))
//        compared lexicographically as (H0_scaled, S0_size); if the FULL tuple
//        ties, RAW_LEX wins, then lower leaf ID, then enumeration order.
//        NO synthetic metadata constant is added: metadata is already inside
//        the object bytes (r5 §4.4). Equal H0 with different S0 is NOT a tie.
// S2  -- isolated Brotli q1 score of the same serialized object (1 q1 call per
//        candidate; zero q11 ranking calls).
//
// Typed comparators are used; S1 is compared as a struct tuple, never flattened
// into one integer that would conflate H0_scaled and S0_size.
//
// ===========================================================================
// NO CROSS-SURFACE FUSION AND NO S3 IN C++ (r5 §4.6, §4.7, §13.18; task G)
// ===========================================================================
// There is NO select_all_s3 / score_s3_fidelity / S3_dictS* / per-column fused
// selector anywhere in this tool. Every (P, F) carrier is produced solely by
// surface P restricted to portfolio F's frozen mask. S3 is reconstructed
// entirely in the workflow from already-measured (P,F) complete bytes and the
// already-measured RAW_BROTLI result using the frozen DSTAR/ISTAR rule; it makes
// zero additional q11 calls and zero ranking calls.

#define main grotli_g3_frozen_main
#include "grotli_g3.cpp"
#undef main

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
// Windows headers expose min/max macros that corrupt std::min/std::max and
// numeric_limits<T>::max(). Keep the research tool's C++ semantics portable.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

// ---------------------------------------------------------------------------
// CPU clock
// ---------------------------------------------------------------------------

static double ms_between(const Clock::time_point& a, const Clock::time_point& b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

// ---------------------------------------------------------------------------
// Instrumentation counters (deterministic; reported separately from timing)
// ---------------------------------------------------------------------------

// Ranking calls are candidate-scoring backend calls used to ORDER candidates.
// The oracle's isolated q11 calls are NOT ranking calls in G4; they are the
// labels (and they are what O11 "paid", reported separately and never gated).
// Final whole-carrier q11 calls are also NOT ranking calls. Ranking q1 is S2's
// own cheap backend bridge. All four are counted separately so the r5
// accounting is explicit (r5 §G8.5, §G8.7).
struct CallCounts {
    uint64_t oracle_q11_leaf_calls = 0;   // isolated oracle scoring (labels/O11)
    uint64_t ranking_q11_calls = 0;       // q11 used to rank candidates (never)
    uint64_t ranking_q1_calls = 0;        // q1 used to rank candidates (S2)
    uint64_t raw_brotli_q11_calls = 0;    // RAW_BROTLI whole-file encodes
    uint64_t pf_verifier_q11_calls = 0;   // paid P×F whole-carrier encodes
    uint64_t probe_q11_calls = 0;         // Q2 pair-probe encodes (separate)
};

static CallCounts g_calls;

// A local parameterized Brotli encoder for the S2 bridge. The frozen G3 TU's
// `brotli_encode` stays untouched at q11/lgwin30 (it IS the oracle). This
// helper uses the same window (lgwin 30) with a caller-chosen quality, so S2 is
// "the same codec family, cheaper quality" (r5 §4.5). Never used for oracle
// scoring.
static constexpr int kS2Quality = 1;      // recorded in every result row
static constexpr int kBackendWindow = 30; // recorded in every result row

static Bytes brotli_encode_quality(int quality, const Bytes& in) {
    const size_t cap = BrotliEncoderMaxCompressedSize(in.size());
    if (!cap && !in.empty()) throw std::runtime_error("brotli size bound overflow");
    Bytes out(std::max<size_t>(cap, 1));
    size_t n = out.size();
    const uint8_t* src = in.empty() ? reinterpret_cast<const uint8_t*>("") : in.data();
    if (!BrotliEncoderCompress(quality, kBackendWindow, BROTLI_MODE_GENERIC,
                               in.size(), src, &n, out.data()))
        throw std::runtime_error("brotli q1/lgwin30 encode failed");
    out.resize(n);
    return out;
}

// ---------------------------------------------------------------------------
// SHA-256 (self-contained; used for serialized-carrier identity, provenance,
// and the outcome-blind column sampling key of r5 §10.2)
// ---------------------------------------------------------------------------

namespace sha256 {

struct Ctx {
    uint32_t h[8];
    uint64_t len = 0;
    uint8_t buf[64];
    size_t buf_len = 0;
};

static const uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u,
    0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u,
    0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
    0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
    0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au,
    0x5b9cca4fu, 0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

static inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

static void init(Ctx& c) {
    c.h[0] = 0x6a09e667u; c.h[1] = 0xbb67ae85u; c.h[2] = 0x3c6ef372u;
    c.h[3] = 0xa54ff53au; c.h[4] = 0x510e527fu; c.h[5] = 0x9b05688cu;
    c.h[6] = 0x1f83d9abu; c.h[7] = 0x5be0cd19u;
}

static void compress(Ctx& c, const uint8_t* p) {
    uint32_t w[64];
    for (int i = 0; i < 16; ++i)
        w[i] = (uint32_t(p[i * 4]) << 24) | (uint32_t(p[i * 4 + 1]) << 16) |
               (uint32_t(p[i * 4 + 2]) << 8) | uint32_t(p[i * 4 + 3]);
    for (int i = 16; i < 64; ++i) {
        const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = c.h[0], b = c.h[1], cc = c.h[2], d = c.h[3];
    uint32_t e = c.h[4], f = c.h[5], g = c.h[6], h = c.h[7];
    for (int i = 0; i < 64; ++i) {
        const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const uint32_t ch = (e & f) ^ (~e & g);
        const uint32_t t1 = h + S1 + ch + K[i] + w[i];
        const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const uint32_t maj = (a & b) ^ (a & cc) ^ (b & cc);
        const uint32_t t2 = S0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = cc; cc = b; b = a; a = t1 + t2;
    }
    c.h[0] += a; c.h[1] += b; c.h[2] += cc; c.h[3] += d;
    c.h[4] += e; c.h[5] += f; c.h[6] += g; c.h[7] += h;
}

static void update(Ctx& c, const uint8_t* p, size_t n) {
    c.len += n;
    while (n) {
        const size_t take = std::min<size_t>(64 - c.buf_len, n);
        std::memcpy(c.buf + c.buf_len, p, take);
        c.buf_len += take;
        p += take;
        n -= take;
        if (c.buf_len == 64) { compress(c, c.buf); c.buf_len = 0; }
    }
}

static std::array<uint8_t, 32> final(Ctx& c) {
    const uint64_t bitlen = c.len * 8ull;
    const uint8_t pad = 0x80;
    update(c, &pad, 1);
    const uint8_t zero = 0;
    while (c.buf_len != 56) update(c, &zero, 1);
    uint8_t lenb[8];
    for (int i = 0; i < 8; ++i) lenb[i] = uint8_t(bitlen >> (56 - i * 8));
    update(c, lenb, 8);
    std::array<uint8_t, 32> out{};
    for (int i = 0; i < 8; ++i) {
        out[i * 4] = uint8_t(c.h[i] >> 24);
        out[i * 4 + 1] = uint8_t(c.h[i] >> 16);
        out[i * 4 + 2] = uint8_t(c.h[i] >> 8);
        out[i * 4 + 3] = uint8_t(c.h[i]);
    }
    return out;
}

static std::array<uint8_t, 32> hash(const Bytes& in) {
    Ctx c; init(c); update(c, in.data(), in.size()); return final(c);
}

static std::string hex(const std::array<uint8_t, 32>& h) {
    static const char* d = "0123456789abcdef";
    std::string s;
    s.reserve(64);
    for (uint8_t b : h) { s += d[b >> 4]; s += d[b & 0xf]; }
    return s;
}

static std::string hex(const Bytes& in) { return hex(hash(in)); }

} // namespace sha256

// ---------------------------------------------------------------------------
// Frozen leaf basis and frozen G3 regional portfolio masks (r5 §4.0, §4.1)
// ---------------------------------------------------------------------------

static const std::vector<LeafId> kMaskRaw = {LeafId::RawLex};
static const std::vector<LeafId> kMaskDict = {LeafId::RawLex, LeafId::ExactDict};
static const std::vector<LeafId> kMaskInt = {
    LeafId::RawLex, LeafId::IntFor, LeafId::IntDeltaFor, LeafId::IntDodFor};
static const std::vector<LeafId> kMaskMixed = {
    LeafId::RawLex, LeafId::ExactDict, LeafId::IntFor,
    LeafId::IntDeltaFor, LeafId::IntDodFor};

enum class Family : uint8_t { Raw = 0, Dict = 1, Int = 2, Mixed = 3 };

static const std::array<const char*, 4> kFamilyName = {"RAW", "DICT", "INT", "MIXED"};

static const std::vector<LeafId>& family_mask(Family f) {
    switch (f) {
        case Family::Raw: return kMaskRaw;
        case Family::Dict: return kMaskDict;
        case Family::Int: return kMaskInt;
        case Family::Mixed: return kMaskMixed;
    }
    throw std::runtime_error("unknown family");
}

// ---------------------------------------------------------------------------
// SCORE-FREE PRODUCTION CANDIDATE MATERIALIZER (r5 §5; task A.2)
// ---------------------------------------------------------------------------
//
// `ProdCandidate` is the score-free candidate: exact frozen leaf ID and exact
// frozen payload bytes, plus the slot-level frozen eligibility facts. It
// deliberately carries NO oracle score and NO proxy score.
//
// `ProdSlot` / `ProdShape` mirror the frozen G3 SlotPlan/ShapePlan structure so
// the identity assertion can compare them against the oracle-path analysis.
// `slots[i].candidates` is in the SAME deterministic emission order as frozen G3
// (`build_structured_candidates`): RAW_LEX, EXACT_DICT, then INT_FOR,
// INT_DELTA_FOR, INT_DOD_FOR. RAW_LEX is first (leaf ID 0), which is what makes
// the frozen RAW-first tie rule fall out of a strict-`<` scan.

struct ProdCandidate {
    LeafId id = LeafId::RawLex;
    Bytes payload;
};

struct ProdSlot {
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
    std::vector<ProdCandidate> candidates;
};

struct ProdShape {
    uint32_t id = 0;
    std::vector<ProdSlot> slots;
};

struct ProdCandidates {
    std::vector<ProdShape> shapes;
    uint64_t candidate_count = 0;
    uint64_t eligible_slots = 0;
};

// Exact frozen G3 eligibility + payload construction, with ZERO Brotli calls.
// This is a faithful re-expression of frozen `build_structured_candidates`
// with the `add(...)` lambda's `isolated_brotli_bytes` q11 call removed. It
// never reads or stores any oracle byte.
//
// The production analysis is populated IN PLACE (`a`), so the caller's
// `RegionAnalysis` carries the same slot metric/eligibility facts that frozen
// G3's builder writes into its own analysis. The score-free candidate list is
// returned separately as `ProdCandidates`. This split keeps the frozen slot
// metric fields present on the production analysis (which the structural
// identity assertion compares) while guaranteeing `isolated_brotli_bytes` is
// never touched.
static ProdCandidates build_candidates_no_score(RegionAnalysis& a) {
    ProdCandidates out;
    out.shapes.resize(a.shapes.size());
    for (size_t sid = 0; sid < a.shapes.size(); ++sid) {
        ShapePlan& sh = a.shapes[sid];
        out.shapes[sid].id = sh.id;
        out.shapes[sid].slots.resize(sh.slots.size());
        for (size_t slot = 0; slot < sh.slots.size(); ++slot) {
            SlotPlan& sp = sh.slots[slot];
            ProdSlot& ps = out.shapes[sid].slots[slot];
            ps.tokens = sp.tokens;

            // Derived candidate/eligibility fields are recomputed from the frozen
            // token stream. Reset them first so this helper is deterministic even
            // if a tiny selftest intentionally invokes it more than once.
            sp.token_bytes = 0;
            sp.distinct_tokens = 0;
            sp.canonical_int = false;
            sp.int_min = sp.int_max = 0;
            sp.delta_eligible = false;
            sp.delta_min = sp.delta_max = 0;
            sp.dod_eligible = false;
            sp.dod_min = sp.dod_max = 0;
            ps = ProdSlot{};
            ps.tokens = sp.tokens;

            std::unordered_map<std::string, uint8_t> distinct;
            distinct.reserve(sp.tokens.size());
            std::vector<int64_t> ints;
            ints.reserve(sp.tokens.size());
            bool all_int = true;
            for (const Bytes& tok : sp.tokens) {
                sp.token_bytes += tok.size();
                distinct.emplace(std::string(reinterpret_cast<const char*>(tok.data()),
                                             tok.size()), 0);
                int64_t v = 0;
                if (parse_canonical_int64(tok, v)) ints.push_back(v);
                else all_int = false;
            }
            sp.distinct_tokens = distinct.size();
            auto add = [&](LeafId id, Bytes payload) {
                ProdCandidate c;
                c.id = id;
                c.payload = std::move(payload);
                ps.candidates.push_back(std::move(c));
                ++out.candidate_count;
            };
            // Frozen G3 candidate emission order: RAW_LEX first (leaf ID 0),
            // then EXACT_DICT, then the int leaves. The frozen eligibility
            // ordering (canonical_int gate, then int range, then FOR/DELTA/DOD)
            // is reproduced exactly so payload bytes cannot drift.
            sp.canonical_int = all_int && ints.size() == sp.tokens.size();
            add(LeafId::RawLex, make_raw_payload(ps.tokens));
            add(LeafId::ExactDict, make_dict_payload(ps.tokens));
            if (sp.canonical_int) {
                const auto mm = std::minmax_element(ints.begin(), ints.end());
                sp.int_min = *mm.first;
                sp.int_max = *mm.second;
                if (auto p = make_int_for_payload(ints)) add(LeafId::IntFor, std::move(*p));
                int64_t dmin = 0, dmax = 0;
                if (auto p = make_int_delta_payload(ints, &dmin, &dmax)) {
                    sp.delta_eligible = true;
                    sp.delta_min = dmin;
                    sp.delta_max = dmax;
                    add(LeafId::IntDeltaFor, std::move(*p));
                }
                int64_t ddmin = 0, ddmax = 0;
                if (auto p = make_int_dod_payload(ints, &ddmin, &ddmax)) {
                    sp.dod_eligible = true;
                    sp.dod_min = ddmin;
                    sp.dod_max = ddmax;
                    add(LeafId::IntDodFor, std::move(*p));
                }
            }

            // Mirror every frozen derived metric into the score-free production
            // view. These values are facts about payload eligibility, not scores.
            ps.token_bytes = sp.token_bytes;
            ps.distinct_tokens = sp.distinct_tokens;
            ps.canonical_int = sp.canonical_int;
            ps.int_min = sp.int_min;
            ps.int_max = sp.int_max;
            ps.delta_eligible = sp.delta_eligible;
            ps.delta_min = sp.delta_min;
            ps.delta_max = sp.delta_max;
            ps.dod_eligible = sp.dod_eligible;
            ps.dod_min = sp.dod_min;
            ps.dod_max = sp.dod_max;

            ++out.eligible_slots;
        }
    }
    return out;
}

// Serialize the exact frozen G3 isolated leaf object for a production candidate.
// ZERO Brotli calls (see frozen `make_leaf_object`).
static Bytes prod_leaf_object(const ProdCandidate& c, size_t occurrences) {
    return make_leaf_object(c.id, occurrences, c.payload);
}

// ---------------------------------------------------------------------------
// ORACLE PATH (r5 §4.2; task A.3)
// ---------------------------------------------------------------------------
//
// The oracle labels are owned by the frozen G3 analysis itself: its
// `SlotPlan::candidates[i].isolated_brotli_bytes`. That structure is produced by
// the FROZEN `build_structured_candidates`, which is the research oracle and the
// O11 reference scoring surface. It is physically distinct from the score-free
// production structure and is consulted ONLY to (a) label/score planner fidelity
// and (b) arbitrate O11. No proxy selection path can reach it.

// Count the isolated q11 leaf evaluations frozen G3 already performed at
// candidate-build time (these are labels, not ranking calls).
static uint64_t count_oracle_leaf_calls(const RegionAnalysis& a) {
    uint64_t n = 0;
    for (const ShapePlan& sh : a.shapes)
        for (const SlotPlan& slot : sh.slots)
            n += slot.candidates.size();
    return n;
}

// PRODUCTION vs ORACLE STRUCTURAL IDENTITY ASSERTION (task A.4).
// Asserts that the score-free production materialization, the score-free
// production analysis (analyze_regions only), and the oracle-path frozen G3
// analysis agree on: shape count/id, slot count, token streams, slot metric
// fields, canonical_int/delta/dod eligibility and their carried ranges, candidate
// count, candidate leaf IDs and order, and payload bytes. Runs before any result
// is accepted; a mismatch is a hard defect and blocks measurement.
static void assert_production_matches_oracle(const RegionAnalysis& prod_a,
                                             const ProdCandidates& pc,
                                             const RegionAnalysis& orac_a) {
    if (pc.shapes.size() != orac_a.shapes.size())
        throw std::runtime_error("production/oracle shape count mismatch");
    if (prod_a.shapes.size() != orac_a.shapes.size())
        throw std::runtime_error("production/oracle analysis shape mismatch");
    if (prod_a.frames.size() != orac_a.frames.size())
        throw std::runtime_error("production/oracle frame count mismatch");
    if (prod_a.records.size() != orac_a.records.size())
        throw std::runtime_error("production/oracle record count mismatch");
    if (prod_a.raw_members != orac_a.raw_members)
        throw std::runtime_error("production/oracle raw membership mismatch");
    for (size_t fi = 0; fi < orac_a.frames.size(); ++fi) {
        if (prod_a.frames[fi].lo != orac_a.frames[fi].lo ||
            prod_a.frames[fi].hi != orac_a.frames[fi].hi)
            throw std::runtime_error("production/oracle frame extent mismatch");
        if (prod_a.records[fi].structured != orac_a.records[fi].structured ||
            prod_a.records[fi].shape_id != orac_a.records[fi].shape_id)
            throw std::runtime_error("production/oracle record mismatch");
    }
    for (size_t sid = 0; sid < orac_a.shapes.size(); ++sid) {
        const ShapePlan& sh = orac_a.shapes[sid];
        const ShapePlan& psh = prod_a.shapes[sid];
        const ProdShape& ps = pc.shapes[sid];
        if (ps.id != sh.id) throw std::runtime_error("production/oracle shape id mismatch");
        if (psh.id != sh.id) throw std::runtime_error("production/oracle analysis shape id");
        if (psh.parts != sh.parts) throw std::runtime_error("production/oracle parts mismatch");
        if (psh.members != sh.members) throw std::runtime_error("production/oracle members");
        if (ps.slots.size() != sh.slots.size())
            throw std::runtime_error("production/oracle slot count mismatch");
        if (psh.slots.size() != sh.slots.size())
            throw std::runtime_error("production/oracle analysis slot count");
        for (size_t slot = 0; slot < sh.slots.size(); ++slot) {
            const SlotPlan& s = sh.slots[slot];
            const SlotPlan& pso = psh.slots[slot];
            const ProdSlot& p = ps.slots[slot];
            if (p.tokens != s.tokens) throw std::runtime_error("production/oracle token mismatch");
            if (pso.tokens != s.tokens) throw std::runtime_error("production/oracle ana token");
            if (p.token_bytes != s.token_bytes || p.distinct_tokens != s.distinct_tokens) {
                throw std::runtime_error(
                    "production/oracle slot metric mismatch sid=" + std::to_string(sid) +
                    " slot=" + std::to_string(slot) +
                    " prod_token_bytes=" + std::to_string(p.token_bytes) +
                    " oracle_token_bytes=" + std::to_string(s.token_bytes) +
                    " prod_distinct=" + std::to_string(p.distinct_tokens) +
                    " oracle_distinct=" + std::to_string(s.distinct_tokens));
            }
            if (pso.token_bytes != s.token_bytes || pso.distinct_tokens != s.distinct_tokens)
                throw std::runtime_error("production/oracle ana slot metric mismatch");
            if (p.canonical_int != s.canonical_int) throw std::runtime_error("eligibility mismatch");
            if (p.canonical_int) {
                if (p.int_min != s.int_min || p.int_max != s.int_max)
                    throw std::runtime_error("production/oracle int range mismatch");
                if (p.delta_eligible != s.delta_eligible) throw std::runtime_error("delta mismatch");
                if (p.dod_eligible != s.dod_eligible) throw std::runtime_error("dod mismatch");
                if (p.delta_eligible && (p.delta_min != s.delta_min || p.delta_max != s.delta_max))
                    throw std::runtime_error("production/oracle delta range mismatch");
                if (p.dod_eligible && (p.dod_min != s.dod_min || p.dod_max != s.dod_max))
                    throw std::runtime_error("production/oracle dod range mismatch");
            }
            if (p.candidates.size() != s.candidates.size())
                throw std::runtime_error("production/oracle candidate count mismatch");
            for (size_t ci = 0; ci < s.candidates.size(); ++ci) {
                if (p.candidates[ci].id != s.candidates[ci].id)
                    throw std::runtime_error("production/oracle candidate leaf mismatch");
                if (p.candidates[ci].payload != s.candidates[ci].payload)
                    throw std::runtime_error("production/oracle candidate payload mismatch");
            }
        }
    }
    // Remaining structural/aggregate identity fields (task 6).
    if (prod_a.raw_frame_count != orac_a.raw_frame_count)
        throw std::runtime_error("production/oracle raw frame count mismatch");
    if (prod_a.structured_frame_count != orac_a.structured_frame_count)
        throw std::runtime_error("production/oracle structured frame count mismatch");
    if (prod_a.raw_source_bytes != orac_a.raw_source_bytes)
        throw std::runtime_error("production/oracle raw source bytes mismatch");
    if (prod_a.structured_source_bytes != orac_a.structured_source_bytes)
        throw std::runtime_error("production/oracle structured source bytes mismatch");
    if (prod_a.lf_frames != orac_a.lf_frames || prod_a.crlf_frames != orac_a.crlf_frames ||
        prod_a.unterminated_frames != orac_a.unterminated_frames)
        throw std::runtime_error("production/oracle terminator diagnostic mismatch");
    for (size_t sid = 0; sid < orac_a.shapes.size(); ++sid) {
        const ShapePlan& sh = orac_a.shapes[sid];
        const ShapePlan& psh = prod_a.shapes[sid];
        const ProdShape& ps = pc.shapes[sid];
        if (psh.source_bytes != sh.source_bytes)
            throw std::runtime_error("production/oracle shape source_bytes mismatch");
        if (ps.slots.size() != sh.slots.size())
            throw std::runtime_error("production/oracle prod slot count mismatch");
    }
}

// ---------------------------------------------------------------------------
// SCORE-FREE CARRIER BASIS (task 2)
// ---------------------------------------------------------------------------
// Frozen `make_region_carrier` reads `SlotPlan::candidates`. To prove that a
// carrier's bytes never depend on an oracle score field, EVERY P x F carrier
// (including O11) and every Q2 pair carrier is serialized from this basis: a
// structural clone of the production analysis whose candidate vectors are
// populated from the score-free `ProdCandidates` with exact id/payload and
// `isolated_brotli_bytes` explicitly zeroed. Payload identity against the frozen
// oracle analysis is asserted BEFORE the basis is handed out, so the bytes are
// provably the same as frozen G3 would emit for the same leaf selection.
static RegionAnalysis build_score_free_carrier_basis(const RegionAnalysis& prod_a,
                                                     const ProdCandidates& pc,
                                                     const RegionAnalysis& orac_a) {
    RegionAnalysis c = prod_a;  // frames/records/shapes/parts/members, no scores
    if (pc.shapes.size() != c.shapes.size())
        throw std::runtime_error("carrier basis shape count mismatch");
    for (size_t sid = 0; sid < c.shapes.size(); ++sid) {
        ShapePlan& sh = c.shapes[sid];
        const ProdShape& ps = pc.shapes[sid];
        const ShapePlan& osh = orac_a.shapes[sid];
        if (sh.slots.size() != ps.slots.size())
            throw std::runtime_error("carrier basis slot count mismatch");
        for (size_t slot = 0; slot < sh.slots.size(); ++slot) {
            SlotPlan& sp = sh.slots[slot];
            const ProdSlot& pl = ps.slots[slot];
            const SlotPlan& osp = osh.slots[slot];
            if (pl.candidates.size() != osp.candidates.size())
                throw std::runtime_error("carrier basis candidate count mismatch");
            sp.candidates.clear();
            sp.candidates.reserve(pl.candidates.size());
            for (size_t ci = 0; ci < pl.candidates.size(); ++ci) {
                if (pl.candidates[ci].id != osp.candidates[ci].id)
                    throw std::runtime_error("carrier basis leaf id mismatch");
                if (pl.candidates[ci].payload != osp.candidates[ci].payload)
                    throw std::runtime_error("carrier basis payload mismatch");
                LeafCandidate lc;
                lc.id = pl.candidates[ci].id;
                lc.payload = pl.candidates[ci].payload;
                lc.isolated_brotli_bytes = 0;  // score-free by construction
                sp.candidates.push_back(std::move(lc));
            }
        }
    }
    return c;
}

// ---------------------------------------------------------------------------
// SCORE-FREE candidate VIEW (the only thing a proxy may see)
// ---------------------------------------------------------------------------

// A "column" is one slot's score-free candidate set under one portfolio mask,
// in the frozen G3 emission order. `index[i]` is the resolved index of
// `ordered[i]` in the owning slot's full candidate vector, so the oracle label
// lines up without any pointer arithmetic guesswork.
struct ProdColumn {
    size_t occurrences = 0;
    std::vector<const ProdCandidate*> ordered;
    std::vector<size_t> index;
};

static bool mask_allows(LeafId id, const std::vector<LeafId>& mask) {
    return std::find(mask.begin(), mask.end(), id) != mask.end();
}

static ProdColumn column_view(const ProdSlot& slot, const std::vector<LeafId>& mask) {
    ProdColumn v;
    v.occurrences = slot.tokens.size();
    for (size_t ci = 0; ci < slot.candidates.size(); ++ci) {
        const ProdCandidate& c = slot.candidates[ci];
        if (mask_allows(c.id, mask)) {
            v.ordered.push_back(&c);
            v.index.push_back(ci);
        }
    }
    return v;
}

// ---------------------------------------------------------------------------
// Frozen S1 integer H0 formula (r5 §4.4; task B)
// ---------------------------------------------------------------------------

// floor_log2(x) = position of the highest set bit of x (x >= 1).
static uint32_t floor_log2_u64(uint64_t x) {
    if (x == 0) throw std::runtime_error("floor_log2(0)");
    return 63u - static_cast<uint32_t>(__builtin_clzll(x));
}

// ceil_log2(x) = 0 if x == 1 else floor_log2(x - 1) + 1.
static uint32_t ceil_log2_u64(uint64_t x) {
    if (x == 0) throw std::runtime_error("ceil_log2(0)");
    if (x == 1) return 0;
    return floor_log2_u64(x - 1) + 1;
}

// H0_scaled = sum_b 256*count[b]*( ceil_log2(N) - floor_log2(count[b]) )
// over the COMPLETE serialized isolated leaf object. Exact integers only.
// No division, no floating point, no synthetic metadata constant.
static uint64_t h0_scaled(const Bytes& object) {
    const uint64_t n = object.size();
    if (n == 0) return 0;
    std::array<uint64_t, 256> hist{};
    for (uint8_t b : object) hist[b] += 1;
    const uint32_t ceil_n = ceil_log2_u64(n);
    // The r5 formula is accumulated in 128-bit arithmetic and range-checked
    // before narrowing, so a bounded input can never silently wrap uint64.
    unsigned __int128 total = 0;
    for (uint64_t count : hist) {
        if (count == 0) continue;
        const uint32_t fl = floor_log2_u64(count);
        // term non-negative because count <= n.
        total += static_cast<unsigned __int128>(256) * count * uint64_t(ceil_n - fl);
        if (total > static_cast<unsigned __int128>(std::numeric_limits<uint64_t>::max()))
            throw std::runtime_error("S1 H0_scaled overflowed uint64");
    }
    return static_cast<uint64_t>(total);
}

// ---------------------------------------------------------------------------
// Typed comparators (task B): S1 is a tuple, never flattened
// ---------------------------------------------------------------------------

struct S1Key {
    uint64_t h0_scaled = 0;
    uint64_t s0_size = 0;
};

// Strict weak ordering on the frozen S1 tuple.
static bool s1_less(const S1Key& a, const S1Key& b) {
    if (a.h0_scaled != b.h0_scaled) return a.h0_scaled < b.h0_scaled;
    return a.s0_size < b.s0_size;
}
static bool s1_equal(const S1Key& a, const S1Key& b) {
    return a.h0_scaled == b.h0_scaled && a.s0_size == b.s0_size;
}

// ---------------------------------------------------------------------------
// Ranking surfaces
// ---------------------------------------------------------------------------

enum class Surface : uint8_t { O11 = 0, S0 = 1, S1 = 2, S2 = 3 };
static const std::array<const char*, 4> kSurfaceName = {"O11", "S0", "S1", "S2"};

// Score-free proxy scores. These read ONLY the production candidate; they can
// never observe `isolated_brotli_bytes`.
static uint64_t score_s0(const ProdCandidate& c, size_t occurrences) {
    return static_cast<uint64_t>(prod_leaf_object(c, occurrences).size());
}

static S1Key score_s1(const ProdCandidate& c, size_t occurrences) {
    const Bytes object = prod_leaf_object(c, occurrences);
    S1Key k;
    k.h0_scaled = h0_scaled(object);
    k.s0_size = static_cast<uint64_t>(object.size());
    return k;
}

static uint64_t score_s2(const ProdCandidate& c, size_t occurrences) {
    const Bytes object = prod_leaf_object(c, occurrences);
    ++g_calls.ranking_q1_calls;
    return static_cast<uint64_t>(brotli_encode_quality(kS2Quality, object).size());
}

// Select a winner for a column under a surface, restricted to the mask already
// applied by `column_view`. Tie behavior, per r5 §6.1:
//   * O11 / S0 / S2 (scalar): RAW_LEX first, then lower leaf ID, then
//     enumeration order. Falls out of incumbent = first ordered candidate
//     (RAW_LEX) with strict-`<` replacement.
//   * S1 (tuple): lexicographic on (H0_scaled, S0_size); RAW_LEX wins ONLY if
//     the FULL tuple ties, then lower leaf ID, then enumeration order. Also
//     falls out of strict-tuple-`<` replacement from the RAW incumbent.
//
// TIMING DECOMPOSITION (r5 §G8.6): score generation and winner selection are
// separately timed. `score_column_*` produce the per-candidate keys (the gated
// ranking_score_ms); `pick_*` scan those keys to fix the winner (selection_ms).

// O11 key generation reads ONLY the frozen G3 oracle labels.
static void score_column_o11(const ProdColumn& col, const RegionAnalysis& oracle,
                            size_t sid, size_t slot, std::vector<size_t>& out_keys) {
    const auto& cands = oracle.shapes[sid].slots[slot].candidates;
    out_keys.clear();
    out_keys.reserve(col.ordered.size());
    for (size_t i = 0; i < col.ordered.size(); ++i)
        out_keys.push_back(cands[col.index[i]].isolated_brotli_bytes);
}

static void score_column_s0(const ProdColumn& col, std::vector<uint64_t>& out_keys) {
    out_keys.clear();
    out_keys.reserve(col.ordered.size());
    for (const ProdCandidate* c : col.ordered)
        out_keys.push_back(score_s0(*c, col.occurrences));
}

static void score_column_s1(const ProdColumn& col, std::vector<S1Key>& out_keys) {
    out_keys.clear();
    out_keys.reserve(col.ordered.size());
    for (const ProdCandidate* c : col.ordered)
        out_keys.push_back(score_s1(*c, col.occurrences));
}

static void score_column_s2(const ProdColumn& col, std::vector<uint64_t>& out_keys) {
    out_keys.clear();
    out_keys.reserve(col.ordered.size());
    for (const ProdCandidate* c : col.ordered)
        out_keys.push_back(score_s2(*c, col.occurrences));
}

// Winner scan over precomputed scalar keys (RAW-first inherited from index 0).
static size_t pick_scalar(const std::vector<uint64_t>& keys) {
    if (keys.empty()) throw std::runtime_error("empty scalar key column");
    size_t best = 0;
    for (size_t i = 1; i < keys.size(); ++i)
        if (keys[i] < keys[best]) best = i;
    return best;
}

// Winner scan over precomputed S1 tuples (full-tuple lexicographic).
static size_t pick_s1(const std::vector<S1Key>& keys) {
    if (keys.empty()) throw std::runtime_error("empty S1 key column");
    size_t best = 0;
    for (size_t i = 1; i < keys.size(); ++i)
        if (s1_less(keys[i], keys[best])) best = i;
    return best;
}

// Legacy single-call helpers retained for the selftests that assert the frozen
// tie semantics directly. They compose the exact same generation + scan.
static size_t select_index_o11(const ProdColumn& col, const RegionAnalysis& oracle,
                               size_t sid, size_t slot, uint64_t* out_score) {
    std::vector<size_t> keys;
    score_column_o11(col, oracle, sid, slot, keys);
    const size_t i = pick_scalar(keys);
    if (out_score) *out_score = keys[i];
    return i;
}

static size_t select_index_s0(const ProdColumn& col, uint64_t* out_score) {
    std::vector<uint64_t> keys;
    score_column_s0(col, keys);
    const size_t i = pick_scalar(keys);
    if (out_score) *out_score = keys[i];
    return i;
}

static size_t select_index_s1(const ProdColumn& col, S1Key* out_key) {
    std::vector<S1Key> keys;
    score_column_s1(col, keys);
    const size_t i = pick_s1(keys);
    if (out_key) *out_key = keys[i];
    return i;
}

static size_t select_index_s2(const ProdColumn& col, uint64_t* out_score) {
    std::vector<uint64_t> keys;
    score_column_s2(col, keys);
    const size_t i = pick_scalar(keys);
    if (out_score) *out_score = keys[i];
    return i;
}

// ---------------------------------------------------------------------------
// Selection type and deterministic selectors
// ---------------------------------------------------------------------------

// LeafSelection[shape][slot] = chosen leaf id (matches frozen G3 `Selection`).
using LeafSelection = std::vector<std::vector<LeafId>>;

// Select every slot under one surface restricted to one frozen mask.
//
// O11 reads the oracle labels (reference). S0/S1/S2 read only score-free
// production candidates.
static LeafSelection select_o11(const ProdCandidates& pc, const RegionAnalysis& oracle,
                                const std::vector<LeafId>& mask) {
    LeafSelection out(pc.shapes.size());
    for (size_t sid = 0; sid < pc.shapes.size(); ++sid) {
        const ProdShape& sh = pc.shapes[sid];
        out[sid].resize(sh.slots.size());
        for (size_t slot = 0; slot < sh.slots.size(); ++slot) {
            const ProdColumn col = column_view(sh.slots[slot], mask);
            if (col.ordered.empty()) throw std::runtime_error("empty O11 column");
            const size_t i = select_index_o11(col, oracle, sid, slot, nullptr);
            out[sid][slot] = col.ordered[i]->id;
        }
    }
    return out;
}

static LeafSelection select_proxy(const ProdCandidates& pc, Surface s,
                                  const std::vector<LeafId>& mask) {
    LeafSelection out(pc.shapes.size());
    for (size_t sid = 0; sid < pc.shapes.size(); ++sid) {
        const ProdShape& sh = pc.shapes[sid];
        out[sid].resize(sh.slots.size());
        for (size_t slot = 0; slot < sh.slots.size(); ++slot) {
            const ProdColumn col = column_view(sh.slots[slot], mask);
            if (col.ordered.empty()) throw std::runtime_error("empty proxy column");
            size_t i = 0;
            switch (s) {
                case Surface::S0: i = select_index_s0(col, nullptr); break;
                case Surface::S1: i = select_index_s1(col, nullptr); break;
                case Surface::S2: i = select_index_s2(col, nullptr); break;
                case Surface::O11: throw std::runtime_error("O11 is not a proxy");
            }
            out[sid][slot] = col.ordered[i]->id;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// SCORE-ONCE-PER-CANDIDATE RANKING TABLES (r5 5, G8.5, G8.6; tasks 3+4)
// ---------------------------------------------------------------------------
// A ranking surface's per-candidate score is computed ONCE per candidate and
// reused across all four family masks. This makes the deterministic S2 q1 count
// equal to `candidates_enumerated` (not 4x), and lets ranking_score_ms(P) be
// timed separately from selection_ms(P).
//
// key[si][sid][slot][ci] is candidate ci's frozen score on surface si. O11
// stores the oracle isolated q11 byte count; S0 the serialized object length;
// S2 the q1 byte count; S1 stores h0_scaled in `key` and its S0 size in `key2`
// so the tuple stays intact.
struct RankingTables {
    std::vector<std::vector<std::vector<std::vector<uint64_t>>>> key;
    std::vector<std::vector<std::vector<std::vector<uint64_t>>>> key2;
};

static RankingTables build_ranking_tables(const ProdCandidates& pc,
                                          const RegionAnalysis& oracle,
                                          std::array<double, 4>& out_ranking_ms) {
    RankingTables t;
    t.key.resize(4);
    t.key2.resize(4);
    for (int si = 0; si < 4; ++si) {
        const auto t0 = Clock::now();
        t.key[si].resize(pc.shapes.size());
        t.key2[si].resize(pc.shapes.size());
        for (size_t sid = 0; sid < pc.shapes.size(); ++sid) {
            const ProdShape& sh = pc.shapes[sid];
            t.key[si][sid].resize(sh.slots.size());
            t.key2[si][sid].resize(sh.slots.size());
            for (size_t slot = 0; slot < sh.slots.size(); ++slot) {
                const ProdSlot& sl = sh.slots[slot];
                t.key[si][sid][slot].resize(sl.candidates.size());
                t.key2[si][sid][slot].resize(sl.candidates.size());
                for (size_t ci = 0; ci < sl.candidates.size(); ++ci) {
                    const ProdCandidate& c = sl.candidates[ci];
                    if (si == 0) {  // O11: frozen isolated q11 oracle label.
                        t.key[si][sid][slot][ci] =
                            oracle.shapes[sid].slots[slot].candidates[ci].isolated_brotli_bytes;
                    } else if (si == 1) {  // S0
                        t.key[si][sid][slot][ci] = score_s0(c, sl.tokens.size());
                    } else if (si == 2) {  // S1 tuple: (h0, s0)
                        const S1Key k = score_s1(c, sl.tokens.size());
                        t.key[si][sid][slot][ci] = k.h0_scaled;
                        t.key2[si][sid][slot][ci] = k.s0_size;
                    } else {  // S2: exactly one q1 call per candidate, ONCE.
                        t.key[si][sid][slot][ci] = score_s2(c, sl.tokens.size());
                    }
                }
            }
        }
        out_ranking_ms[si] = ms_between(t0, Clock::now());
    }
    return t;
}

// Select winners for one family mask from precomputed keys. RAW_LEX is the
// frozen first-enumerated candidate, so a strict-less scan keeps it on an exact
// tie (r5 6.1) for every surface including the S1 full tuple.
static LeafSelection select_from_table(const ProdCandidates& pc, Surface s,
                                       const std::vector<LeafId>& mask,
                                       const RankingTables& t, size_t si) {
    LeafSelection out(pc.shapes.size());
    for (size_t sid = 0; sid < pc.shapes.size(); ++sid) {
        const ProdShape& sh = pc.shapes[sid];
        out[sid].resize(sh.slots.size());
        for (size_t slot = 0; slot < sh.slots.size(); ++slot) {
            const ProdColumn col = column_view(sh.slots[slot], mask);
            if (col.ordered.empty()) throw std::runtime_error("empty table column");
            const auto& keys = t.key[si][sid][slot];
            const auto& keys2 = t.key2[si][sid][slot];
            size_t best = 0;
            for (size_t i = 1; i < col.ordered.size(); ++i) {
                const size_t ci = col.index[i];
                const size_t cb = col.index[best];
                bool better = false;
                if (s == Surface::S1) {
                    if (keys[ci] != keys[cb]) better = keys[ci] < keys[cb];
                    else better = keys2[ci] < keys2[cb];
                } else {
                    better = keys[ci] < keys[cb];
                }
                if (better) best = i;
            }
            out[sid][slot] = col.ordered[best]->id;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Complete-carrier evaluation (exact frozen G3 carrier + frozen q11 backend)
// ---------------------------------------------------------------------------

struct CarrierEval {
    Bytes carrier;
    Bytes brotli;
    CarrierStats stats;
    std::string carrier_sha256;
    size_t complete = 0;
    bool roundtrip = false;
    double build_ms = 0;
    double final_encode_ms = 0;
    double decode_ms = 0;
};

// Builds the EXACT frozen G3 carrier for a selection and runs the frozen
// q11/lgwin30 backend EXACTLY ONCE over it. Counts one paid final q11 carrier
// call (r5 §G8.5). Requires exact carrier roundtrip.
static CarrierEval evaluate_carrier(const Bytes& src, const RegionAnalysis& a,
                                    const LeafSelection& sel) {
    CarrierEval e;
    Selection frozen_sel;
    frozen_sel.structured_shapes = sel;

    auto t0 = Clock::now();
    e.carrier = make_region_carrier(src, a, frozen_sel, e.stats);
    e.build_ms = ms_between(t0, Clock::now());
    e.carrier_sha256 = sha256::hex(e.carrier);

    t0 = Clock::now();
    e.brotli = brotli_encode(e.carrier); // frozen q11/lgwin30
    ++g_calls.pf_verifier_q11_calls;
    e.final_encode_ms = ms_between(t0, Clock::now());
    e.complete = complete_bytes(src.size(), e.brotli);

    const uint64_t bound64 = std::min<uint64_t>(
        std::numeric_limits<size_t>::max(),
        std::min<uint64_t>(kMaxDecoded, uint64_t(src.size()) * 4ull + kMaxCarrierSlack));

    t0 = Clock::now();
    const Bytes carrier_dec = brotli_decode_bounded(e.brotli, static_cast<size_t>(bound64));
    const Bytes decoded = decode_region_carrier(carrier_dec);
    e.decode_ms = ms_between(t0, Clock::now());
    e.roundtrip = (decoded == src);
    if (!e.roundtrip) throw std::runtime_error("P/F carrier roundtrip mismatch");
    return e;
}

// Build the complete carrier bytes for a selection WITHOUT compressing. Used by
// the separability probe to assemble pair carriers at the encoder side before a
// single q11 call.
static Bytes build_carrier_only(const Bytes& src, const RegionAnalysis& a,
                                const LeafSelection& sel) {
    CarrierStats st;
    Selection frozen_sel;
    frozen_sel.structured_shapes = sel;
    return make_region_carrier(src, a, frozen_sel, st);
}

// ---------------------------------------------------------------------------
// Agreement accounting (r5 §G8.4)
// ---------------------------------------------------------------------------

struct Agreement {
    uint64_t eligible_slots = 0;
    uint64_t agree = 0;
    uint64_t disagree = 0;
    uint64_t weight_total = 0;
    uint64_t weight_agree = 0;
    // near-tie columns: MIXED slots whose O11 top-2 isolated q11 candidates
    // are BOTH five-leaf candidates and differ by <= 1 byte.
    uint64_t near_tie_slots = 0;
    uint64_t non_near_tie_slots = 0;
    uint64_t non_near_tie_agree = 0;
    uint64_t non_near_tie_weight_total = 0;
    uint64_t non_near_tie_weight_agree = 0;
};

// w_i = exact total source-byte length of all lexical tokens in slot i.
static uint64_t slot_weight(const ProdSlot& slot) {
    uint64_t w = 0;
    for (const Bytes& tok : slot.tokens) w += tok.size();
    return w;
}

// A near-tie column is one whose O11 top-2 ELIGIBLE candidates differ by <= 1
// isolated q11 byte. The eligible candidate set is exactly this column's
// mask-restricted candidates (r5 §G8.4). There is no other qualification: in
// particular, requiring all five leaves to be eligible is NOT part of r5 and is
// removed.

// O11 MIXED top-2 isolated q11 byte gap for a column. Returns false if the
// column has fewer than two eligible leaves (not a near-tie candidate).
static bool o11_top2_gap(const ProdColumn& col, const RegionAnalysis& oracle,
                         size_t sid, size_t slot, size_t* out_gap) {
    const auto& cands = oracle.shapes[sid].slots[slot].candidates;
    std::vector<size_t> vals;
    vals.reserve(col.ordered.size());
    for (size_t i = 0; i < col.ordered.size(); ++i)
        vals.push_back(cands[col.index[i]].isolated_brotli_bytes);
    if (vals.size() < 2) return false;
    std::sort(vals.begin(), vals.end());
    *out_gap = vals[1] - vals[0];
    return true;
}

// Agreement of a proxy selection versus an O11 selection on one mask.
static Agreement agreement_metrics(const ProdCandidates& pc, const RegionAnalysis& oracle,
                                   const std::vector<LeafId>& mask,
                                   const LeafSelection& o11_sel,
                                   const LeafSelection& proxy_sel) {
    Agreement ag;
    for (size_t sid = 0; sid < pc.shapes.size(); ++sid) {
        const ProdShape& sh = pc.shapes[sid];
        for (size_t slot = 0; slot < sh.slots.size(); ++slot) {
            const ProdColumn col = column_view(sh.slots[slot], mask);
            if (col.ordered.empty()) continue;
            ++ag.eligible_slots;
            const uint64_t w = slot_weight(sh.slots[slot]);
            ag.weight_total += w;
            const bool same = (o11_sel[sid][slot] == proxy_sel[sid][slot]);
            if (same) { ++ag.agree; ag.weight_agree += w; }
            else ++ag.disagree;

            size_t gap = 0;
            // r5 §G8.4: a MIXED near-tie column is one whose O11 top-2 eligible
            // candidates differ by <= 1 isolated q11 byte. Eligibility is the
            // column's own mask-restricted candidate set; no five-leaf
            // requirement exists in r5.
            const bool is_near_tie = o11_top2_gap(col, oracle, sid, slot, &gap) && gap <= 1;
            if (is_near_tie) {
                ++ag.near_tie_slots;
            } else {
                ++ag.non_near_tie_slots;
                ag.non_near_tie_weight_total += w;
                if (same) {
                    ++ag.non_near_tie_agree;
                    ag.non_near_tie_weight_agree += w;
                }
            }
        }
    }
    return ag;
}

// Family attribution: selected leaf counts by family for a selection.
struct FamilyCounts {
    uint64_t raw = 0, dict = 0, ints = 0;
};

static FamilyCounts count_family_selection(const LeafSelection& sel) {
    FamilyCounts fc;
    for (const auto& slots : sel) {
        for (LeafId id : slots) {
            switch (id) {
                case LeafId::RawLex: ++fc.raw; break;
                case LeafId::ExactDict: ++fc.dict; break;
                case LeafId::IntFor:
                case LeafId::IntDeltaFor:
                case LeafId::IntDodFor: ++fc.ints; break;
            }
        }
    }
    return fc;
}

// Disagreement counts by family between a proxy and O11 on one mask.
struct FamilyDisagreement {
    uint64_t raw = 0, dict = 0, ints = 0;
};

static FamilyDisagreement count_family_disagreement(const ProdCandidates& pc,
                                                    const std::vector<LeafId>& mask,
                                                    const LeafSelection& o11_sel,
                                                    const LeafSelection& proxy_sel) {
    FamilyDisagreement fd;
    for (size_t sid = 0; sid < pc.shapes.size(); ++sid) {
        const ProdShape& sh = pc.shapes[sid];
        for (size_t slot = 0; slot < sh.slots.size(); ++slot) {
            if (column_view(sh.slots[slot], mask).ordered.empty()) continue;
            if (o11_sel[sid][slot] == proxy_sel[sid][slot]) continue;
            switch (o11_sel[sid][slot]) {
                case LeafId::RawLex: ++fd.raw; break;
                case LeafId::ExactDict: ++fd.dict; break;
                case LeafId::IntFor:
                case LeafId::IntDeltaFor:
                case LeafId::IntDodFor: ++fd.ints; break;
            }
        }
    }
    return fd;
}

// ---------------------------------------------------------------------------
// G2 control binding (identical validation discipline to frozen G3)
// ---------------------------------------------------------------------------

// The G2 control binding uses the FROZEN G3 `G2Control` struct and the frozen G3
// `parse_g2_control(...)` validator (fail-closed; no silent fallback), which are
// defined in the included frozen translation unit. G4 adds no parallel control
// parser and fabricates no G2 number.
// (struct G2Control is provided by the frozen G3 TU.)
//
//
//
//
//
//
//

// ---------------------------------------------------------------------------
// Per-file measurement (r5 §5, §11; tasks A, B, C, D, E)
// ---------------------------------------------------------------------------

// Per-(surface,family) row facts.
//
// CALL ATTRIBUTION (r5 §G8.5/§G8.7): q11 ranking calls are a SURFACE property.
// O11's reference scoring is `candidates_enumerated` isolated q11 labels; S0/S1
// make ZERO backend ranking calls; S2 makes at most one q1 per eligible
// candidate and zero q11. These are reported at the surface row (authoritative)
// and mirrored here so a reader cannot misread a PF row as its own ranking
// budget. `final_q11_carrier_calls` is exactly 1 per PF carrier (the paid
// verifier), and is never summed into ranking.
struct PFRow {
    std::string label;      // e.g. "S0_REGION_MIXED"
    Surface surface = Surface::S0;
    Family family = Family::Mixed;
    std::string surface_name;
    std::string family_name;
    CarrierEval carrier;
    FamilyCounts selected;
    uint64_t surface_q11_rank_calls = 0;  // surface-level authoritative value
    uint64_t surface_q1_rank_calls = 0;   // surface-level authoritative value
};

// Per-surface per-file accounting.
struct SurfaceRow {
    Surface surface = Surface::S0;
    std::string surface_name;
    std::string selected_family;    // nominal family of C_region (RAW,DICT,INT,MIXED)
    size_t c_region = std::numeric_limits<size_t>::max();
    uint64_t q11_rank_calls = 0;
    uint64_t q1_rank_calls = 0;
    uint64_t final_q11_carrier_calls = 4;
    // r5 §G8.6 timing decomposition. `ranking_score_ms` times ONLY P's frozen
    // score generation; `selection_ms` times the winner scan + leaf
    // materialization; the G8.6 gated quantity is their sum.
    double ranking_score_ms = 0;
    double selection_ms = 0;
    // Shared production terms carried onto each surface row so candidate_planner_ms
    // can be emitted without the workflow reconstructing them.
    double structural_parse_ms = 0;
    double candidate_materialization_ms = 0;
    Agreement mixed;
    Agreement dict;  // diagnostic
    Agreement ints;  // diagnostic
    FamilyDisagreement mixed_disagreement;
    FamilyDisagreement dict_disagreement;
    FamilyDisagreement ints_disagreement;
};

// Forward declarations of helpers used by the emitters below.
static std::string json_str(const std::string& s) { return json_escape(s); }
static uint64_t peak_rss_bytes();
//

static void emit_pf_row(const PFRow& r, size_t c_region_o11) {
    const std::string p = r.label;
    std::cout << ",\"" << p << "\":{";
    std::cout << "\"surface\":\"" << r.surface_name << "\"";
    std::cout << ",\"family\":\"" << r.family_name << "\"";
    std::cout << ",\"carrier_bytes\":" << r.carrier.carrier.size();
    std::cout << ",\"carrier_sha256\":\"" << r.carrier.carrier_sha256 << "\"";
    std::cout << ",\"q11_compressed_bytes\":" << r.carrier.brotli.size();
    std::cout << ",\"complete_bytes\":" << r.carrier.complete;
    std::cout << ",\"roundtrip\":" << (r.carrier.roundtrip ? "true" : "false");
    std::cout << ",\"build_ms\":" << std::fixed << std::setprecision(6) << r.carrier.build_ms;
    std::cout << ",\"final_encode_ms\":" << r.carrier.final_encode_ms;
    std::cout << ",\"decode_ms\":" << r.carrier.decode_ms;
    std::cout << ",\"carrier_quality\":11";
    std::cout << ",\"carrier_window\":" << kBackendWindow;
    std::cout << ",\"selected_raw_lex\":" << r.selected.raw;
    std::cout << ",\"selected_exact_dict\":" << r.selected.dict;
    std::cout << ",\"selected_int\":" << r.selected.ints;
    // Honest attribution: surface-level ranking call counts (never a false 0 for
    // O11/S2), plus the one paid verifier call THIS carrier costs. Ranking and
    // verifier counts are never summed.
    std::cout << ",\"surface_q11_rank_calls\":" << r.surface_q11_rank_calls;
    std::cout << ",\"surface_q1_rank_calls\":" << r.surface_q1_rank_calls;
    std::cout << ",\"final_q11_carrier_calls_this_carrier\":1";
    std::cout << ",\"is_ranking_call\":false";
    std::cout << ",\"per_file_regret_pct\":"
              << delta_pct(r.carrier.complete, c_region_o11);
    std::cout << "}";
}

static void emit_surface_row(const SurfaceRow& s, size_t c_region_o11) {
    std::cout << ",\"" << s.surface_name << "_surface\":{";
    std::cout << "\"selected_family\":\"" << s.selected_family << "\"";
    std::cout << ",\"c_region_bytes\":" << s.c_region;
    std::cout << ",\"per_file_regret_pct\":" << delta_pct(s.c_region, c_region_o11);
    std::cout << ",\"q11_rank_calls\":" << s.q11_rank_calls;
    std::cout << ",\"q1_rank_calls\":" << s.q1_rank_calls;
    std::cout << ",\"final_q11_carrier_calls\":" << s.final_q11_carrier_calls;
    std::cout << ",\"ranking_score_ms\":" << std::fixed << std::setprecision(6)
              << s.ranking_score_ms;
    std::cout << ",\"selection_ms\":" << s.selection_ms;
    // The G8.6 gated quantity, emitted explicitly so the workflow never has to
    // guess whether the two terms are additive. It EXCLUDES the four paid final
    // q11 verifier calls (r5 §G8.5/§G8.6).
    std::cout << ",\"ranking_score_ms_with_selection\":"
              << (s.ranking_score_ms + s.selection_ms);
    // r5 G8.6 end-to-end candidate-planner time, emitted explicitly per surface.
    // It is the shared production terms plus P's own ranking + selection cost.
    // It EXCLUDES oracle reference parsing and the four paid final q11 verifier
    // calls, both of which are reported separately.
    std::cout << ",\"candidate_planner_ms\":"
              << (s.structural_parse_ms + s.candidate_materialization_ms +
                  s.ranking_score_ms + s.selection_ms);
    std::cout << ",\"shared_structural_parse_ms\":" << s.structural_parse_ms;
    std::cout << ",\"shared_candidate_materialization_ms\":"
              << s.candidate_materialization_ms;
    std::cout << ",\"excludes_oracle_reference_parse\":true";
    std::cout << ",\"excludes_final_q11_verifier_calls\":true";
    auto emit_ag = [&](const char* name, const Agreement& a, bool gated) {
        std::cout << ",\"" << name << "_eligible_slots\":" << a.eligible_slots;
        std::cout << ",\"" << name << "_agree\":" << a.agree;
        std::cout << ",\"" << name << "_disagree\":" << a.disagree;
        std::cout << ",\"" << name << "_weight_total\":" << a.weight_total;
        // r5 §G8.4: sum_i(w_i) == 0 is a MEASUREMENT ERROR, never a 1.0
        // fallback. For the gated MIXED surface a zero denominator makes the
        // row's weighted agreement unavailable (null) and is flagged so the
        // workflow fails closed. Diagnostic DICT/INT surfaces also report null
        // rather than a silent 100%.
        if (a.weight_total) {
            std::cout << ",\"" << name << "_weighted_agreement\":"
                      << double(a.weight_agree) / double(a.weight_total);
            std::cout << ",\"" << name << "_weighted_agreement_available\":true";
        } else {
            std::cout << ",\"" << name << "_weighted_agreement\":null";
            std::cout << ",\"" << name << "_weighted_agreement_available\":false";
            std::cout << ",\"" << name << "_zero_weight_measurement_error\":true";
        }
        if (a.eligible_slots)
            std::cout << ",\"" << name << "_raw_agreement\":"
                      << double(a.agree) / double(a.eligible_slots);
        else
            std::cout << ",\"" << name << "_raw_agreement\":null";
        std::cout << ",\"" << name << "_near_tie_slots\":" << a.near_tie_slots;
        std::cout << ",\"" << name << "_non_near_tie_slots\":" << a.non_near_tie_slots;
        if (a.non_near_tie_weight_total)
            std::cout << ",\"" << name << "_non_near_tie_weighted_agreement\":"
                      << double(a.non_near_tie_weight_agree) / double(a.non_near_tie_weight_total);
        else
            std::cout << ",\"" << name << "_non_near_tie_weighted_agreement\":null";
        if (a.non_near_tie_slots)
            std::cout << ",\"" << name << "_non_near_tie_raw_agreement\":"
                      << double(a.non_near_tie_agree) / double(a.non_near_tie_slots);
        else
            std::cout << ",\"" << name << "_non_near_tie_raw_agreement\":null";
        std::cout << ",\"" << name << "_gated\":" << (gated ? "true" : "false");
    };
    emit_ag("mixed_agreement", s.mixed, /*gated=*/true);
    emit_ag("dict_agreement", s.dict, /*gated=*/false);
    emit_ag("int_agreement", s.ints, /*gated=*/false);
    std::cout << ",\"mixed_disagreement_by_family\":{\"raw\":" << s.mixed_disagreement.raw
              << ",\"dict\":" << s.mixed_disagreement.dict
              << ",\"int\":" << s.mixed_disagreement.ints << "}";
    std::cout << ",\"dict_disagreement_by_family\":{\"raw\":" << s.dict_disagreement.raw
              << ",\"dict\":" << s.dict_disagreement.dict
              << ",\"int\":" << s.dict_disagreement.ints << "}";
    std::cout << ",\"int_disagreement_by_family\":{\"raw\":" << s.ints_disagreement.raw
              << ",\"dict\":" << s.ints_disagreement.dict
              << ",\"int\":" << s.ints_disagreement.ints << "}";
    std::cout << "}";
}

static int g4_measure(const std::string& path, const G2Control& g2) {
    const Bytes src = read_file(path);

    const auto raw_t0 = Clock::now();
    const Bytes raw_br = brotli_encode(src);
    ++g_calls.raw_brotli_q11_calls; // RAW_BROTLI is outside P×F verifier accounting
    const double raw_enc_ms = ms_between(raw_t0, Clock::now());
    (void)raw_enc_ms;
    const Bytes raw_dec = brotli_decode_exact(raw_br, src.size());
    if (raw_dec != src) throw std::runtime_error("raw Brotli roundtrip mismatch");
    const size_t raw_complete = complete_bytes(src.size(), raw_br);

    bool region_ok = true;
    std::string region_reason;
    RegionAnalysis prod_a;    // score-free production analysis
    RegionAnalysis orac_a;    // frozen G3 oracle analysis (labels only)
    RegionAnalysis carrier_a; // score-free carrier basis used for ALL G4 serialization
    double structural_parse_ms = 0;
    double candidate_materialization_ms = 0;
    double o11_label_ms = 0;
    double oracle_reference_parse_ms = 0;
    ProdCandidates pc;
    uint64_t oracle_leaf_calls = 0;

    if (src.empty()) {
        region_ok = false;
        region_reason = "empty input has no frames";
    } else {
        try {
            // ---- PRODUCTION PATH: frozen analyze_regions + score-free build ----
            const auto p0 = Clock::now();
            prod_a = analyze_regions(src);
            const auto p1 = Clock::now();
            pc = build_candidates_no_score(prod_a);
            const auto p2 = Clock::now();
            structural_parse_ms = ms_between(p0, p1);
            candidate_materialization_ms = ms_between(p1, p2);

            // ---- ORACLE PATH: independent frozen analyze_regions + frozen
            //      build_structured_candidates (the isolated q11 labels). ----
            // r5: the reference parse and the frozen builder are timed SEPARATELY.
            // o11_label_ms is the builder only; it is what ranking_score_ms(O11)
            // may use. oracle_reference_parse_ms is reported but is NEVER folded
            // into any ranking/score timing.
            const auto o0 = Clock::now();
            orac_a = analyze_regions(src);
            const auto o1 = Clock::now();
            oracle_reference_parse_ms = ms_between(o0, o1);
            build_structured_candidates(orac_a);
            o11_label_ms = ms_between(o1, Clock::now());
            oracle_leaf_calls = count_oracle_leaf_calls(orac_a);

            // ---- Hard structural identity assertion BEFORE any measurement ----
            assert_production_matches_oracle(prod_a, pc, orac_a);
            carrier_a = build_score_free_carrier_basis(prod_a, pc, orac_a);

            // Frozen-G3 O11 identity is proven on synthetic fixtures in the
            // local tool and on D1-D4 by the CI cross-check against a separately
            // built frozen G3 reference (r5 §14).
        } catch (const std::exception& e) {
            region_ok = false;
            region_reason = e.what();
        }
    }

    if (!region_ok) {
        std::cout << "{"
                  << "\"schema\":5"
                  << ",\"file\":\"" << json_str(path) << "\""
                  << ",\"source_bytes\":" << src.size()
                  << ",\"region_ok\":false"
                  << ",\"region_reason\":\"" << json_str(region_reason) << "\""
                  << ",\"raw_brotli_bytes\":" << raw_br.size()
                  << ",\"raw_complete_bytes\":" << raw_complete
                  << ",\"raw_roundtrip\":true"
                  << ",\"selected_arm\":\"RAW_BROTLI\""
                  << ",\"selected_bytes\":" << raw_complete
                  << "}\n";
        return 0;
    }

    g_calls.oracle_q11_leaf_calls += oracle_leaf_calls;

    // ---- Per-surface selections: score ONCE per candidate, select four masks -
    // r5 5: a candidate's score is computed once per file and reused across all
    // four family masks. r5 G8.6: ranking (score generation) and selection
    // (winner scan) are timed SEPARATELY. O11's ranking surface is the frozen
    // isolated q11 oracle, so its label time is its ranking_score_ms.
    std::array<double, 4> ranking_score_ms{};
    RankingTables tables = build_ranking_tables(pc, orac_a, ranking_score_ms);

    std::array<LeafSelection, 4> o11_sel;   // by Family index
    std::array<std::array<LeafSelection, 4>, 3> proxy_sel; // [surf S0..S2][family]
    std::array<double, 4> selection_ms{};

    {
        const auto t0 = Clock::now();
        for (int f = 0; f < 4; ++f)
            o11_sel[f] = select_from_table(pc, Surface::O11, family_mask(Family(f)),
                                           tables, 0);
        selection_ms[0] = ms_between(t0, Clock::now());
    }
    for (int si = 0; si < 3; ++si) {
        const Surface s = Surface(si + 1);
        const auto t0 = Clock::now();
        for (int f = 0; f < 4; ++f)
            proxy_sel[si][f] = select_from_table(pc, s, family_mask(Family(f)),
                                                 tables, static_cast<size_t>(si + 1));
        selection_ms[si + 1] = ms_between(t0, Clock::now());
    }

    // Deterministic call accounting: S2 spends exactly one q1 evaluation per
    // candidate (computed once in build_ranking_tables); S0/S1 spend none.
    // O11's q11 count is its isolated oracle label enumeration (reference only).
    std::array<SurfaceRow, 4> surfaces_by_si;
    for (int si = 0; si < 4; ++si) {
        surfaces_by_si[si].surface = Surface(si);
        surfaces_by_si[si].surface_name = kSurfaceName[si];
        surfaces_by_si[si].ranking_score_ms = ranking_score_ms[si];
        surfaces_by_si[si].selection_ms = selection_ms[si];
        surfaces_by_si[si].structural_parse_ms = structural_parse_ms;
        surfaces_by_si[si].candidate_materialization_ms = candidate_materialization_ms;
        surfaces_by_si[si].q11_rank_calls = (si == 0) ? oracle_leaf_calls : 0;
        surfaces_by_si[si].q1_rank_calls = (si == 3) ? g_calls.ranking_q1_calls : 0;
    }
    std::array<SurfaceRow, 4> surfaces = surfaces_by_si;
    surfaces[0].surface = Surface::O11;
    surfaces[0].surface_name = kSurfaceName[0];

    // ---- Build and evaluate every P x F carrier ----
    // FROZEN ORDER: RAW, DICT, INT, MIXED (r5 §G8.5). Exactly four paid final
    // q11 carrier encodes per P/file. No dedup.
    std::array<std::array<CarrierEval, 4>, 4> carriers;
    std::array<FamilyCounts, 4> fc_o11;
    for (int f = 0; f < 4; ++f) {
        carriers[0][f] = evaluate_carrier(src, carrier_a, o11_sel[f]);
        fc_o11[f] = count_family_selection(o11_sel[f]);
    }

    std::array<std::array<FamilyCounts, 4>, 3> fc_proxy;
    for (int si = 0; si < 3; ++si) {
        for (int f = 0; f < 4; ++f) {
            carriers[si + 1][f] = evaluate_carrier(src, carrier_a, proxy_sel[si][f]);
            fc_proxy[si][f] = count_family_selection(proxy_sel[si][f]);
        }
    }

    // ---- Per-surface C_region and nominal family label (frozen RAW..MIXED) ----
    auto nominal_family = [](const std::array<CarrierEval, 4>& c) -> std::string {
        const size_t best = std::min({c[0].complete, c[1].complete, c[2].complete, c[3].complete});
        for (int f = 0; f < 4; ++f)
            if (c[f].complete == best) return std::string(kFamilyName[f]);
        return std::string(kFamilyName[0]);
    };
    for (int si = 0; si < 4; ++si) {
        surfaces[si].c_region =
            std::min({carriers[si][0].complete, carriers[si][1].complete,
                      carriers[si][2].complete, carriers[si][3].complete});
        surfaces[si].selected_family = nominal_family(carriers[si]);
        surfaces[si].final_q11_carrier_calls = 4;
    }
    const size_t c_region_o11 = surfaces[0].c_region;

    // ---- Agreements (MIXED primary; DICT/INT diagnostics) ----
    for (int si = 0; si < 3; ++si) {
        surfaces[si + 1].mixed = agreement_metrics(pc, orac_a, kMaskMixed,
                                                   o11_sel[3], proxy_sel[si][3]);
        surfaces[si + 1].dict = agreement_metrics(pc, orac_a, kMaskDict,
                                                  o11_sel[1], proxy_sel[si][1]);
        surfaces[si + 1].ints = agreement_metrics(pc, orac_a, kMaskInt,
                                                  o11_sel[2], proxy_sel[si][2]);
        surfaces[si + 1].mixed_disagreement =
            count_family_disagreement(pc, kMaskMixed, o11_sel[3], proxy_sel[si][3]);
        surfaces[si + 1].dict_disagreement =
            count_family_disagreement(pc, kMaskDict, o11_sel[1], proxy_sel[si][1]);
        surfaces[si + 1].ints_disagreement =
            count_family_disagreement(pc, kMaskInt, o11_sel[2], proxy_sel[si][2]);
    }

    // ---- G2 control binding (identical discipline to frozen G3) ----
    bool g2_bound = false;
    if (g2.provided) {
        if (g2.source_bytes != src.size())
            throw std::runtime_error("G2 control source_bytes mismatch");
        if (g2.raw_complete_bytes != raw_complete)
            throw std::runtime_error("G2 control raw_complete_bytes mismatch");
        g2_bound = true;
    }

    // ---- Emit JSON ----
    std::cout << "{";
    std::cout << "\"schema\":5";
    std::cout << ",\"kind\":\"g4_measure\"";
    std::cout << ",\"file\":\"" << json_str(path) << "\"";
    std::cout << ",\"source_bytes\":" << src.size();
    std::cout << ",\"region_ok\":true";
    std::cout << ",\"region_reason\":\"\"";
    std::cout << ",\"backend\":{\"carrier_quality\":11,\"carrier_window\":" << kBackendWindow
              << ",\"s2_quality\":" << kS2Quality << ",\"s2_window\":" << kBackendWindow << "}";
    std::cout << ",\"raw_brotli_bytes\":" << raw_br.size();
    std::cout << ",\"raw_complete_bytes\":" << raw_complete;
    std::cout << ",\"raw_roundtrip\":true";
    std::cout << ",\"frame_count\":" << orac_a.frames.size();
    std::cout << ",\"structured_frame_count\":" << orac_a.structured_frame_count;
    std::cout << ",\"raw_frame_count\":" << orac_a.raw_frame_count;
    std::cout << ",\"shape_count\":" << pc.shapes.size();
    std::cout << ",\"eligible_slot_count\":" << pc.eligible_slots;
    std::cout << ",\"candidates_enumerated\":" << pc.candidate_count;
    std::cout << ",\"structural_parse_ms\":" << std::fixed << std::setprecision(6)
              << structural_parse_ms;
    std::cout << ",\"candidate_materialization_ms\":" << candidate_materialization_ms;
    std::cout << ",\"o11_label_ms\":" << o11_label_ms;
    std::cout << ",\"oracle_reference_parse_ms\":" << oracle_reference_parse_ms;
    std::cout << ",\"oracle_q11_leaf_calls\":" << oracle_leaf_calls;
    std::cout << ",\"ranking_q11_calls\":" << g_calls.ranking_q11_calls;
    std::cout << ",\"ranking_q1_calls\":" << g_calls.ranking_q1_calls;
    std::cout << ",\"pf_verifier_q11_calls\":" << g_calls.pf_verifier_q11_calls;
    std::cout << ",\"raw_brotli_q11_calls\":" << g_calls.raw_brotli_q11_calls;
    std::cout << ",\"s3_additional_q11_calls\":0";
    std::cout << ",\"s3_q11_rank_calls\":0";
    std::cout << ",\"s3_verifier_calls_are_ranking_calls\":false";

    // O11 region rows (frozen G3 identity carriers; the CI cross-check compares
    // carrier SHA-256 + complete bytes against a separately built frozen G3).
    auto emit_o11 = [&](const char* name, const CarrierEval& e) {
        std::cout << ",\"" << name << "\":{\"carrier_sha256\":\"" << e.carrier_sha256
                  << "\",\"carrier_bytes\":" << e.carrier.size()
                  << ",\"q11_compressed_bytes\":" << e.brotli.size()
                  << ",\"complete_bytes\":" << e.complete
                  << ",\"roundtrip\":" << (e.roundtrip ? "true" : "false") << "}";
    };
    emit_o11("o11_region_raw", carriers[0][0]);
    emit_o11("o11_region_dict", carriers[0][1]);
    emit_o11("o11_region_int", carriers[0][2]);
    emit_o11("o11_region_mixed", carriers[0][3]);

    // G2 control echo (diagnostic only; never enters C_region).
    std::cout << ",\"g2_control_provided\":" << (g2.provided ? "true" : "false");
    std::cout << ",\"g2_control_bound\":" << (g2_bound ? "true" : "false");
    std::cout << ",\"g2_control_eligible\":" << (g2_bound && g2.eligible ? "true" : "false");
    if (g2_bound) {
        std::cout << ",\"g2_whole_selected_arm\":\"" << json_str(g2.selected_arm) << "\"";
        std::cout << ",\"g2_whole_complete_bytes\":" << g2.selected_bytes;
        std::cout << ",\"g2_whole_delta_pct\":" << delta_pct(g2.selected_bytes, raw_complete);
    }

    // 16 P x F rows, frozen order P outer (O11,S0,S1,S2) then F (RAW,DICT,INT,MIXED).
    // Each row mirrors its parent surface's authoritative ranking-call counts so
    // a reader cannot mistake a PF row for its own ranking budget, and reports
    // exactly one paid verifier call for this carrier.
    for (int si = 0; si < 4; ++si) {
        for (int f = 0; f < 4; ++f) {
            PFRow r;
            r.surface = Surface(si);
            r.family = Family(f);
            r.surface_name = kSurfaceName[si];
            r.family_name = kFamilyName[f];
            r.label = std::string(kSurfaceName[si]) + "_REGION_" + kFamilyName[f];
            r.carrier = carriers[si][f];
            r.selected = (si == 0) ? fc_o11[f] : fc_proxy[si - 1][f];
            r.surface_q11_rank_calls = surfaces[si].q11_rank_calls;
            r.surface_q1_rank_calls = surfaces[si].q1_rank_calls;
            emit_pf_row(r, c_region_o11);
        }
    }

    // Per-surface rows.
    for (int si = 0; si < 4; ++si) emit_surface_row(surfaces[si], c_region_o11);

    // ---- r5 §G8.9 nominal final-portfolio diagnostic (exact, not hardcoded) ----
    // Over {RAW_BROTLI, G2_WHOLE if eligible, all 16 P×F carriers}, using the
    // frozen §6.3 GENERIC diagnostic tie order: RAW_BROTLI, then O11, then S0,
    // then S1, then S2. (This order is for the generic diagnostic ONLY; S3 has
    // its own §4.7.5 order and is reconstructed in the workflow.)
    struct DiagChoice { std::string label; size_t bytes; int rank; };
    DiagChoice diag{"RAW_BROTLI", raw_complete, 0};
    auto consider_diag = [&](const std::string& label, size_t bytes,
                             int rank) {
        if (bytes < diag.bytes || (bytes == diag.bytes && rank < diag.rank))
            diag = DiagChoice{label, bytes, rank};
    };
    if (g2_bound && g2.eligible) consider_diag("G2_WHOLE", g2.selected_bytes, 1);
    for (int si = 0; si < 4; ++si) {
        const int rank = (si == 0) ? 1 : (si + 1); // O11..S2 -> 1..4 (RAW is 0)
        for (int f = 0; f < 4; ++f)
            consider_diag(std::string(kSurfaceName[si]) + "_REGION_" + kFamilyName[f],
                          carriers[si][f].complete, rank);
    }

    std::cout << ",\"c_region_o11_bytes\":" << c_region_o11;
    std::cout << ",\"generic_diagnostic_selected_label\":\"" << diag.label << "\"";
    std::cout << ",\"generic_diagnostic_selected_bytes\":" << diag.bytes;
    std::cout << ",\"generic_diagnostic_note\":\"RAW_BROTLI/G2_WHOLE are diagnostics and are EXCLUDED from C_region, G8.2 and G8.3; S3 uses its own §4.7.5 order\"";
    std::cout << ",\"peak_rss_bytes\":" << peak_rss_bytes();
    std::cout << ",\"peak_rss_scope\":\"whole-process peak (VmHWM on Linux; PeakWorkingSetSize on Windows); not per-surface\"";
    std::cout << "}\n";
    return 0;
}

// ---------------------------------------------------------------------------
// Q2 bounded outcome-blind pairwise separability probe (r5 §10; task F)
// ---------------------------------------------------------------------------
//
// CI-ONLY surface. `probe` is never the default local measurement command and
// no D1-D4 probe is ever run locally.

// order_key(file, shape, slot) = SHA-256( file_name_ascii || 0x1F ||
//   shape_id_le_u64 || 0x1F || slot_id_le_u64 )
static std::array<uint8_t, 32> probe_order_key(const std::string& file_name,
                                               uint64_t shape_id, uint64_t slot_id) {
    Bytes b;
    b.insert(b.end(), file_name.begin(), file_name.end());
    b.push_back(0x1F);
    for (int i = 0; i < 8; ++i) b.push_back(uint8_t(shape_id >> (i * 8)));
    b.push_back(0x1F);
    for (int i = 0; i < 8; ++i) b.push_back(uint8_t(slot_id >> (i * 8)));
    return sha256::hash(b);
}

struct ProbeColumn {
    size_t sid = 0;
    size_t slot = 0;
    uint64_t shape_id = 0;
    uint64_t slot_id = 0;
    std::array<uint8_t, 32> order_key{};
};

static int g4_probe(const std::string& path, const G2Control& g2) {
    (void)g2;
    const Bytes src = read_file(path);
    // r5 10.8: an incomplete probe is INCONCLUSIVE-INFRA, never a relaxed probe
    // and never a hard tool crash. An input with no frames cannot be probed.
    if (src.empty()) {
        std::cout << "{\"schema\":5,\"kind\":\"q2_pairwise_probe\",\"file\":\""
                  << json_str(path) << "\",\"file_name\":\"" << json_str(path)
                  << "\",\"region_ok\":false,"
                  << "\"verdict\":\"INCONCLUSIVE-INFRA\","
                  << "\"reason\":\"empty input has no frames\"}\n";
        return 0;
    }

    // Production + oracle analyses with the same hard identity assertion.
    RegionAnalysis prod_a = analyze_regions(src);
    const ProdCandidates pc = build_candidates_no_score(prod_a);
    RegionAnalysis orac_a = analyze_regions(src);
    build_structured_candidates(orac_a);
    assert_production_matches_oracle(prod_a, pc, orac_a);
    const RegionAnalysis carrier_a = build_score_free_carrier_basis(prod_a, pc, orac_a);
    g_calls.oracle_q11_leaf_calls += count_oracle_leaf_calls(orac_a);

    // O11 REGION_MIXED selection (the probe's only planning surface).
    const LeafSelection o11_mixed = select_o11(pc, orac_a, kMaskMixed);

    // --- Outcome-blind sampling: eligible columns have >= 2 eligible leaves
    //     under the REGION_MIXED mask. ---
    const std::string fname = [&]() {
        std::string s = path;
        const size_t p = s.find_last_of("/\\");
        return p == std::string::npos ? s : s.substr(p + 1);
    }();
    std::vector<ProbeColumn> eligible;
    for (size_t sid = 0; sid < pc.shapes.size(); ++sid) {
        for (size_t slot = 0; slot < pc.shapes[sid].slots.size(); ++slot) {
            const ProdColumn col = column_view(pc.shapes[sid].slots[slot], kMaskMixed);
            if (col.ordered.size() >= 2) {
                ProbeColumn c;
                c.sid = sid;
                c.slot = slot;
                c.shape_id = pc.shapes[sid].id;
                c.slot_id = slot;
                c.order_key = probe_order_key(fname, c.shape_id, c.slot_id);
                eligible.push_back(c);
            }
        }
    }
    // Sort ascending by raw 32-byte digest as an unsigned big-endian integer.
    std::sort(eligible.begin(), eligible.end(), [](const ProbeColumn& x, const ProbeColumn& y) {
        if (x.order_key != y.order_key) return x.order_key < y.order_key;
        if (x.shape_id != y.shape_id) return x.shape_id < y.shape_id;
        return x.slot_id < y.slot_id;
    });
    const size_t n = std::min<size_t>(4, eligible.size());
    std::vector<ProbeColumn> sampled(eligible.begin(), eligible.begin() + n);

    // --- Independent O11 MIXED reference carrier: every sampled AND non-sampled
    //     column stays at its O11 REGION_MIXED choice. ---
    const Bytes indep_carrier = build_carrier_only(src, carrier_a, o11_mixed);
    const Bytes indep_brotli = brotli_encode(indep_carrier);
    ++g_calls.probe_q11_calls;
    const size_t c_indep = complete_bytes(src.size(), indep_brotli);
    if (brotli_decode_bounded(indep_brotli,
                              static_cast<size_t>(std::min<uint64_t>(
                                  kMaxDecoded, uint64_t(src.size()) * 4ull + kMaxCarrierSlack))) !=
        indep_carrier)
        throw std::runtime_error("probe: independent carrier roundtrip mismatch");
    const std::string indep_sha = sha256::hex(indep_carrier);

    // --- All unordered sampled pairs; all Cartesian eligible leaves per pair. ---
    struct PairResult {
        size_t i = 0, j = 0;
        uint64_t shape_i = 0, slot_i = 0, shape_j = 0, slot_j = 0;
        std::vector<std::string> leaves_i;
        std::vector<std::string> leaves_j;
        uint64_t q11_calls = 0;
        size_t c_pair = std::numeric_limits<size_t>::max();
        std::string best_leaf_i;
        std::string best_leaf_j;
        std::string best_carrier_sha256;
    };
    std::vector<PairResult> pairs;
    for (size_t pi = 0; pi < sampled.size(); ++pi) {
        for (size_t pj = pi + 1; pj < sampled.size(); ++pj) {
            PairResult pr;
            pr.i = pi;
            pr.j = pj;
            pr.shape_i = sampled[pi].shape_id;
            pr.slot_i = sampled[pi].slot_id;
            pr.shape_j = sampled[pj].shape_id;
            pr.slot_j = sampled[pj].slot_id;
            const ProdColumn ci = column_view(pc.shapes[sampled[pi].sid].slots[sampled[pi].slot],
                                              kMaskMixed);
            const ProdColumn cj = column_view(pc.shapes[sampled[pj].sid].slots[sampled[pj].slot],
                                              kMaskMixed);
            for (const ProdCandidate* c : ci.ordered) pr.leaves_i.push_back(leaf_name(c->id));
            for (const ProdCandidate* c : cj.ordered) pr.leaves_j.push_back(leaf_name(c->id));

            for (const ProdCandidate* li : ci.ordered) {
                for (const ProdCandidate* lj : cj.ordered) {
                    LeafSelection sel = o11_mixed; // non-sampled stay at O11 MIXED
                    sel[sampled[pi].sid][sampled[pi].slot] = li->id;
                    sel[sampled[pj].sid][sampled[pj].slot] = lj->id;
                    const Bytes carrier = build_carrier_only(src, carrier_a, sel);
                    const Bytes br = brotli_encode(carrier); // exactly one q11 call
                    ++g_calls.probe_q11_calls;
                    ++pr.q11_calls;
                    const size_t comp = complete_bytes(src.size(), br);
                    if (comp < pr.c_pair) {
                        pr.c_pair = comp;
                        pr.best_leaf_i = leaf_name(li->id);
                        pr.best_leaf_j = leaf_name(lj->id);
                        pr.best_carrier_sha256 = sha256::hex(carrier);
                    }
                }
            }
            pairs.push_back(std::move(pr));
        }
    }

    // --- Probe verdict (r5 §10.6) ---
    double max_pair_gain = 0.0;
    double sum_indep = 0.0, sum_pair = 0.0;
    for (const PairResult& pr : pairs) {
        const double gain = c_indep
            ? (double(c_indep) - double(pr.c_pair)) / double(c_indep) * 100.0 : 0.0;
        if (gain > max_pair_gain) max_pair_gain = gain;
        sum_indep += double(c_indep);
        sum_pair += double(pr.c_pair);
    }
    const double agg_pair_gain =
        sum_indep > 0.0 ? (sum_indep - sum_pair) / sum_indep * 100.0 : 0.0;
    const bool nonseparable = (max_pair_gain > 0.50) || (agg_pair_gain > 0.25);
    const char* verdict = nonseparable ? "NONSEPARABLE" : "SEPARABLE-ENOUGH";

    // --- Emit probe JSON ---
    std::cout << "{";
    std::cout << "\"schema\":5";
    std::cout << ",\"kind\":\"q2_pairwise_probe\"";
    std::cout << ",\"file\":\"" << json_str(path) << "\"";
    std::cout << ",\"file_name\":\"" << json_str(fname) << "\"";
    std::cout << ",\"source_bytes\":" << src.size();
    std::cout << ",\"eligible_column_count\":" << eligible.size();
    std::cout << ",\"sampled_column_count\":" << sampled.size();
    std::cout << ",\"c_indep_mixed_complete_bytes\":" << c_indep;
    std::cout << ",\"c_indep_mixed_carrier_sha256\":\"" << indep_sha << "\"";
    std::cout << ",\"non_sampled_slots_at_o11_mixed\":true";
    std::cout << ",\"sampled_columns\":[";
    for (size_t k = 0; k < sampled.size(); ++k) {
        if (k) std::cout << ",";
        const ProdColumn col = column_view(pc.shapes[sampled[k].sid].slots[sampled[k].slot],
                                           kMaskMixed);
        std::cout << "{\"index\":" << k
                  << ",\"shape_id\":" << sampled[k].shape_id
                  << ",\"slot_id\":" << sampled[k].slot_id
                  << ",\"order_key_sha256\":\"" << sha256::hex(sampled[k].order_key) << "\""
                  << ",\"o11_mixed_leaf\":\""
                  << leaf_name(o11_mixed[sampled[k].sid][sampled[k].slot]) << "\""
                  << ",\"eligible_leaves\":[";
        for (size_t x = 0; x < col.ordered.size(); ++x) {
            if (x) std::cout << ",";
            std::cout << "\"" << leaf_name(col.ordered[x]->id) << "\"";
        }
        std::cout << "]}";
    }
    std::cout << "]";
    std::cout << ",\"pairs\":[";
    for (size_t k = 0; k < pairs.size(); ++k) {
        const PairResult& pr = pairs[k];
        if (k) std::cout << ",";
        std::cout << "{\"i\":" << pr.i << ",\"j\":" << pr.j
                  << ",\"shape_i\":" << pr.shape_i << ",\"slot_i\":" << pr.slot_i
                  << ",\"shape_j\":" << pr.shape_j << ",\"slot_j\":" << pr.slot_j
                  << ",\"leaves_i\":[";
        for (size_t x = 0; x < pr.leaves_i.size(); ++x) {
            if (x) std::cout << ",";
            std::cout << "\"" << pr.leaves_i[x] << "\"";
        }
        std::cout << "],\"leaves_j\":[";
        for (size_t x = 0; x < pr.leaves_j.size(); ++x) {
            if (x) std::cout << ",";
            std::cout << "\"" << pr.leaves_j[x] << "\"";
        }
        std::cout << "]";
        std::cout << ",\"leaf_pair_count\":" << (pr.leaves_i.size() * pr.leaves_j.size());
        std::cout << ",\"q11_calls\":" << pr.q11_calls;
        std::cout << ",\"c_pair_complete_bytes\":" << pr.c_pair;
        std::cout << ",\"best_leaf_i\":\"" << pr.best_leaf_i << "\"";
        std::cout << ",\"best_leaf_j\":\"" << pr.best_leaf_j << "\"";
        std::cout << ",\"best_carrier_sha256\":\"" << pr.best_carrier_sha256 << "\"";
        std::cout << ",\"pair_gain_pct\":"
                  << (c_indep ? (double(c_indep) - double(pr.c_pair)) / double(c_indep) * 100.0
                              : 0.0);
        std::cout << "}";
    }
    std::cout << "]";
    std::cout << ",\"pair_count\":" << pairs.size();
    std::cout << ",\"max_pair_gain_pct\":" << std::fixed << std::setprecision(6) << max_pair_gain;
    std::cout << ",\"agg_pair_gain_pct\":" << agg_pair_gain;
    std::cout << ",\"max_pair_gain_threshold_pct\":0.50";
    std::cout << ",\"agg_pair_gain_threshold_pct\":0.25";
    std::cout << ",\"verdict\":\"" << verdict << "\"";
    std::cout << ",\"probe_q11_calls\":" << g_calls.probe_q11_calls;
    std::cout << ",\"oracle_q11_leaf_calls\":" << count_oracle_leaf_calls(orac_a);
    std::cout << "}\n";
    return 0;
}

// ---------------------------------------------------------------------------
// Peak RSS (whole-process diagnostic; reported, never used for scoring).
// Linux: /proc/self/status VmHWM. Windows: GetProcessMemoryInfo
// PeakWorkingSetSize. Reported with an explicit scope; never fabricated as 0
// when the host can answer.
// ---------------------------------------------------------------------------

static uint64_t peak_rss_bytes_impl() {
#if defined(__linux__)
    std::ifstream f("/proc/self/status");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("VmHWM:", 0) == 0) {
            const std::string val = line.substr(6);
            return static_cast<uint64_t>(std::strtoull(val.c_str(), nullptr, 10)) * 1024ull;
        }
    }
    return 0;
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc)))
        return static_cast<uint64_t>(pmc.PeakWorkingSetSize);
    return 0;
#else
    return 0; // unavailable on this host; reported with an explicit scope note
#endif
}

static uint64_t peak_rss_bytes() { return peak_rss_bytes_impl(); }

// ---------------------------------------------------------------------------
// Self-tests (adversarial; r5 §G8.1 correctness + determinism + leakage)
// ---------------------------------------------------------------------------

static void g4_require(bool cond, const char* label) {
    if (!cond) throw std::runtime_error(std::string("selftest failed: ") + label);
}

// S1 formula exactness on hand cases (r5 §4.4).
static void s1_formula_selftest() {
    // Single symbol alphabet: ceil_log2(N) - floor_log2(N) = 0 for a run.
    {
        Bytes obj(8, 'A');
        g4_require(h0_scaled(obj) == 0, "S1 constant run zero H0");
    }
    // Two symbols 50/50, N=100: ceil_log2(100)=7, floor_log2(50)=5 -> 2.
    // H0 = 256*50*2 + 256*50*2 = 51200.
    {
        Bytes obj(50, 'A');
        obj.insert(obj.end(), 50, 'B');
        g4_require(h0_scaled(obj) == 51200ull, "S1 50/50 H0");
    }
    // Four distinct bytes, N=4: ceil_log2(4)=2, floor_log2(1)=0 -> 2 each.
    // H0 = 4 * 256 * 1 * 2 = 2048.
    {
        Bytes obj = bytes("ABCD");
        g4_require(h0_scaled(obj) == 2048ull, "S1 four distinct H0");
    }
    // ceil/floor exactness at powers of two.
    g4_require(ceil_log2_u64(1) == 0, "ceil_log2(1)");
    g4_require(ceil_log2_u64(2) == 1, "ceil_log2(2)");
    g4_require(ceil_log2_u64(3) == 2, "ceil_log2(3)");
    g4_require(ceil_log2_u64(4) == 2, "ceil_log2(4)");
    g4_require(ceil_log2_u64(5) == 3, "ceil_log2(5)");
    g4_require(floor_log2_u64(1) == 0, "floor_log2(1)");
    g4_require(floor_log2_u64(4) == 2, "floor_log2(4)");
    g4_require(floor_log2_u64(7) == 2, "floor_log2(7)");
}

// Equal-H0 / different-S0 must NOT be a tie; tuple decides (r5 §6.1).
static void s1_equal_h0_different_s0_selftest() {
    S1Key a{1000, 40};
    S1Key b{1000, 41};
    g4_require(s1_less(a, b), "S1 tuple: smaller S0 wins at equal H0");
    g4_require(!s1_less(b, a), "S1 tuple antisymmetry");
    g4_require(!s1_equal(a, b), "S1 equal H0 with different S0 is NOT a tie");
    S1Key c{1000, 40};
    g4_require(s1_equal(a, c), "S1 full tuple tie detected");
    // Lexicographic on H0 dominates S0_size.
    S1Key d{999, 9999};
    g4_require(s1_less(d, a), "S1 H0 dominates S0_size");

    // Exercise the real selection path: score keys must be deterministic.
    RegionAnalysis alpha = analyze_regions(bytes("{\"a\":1}\n{\"a\":2}\n{\"a\":3}\n"));
    const ProdCandidates pc = build_candidates_no_score(alpha);
    const ProdShape& sh = pc.shapes[0];
    for (const ProdSlot& sl : sh.slots) {
        const ProdColumn col = column_view(sl, kMaskMixed);
        S1Key k0 = score_s1(*col.ordered[0], col.occurrences);
        S1Key k1 = score_s1(*col.ordered[0], col.occurrences);
        g4_require(s1_equal(k0, k1), "S1 key determinism");
    }
}

// Full tuple RAW tie semantics: equal (H0,S0) keeps RAW when RAW is incumbent
// and no proxy beats it; and a non-RAW with a strictly smaller tuple wins even
// when RAW is present (r5 §6.1 explicit).
static void s1_full_tuple_raw_tie_selftest() {
    ProdSlot slot;
    slot.tokens = {bytes("\"z\"")};
    ProdCandidate raw;
    raw.id = LeafId::RawLex;
    raw.payload = make_raw_payload(slot.tokens);
    ProdCandidate dict;
    dict.id = LeafId::ExactDict;
    dict.payload = make_dict_payload(slot.tokens);
    slot.candidates = {raw, dict};

    const ProdColumn col = column_view(slot, kMaskMixed);
    g4_require(col.ordered.size() == 2, "S1 tie: two candidates");
    const S1Key k_raw = score_s1(*col.ordered[0], col.occurrences);
    const S1Key k_dict = score_s1(*col.ordered[1], col.occurrences);
    if (s1_equal(k_raw, k_dict)) {
        g4_require(select_index_s1(col, nullptr) == 0, "S1 full-tuple tie keeps RAW_LEX");
    } else if (s1_less(k_dict, k_raw)) {
        g4_require(select_index_s1(col, nullptr) == 1, "S1 strictly smaller tuple wins");
    } else {
        g4_require(select_index_s1(col, nullptr) == 0, "S1 larger tuple loses to RAW");
    }
    // Scalar surfaces: an exact scalar tie keeps RAW_LEX (incumbent).
    {
        ProdSlot s2;
        s2.tokens = {bytes("\"z\"")};
        ProdCandidate a;
        a.id = LeafId::RawLex;
        a.payload = bytes("aaaa");
        ProdCandidate b;
        b.id = LeafId::ExactDict;
        b.payload = bytes("bbbb");
        s2.candidates = {a, b};
        const ProdColumn c = column_view(s2, kMaskMixed);
        const uint64_t sa = score_s0(*c.ordered[0], c.occurrences);
        const uint64_t sb = score_s0(*c.ordered[1], c.occurrences);
        const size_t w = select_index_s0(c, nullptr);
        if (sa == sb) g4_require(w == 0, "S0 scalar tie keeps RAW_LEX");
    }
}

// Determinism: same column scored twice is identical; selection stable.
static void determinism_selftest() {
    RegionAnalysis a = analyze_regions(
        bytes("{\"a\":1,\"b\":\"x\"}\n{\"a\":2,\"b\":\"y\"}\n{\"a\":3,\"b\":\"x\"}\n"));
    const ProdCandidates pc = build_candidates_no_score(a);
    for (const ProdShape& sh : pc.shapes) {
        for (const ProdSlot& sl : sh.slots) {
            const ProdColumn col = column_view(sl, kMaskMixed);
            g4_require(select_index_s0(col, nullptr) == select_index_s0(col, nullptr),
                       "S0 selection determinism");
            S1Key k1, k2;
            const size_t i1 = select_index_s1(col, &k1);
            const size_t i2 = select_index_s1(col, &k2);
            g4_require(i1 == i2 && s1_equal(k1, k2), "S1 selection determinism");
        }
    }
}

// Production materializer must reproduce the oracle path structure exactly
// (task A.4) and must make ZERO oracle calls while building candidates.
static void production_oracle_identity_selftest() {
    const std::vector<std::string> fixtures = {
        "{\"a\":1,\"b\":\"x\",\"n\":null}\n{\"a\":2,\"b\":\"y\",\"n\":null}\n"
        "{\"a\":3,\"b\":\"x\",\"n\":null}\n{\"a\":4,\"b\":\"y\",\"n\":null}\n",
        "{\"a\":100,\"b\":\"p\"}\r\n{\"a\":101,\"b\":\"q\"}\r\n{\"a\":102,\"b\":\"p\"}\r\n",
        "{\"a\":1}\n{\"a\":02}\n{\"a\":3}\n",
        "{\"a\":1}\n{\"a\":2}\n{\"a\":",
        "\x01\x02 not json\n",
    };
    for (const std::string& text : fixtures) {
        const Bytes src = bytes(text);
        RegionAnalysis prod_a = analyze_regions(src);
        const ProdCandidates pc = build_candidates_no_score(prod_a);
        RegionAnalysis orac_a = analyze_regions(src);
        build_structured_candidates(orac_a);
        // Must not throw and must match the oracle-path G3 analysis exactly,
        // including the slot metric fields that only the frozen G3 builder sets.
        assert_production_matches_oracle(prod_a, pc, orac_a);
        // Payload bytes are identical to frozen G3's own candidates.
        for (size_t sid = 0; sid < orac_a.shapes.size(); ++sid) {
            for (size_t slot = 0; slot < orac_a.shapes[sid].slots.size(); ++slot) {
                const auto& gc = orac_a.shapes[sid].slots[slot].candidates;
                const auto& pcand = pc.shapes[sid].slots[slot].candidates;
                g4_require(gc.size() == pcand.size(), "identity candidate count");
                for (size_t ci = 0; ci < gc.size(); ++ci) {
                    g4_require(gc[ci].id == pcand[ci].id, "identity leaf id");
                    g4_require(gc[ci].payload == pcand[ci].payload, "identity payload bytes");
                }
            }
        }
    }
    // Zero-call invariant: building S0/S1 selections must not add any oracle or
    // ranking backend calls beyond what we explicitly account for.
    const uint64_t before_q11 = g_calls.ranking_q11_calls;
    const uint64_t before_q1 = g_calls.ranking_q1_calls;
    RegionAnalysis a = analyze_regions(bytes("{\"a\":1}\n{\"a\":2}\n"));
    const ProdCandidates pc = build_candidates_no_score(a);
    for (int f = 0; f < 4; ++f) {
        (void)select_proxy(pc, Surface::S0, family_mask(Family(f)));
        (void)select_proxy(pc, Surface::S1, family_mask(Family(f)));
    }
    g4_require(g_calls.ranking_q11_calls == before_q11, "S0/S1 make zero q11 ranking calls");
    g4_require(g_calls.ranking_q1_calls == before_q1, "S0/S1 make zero q1 ranking calls");
}

// Oracle-leakage proof: mutate every oracle label arbitrarily, and prove that
// every proxy selection (S0/S1/S2) is bit-for-bit unchanged. S2 does not read
// labels at all; the assertion is still meaningful as a structural guard.
static void oracle_leakage_selftest() {
    RegionAnalysis prod_a = analyze_regions(
        bytes("{\"a\":1,\"b\":\"lorem\"}\n{\"a\":2,\"b\":\"ipsum\"}\n"
              "{\"a\":3,\"b\":\"lorem\"}\n{\"a\":-40,\"b\":\"dolor\"}\n"
              "{\"a\":-39,\"b\":\"sit\"}\n{\"a\":-38,\"b\":\"dolor\"}\n"));
    const ProdCandidates pc = build_candidates_no_score(prod_a);
    RegionAnalysis orac_a = analyze_regions(
        bytes("{\"a\":1,\"b\":\"lorem\"}\n{\"a\":2,\"b\":\"ipsum\"}\n"
              "{\"a\":3,\"b\":\"lorem\"}\n{\"a\":-40,\"b\":\"dolor\"}\n"
              "{\"a\":-39,\"b\":\"sit\"}\n{\"a\":-38,\"b\":\"dolor\"}\n"));
    build_structured_candidates(orac_a);

    auto take = [&]() {
        std::array<LeafSelection, 3> sels;
        for (int si = 0; si < 3; ++si)
            sels[si] = select_proxy(pc, Surface(si + 1), kMaskMixed);
        return sels;
    };
    const auto before = take();

    // Corrupt every oracle label in both directions. The proxy selections must
    // be unaffected because the score-free production structure is what they
    // read.
    for (auto& per_shape : orac_a.shapes)
        for (auto& per_slot : per_shape.slots)
            for (auto& cand : per_slot.candidates)
                cand.isolated_brotli_bytes = (cand.isolated_brotli_bytes == 0 ? 999999u : 0u);
    const auto after = take();

    for (int si = 0; si < 3; ++si)
        g4_require(before[si] == after[si],
                   "proxy selection unaffected by oracle label mutation");
}

// Carrier identity: O11's selection reproduces frozen G3's own regional carrier
// byte-for-byte for ALL FOUR families, and every (P,F) carrier round-trips.
// This is the local synthetic equality test required by r5 §14 / task D.
static void carrier_identity_selftest() {
    const std::vector<std::string> fixtures = {
        "{\"a\":1,\"b\":\"x\",\"n\":null}\n{\"a\":2,\"b\":\"y\",\"n\":null}\n"
        "{\"a\":3,\"b\":\"x\",\"n\":null}\n{\"a\":4,\"b\":\"y\",\"n\":null}\n",
        "{\"a\":100,\"b\":\"p\"}\r\n{\"a\":101,\"b\":\"q\"}\r\n{\"a\":102,\"b\":\"p\"}\r\n",
        "{\"a\":1}\n{\"a\":02}\n{\"a\":3}\n",
        "{\"a\":1}\n{\"a\":2}\n{\"a\":",
        "{\"i\":1,\"j\":[10,12,15],\"k\":\"aa\"}\n{\"i\":2,\"j\":[20,22,25],\"k\":\"aa\"}\n"
        "{\"i\":3,\"j\":[30,32,35],\"k\":\"bb\"}\n",
        "{}\n[]\ngarbage\n{\"z\":7}\n",
    };
    for (const std::string& text : fixtures) {
        const Bytes src = bytes(text);
        RegionAnalysis prod_a = analyze_regions(src);
        const ProdCandidates pc = build_candidates_no_score(prod_a);
        RegionAnalysis orac_a = analyze_regions(src);
        build_structured_candidates(orac_a);
        assert_production_matches_oracle(prod_a, pc, orac_a);

        // Frozen G3 reference selections + carriers for all four families.
        const std::array<const std::vector<LeafId>*, 4> masks = {
            &kMaskRaw, &kMaskDict, &kMaskInt, &kMaskMixed};
        for (int f = 0; f < 4; ++f) {
            const auto g3_sel = local_selection(orac_a, *masks[f]);
            Selection g3_frozen;
            g3_frozen.structured_shapes = g3_sel;
            CarrierStats g3_stats;
            const Bytes g3_carrier = make_region_carrier(src, orac_a, g3_frozen, g3_stats);

            // G4 O11 selection for this family must reproduce it byte-for-byte.
            const LeafSelection o11 = select_o11(pc, orac_a, *masks[f]);
            Selection o11_frozen;
            o11_frozen.structured_shapes = o11;
            CarrierStats o11_stats;
            const Bytes o11_carrier = make_region_carrier(src, orac_a, o11_frozen, o11_stats);
            g4_require(o11_carrier == g3_carrier,
                       "O11 carrier == frozen G3 carrier for this family");
            g4_require(decode_region_carrier(o11_carrier) == src, "O11 carrier roundtrip");
        }

        // Every proxy x family carrier must round-trip exactly.
        for (int si = 0; si < 3; ++si) {
            for (int f = 0; f < 4; ++f) {
                const LeafSelection sel = select_proxy(pc, Surface(si + 1), family_mask(Family(f)));
                Selection fs;
                fs.structured_shapes = sel;
                CarrierStats st;
                const Bytes c = make_region_carrier(src, orac_a, fs, st);
                g4_require(decode_region_carrier(c) == src, "proxy carrier roundtrip");
                const Bytes br = brotli_encode(c);
                const Bytes cd = brotli_decode_bounded(
                    br, static_cast<size_t>(std::min<uint64_t>(
                            kMaxDecoded, uint64_t(src.size()) * 4ull + kMaxCarrierSlack)));
                g4_require(decode_region_carrier(cd) == src, "proxy brotli carrier roundtrip");
            }
        }

        // REGION_RAW planner-independence invariant (r5 §G5.6 / §14).
        const LeafSelection raw_o11 = select_o11(pc, orac_a, kMaskRaw);
        for (int si = 0; si < 3; ++si) {
            const LeafSelection raw_p = select_proxy(pc, Surface(si + 1), kMaskRaw);
            Selection fs_a, fs_b;
            fs_a.structured_shapes = raw_o11;
            fs_b.structured_shapes = raw_p;
            CarrierStats sa, sb;
            g4_require(make_region_carrier(src, orac_a, fs_a, sa) ==
                           make_region_carrier(src, orac_a, fs_b, sb),
                       "REGION_RAW planner-independent");
        }
    }
}

// Malformed / fallback behavior must match frozen G3's fail-closed policy.
static void malformed_fallback_selftest() {
    {
        RegionAnalysis a = analyze_regions(bytes(""));
        g4_require(a.frames.empty(), "empty has no frames");
    }
    {
        const Bytes src = bytes("\x01\x02\x03 not json at all\n\xff\xfe");
        RegionAnalysis prod_a = analyze_regions(src);
        const ProdCandidates pc = build_candidates_no_score(prod_a);
        RegionAnalysis orac_a = analyze_regions(src);
        build_structured_candidates(orac_a);
        g4_require(orac_a.shapes.empty(), "malformed has no structured shape");
        g4_require(orac_a.raw_members.size() == 2, "malformed raw frame count");
        g4_require(pc.shapes.empty(), "malformed production has no shape");
        Selection fs; // no structured shapes
        CarrierStats st;
        const Bytes c = make_region_carrier(src, orac_a, fs, st);
        g4_require(decode_region_carrier(c) == src, "raw-only malformed roundtrip");
    }
    {
        const Bytes src = bytes("{\"a\":1}\n{\"a\":2}\n");
        RegionAnalysis prod_a = analyze_regions(src);
        const ProdCandidates pc = build_candidates_no_score(prod_a);
        RegionAnalysis orac_a = analyze_regions(src);
        build_structured_candidates(orac_a);
        assert_production_matches_oracle(prod_a, pc, orac_a);
        const RegionAnalysis carrier_a = build_score_free_carrier_basis(prod_a, pc, orac_a);
        const LeafSelection sel = select_proxy(pc, Surface::S0, kMaskMixed);
        Selection fs;
        fs.structured_shapes = sel;
        CarrierStats st;
        Bytes c = make_region_carrier(src, carrier_a, fs, st);
        Bytes bad = c;
        bad.pop_back();
        require_throw([&] { (void)decode_region_carrier(bad); }, "g4 truncated carrier");
        bad = c;
        bad.push_back(0);
        require_throw([&] { (void)decode_region_carrier(bad); }, "g4 trailing byte");
        bad = c;
        bad[0] ^= 1;
        require_throw([&] { (void)decode_region_carrier(bad); }, "g4 bad magic");
    }
}

// SHA-256 known-answer tests (the CI identity/probe depend on this).
static void sha256_selftest() {
    g4_require(sha256::hex(bytes("")) ==
                   "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
               "sha256 empty");
    g4_require(sha256::hex(bytes("abc")) ==
                   "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
               "sha256 abc");
    g4_require(sha256::hex(bytes("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")) ==
                   "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
               "sha256 448-bit");
}

// order_key must be deterministic and layout-exact (r5 §10.2).
static void probe_order_key_selftest() {
    const auto k1 = probe_order_key("f.ndjson", 1, 2);
    const auto k2 = probe_order_key("f.ndjson", 1, 2);
    const auto k3 = probe_order_key("f.ndjson", 1, 3);
    g4_require(k1 == k2, "order_key deterministic");
    g4_require(k1 != k3, "order_key distinguishes slot");
    Bytes manual;
    manual.insert(manual.end(), {'f', '.', 'n', 'd', 'j', 's', 'o', 'n'});
    manual.push_back(0x1F);
    for (int i = 0; i < 8; ++i) manual.push_back(uint8_t(1ull >> (i * 8)));
    manual.push_back(0x1F);
    for (int i = 0; i < 8; ++i) manual.push_back(uint8_t(2ull >> (i * 8)));
    g4_require(sha256::hash(manual) == k1, "order_key byte layout");
}

// S2 score reuse (r5 5, G8.5): the cached ranking table must (a) make EXACTLY
// one q1 evaluation per enumerated candidate, and (b) produce selections that
// are bit-identical to the direct per-column S2 selector, reused across all four
// family masks.
static void cached_score_reuse_selftest() {
    const std::vector<std::string> fixtures = {
        "{\"a\":1,\"b\":\"x\"}\n{\"a\":2,\"b\":\"y\"}\n"
        "{\"a\":3,\"b\":\"x\"}\n{\"a\":4,\"b\":\"z\"}\n",
        "{\"n\":-7,\"s\":\"aa\"}\n{\"n\":-6,\"s\":\"ab\"}\n"
        "{\"n\":-5,\"s\":\"aa\"}\n{\"n\":-4,\"s\":\"ab\"}\n",
        "plain text line one\nplain text line two\n",
    };
    for (const std::string& text : fixtures) {
        const Bytes src = bytes(text);
        RegionAnalysis prod_a = analyze_regions(src);
        const ProdCandidates pc = build_candidates_no_score(prod_a);
        RegionAnalysis orac_a = analyze_regions(src);
        build_structured_candidates(orac_a);
        assert_production_matches_oracle(prod_a, pc, orac_a);

        // Build all four surfaces' tables and count the q1 calls S2 spends.
        const uint64_t q1_before = g_calls.ranking_q1_calls;
        std::array<double, 4> rank_ms{};
        const RankingTables t = build_ranking_tables(pc, orac_a, rank_ms);
        const uint64_t s2_q1 = g_calls.ranking_q1_calls - q1_before;
        g4_require(s2_q1 == pc.candidate_count,
                   "S2 spends exactly one q1 call per enumerated candidate");

        // Every family's cached selection must equal the direct selector.
        for (int f = 0; f < 4; ++f) {
            const std::vector<LeafId>& mask = family_mask(Family(f));
            const LeafSelection cached =
                select_from_table(pc, Surface::S2, mask, t, 3);
            const LeafSelection direct = select_proxy(pc, Surface::S2, mask);
            g4_require(cached == direct, "cached S2 selection == direct S2 selection");
            // S0/S1 cached vs direct as well (they spend no backend call).
            g4_require(select_from_table(pc, Surface::S0, mask, t, 1) ==
                           select_proxy(pc, Surface::S0, mask),
                       "cached S0 selection == direct S0 selection");
            g4_require(select_from_table(pc, Surface::S1, mask, t, 2) ==
                           select_proxy(pc, Surface::S1, mask),
                       "cached S1 selection == direct S1 selection");
        }
        // Re-selecting from the cache must not spend a second q1 call.
        const uint64_t q1_mid = g_calls.ranking_q1_calls;
        for (int f = 0; f < 4; ++f)
            (void)select_from_table(pc, Surface::S2, family_mask(Family(f)), t, 3);
        g4_require(g_calls.ranking_q1_calls == q1_mid,
                   "S2 mask selection reuses cached scores (no extra q1)");
    }
}

static void g4_selftest() {
    // Frozen G3's own adversarial suite runs first (parser, leaves, carrier,
    // malformed input) — inherited unchanged.
    selftest();
    sha256_selftest();
    s1_formula_selftest();
    s1_equal_h0_different_s0_selftest();
    s1_full_tuple_raw_tie_selftest();
    determinism_selftest();
    production_oracle_identity_selftest();
    oracle_leakage_selftest();
    cached_score_reuse_selftest();
    carrier_identity_selftest();
    malformed_fallback_selftest();
    probe_order_key_selftest();
    std::cout << "PASS grotli_g4_planner selftest\n";
}

// ---------------------------------------------------------------------------
// Synthetic identity harness (local; tiny synthetic fixtures only)
// ---------------------------------------------------------------------------

// Builds a handful of tiny in-memory fixtures, runs the full measure path
// through a temp file, and prints a compact identity summary. Used by local
// verification; it never touches D1-D4 or V1.
static int g4_selftest_synthetic() {
    const std::vector<std::pair<std::string, std::string>> fixtures = {
        {"lf", "{\"a\":1,\"b\":\"x\",\"n\":null}\n{\"a\":2,\"b\":\"y\",\"n\":null}\n"
                "{\"a\":3,\"b\":\"x\",\"n\":null}\n{\"a\":4,\"b\":\"y\",\"n\":null}\n"},
        {"crlf", "{\"a\":100,\"b\":\"p\"}\r\n{\"a\":101,\"b\":\"q\"}\r\n{\"a\":102,\"b\":\"p\"}\r\n"},
        {"int", "{\"i\":1,\"j\":[10,12,15],\"k\":\"aa\"}\n{\"i\":2,\"j\":[20,22,25],\"k\":\"aa\"}\n"
                "{\"i\":3,\"j\":[30,32,35],\"k\":\"bb\"}\n"},
        {"mixed", "{}\n[]\ngarbage\n{\"z\":7}\n"},
    };
    int failures = 0;
    for (const auto& kv : fixtures) {
        const Bytes src = bytes(kv.second);
        RegionAnalysis prod_a = analyze_regions(src);
        const ProdCandidates pc = build_candidates_no_score(prod_a);
        RegionAnalysis orac_a = analyze_regions(src);
        build_structured_candidates(orac_a);
        assert_production_matches_oracle(prod_a, pc, orac_a);

        std::array<bool, 4> fam_ok{};
        const std::array<const std::vector<LeafId>*, 4> masks = {
            &kMaskRaw, &kMaskDict, &kMaskInt, &kMaskMixed};
        for (int f = 0; f < 4; ++f) {
            const auto g3_sel = local_selection(orac_a, *masks[f]);
            Selection g3_frozen; g3_frozen.structured_shapes = g3_sel;
            CarrierStats g3_stats;
            const Bytes g3c = make_region_carrier(src, orac_a, g3_frozen, g3_stats);
            const LeafSelection o11 = select_o11(pc, orac_a, *masks[f]);
            Selection o11_frozen; o11_frozen.structured_shapes = o11;
            CarrierStats o11_stats;
            const Bytes o11c = make_region_carrier(src, orac_a, o11_frozen, o11_stats);
            fam_ok[f] = (o11c == g3c) && (decode_region_carrier(o11c) == src);
            if (!fam_ok[f]) ++failures;
        }
        std::cout << "SYNTH " << kv.first
                  << " RAW=" << (fam_ok[0] ? "IDENT" : "MISMATCH")
                  << " DICT=" << (fam_ok[1] ? "IDENT" : "MISMATCH")
                  << " INT=" << (fam_ok[2] ? "IDENT" : "MISMATCH")
                  << " MIXED=" << (fam_ok[3] ? "IDENT" : "MISMATCH")
                  << " shapes=" << pc.shapes.size()
                  << " candidates=" << pc.candidate_count << "\n";
    }
    if (failures) {
        std::cerr << "SYNTHETIC IDENTITY FAILURES: " << failures << "\n";
        return 1;
    }
    std::cout << "PASS grotli_g4_planner synthetic O11==frozen-G3 identity (all four F)\n";
    return 0;
}

// ---------------------------------------------------------------------------
// G2 result parsing compatibility (frozen G3 discipline lives in the TU)
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "selftest") {
            g4_selftest();
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "selftest_synthetic") {
            return g4_selftest_synthetic();
        }
        if (argc >= 3 && (std::string(argv[1]) == "measure" ||
                          std::string(argv[1]) == "probe")) {
            const bool is_probe = (std::string(argv[1]) == "probe");
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
            return is_probe ? g4_probe(path, g2) : g4_measure(path, g2);
        }
        std::cerr << "usage: grotli_g4_planner selftest\n"
                     "       grotli_g4_planner selftest_synthetic\n"
                     "       grotli_g4_planner measure INPUT [--g2-result ROW.json]\n"
                     "       grotli_g4_planner probe   INPUT [--g2-result ROW.json]\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
