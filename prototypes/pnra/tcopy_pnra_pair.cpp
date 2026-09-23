#define TCOPY_PNRA_NO_MAIN 1
#include "tcopy_pnra.cpp"

#ifndef PPAIR_BITS
#define PPAIR_BITS 18
#endif
#ifndef PPAIR_K
#define PPAIR_K 2
#endif
#ifndef PPAIR_SCAN
#define PPAIR_SCAN 80
#endif
#ifndef PPAIR_GAP
#define PPAIR_GAP 96
#endif
#ifndef PPAIR_MAXQ
#define PPAIR_MAXQ 2
#endif
#ifndef PPAIR_EX_GATE
#define PPAIR_EX_GATE 32
#endif
#ifndef PPAIR_RAW_GATE
#define PPAIR_RAW_GATE 0
#endif

class PNRAPair {
public:
 struct Cand{uint32_t pos=0,len=0,dist=0;uint8_t nf=0,np=0;std::array<uint32_t,32>f{};std::array<anvil::GPatch,8>p{};double gain=-1e30;};
private:
 const std::vector<uint8_t>&d;
 struct Rel{uint32_t f,norm;}; std::vector<Rel> rel;
 struct Pair{uint32_t f1,f2;uint64_t sig;}; std::vector<Pair> pairs;
 struct Slot{uint64_t sig=0;uint32_t f1=anvil::kNoPos;}; struct B{std::array<Slot,PPAIR_K>s;}; std::vector<B>tab;
 size_t hist=0,scan=0;
 static inline uint32_t h64(uint64_t x){x^=x>>33;x*=0xff51afd7ed558ccdULL;x^=x>>33;x*=0xc4ceb9fe1a85ec53ULL;x^=x>>33;return uint32_t(x)&((1u<<PPAIR_BITS)-1);}
 static inline uint64_t sigof(uint32_t a,uint32_t b,uint32_t gap){uint64_t x=(uint64_t(a)<<32)|b; x^=uint64_t(gap)*0x9E3779B97F4A7C15ULL; return x;}
 void put(const Pair&p){auto&b=tab[h64(p.sig)];for(int i=PPAIR_K-1;i>0;i--)b.s[i]=b.s[i-1];b.s[0]={p.sig,p.f1};}
 Cand verify(uint32_t p,uint32_t q,uint32_t cap)const{
  Cand best;if(q>=p)return best;uint32_t dist=p-q;if(dist<32)return best;uint32_t maxl=std::min<uint32_t>({cap,uint32_t(d.size()-p),uint32_t(d.size()-q),dist,65535u});if(maxl<32)return best;
  std::array<uint32_t,32>fs{};std::array<anvil::GPatch,8>ps{};uint8_t nf=0,np=0;uint32_t j=0;
  auto consider=[&](uint32_t L){if(L<32||nf<2||np>8)return;size_t cmd=1+uvlen(L-8)+uvlen(dist-1)+uvlen(nf)+uvlen(np);uint32_t pf=0;for(uint8_t k=0;k<nf;k++){cmd+=uvlen(fs[k]-pf);pf=fs[k]+4;}uint32_t pp=0;for(uint8_t k=0;k<np;k++){cmd+=uvlen(ps[k].off-pp)+1;pp=ps[k].off+1;}double gain=double(L)-double(cmd)-.35*np;if(gain>best.gain){best.pos=q;best.len=L;best.dist=dist;best.nf=nf;best.np=np;best.gain=gain;std::copy_n(fs.data(),nf,best.f.data());std::copy_n(ps.data(),np,best.p.data());}};
  while(j<maxl){bool found=false;while(j+8<=maxl){uint64_t a,b;memcpy(&a,d.data()+q+j,8);memcpy(&b,d.data()+p+j,8);uint64_t x=a^b;if(!x){j+=8;continue;}j+=uint32_t(std::countr_zero(x)>>3);found=true;break;}if(!found){while(j<maxl&&d[q+j]==d[p+j])++j;if(j>=maxl)break;}consider(j);if(j+4<=maxl&&nf<fs.size()){uint32_t a=load32(d.data()+q+j),b=load32(d.data()+p+j);if(uint32_t(a-dist)==b){fs[nf++]=j;j+=4;continue;}}if(np>=8)break;ps[np++]={j,d[p+j]};++j;}consider(j);return best;
 }
public:
 uint64_t queries=0,pairprobes=0,hits=0,verifies=0;
 explicit PNRAPair(const std::vector<uint8_t>&x):d(x),tab(1u<<PPAIR_BITS){
  rel.reserve(x.size()/20);for(uint32_t op=0;op+5<=x.size();op++)if(x[op]==0xe8||x[op]==0xe9){uint32_t f=op+1,w=load32(x.data()+f);rel.push_back({f,uint32_t(w+f)});}pairs.reserve(rel.size());for(size_t i=0;i+1<rel.size();i++){auto&a=rel[i];auto&b=rel[i+1];uint32_t gap=b.f-a.f;if(gap<=PPAIR_GAP)pairs.push_back({a.f,b.f,sigof(a.norm,b.norm,gap)});}
 }
 void advance(uint32_t e){while(hist<pairs.size()&&uint64_t(pairs[hist].f2)+4<=e){put(pairs[hist]);++hist;}}
 Cand find(uint32_t p,uint32_t cap){++queries;while(scan<pairs.size()&&pairs[scan].f1<p)++scan;Cand best;uint32_t lim=std::min<uint32_t>(d.size(),p+PPAIR_SCAN);uint32_t nq=0;for(size_t z=scan;z<pairs.size()&&pairs[z].f1<lim&&nq<PPAIR_MAXQ;z++,nq++){++pairprobes;const auto&b=tab[h64(pairs[z].sig)];for(auto&s:b.s){if(s.f1==anvil::kNoPos)break;if(s.sig!=pairs[z].sig||s.f1>=pairs[z].f1)continue;++hits;uint32_t off=pairs[z].f1-p;if(s.f1<off)continue;uint32_t q=s.f1-off;if(q>=p)continue;++verifies;auto c=verify(p,q,cap);if(c.gain>best.gain)best=c;}}return best;}
 size_t pair_count()const{return pairs.size();}
};

