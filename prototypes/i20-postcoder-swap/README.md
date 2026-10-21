# I20 C5 — postcoder decode-cost swap prototype

**Owner:** W5 (ANVIL Iteration 20 — Pareto Gate Closure).
**Mandate:** candidate **C5** of `docs/I20-PARETO-GATE-CLOSURE-PREREG-2026-10-21.md` §5.1 —
*"Postcoder decode-cost swap (replace the adaptive-arithmetic MTF token coder with a
cheaper coder from the existing stream suite — rANS/Huffman — over the same MTF token
stream)"*.

This directory is a **research prototype**. It does **not** modify `src/anvil.cpp`, and it is
**not** a proposed ANVIL format change. It produces a *measurement package* (byte cost +
decode throughput + the integration estimate) so the coordinator can decide whether an
integration worker is worth scheduling.

---

## 1. Hypothesis

ANVIL's BWT backend (`backend_id = 2`) codes the MTF token stream with an **adaptive
order-1 arithmetic coder** (postcoder registry ID **2**, the canonical winner; ID **1** is
order-0, ID **3** is the raw-BWT control). An adaptive arithmetic decoder is relatively slow
per symbol. ANVIL already owns a **static, table-driven stream suite** — rANS at 4096/512/256
states, canonical Huffman, default-with-exceptions, raw, and context-switched rANS — with
documented per-stream decode cost units and a documented selection objective
(`J = L + lambda * C_decode`, `lambda = 0.01`).

Re-coding the *same* MTF token stream with a cheaper member of that suite should buy decode
throughput at a fully chargeable byte cost. Whether the trade is worth it depends on how
large the postcoder's share of whole-codec decode time is (**the Amdahl precondition**).

## 2. Pre-registered decision rule (prereg §5.1 C5, quoted verbatim)

> **Adopt iff the prototype's charged complete-byte cost stays inside the 1,989,665 B surplus
> for its measured decode gain, and the postcoder is >= 35 % of whole-codec decode time (per
> C1).** If C1 shows the postcoder is < 20 % of decode time, C5 is `NO-GO` on Amdahl grounds.

This prototype measures the postcoder share **independently** of C1 (which is W1's
whole-path decomposition), because the share is this candidate's own denominator.

## 3. How this differs from the closed QLFC / LZP postcoder NO-GO

`docs/audit-2026-09-07/06-do-not-reburn.md` §G2 closed **QLFC (postcoder ID 4)** and
**LZP (ID 5)** as a **bytes** question: QLFC cost **+2,437,384 B (+3.23 %)** and LZP
**+279,419 B (+0.37 %)** on the measured corpus, because a *static* local-frequency table
cannot track the fast-changing MTF-rank distribution. Those are **different postcoder
families** (a new rank derivation), and they are **not** re-run here.

C5 is a **decode-speed** question about replacing the **entropy coder** over an unchanged
MTF token stream, using coders ANVIL already ships. The byte question is charged, but the
quantity under test is *decode throughput per charged byte*, not a new rank model.

**This is occupied prior art.** Brotli, zstd, FSE and ANS all swap the entropy coder over a
transformed stream. Nothing in I20 — including this prototype — is a novelty claim; every
I20 mechanism is `{adopt-class}` / `{engineering}`.

## 4. Provenance

| Item | Provenance |
|---|---|
| `proto.cpp` | Compiles the repository's **own** `src/anvil.cpp` **unmodified** via `#include "../../src/anvil.cpp"` with `-DANVIL_NO_MAIN`. Every arithmetic-coder, stream-suite, MTF/RLE, container and BWT code path used by the prototype is therefore **byte-identical to the shipped codec**. No codec code is copied or re-implemented here. |
| `huffman_fixed.hpp` | The **only** copied code: a corrected copy of `src/anvil.cpp` lines 1352-1445 (canonical Huffman model + decoder). **Defect fixed:** the repository stores the first canonical code of each length in `std::array<uint16_t,256> first_code`; the first canonical code of length `l` can reach `2^l-1`, so for `l >= 17` the `uint16_t` truncates it and the slow decode path (`max_len > 12`) compares against a wrong bound — this is the documented root cause of the BWT postcoder-0 *"invalid huffman code"* encoder/decoder mismatch (`src/anvil.cpp`, the `try_post` comment near line 4296). The **wire is unchanged** (only the 256 code lengths are transmitted), so payloads stay interoperable. |
| `i20_mtf_expand_raw` (in `proto.cpp`) | A measurement ablation only: a raw-buffer copy of `bwt_mtf_expand` with a switch that skips the zero-run expansion, used to split the MTF stage from the zero-run RLE stage. |

The prototype **links** the pinned libsais primitive from `third_party/libsais` (the same
primitive the shipped BWT backend uses) for the inverse BWT.

## 5. What the prototype does

### 5.1 Container / payload model (revision 2, the canonical ratio route)

