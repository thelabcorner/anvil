# Track 19 — Format/Security/Fuzz — Space Bunny Free (INTERIM CHECKPOINT)

**Agent:** Space Bunny Free · **Track:** 19 (format/security/fuzz) · **Date:** 2026-10-02
**Status:** FINAL constructive — reconciled with the Track 19 critic (`19-format-security-fledge.md`, ruling HOLD) and coordinator delta **D20**. **The authoritative ruling is §12 (HOLD); §9 is superseded.** §10 = Track 11 FLI bound audit; §11 = existing-code zigzag audit (both findings LOW).
**Scope discipline:** created only this file (+ optionally
`prototypes/swarm-2026-10-02/19-format-security/space-bunny/`). No existing file modified.
No commit/push/reset/clean/stash/restore/rebase. No local corpus benchmark, no heavy local fuzz.
All temporary work confined to `scratch/swarm-tmp/` or the prototype directory.

**Reading basis (provenance).** `docs/swarm-2026-10-02/MASTER-BRIEF.md`;
`docs/decoder-audit.md`; `FORMAT.md`; `src/anvil.cpp` at the live worktree
(`i10-aux-unbwt`); `docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md`;
`docs/audit-2026-09-07/04-architecture-and-safety.md`;
`.github/workflows/anvil-research-bench.yml`; `tests/benchmark-summary.csv`;
`tests/fuzz.py` (via invocation site + decoder-audit description).
Line numbers below are from the live tree at checkpoint time and MUST be re-pinned by
the coordinator if `src/anvil.cpp` moves before the remote run (FORMAT.md:907-910 already
requires exactly that re-pin discipline).

---

## 1. Strongest mechanism and falsifiable hypothesis

### 1.1 The gap I am attacking (the thing that is actually broken)

Every resource bound in ANVIL is a **compile-time constant baked into the decoder**, and
the *file-level* bound is a **byte-count proxy, not a work or memory bound**. There is no
caller-suppliable ceiling anywhere in the decode API.

Evidence (all from the live tree):

| fact | location |
|---|---|
| `Options` exposes only `decode_threads` (1..1024); there is no memory/work/budget knob | `src/anvil.cpp:3768`, `src/anvil.cpp:3796`, `src/anvil.cpp:4993` |
| File-level bound is `total <= (in.size()/7 + 2) * max_block`, i.e. *number of blocks* × *max block size* | `src/anvil.cpp:4855` |
| `max_block` is a format constant (64 MiB rev-1, 128 MiB rev-2) | `src/anvil.cpp:4850` |
| Initial output reserve is bounded (`min(total, max(1MiB, in.size()*256))`) — so **allocation is bounded, but total *work* is not** | `src/anvil.cpp:4859-4860` |
| The block loop runs until `out.size() == total`; `out` is a real heap vector that grows to `total` | `src/anvil.cpp:4869-4890`, `src/anvil.cpp:4928` |
| Match lengths inside modes 11/15 are capped at `kSparseMaxLen` (65536 per the audit), and RLZ match length is `varint + 4` with **no per-op cap beyond `raw_n`** | `src/anvil.cpp:2271`, `src/anvil.cpp:2897`, `src/anvil.cpp:1883-1884` |
| RLZ inner copy is a **byte-at-a-time `push_back` loop** with a capacity check per byte | `src/anvil.cpp:1885` |
| Existing per-mode bounds are absolute constants (grammar ≤768, nesting depth 1, `16*out_len+64`, `2*blen+4096`, 384 MiB ratio stream) | `FORMAT.md:929-965`, `src/anvil.cpp:1780-1857`, `src/anvil.cpp:3873` |

**Consequence (projection, not a measurement).** The declared-total bound has ~6–7 orders of
magnitude of slack relative to what any honest encoder can produce, and the block chain lets
an attacker choose `total` and then *pay for it*:

- Format-legal ceiling for a **65,536-byte** file:
  `(65536/7 + 2) × 64 MiB = 9,364 × 67,108,864 = 6.28e11 B ≈ 585 GiB`.
- Format-legal ceiling for a **1 MiB** file: `≈ 9.6e12 B ≈ 8.7 TiB`.
- The one already-tested probe in `docs/decoder-audit.md:44-50` only checks `total = 2^40`
  against `size_t`; it never tests the *slack*.

### 1.2 Falsifiable hypothesis (H1)

> **There exists a well-formed ANVIL rev-1 file of ≤ 64 KiB whose declared `total` is
> ≥ 1 GiB and whose block chain produces ≥ 1 GiB of decoded output, causing the current
> decoder to hold ≥ 1 GiB resident and to spend ≥ 2 s of CPU — while passing every bound
> currently written in `FORMAT.md:929-965`, and therefore while being *accepted* by every
> invariant the project has declared strict.**

H1 is decidable by one deterministic probe file. It is the whole experiment.

### 1.3 Mechanism: Verified Cost Admission (VCA)

**One functional, two consumers.** ANVIL already owns a measured *decoder-cost model* in
wire-adjacent units: the S6-1 whole-codec budget `J = L + λ·C_us`, `λ = 0.01`, with
per-codec costs in ns/byte (raw 0.1, rANS 6.0, Huffman 4.3, defexc 3.2, ctx 7.5) and
raw always a candidate (`FORMAT.md:799-812`, `src/anvil.cpp:1612-1643`). That functional is
an *encoder objective only*. VCA promotes the same functional to a **decoder admission
meter**:

1. **`ρ` — a cost functional over fields the decoder already parses.** No new bits. For each
   substream, `ρ += N_decoded × c_codec` (the `c` table above); for RLZ also
   `ρ += nops + bytes_copied`; for RePair also `ρ += rules`; for rev-2 BWT also
   `ρ += transformed_size × b + expected/8 × i`; for the block frame itself
   `ρ += 7 + plen` per header read. Everything is a function of already-parsed header fields,
   so metering is **O(1) per block, O(1) per substream**, no second pass, no wire change.
2. **`RU` — a monotone additive resource ledger**, incremented *before* any allocation is
   committed. Adding is associative and never decreases, so the meter is a single running
   scalar: boundedness of the whole decode reduces to one comparison.
3. **`C` — a caller-supplied ceiling** (`Options.decode_budget_ru`), defaulting to today's
   effective behavior expressed as one number. Admission is `RU ≤ C` checked at three
   points: file header (declared `total` and `in.size()`), each block header, each substream
   header — i.e. **before** the corresponding `resize`/`reserve`.
4. **Composability invariant (the actual contribution).** *If every mode's local bound is
   expressed as a `ρ` term with a proven constant, then the file-level bound is the sum, and
   a newly added mode inherits boundedness by contributing one registry row instead of
   requiring a new bespoke adversarial audit.* This is what tracks 01 (paged base+overlay
   dictionaries), 07 (BWT subblocking), 09 (TCOPY exact invariants), 11 (orbit programs)
   and 15 (cross-block memory) need from me and currently do not have: five separate
   hand-written audits have already been needed (F1–F5 in `docs/decoder-audit.md:191-242`),
   each with its own magic constant.

**Why this is not novelty-by-difference (and where it is).** Output caps and window caps are
adopt-class (brotli `BROTLI_DECODER_PARAM_LARGE_WINDOW` / ring-buffer policy; zstd
`ZSTD_d_windowLogMax` + `frameContentSize`; LZMA `dicSize` bounds; the generic
"decompression bomb" literature, which covers *output* amplification only). VCA's
defensible residue is the **interaction**, not the idea:
(a) the *same* measured functional is simultaneously the encoder's Pareto objective and the
decoder's admission meter, so the safety property is bought with money the encoder is
already spending; (b) the meter needs **zero wire bits and zero golden-set churn**, so it can
be retrofitted to a frozen revision-1 format whose "never change an existing mode's wire
semantics" rule (`docs/CONTEXT.md`, working agreements) would otherwise block any security
change; (c) boundedness becomes a **sum over a registry**, so it composes over an open-ended
mechanism portfolio. Claims (a)–(c) are the pre-registered novelty separators; (b) is the
one that is both novel-enough and cheap enough to be true on Monday.

### 1.4 Secondary mechanism: typed, region-located corruption containment

Today a single bad block aborts the entire file (`throw` → nonzero exit, no partial output),
so corruption blast radius = the whole artifact, and there is **no defined partial-result
semantics at all**. Proposal requiring **zero wire change**:

- **Fault taxonomy**: every rejection carries `(region_id = block index, class ∈
  {TRUNCATED, CHECKSUM, MALFORMED, BUDGET}, byte offset)`. Cheap to compute; the block
  loop already knows the index and cursor.
- **`--on-corrupt=abort|skip|resync`** with `abort` as today's default. `skip` emits nothing
  for the failing region and records an explicit **hole** `(region_id, out_offset, out_len)`.
  `resync` scans forward with a bounded window for the next header triple that passes a
  validator, then treats it as a hole. Hard rule: **never emit bytes for an unverified
  region** — no zero-fill, no "best effort".
- Why this matters for *scientific accounting*, not just security: the BWT prereg already
  classifies timeout/OOM/runner-kill as `BLOCKED_INFRA`/`INCONCLUSIVE_INFRA`
  (`docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md:310-312`). A single corrupt byte inside a
  50 MB corpus artifact therefore *destroys a gate run*, and the run is lost rather than
  degraded. `skip`/`resync` with hole records converts "gate destroyed" into "gate reported
  with N holes", which is a measurement outcome rather than an infrastructure loss.

### 1.5 Secondary mechanism: random access as an attack surface (currently *unstated*)

ANVIL has **no published index**; random access = scan
(`docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md:316-345` rules `RANDOM_ACCESS = NOT_CLAIMED`
and forbids any index claim without a separate charged prereg; `FORMAT.md:932` confirms no
index). That is currently *accidentally* safe — no index means no index-parsing attack
surface — but it hides a real residual threat that no existing document states:

