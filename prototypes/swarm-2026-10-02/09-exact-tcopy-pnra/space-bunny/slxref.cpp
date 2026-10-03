#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace slx {

enum class Arm : uint8_t {
  ExactOnly = 0,
  TxDeltaDerivedTopo = 1,
  TxDeltaTxTopo = 2,
  DerivedDeltaTxTopo = 3,
  DerivedDeltaDerivedTopo = 4,
};

struct Site {
  uint32_t off;
  int32_t sigma;
};

struct Token {
  uint8_t kind;  // 0 literal run, 1 exact, 2 xref
  uint32_t dist; // exact/xref
  uint32_t len;  // exact/xref
  uint32_t lit_off;
  uint32_t lit_len;
  int32_t tx_delta;           // arms 1 and 2 only
  std::vector<Site> tx_topo;  // arms 2 and 3 only
};

static const uint32_t kMaxLen = 65536;

static bool overlaps(uint32_t off, uint32_t dist) {
  return off + 4u > dist;
}

static bool triple_is_abs(const uint8_t* s, uint32_t L) {
  if (L % 12u != 0u) return false;
  for (uint32_t i = 0; i + 12u <= L; i += 12u) {
    uint32_t begin = (uint32_t)s[i] | ((uint32_t)s[i + 1] << 8) | ((uint32_t)s[i + 2] << 16) |
                     ((uint32_t)s[i + 3] << 24);
    uint32_t end = (uint32_t)s[i + 4] | ((uint32_t)s[i + 5] << 8) | ((uint32_t)s[i + 6] << 16) |
                   ((uint32_t)s[i + 7] << 24);
    uint32_t unwind = (uint32_t)s[i + 8] | ((uint32_t)s[i + 9] << 8) | ((uint32_t)s[i + 10] << 16) |
                      ((uint32_t)s[i + 11] << 24);
    if (!(begin < end)) return false;
    if ((unwind & 3u) != 0u) return false;
  }
  return true;
}

static std::vector<Site> classify_sites(const uint8_t* s, uint32_t L, uint32_t dist) {
  std::vector<Site> out;
  std::vector<uint8_t> claimed(L ? L : 1u, 0u);
  if (L == 0u) return out;
  if (triple_is_abs(s, L)) {
    for (uint32_t i = 0; i + 12u <= L; i += 12u) {
      for (uint32_t k = 0; k < 3u; ++k) {
        uint32_t off = i + 4u * k;
        if (off + 4u > L) break;
        if (overlaps(off, dist)) break;
        claimed[off] = 1u;
        claimed[off + 1u] = 1u;
        claimed[off + 2u] = 1u;
        claimed[off + 3u] = 1u;
        out.push_back(Site{off, +1});
      }
    }
  }
  for (uint32_t i = 0; i < L; ++i) {
    if (s[i] != 0xE8u && s[i] != 0xE9u) continue;
    uint32_t off = i + 1u;
    if (off + 4u > L) break;
    if (overlaps(off, dist)) continue;
    if (claimed[off]) continue;
    claimed[off] = 1u;
    claimed[off + 1u] = 1u;
    claimed[off + 2u] = 1u;
    claimed[off + 3u] = 1u;
    out.push_back(Site{off, -1});
  }
  return out;
}

static bool sites_are_disjoint(const std::vector<Site>& v) {
  for (size_t a = 0; a < v.size(); ++a)
    for (size_t b = a + 1u; b < v.size(); ++b)
      if (v[a].off < v[b].off + 4u && v[b].off < v[a].off + 4u) return false;
  return true;
}

