# Q2 Block/Window Parity — Independent Red-Team Critique

**Date:** 2026-10-02
**Author:** independent red-team (pre-dispatch), dispatched by request
**Target artifact:** `docs/swarm-2026-10-02/REMOTE-EXPERIMENT-QUEUE.md` §Q2 (lines 51–57, 154)
**Mandate:** reconstruct the geometry/semantics claims from source and retained artifacts; specify the
minimum arm set that identifies the parity question without becoming a block-size sweep; attack
selection bias, timing endpoint selection, same-job pairing, binary/RSS comparability, and the
decoder-parallelism regression risk.

**Constraints honored:** no local heavy benchmark, no production edits, no commit, no push. Every
number below is read from committed source, a retained artifact, or a swarm document — none is
produced by a new measurement. Where I could not verify a claim from source, I say **UNVERIFIED**
rather than estimating.

---

## VERDICT

> ## **REVISE**
>
> Q2 is **not** a block-size sweep problem and **not** primarily an epsilon problem. It has **three
> blocking defects** that each independently invalidate dispatch, plus **one structural finding that
> changes what the experiment can conclude even if every defect is fixed**.
>
> - **B1 — the experiment as named confounds geometry with backend.** "Class-A vs Class-B" compares
>   the LZ-token lane against the BWT lane on a different wire revision. The measured gap between
>   them is ~4.5×, and **none** of it is geometry. Dispatching the named comparison attributes a
>   backend win to a window fix.
> - **B2 — the frozen frontier predicate cannot satisfy Q0's own precondition 7.** A
>   `FRONT-CROSSING` token is *defined* as a joint byte-and-speed bracket, so a single noisy timing
>   cell can emit it. Q0 forbids exactly that. The two Q0 requirements are mutually unsatisfiable, and
>   that contradiction is a coordinator ruling, not an implementer's choice.
> - **B3 — Q2's byte-primary scope cannot adjudicate the cells Q2 exists to test.** The only five
>   non-dominated cells in the retained suite are held out of `DOMINATED` by a **7.8 % encode-speed
>   margin** that sits *inside* the project's own measured dispersion band (CV 5.5–35.5 %). A
>   bytes-primary experiment cannot move that axis, so the retained frontier's status is
>   structurally out of reach.
>
> **Structural finding (does not block, but must be disclosed):** the byte-optimal parity geometry
> **collapses ANVIL's only structural decode advantage.** At parity the container is one block, so
> `nthreads = min(decode_threads, segs.size())` is forced to 1. "Bytes-only parity" therefore
> *creates* a decode regression of order 2–4×, far larger than the 5–17 % restart tax that motivated
> the job. Any bytes-only fix that ships is a **new cost regression**, and Q2 as specified cannot
> see it because it never arms the parallelism axis.
>
> **Dispatch is blocked. Cancel is wrong** — the underlying question is real, unmeasured, and the
> 0-crossing result is genuinely geometry-conditional. Revise to §5–§7 and it becomes dispatchable.

---

## 1. PROVENANCE — what this critique is grounded in

| Source | Role in this critique |
|---|---|
| `src/anvil.cpp:3769,3793,3796,4667-4699,4849-4925,4958-5002` | block default, negate default, aux default, thread semantics, revision split |
| `tools/bench_native.cpp:14-35` | the harness that produced the retained Class-A CSV |
| `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` | frozen Class-A grid (364 rows) |
| `prototypes/swarm-2026-10-02/04-dense-frontier/space-bunny/parity_sweep.py` | the frozen predicate as materialized in code |
| `tests/noise-floor.csv` | measured dispersion (243 rows) |
| `tests/thread-attestation.csv` | thread provenance; confirms retained rows predate the threading plumbing |
| `docs/audit-2026-09-07/13-e4-block-routing-oracle.md` | the "5–17 %" restart-tax artifact |
| `tests/block-oracle.csv` | E4 raw data — SHA-256 `3E85D690953BB2934A1B27ADF283ACFA1CA44F3F6B39613712D46A48DF1AEAD8` |
| `docs/audit-2026-09-07/13-e4-block-routing-oracle.md` | SHA-256 `47D95AB2363FB7ADC5BCA03ADA69AF495E235265BDBCD8B05AA819B2E598E1DC` |
| `docs/I10-BREAKTHROUGH-PROGRAM.md:627-652` | I10-1A.2 subblock/RSS sweep |
| `tests/ratio-first-standard.csv`, `tests/ratio-first-benchmark.csv` | Class-B measurements (different producer) |
| `docs/swarm-2026-10-02/Q0-DENSE-RETAINED-RESULT.md` | Q0 ruling + the 9 preconditions Q2 must satisfy |
| `docs/swarm-2026-10-02/SYNTH-MEASUREMENT-FLEDGE.md:475,540-598` | ε critique, INV-22, the "must not inherit ε=0" precondition |

This **closes SYNTH-MEASUREMENT-FLEDGE precondition (a)**: the "5–17 % restart tax with no artifact
identity" is now pinned. It is `tests/block-oracle.csv`, SHA-256 above, summarized in the E4 memo,
and the 17 % / 5 % endpoints are +5,431,727 B (+17.0 %) at 256 KiB and +643,453 B (+2.0 %) at
16 MiB against a 31,950,395 B whole-file base. See §3.4 for why that number is still not a valid
denominator for Q2.

---

## 2. GEOMETRY RECONSTRUCTION

### 2.1 Class-A — the retained grid

`tools/bench_native.cpp:25` constructs `anvil::Options o` and **never assigns `o.block_size`**.
It also never assigns `o.decode_threads`. Therefore every ANVIL row in the frozen CSV runs at:

- `block_size = 256*1024` = **262 144 B** (`src/anvil.cpp:3769`, default, non-ratio)
- `decode_threads = 1` (`src/anvil.cpp:3796`, default, serial)
- `revision = 1` (`src/anvil.cpp:4667`, `opt.parse=="ratio" ? 2 : 1`)
- `negate = true` (`src/anvil.cpp:3778`, default — the difference-cover incompressibility gate)

All 18 retained ANVIL configs are non-ratio (`bench_native.cpp:61-78`: greedy, dp, sparse, sparse-l0,
sparse-channels, tcopy, tcopy-pnra, mdl, mdl-l0, mdl-l001, shape, shape-l0, shape-ctxmap, hotop,
hotop-budget, hotop-rlzp). **None is BWT. None is ratio mode. None carries an aux index.**

`parity_sweep.py:113-115` asserts exactly this and it is **correct**.

### 2.2 Class-B — what the queue calls "128 MiB ANVIL blocks"

`src/anvil.cpp:4999`:

```cpp
if(opt.parse=="ratio" && !block_explicit) opt.block_size=128u<<20;
```

Class-B is **not "ANVIL with a bigger block."** It is `parse=="ratio"`, which simultaneously changes:

| dimension | Class-A | Class-B |
|---|---|---|
| block size | 256 KiB | 128 MiB |
| block encoder | `consider_parse` LZ-token ladder (`:4696-4699`) | `encode_ratio_block` (`:4676`) |
| wire revision | **1** | **2** |
| block mode tag | varies by ladder rungs | **17 (BWT)** |
| entropy | token-type / lit-len / lit-dist streams | BWT + postcoder (+ optional aux index) |
| aux sampling | absent | `bwt_aux_rate_for_size` (`:4247`) |
| format cap | `1..64 MiB` (`:5002`) | `1..128 MiB` (`:5000`) |

**Four variables move together.** The queue's sentence "Class-B ratio mode uses 128 MiB ANVIL
blocks" is true but incomplete, and reading it as a *geometry* axis is the trap.

Measured size of the gap, from `tests/ratio-first-benchmark.csv` vs the retained CSV, on the same
file `generated.jsonl` (2 815 267 B):

- `anvil-ratio` (Class-B, 128 MiB): **54 656 B** (ratio 0.0194)
- `anvil-dp-rans` (Class-A, 256 KiB): **0.087 ratio** ≈ 245 000 B

That is a **~4.5×** difference. If Q2 dispatches the Class-A/Class-B comparison as written, it will
report a spectacular geometry win that is in fact a BWT-backend win. **This is defect B1 and it is
the single most dangerous misattribution available in this experiment.**

### 2.3 Reference windows — the asymmetry is real, one-directional, and corpus-limited

From `tools/bench_native.cpp`:

- `bench_brotli` (`:39`) uses `BROTLI_DEFAULT_WINDOW` = lgwin 22 = **4 MiB**, one-shot, no block split.
- `bench_zstd` (`:49`) uses `ZSTD_compress(..., lvl)` one-shot — window derived from level and
  `srcSize`; effectively whole-file at these sizes.

Every corpus file is **< 2.82 MiB** (largest: `generated.jsonl`, 2 815 267 B). Therefore:

> **At 256 KiB the references already see the whole file. Only ANVIL is segmented.** The
> "asymmetry" is not a two-sided window mismatch — it is a one-sided ANVIL segmentation penalty.

Two consequences the queue does not state:

1. **The corpus cannot exhibit reference-side segmentation.** With no file above 4 MiB, Brotli's
   window never binds, so Q2 can never demonstrate that the two sides are "matched" in any general
   sense. It can only measure ANVIL's penalty. Any general parity claim is **unsupported by
   construction** — the corpus is not sensitive to the hypothesis it is being used to test.
2. **"Parity" is an emergent property, not a declared setting.** There is no configured parity
   geometry to match to; there is a per-codec default that happens to exceed file size. Q2's arm
   identity must therefore record *achieved block count per file*, not a nominal window. This is
   exactly the Q1 census that SYNTH-MEASUREMENT-FLEDGE already identified as missing project-wide
   (line 47: "publish block count / mean block size per canonical file … it is currently missing").

### 2.4 Block-count arithmetic on the frozen corpus — the geometry axis is mostly vacuous

`ceil(bytes / 262144)`:

