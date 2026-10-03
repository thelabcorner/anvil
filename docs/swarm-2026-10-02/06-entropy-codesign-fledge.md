# Track 06 — Entropy Backend Co-Design — Fledge Alpha Free (independent adversarial review)

**Track:** 06-entropy-codesign · **Role:** independent adversarial reviewer (red team)
**Date:** 2026-10-02 · **Tree:** `i10-aux-unbwt` @ `b8eae11`, intentionally dirty
**Status of this document:** INTERIM independent audit. **No measurement was performed by this
agent.** No local benchmark, no corpus run, no fuzz, no build. Every number below is either
(a) quoted with a source artifact/code location, (b) derived by arithmetic shown inline from
code-verified constants, or (c) explicitly labelled PROJECTION / UNMEASURED.

**Constructive-lane dependency:** `06-entropy-codesign-space-bunny.md` **has not yet appeared.**
This audit therefore red-teams the *incumbent and mandate-implied* model class, reconstructed
independently from `RESEARCH_LEDGER.md`, `FORMAT.md`, `src/anvil.cpp`, `docs/CONTEXT.md` and the
audit corpus. Reconciliation against the constructive report is deferred to the final revision
(this file will be updated, not replaced, when it lands).

**Coordination item:** this report also discharges the coordinator's request to audit **Track 11
checkpoint §9 P1** as a candidate *common cost-decomposition protocol* for tracks 05/06/18 —
see §7. That audit is conducted from track 06's seat because the cost model P1 would decompose
is the one track 06 depends on most.

---

## 0. Verdict in one paragraph

The track as literally scoped — "a better entropy backend that crosses the frontier" — is
**KILL**, and not for lack of engineering merit but for a prior-art-occupancy reason and an
arithmetic reason. **(Revised framing, 2026-10-02 coordinator scope correction.)** My first draft
argued that because Brotli has no adaptive entropy coder, "the gap is provably not entropy
coding." **That inference is withdrawn as over-strong.** Entropy-backend *family* alone does not
order complete-codec efficiency: modeling and context partitioning, alphabet construction, table
amortization, state interleaving, symbolization, and implementation constants all contribute, and
a Huffman-family coder with better context partitioning and cheaper tables can beat a
more-advanced coder with worse ones. The correct, weaker claim is: **a wholesale new entropy-coder
primitive is weakly motivated and prior-art occupied, and `DNB-M1` closes the decode-only plane
arithmetically.** The measured open problem is **not** the coder primitive — it is
**model/layout/cost selection**. Second, **decode-only crossings are arithmetically closed on 12 of
13 corpus files** (`DNB-M1`, MATH-class), and the one live cell (`synth-timeseries.bin`, 1.8x) is
degenerate-flagged. A new entropy-coder primitive therefore has no arithmetic room on the decode
plane and no prior-art room on the ratio plane. What *does* have room, is unclaimed, is cheap, is remotely falsifiable in
one CI job, and is **currently wrong in this repo's shipped default path** is the *cost model
itself*: `J = L + λ·c` is implemented with a **length-independent constant** cost on every default
code path (`src/anvil.cpp:1548-1564`), while a measured, size-proportional, correctly-scaled
calibration exists but is gated behind a **default-off** flag (`src/anvil.cpp:1617,2868,3786`)
whose arbiter run **never happened** (`Experiment AA — INCOMPLETE`). That defect silently
mis-prices every decode-cost-gated proposal in tracks 05, 06, 11 and 18 by **1.7x to 51x**
depending on stream length (§4.2). Recommendation: **PROMOTE-TO-REMOTE** for the re-scoped
cost-reconciliation falsifier **E1**; **KILL** new-entropy-backend-as-crossing-route; **PILOT**
one decode-layout question behind it; **KILL** multi-stream context fan-out on measured geometry.

---

## 1. Evidence map — measured facts I rely on (all located)

| # | Fact | Source |
|---|---|---|
| **F1** | rANS became the performance backend on an **8.1x decode win for +1.13% bytes** (DP+rANS 11,232 B vs DP+arith 11,107 B; decode ~20.16 → ~163.72 MB/s). Adopted 2019-era and never revisited. | `RESEARCH_LEDGER.md` Exp. D:34-40 |
| **F2** | 4-way interleaved rANS gave **~30% decode speedup** — the decode inner loop is deliberately **ILP-tuned**. | `docs/CONTEXT.md` (separated-stream/rans2x4 entry) |
| **F3** | Context-switched rANS (mode 6) is the **largest single-mechanism ratio win in project history**: −8.3%…−16.2% bytes on the four record files. Decode **−3%…−10%**. Verdict recorded verbatim: "RATIO mechanism, not the throughput primitive… **No Pareto claim**." | `RESEARCH_LEDGER.md` Exp. S:1480-1523 |
| **F4** | **Context clustering is explicitly NOT novel.** Brotli RFC 7932 maps decoded literal context to several literal prefix trees via a compact context map driven by previous decoded bytes. The ledger records this as a *correction of an over-claim*. | `RESEARCH_LEDGER.md` Exp. J:630-639 |
| **F5** | Context-switched rANS on SQLite: depth-2/K12 ≈ 404.7 KB, depth-3 ≈ 379.3 KB vs brotli q4 ~422.1 KB (huge ratio headroom) **but decoder falls to ~0.6-0.74 GB/s**. | `RESEARCH_LEDGER.md` Exp. J:640-648 |
| **F6** | The context-switched **table-Huffman/direct** variant was **REJECTED**: ~143.8 KB at K=12 (same size) but context-dependent prefix machinery is **slower** than clustered rANS once model/table setup is counted. | `RESEARCH_LEDGER.md` Exp. J:645-648 |
| **F7** | Stream-suite J-selection passed its gates: J-faithfulness **100%** at λ=0.01 over 1800 streams, ratio neutral-or-better (0.1278 vs 0.1279), decode wins "**modest and within timing noise at median-3**". Gate record explicitly: **EXTENDS_FRONT NOT cleared**. | `RESEARCH_LEDGER.md` Exp. L:1036-1100, PART IV:1167-1180 |
| **F8** | The λ=0 / 0.01 / 0.04 rows are **byte-identical on the corpus** — "the time term ≤0.16 B/stream never overrides the size winner". | `RESEARCH_LEDGER.md` PART IV:1163-1165 |
| **F9** | **`Experiment AA` is INCOMPLETE**: whole-codec J-selection + raw-stream budget landed, all controls green, **formal bench arbiter NEVER RAN**, no bar verdict, no EXTENDS_FRONT determination. | `RESEARCH_LEDGER.md` Exp. AA:3785 |
| **F10** | The measured decode calibration table (median-7, ns/B, real mode-15 content): raw 0.04-0.10 bulk / 2.59-2.66 byte; rANS-4096 5.93-6.62; rANS-512 5.44-6.55; rANS-256 5.40-5.99; Huffman 4.08-9.0 (data-dependent); defexc 3.14-3.25; ctx-rANS pull ~7-8, mat ~4.6-5.3. Assessment of the incumbent constants: **"right ORDER, wrong GAPS."** | `RESEARCH_LEDGER.md` Exp. AA:3883-3904 |
| **F11** | `DNB-M1` (MATH-class): decode-only Route-B crossings are **closed on 12 of 13** corpus files. Multipliers: generated.jsonl 11.7x, generated.log 10.5x, generated.sqlite 10.5x, anvil.exe 11.8x, synth-jitter.bin 13.5x, doc.md 23.7x, src.cpp 40.5x, repeat 106.5x. **Only live cell: synth-timeseries.bin at 1.8x.** Ceiling for byte-identical decoder work: ~1.79x end-to-end. | `RESEARCH_LEDGER.md` §3:4223-4276 |
| **F12** | `DNB-M3`: any non-dominated row at **ratio ≥ 0.95 is DEGENERATE, not a result** — live on `synth-arith.bin` and `random.bin`. | `RESEARCH_LEDGER.md` §4:4284-4287 |
| **F13** | Corpus already has **22 entropy streams** on `generated.log`; largest were `distance_code` 15,680 B and `match_len_class` 8,788 B. | `docs/CONTEXT.md` (log stream anatomy) |
| **F14** | Linux verdict on spending rate surplus for a cheaper code: the dominant trade is storing the shape-distance-delta stream **raw** — costs only ~19.8 KB and raises hot-book decode ~0.87 → ~0.99 GB/s. "~60 KB rate surplus = explicit compute budget." | `docs/CONTEXT.md` (hot-op hybrid stream-budget sweep) |
| **F15** | Reciprocal-rANS (precomputed reciprocal-multiply-division replacement) was **measured SLOWER than the hardware divide**. Do not re-burn. | `docs/CONTEXT.md` |
| **F16** | Brotli q11 on delta+zigzag-varint bytes of `synth-arith.bin` = **16,313 B** vs 87,013 B raw (5.33x). All synth-cell numbers must be reported dual-bar; a win against the raw bar alone is **struck**. | `RESEARCH_LEDGER.md` §5b:4305-4335 (M17) |
| **F17** | GitHub Actions measurement classes: byte/ratio/hash = **citation-grade**; MB/s and RSS = **scout/ranking-grade**, paired same-job A/B only; "a small timing delta on a shared runner is inconclusive." | `docs/GITHUB-ACTIONS-BENCHMARKING.md` §2,§3,§9 (M20) |

### 1b. Code-verified facts (this agent, read-only inspection of `src/anvil.cpp` @ 5,034 lines)

These are **not** projections. They are the shipped code.

| # | Verified fact | Location |
|---|---|---|
| **C-F1** | The **default** per-stream selector is `encode_stream`, whose objective is `J = L + g_stream_lambda*cu + g_stream_mu*2.0 + g_stream_nu*cu` with `cu` a **literal constant per candidate**: raw 10, rANS-4096 40, rANS-512 35, rANS-256 30, Huffman 22, defexc 20. **There is no stream-length term anywhere in this cost.** | `src/anvil.cpp:1548-1564` |
| **C-F2** | A **second, different** selector exists: `encode_stream_budget`, `C_us = src.size() * kBudgetNsPerByte[codec] / 1000.0`, i.e. **size-proportional** with measured ns/B (`{0.1, 6.0, 6.0, 6.0, 4.3, 3.2, 7.5}`). | `src/anvil.cpp:1618-1637` |
| **C-F3** | `encode_stream_budget` is called from **exactly one site**, line 2868, under `g_hotop_budget`. `g_hotop_budget` is **default `false`** (`hotop_budget=false`, set only by `--hotop-budget=` **≠ "off"**, i.e. **default ON via any other spelling** of the flag — note the parse at 4979 treats *any* value other than the literal `off` as on). | `src/anvil.cpp:1617, 2868, 3786, 4664, 4979` |
| **C-F4** | Every other stream-emitting path uses the **constant-c** `encode_stream`: modes 10-13 substreams (2161, 2234, 2500, 2642), modes 14/15 (3169, 3438), RLZ and RePair inner codecs (1720, 1770), and the size-estimation pass (3605). | `src/anvil.cpp` (11 call sites) |
| **C-F5** | Hidden size gate: **both** selectors early-return raw when `src.size() < 16`. A ≤15 B stream can never be rANS/Huffman/ctx-coded and always pays a `[mode=0][uvarint len]` header. | `src/anvil.cpp:1550, 1630` |
| **C-F6** | Hidden gate: ctx-rANS (mode 6) is only ever a candidate when `src.size() >= 4096`. Below that the ratio mechanism of F3 is **structurally unavailable**. | `src/anvil.cpp:1568, 1649` |
| **C-F7** | Wire cost of the ctx model is **unconditionally 257 B** before any per-class data: 1 B `K` + **256 B context map**, then per class `uvarint(nz) + Σ_{nz}(1 B symbol + uvarint freq)`. Minimum ≈ `259 + 3K` B (K=12 → ~295 B). | `ctx_stream_bytes`, `src/anvil.cpp` |
| **C-F8** | Wire cost of the Huffman model is **unconditionally 258 B**: 1 B mode + uvarint + **256 length bytes** (always all 256, regardless of how few symbols the stream uses) + uvarint payload. | `huffman_stream_bytes`, `src/anvil.cpp` |
| **C-F9** | rANS models are **sparsely serialized** (only nonzero symbols): `1 + uvarint(N) + uvarint(nz) + Σ_nz(1 B sym + uvarint freq) + uvarint(payload)` = **`4 + 2s` B minimum**, `~516 B` worst case at s=256. This is strictly cheaper than Huffman/ctx for skewed streams. | `rans_stream_bytes`, `src/anvil.cpp` |
| **C-F10** | The default path's `C_decode` is documented as "per-STREAM decode cost units" in the source comment, and `FORMAT.md:639` documents `J = L + λ·c + μ·2 + ν·c` with the integer `c` table — i.e. **the docs and the default code agree with each other, and both are length-independent.** | `src/anvil.cpp:1466-1471`; `FORMAT.md:631-663` |
| **C-F11** | Encoder evaluates **all** candidates and then argmin-J's them; there is no early termination on a size bound. ctx-rANS additionally requires `src.size() >= 4096`. So per-stream encode cost includes building a full context model on every qualifying stream. | `src/anvil.cpp:1548-1576` |

