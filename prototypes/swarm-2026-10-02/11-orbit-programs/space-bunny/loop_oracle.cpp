#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr uint32_t kNone = 0xFFFFFFFFu;
constexpr int kHashBits = 20;
constexpr size_t kHashSize = size_t(1) << kHashBits;
constexpr int kMru = 4;
constexpr uint32_t kMinMatch = 4;
constexpr uint64_t kMinAdmissibleRun = 4;

struct Token {
    uint32_t len;
    uint32_t dist;
};

struct RunHit {
    uint64_t ll_dd = 0;
    uint64_t ll_dn = 0;
    uint64_t ln_dd = 0;
    uint64_t ln_dn = 0;
    uint64_t degen = 0;
};

struct Stats {
    uint64_t input_bytes = 0;
    uint64_t tokens = 0;
    uint64_t literal_bytes = 0;
    uint64_t match_bytes = 0;
    uint64_t runs_ge2 = 0;
    uint64_t runs_adm = 0;
    uint64_t match_bytes_adm = 0;
    uint64_t symbols_eliminated = 0;
    uint64_t expanded_symbols_adm = 0;
    uint64_t delta_b1 = 0;
    uint64_t delta_b1_loop = 0;
    uint64_t delta_b1_expanded = 0;
};

uint32_t load32(const unsigned char* p) {
    uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}

uint32_t hash4(const unsigned char* p) {
    return (load32(p) * 2654435761u) >> (32 - kHashBits);
}

size_t uvarint_len(uint64_t v) {
    size_t n = 1;
    while (v >= 0x80u) {
        v >>= 7;
        ++n;
    }
    return n;
}

uint64_t zigzag(int64_t v) {
    return v >= 0 ? uint64_t(v) * 2u : uint64_t((-v) * 2 - 1);
}

std::vector<double> g_hop = {0.3, 0.5, 1.0, 1.5};
uint32_t g_runcap = 64;

bool parse_hop(const char* s) {
    std::string cur;
    std::vector<double> out;
    for (const char* p = s;; ++p) {
        if (*p == ',' || *p == '\0') {
            if (cur.empty()) return false;
            out.push_back(std::strtod(cur.c_str(), nullptr));
            cur.clear();
            if (*p == '\0') break;
        } else {
            cur.push_back(*p);
        }
    }
    if (out.empty()) return false;
    g_hop.swap(out);
    return true;
}

bool read_file(const char* path, std::vector<unsigned char>& out) {
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    if (std::fseek(f, 0, SEEK_END) != 0) {
        std::fclose(f);
        return false;
    }
    long sz = std::ftell(f);
    if (sz < 0) {
        std::fclose(f);
        return false;
    }
    std::rewind(f);
    out.resize(size_t(sz));
    size_t got = sz > 0 ? std::fread(out.data(), 1, size_t(sz), f) : 0;
    std::fclose(f);
    return got == out.size();
}

std::vector<Token> greedy_parse(const std::vector<unsigned char>& buf,
                               std::vector<uint32_t>& tab) {
    tab.assign(kHashSize * size_t(kMru), kNone);
    std::vector<Token> toks;
    const size_t n = buf.size();
    toks.reserve(n / 8 + 16);
    size_t pos = 0;
    while (pos < n) {
        uint32_t best_len = 0;
        uint32_t best_dist = 0;
        uint32_t* slot = nullptr;
        if (pos + kMinMatch <= n) {
            slot = &tab[size_t(hash4(&buf[pos])) * size_t(kMru)];
            for (int i = 0; i < kMru; ++i) {
                uint32_t q = slot[i];
                if (q == kNone || size_t(q) >= pos) continue;
                if (load32(&buf[q]) != load32(&buf[pos])) continue;
                uint32_t l = kMinMatch;
                while (pos + l < n && buf[pos + l] == buf[q + l]) ++l;
                if (l > best_len) {
                    best_len = l;
                    best_dist = uint32_t(pos - q);
                }
            }
        }
        if (best_len >= kMinMatch) {
            toks.push_back(Token{best_len, best_dist});
        } else {
            toks.push_back(Token{1u, 0u});
        }
        if (slot) {
            for (int i = kMru - 1; i > 0; --i) slot[i] = slot[i - 1];
            slot[0] = uint32_t(pos);
        }
        pos += (best_len >= kMinMatch) ? size_t(best_len) : size_t(1);
    }
    return toks;
}

