# Track 19 — Format/Security/Fuzz — Fledge Alpha Free (INDEPENDENT ADVERSARIAL REVIEW)

**Agent:** Fledge Alpha Free · **Track:** 19 (format/security/fuzz) · **Date:** 2026-10-02
**Status:** FINAL for this pass. Written from an independent reconstruction of `src/anvil.cpp`,
`FORMAT.md`, and the paired constructive report; **no Space Bunny claim is taken on trust.**
**Scope discipline:** this file only. No existing file modified. No commit/push/reset/clean/
stash/restore/rebase. No local corpus benchmark, no heavy local fuzzing, no production-source edit.

**Reading basis (provenance).** `src/anvil.cpp` at live worktree `i10-aux-unbwt` @ `b8eae11`
(dirty, intentionally); `FORMAT.md` (965 lines); `docs/swarm-2026-10-02/MASTER-BRIEF.md`;
`docs/CONTEXT.md`; `docs/swarm-2026-10-02/19-format-security-space-bunny.md` (read for
reconciliation only, §6/§8); `docs/swarm-2026-10-02/11-orbit-programs-space-bunny.md`;
`docs/swarm-2026-10-02/10-correction-topology-space-bunny.md`.
Line numbers are from the live tree at review time and **must be re-pinned** if `src/anvil.cpp`
moves (`FORMAT.md:907-910` already mandates that discipline).

---

## 0. Verdict up front

> **HOLD** — and specifically: **HOLD the format-security *mechanism* program; do not build
> VCA yet.** The paired constructive report's *diagnosis* is correct and important; its
> *mechanism* is mis-prioritized. The cheapest decisive next action is a **byte-only,
> single-job remote probe of the declared-`total` slack**, not a decoder redesign.

Reasoning in one line each:

* The strongest security defect I can independently confirm in the existing format is
  **real but LOW severity and cheap to fix** (zigzag, §3) — it must not drive priority.
* The genuine *architectural* hazard is the **file-level declared-`total` proxy bound**
  (`src/anvil.cpp:4855`), which admits a ~**585 GiB** decode from a **64 KiB** file
  (§2.1). That is the finding worth acting on.
* But VCA (§6) is a **decoder redesign for a corpus-integrity problem**, and the
  demonstration that it is *needed* has not been produced. One remote job settles that.
* Both cross-lane mechanisms I audited (FLI, WCT) carry **format-security blockers that must
  be cleared before either is built**, and in FLI's case one of them invalidates the
  mechanism's headline claim (§4, §5).

---

## 1. Independent reconstruction: what the decoder actually guarantees

I verified the following directly in `src/anvil.cpp`, not from the audit narrative.

### 1.1 The frame (verified)

```
src/anvil.cpp:4846  magic "ANV0" + revision ∈ {1,2} else throw
src/anvil.cpp:4849-4851  block_size uvarint; reject 0 or > max_block
src/anvil.cpp:4850      max_block = 64 MiB (rev1) / 128 MiB (rev2)
src/anvil.cpp:4852      total uvarint; reject > SIZE_MAX
src/anvil.cpp:4855      total <= (in.size()/7 + 2) * max_block      <-- the only file-level bound
src/anvil.cpp:4859-4860 reserve_hint = min(total, max(1 MiB, in.size()*256))
src/anvil.cpp:4870      blen: reject 0, reject > block_size
src/anvil.cpp:4873      plen: reject plen > e-p
src/anvil.cpp:4874      reject blen > total - out.size()
src/anvil.cpp:4882/4888 CRC-32 per block, always
src/anvil.cpp:4928      out.size() == total
src/anvil.cpp:4929      p == e   (trailing bytes rejected)
```

**Verified strengths.** Output length is pinned by `total`; every block is CRC-checked
*before* it is retained; trailing bytes are rejected; there is **no** unbounded output
allocation independent of `total`. The `reserve_hint` clamp at `:4859` genuinely prevents a
crafted 5-byte header from forcing an unbounded `reserve`. These are real and should be
credited to the constructive lane rather than re-litigated.

**Verified weakness.** `:4855` is a **byte-count proxy for work**, and it is the *only*
file-level bound. There is no caller-suppliable ceiling: `Options` exposes `decode_threads`
only. This is the constructive report's §1.1 and I independently confirm it.

### 1.2 Per-mode bounds (verified)

| bound | location | status |
|---|---|---|
| `kSparseMaxLen = 65536` per match | `src/anvil.cpp:453`, enforced `:2272,2897,2939` | strong |
| substream `raw_n <= 16*out_len + 64` | `:2902` (mode 15), and per-mode equivalents | strong, but see §4.2 category error |
| mode-15 book `len == 0 \|\| len > kSparseMaxLen` rejected | `:2897` | **load-bearing, see §4.1** |
| mode-15 book `kind > 1` rejected | `:2893` | strong |
| mode-15 exactly 9 substreams + `p != e` | `:2901,2911` | strong, **but brittle, see §4.4** |
| RePair `R <= 768`, acyclic, cumulative `<= 2N + 64 KiB` | `FORMAT.md:706-712` | strong |
| RLZ addend `>= 0xFFFFFFFF` rejected **before** increment | `FORMAT.md:726-733` | strong |
| nesting depth 1 for codecs 7/8 | `FORMAT.md:753-756` | strong |

The hardening idiom the codebase already uses is **pre-increment rejection**
(`if (dv >= 0xFFFFFFFFull) throw …` at `:2945`, `:3212`, `:3303`, `:3497`). Remember this
idiom: §3 and §4.2 both measure new code against it.

---

## 2. Threat models (nine classes, constructed and adjudicated)