| file | bytes | 256 KiB blocks | note |
|---|---:|---:|---|
| `generated.jsonl` | 2 815 267 | 11 | |
| `generated.log` | 1 942 280 | 8 | |
| `anvil_bench.exe` | 1 929 216 | 8 | |
| `generated.sqlite` | 1 740 800 | 7 | |
| `synth-jitter.bin` | 974 920 | 4 | mixed compressibility — see §3.6 |
| `generated.repeat.jsonl` | 940 000 | 4 | |
| `generated.json` | 827 664 | 4 | |
| `synth-timeseries.bin` | 280 000 | 2 | |
| `anvil.exe` | 268 800 | 2 | |
| `random.bin` | 262 144 | **1** | **geometry-invariant by construction** |
| `synth-arith.bin` | 256 000 | **1** | **geometry-invariant by construction** |
| `src.cpp` | 32 512 | **1** | **geometry-invariant by construction** |
| `doc.md` | 2 724 | **1** | **geometry-invariant by construction** |

- **4 of 13 files (31 %) are already single-block at 256 KiB.** For these, `--block=262144` and
  `--block=67108864` produce *identical container geometry*. They cannot exhibit a parity effect.
- **Median file is 280 000 B → 2 blocks.** Only 3 files exceed 8 blocks.
- **`random.bin` is exactly 262 144 B.** One byte above the block size it becomes 2 blocks. This is
  a knife-edge file and it is also the corpus's designated incompressible `DEGENERATE` row
  (`parity_sweep.py:24`, `DEGENERATE_RATIO = 0.95`). Any geometry arm that changes block size on the
  boundary can flip its classification for a reason that is arithmetic, not informational.

Q0's decisive tally pools all 13 files into 468 cells, so **≥31 % of the cells are structurally
incapable of moving** and are being used as if they were evidence.

---

## 3. SEMANTIC RECONSTRUCTION — the seven items requested

### 3.1 ANVIL block semantics: independent blocks, no cross-block carryover

`src/anvil.cpp:4671-4682` — the encoder walks the input in `block_size` strides and compresses each
block in isolation. There is no carried match window, no carried MTF/context, no cross-block model.
The container header stores `block_size` once (`:4669`) and each block is independently framed with
`uvar(blen)` + mode byte + `uvar(payload_len)` + CRC32 (`:4678-4680`).

So **effective ANVIL history = exactly `block_size`.** At 256 KiB, ANVIL's match window is 256 KiB,
against a whole-file reference. That part of Q0's premise is verified.

Also verified: per-block framing cost is negligible (E4 memo line 19: "Framing is negligible (≥4 MiB,
<200 B total)"), so the geometry effect is *not* a framing artifact. Good.

### 3.2 Thread semantics — the parallelism collapse

`src/anvil.cpp:3796`:
```cpp
uint32_t decode_threads=1; // parallel block-decode worker count; 1 = serial (zero behavioral change)
```

`src/anvil.cpp:4862` (comment) and `:4908`:
```cpp
const size_t nthreads=std::min<size_t>(opt.decode_threads,segs.size());
```
with a serial fast path at `:4868` when `decode_threads<=1`. CLI range 1..1024 (`:4993`), and
validation at `:4993` rejects 0.

The decode loop is explicitly designed around **embarrassing per-block parallelism**, enabled by the
per-block CRC (`:4862`: "CRC-verified, so block decode is embarrassingly parallel").

Therefore:

| geometry | blocks (`generated.jsonl`) | max `nthreads` | decode parallelism |
|---|---:|---:|---|
| 256 KiB (Class-A) | 11 | 11 | available |
| 128 MiB / 64 MiB (parity) | 1 | **1** | **impossible** |

**This is the crux.** Block parity and decoder parallelism are the *same variable* in this format.
You cannot raise the block size to buy ratio without simultaneously and irreversibly deleting the
decode-parallelism option. There is no third point: the container has no inter-block dependency
structure that would permit splitting a large block for decode while keeping a large encode window.

The references are also measured single-threaded (`thread-attestation.csv:3-4`,
`BrotliDecoderDecompress`/`ZSTD_decompress` at `bench_native.cpp:44,52`; `ref_threads=1`). So the
retained comparison is thread-matched at 1-vs-1 — but it is matched *at the cost of both sides
forfeiting parallelism*, and the retained CSV measures neither side's parallel ceiling.

### 3.3 Aux sampling interaction — a Q2/Q9 confound, and it is out of scope for Class-A

`src/anvil.cpp:3913` and `:4247-4254`:

```cpp
static constexpr uint32_t kBwtAuxTargetWalks = 1024;
static int32_t bwt_aux_rate_for_size(int32_t n) {
    uint64_t target=(static_cast<uint64_t>(n)+kBwtAuxTargetWalks-1)/kBwtAuxTargetWalks;
    ...
}
```

The sampling rate is a **pure function of the block size**: one index per 1024 B of the block. The
index count is `icount = 1+(n-1)/rate` (`:4268`), which is **≈1024 regardless of `n`**.

Consequences:

1. **Total aux overhead scales with block *count*, not block size.** At 1024 indexes × 4 B = 4 KiB
   of index per block (`put_u32le` per index, `:4318`) plus a ~7 B per-block header (`:4314-4316`).
   An 11-block file carries ~44 KiB of aux index; a whole-file block carries ~4 KiB. **Parity
   therefore removes ~90 % of the aux index bytes** — a byte win that is attributed to geometry but
   is mechanically a per-block fixed cost.
2. **`kBwtAuxTargetWalks = 1024` *is* Q9's "W1024".** Q9 (aux-index W1024 vs W16) changes the same
   constant that geometry multiplies against. **The two levers push aux bytes in the same direction
   and are not separable** if both run. If Q2 changes block size while Q9 changes the walk target,
   the byte delta is `(nblocks) × (walks/block)` — two factors, one number.
3. **Crucially, none of this touches Class-A.** `bwt_aux` is gated on `opt.parse=="ratio"`
   (`:4266`, inside `encode_ratio_block`), and no retained Class-A row is a BWT/ratio row.

**Design consequence:** Q2's parity arms must be **Class-A token-lane configs with `--block` swept**,
and must **exclude every ratio/BWT arm**. Parity is fully reachable in rev 1 — `--block` is legal up
to 64 MiB (`:5002`), which is whole-file for a corpus whose max file is 2.69 MiB. Bringing ratio/BWT
into Q2 would (a) convert it into a backend comparison, and (b) import the aux confound for no
reason.

### 3.4 The warmup-vs-window confound — the "5–17 %" is a different mechanism

This is the most important reconstruction, and it invalidates the queue's stated denominator.

E4's own mechanism statement (`13-e4-block-routing-oracle.md:42-44`):

> *"The entire regret is **model-warmup tax**: independent blocks restart MTF/context state. webster
> at 4 MiB: Σ BWT blocks = 8,211,644 B vs whole-file BWT 7,317,329 B = **+12.2 %**; Brotli shows the
> same (+9.3 %)."*

MTF/context is **BWT-specific**. It is the sort-transform context model, reachable only via
`--ratio-backend=bwt` under `parse=="ratio"` (§2.2). The Class-A rows that Q2 will actually sweep
(`bench_native.cpp:61-78`) are LZ-token-lane configs whose per-block restart cost has a **different
composition**: match-window truncation at 256 KiB, entropy-table reset, recency-cache reset.

**The 5–17 % figure was measured for a mechanism that is not present in any Class-A arm.** The
project has never measured the Class-A restart penalty. Using 5–17 % as Q2's predicted effect is a
category error: it is a plausible prior from an adjacent mechanism, not a measurement of the arms
under test.

Three further transferability failures, all visible in the E4 artifact:

1. **Different corpus.** E4 is Silesia: `mozilla` 51 220 480 B = **196 blocks** at 256 KiB (E4 memo
   line 51 confirms "mozilla 0/196 @256 KiB"). Q2's largest file is 11 blocks. Warmup tax ≈
   `nblocks × C / filesize ≈ C / block_size`, so relative tax may well be block-size-driven rather
   than block-count-driven — but E4's own data cannot distinguish these, and the aggregate (+17.0 %
   at 256 KiB over a 5-file mix of 10–51 MB files) is **not** a per-file, per-block-count
   observation that can be rescaled. **UNVERIFIED transferability; direction of error unknown.**
2. **Different parse mode and backend.** `--parse=ratio --ratio-backend=bwt|auto` vs the Class-A
   token ladder. E4's oracle is `min(framed Brotli, framed BWT)` per block — a *routing* construct
   that does not exist in any Class-A arm.
3. **The two warmups are different axes and can move in opposite directions.** E4's model warmup
   pushes *against* small blocks on bytes. §3.6's negative-gate sampling grid pushes *for* small
   blocks on bytes. §3.7's allocator/page-fault warmup pushes *against* small blocks on *time*.
   Net sign is **not predictable a priori** — which is precisely the FLEDGE-REVIEW's point that Q2
   needs a `stage-4 ns/block` column (line 48) and a declared ≥ 7 % predicted effect before any
   timing arm. **Q2 as specified declares no predicted effect at all.**

### 3.5 The negative-gate sampling grid is a mathematical function of block size — a hidden third arm

This is a source-verified interaction the queue never mentions, and it directly threatens byte-axis
attribution.

`src/anvil.cpp:3778` — `bool negate=true;` — **on by default**, and `bench_native.cpp:25` does not
override it. The gate is `probe_incompressible` (`:3808-3826`):

```cpp
if (n < 512) return false;
const size_t S = 1024;
size_t stride = std::max<size_t>(1, n / S);        // <-- stride depends on block size n
... sample S=1024 points ...
return dup * 100 <= S;                             // <=1% repeats -> declare incompressible
```

`n` here is the **block** size (`:4685` calls it per block). Therefore the gate's sampling grid is
locked to block size:

| geometry | block n | stride | samples per 2.69 MiB file | verdict granularity |
|---|---:|---:|---:|---|
| 256 KiB | 262 144 | **256 B** | 11 × 1024 = 11 264 | per block, 256 B grid |
| parity | 2 815 267 | **2750 B** | 1 × 1024 = 1 024 | whole file, one verdict |

Two distinct effects:

1. **Detection power changes with geometry.** 11.3 KB-strided × 11-block gating resolves
   incompressibility at ~256 B granularity; whole-file gating resolves it at ~2.75 KB granularity. A
   **mixed-compressibility file** — `synth-jitter.bin` (974 920 B, 4 blocks at 256 KiB) is named for
   exactly this — can be classified differently at the two geometries purely from sampling density.
2. **All-or-nothing vs partial.** At 256 KiB each block gets an independent gate verdict, so a mixed
   file yields a *mixture* of raw and compressed blocks. At parity there is exactly one verdict for
   the entire file — either all raw or none.

