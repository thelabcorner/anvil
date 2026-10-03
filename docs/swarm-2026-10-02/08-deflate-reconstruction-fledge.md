# Track 08 — DEFLATE Reconstruction Integration (Fledge Alpha Free)

**Agent:** Fledge Alpha Free (independent adversarial reviewer) · **Date:** 2026-10-02
**Track:** `08-deflate-reconstruction` · **Worktree:** `i10-aux-unbwt` @ `b8eae11` (intentionally dirty; untouched)
**Status:** FINAL. The constructive lane's report existed at finalization time and is reconciled explicitly in §11.
**Novelty position:** none, by prior ruling (`docs/gate-ruling-i9-p41-deflate.md` V-4). This report makes **no** novelty claim.
**Evidence discipline:** every row is labelled **[M]** measured in a frozen artifact/commit, **[D]** arithmetic derived from [M] or from documented library parameters, **[A]** assumption I introduce, **[P]** projection, **[H]** hypothesis. No local corpus or performance benchmark was run. The only computation I performed is arithmetic over an already-committed JSON artifact (§2.1), reproducible with the command printed there.

---

## 1. Bottom line

The mechanism is real, adopt-class, fail-closed, and its byte win on one corpus file is genuine and large. **That is not the question.** The question is whether ANVIL should carry the pinned-compressor dependency, the decode tax, and the code size to obtain a win that an existing third-party tool plausibly obtains for free.

My independent reconstruction of the decode economics produces a number the constructive lane does not report: **the replay rate measured in this repo fails the constructive lane's own pre-registered promote threshold.** §4.2 derives this. Combined with a missing prior-art control (§5.1), this moves my verdict to **HOLD**, not PILOT.

I did not reach KILL because the downside is structurally bounded at zero (transform 0 always remains available; acceptance is byte-exact and fail-closed), the byte win is measured rather than projected, and the resulting Pareto point is genuinely non-dominated against every same-file reference in evidence.

---

## 2. Independently reconstructed evidence

### 2.1 What I re-derived rather than accepted

I re-computed the stream-size distribution from the frozen census rather than trusting the constructive lane's helper script:

```
# reproducible, reads only committed artifacts; no compression/benchmark performed
python -c "import json,statistics;d=json.load(open(r'prototypes/i9-deflate/results/census-mozilla.json'));\
s=[x for x in d['streams'] if x['method']=='deflate' and x['verified']];\
u=sorted(x['ulen'] for x in s);c=sorted(x['clen'] for x in s);\
print(len(st),sum(u),round(sum(u)/len(u),1),statistics.median(u),u[-1],round(sum(c)/len(c),1))"
```

| quantity | constructive lane (E3) | my recomputation | verdict |
|---|---:|---:|---|
| n verified DEFLATE streams | 2,354 | 2,354 | **agrees** |
| ulen sum / mean | 9,991,436 / 4,244 | 9,991,436 / 4,244.5 | **agrees** |
| ulen median / max | 1,766 / 161,861 | 1,760.5 / 161,861 | agrees (±0.3% rounding) |
| clen mean / max | 1,350 / 53,609 | 1,349.6 / 53,609 | **agrees** |

The lane's E3 is honest. I state this explicitly because a reviewer that finds nothing wrong with the constructive lane's *arithmetic* and still reaches a different verdict is far more informative than one that attacks the arithmetic.

### 2.2 Facts I confirm as measured

| # | fact | label | source |
|---|---|---|---|
| F1 | census: 2,564 detected = 2,354 DEFLATE + 210 stored; all verified; C = 3,177,007 B, U = 9,991,436 B, C/U = 0.3180 | [M] | `prototypes/i9-deflate/results/census-mozilla.json`, `summary-mozilla.json` |
| F2 | replay 2,354 attempted / **2,331 valid / 23 diff / 0 brute**; valid bytes 2,898,132 (91.22%); **3 distinct parameter tuples cover 100%** of valid streams; host **zlib 1.3.1** | [M] | `replay-mozilla.json` (`param_hist`, `zlib_version`) |
| F3 | charged transform 51,220,480 → 57,034,135 B (**+11.35%** carrier); 2,331 records; side table **21,985 B raw / 14,199 B** brotli-q11-lw24 | [M] | `transform-mozilla.json`; my arithmetic reproduces +11.35% vs the lane's +11.37% |
| F4 | ANVIL encode bytes: original **13,806,173**, transformed **12,443,996** ⇒ **1,362,177 B** recovery (−9.87%) | [M] | `RESULTS.md` §2.2, artifacts on disk |
| F5 | mode-17 bound `2·out_len+4096` = 102,445,056 B vs carrier 57,034,135 B ⇒ **PASS with 1.796× headroom** | [M]/[D] | `FORMAT.md` bound; my arithmetic |
| F6 | prototype wire roundtrip sha256-exact on four canonical binaries under the 494/494 encoder-identity gate; **`src/anvil.cpp` integration + fuzz still PENDING** | [M] | ruling V-3 + addenda |
| F7 | same-file references (local, ranking-grade, unpaired): brotli-q11-lw30 **13,806,141**; anvil-ratio **13,806,172** (31 B apart); xz-9e **13,376,248**; zstd-ultra22-long27 **14,967,572** | [M] | `tests/ratio-first-standard.csv` |
| F8 | decode times same window: anvil-ratio 0.3033 s (**168.8 MB/s**); brotli-q11 0.3036 s; xz-9e 0.6053 s (**84.6 MB/s**); zstd 0.1056 s (94.7 MB/s) | [M] | same CSV — **single-window, unpinned, ranking-grade only** |
| F9 | replay cost, **Python**: 8.69 MB plaintext re-deflated in 0.194 s = **44.8 MB/s** | [M] (Python) | `RESULTS.md` §4 |
| F10 | `sao`, `ooffice` contain **zero** DEFLATE; addressable total 1,652,972 B | [M] | `RESULTS.md` §0, ruling V-2b |
| F11 | 23/2,354 streams (8.8% of DEFLATE bytes) were **not reproducible by any zlib-1.3.1 setting** | [M] | `replay-mozilla.json` |
| F12 | no held-out corpus family exists; remote dispatch blocked on authorization | [M] | `I10-FRONTIER-RECON-2026-09-24.md` §3 |

### 2.3 Projections that must not be read as measurements