Legend: **REACH** = reachable from a crafted wire file; **SEV** = severity *to this project*.

### 2.1 Allocation bomb / declared-total slack — **REACH, HIGH**

`(in.size()/7 + 2) * max_block`. For a **65,536-byte** file, rev-1:
`9,364 × 67,108,864 = 6.28e11 B ≈ 585 GiB`. For 1 MiB: ≈ 8.7 TiB.

The bound is *self-consistent*: each block header costs ≥7 bytes, and each block may legally
emit up to `block_size`. So no well-formed encoder output can exceed it — which is exactly
why it is **not a bug** and exactly why it is **useless as a DoS bound**: the attacker pays
proportionally. Reachability requires only that the attacker supply block payloads that
decode cheaply to 64 MiB each (mode 11/12/13/15 with a short token substream), which is
format-legal.

Two honest qualifications the constructive report also makes and I confirm:
1. This is **the standard decompression-bomb shape**, covered by prior art (§7). It is a
   *missing caller-side ceiling*, not a novel vulnerability.
2. **Memory** is bounded by `total` and grows geometrically (transient ≈1.5× during
   realloc); the **threaded** path doubles it (`:4907` then `:4924-4925`). **Work** is the
   real exposure, and work is exactly what nothing bounds.

### 2.2 Integer overflow / wrap — **REACH, LOW (one site), see §3**

The codebase is disciplined: match lengths use the **subtraction** form
`len > out_len - out.size()` (`:2925,2934,2939`), which cannot wrap. Distance adds are
guarded pre-increment. The one exception is the zigzag family (§3).

### 2.3 Cyclic reference — **NOT APPLICABLE, structurally**

ANVIL has **no cyclic-reference construct**. Every copy is `dist ∈ [1, out.size()]`
(`:2925,2949,3225,3316,3501`) — i.e. the source is strictly *already-decoded* history, so
forward/self references are unrepresentable. Periodic/overlapping copies are bounded by
`len ≤ out_len - out.size()`. RLZ self-reference is intra-stream with the same rule
(`FORMAT.md:726-733`). **There is no cyclic-reference class to defend against.** Any future
mechanism that admits a forward or mutual reference (a bidirectional/delta codec, a
two-pass diff) would *create* this class for the first time and must carry a new audit.

### 2.4 Oversized table — **REACH, LOW**

The only decoder-allocated tables are ctx-rANS (`K × 4096`, K ≤ 12 per `FORMAT.md:623`, so
≈ 192 KiB, not the 1 MiB the constructive report quotes at K=256 — **I correct that figure**)
and the 12-bit Huffman table (`4096 × 2`). Both are compile-time-capped. Grammar rules ≤768.
**No attacker-sized table exists.** Note this is a *positive* property worth stating.

### 2.5 Malformed varint — **REACH, LOW–MEDIUM**

`get_uvar` (`:292-296`) and `read_varint_bytes` (`:2156`) both:
* accept **non-canonical** encodings (`80 00` decodes to 0), and
* **silently truncate high bits** on the 10th byte (`<< 63`, bits above bit 63 discarded).

Neither is memory-unsafe — every consumer range-checks afterwards — but together with §3
they are the format's soft underbelly. **Canonical-form enforcement is the single cheapest
hardening win available** (§8, T2).

### 2.6 Resource exhaustion (CPU) — **REACH, MEDIUM–HIGH**

The RLZ inner copy is a **byte-at-a-time `push_back` with a per-byte capacity check**
(`:1885`): not vectorizable, ~2–4 cycles/byte. Combined with §2.1 this is the credible
work-amplification path. The constructive report's ~23 s @ 3 GHz for 21 GiB is a **derived
projection**, correctly labelled as such at `:198-199`. I accept the derivation and its
label; I do not accept it as measured.

### 2.7 Truncation — **REACH, CLEAN**

Every read is bounds-checked (`get_uvar` throws on `p>=e`; each substream checks `zn`; each
block checks `plen`). Truncation is the best-covered class in this decoder.

### 2.8 Trailing garbage — **REACH, CLEAN**

Verified: `:2911` (mode-15 payload), `:2908` (per-substream), `:4929` (whole file), plus
per-postcoder and per-transform checks in `FORMAT.md:293-296,353,262-267`. Strong.

### 2.9 Corruption propagation — **NOT A SAFETY ISSUE; A MEASUREMENT ISSUE**

Blast radius today is *the whole artifact*: any bad block throws, nothing partial is emitted
(`FORMAT.md:964-965`). That is memory-safe and arguably the **correct** default. The real
cost is scientific: one corrupt byte inside a 50 MB corpus file destroys an entire CI gate
run, which is then classified `BLOCKED_INFRA` rather than reported. I **partially agree**
with the constructive report's `--on-corrupt=skip|resync` (§1.4 there) and with its hard rule
*never emit bytes for an unverified region* — but I rank this **below** §2.1 and treat it as a
harness concern, not a format mechanism.

---

## 3. EXISTING-FORMAT FINDING E-1 — zigzag varint wrap (**LOW severity**)

Kept deliberately separate from all cross-lane mechanism work, per coordinator instruction.

### 3.1 The sites

Unsafe form `(zz & 1) ? -int64_t((zz + 1) >> 1) : int64_t(zz >> 1)`:

| line | context |
|---|---|
| `src/anvil.cpp:2947` | mode 15 HOTOP, macro `flag==2` distance delta |
| `src/anvil.cpp:3069` | mode 15 fused variant, same |
| `src/anvil.cpp:3220` | mode 12 SHAPE, `flag==2` |
| `src/anvil.cpp:3311` | mode 12 fused variant |
| `src/anvil.cpp:3499` | mode 13 TOPOLOGY, `flag==2` |

