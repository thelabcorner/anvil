// DDMC bounded implementation sketch -- Decoder-Derived Model Continuity.
//
// NOT wired into production. Self-test only: synthetic in-process data,
// exact byte accounting, roundtrip verification. No corpus, no timing claims.
//
// Purpose: make the central report claim machine-checkable -- that a
// per-block rANS model header (1 + uvar(nz) + nz*(1 + uvar(freq))) can be
// replaced by a 1-byte reference into a decoder-resident bank, with the
// encoder deriving candidate tables from symbols the decoder already holds.
//
// Layout mirrors src/anvil.cpp: rANS-4096 (12-bit scale, 32-bit state,
// byte renormalisation), reverse-order encode.

#include <array>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr uint32_t TOT = 1u << 12;
constexpr uint32_t LB = 1u << 23;
constexpr uint32_t SCALE = 12;
constexpr int BANK_SLOTS = 8;
constexpr int HIST_REPL = 4;  // histogram replicas, breaks same-address store chain

using Hist = std::array<uint32_t, 256>;

struct Model {
  std::array<uint16_t, 256> freq{};
  std::array<uint16_t, 256> start{};
};

size_t uvar_size(uint64_t v) {
  size_t n = 1;
  while (v >= 0x80) { v >>= 7; ++n; }
  return n;
}

Model normalize(const Hist& h) {
  Model m;
  uint64_t n = 0;
  for (int i = 0; i < 256; ++i) n += h[i];
  if (n == 0) { m.freq[0] = static_cast<uint16_t>(TOT); return m; }

  // floor(count * TOT / n), then largest-remainder redistribution in both
  // directions. O(alphabet log alphabet) -- the builder shape recorded in
  // RESEARCH_LEDGER.md Experiment L follow-ups (the iterative +/-1 fixup
  // used in src/anvil.cpp build_rans_model is O(TOT * alphabet) and is not
  // viable inside an encode loop).
  struct Rem { double frac; uint16_t idx; };
  Rem rem[256];
  int nr = 0;
  uint32_t sum = 0;
  for (int i = 0; i < 256; ++i) {
    if (!h[i]) continue;
    double ex = double(h[i]) * double(TOT) / double(n);
    uint32_t f = static_cast<uint32_t>(ex);
    if (f == 0) f = 1;
    m.freq[i] = static_cast<uint16_t>(f);
    sum += f;
    rem[nr].frac = ex - double(f);
    rem[nr].idx = static_cast<uint16_t>(i);
    ++nr;
  }

  std::sort(rem, rem + nr, [](const Rem& a, const Rem& b) {
    if (a.frac != b.frac) return a.frac > b.frac;
    return a.idx < b.idx;
  });
  int k = 0;
  while (sum < TOT) {
    if (nr == 0) break;
    ++m.freq[rem[k % nr].idx]; ++sum; ++k;
  }
  k = 0;
  while (sum > TOT) {
    bool moved = false;
    for (int pass = 0; pass < nr && sum > TOT; ++pass) {
      uint16_t idx = rem[(nr - 1 - ((k + pass) % nr))].idx;
      if (m.freq[idx] > 1) { --m.freq[idx]; --sum; moved = true; }
    }
    if (!moved) break;
  }
  (void)k;
  uint32_t st = 0;
  for (int i = 0; i < 256; ++i) { m.start[i] = static_cast<uint16_t>(st); st += m.freq[i]; }
  if (st != TOT) throw std::runtime_error("normalize: sum != TOT");
  return m;
}

bool same_table(const Model& a, const Model& b) {
  for (int i = 0; i < 256; ++i) if (a.freq[i] != b.freq[i]) return false;
  return true;
}