static uint32_t load32(const uint8_t* p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void store32(uint8_t* p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xFFu);
  p[1] = (uint8_t)((v >> 8) & 0xFFu);
  p[2] = (uint8_t)((v >> 16) & 0xFFu);
  p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static bool xref_reconstructs(const std::vector<uint8_t>& src, uint32_t p, uint32_t d, uint32_t L,
                              const std::vector<Site>& sites) {
  if (p + L > src.size()) return false;
  if (d == 0u || d > p) return false;
  std::vector<uint8_t> out(src.begin() + (long)(p - d), src.begin() + (long)(p - d + L));
  for (const Site& st : sites) {
    if (st.off + 4u > L) return false;
    if (overlaps(st.off, d)) return false;
    uint32_t v = load32(&out[st.off]);
    store32(&out[st.off], v + (uint32_t)((int64_t)st.sigma * (int64_t)d));
  }
  for (uint32_t i = 0; i < L; ++i)
    if (out[i] != src[p + i]) return false;
  return true;
}

static void put_uvarint(std::vector<uint8_t>& w, uint64_t v) {
  while (v >= 0x80u) {
    w.push_back((uint8_t)(v | 0x80u));
    v >>= 7;
  }
  w.push_back((uint8_t)v);
}

static bool get_uvarint(const std::vector<uint8_t>& w, size_t& at, uint64_t& v) {
  v = 0u;
  uint32_t shift = 0u;
  while (at < w.size() && shift < 64u) {
    uint8_t b = w[at++];
    v |= (uint64_t)(b & 0x7Fu) << shift;
    if ((b & 0x80u) == 0u) return true;
    shift += 7u;
  }
  return false;
}

static void put_zigzag(std::vector<uint8_t>& w, int32_t v) {
  put_uvarint(w, (uint64_t)((uint32_t)((v << 1) ^ (v >> 31))));
}

static bool get_zigzag(const std::vector<uint8_t>& w, size_t& at, int32_t& v) {
  uint64_t u = 0u;
  if (!get_uvarint(w, at, u)) return false;
  if (u > 0xFFFFFFFFu) return false;
  v = (int32_t)((u >> 1) ^ (~(u & 1u) + 1u));
  return true;
}

static bool arm_tx_delta(Arm a) {
  return a == Arm::TxDeltaDerivedTopo || a == Arm::TxDeltaTxTopo;
}

static bool arm_tx_topo(Arm a) {
  return a == Arm::TxDeltaTxTopo || a == Arm::DerivedDeltaTxTopo;
}

static bool arm_derived_delta(Arm a) {
  return a == Arm::DerivedDeltaTxTopo || a == Arm::DerivedDeltaDerivedTopo;
}

static bool arm_derived_topo(Arm a) {
  return a == Arm::TxDeltaDerivedTopo || a == Arm::DerivedDeltaDerivedTopo;
}

static std::vector<uint8_t> serialize(const std::vector<uint8_t>& src,
                                      const std::vector<Token>& toks, Arm arm) {
  std::vector<uint8_t> w;
  put_uvarint(w, (uint64_t)arm);
  put_uvarint(w, (uint64_t)src.size());
  put_uvarint(w, (uint64_t)toks.size());
  for (const Token& t : toks) {
    w.push_back(t.kind);
    if (t.kind == 0u) {
      put_uvarint(w, t.lit_off);
      put_uvarint(w, t.lit_len);
      continue;
    }
    put_uvarint(w, t.dist);
    put_uvarint(w, t.len);
    if (arm_tx_delta(arm)) put_zigzag(w, t.tx_delta);
    if (arm_tx_topo(arm)) {
      put_uvarint(w, (uint64_t)t.tx_topo.size());
      for (const Site& s : t.tx_topo) {
        put_uvarint(w, s.off);
        put_zigzag(w, s.sigma);
      }
    }
  }
  return w;
}

static bool deserialize(const std::vector<uint8_t>& w, std::vector<uint8_t>& src,
                        std::vector<Token>& toks) {
  size_t at = 0u;
  uint64_t armv = 0u, n = 0u, nt = 0u;
  if (!get_uvarint(w, at, armv)) return false;
  if (armv > (uint64_t)Arm::DerivedDeltaDerivedTopo) return false;
  Arm arm = (Arm)armv;
  if (!get_uvarint(w, at, n)) return false;
  if (!get_uvarint(w, at, nt)) return false;
  if (n > 0xFFFFFFFFull || nt > 0xFFFFFFFFull) return false;
  src.assign((size_t)n, 0u);
  toks.clear();
  for (uint64_t k = 0; k < nt; ++k) {
    if (at >= w.size()) return false;
    Token t;
    t.kind = w[at++];
    t.dist = 0u;
    t.len = 0u;
    t.lit_off = 0u;
    t.lit_len = 0u;
    t.tx_delta = 0;
    uint64_t a = 0u, b = 0u;
    if (t.kind == 0u) {
      if (!get_uvarint(w, at, a) || !get_uvarint(w, at, b)) return false;
      t.lit_off = (uint32_t)a;
      t.lit_len = (uint32_t)b;
    } else if (t.kind == 1u || t.kind == 2u) {
      if (!get_uvarint(w, at, a) || !get_uvarint(w, at, b)) return false;
      t.dist = (uint32_t)a;
      t.len = (uint32_t)b;
      if (t.dist == 0u || t.len == 0u || t.len > kMaxLen) return false;
      if (arm_tx_delta(arm) && !get_zigzag(w, at, t.tx_delta)) return false;
      if (arm_tx_topo(arm)) {
        if (at > w.size()) return false;
        if (!get_uvarint(w, at, a)) return false;
        if (a > (uint64_t)w.size() - (uint64_t)at) return false;
        for (uint64_t j = 0; j < a; ++j) {
          Site s;
          uint64_t off = 0u;
          int32_t sig = 0;
          if (!get_uvarint(w, at, off) || !get_zigzag(w, at, sig)) return false;
          if (off + 4u > (uint64_t)t.len) return false;
          if (sig != 1 && sig != -1) return false;
          if (overlaps((uint32_t)off, t.dist)) return false;
          s.off = (uint32_t)off;
          s.sigma = sig;
          t.tx_topo.push_back(s);
        }
        if (!sites_are_disjoint(t.tx_topo)) return false;
      }
    } else {
      return false;
    }
    toks.push_back(std::move(t));
  }
  return at == w.size();
}

static std::vector<uint8_t> reconstruct(const std::vector<uint8_t>& src,
                                        const std::vector<Token>& toks, Arm arm) {
  std::vector<uint8_t> out;
  out.reserve(src.size());
  for (const Token& t : toks) {
    if (t.kind == 0u) {
      if (t.lit_off + t.lit_len > src.size()) return {};
      out.insert(out.end(), src.begin() + (long)t.lit_off, src.begin() + (long)(t.lit_off + t.lit_len));
      continue;
    }
    uint32_t p = (uint32_t)out.size();
    if (t.dist > p) return {};
    if ((uint64_t)p + (uint64_t)t.len > (uint64_t)src.size()) return {};
    for (uint32_t i = 0; i < t.len; ++i) out.push_back(out[p - t.dist + i]);
    if (t.kind == 1u) continue;
    std::vector<Site> sites;
    if (arm_derived_topo(arm)) {
      const uint8_t* s = out.data() + (size_t)(p - t.dist);
      sites = classify_sites(s, t.len, t.dist);
    } else {
      sites = t.tx_topo;
    }
    for (const Site& st : sites) {
      if (st.off + 4u > t.len) return {};
      if (overlaps(st.off, t.dist)) return {};
      uint32_t delta;
      if (arm_derived_delta(arm)) {
        int64_t sgn = (int64_t)st.sigma * (int64_t)t.dist;
        delta = (uint32_t)(int32_t)sgn;
      } else {
        delta = (uint32_t)t.tx_delta;
      }
      store32(&out[p + st.off], load32(&out[p + st.off]) + delta);
    }
  }
  if (out.size() != src.size()) return {};
  return out;
}

static std::vector<Token> parse(const std::vector<uint8_t>& src, Arm arm) {
  std::vector<Token> toks;
  std::vector<uint64_t> head(1u << 20, 0xFFFFFFFFull);
  uint32_t p = 0u;
  uint32_t lit_start = 0u;
  while (p < src.size()) {
    bool placed = false;
    if (p + 12u <= src.size() && p >= 4u) {
      uint64_t h = (uint64_t)load32(&src[p]) * 0x9E3779B185EBCA87ull;
      h ^= (uint64_t)load32(&src[p + 4]) * 0xC2B2AE3D27D4EB4Full;
      h ^= (uint64_t)load32(&src[p + 8]) * 0x165667B19E3779F9ull;
      h ^= (uint64_t)p;
      h = (h ^ (h >> 29)) * 0xBF58476D1CE4E5B9ull;
      h ^= h >> 32;
      uint64_t slot = h & ((1u << 20) - 1u);
      uint64_t cand = head[slot];
      if (cand != 0xFFFFFFFFull) {
        uint32_t q = (uint32_t)cand;
        uint32_t d = p - q;
        if (d >= 4u) {
          uint32_t L = 0u;
          while (L < kMaxLen && (uint64_t)p + L + 4u <= (uint64_t)src.size() &&
                 load32(&src[q + L]) + (uint32_t)(-(int64_t)d) == load32(&src[p + L]))
            ++L;
          if (L >= 12u) {
            std::vector<Site> sites = classify_sites(&src[q], L, d);
            if (!sites.empty() && xref_reconstructs(src, p, d, L, sites)) {
              if (lit_start < p) {
                Token lit;
                lit.kind = 0u;
                lit.lit_off = lit_start;
                lit.lit_len = p - lit_start;
                toks.push_back(lit);
              }
              Token t;
              t.kind = 2u;
              t.dist = d;
              t.len = L;
              t.lit_off = 0u;
              t.lit_len = 0u;
              t.tx_delta = 0;
              t.tx_topo = sites;
              toks.push_back(std::move(t));
              p += L;
              lit_start = p;
              head[slot] = q;
              placed = true;
            }
          }
        }
      }
      head[slot] = p;
    }
    if (!placed) ++p;
  }
  if (lit_start < src.size()) {
    Token lit;
    lit.kind = 0u;
    lit.lit_off = lit_start;
    lit.lit_len = (uint32_t)src.size() - lit_start;
    toks.push_back(lit);
  }
  (void)arm;
  return toks;
}

static std::vector<uint8_t> read_file(const char* path) {
  std::vector<uint8_t> b;
  FILE* f = std::fopen(path, "rb");
  if (!f) return b;
  std::fseek(f, 0, SEEK_END);
  long n = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (n <= 0) {
    std::fclose(f);
    return b;
  }
  b.resize((size_t)n);
  size_t got = std::fread(b.data(), 1u, (size_t)n, f);
  std::fclose(f);
  b.resize(got);
  return b;
}

static int selftest() {
  std::vector<uint8_t> src(4096u);
  for (size_t i = 0; i < src.size(); ++i) src[i] = (uint8_t)(i * 31u + (i >> 5));
  src[100] = 0xE8u;
  src[200] = 0xE9u;
  const Arm arms[] = {Arm::ExactOnly,          Arm::TxDeltaDerivedTopo,
                      Arm::TxDeltaTxTopo,       Arm::DerivedDeltaTxTopo,
                      Arm::DerivedDeltaDerivedTopo};
  for (Arm arm : arms) {
    std::vector<Token> toks = parse(src, arm);
    std::vector<uint8_t> w = serialize(src, toks, arm);
    std::vector<uint8_t> back;
    std::vector<Token> toks2;
    if (!deserialize(w, back, toks2)) {
      std::printf("FAIL deserialize arm=%u\n", (unsigned)arm);
      return 1;
    }
    std::vector<uint8_t> out = reconstruct(back, toks2, arm);
    if (out != src) {
      std::printf("FAIL roundtrip arm=%u bytes=%zu\n", (unsigned)arm, out.size());
      return 1;
    }
    std::printf("PASS arm=%u wire=%zu tokens=%zu\n", (unsigned)arm, w.size(), toks.size());
  }
  {
    std::vector<uint8_t> w = serialize(src, parse(src, Arm::DerivedDeltaDerivedTopo),
                                       Arm::DerivedDeltaDerivedTopo);
    for (size_t i = 0; i + 1u < w.size(); ++i) {
      std::vector<uint8_t> mut = w;
      mut[i] ^= 0x20u;
      std::vector<uint8_t> back;
      std::vector<Token> toks2;
      if (!deserialize(mut, back, toks2)) continue;
      std::vector<uint8_t> out = reconstruct(back, toks2, Arm::DerivedDeltaDerivedTopo);
      if (out != src) {
        std::printf("FAIL silent-wrong-accept at byte %zu\n", i);
        return 1;
      }
    }
  }
  std::printf("PASS mutation sweep\n");
  return 0;
}

}

