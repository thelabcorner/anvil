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

## 4. Relation to the I9 projection

The I9 prototype measured the inverse-BWT stage at roughly 3.49× faster on `webster` and projected whole-codec decode around 1.82–1.91×.

The first integrated remote measurement is **1.795×**.

That is slightly below the old projected band but directionally and mechanistically consistent. The important change in evidence class is that this is now:

- integrated production code;
- exact charged wire;
- actual whole-codec decode;
- same-job paired A/B;
- one-thread resource parity;
- low observed timing noise.

The result therefore validates the central I10-1A hypothesis on `webster`.

---

## 5. What this result does and does not establish

It establishes on one canonical target that:

1. auxiliary-index inverse BWT survives integration;
2. legacy/default-off behavior remains available;
3. complete wire cost is tiny;
4. whole-codec decode improves dramatically;
5. encode cost is effectively neutral within this experiment;
6. the gain is not a hidden thread-count change.

It does **not yet** justify changing ANVIL's default wire representation globally.

Before promotion:

- reproduce on `dickens`;
- reproduce on `enwik8`;
- close the candidate branch's full Silesia/enwik8 default-off byte-identity runs;
- inspect corpus-wide routing economics;
- decide whether auxiliary framing should be:
  - default for BWT,
  - frontier/routing-selected,
  - or remain an explicit representation option.

---

## 6. Active follow-ups

Dispatched paired experiments:

- `dickens`: run `35919229083`
- `enwik8`: run `35919233744`

Candidate default-off canonical identity runs:

- Silesia: `35918898984`
- enwik8: `35918904873`

No I10-1B DEFLATE-replay source work will be combined with this branch. I10-1A remains causally isolated until its ruling is complete.
