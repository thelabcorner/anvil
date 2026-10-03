# Track 18 — New Decoder/Backend Architecture for Future Formats
## Space Bunny Free — INTERIM checkpoint (written mid-gathering, per coordinator instruction)

**Status:** INTERIM. Mechanism named, evidence mapped, one decisive remote
experiment pre-registered. No code written, no local measurement performed, no
existing file modified.

**Leaning: PILOT** — narrowly, and conditional on the coordinator accepting the
scope argument in §2. If doctrine-12 is read as covering the token lane, the
leaning drops to **HOLD**.

---

## 0. Scope discipline

Track 18 mandate: *data layout, vectorization, superinstructions, independent-stream
scheduling, cache behavior, SIMD-friendly state machines, for future formats.*

Two constraints from the brief bind everything below:

- **C1 (do-not-reburn).** `docs/anvil-i9-findings.md` §6.1/§6.3 and
  `RESEARCH_LEDGER.md` A8/A12/A15/A16/A19 close all I9 *wire-invisible* decode
  work on the **ratio-first BWT portfolio**. Specifically closed: CRC route
  (A3), postcoder cheap edges (A8), materialization (A9), ALLOC (A12),
  two-level-tables+buffered-renorm "leg 4" (A15).
- **C2 (mandate wording).** "…**for future formats**". Track 18 is therefore the
  one lane explicitly chartered to spend *wire* to buy *cycles*, in a way that
  I9's wire-invisible programme could not.

Everything in §3–§5 is either (a) a **wire-visible** architecture change
targeted at the **token/LZ lane** (block modes 10/12/15), or (b) a
**wire-invisible kernel explicitly labelled as non-crossing infrastructure**
that exists only to make (a) profitable. Nothing here re-runs a closed BWT
leg. §2 proves the BWT portfolio is *arithmetically out of reach* for this
track, which is the report's most decision-relevant finding.

---

## 1. Evidence map (measured vs derived — no splicing)

### 1.1 What is already closed (do not reopen)

| fact | value | source |
|---|---|---|
| Wire-invisible decode programme | **EXHAUSTED** for the ratio-first BWT route | `docs/anvil-i9-findings.md` §6.3, §9; `RESEARCH_LEDGER.md` A15 |
| Postcoder token-decode stage share | **91–93 %** of postcoder; postcoder = **52.1 / 36.4 / 37.8 %** of whole-container decode (dickens/webster/enwik8) | `prototypes/i9-decode-perf/POSTCODER-SPEC.md` §1 |
| Cheap postcoder edges (buffered renorm / MTF / materialization) | **1.021 / 1.014 / 1.013** on e2e — `DECODE-SHORT` | same §2 |
| leg-4 (two-level cumulative tables + buffered renorm, wire-identical) | **1.179× aggregate** (1.215 / 1.169) vs ~1.4× required → target falsified | `RESEARCH_LEDGER.md` A15; `prototypes/i9-arch/LEG4-postcoder.md` |
| Stage-correct postcoder requirement `k_p` | **6.05× / 3.86× / 6.60×** | `RESEARCH_LEDGER.md` A16; `POSTCODER-SPEC.md` "Requirement CORRECTION" |
| Materialization / lazy `StreamPull` | **falsified**: generated.json **+1.4 %**, synth-timeseries **+27.6 %** (CV 9.5 %) | `docs/anvil-i9-findings.md` §6.1; A9 |
| Transmitted-static-table postcoder | **NOT AUTHORIZED** (ID3 raw = 5–6.6× faster but **+52–101 % bytes**) | `RESEARCH_LEDGER.md` A15; `POSTCODER-SPEC.md` §3 |
| Reciprocal-rANS (replace hardware divide) | measured **slower** | `docs/audit-2026-09-07/06-do-not-reburn.md` §D3 |
| More hot-op dispatch tuning without a new whole-codec profile | closed | same §D1 |

### 1.2 What is measured and is *live* (the opportunity surface)

| fact | value | source |
|---|---|---|
| Mode-15 (compiled hot-op book) decode gain over fused mode-12 | generated.log **274 vs 234**, json **194 vs 157**, jsonl **284 vs 226**, sqlite **157 vs 118** MB/s = **1.17–1.34×**; pre-registered **≥2× target NOT met** | `FORMAT.md` §"HOTOP token backend (mode 15)" lines 439–445 |
| **The named remaining floor of mode 15** | "the opcode-stream entropy decode + copy throughput" | same, line 444–445 |
| Mode-15 byte cost | "near-neutral on record files (log −0.1 % vs sparse; +0.3–0.7 % elsewhere)" | same, line 446 |
| Token-lane decode requirement, this lane | generated.json **R = 2.44×** (R′ ≈ 2.49); hotop-rlzp **≈1.29×** | `RESEARCH_LEDGER.md` A12 |
| Token-lane decode requirement, bench frozen bar | generated.json **R′ = 2.0699**; synth-timeseries **R′ = 1.5248** | `RESEARCH_LEDGER.md` A19 |
| Measured ALLOC-only decode win (token lane) | generated.json **4.1604 → 3.0766 ms (1.352×)**; synth-timeseries **1.4829 → 1.3802 ms (1.074×)** | `RESEARCH_LEDGER.md` A12 |
| Wire-invisible rANS precedent (format change, charged) | I10-1A aux-unbwt: **+2,494 / +2,535 / +3,054 B** on dickens/webster/enwik8 for **1.364× / 1.795× / 2.339×** whole-codec decode; portfolio **+22,398 B on 311,938,580 B source = +0.00718 %** | `docs/I10-AUX-UNBWT-RESULTS.md` §3–§6 |
| Same, code/memory cost | **+8,192 B** ELF, **+2,976 B** `.text`, RSS effectively unchanged (~599 MiB enwik8 pair) | same §9 |
| I10-1A ruling vs xz | Silesia **1.7226×** [1.6640, 1.8400] slower than xz; enwik8 **3.9065×** [3.9003, 4.0088]; **`FRONT-GAP_COST` both corpora** | same §11 |

### 1.3 Microarchitecture facts this design leans on (all pre-existing in-repo)

| fact | source |
|---|---|
| Zen 3: ~4-cycle load-to-use, **~13-cycle branch-mispredict penalty**, 4 int pipes, 3 AGUs; 256-bit `MOVMSK` = 1 modeled vector op | `docs/FRONTIER-RESEARCH-ADDENDUM-2026-09-23.md` §10.1 |
| **`VPGATHERDD ymm` on Zen 3 = 39 executed µops, ~8-cycle throughput** → vector width does *not* make random access cheap; several interleaved scalar loads beat one wide gather | same §10.3 |
| One-state rANS "has little useful AVX2 width"; ctx-rANS adds a *second* serial dependency (prev symbol → model) and is "low-EV" to vectorize without a format change | same §10.5, §10.10 |
| zstd's 4-stream Huffman literal form: ~**7.3 B** extra for exposed ILP — the canonical "spend a few bytes to break a dependency chain" precedent | same §10.7; `facebook/zstd/doc/zstd_compression_format.md` |
| `rans_static` ships manually vectorized **8/16/32-state AVX2** variants; all such implementations use **branchless/masked renormalization** | same §10.5; `docs/FRONTIER-RESET-2026-09-23.md` §5.3 |
| `libdeflate`: known output extent + whole-buffer operation is worth 4 documented reasons; streaming API explicitly avoided because it slows fast paths | same §10.11 |
| False dependencies from partial-register writes have *serialized* otherwise-independent Huffman streams in production (klauspost/compress `629c2ea`) | same §10.8 |
| `libsais` inverse-BWT is memory/dependency limited; aux indexes won by exposing **more independent starting points**, not by making LF cheaper | same §10.9; `docs/I10-AUX-UNBWT-RESULTS.md` |
| Zen 3 is the reference target; **AVX-512 must not be a format assumption** | `docs/FRONTIER-RESEARCH-ADDENDUM-2026-09-23.md` line 543; `docs/FRONTIER-RESET-2026-09-23.md` §5.10 |

