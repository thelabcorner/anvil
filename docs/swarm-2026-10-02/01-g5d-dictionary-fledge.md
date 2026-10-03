# 01-g5d-dictionary — FLEDGE ALPHA (independent adversarial review)

**Role:** independent adversarial reviewer. Sections 0–13 (INTERIM) were written with **no**
Space-Bunny output consulted — at that moment `01-g5d-dictionary-space-bunny.md` did not exist
(verified `Test-Path` = False). **STATUS NOW SUPERSEDED IN PART: that report landed at 19:30:14 and
has been read in full.** The interim sections are retained unaltered as the record of what was
derived independently, and are reconciled point-by-point in **ADDENDUM B** (§B.0–§B.8), which
records two errors of mine, three of the constructive lane's, and the shared final ruling. Where
Addendum B contradicts an interim section, **Addendum B governs.**

**Worktree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty. No commit, push, reset, clean,
stash, restore, or rebase was performed. No existing file was modified. No local corpus
compression, benchmark, sweep, or fuzz campaign was run.

**Artifacts read (source of record):**
`docs/swarm-2026-10-02/MASTER-BRIEF.md`; `docs/I10-G5-PAGED-DICTIONARY-PREREG.md`;
`docs/I10-GROTLI-G3-RESULTS.md`; `docs/I10-GROTLI-G2-RESULTS.md`; `RESEARCH_LEDGER.md` L4820–4964;
`tools/grotli_g5_paged_dictionary.cpp` (1,687 lines, 82,006 B).

---

## 0. Verdict (interim)

> **HOLD** the dictionary family; **KILL** the *discovery design as currently preregistered*;
> **PILOT** one census-first, remote-only, factorial-ablation run.

Reasoning in one paragraph. The mechanism is 100% adopt-class by the prereg's own admission
(§19), so no novelty claim is available. Worse, the prereg changes **four** things at once against a
single control set that isolates **none** of them, so a `PASS-G5D-DISCOVERY` would be
uninterpretable and a `NO-GO` would be ambiguous. Independently, I can prove from the source that
the overlay mechanism is **inert on every leaf whose occurrence count does not exceed the page
policy**, and the whole discovery gate reduces arithmetically to a **≥6,924 B** saving that must
almost entirely come from D3 alone — the single file where the mechanism can even be active. The
correct next spend is therefore a **counting-only census** (no compression, no timing), which
either kills the family outright or authorizes one properly controlled ablation.

---

## 1. MEASURED facts (read directly from frozen result documents / source)

These are not projections. Every number below is quoted with its artifact.

### 1.1 G2/G3 baseline arms (complete bytes, q11/lgwin30)

| Family | Raw Brotli | G2 whole | G3 RAW | G3 DICT | G3 INT | G3 MIXED | G3 routed selected |
|---|---:|---:|---:|---:|---:|---:|---:|
| D1 Amazon | 40,126 | 39,299 | 39,460 | 39,277 | 39,460 | 39,277 | **39,277** (G3 DICT) |
| D2 CDISC | 25,029 | 20,903 | 25,964 | 20,861 | 26,044 | 20,891 | **20,861** (G3 DICT) |
| D3 GH Archive | 1,292,757 | — | 1,240,884 | 1,240,155 | 1,247,531 | 1,245,816 | **1,240,155** (G3 DICT) |
| D4 CROVIA | 108,857 | 84,361 | 87,089 | 84,431 | 86,794 | 84,361 | **84,361** (G2 whole) |

Source: `docs/I10-GROTLI-G3-RESULTS.md` §1.

- Σ G3 routed selected = **1,384,654 B**. Σ `G3_REGION_DICT` = **1,384,724 B**.
- D3 is **89.56%** of the routed aggregate; D3+D4 = 95.7%.
- **V1 (held-out) = 100% structured, 3 exact shapes, 19 DICT leaves, and +3.1900% WORSE than raw**
  (2,179,615 vs 2,112,235) — `docs/I10-GROTLI-G3-RESULTS.md` §3, ruling `PASS-G3-NARROW`.

### 1.2 G5D implementation facts (from `tools/grotli_g5_paged_dictionary.cpp`)

| Fact | Location |
|---|---|
| `kPagePolicies = {4096, 16384, 65536}` | L154 |
| `kCapCandidates = {0,16,64,256,1024,4096,16384,65536}` (8 root caps, 8 overlay caps) | L155 |
| root ranked by **descending leaf-wide occurrence**, ties by byte lexicographic | L360–376, L445 |
| overlay ranked by **descending page-local occurrence**, excluding root members | L456–477 |
| page id derived from occurrence counter: `page_id = item.coord.occurrence / leaf.policy` | L868, L1229 |
| `code_width = ceil(log2(root_count + overlay_count + 1))` | L481, L1010 (re-derived & checked on decode) |
| root count rejected if `root_count > occurrences` | L961 |
| overlay count rejected if `overlay_count > page_occurrences` | L991 |
| escapes buffered into a **separate region** appended after the leaf payload, so escapes do **not** disturb packed-ID bit alignment | L879–895 |
| `L_frequency_model = 0` — no entropy model of the ID stream; Brotli models the packed-ID bytes | PREREG §9 |
| arms emitted = 2 rawlex + 6 dictionary (2 orders × 3 policies) | L1465, L1499, PREREG §11 |
| **no forced-overlay=0 arm, no flat-dictionary arm, no frequency-order-only arm anywhere in the source** | absence, verified by grep over `overlay`/`root_cap`/`policy` call sites |

### 1.3 Ledger status (measured, 2026-09-25)

`RESEARCH_LEDGER.md` L4875–4924: prototype compiles warning-clean `/W4 /WX`, both selftests
PASS, `measure README.md` smoke emits real `decode_peak_rss_kib: 5416`, and the previously
hard-coded invariants are now computed. **Dispatch is blocked only on authorized commit/push.**
G5A ordering is **−6.1189%** on D (V1 adverse); G5B-ORDINAL **+1.6507% adverse**; G4
**`NO-GO-G4`** with speed column `INVALID_SPEED_ACCOUNTING`. G5A ordering is `-6.1189%`.

---

## 2. DERIVED facts (proved from §1, not measured)

These are my own deductions. They are labelled DERIVED because no run has produced them.

### D1 — The discovery gate reduces to D3 alone

Prereg §14.2 requires `Σ min(G3_selected, G5D arms) ≤ 0.995 × 1,384,654`, i.e. a saving of
**≥ 6,924 B**. §14.3 requires `Σ best_G5D_dict ≤ 0.995 × 1,384,724`, i.e. **≥ 6,924 B** saved
against `G3_REGION_DICT`.

Per-file headroom available to the routed aggregate:
- **D4: ≤ 70 B.** `G2_WHOLE` = 84,361 is a permanent candidate and wins ties; `G3_REGION_DICT` =
  84,431. G5D can recover at most 70 B on D4 even with a perfect dictionary.
- D1/D2 are already 2.1% / 16.7% below raw on the flat dictionary — G5D must beat an
  already-tight flat dictionary on small files.
- Therefore **≥ ~6,000 B of the 6,924 B must come from D3**, i.e. **0.48–0.56% of D3's
  1,240,155 B**, from the one file where a leaf can plausibly exceed 4,096 occurrences.

### D2 — Overlay is provably inert when occurrences ≤ page policy

For a leaf with `L` occurrences and page policy `P`:
- If `L ≤ P` there is exactly **one** page. `local_ranked` (page-local frequency ranking) is then
  computed over the *same* token multiset as the leaf-wide `ranked` (L456 over the page's token
  slice = the whole leaf). The root already admits up to `min(65536, distinct)` tokens.
- Any token not in root must either enter the overlay — costing its own `len + uvar_len` table
  bytes plus 1 byte `overlay_count` plus 1 byte `code_width` — or escape, costing
  `len + uvar_len` again. The overlay therefore pays `table_bytes + 2` to save at most the escape
  bytes it displaces, **plus** any ID widening.
- **Conclusion (DERIVED): for every leaf with `L ≤ P`, the MDL selector's optimum is
  `overlay_count = 0`, `escape_count = 0`, and G5D reduces exactly to G3's flat dictionary plus
  2 bytes of framing.** It is strictly worse, never better.

Corollary: G5D's novel degree of freedom is **confined to leaves with `L > 4,096`**. D1 (277,673 B),
D2 (615,350 B) and D4 (3,585,053 B) are unlikely to contain many such leaves; D3 (10,485,760 B,
11,228 structured frames) is the only file where the mechanism can be exercised at all.

### D3 — Flat-equivalent overhead of the prereg's own arms

Applying D2 to all three policies: G5D's best case per leaf costs
`G3_flat + 2 × ceil(L_leaf / P)` bytes. For D3, if D3 has ~2,000 shape-slot leaves (ASSUMED — no
leaf count for D3 exists in any results document; this is my single biggest evidence gap), the
P4K arm's floor is **≈ +4,000 B over `G3_REGION_DICT`** before any paging benefit. The gate needs
−6,924 B. The prereg's own framing overhead puts the family **~11 KB away from its own gate**.

### D4 — Occurrence-count and distinct-count are negatively coupled here

For a leaf to benefit from paging it must satisfy **both** `L > 4,096` **and** a locally-hot
out-of-root token mass. In NDJSON structured logs these are anti-correlated:
- **High `L`, low `D`** (constant/低-cardinality columns: `"type":"event"`, flags, enum codes) —
  root absorbs everything, overlay is inert, 2 B/page pure loss. These are exactly the leaves that
  reach `L > 4,096`.