```
ANV0 | 0x02 | uvar block_size | uvar total_input
  per block: uvar blen | u8 mode | uvar plen | u32le crc | payload[plen]
    mode 17 payload: u8 transform | u8 backend | uvar transformed_size | backend bytes
      backend 2 (BWT) payload: 0xFE | u8 postcoder | uvar rate | uvar count | u32le*count | body
        postcoder 2 body: uvar nbits | arithmetic bytes (the adaptive order-1 MTF payload)
```

The prototype parses this **out of a real ANVIL `.anv` file** produced by the unmodified
`anvil` binary with the canonical command

```
anvil c <src> <out> --parse=ratio --ratio-backend=auto --ratio-context=off --ratio-lines=off --bwt-aux=on --quiet
```

so the "real ANVIL backend-2 payload" it decodes is literally the bytes the shipped encoder
emitted — not a re-synthesis.

### 5.2 The swap

The postcoder-2 arithmetic payload is decoded **into the MTF token stream** (tokens + the
zero-run varint bytes) by a faithful copy of `bwt_arith_decode` that captures the tokens
instead of expanding them. The token stream is then re-coded with a candidate:

| Candidate family | Wire (prototype only) |
|---|---|
| `single:<M>` | one stream-suite stream over the *interleaved* symbol stream (tokens with the zero-run varint bytes spliced in, i.e. exactly the symbol sequence the arithmetic coder saw) |
| `split:<Mtok>+<Mrun>` | two stream-suite streams: the token stream and the zero-run varint stream, separately (the same structural split the arithmetic postcoder uses: order-1 model for tokens, order-0 model for runs) |

`<M>` ranges over the seven suite members ANVIL already ships: `raw`, `rans4096`,
`rans512`, `rans256`, `huffman`, `defexc`, `ctx-rans` (stream modes 0-6 in
`src/anvil.cpp`).

**Charging.** The charge is the **COMPLETE** new payload, not a marginal delta: the
prototype computes the exact new `.anv` file size (container header + mode-17 payload +
BWT backend payload + postcoder body), and the portfolio delta is that new size minus the
canonical size, per file.

**Router effect.** If a file's new BWT payload would exceed its Brotli-forced payload, the
canonical router would send that file to Brotli instead; the charged cost of that file then
becomes `brotli_forced - canonical`, not the BWT delta. The driver applies this.

### 5.3 Commands

```
proto info    --anvil F.anv                      # structure dump (JSON)
proto sweep   --anvil F.anv --out out.jsonl [--outdir DIR] [--full-grid]
proto decode  --anvil F.anv --out FILE (--post shipped|arith|candidate --postfile FILE)
proto profile --anvil F.anv [--reps N]           # stage-1 attribution
```

* `decode --post shipped` — the repository's own postcoder-2 decode path.
* `decode --post arith` — the same arithmetic decode but materialising the token stream:
  the **harness-matched baseline** for the swap arms (both arms materialise the token
  stream, so the materialisation cost cancels in the paired difference).
* `decode --post candidate --postfile F` — decodes a prototype candidate payload produced
  by `sweep`.
* `profile` — the stage-1 attribution: container/payload framing, entropy decode, MTF list
  update, zero-run RLE expansion (isolated by an ablation), inverse BWT.

### 5.4 Correctness gate

`sweep` verifies **every** candidate **end to end**: container -> BWT backend (candidate
postcoder) -> MTF expansion -> inverse BWT -> the original bytes, compared byte-for-byte
against the shipped decode of the same container. A candidate that does not reproduce the
file exactly is recorded with `ok=false` and is excluded from the ranking. The shipped
`anvil` binary's own canonical round trip is verified (SHA-256) in the driver before any
timing is consumed.

## 6. Build

```
clang-18 -O3 -DNDEBUG -c third_party/libsais/src/libsais.c -I third_party/libsais/include -o libsais.o
clang++-18 -O3 -DNDEBUG -std=c++20 -DANVIL_NO_MAIN -DANVIL_HAVE_LIBSAIS=1 \
    -I third_party/libsais/include prototypes/i20-postcoder-swap/proto.cpp libsais.o -o proto
```

The workflow `.github/workflows/anvil-i20-postcoder-swap.yml` does this and drives
`tools/i20_postcoder_swap.py`.

## 7. Known limits of this prototype (stated up front)

* The candidate postcoder wire is a **prototype wire**, not an ANVIL format proposal. An
  integration would need a new postcoder registry ID (IDs are never redefined), a
  `FORMAT.md` entry with exact bounds, and the repository's registry/fuzz coverage.
* The prototype decodes the ratio route with **transform 0** and the **BWT** backend only.
  Files the canonical router sends to Brotli are outside the prototype's reach (they are
  measured through the unmodified `anvil` binary).
* The whole-codec decode-time change is transferred from the prototype harness to the
  shipped codec with the harness delta measured explicitly (`proto --post shipped` vs
  `proto --post arith`); the transfer assumption is stated in the results document.