So **changing block size changes which bytes ever reach the parser.** Bytes, decode work, and RSS all
move for a reason that is neither the window nor the model. This is a **second, mandatory factor** in
the design (§5), and it is not in the queue's six-arm notion.

Note also `:4679` — `if(payload.size()+1<block.size())` → compressed, else raw. At parity a single
2.8 MB block has a *lower* compression-ratio barrier to clear than an 11-block file does in aggregate,
so raw-block fallbacks are not block-count-invariant either.

### 3.6 RSS and the threading sign

Two separate RSS problems, both fatal to any RSS claim from Q2.

**(a) The frozen CSV has no RSS column and no RSS instrument.** `bench_native.cpp` emits exactly
`input_bytes / codec,compressed_bytes,ratio,encode_MBps,decode_MBps,roundtrip` (`:81-84`). There is
no peak-RSS measurement anywhere in the harness that produced the retained Class-A grid. RSS in this
repo comes from a **different producer** (`tests/ratio-first-*.csv`, which does carry
`compress_peak_mib` / `decompress_peak_mib`) with a different method. Q0 precondition 9 already
forbids cross-family RSS comparison on scope mismatch; here the mismatch is **instrument**, not scope.

**(b) The existing RSS substrate is internally inconsistent and unusable as a control.** From
`tests/ratio-first-standard.csv`:

| file | bytes | anvil-ratio dec peak | brotli dec peak | zstd dec peak | xz dec peak |
|---|---:|---:|---:|---:|---:|
| `dickens` | 10 192 446 | 24.070 | 25.824 | **1.613** | 22.758 |
| `mozilla` | 51 220 480 | 110.086 | 123.262 | **1.613** | 111.324 |
| `mr` | 9 970 564 | 23.195 | 23.195 | **1.613** | 15.012 |

`zstd` reports a **constant 1.613 MiB peak decode RSS across 9.97 MB and 51.22 MB inputs**. A working
set that does not move when the input grows 5× is not a working set — it is a constant-size streaming
buffer. The zstd RSS axis is measuring something other than peak resident charge. `brotli`'s 1.613
value in the *reference* column of `ratio-first-benchmark.csv` shows the same constant bleeding
across producers.

**Therefore: any Q2 RSS column is a fresh measurement requiring its own instrument and attestation,
and it may not be placed in the same table as, or compared against, any retained RSS number.** The
project currently has no citation-grade RSS substrate at all.

**(c) The threading sign.** Because `nthreads = min(decode_threads, nblocks)` (§3.2), RSS is a
function of *both* block size and thread count:

- 256 KiB × N threads → ≈ N × 256 KiB working set
- parity × N threads → N threads is *unreachable*; `nthreads` is forced to 1

So the "RSS relief" reading is not merely weakly supported (per FLEDGE-REVIEW C2, the RSS-primary
rewrite was correctly **retracted** as a misreading). It is **structurally unavailable**: the arm that
would relieve RSS is the arm that eliminates the threading axis entirely. There is no geometry at
which ANVIL simultaneously (i) matches a whole-file reference window, (ii) keeps multi-block decode
parallelism, and (iii) reduces peak RSS. **The three are mutually exclusive in this container format.**

### 3.7 The frozen frontier predicate — reconstructed, and it is defective

`parity_sweep.py:44-77`. Two independent problems.

**(a) The predicate mixes a deterministic axis with a noisy one, in the same test.**

```python
def dom(a, b, plane):
    return a["ratio"] <= b["ratio"] and a[plane] >= b[plane] and (
        a["ratio"] < b["ratio"] or a[plane] > b[plane])
```

`DOMINATED` and `FRONT-CROSSING` both require **both** axes. `bracket_gap` likewise requires the
candidate to be bracketed on ratio **and** on the timing plane. Consequences:

- A candidate with **byte-identical** ratio to a reference but slower is `DOMINATED` — decided purely
  by a noisy clock.
- A candidate that is byte-bracketed and speed-bracketed emits `FRONT-CROSSING` — and the speed
  bracket is the only thing that can be inside the noise band.

**This makes Q0 precondition 7 — "never let a single timing cell emit a FRONT-CROSSING token" —
unsatisfiable by the frozen predicate.** Q0 demands the token be byte-driven; the code makes it
byte-and-speed-driven. Both requirements are stated as mandatory. **This is defect B2** and it needs
an explicit coordinator ruling on which one is relaxed.

**(b) `eps` is applied in incommensurable units, and at the decisive setting it is vacuous.**

```python
ap.add_argument("--eps", type=float, nargs="*", default=[0.0, 0.005, 0.01, 0.02, 0.05])
...
lad, anvil_names, _ = anvil_ladder(by_file, 0.0)   # <-- line 123: decisive tally at eps = 0.0
```

At `eps = 0.0` all four guards in `bracket_gap` (`:59-63`) become `x >= 0`, i.e. **always true**. The
docstring's stated purpose (`:51-52`, "a material-epsilon requirement so a 0.1 % gap cannot
manufacture a FRONT-GAP label") is **defeated at the decisive setting**. The retained 435/28/5/0 tuple
is therefore an ε-free tuple, and INV-22's warning that "ε = 0 is not neutral — preserving 0 crossings
by retaining an ε-free predicate is itself a threshold choice" is confirmed in code.

Meanwhile the same `eps` is compared against `ratio` (dimensionless, 0.028–0.30 range) **and**
`encode_MBps` (MB/s, 1.3–60 range). `eps = 0.02` means **2 ratio points** on one axis (≈ 18 % relative
at ratio 0.112) and **0.02 MB/s** on the other (≈ 1.4 % relative at 1.4 MB/s). One number, two
meanings, off by more than an order of magnitude.

Per `tests/noise-floor.csv` (243 rows), the asymmetry that matters is exact:

- `ratio` **CV = 0.000** on every row — byte-deterministic
- `encode_MBps` CV = 5.5 %–35.5 % on available cells (e.g. `doc.md`/`anvil-dp-arith` = 11.403 %,
  `doc.md`/`anvil-dp-rans` = 5.565 %)

So an ε on the **ratio** axis is free — it can be set to any value with no noise cost. An ε on the
**speed** axis must exceed 35.5 % to be safe. The frozen code forces the *same* ε on both. **The
predicate must be split into `eps_ratio` and `eps_speed`, or the speed axis must be removed from the
classification and reported descriptively.**

I now resolve the Q0-vs-SYNTH contradiction recorded in
`SYNTH-MEASUREMENT-FLEDGE.md:540-598`: both are right about their own ε reading, and the
reconciliation is that **they implemented different semantics**. `parity_sweep.py` implements an
*inscribed margin* (ε on the candidate-to-bracket-edge distance), which is why ε ≥ 0.02 dissolves the
five cells. SYNTH implemented ε as *minimum bracket separation* and found the tuple invariant. The
substantive finding SYNTH reports is unaffected and is decisive:

> The tightest inscribed margin is **0.9 % on ratio** (`anvil-shape-rans` vs `zstd-19` on
> `generated.json`), and the **7.8 % encode margin against `xz-9e` is the only thing standing between
> these cells and `DOMINATED`** — and 7.8 % sits **inside** the project's own measured encode
> dispersion band. The retained window is ranking-grade, not citation-grade (pinned-core pre-window
> util 26.9 % / 9.2 %, both above the <5 % gate).

**That is defect B3.** The only five non-dominated cells in the entire retained suite are held there
by a **speed** margin. Q2 is scoped bytes-primary. **A bytes-primary experiment cannot move the axis
that is holding those five cells out of `DOMINATED`.** Q2 as specified is therefore structurally
incapable of resolving the frontier status it exists to test.

---

## 4. ATTACKS

### A1 — Selection bias (severe, and structural)

Three independent selection problems, all pointing the same way:

1. **6 of 13 files carry ≤ 31 % of the weight and cannot move.** `random.bin`, `synth-arith.bin`,
   `src.cpp`, `doc.md` are single-block at 256 KiB; `anvil.exe` and `synth-timeseries.bin` are
   2-block. Pooling them into a 468-cell tally (§2.4) inflates apparent evidence.
2. **Only the 4 largest files have any dose.** 11, 8, 8, 7 blocks. `generated.jsonl` at 11 blocks
   carries the entire measurable geometry effect for a 2.69 MiB file. There is no high-block-count
   file, so the design **cannot reach the regime where E4 measured its 17 %**. The corpus is
   structurally under-powered for the question as the queue states it.
3. **The interesting file is the one most exposed to the negative gate.** `synth-jitter.bin` (mixed
   compressibility by design) is exactly the file whose classification is most sensitive to §3.5's
   sampling-grid change. A geometry result on this corpus will be dominated by one synthetic file's
   gate behaviour.

**Required fix:** predeclare the informative subset (`nblocks ≥ 4` at Class-A geometry: 7 files) and
the negative-control subset (`nblocks ≤ 2`: 6 files), and report them separately. Use the control
subset as the **instrument specificity check** — the harness must produce *zero* geometry effect
there, and if it does not, the harness is broken. This turns a weakness into a free validity gate.

### A2 — Timing endpoint selection (severe)

`bench_native.cpp:14-22`:

```cpp
static double median_speed(size_t input_bytes, int reps, F&& fn) {
    std::vector<double> samples; samples.reserve(reps);
    for(int i=0;i<reps;++i) { auto t0=Clock::now(); fn(); auto t1=Clock::now(); ... }
    std::sort(...); return samples[samples.size()/2];
}
```

Three defects for a geometry comparison specifically:

1. **Median-of-5 is a weak estimator against the measured dispersion.** With reps=5 the median is the
   3rd order statistic; one sample moves it by a large fraction of the interquartile spread. Measured
   CV reaches 35.5 %. Q2 needs **≥ 9 timed reps** with **per-rep rows retained**, not a single median.
   `tests/suite-frozen-bdc90474-reps.csv` (107 KB) shows per-rep retention is already the project's
   practice — the frozen harness just doesn't do it in-process.
2. **No warmup rep is discarded, and block size *is* the warmup.** `fn()` on the first rep pays
   first-touch page faults proportional to the working set allocated. A whole-file arm faults in
   ~2.8 MB of decoder output in one event; an 11-block arm does so in 256 KiB instalments. **The
   first rep's cost is a function of block size** — so the estimator's residual geometry bias is
   largest exactly where the effect is being measured. Discard rep 1 explicitly and **report its
   value** so the discarded magnitude is auditable.
