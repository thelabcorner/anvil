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

---

# External Research Pass 4 — TCOPY (13 Aug 2026, most rigorous scan)

Fourth independent pass. Verdict: NOT-FOUND for the implicit-Δ formulation,
WITH a material unresolved gap (Intel) and two corrections to earlier passes.
New findings:

## 1. Single-file LZ / transformed-reference mechanisms
- **VCDIFF / RFC 3284 (June 2002) — CLOSE-BUT-DIFFERENT, IMPORTANT for C1:**
  single-file self-reference is NOT novel — a source window may be drawn from
  the source file OR from already-decoded target data; COPY can reference the
  target itself; overlapping target copies explicitly supported; ADD/RUN
  encode unmatched material. So self-reference + exact COPY + corrections
  was STANDARDIZED long ago. COPY remains verbatim: no arithmetic transform
  of selected copied fields.
- **Microsoft US 11,675,768 B2 / EP 4,154,406 B1 (prio 18-May-2020):**
  LZ4-family single-file COPY including prior/current segments; ordinary
  copying only, no field mask / additive transform / distance-derived
  arithmetic / COPY-time immediate rewriting.
- **Single-file "COPY + arithmetic transform" patent search: NOT-FOUND.**

## 2. Approximate matching / sparse correction
- **GenCompress (Chen, Kwong & Li, Genome Informatics 1999):**
  CLOSE-BUT-DIFFERENT — lossless one-pass DNA compressor built on
  approximate repeats; encodes edits rather than requiring exact matches.
  Does not disclose one common 32-bit additive Δ on selected fields,
  executable-relative algebra, or deriving the transform from reference
  distance.
- **RLZAP (2016):** CLOSE-BUT-DIFFERENT — relative-LZ with adaptive pointers
  and mismatch accommodation; the "relative" arithmetic concerns pointer
  representation, not a transform on COPY-produced bytes; reference-dataset,
  not self-reference.
- **zdelta (2002):** two-input delta; no per-match distance-derived transform.

Consequence: self-reference, approximate matching, and sparse
edit/correction representation EACH have substantial prior art; the narrower
distinguishing mechanism is the shared phrase-local arithmetic transform and
its implicit derivation from d.

## 3. Executable / relocation-aware compression
- **Philips US 5,787,302 A (prio 15-May-1996):** CLOSE-BUT-DIFFERENT —
  genuine relocation-aware instruction compression prior art: compression
  coupled with relocation correction is OLD. No per-reference transformed
  dictionary phrases.
- **Microsoft US 6,907,516 B2 (prio 30-May-2002):** instruction compression
  via prediction/statistical coding (PPM/arithmetic/range oriented), not
  transformed LZ COPY phrases.
- **Courgette:** old→new disassembly + relocation/symbol normalization +
  delta — two-file; evidence relocation normalization is useful, not
  phrase-local self-reference.
- **Chromium Zucchini — IMPORTANT NEAR HIT:** patch application copies from
  the old image + bytewise difference info, with dedicated rel32 target
  corrections — copy + correction + executable-reference handling EXISTS.
  STILL two-file delta patching; no self-referential backreference whose
  distance determines the arithmetic correction.
- **Microsoft US 7,509,636 B2 / WO 2005/071542 (prio 15-Dec-2003):**
  basis→target delta patching, not target-history reference compression.
- **Intel US 7,111,148 B1 / US 7,010,665 B1 (prio 27-Jun-2002) — UNRESOLVED
  ACCESS GAP:** "Method and apparatus for compressing relative addresses"
  / "...decompressing relative addresses". Titles close enough that the
  family CANNOT be waved away; full text/claims not retrievable this session.
  **REQUIRED: manual full-text review before the executable-specific novelty
  boundary is treated as closed.**
- **Qualcomm/Apple/IBM targeted searches: NOT-FOUND (qualifying mechanism).**

