# ANVIL I10-1A — Auxiliary-index inverse-BWT Results

**Date:** 2026-09-23
**Status:** active experiment; first end-to-end target passed strongly
**Mechanism class:** adopt-class decoder engineering; no novelty claim
**Implementation plan:** `docs/I10-AUX-UNBWT-INTEGRATION-PLAN.md`

## 1. Experimental question

Does the measured `libsais_bwt_aux` / `libsais_unbwt_aux` mechanism still create meaningful **whole-codec** value after its exact wire index is charged, with every other representation choice held fixed?

Control and candidate are intentionally narrow:

- **control:** mode-17 direct transform, BWT backend, postcoder 2, auxiliary indexes OFF;
- **candidate:** identical settings, auxiliary indexes ON;
- one source checkout and one binary;
- same GitHub-hosted VM;
- CPU affinity to one logical CPU;
- paired/interleaved timing;
- exact decoded-byte equality required;
- raw repetitions retained.

The only intended codec-variable is the additive inner-BWT auxiliary-index representation.

---

## 2. Correctness gate

### Remote smoke

GitHub Actions run: `35918450943`

Public experiment source commit:

- `bc852ed8ec38001e9002aa1b4697db1045ec8bf1`

Result: **PASS**.

Fuzz summary:

- roundtrip variants: **480**
- mutation cases: **2,880**
- deterministic rev-2 cases: **9**
- deterministic BWT cases: **24**
- new auxiliary-BWT assertions: **19**
- golden BWT cases: **4**
- forced postcoder cases: **20**
- registered block modes: **14**
- registered transforms: **5**

The new auxiliary coverage includes:

- default-off vs explicit-off byte identity;
- `libsais_bwt_aux` postcoder payload identity against legacy `libsais_bwt`;
- `I[0] == legacy primary`;
- supported BWT postcoders 1/2/3;
- tiny and sampling-boundary inputs;
- nested outer `0xFF` subblock framing + inner `0xFE` auxiliary framing;
- malformed rate/count/index rejection.

Runner for the smoke happened to be AMD EPYC 9V74. Timing from this smoke is not used for the mechanism claim.

---

## 3. Webster paired experiment

GitHub Actions run: `35918797463`

Public measured commit:

- `e76beb65071d9a56a3f786e47dc273a3345b5d57`
- source implementation is unchanged from `bc852ed`; the later commit only adds the dedicated paired workflow.

Runner:

- Ubuntu GitHub-hosted VM;
- AMD EPYC 7763 64-Core Processor;
- four logical CPUs exposed;
- timing affinity: CPU 0.

Canonical input:

- file: Silesia `webster`
- input bytes: **41,458,703**
- SHA-256: `6a68f69b26daf09f9dd84f7470368553194a0b294fcfa80f1604efb11143a383`

### 3.1 Charged wire cost

| Arm | Encoded bytes | Delta |
|---|---:|---:|
| control, aux OFF | 7,317,361 | — |
| candidate, aux ON | 7,319,896 | **+2,535** |

Exact auxiliary cost:

- **+2,535 B**
- **+0.0061145% of source size**
- candidate overhead relative to the control payload: approximately **+0.03464%**

Both arms decoded byte-exactly to the canonical source.

The frozen I9 prototype predicted a 2,532-B auxiliary-index array for `webster` at the ~1024-walk policy. The integrated wire is only 3 B above that raw index cost after complete framing is charged.

### 3.2 Whole-codec decode

Paired protocol:

- pairs: **9**
- symmetric warmups: **2**
- bootstrap samples: **20,000**
- practical speed threshold: **2%**
- ambient gate: PASS
- raw repetitions retained

| Metric | Control | Aux candidate |
|---|---:|---:|
| median decode | 2.544450 s | **1.416287 s** |
| robust CV | 0.3249% | **0.0818%** |

Paired candidate/control time ratio:

- point estimate: **0.557218**
- bootstrap 95% CI: **[0.553921, 0.559169]**

Equivalent point speedup:

- **1.7946× whole-codec decode**

The entire confidence interval is far beyond the pre-registered 2% practical-speed gate.

Ruling for this target:

> **PASS_SPEED_GATE.**

This is materially stronger evidence than the prior stage-only benchmark because the timing includes ANVIL's actual postcoder, framing, allocation and inverse-BWT path.

