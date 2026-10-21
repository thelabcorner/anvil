// I20 C5 prototype — corrected COPY of ANVIL's canonical-Huffman decoder.
//
// PROVENANCE
//   Copied from `src/anvil.cpp` @ commit d05fceb (see prototypes/i20-postcoder-swap/README.md
//   for the full provenance note).  Original region: src/anvil.cpp lines 1352-1445
//   (`struct HuffModel`, `build_huff_model`, `huffman_decode`).
//
// THE DEFECT THIS COPY FIXES
//   In src/anvil.cpp the canonical decode table stores the first code of each
//   length in `std::array<uint16_t,256> first_code`.  The first canonical code of
//   length l can be as large as 2^l-1, so for l >= 17 the uint16_t truncates it.
//   The slow decode path (taken when max_len > 12) then compares the incoming
//   code against a WRONG bound and either throws "invalid huffman code" or
//   decodes the wrong symbol.  This is the documented root cause of the BWT
//   postcoder-0 "known encoder/decoder mismatch" (src/anvil.cpp, the
//   `try_post` comment near line 4296: "post0 fails in the stream-suite
//   Huffman path ('invalid huffman code')").
//
//   FIX: `first_code` is `uint32_t`.  Everything else is byte-identical to the
//   repository's code.
//
// WIRE COMPATIBILITY
//   The wire is UNCHANGED.  Only the 256 canonical code lengths are transmitted
//   (`huffman_stream_bytes`), and the model is rebuilt from them at decode
//   time.  A payload produced by the repository's encoder is decoded by this
//   copy and vice versa.  (The repository's own `decode_stream` mode 4 path is
//   still the buggy one; this copy is used ONLY inside the I20 prototype.)
//
// SECOND EXTENSION (prototype-local, NOT a repository change)
//   The repository's `decode_stream` mode 4 rejects any code length > 24
//   ("bad huffman length"), because its Kraft check is `1ull << (24 - len)`.
//   This copy accepts lengths up to 32 so that a heavily skewed token stream
//   (an MTF rank stream with a long Huffman tail) is still measurable as a
//   candidate.  Any candidate whose Huffman code length exceeds 24 would NOT
//   be decodable by the shipped codec; the prototype reports the observed
//   maximum code length so the integration constraint stays explicit.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace i20 {

struct HuffModelFixed {
    std::array<uint8_t, 256> len{};
    std::array<uint64_t, 256> first_code{};  // <-- uint64_t, not the repo's uint16_t
    std::array<uint16_t, 256> first_sym{};
    std::array<uint16_t, 256> n_codes{};
    std::array<uint8_t, 256> order{};
    uint8_t max_len = 0, n_ord = 0;
    std::array<uint16_t, 4096> tbl{};  // 12-bit decode table: (sym<<4)|len; 0xFFFF = long-code marker
};

inline HuffModelFixed build_huff_model_fixed(const std::array<uint8_t, 256>& len) {
    HuffModelFixed h; h.len = len;
    std::array<uint8_t, 256> syms{};
    uint32_t cnt = 0;
    for (int s = 0; s < 256; ++s) if (len[s]) syms[cnt++] = static_cast<uint8_t>(s);
    std::sort(syms.begin(), syms.begin() + cnt, [&](uint8_t a, uint8_t b) {
        return len[a] != len[b] ? len[a] < len[b] : a < b;
    });
    h.n_ord = static_cast<uint8_t>(cnt);
    for (uint32_t i = 0; i < cnt; ++i) h.order[i] = syms[i];
    std::array<uint64_t, 256> code{};
    uint64_t c = 0, clen = 0;
    for (uint32_t i = 0; i < cnt; ++i) {
        while (clen < len[syms[i]]) { c <<= 1; ++clen; }
        code[syms[i]] = c++;
        if (len[syms[i]] > h.max_len) h.max_len = len[syms[i]];
    }
    uint32_t cur = 0;
    for (int l = 1; l <= 32; ++l) {
        while (cur < cnt && len[syms[cur]] < l) ++cur;
        if (cur < cnt && len[syms[cur]] == l) {
            h.first_code[l] = code[syms[cur]];
            h.first_sym[l] = static_cast<uint16_t>(cur);
            uint32_t k = cur;
            while (k < cnt && len[syms[k]] == l) ++k;
            h.n_codes[l] = static_cast<uint16_t>(k - cur);
        }
    }
    h.tbl.fill(0xFFFFu);
    if (h.max_len <= 12) {
        for (uint32_t i = 0; i < cnt; ++i) {
            uint8_t s = syms[i]; uint8_t l = len[s];
            uint32_t base = static_cast<uint32_t>(code[s]) << (12 - l);
            uint16_t entry = static_cast<uint16_t>((s << 4) | l);
            for (uint32_t k = 0; k < (1u << (12 - l)); ++k) h.tbl[base + k] = entry;
        }
    }
    return h;
}

inline std::vector<uint8_t> huffman_decode_fixed(const uint8_t* p, size_t n, size_t out_n, const HuffModelFixed& h) {
    std::vector<uint8_t> out(out_n);
    const uint8_t* e = p + n;
    uint64_t acc = 0; int have = 0;
    auto refill = [&](int need) { while (have < need && p < e) { acc = (acc << 8) | *p++; have += 8; } };
    if (h.max_len <= 12) {
        for (size_t i = 0; i < out_n; ++i) {
            refill(12);
            int win = have; if (win > 12) win = 12;
            uint32_t code = static_cast<uint32_t>((acc >> (have - win)) & ((1u << win) - 1));
            uint32_t idx = code << (12 - win);
            uint16_t entry = h.tbl[idx];
            if (entry == 0xFFFFu) throw std::runtime_error("invalid huffman code");
            int used = entry & 0xF;
            if (used > win) throw std::runtime_error("invalid huffman code");
            out[i] = static_cast<uint8_t>(entry >> 4);
            have -= used;
        }
        return out;
    }
    for (size_t i = 0; i < out_n; ++i) {
        refill(1);
        uint64_t code = 0; uint8_t sym = 0; bool found = false;
        for (int l = 1; l <= 32; ++l) {
            if (have < 1) throw std::runtime_error("truncated huffman bits");
            code = (code << 1) | static_cast<uint64_t>((acc >> (have - 1)) & 1);
            --have; refill(1);
            if (h.n_codes[l] && code >= h.first_code[l] && code < h.first_code[l] + h.n_codes[l]) {
                sym = h.order[h.first_sym[l] + (code - h.first_code[l])];
                found = true; break;
            }
        }
        if (!found) throw std::runtime_error("invalid huffman code");
        out[i] = sym;
    }
    return out;
}

}  // namespace i20