## 4. BCJ / E8-E9 boundary — QUALIFIED
- Official xz docs place BCJ BEFORE LZMA in a filter chain (BCJ converts
  relative machine-code addresses; LZMA is the final filter). 7-Zip calls
  BCJ an executable CONVERTER separate from LZMA compression.
- The defensible distinction is NOT "BCJ always transforms an entire file
  globally" — a BCJ filter may operate within stream/block/filter-chain
  boundaries. The distinction is: BCJ is a PRE-LZ transformation layer, not
  an operation attached to an individual dictionary COPY phrase.
- LZMA SDK is public domain; no Pavlov/7-Zip BCJ patent located in targeted
  searches — but that search result must not be converted into the stronger
  assertion that no such patent exists anywhere.

## 5. Corrections to earlier passes
- **US 12,373,439 REMOVED as compression prior art:** indexed patent
  concerns OptumSoft approximate matching of conditions/table entries, NOT
  approximate LZ/string compression.
- **US 6,564,314 NOT IBM:** surfaced record is an STMicroelectronics/
  SGS-Thomson computer-instruction compression patent; does not support the
  previously assigned description.

## Verdict (pass 4)
NOT-FOUND: no accessible disclosure combines single-file self-referential
LZ COPY with sparse field-wise additive transformation where Δ is derived
implicitly from the backreference distance — although Intel US 7,111,148 /
US 7,010,665 remains a material unresolved full-text gap.

Closest combined prior art: VCDIFF/Microsoft (self-referential COPY),
GenCompress (reference-plus-edits), BCJ/Philips (executable relocation
normalization), Zucchini (executable-aware copy-plus-correction, two-file).

Boundary confirmation: SUPPORTED, WITH QUALIFICATION — BCJ is a pre-LZ
filter-layer transform; Zucchini blurs the broader COPY-with-edits boundary
but in two-file patching; no accessible source claims the specific
per-reference implicit Δ=−d mechanism.

## Explicit gaps (pass 4)
- Intel US 7,111,148 B1 / US 7,010,665 B1 full text/claims: REQUIRED review.
- Espacenet/Lens not reliably enumerable; Google Patents + linked
  bibliographic records supplied most coverage.
- Academic full texts partially restricted (abstracts used where necessary).
- Non-public/unpublished/inadequately indexed documents out of scope.

## Swarm disposition (pass 4)
- Record the Intel family full-text review as a REQUIRED item before the
  executable-specific novelty boundary is treated as closed.
- Correct the two citations (US 12,373,439 removed; US 6,564,314
  STMicroelectronics).
- VCDIFF (RFC 3284) also touches SPARSE-REF C1: self-reference + exact COPY
  + corrections/literals is standardized prior art — C1's separator must
  remain the sparse-correction-mask-as-first-class-entropy-stream +
  implicit-transform combination, not self-reference per se.
- Zucchini is the strongest executable near-hit; the record should cite it as the
  two-file boundary reference.

---

# External Research Pass 5 — TCOPY (21 Aug 2026, BINDING Intel full-text claims review — gate closeout)

Fifth pass. This is the REQUIRED item from Pass 4 and ledger C12: full-text
claims review of Intel US 7,111,148 B1 / US 7,010,665 B1 (prio 27-Jun-2002),
the family four passes could not retrieve and could not wave away by title.
**The claims were retrieved in full this session and the binding condition is
discharged. Verdict: TCOPY claim STANDS.**

## 1. Retrieval record (what was retrieved, from where)

- **US 7,111,148 B1** "Method and apparatus for compressing relative
  addresses" (Toll, St. Clair, Miller, Ahuja; Intel; filed 27-Jun-2002;
  granted 19-Sep-2006): **all 39 claims retrieved verbatim** from Google
  Patents (patents.google.com/patent/US7111148B1/en) AND FreePatentsOnline
  (freepatentsonline.com/7111148.html) — claim texts match across both
  sources. Legal status (Google Patents): **Expired - Fee Related, adjusted
  expiration 2022-07-07**. Classifications: G06F9/26; US class 711/220,
  712/E9.03x (processor memory/pipe-line artifacts — NOT compression classes).
