// ANVIL P4.1 slice-3 — native replay check harness (pinned zlib 1.3.1).
//
// Lane (a) slice-3 per docs/P41-NATIVE-WORKORDER.md: vendored-engine +
// parameter-registry + transform-4 construction, gated on the frozen
// population. Adopt-class infrastructure; NO novelty claim.
//
// What this tool does NOT do: no backend, no router, no CLI surface, no
// src/anvil.cpp changes. Transform-4 record framing below is harness-
// internal (construction + parse roundtrip verified here); wire freeze,
// FORMAT.md registration, and codec integration belong to slice-4/5.
//
// Build (pinned toolchain, same as codec CI):
//   clang++-18 -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic \
//     -Ithird_party/zlib/include \
//     tools/p41_replay_check.cpp \
//     third_party/zlib/src/{adler32,compress,crc32,deflate}.c \
//     third_party/zlib/src/{inffast,inflate,inftrees,trees,zutil}.c \
//     -o p41_replay_check
// Only the C sources needed for deflate+inflate are compiled; the gz file
// API is deliberately excluded (decoder never touches the filesystem).

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "zlib.h"

// ---------------------------------------------------------------------------
// Frozen expectations (prereg P41-DEFLATE-REPLAY-PREREG-DRAFT, gate I9-5).
// ---------------------------------------------------------------------------
namespace frozen {
constexpr long kMozillaValid = 2331;
constexpr long kMozillaValidBytes = 2898132;
constexpr long kSambaValid = 160;
}  // namespace frozen

// ---------------------------------------------------------------------------
// Pinned replay engine identity (immutable; see third_party/zlib/UPSTREAM.md).
// ---------------------------------------------------------------------------
namespace engine {
constexpr int kId = 1;
constexpr const char* kName = "zlib-1.3.1-pinned-v1";
constexpr const char* kZlibVersion = "1.3.1";
}  // namespace engine

// ---------------------------------------------------------------------------
// Parameter registry (immutable). Codes cover the measured population:
// 0:(raw,6,8,default) x2282, 1:(raw,6,9,default) x7,
// 2:(zlib-wrap,6,8,filtered) x42 (PNG). 255 = escape w/ explicit params.
// ---------------------------------------------------------------------------
struct ReplayParams {
  int wbits;  // -15 raw, or 9..15 zlib-wrapped
  int level;  // 1..9
  int mem_level;  // 1..9
  int strategy;  // 0..4
  bool operator==(const ReplayParams& o) const {
    return wbits == o.wbits && level == o.level && mem_level == o.mem_level &&
           strategy == o.strategy;
  }
};

namespace registry {
constexpr int kCodeRaw68Default = 0;
constexpr int kCodeRaw69Default = 1;
constexpr int kCodeZlib68Filtered = 2;
constexpr int kCodeEscape = 255;

inline ReplayParams Decode(int code, int ew = 0, int el = 0, int em = 0,
                           int es = 0) {
  // Mirrors plan §7: three frozen codes + escape with explicit params.
  switch (code) {
    case 0:
      return {-15, 6, 8, 0};
    case 1:
      return {-15, 6, 9, 0};
    case 2:
      return {15, 6, 8, 1};
    case 255:
      return {ew, el, em, es};
    default:
      return {0, 0, 0, -1};  // invalid: caught by IsValidParam below
  }
}

inline bool IsValidParam(const ReplayParams& p) {
  if (p.level < 1 || p.level > 9) return false;
  if (p.mem_level < 1 || p.mem_level > 9) return false;
  if (p.strategy < 0 || p.strategy > 4) return false;
  if (p.wbits == -15) return true;
  if (p.wbits >= 9 && p.wbits <= 15) return true;
  return false;
}

inline int Encode(const ReplayParams& p) {
  if (p == Decode(0)) return 0;
  if (p == Decode(1)) return 1;
  if (p == Decode(2)) return 2;
  return 255;
}
}  // namespace registry

