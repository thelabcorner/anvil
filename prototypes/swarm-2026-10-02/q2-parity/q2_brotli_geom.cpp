// Q2 reference geometry helper -- isolated prototype, not wired to production.
//
// Two jobs, both descriptive / non-classifying:
//
//   R1 : one whole-file Brotli q11 stream at an EXPLICIT lgwin (never a library default;
//        BROTLI_DEFAULT_WINDOW is the confound this job exists to avoid).
//   R3 : the same, split into ceil(n/block) INDEPENDENT streams. This prices the generic
//        256 KiB fragmentation tax that any LZ codec pays, so ANVIL's own geometry
//        movement can be compared against a non-zero denominator.
//
// lgwin must be explicit on every invocation. R3's container overhead is DECLARED and
// reported separately (payload sums vs container bytes) so our own framing is never
// silently charged to, or credited from, the codec under test.
//
// Build:  c++ -O2 -std=c++17 q2_brotli_geom.cpp -lbrotlienc -lbrotlidec -o q2_brotli_geom

#include <brotli/decode.h>
#include <brotli/encode.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr char kMagic[4] = {'Q', '2', 'B', 'G'};
constexpr uint8_t kVersion = 1;

std::vector<uint8_t> read_file(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot open input: " + path);
  f.seekg(0, std::ios::end);
  auto n = f.tellg();
  f.seekg(0);
  if (n < 0) throw std::runtime_error("cannot size input: " + path);
  std::vector<uint8_t> d(static_cast<size_t>(n));
  if (n > 0) f.read(reinterpret_cast<char*>(d.data()), n);
  if (!f && n > 0) throw std::runtime_error("short read: " + path);
  return d;
}

void write_file(const std::string& path, const std::vector<uint8_t>& d) {
  std::ofstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot open output: " + path);
  if (!d.empty()) f.write(reinterpret_cast<const char*>(d.data()),
                          static_cast<std::streamsize>(d.size()));
  if (!f) throw std::runtime_error("short write: " + path);
}

void put_uvar(std::vector<uint8_t>& out, uint64_t v) {
  while (v >= 0x80) {
    out.push_back(static_cast<uint8_t>(v) | 0x80u);
    v >>= 7;
  }
  out.push_back(static_cast<uint8_t>(v));
}

uint64_t get_uvar(const uint8_t*& p, const uint8_t* e) {
  uint64_t v = 0;
  unsigned shift = 0;
  while (true) {
    if (p >= e) throw std::runtime_error("truncated varint");
    uint8_t b = *p++;
    v |= static_cast<uint64_t>(b & 0x7Fu) << shift;
    if (!(b & 0x80u)) break;
    shift += 7;
    if (shift > 63) throw std::runtime_error("varint overflow");
  }
  return v;
}

std::vector<uint8_t> compress_stream(const std::vector<uint8_t>& in, int q, int lgwin) {
  if (lgwin < 10 || lgwin > 30) throw std::runtime_error("lgwin must be 10..30");
  size_t cap = BrotliEncoderMaxCompressedSize(in.size());
  if (!cap && !in.empty()) throw std::runtime_error("brotli size bound overflow");
  std::vector<uint8_t> out(cap ? cap : 1);
  size_t n = out.size();
  const uint8_t* p = in.empty() ? reinterpret_cast<const uint8_t*>("") : in.data();
  if (!BrotliEncoderCompress(q, lgwin, BROTLI_MODE_GENERIC, in.size(), p, &n, out.data()))
    throw std::runtime_error("brotli encode failed");
  out.resize(n);
  return out;
}

std::vector<uint8_t> decompress_stream(const std::vector<uint8_t>& in, size_t expected,
                                       int lgwin) {
  BrotliDecoderState* s = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
  if (!s) throw std::runtime_error("decoder allocation failed");
  struct Guard {
    BrotliDecoderState* p;
    ~Guard() { BrotliDecoderDestroyInstance(p); }
  } g{s};
  if (lgwin > 24) {
    // Large-window streams need the decoder flag; lgwin <= 24 must NOT set it, so the
    // R1 (lgwin 30) and R3 paths are the only ones that take this branch.
    if (!BrotliDecoderSetParameter(s, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1))
      throw std::runtime_error("large-window setup failed");
  }
  std::vector<uint8_t> out(expected + 1);
  size_t ai = in.size(), ao = out.size(), total = 0;
  const uint8_t* ni = in.data();
  uint8_t* no = out.data();
  auto r = BrotliDecoderDecompressStream(s, &ai, &ni, &ao, &no, &total);
  if (r != BROTLI_DECODER_RESULT_SUCCESS || ai != 0 || total != expected)
    throw std::runtime_error("brotli decode failed");
  out.resize(expected);
  return out;
}