**Safe counterpart, already in-tree:** `src/anvil.cpp:2538` (mode 16 ARI-REF) uses
`(z&1) ? -int64_t(z>>1)-1 : int64_t(z>>1)` — wrap-free. The correct formula already exists in
this codebase, one mode away. That is the strongest argument that E-1 is a defect and not a
design choice.

### 3.2 Reachability — **REACH on crafted wire only**

Path to a site: a revision-1 file, mode 12/13/15, a type-1/type-2 match token, distance
`flag==2`, **and** `last[shape] != 0` (set by a prior `flag==0`/`flag==1`, else
`"… delta before first absolute"`). `zz` comes from `read_varint_bytes`, which can express
`0xFFFFFFFFFFFFFFFF`: nine `0xFF` bytes (shift 0…56 → 63 bits) plus a final `0x01`
(shift 63). Modes 12/13/15 are reachable on the current format via `--parse=shape`,
`--parse=topology`, `--parse=hotop`; they are **not** the default path (default rev-2
`--parse=ratio`, `FORMAT.md:20-21`). So: reachable, but only via non-default encoder flags on
revision 1.

### 3.3 Resulting values — three distinct outcomes

1. **`zz = 0xFFFFFFFFFFFFFFFF` (odd).** `zz + 1` wraps unsigned (well-defined) to `0`;
   `(0)>>1 = 0`; `dlt = 0`. **Aliased with `zz = 0`.** ⇒ *non-injective decoding*.
2. **`zz = 0xFFFFFFFFFFFFFFFE` (even).** `zz>>1 = 0x7FFFFFFFFFFFFFFF = INT64_MAX` (in range,
   conversion defined). `dd = int64_t(lastd) + INT64_MAX` with `lastd ≥ 1` ⇒ **signed
   integer overflow — formal UB.**
3. **`zz = 0xFFFFFFFFFFFFFFFD` (odd).** `(zz+1)>>1 = INT64_MAX`; `dlt = −INT64_MAX`;
   `dd = lastd − (2^63−1)` ⇒ **signed underflow — formal UB.**

### 3.4 Do downstream checks fail closed? — **YES for memory safety; NO for correctness**

Every site is followed by the identical guard triple:

```
if (dd <= 0 || dd > 0xFFFFFFFFll) throw …   // :2947, :3222, :3313, :3499 (inline)
dist = static_cast<uint32_t>(dd);
if (dist == 0 || dist > out.size()) throw … // :2949, :3225, :3316, :3501
```

Therefore **`dist ∈ [1, out.size()]` always holds at the point of use**. There is **no OOB
read, no OOB write, and no unbounded work** from this defect. Where the overflow wraps to a
small positive value (outcome 1), the decoder *accepts* the token and produces bytes the
encoder never intended — which is then caught by the **mandatory per-block CRC-32**
(`:4882`/`:4888`), yielding a clean nonzero exit. Outcomes 2/3 wrap negative and are rejected
by `dd <= 0`.

### 3.5 Severity adjudication — **LOW**, with three distinct labels

* **Not** memory-unsafe. **Not** a DoS. **Not** remotely exploitable beyond one wasted block.
* **Is** a *canonicality* violation: `zz = 0` and `zz = UINT64_MAX` decode identically, so
  **two distinct wire files map to the same decode** — a malleability property of the
  container, and a violation of "CRC uniquely binds input bytes to output".
* **Is** formally **undefined behaviour** (outcomes 2/3). In practice clang-cl at `-O2`
  wraps and the guard catches it. But UB is UB: a future optimizer is entitled to assume the
  addition does not overflow and may delete the `dd <= 0` test. **This is the one reason to
  fix it rather than document it.**

**Explicitly not an emergency, and explicitly not a framing change.** Per coordinator
instruction: no format revision, no wire change, no re-pin of the format lane.

### 3.6 Recommended fix (**not applied** — production source is out of scope this pass)

Two acceptable forms; option (b) is a one-token change and already in-tree:

```cpp
// (a) reject out-of-domain before decoding — matches the codebase's own idiom (:2945)
if (zz >= 0xFFFFFFFFFFFFFFFEull) throw std::runtime_error("bad zigzag varint");
int64_t dlt = (zz & 1) ? -int64_t((zz + 1) >> 1) : int64_t(zz >> 1);

// (b) adopt the wrap-free formula already used at src/anvil.cpp:2538
int64_t dlt = (zz & 1) ? -int64_t(zz >> 1) - 1 : int64_t(zz >> 1);
```

### 3.7 Minimal unit/fuzz test design (no production edit required)

A standalone probe under
`prototypes/swarm-2026-10-02/19-format-security/fledge/zz_probe.cpp`
— **no codec code, no heavy run, deterministic, asserts only**:

| # | input | expected |
|---|---|---|
| Z1 | `zz = 0` | `dlt == 0` (baseline) |
| Z2 | `zz = 0xFFFFFFFFFFFFFFFF` | **must not** silently equal Z1's decode; reject or `dlt == −2^63` |
| Z3 | `zz = 0xFFFFFFFFFFFFFFFE` | clean reject, **no UBSan report** |
| Z4 | `zz = 0xFFFFFFFFFFFFFFFD` | clean reject, **no UBSan report** |
| Z5 | `zz = 0x8000000000000000` | `dlt == −2^62`, accepted |
| Z6 | non-canonical `80 00` (value 0) | reject if canonicality is adopted; else documented-accept |