// Priority order: EXACT same sequence as prototypes/i9-deflate/replay.py
// PRIORITY list (registry entries first by construction).
static const ReplayParams kPriority[] = {
    {0, 6, 8, 0}, {0, 9, 8, 0}, {0, 1, 8, 0}, {0, 2, 8, 0}, {0, 3, 8, 0},
    {0, 4, 8, 0}, {0, 5, 8, 0}, {0, 7, 8, 0}, {0, 8, 8, 0}, {0, 6, 9, 0},
    {0, 9, 9, 0}, {0, 6, 8, 1}, {0, 6, 8, 2}, {0, 6, 8, 3}, {0, 6, 8, 4},
    {0, 9, 8, 1}, {0, 9, 8, 2}, {0, 9, 8, 3}, {0, 9, 8, 4},
};
// NOTE: wbits slot in kPriority is 0 as placeholder; the caller substitutes
// the per-kind wbits (-15 raw, CMF-derived/15 wrapped) exactly like the
// prototype: kind in {"zip","cab-mszip"} -> [-15], else [cmf-window, 15].

// ---------------------------------------------------------------------------
// Checked arithmetic + bounds (fail-closed; plan §11).
// ---------------------------------------------------------------------------
inline bool AddLe(size_t a, size_t b, size_t cap, size_t* out) {
  if (a > cap || b > cap - a) return false;
  *out = a + b;
  return true;
}

// ---------------------------------------------------------------------------
// Pinned-zlib deflate/inflate wrappers (narrow ANVIL surface; no zlib state
// leaks through the core codec: each call constructs, uses, tears down).
// ---------------------------------------------------------------------------
namespace pinned {

inline bool Inflate(const uint8_t* in, size_t in_len, uint8_t* out,
                    size_t out_cap, size_t* out_len, int window_bits) {
  z_stream strm;
  memset(&strm, 0, sizeof(strm));
  strm.next_in = const_cast<Bytef*>(in);
  strm.avail_in = static_cast<uInt>(in_len);
  strm.next_out = out;
  strm.avail_out = static_cast<uInt>(out_cap);
  if (inflateInit2(&strm, window_bits) != Z_OK) return false;
  const int rc = inflate(&strm, Z_FINISH);
  *out_len = out_cap - strm.avail_out;
  inflateEnd(&strm);
  return rc == Z_STREAM_END;
}

inline const char* Version() { return zlibVersion(); }

}  // namespace pinned