3. **Encoder per-block allocation is real and scales with `nblocks`.** `src/anvil.cpp:4673` copies
   every block: `std::vector<uint8_t> block(input.begin()+off, input.begin()+off+blen);`. Total bytes
   copied are invariant, but allocation/free *events* scale with `nblocks`. This is a genuine
   per-block fixed cost — the `stage-4 ns/block` effect FLEDGE-REVIEW demanded (line 48) — and it
   means the encoder is not geometry-neutral even before the window acts.

**Required fix:** ≥9 reps, rep 1 discarded and reported, per-rep rows retained, median **and** MAD
published, and a declared predicted effect ≥ 7 % before any timing cell is admitted (FLEDGE-REVIEW
line 48; SYNTH line 475(c): "if infeasible **shrink the family rather than raise reps**").

### A3 — Same-job pairing (necessary, currently unspecified, and it exposes build drift)

Pairing is mandatory (Q0 precondition 2, queue line 57) and it must mean *both* arms re-run in one
job on one binary. That is correct — but it means **the retained CSV's Class-A bytes are no longer
Q2's Class-A control.** Q2's fresh Class-A arm is. And the binary has moved.

Verified drift — `git log -- src/anvil.cpp` after the frozen CSV's commit `9caeee9`
("freeze src BDC90474"):

```
dc5c47f hardening: specify and bound BWT framing
e69a9ee hardening: tighten aux BWT wire and CLI validation
c048e55 experiment: add auxiliary-index inverse BWT framing
```

`bench_native.cpp:2` is `#include "../src/anvil.cpp"` — the bench binary **is** the codec, so a
re-run picks up all three commits. All three are BWT/aux-scoped and `src/anvil.cpp:3793` asserts
`bwt_aux` is *"default OFF keeps legacy bytes exact"* — but that is a **code comment, not a
measurement**. Nothing in the retained artifacts verifies that Class-A byte output is unchanged.

**Required fix (pre-dispatch gate G1):** re-run the retained Class-A configuration at
`--block=262144` with all other options at their `bench_native.cpp` defaults, and require **byte-exact
equality against the retained CSV**, per file, plus identical round-trip hashes. If any file differs,
**the delta is build-drift evidence and must be reported as such — it is not a parity result**, and
the entire Q2 baseline must be re-established before proceeding.

### A4 — Binary and RSS comparability (severe; see also §3.6)

- **Binary:** same as A3. One binary for all arms, SHA-256 recorded in the job text, and the G1
  byte-equality gate above. `thread-attestation.csv:2` also records that the retained rows
  **predate the 2026-09-07 parallel-decode plumbing** and that "window not recorded (pre-PR-4)". So
  the retained rows are **not** PR-4 window-attested. Q2 must produce window-attested rows under
  `tests/pr-4-measurement-window-protocol.md` §1-2, and must **not** claim continuity with the
  retained window.
- **RSS:** no instrument in the Class-A harness, and the only existing RSS substrate is provably
  broken (`zstd` constant 1.613 MiB across a 5× input range). Per Q0 precondition 9, RSS stays
  **descriptive-only, in a separate table, with its own attestation**, and never enters a
  cross-family dominance test.
- **Binary size:** the queue already says descriptive-only unless scope matches (line 57). Agreed and
  reinforced — a bytes-only parity arm changes container overhead (per-block framing scales with
  `nblocks`), so binary size would move for a reason unrelated to the codec.

### A5 — Decoder parallelism: the bytes-only fix creates a new cost regression (severe; §3.2, §3.6)

This is the attack the mandate asked for, and it is confirmed at source. Restated as a decision table:

| Q2 finding | Bytes-only action | Decode consequence | RSS consequence |
|---|---|---|---|
| parity materially improves ratio | ship larger blocks | `nthreads` → 1, parallelism **deleted** | single large working set |
| parity does not change ratio | ship nothing | none | none |

If the first row holds, **the bytes-only "fix" is a net performance regression** whose magnitude is
governed by core count, not by the 5–17 % restart tax. On a 4-vCPU GHA runner that is a ~4× decode
regression at parity versus 256 KiB-with-threads; on a 16-core host, ~16×. The queue's own framing —
"more blocks ⇒ more per-block concat/alloc ⇒ decode **slows**" (FLEDGE-REVIEW line 48) — has the
sign **backwards for the arm that improves bytes**: more blocks slows decode but *enables* the
parallelism that makes decode fast. The two effects are opposite and neither is currently measured.

Worse, the parallelism is **free and already implemented** (`--decode-threads`, 1..1024, CRC-gated
embarrassing parallelism, `src/anvil.cpp:4862,4908`). Q2's retained-twin comparison at
`decode_threads=1` throws away a capability the codec ships, and if Q2 then recommends parity, it
recommends removing it.

**Required fix:** Q2 must measure the parallelism axis descriptively — `decode_threads ∈ {1, N}` at
**both** geometries — and report it as a separate, clearly-labelled column that **cannot** enter the
frozen classification. `parity_sweep.py` needs no change for this; the producer must emit the column
and the arbiter must be told to ignore it. This is the one addition that makes a bytes-only parity
conclusion *safe to act on*, and it is **absent from the queue's six-arm notion**.

---

## 5. MINIMUM ARM SET

**Design principle:** the parity question is **binary** (matched vs unmatched). It is *not* a dose
curve. The minimum identifying set is therefore a **2 × 2 factorial in two non-size factors**, plus
one fidelity gate. Block size takes exactly **two** values. Nothing here is a block-size sweep.

### Fixed, pinned, non-negotiable across all arms

| pin | value | source |
|---|---|---|
| parse lane | Class-A token lane only — **no `ratio`, no BWT** | §2.2 (B1) |
| wire revision | 1 (`parse != ratio`) | `anvil.cpp:4667` |
| `--block` cap | 64 MiB (rev-1 max) — whole-file for this corpus | `anvil.cpp:5002` |
| `bwt_aux` | off / unreachable | `anvil.cpp:3793,4266` |
| `decode_threads` | **1** in the classification vehicle | `anvil.cpp:3796`; matches retained + reference provenance |
| encode threads | 1 (host default; no threading code in producer) | `thread-attestation.csv` |
| `shape_states`, `stream_lambda`, `channels`, `pnra`, `hotop_*`, `surprise` | exactly the `bench_native.cpp:61-78` call arguments for the config under test | reproducibility |
| binary | one SHA-256, recorded in job text | A4 |

### The arms

Per file, per configuration under test:

| # | arm | `--block` | `--negate` | role |
|---|---|---|---|---|
| **G0** | retained twin | 262 144 | on | **fidelity gate** — must byte-match the retained CSV exactly. Not data. |
| **G1** | parity, gate on | 67 108 864 | on | **the parity arm** — the only arm that answers the question |
| **G2** | parity, gate off | 67 108 864 | off | isolates the negative-gate component (§3.5) |
| **G3** | Class-A, gate off | 262 144 | off | closes the 2×2; with G0/G1 gives the **pure window** effect |

- **G1 − G0** = total geometry effect (window + gate + allocation).
- **(G1 − G2) − (G0 − G3)** = gate-sampling component, removed.
- **G0 − G3** = pure window/entropy-restart effect on bytes. **This is the parity answer.**
- **G0 − G3 on the 6 single/near-single-block files must be exactly 0 bytes.** That is the free
  specificity check (A1). Non-zero ⇒ harness bug, stop.

### Configurations under test — and the cap that prevents a sweep

Only the **5 retained FRONT-GAP configurations**, because they are the only cells where the
classification is not already `DOMINATED`:

`anvil-mdl-rans`, `anvil-mdl-rans-l0`, `anvil-mdl-rans-l001`, `anvil-shape-rans`, `anvil-shape-rans-l0`

(Q0-DENSE-RETAINED-RESULT.md:37-42.) **5 configs × 4 arms = 20 rows per file × 13 files.** That is not
a block-size sweep: the block axis is binary, and the second factor is a filter, not a size.

**Do not add `anvil-dp-rans`, `anvil-greedy-*`, `anvil-hotop-*` etc.** as parity arms. They are already
`DOMINATED` at 256 KiB; geometry can only *worsen* a dominated cell's byte axis, so they carry no
information about parity and would convert Q2 into the 20-arm dense vehicle Q0 explicitly forbade
(Q0 line 86: "not a rerun of the old 20-arm dense vehicle").

### Mandatory additional outputs

1. **Achieved block count per (file, arm)** — not a nominal window (§2.3). This is the Q1 census
   SYNTH-MEASUREMENT-FLEDGE:47 already identified as missing project-wide.
2. **Per-block cost column** — bytes and ns per block, encoder and decoder, so fixed vs
   size-proportional cost is separable (FLEDGE-REVIEW:48).
3. **Descriptive parallelism column** — `decode_threads ∈ {1, N}` at **both** geometries, hard-flagged
   as **non-classifying** (A5).
4. **Per-rep timing rows** — all reps, rep 1 separately reported, median + MAD (A2).
5. **`evidence_role` per row**, from the lock, per SYNTH-MEASUREMENT-FLEDGE E1. Every one of these
   rows will be **discovery-evidence**, because `tests/corpus` is 24 files / ~4 independence units with
   zero held-out structured or numeric units. **"citation-grade" is forbidden** for this job
   (SYNTH:181-200 — the `generated.json` 0.1325 anchor is a fact about `make_smoke_corpus.py`, not
   about the world).

---

## 6. PRE-DISPATCH GATES

Ordered. Each is cheap and each can void the job.