**Gate:** Z3/Z4 under `-fsanitize=undefined` must be **clean**. That single assertion converts
a formal-UB claim into a measured one, and it is the cheapest experiment in this entire report.

### 3.8 Analogous conversions elsewhere

* `get_uvar` `:292-296` and `read_varint_bytes` `:2156` — non-canonical acceptance and silent
  high-bit truncation on byte 10 (both benign *today* only because every consumer range-checks).
* `int64_t(zz >> 1)` / `static_cast<uint32_t>(dv)` conversions at `:2945-2947` and siblings —
  the `0xFFFFFFFF` pre-guards make these defined.
* The RLZ addends already carry the correct pre-increment guard (`FORMAT.md:726-733`).
* **Conclusion:** the codebase's hardening idiom is consistent *except* for the five zigzag
  sites. This is a **localized omission, not a systemic weakness** — which is why E-1 is LOW.

---

## 4. CROSS-LANE FINDING C-1 — Track 11 FLI: security + one claim-invalidating defect

Audited against `11-orbit-programs-space-bunny.md` and `src/anvil.cpp`. **Do not conflate with
E-1.**

### 4.1 Amplification: conclusion CORRECT, justification WRONG, one latent hang

FLI never computes `n × len`; it loops with a per-iteration `pos + len ≤ block_end`. Because
`len ≥ 1` (guaranteed **only** by the inherited eager book check
`src/anvil.cpp:2897 — if (len == 0 || len > kSparseMaxLen) throw`), each iteration advances
`pos` by ≥1 into a buffer capped at `out_len` ≤ 128 MiB. **So FLI adds zero asymptotic
amplification.** Lemma 3/4 hold *for LOOP_EQ*.

Two corrections:

* **Category error.** FLI Lemma 3 cites `raw_n ≤ 16·out_len + 64` as a co-dominator. That
  bound governs each **substream's** decoded length (`:2902`). FLI consumes ~0 substream bytes
  per iteration, so `raw_n` is **irrelevant** here. The true dominator is the output bound.
  Right answer, wrong reason.
* **Latent hang, closed only by an unstated coupling.** If `L0 == 0` and `n` is large, `pos`
  never advances and `block_end` is never reached ⇒ unbounded loop. Mode 15 rejects `len == 0`
  at parse time today (`:2897`), so **there is no present bug**. But FLI's spec never states
  `require L0 ≥ 1`. **Mandatory hardening: add it explicitly.** One line; removes the only
  hang class.

### 4.2 BLOCKER F1 — `LOOP_ARITH` is unreachable; the headline claim is vacuous

This is the finding that changes the verdict, so it is stated at full strength.

* Mode-15 hot `kind==1` **reads** `last[b.shape]` and **never writes it** (`:2924`; contrast
  the macro path, which *does* write at `:2950`).
* Therefore across any run of consecutive same-class hot ops, `last[shape_c]` is **constant**
  ⇒ `D_i` constant ⇒ **`δD ≡ 0` forced**.
* Hot length is `book[c].len` (constant per class, `:2918-2919`) ⇒ **`δL ≡ 0` forced**.
* FLI's own P2 filter agrees: a run admits only tokens whose `dist_i` is "derived from
  `last[shape]`" (report L200-201), i.e. all equal; macro/literal tokens end runs.

⇒ **Every admissible FLI run is `LOOP_EQ`. `LOOP_ARITH` can never fire.** Consequently the
"exact multi-parameter recurrence" novelty separator **S3**, ablation arm **A3**, and the
§12.1 "PILOT LOOP_ARITH" disposition are **dead weight**. What remains is *counted repetition
of one hot op* — squarely prior art (§7).

### 4.3 BLOCKER F2 — LOOP_ARITH arithmetic is under-specified anyway

`(zz+1)`-class wrap aside, FLI's recurrence has **no pre-increment guard and no stated
accumulator width**, and it uses the **addition form** `pos + L0 ≤ block_end` where the entire
codebase uses the **subtraction form** `len > out_len - out.size()` (`:2920,2925,2934,2939`).
Safety currently holds only because `L0` happens to stay `uint32`. Moot while F1 stands, but it
must be fixed before any `LOOP_ARITH` revival.

### 4.4 BLOCKER F3 — wire framing breaks backward compatibility, and is unprobed

Mode 15 hard-codes `std::array<std::vector<uint8_t>, 9> s` (`:2901`) and rejects
`p != e` (`:2911`). FLI adds three raw cursors (`S_n`, `S_dl`, `S_dd`) with **no framing spec
and no version/flag**. An unconditional 12-stream read **breaks every existing 9-stream
mode-15 file**. FLI §9's controls check encoder `--fli=off` byte-identity but **never test
decoding a pre-FLI mode-15 file**, violating the COMPATIBILITY control in `FORMAT.md:828-838`.

### 4.5 BLOCKER F4 — opcode-space growth is unmodelled

Adding `K+1…K+4` grows the alphabet past the current dispatch, which reads
`if (op < K) … else if (op == K) <macro>` (`:2917,:2929`). FLI budgets "≤ 4 codes ≈ ≤ 30 B
per block" as a *byte* cost but does **not** re-derive: (a) how `H_op` — the entire §5.1 byte
model's central quantity — inflates when the alphabet grows; (b) that each new opcode must
advance the nine consumption cursors checked at `:2979-2981`.

**Verified correction (blocker partly withdrawn):** I had flagged the risk that a malformed
`op > K+4` might fall through. It does not — `src/anvil.cpp:2977` is
`else throw std::runtime_error("bad hotop opcode");`, and `:2979-2981` enforces full
consumption of all nine substreams. **The rejection and consumption discipline is already
sound; FLI need only preserve it.** F4 is reduced to the `H_op` accounting question, which is
a byte-model concern, not a safety one.

