// Parameterised Brotli reference producer/consumer for the I20 dense grid.
//
// Provenance: see PROVENANCE.md in this directory.
//
// Usage:
//   brotli_dense c <quality> <lgwin> <input> <output>
//   brotli_dense d <input> <output> <expected-bytes>
//
// The encode path deliberately uses BrotliEncoderCompress() -- the exact API
// the repo's binding helper tools/brotli_lw.cpp uses -- so that at
// (quality=11, lgwin=30) this helper is byte-identical to the gate's
// brotli_lw arm when both are linked against the same libbrotli build.
//
// lgwin 30 is a REAL large-window stream, not a clamped 24: brotli v1.1.0's
// BrotliEncoderCompress() sets BROTLI_PARAM_LARGE_WINDOW itself whenever
// lgwin > BROTLI_MAX_WINDOW_BITS (24).  See c/enc/encode.c in the pinned
// commit ed738e842d2fbdf2d6459e39267a633c4a9b2f5d.
//
// The decode path mirrors tools/brotli_lw.cpp exactly (including
// BROTLI_DECODER_PARAM_LARGE_WINDOW) so that decode time and decode peak RSS
// are measured with the same producer/consumer geometry as the gate's arm.

#include <brotli/decode.h>
#include <brotli/encode.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<uint8_t> read_file(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot open input");
  f.seekg(0, std::ios::end);
  std::streamoff n = f.tellg();
  f.seekg(0);
  std::vector<uint8_t> d(static_cast<size_t>(n));
  if (n > 0) f.read(reinterpret_cast<char*>(d.data()), n);
  return d;
}

static void write_file(const std::string& path, const std::vector<uint8_t>& d) {
  std::ofstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot open output");
  if (!d.empty()) f.write(reinterpret_cast<const char*>(d.data()), d.size());
}

static std::vector<uint8_t> compress(const std::vector<uint8_t>& in, int quality,
                                     int lgwin) {
  size_t cap = BrotliEncoderMaxCompressedSize(in.size());
  if (!cap) throw std::runtime_error("brotli size bound overflow");
  std::vector<uint8_t> out(cap);
  size_t n = cap;
  const uint8_t* p = in.empty() ? reinterpret_cast<const uint8_t*>("") : in.data();
  if (!BrotliEncoderCompress(quality, lgwin, BROTLI_MODE_GENERIC, in.size(), p, &n,
                             out.data()))
    throw std::runtime_error("brotli encode failed");
  out.resize(n);
  return out;
}

static std::vector<uint8_t> decompress_lw(const std::vector<uint8_t>& in,
                                           size_t expected) {
  BrotliDecoderState* s = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
  if (!s) throw std::runtime_error("decoder allocation failed");
  struct G {
    BrotliDecoderState* p;
    ~G() { BrotliDecoderDestroyInstance(p); }
  } g{s};
  if (!BrotliDecoderSetParameter(s, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1))
    throw std::runtime_error("large-window setup failed");
  std::vector<uint8_t> out(expected + 1);
  size_t ai = in.size(), ao = out.size(), total = 0;
  const uint8_t* ni = in.data();
  uint8_t* no = out.data();
  auto r = BrotliDecoderDecompressStream(s, &ai, &ni, &ao, &no, &total);
  if (r != BROTLI_DECODER_RESULT_SUCCESS || ai || total != expected)
    throw std::runtime_error("brotli decode failed");
  out.resize(expected);
  return out;
}

int main(int argc, char** argv) {
  try {
    if (argc < 5) {
      std::cerr << "usage: brotli_dense c <quality> <lgwin> <input> <output>\n"
                   "       brotli_dense d <input> <output> <expected-bytes>\n";
      return 2;
    }
    std::string op = argv[1];
    if (op == "c") {
      if (argc < 6) return 2;
      int quality = std::stoi(argv[2]);
      int lgwin = std::stoi(argv[3]);
      if (quality < 0 || quality > 11) {
        std::cerr << "brotli_dense: quality must be 0..11\n";
        return 2;
      }
      if (lgwin < 10 || lgwin > 30) {
        std::cerr << "brotli_dense: lgwin must be 10..30\n";
        return 2;
      }
      auto in = read_file(argv[4]);
      write_file(argv[5], compress(in, quality, lgwin));
      return 0;
    }
    if (op == "d") {
      if (argc < 6) return 2;
      auto in = read_file(argv[2]);
      uint64_t n = std::stoull(argv[4]);
      if (n > std::numeric_limits<size_t>::max())
        throw std::runtime_error("output too large");
      write_file(argv[3], decompress_lw(in, static_cast<size_t>(n)));
      return 0;
    }
    return 2;
  } catch (const std::exception& e) {
    std::cerr << "brotli_dense: " << e.what() << "\n";
    return 1;
  }
}
