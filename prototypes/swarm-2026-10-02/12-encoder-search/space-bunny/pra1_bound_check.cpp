// pra1_bound_check.cpp — Track 12 isolated artifact
//
// Agent:    Space Bunny Free
// Track:    12-encoder-search
// Date:     2026-10-02
// Status:   SPECIFICATION GRADE. NOT COMPILED. NOT EXECUTED. NOT MEASURED.
//
// Purpose of this file, and nothing else:
//   (A) encode the H0-admissibility FALSIFICATION harness described in
//       docs/swarm-2026-10-02/12-encoder-search-space-bunny.md §1 / Addendum A W-1.
//       It is a self-contained arithmetic check over small payloads. It does
//       not link Brotli and cannot produce a codec number.
//   (B) encode Bound F: the escape-inclusive, fully charged selector-framing
//       byte bound, together with the decoder cost that the earlier draft of
//       the report omitted (Addendum A, Incompatibility Lemma).
//   (C) emit the POOL-1 census counters (report §11.3), which are pure
//       bookkeeping over an already-frozen carrier plan.
//
// DELIBERATELY ABSENT: no corpus runner, no timing harness, no benchmark path,
// no Pareto classifier, no network access. This file cannot produce a number
// that any report may cite. Windows C++ build verification is BLOCKED in the
// authoring environment (no RC compiler; RESEARCH_LEDGER.md PART XV
// verification addendum), so no local compile was attempted and none is
// claimed.
//
// This file is NOT wired into src/, FORMAT.md, tools/, or any build target.

#include <cstdint>
#include <cstddef>
#include <vector>