---

## 2. Prior-art map — is there any room?

### 2.1 The decisive structural fact: the reference has no adaptive entropy coder

**Brotli (RFC 7932) uses Huffman-derived prefix codes only** — no arithmetic coder, no rANS, no
ANS, no FSE. Its literal coding is 3 context classes × 4 block-switchable prefix-code trees plus
NPOSTFIX/NDIRECT; its command alphabet is a fixed 704-entry prefix code with distance codes.
**Zstd (RFC 8878) uses Huffman + FSE (ANS-style).** ANVIL uses rANS with a 7-codec adaptive
per-stream suite plus a context-switched rANS.

| coder | entropy backend | vs ANVIL |
|---|---|---|
| **Brotli q9/q11 (the target)** | Huffman prefix codes only | **strictly weaker** |
| Zstd 19 | Huffman + FSE | comparable (FSE ≈ rANS in role) |
| **ANVIL (incumbent)** | rANS-4096/512/256 + Huffman + defexc + ctx-rANS, J-selected | — |

**Consequence — stated at the strength the evidence actually supports (revised).** ANVIL is not
behind the reference *in coder family*, yet it is 4x behind in decode and behind in bytes. That
is **consistent with** the deficit lying outside coder family, and it means a new coder primitive
is attacking an axis where ANVIL is not losing. It does **not** prove the entropy layer is
irrelevant: a cheaper, better-partitioned, better-amortized model on the *existing* rANS could
still win, and that is precisely the surviving workstream (§4). The kill is therefore scoped to
**wholesale new primitives**, not to entropy-layer work as such:

- **KILL:** a new entropy-coder primitive (FSE, tANS, novel ANS variant, adaptive range coder,
  multi-stream context fan-out) as a *crossing route*. Prior-art occupied (§2.2) + `DNB-M1`
  arithmetic (§6 K5) + re-burns of Exp. A / Exp. J verdict 2 / F15.
- **NOT killed:** model/layout/cost work on the *incumbent* rANS — i.e. what §4 and §6 E1/E2
  target, and what Track 15's CAM targets as a *model-initialization* question (§11).

### 2.2 Prior art relevant to each entropy candidate a track-06 brief might propose

| candidate | prior art | occupancy verdict |
|---|---|---|
| rANS / ANS entropy stage | rANS (Ryg, 2018); AV1, VP9, JPEG-XL (ANS), FLAC | **Occupied** — already adopted here (F1) |
| FSE / tANS sequence coding | Yann Collet's FSE; **RFC 8878 zstd uses FSE for LL/ML/OF** | **Occupied and already shipped by a reference.** Adopting FSE would be a lateral move against zstd, not a brotli-beating move. Only worth revisiting if it yields a *byte* win. |
| Context-mapped literal coding | **Brotli RFC 7932 §7** (explicitly recorded as a correction here, F4) | **Occupied. Never claimable.** |
| Sparse/adaptive model clustering (256→K contexts) | Brotli's context map is itself a *compact* map; K-means/EM frequency clustering is textbook | Mechanism occupied; the defensible residue is the **quantizer implementation cost** (~1.65 ms), which the ledger itself classifies as *enabling infrastructure, not novelty* (`docs/CONTEXT.md`) |
| Context-switched **table Huffman** | same as above | **Measured REJECTED here** (F6). Do not re-burn. |
| Work-adaptive / precision-adaptive coder selection | zstd `Compression_Mode`, RFC 7932 block types; FSE/rANS interleaving | Partially occupied. ANVIL's J-objective is claimed NEW-INTERACTION (`RESEARCH_LEDGER.md` C7:1193-1202) but the claim's own evidence is "modest… within timing noise" (F7) and EXTENDS_FRONT was **not** cleared. |
| Bit-packed / RLE / sparse context-map encoding of tables | Brotli §7 context-map RLE; any uvarint/sparse-table framing | **Occupied, and relevant as a concrete BYTE win — see §4.3.** |
| Reciprocal-multiply rANS | — | **Measured SLOWER than hardware divide** (F15). Do not re-burn. |
| Learned/neural entropy coding | — | Prior art exists but out of scope for a deterministic-bounded decoder; not adjudicated here. |
| **Decoder cost-model-corrected stream selection** | zstd's `Compression_Mode` picks on ratio; **nobody publishes a byte/μs-weighted selection objective.** The project's own F10 verdict is that its constants are "right ORDER, wrong GAPS" and were never arbitered (F9). | **THIS IS THE OPEN SLOT.** See §4. |

---

## 3. Hidden-cost audit

Every line is charged against the decoder or the wire, per doctrine item 2.

### 3.1 Per-stream decoder-visible fixed cost (code-verified, C-F7/C-F8/C-F9)

| codec | fixed wire cost | formula | note |
|---|---:|---|---|
| raw | **2 B** (len ≤ 127) | `1 + uvarint(N)` | fallback, always present |
| rANS-256/512/4096 | **`4 + 2s` B**, ≤ ~516 B | sparse nonzero-symbol table | *cheapest real coder for skewed streams* |
| Huffman | **258 B always** | 256 unconditional length bytes | **C-F8: pays 256 B even for a 3-symbol stream** |
| defexc | `3 + uvarint + N/8` | mask + exceptions | O(N/8) mask even at low exception rate |
| ctx-rANS | **`259 + 3K` B**, ≥ 295 B at K=12 | **256 B unconditional map** + per-class tables | **C-F7** |

