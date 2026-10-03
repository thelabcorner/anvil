# Track 08 — DEFLATE Reconstruction Integration (Space Bunny Free)

**Agent:** Space Bunny Free (constructive inventor) · **Date:** 2026-10-02
**Track:** `08-deflate-reconstruction` · **Worktree:** `i10-aux-unbwt` @ `b8eae11` (intentionally dirty; untouched)
**Status:** FINAL — reconciled with `08-deflate-reconstruction-fledge.md` (Fledge Alpha Free, incl. Addendum A). **Verdict: HOLD** (downgraded from PILOT), consistent with the critic. **No threshold was moved, added, or relaxed.** §10 now freezes **two** external controls (precomp→Brotli q11/lgwin30 *and* precomp→xz -9e) plus the binding **producer-identity replayability** gate; §12 ranks that gate first; §13 is the point-by-point response.
**Novelty position:** **none, by ruling.** DEFLATE reconstruction is adopt-class prior art (`docs/gate-ruling-i9-p41-deflate.md` V-4, `prototypes/i9-deflate/PRIOR-ART-NOTE.md` §1). This track must **not** be routed to the mechanism-novelty gate; its only gates are value, Pareto, and safety. Lineage cited once, as required: precomp (schnaader), preflate (D. Steinke), preflate-rs (Microsoft), reflate, grittibanzli, plus container recompressors (zopflipng-class).

## 1. Strongest mechanism (one sentence)

**DEFLATE-REPLAY-v1, registered as ratio transform 4 inside the existing revision-2 mode-17 envelope:** replace every *bit-exactly replayable* embedded DEFLATE payload in a block with its plaintext, transmit a ~1-byte-per-stream parameter code from an immutable registry, and let the decoder regenerate the original compressed bytes with a **pinned, versioned replay engine** — so a stronger backend compresses the plaintext while the outer file stays byte-identical.

Falsifiable hypothesis (the only one this track asserts):

> **H1.** On container-heavy binaries whose payload is dominated by DEFLATE (the files where whole-file BWT is catastrophic), transform 4 yields a complete-payload byte win over the *same-backend* transform-0 control that survives decoder-side replay cost, replay-engine code/state cost, and per-block container-containment loss — i.e. it produces a non-dominated point on the (complete bytes, decode MB/s) plane **without** degrading any other file.

H1 is a **deployment/value** hypothesis. It carries **no** mechanism-novelty claim (ruling V-4; P41-8, P41-9 dispositions).

## 2. Evidence map

Labels: **[M]** measured in a frozen artifact/commit; **[D]** derived by arithmetic from [M] or from documented library parameters; **[P]** projection; **[H]** hypothesis.