> **Seek cost is `O(in.size()/7)` header reads per seek, and the block count is
> attacker-controlled up to `in.size()/7`. An application performing `N` seeks on a hostile
> file pays `O(N × in.size())` header reads.**

Two honest conclusions, and they point in opposite directions: (i) under VCA, `header_reads`
is a first-class `ρ` term, so a caller can bound total seek work; (ii) a *published index*
would *reduce* the asymptotic seek cost at the price of new parse surface — so any index
adopted by tracks 07/15 must route its own bytes and bounds through the same `ρ` table, and
must never be described as making the format "safer". This is the only claim I make about
random access, and it is a constraint on *other* tracks, not a competing mechanism.

### 1.6 Secondary mechanism: make "rejects cleanly" a testable property

The strongest harness gap, directly evidenced. `tests/fuzz.py`'s mutation phase asserts only
**"reject OR reconstruct identical bytes"** (`docs/decoder-audit.md:141-147`). It asserts no
bound on time, allocation, or work. The proof that this matters is in the same document: the
1 GiB substream allocation (`docs/decoder-audit.md:56-69`) was found by a **human-crafted
probe**, *not* by the fuzzer — the fuzzer had been running and passed. Therefore:

> **Add a resource-bounded fuzz mode in which every case runs under a hard allocation cap
> and a work oracle (a decoder-visible counter of bytes-produced and bytes-read), so
> "1 GiB of decode work from a 65 KiB input" becomes a test failure rather than a lucky
> observation.**

This is the cheapest item in the report and the one with the best evidence-to-effort ratio.

---

## 2. Measured evidence vs projection (separation, per doctrine 10)

**Measured / documented facts (not my inference):**
- All five decoder-audit findings and their remediations, including measured before/after:
  1 GiB substream alloc `188 ms` → reject in `11 ms` (`docs/decoder-audit.md:56-69`);
  `total = 2^40` → reject in ~ms (`:44-50`); arithmetic trailing-byte splice accepted then
  fixed (`:93-121`); RePair doubling grammar; RLZ addend wrap; nested 7/8 stack exhaustion
  (`:191-242`).
- Fuzz runs reported green: seed `0xA11E` 650 roundtrip + 5200 mutations; seed `0xBEEF` 1050
  + 10500 (`docs/decoder-audit.md:30-34`); rev-2: 1040 variants / 6240 mutations / 7
  deterministic cases (`docs/audit-2026-09-07/04-architecture-and-safety.md:196-211`).
- Fuzz *is* remote-runnable: `.github/workflows/anvil-research-bench.yml:140-146` invokes
  `tests/fuzz.py --cases N` on `ubuntu-24.04`, job timeout 240 min. It is **not** currently
  sanitizer-enabled (`fuzz.py` is invoked directly; no ASan/UBSan job exists in
  `.github/workflows/`), and the invocation passes **no** `--mutations` flag, so the
  mutation phase that the audit relies on is **not** what CI runs today.
- Aggregate measurement CSVs carry no decoder-build, format-revision, or resource-ceiling
  column (`tests/benchmark-summary.csv:1-28`: `codec,total_ratio,...,roundtrip`). Host
  provenance lives separately in `tests/host-spec.md`. So a remote ratio/throughput row is
  currently not self-identifying about *which decoder contract* produced it.
- The BWT independent-region prereg already fixes the memory-budget and random-access-honesty
  discipline I intend to reuse, and already forbids uncharged index claims
  (`docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md:213-267`, `:316-345`).

**My projections (explicitly labelled, none measured by me — local benchmarking is forbidden):**
- The 585 GiB / 8.7 TiB format-legal `total` ceilings (§1.1). Arithmetic from
  `src/anvil.cpp:4850,4855`; I did not run it.
- The achievable amplification range from a ≤64 KiB file: **≈4×10^4 (mode 11, raw token
  substream)** up to **≈10^6 (RLZ-expanded token substream inside modes 10/11/15)**. I traced
  `kSparseMaxLen` (65536) at `src/anvil.cpp:2271,2897` and the uncapped RLZ match at
  `src/anvil.cpp:1883-1884`, and codec 8 is reachable from any mode-11-style substream
  because `decode_stream` dispatches codec 7/8 at `src/anvil.cpp:1866-1891`. I did **not**
  trace mode 15's exact token-layer framing to confirm end-to-end reachability.
- Cycle/RSS numbers in §3 are **derived**, from the byte-at-a-time loop at
  `src/anvil.cpp:1885` and `std::vector` growth semantics, not measured.

---

## 3. Full byte / cycle / RSS accounting (decoder code and state)

### 3.1 Decoder state the meter must charge (all of it)

| state | bytes | where |
|---|---|---|
| `out` output vector, grows to `total` | `total` | `src/anvil.cpp:4860,4889,4902` |
| per-block `b` (generic path) or `out` slice (fused path 10/15) | `blen` (≤64/128 MiB) | `src/anvil.cpp:4879-4889` |
| `segs` segment table (threaded path) | `32 × nblocks` | `src/anvil.cpp:4893-4904` |
| `blocks[i]` per-thread vector (threaded path) | `total` | `src/anvil.cpp:4907` |
| mode-11 seven substreams, eager | `≤ 7 × (16·out_len+64)` | `src/anvil.cpp:1828-2013` |
| RLZ `out` + byte-at-a-time copy | `raw_n` | `src/anvil.cpp:1875,1885` |
| RePair per-rule expansions, cumulative ≤ `2·raw_n + 64 KiB` | `2·raw_n + 64 KiB` | `src/anvil.cpp:1855-1857` |
| ctx-rANS `csymtab` | **49,152 B (48 KiB), a compile-time CONSTANT** = `kCtxK(12) × 4096` × 1 B. `cctx.K > kCtxK` is rejected, so this is *not* attacker-controllable — a correct bound, and the model row `rho` would need | `src/anvil.cpp:1222,1924,1985,1999`; `FORMAT.md:626` |
| Huffman 12-bit table | `4096 × 2` | `src/anvil.cpp:2046` |
| BWT inverse work array (≈4×N per the audit) | `≈ 4 × expected` | `docs/audit-2026-09-07/04-architecture-and-safety.md:160-167` |

### 3.2 Byte cost of the *attack* (derived, rev-1, 64 KiB attacker file)

Per 64 MiB block, using mode-11 with the token substream coded as RLZ (codec 8):
1024 match tokens of `len = 65536` → token stream ≈ 1024 × 6 B ≈ 6 KiB; RLZ encodes that
with ≈ 32 ops of `1 + varint(dv) + varint(lv)` ≈ 5 B/op ≈ **160 B**; plus RLZ/substream/
block framing ≈ 20 B. **≈ 200 B per 64 MiB block ≈ 3.4×10^5 amplification per block.**
With the cheaper plain (non-RLZ) token substream, ≈ 6 KiB per block ⇒ **≈ 1.1×10^4**.
`65536 B / 200 B ≈ 327` blocks ⇒ **`total ≈ 21 GiB`** from a 64 KiB file.

### 3.3 Cycle cost (derived; the RLZ loop is the bottleneck)

`src/anvil.cpp:1885` is `for(t=0;t<len;++t) out.push_back(out[out.size()-dist]);` — a
per-iteration capacity check plus `size()` reload, so it is **not** vectorizable; central
estimate **3 cycles/byte** (range 2–4).
- 21 GiB = 2.26e10 B × 3 = **6.8e10 cycles** ⇒ **≈ 23 s at 3 GHz**, **≈ 45 s at 1.5 GHz**.
- The same work also pays `crc32` over every decoded byte (`src/anvil.cpp:4882,4888`), which
  on this host is a PCLMUL-assisted pass; add **≈ 0.1–0.3 cycles/byte**, i.e. <10 % of the
  total. The RLZ loop dominates by ~10×.

### 3.4 RSS cost (derived)

- Steady state `out.size() = total ≈ 21 GiB`.
- `reserve_hint` starts at only 16 MiB (`min(total, max(1MiB, 64 KiB × 256))`,
  `src/anvil.cpp:4859`), so growth is geometric; `std::vector` reallocation makes old and new
  buffers coexist ⇒ **transient peak ≈ 1.5 × final ≈ 31 GiB**.
- Threaded path adds a second full copy: `blocks` totals `total`, then `out.clear()` +
  `insert` re-grows to `total` ⇒ **peak ≈ 2.0 × total ≈ 42 GiB** (`src/anvil.cpp:4907,4924-4925`).

### 3.5 Why this breaks the *project*, not just a victim

`.github/workflows/anvil-research-bench.yml:55` sets `timeout-minutes: 240` on
`ubuntu-24.04` (7.5 GiB RAM). A 64 KiB corpus artifact of this shape ⇒ ~31 GiB peak ⇒
runner OOM-kill ⇒ classified `BLOCKED_INFRA`/`INCONCLUSIVE_INFRA`
(`docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md:310-312`) ⇒ **an adversarial input can convert
a valid gate run into an uninterpretable one.** That is the bridge from "security bug" to
"scientific accounting failure", and it is why this track is not optional polish.

---

## 4. Strongest disconfirming evidence against my own direction

I am obliged to state the ways this dies.

1. **H1 may be non-constructible.** I traced `kSparseMaxLen` and the RLZ op decoder, but I
   did **not** trace mode 15's or mode 11's full token-layer framing to prove a hostile
   200-byte block satisfies every per-mode check *and* lands a matching CRC. If the substream
   framing forces a 16-bit minimum token encoding or a per-block model header that dominates,
   the achievable amplification could fall by 1–2 orders of magnitude — and at ~10^3
   amplification a 64 KiB file still only reaches ~64 MiB of output, which is **not** a
   finding. **H1 is therefore all-or-nothing around the ~10^4 threshold, and I have not
   cleared it.** This is the single uncertainty that can flip the verdict, and it is exactly
   why the experiment below is a probe-construction test, not a benchmark.