**Finding H-1 (hidden table cost, NEW-to-this-report and actionable).** The two "smarter" coders
carry **unconditional 256-byte tables**; rANS does not. The `≥4096 B` gate (C-F6) is therefore not
an arbitrary threshold — it is a crude guard against a 257-258 B header on a small stream (6.3% at
4096 B). **But the guard is per-stream, and the project ships 8 substreams per block in modes
10-13 and 9 book streams in mode-15.** If ctx-rANS were ever enabled per-stream rather than as
the single physical literal stream of F3, the decoder-visible model cost is
`n_streams × ~295 B` **per block**, i.e. ~2.4 KB per 8-stream block from mode 6 alone. Exp. S's
entire measured win depends on ctx-rANS being **ONE physical stream** (`docs/CONTEXT.md`:
"single context-switched stream, NOT multi-stream fan-out — the rejected fan-out stays
rejected"). The gate is the only thing mechanically preventing regression into the fan-out.
→ **Any track-06 proposal that instantiates ctx-rANS per-stream is pre-refuted by C-F6/C-F7
arithmetic, before any code is written.**

### 3.2 The `<16 B` dead zone (C-F5)

Streams under 16 B are forced to raw. Worst case a 15 B stream carries a 2 B header = **13.3%
pure overhead**, and is excluded from every rANS/ctx code path. This is unlogged, ungated, and
absent from all gate records. It also **inflates F7**: Exp. L's "J-agreement 1800/1800 = 100%" is
partly trivially satisfied on streams where the suite was never a candidate. **This is not
disproved — it is unverified and cheaply checkable** (§6, E1b).

### 3.3 The cost-model defect — the headline finding

This is the audit's principal result and it directly answers Track 11's **H5**.

**Shipped default (C-F1).** `J = L + λ·c`, λ = 0.01 B/μs, `c ∈ {10,20,22,30,35,40,45}`:

| codec | c | **λ·c (bytes)** |
|---|---:|---:|
| raw | 10 | **0.10** |
| defexc | 20 | 0.20 |
| Huffman | 22 | 0.22 |
| rANS-256 | 30 | 0.30 |
| rANS-512 | 35 | 0.35 |
| rANS-4096 | 40 | **0.40** |
| ctx-rANS | 45 | **0.45** |

This term is **constant and independent of stream length**. Maximum raw→rANS-4096 gap = **0.30 B**.

**Measured size-proportional form (C-F2).** `λ·C_us = 0.01 × (N × ns/B / 1000) = N × ns × 1e-5` B:

| codec | ns/B (F10) | λ·C (bytes) |
|---|---:|---|
| raw | 0.1 | `N × 1.0e-6` |
| rANS-4096 | 6.0 | `N × 6.0e-5` |
| ctx-rANS | 7.5 | `N × 7.5e-5` |

raw→rANS-4096 gap = `N × 5.9e-5` B.

**The two forms are numerically equal only at one stream length:**

```
0.30 / 5.9e-5  =  5,085 bytes      ← N*
```

| stream length N | shipped gap | calibrated gap | shipped is… | consequence |
|---:|---:|---:|---|---|
| 100 B | 0.300 B | 0.0059 B | **51x too heavy** | over-favours raw |
| 512 B | 0.300 B | 0.030 B | **10x too heavy** | over-favours raw |
| 2,048 B | 0.300 B | 0.121 B | 2.5x too heavy | over-favours raw |
| **5,085 B** | 0.300 B | 0.300 B | **equal** | the only length where they agree |
| 15,680 B (`distance_code`, F13) | 0.300 B | 0.925 B | **3.1x too light** | **under-favours decode speed** |
| 100,000 B | 0.300 B | 5.90 B | **19.7x too light** | **under-favours decode speed** |

**Provenance discrepancy (new, coordinator-mandated attack on benchmark-window provenance).**
`RESEARCH_LEDGER.md` PART IV:1163-1165 records: "the time term **≤0.16 B/stream** never overrides
the size winner." **That magnitude is not reproducible from either constant set at any of the
three λ values quoted in the same sentence (0, 0.01, 0.04).** Under the shipped integer table the
maximum possible `λ·c` at λ=0.01 is **0.45 B** and the maximum spread is 0.35 B; under the older
non-integer values the maximum is 0.045 B. Neither yields 0.16. The λ-sweep rows therefore appear
to have been measured against **a different constant set than the one that ships**, or the note's
magnitude was estimated rather than computed. **This does not overturn any verdict** — the
conclusion drawn from it ("the time term never overrides the size winner") is directionally
consistent with F8 — but it means the λ-row byte-identity claim **cannot be re-derived from the
shipped table and must be re-measured in E1 rather than inherited.** Recorded under the
claim-hygiene standing orders (`RESEARCH_LEDGER.md` §6 item 4: a number in a document is a claim,
not a fact).

**Finding H-2 (decisive, derived not measured).** On the streams that dominate decode time
(> ~5 KB, which is where F13's `distance_code` 15,680 B and `match_len_class` 8,788 B and any
large literal stream live), the shipped default **under-prices decode cost by 1.7x-19.7x**. The
selector therefore systematically prefers **smaller-but-slower** codings on exactly the streams
that carry the decode budget. **The shipped default is biased in the opposite direction from the
"rate surplus buys decode speed" strategy** that F14's raw-stream budget and Tracks 05/11/18's
decode-cost-gated proposals are built on. Concretely for F14's raw-side-stream trade on a ~19.8 KB
stream: the true time delta is `19,800 × 5.9 ns = 117 μs`, correctly priced at **1.17 B**, priced
at **0.30 B** — **3.9x under-charged**. The "~60 KB rate surplus = explicit compute budget"
reasoning in `docs/CONTEXT.md` has, on this evidence, **never been quantitatively grounded**.

**Finding H-3.** Track 11's H5 is directionally right but mis-stated. The defect is **not** that
"λ·C_decode is too small"; it is that **`C_decode` is not a function of stream length at all on
the default path**, and a correctly-scaled version exists but is default-OFF (C-F3) and
un-arbitered (F9). Track 11's P1d instinct — publish the `λ·c` arithmetic before anything else —
is **correct and should be promoted above all other P1 items.** Note also C-F3's flag-parse
asymmetry: `--hotop-budget=<anything but "off">` turns it ON, so the "default off" is a property
of the *absence* of the flag, and any driver that passes `--hotop-budget=1` silently switches cost
models mid-experiment. That is a **measurement-provenance hazard** for any A/B that sets the flag.

### 3.3a Was `c` *designed* as a per-stream constant, or is it a mis-scaling? — VERIFIED

The coordinator required this be settled before the word "defect" is used. **It is settled, and the
answer is neither of the two obvious options: the constants were designed as PER-BYTE coefficients
and were re-purposed as ABSOLUTE per-candidate penalties when the objective changed form. The
`× L` factor was dropped; the values were never rescaled.**

Evidence chain, all code/doc verified:

1. `src/anvil.cpp:1139` — the **original** design comment at the head of the stream-suite section:
   `J = L + lambda * C_decode * L   (C_decode = per-byte decode cost units)`. **Multiplicative**,
   and explicitly *per-byte*.
2. `FORMAT.md:642` — the cost table is still introduced as "`c` = **per-byte** decode-cost unit
   from the table below", with values 10/22/20/30/35/40/45 described as "integer tenths of the
   `C_decode` values used in earlier text" (raw 1 … ctx-rANS 4.5).
3. `FORMAT.md:845-846` — "Any earlier description of a multiplicative `J = L + λ·C_decode·L`
   (λ = 0.04) is **superseded** — the implemented objective is additive." So the form change was
   deliberate and documented.
4. `src/anvil.cpp:1467` — the comment was **relabelled** to "`C_decode` = **per-STREAM** decode
   cost units" with the **same** integers. This relabel is precisely what makes the incoherence
   invisible on inspection.
5. `src/anvil.cpp:1553` — `J = L + g_stream_lambda*cu`. **The `* L` factor is absent.**

**Dimensional proof that the shipped form is incoherent under its own documentation.** λ = 0.01
bytes/μs. For `λ·c` to be a byte count, `c` must be a **time** (μs). But `FORMAT.md:642` defines
`c` as a **per-byte** cost (ns/byte). So `λ·c` = (bytes/μs)·(ns/byte) = ns/μs, which is **not
bytes**. The shipped default therefore adds a dimensionally meaningless quantity to a byte count.
The size-proportional form `C_us = N · ns_per_byte / 1000` (C-F2) restores consistency exactly:
λ·C_us = (bytes/μs)·(μs) = bytes. ✓

**The benign alternative reading, stated fairly.** One can re-read `c` as an abstract
*dimensionless cost score*, not a time — in which case `λ` is simply "bytes charged per unit of
cost score" and `λ·c` is a deliberate **fixed rate premium per codec** ("pay 0.30 B to avoid a
6 ns/B decoder"). That reading is coherent, and it would make the constants intentional. **But it
has no pre-registered provenance**: it appears only in the `src/anvil.cpp:1467` relabel, it
contradicts `FORMAT.md:642` in the same tree, and it contradicts the project's own committed
calibration (`Experiment AA`, which defines `C_decode` as explicitly **SIZE-PROPORTIONAL**,
`raw_n × ns_per_byte / 1000`).

**Verdict, stated with the burden of proof where it belongs.** The pre-registered and documented
intent is **size-proportional**; the shipped default is a **coherence regression introduced at the
multiplicative→additive correction**, in which a per-byte coefficient was retained at its old
numeric value after its `×L` factor was removed. This is not a claim about the designer's
intent — it is a **code ≠ documentation ≠ pre-registration** contradiction, which stands on its
own. **The burden of proving the fixed-premium reading intentional rests on whoever authored the
additive correction**, and until then the size-proportional form is the correct target because it
is the one the project pre-registered and measured.

**Severity is therefore real but should be stated as a form/coherence defect, not as "wrong
numbers."** If the fixed-premium reading is confirmed intentional, the remedy changes from
"restore `×L`" to "re-derive the seven constants against the measured ns/B table" (F10), and the
magnitude of the mis-pricing in §3.3 becomes a calibration question rather than a correctness one.
Either way `E1a` is the decisive measurement and it is unaffected.

### 3.4 Cold-start / warmup

- **rANS**: static table per stream, built at block start. Model build is `O(256)` per stream.
  No cross-block adaptation → no cold-start *penalty*, but also no cross-block learning.
- **ctx-rANS**: model build measured at **~1.65 ms** for K=12 after the sparse-support quantizer
  work (408 ms → 15 ms → 1.65 ms, `docs/CONTEXT.md`). This is an **encode-side** cost. It is paid
  **once per stream per block** on the encoder only (C-F11). Decoder pays table *construction from
  the wire*, which is `O(256)` for the map + `O(Σ nz)` for classes — the reason F6 measured
  "context-dependent prefix machinery is slower… once model/table setup is counted."
- **Adaptive/2-pass arithmetic (Fenwick)**: rejected outright — "the Fenwick-backed arithmetic
  decoder is the dominant performance problem," 21.19 MB/s decode (F1/Exp. A). Any track-06
  candidate that reintroduces a range-coder or Fenwick decode is re-burning a closed lane.
- **UNMEASURED and material:** the decode-side cost of ctx table construction **as a function of
  block count and stream count** on Windows. Exp. S measured an aggregate −3%…−10% decode but did
  not decompose it into per-symbol selection vs per-block model setup (F3). This is the one place
  where a *layout* change could plausibly recover the ratio win's decode cost, and nobody has
  measured the split. → §6, E2.

### 3.5 Serial dependencies and branch/cache hazards

- **rANS is a serial renormalization chain** (F2 exploited this via 4-way interleaving to expose
  ILP). **This is the single most important fact for the instrumentation question in §7**: the
  inner loop is register-pressure-bound by design, which is precisely the condition under which
  adding a counter is *not* a cheap additive perturbation.
- **ctx-rANS adds a per-symbol table-pointer load** whose address depends on the *previous
  decoded byte* — a genuine serial dependency in the address computation, on top of the rANS
  chain. This is the mechanistic explanation for F3's −3%…−10% decode cost and F5's collapse to
  0.6-0.74 GB/s, and it is why F6's prefix-code variant lost.
- **Cache hazard**: the ctx model's 256-entry map + K tables must be resident per stream; across
  22 streams (F13) a block's working set is the aggregate of all model tables. rANS-4096's table
  is the largest single item. On a 4 KB microarchitectural budget this is the plausible source
  of F5's throughput fall. **UNMEASURED in-repo.**
- **Branch hazard**: the `src.size() < 16` and `>= 4096` gates (C-F5, C-F6) are perfectly
  predictable per stream and cost ~nothing; they are an *economics* gate, not a performance one.
  Do not over-claim here.
- **Decode-state inflation**: worst case per block, ctx-rANS on 8 substreams = `8 × ~295 B =
  ~2.4 KB` of decoder-visible model (H-1) plus the rANS state words. With the single-physical-stream
  discipline of F3 it is ~295 B once. **This is the concrete quantity any track-06 layout proposal
  must be charged against.**

---

## 4. Alternative mechanism if the main framing is judged dead

The track's literal framing ("better entropy backend") is killed in §2.1/§6. The **dissimilar
alternative** I advance — and the only thing in this track I believe is worth funding — is:

> **A byte-size-proportional, measured-cost stream-selection objective, shipped on the default
> path, with a published cost table and a closure test against measured whole-codec decode time.**

Why this is defensible and not a re-skin:
1. It is **not occupied**: zstd's `Compression_Mode` and Brotli's block types select on **ratio**;
   no shipped codec publishes a selection objective weighted in **bytes-per-microsecond**
   (§2.2 last row).
2. It is **falsifiable in one remote CI job** with byte-only arithmetic (§6, E1a/E1b/E1d).
3. It is **already half-built and demonstrably broken on the default path** (H-2), so the work is
   a correction with a known-defect baseline, not a speculative build.
4. It has **leverage beyond track 06**: every decode-cost-gated proposal in tracks 05, 06, 11 and
   18 is calibrated against this term. Fixing it either rescues or kills those proposals, and
   either way it is the cheapest information in the swarm.
5. Its **falsification is cheap and pre-registerable**: if the argmin disagreement rate between
   the two cost models on real corpus content is ≤2%, the defect is immaterial and this dies too
   (§6, K2).

A second, narrower alternative worth PILOTing behind it (§6, E2): **decode-layout co-design of the
context-switched literal stream** — specifically, holding K tables resident and letting *four
interleaved rANS states each pin its own table* so the per-symbol table selection leaves the
serial address chain. This exploits F2 (ILP interleaving) against F5 (throughput collapse).
Honest framing: the *mechanism* (context-mapped literals) is occupied by RFC 7932 (F4); only the
**layout/vectorization** is open, so this is **infra, not novelty**. It must not be reported as a
crossing without a two-plane non-dominated row.

---

## 5. Strongest falsification case, and strongest surviving case

### 5.1 Strongest KILL case (against the track as scoped)

> Brotli has **no adaptive entropy coder** (RFC 7932: Huffman prefix codes only). ANVIL already
> ships rANS + a 7-codec adaptive suite + context-switched rANS, and is *still* 4x behind on decode
> and behind on bytes. The deficit is provably not in entropy coding. Worse, the one axis a new
> entropy backend *could* improve — decode throughput alone — is **arithmetically closed on 12 of
> 13 corpus files** by `DNB-M1` (MATH-class, F11), and the single live cell (`synth-timeseries.bin`,
> 1.8x) is degenerate-flagged by `DNB-M3` (F12). The project's own two best entropy results are
> both explicitly non-crossing: context-switched rANS is recorded as "**No Pareto claim**" (F3)
> and the J-selection suite cleared "EXTENDS_FRONT: **NOT**" (F7). Additionally the one candidate
> that *would* be a genuine step change — FSE — is already shipped by zstd, so it is a lateral
> move against a non-reference. **Therefore no new entropy backend can produce a
> brotli-beating Pareto row, and funding one burns a CI cycle to learn nothing.**

### 5.2 Strongest surviving case

> The entropy *coder* is settled and adopted; the entropy *economics* are not, and are measurably
> wrong in the shipped default (H-2: 1.7x-51x mis-pricing of decode cost, length-independently).
> The measured calibration that would fix it exists, is gated default-off, and **never had its
> arbiter run** (F9). Meanwhile the entire "rate surplus buys decode speed" argument underpinning
> Tracks 05/11/18 rests on that unvalidated term. One byte-only remote job (E1a/E1b/E1d) either
> confirms a real defect and forces re-derivation of ~6 pending claims, or kills the defect
> hypothesis cleanly at ~1% of the cost of a mechanism build. **That asymmetry is the strongest
> available argument in this track.**

---

## 6. Decisive REMOTE-ONLY experiment (pre-registered)

**Name: E1 — "Cost-model reconciliation" (R0 + R2; byte-only, citation-grade).**
Protocol per `docs/GITHUB-ACTIONS-BENCHMARKING.md` §5 `taskset`, §8.1 `manifest.json`+`bytes.csv`,
§9 promotion questions; manual dispatch only. **No local run. No corpus sweep locally.**

**E1a — argmin disagreement (THE decisive number).** For every corpus file (Silesia, enwik8, PE
set `u3`, `generated.{json,jsonl,log,sqlite}`), dump per stream: `(stream_index, N, chosen_codec,
J_const_components, J_prop_components)`. Compute the **argmin disagreement rate** = fraction of
streams where `argmin_c [L_c + 0.10]` ≠ `argmin_c [L_c + N·ns_c·1e-5]`. Deterministic, byte-only,
no timing.
*Report:* overall rate, rate by length bucket (<16, 16-512, 512-4096, ≥4096), and total bytes
under each model. Hypothesis: disagreement concentrates **below N\* = 5,085 B** (§3.3) and is
material on the 16-512 and 512-4096 buckets.

**E1b — J-faithfulness stratified (attack on F7).** Re-derive Exp. L's agreement statistic
**separately per length bucket**. The published 100%/1800 is unstratified; the `<16 B` bucket is
locked to raw by C-F5 and cannot falsify J.

**E1c — the clean paired control (§7).** Instrumentation-tax protocol on mode-15, both arms
instrumented identically, plus a **counters-OUT paired control**. Scout-grade only.

**E1d — publish the arithmetic (do this first; it needs no build at all).** Emit the full
`λ·c` vs `N·ns·1e-5` reconciliation as a table with N* = 5,085 B derived, the per-codec byte
tables of §3.1, and the flag-provenance hazard of C-F3.

**E2 (PILOT, only if E1 clears) — ctx decode-layout.** Single-stream ctx-rANS, K=12, four
interleaved states each pinning one context table, vs the current shared-table-select path.
Byte-identical wire (layout-only). Must show ≥1.10x decode on the literal subsystem at
byte-identical output, or it is KILL as infra.

### Pre-registered thresholds (fixed BEFORE any E1 measurement)

| Gate | PROMOTE | KILL |
|---|---|---|
| **K1 — defect materiality** | E1a argmin disagreement **≥ 10%** of streams on ≥2 corpus files ⇒ cost model is materially wrong on real content | ≤ 2% on all files ⇒ defect is immaterial on real content ⇒ **KILL** the reconciliation workstream as low-value; fold into adopt-class hygiene |
| **K2 — band** (if 2% < rate < 10%) | ≥ 5% of disagreement in the **16-512 B or 512-4096 B** buckets ⇒ PROMOTE as a *correctness-of-experiment* fix (all J-gated claims must be re-derived) but **not** as a compression contribution | disagreement confined to <16 B ⇒ immaterial; KILL |
| **K3 — F7 stratification** | J-faithfulness in the 16-512 B bucket **< 95%** while ≥4096 B **≥ 99%** ⇒ the "100% faithfulness" headline is **stratification-inflated**; Exp. L/C7 figures must be restated with the bucket table | ≥ 99% in every bucket ⇒ F7 survives intact; no restatement |
| **K4 — instrumentation admissibility** | tax ≤ **2%** on both arms **AND** \|tax_A − tax_B\| ≤ the same-job timing CV ⇒ P1c decomposition is admissible | either condition fails ⇒ decomposition **non-attributive**; it may not be cited in any gate; fall back to differential microbench |
| **K5 — new entropy backend (any)** | must reduce bytes **≥1.0% on ≥2 of 4 record files at ≤2% decode regression**. A decode-only win is **pre-refused** by `DNB-M1` on 12/13 files (F11); a degenerate-cell win is void by `DNB-M3` (F12) | any decode-only or degenerate-cell argument ⇒ immediate NO-GO, no measurement needed |
| **K6 — E2 layout** | ≥1.10x decode at byte-identical output on the literal subsystem | <1.10x ⇒ KILL as infra, record; no ratio claim either way |

**Non-negotiable** (master brief item 5): thresholds are not moved after seeing data. Any synth
cell is reported **dual-bar** (raw reference score AND transform-enabled reference score) per
F16/M17.

---

## 7. Audit of Track 11 checkpoint §9 P1 as a common protocol for tracks 05/06/18

Requested by the coordinator. I read `docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md`
§9 P1a-P1d, §10 G1, §12.3. **P1 is the right instrument and Track 11 is right that H5 blocks
everything downstream — but P1c as specified cannot carry the weight, and its most important
limitation is not the one Track 11 names.**

### 7.1 Verdict

| P1 item | Verdict |
|---|---|
| **P1a** (byte-only `H_op` + opcode distribution) | **ADOPT AS-IS.** Deterministic, citation-grade, cannot perturb timing. |
| **P1b** (byte-only token-category / mask / literal / macro shares) | **ADOPT AS-IS.** Same. |
| **P1d** (reconcile λ, c, and the Exp-AA ns/B table) | **ADOPT, AND PROMOTE TO FIRST.** Needs no build. I have effectively pre-computed it (§3.3) — the answer is that the default `C_decode` is length-independent and mis-prices by 1.7x-51x. Run it first, publish it, and let it gate P1c's interpretation. |
| **P1c** (counters for entropy-pull / varint / copy / macro / literal counts, `pulls per output byte`) | **ADOPT ONLY WITH THE SIX MANDATORY ADDITIONS BELOW.** Counters-not-timers is necessary but **not** sufficient. |

### 7.2 Attack: can counters identify entropy cost without perturbing timing?

**No — not on their own, for five reasons.**

1. **Events ≠ cycles.** `pulls per output byte` conflates a 4 KB bulk pull (~0.1 ns/B, F10) with
   a table-Huffman symbol decode (~4.3-9.0 ns/B). Two builds with *identical* pull/byte ratios
   can differ ~2x in real decode time. Converting events to cycles requires per-event ns — which
   **is** the Exp-AA table, which the project itself grades "right ORDER, wrong GAPS" (F10).
   **The decomposition is therefore circular with respect to the cost model it is meant to
   validate.** P1c relocates the dependency on C_decode; it does not remove it. (H-2 makes this
   worse, not better: the observed *mixture* of codecs is the output of the mis-scaled J.)

2. **Codegen perturbation, not arithmetic perturbation.** The dangerous failure is not the `++`;
   it is the compiler changing inlining, unrolling and register allocation. ANVIL's rANS inner
   loop is **deliberately ILP-tuned** (F2: 4-way interleaving bought ~30%). An extra live value in
   a register-pressure-bound loop can spill an rANS state and destroy that ILP — a *multiplicative*
   change, not additive. This cannot be bounded a priori; it must be measured.

3. **Latency-chain aliasing.** rANS decode is a serial renormalization chain. A counter off the
   chain is nearly free; a counter that aliases a state register sits **on** the chain. Tax is
   therefore **data-dependent and arm-dependent** — it cannot be assumed constant or symmetric.

4. **Asymmetric-arm bias.** Instrumenting only the mechanism arm and comparing to a clean
   baseline **manufactures the effect**. This is the classic self-fulfilling ablation.

5. **Selection confound (the worst one, and Track 11 does not name it).** The codec mixture the
   counters observe is itself the output of the mis-scaled J (H-2). P1c therefore measures a
   distribution the encoder has already distorted, and **cannot** answer the question tracks
   05/06/18 actually want — *"what would a corrected cost model choose?"* P1c describes the
   incumbent; it does not evaluate the alternative.

**Additional protocol defect:** P1c's categories are a **list, not a partition**. As specified they
can overlap (an entropy pull that is also a varint read that is also a macro-path entry) and leave
gaps (literal bytes vs copy bytes vs "everything else"). A cost decomposition must close.

### 7.3 Mandatory additions — the no-instrumentation paired control

- **C1 — CLEAN PAIRED CONTROL (the control the coordinator asked for).** Every A/B pair runs
  **twice in the same CI job**: (i) **both arms counters-OUT**, median-N; (ii) **both arms
  counters-IN**, median-N. Report `tax_A = (t_in(A) − t_out(A))/t_out(A)` and the same for B.
  A decomposition is only admissible if the clean pair is measured in the same job, so the
  instrumented pair's delta can be compared against a tax-free baseline delta.
- **C2 — pre-registered tax budget.** Admissible iff `tax_A ≤ 2%` **and** `tax_B ≤ 2%` **and**
  `|tax_A − tax_B| ≤` the same-job timing CV. Otherwise the decomposition is declared
  **non-attributive** and may not appear in any gate (this is pre-registered as **K4** in §6).
- **C3 — byte-identity under instrumentation.** Counters must not change output. SHA-compare
  counters-OUT vs counters-IN on **every** corpus file + both pinned PEs. Any mismatch ⇒ codegen
  changed the algorithm ⇒ **VOID**. (Catches reason 2 when C2 happens not to trip.)
- **C4 — partition closure.** Require, and report as pass/fail: `Σ(opcode pulls) == ` hot-op tokens
  decoded (cross-checked against **P1a's** byte-only distribution — this is why P1a must run);
  `Σ(copy bytes) == ` total match bytes; and a reconciled grand total against output bytes. A
  partition that does not close is a list, not a decomposition.
- **C5 — cycle attribution is a second, independent step.** Per-event costs must come from
  **differential microbench** (region disabled vs enabled in the same binary), each reported with
  its own CV. Then run the **closure test**: predicted total cycles from (counts × costs) vs
  measured whole-codec decode time. **`|predicted − measured| > 20%` ⇒ decomposition VOID.**
  This is the falsifiable check that P1c otherwise lacks, and it is what forces the honest
  dependence on a cost table to be *validated* rather than assumed.
- **C6 — no per-region timers as primary.** `rdtsc`/`QueryPerformanceCounter` per region on a
  shared runner has ~10-50 ns granularity against regions of tens of ns; it is worse than useless
  and F17 already rules small shared-runner deltas inconclusive. Permitted **only** inside the
  C1 same-job differential form.
- **C7 — grade labeling.** P1c output is **scout/ranking-grade**. Only P1a/P1b/P1d are
  citation-grade. This must be printed in the artifact header so no downstream brief can promote it.

### 7.4 One correction to Track 11's own reasoning

Track 11 §12.3 says "**P1d first, before P1a**". I agree on P1d-first but **P1a must run
immediately after and before P1c**, because C4 makes P1a's byte-only opcode distribution the
*independent check* on P1c's counter totals. P1c without P1a is unverifiable.

Track 11 §12.2 also states its H5 as "λ·C_decode is currently too small to move real byte-vs-cycle
decisions." My code-level finding is sharper and should replace that phrasing: **on the default
path `C_decode` is a constant, not a function of stream length, and the correctly-scaled
size-proportional version is gated behind a default-off flag whose arbiter never ran.** "Too
small" is true only for N > ~5 KB; for N < ~1 KB the shipped term is 10-51x **too large**. Any
brief that repeats the "too small" framing will draw the wrong remedy (raise λ) — raising λ would
make small-stream mis-pricing *worse*. The remedy is the size-proportional form, not a bigger λ.

---

## 8. Decoder / resource risks

| # | Risk | Severity | Basis |
|---|---|---|---|
| R1 | **Decode-state inflation via ctx fan-out**: per-stream ctx-rANS costs `n_streams × ~295 B` per block of decoder-visible model vs ~295 B once. The only thing preventing it is the `≥4096 B` gate (C-F6), which is an economic guard, not a stated invariant. | **high** if any track-06/18 proposal instantiates ctx per stream | C-F7, §3.1 |
| R2 | **Cost-model defect propagates silently** into every J-gated claim in tracks 05/06/11/18. | **high** | H-2 (C-F1 vs C-F2) |
| R3 | **Flag-provenance hazard**: `--hotop-budget=<x>` for any `x ≠ "off"` silently switches cost models; two arms of one A/B could run different cost models. | **medium** | C-F3 (line 4979) |
| R4 | **Unconditional 256 B tables** in Huffman and ctx-rANS make small streams structurally un-codable; the `<16 B` dead zone is unlogged. | medium | C-F5, C-F8, §3.2 |
| R5 | **Serial address dependency** in ctx-rANS (table chosen by previous byte) is the mechanistic cause of F3's −3…−10% and F5's 0.6-0.74 GB/s; any fix must not add a second dependent load. | medium | §3.5, F3, F5 |
| R6 | **Encoder cost is not decoder cost**: C-F11 evaluates all candidates per stream including full ctx model construction. Encoder regressions from ctx enablement will be real and must not be charged to the decoder or hidden. | low-medium | C-F11 |
| R7 | **Malformed-input surface**: any new model field (extra table, RLE context map) enlarges the strict-rejection surface that `FORMAT.md` §Integrity and M13/M14 govern; a new decoder-visible registry ID requires a **forced** encode→decode test (M14) or the path rots silently. | medium | F17-adjacent; `FORMAT.md`; Track 11 M13/M14 |

---

## 9. Failure modes if E1/E2 proceed

| # | Failure mode | Expected behaviour |
|---|---|---|
| F-1 | E1a disagreement is ~0 because real streams cluster tightly and raw wins on bytes anyway | K1 fires, workstream killed cleanly, F7/F8 survive intact. **Good outcome — one CI job.** |
| F-2 | E1a disagreement is large but *concentrated in the `<16 B` bucket* | K2 fires (immaterial). The defect is real but cosmetic. Do **not** promote. |
| F-3 | E1b shows F7's 100% is stratified-inflated | Exp. L/C7 figures must be restated with a bucket table. The mechanism stays validated; the **headline number** does not. Note C7 was already recorded as *enabling infrastructure*, so the blast radius is the figure, not the claim. |
| F-4 | P1c tax > 2% or asymmetric | K4 fires; decomposition non-attributive; Track 11 G1 (≥25% entropy+dispatch) becomes **unmeasurable as specified** and FLI's decode leg must be re-grounded on differential microbench or declared DECODE-SHORT. |
| F-5 | E2 shows four interleaved ctx states do not beat shared-table-select | K6 fires; record and move on. **No ratio claim either way** — the ratio is already settled by F3. |
| F-6 | Someone proposes FSE as a "new backend" | Prior-art kill per §2.2 (already in zstd/RFC 8878) + K5 arithmetic. Do not spend a build. |
| F-7 | A synth-cell win is reported against the raw bar only | **Struck** per F16/M17 dual-bar rule. |

---

## 11. Cross-track audit: Track 15's CAM alternative, from the entropy seat

Requested by the coordinator. I read `docs/swarm-2026-10-02/15-crossblock-memory-space-bunny.md`
§2.2, §4.1-4.4, §5, §6, §7.2-7.4. **CAM is a model-initialization question, not a new entropy
coder**, and I audit it on that basis — which also means it falls **inside** the surviving
workstream of §4, not inside the KILL of §2.1.

### 11.1 What must actually be primed (code-verified from my §3.1 table)

| model | state to prime | serialized size |
|---|---|---:|
| rANS-256/512/4096 | 256-symbol order-0 freq table | `4 + 2s` B, ≤ ~516 B |
| ctx-rANS | **256-entry context map** + K class tables | **`259 + 3K` B (≥295 at K=12)** |
| Huffman | 256 length bytes | **258 B always** |
| literal context (prev byte) | 1 byte | **free — self-priming** |
| per-shape displacement (mode 12/15) | 1 value/shape | negligible |

### 11.2 The capacity pincer on CAM-1 — the answer to the coordinator's question is **no**

**Can a compact 64-256 B prior represent the context/MTF state responsible for the +5-17 % tax
without becoming a large table or hidden dependency? Measured answer: no, and the two analyses
point in opposite directions.**

- **Capacity floor for a complete order-0 prior:** 256 count values needed. At 4 bits/count
  (16 levels) that is **128 B**, plus a symbol-presence descriptor — **32 B** as a bitmap, or
  **256 B** explicit. So a *complete* order-0 prior costs **≥160 B** at the coarsest useful
  precision, and that precision is coarse relative to a 4096-scale total.
- **A complete ctx-rANS prior does not fit in 256 B at all.** The context map alone is 256 B
  (C-F7). With K=12 class tables the honest floor is **≥500 B**, realistically **700+ B**.
- **Now cross-reference Track 15's own break-even table (§4.3).** P=64 B and P=256 B are the
  sizes that break even cheaply (0.11 % / 0.45 % of Silesia; needing ≤0.81 % / ≤3.25 % of the tax
  to be model state). P=1024 B is the smallest size that can *actually represent* the ctx-model
  state — and §4.3's own table shows it "is admissible only at ≤256 KiB blocks and fails at 4 MiB
  unless ≥38.4 % of the tax is model state."
- **⇒ Adversarial pincer: the P values that are cheap are the P values that cannot represent the
  state; the P value that can represent it is the one that usually fails break-even.** §4.3
  performs the byte break-even analysis and **never performs the representation-capacity
  analysis**, so the table is not decision-grade as printed.

### 11.3 The hidden dependency the coordinator asked about: **per-model-type multiplicity**

This is the sharpest new finding and it is downstream of my §3.1 code-verified table audit.

Track 15 charges CAM-1 a single **`P` B/block**. But ANVIL's model state is **per stream and per
model type**: modes 10-13 carry 5-8 substreams each with its own order-0 table (C-F1, `src/anvil.cpp`
call sites 2161/2234/2500/2642), and mode-15 carries **9 book streams** (line 2868). A prior for
"the block's model" is not one object — it is at minimum **one per distinct model type present**:
order-0 rANS, ctx-rANS (256 B map + K tables), and Huffman lengths.

| charged as | actual, at P=256 B × 3 model types | on Silesia (809 blocks) |
|---|---:|---:|
| Track 15 §4.3: `P` = 256 B | 0.4459 % | 207,104 B |
| **per-model-type: 3 × 256 B** | **1.3377 %** | **621,312 B** |

**That is a 3.0x undercharge**, before any bytes for interpretability (§11.5, M6). And it is worse
than 3x in principle: the two 256-byte-map models (ctx map, Huffman lengths) dominate the budget
while contributing the least *shared* information — they are different objects and cannot share a
prior without changing what the decoder reconstructs.

### 11.4 The target is real but **mislabeled** — "warm-up" is the wrong mechanism

Track 15 inherits M4's framing ("independent blocks restart MTF/context state… warmup is the whole
story") and uses **BWT's** +12.79 % as the "pure warm-up floor" (§2.2). Three corrections from the
entropy seat:

1. **ANVIL's incumbent entropy models are not online-adaptive.** `build_rans_model(src)` and
   `build_ctx_model(src, …)` are **batch** constructions from the block's own bytes, rebuilt per
   stream (C-F9). There is **no MTF** anywhere in the rANS/Huffman/defexc/ctx-rANS suite, and no
   per-symbol incremental adaptation. So the warm-up exposure ANVIL actually carries is
   **not** an unbounded online-adaptation cost.
2. **The real per-block penalty is estimator variance, not cold start.** Because the table is fitted
   to the block's own bytes, the model is already block-adapted by construction; what independent
   blocks cost is that **each block's statistics are estimated from that block alone** (higher
   variance) rather than from the whole file (lower variance). *Variance reduction* — not warm-up —
   is the correct label, and the two have **different** optimal remedies.
3. **This inverts Track 15's central ranking.** For a variance-reduction target, a **previous
   block's converged counts (CAM-1) carries strictly more information** than re-deriving from the
   block's own first K bytes (CAM-0), unless the block's prefix is *representative* of its body —
   an extra assumption CAM-0 requires and does not get for free. Track 15 asserts "**CAM-0 is my
   contribution and it dominates CAM-1 on every axis except one**" (§4.2). On the mechanism axis
   that is **not correct**; CAM-0's win is the 0-wire budget, not information content. (CAM-0's
   zero-wire property *does* hold — I verified the mechanism is implementable with no transmitted
   state, given a prefix re-estimation pass at byte K. That part is sound.)

