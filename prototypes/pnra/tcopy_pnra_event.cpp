#define TCOPY_PNRA_NO_MAIN 1
#include "tcopy_pnra.cpp"

#ifndef EV_BITS
#define EV_BITS 19
#endif
#ifndef EV_BACK
#define EV_BACK 64
#endif
#ifndef EV_GAP
#define EV_GAP 96
#endif
#ifndef EV_EXACT_K
#define EV_EXACT_K 3
#endif

struct EvCand {uint32_t start=0,src=0,len=0,dist=0,fo=0,po=0;uint8_t nf=0,np=0;double gain=-1e30;};
struct EvBook {std::vector<int32_t> at;std::vector<EvCand> c;std::vector<uint32_t> fields;std::vector<anvil::GPatch> patches;uint64_t pairs=0,hits=0,verified=0,accepted=0;};

struct PairEventBuilder {
 const std::vector<uint8_t>&d;
 struct Rel{uint32_t f,norm;uint8_t op;};
 struct Pair{uint32_t f1,f2;uint64_t sig;};
 struct Slot{uint64_t sig=0;uint32_t f1=anvil::kNoPos,f2=anvil::kNoPos;};
 std::vector<Rel> rel;std::vector<Pair> pairs;std::vector<Slot> tab;
 static inline uint32_t hh(uint64_t x){x^=x>>33;x*=0xff51afd7ed558ccdULL;x^=x>>33;x*=0xc4ceb9fe1a85ec53ULL;x^=x>>33;return uint32_t(x)&((1u<<EV_BITS)-1);}
 static inline uint64_t sigof(uint32_t a,uint32_t b,uint32_t gap,uint8_t opa,uint8_t opb){uint64_t x=(uint64_t(a)<<32)|b;x^=uint64_t(gap)*0x9E3779B97F4A7C15ULL;x^=uint64_t(opa==0xe9)*0xD6E8FEB86659FD93ULL;x^=uint64_t(opb==0xe9)*0xA5A3564E27F88693ULL;return x;}
 explicit PairEventBuilder(const std::vector<uint8_t>&x):d(x),tab(1u<<EV_BITS){
  rel.reserve(x.size()/20);for(uint32_t op=0;op+5<=x.size();op++)if(x[op]==0xe8||x[op]==0xe9){uint32_t f=op+1,w=load32(x.data()+f);rel.push_back({f,uint32_t(w+f),x[op]});}
  pairs.reserve(rel.size());for(size_t i=0;i+1<rel.size();i++){auto&a=rel[i];auto&b=rel[i+1];uint32_t gap=b.f-a.f;if(gap<=EV_GAP)pairs.push_back({a.f,b.f,sigof(a.norm,b.norm,gap,a.op,b.op)});}
 }
 EvCand verify(uint32_t p,uint32_t q,uint32_t cap,std::array<uint32_t,32>&fs,std::array<anvil::GPatch,8>&ps)const{
  EvCand best;if(q>=p)return best;uint32_t dist=p-q;if(dist<32)return best;uint32_t maxl=std::min<uint32_t>({cap,uint32_t(d.size()-p),uint32_t(d.size()-q),dist,65535u});if(maxl<32)return best;uint8_t nf=0,np=0;uint32_t j=0;
  auto consider=[&](uint32_t L){if(L<32||nf<2||np>8)return;size_t cmd=1+uvlen(L-8)+uvlen(dist-1)+uvlen(nf)+uvlen(np);uint32_t pf=0;for(uint8_t k=0;k<nf;k++){cmd+=uvlen(fs[k]-pf);pf=fs[k]+4;}uint32_t pp=0;for(uint8_t k=0;k<np;k++){cmd+=uvlen(ps[k].off-pp)+1;pp=ps[k].off+1;}double g=double(L)-double(cmd)-.35*np;if(g>best.gain){best.start=p;best.src=q;best.len=L;best.dist=dist;best.nf=nf;best.np=np;best.gain=g;}};
  while(j<maxl){bool found=false;while(j+8<=maxl){uint64_t a,b;memcpy(&a,d.data()+q+j,8);memcpy(&b,d.data()+p+j,8);uint64_t x=a^b;if(!x){j+=8;continue;}j+=uint32_t(std::countr_zero(x)>>3);found=true;break;}if(!found){while(j<maxl&&d[q+j]==d[p+j])++j;if(j>=maxl)break;}consider(j);if(j+4<=maxl&&nf<fs.size()){uint32_t a=load32(d.data()+q+j),b=load32(d.data()+p+j);if(uint32_t(a-dist)==b){fs[nf++]=j;j+=4;continue;}}if(np>=8)break;ps[np++]={j,d[p+j]};++j;}consider(j);return best;
 }
 EvBook build(){EvBook out;out.at.assign(d.size(),-1);out.pairs=pairs.size();out.c.reserve(pairs.size()/8);out.fields.reserve(65536);out.patches.reserve(32768);for(auto&pe:pairs){auto&sl=tab[hh(pe.sig)];if(sl.f1!=anvil::kNoPos&&sl.sig==pe.sig){out.hits++;uint32_t g1=sl.f1,g2=sl.f2;if(g2-g1==pe.f2-pe.f1&&g1<pe.f1){uint32_t dist=pe.f1-g1; // exact-backward extension; pair opcode is part of signature.
      uint32_t back=0,limit=std::min<uint32_t>({uint32_t(EV_BACK),pe.f1,g1});while(back<limit&&d[pe.f1-1-back]==d[g1-1-back])back++;uint32_t p=pe.f1-back,q=g1-back;if(p>q&&p<d.size()){std::array<uint32_t,32>fs{};std::array<anvil::GPatch,8>ps{};out.verified++;auto c=verify(p,q,65535,fs,ps);if(c.len>=32&&c.gain>0){int32_t old=out.at[p];if(old<0||c.gain>out.c[old].gain){EvCand ec=c;ec.fo=out.fields.size();ec.po=out.patches.size();out.fields.insert(out.fields.end(),fs.begin(),fs.begin()+c.nf);out.patches.insert(out.patches.end(),ps.begin(),ps.begin()+c.np);out.c.push_back(ec);out.at[p]=int32_t(out.c.size()-1);out.accepted++;}}}}}sl={pe.sig,pe.f1,pe.f2};}return out;}
};

