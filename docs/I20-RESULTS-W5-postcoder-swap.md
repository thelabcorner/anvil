# I20 / C5 — postcoder decode-cost swap (results)

**Worker:** W5 (swarm) · **Iteration:** ANVIL 20 — "Pareto gate closure"
**Branch:** `research/anvil-i20-w5-postcoder-swap` · **Candidate:** C5 (prereg §5.1)
**Class:** `{adopt-class}` / `{engineering}`. **No novelty claim is made or authorized** — an
entropy-coder swap is occupied prior art (Brotli, zstd, FSE and ANS all do it).

> **This entry was completed by the I20 coordinator from W5's committed prototype, tool and
> workflow.** W5's provider session terminated before it could publish its own write-up, and its
> two dispatch attempts did not produce a usable measurement. **The verdict is therefore
> `INCONCLUSIVE`, not NO-GO.**

---

## 1. Hypothesis and pre-registered decision rule

**Hypothesis (pre-registered, prereg §5.1, C5):** the BWT backend's postcoder codes the MTF token
stream with an **adaptive order-0/order-1 arithmetic coder** (postcoder IDs 1 and 2; ID 2 is the
canonical winner). An adaptive arithmetic decoder is relatively slow per symbol, while ANVIL already
owns a **static, table-driven stream suite** (rANS 256/512/4096, canonical Huffman,
default-with-exceptions, raw) with calibrated per-byte decode costs and a documented selection
objective `J = L + λ·c`. Re-coding the **same** MTF token stream with a cheaper member of that
suite should buy decode throughput at a fully chargeable byte cost.

**Pre-registered decision rule, quoted verbatim from prereg §5.1:**

> **Adopt iff the prototype's charged complete-byte cost stays inside the 1,989,665 B surplus for its
> measured decode gain, and the postcoder is >= 35 % of whole-codec decode time (per C1).** If C1
> shows the postcoder is < 20 % of decode time, C5 is `NO-GO` on Amdahl grounds.

**The boundary that must not be re-litigated.** The repository's do-not-reburn register §G2 closed
the **QLFC (postcoder ID 4)** and **LZP (ID 5)** postcoders as *byte* losers (+2,437,384 B and
+279,419 B measured). C5 is **not** a re-run of those: they are different mechanisms evaluated on
the byte axis, whereas C5 asks whether the *entropy coder* can be swapped for a faster one on the
decode axis. The two must be kept apart in any write-up.

---

## 2. What was built (MEASURED artefacts exist; the runs did not complete)