**A/BWT transfer caveat.** Using BWT's tax to bound ANVIL's addressable share requires BWT's
context model to lower-bound ANVIL's. They warm up differently by construction (§11.4.1), so the
"≤0.95 pp not-warm-up" bound (§2.2 consequence 1) is a bound on *BWT-vs-Brotli*, and its transfer
to ANVIL is an assumption, not a measurement.

### 11.5 Required ablation — the coordinator's cold-vs-prior same-parse test

**Track 15 already specifies the right experiment.** M0/M1/M2/M3 with `token_stream_sha256` frozen
across arms, byte-only, no timing claim, and **M3 (shuffled/null prior) explicitly identified as
load-bearing** — "if M2 ≈ M3, the gain is a warm-start artifact carrying no cross-block
information." **That is methodologically the strongest element of Track 15** and it anticipated
this request. I confirm it as **mandatory and sufficient-in-principle**, and add three arms from
the entropy seat:

- **M5 (new, REQUIRED — separates dead mechanism from bad implementation).** Prime from the
  block's own first K bytes as in M1, but seed the counts from the **whole-block** counts — a
  *non-causal oracle upper bound* on what **any** zero-wire intra-block scheme can recover. If
  `M5 ≈ M1`, the intra-block signal is exhausted and only a transmitted prior can help. If
  `M5 ≫ M1`, the loss is an implementation artifact, not a mechanism limit. **Without M5 the
  ablation cannot distinguish those two**, and that distinction is what previously separated
  "representation useful, matcher can't see it" (TCOPY) from "representation closed" in this
  project's history.