### 1.4 Source anchors in the live decoder

| what | where |
|---|---|
| one-state rANS decode; `while (x < sp.L) { x = (x<<8) \| *q++; }` — **the data-dependent renorm branch** | `src/anvil.cpp:1194`, loop at `:1201–1205`, branch at `:1204` |
| rANS encoder starts every stream at the canonical state `x = sp.L` (**this is what makes lane states free — see §4.1**) | `src/anvil.cpp:1178–1180`, emitted 4-byte state at `:1187–1191` |
| ctx-rANS decode: serial state recurrence **plus** `prev → ctx` dependency | `src/anvil.cpp:1328`, ctx lookup at `:1340`, same renorm branch at `:1345` |
| `StreamPull` — 8 codec modes in one fat state object; per-stream model + `symtab` | `src/anvil.cpp:1900–2019` |
| mode-15 fused hot loop: 9 substreams, opcode pull, `memcpy` for `dist ≥ len`, **byte-wise** loop for overlap, POPCOUNT/TZCNT residual patch | `src/anvil.cpp:2989`; copy at `:3047–3049`; patch at `:3091–3098` |
| `symtab.assign(spec.tot, 0)` + O(4096) refill **per stream per block** | `src/anvil.cpp:1959–1960`, `:1333–1336`, `:1198–1199` |
| ALLOC decode-into-output-slot (retained 1.352×) | `RESEARCH_LEDGER.md` A12; `prototypes/i9-arch/MAT-ALLOC-leg.md` |

### 1.5 Hard decoder invariants a new format must inherit

`docs/decoder-audit.md` (all `[FIXED]`/`[CLOSED]`, landed): `total ≤ (in.size()/7+2)*2^26`;
per-substream `raw_n ≤ 16*out_len + 64`; `sum(freq) == tot`; rANS state ≥ 4 B;
full substream consumption (`q == qe`); trailing-payload rejection; CRC-32 over
reconstructed bytes; overlapping copy is always in-range because `dist ≤ out.size()`
is checked *first*. **Any lane-partitioned stream must add a new strictness class:
lane lengths must sum exactly to `raw_n`, and a mismatch must be a clean reject,
never a pad.**

---

## 2. The finding that should redirect the track: an Amdahl ceiling on the BWT portfolio

**Derived** from recorded shares (`RESEARCH_LEDGER.md` A16 table; `POSTCODER-SPEC.md`
§1 gives a slightly different postcoder share — both are recorded, neither is
spliced):

Using A16's shares, aux `k_u` (3.40 / 3.49) and leg-4 stage `k_p` (1.674 / 1.733):

| file | s_p | s_u | other | postcoder after legs | unbwt after aux | implied total speedup |
|---|---:|---:|---:|---:|---:|---:|
| dickens | 0.439 | 0.502 | 0.059 | 0.2622 | 0.1476 | 2.133× (A16 records 2.134×) |
| webster | 0.342 | 0.610 | 0.048 | 0.1973 | 0.1748 | 2.380× (A16 records 2.379×) |
| enwik8 (s_u est.) | 0.378 | 0.582 | 0.040 | 0.2262 | 0.1712 | 2.286× (A16 records 2.286×) |

The reconstruction reproduces A16's recorded end-to-end factors to 3 significant
figures, which is the validity check for the arithmetic.

**Consequence — the ceiling.** After I10-1A + leg-4, the postcoder is
**51.7 %** (enwik8) / **55.9 %** (dickens) / **47.0 %** (webster) of the residual
decode time. Therefore:

> even an **infinitely fast** BWT postcoder caps the whole-codec gain at
> **1.93× (enwik8) / 1.79× (dickens) / 1.88× (webster)**.

Recorded requirements to reach xz decode parity from the aux state:
**3.9065×** (enwik8) and **1.7226×** (Silesia aggregate)
(`docs/I10-AUX-UNBWT-RESULTS.md` §11).

> **enwik8 needs 3.91×; the ceiling is 1.93×. The ratio-first BWT portfolio is
> therefore provably out of reach for any decoder-architecture work, including a
> perfect one. This is arithmetic on recorded shares, not an estimate.**

Silesia's 1.72× aggregate is not formally excluded by the same bound, but the
BWT-routed share is 56.79 % of Silesia (`docs/audit-2026-09-07/06-do-not-reburn.md`
§F0.1) and the per-file deficits are all ≥1.7×, so it is out of practical reach
too.

**Action:** track 18 must target the **token/LZ lane (modes 10/12/15)**, where
ANVIL already holds measured byte wins and where a decode deficit of
**1.53×–1.80×** (computed next) is plausibly closable. Any track-18 proposal that
targets the BWT portfolio should be rejected on this arithmetic without further
work.

### 2.1 The token lane's actual, residual bar

Residual requirement after the retained ALLOC win (1.352× on generated.json):

| bar | required | measured | **residual needed** |
|---|---:|---:|---:|
| bench frozen `R'` (A19) | 2.0699× | 1.352× | **1.531×** |
| this-lane `R` (A12) | 2.44× (`R'`≈2.49) | 1.352× | **1.805×** |
| synth-timeseries hotop-rlzp (A12) | ~1.29× | 1.074× | **1.201×** |

> **The falsifiable target for track 18 is: ~1.5–1.8× more whole-cell decode on
> the token lane, at ≤ +0.5 % compressed bytes.**

That band is the whole programme. Everything below is aimed at it.

---

## 3. Falsifiable mechanism (three legs, each independently killable)

### 3.0 The diagnosis (the load-bearing claim)

> **H-DIAG.** In ANVIL's rANS decode loop the per-symbol
> `while (x < sp.L) { x = (x<<8)|*q++; }` (`src/anvil.cpp:1204`) is an
> **unpredictable data-dependent branch**, and on Zen 3 its mispredict penalty
> (~13 cycles, Addendum §10.1) — not L1 bandwidth, not the state recurrence — is
> the dominant per-symbol cost.

This is a *diagnosis*, not a speed claim, and it is falsifiable by the §6
experiment. Component estimates, all labelled derived:

- dependent-load chain per symbol: `symtab[slot]` → `freq[sym]` → `start[sym]`,
  which live in **three separate 512-byte arrays** (`src/anvil.cpp:1147–1150`),
  ≈ 3 × 4 cycles = 12 cycles if fully serialized;
