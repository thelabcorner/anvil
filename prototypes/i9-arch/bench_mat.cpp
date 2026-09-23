#define ANVIL_NO_MAIN
#include "../../src/anvil.cpp"
#include <brotli/encode.h>
#include <brotli/decode.h>
#include <zstd.h>
#include <iomanip>
#include <functional>

using Clock = std::chrono::steady_clock;

struct Row { std::string codec; size_t bytes; double ratio; double enc_mbs; double dec_mbs; bool ok; };

template <class F>
static double median_speed(size_t input_bytes, int reps, F&& fn) {
    std::vector<double> samples; samples.reserve(reps);
    for(int i=0;i<reps;++i) {
        auto t0=Clock::now(); fn(); auto t1=Clock::now();
        double sec=std::chrono::duration<double>(t1-t0).count();
        samples.push_back(sec>0 ? input_bytes/1e6/sec : 0);
    }
    std::sort(samples.begin(),samples.end()); return samples[samples.size()/2];
}

static Row bench_anvil(const std::vector<uint8_t>& src, std::string name, std::string parse, std::string lit, std::string entropy, int reps, uint32_t shape_states=28, double stream_lambda=0.04, bool channels=false, bool pnra=false, bool hotop_rlzp=false, bool hotop_budget=false) {
    anvil::Options o; o.parse=parse; o.literal=lit; o.entropy=entropy; o.quiet=true; o.shape_states=shape_states;
    o.channels = channels; // R4 structural-distance channels (research row)
    o.pnra = pnra; // Experiment X: PNRA invariant-anchored candidate source (mode 14 only)
    o.hotop_rlzp = hotop_rlzp; // Experiment Y: RLZ/RePair book-stream codecs (modes 7/8; --hotop-rlzp=on)
    o.hotop_budget = hotop_budget; // S6-1: whole-codec stream budget on mode-15 book streams (--hotop-budget=on)
    anvil::g_stream_lambda = stream_lambda; // stream-suite J weight (ANVIL_STREAM_LAMBDA in CLI main; set directly here)
    anvil::GlobalStats st; auto packed=anvil::compress(src,o,&st); auto unpacked=anvil::decompress(packed,o);
    double enc=median_speed(src.size(),reps,[&]{ anvil::GlobalStats x; auto y=anvil::compress(src,o,&x); volatile size_t sink=y.size(); (void)sink; });
    double dec=median_speed(src.size(),reps,[&]{ auto y=anvil::decompress(packed,o); volatile size_t sink=y.size(); (void)sink; });
    return {std::move(name),packed.size(),src.empty()?0.0:double(packed.size())/src.size(),enc,dec,unpacked==src};
}

static Row bench_brotli(const std::vector<uint8_t>& src, int q, int reps) {
    size_t cap=std::max<size_t>(1,BrotliEncoderMaxCompressedSize(src.size())); std::vector<uint8_t> packed(cap); size_t n=cap;
    BROTLI_BOOL ok=BrotliEncoderCompress(q,BROTLI_DEFAULT_WINDOW,BROTLI_MODE_GENERIC,src.size(),src.data(),&n,packed.data());
    if(!ok) throw std::runtime_error("brotli encode failed");
    packed.resize(n);
    std::vector<uint8_t> unpacked(src.size()); size_t dn=unpacked.size(); auto dr=BrotliDecoderDecompress(packed.size(),packed.data(),&dn,unpacked.data());
    double enc=median_speed(src.size(),reps,[&]{ size_t c=cap; std::vector<uint8_t> y(c); if(!BrotliEncoderCompress(q,BROTLI_DEFAULT_WINDOW,BROTLI_MODE_GENERIC,src.size(),src.data(),&c,y.data())) throw std::runtime_error("brotli encode failed"); volatile size_t sink=c; (void)sink; });
    double dec=median_speed(src.size(),reps,[&]{ size_t c=src.size(); std::vector<uint8_t> y(c); auto r=BrotliDecoderDecompress(packed.size(),packed.data(),&c,y.data()); if(r!=BROTLI_DECODER_RESULT_SUCCESS)throw std::runtime_error("brotli decode failed"); volatile size_t sink=c; (void)sink; });
    return {"brotli-q"+std::to_string(q),packed.size(),src.empty()?0.0:double(packed.size())/src.size(),enc,dec,dr==BROTLI_DECODER_RESULT_SUCCESS && dn==src.size() && unpacked==src};
}