void analyze(const char* name, const std::vector<unsigned char>& buf,
             const std::vector<Token>& toks, Stats& st, RunHit& hit,
             std::vector<uint64_t>& hist) {
    st.input_bytes = buf.size();
    st.tokens = toks.size();
    for (const Token& t : toks) {
        if (t.dist == 0)
            st.literal_bytes += t.len;
        else
            st.match_bytes += t.len;
    }
    size_t i = 0;
    while (i < toks.size()) {
        if (toks[i].dist == 0) {
            ++i;
            continue;
        }
        size_t e = i;
        while (e < toks.size() && toks[e].dist != 0) ++e;
        uint64_t k = uint64_t(e - i);
        hist[k <= g_runcap ? size_t(k) : size_t(g_runcap) + 1] += 1;
        if (k < 2) {
            i = e;
            continue;
        }
        ++st.runs_ge2;
        int64_t dl = int64_t(toks[i + 1].len) - int64_t(toks[i].len);
        int64_t dd = int64_t(toks[i + 1].dist) - int64_t(toks[i].dist);
        bool const_l = true;
        bool const_d = true;
        for (size_t j = i + 1; j + 1 < e; ++j) {
            if (int64_t(toks[j + 1].len) - int64_t(toks[j].len) != dl) const_l = false;
            if (int64_t(toks[j + 1].dist) - int64_t(toks[j].dist) != dd) const_d = false;
            if (!const_l && !const_d) break;
        }
        if (k < kMinAdmissibleRun) {
            ++hit.degen;
        } else if (const_l && const_d) {
            ++hit.ll_dd;
        } else if (const_l) {
            ++hit.ll_dn;
        } else if (const_d) {
            ++hit.ln_dd;
        } else {
            ++hit.ln_dn;
        }
        if (const_l && const_d) {
            uint64_t bytes = 0;
            uint64_t b1_expanded = 0;
            for (size_t j = i; j < e; ++j) {
                bytes += toks[j].len;
                int64_t d = (j == i) ? 0
                                     : int64_t(toks[j].dist) - int64_t(toks[j - 1].dist);
                b1_expanded += 1 + uvarint_len(uint64_t(toks[j].len) - kMinMatch) +
                               uvarint_len(zigzag(d));
            }
            uint64_t b1_loop = 1 + uvarint_len(k);
            st.match_bytes_adm += bytes;
            ++st.runs_adm;
            st.symbols_eliminated += (k - 1);
            st.expanded_symbols_adm += k;
            st.delta_b1_expanded += b1_expanded;
            st.delta_b1_loop += b1_loop;
        }
        i = e;
    }
    st.delta_b1 = st.delta_b1_loop - st.delta_b1_expanded;
    double cov_match =
        st.match_bytes ? 100.0 * double(st.match_bytes_adm) / double(st.match_bytes) : 0.0;
    double cov_input =
        st.input_bytes ? 100.0 * double(st.match_bytes_adm) / double(st.input_bytes) : 0.0;
    std::printf("RUNSUM,%s,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%.3f,%.3f,%llu\n", name,
                (unsigned long long)st.input_bytes, (unsigned long long)st.tokens,
                (unsigned long long)st.literal_bytes, (unsigned long long)st.match_bytes,
                (unsigned long long)st.runs_ge2, (unsigned long long)st.runs_adm,
                (unsigned long long)st.match_bytes_adm, cov_match, cov_input,
                (unsigned long long)st.symbols_eliminated);
    std::printf("HITM,%s,%llu,%llu,%llu,%llu,%llu\n", name, (unsigned long long)hit.ll_dd,
                (unsigned long long)hit.ll_dn, (unsigned long long)hit.ln_dd,
                (unsigned long long)hit.ln_dn, (unsigned long long)hit.degen);
    for (size_t k = 1; k <= size_t(g_runcap) + 1; ++k) {
        if (hist[k] == 0) continue;
        std::printf("HIST,%s,%s%llu,%llu\n", name, k == size_t(g_runcap) + 1 ? "cap+" : "",
                    (unsigned long long)(k == size_t(g_runcap) + 1 ? 0 : k),
                    (unsigned long long)hist[k]);
    }
    double d1_in =
        st.input_bytes ? 100.0 * double(st.delta_b1) / double(st.input_bytes) : 0.0;
    std::printf("DELTA,%s,B1,NA,%lld,%.4f,NA\n", name, (long long)st.delta_b1, d1_in);
    for (double hop : g_hop) {
        long long expanded = (long long)(double(st.expanded_symbols_adm) * hop);
        long long loop =
            (long long)(hop + double(uvarint_len(st.expanded_symbols_adm)));
        long long delta = loop - expanded;
        double pct_in = st.input_bytes ? 100.0 * double(delta) / double(st.input_bytes) : 0.0;
        double pct_exp = st.expanded_symbols_adm
                             ? 100.0 * double(delta) / double(expanded)
                             : 0.0;
        std::printf("DELTA,%s,B2,%.4f,%lld,%.4f,%.4f\n", name, hop, delta, pct_in, pct_exp);
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::vector<const char*> files;
    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], "--hop=", 6) == 0) {
            if (!parse_hop(argv[i] + 6)) {
                std::fprintf(stderr, "bad --hop list\n");
                return 2;
            }
        } else if (std::strncmp(argv[i], "--runcap=", 9) == 0) {
            g_runcap = uint32_t(std::strtoul(argv[i] + 9, nullptr, 10));
            if (g_runcap < 2) g_runcap = 2;
        } else {
            files.push_back(argv[i]);
        }
    }
    if (files.empty()) {
        std::fprintf(stderr, "usage: loop_oracle <file>... [--hop=a,b,c] [--runcap=N]\n");
        return 2;
    }
    std::vector<unsigned char> buf;
    std::vector<uint32_t> tab;
    for (const char* path : files) {
        if (!read_file(path, buf)) {
            std::fprintf(stderr, "cannot read %s\n", path);
            return 2;
        }
        std::vector<Token> toks = greedy_parse(buf, tab);
        Stats st;
        RunHit hit;
        std::vector<uint64_t> hist(size_t(g_runcap) + 2, 0);
        analyze(path, buf, toks, st, hit, hist);
    }
    return 0;
}