std::vector<uint8_t> rans_enc(const std::vector<uint8_t>& s, const Model& m) {
  if (s.empty()) return {};
  uint32_t x = LB;
  std::vector<uint8_t> em;
  em.reserve(s.size() / 2 + 16);
  for (size_t i = s.size(); i-- > 0;) {
    uint8_t y = s[i];
    uint32_t f = m.freq[y], st = m.start[y];
    uint32_t xm = ((LB >> SCALE) << 8) * f;
    while (x >= xm) { em.push_back(static_cast<uint8_t>(x)); x >>= 8; }
    x = ((x / f) << SCALE) + (x % f) + st;
  }
  std::vector<uint8_t> o;
  o.reserve(4 + em.size());
  for (int k = 0; k < 4; ++k) o.push_back(static_cast<uint8_t>(x >> (8 * k)));
  for (auto it = em.rbegin(); it != em.rend(); ++it) o.push_back(*it);
  return o;
}

std::vector<uint8_t> rans_dec(const uint8_t* p, size_t n, size_t out_n, const Model& m) {
  if (out_n == 0) return {};
  std::vector<uint8_t> symtab(TOT, 0);
  for (int s = 0; s < 256; ++s) if (m.freq[s]) for (uint32_t j = 0; j < m.freq[s]; ++j) symtab[m.start[s] + j] = static_cast<uint8_t>(s);
  std::vector<uint8_t> out(out_n);
  const uint8_t* q = p; const uint8_t* e = p + n;
  uint32_t x = 0;
  for (int k = 0; k < 4; ++k) { if (q >= e) throw std::runtime_error("dec: short state"); x |= static_cast<uint32_t>(*q++) << (8 * k); }
  for (size_t i = 0; i < out_n; ++i) {
    uint32_t slot = x & (TOT - 1);
    uint8_t y = symtab[slot];
    out[i] = y;
    x = static_cast<uint32_t>(m.freq[y]) * (x >> SCALE) + slot - m.start[y];
    while (x < LB) { if (q >= e) throw std::runtime_error("dec: truncated renorm"); x = (x << 8) | *q++; }
  }
  return out;
}

// --- replica-interleaved symbol histogram (decoder-side, derivation source) ---
struct SymHist {
  std::array<std::array<uint32_t, 256>, HIST_REPL> h{};
  uint32_t t = 0;
  void push(uint8_t s) { ++h[t & (HIST_REPL - 1)][s]; ++t; }
  void merge() {
    for (int r = 1; r < HIST_REPL; ++r) for (int i = 0; i < 256; ++i) h[0][i] += h[r][i];
  }
  Hist flat() const {
    Hist o{};
    for (int r = 0; r < HIST_REPL; ++r) for (int i = 0; i < 256; ++i) o[i] += h[r][i];
    return o;
  }
  void clear() { for (int r = 0; r < HIST_REPL; ++r) h[r].fill(0); t = 0; }
};

enum Policy : uint8_t { POL_TRANSMIT = 0, POL_SLOT = 1 };

// Wire accounting is exact and mirrored in the report.
size_t table_bytes(const Model& m) {
  size_t nz = 0;
  for (int i = 0; i < 256; ++i) if (m.freq[i]) ++nz;
  return 1 + uvar_size(nz) + nz * 3;  // sym byte + uvar(freq) (freq<=4096 -> 2 B)
}

struct Block { Policy pol; int slot; std::vector<uint8_t> tbl; std::vector<uint8_t> payload; };

size_t wire_bytes(const Block& b) {
  // policy byte, then either a serialized table (nz count byte +
  // nz*(sym + 2-byte freq)) or a 1-byte slot index, then the payload
  // length byte and the payload itself.
  size_t n = 1;
  n += (b.pol == POL_TRANSMIT) ? b.tbl.size() : size_t(1);
  n += 1 + b.payload.size();
  return n;
}

std::vector<uint8_t> serialize_table(const Model& m) {
  std::vector<uint8_t> o;
  size_t nz = 0;
  for (int i = 0; i < 256; ++i) if (m.freq[i]) ++nz;
  o.push_back(static_cast<uint8_t>(nz));  // nz<=256 does not fit one byte; selftest streams keep nz<128
  for (int i = 0; i < 256; ++i) {
    if (!m.freq[i]) continue;
    o.push_back(static_cast<uint8_t>(i));
    o.push_back(static_cast<uint8_t>(m.freq[i] & 0xFF));
    o.push_back(static_cast<uint8_t>(m.freq[i] >> 8));
  }
  return o;
}