// ---------------------------------------------------------------------------
// Transform-4 record framing (harness-internal construction + parse;
// mirrors plan §6 layout; wire freeze + FORMAT.md belong to slice-4/5).
//
// uvarint encoding (LEB128, unsigned). Layout:
//   uvarint representation_version (=1)
//   uvarint replay_engine_id (=1)
//   uvarint record_count
//   per record:
//     uvarint gap_len | uvarint compressed_len | uvarint plaintext_len
//     uvarint param_code
//     gap bytes | plaintext bytes
//   uvarint tail_len | tail bytes
// ---------------------------------------------------------------------------
namespace t4 {
constexpr uint64_t kVersion = 1;
constexpr uint64_t kMaxRecords = 1 << 20;  // 1M cap, far above 2,354
constexpr size_t kMaxPlainPerRecord = 64u << 20;  // 64 MiB bound
constexpr size_t kMaxTotal = 512u << 20;          // 512 MiB bound

inline bool PutUvar(std::vector<uint8_t>& out, uint64_t v) {
  do {
    uint8_t b = static_cast<uint8_t>(v & 0x7f);
    v >>= 7;
    if (v) b |= 0x80;
    out.push_back(b);
  } while (v);
  return true;
}

inline bool GetUvar(const uint8_t*& p, const uint8_t* end, uint64_t* v) {
  uint64_t r = 0;
  int shift = 0;
  while (p < end) {
    const uint8_t b = *p++;
    if (shift >= 64) return false;
    r |= static_cast<uint64_t>(b & 0x7f) << shift;
    shift += 7;
    if (!(b & 0x80)) {
      *v = r;
      return true;
    }
    if (shift > 63) return false;
  }
  return false;  // truncated
}

struct Record {
  std::vector<uint8_t> gap;
  std::vector<uint8_t> plain;
  size_t compressed_len = 0;  // declared original payload length
  int param_code = 0;
};

inline bool Serialize(const std::vector<Record>& recs,
                      const std::vector<uint8_t>& tail,
                      std::vector<uint8_t>* out) {
  if (recs.size() > kMaxRecords) return false;
  std::vector<uint8_t> buf;
  PutUvar(buf, kVersion);
  PutUvar(buf, static_cast<uint64_t>(engine::kId));
  PutUvar(buf, recs.size());
  for (const auto& r : recs) {
    if (r.gap.size() > kMaxTotal || r.plain.size() > kMaxPlainPerRecord)
      return false;
    if (!registry::IsValidParam(registry::Decode(
            r.param_code,
            /*explicit*/ 0, 0, 0, 0)) &&
        r.param_code != registry::kCodeEscape)
      return false;
    PutUvar(buf, r.gap.size());
    PutUvar(buf, r.compressed_len);
    PutUvar(buf, r.plain.size());
    PutUvar(buf, static_cast<uint64_t>(r.param_code));
    buf.insert(buf.end(), r.gap.begin(), r.gap.end());
    buf.insert(buf.end(), r.plain.begin(), r.plain.end());
    if (buf.size() > kMaxTotal) return false;
  }
  PutUvar(buf, tail.size());
  buf.insert(buf.end(), tail.begin(), tail.end());
  if (buf.size() > kMaxTotal) return false;
  *out = std::move(buf);
  return true;
}

inline bool Parse(const uint8_t* data, size_t len, uint64_t block_len,
                  std::vector<Record>* recs, std::vector<uint8_t>* tail) {
  const uint8_t* p = data;
  const uint8_t* end = data + len;
  uint64_t version = 0, engine = 0, count = 0;
  if (!GetUvar(p, end, &version)) return false;
  if (version != kVersion) return false;  // unknown representation version
  if (!GetUvar(p, end, &engine)) return false;
  if (engine != static_cast<uint64_t>(engine::kId))
    return false;  // unknown engine ID
  if (!GetUvar(p, end, &count)) return false;
  if (count > kMaxRecords) return false;  // impossible record count
  // Bound total output by the declared block length (plan §11).
  size_t produced = 0;
  recs->clear();
  for (uint64_t i = 0; i < count; ++i) {
    uint64_t gap_len = 0, clen = 0, ulen = 0, pcode = 0;
    if (!GetUvar(p, end, &gap_len)) return false;  // truncated varint
    if (!GetUvar(p, end, &clen)) return false;      // truncated varint
    if (!GetUvar(p, end, &ulen)) return false;      // truncated varint
    if (!GetUvar(p, end, &pcode)) return false;     // truncated varint
    if (pcode > 255) return false;
    if (pcode != 0 && pcode != 1 && pcode != 2 && pcode != 255)
      return false;  // invalid parameter code
    if (gap_len > static_cast<uint64_t>(end - p)) return false;  // gap overflow
    size_t grown = 0;
    if (!AddLe(produced, static_cast<size_t>(gap_len), block_len, &grown))
      return false;  // gap overflow vs block
    produced = grown;
    if (ulen > kMaxPlainPerRecord) return false;  // plaintext overflow
    if (ulen > static_cast<uint64_t>(end - p - gap_len))
      return false;  // plaintext overflow
    if (!AddLe(produced, static_cast<size_t>(ulen), block_len, &grown))
      return false;  // final-size overflow
    produced = grown;
    if (clen == 0 || clen > kMaxTotal) return false;  // bad declared length
    Record r;
    r.gap.assign(p, p + gap_len);
    p += gap_len;
    r.plain.assign(p, p + ulen);
    p += ulen;
    r.compressed_len = static_cast<size_t>(clen);
    r.param_code = static_cast<int>(pcode);
    recs->push_back(std::move(r));
  }
  uint64_t tail_len = 0;
  if (!GetUvar(p, end, &tail_len)) return false;  // truncated tail
  if (tail_len > static_cast<uint64_t>(end - p)) return false;  // trunc tail
  size_t grown = 0;
  if (!AddLe(produced, static_cast<size_t>(tail_len), block_len, &grown))
    return false;
  produced = grown;
  tail->assign(p, p + tail_len);
  p += tail_len;
  if (p != end) return false;            // exact transformed-input consumption
  if (produced != block_len) return false;  // exact final size required
  return true;
}

}  // namespace t4

