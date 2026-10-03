#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>

static uint32_t rd32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }

static void wr32(std::vector<uint8_t>& o, uint32_t v) { for (int i = 0; i < 4; i++) o.push_back(uint8_t(v >> (8 * i))); }

static void putvar(std::vector<uint8_t>& o, uint64_t v) { while (v >= 0x80) { o.push_back(uint8_t(v) | 0x80); v >>= 7; } o.push_back(uint8_t(v)); }
static uint64_t getvar(const uint8_t* p, size_t& i, size_t n) {
    uint64_t v = 0; int s = 0;
    while (i < n) { uint8_t b = p[i++]; v |= uint64_t(b & 0x7F) << s; s += 7; if (!(b & 0x80)) break; }
    return v;
}

struct Sec {
    std::string name;
    uint64_t rawStart = 0, rawEnd = 0, rva = 0;
};

struct Image {
    std::vector<uint8_t> d;
    std::vector<Sec> secs;
    std::vector<int32_t> secOf;
    bool isPE = false;
    int64_t addr(uint64_t p) const {
        int32_t s = secOf[p];
        if (s < 0) return 0;
        return int64_t(secs[(size_t)s].rva + (p - secs[(size_t)s].rawStart));
    }
    int secIdx(uint64_t p) const { return p < secOf.size() ? secOf[p] : -1; }
};

static bool loadPE(Image& im) {
    const std::vector<uint8_t>& d = im.d;
    if (d.size() < 0x200) return false;
    if (!(d[0] == 'M' && d[1] == 'Z')) return false;
    uint32_t lfa = rd32(&d[0x3C]);
    if (lfa + 24 > d.size()) return false;
    if (!(d[lfa] == 'P' && d[lfa + 1] == 'E' && d[lfa + 2] == 0 && d[lfa + 3] == 0)) return false;
    uint16_t nsec = uint16_t(d[lfa + 6] | (d[lfa + 7] << 8));
    uint16_t optsz = uint16_t(d[lfa + 20] | (d[lfa + 21] << 8));
    uint64_t st = lfa + 24 + optsz;
    for (uint16_t i = 0; i < nsec; i++) {
        const uint8_t* e = &d[st + 40ull * i];
        if (st + 40ull * i + 40 > d.size()) break;
        char nm[9] = {0};
        std::memcpy(nm, e, 8);
        Sec s;
        s.name = nm;
        uint32_t vsz = rd32(e + 8), rva = rd32(e + 12), rsz = rd32(e + 16), praw = rd32(e + 20);
        s.rva = rva;
        if (praw == 0 || rsz == 0) continue;
        s.rawStart = praw;
        s.rawEnd = std::min<uint64_t>(praw + rsz, d.size());
        if (s.rawStart >= s.rawEnd) continue;
        (void)vsz;
        im.secs.push_back(s);
    }
    im.isPE = !im.secs.empty();
    return im.isPE;
}

static void buildSecOf(Image& im) {
    size_t n = im.d.size();
    im.secOf.assign(n, -1);
    for (size_t i = 0; i < im.secs.size(); i++)
        for (uint64_t p = im.secs[i].rawStart; p < im.secs[i].rawEnd; p++) im.secOf[p] = int32_t(i);
}

static void loadFile(Image& im, const char* path) {
    FILE* f = std::fopen(path, "rb");
    if (!f) { std::fprintf(stderr, "open failed: %s\n", path); std::exit(2); }
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    im.d.resize(size_t(n));
    if (n > 0 && std::fread(im.d.data(), 1, size_t(n), f) != size_t(n)) { std::fprintf(stderr, "short read\n"); std::exit(2); }
    std::fclose(f);
    loadPE(im);
    buildSecOf(im);
}

struct Run {
    uint64_t p = 0, q = 0;
    uint32_t slots = 0;
    uint8_t mode = 0;
};

struct HashTab {
    std::vector<uint64_t> key;
    std::vector<uint32_t> val;
    std::vector<uint32_t> next;
    uint64_t mask = 0;
    void init(uint64_t capPow2, uint32_t n) {
        uint64_t sz = 1; while (sz < capPow2) sz <<= 1;
        key.assign(size_t(sz), 0);
        val.assign(size_t(sz), 0);
        next.assign(size_t(n + 1), 0);
        mask = sz - 1;
    }
    static uint64_t mix(uint64_t x) {
        x ^= x >> 33; x *= 0xff51afd7ed558ccdULL;
        x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL;
        x ^= x >> 33; return x;
    }
    void insert(uint64_t k, uint32_t pos) {
        uint64_t h = mix(k) & mask;
        next[pos + 1] = val[h];
        val[h] = pos + 1;
        key[h] = k;
    }
    uint32_t chain(uint64_t k) const {
        uint64_t h = mix(k) & mask;
        if (key[h] != k) return 0;
        return val[h];
    }
};