- expected renorm bytes/symbol ≈ entropy(symbols) − `scale_bits`; for an
  ~5–7 bit/symbol distribution with `scale_bits = 12`, that is **0 – 1 byte per
  symbol**, i.e. a branch taken somewhere in 0–100 % of symbols — unpredictable
  over that range;
- mispredict contribution at p≈0.5: **+6.5 cycles/symbol**.

Both contributions are of the same order (12 vs 6.5 cycles), which is exactly why
the experiment must be a 2×2 factorial rather than a single A/B.

**Sharpest self-criticism, stated up front (see §8.2):** the *recorded* mode-15
floor is the **opcode** stream, and the hot-op book makes opcodes nearly
deterministic (~1 bit/opcode, 75–77 % coverage per
`docs/CONTEXT.md` Linux notes). If opcode entropy is ≈1 bit/symbol then renorm
is rare *and* well-predicted, so **H-DIAG may be true for literals and false
for opcodes** — i.e. false for the exact stream the floor names. §6 therefore
deliberately runs the factorial **on opcode streams**, so this kills or confirms
the mechanism where it matters.

### 3.1 Leg L1 — lane-partitioned entropy streams ("independent-stream scheduling")

Transpose every entropy-coded stream into `L = 4` **independent rANS states over
the same model and the same alphabet**, lane `j` carrying symbols `i` with
`i mod L == j`. Physical storage is transposed (lane 0's renorm bytes, then lane
1's, …).

**Why the states are free (grounded, not assumed):** ANVIL's encoder already
starts every rANS stream at the canonical state `x = sp.L` and writes that state
explicitly (`src/anvil.cpp:1180`, `:1187–1191`). rANS is exactly invertible, so a
lane is just another stream over a permuted symbol sequence. **No per-lane
initial state needs to be transmitted at all** — each lane re-inits to `sp.L`.
The only new wire is `L−1` lane lengths (the last is implied by the existing
`raw_n`), because the symbol counts per lane are data-dependent.

**Mechanism effect:** one serial recurrence of depth 1 becomes 4 recurrences that
the OoO engine overlaps. Combined with L2 it removes the branch that currently
serializes them. This is exactly zstd's 4-stream-Huffman ILP argument
(Addendum §10.7) and `rans_static`'s 8/16/32-state practice (§10.5) — **the
individual idea is adopt-class prior art and I claim no novelty for it.**

**Explicitly excluded:** the context-switched literal coder (`src/anvil.cpp:1328`).
It carries a `prev → ctx` dependency (§10.10); partitioning it would not remove
that. Do not attempt it.

**Explicit anti-pattern:** lanes must do **4 independent scalar table probes**,
never a `VPGATHERDD` (§10.3: 39 µops on Zen 3).

### 3.2 Leg L2 — branchless bounded renormalization

Replace the renorm `while` loop with a fixed-depth, branch-free ladder: one
unaligned 4-byte speculative load, then `scale_bits = 12` bounds the cascade at
**2 bytes**, so two `cmov`s finish it. Requires ≤3 B of readable slack past the
lane end (a sentinel or a guarded tail).

**Wire cost: zero bytes.** **This is therefore explicitly *not* a crossing route
on its own** — it is admitted as infrastructure whose only purpose is to make
L1 pay. All branchless-renorm implementations in the `rans_static` family do this
(§10.5) — adopt-class.

Adverse precedent, stated plainly: the closest already-measured analogue is leg-4's
**buffered renorm**, which returned **1.04×** on the *arithmetic* coder
(`POSTCODER-SPEC.md` §2). That is direct evidence against optimism here. The
defence is that leg-4's buffered renorm *added buffering* rather than *removing a
branch*, and that the arithmetic coder's `bit()` has a different (bit-granular,
much more predictable) profile than rANS byte-renorm. This defence is a hypothesis.

### 3.3 Leg L3 — fixed-extent copy superinstructions (near-zero wire)

Widen the mode-15 hot-op **book** with entries whose `len` is restricted to
`{4, 8, 16, 32}` and whose `dist ≥ len`, so the hot kernel becomes a short
sequence of **fixed-width stores** instead of a variable-length `memcpy` call
(`src/anvil.cpp:3047`). The book entry format already carries `(kind, len, shape)`
(`FORMAT.md` line 427), so a width-restricted entry costs **4 bytes** and
replaces a general entry — the book header grows by `4 × Δentries` per block only.

Also targeted by L3, at zero wire: the overlap branch. `src/anvil.cpp:3048` copies
byte-by-byte when `dist < len`; the `dist ∈ {1,2,4}` classes have fixed
semantics ("repeat one byte", "repeat a 2-byte pattern") exactly as LZ4's
construction documents (`FRONTIER-RESEARCH-ADDENDUM` §10.10, lz4.c wild-copy).
Specializing the *semantic operation* rather than vectorizing a generic loop is
the zlib-ng lesson (§10.4).

**Wire cost:** `4 B × Δentries` per block; `Δentries = 64` → **256 B per block**.

### 3.4 Leg L4 — extent-known lowering (precondition, not a separate cost)

`libdeflate`'s documented lesson (§10.11): a *declared* output extent is an
optimization input, not just safety metadata. ALLOC already gives whole-cell
decode a known destination (A12). L3's fixed-width stores need only the tile's
extent to be known, which ALLOC supplies. **L4 therefore costs nothing extra and
is listed as a dependency satisfied by a retained win, not a new claim.**

### 3.5 Leg L0 — free but non-crossing: bulk `symtab` build

Every rANS stream builds a 4096-byte inverse table by an O(4096) byte-fill
(`src/anvil.cpp:1959–1960`; same at `:1198–1199`, `:1333–1336`). Since
`Σ freq = tot` over ≤256 symbols, the average run length is ≥16, so an
8-byte-store run-fill (or PDEP-style expansion) is 4–8× fewer stores.
Per 64 KiB block × 9 streams this is ≈36.9 K byte-stores ≈ **0.56 stores per
output byte**. This is the cheapest unharvested item in the decoder and it is
**wire-invisible, therefore explicitly not a crossing route**; it is listed so
that the §6 experiment's baseline is honest about what the reference arm already
lacks.

---

## 4. Encoder / decoder state and complete cost accounting

### 4.1 State

| | encoder (new) | decoder (new) |
|---|---|---|
| rANS states | `L` per stream (transposed encode) | `L` per stream |
| models | **shared, unchanged** — no new model bytes | **shared, unchanged** |
| `symtab` | — | **unchanged** (one per stream, not per lane) |
| book | `+Δentries` entries, `len ∈ {4,8,16,32}` | same table, wider |
| copy overlap state | none | none (specialized kernels) |
| **new persistent state** | `4 × RansState` ≈ **64 B** | `4 × RansState` ≈ **64 B** |
| **parse change** | **none** — byte comparison is clean | — |

### 4.2 Complexity

