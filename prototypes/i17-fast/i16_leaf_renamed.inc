// ANVIL I16: whole-file, complete-byte adaptive envelope of frozen I14,
// pinned Brotli q5, and pinned Brotli q11. This is an ADOPTED FALLBACK, not
// a novel compression algorithm nor a verified general-purpose Pareto crossing.
// All compilation and workloads MUST run in GitHub Actions only.
#define main i14_frozen_command_main
#include "../i14-differential/i14_differential.cpp"
#undef main

// The frozen I14 code above supplies exact AVI4 and Brotli codecs, bounded
// varint/format decoders and a byte-consuming digest. It is source-attested.
static constexpr std::array<uint8_t,4> HYBRID_MAGIC{'A','V','H','1'};
static constexpr uint8_t MODE_AVI4=1, MODE_BROTLI_Q5=2, MODE_BROTLI_Q11=3;

struct HybridStats {
  uint8_t mode=0;
  uint64_t i14Models=0;
  uint64_t i14Diff=0;
  size_t i14Bytes=0, q5Bytes=0, q11Bytes=0;
};

static Bytes hybridEncode(const Bytes& original, HybridStats& stats) {
  if(original.size()>OUTPUT_LIMIT)fail("hybrid source too large");
  Stats i14Stats;
  const Bytes predicted=encode(original,i14Stats);
  // Empty source has an exact byte-safe AVI4 encoding. Otherwise test both
  // independently built Brotli reference qualities and charge whole wire size.
  Bytes b5,b11;
  if(!original.empty()) {
    b5=brotliEncode(original,5);
    b11=brotliEncode(original,11);
  }
  uint8_t selected=MODE_AVI4;
  const Bytes* chosen=&predicted;
  if(!original.empty() && b5.size()<chosen->size()) {
    selected=MODE_BROTLI_Q5;chosen=&b5;
  }
  if(!original.empty() && b11.size()<chosen->size()) {
    selected=MODE_BROTLI_Q11;chosen=&b11;
  }
  Bytes result;
  result.reserve(4+varSize(original.size())+1+chosen->size());
  result.insert(result.end(),HYBRID_MAGIC.begin(),HYBRID_MAGIC.end());
  putVar(result,original.size());
  result.push_back(selected);
  result.insert(result.end(),chosen->begin(),chosen->end());
  stats={selected,i14Stats.model,i14Stats.diff,predicted.size(),b5.size(),b11.size()};
  return result;
}

static Bytes hybridDecode(const Bytes& compressed) {
  if(compressed.size()<5||!std::equal(HYBRID_MAGIC.begin(),HYBRID_MAGIC.end(),compressed.begin()))
    fail("hybrid invalid magic");
  size_t position=4;
  const uint64_t targetSize=getVar(compressed,position);
  if(targetSize>OUTPUT_LIMIT)fail("hybrid output cap");
  if(position==compressed.size())fail("hybrid mode missing");
  const uint8_t mode=compressed[position++];
  if(position>=compressed.size())fail("hybrid payload absent");
  const Bytes payload(compressed.begin()+position,compressed.end());
  Bytes recovered;
  switch(mode) {
    case MODE_AVI4: recovered=decode(payload);break;
    case MODE_BROTLI_Q5:
    case MODE_BROTLI_Q11:
      recovered=brotliDecode(payload,size_t(targetSize));
      break;
    default: fail("hybrid invalid mode");
  }
  if(recovered.size()!=targetSize)fail("hybrid output length mismatch");
  return recovered;
}

static void hybridSelftest() {
  std::vector<Bytes> corpus;
  for(size_t n:{0u,1u,2u,7u,15u,16u,31u,128u,4095u,4096u,4097u,8192u})
    corpus.push_back(Bytes(n,uint8_t(n)));
  Bytes arithmetic(8192),repeated(8192),noise(8192);
  uint64_t walk=0xfffffc00ULL, rng=0x8c4e7ad3f6192b55ULL;
  for(size_t i=0;i<arithmetic.size()/4;++i) {
    rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;
    walk+=uint64_t(64+int(rng%3)-1);
    setWord(arithmetic.data()+i*4,walk,4);
  }
  std::string sentence="{\"id\":123,\"name\":\"anvil\",\"flag\":true}\n";
  for(size_t i=0;i<repeated.size();++i)repeated[i]=uint8_t(sentence[i%sentence.size()]);
  for(auto &c:noise) {
    rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;c=uint8_t(rng);
  }
  corpus.push_back(arithmetic);
  corpus.push_back(repeated);
  corpus.push_back(noise);
  uint64_t selectNumeric=0,selectBrotli=0;
  for(const auto& raw:corpus) {
    HybridStats stats;
    const auto encoded=hybridEncode(raw,stats);
    expect(hybridDecode(encoded)==raw,"I16 exact roundtrip");
    expect(encoded.size()>=5+1+std::min(stats.i14Bytes,
      stats.q5Bytes?std::min(stats.q5Bytes,stats.q11Bytes):stats.i14Bytes),
      "hybrid incorrect framing");
    if(stats.mode==MODE_AVI4 && !raw.empty())++selectNumeric;
    if(stats.mode==MODE_BROTLI_Q5||stats.mode==MODE_BROTLI_Q11)++selectBrotli;
    // A payload truncation cannot legally decode to the declared full bytes.
    auto shortWire=encoded;
    shortWire.pop_back();
    bool rejected=false;
    try{(void)hybridDecode(shortWire);}catch(const std::exception&){rejected=true;}
    expect(rejected,"hybrid accepted truncated payload");
  }
  expect(selectNumeric>0,"hybrid never chose modeled numeric");
  expect(selectBrotli>0,"hybrid never chose dictionary source");
  for(auto invalid:{
    Bytes{'B','A','D','1',0,1,1},
    Bytes{'A','V','H','1',0x80},
    Bytes{'A','V','H','1',0},
    Bytes{'A','V','H','1',1,8,0},
    Bytes{'A','V','H','1',1,1,0},
    Bytes{'A','V','H','1',0xff,0xff,0xff,0xff,0x01,1,0}
  }) {
    bool rejected=false;
    try{(void)hybridDecode(invalid);}catch(const std::exception&){rejected=true;}
    expect(rejected,"invalid hybrid wire accepted");
  }
  std::cout<<"SELFTEST PASS cases="<<corpus.size()
           <<" modeled_selected="<<selectNumeric
           <<" reference_selected="<<selectBrotli<<"\n";
}

