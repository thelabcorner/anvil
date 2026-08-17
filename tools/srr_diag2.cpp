// SRR alignment diagnostic #2: analyze token structure vs record boundaries.
// For each type-2 token, report whether it starts near a record boundary,
// its span-likeness (len vs dist), and per-token mask repetition.
#define ANVIL_NO_MAIN
#include "../src/anvil.cpp"
#include <cstdio>
#include <map>
#include <algorithm>

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: srr_diag2 <file> <record_len>\n"); return 2; }
    auto d = anvil::read_file(argv[1]);
    uint32_t rec = argc >= 3 ? (uint32_t)atoi(argv[2]) : 235;
    auto toks = anvil::parse_sparse(d, 128, 1u<<20, 12, false, true, false);
    uint64_t t2 = 0, at_boundary = 0, span_like = 0, boundary_span = 0;
    uint64_t near_tok = 0;
    // token boundary alignment: token start mod ~record len (allow drift +-2 via any multiple window)
    for (auto& t : toks) {
        if (t.type != 2) continue;
        ++t2;
        uint32_t rem = t.pos % rec;
        bool b = rem <= 3 || rem >= rec - 3;
        if (b) ++at_boundary;
        bool sl = t.len >= t.dist && t.len <= t.dist + 8;
        if (sl) ++span_like;
        if (b && sl) ++boundary_span;
        if (t.dist <= 16384) ++near_tok;
    }
    printf("t2=%llu at_record_boundary(+/-3)=%llu (%.1f%%) span_like(len~dist)=%llu (%.1f%%) boundary_span=%llu (%.1f%%) near=%llu (%.1f%%)\n",
        (unsigned long long)t2, (unsigned long long)at_boundary, 100.0*at_boundary/t2,
        (unsigned long long)span_like, 100.0*span_like/t2,
        (unsigned long long)boundary_span, 100.0*boundary_span/t2,
        (unsigned long long)near_tok, 100.0*near_tok/t2);
    return 0;
}