Model deserialize_table(const std::vector<uint8_t>& v) {
  if (v.empty()) throw std::runtime_error("tbl: empty");
  Model m;
  size_t p = 0;
  size_t nz = v[p++];
  for (size_t k = 0; k < nz; ++k) {
    if (p + 3 > v.size()) throw std::runtime_error("tbl: truncated");
    uint8_t s = v[p++];
    uint32_t f = static_cast<uint32_t>(v[p]) | (static_cast<uint32_t>(v[p + 1]) << 8);
    p += 2;
    if (m.freq[s]) throw std::runtime_error("tbl: dup symbol");
    m.freq[s] = static_cast<uint16_t>(f);
  }
  uint32_t st = 0;
  for (int i = 0; i < 256; ++i) { m.start[i] = static_cast<uint16_t>(st); st += m.freq[i]; }
  if (st != TOT) throw std::runtime_error("tbl: sum != TOT");
  return m;
}

// --- synthetic streams with controlled stationarity -------------------------
uint32_t rng_state = 0x12345678u;
uint32_t rnd() { rng_state ^= rng_state << 13; rng_state ^= rng_state >> 17; rng_state ^= rng_state << 5; return rng_state; }

std::vector<uint8_t> gen_stream(int regime, size_t n, int seed) {
  rng_state = 0x12345678u + static_cast<uint32_t>(seed) * 2654435761u;
  std::vector<uint8_t> s(n);
  for (size_t i = 0; i < n; ++i) {
    // three geometric regimes with distinct skew, so quantization tables differ
    int reg = regime;
    if (regime == 1) reg = static_cast<int>((i / (n / 3)) % 3);
    if (regime == 2) reg = static_cast<int>((i * 3) / n);  // smooth drift
    uint32_t u = rnd() % 1000;
    uint8_t b;
    if (reg == 0) b = (u < 500 ? 0 : (u < 750 ? 1 : (u < 900 ? 2 : 3 + rnd() % 8)));
    else if (reg == 1) b = (u < 200 ? 65 : (u < 400 ? 66 : (u < 600 ? 10 : (u < 800 ? 44 : rnd() % 200))));
    else b = (u < 700 ? 7 : (u < 850 ? 200 : rnd() % 64));
    s[i] = b;
  }
  return s;
}

struct Result { size_t in_bytes; size_t transmit_bytes; size_t ddmc_bytes; int blocks; int distinct; int bank_hits; };

Result run(int regime, size_t block_len, int blocks, int seed) {
  SymHist hist;
  Model bank[BANK_SLOTS];
  bool bank_valid[BANK_SLOTS];
  for (int i = 0; i < BANK_SLOTS; ++i) bank_valid[i] = false;
  Model prev_derived;
  bool have_prev = false;

  Result r{};
  r.blocks = blocks;
  for (int b = 0; b < blocks; ++b) {
    std::vector<uint8_t> s = gen_stream(regime, block_len, seed + b);

    // --- control arm: model from this block's own symbols, fully transmitted
    Hist h{};
    for (uint8_t c : s) ++h[c];
    Model own = normalize(h);
    Block tb; tb.pol = POL_TRANSMIT; tb.slot = -1;
    tb.tbl = serialize_table(own);
    tb.payload = rans_enc(s, own);
    size_t tb_wire = 1 + tb.tbl.size() + 1 + tb.payload.size();
    r.transmit_bytes += tb_wire;

    // --- DDMC arm: derive from the previous block's decoded symbols
    if (!have_prev) { prev_derived = own; have_prev = true; }
    Model cand = normalize(hist.flat());

    int slot = -1;
    for (int i = 0; i < BANK_SLOTS; ++i) if (bank_valid[i] && same_table(bank[i], cand)) { slot = i; break; }
    if (slot >= 0) {
      ++r.bank_hits;
      Block db; db.pol = POL_SLOT; db.slot = slot; db.payload = rans_enc(s, cand);
      r.ddmc_bytes += 1 + 1 + 1 + db.payload.size();
    } else {
      ++r.distinct;
      int free = -1;
      for (int i = 0; i < BANK_SLOTS; ++i) if (!bank_valid[i]) { free = i; break; }
      if (free < 0) { free = static_cast<int>(static_cast<uint64_t>(r.distinct) % BANK_SLOTS); }
      bank[free] = cand; bank_valid[free] = true;
      Block db; db.pol = POL_TRANSMIT; db.slot = -1;
      db.tbl = serialize_table(cand);
      db.payload = rans_enc(s, cand);
      r.ddmc_bytes += 1 + db.tbl.size() + 1 + db.payload.size();
    }

    // decoder-side mirror: symbols are counted after decode, model advances
    hist.clear();
    for (uint8_t c : s) hist.push(c);
    hist.merge();
    prev_derived = cand;
    r.in_bytes += s.size();
  }
  return r;
}