| leg | encode | decode | wire | new state |
|---|---|---|---|---|
| L0 `symtab` bulk fill | — | `O(tot/8)` instead of `O(tot)` per stream/block | 0 B | 0 |
| L1 lanes `L=4` | `O(L·n)` naive, `O(n)` with a permuted single pass | `O(1)`/symbol, ILP depth `L` | **`(L−1)` uvarints ≈ 2–6 B per partitioned stream per block** | 64 B |
| L2 branchless renorm | — | `O(1)`/symbol, **branch count 1 → 0** | **0 B** | ~8 B stack |
| L3 superinstructions | O(n) book build | `O(out/32)` vs `O(out)` ops on hot runs | **256 B per block** | 0 |
| L4 extent-known | — | enables ≥32 B-granular stores | **0 B** | 0 |

### 4.3 Byte accounting — the hidden per-block cost, charged honestly

L1's cost is **per stream per block**, so it scales as `blocks⁻¹`. Charging it:

| block size | streams partitioned | charge | on `generated.log` (~104 KB compressed, 1.85 MB source) | on enwik8 aux (23,537,422 B) |
|---:|---|---:|---:|---:|
| 64 KiB | 9 | ~5 B × 9 = 45 B/block | 28 blocks → **1.26 KB = +1.21 %** ❌ | 1526 blocks → **69 KB = +0.29 %** |
| 256 KiB | 9 | 45 B/block | 8 blocks → 0.36 KB = **+0.35 %** | 382 blocks → 17 KB = **+0.07 %** |
| 1 MiB | ≥1 (size-gated) | ~5 B/block | 2 blocks → **+0.01 %** ✅ | 92 blocks → **+0.02 %** ✅ |

**Therefore L1 must be gated on stream decoded-size, not applied uniformly.**
Pre-registered rule: *partition a stream only when `raw_n ≥ 64 KiB` and
`block_size ≥ 256 KiB`.* Under that gate the charge is **≤ +0.35 %** on the
smallest relevant corpus and **≤ +0.02 %** at 1 MiB blocks. Combined package
budget: **≤ +0.50 %** compressed bytes (§6, G5).

This per-block scaling is exactly the class of hidden cost the brief demands be
charged, and it is the single most likely reason a naive L1 implementation would
have been rejected on bytes.

### 4.4 Cycle accounting (derived; all components labelled)

Per decoded symbol, target stream, Zen 3:

| | A0 today (derived) | A3 target (derived) |
|---|---|---|
| dependent loads | 3 × ~4 c = 12 c | 3 × ~4 c, **overlapped across L=4 lanes** → ~4 c amortized |
| renorm branch | mispredict p ≈ 0–1 × ~13 c → **0–13 c** | **0 c** (2 cmovs, ~2 c) |
| ILP available | 1 chain | 4 chains |
| **amortized per symbol** | **~12–25 c** | **~6–8 c** |
| implied rate at 4 GHz | 160–330 MB/s | **500–660 MB/s** |

Recorded corroboration that the A0 end of this band is real: mode-15 measured
**157–284 MB/s** across four corpus files (`FORMAT.md` line 441) and the ctx-rANS
literal subsystem measured **~289–320 MB/s** (`docs/CONTEXT.md` Linux frontier
notes). **These are Linux-host numbers from a different session and are cited as
band corroboration only, never spliced with Windows I9 numbers.**

### 4.5 Memory / RSS

- L1–L4 add **no output-sized allocation**. Lane states are 64 B/stream.
- Expected decode peak-RSS delta: **0**. This is stated as a *prediction with a
  gate*, not a fact: the remote experiment must record per-process peak RSS and
  require the candidate/control ratio ≤ 1.02.
- Contrast worth recording: the BWT subblock sweep bought RSS at **+1.0 % to
  +5.4 % bytes** (`docs/I10-AUX-UNBWT-RESULTS.md` §11). Track 18's package buys
  speed at **~0.2 %** expected bytes and 0 expected RSS. That asymmetry is the
  economic case for the token lane.
- Decoder binary: aux-unbwt cost **+2,976 B** `.text` / **+8,192 B** ELF for a
  substantially larger feature (§9 of the same doc). Track 18's package is
  estimated **+1.5–3.0 KiB** `.text`, against the AITDCC **≤1 MiB decompressor**
  constraint (current stripped combined binary **453,512 B**) — headroom is not
  at risk, but the number must be measured, not asserted.

---

## 5. Novelty / prior-art risk (honest)

| element | class | prior art |
|---|---|---|
| multi-state rANS / lane partitioning | **adopt-class** | `ryg_rans`; `rans_static` (8/16/32-state AVX2); zstd 4-stream Huffman (~7.3 B for ILP) |
| branchless/masked renormalization | **adopt-class** | every `rans_static` variant; zlib-ng dispatch architecture |
| fixed-extent copy specialization by distance class | **adopt-class** | LZ4 wild-copy (`dist=1,2,4` constructions); zlib-ng inflate chunk kernels |
| fixed-width multi-symbol Huffman / wider decode tables | **adopt-class** | mature codecs; PivCo-Huffman (`arXiv:2606.05765`) for the block-SIMD version |
| integer side-streams (FOR/PFor/Stream-VByte) | **adopt-class** | `arXiv:1709.08990`; TurboPFor |
| hardware-first representation design | **adopt-class** | ALP (`doi:10.1145/3626717`), FastLanes, Parquet ALP (2026-09) |
| **the interaction**: *a single token lane where (a) lane transposition is applied to the opcode+literal streams, (b) the renorm is branch-free so lanes actually overlap, (c) copy superinstructions are width-restricted to the same tile grid, and (d) all three are charged against one ≤0.5 % byte budget with an extent-known destination* | **unestablished** — this is the only place a novelty claim could live, and it is a *composition* claim, which the project's own doctrine treats sceptically (`docs/CONTEXT.md` novelty gate: "an arbitrary combination of known components is NOT sufficient") | — |

**Verdict on novelty: the individual legs are adopt-class and I do not claim
novelty for any of them.** The composition claim is exactly the kind of
"novelty-by-difference" the brief's doctrine-1 forbids relying on. Track 18's
defensible output is therefore **not a novelty claim** — it is (i) the Amdahl
exclusion of §2, (ii) a falsifiable microarchitectural diagnosis, (iii) a
measured cost model for a cheap Pareto knob that the project has never tried, and
(iv) a pre-registered falsification of that diagnosis. **Track 20 (kill team)
should be asked to rule on the composition claim before any of this is described
as novel.**

---

## 6. The one decisive remote-only experiment (pre-registered)

**Name: D1 — "renorm/lane factorial on real ANVIL opcode streams."**
Vehicle: GitHub Actions, `workflow_dispatch`, `ubuntu-24.04`, one job, one
checkout, one binary — per `docs/github-actions-benchmark-protocol.md` §2, §4
(`microarch` class), §5, §6, §8. **No local run. No local corpus benchmark. No
local fuzzing.**

### 6.1 Inputs (deterministic, in-repo-reproducible)

Two **opcode** symbol traces and one synthetic control, captured from the shipped
mode-15 encoder path:

- **T1** — `tests/corpus/generated.log`, regenerated by the pinned
  `tests/make_synth_corpus.py`, SHA-256 verified against `tests/corpus/CHECKSUMS.txt`.
- **T2** — `tests/corpus/generated.json`, same procedure.
- **T3** — synthetic geometric distribution at ~6 bits/symbol, pinned seed,
  same total symbol count (guards against a trace that is too regular to be
  representative).