2. **CRC-32 already certifies decoded bytes** (`src/anvil.cpp:4882,4888`;
   `FORMAT.md:32`, `:795-797`). My "semantic integrity" instinct is therefore *already
   satisfied* — I explicitly withdraw it as a contribution. Anyone reading a variant of this
   report that claims output-certification novelty should discard it.
3. **`ρ` may be a duplicate of what the audit already achieved by hand.** The audit's five
   remediations *are* per-mode `ρ` bounds, just written as code instead of as a table. If the
   only remaining value is "put the constants in a table", this is bookkeeping, not
   mechanism — adopt-class, and the correct recommendation collapses to PILOT of the §1.6
   harness item only.
4. **The cost table is a liability for reproducibility.** `c_codec` values
   (`FORMAT.md:804-806`) were calibrated on one machine (`src/anvil.cpp:1612-1643` comments
   cite a specific t3 floor profile). If `C` is expressed in machine-specific ns/byte, then
   "decode the same artifact under a different ceiling" becomes a *different* experiment
   unless the units are pinned — which risks manufacturing exactly the kind of
   measurement-splicing the coordinator forbids. Mitigation: `C` must be defined in
   **machine-independent units** (bytes produced, bytes read, allocation bytes, op counts),
   with ns/byte used only as a *documented, separately-reported* projection.
5. **Prior art may be closer than I think.** The exact combination "meter the decoder's own
   work and refuse beyond a caller budget" appears in brotli's ring-buffer/reallocation
   policy, zstd's `ZSTD_d_windowLogMax` + content-size reservation, and 7-Zip's
   `kDicSizeMax`. If a reviewer shows any of them *also* meter per-operation decode work in
   a caller-visible unit, VCA's residue collapses to §1.4/§1.6 and the recommendation must
   drop to PILOT.
6. **Contention with peers.** Track 07 owns BWT memory budgeting, track 15 owns random
   access, track 01 owns dictionary paging. VCA's registry-sum claim is only useful if they
   *adopt* it; if they each keep bespoke bounds, VCA is an unused abstraction and the honest
   recommendation is HOLD, not PILOT.

---

## 5. Minimum prototype (isolated, optional, not wired into production)

Path: `prototypes/swarm-2026-10-02/19-format-security/space-bunny/`

**`ru_probe.cpp` — a static resource-ceiling linter for `.anv` artifacts.** Single
translation unit, no dependency on `src/anvil.cpp`, no decode. It walks only the framing
(`ANV0`, revision, `block_size`, `total`, then per-block `blen`/mode/`plen`/CRC and skips
`plen` bytes) and reports:

```
declared_total, header_reads, framed_payload_bytes,
amplification_bound   = (in.size()/7 + 2) * max_block,
legal_total_ceiling   = min(declared_total, amplification_bound),
rho_static            = sum over blocks of (7 + plen) + per-block mode rho row,
verdict               = OK | OVER_BUDGET | ILL_FORMED
```

Why this is the right minimum: it computes the attack surface **from headers alone**, so it
works on truncated, garbage, and hostile files without a valid payload; it needs no encoder;
it is exactly the missing number in `docs/I10-BWT-INDEPENDENT-REGIONS-PREREG.md:213-267`
(a *measured* memory budget) and in the crash-free-accounting column set of
`tests/benchmark-summary.csv:1`. Expected size: < 400 lines.

**Second, smaller artifact:** `probe_gen.py` — emits the deterministic H1 probe family with
both the `.anv` and its *expected* `total`, block count, and SHA-256, so the probe is
self-describing and correctness does not depend on my reading of the framing. Emits only;
never decodes locally (doctrine 7).

I have **not** yet written these. They are gated on the verdict below.

---

## 6. Decisive remote-only experiment (pre-registered)

**Name:** RU-F1 — resource-ceiling falsification, single remote job.
**Where:** GitHub Actions `ubuntu-24.04`, one pinned checkout, one pinned toolchain — no
cross-job comparison; nothing spliced with any existing benchmark window.
**Why remote:** local corpus benchmarking and heavy fuzzing are forbidden (doctrine 7); this
experiment is small (a handful of probe files) but measures RSS and CPU, both of which are
host-specific and already established as measurement-sensitive
(`docs/anvil-i9-findings.md`, `tests/pareto-i9-decode-remeasure.md`).

**Arms**
- **A0 (control).** 65 deterministic valid files: 16 corpus sources × {sparse, hotop,
  hotop --rlzp, arith, ratio, random-truncations}, each round-tripped. Establishes that the
  harness is not the thing under test.
- **A1 (H1 probe).** Probe family `P_{n}` for `n ∈ {64 KiB, 256 KiB, 1 MiB}`, built by
  `probe_gen.py`, each a hostile-but-well-formed rev-1 file targeting a declared
  `total` ∈ {1 GiB, 8 GiB, 64 GiB}.
- **A2 (bounded-mode control).** The same probes under the VCA rule applied at the three
  admission points (`total` at header, `ρ` at block, `ρ` at substream) with
  `C = 64 · in.size()` (machine-independent units). Must reject **before** the first
  growing allocation.
- **A3 (partial-result semantics).** `abort` vs `skip` vs `resync` on a file whose block 7 of
  40 has a flipped CRC byte. Asserts hole records, and asserts **no** bytes emitted for the
  failing region.
- **A4 (harness fix).** `tests/fuzz.py` under a 512 MiB `RLIMIT_AS` cap plus a work oracle,
  running the existing mutation phase at seeds `0xA11E` and `0xBEEF`. A4 must be a
  *no-regression* control for the existing green numbers.

**Measured, per case:** wall-clock ms, `/usr/bin/time -v` peak RSS, exit status, diagnostic
string, decoder bytes-produced counter, and — for A1 — whether the attacker's target was
actually reached.

**Pre-registered thresholds (frozen before the run).**

| gate | metric | PROMOTE | HOLD | KILL |
|---|---|---|---|---|
| RU-F1-G1 | A0 round-trip | 65/65 byte-exact | — | any failure |
| RU-F1-G2 | **H1 reachability** (A1): peak RSS on the 1 MiB probe | ≥ 1 GiB **and** ≥ 2 s CPU **and** probe accepted under all `FORMAT.md:929-965` bounds | 64 MiB–1 GiB, or CPU < 2 s | peak RSS < 64 MiB **and** CPU < 2 s ⇒ H1 dead |
| RU-F1-G3 | A2 rejection | 3/3 reject, rejection strictly precedes the first growing allocation, wall clock ≤ 1 % of A1 | reject but after ≥ 1 allocation growth | any acceptance |
| RU-F1-G4 | A3 semantics | holes exact, zero bytes for the failed region, identical `abort` output to today | correct but with an off-by-one region index | any bytes emitted for an unverified region |
| RU-F1-G4b | A4 no-regression | mutation phase green **under the 512 MiB cap**, plus ≥ 1 newly-caught case or a documented proof none exists | green, no new catches | existing phase regresses under the cap |
| RU-F1-G5 | A1/A2 rate cost on real corpus (charge every byte, doctrine 2) | header cost ≤ 0.05 % of compressed size and ≤ 0.2 % of decode time, both from the same job | ≤ 0.5 % | > 1 % |
| RU-F1-G6 | **novelty separator** (reviewer judgement, recorded not assumed) | ≥ 2 of the 3 residues in §1.3 hold after a prior-art pass | exactly 1 | 0 |

**Explicit non-negotiables.** RU-F1-G4b is a *no-regression* gate: the resource-bounded fuzz
mode must not weaken the existing reject-or-identical property. RU-F1-G5 must be measured in
the **same job** as G2/G3 (no cross-window splicing). If G2 and G6 disagree, the rule is
**HOLD** — a real bug with adopt-class novelty is a patch, not a mechanism, and must not be
reported as a crossing.

---

## 7. Adversarial failure modes of the mechanism itself

The meter must fail *closed and safely*, and it must not become an attack in its own right.

1. **Ceiling too low rejects honest files.** Mitigation: `C` defaults to today's behavior;
   every rejection names the ceiling and the observed `RU` so the cause is legible.
2. **`ρ` under-counts a future mode.** This is the compositional claim's failure mode. A
   registry row with a wrong constant silently breaks the bound. Mitigation: require the
   registry to be *exhaustive by construction* — a mode id with no `ρ` row is a decode-time
   rejection, and a fuzz case must exist per row. Enforcement reuses the existing
   registry-coverage discipline (`FORMAT.md:900-927`), which already requires a forced
   round-trip per decoder-visible id.
3. **Meter itself becomes an attack.** An attacker inflates `ρ` cheaply to force honest
   decoders to reject valid-looking files. Mitigation: `ρ` is *computed from* fields, never
   *read from* the wire — there is no declared-cost field to forge. This is the reason the
   meter must be derived, not transmitted; it is also why VCA needs zero wire bits.
4. **`resync` mis-anchors inside payload bytes.** A hostile payload can contain bytes that
   look like a valid header triple. Mitigation: resync must require the candidate header to
   parse *and* to have a passing block CRC after decode, or to be followed by ≥ 1
   well-formed block; anything else is treated as trailing garbage and the region stays a
   hole. Residual risk accepted and must be documented, not hidden.
5. **Ceiling divergence across machines corrupts comparability.** Mitigation: `C` in
   machine-independent units (mitigation 4 above), ns/byte reported separately.
6. **`total` is a *declaration*, so admission on `total` alone is still a proxy.**
   Honest limitation: RU-F1-G3 makes rejection happen early and cheap, but the format cannot
   make `total` trustworthy. Only a real `ρ`-to-completion accounting could, and that is
   future work, not claimed here.
7. **Fuzz under a memory cap changes what the fuzzer finds.** A cap can mask a
   slow-drip allocation that a real deployment would still suffer. Mitigation: report both
   the capped and uncapped (small-case) runs and never treat "green under cap" as "bounded".

---

## 8. Format evolution strategy that preserves scientific accounting

