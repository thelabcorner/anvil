# Prior-Art Ledger: TCOPY Mechanism — External Research Pass

Source: external web-research agent pass (independently corroborates the
swarm's `research/patent-tcopy` gate record and ledger Experiment K).
Scope note (verbatim from source): search budget exhausted mid-investigation
(6 queries, general web search; Google Patents results came through general
web search; Espacenet/lens.org not directly queried). Gaps flagged per target.

## 1. Relocation-aware compression patents (Microsoft / Qualcomm / Apple / IBM)

- **Microsoft — EXISTS (adjacent, two-file, not implicit-Δ).**
  - US7861224B2 "Delta compression using multiple pointers" (Sliger,
    McGuire, Petrov; G06F8/658 "Incremental updates; Differential updates").
  - US20050022175A1 / US7600225B2 / EP1501196A1 "System and method for
    intra-package delta compression of data" — delta compression in a
    self-contained package; references an EARLIER patent for
    executable-aware delta: US 6,466,999 (iterator + symbol information to
    optimize delta size when both inputs are executables). This is the
    closest documented Microsoft lineage to "executable-aware delta," but it
    is a TWO-FILE (base→target) delta engine using symbol tables, not a
    single-file self-referential LZ match with implicit distance-derived Δ.
    GAP: full text/claims of US 6,466,999 not retrieved this session.
- **Qualcomm — NOT-FOUND (this session).** Only unrelated memory/cache
  relocation filings (US10261910B2 cache-line compaction; wireless user-plane
  relocation family). GAP: dedicated Qualcomm "code compression" +
  "PC-relative" + "branch immediate" query not run.
- **Apple (dyld shared cache / arm64e) — CLOSE-BUT-DIFFERENT.**
  Chained-pointer relocation scheme: relocation info stored almost for free
  via linked lists in unused pointer bits; also used for the kernel. This is
  single-file and self-referential in spirit, but it compresses
  pointer/rebase METADATA, not repeated instruction templates whose
  branch-immediate differs by exactly −distance. GAP: no Apple patent number
  for the chained-fixup scheme confirmed.
- **IBM — NOT-FOUND (this session).** GAP: dedicated IBM query not run.

Verdict: across all four assignees, no patent retrieved teaches a
single-file, self-referential compressor copying a prior phrase and adjusting
an embedded relocation/immediate field by an amount IMPLICITLY derived from
the match distance (Δ = −d).

## 2. Transformed-copy / "copy with edit" single-file patents

- **NOT-FOUND (this session).** US7861224B2 is the nearest keyword overlap;
  no dedicated query completed for "match + arithmetic transform on copied
  bytes + implicit parameter from match metadata". GAP: dedicated Google
  Patents CPC query (G06F8/658 × transform classes) recommended.

## 3. Sparse-offset correction streams ("match + mismatch mask + residual")

- **CLOSE-BUT-DIFFERENT — zdelta and vdelta lineage (academic).**
  - zdelta (Trendafilov, Memon, Suel, TR-CIS-2002-02, 2002)
  - vdelta (Korn, Vo, 1995)
  - Delta algorithms: an empirical analysis (Hunt, Vo, Tichy, ACM TOSEM 7,
    192–214, 1998)
  All TWO-FILE (source vs target) copy/insert/mismatch-list schemes; sparse
  correction exists broadly but never within a single self-referential
  stream and never with an implicit arithmetic transform on the copied
  region. GAP: no dedicated search for approximate-matching-compression or
  DNA edit-operation patents.

## 4. Academic literature

- Delta surveys retrieved (Hunt/Vo/Tichy 1998, Korn/Vo 1995, zdelta 2002)
  are uniformly two-file. GAP: RePair/grammar-transform-with-transform
  search not run.

## 5. BCJ/E8-E9 filter — public-domain confirmation

- **EXISTS as public-domain, global pre-filter.**
  - LZMA SDK placed in the public domain (4.62, December 2008, per SDK
    history).
  - BCJ/E8-E9 and ARM64 filters documented as FILE-WIDE preprocessing passes
    before LZMA (7-Zip 23.01 added ARM64 filter; 7-Zip parses executables by
    extension, selecting BCJ/BCJ2/ARM64).
  - Confirms the boundary: BCJ-class filters are global pre-LZ transforms,
    structurally distinct from a per-reference in-match transform.

## Final assessment

**Does the implicit-Δ self-referential TCOPY formulation exist in the
record?** Based on completed searches: NO prior art found teaching a
single-file, self-referential LZ-style match where an embedded
relocation/branch-immediate field is corrected by an amount implicitly equal
to the negative of the match distance (Δ = −d, zero-bit parameter). Closest
analogs: (a) Microsoft two-file executable-aware delta engines (US 6,466,999
lineage, US7861224B2), (b) Apple dyld chained-pointer relocation compaction
(pointer/rebase metadata, not LZ-copied instruction bytes).

**Boundary confirmation:** record supports BCJ being global/pre-LZ;
no prior art retrieved blurs that boundary with a per-match/per-reference
transform — PROVISIONAL given incomplete coverage.

## Access / coverage gaps (explicit)

- Espacenet and Lens.org not queried (general web search only).
- Qualcomm "code compression" + branch-immediate query not run.
- IBM delta/relocation query not run.
- Full claims of US 6,466,999 not retrieved.
- Apple dyld chained-pointer patent lookup not run.
- RePair/grammar-transform, approximate-matching, DNA edit-op searches not run.
- 6 queries total; follow-up pass recommended before finalizing a novelty
  opinion.

## Swarm disposition

- Corroborates research/patent-tcopy (US7676506B2 relocation algebra,
  two-file; Microsoft minimum-delta CFG two-file; IBM exact-match block-move)
  with independent sourcing and adds: Microsoft intra-package delta lineage
  (US7861224B2, US20050022175A1/US7600225B2/EP1501196A1, US 6,466,999
  reference), Apple dyld chained-pointer (close-but-different), and the
  BCJ public-domain confirmation.
- Residual open items (flagged): US 6,466,999 full claims; Qualcomm/IBM
  targeted queries; Apple chained-fixup patent number; formal FTO attorney
  review remains recommended before any commercial claim (research's gate
  record already states this).

---

# External Research Pass 2 — TCOPY (independent, additional citations)

Second independent pass. Verdict again: NO prior art teaches the
single-file self-referential implicit-Δ transformed copy. New entries not in
pass 1 (all CLOSE-BUT-DIFFERENT unless noted):

## 1. Relocation-aware compression patents
- **Red Bend Software — US 6,546,552 B1 (priority 1998):** disassembly +
  pointer normalization across TWO executable versions (two-file differential
  update); no in-stream single-file LZ match primitive.
- **Microsoft — US 6,374,250 B1 / MS-RDC (Remote Differential Compression):**
  chunking + global address fixups across files (two-file); no per-reference
  implicit Δ=−d arithmetic inside a self-referential LZ decoder.
- **Apple — US 10,229,282 B2 (Dyld Shared Cache):** page-level compression +
  pointer-stub fixup tables; no per-match relative-offset transform during
  single-file decompression.
- **Qualcomm — US 9,300,320 B2:** cache-line code compression via dictionary
  lookups and bit-removal; lacks match-distance-derived relocation algebra.

## 2. Transformed-copy / "copy with edit" single-file
- Single-file LZ match with distance-implicit arithmetic (Δ=−d): NOT-FOUND.

## 3. Sparse-offset correction streams
- **Zdelta (Trendafilov/Memon/Suel 2002):** byte-level mismatch lists in LZ77
  copy commands, but strictly two-file; no PC-relative relocation arithmetic.
- **DNA/sequence compression — FaStore / US 9,223,794 B2:** edit-distance
  operations (substitutions/indels) inside single-file LZ match references,
  but symbol-level edits on character alphabets, not 32-bit additive
  relocation adjustments.

## 4. Academic literature
- **ZPAQ/PCOMP (Matt Mahoney, 2016):** global pre-transform replacing x86
  CALL/JMP (E8/E9) relative offsets with absolute file offsets before LZ
  context modeling — a GLOBAL filter, not a per-reference LZ primitive.
- **Self-referential LZ77 with edit distance (Gawrychowski et al. 2011/2021;
  Kreft & Navarro 2013):** theoretical bounds for self-referential LZ77
  parses with Hamming/Levenshtein errors; does not disclose machine-code
  relocation transforms or implicit parameter derivation.

## 5. BCJ/E8-E9 filings
- **7-Zip / LZMA SDK Bra86.c (Igor Pavlov):** EXISTS — placed in the public
  domain in 2008 (matches pass 1: LZMA SDK 4.62). Global stream-level
  pre-transform converting relative offsets to absolute offsets across the
  entire binary before LZMA.

## Boundary / verdict
- BCJ, PCOMP, Courgette, Red Bend rely on GLOBAL pre-transforms or TWO-FILE
  differential patching. TCOPY as an in-stream, per-reference match primitive
  with an implicit distance-derived parameter (Δ=−d) remains UNCLAIMED in the
  searched record.

## Gaps (pass 2)
- Non-public applications within the 18-month publication blackout.
- Unindexed proprietary game-console / embedded packers (custom
  demoscene/console crunchers).
- Paywalled corporate repositories not indexed by public search.
- Source domains consulted (trail): lwn.net, mozilla.org, uspto.gov,
  microsoft.com, computer.org, google.com, github.io, ijcs.net, tdx.cat,
  mattmahoney.net, researchgate.net, dtu.dk, europa.eu, github.com,
  googlesource.com, hostingadvice.com, quora.com.

---

# External Research Pass 3 — TCOPY (13 Aug 2026, independent scan)

Third independent pass. Verdict again: NO — the implicit-Δ self-referential
formulation does not exist in the searched record. New entries (all
CLOSE-BUT-DIFFERENT unless noted):

1. **Microsoft delta-patching family — WO 2005/071542 / US 7,509,636 B2
   (prio 15-Dec-2003):** two-file (basis vs target) LZ-style delta; literals,
   COPY-from-basis, per-copy mismatch lists. No self-referential matches, no
   per-match arithmetic on copied bytes.
2. **Microsoft "Index correlating uncompressed/compressed content" — EP
   4,154,406 B1 (prio 18-May-2020):** single-file LZ4-like codec, per-segment
   COPY, verbatim copy only. No field-wise adjustments, no distance-derived
   delta.
3. **IBM "Computer instruction compression" — US 6,564,314 (prio
   6-Apr-1999):** instruction streams compressed after GLOBAL relocation
   normalization; relocation handled once, not per match; file-wide pre-pass.
4. **Approximate-match / LZ with mismatches — US 12,373,439 (prio
   31-Jan-2019):** masking/"don't-care" bits and early abort on excessive
   mismatch; no arithmetic transform, no implicit Δ.
5. **Zdelta (TR-CIS-2002-02):** two-file delta; copy-with-patch mismatch
   lists; no self-reference, no Δ=−d rule.
6. **Relocation-aware executable compressors (survey):** NOT-FOUND for a
   single-file per-phrase transform — all located material is global branch
   conversion (BCJ/E8-E9) or multi-file delta.
7. **Transformed-copy LZ patents ("copy instruction AND add
   constant/XOR/transform"):** NOT-FOUND — no patent embeds an arithmetic
   addend inside an LZ match, implicit or explicit.
8. **Sparse-offset correction streams (DNA, image, mask-based — e.g. US
   2024/0211132 A1):** mismatch masks exist, but no distance-derived additive
   corrections.

**Boundary confirmation (3rd pass):** Wikipedia + xz/BCJ docs confirm BCJ is
a global pre-LZ pass rewriting branch immediates before any LZ copy; no
source claims per-phrase implicit Δ.

**Gaps (pass 3):** corporate intranet disclosures behind paywalls; none
accessible in Espacenet/Google Patents at search time.

**Three-pass convergence:** passes 1-3 are independent and agree: closest
art is either (a) global once-per-file relocation/branch adjustment, or
(b) two-file copy-with-edits. The single-file self-referential implicit-Δ
transformed copy remains unclaimed in the searched record.