T1/T2 are chosen as **opcode streams, not literal streams**, deliberately: the
recorded mode-15 floor names the opcode stream, and the risk in §3.0 is that the
diagnosis is false precisely there. If it is false there, the leg dies, and it
should die cheaply.

### 6.2 Arms (2×2 factorial + null, one binary, output-hash identical)

| arm | states | renorm | purpose |
|---|---:|---|---|
| **A0** | 1 | branchy (as shipped, `src/anvil.cpp:1204`) | reference |
| **A1** | 1 | branchless 2-deep cmov ladder | isolates **L2** |
| **A2** | 4 | branchy | isolates **L1** under branchy |
| **A3** | 4 | branchless | the composed candidate |
| **A4** | 4 (AVX2 `__m256i`, 4×32-bit) | branchless, **per-lane scalar table probes, no gather** | tests whether AVX2 earns its dispatch cost |
| **A5** | = A0, second symbol name | = A0 | **A/A null** |

All arms consume **byte-identical** streams. Per-arm SHA-256 of the decoded symbol
stream must be identical to A0; any difference is a hard failure, not a warning.
A4 must also assert its scalar fallback equals the vector path.

### 6.3 Metrics

`ns/symbol` and (for context only, not as a claim) `ns/output-byte`; paired
`log(t_cand/t_ctrl)`; median; MAD-robust CV; seeded bootstrap 95 % CI; all raw
reps retained; measurement order retained. Runner fingerprint per protocol §3,
including `avx2 bmi1 bmi2 pclmulqdq popcnt sse4_2 avx512*`. Ambient-load gate
(§6): `BLOCKED_AMBIENT` ⇒ neutral, rerun.

**PMU honesty clause, pre-registered:** hosted runners usually do not expose
reliable branch-miss counters. If they do not, **the mispredict *attribution*
remains UNRESOLVED even if the speedup is real**. In that case a speed win
satisfies G4 but **may not be used to support any mechanism or novelty claim**.

### 6.4 Pre-registered thresholds (fixed before any run; no post-hoc movement)

**G0 — validity (all required, else the run is void):**
identical output SHA-256 for A0–A4 on T1/T2/T3; A5 null CI spans 1.0; max arm
robust CV ≤ 15 %; ambient gate PASS.

**G1 — L2 (branchless renorm) KILL criterion:**
`A1/A0 ≥ 1.15` with CI lower bound ≥ 1.10 **on both T1 and T2**.
*Otherwise L2 is KILLED and H-DIAG is falsified for the stream that matters.*

**G2 — L1 (lanes) KILL criterion:**
`(A2/A0) ≥ 1.10` **or** `(A3/A1) ≥ 1.10`, CI lower bound ≥ 1.05, on both T1 and T2.
*Otherwise L1 is KILLED as a rate-cheap lever.*

**G3 — AVX2 disposition (not a kill):**
if `A4 ≤ 1.05 × A3`, **drop AVX2** and ship scalar-4-lane. Rationale: §10.3's
39-µop gather figure and the AITDCC decoder-size constraint both favour the
smaller decoder when the vector arm does not pay.

**G4 — PROMOTE to a full-codec pilot** *(superseded before any data by §12.2;
the share-conditioned form G4′ in §12.5 is binding, not this one)*:
all of G0, G1, G2 pass, **and** the best composed entropy arm reaches
`≥ 1.60×` vs A0 with CI lower bound ≥ 1.40× on **at least one** of T1/T2.

> **Withdrawn as mis-sized.** A flat 1.60x *kernel* threshold is not a
> whole-cell decision. §12.3 derives the binding rule:
> `k_ED >= s_ED / (s_ED - 0.3468)`, where `s_ED` is the measured
> entropy+dispatch share from Track 11 P1c. At any plausible `s_ED` a flat
> 1.60x gate is either trivially satisfied and useless, or unmeetable.
> Replaced by **G4'** in §12.5.

**G5 — STOP the entire leg:**
if no arm reaches 1.60× on either trace, the token lane cannot be closed by
entropy work. Track 18 then re-scopes to L3 (copy superinstructions) alone on its
own merits, or is **KILLED**.

**G6 — stage-2 byte + whole-cell gate (only after G4; pre-registered now):**
the full package (L1 gated per §4.3 + L2 + L3) on `generated.log` and
`generated.json` must cost **≤ +0.50 %** compressed bytes versus the identical
parse with the package off, deliver **≥ 1.35×** whole-cell decode, and keep
peak-RSS ratio ≤ 1.02. Failing any of the three **kills the package** even if D1
passed — a microbench win does not survive a byte-budget failure.

**G7 — external-class gate (only after G6):**
a win measured only on `generated.log`/`generated.json` is `{synthetic}`-class
(these are repo-generated; see `docs/audit-2026-09-07/06-do-not-reburn.md` §C1,
§C2). Confirmation is required on `tests/corpus/doc.md`, `tests/corpus/src.cpp`
and the PE family before any frontier language is used.

### 6.5 Explicitly out of scope for D1, with reasons

- **BWT postcoder / `bwt_arith_decode`** — Amdahl ceiling §2; already closed by
  A8/A15/A16.
- **Context-switched literal coder** (`src/anvil.cpp:1328`) — serial by
  construction (§10.10).
- **CRC** — already PCLMUL, a signed wire-identical win
  (`docs/anvil-i9-findings.md` §6.4).
- **Threaded decode** — `libsais_unbwt_omp` already falsified as a no-op
  (`docs/anvil-i9-findings.md` §6.1); this track is single-thread by contract.

### 6.6 The reopen-condition statement the coordinator must rule on

D1 does **not** re-run any closed I9 leg: it targets the **token lane**, not the
BWT postcoder; its central leg is **wire-visible** (lane transposition); and its
one wire-invisible component (L2) is **explicitly barred from carrying a crossing
claim** and exists only to make the wire-visible leg pay. If doctrine-12 is read as
covering the token lane, then D1 is demoted to infrastructure-only and **cannot
support a frontier claim** — in which case the correct recommendation is HOLD,
and I will not argue otherwise.

---

## 7. Algorithmic complexity of the whole-codec picture

For a block of `n` output bytes on the token lane:

| stage | today | with package | note |
|---|---|---|---|
| entropy decode | `Θ(n)` symbols, ILP depth **1** | `Θ(n)` symbols, ILP depth **4** | constant-factor only |
| token dispatch | `Θ(1)`/token, **data-dependent branch per token** | `Θ(1)`, width-restricted | no asymptotic change |
| copy | `Θ(len)`, byte loop when `dist < len` | `Θ(len/32)` wide stores on hot classes | constant factor |
| `symtab` build | `Θ(4096)` per stream per block | `Θ(512)` wide stores | **this is `Θ(1)` per block and dominates on small blocks** |
| memory | `Θ(n)` output, no extra | `Θ(n)`, no extra | RSS delta 0 (predicted) |

No asymptotic improvement is claimed anywhere. The entire claim is a
constant-factor microarchitectural one, which is the correct and honest framing
for a track whose predecessors have already extracted the algorithmic wins.

---

## 8. Strongest disconfirming evidence (adversarial, ranked)