Six rules, each with its enforcement mechanism and its scientific-accounting payoff.

| # | rule | enforcement | accounting payoff |
|---|---|---|---|
| E1 | **Append-only ID namespaces.** New mode / substream / postcoder ids only; never repurpose or reorder. | existing `registry_coverage_roundtrip` (`FORMAT.md:900-927`) extended to require the malformed probe in the *same* change | no artifact ever changes meaning silently |
| E2 | **The `ρ` table ships with the id.** No id without a cost row; unknown row ⇒ reject. | registry table is the dispatch table | boundedness can never go stale relative to the id space |
| E3 | **Separate `capability` from `grammar version`.** Today revision 2 hard-rejects any mode ∉ {0,17} (`FORMAT.md:12-18`), so each new mode costs a new revision. Propose revision = (core grammar, capability mask), so a new mode is refused by *unknown-id*, not by *revision*. | header field; golden-set re-pin required | removes the "we need a new revision" pressure that currently blocks additive evolution |
| E4 | **Per-row provenance in the measurement record.** Every benchmark row carries `(format revision, capability mask, ρ-table version, decoder build sha256, C, peak RU observed)`. | new columns in `tests/benchmark-*.csv`; precedent exists in `tests/host-spec.md` | a ratio/throughput row becomes self-identifying; today's CSV (`:1-28`) is not |
| E5 | **Malformed-Input Conformance Table (MICT).** Every reject class gets a row: condition, required observable behavior, diagnostic class, and its **resource-ceiling obligation** (bytes read ≤ c × input, allocations ≤ C). Negative space is mandatory — the table must also state which behaviors are *not* promised (e.g. no authenticity against a hostile encoder; CRC-32 covers decoded bytes only, `FORMAT.md:795-797`). | MICT is the fuzz matrix's row set | converts the audit's prose into a testable, versioned contract |
| E6 | **Freeze the measurement window per gate.** Any run that touches RU-F1 inherits its own job, its own pinned toolchain, and its own provenance row; results are never combined with `tests/benchmark-suite.*.csv` or the BWT windows. | documented in the run's artifact manifest | no cross-window splicing, per coordinator instruction |

**Explicitly deferred (wire-changing, would churn all 65 goldens):** replacing per-block
CRC-32 with a wider digest or a per-region Merkle root for third-party *subset* certification.
Named here so a later track does not rediscover it and assume it is unclaimed; it is
**not** claimed by this report.

---

## 9. Current recommendation

> **SUPERSEDED by §12.** On pair reconciliation with the Track 19 critic
> (`19-format-security-fledge.md`, ruling **HOLD**) and coordinator delta **D20**, this
> section's `PILOT` is withdrawn. The *diagnosis* in §1.1, §1.6 and §2 is confirmed and
> retained; the *mechanism* (VCA) and the ordering were wrong. Read §12 for the authoritative
> recommendation, the single decisive probe, and the caller-side ceiling contract. §1–§8 are
> unchanged and remain the evidence base; §10 and §11 are retained as cross-lane audits with
> severities revised per §12.1.

# **PILOT** — of a strictly bounded slice, remote-verified, with an explicit kill surface.

**Why not KILL.** Two independent, code-evidenced gaps survive scrutiny today: the file-level
ceiling is a byte-count proxy with ~6–7 orders of magnitude of slack and no caller-suppliable
alternative exists (`src/anvil.cpp:3768,3796,4850,4855,4859`); and the mutation fuzzer that
the project relies on demonstrably failed to catch a 1 GiB allocation that a human probe did
(`docs/decoder-audit.md:56-69,141-147`). Both are real and neither is closed.

**Why not PROMOTE-TO-REMOTE.** H1 has **not** cleared its own construction bar (§4.1). I
traced the match-length caps and the RLZ op decoder but not the full token-layer framing, and
the finding is all-or-nothing around the ~10^4 amplification threshold. Promoting an
unconstructed DoS claim to a remote job would burn a runner on a hypothesis I have not
earned. Additionally, three of the six disconfirming arguments (§4.3, §4.5, §4.6) can each
independently collapse VCA's novelty residue to bookkeeping, and I have not yet run a
prior-art pass against brotli/zstd/7-Zip metering.

**Why not HOLD.** The §1.6 harness fix and the §1.4 containment semantics are cheap, have
the strongest evidence per unit of effort in the whole report, do not depend on H1, and do
not compete with any peer track's lane.

**The bounded slice I would pilot, in priority order:**
1. **A4 — resource-bounded fuzz mode** (work oracle + `RLIMIT_AS`), and add
   `--mutations` to the existing CI fuzz invocation, which today omits it
   (`.github/workflows/anvil-research-bench.yml:143-146`). Best evidence, lowest cost, no
   dependency on H1, and it makes every future format claim cheaper to trust.
2. **§1.4 typed fault taxonomy + `abort|skip|resync`** — zero wire change; converts a
   destroyed gate run into a reported one.
3. **§1.5 `header_reads` as a `ρ` term**, delivered as a *constraint note* to tracks 07/15
   rather than as code, since neither publishes an index today.
4. **§1.3 VCA**, held behind **RU-F1-G6**: run the A1/A2 arms, but treat the result as
   *novelty evidence* only. If G2 lands and G6 fails, the honest outcome is a security patch
   with a documented attack, reported as such — **not** as a Pareto-frontier contribution.
5. **`ru_probe.cpp`** as a static linter, so the ceiling is a reported artifact property
   rather than an internal constant.

**What would move me off PILOT, in either direction:**
- → **PROMOTE-TO-REMOTE**: `probe_gen.py` emits a probe that provably clears
  `kSparseMaxLen` + substream framing, i.e. I have *constructed* rather than *argued* the
  ≥10^4-amplification file. That is the single cheapest action available to me and it is the
  only thing that can move the headline.
- → **HOLD**: a peer track (07/15/01) declines the `ρ`-registry contract, removing VCA's
  only compositional argument and leaving bookkeeping.
- → **KILL**: RU-F1-G2 returns < 64 MiB and < 2 s on all three probes, or a prior-art pass
  shows brotli/zstd/7-Zip already meter per-operation decode work in a caller-visible unit.

**Pending coordinator decisions:** (i) whether `total`'s byte-count proxy should be tightened
regardless of VCA's novelty outcome — that is a one-line defensive change with its own
gate, and it is arguably independent of this track's mechanism question; (ii) whether E3
(capability mask) is worth a golden-set re-pin, since it is the only rule here that costs
existing artifacts.

**Explicit non-claims.** No ratio, throughput, or Pareto claim is made. No measurement from
any existing benchmark window is reused or combined. Every amplification, cycle, and RSS
number above is a derivation from cited lines and is labelled as such.

---

## 10. Cross-lane bound audit — Track 11 FLI (§9 malformed-loop probes)

**Requested by:** coordinator, security cross-lane review of
`docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md` (FLI, "Fused Loop Instruction"),
auditing whether its stated bounds are *sufficient* across the eight named classes.
**Audited artifact:** track 11 checkpoint only. **I did not modify track 11's file.**
**Verdict: FLI's bounds are NOT sufficient.** 2 critical guard defects, 1 hard wire defect,
1 project-invariant breach, 5 mandatory clarifications. Three of the nine (MG-1, MG-2, MG-5)
are defects in FLI's *specification*, not in its implementation — it has none yet — so they
are cheap to fix now and expensive later.

Line references are to `docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md` and to the
live `src/anvil.cpp` at checkpoint time (re-pin per `FORMAT.md:907-910` if the tree moves).

### 10.1 Per-class audit

| # | Class | FLI's stated bound | Sufficient? | Finding |
|---|---|---|---|---|
| 1 | `n` overflow | `n >= 2 and n <= KITER_MAX` (`:137`); probes `n = 0`, `n = KITER_MAX+1` (`:481`) | **Yes**, with one caveat | `n` is a uvarint whose domain is the full u64 (`get_uvar` accepts ten-byte encodings to `UINT64_MAX`; house precedent for this exact hazard at `src/anvil.cpp:1882`), so `n <= KITER_MAX` is a u64 comparison and rejects safely. Caveat: FLI never pins the **iteration-counter width**; if `n` is u64 and the loop counter is u32, `KITER_MAX` must be ≤ 2^32−1. Covered by MG-7. |
| 2 | Arithmetic `L/D` overflow | "L0 += dL and D0 += dD after each iteration" (`:148`) — **no type, no width, no cap, no post-check** | **NO — CRITICAL** | See MG-2. Worse, it interacts with class 4 to become a **guard bypass**. |
| 3 | Overlapping copy semantics | "emit_copy(D0, L0) periodic if D0 < L0 (as mode 11-15)" (`:142`) | **Intent correct, precondition unstated** | See MG-9. Also: in `LOOP_ARITH` the split test must be re-evaluated **every iteration** because `L0` drifts; FLI says "identical", which is ambiguous. |
| 4 | Block-end multiplication overflow | `require pos + L0 <= block_end` every iteration (`:141`, `:149`, Lemma 3 `:257`) | **NO — CRITICAL** | See MG-1. FLI uses the **addition** form; the entire ANVIL codebase uses the **subtraction** form (`b.len > out_len - out.size()` at `src/anvil.cpp:2920,2939`; `dist > out.size()` at `:2949`). The addition form wraps exactly when `L0` does, so the guard fails open. |
| 5 | Unset `last[shape]` | "require set and D0 in [1, pos]" (`:139`); A5 "reject, same rule mode-15 hot kind-1 already enforces" (`:518`) | **Yes**, one correction | House pattern confirmed sound: `last{}` zero-init with `0` as the unset sentinel, explicit `if (ld == 0) throw "macro reuse/delta before absolute"` (`src/anvil.cpp:2946,2947`), and `dist == 0` always rejected (`:2925,2945,2949`). Correction: the bound must apply to the **accumulated** escape class, not the per-escape byte — see MG-6. |
| 6 | Stream truncation | A3 "S_n truncated / varint wrap … uvarint canonical-form + length checks" (`:516`) | **Half sound, half false** | Truncation **is** handled: `get_uvar` does `if(p>=e) throw "truncated varint"` every iteration and `throw "varint overflow"` after ten (`src/anvil.cpp`, `get_uvar`). But **canonical-form checking does not exist anywhere in ANVIL** — `get_uvar` accepts overlong encodings (`0x80 0x00` = 0). A3's stated mitigation is not implemented. See MG-8. The far more serious class-6 gap is MG-4. |
| 7 | Opcode alphabet corruption | A12 "K ≤ 254; adding ≤ 4 loop codes … bounded, charged in §5.1" (`:525`) | **NO — HARD WIRE DEFECT** | See MG-5. A12 asserts boundedness without reaching the byte-representability cliff. |
| 8 | Amplification ceilings | "≤ KITER_MAX · LEN_MAX per instruction, and ≤ declared blen; no new amplification class" (`:320`, Lemma 4 `:261`) | **Inherits MG-1/MG-2, and understates the delta** | The "≤ declared blen" half is enforced *only* by the class-4 guard, which MG-1 shows is bypassable. And "no new amplification class" is **true in kind, false in degree** — see §10.3. |