template<int K> struct CompactExact {
 const std::vector<uint8_t>&d;struct B{std::array<uint32_t,K>p;B(){p.fill(anvil::kNoPos);}};std::vector<B>b;std::vector<uint32_t>s4;
 static inline void hashes(const uint8_t*p,uint32_t&h8,uint32_t&h4){uint64_t x;memcpy(&x,p,8);uint32_t y=uint32_t(x);h4=(y*0x9E3779B1u)>>16;x^=x>>32;x*=0x9E3779B185EBCA87ULL;h8=uint32_t(x>>(64-anvil::kHashBits));}
 CompactExact(const std::vector<uint8_t>&x):d(x),b(anvil::kHashSize),s4(1u<<16,anvil::kNoPos){}
 void insert_range(uint32_t a,uint32_t e){for(uint32_t p=a;p<e;p++){if(p+8<=d.size()){uint32_t h8,h4;hashes(d.data()+p,h8,h4);auto&z=b[h8];for(int k=K-1;k>0;k--)z.p[k]=z.p[k-1];z.p[0]=p;s4[h4]=p;}else if(p+4<=d.size()){uint32_t y;memcpy(&y,d.data()+p,4);s4[(y*0x9E3779B1u)>>16]=p;}}}
 anvil::Match find(uint32_t pos,uint32_t cap)const{anvil::Match ex{0,0};cap=std::min<uint32_t>(cap,d.size()-pos);if(pos+8<=d.size()){uint32_t h8,h4;hashes(d.data()+pos,h8,h4);for(auto q:b[h8].p){if(q==anvil::kNoPos)break;if(q>=pos)continue;uint32_t l=anvil::match_length(d.data()+q,d.data()+pos,std::min<uint32_t>(cap,d.size()-q));if(l>=8&&(l>ex.len||(l==ex.len&&pos-q<ex.dist)))ex={l,pos-q};}if(ex.len<8){uint32_t q=s4[h4];if(q!=anvil::kNoPos&&q<pos){uint32_t l=anvil::match_length(d.data()+q,d.data()+pos,std::min<uint32_t>(15,cap));if(l>=4)ex={l,pos-q};}}}return ex;}
};

