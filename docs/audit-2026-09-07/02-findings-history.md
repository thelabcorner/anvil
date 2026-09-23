# 02 — Findings History Audit

This is not a full replay of `RESEARCH_LEDGER.md`. It is a compressed audit of what each major line actually taught the project.

## 1. Exact LZ + arithmetic / DP era

### Adaptive arithmetic backend

Worked correctly and provided a high-ratio reference, but decode economics were poor. It established that ANVIL’s parser/representation had usable redundancy but was not a production throughput path.

**Keep:** as a modeling reference and for fixed-point machinery.  
**Do not fund:** generic optimization of this old backend as a main codec.

### Bit-cost / measured-cost parsing

The DP and later MDL work proved a recurring principle: **parsing against the downstream coder’s actual economics matters**.

R3 MDL improved record-structured generated data by roughly 8–11% versus the older DP, and beat DP across the local corpus. But encode remained ~1–2 MB/s rather than LZ-class.

**Mechanism success:** yes.  
**Project success:** no; it moved ratio while preserving the main encode bottleneck.

## 2. SPARSE-REF / approximate self-reference

Mode 11 demonstrated that sparse-corrected copies can efficiently express repetitive-but-not-identical records.

On generated.jsonl:

- about −9.6% bytes versus the exact-LZ DP reference;
- ~21× faster encode than DP;
- decode remained far below Brotli.

The repeat control exposed flat-mask overhead, and ordinary source/text files often preferred exact LZ.

**What survives:** sparse-corrected reference is a valid representation primitive.  
**What does not:** the generated-record result as evidence of general-purpose superiority.

## 3. Cheap engineering adopts

### Incompressibility negative gate

One of the cleanest wins in the project. A cheap content-hash probe skips expensive parse work on incompressible blocks. Random-data encode improved by hundreds of times with identical output.

This is a reminder that **not doing work** can outperform sophisticated codec changes.

### Surprise/dead-band sweep

Useful after a confound was corrected. Demonstrated another durable rule: parser/model thresholds should be exposed and swept under a fixed objective, not fossilized from earlier representations.

### Boundary-aligned candidate generation

Measured no output change. The measured-cost parser already captured the useful signal.

**Lesson:** an observed statistical pattern can be real and still add zero incremental information to an architecture that already exploits it indirectly.

## 4. Shape-conditioned displacement prediction

Mode 12 was a genuine isolated success. Per-shape displacement state decisively beat a generic state control on record data; the FLAG-D ablation passed.

Examples on generated data:

- log generic ~0.0921 → per-shape ~0.0756;
- JSON generic ~0.134 → ~0.123.

But Windows decode was still far below Brotli.

**Keep:** semantic shape → conditional displacement state is a sound modeling idea.  
**Audit classification:** mechanism validated, global objective not reached.

## 5. Topology / modal-residual coding

Mode 13 looked attractive from Linux diagnostics but lost badly on the Windows corpus: roughly +13–21% on the main record files.

Measured cause:

- modal accuracy ~17–23%, not the expected ~86%;
- masks mostly unique;
- parser alignment drift destroyed the stationarity the coder needed.

Later SRR work found more structural tokens but still did not make the topology model economical.

**Status:** closed for the current `(k,slot)` formulation. Do not tune it further without a new source of stable aligned topology and a pre-hoc proof that the resulting distribution crosses the coding break-even point.

## 6. Stream entropy suite and context-switched rANS

### Precision/work-adaptive stream suite

After fixing an incorrect objective form and restoring raw as a real candidate, the J-selector became 100% faithful at the frozen λ on the measured streams, with neutral/better ratio and modest decode improvements.

**Strong lesson:** fallback correctness and cost-model fidelity mattered more than exotic entropy coding.

### Context-switched rANS

The strongest single local-corpus ratio mechanism:

- JSON about −12%;
- JSONL about −16%;
- log about −12%;
- SQLite about −8%.

It cost a few percent decode and did not produce a real frontier crossing. Brotli already uses decoder-visible context maps, so novelty is narrow; ANVIL’s fast quantizer is enabling infrastructure.

The new Silesia result is also sobering: the top-level ctx1 transform generalizes only to `mr`, not to broad text. Context signal is real, but its economic value is backend- and representation-dependent.

## 7. Decoder fusion and hot-op books

### Fused decode

Real ~1.2× improvement on primary record files, up to ~1.76× on repeat data. The 2× target failed because entropy pulls remained the floor.