- **High `L`, high `D`** (timestamps, UUIDs, free-text) — either locally all-distinct (no local hot
  set → no overlay) or the repeats are adjacent runs that Brotli already codes for free (§4.3).

This is the central structural objection. The mechanism needs the quadrant the corpus does not
populate.

---

## 3. STRONGEST FALSIFICATION CASE (the kill argument)

Stated as tightly as I can make it.

> **K1 (credit collapse).** G5D changes four things simultaneously against G3's `EXACT_DICT`:
> (i) page/overlay structure, (ii) mandatory escape, (iii) **root ordering by descending frequency**
> instead of G2/G3's **first-occurrence table order** (PREREG §1.2 vs §7.2), and (iv) a bounded root
> cap. The prereg's entire control set is `RAW_BROTLI`, frozen G2 whole, frozen G3 region arms, and
> two `RAWLEX` arms (§11.1–§11.3). **There is no arm that applies frequency ordering to a flat
> dictionary.** Therefore *any* pass is uninterpretable and *any* fail is ambiguous. A PASS cannot
> distinguish "paging works" from "you renumbered the ID alphabet by frequency and Brotli modelled
> the resulting byte skew better." Frequency-ordered symbol tables are textbook, so that win — if
> it exists — is adopt-class and belongs to G2, not to G5D.
>
> **K2 (prior-art collapse).** Page dictionaries with per-page local alphabets, bit-packed IDs,
> and mandatory escapes are Parquet dictionary pages, Blosc/FastLanes block dictionaries, and FSST's
> escape-plus-symbol layout. The prereg concedes all of it (§19). With no primitive novelty claimed
> and now no isolating control, G5D cannot produce a project-level claim either: §19 says a paper
> needs "independently established" causal accounting, which §14's gate design cannot deliver.
>
> **K3 (mechanism inertness).** By D2, the overlay contributes nothing on any leaf with
> `L ≤ 4,096`. By D1, the whole gate is D3. By D4, the one file where paging can act is the one
> file whose leaf cardinality profile is *anti*-correlated with the mechanism's requirement.
>
> **K4 (selector risk, already paid once).** §7.5 selects the root cap and every page's overlay cap
> by **exact serialized leaf cost** — an MDL proxy. G4's identical class of choice (cheap surrogate
> score instead of true q11 bytes) already returned `NO-GO-G4` with its speed column withdrawn as
> `INVALID_SPEED_ACCOUNTING`. Only 3 page policies are arbitrated by real q11; the 8 root caps and
> 8 overlay caps are **not**. So G5D re-runs a decision procedure that has one recorded failure.
>
> **K5 (hidden cost the accounting does not carry).** PREREG §9 charges wire bytes only. It does
> not charge `L_decoder_memory`, and G5D's decode working set is materially larger than a single
> flat dictionary — see §5.

**If K1–K5 hold, the honest outcome is not "G5D failed." It is "G5D's experiment cannot fail
usefully," and the remote cycle is wasted.**

---

## 4. PRIOR-ART MAP

| Mechanism element in G5D | Established lineage | Novelty available? |
|---|---|---|
| Exact lexical dictionary of distinct column values | Parquet dictionary encoding; Brotli static dictionary (RFC 7932); Zstd trained dictionaries | **No** |
| Per-page / per-block local dictionary alphabet | Parquet dictionary pages; Blosc/FastLanes block dictionaries; Brotli & Zstd block splits | **No** |
| Fixed-width bit-packed symbol IDs | Parquet bit-packing hybrid; classical `dictionary_id × width` coding | **No** |
| Mandatory escape code + raw payload | FSST escape; Parquet dictionary fallback; every prefix/suffix codable scheme | **No** |
| Frequency-ordered symbol table | Universal; Huffman/FSE canonical-order practice | **No** |
| `ceil(log2(n+1))` width quantization | Textbook | **No** |
| MDL cap selection by serialized leaf cost | Standard MDL model selection | **No** |
| **Paging *plus* a *shared* root *plus* per-leaf namespace** | closest to Parquet + page-local extension | **No** — an arbitrary combination of known components, explicitly disqualified by MASTER-BRIEF binding doctrine 1 |

**Verdict on novelty:** zero. This is adopt-class. Per MASTER-BRIEF doctrine, that is acceptable
*only* if it wins on an accounted Pareto axis. So the entire burden of proof shifts to ratio and
resource accounting — where, per §3, the evidence is thin and the controls are missing.

---

## 5. HIDDEN-COST AUDIT (costs the prereg does not charge)

### 5.1 Decoder dictionary memory is not in the accounting at all

PREREG §9's `L_total` enumerates wire bytes. There is **no `L_decoder_memory`** term, yet the
performance gate at §15 constrains peak RSS to ≤110% of current routed decode.

Worse, reconstruction is in **source order** across leaves (`tools/grotli_g5_paged_dictionary.cpp`
L1166–1201: a per-group cursor emits destinations in original source order, each carrying its
`leaf`). A single source frame touches many distinct shape-slot leaves, so **many leaves'
dictionaries must be simultaneously resident and randomly indexed** during decode.

Working set ≈ `Σ_leaves [ root_count×8 B (offset,len index) + root_table_bytes + overlay_table_bytes ]`.

DERIVED scale estimate (ASSUMED leaf counts, flagged): if D3 has ~2,000 leaves averaging ~500 root
entries at ~12 B mean token length, that is ~2,000 × (4,000 + 6,000) ≈ **20 MB of live dictionary
per decoded object**. G3's flat dictionary has the same structural problem, so this is not a
*regression* — but the 110% RSS gate leaves no headroom for the overlay tables, and **§9 gives the
experimenter no budget line to stay inside.** That is an accounting hole, not a cost.

### 5.2 The `ceil()` quantization gives G5D unearned headroom — and is unmeasured

`code_width = ceil(log2(root_count + overlay_count + 1))`. Adding a small overlay often leaves the
width **unchanged** (e.g. `R=1024,O=1` → both widths 11). So the overlay can grow the alphabet at
**zero ID cost**, and its real effect is *renumbering* — local-frequency IDs replace global-frequency
IDs in the packed-ID byte region. That is (a) unearned by any continuously-width scheme, and
(b) exactly the effect confounded with K1. Without a `flat + frequency-order + no-overlay` control,
the experiment cannot tell free-widening from renumbering from paging.

### 5.3 Escape economics are strictly hostile in the low-value regime

Escape cost per occurrence = `uvar_len(len) + len` bytes. Flat-ID cost = `w/8` bytes. Escape only
wins when `len + uvar_len(len) > w/8` **and** the token cannot be admitted — but the root admits
every distinct token up to 65,536. Escape therefore only ever fires for tokens that are (i) beyond a
65,536-distinct leaf, or (ii) not selected by the MDL cap. Both are *rare-by-construction* regimes,
and in regime (ii) escape is a pure loss versus the flat dictionary. **The mandatory escape is a
decoder-memory bound bought at a ratio cost, with no ratio upside in the common case.** That is a
legitimate engineering trade (§7) but it is not a ratio mechanism.

### 5.4 Per-page framing is charged but never amortized

`overlay_count uvar` + `code_width u8` = 2 B per page, unconditionally, even for width-zero /
zero-overlay pages. DERIVED: this is the entire G5D-vs-G3 delta in the inert regime, and per D3 it
is ~+4 KB in G5D's disfavour.

### 5.5 Encoder-side cost is unbounded and unmeasured

§7.5 requires, per candidate root cap, that **every page independently evaluate every candidate
overlay cap** against exact serialized cost — that is `8 root caps × 8 overlay caps × total pages`
cost evaluations per leaf, plus 3 page policies × 2 orders. Prereg §9.1 concedes "encoder compute
is not a wire cost" but §15 caps end-to-end encode at 2× the control. With `P4K` on a 10 MiB object
the page count can be in the thousands; the encode gate is at genuine risk of being the binding
constraint rather than decode.

---

## 6. DECODER / RESOURCE RISKS

| Risk | Assessment |
|---|---|
| Per-token division for page id | **Mitigable.** `occurrence / policy` (L868) can be strength-reduced to a per-leaf counter compared against the page end, since each group's cursor is sequential (L1182). Must be stated in the prereg; not currently. |
| Overlay index rebuild per page | O(`overlay_count`) direct-indexed stores at page boundary. Cheap, sequential, predictable. **Not a risk.** |
| Cold ESC branch | Predictable, rare in the common regime. **Not a risk.** |
| Multi-leaf random indexing (the real risk) | Source-order reconstruction across many simultaneously-live leaves ⇒ cache-miss storm across N separate `(offset,len)` tables. **Unquantified in the prereg.** This is the decode risk that matters. |
| RSS | G5D ≥ G3 structurally (overlay tables + per-leaf policy). 110% gate may bind before ratio does. |
| Correctness | Source is defensively written: widths re-derived and checked on decode (L1010), bounds checked pre-allocation (L961, L991), escapes region-separated so bit alignment is safe (L879–895), canonical-varint and alignment assertions present (L677). **I found no correctness hole by inspection.** 42-case malformed-input matrix in §13 is appropriate. |
| Malformed-input safety of `ceil` width | `code_width` is validated against recomputed `bit_width_u64(active)`, so a forged width is rejected. Good. |

---

## 7. STRONGEST SURVIVING CASE (steelman, stated fairly)

I must state this at full strength, because it is real and it is *not* a ratio argument.