| quantity | status | why it is fragile |
|---|---|---|
| diff-mode upside **+37,429 … +72,443 B** | **[P]** | applies *published* preflate-rs correction overheads to our stream population; no correction coder was built |
| carrier growth "**+11.37%**" | [M] | correct for mozilla only; it is a function of the corpus's C/U, not a constant |
| encode cost "**+5.2%**" | **[M] but mis-scoped** | see §3.4 — it excludes the replay search entirely |
| all decode MB/s rows | **[P]** | no C measurement exists; my §4.2 supersedes them |

---

## 3. Hidden-cost audit

Charge every decoder-visible byte, table, model, framing field, code byte, and cycle. The constructive lane charges most of the byte axis correctly. These are the items that are **not** on its books.

### 3.1 The replay rate `R` is the whole verdict, and it is already measured — badly

`RESULTS.md` §4 measured 8.69 MB of level-6 re-deflate at **44.8 MB/s**. That is not a Python-overhead artifact: 2,331 calls over 8.69 MB is ~83 µs/call, and the per-call Python overhead is ~1–2 µs, so **the measurement is essentially the C zlib-6 rate on this data**. Python's number is a *good* proxy for C, which is worse news than it first appears, because the constructive lane treats it as a pessimistic floor.

The lane's §5.2 table nonetheless projects "faster than xz-9e" at R = 40/80/120 MB/s and calls the point "plausibly non-dominated." At the *actually measured* R = 44.8 MB/s the point is **1.75× slower than the control arm**, which **fails the lane's own G4 promote threshold of 1.60×**. See §4.2 for the derivation. This is the single most consequential finding in this report and it is absent from the constructive lane.

### 3.2 Encode cost is understated by roughly 2×

The measured "+5.2% encode" (129.6 → 136.4 s) compares two ANVIL encode runs. It **excludes the replay search**, which was executed in a separate offline Python pass (`replay.py`) and is therefore not in either number.

**[D] estimate of the missing term**, from F2 and the stream distribution I verified:

- valid streams: 2,331 first-attempt registry hits × avg 3,727 B at 44.8 MB/s ≈ **0.19 s**;
- inflate of all 2,354 streams, 9.99 MB at ~200 MB/s ≈ **0.05 s**;
- the 23 unmatched streams, if the full grid is searched (405 combos), avg ulen 56,593 B at 44.8 MB/s ⇒ 405 × 1.26 ms ≈ **0.51 s each ⇒ ≈ 11.7 s**.

So the real incremental encode is roughly **+12 s on a 130 s encode ≈ +9%** with `A_max = 405`, versus the +5.2% reported. With a small global `A_max` (8–16) the search term collapses and the reported figure becomes defensible. **The lane must report `A_max` and the search-attempt count in the artifact** (it says it will, in §5.2); the headline "+5.2%" must not be quoted without it.

### 3.3 Unquantified, permanent: the format now depends on a compressor's bit-exact behaviour

This is the cost I rank highest and the lane does not list it.

Transform 4 makes "replay engine output" part of ANVIL's **format definition**. A zlib security or correctness patch that changes deflate output would silently break decode of every archived ANVIL file that uses transform 4. That inverts the usual obligation: a routine upstream update becomes a **data-loss event**.

This is not speculative caution — **F11 is measured proof that it happens.** 23 streams produced by a different zlib build could not be reproduced by *any* zlib-1.3.1 setting. DEFLATE bitstream output is demonstrably **not** stable across encoder versions on this very corpus. The lane's own §11 A2 acknowledges "silent byte corruption or mass decode failure" as a risk; it does not draw the conclusion that the risk is *unmitigable* once archives exist, only *manageable before launch*.

**Consequence:** there is a one-way door. After the first transform-4 archive ships, `replay_engine_id` becomes permanent ABI. That argues for a high bar before building, which is the core of my HOLD recommendation.

### 3.4 Metadata is *not* a hidden cost — credit where due

I tried to break the metadata charge and could not. The prototype's 21,985 B side table (9.43 B/record) includes absolute offsets that the lane's design **does not transmit** (it transmits `raw_gap_length` instead), so the lane's representation is strictly cheaper; the residual saving is a few KB on a 1.36 MB gain (**< 0.6%**) and is immaterial either way.

The lane's characterisation of the measured table as "a measured proxy for the full cost of transmitting offsets anyway" is loose (offset cost ≠ gap-length cost) but directionally conservative, i.e. it over-charges itself. **Not a finding.** The pilot must nonetheless charge the lane's *own* §3 layout, not the prototype's layout — they are different wire formats and only one has been measured.

### 3.5 Applicability cliff vs the mode-17 bound — unstated by either report

`2·out_len + 4096` (F5) implies transform 4 is available only while

> **U ≤ 2·B_out + 4096**, i.e. total replayable plaintext must not exceed twice the block output.

On mozilla U/B = **0.195**, so there is 5.1× headroom — comfortable. But the headroom is consumed precisely in proportion to how compressible the DEFLATE payload is. A block dominated by highly-compressible DEFLATE (a tar of source JARs, a ZIP of text) has C/U small and U/B large; at **U/B > 2** transform 4 becomes unavailable. **Applicability and value are anti-correlated**: the more compressible the DEFLATE, the larger the prospective win and the sooner the bound kills it. The lane's §11 A6 notices the bound but never computes this. It should be stated in any integration note as a hard applicability boundary, not an implementation detail.

### 3.6 Costs that genuinely are bounded

For fairness, these are *not* hidden costs and I am not going to manufacture objections to them:

- **Decoder RSS**: pinned deflate state at memLevel 8 is ≈192 KiB (hash 64 KiB + window 64 KiB + pending 64 KiB per zlib's documented `deflateInit2` sizing), plus a fixed staging buffer. Bounded, small, streaming. **[D]**, design requirement, unmeasured — the lane labels it honestly as [D/P].
- **Decode CPU amplification**: bounded by carrier size, because `ulen ≤ remaining transformed bytes` bounds total replay input by `transformed_size ≤ 2·out_len + 4096`. No unbounded-work exploit. **But see §6.1 — the lane's stated reason for this is wrong.**
- **Downside**: bounded at exactly zero. Transform 0 is always available and selection is "complete payload, same backend." The mechanism cannot make a file worse.

---

## 4. Decode economics — my independent reconstruction

### 4.1 Corrected cost model

```
t_decode(candidate) = t_decode(direct) · (57,034,135 / 51,220,480)   # carrier is 11.35% larger
                    + t_replay(U_valid = 8,689,786 B at rate R)       # NOT 9,991,436 B
```

**Correction to the lane's §5.2:** it uses `U = 9,991,436 B` — the plaintext of *all* 2,354 streams. The 23 diff streams are left **raw** and are never replayed (lane's own E2/A1). The correct replay input is the valid subset, **8,689,786 B** (F2, `summary-mozilla.json` `valid_only.U`). The lane overstates replay cost by **15%**. That error is in the conservative direction so it does not inflate the claim, but it means all three of its projection rows are numerically wrong.