- **US 7,010,665 B1** "Method and apparatus for decompressing relative
  addresses" (same inventors; filed 27-Jun-2002; granted 7-Mar-2006): **all
  34 claims retrieved verbatim** from Google Patents AND FreePatentsOnline —
  matching. Legal status: **Expired - Fee Related, adjusted expiration
  2023-03-25**. Classification G06F12/02; 711/220.
- **US 7,617,382 B1** (continuation of the '665 application, granted
  10-Nov-2009): **all 32 claims retrieved** (FreePatentsOnline). Same
  specification, same subject matter — the family is fully covered.
- Espacenet/lens.org were not directly queried this session (Google Patents
  + FPO served as the two independent full-text sources; both confirmed
  verbatim on both). Google Patents' search endpoint rate-limited (HTTP 503)
  partway through the session; direct patent-page and FPO retrieval were
  unaffected.

## 2. What the Intel family actually is (verbatim claim language)

The titles say "compressing/decompressing relative addresses"; the claims
are **CPU microarchitecture**: bit-width compaction of RIP-relative address
operands of decoded **micro-operations** inside on-die **micro-operation
storage** (trace cache / pipeline FIFO / scheduling queue / reorder buffer),
reconstructed at execution time from a per-storage-line **head instruction
pointer** plus a small transmitted correction field. Abstract ('148):
"A relative virtual address is computed in a particular stage of a processor
pipeline and then compressed according to one or more compression techniques
for storage in a micro-operation storage." Field ('148): "This disclosure
relates generally to the field of processors."

- **'148 claim 1** (verbatim): "A method comprising: decoding a first
  instruction with a K-bit displacement data to identify a first
  micro-operation; adding an address of a second instruction to the K-bit
  displacement to generate an N-bit relative address; compressing the N-bit
  relative address to generate an M-bit immediate data; and storing the
  M-bit immediate data at one or more storage locations associated with the
  first micro-operation." Dependent: N−M ≥ 14 (cl. 2), M = 34 (cl. 3),
  N = 48 (cl. 4), "the M-bit immediate data comprises a J-bit correction
  field" (cl. 9), J = 2 (cl. 10). Apparatus claim 13: decoder + address
  generator + compression logic + "a micro-operation storage ... to store,
  in one or more entries of a storage location, the first micro-operation
  and the M-bit compact representation of the address."
- **'665 claim 1** (verbatim): "An apparatus comprising: a storage medium
  having a first location to store at least a first micro-operation and an
  M-bit representation of an N-bit address, M being less than N, the M-bit
  representation having a first J-bit field; and decompression logic coupled
  with said storage medium to access the M-bit representation of the N-bit
  address and to reconstruct the N-bit address by combining at least a first
  portion of an instruction pointer address for the first location and the
  M-bit representation of the N-bit address..." Claim 13: storage "to store
  a compact representation of a relative address computed with respect to a
  first instruction pointer address, and to associate with a second
  instruction pointer address different from the first instruction pointer
  address."
- **'382 claim 1** (continuation, verbatim): "A method comprising:
  retrieving a compressed representation of an N-bit relative address
  comprising an M-bit data having a first J-bit field ... retrieving at
  least a first portion of an instruction pointer address for a
  micro-operation storage location; modifying the first portion of the
  instruction pointer address as needed to correspond with the N-bit
  relative address; and reconstructing the N-bit relative address from the
  compressed representation and the modified first portion of the
  instruction pointer address."

Mechanism summary: a 32-bit displacement + next-IP forms a 48-bit relative
address; the low 34 bits are stored as μop immediate data; the high-order
bits are recovered at execution from the head-IP stored once per μop-storage
line, adjusted by a 2-bit correction field comparing the target-IP high bits
against the line's head-IP high bits (with carry/borrow variants, '665 cl. 7).

## 3. Four-question analysis (the binding questions)