**8.1 The BWT exclusion may be read as "track 18 has no target."** If the
coordinator's success criterion is the canonical ratio-first portfolio, §2 shows
this track cannot reach it at all, and PILOT is wrong. **This is the strongest
argument against my own recommendation.**

**8.2 The diagnosis may be false for opcodes.** The named mode-15 floor is the
opcode stream, and the hot-op book drives opcode entropy toward ~1 bit/symbol.
At ~1 bit/symbol with `scale_bits = 12`, renorm is *rare* and *predictable*, so
there is little branch to remove and few lanes to interleave. **This could kill
both L1 and L2 on the only stream the project has identified as the floor.** §6 is
built to detect exactly this, cheaply.

**8.3 The closest measured analogue underdelivered by 3–5×.** leg-4's buffered
renorm + two-level tables returned **1.179×** aggregate against a **~1.4×**
requirement, and the stage-correct requirement is **3.86–6.60×**
(`RESEARCH_LEDGER.md` A15, A16). The same class of change on the arithmetic coder
did not come close. This is the strongest direct empirical prior against L2's
predicted 1.4×.

**8.4 "Bulk + parallel beats per-symbol pull" has already failed once, in the
adverse direction.** Materialization/lazy `StreamPull` was **+1.4 %** on
generated.json and **+27.6 %** on synth-timeseries (`docs/anvil-i9-findings.md`
§6.1; A9). L1 and L3 both move work toward "decode more, eagerly, in parallel."
The counter-argument is that L1 parallelizes *across lanes* rather than across
*streams*, so it should not inherit A9's failure — but that is an argument, not
evidence.

**8.5 SIMD may be a trap on this host class.** `VPGATHERDD ymm` = 39 µops on
Zen 3 (§10.3). Any AVX2 arm that gathers symbols will lose. §6.2 forbids it in
A4 and G3 exists precisely to discard AVX2. A "vectorized decoder" narrative
could still be net-negative in decoder size (AITDCC ≤1 MiB) for no gain.

**8.6 False dependencies can defeat lane interleaving at the µop level.**
Partial-register writes have serialized independent Huffman streams in production
before (klauspost/compress `629c2ea`, §10.8). Source-level independence is not
machine-level independence. Mitigation is pre-registered: collect disassembly in
the remote job and inspect the dependency graph (protocol §4 `microarch`).

**8.7 Per-block wire scaling could kill L1 on bytes** even at 4× speed (§4.3).
At 64 KiB blocks the charge is **+1.21 %** on `generated.log`, over budget. The
size-gate in §4.3 is the mitigation and it is itself unvalidated.

**8.8 Corpus-class risk.** `generated.log`/`generated.json` are repo-generated
synthetic families; §C1/§C2 of the do-not-reburn register records that wins there
have repeatedly failed to transfer. Pre-registered as G7. Any D1/G4 result must
carry `{synthetic}` until G7 passes.

**8.9 Book widening could backfire.** More width-restricted entries dilute the
opcode alphabet, raising per-opcode cost; and on small blocks the 4 B/entry header
grows. Mode-15's own ratio was already "+0.3–0.7 % elsewhere" for the base book
(`FORMAT.md` line 446). L3's 256 B/block must be measured against that baseline,
not assumed negligible.

**8.10 The mechanism may be right and the ledger may still say NO.** Doctrine 12
is broad on its face. §6.6 is my honest statement of the boundary and I am not
claiming the ruling.

---

## 9. Minimum prototype (only if G4 promotes — not started)

Isolated, under `prototypes/swarm-2026-10-02/18-future-decoder-architecture/space-bunny/`:

1. `trace_dump.cpp` — deterministic extraction of T1/T2 opcode symbol traces from
   the shipped mode-15 encoder path (no new compression logic).
2. `dec_arms.cpp` — the six arms of §6.2 in **one** binary, sharing the model
   builder and the symbol table; `assert(sha256(arm) == sha256(A0))` per trace.
3. `d1_report.py` — paired log-ratio, robust CV, seeded bootstrap CI, raw-rep CSV,
   fingerprint block. Reuse the protocol's existing driver
   (`tools/paired_bench.py`) rather than writing a new harness.
4. A **scalar-only fallback path** compiled unconditionally so A4's fallback is
   testable and so the shipped decoder has no hard AVX2 dependency.
5. **No** local execution. Build and run happen only in the remote job; locally
   only static review.

Explicitly excluded from the prototype: any `src/anvil.cpp` edit, any `FORMAT.md`
edit, any new block mode. A format proposal (mode 18) would be a **separate,
later** document with its own registry-coverage and fuzz obligations
(`docs/decoder-audit.md` conventions), authored only after G6.

---

## 10. Recommendation

**PILOT** — authorize **D1 only** (the §6 remote factorial), on these conditions:

1. Scope is the **token lane** (modes 10/12/15). The BWT portfolio is excluded by
   the §2 Amdahl arithmetic and no track-18 work should be funded there.
2. All thresholds in §6.4 are pre-registered before the run, and no threshold
   moves after data is seen.
3. D1's outcome is allowed to be **negative**, cheaply. Cost of the experiment is
   one remote job; cost of a wrong belief is one day's reading.
4. **No wire-visible change is authorized by D1.** G6 is a separate gate; a mode-18
   format proposal is out of scope until G6 passes.
5. **No novelty claim** is authorized on the composition (§5). Track 20 rules on
   novelty separately or not at all.

**Downgrade to HOLD** if the coordinator rules that doctrine-12 covers the token
lane (§6.6), or if the coordinator requires a novelty claim as a precondition for
spending track effort (§5 says none is available).

**Upgrade to PROMOTE-TO-REMOTE** only on G4 **and** G6 **and** G7, with the
whole-cell number (≥1.35× decode at ≤+0.50 % bytes on a non-synthetic corpus) as
the promotion artifact.

**Kill conditions standing before any further work:** G1 fails, or G2 fails, or
G5 (no arm ≥1.60×). Under any of those, track 18 closes as a measured negative and
the residual 1.5–1.8× is reallocated.

---

## 11. Provenance and honesty notes

- **No measurement in this document was produced by me.** Every number carries
  its source file and section. Nothing is spliced: I9 Windows numbers, I10
  GitHub-Actions Linux numbers, and `docs/CONTEXT.md`'s historical Linux numbers
  are cited separately and never combined into one figure. The §2 Amdahl
  derivation uses only A16's own share/factor table and is validated by
  reproducing A16's recorded end-to-end factors to 3 s.f.
- **Window provenance preserved:** I10-1A timings are GitHub Actions runs
  `35918797463` / `35919229083` / `35919233744` with bootstrap CIs, per
  `docs/I10-AUX-UNBWT-RESULTS.md` §3–§6; I9 timings are arch's paired
  interleaved A/B (reps=7, core 18, 1 thread) per `RESEARCH_LEDGER.md` A12/A15.
  These are different protocols and are not merged.
- **Cycle figures in §4.4 are derived**, from Zen 3 scheduling-model values
  recorded in `docs/FRONTIER-RESEARCH-ADDENDUM-2026-09-23.md` §10.1 and §10.3.
  They are labelled derived because the reference explicitly says scheduling-model
  values "are not a substitute for measurement."