- **M6 (new, REQUIRED — charge ALL prior bytes).** Define `P_total = P + Q`, where `Q` is every
  byte needed to make the prior *interpretable*: dequantization scale/precision, which-symbols-are-
  covered, and any per-class descriptor. Recompute §4.3's break-even on `P_total`. A 64 B quantized
  prior with a 32 B presence bitmap and a 2 B scale is **98 B, not 64 B** — a 53 % overstatement of
  efficiency. Report both.
- **M7 (new, REQUIRED — per-model-type stratification).** Emit the byte delta **per model type**
  (order-0 / ctx / Huffman-lengths) and **per stream-length bucket**, so that the C-F5 `<16 B` dead
  zone and the C-F6 `≥4096 B` ctx gate are visible in the result. Streams under 16 B can benefit
  from **no** prior at all; a per-block aggregate will silently average that away.

### 11.6 Novelty vs engineering value — classified separately, as instructed

| axis | classification | basis |
|---|---|---|
| **Novelty** | **≈ NIL. Adopt-class. I do not contest this and Track 15 does not claim it.** | Cross-block model carryover / reset is a **decades-old, solved design axis** in every block-based adaptive format (DEFLATE resets per block; bzip2 resets MTF per block; LZMA resets per block/chunk; zstd exposes `ZSTD_reset_session_only`). CAM-0's specific twist — re-estimating from the block's own already-decoded prefix at byte K, zero transmitted bytes — is the standard periodic-restart / decaying-count remedy for estimation noise in on-line learning. Track 15's §6 is honest about this and routes the separator to Track 20. **Correct posture; no novelty claim should survive into any synthesis.** |
| **Engineering value** | **Potentially real, currently unproven, and gated on §11.4's misattribution + §11.3's 3x undercharge.** | If the tax is estimator variance (my reading), a zero-wire intra-block re-estimation is cheap, RSS-neutral, and — genuinely valuable — preserves independent decodability, random access and parallel decode **structurally**, which is exactly what PIM destroys. Those are real properties worth paying for. |