1. **Bounded decoder dictionary memory with exact escape is a genuine robustness improvement.**
   G2/G3 `EXACT_DICT` admits **every distinct token** per leaf with **no escape** (PREREG §1.2). On a
   high-cardinality leaf that is an *unbounded* decoder-side table. G5D's `root_count ≤ 65,536` plus
   mandatory escape converts an unbounded allocation into a bounded one with exact behaviour. That
   is a real, adoptable engineering property that the current `EXACT_DICT` line simply does not have,
   and it is independently valuable regardless of ratio. It also directly serves the
   resource/format-security lane (track 19).
2. **The escape is strictly more robust than G2's silent full admission**, and the 42-case
   malformed-input matrix is a genuine contribution to format safety.
3. **Discovery gains, if any, are large enough to be worth one bounded remote cycle.** G2/G3 show
   −16.65% and −22.50% same-backend wins on D2/D4 from exact lexical dictionaries. The family is
   not dead; the *paging axis* is what I am attacking.
4. **Routing makes the experiment non-regressive.** `C_selected = min(raw, G3, all G5D)` with
   `R_fallback == 0` required (§9.3) means a G5D arm cannot make the portfolio worse. The downside
   is bounded and the census cost is small.

**But note carefully: (1)–(3) are arguments for the *bounded-escape dictionary*, not for the
*paged base+overlay dictionary*.** None of them require the page/overlay axis at all.

---

## 8. HIDDEN COST: benchmark-window provenance

Checked because the checkpoint asked. Findings:

- PREREG §12.2 requires `build_ms`, `encode_ms`, `decode_ms`, `decode_peak_rss_kib` **in the
  discovery rows**, and §12 closes with "Timing is diagnostic in the discovery workflow and is not
  accepted as a cross-machine Pareto result." That is the correct guardrail and it is present.
- Risk: emitting per-arm timings in discovery rows invites them being read as a Pareto result. The
  §15 gate correctly forbids it ("Timing from the discovery workflow cannot satisfy this gate").
  **Recommendation:** the discovery schema should emit timings in a clearly quarantined field name
  (e.g. `diagnostic_only_build_ms`) so a downstream summarizer cannot mistake them for gate
  evidence. Currently they sit in the same object as the gate-relevant integers.
- RSS: §12.2 asks for "dedicated decode peak RSS **when the performance gate is run**," which
  correctly avoids paying the measurement cost during discovery. Good.
- Ordering provenance: §4 requires the G5A source-of-record identity to be verified *before* a G5D
  corpus object is measured, and both orders must share identical dictionary plans and table
  content. This is well specified and, given G5A's −6.1189% / V1-adverse split, it is essential:
  **an order effect of that size would swamp any paging effect**, so §14.4's rule that the mechanism
  is credited only from dictionary-vs-same-order controls is load-bearing and correct.

---

## 9. PROJECTED-vs-MEASURED audit (are projections being mistaken for facts?)

| Claim in circulation | Actual status |
|---|---|
| G2 `EXACT_DICT` is a real typed mechanism (−16.48% D2) | **MEASURED** (G2 RESULTS §) |
| G3 regionization generalizes | **MEASURED FALSE** — V1 +3.19%, `PASS-G3-NARROW` |
| G5D prototype compiles / selftests / measures RSS | **MEASURED** (ledger L4875–4884) |
| G5D invariants are computed not hard-coded | **MEASURED** (ledger L4912–4924) |
| G5A ordering is beneficial | **MEASURED** −6.1189% on D, **adverse on V1** — not general |
| G5D root-cap MDL selection predicts q11 bytes | **UNSUPPORTED** — G4's identical proxy class returned `NO-GO-G4` |
| "Paging will capture dictionary drift" | **PROJECTION** — no drift measurement exists on D1–D4 |
| "P4K/P16K/P64K are three meaningfully different arms" | **PROJECTION, probably FALSE** — if most leaves have `L < 4,096`, all three policies produce ~1 page/leaf and the three arms **collapse to near-identical configurations** (D2). Untested. |
| D3 leaf count / occurrence histogram | **UNKNOWN — my largest evidence gap.** No results document reports per-leaf occurrence counts for D3. |
| Any G5D ratio number | **DOES NOT EXIST.** Zero G5D corpus bytes have ever been measured. |

**No G5D ratio result has been produced. Any document claiming one is fabricating or projecting.**

---

## 10. DECISIVE REMOTE-ONLY EXPERIMENT (frozen, two-stage, census-gated)

Doctrine-compliant: no local corpus work, no local timing, single remote dispatch.

### Stage 1 — LEAF CENSUS (counting only; no compression, no timing, no benchmark)

Emitted per shape-slot leaf for D1–D4 (and V1 for context only, zero gate weight):
`L` (occurrences), `D` (distinct), and derived:
- `pages_P = ceil(L/P)` for P ∈ {4096, 16384, 65536};
- `pages_total` summed per file;
- `mass_heavy = Σ occurrences in leaves with L > 4096` / `total_occurrences`;
- `mass_heavy_distinct = Σ occurrences in leaves with L > 4096 AND D > 4096` / `total_occurrences`;
- `overlay_break_even_bytes` = the exact §11 quantity (below), summed per file.

**Exact overlay break-even quantity** (this is the decision-relevant number; it is computable
without any compression):

```
for each page p of leaf with occurrences L, root R (capped), policy P:
  w0 = ceil(log2(R+1));  w1 = ceil(log2(R+min(65536,D_local_not_in_root)+1))
  overlay_table = Σ_{i≤O}(uvar_len(len_i) + len_i) + uvar_len(O) + 1
  widening      = page_occurrences * (w1 - w0) / 8
  displaced     = Σ over overlay uses of (uvar_len(len_k) + len_k)      # what escape would have cost
  net_p         = displaced - overlay_table - widening - 2
BE_overlay = Σ_p max(0, net_p)
```

**Pre-registered Stage-1 gates (frozen before dispatch):**
- `INVALID` if any identity, hash, or round-trip check fails.
- **`KILL-FAMILY`** if `BE_overlay(D3) < +6,924 B`, **or** if
  `mass_heavy_distinct(D1..D4) < 0.05`. Rationale for the 6,924: it is exactly the §14.2/§14.3
  gate; a carrier that is not ≥6,924 B smaller before Brotli cannot plausibly become 0.5% smaller
  after Brotli at this scale, and declining to spend the compression cycle on that basis is the
  whole point of the census.
- **`KILL-DEGENERATE-DESIGN`** if `Σ_D3 pages_total` < 500 — that means the three page policies are
  not distinct arms and the prereg's 6 dictionary arms carry ~1 configuration.
- **PROCEED** to Stage 2 only if none of the above trips.

### Stage 2 — FACTORIAL ABLATION (the cells the prereg is missing)

Per file, per order, remote, alternating arm order, pinned binaries, same backend
(q11/lgwin30), median of ≥9 with warmups:

| # | Arm | Isolates |
|---|---|---|
| 1 | `RAW_BROTLI` | floor |
| 2 | `G3_REGION_DICT` (reproduced) | control |
| 3 | **`G2_FLAT_FREQ`** — single page, root = all distinct, **frequency order**, no escape, no cap | **missing cell: frequency ordering alone** |
| 4 | `G2_FLAT_FIRSTOCC` — single page, root = all distinct, first-occurrence order (= arm 2 semantics, reproduced) | baseline for arm 3 |
| 5 | **`G5D_ESC_BOUNDED_FREQ`** — single page, root ≤ 65,536, frequency order, mandatory escape, **no pages, no overlay** | **missing cell: cap + escape alone** |
| 6 | `G5D_P{4,16,64}K` | paging + overlay, on top of 5 |

Credit rule, frozen: **paging is credited only from `arm6 − arm5`**, never from `arm6 − arm2` and
never from `arm3 − arm4`.

### Pre-registered thresholds

- **`KILL-PAGING`** if `Σ(arm6 − arm5) ≥ 0` on the aggregate (paging is net-negative against its own
  flat parent). *This is the primary kill and it is the cleanest comparison in the whole design.*
- **`NO-GO-G5D-PAGING`** if `Σ(arm6 − arm5) > −0.5% × Σ arm2`, or if paging wins on <2 of 4 files.
- **`PASS-G5D-PAGING`** (authorizes only a separately frozen performance micro) requires **all**:
  1. `Σ(arm6 − arm5) ≤ −0.995 × Σ arm2` (paging, isolated, ≥0.5%);
  2. `arm6 < arm5` strictly on ≥2 of D1–D4;
  3. `arm5 ≤ arm2` (the bounded-escape dictionary is itself not a regression — this is the
     adoptable property from §7);
  4. `arm3 < arm4` reported and **any** frequency-ordering gain credited to G2/G3, not G5D;
  5. every §13 malformed-input gate passes, all arms round-trip exactly, component sums reconcile;
  6. V1 reported separately, zero gate weight;
  7. timing fields quarantined as diagnostic-only.
- **Performance (separately frozen micro, not this run):** end-to-end decode ≥95% of reproduced
  `G3_REGION_DICT` decode; peak RSS ≤110%; encode ≤2× control; **plus a new explicit
  `L_decoder_memory` budget line** that must be added to §9 before any RSS number is accepted.
- **Held-out:** unopened until Stage 2 passes; requires ≥1 previously unseen object with genuine
  regional dictionary drift, plus a negative control, both preregistered before fetch.