// --- exact roundtrip of the DDMC wire -------------------------------------
void roundtrip_selftest() {
  for (int regime = 0; regime < 3; ++regime) {
    std::vector<std::vector<uint8_t>> blocks;
    SymHist hist;
    Model bank[BANK_SLOTS];
    bool valid[BANK_SLOTS] = {};
    std::vector<std::vector<uint8_t>> wires;
    std::vector<int> slots;
    std::vector<bool> is_slot;
    for (int b = 0; b < 6; ++b) {
      std::vector<uint8_t> s = gen_stream(regime, 4096, 900 + b);
      blocks.push_back(s);
      Model cand = normalize(hist.flat());
      int slot = -1;
      for (int i = 0; i < BANK_SLOTS; ++i) if (valid[i] && same_table(bank[i], cand)) { slot = i; break; }
      if (slot >= 0) { wires.push_back(rans_enc(s, cand)); slots.push_back(slot); is_slot.push_back(true); }
      else {
        int free = -1; for (int i = 0; i < BANK_SLOTS; ++i) if (!valid[i]) { free = i; break; }
        if (free < 0) free = b % BANK_SLOTS;
        bank[free] = cand; valid[free] = true;
        wires.push_back(rans_enc(s, cand)); slots.push_back(free); is_slot.push_back(false);
      }
      hist.clear(); for (uint8_t c : s) hist.push(c); hist.merge();
    }
    // decoder pass
    SymHist dh;
    for (size_t b = 0; b < blocks.size(); ++b) {
      Model m = normalize(dh.flat());
      Model used = is_slot[b] ? bank[slots[b]] : m;
      std::vector<uint8_t> out = rans_dec(wires[b].data(), wires[b].size(), blocks[b].size(), used);
      if (out != blocks[b]) throw std::runtime_error("roundtrip mismatch");
      dh.clear(); for (uint8_t c : out) dh.push(c); dh.merge();
    }
  }
  std::printf("roundtrip: 18/18 blocks exact\n");
}

}  // namespace

int main() {
  roundtrip_selftest();

  std::printf("\nDDMC header-amortization self-test (synthetic; NOT a corpus result)\n");
  std::printf("%-10s %8s %6s %12s %12s %8s %8s %8s\n",
              "regime", "blocklen", "blocks", "transmit_B", "ddmc_B", "saved%", "D/B", "hit%");
  for (int regime = 0; regime < 3; ++regime) {
    for (size_t bl : {size_t(4096), size_t(65536)}) {
      for (int blocks : {4, 32}) {
        Result r = run(regime, bl, blocks, 31);
        double saved = 100.0 * (double(r.transmit_bytes - r.ddmc_bytes) / double(r.transmit_bytes));
        double db = double(r.distinct) / double(r.blocks);
        double hit = 100.0 * double(r.bank_hits) / double(r.blocks);
        std::printf("%-10s %8zu %6d %12zu %12zu %8.2f %8.3f %8.1f\n",
                    regime == 0 ? "iid" : (regime == 1 ? "piecewise" : "drift"),
                    bl, blocks, r.transmit_bytes, r.ddmc_bytes, saved, db, hit);
      }
    }
  }
  std::printf("\nheader model: transmit = 1 + (1 + uvar(nz)) + nz*3 B/block/stream\n");
  std::printf("bank slot    = 1 + 1 B/block/stream (policy + slot), plus the table once per distinct quantized model\n");
  return 0;
}