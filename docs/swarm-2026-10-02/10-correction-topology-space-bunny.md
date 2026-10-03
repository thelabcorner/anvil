# Track 10 — Correction Topology Coding (Space Bunny Free) — **REVISED after critic reconciliation**

**Agent:** Space Bunny Free (constructive inventor)
**Date:** 2026-10-02 · **Revision 2** (supersedes rev-1 interim; two correctness blockers fixed, scope narrowed)
**Worktree:** `i10-aux-unbwt`, intentionally dirty. No commit, no push, no reset/clean/stash/restore/rebase. No pre-existing file modified.
**Reconciles with:** `docs/swarm-2026-10-02/10-correction-topology-fledge.md` (Fledge Alpha, INTERIM).

**Recommendation: run ONE byte-only Step-0 oracle, then KILL or PILOT on its result.** No codec implementation is authorized before the oracle returns. The oracle is ~30 min of remote CI and measures the *ceiling* of the entire mask-replacement namespace, so a negative closes the namespace for both my lane and the critic's.

---

## 0. Rev-1 → rev-2 change log (explicit, so the record is auditable)

| # | Rev-1 claim | Blocker | Rev-2 status |
|---|---|---|---|
| C1 | §3.2 `T7b` = 3 bits per dirty RESID window | **Coordinator, correct.** An arbitrary non-empty subset of 4 bytes has **15** states; 3 bits codes 8. Rev-1's "which of the 4 bytes differ" was **not injective** — a real format bug, not a typo. | **Fixed.** Rev-2 replaces the ad-hoc T6/T7a/T7b triad with a **single 17-symbol per-window alphabet** (§3.2), proven bijective. Per-dirty-window cost rises from 4 bits to a symbol; corrected totals in §4. |
| C2 | §3.4 pseudocode applied patches *before* the copy | **Coordinator, correct.** `out[start+…]` writes precede `out` being extended by the copy; taken literally the copy then overwrites them. Rev-1's ordering was wrong. | **Fixed.** §3.5 is now a normative ordered decode with an index-validity invariant stated per step. |
| C3 | rev-1 projected "3–8% decode improvement" | **Self-correction from re-audit.** That number was hand-wavy and is **retracted**. Correct projection is decode-neutral-to-slightly-**negative** (§4.2). |
| C4 | rev-1 cited the mode-11-vs-mode-13 A/B as evidence that topology coding fails | **Critic, correct, and it inverts the meaning.** The source comment at `src/anvil.cpp:3363` states mode 13 uses "**Same token/dist coding as mode 12**". The published A/B ran mode 13 against **mode 11** — a different distance coder. The result is confounded and is **not** evidence about mask replacement either way. Retracted as evidence in §1. |
| C5 | rev-1 recommended HOLD→PILOT on a projection | **Coordinator.** Narrow to a byte-only ceiling oracle; KILL before implementation. | **Adopted.** §8 is now a single preregistered oracle. Implementation is explicitly *not* authorized. |

---

## 1. Evidence map

### 1.1 Measured facts (this project; cited to file and line)