### 10.2 Mandatory guards to feed back into track 19 (MG-1 … MG-9)

**MG-1 — block-end guard form (CRITICAL).** Replace `pos + L0 <= block_end` with the house
subtraction form **`L0 <= block_end - pos`**, every iteration, both opcodes. Given the
invariant `pos <= block_end`, the subtraction form cannot overflow for any `L0`, so a wrapped
or garbage `L0` fails closed instead of open. Rationale is not stylistic: MG-2 shows `L0` is
attacker-drifted, and under the addition form a drifted `L0` disables the very guard that
contains it.

**MG-2 — arithmetic drift typing and cap (CRITICAL).** FLI must specify, normatively:
`δL`/`δD` decoded by zigzag from a uvarint **with a pre-decode cap on the varint itself**
(mirroring `if (dv >= 0xFFFFFFFF) throw "bad macro abs dist"` at `src/anvil.cpp:2945`);
the recurrence accumulated in **`int64_t`**; post-checked on **both** sides — `D_i` into
`[1, 0xFFFFFFFF]` and `L_i` into `[1, LEN_MAX]` — exactly as `if (dd <= 0 || dd > 0xFFFFFFFF)`
at `src/anvil.cpp:2947`; and the narrowed result re-validated **before** it is used as an index
or a length. FLI currently specifies none of this.
*Note for track 11:* the house zigzag itself has a residual instance of this class —
`(zz & 1) ? -int64_t((zz + 1) >> 1) : int64_t(zz >> 1)` at `src/anvil.cpp:2947` applies no cap
to `zz`, so `zz = UINT64_MAX` makes `(zz+1)` wrap and the delta silently decode as `0`. That is
benign (delta 0 is legal, and CRC catches a mis-decode) but it is direct evidence that the
zigzag path is **not** overflow-hardened, so FLI must not assume it inherits that safety.

**MG-3 — `L_i >= 1` every iteration (MANDATORY, FLI-specific).** `LOOP_ARITH` with `δL = -1`
drives `L0` to 0 at iteration `L0_0`; the remaining `KITER_MAX − L0_0` iterations then perform
dispatch and bounds-checking with **zero bytes produced**. The entry-time house check
(`len == 0 || len > kSparseMaxLen → throw`, `src/anvil.cpp:3004`) does **not** cover a
mid-loop value. Require `1 <= L_i <= LEN_MAX` on every iteration. This is the only
decode-time amplification vector that is FLI's own, and it is a pure-work attack: bounded
memory, unbounded (up to `KITER_MAX × LEN_MAX`) useless cycles.

**MG-4 — substream full-consumption extension (MANDATORY, highest project-level severity).**
FLI adds three new materialized substreams `S_n`, `S_dl`, `S_dd` (track 11 `:134`) but does
**not** extend the block's full-consumption contract to them. The live contract is
`if (out.size() != out_len || ip_mt != s[1].size() || … || ip_res != s[8].size()) throw
"substream consumption mismatch"` (`src/anvil.cpp:2979-2981`), plus
`if (p != e) throw "payload trailing bytes"` (`:2911`). FLI's new streams must be added to
**both**. Consequence if omitted: bytes hidden in the unused remainder of `S_n`/`S_dl`/`S_dd`
are accepted and silently discarded. This is **not** a memory-safety bug — it is a
**scientific-accounting** breach and therefore in my track's mandatory scope, on two counts:
(a) it breaks the format's central evolution invariant that "every selectable state was
already a legal file before" any selector (`FORMAT.md:782`), and (b) it creates a
**byte-laundering channel past the J accounting**, since bytes inside a discarded stream
remainder are decoder-visible-in-the-file but not accounted in any rate or cost objective.
That is precisely the "charge every decoder-visible byte" doctrine, and it is invisible to
every existing test in the repo, because CRC-over-decoded-bytes cannot see it.

**MG-5 — opcode byte representability (HARD WIRE DEFECT).** `K` is **not** a constant: it is
a per-block uvarint read from the payload (`uint64_t K = get_uvar(p, e)`,
`src/anvil.cpp:2994`) capped at `kHotMaxOps = 254` (`:2797`), and the opcode stream is a
**byte** stream (`for (uint8_t op : s[0])`, `:2915`). So FLI's `K+1 … K+4` are only
representable while `K <= 251`. At `K = 254` the opcode `K+4 = 258` does not exist in the
alphabet and `K+3 = 257` collides with the wrap of the byte stream. Mandatory: **reject any
block that declares loop opcodes when `K + 4 > 255`** (i.e. `K > 251`), or reserve the top of
the alphabet by lowering `kHotMaxOps`. A12 must be rewritten to state the byte cliff.
*Credit where due — one positive finding for track 11:* there is **no collision** with the
existing alphabet. The live dispatcher uses exactly `0..K` (`op < K` at `:2917`, `op == K` at
`:2929`, `else throw "bad hotop opcode"` at `:2977`), so `K+1 …` are currently-unused slots
and FLI's append is structurally legal.

**MG-6 — accumulated-class bound.** Track 11 `:125` has escape `K+4 → class+1`, "bounded at 4
classes". A4's "reject `loop_class >= K` before touching the book" (`:517`) is only sufficient
if applied to the **accumulated** class. Per-escape-byte bounding is the classic
wrapper-bounded-but-nesting-unbounded defect — the same shape as mode 7's rule-reference
depth cap (`src/anvil.cpp:1781`) and mode 7/8 nesting
(`docs/decoder-audit.md:225-231`). Mandatory: state that the bound applies post-accumulation,
and fuzz it with a chained-escape probe.

**MG-7 — pin the numbers.** `KITER_MAX` and `LEN_MAX` occur **only as symbols** in track 11
(lines 137, 256, 258, 320, 481, 514, 710) with **no numeric value anywhere in the document**.
A bound that is not a number is not a bound, §10's gates reference them, and the probes at
`:481` are unrunnable as written. Pin both **before P2 runs**. See §10.3: `KITER_MAX` is a
**security** parameter, not a ratio knob.

**MG-8 — correct A3.** Restate A3 as the checks that actually exist (`n <= KITER_MAX`,
`n >= 2`, and `get_uvar`'s own `p < e` / 10-iteration bounds), or implement canonical-form
enforcement and pay for it in the header-size ledger. Do not leave a mitigation in a
specification that no implementation provides.

**MG-9 — overlap path as a prohibition.** FLI must state that the two-path split of
`src/anvil.cpp:2926-2927` is reproduced **per iteration** (bulk `insert` iff `D0 >= L_i`;
byte-at-a-time `push_back(out[out.size()-D_i])` otherwise) and that **`memcpy` is forbidden**
when `D0 < L_i`, because source and destination then overlap and `memcpy` is undefined
behaviour. "periodic if D0 < L0" describes the behaviour but does not prohibit the
optimization a reviewer will reach for first. Carry the audit's proof obligation
(`docs/decoder-audit.md:81-82`): index safety follows only because `D0 <= pos` is checked
*first*.

### 10.3 The amplification claim, quantified — and the cross-lane obligation

FLI asserts "**no new amplification class**" (`:320`, Lemma 4 at `:261`). The *kind* claim is
sound — FLI cannot recurse and its worst case is its own output length. The *degree* claim is
wrong, and the delta is an order of magnitude:

- FLI's own §4.2 states the inner loop performs "no entropy decoder call, no varint read, no
  dispatch on a symbol" (`:239`). So a loop header of **≈ 3 wire bytes** (one uvarint `n`, with
  `loop_class` amortized into the block header) buys up to `KITER_MAX × LEN_MAX` bytes of
  decode work at the **cheapest cycles-per-output-byte in the entire format**.
- The only defensible `LEN_MAX` is `kSparseMaxLen = 65536`, because the book entry `len` is
  already validated to `[1, kSparseMaxLen]` (`src/anvil.cpp:3004`) and FLI's `L0` comes from
  the book entry (`:138`).
- Therefore FLI's work-per-input-byte is `KITER_MAX × 65536 / 3`. At `KITER_MAX = 64` that is
  **1.4×10^6×**, versus the **3.4×10^5×** per-block ceiling I derived in §3.2 for the existing
  RLZ path — i.e. FLI would be **~4× the format's current best amplification primitive**, and
  it would be the cheapest one in cycles. At `KITER_MAX = 256` it is 5.6×10^6×.
- FLI's claim is therefore only preservable if `KITER_MAX` is bounded so that
  `KITER_MAX × 65536 / 3` does not exceed the format's existing worst case, i.e.
  **`KITER_MAX ≤ 15`** — which would largely dissolve the mechanism, since the documented
  rationale is runs "just above the byte-neutrality threshold". So the honest reading is:
  **`KITER_MAX` cannot be chosen for ratio at all.** It is a security parameter whose value is
  set by the format's amplification ceiling, and if the ceiling is raised, that is a
  deliberate, documented, preregistered decision — not a tuning knob.