static Row bench_zstd(const std::vector<uint8_t>& src, int lvl, int reps) {
    size_t cap=ZSTD_compressBound(src.size()); std::vector<uint8_t> packed(cap); size_t n=ZSTD_compress(packed.data(),cap,src.data(),src.size(),lvl); if(ZSTD_isError(n))throw std::runtime_error(ZSTD_getErrorName(n)); packed.resize(n);
    std::vector<uint8_t> unpacked(src.size()); size_t dn=ZSTD_decompress(unpacked.data(),unpacked.size(),packed.data(),packed.size()); if(ZSTD_isError(dn))throw std::runtime_error(ZSTD_getErrorName(dn));
    double enc=median_speed(src.size(),reps,[&]{ std::vector<uint8_t> y(cap); size_t c=ZSTD_compress(y.data(),cap,src.data(),src.size(),lvl); if(ZSTD_isError(c))throw std::runtime_error(ZSTD_getErrorName(c)); volatile size_t sink=c; (void)sink; });
    double dec=median_speed(src.size(),reps,[&]{ std::vector<uint8_t> y(src.size()); size_t c=ZSTD_decompress(y.data(),y.size(),packed.data(),packed.size()); if(ZSTD_isError(c))throw std::runtime_error(ZSTD_getErrorName(c)); volatile size_t sink=c; (void)sink; });
    return {"zstd-"+std::to_string(lvl),packed.size(),src.empty()?0.0:double(packed.size())/src.size(),enc,dec,dn==src.size() && unpacked==src};
}

int main(int argc,char**argv) {
    try {
        if(argc<2){std::cerr<<"usage: bench_native <file> [reps]\n";return 2;}
        auto src=anvil::read_file(argv[1]); int reps=argc>=3?std::max(1,std::stoi(argv[2])):5;
        std::vector<Row> rows;
        rows.push_back(bench_anvil(src,"anvil-greedy-arith","greedy","o0","arith",reps));
        rows.push_back(bench_anvil(src,"anvil-dp-arith","dp","o0","arith",reps));
        rows.push_back(bench_anvil(src,"anvil-greedy-rans","greedy","o0","rans",reps));
        rows.push_back(bench_anvil(src,"anvil-dp-rans","dp","o0","rans",reps));
        rows.push_back(bench_anvil(src,"anvil-sparse-rans","sparse","o0","rans",reps));
        rows.push_back(bench_anvil(src,"anvil-sparse-rans-l0","sparse","o0","rans",reps,28,0.0));
        rows.push_back(bench_anvil(src,"anvil-sparse-channels-rans","sparse","o0","rans",reps,28,0.04,true));
        rows.push_back(bench_anvil(src,"anvil-tcopy-rans","tcopy","o0","rans",reps));
        rows.push_back(bench_anvil(src,"anvil-tcopy-pnra-rans","tcopy","o0","rans",reps,28,0.04,false,true));
        rows.push_back(bench_anvil(src,"anvil-mdl-rans","mdl","o0","rans",reps));
        rows.push_back(bench_anvil(src,"anvil-mdl-rans-l0","mdl","o0","rans",reps,28,0.0));
        rows.push_back(bench_anvil(src,"anvil-mdl-rans-l001","mdl","o0","rans",reps,28,0.01));
        rows.push_back(bench_anvil(src,"anvil-shape-rans","shape","o0","rans",reps,28));
        rows.push_back(bench_anvil(src,"anvil-shape-rans-l0","shape","o0","rans",reps,28,0.0));
        rows.push_back(bench_anvil(src,"anvil-shape-ctxmap-rans","shape","o0","rans",reps,1));
        rows.push_back(bench_anvil(src,"anvil-hotop-rans","hotop","o0","rans",reps));
        rows.push_back(bench_anvil(src,"anvil-hotop-budget-rans","hotop","o0","rans",reps,28,0.04,false,false,false,true)); // S6-1 verdict row: whole-codec stream budget (gate rule Y-1: rlzp=off). Expected byte-equal to anvil-hotop-rans — that equality IS the verdict evidence.
        rows.push_back(bench_anvil(src,"anvil-hotop-rlzp-rans","hotop","o0","rans",reps,28,0.04,false,false,true)); // Experiment Y record row (gate rule Y-1: NOT part of the S6-1 verdict; S6-1 verdict rows all run rlzp=off)
        for(int q: {1,4,6,9,11}) rows.push_back(bench_brotli(src,q,reps));
        for(int l: {1,3,9,19}) rows.push_back(bench_zstd(src,l,reps));
        std::cout<<"input_bytes,"<<src.size()<<"\n";
        std::cout<<"codec,compressed_bytes,ratio,encode_MBps,decode_MBps,roundtrip\n";
        std::cout<<std::fixed<<std::setprecision(3);
        for(auto&r:rows) std::cout<<r.codec<<','<<r.bytes<<','<<r.ratio<<','<<r.enc_mbs<<','<<r.dec_mbs<<','<<(r.ok?"OK":"FAIL")<<'\n';
        return 0;
    } catch(const std::exception&e){std::cerr<<"bench: "<<e.what()<<"\n";return 1;}
}
