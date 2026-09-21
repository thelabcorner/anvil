// pe_text.cpp — extract executable sections from a PE/COFF image.
// Windows counterpart of the Linux ELF .text extraction the PNRA numbers were
// measured against. Emits a flat concatenation of IMAGE_SCN_CNT_CODE sections
// (default: the largest one, or all of them with --all), which is the input a
// PNRA/TCOPY experiment needs.
//
// Usage: pe_text <in.exe> <out.bin> [--all] [--list]
//        pe_text --list <in.exe>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

namespace {

using u8 = uint8_t; using u16 = uint16_t; using u32 = uint32_t; using u64 = uint64_t;

template <class T> T rd(const u8* p, size_t off) { T v; std::memcpy(&v, p + off, sizeof(T)); return v; }

struct Sec { char name[9]; u32 vsize, vaddr, rawsize, rawptr; u32 flags; };

bool pe_sections(const std::vector<u8>& d, std::vector<Sec>& out, std::string& err) {
    if (d.size() < 0x40) { err = "too small"; return false; }
    u32 mz = rd<u16>(d.data(), 0);
    if (mz != 0x5A4D) { err = "not MZ"; return false; }
    u32 e_lfanew = rd<u32>(d.data(), 0x3C);
    if (e_lfanew + 24 > d.size()) { err = "bad e_lfanew"; return false; }
    if (std::memcmp(d.data() + e_lfanew, "PE\0\0", 4) != 0) { err = "not PE"; return false; }
    u32 coff = e_lfanew + 4;
    u16 nsec = rd<u16>(d.data(), coff + 2);
    u16 optsz = rd<u16>(d.data(), coff + 16);
    u32 opthdr = coff + 20;
    if (optsz < 2 || opthdr + optsz > d.size()) { err = "bad optional header"; return false; }
    u16 magic = rd<u16>(d.data(), opthdr);
    u64 image_base = 0; u32 sect_align = 0, file_align = 0;
    if (magic == 0x10B) { // PE32
        image_base = rd<u32>(d.data(), opthdr + 28);
        sect_align = rd<u32>(d.data(), opthdr + 32);
        file_align = rd<u32>(d.data(), opthdr + 36);
    } else if (magic == 0x20B) { // PE32+
        image_base = rd<u64>(d.data(), opthdr + 24);
        sect_align = rd<u32>(d.data(), opthdr + 32);
        file_align = rd<u32>(d.data(), opthdr + 36);
    } else { err = "unknown optional header magic"; return false; }
    (void)image_base; (void)sect_align; (void)file_align;
    u32 secttab = opthdr + optsz;
    if (secttab + nsec * 40 > d.size()) { err = "section table overruns file"; return false; }
    for (u16 i = 0; i < nsec; i++) {
        const u8* s = d.data() + secttab + i * 40;
        Sec e{};
        std::memcpy(e.name, s, 8); e.name[8] = 0;
        e.vsize = rd<u32>(s, 8); e.vaddr = rd<u32>(s, 12);
        e.rawsize = rd<u32>(s, 16); e.rawptr = rd<u32>(s, 20);
        e.flags = rd<u32>(s, 36);
        out.push_back(e);
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "usage: pe_text <in.exe> <out.bin> [--all] [--list]\n"); return 2; }
    std::string in = argv[1], out = argv[2];
    bool all = false, list = false;
    for (int i = 3; i < argc; i++) {
        if (!std::strcmp(argv[i], "--all")) all = true;
        if (!std::strcmp(argv[i], "--list")) list = true;
    }
    std::ifstream f(in, std::ios::binary);
    if (!f) { std::fprintf(stderr, "cannot open %s\n", in.c_str()); return 1; }
    std::vector<u8> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<Sec> secs; std::string err;
    if (!pe_sections(d, secs, err)) { std::fprintf(stderr, "PE parse failed: %s\n", err.c_str()); return 1; }

    std::vector<const Sec*> code;
    for (auto& s : secs) if (s.flags & 0x00000020) code.push_back(&s); // CNT_CODE
    if (list) {
        std::printf("file=%s size=%zu nsec=%zu\n", in.c_str(), d.size(), secs.size());
        for (auto& s : secs)
            std::printf("  %-8s vsize=%-10u vaddr=0x%08x rawsize=%-10u rawptr=%-10u flags=0x%08x%s\n",
                        s.name, s.vsize, s.vaddr, s.rawsize, s.rawptr, s.flags,
                        (s.flags & 0x20) ? " [CODE]" : "");
    }
    if (out == "-" || out == "--list") return 0;
    if (code.empty()) { std::fprintf(stderr, "no executable section\n"); return 1; }
    if (!all) {
        const Sec* best = code[0];
        for (auto* s : code) if (s->rawsize > best->rawsize) best = s;
        code.assign(1, best);
    }
    // Concatenate in virtual-address order (that is the order the loader and
    // hence the source position semantics follow).
    std::sort(code.begin(), code.end(), [](const Sec* a, const Sec* b) { return a->vaddr < b->vaddr; });
    std::vector<u8> o;
    for (auto* s : code) {
        u32 n = s->rawsize;
        if (s->rawptr + n > d.size()) n = (u32)(d.size() - std::min<size_t>(d.size(), s->rawptr));
        o.insert(o.end(), d.begin() + s->rawptr, d.begin() + s->rawptr + n);
    }
    std::ofstream g(out, std::ios::binary);
    g.write((const char*)o.data(), (std::streamsize)o.size());
    std::fprintf(stderr, "%s -> %s : %zu bytes from %zu code section(s)\n",
                 in.c_str(), out.c_str(), o.size(), code.size());
    return 0;
}