### 4.6 Fuzz-probe gaps in FLI §9

Missing: `n == 1` (spec requires `n ≥ 2`); `book[loop_class].kind == 1` (a kind-0 literal entry
has no meaningful `last[shape]` distance — `loop_class ≥ K` *is* probed, `kind==0` is not);
`δL`/`δD` overflow; per-iteration copy-strategy crossing in `LOOP_ARITH` (the
`dist ≥ len ? range-insert : periodic` branch at `:2926-2927` depends on values that vary per
iteration, and FLI never re-specifies it); old-file decode compatibility (§4.4).

---

## 5. CROSS-LANE FINDING C-2 — Track 10 WCT: wire blockers

Audited against `10-correction-topology-space-bunny.md`. **Positive first:** the constructive
lane **conceded C1 outright** (report L16) — rev-1's 3-bit submask cannot code the **15**
non-empty subsets of 4 bytes, which is a real non-injective format bug, and rev-2 replaces the
T6/T7a/T7b triad with a proven-bijective **17-symbol** alphabet. That concession is correct and
I verify the forward/inverse argument at report L112-114.

Blockers that remain:

### 5.1 BLOCKER W1 — the 17-symbol alphabet costs *more* than the 4-bit submask it replaces

Quantified at `L = 65536` (`W = ⌈L/4⌉ = 16384` windows), which is the format's hard cap
(`kSparseMaxLen`, `src/anvil.cpp:453`):

| form | bits/window before entropy coding | bytes per 64 KiB token | % of `L` |
|---|---|---|---|
| 4-bit packed submask | 4.00 | 8192 | 12.50% |
| 17-symbol, byte-aligned | **8.00** | 16384 | **25.00%** |
| 17-symbol, order-0 entropy (⌈log₂17⌉ = 4.087) | 4.087 | 8379 | 12.79% |

**The best case for the 17-symbol alphabet is 12.79% versus 12.50% — it is strictly worse
before any distribution term, and the report itself concedes the byte-aligned form "pays ~1
byte/window where a bit-packed form would pay ~0.1" (report L126).** The simplification buys
*alphabet disjointness* (a real correctness win) at a byte price that is ≈0 even in the best
case and **2× in the CLEAN-heavy case the report says it is designed for**. Since the project's
standing rule is that no mechanism may worsen bytes, **WCT as specified cannot pass its own
gate.** It needs an order-1+ symbol coder or a 5-bit packed `submask ∈ {0..15} ∪ {16}`
representation (which is injective at 5 bits = 16.4% … still worse than 4-bit submask + 1
class bit = 5 bits, i.e. *identical*, and the disjointness property would be restored by
construction). **Recommendation: adopt the 5-bit packed form; keep the bijection.**

### 5.2 BLOCKER W2 — tail-window submask must be masked to `T = L mod 4`

The report's inverse rule (L113) counts residual bytes as `Σ popcount(sym_w)` for `sym_w < 16`,
"plus `popcount(tail)` if `T > 0`" — but it never states that **bits `b ≥ T` in the final
partial window must be rejected**. If a crafted stream sets a tail-window bit beyond the
phrase end, `out[start + 4W + b]` (report L183) writes **out of the block**. Mode 11/12/13/14
all enforce the analogous rule today, and — decisively for the "must state it" argument —
**the correct check already exists verbatim in mode 15's macro path**: `src/anvil.cpp:2962`
computes `over = first + 32 - len` and throws `"mask bits beyond copy length"` when the
corresponding high bits are set. So WCT's per-window tail case is not an unknown hazard; it is
an **inconsistency with house style**, which makes omission more likely to be an oversight than
a decision. **WCT must carry `:2962`'s semantics into the tail window.** This is a genuine
memory-safety blocker, not a canonicality nit — and it is the **only** one in this report that
could produce an OOB write.

### 5.3 BLOCKER W3 — XFORM/RESID mutual exclusivity is an *encoder* property, not a decoder invariant

The report's forward rule (L112) says the three cases are "disjoint and exhaustive (**E4**
guarantees disjointness)". But E4 is `scan_candidate` behaviour — `j += 3; continue`
(`src/anvil.cpp:594-604`) — i.e. an **encoder-side** greedy-scan property. The decoder cannot
verify that an `XFORM` window's copied bytes actually satisfy `t32 == s32 − dist`; it simply
performs the copy. **The only guarantee the decoder can rely on is alphabet disjointness
(`0` = CLEAN vs `16` = XFORM).** The report conflates the two. Impact is bounded (wrong bytes →
CRC reject, `:4882/4888`), so this is a **precision/claim** blocker, not a safety blocker —
but the "mutual exclusivity is structural" argument is the load-bearing novelty claim
(report L276 itself calls it "fragile"), so it must be stated correctly.

### 5.4 BLOCKER W4 — stream count and full consumption

Report L120-121 goes 8 streams → 7 (masks+tmask → `wclass`). Verified that mode 14 currently
has 8 and that ordering is now `resid` **before** `wclass` (report L121). Required: full
consumption on **both**, plus `p != e` (`src/anvil.cpp:2911`) — consistent with
`FORMAT.md:552`. And note the ordering change means the decoder must materialize `sym[]`
(eagerly) to know how many residual bytes to consume, i.e. **+16384 B of transient state at
L = 65536** — a real RSS term the report does not charge. Relatedly, the existing
`std::array<uint32_t,(kSparseMaxLen+31)/32> words{}` is an **8 KiB zero-init per token** on the
stack (`:2956`); WCT should state whether that per-token memset survives.

---

