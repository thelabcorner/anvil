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
  f.seekg(0, std::ios::end); auto n=f.tellg(); f.seekg(0);
  std::vector<uint8_t> d(static_cast<size_t>(n));
  if (n>0) f.read(reinterpret_cast<char*>(d.data()), n);
  return d;
}

static void write_file(const std::string& path, const std::vector<uint8_t>& d) {
  std::ofstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot open output");
  if (!d.empty()) f.write(reinterpret_cast<const char*>(d.data()), d.size());
}

static std::vector<uint8_t> compress_q11_lw30(const std::vector<uint8_t>& in) {
  size_t cap=BrotliEncoderMaxCompressedSize(in.size());
  if (!cap) throw std::runtime_error("brotli size bound overflow");
  std::vector<uint8_t> out(cap); size_t n=cap;
  const uint8_t* p=in.empty()?reinterpret_cast<const uint8_t*>(""):in.data();
  if (!BrotliEncoderCompress(11, 30, BROTLI_MODE_GENERIC, in.size(), p, &n, out.data()))
    throw std::runtime_error("brotli encode failed");
  out.resize(n); return out;
}

static std::vector<uint8_t> decompress_lw(const std::vector<uint8_t>& in, size_t expected) {
  BrotliDecoderState* s=BrotliDecoderCreateInstance(nullptr,nullptr,nullptr);
  if (!s) throw std::runtime_error("decoder allocation failed");
  struct G { BrotliDecoderState* p; ~G(){BrotliDecoderDestroyInstance(p);} } g{s};
  if (!BrotliDecoderSetParameter(s,BROTLI_DECODER_PARAM_LARGE_WINDOW,1))
    throw std::runtime_error("large-window setup failed");
  std::vector<uint8_t> out(expected+1);
  size_t ai=in.size(), ao=out.size(), total=0; const uint8_t* ni=in.data(); uint8_t* no=out.data();
  auto r=BrotliDecoderDecompressStream(s,&ai,&ni,&ao,&no,&total);
  if (r!=BROTLI_DECODER_RESULT_SUCCESS || ai || total!=expected) throw std::runtime_error("brotli decode failed");
  out.resize(expected); return out;
}

int main(int argc,char**argv) {
  try {
    if(argc<4){ std::cerr<<"usage: brotli_lw c <input> <output> | brotli_lw d <input> <output> <expected-bytes>\n"; return 2; }
    std::string op=argv[1]; auto in=read_file(argv[2]);
    if(op=="c") write_file(argv[3],compress_q11_lw30(in));
    else if(op=="d") {
      if(argc<5) return 2;
      uint64_t n=std::stoull(argv[4]); if(n>std::numeric_limits<size_t>::max()) throw std::runtime_error("output too large");
      write_file(argv[3],decompress_lw(in,static_cast<size_t>(n)));
    } else return 2;
    return 0;
  } catch(const std::exception& e) { std::cerr<<"brotli_lw: "<<e.what()<<"\n"; return 1; }
}