static FParse parse_pair(const std::vector<uint8_t>&d){ExactOnlyIndex idx(d);PNRAPair pn(d);FlatGate rg(d);FParse r;r.t.reserve(d.size()/8);r.fields.reserve(32768);r.patches.reserve(32768);uint32_t i=0;auto lit=[&](uint32_t p){if(!r.t.empty()&&!r.t.back().kind&&r.t.back().pos+r.t.back().len==p)r.t.back().len++;else r.t.push_back({0,p,1,0,0,0,0,0});};while(i<d.size()){anvil::Match ex{0,0}; bool raw_plausible=true; if constexpr(PPAIR_RAW_GATE) raw_plausible=rg.plausible(i); if(raw_plausible) ex=idx.find(i,65535); PNRAPair::Cand tc;if(ex.len<PPAIR_EX_GATE)tc=pn.find(i,65535);bool exok=ex.len>=5;double eg=exok?double(ex.len)-double(1+uvlen(ex.len-4)+uvlen(ex.dist-1)):-1e30;uint32_t take=1;if(tc.len>=32&&tc.gain>eg+1){FTok x{};x.kind=2;x.pos=i;x.len=tc.len;x.dist=tc.dist;x.nf=tc.nf;x.np=tc.np;x.fo=r.fields.size();x.po=r.patches.size();r.fields.insert(r.fields.end(),tc.f.begin(),tc.f.begin()+tc.nf);r.patches.insert(r.patches.end(),tc.p.begin(),tc.p.begin()+tc.np);r.t.push_back(x);take=x.len;}else if(exok){FTok x{};x.kind=1;x.pos=i;x.len=ex.len;x.dist=ex.dist;r.t.push_back(x);take=ex.len;}else lit(i);uint32_t e=std::min<uint32_t>(d.size(),i+take);idx.insert_range(i,e); if constexpr(PPAIR_RAW_GATE) rg.insert_range(i,e); pn.advance(e);i=e;}std::cerr<<"pairs="<<pn.pair_count()<<" queries="<<pn.queries<<" probes="<<pn.pairprobes<<" hits="<<pn.hits<<" verifies="<<pn.verifies<<"\n";return r;}
int main(int argc,char**argv){auto d=anvil::read_file(argv[1]);using C=std::chrono::steady_clock;std::vector<double>p,e,dd;size_t bytes=0,tn=0,tc=0,tf=0;for(int z=0;z<3;z++){auto A=C::now();auto r=parse_pair(d);auto B=C::now();auto enc=enc_flat(d,r);auto D=C::now();auto out=dec_cd(enc.data(),enc.size(),d.size());auto E=C::now();if(out!=d)abort();bytes=enc.size();tn=r.t.size();tc=tf=0;for(auto&t:r.t)if(t.kind==2){tc++;tf+=t.nf;}p.push_back(std::chrono::duration<double>(B-A).count());e.push_back(std::chrono::duration<double>(D-A).count());dd.push_back(std::chrono::duration<double>(E-D).count());}std::sort(p.begin(),p.end());std::sort(e.begin(),e.end());std::sort(dd.begin(),dd.end());std::cout<<bytes<<","<<tn<<","<<tc<<","<<tf<<","<<d.size()/1e6/p[1]<<","<<d.size()/1e6/e[1]<<","<<d.size()/1e6/dd[1]<<","<<brotli_size(d,4)<<"\n";}