struct Stats {
    uint64_t runBytes = 0, runCount = 0, runBytesV1 = 0, runBytesCross = 0, runBytesMode0 = 0;
    uint32_t maxSlots = 0;
    uint64_t slotHist[9] = {0};
    uint64_t tokenCost = 0;
    uint64_t perSecBytes[16] = {0};
    uint64_t perSecCount[16] = {0};
    uint64_t secCount = 0;
};

static uint64_t invariant(const Image& im, uint64_t p) {
    return uint64_t(rd32(&im.d[p])) + uint64_t(im.addr(p));
}

static void growRun(const Image& im, uint64_t p, uint64_t q, uint32_t& lenAll, uint32_t& lenNz) {
    size_t n = im.d.size();
    lenAll = 0; lenNz = 0;
    uint32_t la = 0, ln = 0;
    for (uint32_t i = 0; ; i++) {
        uint64_t ap = p + 4ull * i, bq = q + 4ull * i;
        if (ap + 4 > n || bq + 4 > n || bq + 4 > p) break;
        uint32_t dv = rd32(&im.d[ap]), sv = rd32(&im.d[bq]);
        bool okAll = (uint64_t(dv) + uint64_t(im.addr(ap))) == (uint64_t(sv) + uint64_t(im.addr(bq)));
        bool okNz = ((sv == 0 && dv == 0) || okAll);
        if (okAll) la = i + 1; else break;
        if (okNz) ln = i + 1; else break;
    }
    lenAll = la; lenNz = ln;
}

static uint64_t varintBytes(uint64_t v) { uint64_t n = 1; while (v >= 0x80) { n++; v >>= 7; } return n; }

static Stats analyze(Image& im, uint32_t chainCap, int minSlots) {
    Stats st;
    size_t n = im.d.size();
    st.secCount = im.secs.size();
    HashTab ht;
    uint64_t nSlots = (uint64_t)(n / 4) + 4;
    ht.init(nSlots * 2, uint32_t(n + 1));
    std::vector<uint32_t> aligned;
    aligned.reserve(size_t(nSlots));
    for (size_t p = 0; p + 4 <= n; p += 4) {
        if (im.secIdx(uint64_t(p)) < 0) continue;
        aligned.push_back(uint32_t(p));
    }
    std::vector<char> covered(n, 0);
    for (size_t i = 0; i < aligned.size(); i++) {
        uint64_t p = aligned[i];
        uint32_t bestSlots = 0, bestQ = 0; uint8_t bestMode = 0;
        uint32_t tried = 0;
        for (uint32_t cur = ht.chain(invariant(im, p)); cur && tried < chainCap; cur = ht.next[cur]) {
            uint64_t q = cur - 1;
            if (q >= p) continue;
            uint32_t la, ln;
            growRun(im, p, q, la, ln);
            int32_t sP = im.secIdx(p), sQ = im.secIdx(q);
            bool cross = (sP != sQ);
            if (sP >= 0 && sQ >= 0 && !cross) {
                if (la > bestSlots) { bestSlots = la; bestQ = uint32_t(q); bestMode = 0; }
                if (ln > bestSlots) { bestSlots = ln; bestQ = uint32_t(q); bestMode = 1; }
            }
            tried++;
        }
        (void)bestSlots; (void)bestQ; (void)bestMode;
        ht.insert(invariant(im, p), uint32_t(p));
    }
    for (size_t i = 0; i < aligned.size(); i++) {
        uint64_t p = aligned[i];
        if (p + 4 > n || covered[p]) continue;
        uint32_t bestSlots = 0, bestQ = 0; uint8_t bestMode = 0;
        uint32_t tried = 0;
        for (uint32_t cur = ht.chain(invariant(im, p)); cur && tried < chainCap; cur = ht.next[cur]) {
            uint64_t q = cur - 1;
            if (q >= p) continue;
            uint32_t la, ln;
            growRun(im, p, q, la, ln);
            int32_t sP = im.secIdx(p), sQ = im.secIdx(q);
            if (sP >= 0 && sQ >= 0 && sP == sQ) {
                if (la > bestSlots) { bestSlots = la; bestQ = uint32_t(q); bestMode = 0; }
                if (ln > bestSlots) { bestSlots = ln; bestQ = uint32_t(q); bestMode = 1; }
            }
            tried++;
        }
        if (bestSlots < uint32_t(minSlots) || !bestQ) continue;
        uint64_t L = 4ull * bestSlots;
        st.runBytes += L;
        st.runCount++;
        st.maxSlots = std::max(st.maxSlots, bestSlots);
        st.slotHist[std::min<uint32_t>(bestSlots, 8)]++;
        if (bestMode == 0) st.runBytesMode0 += L;
        int32_t sP = im.secIdx(p);
        if (sP >= 0 && sP < 16) { st.perSecBytes[sP] += L; st.perSecCount[sP]++; }
        uint64_t d = p - bestQ;
        st.tokenCost += varintBytes(d - 1) + varintBytes(L - 4) + 2;
        for (uint64_t t = p; t < p + L && t < n; t++) covered[t] = 1;
    }
    (void)st.runBytesV1; (void)st.runBytesCross;
    return st;
}