**Falsification criteria, stated as kill conditions (any one suffices):**
1. `BE_overlay(D3) < 6,924 B` on census → family dead without compression.
2. `mass_heavy_distinct < 0.05` → no leaf population can benefit.
3. `Σ(arm6 − arm5) ≥ 0` → paging adds cost and no value.
4. Win is fully explained by `arm3 − arm4` (frequency ordering) → the mechanism is not paging;
   reclassify as an adoptable G2 change and close G5D.
5. Peak RSS exceeds 110% once `L_decoder_memory` is charged → resource kill regardless of ratio.
6. The three page policies produce indistinguishable carriers → design is degenerate; the
   experiment cannot resolve anything and must be reported `NULL`, not `PASS`.

---

## 11. ALTERNATIVE MECHANISM (one, materially different, worth testing)

**If G5D is killed, do not close the dictionary lane — close the *paging* axis.** The alternative:

### Retire-and-Rebuild Generational Dictionary (TRGD)

**Base:** one fixed frequency-ordered root per leaf, built once.
**Per page:** instead of a fresh *union* overlay table, transmit a compact **edit script** —
`retire_count`, `insert_count`, the inserted entries, and a retire descriptor — that mutates a
**single rolling decoder index**. The active alphabet is a sliding window of the last `W` distinct
tokens, not `root ∪ ⋃page_overlays`.

**Why this is materially different, not a relabel:**
- G5D's overlay is **monotone**: entries are never retired, so under drift the active alphabet
  grows toward `root_count + 65,536` and the ID width `ceil(log2(active+1))` ratchets **up and never
  back down**. Aging tokens out is precisely the operation G5D structurally cannot express, and it
  is the operation that "dictionary drift economics" actually requires.
- Cost model differs qualitatively: G5D pays `O(page_distinct)` table bytes per page;
  TRGD pays `O(changes)` plus a bounded `active_count ≤ W`. The crossover is explicit and
  computable: TRGD wins whenever `Σ_pages |retire ∪ insert| × (len + uvar) <
  Σ_pages overlay_table_bytes + Σ_pages P·(w_after − w_before)/8`.
- Decode differs: G5D rebuilds a per-page direct-index table at each boundary; TRGD mutates one
  index in place — strictly less per-page work, and it keeps **one** dictionary per leaf resident,
  which directly attacks the §5.1 multi-leaf RSS problem.

**Honest prior-art label:** closest lineage is dictionary-reset block splitting (Brotli/Zstd) and
Parquet dictionary-page rewrite; retirement/deletion is the distinguishing operation. **I claim no
novelty.** TRGD is offered as the correct *next experiment*, not as a novelty claim.

**Gating condition:** TRGD is only worth a remote cycle if the Stage-1 census shows real drift
(`mass_heavy_distinct ≥ 0.05` and `BE_overlay > 0`). Otherwise the honest conclusion is that the
whole exact-dictionary family on this corpus is saturating, and the correct move is arm 5
(`G5D_ESC_BOUNDED_FREQ`) — the bounded-memory, escape-safe, frequency-ordered flat dictionary —
folded into G2 as a decoder-safety improvement with **no ratio claim at all**.

---

## 12. Reconciliation obligations when the Space-Bunny report lands

I will read it and explicitly adjudicate, in an addendum to this file, each of:

1. Whether it credits any gain to **paging** rather than to **frequency ordering / escape / cap**.
2. Whether it reports the **flat-equivalent +2 B/page floor** (my D2) or claims a per-page gain.
3. Whether it acknowledges that the **gate is D3-concentrated** (my D1) and what its D3 projection is.
4. Whether it treats the **three page policies as three distinct arms** without a leaf-occurrence
   census.
5. Whether its numbers are **MEASURED or PROJECTED** — zero G5D corpus bytes exist as of this
   writing.
6. Whether it charges **`L_decoder_memory`** anywhere.
7. Any disagreement on the KILL-PAGING comparison `arm6 − arm5`.

Where my analysis and the constructive lane disagree, the disagreement is resolved by Stage-1
census + Stage-2 arm 5, not by argument.

---

## 13. Final recommendation

> ## HOLD (family) · KILL (current prereg discovery design) · PILOT (one census-first remote ablation)

**KILL the discovery workflow as preregistered.** It cannot produce an interpretable result: four
confounded changes (§3 K1), zero isolating controls, an overlay that is provably inert on all but
the largest leaves (§2 D2), a gate that reduces to 6,924 B on one file (§2 D1), and cap selection
driven by a proxy class with a recorded failure (§3 K4).

**HOLD the family,** because the bounded-root + mandatory-escape dictionary is a real
decoder-resource improvement over G2's unbounded full admission (§7), and because routing bounds
the downside.

**PILOT exactly one remote run:** the two-stage census-gated factorial in §10, with the
`arm6 − arm5` isolated comparison as the primary kill and `KILL-FAMILY` / `KILL-DEGENERATE-DESIGN`
decided on the counting pass alone — before spending a single compression cycle.

**Not PROMOTE-TO-REMOTE.** Not KILL-either. The decisive unknown (D3's per-leaf occurrence and
distinctness distribution) is answerable by counting, and counting is an order of magnitude cheaper
than the compression sweep. Spending the sweep first is the actual waste here.

**Closing note on doctrine:** MASTER-BRIEF binding doctrine 1 requires prior art to be a
first-class constraint. G5D's own §19 already concedes every mechanism it uses. The only honest
route to a project-level contribution from this lane is therefore **not** a new primitive but an
*exact, isolated, fully charged* demonstration that one specific component moves a Pareto axis —
which is precisely what the current control set is designed to prevent.

---
---

# ADDENDUM A — cross-lane reconciliation (2026-10-02, post-interim)

**Trigger:** `01-g5d-dictionary-space-bunny.md` **still absent** (`Test-Path` = False, re-verified).
The constructive lane for track 01 has not reported. So this addendum does **not** reconcile against
it. Instead it reconciles against the two sibling lanes that **have** landed and that both touch
G5D's territory:

- `docs/swarm-2026-10-02/20-priorart-killteam-space-bunny.md` (prior-art kill team)
- `docs/swarm-2026-10-02/15-crossblock-memory-space-bunny.md` (long-range / cross-block memory)

Per the coordinator's standing instruction — *if any redesigned ablation does not isolate root-cap,
overlay-cap, paging, and ordering separately, keep the discovery design killed regardless of
aggregate byte gain* — I have run that isolation test explicitly and mechanically below. It also
produced **one correction to my own §10 design** and **one retraction of my own §11 alternative**.
Both are recorded against my own prior claims, not softened.

---

## A.0 Bottom line of the addendum

| Question | Answer |
|---|---|
| Does the prereg's design isolate **root-cap**? | **NO — 0/5 factors isolated overall** |
| Does it isolate **overlay-cap**? | **NO** |
| Does it isolate **paging**? | **NO** |
| Does it isolate **ordering**? | **NO** |
| Does it isolate **escape**? | **NO** |
| Does my own §10 6-arm design isolate root-cap / overlay-cap? | **NO — my §10 design is also defective on these two; corrected in §A.4** |
| Discovery-design status | **KILLED**, and it stays killed **regardless of any aggregate byte gain** |
| Is my §11 TRGD alternative still defensible? | **NO — RETRACTED.** Superseded by track 15's PIM, which already did the prior-art work. |
| Recommendation change | **Unchanged**: HOLD family · KILL discovery design · PILOT the §A.4 R1–R8 ladder. Now with **stronger** evidence and **fewer** of my own claims. |

---

## A.1 The isolation test, run mechanically

G5D introduces **five** simultaneous changes relative to G3's frozen `EXACT_DICT`. A design isolates
a factor only if two of its emitted arms differ **in that factor and no other**. I checked every
arm the prereg actually emits.

### A.1.1 The complete arm inventory the source emits

From `tools/grotli_g5_paged_dictionary.cpp` (L1465, L1499) and PREREG §11, the discovery run emits
**8 arms**: `RAWLEX_SOURCE_ORDER`, `RAWLEX_SHAPE_COLUMN`, and
`{SOURCE_ORDER, SHAPE_COLUMN} x {P4K, P16K, P64K}` = 6 dictionary arms. Plus external controls
`RAW_BROTLI`, `G2_WHOLE`, `G3_REGION_{RAW,DICT,INT,MIXED}`.

### A.1.2 Factor-by-factor verdict

| # | Factor | Levels the design actually varies | Isolate? | Why |
|---|---|---|---|---|
| **F1** | **Root cap** | none — MDL-selected from 8 levels *per leaf* (PREREG §7.5) | **NO** | No arm forces a cap. Every arm lets the MDL surrogate pick, so cap is a *hidden variable*, never a controlled one. The 8x8 sweep is never q11-arbitrated. |
| **F2** | **Overlay cap** | none — MDL-selected from 8 levels *per page* (§7.5) | **NO** | Same. And per my §2 D2 the MDL optimum is `overlay=0` wherever `L <= P`, so F2 is *definitionally* frozen at zero on exactly the arms where it would matter least. |
| **F3** | **Paging** | 3 levels {4096, 16384, 65536} — but **no single-page arm exists** | **NO** | The contrast that isolates paging is `paged` vs **flat**. Flat is absent from the arm set. The only near-flat configurations are MDL-chosen `overlay=0` outcomes, which are *inferred*, not *controlled*. §11.2's `RAWLEX` arms have **no dictionary at all**, so they confound F3 with F1+F2+F5. |
| **F4** | **Table entry order** (frequency-descending vs first-occurrence) | **not varied anywhere** | **NO** | Critical distinction the prereg invites confusion on: §4's two "orders" are **SOURCE_ORDER vs SHAPE_COLUMN** — the order of *columns in the carrier*, a **different factor** (layout). The **dictionary table entry order** is fixed by §7.2 to frequency-descending and is never contrasted against G3's first-occurrence order (§1.2). The F4 contrast exists only *between G5D and G3*, fully confounded with F1+F2+F3+F5. |
| **F5** | **Mandatory escape** | forced on in every dictionary arm (§7.4: "no no-escape candidate") | **NO** | The prereg **forbids** the escape-free arm, so F5 cannot be varied at all. |