- **No local benchmark, sweep, or fuzz campaign was run.** No existing file was
  modified. No git operation of any kind was performed.

---

# 12. ADDENDUM — Track 11 P1 adopted as a shared prerequisite; the one-run impossibility test

*Added after the coordinator's shared-measurement coordination. This addendum
supersedes §6.4's G4 and §10 where they conflict. No new measurement was taken
to write it: §12.2 is a pre-data self-correction and §12.3 is closed-form
algebra on numbers this checkpoint already cites.*

## 12.1 Dependency, not a competing lane

Track 11 §9 **P1** ("cost decomposition of the incumbent (no mechanism)",
`docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md` lines 450-458) measures
the **same quantity** my §3.0 diagnosis depends on. P1c instruments mode-15
decode with **counters, not timers** — entropy-pull count, varint-read count,
copy bytes, macro-path entries, literal bytes — and reports **pulls per output
byte**. Track 11's **H1** is "is per-token symbol coding a large share of decode
cost?"; my **s_ED** is "what fraction of decode time is entropy+dispatch?"
**They are one measurement, not two.**

Therefore:

1. **Track 11 P1 is prerequisite `P0` for track 18.** I do not re-specify it, do
   not rebuild its instrumentation, and do not re-measure its quantity.
2. **Primary cell, adopted verbatim from Track 11 §9:** `generated.log`,
   `anvil-hotop-rans` **175,550 B / 273.534 MB/s**; control `brotli-q9`
   **124,669 B / 619.508 MB/s** (Track 11 M10). That is a **3.656 ns/output-byte**
   budget — Track 11 §5.2 states 3.66 ns/B, the same figure.
3. **Sequencing adopted from Track 11 §12.3:** P1d → P1a/P1b → P1c → kernel
   factorial. My §6 D1 is **re-scoped as a phase inside the P1c job**, not as a
   separate run.
4. **Track 11's G1 threshold (entropy+dispatch ≥ 25 % of per-iteration decode
   cost, §10) is inherited as the track-18 liveness gate.** §12.5 states exactly
   what dies below it.
5. **Provenance discipline preserved.** 273.534 MB/s is an I9-era *Windows host*
   gate-snapshot figure (Track 11 M10; corroborated by `FORMAT.md` line 441,
   "generated.log 274 vs 234 MB/s"). **All projections in §12.3-§12.5 must use
   GH-runner measured rates from the same job**, not this number. It is used here
   only to show the ns/B budget is ~3.6, i.e. O(10 cycles/byte). The
   `docs/CONTEXT.md` Linux figures (913 MB/s, 0.87-0.99 GB/s) are a **different
   configuration on a different host** and are never spliced in.

## 12.2 Self-correction: §6.4's G4 was mis-sized, decided before any data exists

Track 11 §5.2 already names the trap this addendum acts on: sizing a decoder-leg
claim on an *unmeasured* share is "the single easiest way for this track to fail
honestly-but-wisely after burning a CI cycle."

My flat G4 (`kernel ≥ 1.60x`) is exactly that error. Let `s` be the share of
decode time a kernel occupies and `k` the speedup applied to it. Whole-cell gain:

```
gain(s, k) = 1 / (1 - s + s/k)          [Amdahl on a sped-up fraction]
```

With the residual token-lane bar `R_res` from §2.1 (**1.531x** bench frozen,
**1.805x** this-lane), the gain clears the bar only when:

```
k >= s / (s - (1 - 1/R_res))
```

For `R_res = 1.531`: `k >= s / (s - 0.3468)`.
For `R_res = 1.805`: `k >= s / (s - 0.4460)`.

A flat `k >= 1.60` is trivially satisfied at `s = 1.00` and **unmeetable at any
realistic `s`**. It is not a gate; it is a decoration. It is withdrawn **before**
any data — the only time a threshold may legitimately move.

**The corrected binding rule is G4' in §12.5.**

## 12.3 The impossibility table (derived, closed form)

Required kernel speedup to close the residual bar, versus the attacked share:

| attacked share `s` | `k` needed for **1.531x** | `k` needed for **1.805x** |
|---:|---:|---:|
| 0.25 (Track 11 G1 floor) | **impossible** (ceiling 1.333x) | **impossible** (ceiling 1.400x) |
| 0.35 | 110x | impossible |
| 0.45 | 4.36x | impossible |
| 0.50 | 3.26x | 9.26x |
| 0.55 | 2.71x | 5.29x |
| 0.58 | 2.55x | 4.47x |
| 0.60 | 2.37x | 3.90x |
| 0.65 | 2.14x | 3.19x |
| 0.70 | 1.98x | 2.76x |
| 0.75 | 1.86x | 2.47x |
| 0.80 | 1.77x | 2.26x |
| 0.85 | 1.69x | 2.10x |
| 0.90 | 1.63x | 1.98x |
| 0.95 | 1.575x | 1.885x |

Inverted — **what the whole package must achieve on the share it attacks**:

| package kernel speedup `k` | min attackable share for **1.531x** | for **1.805x** |
|---:|---:|---:|
| 1.5x | **impossible at any `s`** | impossible |
| 2.0x | **0.694** | impossible |
| 2.5x | **0.578** | 0.694 |
| 3.0x | **0.520** | 0.641 |
| 4.0x | 0.441 | 0.585 |

**The headline derived fact of this addendum:**

> With a realistic package ceiling of ~2.5x on its attacked terms (§4.4 predicts
> 2-3.5x on the entropy kernel; Track 11's own M-model `max(2.0, 0.12*L)` implies
> ~1.5x on copies), **the entire track-18 token-lane programme is arithmetically
> dead unless `s_ED >= ~0.58`** — or, with copies added to the attackable set,
> unless `s_ED + s_copy >= ~0.58`. If P1c returns `s_ED ≈ 0.25` and
> `s_copy ≈ 0.11` (the values Track 11's G1 floor plus §4.4's copy model
> suggest), then **all decoder-architecture work combined has a whole-cell
> ceiling of `1/(1 - 0.36) = 1.5625x`** — barely above the 1.531x low bar, far
> below the 1.805x high bar — and track 18 must report **KILL on the token
> lane**, with the residual reallocated to *representation* change, not to
> decoder architecture.

This is the answer to "which proposals become impossible", and it is knowable
**before** any mechanism is built.

## 12.4 The ONE remote run: two phases, single job, single checkout

Adopting Track 11's discipline — **counters, not timers**, for everything
countable; timers only where counting is impossible; one job so pairing and
fingerprint are valid (`docs/github-actions-benchmark-protocol.md` §2, §3, §4
`microarch`, §5, §6).

### Phase A — deterministic, citation-grade, **no timing dependence**

Track 11's P1a/P1b/P1c counters, **imported not reimplemented**, on the primary
cell plus `generated.json`, `generated.jsonl`, `doc.md`, `src.cpp` and the PE set:

- `H_op` and the full opcode distribution (P1a) — also the direct test of my
  §8.2 risk, i.e. whether renorm is frequent enough for L2 to exist at all;