**Cross-lane obligation to track 11:** FLI's new opcode must ship with a **`ρ` row** in the
resource-meter registry (my §8 rule E2 / §1.3). Concretely, FLI's row must charge
`n × L_i` (bytes produced), `n` (dispatch iterations), and `n` × (periodic-vs-bulk class) so
that the loop instruction is *metered at the same rate* as the tokens it replaces. Without that
row, FLI becomes the cheapest amplification primitive in the format and every resource bound
in `FORMAT.md:929-965` has to be re-derived around it. This is the single dependency track 11
has on track 19, and it is cheap to honour now and expensive to retrofit.

### 10.4 What I am *not* claiming

I have not measured any of this — local fuzzing and benchmarking are forbidden (doctrine 7),
and FLI does not exist yet, so there is nothing to measure. Every claim above is a
specification-level audit against the live `src/anvil.cpp` guards, cited by line. Where I say
"defect", I mean *the specification does not require the guard*, not *the code is broken* —
FLI has no code. MG-1, MG-2, MG-4, MG-5 and MG-7 are the items I would not sign off without;
MG-3, MG-6, MG-8, MG-9 are mandatory clarifications that cost a sentence each.

**Recommendation delta.** This audit does not change my own §9 recommendation (still
**PILOT**, gated on RU-F1, with H1 unconstructed). It does add one dependency I must record:
if FLI proceeds, MG-4 and the §10.3 `ρ` row are **preconditions for FLI's wire landing**, not
follow-up work, because MG-4 breaks the format-evolution invariant and the `ρ` omission
re-opens every bound in `FORMAT.md:929-965`.

---

## 11. Existing-code audit — every zigzag decoder in `src/anvil.cpp`

**Requested by:** coordinator, following the §10.2 MG-2 observation. Scope: every
zigzag *decoder* reachable from the wire in the live tree. **No production edits made.**
Method: exhaustive grep for the zigzag idioms (`zigzag`, `>>1`, `& 1) ?`, `-int64_t`,
`(zz`), then line-by-line read of each hit with its downstream checks. Nothing was executed;
all classification is from the cited code, and I label it as such.

### 11.1 Inventory — 6 decoder-side sites, 2 encoder-side inverses

| # | line | mode / decoder | form | classification |
|---|---|---|---|---|
| Z1 | `src/anvil.cpp:2947` | mode 15 HOTOP **eager** macro, `s[5]`/`ip_mdv` | `(zz & 1) ? -int64_t((zz + 1) >> 1) : int64_t(zz >> 1)` | **A1 + A2** |
| Z2 | `src/anvil.cpp:3069` | mode 15 HOTOP **fused**, `mv[4]`/`ip_mdv` | same | **A1 + A2** |
| Z3 | `src/anvil.cpp:3220` | mode 12 SHAPE **eager**, `s[4]`/`ip_dv` | same | **A1 + A2** |
| Z4 | `src/anvil.cpp:3311` | mode 12 SHAPE **fused**, `s[4]`/`ip_dv` (pull) | same | **A1 + A2** |
| Z5 | `src/anvil.cpp:3499` | mode 12 SHAPE variant, `s[4]`/`ip_dv` | same | **A1 + A2** |
| Z6 | `src/anvil.cpp:2538` | mode 16 ARI-REF type 4, `s[8]`/`ip_ad` | `(z&1) ? -int64_t(z>>1)-1 : int64_t(z>>1)` | **SAFE** |
| E1 | `src/anvil.cpp:2845` | mode 15 encoder, `zz = dlt>=0 ? dlt*2 : (-dlt)*2-1` | inverse | **SAFE**, encoder-only |
| E2 | `src/anvil.cpp:3413` | mode 12 encoder, same inverse | inverse | **SAFE**, encoder-only |

Not a defect, recorded for completeness: `src/anvil.cpp:2376` and `:2400` compute
`int64_t key = int64_t(w7) - int64_t(w0)` in the ARI **encoder**. With `w7`/`w0` as 32-bit
words the range is `[-2^32, 2^32]` and it cannot overflow. If they are ever widened to 64-bit
words this becomes a signed-overflow site, so it is flagged here as a latent hazard, not a bug.
`src/anvil.cpp:3144` is `int64_t(t.dist) - int64_t(lastd)`, also encoder-only and bounded by
`0xFFFFFFFF` on both sides.

**Central finding: the correct form already exists in the same file.** Z6 at
`src/anvil.cpp:2538` uses `-int64_t(z>>1)-1`, which is overflow-free over the *entire* u64
input domain. The same file uses the defective `(zz + 1) >> 1` form five times. This is
therefore a **pure consistency defect with an in-tree reference implementation to copy
verbatim** — no design work, no review of a novel fix, no wire change.

### 11.2 Reachability of the defect inputs

All three varint readers accept the full u64 domain:
- `get_uvar` (`src/anvil.cpp`), `read_varint_bytes` (`:2156`), `read_varint_pull` (`:2143`) —
  each loops `for(i=0;i<10;++i)` and does `x |= uint64_t(b & 0x7f) << shift` with
  `shift = 7*i`. At `i = 9`, `shift = 63`, so a tenth byte of `0x01` sets bit 63.
- Therefore `zz = 2^64 - 1` is reachable as the 10-byte encoding `FF FF FF FF FF FF FF FF
  FF 01`, and `zz = 2^64 - 2` as `FF FF FF FF FF FF FF FF FE 01`. Both are **fully
  wire-legal** under the current readers — no malformed-varint rejection occurs.

### 11.3 Two distinct defects, five sites

**A1 — `(zz + 1)` unsigned wrap at `zz = 2^64 - 1` → silent mis-decode.**
`zz + 1` wraps to `0`, so `>> 1` gives `0` and `dlt = -0 = 0`. Intended value is `INT64_MIN`
(`-2^63`). Downstream at all five sites: `dd = int64_t(lastd) + 0 = lastd`, and `lastd` was
already range-checked to `[1, 0xFFFFFFFF]` by the flag-0/flag-1 paths, so `dd` **passes** both
`dd <= 0` and `dd > 0xFFFFFFFF` checks; `dist = lastd`; and `dist == 0 || dist > pos/out.size()`
also passes. Net effect: a hostile flag-2 (delta) token silently decodes as flag-1 (reuse-last).
The copy is in-bounds and memory-safe. Output is **wrong**, and the only thing that catches it
is the per-block CRC-32 over decoded bytes (`src/anvil.cpp:4882,4888,4915`;
`FORMAT.md:32`). *Classification: **malformed-input canonicality bug**, fail-closed only by
CRC, not by the intended distance check.* Severity LOW as observed — and the reason it is
worth fixing anyway is that the guarantee currently rests on a *different* mechanism than the
one the check was written to provide, which is exactly the coupling that fails the day someone
adds a mode or a decode path without a CRC.

**A2 — `int64_t(lastd) + dlt` signed overflow (UB). Severity: LOW. This is a code-hygiene
item, not a security finding.**
For **even** `zz`, `dlt = int64_t(zz >> 1)`, which reaches `2^63 - 2` at `zz = 2^64 - 2`. Then
`int64_t(lastd) + dlt` exceeds `INT64_MAX` as soon as `lastd >= 3`
(`2^63 - 2 + 3 > 2^63 - 1`). Trigger set: **`zz` even, `zz >= 2^64 - 4`, `lastd >= 3`** —
all wire-legal per §11.2. On the x86-64 target this wraps two's-complement to a large negative
value, `dd <= 0` fires, and the decoder **rejects**.

**Severity adjudication (revised; see §12.1).** This is genuinely undefined behaviour, and
clang enables `-fstrict-overflow` by default in `Release`, so the compiler is entitled to reason
about the following `dd <= 0` comparison under a no-signed-overflow assumption. But the
**observed behaviour is a clean reject**, there is **no memory-safety consequence**, **no
silent output acceptance**, and **no work amplification**. The honest classification is
**LOW — a UB-hygiene defect**, on the same footing as A1: both are cheap correctness fixes to
take opportunistically in the same edit, and neither should influence priorities. An earlier
draft rated A2 "MEDIUM as latent" and argued it up on the F2 precedent; **that inflation is
withdrawn.** A hypothetical optimizer miscompile is not evidence, and the F2 precedent
(`docs/decoder-audit.md:200-212`) is about a *proven* wrap producing a *proven* out-of-bounds
read, which is categorically stronger than this.

Z6 is immune to both: it performs no `z + 1` and adds the decoded delta to nothing.

### 11.4 Smallest fail-closed fix (not applied — production edit forbidden)

Two lines at each of the five sites (Z1–Z5). Z6 is the reference and stays untouched.

```cpp
// before
uint64_t zz = read_varint_bytes(<stream>, <cursor>);
int64_t dlt = (zz & 1) ? -int64_t((zz + 1) >> 1) : int64_t(zz >> 1);
int64_t dd  = int64_t(lastd) + dlt;
if (dd <= 0 || dd > 0xFFFFFFFFll) throw std::runtime_error("bad distance delta");

// after
uint64_t zz = read_varint_bytes(<stream>, <cursor>);
if (zz >= 0x100000000ull) throw std::runtime_error("bad distance delta");
int64_t dlt = (zz & 1) ? -int64_t(zz >> 1) - 1 : int64_t(zz >> 1);
int64_t dd  = int64_t(lastd) + dlt;
if (dd <= 0 || dd > 0xFFFFFFFFll) throw std::runtime_error("bad distance delta");
```

Why this is the smallest *fail-closed* fix, stated as properties rather than as taste:

1. **A1 closed.** `(zz + 1)` no longer exists; the Z6 form is exact over the whole u64 domain.
2. **A2 closed by construction, not by luck.** `zz < 2^32` ⇒ `|dlt| <= 2^31` ⇒
   `lastd + dlt` lies in `[1 - 2^31, 0xFFFFFFFF + 2^31]`, strictly inside `int64_t`. The
   addition **cannot** overflow for any input, so the existing `dd` check becomes reachable-by-
   construction. No `__builtin_add_overflow`, no saturating add, no wider type — one compare.
3. **The cap mirrors the guard already sitting three lines above in the same statement block.**
   The absolute path caps `dv >= 0xFFFFFFFF` before its `+1` (`:2945`, `:3212`, `:3303`); the
   new cap makes the delta path symmetric with it. Same idiom, same file, same magnitude.
4. **Wire-invisible for every legitimate file.** The encoder's inverse at `:2845` / `:3413`
   emits `zz = 2*|dlt|` or `2*|dlt|-1` with `|dlt| <= 0xFFFFFFFF`, so `zz <= 0x1FFFFFFFE`
   for honest output. The only newly rejected values are `zz >= 2^32`, which no encoder in
   this repo can produce. **Zero golden-set churn; all 65 artifacts
   (`scratch/format/golden/`) must still decode byte-identically** — that is the gate.
5. **Falsifiable alternative considered and rejected:** correcting only the `(zz + 1)` form
   leaves A2 open; correcting only the cap leaves A1 open (at `zz = 2^64 - 1` the cap fires
   first, so *cap-only is in fact sufficient for fail-closedness on these five sites*) — but
   cap-only leaves the tree with two different zigzag idioms and reintroduces A1 the moment a
   sixth site is written without the cap. Both lines are one line each; take both.

### 11.5 Forced fuzz vector

Register in `tests/fuzz.py` beside `adversarial_rev2` / `adversarial_bwt`
(`FORMAT.md:918-921`). Note the existing registry rule (`FORMAT.md:900-927`) mandates a forced
case per decoder-visible **id**; these are new *branches* within existing ids, so that rule does
**not** already cover them and they must be added explicitly.

**Construction** (splice discipline from `docs/decoder-audit.md:104-112`, including its
mandatory no-op-rebuild control):
1. Take a real `--parse=hotop` (or `--parse=shape`) artifact containing a block that already
   has a flag-2 delta token, so every other framing/book/consumption check passes unchanged.
2. Locate that token's `zz` in the distance substream — `s[5]` for Z1/Z3/Z5, `mv[4]` for Z2/Z4.
3. Overwrite with **A1 probe** `FF FF FF FF FF FF FF FF FF 01` (= `2^64 - 1`).
4. Overwrite with **A2 probe** `FF FF FF FF FF FF FF FF FE 01` (= `2^64 - 2`, even).
5. Rewrite the substream length prefix, `plen`, and the block CRC — and **run the no-op
   rebuild control first**, since that is exactly the harness bug the audit caught in itself.

**Required assertions — "rejects" alone is not an acceptable result:**
- **P1 pre-fix behaviour, recorded not assumed.** For A1: expect *accept + wrong bytes + CRC
  reject*; for A2: expect *reject*. Either outcome is informative; an accept-without-CRC-reject
  would mean A2 is worse than classified here and must be escalated immediately.
- **P2 post-fix, order-sensitive.** Must reject with the **distance** diagnostic, and must
  reject **before any copy executes** — zero `memcpy` / `push_back`. This is the assertion that
  distinguishes a real fix from a CRC catch. Assert the diagnostic is *not*
  `"substream consumption mismatch"`; a consumption mismatch would mean the probe desynced and
  proved nothing.
- **P3 anti-tautology control (mandatory).** A control probe with the same 10-byte length
  carrying `zz = 0` as an overlong `80 80 80 80 80 80 80 80 80 00` must behave **identically
  to the pre-fix build** — still decodes, still round-trips byte-exactly. Without P3 the probe
  is unfalsifiable, because "longer varint broke it" and "value broke it" are indistinguishable.
- **P4 coverage.** Five distinct forced cases (one per Z1–Z5); the eager and fused variants
  are separate code paths and must both be hit, which is the same lesson as mode 7/8
  (`docs/decoder-audit.md:191-198`, where an independent reviewer found a divergence the first
  pass missed).
- **P5 UBSan is the only detector for A2.** Build with `-fsanitize=undefined` and assert the
  pre-fix binary **reports** signed-overflow on the A2 probe. Without UBSan, A2 is *undetectable*
  by the reject-or-identical oracle, and "no test failed" would be misread as "safe". This is
  the concrete reason the fix must not wait on a fuzz signal. (`.github/workflows/` currently
  has no sanitizer job — `anvil-research-bench.yml:140-146` runs plain `tests/fuzz.py` — so P5
  is also the justification for the sanitizer gap I flagged in §2.)

### 11.6 Canonicality sub-finding (feeds MG-8 and evolution rule E4)