Using F8 (`t_direct = 0.3033 s`) as the bracket:

| term | value |
|---|---|
| `t_direct` | 0.3033 s |
| backend scaling `× 1.11353` | **0.33773 s** |
| replay input | 8,689,786 B |
| replay at **R = 44.8 MB/s (measured, F9)** | **0.19397 s** |
| **total projected decode** | **0.53170 s ⇒ 96.3 MB/s** |
| **paired decode ratio vs control** | **1.753×** |

### 4.2 The decisive arithmetic: the measured replay rate fails the lane's own promote gate

The lane's G4 promote condition is paired decode ratio **≤ 1.60×**. Solve for the replay rate that satisfies it:

```
0.3033 × 1.60 = 0.48528 s budget
t_replay      = 0.48528 − 0.33773 = 0.14755 s
R_required    = 8,689,786 / 0.14755 = 58.9 MB/s
```

> **R ≥ 58.9 MB/s is required to pass the constructive lane's own promote threshold. The replay rate measured in this repository is 44.8 MB/s. A 31% improvement is required, and no evidence for it exists.**

At the measured rate the projected point falls in the lane's own **HOLD band** (between promote 1.60× and kill 2.20×), i.e. by the lane's own pre-registered rules the mechanism does **not** promote today.

**Internal contradiction to flag:** lane §5.2 states that at R = 40 MB/s the point is "plausibly non-dominated" (1.94× slower), while lane §10 G4 fails anything above 1.60×. Those two statements cannot both be operative. Either G4 is too strict for the lane's own hypothesis, or §5.2's non-domination claim is too generous. **This must be resolved before G4 is frozen as written.** Thresholds must not move after data is seen, and the data is already in hand — so the honest move is to state which criterion is binding *now*, before the remote run.

### 4.3 Non-domination check (against every same-file reference in evidence)

Points are (complete bytes ↓, decode MB/s ↑). All from the single ranking-grade window F7/F8.

| codec | bytes | decode MB/s | dominated by candidate? |
|---|---:|---:|---|
| brotli q11 lw30 / anvil-ratio direct | 13,806,141 / 13,806,172 | 168.8 | no — candidate is smaller but slower ⇒ **trade, not domination** |
| **transform 4 + backend 1 (projected)** | **12,443,996** | **96.3** | — |
| xz -9e | 13,376,248 | 84.6 | **yes** — candidate smaller *and* faster |
| zstd ultra22 long27 | 14,967,572 | 94.7 | **yes** — candidate smaller *and* faster |

The candidate point is **non-dominated** against this set. That is the honest positive: the mechanism likely does place a new point on the (bytes, decode) plane for this file. It is a **bytes-for-latency trade**, roughly 1.36 MB smaller for 1.75× the decode latency.

---

## 5. Prior-art map and the strongest falsification case

### 5.1 Prior-art map

| art | mechanism | covers exactly what |
|---|---|---|
| **precomp** (schnaader, ~2010→) | container scan → inflate → **recompress with the same encoder → accept only if bit-identical** → hand plaintext to a stronger codec | **the entire v1 mechanism, including the "valid mode" split and the bit-exact accept rule** |
| **preflate** (D. Steinke) | predict the original encoder's parse decisions, transmit compact corrections | the diff mode v1 declines to implement |
| **preflate-rs** (Microsoft) | multi-encoder detection (zlib, zlib-ng, libdeflate, miniz), hash/chain/nice-length/block-split estimation, CABAC corrections | the state of the art; the source of the §5.2 projection |
| **reflate**, **grittibanzli** | same problem class | siblings |
| zopflipng-class, ZIP recompressors | container-specific recompression | container instances |

Clean-room obligation is correctly stated: do not import preflate-rs (Apache-2.0 / LGPL-3.0-or-later); implement against zlib; vendor under `third_party` with pinned build configuration; never let system zlib define format semantics. **No separator exists and none is claimed. Agreed with the ruling and with the lane.**

### 5.2 Strongest falsification case (the one that decides this track)

> **The prior-art reference implementation is never run as a baseline.**

precomp is a mature, ~15-year-old tool that performs *precisely* this transform — scan, inflate, same-encoder recompress, bit-exact accept, pipe plaintext to any backend — and is available off the shelf with **zero ANVIL decoder cost, zero decoder binary delta, and zero pinned-engine liability**. Its canonical demo corpus is archive-heavy binaries, which is exactly mozilla's shape (a TAR containing 19 JARs/XPI archives).

The constructive lane's §10 experiment has arms A (transform 0), B (transform 4), and generic references. It has **no arm for "precomp + xz -9e"**.

If precomp + xz -9e lands within a few tens of KB of 12,443,996 B on mozilla, then:

1. the byte win is **already available today** without writing a line of ANVIL code;
2. ANVIL's integration buys **zero incremental bytes**;
3. and it costs 1.75× decode latency, an estimated 40–300 KiB of decoder binary (§6.3), and a permanent pinned-compressor format liability (§3.3).

That is a **KILL on product grounds**, and it is falsifiable in a remote job **in minutes, with no ANVIL integration at all**. The constructive lane's own §11 A10 ("reviewer objection: just call preflate-rs") is answered with "no argument available" — that concession is correct for preflate-rs, but it does not address the **precomp** control, which is the cheaper and more damaging comparison.

I do not assert precomp *will* match. I assert that the question is unanswered, cheap to answer, and that authorizing the expensive step (vendoring zlib, writing the ZIP scanner, freezing a `replay_engine_id` ABI) before answering it is the wrong sequencing.

### 5.3 Strongest surviving case

The constructive lane's F7 datum is decisive and I confirm it: on mozilla, **anvil-ratio ties brotli-q11-lw30 to within 31 B**. The portfolio contributes *nothing* on this file. Combined with F8's BWT catastrophe on the same file (+4,017,478 B when routed to BWT), mozilla is provably a file where ANVIL has no lever other than a representation change.

Against that:

- the win is **measured**, not projected (F4), and is **fail-closed** — the downside is exactly zero bytes;
- the resulting point is **non-dominated** against every same-file reference in evidence (§4.3);
- the decode tax is a **linear, bounded, streaming** cost, structurally unlike BWT's sort-bound penalty;
- the incremental decoder RSS is ≈192 KiB and independent of input size.

If the precomp control (§5.2) fails to reproduce, the mechanism is genuinely valuable to ANVIL and the correct response is to build it. **That is why my verdict is HOLD and not KILL.**

---

## 6. Decoder, resource, and security risks

### 6.1 The lane's security thesis is inverted — a design-contract defect

Lane §8 states: *"the decoder's only unbounded-work surface is the pinned engine's work, which is O(clen) per record because clen is declared and capped … That is a genuinely better security posture than a nested decompressor."*

**This is wrong.** A DEFLATE *compressor* is O(**ulen**) in its input, not O(clen) in its output. It must read every plaintext byte and perform window/hash-chain match search to produce the output. A hostile record with `clen` = 200 B and `ulen` = 8 MiB (8 MiB of zeros) makes the decoder do megabytes of match-search work to emit a few hundred bytes. Capping `clen` does **not** bound work; capping `ulen` does.

The *conclusion* the lane reaches happens to be salvageable — because `D4` does require `ulen ≤ remaining transformed bytes`, total replay input is bounded by `transformed_size ≤ 2·out_len+4096`, so work **is** bounded. But the **stated reasoning is inverted**, and if an implementer followed the prose instead of the state machine they would ship a real DoS surface.

Worse, the underlying posture is the opposite of "better": inflate is O(output) with a hard work cap; deflate is O(input) with hash-chain work whose worst case is ~`max_chain`×`ulen` (128× at level 6). **Every archived transform-4 file makes an ANVIL decoder strictly more attack-prone than an ANVIL decoder without it**, and every future zlib memory-safety CVE becomes an ANVIL decoder CVE. Track 19's mandate (bounded allocations, corruption containment, deterministic resource ceilings) must be discharged against a *compressor*, which is a different and less-trodden audit than the one the project has been running against decompressors.

**Required correction before implementation:** replace the lane's §8 paragraph with the `ulen`-based bound, and state plainly that the mechanism trades "no nested-inflate bomb surface" for "a vendored compressor in the decode path."

### 6.2 Missing implementation requirements (small, but each is a real gap)

1. **Streaming output cap.** zlib has no "abort after N output bytes" API. The decoder must feed plaintext in bounded chunks and hard-stop the moment replayed bytes exceed the declared `clen`, writing into a buffer capped at `clen`. Checking only after `flush()` permits unbounded intermediate emission. Not specified by the lane.
2. **Reject engine/format skew explicitly.** On `parameter_code` resolution failure or `replayed ≠ clen`, the decoder must abort **before** appending partial output, so the output buffer never contains a partial stream. Three independent nets (per-record length, final block length, existing CRC) are correct but must be ordered so partial output is discarded, not merely flagged.
3. **`record_count` bound is correct but should also be bounded below by carrier structure.** `record_count ≤ min(transformed_size, block_out_len)` (lane D3) is sound; adding `record_count ≤ carrier_len / min_record_size` makes the parser loop bound self-evident without arithmetic.
4. **Container-completeness vs `--block` (lane §4.1).** Correctly raised by the lane and I endorse it: coverage is a function of `--block`, must be **reported**, and block-size choice is a whole-file decision that also moves the BWT backend's memory profile. Because `block_size` is one archive-level field, this is a genuine coupling the I10 plan missed.

### 6.3 Decoder binary delta — the lane's 64 KiB budget is optimistic

Vendored zlib **deflate** (`deflate.c` + `trees.c` + `adler32.c`) at `-O3` x64 is typically 45–70 KiB of `.text` when all levels and strategies are compiled in, plus a strict ZIP reader (~5–10 KiB). Realistic total: **55–80 KiB** — at or slightly over the lane's 64 KiB promote budget, well under its 256 KiB kill threshold. A specialised build (only level 6 / memLevel 8 / default strategy / raw deflate, since F2 shows 3 tuples cover 100%) could plausibly reach 25–35 KiB, but that is unproven. The lane declares the budget and honestly marks it **[gate]** — correct — but a budget that is more likely than not to be missed is a budget that should be treated as a risk, not a formality.

---

## 7. Corpus, coverage, and recompression ambiguity

### 7.1 The frozen population is a 20-year-old artifact, and coverage is inversely correlated with corpus modernity

F2's parameter histogram — 2,282 of 2,331 at `raw/level 6/memLevel 8/default` — is not a property of DEFLATE. It is a property of **the Firefox build that produced mozilla**. The registry has 3 entries covering 100% of this corpus because this corpus is homogeneous.

Modern archive writers increasingly use **zlib-ng, libdeflate, or miniz** (preflate-rs detects all four precisely because they exist in the wild). None is byte-compatible with stock zlib at the same parameters. A held-out family of *modern* archives could plausibly yield **< 50% replayable bytes**, against which transform 4 becomes byte-neutral and is discarded by the selection rule.

**F11 is direct in-corpus evidence of the failure mode**: 8.8% of DEFLATE bytes came from an encoder that *no* zlib-1.3.1 setting reproduces. Version drift breaks exact replay. Extend that from "one other encoder in a 2005 tarball" to "a whole modern toolchain" and the coverage collapses.

The lane's G3 held-out clause — "≥ 90% of stream bytes replayable" — is therefore well-chosen and is, in my judgement, **the clause most likely to fail**. It is not weighted as such in the lane's ranking of uncertainties (§12 lists it 4th of 5).

### 7.2 Recompression ambiguity

Plaintext does not uniquely determine a DEFLATE bitstream; the encoder's parse decisions are not derivable from the decoder side. The lane handles this correctly with an immutable `replay_engine_id` and a parameter registry. Two residual concerns:

- The registry's `0xFF` escape is **never exercised** by the frozen population (§3, lane E2). It is untested code on the critical path; it must be fuzzed or removed from v1.
- v1's refusal to implement the diff mode forfeits exactly the population where ambiguity bites. That is a defensible scope choice, and the +37–72 KB upside must stay labelled **[P]** and unclaimed. Agreed with the lane.

---

## 8. Alternative mechanism if the main idea fails

The lane's §11 A10 objection ("just call preflate-rs") is correctly recorded as unanswered. There is a materially better alternative than either "give up" or "vendor zlib":