### Compiled hot-op instruction book

Another real ~1.17–1.34× over fused mode 12, but again below target. It removed dispatch/semantic overhead until the surrounding codec—CRC, materialization, allocation, and entropy—became dominant.

**Project-level lesson:** repeated local hot-loop optimization hit Amdahl’s law. Once the hot loop is no longer dominant, more instruction-book work is low EV unless bytes also move.

## 8. SRR / structural-period discovery

This line is one of the project’s most instructive negatives.

The synchronized probe **did** find more period-aligned/span-like tokens. On purpose-built jitter data it raised period-window hits dramatically. Yet final compressed size regressed and topology coding still lost.

The result survived a fair synthetic retest designed specifically to contain the target signal.

**Conclusion:** detection success ≠ representation success ≠ codec success.

The old SRR/topology combination should remain retired.

## 9. TCOPY / exact transformed references

Implicit `Delta = -distance` captures a real executable relocation algebra. Small Windows PE gains were measurable, and Linux `.text` prototypes showed stronger density under a different parser/search regime.

The important theoretical narrowing from later audits:

- exact reference-derived transforms can be economically and conceptually distinct;
- noisy/statistical zero-bit derived parameters fail because estimation error grows with distance;
- transmitted generic parameters can work but are established prior-art territory and need no novelty framing.

**Keep:** exact transform invariants as a specialized binary lane.  
**Do not generalize:** into “arbitrary transformed copy” without evidence.

## 10. PNRA / invariant indexing

The isolated harness showed `raw+pnra` finds additional valid PE copy opportunities. The end-to-end entropy-coded integration then produced essentially zero gain / slight regression despite hundreds to thousands of real committed candidates.

Measured cause: short, far candidates did not amortize framing and the local heuristic underpredicted real entropy-coded cost.

This is a textbook example of why candidate-count or match-density metrics cannot substitute for final bytes.

**Status:** default-off, not adopted. Reopen only with a measured fix to real cost prediction or a token form that amortizes the short-field case.

## 11. ARI-REF / arithmetic relations

Transmitted-Δ ARI-REF strongly encoded a purpose-built arithmetic progression. That proves ANVIL can express a redundancy family exact LZ cannot.

But:

- the implicit/zero-bit variant failed under noise;
- the statistical derived-parameter family was later closed mathematically;
- transformed Brotli on the same synthetic data was dramatically stronger than the original raw-Brotli comparison.

**Useful result:** representation capability validated.  
**General compressor evidence:** none.

## 12. RLZ/RePair stream compression

Modes 7/8 gave real ratio wins—around 10% on some generated record streams—but worsened decode and made encode tens of times slower.

**Status:** valid ratio-only research modes, default-off.  
**Reopen condition:** a fundamentally cheaper grammar/reference construction or lazy/fused decoder that changes the measured economics, not micro-tuning of the current implementation.

## 13. Whole-codec stream budget

At the frozen λ=0.01, the objective was mathematically too conservative to flip expensive streams to raw; it only changed rANS precision on exact-size ties. Peer measurements saw no meaningful end-to-end decode change, and the formal arbiter never ran before that session ended.

This experiment teaches a procedural rule:

> Before implementing a cost-weighted router, calculate whether the allowed cost term is large enough to change any decision.

If the answer is “no,” the experiment is arithmetically dead before code is written.

## 14. Third-party ratio reset: mode 17

The new ratio session intentionally challenged the internal narrative.

Mode 17 initially wrapped Brotli q11/lw30 with two reversible candidates:

- previous-byte context partition (`ctx1-256`);
- line-record column transpose.

Canonical Silesia showed:

- 11/12 files reject both transforms;
- `mr` selects ctx1 and saves 54,503 B versus Brotli;
- `mr` still loses to xz on that file;
- enwik8 rejects both transforms and costs only framing.

This is the current highest-quality evidence about the transforms.

## 15. BWT backend reset

Backend ID 2 is the first new backend under the explicit rev-2 registry. Direct BWT already beats Brotli on seven Silesia files, including large gains on `webster`, `x-ray`, and `mr`, while losing catastrophically on mixed/archive-heavy `mozilla`.

This changes the strategic question:

> The next win may come from **choosing the right compressor for the local data regime**, not from forcing one model to dominate every regime.

That hypothesis is now the project’s highest-EV near-term test.