static Stats analyzeSparse(Image& im, uint32_t chainCap, uint32_t slotCap) {
    Stats st;
    size_t n = im.d.size();
    st.secCount = im.secs.size();
    HashTab ht;
    ht.init((uint64_t)(n / 4 + 4) * 2, uint32_t(n + 1));
    std::vector<uint32_t> aligned;
    aligned.reserve(size_t(n / 4));
    for (size_t p = 0; p + 4 <= n; p += 4)
        if (im.secIdx(uint64_t(p)) >= 0) aligned.push_back(uint32_t(p));
    uint64_t matchSlots = 0, crossSlots = 0;
    for (size_t i = 0; i < aligned.size(); i++) {
        uint64_t p = aligned[i];
        uint32_t bestAll = 0, bestCross = 0, tried = 0;
        for (uint32_t cur = ht.chain(invariant(im, p)); cur && tried < chainCap; cur = ht.next[cur]) {
            uint64_t q = cur - 1;
            if (q >= p) continue;
            bool cross = im.secIdx(q) != im.secIdx(p);
            uint32_t k = 0;
            for (; k < slotCap; k++) {
                uint64_t ap = p + 4ull * k, bq = q + 4ull * k;
                if (ap + 4 > n || bq + 4 > n || bq + 4 > p) break;
                if (uint64_t(rd32(&im.d[ap])) + uint64_t(im.addr(ap)) !=
                    uint64_t(rd32(&im.d[bq])) + uint64_t(im.addr(bq))) break;
            }
            if (cross) bestCross = std::max(bestCross, k);
            else bestAll = std::max(bestAll, k);
            tried++;
        }
        matchSlots += bestAll;
        crossSlots += bestCross;
        int32_t s = im.secIdx(p);
        if (s >= 0 && s < 16) st.perSecBytes[s] += 4ull * bestAll;
        st.runBytes += 4ull * bestAll;
        if (bestAll) st.runCount++;
        st.maxSlots = std::max(st.maxSlots, bestAll);
        ht.insert(invariant(im, p), uint32_t(p));
    }
    st.runBytesV1 = matchSlots * 4;
    st.runBytesCross = crossSlots * 4;
    st.runBytesMode0 = matchSlots * 4;
    return st;
}

