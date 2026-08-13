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