// ---------------------------------------------------------------------------
// Minimal CSV reader (census schema: offset,clen,ulen,kind,entry,method,...).
// ---------------------------------------------------------------------------
struct CsvRow {
  std::map<std::string, std::string> f;
};

static std::vector<std::string> SplitCsv(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  bool q = false;
  for (size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (q) {
      if (c == '"') {
        if (i + 1 < line.size() && line[i + 1] == '"') {
          cur += '"';
          ++i;
        } else {
          q = false;
        }
      } else {
        cur += c;
      }
    } else if (c == '"') {
      q = true;
    } else if (c == ',') {
      out.push_back(cur);
      cur.clear();
    } else if (c == '\r') {
      continue;
    } else {
      cur += c;
    }
  }
  out.push_back(cur);
  return out;
}

static bool ReadCsv(const std::string& path,
                    std::vector<std::string>* header,
                    std::vector<CsvRow>* rows, std::string* err) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    *err = "cannot open " + path;
    return false;
  }
  std::string line;
  if (!std::getline(f, line)) {
    *err = "empty csv " + path;
    return false;
  }
  *header = SplitCsv(line);
  while (std::getline(f, line)) {
    if (line.empty()) continue;
    const std::vector<std::string> cells = SplitCsv(line);
    if (cells.size() != header->size()) {
      *err = "ragged row in " + path;
      return false;
    }
    CsvRow r;
    for (size_t i = 0; i < header->size(); ++i) r.f[(*header)[i]] = cells[i];
    rows->push_back(std::move(r));
  }
  return true;
}