### 3.3 Encode

Paired encode result:

| Metric | Control | Aux candidate |
|---|---:|---:|
| median encode | 2.545393 s | 2.542066 s |
| robust CV | 0.0525% | 0.4167% |

Paired candidate/control ratio:

- point estimate: **0.998855**
- 95% CI: **[0.993930, 1.001095]**

The CI crosses 1.0 and does not clear the 2% practical threshold.

Ruling:

> **NO_SPEED_GATE** — no material encode gain or penalty measured.

Every timed encode produced the same deterministic bytes and SHA-256 for its respective arm.

---

## 4. Dickens paired experiment

GitHub Actions run: `35919229083`

Measured commit:

- `e76beb65071d9a56a3f786e47dc273a3345b5d57`

Runner:

- AMD EPYC 7763 64-Core Processor;
- timing affinity: CPU 0.

Canonical input:

- source bytes: **10,192,446**
- SHA-256: `b24c37886142e11d0ee687db6ab06f936207aa7f2ea1fd1d9a36763c7a507e6a`

### 4.1 Charged wire cost

| Arm | Encoded bytes | Delta |
|---|---:|---:|
| control, aux OFF | 2,571,873 | — |
| candidate, aux ON | 2,574,367 | **+2,494** |

The integrated overhead is:

- **+2,494 B**
- **+0.024469% of source size**
- approximately **+0.09697% of the control compressed payload**

The I9 prototype predicted a 2,492-B raw index for this policy, so complete integrated framing is only 2 B above the raw-index count.

### 4.2 Whole-codec decode

| Metric | Control | Aux candidate |
|---|---:|---:|
| median decode | 0.607222 s | **0.443266 s** |
| robust CV | 7.293% | 2.082% |

Paired candidate/control ratio:

- point: **0.732896**
- 95% CI: **[0.684844, 0.783031]**
- point speedup: **1.3645×**
- ambient robust CV: **1.335%**
- timing-valid: **yes**

Ruling:

> **PASS_SPEED_GATE.**

The effect is smaller and noisier than on `webster`, but the entire CI remains comfortably beyond the 2% practical threshold.

### 4.3 Encode

Paired encode:

- control median: **0.757793 s**
- candidate median: **0.751732 s**
- candidate/control ratio: **0.988795**
- 95% CI: **[0.987006, 1.001520]**
- ambient robust CV: **0.228%**

Ruling:

> **NO_SPEED_GATE** — again, no reliable material encode difference.

---

## 5. enwik8 paired experiment

GitHub Actions run: `35919233744`

Measured commit:

- `e76beb65071d9a56a3f786e47dc273a3345b5d57`

Runner:

- AMD EPYC 9V74 80-Core Processor;
- timing affinity: CPU 0.

Canonical input:

- source bytes: **100,000,000**
- SHA-256: `2b49720ec4d78c3c9fabaee6e4179a5e997302b3a70029f30f2d582218c024a8`

### 5.1 Charged wire cost

| Arm | Encoded bytes | Delta |
|---|---:|---:|
| control, aux OFF | 23,534,368 | — |
| candidate, aux ON | 23,537,422 | **+3,054** |

Integrated overhead:

- **+3,054 B**
- **+0.003054% of source size**
- approximately **+0.01297% of the control compressed payload**

The I9 prototype predicted a **3,052-B** raw index at the same policy. Complete integrated framing is only 2 B above that count.

### 5.2 Whole-codec decode

| Metric | Control | Aux candidate |
|---|---:|---:|
| median decode | 8.273907 s | **3.538773 s** |
| robust CV | 2.217% | **0.504%** |

Paired candidate/control ratio:

- point: **0.427489**
- 95% CI: **[0.421034, 0.432620]**
- point speedup: **2.3392×**
- ambient robust CV: **0.392%**
- timing-valid: **yes**

Ruling:

> **PASS_SPEED_GATE.**

This is the largest integrated whole-codec gain of the three measured targets.

### 5.3 Encode

Paired encode:

- control median: **7.239934 s**
- candidate median: **7.202237 s**
- candidate/control ratio: **0.996899**
- 95% CI: **[0.992437, 1.011097]**
- ambient robust CV: **1.199%**

Ruling:

> **NO_SPEED_GATE** — no reliable material encode difference.

---