| # | Fact | Source |
|---|---|---|
| E1 | Mode 11 flat-A mask cost is `4·⌈L/32⌉` raw bytes per type-2 token = **1 bit per covered byte, independent of k** | `src/anvil.cpp:2222-2229`; `FORMAT.md:868` |
| E2 | Type-2 tokens always have `k ≥ 1` (candidate requires `bk >= 1 && blen >= 8`), so **no zero-correction sparse token exists** | `src/anvil.cpp:642, 709` |
| E3 | Mode 14 has **8** streams; residual mask `4·⌈L/32⌉` B, transform mask `4·⌈(L/4)/32⌉` B, **separate by design** ("isolated statistical domains") | `FORMAT.md:494-504`; `src/anvil.cpp:2639-2640` |
| E4 | In the encoder, a window accepted as a transform field does `j += 3; continue` — so **a transform window can never also carry a residual correction**. Mutual exclusivity is structural, by construction | `src/anvil.cpp:594-604` |
| E5 | Transform detection requires `(j & 3)==0 && j+4 <= cap && j+4 <= dist`, i.e. **4-aligned windows inside the non-overlap region** | `src/anvil.cpp:595` |
| E6 | Mode 14 normative decode order is already **copy → transform → residual** | `src/anvil.cpp:2712, 2717-2735, 2751-2758` |
| E7 | A transform window's source value **is** the post-copy destination value (copy is exact), but the **target** value is unknown to the decoder — so XFORM-vs-RESID is **not** decoder-derivable and must be transmitted | derived from `src/anvil.cpp:2727-2728` |
| E8 | **Mode 13's own source comment: "Same token/dist coding as mode 12"** — yet the published A/B ran mode 13 vs **mode 11**. Distance coder differs between the compared arms ⇒ **the mode-11-vs-mode-13 result is confounded** | `src/anvil.cpp:3362-3371` vs `RESEARCH_LEDGER.md:594-599` |
| E9 | Mode 13 loses to mode 11 by +13.5% to +21.3%; modal accuracy 17–23.5% vs a Linux-line claim of 86.5% | `RESEARCH_LEDGER.md:594-617` |
| E10 | Masks ~90% unique; top-32 masks cover 7–12% of type-2 tokens | `RESEARCH_LEDGER.md:610-612` |
| E11 | SRR probe: mask recurrence **flat-to-down** on all 3 record files; `--channels=on` is itself a +0.48% to +2.23% ratio regression | `RESEARCH_LEDGER.md:1574-1627` |
| E12 | On `synth-timeseries.bin` (purpose-built, zero-jitter, **97.1%** top-32 mask coverage) mode 13 still loses by 6.5% | `RESEARCH_LEDGER.md:2279-2314` |
| E13 | Mode 11 on `generated.jsonl` = 222,409 B output from 2,815,267 B; −9.6% vs dp-rans at ~21× encode | `RESEARCH_LEDGER.md:128-139, 598` |
| E14 | `brotli q9` = 0.073 vs mode 11 = 0.079 on the same file ⇒ **~7.6% ratio gap** is the distance to close | `docs/CONTEXT.md` (SPARSE-REF anchor) |
| E15 | Frozen frontier: **0 FRONT-CROSSING**, 33 non-dominated, 5 FRONT-GAP, 28 DEGENERATE, 435/468 dominated. **GRID-THIN binding** | `RESEARCH_LEDGER.md:4711-4714` |
| E16 | Decode is the binding gap: mode 11 ~215 MB/s vs `brotli q9` ~900 MB/s on jsonl | `RESEARCH_LEDGER.md:136-150` |
| E17 | Per-substream DoS bound `max_sub = 16·out_len + 64`; **9 streams ⇒ 144·out_len** worst-case decode allocation | `src/anvil.cpp:2244, 3469` |
| E18 | Block default `256 KiB`; revision-1 max block 64 MiB | `src/anvil.cpp:3769, 5000` |
| E19 | Reps C/D (order-1 over masks; per-record mask delta) specified in the advisory, **never implemented** | `docs/mask-stream-advisory.md:23-25` |
| E20 | Gate ruling nominates this family as claimable position 3, with AUDIT-7 causality guard | `docs/gate-ruling-i9-pr5-position-derived.md:107-111, 84-93` |
| E21 | **Intra-project competing idea**: derive transform sites from the copied source span instead of transmitting a transform mask | `docs/I10-BREAKTHROUGH-PROGRAM.md:243-247` |

### 1.2 Retractions (evidence I withdraw)

| Withdrawn | Why |
|---|---|
| Rev-1 §1 rows E4–E6 (mode-13 losses) as evidence **against** mask replacement | **Confounded** (E8). The compared arms differ in distance coder. Per coordinator: not usable as positive evidence, and equally not usable as negative evidence. |
| The ledger's own root-cause narrative for mode 13 ("blocked on R4 / corpus lacks structure") | Not supported; E8 shows the A/B design is confounded. **I request the ledger correction** (Fledge §11) but do not make it myself. |
| Rev-1's ledger attribution that mode 11's repeat-control +2.2% is "flat-A mask cost when corrections are zero" | **Impossible** — E2 shows `k ≥ 1` always. The correct explanation is mask *granularity* (a `k=1` token over L=235 pays 32 B of mask to save 1 residual byte). This is Fledge §3's point and it is right. |
| Rev-1's "3–8% decode improvement" | Hand-wavy; re-audited to neutral-to-negative (§4.2). |

### 1.3 Projections (no measurement; gated by §8)

Only §4.1 and §4.2 are projections. Both are labelled inline and neither carries gate weight until the oracle lands.

---

## 2. Reconciliation with the critic — where I agree, where I do not

I read `10-correction-topology-fledge.md` before writing rev-2. Per-item:

| Critic point | My position | Concession |
|---|---|---|
| Mode-11-vs-mode-13 A/B is confounded (mode 12 distance coder, §1, §7.4) | **Agree, and it inverts rev-1.** Added as E8; rev-1's use of it is retracted (§1.2). | **Full.** |
| Arithmetic ceiling `p > h_e/H̄`, ceiling 2–5.5% < 7.6% gap (§4) | **Agree for mode 13's shape.** That inequality governs *per-correction modal residual* coding. My mechanism pays **per-window**, not per-correction, and has no exception bitmap. So §4.1 does **not** bound it — but this is a *scope* distinction, not a refutation of the critic's arithmetic. | **Full on mode 13; scope-noted on WCT.** |
| Novelty ≈ 0; RFC 7932 §7.1 context map subsumes decoder-visible context→cluster mapping (§5) | **Agree on the general principle.** WCT's 17-symbol window alphabet is **order-0** with no transmitted context map and no context ID, so it is not literally the RFC 7932 construct. **But see §6: I now concede the mechanism is very likely adopt-class**, because field-type/exception-pattern coding is standard in columnar codecs. | **Substantially.** |
| Decode direction is backwards — adds a 9th stream (§1 prong 3, §6) | **Agree on mode 13.** WCT as specified in §3 **replaces** stream 5 and absorbs stream 7, so the stream count is **7 for mode 11 and 7 for mode 14** — never a ninth. But decode is still *neutral-to-negative* (§4.2), so this does not rescue the decode leg. | **Full.** |
| "Nothing in the family attacks the dominant cost" — the mask is `L/8` B/token, k-independent (§6 headline) | **Agree, and this is why my mechanism exists.** Rev-2 is entirely about replacing that mask. | **Full.** |
| `mask_wire` has never been measured in isolation; its ceiling is unknown (§8-A, §11) | **Agree — and this is precisely why §8 is now a byte-only oracle.** | **Full.** |
| Only RFC 7932 was verified in-session; PAQ-match-model row unverified | I did not fetch any source this session. **I neither accept nor dispute the unverified rows.** | **N/A.** |