static bool ReadFile(const std::string& path, std::vector<uint8_t>* out,
                     std::string* err) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) {
    *err = "cannot open " + path;
    return false;
  }
  const auto n = static_cast<size_t>(f.tellg());
  f.seekg(0);
  out->resize(n);
  if (n && !f.read(reinterpret_cast<char*>(out->data()),
                   static_cast<std::streamsize>(n))) {
    *err = "short read " + path;
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// Replay one stream with the pinned zlib; registry first (plan §9 step 2),
// then bounded fallback grid in prototype order (plan §9 step 3).
// ---------------------------------------------------------------------------
struct ReplayResult {
  bool valid = false;
  ReplayParams params{0, 0, 0, -1};
  int registry_code = -1;
  size_t plain_len = 0;
};

static const int kStrategies[5] = {0, 1, 2, 3, 4};
static const int kLevels[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
static const int kMemLevels[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

static bool TryDeflate(const std::vector<uint8_t>& plain,
                       const ReplayParams& p, std::vector<uint8_t>* out) {
  if (!registry::IsValidParam(p)) return false;
  uLong bound = static_cast<uLong>(plain.size() + plain.size() / 8 + 128);
  out->assign(bound, 0);
  z_stream strm;
  memset(&strm, 0, sizeof(strm));
  if (deflateInit2(&strm, p.level, Z_DEFLATED, p.wbits, p.mem_level,
                   p.strategy) != Z_OK)
    return false;
  strm.next_in = const_cast<Bytef*>(plain.data());
  strm.avail_in = static_cast<uInt>(plain.size());
  strm.next_out = out->data();
  strm.avail_out = static_cast<uInt>(out->size());
  const int rc = deflate(&strm, Z_FINISH);
  const size_t used = out->size() - strm.avail_out;
  deflateEnd(&strm);
  if (rc != Z_STREAM_END) return false;
  out->resize(used);
  return true;
}

static ReplayResult ReplayStream(const std::vector<uint8_t>& plain,
                                 const std::vector<uint8_t>& target,
                                 bool is_raw) {
  ReplayResult r;
  r.plain_len = plain.size();
  std::vector<int> wbits_list;
  if (is_raw) {
    wbits_list.push_back(-15);
  } else {
    if (!target.empty()) {
      const int cinfo = (target[0] >> 4) & 0x0F;
      const int w = cinfo + 8;
      if (w >= 9 && w <= 15) wbits_list.push_back(w);
    }
    if (std::find(wbits_list.begin(), wbits_list.end(), 15) ==
        wbits_list.end())
      wbits_list.push_back(15);
  }
  std::vector<uint8_t> out;
  for (int wb : wbits_list) {
    // Registry-first in PRIORITY order (plan §9 step 2).
    for (size_t pi = 0;
         pi < sizeof(kPriority) / sizeof(kPriority[0]); ++pi) {
      ReplayParams p{wb, kPriority[pi].level, kPriority[pi].mem_level,
                     kPriority[pi].strategy};
      if (!TryDeflate(plain, p, &out)) continue;
      if (out.size() == target.size() &&
          memcmp(out.data(), target.data(), out.size()) == 0) {
        r.valid = true;
        r.params = p;
        r.registry_code = registry::Encode(p);
        return r;
      }
    }
    // Bounded fallback grid in prototype order (plan §9 step 3).
    for (int l : kLevels)
      for (int m : kMemLevels)
        for (int s : kStrategies) {
          ReplayParams p{wb, l, m, s};
          if (!TryDeflate(plain, p, &out)) continue;
          if (out.size() == target.size() &&
              memcmp(out.data(), target.data(), out.size()) == 0) {
            r.valid = true;
            r.params = p;
            r.registry_code = registry::Encode(p);
            return r;
          }
        }
  }
  r.valid = false;
  r.registry_code = -1;
  return r;
}

// ---------------------------------------------------------------------------
// Self-test: malformed/framing suite (no corpus needed). Any missed
// rejection or crash => nonzero exit.
// ---------------------------------------------------------------------------
static int failures = 0;
#define CHECK(cond, msg)                          \
  do {                                          \
    if (!(cond)) {                              \
      ++failures;                               \
      std::cerr << "SELFTEST FAIL: " << (msg)   \
                << " (" << __LINE__ << ")\n";   \
    }                                           \
  } while (0)

static void SelfTestVarints() {
  std::vector<uint8_t> rec;
  t4::PutUvar(rec, 300);
  for (size_t cut = 0; cut < rec.size(); ++cut) {
    const uint8_t* p = rec.data();
    const uint8_t* end = rec.data() + cut;
    uint64_t v = 0;
    CHECK(!t4::GetUvar(p, end, &v) || cut == rec.size(),
          "truncated varint must fail");
  }
  const uint8_t* p = rec.data();
  const uint8_t* end = rec.data() + rec.size();
  uint64_t v = 0;
  CHECK(t4::GetUvar(p, end, &v) && v == 300, "full varint must parse");
  CHECK(p == end, "varint must consume exactly");
  // Overlong varint must fail.
  const uint8_t big[11] = {0x80, 0x80, 0x80, 0x80, 0x80,
                           0x80, 0x80, 0x80, 0x80, 0x80, 0x01};
  p = big;
  end = big + sizeof(big);
  CHECK(!t4::GetUvar(p, end, &v), "overlong varint must fail");
}

static void SelfTestFraming() {
  t4::Record r;
  r.gap = {1, 2, 3};
  r.plain = {4, 5, 6, 7};
  r.compressed_len = 9;
  r.param_code = 0;
  std::vector<t4::Record> recs = {r};
  std::vector<uint8_t> tail = {9, 9};
  std::vector<uint8_t> wire;
  CHECK(t4::Serialize(recs, tail, &wire), "serialize valid");
  // block_len = gap(3) + plain(4) + tail(2) = 9
  std::vector<t4::Record> back;
  std::vector<uint8_t> tail2;
  CHECK(t4::Parse(wire.data(), wire.size(), 9, &back, &tail2),
        "parse valid");
  CHECK(back.size() == 1 && back[0].gap == r.gap && back[0].plain == r.plain,
        "roundtrip content");
  CHECK(tail2 == tail, "roundtrip tail");
  // Unknown version: flip first varint byte.
  {
    std::vector<uint8_t> bad = wire;
    bad[0] = 2;  // version 1 -> 2
    std::vector<t4::Record> b2;
    std::vector<uint8_t> t2;
    CHECK(!t4::Parse(bad.data(), bad.size(), 9, &b2, &t2),
          "unknown version must fail");
  }
  // Unknown engine: second varint byte.
  {
    std::vector<uint8_t> bad = wire;
    bad[1] = 7;
    std::vector<t4::Record> b2;
    std::vector<uint8_t> t2;
    CHECK(!t4::Parse(bad.data(), bad.size(), 9, &b2, &t2),
          "unknown engine must fail");
  }
  // Invalid param code: construct invalid record, serialize must fail.
  {
    t4::Record bad = r;
    bad.param_code = 3;
    std::vector<t4::Record> rs = {bad};
    std::vector<uint8_t> w2;
    CHECK(!t4::Serialize(rs, tail, &w2),
          "serialize must reject invalid param code");
  }
  {
    t4::Record bad = r;
    bad.param_code = 256;
    std::vector<t4::Record> rs = {bad};
    std::vector<uint8_t> w2;
    CHECK(!t4::Serialize(rs, tail, &w2),
          "serialize must reject oversized param code");
  }
  // Truncation at every byte must fail (never OOB read).
  for (size_t cut = 0; cut < wire.size(); ++cut) {
    std::vector<t4::Record> b2;
    std::vector<uint8_t> t2;
    CHECK(!t4::Parse(wire.data(), cut, 9, &b2, &t2),
          "truncated wire must fail");
  }
  // Gap overflow vs block: gap claims more than block allows.
  {
    t4::Record bad = r;
    bad.gap.assign(100, 0x41);
    std::vector<t4::Record> rs = {bad};
    std::vector<uint8_t> w2;
    CHECK(t4::Serialize(rs, tail, &w2), "serialize big gap (test only)");
    std::vector<t4::Record> b2;
    std::vector<uint8_t> t2;
    CHECK(!t4::Parse(w2.data(), w2.size(), 9, &b2, &t2),
          "gap overflow must fail");
  }
  // Record count bound: impossible count must fail fast.
  {
    std::vector<uint8_t> w3;
    t4::PutUvar(w3, t4::kVersion);
    t4::PutUvar(w3, static_cast<uint64_t>(engine::kId));
    t4::PutUvar(w3, t4::kMaxRecords + 1);
    std::vector<t4::Record> b2;
    std::vector<uint8_t> t2;
    CHECK(!t4::Parse(w3.data(), w3.size(), 100, &b2, &t2),
          "impossible record count must fail");
  }
}

static void SelfTestBounds() {
  size_t o = 0;
  CHECK(!AddLe(SIZE_MAX, 1, SIZE_MAX, &o), "add overflow must fail");
  CHECK(!AddLe(10, 20, 25, &o), "add over cap must fail");
  CHECK(AddLe(10, 20, 40, &o) && o == 30, "add in-bounds must succeed");
  (void)o;
}

static void SelfTestEmptyRecord() {
  // Zero-record frame with empty tail must parse to empty output.
  std::vector<t4::Record> recs;
  std::vector<uint8_t> tail;
  std::vector<uint8_t> wire;
  CHECK(t4::Serialize(recs, tail, &wire), "serialize empty");
  std::vector<t4::Record> b2;
  std::vector<uint8_t> t2;
  CHECK(t4::Parse(wire.data(), wire.size(), 0, &b2, &t2),
        "empty frame must parse");
  CHECK(b2.empty() && t2.empty(), "empty frame content");
}

static int RunSelfTest() {
  SelfTestVarints();
  SelfTestFraming();
  SelfTestBounds();
  SelfTestEmptyRecord();
  // Determinism: replay the same synthetic payload twice via pinned engine.
  {
    std::vector<uint8_t> plain(1000, 0x41);
    for (size_t i = 0; i < plain.size(); ++i)
      plain[i] = static_cast<uint8_t>((i * 31 + 7) & 0xFF);
    ReplayParams p{-15, 6, 8, 0};
    std::vector<uint8_t> a, b;
    // Two independent deflate runs must agree byte-for-byte.
    {
      std::vector<uint8_t> tmp;
      if (!TryDeflate(plain, p, &tmp)) {
        CHECK(false, "determinism replay 1 must succeed");
        return 1;
      }
      a = tmp;
    }
    {
      std::vector<uint8_t> tmp;
      if (!TryDeflate(plain, p, &tmp)) {
        CHECK(false, "determinism replay 2 must succeed");
        return 1;
      }
      b = tmp;
    }
    CHECK(a == b, "pinned engine must be deterministic");
    CHECK(std::string(pinned::Version()) == "1.3.1",
          "pinned engine must be 1.3.1");
  }
  if (failures == 0) std::cout << "SELFTEST: PASS\n";
  return failures == 0 ? 0 : 1;
}

// ---------------------------------------------------------------------------
// main: --selftest | --census CSV --src FILE --out-dir DIR [--expect-*]
//// ---------------------------------------------------------------------------
static void Usage() {
  std::cerr << "usage: p41_replay_check --selftest\n"
               "   or: p41_replay_check --census CSV --src FILE --out-dir DIR\n"
               "       [--expect-valid N --expect-bytes N --name NAME]\n";
}

static std::string Arg(const std::vector<std::string>& a, size_t i) {
  return i < a.size() ? a[i] : std::string();
}

int main(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  bool selftest = false;
  std::string census, src, outdir, name = "mozilla";
  long expect_valid = -1, expect_bytes = -1;
  for (size_t i = 0; i < args.size(); ++i) {
    const std::string& a = args[i];
    if (a == "--selftest")
      selftest = true;
    else if (a == "--census")
      census = Arg(args, ++i);
    else if (a == "--src")
      src = Arg(args, ++i);
    else if (a == "--out-dir")
      outdir = Arg(args, ++i);
    else if (a == "--expect-valid")
      expect_valid = std::stol(Arg(args, ++i));
    else if (a == "--expect-bytes")
      expect_bytes = std::stol(Arg(args, ++i));
    else if (a == "--name")
      name = Arg(args, ++i);
    else {
      Usage();
      return 2;
    }
  }
  if (selftest) return RunSelfTest();
  if (census.empty() || src.empty() || outdir.empty()) {
    Usage();
    return 2;
  }

  std::string err;
  std::vector<std::string> header;
  std::vector<CsvRow> rows;
  {
    // Create the output dir (mirrors census.py os.makedirs(exist_ok=True)).
    std::error_code ec;
    std::filesystem::create_directories(outdir, ec);
    if (ec) {
      std::cerr << "cannot create out-dir " << outdir << ": " << ec.message()
                << "\n";
      return 1;
    }
  }
  if (!ReadCsv(census, &header, &rows, &err)) {
    std::cerr << "csv error: " << err << "\n";
    return 1;
  }
  auto need = [&](const char* c) {
    for (const auto& h : header)
      if (h == c) return true;
    return false;
  };
  if (!need("offset") || !need("clen") || !need("ulen") || !need("kind") ||
      !need("method")) {
    std::cerr << "csv schema mismatch (need offset/clen/ulen/kind/method)\n";
    return 1;
  }
  std::vector<uint8_t> data;
  if (!ReadFile(src, &data, &err)) {
    std::cerr << "src error: " << err << "\n";
    return 1;
  }

  long n_valid = 0, valid_bytes = 0, n_diff = 0, n_brute = 0, diff_bytes = 0;
  std::map<std::string, long> param_hist;
  std::string out_csv = outdir + "/replay-" + name + ".csv.native-check";
  std::string out_json = outdir + "/replay-" + name + ".json.native-check";
  {
    std::ofstream f(out_csv, std::ios::binary | std::ios::trunc);
    if (!f) {
      std::cerr << "cannot write " << out_csv << "\n";
      return 1;
    }
    f << "offset,clen,ulen,kind,entry,mode,param,registry_code\n";
    size_t idx = 0;
    for (const auto& r : rows) {
      if (r.f.at("method") != "deflate") continue;
      // Checked bounds (fail-closed per plan §11).
      char* e1 = nullptr;
      char* e2 = nullptr;
      const unsigned long off =
          std::strtoul(r.f.at("offset").c_str(), &e1, 10);
      const unsigned long cl =
          std::strtoul(r.f.at("clen").c_str(), &e2, 10);
      const unsigned long ul =
          std::strtoul(r.f.at("ulen").c_str(), nullptr, 10);
      if (*e1 || *e2 || off > data.size() || cl > data.size() - off) {
        std::cerr << "bounds reject at row " << idx << "\n";
        return 1;
      }
      if (ul > t4::kMaxPlainPerRecord) {
        std::cerr << "plaintext cap reject at row " << idx << "\n";
        return 1;
      }
      const uint8_t* comp = data.data() + off;
      const bool is_raw =
          (r.f.at("kind") == "zip" || r.f.at("kind") == "cab-mszip");
      // Inflate plaintext via pinned engine (validates decompress path).
      std::vector<uint8_t> plain(ul ? ul : 1, 0);
      size_t got = 0;
      bool ok = false;
      if (is_raw) {
        if (cl == 0) {
          ok = false;
        } else {
          ok = pinned::Inflate(comp, cl, plain.data(), plain.size(), &got,
                               -15);
        }
      } else if (cl == 0) {
        ok = false;
      } else {
        const int cinfo = (comp[0] >> 4) & 0x0F;
        const int w = cinfo + 8;
        if (w >= 9 && w <= 15)
          ok = pinned::Inflate(comp, cl, plain.data(), plain.size(), &got, w);
        if (!ok)
          ok =
              pinned::Inflate(comp, cl, plain.data(), plain.size(), &got, 15);
      }
      std::string mode;
      std::string param = "-/-/-/-";
      int reg = -1;
      if (ok && ul > 0 && got == ul) {
        plain.resize(got);
        ReplayResult rr = ReplayStream(
            plain, std::vector<uint8_t>(comp, comp + cl), is_raw);
        if (rr.valid) {
          mode = "valid";
          char pb[64];
          snprintf(pb, sizeof(pb), "%d/%d/%d/%d", rr.params.wbits,
                   rr.params.level, rr.params.mem_level, rr.params.strategy);
          param = pb;
          reg = rr.registry_code;
          ++n_valid;
          valid_bytes += static_cast<long>(cl);
          std::string key = "w" + std::to_string(rr.params.wbits) + "_l" +
                            std::to_string(rr.params.level) + "_m" +
                            std::to_string(rr.params.mem_level) + "_s" +
                            std::to_string(rr.params.strategy);
          param_hist[key]++;
        } else {
          mode = (plain.size() >= 256 ? "diff" : "brute");
          if (mode == "diff")
            diff_bytes += static_cast<long>(cl);
          else
            ++n_brute;
        }
      } else {
        // Inflate failed or zero-length: counted as diff (payload kept raw
        // in v1). Never fatal: census already verified decompressibility.
        mode = "diff";
        diff_bytes += static_cast<long>(cl);
      }
      if (mode != "valid" && mode != "diff" && mode != "brute") {
        std::cerr << "internal mode error at row " << idx << "\n";
        return 1;
      }
      f << off << "," << cl << "," << ul << "," << r.f.at("kind") << ",\""
        << r.f.at("entry") << "\"," << mode << "," << param << "," << reg
        << "\n";
      ++idx;
    }
  }
  {
    std::ofstream jf(out_json, std::ios::binary | std::ios::trunc);
    jf << "{\n  \"engine\": \"" << engine::kName << "\",\n";
    jf << "  \"engine_zlib_version\": \"" << pinned::Version() << "\",\n";
    jf << "  \"file\": \"" << name << "\",\n";
    jf << "  \"n_streams\": " << rows.size() << ",\n";
    jf << "  \"valid\": " << n_valid << ",\n";
    jf << "  \"valid_bytes\": " << valid_bytes << ",\n";
    jf << "  \"diff\": " << n_diff << ",\n";
    jf << "  \"brute\": " << n_brute << ",\n";
    jf << "  \"param_hist\": {";
    bool first = true;
    for (const auto& kv : param_hist) {
      if (!first) jf << ",";
      jf << "\"" << kv.first << "\":" << kv.second;
      first = false;
    }
    jf << "}\n}\n";
  }
  std::cout << "native valid=" << n_valid << " valid_bytes=" << valid_bytes
            << " diff=" << n_diff << " brute=" << n_brute
            << " engine=" << pinned::Version() << "\n";
  if (expect_valid >= 0 && n_valid != expect_valid) {
    std::cerr << "STOP: valid " << n_valid << " != expected " << expect_valid
              << "\n";
    return 1;
  }
  if (expect_bytes >= 0 && valid_bytes != expect_bytes) {
    std::cerr << "STOP: valid bytes " << valid_bytes
              << " != expected " << expect_bytes << "\n";
    return 1;
  }
  std::cout << "NATIVE_REPLAY_GATE: PASS\n";
  return 0;
}