## 6. Assessment of the constructive mechanism VCA

**I accept the diagnosis, reject the priority, and reject two of the three novelty claims.**

### 6.1 What I accept

* §2.1: the declared-`total` bound is a byte-count proxy, not a work bound, and there is no
  caller-suppliable ceiling. **Independently confirmed** at `src/anvil.cpp:4855`.
* §1.6: the fuzz harness asserts only "reject OR reconstruct identical bytes"
  (`docs/decoder-audit.md:141-147`) with **no bound on time, allocation, or work**, and the
  1 GiB substream allocation was found by a **human probe, not the fuzzer**
  (`docs/decoder-audit.md:56-69`). **This is the best-evidenced, cheapest item in either
  report and I promote it above VCA.**
* §1.5's random-access observation (`O(N × in.size())` header reads on hostile files) is
  correct as a *constraint on other tracks* and correctly declined as a competing mechanism.

### 6.2 What I reject or discount

* **Novo claim (b)** — "zero wire bits, so it retrofits a frozen revision". True, but it is
  also true of the trivially cheaper fix in §8/T2 and of a plain `--max-output-bytes` flag.
  Zero-wire-bits is a *convenience*, not a novelty separator.
* **Novo claim (a)** — "same functional is both encoder objective and admission meter". The
  S6-1 costs are in **ns/byte** and are *calibration-dependent* (FORMAT.md:804-806 records
  they came from a decode-perf floor profile on one host). A **safety** bound keyed to
  uncalibrated, host-specific, measurement-derived constants is a poor design: tighten the
  calibration and you change what files are *legal*. **This is my principal technical
  objection to VCA.** A safety ceiling must be expressed in **structural** units (blocks,
  bytes-out, bytes-in, allocation-events), not in the encoder's cost proxy.
* **Mechanism priority.** VCA is a decoder-API redesign motivated by corpus-artifact
  corruption. That is a real problem, but the demonstrated cost of *not* fixing it is
  currently **zero recorded incidents**, while the cost of VCA is a new invariant every future
  mode must be audited against — precisely the recurring cost the report is trying to end.
  **The cheapest thing that could possibly falsify "VCA is needed" is one remote probe. Run
  that first.**

### 6.3 Strongest falsification case against my own position

> *"A 64 KiB corpus artifact that expands to 585 GiB has never occurred, cannot occur by
> accident, and cannot occur without someone deliberately planting it — and the project's own
> inputs are trusted. VCA is therefore **engineering theater**: a large, invariant-bearing
> decoder change justified by an attack the project will never face, at the cost of a
> calibration-dependent safety invariant."*

**This is the strongest argument in the entire paired report set, and it is why my verdict is
HOLD rather than PILOT.** My answer is narrow and I do not claim otherwise: §2.1 is real,
cheap to *demonstrate*, and the demonstration is worth one CI job. Whether it justifies a
mechanism is a decision that requires the demonstration plus an incident record — neither of
which exists. **If the probe comes back at the predicted amplification, HOLD converts to
PILOT. If it comes back low, VCA is KILL and the project keeps a one-line flag.**

---

## 7. Prior-art map

| threat / mechanism | prior art | our residue |
|---|---|---|
| output-amplification cap | zstd `ZSTD_d_windowLogMax` + `frameContentSize`; Brotli `BROTLI_DECODER_PARAM_LARGE_WINDOW`; LZMA `dicSize`; "decompression bomb" literature | **adopt-class.** Nothing here is new. |
| caller-supplied decode ceiling | `libdeflate` / zstd `ZSTD_d_*` budget params; 7-Zip `-mmem`; `zlib` `Z_STREAM` limits | **adopt-class.** |
| strict malformed-input rejection | the entire DEFLATE/zstd/Brotli RFC decoder discipline | already largely held (§1.2) |
| canonical varint rejection | protobuf, CBOR, FlatBuffers strict decoders | adopt-class, cheap |
| corruption containment / partial decode | zstd frame checks, `--ignore-checksum`, `gzip -f`, archive formats with per-entry CRCs | **adopt-class**; the *hole-record* accounting is the only mildly novel residue |
| counted repetition superinstruction (FLI) | repeat strings, XMill, DEFLATE rep-matches, LZMA rep0–3, RePair; superinstruction literature | after F1, **adopt-class engineering** |
| window-class alphabet (WCT) | Elias/Fano/Waldvogel bitvector coding; sparse-bitvector residual coding | **blocked** by W1 |

**Net novelty assessment for this track: essentially zero.** That is the correct and expected
answer for a format/security track. The value delivered here is *proof that the frozen format
is already in good shape*, plus a small localized defect list and one demonstrated hazard.
Claiming novelty here would be exactly the novelty-by-difference the brief forbids (item 1).

---

## 8. Hidden costs, provenance issues, and required ceilings/gates

### 8.1 Hidden costs I found that the constructive report does not charge

| item | cost | where |
|---|---|---|
| ctx-rANS table | **≈192 KiB** (K ≤ 12 × 4096), *not* 1 MiB | `FORMAT.md:623` — corrects constructive §3.1 |
| mode-14 per-token stack zero-init | **8 KiB memset per token** (`(65536+31)/32 × 4`) | `src/anvil.cpp:2956` |
| WCT transient `sym[]` | **+16384 B** at `L = 65536` | report L121, uncharged |
| FLI `H_op` inflation | unquantified; it is the *central* term of FLI's byte model | report §5.1 |
| threaded decode | **2.0× peak** output memory | `src/anvil.cpp:4907,4924-4925` |

### 8.2 Provenance / audit issues