**Net:** the critic's §8-A (context-modelled mask coding) and my §3 (window-class alphabet) are **the same attack on the same cost** under different alphabets. The oracle in §8 measures **both** as candidate ceilings, so the lanes converge on one cheap decisive measurement instead of competing pilots. That is the useful outcome of this reconciliation.

---

## 3. Mechanism — WCT (Window-Class Topology), corrected

### 3.1 The structural fact that makes it coherent

From **E4**: in `scan_candidate`, when a 4-aligned window inside the non-overlap region satisfies `t32 == s32 - dist`, the encoder records a transform field, does `j += 3`, and `continue`s — so that window **never reaches the residual branch**. Transform fields and residual corrections are therefore **mutually exclusive by construction**, not by convention.

Therefore the two bitmaps mode 14 ships separately (**E3**) are projections of a **single per-window class**. That is the whole mechanism: stop paying two independent bitmaps for one underlying three-valued field.

### 3.2 Wire model — one symbol per 4-byte window (corrected; blocker C1 fixed)

For a sparse token of length `L` at distance `d`:

```
W  = L >> 2                        # full 4-byte windows
T  = L & 3                         # tail bytes, 0..3

Window symbol alphabet (17 values), emitted W times in increasing w:
    0        CLEAN                 # no residual bytes for this window
    1..15    RESID                 # residual submask: bit b set  =>  byte 4w+b differs
    16       XFORM                 # implicit Delta = -d, no residual bytes
Tail symbol alphabet (8 values), emitted once if T > 0:
    0        CLEAN
    1..7      RESID                 # bit b set => byte 4W+b differs
```

**Injectivity proof (closes blocker C1).** The map from "true window state" to "emitted symbol" is a bijection on the reachable set:

- *Forward.* Every window is in exactly one of: all 4 bytes equal ⇒ CLEAN ⇒ `0`; ≥1 byte differs and not a transform field ⇒ RESID with submask in `1..15`; transform field ⇒ `XFORM` ⇒ `16`. These three are disjoint and exhaustive (**E4** guarantees disjointness), so symbol `16` is never confused with `0`, and submask `0` is never emitted (it would be CLEAN).
- *Inverse.* Given the symbol stream plus `L`, `d`, and the copied bytes, the window state is recovered uniquely, and the number of residual bytes this token consumes is determined: `Σ_{w} popcount(sym_w)` for `sym_w < 16`, plus `popcount(tail)` if `T>0`.
- No symbol is unreachable-but-ambiguous. `sym=0` (submask 0) is excluded from the RESID range by construction, so the 16 submask symbols are exactly the non-empty subsets.

**Stream accounting — no ninth stream (constraint satisfied).**

| | mode 11 (7 streams) | mode 14 (8 streams) |
|---|---|---|
| before | types, ll, ml, ds, lits, **masks**, resid | types, ll, ml, ds, lits, masks, resid, **tmask** |
| after (WCT) | types, ll, ml, ds, lits, **wclass**, resid = **7** | types, ll, ml, ds, lits, resid, **wclass** = **7** |
| delta | **0 streams** (masks → wclass) | **−1 stream** (masks + tmask → wclass) |

`wclass` is one byte-per-symbol stream serialized by the existing per-stream suite (`encode_stream`, `src/anvil.cpp:1548`), which supports alphabets up to 256 (`src/anvil.cpp:1785`), so 17 is in range. The suite's transmitted model (≈17 entries, tens of bytes per block) is **charged** in §4.

**Why a symbol and not the rev-1 4-bit submask.** The 4-bit submask (rev-2 first fix, still valid) would need 4 bits/window plus a separate class bit plus a separate dirty bit — three coupled fields, which is strictly more information than the 17-symbol single field for the same states. One symbol also removes the rev-1 cross-field invariant risk entirely: mutual exclusivity becomes a property of the alphabet, not an agreement between two streams. Cost of this simplification: a symbol byte is not bit-packed, so a CLEAN-heavy block pays ~1 byte/window where a bit-packed form would pay ~0.1. §8's oracle measures exactly this trade and reports both.

### 3.3 Encoder (unchanged candidate discovery; serialization only)

No change to `parse_sparse`, `find_sparse`, `find_sparse_at`, or `scan_candidate` — the parse, distances, dead band, and surprise budget are **bit-identical to mode 11**, which is what makes the §8 oracle a clean substitution. Only `encode_tokens_*` changes:

1. For each type-2/3 token with recorded `len`, `off[]`, `val[]`, `tfo[]` (`src/anvil.cpp:463-470`):
   - `W = len>>2`, `T = len&3`.
   - Initialize `sym[0..W-1] = 0`, `tail = 0`.
   - For each `tfo` entry `w` (already validated `4w+4 <= dist` by the encoder): set `sym[w] = 16`.
   - For each `(off[k], val[k])`: `w = off[k]>>2`; **if `sym[w] == 16` this is a bug** — the encoder must reject its own token (defensive assert). Else `sym[w] |= 1<<(off[k]&3)`.
   - Tail: for `off[k] >= 4W`: `tail |= 1<<(off[k]-4W)`.
   - Emit `sym` bytes to `wclass`; emit `val[]` to `resid` in ascending `off` order (unchanged order).
2. Because `scan_candidate` guarantees a transform window absorbs all 4 of its bytes (`j += 3`), step 1 cannot produce a mixed XFORM/RESID window. **This is the invariant rev-2 makes explicit and the oracle asserts.**

### 3.4 Decoder state machine — normative ordered steps

Fixed-size per-token state only: one `uint8_t` symbol cursor, one `uint32_t` window accumulator, one 4-byte scratch. **No O(L) array** (rev-1's and mode 11's `words[]` arrays are not needed — symbols are consumed in one forward pass).

### 3.5 NORMATIVE DECODE ORDER (blocker C2 fixed)

The following order is **normative and total**. Steps 1–2 must complete before any `out[]` write. This mirrors the already-shipped mode-14 order (**E6**).

```
STEP 0  read L, d from S2, S3.
        REJECT if d == 0 || d > out.size()            [out not yet extended]
        REJECT if L > kSparseMaxLen (65536)           [src/anvil.cpp:453]
        REJECT if L > out_len - out.size()
        start := out.size()                            # capture BEFORE extending

STEP 1  PERIODIC COPY — extend output first.
        for k in [0, L):  out.push_back(out[out.size() - d])
        INVARIANT I1: out.size() == start + L.
                      All indices out[start .. start+L) are now valid to read AND write.

STEP 2  WINDOW SYMBOL SWEEP — w = 0 .. W-1, W = L>>2.
        for w in [0, W):
            sym := next byte from wclass
            REJECT if wclass exhausted                    "truncated window stream"
            if sym == 16:                                # XFORM
                REJECT if 4w + 4 > d                     "transform beyond non-overlap"
                v := u32le(out[start+4w .. start+4w+3])  # reads the COPIED source value
                v := v - u32(d)
                write u32le(v) to out[start+4w .. +3]
            elif sym >= 1:                               # RESID
                for b in [0,4) where bit b of sym:
                    REJECT if resid exhausted            "truncated residual stream"
                    out[start + 4w + b] := next byte from resid
            else: REJECT if sym != 0                     "unknown window symbol"
        INVARIANT I2: every write above is at index in [start, start+L).

STEP 3  TAIL SYMBOL — only if T := L & 3 > 0.
        tsym := next byte from wclass
        REJECT if wclass exhausted
        REJECT if tsym > 7
        for b in [0, T) where bit b of tsym:
            REJECT if resid exhausted
            out[start + 4W + b] := next byte from resid
        INVARIANT I3: every tail index is in [start+4W, start+L).

STEP 4  EXHAUSTION.
        REJECT if wclass has unconsumed bytes            "wclass trailing bytes"
        REJECT if resid  has unconsumed bytes            "substream consumption mismatch"

STEP 5  (per block, existing) all substreams consumed; CRC-32 over the block.
```

**Why the rev-1 order was wrong, stated precisely:** rev-1 wrote patches before the copy. Two independent failures: (a) `out[start+…]` is out of range because `out` has not been extended to `start+L`; (b) even if in range, **STEP 1's copy would then overwrite every patch**, because the copy writes all `L` bytes unconditionally. The transform read at STEP 2 is only sound *after* the copy, because the copy places the source bytes at the destination (**E7**).

### 3.6 Strictness and boundedness (re-audited)

| Property | Value |
|---|---|
| Max windows per token | `W = L>>2 ≤ 16384` |
| `wclass` bytes per token | `W + (T>0 ? 1 : 0) ≤ 16385` |
| Decoder scratch | **O(1)**: 1 symbol cursor + 1 u32 accumulator + 4 B scratch. Strictly **less** stack than mode 11's `std::array<uint32_t, 2048> words` (8 KB, `src/anvil.cpp:2275`). |
| Per-substream DoS bound | `wclass ≤ Σ(L_i>>2) + tokens ≤ out_len/4 + out_len/4 ≤ out_len/2` ⇒ inside the existing `max_sub = 16·out_len + 64` (**E17**). **Amplification does not regress**: WCT is 7 streams, not 9. |
| Decoder determinism | No data-dependent branching beyond the symbol value; fully deterministic. |
| Malformed-input behaviour | 10 distinct `throw` sites, all fail-closed. No UB: every write index is range-proved by I1–I3. |