**Score: 0 of 5 factors isolated.** Every one of the six dictionary arms differs from the
`G3_REGION_DICT` control in all five factors simultaneously.

### A.1.3 The two orders do not rescue isolation

A charitable reading: "the two orders give us a controlled pair." They do not, for F4. Both orders
use the **same** frequency-descending table order (§7.2 is order-independent) and the same
dictionary plan and table content (§4 requires it explicitly). So the SOURCE_ORDER/SHAPE_COLUMN pair
is a clean contrast for **layout only** — and §14.4 already forbids crediting it to the dictionary.
It contributes **zero** information about F1–F5.

### A.1.4 Verdict under the coordinator's rule

> The discovery design isolates **0 of 5** factors. Under the standing instruction, it is
> **KILLED regardless of aggregate byte gain**. A 5% aggregate win from such a design is
> **unfalsifiable evidence**: it cannot distinguish "paging works" from "frequency-ordering the ID
> alphabet works" from "the mandatory escape changed which tokens Brotli sees" from "the root cap
> changed the alphabet size". Any of those four would produce the identical 5%.

This is not stylistic. It is the same defect that already produced one recorded failure in this
project: **G4's cheap planner proxy returned `NO-GO-G4` with its speed column withdrawn as
`INVALID_SPEED_ACCOUNTING`** (`RESEARCH_LEDGER.md` L4725–4735 region). G5D re-runs the identical
move — a surrogate makes the internal decisions, only three coarse settings get real arbitration.

---

## A.2 Cross-lane finding #1 — track 20's G5D row is self-contradictory and evidentially empty

`20-priorart-killteam-space-bunny.md` is the lane that maps mechanisms to literature, and it has
**already ruled on G5D**. This is a direct conflict with my interim and the coordinator needs it
flagged:

| Location | Text | Disposition |
|---|---|---|
| L91 (prior-art map, row M-4) | "G5D paged base+overlay exact dictionary — **PROMOTE to remote pilot**; 'claims no primitive novelty'" | **PROMOTE** |
| L215 (map row M-4 detail) | prior art = Parquet column-chunk dictionaries; Brotli static dict; Zstd trained dicts; local/page dicts; FSST; front coding | map only |
| §14.1 dispositions table | "M-1 … **M-4 G5D dictionary**, M-3 planner … — **KILL as claims** (adopt-class / engineering / already closed)" | **KILL** |
| §14.2 final | "Final: **PILOT**" — but the stated rationale is entirely about **track 20's own H2** mechanism and G3's historical passes, *not* G5D | PILOT (not about G5D) |

**Two rulings for the same object in one document.** Reconciliation on the merits is unambiguous:
§14.1 says KILL-as-claim, §14.2's PILOT is about H2, and §14.1's "KILL as claims" is the operative
disposition for M-4. **Track 20 has not cleared G5D for promotion; it has killed G5D as a novelty
claim while noting the engineering may be adoptable.**

Second, and more important: **track 20's L91 "PROMOTE to remote pilot" was derived purely by reading
PREREG §19's own prior-art concession.** It involved no measurement, no arm review, and — critically
— **no inspection of whether the discovery design isolates anything**. So it carries **zero**
evidentiary weight on design validity. Under the standing instruction it cannot revive the design.

**Recommended coordinator action:** strike or annotate L91's "PROMOTE to remote pilot" for M-4 as
superseded by §14.1's KILL. Otherwise a sibling report will be quoted as authorization for a
dispatch its own author contradicts.

---

## A.3 Cross-lane finding #2 — track 15 states a fact about G5D that is false

`15-crossblock-memory-space-bunny.md` M18 (L106) describes G5D as **"a promoted 'paged base+overlay
exact dictionary'"**.

**G5D has not been promoted.** Per `RESEARCH_LEDGER.md` L4912–4924: the prototype compiles
warning-clean, both selftests pass, RSS is now genuinely measured, invariants are computed rather
than hard-coded — and **dispatch is blocked solely on authorized commit/push of the uncommitted
source, prereg, and workflow.** Discovery has never run; **zero G5D corpus bytes exist**. The word
"promoted" is a status error that, if propagated, would let the coordinator treat G5D as settled
evidence rather than an unmeasured hypothesis. Flagging for correction by that lane.

---

## A.4 Correction to my own §10 design — and the design that actually isolates

**Self-criticism first.** My §10 6-arm table isolates F4 (arm3 vs arm4) and F3+F5 jointly (arm6 vs
arm5). It **still fails to isolate F1 (root-cap) and F2 (overlay-cap)**, because arm3, arm5 and arm6
all leave caps to the MDL surrogate. **The coordinator's test would fail my own design too.** I am
recording that rather than quietly patching it.

There is also a structural obstacle nobody has stated: **the five factors are not orthogonally
crossable.** F2 (overlay) is only *defined* when F3 = paged. F5 (escape) is only *meaningful* when
F1 = all-distinct (if the root is capped, escapes are forced by construction, so you cannot hold
escape fixed while varying cap). A naive 2^5 = 32-arm screening would therefore be **half
undefined**. The honest design is a **nested single-factor ladder**, not a factorial.

### A.4.1 Corrected design: the R1–R8 ladder

Run on **D3 only** (per §2 D1, D3 is the only file where F2/F3 can be active, and it is 89.6% of the
aggregate). Every rung differs from the previous by **exactly one factor**. All rungs use the same
parser, frame split, shape identities, raw residuals, token multiset, backend (q11/lgwin30), and are
arbitrated on **real complete q11 bytes** — not MDL.

| Rung | Configuration | Δ from prior rung | Factor isolated |
|---:|---|---|---|
| **R1** | single page; root = **all distinct**; order = **first-occurrence**; **no escape** | — (control) | — |
| **R2** | R1 + table order = **frequency-descending** | F4 | **ORDERING** |
| **R3** | R2 + **mandatory escape** (root still all-distinct) | F5 | **ESCAPE** |
| **R4** | R3 + **root forced to 4,096** (paging still OFF) | F1 | **ROOT-CAP** |
| **R5** | R4 + **paging P=4096, overlay FORCED 0** | F3 | **PAGING** (cost only) |
| **R6** | R5 + **overlay = MDL-selected** | F2 | **OVERLAY-CAP** — *the only rung that can win* |
| **R7** | R5 with P=16384, overlay forced 0 | F3-size | paging-size sensitivity |
| **R8** | R5 with P=65536, overlay forced 0 | F3-size | paging-size sensitivity |

Score against the coordinator's test: **F1 yes (R4−R3), F2 yes (R6−R5), F3 yes (R5−R4), F4 yes
(R2−R1), F5 yes (R3−R2). 5 of 5 isolated.**

### A.4.2 This design also falsifies my own theory — deliberately

My §2 D2 derivation predicts **R5 > R4**: paging with the overlay forced to 0 must cost exactly
`2 x sum_leaves ceil(L_leaf/4096)` bytes of framing, since that is the entire wire difference. I am
exposing this as a falsifiable prediction of my own analysis rather than leaving it as assertion:

- If **R5 > R4** -> my derivation is **confirmed**, and paging's cost is exactly as predicted.
- If **R5 <= R4** -> my derivation is **WRONG**. I have mis-modelled the wire, or Brotli compacts the
  framing away, or `code_width` quantization does something I did not account for. In that case my
  entire §2 cost model is retracted and must be re-derived before any other rung is read.

Either outcome is informative. That is the point of pre-registering it.

### A.4.3 Frozen kill/promote rules for the ladder

All frozen before dispatch:

- `INVALID-LADDER` — any identity, hash, round-trip, or §13 malformed-input failure.
- **`KILL-PAGING`** (primary) — `R6 − R5 >= 0`. The overlay cannot pay for itself against its own
  flat parent. *This is the whole thesis of G5D and it is one subtraction.*
- **`KILL-PAGING-SIZE`** — `R7 − R5 >= 0` **and** `R8 − R5 >= 0`. No page size helps.
- **`RECLASSIFY-TO-G2`** — if `R2 < R1` by >=0.5% on aggregate, the mechanism's value is **table
  ordering**, which is textbook and belongs to G2/G3. G5D's discovery design is then confirmed
  killed, and the frequency-ordering change is extracted as a separate adoptable item against G2.
  **A G5D PASS is impossible in this branch**, regardless of what R6 does.
- **`NO-GO-G5D`** — none of the above trip, but `R6 − R5 > −0.5%` of `R1`, or R6 wins on <2 of the
  leaf populations inspected.
- **`PASS-G5D-PAGING`** — **all** of: (i) `R6 − R5 <= −0.995 x R1`; (ii) `R6 < R5` strictly on >=2
  of >=4 independent heavy-drift leaf populations; (iii) `R2 >= R1` **or** the ordering gain
  separately credited to G2; (iv) `R3 >= R1` **or** escape cost separately credited as a resource
  trade; (v) `L_decoder_memory` charged and within the §15 RSS budget; (vi) all §13 gates pass.
- **Aggregation is explicitly forbidden** as a promotion path. No sum across rungs, files, or orders
  may substitute for any single-factor contrast above.