| # | gate | criterion if failed |
|---|---|---|
| **G1** | **Binary byte-equality.** Re-run G0 at `--block=262144`; require byte-exact match to `tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` on every file. | **STOP.** Report as build drift (3 commits since `9caeee9`). No parity claim admissible. |
| **G2** | **Coordinator ruling on B2.** Which is relaxed: Q0 precondition 7 (no timing-driven token) or Q0's "frozen predicate" requirement? Concretely: does the classification vehicle use the byte+time predicate, or a byte-only predicate with timing reported descriptively? | **STOP.** Unimplementable as specified. |
| **G3** | **ε ruling on B3.** Fix `eps_ratio` and `eps_speed` as separate constants derived from `tests/noise-floor.csv` (`eps_ratio` free — ratio CV = 0; `eps_speed` ≥ 0.355 or the speed axis leaves the classification). State which axis carries the token. | **STOP.** The retained 5-cell status is undetermined until this is fixed, independent of Q2. |
| **G4** | **Predictive-power declaration.** State the expected byte effect per informative file from E4 (`tests/block-oracle.csv`, SHA-256 `3E85D690…`) **and** declare the gate-sampling component (§3.5) as a separately predicted term. Declare ≥ 7 % or drop the timing arms (SYNTH:475). | Timing arms inadmissible. Byte arms may still run. |
| **G5** | **Single-block specificity.** G0 − G3 = 0 bytes on all 6 files with `nblocks ≤ 2`. | **STOP.** Harness is not geometry-controlled. |
| **G6** | **Parallelism disclosure.** Confirm the parallelism column is emitted and hard-flagged non-classifying; confirm `decode_threads=1` in the classification vehicle. | **STOP.** A bytes-only parity conclusion without this is unsafe to act on (A5). |
| **G7** | **Artifact pinning.** Record SHA-256 for the retained CSV, `parity_sweep.py`, `noise-floor.csv`, `tests/block-oracle.csv`, the E4 memo, and the Q2 binary, in the job text. | SYNTH:626 precondition (a) unmet. |

**G2 and G3 are the two that block.** They are coordinator decisions, not implementer's choices, and
they cannot be resolved by writing better code — G2 in particular is a direct contradiction between
two mandatory clauses of Q0's own precondition list.

---

## 7. WHAT Q2 WILL AND WILL NOT BE ABLE TO CONCLUDE

Stated plainly, so the result is not over-read on landing:

**Q2 can establish:**
- whether ANVIL's byte axis improves, and by how much, when its history window matches the
  references' effective window — measured on 7 informative files with a gate-controlled design;
- whether the classification of the 5 retained FRONT-GAP cells changes under matched geometry;
- whether the negative-gate sampling grid is a material contributor to that change;
- whether the decoder-parallelism cost of parity is material on this runner.

**Q2 cannot establish:**
- **A general parity claim.** The corpus maxes at 2.69 MiB, below Brotli's 4 MiB window, so the
  reference side is never segmented and the hypothesis is untestable here (§2.3).
- **A block-size optimum.** Two points identify a difference, not a curve. Do not report a
  recommendation; report the matched-vs-unmatched contrast only.
- **A performance claim.** Hosted-runner timing is ranking-grade (Q0 precondition 6; SYNTH:586-587
  records pre-window util 26.9 % / 9.2 % against a < 5 % gate). Timing is descriptive and
  classification guard only.
- **Anything about the retained frontier's citation-grade status.** Per B3, the 5 cells are held out
  of `DOMINATED` by a speed margin inside the noise band. **Byte parity cannot fix that.** If those
  5 cells are what the project needs adjudicated, Q2 is the wrong instrument and the fix is a
  citation-grade re-measurement of the retained window (which Q0 precondition 6 and SYNTH:586 both
  say was never done) — a strictly larger job than Q2.
- **Any held-out or structured/numeric promotion.** `evidence_role = discovery` for all rows (§5).

**Framework guard, verbatim (SYNTH-NOVELTY-FLEDGE:383, queue NQ-8).** A block-size finding is a
**configuration fix**, one write-up away from "adaptive blocking is our mechanism" — occupied by
Brotli's meta-block splitter and zstd's `targetCBlockSize`. Q2 must not authorize a block-size-selection
or adaptive-blocking mechanism claim.

---

## 8. REASON SUMMARY FOR THE VERDICT

| # | finding | severity | disposition |
|---|---|---|---|
| **B1** | Class-A vs Class-B confounds geometry with backend + revision; measured gap ~4.5×, none of it geometry | **blocker** | Fix: Class-A-only arms, `--block` swept in rev 1 |
| **B2** | Frozen predicate mixes bytes and timing; Q0 precondition 7 is therefore unsatisfiable | **blocker** | Coordinator ruling (G2) |
| **B3** | The 5 retained FRONT-GAP cells are held by a 7.8 % encode margin inside a 5.5–35.5 % noise band; Q2 is bytes-primary | **blocker** | Coordinator ruling (G3) + scope honesty (§7) |
| **S1** | Parity geometry collapses decode parallelism (`nthreads = min(threads, nblocks)`); a bytes-only fix creates a 2–4×+ decode regression | **structural** | Add non-classifying parallelism column (G6) |
| A1 | 6/13 files geometry-invariant; only 3 files > 8 blocks | severe | Predeclare informative vs control subsets; G5 |
| A2 | median-of-5, no discarded warmup rep; block size *is* the warmup | severe | ≥9 reps, rep 1 reported, per-rep rows, ≥7 % declared |
| A3 | Binary drift: 3 commits after `9caeee9`; retained rows predate threading plumbing | severe | G1 byte-equality gate |
| A4 | No RSS instrument in Class-A harness; existing RSS substrate provably broken (`zstd` 1.613 MiB constant over 5× input range) | severe | RSS descriptive-only, separate table, own attestation |
| A5 | Parallelism axis unmeasured and orthogonal to the question | severe | G6 |
| §3.3 | Aux index bytes ∝ block *count*; Q9's W1024 is the same constant; both levers same-signed | moderate | Exclude all ratio/BWT arms |
| §3.4 | E4's 5–17 % is a **BWT MTF/context** warmup on 196-block Silesia files — not the Class-A mechanism, not this corpus | moderate | G4: declare predicted effect from the pinned artifact, treat transferability as UNVERIFIED |
| §3.5 | `--negate` (default **on**) samples on a grid locked to block size; changes which bytes reach the parser | moderate | 2×2 factorial (G0–G3) |
| §3.7b | `eps` applied to incommensurable units; **vacuous at the decisive `eps=0.0`** (`parity_sweep.py:123`) | moderate | Split `eps_ratio` / `eps_speed` under G3 |

---

## 9. FINAL VERDICT

> # REVISE
>
> **Do not dispatch Q2 as written.** Three blockers, each independently sufficient:
>
> 1. **B1** — the named Class-A/Class-B comparison measures the BWT backend, not the window. A
>    ~4.5× backend gap would be reported as a geometry win.
> 2. **B2** — Q0's precondition 7 (no timing-driven `FRONT-CROSSING`) and Q0's frozen-predicate
>    requirement are mutually unsatisfiable as implemented in `parity_sweep.py:44-77`.
> 3. **B3** — the only cells Q2 exists to adjudicate are held out of `DOMINATED` by a **speed** margin
>    inside the project's own measured noise band. Q2 is bytes-primary and cannot move that axis.
>
> **Do not cancel.** The question is real, unmeasured, and the 0-crossing result is genuinely
> geometry-conditional: 4/13 corpus files are already single-block at 256 KiB, and no corpus file
> exceeds 4 MiB, so the reference side is never segmented and the retained grid cannot speak to
> parity at all.
>
> **Path to dispatchable — two coordinator rulings and six mechanical fixes:**
>
> - **Coordinator:** rule on B2 (which Q0 clause relaxes) and B3 (split `eps_ratio` from `eps_speed`;
>   declare whether the speed axis carries the token). Neither is an implementer's decision.
> - **Implementer:** adopt the §5 arm set (4 arms, 2 block values, 5 FRONT-GAP configs only, plus the
>   G0 fidelity gate); pin `parse != ratio`, `decode_threads = 1`, `bwt_aux` off; emit block count,
>   per-block cost, non-classifying parallelism, and per-rep timing; label every row
>   `evidence_role = discovery`; and run gates G1 and G5 before any parity cell is admitted.
>
> **One finding must reach the coordinator regardless of whether Q2 proceeds:** S1. Block parity and
> decoder parallelism are the same variable in this container format. The byte-optimal geometry
> deletes the decode-parallelism option outright, by construction, at any thread count. If a
> bytes-only parity result later authorizes a block-size change, it will ship a decode regression that
> no bytes-only experiment can see. **Q2 must measure the parallelism axis descriptively before any
> parity conclusion is acted on — otherwise the fix for the byte deficit becomes the performance
> deficit.**
---

# 10. COORDINATOR CORRECTION AND RULING — appended 2026-10-02

This section supersedes **§5's contrast labels and §5's specificity check**, and updates the verdict
per the coordinator's ruling on B2/B3. Everything in §1–§4 and §6–§9 stands unless explicitly
retired below.

## 10.1 My §5 contrast algebra was wrong — the coordinator's correction is correct

I labeled the four arms' contrasts incorrectly. Two of my four labels were wrong, and the error
propagated into the specificity check. Stating the error plainly rather than quietly patching it.

**The design (unchanged, and correct):**

| arm | `--block` | `--negate` |
|---|---:|---|
| G0 | 262 144 | on |
| G1 | 67 108 864 | on |
| G2 | 67 108 864 | off |
| G3 | 262 144 | off |

**What I wrote in §5, versus what is actually true:**

| contrast | I labeled it | it actually is | verdict |
|---|---|---|---|
| `G1 − G0` | "total geometry effect (window + gate + allocation)" | simple effect of **geometry, gate held ON** | **wrong** |
| `G2 − G3` | *(not labeled)* | simple effect of **geometry, gate held OFF** | **the pure-window contrast** |
| `G0 − G3` | "pure window/entropy-restart effect — **this is the parity answer**" | simple effect of **gate, block held small** | **wrong, and it was load-bearing** |
| `G1 − G2` | *(not labeled)* | simple effect of **gate, block held large** | — |
| `(G1 − G2) − (G0 − G3)` | "gate-sampling component, removed" | **interaction term** `I` | right formula, wrong status (I called it a correction term, not an estimable main-effect-adjacent quantity) |

**The correct decomposition**, with `I = (G1 − G0) − (G2 − G3)`:

```
I        = (G1 - G2) - (G0 - G3)       interaction: gate effect depends on block size

G2 - G3  =  geometry effect, gate OFF  <-- PURE WINDOW / entropy-restart effect
G1 - G0  =  geometry effect, gate ON   =  (G2 - G3) + I      [identity check: holds]

G0 - G3  =  gate effect at small block
G1 - G2  =  gate effect at large block
```