---

## 4. Cost model (re-audited after the C1 fix)

### 4.1 Bytes per token — corrected

Let `L` = phrase length, `W = ⌊L/4⌋`, `T = L mod 4`, `k` = corrections, `m_w ∈ {0..4}` = differing bytes in window `w`, `x` = XFORM windows, `c` = CLEAN windows, `H_s` = entropy of the window-symbol distribution (bits/symbol, order-0).

| | topology bytes per token (nominal) |
|---|---|
| **mode 11 today** | `4·⌈L/32⌉` = **L/8** B (E1) |
| **mode 14 today** | `4·⌈L/32⌉ + 4·⌈W/32⌉` = **5L/32** B (E3) |
| **WCT nominal** | `W + (T>0)` bytes, order-0 coded to `W·H_s/8` bytes + tail + transmitted model |
| **WCT ideal (H_s→0)** | `≈ 0` — this is the ceiling the oracle measures |

Worked **projection** at `L = 235` (jsonl-like), `W = 58`, corrections clustered so ~10% of windows are dirty:

- today (mode 11): `4·⌈235/32⌉` = `4·8` = **32 B/token**.
- WCT: `58·H_s/8 + 1` B. With `H_s ≈ 0.87` bits (90% CLEAN, 10% spread over 16 symbols): `58·0.87/8 + 1` ≈ **7.3 B/token**.
- projected nominal saving ≈ **24.7 B/token**, i.e. **77% of the mask stream**.

**This is a projection and it is not the gate.** The real number depends on (a) the actual `H_s`, (b) how much the existing order-0 mask coder already compresses (`mask_wire` ≠ `mask_raw`), and (c) how many type-2 tokens exist. All three are **unmeasured**; §8 measures them. Note the arithmetic sanity check the oracle must respect: total mask bytes cannot exceed total output, so either type-2 token count or mean `L` must be far smaller than "one token per record" — rev-1 did not check this and I am flagging it as a known error in the projection.

### 4.2 Decode cycles — corrected and *worse* than rev-1 claimed

Per token at `L = 235`, `k ≈ 35`:

| work item | mode 11 | WCT |
|---|---|---|
| entropy-decode output materialized | `4·⌈235/32⌉` = **32 B** | `W + 1` = **59 B** symbols (**+84%**) |
| word loads / popcounts | 8 | 0 |
| symbol loads + branches | 0 | **59** (**new**) |
| `countr_zero` residual stores | ~35 | ~35 (unchanged) |
| copy loop | 235 | 235 (unchanged, dominates) |

**Corrected projection: decode-neutral to ~+3–6% slower on sparse-heavy files.** The materialized byte count goes *up* (59 vs 32) because a 17-symbol alphabet cannot be bit-packed into 4 bits. The offsetting saving is that WCT drops 8 word loads and 8 popcounts and — importantly — **removes mode 14's entire transform-mask stream and its 8-byte materialized array**.

Rev-1's "3–8% decode improvement" is **retracted**. WCT does not rescue the decode leg, and per **E16** nothing in this namespace will: mode 11 is already ~4× behind `brotli q9` on decode, and the binding constraint is the rANS/postcoder path, not mask scan cost. **Any candidate from this lane is ratio-only and therefore a doctrine-12 rejection candidate regardless of the oracle result.** The oracle is therefore scoped to decide *ratio headroom*, and §8.3 carries an explicit standing warning that a positive oracle does **not** produce a Pareto crossing.

### 4.3 Encoder cost / code size

~120 lines prototype, isolated at `prototypes/swarm-2026-10-02/10-correction-topology/space-bunny/`, `src/anvil.cpp` untouched. No new candidate generation, no new verification (contrast PNRA, which needed both). Encoder throughput impact: unmeasurable at rev-2 (no implementation authorized); the only serialization-side delta is building `sym[]` from already-materialized `off[]`/`tfo[]`, i.e. O(k) per token.

### 4.4 Memory / RSS

Decoder RSS: **net neutral to slightly better** — one materialized stream (`wclass`, `≤ out_len/2` B) replaces one (`masks`, `≤ out_len/8` B) in mode 11; in mode 14 it replaces two. Stack scratch drops from 8 KB to O(1) (§3.6). Peak RSS is dominated by the output buffer and block framing, unchanged.

---

## 5. Asymptotics

With `ρ = ` fraction of non-clean windows, `H_s(ρ)` = entropy of the window symbol distribution, `W ≈ L/4`:

| | topology bytes over `n_T` covered bytes |
|---|---|
| mode 11 | `Θ(n_T/8)` — **no dependence on `k` or `ρ`** (E1) |
| WCT nominal | `Θ(n_T/4)` symbols → order-0 coded `Θ(H_s(ρ)·n_T/4)` bytes |
| WCT best case `ρ→0` | `Θ(H_s·n_T/4)` with `H_s→0` ⇒ **sublinear in corrections** |
| WCT worst case `ρ→1` | `H_s→log₂17 ≈ 4.09` ⇒ `Θ(1.02·n_T)` — **≈ 8× worse than mode 11** |

