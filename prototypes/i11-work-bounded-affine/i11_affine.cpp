// ANVIL I11: exact, bounded-work affine/sparse reconstruction pilot.
// Research-only, NOT ANVIL production wire format. All heavy execution: GitHub Actions.
// Wire: "AVI1" | uvar(raw_bytes) | sequence of tagged blocks.
// RAW = 0 | uvar(nbytes) | bytes
// AFF = 1 | width(1/2/4/8) | uvar(count) | base(width LE) | step(width LE)
// PATCH = 2 | AFF header | uvar(nexceptions) | (uvar(gap), value(width LE))*
// Arithmetic is modulo 2^(8*width); patches contain original full values.
// No backward references, variable-sized programs, external dictionaries or entropy coder.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#if defined(I11_WITH_BROTLI)
#include <brotli/encode.h>
#include <brotli/decode.h>
#endif
using Bytes = std::vector<uint8_t>;
namespace fs = std::filesystem;
static constexpr size_t BLOCK_BYTES = 4096;
static constexpr uint64_t MAX_OUTPUT = 1ULL << 28; // research safety limit: 256 MiB

[[noreturn]] static void bad(const char* msg) { throw std::runtime_error(msg); }
static size_t varLen(uint64_t n) { size_t k=1; while(n>=128) { ++k; n>>=7; } return k; }
static void putVar(Bytes& out,uint64_t n) {
  do { uint8_t b=uint8_t(n&127); n>>=7; out.push_back(b|uint8_t(n?128:0)); } while(n);
}
static uint64_t getVar(const Bytes& in,size_t& p) {
  uint64_t value=0;
  for(unsigned i=0;i<10;++i) {
    if(p>=in.size()) bad("truncated varint");
    const uint8_t b=in[p++]; const uint64_t digit=b&127;
    if(i==9 && digit>1) bad("overflow varint");
    value|=(digit<<(7*i));
    if(!(b&128)) return value;
  }
  bad("varint exceeds 10 bytes");
}
static uint64_t maskFor(unsigned w) { return w==8?~uint64_t(0):((uint64_t(1)<<(8*w))-1); }
static uint64_t readWord(const uint8_t* p,unsigned w) {
  uint64_t x=0; for(unsigned k=0;k<w;++k) x|=uint64_t(p[k])<<(8*k); return x;
}
static void putWord(Bytes& b,uint64_t v,unsigned w) {
  for(unsigned k=0;k<w;++k) b.push_back(uint8_t(v>>(8*k)));
}
static void setWord(uint8_t* p,uint64_t v,unsigned w) {
  for(unsigned k=0;k<w;++k) p[k]=uint8_t(v>>(8*k));
}
static uint64_t getWord(const Bytes& in,size_t& p,unsigned w) {
  if(w>in.size()-p) bad("truncated word");
  auto x=readWord(in.data()+p,w); p+=w; return x;
}
static bool validWidth(unsigned w) { return w==1||w==2||w==4||w==8; }
struct Stats { uint64_t raw=0,affine=0,patched=0,exceptions=0; };
static std::pair<uint64_t,size_t> modal(std::vector<uint64_t>& values) {
  if(values.empty())bad("modal needs data");
  std::sort(values.begin(),values.end());
  uint64_t winner=values[0];
  size_t winnerCount=0;
  for(size_t i=0;i<values.size();) {
    size_t j=i+1;
    while(j<values.size()&&values[j]==values[i]) ++j;
    if(j-i>winnerCount) {winnerCount=j-i;winner=values[i];}
    i=j;
  }
  return {winner,winnerCount};
}
static void emitBlock(Bytes& out,const uint8_t* src,size_t n,Stats& stats) {
  Bytes chosen; chosen.reserve(n+12);
  chosen.push_back(0); putVar(chosen,n); chosen.insert(chosen.end(),src,src+n);
  unsigned mode=0; uint64_t winningExceptions=0;
  for(unsigned w : {1u,2u,4u,8u}) {
    if(n%(size_t)w || n<2*w) continue;
    const size_t count=n/w;
    const uint64_t mask=maskFor(w);
    const size_t ceiling=std::min(count/4,chosen.size()/(w+1));
    // Robust, bounded encoder-only inference. Sparse outliers can spoil the
    // first two fields, so select the modal finite difference and modal
    // intercept across the entire independent 4KiB block.
    std::vector<uint64_t> differences;
    differences.reserve(count-1);
    uint64_t prev=readWord(src,w);
    for(size_t i=1;i<count;++i) {
      const uint64_t current=readWord(src+i*w,w);
      differences.push_back((current-prev)&mask);
      prev=current;
    }
    const auto [step,stepSupport]=modal(differences);
    if(stepSupport*2<count-1) continue; // max 25% sparse outliers
    std::vector<uint64_t> intercepts;
    intercepts.reserve(count);
    for(size_t i=0;i<count;++i)
      intercepts.push_back((readWord(src+i*w,w)-uint64_t(i)*step)&mask);
    const auto [base,baseSupport]=modal(intercepts);
    if(count-baseSupport>ceiling) continue;
    std::vector<std::pair<uint64_t,uint64_t>> exceptions;
    // Avoid pathological sparse side-stream size and excessive encoder work.
    uint64_t expected=base;
    for(size_t i=0;i<count;++i) {
      uint64_t actual=readWord(src+i*w,w);
      if(expected!=actual) {
        exceptions.emplace_back(i,actual);
        if(exceptions.size()>ceiling) break;
      }
      expected=(expected+step)&mask;
    }
    if(exceptions.size()>ceiling) continue;
    Bytes candidate; candidate.reserve(2+varLen(count)+2*w+exceptions.size()*(w+2));
    candidate.push_back(exceptions.empty()?1:2);
    candidate.push_back(uint8_t(w));putVar(candidate,count);
    putWord(candidate,base,w);putWord(candidate,step,w);
    if(!exceptions.empty()) {
      putVar(candidate,exceptions.size());
      uint64_t next=0;
      for(auto [pos,val]:exceptions) {
        putVar(candidate,pos-next);putWord(candidate,val,w);
        next=pos+1;
      }
    }
    // Full serialized cost, raw favored on equal bytes.
    if(candidate.size()<chosen.size()) {
      chosen=std::move(candidate);
      mode=exceptions.empty()?1:2;
      winningExceptions=exceptions.size();
    }
  }
  out.insert(out.end(),chosen.begin(),chosen.end());
  if(mode==0)++stats.raw;
  else if(mode==1)++stats.affine;
  else {++stats.patched;stats.exceptions+=winningExceptions;}
}
static Bytes encode(const Bytes& in,Stats& stats) {
  if(in.size()>MAX_OUTPUT) bad("input exceeds research size ceiling");
  Bytes out={'A','V','I','1'};putVar(out,in.size());
  for(size_t p=0;p<in.size();) {
    const size_t n=std::min(BLOCK_BYTES,in.size()-p);
    emitBlock(out,in.data()+p,n,stats);p+=n;
  }
  return out;
}
static Bytes decode(const Bytes& in) {
  if(in.size()<4||in[0]!='A'||in[1]!='V'||in[2]!='I'||in[3]!='1')
    bad("invalid magic");
  size_t p=4;const uint64_t length=getVar(in,p);
  if(length>MAX_OUTPUT) bad("declared output over safety ceiling");
  Bytes out;out.reserve(size_t(length));
  while(out.size()<length) {
    if(p>=in.size())bad("truncated block");
    const unsigned tag=in[p++];
    if(tag==0) {
      uint64_t n=getVar(in,p);
      if(!n||n>length-out.size()||n>in.size()-p)bad("invalid raw length");
      out.insert(out.end(),in.begin()+p,in.begin()+p+size_t(n));p+=size_t(n);
    } else if(tag==1||tag==2) {
      if(p>=in.size())bad("truncated width");
      const unsigned w=in[p++];
      if(!validWidth(w))bad("invalid word width");
      const uint64_t count=getVar(in,p);
      if(count<2||count>(length-out.size())/w)bad("invalid affine count");
      const uint64_t mask=maskFor(w);
      const uint64_t base=getWord(in,p,w),step=getWord(in,p,w);
      const size_t start=out.size();out.resize(start+size_t(count)*w);
      uint64_t value=base;
      for(uint64_t i=0;i<count;++i) {
        setWord(out.data()+start+size_t(i)*w,value,w);
        value=(value+step)&mask;
      }
      if(tag==2) {
        const uint64_t n=getVar(in,p);
        if(n==0||n>=count)bad("invalid exception count");
        uint64_t next=0;
        for(uint64_t k=0;k<n;++k) {
          const uint64_t gap=getVar(in,p);
          if(gap>=count-next)bad("invalid exception gap");
          const uint64_t pos=next+gap;
          const uint64_t actual=getWord(in,p,w);
          setWord(out.data()+start+size_t(pos)*w,actual,w);
          next=pos+1;
        }
      }
    } else bad("unknown block tag");
  }
  if(p!=in.size())bad("trailing bytes");
  return out;
}
static Bytes readFile(const fs::path& f) {
  std::ifstream is(f,std::ios::binary|std::ios::ate);
  if(!is)bad("cannot open input");
  auto end=is.tellg();if(end<0||uint64_t(end)>MAX_OUTPUT+1024*1024)bad("input size invalid");
  Bytes b(size_t(end),0);is.seekg(0);
  if(!b.empty()&&!is.read(reinterpret_cast<char*>(b.data()),std::streamsize(b.size())))
    bad("short read");
  return b;
}
static void writeFile(const fs::path& f,const Bytes& b) {
  std::ofstream os(f,std::ios::binary|std::ios::trunc);
  if(!os)bad("cannot create output");
  if(!b.empty())os.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));
  if(!os)bad("short write");
}
static void expect(bool value,const char* msg) {if(!value)bad(msg);}
static void selftest() {
  std::vector<Bytes> cases;
  cases.push_back({});
  cases.push_back(Bytes(1,5));
  cases.push_back(Bytes(8192,0));
  Bytes affine(8192),sparse(8192),random(8192);
  for(size_t i=0;i<affine.size()/4;++i) {
    const uint32_t x=uint32_t(19+71*i);
    setWord(affine.data()+4*i,x,4);
  }
  sparse=affine;sparse[37]=0x77;sparse[5123]=0x42;
  uint64_t s=0x9e3779b97f4a7c15ULL;
  for(auto &b:random){s^=s<<13;s^=s>>7;s^=s<<17;b=uint8_t(s);}
  cases.push_back(affine);cases.push_back(sparse);cases.push_back(random);
  for(size_t n=2;n<27;++n) {
    Bytes t(n);for(size_t j=0;j<n;++j)t[j]=uint8_t(j*j+13*n);
    cases.push_back(std::move(t));
  }
  for(const auto& c:cases) {
    Stats st;auto encoded=encode(c,st);auto decoded=decode(encoded);
    expect(c==decoded,"roundtrip mismatch");
    if(c.size()>=BLOCK_BYTES && c.size()%BLOCK_BYTES==0)
      expect(encoded.size()<=c.size()+5+varLen(c.size())+c.size()/BLOCK_BYTES*3,
        "raw-bound guarantee broken");
    if(encoded.size()>1) {
      auto trunc=encoded;trunc.pop_back();
      bool rejected=false;try{(void)decode(trunc);}catch(const std::exception&){rejected=true;}
      expect(rejected,"truncated data accepted");
    }
  }
  Stats st;auto wire=encode(affine,st);
  expect(st.affine>0,"affine detector did not trigger");
  expect(wire.size()<affine.size()/4,"affine size improvement absent");
  Stats sparseStats;auto sparseWire=encode(sparse,sparseStats);
  expect(sparseStats.patched>0,"sparse exceptions were not represented");
  expect(decode(sparseWire)==sparse,"sparse roundtrip failed");
  // Malicious declaration and invalid syntax must reject before allocation.
  for(Bytes badWire: {Bytes{'N','O','P','E'},Bytes{'A','V','I','1',0,0},Bytes{'A','V','I','1',0x80}}) {
    bool rejected=false;try{(void)decode(badWire);}catch(const std::exception&){rejected=true;}
    expect(rejected,"malformed wire accepted");
  }
  std::cout<<"SELFTEST PASS cases="<<cases.size()<<" affine_bytes="<<wire.size()<<"\n";
}
// Consume every reconstructed byte in every timed decode. This shared digest
// prevents the compiler from eliding unused reconstruction, unlike size-only
// sinks. Timings include equal digest work on all reference decoders.
static uint64_t observedDigest(const Bytes& bytes) {
  uint64_t h=0xcbf29ce484222325ULL;
  size_t i=0;
  for(;i+8<=bytes.size();i+=8) {
    h=(h<<9)|(h>>55);
    h^=readWord(bytes.data()+i,8)*0x9e3779b185ebca87ULL;
  }
  for(;i<bytes.size();++i) {
    h=(h<<9)|(h>>55);
    h^=uint64_t(bytes[i])*0x9e3779b185ebca87ULL;
  }
  return h;
template<class F>static double medianMicros(F f,int reps=7) {
  using clock=std::chrono::steady_clock;
  f();std::vector<double> v;v.reserve(reps);
  for(int i=0;i<reps;++i) {
    auto start=clock::now();f();auto end=clock::now();
    v.push_back(std::chrono::duration<double,std::micro>(end-start).count());
  }
  std::sort(v.begin(),v.end());
  return std::max(v[v.size()/2],0.001);
}
#if defined(I11_WITH_BROTLI)
static Bytes brotliEncode(const Bytes& raw,int quality) {
  size_t capacity=BrotliEncoderMaxCompressedSize(raw.size());
  if(capacity==0) bad("Brotli bound failed");
  Bytes compressed(capacity);
  size_t encoded=compressed.size();
  if(!BrotliEncoderCompress(quality,22,BROTLI_MODE_GENERIC,
      raw.size(),raw.data(),&encoded,compressed.data())) bad("Brotli encode failed");
  compressed.resize(encoded);
  return compressed;
}
static Bytes brotliDecode(const Bytes& wire,size_t length) {
  Bytes decoded(length);
  size_t n=decoded.size();
  if(BrotliDecoderDecompress(wire.size(),wire.data(),&n,decoded.data())
      !=BROTLI_DECODER_RESULT_SUCCESS||n!=length)bad("Brotli decode failed");
  return decoded;
}
#endif
static void bench(const fs::path& f) {
  auto raw=readFile(f);Stats stats;auto wire=encode(raw,stats);
  expect(decode(wire)==raw,"bench roundtrip failed");
  volatile uint64_t sink=0;
  const double enc=medianMicros([&]{Stats st;auto z=encode(raw,st);sink=sink^z.size();},5);
  const double dec=medianMicros([&]{auto z=decode(wire);sink=sink^observedDigest(z);},7);
  const double encMBs=raw.empty()?0:double(raw.size())/enc;
  const double decMBs=raw.empty()?0:double(raw.size())/dec;
  std::cout<<f.filename().string()<<'\t'<<raw.size()<<'\t'<<wire.size()<<'\t'
           <<stats.raw<<'\t'<<stats.affine<<'\t'<<stats.patched<<'\t'
           <<stats.exceptions<<'\t'<<std::fixed<<std::setprecision(3)
           <<encMBs<<'\t'<<decMBs;
#if defined(I11_WITH_BROTLI)
  for(int quality:{5,11}) {
    const auto reference=brotliEncode(raw,quality);
    expect(brotliDecode(reference,raw.size())==raw,"Brotli roundtrip mismatch");
    const double referenceEnc=medianMicros([&] {
      const auto z=brotliEncode(raw,quality);sink=sink^z.size();
    },quality==11?3:5);
    const double referenceDec=medianMicros([&] {
      const auto z=brotliDecode(reference,raw.size());sink=sink^observedDigest(z);
    },7);
    std::cout<<'\t'<<reference.size()
             <<'\t'<<(raw.empty()?0:double(raw.size())/referenceEnc)
             <<'\t'<<(raw.empty()?0:double(raw.size())/referenceDec);
  }
#endif
  std::cout<<'\n';
  (void)sink;
}
int main(int argc,char** argv) {
  try {
    if(argc==2&&std::string(argv[1])=="--selftest") {selftest();return 0;}
    if(argc==4&&std::string(argv[1])=="--encode") {
      Stats stats;auto wire=encode(readFile(argv[2]),stats);writeFile(argv[3],wire);return 0;
    }
    if(argc==4&&std::string(argv[1])=="--decode") {
      writeFile(argv[3],decode(readFile(argv[2])));return 0;
    }
    if(argc>=3&&std::string(argv[1])=="--bench") {
      std::cout<<"file\tinput_bytes\twire_bytes\traw_blocks\taffine_blocks\tpatched_blocks\texceptions\tencode_MBps\tdecode_MBps";
#if defined(I11_WITH_BROTLI)
      std::cout<<"\tbrotli_q5_bytes\tbrotli_q5_encode_MBps\tbrotli_q5_decode_MBps"
                 "\tbrotli_q11_bytes\tbrotli_q11_encode_MBps\tbrotli_q11_decode_MBps";
#endif
      std::cout<<'\n';
      for(int i=2;i<argc;++i)bench(argv[i]);return 0;
    }
    std::cerr<<"usage: i11_affine --selftest | --encode IN OUT | --decode IN OUT | --bench FILE...\n";
    return 2;
  }catch(const std::exception& e) {std::cerr<<"I11_FAIL "<<e.what()<<'\n';return 1;}
}
