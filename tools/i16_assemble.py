#!/usr/bin/env python3
"""Text-only deterministic I16 assembly from frozen source I15; CPU codec work remote."""
from pathlib import Path
s=Path("prototypes/i15-field-diff/i15_field_diff.cpp").read_bytes().decode("utf-8")
assert "\r" not in s
def ch(old,new,expected=1):
    global s
    n=s.count(old)
    if n!=expected:
        raise ValueError(f"anchor mismatch expected {expected}, found {n}: {old[:70]!r}")
    s=s.replace(old,new)
ch("// ANVIL I15","// ANVIL I16",1)
ch("I15_WITH_BROTLI","I16_WITH_BROTLI",4)
ch("struct Stats {uint64_t raw=0,model=0,diff=0,packed=0,totalBits=0,maxBits=0,columns=0,columnModels=0,columnDiff=0;};",
"""struct Stats {uint64_t raw=0,model=0,diff=0,packed=0,totalBits=0,maxBits=0,columns=0,columnModels=0,columnDiff=0,frames=0,frameDiff=0;};""")
anchor="static void emit(Bytes& out,const uint8_t* src,size_t n,Stats& s) {"
helper=r"""
// I16 bounded file-origin record-frame hypotheses. Every frame has a complete
// independent byte-accurate prefix, field-run and tail. No hidden source schema.
// This pilot searches byte record widths 10,12,14,16,20,24,28,32 and two
// candidate phases: 0 and the next origin-aligned full record.
// Each field is either a literal byte span or 1/2/4/8-byte local modular DIFF.
// Dynamic programming minimizes EXACT framed column wire bytes.
struct FrameCandidate {Bytes wire;size_t diffFields=0;};
static FrameCandidate frameEncode(const uint8_t* src,size_t n,
                                  unsigned stride,unsigned phase) {
  FrameCandidate failed;
  if(phase>=stride||phase>=n)return failed;
  const size_t records=(n-phase)/stride;
  if(records<3)return failed;
  const size_t full=phase+records*stride;
  std::vector<Bytes> dp(stride+1);
  std::vector<size_t> models(stride+1,0);
  for(int pos=int(stride)-1;pos>=0;--pos) {
    bool has=false;
    Bytes chosen;size_t chosenModels=0;
    const unsigned j=unsigned(pos);
    // Merge arbitrary adjacent opaque bytes into a single literal field.
    for(unsigned width=1;width<=stride-j;++width) {
      Bytes candidate;
      candidate.reserve(2+records*width+dp[j+width].size());
      candidate.push_back(0);candidate.push_back(uint8_t(width));
      for(size_t i=0;i<records;++i) {
        const uint8_t* start=src+phase+i*stride+j;
        candidate.insert(candidate.end(),start,start+width);
      }
      candidate.insert(candidate.end(),dp[j+width].begin(),dp[j+width].end());
      if(!has||candidate.size()<chosen.size()) {
        chosen=std::move(candidate);
        chosenModels=models[j+width];has=true;
      }
    }
    for(unsigned width:{1u,2u,4u,8u}) {
      if(width>stride-j)continue;
      const uint64_t mask=maskFor(width);
      std::vector<int64_t> diffs;diffs.reserve(records-1);
      uint64_t previous=word(src+phase+j,width);
      for(size_t i=1;i<records;++i) {
        const uint64_t current=word(src+phase+i*stride+j,width);
        diffs.push_back(signedRing((current-previous)&mask,width));
        previous=current;
      }
      const uint64_t step=uint64_t(median(diffs))&mask;
      const uint64_t initial=word(src+phase+j,width);
      previous=initial;
      std::vector<uint64_t> residuals;residuals.reserve(records-1);
      uint64_t maxResidual=0;
      for(size_t i=1;i<records;++i) {
        const uint64_t current=word(src+phase+i*stride+j,width);
        const uint64_t z=zigRing((current-previous-step)&mask,width);
        residuals.push_back(z);
        maxResidual=std::max(maxResidual,z);
        previous=current;
      }
      const unsigned bits=bitLength(maxResidual);
      if(bits>=8*width||bits>56)continue;
      const size_t payload=((records-1)*size_t(bits)+7)/8;
      const size_t header=2*width+2;
      if(has && header+payload+dp[j+width].size()>=chosen.size())
        continue;
      Bytes candidate;candidate.reserve(header+payload+dp[j+width].size());
      candidate.push_back(uint8_t(width));
      putWord(candidate,initial,width);putWord(candidate,step,width);
      candidate.push_back(uint8_t(bits));pack(candidate,residuals,bits);
      candidate.insert(candidate.end(),dp[j+width].begin(),dp[j+width].end());
      if(candidate.size()<chosen.size()) {
        chosen=std::move(candidate);
        chosenModels=1+models[j+width];
      }
    }
    dp[j]=std::move(chosen);
    models[j]=chosenModels;
  }
  Bytes candidate;
  candidate.reserve(n+stride*3);
  candidate.push_back(6);putVar(candidate,n);
  candidate.push_back(uint8_t(stride));candidate.push_back(uint8_t(phase));
  candidate.insert(candidate.end(),src,src+phase);
  candidate.insert(candidate.end(),dp[0].begin(),dp[0].end());
  candidate.insert(candidate.end(),src+full,src+n);
  return {std::move(candidate),models[0]};
}
"""
ch(anchor,helper+"\n"+anchor)
ch("static void emit(Bytes& out,const uint8_t* src,size_t n,Stats& s) {",
   "static void emit(Bytes& out,const uint8_t* src,size_t n,size_t absoluteOffset,Stats& s) {")