**The worst case is severe and it is the honest headline.** Mode 11's `1 bit/byte` is a hard floor; WCT's per-window symbol can exceed it by up to 4× when every window is dirty. WCT wins only when `H_s(ρ) < 2·ρ·log₂17`, i.e. when the window-symbol distribution is skewed. §8's oracle measures `H_s` and `ρ` directly and the **gate is written against this inequality, not against a hand-picked percentage**.

---

## 6. Novelty / prior-art risk — concession

I now concede that **WCT is very likely adopt-class engineering, not mechanism novelty**, for reasons stronger than my rev-1 listed:

1. **The critic's RFC 7932 §7.1 point stands for the general family** — any decoder-visible context→cluster mapping is adopted art in this project (`RESEARCH_LEDGER.md:630-639`). WCT's order-0 17-symbol alphabet is *narrower* than that, but it is not a new kind of object.
2. **Columnar codecs already do exactly this.** ALP encodes per-value type classes (exponent/NaN/mantissa) plus a per-column exception pattern; FastLanes/LeCo/Parity/ParitySE encode field-type or scheme-selection patterns followed by packed values. **Encoding "what kind of thing is at this position" then packing the payload is the defining pattern of that literature.** WCT applies it to correction positions inside an LZ reference. That is novelty-by-difference, which `MASTER-BRIEF.md:9` rejects.
3. **Rev-1's "mutual exclusivity is the novelty" argument is weak.** Mutual exclusivity (**E4**) is a property of ANVIL's greedy scan, not a mathematical necessity — a better parser could legitimately want a window that is both a transform field and has a residual correction. **The mechanism's defining property is therefore an artifact of the current encoder, which is the definition of a fragile claim.**
4. **E21 collision unresolved.** `docs/I10-BREAKTHROUGH-PROGRAM.md:243-247` proposes deriving transform sites instead of transmitting them. If that lands, the XFORM symbol becomes redundant and WCT degenerates to "4-bit submask coding", i.e. plain sparse-bitvector coding (Elias/Fano/Waldvogel — 1970s art).
5. **I ran no prior-art search this session.** The novelty status above is an argument, not a clearance. Track 20 owns the decisive search; it should search *"per-position type-class + packed payload inside an LZ reference correction stream"*, not "relocation-aware compression" (already 6 passes deep, `docs/priorart-tcopy-external.md`).

**Position:** novelty is **not** a reason to build or not to build this. It is a reason the *report* must not claim more than "adopt-class representation change with a measured ceiling". The oracle in §8 is worth running **precisely because it also measures the critic's §8-A ceiling** — if the whole mask-replacement namespace is small, the novelty question becomes moot and the correct verdict is KILL.

---

## 7. Minimum prototype — NOT authorized at rev-2

Rev-1 proposed a codec prototype. **Rev-2 withdraws it.** No implementation proceeds until §8 returns.

| Stage | Status |
|---|---|
| Stage 0 — byte-only topology oracle | **The only authorized work.** §8. Not a codec; an offline analysis of captured token streams. |
| Stage 1 — WCT block mode | **Blocked.** Authorized only by an `S0-PASS`. |
| Stage 2 — remote Pareto | **Blocked.** Authorized only by an `S1-GO`. |

Scratch/artifact root for the remote run: `scratch/swarm-2026-10-02/10-correction-topology/space-bunny/`. No external temp paths.

---

## 8. THE DECISIVE EXPERIMENT — byte-only replacement-mask ceiling oracle (preregistered)

