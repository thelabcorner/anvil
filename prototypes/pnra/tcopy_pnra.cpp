#include "tcopy_flat_hot_lib.inc"

#ifndef PNRA_EXACT_K
#define PNRA_EXACT_K 4
#endif

// Position-Normalized Relocation Anchor (PNRA) prototype.
// Isolated research tool: exact search stays conventional; transformed-reference
// discovery is sourced from sparse E8/E9 displacement invariants instead of
// approximate byte-string candidates.

struct ExactOnlyIndex {
    const std::vector<uint8_t>& d;
    struct B { std::array<uint32_t,PNRA_EXACT_K> p; B(){p.fill(anvil::kNoPos);} };
    std::vector<B> b;
    std::vector<uint32_t> s4;
    static inline void hashes(const uint8_t* p, uint32_t& h8, uint32_t& h4) {
        uint64_t x; std::memcpy(&x,p,8); uint32_t y=uint32_t(x);
        h4=(y*0x9E3779B1u)>>16;
        x^=x>>32; x*=0x9E3779B185EBCA87ULL;
        h8=uint32_t(x>>(64-anvil::kHashBits));
    }
    explicit ExactOnlyIndex(const std::vector<uint8_t>& x):d(x),b(anvil::kHashSize),s4(1u<<16,anvil::kNoPos){}
    void insert_range(uint32_t a,uint32_t e){
        const uint32_t le=d.size()>=8?uint32_t(d.size()-7):0;
        for(uint32_t p=a;p<e;p++){
            if(p<le){
                uint32_t h8,h4; hashes(d.data()+p,h8,h4); auto& z=b[h8];
                for(int kk=PNRA_EXACT_K-1;kk>0;--kk)z.p[kk]=z.p[kk-1]; z.p[0]=p;s4[h4]=p;
            } else if(p+4<=d.size()) {
                uint32_t y;std::memcpy(&y,d.data()+p,4);s4[(y*0x9E3779B1u)>>16]=p;
            }
        }
    }
    anvil::Match find(uint32_t pos,uint32_t cap) const {
        anvil::Match ex{0,0}; cap=std::min<uint32_t>(cap,d.size()-pos);
        if(pos+8<=d.size()){
            uint32_t h8,h4; hashes(d.data()+pos,h8,h4); const auto& z=b[h8];
            for(uint32_t k=0;k<PNRA_EXACT_K;k++){
                uint32_t q=z.p[k]; if(q==anvil::kNoPos)break; if(q>=pos)continue;
                uint32_t ml=std::min<uint32_t>(cap,d.size()-q);
                uint32_t l=anvil::match_length(d.data()+q,d.data()+pos,ml);
                if(l>=8&&(l>ex.len||(l==ex.len&&pos-q<ex.dist)))ex={l,pos-q};
            }
            if(ex.len<8){
                uint32_t q=s4[h4]; if(q!=anvil::kNoPos&&q<pos){
                    uint32_t l=anvil::match_length(d.data()+q,d.data()+pos,std::min<uint32_t>(15,cap));
                    if(l>=4)ex={l,pos-q};
                }
            }
        } else if(pos+4<=d.size()) {
            uint32_t y;std::memcpy(&y,d.data()+pos,4);uint32_t q=s4[(y*0x9E3779B1u)>>16];
            if(q!=anvil::kNoPos&&q<pos){uint32_t l=anvil::match_length(d.data()+q,d.data()+pos,std::min<uint32_t>(15,cap));if(l>=4)ex={l,pos-q};}
        }
        return ex;
    }
};

#ifndef PNRA_BITS
#define PNRA_BITS 17
#endif
#ifndef PNRA_K
#define PNRA_K 4
#endif
#ifndef PNRA_SCAN
#define PNRA_SCAN 64
#endif
#ifndef PNRA_CTX
#define PNRA_CTX 0
#endif
#ifndef PNRA_MAX_FIELDS
#define PNRA_MAX_FIELDS 0xffffffffu
#endif
#ifndef PNRA_EX_GATE
#define PNRA_EX_GATE 0xffffffffu
#endif
#ifndef PNRA_REPS
#define PNRA_REPS 3
#endif