1. `tests/benchmark-summary.csv` carries **no decoder-build, format-revision, or
   resource-ceiling column** — a remote ratio row is not self-identifying about which decoder
   contract produced it. (Constructive §2; confirmed and important.)
2. CI runs `tests/fuzz.py` **without `--mutations`** and with **no sanitizer job**, so the
   mutation phase the audit relies on is *not what CI runs today*. (Constructive §2; this is
   the single highest-leverage harness gap.)
3. Line numbers in both reports are unpinned to a commit; `FORMAT.md:907-910` mandates re-pin.
4. Constructive §3.1's ctx-rANS figure is wrong by ~5× (§8.1). Minor, but it shows the
   accounting table was not derived from the live tree.

### 8.3 Deterministic decode ceilings I recommend (structural units, per §6.2)

| ceiling | value | rationale |
|---|---|---|
| `max_output_bytes` | caller default `min(total, 4 × in.size() + 1 MiB)` | ~4× is above honest worst case (mode-11 raw ≈ 1.1×10⁴ would exceed it — **so set the default from measurement, not from this**) |
| `max_total_blocks` | `⌈in.size()/7⌉ + 2` | already implied; make it explicit and checkable |
| `max_alloc_bytes` | caller-supplied, default `2 × total + 64 MiB` | caps the threaded path's 2× and realloc transients |
| `max_substream_bytes` | keep `16·out_len + 64`, but **verify it is not cited as a work bound** | see §4.1 category error |
| canonical varints | **enforce** | §2.5 |

### 8.4 Fuzz gates (deterministic, remote-only)

