// ANVIL I17: work-aware fast compression profile of source-pinned I16.
// This only tests a portfolio operating point; there is no mechanism novelty.
// Compilation and benchmarks: pinned GitHub Actions ONLY.
#define main i16_frozen_command_main
#include "../i16-envelope/i16_envelope.cpp"
#undef main

static Bytes fastEncode(const Bytes& source,HybridStats& stats) {
  if(source.size()>OUTPUT_LIMIT)fail("fast source size limit");
  Stats predictionStats;
  const Bytes numeric=encode(source,predictionStats);
  Bytes quick;
  if(!source.empty())quick=brotliEncode(source,5);
  const bool chooseQuick=!source.empty() && quick.size()<numeric.size();
  const Bytes& selected=chooseQuick?quick:numeric;
  Bytes result;
  result.reserve(5+varSize(source.size())+selected.size());
  result.insert(result.end(),HYBRID_MAGIC.begin(),HYBRID_MAGIC.end());
  putVar(result,source.size());
  result.push_back(chooseQuick?MODE_BROTLI_Q5:MODE_AVI4);
  result.insert(result.end(),selected.begin(),selected.end());
  stats={uint8_t(chooseQuick?MODE_BROTLI_Q5:MODE_AVI4),
         predictionStats.model,predictionStats.diff,numeric.size(),quick.size(),0};
  return result;
}

static void fastSelftest() {
  std::vector<Bytes> corpus;
  for(size_t n:{0u,1u,2u,7u,31u,128u,4095u,4096u,4097u,8192u})
    corpus.push_back(Bytes(n,uint8_t(n)));
  Bytes arithmetic(8192),rawRepetition(8192),incompressible(8192);
  uint64_t cur=0xffffed11ULL;
  uint64_t seed=0x37c92d4a91f6b8e5ULL;
  for(size_t i=0;i<arithmetic.size()/4;++i) {
    seed^=seed<<13;seed^=seed>>7;seed^=seed<<17;
    cur+=uint64_t(32+int(seed%3)-1);
    setWord(arithmetic.data()+i*4,cur,4);
  }
  for(size_t i=0;i<rawRepetition.size();++i)
    rawRepetition[i]=uint8_t("anvil-fast-speed-envelope."[i%26]);
  for(auto& c:incompressible) {
    seed^=seed<<13;seed^=seed>>7;seed^=seed<<17;c=uint8_t(seed);
  }
  corpus.push_back(arithmetic);
  corpus.push_back(rawRepetition);
  corpus.push_back(incompressible);
  size_t predicted=0,quick=0;
  for(const auto& raw:corpus) {
    HybridStats fastStats,fullStats;
    const Bytes fast=fastEncode(raw,fastStats);
    const Bytes full=hybridEncode(raw,fullStats);
    expect(fastStats.mode!=MODE_BROTLI_Q11,"q11 visited fast mode");
    expect(hybridDecode(fast)==raw,"fast roundtrip");
    expect(hybridDecode(full)==raw,"full roundtrip");
    const size_t envelope=5+varSize(raw.size());
    const size_t expected=envelope+
      (raw.empty()?fastStats.i14Bytes:std::min(fastStats.i14Bytes,fastStats.q5Bytes));
    expect(fast.size()==expected,"fast complete-byte oracle failed");
    expect(fast.size()>=full.size(),"fast undercut full-size oracle");
    if(fastStats.mode==MODE_AVI4 && !raw.empty())++predicted;
    if(fastStats.mode==MODE_BROTLI_Q5)++quick;
    // All selected streams must reconstruct; malformed wires are checked
    // in the immutable I16 leaf's own selftest as well.
    Bytes missing=fast;missing.pop_back();
    bool rejected=false;
    try{(void)hybridDecode(missing);}catch(const std::exception&){rejected=true;}
    expect(rejected,"fast truncated output accepted");
  }
  expect(predicted>0 && quick>0,"fast encoder must select both classes");
  hybridSelftest(); // parent malformed decoder and both reference selector checks
  std::cout<<"FAST_SELFTEST PASS cases="<<corpus.size()
           <<" numeric="<<predicted<<" q5="<<quick<<"\n";
}