class PNRA {
public:
    struct Cand{uint32_t pos=0,len=0,dist=0;uint8_t nf=0,np=0;std::array<uint32_t,32> f{};std::array<anvil::GPatch,8> p{};double gain=-1e30;};
private:
    const std::vector<uint8_t>&d;
    struct Ev{uint32_t f,key;};
    std::vector<Ev> ev;
    struct Slot{uint32_t key=0,pos=anvil::kNoPos;};
    struct Bucket{std::array<Slot,PNRA_K>s;};
    std::vector<Bucket> tab;
    size_t hist=0, scan=0;
    static inline uint32_t hh(uint32_t x){x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;x^=x>>16;return x&((1u<<PNRA_BITS)-1);}
    uint32_t make_key(uint32_t f) const {
        uint32_t w=load32(d.data()+f); uint32_t k=uint32_t(w+f);
#if PNRA_CTX == 1
        if(f+6<=d.size()){uint16_t x;std::memcpy(&x,d.data()+f+4,2);k ^= uint32_t(x)*0x9E3779B1u;}
#elif PNRA_CTX == 2
        if(f+8<=d.size()){uint32_t x=load32(d.data()+f+4);k ^= x*0x9E3779B1u;}
#elif PNRA_CTX == 3
        if(f>=5){uint32_t x=load32(d.data()+f-5);k ^= x*0x85ebca6bu;}
#elif PNRA_CTX == 4
        if(f>=5&&f+8<=d.size()){uint32_t a=load32(d.data()+f-5),b=load32(d.data()+f+4);k ^= a*0x85ebca6bu; k ^= std::rotl(b,13)*0xc2b2ae35u;}
#elif PNRA_CTX == 5
        if(f+5<=d.size()) k ^= uint32_t(d[f+4])*0x9E3779B1u;
#endif
        return k;
    }
    void put(uint32_t key,uint32_t f){auto&b=tab[hh(key)];for(int i=PNRA_K-1;i>0;i--)b.s[i]=b.s[i-1];b.s[0]={key,f};}
    Cand verify(uint32_t p,uint32_t q,uint32_t cap) const {
        Cand best; if(q>=p)return best; uint32_t dist=p-q; if(dist<32)return best;
        uint32_t maxl=std::min<uint32_t>({cap,uint32_t(d.size()-p),uint32_t(d.size()-q),dist,65535u});
        if(maxl<32)return best;
        std::array<uint32_t,32> fields{}; std::array<anvil::GPatch,8> patches{}; uint8_t nf=0,np=0;
        uint32_t j=0;
        auto consider=[&](uint32_t L){
            if(L<32||nf<2||np>8)return;
            size_t cmd=1+uvlen(L-8)+uvlen(dist-1)+uvlen(nf)+uvlen(np);
            uint32_t pf=0;for(uint8_t k=0;k<nf;k++){cmd+=uvlen(fields[k]-pf);pf=fields[k]+4;}
            uint32_t pp=0;for(uint8_t k=0;k<np;k++){cmd+=uvlen(patches[k].off-pp)+1;pp=patches[k].off+1;}
            double gain=double(L)-double(cmd)-.35*np;
            if(gain>best.gain){best.pos=q;best.len=L;best.dist=dist;best.nf=nf;best.np=np;best.gain=gain;std::copy_n(fields.data(),nf,best.f.data());std::copy_n(patches.data(),np,best.p.data());}
        };
        while(j<maxl){
            bool found=false;
            while(j+8<=maxl){uint64_t a,b;std::memcpy(&a,d.data()+q+j,8);std::memcpy(&b,d.data()+p+j,8);uint64_t x=a^b;if(!x){j+=8;continue;}j+=uint32_t(std::countr_zero(x)>>3);found=true;break;}
            if(!found){while(j<maxl&&d[q+j]==d[p+j])++j;if(j>=maxl)break;}
            consider(j);
            if(j+4<=maxl&&nf<fields.size()){
                uint32_t a=load32(d.data()+q+j),b=load32(d.data()+p+j);
                if(uint32_t(a-dist)==b){fields[nf++]=j;j+=4;continue;}
            }
            if(np>=8)break;
            patches[np++]={j,d[p+j]};++j;
        }
        consider(j); return best;
    }
public:
    uint64_t query_calls=0, field_probes=0, table_hits=0, verified=0;
    explicit PNRA(const std::vector<uint8_t>&x):d(x),tab(1u<<PNRA_BITS){
        ev.reserve(x.size()/20);
        for(uint32_t op=0;op+5<=x.size();op++)if(x[op]==0xE8||x[op]==0xE9){uint32_t f=op+1;ev.push_back({f,make_key(f)});}    
    }
    size_t event_count() const{return ev.size();}
    void advance_history(uint32_t decoded_end){
        while(hist<ev.size()&&uint64_t(ev[hist].f)+4<=decoded_end){put(ev[hist].key,ev[hist].f);++hist;}
    }
    Cand find(uint32_t p,uint32_t cap){
        ++query_calls;
        while(scan<ev.size()&&ev[scan].f<p)++scan;
        Cand best;
        uint32_t lim=std::min<uint32_t>(uint32_t(d.size()),p+PNRA_SCAN);
        uint32_t used_fields=0; for(size_t z=scan;z<ev.size()&&ev[z].f<lim&&used_fields<PNRA_MAX_FIELDS;z++,used_fields++){
            uint32_t f=ev[z].f; if(f+4>d.size())break; ++field_probes;
            const auto&b=tab[hh(ev[z].key)];
            for(const auto&s:b.s){
                if(s.pos==anvil::kNoPos)break; if(s.key!=ev[z].key||s.pos>=f){continue;}
                ++table_hits;
                uint32_t off=f-p; if(s.pos<off)continue; uint32_t q=s.pos-off; if(q>=p)continue;
                // Corresponding anchor must itself satisfy the TCOPY field algebra.
                uint32_t dist=p-q,a=load32(d.data()+s.pos),bb=load32(d.data()+f);
                if(uint32_t(a-dist)!=bb)continue;
                ++verified; auto c=verify(p,q,cap); if(c.gain>best.gain)best=c;
            }
        }
        return best;
    }
};