1. **Single-file self-referential compression?** NO. There is no input
   file/stream being compressed for storage or transmission, no LZ parse, no
   dictionary, no back-reference, no copy primitive, no literals. The
   "compression" is runtime bit-width compaction of already-computed address
   operands inside processor storage. The only "self-referential" flavor is
   that the reconstruction context (head-IP) is stored alongside the data in
   the same hardware structure — that is stored metadata, claimed as such
   ('665 cl. 13 "to associate with a second instruction pointer address"),
   not a self-referential match in a compressed stream.
2. **Distance-derived implicit Δ (zero transmitted bits)?** NO. No match
   distance exists anywhere in the family — there are no matches. The
   reconstruction input is (a) an explicitly STORED per-line head instruction
   pointer and (b) a TRANSMITTED J-bit (2-bit) correction field inside the
   M-bit immediate ('148 cl. 9-10; '665 cl. 1, 6-7). Honest nuance recorded:
   recovering high-order bits from locally-available context is a conceptual
   echo of "derive part of the value from decoder-visible state," but the
   derivation input is stored/transmitted metadata, never the copy distance
   d. TCOPY's Δ=−d (zero-bit parameter derived from the backreference
   distance) has no counterpart in any claim.
3. **Executable-specific vs general-purpose?** NEITHER. The family is
   CPU-runtime-specific (post-decode μop storage width), not a software
   compressor of executable files and not a general-purpose data compressor.
   It operates on addresses AFTER the address generator computed them —
   upstream of any file-format concern.
4. **Pre-LZ global filter vs match-level transformed copy?** NEITHER. There
   is no LZ layer in the claims at all; the BCJ boundary discussion from
   passes 1-4 is simply not engaged by this family.

## 4. Closest-art analysis and distinguishers table

Closest elements of the family to TCOPY: (a) exploiting redundancy of
position-dependent address fields, (b) reconstructing a field from locally
available context instead of storing it in full, (c) a small correction
field for the residual difference. None of these is at match level, in a
compressed stream, or implicitly derived from reference distance.

| TCOPY(d,L,Δ,M,R) element | Intel '148/'665/'382 | Reads on TCOPY? |
|---|---|---|
| Single-file compressed stream | No file/stream; on-die μop storage at runtime | NO |
| Self-referential LZ backreference (copy prior phrase) | No LZ parse, no matches, no copy | NO |
| Implicit Δ=−d from match distance (0 bits) | No distance; STORED head-IP + TRANSMITTED 2-bit correction field | NO |
| Match-level transformed copy | Per-μop address compaction; no match concept | NO |
| Global pre-LZ filter | No LZ layer exists | NO |
| Executable-file compressor | CPU microarchitecture (G06F9/26, G06F12/02; 711/220) | NO |

Obviousness hygiene: the family shows Intel recognized that position-dependent
address fields carry redundancy exploitable via locally-stored context — in
HARDWARE μop storage, with the context explicitly stored per line. It does
not teach or suggest applying a distance-derived transform to LZ copy phrases
inside a single-file software compressor; the problem (μop storage width vs
file size), the art unit, and the mechanism all differ.

## 5. VERDICT (binding gate item)

**TCOPY claim STANDS.** The Intel US 7,111,148 / US 7,010,665 family (incl.
continuation US 7,617,382) does not anticipate, and does not render obvious,
the narrowed TCOPY mechanism: a single-file, self-referential LZ-style
transformed copy whose additive transform parameter is derived implicitly
from the match distance (Δ=−d, zero transmitted parameter bits) in
executable code. The four-pass fear — titles "too close to wave away" — is
resolved: same words ("compressing/decompressing relative addresses"),
different art (CPU microarchitecture vs file compression). Additionally,
both '148 and '665 are EXPIRED (fee-related; 2022-07-07 / 2023-03-25 per
Google Patents legal status), so even the adjacent hardware claims are off
the table for FTO purposes; '382's expiration was not independently verified
this session.

Ledger C12 condition (2) — "Intel US 7,111,148 / US 7,010,665 full-text
review" — is DISCHARGED. Condition (1) (explicit-Δ control ablation) remains
with the experimental lanes.