ch("  bool columnsChosen=false;",
   "  bool framedChosen=false;size_t chosenFrameDiff=0;\n  bool columnsChosen=false;")
ch("  out.insert(out.end(),best.begin(),best.end());",
r"""  // I16 hypothesis: file-origin phase-aligned byte record frames or phase 0.
  // This is a bounded catalog, not exhaustive parsing or learned schema.
  for(unsigned stride:{10u,12u,14u,16u,20u,24u,28u,32u}) {
    const unsigned originPhase=unsigned((stride-(absoluteOffset%stride))%stride);
    for(unsigned phase:{0u,originPhase}) {
      if(phase==originPhase && phase==0u) {
        // Duplicate phase is harmless but avoid twice the search.
        if(phase!=0u)fail("impossible phase");
      }
      if(phase==0u && originPhase==0u) {
        // One candidate; the duplicate iteration below is filtered.
      }
      if(phase==originPhase && phase==0u && false)continue;
      const auto candidate=frameEncode(src,n,stride,phase);
      if(!candidate.wire.empty() && candidate.wire.size()<best.size()) {
        best=candidate.wire;
        framedChosen=true;chosenFrameDiff=candidate.diffFields;
        columnsChosen=false;modeled=false;differential=false;
      }
      if(originPhase==0u)break;
    }
  }
  out.insert(out.end(),best.begin(),best.end());""")
ch("  if(columnsChosen){++s.columns;",
   "  if(framedChosen){++s.frames;s.frameDiff+=chosenFrameDiff;}\n  else if(columnsChosen){++s.columns;")
ch("Bytes out={'A','V','I','5'};", "Bytes out={'A','V','I','6'};")
ch("    emit(out,src.data()+i,n,s);i+=n;",
   "    emit(out,src.data()+i,n,i,s);i+=n;")