- per-token category counts, mask/literal/macro byte shares (P1b);
- **share-vector inputs**, all deterministic counts: entropy-pull count,
  varint-read count, **copy bytes split by `dist >= len` vs `dist < len`**,
  literal bytes, macro-path entries, per-stream decoded length, block count,
  and `symtab` table-build count / total bytes filled.

Phase A alone decides the byte-only gates in §12.6. **It can, and should,
return a decisive NO-GO by itself** — that is the good outcome.

### Phase B — timed, paired, same job, same binary

Two arm families, both against the **un-stubbed** arm.

**B1 — term-ablation arms** (whole-cell, one term stubbed per arm):
(i) entropy pull → constant byte; (ii) copy → `memset`;
(iii) literal materialization → constant; (iv) `symtab` build → hoisted static.
Each arm yields `s_i ≈ 1 - 1/speedup_i`. **Pre-registered caveat:** a stub can
remove more than its own term by exposing a bottleneck that was hidden behind it,
so `s_i` is an **over**-estimate and the bias direction is fixed in advance.
**Mandatory non-additivity measurement:** also ablate all terms jointly and report
`Σ s_i` against the joint `s_joint`; the gap is the shared-resource term (branch
predictor, load ports) and is itself a result.

**B2 — kernel factorial arms** (trace level, §6.2's A0-A5) on traces captured
**from the same cells in the same job**, so the projection in §12.3 joins a share
measured on a corpus to a kernel speedup measured on the same corpus.

### The join — this is the run's single output

For each arm, in one report:

```
whole_cell_projection(s_i, k_i) = 1 / (1 - s_i + s_i/k_i)
impossibility_check:             k_i >= s_i / (s_i - 0.3468)  ?
```

with a bootstrap CI on `k_i` propagated into the projection. **The decision
variable is `whole_cell_projection`, not `k_i`.**

### Mandatory controls (any FAIL ⇒ run VOID)

- The Phase-B harness carries Track 11's precedent check: the instrumented
  replica must be **byte-identical to the shipped decoder** in the un-stubbed
  configuration. `prototypes/i9-decode-perf/POSTCODER-SPEC.md` §4 established
  exactly this pattern for `bwt_arith_decode`. An unverified replica voids B1.
- Ablation arms are **timing instruments only**: they need not round-trip and
  their output is discarded. They must be flagged non-decoders so nobody mistakes
  them for a format proposal.
- Full round-trip + forced registry coverage + fuzz on the **unmodified** decode
  path, per `docs/decoder-audit.md` conventions, before any byte count.
- No `src/anvil.cpp` edit; harness lives only under
  `prototypes/swarm-2026-10-02/18-future-decoder-architecture/space-bunny/`.
- A/A null arm spans 1.0; max arm robust CV ≤ 15 %; ambient gate PASS.

## 12.5 Binding pre-registered gates (G0-G3, G5-G7 of §6.4 stand; G4 is replaced)

**G4' — share-conditioned promotion.** Promote to a full-codec pilot **only if**
all of:

1. G0, G1, G2 pass (§6.4);
2. Phase A returns `s_ED` (from the deterministic counters and, where needed, the
   B1 ablation), reported as a range;
3. the best composed arm's kernel speedup `k_best` satisfies
   **`k_best >= s_ED / (s_ED - 0.3468)`** (bench bar) **and**
   **`k_best >= s_ED / (s_ED - 0.4460)`** (this-lane bar);
4. `whole_cell_projection(s_ED, k_best) >= 1.531` with CI lower bound ≥ 1.35.

**G8 — structural NO-GO on the whole track (new, and the most likely outcome).**
Declare track 18 **KILL on the token lane** — and publish the impossibility table
as the deliverable — if any of:

- **`s_ED < 0.25` (Track 11's G1 NO-GO).** Then `gain(∞, s_ED) ≤ 1.333x < 1.531x`,
  so **L1 and L2 are arithmetically dead, not merely DECODE-SHORT** — no number of
  lanes, no branchless ladder, no AVX2 changes that. Every future
  entropy-architecture proposal landing in the same term (including track 06's and
  track 05's entropy-side gains) inherits the same verdict.
- **`s_ED + s_copy < 0.3468`.** Then the entire attackable set for *any* decoder
  architecture is below the bar and no package can close the lane.
- **`k_best < 2.0x` while `s_ED < 0.694`.** The package is too weak for the share
  it has, regardless of composition.

**G9 — what survives G8.** A G8 NO-GO does not make this report worthless; it
converts it into the measurement that stops three sibling tracks from funding
entropy-architecture work against a term that is a quarter of decode. Under G8
the correct disposition is **KILL with a published ceiling**, and the residual
1.53-1.81x is reallocated to representation work (Track 11 FLI's byte leg; track
12 encoder-only search; a different parse).

## 12.6 Phase-A gates that need **no timing at all** (count-only, byte/count-only)

These are the cheapest decisions available and should be read off Phase A before
any timer is trusted:

| deterministic quantity | gate | verdict if failed |
|---|---|---|
| `H_op` (bits/symbol, mode-15 opcode stream) | if `H_op <= 1.5` bits then expected renorm ≈ 0 and L2's mispredict target **may not exist on opcodes** (§8.2) | L2's premise unverified on the named floor stream; a B2 arm result may not be read as mechanism confirmation |
| copy bytes with `dist < len`, as fraction of `out_len` | if `< 0.10`, even making overlap copies free gives `s_copy ≤ 0.11` → ceiling **1.123x** ≪ 1.531x | **L3 is structurally DECODE-SHORT on its own**; it may only ever be a contributor to a combined package, and must be labelled so |
| `symtab` bytes filled per output byte (`Θ(4096)` per stream per block) | if `> 0.5` stores/output byte, L0 is a real, unharvested, wire-invisible win | L0 confirmed as infrastructure — **explicitly not a crossing route** (§3.5) |
| entropy pulls per output byte | if `< 0.25`, per-symbol entropy work cannot dominate regardless of per-symbol cost | `s_ED` ceiling drops; strengthens G8 |
| macro-path entries per output byte | if `> 0.05`, the *rare* path is not rare and the hot/fused premise is wrong | invalidates the "fused single path" premise that both track 11's FLI and my L3 assume |

## 12.7 Revised recommendation after this addendum

Unchanged: **PILOT**, and now *specifically* the single Phase-A + Phase-B job —
not a standalone kernel microbenchmark.

Sharpened:

- **The run's primary deliverable is the share vector and the impossibility
  table, not a speedup.** It is designed to be able to return a decisive,
  publishable negative in one job.
- **The §6 kernel factorial (A0-A5) is now conditional.** It earns its arms only
  if Phase A shows `s_ED >= 0.25` and the §12.6 deterministic gates have not
  already voided L2 or L3. Otherwise it is skipped and the job reports the
  ceiling instead.
- **Track 11 P1 remains the prerequisite; this track does not duplicate it.** If
  the coordinator runs P1 for track 11 first, track 18 consumes its output and
  runs only the join plus the B2 arms — a strict saving.
- **Downgrade to HOLD** if doctrine-12 is read to cover the token lane (§6.6).
- **The kill conditions are now G8's three clauses**, which are strictly stronger
  and cheaper than the G5 originally preregistered, because two of the three are
  decidable without any timer at all.