### 8.1 Preferred alternative — replace the *program* with *data* (correction-class explanation)

**Mechanism:** store the plaintext as the payload and transmit a generic, **engine-independent correction** for the bytes that differ from the original compressed payload. The decoder applies the correction; it **never runs a compressor**.

| property | deterministic replay (precomp-class) | correction explanation (preflate-class) |
|---|---|---|
| decoder contains a compressor | **yes** | **no** |
| pinned-engine ABI | **permanent one-way door** | none |
| fails when encoder is unknown | falls back to raw (loses 8.8% now) | **degrades gracefully** into the correction stream |
| worst-case wire cost | 0 corrections | correction bytes (preflate: 0.01–2.7% of U) |
| dependency liability | zlib bit-exactness forever | none |
| clean-room | needed | needed |

The decisive advantage is the last two rows: **the explanation becomes data the decoder already knows how to read, not a program whose semantics are frozen into the format.** All failure modes become graceful degradation rather than total loss.

**The genuinely interesting angle, and the only place this track could earn mechanism-level value:** ANVIL *already owns* residual-coding machinery — the SPARSE-REF residual class coding, the slot-default residual coding, the clustered residual backend, correction-stream topology coding (track 10), and the sparse-corrected phrase wire. Coding the DEFLATE-replay correction with ANVIL's own residual stack instead of preflate-rs's CABAC could plausibly beat the published 0.01–2.70% overhead band on this stream population.

I am **not** claiming this is novel. Preflate and preflate-rs already occupy the class, and the ruling would apply identically. I claim only that it is the **lower-risk instance of the same economic content**, and that if the coordinator funds exactly one of the two, this is the one whose failure mode is graceful.

### 8.2 The cheapest alternative of all — measure before building

Before either: **run precomp + xz -9e on mozilla and on a locked held-out archive family.** This requires no ANVIL code, resolves §5.2, and is the single highest information-per-cost action available to this track.

---

## 9. Decisive remote-only experiment (preregistered)

**Name:** `I10-1B-R Fledge decisive control run`. GitHub Actions only. No ANVIL `src/` change. No corpus sweep. Runs **before** any PILOT authorization.

**Design:** single job, single checkout, single locked corpus, ≥5 repetitions for timing, all non-timing fields verified by post-run SHA-256. Timing is reported as ranking-grade scout evidence only; **no promotion decision may be taken on this run's timings** (per `docs/gate-ruling-i9-recon-crossing.md` R-3, single-rep unpinned timing is not citation-grade).

**Arms:**

| arm | what | why |
|---|---|---|
| **C1** | `xz -9e` direct on mozilla | existing reference; known 13,376,248 B |
| **C2** | **`precomp` + `xz -9e` on mozilla** | **the decisive prior-art control** |
| **C3** | `precomp` + `xz -9e` on the locked **held-out archive family** | decisive control, independent corpus |
| **C4** | `brotli q11 lgwin30` direct | existing reference; known 13,806,141 B |
| **C5** | replayable-byte fraction on the held-out family, split **legacy vs modern** producers | tests §7.1 |
| **C6** | (only if PILOT is authorized later) transform 0 vs transform 4, same backend | the lane's own A/B |