static FParse parse_pnra(const std::vector<uint8_t>&d,bool use_raw,bool use_pnra){
    ExactOnlyIndex exidx(d); FlatIndex rawidx(d); FlatGate gate(d); PNRA pn(d);
    FParse r;r.t.reserve(d.size()/8);r.fields.reserve(32768);r.patches.reserve(32768);uint32_t i=0;
    auto lit=[&](uint32_t p){if(!r.t.empty()&&!r.t.back().kind&&r.t.back().pos+r.t.back().len==p)r.t.back().len++;else r.t.push_back({0,p,1,0,0,0,0,0});};
    while(i<d.size()){
        anvil::Match ex{0,0}; FlatIndex::C rawc; PNRA::Cand pc;
        if(use_raw){auto z=rawidx.find(i,65535);ex=z.first;rawc=z.second;} else ex=exidx.find(i,65535);
        if(use_pnra && ex.len < PNRA_EX_GATE)pc=pn.find(i,65535);
        bool exok=ex.len>=5;double eg=exok?double(ex.len)-double(1+uvlen(ex.len-4)+uvlen(ex.dist-1)):-1e30;
        // Pick better transformed candidate across raw and PNRA sources.
        bool choose_p=false; double tg=rawc.gain; uint32_t tlen=rawc.len;
        if(pc.gain>tg){choose_p=true;tg=pc.gain;tlen=pc.len;}
        uint32_t take=1;
        if(tlen>=32&&tg>eg+1){FTok x{};x.kind=2;x.pos=i;
            if(choose_p){x.len=pc.len;x.dist=pc.dist;x.nf=pc.nf;x.np=pc.np;x.fo=r.fields.size();x.po=r.patches.size();r.fields.insert(r.fields.end(),pc.f.begin(),pc.f.begin()+pc.nf);r.patches.insert(r.patches.end(),pc.p.begin(),pc.p.begin()+pc.np);}else{x.len=rawc.len;x.dist=rawc.dist;x.nf=rawc.nf;x.np=rawc.np;x.fo=r.fields.size();x.po=r.patches.size();r.fields.insert(r.fields.end(),rawc.f.begin(),rawc.f.begin()+rawc.nf);r.patches.insert(r.patches.end(),rawc.p.begin(),rawc.p.begin()+rawc.np);}r.t.push_back(x);take=x.len;
        }else if(exok){FTok x{};x.kind=1;x.pos=i;x.len=ex.len;x.dist=ex.dist;r.t.push_back(x);take=ex.len;}else lit(i);
        uint32_t e=std::min<uint32_t>(d.size(),i+take);
        if(use_raw)rawidx.insert_range(i,e);else exidx.insert_range(i,e);
        // PNRA event history is sparse and independent of raw history.
        if(use_pnra)pn.advance_history(e);
        i=e;
    }
    std::cerr<<"PNRA events="<<pn.event_count()<<" queries="<<pn.query_calls<<" probes="<<pn.field_probes<<" hits="<<pn.table_hits<<" verified="<<pn.verified<<"\n";
    return r;
}

static void runone(const std::vector<uint8_t>&d,const char*name,bool raw,bool pn){
    using C=std::chrono::steady_clock;std::vector<double>pt,et,dt;size_t bytes=0,tn=0,tc=0,tf=0;
    for(int z=0;z<PNRA_REPS;z++){
        auto A=C::now();auto p=parse_pnra(d,raw,pn);auto B=C::now();auto enc=enc_flat(d,p);auto D=C::now();auto out=dec_cd(enc.data(),enc.size(),d.size());auto E=C::now();if(out!=d)throw std::runtime_error("roundtrip");
        bytes=enc.size();tn=p.t.size();tc=tf=0;for(auto&x:p.t)if(x.kind==2){tc++;tf+=x.nf;}
        pt.push_back(std::chrono::duration<double>(B-A).count());et.push_back(std::chrono::duration<double>(D-A).count());dt.push_back(std::chrono::duration<double>(E-D).count());
    }
    std::sort(pt.begin(),pt.end());std::sort(et.begin(),et.end());std::sort(dt.begin(),dt.end());
    std::cout<<name<<","<<bytes<<","<<tn<<","<<tc<<","<<tf<<","<<d.size()/1e6/pt[PNRA_REPS/2]<<","<<d.size()/1e6/et[PNRA_REPS/2]<<","<<d.size()/1e6/dt[PNRA_REPS/2]<<"\n";
}

#ifndef TCOPY_PNRA_NO_MAIN
int main(int argc,char**argv){if(argc<2)return 2;auto d=anvil::read_file(argv[1]);std::cout<<"mode,bytes,tokens,tcopy,fields,parse_MBps,enc_MBps,dec_MBps\n";runone(d,"raw",true,false);runone(d,"pnra",false,true);runone(d,"raw+pnra",true,true);}
#endif
