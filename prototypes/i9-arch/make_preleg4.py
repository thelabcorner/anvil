import sys
src = open(r"src/anvil.cpp", encoding="utf-8").read()

def rep(s, old, new, tag):
    n = s.count(old)
    if n != 1:
        print(f"ABORT {tag}: matched {n} times (expected 1)")
        sys.exit(1)
    return s.replace(old, new)

# 1) BitReader -> original bit-at-a-time
src = rep(src, """struct BitReader {
    const uint8_t* p;
    size_t n;
    size_t byte = 0;
    uint64_t acc = 0;   // buffered byte (I9 leg 4: bit-at-a-time load removed)
    int have = 0;       // bits still available in acc
    uint32_t bit() {
        if (have == 0) {
            if (byte >= n) return 0; // arithmetic decoder pads with zeros
            acc = p[byte++];
            have = 8;
        }
        --have;
        return static_cast<uint32_t>(acc >> have) & 1u;
    }
    // Number of payload bytes touched (a partially-consumed buffered byte counts).
    size_t consumed_bytes() const { return byte; }
};""", """struct BitReader {
    const uint8_t* p;
    size_t n;
    size_t byte = 0;
    uint8_t bitpos = 0;
    uint32_t bit() {
        if (byte >= n) return 0; // arithmetic decoder pads with zeros
        uint32_t v = (p[byte] >> (7 - bitpos)) & 1u;
        if (++bitpos == 8) { bitpos = 0; ++byte; }
        return v;
    }
};""", "bitreader")

# 2) remove AdaptiveDecModel block
start = src.find("// I9 leg 4: decode-only adaptive model.")
end = src.find("struct CodecModels {")
if start < 0 or end < 0 or end < start:
    print("ABORT adaptivedecmodel: markers not found"); sys.exit(1)
src = src[:start] + src[end:]

# 3) remove decode_uvar_m block
start = src.find("// I9 leg 4: same loop over the decode-only model type.")
end = src.find("static void put_uvar(")
if start < 0 or end < 0 or end < start:
    print("ABORT decode_uvar_m: markers not found"); sys.exit(1)
src = src[:start] + src[end:]

# 4) consumed_bytes line
src = rep(src, "    size_t consumed_bytes() const { return br_.consumed_bytes(); }",
          "    size_t consumed_bytes() const { return br_.byte + (br_.bitpos ? 1u : 0u); }", "consumed")

# 5) bwt_arith_decode models
src = rep(src, """    ArithmeticDecoder ad(p,n); AdaptiveDecModel tok0(256), runm(256);
    std::array<std::unique_ptr<AdaptiveDecModel>,257> tok1;
    auto model1=[&](uint32_t ctx)->AdaptiveDecModel& { if(!tok1[ctx])tok1[ctx]=std::make_unique<AdaptiveDecModel>(256); return *tok1[ctx]; };""",
"""    ArithmeticDecoder ad(p,n); AdaptiveModel tok0(256), runm(256);
    std::array<std::unique_ptr<AdaptiveModel>,257> tok1;
    auto model1=[&](uint32_t ctx)->AdaptiveModel& { if(!tok1[ctx])tok1[ctx]=std::make_unique<AdaptiveModel>(256); return *tok1[ctx]; };""", "bwt_models")
src = rep(src, "            uint64_t rv=decode_uvar_m(ad,runm);", "            uint64_t rv=decode_uvar(ad,runm);", "decode_uvar_call")

open(r"prototypes/i9-arch/variants/anvil_preleg4.cpp", "w", encoding="utf-8", newline="").write(src)
print("preleg4 written", len(src))
