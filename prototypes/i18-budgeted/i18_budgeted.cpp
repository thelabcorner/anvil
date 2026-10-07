// I18 preregistered budget-gated experimental AVH2 wire.
// All CPU-intensive compilation, tests and measurements: GitHub Actions only.
// Source-pinned original AVH1/I17 controls are included without modification.
#define main i17_unchanged_command_main
#include "../i17-fast/i17_fast.cpp"
#undef main

// The entire exact-byte AVI6 I16 fusion source is isolated from I14/I17
// global/static identifiers, preserving independent AVI6 encode and decode.
#define main i16_frozen_command_main
namespace fusion {
#include "i16_fusion_frozen.inc"
}
#undef main

static constexpr std::array<uint8_t,4> I18_MAGIC{'A','V','H','2'};
static constexpr uint8_t I18_RAW=0, I18_AVI4=1, I18_BROTLI=2, I18_AVI6=4;

struct I18Stats {
  uint8_t mode=I18_RAW;
  bool probed=false;
  bool regular=false;
  bool skippedByRatio=false;
  unsigned mathematicalCandidates=0;
  size_t rawBytes=0, q5Bytes=0, avi4Bytes=0, avi6Bytes=0;
};

// A bounded rejection filter, not a correctness assumption or an oracle.
// At most 12 * 4 * 32 strided word differences are evaluated.
static bool i18NumericProbe(const Bytes& raw) {
  const size_t words=raw.size()/4;
  if(words<128)return false;
  for(size_t stride=1;stride<=12;++stride) {
    for(size_t lane=0;lane<std::min<size_t>(stride,4);++lane) {
      if(words<=lane+24*stride)continue;
      const size_t pairs=std::min<size_t>(32,(words-1-lane)/stride);
      uint32_t prev=uint32_t(word(raw.data()+4*lane,4));
      size_t small=0;
      for(size_t k=1;k<=pairs;++k) {
        const uint32_t now=uint32_t(word(raw.data()+4*(lane+k*stride),4));
        const uint32_t delta=now-prev;
        small+=delta<=4096u || delta>=uint32_t(0xfffff000u);
        prev=now;
      }
      if(small*8>=pairs*7)return true;
    }
  }
  return false;
}

static Bytes i18Encode(const Bytes& raw,I18Stats& stats) {
  if(raw.size()>OUTPUT_LIMIT)fail("I18 source above hard cap");
  stats=I18Stats{};
  stats.rawBytes=raw.size();
  Bytes q5;
  if(!raw.empty())q5=brotliEncode(raw,5);
  stats.q5Bytes=q5.size();

  uint8_t mode=I18_RAW;
  const Bytes* best=&raw;
  if(!raw.empty()&&q5.size()<best->size()) {
    mode=I18_BROTLI;best=&q5;
  }
  Bytes avi4,avi6;
  // Always reject when the q5 carrier is already extremely compact.
  // Integer division is avoided and overflow cannot occur at OUTPUT_LIMIT.
  stats.skippedByRatio=!raw.empty() && q5.size()<=raw.size()/8;
  if(!stats.skippedByRatio && raw.size()>=512) {
    stats.probed=true;
    stats.regular=i18NumericProbe(raw);
    if(stats.regular) {
      Stats numericalStats;
      avi4=encode(raw,numericalStats);
      ++stats.mathematicalCandidates;
      stats.avi4Bytes=avi4.size();
      if(avi4.size()<best->size()){mode=I18_AVI4;best=&avi4;}
      fusion::Stats fusionStats;
      avi6=fusion::encode(raw,fusionStats);
      ++stats.mathematicalCandidates;
      stats.avi6Bytes=avi6.size();
      if(avi6.size()<best->size()){mode=I18_AVI6;best=&avi6;}
    }
  }
  Bytes wire;
  wire.reserve(5+varSize(raw.size())+best->size());
  wire.insert(wire.end(),I18_MAGIC.begin(),I18_MAGIC.end());
  putVar(wire,raw.size());
  wire.push_back(mode);
  wire.insert(wire.end(),best->begin(),best->end());
  stats.mode=mode;
  return wire;
}