static std::vector<uint8_t> encodeRef(const std::vector<uint8_t>& d, const std::vector<int32_t>& secOf,
                                     const std::vector<Sec>& secs, uint32_t minSlots) {
    Image im; im.d = d; im.secOf = secOf; im.secs = secs;
    size_t n = d.size();
    std::vector<uint8_t> out;
    HashTab ht;
    ht.init((uint64_t)(n / 4 + 4) * 2, uint32_t(n + 1));
    std::vector<char> covered(n, 0);
    uint64_t p = 0;
    uint64_t litStart = 0;
    auto flushLit = [&](uint64_t upto) {
        if (upto > litStart) { out.push_back(0); putvar(out, upto - litStart); out.insert(out.end(), d.begin() + long(litStart), d.begin() + long(upto)); }
    };
    while (p + 4 <= n) {
        if (secOf[p] < 0) { p += 4; continue; }
        uint32_t bestSlots = 0, bestQ = 0; uint8_t bestMode = 0; uint32_t tried = 0;
        for (uint32_t cur = ht.chain(invariant(im, p)); cur && tried < 8; cur = ht.next[cur]) {
            uint64_t q = cur - 1;
            if (q >= p) continue;
            uint32_t la, ln;
            growRun(im, p, q, la, ln);
            if (secOf[q] == secOf[p]) {
                if (la > bestSlots) { bestSlots = la; bestQ = uint32_t(q); bestMode = 0; }
                if (ln > bestSlots) { bestSlots = ln; bestQ = uint32_t(q); bestMode = 1; }
            }
            tried++;
        }
        if (bestSlots >= minSlots && bestQ) {
            flushLit(p);
            out.push_back(1);
            putvar(out, p - bestQ - 1);
            putvar(out, 4ull * bestSlots - 4);
            out.push_back(bestMode);
            p += 4ull * bestSlots;
            litStart = p;
        } else {
            ht.insert(invariant(im, p), uint32_t(p));
            p += 4;
        }
    }
    flushLit(n);
    return out;
}

static std::vector<uint8_t> decodeRef(const std::vector<uint8_t>& w, size_t n, bool* ok) {
    std::vector<uint8_t> out;
    size_t i = 0;
    *ok = true;
    while (i < w.size()) {
        uint8_t t = w[i++];
        if (t == 0) {
            uint64_t len = getvar(w.data(), i, w.size());
            if (i + len > w.size() || out.size() + len > n) { *ok = false; return out; }
            out.insert(out.end(), w.begin() + long(i), w.begin() + long(i + long(len)));
            i += len;
        } else if (t == 1) {
            uint64_t dm1 = getvar(w.data(), i, w.size());
            uint64_t lm4 = getvar(w.data(), i, w.size());
            if (i >= w.size()) { *ok = false; return out; }
            uint8_t mode = w[i++];
            uint64_t L = lm4 + 4;
            uint64_t p = out.size();
            if (dm1 + 1 >= p || p + L > n) { *ok = false; return out; }
            uint64_t q = p - dm1 - 1;
            for (uint64_t t = 0; t < L; t += 4) {
                uint32_t sv = rd32(&out[q + t]);
                uint32_t dv;
                if (mode == 0) dv = sv - uint32_t(dm1 + 1);
                else dv = (sv == 0) ? 0u : (sv - uint32_t(dm1 + 1));
                wr32(out, 0); out.pop_back(); out.pop_back(); out.pop_back(); out.pop_back();
                wr32(out, dv);
            }
        } else { *ok = false; return out; }
    }
    *ok = (out.size() == n);
    return out;
}

