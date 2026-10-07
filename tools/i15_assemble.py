"""Generate isolated I15 research codec by mechanically extending frozen I13 source.
File transformation only. Never compile or benchmark outside GitHub Actions.
"""
from pathlib import Path

src = Path("prototypes/i13-strided/i13_strided.cpp").read_bytes().decode("utf-8")
assert "\r" not in src

def change(old, new, expected=1):
    global src
    count = src.count(old)
    if count != expected:
        raise ValueError(f"anchor mismatch {count}/{expected}: {old[:70]!r}")
    src = src.replace(old, new)

change("// ANVIL I13", "// ANVIL I15", 1)
change("I13_WITH_BROTLI", "I15_WITH_BROTLI", 4)
start = src.index("// A single 32-bit interleaved column")
end = src.index("struct Stats", start)
replacement = r"""// I15: retain each I13 RAW / AFF column, add exact local modular DIFF.
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
"""
src = src[:start]+replacement+src[end:]
change("struct Stats {uint64_t raw=0,model=0,packed=0,totalBits=0,maxBits=0,columns=0,columnModels=0;};",
       "struct Stats {uint64_t raw=0,model=0,diff=0,packed=0,totalBits=0,maxBits=0,columns=0,columnModels=0,columnDiff=0;};")
change("unsigned modeBits=0;size_t payloadBytes=0;bool modeled=false;",
       "unsigned modeBits=0;size_t payloadBytes=0;bool modeled=false,differential=false;")
change("  // Consider byte-exact 32-bit interleaved fields with RAW fallback per lane.",
r"""  // I15 whole-block local DIFF, with all frozen I13 choices retained.
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
  // Consider byte-exact 32-bit interleaved fields with RAW fallback per lane.""")
change("size_t selectedColumnModels=0;",
       "size_t selectedColumnModels=0,selectedColumnDiff=0;")
change("size_t modeledFields=0;",
       "size_t modeledFields=0,diffFields=0;")
change("bool fieldModeled=false;\n      const Bytes field=encodeColumn(src,records,stride,lane,fieldModeled);",
       "bool fieldModeled=false,fieldDiff=false;\n      const Bytes field=encodeColumn(src,records,stride,lane,fieldModeled,fieldDiff);")
change("modeledFields+=fieldModeled?1:0;",
       "modeledFields+=fieldModeled?1:0;\n      diffFields+=fieldDiff?1:0;")
change("columnsChosen=true;selectedColumnModels=modeledFields;modeled=false;",
       "columnsChosen=true;selectedColumnModels=modeledFields;selectedColumnDiff=diffFields;modeled=false;differential=false;")
change("if(columnsChosen){++s.columns;s.columnModels+=selectedColumnModels;}",
       "if(columnsChosen){++s.columns;s.columnModels+=selectedColumnModels;s.columnDiff+=selectedColumnDiff;}")
change("else if(modeled){++s.model;s.packed+=payloadBytes;",
       "else if(modeled){++s.model;if(differential)++s.diff;s.packed+=payloadBytes;")
change("Bytes out={'A','V','I','3'};", "Bytes out={'A','V','I','5'};")
change("src[3]!='3'", "src[3]!='5'")
change("    }else if(tag==4) {",r"""    }else if(tag==5) {
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
    }else if(tag==4) {""")
change('        } else fail("unknown column tag");',
r"""        } else if(mode==2) {
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
        } else fail("unknown column tag");""")
change("Bytes{'A','V','I','3',0,9}", "Bytes{'A','V','I','5',0,9}")
change("Bytes{'A','V','I','3',0x80}", "Bytes{'A','V','I','5',0x80}")
change("Bytes{'A','V','I','3',4,3,3}", "Bytes{'A','V','I','5',4,3,3}")
change("  cases.push_back(numeric);cases.push_back(jitter);cases.push_back(wrapping);",
r"""  Bytes drifting(8192);
  uint32_t counter=0xfffff000u;
  for(size_t i=0;i<drifting.size()/4;++i) {
    counter+=64u+(i%3==0?1u:0u);
    setWord(drifting.data()+i*4,counter,4);
  }
  cases.push_back(drifting);
  cases.push_back(numeric);cases.push_back(jitter);cases.push_back(wrapping);""")
change("  Stats stridedStats;",
r"""  Stats driftStats;
  const auto driftWire=encode(drifting,driftStats);
  expect(driftStats.diff>0,"I15 global DIFF not selected");
  expect(decode(driftWire)==drifting,"I15 global DIFF mismatch");
  Bytes mixedWalk(7*4*128+13);
  uint64_t mwseed=0xabcddcba12344321ULL;
  uint32_t mwval=0xfff00000u;
  for(size_t i=0;i<128;++i) {
    mwval+=64u+(i%3==0?1u:0u);
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
  Stats stridedStats;""")
change('  std::cout<<"SELFTEST PASS cases="<<cases.size()<<" jitter_model_blocks="<<s1.model<<"\\n";',
       '  std::cout<<"SELFTEST PASS cases="<<cases.size()<<" diff_blocks="<<driftStats.diff<<" column_diff="<<mixedWalkStats.columnDiff<<"\\n";')
change("<<initial.columns<<'\\t'<<initial.columnModels<<'\\t'",
       "<<initial.columns<<'\\t'<<initial.columnModels<<'\\t'<<initial.diff<<'\\t'<<initial.columnDiff<<'\\t'")
change("column_blocks\\tmodeled_columns\\tencode_MBps", "column_blocks\\tmodeled_columns\\tdiff_blocks\\tcolumn_diff_models\\tencode_MBps")
change("I13: --selftest", "I15: --selftest")
change("I13_FAIL", "I15_FAIL")
Path("prototypes/i15-field-diff").mkdir(parents=True,exist_ok=True)
Path("prototypes/i15-field-diff/i15_field_diff.cpp").write_bytes(src.encode("utf-8"))
print("I15 generated:",len(src.splitlines()),"lines")
