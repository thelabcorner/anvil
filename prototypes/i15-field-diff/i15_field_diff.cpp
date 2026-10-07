// ANVIL I15: bounded modular predictor with dense bitpacked innovations.
// Research-only AVI3 wire; never substitute for ANVIL's production format.
// CPU-heavy compilation, selftests and benchmarking: GitHub Actions ONLY.
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#if defined(I15_WITH_BROTLI)
#include <brotli/encode.h>
#include <brotli/decode.h>
#endif
namespace fs = std::filesystem;
using Bytes = std::vector<uint8_t>;
static constexpr size_t BLOCK = 4096;
static constexpr uint64_t OUTPUT_LIMIT = 1ULL << 28;
[[noreturn]] static void fail(const char* m) { throw std::runtime_error(m); }
static size_t varSize(uint64_t x) { size_t n=1; while(x>=128) {x>>=7;++n;}return n; }
static void putVar(Bytes& b,uint64_t x) {
  do {const uint8_t v=uint8_t(x&127);x>>=7;b.push_back(uint8_t(v|(x?128:0)));}while(x);
}
static uint64_t getVar(const Bytes& b,size_t& p) {
  uint64_t n=0;
  for(unsigned i=0;i<10;++i) {
    if(p>=b.size())fail("truncated varint");
    const uint8_t v=b[p++];
    if(i==9&&(v&127)>1)fail("overflow varint");
    n|=uint64_t(v&127)<<(7*i);
    if(!(v&128)) {
      if(i && (v&127)==0)fail("noncanonical varint");
      return n;
    }
  }
  fail("overlong varint");
}
static bool widthOK(unsigned w) {return w==1||w==2||w==4||w==8;}
static uint64_t maskFor(unsigned w) {
  return w==8 ? ~uint64_t(0) : (uint64_t(1)<<(8*w))-1;
}
static uint64_t word(const uint8_t* p,unsigned w) {
  uint64_t x=0;for(unsigned j=0;j<w;++j)x|=uint64_t(p[j])<<(j*8);return x;
}
static void putWord(Bytes& b,uint64_t x,unsigned w) {
  for(unsigned j=0;j<w;++j)b.push_back(uint8_t(x>>(8*j)));
}
static void setWord(uint8_t* p,uint64_t x,unsigned w) {
  for(unsigned j=0;j<w;++j)p[j]=uint8_t(x>>(8*j));
}
static uint64_t takeWord(const Bytes& b,size_t& p,unsigned w) {
  if(p>b.size()||w>b.size()-p)fail("truncated word");
  const auto x=word(b.data()+p,w);p+=w;return x;
}
static int64_t signedRing(uint64_t x,unsigned w) {
  if(w==8)return std::bit_cast<int64_t>(x);
  const uint64_t mask=maskFor(w), sign=uint64_t(1)<<(8*w-1);
  x&=mask;
  if(x&sign)x|=~mask;
  return std::bit_cast<int64_t>(x);
}
static uint64_t zigRing(uint64_t x,unsigned w) {
  const uint64_t mask=maskFor(w),sign=uint64_t(1)<<(8*w-1);
  x&=mask;
  if(x&sign) {
    const uint64_t mag=(~x+1)&mask;
    return ((mag<<1)-1)&mask;
  }
  return (x<<1)&mask;
}
static uint64_t unzigRing(uint64_t z,unsigned w) {
  const uint64_t mask=maskFor(w);
  if(z&1)return (-((z>>1)+1))&mask;
  return (z>>1)&mask;
}
static unsigned bitLength(uint64_t x) {
  return x==0?0:64u-std::countl_zero(x);
}
static int64_t median(std::vector<int64_t>& xs) {
  if(xs.empty())fail("empty median");
  auto it=xs.begin()+xs.size()/2;
  std::nth_element(xs.begin(),it,xs.end());
  return *it;
}
// The accumulator is safe because candidate bit widths are <= 56.
static void pack(Bytes& dst,const std::vector<uint64_t>& v,unsigned bits) {
  if(!bits)return;
  uint64_t acc=0;unsigned buffered=0;
  for(uint64_t x:v) {
    acc|=x<<buffered;
    buffered+=bits;
    while(buffered>=8) {
      dst.push_back(uint8_t(acc));
      acc>>=8;buffered-=8;
    }
  }
  if(buffered)dst.push_back(uint8_t(acc));
}
static std::vector<uint64_t> unpack(const Bytes& src,size_t& p,size_t count,unsigned bits) {
  const size_t need=(count*size_t(bits)+7)/8;
  if(p>src.size()||need>src.size()-p)fail("truncated packed residuals");
  const size_t end=p+need;
  std::vector<uint64_t> residuals;
  residuals.reserve(count);
  uint64_t acc=0;unsigned buffered=0;
  const uint64_t mask=bits?((uint64_t(1)<<bits)-1):0;
  for(size_t i=0;i<count;++i) {
    while(buffered<bits) {
      if(p>=end)fail("packed bit underflow");
      acc|=uint64_t(src[p++])<<buffered;
      buffered+=8;
    }
    residuals.push_back(acc&mask);
    if(bits){acc>>=bits;buffered-=bits;}
  }
  if(p!=end)fail("packed length mismatch");
  if(buffered && acc)fail("nonzero padding bits");
  return residuals;
}
// I15: retain each I13 RAW / AFF column, add exact local modular DIFF.
// Selection is strictly by complete serialized column descriptor + payload.
static Bytes encodeColumn(const uint8_t* src,size_t records,unsigned stride,unsigned lane,
                          bool& modeled,bool& differential) {
  Bytes best;best.reserve(1+records*4);best.push_back(0);
  for(size_t i=0;i<records;++i)
    putWord(best,word(src+(i*stride+lane)*4,4),4);
  modeled=false;differential=false;
  if(records<3)return best;
  std::vector<int64_t> differences;differences.reserve(records-1);
  uint64_t prev=word(src+lane*4,4);
  for(size_t i=1;i<records;++i) {
    const auto cur=word(src+(i*stride+lane)*4,4);
    differences.push_back(signedRing((cur-prev)&0xffffffffu,4));prev=cur;
  }
  const uint64_t step=uint64_t(median(differences))&0xffffffffu;
  const uint64_t seed=word(src+lane*4,4);
  std::vector<int64_t> offsets;offsets.reserve(records);
  for(size_t i=0;i<records;++i) {
    const uint64_t val=word(src+(i*stride+lane)*4,4);
    offsets.push_back(signedRing((val-seed-uint64_t(i)*step)&0xffffffffu,4));
  }
  const uint64_t base=(seed+uint64_t(median(offsets)))&0xffffffffu;
  std::vector<uint64_t> residuals;residuals.reserve(records);
  uint64_t maximal=0;
  for(size_t i=0;i<records;++i) {
    const uint64_t predicted=(base+uint64_t(i)*step)&0xffffffffu;
    const uint64_t val=word(src+(i*stride+lane)*4,4);
    const uint64_t z=zigRing((val-predicted)&0xffffffffu,4);
    residuals.push_back(z);maximal=std::max(maximal,z);
  }
  const unsigned bits=bitLength(maximal);
  if(bits<32) {
    const size_t payload=(records*size_t(bits)+7)/8;
    if(10+payload<best.size()) {
      Bytes candidate;candidate.reserve(10+payload);
      candidate.push_back(1);putWord(candidate,base,4);putWord(candidate,step,4);
      candidate.push_back(uint8_t(bits));pack(candidate,residuals,bits);
      if(candidate.size()<best.size()){best=std::move(candidate);modeled=true;}
    }
  }
  // Local innovations independently encode adjacent modular step deviations.
  // Median slope workspace above is permuted; re-read words in original order.
  std::vector<uint64_t> local;local.reserve(records-1);
  uint64_t localMax=0;prev=seed;
  for(size_t i=1;i<records;++i) {
    const uint64_t cur=word(src+(i*stride+lane)*4,4);
    const uint64_t z=zigRing((cur-prev-step)&0xffffffffu,4);
    local.push_back(z);localMax=std::max(localMax,z);prev=cur;
  }
  const unsigned localBits=bitLength(localMax);
  if(localBits<32) {
    const size_t payload=((records-1)*size_t(localBits)+7)/8;
    if(10+payload<best.size()) {
      Bytes candidate;candidate.reserve(10+payload);
      candidate.push_back(2);putWord(candidate,seed,4);putWord(candidate,step,4);
      candidate.push_back(uint8_t(localBits));pack(candidate,local,localBits);
      if(candidate.size()<best.size()) {
        best=std::move(candidate);modeled=true;differential=true;
      }
    }
  }
  return best;
}
struct Stats {uint64_t raw=0,model=0,diff=0,packed=0,totalBits=0,maxBits=0,columns=0,columnModels=0,columnDiff=0;};
static void emit(Bytes& out,const uint8_t* src,size_t n,Stats& s) {
  Bytes best;
  best.reserve(n+4);best.push_back(0);putVar(best,n);
  best.insert(best.end(),src,src+n);
  unsigned modeBits=0;size_t payloadBytes=0;bool modeled=false,differential=false;
  for(unsigned w:{1u,2u,4u,8u}) {
    if(n%w || n<3*w)continue;
    const size_t count=n/w;
    const uint64_t mask=maskFor(w);
    std::vector<int64_t> diffs;diffs.reserve(count-1);
    uint64_t prev=word(src,w);
    for(size_t i=1;i<count;++i) {
      const uint64_t cur=word(src+i*w,w);
      diffs.push_back(signedRing((cur-prev)&mask,w));prev=cur;
    }
    const uint64_t step=uint64_t(median(diffs))&mask;
    const uint64_t seed=word(src,w);
    std::vector<int64_t> offsets;offsets.reserve(count);
    for(size_t i=0;i<count;++i) {
      const uint64_t v=word(src+i*w,w);
      const uint64_t pred=(seed+uint64_t(i)*step)&mask;
      offsets.push_back(signedRing((v-pred)&mask,w));
    }
    const uint64_t base=(seed+uint64_t(median(offsets)))&mask;
    std::vector<uint64_t> residuals;residuals.reserve(count);
    uint64_t maximum=0;
    for(size_t i=0;i<count;++i) {
      const uint64_t pred=(base+uint64_t(i)*step)&mask;
      const uint64_t z=zigRing((word(src+i*w,w)-pred)&mask,w);
      maximum=std::max(maximum,z);
      residuals.push_back(z);
    }
    const unsigned bits=bitLength(maximum);
    // At >= raw word size or > 56 bits, RAW cannot be beaten by this
    // single-stream representation once descriptor overhead is charged.
    if(bits>=8*w||bits>56)continue;
    const size_t packedBytes=(count*size_t(bits)+7)/8;
    const size_t header=3+varSize(count)+2*w;
    if(header+packedBytes>=best.size())continue;
    Bytes candidate;
    candidate.reserve(header+packedBytes);
    candidate.push_back(3);candidate.push_back(uint8_t(w));putVar(candidate,count);
    putWord(candidate,base,w);putWord(candidate,step,w);
    candidate.push_back(uint8_t(bits));
    pack(candidate,residuals,bits);
    if(candidate.size()<best.size()) {
      best=std::move(candidate);
      modeled=true;modeBits=bits;payloadBytes=packedBytes;
    }
  }
  // I15 whole-block local DIFF, with all frozen I13 choices retained.
  for(unsigned w:{1u,2u,4u,8u}) {
    if(n%w || n<3*w)continue;
    const size_t count=n/w, mask=maskFor(w);
    std::vector<int64_t> differences;differences.reserve(count-1);
    uint64_t prev=word(src,w);
    for(size_t i=1;i<count;++i) {
      const uint64_t cur=word(src+i*w,w);
      differences.push_back(signedRing((cur-prev)&mask,w));prev=cur;
    }
    const uint64_t step=uint64_t(median(differences))&mask;
    const uint64_t seed=word(src,w);
    std::vector<uint64_t> local;local.reserve(count-1);
    uint64_t localMax=0;prev=seed;
    for(size_t i=1;i<count;++i) {
      const uint64_t cur=word(src+i*w,w);
      const uint64_t z=zigRing((cur-prev-step)&mask,w);
      local.push_back(z);localMax=std::max(localMax,z);prev=cur;
    }
    const unsigned bits=bitLength(localMax);
    if(bits>=8*w||bits>56)continue;
    const size_t payload=((count-1)*size_t(bits)+7)/8;
    if(3+varSize(count)+2*w+payload>=best.size())continue;
    Bytes candidate;candidate.reserve(3+varSize(count)+2*w+payload);
    candidate.push_back(5);candidate.push_back(uint8_t(w));putVar(candidate,count);
    putWord(candidate,seed,w);putWord(candidate,step,w);
    candidate.push_back(uint8_t(bits));pack(candidate,local,bits);
    if(candidate.size()<best.size()) {
      best=std::move(candidate);modeled=true;differential=true;
      modeBits=bits;payloadBytes=payload;
    }
  }
  // Consider byte-exact 32-bit interleaved fields with RAW fallback per lane.
  bool columnsChosen=false;
  size_t selectedColumnModels=0,selectedColumnDiff=0;
  for(unsigned stride=2;stride<=12;++stride) {
    const size_t records=n/(size_t(stride)*4);
    if(records<3)continue;
    const size_t full=records*stride*4;
    Bytes candidate;candidate.reserve(n+64);
    candidate.push_back(4);putVar(candidate,n);candidate.push_back(uint8_t(stride));
    size_t modeledFields=0,diffFields=0;
    for(unsigned lane=0;lane<stride;++lane) {
      bool fieldModeled=false,fieldDiff=false;
      const Bytes field=encodeColumn(src,records,stride,lane,fieldModeled,fieldDiff);
      candidate.insert(candidate.end(),field.begin(),field.end());
      modeledFields+=fieldModeled?1:0;
      diffFields+=fieldDiff?1:0;
    }
    candidate.insert(candidate.end(),src+full,src+n);
    if(candidate.size()<best.size()) {
      best=std::move(candidate);
      columnsChosen=true;selectedColumnModels=modeledFields;selectedColumnDiff=diffFields;modeled=false;differential=false;
    }
  }
  out.insert(out.end(),best.begin(),best.end());
  if(columnsChosen){++s.columns;s.columnModels+=selectedColumnModels;s.columnDiff+=selectedColumnDiff;}
  else if(modeled){++s.model;if(differential)++s.diff;s.packed+=payloadBytes;s.totalBits+=modeBits;s.maxBits=std::max(s.maxBits,uint64_t(modeBits));}
  else ++s.raw;
}
static Bytes encode(const Bytes& src,Stats& s) {
  if(src.size()>OUTPUT_LIMIT)fail("input exceeds cap");
  Bytes out={'A','V','I','5'};putVar(out,src.size());
  for(size_t i=0;i<src.size();) {
    const size_t n=std::min(BLOCK,src.size()-i);
    emit(out,src.data()+i,n,s);i+=n;
  }
  return out;
}
static Bytes decode(const Bytes& src) {
  if(src.size()<4||src[0]!='A'||src[1]!='V'||src[2]!='I'||src[3]!='5')
    fail("AVI3 magic");
  size_t p=4;const uint64_t n=getVar(src,p);
  if(n>OUTPUT_LIMIT)fail("declared output exceeds cap");
  Bytes out;out.reserve(size_t(n));
  while(out.size()<n) {
    if(p>=src.size())fail("truncated tag");
    const unsigned tag=src[p++];
    if(tag==0) {
      const uint64_t count=getVar(src,p);
      if(!count||count>BLOCK||count>n-out.size()||p>src.size()||count>src.size()-p)
        fail("invalid RAW length");
      out.insert(out.end(),src.begin()+p,src.begin()+p+size_t(count));
      p+=size_t(count);
    }else if(tag==3) {
      if(p>=src.size())fail("truncated width");
      const unsigned w=src[p++];if(!widthOK(w))fail("invalid width");
      const uint64_t count=getVar(src,p);
      if(count<3||count>BLOCK/w||count>(n-out.size())/w)fail("invalid count");
      const uint64_t mask=maskFor(w);
      const uint64_t base=takeWord(src,p,w),step=takeWord(src,p,w);
      if(p>=src.size())fail("truncated bit width");
      const unsigned bits=src[p++];
      if(bits>=8*w||bits>56)fail("invalid bit width");
      auto residuals=unpack(src,p,size_t(count),bits);
      const size_t start=out.size();out.resize(start+size_t(count)*w);
      for(size_t i=0;i<count;++i) {
        const uint64_t pred=(base+uint64_t(i)*step)&mask;
        const uint64_t v=(pred+unzigRing(residuals[i],w))&mask;
        setWord(out.data()+start+i*w,v,w);
      }
    }else if(tag==5) {
      if(p>=src.size())fail("truncated DIFF width");
      const unsigned w=src[p++];if(!widthOK(w))fail("invalid DIFF width");
      const uint64_t count=getVar(src,p);
      if(count<3||count>BLOCK/w||count>(n-out.size())/w)fail("invalid DIFF count");
      const uint64_t mask=maskFor(w);
      const uint64_t first=takeWord(src,p,w),step=takeWord(src,p,w);
      if(p>=src.size())fail("truncated DIFF bits");
      const unsigned bits=src[p++];
      if(bits>=8*w||bits>56)fail("invalid DIFF bits");
      const auto residuals=unpack(src,p,size_t(count-1),bits);
      const size_t start=out.size();out.resize(start+size_t(count)*w);
      uint64_t state=first;setWord(out.data()+start,state,w);
      for(size_t i=1;i<count;++i) {
        state=(state+step+unzigRing(residuals[i-1],w))&mask;
        setWord(out.data()+start+i*w,state,w);
      }
    }else if(tag==4) {
      const uint64_t blockBytes=getVar(src,p);
      if(blockBytes<24||blockBytes>BLOCK||blockBytes>n-out.size())
        fail("invalid strided block bytes");
      if(p>=src.size())fail("truncated stride");
      const unsigned stride=src[p++];
      if(stride<2||stride>12)fail("invalid stride");
      const size_t records=size_t(blockBytes)/(size_t(stride)*4);
      if(records<3)fail("strided record count underflow");
      const size_t full=records*stride*4;
      const size_t start=out.size();out.resize(start+size_t(blockBytes));
      for(unsigned lane=0;lane<stride;++lane) {
        if(p>=src.size())fail("truncated column tag");
        const unsigned mode=src[p++];
        if(mode==0) {
          if(p>src.size()||records*4>src.size()-p)fail("truncated raw column");
          for(size_t i=0;i<records;++i)
            setWord(out.data()+start+(i*stride+lane)*4,
                    word(src.data()+p+i*4,4),4);
          p+=records*4;
        } else if(mode==1) {
          const uint64_t base=takeWord(src,p,4),step=takeWord(src,p,4);
          if(p>=src.size())fail("truncated column bits");
          const unsigned bits=src[p++];
          if(bits>=32)fail("invalid column bits");
          const auto residuals=unpack(src,p,records,bits);
          for(size_t i=0;i<records;++i) {
            const uint64_t predicted=(base+uint64_t(i)*step)&0xffffffffu;
            const uint64_t value=(predicted+unzigRing(residuals[i],4))&0xffffffffu;
            setWord(out.data()+start+(i*stride+lane)*4,value,4);
          }
        } else if(mode==2) {
          const uint64_t first=takeWord(src,p,4),step=takeWord(src,p,4);
          if(p>=src.size())fail("truncated column DIFF bits");
          const unsigned bits=src[p++];
          if(bits>=32)fail("invalid column DIFF bits");
          const auto residuals=unpack(src,p,records-1,bits);
          uint64_t state=first;
          setWord(out.data()+start+lane*4,state,4);
          for(size_t i=1;i<records;++i) {
            state=(state+step+unzigRing(residuals[i-1],4))&0xffffffffu;
            setWord(out.data()+start+(i*stride+lane)*4,state,4);
          }
        } else fail("unknown column tag");
      }
      const size_t tail=size_t(blockBytes)-full;
      if(p>src.size()||tail>src.size()-p)fail("truncated strided tail");
      for(size_t i=0;i<tail;++i)out[start+full+i]=src[p+i];
      p+=tail;
    }else fail("unknown block tag");
  }
  if(p!=src.size())fail("trailing bytes");
  return out;
}
static Bytes readFile(const fs::path& f) {
  std::ifstream in(f,std::ios::binary|std::ios::ate);
  if(!in)fail("input file open");
  const auto n=in.tellg();
  if(n<0||uint64_t(n)>OUTPUT_LIMIT+1024*1024)fail("input file size");
  Bytes data(size_t(n),0);in.seekg(0);
  if(!data.empty()&&!in.read(reinterpret_cast<char*>(data.data()),std::streamsize(data.size())))
    fail("input file read");
  return data;
}
static void writeFile(const fs::path& f,const Bytes& data) {
  std::ofstream out(f,std::ios::binary|std::ios::trunc);
  if(!out)fail("output file open");
  if(!data.empty())out.write(reinterpret_cast<const char*>(data.data()),std::streamsize(data.size()));
  if(!out)fail("output file write");
}
static void expect(bool ok,const char* m){if(!ok)fail(m);}
static uint64_t observedDigest(const Bytes& b) {
  uint64_t hash=0xcbf29ce484222325ULL;
  for(uint8_t c:b){hash=(hash<<7)|(hash>>57);hash=(hash^c)*0x9e3779b185ebca87ULL;}
  return hash;
}
static void selftest() {
  std::vector<Bytes> cases;
  for(size_t n:{0u,1u,2u,7u,8u,9u,15u,16u,17u,4095u,4096u,4097u,8192u})
    cases.push_back(Bytes(n,uint8_t(n)));
  Bytes numeric(8192),jitter(8192),wrapping(8192),noise(8192);
  uint32_t seed=0xfffffe00u;
  uint64_t random=0xd1b54a32d192ed03ULL;
  for(size_t i=0;i<numeric.size()/4;++i) {
    const uint32_t v=uint32_t(748451u+64u*uint32_t(i));
    setWord(numeric.data()+4*i,v,4);
    setWord(jitter.data()+4*i,v+uint32_t(int(i%5)-2),4);
    setWord(wrapping.data()+4*i,seed+uint32_t(i*71u),4);
  }
  for(auto &x:noise){random^=random<<13;random^=random>>7;random^=random<<17;x=uint8_t(random);}
  // Mixed 7-column source: predictable fields next to random 32-bit values.
  // The registered experiment uses separate pre-existing files; this fixture
  // verifies the structural mode and scatter/tail reconstruction invariants.
  Bytes mixed(7*4*128),mixedTail(7*4*128+13);
  uint64_t mr=0xabcddcba12344321ULL;
  for(size_t i=0;i<128;++i) {
    const auto set=[&](size_t lane,uint32_t value) {
      setWord(mixed.data()+(i*7+lane)*4,value,4);
    };
    mr^=mr<<13;mr^=mr>>7;mr^=mr<<17;
    set(0,uint32_t(100000+61*i));
    set(1,0x12345678u);
    set(2,uint32_t(mr));
    mr^=mr<<13;mr^=mr>>7;mr^=mr<<17;
    set(3,uint32_t(mr));
    set(4,uint32_t(i%3));
    mr^=mr<<13;mr^=mr>>7;mr^=mr<<17;
    set(5,uint32_t(mr));
    set(6,0xdeadbeefu);
  }
  std::copy(mixed.begin(),mixed.end(),mixedTail.begin());
  for(size_t i=mixed.size();i<mixedTail.size();++i)mixedTail[i]=uint8_t(i);
  Bytes drifting(8192);
  uint32_t counter=0xfffff000u;
  uint64_t noiseSeed=0x9e3779b97f4a7c15ULL;
  for(size_t i=0;i<drifting.size()/4;++i) {
    noiseSeed^=noiseSeed<<13;noiseSeed^=noiseSeed>>7;noiseSeed^=noiseSeed<<17;
    counter+=64u+uint32_t(noiseSeed%3u)-1u;
    setWord(drifting.data()+i*4,counter,4);
  }
  cases.push_back(drifting);
  cases.push_back(numeric);cases.push_back(jitter);cases.push_back(wrapping);
  cases.push_back(noise);cases.push_back(mixed);cases.push_back(mixedTail);
  for(const auto& c:cases) {
    Stats s;const auto wire=encode(c,s);
    expect(decode(wire)==c,"roundtrip mismatch");
    if(!wire.empty()) {
      auto shortWire=wire;shortWire.pop_back();
      bool rejected=false;
      try{(void)decode(shortWire);}catch(const std::exception&){rejected=true;}
      expect(rejected,"truncated accepted");
    }
  }
  Stats s1,s2;
  expect(encode(jitter,s1).size()<jitter.size()/2,"jitter dense-innovation test");
  expect(s1.model+s1.columns>0,"bounded innovation was not selected");
  expect(encode(numeric,s2).size()<numeric.size()/4,"exact affine test");
  Stats driftStats;
  const auto driftWire=encode(drifting,driftStats);
  expect(driftStats.diff>0,"I15 global DIFF not selected");
  expect(decode(driftWire)==drifting,"I15 global DIFF mismatch");
  Bytes mixedWalk(7*4*128+13);
  uint64_t mwseed=0xabcddcba12344321ULL;
  uint32_t mwval=0xfff00000u;
  for(size_t i=0;i<128;++i) {
    mwseed^=mwseed<<13;mwseed^=mwseed>>7;mwseed^=mwseed<<17;
    mwval+=64u+uint32_t(mwseed%3u)-1u;
    setWord(mixedWalk.data()+(i*7+0)*4,mwval,4);
    for(size_t lane=1;lane<7;++lane) {
      mwseed^=mwseed<<13;mwseed^=mwseed>>7;mwseed^=mwseed<<17;
      const uint32_t val=(lane==6?0x12345678u:uint32_t(mwseed));
      setWord(mixedWalk.data()+(i*7+lane)*4,val,4);
    }
  }
  for(size_t i=128*7*4;i<mixedWalk.size();++i)mixedWalk[i]=uint8_t(i);
  Stats mixedWalkStats;
  const auto mixedWalkWire=encode(mixedWalk,mixedWalkStats);
  expect(mixedWalkStats.columnDiff>0,"I15 strided column DIFF not selected");
  expect(decode(mixedWalkWire)==mixedWalk,"I15 column DIFF mismatch");
  Stats stridedStats;
  const auto columnWire=encode(mixed,stridedStats);
  expect(decode(columnWire)==mixed,"strided column exact reconstruction");
  expect(stridedStats.columns>0&&stridedStats.columnModels>=2,
         "interleaved columns were not represented");
  expect(columnWire.size()<mixed.size()*3/4,"strided source not compact enough");
  for(Bytes badWire:{Bytes{'B','A','D','2'},Bytes{'A','V','I','5',0,9},
                     Bytes{'A','V','I','5',0x80},Bytes{'A','V','I','5',4,3,3}}) {
    bool rejected=false;
    try{(void)decode(badWire);}catch(const std::exception&){rejected=true;}
    expect(rejected,"malformed wire accepted");
  }
  std::cout<<"SELFTEST PASS cases="<<cases.size()<<" diff_blocks="<<driftStats.diff<<" column_diff="<<mixedWalkStats.columnDiff<<"\n";
}
template<class Fn>static double medianMicros(Fn f,int reps) {
  using clock=std::chrono::steady_clock;
  f();std::vector<double> v;v.reserve(reps);
  for(int i=0;i<reps;++i){const auto a=clock::now();f();const auto b=clock::now();
    v.push_back(std::chrono::duration<double,std::micro>(b-a).count());}
  std::sort(v.begin(),v.end());return std::max(0.001,v[v.size()/2]);
}
#if defined(I15_WITH_BROTLI)
static Bytes brotliEncode(const Bytes& raw,int q) {
  const size_t capacity=BrotliEncoderMaxCompressedSize(raw.size());
  if(!capacity)fail("Brotli bound");
  Bytes b(capacity);size_t n=b.size();
  if(!BrotliEncoderCompress(q,22,BROTLI_MODE_GENERIC,raw.size(),raw.data(),&n,b.data()))
    fail("Brotli encoding");
  b.resize(n);return b;
}
static Bytes brotliDecode(const Bytes& b,size_t n) {
  Bytes out(n);size_t actual=out.size();
  if(BrotliDecoderDecompress(b.size(),b.data(),&actual,out.data())!=BROTLI_DECODER_RESULT_SUCCESS
     ||actual!=n)fail("Brotli decoding");
  return out;
}
#endif
static void bench(const fs::path& f) {
  const auto raw=readFile(f);
  Stats initial;const auto wire=encode(raw,initial);
  expect(decode(wire)==raw,"measured-input roundtrip");
  volatile uint64_t sink=0;
  const auto enc=medianMicros([&] {Stats s;auto v=encode(raw,s);sink=sink^v.size();},5);
  const auto dec=medianMicros([&] {auto v=decode(wire);sink=sink^observedDigest(v);},7);
  const double eMB=raw.empty()?0:double(raw.size())/enc;
  const double dMB=raw.empty()?0:double(raw.size())/dec;
  std::cout<<f.filename().string()<<'\t'<<raw.size()<<'\t'<<wire.size()<<'\t'
    <<initial.raw<<'\t'<<initial.model<<'\t'<<initial.packed<<'\t'
    <<initial.totalBits<<'\t'<<initial.maxBits<<'\t'
    <<initial.columns<<'\t'<<initial.columnModels<<'\t'<<initial.diff<<'\t'<<initial.columnDiff<<'\t'
    <<std::fixed<<std::setprecision(3)<<eMB<<'\t'<<dMB;
#if defined(I15_WITH_BROTLI)
  for(int q:{5,11}) {
    const auto ref=brotliEncode(raw,q);
    expect(brotliDecode(ref,raw.size())==raw,"reference roundtrip");
    const double re=medianMicros([&] {auto v=brotliEncode(raw,q);sink=sink^v.size();},q==11?3:5);
    const double rd=medianMicros([&] {auto v=brotliDecode(ref,raw.size());sink=sink^observedDigest(v);},7);
    std::cout<<'\t'<<ref.size()<<'\t'<<(raw.empty()?0:double(raw.size())/re)
      <<'\t'<<(raw.empty()?0:double(raw.size())/rd);
  }
#endif
  std::cout<<"\n";(void)sink;
}
int main(int argc,char** argv) {
  try {
    if(argc==2&&std::string(argv[1])=="--selftest"){selftest();return 0;}
    if(argc==4&&std::string(argv[1])=="--encode"){
      Stats s;writeFile(argv[3],encode(readFile(argv[2]),s));return 0;
    }
    if(argc==4&&std::string(argv[1])=="--decode"){
      writeFile(argv[3],decode(readFile(argv[2])));return 0;
    }
    if(argc>=3&&std::string(argv[1])=="--bench"){
      std::cout<<"file\tinput_bytes\twire_bytes\traw_blocks\tmodel_blocks\tpacked_bytes\tmodel_bits_sum\tmodel_bits_max\tcolumn_blocks\tmodeled_columns\tdiff_blocks\tcolumn_diff_models\tencode_MBps\tdecode_MBps";
#if defined(I15_WITH_BROTLI)
      std::cout<<"\tbrotli_q5_bytes\tbrotli_q5_encode_MBps\tbrotli_q5_decode_MBps"
        <<"\tbrotli_q11_bytes\tbrotli_q11_encode_MBps\tbrotli_q11_decode_MBps";
#endif
      std::cout<<"\n";
      for(int i=2;i<argc;++i)bench(argv[i]);
      return 0;
    }
    std::cerr<<"I15: --selftest | --encode IN OUT | --decode IN OUT | --bench FILE...\n";
    return 2;
  }catch(const std::exception& e){std::cerr<<"I15_FAIL "<<e.what()<<"\n";return 1;}
}