| item | value |
|---|---|
| prototype | `prototypes/i20-postcoder-swap/` — `proto.cpp`, `huffman_fixed.hpp`, `README.md` (a standalone re-coder that extracts the MTF token stream from a real backend-2 payload and re-codes it with the candidate coders) |
| tool | `tools/i20_postcoder_swap.py` (this branch) — `prepare` / `measure` / `summarize` |
| workflow | `.github/workflows/anvil-i20-postcoder-swap.yml` (this branch; registered on `main` for dispatch) |
| measurement design | the **complete** re-coded payload is charged (never a marginal delta); the decoded output is forced into a checksum so it cannot be optimised away; the comparison is against the **shipped** decode path, not a re-implementation |
| commits | `9141c85` (trade-curve throughput column + per-candidate profiles), `a6c5747` (measure against the shipped decode path), `050e69a` (option offset + ratio-payload varint parse fix), `a7cbeea` (materialise only the *selected* candidates' post bodies) |

**Dispatch attempts:** `39666707896` (cancelled), `39669157654` (failure). Neither produced a
usable trade-curve measurement. The cause is in the workflow/prototype path, not a finding about
the codec.

---

## 3. Verdict

**C5 — postcoder decode-cost swap: `INCONCLUSIVE`.**

**Reason, stated precisely:** the trade-curve measurement (bytes vs decode speed per candidate
coder, per BWT-routed Silesia file) was never obtained, and the worker session ended before its
write-up. **Neither the postcoder's share of whole-codec decode time nor the charged complete-byte
cost of any candidate coder was measured**, so neither branch of the pre-registered rule can be
evaluated. **This is a missing measurement, not a falsification.**

**What the programme's other measurements say about the hypothesis, without substituting for it:**

* The inverse BWT is the dominant term: the vendored source shows it performs **`n/2` dependent
  random accesses for every checkpoint stride** (`third_party/libsais/src/libsais.c:7854-7892`),
  and W2 fitted the walk-budget lever's Amdahl share at **f = 0.000 ± 0.02** against a required
  0.70 (run `39676039083`). The decode budget is therefore concentrated in the LF walk, not in the
  entropy coder, which is the opposite of what C5 needs: **C5's lever can only pay off if the
  postcoder is a large share of the decode, and the evidence available says the LF walk is.**
* The **byte** budget that C5 would have to spend inside is also nearly exhausted: the closed arm
  already spends 1,880,667 B of the 1,989,665 B Silesia surplus, leaving 108,998 B. A postcoder
  swap that costs more than ~109 KB on the Silesia portfolio cannot be adopted at the 2 MiB cap
  that clears the gate, whatever its decode gain.
* **The decisive arithmetic for any future C5 run** is therefore: the postcoder's share of the
  BWT-path decode time (which the *closed* arm's 2 MiB-cap decode of ~2.0–2.7 s makes the
  denominator for), against a remaining byte budget of 108,998 B.

---

## 4. Evidence fields for every number

| # | Claim | run_url | job | workflow | commit_sha | artifact | corpus | label |
|---|---|---|---|---|---|---|---|---|
| E1 | the prototype, tool and workflow exist and are committed | — | — | `.github/workflows/anvil-i20-postcoder-swap.yml` | `9141c85`, `a6c5747`, `050e69a`, `a7cbeea` | — | silesia, enwik8 | — |
| E2 | two dispatch attempts, neither produced a usable measurement | https://github.com/thelabcorner/anvil/actions/runs/39666707896 · …/39669157654 | postcoder-swap | `.github/workflows/anvil-i20-postcoder-swap.yml` | `a7cbeea` | — | silesia, enwik8 | MEASURED (the failures) |
| E3 | the fitted LF-walk share that bounds C5's headroom | https://github.com/thelabcorner/anvil/actions/runs/39676039083 | sweep (silesia) / sweep (enwik8) | `.github/workflows/anvil-i20-aux-walk-sweep.yml` | `ef5d44a` | `anvil-i20-aux-walk-sweep-silesia-39676039083`, `...-enwik8-...` | silesia, enwik8 | MEASURED (W2's) |
| E4 | the remaining Silesia byte surplus at the closed arm | https://github.com/thelabcorner/anvil/actions/runs/39684313068 | gate-closure (silesia) | `.github/workflows/anvil-i20-gate-closure.yml` | `7b9ea3f18397d61bab6f41cd14f0eb50da40b36b` | `anvil-i20-gate-closure-silesia-39684313068` | silesia | MEASURED |
| E5 | the closed postcoder candidates that must not be re-litigated | — | — | — | `docs/audit-2026-09-07/06-do-not-reburn.md` §G2 | — | — | MEASURED (quoted) |

---

## 5. Next steps / blockers

1. **Re-run the trade curve** with the committed prototype and tool; the measurement design
   (charge the complete payload, force the output into a checksum, compare against the shipped
   decode path) is already correct and needs no change.
2. **Bound it before spending it:** the postcoder's share of the BWT-path decode time must exceed
   the pre-registered 35 % for the lever to be worth funding, and the charged cost must fit inside
   the **108,998 B** that the closed arm leaves. Measure the share first — it is a single
   instrumentation run, not a sweep.
3. **Do not re-run the QLFC/LZP postcoders** (§G2) and do not re-run the auxiliary walk budget
   (C2, NO-GO with the fitted `f`).