| # | Fact | Label | Source (exact path) |
|---|---|---|---|
| E1 | mozilla census: **2,564 detected = 2,354 DEFLATE + 210 stored**; all 2,564 verified and CRC/Adler-clean; DEFLATE **C = 3,177,007 B**, **U = 9,991,436 B**; kinds zip 2,520 / png-zlib 44; 10,393 raw-zlib header candidates attempted and rejected | [M] | `prototypes/i9-deflate/results/census-mozilla.json`; corroborated `RESEARCH_LEDGER.md` §A2, `docs/gate-ruling-i9-p41-deflate.md` V-1 |
| E2 | replay: **2,354 attempted / 2,331 valid / 23 diff / 0 brute**; valid bytes **2,898,132 (91.22%)**; parameter histogram **2,282× raw/l6/m8/default, 7× raw/l6/m9/default, 42× zlib-wrapped/l6/m8/filtered**; zlib **1.3.1** | [M] | `prototypes/i9-deflate/results/replay-mozilla.json` |
| E3 | stream-size distribution (2354 verified DEFLATE): ulen max **161,861**, p99 **40,186**, p95 **16,347**, median **1,766**, mean **4,244**; clen max **53,609**, p99 **11,359**, median **752**, mean **1,350** | [M] | recomputed from `prototypes/i9-deflate/results/census-mozilla.json` (helper `scratch/swarm-tmp/i10_dflt_stats.py`) |
| E4 | charged transform: **51,220,480 → 57,034,135 B**; **2,331** records replaced; side table **21,985 B raw / 14,199 B** brotli-q11-lw24; prototype inverse **sha256-identical** (`657fc376…` both ways) | [M] | `prototypes/i9-deflate/results/transform-mozilla.json`, `RESULTS.md` §2.2/§5.5 |
| E5 | ANVIL encode (ratio mode, Brotli backend, rev 2): original **13,806,173 B** / 129.6 s; transformed **12,443,996 B** / 136.4 s ⇒ **recovery 1,362,177 B**, encode **+5.2%** for **−9.87% bytes** | [M] | `prototypes/i9-deflate/RESULTS.md` §2.2; artifacts `results/mozilla-anvil-brotli*.anv`, `results/transformed-mozilla-anvil-brotli*.anv` |
| E6 | same-file reference bytes (Windows local window, **ranking-grade, unpaired**): mozilla brotli-q11-lw30 **13,806,141** (dec 0.3036 s), xz-9e **13,376,248** (dec 0.6053 s), zstd-ultra22-long27 **14,967,572** (dec 0.1056 s), **anvil-ratio 13,806,172** (dec 0.3033 s) | [M] | `tests/ratio-first-standard.csv` (**do not splice** with remote Class B rows) |
| E7 | the whole portfolio currently contributes ~**nothing** on mozilla: anvil-ratio ≈ brotli-q11 to 31 B | [M] | `tests/ratio-first-standard.csv` (same window as E6) |
| E8 | BWT catastrophe on the target files: mozilla **anvil-bwt-direct 17,823,651** vs **anvil-auto-direct 13,806,173** (**+4,017,478**); samba **4,398,658** vs **3,761,931** (**+636,727**) | [M] | `tests/auto-routing.csv` (**different protocol from E6 — not spliceable with it**) |
| E9 | scope limit: **sao and ooffice contain zero DEFLATE**; untouchable **212,047 B**; addressable total **1,652,972 B** (mozilla 1,459,509 ceiling + samba 193,463) | [M] | `prototypes/i9-deflate/RESULTS.md` §0; ruling V-2b |
| E10 | replay decode cost, **Python**: 8.69 MB plaintext re-deflated in **0.194 s = 44.7 MB/s** | [M] (Python) | `prototypes/i9-deflate/RESULTS.md` §4 |
| E11 | prototype wire roundtrip verified sha256-exact on four canonical binaries (`8EAE1FB3`, `0D1E130B`, `E8AA2E48`, `72D65150`) under the 494/494 encoder-identity gate | [M] | ruling V-3 + addenda; `RESEARCH_LEDGER.md` §A2, §A14, §A16 |
| E12 | **compression claim still PENDING `src/anvil.cpp` integration + fuzz**; all ANVIL byte figures come from a *Python* prototype wire, never from an integrated ANVIL decoder | [M] | ruling V-3 (`docs/gate-ruling-i9-p41-deflate.md`) |
| E13 | mode-17 wire today: `transform_id` ∈ {0,1,2,3(disabled)}, `backend_id` ∈ {1 Brotli, 2 BWT}, `uvarint transformed_size`, payload; bound `2*out_len+4096`; decoder rejects unknown IDs **before** backend decode; transform 3 is a retired clean rejection | [M] | `src/anvil.cpp:4641-4657`, `FORMAT.md` §"Revision 2 ratio envelope", §"Transform registry" |
| E14 | `--parse=ratio` forces **block_size = 128 MiB** unless `--block=` is explicit; the whole-file field is written once in the header, so all blocks share one size | [M] | `src/anvil.cpp:3769`, `src/anvil.cpp:4999`, `src/anvil.cpp:4669` |
| E15 | zero FRONT-CROSSING on every grid and reference class; `GRID-THIN` always binding; transform-enabled controls are mandatory side-channel bars on `{synthetic}` cells | [M] | `docs/I10-FRONTIER-RECON-2026-09-24.md` §1, §3; `docs/gate-ruling-i9-recon-crossing.md` R-2/R-5 |
| E16 | the "wins bytes on BWT-catastrophic files" property is explicitly **ruled a routing property, not a novelty position** | [M] | ruling V-4; `RESEARCH_LEDGER.md` §A2 (P41-9 REJECT) |
| E17 | publication of new workflow files is blocked pending **explicit authorization** — a remote experiment cannot be dispatched from this worktree today | [M] | `docs/I10-FRONTIER-RECON-2026-09-24.md` §3 "Pending local handoff" |
| E18 | **container-containment coverage vs block size, measured** on the frozen mozilla metadata (19 ZIP archive groups; largest archive span **841,666 B**; all archive extents end by offset **4,216,794**; ZIP DEFLATE C = **3,132,001 B**, matching the integration plan's independent figure): eligible DEFLATE bytes are **100.00%** at block ∈ {128, 64, 16, 8} MiB, **86.29%** at 4 MiB, **54.39%** at 1 MiB (streams 2520/2520, 2319/2520, 1748/2520). **This is the CONTAINMENT axis only — it says nothing about whether those bytes are replayable** (§12 item 1, E20) | [M] | recomputed from `prototypes/i9-deflate/results/census-mozilla.json` (helper `scratch/swarm-tmp/i10_dflt_blocksize.py`); independently confirmed by Fledge A.2 (±0.2–0.7 pt, scope difference) |
| E19 | **no DEFLATE encoder exists anywhere in the tree**: `third_party/` holds brotli, libsais, zstd **1.5.7** — and vendored zstd exposes only `ZSTD_f_zstd1` / `ZSTD_f_zstd1_magicless` frame formats (no zlib/gzip **encoding**); its `zlibWrapper/` is a gzip-file/zlib *decompression* compatibility layer. A replay engine must therefore be newly vendored, and the engine-swap escape is closed | [M] | `third_party/zstd/lib/zstd.h:1385-1386`, `third_party/zstd/zlibWrapper/`, `third_party/` listing; confirmed verbatim by Fledge A.2 |
| E20 | **producer-identity replayability on the frozen corpus: 91.22% of DEFLATE bytes / 99.02% of streams, 100% of them from one legacy producer (zlib 1.3.1, 3 parameter tuples). The frozen corpus therefore contains ZERO modern-producer streams and carries ZERO information about modern-producer coverage** — the 8.78% of DEFLATE bytes (23 streams) that no zlib-1.3.1 setting reproduces is in-corpus evidence that encoder-version drift breaks exact replay | [M] | `prototypes/i9-deflate/results/replay-mozilla.json` (`param_hist`, `tally`, `bytes_tally`), `zlib_version` |

## 3. Precise mechanism

Scope v1 (narrow on purpose, per `docs/I10-DEFLATE-REPLAY-INTEGRATION-PLAN.md` §3): **ZIP method-8 raw DEFLATE only**, no PNG IDAT, no PDF FlateDecode, no generic zlib/gzip scan, no recursive containers, no correction coder. Corpus census says ZIP is **≈98.6%** of measured DEFLATE bytes (3,132,001 of 3,177,007; integration plan §2).

**Representation (transform 4, decoder-visible program):**

```
byte      transform_id = 4
byte      backend_id                      (1 = Brotli, 2 = BWT; unchanged registry)
uvarint   transformed_size
--- transformed representation, compressed by backend_id ---
byte      representation_version = 1
byte      replay_engine_id              (immutable; pins compressor semantics)
uvarint   record_count
  repeated record_count times, in ascending original-stream order, non-overlapping:
    uvarint raw_gap_length              (opaque bytes between replayed payloads)
    uvarint original_deflate_length     (declared, verified against replay output)
    uvarint plaintext_length
    byte    parameter_code              (index into the immutable parameter registry)
    raw_gap_bytes[raw_gap_length]
    plaintext_bytes[plaintext_length]
uvarint   tail_length
          tail_bytes[tail_length]
--- end; backend payload continues in the mode-17 envelope ---
```

Invariants that make the byte accounting closed:

- Absolute offsets are **never transmitted**; the next offset is derivable from cumulative reconstruction output (integration plan §6). Measured proxy for the full cost of *transmitting* offsets anyway: **21,985 B raw / 14,199 B** brotli-compressed (E4).
- Every reconstructed stream must reproduce **exactly** `original_deflate_length` bytes; therefore all outer ZIP offsets, CRCs, and sizes stay valid with no metadata rewrite (integration plan §6).
- Parameter registry, not generic 4-field parameters: the measured distribution is 3 distinct parameter tuples covering 100% of valid streams (E2). **[D]** one byte per record; `parameter_code = 0xFF` escapes to explicit fields (never used on the frozen population).
- The **replay engine is part of the format**: plaintext does not uniquely define a DEFLATE bitstream. The decoder must not "call whatever zlib is installed". v1 pins a vendored zlib source+configuration under an immutable `replay_engine_id`; a different engine or version is a **new ID**, never a silent semantic change (integration plan §5). Clean-room: implement the search/replay path directly; **do not import preflate-rs code** (Apache-2.0/LGPL; ruling V-4).
- Encoder acceptance is fail-closed and self-verifying: inflate → registry replay → byte-compare → accept only on equality; then the **complete** transform-4 payload must beat the complete transform-0 payload under the **same** backend (E13 already enforces this for transforms 1/2; transform 4 inherits it).

## 4. Encoder and decoder state machines

### 4.1 Encoder (`transform 4` build, per block)

```
S0  scan: parse containers inside the block; produce candidate list of
    (payload_offset, clen) for method-8 ZIP entries; fail closed on any
    structural doubt (checked arithmetic, reject encryption, ZIP64, data
    descriptors in v1, duplicate/overlapping headers, fake PK signatures)
S1  for each candidate, ascending offset:
      inflate -> U (bounded output cap = declared usize, else kMaxRatio)
      attempt registry order [ (raw,l6,m8,default), (raw,l6,m9,default), ... ]
      on first byte-exact match -> emit record, advance
      on registry miss -> BOUNDED fallback search (hard attempt budget A_max)
      on budget exhaustion -> leave the region RAW (no record)
S2  emit gaps, tail, footer; compute transformed_size; assert
    transformed_size <= 2*block_out_len + 4096   (mode-17 bound, E13)
S3  backend-encode the transformed representation
S4  emit the mode-17 payload only if it is COMPLETELY smaller than the
    transform-0 payload built under the same backend; else emit transform 0
```

Encoder state: container cursor, per-stream inflate state (≈32 KiB window + 32 KiB output), a bounded attempt counter (global per block, not per stream — this is the anti-wobble rule), the record list, and the candidate registry (immutable).

**New architectural constraint this track surfaces (not in the I10 plan), now measured:** transform 4 needs **whole-container containment inside the block**. With the ratio-mode default 128 MiB block (E14), mozilla (51.2 MB) is one block and the measured 1,362,177 B applies unchanged. E18 quantifies how fragile that is: mozilla's 19 ZIP archives are small (largest span 841,666 B; all extents end by offset 4,216,794), so **100% of replayable DEFLATE bytes survive any block size ≥ 8 MiB**, 86.29% survive 4 MiB, and 54.39% survive 1 MiB. Consequences: (i) on the **containment axis** the risk is closed for the frozen corpus at the production default and at any sane explicit block size. **Containment is necessary but NOT sufficient**: it establishes that the scanner *can see* the streams, not that they are *replayable* — those are two different axes, and the binding coverage gate is the second one (E20, §10 gate G6); (ii) at small blocks the transform **silently becomes unavailable** for straddling archives (fail-closed = no loss, no gain, so the failure mode is bounded by construction); (iii) because `block_size` is one archive-level field (E14), raising it for container-heavy inputs is a whole-file decision that also moves the BWT backend's memory profile — **[H]** a container-completeness-aware block-size policy may be worth one cheap later experiment, but it is **out of scope for the pilot and must not be bundled into it**. A block-size sensitivity ablation is nevertheless added to §10 because it is deterministic (bytes only) and cheap: it converts a stated constraint into a measured curve.

### 4.2 Decoder (`transform 4`, per block)

```
D0  validate transform_id == 4, backend_id in {1,2}; validate transformed_size bound
D1  backend-decode -> X (transformed representation bytes)
D2  parse representation_version (reject unknown), replay_engine_id (reject unknown)
D3  record_count <= min(transformed_size, block_out_len)   [bound BEFORE allocation]
D4  for each record, in order:
      read gap_len, clen, ulen, parameter_code with checked uvarints
      require gap_len <= remaining output budget
      require ulen <= remaining transformed bytes
      resolve parameter_code -> (level, memLevel, strategy, wbits); reject unknown
      copy gap bytes verbatim
      replay: pinned engine, raw DEFLATE, streaming
        require replayed byte count == clen exactly          [else malformed]
        require cumulative reconstructed <= block_out_len    [else malformed]
D5  copy tail; require exact consumption of X
D6  require reconstructed length == block output length exactly
D7  existing mode-17 CRC check runs on the reconstructed block (unchanged path)
```

Decoder state (resident, per stream, streaming): inflate-free — the pinned engine is a **deflate compressor**: hash table + window + pending buffer. **[D]** from zlib's documented `deflateInit2` sizing (`hash_bits = memLevel + 7`, `hash_size = 1<<hash_bits`, `window = 2 * (1<<windowBits)`, `pending_buf = 4 * lit_bufsize` with `lit_bufsize = 1<<(memLevel+6)`):

| engine params | hash | window | pending | ≈total resident |
|---|---:|---:|---:|---:|
| memLevel 8, wbits 15 | 64 KiB | 64 KiB | 64 KiB | **≈192 KiB** |
| memLevel 9, wbits 15 | 128 KiB | 64 KiB | 64 KiB | **≈256 KiB** |

Plus a staging plaintext buffer. Because the largest measured stream plaintext is **161,861 B** (E3) and the p99 is **40,186 B** (E3), a streaming replay with a fixed staging buffer keeps the *incremental* decoder RSS at roughly **192 KiB + staging** — i.e. **< 0.5 MiB**, provided the record table is parsed to a bounded index before any replay begins and plaintext is never fully materialised for large records. **[D/P]** this is a design requirement, not yet measured.

## 5. Full cost model

### 5.1 Bytes (all charged)

| item | value | label |
|---|---:|---|
| original mozilla | 51,220,480 B | [M] E4 |
| transformed carrier (pre-backend) | 57,034,135 B (**+11.37%** carrier growth) | [M] E4 |
| side/replay metadata, raw | 21,985 B (incl. offsets + 2 param B/record, i.e. an **upper bound**) | [M] E4 |
| side/replay metadata, brotli-compressed | 14,199 B | [M] E4 |
| complete payload, transform 0 | 13,806,173 B | [M] E5 |
| complete payload, transform 4 | 12,443,996 B | [M] E5 |
| **net recovery (complete payload)** | **1,362,177 B (−9.87%)** | [M] E5 |
| vs same-file xz-9e (13,376,248) | **−932,252 B** | [D] from E5+E6 |
| vs same-file brotli-q11-lw30 (13,806,141) | **−1,362,145 B** | [D] from E5+E6 |
| 23 diff streams left raw | 278,875 B of deflate (8.8%) | [M] E2 |
| unrealized diff-mode upside | +37,429 … +72,443 B | [P] prior-art correction model, `RESULTS.md` §2.3 — **not** claimed |

**[D] Why the arithmetic works:** every replayed stream replaces `clen` compressed bytes with `ulen` plaintext bytes; the backend then re-compresses the plaintext at a *stronger* rate. Measured net effect on this corpus: the plaintext carrier grows 11.37%, yet the complete payload shrinks 9.87% — i.e. the backend's rate advantage on plaintext exceeds the carrier growth by ~21 percentage points. That margin, not the DEFLATE ratio itself, is the economic content of H1.

### 5.2 Cycles / throughput

Decode is the axis that decides the verdict. **Corrected model** (two corrections to my first publication, both conceded to Fledge §4.1/§3.2, both verified against the frozen artifacts):

```
t_decode(B)  =  t_backend · (57,034,135 / 51,220,480)      # carrier factor 1.113503
              +  t_replay(U_valid = 8,689,786 B at engine rate R)
```

- **Correction 1 (mine was wrong):** the replay input is **not** `U = 9,991,436 B` (all streams) but `U_valid = 8,689,786 B` — the 23 diff streams are left raw and never replayed (E2/A1). My published figure overstated replay cost by 15% and all three rows of my original table were numerically wrong.
- **Correction 2 (mine was wrong, and it was the load-bearing one):** I labelled the 44.7 MB/s replay rate (E10) a "Python overhead floor" and treated it as pessimistic. It is not. `RESULTS.md` §4 replays **the 2,331 valid streams — exactly the population above — in 0.194 s**, i.e. 3,727 B/call ⇒ ≈83 µs/call, against ~1–2 µs of per-call wrapper overhead **[A, unverified]**. The measurement is therefore essentially **zlib 1.3.1 deflate level-6 throughput on this data**, i.e. a *stock-engine* rate, not a floor.

Independent reproduction of Fledge's derivation from same-window inputs only (`scratch/swarm-tmp/i10_dflt_replay_model.py`, reading `tests/ratio-first-standard.csv` + the two frozen JSONs):

| term | value | provenance |
|---|---:|---|
| `t_direct` (anvil-ratio, mozilla, `decompress_s`) | **0.303345 s** | [M] `tests/ratio-first-standard.csv` (ranking-grade, unpinned) |
| carrier factor 57,034,135 / 51,220,480 | **1.113503** | [M]/[D] |
| backend term | **0.337775 s** | [D] |
| `U_valid` | **8,689,786 B** | [M] `RESULTS.md` §1 |
| `R` = 8,689,786 / 0.194 | **44.793 MB/s** | [M] `RESULTS.md` §4 (prints 44.7) |
| `t_replay` | **0.194000 s** | [D] |
| **total projected decode** | **0.53178 s ⇒ 96.3 MB/s** | [P] |
| **paired decode ratio vs control** | **1.753×** | [P] |

**Answer to "is the threshold actually failed?" — YES, on present evidence.** My G4 requires `B/A ≤ 1.60×`. Solving on the same numbers:

| G4 ratio target | `t_replay` budget | **R required** |
|---:|---:|---:|
| 1.60× (promote) | 0.14758 s | **58.88 MB/s** |
| 1.753× (measured R lands here) | 0.19408 s | 44.77 MB/s |
| 2.20× (kill) | 0.32958 s | 26.37 MB/s |

At the only replay rate this repository has ever measured, the mechanism projects to **1.753× — above the 1.60× promote bar and inside my own preregistered HOLD band (1.60×–2.20×)**. By my own rules it does not promote today. **I am not moving G4.** The honest reading is also that my earlier "plausibly non-dominated" sentence was a *descriptive* claim about the point cloud that I failed to label as non-operative next to a gate — the binding criterion is and always was **G4**, and at 1.753× it fails.

One provenance defect Fledge did not raise, which cuts against over-reading this number in **either** direction: `RESULTS.md` §4 does not record the **host or the zlib build** for the 0.194 s figure, so pairing it with a Windows-window `t_direct` is itself a cross-window splice — the exact discipline this project forbids. Therefore 1.753× is a projection built on a partially unpaired input, and the correct response is to **make `R` a first-class measured output of the remote run** (with host, compiler, flags and engine fingerprint recorded), not to argue the projection.

**`R` is an implementation choice with a hard constraint, so it must be preregistered as two numbers, not one:**

- `R_stock` — stock zlib 1.3.1 deflate semantics. Any change to match-finding changes output bytes, so this is essentially pinned. zlib-ng/libdeflate are 2–5× faster but are **not** byte-compatible at the same parameters: they are a different `engine_id` serving a *different* producer population, not a speedup for this corpus.
- `R_max` — the fastest **byte-identical** build of the unmodified algorithm (`-O3`/`-march`/LTO etc.), admitted **only** if the golden self-test reproduces all 2,331 replay outputs exactly.

**G4 is evaluated on the shipped configuration**: `R_max` if and only if the golden test passes, otherwise `R_stock`. The 31% gap between 44.79 and 58.88 MB/s is ordinary cross-build/cross-host variation for one library at one level, so `R_max` is a plausible route through the gate — but it is a *hypothesis to be measured*, not evidence, and no threshold depends on it being met.

Encode, corrected: the measured **+5.2%** (129.6 → 136.4 s, E5) **excludes the replay search**, which ran offline in `replay.py` and is in neither number. Adding the missing term at `A_max = 405` (2,331 registry hits ≈0.19 s; 2,354 inflates ≈0.05 s; 23 unmatched streams × 405 combos ≈11.7 s) gives **≈ +9% [D]**. With a small global `A_max` (8–16) the term collapses and +5.2% becomes defensible. **`A_max` and the realized attempt count must be reported in the artifact**; the headline "+5.2%" must never be quoted without them.

### 5.3 Memory and code size

| item | value | label |
|---|---:|---|
| incremental decoder RSS (pinned deflate state) | ≈192–256 KiB, **engine-dependent, must be measured** | [D] from zlib documented sizing |
| staging plaintext buffer | 64 KiB fixed target (max measured stream plaintext is 161,861 B, E3, so a fixed buffer means chunked replay) | [D]/design requirement |
| decoder binary delta | **unmeasured** — vendored zlib deflate + strict ZIP reader; budget declared here as **≤ 64 KiB** (gate, §10); independent estimate **55–80 KiB** for a full `deflate.c`+`trees.c`+`adler32.c` `-O3` build plus ~5–10 KiB ZIP reader, or **25–35 KiB** for a specialised build (level 6 / memLevel 8 / default / raw only — 3 tuples cover 100% of the frozen population, E2) | [P], Fledge §6.3 |
| permanent format liability | pinning a compressor's **bit-exact output** into the format definition: after the first transform-4 archive ships, `replay_engine_id` is permanent ABI and a routine upstream zlib patch becomes a **data-loss event**. Not mitigable by care — only by never opening the door | **cost, not risk** (admitted from Fledge §3.3) |
| record index at decode | ~6 B/record × 2,331 ≈ **14 KiB** for mozilla | [D] |

The project's Pareto accounting charges RSS and binary size as real axes (`docs/I10-FRONTIER-RECON-2026-09-24.md` §"auxiliary index", ruling §A15/A16 practice). Transform 4 must therefore be reported with **decoder binary delta and decode peak RSS at decode completion** as first-class fields, not footnotes. **[M]** the reference point for how this is measured: `decode_peak_rss_kib` measured at decode completion, `GetProcessMemoryInfo`/`PeakWorkingSetSize` on Windows, `getrusage` maxrss on Linux, with an explicit status string (`docs/I10-FRONTIER-RECON-2026-09-24.md` §"Pending local handoff").

**Two properties that make the memory design safe, and must nonetheless be asserted, not assumed:**

1. **Chunked replay is output-identical.** DEFLATE output is a function of the input byte stream and the flush boundaries, not of how the input is chunked, provided no intermediate flush is issued. **[D]** therefore a fixed 64 KiB staging buffer reproduces the golden bytes exactly, and the memory-bounded design in §5.3 does not require buffering whole streams (max measured plaintext 161,861 B, E3). This must be *proven* by a golden test that feeds one frozen stream in 64 KiB chunks and compares against the single-shot bytes — it is a one-line assertion that would otherwise be a silent correctness cliff.
2. **~~No decompression-bomb surface~~ — RETRACTED (Fledge §6.1).** My earlier reasoning was inverted: a DEFLATE *compressor* is **O(`ulen`) in its input**, not O(`clen`) in its output — it must read and hash-chain-match every plaintext byte. A record with `clen` = 200 B and `ulen` = 8 MiB makes the decoder do megabytes of match search to emit a few hundred bytes. Capping `clen` does not bound work. The bound is real but comes from elsewhere: `ulen ≤ remaining transformed bytes` (D4) ⇒ total replay input ≤ `transformed_size ≤ 2·out_len+4096`. **The corrected posture is worse, not better:** every transform-4 archive makes an ANVIL decoder strictly more attack-prone than one without it (inflate is O(output) with a hard work cap; deflate's worst case is ~`max_chain`×`ulen`, 128× at level 6), and every future zlib memory-safety CVE becomes an ANVIL decoder CVE. Track 19's mandate must be discharged against a **compressor**, which is a less-trodden audit than the decompressor audit this project has been running.

**Decoder binary delta is a real, unbudgeted cost and cannot be dodged by substitution** (estimate per Fledge §6.3 above; the 64 KiB budget is more likely than not to be missed, and the specialised level-6-only build is the mitigation to measure). E19 establishes that nothing already vendored can emit DEFLATE-format bytes, so a zlib deflate subset must be added. The delta must be measured in CI as the `size`/section delta of `anvil.exe` built with and without the replay engine under an identical recipe (the local CMake configure path is currently blocked by a missing Windows RC toolchain, per `docs/I10-FRONTIER-RECON-2026-09-24.md` §"Pending local handoff", so **CI is the only authoritative build gate here**). Until that number exists, treat the 64 KiB promote budget in §10 as an **[unverified]** assumption, not a measured fact; a plausible Release `.text` for a deflate-only zlib subset is on the order of tens of KiB **[P]**, which is the same order as the budget and therefore not comfortable.

### 5.4 Routing economics and BWT-catastrophic portfolio interaction

- Target files are exactly the files where BWT is catastrophic: mozilla **+4,017,478 B** and samba **+636,727 B** when routed to BWT instead of the Brotli backend (E8). Those files are currently routed to the Brotli backend, where ANVIL ties Brotli to within 31 B (E7) — i.e. **the portfolio's only lever on these files is transform 4**. That is the whole economic case, and it is a routing/composition property, expressly **not** a novelty position (E16).
- Selection is per block and always "complete payload, same backend" (E13), so transform 4 **cannot** be a disguised backend substitution — the format already enforces the causal comparison.
- Portfolio-scale effect on Silesia: **[D]** 1,362,177 B of 211,938,580 B input ≈ **0.64%** of corpus bytes, concentrated on **2 of 12** files, with **zero** effect on sao/ooffice (E9). Aggregate movement is therefore small; the per-file rows must stay visible (PB-17/PB-23 of `docs/I10-CORPUS-LOCK-PROTOCOL.md`).
- Interaction with the aux-unBWT branch: **none required, and none should be bundled.** Transform 4 competes under backend 1; the BWT leg's decode problem is separately measured and unresolved (FRONT-GAP, ruling §A16). Bundling would confound both.

## 6. Novelty and prior-art risk

- **Mechanism novelty: NO.** Ruled (E16, P41-8). Prior art covers the container scan, the parameter search, the bit-exact accept, the parameter record, and the correction coder for unknown encoders.
- **What this track may legitimately claim:** a *measured, charged* portfolio property on named corpora, plus a clean-room implementation artifact. Precedent: the K≈8–12 context quantizer kept as enabling infrastructure.
- **Clean-room obligations:** do not import preflate-rs code; implement against zlib directly (ruling V-4); vendor zlib under `third_party` with pinned build configuration that can alter output (`docs/I10-DEFLATE-REPLAY-INTEGRATION-PLAN.md` §12); never let system zlib define format semantics.
- **Risk that the reviewer kills the whole direction anyway:** "a pinned zlib in your decoder is a worse engineering choice than calling a mature production recompressor" is a legitimate objection. Mitigation is empirical only: if the C integration does not beat transform 0 **complete payload** on the same backend, the direction dies on its own terms.
- **Prior-art map to hand track 20 (kill team):** precomp = valid mode; preflate/preflate-rs = diff mode; reflate/grittibanzli = same problem class; zopflipng-class = container-specific recompression. There is no separator to defend, and none is claimed.

## 7. Asymptotic and performance analysis

Let `B` = block bytes, `P` = number of replayable streams, `U` = total plaintext bytes, `C` = total compressed payload bytes inside those streams, `A` = backend attempt budget.

- **Encode:** scan O(B). Replay work O(A·U) in the worst case, O(U) when the registry hits (measured: first-attempt hit rate 2,282/2,331 = 97.9%, E2). Registry miss cost is the only super-linear risk; a global per-block budget makes worst-case encode O(B + A_max·U).
- **Carrier growth:** `+ (U − C)/B`. Measured **+11.37%** on mozilla **[M]**. Worst case is a block of pure incompressible DEFLATE-stored data (U ≈ C), where growth ≈ 0 and the transform is byte-neutral — the selection rule then keeps transform 0. Growth is monotone in the corpus's DEFLATE ratio, i.e. **the mechanism's carrier cost is largest exactly where its byte value is largest**; the pilot must report both.
- **Decode:** O(B) backend work × carrier factor, plus O(U) replay at rate `R`. `R` is engine-bound and roughly independent of `B`. So the decode-time tax is **linear in U with a small constant** — structurally unlike BWT's postcoder/inverse-BWT penalty, which is why this is the "fast decode" branch. **[H]** the mechanism's whole strategic value is that its decode tax is a *compressor at ~10⁸ B/s-class throughput*, not a sort.
- **Hard applicability boundary (Fledge §3.5, admitted; this was missing and it matters):** the mode-17 bound `transformed_size ≤ 2·B_out + 4096` (E13) and `carrier ≥ U` together imply **`U ≤ 2·B_out + 4096`** — total replayable plaintext must not exceed twice the block output. Measured on mozilla: `U_all/B = 0.1951`, carrier 57,034,135 vs limit 102,445,056 ⇒ **1.796× headroom**, comfortable. But the headroom is consumed *in proportion to how compressible the DEFLATE payload is*: a block dominated by highly-compressible DEFLATE (a tar of source JARs, a ZIP of text) has small `C/U` and large `U/B`, and crosses the bound at `U/B > 2`. **Applicability and value are anti-correlated** — the more compressible the DEFLATE, the larger the prospective win and the sooner the bound kills the transform. This is a hard applicability boundary to be stated in any integration note, not an implementation detail.
- **Asymptotic caveat:** the value is bounded by the fraction of `B` that is replayable DEFLATE, `ρ = C/B`. Value per input byte grows with `ρ` and with the backend's rate gap between plaintext and DEFLATE-compressed bytes. At `ρ → 0` (sao, ooffice) the mechanism is inert — measured, E9. Any claim of generality must therefore be stated as a function of `ρ`, not as a corpus-general property.

## 8. Malformed-stream handling, fuzz, and security

Decoder-side (all fail-closed, no recursion, no search):

| input class | required behaviour |
|---|---|
| unknown `representation_version` / `replay_engine_id` | reject before any replay |
| `record_count` > min(transformed_size, block_out_len) | reject before allocation |
| truncated varint / truncated gap / plaintext / tail | reject; never read past the payload |
| gap_len > remaining output budget | reject |
| `plaintext_length` > remaining transformed bytes | reject |
| unknown `parameter_code` | reject |
| replayed length ≠ declared `original_deflate_length` | reject |
| replay output would exceed block output length | reject |
| trailing bytes after tail, or short tail | reject |
| final reconstructed length ≠ block output length | reject (then the existing mode-17 CRC check still applies) |

Encoder-side: the scanner "may fail closed and must never guess" (`docs/I10-DEFLATE-REPLAY-INTEGRATION-PLAN.md` §8). Adversarial ZIP fixtures required: encryption, data descriptors, malformed extra fields, ZIP64, duplicate/overlapping headers, fake `PK` signatures inside payloads, truncated central directory, concatenated archives, self-extracting prefixes.

**Resource ceilings — CORRECTED POSTURE (this paragraph previously argued the opposite and was wrong; see §5.3 property 2 and Fledge §6.1).** The decoder never inflates, but it does run a **compressor**, and a compressor's work is **O(`ulen`) in its input**, not O(`clen`) in its output. Capping the declared `clen` therefore does **not** bound work. The bound comes from the state machine instead: `ulen ≤ remaining transformed bytes` (D4) and `transformed_size ≤ 2·out_len+4096` (E13) ⇒ total replay input is bounded by ~2× the block output budget, and per-record replay input is bounded by the record's declared `ulen`. Worst-case match-search amplification is ~`max_chain`×`ulen` (128× at level 6). Net statement, to be used verbatim in any integration note: **transform 4 trades "no nested-inflate bomb surface" for "a vendored compressor in the decode path" — the decoder becomes more attack-prone, not less, and zlib's CVE surface becomes ANVIL's CVE surface.**

**Three implementation requirements this correction forces (Fledge §6.2; missing from my first version):**

1. **Streaming output cap.** zlib has no "abort after N output bytes" API. The decoder must feed plaintext in bounded chunks and **hard-stop the instant replayed bytes exceed the declared `clen`**, writing only into a buffer capped at `clen`. Checking after `flush()` permits unbounded intermediate emission and is not acceptable.
2. **Abort before append.** On `parameter_code` resolution failure or `replayed ≠ clen`, the decoder must discard partial output rather than append-and-flag, so the output buffer never contains a partial stream. The three nets (per-record length, final block length, existing CRC) are correct but must be *ordered* so the partial stream is dropped.
3. **`record_count` bounded below by carrier structure.** `record_count ≤ min(transformed_size, block_out_len)` (D3) is sound; adding `record_count ≤ carrier_len / min_record_size` makes the parser-loop bound self-evident without arithmetic.

Fuzz requirements (CI only, never local-heavy): (a) structured negative corpus of malformed transform-4 payloads (one per row above); (b) bit-flip/byte-mutation campaign over valid transform-4 archives; (c) **differential oracle**: for any archive decoded under transform 4, the reconstructed bytes must equal the transform-0 reconstruction of the same input (both must equal the source); (d) determinism: same input → identical bytes across repetitions; (e) resource assertion: peak RSS and wall time bounded on adversarial inputs.

## 9. Minimum prototype (bounded, isolated, not wired to production)

Location (per brief §33): `prototypes/swarm-2026-10-02/08-deflate-reconstruction/space-bunny/`. Scope, deliberately minimal:

1. `replay_engine.h` — a **newly vendored, pinned** DEFLATE compressor entry point behind an `engine_id` (E19: nothing in `third_party` can emit DEFLATE bytes), plus a self-test asserting byte-identity against a frozen golden corpus (the 2,331 valid streams' replay outputs hashed once and frozen). No engine internals leak into codec code. The self-test **must also assert chunked-vs-single-shot equality** (§5.3 property 1) — feeding one frozen stream in 64 KiB chunks must reproduce the single-shot bytes.
2. `dflt_scan.cpp` — v1 scanner: ZIP EOCD → central directory → method-8 entries only; checked arithmetic; reject encryption/ZIP64/data descriptors; reject overlap; cap entry count and metadata lengths; **fail closed**. Its coverage must be reported against the active `--block` value so degradation is visible rather than silent (E18).
3. `dflt_build.cpp` — registry-first replay with a **global per-block attempt budget**; byte-exact accept; raw fallback; emits the §3 representation and asserts the `2·out_len+4096` bound.
4. `dflt_apply.cpp` — the inverse program with the §4.2 validation ladder; used both standalone and as the fuzz target.
5. `dflt_selftest.cpp` — the four blocking gates in §10 (G1–G4) runnable on the pinned frozen mozilla/samba artifacts **without** any corpus benchmark (byte assertions only, no throughput claim).

Deliberately **out** of the prototype: PNG/PDF, diff-mode corrections, recursive containers, BWT backend interop, any change to `src/anvil.cpp` or `FORMAT.md`, any timing claim.

**Why no prototype code is written yet.** The verdict cannot move on code written in this session: every gate in §10 that a prototype could inform is either a *measurement* (gated to GitHub Actions by the mandate) or blocked by the publication-authorization dependency (E17). The two things code would settle early — engine byte-identity and engine throughput — are precisely the two that must be settled **on the remote runner**, and engine identity additionally requires a newly vendored dependency whose license/build pinning is a coordinator-level decision, not a prototype decision. Writing code now would create an unwired, unverified artifact that must be rewritten anyway. The file list above is therefore the pilot's entry contract, frozen here.

## 10. Decisive remote-only experiment (preregistered, GitHub Actions only)

**Name:** `I10-1B paired DEFLATE-REPLAY remote run`. One job, same-job paired/interleaved, ≥13 repetitions, A/A null control, bootstrap CIs, per-file rows, all fields outside timing verified by post-run byte/SHA-256.

**Arms** (identical checkout, identical inputs, identical outer framing, complete metadata charged):
- **A (control):** transform 0 + backend 1 (Brotli q11/lgwin30 — current production policy, E13).
- **B (candidate):** transform 4 + backend 1, same backend, same corpus.

**Two external prior-art controls — BOTH frozen before any run, neither substitutable for the other post hoc** (coordinator ruling; they answer different questions and each answers one that the other cannot):

| arm | pipeline | question it answers | why it cannot replace the other |
|---|---|---|---|
| **C2a** | `precomp <file> \| brotli q11 lgwin30` (same job, same corpus) | **What is ANVIL's marginal transform value over the prior art under the *same* backend?** = `bytes(C2a) − bytes(B)` | holds the backend fixed; the only clean isolator of the transform from the backend |
| **C2b** | `precomp <file> \| xz -9e` (**already-frozen external control**, the deployment-recipe number) | **Is the win already obtainable off the shelf at all**, against the strongest reference backend? | uses a *different* backend, so it confounds transform gain with backend substitution |
| **C3a / C3b** | C2a and C2b repeated on the locked **held-out archive family** | does either hold off the shelf on independent data? | as above |
| **C4** | a mainstream archive **rebuilder** (e.g. `advdef`-class) on the same corpora — **supplementary only** | the mechanism's **ceiling** without a byte-identity invariant | it *rebuilds* the container, so its output ≠ the input file; it does **not** share transform 4's byte-identity invariant, so it cannot measure ANVIL's *marginal* value (Fledge A.4.1). Its in-place-invariance must be **tested in-run**, not assumed |
| **C5** | replayable-byte fraction **split by producer identity** (legacy zlib vs zlib-ng/libdeflate/miniz) on the held-out family, derived from the C2/C3 valid/diff tallies | the binding generalization question (E20, G6) — obtained with **zero ANVIL code**, since precomp implements the same bit-exact-accept rule | free dividend of C2/C3; no separate run needed |

**Control-tool discipline (binding):** every external tool must be pinned by **version + SHA-256**, its **license verified at build time** (not assumed), and its in-place byte-identity **tested** in-run. If a named tool cannot be built in the CI image, the substitute must be **named and justified in the artifact** — a silent swap converts a frozen control into a post-hoc one and invalidates G5.

**References (same job, same corpus):** brotli q1/q4/q6/q9/q11/lw30, dense zstd tiers, xz -9e.

**Block-size sensitivity ablation (deterministic, bytes only, same job):** arm B repeated at `--block=` 128 / 16 / 8 / 4 / 1 MiB, reporting complete bytes and eligible-container coverage per file. E18 predicts the 128/16/8 MiB points coincide on mozilla and that degradation at 4/1 MiB is graceful; this converts §4.1's stated constraint into a measured curve and is the cheapest possible guard against a silent `--block` regression.

**Corpora (discovery / validation / held-out separated, per brief §15 and §10):**
- *Discovery:* the existing frozen mozilla artifacts (E1–E5) and the bounded region `mozilla/chrome/en-US.jar` (464 streams).
- *Validation:* `samba` (160 PDF FlateDecode streams — different container, different path; note v1 is ZIP-only so samba is expected to be a **negative** control for v1 and a positive control only if the scope is widened).
- *Held-out:* **a new, locked archive-family family** created and hashed before measurement (`docs/I10-CORPUS-LOCK-PROTOCOL.md`). E15/FRONTIER-RECON §3.8 states no new held-out corpus currently exists — so **creating and locking it is a precondition of this experiment**, not an output of it.

**Reported fields (deterministic, outside timing):** complete bytes per arm per file; transform-4 metadata bytes (raw and compressed); streams replayed / rejected raw; **`A_max` (attempt budget) and realized attempt count** (mandatory — the encode figure is uninterpretable without them, §5.2); **`R_stock` and `R_max`**, each with host, compiler, flags and engine fingerprint, and the golden-test verdict that decides which one ships; **replayable stream bytes split by producer identity** (legacy vs modern); decoder binary delta (bytes); replay engine id and build fingerprint; sha256 of every output; `decode_peak_rss_kib` at decode completion with its status string. **Timing fields:** paired encode ratio B/A, paired decode ratio B/A, decode MB/s, peak RSS, A/A CI, per-file rows including skipped/invalid/catastrophic (PB-17/PB-23). Labels carried: `GRID-THIN`, `NON-DEFAULT/RESEARCH-CONFIG` if raised above production policy, throughput grade explicitly stated.

**Gates (frozen before measurement; thresholds are NOT to be moved after seeing data):**

| gate | condition | class |
|---|---|---|
| **G1 correctness (blocking)** | sha256-exact roundtrip on 100% of archives in all arms; every §8 malformed case rejected cleanly; zero crash/OOM/hang in the CI fuzz campaign; differential oracle A≡B≡source; every external control's in-place byte-identity **tested** | **blocking** |
| **G2 existing-path identity (blocking)** | with transform 4 disabled, every arm's bytes are identical to the frozen baseline (the project's standing 351/351-equivalent identity gate) | **blocking** |
| **G3 bytes (promote)** | on mozilla: `B ≤ A − 900,000 B` **and** `B < min(xz-9e, brotli q11)` complete bytes; no portfolio file regresses > 0.05% because of transform-4 selection; held-out family recovers **≥ 250,000 B** | promote |
| **G4 decode cost (promote)** | paired decode ratio `B/A ≤ 1.60x` on mozilla, **and** `B` decode MB/s ≥ xz-9e decode MB/s (same job), **and** decode peak-RSS delta ≤ **+8 MiB**, **and** decoder binary delta ≤ **64 KiB** | promote |
| **G5 prior-art control (promote/KILL — the decisive value gate)** | `C2a > B + 50,000 B` (i.e. precomp under the *same* backend is materially worse) **and** `C2b > B + 50,000 B`; both arms mandatory | if `C2a ≤ B + 50,000 B` **or** `C2b ≤ B + 50,000 B` ⇒ **KILL** — the win is obtainable off the shelf at zero decoder cost, zero binary delta, zero engine ABI; publish the deployment recipe instead |
| **G6 producer-identity replayability (promote/KILL — the binding coverage gate)** | on the locked held-out family, **≥ 90% of replayable stream bytes** measured **by producer identity** (not by containment, not by block placement), reported split legacy vs modern | < 50% ⇒ **KILL** for the general-purpose claim (survives only as a narrow legacy-archive path, which does not justify a frozen engine ABI); 50–90% ⇒ **HOLD, reclassified** as a legacy-archive tool |
| **K1 kill** | mozilla byte gain < 500,000 B, **or** paired decode ratio > 2.20x, **or** decode RSS delta > 64 MiB, **or** decoder binary delta > 256 KiB, **or** any G1/G2 failure, **or** any G5/G6 KILL condition, **or** integration requires changing an existing transform ID's semantics, **or** held-out family yields 0 replayable streams | **KILL** |
| **H1 hold band** | everything strictly between promote and kill | **HOLD** — re-derive from the same-job dense frontier evidence; no promotion, no re-tuning of thresholds |

**On the two axes of "coverage", stated once so they cannot be conflated again.** *Containment coverage* (E18, §4.1) asks whether the scanner can see a whole container inside the block. *Producer-identity replayability* (E20, G6) asks whether the pinned engine can regenerate that stream's exact bytes. The ≥90% threshold is a threshold on **replayable stream bytes by producer identity**; E18's 100%-at-8-MiB result neither satisfies nor bears on it, and the frozen corpus (100% legacy producer, zero modern streams) provides no evidence on it at all. Containment is a necessary condition and a measured one; replayability is the binding condition and an unmeasured one.

**Falsification shortcuts that do not need the full run:** if a C pinned-engine replay of the frozen 2,331-stream population fails byte-identity on any stream, or if the shipped `R` is **< 40 MB/s**, the decode economics fail outright and the direction is dead before any corpus run.

## 11. Adversarial failure cases

| # | Case | Consequence | Handling |
|---|---|---|---|
| A1 | encoder replay misses (unknown encoder: libdeflate/zlib-ng/miniz, different zlib build, non-zlib DEFLATE writer) | region stays raw | fail-closed; 23/2,354 measured; v1 does **not** implement preflate-class corrections; upside +37–72 KB left explicitly unrealized **[P]** |
| A2 | replay engine drift (a "patched" zlib, a different `-O` level altering output, a vendored-source update) | **silent byte corruption or mass decode failure** — and once archives exist this is a **permanent one-way door**, not a manageable risk: `replay_engine_id` becomes ABI and an ordinary upstream patch becomes a data-loss event. F11 is in-corpus proof that DEFLATE output is not stable across encoder versions (23 streams, 8.78% of DEFLATE bytes, unreproducible by any zlib-1.3.1 setting) | immutable `engine_id`; golden self-test; frozen engine fingerprint in every artifact; a new engine is a **new ID**, never an in-place change; **the bar for opening this door should be higher than "1.36 MB on one 2005-era file"** |
| A3 | block straddles a container | transform 4 unavailable for that block | fail-closed; coverage must be reported as a function of `--block` (§4.1) |
| A4 | zip-bomb-shaped input (huge `usize`, tiny `clen`, declared sizes inconsistent) | inflated inflate work at **encode** time; at **decode** time the replay is O(`ulen`) — bounded by D4 + the mode-17 bound, **not** by `clen` (§8, corrected) | bounded inflate cap per stream at encode; global attempt budget; at decode, `ulen` capped by remaining transformed bytes and replay output hard-capped at `clen` during chunking |
| A5 | malicious ZIP: overlapping/duplicate headers, fake `PK` in payload, ZIP64, data descriptors, truncated CD, concatenated archives, self-extracting prefix | wrong region replayed, or crafted replay program | strict scanner, checked arithmetic, overlap rejection, v1 rejects ZIP64/encryption/descriptor layouts, synthetic adversarial fixture set in CI |
| A6 | `transformation_size` blow-up (plaintext ≫ compressed) | mode-17 bound `2·out_len+4096` violated | bound asserted at build time (§4.1 S2); the transform is simply unavailable for such blocks |
| A7 | replay output ≠ declared `clen` (engine/version skew reaching the decoder) | decoder-side divergence | mandatory per-record length check + final block-length check + existing CRC (D7) — three independent nets, **ordered so partial output is discarded, not merely flagged** (§8 requirement 2) |
| A8 | corpus concentration: a "win" that is really one file | aggregate claims become misleading | per-file rows mandatory (PB-17/PB-23); report mozilla and samba separately; never quote only the aggregate |
| A9 | decode-time cost surprise (`R` far below the Python proxy) | the Pareto point flips to dominated | gate G4 + the §10 falsification shortcut |
| A10 | reviewer objection: "just call preflate-rs" | direction judged not worth carrying | answered only by G3/G4 passing in ANVIL's own portfolio; no argument available |
| A11 | **prior-art control reproduces the win off the shelf** — precomp does exactly this transform with zero ANVIL decoder cost, zero binary delta and zero pinned-engine liability, on a canonical archive-heavy corpus | the entire PILOT is redundant | **now the first thing to measure, not a last-resort objection** (§10 arms C2a/C2b, gate G5). Cheap, decisive, requires no ANVIL code |
| A12 | modern producers (zlib-ng / libdeflate / miniz) are not byte-compatible with stock zlib, so a held-out family of *modern* archives may yield far fewer replayable bytes; F11 is in-corpus evidence that encoder-version drift breaks exact replay | transform 4 becomes byte-neutral on modern archives and the selection rule discards it; the mechanism degenerates into a narrow legacy-archive tool | §10 gate G3's held-out clause with an explicit **legacy-vs-modern producer split**; reclassified as the top generalization risk in §12 |

## 12. Recommendation — **HOLD**

**Verdict: HOLD** (downgraded from my earlier PILOT; now identical to the critic's, and adopted on the strength of my own gates rather than on new evidence).

**My promote gate is not met, and I am not moving it.** At the only replay rate this repository has ever measured (`R` = 44.793 MB/s, §5.2), the projected paired decode ratio is **1.753×** against a promote bar of **≤ 1.60×**, which requires `R ≥ 58.88 MB/s`. That point sits inside my own preregistered **HOLD band** (1.60×–2.20×). So: **HOLD**, explicitly, until a run produces either (a) a shipped `R ≥ 58.88 MB/s` from a byte-identical build, or (b) a coordinator decision, recorded **before** the run, that a different criterion binds.

**Not KILL**, because nothing yet measured is a value argument: the byte win is measured (E5), fail-closed with exactly zero downside, and non-dominated against every same-file reference in evidence (§5.2). **Not PILOT**, for two sequencing reasons that are not scientific objections: (i) my own promote gate currently fails on my own measurement, so authorising a vendored-zlib build and a frozen `replay_engine_id` ABI on that basis is premature; (ii) the decisive question — *is the win already available off the shelf?* — is answered by two cheap shell pipelines requiring **zero ANVIL code**, and running the expensive step first inverts the correct order. **Not PROMOTE-TO-REMOTE**: no integrated C decoder exists (E12), no C `R` exists, and dispatch is itself blocked on an authorization dependency (E17).

**Authorized now, before any implementation:** the §10 remote-only control run — arms C2a and C2b (**both** precomp pipelines, frozen together, not substitutable), C3a/C3b on the locked held-out family, C4 supplementary, plus C5 producer-identity coverage. All thresholds in §10 are frozen and were not moved after the critic's derivation was reproduced.

**Unlock conditions for re-entering PILOT** (all required): C2a and C2b both materially worse than B (G5 passes); producer-identity replayability ≥ 90% on the held-out family (G6 passes); shipped `R` measured on the remote runner with host/flags/fingerprint recorded; G1 and G2 green.

**Ranked uncertainties, re-ordered after reconciliation.** The critic is right that my previous ranking was wrong in kind, not just in order: I had closed a *containment* question and let it stand in for the *replayability* question.

1. **Producer-identity replayability of modern archives (OPEN, now first).** E20: the frozen corpus is 100% legacy zlib 1.3.1 and carries **zero** information about this; the only in-corpus evidence is that 8.78% of DEFLATE bytes were unreproducible by any zlib setting. zlib-ng/libdeflate/miniz are not byte-compatible with stock zlib, so a modern held-out family could yield far fewer replayable bytes and the transform would degrade to byte-neutral. This is the binding coverage gate (**G6**) and it is entirely unmeasured.
2. **Whether the prior art already delivers the win (OPEN).** `precomp → brotli q11` and `precomp → xz -9e` are both frozen controls (G5); each answers a question the other cannot, and both must clear. Cheap, decisive, no ANVIL code.
3. **`R` (OPEN).** G4 currently fails at the measured 44.793 MB/s; `R_max` (byte-identical faster build, admitted only by golden test) is the plausible route through, and is a hypothesis, not evidence.
4. **Decoder binary delta (OPEN).** 64 KiB budget vs 55–80 KiB estimated for a full build, 25–35 KiB for a specialised level-6-only build; E19 removes any substitution escape.
5. **Held-out archive family availability (OPEN, and a precondition for 1, 2 and G6).** If none can be locked and hashed before measurement, G6 is unmeasurable and the honest verdict stays "narrow legacy value", with the 0.64%-of-corpus aggregate (E9, §5.4) as the headline.
6. **The one-way door (OPEN, coordinator judgement).** `replay_engine_id` becomes permanent ABI on first ship, and F11 proves bit-exactness is not stable across encoder versions. Combined with the ruling that portfolio composition is not a novelty position (E16), the coordinator must decide whether this door is worth opening at all. **A lower-risk instantiation of the same economic content exists** — the correction-class explanation (preflate-class: transmit an engine-independent correction for bytes that differ from the original payload, so the decoder runs **no** compressor, has no frozen ABI, and degrades gracefully on unknown encoders). That variant is not mine to claim novelty for; it belongs to the tracks that own correction topology (09/10/13) and to track 20's kill team. I record it here only as the version I would fund if exactly one instantiation could be funded.

**Stated in advance as KILL, not HOLD:** any G1/G2 failure; engine drift on the golden self-test; `C2a` or `C2b` ≤ B + 50,000 B (G5); producer-identity replayability < 50% on the held-out family (G6); shipped `R < 40 MB/s`; mozilla byte gain < 500,000 B; decoder binary delta > 256 KiB. None is recoverable by re-tuning, because the thresholds are frozen here and the mechanism's economics depend on no tunable.

---

## 13. Point-by-point response to Fledge Alpha Free

**Process note first, because it bears on how the addendum should be read.** Addendum A reports 9 of 11 items as unresolved against "the lane's revised report (316 lines)". That line count is the revision containing E18/E19 and the §5.3 property-1 addition — i.e. it **predates my second edit round**, in which I applied the corrections to §5.2, §5.3, §7, §8 and §11. Every one of D1, D2, D4, D5, D6, D8, D10 and D11 is addressed in the current text; the addendum's line-level complaints ("§5.2 still projects only at R = 40/80/120", "§8 still says O(clen)", "A6 still asserts the bound without computing it") are true of that snapshot and false of this one. I state this as a sequencing fact, not as a rebuttal: **the critic's arithmetic was right both times**, and where the addendum and this section disagree, this section is the later text.

| # | critic item | my response | where |
|---|---|---|---|
| **D1** | measured `R` = 44.8 MB/s ⇒ 1.753×, failing G4's ≤1.60× | **Accept in full.** I reproduced the derivation independently from same-window inputs (carrier factor 1.113503, backend term 0.337775 s, `U_valid` 8,689,786 B, `t_replay` 0.194000 s, total 0.53178 s ⇒ 96.3 MB/s, ratio **1.753×**) and solved the gate: `R ≥ 58.88 MB/s` required. I had never evaluated my own gate on my own measurement. G4 stands unchanged and **fails**; the mechanism is in my own HOLD band. | §5.2 |
| **D2** | "plausibly non-dominated" vs G4 1.60× is internally inconsistent | **Accept.** The non-domination sentence was a *descriptive* claim about the point cloud that I failed to label as non-operative. **Binding criterion is and was G4.** At 1.753× it fails. | §5.2 |
| **D3** | no prior-art control arm | **Accept, and extended per coordinator ruling.** **Both** controls are frozen: **C2a** `precomp → brotli q11 lgwin30` (same backend; isolates marginal transform value) and **C2b** `precomp → xz -9e` (the already-frozen external control; answers "available off the shelf at all"). They answer different questions, so freezing both is what prevents a post-hoc control substitution; neither may replace the other. Gate **G5** KILLs if either lands within B + 50,000 B. Plus C4 (rebuilder, ceiling-only) and C5 (producer-identity coverage, free from the same runs). | §10 |
| **D4** | engine work is O(**ulen**) not O(**clen**); posture is worse, not better | **Accept — this was a real technical error in my §8 prose, not a wording issue.** Retracted verbatim; the bound now rests on `ulen ≤ remaining transformed bytes` plus `transformed_size ≤ 2·out_len+4096`. Net statement added: transform 4 **trades** "no nested-inflate bomb surface" **for** "a vendored compressor in the decode path"; zlib's CVE surface becomes ANVIL's. | §5.3 prop. 2, §8 |
| **D5** | replay input is 8,689,786 B, not 9,991,436 B | **Accept.** Corrected; my figure overstated replay cost by 15%. | §5.2 |
| **D6** | "+5.2%" excludes the offline search; real ≈ +9% at `A_max`=405 | **Accept.** Corrected with the missing term broken out, and `A_max` + realized attempt count are now **mandatory reported fields**, not prose. | §5.2, §10 |
| **D7** | binary delta likely 55–80 KiB vs the 64 KiB budget | **Already agreed** (it was the one item the critic withdrew in A.3). Estimate carried into §5.3 with the specialised-build mitigation. | §5.3 |
| **D8** | bound implies `U ≤ 2·B_out + 4096`; applicability anti-correlated with value | **Accept.** Now stated as a hard applicability boundary with the measured headroom (mozilla `U/B` = 0.1951, 1.796×) and the anti-correlation made explicit. | §7 |
| **D9** | producer-identity replayability is the binding generalization risk; E18 closed a *different* axis | **Accept — this is the sharpest correction in the addendum, and it fixes a category error rather than an ordering.** Containment ≠ replayability. The ≥90% threshold is on **replayable stream bytes by producer identity**; E18's 100%-at-8-MiB result bears on it not at all; and the frozen corpus is 100% legacy zlib with **zero** modern-producer streams (E20), so it is silent on the question. G6 is now a separate binding gate with its own KILL/HOLD bands, and this is ranked **#1**. | §2 (E18/E20), §4.1, §10 (G6), §12.1 |
| **D10** | pinned bit-exactness is a permanent one-way door; charge it as a cost, not a risk | **Accept.** Added as a cost row in §5.3 and as the standing condition in A2, with F11 as the in-corpus proof that DEFLATE output is not version-stable. | §5.3, §11 A2 |
| **D11** | streaming hard cap at `clen` unspecified (zlib has no output-abort API) | **Accept.** Three implementation requirements added: chunked hard-stop at `clen` into a `clen`-capped buffer; abort-**before**-append so no partial stream survives; `record_count` bounded below by carrier structure. My §5.3 property 1 (chunking is output-identical) is what makes the first implementable — the critic is right that I found the enabling fact without noticing the requirement it enables. | §8, §5.3 |

**Where I add something the critic did not ask for.** The addendum calls `precomp → xz -9e` "the *wrong* control for the marginal-value question". I agree it is the wrong *isolator*, and I agree `precomp` is the right tool for it (in-place payload substitution preserves byte identity, so it composes with a byte-identity-invariant transform; a container *rebuilder* does not — Fledge A.4.1). But per coordinator ruling the frozen xz control is **not** replaced: it is the deployment-recipe number and the "already available at all" test, and dropping it after the fact is exactly the post-hoc control substitution the ruling forbids. Keeping both also removes the incentive to pick whichever control flatters the candidate.

**One provenance defect the critic did not raise, which cuts against over-reading 1.753× in either direction:** `RESULTS.md` §4 records neither the host nor the zlib build for the 0.194 s replay measurement, so pairing it with a Windows-window `t_direct` is itself a cross-window splice. The 1.753× is therefore a projection on a partially unpaired input. This does not rescue the claim — it makes the *measurement* of `R`, with host/compiler/flags/fingerprint recorded, a preregistered output of the remote run rather than an argument.

**Net position after reconciliation:** the critic's verdict is correct and is now mine. The mechanism stays adopt-class with no novelty claim (E16); the frontier status stays **0 FRONT-CROSSING** and unchanged; the byte value is real and measured; and the two things that would decide it — the prior-art controls and producer-identity replayability — are both cheap, both frozen, and neither has been run.

---

*Provenance: every [M] row names a frozen artifact or commit in this repository. Rows E6 and E8 come from two different local measurement protocols (`tests/ratio-first-standard.csv`, `tests/auto-routing.csv`) and are **not** comparable to each other or to the remote Class B rows in `docs/I10-FRONTIER-RECON-2026-09-24.md`; they are used only as same-file order-of-magnitude brackets. All §5.2 throughput rows are projections and must be replaced by same-job paired remote measurements before any citation. E3, E18, E20 and the §5.2 replay-model arithmetic are static recomputations over frozen artifacts, not benchmarks; the helper scripts live at `scratch/swarm-tmp/i10_dflt_stats.py`, `scratch/swarm-tmp/i10_dflt_blocksize.py`, `scratch/swarm-tmp/i10_dflt_replay_model.py`. `RESULTS.md` §4 does not record the host or zlib build for the replay measurement (see §13). No local corpus benchmark, sweep, build, or fuzz campaign was run for this report; no existing file was modified; no thresholds were moved, added, or relaxed after data was seen; no prototype code was written (see §9 for the deferred file list and §12 for why coding now cannot change the verdict). Carries the binding labels `GRID-THIN` and `0 FRONT-CROSSING`.*