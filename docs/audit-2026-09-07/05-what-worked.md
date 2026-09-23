# 05 — What Worked and Should Be Reused

The project has more successes than its lack of a final frontier crossing suggests. The mistake would be treating every non-crossing mechanism as wasted work.

## 1. Experimental discipline that worked

### Pre-registration and stop conditions

The best example is the statistical implicit-parameter work: once the pre-hoc estimator experiment showed the residual-entropy slope did not disappear, the lane stopped instead of spending an iteration polishing a dead mechanism.

Keep this pattern:

1. state the mechanism;
2. state the required causal ablation;
3. state the quantitative bar;
4. state the stop condition;
5. measure before integrating deeply.

### Negative-result preservation

Topology, SRR, PNRA, grammar streams, and whole-codec budget all remain useful because their failure causes were recorded. This lowers the risk that a future session repeats the same attractive idea without understanding why it lost.

### Strict roundtrip + malformed-input gate

The decoder audit repeatedly caught real issues before benchmark claims were accepted. This is research infrastructure, not QA overhead.

## 2. Architecture patterns that worked

### Always keep a fallback

Raw-block fallback, exact-LZ fallback, direct-transform fallback, and per-stream raw candidates prevented experimental coders from turning local wins into global regressions.

This should generalize upward:

> every transform, backend, postcoder, and specialized container reconstruction path should compete against a simpler complete-payload fallback.

### Exact-size candidate routing

ANVIL has repeatedly benefited from “build candidates, measure complete bytes, choose smallest.” It is expensive, but it is the most trustworthy research oracle.

Use it to generate labels for a later cheap router rather than prematurely replacing it with heuristics.

### Separate representation from entropy coding

SPARSE-REF, TCOPY, ARI-REF, rev-2 transforms, and BWT all became more interpretable when representation value could be tested with the backend held fixed.

This separation should remain a constitutional design rule.

## 3. Mechanisms worth retaining

### Incompressibility gate

Very high engineering value, essentially free ratio-wise. Keep and generalize to other expensive backends: BWT/CM should have cheap negative gates so incompressible/already-compressed regions do not pay full analysis cost.

### Measured-cost parsing

Even though the MDL implementation was too slow, the principle is correct: optimize the parser against the real downstream coder, not a proxy that drifts from actual bytes.

Future specialized parsers should use exact/learned cost tables generated from real backend output.

### Per-shape state

Semantic state partitioning was a real isolated win. The reusable idea is broader than distances: **state should follow decoder-visible semantic class when that class makes a distribution stationary**.

### Stream codec portfolio + raw candidate

The corrected entropy suite validated portfolio selection inside a block. The new BWT/Brotli result suggests the same economic architecture works at a higher level.

### Context signal with compact physical state

Context-switched rANS showed that the failure of old unconditional order-1 was not “context is useless”; it was “hundreds of cold physical models are expensive.” Compressing logical contexts into a small physical model set is broadly reusable.

### Exact transform invariants

TCOPY’s exact relocation algebra and the PNRA invariant search are still conceptually valuable even though the current token economics did not win. Exact invariants are the only surviving defensible zero-bit transformed-reference family after the statistical-transform audit.

## 4. Research tooling that worked

### Resumable cell benchmark harness

This directly solved a real operational problem: long standard-corpus runs no longer lose all progress when interrupted.

### Purpose-built synthetic falsification files

The right use of synthetic data was demonstrated by SRR/finite-difference retests: construct data where the hypothesized signal definitely exists, then distinguish detector failure from corpus absence.

### Diagnostics before integration

Counters such as PNRA gate/index-hit/verify/commit and mask-recurrence diagnostics made it possible to say *where* a mechanism failed.

Every new backend should expose analogous research diagnostics behind non-wire flags.

## 5. A deeper reusable principle: optimize expected work, not maximum cleverness

The highest-value engineering wins generally reduced unnecessary work:

- negative-gate random data;
- shallow/direct match history where deep chains added little;
- raw stream fallback;
- exact candidate routing;
- event-driven structural search rather than dense probing.

This suggests a production ANVIL should be **sparse in computation**: expensive inference only where cheap evidence says it can plausibly buy enough bytes.

That principle is likely more important to future performance than any one entropy coder.

## 6. What BWT teaches about earlier work

BWT’s first real Silesia pass is important because it obtains large byte gains on several files without using ANVIL’s complex token machinery at all.

That does not invalidate the token research. It tells us where to deploy it:

- use specialized token/reference mechanisms only where they beat simpler backends;
- do not make every file pay for every research idea;
- let backend routing turn domain-specific strengths into a general-purpose aggregate.

The architecture should be a **portfolio of specialists**, not a universal maximalist pipeline.