### A.4.4 Census gate still applies first

The §10 Stage-1 census remains a hard prerequisite. If `BE_overlay(D3) < 6,924 B`, or
`mass_heavy_distinct < 0.05`, or `sum_D3 pages_total < 500`, the ladder **does not run** — the
family is killed on counting alone, with no compression spend. My §10 recommendation stands
unchanged; the ladder replaces only the defective Stage-2 arm list.

---

## A.5 Retraction of my own §11 alternative (TRGD)

**I withdraw TRGD as a "materially different" alternative.** Track 15's PIM has already staked that
territory and has done the prior-art work I had not:

- Track 15 lists as novelty candidate #2: **"a zero-wire deterministic eviction/epoch policy …
  targeting the dictionary-drift weakness G5D names explicitly"**, asserting **"No equivalent found
  in the dedup literature."** That is my TRGD's retirement mechanism — already claimed, already
  literature-searched, and better documented than mine.
- Track 15's comparison table scores PIM's eviction as **"the one thing G5D explicitly lacks"**
  against G5D's "all-or-nothing, no drift story".
- Worse for my claim: track 15 cites **arXiv:1901.02720, *Generalized Deduplication: Bounds,
  Convergence, and Asymptotic Properties*** — chunks expressed as `base + low-Hamming-weight
  deviation`, where **"the decoder needs only the base from a small dictionary and deviations are
  cheap."** That is base+overlay, in published form, with a convergence analysis. It hardens my §4
  prior-art kill of G5D and simultaneously removes whatever separation TRGD might have claimed.

**Revised position on the alternative:** the dictionary-drift question is **not mine to answer**.
It belongs to track 15's PIM, whose novelty separator is already frozen and correctly delegated to
track 20. The only honest contribution I can make here is the negative result that **G5D's own
overlay cannot be the vehicle for it** — because per §2 D2 the overlay is inert on every leaf with
`L <= P`, and per §2 D1 the only file where it could act is D3. **Duplicate staking of the drift
mechanism across tracks 01 and 15 would waste a remote cycle and invite a priority dispute the
coordinator should not have to arbitrate.**

If G5D is killed and the drift direction is wanted, the correct sequence is: track 20 answers PIM's
novelty separator -> track 15 runs PIM -> G5D's paged axis is closed and is not reopened as a
competing vehicle.

---

## A.6 What track 15's evidence strengthens in my interim

One interim claim is upgraded from UNSUPPORTED to MEASURED-ADVERSE, courtesy of track 15's M21
(which cites `RESEARCH_LEDGER.md` L4725–4735): **G4's cheap proxy produced worst-file regrets of
6.6056% / 7.0802% / 1.5388%** when predicting which backend wins. My §9 row previously read the
MDL-selector risk as "UNSUPPORTED — G4's identical proxy class returned `NO-GO-G4`." That was
accurate but under-stated. The measured worst-file regret reaches **7.08%**.

G5D uses that same class of surrogate to make **64 decisions per leaf** (8 root caps x 8 overlay
caps) plus 3 page policies, arbitrating only the page policy on real bytes. Against a discovery
gate of **0.5%**, a surrogate with a measured **7.08% worst-file regret** is being asked to make
every decision the gate actually depends on — a **14x margin deficit**. My §3 K4 is strengthened,
not weakened, by the sibling lane.

---

## A.7 Reconciliation status against the constructive lane

> **RESOLVED — see ADDENDUM B.** This section was accurate when written: at the time of Addendum A
> the constructive report did not exist. It landed at 19:30:14 and **has now been read in full**. It
> read this critic's **interim only** — it contains no reference to Addendum A, to the R1–R8 ladder,
> or to the track-15/track-20 conflicts — so its "where the critic is wrong" analysis addresses a
> superseded version of this report, and its adoption of TRGD as designated follow-on is
> **retracted by Addendum A §A.5**. The checklist below has been applied; results in **§B.3**.

When it lands, adjudication is fully mechanical — the §A.1 isolation test is a pass/fail on its arm
inventory, and the §A.4.1 ladder is the only design that can pass it:

1. Does it emit a **flat, all-distinct, first-occurrence-order, no-escape** arm (R1)? If not, F4 is
   unisolated -> design stays killed.
2. Does it emit a **paged, overlay-forced-0** arm (R5)? If not, F3 unisolated -> killed.
3. Does it emit **forced-cap** arms rather than MDL-selected caps? If not, F1/F2 unisolated -> killed.
4. Does it separate `escape` from `cap`? If not, F5 unisolated -> killed.
5. If it passes all four, the ladder above is *still* required — a correct factor list with the
   wrong cross is not isolation.
6. Whichever way it lands: **aggregate byte gain alone never satisfies the gate.**

I will append the explicit point-by-point reconciliation once the file exists, and I will not soften
the §A.1 verdict to accommodate it.

---

## A.8 Addendum verdict

> **KILL the G5D discovery design — reaffirmed, now on measured grounds (0 of 5 factors isolated),
> and reaffirmed regardless of aggregate byte gain per the coordinator's standing rule.**
>
> **PILOT** the census-first R1–R8 single-factor ladder (§A.4.1) on D3 only.
>
> **HOLD** the bounded-root + mandatory-escape dictionary as adopt-class engineering (§7).
>
> **RETRACT** my own §11 TRGD alternative; the drift/eviction question belongs to track 15's PIM.
>
> **Correct two sibling-lane records:** track 20 L91's unreconciled "PROMOTE to remote pilot" for
> M-4, and track 15 M18's false statement that G5D is "promoted".

Net effect of this addendum on my own position: **fewer claims, stronger verdict.** The two things I
originally proposed as contributions — the 6-arm factorial (partially defective) and the TRGD
alternative (superseded) — are both withdrawn or corrected. What survives is the isolation test, the
census gate, the single-factor ladder, and the derivation that the overlay is inert wherever
occurrences do not exceed the page policy.

---
---

# ADDENDUM B — FINAL point-by-point reconciliation (2026-10-02, post-cross-read)

**Now reconciled.** `01-g5d-dictionary-space-bunny.md` landed at 19:30:14 (46,062 B) and has been
read in full: §§1, 3, 4.1–4.8, 6, 7, 9, 10, 11, 12. The pair **converges**. This addendum records
what I accept, the arithmetic and source errors on **both** sides, whether the redesigned factorial
actually fixes attribution, and the shared ruling.

**Provenance note (material).** The constructive report read this critic's **interim (§0–13) only**.
Verified by absence: it contains no reference to Addendum A, to the R1–R8 ladder, to track 15 or
track 20, to PIM, or to arXiv:1901.02720; it cites "the critic's §10 `mass_heavy_distinct`" and "the
critic's §11 TRGD", both of which Addendum A **superseded or retracted**. So its §10
"where the critic is wrong" analysis is addressed to a stale version of this report, and one of its
conclusions is already dead on arrival (§B.2.6).

---

## B.0 Convergence summary

| Axis | Fledge (interim + Addendum A) | Space-Bunny | Agreed? |
|---|---|---|---|
| Family ruling | HOLD | HOLD | **YES** |
| Discovery design | KILL | KILL | **YES** |
| Next spend | census-first remote PILOT | census-first remote PILOT | **YES** |
| Four-way confound (my K1) | fatal | fatal, conceded §4.8 | **YES** |
| D3 concentration (my D1) | ≥6,924 B, D4 ≤70 B | identical arithmetic §4.7 | **YES** (independently recomputed, §B.4) |
| Overlay inert on single-page leaves (my D2) | inert | inert, and stronger (§3) | **YES** — reason corrected |
| `arm6 − arm5` primary kill | adopt | adopt verbatim | **YES** |
| Bounded-root + escape engineering value | HOLD as adopt-class | HOLD as adopt-class | **YES** |
| Attribution after redesign | — | claims fixed | **PARTIAL — one arm missing, §B.3** |

---

## B.1 Findings I ACCEPT from the constructive report

**A1 — §3, the wire-identity structural fact. ACCEPTED, and it supersedes my stated reason.**
Verified independently against source: `page.code_width = bit_width_u64(active)` with
`active = root_count + overlay_count` (L481); root IDs occupy `[0,r)` and overlay IDs `[r, r+o)` with
ESC at `r+o` (PREREG §7.4); the decoder rebuilds **one** `(offset,len)` table over `[0, a_p)`
(L1010+). Nothing on the wire records which table an ID came from. Therefore on a leaf with
`n_ℓ ≤ P`, **root and overlay are not distinguishable wire entities** and G5D is a flat
frequency-ordered capped dictionary plus escape, one `page_policy_id` byte, and one page
descriptor. This is stronger than my D2 and I adopt it as the canonical statement.

**A2 — §4.6 / §10, refutation of my §5.3 multiplicity error. ACCEPTED IN FULL. I was wrong.**
I wrote that the overlay "pays `table_bytes + 2` to save at most the escape bytes it displaces."
That ignores multiplicity: admitting a token with in-page count `c_i` displaces `c_i` escape
occurrences while costing one table entry. The correct marginal is `(1+|e_i|)(1 − c_i)`, which is
**strictly negative for every token with `c_i ≥ 2`**. So the MDL optimum is *not* `overlay = 0` in
general, and escape fires routinely in the mid-cardinality regime. My characterisation of escape as
a near-pure loss was wrong. **Retracted.**

**A3 — but my D2 conclusion SURVIVES, restated.** The refutation removes my *reason*, not my
*conclusion*, because the operative reason is structural (A1), not economic:

- If the root is unclamped (`D_ℓ ≤ 65,536`) and `n_ℓ ≤ P`, then `overlay = escape = 0` necessarily
  and the cost is exactly `2 B/page`.
- If the root is clamped and `n_ℓ ≤ P`, then cap+escape are live — but those are **arm 4/5**
  territory, not arm 6. The *paging/overlay axis is still absent*, because there is only one page.

Either way paging contributes nothing on a single-page leaf. D2 stands; §5.3 does not.

**A4 — §4.3, the frozen ladder is not the argmin of PREREG §7.5's stated objective. ACCEPTED,
and it is a better finding than my K4.** Verified: `kCapCandidates` at L155 and argmin over that set
at L472/L509–513, against a spec that says "smallest exact serialized G5D leaf cost." The
implementation does not compute the objective the frozen spec names. My K4 concerned a *different*
axis (pre-backend MDL → q11 fidelity); this is a spec/code mismatch, strictly stronger. Accepted as
the primary correctness defect.

**A5 — §4.7, D3 concentration. ACCEPTED; my §2 D1 confirmed.** Arithmetic independently recomputed
in §B.4 — no discrepancy.

**A6 — §4.8, the four-way confound. ACCEPTED; my K1 confirmed**, including his identification of
confound 2's payoff mechanism as my §5.2's `ceil()`-quantization free headroom.

**A7 — §4.5, `L_decoder_memory` + `ΔRSS` + the implicit-length fix. ACCEPTED, and the fix is
better than mine.** Replacing `(offset,len)` pairs with a single `u32` offset array (entries are
concatenated in ID order, so entry `i` spans `[off_i, off_{i+1})`, needing one trailing length) is
4 B/entry instead of 8, one load instead of two, at **zero wire bytes**. That halves my §5.1 working
set for free. I adopt it.

**A8 — §9 census metric set. ACCEPTED as an upgrade on mine** — it carries **both halves** of the
decision (`BE_overlay_exact` = the benefit, `flat_equivalent_floor` = the framing cost) and adds
`R_ladder`. My `mass_heavy_distinct` is correctly retired as insufficient, and his closed-form
`O(D)` evaluation is strictly cheaper than my cap-enumerating formulation. Accepted.

**A9 — §10 obligations 1–6 and §12's downgraded leaning. ACCEPTED.** Notably §10.3: *"My D3
projection is: I do not have one, and I decline to invent one."* That is the correct posture and it
matches my §9. Zero G5D corpus bytes exist; both reports agree.

---

## B.2 Errors found — both directions

### B.2.1 My errors (2) — already stated above

1. **§5.3 multiplicity error** (A2/A3). Real, conceded, retracted.
2. **§10 `mass_heavy_distinct` is a weak decision variable** (A8). Global distinctness cannot
   separate "globally large, locally stable" from "globally large, locally all-distinct." Retired in
   favour of `local_repeat_share` + `BE_overlay_exact`.

### B.2.2 His error 1 — ARITHMETIC. §4.3's blast-radius narrowing is wrong, and it *widens* his own
defect.

He writes: *"`c* = distinct` is always on the ladder for leaves with `D_ℓ ≤ 65536`. For overlays,
page-local distinct is bounded by `P`, so `P = 4096` and `P = 16384` are **always exact**."*

Both claims are **false**. Verified at source: `root_caps.push_back(std::min<uint64_t>(cap,
ranked.size()))` (L429) and `overlay_caps.push_back(std::min<uint64_t>(cap,
local_ranked.size()))` (L465). Clamping yields `min(cap, distinct)` — **not** a ladder point unless
`distinct` happens to equal one of

```
{0, 16, 64, 256, 1024, 4096, 16384, 65536}   = {2^0..2^16, every OTHER power of two}
```

So `D_ℓ = 3,000` clamps to 3,000 (off-ladder); `D_ℓ = 50,000` clamps to 50,000 (off-ladder). The
same holds for page-local distinct: under `P = 4096` a page with 3,000 distinct non-root tokens
clamps to 3,000, which is not a ladder value. **Being bounded by `P` does not help unless `P` is the
bound.**

**Correct statement:** `R_ladder > 0` for the **general case** — any leaf or page whose true argmin
`c*` does not coincide with a ladder point — which is most of them, at all three page policies.
Consequences:

- His "blast radius is bounded and therefore cheap to fix" reassurance is **withdrawn**.
- The **fix remains cheap** (`O(D)` marginal scan replacing `O(64·n·log D)` cap enumeration) — that
  part of his analysis stands.
- **Any G5D number produced before the fix is inadmissible**, because it was produced by a selector
  that does not compute its own stated objective. This must be stated in the ruling schema.
- `REPORT-LADDER-DEFECT` should be promoted from "engineering finding, no gate weight" to a
  **`INVALID-LADDER-PRECONDITION`** on any pre-fix measurement.

### B.2.3 His error 2 — his §4.5 conclusion is likely inverted.

He argues the RSS gate "cannot bind" because "both arms decode a large-window stream whose window is
fixed at `2^30` bits regardless of the dictionary" and a 110% band on "a fixed multi-hundred-MiB
base cannot detect a dictionary regression."

**The project's own measurement refutes his premise, and he cites it two paragraphs earlier:**
`RESEARCH_LEDGER.md` L4879 records `decode_peak_rss_kib: 5416` for a smoke decode — **5.4 MB, not
hundreds of MiB.** Brotli therefore does *not* allocate the full `2^30` window for these inputs.
On a ~5,416 KiB baseline the 110% band is **+542 KiB** of headroom. My §5.1 estimated a **~20 MB**
live dictionary at D3 scale (Σ over leaves of root index + token bytes, with many leaves
simultaneously resident because reconstruction is source-order). On those numbers the gate does not
merely bind — it fails by more than an order of magnitude.

**Status: OPEN, and it must be settled by measurement, not argument.** Neither report has a
D3-scale RSS figure; mine is a projection with an ASSUMED leaf count, his is a projection resting on
a premise the data contradicts. This is now the **largest single unknown risk in the family** and it
is a *resource* risk, which no byte gate can catch. Add a census-time measurement: report
`Σ_leaves (root_index + root_bytes)` and the peak simultaneously-resident set, per file, from
Stage 0 — counting only, no decode required.

### B.2.4 His error 3 — MINOR but important for implementation. §4.2's "exact" rule is described by
a smoothed approximation in the same breath.

His marginal identity is **correct** — I re-derived it:

```
m_c = cost(c) − cost(c−1) = (1+|e_c|)(1 − c_c) + n_p·Δw_c/8
```

and his `Δw_c = 1 ⟺ c is a power of two` is also correct (`w(c) = ceil(log2(c+1))`; checked at
c = 1,2,3,4,8 — increments exactly at powers of two).

But his "first-order reading" — an in-page count threshold near `n_p/(8(1+ℓ̄))`, softened by a
`2^⌊log2 i⌋` band correction, with a worked example of ~1,250 — **does not follow from his own
identity.** From the identity: for any non-power-of-two rank, `Δw_i = 0`, so
`m_i = (1+|e_i|)(1 − c_i)`, which is strictly negative for every `c_i ≥ 2` and exactly zero at
`c_i = 1`. The exact minimiser is therefore

> **`c* = (first rank `i` that is a power of two AND has `c_i = 1`) − 1`**, or `distinct` if no such
> rank exists.

Worked check: `D = 2, n = 100`, counts `(99, 1)`, `|e| = 10`. `m_1 < 0` (admit); `m_2 = 11·0 +
100/8 = 12.5 > 0` (stop) ⇒ `c* = 1`. Direct evaluation: `cost(1) = 11 + 12.5 + 11 = 34.5`;
`cost(2) = 22 + 25 = 47`. Confirmed.

There is **no count threshold** in the exact rule; `c*` is governed by the position of the first
singleton sitting on a power-of-two rank. His ~1,250 example cannot be produced by his own formula.

**Why this matters:** he proposes replacing a spec-violating surrogate with an "exact" rule. If
anyone implements from the prose instead of the marginal identity, they reintroduce **exactly the
defect he is correcting** — a smoothed surrogate standing in for an exact objective. The label
"first-order" is a mitigation; the worked example reads as operational. His §11 prototype must
therefore be specified against `m_c` directly, and the prose threshold must be struck or explicitly
marked non-normative.

### B.2.5 Not an error, but a note on his §9.1 `local_repeat_share`.

`local_repeat_share = (Σ_p n_p where D_p < n_p) / total_occurrences` is **near-vacuous as a kill
gate**: any page containing a single repeat qualifies, so it will pass essentially every population.
It is a necessary-not-sufficient sanity check, not a decision variable. The binding gate is
`BE_overlay_exact(D3) ≥ 6,924 B`. Recommend demoting `local_repeat_share` to diagnostic, or
strengthening it to *the share of occurrences in pages where `BE_overlay_exact_p > 0`*, which is the
statistic that actually tracks the mechanism. (His `mass_heavy < 0.02` loosening from my 0.05 is
internally consistent — his predicate `n_ℓ > 4096` is broader than my `n_ℓ>4096 ∧ D_ℓ>4096`. Accept.)

### B.2.6 A conclusion of his that is dead on arrival.