std::string arg_value(int argc, char** argv, const std::string& name,
                      const std::string& fallback) {
  std::string prefix = "--" + name + "=";
  for (int i = 3; i < argc; ++i) {
    if (std::strncmp(argv[i], prefix.c_str(), prefix.size()) == 0)
      return std::string(argv[i] + prefix.size());
  }
  return fallback;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 4) {
      std::cerr << "usage: q2_brotli_geom c <in> <out> --q=N --lgwin=N --streams=N "
                   "[--block=N]\n"
                   "       q2_brotli_geom d <in> <out> <expected-bytes> --lgwin=N\n"
                   "--lgwin is REQUIRED; there is no library default.\n"
                   "--block is required when --streams>1 (it is the per-stream chunk size).\n"
                   "There is NO payload side-file. Per-stream payload sums are reported on\n"
                   "stderr as: payload_sum_bytes=<N> framing_overhead_bytes=<N>\n"
                   "That stderr line is the single authoritative source for payload sums;\n"
                   "the collector parses it and does not expect any output file.\n";
      return 2;
    }
    const std::string op = argv[1];
    const std::string in_path = argv[2];
    const std::string out_path = argv[3];
    const int q = std::atoi(arg_value(argc, argv, "q", "11").c_str());
    const int lgwin = std::atoi(arg_value(argc, argv, "lgwin", "0").c_str());
    if (lgwin == 0) throw std::runtime_error("--lgwin=N is required; never a library default");
    const long long streams_req = std::atoll(arg_value(argc, argv, "streams", "1").c_str());
    const size_t block = static_cast<size_t>(
        std::atoll(arg_value(argc, argv, "block", "0").c_str()));

    const std::vector<uint8_t> src = read_file(in_path);

    if (op == "c") {
      if (streams_req <= 1) {
        // Single whole-file stream. NO container, so R1 bytes are directly comparable
        // with published whole-file Brotli totals (validity gate G-F).
        const std::vector<uint8_t> payload = compress_stream(src, q, lgwin);
        write_file(out_path, payload);
        // Same stderr contract as the split path, so the collector has ONE parser.
        std::cerr << "q2_brotli_geom streams=1 payload_sum_bytes=" << payload.size()
                  << " container_bytes=" << payload.size()
                  << " framing_overhead_bytes=0\n";
      } else {
        if (block == 0) throw std::runtime_error("--block=N required when --streams>1");
        const size_t nseg =
            (src.size() + block - 1) / block;  // final stream may be short
        std::vector<uint8_t> out;
        out.insert(out.end(), kMagic, kMagic + 4);
        out.push_back(kVersion);
        put_uvar(out, static_cast<uint64_t>(nseg));
        put_uvar(out, static_cast<uint64_t>(block));
        out.push_back(static_cast<uint8_t>(lgwin));
        out.push_back(static_cast<uint8_t>(q));
        uint64_t payload_sum = 0;
        for (size_t i = 0; i < nseg; ++i) {
          const size_t off = i * block;
          const size_t len = (src.size() - off < block) ? (src.size() - off) : block;
          std::vector<uint8_t> chunk(src.begin() + static_cast<long>(off),
                                     src.begin() + static_cast<long>(off + len));
          const std::vector<uint8_t> payload = compress_stream(chunk, q, lgwin);
          put_uvar(out, payload.size());
          out.insert(out.end(), payload.begin(), payload.end());
          payload_sum += payload.size();
        }
        write_file(out_path, out);
        // Declared, reported separately: our container overhead is never charged to the
        // codec, and the framing-free tax is computable as payload_sum - R1_bytes.
        // This stderr line is the ONLY channel for payload sums; there is no side-file.
        std::cerr << "q2_brotli_geom streams=" << nseg << " payload_sum_bytes=" << payload_sum
                  << " container_bytes=" << out.size()
                  << " framing_overhead_bytes=" << (out.size() - payload_sum) << "\n";
      }
      return 0;
    }

    if (op == "d") {
      if (argc < 5) throw std::runtime_error("decode needs <expected-bytes>");
      const size_t expected = static_cast<size_t>(std::stoull(argv[4]));
      const std::vector<uint8_t> in = read_file(in_path);
      if (in.size() >= 4 && std::memcmp(in.data(), kMagic, 4) == 0) {
        const uint8_t* p = in.data() + 4;
        const uint8_t* e = in.data() + in.size();
        if (p >= e || *p++ != kVersion) throw std::runtime_error("bad container version");
        const uint64_t nseg = get_uvar(p, e);
        const uint64_t blk = get_uvar(p, e);
        if (p >= e) throw std::runtime_error("truncated container header");
        const int clgwin = *p++;
        const int cq = *p++;
        std::vector<uint8_t> out;
        out.reserve(expected);
        for (uint64_t i = 0; i < nseg; ++i) {
          const uint64_t len = get_uvar(p, e);
          if (len > static_cast<uint64_t>(e - p)) throw std::runtime_error("truncated payload");
          std::vector<uint8_t> payload(p, p + len);
          p += len;
          const size_t off = out.size();
          const size_t seglen =
              (expected - off < blk) ? (expected - off) : static_cast<size_t>(blk);
          const std::vector<uint8_t> chunk = decompress_stream(payload, seglen, clgwin);
          out.insert(out.end(), chunk.begin(), chunk.end());
        }
        if (out.size() != expected) throw std::runtime_error("decoded size mismatch");
        if (p != e) throw std::runtime_error("trailing bytes in container");
        (void)cq;
        write_file(out_path, out);
      } else {
        write_file(out_path, decompress_stream(in, expected, lgwin));
      }
      return 0;
    }

    std::cerr << "q2_brotli_geom: unknown op\n";
    return 2;
  } catch (const std::exception& e) {
    std::cerr << "q2_brotli_geom: " << e.what() << "\n";
    return 1;
  }
}