### 11.7 CAM verdict from the entropy seat

**HOLD — with M5/M6/M7 added as mandatory, and the stated ranking between CAM-0 and CAM-1
corrected.**

- **Not KILL:** the fragmentation tax is measured and real (§2.2, [A]); CAM-0's zero-wire property
  is **sound** (verified implementable with no transmitted state); and M0-M4 + frozen token hash +
  the M3 null prior is a genuinely sound design.
- **Not PILOT:** the target is misattributed (§11.4), the byte budget is undercharged ~3x by
  ignoring per-model-type multiplicity (§11.3), and CAM-1's only representable `P` (≥1024 B) is
  the size §4.3 itself shows failing at 4 MiB (§11.2).
- **Promote to PILOT only if, on top of Track 15's frozen thresholds:** M5 lands ≥0.30 % (so the
  mechanism is live rather than unimplemented), **and** the per-model-type `P_total` break-even
  still clears on Silesia, **and** §11.4's variance re-labelling is recorded so the M1-vs-M2
  comparison is read correctly.
- **Correction Track 15 should adopt:** "CAM-0 dominates CAM-1 on every axis except one" →
  "CAM-0 wins on wire cost and independence; CAM-1 wins on information content for a
  variance-reduction target. Neither dominates; the ranking is an empirical question the M1/M2 arms
  must settle, and §4.3's break-even table is not decision-grade until §11.2's capacity floor and
  §11.3's per-model-type multiplicity are folded in."

---

## 10. Recommendation

### 10.1 Dispositions

| item | disposition | why |
|---|---|---|
| **New entropy-coder primitive as a crossing route** (FSE, tANS, novel ANS variant, adaptive range coder, multi-stream context fan-out) | **KILL** | §2.1 (revised): prior-art occupied + `DNB-M1` (F11) closes the decode-only plane on 12/13 + `DNB-M3` (F12) voids the one live cell + K5 pre-refusal. FSE is prior art in zstd. Range/Fenwick decode re-burns Exp. A. Fan-out is pre-refuted by H-1. **Scope note: this kills the *primitive*, not entropy-layer work — model/layout/cost work on the incumbent rANS survives below.** |
| **CAM (Track 15) — model initialization** | **HOLD**, M5/M6/M7 mandatory | §11. Novelty ≈ nil (adopt-class, correct posture). Engineering value real but gated on §11.4's variance-vs-warmup misattribution and §11.3's ~3x per-model-type undercharge. Zero-wire property of CAM-0 verified sound. |
| **rANS / ctx-rANS as *novelty*** | **KILL as novelty; RETAIN as adopted infra** | RFC 7932 §7 lineage, recorded as a correction in this repo's own ledger (F4). |
| **Cost-model reconciliation E1 (a/b/d)** | **PROMOTE-TO-REMOTE** | Byte-only, citation-grade, needs no build, one CI job, ~1% the cost of a mechanism build, and it gates five other tracks. Pre-registered kills K1/K2/K3. |
| **Instrumentation protocol P1c** | **ADOPT WITH C1-C7** | §7. Counters are necessary-not-sufficient; the clean paired control + partition closure + 20% closure test are mandatory. |
| **E2 ctx decode-layout (4 states × pinned tables)** | **PILOT, strictly behind E1** | Only open entropy question with a mechanistic hypothesis (R5/F5) and an honest infra (not novelty) framing. K6 = 1.10x byte-identical. |
| **Sparse/RLE context-map encoding (byte win)** | **HOLD — cheap, adjacent, not yet scoped** | C-F7/C-F8 show 256 B unconditional tables; a compact map is real bytes. But Brotli already does compact context maps (§2.2), so it is adopt-class. Fold into E1's report; do not fund separately. |

### 10.2 Final verdict: **PROMOTE-TO-REMOTE** (re-scoped), with the track's literal scope **KILLed**

**PROMOTE-TO-REMOTE** for **E1** — `docs/GITHUB-ACTIONS-BENCHMARKING.md` §8.1 artifact
(`manifest.json` + `bytes.csv`), manual dispatch, one job, no local run, `taskset` pin per §5.
Promote E2 to PILOT only if K1 fires (defect material) — E2's justification is the *cost* result,
not the mechanism.

**The track as literally scoped ("entropy backend co-design that crosses the frontier") is
KILLed**, on prior-art + arithmetic grounds that no implementation can overturn (§5.1).

I am **not** issuing HOLD, because the surviving workstream (E1) is cheap, byte-only,
pre-registerably falsifiable, already half-built with a known-defect baseline, and unblocks five
tracks. I am **not** issuing PROMOTE for any *mechanism*, because nothing in the entropy-coder
space has room (§2.1) and two of the project's three best entropy results are already recorded as
non-crossing (F3, F7).

### 10.3 Quantified accounting summary

**Bytes (decoder-visible, per stream, code-verified):** raw 2 B · rANS `4+2s` B (≤516) ·
Huffman **258 B always** · defexc `3+N/8` · ctx-rANS **`259+3K` B (≥295 at K=12)**. Fan-out risk
`n_streams × ~295 B/block` (R1). `<16 B` streams locked to raw with up to 13.3% header overhead
(§3.2).

**Cycles (decode-cost model, F10 median-7):** raw 0.04-0.10 ns/B bulk / 2.59-2.66 byte-style ·
rANS-4096 5.93-6.62 · rANS-512 5.44-6.55 · rANS-256 5.40-5.99 · Huffman 4.08-9.0 (data-dep) ·
defexc 3.14-3.25 · ctx-rANS pull 7-8 / materialize 4.6-5.3. **Shipped J mis-prices these by
1.7x-51x** (H-2). Break-even length between the two cost models: **N\* = 5,085 B**.

**Memory:** ctx-rANS decoder model ~295 B (single stream) to ~2.4 KB (8 streams/block, R1);
rANS state words negligible; no new amplification class if the single-physical-stream discipline
of F3 is preserved. E2 changes layout only → **0 bytes, 0 model bytes, expected 0 RSS delta**.

**Encode (not decoder-charged):** ctx model build ~1.65 ms at K=12 after the sparse-support
quantizer work (408 → 15 → 1.65 ms), paid per qualifying stream per block (C-F11).

### 10.4 Explicit falsification criteria (binding)

This recommendation is **wrong** — and the workstream should be killed — if any of:

1. **E1a argmin disagreement ≤ 2% on all corpus files** ⇒ H-2 is immaterial on real content; the
   shipped constant-`c` form is adequate in practice and the reconciliation has no value beyond
   hygiene (K1).
2. **E1b J-faithfulness ≥ 99% in every length bucket** ⇒ F7 survives unstratified and no
   restatement is owed (K3).
3. **K4 fails** ⇒ P1c cannot attribute cycles between arms; Track 11's G1 (≥25% entropy+dispatch)
   is unmeasurable as specified and any FLI decode claim must be re-grounded or dropped.
4. **K6 fails** ⇒ the ctx serial-address-chain hypothesis (R5) is wrong and there is no open
   entropy decode lever in this architecture at all.
5. **Contradiction from the constructive lane:** if `06-entropy-codesign-space-bunny.md` presents a
   byte-reducing entropy mechanism with a pre-registered ≥1.0%-on-2-of-4-record-files result
   (K5) **and** a two-plane non-dominated row, then §2.1 is refuted on its own terms and this
   KILL must be revisited. I have pre-committed to that reconciliation rather than defending the
   verdict.
6. **§3.3a is wrong about *form* (not about magnitude):** if the additive fixed-premium reading is
   produced with pre-registered provenance, then `×L` should **not** be restored and the remedy
   becomes recalibrating seven constants against F10's ns/B table. H-2's 1.7x-51x *magnitudes*
   survive; its *diagnosis* would change from coherence-regression to calibration-drift. E1a is
   unaffected either way.
7. **§11 is wrong about CAM:** if a byte-only same-parse ablation shows the per-model-type
   `P_total` break-even clearing on Silesia **and** M5 ≥ 0.30 %, CAM promotes to PILOT over my
   objection. I pre-commit to that too.

### 10.5 Reconciliation pending

`06-entropy-codesign-space-bunny.md` had not appeared when this interim audit was written. **This
file will be revised, not replaced**, when it lands, with an explicit disagreement-resolution
section. Until then the constructive lane is unaddressed and no claim is made about it.

---

---

# PART II — RECONCILIATION (added after `06-entropy-codesign-space-bunny.md` landed)

## 12. Q3-SELECTOR-PREFLIGHT independent audit

Constructive report read in full (`06-entropy-codesign-space-bunny.md`, 527 lines).
`Q3-SELECTOR-PREFLIGHT` had **not** appeared at audit time — §12.9 pre-registers its audit criteria.

### 12.1 Reproduction of the 792-selection / 139-exact-L-tie finding — **internally consistent, but NOT independently reproducible**