| gate | assertion |
|---|---|
| **FG-1** | every case under a hard `max_output_bytes` + `max_alloc_bytes`; violation = **test failure**, not a lucky find |
| **FG-2** | UBSan/ASan job in CI (currently **absent**) |
| **FG-3** | work oracle: `bytes_produced / bytes_consumed` ratio **per case**, asserted ≤ declared ceiling |
| **FG-4** | **canonical-form rejection**: overlong varints rejected (or documented-accepted and pinned) |
| **FG-5** | non-canonical zigzag Z2–Z4 from §3.7; **UBSan-clean is the assertion** |
| **FG-6** | per-mode **tail-mask** rejection (WCT's W2), added before WCT is built |
| **FG-7** | old-file decode compatibility for any mode change (FLI's F3) |
| **FG-8** | CI actually passes `--mutations` with a fixed seed and pins the mutation count |

---

## 9. Decisive GitHub-Actions-only experiment (**frozen, cheapest first**)

**E19-1 — declared-`total` slack probe. Byte-only + counters. No timing, no corpus benchmark.**

* **What:** generate, deterministically, a set of hand-built rev-1 files with declared `total`
  at the format-legal ceiling (`(in.size()/7 + 2) × max_block`) and payloads that decode
  cheaply, at three sizes: **64 KiB, 1 MiB, 8 MiB**. Decode each under an **rlimit** of
  (a) 2 GiB address space and (b) a 60 s wall clock. Record, per file: input bytes, declared
  `total`, bytes actually produced before abort, peak RSS, wall time, exit class
  (`clean-reject` / `oom` / `timeout`).
* **Runs on:** `ubuntu-24.04`, **manual dispatch only** (master brief item 7; tier C of
  `docs/GITHUB-ACTIONS-BENCHMARKING.md`). No local execution.
* **Frozen thresholds — registered BEFORE the run:**

| # | Threshold (frozen) |
|---|---|
| **K1** | If any file with `in.size() ≤ 64 KiB` produces **≥ 1 GiB** of output before abort, the slack is **CONFIRMED** ⇒ VCA (or a ceiling flag) is justified. |
| **K2** | If peak RSS under the 2 GiB rlimit **exceeds** 2 GiB or the process is **OOM-killed**, the *memory* exposure is confirmed independently of `total` ⇒ escalate. |
| **K3** | If the 8 MiB file does **not** clear 8 GiB, my §2.1 amplification projection is **wrong** ⇒ retract it and downgrade to HOLD/KILL. |
| **K4** | If the probe is **inconclusive** (runner too small to observe it), the result is `BLOCKED_INFRA`, **not** a pass. Repeat at a larger runner or with a smaller declared `total`. **Never** record "safe" from an inconclusive run. |
| **P1** | (promotion) Only if K1 **and** K2 hold does a *ceiling mechanism* earn a PILOT. K1 alone ⇒ adopt the plain `--max-output-bytes` flag (hours, not weeks). |

**E19-2 — zigzag canonicality/UB probe. Microsecond-scale, same job.**
Run `zz_probe` (§3.7) under `-fsanitize=undefined,address` at `-O2`. **Gate:** Z3/Z4 must be
**UBSan-clean and reject**; Z2 must not alias Z1. Failure ⇒ E-1 is upgraded from LOW to
**MEDIUM** (real UB in a shipped decoder) and the fix lands before any new mode is added.

Both are cheap, deterministic, and neither is a corpus benchmark.

---

## 10. Frozen kill / promote thresholds (pre-registered, not to be moved)

| ID | Promote if | Kill if |
|---|---|---|
| **T1** | E19-1 confirms K1+K2 | K3 ⇒ retract the hazard; VCA **KILL** |
| **T2** | E19-2 shows UB (⇒ fix is mandatory) | clean ⇒ E-1 stays **LOW**, fix opportunistically, **no framing change** |
| **T3** | FLI: §4.2 F1 resolved (either `LOOP_ARITH` shown reachable, or it is struck and S3/A3 withdrawn) | F1 stands ⇒ FLI-as-claimed **KILL**; FLI-as-`LOOP_EQ` = adopt-class repetition, **HOLD** |
| **T4** | FLI: F3 backward-compat test passes | F3 fails ⇒ FLI **HOLD** until fixed (F4's rejection concern withdrawn per `:2977`) |
| **T5** | WCT: W1 symbol-coding cost ≤ 4-bit submask cost (order-1+ or 5-bit packed), **and** W2 tail-mask rejection stated, **and** W3 claim restated as encoder-side | W1 unresolved ⇒ WCT **KILL** (it cannot pass a no-regression-bytes gate); W2 unresolved ⇒ **HARD KILL** (only OOB-write in this report) |
| **T6** | FG-1/FG-2/FG-8 land in CI | not landed ⇒ **no** new mode may be merged (harness is the gate) |

---

## 11. Reconciliation with the paired constructive report

| Constructive claim | My verdict | Basis |
|---|---|---|
| §1.1 declared-`total` is a byte-count proxy, no caller ceiling | **CONFIRMED** | `src/anvil.cpp:4855`, `:4859-4860`, verified |
| §1.6 fuzz asserts no resource bound; 1 GiB alloc found by hand | **CONFIRMED, and promoted above VCA** | `docs/decoder-audit.md:56-69,141-147` |
| §2 CI passes no `--mutations`, no sanitizer job | **CONFIRMED** | `.github/workflows/` |
| §3.1 ctx-rANS table ≈1 MiB at K=256 | **CORRECTED to ≈192 KiB** (K ≤ 12) | `FORMAT.md:623` |
| §3.2/§3.3 21 GiB, ≈23 s @ 3 GHz | **ACCEPTED as a correctly-labelled projection** | derived from `:1885` |
| §1.3 VCA novelty (a)(b)(c) | **(b) discounted; (a) objected to** — safety must not key on host-calibrated ns/byte; (c) is the only durable residue | §6.2 |
| §1.4 `--on-corrupt` containment | **PARTIALLY ACCEPTED, demoted** — harness concern, not format mechanism; "never emit bytes for an unverified region" is the right hard rule | §2.9 |
| §1.5 random access as attack surface | **ACCEPTED as a constraint on tracks 07/15** | §6.1 |
| §9 PILOT | **NOT INHERITED.** I issue **HOLD**; the constructive lane's own §4 self-disconfirmation plus my §6.3 falsification of the priority argue for measuring before building. | §6.3 |
| §10 FLI cross-lane audit | **AGREE on amplification probes; DISAGREE that FLI is merely under-specified** — `LOOP_ARITH` is unreachable, which invalidates the claim, not just the spec | §4.2 |

**Unresolved contradictions for the coordinator** (I do not resolve these unilaterally):
1. Constructive §9 recommends PILOT; I recommend HOLD. Both are defensible; the difference is
   entirely whether one remote job should precede a decoder redesign. **E19-1 settles it.**
2. Constructive credits FLI as "bounded, needs 3 more guards"; I find one guard insufficient —
   the mechanism's generality arm is unreachable. **Coordinator should rule.**
3. Track 10 conceded C1 to a critic; the surviving 17-symbol design is *worse on bytes* than
   what it replaced. **This may not have been noticed by the constructive lane.**

---

## 12. Final ruling

# **HOLD**

Not KILL: the format's core is genuinely strong — I verified strict trailing-byte rejection,
per-block CRC before retention, subtraction-form length bounds, pre-increment distance guards,
capped tables, no cyclic-reference class, and a real allocation clamp on the output reserve.
The one genuine architectural hazard (§2.1) is real, cheap to demonstrate, and currently
undemonstrated.

Not PILOT: the mechanism that would address it (VCA) keys a **safety** invariant on
**host-calibrated, measurement-derived cost constants**, which is the wrong substrate; and its
justification is a corpus-integrity problem with **zero recorded incidents**. Building it
before E19-1 spends a decoder redesign on a hypothesis that one deterministic 64 KiB probe can
confirm or kill in a single CI job.

Not PROMOTE-TO-REMOTE: there is no remote experiment worth funding yet — the pilot *is* the
experiment.

**Explicitly KILL, independent of the HOLD:**
* **VCA as specified** — killed *if* E19-1 K3 fires; downgraded to a one-line
  `--max-output-bytes` flag *if* only K1 fires; promoted to a real mechanism only if K1 **and**
  K2 both fire.
* **FLI-as-claimed** (KILL — §4.2 F1: `LOOP_ARITH` unreachable ⇒ S3/A3 vacuous).
* **WCT as specified** (KILL — §5.1 W1: strictly worse on bytes than the 4-bit form it
  replaces; §5.2 W2 is a hard OOB-write blocker).

**Cheapest decisive next experiment — exactly one:**

> **E19-1 + E19-2 in a single manual-dispatch `ubuntu-24.04` job.** Build one 64 KiB / 1 MiB /
> 8 MiB declared-`total`-ceiling probe set and decode it under a 2 GiB rlimit and 60 s wall
> clock; in the same job compile and run the 8-case `zz_probe` under
> `-fsanitize=undefined,address -O2`. Deterministic, no corpus, no timing comparison, no local
> run. **K1+K2 fire ⇒ the format-security mechanism is funded and I move to PILOT. K3 fires ⇒
> VCA is dead, E-1 stays LOW, and track 19 closes with a one-line ceiling flag.**

Estimated cost: one CI job, < 10 minutes, no GPU, no corpus download.

---

*Prepared by Fledge Alpha Free, independent adversarial reviewer. All line references are to
the live dirty worktree at review time and require re-pin before any remote run. No claim in
this document is inherited from the constructive lane without independent verification in
`src/anvil.cpp`.*