The identity `(G2 − G3) + I = G1 − G0` verifies the decomposition closes.

**The substantive point I got wrong, and it matters.** I claimed `G1 − G0` was a *total* geometry
effect "including gate." It is not a total effect, and it does not need to be. Because the negative
gate's sampling grid is locked to block size (`stride = n/1024`, `src/anvil.cpp:3810`), the gate
*mediates* part of the geometry contrast — but the factorial isolates exactly that mediation as `I`.
So `G1 − G0` is cleanly "geometry with the gate on," and the gate-mediated share of it is `I`, not an
unremoved residual.

**Consequently: the pure-window parity number is `G2 − G3`, not `G0 − G3`.** I had named the gate
contrast as the parity answer. Under the corrected algebra:

- `G2 − G3` answers *"how much do ANVIL's bytes move when the window is matched, with the
  incompressibility pre-filter disabled"* — the clean window/entropy-restart sensitivity.
- `G1 − G0` answers *"how much do the bytes move in the configuration that would actually
  ship"* (gate on, default), and equals `(G2 − G3) + I`.
- `I` answers *"how much of the shipped-geometry byte movement is caused by the gate's
  sampling grid shifting rather than by the window."* Because §3.5 established the grid is a
  mathematical function of block size, **`I` is expected to be non-zero by construction.** A
  measured `I = 0` would itself be a red flag worth investigating, not a clean result.

Both `G1 − G0` and `G2 − G3` must be reported. Neither alone answers the question.

## 10.2 Second correction: the specificity check was the wrong contrast on the wrong file set

§5 asserted *"G0 − G3 on the 6 single/near-single-block files must be exactly 0 bytes."* Both halves
are wrong.

1. **Wrong contrast.** `G0 − G3` is the **gate** contrast. Gate on vs off can legitimately differ
   even when geometry is a no-op — `probe_incompressible` may fire at one block and change raw vs
   compressed output. It is a *measurement*, never a zero-check.
2. **Wrong file set — it is 4 files, not 6.** A file is a true control only if it is single-block at
   *both* settings, so that geometry provably cannot act:

| file | bytes | `nblocks` @ 256 KiB | `nblocks` @ 64 MiB | control? |
|---|---:|---:|---:|---|
| `random.bin` | 262 144 | 1 | 1 | **yes** |
| `synth-arith.bin` | 256 000 | 1 | 1 | **yes** |
| `src.cpp` | 32 512 | 1 | 1 | **yes** |
| `doc.md` | 2 724 | 1 | 1 | **yes** |
| `anvil.exe` | 268 800 | 2 | 1 | no — geometry acts |
| `synth-timeseries.bin` | 280 000 | 2 | 1 | no — geometry acts |

**Corrected gate G5 (specificity / harness validity):** on those **4** files,
**`G1 − G0 = 0` bytes and `G2 − G3 = 0` bytes.** Geometry cannot act there, so any non-zero value is
a harness or attribution bug → **STOP**. `G0 − G3` is reported on those files as a measurement of
whether the negative gate fires at one block; it is expected to be non-zero for some inputs and is
**not** a failure condition. Note `I = −(G0 − G3)` on these files by construction, so `I` is not
zero there either — another reason `I` must not be used as a global validity check.

The 2-block files (`anvil.exe`, `synth-timeseries.bin`) move to the informative set.

## 10.3 Coordinator ruling on B2 and B3 — recorded

> **Ruling (2026-10-02).**
>
> 1. **Q2 will NOT emit `FRONT-CROSSING` and will NOT reclassify the frontier.**
> 2. The **frozen predicate is used only for G0 fidelity/reproduction.**
> 3. Q2 emits **exact byte/geometry sensitivity plus descriptive timing/parallelism**.
> 4. **No `eps_ratio` / `eps_speed` thresholds will be invented post hoc.**
> 5. The **5 `FRONT-GAP` cells remain citation-grade unresolved** until a **separate proper timing
>    remeasurement**.

This is the correct resolution and it is strictly better than either option I offered in §6. I framed
B2 and B3 as *irreconcilable clauses requiring a choice between them*; the ruling dissolves the
obligation instead of choosing. Q2 no longer classifies anything, so the predicate's byte/timing
conflation (§3.7a), the vacuous `eps = 0.0` at the decisive setting (`parity_sweep.py:123`), and the
incommensurable-units defect (§3.7b) all become **out of scope for Q2** rather than things to be
patched. The frozen predicate is now an *input* to a reproduction check, not an *output* of Q2.

## 10.4 Disposition of every gate in §6 under the ruling

| gate | was | now | reason |
|---|---|---|---|
| **G1** binary byte-equality vs retained CSV | severe | **RETAINED — mandatory** | unchanged; still the baseline-admission gate |
| **G2** predicate ruling (B2) | **blocker** | **WITHDRAWN — moot** | Q2 emits no classification token |
| **G3** ε ruling (B3) | **blocker** | **WITHDRAWN — moot for Q2** | no threshold invented, no token emitted |
| **G4** declared predicted effect ≥ 7 % | moderate | **RETAINED, tightened** | timing is now purely descriptive; the ≥ 7 % declaration still gates whether timing arms are run at all |
| **G5** specificity | severe | **RETAINED, CORRECTED** | now `G1−G0 = 0` and `G2−G3 = 0` on **4** files (§10.2) |
| **G6** parallelism disclosure | severe | **RETAINED, ELEVATED** | see §10.5 |
| **G7** artifact pinning | moderate | **RETAINED** | unchanged; now also pins the successor job's obligations |
| *(new)* **G8** | — | **ADDED** | no `FRONT-CROSSING`, no frontier reclassification, no post-hoc ε in any Q2 output — asserted by the arbiter, not by convention |

**G8 is the machine-checkable form of the ruling.** Because Q2's outputs feed a predicate that a
later reader might otherwise re-run, the arbiter must *refuse* to classify Q2 rows: any attempt to
emit `FRONT-CROSSING`/`FRONT-GAP` from a Q2 arm is an error, not a finding. Stating it as a gate
rather than a note is the difference between a constraint and a hope.

## 10.5 The ruling makes S1 (decoder parallelism) the highest-value output in the job

Under the ruling, Q2's byte output no longer feeds any classification, and its timing output is
descriptive. The parallelism column is therefore the **only** channel through which the structural
finding from §3.2 can surface — and that finding is the one with real consequences:

> At parity the container is one block, so `nthreads = min(decode_threads, segs.size())`
> (`src/anvil.cpp:4908`) is forced to 1. The byte-optimal geometry **deletes** the CRC-gated
> embarrassingly-parallel decode path (`src/anvil.cpp:4862`) outright, at any thread count.

Concretely, Q2 must report, at **both** geometries and clearly flagged non-classifying (G6/G8):