**Name:** `T10-MASK-CEILING` (superset of the critic's `T10-MASK-ANATOMY`, adopts its §9 instrumentation).

**Scope discipline, per coordinator:**
- **Byte-only.** No throughput measurement, no timing claim, no decode measurement, no Pareto run.
- **No codec change.** `src/anvil.cpp` untouched. The oracle is an offline analyzer over token streams produced by the existing encoder path, behind a standalone prototype binary.
- **No ninth entropy stream.** The oracle evaluates candidate alphabets **offline**; nothing is wired into any block mode.
- **No distance-coding change.** Token stream, distances, and `parse_sparse` are held **bit-identical** to the baseline, so every delta is attributable to mask representation alone.
- **GitHub Actions only**, `workflow_dispatch`, per `docs/github-actions-benchmark-protocol.md`: one checkout, both arms in one job, artifacts uploaded under `if: always()`, no commit/push, `permissions: contents: read`, finite timeout, concurrency group.

### 8.1 Measured quantities (one instrumented run)

| # | Quantity | Symbol | Why it is decisive |
|---|---|---|---|
| Q1 | actual encoded `masks` stream size (+ mode-14 `tmask`) | `mask_wire` | **The gate denominator.** Never measured in this repo (critic §11). |
| Q2 | `Σ 4·⌈L_i/32⌉` | `mask_raw` | Nominal floor; isolates how much the order-0 coder already achieves. |
| Q3 | `Σ L_i` over type-2/3 tokens, token count, mean/median `L` | `cover`, `n_tok`, `L_stats` | Rev-1's projection failed to check `mask_raw ≤ total`; this closes that error. |
| Q4 | `Σ k_i`; `m_w` histogram | `n_corr`, `m_hist` | Feeds the §5 break-even inequality. |
| Q5 | `ρ = non-clean windows / W`; **order-0 and order-1 entropy of the 17-symbol window alphabet** | `rho`, `H0_s`, `H1_s` | **The ceiling.** `H0_s` is WCT's order-0 cost; `H1_s` bounds any order-1 variant. |
| Q6 | **Unbounded-context (static-dictionary) entropy of the window-symbol stream** | `H∞_s` | **Absolute information-theoretic ceiling.** No mechanism beats `H∞_s` without transmitting side information. |
| Q7 | **Bit-level mask entropy** under order-0 and order-1-on-previous-bit | `H0_m`, `H1_m` | **The critic's §8-A ceiling**, measured in the same run. |
| Q8 | **Elias/Fano position-list entropy** of the correction set | `H_pos` | Second ceiling for a non-WCT replacement. |
| Q9 | `H̄` residual-byte entropy; modal accuracy `p` | `H_resid`, `p_ctx` | Feeds the critic's §4.1 inequality and closes the ledger's confounded A/B question. |
| Q10 | XFORM window count | `x_tot` | Quantifies the E21 collision exposure. |

### 8.2 Derived ceilings (byte-only, no new stream)

```
Ceiling_WCT(order-0)  = Q2 − Q3_cover · H0_s / 8
Ceiling_WCT(order-1)  = Q2 − Q3_cover · H1_s / 8
Ceiling_ANY           = Q2 − Q3_cover · H∞_s / 8      <-- dominates; no mask code can beat this
Ceiling_criticA       = Q2 − Q3_cover · H1_m / 8
Ceiling_positionlist  = Q2 − Q3_cover · H_pos / 8
best_ceiling          = max( of the four )            <-- the number that decides the lane
```

`best_ceiling` is reported as an **exact byte count** and as a **percentage of total compressed bytes**, per file, with no model-transmission cost and no decoder cost — i.e. it is a strict **upper bound** on what any implementation could achieve. It is therefore the correct quantity for a KILL/PILOT decision: if the upper bound fails, no implementation can pass.

### 8.3 PREREGISTERED GATES — fixed now, before any data

Let `record files` = `generated.jsonl`, `generated.log`, `generated.json`, `generated.sqlite`; `synthetic` = `synth-timeseries.bin`, `synth-jitter.bin`, `synth-ndjson-columnar.ndjson`. All are `discovery`/`known_stress` per `docs/I10-CORPUS-LOCK-PROTOCOL.md:62-64`. **No held-out data is opened at this stage** — a held-out confirmation is authorized only by an `S0-PASS`, and is then required by the firewall's §1.7/§1.8.

**KILL the entire mask-replacement namespace (this lane *and* critic §8-A) if ANY fires:**

| Gate | Condition | Rationale |
|---|---|---|
| **K1** | `mask_wire / total_output < 10%` on **≥3 of 4** record files | Adopted verbatim from critic §9. A budget under 10% cannot produce a frontier move against a 7.6% gap (E14). |
| **K2** | `best_ceiling / total_output < 2.0%` on **≥3 of 4** record files | The strict upper bound — all four candidate codes pooled, no cost charged — fails. **Nothing downstream can recover it.** |
| **K3** | median `ρ > 0.60` on **≥3 of 4** record files | §5 worst case: WCT is up to ~8× worse than mode 11 and the parser cannot dodge it without a measured-cost arm that rev-2 does not build. |
| **K4** | `H_resid ≤ 6.0 bits` **and** `p_ctx ≤ 0.25` in **every** context tested | Critic §9 rule, adopted verbatim: closes the modal-residual branch on the project's own measured operating point. |

**PILOT authorized (a genuine reopen) only if ALL hold:**

| Gate | Condition |
|---|---|
| **P1** | `mask_wire / total_output ≥ 15%` on **≥3 of 4** record files |
| **P2** | `best_ceiling / total_output ≥ 4.0%` on **≥3 of 4** record files — i.e. the ceiling is large enough that even 25% realization clears 1.0% end-to-end |
| **P3** | `ρ ≤ 0.40` median on **≥3 of 4** record files (the skew regime where §5 says WCT can win) |
| **P4** | the ceiling is **not** concentrated in a single file (≥3 of 4 individually pass P2) |
| **P5** | a byte-identical-token-stream control reproduces baseline total output exactly (CV 0.000%), proving the substitution is clean |

**HOLD (extend the corpus, no implementation) if:** K1–K4 all clear, `best_ceiling` lands in **[2.0%, 4.0%)** on ≥3 of 4 files.

**Standing warning attached to every outcome, to prevent over-reading:**
> A positive oracle establishes **ratio headroom only**. Per **E15** the frozen frontier is 0 FRONT-CROSSING and per **E16** this namespace is decode-neutral-to-negative (§4.2). Per `MASTER-BRIEF.md:12,20` a ratio-only result is **not** a frontier advance. Even `S0-PASS` authorizes a PILOT of an **adopt-class representation change** (§6), never a crossing claim.

### 8.4 Adversarial failure cases

1. **Oracle is unfalsifiable-safe.** `H∞_s` is an *upper bound* on achievable coding, so a KILL is sound. A PILOT is only as good as `H0_s ≈ H∞_s`; the oracle must therefore also report the **gap** `H∞_s − H0_s`. A large gap means the ceiling is real but unreachable order-0, and that is a **HOLD**, not a PILOT. *(New at rev-2 — this was a hole in rev-1.)*
2. **Type-2 token count surprise.** If `n_tok` is far below "one per record", `mask_wire` will be small and K1 fires. This is the most likely single outcome.
3. **Order-0 coder already strong on the mask.** Mode 11's mask stream is mostly zeros; `encode_stream` may already compress it near `H∞`. Then `Ceiling_criticA ≈ 0` and the whole namespace closes even if `mask_wire` is 15%. This is the honest reason K1 alone is not sufficient — hence K2.
4. **`ρ` bimodality.** Structured files may be strongly bimodal (mostly-clean records + a few dense ones). A single median hides this; the oracle must report the full distribution, not just the median, and the gate uses median only as a coarse screen with the distribution reported alongside.
5. **Corpus substitution.** `generated.*` is 2026-vintage synthetic. If the ceiling is large only there, the result is in-sample and worthless. The oracle therefore also reports the `synth-*` family separately, and a PILOT requires the ceiling to appear in **both** families.
6. **Prior-art kill survives a positive oracle.** Even on P1–P5 pass, §6's adopt-class concession stands. The PILOT must be labelled `{engineering}` per `docs/gate-ruling-i9-pr5-position-derived.md:120-138` conditions C1–C8, and no novelty claim may be attached.
7. **Ledger corruption risk.** I am not editing `RESEARCH_LEDGER.md`. The E8 confound and the E2-impossibility finding are recorded here and must be reconciled by the coordinator or by `research`, not by this lane.

---

## 9. Final recommendation

### **HOLD the mechanism. Authorize ONE byte-only Step-0 oracle (`T10-MASK-CEILING`), then KILL or PILOT on its result.**

**Why not KILL now:** the critic's KILL is correctly scoped to mode 13's shape (transmitted modal table + exception bitmap on a separate stream), and my lane's proposed mechanism is outside that scope — it *replaces* stream 5 rather than adding a ninth (**E17**, §3.2). But I cannot distinguish "correctly scoped to mode 13" from "correctly scoped to the whole neighbourhood" without one measurement, and that measurement (`mask_wire`) has **never been taken** in this project. KILLing the namespace on an unmeasured budget would be exactly the kind of unverified cumulative the project has been burned by before (`docs/i8-streak-correction.md:106-116`).

**Why not PILOT now:** three independent negatives already exist for the *mask-topology* idea (E9, E10, E11), and my own §5 worst case is an ~8× regression. Building a block mode before knowing the ceiling spends the most expensive resource (implementation + remote rigour) on a quantity that is currently a guess.

**Why the oracle is the right next action:** it is byte-only, needs no codec change, costs ~30 min of remote CI, is scoped to kill the **critic's** §8-A as well as mine, and its central quantity `best_ceiling` is a strict upper bound — so a KILL from it is sound and a PASS from it is honest about its own limits (§8.4.6).

**Specific asks of the coordinator:**

1. **Dispatch `T10-MASK-CEILING` on GitHub Actions** per §8. Artifacts to `scratch/swarm-2026-10-02/10-correction-topology/space-bunny/`. No local run.
2. **Reconcile the ledger root-cause for mode 13** — from "blocked on R4 / corpus lacks structure" to "confounded A/B (E8: mode 13 shares mode 12's distance coder but was compared against mode 11) + arithmetic ceiling + stream fragmentation". Requested by both lanes; **not performed by me**.
3. **Correct the mode-11 repeat-control attribution** — the "mask cost when corrections are zero" explanation is impossible (E2); the real driver is mask granularity. Also not performed by me.
4. **Route the §6 novelty question to track 20** with the corrected search phrase (*"per-position type-class + packed payload inside an LZ correction stream"*), since the TCOPY/relocation searches are already 6 passes deep and would not surface this.

**Decision rule I am bound by:** K1–K4 → **KILL** the namespace. P1–P5 → **PILOT** an adopt-class representation change, still ratio-only, still not a crossing claim. In-between → **HOLD** with no implementation.

---

## 10. Claim hygiene

- §1.1 rows are measured and cited to file and line. §1.2 lists four retractions by name.
- §4.1 and §4.2 are labelled projections; §4.2 is a **corrected** projection whose direction changed from rev-1.
- §5 states an ~8× worst case. §6 concedes adopt-class status. §8.3 attaches a standing warning that a positive oracle is not a crossing.
- §9's recommendation is conditional on a preregistered gate fixed **before** any data, per `MASTER-BRIEF.md:14`.
- No pre-existing file was modified. No commit, no push, no local benchmark, no corpus run, no network fetch.