Primary artifact: `RESEARCH_LEDGER.md:3965-3979` ("arch selector manifest, diagnostic build =
fc23d9a + SEL logging only"). Arithmetic self-check, done by me:

| claim | check | result |
|---|---|---|
| 792 selections = 88 blocks × 9 streams | 88 × 9 | **792 ✓** |
| 139 flips = m3→m2 ×93, m3→m1 ×35, m2→m1 ×11 | 93+35+11 | **139 ✓** |
| flips on 20 of 23 files; zero on synth-arith, random.bin, repeat.jsonl | 23 − 3 | **20 ✓** |

All three decompositions close exactly. **The finding is arithmetically self-consistent.**

**But it is a claim, not a citation-grade fact, by the project's own rule.** The raw
`stream_log` output format is `src/anvil.cpp:5010` (`stream_log chosen=… l_winner=… chosen_L=…
min_L=…`). I searched the tree: **no raw `stream_log` artifact is checked in**, and the ledger entry
shows **no verifying command**. `RESEARCH_LEDGER.md` §6 item 4 — "a number in a document is a claim,
not a fact… recompute from the source artifact at the moment of writing with the verifying command
shown" — therefore binds this entry. **Reproduce with one command before it is cited as a gate
input.** I did not run it: that is a benchmark action, out of scope for this audit.

### 12.2 Zero byte delta and zero speed delta — treated **separately**

These are two different measurements and the record contains both. Conflating them is the main
risk in this lane.

| quantity | measured | source |
|---|---|---|
| **Complete bytes, budget-ON vs legacy** | **ZERO.** SIZE-equality **22/22**; `generated.log` **175,550 B exact**, json 121,316, jsonl 222,381, sqlite 366,019 | `RESEARCH_LEDGER.md:3998-4001` |
| **Wire content** | **hashes DIFFER** — precision flips change bytes-in-place at L-ties, so length is preserved but content is not | `:3973-3975, 3999-4001` |
| **Decode throughput** | **ZERO.** budget-on 234.4 MB/s vs state-A band 231.6 / 235.8 / 240.4 → "UNCHANGED WITHIN NOISE" | `:3991-3993` |
| **Decode throughput, 2nd protocol** | **ZERO.** arch CLI median-3: OFF 206.8 vs ON 201.0 MB/s, inside the record's 5-19% noise band | `:3996-3997` |

**Both deltas are independently measured zero, on two protocols.** This is the single most
important fact in the lane and it is already in the record.

### 12.3 Why the speed delta is **structurally** zero — the cleanest refutation available

`kBudgetNsPerByte[7] = {0.1, 6.0, 6.0, 6.0, 4.3, 3.2, 7.5}` (`src/anvil.cpp:1618-1626`) assigns
**identical 6.0 ns/B to rANS-4096, rANS-512 and rANS-256**. Every one of the 139 flips is a swap
*within that trio*. Therefore:

> **The objective that produced the flips has zero cost resolution across exactly the codecs it
> flipped between.** A precision swap under the budget objective is not a speed decision — it is a
> tie broken by candidate insertion order (`m3→m2 ×93` "flat-6.0 J-ties broken toward
> earlier-added candidate", `:3974-3975`). **A1 cannot produce a speed delta even in principle.**

This is why the arbiter's A1 arm is not merely "expected null" — it is **incapable** of producing
a signal. Its hypothesis was falsified by the shape of the cost table before any run.

Corroborated from the other direction: the flipped streams' in-block pull cost roughly doubles
(macro-types 26→46 cyc/B) yet types+dflags+opcodes are **0.2-0.3% of decode** → "~0.001% stakes"
(`:3993-3996`). A real 1.77x cost increase on 0.25% of the work.

### 12.4 My independent upper bound on whole-codec speedup from precision swaps

Derived by me from the ledger's own numbers. No run.

**Bound A — Amdahl over the flipped streams (tightest, mechanism-specific).**
f = 0.002…0.003 of decode; measured cost multiplier r = 46/26 = **1.77**.

| case | new total cost | whole-codec throughput |
|---|---:|---:|
| flipped cost → 0 (impossible upper bound) | 1 − f = 0.9975 | **1.0025x** |
| measured r = 1.77, f = 0.0025 | 1 + 0.0025×0.77 = 1.0019 | **0.9981x (0.19% SLOWER)** |

⇒ **Absolute ceiling for precision swaps alone = 1.003x. Point estimate = 0.998x.**

**Bound B — absolute ceiling for *any* selector change.** crc32 = **44% of decode** (F18;
corroborated by DNB-M1's 1.79x byte-identical ceiling, `RESEARCH_LEDGER.md:4247`). Stream decode is
therefore ≤56%: `1/(1−0.56)` = **2.27x** — but only by eliminating *all* stream decode, which
requires paying bytes. Unreachable for a selector.

**Bound C — realized ceiling from the manifest = 1.00x exactly.** All 139 flips lie inside the
6.0 ns/B iso-cost group. Realized speed ceiling: **zero**.

**Bound D — the remaining reachable trades are closed, and mutually exclusive with headroom:**

| stream class | headroom | blocker |
|---|---|---|
| mdvar, mdflags, mll | **none** — already raw | `chosen_L == min_L` on every line (`:4006`) |
| lits, mmasks | rANS 6.0 → raw 0.1 = **60x** | byte gaps **> 100 B**; λ=0.01 buys **0.02-0.66 B** ⇒ short by **~150x-5000x** |
| any | first raw-flip candidate | **λ ≈ 44.6 B/μs** — 4.66 orders above λ=0.01 (`:4008-4010`) |

> **The streams that have speed headroom all carry byte gaps beyond any plausible λ; the streams
> within λ's buying power have no headroom. The two sets are disjoint.** That is an arithmetic
> result, not an empirical one, and it is why "zero raw-flips" was *predicted* (`:4011`: "Zero
> flips-to-raw was arithmetically certain under the frozen constant").

### 12.5 Ruling: **DELETE the Q3 remote timing arm.** Do not repeat it.

Ceiling **1.003x (0.3%)** versus the noise floor:

| floor | value | ratio |
|---|---:|---:|
| F18 zero-byte-change controls | **+2.3-8.1%** | ceiling is **7.7x-27x below** |
| record band (`:3997`) | **5-19%** | ceiling is **17x-63x below** |

**And the floor is environmental, not statistical.** F18 establishes the 2.3-8.1% band on controls
that **change zero bytes** — i.e. run-to-run variation with the work held identical. More
repetitions shrink the estimator's variance; they do **not** shrink an environmental floor. **No
sample size, pairing scheme, or repetition count on a shared GitHub Actions runner can resolve a
0.3% ceiling against a 2.3% floor.** Repeating the arm would spend a CI cycle to re-derive a
zero that is already measured twice (§12.2) and bounded arithmetically three ways (§12.4).

**Explicitly recommended: remove Stage 2 / remote timing from Q3 entirely** — do not defer it,
do not "run it once to confirm". It is not underpowered; it is **structurally incapable of
discrimination**.

### 12.6 The scope gap that legitimately keeps Stage 1 alive

The manifest is **88 blocks × 256 KiB ≈ 23 MB across 23 files** — every corpus file is 1-3 blocks.
Stream lengths there sit **at or below N\* = 5,085 B**, i.e. inside the regime where the shipped
constant-`c` term **over**-prices decode cost. The regime where it **under**-prices (large streams,
N > N\*) has **zero manifest coverage**, and that is precisely where the divergence grows
(3.1x at 15,680 B; 19.7x at 100,000 B — §3.3).

So: **the manifest's null does not bound Stage 1 on Silesia/enwik8 at ≥1 MiB blocks.** Stage 1
is worth running *there*, byte-only. Its stated value is not "will the selector move?" — that is
already answered — but "**does the sign of the defect flip in the untested regime?**"

### 12.7 Gate defect: **K1 and K3 return opposite verdicts on already-measured data**

From the manifest: argmin disagreement = 139/792 = **17.55%**. Complete-bytes delta = **0.00%**.

- **K1** (argmin disagreement **≥10% ⇒ PROMOTE**) → **fires. PROMOTE.**
- **K3** (complete-bytes delta **≥1.0% ⇒ PROMOTE**) → **fails. KILL.**

**The arbiter as frozen would promote a workstream that the record already shows produces zero
bytes.** `argmin disagreement` is demonstrably **decoupled** from byte outcome — a 17.55% flip rate
with an exact 0.00% byte delta is the proof, and it is already in hand.

**Required before dispatch:** K1 must **lose promote authority**. Collapse it into K3 and report
argmin disagreement as a **diagnostic with no gate power** (same treatment the coordinator has
correctly applied to AOC's H(op) diagnostic — §12.8). Alternatively re-key K1 to
"argmin disagreement **that changes complete bytes**", which on this record is 0%.

### 12.8 AOC (R3): **no wire proposal before complete charges; H(op) − H(op|prev) is diagnostic only**

`06-entropy-codesign-space-bunny.md` §3.2 item 5 proposes measuring `H(op)` vs `H(op|prev_op)`
byte-only, with K7 promoting at **≥0.5% of complete bytes**. Three objections:

1. **The diagnostic omits the entire model cost.** `H(op) − H(op|prev_op)` is a *gross* entropy
   saving from an **ideal coder with zero model cost**. AOC's real model is not free: a
   context-conditioned order-1 literal stream costs **a 256-byte context map plus K class tables =
   `259 + 3K` B** (`ctx_stream_bytes`, §3.1/C-F7), plus stream header, plus decoder state/RSS. On a
   mode-15 opcode stream with only 75-77% hot coverage (F7/M7), the map must cover the **full**
   alphabet including cold/escape opcodes, so K is set by the whole opcode set, not the hot subset.
2. **K7 compares gross to net.** "≥0.5% of complete bytes" measured on the gross number, then
   compared against a byte budget, is not a valid comparison. The required quantity is
   `net = (H(op) − H(op|prev)) × n_opcodes/8 − model_bytes − header_bytes`, and only then the
   decode delta. **H(op) − H(op|prev) screens; it does not promote.**
3. **New substream mode id carries obligations the arbiter does not charge.** Per M14 /
   `FORMAT.md` §Registry coverage, a new decoder-visible registry id requires a **forced**
   encode→decode test or the path rots silently; and it enlarges the malformed-input rejection
   surface owned by Track 19. Both are hard preconditions, not notes.

**Demand, as a hard gate before any AOC wire proposal:** (i) exact table serialization format and
its byte count; (ii) header bytes; (iii) decoder state / peak-RSS delta; (iv) cold-start
behaviour for rare/unseen contexts, including what the decoder does when the context is
out-of-book; (v) the net arithmetic above; (vi) the forced registry test. **Absent all six, AOC
stays PILOT-at-best with no wire commitment.**

### 12.9 Constructor-verification items the coordinator required

**Default λ = 0.01 — CONFIRMED, three sites.** `src/anvil.cpp:1471` (`static double
g_stream_lambda = 0.01`), `:3783` (`double stream_lambda=0.01`), `:4662` (assignment).
`g_stream_mu`/`g_stream_nu` are `0.0` at `:1472` and are **never reassigned** — no CLI, no env —
so the `μ·2` and `ν·c` terms in `FORMAT.md:639` are **dead code** and the shipped objective is
exactly `L + λ·c`. Good: no hidden term.

**Formula drift — ONE FOUND, and it is material.** Constructive §3.2 item 1 writes the Stage-1
disagreement test as `argmin[L+0.10·c] ≠ argmin[L+N·ns·1e-5]`. **The shipped constant is `0.01·c`,
not `0.10·c`** (`λ=0.01`, `c ∈ {10…45}` ⇒ the raw term is *0.10 B*, which is evidently what was
transposed into the coefficient). Implementing `0.10·c` computes disagreement against a **third
objective, 10x the shipped one**, and K1/K2 would then be measuring the wrong thing. The
proportional term `N·ns·1e-5` is **correct** (`λ·C_us = 0.01·N·ns/1000`). **Fix before dispatch.**

**Two undocumented provenance hazards:**
- `ANVIL_STREAM_LAMBDA` (`:4959`) is `std::atof` with **no validation** and silently overrides the
  pre-registered λ. `--stream-lambda=N` (`:4977`) is `std::stod`, also unvalidated (negative and
  huge values accepted). **The arbiter must assert the env var is unset and record the effective
  λ per arm**, or a contaminated runner silently invalidates the pre-registration.

**`--hotop-budget` semantics — exact, and there is a live footgun.** `:4979`
`opt.hotop_budget=(a.substr(15)!="off")`. Therefore **every value except the literal string `off`
turns the budget ON**: `--hotop-budget=0`, `=false`, `=no`, `=OFF`, and `--hotop-budget=` (empty)
all enable it. Mitigating: bare `--hotop-budget` with no `=` fails the `rfind` and reaches `:4996`
`throw std::runtime_error("unknown option: "+a)` — a **fatal** error, so the omission case is
loud, not silent. The `(x != "off")` convention is uniform across ~15 flags in this CLI, so this is
a **systemic convention, not a one-off bug**. **Binding rule for the arbiter: use flag ABSENCE or
the exact literal `off`; never `0`/`false`/`no`/`OFF`/empty.** Note also `A1` reaches
`encode_stream_budget` at **exactly one call site** (`:2868`, mode-15 book streams only), so A1's
maximum byte influence is structurally bounded by that stream share.

**A third calibrated-λ arm (A2 / §2.4's λ′) — PREMATURE *and* DATA-FIT. Strike it.** Two
independent reasons:
1. **Data-fit.** λ′ is defined from `N_med`, "read off the byte-only stream dump" — i.e. from the
   **evaluation corpus's own** stream statistics. That is parameter selection on the evaluation
   set, which doctrine item 5 forbids in substance even if λ′ is frozen before the timing arms.
   λ′ must come from a **held-out or pre-existing** measurement. (In-repo priors that would
   qualify: F13's `distance_code` 15,680 B / `match_len_class` 8,788 B.)
2. **Predicted null — arithmetically.** `λ′ = N_med·5.9e-5/30`. At N_med = 8,788 B ⇒ **λ′ ≈
   0.0173**. The first stream flip occurs at **λ ≈ 44.6 B/μs** (`:4008-4010`). λ′ is **~2,580x
   short** of moving a single selection. A2 cannot produce a byte delta or a speed delta.
   Minor overstatement to correct: §2.4 calls this "scale calibration, not re-ranking" — true of
   the *codec ordering*, but a uniform λ rescale **does** re-rank cost against `L`, which is the
   comparison that matters.

**Cost: zero arms, zero builds, zero CI minutes.**

### 12.10 Track 20 and CAM

**Track 20 has issued no CAM-specific determination.** I searched
`20-priorart-killteam-space-bunny.md`: its S1-S4 framework covers FLI (track 11), transmitted vs
derived parameters, and the reference-local-placement separator; **CAM appears nowhere**. Flagged
as a coordination gap, not a criticism of track 20.

The **closest applicable Track 20 material** is its §2 principle: *"`k` scalars, **transmitted once
per block** ⇒ prior art (`gate-priorart-audit-i8.md` §4)."* That transfers directly and
adversarially to **CAM-1** (a transmitted per-block model prior) — it is prior art, full stop, and
Track 11's `M4` already recorded transmitted parameters as foreclosed twice. It does **not**
transfer to **CAM-0**, whose prior is derived from the block's own decoded prefix and transmits
nothing; CAM-0's novelty question is untouched by Track 20 and remains the §11.6 question (my own
classification: adopt-class, on the periodic-restart / estimation-noise literature, not on any
transmitted-parameter art).

So the CAM split sharpens: **CAM-1 is prior art by Track 20's own transmitted-scalars principle;
CAM-0 is adopt-class by a different route.** Neither is claimable. This strengthens §11.7's HOLD
and further lowers the odds that CAM-1's 64-1024 B budget ever earns promotion.

### 12.11 Q3 disposition

| element | ruling |
|---|---|
| Q3 Stage 2 / remote timing | **DELETE.** Ceiling 1.003x vs 2.3-8.1% environmental floor; structurally incapable of discrimination (§12.5). |
| Q3 Stage 1, byte-only, on **small blocks** | **DROP as a decision input** — already answered: 792 selections, 0 raw-flips, 0.00% byte delta, measured twice. |
| Q3 Stage 1, byte-only, on **≥1 MiB blocks / large corpora** | **KEEP, narrowed.** The only genuinely unmeasured question: does the defect's **sign flip** above N\* = 5,085 B (§12.6)? |
| K1 argmin-disagreement promote authority | **REMOVE.** Fires at 17.55% against 0.00% bytes — decoupled, already in hand (§12.7). |
| A2 / λ′ calibrated arm | **STRIKE.** Data-fit on the eval set *and* ~2,580x short of any flip (§12.9). |
| A1 budget arm | **RETAIN as a NEGATIVE CONTROL only.** Incapable of a speed delta by construction (§12.3); its result must not be read as evidence for or against the cost-model correction. |
| AOC (R3) | **PILOT, no wire commitment** until all six charges in §12.8 are on the table. |
| CAM (track 15) | **HOLD.** CAM-1 = prior art via Track 20's transmitted-scalars principle; CAM-0 = adopt-class. M5/M6/M7 still mandatory. |
| §3.2 `0.10·c` | **FIX → `0.01·c`** before any dispatch (§12.9). |

---

## 13. Reconciliation with the constructive lane — explicit

The constructive report landed after my first pass. I read it in full and record agreement and
disagreement. **Per §10.5 I pre-committed to reconciling rather than defending.**

### 13.1 Agreed (independently reached, and I confirm)

| # | Constructive position | My position | Status |
|---|---|---|---|
| A1 | Confirms the length-independence defect, re-derives N\* = 5,085 B and the 51x/19.7x table | same (§3.3) | **AGREE** |
| A2 | Unit incoherence; two live objectives; `FORMAT.md:642` labelling bug | same, plus the multiplicative-origin proof (§3.3a) | **AGREE, I add evidence** |
| A3 | R1 new-coder primitive = KILL; FSE is zstd prior art; `DNB-M1` closes decode-only | same (§2.1) | **AGREE** |
| A4 | Anti-overclaim correction: entropy modelling *does* move bytes (F3/F5/F6) | this is the correction I made per coordinator direction (§2.1 revised) | **AGREE — constructively better than my draft** |
| A5 | H-A/H-B/H-C trichotomy; holds **no position** on which branch is live | better than my own §6 framing; I adopt it | **AGREE — I defer** |
| A6 | Struck opcode-pull re-profiling; F18 already answers it | correct; my §7 C1-C7 remain the right protocol for anything *else* | **AGREE** |
| A7 | §3.0's already-measured prerequisites and the +2.3-8.1% noise floor | this floor is what kills Stage 2 in §12.5 | **AGREE, and I act on it** |

### 13.2 Disagreements — resolved against the constructive lane, with reasons

| # | Constructive | Critic | Resolution and why |
|---|---|---|---|
| **D1** | Stage 1 item 1 uses `argmin[L+0.10·c]` | shipped constant is `0.01·c` | **Constructive is wrong.** 10x drift; fixes the wrong objective (§12.9). |
| **D2** | K1: argmin disagreement ≥10% ⇒ PROMOTE | decoupled from bytes; 17.55% vs 0.00% | **Critic.** K1 must lose promote authority (§12.7). |
| **D3** | A2 / §2.4 λ′ calibrated arm | data-fit + ~2,580x short of any flip | **Critic. Strike A2** (§12.9). |
| **D4** | A1 is a promotion candidate | structurally incapable of a speed delta; negative control only | **Critic** (§12.3, §12.11). |
| **D5** | K7 promotes AOC on byte-only `H(op)−H(op\|prev)` ≥0.5% | gross-vs-net; omits the 259+3K B model | **Critic. Diagnostic only** (§12.8). |
| **D6** | Arm table treats `--hotop-budget` as a clean boolean | `!= "off"` ⇒ `0`/`false`/`no`/`OFF`/empty all enable | **Critic. Binding flag rule** (§12.9). |
| **D7** | Provenance hazards listed as C3/C5/C6/F19 | `ANVIL_STREAM_LAMBDA` unvalidated `atof` override omitted | **Critic. Add** (§12.9). |
| **D8** | §3.1 K1/K2 thresholds unchanged | pre-registered against a metric the record already answers | **Critic** (§12.7). |

### 13.3 Net effect on the lane's bottom line

**No disagreement on the ruling.** Constructive §8 and my §10 both land on: **new-coder primitive
KILL; cost-objective correction PROMOTE-TO-REMOTE (shared arbiter); AOC PILOT adopt-class; CAM
HOLD; DDMC withdrawn.** The disagreements are about **what the arbiter should measure**, and they
all point the same way — **the arbiter is over-specified for its expected information yield.**

Concretely, after D1-D8 the cost-objective arbiter reduces to:

- **one byte-only job** (Stage 1) **on large-block inputs**, asking only: *does argmin disagreement
  accompanied by a complete-bytes delta appear above N\* = 5,085 B?*
- **zero timing arms** (§12.5),
- **zero λ-fit arms** (§12.9),
- **plus** one negative control (A1) and the strict provenance set (λ assertion, exact flag strings,
  `ANVIL_STREAM_LAMBDA` unset).

That is a materially smaller job than §3 as frozen, and it is the version I will defend. **If the
constructive lane prefers the larger arbiter, the disagreement is D1/D2/D3 and is resolvable by
the coordinator on the arithmetic above — all three of my objections are checkable without running
anything.**

### 13.4 Status of `Q3-SELECTOR-PREFLIGHT`

**Not present at audit time** (searched `docs/swarm-2026-10-02/`; no matching artifact). I will
audit it against these pre-registered criteria when it lands:

1. Does it use `0.01·c` (correct) or `0.10·c` (drift)?
2. Does it assert `ANVIL_STREAM_LAMBDA` unset and record effective λ per arm?
3. Does it use flag **absence** or literal `off` only — never `0`/`false`/`no`/empty?
4. Does it **delete** the remote timing arm, or attempt to rescue it with reps/pairing?
5. Does it key promotion on **complete-bytes delta** rather than argmin disagreement?
6. Does it drop the λ′ third arm, or derive λ′ from a **held-out** N_med?
7. Does it treat `H(op)−H(op|prev)` as diagnostic-only with the six AOC charges enumerated?
8. Does it record the manifest's provenance gap (no raw `stream_log` artifact checked in)?
9. Does it report byte delta and speed delta **separately**, never as one "delta"?

---

*Prepared by Fledge Alpha Free (independent adversarial reviewer), track 06. Read-only
inspection only: `src/anvil.cpp`, `FORMAT.md`, `RESEARCH_LEDGER.md`, `docs/CONTEXT.md`,
`docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md`,
`docs/swarm-2026-10-02/15-crossblock-memory-space-bunny.md`,
`docs/swarm-2026-10-02/20-priorart-killteam-space-bunny.md`. No files modified, no builds, no
benchmarks, no fuzz, no commits. No heavy local runs performed or recommended.*