## 6. Secondary targets (Pass-4 residual gaps)

- **US 6,466,999 B1 — full claims retrieved (32 claims), gap CLOSED, with a
  correction to Pass 1.** Actual title: "Preprocessing a reference data
  stream for patch generation and compression" (Sliger, McGuire, Shupak;
  Microsoft; prio 31-Mar-1999; Expired - Lifetime). Pass 1's description
  ("iterator + symbol information to optimize delta size when both inputs
  are executables") was inaccurate — symbol tables appear only as one of
  several "cross-referencing information sources" (cl. 16). Claim 1
  (verbatim): "A method of compressing an input data stream for distribution
  to a destination computer, comprising: analyzing relationships of a
  reference data stream known to exist on the destination computer to the
  input data stream; generating a set of preprocessor-driving information
  based on the relationships; deterministically preprocessing the reference
  data stream based on the preprocessor-driving information; and encoding
  portions of the input data stream as codes that reference matching
  portions of the preprocessed reference data stream; and forming a
  compressed data stream from the codes." Claim 2: "distributing the
  compressed data stream along with the preprocessor-driving information to
  the destination computer..." Definitively TWO-FILE (reference "known to
  exist on the destination computer") with TRANSMITTED preprocessing
  directives (block-motion/"rift table" fixups of jump/call references in
  the reference before matching). Not single-file; not implicit; not
  match-level.
- **Apple dyld chained-fixups patent number — NOT CONFIRMED (honest
  negative).** FreePatentsOnline full-text search: exact phrase "fixup
  chains" → 0 hits; "chained fixups" OR "fixup chain" → 0 hits across
  US/EP/JP/PCT. Google Patents' search endpoint rate-limited (503) before a
  usable query. The mechanism itself is publicly documented (rebase/bind
  chains encoded in unused pointer bits; ipsw tooling documentation
  retrieved this session). Pass 1's classification is unchanged —
  CLOSE-BUT-DIFFERENT (pointer/rebase METADATA encoding, not LZ-copied
  instruction bytes, no arithmetic transform of copied bytes). Caveat
  recorded: absence of the marketing phrase in FPO full text is NOT proof
  that no Apple patent exists; claims could use different wording. Status:
  UNRESOLVED-NUMBER, classification unchanged.
- **Qualcomm dedicated query — RUN, decisive, with a correction.** FPO
  query: "code compression" AND "relative addressing" AND Qualcomm →
  exactly ONE hit: US 7,676,506 B2 "Differential file compression of
  software image versions" (claims 1-19 retrieved). **Correction: the FPO
  bibliographic record shows Assignee = Innopath Software, Inc.;
  QUALCOMM Incorporated appears only as Attorney/Agent or Firm ("Formerly
  Paradice and Li").** The swarm record's Qualcomm attribution to this
  patent appears to rest on that attorney-field. Claim 1 (verbatim, key
  limitation): "identifying a plurality of regions in an original version
  of software image and in a new version of a software image ... describing
  the differences using a transformation that depends on values of groups
  of N bytes in the corresponding regions." Transformation G(x)=x+f(x)
  (piece-wise constant f) applied as transmitted CFD hint data between TWO
  file versions. Close-but-different CONFIRMED at claims level: two-file,
  transmitted transform parameters, no self-reference, no distance-derived
  Δ.
- **IBM dedicated query — RUN, no qualifying art.** FPO query: "delta
  compression" AND executable AND relocation AND IBM → 16 hits, none
  teaching the mechanism (VM/storage migration, de-duplication, floating-
  point, LPT-table compression). Closest IBM art: the "Systems and methods
  for efficient data searching, storage and reduction" family (US 8,275,755
  / 8,275,756 / 8,275,782 / 9,378,211 / 9,400,796 / 9,430,486 / 10,282,257
  / 10,649,854 — Ajtai-lineage differential compression): similarity-search
  against a repository, then delta encoding — repository/two-party, not
  self-referential per-match transform.

## 7. Residual gaps (pass 5, honest)

- Apple chained-fixups patent number still unconfirmed (phrase-level FPO
  negative only; Google Patents search rate-limited mid-session).
- US 7,617,382 legal status/expiration not independently verified.
- Espacenet and lens.org not directly queried (two full-text sources used:
  Google Patents + FreePatentsOnline, verbatim-matched for all three Intel
  family patents).
- Non-patent prior art unchanged from passes 1-4 (VCDIFF, zdelta/vdelta,
  GenCompress, RLZAP, ZPAQ/PCOMP, BCJ, Courgette/Zucchini).
- Formal FTO attorney review remains recommended before any commercial
  claim (this document is a literature/patent-classification gate record,
  not legal advice).

## 8. Five-pass convergence (final)

Passes 1-4 (independent) + Pass 5 (full-text claims, binding item) agree:
the closest art is (a) global once-per-file relocation/branch normalization
(BCJ/Philips/IBM-'656414-lineage), (b) two-file copy-with-edits delta
(Microsoft '999/'506-lineage, VCDIFF, Zucchini), (c) hardware runtime
address compaction (Intel '148/'665/'382 — now read in full), or (d)
loader metadata encoding (Apple chained fixups). **No accessible prior art
teaches the single-file self-referential LZ copy with an implicit
distance-derived additive transform (Δ=−d) at match level.** The TCOPY
novelty claim's patent-classification condition is CLOSED; the remaining
gate conditions are the experimental ones recorded in ledger C12.

---

# External Research Pass 6 — GORILLA / vXOR float-domain orbit (21 Aug 2026, non-conflict documentation)

Coordinator briefing item (GROTLI × GORILLA × ANVIL timeseries auto-detect):
document Gorilla-class transforms as NON-CONFLICTING prior art relative to the
closed TCOPY gate and the validated ARI-REF transmitted-Δ form. This is a
documentation pass (classification only, no new novelty claim), per the same
honesty rules as passes 1–5.

## 1. What Gorilla is (standard literature; not independently re-fetched this session)

Gorilla = Pelkonen et al., "Gorilla: A Fast, Scalable, In-Memory Time Series
Database", PVLDB 8(12), 2015 (Facebook/Meta). Two per-column transforms on a
FIXED-STRIDE numeric time series: (a) timestamps — delta-of-delta
D_n = Δ_n − Δ_{n−1} with 5-tier codes (0→1b; ±[−63,64]→9b; ±[−255,256]→12b;
±[−2047,2048]→16b; else 36b); (b) values — XOR_n = u_{n−1} ⊕ u_n with
leading-zero/trailing-zero reuse (Case A xor=0 → 1b; B1 reuse → 2+mb;
B2 → 13+mb). Both transforms are PREDICTOR-RELATIVE against the immediately
previous value in the SAME column — the reference is fixed by the scheme,
never searched, and there is no byte-stream match primitive anywhere.

## 2. Patent-landscape queries run THIS session (FreePatentsOnline full-text)

- Query "delta-of-delta" AND timestamp AND compression → 53 hits. Top
  relevant cluster is THIRD-PARTY metrics/TSDB art, NOT Facebook:
  Splunk "Compressing digital metrics ... time slice delta compression"
  (US 11,463,559 / 11,902,402 / 11,949,764), "Systems and methods for
  compressing digital data" (US 12,284,261; apps US 2024/0244121,
  US 2025/0379922), "Double-Delta Filter Systems and Methods"
  (US 2026/0180558, SIMD register-width double-delta), plus Splunk DIQS
  metrics-store family. Several of these CITE Gorilla in their references
  (corroborates Gorilla as public-literature antecedent).
- Query "delta of delta" AND "time series" AND Facebook → 4 hits, all the
  Splunk cluster above (Facebook appears only in citation text).
  **No Facebook/Meta-ASSIGNED Gorilla patent surfaced.** Caveat recorded:
  one targeted query is evidence, not proof of nonexistence.
- Query "floating point" AND "leading zeros" AND compression → 1203 hits
  (broad; top-50 reviewed). Dense float-compression cluster, all
  column/statistics-oriented, NONE LZ-match-level: IBM "Floating point data
  set compression" (US 10,756,756 / 11,018,692 — lossless f64 time series),
  exponent-token family (US 8,959,129 / 8,959,130), exponent-threshold
  classification (US 9,047,118), Elf erasing-based (US 12,289,120 /
  US 2024/0250694), decimal-float compressed operands (US 10,365,892).
- Burtscher FPC lineage (FPC: high-speed compressor for double-precision
  floating-point data, DCC 2006 / IEEE TC 58(1) 2009 — XOR against fcm/dcm
  predicted value + leading-zero class coding): standard academic literature,
  PREDATES Gorilla by ~9 years as the XOR-predictor-float antecedent; no
  FPC-assigned patent surfaced in the queried space. Recorded as
  standard-literature citation (not independently re-fetched this session).

Derivation classes (per ledger rule): query hit lists + assignees =
measured-exact this session (FPO); Gorilla/FPC paper content = standard
literature; coordinator-briefing figures (tier sizes, −36.3% vXOR NDJSON)
= modelled/measured upstream, not re-measured here.

## 3. Non-conflict analysis

| Mechanism | Gorilla/vXOR orbit | Conflict? |
|---|---|---|
| TCOPY implicit Δ=−d (executable code) | Float/value columns; XOR or DoD vs PREVIOUS value; fixed stride assumed | NO — different domain (machine code), different reference (LZ backreference at distance d), different derivation (Δ from match distance, zero bits) |
| ARI-REF transmitted-Δ (integer additive, validated form) | Closest structural cousin: predictor-relative transform on uniform column | NO conflict — same GENUS (encode relative to decoder-visible previous state = the delta-coding principle itself: LZ77, VCDIFF, FPC, Gorilla all instantiate it), different SPECIES (discovered variable-distance references inside a general byte stream vs fixed-column previous-value) |
| PNRA invariant anchoring | No analog (search-formulation mechanism) | NO |
| SPARSE-REF correction masks | Gorilla has no sparse-residual stream | NO |

Shared-lineage statement (for strategy/arch): Gorilla, FPC, delta patching,
and TCOPY/ARI-REF all descend from the same public principle — encode a value
relative to already-reconstructed context. This principle is NOT claimed by
ANVIL and must never appear in a novelty claim. The ANVIL-specific elements
remain exactly those the five-pass record isolated: match-level transforms,
distance-derived (implicit) parameters, transformation-invariant discovery,
and sparse-correction topology as first-class entropy streams. Gorilla adds a
float-domain INSTANCE of the genus — useful as a competing baseline (strategy's
matched-tier Pareto table) and as confirmation that predictor-relative
transforms are rewarding on smooth numeric columns, not as a blocker.

FTO note: the coding PRIMITIVES (DoD tier codes, XOR bit-reuse) are published
academic methods (VLDB 2015; FPC 2006/2009) with multiple independent
open-source implementations; the third-party patent cluster concentrated in
database-system implementations (Splunk et al.) covers system architectures,
not the primitives ANVIL would use. Standard caveat: this is a classification
record, not legal advice; FTO attorney review remains recommended before any
commercial claim.

## 4. Residual gaps (pass 6)

- Espacenet/Google Patents not re-queried this session (FPO only; GP was
  rate-limited earlier in the day).
- Facebook/Meta Gorilla patent absence: single-query negative, not exhaustive
  (assignee variants: Meta Platforms, Facebook Ireland, application-only
  filings under different wording).
- vXOR (vertical/columnar XOR) per-record frame detection: no dedicated
  patent query run (grotli-codec implements it; if ANVIL adopts vXOR-shaped
  framing, run a dedicated columnar-frame query first).
- Gorilla paper text not independently re-fetched; figures cited from the
  coordinator briefing + standard literature.