ch("src[3]!='5'","src[3]!='6'")
ch("    }else if(tag==5) {",
r"""    }else if(tag==6) {
      const uint64_t blockBytes=getVar(src,p);
      if(blockBytes<30||blockBytes>BLOCK||blockBytes>n-out.size())
        fail("invalid framed block length");
      if(p+2>src.size())fail("truncated frame header");
      const unsigned stride=src[p++],phase=src[p++];
      if(stride!=10&&stride!=12&&stride!=14&&stride!=16
         &&stride!=20&&stride!=24&&stride!=28&&stride!=32)
        fail("invalid frame stride");
      if(phase>=stride||phase>=blockBytes)fail("invalid frame phase");
      const size_t records=(size_t(blockBytes)-phase)/stride;
      if(records<3)fail("invalid frame record count");
      const size_t full=phase+records*stride;
      const size_t start=out.size();out.resize(start+size_t(blockBytes));
      if(p>src.size()||phase>src.size()-p)fail("truncated prefix");
      std::copy(src.begin()+p,src.begin()+p+phase,out.begin()+start);
      p+=phase;
      unsigned j=0;
      while(j<stride) {
        if(p>=src.size())fail("truncated framed field");
        const unsigned mode=src[p++];
        if(mode==0) {
          if(p>=src.size())fail("truncated raw field length");
          const unsigned width=src[p++];
          if(width==0||width>stride-j)fail("invalid raw field width");
          if(p>src.size()||records*width>src.size()-p)
            fail("truncated raw field payload");
          for(size_t i=0;i<records;++i)
            std::copy(src.begin()+p+i*width,src.begin()+p+(i+1)*width,
                      out.begin()+start+phase+i*stride+j);
          p+=records*width;j+=width;
        }else if(widthOK(mode)) {
          const unsigned width=mode;
          if(width>stride-j)fail("overlapping framed predictor");
          const uint64_t mask=maskFor(width);
          const uint64_t seed=takeWord(src,p,width),step=takeWord(src,p,width);
          if(p>=src.size())fail("truncated framed bits");
          const unsigned bits=src[p++];
          if(bits>=8*width||bits>56)fail("invalid framed bits");
          const auto residuals=unpack(src,p,records-1,bits);
          uint64_t state=seed;
          setWord(out.data()+start+phase+j,state,width);
          for(size_t i=1;i<records;++i) {
            state=(state+step+unzigRing(residuals[i-1],width))&mask;
            setWord(out.data()+start+phase+i*stride+j,state,width);
          }
          j+=width;
        }else fail("unknown framed mode");
      }
      const size_t tail=size_t(blockBytes)-full;
      if(p>src.size()||tail>src.size()-p)fail("truncated framed tail");
      std::copy(src.begin()+p,src.begin()+p+tail,out.begin()+start+full);
      p+=tail;
    }else if(tag==5) {""")
# Minimal positive frame test: synthetic 14-byte records carrying step counters
# and opaque 4-byte payload. Purpose only to establish wire+decoder correctness;
# actual seven-source discovery still determines scientific adoption.
ch("  Stats stridedStats;",
r"""  Bytes framed(14*256);
  uint64_t t=1700000000000ULL, fseed=0xfeedcafebabedeadULL;
  for(size_t i=0;i<256;++i) {
    t+=1000u+uint64_t(i%3u);
    setWord(framed.data()+14*i,t,8);
    fseed^=fseed<<13;fseed^=fseed>>7;fseed^=fseed<<17;
    setWord(framed.data()+14*i+8,uint32_t(fseed),4);
    setWord(framed.data()+14*i+12,uint16_t(i%1000),2);
  }
  Stats framedStats;
  const auto framedWire=encode(framed,framedStats);
  expect(decode(framedWire)==framed,"I16 framed wire mismatch");
  expect(framedStats.frames>0 && framedStats.frameDiff>0,
         "I16 byte frame predictor not selected");
  Stats stridedStats;""")
ch("Bytes{'A','V','I','5',0,9}", "Bytes{'A','V','I','6',0,9}")
ch("Bytes{'A','V','I','5',0x80}", "Bytes{'A','V','I','6',0x80}")
ch("Bytes{'A','V','I','5',4,3,3}", "Bytes{'A','V','I','6',4,3,3}")
ch('<<" column_diff="<<mixedWalkStats.columnDiff<<"\\n";',
   '<<" column_diff="<<mixedWalkStats.columnDiff<<" frame_blocks="<<framedStats.frames<<"\\n";')
ch("<<initial.columns<<'\\t'<<initial.columnModels<<'\\t'<<initial.diff<<'\\t'<<initial.columnDiff<<'\\t'",
   "<<initial.columns<<'\\t'<<initial.columnModels<<'\\t'<<initial.diff<<'\\t'<<initial.columnDiff<<'\\t'<<initial.frames<<'\\t'<<initial.frameDiff<<'\\t'")
ch("column_diff_models\\tencode_MBps","column_diff_models\\tframed_blocks\\tframed_diff_fields\\tencode_MBps")
ch("I15: --selftest","I16: --selftest")
ch("I15_FAIL","I16_FAIL")
dst=Path("prototypes/i16-byte-frame/i16_byte_frame.cpp")
dst.parent.mkdir(parents=True,exist_ok=True)
dst.write_bytes(s.encode("utf-8"))
print(f"I16 generated {len(s.splitlines())} lines, {len(s)} bytes")