static Bytes i18Decode(const Bytes& wire) {
  if(wire.size()<6 || !std::equal(I18_MAGIC.begin(),I18_MAGIC.end(),wire.begin()))
    fail("I18 invalid magic");
  size_t p=4;
  const auto n=getVar(wire,p);
  if(n>OUTPUT_LIMIT)fail("I18 output cap");
  if(p>=wire.size())fail("I18 missing mode");
  const auto mode=wire[p++];
  if(p>wire.size())fail("I18 payload position");
  if(mode==I18_RAW) {
    if(wire.size()-p!=n)fail("I18 RAW length mismatch");
    return Bytes(wire.begin()+p,wire.end());
  }
  if(p==wire.size())fail("I18 nonraw empty payload");
  const Bytes payload(wire.begin()+p,wire.end());
  Bytes decoded;
  switch(mode) {
    case I18_AVI4:decoded=decode(payload);break;
    case I18_AVI6:decoded=fusion::decode(payload);break;
    case I18_BROTLI:decoded=brotliDecode(payload,size_t(n));break;
    default:fail("I18 invalid mode");
  }
  if(decoded.size()!=n)fail("I18 decoded size mismatch");
  return decoded;
}

static void i18Selftest() {
  // Invariant cases, byte-truncation rejection, and independently valid AVI6.
  std::vector<Bytes> cases;
  for(size_t n:{0u,1u,2u,7u,15u,31u,128u,511u,512u,4095u,4096u,4097u,8192u})
    cases.push_back(Bytes(n,uint8_t(n)));
  Bytes walk(8192);
  uint32_t value=0xfffff000u;
  for(size_t i=0;i<walk.size()/4;++i) {
    value+=uint32_t(32+int(i%3)-1);
    setWord(walk.data()+4*i,value,4);
  }
  cases.push_back(walk);
  for(const auto& raw:cases) {
    I18Stats s;
    const auto newWire=i18Encode(raw,s);
    expect(i18Decode(newWire)==raw,"I18 exact-roundtrip failure");
    HybridStats control;
    expect(hybridDecode(fastEncode(raw,control))==raw,"I17 control decode");
    expect(newWire.size()==5+varSize(raw.size())+
      (s.mode==I18_RAW?raw.size():
       s.mode==I18_BROTLI?s.q5Bytes:
       s.mode==I18_AVI4?s.avi4Bytes:s.avi6Bytes),
       "I18 exact-complete-byte oracle");
    if(!newWire.empty()) {
      Bytes broken=newWire;broken.pop_back();
      bool rejected=false;
      try{(void)i18Decode(broken);}catch(const std::exception&){rejected=true;}
      expect(rejected,"I18 accepted truncated wire");
    }
  }
  fusion::selftest();
  fastSelftest();
  const std::vector<Bytes> invalid{
    Bytes{'B','A','D','2',0,0},
    Bytes{'A','V','H','2',0x80},
    Bytes{'A','V','H','2',0,9},
    Bytes{'A','V','H','2',0,0,1},
    Bytes{'A','V','H','2',0x80,0,0}
  };
  for(const auto& b:invalid) {
    bool rejected=false;
    try{(void)i18Decode(b);}catch(const std::exception&){rejected=true;}
    expect(rejected,"I18 invalid wire accepted");
  }
  std::cout<<"I18_SELFTEST PASS cases="<<cases.size()<<" invalid="<<invalid.size()<<"\n";
}