- `decode_threads = 1` (the comparison vehicle — matches retained + reference provenance), and
- `decode_threads = N` (the runner's available parallelism),

with the **ratio of the two** stated per file. If the `N`-over-`1` decode gain is material at
256 KiB and collapses to ~1.0 at parity, then a bytes-only parity recommendation is a **net
performance regression of order N×**, and that number must be in the same table as the byte
sensitivity. Without it, Q2 hands the coordinator a byte win and hides the cost.

This is now the single most consequential number Q2 will produce, and it is a *cost*, not a prize.

## 10.6 "Exact byte" is determinism, not validity — the honesty burden rises under the ruling

The ruling makes bytes Q2's primary and near-only output. That raises the over-reading risk, so the
distinction from §5 must be held firmly:

- `tests/noise-floor.csv`: **`ratio` CV = 0.000 on every row.** Byte output is *exactly* reproducible.
- `tests/corpus` is 24 files ≈ 4 independence units, with **zero** held-out structured/numeric units;
  the historical PE set and Sino-US DrugQA V1 are already consumed.

So **exactness is a property of the codec; validity is a property of the corpus.** Q2 will produce
byte numbers that are reproducible to the byte and still not admissible as held-out evidence. Every
Q2 row carries **`evidence_role = discovery`** (SYNTH-MEASUREMENT-FLEDGE E1); the token
"citation-grade" is **forbidden** for this job. Concretely: `generated.json`'s 0.1325 anchor is a
fact about `make_smoke_corpus.py`'s schema, not about the world, and a byte-parity result measured on
that corpus inherits exactly that property.

## 10.7 The ruling relocates the frontier obligation — it does not discharge it

Item 5 defers the 5 `FRONT-GAP` cells to "a separate proper timing remeasurement." That job does not
exist, has no owner, and no queue entry. The obligation has been **moved, not met**, and §7's
conclusion stands unchanged: a bytes-primary experiment cannot resolve cells whose only non-dominated
status rests on a 7.8 % encode-speed margin sitting inside a 5.5–35.5 % measured dispersion band.

Two consequences for the coordinator:

1. **A successor job must be created and owned.** Its scope is not Q2's: it must re-measure the
   retained window under PR-4 attestation (the retained rows predate the threading plumbing —
   `tests/thread-attestation.csv:2`), which is a strictly larger job than Q2.
2. **Q0's own precondition 7 and its frozen-predicate requirement remain contradictory** for any job
   that *does* classify. The ruling sidesteps this for Q2 only. The defect in
   `parity_sweep.py:44-77` and the `eps` defects in §3.7 are **deferred, not fixed** — they must
   travel with the obligation into the successor's preregistration, or the same unsatisfiable pair
   will be re-encoded when someone finally tries to classify these rows.

## 10.8 B1 is unchanged and still the live implementation constraint

The ruling does not touch B1, and B1 is not a coordinator decision — it is an implementer constraint
that §5 already specifies:

- arms are **Class-A token-lane only**; `--parse=ratio` and every BWT/mode-17 arm are **excluded**;
- parity is reached with `--block=67108864` **inside rev 1** (cap 64 MiB, `src/anvil.cpp:5002`),
  which is whole-file for this corpus (max file 2 815 267 B);
- this keeps `bwt_aux` unreachable (`src/anvil.cpp:3793`, `:4266`) and so keeps the §3.3 aux/Q9
  confound out of Q2 entirely.

If B1 is not honoured the job reports a ~4.5× backend win as a geometry win and must be rejected.

---

# 11. REVISED FINAL VERDICT

> # DISPATCH — of the revised spec only
>
> **The verdict moves from REVISE to DISPATCH**, because the coordinator's ruling removes both
> coordinator-shaped blockers. B2 and B3 existed only because Q2 was going to classify; Q2 no longer
> classifies, so the unsatisfiable predicate pair, the ε vacuum, and the unownable 5 cells are all out
> of Q2's scope. **What remains is entirely implementer work that this report already specifies** —
> there is no outstanding decision required from anyone.
>
> **Dispatch is authorized for this shape and no other:**
>
> | | |
> |---|---|
> | Arms | **G0/G1/G2/G3** — 4 arms, **2 block values** (262 144, 67 108 864), `--negate` 2×2 |
> | Configs | the **5 retained `FRONT-GAP` configs only** — `anvil-mdl-rans`, `-l0`, `-l001`, `anvil-shape-rans`, `-l0` (20 rows/file; **not** a block-size sweep) |
> | Pins | `parse != ratio`, rev 1, `bwt_aux` unreachable, `decode_threads = 1` in the comparison vehicle, `bench_native.cpp:61-78` option literals verbatim |
> | Outputs | `G1−G0`, `G2−G3`, `I = (G1−G2)−(G0−G3)`, `G0−G3`, `G1−G2`; achieved block count; per-block bytes/ns; **descriptive** timing (≥9 reps, rep 1 reported); **descriptive** parallelism (`threads ∈ {1,N}`, ratio stated); `evidence_role = discovery` |
> | Forbidden | any `FRONT-CROSSING`/`FRONT-GAP` emission or frontier reclassification (G8); any post-hoc ε; the words "citation-grade"; any `ratio`/BWT arm (B1) |
> | Gates | G1 byte-equality, G4 declared effect ≥ 7 %, G5 specificity (`G1−G0 = G2−G3 = 0` on the **4** control files), G6 parallelism disclosure, G7 artifact pinning, G8 arbiter refusal |
>
> **Three exact reasons the verdict changed, and one reason it did not.**
>
> 1. **B2 dissolved.** Q2 emits no classification token, so `parity_sweep.py`'s byte/timing conflation
>    is never invoked. *(was: blocker)*
> 2. **B3 dissolved.** Q2 makes no frontier claim, so the 7.8 %-speed-margin problem in the 5
>    `FRONT-GAP` cells is no longer load-bearing on Q2. *(was: blocker)*
> 3. **B1 was never a coordinator decision.** Class-A-only rev-1 arms reach parity without touching
>    the BWT backend or the aux confound. Already specified; no ruling needed. *(was: blocker, now an
>    implementer constraint)*
> 4. **Not a reason to upgrade to "unconditional dispatch":** §7's scope limits are unchanged. Q2
>    still cannot support a general parity claim (no corpus file exceeds 4 MiB, so the reference side
>    is never segmented), cannot locate a block-size optimum (two points identify a difference, not a
>    curve), and cannot make a performance claim (hosted-runner timing is ranking-grade).
>
> **Two obligations leave with this ruling, and both must be tracked:**
>
> - **The successor timing remeasurement (§10.7) does not yet exist.** The 5 `FRONT-GAP` cells are now
>   citation-grade unresolved *and unowned*. This is the one thing the ruling creates rather than
>   solves, and it is the larger job.
> - **The predicate and ε defects are deferred, not fixed.** They must be carried into that
>   successor's preregistration, or the same unsatisfiable requirement will be re-encoded the first
>   time someone classifies these rows.
>
> **And the finding that must reach the coordinator whatever Q2 reports:** block parity and decoder
> parallelism are the same variable in this container format (`src/anvil.cpp:4908`). The byte-optimal
> geometry deletes ANVIL's parallel decode path at any core count. Q2 is now the only job that will
> ever measure that cost. If its parallelism column is omitted or read as descriptive-and-then-
> forgotten, **the fix for the byte deficit becomes the performance deficit** — and nothing in the
> queue will catch it.
---

# 12. CONSTRUCTIVE-DRAFT AUDIT — pre-dispatch verification

Requested verification of the final constructive draft against three questions: (i) are the exact
argv/settings for all five retained `FRONT-GAP` configs preserved, (ii) does the draft introduce a new
materiality/epsilon threshold, (iii) are deterministic byte contrasts exact values rather than gates.

**Result: (ii) and (iii) are now clean. (i) was NOT clean — §11 asserted "option literals verbatim"
without writing them out, and the literal set contains a byte-affecting trap that would have failed
G1 for the wrong reason.** §13 is the corrected constructive spec and supersedes §5's arm list and
§11's dispatch table.

## 12.1 FINDING — `--stream-lambda` is a byte-affecting trap, and §11 never recorded it

`tools/bench_native.cpp:24` declares:

```cpp
bench_anvil(src, name, parse, lit, entropy, reps,
            uint32_t shape_states=28, double stream_lambda=0.04,
            bool channels=false, bool pnra=false,
            bool hotop_rlzp=false, bool hotop_budget=false)
```

The **harness** default for the J-weight is **0.04**. The **codec** default is different:

```
src/anvil.cpp:3783   double stream_lambda=0.01;  // J-cost decode-weight (pre-registered binding value 0.01)
src/anvil.cpp:1471   static double g_stream_lambda = 0.01;
```

So `anvil-mdl-rans` and `anvil-shape-rans` — which pass **no** explicit λ and therefore inherit
`bench_anvil`'s **0.04** — were measured at **λ = 0.04**, while the CLI would give **0.01**.

**Consequence:** a Q2 arm written as `--parse=mdl --literal=o0 --entropy=rans --block=262144`
produces **different bytes** than the retained CSV, and **G1 fails for a settings reason, not a build
reason.** The failure signature is actively misleading:

| config | retained λ | CLI default λ | would G1 pass by accident? |
|---|---:|---:|---|
| `anvil-mdl-rans` | **0.04** | 0.01 | no — fails |
| `anvil-mdl-rans-l0` | 0.0 | 0.01 | no — fails |
| `anvil-mdl-rans-l001` | **0.01** | 0.01 | **yes — passes by coincidence** |
| `anvil-shape-rans` | **0.04** | 0.01 | no — fails |
| `anvil-shape-rans-l0` | 0.0 | 0.01 | no — fails |

**2 of 5 fail, 1 passes coincidentally, 2 fail.** A reviewer diagnosing G1 failure would reasonably
conclude build drift (the §4/A3 hypothesis), investigate three commits, and never find the cause.
This is the single most likely way Q2 dies for the wrong reason, and it is exactly what §11's
"verbatim" assertion would have concealed.

**Precedence hazard, also verified.** `src/anvil.cpp:4959` reads the environment **before** the argv
loop; `--stream-lambda=` is parsed at `:4977`, inside the loop. Therefore:

- **argv beats env** — `--stream-lambda=N` overrides `ANVIL_STREAM_LAMBDA`;
- but a **runner-level `ANVIL_STREAM_LAMBDA` sets the baseline** that argv then overrides, so an
  unset-vs-set inconsistency between the four arms of one factorial cell would silently produce a
  four-way λ confound.

Q2 must therefore **both** pass `--stream-lambda=` explicitly on every arm **and** assert
`ANVIL_STREAM_LAMBDA` is unset in the job environment.

## 12.2 Full settings audit — every option, source of value, and whether it must be pinned

Derived from `bench_native.cpp:25-31` (what the harness sets) against `src/anvil.cpp:3769-3800`
(what the codec defaults to) and `:4959-4982` (the CLI surface):

| option | retained value (all 5 configs) | source | CLI pin required? |
|---|---|---|---|
| `parse` | `mdl` / `shape` | `bench_native.cpp:70-74` | **yes** |
| `literal` | `o0` | `:70-74` | **yes** |
| `entropy` | `rans` | `:70-74` | **yes** |
| `shape_states` | `28` | `:24` default param | yes (explicitness) |
| `stream_lambda` | **`0.04` / `0.0` / `0.01` — three distinct values** | `:24` default param | **yes — byte-affecting trap** |
| `quiet` | `true` | `:25` | yes |
| `channels` | `false` | `:27` | yes |
| `pnra` | `false` | `:28` | yes |
| `hotop_rlzp` | `false` | `:29` | yes |
| `hotop_budget` | `false` | `:30` | yes |
| `negate` | `true` (inherited) | `anvil.cpp:3778` | **yes — this is the G0/G1 vs G2/G3 factor** |
| `boundary` | `false` (inherited) | `:3777` | yes |
| `stream_suite` | `true` (inherited) | `:3780` | yes |
| `stream_ctx` | `true` (inherited) | `:3781` | yes |
| `surprise` | `12` (inherited) | `:3773` | yes |
| `max_chain` | `48` (inherited) | `:3770` | yes |
| `max_match` | `65535` (inherited) | `:3771` | yes |
| `ariref` | `false` (inherited) | `:3786` | yes |
| `decode_threads` | `1` (inherited) | `:3796` | yes |
| `block_size` | `262144` (inherited) | `:3769` | **yes — the geometry factor** |

Every option is pinned explicitly in the final spec (§13.1), so no arm depends on a codec default.
That is deliberate: the parity arm sits at `--block=67108864`, which is **exactly** the rev-1 cap
(`64u<<20`, validated at `:5002` as `> cap` → legal), so a default drift in either direction would
break it — upward by throwing, downward by silently producing a 2-block file.

**`reps` is byte-neutral — verified.** `bench_native.cpp:31` computes `packed.size()` **once, outside**
both timing loops; reps enters only `:32-33`. So raising reps from the harness default 5 to ≥9
(§4/A2) **cannot** change byte output and cannot affect G1. This is the one permitted deviation from
the retained harness, and it is confined to the timing estimator.

## 12.3 Threshold audit — one numeric band exists, it is inherited, and it never touches bytes

Classification used: a **materiality/ε threshold** is a numeric band applied to a measured value that
decides accept/reject. Exact equality on a deterministic quantity is *not* a threshold — it is
reproduction.

| draft element | is it a threshold? | disposition |
|---|---|---|
| `G1` byte-equality vs retained CSV | **no** — exact equality, `ratio` CV = 0.000 | keep; reproduction, not materiality |
| `G4` declared effect **≥ 7 %** | **yes** — a numeric band | **inherited, not invented**; scope to timing only |
| `G5` "must be exactly 0 bytes" | **no** in substance, but I framed it as a STOP gate | **demoted** to an exact-value expectation + declared anomaly (§12.4) |
| `G6` parallelism disclosure | no — output requirement | keep |
| `G7` artifact pinning | no — exact hash equality | keep |
| `G8` arbiter refusal | no — output prohibition | keep |
| `G2` / `G3` ε rulings | would have been | **withdrawn** in §10.4; `eps_ratio`/`eps_speed` are never defined |
| `≥ 9 reps` | no — sample size / power | keep; byte-neutral (§12.2) |

**The ≥ 7 % is inherited verbatim.** `REMOTE-EXPERIMENT-QUEUE-FLEDGE-REVIEW.md:48`:

> *"Timing arms admissible **only** with a declared ≥ 7 % predicted effect."*

It is a pre-existing queue rule, not something this critique added, so removing it would itself be a
draft change. But two honest qualifications are owed:

1. **Its rationale has partly collapsed.** The ≥ 7 % appears in a table row whose surrounding framing
   — *"Re-scope: RSS-primary… the predicted decode sign is **negative**. More blocks ⇒ more per-block
   concat/alloc ⇒ decode **slows**"* — was **RETRACTED** at `FLEDGE-REVIEW:329` (C2), and its sign is
   **superseded by S1**: more blocks means `nblocks > 1`, which is exactly what makes
   `decode_threads > 1` possible (`anvil.cpp:4908`). So Q2 must declare the predicted effect
   **signed per arm**, not as one global negative direction.
2. **Its justification under the ruling is power, not materiality.** Timing now feeds no
   classification, so the band cannot be justifying an accept/reject on a frontier claim. Its only
   surviving role is CI-cost discipline against a measured `decode` dispersion. It stays **scoped to
   timing arms** and **never applies to bytes.**

**No replacement number is invented here, and none may be.** Byte contrasts get **no band at all** —
they are exact integers. The measured `decode` dispersion (`tests/noise-floor.csv`) is reported
alongside so a reader can judge power; Q2 does not adjudicate it.

## 12.4 Deterministic byte contrasts — demoted from gates to exact values

`tests/noise-floor.csv` reports **`ratio` CV = 0.000 on every one of its 243 rows.** Byte output is
deterministic, so **no contrast on the byte axis can be uncertain, and a band on one is
incoherent** — a contrast is either an exact integer or it is a bug. Applying a materiality
threshold to a deterministic quantity is the same error class as `parity_sweep.py`'s `eps`, just
smaller. My §10.2 G5 inherited that error by dressing an exact expectation as a STOP condition.

**Corrected treatment.** All five contrasts are **reported as exact byte integers**, per file, per
config, with no band:

```
Δ_ship  = G1 − G0      exact bytes   geometry, gate on   (= Δ_pure + I)
Δ_pure  = G2 − G3      exact bytes   geometry, gate off  <- the window/entropy-restart sensitivity
I       = (G1 − G2) − (G0 − G3)   exact bytes   interaction
Δ_gate_small = G0 − G3 exact bytes
Δ_gate_large = G1 − G2 exact bytes
```

On the **4** single-block control files (`random.bin`, `synth-arith.bin`, `src.cpp`, `doc.md`),
geometry provably cannot act, so `Δ_ship` and `Δ_pure` are **expected to be exactly 0**. That is an
**exact-value expectation, stated in advance**, not a gate:

- **both 0** → the expected result; report as such;
- **either non-zero** → a **declared anomaly** that the writeup must explain. It is not an automatic
  abort, because the honest reading is diagnostic: it localises the leak (env drift, a pinned option
  that did not take, block-count bookkeeping) rather than condemning the run.

This preserves the validity check while removing the invented band. **G5 survives as a stated
expectation; it does not survive as a gate.**

---

# 13. FINAL CONSTRUCTIVE SPEC — Q2 as dispatchable

Supersedes §5's arm list and §11's dispatch table. Everything else stands.

## 13.1 Canonical argv (complete, no inherited defaults)

Common to **all** arms and configs — every option pinned, nothing left to a codec default:

```
--literal=o0 --entropy=rans --shape-states=28 --surprise=12
--chain=48 --max-match=65535
--boundary=off --channels=off --pnra=off --ariref=off
--stream-suite=on --stream-ctx=on --hotop-rlzp=off --hotop-budget=off
--decode-threads=1 --quiet
```

Per **config** (the three distinct λ values — §12.1):

| config | add |
|---|---|
| `anvil-mdl-rans` | `--parse=mdl --stream-lambda=0.04` |
| `anvil-mdl-rans-l0` | `--parse=mdl --stream-lambda=0.0` |
| `anvil-mdl-rans-l001` | `--parse=mdl --stream-lambda=0.01` |
| `anvil-shape-rans` | `--parse=shape --stream-lambda=0.04` |
| `anvil-shape-rans-l0` | `--parse=shape --stream-lambda=0.0` |

Per **arm** (the 2×2 — exactly two block values, `--negate` as the second factor):

| arm | add | role |
|---|---|---|
| **G0** | `--block=262144 --negate=on` | retained-twin **fidelity/reproduction** (the ruling's only predicate use) |
| **G1** | `--block=67108864 --negate=on` | **the parity arm** |
| **G2** | `--block=67108864 --negate=off` | closes the interaction |
| **G3** | `--block=262144 --negate=off` | closes the interaction |

Environment: **`ANVIL_STREAM_LAMBDA` must be asserted unset** (§12.1 precedence).

**Shape:** 5 configs × 4 arms = **20 rows/file × 13 files = 260** compress+round-trip calls.
Two block values, so **not a block-size sweep**. Round-trip verification on every row.

## 13.2 Outputs — exact values, no bands

| output | form |
|---|---|
| the five contrasts (§12.4) | **exact byte integers**, per file per config, no band |
| achieved block count | exact integer per (file, arm) |
| per-block bytes / ns | encoder and decoder, per-block, exact |
| timing | **descriptive**: ≥9 reps, rep 1 reported separately, per-rep rows, median **and** MAD, plus the noise-floor CV for that row |
| parallelism | **descriptive, non-classifying**: `decode_threads ∈ {1, N}` at both geometries, **ratio of the two stated per file** (§10.5) |
| `evidence_role` | `discovery` on every row |

**Forbidden:** any `FRONT-CROSSING`/`FRONT-GAP` emission or frontier reclassification (G8); defining
`eps_ratio`/`eps_speed` or any post-hoc ε; the token "citation-grade"; any `--parse=ratio` or BWT arm
(B1); any numeric band on a byte-axis value.

## 13.3 Checks — exact-value, not thresholds

| check | form | on failure |
|---|---|---|
| **G1** retained-twin reproduction | **byte-exact** equality to `benchmark-suite.frozen-bdc90474.plus-xz.csv`, per file | report build drift (3 commits post-`9caeee9`) as build drift; **do not** proceed to parity cells |
| **G5** control-file expectation | `Δ_ship` and `Δ_pure` **expected exactly 0** on the 4 single-block files | **declared anomaly to explain**; not an automatic abort (§12.4) |
| **G6** parallelism column present and flagged non-classifying | present / flagged | incomplete output |
| **G7** artifact pinning | SHA-256 for retained CSV, `parity_sweep.py`, `noise-floor.csv`, `block-oracle.csv`, E4 memo, Q2 binary | unmet precondition |
| **G8** arbiter refuses to classify Q2 rows | enforced in code | violation of the ruling |
| **G4** timing-arm admissibility | inherited **≥ 7 %**, **timing only**, declared **signed per arm** | timing arms dropped; **byte arms unaffected** |

**Only one numeric band exists anywhere in this job, it is inherited from
`FLEDGE-REVIEW:48`, and it never touches a byte-axis value.**

---

# 14. VERDICT ON THE CONSTRUCTIVE DRAFT

> # DISPATCH — after the §12/§13 corrections; not before
>
> **The §11 draft was not dispatchable.** It asserted "option literals verbatim" without recording
> them, and the literal set hides `--stream-lambda` — harness default **0.04**, codec default
> **0.01**, three distinct values across the five configs. Three configs would have failed the
> retained-twin byte check for a **settings** reason presenting as **build drift**, and one would
> have passed by coincidence. That is how a correct job dies of a wrong diagnosis.
>
> **With §13's argv recorded in full, every option pinned, byte contrasts demoted to exact values,
> and the single inherited ≥ 7 % band scoped to timing only, the draft is dispatchable.**
>
> **Verified against the three questions asked:**
>
> | question | answer |
> |---|---|
> | exact argv/settings for all five configs preserved? | **Yes — now.** §13.1, all 19 options + λ, verified against `bench_native.cpp:24-31`, `anvil.cpp:3769-3800`, `:4959-4982`. Found and fixed one byte-affecting trap. |
> | new materiality/ε threshold introduced? | **No.** One numeric band exists (`≥ 7 %`), it is **inherited verbatim** from `FLEDGE-REVIEW:48`, it is scoped to timing arms, and its original rationale is flagged as partly retracted (C2) and partly superseded by S1. **No replacement number invented; none may be.** |
> | deterministic byte contrasts exact values, not gates? | **Yes — now.** `ratio` CV = 0.000, so all five contrasts are exact integers with **no band**. G5 is demoted from STOP-gate to a stated exact-value expectation; a non-zero on a control file is a **declared anomaly to explain**, not an automatic abort. |
>
> **Unchanged and still binding:**
>
> - **B1** — Class-A token-lane only. Any `--parse=ratio` or BWT arm reports a ~4.5× backend win as a
>   geometry win and must be rejected.
> - **Scope (§7)** — no general parity claim (no corpus file exceeds 4 MiB, so the reference side is
>   never segmented); no block-size optimum from two points; no performance claim (hosted-runner
>   timing is ranking-grade).
> - **S1** — the parallelism ratio (§13.2) is the only channel that can surface the fact that parity
>   deletes `decode_threads` at any core count. If it is omitted or filed as descriptive, the fix
>   for the byte deficit becomes the performance deficit.
> - **§10.7** — the successor timing remeasurement still has no owner. The 5 `FRONT-GAP` cells remain
>   citation-grade unresolved, and the predicate/ε defects are deferred, not fixed.
> - **`evidence_role = discovery` on every row** — exactness is a property of the codec; validity is
>   a property of the corpus.