static void hybridBench(const fs::path& filename) {
  const Bytes source=readFile(filename);
  HybridStats stats;
  const Bytes selected=hybridEncode(source,stats);
  expect(hybridDecode(selected)==source,"hybrid benchmark roundtrip");
  const double enc=medianMicros([&]{
    HybridStats local;const Bytes output=hybridEncode(source,local);
    volatile uint64_t checksum=output.size();
    (void)checksum;
  },3);
  volatile uint64_t sink=0;
  const double dec=medianMicros([&]{
    const Bytes output=hybridDecode(selected);
    sink=sink^observedDigest(output);
  },7);
  const double encMB=source.empty()?0:double(source.size())/enc;
  const double decMB=source.empty()?0:double(source.size())/dec;
  std::cout<<filename.filename().string()<<'\t'<<source.size()<<'\t'
           <<selected.size()<<'\t'<<unsigned(stats.mode)<<'\t'
           <<stats.i14Bytes<<'\t'<<stats.q5Bytes<<'\t'<<stats.q11Bytes<<'\t'
           <<stats.i14Models<<'\t'<<stats.i14Diff<<'\t'
           <<std::fixed<<std::setprecision(3)<<encMB<<'\t'<<decMB;
  for(int quality:{5,11}) {
    if(source.empty()) {
      std::cout<<"\t0\t0\t0";
      continue;
    }
    const Bytes reference=brotliEncode(source,quality);
    expect(brotliDecode(reference,source.size())==source,"hybrid Brotli roundtrip");
    const double e=medianMicros([&]{
      auto candidate=brotliEncode(source,quality);
      sink=sink^candidate.size();
    },quality==11?3:5);
    const double d=medianMicros([&]{
      auto unpacked=brotliDecode(reference,source.size());
      sink=sink^observedDigest(unpacked);
    },7);
    std::cout<<'\t'<<reference.size()<<'\t'
             <<double(source.size())/e<<'\t'<<double(source.size())/d;
  }
  std::cout<<"\n";(void)sink;
}

int i16_frozen_command_main(int argc,char** argv) {
  try {
    if(argc==2&&std::string(argv[1])=="--selftest") {
      hybridSelftest();return 0;
    }
    if(argc==4&&std::string(argv[1])=="--encode") {
      HybridStats s;writeFile(argv[3],hybridEncode(readFile(argv[2]),s));return 0;
    }
    if(argc==4&&std::string(argv[1])=="--decode") {
      writeFile(argv[3],hybridDecode(readFile(argv[2])));return 0;
    }
    if(argc>=3&&std::string(argv[1])=="--bench") {
      std::cout<<"file\tinput_bytes\twire_bytes\tselected_mode\tAVI4_bytes"
               <<"\tBrotli_q5_bytes\tBrotli_q11_bytes\tI14_model_blocks\tI14_diff_blocks"
               <<"\tencode_MBps\tdecode_MBps"
               <<"\tpaired_Brotli_q5_bytes\tpaired_Brotli_q5_encode_MBps"
               <<"\tpaired_Brotli_q5_decode_MBps\tpaired_Brotli_q11_bytes"
               <<"\tpaired_Brotli_q11_encode_MBps\tpaired_Brotli_q11_decode_MBps\n";
      for(int i=2;i<argc;++i)hybridBench(argv[i]);
      return 0;
    }
    std::cerr<<"I16: --selftest | --encode IN OUT | --decode IN OUT | --bench FILE...\n";
    return 2;
  } catch(const std::exception& e) {
    std::cerr<<"I16_FAIL "<<e.what()<<"\n";return 1;
  }
}