**Corpora:** discovery = the existing frozen mozilla artifacts. Held-out = **one new archive family, created and hashed before measurement** per `docs/I10-CORPUS-LOCK-PROTOCOL.md`, deliberately containing both a legacy-zlib producer and a modern zlib-ng/libdeflate producer. Validation: `samba` (expected negative for v1's ZIP-only scope — worth reporting, not worth gating on).

**Reported:** complete bytes per arm; replayable stream count and byte fraction; producer-identity breakdown for C5; sha256 of every output; wall time (scout-grade).

### 9.1 Pre-registered thresholds (frozen before measurement; not to be moved)

**Primary — the precomp control (C2 vs the candidate 12,443,996 B):**

| condition | verdict |
|---|---|
| C2 ≤ 12,493,996 (candidate + 50,000 B tolerance) | **KILL** — the byte win is obtainable off the shelf at zero decoder cost; ANVIL should decline to implement and publish a deployment recipe instead |
| C2 > 13,000,000 | **HOLD stands** — the mechanism has real incremental value; proceed to PILOT |
| C2 between | inconclusive → report, do not promote |

**Secondary — modern-producer coverage (C5):**

| condition | verdict |
|---|---|
| replayable bytes ≥ 90% on **modern** producers | mechanism generalises |
| 50–90% | **HOLD, reclassified** as a legacy-archive tool, not a general-purpose codec feature |
| **< 50%** | **KILL** for the general-purpose claim; mechanism survives only as a narrow legacy path, which does not justify a frozen engine ABI |

**Tertiary — replay rate `R` (resolve in the pilot's byte-identity self-test; no corpus run needed):**

| condition | verdict |
|---|---|
| **R ≥ 58.9 MB/s** | the constructive lane's G4 (≤1.60×) is satisfiable; proceed |
| **40 ≤ R < 58.9 MB/s** | **HOLD** — measured rate 44.8 MB/s sits here; the point is non-dominated (§4.3) but fails the lane's promote bar; the coordinator must choose which criterion binds, **before** the run |
| **R < 40 MB/s** | **KILL** (the lane's own shortcut; I concur) |

**Blocking correctness (applies whenever any C implementation is built):** sha256-exact roundtrip on 100% of archives; all §6 malformed cases rejected cleanly; zero crash/OOM/hang in the CI fuzz campaign; differential oracle (transform-0 and transform-4 reconstructions both equal the source); transform-4-disabled byte identity against the frozen baseline.

**Absolute kills (not recoverable by re-tuning):** any blocking-correctness failure; replay engine drift detected by the golden self-test; integration requiring a change to an existing transform ID's semantics.

---

## 10. Quantified accounting summary

| axis | control (transform 0) | candidate (transform 4) | delta | label |
|---|---:|---:|---:|---|
| complete payload, mozilla | 13,806,173 B | 12,443,996 B | **−1,362,177 B (−9.87%)** | [M] |
| vs xz -9e (13,376,248 B) | — | — | −932,252 B | [D] |
| carrier size before backend | 51,220,480 B | 57,034,135 B | +5,813,655 B (+11.35%) | [M] |
| mode-17 bound headroom | — | 102,445,056 B limit | **1.796×** | [D] |
| replay metadata (prototype layout) | — | 21,985 B raw / 14,199 B brotli | < 0.11% of payload | [M] |
| replay metadata (lane's layout) | — | **unmeasured** — must charge §3 layout, not prototype layout | ~0.05–0.1% est. | [A] |
| encode time | 129.6 s | 136.4 s | +5.2% **as measured, excludes search** | [M], mis-scoped |
| encode time incl. search (`A_max`=405) | 129.6 s | ~142 s | **≈ +9%** | [D] |
| decode time | 0.3033 s | **≈ 0.5317 s** | **+1.753×** | [P] from measured R |
| decode throughput | 168.8 MB/s | **≈ 96.3 MB/s** | −43% | [P] |
| decode throughput needed for lane's G4 | — | requires R ≥ 58.9 MB/s | measured R = 44.8 | [D] |
| incremental decoder RSS | — | ≈ 192 KiB + staging | bounded, input-independent | [D] |
| decoder binary delta | — | **unmeasured**, est. 55–80 KiB; budget 64 KiB | likely at/over budget | [D]/[A] |
| silesia corpus effect | — | 1,362,177 B ≈ 0.64% of 211,938,580 B | on 1 file of 13; zero on sao/ooffice | [D] |
| frontier-crossing status | 0 FRONT-CROSSING | **still 0** | unchanged | [M] |

**On the last row:** this track cannot produce a FRONT-CROSSING in the project's own terms. Per ruling R-2/R-3 and the master brief §11, a crossing requires clearing both bars on a complete grid at a non-retired configuration. Transform 4 activates only on DEFLATE-containing archives, is adopt-class, and is a routing/portfolio property by prior ruling (P41-9). Its correct status is **"adopt-class portfolio row"** — exactly what ruling V-4 concluded. Any document that describes it as frontier-advancing should be corrected.

---

## 11. Reconciliation with the constructive lane

I read `docs/swarm-2026-10-02/08-deflate-reconstruction-space-bunny.md` in full before finalizing. My disagreements, in order of severity:

| # | lane position | my position | severity |
|---|---|---|---|
| D1 | §5.2 projects decode at R = 40/80/120 MB/s; §10 G4 promotes at ≤ 1.60×; both stand | **The measured R = 44.8 MB/s yields 1.753×, which fails G4.** Required R is 58.9 MB/s. The lane did not evaluate its own gate on its own measurement. | **decisive** |
| D2 | §5.2 claims the point is "plausibly non-dominated" even at 1.94× | Internally inconsistent with G4's 1.60×. One criterion must be declared binding **before** the remote run; thresholds must not move after data is seen. | **decisive** |
| D3 | §10 has no prior-art control arm | **`precomp` + `xz -9e` must be an arm.** If it matches the candidate, the whole PILOT is redundant. Cheap, decisive, requires no ANVIL code. | **decisive** |
| D4 | §8: engine work is "O(clen)… genuinely better security posture than a nested decompressor" | **Wrong.** Deflate is O(**ulen**) in input; the `clen` cap does not bound work. Conclusion survives only because `D4` caps `ulen`; the prose must be replaced. The real posture is *worse*, not better. | **high (design defect)** |
| D5 | §5.2 uses `U = 9,991,436 B` as replay input | Should be **8,689,786 B** (valid subset only). Overstates replay cost 15%. Conservative direction, but all three projection rows are wrong. | medium |
| D6 | §5.2 "encode +5.2%" | **Excludes the replay search** (done offline in Python). Real incremental is ≈ +9% at `A_max`=405. Must report `A_max`. | medium |
| D7 | §5.3 decoder binary budget 64 KiB; §12 uncertainty #2 | Concurs this is a real risk; my estimate is **55–80 KiB**, i.e. the promote budget is more likely than not missed. Should be weighted above uncertainty #4. | medium |
| D8 | §11 A6 notes the `2·out_len+4096` bound | Bound implies **U ≤ 2·B + 4096**; applicability is *anti-correlated* with value. Never computed. Must be stated as a hard applicability boundary. | medium |
| D9 | §12 ranks held-out family availability **4th** of 5 | It should be **1st or 2nd**: modern producers (zlib-ng/libdeflate/miniz) plausibly yield <90% replayable bytes, and F11 (8.8% of bytes unreproducible by any zlib-1.3.1 setting) is direct evidence that encoder-version drift breaks exact replay. | medium |
| D10 | format-stability liability not listed as a cost | **Highest-ranked unlisted cost.** Pinning a compressor's bit-exact output into the format is a permanent one-way door (§3.3). Listed in A2 as a risk, never as a cost. | high |
| D11 | §8 streaming output cap unspecified | zlib has no output-abort API; a hard cap at `clen` during chunked replay must be specified or a real DoS surface ships. | medium |

**Where I agree and give credit** — the lane is unusually disciplined and I say so plainly: E3's stream distribution is accurate (I recomputed it); the side-table charge is honest and conservative; provenance labels ([M]/[D]/[P]/[H]) are used correctly; E6/E8 non-spliceability between two local protocols is flagged correctly; the container-completeness-vs-`--block` coupling (§4.1) is a genuine architectural finding absent from the I10 plan; the novelty concession is clean and correctly scoped; the held-out-corpus precondition is correctly identified as a *precondition* rather than an output; and the diff-mode upside is correctly left unclaimed and labelled [P]. **My disagreement is with the verdict and the accounting, not with the rigour.**

---

## 12. Recommendation

# HOLD

**Not PILOT.** Three concrete reasons, each of which is a sequencing error rather than a scientific objection:

1. **The pilot's own promote gate fails on the pilot's own measurement** (D1/D2). `R = 44.8 MB/s` measured ⇒ 1.753× decode ratio ⇒ fails G4's 1.60×. Authorising a vendored-zlib build and a frozen `replay_engine_id` ABI on the basis of a gate that the available data already fails is premature.
2. **The decisive question is cheap and unasked** (D3). precomp + xz -9e on mozilla answers "should ANVIL implement this at all?" in minutes with zero ANVIL code. Spending the expensive step first inverts the correct order.
3. **`replay_engine_id` is a one-way door** (D10). Once an archive ships, the pinned compressor's bit-exactness becomes permanent format ABI — and F11 proves bit-exactness is not stable across encoder versions. The bar for opening that door should be higher than "1.36 MB on one 2005-era file."

**Not KILL**, because the disconfirming evidence is not about value: the byte win is measured (F4), fail-closed with exactly zero downside, non-dominated against every same-file reference in evidence (§4.3), and located on a file where the portfolio is provably inert (F7, 31 B). If the precomp control fails to reproduce, this is genuinely valuable and should be built.

**Not PROMOTE-TO-REMOTE**: the compression claim has never been produced by an integrated ANVIL decoder (F6), no C replay rate exists, and remote dispatch is itself blocked on an authorization dependency.

**Authorised now (no ANVIL code, cheap, decisive):** the §9 remote-only control run — arms C1–C5, pre-registered thresholds frozen at §9.1.

**Authorised on C2 > 13,000,000 B and modern-producer coverage ≥ 90%:** the bounded isolated PILOT (`prototypes/…/fledge/` peer to the lane's §9 prototype) with `R` resolved inside its byte-identity self-test — no corpus benchmark required.

**Then reconsider:** if `R` lands ≥ 58.9 MB/s and G1/G2 pass, the correct next question is **§8.1 — the correction-class explanation**, which captures the same economic content with no compressor in the decoder, no frozen ABI, and graceful degradation on unknown encoders. It is strictly lower-risk than the mechanism the lane proposes, and it is the version I would fund if only one could be funded.

**Stated in advance as KILL, not HOLD:** C2 ≤ candidate + 50,000 B; modern-producer replayable bytes < 50%; `R` < 40 MB/s; any blocking-correctness failure; replay-engine drift on the golden self-test. None is recoverable by re-tuning — the thresholds are frozen here and the mechanism's economics depend on no tunable.

---

*Provenance: every **[M]** row names a frozen artifact or commit in this repository. The only computation I performed is arithmetic over committed JSON (§2.1, command printed there); no compression, corpus sweep, timing run, or fuzz campaign was executed locally. Rows drawn from `tests/ratio-first-standard.csv` and `tests/auto-routing.csv` come from two different local protocols and are **not** spliceable with each other or with remote Class B rows; both are ranking-grade brackets only. All decode MB/s figures are projections and must be replaced by same-job paired remote measurements before citation. Carries the binding labels `GRID-THIN` and `0 FRONT-CROSSING`. No existing file was modified; no commit, push, reset, clean, stash, restore, or rebase was performed.*

---

# ADDENDUM A — Reconciliation after the constructive lane's revision

**Date:** 2026-10-02 · **Author:** Fledge Alpha Free · **Reads:** the lane's revised report (316 lines, §5.3 now carries a new E19 discussion; §2 gained E18/E19; §4.1/§9/§12 updated).

**Verdict unchanged: HOLD.** Thresholds are unchanged and none are added; §9.1 stands as frozen.

## A.1 What the revision changed, and what it did not

The revision adds two measured rows and one good design property:

- **E18** — container-containment coverage vs `--block`, measured over the frozen census.
- **E19** — no DEFLATE encoder exists anywhere in `third_party/`.
- **§5.3 property 1** — chunked replay is output-identical to single-shot (correct, and worth the golden test it mandates).
- **§9** — an explicit, and in my view correct, decision not to write prototype code this session.

**None of my eleven disputed items was addressed.** In particular §5.2's decode model, §8's security thesis, §10's arm list, and the format-stability cost are all byte-identical to the revision I reviewed.

## A.2 Independent verification of the two new [M] rows

**E18 — CONFIRMED in substance** (my recomputation over `census-mozilla.json`, all 63 container labels):

| block size | lane (ZIP-only, 19 groups) | mine (all containers) | verdict |
|---|---|---|---|
| 128 / 64 / 16 / 8 MiB | 100.00% | 100.00% | **agrees exactly** |
| 4 MiB | 86.29% | 86.49% | agrees (±0.2 pt; scope differs: lane filtered to ZIP, I included PNG) |
| 1 MiB | 54.39% | 55.04% | agrees (±0.7 pt, same scope difference) |

E18 is sound and the coverage-vs-block-size risk is genuinely closed for this corpus. I withdraw nothing here.

**E19 — CONFIRMED exactly.** `third_party/` = {brotli, install, libsais, zstd}; no zlib. `third_party/zstd/lib/zstd.h:1385-1386` is verbatim `ZSTD_f_zstd1 = 0` / `ZSTD_f_zstd1_magicless = 1` — zstd frame formats only. The only `zlib*` paths are `zstd/zlibWrapper/` (`gzread.c`, `gzclose.c`, …), a gzip-container *decompression* compatibility layer. **No DEFLATE encoder is vendored, so the vendoring cost is genuinely unavoidable** — this is a real strengthening of my §3.3/§6.3 position, and I accept the lane's E19 as correct and load-bearing.

## A.3 AGREED / DISPUTED per disputed item

| # | item | status after revision | note |
|---|---|---|---|
| **D1** | measured `R` = 44.8 MB/s ⇒ 1.753× decode ratio, failing the lane's own G4 (≤1.60×) | **DISPUTED — unresolved** | §5.2 still projects only at R = 40/80/120 and still never evaluates G4 at the rate this repository already measured. |
| **D2** | §5.2 "plausibly non-dominated" at 1.94× vs G4's 1.60× — internally inconsistent | **DISPUTED — unresolved** | both statements still present, still unreconciled. |
| **D3** | no prior-art control arm in §10 | **DISPUTED — unresolved** | arm list unchanged: A, B, generic references. See §A.4 for the exact arm to add. |
| **D4** | §8 / §5.3-prop-2: engine work "O(clen)", "better security posture than a nested decompressor" | **DISPUTED — unresolved, and slightly worsened** | §5.3 property 2 now *reasserts* the same claim ("output capped by the declared `clen`"). Deflate is O(**ulen**) in input; the `clen` cap does not bound work. The conclusion survives only via the state machine's `ulen` bound, not via this prose. |
| **D5** | §5.2 uses `U = 9,991,436 B`; correct replay input is the valid subset **8,689,786 B** | **DISPUTED — unresolved** | all three projection rows still 15% high on the replay term. |
| **D6** | encode "+5.2%" excludes the offline replay search; real ≈ +9% at `A_max`=405 | **DISPUTED — unresolved** | §5.2 still reports +5.2% and still does not require `A_max` in the artifact as a reported field (it says "must be reported" in prose only). |
| **D7** | decoder binary delta is a real, likely-missed cost; my estimate 55–80 KiB vs 64 KiB budget | **AGREED — resolved in my favour** | §5.3 now calls the 64 KiB budget "an **[unverified]** assumption, not a measured fact" and "**not comfortable**". E19 additionally removes any substitution escape. Withdraw my objection; this is now the lane's own position. |
| **D8** | mode-17 bound implies **U ≤ 2·B + 4096**; applicability is anti-correlated with value | **DISPUTED — unresolved** | A6 still asserts the bound without computing the applicability boundary. |
| **D9** | held-out **producer-identity replayability** (zlib-ng/libdeflate/miniz) is the binding generalization risk | **DISPUTED — unresolved, and mis-targeted** | E18 closed *containment* coverage, a **different axis** from *replayability*. G3's clause is "≥ 90% of stream **bytes replayable**" — producer identity. E18 does not bear on it. §12 nonetheless still ranks this 4th. |
| **D10** | pinning a compressor's bit-exactness into the format is a permanent one-way door (F11: 8.8% of bytes unreproducible by any zlib-1.3.1 setting) | **DISPUTED — unresolved** | A2 still lists drift as a *risk*; it is never charged as a *cost*. E19 makes it strictly worse — the engine cannot be swapped for anything already vendored. |
| **D11** | streaming hard-cap at `clen` during chunked replay is unspecified (zlib has no output-abort API) | **PARTLY AGREED — now easy, still unstated** | §5.3 property 1 (chunking is output-identical) is exactly what makes this implementable: feed bounded chunks, check emitted bytes after each, stop at `clen`. The lane discovered the enabling fact without noticing the requirement it enables. Cheap to close; still not written down. |

**Net: 1 agreed, 1 partly agreed, 9 disputed.** The revision improved the report's *supporting* evidence (E18, E19, property 1) without touching any of the *decision-relevant* arithmetic or the missing control.

## A.4 The exact control that most efficiently separates "ANVIL-specific value" from "already available free"

### A.4.1 The discriminator

**One arm, added to the same-job run: `precomp <file> | brotli q11 lgwin30`, on the same corpus, in the same job, with complete bytes charged.**

Read head-to-head against the two arms that already exist:

| arm | transform | backend | complete bytes |
|---|---|---|---|
| A (existing) | none | brotli q11/lgwin30 | 13,806,141 (known) |
| **NEW control** | **precomp (external, valid mode)** | **brotli q11/lgwin30** | **?** |
| B (existing) | ANVIL transform 4 | brotli q11/lgwin30 | 12,443,996 |

> **ANVIL-specific value = bytes(precomp | brotli) − bytes(ANVIL transform 4 | brotli).**

This is the efficient form because it **holds the backend fixed**, isolating the transform from the backend choice. `precomp | xz -9e` — the form I sketched in §9 — conflates the transform gain with a backend substitution and is the *wrong* control for the marginal-value question, though still worth reporting as the deployment-recipe number.

**Why `precomp` specifically, and not `advdef`:**

- **Byte-composition equivalence.** ANVIL transform 4's core invariant is that the reconstructed output is **byte-identical to the original file** — outer ZIP offsets, CRCs and sizes stay valid with no metadata rewrite (§5.1, integration plan §6). precomp substitutes payloads **in situ** inside the original container, so `precomp f.zip` is byte-identical to `f.zip` and composes cleanly as `precomp f.zip | <backend>`. **advdef *rebuilds* the archive**, so its output container differs from the input. advdef is mainstream and Microsoft-maintained and should be run — but it does not share ANVIL's byte-identity invariant, so it measures the mechanism's ceiling, not ANVIL's marginal value over it.
- **Zero ANVIL code, zero vendoring, zero frozen ABI.** The whole comparison costs one shell pipeline in an existing CI job.
- **It is the reference implementation of precisely this mechanism** — scan, inflate, same-encoder recompress, bit-exact accept — i.e. the exact valid mode the lane proposes.

### A.4.2 The free second dividend

**The same precomp run also resolves my §9.1 secondary threshold (modern-producer replayability) with no ANVIL code and no preflate-class implementation.** precomp and advdef implement the same bit-exact-accept rule as ANVIL transform 4, so their output on a modern archive family *is* the replayable fraction. Running them on the locked held-out family therefore settles D9 — the one uncertainty the lane's E18 did not touch and still ranks 4th — at zero additional cost, and it does so on the very axis where a non-zlib producer (zlib-ng / libdeflate / miniz) would cause both the tool and ANVIL to fail identically.

**One external run, two frozen thresholds answered.** That is the efficiency claim.

### A.4.3 Decision table (applies the §9.1 thresholds already frozen — no new thresholds)

| observation | verdict |
|---|---|
| `precomp | brotli` ≤ 12,493,996 B (candidate + 50,000 B) | **KILL** — the win is obtainable off the shelf at zero decoder cost, zero binary delta and zero engine ABI. Publish the recipe; do not vendor zlib |
| `precomp | brotli` materially worse than candidate, and modern-producer coverage ≥ 90% | ANVIL-specific value is real and generalises → §9 PILOT proceeds |
| `precomp | brotli` materially worse, but modern-producer coverage < 50% | **KILL** for the general-purpose claim (§9.1 secondary); at best a narrow legacy-archive path, which does not justify a frozen ABI |
| `precomp | brotli` ≈ candidate, **but** the held-out family shows a real coverage gap ANVIL's registry could close | the *only* genuinely ANVIL-specific value on offer is the parameter registry — weigh that against the one-way door, not against a generic replay win |

The last row is the honest form of "ANVIL-specific value" I can construct from the evidence: not a better replay engine, but a **better parameter registry plus an integrated, byte-exact container path** — worth carrying only if it clears a coverage gap that precomp cannot.

## A.5 Recommendation after reconciliation

**HOLD, unchanged.** The revision strengthened the lane's factual base (E18, E19 verified; property 1 correct and valuable) without disturbing any of the arithmetic or the missing control that decides the track. Add the single arm in §A.4.1 to §10 — it is one pipeline, needs no ANVIL code, and answers both open value questions. The 1.753×-versus-1.60× conflict (D1/D2) remains the coordinator's to declare binding **before** the run, not after.

*Addendum provenance: E18 and E19 verified by me over committed artifacts only — container-span arithmetic over `census-mozilla.json`, and a `third_party/` listing plus direct read of `zstd/lib/zstd.h:1385-1386` and `zstd/zlibWrapper/`. No compression, timing, sweep, or fuzz campaign was run. No thresholds added, moved, or relaxed. No existing file was modified beyond this deliverable; no commit, push, or tree operation performed.*