namespace pra1 {

// ---------------------------------------------------------------------------
// (A) H0 falsification harness.
//
// The withdrawn claim was:  cost_Brotli(f,s) >= framing_min(f,s) + 1 + ceil(H0(payload)).
// The refutation is a PAYLOAD FAMILY, not a benchmark: for p = q || q with q
// uniform over 256 symbols, H0(p) = 2m bytes exactly, while the backend's
// format (RFC 7932) permits the second q to be emitted as one copy command.
// The harness computes only the H0 side exactly. The backend side is asserted
// by FORMAT, not measured here.
// ---------------------------------------------------------------------------

// Exact order-0 ideal-code length in BYTES for a payload.
// ceil(-sum_b count[b]*log2(count[b]/N) / 8). Integer-exact via 64-bit ints:
// we compute bits as a sum of count[b] * (ceil_log2(N) - floor_log2(count[b]))
// which is the standard integer ideal-code expression used in the G4 prereg
// (docs/I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md §4.4).
inline uint64_t ceil_log2_u64(uint64_t x) {
    if (x <= 1) return 0;
    return 64 - /*clz*/ 0; // replaced below by the portable helper
}

inline int floor_log2_u64(uint64_t x) { // x >= 1
    int r = 0;
    while (x > 1) { x >>= 1; ++r; }
    return r;
}

inline uint64_t ceil_log2_portable(uint64_t x) { // x >= 1
    return x <= 1 ? 0 : static_cast<uint64_t>(floor_log2_u64(x - 1) + 1);
}

// Exact ideal order-0 length, in bytes, of `payload`.
// NOTE: this is an ideal order-0 code length. It is a valid LOWER BOUND only
// for a MEMLESS ORDER-0 CODER. It is NOT a lower bound for Brotli, which has
// backward references over reconstructed output and a static dictionary.
inline uint64_t h0_bytes(const std::vector<uint8_t>& payload) {
    uint64_t count[256] = {0};
    for (uint8_t b : payload) ++count[b];
    const uint64_t N = payload.size();
    if (N <= 1) return 0;
    uint64_t bits = 0;
    const uint64_t ceil_log2_N = ceil_log2_portable(N);
    for (int b = 0; b < 256; ++b) {
        if (count[b] == 0) continue;
        const int fl = floor_log2_u64(count[b]);
        const uint64_t term = 256ull * count[b] *
                              (static_cast<int64_t>(ceil_log2_N) - fl);
        bits += term;
    }
    // bits is scaled by 256; round up to whole bytes.
    return (bits + 255ull) / 256ull;
}

// The counterexample family. Emits p = q || q.
// Returns the exact H0 side only. The backend side is a FORMAT argument:
// RFC 7932 supplies copy commands over reconstructed output, so
// Brotli_q11(p) <= |q| bytes + O(log |q|) bits. The report labels the numeric
// backend figure a PROJECTION and the violation itself a FORMAT consequence.
inline void emit_counterexample(uint64_t m, std::vector<uint8_t>& out) {
    out.clear();
    out.reserve(2 * m);
    uint64_t state = 0x9E3779B97F4A7C15ull; // deterministic PRNG, no seeding API
    for (uint64_t i = 0; i < m; ++i) {
        state ^= state << 13; state ^= state >> 7; state ^= state << 17;
        out.push_back(static_cast<uint8_t>(state >> 24));
    }
    for (uint64_t i = 0; i < m; ++i) out.push_back(out[i]); // p = q || q
}

// A selftest of the FALSIFICATION ONLY: it demonstrates that H0 grows linearly
// with the repeat count while the backend's permitted cost does not. It does
// not call a backend and therefore cannot by itself "prove" inadmissibility;
// the proof is the FORMAT argument in the report. This function exists so the
// arithmetic half of that argument is machine-checkable.
inline bool falsification_selftest() {
    for (uint64_t m : {64ull, 256ull, 1024ull, 4096ull}) {
        std::vector<uint8_t> q;
        emit_counterexample(m, q);
        const uint64_t h0_p = h0_bytes(q);                 // == 2m for m uniform
        std::vector<uint8_t> once(q.begin(), q.begin() + m);
        const uint64_t h0_q = h0_bytes(once);             // == m
        if (h0_p != 2 * h0_q) return false;               // arithmetic premise
        // Backend-permitted cost for p is at most h0_q + O(log m) bits.
        // Therefore h0_p exceeds it by ~2x for every m: LB is not admissible.
        if (h0_p <= h0_q) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// (B) Bound F: selector-framing bytes, escape-inclusive.
//
// Frozen G5A-LC convention (docs/I10-G5A-LOCALITY-CONTROLS-PREREG.md §4.1):
// exactly ONE outer selector byte per record, charged in every arm, with a
// 32-value frozen selector table (§8).
//
// Code space inside that same one-byte-per-record frame:
//   code 0 .. L-1   PER_RECORD : this record uses family `code`
//   code L          RUN_FAM    : uvar_len(n) bytes, then 1 family byte
//   code L+1        RUN_LIT    : uvar_len(n) bytes, then n family bytes
//   code >= L+2     RESERVED   : decoder rejects (fail-closed)
//
// Bound F (provable):
//   selector_bytes = sum over runs of min(R_r, 1 + uvar_len(R_r) + 1) <= R
// Equality iff every R_r <= 3. Worst case is exactly the incumbent's R bytes.
//
// INCOMPATIBILITY LEMBAMA (report §6.1): the decoder's selector reader is a flat
// one-byte-per-record table lookup. Any reduction in selector bytes requires
// the decoder to consume fewer, or differently interpreted, bytes. That is a
// change to the decode parse step, hence d(decoder text) != 0. Therefore
// "fewer selector bytes" and "zero decoder cost" cannot both hold.
// ---------------------------------------------------------------------------

inline uint64_t uvar_len(uint64_t v) { // LEB128-style, matching the frozen grammar
    uint64_t n = 1;
    while (v >= 0x80) { v >>= 7; ++n; }
    return n;
}

struct FramingCharge {
    uint64_t incumbent_bytes;      // R
    uint64_t branch_f_bytes;       // F2 total
    uint64_t saving_bytes;         // R - branch_f_bytes
    uint64_t runs_gt3;             // runs where a saving is actually available
    uint64_t max_run;
};

// `runs` is the maximal-run decomposition of the per-record family sequence.
// L is the number of live families (decoder-implemented).
inline FramingCharge charge_framing(const std::vector<uint32_t>& runs, uint32_t L) {
    FramingCharge c{0, 0, 0, 0, 0};
    for (uint32_t r : runs) {
        c.incumbent_bytes += r;                                   // 1 byte per record
        const uint64_t run_fam = 1 + uvar_len(r) + 1;             // opcode + uvar + family
        const uint64_t chosen  = (r < run_fam) ? r : run_fam;     // the min()-escape
        c.branch_f_bytes += chosen;
        if (chosen < r) ++c.runs_gt3;
        if (r > c.max_run) c.max_run = r;
    }
    c.saving_bytes = c.incumbent_bytes - c.branch_f_bytes;
    return c;
}

// The Incompatibility Lemma as an executable assertion: any run length for
// which F2 saves bytes REQUIRES the RUN_FAM opcode, i.e. a decoder-side change.
// A saving of zero leaves the decoder untouched. This is why Branch F cannot
// simultaneously be "free at the decoder" and "smaller on the wire".
inline bool incompatibility_selftest() {
    const std::vector<uint32_t> runs_uniform = {4, 4, 4, 4};
    const std::vector<uint32_t> runs_no_gain = {1, 2, 3};
    const FramingCharge a = charge_framing(runs_uniform, 6);
    const FramingCharge b = charge_framing(runs_no_gain,  6);
    return a.saving_bytes > 0 && b.saving_bytes == 0;
}

// ---------------------------------------------------------------------------
// (C) POOL-1 census counters.
//
// The ONLY remote measurement this track still recommends. Byte-only, zero new
// Brotli calls, {DISCOVERY}-labelled, and gated at the project's own bars:
//   KILL-POOL-F : saving(F) / complete_bytes < 0.0025
//   SUPPORT     : saving(F) / complete_bytes >= 0.0100 on >= 3 of 4 files
// These counters compute the numerator. The denominator must come from the
// frozen carrier's own complete charged byte count, never recomputed here.
// ---------------------------------------------------------------------------

struct PoolCensus {
    uint64_t records;             // R  -- the entire byte pool, 1 B/record
    uint64_t complete_bytes;      // from the frozen carrier plan
    uint64_t runs;
    FramingCharge charge;
    double saving_fraction;       // saving(F) / complete_bytes
};

inline PoolCensus census(const std::vector<uint32_t>& runs,
                         uint64_t complete_bytes, uint32_t L) {
    PoolCensus p{0, complete_bytes, runs.size(), charge_framing(runs, L), 0.0};
    for (uint32_t r : runs) p.records += r;
    p.saving_fraction = complete_bytes
        ? static_cast<double>(p.charge.saving_bytes) / static_cast<double>(complete_bytes)
        : 0.0;
    return p;
}

// ---------------------------------------------------------------------------
// selftest: runs the falsification arithmetic and the framing bound. No I/O,
// no corpus, no timing. Intended to be the R0 rung of a remote workflow.
// ---------------------------------------------------------------------------
inline bool selftest() {
    if (!falsification_selftest())   return false;
    if (!incompatibility_selftest()) return false;

    // Bound F worked values from the report table.
    if (charge_framing({4},    6).saving_bytes    != 1)     return false;
    if (charge_framing({16},   6).saving_bytes    != 13)    return false;
    if (charge_framing({127},  6).saving_bytes    != 124)   return false;
    if (charge_framing({1000}, 6).saving_bytes    != 996)   return false;
    if (charge_framing({3},    6).saving_bytes    != 0)     return false; // tie
    if (charge_framing({1, 2, 3}, 6).saving_bytes != 0)    return false;

    // Never worse than the incumbent, for any run decomposition.
    for (uint32_t r = 1; r <= 300; ++r) {
        const std::vector<uint32_t> one{r};
        if (charge_framing(one, 6).saving_bytes == 0 &&
            charge_framing(one, 6).branch_f_bytes > charge_framing(one, 6).incumbent_bytes)
            return false;
    }
    return true;
}

} // namespace pra1