static int selftest() {
    int fails = 0;
    auto mk = [](std::vector<uint32_t> targets, size_t repeat, uint64_t base) {
        std::vector<uint8_t> d;
        for (size_t r = 0; r < repeat; r++)
            for (size_t k = 0; k < targets.size(); k++) {
                uint64_t fieldPos = base + d.size();
                uint32_t v = uint32_t(uint64_t(targets[k]) - fieldPos);
                wr32(d, v);
            }
        return d;
    };
    struct Case { const char* name; std::vector<uint8_t> d; };
    std::vector<Case> cases;
    { std::vector<uint8_t> d = mk({0x1000, 0x1020, 0x1040, 0x1000}, 3, 0); cases.push_back({"shiftall", d}); }
    { std::vector<uint8_t> d = mk({0x1000, 0x1020, 0x1040, 0x1000}, 3, 0);
      d[4 * 2] = 0; d[4 * 2 + 1] = 0; d[4 * 2 + 2] = 0; d[4 * 2 + 3] = 0;
      cases.push_back({"nonzero", d}); }
    { std::vector<uint8_t> d = mk({0x40, 0x48, 0x50}, 5, 0); cases.push_back({"short", d}); }
    { std::vector<uint8_t> d = mk({0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000}, 2, 0);
      for (size_t i = 0; i < 7; i++) d[i] = uint8_t(i * 37);
      cases.push_back({"mixed", d}); }
    for (auto& c : cases) {
        Image im; im.d = c.d; buildSecOf(im);
        Sec s; s.name = ".synthetic"; s.rawStart = 0; s.rawEnd = c.d.size(); s.rva = 0x1000;
        im.secs.push_back(s);
        for (size_t p = 0; p < c.d.size(); p++) im.secOf[p] = 0;
        std::vector<uint8_t> w = encodeRef(c.d, im.secOf, im.secs, 2);
        bool ok = false;
        std::vector<uint8_t> back = decodeRef(w, c.d.size(), &ok);
        bool pass = ok && back.size() == c.d.size() &&
                    std::memcmp(back.data(), c.d.data(), c.d.size()) == 0;
        std::printf("selftest %-10s in=%zu wire=%zu %s\n", c.name, c.d.size(), w.size(), pass ? "PASS" : "FAIL");
        if (!pass) fails++;
    }
    {
        std::vector<uint8_t> w = {1, 0xFF, 0xFF, 0x7F, 0, 200};
        bool ok = false; decodeRef(w, 4096, &ok);
        if (ok) { std::printf("selftest malformed      FAIL (accepted)\n"); fails++; }
        else std::printf("selftest %-10s in=? wire=? PASS\n", "malformed");
    }
    std::printf("selftest %s (%d failures)\n", fails ? "FAILED" : "PASS", fails);
    return fails ? 1 : 0;
}

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "selftest") return selftest();
    if (argc < 2) { std::fprintf(stderr, "usage: rsc_probe selftest | <file>...\n"); return 2; }
    for (int a = 1; a < argc; a++) {
        Image im;
        loadFile(im, argv[a]);
        Stats st = analyze(im, 8, 2);
        if (std::getenv("RSC_SPARSE")) {
            Stats sp = analyzeSparse(im, 8, 64);
            std::printf("SPARSE sameSecBytes=%llu crossSecBytes=%llu sparseCoverage=%.4f%% maxSlots=%u\n",
                        (unsigned long long)sp.runBytesV1, (unsigned long long)sp.runBytesCross,
                        im.d.empty() ? 0.0 : 100.0 * double(sp.runBytesV1) / double(im.d.size()),
                        sp.maxSlots);
            for (size_t s = 0; s < im.secs.size() && s < 16; s++) {
                uint64_t sz = im.secs[s].rawEnd - im.secs[s].rawStart;
                if (!sz) continue;
                std::printf("  SPARSE sec %-9s raw=%llu sparseBytes=%llu secSparse=%.3f%%\n",
                            im.secs[s].name.c_str(), (unsigned long long)sz,
                            (unsigned long long)sp.perSecBytes[s],
                            100.0 * double(sp.perSecBytes[s]) / double(sz));
            }
        }
        std::printf("FILE %s bytes=%zu pe=%d sections=%llu\n", argv[a], im.d.size(), im.isPE ? 1 : 0,
                    (unsigned long long)st.secCount);
        double cov = im.d.empty() ? 0.0 : 100.0 * double(st.runBytes) / double(im.d.size());
        std::printf("  runs=%llu runBytes=%llu coverage=%.4f%% maxSlots=%u mode0bytes=%llu\n",
                    (unsigned long long)st.runCount, (unsigned long long)st.runBytes, cov, st.maxSlots,
                    (unsigned long long)st.runBytesMode0);
        std::printf("  tokenCostBound=%llu netUpperBound=%lld (optimistic: no entropy coding, no literals for uncovered)\n",
                    (unsigned long long)st.tokenCost, (long long)(st.runBytes - st.tokenCost));
        std::printf("  slotHist[1..8+]=");
        for (int i = 1; i <= 8; i++) std::printf("%llu ", (unsigned long long)st.slotHist[i]);
        std::printf("\n");
        for (size_t s = 0; s < im.secs.size() && s < 16; s++) {
            uint64_t sz = im.secs[s].rawEnd - im.secs[s].rawStart;
            if (!sz) continue;
            double sc = 100.0 * double(st.perSecBytes[s]) / double(sz);
            std::printf("  sec %-9s raw=%llu runs=%llu runBytes=%llu secCoverage=%.3f%% align4=%s\n",
                        im.secs[s].name.c_str(), (unsigned long long)sz,
                        (unsigned long long)st.perSecCount[s], (unsigned long long)st.perSecBytes[s], sc,
                        (im.secs[s].rawStart % 4 == 0) ? "yes" : "no");
        }
    }
    return 0;
}