All three readers accept **overlong** encodings: `80 ×9, 00` decodes to `0` in ten bytes.
Classification: **malformed-input canonicality bug**, with no safety impact (the value is
bounded by the caller's range check) but a real **measurement** impact, and therefore mine:

> A hostile or merely sloppy encoder can inflate any varint to ten bytes carrying zero
> information. Since the J/rate objective charges *length*, a non-canonically-encoded artifact
> is silently scored as worse than it is. If such an artifact ever entered a corpus or a
> comparison set, the ratio row would be wrong for a reason that has nothing to do with the
> mechanism under test.

Cheapest closure, and it belongs with evolution rule E4: **require canonical varint form on
write** (one line in each `append_varint_bytes`/`put_uvar` producer), or — if a decoder
leniency is wanted for compatibility — state the explicit clause *"ratio accounting assumes
canonical varint encoding"* in `FORMAT.md`. Either way the assumption becomes written rather
than ambient. Not a security fix; a measurement-validity fix.

### 11.7 Disposition

- **Verdict: two real defects, five sites, both LOW.** A1 is a non-canonical/semantic decode
  defect, fail-closed by the mandatory per-block CRC over decoded bytes. A2 is a signed-overflow
  UB hygiene defect, fail-closed by the target ABI. **Neither is a memory-safety issue and
  neither produces silent output acceptance.** Per §12.1 both are demoted below the
  declared-`total` hazard and below the FLI/WCT blockers in priority.
- **Smallest fix (substance unchanged, priority downgraded):** two lines per site, five sites
  (§11.4) — the in-tree correct form from `:2538`, plus a `zz >= 2^32` reject mirroring the
  absolute-distance cap three lines above. Wire-invisible for all legitimate output; **gated
  on the 65-artifact golden set decoding byte-identically.** Take it as opportunistic hygiene
  in whatever change next touches `:2947/:3069/:3220/:3311/:3499`.
- **Forced malformed-vector unit test (unchanged):** §11.5, with P3 (anti-tautology control)
  mandatory. P5/UBSan is **desirable, not gating** — demoted, since A2 is LOW.
- **Not done by me:** no production edit, no local run. Specification and vector handoff only.

---

## 12. Pair reconciliation and REVISED ruling (supersedes §9)

**Read first:** `19-format-security-fledge.md` (Fledge Alpha Free, independent adversarial
review, final ruling **HOLD**) and coordinator delta **D20** in `COORDINATOR-STATE.md`. This
section is my side of the reconciliation and is now the authoritative recommendation for
Track 19.

### 12.0 Non-novelty declaration (binding on this track)

**This track claims no novelty, and nothing below may be presented as a mechanism
contribution.** Both lanes agree the format's core is already strong: strict trailing-byte
rejection, per-block CRC *before* retention, subtraction-form length bounds, pre-increment
distance guards, capped tables, no cyclic-reference class, a real output-reserve clamp. What
this track delivers is **(a) proof the frozen format is in good shape, (b) one demonstrated
operational hazard, (c) a localized LOW-severity defect list, and (d) harness/corpus-integrity
infrastructure.** Item (d) is adopt-class engineering by construction — a fuzz oracle and a
caller-side limit flag are not research contributions, and the brief's novelty gate
(doctrine 1) forbids dressing them as one. D24's queue filter applies: this lane earns CI
because it can invalidate other lanes cheaply, not because it is interesting.

### 12.1 What I accept from the critic, and what I correct in it

| critic's finding | my response |
|---|---|
| Declared-`total` proxy is a real operational weakness (**REACH, HIGH**); my §1.1/§3.2/§3.3 confirmed independently, including the same 585 GiB / 8.7 TiB arithmetic at critic §2.1 | **ACCEPTED.** It is the finding worth acting on. |
| Zigzag is **LOW**; critic §3.4 "YES for memory safety, NO for correctness"; the CRC catches it | **ACCEPTED, and my rating is now aligned.** A1 is re-labelled *non-canonical/semantic decoder defect*. I withdraw any framing implying silent output acceptance or a safety consequence. |
| My §3.1 ctx-rANS figure is wrong | **ACCEPTED — but the critic's correction is also wrong, and the correct number matters because charging bytes is this track's mandate.** Truth: `kCtxK = 12` (`src/anvil.cpp:1222`), `csymtab` is `std::vector<uint8_t>` (`:1924`), assigned `kCtxK × 4096` (`:1999`) = **49,152 B = 48 KiB**. My "~1 MiB" assumed `K = 256`; the critic's "~192 KiB" is 4x too high for the same expression. **48 KiB**, and it is a *compile-time constant* because `cctx.K > kCtxK` is rejected (`:1985`) — i.e. not attacker-controllable. §3.1 is corrected. |
| VCA novelty residue (b) discounted; (a) **objected to** — safety must not key on host-calibrated ns/byte | **ACCEPTED on both counts.** (a) is withdrawn: my own §4.4 already flagged it as a reproducibility hazard, and the critic is right that it should have been a withdrawal, not a "disconfirming argument". Only (c) — boundedness-as-a-sum-over-a-registry — survives, and it survives as an *engineering* property. |
| §1.4 `--on-corrupt` containment demoted to a harness concern | **ACCEPTED.** "Never emit bytes for an unverified region" is the right hard rule and I keep it; the taxonomy/policy is harness work, not a format mechanism. |
| §9 PILOT not inherited; measure before building | **ACCEPTED.** See §12.3. |
| §10 FLI: **disagrees that FLI is merely under-specified** — `LOOP_ARITH` is unreachable, which invalidates the claim rather than the spec | **ACCEPTED, and it changes my §10 conclusion.** I found under-specification; the critic found the *mechanism arm itself* vacuous. The critic's finding is strictly stronger and I adopt it. My MG-1/MG-2/MG-5 remain valid as spec guards, but they are now **downstream of a claim-validity question**, not the primary finding. |
| Critic T5 asserts WCT is **blocked** (W1 worse-on-bytes, W2 hard OOB-write) | **ACCEPTED.** §12.5 records my own track-19 security verdict on WCT. |

### 12.2 Revised severity ladder (single ordering, no inflation)

1. **Declared-`total` / work-amplification slack** (`src/anvil.cpp:4855`) — REACH, HIGH
   *operational*. Confirmed independently by both lanes. **The only item that earns CI.**
2. **FLI `LOOP_ARITH` unreachability** — claim-invalidating, not merely a security gap.
3. **WCT W2 tail-window mask OOB-write** — the only memory-safety item in either report, and it
   is a *hard* blocker on a mechanism that has not been built.
4. **WCT W1 / FLI F3/F4** — bytes-and-compatibility blockers, pre-build.
5. **Substream full-consumption extension (my MG-4)** — accounting correctness, pre-build, cheap.
6. **Zigzag A1 + A2** — **LOW**, opportunistic hygiene, no gating.

### 12.3 REVISED RULING

# **HOLD** — the format-security *mechanism* program (VCA) is not built.

Concretely: **VCA is not built, no decoder redesign is proposed, and no revision bump or wire
change is sought.** My §9 `PILOT` is withdrawn. What survives is the diagnosis (§1.1, §1.6,
§2, §3), the cross-lane blockers (§10, §12.5), and the two deliverables below, which are
infrastructure and explicitly non-novel per §12.0.

The critic's argument is the one I accept as decisive: VCA spends a decoder redesign against a
**corpus-integrity problem with zero recorded incidents**, and one deterministic byte-only
remote job either produces the evidence or kills the hypothesis. Building first inverts the
order of proof. My own §4.1 (H1 unconstructed) said the same thing; I mis-weighted it as a
scheduling detail rather than as the ruling.

### 12.4 The single decisive next action — byte-only synthetic probe (E19-SB)

The cheapest thing that can change the verdict, and it is **byte-only and synthetic**: no
corpus, no timing comparison, no benchmark window, no local execution. It is deliberately
narrower than the critic's E19-1 (which bundled three sizes plus rlimits plus a second probe)
so that it can be run and adjudicated as one artifact.

**E19-SB — achievable-expansion/work probe under current legal framing.**

- **What.** Deterministically generate hand-built **rev-1** files that declare `total` at the
  format-legal ceiling `(in.size()/7 + 2) * max_block` and whose block payloads decode cheaply,
  at three input sizes: **64 KiB, 1 MiB, 8 MiB**. Byte-only: no timing benchmark is being
  measured, only bytes-in vs bytes-produced and the process outcome.
- **Why it is decisive.** My §1.2/H1 required a *constructed* >= 1 GiB producer. I never
  constructed one — I derived it (argued from `kSparseMaxLen` and the RLZ op decoder) without
  tracing the substream framing end to end, and I said so. E19-SB is precisely the missing
  construction, and it also produces the number no existing document in the repo states: the
  **achievable** expansion ratio under current legal framing, as opposed to the
  *format-legal* ceiling of ~9.6e6x. Those are different numbers and the gap between them is
  the whole argument.
- **Per-file recorded outputs (mandatory set, all in one job):** input bytes; declared `total`;
  block count; **bytes actually produced before abort**; peak RSS under a 2 GiB address-space
  rlimit; wall clock under a 60 s cap; exit class (`clean-reject` / `oom` / `timeout`); and the
  **achieved ratio** `bytes_produced / input_bytes`.
- **Frozen thresholds, registered before dispatch (critic K1–K4, adopted verbatim):**

| # | Threshold (frozen, not movable after seeing data) |
|---|---|
| **K1** | any file with `in.size() <= 64 KiB` produces **>= 1 GiB** before abort ⇒ slack **CONFIRMED** ⇒ the caller-side ceiling in §12.5 is funded and E19-SB closes as CONFIRMED |
| **K2** | peak RSS under the 2 GiB rlimit exceeds 2 GiB, or the process is **OOM-killed** ⇒ memory exposure confirmed **independently of `total`** ⇒ escalate |
| **K3** | the 8 MiB file does **not** clear 8 GiB ⇒ my §3.2 amplification projection is **wrong**; **retract it**, withdraw H1, and close the track at HOLD with the one-line flag |
| **K4** | probe **inconclusive** (runner too small to observe) ⇒ `BLOCKED_INFRA`, **not a pass**. Repeat at a larger runner or a smaller declared `total`. **Never record "safe" from an inconclusive run.** |

- **The decisive asymmetry, stated so nobody can misread the outcome.** K3 firing is a genuine
  result and it **kills my own projection** — I have pre-committed to retracting §3.2 rather
  than reinterpreting it. K1 firing without K2 does **not** promote VCA; per critic P1 it buys
  a plain `--max-output-bytes` flag and nothing more. Only K1 **and** K2 together fund a real
  mechanism. A partial confirmation is not a mechanism.
- **Cost.** One manual-dispatch `ubuntu-24.04` job, tier C of
  `docs/GITHUB-ACTIONS-BENCHMARKING.md`, no corpus download, no GPU, under 10 minutes.

### 12.5 Caller-side ceiling contract — design sketch (non-novel, adopt-class)

Deliberately **not** a format change. Every parameter is caller-supplied or an explicit
documented default; the default reproduces today's behaviour, so landing it is
behaviour-neutral for existing callers and needs no revision bump, no wire change, and no
golden-set churn.

```
--max-output-bytes N   caller output ceiling. Default: min(total, 4 * in.size() + 1 MiB)
--max-alloc-bytes  M   caller allocation ceiling for the output buffer plus all decoder-side
                       temporaries. Default: 2 * total + 64 MiB
--max-blocks       B   default ceil(in.size()/7) + 2   (already implied; make it explicit)
--max-work-bytes   W   caller work ceiling = bytes produced + bytes copied.
                       Default: unset (= --max-output-bytes), so the flag is opt-in for embedders
```

- **Checked at three points, each before the corresponding allocation:** (1) after the header,
  against declared `total`; (2) at each block header, before `resize`/`reserve`; (3) at each
  substream header, against the existing `16*out_len + 64` rule. This is the same three-point
  admission skeleton from §1.3 — I keep the *skeleton* and drop the mechanism, the meter, the
  ns/byte table, and the registry.
- **Structural units only** (`bytes produced`, `bytes copied`, `bytes allocated`, `block
  count`). **No ns/byte.** The critic's objection to VCA(a) is correct and this is the direct
  consequence: a safety invariant must not depend on a host-calibrated measurement constant,
  or "decode the same artifact under a different ceiling" silently becomes a different
  experiment. Any ns/byte figure stays a separately-reported *projection*, never a gate.
- **The default is set from measurement, not theory.** Critic §8.3 is right that `4x` is *not*
  obviously above honest worst case — a mode-11 raw-token path is ~1.1e4x by my own §3.2
  arithmetic. So the default is **provisional**, must be set from E19-SB's *honest-file* arm
  (encode the corpus, measure the maximum achieved ratio), and must be labelled provisional in
  the help text until then. Shipping a default that rejects honest files is a worse outcome
  than shipping no default.
- **Why this is the right scope.** Roughly 30 lines, reviewable, adopt-class, and it converts
  an unbounded operational hazard into a caller decision — which is the actual requirement.
  Everything else in §1.3 waits for K1+K2.

**Cross-lane security verdicts I still owe the coordinator (D13).**

- **WCT (Track 10).** Concur with the critic on priority: **W2 (tail-window submask must be
  masked to `T = L mod 4`) is the only out-of-bounds-write item in either report and is a HARD
  blocker**; **W1** (a non-empty subset of four residual bytes has 15 states, so a 3-bit submask
  is not a valid code absent a stated missing invariant) means the design is worse on bytes
  than what it replaces and therefore cannot pass a no-regression-bytes gate. **W3**
  (XFORM/RESID mutual exclusivity) I re-label: it is an **encoder** property, so it must be
  asserted by a forced-encode test, not enforced as a decoder invariant — the decoder cannot
  know which the encoder meant. **W4 (stream count and full consumption)** is my §10 MG-4
  generalized and stands unchanged: every new substream must join the block's full-consumption
  check (`src/anvil.cpp:2979-2981`) and the payload trailing-byte check (`:2911`), or the
  format silently accepts bytes it never accounts for. WCT is **KILL as specified**; a corrected
  W1/W2 design may be re-proposed as engineering only.
- **FLI (Track 11).** My §10 stands as a spec-level audit; the critic's §4.2 finding
  (`LOOP_ARITH` unreachable) is stronger and I adopt it, so FLI-as-claimed is **KILL** and
  FLI-as-`LOOP_EQ` is **HOLD** as adopt-class counted repetition. If a corrected FLI is ever
  re-proposed, the §10.3 amplification obligation stands: its loop opcode must ship with a
  resource-cost row or it becomes the cheapest work-amplification primitive in the format.

### 12.6 What I am explicitly not doing

Not building VCA or any decoder redesign. Not changing `src/anvil.cpp`, `FORMAT.md`, or any
existing file. Not running the probe locally, and not fabricating its numbers. Not claiming
novelty, a ratio win, or a frontier contribution. Not moving any threshold after data arrives
— K3 in particular retracts my own §3.2 projection on contact.