int main(int argc, char** argv) {
  if (argc >= 2 && std::strcmp(argv[1], "--selftest") == 0) return slx::selftest();
  if (argc < 3) {
    std::printf("usage: slxref <in> <out> [--arm=N] | --selftest\n");
    return 2;
  }
  std::vector<uint8_t> src = slx::read_file(argv[1]);
  if (src.empty()) {
    std::printf("FAIL read\n");
    return 1;
  }
  slx::Arm arm = slx::Arm::DerivedDeltaDerivedTopo;
  if (argc >= 4 && std::strncmp(argv[3], "--arm=", 6) == 0)
    arm = (slx::Arm)(unsigned)std::atoi(argv[3] + 6);
  std::vector<slx::Token> toks = slx::parse(src, arm);
  std::vector<uint8_t> w = slx::serialize(src, toks, arm);
  std::vector<uint8_t> back;
  std::vector<slx::Token> toks2;
  if (!slx::deserialize(w, back, toks2) || slx::reconstruct(back, toks2, arm) != src) {
    std::printf("FAIL roundtrip\n");
    return 1;
  }
  FILE* o = std::fopen(argv[2], "wb");
  if (!o) return 1;
  std::fwrite(w.data(), 1u, w.size(), o);
  std::fclose(o);
  std::printf("orig=%zu wire=%zu tokens=%zu arm=%u\n", src.size(), w.size(), toks.size(),
              (unsigned)arm);
  return 0;
}