## 6. Cross-target synthesis and relation to I9

| Target | Integrated aux wire delta | Whole-decode speedup | Decode ruling | Encode ruling |
|---|---:|---:|---|---|
| dickens | +2,494 B | **1.364×** | PASS | neutral |
| webster | +2,535 B | **1.795×** | PASS | neutral |
| enwik8 | +3,054 B | **2.339×** | PASS | neutral |

Across these three independent experiments:

- total charged auxiliary wire increase: **8,083 B**;
- each complete integrated wire landed only **2–3 B above** the prototype's raw auxiliary-index prediction;
- every target cleared the pre-registered 2% whole-decode speed gate;
- no target showed a reliable material encode penalty.

The old I9 stage measurements projected approximately:

- `dickens`: 1.56–1.71× whole decode;
- `webster`: 1.82–1.91×;
- `enwik8`: ~1.9×.

The integrated results span below and above those projections:

- `dickens` measured **1.364×**;
- `webster` measured **1.795×**;
- `enwik8` measured **2.339×**.

That spread is expected because the old whole-codec values were derived from separately measured stage fractions. The important result is mechanism-level consistency: every integrated target moves strongly in the predicted direction once the exact index bytes are charged.

Evidence is now substantially stronger than I9 because it is:

- integrated production code;
- exact charged wire;
- actual whole-codec decode;
- same-job paired A/B;
- one-thread resource parity;
- raw repetitions retained;
- ambient noise measured rather than filtered away.

---

## 7. What these results do and do not establish

The three completed canonical targets establish that:

1. auxiliary-index inverse BWT survives integration;
2. legacy/default-off behavior remains available;
3. complete wire cost is tiny;
4. whole-codec decode improves dramatically;
5. encode cost is effectively neutral within this experiment;
6. the gain is not a hidden thread-count change.

It does **not yet** justify changing ANVIL's default wire representation globally.

### 7.1 Candidate-branch default-off identity closure

The full canonical default-off reruns have now completed successfully:

- Silesia: run `35918898984` — **SUCCESS**;
- enwik8: run `35918904873` — **SUCCESS**.

Their raw CSV artifacts were downloaded and passed through
`tools/close_i10_remote_baseline.py` together. Hard ANVIL byte identity passed
**per file**, not merely in aggregate:

| Corpus | Candidate-branch `anvil-auto-direct` | Frozen I9 | Delta |
|---|---:|---:|---:|
| Silesia | 46,446,995 B | 46,446,995 B | **0 B** |
| enwik8 | 23,534,368 B | 23,534,368 B | **0 B** |

The candidate branch's `anvil-ratio-auto` totals also equal those same
auto-direct totals on both corpora. Every observed row roundtripped.

Brotli bytes were also identical to the frozen references. The hosted Linux
Zstd/xz rows differed slightly from the historical Windows-era reference rows,
as expected for a different toolchain/library series; those differences are
recorded as series/toolchain observations and are not ANVIL regressions.

This closes the most important compatibility gate: **adding the auxiliary
decoder/wire support does not perturb the legacy/default-off representation.**

Before promotion:

- close the corpus-wide routing/byte audit;
- decide whether auxiliary framing should be:
  - default for BWT,
  - frontier/routing-selected,
  - or remain an explicit representation option.

---

## 8. Active follow-ups

Paired experiments:

- `webster`: run `35918797463` — **PASS_SPEED_GATE**;
- `dickens`: run `35919229083` — **PASS_SPEED_GATE**;
- `enwik8`: run `35919233744` — **PASS_SPEED_GATE**.

Candidate default-off canonical identity runs:

- Silesia: `35918898984` — **SUCCESS / byte-identical to frozen I9**;
- enwik8: `35918904873` — **SUCCESS / byte-identical to frozen I9**.

Corpus-wide exact routing/byte audit:

- run `35920128901` — Silesia still running;
- enwik8 cell complete:
  - control auto: **23,534,368 B**;
  - aux auto: **23,537,422 B**;
  - delta: **+3,054 B** / **+0.003054% of source**;
  - route remains BWT;
  - no routing change.

The portfolio run uses the unchanged measured codec source plus benchmark-tooling-only follow-up commits.

No I10-1B DEFLATE-replay source work will be combined with this branch. I10-1A remains causally isolated until its ruling is complete.