static void fastBench(const fs::path& path) {
  const Bytes raw=readFile(path);
  HybridStats fastStats,fullStats;
  const Bytes fast=fastEncode(raw,fastStats);
  const Bytes full=hybridEncode(raw,fullStats);
  expect(hybridDecode(fast)==raw && hybridDecode(full)==raw,
         "I17 measured input roundtrip");
  volatile uint64_t sink=0;
  // Unlike I16's legacy encode scout, every encoded byte here is consumed.
  const double fastEnc=medianMicros([&]{
    HybridStats st;const Bytes coded=fastEncode(raw,st);
    sink=sink^observedDigest(coded);
  },5);
  const double fullEnc=medianMicros([&]{
    HybridStats st;const Bytes coded=hybridEncode(raw,st);
    sink=sink^observedDigest(coded);
  },3);
  const double fastDec=medianMicros([&]{
    const Bytes decoded=hybridDecode(fast);
    sink=sink^observedDigest(decoded);
  },7);
  const double fullDec=medianMicros([&]{
    const Bytes decoded=hybridDecode(full);
    sink=sink^observedDigest(decoded);
  },7);
  std::cout<<path.filename().string()<<'\t'<<raw.size()<<'\t'
           <<fast.size()<<'\t'<<full.size()<<'\t'
           <<unsigned(fastStats.mode)<<'\t'<<unsigned(fullStats.mode)<<'\t'
           <<fastStats.i14Bytes<<'\t'<<fullStats.q5Bytes<<'\t'
           <<fullStats.q11Bytes<<'\t'
           <<fastStats.i14Models<<'\t'<<fastStats.i14Diff<<'\t'
           <<std::fixed<<std::setprecision(3)
           <<(raw.empty()?0:double(raw.size())/fastEnc)<<'\t'
           <<(raw.empty()?0:double(raw.size())/fullEnc)<<'\t'
           <<(raw.empty()?0:double(raw.size())/fastDec)<<'\t'
           <<(raw.empty()?0:double(raw.size())/fullDec);
  for(int q:{5,11}) {
    if(raw.empty()) {
      std::cout<<"\t0\t0\t0";
      continue;
    }
    const Bytes reference=brotliEncode(raw,q);
    expect(brotliDecode(reference,raw.size())==raw,"Brotli ref exact decode");
    const double enc=medianMicros([&]{
      const Bytes v=brotliEncode(raw,q);
      sink=sink^observedDigest(v);
    },q==11?3:5);
    const double dec=medianMicros([&]{
      const Bytes v=brotliDecode(reference,raw.size());
      sink=sink^observedDigest(v);
    },7);
    std::cout<<'\t'<<reference.size()<<'\t'
             <<double(raw.size())/enc<<'\t'<<double(raw.size())/dec;
  }
  std::cout<<"\n";(void)sink;
}

int main(int argc,char** argv) {
  try {
    if(argc==2 && std::string(argv[1])=="--selftest") {
      fastSelftest();return 0;
    }
    if(argc==4 && std::string(argv[1])=="--encode-fast") {
      HybridStats stats;
      writeFile(argv[3],fastEncode(readFile(argv[2]),stats));return 0;
    }
    if(argc==4 && std::string(argv[1])=="--encode-full") {
      HybridStats stats;
      writeFile(argv[3],hybridEncode(readFile(argv[2]),stats));return 0;
    }
    if(argc==4 && std::string(argv[1])=="--decode") {
      writeFile(argv[3],hybridDecode(readFile(argv[2])));return 0;
    }
    if(argc>=3 && std::string(argv[1])=="--bench") {
      std::cout<<"file\tinput_bytes\tfast_bytes\tfull_bytes\tfast_mode\tfull_mode"
               <<"\tI14_bytes\tq5_bytes\tq11_bytes\tmodel_blocks\tdiff_blocks"
               <<"\tfast_encode_MBps\tfull_encode_MBps"
               <<"\tfast_decode_MBps\tfull_decode_MBps"
               <<"\tpaired_q5_bytes\tpaired_q5_encode_MBps\tpaired_q5_decode_MBps"
               <<"\tpaired_q11_bytes\tpaired_q11_encode_MBps\tpaired_q11_decode_MBps\n";
      for(int i=2;i<argc;++i)fastBench(argv[i]);
      return 0;
    }
    std::cerr<<"I17: --selftest | --encode-fast IN OUT | --encode-full IN OUT | --decode IN OUT | --bench FILE...\n";
    return 2;
  } catch(const std::exception& e) {
    std::cerr<<"I17_FAIL "<<e.what()<<"\n";return 1;
  }
}