static FParse parse_event(const std::vector<uint8_t>&d,const EvBook&book){CompactExact<EV_EXACT_K>idx(d);FParse r;r.t.reserve(d.size()/8);r.fields.reserve(book.fields.size());r.patches.reserve(book.patches.size());uint32_t i=0;auto lit=[&](uint32_t p){if(!r.t.empty()&&!r.t.back().kind&&r.t.back().pos+r.t.back().len==p)r.t.back().len++;else r.t.push_back({0,p,1,0,0,0,0,0});};while(i<d.size()){auto ex=idx.find(i,65535);bool exok=ex.len>=5;double eg=exok?double(ex.len)-double(1+uvlen(ex.len-4)+uvlen(ex.dist-1)):-1e30;const EvCand*tc=nullptr;int32_t id=book.at[i];if(id>=0)tc=&book.c[id];uint32_t take=1;if(tc&&tc->gain>eg+1){FTok x{};x.kind=2;x.pos=i;x.len=tc->len;x.dist=tc->dist;x.nf=tc->nf;x.np=tc->np;x.fo=r.fields.size();x.po=r.patches.size();r.fields.insert(r.fields.end(),book.fields.begin()+tc->fo,book.fields.begin()+tc->fo+tc->nf);r.patches.insert(r.patches.end(),book.patches.begin()+tc->po,book.patches.begin()+tc->po+tc->np);r.t.push_back(x);take=x.len;}else if(exok){FTok x{};x.kind=1;x.pos=i;x.len=ex.len;x.dist=ex.dist;r.t.push_back(x);take=ex.len;}else lit(i);uint32_t e=std::min<uint32_t>(d.size(),i+take);idx.insert_range(i,e);i=e;}return r;}
int main(int argc,char**argv){auto d=anvil::read_file(argv[1]);using C=std::chrono::steady_clock;std::vector<double>bt,pt,et,dt;size_t bytes=0,tn=0,tc=0,tf=0;uint64_t np=0,nh=0,nv=0,na=0;for(int z=0;z<5;z++){PairEventBuilder pb(d);auto A=C::now();auto book=pb.build();auto B=C::now();auto r=parse_event(d,book);auto D=C::now();auto enc=enc_flat(d,r);auto E=C::now();auto out=dec_cd(enc.data(),enc.size(),d.size());auto F=C::now();if(out!=d)abort();bytes=enc.size();tn=r.t.size();tc=tf=0;for(auto&t:r.t)if(t.kind==2){tc++;tf+=t.nf;}np=book.pairs;nh=book.hits;nv=book.verified;na=book.accepted;bt.push_back(std::chrono::duration<double>(B-A).count());pt.push_back(std::chrono::duration<double>(D-A).count());et.push_back(std::chrono::duration<double>(E-A).count());dt.push_back(std::chrono::duration<double>(F-E).count());}auto md=[](std::vector<double>v){sort(v.begin(),v.end());return v[v.size()/2];};std::cout<<bytes<<","<<tn<<","<<tc<<","<<tf<<","<<np<<","<<nh<<","<<nv<<","<<na<<","<<d.size()/1e6/md(bt)<<","<<d.size()/1e6/md(pt)<<","<<d.size()/1e6/md(et)<<","<<d.size()/1e6/md(dt)<<","<<brotli_size(d,4)<<"\n";}