static void i18Bench(const fs::path& path) {
  const Bytes raw=readFile(path);
  I18Stats s;
  const Bytes budget=i18Encode(raw,s);
  HybridStats fastS,fullS;
  const Bytes fast=fastEncode(raw,fastS);
  const Bytes full=hybridEncode(raw,fullS);
  expect(i18Decode(budget)==raw,"I18 benchmark inverse");
  expect(hybridDecode(fast)==raw&&hybridDecode(full)==raw,"I17 benchmark inverses");
  volatile uint64_t sink=0;
  const auto eBudget=medianMicros([&] {
    I18Stats st;const auto v=i18Encode(raw,st);sink=sink^observedDigest(v);
  },5);
  const auto eFast=medianMicros([&] {
    HybridStats st;const auto v=fastEncode(raw,st);sink=sink^observedDigest(v);
  },5);
  const auto eFull=medianMicros([&] {
    HybridStats st;const auto v=hybridEncode(raw,st);sink=sink^observedDigest(v);
  },3);
  const auto dBudget=medianMicros([&] {
    const auto v=i18Decode(budget);sink=sink^observedDigest(v);
  },7);
  const auto dFast=medianMicros([&] {
    const auto v=hybridDecode(fast);sink=sink^observedDigest(v);
  },7);
  const auto dFull=medianMicros([&] {
    const auto v=hybridDecode(full);sink=sink^observedDigest(v);
  },7);
  const Bytes q5=brotliEncode(raw,5),q11=brotliEncode(raw,11);
  expect(brotliDecode(q5,raw.size())==raw && brotliDecode(q11,raw.size())==raw,
         "I18 reference inverse");
  const auto eQ5=medianMicros([&] {
    const auto v=brotliEncode(raw,5);sink=sink^observedDigest(v);
  },5);
  const auto dQ5=medianMicros([&] {
    const auto v=brotliDecode(q5,raw.size());sink=sink^observedDigest(v);
  },7);
  const auto dQ11=medianMicros([&] {
    const auto v=brotliDecode(q11,raw.size());sink=sink^observedDigest(v);
  },7);
  const auto speed=[&](double micros) {
    return raw.empty()?0.0:double(raw.size())/micros;
  };
  std::cout<<path.filename().string()<<'\t'<<raw.size()<<'\t'
    <<budget.size()<<'\t'<<fast.size()<<'\t'<<full.size()<<'\t'
    <<q5.size()<<'\t'<<q11.size()<<'\t'<<unsigned(s.mode)<<'\t'
    <<unsigned(s.probed)<<'\t'<<unsigned(s.regular)<<'\t'
    <<unsigned(s.skippedByRatio)<<'\t'<<s.mathematicalCandidates<<'\t'
    <<s.avi4Bytes<<'\t'<<s.avi6Bytes<<'\t'
    <<std::fixed<<std::setprecision(3)
    <<speed(eBudget)<<'\t'<<speed(eFast)<<'\t'<<speed(eFull)<<'\t'
    <<speed(eQ5)<<'\t'
    <<speed(dBudget)<<'\t'<<speed(dFast)<<'\t'<<speed(dFull)<<'\t'
    <<speed(dQ5)<<'\t'<<speed(dQ11)<<'\n';
  (void)sink;
}

int main(int argc,char** argv) {
  try {
    if(argc==2&&std::string(argv[1])=="--selftest") {
      i18Selftest();return 0;
    }
    if(argc==4&&std::string(argv[1])=="--encode-budget") {
      I18Stats stats;writeFile(argv[3],i18Encode(readFile(argv[2]),stats));return 0;
    }
    if(argc==4&&std::string(argv[1])=="--encode-fast") {
      HybridStats stats;writeFile(argv[3],fastEncode(readFile(argv[2]),stats));return 0;
    }
    if(argc==4&&std::string(argv[1])=="--encode-full") {
      HybridStats stats;writeFile(argv[3],hybridEncode(readFile(argv[2]),stats));return 0;
    }
    if(argc==4&&std::string(argv[1])=="--decode-budget") {
      writeFile(argv[3],i18Decode(readFile(argv[2])));return 0;
    }
    if(argc==4&&std::string(argv[1])=="--decode-control") {
      writeFile(argv[3],hybridDecode(readFile(argv[2])));return 0;
    }
    if(argc>=3&&std::string(argv[1])=="--bench") {
      std::cout<<"file\tinput_bytes\tbudget_bytes\tfast_bytes\tfull_bytes"
        <<"\tq5_bytes\tq11_bytes\tbudget_mode\tprobed\tregular"
        <<"\tskipped_by_ratio\tmath_candidate_calls\tavi4_bytes\tavi6_bytes"
        <<"\tbudget_encode_MBps\tfast_encode_MBps\tfull_encode_MBps"
        <<"\tq5_encode_MBps\tbudget_decode_MBps\tfast_decode_MBps"
        <<"\tfull_decode_MBps\tq5_decode_MBps\tq11_decode_MBps\n";
      for(int i=2;i<argc;++i)i18Bench(argv[i]);
      return 0;
    }
    std::cerr<<"I18: --selftest | --encode-budget IN OUT | --encode-fast IN OUT | --encode-full IN OUT | --decode-budget IN OUT | --decode-control IN OUT | --bench FILE...\n";
    return 2;
  }catch(const std::exception& e){std::cerr<<"I18_FAIL "<<e.what()<<"\n";return 1;}
}