His §10 bullet: *"§11 TRGD is the correct successor if drift is real. I adopt it as the designated
follow-on."* **Superseded by Addendum A §A.5, which he did not read.** TRGD is **retracted**:
track 15's PIM has already claimed the eviction/drift mechanism and did the literature work
(arXiv:1901.02720, *Generalized Deduplication*, expresses chunks as `base + low-Hamming-weight
deviation` with a small base dictionary — base+overlay, published). His own addition stands and is
good: **TRGD's crossover must be computed against arm 5, not arm 2**, or it inherits the very
four-way confound being killed. But the drift question belongs to track 15's PIM, sequenced after
track 20 answers PIM's novelty separator. Do not execute TRGD from track 01.

---

## B.3 Does the redesigned factorial actually fix attribution? — PARTIAL. One arm missing.

Applied to his §9.2 arm set against the four factors the coordinator named:

| Factor | Contrast | Isolated? |
|---|---|---|
| **Ordering** (freq vs first-occ) | `arm3 − arm2` | **YES** |
| **Root cap** | `arm4 − arm3` — varies cap **and** escape together | **NO** — cap never varies alone |
| **Overlay cap** | `arm6 − arm5` — varies paging **and** overlay together | **NO** |
| **Paging** | `arm6 − arm5` — same | **NO** |
| *(bonus)* ladder vs exact admission | `arm5 − arm4` | **YES** — a real isolation his version lacked |

**3 of 4 named factors remain unisolated, plus the overlay/paging pair is the one that actually
carries the thesis.** `arm6 − arm5` is a *block* contrast: {pages + overlay} vs {flat}. It answers
"does the paged apparatus pay for itself?" — a legitimate and important question, and the right
primary kill. It does **not** answer the question the mechanism was named for.

**Required addition — one arm:** `G5D_P4K_OVERLAY0` = paged at P=4096 with **overlay forced to 0**
(Addendum A's R5). It completes the design with one extra subtraction:

```
overlay value   = (arm6 − arm5_OVERLAY0)     ← the only term that can be positive
paging cost     = (arm5_OVERLAY0 − arm5)     ← predicted ≈ +2·pages_total by my §5.4
```

Without it the two cannot be separated, and the outcomes imply **opposite** follow-ons:

- `arm6 − arm5 ≥ 0` with `arm6 − arm5_OVERLAY0 < 0` ⇒ **the overlay works, paging framing kills
  it.** Action: raise the page policy / amortize framing. Not a mechanism kill.
- `arm6 − arm5_OVERLAY0 ≥ 0` ⇒ **the overlay never pays for itself.** Action: close the overlay
  axis. Mechanism kill.

His own §3 makes this arm mandatory rather than optional: on a corpus where most leaves are
single-page, `arm6 − arm5` is dominated **by construction** by framing overhead, because the
overlay contributes nothing there. So without the split, the primary kill will most likely trip for
a trivial reason and the experiment will return an uninformative NO-GO. His Stage-0 census already
computes both halves (`BE_overlay_exact` and `flat_equivalent_floor`) — only the arm is missing.

**Sequencing constraint (his §12, confirmed and extended).** Adding arms 3–5, plus this arm, plus
the §4.3 selector fix, all require editing `tools/grotli_g5_paged_dictionary.cpp`, which neither
lane may modify. All four must land in **one authorized commit by the owning lane**, or the arms
will be measured against a selector that violates its own spec (see B.2.2).

**Highest-leverage unblock for the coordinator.** His §12 correctly notes dispatch is blocked on
user authorization and that "Stage 0 does not unblock it." Worth stating plainly: **the cheapest
decisive experiment in this entire track is also authorization-blocked.** Stage 0 is counting-only —
no compression, no timing, no benchmark — yet it requires corpus access and therefore the same
authorization as the full sweep. It could return `KILL-PAGING` outright and close the family for
the cost of one CI job. That is the highest ratio of information to authorization in track 01, and
it is currently unfunded.

---

## B.4 Arithmetic verification (recomputed independently, no discrepancy)

From `docs/I10-GROTLI-G3-RESULTS.md` §1:

```
Σ G3 routed selected  = 39,277 + 20,861 + 1,240,155 + 84,361 = 1,384,654
Σ G3_REGION_DICT      = 39,277 + 20,861 + 1,240,155 + 84,431 = 1,384,724
Σ raw Brotli          = 40,126 +  25,029 + 1,292,757 + 108,857 = 1,466,769

§14.2:  0.005 × 1,384,654 = 6,923.27  ->  saving >= 6,924 B
§14.3:  0.005 × 1,384,724 = 6,923.62  ->  saving >= 6,924 B
D4 headroom = 84,431 − 84,361 = 70 B   (G2_WHOLE is permanent, wins ties)
D3 share of routed = 1,240,155 / 1,384,654 = 89.56%
Required from D3 ≈ 6,924 − 70 ≈ 6,854 B = 0.5526% of D3
```

**Both reports' §4.7 / §D1 figures match this exactly.** No arithmetic error found on either side
in the gate arithmetic. (My interim phrasing "≥ ~6,000 B must come from D3" was a slightly loose
restatement of 6,854 B; the exact figure is 6,854 B, and it makes the conclusion marginally
*stronger*, not weaker.)

---

## B.5 Final shared ruling

> ## HOLD (family) · KILL (G5D discovery design as preregistered) · PILOT (one census-first,
> attribution-fixed, remote-only ablation) · NOT PROMOTE-TO-REMOTE

**Both reports reach this independently**, from opposite directions: I from the isolation defect and
the D3-concentration arithmetic; he from the four-way confound, the D3 gate, and the policy
degeneracy. His §12 explicitly downgrades his own prior leaning toward dispatching as preregistered.

**HOLD the family** — bounded `root_count ≤ 65,536` + mandatory escape converts G2/G3's unbounded,
cardinality-driven decoder table into a bounded allocation with a deterministic decode-time ceiling.
Real, ratio-agnostic, serves track 19, survives a paging kill intact. Adopt-class; no novelty claim.

**KILL the discovery design** — 0 of 5 factors isolated (my §A.1), plus the §4.3 spec/code
mismatch. Not the family. Cheap to reverse: PREREG §7.5 (exact selector objective), §11 (add arms
3–5 **plus `G5D_P4K_OVERLAY0`**), §9 (add `L_decoder_memory`), §14 (replace the single threshold
with the single-factor credit rule), §15 (charge `ΔRSS`, not absolute RSS). All narrowings or
additions; no new leaf family; no threshold chosen after outcomes are visible.

**PILOT one remote dispatch:** Stage 0 counting-only census → if it does not trip `KILL-PAGING` /
`KILL-DEGENERATE-DESIGN`, Stage 1's factorial with `arm6 − arm5` as primary kill, **extended by the
one missing overlay-forced-0 arm**, all rungs arbitrated on real q11 bytes with no MDL-selected
factor, and aggregation forbidden as a promotion path.

**The one remaining technical disagreement (§B.2.4).** Whether the exact overlay admission rule is
the count-threshold law of his §4.2 prose or the power-of-two-singleton rule implied by his own
marginal identity. I hold the latter; the identity is verified and the threshold is not derivable
from it. This must be settled **before** his §11 prototype is written, because implementing from
the prose would reintroduce the very surrogate-for-exact defect he is correcting. It does not
change any gate, threshold, or the shared verdict — it changes one function body.

---

## B.6 What neither report has, and should not claim

- **Zero G5D corpus bytes exist.** Both reports agree and both label every G5D number
  PROJECTED/DERIVED. Correct. Any downstream document quoting a G5D ratio number is fabricating.
- **No D3 leaf-occurrence or distinctness distribution is known.** This is the single largest
  evidence gap, and it is why Stage 0 must run before Stage 1.
- **No D3-scale RSS measurement exists.** Per §B.2.3 the RSS risk is open and possibly the largest
  one in the family. Add a Stage-0 counting-time memory estimate.
- **Sibling-lane records still need correction:** track 20 L91's unreconciled "PROMOTE to remote
  pilot" for M-4 (contradicted by its own §14.1 KILL), and track 15 M18's false statement that G5D
  is "promoted" (it is unmeasured and authorization-blocked).

---

## B.7 Disposition of my own claims (final ledger)

| Claim | Disposition |
|---|---|
| §D1 D3 concentration / 6,924 B gate | **CONFIRMED**, independently recomputed (§B.4) |
| §D2 overlay inert on single-page leaves | **CONFIRMED, reason upgraded** (§3 wire-identity) |
| §D3 occurrence/distinct anti-correlation | **CONFIRMED** as the central risk |
| §K1 four-way confound | **CONFIRMED** (his §4.8) |
| §K2 prior-art collapse | **CONFIRMED and hardened** (arXiv:1901.02720) |
| §K4 MDL-selector risk | **CONFIRMED and escalated** to a spec/code mismatch (§4.3) |
| §K5 uncharged decoder memory | **CONFIRMED**, fix improved (A7), RSS risk **inverted and enlarged** (§B.2.3) |
| §5.3 escape economics | **RETRACTED** — multiplicity error (A2) |
| §10 `mass_heavy_distinct` | **RETIRED** — superseded (A8) |
| §A.4 R1–R8 ladder | **SUPERSEDED** by his §9.2 arm set **plus the one missing arm** (§B.3) |
| §A.5 TRGD alternative | **RETRACTED** — belongs to track 15's PIM (§B.2.6) |
| §7 bounded-memory engineering value | **CONFIRMED** and independently adopted by him |

Net across both reports and two addenda: **eleven claims confirmed, three retracted by me, three
errors found in the constructive lane, one shared verdict reached independently.** The verdict did
not change. The reasons got sharper, and two of my own contributions did not survive contact with
the constructive lane's better analysis — which is the correct outcome for an adversarial review
that was actually adversarial.
